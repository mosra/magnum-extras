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

#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Containers/StringIterable.h>
#include <Corrade/PluginManager/Manager.h>
#include <Corrade/PluginManager/PluginMetadata.h>
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/String.h>
#include <Corrade/Utility/ConfigurationGroup.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/Math/Range.h>
#include <Magnum/Text/AbstractGlyphCache.h>
#include <Magnum/Text/AbstractFont.h>
#include <Magnum/TextureTools/Atlas.h>
#include <Magnum/Trade/AbstractImporter.h>

#include "Magnum/Ui/AbstractTheme.hpp" /* TextStyle in a single test case */
#include "Magnum/Ui/BaseLayer.h"
#include "Magnum/Ui/BaseLayerAnimator.h"
#include "Magnum/Ui/DataLayer.h"
#include "Magnum/Ui/Event.h"
#include "Magnum/Ui/EventLayer.h"
#include "Magnum/Ui/GenericLayouter.h"
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/LayoutLayer.h"
#include "Magnum/Ui/NodeFlags.h"
#include "Magnum/Ui/SnapLayouter.h"
#include "Magnum/Ui/TextLayer.h"
#include "Magnum/Ui/TextLayerAnimator.h"
#include "Magnum/Ui/TextProperties.h"
#include "Magnum/Ui/Theme.h"
#include "Magnum/Ui/UserInterface.h"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct ThemeTest: TestSuite::Tester {
    explicit ThemeTest();

    void debugFeature();
    void debugFeatures();
    void debugFeaturesSupersets();

    void baseStyleDark();
    void textStyleDark();
    void layoutStyleDark();

    void apply();
    void applyTextLayerCannotOpenFont();
    void applyTextLayerCannotFillCache();
    void applyTextLayerImagesCannotOpen();
    void applyTextLayerImagesCannotFit();
    void applyTextLayerImagesUnexpectedFormat();
    void applyTextLayerTwice();

    void removePreviousAnimationForBlinkingCursor();

    private:
        PluginManager::Manager<Trade::AbstractImporter> _importerManager;
        PluginManager::Manager<Text::AbstractFont> _fontManager;
};

using namespace Math::Literals;

const struct {
    const char* name;
    DarkTheme::Features themeFeatures;
    ThemeFeatures features;
    Containers::Optional<UnsignedInt> imageImporterChannelCount;
    Float dpiScaling, expectedFontSize;
} ApplyData[]{
    {"data layer", {},
        ThemeFeature::DataLayer, {}, 1.0f, 0.0f},
    {"base layer only", {},
        ThemeFeature::BaseLayer, {}, 1.0f, 0.0f},
    {"base layer, animations enabled but not applied",
        DarkTheme::Feature::Animations,
        ThemeFeature::BaseLayer, {}, 1.0f, 0.0f},
    /* There are currently no essential animations in BaseLayer */
    {"base layer + base layer animations",
        DarkTheme::Feature::Animations,
        ThemeFeature::BaseLayer|ThemeFeature::BaseLayerAnimations, {}, 1.0f, 0.0f},
    {"base layer animations only",
        DarkTheme::Feature::Animations,
        ThemeFeature::BaseLayerAnimations, {}, 1.0f, 0.0f},
    {"text layer only", {},
        ThemeFeature::TextLayer, {}, 1.0f, 16.0f*2},
    {"text layer + text layer images", {},
        ThemeFeature::TextLayer|ThemeFeature::TextLayerImages, {}, 1.0f, 16.0f*2},
    {"text layer images only", {},
        ThemeFeature::TextLayerImages, {}, 1.0f, 16.0f*2},
    {"text layer images only, imported as RG", {},
        ThemeFeature::TextLayerImages, 2, 1.0f, 16.0f*2},
    {"text layer, animations enabled but not applied",
        DarkTheme::Feature::Animations,
        ThemeFeature::TextLayer, {}, 1.0f, 16.0f*2},
    {"text layer + essential text layer animations",
        DarkTheme::Feature::EssentialAnimations,
        ThemeFeature::TextLayer|ThemeFeature::TextLayerAnimations, {}, 1.0f, 16.0f*2},
    {"text layer + text layer animations",
        DarkTheme::Feature::Animations,
        ThemeFeature::TextLayer|ThemeFeature::TextLayerAnimations, {}, 1.0f, 16.0f*2},
    {"text layer animations only",
        DarkTheme::Feature::Animations,
        ThemeFeature::TextLayerAnimations, {}, 1.0f, 16.0f*2},
    {"event layer", {},
        ThemeFeature::EventLayer, {}, 1.0f, 0.0f},
    {"layout layer", {},
        ThemeFeature::LayoutLayer, {}, 1.0f, 0.0f},
    {"snap layouter", {},
        ThemeFeature::SnapLayouter, {}, 1.0f, 0.0f},
    {"generic layouter", {},
        ThemeFeature::GenericLayouter, {}, 1.0f, 0.0f},
    {"everything",
        DarkTheme::Feature::Animations,
        ~ThemeFeatures{}, {}, 1.0f, 16.0f*2},
    {"text layer + text layer images, 0.625x DPI scaling", {},
        ThemeFeature::TextLayer|ThemeFeature::TextLayerImages, {}, 0.625f, 10.0f*2},
};

