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

#include <Corrade/Containers/ArrayView.h>
#include <Corrade/TestSuite/Tester.h>

#include "Magnum/Ui/Application.h"
#include "Magnum/Ui/AbstractLayer.h"
#include "Magnum/Ui/AbstractUserInterface.h"
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/NodeFlags.h"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct ApplicationTest: TestSuite::Tester {
    explicit ApplicationTest();

    /* All these are testing with fake event classes in order to verify
       concrete behavior. Tests with actual application classes are in
       Sdl2ApplicationTest.cpp, GlfwApplicationTest.cpp etc. */

    void mousePressEvent();
    void mouseReleaseEvent();
    void mouseMoveEvent();
    void keyPressEvent();
    void keyReleaseEvent();
    void textInputEvent();
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

struct CustomKeyEvent {
    enum class Key {
        /* Starting the values really high to uncover any accidental 1:1
           mapping attempts */
        Unknown = 10000000,

        LeftShift,
        RightShift,
        LeftCtrl,
        RightCtrl,
        LeftAlt,
        RightAlt,
        LeftSuper,
        RightSuper,

        Enter,
        Esc,

        Up,
        Down,
        Left,
        Right,
        Home,
        End,
        PageUp,
        PageDown,
        Backspace,
        Insert,
        Delete,

        F1,
        F2,
        F3,
        F4,
        F5,
        F6,
        F7,
        F8,
        F9,
        F10,
        F11,
        F12,

        Zero,
        One,
        Two,
        Three,
        Four,
        Five,
        Six,
        Seven,
        Eight,
        Nine,

        A,
        B,
        C,
        D,
        E,
        F,
        G,
        H,
        I,
        J,
        K,
        L,
        M,
        N,
        O,
        P,
        Q,
        R,
        S,
        T,
        U,
        V,
        W,
        X,
        Y,
        Z,

        Space,
        Tab,
        Quote,
        Comma,
        Period,
        Minus,

        Plus,
        Slash,
        Percent,
        Semicolon,

        Equal,
        LeftBracket,
        RightBracket,
        Backslash,
        Backquote,

        CapsLock,
        ScrollLock,
        NumLock,
        PrintScreen,
        Pause,
        Menu,

        NumZero,
        NumOne,
        NumTwo,
        NumThree,
        NumFour,
        NumFive,
        NumSix,
        NumSeven,
        NumEight,
        NumNine,
        NumDecimal,
        NumDivide,
        NumMultiply,
        NumSubtract,
        NumAdd,
        NumEnter,
        NumEqual,

        /* These are available only in some applications */
        World1,
        World2,
        AltGr
    };

    enum class Modifier {
        Shift = 1 << 12,
        Ctrl = 1 << 10,
        Alt = 1 << 20,
        Super = 1 << 31
    };

    typedef Containers::EnumSet<Modifier> Modifiers;

    explicit CustomKeyEvent(Key key, Modifiers modifiers): _key{key}, _modifiers{modifiers} {}

    Key key() const { return _key; }
    Modifiers modifiers() const { return _modifiers; }
    void setAccepted() { accepted = true; }

    bool accepted = false;

    private:
        Key _key;
        Modifiers _modifiers;
};

#ifdef CORRADE_TARGET_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#endif
CORRADE_ENUMSET_OPERATORS(CustomKeyEvent::Modifiers)
#ifdef CORRADE_TARGET_CLANG
#pragma clang diagnostic pop
#endif

