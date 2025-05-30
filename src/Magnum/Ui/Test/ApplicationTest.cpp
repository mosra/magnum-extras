/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023, 2024, 2025
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

    /* All these are testing with fake application / event classes in order to
       verify concrete behavior. Tests with actual application classes are in
       Sdl2ApplicationTest.cpp, GlfwApplicationTest.cpp etc. */

    void construct();
    void setSize();
    void pointerPressEvent();
    void pointerReleaseEvent();
    void pointerMoveEvent();
    void pointerPressReleaseMoveEventNoTouchOrPen();
    void scrollEvent();
    #ifdef MAGNUM_BUILD_DEPRECATED
    void mousePressEvent();
    void mouseReleaseEvent();
    void mouseMoveEvent();
    void mouseScrollEvent();
    #endif
    void keyPressEvent();
    void keyReleaseEvent();
    void textInputEvent();
};

struct CustomApplicationOrViewportEvent {
    explicit CustomApplicationOrViewportEvent(const Vector2i& windowSize, const Vector2i& framebufferSize, const Vector2& dpiScaling): _windowSize{windowSize}, _framebufferSize{framebufferSize}, _dpiScaling{dpiScaling} {}

    Vector2i windowSize() const { return _windowSize; }
    Vector2i framebufferSize() const { return _framebufferSize; }
    Vector2 dpiScaling() const { return _dpiScaling; }

    private:
        Vector2i _windowSize;
        Vector2i _framebufferSize;
        Vector2 _dpiScaling;
};

enum class CustomPointerEventSource {
    Mouse = 3785,
    Touch = 3868762,
    Pen = -1134,
    Trackball = 1337
};

enum class CustomPointer {
    MouseLeft = 0x0100,
    MouseRight = 0x0200,
    MouseMiddle = 0x0040,
    Finger = 0x8000,
    Pen = 0x2000,
    Eraser = 0x1000,
    TrackballFire = 0x4000
};

typedef Containers::EnumSet<CustomPointer> CustomPointers;

#ifdef CORRADE_TARGET_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#endif
CORRADE_ENUMSET_OPERATORS(CustomPointers)
#ifdef CORRADE_TARGET_CLANG
#pragma clang diagnostic pop
#endif

enum class CustomModifier {
    Shift = 1 << 12,
    Ctrl = 1 << 10,
    Alt = 1 << 20,
    Super = 1 << 31
};

typedef Containers::EnumSet<CustomModifier> CustomModifiers;

#ifdef CORRADE_TARGET_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#endif
CORRADE_ENUMSET_OPERATORS(CustomModifiers)
#ifdef CORRADE_TARGET_CLANG
#pragma clang diagnostic pop
#endif

struct CustomPointerEvent {
    explicit CustomPointerEvent(CustomPointerEventSource source, CustomPointer pointer, bool primary, Long id, CustomModifiers modifiers, const Vector2& position): _source{source}, _pointer{pointer}, _primary{primary}, _id{id}, _modifiers{modifiers}, _position{position} {}

    CustomPointerEventSource source() const { return _source; }
    CustomPointer pointer() const { return _pointer; }
    bool isPrimary() const { return _primary; }
    Long id() const { return _id; }
    CustomModifiers modifiers() const { return _modifiers; }
    Vector2 position() const { return _position; }
    void setAccepted() { accepted = true; }

    bool accepted = false;

    private:
        CustomPointerEventSource _source;
        CustomPointer _pointer;
        bool _primary;
        Long _id;
        CustomModifiers _modifiers;
        Vector2 _position;
};

const struct {
    TestSuite::TestCaseDescriptionSourceLocation name;
    CustomPointerEventSource source;
    CustomPointer pointer;
    bool primary;
    CustomModifiers modifiers;
    /* If NullOpt, the event shouldn't even be called */
    Containers::Optional<PointerEventSource> expectedSource;
    Pointer expectedPointer;
    Modifiers expectedModifiers;
    bool accept;
} PointerPressReleaseEventData[]{
    {"mouse left",
        CustomPointerEventSource::Mouse, CustomPointer::MouseLeft, true, {},
        PointerEventSource::Mouse, Pointer::MouseLeft, {}, true},
    {"mouse middle + shift & ctrl",
        CustomPointerEventSource::Mouse, CustomPointer::MouseMiddle, true, CustomModifier::Shift|CustomModifier::Ctrl,
        PointerEventSource::Mouse, Pointer::MouseMiddle, Modifier::Shift|Modifier::Ctrl, true},
    {"mouse right + super, not accepted",
        CustomPointerEventSource::Mouse, CustomPointer::MouseRight, true, CustomModifier::Super,
        PointerEventSource::Mouse, Pointer::MouseRight, Modifier::Super, false},
    {"finger",
        CustomPointerEventSource::Touch, CustomPointer::Finger, true, {},
        PointerEventSource::Touch, Pointer::Finger, {}, true},
    {"finger + ctrl & alt, secondary",
        CustomPointerEventSource::Touch, CustomPointer::Finger, false, CustomModifier::Ctrl|CustomModifier::Alt,
        PointerEventSource::Touch, Pointer::Finger, Modifier::Ctrl|Modifier::Alt, true},
    {"pen",
        CustomPointerEventSource::Pen, CustomPointer::Pen, true, {},
        PointerEventSource::Pen, Pointer::Pen, {}, true},
    {"eraser + shift",
        CustomPointerEventSource::Pen, CustomPointer::Eraser, true, CustomModifier::Shift,
        PointerEventSource::Pen, Pointer::Eraser, Modifier::Shift, true},
    {"unknown source",
        CustomPointerEventSource::Trackball, CustomPointer::MouseLeft, false, {},
        {}, Pointer{}, {}, false},
    {"unknown pointer",
        CustomPointerEventSource::Mouse, CustomPointer::TrackballFire, false, {},
        {}, Pointer{}, {}, false},
};

