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
#include "Magnum/Ui/Label.h"
#include "Magnum/Ui/TextProperties.h"
#include "Magnum/Ui/Test/WidgetTester.hpp"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct LabelTest: WidgetTester {
    explicit LabelTest();

    void debugStyle();

    void constructEmpty();
    void constructIcon();
    void constructText();
    void constructTextTextProperties();
    void constructNoCreate();

    void setStyle();

    void setIcon();
    void setIconFromText();
    void setIconFromEmpty();
    void setIconEmpty();
    void setIconEmptyFromText();

    void setText();
    void setTextTextProperties();
    void setTextFromIcon();
    void setTextFromEmpty();
    void setTextEmpty();
    void setTextEmptyFromIcon();
};

const struct {
    const char* name;
    Icon icon;
    const char* text;
} SetStyleData[]{
    {"empty", Icon::None, nullptr},
    {"icon", Icon::No, nullptr},
    {"text", Icon::None, "hello"},
};

LabelTest::LabelTest() {
    addTests({&LabelTest::debugStyle});

    addTests<LabelTest>({
        &LabelTest::constructEmpty,
        &LabelTest::constructIcon,
        &LabelTest::constructText,
        &LabelTest::constructTextTextProperties,
        &LabelTest::constructNoCreate
    }, &WidgetTester::setup,
       &WidgetTester::teardown);

    addInstancedTests<LabelTest>({&LabelTest::setStyle},
        Containers::arraySize(SetStyleData),
        &WidgetTester::setup,
        &WidgetTester::teardown);

    addTests<LabelTest>({
        &LabelTest::setIcon,
        &LabelTest::setIconFromText,
        &LabelTest::setIconFromEmpty,
        &LabelTest::setIconEmpty,
        &LabelTest::setIconEmptyFromText,

        &LabelTest::setText,
        &LabelTest::setTextTextProperties,
        &LabelTest::setTextFromIcon,
        &LabelTest::setTextFromEmpty,
        &LabelTest::setTextEmpty,
        &LabelTest::setTextEmptyFromIcon
    }, &WidgetTester::setup,
       &WidgetTester::teardown);
}

void LabelTest::debugStyle() {
    Containers::String out;
    Debug{&out} << LabelStyle::Success << LabelStyle(0xef);
    CORRADE_COMPARE(out, "Ui::LabelStyle::Success Ui::LabelStyle(0xef)\n");
}

void LabelTest::constructEmpty() {
    {
        NodeHandle node1 = label({ui, rootNode, {32, 16}}, Icon::None, LabelStyle::Success);
        NodeHandle node2 = label({ui, rootNode, {32, 16}}, "", LabelStyle::Success);
        CORRADE_COMPARE(ui.nodeParent(node1), rootNode);
        CORRADE_COMPARE(ui.nodeParent(node2), rootNode);
        CORRADE_COMPARE(ui.nodeSize(node1), (Vector2{32, 16}));
        CORRADE_COMPARE(ui.nodeSize(node2), (Vector2{32, 16}));

        /* Can only verify that the data were (not) created, nothing else.
           Visually tested in StyleGLTest. */
        CORRADE_COMPARE(ui.baseLayer().usedCount(), 0);
        CORRADE_COMPARE(ui.textLayer().usedCount(), 0);
    } {
        Label label1{{ui, rootNode, {32, 16}}, Icon::None, LabelStyle::Success};
        Label label2{{ui, rootNode, {32, 16}}, "", LabelStyle::Success};
        CORRADE_COMPARE(ui.nodeParent(label1), rootNode);
        CORRADE_COMPARE(ui.nodeParent(label2), rootNode);
        CORRADE_COMPARE(ui.nodeSize(label1), (Vector2{32, 16}));
        CORRADE_COMPARE(ui.nodeSize(label2), (Vector2{32, 16}));

        CORRADE_COMPARE(label1.style(), LabelStyle::Success);
        CORRADE_COMPARE(label2.style(), LabelStyle::Success);
        CORRADE_COMPARE(label1.icon(), Icon::None);
        CORRADE_COMPARE(label2.icon(), Icon::None);
        CORRADE_COMPARE(label1.data(), DataHandle::Null);
        CORRADE_COMPARE(label2.data(), DataHandle::Null);
    }
}

