/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023, 2024, 2025, 2026
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

    template<class T> void setText();
    template<class T> void setTextTextProperties();

    template<class T> void edit();
};

InputTest::InputTest() {
    addTests({&InputTest::debugStyle});

    addTests<InputTest>({
        &InputTest::construct,
        &InputTest::constructTextProperties
    }, &WidgetTester::setup,
       &WidgetTester::teardown);

    addTests<InputTest>({&InputTest::constructNoCreate},
        &WidgetTester::setupNoCreate,
        &WidgetTester::teardownNoCreate);

    addTests<InputTest>({
        &InputTest::setStyle,
        &InputTest::setStyleWhileActive,

        &InputTest::setText<Input>,
        &InputTest::setText<PasswordInput>,
        &InputTest::setTextTextProperties<Input>,
        &InputTest::setTextTextProperties<PasswordInput>,

        &InputTest::edit<Input>,
        &InputTest::edit<PasswordInput>
    }, &WidgetTester::setup,
       &WidgetTester::teardown);
}

using Implementation::BaseStyle;
using Implementation::TextStyle;

void InputTest::debugStyle() {
    Containers::String out;
    Debug{&out} << InputStyle::Warning << InputStyle(0xef);
    CORRADE_COMPARE(out, "Ui::InputStyle::Warning Ui::InputStyle(0xef)\n");
}

void InputTest::construct() {
    Input input1{{root, {}, {32, 16}}, "hello", InputStyle::Warning};
    PasswordInput input1Password{{root, {}, {32, 16}}, "hello", InputStyle::Warning};
    Input input2{{root, {}, {16, 32}}, InputStyle::Flat};
    PasswordInput input2Password{{root, {}, {16, 32}}, InputStyle::Flat};
    CORRADE_COMPARE(ui.nodeParent(input1), root);
    CORRADE_COMPARE(ui.nodeParent(input1Password), root);
    CORRADE_COMPARE(ui.nodeParent(input2), root);
    CORRADE_COMPARE(ui.nodeParent(input2Password), root);
    CORRADE_COMPARE(ui.nodeSize(input1), (Vector2{32, 16}));
    CORRADE_COMPARE(ui.nodeSize(input1Password), (Vector2{32, 16}));
    CORRADE_COMPARE(ui.nodeSize(input2), (Vector2{16, 32}));
    CORRADE_COMPARE(ui.nodeSize(input2Password), (Vector2{16, 32}));
    CORRADE_COMPARE(ui.nodeFlags(input1), NodeFlag::Focusable);
    CORRADE_COMPARE(ui.nodeFlags(input1Password), NodeFlag::Focusable);
    CORRADE_COMPARE(ui.nodeFlags(input2), NodeFlag::Focusable);
    CORRADE_COMPARE(ui.nodeFlags(input2Password), NodeFlag::Focusable);
    CORRADE_VERIFY(input1.isOwned());
    CORRADE_VERIFY(input1Password.isOwned());
    CORRADE_VERIFY(input2.isOwned());
    CORRADE_VERIFY(input2Password.isOwned());

    CORRADE_COMPARE(input1.style(), InputStyle::Warning);
    CORRADE_COMPARE(input1Password.style(), InputStyle::Warning);
    CORRADE_COMPARE(input2.style(), InputStyle::Flat);
    CORRADE_COMPARE(input2Password.style(), InputStyle::Flat);
    /* The password input should store the text as well */
    CORRADE_COMPARE(input1.text(), "hello");
    CORRADE_COMPARE(input1Password.text(), "hello");
    CORRADE_COMPARE(input2.text(), "");
    CORRADE_COMPARE(input2Password.text(), "");

    CORRADE_VERIFY(ui.isHandleValid(input1.backgroundData()));
    CORRADE_VERIFY(ui.isHandleValid(input1Password.backgroundData()));
    CORRADE_VERIFY(ui.isHandleValid(input2.backgroundData()));
    CORRADE_VERIFY(ui.isHandleValid(input2Password.backgroundData()));
    CORRADE_VERIFY(ui.isHandleValid(input1.textData()));
    CORRADE_VERIFY(ui.isHandleValid(input1Password.textData()));
    CORRADE_VERIFY(ui.isHandleValid(input2.textData()));
    CORRADE_VERIFY(ui.isHandleValid(input2Password.textData()));
    CORRADE_COMPARE(ui.textLayer().glyphCount(input1.textData()), 5);
    CORRADE_COMPARE(ui.textLayer().glyphCount(input1Password.textData()), 5);
    CORRADE_COMPARE(ui.textLayer().glyphCount(input2.textData()), 0);
    CORRADE_COMPARE(ui.textLayer().glyphCount(input2Password.textData()), 0);

    /* The password inputs should have different text style to choose a
       different font */
    CORRADE_COMPARE(ui.textLayer().style(input1.textData()), UnsignedInt(TextStyle::InputWarning));
    CORRADE_COMPARE(ui.textLayer().style(input1Password.textData()), UnsignedInt(TextStyle::InputWarningPassword));
    CORRADE_COMPARE(ui.textLayer().style(input2.textData()), UnsignedInt(TextStyle::InputFlat));
    CORRADE_COMPARE(ui.textLayer().style(input2Password.textData()), UnsignedInt(TextStyle::InputFlatPassword));

    /* Can only verify that the layout data were created, they're not saved */
    CORRADE_COMPARE(ui.layoutLayer().usedCount(), 4);
}

