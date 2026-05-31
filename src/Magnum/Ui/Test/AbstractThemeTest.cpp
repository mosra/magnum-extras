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

#include <Corrade/Containers/ArrayView.h>
#include <Corrade/Containers/String.h>
#include <Corrade/PluginManager/Manager.h>
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/String.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/Text/AbstractFont.h>
#include <Magnum/Text/AbstractGlyphCache.h>

#include "Magnum/Ui/AbstractTheme.h"
#include "Magnum/Ui/BaseLayer.h"
#include "Magnum/Ui/BaseLayerAnimator.h"
#include "Magnum/Ui/DataLayer.h"
#include "Magnum/Ui/EventLayer.h"
#include "Magnum/Ui/GenericLayouter.h"
#include "Magnum/Ui/LayoutLayer.h"
#include "Magnum/Ui/NodeAnimator.h"
#include "Magnum/Ui/SnapLayouter.h"
#include "Magnum/Ui/TextLayer.h"
#include "Magnum/Ui/TextLayerAnimator.h"
#include "Magnum/Ui/UserInterface.h"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct AbstractThemeTest: TestSuite::Tester {
    explicit AbstractThemeTest();

    void debugFeature();
    void debugFeatures();

    void construct();
    void constructCopy();

    void noFeaturesReturned();

    void backgroundLayer();
    void backgroundLayerNotSupported();
    void backgroundLayerNotImplemented();
    void backgroundLayerNotImplementedDefaults();

    void backgroundLayerDynamicStyleCountInvalid();
    void setBackgroundLayerDynamicStyleCount();

    void backgroundLayerFlags();
    void backgroundLayerFlagsInvalid();

    void baseLayer();
    void baseLayerNotSupported();
    void baseLayerNotImplemented();
    void baseLayerNotImplementedDefaults();

    void baseLayerDynamicStyleCountInvalid();
    void setBaseLayerDynamicStyleCount();

    void baseLayerFlags();
    void baseLayerFlagsInvalid();

    void textLayer();
    void textLayerNotSupported();
    void textLayerNotImplemented();
    void textLayerNotImplementedDefaults();

    void textLayerDynamicStyleCountInvalid();
    void setTextLayerDynamicStyleCount();

    void textLayerGlyphCacheSizeNoTextFeature();
    void textLayerGlyphCacheSizeFeaturesNotSupported();
    void setTextLayerGlyphCacheSize();

    void layoutLayer();
    void layoutLayerNotSupported();
    void layoutLayerNotImplemented();
    /* No defaults for not implemented layout layer properties yet */

    /* SnapLayouter, GenericLayouter and NodeAnimations have no getters or
       setters to test, verified only for apply() below */

    void apply();
    void applyNoSizeSet();
    void applyNoFeatures();
    void applyFeaturesNotSupported();
    void applyDataLayerNotPresent();
    void applyBackgroundLayerNotPresent();
    void applyBackgroundLayerDifferentStyleCount();
    void applyBackgroundLayerStyleAnimatorNotPresent();
    void applyBaseLayerNotPresent();
    void applyBaseLayerDifferentStyleCount();
    void applyBaseLayerStyleAnimatorNotPresent();
    void applyTextLayerNotPresent();
    void applyTextLayerDifferentStyleCount();
    void applyTextLayerDifferentGlyphCache();
    void applyTextLayerNoFontManager();
    void applyTextLayerStyleAnimatorNotPresent();
    void applyEventLayerNotPresent();
    void applyLayoutLayerNotPresent();
    void applyLayoutLayerDifferentStyleCount();
    void applySnapLayouterNotPresent();
    void applyGenericLayouterNotPresent();
    void applyNodeAnimatorNotPresent();

    private:
        PluginManager::Manager<Text::AbstractFont> _fontManager;
};

const struct {
    const char* name;
    bool dataLayerPresent;
    bool backgroundLayerPresent, backgroundLayerStyleAnimatorPresent;
    bool baseLayerPresent, baseLayerStyleAnimatorPresent;
    bool textLayerPresent, textLayerStyleAnimatorPresent;
    bool eventLayerPresent, layoutLayerPresent;
    bool snapLayouterPresent, genericLayouterPresent, nodeAnimatorPresent;
    ThemeFeatures features;
    bool succeed;
} ApplyData[]{
    {"data layer only",
        true,
        false, false,
        false, false,
        false, false,
        false, false,
        false, false, false,
        ThemeFeature::DataLayer, true},
    {"background layer only",
        false,
        true, false,
        false, false,
        false, false,
        false, false,
        false, false, false,
        ThemeFeature::BackgroundLayer, true},
    {"background layer animations only",
        false,
        true, true,
        false, false,
        false, false,
        false, false,
        false, false, false,
        ThemeFeature::BackgroundLayerAnimations, true},
    {"background layer + base layer animations",
        false,
        true, true,
        false, false,
        false, false,
        false, false,
        false, false, false,
        ThemeFeature::BackgroundLayer|ThemeFeature::BackgroundLayerAnimations, true},
    {"base layer only",
        false,
        false, false,
        true, false,
        false, false,
        false, false,
        false, false, false,
        ThemeFeature::BaseLayer, true},
    {"base layer animations only",
        false,
        false, false,
        true, true,
        false, false,
        false, false,
        false, false, false,
        ThemeFeature::BaseLayerAnimations, true},
    {"base layer + base layer animations",
        false,
        false, false,
        true, true,
        false, false,
        false, false,
        false, false, false,
        ThemeFeature::BaseLayer|ThemeFeature::BaseLayerAnimations, true},
    {"text layer only",
        false,
        false, false,
        false, false,
        true, false,
        false, false,
        false, false, false,
        ThemeFeature::TextLayer, true},
    {"text layer animations only",
        false,
        false, false,
        false, false,
        true, true,
        false, false,
        false, false, false,
        ThemeFeature::TextLayerAnimations, true},
    {"text layer + text layer animations",
        false,
        false, false,
        false, false,
        true, true,
        false, false,
        false, false, false,
        ThemeFeature::TextLayer|ThemeFeature::TextLayerAnimations, true},
    {"event layer only",
        false,
        false, false,
        false, false,
        false, false,
        true, false,
        false, false, false,
        ThemeFeature::EventLayer, true},
    {"layout layer only",
        false,
        false, false,
        false, false,
        false, false,
        false, true,
        false, false, false,
        ThemeFeature::LayoutLayer, true},
    {"snap layouter only",
        false,
        false, false,
        false, false,
        false, false,
        false, false,
        true, false, false,
        ThemeFeature::SnapLayouter, true},
    {"generic layouter only",
        false,
        false, false,
        false, false,
        false, false,
        false, false,
        false, true, false,
        ThemeFeature::GenericLayouter, true},
    {"node animations only",
        false,
        false, false,
        false, false,
        false, false,
        false, false,
        false, false, true,
        ThemeFeature::NodeAnimations, true},
    {"everything except base layer (and its animations)",
        true,
        true, true,
        false, false,
        true, true,
        true, true,
        true, true, true,
        ~(ThemeFeature::BaseLayer|ThemeFeature::BaseLayerAnimations), true},
    {"everything",
        true,
        true, true,
        true, true,
        true, true,
        true, true,
        true, true, true,
        ~ThemeFeatures{}, true},
    {"application failed",
        false,
        false, false,
        true, false,
        false, false,
        false, false,
        false, false, false,
        ThemeFeature::BaseLayer, false}
};

AbstractThemeTest::AbstractThemeTest() {
    addTests({&AbstractThemeTest::debugFeature,
              &AbstractThemeTest::debugFeatures,

              &AbstractThemeTest::construct,
              &AbstractThemeTest::constructCopy,

              &AbstractThemeTest::noFeaturesReturned,

              &AbstractThemeTest::backgroundLayer,
              &AbstractThemeTest::backgroundLayerNotSupported,
              &AbstractThemeTest::backgroundLayerNotImplemented,
              &AbstractThemeTest::backgroundLayerNotImplementedDefaults,

              &AbstractThemeTest::backgroundLayerDynamicStyleCountInvalid,
              &AbstractThemeTest::setBackgroundLayerDynamicStyleCount,

              &AbstractThemeTest::backgroundLayerFlags,
              &AbstractThemeTest::backgroundLayerFlagsInvalid,

              &AbstractThemeTest::baseLayer,
              &AbstractThemeTest::baseLayerNotSupported,
              &AbstractThemeTest::baseLayerNotImplemented,
              &AbstractThemeTest::baseLayerNotImplementedDefaults,

              &AbstractThemeTest::baseLayerDynamicStyleCountInvalid,
              &AbstractThemeTest::setBaseLayerDynamicStyleCount,

              &AbstractThemeTest::baseLayerFlags,
              &AbstractThemeTest::baseLayerFlagsInvalid,

              &AbstractThemeTest::textLayer,
              &AbstractThemeTest::textLayerNotSupported,
              &AbstractThemeTest::textLayerNotImplemented,
              &AbstractThemeTest::textLayerNotImplementedDefaults,

              &AbstractThemeTest::textLayerDynamicStyleCountInvalid,
              &AbstractThemeTest::setTextLayerDynamicStyleCount,

              &AbstractThemeTest::textLayerGlyphCacheSizeNoTextFeature,
              &AbstractThemeTest::textLayerGlyphCacheSizeFeaturesNotSupported,
              &AbstractThemeTest::setTextLayerGlyphCacheSize,

              &AbstractThemeTest::layoutLayer,
              &AbstractThemeTest::layoutLayerNotSupported,
              &AbstractThemeTest::layoutLayerNotImplemented});

    addInstancedTests({&AbstractThemeTest::apply},
        Containers::arraySize(ApplyData));

    addTests({&AbstractThemeTest::applyNoFeatures,
              &AbstractThemeTest::applyFeaturesNotSupported,
              &AbstractThemeTest::applyNoSizeSet,
              &AbstractThemeTest::applyDataLayerNotPresent,
              &AbstractThemeTest::applyBackgroundLayerNotPresent,
              &AbstractThemeTest::applyBackgroundLayerDifferentStyleCount,
              &AbstractThemeTest::applyBackgroundLayerStyleAnimatorNotPresent,
              &AbstractThemeTest::applyBaseLayerNotPresent,
              &AbstractThemeTest::applyBaseLayerDifferentStyleCount,
              &AbstractThemeTest::applyBaseLayerStyleAnimatorNotPresent,
              &AbstractThemeTest::applyTextLayerNotPresent,
              &AbstractThemeTest::applyTextLayerDifferentStyleCount,
              &AbstractThemeTest::applyTextLayerDifferentGlyphCache,
              &AbstractThemeTest::applyTextLayerNoFontManager,
              &AbstractThemeTest::applyTextLayerStyleAnimatorNotPresent,
              &AbstractThemeTest::applyEventLayerNotPresent,
              &AbstractThemeTest::applyLayoutLayerNotPresent,
              &AbstractThemeTest::applyLayoutLayerDifferentStyleCount,
              &AbstractThemeTest::applySnapLayouterNotPresent,
              &AbstractThemeTest::applyGenericLayouterNotPresent,
              &AbstractThemeTest::applyNodeAnimatorNotPresent});
}

