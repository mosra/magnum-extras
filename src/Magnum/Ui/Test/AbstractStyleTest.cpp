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

#include <Corrade/Containers/ArrayView.h>
#include <Corrade/Containers/String.h>
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/String.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/Text/AbstractFont.h>
#include <Magnum/Text/AbstractGlyphCache.h>
#include <Magnum/Trade/AbstractImporter.h>

#include "Magnum/Ui/AbstractStyle.h"
#include "Magnum/Ui/BaseLayer.h"
#include "Magnum/Ui/EventLayer.h"
#include "Magnum/Ui/SnapLayouter.h"
#include "Magnum/Ui/TextLayer.h"
#include "Magnum/Ui/UserInterface.h"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct AbstractStyleTest: TestSuite::Tester {
    explicit AbstractStyleTest();

    void debugFeature();
    void debugFeatures();

    void construct();
    void constructCopy();

    void noFeaturesReturned();

    void styleCount();
    void styleCountNotSupported();
    void styleCountNotImplemented();
    void styleCountNotImplementedDefaults();

    void setBaseLayerDynamicStyleCount();

    void baseLayerFlags();
    void baseLayerFlagsNotImplementedDefaults();
    void baseLayerFlagsInvalid();

    void setTextLayerDynamicStyleCount();

    void textLayerGlyphCacheProperties();
    void textLayerGlyphCachePropertiesNotSupported();
    void textLayerGlyphCachePropertiesNotImplemented();
    void textLayerGlyphCachePropertiesNotImplementedDefaults();
    void textLayerGlyphCacheSizeNoTextFeature();
    void textLayerGlyphCacheSizeFeaturesNotSupported();
    void setTextLayerGlyphCacheSize();

    void apply();
    void applyNoSizeSet();
    void applyNoFeatures();
    void applyFeaturesNotSupported();
    void applyBaseLayerNotPresent();
    void applyBaseLayerDifferentStyleCount();
    void applyTextLayerNotPresent();
    void applyTextLayerDifferentStyleCount();
    void applyTextLayerDifferentGlyphCache();
    void applyTextLayerNoFontManager();
    void applyTextLayerImagesTextLayerNotPresent();
    void applyTextLayerImagesNoImporterManager();
    void applyEventLayerNotPresent();
    void applySnapLayouterNotPresent();

    private:
        PluginManager::Manager<Trade::AbstractImporter> _importerManager;
        PluginManager::Manager<Text::AbstractFont> _fontManager;
};

const struct {
    const char* name;
    bool baseLayerPresent, textLayerPresent, eventLayerPresent, snapLayouterPresent;
    StyleFeatures features;
    bool succeed;
} ApplyData[]{
    {"base layer only", true, false, false, false,
        StyleFeature::BaseLayer, true},
    {"text layer only", false, true, false, false,
        StyleFeature::TextLayer, true},
    {"text layer images only", false, true, false, false,
        StyleFeature::TextLayerImages, true},
    {"text layer + text layer images", false, true, false, false,
        StyleFeature::TextLayer|StyleFeature::TextLayerImages, true},
    {"event layer only", false, false, true, false,
        StyleFeature::EventLayer, true},
    {"snap layouter only", false, false, false, true,
        StyleFeature::SnapLayouter, true},
    {"everything except base layer", false, true, true, true,
        ~StyleFeature::BaseLayer, true},
    {"everything", true, true, true, true,
        ~StyleFeatures{}, true},
    {"application failed", true, false, false, false,
        StyleFeature::BaseLayer, false}
};

AbstractStyleTest::AbstractStyleTest() {
    addTests({&AbstractStyleTest::debugFeature,
              &AbstractStyleTest::debugFeatures,

              &AbstractStyleTest::construct,
              &AbstractStyleTest::constructCopy,

              &AbstractStyleTest::noFeaturesReturned,

              &AbstractStyleTest::styleCount,
              &AbstractStyleTest::styleCountNotSupported,
              &AbstractStyleTest::styleCountNotImplemented,
              &AbstractStyleTest::styleCountNotImplementedDefaults,

              &AbstractStyleTest::setBaseLayerDynamicStyleCount,

              &AbstractStyleTest::baseLayerFlags,
              &AbstractStyleTest::baseLayerFlagsNotImplementedDefaults,
              &AbstractStyleTest::baseLayerFlagsInvalid,

              &AbstractStyleTest::setTextLayerDynamicStyleCount,

              &AbstractStyleTest::textLayerGlyphCacheProperties,
              &AbstractStyleTest::textLayerGlyphCachePropertiesNotSupported,
              &AbstractStyleTest::textLayerGlyphCachePropertiesNotImplemented,
              &AbstractStyleTest::textLayerGlyphCachePropertiesNotImplementedDefaults,
              &AbstractStyleTest::textLayerGlyphCacheSizeNoTextFeature,
              &AbstractStyleTest::textLayerGlyphCacheSizeFeaturesNotSupported,
              &AbstractStyleTest::setTextLayerGlyphCacheSize});

    addInstancedTests({&AbstractStyleTest::apply},
        Containers::arraySize(ApplyData));

    addTests({&AbstractStyleTest::applyNoFeatures,
              &AbstractStyleTest::applyFeaturesNotSupported,
              &AbstractStyleTest::applyNoSizeSet,
              &AbstractStyleTest::applyBaseLayerNotPresent,
              &AbstractStyleTest::applyBaseLayerDifferentStyleCount,
              &AbstractStyleTest::applyTextLayerNotPresent,
              &AbstractStyleTest::applyTextLayerDifferentStyleCount,
              &AbstractStyleTest::applyTextLayerDifferentGlyphCache,
              &AbstractStyleTest::applyTextLayerNoFontManager,
              &AbstractStyleTest::applyTextLayerImagesTextLayerNotPresent,
              &AbstractStyleTest::applyTextLayerImagesNoImporterManager,
              &AbstractStyleTest::applyEventLayerNotPresent,
              &AbstractStyleTest::applySnapLayouterNotPresent});
}

void AbstractStyleTest::debugFeature() {
    Containers::String out;
    Debug{&out} << StyleFeature::BaseLayer << StyleFeature(0xbe);
    CORRADE_COMPARE(out, "Ui::StyleFeature::BaseLayer Ui::StyleFeature(0xbe)\n");
}

void AbstractStyleTest::debugFeatures() {
    Containers::String out;
    Debug{&out} << (StyleFeature::TextLayer|StyleFeature(0xe0)) << StyleFeatures{};
    CORRADE_COMPARE(out, "Ui::StyleFeature::TextLayer|Ui::StyleFeature(0xe0) Ui::StyleFeatures{}\n");
}