struct CustomPointerMoveEvent {
    explicit CustomPointerMoveEvent(CustomPointerEventSource source, Containers::Optional<CustomPointer> pointer, CustomPointers pointers, bool primary, Long id, CustomModifiers modifiers, const Vector2& position): _source{source}, _pointer{pointer}, _pointers{pointers}, _primary{primary}, _id{id}, _modifiers{modifiers}, _position{position} {}

    CustomPointerEventSource source() const { return _source; }
    Containers::Optional<CustomPointer> pointer() const { return _pointer; }
    CustomPointers pointers() const { return _pointers; }
    bool isPrimary() const { return _primary; }
    Long id() const { return _id; }
    CustomModifiers modifiers() const { return _modifiers; }
    Vector2 position() const { return _position; }
    void setAccepted() { accepted = true; }

    bool accepted = false;

    private:
        CustomPointerEventSource _source;
        Containers::Optional<CustomPointer> _pointer;
        CustomPointers _pointers;
        bool _primary;
        Long _id;
        CustomModifiers _modifiers;
        Vector2 _position;
};

const struct {
    TestSuite::TestCaseDescriptionSourceLocation name;
    CustomPointerEventSource source;
    Containers::Optional<CustomPointer> pointer;
    bool primary;
    CustomPointers pointers;
    CustomModifiers modifiers;
    /* If NullOpt, the event shouldn't even be called */
    Containers::Optional<PointerEventSource> expectedSource;
    Containers::Optional<Pointer> expectedPointer;
    Pointers expectedPointers;
    Modifiers expectedModifiers;
    bool accept;
} PointerMoveEventData[]{
    {"mouse left + middle + eraser + ctrl, not accepted",
        CustomPointerEventSource::Pen, {}, true,
        CustomPointer::MouseLeft|CustomPointer::MouseMiddle|CustomPointer::Eraser, CustomModifier::Ctrl,
        PointerEventSource::Pen, {},
        Pointer::MouseLeft|Pointer::MouseMiddle|Pointer::Eraser, Modifier::Ctrl, false},
    {"mouse middle + finger + unknown button",
        CustomPointerEventSource::Mouse, {}, true,
        CustomPointer::MouseMiddle|CustomPointer::Finger|CustomPointer::TrackballFire, {},
        PointerEventSource::Mouse, {},
        Pointer::MouseMiddle|Pointer::Finger, {}, true},
    {"pen hover + shift & alt",
        CustomPointerEventSource::Pen, {}, true,
        {}, CustomModifier::Shift|CustomModifier::Alt,
        PointerEventSource::Pen, {},
        {}, Modifier::Shift|Modifier::Alt, true},
    {"secondary touch event, nothing pressed",
        CustomPointerEventSource::Touch, {}, false,
        {}, {},
        PointerEventSource::Touch, {},
        {}, {}, true},
    {"mouse left, unknown source",
        CustomPointerEventSource::Trackball, {}, true,
        CustomPointer::MouseLeft, {},
        /* Not propagated */
        {}, {},
        {}, {}, false},
    {"unknown button alone",
        CustomPointerEventSource::Mouse, {}, true,
        CustomPointer::TrackballFire, {},
        PointerEventSource::Mouse, {},
        {}, {}, true},
    {"mouse left, right newly pressed",
        CustomPointerEventSource::Mouse, CustomPointer::MouseRight, true,
        CustomPointer::MouseLeft|CustomPointer::MouseRight, {},
        PointerEventSource::Mouse, Pointer::MouseRight,
        Pointer::MouseLeft|Pointer::MouseRight, {}, true},
    {"pen + eraser, eraser released",
        CustomPointerEventSource::Pen, CustomPointer::Eraser, true,
        CustomPointer::Pen, {},
        PointerEventSource::Pen, Pointer::Eraser,
        Pointer::Pen, {}, true},
    {"unknown button released alone",
        CustomPointerEventSource::Mouse, CustomPointer::TrackballFire, true,
        {}, {},
        /* Still propagated, but as a plain move event without any buttons */
        PointerEventSource::Mouse, {},
        {}, {}, true},
};

struct CustomScrollEvent {
    explicit CustomScrollEvent(const Vector2& position, const Vector2& offset, CustomModifiers modifiers): _position{position}, _offset{offset}, _modifiers{modifiers} {}

    Vector2 position() const { return _position; }
    Vector2 offset() const { return _offset; }
    CustomModifiers modifiers() const { return _modifiers; }
    void setAccepted() { accepted = true; }

    bool accepted = false;

    private:
        Vector2 _position;
        Vector2 _offset;
        CustomModifiers _modifiers;
};

const struct {
    const char* name;
    CustomModifiers modifiers;
    Modifiers expectedModifiers;
    bool accept;
} ScrollEventData[]{
    {"alt, not accepted",
        CustomModifier::Alt, Modifier::Alt, false},
    {"",
        {}, {}, true},
    {"shift + ctrl",
        CustomModifier::Shift|CustomModifier::Ctrl, Modifier::Shift|Modifier::Ctrl, true}
};

