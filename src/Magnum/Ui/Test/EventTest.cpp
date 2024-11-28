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

#include <sstream>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/StringStl.h> /** @todo remove once Debug is stream-free */
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/String.h>
#include <Corrade/Utility/DebugStl.h> /** @todo remove once Debug is stream-free */

#include "Magnum/Ui/Event.h"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct EventTest: TestSuite::Tester {
    explicit EventTest();

    void debugPointerEventSource();
    void debugPointer();
    void debugPointers();
    void debugKey();
    void debugModifier();
    void debugModifiers();

    void pointer();
    void pointerInvalid();
    void pointerMove();
    void pointerMoveInvalid();
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
    addTests({&EventTest::debugPointerEventSource,
              &EventTest::debugPointer,
              &EventTest::debugPointers,
              &EventTest::debugKey,
              &EventTest::debugModifier,
              &EventTest::debugModifiers,

              &EventTest::pointer,
              &EventTest::pointerInvalid,
              &EventTest::pointerMove,
              &EventTest::pointerMoveInvalid,
              &EventTest::pointerMoveRelativePosition,
              &EventTest::pointerMoveNoPointer,
              &EventTest::pointerMoveNoPointerRelativePosition,

              &EventTest::focus,

              &EventTest::key,
              &EventTest::textInput,

              &EventTest::visibilityLost});
}

void EventTest::debugPointerEventSource() {
    std::ostringstream out;
    Debug{&out} << PointerEventSource::Touch << PointerEventSource(0xde);
    CORRADE_COMPARE(out.str(), "Ui::PointerEventSource::Touch Ui::PointerEventSource(0xde)\n");
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
    PointerEvent event{1234567_nsec, PointerEventSource::Mouse, Pointer::MouseMiddle, true, 1ll << 36};
    CORRADE_COMPARE(event.time(), 1234567_nsec);
    CORRADE_COMPARE(event.source(), PointerEventSource::Mouse);
    CORRADE_COMPARE(event.pointer(), Pointer::MouseMiddle);
    CORRADE_VERIFY(event.isPrimary());
    CORRADE_COMPARE(event.id(), 1ll << 36);
    CORRADE_COMPARE(event.position(), Vector2{});
    CORRADE_VERIFY(!event.isNodePressed());
    CORRADE_VERIFY(!event.isNodeHovered());
    CORRADE_VERIFY(!event.isNodeFocused());
    CORRADE_VERIFY(!event.isCaptured());
    CORRADE_VERIFY(!event.isAccepted());

    event.setCaptured(true);
    CORRADE_VERIFY(event.isCaptured());

    event.setAccepted();
    CORRADE_VERIFY(event.isAccepted());

    event.setAccepted(false);
    CORRADE_VERIFY(!event.isAccepted());

    /* Verify it works with all other combinations as well */
    PointerEvent event2{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
    CORRADE_COMPARE(event2.source(), PointerEventSource::Mouse);
    CORRADE_COMPARE(event2.pointer(), Pointer::MouseLeft);
    CORRADE_VERIFY(event2.isPrimary());

    PointerEvent event3{{}, PointerEventSource::Mouse, Pointer::MouseRight, true, 0};
    CORRADE_COMPARE(event3.source(), PointerEventSource::Mouse);
    CORRADE_COMPARE(event3.pointer(), Pointer::MouseRight);
    CORRADE_VERIFY(event3.isPrimary());

    PointerEvent event4{{}, PointerEventSource::Touch, Pointer::Finger, false, 0};
    CORRADE_COMPARE(event4.source(), PointerEventSource::Touch);
    CORRADE_COMPARE(event4.pointer(), Pointer::Finger);
    CORRADE_VERIFY(!event4.isPrimary());

    PointerEvent event5{{}, PointerEventSource::Pen, Pointer::Pen, true, 0};
    CORRADE_COMPARE(event5.source(), PointerEventSource::Pen);
    CORRADE_COMPARE(event5.pointer(), Pointer::Pen);
    CORRADE_VERIFY(event5.isPrimary());

    PointerEvent event6{{}, PointerEventSource::Pen, Pointer::Eraser, true, 0};
    CORRADE_COMPARE(event6.source(), PointerEventSource::Pen);
    CORRADE_COMPARE(event6.pointer(), Pointer::Eraser);
    CORRADE_VERIFY(event6.isPrimary());
}

void EventTest::pointerInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    std::ostringstream out;
    Error redirectError{&out};
    PointerEvent{{}, PointerEventSource::Mouse, Pointer::Finger, true, 0};
    PointerEvent{{}, PointerEventSource::Touch, Pointer::MouseMiddle, true, 0};
    PointerEvent{{}, PointerEventSource::Pen, Pointer::Finger, true, 0};
    PointerEvent{{}, PointerEventSource::Mouse, Pointer::MouseMiddle, false, 0};
    PointerEvent{{}, PointerEventSource::Pen, Pointer::Eraser, false, 0};
    CORRADE_COMPARE_AS(out.str(),
        "Ui::PointerEvent: invalid combination of Ui::PointerEventSource::Mouse and Ui::Pointer::Finger\n"
        "Ui::PointerEvent: invalid combination of Ui::PointerEventSource::Touch and Ui::Pointer::MouseMiddle\n"
        "Ui::PointerEvent: invalid combination of Ui::PointerEventSource::Pen and Ui::Pointer::Finger\n"
        "Ui::PointerEvent: Ui::PointerEventSource::Mouse events are expected to be primary\n"
        "Ui::PointerEvent: Ui::PointerEventSource::Pen events are expected to be primary\n",
        TestSuite::Compare::String);
}

