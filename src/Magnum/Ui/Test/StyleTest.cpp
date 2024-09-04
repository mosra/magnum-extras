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
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Containers/StringIterable.h>
#include <Corrade/PluginManager/Manager.h>
#include <Corrade/PluginManager/PluginMetadata.h>
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/Numeric.h>
#include <Corrade/TestSuite/Compare/String.h>
#include <Corrade/Utility/ConfigurationGroup.h>
#include <Corrade/Utility/DebugStl.h> /** @todo remove once Debug is stream-free */
#include <Magnum/PixelFormat.h>
#include <Magnum/Text/AbstractGlyphCache.h>
#include <Magnum/Text/AbstractFont.h>
#include <Magnum/TextureTools/Atlas.h>
#include <Magnum/Trade/AbstractImporter.h>

#include "Magnum/Ui/BaseLayer.h"
#include "Magnum/Ui/EventLayer.h"
#include "Magnum/Ui/SnapLayouter.h"
#include "Magnum/Ui/Style.h"
#include "Magnum/Ui/Style.hpp"
#include "Magnum/Ui/TextLayer.h"
#include "Magnum/Ui/UserInterface.h"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct StyleTest: TestSuite::Tester {
    explicit StyleTest();

    void debugIcon();

    void baseMcssDark();
    void textMcssDark();
    void textUniformsMcssDark();
    void textEditingMcssDark();

    void apply();
    void applyTextLayerCannotOpenFont();
    void applyTextLayerImagesCannotOpen();
    void applyTextLayerImagesCannotFit();
    void applyTextLayerImagesUnexpectedFormat();
    void applyTextLayerTwice();

    private:
        PluginManager::Manager<Trade::AbstractImporter> _importerManager;
        PluginManager::Manager<Text::AbstractFont> _fontManager;
};

const struct {
    const char* name;
    StyleFeatures features;
    Containers::Optional<UnsignedInt> imageImporterChannelCount;
    Float dpiScaling, expectedFontSize;
} ApplyData[]{
    {"base layer only",
        StyleFeature::BaseLayer, {}, 1.0f, 0.0f},
    {"text layer only",
        StyleFeature::TextLayer, {}, 1.0f, 16.0f*2},
    {"text layer + text layer images",
        StyleFeature::TextLayer|StyleFeature::TextLayerImages, {}, 1.0f, 16.0f*2},
    {"text layer images only",
        StyleFeature::TextLayerImages, {}, 1.0f, 16.0f*2},
    {"text layer images only, imported as RG",
        StyleFeature::TextLayerImages, 2, 1.0f, 16.0f*2},
    {"event layer",
        StyleFeature::EventLayer, {}, 1.0f, 0.0f},
    {"snap layouter",
        StyleFeature::SnapLayouter, {}, 1.0f, 0.0f},
    {"everything",
        StyleFeature::BaseLayer|StyleFeature::TextLayer|StyleFeature::TextLayerImages|StyleFeature::EventLayer, {}, 1.0f, 16.0f*2},
    {"text layer + text layer images, 0.625x DPI scaling",
        StyleFeature::TextLayer|StyleFeature::TextLayerImages, {}, 0.625f, 10.0f*2},
};

StyleTest::StyleTest() {
    addTests({&StyleTest::debugIcon,

              &StyleTest::baseMcssDark,
              &StyleTest::textMcssDark,
              &StyleTest::textUniformsMcssDark,
              &StyleTest::textEditingMcssDark});

    addInstancedTests({&StyleTest::apply},
        Containers::arraySize(ApplyData));

    addTests({&StyleTest::applyTextLayerCannotOpenFont,
              &StyleTest::applyTextLayerImagesCannotOpen,
              &StyleTest::applyTextLayerImagesCannotFit,
              &StyleTest::applyTextLayerImagesUnexpectedFormat,
              &StyleTest::applyTextLayerTwice});
}

using Implementation::BaseStyle;
using Implementation::TextStyle;
using Implementation::TextStyleUniform;
using Implementation::TextEditingStyle;

void StyleTest::debugIcon() {
    std::ostringstream out;
    Debug{&out} << Icon::Yes << Icon(0xdeadcafe);
    CORRADE_COMPARE(out.str(), "Ui::Icon::Yes Ui::Icon(0xdeadcafe)\n");
}