void InputTest::constructTextProperties() {
    Input input{{root, {}, {32, 16}}, "hello",
            TextProperties{}.setScript(Text::Script::Braille),
            InputStyle::Flat};
    PasswordInput inputPassword{{root, {}, {32, 16}}, "hello",
            TextProperties{}.setScript(Text::Script::Braille),
            InputStyle::Flat};
    CORRADE_COMPARE(ui.nodeParent(input), root);
    CORRADE_COMPARE(ui.nodeParent(inputPassword), root);
    CORRADE_COMPARE(ui.nodeSize(input), (Vector2{32, 16}));
    CORRADE_COMPARE(ui.nodeSize(inputPassword), (Vector2{32, 16}));
    CORRADE_COMPARE(ui.nodeFlags(input), NodeFlag::Focusable);
    CORRADE_COMPARE(ui.nodeFlags(inputPassword), NodeFlag::Focusable);
    CORRADE_VERIFY(input.isOwned());
    CORRADE_VERIFY(inputPassword.isOwned());

    CORRADE_COMPARE(input.style(), InputStyle::Flat);
    CORRADE_COMPARE(inputPassword.style(), InputStyle::Flat);
    /* The password input should store the text as well */
    CORRADE_COMPARE(input.text(), "hello");
    CORRADE_COMPARE(inputPassword.text(), "hello");

    CORRADE_VERIFY(ui.isHandleValid(input.backgroundData()));
    CORRADE_VERIFY(ui.isHandleValid(inputPassword.backgroundData()));
    CORRADE_VERIFY(ui.isHandleValid(input.textData()));
    CORRADE_VERIFY(ui.isHandleValid(inputPassword.textData()));
    /* Multiplied by 6 because of the Braille script */
    CORRADE_COMPARE(ui.textLayer().glyphCount(input.textData()), 5*6);
    CORRADE_COMPARE(ui.textLayer().glyphCount(inputPassword.textData()), 5*6);

    /* The password inputs should have different text style to choose a
       different font */
    CORRADE_COMPARE(ui.textLayer().style(input.textData()), UnsignedInt(TextStyle::InputFlat));
    CORRADE_COMPARE(ui.textLayer().style(inputPassword.textData()), UnsignedInt(TextStyle::InputFlatPassword));

    /* Can only verify that the layout data were created, they're not saved */
    CORRADE_COMPARE(ui.layoutLayer().usedCount(), 2);
}

void InputTest::constructNoCreate() {
    Input input{NoCreate};
    PasswordInput inputPassword{NoCreate};
    CORRADE_COMPARE(input.node(), NodeHandle::Null);
    CORRADE_COMPARE(inputPassword.node(), NodeHandle::Null);
    CORRADE_COMPARE(input.backgroundData(), DataHandle::Null);
    CORRADE_COMPARE(inputPassword.backgroundData(), DataHandle::Null);
    CORRADE_COMPARE(input.textData(), DataHandle::Null);
    CORRADE_COMPARE(inputPassword.textData(), DataHandle::Null);
}

void InputTest::setStyle() {
    Input input{{root, {}, {32, 16}}, "hello", InputStyle::Danger};
    PasswordInput inputPassword{{root, {}, {32, 16}}, "hello", InputStyle::Danger};
    CORRADE_COMPARE(input.style(), InputStyle::Danger);
    CORRADE_COMPARE(inputPassword.style(), InputStyle::Danger);

    /* The password inputs should have different text style to choose a
       different font */
    input.setStyle(InputStyle::Success);
    inputPassword.setStyle(InputStyle::Success);
    CORRADE_COMPARE(input.style(), InputStyle::Success);
    CORRADE_COMPARE(inputPassword.style(), InputStyle::Success);
    CORRADE_COMPARE(ui.baseLayer().style(input.backgroundData()),
        UnsignedInt(BaseStyle::InputSuccess));
    CORRADE_COMPARE(ui.baseLayer().style(inputPassword.backgroundData()),
        UnsignedInt(BaseStyle::InputSuccess));
    CORRADE_COMPARE(ui.textLayer().style(input.textData()),
        UnsignedInt(TextStyle::InputSuccess));
    CORRADE_COMPARE(ui.textLayer().style(inputPassword.textData()),
        UnsignedInt(TextStyle::InputSuccessPassword));
}

