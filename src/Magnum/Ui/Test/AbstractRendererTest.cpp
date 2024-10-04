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
#include <Corrade/Containers/GrowableArray.h>
#include <Corrade/Containers/Pair.h>
#include <Corrade/Containers/StringStl.h> /** @todo remove once Debug is stream-free */
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/Container.h>
#include <Corrade/TestSuite/Compare/String.h>
#include <Corrade/Utility/DebugStl.h> /** @todo remove once Debug is stream-free */
#include <Magnum/Math/Vector2.h>

#include "Magnum/Ui/AbstractRenderer.h"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct AbstractRendererTest: TestSuite::Tester {
    explicit AbstractRendererTest();

    void debugFeature();
    void debugFeatures();

    void debugTargetState();

    void debugDrawState();
    void debugDrawStates();

    void construct();
    void constructCopy();
    void constructMove();

    void setupFramebuffers();
    void setupFramebuffersInvalid();

    void transition();
    void transitionInvalid();
    void transitionNoFramebufferSetup();
    void transitionCompositeNotSupported();
};

AbstractRendererTest::AbstractRendererTest() {
    addTests({&AbstractRendererTest::debugFeature,
              &AbstractRendererTest::debugFeatures,

              &AbstractRendererTest::debugTargetState,

              &AbstractRendererTest::debugDrawState,
              &AbstractRendererTest::debugDrawStates,

              &AbstractRendererTest::construct,
              &AbstractRendererTest::constructCopy,
              &AbstractRendererTest::constructMove,

              &AbstractRendererTest::setupFramebuffers,
              &AbstractRendererTest::setupFramebuffersInvalid,

              &AbstractRendererTest::transition,
              &AbstractRendererTest::transitionInvalid,
              &AbstractRendererTest::transitionNoFramebufferSetup,
              &AbstractRendererTest::transitionCompositeNotSupported});
}

void AbstractRendererTest::debugFeature() {
    std::ostringstream out;
    Debug{&out} << RendererFeature::Composite << RendererFeature(0xbe);
    CORRADE_COMPARE(out.str(), "Ui::RendererFeature::Composite Ui::RendererFeature(0xbe)\n");
}

void AbstractRendererTest::debugFeatures() {
    std::ostringstream out;
    Debug{&out} << (RendererFeature::Composite|RendererFeature(0xb0)) << RendererFeatures{};
    CORRADE_COMPARE(out.str(), "Ui::RendererFeature::Composite|Ui::RendererFeature(0xb0) Ui::RendererFeatures{}\n");
}

void AbstractRendererTest::debugTargetState() {
    std::ostringstream out;
    Debug{&out} << RendererTargetState::Draw << RendererTargetState(0xbe);
    CORRADE_COMPARE(out.str(), "Ui::RendererTargetState::Draw Ui::RendererTargetState(0xbe)\n");
}

void AbstractRendererTest::debugDrawState() {
    std::ostringstream out;
    Debug{&out} << RendererDrawState::Blending << RendererDrawState(0xbe);
    CORRADE_COMPARE(out.str(), "Ui::RendererDrawState::Blending Ui::RendererDrawState(0xbe)\n");
}

void AbstractRendererTest::debugDrawStates() {
    std::ostringstream out;
    Debug{&out} << (RendererDrawState::Scissor|RendererDrawState(0xe0)) << RendererDrawStates{};
    CORRADE_COMPARE(out.str(), "Ui::RendererDrawState::Scissor|Ui::RendererDrawState(0xe0) Ui::RendererDrawStates{}\n");
}

void AbstractRendererTest::construct() {
    struct: AbstractRenderer {
        RendererFeatures doFeatures() const override { return RendererFeatures{0x80}; }
        void doSetupFramebuffers(const Vector2i&) override {}
        void doTransition(RendererTargetState, RendererTargetState, RendererDrawStates, RendererDrawStates) override {}
    } renderer;
    CORRADE_COMPARE(renderer.features(), RendererFeatures{0x80});
    CORRADE_COMPARE(renderer.framebufferSize(), Vector2i{});
    CORRADE_COMPARE(renderer.currentTargetState(), RendererTargetState::Initial);
    CORRADE_COMPARE(renderer.currentDrawStates(), RendererDrawStates{});
}

void AbstractRendererTest::constructCopy() {
    CORRADE_VERIFY(!std::is_copy_constructible<AbstractRenderer>{});
    CORRADE_VERIFY(!std::is_copy_assignable<AbstractRenderer>{});
}