void EventTest::pointerMove() {
    PointerMoveEvent event{1234567_nsec, PointerEventSource::Mouse, Pointer::MouseRight, Pointer::MouseLeft|Pointer::Finger, true, 1ll << 37};
    CORRADE_COMPARE(event.time(), 1234567_nsec);
    CORRADE_COMPARE(event.source(), PointerEventSource::Mouse);
    CORRADE_COMPARE(event.pointer(), Pointer::MouseRight);
    CORRADE_COMPARE(event.pointers(), Pointer::MouseLeft|Pointer::Finger);
    CORRADE_VERIFY(event.isPrimary());
    CORRADE_COMPARE(event.id(), 1ll << 37);
    CORRADE_COMPARE(event.position(), Vector2{});
    CORRADE_COMPARE(event.relativePosition(), Vector2{});
    CORRADE_VERIFY(!event.isNodePressed());
    CORRADE_VERIFY(!event.isNodeHovered());
    CORRADE_VERIFY(!event.isNodeFocused());
    CORRADE_VERIFY(!event.isCaptured());
    CORRADE_VERIFY(!event.isAccepted());

    event.setCaptured(true);
    CORRADE_VERIFY(event.isCaptured());

    event.setAccepted();
    CORRADE_VERIFY(event.isAccepted());

    event.setAccepted(false);
    CORRADE_VERIFY(!event.isAccepted());

    /* Verify it works with all other combinations as well. The set of pressed
       pointers can be arbitrary. */
    PointerMoveEvent event2{{}, PointerEventSource::Mouse, Pointer::MouseLeft, Pointer::Pen, true, 0};
    CORRADE_COMPARE(event2.source(), PointerEventSource::Mouse);
    CORRADE_COMPARE(event2.pointer(), Pointer::MouseLeft);
    CORRADE_COMPARE(event2.pointers(), Pointer::Pen);
    CORRADE_VERIFY(event2.isPrimary());

    PointerMoveEvent event3{{}, PointerEventSource::Mouse, Pointer::MouseRight, Pointer::Finger, true, 0};
    CORRADE_COMPARE(event3.source(), PointerEventSource::Mouse);
    CORRADE_COMPARE(event3.pointer(), Pointer::MouseRight);
    CORRADE_COMPARE(event3.pointers(), Pointer::Finger);
    CORRADE_VERIFY(event3.isPrimary());

    PointerMoveEvent event4{{}, PointerEventSource::Touch, Pointer::Finger, Pointer::MouseMiddle|Pointer::Pen, false, 0};
    CORRADE_COMPARE(event4.source(), PointerEventSource::Touch);
    CORRADE_COMPARE(event4.pointer(), Pointer::Finger);
    CORRADE_COMPARE(event4.pointers(), Pointer::MouseMiddle|Pointer::Pen);
    CORRADE_VERIFY(!event4.isPrimary());

    PointerMoveEvent event5{{}, PointerEventSource::Pen, Pointer::Pen, Pointer::MouseRight, true, 0};
    CORRADE_COMPARE(event5.source(), PointerEventSource::Pen);
    CORRADE_COMPARE(event5.pointer(), Pointer::Pen);
    CORRADE_COMPARE(event5.pointers(), Pointer::MouseRight);
    CORRADE_VERIFY(event5.isPrimary());

    PointerMoveEvent event6{{}, PointerEventSource::Pen, Pointer::Eraser, Pointer::Finger, true, 0};
    CORRADE_COMPARE(event6.source(), PointerEventSource::Pen);
    CORRADE_COMPARE(event6.pointer(), Pointer::Eraser);
    CORRADE_COMPARE(event6.pointers(), Pointer::Finger);
    CORRADE_VERIFY(event6.isPrimary());
}

