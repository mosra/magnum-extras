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

#include "Magnum/Ui/Button.h"
#include "Magnum/Ui/Style.h"
#include "Magnum/Ui/Test/StyleGLTester.hpp"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct ButtonGLTest: StyleGLTester {
    explicit ButtonGLTest();

    void textIcon();
    void text();
    void icon();
};

using namespace Math::Literals;

const Style StyleData[]{
    {"m.css dark", "mcss-dark-", false, Containers::pointer<McssDarkStyle>()},
    {"m.css dark SubdividedQuads", "mcss-dark-", false, []{
        Containers::Pointer<McssDarkStyle> style{InPlaceInit};
        style->setBaseLayerFlags(BaseLayerSharedFlag::SubdividedQuads, {});
        return style;
    }()},
    /* No EssentialAnimations as those aren't affecting Button in any way */
    {"m.css dark, Animations", "mcss-dark-", true, Containers::pointer<McssDarkStyle>(McssDarkStyle::Feature::Animations)},
};

const struct {
    const char* name;
    NodeHandle(*create)(UserInterface&, Int, Int);
} TextIconData[]{
    {"stateless",
        [](UserInterface& ui, Int style, Int counter) {
            /** @todo differently wide icons to test alignment */
            return button({ui, {}, {96, 36}}, counter % 2 ? Icon::No : Icon::Yes, counter % 2 ? "Bye" : "Hello!", ButtonStyle(style)).node();
        }},
    {nullptr,
        [](UserInterface& ui, Int style, Int counter) {
            return Button{{ui, {}, {96, 36}}, counter % 2 ? Icon::No : Icon::Yes, counter % 2 ? "Bye" : "Hello!", ButtonStyle(style)}.release();
        }},
    {"setters",
        [](UserInterface& ui, Int style, Int counter) {
            Button button{{ui, {}, {96, 36}}, Icon::No, "Hey", ButtonStyle(style)};
            button.setIcon(counter % 2 ? Icon::No : Icon::Yes);
            button.setText(counter % 2 ? "Bye" : "Hello!");
            return button.release();
        }},
    {"setters on empty",
        [](UserInterface& ui, Int style, Int counter) {
            Button button{{ui, {}, {96, 36}}, Icon::None, "", ButtonStyle(style)};
            button.setIcon(counter % 2 ? Icon::No : Icon::Yes);
            button.setText(counter % 2 ? "Bye" : "Hello!");
            return button.release();
        }},
    {"setters on empty, different order",
        [](UserInterface& ui, Int style, Int counter) {
            Button button{{ui, {}, {96, 36}}, Icon::None, "", ButtonStyle(style)};
            button.setText(counter % 2 ? "Bye" : "Hello!");
            button.setIcon(counter % 2 ? Icon::No : Icon::Yes);
            return button.release();
        }},
    {"setStyle()",
        [](UserInterface& ui, Int style, Int counter) {
            Button button{{ui, {}, {96, 36}}, counter % 2 ? Icon::No : Icon::Yes, counter % 2 ? "Bye" : "Hello!", ButtonStyle(style == 0 ? 1 : 0)};
            button.setStyle(ButtonStyle(style));
            return button.release();
        }},
    {"setStyle() on empty, setters",
        [](UserInterface& ui, Int style, Int counter) {
            Button button{{ui, {}, {96, 36}}, Icon::None, "", ButtonStyle(style == 0 ? 1 : 0)};
            button.setStyle(ButtonStyle(style));
            button.setIcon(counter % 2 ? Icon::No : Icon::Yes);
            button.setText(counter % 2 ? "Bye" : "Hello!");
            return button.release();
        }},
};

const struct {
    const char* name;
    NodeHandle(*create)(UserInterface&, Int, Int);
} TextData[]{
    {"stateless",
        [](UserInterface& ui, Int style, Int counter) {
            return button({ui, {}, {64, 36}}, counter % 2 ? "Bye" : "Hello!", ButtonStyle(style)).node();
        }},
    {nullptr,
        [](UserInterface& ui, Int style, Int counter) {
            return Button{{ui, {}, {64, 36}}, counter % 2 ? "Bye" : "Hello!", ButtonStyle(style)}.release();
        }},
    {"setters",
        [](UserInterface& ui, Int style, Int counter) {
            Button button{{ui, {}, {64, 36}}, "Hey", ButtonStyle(style)};
            button.setText(counter % 2 ? "Bye" : "Hello!");
            return button.release();
        }},
    {"setters on empty",
        [](UserInterface& ui, Int style, Int counter) {
            Button button{{ui, {}, {64, 36}}, "", ButtonStyle(style)};
            button.setText(counter % 2 ? "Bye" : "Hello!");
            return button.release();
        }},
    {"setStyle()",
        [](UserInterface& ui, Int style, Int counter) {
            Button button{{ui, {}, {64, 36}}, counter % 2 ? "Bye" : "Hello!", ButtonStyle(style == 0 ? 1 : 0)};
            button.setStyle(ButtonStyle(style));
            return button.release();
        }},
    {"setStyle() on empty, setters",
        [](UserInterface& ui, Int style, Int counter) {
            Button button{{ui, {}, {64, 36}}, "", ButtonStyle(style == 0 ? 1 : 0)};
            button.setStyle(ButtonStyle(style));
            button.setText(counter % 2 ? "Bye" : "Hello!");
            return button.release();
        }},
};