const struct {
    TestSuite::TestCaseDescriptionSourceLocation name;
    CustomKeyEvent::Key key;
    CustomKeyEvent::Modifiers modifiers;
    Key expectedKey;
    Modifiers expectedModifiers;
    bool accept;
} KeyPressReleaseEventData[]{
    {"Enter, not accepted",
        CustomKeyEvent::Key::Enter,
        {},
        Key::Enter,
        {},
        false},
    {"Shift + Ctrl + C",
        CustomKeyEvent::Key::C, CustomKeyEvent::Modifier::Shift|CustomKeyEvent::Modifier::Ctrl,
        Key::C, Modifier::Shift|Modifier::Ctrl, true},
    {"Super + Alt + Esc, not accepted",
        CustomKeyEvent::Key::Esc, CustomKeyEvent::Modifier::Super|CustomKeyEvent::Modifier::Alt,
        Key::Esc, Modifier::Super|Modifier::Alt, false},
    {"left Ctrl, recognized as a key and not a modifier",
        CustomKeyEvent::Key::LeftCtrl, {},
        Key::LeftCtrl, {}, true},
    {"Super + Unknown",
        CustomKeyEvent::Key::Unknown, CustomKeyEvent::Modifier::Super,
        Key{}, {}, false},
    {"unhandled World1 key",
        CustomKeyEvent::Key::World1, {},
        Key{}, {}, false},
    {"unhandled World2 key",
        CustomKeyEvent::Key::World2, {},
        Key{}, {}, false},
    {"unhandled AltGr key",
        CustomKeyEvent::Key::World2, {},
        Key{}, {}, false},
    {"unrecognized key",
        /* Not using a named enum value to avoid -Wswitch warnings */
        CustomKeyEvent::Key(0x7fffffff), {},
        Key{}, {}, false},
};

struct CustomTextInputEvent {
    explicit CustomTextInputEvent(Containers::StringView text): _text{text} {}

    Containers::StringView text() const { return _text; }
    void setAccepted() { accepted = true; }

    bool accepted = false;

    private:
        Containers::StringView _text;
};

const struct {
    const char* name;
    bool accept;
} TextInputEventData[]{
    {"not accepted", false},
    {"", true}
};

