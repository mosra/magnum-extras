/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023 Vladimír Vondruš <mosra@centrum.cz>

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

#include <Magnum/GL/Renderer.h>
#include <Magnum/Math/Range.h>

namespace Magnum { namespace Whee {

struct RendererGL::State {
    bool scissorUsed = false;
};

RendererGL::RendererGL(): _state{InPlaceInit} {}

RendererGL::RendererGL(RendererGL&&) noexcept = default;

RendererGL::~RendererGL() = default;

RendererGL& RendererGL::operator=(RendererGL&&) noexcept = default;

RendererFeatures RendererGL::doFeatures() const { return {}; }

void RendererGL::doSetupFramebuffers(const Vector2i&) {}

void RendererGL::doTransition(RendererTargetState, const RendererTargetState targetStateTo, const RendererDrawStates drawStatesFrom, const RendererDrawStates drawStatesTo) {
    State& state = *_state;

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