void InputTest::setStyleWhileActive() {
    Input input{{root, {}, {32, 16}}, "hello", InputStyle::Success};
    PasswordInput inputPassword{{root, {}, {32, 16}}, "hello", InputStyle::Success};
    CORRADE_COMPARE(input.style(), InputStyle::Success);
    CORRADE_COMPARE(inputPassword.style(), InputStyle::Success);

    /* The password inputs should have different text style to choose a
       different font */
    CORRADE_COMPARE(ui.baseLayer().style(input.backgroundData()), UnsignedInt(BaseStyle::InputSuccess));
    CORRADE_COMPARE(ui.baseLayer().style(inputPassword.backgroundData()), UnsignedInt(BaseStyle::InputSuccess));
    CORRADE_COMPARE(ui.textLayer().style(input.textData()), UnsignedInt(TextStyle::InputSuccess));
    CORRADE_COMPARE(ui.textLayer().style(inputPassword.textData()), UnsignedInt(TextStyle::InputSuccessPassword));

    {
        FocusEvent focusEvent{{}};
        CORRADE_VERIFY(ui.focusEvent(input, focusEvent));
        CORRADE_COMPARE(ui.currentFocusedNode(), input);

        /* Verify that style transition works */
        CORRADE_COMPARE(ui.baseLayer().style(input.backgroundData()), UnsignedInt(BaseStyle::InputSuccessFocused));
        CORRADE_COMPARE(ui.textLayer().style(input.textData()), UnsignedInt(TextStyle::InputSuccessFocused));

        input.setStyle(InputStyle::Default);
        CORRADE_COMPARE(input.style(), InputStyle::Default);

        /* All styles should now be changed in a way that preserves the current
           focused state */
        CORRADE_COMPARE(ui.baseLayer().style(input.backgroundData()), UnsignedInt(BaseStyle::InputDefaultFocused));
        CORRADE_COMPARE(ui.textLayer().style(input.textData()), UnsignedInt(TextStyle::InputDefaultFocused));

    /* Same for the password input, which should use different text styles */
    } {
        FocusEvent focusEvent{{}};
        CORRADE_VERIFY(ui.focusEvent(inputPassword, focusEvent));
        CORRADE_COMPARE(ui.currentFocusedNode(), inputPassword);

        CORRADE_COMPARE(ui.baseLayer().style(inputPassword.backgroundData()), UnsignedInt(BaseStyle::InputSuccessFocused));
        CORRADE_COMPARE(ui.textLayer().style(inputPassword.textData()), UnsignedInt(TextStyle::InputSuccessPasswordFocused));

        inputPassword.setStyle(InputStyle::Default);
        CORRADE_COMPARE(inputPassword.style(), InputStyle::Default);

        CORRADE_COMPARE(ui.baseLayer().style(inputPassword.backgroundData()), UnsignedInt(BaseStyle::InputDefaultFocused));
        CORRADE_COMPARE(ui.textLayer().style(inputPassword.textData()), UnsignedInt(TextStyle::InputDefaultPasswordFocused));
    }
}

template<class> struct InputTraits;
template<> struct InputTraits<Input> {
    static const char* name() { return "Input"; }
};
template<> struct InputTraits<PasswordInput> {
    static const char* name() { return "PasswordInput"; }
};

template<class T> void InputTest::setText() {
    setTestCaseTemplateName(InputTraits<T>::name());

    T input{{root, {}, {32, 16}}, "hiya"};
    CORRADE_COMPARE(input.text(), "hiya");
    CORRADE_COMPARE(ui.textLayer().glyphCount(input.textData()), 4);

    /* Both the rendered and stored text should update */
    input.setText("buh bye");
    CORRADE_COMPARE(input.text(), "buh bye");
    CORRADE_COMPARE(ui.textLayer().glyphCount(input.textData()), 7);
}

template<class T> void InputTest::setTextTextProperties() {
    setTestCaseTemplateName(InputTraits<T>::name());

    T input{{root, {}, {32, 16}}, "hiya"};
    CORRADE_COMPARE(input.text(), "hiya");
    CORRADE_COMPARE(ui.textLayer().glyphCount(input.textData()), 4);

    /* Both the rendered and stored text should update */
    input.setText("buh bye",
        TextProperties{}.setScript(Text::Script::Braille));
    CORRADE_COMPARE(input.text(), "buh bye");
    /* Multiplied by 6 because of the Braille script */
    CORRADE_COMPARE(ui.textLayer().glyphCount(input.textData()), 7*6);
}

template<class T> void InputTest::edit() {
    setTestCaseTemplateName(InputTraits<T>::name());

    T input{{root, {}, {32, 16}}, "set"};
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