ApplicationTest::ApplicationTest() {
    addInstancedTests({&ApplicationTest::mousePressEvent},
        Containers::arraySize(MousePressReleaseEventData));

    addInstancedTests({&ApplicationTest::mouseReleaseEvent},
        Containers::arraySize(MousePressReleaseEventData));

    addInstancedTests({&ApplicationTest::mouseMoveEvent},
        Containers::arraySize(MouseMoveEventData));

    addInstancedTests({&ApplicationTest::keyPressEvent},
        Containers::arraySize(KeyPressReleaseEventData));

    addInstancedTests({&ApplicationTest::keyReleaseEvent},
        Containers::arraySize(KeyPressReleaseEventData));

    addInstancedTests({&ApplicationTest::textInputEvent},
        Containers::arraySize(TextInputEventData));
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
        void doFocusEvent(UnsignedInt, FocusEvent&) override {
            CORRADE_FAIL("This shouldn't be called.");
        }
        void doBlurEvent(UnsignedInt, FocusEvent&) override {
            CORRADE_FAIL("This shouldn't be called.");
        }
        void doKeyPressEvent(UnsignedInt, KeyEvent&) override {
            CORRADE_FAIL("This shouldn't be called.");
        }
        void doKeyReleaseEvent(UnsignedInt, KeyEvent&) override {
            CORRADE_FAIL("This shouldn't be called.");
        }
        void doTextInputEvent(UnsignedInt, TextInputEvent&) override {
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
        void doFocusEvent(UnsignedInt, FocusEvent&) override {
            CORRADE_FAIL("This shouldn't be called.");
        }
        void doBlurEvent(UnsignedInt, FocusEvent&) override {
            CORRADE_FAIL("This shouldn't be called.");
        }
        void doKeyPressEvent(UnsignedInt, KeyEvent&) override {
            CORRADE_FAIL("This shouldn't be called.");
        }
        void doKeyReleaseEvent(UnsignedInt, KeyEvent&) override {
            CORRADE_FAIL("This shouldn't be called.");
        }
        void doTextInputEvent(UnsignedInt, TextInputEvent&) override {
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
        void doFocusEvent(UnsignedInt, FocusEvent&) override {
            CORRADE_FAIL("This shouldn't be called.");
        }
        void doBlurEvent(UnsignedInt, FocusEvent&) override {
            CORRADE_FAIL("This shouldn't be called.");
        }
        void doKeyPressEvent(UnsignedInt, KeyEvent&) override {
            CORRADE_FAIL("This shouldn't be called.");
        }
        void doKeyReleaseEvent(UnsignedInt, KeyEvent&) override {
            CORRADE_FAIL("This shouldn't be called.");
        }
        void doTextInputEvent(UnsignedInt, TextInputEvent&) override {
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

void ApplicationTest::keyPressEvent() {
    auto&& data = KeyPressReleaseEventData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* The events should internally still be reported relative to the UI size,
       same as when passed directly. I.e., scaled by {0.1f, 10.0f}; framebuffer
       size isn't used for anything here. */
    AbstractUserInterface ui{{200.0f, 300.0f}, {2000.0f, 30.0f}, {666, 777}};

    struct Layer: AbstractLayer {
        explicit Layer(LayerHandle handle, Key expectedKey, Modifiers expectedModifiers, bool accept): AbstractLayer{handle}, expectedKey{expectedKey}, expectedModifiers{expectedModifiers}, accept{accept} {}

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
        /* Move and enter event gets called in order to remember the pointer
           position, the move has to accept */
        void doPointerMoveEvent(UnsignedInt, PointerMoveEvent& event) override {
            event.setAccepted();
        }
        void doPointerLeaveEvent(UnsignedInt, PointerMoveEvent&) override {
            CORRADE_FAIL("This shouldn't be called.");
        }
        void doPointerTapOrClickEvent(UnsignedInt, PointerEvent&) override {
            CORRADE_FAIL("This shouldn't be called.");
        }
        void doFocusEvent(UnsignedInt, FocusEvent&) override {
            CORRADE_FAIL("This shouldn't be called.");
        }
        void doBlurEvent(UnsignedInt, FocusEvent&) override {
            CORRADE_FAIL("This shouldn't be called.");
        }
        void doKeyPressEvent(UnsignedInt, KeyEvent& event) override {
            CORRADE_COMPARE(event.position(), (Vector2{156.0f, 230.0f}));
            CORRADE_COMPARE(event.key(), expectedKey);
            CORRADE_COMPARE(event.modifiers(), expectedModifiers);
            event.setAccepted(accept);
            ++called;
        }
        void doKeyReleaseEvent(UnsignedInt, KeyEvent&) override {
            CORRADE_FAIL("This shouldn't be called.");
        }
        void doTextInputEvent(UnsignedInt, TextInputEvent&) override {
            CORRADE_FAIL("This shouldn't be called.");
        }

        Key expectedKey;
        Modifiers expectedModifiers;
        bool accept;
        Int called = 0;
    };
    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), data.expectedKey, data.expectedModifiers, data.accept));
    layer.create(ui.createNode({}, ui.size()));

    /* Have to first submit an event that actually makes a node hovered, to
       have something to call the event on */
    PointerMoveEvent moveEvent{{}, {}, {}};
    CORRADE_VERIFY(ui.pointerMoveEvent({1560.0f, 23.0f}, moveEvent));
    CORRADE_VERIFY(ui.currentHoveredNode() != NodeHandle::Null);

    CustomKeyEvent e{data.key, data.modifiers};
    /* Should return true only if it's accepted */
    CORRADE_COMPARE(ui.keyPressEvent(e), data.accept);
    /* Should be called only if there's a key to translate to */
    CORRADE_COMPARE(layer.called, data.expectedKey == Key{} ? 0 : 1);
    CORRADE_COMPARE(e.accepted, data.accept);
}

void ApplicationTest::keyReleaseEvent() {
    auto&& data = KeyPressReleaseEventData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* The events should internally still be reported relative to the UI size,
       same as when passed directly. I.e., scaled by {10.0f, 0.1f}; framebuffer
       size isn't used for anything here. */
    AbstractUserInterface ui{{200.0f, 300.0f}, {20.0f, 3000.0f}, {666, 777}};

    struct Layer: AbstractLayer {
        explicit Layer(LayerHandle handle, Key expectedKey, Modifiers expectedModifiers, bool accept): AbstractLayer{handle}, expectedKey{expectedKey}, expectedModifiers{expectedModifiers}, accept{accept} {}

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
        /* Move and enter event gets called in order to remember the pointer
           position, the move has to accept */
        void doPointerMoveEvent(UnsignedInt, PointerMoveEvent& event) override {
            event.setAccepted();
        }
        void doPointerLeaveEvent(UnsignedInt, PointerMoveEvent&) override {
            CORRADE_FAIL("This shouldn't be called.");
        }
        void doPointerTapOrClickEvent(UnsignedInt, PointerEvent&) override {
            CORRADE_FAIL("This shouldn't be called.");
        }
        void doFocusEvent(UnsignedInt, FocusEvent&) override {
            CORRADE_FAIL("This shouldn't be called.");
        }
        void doBlurEvent(UnsignedInt, FocusEvent&) override {
            CORRADE_FAIL("This shouldn't be called.");
        }
        void doKeyPressEvent(UnsignedInt, KeyEvent&) override {
            CORRADE_FAIL("This shouldn't be called.");
        }
        void doKeyReleaseEvent(UnsignedInt, KeyEvent& event) override {
            CORRADE_COMPARE(event.position(), (Vector2{150.0f, 236.0f}));
            CORRADE_COMPARE(event.key(), expectedKey);
            CORRADE_COMPARE(event.modifiers(), expectedModifiers);
            event.setAccepted(accept);
            ++called;
        }
        void doTextInputEvent(UnsignedInt, TextInputEvent&) override {
            CORRADE_FAIL("This shouldn't be called.");
        }

        Key expectedKey;
        Modifiers expectedModifiers;
        bool accept;
        Int called = 0;
    };
    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), data.expectedKey, data.expectedModifiers, data.accept));
    layer.create(ui.createNode({}, ui.size()));

    /* Have to first submit an event that actually makes a node hovered, to
       have something to call the event on */
    PointerMoveEvent moveEvent{{}, {}, {}};
    CORRADE_VERIFY(ui.pointerMoveEvent({15.0f, 2360.0f}, moveEvent));
    CORRADE_VERIFY(ui.currentHoveredNode() != NodeHandle::Null);

    CustomKeyEvent e{data.key, data.modifiers};
    /* Should return true only if it's accepted */
    CORRADE_COMPARE(ui.keyReleaseEvent(e), data.accept);
    /* Should be called only if there's a key to translate to */
    CORRADE_COMPARE(layer.called, data.expectedKey == Key{} ? 0 : 1);
    CORRADE_COMPARE(e.accepted, data.accept);
}

