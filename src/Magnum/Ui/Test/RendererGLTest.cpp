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

#include <sstream> /** @todo remove once Debug is stream-free */
#include <Corrade/Containers/StringStl.h> /** @todo remove once Debug is stream-free */
#include <Corrade/TestSuite/Compare/String.h>
#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/OpenGLTester.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/Math/Vector4.h>

#include "Magnum/Ui/RendererGL.h"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct RendererGLTest: GL::OpenGLTester {
    explicit RendererGLTest();

    void construct();
    void constructCompositingFramebuffer();
    void constructCopy();
    void constructMove();

    void compositingFramebuffer();
    void compositingFramebufferNoFramebufferSizeSet();

    void setupTeardown();

    void transition();
    void transitionCompositing();
    void transitionNoScissor();
};

RendererGLTest::RendererGLTest() {
    addTests({&RendererGLTest::construct,
              &RendererGLTest::constructCompositingFramebuffer,
              &RendererGLTest::constructCopy,
              &RendererGLTest::constructMove,

              &RendererGLTest::compositingFramebuffer,
              &RendererGLTest::compositingFramebufferNoFramebufferSizeSet});

    addTests({&RendererGLTest::transition,
              &RendererGLTest::transitionCompositing,
              &RendererGLTest::transitionNoScissor},
              &RendererGLTest::setupTeardown,
              &RendererGLTest::setupTeardown);
}

void RendererGLTest::construct() {
    RendererGL renderer;
    CORRADE_COMPARE(renderer.flags(), RendererGL::Flags{});
    CORRADE_COMPARE(renderer.features(), RendererFeatures{});
}

void RendererGLTest::constructCompositingFramebuffer() {
    RendererGL renderer{RendererGL::Flag::CompositingFramebuffer};
    CORRADE_COMPARE(renderer.flags(), RendererGL::Flag::CompositingFramebuffer);
    CORRADE_COMPARE(renderer.features(), RendererFeature::Composite);

    /* Queries tested in compositingFramebuffer() as they need also size set */
}

void RendererGLTest::constructCopy() {
    CORRADE_VERIFY(!std::is_copy_constructible<RendererGL>{});
    CORRADE_VERIFY(!std::is_copy_assignable<RendererGL>{});
}

void RendererGLTest::constructMove() {
    RendererGL a{RendererGL::Flag::CompositingFramebuffer};
    a.setupFramebuffers({15, 37});

    RendererGL b = Utility::move(a);
    CORRADE_COMPARE(b.flags(), RendererGL::Flag::CompositingFramebuffer);
    CORRADE_COMPARE(b.framebufferSize(), (Vector2i{15, 37}));

    RendererGL c;
    c = Utility::move(b);
    CORRADE_COMPARE(c.flags(), RendererGL::Flag::CompositingFramebuffer);
    CORRADE_COMPARE(c.framebufferSize(), (Vector2i{15, 37}));

    CORRADE_VERIFY(std::is_nothrow_move_constructible<RendererGL>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<RendererGL>::value);
}

void RendererGLTest::compositingFramebuffer() {
    RendererGL renderer{RendererGL::Flag::CompositingFramebuffer};
    const RendererGL& crenderer = renderer;

    /* The objects are created on the first framebuffer size setup */
    renderer.setupFramebuffers({200, 300});
    UnsignedInt framebufferId = renderer.compositingFramebuffer().id();
    UnsignedInt textureId = renderer.compositingTexture().id();
    MAGNUM_VERIFY_NO_GL_ERROR();
    CORRADE_VERIFY(framebufferId);
    CORRADE_COMPARE(crenderer.compositingFramebuffer().id(), framebufferId);
    CORRADE_COMPARE(renderer.compositingFramebuffer().viewport(), (Range2Di{{}, {200, 300}}));
    CORRADE_VERIFY(textureId);
    CORRADE_COMPARE(crenderer.compositingTexture().id(), textureId);
    /* Nothing else to verify on the texture */

    /* They get recreated from scratch on resize */
    renderer.setupFramebuffers({150, 200});
    MAGNUM_VERIFY_NO_GL_ERROR();
    CORRADE_VERIFY(renderer.compositingFramebuffer().id());
    CORRADE_VERIFY(renderer.compositingFramebuffer().id() != framebufferId);
    CORRADE_COMPARE(renderer.compositingFramebuffer().viewport(), (Range2Di{{}, {150, 200}}));
    CORRADE_VERIFY(renderer.compositingTexture().id());
    CORRADE_VERIFY(renderer.compositingTexture().id() != textureId);
}