void StyleTest::baseMcssDark() {
    const BaseStyle styleUniforms[]{
        #define _c(style, ...) BaseStyle::style,
        #include "Magnum/Ui/Implementation/baseStyleUniformsMcssDark.h"
        #undef _c
    };

    /* Verify that the harcoded BaseStyleCount matches the actual number of
       entries in the mapping. The same should be a static_assert() in
       Style.cpp already, here it's just a sanity check. */
    CORRADE_COMPARE(Implementation::BaseStyleCount, Containers::arraySize(styleUniforms));
    CORRADE_COMPARE(Implementation::BaseStyleUniformCount, Containers::arraySize(styleUniforms));

    /* This checks that:
        - the mapping contains entries for all enum values (otherwise -Wswitch
          would trigger)
        - none of the values are duplicated (otherwise it'd be a syntax error)
        - and the position of the value in the mapping corresponds to the
          enum value (by checking against the styles[] above) */
    for(UnsignedInt i = 0; i != Implementation::BaseStyleCount; ++i) {
        #ifdef CORRADE_TARGET_GCC
        #pragma GCC diagnostic push
        #pragma GCC diagnostic error "-Wswitch"
        #endif
        switch(BaseStyle(i)) {
            #define _c(style, ...) \
                case BaseStyle::style: \
                    CORRADE_COMPARE(UnsignedInt(styleUniforms[i]), UnsignedInt(BaseStyle::style)); \
                    break;
            #include "Magnum/Ui/Implementation/baseStyleUniformsMcssDark.h"
            #undef _c
        }
        #ifdef CORRADE_TARGET_GCC
        #pragma GCC diagnostic pop
        #endif
    }
}

