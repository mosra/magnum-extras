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

#include "Magnum/Ui/Icon.h"
#include "Magnum/Ui/Label.h"
#include "Magnum/Ui/Style.h"
#include "Magnum/Ui/Test/StyleGLTester.hpp"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct LabelGLTest: StyleGLTester {
    explicit LabelGLTest();

    void text();
    void icon();
};

using namespace Math::Literals;

const Style StyleData[]{
    {"m.css dark", "mcss-dark-", false, Containers::pointer<McssDarkStyle>()},
    /* No SubdividedQuads as labels currently don't have any backgrounds, no
       animations either */
};

const struct {
    const char* name;
    NodeHandle(*create)(UserInterface&, Int, Int);
} TextData[]{
    {"stateless",
        [](UserInterface& ui, Int style, Int counter) {
            return label({ui, {}, {52, 36}}, counter % 3 ? "Bye" : "Hello!", LabelStyle(style)).node();
        }},
    {nullptr,
        [](UserInterface& ui, Int style, Int counter) {
            return Label{{ui, {}, {52, 36}}, counter % 3 ? "Bye" : "Hello!", LabelStyle(style)}.release();
        }},
    {"setters",
        [](UserInterface& ui, Int style, Int counter) {
            Label label{{ui, {}, {52, 36}}, "Hey", LabelStyle(style)};
            label.setText(counter % 3 ? "Bye" : "Hello!");
            return label.release();
        }},
    {"setters from empty",
        [](UserInterface& ui, Int style, Int counter) {
            Label label{{ui, {}, {52, 36}}, "", LabelStyle(style)};
            label.setText(counter % 3 ? "Bye" : "Hello!");
            return label.release();
        }},
    {"setStyle()",
        [](UserInterface& ui, Int style, Int counter) {
            Label label{{ui, {}, {52, 36}}, counter % 3 ? "Bye" : "Hello!", LabelStyle(style == 0 ? 1 : 0)};
            label.setStyle(LabelStyle(style));
            return label.release();
        }},
    {"setStyle() on empty, setters",
        [](UserInterface& ui, Int style, Int counter) {
            Label label{{ui, {}, {52, 36}}, "", LabelStyle(style == 0 ? 1 : 0)};
            label.setStyle(LabelStyle(style));
            label.setText(counter % 3 ? "Bye" : "Hello!");
            return label.release();
        }},
};

const struct {
    const char* name;
    NodeHandle(*create)(UserInterface& ui, Int style, Int counter);
} IconData[]{
    {"stateless",
        [](UserInterface& ui, Int style, Int counter) {
            /** @todo differently wide icons to test alignment */
            return label({ui, {}, {48, 36}}, counter % 3 ? Icon::Yes : Icon::No, LabelStyle(style)).node();
        }},
    {nullptr,
        [](UserInterface& ui, Int style, Int counter) {
            return Label{{ui, {}, {48, 36}}, counter % 3 ? Icon::Yes : Icon::No, LabelStyle(style)}.release();
        }},
    {"setters",
        [](UserInterface& ui, Int style, Int counter) {
            Label label{{ui, {}, {48, 36}}, Icon::Yes, LabelStyle(style)};
            label.setIcon(counter % 3 ? Icon::Yes : Icon::No);
            return label.release();
        }},
    {"setters on empty",
        [](UserInterface& ui, Int style, Int counter) {
            Label label{{ui, {}, {48, 36}}, Icon::None, LabelStyle(style)};
            label.setIcon(counter % 3 ? Icon::Yes : Icon::No);
            return label.release();
        }},
    {"setStyle()",
        [](UserInterface& ui, Int style, Int counter) {
            Label label{{ui, {}, {48, 36}}, counter % 3 ? Icon::Yes : Icon::No, LabelStyle(style == 0 ? 1 : 0)};
            label.setStyle(LabelStyle(style));
            return label.release();
        }},
    {"setStyle() on empty, setters",
        [](UserInterface& ui, Int style, Int counter) {
            Label label{{ui, {}, {48, 36}}, Icon::None, LabelStyle(style == 0 ? 1 : 0)};
            label.setStyle(LabelStyle(style));
            label.setIcon(counter % 3 ? Icon::Yes : Icon::No);
            return label.release();
        }},
};

LabelGLTest::LabelGLTest(): StyleGLTester{StyleData} {
    addInstancedTests({&LabelGLTest::text},
        Containers::arraySize(TextData)*styleCount());

    addInstancedTests({&LabelGLTest::icon},
        Containers::arraySize(IconData)*styleCount());
}

void LabelGLTest::text() {
    auto&& data = TextData[testCaseInstanceId()/styleCount()];
    auto&& styleData = StyleData[testCaseInstanceId()%styleCount()];
    if(!data.name)
        setTestCaseDescription(styleData.name);
    else
        setTestCaseDescription(Utility::format("{}, {}", data.name, styleData.name));

    CORRADE_VERIFY(true); /* Capture correct function name */

    StyleGLTester::render(data.create, styleData, "label-text.png",
        Flag::Disabled, 7, {}, 2.0f, 0.0248f);
}

void LabelGLTest::icon() {
    auto&& data = IconData[testCaseInstanceId()/styleCount()];
    auto&& styleData = StyleData[testCaseInstanceId()%styleCount()];
    if(!data.name)
        setTestCaseDescription(styleData.name);
    else
        setTestCaseDescription(Utility::format("{}, {}", data.name, styleData.name));

    CORRADE_VERIFY(true); /* Capture correct function name */

    StyleGLTester::render(data.create, styleData, "label-icon.png",
        Flag::Disabled, 7, {}, 1.75f, 0.0099f);
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::LabelGLTest)