void LabelTest::constructIcon() {
    {
        NodeHandle node = label({ui, rootNode, {32, 16}}, Icon::Yes, LabelStyle::Success);
        CORRADE_COMPARE(ui.nodeParent(node), rootNode);
        CORRADE_COMPARE(ui.nodeSize(node), (Vector2{32, 16}));

        /* Can only verify that the data were created, nothing else. Visually
           tested in StyleGLTest. */
        CORRADE_COMPARE(ui.baseLayer().usedCount(), 0);
        CORRADE_COMPARE(ui.textLayer().usedCount(), 1);
    } {
        Label label{{ui, rootNode, {32, 16}}, Icon::Yes, LabelStyle::Warning};
        CORRADE_COMPARE(ui.nodeParent(label), rootNode);
        CORRADE_COMPARE(ui.nodeSize(label), (Vector2{32, 16}));

        CORRADE_COMPARE(label.style(), LabelStyle::Warning);
        CORRADE_COMPARE(label.icon(), Icon::Yes);

        CORRADE_VERIFY(ui.isHandleValid(label.data()));
        CORRADE_COMPARE(ui.textLayer().glyphCount(label.data()), 1);
    }
}

void LabelTest::constructText() {
    {
        NodeHandle node1 = label({ui, rootNode, {32, 16}}, "hello!", LabelStyle::Warning);
        CORRADE_COMPARE(ui.nodeParent(node1), rootNode);
        CORRADE_COMPARE(ui.nodeOffset(node1), Vector2{});
        CORRADE_COMPARE(ui.nodeSize(node1), (Vector2{32, 16}));

        /* Can only verify that the data were created, nothing else. Visually
           tested in StyleGLTest. */
        CORRADE_COMPARE(ui.baseLayer().usedCount(), 0);
        CORRADE_COMPARE(ui.textLayer().usedCount(), 1);
    } {
        Label label{{ui, rootNode, {32, 16}}, "hello!", LabelStyle::Danger};
        CORRADE_COMPARE(ui.nodeParent(label), rootNode);
        CORRADE_COMPARE(ui.nodeOffset(label), Vector2{});
        CORRADE_COMPARE(ui.nodeSize(label), (Vector2{32, 16}));

        CORRADE_COMPARE(label.style(), LabelStyle::Danger);
        CORRADE_COMPARE(label.icon(), Icon::None);

        CORRADE_VERIFY(ui.isHandleValid(label.data()));
        CORRADE_COMPARE(ui.textLayer().glyphCount(label.data()), 6);
    }
}

void LabelTest::constructTextTextProperties() {
    {
        NodeHandle node = label({ui, rootNode, {32, 16}}, "hello!",
            TextProperties{}.setScript(Text::Script::Braille),
            LabelStyle::Primary);
        CORRADE_COMPARE(ui.nodeParent(node), rootNode);
        CORRADE_COMPARE(ui.nodeOffset(node), Vector2{});
        CORRADE_COMPARE(ui.nodeSize(node), (Vector2{32, 16}));

        /* Can only verify that the data were created, nothing else. Visually
           tested in StyleGLTest. */
        /** @todo this doesn't verify that the properties were passed :/ */
        CORRADE_COMPARE(ui.baseLayer().usedCount(), 0);
        CORRADE_COMPARE(ui.textLayer().usedCount(), 1);
    } {
        Label label{{ui, rootNode, {32, 16}}, "hello!",
            TextProperties{}.setScript(Text::Script::Braille),
            LabelStyle::Info};
        CORRADE_COMPARE(ui.nodeParent(label), rootNode);
        CORRADE_COMPARE(ui.nodeOffset(label), Vector2{});
        CORRADE_COMPARE(ui.nodeSize(label), (Vector2{32, 16}));

        CORRADE_COMPARE(label.style(), LabelStyle::Info);
        CORRADE_COMPARE(label.icon(), Icon::None);

        CORRADE_VERIFY(ui.isHandleValid(label.data()));
        /* Multiplied by 6 because of the Braille script */
        CORRADE_COMPARE(ui.textLayer().glyphCount(label.data()), 6*6);
    }
}