void RendererGLTest::compositingFramebufferNoFramebufferSizeSet() {
    CORRADE_SKIP_IF_NO_ASSERT();

    RendererGL renderer{RendererGL::Flag::CompositingFramebuffer};
    const RendererGL& crenderer = renderer;

    std::ostringstream out;
    Error redirectError{&out};
    renderer.compositingFramebuffer();
    crenderer.compositingFramebuffer();
    renderer.compositingTexture();
    crenderer.compositingTexture();
    CORRADE_COMPARE_AS(out.str(),
        "Ui::RendererGL::compositingFramebuffer(): framebuffer size wasn't set up\n"
        "Ui::RendererGL::compositingFramebuffer(): framebuffer size wasn't set up\n"
        "Ui::RendererGL::compositingTexture(): framebuffer size wasn't set up\n"
        "Ui::RendererGL::compositingTexture(): framebuffer size wasn't set up\n",
        TestSuite::Compare::String);
}

void RendererGLTest::setupTeardown() {
    GL::Renderer::disable(GL::Renderer::Feature::Blending);
    GL::Renderer::disable(GL::Renderer::Feature::ScissorTest);
    GL::defaultFramebuffer.bind();
}

void RendererGLTest::transition() {
    Vector4i defaultScissorRect{};
    glGetIntegerv(GL_SCISSOR_BOX, defaultScissorRect.data());
    MAGNUM_VERIFY_NO_GL_ERROR();

    Int currentBlending = 1, currentScissor = 1, currentFramebuffer = 999;
    Vector4i currentScissorRect{};

    RendererGL renderer;
    renderer.setupFramebuffers({15, 37});

    glGetIntegerv(GL_BLEND, &currentBlending);
    glGetIntegerv(GL_SCISSOR_TEST, &currentScissor);
    glGetIntegerv(GL_SCISSOR_BOX, currentScissorRect.data());
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &currentFramebuffer);
    MAGNUM_VERIFY_NO_GL_ERROR();
    CORRADE_VERIFY(!currentBlending);
    CORRADE_VERIFY(!currentScissor);
    CORRADE_COMPARE(currentScissorRect, defaultScissorRect);
    /* Currently bound framebuffer should not be changed at all during the
       whole lifetime */
    CORRADE_COMPARE(currentFramebuffer, 0);

    glScissor(0, 1, 2, 3);

    renderer.transition(RendererTargetState::Draw, RendererDrawState::Blending);
    glGetIntegerv(GL_BLEND, &currentBlending);
    glGetIntegerv(GL_SCISSOR_TEST, &currentScissor);
    glGetIntegerv(GL_SCISSOR_BOX, currentScissorRect.data());
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &currentFramebuffer);
    MAGNUM_VERIFY_NO_GL_ERROR();
    CORRADE_VERIFY(currentBlending);
    CORRADE_VERIFY(!currentScissor);
    CORRADE_COMPARE(currentScissorRect, (Vector4i{0, 1, 2, 3}));
    CORRADE_COMPARE(currentFramebuffer, 0);

    renderer.transition(RendererTargetState::Draw, RendererDrawState::Scissor);
    glGetIntegerv(GL_BLEND, &currentBlending);
    glGetIntegerv(GL_SCISSOR_TEST, &currentScissor);
    glGetIntegerv(GL_SCISSOR_BOX, currentScissorRect.data());
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &currentFramebuffer);
    MAGNUM_VERIFY_NO_GL_ERROR();
    CORRADE_VERIFY(!currentBlending);
    CORRADE_VERIFY(currentScissor);
    CORRADE_COMPARE(currentScissorRect, (Vector4i{0, 1, 2, 3}));
    CORRADE_COMPARE(currentFramebuffer, 0);

    renderer.transition(RendererTargetState::Draw, RendererDrawState::Blending|RendererDrawState::Scissor);
    glGetIntegerv(GL_BLEND, &currentBlending);
    glGetIntegerv(GL_SCISSOR_TEST, &currentScissor);
    glGetIntegerv(GL_SCISSOR_BOX, currentScissorRect.data());
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &currentFramebuffer);
    MAGNUM_VERIFY_NO_GL_ERROR();
    CORRADE_VERIFY(currentBlending);
    CORRADE_VERIFY(currentScissor);
    CORRADE_COMPARE(currentScissorRect, (Vector4i{0, 1, 2, 3}));
    CORRADE_COMPARE(currentFramebuffer, 0);

    renderer.transition(RendererTargetState::Draw, {});
    glGetIntegerv(GL_BLEND, &currentBlending);
    glGetIntegerv(GL_SCISSOR_TEST, &currentScissor);
    glGetIntegerv(GL_SCISSOR_BOX, currentScissorRect.data());
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &currentFramebuffer);
    MAGNUM_VERIFY_NO_GL_ERROR();
    CORRADE_VERIFY(!currentBlending);
    CORRADE_VERIFY(!currentScissor);
    CORRADE_COMPARE(currentScissorRect, (Vector4i{0, 1, 2, 3}));
    CORRADE_COMPARE(currentFramebuffer, 0);

    renderer.transition(RendererTargetState::Final, {});
    glGetIntegerv(GL_BLEND, &currentBlending);
    glGetIntegerv(GL_SCISSOR_TEST, &currentScissor);
    glGetIntegerv(GL_SCISSOR_BOX, currentScissorRect.data());
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &currentFramebuffer);
    MAGNUM_VERIFY_NO_GL_ERROR();
    CORRADE_VERIFY(!currentBlending);
    CORRADE_VERIFY(!currentScissor);
    /* Scissor rectangle should get reset to the full framebuffer size */
    CORRADE_COMPARE(currentScissorRect, (Vector4i{0, 0, 15, 37}));
    CORRADE_COMPARE(currentFramebuffer, 0);
}