ThemeTest::ThemeTest() {
    addTests({&ThemeTest::debugFeature,
              &ThemeTest::debugFeatures,
              &ThemeTest::debugFeaturesSupersets,

              &ThemeTest::baseStyleDark,
              &ThemeTest::textStyleDark,
              &ThemeTest::layoutStyleDark});

    addInstancedTests({&ThemeTest::apply},
        Containers::arraySize(ApplyData));

    addTests({&ThemeTest::applyTextLayerCannotOpenFont,
              &ThemeTest::applyTextLayerCannotFillCache,
              &ThemeTest::applyTextLayerImagesCannotOpen,
              &ThemeTest::applyTextLayerImagesCannotFit,
              &ThemeTest::applyTextLayerImagesUnexpectedFormat,
              &ThemeTest::applyTextLayerTwice,

              &ThemeTest::removePreviousAnimationForBlinkingCursor});
}

using Implementation::BaseStyle;
using Implementation::TextStyle;
using Implementation::LayoutStyle;

void ThemeTest::debugFeature() {
    Containers::String out;
    Debug{&out} << DarkTheme::Feature::Animations << DarkTheme::Feature(0xbe);
    CORRADE_COMPARE(out, "Ui::DarkTheme::Feature::Animations Ui::DarkTheme::Feature(0xbe)\n");
}

void ThemeTest::debugFeatures() {
    Containers::String out;
    Debug{&out} << (DarkTheme::Feature::EssentialAnimations|DarkTheme::Feature(0x80)) << DarkTheme::Features{};
    CORRADE_COMPARE(out, "Ui::DarkTheme::Feature::EssentialAnimations|Ui::DarkTheme::Feature(0x80) Ui::DarkTheme::Features{}\n");
}

void ThemeTest::debugFeaturesSupersets() {
    /* Animations is a superset of EssentialAnimations, so only one should be
       printed */
    {
        Containers::String out;
        Debug{&out} << (DarkTheme::Feature::EssentialAnimations|DarkTheme::Feature::Animations);
        CORRADE_COMPARE(out, "Ui::DarkTheme::Feature::Animations\n");
    }
}

/* These arrays are used by the following test cases to verify that the
   generated files are up-to-date, matching the style enums */
