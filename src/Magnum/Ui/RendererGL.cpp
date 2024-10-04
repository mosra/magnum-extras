/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023, 2024
              Vladimír Vondruš <mosra@centrum.cz>

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

#include "RendererGL.h"

#include <Corrade/Containers/EnumSet.hpp>
#include <Corrade/Utility/Assert.h>
#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/Math/Range.h>

namespace Magnum { namespace Ui {

Debug& operator<<(Debug& debug, const RendererGL::Flag value) {
    debug << "Ui::RendererGL::Flag" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case RendererGL::Flag::value: return debug << "::" #value;
        _c(CompositingFramebuffer)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "(" << Debug::nospace << Debug::hex << UnsignedByte(value) << Debug::nospace << ")";
}

Debug& operator<<(Debug& debug, const RendererGL::Flags value) {
    return Containers::enumSetDebugOutput(debug, value, "Ui::RendererGL::Flags{}", {
        RendererGL::Flag::CompositingFramebuffer
    });
}

struct RendererGL::State {
    explicit State(Flags flags): flags{flags} {}

    bool scissorUsed = false;
    Flags flags;
    GL::Texture2D compositingTexture{NoCreate};
    GL::Framebuffer compositingFramebuffer{NoCreate};
};

RendererGL::RendererGL(const Flags flags): _state{InPlaceInit, flags} {}

RendererGL::RendererGL(RendererGL&&) noexcept = default;

RendererGL::~RendererGL() = default;

RendererGL& RendererGL::operator=(RendererGL&&) noexcept = default;

RendererGL::Flags RendererGL::flags() const { return _state->flags; }

const GL::Framebuffer& RendererGL::compositingFramebuffer() const {
    const State& state = *_state;
    CORRADE_ASSERT(state.flags & Flag::CompositingFramebuffer,
        "Ui::RendererGL::compositingFramebuffer(): compositing framebuffer not enabled", state.compositingFramebuffer);
    CORRADE_ASSERT(!framebufferSize().isZero(),
        "Ui::RendererGL::compositingFramebuffer(): framebuffer size wasn't set up", state.compositingFramebuffer);
    return state.compositingFramebuffer;
}

GL::Framebuffer& RendererGL::compositingFramebuffer() {
    return const_cast<GL::Framebuffer&>(const_cast<const RendererGL&>(*this).compositingFramebuffer());
}

const GL::Texture2D& RendererGL::compositingTexture() const {
    const State& state = *_state;
    CORRADE_ASSERT(state.flags & Flag::CompositingFramebuffer,
        "Ui::RendererGL::compositingTexture(): compositing framebuffer not enabled", state.compositingTexture);
    CORRADE_ASSERT(!framebufferSize().isZero(),
        "Ui::RendererGL::compositingTexture(): framebuffer size wasn't set up", state.compositingTexture);
    return state.compositingTexture;
}

GL::Texture2D& RendererGL::compositingTexture() {
    return const_cast<GL::Texture2D&>(const_cast<const RendererGL&>(*this).compositingTexture());
}

RendererFeatures RendererGL::doFeatures() const {
    return _state->flags & Flag::CompositingFramebuffer ?
        RendererFeature::Composite : RendererFeatures{};
}

void RendererGL::doSetupFramebuffers(const Vector2i& size) {
    /** @todo recreate only if size changes, and not if size gets smaller?
        would however mean the compositor needs to be aware that there's just a
        subset of the texture being used */
    if(_state->flags & Flag::CompositingFramebuffer) {
        (_state->compositingTexture = GL::Texture2D{})
            .setMinificationFilter(GL::SamplerFilter::Linear, GL::SamplerMipmap::Base)
            .setMagnificationFilter(GL::SamplerFilter::Linear)
            .setWrapping(GL::SamplerWrapping::ClampToEdge)
            .setStorage(1, GL::TextureFormat::RGBA8, size);
        (_state->compositingFramebuffer = GL::Framebuffer{{{}, size}})
            .attachTexture(GL::Framebuffer::ColorAttachment{0}, _state->compositingTexture, 0);
    }
}

void RendererGL::doTransition(RendererTargetState, const RendererTargetState targetStateTo, const RendererDrawStates drawStatesFrom, const RendererDrawStates drawStatesTo) {
    State& state = *_state;

    /* If the compositing framebuffer is active, make sure to bind it when
       transitioning to a layer draw state or to the final state. */
    if(state.flags & Flag::CompositingFramebuffer &&
       (targetStateTo == RendererTargetState::Draw ||
        targetStateTo == RendererTargetState::Final))
    {
        state.compositingFramebuffer.bind();
    }

    /* Flip GL state as appropriate. This does the right thing (i.e., disabling
       both) for compositing transition as well, which is enforced by the base
       class. */
    if((drawStatesFrom >= RendererDrawState::Blending) !=
         (drawStatesTo >= RendererDrawState::Blending)) {
        GL::Renderer::setFeature(GL::Renderer::Feature::Blending, drawStatesTo >= RendererDrawState::Blending);
    }

    if((drawStatesFrom >= RendererDrawState::Scissor) !=
         (drawStatesTo >= RendererDrawState::Scissor)) {
        GL::Renderer::setFeature(GL::Renderer::Feature::ScissorTest, drawStatesTo >= RendererDrawState::Scissor);
        state.scissorUsed = true;
    }

    /* Reset the scissor rect back to the whole framebuffer if scissor test was
       used by any layer in this draw */
    if(targetStateTo == RendererTargetState::Initial) {
        state.scissorUsed = false;
    } else if(targetStateTo == RendererTargetState::Final) {
        if(state.scissorUsed)
            GL::Renderer::setScissor(Range2Di::fromSize({}, framebufferSize()));
    }
}

}}