void ApplicationTest::textInputEvent() {
    auto&& data = TextInputEventData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* The events should internally still be reported relative to the UI size,
       same as when passed directly. I.e., scaled by {0.1f, 10.0f}; framebuffer
       size isn't used for anything here. */
    AbstractUserInterface ui{{100, 100}};

    struct Layer: AbstractLayer {
        explicit Layer(LayerHandle handle, bool accept): AbstractLayer{handle}, accept{accept} {}

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
        /* Move and enter event gets called in order to remember the pointer
           position, the move has to accept */
        void doPointerMoveEvent(UnsignedInt, PointerMoveEvent& event) override {
            event.setAccepted();
        }
        void doPointerLeaveEvent(UnsignedInt, PointerMoveEvent&) override {
            CORRADE_FAIL("This shouldn't be called.");
        }
        void doPointerTapOrClickEvent(UnsignedInt, PointerEvent&) override {
            CORRADE_FAIL("This shouldn't be called.");
        }
        void doFocusEvent(UnsignedInt, FocusEvent& event) override {
            event.setAccepted();
        }
        void doBlurEvent(UnsignedInt, FocusEvent&) override {
            CORRADE_FAIL("This shouldn't be called.");
        }
        void doKeyPressEvent(UnsignedInt, KeyEvent&) override {
            CORRADE_FAIL("This shouldn't be called.");
        }
        void doKeyReleaseEvent(UnsignedInt, KeyEvent&) override {
            CORRADE_FAIL("This shouldn't be called.");
        }
        void doTextInputEvent(UnsignedInt, TextInputEvent& event) override {
            CORRADE_COMPARE(event.text(), "hello");
            event.setAccepted(accept);
            ++called;
        }

        bool accept;
        Int called = 0;
    };
    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), data.accept));
    NodeHandle node = ui.createNode({}, ui.size(), NodeFlag::Focusable);
    layer.create(node);

    /* Have to first submit an event that actually makes a node focused, to
       have something to call the event on */
    FocusEvent focusEvent{{}};
    CORRADE_VERIFY(ui.focusEvent(node, focusEvent));
    CORRADE_COMPARE(ui.currentFocusedNode(), node);

    CustomTextInputEvent e{"hello"};
    /* Should return true only if it's accepted */
    CORRADE_COMPARE(ui.textInputEvent(e), data.accept);
    CORRADE_COMPARE(layer.called, 1);
    CORRADE_COMPARE(e.accepted, data.accept);
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::ApplicationTest)