void StyleTest::textMcssDark() {
    const TextStyle styles[]{
        #define _c(style, suffix, ...) TextStyle::style ## suffix,
        #define _s _c
        #define _e _c
        #include "Magnum/Ui/Implementation/textStyleMcssDark.h"
        #undef _c
        #undef _s
        #undef _e
    };

    /* Verify that the harcoded TextStyleCount matches the actual number of
       entries in the mapping. The same should be a static_assert() in
       Style.cpp already, here it's just a sanity check. */
    CORRADE_COMPARE(Implementation::TextStyleCount, Containers::arraySize(styles));

    /* This checks that:
        - the mapping contains entries for all enum values (otherwise -Wswitch
          would trigger)
        - none of the values are duplicated (otherwise it'd be a syntax error)
        - and the position of the value in the mapping corresponds to the
          enum value (by checking against the styles[] above) */
    for(UnsignedInt i = 0; i != Implementation::TextStyleCount; ++i) {
        #ifdef CORRADE_TARGET_GCC
        #pragma GCC diagnostic push
        #pragma GCC diagnostic error "-Wswitch"
        #endif
        switch(TextStyle(i)) {
            #define _c(style, suffix, ...) \
                case TextStyle::style ## suffix: \
                    CORRADE_COMPARE(UnsignedInt(styles[i]), UnsignedInt(TextStyle::style ## suffix)); \
                    CORRADE_COMPARE_AS(UnsignedInt(TextStyleUniform::style), Implementation::TextStyleUniformCount, \
                        TestSuite::Compare::Less); \
                    break;
            #define _s _c
            #define _e _c
            #include "Magnum/Ui/Implementation/textStyleMcssDark.h"
            #undef _c
            #undef _s
            #undef _e
        }
        #ifdef CORRADE_TARGET_GCC
        #pragma GCC diagnostic pop
        #endif
    }
}

void StyleTest::textUniformsMcssDark() {
    const TextStyleUniform styleUniforms[]{
        #define _c(value, ...) TextStyleUniform::value,
        #include "Magnum/Ui/Implementation/textStyleUniformsMcssDark.h"
        #undef _c
    };

    /* Verify that the harcoded TextStyleUniformCount matches the actual number
       of entries in the mapping. The same should be a static_assert() in
       Style.cpp already, here it's just a sanity check. */
    CORRADE_COMPARE(Implementation::TextStyleUniformCount, Containers::arraySize(styleUniforms));

    /* This checks that:
        - the mapping contains entries for all enum values (otherwise -Wswitch
          would trigger)
        - none of the values are duplicated (otherwise it'd be a syntax error)
        - and the position of the value in the mapping corresponds to the
          enum value (by checking against the styles[] above) */
    for(UnsignedInt i = 0; i != Implementation::TextStyleUniformCount; ++i) {
        #ifdef CORRADE_TARGET_GCC
        #pragma GCC diagnostic push
        #pragma GCC diagnostic error "-Wswitch"
        #endif
        switch(TextStyleUniform(i)) {
            #define _c(style, ...) \
                case TextStyleUniform::style: \
                    CORRADE_COMPARE(UnsignedInt(styleUniforms[i]), UnsignedInt(TextStyleUniform::style)); \
                    break;
            #include "Magnum/Ui/Implementation/textStyleUniformsMcssDark.h"
            #undef _c
        }
        #ifdef CORRADE_TARGET_GCC
        #pragma GCC diagnostic pop
        #endif
    }
}

void StyleTest::textEditingMcssDark() {
    const TextEditingStyle styleUniforms[]{
        #define _c(style, ...) TextEditingStyle::style,
        #define _s _c
        #include "Magnum/Ui/Implementation/textEditingStyleMcssDark.h"
        #undef _s
        #undef _c
    };

    /* Verify that the harcoded BaseStyleCount matches the actual number of
       entries in the mapping. The same should be a static_assert() in
       Style.cpp already, here it's just a sanity check. */
    CORRADE_COMPARE(Implementation::TextEditingStyleCount, Containers::arraySize(styleUniforms));
    CORRADE_COMPARE(Implementation::TextEditingStyleUniformCount, Containers::arraySize(styleUniforms));

    /* This checks that:
        - the mapping contains entries for all enum values (otherwise -Wswitch
          would trigger)
        - none of the values are duplicated (otherwise it'd be a syntax error)
        - and the position of the value in the mapping corresponds to the
          enum value (by checking against the styles[] above) */
    for(UnsignedInt i = 0; i != Implementation::TextEditingStyleCount; ++i) {
        #ifdef CORRADE_TARGET_GCC
        #pragma GCC diagnostic push
        #pragma GCC diagnostic error "-Wswitch"
        #endif
        switch(TextEditingStyle(i)) {
            #define _c(style, ...) \
                case TextEditingStyle::style: \
                    CORRADE_COMPARE(UnsignedInt(styleUniforms[i]), UnsignedInt(TextEditingStyle::style)); \
                    break;
            #define _s _c
            #include "Magnum/Ui/Implementation/textEditingStyleMcssDark.h"
            #undef _s
            #undef _c
        }
        #ifdef CORRADE_TARGET_GCC
        #pragma GCC diagnostic pop
        #endif
    }
}

void StyleTest::apply() {
    auto&& data = ApplyData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    PluginManager::Manager<Trade::AbstractImporter> importerManager;

    if(!(importerManager.load("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(importerManager.load("PngImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / PngImporter plugins not found.");
    if(!(_fontManager.load("TrueTypeFont") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("TrueTypeFont plugin not found.");

    /* If overriding channel count is requested, make StbImageImporter the
       preferred plugin to load PNGs */
    if(data.imageImporterChannelCount) {
        if(!(importerManager.load("StbImageImporter") & PluginManager::LoadState::Loaded))
            CORRADE_SKIP("StbImageImporter plugin not found.");

        importerManager.setPreferredPlugins("PngImporter", {"StbImageImporter"});
        importerManager.metadata("StbImageImporter")->configuration().setValue("forceChannelCount", *data.imageImporterChannelCount);
    }

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    /* Window size isn't needed for anything here */
    ui.setSize(Vector2{200.0f, 300.0f}/data.dpiScaling, {1, 1}, {200, 300});

    struct baseLayerShared: BaseLayer::Shared {
        explicit baseLayerShared(): BaseLayer::Shared{Configuration{Implementation::BaseStyleCount}} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } baseLayerShared;

    struct TestBaseLayer: BaseLayer {
        explicit TestBaseLayer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    };
    ui.setBaseLayerInstance(Containers::pointer<TestBaseLayer>(ui.createLayer(), baseLayerShared));

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } glyphCache{PixelFormat::R8Unorm, {512, 256}};

    struct TestTextLayerShared: TextLayer::Shared {
        explicit TestTextLayerShared(): TextLayer::Shared{Configuration{Implementation::TextStyleUniformCount, Implementation::TextStyleCount}
            .setEditingStyleCount(Implementation::TextEditingStyleCount)
        } {}

        using TextLayer::Shared::setGlyphCache;

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } textLayerShared;
    textLayerShared.setGlyphCache(glyphCache);

    struct TestTextLayer: TextLayer {
        explicit TestTextLayer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    };
    ui.setTextLayerInstance(Containers::pointer<TestTextLayer>(ui.createLayer(), textLayerShared));

    ui.setEventLayerInstance(Containers::pointer<EventLayer>(ui.createLayer()));
    ui.setSnapLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));

    McssDarkStyle style;

    CORRADE_VERIFY(style.apply(ui, data.features, &importerManager, &_fontManager));
    if(data.features >= StyleFeature::BaseLayer) {
        /* No way to check the contents. Widget visuals tested in
           StyleGLTest. */
    }
    if(data.features >= StyleFeature::TextLayer) {
        CORRADE_COMPARE(ui.textLayer().shared().fontCount(), 2);
        CORRADE_COMPARE(ui.textLayer().shared().glyphCache().fontCount(), 2);

        /** @todo there's no way to get a font handle out of a style, have to
            fake it like this */
        CORRADE_COMPARE(ui.textLayer().shared().font(fontHandle(1, 1)).size(), data.expectedFontSize);

        /* No other way to check the contents. Widget visuals tested in
           StyleGLTest. */
    }
    if(data.features >= StyleFeature::TextLayerImages) {
        CORRADE_COMPARE(ui.textLayer().shared().fontCount(),
            data.features >= StyleFeature::TextLayer ? 2 : 1);
        CORRADE_COMPARE(ui.textLayer().shared().glyphCache().fontCount(),
            data.features >= StyleFeature::TextLayer ? 2 : 1);
        /* No other way to check the contents. Widget visuals tested in
           StyleGLTest. */
    }
    if(data.features >= StyleFeature::EventLayer) {
        /* Nothing to check here */
    }
    if(data.features >= StyleFeature::SnapLayouter) {
        /* No need to check everything */
        CORRADE_COMPARE(ui.snapLayouter().margin(), (Vector2{12.0f, 10.0f}));
    }
}

void StyleTest::applyTextLayerCannotOpenFont() {
    /* Manager that deliberately picks a nonexistent plugin directory to not
       have any plugins loaded */
    PluginManager::Manager<Text::AbstractFont> fontManager{"nonexistent"};

    /* Happens on builds with static plugins. Can't really do much there. */
    if(fontManager.loadState("TrueTypeFont") != PluginManager::LoadState::NotFound)
        CORRADE_SKIP("TrueTypeFont plugin loaded, cannot test");

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    ui.setSize({200, 300});

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } glyphCache{PixelFormat::R8Unorm, {512, 256}};

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(): TextLayer::Shared{Configuration{Implementation::TextStyleUniformCount, Implementation::TextStyleCount}
            .setEditingStyleCount(Implementation::TextEditingStyleCount)
        } {}

        using TextLayer::Shared::setGlyphCache;

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } layerShared;
    layerShared.setGlyphCache(glyphCache);

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    };
    ui.setTextLayerInstance(Containers::pointer<Layer>(ui.createLayer(), layerShared));

    McssDarkStyle style;

    std::ostringstream out;
    Error redirectError{&out};
    CORRADE_VERIFY(!style.apply(ui, StyleFeature::TextLayer, nullptr, &fontManager));
    CORRADE_COMPARE_AS(out.str(),
        "\nUi::McssDarkStyle::apply(): cannot open a font\n",
        TestSuite::Compare::StringHasSuffix);
}

void StyleTest::applyTextLayerImagesCannotOpen() {
    /* Manager that deliberately picks a nonexistent plugin directory to not
       have any plugins loaded */
    PluginManager::Manager<Trade::AbstractImporter> importerManager{"nonexistent"};

    /* Happens on builds with static plugins. Can't really do much there. */
    if(importerManager.loadState("AnyImageImporter") != PluginManager::LoadState::NotFound)
        CORRADE_SKIP("AnyImageImporter plugin loaded, cannot test");

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    ui.setSize({200, 300});

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } glyphCache{PixelFormat::R8Unorm, {512, 256}};

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(): TextLayer::Shared{Configuration{Implementation::TextStyleUniformCount, Implementation::TextStyleCount}
            .setEditingStyleCount(Implementation::TextEditingStyleCount)
        } {}

        using TextLayer::Shared::setGlyphCache;

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } layerShared;
    layerShared.setGlyphCache(glyphCache);

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    };
    ui.setTextLayerInstance(Containers::pointer<Layer>(ui.createLayer(), layerShared));

    McssDarkStyle style;

    std::ostringstream out;
    Error redirectError{&out};
    CORRADE_VERIFY(!style.apply(ui, StyleFeature::TextLayerImages, &importerManager, nullptr));
    CORRADE_COMPARE_AS(out.str(),
        "\nUi::McssDarkStyle::apply(): cannot open an icon atlas\n",
        TestSuite::Compare::StringHasSuffix);
}