#ifdef MAGNUM_BUILD_DEPRECATED
struct CustomMouseEvent {
    enum class Button {
        Left = 0x13f7,
        Right = 0x167,
        Middle = 0x1dd1e,
        MiddleLeft
    };

    enum class Modifier {
        Shift = 1 << 3,
        Ctrl = 1 << 9,
        Alt = 1 << 19,
        Super = 1 << 22
    };

    typedef Containers::EnumSet<Modifier> Modifiers;

    explicit CustomMouseEvent(const Vector2i& position, Button button, Modifiers modifiers): _position{position}, _button{button}, _modifiers{modifiers} {}

    Vector2i position() const { return _position; }
    Button button() const { return _button; }
    Modifiers modifiers() const { return _modifiers; }
    void setAccepted() { accepted = true; }

    bool accepted = false;

    private:
        Vector2i _position;
        Button _button;
        Modifiers _modifiers;
};

#ifdef CORRADE_TARGET_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#endif
CORRADE_ENUMSET_OPERATORS(CustomMouseEvent::Modifiers)
#ifdef CORRADE_TARGET_CLANG
#pragma clang diagnostic pop
#endif

const struct {
    TestSuite::TestCaseDescriptionSourceLocation name;
    CustomMouseEvent::Button button;
    CustomMouseEvent::Modifiers modifiers;
    Containers::Optional<Pointer> expectedPointer;
    Modifiers expectedModifiers;
    bool accept;
} MousePressReleaseEventData[]{
    {"left",
        CustomMouseEvent::Button::Left, {},
        Pointer::MouseLeft, {}, true},
    {"middle + shift",
        CustomMouseEvent::Button::Middle, CustomMouseEvent::Modifier::Shift,
        Pointer::MouseMiddle, Modifier::Shift, true},
    {"right + super, not accepted",
        CustomMouseEvent::Button::Right, CustomMouseEvent::Modifier::Super,
        Pointer::MouseRight, Modifier::Super, false},
    {"unknown button",
        CustomMouseEvent::Button::MiddleLeft, {},
        {}, {}, false},
};

struct CustomMouseMoveEvent {
    enum class Button {
        Left = 1 << 3,
        Right = 1 << 12,
        Middle = 1 << 6,
        Bottom = 1 << 0
    };

    typedef Containers::EnumSet<Button> Buttons;

    enum class Modifier {
        Shift = 1 << 3,
        Ctrl = 1 << 9,
        Alt = 1 << 19,
        Super = 1 << 22
    };

    typedef Containers::EnumSet<Modifier> Modifiers;

    explicit CustomMouseMoveEvent(const Vector2i& position, Buttons buttons, Modifiers modifiers): _position{position}, _buttons{buttons}, _modifiers{modifiers} {}

    Vector2i position() const { return _position; }
    Buttons buttons() const { return _buttons; }
    Modifiers modifiers() const { return _modifiers; }
    void setAccepted() { accepted = true; }

    bool accepted = false;

    private:
        Vector2i _position;
        Buttons _buttons;
        Modifiers _modifiers;
};

#ifdef CORRADE_TARGET_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#endif
CORRADE_ENUMSET_OPERATORS(CustomMouseMoveEvent::Buttons)
CORRADE_ENUMSET_OPERATORS(CustomMouseMoveEvent::Modifiers)
#ifdef CORRADE_TARGET_CLANG
#pragma clang diagnostic pop
#endif

const struct {
    TestSuite::TestCaseDescriptionSourceLocation name;
    CustomMouseMoveEvent::Buttons buttons;
    CustomMouseMoveEvent::Modifiers modifiers;
    Pointers expectedPointers;
    Modifiers expectedModifiers;
    bool accept;
} MouseMoveEventData[]{
    {"left + middle, not accepted",
        CustomMouseMoveEvent::Button::Left|CustomMouseMoveEvent::Button::Middle, {},
        Pointer::MouseLeft|Pointer::MouseMiddle, {}, false},
    {"middle + right + unknown button + alt",
        CustomMouseMoveEvent::Button::Middle|CustomMouseMoveEvent::Button::Right|CustomMouseMoveEvent::Button::Bottom, CustomMouseMoveEvent::Modifier::Alt,
        Pointer::MouseMiddle|Pointer::MouseRight, Modifier::Alt, true},
    {"unknown button alone",
        CustomMouseMoveEvent::Button::Bottom, {},
        {}, {}, true},
    {"no buttons", {}, {}, {}, {}, false},
};

struct CustomMouseScrollEvent {
    enum class Modifier {
        Shift = 1 << 3,
        Ctrl = 1 << 9,
        Alt = 1 << 19,
        Super = 1 << 22
    };

    typedef Containers::EnumSet<Modifier> Modifiers;

    explicit CustomMouseScrollEvent(const Vector2i& position, const Vector2& offset, Modifiers modifiers): _position{position}, _offset{offset}, _modifiers{modifiers} {}

    Vector2i position() const { return _position; }
    Vector2 offset() const { return _offset; }
    Modifiers modifiers() const { return _modifiers; }
    void setAccepted() { accepted = true; }

    bool accepted = false;

    private:
        Vector2i _position;
        Vector2 _offset;
        Modifiers _modifiers;
};