void AbstractStyleTest::construct() {
    struct: AbstractStyle {
        StyleFeatures doFeatures() const override { return StyleFeature::BaseLayer; }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }
    } style;
    CORRADE_COMPARE(style.features(), StyleFeature::BaseLayer);
}

void AbstractStyleTest::constructCopy() {
    struct Style: AbstractStyle {
        explicit Style(StyleFeatures features): _features{features} {}

        StyleFeatures doFeatures() const override { return _features; }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }

        StyleFeatures _features;
    };

    Style a{StyleFeature::TextLayer};

    Style b = a;
    CORRADE_COMPARE(b.features(), StyleFeature::TextLayer);

    Style c{StyleFeature::BaseLayer};
    c = b;
    CORRADE_COMPARE(c.features(), StyleFeature::TextLayer);

    CORRADE_VERIFY(std::is_nothrow_move_constructible<Style>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<Style>::value);
}

void AbstractStyleTest::noFeaturesReturned() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractStyle {
        StyleFeatures doFeatures() const override { return {}; }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }
    } style;

    Containers::String out;
    Error redirectError{&out};
    style.features();
    CORRADE_COMPARE(out, "Ui::AbstractStyle::features(): implementation returned an empty set\n");
}

void AbstractStyleTest::styleCount() {
    struct: AbstractStyle {
        StyleFeatures doFeatures() const override {
            return StyleFeature::BaseLayer|StyleFeature::TextLayer;
        }
        UnsignedInt doBaseLayerStyleUniformCount() const override { return 3; }
        UnsignedInt doBaseLayerStyleCount() const override { return 5; }
        UnsignedInt doBaseLayerDynamicStyleCount() const override { return 11; }
        UnsignedInt doTextLayerStyleUniformCount() const override { return 7; }
        UnsignedInt doTextLayerStyleCount() const override { return 9; }
        UnsignedInt doTextLayerEditingStyleUniformCount() const override { return 2; }
        UnsignedInt doTextLayerEditingStyleCount() const override { return 4; }
        UnsignedInt doTextLayerDynamicStyleCount() const override { return 13; }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }
    } style;
    CORRADE_COMPARE(style.baseLayerStyleUniformCount(), 3);
    CORRADE_COMPARE(style.baseLayerStyleCount(), 5);
    CORRADE_COMPARE(style.baseLayerDynamicStyleCount(), 11);
    CORRADE_COMPARE(style.textLayerStyleUniformCount(), 7);
    CORRADE_COMPARE(style.textLayerStyleCount(), 9);
    CORRADE_COMPARE(style.textLayerEditingStyleUniformCount(), 2);
    CORRADE_COMPARE(style.textLayerEditingStyleCount(), 4);
    CORRADE_COMPARE(style.textLayerDynamicStyleCount(), 13);
}

void AbstractStyleTest::styleCountNotSupported() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractStyle {
        StyleFeatures doFeatures() const override { return StyleFeatures{0x10}; }
        UnsignedInt doBaseLayerStyleUniformCount() const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
        UnsignedInt doBaseLayerStyleCount() const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
        UnsignedInt doBaseLayerDynamicStyleCount() const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
        UnsignedInt doTextLayerStyleUniformCount() const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
        UnsignedInt doTextLayerStyleCount() const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
        UnsignedInt doTextLayerEditingStyleUniformCount() const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
        UnsignedInt doTextLayerEditingStyleCount() const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
        UnsignedInt doTextLayerDynamicStyleCount() const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }
    } style;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::String out;
    Error redirectError{&out};
    style.baseLayerStyleUniformCount();
    style.baseLayerStyleCount();
    style.baseLayerDynamicStyleCount();
    style.textLayerStyleUniformCount();
    style.textLayerStyleCount();
    style.textLayerEditingStyleUniformCount();
    style.textLayerEditingStyleCount();
    style.textLayerDynamicStyleCount();
    CORRADE_COMPARE_AS(out,
        "Ui::AbstractStyle::baseLayerStyleUniformCount(): feature not supported\n"
        "Ui::AbstractStyle::baseLayerStyleCount(): feature not supported\n"
        "Ui::AbstractStyle::baseLayerDynamicStyleCount(): feature not supported\n"
        "Ui::AbstractStyle::textLayerStyleUniformCount(): feature not supported\n"
        "Ui::AbstractStyle::textLayerStyleCount(): feature not supported\n"
        "Ui::AbstractStyle::textLayerEditingStyleUniformCount(): feature not supported\n"
        "Ui::AbstractStyle::textLayerEditingStyleCount(): feature not supported\n"
        "Ui::AbstractStyle::textLayerDynamicStyleCount(): feature not supported\n",
        TestSuite::Compare::String);
}

void AbstractStyleTest::styleCountNotImplemented() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractStyle {
        StyleFeatures doFeatures() const override {
            return StyleFeature::BaseLayer|StyleFeature::TextLayer;
        }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }
    } style;

    /* *DynamicStyleCount() and textLayerEditingStyle*Count() has a default
       implementation, tested in styleCountNotImplementedDefaults() below */

    Containers::String out;
    Error redirectError{&out};
    /* The *UniformCount() delegate to *Count() by default, so the assertion
       is the same. Delegation and value propagation tested below. */
    style.baseLayerStyleUniformCount();
    style.baseLayerStyleCount();
    style.textLayerStyleUniformCount();
    style.textLayerStyleCount();
    style.textLayerEditingStyleUniformCount();
    style.textLayerEditingStyleCount();
    CORRADE_COMPARE_AS(out,
        "Ui::AbstractStyle::baseLayerStyleCount(): feature advertised but not implemented\n"
        "Ui::AbstractStyle::baseLayerStyleCount(): feature advertised but not implemented\n"
        "Ui::AbstractStyle::textLayerStyleCount(): feature advertised but not implemented\n"
        "Ui::AbstractStyle::textLayerStyleCount(): feature advertised but not implemented\n",
        TestSuite::Compare::String);
}

void AbstractStyleTest::styleCountNotImplementedDefaults() {
    struct: AbstractStyle {
        StyleFeatures doFeatures() const override {
            return StyleFeature::BaseLayer|StyleFeature::TextLayer;
        }
        UnsignedInt doBaseLayerStyleCount() const override {
            return 17;
        }
        UnsignedInt doTextLayerStyleCount() const override {
            return 35;
        }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }
    } style;

    CORRADE_COMPARE(style.baseLayerDynamicStyleCount(), 0);
    CORRADE_COMPARE(style.textLayerEditingStyleUniformCount(), 0);
    CORRADE_COMPARE(style.textLayerEditingStyleCount(), 0);
    CORRADE_COMPARE(style.textLayerDynamicStyleCount(), 0);

    /* With baseLayerStyleCount() / textLayerStyleCount() not implemented it
       would assert, tested above */
    CORRADE_COMPARE(style.baseLayerStyleUniformCount(), 17);
    CORRADE_COMPARE(style.textLayerStyleUniformCount(), 35);
}