void AbstractRendererTest::constructMove() {
    struct Renderer: AbstractRenderer {
        Renderer(RendererFeatures features): _features{features} {}
        RendererFeatures doFeatures() const override { return _features; }
        void doSetupFramebuffers(const Vector2i&) override {}
        void doTransition(RendererTargetState, RendererTargetState, RendererDrawStates, RendererDrawStates) override {}

        private:
            RendererFeatures _features;
    };

    /** @todo use a real value once it exists */
    Renderer a{RendererFeatures{0x8f}};

    Renderer b{Utility::move(a)};
    CORRADE_COMPARE(b.features(), RendererFeatures{0x8f});

    Renderer c{RendererFeatures{0xf8}};
    c = Utility::move(b);
    CORRADE_COMPARE(c.features(), RendererFeatures{0x8f});

    CORRADE_VERIFY(std::is_nothrow_move_constructible<Renderer>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<Renderer>::value);
}

void AbstractRendererTest::setupFramebuffers() {
    struct: AbstractRenderer {
        RendererFeatures doFeatures() const override { return {}; }
        void doSetupFramebuffers(const Vector2i& size) override {
            CORRADE_COMPARE(size, (called ? Vector2i{37, 15} : Vector2i{15, 37}));
            ++called;
        }
        void doTransition(RendererTargetState, RendererTargetState, RendererDrawStates, RendererDrawStates) override {}

        Int called = 0;
    } renderer;
    CORRADE_COMPARE(renderer.framebufferSize(), Vector2i{});

    renderer.setupFramebuffers({15, 37});
    CORRADE_COMPARE(renderer.framebufferSize(), (Vector2i{15, 37}));
    CORRADE_COMPARE(renderer.called, 1);

    /* Should be allowed also if in the Final state */
    renderer.transition(RendererTargetState::Final, {});
    renderer.setupFramebuffers({37, 15});
    CORRADE_COMPARE(renderer.framebufferSize(), (Vector2i{37, 15}));
    CORRADE_COMPARE(renderer.called, 2);
}

void AbstractRendererTest::setupFramebuffersInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct
        /* MSVC 2017 (_MSC_VER == 191x) crashes at runtime accessing instance2
           in the following case. Not a problem in MSVC 2015 or 2019+.
            struct: SomeBase {
            } instance1, instance2;
           Simply naming the derived struct is enough to fix the crash, FFS. */
        #if defined(CORRADE_TARGET_MSVC) && _MSC_VER >= 1910 && _MSC_VER < 1920
        ThisNameAlonePreventsMSVC2017FromBlowingUp
        #endif
        : AbstractRenderer
    {
        RendererFeatures doFeatures() const override { return {}; }
        void doSetupFramebuffers(const Vector2i&) override {}
        void doTransition(RendererTargetState, RendererTargetState, RendererDrawStates, RendererDrawStates) override {}
    } initial, draw;

    /* Transition needs a framebuffer size set up */
    draw.setupFramebuffers({15, 37});
    draw.transition(RendererTargetState::Draw, {});

    std::ostringstream out;
    Error redirectError{&out};
    initial.setupFramebuffers({0, 13});
    initial.setupFramebuffers({14, 0});
    draw.setupFramebuffers({15, 37});
    CORRADE_COMPARE_AS(out.str(),
        "Ui::AbstractRenderer::setupFramebuffers(): expected non-zero size, got {0, 13}\n"
        "Ui::AbstractRenderer::setupFramebuffers(): expected non-zero size, got {14, 0}\n"
        "Ui::AbstractRenderer::setupFramebuffers(): not allowed to be called in Ui::RendererTargetState::Draw\n",
        TestSuite::Compare::String);
}

