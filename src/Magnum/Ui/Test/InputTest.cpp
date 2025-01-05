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

#include <Corrade/Containers/String.h>
#include <Corrade/TestSuite/Compare/Numeric.h>

#include "Magnum/Ui/Anchor.h"
#include "Magnum/Ui/Event.h"
#include "Magnum/Ui/Input.h"
#include "Magnum/Ui/NodeFlags.h"
#include "Magnum/Ui/TextProperties.h"
#include "Magnum/Ui/Test/WidgetTester.hpp"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct InputTest: WidgetTester {
    explicit InputTest();

    void debugStyle();

    void construct();
    void constructTextProperties();
    void constructNoCreate();

    void setStyle();
    void setStyleWhileActive();

    void setText();
    void setTextTextProperties();

    void edit();
};

InputTest::InputTest() {
    addTests({&InputTest::debugStyle});

    addTests<InputTest>({
        &InputTest::construct,
        &InputTest::constructTextProperties,
        &InputTest::constructNoCreate,

        &InputTest::setStyle,
        &InputTest::setStyleWhileActive,

        &InputTest::setText,
        &InputTest::setTextTextProperties,

        &InputTest::edit
    }, &WidgetTester::setup,
       &WidgetTester::teardown);
}

void InputTest::debugStyle() {
    Containers::String out;
    Debug{&out} << InputStyle::Warning << InputStyle(0xef);
    CORRADE_COMPARE(out, "Ui::InputStyle::Warning Ui::InputStyle(0xef)\n");
}

void InputTest::construct() {
    Input input{{ui, rootNode, {32, 16}}, "hello", InputStyle::Warning};
    CORRADE_COMPARE(ui.nodeParent(input), rootNode);
    CORRADE_COMPARE(ui.nodeSize(input), (Vector2{32, 16}));
    CORRADE_COMPARE(ui.nodeFlags(input), NodeFlag::Focusable);

    CORRADE_COMPARE(input.style(), InputStyle::Warning);
    CORRADE_COMPARE(input.text(), "hello");

    CORRADE_VERIFY(ui.isHandleValid(input.backgroundData()));
    CORRADE_VERIFY(ui.isHandleValid(input.textData()));
    CORRADE_COMPARE(ui.textLayer().glyphCount(input.textData()), 5);
}

void InputTest::constructTextProperties() {
    Input input{{ui, rootNode, {32, 16}}, "hello",
            TextProperties{}.setScript(Text::Script::Braille),
            InputStyle::Flat};
    CORRADE_COMPARE(ui.nodeParent(input), rootNode);
    CORRADE_COMPARE(ui.nodeSize(input), (Vector2{32, 16}));
    CORRADE_COMPARE(ui.nodeFlags(input), NodeFlag::Focusable);

    CORRADE_COMPARE(input.style(), InputStyle::Flat);
    CORRADE_COMPARE(input.text(), "hello");

    CORRADE_VERIFY(ui.isHandleValid(input.backgroundData()));
    CORRADE_VERIFY(ui.isHandleValid(input.textData()));
    /* Multiplied by 6 because of the Braille script */
    CORRADE_COMPARE(ui.textLayer().glyphCount(input.textData()), 5*6);
}

void InputTest::constructNoCreate() {
    Input input{NoCreate, ui};
    CORRADE_COMPARE(input.node(), NodeHandle::Null);
    CORRADE_COMPARE(input.backgroundData(), DataHandle::Null);
    CORRADE_COMPARE(input.textData(), DataHandle::Null);
}

void InputTest::setStyle() {
    Input input{{ui, rootNode, {32, 16}}, "hello", InputStyle::Danger};
    CORRADE_COMPARE(input.style(), InputStyle::Danger);

    input.setStyle(InputStyle::Success);
    CORRADE_COMPARE(input.style(), InputStyle::Success);
    CORRADE_COMPARE(ui.baseLayer().style(input.backgroundData()),
        UnsignedInt(Implementation::BaseStyle::InputSuccessInactiveOut));
    CORRADE_COMPARE(ui.textLayer().style(input.textData()),
        UnsignedInt(Implementation::TextStyle::InputSuccessInactiveOut));
}