const BaseStyle BaseStyles[]{
    #define _c(style, ...) BaseStyle::style,
    #include "Magnum/Ui/Implementation/themeDarkBaseStyleUniforms.h"
    #undef _c
};
const TextStyle TextStyles[]{
    #define _c(style, ...) TextStyle::style,
    #include "Magnum/Ui/Implementation/themeDarkTextStyles.h"
    #undef _c
};
const LayoutStyle LayoutStyles[]{
    #define _c(style, ...) LayoutStyle::style,
    #include "Magnum/Ui/Implementation/themeDarkLayoutStyles.h"
    #undef _c
};

/* This cryptic nonsense is just to have statically defined style counts,
   without having to rely on runtime-defined values coming from live Theme
   instances */
enum: UnsignedInt {
    BaseStyleCount = Containers::arraySize(BaseStyles),
    TextStyleCount = Containers::arraySize(TextStyles),
    #define _c(...) + 1
    TextStyleUniformCount =
        #include "Magnum/Ui/Implementation/themeDarkTextStyleUniforms.h"
        ,
    TextEditingStyleUniformCount =
        #include "Magnum/Ui/Implementation/themeDarkTextEditingStyles.h"
        ,
    #undef _c
    LayoutStyleCount = Containers::arraySize(LayoutStyles),

};

void ThemeTest::baseStyleDark() {
    /* This checks that:
        - the mapping contains entries for all enum values (otherwise -Wswitch
          would trigger)
        - none of the values are duplicated (otherwise it'd be a syntax error)
        - the position of the value in the mapping corresponds to the enum
          value (by checking against the BaseStyleUniforms[] above)
        - there are no extra values in the array that wouldn't be handled by
          any of the switch cases
       Given that the Implementation/theme* files are autogenerated, this test
       failing (or failing to compile) means they're out of date and need to be
       regenerated. */
    UnsignedInt unhandledCount = 0;
    for(UnsignedInt i = 0; i != Containers::arraySize(BaseStyles); ++i) {
        #ifdef CORRADE_TARGET_GCC
        #pragma GCC diagnostic push
        #pragma GCC diagnostic error "-Wswitch"
        #endif
        switch(BaseStyle(i)) {
            #define _c(style, ...) \
                case BaseStyle::style: \
                    CORRADE_COMPARE(UnsignedInt(BaseStyles[i]), UnsignedInt(BaseStyle::style)); \
                    continue;
            #include "Magnum/Ui/Implementation/themeDarkBaseStyleUniforms.h"
            #undef _c
            case BaseStyle::Count:
                CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        }
        #ifdef CORRADE_TARGET_GCC
        #pragma GCC diagnostic pop
        #endif

        ++unhandledCount;
    }
    CORRADE_COMPARE(unhandledCount, 0);

    /* Just a sanity check. If this fails, the Count isn't the last value of
       the enum, it has an explicit value assigned or the theme header contains
       some strange extra values. */
    CORRADE_COMPARE(Int(BaseStyle::Count), Containers::arraySize(BaseStyles));
}

void ThemeTest::textStyleDark() {
    /* This checks that:
        - the mapping contains entries for all enum values (otherwise -Wswitch
          would trigger)
        - none of the values are duplicated (otherwise it'd be a syntax error)
        - the position of the value in the mapping corresponds to the enum
          value (by checking against the TextStyles[] above)
        - there are no extra values in the array that wouldn't be handled by
          any of the switch cases
       Given that the Implementation/theme* files are autogenerated, this test
       failing (or failing to compile) means they're out of date and need to be
       regenerated. */
    UnsignedInt unhandledCount = 0;
    for(UnsignedInt i = 0; i != Containers::arraySize(TextStyles); ++i) {
        #ifdef CORRADE_TARGET_GCC
        #pragma GCC diagnostic push
        #pragma GCC diagnostic error "-Wswitch"
        #endif
        switch(TextStyle(i)) {
            #define _c(style, ...) \
                case TextStyle::style: \
                    CORRADE_COMPARE(UnsignedInt(TextStyles[i]), UnsignedInt(TextStyle::style)); \
                    continue;
            #include "Magnum/Ui/Implementation/themeDarkTextStyles.h"
            #undef _c
            case TextStyle::Count:
                CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        }
        #ifdef CORRADE_TARGET_GCC
        #pragma GCC diagnostic pop
        #endif

        ++unhandledCount;
    }
    CORRADE_COMPARE(unhandledCount, 0);

    /* Just a sanity check. If this fails, the Count isn't the last value of
       the enum, it has an explicit value assigned or the theme header contains
       some strange extra values. */
    CORRADE_COMPARE(Int(TextStyle::Count), Containers::arraySize(TextStyles));
}