void LabelTest::constructNoCreate() {
    Label label{NoCreate, ui};
    CORRADE_COMPARE(label.node(), NodeHandle::Null);
    CORRADE_COMPARE(label.data(), DataHandle::Null);
}

void LabelTest::setStyle() {
    auto&& data = SetStyleData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    Label label = data.text ?
        Label{{ui, rootNode, {32, 16}}, data.text, LabelStyle::Dim} :
        Label{{ui, rootNode, {32, 16}}, data.icon, LabelStyle::Dim};
    CORRADE_COMPARE(label.style(), LabelStyle::Dim);

    UnsignedInt previousStyle;
    if(data.text || data.icon != Icon::None)
        previousStyle = ui.textLayer().style(label.data());
    else CORRADE_COMPARE(label.data(), DataHandle::Null);

    /* The style change should result in different layer style being used */
    label.setStyle(LabelStyle::Success);
    CORRADE_COMPARE(label.style(), LabelStyle::Success);
    if(data.text || data.icon != Icon::None)
        CORRADE_COMPARE_AS(ui.textLayer().style(label.data()),
            previousStyle,
            TestSuite::Compare::NotEqual);
    else CORRADE_COMPARE(label.data(), DataHandle::Null);
}

void LabelTest::setIcon() {
    Label label{{ui, rootNode, {16, 32}}, Icon::No, LabelStyle::Default};
    CORRADE_COMPARE(label.icon(), Icon::No);
    CORRADE_COMPARE(ui.textLayer().glyphCount(label.data()), 1);

    /* Clear the icon data to be able to verify that it gets updated */
    ui.textLayer().setText(label.data(), "", {});
    CORRADE_COMPARE(ui.textLayer().glyphCount(label.data()), 0);

    label.setIcon(Icon::Yes);
    CORRADE_COMPARE(label.icon(), Icon::Yes);
    CORRADE_COMPARE(ui.textLayer().glyphCount(label.data()), 1);
}

void LabelTest::setIconFromText() {
    Label label{{ui, rootNode, {16, 32}}, "hello", LabelStyle::Default};
    CORRADE_COMPARE(label.icon(), Icon::None);
    CORRADE_VERIFY(ui.isHandleValid(label.data()));

    /* It should reuse the same data instead of recreating */
    DataHandle previousData = label.data();
    label.setIcon(Icon::Yes);
    CORRADE_COMPARE(label.icon(), Icon::Yes);
    CORRADE_COMPARE(label.data(), previousData);
    CORRADE_VERIFY(ui.isHandleValid(label.data()));
    CORRADE_COMPARE(ui.textLayer().glyphCount(label.data()), 1);
}

void LabelTest::setIconFromEmpty() {
    Label label{{ui, rootNode, {16, 32}}, Icon::None, LabelStyle::Danger};
    CORRADE_COMPARE(label.icon(), Icon::None);
    CORRADE_COMPARE(label.data(), DataHandle::Null);
    CORRADE_COMPARE(ui.textLayer().usedCount(), 0);

    label.setIcon(Icon::Yes);
    CORRADE_COMPARE(label.icon(), Icon::Yes);
    CORRADE_VERIFY(ui.isHandleValid(label.data()));
    CORRADE_COMPARE(ui.textLayer().glyphCount(label.data()), 1);
}

void LabelTest::setIconEmpty() {
    Label label{{ui, rootNode, {16, 32}}, Icon::No, LabelStyle::Primary};
    CORRADE_COMPARE(label.icon(), Icon::No);
    CORRADE_VERIFY(ui.isHandleValid(label.data()));
    CORRADE_COMPARE(ui.textLayer().usedCount(), 1);

    /* The original icon data should be removed */
    label.setIcon(Icon::None);
    CORRADE_COMPARE(label.icon(), Icon::None);
    CORRADE_COMPARE(label.data(), DataHandle::Null);
    CORRADE_COMPARE(ui.textLayer().usedCount(), 0);
}

