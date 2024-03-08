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
#include <Corrade/PluginManager/Manager.h>
#include <Corrade/Utility/DebugStl.h> /** @todo remove once Debug is stream-free */
#include <Magnum/PixelFormat.h>
#include <Magnum/GL/OpenGLTester.h>
#include <Magnum/Text/AbstractFont.h>
#include <Magnum/Text/AbstractGlyphCache.h>
#include <Magnum/Trade/AbstractImporter.h>

#include "Magnum/Whee/AbstractStyle.h"
#include "Magnum/Whee/BaseLayerGL.h"
#include "Magnum/Whee/EventLayer.h"
#include "Magnum/Whee/RendererGL.h"
#include "Magnum/Whee/TextLayerGL.h"
#include "Magnum/Whee/UserInterfaceGL.h"

namespace Magnum { namespace Whee { namespace Test { namespace {

struct UserInterfaceGLTest: GL::OpenGLTester {
    explicit UserInterfaceGLTest();

    void construct();
    /* All NoCreate variants tested in UserInterfaceGL_Test to verify they work
       without a GL context */
    void constructSingleSize();
    void constructCopy();
    void constructMove();

    void setStyle();
    void setStyleRendererAlreadyPresent();
    void setStyleNoFeatures();
    void setStyleFeaturesNotSupported();
    void setStyleBaseLayerAlreadyPresent();
    void setStyleTextLayerAlreadyPresent();
    void setStyleTextLayerArrayGlyphCache();
    void setStyleTextLayerImagesTextLayerNotPresentNotApplied();
    void setStyleEventLayerAlreadyPresent();