void AbstractThemeTest::debugFeature() {
    Containers::String out;
    Debug{&out} << ThemeFeature::BaseLayer << ThemeFeature(0xbeef);
    CORRADE_COMPARE(out, "Ui::ThemeFeature::BaseLayer Ui::ThemeFeature(0xbeef)\n");
}

void AbstractThemeTest::debugFeatures() {
    Containers::String out;
    Debug{&out} << (ThemeFeature::TextLayer|ThemeFeature(0x8000)) << ThemeFeatures{};
    CORRADE_COMPARE(out, "Ui::ThemeFeature::TextLayer|Ui::ThemeFeature(0x8000) Ui::ThemeFeatures{}\n");
}

void AbstractThemeTest::construct() {
    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override { return ThemeFeature::BaseLayer; }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }
    } theme;
    CORRADE_COMPARE(theme.features(), ThemeFeature::BaseLayer);
}

void AbstractThemeTest::constructCopy() {
    struct Theme: AbstractTheme {
        explicit Theme(ThemeFeatures features): _features{features} {}

        ThemeFeatures doFeatures() const override { return _features; }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }

        ThemeFeatures _features;
    };

    Theme a{ThemeFeature::TextLayer};

    Theme b = a;
    CORRADE_COMPARE(b.features(), ThemeFeature::TextLayer);

    Theme c{ThemeFeature::BaseLayer};
    c = b;
    CORRADE_COMPARE(c.features(), ThemeFeature::TextLayer);

    CORRADE_VERIFY(std::is_nothrow_move_constructible<Theme>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<Theme>::value);
}

void AbstractThemeTest::noFeaturesReturned() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override { return {}; }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }
    } theme;

    Containers::String out;
    Error redirectError{&out};
    theme.features();
    CORRADE_COMPARE(out, "Ui::AbstractTheme::features(): implementation returned an empty set\n");
}

void AbstractThemeTest::backgroundLayer() {
    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override {
            /* Verify it's testing a superset, not equality */
            return ThemeFeature::BackgroundLayer|ThemeFeature(0x1000);
        }
        UnsignedInt doBackgroundLayerStyleUniformCount() const override { return 3; }
        UnsignedInt doBackgroundLayerStyleCount() const override { return 5; }
        UnsignedInt doBackgroundLayerDynamicStyleCount() const override { return 11; }
        UnsignedInt doBackgroundLayerBlurRadius() const override { return 7; }
        Float doBackgroundLayerBlurCutoff() const override { return 0.5f; }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }
    } theme;
    CORRADE_COMPARE(theme.backgroundLayerStyleUniformCount(), 3);
    CORRADE_COMPARE(theme.backgroundLayerStyleCount(), 5);
    CORRADE_COMPARE(theme.backgroundLayerDynamicStyleCount(), 11);
    CORRADE_COMPARE(theme.backgroundLayerBlurRadius(), 7);
    CORRADE_COMPARE(theme.backgroundLayerBlurCutoff(), 0.5f);
}

void AbstractThemeTest::backgroundLayerNotSupported() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override {
            /* To verify it's not accidentally checking some other bit */
            return ~ThemeFeature::BackgroundLayer;
        }
        UnsignedInt doBackgroundLayerStyleUniformCount() const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
        UnsignedInt doBackgroundLayerStyleCount() const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
        UnsignedInt doBackgroundLayerDynamicStyleCount() const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
        BaseLayerSharedFlags doBackgroundLayerFlags() const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
        UnsignedInt doBackgroundLayerBlurRadius() const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
        Float doBackgroundLayerBlurCutoff() const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }
    } theme;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::String out;
    Error redirectError{&out};
    theme.backgroundLayerStyleUniformCount();
    theme.backgroundLayerStyleCount();
    theme.backgroundLayerDynamicStyleCount();
    theme.setBackgroundLayerDynamicStyleCount({});
    theme.backgroundLayerFlags();
    theme.setBackgroundLayerFlags({}, {});
    theme.backgroundLayerBlurRadius();
    theme.backgroundLayerBlurCutoff();
    CORRADE_COMPARE_AS(out,
        "Ui::AbstractTheme::backgroundLayerStyleUniformCount(): feature not supported\n"
        "Ui::AbstractTheme::backgroundLayerStyleCount(): feature not supported\n"
        "Ui::AbstractTheme::backgroundLayerDynamicStyleCount(): feature not supported\n"
        "Ui::AbstractTheme::setBackgroundLayerDynamicStyleCount(): feature not supported\n"
        "Ui::AbstractTheme::backgroundLayerFlags(): feature not supported\n"
        "Ui::AbstractTheme::setBackgroundLayerFlags(): feature not supported\n"
        "Ui::AbstractTheme::backgroundLayerBlurRadius(): feature not supported\n"
        "Ui::AbstractTheme::backgroundLayerBlurCutoff(): feature not supported\n",
        TestSuite::Compare::String);
}

void AbstractThemeTest::backgroundLayerNotImplemented() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override {
            return ThemeFeature::BackgroundLayer;
        }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }
    } theme;

    Containers::String out;
    Error redirectError{&out};
    /* The backgroundLayerUniformCount() delegates to
       backgroundLayerStyleCount() by default, so the assertion is the same.
       Delegation and value propagation tested below. */
    theme.backgroundLayerStyleUniformCount();
    theme.backgroundLayerStyleCount();
    /* backgroundLayerDynamicStyleCount(), backgroundLayerBlurRadius() and
       backgroundLayerBlurCutoff() have a default, tested in
       backgroundLayerNotImplementedDefaults() below */
    CORRADE_COMPARE_AS(out,
        "Ui::AbstractTheme::backgroundLayerStyleCount(): feature advertised but not implemented\n"
        "Ui::AbstractTheme::backgroundLayerStyleCount(): feature advertised but not implemented\n",
        TestSuite::Compare::String);
}

void AbstractThemeTest::backgroundLayerNotImplementedDefaults() {
    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override {
            return ThemeFeature::BackgroundLayer;
        }
        UnsignedInt doBackgroundLayerStyleCount() const override {
            return 17;
        }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }
    } theme;

    CORRADE_COMPARE(theme.backgroundLayerDynamicStyleCount(), 0);
    /* With backgroundLayerStyleCount() not implemented it would assert, tested
       above */
    CORRADE_COMPARE(theme.backgroundLayerStyleUniformCount(), 17);
    CORRADE_COMPARE(theme.backgroundLayerFlags(), BaseLayerSharedFlags{});
    CORRADE_COMPARE(theme.backgroundLayerBlurRadius(), 4);
    CORRADE_COMPARE(theme.backgroundLayerBlurCutoff(), 0.5f/255.0f);
}

void AbstractThemeTest::backgroundLayerDynamicStyleCountInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override {
            return ThemeFeature::BackgroundLayer|ThemeFeature::BackgroundLayerAnimations;
        }
        /* Other layer dynamic styles set instead, to verify the correct
           interface is checked */
        UnsignedInt doBaseLayerDynamicStyleCount() const override { return 9; }
        UnsignedInt doTextLayerDynamicStyleCount() const override { return 3; }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }
    } theme;

    Containers::String out;
    Error redirectError{&out};
    theme.backgroundLayerDynamicStyleCount();
    CORRADE_COMPARE(out, "Ui::AbstractTheme::backgroundLayerDynamicStyleCount(): implementation advertises Ui::ThemeFeature::BackgroundLayerAnimations but zero dynamic styles\n");
}

