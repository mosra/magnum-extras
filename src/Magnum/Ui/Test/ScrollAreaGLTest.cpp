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

#include <Corrade/Utility/Format.h>

#include "Magnum/Ui/ScrollArea.h"
#include "Magnum/Ui/Theme.h"
#include "Magnum/Ui/Test/ThemeGLTester.hpp"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct ScrollAreaGLTest: ThemeGLTester {
    /* For accessing the contentsRectLayerShared from within the lambdas */
    /** @todo make this not use an internal undocumented API, or better none
        of this mess */
    static ScrollAreaGLTest& instance() {
        return static_cast<ScrollAreaGLTest&>(TestSuite::Tester::instance());
    }

    explicit ScrollAreaGLTest();

    void x();
    void y();
    void xy();

    /* For displaying a rect for the contents, as there's currently nothing
       better to use */
    BaseLayerGL::Shared contentsRectLayerShared{NoCreate};
};

using namespace Math::Literals;

const Theme ThemeData[]{
    {"dark", "dark-", false, Containers::pointer<DarkTheme>()},
    {"dark SubdividedQuads", "dark-", false, []{
        Containers::Pointer<DarkTheme> style{InPlaceInit};
        style->setBaseLayerFlags(BaseLayerSharedFlag::SubdividedQuads, {});
        return style;
    }()},
    /* No EssentialAnimations as those aren't affecting ScrollArea in any
       way */
    {"dark, Animations", "dark-", true, Containers::pointer<DarkTheme>(DarkTheme::Feature::Animations)},
};

ScrollAreaGLTest::ScrollAreaGLTest(): ThemeGLTester{ThemeData} {
    addInstancedTests({&ScrollAreaGLTest::x,
                       &ScrollAreaGLTest::y,
                       &ScrollAreaGLTest::xy},
        themeCount());

    /* A single style with a (non-smooth) red outline and transparent
       background for visualizing where it is scrolling */
    contentsRectLayerShared = BaseLayerGL::Shared{BaseLayerGL::Shared::Configuration{1}};
    contentsRectLayerShared.setStyle(
        BaseLayerCommonStyleUniform{}, /* deliberately no smoothness */
        {BaseLayerStyleUniform{}
            .setColor(0x00000000_rgbaf)
            .setOutlineWidth(1.0f)
            .setOutlineColor(0xff0000ff_rgbaf*0.25f)},
        {});
}

void ScrollAreaGLTest::x() {
    auto&& themeData = ThemeData[testCaseInstanceId()%themeCount()];
    setTestCaseDescription(themeData.name);

    CORRADE_VERIFY(true); /* Capture correct function name */

    /* Style is:
        0 small contents 0%
        1 small contents 100%
        2 large contents (i.e., hitting the min thumb size) 0%
        3 large contents (i.e., hitting the min thumb size) 100%
        4 contents smaller than the view, thus no thumb shown */
    ThemeGLTester::render([](UserInterface& ui, Int style, Flags flags, Int) {
        /* For no thumb shown, skip everything besides default and disabled
           state as there's nothing to hover or press */
        if(style == 4 && flags != Flags{} && flags != Flag::Disabled)
            return NodeHandle::Null;

        /* There's always one widget per UI, so add a layer for the contents
           rectangle; add it as the first so below the data layer */
        BaseLayer& contentsRectLayer = ui.setLayerInstance(Containers::pointer<BaseLayerGL>(ui.createLayer(ui.dataLayer()), ScrollAreaGLTest::instance().contentsRectLayerShared));

        ScrollArea scroll{{ui, {}, {60, 40}}, ScrollAreaFlag::OnlyX};
        contentsRectLayer.create(0, scroll.contentsNode());
        if(style == 0 || style == 1)
            ui.setNodeSizeX(scroll.contentsNode(), 90);
        else if(style == 2 || style == 3)
            ui.setNodeSizeX(scroll.contentsNode(), 10000);
        else if(style == 4)
            ui.setNodeSizeX(scroll.contentsNode(), 20);
        else CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        if(style == 1 || style == 3) {
            ui.update(); /* Scrolling works only after an initial layout */
            scroll.scrollToPercentageX(100.0f);
        }
        NodeHandle out = scroll.scrollbarThumbXNode();
        scroll.release();
        return out;
    },
        themeData, "scrollarea-x.png",
        Flag::Hovered|Flag::Pressed|Flag::Disabled,
        /* ScrollArea fade out animations are all 0.5 sec */
        5, 0.5_sec, 4.0f, 0.0063f);
}

