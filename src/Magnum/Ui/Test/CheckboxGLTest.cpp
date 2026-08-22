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

#include "Magnum/Ui/Checkbox.h"
#include "Magnum/Ui/EnumStorage.h"
#include "Magnum/Ui/Theme.h"
#include "Magnum/Ui/Test/ThemeGLTester.hpp"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct CheckboxGLTest: ThemeGLTester {
    explicit CheckboxGLTest();

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
    /* No EssentialAnimations as those aren't affecting Checkbox in any way */
    {"dark, Animations", "dark-", true, Containers::pointer<DarkTheme>(DarkTheme::Feature::Animations)},
};

constexpr Vector2 WidgetSize{76, 32};

const struct {
    const char* name;
    NodeHandle(*create)(UserInterface&, Int, Flags, Int);
} TestData[]{
    {"stateless",
        [](UserInterface& ui, Int style, Flags, Int counter) {
            EnumStorage<Int> storage{ui, DirectInit, style % 2 == 0 ? 0 : 1, StorageFlag::ReferenceCounted};
            return style/2 % 2 == 0 ?
                checkbox(Anchor{ui, {}, WidgetSize}, storage.value<1>(), counter % 2 ? "Pick" : "Toggle").node() :
                radioButton(Anchor{ui, {}, WidgetSize}, storage.value<1>(), counter % 2 ? "Pick" : "Toggle").node();
        }},
    {nullptr,
        [](UserInterface& ui, Int style, Flags, Int counter) {
            return Checkbox{NonOwned, Anchor{ui, {}, WidgetSize},
                EnumStorage<Int>{ui, DirectInit, style % 2 == 0 ? 0 : 1, StorageFlag::ReferenceCounted}.value<1>(),
                counter % 2 ? "Pick" : "Toggle",
                style/2 % 2 == 0 ? CheckboxStyle::Checkbox : CheckboxStyle::RadioButton}.node();
        }},
    /* Tests including a setter for the query */
    {"setters",
        [](UserInterface& ui, Int style, Flags, Int counter) {
            Checkbox checkbox{NonOwned, Anchor{ui, {}, WidgetSize},
                /* Setting to a value opposite from the setChecked() below to
                   verify that both directions work properly. Setting to false
                   wouldn't work if this wouldn't be an EnumSet storage. */
                EnumStorage<Int>{ui, DirectInit, style % 2 == 0 ? 1 : 0,
                    StorageFlag::ReferenceCounted}
                        .setEnumSet(true)
                        .value<1>(),
                {},
                style/2 % 2 == 0 ? CheckboxStyle::Checkbox : CheckboxStyle::RadioButton};
            checkbox.setChecked(style % 2 == 0 ? false : true);
            checkbox.setText(counter % 2 ? "Pick" : "Toggle");
            return checkbox.node();
        }},
    /* This verifies also that the icon glyph gets appropriately updated after
       a style change */
    {"setStyle()",
        [](UserInterface& ui, Int style, Flags, Int counter) {
            Checkbox checkbox{NonOwned, Anchor{ui, {}, WidgetSize},
                EnumStorage<Int>{ui, DirectInit, style % 2 == 0 ? 0 : 1, StorageFlag::ReferenceCounted}.value<1>(),
                counter % 2 ? "Pick" : "Toggle",
                style/2 % 2 == 0 ? CheckboxStyle::RadioButton : CheckboxStyle::Checkbox};
            checkbox.setStyle(style/2 % 2 == 0 ? CheckboxStyle::Checkbox : CheckboxStyle::RadioButton);
            return checkbox.node();
        }}
};

CheckboxGLTest::CheckboxGLTest(): ThemeGLTester{ThemeData} {
    addInstancedTests({&CheckboxGLTest::test},
        Containers::arraySize(TestData)*themeCount());
}

void CheckboxGLTest::test() {
    auto&& data = TestData[testCaseInstanceId()/themeCount()];
    auto&& themeData = ThemeData[testCaseInstanceId()%themeCount()];
    if(!data.name)
        setTestCaseDescription(themeData.name);
    else
        setTestCaseDescription(Utility::format("{}, {}", data.name, themeData.name));

    CORRADE_VERIFY(true); /* Capture correct function name */

    ThemeGLTester::render(data.create, themeData, "checkbox.png",
        Flag::HoveredPressed|Flag::Disabled|Flag::XfailLlvmpipe20,
        /* Checkbox fade out animations are all 0.5 sec */
        4, 0.5_sec, 2.25f, 0.0466f);
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::CheckboxGLTest)