void ThemeTest::layoutStyleDark() {
    /* This checks that:
        - the mapping contains entries for all enum values (otherwise -Wswitch
          would trigger)
        - none of the values are duplicated (otherwise it'd be a syntax error)
        - the position of the value in the mapping corresponds to the enum
          value (by checking against the LayoutStyles[] above)
        - there are no extra values in the array that wouldn't be handled by
          any of the switch cases
       Given that the Implementation/theme* files are autogenerated, this test
       failing (or failing to compile) means they're out of date and need to be
       regenerated. */
    UnsignedInt unhandledCount = 0;
    for(UnsignedInt i = 0; i != Containers::arraySize(LayoutStyles); ++i) {
        #ifdef CORRADE_TARGET_GCC
        #pragma GCC diagnostic push
        #pragma GCC diagnostic error "-Wswitch"
        #endif
        switch(LayoutStyle(i)) {
            #define _c(style, ...) \
                case LayoutStyle::style: \
                    CORRADE_COMPARE(UnsignedInt(LayoutStyles[i]), UnsignedInt(LayoutStyle::style)); \
                    continue;
            #include "Magnum/Ui/Implementation/themeDarkLayoutStyles.h"
            #undef _c
            case LayoutStyle::Count:
                CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        }
        #ifdef CORRADE_TARGET_GCC
        #pragma GCC diagnostic pop
        #endif

        ++unhandledCount;
    }
    CORRADE_COMPARE(unhandledCount, 0);

    /* Just a sanity check. If this fails, the Count isn't the last value of
       the enum, it has an explicit value assigned or the theme header contains
       some strange extra values. */
    CORRADE_COMPARE(Int(LayoutStyle::Count), Containers::arraySize(LayoutStyles));
}

