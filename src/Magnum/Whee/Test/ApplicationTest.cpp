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

#include <Corrade/Containers/ArrayView.h>
#include <Corrade/TestSuite/Tester.h>

#include "Magnum/Whee/Application.h"
#include "Magnum/Whee/AbstractLayer.h"
#include "Magnum/Whee/AbstractUserInterface.h"

namespace Magnum { namespace Whee { namespace Test { namespace {

struct ApplicationTest: TestSuite::Tester {
    explicit ApplicationTest();

    /* All these are testing with fake event classes in order to verify
       concrete behavior. Tests with actual application classes are in
       Sdl2ApplicationTest.cpp, GlfwApplicationTest.cpp etc. */

    void mousePressEvent();
    void mouseReleaseEvent();
    void mouseMoveEvent();
};

struct CustomMouseEvent {
    enum class Button {
        Left = 0x13f7,
        Right = 0x167,
        Middle = 0x1dd1e,
        MiddleLeft
    };

    explicit CustomMouseEvent(const Vector2i& position, Button button): _position{position}, _button{button} {}

    Vector2i position() const { return _position; }
    Button button() const { return _button; }
    void setAccepted() { accepted = true; }

    bool accepted = false;

    private:
        Vector2i _position;
        Button _button;
};

const struct {
    TestSuite::TestCaseDescriptionSourceLocation name;
    CustomMouseEvent::Button button;
    Containers::Optional<Pointer> expectedPointer;
    bool accept;
} MousePressReleaseEventData[]{
    {"left", CustomMouseEvent::Button::Left, Pointer::MouseLeft, true},
    {"middle", CustomMouseEvent::Button::Middle, Pointer::MouseMiddle, true},
    {"right, not accepted", CustomMouseEvent::Button::Right, Pointer::MouseRight, false},
    {"unknown button", CustomMouseEvent::Button::MiddleLeft, {}, false},
};

struct CustomMouseMoveEvent {
    enum class Button {
        Left = 1 << 3,
        Right = 1 << 12,
        Middle = 1 << 6,
        Bottom = 1 << 0
    };

    typedef Containers::EnumSet<Button> Buttons;

    explicit CustomMouseMoveEvent(const Vector2i& position, Buttons buttons): _position{position}, _buttons{buttons} {}

    Vector2i position() const { return _position; }
    Buttons buttons() const { return _buttons; }
    void setAccepted() { accepted = true; }

    bool accepted = false;

    private:
        Vector2i _position;
        Buttons _buttons;
};

#ifdef CORRADE_TARGET_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#endif
CORRADE_ENUMSET_OPERATORS(CustomMouseMoveEvent::Buttons)
#ifdef CORRADE_TARGET_CLANG
#pragma clang diagnostic pop
#endif

const struct {
    TestSuite::TestCaseDescriptionSourceLocation name;
    CustomMouseMoveEvent::Buttons buttons;
    Pointers expectedPointers;
    bool accept;
} MouseMoveEventData[]{
    {"left + middle, not accepted",
        CustomMouseMoveEvent::Button::Left|CustomMouseMoveEvent::Button::Middle,
        Pointer::MouseLeft|Pointer::MouseMiddle, false},
    {"middle + right + unknown button",
        CustomMouseMoveEvent::Button::Middle|CustomMouseMoveEvent::Button::Right|CustomMouseMoveEvent::Button::Bottom,
        Pointer::MouseMiddle|Pointer::MouseRight, true},
    {"unknown button alone",
        CustomMouseMoveEvent::Button::Bottom,
        {}, true},
    {"no buttons", {}, {}, false},
};

ApplicationTest::ApplicationTest() {
    addInstancedTests({&ApplicationTest::mousePressEvent},
        Containers::arraySize(MousePressReleaseEventData));

    addInstancedTests({&ApplicationTest::mouseReleaseEvent},
        Containers::arraySize(MousePressReleaseEventData));

    addInstancedTests({&ApplicationTest::mouseMoveEvent},
        Containers::arraySize(MouseMoveEventData));
}

void ApplicationTest::mousePressEvent() {
    auto&& data = MousePressReleaseEventData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* The events should internally still be reported relative to the UI size,
       same as when passed directly. I.e., scaled by {0.1f, 10.0f}; framebuffer
       size isn't used for anything here. */
    AbstractUserInterface ui{{200.0f, 300.0f}, {2000.0f, 30.0f}, {666, 777}};

    struct Layer: AbstractLayer {
        explicit Layer(LayerHandle handle, Pointer expectedPointer, bool accept): AbstractLayer{handle}, expectedPointer{expectedPointer}, accept{accept} {}

        using AbstractLayer::create;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Event;
        }
        void doPointerPressEvent(UnsignedInt, PointerEvent& event) override {
            CORRADE_COMPARE(event.position(), (Vector2{156.0f, 230.0f}));
            CORRADE_COMPARE(event.type(), expectedPointer);
            event.setAccepted(accept);
            ++called;
        }
        void doPointerReleaseEvent(UnsignedInt, PointerEvent&) override {
            CORRADE_FAIL("This shouldn't be called.");
        }
        void doPointerMoveEvent(UnsignedInt, PointerMoveEvent&) override {
            CORRADE_FAIL("This shouldn't be called.");
        }
        void doPointerEnterEvent(UnsignedInt, PointerMoveEvent&) override {
            CORRADE_FAIL("This shouldn't be called.");
        }
        void doPointerLeaveEvent(UnsignedInt, PointerMoveEvent&) override {
            CORRADE_FAIL("This shouldn't be called.");
        }
        void doPointerTapOrClickEvent(UnsignedInt, PointerEvent&) override {
            CORRADE_FAIL("This shouldn't be called.");
        }

        Pointer expectedPointer;
        bool accept;
        Int called = 0;
    };
    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), data.expectedPointer ? *data.expectedPointer : Pointer{}, data.accept));
    layer.create(ui.createNode({}, ui.size()));