void AbstractThemeTest::setBackgroundLayerDynamicStyleCount() {
    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override {
            return ThemeFeature::BackgroundLayer;
        }
        UnsignedInt doBackgroundLayerDynamicStyleCount() const override { return 9; }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }
    } theme;

    /* By default it returns what the theme says */
    CORRADE_COMPARE(theme.backgroundLayerDynamicStyleCount(), 9);

    /* Setting a new value */
    theme.setBackgroundLayerDynamicStyleCount(11);
    CORRADE_COMPARE(theme.backgroundLayerDynamicStyleCount(), 11);

    /* Setting a new but smaller value than before */
    theme.setBackgroundLayerDynamicStyleCount(10);
    CORRADE_COMPARE(theme.backgroundLayerDynamicStyleCount(), 10);

    /* Setting a value smaller than what theme says picks the theme instead */
    theme.setBackgroundLayerDynamicStyleCount(3);
    CORRADE_COMPARE(theme.backgroundLayerDynamicStyleCount(), 9);
}

void AbstractThemeTest::backgroundLayerFlags() {
    struct Theme: AbstractTheme {
        explicit Theme(BaseLayerSharedFlags flags): _flags{flags} {}

        ThemeFeatures doFeatures() const override {
            return ThemeFeature::BackgroundLayer;
        }
        BaseLayerSharedFlags doBackgroundLayerFlags() const override {
            return _flags;
        }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }

        private:
            BaseLayerSharedFlags _flags;
    } themeNeither{BaseLayerSharedFlag::NoOutline|BaseLayerSharedFlag::NoRoundedCorners},
      themeBlurNoRoundedCorners{BaseLayerSharedFlag::BackgroundBlur|BaseLayerSharedFlag::NoRoundedCorners};

    /* By default it returns what the theme says */
    CORRADE_COMPARE(themeNeither.backgroundLayerFlags(), BaseLayerSharedFlag::NoOutline|BaseLayerSharedFlag::NoRoundedCorners);
    CORRADE_COMPARE(themeBlurNoRoundedCorners.backgroundLayerFlags(), BaseLayerSharedFlag::BackgroundBlur|BaseLayerSharedFlag::NoRoundedCorners);

    /* Adding / clearing no flags doesn't change anything */
    themeBlurNoRoundedCorners.setBackgroundLayerFlags({}, {});
    CORRADE_COMPARE(themeBlurNoRoundedCorners.backgroundLayerFlags(), BaseLayerSharedFlag::BackgroundBlur|BaseLayerSharedFlag::NoRoundedCorners);

    /* Clearing a flag that isn't there doesn't change anything */
    /** @todo test also adding a flag that is there, once such a flag is
        allowed */
    themeBlurNoRoundedCorners.setBackgroundLayerFlags({}, BaseLayerSharedFlag::NoOutline);
    CORRADE_COMPARE(themeBlurNoRoundedCorners.backgroundLayerFlags(), BaseLayerSharedFlag::BackgroundBlur|BaseLayerSharedFlag::NoRoundedCorners);

    /* Adding a flag that isn't there updates the theme, clearing a flag that
       is there updates it also */
    themeBlurNoRoundedCorners.setBackgroundLayerFlags(BaseLayerSharedFlag::SubdividedQuads, {});
    themeNeither.setBackgroundLayerFlags({}, BaseLayerSharedFlag::NoOutline);
    CORRADE_COMPARE(themeBlurNoRoundedCorners.backgroundLayerFlags(), BaseLayerSharedFlag::SubdividedQuads|BaseLayerSharedFlag::BackgroundBlur|BaseLayerSharedFlag::NoRoundedCorners);
    CORRADE_COMPARE(themeNeither.backgroundLayerFlags(), BaseLayerSharedFlag::NoRoundedCorners);

    /* Adding no flags returns to the previous state */
    themeNeither.setBackgroundLayerFlags({}, {});
    themeBlurNoRoundedCorners.setBackgroundLayerFlags({}, {});
    CORRADE_COMPARE(themeNeither.backgroundLayerFlags(), BaseLayerSharedFlag::NoOutline|BaseLayerSharedFlag::NoRoundedCorners);
    CORRADE_COMPARE(themeBlurNoRoundedCorners.backgroundLayerFlags(), BaseLayerSharedFlag::BackgroundBlur|BaseLayerSharedFlag::NoRoundedCorners);
}

void AbstractThemeTest::backgroundLayerFlagsInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Theme: AbstractTheme {
        explicit Theme(BaseLayerSharedFlags flags): _flags{flags} {}

        ThemeFeatures doFeatures() const override {
            return ThemeFeature::BackgroundLayer;
        }
        BaseLayerSharedFlags doBackgroundLayerFlags() const override {
            return _flags;
        }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }

        private:
            BaseLayerSharedFlags _flags;
    } theme{{}},
      themeReturnedInvalid{BaseLayerSharedFlag::SubdividedQuads|BaseLayerSharedFlag::Textured|BaseLayerSharedFlag::NoOutline};

    Containers::String out;
    Error redirectError{&out};
    themeReturnedInvalid.backgroundLayerFlags();
    theme.setBackgroundLayerFlags(BaseLayerSharedFlag::NoOutline|BaseLayerSharedFlag::SubdividedQuads|BaseLayerSharedFlag::Textured|BaseLayerSharedFlag::BackgroundBlur, {});
    theme.setBackgroundLayerFlags({}, BaseLayerSharedFlag::NoOutline|BaseLayerSharedFlag::SubdividedQuads|BaseLayerSharedFlag::Textured|BaseLayerSharedFlag::BackgroundBlur);
    CORRADE_COMPARE_AS(out,
        "Ui::AbstractTheme::backgroundLayerFlags(): implementation returned disallowed Ui::BaseLayerSharedFlag::Textured|Ui::BaseLayerSharedFlag::SubdividedQuads\n"
        "Ui::AbstractTheme::setBackgroundLayerFlags(): Ui::BaseLayerSharedFlag::Textured|Ui::BaseLayerSharedFlag::BackgroundBlur|Ui::BaseLayerSharedFlag::NoOutline isn't allowed to be added\n"
        "Ui::AbstractTheme::setBackgroundLayerFlags(): Ui::BaseLayerSharedFlag::Textured|Ui::BaseLayerSharedFlag::SubdividedQuads isn't allowed to be cleared\n",
        TestSuite::Compare::String);
}

void AbstractThemeTest::baseLayer() {
    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override {
            /* Verify it's testing a superset, not equality */
            return ThemeFeature::BaseLayer|ThemeFeature(0x1000);
        }
        UnsignedInt doBaseLayerStyleUniformCount() const override { return 3; }
        UnsignedInt doBaseLayerStyleCount() const override { return 5; }
        UnsignedInt doBaseLayerDynamicStyleCount() const override { return 11; }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }
    } theme;
    CORRADE_COMPARE(theme.baseLayerStyleUniformCount(), 3);
    CORRADE_COMPARE(theme.baseLayerStyleCount(), 5);
    CORRADE_COMPARE(theme.baseLayerDynamicStyleCount(), 11);
}

void AbstractThemeTest::baseLayerNotSupported() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override {
            /* To verify it's not accidentally checking some other bit */
            return ~ThemeFeature::BaseLayer;
        }
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
        BaseLayerSharedFlags doBaseLayerFlags() const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }
    } theme;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::String out;
    Error redirectError{&out};
    theme.baseLayerStyleUniformCount();
    theme.baseLayerStyleCount();
    theme.baseLayerDynamicStyleCount();
    theme.setBaseLayerDynamicStyleCount({});
    theme.baseLayerFlags();
    theme.setBaseLayerFlags({}, {});
    CORRADE_COMPARE_AS(out,
        "Ui::AbstractTheme::baseLayerStyleUniformCount(): feature not supported\n"
        "Ui::AbstractTheme::baseLayerStyleCount(): feature not supported\n"
        "Ui::AbstractTheme::baseLayerDynamicStyleCount(): feature not supported\n"
        "Ui::AbstractTheme::setBaseLayerDynamicStyleCount(): feature not supported\n"
        "Ui::AbstractTheme::baseLayerFlags(): feature not supported\n"
        "Ui::AbstractTheme::setBaseLayerFlags(): feature not supported\n",
        TestSuite::Compare::String);
}

void AbstractThemeTest::baseLayerNotImplemented() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override {
            return ThemeFeature::BaseLayer;
        }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }
    } theme;

    Containers::String out;
    Error redirectError{&out};
    /* The baseLayerUniformCount() delegates to baseLayerStyleCount() by
       default, so the assertion is the same. Delegation and value propagation
       tested below. */
    theme.baseLayerStyleUniformCount();
    theme.baseLayerStyleCount();
    /* baseLayerDynamicStyleCount() has a default, tested in
       baseLayerNotImplementedDefaults() below */
    CORRADE_COMPARE_AS(out,
        "Ui::AbstractTheme::baseLayerStyleCount(): feature advertised but not implemented\n"
        "Ui::AbstractTheme::baseLayerStyleCount(): feature advertised but not implemented\n",
        TestSuite::Compare::String);
}

void AbstractThemeTest::baseLayerNotImplementedDefaults() {
    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override {
            return ThemeFeature::BaseLayer;
        }
        UnsignedInt doBaseLayerStyleCount() const override {
            return 17;
        }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }
    } theme;

    CORRADE_COMPARE(theme.baseLayerDynamicStyleCount(), 0);
    /* With baseLayerStyleCount() not implemented it would assert, tested
       above */
    CORRADE_COMPARE(theme.baseLayerStyleUniformCount(), 17);
    CORRADE_COMPARE(theme.baseLayerFlags(), BaseLayerSharedFlags{});
}