void AbstractStyleTest::setBaseLayerDynamicStyleCount() {
    struct: AbstractStyle {
        StyleFeatures doFeatures() const override {
            return StyleFeature::BaseLayer;
        }
        UnsignedInt doBaseLayerDynamicStyleCount() const override { return 9; }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }
    } style;

    /* By default it returns what the style says */
    CORRADE_COMPARE(style.baseLayerDynamicStyleCount(), 9);

    /* Setting a new value */
    style.setBaseLayerDynamicStyleCount(11);
    CORRADE_COMPARE(style.baseLayerDynamicStyleCount(), 11);

    /* Setting a new but smaller value than before */
    style.setBaseLayerDynamicStyleCount(10);
    CORRADE_COMPARE(style.baseLayerDynamicStyleCount(), 10);

    /* Setting a value smaller than what style says picks the style instead */
    style.setBaseLayerDynamicStyleCount(3);
    CORRADE_COMPARE(style.baseLayerDynamicStyleCount(), 9);
}

void AbstractStyleTest::baseLayerFlags() {
    struct Style: AbstractStyle {
        explicit Style(BaseLayerSharedFlags flags): _flags{flags} {}

        StyleFeatures doFeatures() const override {
            return StyleFeature::BaseLayer|StyleFeature(0x10);
        }
        BaseLayerSharedFlags doBaseLayerFlags() const override {
            return _flags;
        }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }

        private:
            BaseLayerSharedFlags _flags;
    } styleNeither{BaseLayerSharedFlag::NoOutline|BaseLayerSharedFlag::NoRoundedCorners},
      styleNoRoundedCorners{BaseLayerSharedFlag::NoRoundedCorners};

    /* By default it returns what the style says */
    CORRADE_COMPARE(styleNeither.baseLayerFlags(), BaseLayerSharedFlag::NoOutline|BaseLayerSharedFlag::NoRoundedCorners);
    CORRADE_COMPARE(styleNoRoundedCorners.baseLayerFlags(), BaseLayerSharedFlag::NoRoundedCorners);

    /* Adding / clearing no flags doesn't change anything */
    styleNoRoundedCorners.setBaseLayerFlags({}, {});
    CORRADE_COMPARE(styleNoRoundedCorners.baseLayerFlags(), BaseLayerSharedFlag::NoRoundedCorners);

    /* Clearing a flag that isn't there doesn't change anything */
    /** @todo test also adding a flag that is there, once such a flag is
        allowed */
    styleNoRoundedCorners.setBaseLayerFlags({}, BaseLayerSharedFlag::NoOutline);
    CORRADE_COMPARE(styleNoRoundedCorners.baseLayerFlags(), BaseLayerSharedFlag::NoRoundedCorners);

    /* Adding a flag that isn't there updates the style, clearing a flag that
       is there updates it also */
    styleNoRoundedCorners.setBaseLayerFlags(BaseLayerSharedFlag::SubdividedQuads, {});
    styleNeither.setBaseLayerFlags({}, BaseLayerSharedFlag::NoOutline);
    CORRADE_COMPARE(styleNoRoundedCorners.baseLayerFlags(), BaseLayerSharedFlag::SubdividedQuads|BaseLayerSharedFlag::NoRoundedCorners);
    CORRADE_COMPARE(styleNeither.baseLayerFlags(), BaseLayerSharedFlag::NoRoundedCorners);

    /* Adding no flags returns to the previous state */
    styleNeither.setBaseLayerFlags({}, {});
    styleNoRoundedCorners.setBaseLayerFlags({}, {});
    CORRADE_COMPARE(styleNeither.baseLayerFlags(), BaseLayerSharedFlag::NoOutline|BaseLayerSharedFlag::NoRoundedCorners);
    CORRADE_COMPARE(styleNoRoundedCorners.baseLayerFlags(), BaseLayerSharedFlag::NoRoundedCorners);
}

void AbstractStyleTest::baseLayerFlagsNotImplementedDefaults() {
    struct: AbstractStyle {
        StyleFeatures doFeatures() const override {
            return StyleFeature::BaseLayer;
        }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }
    } style;

    CORRADE_COMPARE(style.baseLayerFlags(), BaseLayerSharedFlags{});
}

void AbstractStyleTest::baseLayerFlagsInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Style: AbstractStyle {
        explicit Style(BaseLayerSharedFlags flags): _flags{flags} {}

        StyleFeatures doFeatures() const override {
            return StyleFeature::BaseLayer|StyleFeature(0x10);
        }
        BaseLayerSharedFlags doBaseLayerFlags() const override {
            return _flags;
        }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }

        private:
            BaseLayerSharedFlags _flags;
    } style{{}},
      styleReturnedInvalid{BaseLayerSharedFlag::SubdividedQuads|BaseLayerSharedFlag::Textured|BaseLayerSharedFlag::NoOutline};

    Containers::String out;
    Error redirectError{&out};
    styleReturnedInvalid.baseLayerFlags();
    style.setBaseLayerFlags(BaseLayerSharedFlag::NoOutline|BaseLayerSharedFlag::SubdividedQuads|BaseLayerSharedFlag::Textured, {});
    style.setBaseLayerFlags({}, BaseLayerSharedFlag::NoOutline|BaseLayerSharedFlag::SubdividedQuads|BaseLayerSharedFlag::Textured);
    CORRADE_COMPARE_AS(out,
        "Ui::AbstractStyle::baseLayerFlags(): implementation returned disallowed Ui::BaseLayerSharedFlag::Textured|Ui::BaseLayerSharedFlag::SubdividedQuads\n"
        "Ui::AbstractStyle::setBaseLayerFlags(): Ui::BaseLayerSharedFlag::Textured|Ui::BaseLayerSharedFlag::NoOutline isn't allowed to be added\n"
        "Ui::AbstractStyle::setBaseLayerFlags(): Ui::BaseLayerSharedFlag::Textured|Ui::BaseLayerSharedFlag::SubdividedQuads isn't allowed to be cleared\n",
        TestSuite::Compare::String);
}