void AbstractRendererTest::transition() {
    struct: AbstractRenderer {
        RendererFeatures doFeatures() const override {
            return RendererFeature::Composite;
        }
        void doSetupFramebuffers(const Vector2i&) override {}
        void doTransition(RendererTargetState targetStateFrom, RendererTargetState targetStateTo, RendererDrawStates drawStatesFrom, RendererDrawStates drawStatesTo) override {
            /* The current*() values shouldn't be overwritten during this call
               yet */
            CORRADE_COMPARE(targetStateFrom, currentTargetState());
            CORRADE_COMPARE(drawStatesFrom, currentDrawStates());

            arrayAppend(called, InPlaceInit,
                Containers::pair(targetStateFrom, targetStateTo),
                Containers::pair(drawStatesFrom, drawStatesTo));
        }

        Containers::Array<Containers::Pair<Containers::Pair<RendererTargetState, RendererTargetState>, Containers::Pair<RendererDrawStates, RendererDrawStates>>> called;
    } renderer;
    CORRADE_COMPARE(renderer.currentTargetState(), RendererTargetState::Initial);
    CORRADE_COMPARE(renderer.currentDrawStates(), RendererDrawStates{});

    /* Transition needs a non-zero size */
    renderer.setupFramebuffers({15, 37});

    /* Transition to Initial is a no-op */
    renderer.transition(RendererTargetState::Initial, {});
    CORRADE_COMPARE(renderer.currentTargetState(), RendererTargetState::Initial);
    CORRADE_COMPARE(renderer.currentDrawStates(), RendererDrawStates{});

    /* Transition to a layer drawing */
    renderer.transition(RendererTargetState::Draw, {});
    CORRADE_COMPARE(renderer.currentTargetState(), RendererTargetState::Draw);
    CORRADE_COMPARE(renderer.currentDrawStates(), RendererDrawStates{});

    /* Another draw with different draw states */
    renderer.transition(RendererTargetState::Draw, RendererDrawState::Blending|RendererDrawState::Scissor);
    CORRADE_COMPARE(renderer.currentTargetState(), RendererTargetState::Draw);
    CORRADE_COMPARE(renderer.currentDrawStates(), RendererDrawState::Blending|RendererDrawState::Scissor);

    /* Another draw with the same draw states, doesn't get propagated */
    renderer.transition(RendererTargetState::Draw, RendererDrawState::Blending|RendererDrawState::Scissor);
    CORRADE_COMPARE(renderer.currentTargetState(), RendererTargetState::Draw);
    CORRADE_COMPARE(renderer.currentDrawStates(), RendererDrawState::Blending|RendererDrawState::Scissor);

    /* Another draw with draw state subset, should get propagated again */
    renderer.transition(RendererTargetState::Draw, RendererDrawState::Scissor);
    CORRADE_COMPARE(renderer.currentTargetState(), RendererTargetState::Draw);
    CORRADE_COMPARE(renderer.currentDrawStates(), RendererDrawState::Scissor);

    /* Draw with empty draw states */
    renderer.transition(RendererTargetState::Draw, {});
    CORRADE_COMPARE(renderer.currentTargetState(), RendererTargetState::Draw);
    CORRADE_COMPARE(renderer.currentDrawStates(), RendererDrawStates{});

    /* Draw with empty draw states again */
    renderer.transition(RendererTargetState::Draw, {});
    CORRADE_COMPARE(renderer.currentTargetState(), RendererTargetState::Draw);
    CORRADE_COMPARE(renderer.currentDrawStates(), RendererDrawStates{});

    /* Final state */
    renderer.transition(RendererTargetState::Final, {});
    CORRADE_COMPARE(renderer.currentTargetState(), RendererTargetState::Final);
    CORRADE_COMPARE(renderer.currentDrawStates(), RendererDrawStates{});

    /* Initial state again */
    renderer.transition(RendererTargetState::Initial, {});
    CORRADE_COMPARE(renderer.currentTargetState(), RendererTargetState::Initial);
    CORRADE_COMPARE(renderer.currentDrawStates(), RendererDrawStates{});

    /* Compositing from the initial state */
    renderer.transition(RendererTargetState::Composite, {});
    CORRADE_COMPARE(renderer.currentTargetState(), RendererTargetState::Composite);
    CORRADE_COMPARE(renderer.currentDrawStates(), RendererDrawStates{});

    /* Drawing */
    renderer.transition(RendererTargetState::Draw, RendererDrawState::Scissor);
    CORRADE_COMPARE(renderer.currentTargetState(), RendererTargetState::Draw);
    CORRADE_COMPARE(renderer.currentDrawStates(), RendererDrawState::Scissor);

    /* Compositing from the drawn state */
    renderer.transition(RendererTargetState::Composite, {});
    CORRADE_COMPARE(renderer.currentTargetState(), RendererTargetState::Composite);
    CORRADE_COMPARE(renderer.currentDrawStates(), RendererDrawStates{});

    /* Verify only the actually changing transitions got propagated to
       doTransition() */
    CORRADE_COMPARE_AS(renderer.called, (Containers::array<Containers::Pair<Containers::Pair<RendererTargetState, RendererTargetState>, Containers::Pair<RendererDrawStates, RendererDrawStates>>>({
        {{RendererTargetState::Initial, RendererTargetState::Draw},
         {{}, {}}},
        {{RendererTargetState::Draw, RendererTargetState::Draw},
         {{}, RendererDrawState::Blending|RendererDrawState::Scissor}},
        /* Second transition to the same omitted */
        {{RendererTargetState::Draw, RendererTargetState::Draw},
         {RendererDrawState::Blending|RendererDrawState::Scissor, RendererDrawState::Scissor}},
        {{RendererTargetState::Draw, RendererTargetState::Draw},
         {RendererDrawState::Scissor, {}}},
        /* Second transition to the same empty set omitted */
        {{RendererTargetState::Draw, RendererTargetState::Final},
         {{}, {}}},
        {{RendererTargetState::Final, RendererTargetState::Initial},
         {{}, {}}},
        {{RendererTargetState::Initial, RendererTargetState::Composite},
         {{}, {}}},
        {{RendererTargetState::Composite, RendererTargetState::Draw},
         {{}, RendererDrawState::Scissor}},
        {{RendererTargetState::Draw, RendererTargetState::Composite},
         {RendererDrawState::Scissor, {}}},
    })), TestSuite::Compare::Container);
}