void AbstractThemeTest::baseLayerDynamicStyleCountInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override {
            return ThemeFeature::BaseLayer|ThemeFeature::BaseLayerAnimations;
        }
        /* Other layer dynamic styles set instead, to verify the correct
           interface is checked */
        UnsignedInt doBackgroundLayerDynamicStyleCount() const override { return 9; }
        UnsignedInt doTextLayerDynamicStyleCount() const override { return 9; }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }
    } theme;

    Containers::String out;
    Error redirectError{&out};
    theme.baseLayerDynamicStyleCount();
    CORRADE_COMPARE(out, "Ui::AbstractTheme::baseLayerDynamicStyleCount(): implementation advertises Ui::ThemeFeature::BaseLayerAnimations but zero dynamic styles\n");
}

void AbstractThemeTest::setBaseLayerDynamicStyleCount() {
    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override {
            return ThemeFeature::BaseLayer;
        }
        UnsignedInt doBaseLayerDynamicStyleCount() const override { return 9; }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }
    } theme;

    /* By default it returns what the theme says */
    CORRADE_COMPARE(theme.baseLayerDynamicStyleCount(), 9);

    /* Setting a new value */
    theme.setBaseLayerDynamicStyleCount(11);
    CORRADE_COMPARE(theme.baseLayerDynamicStyleCount(), 11);

    /* Setting a new but smaller value than before */
    theme.setBaseLayerDynamicStyleCount(10);
    CORRADE_COMPARE(theme.baseLayerDynamicStyleCount(), 10);

    /* Setting a value smaller than what theme says picks the theme instead */
    theme.setBaseLayerDynamicStyleCount(3);
    CORRADE_COMPARE(theme.baseLayerDynamicStyleCount(), 9);
}

void AbstractThemeTest::baseLayerFlags() {
    struct Theme: AbstractTheme {
        explicit Theme(BaseLayerSharedFlags flags): _flags{flags} {}

        ThemeFeatures doFeatures() const override {
            return ThemeFeature::BaseLayer;
        }
        BaseLayerSharedFlags doBaseLayerFlags() const override {
            return _flags;
        }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }

        private:
            BaseLayerSharedFlags _flags;
    } themeNeither{BaseLayerSharedFlag::NoOutline|BaseLayerSharedFlag::NoRoundedCorners},
      themeNoRoundedCorners{BaseLayerSharedFlag::NoRoundedCorners};

    /* By default it returns what the theme says */
    CORRADE_COMPARE(themeNeither.baseLayerFlags(), BaseLayerSharedFlag::NoOutline|BaseLayerSharedFlag::NoRoundedCorners);
    CORRADE_COMPARE(themeNoRoundedCorners.baseLayerFlags(), BaseLayerSharedFlag::NoRoundedCorners);

    /* Adding / clearing no flags doesn't change anything */
    themeNoRoundedCorners.setBaseLayerFlags({}, {});
    CORRADE_COMPARE(themeNoRoundedCorners.baseLayerFlags(), BaseLayerSharedFlag::NoRoundedCorners);

    /* Clearing a flag that isn't there doesn't change anything */
    /** @todo test also adding a flag that is there, once such a flag is
        allowed */
    themeNoRoundedCorners.setBaseLayerFlags({}, BaseLayerSharedFlag::NoOutline);
    CORRADE_COMPARE(themeNoRoundedCorners.baseLayerFlags(), BaseLayerSharedFlag::NoRoundedCorners);

    /* Adding a flag that isn't there updates the theme, clearing a flag that
       is there updates it also */
    themeNoRoundedCorners.setBaseLayerFlags(BaseLayerSharedFlag::SubdividedQuads, {});
    themeNeither.setBaseLayerFlags({}, BaseLayerSharedFlag::NoOutline);
    CORRADE_COMPARE(themeNoRoundedCorners.baseLayerFlags(), BaseLayerSharedFlag::SubdividedQuads|BaseLayerSharedFlag::NoRoundedCorners);
    CORRADE_COMPARE(themeNeither.baseLayerFlags(), BaseLayerSharedFlag::NoRoundedCorners);

    /* Adding no flags returns to the previous state */
    themeNeither.setBaseLayerFlags({}, {});
    themeNoRoundedCorners.setBaseLayerFlags({}, {});
    CORRADE_COMPARE(themeNeither.baseLayerFlags(), BaseLayerSharedFlag::NoOutline|BaseLayerSharedFlag::NoRoundedCorners);
    CORRADE_COMPARE(themeNoRoundedCorners.baseLayerFlags(), BaseLayerSharedFlag::NoRoundedCorners);
}

void AbstractThemeTest::baseLayerFlagsInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Theme: AbstractTheme {
        explicit Theme(BaseLayerSharedFlags flags): _flags{flags} {}

        ThemeFeatures doFeatures() const override {
            return ThemeFeature::BaseLayer;
        }
        BaseLayerSharedFlags doBaseLayerFlags() const override {
            return _flags;
        }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }

        private:
            BaseLayerSharedFlags _flags;
    } theme{{}},
      themeReturnedInvalid{BaseLayerSharedFlag::SubdividedQuads|BaseLayerSharedFlag::Textured|BaseLayerSharedFlag::NoOutline};

    Containers::String out;
    Error redirectError{&out};
    themeReturnedInvalid.baseLayerFlags();
    theme.setBaseLayerFlags(BaseLayerSharedFlag::NoOutline|BaseLayerSharedFlag::SubdividedQuads|BaseLayerSharedFlag::Textured, {});
    theme.setBaseLayerFlags({}, BaseLayerSharedFlag::NoOutline|BaseLayerSharedFlag::SubdividedQuads|BaseLayerSharedFlag::Textured);
    CORRADE_COMPARE_AS(out,
        "Ui::AbstractTheme::baseLayerFlags(): implementation returned disallowed Ui::BaseLayerSharedFlag::Textured|Ui::BaseLayerSharedFlag::SubdividedQuads\n"
        "Ui::AbstractTheme::setBaseLayerFlags(): Ui::BaseLayerSharedFlag::Textured|Ui::BaseLayerSharedFlag::NoOutline isn't allowed to be added\n"
        "Ui::AbstractTheme::setBaseLayerFlags(): Ui::BaseLayerSharedFlag::Textured|Ui::BaseLayerSharedFlag::SubdividedQuads isn't allowed to be cleared\n",
        TestSuite::Compare::String);
}

void AbstractThemeTest::textLayer() {
    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override {
            /* Verify it's testing a superset, not equality */
            return ThemeFeature::TextLayer|ThemeFeature(0x1000);
        }
        UnsignedInt doTextLayerStyleUniformCount() const override { return 3; }
        UnsignedInt doTextLayerStyleCount() const override { return 5; }
        UnsignedInt doTextLayerEditingStyleUniformCount() const override { return 11; }
        UnsignedInt doTextLayerEditingStyleCount() const override { return 7; }
        UnsignedInt doTextLayerDynamicStyleCount() const override { return 9; }
        PixelFormat doTextLayerGlyphCacheFormat() const override {
            return PixelFormat::RG32F;
        }
        Vector3i doTextLayerGlyphCacheSize(ThemeFeatures features) const override {
            CORRADE_COMPARE(features, ThemeFeature::TextLayer|ThemeFeature(0x1000));
            return {3, 5, 18};
        }
        Vector2i doTextLayerGlyphCachePadding() const override {
            return {2, 4};
        }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }
    } theme;
    CORRADE_COMPARE(theme.textLayerStyleUniformCount(), 3);
    CORRADE_COMPARE(theme.textLayerStyleCount(), 5);
    CORRADE_COMPARE(theme.textLayerEditingStyleUniformCount(), 11);
    CORRADE_COMPARE(theme.textLayerEditingStyleCount(), 7);
    CORRADE_COMPARE(theme.textLayerDynamicStyleCount(), 9);
    CORRADE_COMPARE(theme.textLayerGlyphCacheFormat(), PixelFormat::RG32F);
    CORRADE_COMPARE(theme.textLayerGlyphCacheSize(ThemeFeature::TextLayer|ThemeFeature(0x1000)), (Vector3i{3, 5, 18}));
    CORRADE_COMPARE(theme.textLayerGlyphCachePadding(), (Vector2i{2, 4}));
}

void AbstractThemeTest::textLayerNotSupported() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override {
            /* To verify it's not accidentally checking some other bit */
            return ~ThemeFeature::TextLayer;
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
        PixelFormat doTextLayerGlyphCacheFormat() const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
        Vector3i doTextLayerGlyphCacheSize(ThemeFeatures) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
        Vector2i doTextLayerGlyphCachePadding() const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }
    } theme;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::String out;
    Error redirectError{&out};
    theme.textLayerStyleUniformCount();
    theme.textLayerStyleCount();
    theme.textLayerEditingStyleUniformCount();
    theme.textLayerEditingStyleCount();
    theme.textLayerDynamicStyleCount();
    theme.setTextLayerDynamicStyleCount({});
    theme.textLayerGlyphCacheFormat();
    theme.textLayerGlyphCacheSize(ThemeFeature::TextLayer);
    theme.textLayerGlyphCachePadding();
    theme.setTextLayerGlyphCacheSize({});
    CORRADE_COMPARE_AS(out,
        "Ui::AbstractTheme::textLayerStyleUniformCount(): feature not supported\n"
        "Ui::AbstractTheme::textLayerStyleCount(): feature not supported\n"
        "Ui::AbstractTheme::textLayerEditingStyleUniformCount(): feature not supported\n"
        "Ui::AbstractTheme::textLayerEditingStyleCount(): feature not supported\n"
        "Ui::AbstractTheme::textLayerDynamicStyleCount(): feature not supported\n"
        "Ui::AbstractTheme::setTextLayerDynamicStyleCount(): feature not supported\n"
        "Ui::AbstractTheme::textLayerGlyphCacheFormat(): feature not supported\n"
        "Ui::AbstractTheme::textLayerGlyphCacheSize(): feature not supported\n"
        "Ui::AbstractTheme::textLayerGlyphCachePadding(): feature not supported\n"
        "Ui::AbstractTheme::setTextLayerGlyphCacheSize(): feature not supported\n",
        TestSuite::Compare::String);
}