void AbstractStyleTest::setTextLayerDynamicStyleCount() {
    struct: AbstractStyle {
        StyleFeatures doFeatures() const override {
            return StyleFeature::TextLayer;
        }
        UnsignedInt doTextLayerDynamicStyleCount() const override { return 9; }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }
    } style;

    /* By default it returns what the style says */
    CORRADE_COMPARE(style.textLayerDynamicStyleCount(), 9);

    /* Setting a new value */
    style.setTextLayerDynamicStyleCount(11);
    CORRADE_COMPARE(style.textLayerDynamicStyleCount(), 11);

    /* Setting a new but smaller value than before */
    style.setTextLayerDynamicStyleCount(10);
    CORRADE_COMPARE(style.textLayerDynamicStyleCount(), 10);

    /* Setting a value smaller than what style says picks the style instead */
    style.setTextLayerDynamicStyleCount(3);
    CORRADE_COMPARE(style.textLayerDynamicStyleCount(), 9);
}

void AbstractStyleTest::textLayerGlyphCacheProperties() {
    struct: AbstractStyle {
        StyleFeatures doFeatures() const override {
            return StyleFeature::TextLayer|StyleFeature(0x10);
        }
        PixelFormat doTextLayerGlyphCacheFormat() const override {
            return PixelFormat::RG32F;
        }
        Vector3i doTextLayerGlyphCacheSize(StyleFeatures features) const override {
            CORRADE_COMPARE(features, StyleFeature::TextLayer|StyleFeature(0x10));
            return {3, 5, 18};
        }
        Vector2i doTextLayerGlyphCachePadding() const override {
            return {2, 4};
        }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }
    } style;
    CORRADE_COMPARE(style.textLayerGlyphCacheFormat(), PixelFormat::RG32F);
    CORRADE_COMPARE(style.textLayerGlyphCacheSize(StyleFeature::TextLayer|StyleFeature(0x10)), (Vector3i{3, 5, 18}));
    CORRADE_COMPARE(style.textLayerGlyphCachePadding(), (Vector2i{2, 4}));
}

void AbstractStyleTest::textLayerGlyphCachePropertiesNotSupported() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractStyle {
        StyleFeatures doFeatures() const override {
            return StyleFeature::BaseLayer;
        }
        PixelFormat doTextLayerGlyphCacheFormat() const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
        Vector3i doTextLayerGlyphCacheSize(StyleFeatures) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
        Vector2i doTextLayerGlyphCachePadding() const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }
    } style;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::String out;
    Error redirectError{&out};
    style.textLayerGlyphCacheFormat();
    style.textLayerGlyphCacheSize(StyleFeature::TextLayer);
    style.textLayerGlyphCachePadding();
    CORRADE_COMPARE_AS(out,
        "Ui::AbstractStyle::textLayerGlyphCacheFormat(): feature not supported\n"
        "Ui::AbstractStyle::textLayerGlyphCacheSize(): feature not supported\n"
        "Ui::AbstractStyle::textLayerGlyphCachePadding(): feature not supported\n",
        TestSuite::Compare::String);
}

void AbstractStyleTest::textLayerGlyphCachePropertiesNotImplemented() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractStyle {
        StyleFeatures doFeatures() const override {
            return StyleFeature::TextLayer;
        }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }
    } style;

    Containers::String out;
    Error redirectError{&out};
    /* textLayerGlyphCacheFormat() and textLayerGlyphCachePadding() have
       defaults, tested below */
    style.textLayerGlyphCacheSize(StyleFeature::TextLayer);
    CORRADE_COMPARE(out, "Ui::AbstractStyle::textLayerGlyphCacheSize(): feature advertised but not implemented\n");
}

void AbstractStyleTest::textLayerGlyphCachePropertiesNotImplementedDefaults() {
    struct: AbstractStyle {
        StyleFeatures doFeatures() const override {
            return StyleFeature::TextLayer;
        }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }
    } style;

    CORRADE_COMPARE(style.textLayerGlyphCacheFormat(), PixelFormat::R8Unorm);
    /* Padding is 1 by default, consistently with Text::AbstractGlyphCache */
    CORRADE_COMPARE(style.textLayerGlyphCachePadding(), Vector2i{1});
    /* textLayerGlyphCacheSize() asserts, tested above */
}

void AbstractStyleTest::textLayerGlyphCacheSizeNoTextFeature() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractStyle {
        StyleFeatures doFeatures() const override {
            return StyleFeature::TextLayer;
        }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }
    } style;

    Containers::String out;
    Error redirectError{&out};
    style.textLayerGlyphCacheSize(StyleFeature::BaseLayer|StyleFeature(0x40));
    CORRADE_COMPARE(out, "Ui::AbstractStyle::textLayerGlyphCacheSize(): expected a superset of Ui::StyleFeature::TextLayer but got Ui::StyleFeature::BaseLayer|Ui::StyleFeature(0x40)\n");
}

void AbstractStyleTest::textLayerGlyphCacheSizeFeaturesNotSupported() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractStyle {
        StyleFeatures doFeatures() const override {
            return StyleFeature::TextLayer;
        }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }
    } style;

    Containers::String out;
    Error redirectError{&out};
    style.textLayerGlyphCacheSize(StyleFeature::TextLayer|StyleFeature::BaseLayer);
    CORRADE_COMPARE(out, "Ui::AbstractStyle::textLayerGlyphCacheSize(): Ui::StyleFeature::BaseLayer|Ui::StyleFeature::TextLayer not a subset of supported Ui::StyleFeature::TextLayer\n");
}

