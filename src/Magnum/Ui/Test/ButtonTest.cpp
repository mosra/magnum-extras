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

#include <sstream> /** @todo remove once Debug is stream-free */
#include <Corrade/TestSuite/Compare/Numeric.h>
#include <Corrade/Utility/DebugStl.h> /** @todo remove once Debug is stream-free */

#include "Magnum/Ui/Anchor.h"
#include "Magnum/Ui/Button.h"
#include "Magnum/Ui/Event.h"
#include "Magnum/Ui/TextProperties.h"
#include "Magnum/Ui/Test/WidgetTester.hpp"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct ButtonTest: WidgetTester {
    explicit ButtonTest();

    void debugStyle();

    void constructEmpty();
    void constructIconOnly();
    void constructTextOnly();
    void constructTextOnlyTextProperties();
    void constructIconText();
    void constructIconTextTextProperties();
    void constructNoCreate();

    void setStyle();
    void setStyleWhileActive();

    void setIcon();
    void setIconFromTextOnly();
    void setIconEmpty();
    void setIconEmptyFromTextOnly();

    void setText();
    void setTextTextProperties();
    void setTextFromIconOnly();
    void setTextEmpty();
    void setTextEmptyFromIconOnly();
};

const struct {
    const char* name;
    Icon icon;
    const char* text;
} SetStyleData[]{
    {"empty", Icon::None, nullptr},
    {"icon only", Icon::No, nullptr},
    {"text only", Icon::None, "hello"},
    {"icon + text", Icon::No, "hello"},
};

ButtonTest::ButtonTest() {
    addTests({&ButtonTest::debugStyle});

    addTests<ButtonTest>({
        &ButtonTest::constructEmpty,
        &ButtonTest::constructIconOnly,
        &ButtonTest::constructTextOnly,
        &ButtonTest::constructTextOnlyTextProperties,
        &ButtonTest::constructIconText,
        &ButtonTest::constructIconTextTextProperties,
        &ButtonTest::constructNoCreate,
    }, &WidgetTester::setup,
       &WidgetTester::teardown);

    addInstancedTests<ButtonTest>({&ButtonTest::setStyle},
        Containers::arraySize(SetStyleData),
        &WidgetTester::setup,
        &WidgetTester::teardown);

    addTests<ButtonTest>({
        &ButtonTest::setStyleWhileActive,

        &ButtonTest::setIcon,
        &ButtonTest::setIconFromTextOnly,
        &ButtonTest::setIconEmpty,
        &ButtonTest::setIconEmptyFromTextOnly,

        &ButtonTest::setText,
        &ButtonTest::setTextTextProperties,
        &ButtonTest::setTextFromIconOnly,
        &ButtonTest::setTextEmpty,
        &ButtonTest::setTextEmptyFromIconOnly
    }, &WidgetTester::setup,
       &WidgetTester::teardown);
}

void ButtonTest::debugStyle() {
    std::ostringstream out;
    Debug{&out} << ButtonStyle::Success << ButtonStyle(0xef);
    CORRADE_COMPARE(out.str(), "Ui::ButtonStyle::Success Ui::ButtonStyle(0xef)\n");
}