void ThemeTest::apply() {
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

    ui.setDataLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));

    struct BaseLayerShared: BaseLayer::Shared {
        explicit BaseLayerShared(): BaseLayer::Shared{Configuration{BaseStyleCount}
            .setDynamicStyleCount(10) /* for animations */
        } {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } baseLayerShared;

    struct TestBaseLayer: BaseLayer {
        explicit TestBaseLayer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    };
    ui.setBaseLayerInstance(Containers::pointer<TestBaseLayer>(ui.createLayer(), baseLayerShared));
    ui.setBaseLayerStyleAnimatorInstance(Containers::pointer<BaseLayerStyleAnimator>(ui.createAnimator()));

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } glyphCache{PixelFormat::R8Unorm, {512, 512}};

    struct TestTextLayerShared: TextLayer::Shared {
        explicit TestTextLayerShared(Text::AbstractGlyphCache& glyphCache): TextLayer::Shared{glyphCache, Configuration{TextStyleUniformCount, TextStyleCount}
            .setEditingStyleCount(TextEditingStyleUniformCount)
            .setDynamicStyleCount(10) /* for animations */
        } {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } textLayerShared{glyphCache};

    struct TestTextLayer: TextLayer {
        explicit TestTextLayer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    };
    ui.setTextLayerInstance(Containers::pointer<TestTextLayer>(ui.createLayer(), textLayerShared));
    ui.setTextLayerStyleAnimatorInstance(Containers::pointer<TextLayerStyleAnimator>(ui.createAnimator()));

    ui.setEventLayerInstance(Containers::pointer<EventLayer>(ui.createLayer()));
    ui.setLayoutLayerInstance(Containers::pointer<LayoutLayer>(ui.createLayer(), LayoutStyleCount));
    ui.setSnapLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));
    ui.setGenericLayouterInstance(Containers::pointer<GenericLayouter>(ui.createLayouter()));

    DarkTheme theme{data.themeFeatures};
    /* The AbstractTheme can have the set of features bigger than what the
       particular DarkTheme supports, which would then fail if data.features is
       ~ThemeFeatures{}. Make it an union with what's actually supported. */
    CORRADE_VERIFY(theme.apply(ui, data.features & theme.features(), &importerManager, &_fontManager));

    /* Style transition for disabled functions should be set if base / text
       layer style is set and not if not */
    CORRADE_COMPARE(ui.baseLayer().shared().styleTransitionToDisabled(), data.features >= ThemeFeature::BaseLayer);
    CORRADE_COMPARE(ui.textLayer().shared().styleTransitionToDisabled(), data.features >= ThemeFeature::TextLayer);

    /* Style fade out animations should be set if base / text animations are
       set and not if not. In case of text layer there are also essential
       persistent animations. */
    CORRADE_VERIFY(!ui.baseLayer().shared().styleAnimationOnEnter());
    CORRADE_VERIFY(!ui.textLayer().shared().styleAnimationOnEnter());

    CORRADE_VERIFY(!ui.baseLayer().shared().styleAnimationOnFocus());
    CORRADE_VERIFY(!ui.textLayer().shared().styleAnimationOnFocus());

    CORRADE_VERIFY(!ui.baseLayer().shared().styleAnimationOnPress());
    CORRADE_VERIFY(!ui.textLayer().shared().styleAnimationOnPress());

    CORRADE_COMPARE(ui.baseLayer().shared().styleAnimationOnLeave(),
        data.features >= ThemeFeature::BaseLayerAnimations &&
        data.themeFeatures >= DarkTheme::Feature::Animations);
    CORRADE_COMPARE(ui.textLayer().shared().styleAnimationOnLeave(),
        data.features >= ThemeFeature::TextLayerAnimations &&
        data.themeFeatures >= DarkTheme::Feature::Animations);

    CORRADE_COMPARE(ui.baseLayer().shared().styleAnimationOnBlur(),
        data.features >= ThemeFeature::BaseLayerAnimations &&
        data.themeFeatures >= DarkTheme::Feature::Animations);
    CORRADE_COMPARE(ui.textLayer().shared().styleAnimationOnBlur(),
        data.features >= ThemeFeature::TextLayerAnimations &&
        data.themeFeatures >= DarkTheme::Feature::Animations);

    CORRADE_COMPARE(ui.baseLayer().shared().styleAnimationOnRelease(),
        data.features >= ThemeFeature::BaseLayerAnimations &&
        data.themeFeatures >= DarkTheme::Feature::Animations);
    CORRADE_COMPARE(ui.textLayer().shared().styleAnimationOnRelease(),
        data.features >= ThemeFeature::TextLayerAnimations &&
        data.themeFeatures >= DarkTheme::Feature::Animations);

    CORRADE_VERIFY(!ui.baseLayer().shared().styleAnimationPersistent());
    CORRADE_COMPARE(ui.textLayer().shared().styleAnimationPersistent(),
        data.features >= ThemeFeature::TextLayerAnimations &&
        data.themeFeatures >= DarkTheme::Feature::EssentialAnimations);

    if(data.features >= ThemeFeature::DataLayer) {
        /* Nothing to check here */
    }
    if(data.features >= ThemeFeature::TextLayer) {
        CORRADE_COMPARE(ui.textLayer().shared().fontCount(), 2);
        CORRADE_COMPARE(ui.textLayer().shared().glyphCache().fontCount(), 2);

        /** @todo there's no way to get a font handle out of a style, have to
            fake it like this */
        CORRADE_COMPARE(ui.textLayer().shared().font(fontHandle(1, 1)).size(), data.expectedFontSize);

        /* No other way to check the contents. Widget visuals tested in
           <Widget>GLTest. */
    }
    if(data.features >= ThemeFeature::TextLayerImages) {
        CORRADE_COMPARE(ui.textLayer().shared().fontCount(),
            data.features >= ThemeFeature::TextLayer ? 2 : 1);
        CORRADE_COMPARE(ui.textLayer().shared().glyphCache().fontCount(),
            data.features >= ThemeFeature::TextLayer ? 2 : 1);
        /* No other way to check the contents. Widget visuals tested in
           <Widget>GLTest. */
    }
    if(data.features >= ThemeFeature::EventLayer) {
        /* Nothing to check here */
    }
    if(data.features >= ThemeFeature::LayoutLayer) {
        /* Nothing to check here */
    }
    if(data.features >= ThemeFeature::SnapLayouter) {
        /* Nothing to check here */
    }
    if(data.features >= ThemeFeature::GenericLayouter) {
        /* Nothing to check here */
    }
}

