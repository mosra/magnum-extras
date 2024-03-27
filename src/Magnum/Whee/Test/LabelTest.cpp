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
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/Numeric.h>
#include <Corrade/Utility/DebugStl.h> /** @todo remove once Debug is stream-free */

#include "Magnum/Whee/Label.h"
#include "Magnum/Whee/Handle.h"
#include "Magnum/Whee/Style.h"
#include "Magnum/Whee/TextProperties.h"

#include "Magnum/Whee/Test/WidgetTest.hpp"

namespace Magnum { namespace Whee { namespace Test { namespace {

struct LabelTest: TestSuite::Tester {
    explicit LabelTest();

    void debugStyle();

    void setup();
    void teardown();

    void constructEmpty();
    void constructIconOnly();
    void constructTextOnly();
    void constructTextOnlyTextProperties();
    void constructIconText();
    void constructIconTextTextProperties();

    void setStyle();

    void setIcon();
    void setIconFromTextOnly();
    void setIconEmpty();
    void setIconEmptyFromTextOnly();

    void setText();
    void setTextTextProperties();
    void setTextFromIconOnly();
    void setTextEmpty();
    void setTextEmptyFromIconOnly();

    TestBaseLayerShared baseLayerShared;
    TestTextLayerShared textLayerShared;
    TestUserInterface ui{NoCreate};
    /* Deliberately an invalid non-null handle initially, to make sure nothing
       is parented to it before it's populated in setup() */
    NodeHandle rootNode = nodeHandle(0xfffff, 0xfff);
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

LabelTest::LabelTest() {
    addTests({&LabelTest::debugStyle});

    addTests({&LabelTest::constructEmpty,
              &LabelTest::constructIconOnly,
              &LabelTest::constructTextOnly,
              &LabelTest::constructTextOnlyTextProperties,
              &LabelTest::constructIconText,
              &LabelTest::constructIconTextTextProperties},
        &LabelTest::setup,
        &LabelTest::teardown);

    addInstancedTests({&LabelTest::setStyle},
        Containers::arraySize(SetStyleData),
        &LabelTest::setup,
        &LabelTest::teardown);

    addTests({&LabelTest::setIcon,
              &LabelTest::setIconFromTextOnly,
              &LabelTest::setIconEmpty,
              &LabelTest::setIconEmptyFromTextOnly,

              &LabelTest::setText,
              &LabelTest::setTextTextProperties,
              &LabelTest::setTextFromIconOnly,
              &LabelTest::setTextEmpty,
              &LabelTest::setTextEmptyFromIconOnly},
        &LabelTest::setup,
        &LabelTest::teardown);

    ui.setBaseLayerInstance(Containers::pointer<TestBaseLayer>(ui.createLayer(), baseLayerShared));
    ui.setTextLayerInstance(Containers::pointer<TestTextLayer>(ui.createLayer(), textLayerShared));
}

void LabelTest::debugStyle() {
    std::ostringstream out;
    Debug{&out} << LabelStyle::Success << LabelStyle(0xef);
    CORRADE_COMPARE(out.str(), "Whee::LabelStyle::Success Whee::LabelStyle(0xef)\n");
}

void LabelTest::setup() {
    CORRADE_INTERNAL_ASSERT(!ui.isHandleValid(rootNode));
    CORRADE_INTERNAL_ASSERT(ui.nodeUsedCount() == 0);
    CORRADE_INTERNAL_ASSERT(ui.baseLayer().usedCount() == 0);
    CORRADE_INTERNAL_ASSERT(ui.textLayer().usedCount() == 0);
    rootNode = ui.createNode({}, ui.size());
}

void LabelTest::teardown() {
    ui.removeNode(rootNode);
    ui.clean();
    CORRADE_INTERNAL_ASSERT(!ui.isHandleValid(rootNode));
    CORRADE_INTERNAL_ASSERT(ui.nodeUsedCount() == 0);
    CORRADE_INTERNAL_ASSERT(ui.baseLayer().usedCount() == 0);
    CORRADE_INTERNAL_ASSERT(ui.textLayer().usedCount() == 0);
}

void LabelTest::constructEmpty() {
    {
        NodeHandle node1 = label(ui, rootNode, LabelStyle::Success, {32, 16}, Icon::None);
        NodeHandle node2 = label(ui, rootNode, LabelStyle::Success, {32, 16}, "");
        NodeHandle node3 = label(ui, rootNode, LabelStyle::Success, {32, 16}, Icon::None, "");
        CORRADE_COMPARE(ui.nodeParent(node1), rootNode);
        CORRADE_COMPARE(ui.nodeParent(node2), rootNode);
        CORRADE_COMPARE(ui.nodeParent(node3), rootNode);
        CORRADE_COMPARE(ui.nodeOffset(node1), Vector2{});
        CORRADE_COMPARE(ui.nodeOffset(node2), Vector2{});
        CORRADE_COMPARE(ui.nodeOffset(node3), Vector2{});
        CORRADE_COMPARE(ui.nodeSize(node1), (Vector2{32, 16}));
        CORRADE_COMPARE(ui.nodeSize(node2), (Vector2{32, 16}));
        CORRADE_COMPARE(ui.nodeSize(node3), (Vector2{32, 16}));

        /* Can only verify that the data were (not) created, nothing else.
           Visually tested in StyleGLTest. */
        CORRADE_COMPARE(ui.baseLayer().usedCount(), 0);
        CORRADE_COMPARE(ui.textLayer().usedCount(), 0);
    } {
        Label label1{ui, rootNode, LabelStyle::Success, {32, 16}, Icon::None};
        Label label2{ui, rootNode, LabelStyle::Success, {32, 16}, ""};
        Label label3{ui, rootNode, LabelStyle::Success, {32, 16}, Icon::None, ""};
        CORRADE_COMPARE(ui.nodeParent(label1), rootNode);
        CORRADE_COMPARE(ui.nodeParent(label2), rootNode);
        CORRADE_COMPARE(ui.nodeParent(label3), rootNode);
        CORRADE_COMPARE(ui.nodeOffset(label1), Vector2{});
        CORRADE_COMPARE(ui.nodeOffset(label2), Vector2{});
        CORRADE_COMPARE(ui.nodeOffset(label3), Vector2{});
        CORRADE_COMPARE(ui.nodeSize(label1), (Vector2{32, 16}));
        CORRADE_COMPARE(ui.nodeSize(label2), (Vector2{32, 16}));
        CORRADE_COMPARE(ui.nodeSize(label3), (Vector2{32, 16}));

        CORRADE_COMPARE(label1.style(), LabelStyle::Success);
        CORRADE_COMPARE(label2.style(), LabelStyle::Success);
        CORRADE_COMPARE(label3.style(), LabelStyle::Success);
        CORRADE_COMPARE(label1.icon(), Icon::None);
        CORRADE_COMPARE(label2.icon(), Icon::None);
        CORRADE_COMPARE(label3.icon(), Icon::None);

        CORRADE_COMPARE(label1.iconData(), DataHandle::Null);
        CORRADE_COMPARE(label2.iconData(), DataHandle::Null);
        CORRADE_COMPARE(label3.iconData(), DataHandle::Null);
        CORRADE_COMPARE(label1.textData(), DataHandle::Null);
        CORRADE_COMPARE(label2.textData(), DataHandle::Null);
        CORRADE_COMPARE(label3.textData(), DataHandle::Null);
    }
}

void LabelTest::constructIconOnly() {
    {
        NodeHandle node1 = label(ui, rootNode, LabelStyle::Success, {32, 16}, Icon::Yes);
        NodeHandle node2 = label(ui, rootNode, LabelStyle::Success, {32, 16}, Icon::Yes, "");
        CORRADE_COMPARE(ui.nodeParent(node1), rootNode);
        CORRADE_COMPARE(ui.nodeParent(node2), rootNode);
        CORRADE_COMPARE(ui.nodeOffset(node1), Vector2{});
        CORRADE_COMPARE(ui.nodeOffset(node2), Vector2{});
        CORRADE_COMPARE(ui.nodeSize(node1), (Vector2{32, 16}));
        CORRADE_COMPARE(ui.nodeSize(node2), (Vector2{32, 16}));

        /* Can only verify that the data were created, nothing else. Visually
           tested in StyleGLTest. */
        CORRADE_COMPARE(ui.baseLayer().usedCount(), 0);
        CORRADE_COMPARE(ui.textLayer().usedCount(), 2);
    } {
        Label label1{ui, rootNode, LabelStyle::Success, {32, 16}, Icon::Yes};
        Label label2{ui, rootNode, LabelStyle::Success, {32, 16}, Icon::Yes, ""};
        CORRADE_COMPARE(ui.nodeParent(label1), rootNode);
        CORRADE_COMPARE(ui.nodeParent(label2), rootNode);
        CORRADE_COMPARE(ui.nodeOffset(label1), Vector2{});
        CORRADE_COMPARE(ui.nodeOffset(label2), Vector2{});
        CORRADE_COMPARE(ui.nodeSize(label1), (Vector2{32, 16}));
        CORRADE_COMPARE(ui.nodeSize(label2), (Vector2{32, 16}));

        CORRADE_COMPARE(label1.style(), LabelStyle::Success);
        CORRADE_COMPARE(label2.style(), LabelStyle::Success);
        CORRADE_COMPARE(label1.icon(), Icon::Yes);
        CORRADE_COMPARE(label2.icon(), Icon::Yes);

        CORRADE_VERIFY(ui.isHandleValid(label1.iconData()));
        CORRADE_VERIFY(ui.isHandleValid(label2.iconData()));
        CORRADE_COMPARE(ui.textLayer().glyphCount(label1.iconData()), 1);
        CORRADE_COMPARE(ui.textLayer().glyphCount(label2.iconData()), 1);
        CORRADE_COMPARE(label1.textData(), DataHandle::Null);
        CORRADE_COMPARE(label2.textData(), DataHandle::Null);
    }
}

void LabelTest::constructTextOnly() {
    {
        NodeHandle node1 = label(ui, rootNode, LabelStyle::Danger, {32, 16}, "hello!");
        NodeHandle node2 = label(ui, rootNode, LabelStyle::Danger, {32, 16}, Icon::None, "hello!");
        CORRADE_COMPARE(ui.nodeParent(node1), rootNode);
        CORRADE_COMPARE(ui.nodeParent(node2), rootNode);
        CORRADE_COMPARE(ui.nodeOffset(node1), Vector2{});
        CORRADE_COMPARE(ui.nodeOffset(node2), Vector2{});
        CORRADE_COMPARE(ui.nodeSize(node1), (Vector2{32, 16}));
        CORRADE_COMPARE(ui.nodeSize(node2), (Vector2{32, 16}));

        /* Can only verify that the data were created, nothing else. Visually
           tested in StyleGLTest. */
        CORRADE_COMPARE(ui.baseLayer().usedCount(), 0);
        CORRADE_COMPARE(ui.textLayer().usedCount(), 2);
    } {
        Label label1{ui, rootNode, LabelStyle::Danger, {32, 16}, "hello!"};
        Label label2{ui, rootNode, LabelStyle::Danger, {32, 16}, Icon::None, "hello!"};
        CORRADE_COMPARE(ui.nodeParent(label1), rootNode);
        CORRADE_COMPARE(ui.nodeParent(label2), rootNode);
        CORRADE_COMPARE(ui.nodeOffset(label1), Vector2{});
        CORRADE_COMPARE(ui.nodeOffset(label2), Vector2{});
        CORRADE_COMPARE(ui.nodeSize(label1), (Vector2{32, 16}));
        CORRADE_COMPARE(ui.nodeSize(label2), (Vector2{32, 16}));

        CORRADE_COMPARE(label1.style(), LabelStyle::Danger);
        CORRADE_COMPARE(label2.style(), LabelStyle::Danger);
        CORRADE_COMPARE(label1.icon(), Icon::None);
        CORRADE_COMPARE(label2.icon(), Icon::None);

        CORRADE_COMPARE(label1.iconData(), DataHandle::Null);
        CORRADE_COMPARE(label2.iconData(), DataHandle::Null);
        CORRADE_VERIFY(ui.isHandleValid(label1.textData()));
        CORRADE_VERIFY(ui.isHandleValid(label2.textData()));
        CORRADE_COMPARE(ui.textLayer().glyphCount(label1.textData()), 6);
        CORRADE_COMPARE(ui.textLayer().glyphCount(label2.textData()), 6);
    }
}

void LabelTest::constructTextOnlyTextProperties() {
    {
        NodeHandle node1 = label(ui, rootNode, LabelStyle::Danger, {32, 16}, "hello!",
            TextProperties{}.setScript(Text::Script::Braille));
        NodeHandle node2 = label(ui, rootNode, LabelStyle::Danger, {32, 16}, Icon::None, "hello!",
            TextProperties{}.setScript(Text::Script::Braille));
        CORRADE_COMPARE(ui.nodeParent(node1), rootNode);
        CORRADE_COMPARE(ui.nodeParent(node2), rootNode);
        CORRADE_COMPARE(ui.nodeOffset(node1), Vector2{});
        CORRADE_COMPARE(ui.nodeOffset(node2), Vector2{});
        CORRADE_COMPARE(ui.nodeSize(node1), (Vector2{32, 16}));
        CORRADE_COMPARE(ui.nodeSize(node2), (Vector2{32, 16}));

        /* Can only verify that the data were created, nothing else. Visually
           tested in StyleGLTest. */
        /** @todo this doesn't verify that the properties were passed :/ */
        CORRADE_COMPARE(ui.baseLayer().usedCount(), 0);
        CORRADE_COMPARE(ui.textLayer().usedCount(), 2);
    } {
        Label label1{ui, rootNode, LabelStyle::Danger, {32, 16}, "hello!",
            TextProperties{}.setScript(Text::Script::Braille)};
        Label label2{ui, rootNode, LabelStyle::Danger, {32, 16}, Icon::None, "hello!",
            TextProperties{}.setScript(Text::Script::Braille)};
        CORRADE_COMPARE(ui.nodeParent(label1), rootNode);
        CORRADE_COMPARE(ui.nodeParent(label2), rootNode);
        CORRADE_COMPARE(ui.nodeOffset(label1), Vector2{});
        CORRADE_COMPARE(ui.nodeOffset(label2), Vector2{});
        CORRADE_COMPARE(ui.nodeSize(label1), (Vector2{32, 16}));
        CORRADE_COMPARE(ui.nodeSize(label2), (Vector2{32, 16}));

        CORRADE_COMPARE(label1.style(), LabelStyle::Danger);
        CORRADE_COMPARE(label2.style(), LabelStyle::Danger);
        CORRADE_COMPARE(label1.icon(), Icon::None);
        CORRADE_COMPARE(label2.icon(), Icon::None);

        CORRADE_COMPARE(label1.iconData(), DataHandle::Null);
        CORRADE_COMPARE(label2.iconData(), DataHandle::Null);
        CORRADE_VERIFY(ui.isHandleValid(label1.textData()));
        CORRADE_VERIFY(ui.isHandleValid(label2.textData()));
        /* Multiplied by 6 because of the Braille script */
        CORRADE_COMPARE(ui.textLayer().glyphCount(label1.textData()), 6*6);
        CORRADE_COMPARE(ui.textLayer().glyphCount(label2.textData()), 6*6);
    }
}

void LabelTest::constructIconText() {
    {
        NodeHandle node = label(ui, rootNode, LabelStyle::Dim, {32, 16}, Icon::No, "bye!");
        CORRADE_COMPARE(ui.nodeParent(node), rootNode);
        CORRADE_COMPARE(ui.nodeOffset(node), Vector2{});
        CORRADE_COMPARE(ui.nodeSize(node), (Vector2{32, 16}));

        /* Can only verify that the data were created, nothing else. Visually
           tested in StyleGLTest. */
        CORRADE_COMPARE(ui.baseLayer().usedCount(), 0);
        CORRADE_COMPARE(ui.textLayer().usedCount(), 2);
    } {
        Label label{ui, rootNode, LabelStyle::Dim, {32, 16}, Icon::No, "bye!"};
        CORRADE_COMPARE(ui.nodeParent(label), rootNode);
        CORRADE_COMPARE(ui.nodeOffset(label), Vector2{});
        CORRADE_COMPARE(ui.nodeSize(label), (Vector2{32, 16}));

        CORRADE_COMPARE(label.style(), LabelStyle::Dim);
        CORRADE_COMPARE(label.icon(), Icon::No);

        CORRADE_VERIFY(ui.isHandleValid(label.iconData()));
        CORRADE_COMPARE(ui.textLayer().glyphCount(label.iconData()), 1);
        CORRADE_VERIFY(ui.isHandleValid(label.textData()));
        CORRADE_COMPARE(ui.textLayer().glyphCount(label.textData()), 4);
    }
}

void LabelTest::constructIconTextTextProperties() {
    {
        NodeHandle node = label(ui, rootNode, LabelStyle::Dim, {32, 16}, Icon::No, "bye!",
            TextProperties{}.setScript(Text::Script::Braille));
        CORRADE_COMPARE(ui.nodeParent(node), rootNode);
        CORRADE_COMPARE(ui.nodeOffset(node), Vector2{});
        CORRADE_COMPARE(ui.nodeSize(node), (Vector2{32, 16}));

        /* Can only verify that the data were created, nothing else. Visually
           tested in StyleGLTest. */
        /** @todo this doesn't verify that the properties were passed :/ */
        CORRADE_COMPARE(ui.baseLayer().usedCount(), 0);
        CORRADE_COMPARE(ui.textLayer().usedCount(), 2);
    } {
        Label label{ui, rootNode, LabelStyle::Dim, {32, 16}, Icon::No, "bye!",
            TextProperties{}.setScript(Text::Script::Braille)};
        CORRADE_COMPARE(ui.nodeParent(label), rootNode);
        CORRADE_COMPARE(ui.nodeOffset(label), Vector2{});
        CORRADE_COMPARE(ui.nodeSize(label), (Vector2{32, 16}));

        CORRADE_COMPARE(label.style(), LabelStyle::Dim);
        CORRADE_COMPARE(label.icon(), Icon::No);

        CORRADE_VERIFY(ui.isHandleValid(label.iconData()));
        /* Not multiplied as it goes directly, without the shaper */
        CORRADE_COMPARE(ui.textLayer().glyphCount(label.iconData()), 1);
        CORRADE_VERIFY(ui.isHandleValid(label.textData()));
        /* Multiplied by 6 because of the Braille script */
        CORRADE_COMPARE(ui.textLayer().glyphCount(label.textData()), 4*6);
    }
}

void LabelTest::setStyle() {
    auto&& data = SetStyleData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    Label label{ui, rootNode, LabelStyle::Dim, {32, 16}, data.icon, data.text};
    CORRADE_COMPARE(label.style(), LabelStyle::Dim);

    UnsignedInt previousStyleIcon, previousStyleText;
    if(data.icon != Icon::None)
        previousStyleIcon = ui.textLayer().style(label.iconData());
    if(data.text)
        previousStyleText = ui.textLayer().style(label.textData());

    /* The style change should result in different layer style being used, and
       the text data not being suddenly present */
    label.setStyle(LabelStyle::Success);
    CORRADE_COMPARE(label.style(), LabelStyle::Success);
    if(data.icon != Icon::None)
        CORRADE_COMPARE_AS(ui.textLayer().style(label.iconData()),
            previousStyleIcon,
            TestSuite::Compare::NotEqual);
    if(data.text)
        CORRADE_COMPARE_AS(ui.textLayer().style(label.textData()),
            previousStyleText,
            TestSuite::Compare::NotEqual);
}

void LabelTest::setIcon() {
    Label label{ui, rootNode, LabelStyle::Default, {16, 32}, Icon::No};
    CORRADE_COMPARE(label.icon(), Icon::No);
    CORRADE_COMPARE(ui.textLayer().glyphCount(label.iconData()), 1);

    /* Clear the icon data to be able to verify that it gets updated */
    ui.textLayer().setText(label.iconData(), "", {});
    CORRADE_COMPARE(ui.textLayer().glyphCount(label.iconData()), 0);

    label.setIcon(Icon::Yes);
    CORRADE_COMPARE(label.icon(), Icon::Yes);
    CORRADE_COMPARE(ui.textLayer().glyphCount(label.iconData()), 1);
}

void LabelTest::setIconFromTextOnly() {
    Label label{ui, rootNode, LabelStyle::Default, {16, 32}, "hello"};
    CORRADE_COMPARE(label.icon(), Icon::None);
    CORRADE_COMPARE(label.iconData(), DataHandle::Null);

    /* It should create the icon data now, the text should however stay as
       before */
    label.setIcon(Icon::Yes);
    CORRADE_COMPARE(label.icon(), Icon::Yes);
    CORRADE_VERIFY(ui.isHandleValid(label.iconData()));
    CORRADE_COMPARE(ui.textLayer().glyphCount(label.iconData()), 1);
    CORRADE_VERIFY(ui.isHandleValid(label.textData()));
}

void LabelTest::setIconEmpty() {
    Label label{ui, rootNode, LabelStyle::Default, {16, 32}, Icon::No};
    CORRADE_COMPARE(label.icon(), Icon::No);
    CORRADE_VERIFY(ui.isHandleValid(label.iconData()));
    CORRADE_COMPARE(ui.textLayer().usedCount(), 1);

    label.setIcon(Icon::None);
    CORRADE_COMPARE(label.icon(), Icon::None);
    CORRADE_COMPARE(label.iconData(), DataHandle::Null);
    /* The original icon data should be removed */
    CORRADE_COMPARE(ui.textLayer().usedCount(), 0);
}

void LabelTest::setIconEmptyFromTextOnly() {
    Label label{ui, rootNode, LabelStyle::Default, {16, 32}, "hello"};
    CORRADE_COMPARE(label.icon(), Icon::None);
    CORRADE_COMPARE(label.iconData(), DataHandle::Null);
    CORRADE_COMPARE(ui.textLayer().usedCount(), 1);

    /* It should just leave the icon null, the text should however stay as
       before */
    label.setIcon(Icon::None);
    CORRADE_COMPARE(label.icon(), Icon::None);
    CORRADE_COMPARE(label.iconData(), DataHandle::Null);
    CORRADE_VERIFY(ui.isHandleValid(label.textData()));
    CORRADE_COMPARE(ui.textLayer().usedCount(), 1);
}

void LabelTest::setText() {
    Label label{ui, rootNode, LabelStyle::Default, {16, 32}, "hello"};
    CORRADE_COMPARE(ui.textLayer().glyphCount(label.textData()), 5);

    label.setText("wonderful!!");
    CORRADE_COMPARE(ui.textLayer().glyphCount(label.textData()), 11);
}

void LabelTest::setTextTextProperties() {
    Label label{ui, rootNode, LabelStyle::Default, {16, 32}, "hello"};
    CORRADE_COMPARE(ui.textLayer().glyphCount(label.textData()), 5);

    label.setText("wonderful!!",
        TextProperties{}.setScript(Text::Script::Braille));
    /* Multiplied by 6 because of the Braille script */
    CORRADE_COMPARE(ui.textLayer().glyphCount(label.textData()), 11*6);
}

void LabelTest::setTextFromIconOnly() {
    Label label{ui, rootNode, LabelStyle::Default, {16, 32}, Icon::No};
    CORRADE_COMPARE(label.textData(), DataHandle::Null);

    /* It should create the text data now, the icon should however stay as
       well */
    label.setText("wonderful!!");
    CORRADE_VERIFY(ui.isHandleValid(label.textData()));
    CORRADE_COMPARE(ui.textLayer().glyphCount(label.textData()), 11);
    CORRADE_COMPARE(label.icon(), Icon::No);
    CORRADE_VERIFY(ui.isHandleValid(label.iconData()));
}

void LabelTest::setTextEmpty() {
    Label label{ui, rootNode, LabelStyle::Default, {16, 32}, "hello"};
    CORRADE_VERIFY(ui.isHandleValid(label.textData()));
    CORRADE_COMPARE(ui.textLayer().usedCount(), 1);

    label.setText("");
    CORRADE_COMPARE(label.textData(), DataHandle::Null);
    /* The original text data should be removed */
    CORRADE_COMPARE(ui.textLayer().usedCount(), 0);
}

void LabelTest::setTextEmptyFromIconOnly() {
    Label label{ui, rootNode, LabelStyle::Default, {16, 32}, Icon::No};
    CORRADE_COMPARE(label.textData(), DataHandle::Null);
    CORRADE_COMPARE(ui.textLayer().usedCount(), 1);

    /* It should just leave the text null, the icon should however stay as
       well */
    label.setText("");
    CORRADE_COMPARE(label.textData(), DataHandle::Null);
    CORRADE_COMPARE(label.icon(), Icon::No);
    CORRADE_VERIFY(ui.isHandleValid(label.iconData()));
    CORRADE_COMPARE(ui.textLayer().usedCount(), 1);
}

}}}}

CORRADE_TEST_MAIN(Magnum::Whee::Test::LabelTest)