    private:
        PluginManager::Manager<Trade::AbstractImporter> _importerManager;
        PluginManager::Manager<Text::AbstractFont> _fontManager;
};

const struct {
    const char* name;
    StyleFeatures expectedFeatures;
    StyleFeatures supportedFeatures;
    bool succeed;
    UnsignedInt expectedLayerCount;
    Containers::Array<StyleFeatures> features;
} SetStyleData[]{
    {"base layer only", StyleFeature::BaseLayer, StyleFeature::BaseLayer, true, 1, {InPlaceInit, {
        StyleFeature::BaseLayer
    }}},
    {"base layer only, everything supported", StyleFeature::BaseLayer, StyleFeature::BaseLayer|StyleFeature::TextLayer|StyleFeature::EventLayer, true, 1, {InPlaceInit, {
        StyleFeature::BaseLayer
    }}},
    {"text layer only", StyleFeature::TextLayer, StyleFeature::TextLayer, true, 1, {InPlaceInit, {
        StyleFeature::TextLayer
    }}},
    {"text layer only, everything supported", StyleFeature::TextLayer, StyleFeature::BaseLayer|StyleFeature::TextLayer|StyleFeature::EventLayer, true, 1, {InPlaceInit, {
        StyleFeature::TextLayer
    }}},
    {"text layer + images only", StyleFeature::TextLayer|StyleFeature::TextLayerImages, StyleFeature::TextLayer|StyleFeature::TextLayerImages, true, 1, {InPlaceInit, {
        StyleFeature::TextLayer|StyleFeature::TextLayerImages
    }}},
    {"text layer + images, applied gradually", StyleFeature::TextLayer|StyleFeature::TextLayerImages, StyleFeature::TextLayer|StyleFeature::TextLayerImages, true, 1, {InPlaceInit, {
        StyleFeature::TextLayer,
        StyleFeature::TextLayerImages
    }}},
    {"text layer + images only, everything supported", StyleFeature::TextLayer|StyleFeature::TextLayerImages, StyleFeature::BaseLayer|StyleFeature::TextLayer|StyleFeature::TextLayerImages|StyleFeature::EventLayer, true, 1, {InPlaceInit, {
        StyleFeature::TextLayer|StyleFeature::TextLayerImages
    }}},
    {"event layer only", StyleFeature::EventLayer, StyleFeature::EventLayer, true, 1, {InPlaceInit, {
        StyleFeature::EventLayer
    }}},
    {"event layer only, everything supported", StyleFeature::EventLayer, StyleFeature::BaseLayer|StyleFeature::TextLayer|StyleFeature::EventLayer, true, 1, {InPlaceInit, {
        StyleFeature::EventLayer
    }}},
    {"everything except base layer", StyleFeature::TextLayer|StyleFeature::TextLayerImages|StyleFeature::EventLayer, StyleFeature::TextLayer|StyleFeature::TextLayerImages|StyleFeature::EventLayer, true, 2, {InPlaceInit, {
        ~StyleFeature::BaseLayer
    }}},
    {"everything except base layer, applied gradually", StyleFeature::TextLayer|StyleFeature::TextLayerImages|StyleFeature::EventLayer, StyleFeature::TextLayer|StyleFeature::TextLayerImages|StyleFeature::EventLayer, true, 2, {InPlaceInit, {
        StyleFeature::TextLayer,
        StyleFeature::TextLayerImages,
        StyleFeature::EventLayer,
    }}},
    {"everything except base layer, everything supported", StyleFeature::TextLayer|StyleFeature::TextLayerImages|StyleFeature::EventLayer, StyleFeature::BaseLayer|StyleFeature::TextLayer|StyleFeature::TextLayerImages|StyleFeature::EventLayer, true, 2, {InPlaceInit, {
        ~StyleFeature::BaseLayer
    }}},
    {"everything", StyleFeature::BaseLayer|StyleFeature::TextLayer|StyleFeature::TextLayerImages|StyleFeature::EventLayer, StyleFeature::BaseLayer|StyleFeature::TextLayer|StyleFeature::TextLayerImages|StyleFeature::EventLayer, true, 3, {InPlaceInit, {
        ~StyleFeatures{}
    }}},
    {"everything, applied gradually", StyleFeature::BaseLayer|StyleFeature::TextLayer|StyleFeature::TextLayerImages|StyleFeature::EventLayer, StyleFeature::BaseLayer|StyleFeature::TextLayer|StyleFeature::TextLayerImages|StyleFeature::EventLayer, true, 3, {InPlaceInit, {
        StyleFeature::TextLayer,
        StyleFeature::TextLayerImages,
        StyleFeature::EventLayer,
        StyleFeature::BaseLayer,
    }}},
    {"application failed", StyleFeature::BaseLayer|StyleFeature::EventLayer, StyleFeature::BaseLayer|StyleFeature::EventLayer, false, 2, {InPlaceInit, {
        StyleFeature::BaseLayer|StyleFeature::EventLayer
    }}},
    {"everything, implicitly", StyleFeature::BaseLayer|StyleFeature::TextLayer|StyleFeature::TextLayerImages|StyleFeature::EventLayer, StyleFeature::BaseLayer|StyleFeature::TextLayer|StyleFeature::TextLayerImages|StyleFeature::EventLayer, true, 3, {}},
    {"everything, implicitly, application failed", StyleFeature::BaseLayer|StyleFeature::TextLayer|StyleFeature::TextLayerImages|StyleFeature::EventLayer, StyleFeature::BaseLayer|StyleFeature::TextLayer|StyleFeature::TextLayerImages|StyleFeature::EventLayer, false, 3, {}},
    {"everything, implicitly, only unknown layer supported", StyleFeatures{0x10}, StyleFeatures{0x10}, true, 0, {}},
    {"everything, implicitly, only base layer supported", StyleFeature::BaseLayer, StyleFeature::BaseLayer, true, 1, {}},
    {"everything, implicitly, everything except text layer supported", StyleFeature::BaseLayer|StyleFeature::EventLayer, StyleFeature::BaseLayer|StyleFeature::EventLayer, true, 2, {}},
};

UserInterfaceGLTest::UserInterfaceGLTest() {
    addTests({&UserInterfaceGLTest::construct,
              &UserInterfaceGLTest::constructSingleSize,
              &UserInterfaceGLTest::constructCopy,
              &UserInterfaceGLTest::constructMove});

    addInstancedTests({&UserInterfaceGLTest::setStyle},
        Containers::arraySize(SetStyleData));

    addTests({&UserInterfaceGLTest::setStyleRendererAlreadyPresent,
              &UserInterfaceGLTest::setStyleNoFeatures,
              &UserInterfaceGLTest::setStyleFeaturesNotSupported,
              &UserInterfaceGLTest::setStyleBaseLayerAlreadyPresent,
              &UserInterfaceGLTest::setStyleTextLayerAlreadyPresent,
              &UserInterfaceGLTest::setStyleTextLayerArrayGlyphCache,
              &UserInterfaceGLTest::setStyleTextLayerImagesTextLayerNotPresentNotApplied,
              &UserInterfaceGLTest::setStyleEventLayerAlreadyPresent});
}

void UserInterfaceGLTest::construct() {
    Int applyCalled = 0;
    struct Style: AbstractStyle {
        explicit Style(Int& applyCalled): applyCalled(applyCalled) {}
        StyleFeatures doFeatures() const override { return StyleFeatures{0x10}; }
        bool doApply(UserInterface&, StyleFeatures features, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_COMPARE(features, StyleFeatures{0x10});
            ++applyCalled;
            return true;
        }

        Int& applyCalled;
    } style{applyCalled};

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    UserInterfaceGL ui{{100.0f, 150.0f}, {50.0f, 75.0f}, {200, 300}, style, &_importerManager, &_fontManager};
    CORRADE_COMPARE(ui.size(), (Vector2{100.0f, 150.0f}));
    CORRADE_COMPARE(ui.windowSize(), (Vector2{50.0f, 75.0f}));
    CORRADE_COMPARE(ui.framebufferSize(), (Vector2i{200, 300}));
    CORRADE_COMPARE(ui.layerCapacity(), 0);
    CORRADE_COMPARE(ui.layerUsedCount(), 0);
    CORRADE_VERIFY(!ui.hasBaseLayer());
    CORRADE_VERIFY(!ui.hasTextLayer());
    CORRADE_VERIFY(!ui.hasEventLayer());
    CORRADE_COMPARE(applyCalled, 1);

    /* The renderer instance is set implicitly first time a style is */
    CORRADE_VERIFY(ui.hasRenderer());
    CORRADE_COMPARE(ui.renderer().currentTargetState(), RendererTargetState::Initial);
    /* const overload */
    const UserInterfaceGL& cui = ui;
    CORRADE_COMPARE(cui.renderer().currentTargetState(), RendererTargetState::Initial);
}

void UserInterfaceGLTest::constructSingleSize() {
    Int applyCalled = 0;
    struct Style: AbstractStyle {
        explicit Style(Int& applyCalled): applyCalled(applyCalled) {}
        StyleFeatures doFeatures() const override { return StyleFeatures{0x10}; }
        bool doApply(UserInterface&, StyleFeatures features, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_COMPARE(features, StyleFeatures{0x10});
            ++applyCalled;
            return true;
        }

        Int& applyCalled;
    } style{applyCalled};

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    UserInterfaceGL ui{{200, 300}, style, &_importerManager, &_fontManager};
    CORRADE_COMPARE(ui.size(), (Vector2{200.0f, 300.0f}));
    CORRADE_COMPARE(ui.windowSize(), (Vector2{200.0f, 300.0f}));
    CORRADE_COMPARE(ui.framebufferSize(), (Vector2i{200, 300}));
    CORRADE_COMPARE(ui.layerCapacity(), 0);
    CORRADE_COMPARE(ui.layerUsedCount(), 0);
    CORRADE_VERIFY(!ui.hasBaseLayer());
    CORRADE_VERIFY(!ui.hasTextLayer());
    CORRADE_VERIFY(!ui.hasEventLayer());
    CORRADE_COMPARE(applyCalled, 1);

    /* The renderer instance is set implicitly first time a style is */
    CORRADE_VERIFY(ui.hasRenderer());
    CORRADE_COMPARE(ui.renderer().currentTargetState(), RendererTargetState::Initial);
    /* const overload */
    const UserInterfaceGL& cui = ui;
    CORRADE_COMPARE(cui.renderer().currentTargetState(), RendererTargetState::Initial);
}

void UserInterfaceGLTest::constructCopy() {
    CORRADE_VERIFY(!std::is_copy_constructible<UserInterfaceGL>{});
    CORRADE_VERIFY(!std::is_copy_assignable<UserInterfaceGL>{});
}

void UserInterfaceGLTest::constructMove() {
    struct: AbstractStyle {
        StyleFeatures doFeatures() const override { return StyleFeatures{0x10}; }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override { return true; }
    } style;

    UserInterfaceGL a{{200, 300}, style, &_importerManager, &_fontManager};
    a.setEventLayerInstance(Containers::pointer<EventLayer>(a.createLayer()));

    UserInterfaceGL b{Utility::move(a)};
    CORRADE_COMPARE(b.size(), (Vector2{200.0f, 300.0f}));
    CORRADE_VERIFY(b.hasEventLayer());

    UserInterfaceGL c{{10, 10}, style, &_importerManager, &_fontManager};
    c = Utility::move(b);
    CORRADE_COMPARE(c.size(), (Vector2{200.0f, 300.0f}));
    CORRADE_VERIFY(c.hasEventLayer());

    CORRADE_VERIFY(std::is_nothrow_move_constructible<UserInterfaceGL>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<UserInterfaceGL>::value);
}

void UserInterfaceGLTest::setStyle() {
    auto&& data = SetStyleData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    Int applyCalled = 0;
    StyleFeatures glyphCacheSizeQueriedFeatures, actualFeatures;
    struct Style: AbstractStyle {
        Style(Int& applyCalled, StyleFeatures& glyphCacheSizeQueriedFeatures, StyleFeatures& actualFeatures, StyleFeatures supportedFeatures, bool succeed): _applyCalled(applyCalled), _glyphCacheSizeQueriedFeatures(glyphCacheSizeQueriedFeatures), _actualFeatures(actualFeatures), _supportedFeatures{supportedFeatures}, _succeed{succeed} {}

        StyleFeatures doFeatures() const override {
            return _supportedFeatures;
        }
        UnsignedInt doBaseLayerStyleUniformCount() const override { return 3; }
        UnsignedInt doBaseLayerStyleCount() const override { return 5; }
        UnsignedInt doBaseLayerDynamicStyleCount() const override { return 11; }
        UnsignedInt doTextLayerStyleUniformCount() const override { return 2; }
        UnsignedInt doTextLayerStyleCount() const override { return 4; }
        UnsignedInt doTextLayerDynamicStyleCount() const override { return 13; }
        PixelFormat doTextLayerGlyphCacheFormat() const override { return PixelFormat::R16F; }
        /** @todo test the array size once supported */
        Vector3i doTextLayerGlyphCacheSize(StyleFeatures features) const override {
            _glyphCacheSizeQueriedFeatures = features;
            return {16, 24, 1};
        }
        Vector2i doTextLayerGlyphCachePadding() const override { return {3, 1}; }
        bool doApply(UserInterface&, StyleFeatures features, PluginManager::Manager<Trade::AbstractImporter>* importerManager, PluginManager::Manager<Text::AbstractFont>* fontManager) const override {
            /* The features passed to this function and to the
               doTextLayerGlyphCacheSize() query, if called, should match */
            if(_glyphCacheSizeQueriedFeatures)
                CORRADE_COMPARE(features, _glyphCacheSizeQueriedFeatures);
            _glyphCacheSizeQueriedFeatures = {};

            _actualFeatures |= features;
            if(features >= StyleFeature::TextLayer)
                CORRADE_VERIFY(fontManager);
            if(features >= StyleFeature::TextLayerImages)
                CORRADE_VERIFY(importerManager);
            ++_applyCalled;
            return _succeed;
        }

        Int& _applyCalled;
        StyleFeatures& _glyphCacheSizeQueriedFeatures;
        StyleFeatures& _actualFeatures;
        StyleFeatures _supportedFeatures;
        bool _succeed;
    } style{applyCalled, glyphCacheSizeQueriedFeatures, actualFeatures, data.supportedFeatures, data.succeed};

    UserInterfaceGL ui{NoCreate, {200, 300}};
    CORRADE_VERIFY(!ui.hasRenderer());
    CORRADE_COMPARE(ui.layerUsedCount(), 0);

    /** @todo once FreeTypeFont is fixed to work with multiple plugin managers,
        test also a variant with the manager not passed */
    if(data.features.isEmpty())
        CORRADE_COMPARE(ui.trySetStyle(style, &_importerManager, &_fontManager), data.succeed);
    else for(StyleFeatures features: data.features)
        CORRADE_COMPARE(ui.trySetStyle(style, features, features >= StyleFeature::TextLayerImages ? &_importerManager : nullptr, features >= StyleFeature::TextLayer ? &_fontManager : nullptr), data.succeed);
    CORRADE_COMPARE(ui.layerUsedCount(), data.expectedLayerCount);
    CORRADE_COMPARE(applyCalled, data.features.isEmpty() ? 1 : data.features.size());
    CORRADE_COMPARE(actualFeatures, data.expectedFeatures);

    /* The renderer instance is set implicitly first time a style is, and only
       if not already */
    CORRADE_VERIFY(ui.hasRenderer());

    if(data.expectedFeatures & StyleFeature::BaseLayer) {
        CORRADE_VERIFY(ui.hasBaseLayer());
        CORRADE_COMPARE(ui.baseLayer().shared().styleUniformCount(), 3);
        CORRADE_COMPARE(ui.baseLayer().shared().styleCount(), 5);
        CORRADE_COMPARE(ui.baseLayer().shared().dynamicStyleCount(), 11);
    }

    if(data.expectedFeatures & StyleFeature::TextLayer) {
        CORRADE_VERIFY(ui.hasTextLayer());
        CORRADE_COMPARE(ui.textLayer().shared().styleUniformCount(), 2);
        CORRADE_COMPARE(ui.textLayer().shared().styleCount(), 4);
        CORRADE_COMPARE(ui.textLayer().shared().dynamicStyleCount(), 13);

        CORRADE_VERIFY(ui.textLayer().shared().hasGlyphCache());
        CORRADE_COMPARE(ui.textLayer().shared().glyphCache().format(), PixelFormat::R16F);
        /** @todo test the array size once supported */
        CORRADE_COMPARE(ui.textLayer().shared().glyphCache().size(), (Vector3i{16, 24, 1}));
        CORRADE_COMPARE(ui.textLayer().shared().glyphCache().padding(), (Vector2i{3, 1}));
    }

    if(data.expectedFeatures & StyleFeature::EventLayer) {
        CORRADE_VERIFY(ui.hasEventLayer());
    }
}

void UserInterfaceGLTest::setStyleRendererAlreadyPresent() {
    UserInterfaceGL ui{NoCreate, {200, 300}};
    CORRADE_VERIFY(!ui.hasRenderer());

    ui.setRendererInstance(Containers::pointer<RendererGL>());
    CORRADE_VERIFY(ui.hasRenderer());

    struct: AbstractStyle {
        StyleFeatures doFeatures() const override { return StyleFeatures{0x10}; }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override { return true; }
    } style;

    /* Setting a style shouldn't attempt to set a renderer instance again if
       it's already there */
    ui.setStyle(style);
    CORRADE_VERIFY(ui.hasRenderer());
}

void UserInterfaceGLTest::setStyleNoFeatures() {
    CORRADE_SKIP_IF_NO_ASSERT();

    UserInterfaceGL ui{NoCreate, {200, 300}};

    struct: AbstractStyle {
        StyleFeatures doFeatures() const override { return StyleFeature::BaseLayer; }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } style;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    std::ostringstream out;
    Error redirectError{&out};
    ui.trySetStyle(style, {}, &_importerManager, &_fontManager);
    CORRADE_COMPARE(out.str(), "Whee::UserInterfaceGL::trySetStyle(): no features specified\n");
}

void UserInterfaceGLTest::setStyleFeaturesNotSupported() {
    CORRADE_SKIP_IF_NO_ASSERT();

    UserInterfaceGL ui{NoCreate, {200, 300}};

    struct: AbstractStyle {
        StyleFeatures doFeatures() const override { return StyleFeature::BaseLayer; }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } style;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    std::ostringstream out;
    Error redirectError{&out};
    ui.trySetStyle(style, StyleFeature::BaseLayer|StyleFeature::TextLayer, &_importerManager, &_fontManager);
    CORRADE_COMPARE(out.str(), "Whee::UserInterfaceGL::trySetStyle(): Whee::StyleFeature::BaseLayer|Whee::StyleFeature::TextLayer not a subset of supported Whee::StyleFeature::BaseLayer\n");
}

void UserInterfaceGLTest::setStyleBaseLayerAlreadyPresent() {
    BaseLayerGL::Shared shared{BaseLayer::Shared::Configuration{1}};
    UserInterfaceGL ui{NoCreate, {200, 300}};
    ui.setBaseLayerInstance(Containers::pointer<BaseLayerGL>(ui.createLayer(), shared));

    struct: AbstractStyle {
        StyleFeatures doFeatures() const override { return StyleFeature::BaseLayer; }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } style;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    std::ostringstream out;
    Error redirectError{&out};
    ui.trySetStyle(style, &_importerManager, &_fontManager);
    CORRADE_COMPARE(out.str(), "Whee::UserInterfaceGL::trySetStyle(): base layer already present\n");
}

void UserInterfaceGLTest::setStyleTextLayerAlreadyPresent() {
    TextLayerGL::Shared shared{TextLayer::Shared::Configuration{1}};
    UserInterfaceGL ui{NoCreate, {200, 300}};
    ui.setTextLayerInstance(Containers::pointer<TextLayerGL>(ui.createLayer(), shared));

    struct: AbstractStyle {
        StyleFeatures doFeatures() const override { return StyleFeature::TextLayer; }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } style;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    std::ostringstream out;
    Error redirectError{&out};
    ui.trySetStyle(style, &_importerManager, &_fontManager);
    CORRADE_COMPARE(out.str(), "Whee::UserInterfaceGL::trySetStyle(): text layer already present\n");
}

void UserInterfaceGLTest::setStyleTextLayerArrayGlyphCache() {
    UserInterfaceGL ui{NoCreate, {200, 300}};

    struct: AbstractStyle {
        StyleFeatures doFeatures() const override { return StyleFeature::TextLayer; }
        UnsignedInt doTextLayerStyleCount() const override { return 1; }
        Vector3i doTextLayerGlyphCacheSize(StyleFeatures) const override { return {16, 24, 2}; }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } style;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    std::ostringstream out;
    Error redirectError{&out};
    ui.trySetStyle(style, &_importerManager, &_fontManager);
    CORRADE_COMPARE(out.str(), "Whee::UserInterfaceGL::trySetStyle(): only 2D glyph cache is supported at the moment, got a size of {16, 24, 2}\n");
}

void UserInterfaceGLTest::setStyleTextLayerImagesTextLayerNotPresentNotApplied() {
    UserInterfaceGL ui{NoCreate, {200, 300}};

    struct: AbstractStyle {
        StyleFeatures doFeatures() const override { return StyleFeature::TextLayerImages; }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } style;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    std::ostringstream out;
    Error redirectError{&out};
    ui.trySetStyle(style, &_importerManager, &_fontManager);
    CORRADE_COMPARE(out.str(), "Whee::UserInterfaceGL::trySetStyle(): text layer not present and Whee::StyleFeature::TextLayer isn't being applied as well\n");
}

void UserInterfaceGLTest::setStyleEventLayerAlreadyPresent() {
    UserInterfaceGL ui{NoCreate, {200, 300}};
    ui.setEventLayerInstance(Containers::pointer<EventLayer>(ui.createLayer()));

    struct: AbstractStyle {
        StyleFeatures doFeatures() const override { return StyleFeature::EventLayer; }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } style;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    std::ostringstream out;
    Error redirectError{&out};
    ui.trySetStyle(style, &_importerManager, &_fontManager);
    CORRADE_COMPARE(out.str(), "Whee::UserInterfaceGL::trySetStyle(): event layer already present\n");
}

}}}}

CORRADE_TEST_MAIN(Magnum::Whee::Test::UserInterfaceGLTest)
