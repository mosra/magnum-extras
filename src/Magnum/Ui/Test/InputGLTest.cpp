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

#include "Magnum/Ui/Input.h"
#include "Magnum/Ui/Theme.h"
#include "Magnum/Ui/Test/ThemeGLTester.hpp"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct InputGLTest: ThemeGLTester {
    explicit InputGLTest();

    void test();
};

using namespace Math::Literals;

const Theme ThemeData[]{
    {"dark", "dark-", false, Containers::pointer<DarkTheme>()},
    {"dark SubdividedQuads", "dark-", false, []{
        Containers::Pointer<DarkTheme> style{InPlaceInit};
        style->setBaseLayerFlags(BaseLayerSharedFlag::SubdividedQuads, {});
        return style;
    }()},
    {"dark, EssentialAnimations", "dark-", true, Containers::pointer<DarkTheme>(DarkTheme::Feature::EssentialAnimations)},
    {"dark, Animations", "dark-", true, Containers::pointer<DarkTheme>(DarkTheme::Feature::Animations)},
};

const struct {
    const char* name;
    NodeHandle(*create)(UserInterface&, Int, Flags, Int);
} TestData[]{
    {nullptr,
        [](UserInterface& ui, Int style, Flags, Int counter) {
            Input input{{ui, {}, {64, 36}}, counter % 2 ? "Edit..." : "Type?", InputStyle(style)};
            /** @todo use a cursor setting API once it exists */
            ui.textLayer().setCursor(input.textData(), counter % 2 ? 2 : 5, counter % 2 ? 5 : 2);
            return input.release();
        }},
    {"setters",
        [](UserInterface& ui, Int style, Flags, Int counter) {
            Input input{{ui, {}, {64, 36}}, "", InputStyle(style)};
            input.setText(counter % 2 ? "Edit..." : "Type?");
            /** @todo use a cursor setting API once it exists */
            ui.textLayer().setCursor(input.textData(), counter % 2 ? 2 : 5, counter % 2 ? 5 : 2);
            return input.release();
        }},
    {"setStyle()",
        [](UserInterface& ui, Int style, Flags, Int counter) {
            Input input{{ui, {}, {64, 36}}, counter % 2 ? "Edit..." : "Type?", InputStyle(style == 0 ? 1 : 0)};
            input.setStyle(InputStyle(style));
            /** @todo use a cursor setting API once it exists */
            ui.textLayer().setCursor(input.textData(), counter % 2 ? 2 : 5, counter % 2 ? 5 : 2);
            return input.release();
        }},
};

InputGLTest::InputGLTest(): ThemeGLTester{ThemeData} {
    addInstancedTests({&InputGLTest::test},
        Containers::arraySize(TestData)*themeCount());
}

void InputGLTest::test() {
    auto&& data = TestData[testCaseInstanceId()/themeCount()];
    auto&& themeData = ThemeData[testCaseInstanceId()%themeCount()];
    if(!data.name)
        setTestCaseDescription(themeData.name);
    else
        setTestCaseDescription(Utility::format("{}, {}", data.name, themeData.name));

    CORRADE_VERIFY(true); /* Capture correct function name */

    ThemeGLTester::render(data.create, themeData, "input.png",
        Flag::Hovered|Flag::Pressed|Flag::Focused|Flag::Disabled|Flag::XfailLlvmpipe20,
        /* Input cursor blinking lasts 0.55 sec and is reversed every other
           iteration, so it'll be fully visible at twice as much */
        5, 0.55_sec*2, 2.0f, 0.02292f);
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::InputGLTest)