void ThemeTest::applyTextLayerCannotOpenFont() {
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
    } glyphCache{PixelFormat::R8Unorm, {512, 512}};

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache): TextLayer::Shared{glyphCache, Configuration{TextStyleUniformCount, TextStyleCount}
            .setEditingStyleCount(TextEditingStyleUniformCount)
        } {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } layerShared{glyphCache};

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    };
    ui.setTextLayerInstance(Containers::pointer<Layer>(ui.createLayer(), layerShared));

    DarkTheme theme;

    Containers::String out;
    Error redirectError{&out};
    CORRADE_VERIFY(!theme.apply(ui, ThemeFeature::TextLayer, nullptr, &fontManager));
    /* Error from the PluginManager is printed before */
    CORRADE_COMPARE_AS(out,
        "\nUi::DarkTheme::apply(): cannot open a font\n",
        TestSuite::Compare::StringHasSuffix);
}

void ThemeTest::applyTextLayerCannotFillCache() {
    if(!(_fontManager.load("TrueTypeFont") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("TrueTypeFont plugin not found.");

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    ui.setSize({200, 300});

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } glyphCache{PixelFormat::R8Unorm, {512, 512}};

    /* Add a monster image to the atlas which in turn should make it impossible
       to put anything else there */
    Vector2i offset[1];
    CORRADE_VERIFY(glyphCache.atlas().add({{500, 500}}, offset));

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache): TextLayer::Shared{glyphCache, Configuration{TextStyleUniformCount, TextStyleCount}
            .setEditingStyleCount(TextEditingStyleUniformCount)
        } {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } layerShared{glyphCache};

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    };
    ui.setTextLayerInstance(Containers::pointer<Layer>(ui.createLayer(), layerShared));

    DarkTheme theme;

    Containers::String out;
    Error redirectError{&out};
    CORRADE_VERIFY(!theme.apply(ui, ThemeFeature::TextLayer, nullptr, &_fontManager));
    /* Error from the font fillGlyphCache() is printed before */
    CORRADE_COMPARE_AS(out,
        "\nUi::DarkTheme::apply(): cannot fill a glyph cache\n",
        TestSuite::Compare::StringHasSuffix);
}

void ThemeTest::applyTextLayerImagesCannotOpen() {
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
    } glyphCache{PixelFormat::R8Unorm, {512, 512}};

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache): TextLayer::Shared{glyphCache, Configuration{TextStyleUniformCount, TextStyleCount}
            .setEditingStyleCount(TextEditingStyleUniformCount)
        } {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } layerShared{glyphCache};

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    };
    ui.setTextLayerInstance(Containers::pointer<Layer>(ui.createLayer(), layerShared));

    DarkTheme theme;

    Containers::String out;
    Error redirectError{&out};
    CORRADE_VERIFY(!theme.apply(ui, ThemeFeature::TextLayerImages, &importerManager, nullptr));
    CORRADE_COMPARE_AS(out,
        "\nUi::DarkTheme::apply(): cannot open an icon atlas\n",
        TestSuite::Compare::StringHasSuffix);
}