#ifdef CORRADE_TARGET_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#endif
CORRADE_ENUMSET_OPERATORS(CustomMouseScrollEvent::Modifiers)
#ifdef CORRADE_TARGET_CLANG
#pragma clang diagnostic pop
#endif

const struct {
    const char* name;
    CustomMouseScrollEvent::Modifiers modifiers;
    Modifiers expectedModifiers;
    bool accept;
} MouseScrollEventData[]{
    {"not accepted + super",
        CustomMouseScrollEvent::Modifier::Super, Modifier::Super, false},
    {"",
        {}, {}, true},
    {"shift + alt",
        CustomMouseScrollEvent::Modifier::Shift|CustomMouseScrollEvent::Modifier::Alt, Modifier::Shift|Modifier::Alt, true}
};
#endif

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

    explicit CustomKeyEvent(Key key, CustomModifiers modifiers): _key{key}, _modifiers{modifiers} {}

    Key key() const { return _key; }
    CustomModifiers modifiers() const { return _modifiers; }
    void setAccepted() { accepted = true; }

    bool accepted = false;

    private:
        Key _key;
        CustomModifiers _modifiers;
};

const struct {
    TestSuite::TestCaseDescriptionSourceLocation name;
    CustomKeyEvent::Key key;
    CustomModifiers modifiers;
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
        CustomKeyEvent::Key::C, CustomModifier::Shift|CustomModifier::Ctrl,
        Key::C, Modifier::Shift|Modifier::Ctrl, true},
    {"Super + Alt + Esc, not accepted",
        CustomKeyEvent::Key::Esc, CustomModifier::Super|CustomModifier::Alt,
        Key::Esc, Modifier::Super|Modifier::Alt, false},
    {"left Ctrl, recognized as a key and not a modifier",
        CustomKeyEvent::Key::LeftCtrl, {},
        Key::LeftCtrl, {}, true},
    {"Super + Unknown",
        CustomKeyEvent::Key::Unknown, CustomModifier::Super,
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
    addTests({&ApplicationTest::construct,
              &ApplicationTest::setSize});

    addInstancedTests({&ApplicationTest::pointerPressEvent},
        Containers::arraySize(PointerPressReleaseEventData));

    addInstancedTests({&ApplicationTest::pointerReleaseEvent},
        Containers::arraySize(PointerPressReleaseEventData));

    addInstancedTests({&ApplicationTest::pointerMoveEvent},
        Containers::arraySize(PointerMoveEventData));

    addTests({&ApplicationTest::pointerPressReleaseMoveEventNoTouchOrPen});

    addInstancedTests({&ApplicationTest::scrollEvent},
        Containers::arraySize(ScrollEventData));

    #ifdef MAGNUM_BUILD_DEPRECATED
    addInstancedTests({&ApplicationTest::mousePressEvent},
        Containers::arraySize(MousePressReleaseEventData));

    addInstancedTests({&ApplicationTest::mouseReleaseEvent},
        Containers::arraySize(MousePressReleaseEventData));

    addInstancedTests({&ApplicationTest::mouseMoveEvent},
        Containers::arraySize(MouseMoveEventData));

    addInstancedTests({&ApplicationTest::mouseScrollEvent},
        Containers::arraySize(MouseScrollEventData));
    #endif

    addInstancedTests({&ApplicationTest::keyPressEvent},
        Containers::arraySize(KeyPressReleaseEventData));

    addInstancedTests({&ApplicationTest::keyReleaseEvent},
        Containers::arraySize(KeyPressReleaseEventData));

    addInstancedTests({&ApplicationTest::textInputEvent},
        Containers::arraySize(TextInputEventData));
}

void ApplicationTest::construct() {
    const CustomApplicationOrViewportEvent application{{100, 200}, {300, 400}, {1.25f, 1.33333333f}};

    AbstractUserInterface ui{application};
    CORRADE_COMPARE(ui.size(), (Vector2{80.0f, 150.0f}));
    CORRADE_COMPARE(ui.windowSize(), (Vector2{100.0f, 200.0f}));
    CORRADE_COMPARE(ui.framebufferSize(), (Vector2i{300, 400}));
}

void ApplicationTest::setSize() {
    const CustomApplicationOrViewportEvent applicationOrViewportEvent{{100, 200}, {300, 400}, {1.25f, 1.33333333f}};

    AbstractUserInterface ui{NoCreate};
    ui.setSize(applicationOrViewportEvent);
    CORRADE_COMPARE(ui.size(), (Vector2{80.0f, 150.0f}));
    CORRADE_COMPARE(ui.windowSize(), (Vector2{100.0f, 200.0f}));
    CORRADE_COMPARE(ui.framebufferSize(), (Vector2i{300, 400}));
}

