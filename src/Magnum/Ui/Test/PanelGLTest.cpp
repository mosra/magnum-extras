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

#include "Magnum/Ui/Panel.h"
#include "Magnum/Ui/Theme.h"
#include "Magnum/Ui/Test/ThemeGLTester.hpp"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct PanelGLTest: ThemeGLTester {
    explicit PanelGLTest();

    void test();
};

using namespace Math::Literals;

const Theme ThemeData[]{
    {"m.css dark", "mcss-dark-", false, Containers::pointer<McssDarkTheme>()},
    {"m.css dark SubdividedQuads", "mcss-dark-", false, []{
        Containers::Pointer<McssDarkTheme> style{InPlaceInit};
        style->setBaseLayerFlags(BaseLayerSharedFlag::SubdividedQuads, {});
        return style;
    }()},
    /* No animations as Panel doesn't animate anything */
};

const struct {
    const char* name;
    NodeHandle(*create)(UserInterface&, Int, Flags, Int);
} TestData[]{
    {"stateless",
        [](UserInterface& ui, Int style, Flags, Int) {
            return panel({ui, {}, {48, 36}}, PanelStyle(style)).node();
        }},
    {nullptr,
        [](UserInterface& ui, Int style, Flags, Int) {
            return Panel{{ui, {}, {48, 36}},PanelStyle(style)}.release();
        }},
    {"setStyle()",
        [](UserInterface& ui, Int style, Flags, Int) {
            Panel panel{{ui, {}, {48, 36}},PanelStyle(style == 0 ? 1 : 0)};
            panel.setStyle(PanelStyle(style));
            return panel.release();
        }},
};

PanelGLTest::PanelGLTest(): ThemeGLTester{ThemeData} {
    addInstancedTests({&PanelGLTest::test},
        Containers::arraySize(TestData)*themeCount());
}

void PanelGLTest::test() {
    auto&& data = TestData[testCaseInstanceId()/themeCount()];
    auto&& themeData = ThemeData[testCaseInstanceId()%themeCount()];
    if(!data.name)
        setTestCaseDescription(themeData.name);
    else
        setTestCaseDescription(Utility::format("{}, {}", data.name, themeData.name));

    CORRADE_VERIFY(true); /* Capture correct function name */

    ThemeGLTester::render(data.create, themeData, "panel.png",
        Flag::Disabled, 2, {}, 0.25f, 0.0339f);
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::PanelGLTest)