void ScrollAreaGLTest::y() {
    auto&& themeData = ThemeData[testCaseInstanceId()%themeCount()];
    setTestCaseDescription(themeData.name);

    CORRADE_VERIFY(true); /* Capture correct function name */

    /* Same as x(), just with the axes flipped */
    ThemeGLTester::render([](UserInterface& ui, Int style, Flags flags, Int) {
        if(style == 4 && flags != Flags{} && flags != Flag::Disabled)
            return NodeHandle::Null;

        BaseLayer& contentsRectLayer = ui.setLayerInstance(Containers::pointer<BaseLayerGL>(ui.createLayer(ui.dataLayer()), ScrollAreaGLTest::instance().contentsRectLayerShared));

        ScrollArea scroll{{ui, {}, {40, 60}}, ScrollAreaFlag::OnlyY};
        contentsRectLayer.create(0, scroll.contentsNode());
        if(style == 0 || style == 1)
            ui.setNodeSizeY(scroll.contentsNode(), 90);
        else if(style == 2 || style == 3)
            ui.setNodeSizeY(scroll.contentsNode(), 10000);
        else if(style == 4)
            ui.setNodeSizeY(scroll.contentsNode(), 20);
        else CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        if(style == 1 || style == 3) {
            ui.update();
            scroll.scrollToPercentageY(100.0f);
        }
        NodeHandle out = scroll.scrollbarThumbYNode();
        scroll.release();
        return out;
    },
        themeData, "scrollarea-y.png",
        Flag::Hovered|Flag::Pressed|Flag::Disabled,
        /* ScrollArea fade out animations are all 0.5 sec */
        5, 0.5_sec, 4.0f, 0.0063f);
}

void ScrollAreaGLTest::xy() {
    auto&& themeData = ThemeData[testCaseInstanceId()%themeCount()];
    setTestCaseDescription(themeData.name);

    CORRADE_VERIFY(true); /* Capture correct function name */

    /* Style is:
         0 small contents, hover/press x 0%
         1 small contents, hover/press x 100%
         2 small contents, hover/press y 0%
         3 small contents, hover/press y 100%
         4 large contents, hover/press x (i.e., min size) 0%
         5 large contents, hover/press x (i.e., min size) 100%
         6 large contents, hover/press y (i.e., min size) 0%
         7 large contents, hover/press y (i.e., min size) 100%
         8 contents smaller than the view on x, thus no x thumb shown
         9 contents smaller than the view on y, thus no y thumb shown
        10 contents smaller than the view on xy, thus no xy thumb shown */
    ThemeGLTester::render([](UserInterface& ui, Int style, Flags flags, Int) {
        /* For no thumbs shown, skip everything besides default and disabled
           state as there's nothing to hover or press or it was tested
           sufficiently before */
        if(style >= 8 && flags != Flags{} && flags != Flag::Disabled)
            return NodeHandle::Null;

        BaseLayer& contentsRectLayer = ui.setLayerInstance(Containers::pointer<BaseLayerGL>(ui.createLayer(ui.dataLayer()), ScrollAreaGLTest::instance().contentsRectLayerShared));

        ScrollArea scroll{{ui, {}, {80, 80}}};
        contentsRectLayer.create(0, scroll.contentsNode());
        if(style == 0 || style == 1 || style == 2 || style == 3)
            ui.setNodeSize(scroll.contentsNode(), {120, 120});
        else if(style == 4 || style == 5 || style == 6 || style == 7)
            ui.setNodeSize(scroll.contentsNode(), {10000, 10000});
        else if(style == 8)
            ui.setNodeSize(scroll.contentsNode(), {20, 120});
        else if(style == 9)
            ui.setNodeSize(scroll.contentsNode(), {120, 20});
        else if(style == 10)
            ui.setNodeSize(scroll.contentsNode(), {20, 20});
        else CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        if(style == 1 || style == 3 || style == 5 || style == 7) {
            ui.update(); /* Scrolling works only after an initial layout */
            scroll.scrollToPercentage(Vector2{100.0f});
        }
        NodeHandle out;
        if(style == 0 || style == 1 || style == 4 || style == 5 || style == 8 || style == 10)
            out = scroll.scrollbarThumbXNode();
        else if(style == 2 || style == 3 || style == 6 || style == 7 || style == 9)
            out = scroll.scrollbarThumbYNode();
        else CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        scroll.release();
        return out;
    },
        themeData, "scrollarea-xy.png",
        Flag::Hovered|Flag::Pressed|Flag::Disabled,
        /* ScrollArea fade out animations are all 0.5 sec */
        11, 0.5_sec, 4.0f, 0.0022f);
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::ScrollAreaGLTest)