void ApplicationTest::pointerPressEvent() {
    auto&& data = PointerPressReleaseEventData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* The events should internally still be reported relative to the UI size,
       same as when passed directly. I.e., scaled by {0.1f, 10.0f}; framebuffer
       size isn't used for anything here. */
    AbstractUserInterface ui{{200.0f, 300.0f}, {2000.0f, 30.0f}, {666, 777}};

    struct Layer: AbstractLayer {
        explicit Layer(LayerHandle handle, PointerEventSource expectedSource, Pointer expectedPointer, bool expectedPrimary, Modifiers expectedModifiers, bool accept): AbstractLayer{handle}, expectedSource{expectedSource}, expectedPointer{expectedPointer}, expectedPrimary{expectedPrimary}, expectedModifiers{expectedModifiers}, accept{accept} {}

        using AbstractLayer::create;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Event;
        }
        void doPointerPressEvent(UnsignedInt, PointerEvent& event) override {
            CORRADE_COMPARE(event.source(), expectedSource);
            CORRADE_COMPARE(event.pointer(), expectedPointer);
            CORRADE_COMPARE(event.isPrimary(), expectedPrimary);
            CORRADE_COMPARE(event.id(), 1ll << 36);
            CORRADE_COMPARE(event.modifiers(), expectedModifiers);
            CORRADE_COMPARE(event.position(), (Vector2{156.25f, 230.7f}));
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
        void doScrollEvent(UnsignedInt, ScrollEvent&) override {
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

        PointerEventSource expectedSource;
        Pointer expectedPointer;
        bool expectedPrimary;
        Modifiers expectedModifiers;
        bool accept;
        Int called = 0;
    };
    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(),
        data.expectedSource ? *data.expectedSource : PointerEventSource{},
        data.expectedPointer,
        data.primary,
        data.expectedModifiers,
        data.accept));
    layer.create(ui.createNode({}, ui.size()));

    CustomPointerEvent e{data.source, data.pointer, data.primary, 1ll << 36, data.modifiers, {1562.5f, 23.07f}};
    /* Should return true only if it's accepted */
    CORRADE_COMPARE(ui.pointerPressEvent(e), data.accept);
    /* Should be called only if there's a source / pointer to translate to */
    CORRADE_COMPARE(layer.called, data.expectedSource ? 1 : 0);
    CORRADE_COMPARE(e.accepted, data.accept);
}

void ApplicationTest::pointerReleaseEvent() {
    auto&& data = PointerPressReleaseEventData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* The events should internally still be reported relative to the UI size,
       same as when passed directly. I.e., scaled by {10.0f, 0.1f}; framebuffer
       size isn't used for anything here. */
    AbstractUserInterface ui{{200.0f, 300.0f}, {20.0f, 3000.0f}, {666, 777}};

    struct Layer: AbstractLayer {
        explicit Layer(LayerHandle handle, PointerEventSource expectedSource, Pointer expectedPointer, bool expectedPrimary, Modifiers expectedModifiers, bool accept): AbstractLayer{handle}, expectedSource{expectedSource}, expectedPointer{expectedPointer}, expectedPrimary{expectedPrimary}, expectedModifiers{expectedModifiers}, accept{accept} {}

        using AbstractLayer::create;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Event;
        }
        void doPointerPressEvent(UnsignedInt, PointerEvent&) override {
            CORRADE_FAIL("This shouldn't be called.");
        }
        void doPointerReleaseEvent(UnsignedInt, PointerEvent& event) override {
            CORRADE_COMPARE(event.source(), expectedSource);
            CORRADE_COMPARE(event.pointer(), expectedPointer);
            CORRADE_COMPARE(event.isPrimary(), expectedPrimary);
            CORRADE_COMPARE(event.id(), 1ll << 47);
            CORRADE_COMPARE(event.modifiers(), expectedModifiers);
            CORRADE_COMPARE(event.position(), (Vector2{150.75f, 236.25f}));
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
        void doScrollEvent(UnsignedInt, ScrollEvent&) override {
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

        PointerEventSource expectedSource;
        Pointer expectedPointer;
        bool expectedPrimary;
        Modifiers expectedModifiers;
        bool accept;
        Int called = 0;
    };
    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(),
        data.expectedSource ? *data.expectedSource : PointerEventSource{},
        data.expectedPointer,
        data.primary,
        data.expectedModifiers,
        data.accept));
    layer.create(ui.createNode({}, ui.size()));

    CustomPointerEvent e{data.source, data.pointer, data.primary, 1ll << 47, data.modifiers, {15.075f, 2362.5f}};
    /* Should return true only if it's accepted */
    CORRADE_COMPARE(ui.pointerReleaseEvent(e), data.accept);
    /* Should be called only if there's a source / pointer to translate to */
    CORRADE_COMPARE(layer.called, data.expectedSource ? 1 : 0);
    CORRADE_COMPARE(e.accepted, data.accept);
}