void AbstractThemeTest::textLayerNotImplemented() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override {
            return ThemeFeature::TextLayer;
        }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }
    } theme;

    Containers::String out;
    Error redirectError{&out};
    /* The textLayerStyleUniformCount() delegates to textLayerStyleCount() by
       default, so the assertion is the same. Delegation and value propagation
       tested below. */
    theme.textLayerStyleUniformCount();
    theme.textLayerStyleCount();
    /* textLayerEditingStyleUniformCount(), textLayerEditingStyleCount(),
       textLayerDynamicStyleCount(), textLayerGlyphCacheFormat() and
       textLayerGlyphCachePadding() have defaults, tested below */
    theme.textLayerGlyphCacheSize(ThemeFeature::TextLayer);
    CORRADE_COMPARE_AS(out,
        "Ui::AbstractTheme::textLayerStyleCount(): feature advertised but not implemented\n"
        "Ui::AbstractTheme::textLayerStyleCount(): feature advertised but not implemented\n"
        "Ui::AbstractTheme::textLayerGlyphCacheSize(): feature advertised but not implemented\n",
        TestSuite::Compare::String);
}

void AbstractThemeTest::textLayerNotImplementedDefaults() {
    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override {
            return ThemeFeature::TextLayer;
        }
        UnsignedInt doTextLayerStyleCount() const override {
            return 35;
        }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }
    } theme;

    CORRADE_COMPARE(theme.textLayerEditingStyleUniformCount(), 0);
    CORRADE_COMPARE(theme.textLayerEditingStyleCount(), 0);
    CORRADE_COMPARE(theme.textLayerDynamicStyleCount(), 0);
    /* With textLayerStyleCount() not implemented it would assert, tested
       above */
    CORRADE_COMPARE(theme.textLayerStyleUniformCount(), 35);
    CORRADE_COMPARE(theme.textLayerGlyphCacheFormat(), PixelFormat::R8Unorm);
    /* Padding is 1 by default, consistently with Text::AbstractGlyphCache */
    CORRADE_COMPARE(theme.textLayerGlyphCachePadding(), Vector2i{1});
    /* textLayerGlyphCacheSize() asserts, tested above */
}

void AbstractThemeTest::textLayerDynamicStyleCountInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override {
            return ThemeFeature::TextLayer|ThemeFeature::TextLayerAnimations;
        }
        /* Other layer dynamic styles set instead, to verify the correct
           interface is checked */
        UnsignedInt doBaseLayerDynamicStyleCount() const override { return 9; }
        UnsignedInt doBackgroundLayerDynamicStyleCount() const override { return 3; }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }
    } theme;

    Containers::String out;
    Error redirectError{&out};
    theme.textLayerDynamicStyleCount();
    CORRADE_COMPARE(out, "Ui::AbstractTheme::textLayerDynamicStyleCount(): implementation advertises Ui::ThemeFeature::TextLayerAnimations but zero dynamic styles\n");
}

void AbstractThemeTest::setTextLayerDynamicStyleCount() {
    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override {
            return ThemeFeature::TextLayer;
        }
        UnsignedInt doTextLayerDynamicStyleCount() const override { return 9; }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }
    } theme;

    /* By default it returns what the theme says */
    CORRADE_COMPARE(theme.textLayerDynamicStyleCount(), 9);

    /* Setting a new value */
    theme.setTextLayerDynamicStyleCount(11);
    CORRADE_COMPARE(theme.textLayerDynamicStyleCount(), 11);

    /* Setting a new but smaller value than before */
    theme.setTextLayerDynamicStyleCount(10);
    CORRADE_COMPARE(theme.textLayerDynamicStyleCount(), 10);

    /* Setting a value smaller than what theme says picks the theme instead */
    theme.setTextLayerDynamicStyleCount(3);
    CORRADE_COMPARE(theme.textLayerDynamicStyleCount(), 9);
}

void AbstractThemeTest::textLayerGlyphCacheSizeNoTextFeature() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override {
            return ThemeFeature::TextLayer;
        }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }
    } theme;

    Containers::String out;
    Error redirectError{&out};
    theme.textLayerGlyphCacheSize(ThemeFeature::BaseLayer|ThemeFeature(0x8000));
    CORRADE_COMPARE(out, "Ui::AbstractTheme::textLayerGlyphCacheSize(): expected a superset of Ui::ThemeFeature::TextLayer but got Ui::ThemeFeature::BaseLayer|Ui::ThemeFeature(0x8000)\n");
}

void AbstractThemeTest::textLayerGlyphCacheSizeFeaturesNotSupported() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override {
            return ThemeFeature::TextLayer;
        }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }
    } theme;

    Containers::String out;
    Error redirectError{&out};
    theme.textLayerGlyphCacheSize(ThemeFeature::TextLayer|ThemeFeature::BaseLayer);
    CORRADE_COMPARE(out, "Ui::AbstractTheme::textLayerGlyphCacheSize(): Ui::ThemeFeature::BaseLayer|Ui::ThemeFeature::TextLayer not a subset of supported Ui::ThemeFeature::TextLayer\n");
}

void AbstractThemeTest::setTextLayerGlyphCacheSize() {
    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override {
            return ThemeFeature::TextLayer|ThemeFeature::TextLayerAnimations;
        }
        Vector3i doTextLayerGlyphCacheSize(ThemeFeatures features) const override {
            /** @todo switch to some actual practical feature once it exists,
                such as toggling distance field fonts, this doesn't make much
                sense */
            if(features >= ThemeFeature::TextLayerAnimations)
                return {256, 128, 32};
            return {16, 32, 8};
        }
        Vector2i doTextLayerGlyphCachePadding() const override {
            return {4, 2};
        }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }
    } theme;

    /* By default it returns what the theme says */
    CORRADE_COMPARE(theme.textLayerGlyphCacheSize(ThemeFeature::TextLayer|ThemeFeature::TextLayerAnimations), (Vector3i{256, 128, 32}));
    CORRADE_COMPARE(theme.textLayerGlyphCacheSize(ThemeFeature::TextLayer), (Vector3i{16, 32, 8}));
    CORRADE_COMPARE(theme.textLayerGlyphCachePadding(), (Vector2i{4, 2}));

    /* Setting a new value */
    theme.setTextLayerGlyphCacheSize({48, 56, 12}, {6, 8});
    CORRADE_COMPARE(theme.textLayerGlyphCacheSize(ThemeFeature::TextLayer), (Vector3i{48, 56, 12}));
    CORRADE_COMPARE(theme.textLayerGlyphCachePadding(), (Vector2i{6, 8}));

    /* It doesn't get overwritten or forgotten when asking for a size with
       different features */
    CORRADE_COMPARE(theme.textLayerGlyphCacheSize(ThemeFeature::TextLayer|ThemeFeature::TextLayerAnimations), (Vector3i{256, 128, 32}));
    CORRADE_COMPARE(theme.textLayerGlyphCacheSize(ThemeFeature::TextLayer), (Vector3i{48, 56, 12}));

    /* Setting a new but smaller value than before */
    theme.setTextLayerGlyphCacheSize({24, 48, 10}, {5, 3});
    CORRADE_COMPARE(theme.textLayerGlyphCacheSize(ThemeFeature::TextLayer), (Vector3i{24, 48, 10}));
    CORRADE_COMPARE(theme.textLayerGlyphCachePadding(), (Vector2i{5, 3}));

    /* Setting a value smaller than what theme says picks the theme instead */
    theme.setTextLayerGlyphCacheSize({}, {});
    CORRADE_COMPARE(theme.textLayerGlyphCacheSize(ThemeFeature::TextLayer), (Vector3i{16, 32, 8}));
    CORRADE_COMPARE(theme.textLayerGlyphCachePadding(), (Vector2i{4, 2}));

    /* Setting a new value only picks the dimensions that are actually
       larger */
    theme.setTextLayerGlyphCacheSize({12, 33, 6}, {5, 1});
    CORRADE_COMPARE(theme.textLayerGlyphCacheSize(ThemeFeature::TextLayer), (Vector3i{16, 33, 8}));
    CORRADE_COMPARE(theme.textLayerGlyphCachePadding(), (Vector2i{5, 2}));

    theme.setTextLayerGlyphCacheSize({17, 24, 6}, {3, 3});
    CORRADE_COMPARE(theme.textLayerGlyphCacheSize(ThemeFeature::TextLayer), (Vector3i{17, 32, 8}));
    CORRADE_COMPARE(theme.textLayerGlyphCachePadding(), (Vector2i{4, 3}));

    theme.setTextLayerGlyphCacheSize({12, 24, 12});
    CORRADE_COMPARE(theme.textLayerGlyphCacheSize(ThemeFeature::TextLayer), (Vector3i{16, 32, 12}));
    CORRADE_COMPARE(theme.textLayerGlyphCachePadding(), (Vector2i{4, 2}));
}

void AbstractThemeTest::layoutLayer() {
    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override {
            /* Verify it's testing a superset, not equality */
            return ThemeFeature::LayoutLayer|ThemeFeature(0x1000);
        }
        UnsignedInt doLayoutLayerStyleCount() const override { return 15; }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }
    } theme;
    CORRADE_COMPARE(theme.layoutLayerStyleCount(), 15);
}