void RendererGLTest::transitionCompositing() {
    Int currentFramebuffer = 999;

    GL::Framebuffer anotherFramebuffer{{}};

    RendererGL renderer{RendererGL::Flag::CompositingFramebuffer};
    renderer.setupFramebuffers({15, 37});

    /* Setting up the renderer should not bind the framebuffer yet */
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &currentFramebuffer);
    MAGNUM_VERIFY_NO_GL_ERROR();
    CORRADE_COMPARE(currentFramebuffer, 0);

    /* Transitioning to the Initial state does nothing */
    renderer.transition(RendererTargetState::Initial, {});
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &currentFramebuffer);
    MAGNUM_VERIFY_NO_GL_ERROR();
    CORRADE_COMPARE(currentFramebuffer, 0);

    /* Transitioning to a Draw state will bind it */
    anotherFramebuffer.bind();
    renderer.transition(RendererTargetState::Draw, {});
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &currentFramebuffer);
    MAGNUM_VERIFY_NO_GL_ERROR();
    CORRADE_COMPARE(currentFramebuffer, renderer.compositingFramebuffer().id());

    /* Transitioning to a Composite state will not bind it */
    anotherFramebuffer.bind();
    renderer.transition(RendererTargetState::Composite, {});
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &currentFramebuffer);
    MAGNUM_VERIFY_NO_GL_ERROR();
    CORRADE_COMPARE(currentFramebuffer, anotherFramebuffer.id());

    /* Transitioning to a Draw state will bind it back */
    renderer.transition(RendererTargetState::Draw, {});
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &currentFramebuffer);
    MAGNUM_VERIFY_NO_GL_ERROR();
    CORRADE_COMPARE(currentFramebuffer, renderer.compositingFramebuffer().id());

    /* Transitioning to a Final state will rebind it again. Which shouldn't be
       needed in practice, but the state tracker will deal with that
       redundancy. */
    anotherFramebuffer.bind();
    renderer.transition(RendererTargetState::Final, {});
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &currentFramebuffer);
    MAGNUM_VERIFY_NO_GL_ERROR();
    CORRADE_COMPARE(currentFramebuffer, renderer.compositingFramebuffer().id());
}

void RendererGLTest::transitionNoScissor() {
    /* Compared to transition() this doesn't touch scissor state, which means
       it won't get reset at the end */

    Int currentBlending = 1, currentScissor = 1;
    Vector4i currentScissorRect{};

    RendererGL renderer;
    renderer.setupFramebuffers({15, 37});

    glEnable(GL_SCISSOR_TEST);
    glScissor(0, 1, 2, 3);
    MAGNUM_VERIFY_NO_GL_ERROR();

    renderer.transition(RendererTargetState::Draw, RendererDrawState::Blending);
    glGetIntegerv(GL_BLEND, &currentBlending);
    glGetIntegerv(GL_SCISSOR_TEST, &currentScissor);
    glGetIntegerv(GL_SCISSOR_BOX, currentScissorRect.data());
    MAGNUM_VERIFY_NO_GL_ERROR();
    CORRADE_VERIFY(currentBlending);
    CORRADE_VERIFY(currentScissor); /* enabled outside of the RendererGL */
    CORRADE_COMPARE(currentScissorRect, (Vector4i{0, 1, 2, 3}));

    renderer.transition(RendererTargetState::Draw, {});
    glGetIntegerv(GL_BLEND, &currentBlending);
    glGetIntegerv(GL_SCISSOR_TEST, &currentScissor);
    glGetIntegerv(GL_SCISSOR_BOX, currentScissorRect.data());
    MAGNUM_VERIFY_NO_GL_ERROR();
    CORRADE_VERIFY(!currentBlending);
    CORRADE_VERIFY(currentScissor); /* enabled outside of the RendererGL */
    CORRADE_COMPARE(currentScissorRect, (Vector4i{0, 1, 2, 3}));

    renderer.transition(RendererTargetState::Final, {});
    glGetIntegerv(GL_BLEND, &currentBlending);
    glGetIntegerv(GL_SCISSOR_TEST, &currentScissor);
    glGetIntegerv(GL_SCISSOR_BOX, currentScissorRect.data());
    MAGNUM_VERIFY_NO_GL_ERROR();
    CORRADE_VERIFY(!currentBlending);
    /* Because the renderer thinks scissor wasn't used (and thus updated) by
       any layer, it won't reset it back to the whole size (and won't disable
       it either) */
    CORRADE_VERIFY(currentScissor);
    CORRADE_COMPARE(currentScissorRect, (Vector4i{0, 1, 2, 3}));
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::RendererGLTest)