void ApplicationTest::pointerMoveEvent() {
    auto&& data = PointerMoveEventData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* The events should internally still be reported relative to the UI size,
       same as when passed directly. I.e., scaled by {0.1f, 10.0f}; framebuffer
       size isn't used for anything here. */
    AbstractUserInterface ui{{200.0f, 300.0f}, {2000.0f, 30.0f}, {666, 777}};

    struct Layer: AbstractLayer {
        explicit Layer(LayerHandle handle, PointerEventSource expectedSource, Containers::Optional<Pointer> expectedPointer, Pointers expectedPointers, bool expectedPrimary, Modifiers expectedModifiers, bool accept): AbstractLayer{handle}, expectedSource{expectedSource}, expectedPointer{expectedPointer}, expectedPointers{expectedPointers}, expectedPrimary{expectedPrimary}, expectedModifiers{expectedModifiers}, accept{accept} {}

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
            CORRADE_COMPARE(event.source(), expectedSource);
            CORRADE_COMPARE(event.pointer(), expectedPointer);
            CORRADE_COMPARE(event.pointers(), expectedPointers);
            CORRADE_COMPARE(event.isPrimary(), expectedPrimary);
            CORRADE_COMPARE(event.id(), 1ll << 55);
            CORRADE_COMPARE(event.modifiers(), expectedModifiers);
            CORRADE_COMPARE(event.position(), (Vector2{156.125f, 230.4f}));
            event.setAccepted(accept);
            ++called;
        }
        /* Enter / leave events do get called as a consequence of the move
           event internally, we don't care */
        void doScrollEvent(UnsignedInt, ScrollEvent&) override {
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

        PointerEventSource expectedSource;
        Containers::Optional<Pointer> expectedPointer;
        Pointers expectedPointers;
        bool expectedPrimary;
        Modifiers expectedModifiers;
        bool accept;
        Int called = 0;
    };
    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(),
        data.expectedSource ? *data.expectedSource : PointerEventSource{},
        data.expectedPointer,
        data.expectedPointers,
        data.primary,
        data.expectedModifiers,
        data.accept));
    layer.create(ui.createNode({}, ui.size()));

    CustomPointerMoveEvent e{data.source, data.pointer, data.pointers, data.primary, 1ll << 55, data.modifiers, {1561.25f, 23.04f}};
    /* Should return true only if it's accepted */
    CORRADE_COMPARE(ui.pointerMoveEvent(e), data.accept);
    /* Should be called only if there's a source to translate to */
    CORRADE_COMPARE(layer.called, data.expectedSource ? 1 : 0);
    CORRADE_COMPARE(e.accepted, data.accept);
}

void ApplicationTest::pointerPressReleaseMoveEventNoTouchOrPen() {
    /* To verify that it works even with enums that don't have the extra Touch
       or Pen entries */

    enum class MouseOnlyPointerEventSource {
        Mouse = 17862
    };
    enum class MouseOnlyPointer {
        MouseLeft = 0x010,
        MouseMiddle = 0x100,
        MouseRight = 0x001,
    };

    typedef Containers::EnumSet<MouseOnlyPointer> MouseOnlyPointers;

    struct MouseOnlyPointerEvent {
        explicit MouseOnlyPointerEvent(MouseOnlyPointer pointer, Long id, CustomModifiers modifiers, const Vector2& position): _pointer{pointer}, _id{id}, _modifiers{modifiers}, _position{position} {}

        MouseOnlyPointerEventSource source() const {
            return MouseOnlyPointerEventSource::Mouse;
        }
        MouseOnlyPointer pointer() const { return _pointer; }
        bool isPrimary() const { return true; }
        Long id() const { return _id; }
        CustomModifiers modifiers() const { return _modifiers; }
        Vector2 position() const { return _position; }
        void setAccepted() { accepted = true; }

        bool accepted = false;

        private:
            MouseOnlyPointer _pointer;
            Long _id;
            CustomModifiers _modifiers;
            Vector2 _position;
    };

    struct MouseOnlyPointerMoveEvent {
        explicit MouseOnlyPointerMoveEvent(Containers::Optional<MouseOnlyPointer> pointer, MouseOnlyPointers pointers, Long id, CustomModifiers modifiers, const Vector2& position): _pointer{pointer}, _pointers{pointers}, _id{id}, _modifiers{modifiers}, _position{position} {}

        MouseOnlyPointerEventSource source() const {
            return MouseOnlyPointerEventSource::Mouse;
        }
        Containers::Optional<MouseOnlyPointer> pointer() const { return _pointer; }
        MouseOnlyPointers pointers() const { return _pointers; }
        bool isPrimary() const { return true; }
        Long id() const { return _id; }
        CustomModifiers modifiers() const { return _modifiers; }
        Vector2 position() const { return _position; }
        void setAccepted() { accepted = true; }

        bool accepted = false;

        private:
            Containers::Optional<MouseOnlyPointer> _pointer;
            MouseOnlyPointers _pointers;
            Long _id;
            CustomModifiers _modifiers;
            Vector2 _position;
    };

    AbstractUserInterface ui{{100, 100}};

    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Event;
        }
        void doPointerPressEvent(UnsignedInt, PointerEvent& event) override {
            CORRADE_COMPARE(event.source(), PointerEventSource::Mouse);
            CORRADE_COMPARE(event.pointer(), Pointer::MouseLeft);
            CORRADE_COMPARE(event.isPrimary(), true);
            CORRADE_COMPARE(event.id(), 1ll << 33);
            CORRADE_COMPARE(event.modifiers(), Modifier::Alt);
            CORRADE_COMPARE(event.position(), (Vector2{1.0f, 2.0f}));
            event.setAccepted();
            called *= 2;
        }
        void doPointerReleaseEvent(UnsignedInt, PointerEvent& event) override {
            CORRADE_COMPARE(event.source(), PointerEventSource::Mouse);
            CORRADE_COMPARE(event.pointer(), Pointer::MouseRight);
            CORRADE_COMPARE(event.isPrimary(), true);
            CORRADE_COMPARE(event.id(), 1ll << 44);
            CORRADE_COMPARE(event.modifiers(), Modifier::Shift|Modifier::Ctrl);
            CORRADE_COMPARE(event.position(), (Vector2{3.0f, 4.0f}));
            event.setAccepted();
            called *= 3;
        }
        void doPointerMoveEvent(UnsignedInt, PointerMoveEvent& event) override {
            CORRADE_COMPARE(event.source(), PointerEventSource::Mouse);
            CORRADE_COMPARE(event.pointer(), Pointer::MouseMiddle);
            CORRADE_COMPARE(event.pointers(), Pointer::MouseRight|Pointer::MouseLeft);
            CORRADE_COMPARE(event.isPrimary(), true);
            CORRADE_COMPARE(event.id(), 1ll << 55);
            CORRADE_COMPARE(event.modifiers(), Modifier::Super);
            CORRADE_COMPARE(event.position(), (Vector2{5.0f, 6.0f}));
            event.setAccepted();
            called *= 5;
        }

        Int called = 1;
    };
    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer()));
    layer.create(ui.createNode({}, ui.size()));

    MouseOnlyPointerEvent press{MouseOnlyPointer::MouseLeft, 1ll << 33, CustomModifier::Alt, {1.0f, 2.0f}};
    MouseOnlyPointerEvent release{MouseOnlyPointer::MouseRight, 1ll << 44, CustomModifier::Shift|CustomModifier::Ctrl, {3.0f, 4.0f}};
    /* Using MouseOnlyPointers{} to not need CORRADE_ENUMSET_OPERATORS() */
    MouseOnlyPointerMoveEvent move{MouseOnlyPointer::MouseMiddle, MouseOnlyPointers{}|MouseOnlyPointer::MouseRight|MouseOnlyPointer::MouseLeft, 1ll << 55, CustomModifier::Super, {5.0f, 6.0f}};
    CORRADE_VERIFY(ui.pointerPressEvent(press));
    CORRADE_VERIFY(ui.pointerReleaseEvent(release));
    CORRADE_VERIFY(ui.pointerMoveEvent(move));
    CORRADE_COMPARE(layer.called, 2*3*5);
    CORRADE_VERIFY(press.accepted);
    CORRADE_VERIFY(release.accepted);
    CORRADE_VERIFY(move.accepted);
}