void AbstractThemeTest::layoutLayerNotSupported() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override {
            /* To verify it's not accidentally checking some other bit */
            return ~ThemeFeature::LayoutLayer;
        }
        UnsignedInt doLayoutLayerStyleCount() const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }
    } theme;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::String out;
    Error redirectError{&out};
    theme.layoutLayerStyleCount();
    CORRADE_COMPARE_AS(out,
        "Ui::AbstractTheme::layoutLayerStyleCount(): feature not supported\n",
        TestSuite::Compare::String);
}

void AbstractThemeTest::layoutLayerNotImplemented() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override {
            return ThemeFeature::LayoutLayer;
        }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }
    } theme;

    Containers::String out;
    Error redirectError{&out};
    theme.layoutLayerStyleCount();
    CORRADE_COMPARE_AS(out,
        "Ui::AbstractTheme::layoutLayerStyleCount(): feature advertised but not implemented\n",
        TestSuite::Compare::String);
}

void AbstractThemeTest::apply() {
    auto&& data = ApplyData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct LayerSharedBase: BaseLayer::Shared {
        explicit LayerSharedBase(const Configuration& configuration): BaseLayer::Shared{configuration} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } sharedBackground{BaseLayer::Shared::Configuration{17, 19}
        .setDynamicStyleCount(23)
    }, sharedBase{BaseLayer::Shared::Configuration{3, 5}
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
    if(data.dataLayerPresent)
        ui.setDataLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));
    if(data.backgroundLayerPresent)
        ui.setBackgroundLayerInstance(Containers::pointer<LayerBase>(ui.createLayer(), sharedBackground));
    if(data.backgroundLayerStyleAnimatorPresent)
        ui.setBackgroundLayerStyleAnimatorInstance(Containers::pointer<BaseLayerStyleAnimator>(ui.createAnimator()));
    if(data.baseLayerPresent)
        ui.setBaseLayerInstance(Containers::pointer<LayerBase>(ui.createLayer(), sharedBase));
    if(data.baseLayerStyleAnimatorPresent)
        ui.setBaseLayerStyleAnimatorInstance(Containers::pointer<BaseLayerStyleAnimator>(ui.createAnimator()));
    if(data.textLayerPresent)
        ui.setTextLayerInstance(Containers::pointer<LayerText>(ui.createLayer(), sharedText));
    if(data.textLayerStyleAnimatorPresent)
        ui.setTextLayerStyleAnimatorInstance(Containers::pointer<TextLayerStyleAnimator>(ui.createAnimator()));
    if(data.eventLayerPresent)
        ui.setEventLayerInstance(Containers::pointer<EventLayer>(ui.createLayer()));
    if(data.layoutLayerPresent)
        ui.setLayoutLayerInstance(Containers::pointer<LayoutLayer>(ui.createLayer(), 13u));
    if(data.snapLayouterPresent)
        ui.setSnapLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));
    if(data.genericLayouterPresent)
        ui.setGenericLayouterInstance(Containers::pointer<GenericLayouter>(ui.createLayouter()));
    if(data.nodeAnimatorPresent)
        ui.setNodeAnimatorInstance(Containers::pointer<NodeAnimator>(ui.createAnimator()));

    Int applyCalled = 0;
    struct Theme: AbstractTheme {
        Theme(Int& applyCalled, ThemeFeatures expectedFeatures, bool succeed): _applyCalled(applyCalled), _expectedFeatures{expectedFeatures}, _succeed{succeed} {}

        ThemeFeatures doFeatures() const override {
            return
                ThemeFeature::DataLayer|
                ThemeFeature::BackgroundLayer|ThemeFeature::BackgroundLayerAnimations|
                ThemeFeature::BaseLayer|ThemeFeature::BaseLayerAnimations|
                ThemeFeature::TextLayer|ThemeFeature::TextLayerAnimations|
                ThemeFeature::EventLayer|ThemeFeature::LayoutLayer|
                ThemeFeature::SnapLayouter|ThemeFeature::GenericLayouter|
                ThemeFeature::NodeAnimations;
        }
        UnsignedInt doBackgroundLayerStyleUniformCount() const override { return 17; }
        UnsignedInt doBackgroundLayerStyleCount() const override { return 19; }
        UnsignedInt doBackgroundLayerDynamicStyleCount() const override { return 23; }
        UnsignedInt doBackgroundLayerBlurRadius() const override { return 8; }
        Float doBackgroundLayerBlurCutoff() const override { return 0.5f; }
        UnsignedInt doBaseLayerStyleUniformCount() const override { return 3; }
        UnsignedInt doBaseLayerStyleCount() const override { return 5; }
        UnsignedInt doBaseLayerDynamicStyleCount() const override { return 11; }
        UnsignedInt doTextLayerStyleUniformCount() const override { return 2; }
        UnsignedInt doTextLayerStyleCount() const override { return 4; }
        UnsignedInt doTextLayerEditingStyleUniformCount() const override { return 6; }
        UnsignedInt doTextLayerEditingStyleCount() const override { return 7; }
        UnsignedInt doTextLayerDynamicStyleCount() const override { return 9; }
        PixelFormat doTextLayerGlyphCacheFormat() const override { return PixelFormat::R16F; }
        Vector3i doTextLayerGlyphCacheSize(ThemeFeatures) const override { return {16, 24, 2}; }
        Vector2i doTextLayerGlyphCachePadding() const override { return {3, 1}; }
        UnsignedInt doLayoutLayerStyleCount() const override { return 13; }
        bool doApply(UserInterface&, ThemeFeatures features, PluginManager::Manager<Text::AbstractFont>* fontManager) const override {
            CORRADE_COMPARE(features, _expectedFeatures);
            if(features >= ThemeFeature::TextLayer)
                CORRADE_VERIFY(fontManager);
            ++_applyCalled;
            return _succeed;
        }

        Int& _applyCalled;
        ThemeFeatures _expectedFeatures;
        bool _succeed;
    } theme{applyCalled, data.features, data.succeed};

    CORRADE_COMPARE(theme.apply(ui, data.features, data.features >= ThemeFeature::TextLayer ? &_fontManager : nullptr), data.succeed);
    CORRADE_COMPARE(applyCalled, 1);
}

void AbstractThemeTest::applyNoFeatures() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};

    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override { return ThemeFeature::TextLayer; }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } theme;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::String out;
    Error redirectError{&out};
    theme.apply(ui, {}, nullptr);
    CORRADE_COMPARE(out, "Ui::AbstractTheme::apply(): no features specified\n");
}

void AbstractThemeTest::applyFeaturesNotSupported() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};

    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override { return ThemeFeature::TextLayer; }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } theme;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::String out;
    Error redirectError{&out};
    theme.apply(ui, ThemeFeature::TextLayer|ThemeFeature::BaseLayer, nullptr);
    CORRADE_COMPARE(out, "Ui::AbstractTheme::apply(): Ui::ThemeFeature::BaseLayer|Ui::ThemeFeature::TextLayer not a subset of supported Ui::ThemeFeature::TextLayer\n");
}

void AbstractThemeTest::applyNoSizeSet() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};

    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override { return ThemeFeature::TextLayer; }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } theme;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::String out;
    Error redirectError{&out};
    theme.apply(ui, ThemeFeature::TextLayer, nullptr);
    CORRADE_COMPARE(out, "Ui::AbstractTheme::apply(): user interface size wasn't set\n");
}

void AbstractThemeTest::applyDataLayerNotPresent() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    ui.setSize({200, 300})
      /* Have two other layers present to test that it's not just checking if
         any layers are there at all */
      .setLayoutLayerInstance(Containers::pointer<LayoutLayer>(ui.createLayer(), 3u))
      .setEventLayerInstance(Containers::pointer<EventLayer>(ui.createLayer()));

    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override {
            return ThemeFeature::DataLayer;
        }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } theme;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::String out;
    Error redirectError{&out};
    theme.apply(ui, ThemeFeature::DataLayer, nullptr);
    CORRADE_COMPARE(out, "Ui::AbstractTheme::apply(): data layer not present in the user interface\n");
}

void AbstractThemeTest::applyBackgroundLayerNotPresent() {
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
      /* Having base layer present (which is used for ui.backgroundLayer() if
         background layer isn't present) shouldn't be treated as if the
         background layer is there */
      .setBaseLayerInstance(Containers::pointer<Layer>(ui.createLayer(), shared))
      .setEventLayerInstance(Containers::pointer<EventLayer>(ui.createLayer()));

    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override {
            return ThemeFeature::BackgroundLayer;
        }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } theme;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::String out;
    Error redirectError{&out};
    theme.apply(ui, ThemeFeature::BackgroundLayer, nullptr);
    CORRADE_COMPARE(out, "Ui::AbstractTheme::apply(): background layer not present in the user interface\n");
}

