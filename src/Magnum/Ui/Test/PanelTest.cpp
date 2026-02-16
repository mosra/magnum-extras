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

#include "Magnum/Ui/Anchor.h"
#include "Magnum/Ui/Panel.h"
#include "Magnum/Ui/Test/WidgetTester.hpp"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct PanelTest: WidgetTester {
    explicit PanelTest();

    void debugStyle();

    void constructDefault();
    void constructDefaultStateless();
    void constructFilled();
    void constructFilledStateless();
    void constructNoCreate();

    void setStyleNoOp();
    void setStyleFromNoBackground();
    void setStyleToNoBackground();
};

PanelTest::PanelTest() {
    addTests({&PanelTest::debugStyle});

    addTests<PanelTest>({
        &PanelTest::constructDefault,
        &PanelTest::constructDefaultStateless,
        &PanelTest::constructFilled,
        &PanelTest::constructFilledStateless,
    }, &WidgetTester::setup,
       &WidgetTester::teardown);

    addTests<PanelTest>({&PanelTest::constructNoCreate},
        &WidgetTester::setupNoCreate,
        &WidgetTester::teardownNoCreate);

    addTests<PanelTest>({
        &PanelTest::setStyleNoOp,
        &PanelTest::setStyleFromNoBackground,
        &PanelTest::setStyleToNoBackground,
    }, &WidgetTester::setup,
       &WidgetTester::teardown);
}

void PanelTest::debugStyle() {
    Containers::String out;
    Debug{&out} << PanelStyle::Filled << PanelStyle(0xef);
    CORRADE_COMPARE(out, "Ui::PanelStyle::Filled Ui::PanelStyle(0xef)\n");
}

void PanelTest::constructDefault() {
    Panel panel{{rootAnchor, {}, {32, 16}}, PanelStyle::Default};
    CORRADE_COMPARE(ui.nodeParent(panel), rootAnchor);
    CORRADE_COMPARE(ui.nodeSize(panel), (Vector2{32, 16}));

    CORRADE_COMPARE(panel.style(), PanelStyle::Default);

    CORRADE_COMPARE(panel.backgroundData(), DataHandle::Null);

    /* Can only verify that the layout data were created, they're not saved */
    CORRADE_COMPARE(ui.layoutLayer().usedCount(), 1);
}

void PanelTest::constructDefaultStateless() {
    NodeHandle node = panel({rootAnchor, {}, {32, 16}}, PanelStyle::Default);
    CORRADE_COMPARE(ui.nodeParent(node), rootAnchor);
    CORRADE_COMPARE(ui.nodeSize(node), (Vector2{32, 16}));

    /* Can only verify that the data were (not) created, nothing else. Visually
       tested in StyleGLTest. */
    CORRADE_COMPARE(ui.baseLayer().usedCount(), 0);
    CORRADE_COMPARE(ui.layoutLayer().usedCount(), 1);
}

void PanelTest::constructFilled() {
    Panel panel{{rootAnchor, {}, {32, 16}}, PanelStyle::Filled};
    CORRADE_COMPARE(ui.nodeParent(panel), rootAnchor);
    CORRADE_COMPARE(ui.nodeSize(panel), (Vector2{32, 16}));

    CORRADE_COMPARE(panel.style(), PanelStyle::Filled);

    CORRADE_VERIFY(ui.isHandleValid(panel.backgroundData()));

    /* Can only verify that the layout data were created, they're not saved */
    CORRADE_COMPARE(ui.layoutLayer().usedCount(), 1);
}

void PanelTest::constructFilledStateless() {
    NodeHandle node = panel({rootAnchor, {}, {32, 16}}, PanelStyle::Filled);
    CORRADE_COMPARE(ui.nodeParent(node), rootAnchor);
    CORRADE_COMPARE(ui.nodeSize(node), (Vector2{32, 16}));

    /* Can only verify that the data were created, nothing else. Visually
       tested in StyleGLTest. */
    CORRADE_COMPARE(ui.baseLayer().usedCount(), 1);
    CORRADE_COMPARE(ui.layoutLayer().usedCount(), 1);
}

void PanelTest::constructNoCreate() {
    Panel label{NoCreate};
    CORRADE_COMPARE(label.node(), NodeHandle::Null);
    CORRADE_COMPARE(label.backgroundData(), DataHandle::Null);
}

void PanelTest::setStyleNoOp() {
    Panel a{{rootAnchor, {}, {32, 16}}, PanelStyle::Default};
    Panel b{{rootAnchor, {}, {32, 16}}, PanelStyle::Filled};
    CORRADE_COMPARE(a.style(), PanelStyle::Default);
    CORRADE_COMPARE(b.style(), PanelStyle::Filled);
    CORRADE_COMPARE(a.backgroundData(), DataHandle::Null);
    DataHandle previousData = b.backgroundData();
    CORRADE_VERIFY(ui.isHandleValid(b.backgroundData()));

    /* The style change should result in no change, not even the data being
       recreated */
    a.setStyle(PanelStyle::Default);
    b.setStyle(PanelStyle::Filled);
    CORRADE_COMPARE(a.style(), PanelStyle::Default);
    CORRADE_COMPARE(b.style(), PanelStyle::Filled);
    CORRADE_COMPARE(a.backgroundData(), DataHandle::Null);
    CORRADE_COMPARE(b.backgroundData(), previousData);
    CORRADE_COMPARE(ui.baseLayer().usedCount(), 1);
}

void PanelTest::setStyleFromNoBackground() {
    /* Default style is ... the default */
    Panel panel{{rootAnchor, {}, {32, 16}}};
    CORRADE_COMPARE(panel.style(), PanelStyle::Default);
    CORRADE_COMPARE(panel.backgroundData(), DataHandle::Null);

    /* The style change should result in the data newly created */
    panel.setStyle(PanelStyle::Filled);
    CORRADE_COMPARE(panel.style(), PanelStyle::Filled);
    CORRADE_VERIFY(ui.isHandleValid(panel.backgroundData()));
}

void PanelTest::setStyleToNoBackground() {
    Panel panel{{rootAnchor, {}, {32, 16}}, PanelStyle::Filled};
    CORRADE_COMPARE(panel.style(), PanelStyle::Filled);
    CORRADE_VERIFY(ui.isHandleValid(panel.backgroundData()));

    /* The style change should result in the data removed */
    panel.setStyle(PanelStyle::Default);
    CORRADE_COMPARE(panel.style(), PanelStyle::Default);
    CORRADE_COMPARE(panel.backgroundData(), DataHandle::Null);
    CORRADE_COMPARE(ui.baseLayer().usedCount(), 0);
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::PanelTest)