void ApplicationTest::scrollEvent() {
    auto&& data = ScrollEventData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* The events should internally still be reported relative to the UI size,
       same as when passed directly. I.e., scaled by {0.1f, 10.0f}; framebuffer
       size isn't used for anything here. */
    AbstractUserInterface ui{{200.0f, 300.0f}, {2000.0f, 30.0f}, {666, 777}};

    struct Layer: AbstractLayer {
        explicit Layer(LayerHandle handle, Modifiers expectedModifiers, bool accept): AbstractLayer{handle}, expectedModifiers{expectedModifiers}, accept{accept} {}

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
        void doPointerMoveEvent(UnsignedInt, PointerMoveEvent&) override {
            CORRADE_FAIL("This shouldn't be called.");
        }
        void doPointerEnterEvent(UnsignedInt, PointerMoveEvent&) override {
            CORRADE_FAIL("This shouldn't be called.");
        }
        void doPointerLeaveEvent(UnsignedInt, PointerMoveEvent&) override {
            CORRADE_FAIL("This shouldn't be called.");
        }
        void doScrollEvent(UnsignedInt, ScrollEvent& event) override {
            CORRADE_COMPARE(event.position(), (Vector2{156.25f, 230.7f}));
            CORRADE_COMPARE(event.offset(), (Vector2{2.5f, -3.7f}));
            CORRADE_COMPARE(event.modifiers(), expectedModifiers);
            event.setAccepted(accept);
            ++called;
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

        Modifiers expectedModifiers;
        bool accept;
        Int called = 0;
    };
    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), data.expectedModifiers, data.accept));
    layer.create(ui.createNode({}, ui.size()));

    CustomScrollEvent e{{1562.5f, 23.07f}, {2.5f, -3.7f}, data.modifiers};
    /* Should return true only if it's accepted */
    CORRADE_COMPARE(ui.scrollEvent(e), data.accept);
    CORRADE_COMPARE(e.accepted, data.accept);
}