void AbstractThemeTest::applyBackgroundLayerDifferentStyleCount() {
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
      .setBackgroundLayerInstance(Containers::pointer<Layer>(ui.createLayer(), shared));

    struct Theme: AbstractTheme {
        Theme(UnsignedInt styleUniformCount, UnsignedInt styleCount, UnsignedInt dynamicStyleCount): _styleUniformCount{styleUniformCount}, _styleCount{styleCount}, _dynamicStyleCount{dynamicStyleCount} {}

        ThemeFeatures doFeatures() const override {
            return ThemeFeature::BackgroundLayer;
        }
        UnsignedInt doBackgroundLayerStyleUniformCount() const override { return _styleUniformCount; }
        UnsignedInt doBackgroundLayerStyleCount() const override { return _styleCount; }
        UnsignedInt doBackgroundLayerDynamicStyleCount() const override { return _dynamicStyleCount; }
        bool doApply(UserInterface& ui, ThemeFeatures, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL_IF(
                ui.backgroundLayer().shared().styleCount() != _styleCount ||
                ui.backgroundLayer().shared().styleUniformCount() != _styleUniformCount ||
                ui.backgroundLayer().shared().dynamicStyleCount() < _dynamicStyleCount,
                "This shouldn't get called.");
            return true;
        }

        UnsignedInt _styleUniformCount, _styleCount, _dynamicStyleCount;
    };

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    /* Applying a theme with a smaller or equal dynamic style count is
       alright */
    CORRADE_VERIFY(Theme{3, 5, 11}
        .apply(ui, ThemeFeature::BackgroundLayer, nullptr));
    CORRADE_VERIFY(Theme{3, 5, 10}
        .apply(ui, ThemeFeature::BackgroundLayer, nullptr));

    Containers::String out;
    Error redirectError{&out};
    Theme{4, 5, 11}
        .apply(ui, ThemeFeature::BackgroundLayer, nullptr);
    Theme{3, 4, 11}
        .apply(ui, ThemeFeature::BackgroundLayer, nullptr);
    Theme{3, 5, 12}
        .apply(ui, ThemeFeature::BackgroundLayer, nullptr);
    CORRADE_COMPARE_AS(out,
        "Ui::AbstractTheme::apply(): theme wants 4 uniforms, 5 styles and at least 11 dynamic styles but the background layer has 3, 5 and 11\n"
        "Ui::AbstractTheme::apply(): theme wants 3 uniforms, 4 styles and at least 11 dynamic styles but the background layer has 3, 5 and 11\n"
        "Ui::AbstractTheme::apply(): theme wants 3 uniforms, 5 styles and at least 12 dynamic styles but the background layer has 3, 5 and 11\n",
        TestSuite::Compare::String);
}

void AbstractThemeTest::applyBackgroundLayerStyleAnimatorNotPresent() {
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
      .setBackgroundLayerInstance(Containers::pointer<Layer>(ui.createLayer(), shared));

    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override {
            return ThemeFeature::BackgroundLayerAnimations;
        }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } theme;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::String out;
    Error redirectError{&out};
    theme.apply(ui, ThemeFeature::BackgroundLayerAnimations, nullptr);
    CORRADE_COMPARE(out, "Ui::AbstractTheme::apply(): background layer style animator not present in the user interface\n");
}

void AbstractThemeTest::applyBaseLayerNotPresent() {
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
      /* Having background layer present shouldn't be treated as if the base
         layer is there */
      .setBackgroundLayerInstance(Containers::pointer<Layer>(ui.createLayer(), shared))
      .setEventLayerInstance(Containers::pointer<EventLayer>(ui.createLayer()));

    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override { return ThemeFeature::BaseLayer; }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } theme;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::String out;
    Error redirectError{&out};
    theme.apply(ui, ThemeFeature::BaseLayer, nullptr);
    CORRADE_COMPARE(out, "Ui::AbstractTheme::apply(): base layer not present in the user interface\n");
}

void AbstractThemeTest::applyBaseLayerDifferentStyleCount() {
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

    struct Theme: AbstractTheme {
        Theme(UnsignedInt styleUniformCount, UnsignedInt styleCount, UnsignedInt dynamicStyleCount): _styleUniformCount{styleUniformCount}, _styleCount{styleCount}, _dynamicStyleCount{dynamicStyleCount} {}

        ThemeFeatures doFeatures() const override { return ThemeFeature::BaseLayer; }
        UnsignedInt doBaseLayerStyleUniformCount() const override { return _styleUniformCount; }
        UnsignedInt doBaseLayerStyleCount() const override { return _styleCount; }
        UnsignedInt doBaseLayerDynamicStyleCount() const override { return _dynamicStyleCount; }
        bool doApply(UserInterface& ui, ThemeFeatures, PluginManager::Manager<Text::AbstractFont>*) const override {
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

    /* Applying a theme with a smaller or equal dynamic style count is
       alright */
    CORRADE_VERIFY(Theme{3, 5, 11}
        .apply(ui, ThemeFeature::BaseLayer, nullptr));
    CORRADE_VERIFY(Theme{3, 5, 10}
        .apply(ui, ThemeFeature::BaseLayer, nullptr));

    Containers::String out;
    Error redirectError{&out};
    Theme{4, 5, 11}
        .apply(ui, ThemeFeature::BaseLayer, nullptr);
    Theme{3, 4, 11}
        .apply(ui, ThemeFeature::BaseLayer, nullptr);
    Theme{3, 5, 12}
        .apply(ui, ThemeFeature::BaseLayer, nullptr);
    CORRADE_COMPARE_AS(out,
        "Ui::AbstractTheme::apply(): theme wants 4 uniforms, 5 styles and at least 11 dynamic styles but the base layer has 3, 5 and 11\n"
        "Ui::AbstractTheme::apply(): theme wants 3 uniforms, 4 styles and at least 11 dynamic styles but the base layer has 3, 5 and 11\n"
        "Ui::AbstractTheme::apply(): theme wants 3 uniforms, 5 styles and at least 12 dynamic styles but the base layer has 3, 5 and 11\n",
        TestSuite::Compare::String);
}

void AbstractThemeTest::applyBaseLayerStyleAnimatorNotPresent() {
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

    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override { return ThemeFeature::BaseLayerAnimations; }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } theme;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::String out;
    Error redirectError{&out};
    theme.apply(ui, ThemeFeature::BaseLayerAnimations, nullptr);
    CORRADE_COMPARE(out, "Ui::AbstractTheme::apply(): base layer style animator not present in the user interface\n");
}

void AbstractThemeTest::applyTextLayerNotPresent() {
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

    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override { return ThemeFeature::TextLayer; }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } theme;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::String out;
    Error redirectError{&out};
    theme.apply(ui, ThemeFeature::TextLayer, nullptr);
    CORRADE_COMPARE(out, "Ui::AbstractTheme::apply(): text layer not present in the user interface\n");
}