void ThemeTest::applyTextLayerImagesCannotFit() {
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
    } glyphCache{PixelFormat::R8Unorm, {512, 512}};

    /* Add a monster image to the atlas which in turn should make it impossible
       to put anything else there */
    Vector2i offset[1];
    CORRADE_VERIFY(glyphCache.atlas().add({{500, 500}}, offset));

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache): TextLayer::Shared{glyphCache, Configuration{TextStyleUniformCount, TextStyleCount}
            .setEditingStyleCount(TextEditingStyleUniformCount)
        } {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } layerShared{glyphCache};

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    };
    ui.setTextLayerInstance(Containers::pointer<Layer>(ui.createLayer(), layerShared));

    DarkTheme theme;

    Containers::String out;
    Error redirectError{&out};
    CORRADE_VERIFY(!theme.apply(ui, ThemeFeature::TextLayerImages, &_importerManager, nullptr));
    CORRADE_COMPARE(out, "Ui::DarkTheme::apply(): cannot fit 2 icons into the glyph cache\n");
}

void ThemeTest::applyTextLayerImagesUnexpectedFormat() {
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
    } glyphCache{PixelFormat::R8Unorm, {512, 512}};

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache): TextLayer::Shared{glyphCache, Configuration{TextStyleUniformCount, TextStyleCount}
            .setEditingStyleCount(TextEditingStyleUniformCount)
        } {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } layerShared{glyphCache};

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    };
    ui.setTextLayerInstance(Containers::pointer<Layer>(ui.createLayer(), layerShared));

    DarkTheme theme;

    Containers::String out;
    Error redirectError{&out};
    CORRADE_VERIFY(!theme.apply(ui, ThemeFeature::TextLayerImages, &importerManager, nullptr));
    CORRADE_COMPARE(out, "Ui::DarkTheme::apply(): expected PixelFormat::R8Unorm icons but got an image with PixelFormat::R32F\n");
}