void AbstractStyleTest::setTextLayerGlyphCacheSize() {
    struct: AbstractStyle {
        StyleFeatures doFeatures() const override {
            return StyleFeature::TextLayer|StyleFeature::TextLayerImages;
        }
        Vector3i doTextLayerGlyphCacheSize(StyleFeatures features) const override {
            if(features >= StyleFeature::TextLayerImages)
                return {256, 128, 32};
            return {16, 32, 8};
        }
        Vector2i doTextLayerGlyphCachePadding() const override {
            return {4, 2};
        }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }
    } style;

    /* By default it returns what the style says */
    CORRADE_COMPARE(style.textLayerGlyphCacheSize(StyleFeature::TextLayer|StyleFeature::TextLayerImages), (Vector3i{256, 128, 32}));
    CORRADE_COMPARE(style.textLayerGlyphCacheSize(StyleFeature::TextLayer), (Vector3i{16, 32, 8}));
    CORRADE_COMPARE(style.textLayerGlyphCachePadding(), (Vector2i{4, 2}));

    /* Setting a new value */
    style.setTextLayerGlyphCacheSize({48, 56, 12}, {6, 8});
    CORRADE_COMPARE(style.textLayerGlyphCacheSize(StyleFeature::TextLayer), (Vector3i{48, 56, 12}));
    CORRADE_COMPARE(style.textLayerGlyphCachePadding(), (Vector2i{6, 8}));

    /* It doesn't get overwritten or forgotten when asking for a size with
       different features */
    CORRADE_COMPARE(style.textLayerGlyphCacheSize(StyleFeature::TextLayer|StyleFeature::TextLayerImages), (Vector3i{256, 128, 32}));
    CORRADE_COMPARE(style.textLayerGlyphCacheSize(StyleFeature::TextLayer), (Vector3i{48, 56, 12}));

    /* Setting a new but smaller value than before */
    style.setTextLayerGlyphCacheSize({24, 48, 10}, {5, 3});
    CORRADE_COMPARE(style.textLayerGlyphCacheSize(StyleFeature::TextLayer), (Vector3i{24, 48, 10}));
    CORRADE_COMPARE(style.textLayerGlyphCachePadding(), (Vector2i{5, 3}));

    /* Setting a value smaller than what style says picks the style instead */
    style.setTextLayerGlyphCacheSize({}, {});
    CORRADE_COMPARE(style.textLayerGlyphCacheSize(StyleFeature::TextLayer), (Vector3i{16, 32, 8}));
    CORRADE_COMPARE(style.textLayerGlyphCachePadding(), (Vector2i{4, 2}));

    /* Setting a new value only picks the dimensions that are actually
       larger */
    style.setTextLayerGlyphCacheSize({12, 33, 6}, {5, 1});
    CORRADE_COMPARE(style.textLayerGlyphCacheSize(StyleFeature::TextLayer), (Vector3i{16, 33, 8}));
    CORRADE_COMPARE(style.textLayerGlyphCachePadding(), (Vector2i{5, 2}));

    style.setTextLayerGlyphCacheSize({17, 24, 6}, {3, 3});
    CORRADE_COMPARE(style.textLayerGlyphCacheSize(StyleFeature::TextLayer), (Vector3i{17, 32, 8}));
    CORRADE_COMPARE(style.textLayerGlyphCachePadding(), (Vector2i{4, 3}));

    style.setTextLayerGlyphCacheSize({12, 24, 12});
    CORRADE_COMPARE(style.textLayerGlyphCacheSize(StyleFeature::TextLayer), (Vector3i{16, 32, 12}));
    CORRADE_COMPARE(style.textLayerGlyphCachePadding(), (Vector2i{4, 2}));
}

void AbstractStyleTest::apply() {
    auto&& data = ApplyData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct LayerSharedBase: BaseLayer::Shared {
        explicit LayerSharedBase(const Configuration& configuration): BaseLayer::Shared{configuration} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } sharedBase{BaseLayer::Shared::Configuration{3, 5}
        .setDynamicStyleCount(11)
    };

    struct LayerBase: BaseLayer {
        explicit LayerBase(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    };

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R16F, {16, 24, 2}, {3, 1}};

    struct LayerSharedText: TextLayer::Shared {
        explicit LayerSharedText(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } sharedText{cache, TextLayer::Shared::Configuration{2, 4}
        .setEditingStyleCount(6, 7)
        .setDynamicStyleCount(9)
    };

    struct LayerText: TextLayer {
        explicit LayerText(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    };

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    ui.setSize({200, 300});
    if(data.baseLayerPresent)
        ui.setBaseLayerInstance(Containers::pointer<LayerBase>(ui.createLayer(), sharedBase));
    if(data.textLayerPresent)
        ui.setTextLayerInstance(Containers::pointer<LayerText>(ui.createLayer(), sharedText));
    if(data.eventLayerPresent)
        ui.setEventLayerInstance(Containers::pointer<EventLayer>(ui.createLayer()));
    if(data.snapLayouterPresent)
        ui.setSnapLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));

    Int applyCalled = 0;
    struct Style: AbstractStyle {
        Style(Int& applyCalled, StyleFeatures expectedFeatures, bool succeed): _applyCalled(applyCalled), _expectedFeatures{expectedFeatures}, _succeed{succeed} {}

        StyleFeatures doFeatures() const override {
            return StyleFeature::BaseLayer|StyleFeature::TextLayer|StyleFeature::TextLayerImages|StyleFeature::EventLayer|StyleFeature::SnapLayouter;
        }
        UnsignedInt doBaseLayerStyleUniformCount() const override { return 3; }
        UnsignedInt doBaseLayerStyleCount() const override { return 5; }
        UnsignedInt doBaseLayerDynamicStyleCount() const override { return 11; }
        UnsignedInt doTextLayerStyleUniformCount() const override { return 2; }
        UnsignedInt doTextLayerStyleCount() const override { return 4; }
        UnsignedInt doTextLayerEditingStyleUniformCount() const override { return 6; }
        UnsignedInt doTextLayerEditingStyleCount() const override { return 7; }
        UnsignedInt doTextLayerDynamicStyleCount() const override { return 9; }
        PixelFormat doTextLayerGlyphCacheFormat() const override { return PixelFormat::R16F; }
        Vector3i doTextLayerGlyphCacheSize(StyleFeatures) const override { return {16, 24, 2}; }
        Vector2i doTextLayerGlyphCachePadding() const override { return {3, 1}; }
        bool doApply(UserInterface&, StyleFeatures features, PluginManager::Manager<Trade::AbstractImporter>* importerManager, PluginManager::Manager<Text::AbstractFont>* fontManager) const override {
            CORRADE_COMPARE(features, _expectedFeatures);
            if(features >= StyleFeature::TextLayer)
                CORRADE_VERIFY(fontManager);
            if(features >= StyleFeature::TextLayerImages)
                CORRADE_VERIFY(importerManager);
            ++_applyCalled;
            return _succeed;
        }

        Int& _applyCalled;
        StyleFeatures _expectedFeatures;
        bool _succeed;
    } style{applyCalled, data.features, data.succeed};

    CORRADE_COMPARE(style.apply(ui, data.features, data.features >= StyleFeature::TextLayerImages ? &_importerManager : nullptr, data.features >= StyleFeature::TextLayer ? &_fontManager : nullptr), data.succeed);
    CORRADE_COMPARE(applyCalled, 1);
}

void AbstractStyleTest::applyNoFeatures() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};

    struct: AbstractStyle {
        StyleFeatures doFeatures() const override { return StyleFeature::TextLayer; }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } style;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::String out;
    Error redirectError{&out};
    style.apply(ui, {}, nullptr, nullptr);
    CORRADE_COMPARE(out, "Ui::AbstractStyle::apply(): no features specified\n");
}