void AbstractThemeTest::applyTextLayerDifferentStyleCount() {
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

    struct Theme: AbstractTheme {
        Theme(UnsignedInt styleUniformCount, UnsignedInt styleCount, UnsignedInt editingStyleUniformCount, UnsignedInt editingStyleCount, UnsignedInt dynamicStyleCount): _styleUniformCount{styleUniformCount}, _styleCount{styleCount}, _editingStyleUniformCount{editingStyleUniformCount}, _editingStyleCount{editingStyleCount}, _dynamicStyleCount{dynamicStyleCount} {}

        ThemeFeatures doFeatures() const override { return ThemeFeature::TextLayer; }
        UnsignedInt doTextLayerStyleUniformCount() const override { return _styleUniformCount; }
        UnsignedInt doTextLayerStyleCount() const override { return _styleCount; }
        UnsignedInt doTextLayerEditingStyleUniformCount() const override { return _editingStyleUniformCount; }
        UnsignedInt doTextLayerEditingStyleCount() const override { return _editingStyleCount; }
        UnsignedInt doTextLayerDynamicStyleCount() const override { return _dynamicStyleCount; }
        Vector3i doTextLayerGlyphCacheSize(ThemeFeatures) const override { return {16, 16, 1}; }
        bool doApply(UserInterface& ui, ThemeFeatures, PluginManager::Manager<Text::AbstractFont>*) const override {
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

    /* Applying a theme with a smaller or equal dynamic style count is
       alright */
    CORRADE_VERIFY(Theme{3, 5, 7, 2, 11}
        .apply(ui, ThemeFeature::TextLayer, &_fontManager));
    CORRADE_VERIFY(Theme{3, 5, 7, 2, 10}
        .apply(ui, ThemeFeature::TextLayer, &_fontManager));

    Containers::String out;
    Error redirectError{&out};
    Theme{4, 5, 7, 2, 11}
        .apply(ui, ThemeFeature::TextLayer, nullptr);
    Theme{3, 4, 7, 2, 11}
        .apply(ui, ThemeFeature::TextLayer, nullptr);
    Theme{3, 5, 8, 2, 11}
        .apply(ui, ThemeFeature::TextLayer, nullptr);
    Theme{3, 5, 7, 1, 11}
        .apply(ui, ThemeFeature::TextLayer, nullptr);
    Theme{3, 5, 7, 2, 12}
        .apply(ui, ThemeFeature::TextLayer, nullptr);
    CORRADE_COMPARE_AS(out,
        "Ui::AbstractTheme::apply(): theme wants 4 uniforms, 5 styles, 7 editing uniforms, 2 editing styles and at least 11 dynamic styles but the text layer has 3, 5, 7, 2 and 11\n"
        "Ui::AbstractTheme::apply(): theme wants 3 uniforms, 4 styles, 7 editing uniforms, 2 editing styles and at least 11 dynamic styles but the text layer has 3, 5, 7, 2 and 11\n"
        "Ui::AbstractTheme::apply(): theme wants 3 uniforms, 5 styles, 8 editing uniforms, 2 editing styles and at least 11 dynamic styles but the text layer has 3, 5, 7, 2 and 11\n"
        "Ui::AbstractTheme::apply(): theme wants 3 uniforms, 5 styles, 7 editing uniforms, 1 editing styles and at least 11 dynamic styles but the text layer has 3, 5, 7, 2 and 11\n"
        "Ui::AbstractTheme::apply(): theme wants 3 uniforms, 5 styles, 7 editing uniforms, 2 editing styles and at least 12 dynamic styles but the text layer has 3, 5, 7, 2 and 11\n",
        TestSuite::Compare::String);
}

void AbstractThemeTest::applyTextLayerDifferentGlyphCache() {
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

    struct Theme: AbstractTheme {
        Theme(PixelFormat format, const Vector3i& size, const Vector2i& padding): _format{format}, _size{size}, _padding{padding} {}

        ThemeFeatures doFeatures() const override { return ThemeFeature::TextLayer; }
        UnsignedInt doTextLayerStyleCount() const override { return 1; }
        PixelFormat doTextLayerGlyphCacheFormat() const override { return _format; }
        Vector3i doTextLayerGlyphCacheSize(ThemeFeatures) const override { return _size; }
        Vector2i doTextLayerGlyphCachePadding() const override { return _padding; }
        bool doApply(UserInterface& ui, ThemeFeatures, PluginManager::Manager<Text::AbstractFont>*) const override {
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

    /* Applying a theme with a smaller or equal size or padding is alright */
    CORRADE_VERIFY(Theme{PixelFormat::RG16F, {3, 5, 2}, {4, 1}}
        .apply(ui, ThemeFeature::TextLayer, &_fontManager));
    CORRADE_VERIFY(Theme{PixelFormat::RG16F, {3, 5, 2}, {4, 0}}
        .apply(ui, ThemeFeature::TextLayer, &_fontManager));
    CORRADE_VERIFY(Theme{PixelFormat::RG16F, {3, 5, 2}, {3, 1}}
        .apply(ui, ThemeFeature::TextLayer, &_fontManager));
    CORRADE_VERIFY(Theme{PixelFormat::RG16F, {3, 5, 1}, {4, 1}}
        .apply(ui, ThemeFeature::TextLayer, &_fontManager));
    CORRADE_VERIFY(Theme{PixelFormat::RG16F, {3, 4, 2}, {4, 1}}
        .apply(ui, ThemeFeature::TextLayer, &_fontManager));
    CORRADE_VERIFY(Theme{PixelFormat::RG16F, {2, 5, 2}, {4, 1}}
        .apply(ui, ThemeFeature::TextLayer, &_fontManager));

    Containers::String out;
    Error redirectError{&out};
    Theme{PixelFormat::R8Unorm, {3, 5, 2}, {4, 1}}
        .apply(ui, ThemeFeature::TextLayer, nullptr);
    Theme{PixelFormat::RG16F, {4, 5, 2}, {4, 1}}
        .apply(ui, ThemeFeature::TextLayer, nullptr);
    Theme{PixelFormat::RG16F, {3, 6, 2}, {4, 1}}
        .apply(ui, ThemeFeature::TextLayer, nullptr);
    Theme{PixelFormat::RG16F, {3, 5, 3}, {4, 1}}
        .apply(ui, ThemeFeature::TextLayer, nullptr);
    Theme{PixelFormat::RG16F, {3, 5, 2}, {5, 1}}
        .apply(ui, ThemeFeature::TextLayer, nullptr);
    Theme{PixelFormat::RG16F, {3, 5, 2}, {4, 2}}
        .apply(ui, ThemeFeature::TextLayer, nullptr);
    CORRADE_COMPARE_AS(out,
        "Ui::AbstractTheme::apply(): theme wants a PixelFormat::R8Unorm glyph cache of size at least {3, 5, 2} and padding at least {4, 1} but the text layer has PixelFormat::RG16F, {3, 5, 2} and {4, 1}\n"
       "Ui::AbstractTheme::apply(): theme wants a PixelFormat::RG16F glyph cache of size at least {4, 5, 2} and padding at least {4, 1} but the text layer has PixelFormat::RG16F, {3, 5, 2} and {4, 1}\n"
       "Ui::AbstractTheme::apply(): theme wants a PixelFormat::RG16F glyph cache of size at least {3, 6, 2} and padding at least {4, 1} but the text layer has PixelFormat::RG16F, {3, 5, 2} and {4, 1}\n"
       "Ui::AbstractTheme::apply(): theme wants a PixelFormat::RG16F glyph cache of size at least {3, 5, 3} and padding at least {4, 1} but the text layer has PixelFormat::RG16F, {3, 5, 2} and {4, 1}\n"
       "Ui::AbstractTheme::apply(): theme wants a PixelFormat::RG16F glyph cache of size at least {3, 5, 2} and padding at least {5, 1} but the text layer has PixelFormat::RG16F, {3, 5, 2} and {4, 1}\n"
       "Ui::AbstractTheme::apply(): theme wants a PixelFormat::RG16F glyph cache of size at least {3, 5, 2} and padding at least {4, 2} but the text layer has PixelFormat::RG16F, {3, 5, 2} and {4, 1}\n",
        TestSuite::Compare::String);
}

void AbstractThemeTest::applyTextLayerNoFontManager() {
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

    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override { return ThemeFeature::TextLayer; }
        Vector3i doTextLayerGlyphCacheSize(ThemeFeatures) const override { return {16, 16, 1}; }
        UnsignedInt doTextLayerStyleCount() const override { return 1; }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } theme;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::String out;
    Error redirectError{&out};
    theme.apply(ui, ThemeFeature::TextLayer, nullptr);
    CORRADE_COMPARE(out, "Ui::AbstractTheme::apply(): fontManager has to be specified for applying a text layer style\n");
}

void AbstractThemeTest::applyTextLayerStyleAnimatorNotPresent() {
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

    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override { return ThemeFeature::TextLayerAnimations; }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } theme;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::String out;
    Error redirectError{&out};
    theme.apply(ui, ThemeFeature::TextLayerAnimations, nullptr);
    CORRADE_COMPARE(out, "Ui::AbstractTheme::apply(): text layer style animator not present in the user interface\n");
}

void AbstractThemeTest::applyEventLayerNotPresent() {
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

    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override { return ThemeFeature::EventLayer; }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } theme;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::String out;
    Error redirectError{&out};
    theme.apply(ui, ThemeFeature::EventLayer, nullptr);
    CORRADE_COMPARE(out, "Ui::AbstractTheme::apply(): event layer not present in the user interface\n");
}

void AbstractThemeTest::applyLayoutLayerNotPresent() {
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

    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override { return ThemeFeature::LayoutLayer; }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } theme;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::String out;
    Error redirectError{&out};
    theme.apply(ui, ThemeFeature::LayoutLayer, nullptr);
    CORRADE_COMPARE(out, "Ui::AbstractTheme::apply(): layout layer not present in the user interface\n");
}

void AbstractThemeTest::applyLayoutLayerDifferentStyleCount() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    ui.setSize({200, 300})
      .setLayoutLayerInstance(Containers::pointer<LayoutLayer>(ui.createLayer(), 7u));

    /* Passing the style count via a param like with BaseLayer just in case
       more variants would get added in the future */
    struct Theme: AbstractTheme {
        Theme(UnsignedInt styleCount): _styleCount{styleCount} {}

        ThemeFeatures doFeatures() const override { return ThemeFeature::LayoutLayer; }
        UnsignedInt doLayoutLayerStyleCount() const override { return _styleCount; }
        bool doApply(UserInterface& ui, ThemeFeatures, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL_IF(
                ui.layoutLayer().styleCount() != _styleCount,
                "This shouldn't get called.");
            return true;
        }

        UnsignedInt _styleCount;
    };

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::String out;
    Error redirectError{&out};
    Theme{6}
        .apply(ui, ThemeFeature::LayoutLayer, nullptr);
    CORRADE_COMPARE_AS(out,
        "Ui::AbstractTheme::apply(): theme wants 6 styles but the layout layer has 7\n",
        TestSuite::Compare::String);
}

void AbstractThemeTest::applySnapLayouterNotPresent() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    ui.setSize({200, 300})
      .setGenericLayouterInstance(Containers::pointer<GenericLayouter>(ui.createLayouter()));

    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override { return ThemeFeature::SnapLayouter; }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } theme;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::String out;
    Error redirectError{&out};
    theme.apply(ui, ThemeFeature::SnapLayouter, nullptr);
    CORRADE_COMPARE(out, "Ui::AbstractTheme::apply(): snap layouter not present in the user interface\n");
}

void AbstractThemeTest::applyGenericLayouterNotPresent() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    ui.setSize({200, 300})
      .setSnapLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));

    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override { return ThemeFeature::GenericLayouter; }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } theme;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::String out;
    Error redirectError{&out};
    theme.apply(ui, ThemeFeature::GenericLayouter, nullptr);
    CORRADE_COMPARE(out, "Ui::AbstractTheme::apply(): generic layouter not present in the user interface\n");
}

void AbstractThemeTest::applyNodeAnimatorNotPresent() {
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
      .setBaseLayerInstance(Containers::pointer<Layer>(ui.createLayer(), shared))
      .setBaseLayerStyleAnimatorInstance(Containers::pointer<BaseLayerStyleAnimator>(ui.createAnimator()));

    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override {
            return ThemeFeature::NodeAnimations;
        }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } theme;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::String out;
    Error redirectError{&out};
    theme.apply(ui, ThemeFeature::NodeAnimations, nullptr);
    CORRADE_COMPARE(out, "Ui::AbstractTheme::apply(): node animator not present in the user interface\n");
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::AbstractThemeTest)