void ThemeTest::applyTextLayerTwice() {
    if(!(_importerManager.load("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_importerManager.load("PngImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / PngImporter plugins not found.");
    if(!(_fontManager.load("TrueTypeFont") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("TrueTypeFont plugin not found.");

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    /* Window size isn't used for anything, use a 2x DPI scaling to have the
       glyph cache fully filled */
    ui.setSize({200, 300}, {1, 1}, {400, 600});

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } glyphCache{PixelFormat::R8Unorm, {512, 512}};

    struct TestTextLayerShared: TextLayer::Shared {
        explicit TestTextLayerShared(Text::AbstractGlyphCache& glyphCache): TextLayer::Shared{glyphCache, Configuration{TextStyleUniformCount, TextStyleCount}
            .setEditingStyleCount(TextEditingStyleUniformCount)
        } {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } textLayerShared{glyphCache};

    struct TestTextLayer: TextLayer {
        explicit TestTextLayer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    };
    ui.setTextLayerInstance(Containers::pointer<TestTextLayer>(ui.createLayer(), textLayerShared));

    DarkTheme theme;
    CORRADE_VERIFY(theme.apply(ui, ThemeFeature::TextLayer|ThemeFeature::TextLayerImages, &_importerManager, &_fontManager));
    CORRADE_COMPARE(ui.textLayer().shared().fontCount(), 2);
    CORRADE_COMPARE(ui.textLayer().shared().glyphCache().fontCount(), 2);

    CORRADE_EXPECT_FAIL("This shouldn't fail but should instead replace the previous font and image font, *somehow*.");
    CORRADE_VERIFY(theme.apply(ui, ThemeFeature::TextLayer|ThemeFeature::TextLayerImages, &_importerManager, &_fontManager));
    CORRADE_COMPARE(ui.textLayer().shared().fontCount(), 2);
    CORRADE_COMPARE(ui.textLayer().shared().glyphCache().fontCount(), 2);
}

void ThemeTest::removePreviousAnimationForBlinkingCursor() {
    /* Verifies that a fade out animation is properly removed when
       transitioning to a focused style with blinking cursor. Otherwise it'd
       assert at runtime. */

    PluginManager::Manager<Trade::AbstractImporter> importerManager;

    if(!(_fontManager.load("TrueTypeFont") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("TrueTypeFont plugin not found.");

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    ui.setSize({100, 100});

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } glyphCache{PixelFormat::R8Unorm, {512, 512}};

    struct TestTextLayerShared: TextLayer::Shared {
        explicit TestTextLayerShared(Text::AbstractGlyphCache& glyphCache): TextLayer::Shared{glyphCache, Configuration{TextStyleUniformCount, TextStyleCount}
            .setEditingStyleCount(TextEditingStyleUniformCount)
            .setDynamicStyleCount(10) /* for animations */
        } {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } textLayerShared{glyphCache};

    struct TestTextLayer: TextLayer {
        explicit TestTextLayer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    };
    ui.setTextLayerInstance(Containers::pointer<TestTextLayer>(ui.createLayer(), textLayerShared));
    ui.setTextLayerStyleAnimatorInstance(Containers::pointer<TextLayerStyleAnimator>(ui.createAnimator()));

    DarkTheme theme{DarkTheme::Feature::Animations};
    CORRADE_VERIFY(theme.apply(ui, ThemeFeature::TextLayer|ThemeFeature::TextLayerAnimations, nullptr, &_fontManager));

    /* Not using Input as that would require also the base layer, setting up a
       styled data directly */
    NodeHandle node = ui.createNode({}, {100, 100}, NodeFlag::Focusable);
    DataHandle data = ui.textLayer().create(TextStyle::InputDanger, "", {}, node);

    /* Press switches to InputDangerPressed, immediately */
    PointerEvent pressEvent{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0, {}};
    CORRADE_VERIFY(ui.pointerPressEvent({50, 50}, pressEvent));
    CORRADE_COMPARE(ui.textLayerStyleAnimator().capacity(), 0);
    CORRADE_COMPARE(ui.textLayer().style(data), UnsignedInt(TextStyle::InputDangerPressed));

    /* Release makes a transition animation, and then directly after a
       persistent animation, removing the transition. Initially the style is
       unchanged... */
    PointerEvent releaseEvent{667.0_sec, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0, {}};
    CORRADE_VERIFY(ui.pointerReleaseEvent({50, 50}, releaseEvent));
    CORRADE_COMPARE(ui.textLayerStyleAnimator().capacity(), 1);
    CORRADE_COMPARE(ui.currentFocusedNode(), node);
    CORRADE_COMPARE(ui.textLayer().style(data), UnsignedInt(TextStyle::InputDangerPressed));

    /* After an advance it changes to a dynamic style that's associated with an
       animation */
    ui.advanceAnimations(667.0_sec);
    CORRADE_COMPARE(ui.textLayer().style(data), TextStyleCount + 0);
    AnimationHandle animation = ui.textLayer().dynamicStyleAnimation(0);
    CORRADE_COMPARE(ui.textLayerStyleAnimator().styles(animation), Containers::pair(
        UnsignedInt(TextStyle::InputDangerFocusedBlink),
        UnsignedInt(TextStyle::InputDangerFocused)));
    /* The animation has a handle with generation 2 as it replaces the
       previously made transition animation */
    CORRADE_COMPARE(animationHandleData(animation), animatorDataHandle(0, 2));
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::ThemeTest)