void AbstractRendererTest::transitionInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct
        /* MSVC 2017 (_MSC_VER == 191x) crashes at runtime accessing instance2
           in the following case. Not a problem in MSVC 2015 or 2019+.
            struct: SomeBase {
            } instance1, instance2;
           Simply naming the derived struct is enough to fix the crash, FFS. */
        #if defined(CORRADE_TARGET_MSVC) && _MSC_VER >= 1910 && _MSC_VER < 1920
        ThisNameAlonePreventsMSVC2017FromBlowingUp
        #endif
        : AbstractRenderer
    {
        RendererFeatures doFeatures() const override {
            return RendererFeature::Composite;
        }
        void doSetupFramebuffers(const Vector2i&) override {}
        void doTransition(RendererTargetState, RendererTargetState, RendererDrawStates, RendererDrawStates) override {}
    } draw, final;

    /* Transition needs a framebuffer size set up */
    draw.setupFramebuffers({15, 37});
    final.setupFramebuffers({15, 37});
    draw.transition(RendererTargetState::Draw, {});
    final.transition(RendererTargetState::Final, {});

    std::ostringstream out;
    Error redirectError{&out};
    /* The check is a whitelist so we shouldn't need to verify all invalid
       combinations, just one. OTOH transition() above verifies all valid
       states. */
    draw.transition(RendererTargetState::Initial, {});
    draw.transition(RendererTargetState::Composite, RendererDrawState::Scissor|RendererDrawState::Blending);
    draw.transition(RendererTargetState::Final, RendererDrawState::Scissor);
    final.transition(RendererTargetState::Initial, RendererDrawState::Blending);
    CORRADE_COMPARE_AS(out.str(),
        "Ui::AbstractRenderer::transition(): invalid transition from Ui::RendererTargetState::Draw to Ui::RendererTargetState::Initial\n"
        "Ui::AbstractRenderer::transition(): invalid Ui::RendererDrawState::Blending|Ui::RendererDrawState::Scissor in a transition to Ui::RendererTargetState::Composite\n"
        "Ui::AbstractRenderer::transition(): invalid Ui::RendererDrawState::Scissor in a transition to Ui::RendererTargetState::Final\n"
        "Ui::AbstractRenderer::transition(): invalid Ui::RendererDrawState::Blending in a transition to Ui::RendererTargetState::Initial\n",
        TestSuite::Compare::String);
}

void AbstractRendererTest::transitionNoFramebufferSetup() {
    /* Has to be tested separately from transitionInvalid() because above it
       has to call setupFramebuffers() in order to transition() */

    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractRenderer {
        RendererFeatures doFeatures() const override { return {}; }
        void doSetupFramebuffers(const Vector2i&) override {}
        void doTransition(RendererTargetState, RendererTargetState, RendererDrawStates, RendererDrawStates) override {}
    } renderer;

    std::ostringstream out;
    Error redirectError{&out};
    renderer.transition(RendererTargetState::Initial, {});
    CORRADE_COMPARE(out.str(), "Ui::AbstractRenderer::transition(): framebuffer size wasn't set up\n");
}

void AbstractRendererTest::transitionCompositeNotSupported() {
    /* Has to be tested separately from transitionInvalid() because above it
       has to support RendererFeature::Composite in order to transition() */

    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractRenderer {
        RendererFeatures doFeatures() const override { return {}; }
        void doSetupFramebuffers(const Vector2i&) override {}
        void doTransition(RendererTargetState, RendererTargetState, RendererDrawStates, RendererDrawStates) override {}
    } renderer;

    /* Transition needs a framebuffer size set up */
    renderer.setupFramebuffers({15, 37});

    std::ostringstream out;
    Error redirectError{&out};
    renderer.transition(RendererTargetState::Composite, {});
    CORRADE_COMPARE(out.str(), "Ui::AbstractRenderer::transition(): transition to Ui::RendererTargetState::Composite not supported\n");
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::AbstractRendererTest)