void ButtonTest::constructEmpty() {
    {
        NodeHandle node1 = button({ui, rootNode, {32, 16}}, Icon::None, ButtonStyle::Success);
        NodeHandle node2 = button({ui, rootNode, {32, 16}}, "", ButtonStyle::Success);
        NodeHandle node3 = button({ui, rootNode, {32, 16}}, Icon::None, "", ButtonStyle::Success);
        CORRADE_COMPARE(ui.nodeParent(node1), rootNode);
        CORRADE_COMPARE(ui.nodeParent(node2), rootNode);
        CORRADE_COMPARE(ui.nodeParent(node3), rootNode);
        CORRADE_COMPARE(ui.nodeSize(node1), (Vector2{32, 16}));
        CORRADE_COMPARE(ui.nodeSize(node2), (Vector2{32, 16}));
        CORRADE_COMPARE(ui.nodeSize(node3), (Vector2{32, 16}));

        /* Can only verify that the data were created, nothing else. Visually
           tested in StyleGLTest. */
        CORRADE_COMPARE(ui.baseLayer().usedCount(), 3);
        CORRADE_COMPARE(ui.textLayer().usedCount(), 0);
    } {
        Button button1{{ui, rootNode, {32, 16}}, Icon::None, ButtonStyle::Success};
        Button button2{{ui, rootNode, {32, 16}}, "", ButtonStyle::Success};
        Button button3{{ui, rootNode, {32, 16}}, Icon::None, "", ButtonStyle::Success};
        CORRADE_COMPARE(ui.nodeParent(button1), rootNode);
        CORRADE_COMPARE(ui.nodeParent(button2), rootNode);
        CORRADE_COMPARE(ui.nodeParent(button3), rootNode);
        CORRADE_COMPARE(ui.nodeSize(button1), (Vector2{32, 16}));
        CORRADE_COMPARE(ui.nodeSize(button2), (Vector2{32, 16}));
        CORRADE_COMPARE(ui.nodeSize(button3), (Vector2{32, 16}));

        CORRADE_COMPARE(button1.style(), ButtonStyle::Success);
        CORRADE_COMPARE(button2.style(), ButtonStyle::Success);
        CORRADE_COMPARE(button3.style(), ButtonStyle::Success);
        CORRADE_COMPARE(button1.icon(), Icon::None);
        CORRADE_COMPARE(button2.icon(), Icon::None);
        CORRADE_COMPARE(button3.icon(), Icon::None);

        CORRADE_VERIFY(ui.isHandleValid(button1.backgroundData()));
        CORRADE_VERIFY(ui.isHandleValid(button2.backgroundData()));
        CORRADE_VERIFY(ui.isHandleValid(button3.backgroundData()));
        CORRADE_COMPARE(button1.iconData(), DataHandle::Null);
        CORRADE_COMPARE(button2.iconData(), DataHandle::Null);
        CORRADE_COMPARE(button3.iconData(), DataHandle::Null);
        CORRADE_COMPARE(button1.textData(), DataHandle::Null);
        CORRADE_COMPARE(button2.textData(), DataHandle::Null);
        CORRADE_COMPARE(button3.textData(), DataHandle::Null);
    }
}

void ButtonTest::constructIconOnly() {
    {
        NodeHandle node1 = button({ui, rootNode, {32, 16}}, Icon::Yes, ButtonStyle::Danger);
        NodeHandle node2 = button({ui, rootNode, {32, 16}}, Icon::Yes, "", ButtonStyle::Danger);
        CORRADE_COMPARE(ui.nodeParent(node1), rootNode);
        CORRADE_COMPARE(ui.nodeParent(node2), rootNode);
        CORRADE_COMPARE(ui.nodeSize(node1), (Vector2{32, 16}));
        CORRADE_COMPARE(ui.nodeSize(node2), (Vector2{32, 16}));

        /* Can only verify that the data were created, nothing else. Visually
           tested in StyleGLTest. */
        CORRADE_COMPARE(ui.baseLayer().usedCount(), 2);
        CORRADE_COMPARE(ui.textLayer().usedCount(), 2);
    } {
        Button button1{{ui, rootNode, {32, 16}}, Icon::Yes, ButtonStyle::Danger};
        Button button2{{ui, rootNode, {32, 16}}, Icon::Yes, "", ButtonStyle::Danger};
        CORRADE_COMPARE(ui.nodeParent(button1), rootNode);
        CORRADE_COMPARE(ui.nodeParent(button2), rootNode);
        CORRADE_COMPARE(ui.nodeSize(button1), (Vector2{32, 16}));
        CORRADE_COMPARE(ui.nodeSize(button2), (Vector2{32, 16}));

        CORRADE_COMPARE(button1.style(), ButtonStyle::Danger);
        CORRADE_COMPARE(button2.style(), ButtonStyle::Danger);
        CORRADE_COMPARE(button1.icon(), Icon::Yes);
        CORRADE_COMPARE(button2.icon(), Icon::Yes);

        CORRADE_VERIFY(ui.isHandleValid(button1.backgroundData()));
        CORRADE_VERIFY(ui.isHandleValid(button2.backgroundData()));
        CORRADE_VERIFY(ui.isHandleValid(button1.iconData()));
        CORRADE_VERIFY(ui.isHandleValid(button2.iconData()));
        CORRADE_COMPARE(ui.textLayer().glyphCount(button1.iconData()), 1);
        CORRADE_COMPARE(ui.textLayer().glyphCount(button2.iconData()), 1);
        CORRADE_COMPARE(button1.textData(), DataHandle::Null);
        CORRADE_COMPARE(button2.textData(), DataHandle::Null);
    }
}