void LabelTest::setIconEmptyFromText() {
    Label label{{ui, rootNode, {16, 32}}, "hello", LabelStyle::Default};
    CORRADE_COMPARE(label.icon(), Icon::None);
    CORRADE_VERIFY(ui.isHandleValid(label.data()));
    CORRADE_COMPARE(ui.textLayer().usedCount(), 1);

    /* The original text data should be removed */
    label.setIcon(Icon::None);
    CORRADE_COMPARE(label.icon(), Icon::None);
    CORRADE_COMPARE(label.data(), DataHandle::Null);
    CORRADE_COMPARE(ui.textLayer().usedCount(), 0);
}

void LabelTest::setText() {
    Label label{{ui, rootNode, {16, 32}}, "hello", LabelStyle::Default};
    CORRADE_COMPARE(ui.textLayer().glyphCount(label.data()), 5);

    label.setText("wonderful!!");
    CORRADE_COMPARE(ui.textLayer().glyphCount(label.data()), 11);
}

void LabelTest::setTextTextProperties() {
    Label label{{ui, rootNode, {16, 32}}, "hello", LabelStyle::Default};
    CORRADE_COMPARE(ui.textLayer().glyphCount(label.data()), 5);

    label.setText("wonderful!!",
        TextProperties{}.setScript(Text::Script::Braille));
    /* Multiplied by 6 because of the Braille script */
    CORRADE_COMPARE(ui.textLayer().glyphCount(label.data()), 11*6);
}

void LabelTest::setTextFromIcon() {
    Label label{{ui, rootNode, {16, 32}}, Icon::No, LabelStyle::Default};
    CORRADE_VERIFY(ui.isHandleValid(label.data()));
    CORRADE_COMPARE(ui.textLayer().usedCount(), 1);

    /* It should reuse the same data instead of recreating */
    DataHandle previousData = label.data();
    label.setText("wonderful!!");
    CORRADE_COMPARE(label.icon(), Icon::None);
    CORRADE_COMPARE(label.data(), previousData);
    CORRADE_VERIFY(ui.isHandleValid(label.data()));
    CORRADE_COMPARE(ui.textLayer().glyphCount(label.data()), 11);
}

void LabelTest::setTextFromEmpty() {
    Label label{{ui, rootNode, {16, 32}}, ""};
    CORRADE_COMPARE(label.data(), DataHandle::Null);
    CORRADE_COMPARE(ui.textLayer().usedCount(), 0);

    label.setText("wonderful!!");
    CORRADE_COMPARE(label.icon(), Icon::None);
    CORRADE_VERIFY(ui.isHandleValid(label.data()));
    CORRADE_COMPARE(ui.textLayer().glyphCount(label.data()), 11);
}

void LabelTest::setTextEmpty() {
    Label label{{ui, rootNode, {16, 32}}, "hello", LabelStyle::Default};
    CORRADE_VERIFY(ui.isHandleValid(label.data()));
    CORRADE_COMPARE(ui.textLayer().usedCount(), 1);

    /* The original text data should be removed */
    label.setText("");
    CORRADE_COMPARE(label.icon(), Icon::None);
    CORRADE_COMPARE(label.data(), DataHandle::Null);
    CORRADE_COMPARE(ui.textLayer().usedCount(), 0);
}

void LabelTest::setTextEmptyFromIcon() {
    Label label{{ui, rootNode, {16, 32}}, Icon::No, LabelStyle::Info};
    CORRADE_COMPARE(label.icon(), Icon::No);
    CORRADE_VERIFY(ui.isHandleValid(label.data()));
    CORRADE_COMPARE(ui.textLayer().usedCount(), 1);

    /* The original icon data should be removed */
    label.setText("");
    CORRADE_COMPARE(label.icon(), Icon::None);
    CORRADE_COMPARE(label.data(), DataHandle::Null);
    CORRADE_COMPARE(ui.textLayer().usedCount(), 0);
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::LabelTest)