void StyleTest::applyTextLayerImagesCannotFit() {
    if(!(_importerManager.load("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_importerManager.load("PngImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / PngImporter plugins not found.");

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    ui.setSize({200, 300});

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } glyphCache{PixelFormat::R8Unorm, {512, 256}};

    /* Add a monster image to the atlas which in turn should make it impossible
       to put anything else there */
    Vector2i offset[1];
    glyphCache.atlas().add({{500, 200}}, offset);

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(): TextLayer::Shared{Configuration{Implementation::TextStyleUniformCount, Implementation::TextStyleCount}
            .setEditingStyleCount(Implementation::TextEditingStyleCount)
        } {}

        using TextLayer::Shared::setGlyphCache;

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } layerShared;
    layerShared.setGlyphCache(glyphCache);

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    };
    ui.setTextLayerInstance(Containers::pointer<Layer>(ui.createLayer(), layerShared));

    McssDarkStyle style;

    std::ostringstream out;
    Error redirectError{&out};
    CORRADE_VERIFY(!style.apply(ui, StyleFeature::TextLayerImages, &_importerManager, nullptr));
    CORRADE_COMPARE(out.str(), "Ui::McssDarkStyle::apply(): cannot fit 2 icons into the glyph cache\n");
}

void StyleTest::applyTextLayerImagesUnexpectedFormat() {
    PluginManager::Manager<Trade::AbstractImporter> importerManager;

    if(!(importerManager.load("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(importerManager.load("StbImageImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / StbImageImporter plugins not found.");

    /* Make StbImageImporter the preferred plugin to load PNGs, but configure
       it to open the image as a different format */
    importerManager.setPreferredPlugins("PngImporter", {"StbImageImporter"});
    importerManager.metadata("StbImageImporter")->configuration().setValue("forceBitDepth", 32);

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    ui.setSize({200, 300});

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } glyphCache{PixelFormat::R8Unorm, {512, 256}};

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(): TextLayer::Shared{Configuration{Implementation::TextStyleUniformCount, Implementation::TextStyleCount}
            .setEditingStyleCount(Implementation::TextEditingStyleCount)
        } {}

        using TextLayer::Shared::setGlyphCache;

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } layerShared;
    layerShared.setGlyphCache(glyphCache);

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    };
    ui.setTextLayerInstance(Containers::pointer<Layer>(ui.createLayer(), layerShared));

    McssDarkStyle style;

    std::ostringstream out;
    Error redirectError{&out};
    CORRADE_VERIFY(!style.apply(ui, StyleFeature::TextLayerImages, &importerManager, nullptr));
    CORRADE_COMPARE(out.str(), "Ui::McssDarkStyle::apply(): expected PixelFormat::R8Unorm icons but got an image with PixelFormat::R32F\n");
}