void ButtonTest::constructTextOnly() {
    {
        NodeHandle node1 = button({ui, rootNode, {32, 16}}, "hello!", ButtonStyle::Primary);
        NodeHandle node2 = button({ui, rootNode, {32, 16}}, Icon::None, "hello!", ButtonStyle::Primary);
        CORRADE_COMPARE(ui.nodeParent(node1), rootNode);
        CORRADE_COMPARE(ui.nodeParent(node2), rootNode);
        CORRADE_COMPARE(ui.nodeSize(node1), (Vector2{32, 16}));
        CORRADE_COMPARE(ui.nodeSize(node2), (Vector2{32, 16}));

        /* Can only verify that the data were created, nothing else. Visually
           tested in StyleGLTest. */
        CORRADE_COMPARE(ui.baseLayer().usedCount(), 2);
        CORRADE_COMPARE(ui.textLayer().usedCount(), 2);
    } {
        Button button1{{ui, rootNode, {32, 16}}, "hello!", ButtonStyle::Primary};
        Button button2{{ui, rootNode, {32, 16}}, Icon::None, "hello!", ButtonStyle::Primary};
        CORRADE_COMPARE(ui.nodeParent(button1), rootNode);
        CORRADE_COMPARE(ui.nodeParent(button2), rootNode);
        CORRADE_COMPARE(ui.nodeSize(button1), (Vector2{32, 16}));
        CORRADE_COMPARE(ui.nodeSize(button2), (Vector2{32, 16}));

        CORRADE_COMPARE(button1.style(), ButtonStyle::Primary);
        CORRADE_COMPARE(button2.style(), ButtonStyle::Primary);
        CORRADE_COMPARE(button1.icon(), Icon::None);
        CORRADE_COMPARE(button2.icon(), Icon::None);

        CORRADE_VERIFY(ui.isHandleValid(button1.backgroundData()));
        CORRADE_VERIFY(ui.isHandleValid(button2.backgroundData()));
        CORRADE_COMPARE(button1.iconData(), DataHandle::Null);
        CORRADE_COMPARE(button2.iconData(), DataHandle::Null);
        CORRADE_VERIFY(ui.isHandleValid(button1.textData()));
        CORRADE_VERIFY(ui.isHandleValid(button2.textData()));
        CORRADE_COMPARE(ui.textLayer().glyphCount(button1.textData()), 6);
        CORRADE_COMPARE(ui.textLayer().glyphCount(button2.textData()), 6);
    }
}

void ButtonTest::constructTextOnlyTextProperties() {
    {
        NodeHandle node1 = button({ui, rootNode, {32, 16}}, "hello!",
            TextProperties{}.setScript(Text::Script::Braille),
            ButtonStyle::Info);
        NodeHandle node2 = button({ui, rootNode, {32, 16}}, Icon::None, "hello!",
            TextProperties{}.setScript(Text::Script::Braille),
            ButtonStyle::Info);
        CORRADE_COMPARE(ui.nodeParent(node1), rootNode);
        CORRADE_COMPARE(ui.nodeParent(node2), rootNode);
        CORRADE_COMPARE(ui.nodeSize(node1), (Vector2{32, 16}));
        CORRADE_COMPARE(ui.nodeSize(node2), (Vector2{32, 16}));

        /* Can only verify that the data were created, nothing else. Visually
           tested in StyleGLTest. */
        /** @todo this doesn't verify that the properties were passed :/ */
        CORRADE_COMPARE(ui.baseLayer().usedCount(), 2);
        CORRADE_COMPARE(ui.textLayer().usedCount(), 2);
    } {
        Button button1{{ui, rootNode, {32, 16}}, "hello!",
            TextProperties{}.setScript(Text::Script::Braille),
            ButtonStyle::Info};
        Button button2{{ui, rootNode, {32, 16}}, Icon::None, "hello!",
            TextProperties{}.setScript(Text::Script::Braille),
            ButtonStyle::Info};
        CORRADE_COMPARE(ui.nodeParent(button1), rootNode);
        CORRADE_COMPARE(ui.nodeParent(button2), rootNode);
        CORRADE_COMPARE(ui.nodeSize(button1), (Vector2{32, 16}));
        CORRADE_COMPARE(ui.nodeSize(button2), (Vector2{32, 16}));

        CORRADE_COMPARE(button1.style(), ButtonStyle::Info);
        CORRADE_COMPARE(button2.style(), ButtonStyle::Info);
        CORRADE_COMPARE(button1.icon(), Icon::None);
        CORRADE_COMPARE(button2.icon(), Icon::None);

        CORRADE_VERIFY(ui.isHandleValid(button1.backgroundData()));
        CORRADE_VERIFY(ui.isHandleValid(button2.backgroundData()));
        CORRADE_COMPARE(button1.iconData(), DataHandle::Null);
        CORRADE_COMPARE(button2.iconData(), DataHandle::Null);
        CORRADE_VERIFY(ui.isHandleValid(button1.textData()));
        CORRADE_VERIFY(ui.isHandleValid(button2.textData()));
        /* Multiplied by 6 because of the Braille script */
        CORRADE_COMPARE(ui.textLayer().glyphCount(button1.textData()), 6*6);
        CORRADE_COMPARE(ui.textLayer().glyphCount(button2.textData()), 6*6);
    }
}