void AbstractStyleTest::applyFeaturesNotSupported() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};

    struct: AbstractStyle {
        StyleFeatures doFeatures() const override { return StyleFeature::TextLayer; }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } style;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::String out;
    Error redirectError{&out};
    style.apply(ui, StyleFeature::TextLayer|StyleFeature::BaseLayer, nullptr, nullptr);
    CORRADE_COMPARE(out, "Ui::AbstractStyle::apply(): Ui::StyleFeature::BaseLayer|Ui::StyleFeature::TextLayer not a subset of supported Ui::StyleFeature::TextLayer\n");
}

void AbstractStyleTest::applyNoSizeSet() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};

    struct: AbstractStyle {
        StyleFeatures doFeatures() const override { return StyleFeature::TextLayer; }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } style;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::String out;
    Error redirectError{&out};
    style.apply(ui, StyleFeature::TextLayer, nullptr, nullptr);
    CORRADE_COMPARE(out, "Ui::AbstractStyle::apply(): user interface size wasn't set\n");
}

void AbstractStyleTest::applyBaseLayerNotPresent() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32}};

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{1, 3}};

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    };

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    ui.setSize({200, 300})
      .setTextLayerInstance(Containers::pointer<Layer>(ui.createLayer(), shared))
      .setEventLayerInstance(Containers::pointer<EventLayer>(ui.createLayer()));

    struct: AbstractStyle {
        StyleFeatures doFeatures() const override { return StyleFeature::BaseLayer; }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } style;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::String out;
    Error redirectError{&out};
    style.apply(ui, StyleFeature::BaseLayer, nullptr, nullptr);
    CORRADE_COMPARE(out, "Ui::AbstractStyle::apply(): base layer not present in the user interface\n");
}

void AbstractStyleTest::applyBaseLayerDifferentStyleCount() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(const Configuration& configuration): BaseLayer::Shared{configuration} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } shared{BaseLayer::Shared::Configuration{3, 5}
        .setDynamicStyleCount(11)
    };

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    };

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    ui.setSize({200, 300})
      .setBaseLayerInstance(Containers::pointer<Layer>(ui.createLayer(), shared));

    struct Style: AbstractStyle {
        Style(UnsignedInt styleUniformCount, UnsignedInt styleCount, UnsignedInt dynamicStyleCount): _styleUniformCount{styleUniformCount}, _styleCount{styleCount}, _dynamicStyleCount{dynamicStyleCount} {}

        StyleFeatures doFeatures() const override { return StyleFeature::BaseLayer; }
        UnsignedInt doBaseLayerStyleUniformCount() const override { return _styleUniformCount; }
        UnsignedInt doBaseLayerStyleCount() const override { return _styleCount; }
        UnsignedInt doBaseLayerDynamicStyleCount() const override { return _dynamicStyleCount; }
        bool doApply(UserInterface& ui, StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL_IF(
                ui.baseLayer().shared().styleCount() != _styleCount ||
                ui.baseLayer().shared().styleUniformCount() != _styleUniformCount ||
                ui.baseLayer().shared().dynamicStyleCount() < _dynamicStyleCount,
                "This shouldn't get called.");
            return true;
        }

        UnsignedInt _styleUniformCount, _styleCount, _dynamicStyleCount;
    };

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    /* Applying a style with a smaller or equal dynamic style count is
       alright */
    CORRADE_VERIFY(Style{3, 5, 11}
        .apply(ui, StyleFeature::BaseLayer, nullptr, nullptr));
    CORRADE_VERIFY(Style{3, 5, 10}
        .apply(ui, StyleFeature::BaseLayer, nullptr, nullptr));

    Containers::String out;
    Error redirectError{&out};
    Style{4, 5, 11}
        .apply(ui, StyleFeature::BaseLayer, nullptr, nullptr);
    Style{3, 4, 11}
        .apply(ui, StyleFeature::BaseLayer, nullptr, nullptr);
    Style{3, 5, 12}
        .apply(ui, StyleFeature::BaseLayer, nullptr, nullptr);
    CORRADE_COMPARE_AS(out,
        "Ui::AbstractStyle::apply(): style wants 4 uniforms, 5 styles and at least 11 dynamic styles but the base layer has 3, 5 and 11\n"
        "Ui::AbstractStyle::apply(): style wants 3 uniforms, 4 styles and at least 11 dynamic styles but the base layer has 3, 5 and 11\n"
        "Ui::AbstractStyle::apply(): style wants 3 uniforms, 5 styles and at least 12 dynamic styles but the base layer has 3, 5 and 11\n",
        TestSuite::Compare::String);
}

void AbstractStyleTest::applyTextLayerNotPresent() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(const Configuration& configuration): BaseLayer::Shared{configuration} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } shared{BaseLayer::Shared::Configuration{3, 5}};

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, LayerShared& shared): BaseLayer{handle, shared} {}
    };

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    ui.setSize({200, 300})
      .setBaseLayerInstance(Containers::pointer<Layer>(ui.createLayer(), shared))
      .setEventLayerInstance(Containers::pointer<EventLayer>(ui.createLayer()));

    struct: AbstractStyle {
        StyleFeatures doFeatures() const override { return StyleFeature::TextLayer; }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } style;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::String out;
    Error redirectError{&out};
    style.apply(ui, StyleFeature::TextLayer, nullptr, nullptr);
    CORRADE_COMPARE(out, "Ui::AbstractStyle::apply(): text layer not present in the user interface\n");
}