void EventTest::pointerMoveInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    std::ostringstream out;
    Error redirectError{&out};
    PointerMoveEvent{{}, PointerEventSource::Mouse, Pointer::Finger, {}, true, 0};
    PointerMoveEvent{{}, PointerEventSource::Touch, Pointer::MouseMiddle, {}, true, 0};
    PointerMoveEvent{{}, PointerEventSource::Pen, Pointer::Finger, {}, true, 0};
    PointerMoveEvent{{}, PointerEventSource::Mouse, Pointer::MouseMiddle, {}, false, 0};
    PointerMoveEvent{{}, PointerEventSource::Pen, Pointer::Eraser, {}, false, 0};
    CORRADE_COMPARE_AS(out.str(),
        "Ui::PointerMoveEvent: invalid combination of Ui::PointerEventSource::Mouse and Ui::Pointer::Finger\n"
        "Ui::PointerMoveEvent: invalid combination of Ui::PointerEventSource::Touch and Ui::Pointer::MouseMiddle\n"
        "Ui::PointerMoveEvent: invalid combination of Ui::PointerEventSource::Pen and Ui::Pointer::Finger\n"
        "Ui::PointerMoveEvent: Ui::PointerEventSource::Mouse events are expected to be primary\n"
        "Ui::PointerMoveEvent: Ui::PointerEventSource::Pen events are expected to be primary\n",
        TestSuite::Compare::String);
}

void EventTest::pointerMoveRelativePosition() {
    PointerMoveEvent event{1234567_nsec, PointerEventSource::Pen, Pointer::Eraser, Pointer::MouseLeft|Pointer::Finger, true, 1ll << 44, {3.0f, -6.5f}};
    CORRADE_COMPARE(event.time(), 1234567_nsec);
    CORRADE_COMPARE(event.source(), PointerEventSource::Pen);
    CORRADE_COMPARE(event.pointer(), Pointer::Eraser);
    CORRADE_COMPARE(event.pointers(), Pointer::MouseLeft|Pointer::Finger);
    CORRADE_VERIFY(event.isPrimary());
    CORRADE_COMPARE(event.id(), 1ll << 44);
    CORRADE_COMPARE(event.position(), Vector2{});
    CORRADE_COMPARE(event.relativePosition(), (Vector2{3.0f, -6.5f}));
    CORRADE_VERIFY(!event.isNodePressed());
    CORRADE_VERIFY(!event.isNodeHovered());
    CORRADE_VERIFY(!event.isNodeFocused());
    CORRADE_VERIFY(!event.isCaptured());
    CORRADE_VERIFY(!event.isAccepted());
}

void EventTest::pointerMoveNoPointer() {
    PointerMoveEvent event{1234567_nsec, PointerEventSource::Touch, {}, Pointer::MouseLeft|Pointer::Finger, false, 1ll << 55};
    CORRADE_COMPARE(event.time(), 1234567_nsec);
    CORRADE_COMPARE(event.source(), PointerEventSource::Touch);
    CORRADE_COMPARE(event.pointer(), Containers::NullOpt);
    CORRADE_COMPARE(event.pointers(), Pointer::MouseLeft|Pointer::Finger);
    CORRADE_VERIFY(!event.isPrimary());
    CORRADE_COMPARE(event.id(), 1ll << 55);
    CORRADE_COMPARE(event.position(), Vector2{});
    CORRADE_COMPARE(event.relativePosition(), Vector2{});
    CORRADE_VERIFY(!event.isNodePressed());
    CORRADE_VERIFY(!event.isNodeHovered());
    CORRADE_VERIFY(!event.isNodeFocused());
    CORRADE_VERIFY(!event.isCaptured());
    CORRADE_VERIFY(!event.isAccepted());
}

void EventTest::pointerMoveNoPointerRelativePosition() {
    PointerMoveEvent event{1234567_nsec, PointerEventSource::Touch, {}, Pointer::MouseLeft|Pointer::Finger, false, 1ll << 59, {3.0f, -6.5f}};
    CORRADE_COMPARE(event.time(), 1234567_nsec);
    CORRADE_COMPARE(event.source(), PointerEventSource::Touch);
    CORRADE_COMPARE(event.pointer(), Containers::NullOpt);
    CORRADE_COMPARE(event.pointers(), Pointer::MouseLeft|Pointer::Finger);
    CORRADE_VERIFY(!event.isPrimary());
    CORRADE_COMPARE(event.id(), 1ll << 59);
    CORRADE_COMPARE(event.position(), Vector2{});
    CORRADE_COMPARE(event.relativePosition(), (Vector2{3.0f, -6.5f}));
    CORRADE_VERIFY(!event.isNodePressed());
    CORRADE_VERIFY(!event.isNodeHovered());
    CORRADE_VERIFY(!event.isNodeFocused());
    CORRADE_VERIFY(!event.isCaptured());
    CORRADE_VERIFY(!event.isAccepted());
}

void EventTest::focus() {
    FocusEvent event{1234567_nsec};
    CORRADE_COMPARE(event.time(), 1234567_nsec);
    CORRADE_VERIFY(!event.isNodePressed());
    CORRADE_VERIFY(!event.isNodeHovered());
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
    CORRADE_VERIFY(!event.isNodePressed());
    CORRADE_VERIFY(!event.isNodeHovered());
    CORRADE_VERIFY(!event.isNodeFocused());
    CORRADE_VERIFY(!event.isCaptured());
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
    CORRADE_VERIFY(!event.isNodePressed());
    CORRADE_VERIFY(!event.isNodeHovered());

    /* No accept status in this one */
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::EventTest)