    CustomMouseEvent e{{1560, 23}, data.button};
    /* Should return true only if it's accepted */
    CORRADE_COMPARE(ui.pointerPressEvent(e), data.accept);
    /* Should be called only if there's a pointer type to translate to */
    CORRADE_COMPARE(layer.called, data.expectedPointer ? 1 : 0);
    CORRADE_COMPARE(e.accepted, data.accept);
}

void ApplicationTest::mouseReleaseEvent() {
    auto&& data = MousePressReleaseEventData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* The events should internally still be reported relative to the UI size,
       same as when passed directly. I.e., scaled by {10.0f, 0.1f}; framebuffer
       size isn't used for anything here. */
    AbstractUserInterface ui{{200.0f, 300.0f}, {20.0f, 3000.0f}, {666, 777}};

    struct Layer: AbstractLayer {
        explicit Layer(LayerHandle handle, Pointer expectedPointer, bool accept): AbstractLayer{handle}, expectedPointer{expectedPointer}, accept{accept} {}

        using AbstractLayer::create;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Event;
        }
        void doPointerPressEvent(UnsignedInt, PointerEvent&) override {
            CORRADE_FAIL("This shouldn't be called.");
        }
        void doPointerReleaseEvent(UnsignedInt, PointerEvent& event) override {
            CORRADE_COMPARE(event.position(), (Vector2{150.0f, 236.0f}));
            CORRADE_COMPARE(event.type(), expectedPointer);
            event.setAccepted(accept);
            ++called;
        }
        void doPointerMoveEvent(UnsignedInt, PointerMoveEvent&) override {
            CORRADE_FAIL("This shouldn't be called.");
        }
        void doPointerEnterEvent(UnsignedInt, PointerMoveEvent&) override {
            CORRADE_FAIL("This shouldn't be called.");
        }
        void doPointerLeaveEvent(UnsignedInt, PointerMoveEvent&) override {
            CORRADE_FAIL("This shouldn't be called.");
        }
        void doPointerTapOrClickEvent(UnsignedInt, PointerEvent&) override {
            CORRADE_FAIL("This shouldn't be called.");
        }

        Pointer expectedPointer;
        bool accept;
        Int called = 0;
    };
    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), data.expectedPointer ? *data.expectedPointer : Pointer{}, data.accept));
    layer.create(ui.createNode({}, ui.size()));

    CustomMouseEvent e{{15, 2360}, data.button};
    /* Should return true only if it's accepted */
    CORRADE_COMPARE(ui.pointerReleaseEvent(e), data.accept);
    /* Should be called only if there's a pointer type to translate to */
    CORRADE_COMPARE(layer.called, data.expectedPointer ? 1 : 0);
    CORRADE_COMPARE(e.accepted, data.accept);
}

void ApplicationTest::mouseMoveEvent() {
    auto&& data = MouseMoveEventData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* The events should internally still be reported relative to the UI size,
       same as when passed directly. I.e., scaled by {0.1f, 10.0f}; framebuffer
       size isn't used for anything here. */
    AbstractUserInterface ui{{200.0f, 300.0f}, {2000.0f, 30.0f}, {666, 777}};

    struct Layer: AbstractLayer {
        explicit Layer(LayerHandle handle, Pointers expectedPointers, bool accept): AbstractLayer{handle}, expectedPointers{expectedPointers}, accept{accept} {}

        using AbstractLayer::create;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Event;
        }
        void doPointerPressEvent(UnsignedInt, PointerEvent&) override {
            CORRADE_FAIL("This shouldn't be called.");
        }
        void doPointerReleaseEvent(UnsignedInt, PointerEvent&) override {
            CORRADE_FAIL("This shouldn't be called.");
        }
        void doPointerMoveEvent(UnsignedInt, PointerMoveEvent& event) override {
            CORRADE_COMPARE(event.position(), (Vector2{156.0f, 230.0f}));
            CORRADE_COMPARE(event.type(), Containers::NullOpt);
            CORRADE_COMPARE(event.types(), expectedPointers);
            event.setAccepted(accept);
            ++called;
        }
        /* Enter / leave events do get called as a consequence of the move
           event internally, we don't care */
        void doPointerTapOrClickEvent(UnsignedInt, PointerEvent&) override {
            CORRADE_FAIL("This shouldn't be called.");
        }

        Pointers expectedPointers;
        bool accept;
        Int called = 0;
    };
    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), data.expectedPointers, data.accept));
    layer.create(ui.createNode({}, ui.size()));

    CustomMouseMoveEvent e{{1560, 23}, data.buttons};
    /* Should return true only if it's accepted */
    CORRADE_COMPARE(ui.pointerMoveEvent(e), data.accept);
    /* Should be called always */
    CORRADE_COMPARE(layer.called, 1);
    CORRADE_COMPARE(e.accepted, data.accept);
}

}}}}

CORRADE_TEST_MAIN(Magnum::Whee::Test::ApplicationTest)
