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

#include <Magnum/GL/OpenGLTester.h>
#include <Magnum/Math/Vector2.h>

#include "Magnum/Ui/AbstractStyle.h"
#include "Magnum/Ui/Application.h"
#include "Magnum/Ui/UserInterfaceGL.h"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct ApplicationGLTest: GL::OpenGLTester {
    explicit ApplicationGLTest();

    void construct();
    void create();
};

struct CustomApplication {
    explicit CustomApplication(const Vector2i& windowSize, const Vector2i& framebufferSize, const Vector2& dpiScaling): _windowSize{windowSize}, _framebufferSize{framebufferSize}, _dpiScaling{dpiScaling} {}

    Vector2i windowSize() const { return _windowSize; }
    Vector2i framebufferSize() const { return _framebufferSize; }
    Vector2 dpiScaling() const { return _dpiScaling; }

    private:
        Vector2i _windowSize;
        Vector2i _framebufferSize;
        Vector2 _dpiScaling;
};

const struct {
    const char* name;
    bool styleSubset;
} ConstructData[]{
    {"", false},
    {"style subset", true},
};

const struct {
    const char* name;
    bool tryCreate;
    bool styleSubset;
} CreateData[]{
    {"", false, false},
    {"style subset", false, true},
    {"try", true, false},
    {"try, style subset", true, true},
};

ApplicationGLTest::ApplicationGLTest() {
    addInstancedTests({&ApplicationGLTest::construct},
        Containers::arraySize(ConstructData));

    addInstancedTests({&ApplicationGLTest::create},
        Containers::arraySize(CreateData));
}

void ApplicationGLTest::construct() {
    auto&& data = ConstructData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    const CustomApplication application{{100, 200}, {300, 400}, {1.25f, 1.33333333f}};
    struct: AbstractStyle {
        StyleFeatures doFeatures() const override {
            return StyleFeature::EventLayer;
        }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override {
            return true;
        }
    } style;

    Containers::Optional<UserInterfaceGL> ui;
    if(data.styleSubset)
        ui.emplace(application, style, StyleFeature::EventLayer);
    else
        ui.emplace(application, style);
    CORRADE_COMPARE(ui->size(), (Vector2{80.0f, 150.0f}));
    CORRADE_COMPARE(ui->windowSize(), (Vector2{100.0f, 200.0f}));
    CORRADE_COMPARE(ui->framebufferSize(), (Vector2i{300, 400}));
}

void ApplicationGLTest::create() {
    auto&& data = CreateData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    const CustomApplication application{{100, 200}, {300, 400}, {1.25f, 1.33333333f}};
    struct: AbstractStyle {
        StyleFeatures doFeatures() const override {
            return StyleFeature::EventLayer;
        }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override {
            return true;
        }
    } style;

    UserInterfaceGL ui{NoCreate};
    if(data.tryCreate) {
        if(data.styleSubset)
            ui.tryCreate(application, style, StyleFeature::EventLayer);
        else
            ui.tryCreate(application, style);
    } else {
        if(data.styleSubset)
            ui.create(application, style, StyleFeature::EventLayer);
        else
            ui.create(application, style);
    }
    CORRADE_COMPARE(ui.size(), (Vector2{80.0f, 150.0f}));
    CORRADE_COMPARE(ui.windowSize(), (Vector2{100.0f, 200.0f}));
    CORRADE_COMPARE(ui.framebufferSize(), (Vector2i{300, 400}));
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::ApplicationGLTest)
