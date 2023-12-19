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

#include <Magnum/GL/OpenGLTester.h>
#include <Magnum/Math/Vector4.h>

#include "Magnum/Whee/RendererGL.h"

namespace Magnum { namespace Whee { namespace Test { namespace {

struct RendererGLTest: GL::OpenGLTester {
    explicit RendererGLTest();

    void construct();
    void constructCopy();
    void constructMove();

    void setupTeardown();

    void transition();
    void transitionNoScissor();
};

RendererGLTest::RendererGLTest() {
    addTests({&RendererGLTest::construct,
              &RendererGLTest::constructCopy,
              &RendererGLTest::constructMove});

    addTests({&RendererGLTest::transition,
              &RendererGLTest::transitionNoScissor},
              &RendererGLTest::setupTeardown,
              &RendererGLTest::setupTeardown);
}

void RendererGLTest::construct() {
    RendererGL renderer;

    /* There's nothing else to query on top of AbstractRenderer */
    CORRADE_COMPARE(renderer.currentTargetState(), RendererTargetState::Initial);
}

void RendererGLTest::constructCopy() {
    CORRADE_VERIFY(!std::is_copy_constructible<RendererGL>{});
    CORRADE_VERIFY(!std::is_copy_assignable<RendererGL>{});
}

void RendererGLTest::constructMove() {
    RendererGL a;
    a.setupFramebuffers({15, 37});

    RendererGL b = Utility::move(a);
    /* There's nothing else to query on top of AbstractRenderer */
    CORRADE_COMPARE(b.framebufferSize(), (Vector2i{15, 37}));

    RendererGL c;
    c = Utility::move(b);
    /* There's nothing else to query on top of AbstractRenderer */
    CORRADE_COMPARE(c.framebufferSize(), (Vector2i{15, 37}));

    CORRADE_VERIFY(std::is_nothrow_move_constructible<RendererGL>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<RendererGL>::value);
}

void RendererGLTest::setupTeardown() {
    GL::Renderer::disable(GL::Renderer::Feature::Blending);
    GL::Renderer::disable(GL::Renderer::Feature::ScissorTest);
}

void RendererGLTest::transition() {
    Vector4i defaultScissorRect{};
    glGetIntegerv(GL_SCISSOR_BOX, defaultScissorRect.data());
    MAGNUM_VERIFY_NO_GL_ERROR();

    Int currentBlending = 1, currentScissor = 1;
    Vector4i currentScissorRect{};

    RendererGL renderer;
    renderer.setupFramebuffers({15, 37});

    glGetIntegerv(GL_BLEND, &currentBlending);
    glGetIntegerv(GL_SCISSOR_TEST, &currentScissor);
    glGetIntegerv(GL_SCISSOR_BOX, currentScissorRect.data());
    MAGNUM_VERIFY_NO_GL_ERROR();
    CORRADE_VERIFY(!currentBlending);
    CORRADE_VERIFY(!currentScissor);
    CORRADE_COMPARE(currentScissorRect, defaultScissorRect);

    glScissor(0, 1, 2, 3);

    renderer.transition(RendererTargetState::Draw, RendererDrawState::Blending);
    glGetIntegerv(GL_BLEND, &currentBlending);
    glGetIntegerv(GL_SCISSOR_TEST, &currentScissor);
    glGetIntegerv(GL_SCISSOR_BOX, currentScissorRect.data());
    MAGNUM_VERIFY_NO_GL_ERROR();
    CORRADE_VERIFY(currentBlending);
    CORRADE_VERIFY(!currentScissor);
    CORRADE_COMPARE(currentScissorRect, (Vector4i{0, 1, 2, 3}));

    renderer.transition(RendererTargetState::Draw, RendererDrawState::Scissor);
    glGetIntegerv(GL_BLEND, &currentBlending);
    glGetIntegerv(GL_SCISSOR_TEST, &currentScissor);
    glGetIntegerv(GL_SCISSOR_BOX, currentScissorRect.data());
    MAGNUM_VERIFY_NO_GL_ERROR();
    CORRADE_VERIFY(!currentBlending);
    CORRADE_VERIFY(currentScissor);
    CORRADE_COMPARE(currentScissorRect, (Vector4i{0, 1, 2, 3}));

    renderer.transition(RendererTargetState::Draw, RendererDrawState::Blending|RendererDrawState::Scissor);
    glGetIntegerv(GL_BLEND, &currentBlending);
    glGetIntegerv(GL_SCISSOR_TEST, &currentScissor);
    glGetIntegerv(GL_SCISSOR_BOX, currentScissorRect.data());
    MAGNUM_VERIFY_NO_GL_ERROR();
    CORRADE_VERIFY(currentBlending);
    CORRADE_VERIFY(currentScissor);
    CORRADE_COMPARE(currentScissorRect, (Vector4i{0, 1, 2, 3}));

    renderer.transition(RendererTargetState::Draw, {});
    glGetIntegerv(GL_BLEND, &currentBlending);
    glGetIntegerv(GL_SCISSOR_TEST, &currentScissor);
    glGetIntegerv(GL_SCISSOR_BOX, currentScissorRect.data());
    MAGNUM_VERIFY_NO_GL_ERROR();
    CORRADE_VERIFY(!currentBlending);
    CORRADE_VERIFY(!currentScissor);
    CORRADE_COMPARE(currentScissorRect, (Vector4i{0, 1, 2, 3}));

    renderer.transition(RendererTargetState::Final, {});
    glGetIntegerv(GL_BLEND, &currentBlending);
    glGetIntegerv(GL_SCISSOR_TEST, &currentScissor);
    glGetIntegerv(GL_SCISSOR_BOX, currentScissorRect.data());
    MAGNUM_VERIFY_NO_GL_ERROR();
    CORRADE_VERIFY(!currentBlending);
    CORRADE_VERIFY(!currentScissor);
    /* Scissor rectangle should get reset to the full framebuffer size */
    CORRADE_COMPARE(currentScissorRect, (Vector4i{0, 0, 15, 37}));
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

CORRADE_TEST_MAIN(Magnum::Whee::Test::RendererGLTest)