#ifdef MAGNUM_BUILD_DEPRECATED
void ApplicationTest::mousePressEvent() {
    auto&& data = MousePressReleaseEventData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* The events should internally still be reported relative to the UI size,
       same as when passed directly. I.e., scaled by {0.1f, 10.0f}; framebuffer
       size isn't used for anything here. */
    AbstractUserInterface ui{{200.0f, 300.0f}, {2000.0f, 30.0f}, {666, 777}};

    struct Layer: AbstractLayer {
        explicit Layer(LayerHandle handle, Pointer expectedPointer, Modifiers expectedModifiers, bool accept): AbstractLayer{handle}, expectedPointer{expectedPointer}, expectedModifiers{expectedModifiers}, accept{accept} {}

        using AbstractLayer::create;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Event;
        }
        void doPointerPressEvent(UnsignedInt, PointerEvent& event) override {
            CORRADE_COMPARE(event.source(), PointerEventSource::Mouse);
            CORRADE_COMPARE(event.pointer(), expectedPointer);
            CORRADE_COMPARE(event.isPrimary(), true);
            CORRADE_COMPARE(event.id(), 0);
            CORRADE_COMPARE(event.modifiers(), expectedModifiers);
            CORRADE_COMPARE(event.position(), (Vector2{156.0f, 230.0f}));
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
        void doScrollEvent(UnsignedInt, ScrollEvent&) override {
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
        Modifiers expectedModifiers;
        bool accept;
        Int called = 0;
    };
    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), data.expectedPointer ? *data.expectedPointer : Pointer{}, data.expectedModifiers, data.accept));
    layer.create(ui.createNode({}, ui.size()));

    CustomMouseEvent e{{1560, 23}, data.button, data.modifiers};
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
        explicit Layer(LayerHandle handle, Pointer expectedPointer, Modifiers expectedModifiers, bool accept): AbstractLayer{handle}, expectedPointer{expectedPointer}, expectedModifiers{expectedModifiers}, accept{accept} {}

        using AbstractLayer::create;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Event;
        }
        void doPointerPressEvent(UnsignedInt, PointerEvent&) override {
            CORRADE_FAIL("This shouldn't be called.");
        }
        void doPointerReleaseEvent(UnsignedInt, PointerEvent& event) override {
            CORRADE_COMPARE(event.source(), PointerEventSource::Mouse);
            CORRADE_COMPARE(event.pointer(), expectedPointer);
            CORRADE_COMPARE(event.isPrimary(), true);
            CORRADE_COMPARE(event.id(), 0);
            CORRADE_COMPARE(event.modifiers(), expectedModifiers);
            CORRADE_COMPARE(event.position(), (Vector2{150.0f, 236.0f}));
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
        void doScrollEvent(UnsignedInt, ScrollEvent&) override {
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
        Modifiers expectedModifiers;
        bool accept;
        Int called = 0;
    };
    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), data.expectedPointer ? *data.expectedPointer : Pointer{}, data.expectedModifiers, data.accept));
    layer.create(ui.createNode({}, ui.size()));

    CustomMouseEvent e{{15, 2360}, data.button, data.modifiers};
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
        explicit Layer(LayerHandle handle, Pointers expectedPointers, Modifiers expectedModifiers, bool accept): AbstractLayer{handle}, expectedPointers{expectedPointers}, expectedModifiers{expectedModifiers}, accept{accept} {}

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
            CORRADE_COMPARE(event.source(), PointerEventSource::Mouse);
            CORRADE_COMPARE(event.pointer(), Containers::NullOpt);
            CORRADE_COMPARE(event.pointers(), expectedPointers);
            CORRADE_COMPARE(event.isPrimary(), true);
            CORRADE_COMPARE(event.id(), 0);
            CORRADE_COMPARE(event.modifiers(), expectedModifiers);
            CORRADE_COMPARE(event.position(), (Vector2{156.0f, 230.0f}));
            event.setAccepted(accept);
            ++called;
        }
        /* Enter / leave events do get called as a consequence of the move
           event internally, we don't care */
        void doScrollEvent(UnsignedInt, ScrollEvent&) override {
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
        Modifiers expectedModifiers;
        bool accept;
        Int called = 0;
    };
    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), data.expectedPointers, data.expectedModifiers, data.accept));
    layer.create(ui.createNode({}, ui.size()));

    CustomMouseMoveEvent e{{1560, 23}, data.buttons, data.modifiers};
    /* Should return true only if it's accepted */
    CORRADE_COMPARE(ui.pointerMoveEvent(e), data.accept);
    /* Should be called always */
    CORRADE_COMPARE(layer.called, 1);
    CORRADE_COMPARE(e.accepted, data.accept);
}

void ApplicationTest::mouseScrollEvent() {
    auto&& data = MouseScrollEventData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* The events should internally still be reported relative to the UI size,
       same as when passed directly. I.e., scaled by {0.1f, 10.0f}; framebuffer
       size isn't used for anything here. */
    AbstractUserInterface ui{{200.0f, 300.0f}, {2000.0f, 30.0f}, {666, 777}};

    struct Layer: AbstractLayer {
        explicit Layer(LayerHandle handle, Modifiers expectedModifiers, bool accept): AbstractLayer{handle}, expectedModifiers{expectedModifiers}, accept{accept} {}

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
        void doPointerMoveEvent(UnsignedInt, PointerMoveEvent&) override {
            CORRADE_FAIL("This shouldn't be called.");
        }
        void doPointerEnterEvent(UnsignedInt, PointerMoveEvent&) override {
            CORRADE_FAIL("This shouldn't be called.");
        }
        void doPointerLeaveEvent(UnsignedInt, PointerMoveEvent&) override {
            CORRADE_FAIL("This shouldn't be called.");
        }
        void doScrollEvent(UnsignedInt, ScrollEvent& event) override {
            CORRADE_COMPARE(event.position(), (Vector2{156.0f, 230.0f}));
            CORRADE_COMPARE(event.offset(), (Vector2{2.5f, -3.7f}));
            CORRADE_COMPARE(event.modifiers(), expectedModifiers);
            event.setAccepted(accept);
            ++called;
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

        Modifiers expectedModifiers;
        bool accept;
        Int called = 0;
    };
    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), data.expectedModifiers, data.accept));
    layer.create(ui.createNode({}, ui.size()));

    CustomMouseScrollEvent e{{1560, 23}, {2.5f, -3.7f}, data.modifiers};
    /* Should return true only if it's accepted */
    CORRADE_COMPARE(ui.scrollEvent(e), data.accept);
    CORRADE_COMPARE(e.accepted, data.accept);
}
#endif

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
        void doScrollEvent(UnsignedInt, ScrollEvent&) override {
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
    PointerMoveEvent moveEvent{{}, PointerEventSource::Mouse, {}, {}, true, 0, {}};
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
        void doScrollEvent(UnsignedInt, ScrollEvent&) override {
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
    PointerMoveEvent moveEvent{{}, PointerEventSource::Mouse, {}, {}, true, 0, {}};
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
        void doScrollEvent(UnsignedInt, ScrollEvent&) override {
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