void AbstractStyleTest::applyTextLayerDifferentStyleCount() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, Vector2i{16}};

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{3, 5}
        .setEditingStyleCount(7, 2)
        .setDynamicStyleCount(11)
    };

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    };

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    ui.setSize({200, 300})
      .setTextLayerInstance(Containers::pointer<Layer>(ui.createLayer(), shared));

    struct Style: AbstractStyle {
        Style(UnsignedInt styleUniformCount, UnsignedInt styleCount, UnsignedInt editingStyleUniformCount, UnsignedInt editingStyleCount, UnsignedInt dynamicStyleCount): _styleUniformCount{styleUniformCount}, _styleCount{styleCount}, _editingStyleUniformCount{editingStyleUniformCount}, _editingStyleCount{editingStyleCount}, _dynamicStyleCount{dynamicStyleCount} {}

        StyleFeatures doFeatures() const override { return StyleFeature::TextLayer; }
        UnsignedInt doTextLayerStyleUniformCount() const override { return _styleUniformCount; }
        UnsignedInt doTextLayerStyleCount() const override { return _styleCount; }
        UnsignedInt doTextLayerEditingStyleUniformCount() const override { return _editingStyleUniformCount; }
        UnsignedInt doTextLayerEditingStyleCount() const override { return _editingStyleCount; }
        UnsignedInt doTextLayerDynamicStyleCount() const override { return _dynamicStyleCount; }
        Vector3i doTextLayerGlyphCacheSize(StyleFeatures) const override { return {16, 16, 1}; }
        bool doApply(UserInterface& ui, StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL_IF(
                ui.textLayer().shared().styleCount() != _styleCount ||
                ui.textLayer().shared().styleUniformCount() != _styleUniformCount ||
                ui.textLayer().shared().editingStyleCount() != _editingStyleCount ||
                ui.textLayer().shared().editingStyleUniformCount() != _editingStyleUniformCount ||
                ui.textLayer().shared().dynamicStyleCount() < _dynamicStyleCount,
                "This shouldn't get called.");
            return true;
        }

        UnsignedInt _styleUniformCount, _styleCount, _editingStyleUniformCount, _editingStyleCount, _dynamicStyleCount;
    };

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    /* Applying a style with a smaller or equal dynamic style count is
       alright */
    CORRADE_VERIFY(Style{3, 5, 7, 2, 11}
        .apply(ui, StyleFeature::TextLayer, nullptr, &_fontManager));
    CORRADE_VERIFY(Style{3, 5, 7, 2, 10}
        .apply(ui, StyleFeature::TextLayer, nullptr, &_fontManager));

    Containers::String out;
    Error redirectError{&out};
    Style{4, 5, 7, 2, 11}
        .apply(ui, StyleFeature::TextLayer, nullptr, nullptr);
    Style{3, 4, 7, 2, 11}
        .apply(ui, StyleFeature::TextLayer, nullptr, nullptr);
    Style{3, 5, 8, 2, 11}
        .apply(ui, StyleFeature::TextLayer, nullptr, nullptr);
    Style{3, 5, 7, 1, 11}
        .apply(ui, StyleFeature::TextLayer, nullptr, nullptr);
    Style{3, 5, 7, 2, 12}
        .apply(ui, StyleFeature::TextLayer, nullptr, nullptr);
    CORRADE_COMPARE_AS(out,
        "Ui::AbstractStyle::apply(): style wants 4 uniforms, 5 styles, 7 editing uniforms, 2 editing styles and at least 11 dynamic styles but the text layer has 3, 5, 7, 2 and 11\n"
        "Ui::AbstractStyle::apply(): style wants 3 uniforms, 4 styles, 7 editing uniforms, 2 editing styles and at least 11 dynamic styles but the text layer has 3, 5, 7, 2 and 11\n"
        "Ui::AbstractStyle::apply(): style wants 3 uniforms, 5 styles, 8 editing uniforms, 2 editing styles and at least 11 dynamic styles but the text layer has 3, 5, 7, 2 and 11\n"
        "Ui::AbstractStyle::apply(): style wants 3 uniforms, 5 styles, 7 editing uniforms, 1 editing styles and at least 11 dynamic styles but the text layer has 3, 5, 7, 2 and 11\n"
        "Ui::AbstractStyle::apply(): style wants 3 uniforms, 5 styles, 7 editing uniforms, 2 editing styles and at least 12 dynamic styles but the text layer has 3, 5, 7, 2 and 11\n",
        TestSuite::Compare::String);
}

void AbstractStyleTest::applyTextLayerDifferentGlyphCache() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::RG16F, {3, 5, 2}, {4, 1}};

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{1, 1}};

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    };

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    ui.setSize({200, 300})
      .setTextLayerInstance(Containers::pointer<Layer>(ui.createLayer(), shared));

    struct Style: AbstractStyle {
        Style(PixelFormat format, const Vector3i& size, const Vector2i& padding): _format{format}, _size{size}, _padding{padding} {}

        StyleFeatures doFeatures() const override { return StyleFeature::TextLayer; }
        UnsignedInt doTextLayerStyleCount() const override { return 1; }
        PixelFormat doTextLayerGlyphCacheFormat() const override { return _format; }
        Vector3i doTextLayerGlyphCacheSize(StyleFeatures) const override { return _size; }
        Vector2i doTextLayerGlyphCachePadding() const override { return _padding; }
        bool doApply(UserInterface& ui, StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL_IF(
                (ui.textLayer().shared().glyphCache().size() < _size).any() ||
                (ui.textLayer().shared().glyphCache().padding() < _padding).any(),
                "This shouldn't get called.");
            return true;
        }

        PixelFormat _format;
        Vector3i _size;
        Vector2i _padding;
    };

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    /* Applying a style with a smaller or equal size or padding is alright */
    CORRADE_VERIFY(Style{PixelFormat::RG16F, {3, 5, 2}, {4, 1}}
        .apply(ui, StyleFeature::TextLayer, nullptr, &_fontManager));
    CORRADE_VERIFY(Style{PixelFormat::RG16F, {3, 5, 2}, {4, 0}}
        .apply(ui, StyleFeature::TextLayer, nullptr, &_fontManager));
    CORRADE_VERIFY(Style{PixelFormat::RG16F, {3, 5, 2}, {3, 1}}
        .apply(ui, StyleFeature::TextLayer, nullptr, &_fontManager));
    CORRADE_VERIFY(Style{PixelFormat::RG16F, {3, 5, 1}, {4, 1}}
        .apply(ui, StyleFeature::TextLayer, nullptr, &_fontManager));
    CORRADE_VERIFY(Style{PixelFormat::RG16F, {3, 4, 2}, {4, 1}}
        .apply(ui, StyleFeature::TextLayer, nullptr, &_fontManager));
    CORRADE_VERIFY(Style{PixelFormat::RG16F, {2, 5, 2}, {4, 1}}
        .apply(ui, StyleFeature::TextLayer, nullptr, &_fontManager));

    Containers::String out;
    Error redirectError{&out};
    Style{PixelFormat::R8Unorm, {3, 5, 2}, {4, 1}}
        .apply(ui, StyleFeature::TextLayer, nullptr, nullptr);
    Style{PixelFormat::RG16F, {4, 5, 2}, {4, 1}}
        .apply(ui, StyleFeature::TextLayer, nullptr, nullptr);
    Style{PixelFormat::RG16F, {3, 6, 2}, {4, 1}}
        .apply(ui, StyleFeature::TextLayer, nullptr, nullptr);
    Style{PixelFormat::RG16F, {3, 5, 3}, {4, 1}}
        .apply(ui, StyleFeature::TextLayer, nullptr, nullptr);
    Style{PixelFormat::RG16F, {3, 5, 2}, {5, 1}}
        .apply(ui, StyleFeature::TextLayer, nullptr, nullptr);
    Style{PixelFormat::RG16F, {3, 5, 2}, {4, 2}}
        .apply(ui, StyleFeature::TextLayer, nullptr, nullptr);
    CORRADE_COMPARE_AS(out,
        "Ui::AbstractStyle::apply(): style wants a PixelFormat::R8Unorm glyph cache of size at least {3, 5, 2} and padding at least {4, 1} but the text layer has PixelFormat::RG16F, {3, 5, 2} and {4, 1}\n"
       "Ui::AbstractStyle::apply(): style wants a PixelFormat::RG16F glyph cache of size at least {4, 5, 2} and padding at least {4, 1} but the text layer has PixelFormat::RG16F, {3, 5, 2} and {4, 1}\n"
       "Ui::AbstractStyle::apply(): style wants a PixelFormat::RG16F glyph cache of size at least {3, 6, 2} and padding at least {4, 1} but the text layer has PixelFormat::RG16F, {3, 5, 2} and {4, 1}\n"
       "Ui::AbstractStyle::apply(): style wants a PixelFormat::RG16F glyph cache of size at least {3, 5, 3} and padding at least {4, 1} but the text layer has PixelFormat::RG16F, {3, 5, 2} and {4, 1}\n"
       "Ui::AbstractStyle::apply(): style wants a PixelFormat::RG16F glyph cache of size at least {3, 5, 2} and padding at least {5, 1} but the text layer has PixelFormat::RG16F, {3, 5, 2} and {4, 1}\n"
       "Ui::AbstractStyle::apply(): style wants a PixelFormat::RG16F glyph cache of size at least {3, 5, 2} and padding at least {4, 2} but the text layer has PixelFormat::RG16F, {3, 5, 2} and {4, 1}\n",
        TestSuite::Compare::String);
}