void ButtonTest::constructIconText() {
    {
        NodeHandle node = button({ui, rootNode, {32, 16}}, Icon::No, "bye!", ButtonStyle::Dim);
        CORRADE_COMPARE(ui.nodeParent(node), rootNode);
        CORRADE_COMPARE(ui.nodeSize(node), (Vector2{32, 16}));

        /* Can only verify that the data were created, nothing else. Visually
           tested in StyleGLTest. */
        CORRADE_COMPARE(ui.baseLayer().usedCount(), 1);
        CORRADE_COMPARE(ui.textLayer().usedCount(), 2);
    } {
        Button button{{ui, rootNode, {32, 16}}, Icon::No, "bye!", ButtonStyle::Dim};
        CORRADE_COMPARE(ui.nodeParent(button), rootNode);
        CORRADE_COMPARE(ui.nodeSize(button), (Vector2{32, 16}));

        CORRADE_COMPARE(button.style(), ButtonStyle::Dim);
        CORRADE_COMPARE(button.icon(), Icon::No);

        CORRADE_VERIFY(ui.isHandleValid(button.backgroundData()));
        CORRADE_VERIFY(ui.isHandleValid(button.iconData()));
        CORRADE_COMPARE(ui.textLayer().glyphCount(button.iconData()), 1);
        CORRADE_VERIFY(ui.isHandleValid(button.textData()));
        CORRADE_COMPARE(ui.textLayer().glyphCount(button.textData()), 4);
    }
}

void ButtonTest::constructIconTextTextProperties() {
    {
        NodeHandle node = button({ui, rootNode, {32, 16}}, Icon::No, "bye!",
            TextProperties{}.setScript(Text::Script::Braille),
            ButtonStyle::Warning);
        CORRADE_COMPARE(ui.nodeParent(node), rootNode);
        CORRADE_COMPARE(ui.nodeSize(node), (Vector2{32, 16}));

        /* Can only verify that the data were created, nothing else. Visually
           tested in StyleGLTest. */
        /** @todo this doesn't verify that the properties were passed :/ */
        CORRADE_COMPARE(ui.baseLayer().usedCount(), 1);
        CORRADE_COMPARE(ui.textLayer().usedCount(), 2);
    } {
        Button button{{ui, rootNode, {32, 16}}, Icon::No, "bye!",
            TextProperties{}.setScript(Text::Script::Braille),
            ButtonStyle::Warning};
        CORRADE_COMPARE(ui.nodeParent(button), rootNode);
        CORRADE_COMPARE(ui.nodeSize(button), (Vector2{32, 16}));

        CORRADE_COMPARE(button.style(), ButtonStyle::Warning);
        CORRADE_COMPARE(button.icon(), Icon::No);

        CORRADE_VERIFY(ui.isHandleValid(button.backgroundData()));
        CORRADE_VERIFY(ui.isHandleValid(button.iconData()));
        /* Not multiplied as it goes directly, without the shaper */
        CORRADE_COMPARE(ui.textLayer().glyphCount(button.iconData()), 1);
        CORRADE_VERIFY(ui.isHandleValid(button.textData()));
        /* Multiplied by 6 because of the Braille script */
        CORRADE_COMPARE(ui.textLayer().glyphCount(button.textData()), 4*6);
    }
}

void ButtonTest::constructNoCreate() {
    Button button{NoCreate, ui};
    CORRADE_COMPARE(button.node(), NodeHandle::Null);
    CORRADE_COMPARE(button.backgroundData(), DataHandle::Null);
    CORRADE_COMPARE(button.iconData(), DataHandle::Null);
    CORRADE_COMPARE(button.textData(), DataHandle::Null);
}

