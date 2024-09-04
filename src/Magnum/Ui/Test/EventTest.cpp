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

#include <sstream>
#include <Corrade/Containers/Optional.h>
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/Utility/DebugStl.h>

#include "Magnum/Ui/Event.h"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct EventTest: TestSuite::Tester {
    explicit EventTest();

    void debugPointer();
    void debugPointers();
    void debugKey();
    void debugModifier();
    void debugModifiers();

    void pointer();
    void pointerMove();
    void pointerMoveRelativePosition();
    void pointerMoveNoPointer();
    void pointerMoveNoPointerRelativePosition();

    void focus();

    void key();
    void textInput();

    void visibilityLost();
};

using namespace Containers::Literals;
using namespace Math::Literals;

EventTest::EventTest() {
    addTests({&EventTest::debugPointer,
              &EventTest::debugPointers,
              &EventTest::debugKey,
              &EventTest::debugModifier,
              &EventTest::debugModifiers,

              &EventTest::pointer,
              &EventTest::pointerMove,
              &EventTest::pointerMoveRelativePosition,
              &EventTest::pointerMoveNoPointer,
              &EventTest::pointerMoveNoPointerRelativePosition,

              &EventTest::focus,

              &EventTest::key,
              &EventTest::textInput,

              &EventTest::visibilityLost});
}

void EventTest::debugPointer() {
    std::ostringstream out;
    Debug{&out} << Pointer::MouseMiddle << Pointer(0xde);
    CORRADE_COMPARE(out.str(), "Ui::Pointer::MouseMiddle Ui::Pointer(0xde)\n");
}

void EventTest::debugPointers() {
    std::ostringstream out;
    Debug{&out} << (Pointer::MouseLeft|Pointer::Finger|Pointer(0x80)) << Pointers{};
    CORRADE_COMPARE(out.str(), "Ui::Pointer::MouseLeft|Ui::Pointer::Finger|Ui::Pointer(0x80) Ui::Pointers{}\n");
}

void EventTest::debugKey() {
    std::ostringstream out;
    Debug{&out} << Key::RightSuper << Key(0xcc00);
    CORRADE_COMPARE(out.str(), "Ui::Key::RightSuper Ui::Key(0xcc00)\n");
}

void EventTest::debugModifier() {
    std::ostringstream out;
    Debug{&out} << Modifier::Super << Modifier(0xbb);
    CORRADE_COMPARE(out.str(), "Ui::Modifier::Super Ui::Modifier(0xbb)\n");
}

void EventTest::debugModifiers() {
    std::ostringstream out;
    Debug{&out} << (Modifier::Shift|Modifier::Ctrl|Modifier(0x80)) << Modifiers{};
    CORRADE_COMPARE(out.str(), "Ui::Modifier::Shift|Ui::Modifier::Ctrl|Ui::Modifier(0x80) Ui::Modifiers{}\n");
}

void EventTest::pointer() {
    PointerEvent event{1234567_nsec, Pointer::MouseMiddle};
    CORRADE_COMPARE(event.time(), 1234567_nsec);
    CORRADE_COMPARE(event.type(), Pointer::MouseMiddle);
    CORRADE_COMPARE(event.position(), Vector2{});
    CORRADE_VERIFY(!event.isCaptured());
    CORRADE_VERIFY(!event.isHovering());
    CORRADE_VERIFY(!event.isFocused());
    CORRADE_VERIFY(!event.isAccepted());

    event.setCaptured(true);
    CORRADE_VERIFY(event.isCaptured());

    event.setAccepted();
    CORRADE_VERIFY(event.isAccepted());

    event.setAccepted(false);
    CORRADE_VERIFY(!event.isAccepted());
}

void EventTest::pointerMove() {
    PointerMoveEvent event{1234567_nsec, Pointer::MouseRight, Pointer::MouseLeft|Pointer::Finger};
    CORRADE_COMPARE(event.time(), 1234567_nsec);
    CORRADE_COMPARE(event.type(), Pointer::MouseRight);
    CORRADE_COMPARE(event.types(), Pointer::MouseLeft|Pointer::Finger);
    CORRADE_COMPARE(event.position(), Vector2{});
    CORRADE_COMPARE(event.relativePosition(), Vector2{});
    CORRADE_VERIFY(!event.isCaptured());
    CORRADE_VERIFY(!event.isHovering());
    CORRADE_VERIFY(!event.isFocused());
    CORRADE_VERIFY(!event.isAccepted());

    event.setCaptured(true);
    CORRADE_VERIFY(event.isCaptured());

    event.setAccepted();
    CORRADE_VERIFY(event.isAccepted());

    event.setAccepted(false);
    CORRADE_VERIFY(!event.isAccepted());
}