void AbstractStyleTest::applyTextLayerNoFontManager() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {16, 16}};

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{1, 1}};

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    };

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    ui.setSize({200, 300})
      .setTextLayerInstance(Containers::pointer<Layer>(ui.createLayer(), shared));

    struct: AbstractStyle {
        StyleFeatures doFeatures() const override { return StyleFeature::TextLayer; }
        Vector3i doTextLayerGlyphCacheSize(StyleFeatures) const override { return {16, 16, 1}; }
        UnsignedInt doTextLayerStyleCount() const override { return 1; }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } style;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::String out;
    Error redirectError{&out};
    style.apply(ui, StyleFeature::TextLayer, &_importerManager, nullptr);
    CORRADE_COMPARE(out, "Ui::AbstractStyle::apply(): fontManager has to be specified for applying a text layer style\n");
}

void AbstractStyleTest::applyTextLayerImagesTextLayerNotPresent() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    ui.setSize({200, 300});

    struct: AbstractStyle {
        StyleFeatures doFeatures() const override { return StyleFeature::TextLayerImages; }
        Vector3i doTextLayerGlyphCacheSize(StyleFeatures) const override { return {16, 16, 1}; }
        UnsignedInt doTextLayerStyleCount() const override { return 1; }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } style;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::String out;
    Error redirectError{&out};
    style.apply(ui, StyleFeature::TextLayerImages, nullptr, nullptr);
    CORRADE_COMPARE(out, "Ui::AbstractStyle::apply(): text layer not present in the user interface\n");
}

void AbstractStyleTest::applyTextLayerImagesNoImporterManager() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {16, 16}};

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{1}};

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    };

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    ui.setSize({200, 300})
      .setTextLayerInstance(Containers::pointer<Layer>(ui.createLayer(), shared));

    struct: AbstractStyle {
        StyleFeatures doFeatures() const override { return StyleFeature::TextLayerImages; }
        Vector3i doTextLayerGlyphCacheSize(StyleFeatures) const override { return {16, 16, 1}; }
        UnsignedInt doTextLayerStyleCount() const override { return 1; }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } style;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::String out;
    Error redirectError{&out};
    style.apply(ui, StyleFeature::TextLayerImages, nullptr, &_fontManager);
    CORRADE_COMPARE(out, "Ui::AbstractStyle::apply(): importerManager has to be specified for applying text layer style images\n");
}

void AbstractStyleTest::applyEventLayerNotPresent() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct LayerSharedBase: BaseLayer::Shared {
        explicit LayerSharedBase(const Configuration& configuration): BaseLayer::Shared{configuration} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } sharedBase{BaseLayer::Shared::Configuration{3, 5}};

    struct LayerBase: BaseLayer {
        explicit LayerBase(LayerHandle handle, LayerSharedBase& shared): BaseLayer{handle, shared} {}
    };

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32}};

    struct LayerSharedText: TextLayer::Shared {
        explicit LayerSharedText(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } sharedText{cache, TextLayer::Shared::Configuration{1, 3}};

    struct LayerText: TextLayer {
        explicit LayerText(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    };

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    ui.setSize({200, 300})
      .setBaseLayerInstance(Containers::pointer<LayerBase>(ui.createLayer(), sharedBase))
      .setTextLayerInstance(Containers::pointer<LayerText>(ui.createLayer(), sharedText));

    struct: AbstractStyle {
        StyleFeatures doFeatures() const override { return StyleFeature::EventLayer; }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } style;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::String out;
    Error redirectError{&out};
    style.apply(ui, StyleFeature::EventLayer, nullptr, nullptr);
    CORRADE_COMPARE(out, "Ui::AbstractStyle::apply(): event layer not present in the user interface\n");
}

void AbstractStyleTest::applySnapLayouterNotPresent() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct LayerSharedBase: BaseLayer::Shared {
        explicit LayerSharedBase(const Configuration& configuration): BaseLayer::Shared{configuration} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } sharedBase{BaseLayer::Shared::Configuration{3, 5}};

    struct LayerBase: BaseLayer {
        explicit LayerBase(LayerHandle handle, LayerSharedBase& shared): BaseLayer{handle, shared} {}
    };

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    ui.setSize({200, 300})
      .setBaseLayerInstance(Containers::pointer<LayerBase>(ui.createLayer(), sharedBase))
      .setEventLayerInstance(Containers::pointer<EventLayer>(ui.createLayer()));

    struct: AbstractStyle {
        StyleFeatures doFeatures() const override { return StyleFeature::SnapLayouter; }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } style;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::String out;
    Error redirectError{&out};
    style.apply(ui, StyleFeature::SnapLayouter, nullptr, nullptr);
    CORRADE_COMPARE(out, "Ui::AbstractStyle::apply(): snap layouter not present in the user interface\n");
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::AbstractStyleTest)