void ButtonTest::setStyle() {
    auto&& data = SetStyleData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    Button button{{ui, rootNode, {32, 16}}, data.icon, data.text, ButtonStyle::Flat};
    CORRADE_COMPARE(button.style(), ButtonStyle::Flat);

    UnsignedInt previousStyleIcon, previousStyleText;
    if(data.icon != Icon::None)
        previousStyleIcon = ui.textLayer().style(button.iconData());
    if(data.text)
        previousStyleText = ui.textLayer().style(button.textData());

    /* The style change should result in different layer style being used */
    button.setStyle(ButtonStyle::Success);
    CORRADE_COMPARE(button.style(), ButtonStyle::Success);
    CORRADE_COMPARE(ui.baseLayer().style(button.backgroundData()),
        UnsignedInt(Implementation::BaseStyle::ButtonSuccessInactiveOut));
    /* These have different combinations based on whether just one or both are
       present, verifying just that it's different. StyleGLTest verifies the
       actual visuals and thus also catches potential mismatches. */
    if(data.icon != Icon::None)
        CORRADE_COMPARE_AS(ui.textLayer().style(button.iconData()),
            previousStyleIcon,
            TestSuite::Compare::NotEqual);
    if(data.text)
        CORRADE_COMPARE_AS(ui.textLayer().style(button.textData()),
            previousStyleText,
            TestSuite::Compare::NotEqual);
}

void ButtonTest::setStyleWhileActive() {
    Button button{{ui, rootNode, {32, 16}}, Icon::No, "yes", ButtonStyle::Primary};
    CORRADE_COMPARE(button.style(), ButtonStyle::Primary);

    CORRADE_COMPARE(ui.baseLayer().style(button.backgroundData()), UnsignedInt(Implementation::BaseStyle::ButtonPrimaryInactiveOut));
    CORRADE_COMPARE(ui.textLayer().style(button.iconData()), UnsignedInt(Implementation::TextStyle::ButtonIcon));
    CORRADE_COMPARE(ui.textLayer().style(button.textData()), UnsignedInt(Implementation::TextStyle::ButtonText));

    PointerEvent pressEvent{{}, Pointer::MouseLeft};
    CORRADE_VERIFY(ui.pointerPressEvent({16, 8}, pressEvent));
    CORRADE_COMPARE(ui.currentPressedNode(), button);

    /* Verify that style transition works */
    CORRADE_COMPARE(ui.baseLayer().style(button.backgroundData()), UnsignedInt(Implementation::BaseStyle::ButtonPrimaryPressedOut));
    CORRADE_COMPARE(ui.textLayer().style(button.iconData()), UnsignedInt(Implementation::TextStyle::ButtonPressedIcon));
    CORRADE_COMPARE(ui.textLayer().style(button.textData()), UnsignedInt(Implementation::TextStyle::ButtonPressedText));

    button.setStyle(ButtonStyle::Flat);
    CORRADE_COMPARE(button.style(), ButtonStyle::Flat);

    /* All styles should now be changed in a way that preserves the current
       pressed state */
    CORRADE_COMPARE(ui.baseLayer().style(button.backgroundData()), UnsignedInt(Implementation::BaseStyle::ButtonFlatPressedOut));
    CORRADE_COMPARE(ui.textLayer().style(button.iconData()), UnsignedInt(Implementation::TextStyle::ButtonFlatPressedOutIcon));
    CORRADE_COMPARE(ui.textLayer().style(button.textData()), UnsignedInt(Implementation::TextStyle::ButtonFlatPressedOutText));
}

void ButtonTest::setIcon() {
    Button button{{ui, rootNode, {16, 32}}, Icon::No};
    CORRADE_COMPARE(button.icon(), Icon::No);
    CORRADE_COMPARE(ui.textLayer().glyphCount(button.iconData()), 1);

    /* Clear the icon data to be able to verify that it gets updated */
    ui.textLayer().setText(button.iconData(), "", {});
    CORRADE_COMPARE(ui.textLayer().glyphCount(button.iconData()), 0);

    button.setIcon(Icon::Yes);
    CORRADE_COMPARE(button.icon(), Icon::Yes);
    CORRADE_COMPARE(ui.textLayer().glyphCount(button.iconData()), 1);
}