void InputTest::setStyleWhileActive() {
    Input input{{ui, rootNode, {32, 16}}, "hello", InputStyle::Success};
    CORRADE_COMPARE(input.style(), InputStyle::Success);

    CORRADE_COMPARE(ui.baseLayer().style(input.backgroundData()),
        UnsignedInt(Implementation::BaseStyle::InputSuccessInactiveOut));
    CORRADE_COMPARE(ui.textLayer().style(input.textData()),
        UnsignedInt(Implementation::TextStyle::InputSuccessInactiveOut));

    FocusEvent focusEvent{{}};
    CORRADE_VERIFY(ui.focusEvent(input, focusEvent));
    CORRADE_COMPARE(ui.currentFocusedNode(), input);

    /* Verify that style transition works */
    CORRADE_COMPARE(ui.baseLayer().style(input.backgroundData()), UnsignedInt(Implementation::BaseStyle::InputSuccessFocused));
    CORRADE_COMPARE(ui.textLayer().style(input.textData()), UnsignedInt(Implementation::TextStyle::InputSuccessFocused));

    input.setStyle(InputStyle::Default);
    CORRADE_COMPARE(input.style(), InputStyle::Default);

    /* All styles should now be changed in a way that preserves the current
       focused state */
    CORRADE_COMPARE(ui.baseLayer().style(input.backgroundData()), UnsignedInt(Implementation::BaseStyle::InputDefaultFocused));
    CORRADE_COMPARE(ui.textLayer().style(input.textData()), UnsignedInt(Implementation::TextStyle::InputDefaultFocused));
}

void InputTest::setText() {
    Input input{{ui, rootNode, {32, 16}}, "hiya"};
    CORRADE_COMPARE(input.text(), "hiya");
    CORRADE_COMPARE(ui.textLayer().glyphCount(input.textData()), 4);

    /* Both the rendered and stored text should update */
    input.setText("buh bye");
    CORRADE_COMPARE(input.text(), "buh bye");
    CORRADE_COMPARE(ui.textLayer().glyphCount(input.textData()), 7);
}

void InputTest::setTextTextProperties() {
    Input input{{ui, rootNode, {32, 16}}, "hiya"};
    CORRADE_COMPARE(input.text(), "hiya");
    CORRADE_COMPARE(ui.textLayer().glyphCount(input.textData()), 4);

    /* Both the rendered and stored text should update */
    input.setText("buh bye",
        TextProperties{}.setScript(Text::Script::Braille));
    CORRADE_COMPARE(input.text(), "buh bye");
    /* Multiplied by 6 because of the Braille script */
    CORRADE_COMPARE(ui.textLayer().glyphCount(input.textData()), 7*6);
}

void InputTest::edit() {
    Input input{{ui, rootNode, {32, 16}}, "set"};
    /** @todo use a cursor API once it exists */
    CORRADE_COMPARE(ui.textLayer().cursor(input.textData()), Containers::pair(3u, 3u));

    /* Focus first */
    FocusEvent focusEvent{{}};
    CORRADE_VERIFY(ui.focusEvent(input, focusEvent));
    CORRADE_COMPARE(ui.currentFocusedNode(), input);

    /* Move two chars to the left */
    KeyEvent keyEvent1{{}, Key::Left, {}};
    KeyEvent keyEvent2{{}, Key::Left, {}};
    CORRADE_VERIFY(ui.keyPressEvent(keyEvent1));
    CORRADE_VERIFY(ui.keyPressEvent(keyEvent2));
    /** @todo use a cursor API once it exists */
    CORRADE_COMPARE(ui.textLayer().cursor(input.textData()), Containers::pair(1u, 1u));

    /* Insert a text */
    TextInputEvent textInputEvent{{}, "uns"};
    CORRADE_VERIFY(ui.textInputEvent(textInputEvent));
    CORRADE_COMPARE(input.text(), "sunset");
    /** @todo use a cursor API once it exists */
    CORRADE_COMPARE(ui.textLayer().cursor(input.textData()), Containers::pair(4u, 4u));
    CORRADE_COMPARE(ui.textLayer().glyphCount(input.textData()), 6);
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::InputTest)