void StyleTest::applyTextLayerTwice() {
    if(!(_importerManager.load("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_importerManager.load("PngImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / PngImporter plugins not found.");
    if(!(_fontManager.load("TrueTypeFont") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("TrueTypeFont plugin not found.");

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    /* Window size isn't used for anything, use a 1.5x DPI scaling to have the
       glyph cache fully filled */
    ui.setSize({200, 300}, {1, 1}, {300, 450});

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } glyphCache{PixelFormat::R8Unorm, {512, 256}};

    struct TestTextLayerShared: TextLayer::Shared {
        explicit TestTextLayerShared(): TextLayer::Shared{Configuration{Implementation::TextStyleUniformCount, Implementation::TextStyleCount}
            .setEditingStyleCount(Implementation::TextEditingStyleCount)
        } {}

        using TextLayer::Shared::setGlyphCache;

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } textLayerShared;
    textLayerShared.setGlyphCache(glyphCache);

    struct TestTextLayer: TextLayer {
        explicit TestTextLayer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    };
    ui.setTextLayerInstance(Containers::pointer<TestTextLayer>(ui.createLayer(), textLayerShared));

    McssDarkStyle style;
    CORRADE_VERIFY(style.apply(ui, StyleFeature::TextLayer|StyleFeature::TextLayerImages, &_importerManager, &_fontManager));
    CORRADE_COMPARE(ui.textLayer().shared().fontCount(), 2);
    CORRADE_COMPARE(ui.textLayer().shared().glyphCache().fontCount(), 2);

    CORRADE_EXPECT_FAIL("This shouldn't fail but should instead replace the previous font and image font, *somehow*.");
    CORRADE_VERIFY(style.apply(ui, StyleFeature::TextLayer|StyleFeature::TextLayerImages, &_importerManager, &_fontManager));
    CORRADE_COMPARE(ui.textLayer().shared().fontCount(), 2);
    CORRADE_COMPARE(ui.textLayer().shared().glyphCache().fontCount(), 2);
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::StyleTest)