void EventTest::pointerMoveRelativePosition() {
    PointerMoveEvent event{1234567_nsec, Pointer::MouseRight, Pointer::MouseLeft|Pointer::Finger, {3.0f, -6.5f}};
    CORRADE_COMPARE(event.time(), 1234567_nsec);
    CORRADE_COMPARE(event.type(), Pointer::MouseRight);
    CORRADE_COMPARE(event.types(), Pointer::MouseLeft|Pointer::Finger);
    CORRADE_COMPARE(event.position(), Vector2{});
    CORRADE_COMPARE(event.relativePosition(), (Vector2{3.0f, -6.5f}));
    CORRADE_VERIFY(!event.isCaptured());
    CORRADE_VERIFY(!event.isHovering());
    CORRADE_VERIFY(!event.isFocused());
    CORRADE_VERIFY(!event.isAccepted());
}

void EventTest::pointerMoveNoPointer() {
    PointerMoveEvent event{1234567_nsec, {}, Pointer::MouseLeft|Pointer::Finger};
    CORRADE_COMPARE(event.time(), 1234567_nsec);
    CORRADE_COMPARE(event.type(), Containers::NullOpt);
    CORRADE_COMPARE(event.position(), Vector2{});
    CORRADE_COMPARE(event.relativePosition(), Vector2{});
    CORRADE_VERIFY(!event.isCaptured());
    CORRADE_VERIFY(!event.isHovering());
    CORRADE_VERIFY(!event.isFocused());
    CORRADE_VERIFY(!event.isAccepted());
}

void EventTest::pointerMoveNoPointerRelativePosition() {
    PointerMoveEvent event{1234567_nsec, {}, Pointer::MouseLeft|Pointer::Finger, {3.0f, -6.5f}};
    CORRADE_COMPARE(event.time(), 1234567_nsec);
    CORRADE_COMPARE(event.type(), Containers::NullOpt);
    CORRADE_COMPARE(event.position(), Vector2{});
    CORRADE_COMPARE(event.relativePosition(), (Vector2{3.0f, -6.5f}));
    CORRADE_VERIFY(!event.isCaptured());
    CORRADE_VERIFY(!event.isHovering());
    CORRADE_VERIFY(!event.isFocused());
    CORRADE_VERIFY(!event.isAccepted());
}

void EventTest::focus() {
    FocusEvent event{1234567_nsec};
    CORRADE_COMPARE(event.time(), 1234567_nsec);
    CORRADE_VERIFY(!event.isPressed());
    CORRADE_VERIFY(!event.isHovering());
    CORRADE_VERIFY(!event.isAccepted());

    event.setAccepted();
    CORRADE_VERIFY(event.isAccepted());

    event.setAccepted(false);
    CORRADE_VERIFY(!event.isAccepted());
}

void EventTest::key() {
    KeyEvent event{1234567_nsec, Key::Delete, Modifier::Ctrl|Modifier::Alt};
    CORRADE_COMPARE(event.time(), 1234567_nsec);
    CORRADE_COMPARE(event.key(), Key::Delete);
    CORRADE_COMPARE(event.modifiers(), Modifier::Ctrl|Modifier::Alt);
    CORRADE_COMPARE(event.position(), Containers::NullOpt);
    CORRADE_VERIFY(!event.isCaptured());
    CORRADE_VERIFY(!event.isHovering());
    CORRADE_VERIFY(!event.isFocused());
    CORRADE_VERIFY(!event.isAccepted());

    event.setAccepted();
    CORRADE_VERIFY(event.isAccepted());

    event.setAccepted(false);
    CORRADE_VERIFY(!event.isAccepted());
}

void EventTest::textInput() {
    /* The input string view isn't copied anywhere */
    TextInputEvent event1{1234567_nsec, "hello!"_s.exceptSuffix(1)};
    TextInputEvent event2{1234567_nsec, "hello"};
    CORRADE_COMPARE(event1.time(), 1234567_nsec);
    CORRADE_COMPARE(event2.time(), 1234567_nsec);
    CORRADE_COMPARE(event1.text(), "hello");
    CORRADE_COMPARE(event2.text(), "hello");
    CORRADE_COMPARE(event1.text().flags(), Containers::StringViewFlag::Global);
    CORRADE_COMPARE(event2.text().flags(), Containers::StringViewFlag::NullTerminated);
    CORRADE_VERIFY(!event1.isAccepted());
    CORRADE_VERIFY(!event2.isAccepted());

    event1.setAccepted();
    CORRADE_VERIFY(event1.isAccepted());

    event1.setAccepted(false);
    CORRADE_VERIFY(!event1.isAccepted());
}

void EventTest::visibilityLost() {
    VisibilityLostEvent event;
    CORRADE_VERIFY(!event.isPressed());
    CORRADE_VERIFY(!event.isHovering());

    /* No accept status in this one */
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::EventTest)