void ButtonTest::setIconFromTextOnly() {
    Button button{{ui, rootNode, {16, 32}}, "hello"};
    CORRADE_COMPARE(button.icon(), Icon::None);
    CORRADE_COMPARE(button.iconData(), DataHandle::Null);

    /* It should create the icon data now, the text should however stay as
       before */
    button.setIcon(Icon::Yes);
    CORRADE_COMPARE(button.icon(), Icon::Yes);
    CORRADE_VERIFY(ui.isHandleValid(button.iconData()));
    CORRADE_COMPARE(ui.textLayer().glyphCount(button.iconData()), 1);
    CORRADE_VERIFY(ui.isHandleValid(button.textData()));
}

void ButtonTest::setIconEmpty() {
    Button button{{ui, rootNode, {16, 32}}, Icon::No};
    CORRADE_COMPARE(button.icon(), Icon::No);
    CORRADE_VERIFY(ui.isHandleValid(button.iconData()));
    CORRADE_COMPARE(ui.textLayer().usedCount(), 1);

    button.setIcon(Icon::None);
    CORRADE_COMPARE(button.icon(), Icon::None);
    CORRADE_COMPARE(button.iconData(), DataHandle::Null);
    /* The original icon data should be removed */
    CORRADE_COMPARE(ui.textLayer().usedCount(), 0);
}

void ButtonTest::setIconEmptyFromTextOnly() {
    Button button{{ui, rootNode, {16, 32}}, "hello"};
    CORRADE_COMPARE(button.icon(), Icon::None);
    CORRADE_COMPARE(button.iconData(), DataHandle::Null);
    CORRADE_COMPARE(ui.textLayer().usedCount(), 1);

    /* It should just leave the icon null, the text should however stay as
       before */
    button.setIcon(Icon::None);
    CORRADE_COMPARE(button.icon(), Icon::None);
    CORRADE_COMPARE(button.iconData(), DataHandle::Null);
    CORRADE_VERIFY(ui.isHandleValid(button.textData()));
    CORRADE_COMPARE(ui.textLayer().usedCount(), 1);
}

void ButtonTest::setText() {
    Button button{{ui, rootNode, {16, 32}}, "hello"};
    CORRADE_COMPARE(ui.textLayer().glyphCount(button.textData()), 5);

    button.setText("wonderful!!");
    CORRADE_COMPARE(ui.textLayer().glyphCount(button.textData()), 11);
}

void ButtonTest::setTextTextProperties() {
    Button button{{ui, rootNode, {16, 32}}, "hello"};
    CORRADE_COMPARE(ui.textLayer().glyphCount(button.textData()), 5);

    button.setText("wonderful!!",
        TextProperties{}.setScript(Text::Script::Braille));
    /* Multiplied by 6 because of the Braille script */
    CORRADE_COMPARE(ui.textLayer().glyphCount(button.textData()), 11*6);
}

void ButtonTest::setTextFromIconOnly() {
    Button button{{ui, rootNode, {16, 32}}, Icon::No};
    CORRADE_COMPARE(button.textData(), DataHandle::Null);

    /* It should create the text data now, the icon should however stay as
       well */
    button.setText("wonderful!!");
    CORRADE_VERIFY(ui.isHandleValid(button.textData()));
    CORRADE_COMPARE(ui.textLayer().glyphCount(button.textData()), 11);
    CORRADE_COMPARE(button.icon(), Icon::No);
    CORRADE_VERIFY(ui.isHandleValid(button.iconData()));
}

void ButtonTest::setTextEmpty() {
    Button button{{ui, rootNode, {16, 32}}, "hello"};
    CORRADE_VERIFY(ui.isHandleValid(button.textData()));
    CORRADE_COMPARE(ui.textLayer().usedCount(), 1);

    button.setText("");
    CORRADE_COMPARE(button.textData(), DataHandle::Null);
    /* The original text data should be removed */
    CORRADE_COMPARE(ui.textLayer().usedCount(), 0);
}

void ButtonTest::setTextEmptyFromIconOnly() {
    Button button{{ui, rootNode, {16, 32}}, Icon::No};
    CORRADE_COMPARE(button.textData(), DataHandle::Null);
    CORRADE_COMPARE(ui.textLayer().usedCount(), 1);

    /* It should just leave the text null, the icon should however stay as
       well */
    button.setText("");
    CORRADE_COMPARE(button.textData(), DataHandle::Null);
    CORRADE_COMPARE(button.icon(), Icon::No);
    CORRADE_VERIFY(ui.isHandleValid(button.iconData()));
    CORRADE_COMPARE(ui.textLayer().usedCount(), 1);
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::ButtonTest)