const struct {
    const char* name;
    NodeHandle(*create)(UserInterface& ui, Int style, Int counter);
} IconData[]{
    {"stateless",
        [](UserInterface& ui, Int style, Int counter) {
            /** @todo differently wide icons to test alignment */
            return button({ui, {}, {48, 36}}, counter % 2 ? Icon::Yes : Icon::No, ButtonStyle(style)).node();
        }},
    {nullptr,
        [](UserInterface& ui, Int style, Int counter) {
            return Button{{ui, {}, {48, 36}}, counter % 2 ? Icon::Yes : Icon::No, ButtonStyle(style)}.release();
        }},
    {"setters",
        [](UserInterface& ui, Int style, Int counter) {
            Button button{{ui, {}, {48, 36}}, Icon::Yes, ButtonStyle(style)};
            button.setIcon(counter % 2 ? Icon::Yes : Icon::No);
            return button.release();
        }},
    {"setters on empty",
        [](UserInterface& ui, Int style, Int counter) {
            Button button{{ui, {}, {48, 36}}, Icon::None, ButtonStyle(style)};
            button.setIcon(counter % 2 ? Icon::Yes : Icon::No);
            return button.release();
        }},
    {"setStyle()",
        [](UserInterface& ui, Int style, Int counter) {
            Button button{{ui, {}, {48, 36}}, counter % 2 ? Icon::Yes : Icon::No, ButtonStyle(style == 0 ? 1 : 0)};
            button.setStyle(ButtonStyle(style));
            return button.release();
        }},
    {"setStyle() on empty, setters",
        [](UserInterface& ui, Int style, Int counter) {
            Button button{{ui, {}, {48, 36}}, Icon::None, ButtonStyle(style == 0 ? 1 : 0)};
            button.setStyle(ButtonStyle(style));
            button.setIcon(counter % 2 ? Icon::Yes : Icon::No);
            return button.release();
        }},
};

ButtonGLTest::ButtonGLTest(): StyleGLTester{StyleData} {
    addInstancedTests({&ButtonGLTest::textIcon},
        Containers::arraySize(TextIconData)*styleCount());

    addInstancedTests({&ButtonGLTest::text},
        Containers::arraySize(TextData)*styleCount());

    addInstancedTests({&ButtonGLTest::icon},
        Containers::arraySize(IconData)*styleCount());
}

void ButtonGLTest::textIcon() {
    auto&& data = TextIconData[testCaseInstanceId()/styleCount()];
    auto&& styleData = StyleData[testCaseInstanceId()%styleCount()];
    if(!data.name)
        setTestCaseDescription(styleData.name);
    else
        setTestCaseDescription(Utility::format("{}, {}", data.name, styleData.name));

    CORRADE_VERIFY(true); /* Capture correct function name */

    StyleGLTester::render(data.create, styleData, "button-text-icon.png",
        Flag::HoveredPressed|Flag::Disabled|Flag::XfailLlvmpipe20,
        /* Button fade out animations are all 0.5 sec */
        8, 0.5_sec, 2.0f, 0.0399f);
}

void ButtonGLTest::text() {
    auto&& data = TextData[testCaseInstanceId()/styleCount()];
    auto&& styleData = StyleData[testCaseInstanceId()%styleCount()];
    if(!data.name)
        setTestCaseDescription(styleData.name);
    else
        setTestCaseDescription(Utility::format("{}, {}", data.name, styleData.name));

    CORRADE_VERIFY(true); /* Capture correct function name */

    StyleGLTester::render(data.create, styleData, "button-text.png",
        Flag::HoveredPressed|Flag::Disabled|Flag::XfailLlvmpipe20,
        /* Button fade out animations are all 0.5 sec */
        8, 0.5_sec, 2.0f, 0.0386f);
}

void ButtonGLTest::icon() {
    auto&& data = IconData[testCaseInstanceId()/styleCount()];
    auto&& styleData = StyleData[testCaseInstanceId()%styleCount()];
    if(!data.name)
        setTestCaseDescription(styleData.name);
    else
        setTestCaseDescription(Utility::format("{}, {}", data.name, styleData.name));

    CORRADE_VERIFY(true); /* Capture correct function name */

    StyleGLTester::render(data.create, styleData, "button-icon.png",
        Flag::HoveredPressed|Flag::Disabled|Flag::XfailLlvmpipe20,
        /* Button fade out animations are all 0.5 sec */
        8, 0.5_sec, 1.25f, 0.0278f);
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::ButtonGLTest)
