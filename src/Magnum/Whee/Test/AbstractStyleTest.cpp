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
#include <Corrade/Containers/StringStl.h> /** @todo remove once Debug is stream-free */
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/String.h>
#include <Corrade/Utility/DebugStl.h> /** @todo remove once Debug is stream-free */
#include <Magnum/PixelFormat.h>
#include <Magnum/Text/AbstractFont.h>
#include <Magnum/Text/AbstractGlyphCache.h>

#include "Magnum/Whee/AbstractStyle.h"
#include "Magnum/Whee/BaseLayer.h"
#include "Magnum/Whee/EventLayer.h"
#include "Magnum/Whee/TextLayer.h"
#include "Magnum/Whee/UserInterface.h"

namespace Magnum { namespace Whee { namespace Test { namespace {

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

    void textLayerGlyphCacheProperties();
    void textLayerGlyphCachePropertiesNotSupported();
    void textLayerGlyphCachePropertiesNotImplemented();
    void textLayerGlyphCachePropertiesNotImplementedDefaults();
    void setTextLayerGlyphCacheSize();

    void apply();
    void applyNoFeatures();
    void applyFeaturesNotSupported();
    void applyBaseLayerNotPresent();
    void applyBaseLayerDifferentStyleCount();
    void applyTextLayerNotPresent();
    void applyTextLayerDifferentStyleCount();
    void applyTextLayerNoGlyphCache();
    void applyTextLayerDifferentGlyphCache();
    void applyTextLayerNoFontManager();
    void applyEventLayerNotPresent();

    private:
        PluginManager::Manager<Text::AbstractFont> _fontManager;
};

const struct {
    const char* name;
    bool baseLayerPresent, textLayerPresent, eventLayerPresent;
    StyleFeatures features;
    bool succeed;
} ApplyData[]{
    {"base layer only", true, false, false,
        StyleFeature::BaseLayer, true},
    {"text layer only", false, true, false,
        StyleFeature::TextLayer, true},
    {"event layer only", false, false, true,
        StyleFeature::EventLayer, true},
    {"everything except base layer", false, true, true,
        ~StyleFeature::BaseLayer, true},
    {"everything", true, true, true,
        ~StyleFeatures{}, true},
    {"application failed", true, false, false,
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

              &AbstractStyleTest::textLayerGlyphCacheProperties,
              &AbstractStyleTest::textLayerGlyphCachePropertiesNotSupported,
              &AbstractStyleTest::textLayerGlyphCachePropertiesNotImplemented,
              &AbstractStyleTest::textLayerGlyphCachePropertiesNotImplementedDefaults,
              &AbstractStyleTest::setTextLayerGlyphCacheSize});

    addInstancedTests({&AbstractStyleTest::apply},
        Containers::arraySize(ApplyData));

    addTests({&AbstractStyleTest::applyNoFeatures,
              &AbstractStyleTest::applyFeaturesNotSupported,
              &AbstractStyleTest::applyBaseLayerNotPresent,
              &AbstractStyleTest::applyBaseLayerDifferentStyleCount,
              &AbstractStyleTest::applyTextLayerNotPresent,
              &AbstractStyleTest::applyTextLayerDifferentStyleCount,
              &AbstractStyleTest::applyTextLayerNoGlyphCache,
              &AbstractStyleTest::applyTextLayerDifferentGlyphCache,
              &AbstractStyleTest::applyTextLayerNoFontManager,
              &AbstractStyleTest::applyEventLayerNotPresent});
}

void AbstractStyleTest::debugFeature() {
    std::ostringstream out;
    Debug{&out} << StyleFeature::BaseLayer << StyleFeature(0xbe);
    CORRADE_COMPARE(out.str(), "Whee::StyleFeature::BaseLayer Whee::StyleFeature(0xbe)\n");
}

void AbstractStyleTest::debugFeatures() {
    std::ostringstream out;
    Debug{&out} << (StyleFeature::TextLayer|StyleFeature(0xe0)) << StyleFeatures{};
    CORRADE_COMPARE(out.str(), "Whee::StyleFeature::TextLayer|Whee::StyleFeature(0xe0) Whee::StyleFeatures{}\n");
}

void AbstractStyleTest::construct() {
    struct: AbstractStyle {
        StyleFeatures doFeatures() const override { return StyleFeature::BaseLayer; }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }
    } style;
    CORRADE_COMPARE(style.features(), StyleFeature::BaseLayer);
}

void AbstractStyleTest::constructCopy() {
    struct Style: AbstractStyle {
        explicit Style(StyleFeatures features): _features{features} {}

        StyleFeatures doFeatures() const override { return _features; }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }

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
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }
    } style;

    std::ostringstream out;
    Error redirectError{&out};
    style.features();
    CORRADE_COMPARE(out.str(), "Whee::AbstractStyle::features(): implementation returned an empty set\n");
}

void AbstractStyleTest::styleCount() {
    struct: AbstractStyle {
        StyleFeatures doFeatures() const override {
            return StyleFeature::BaseLayer|StyleFeature::TextLayer;
        }
        UnsignedInt doBaseLayerStyleUniformCount() const override { return 3; }
        UnsignedInt doBaseLayerStyleCount() const override { return 5; }
        UnsignedInt doTextLayerStyleUniformCount() const override { return 7; }
        UnsignedInt doTextLayerStyleCount() const override { return 9; }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }
    } style;
    CORRADE_COMPARE(style.baseLayerStyleUniformCount(), 3);
    CORRADE_COMPARE(style.baseLayerStyleCount(), 5);
    CORRADE_COMPARE(style.textLayerStyleUniformCount(), 7);
    CORRADE_COMPARE(style.textLayerStyleCount(), 9);
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
        UnsignedInt doTextLayerStyleUniformCount() const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
        UnsignedInt doTextLayerStyleCount() const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }
    } style;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    std::ostringstream out;
    Error redirectError{&out};
    style.baseLayerStyleUniformCount();
    style.baseLayerStyleCount();
    style.textLayerStyleUniformCount();
    style.textLayerStyleCount();
    CORRADE_COMPARE_AS(out.str(),
        "Whee::AbstractStyle::baseLayerStyleUniformCount(): feature not supported\n"
        "Whee::AbstractStyle::baseLayerStyleCount(): feature not supported\n"
        "Whee::AbstractStyle::textLayerStyleUniformCount(): feature not supported\n"
        "Whee::AbstractStyle::textLayerStyleCount(): feature not supported\n",
        TestSuite::Compare::String);
}

void AbstractStyleTest::styleCountNotImplemented() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractStyle {
        StyleFeatures doFeatures() const override {
            return StyleFeature::BaseLayer|StyleFeature::TextLayer;
        }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }
    } style;

    std::ostringstream out;
    Error redirectError{&out};
    /* The *UniformCount() delegate to *Count() by default, so the assertion
       is the same. Delegation and value propagation tested below. */
    style.baseLayerStyleUniformCount();
    style.baseLayerStyleCount();
    style.textLayerStyleUniformCount();
    style.textLayerStyleCount();
    CORRADE_COMPARE_AS(out.str(),
        "Whee::AbstractStyle::baseLayerStyleCount(): feature advertised but not implemented\n"
        "Whee::AbstractStyle::baseLayerStyleCount(): feature advertised but not implemented\n"
        "Whee::AbstractStyle::textLayerStyleCount(): feature advertised but not implemented\n"
        "Whee::AbstractStyle::textLayerStyleCount(): feature advertised but not implemented\n",
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
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }
    } style;

    /* With baseLayerStyleCount() / textLayerStyleCount() not implemented it
       would assert, tested above */
    CORRADE_COMPARE(style.baseLayerStyleUniformCount(), 17);
    CORRADE_COMPARE(style.textLayerStyleUniformCount(), 35);
}

void AbstractStyleTest::textLayerGlyphCacheProperties() {
    struct: AbstractStyle {
        StyleFeatures doFeatures() const override {
            return StyleFeature::TextLayer;
        }
        PixelFormat doTextLayerGlyphCacheFormat() const override {
            return PixelFormat::RG32F;
        }
        Vector3i doTextLayerGlyphCacheSize() const override {
            return {3, 5, 18};
        }
        Vector2i doTextLayerGlyphCachePadding() const override {
            return {2, 4};
        }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }
    } style;
    CORRADE_COMPARE(style.textLayerGlyphCacheFormat(), PixelFormat::RG32F);
    CORRADE_COMPARE(style.textLayerGlyphCacheSize(), (Vector3i{3, 5, 18}));
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
        Vector3i doTextLayerGlyphCacheSize() const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
        Vector2i doTextLayerGlyphCachePadding() const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }
    } style;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    std::ostringstream out;
    Error redirectError{&out};
    style.textLayerGlyphCacheFormat();
    style.textLayerGlyphCacheSize();
    style.textLayerGlyphCachePadding();
    CORRADE_COMPARE_AS(out.str(),
        "Whee::AbstractStyle::textLayerGlyphCacheFormat(): feature not supported\n"
        "Whee::AbstractStyle::textLayerGlyphCacheSize(): feature not supported\n"
        "Whee::AbstractStyle::textLayerGlyphCachePadding(): feature not supported\n",
        TestSuite::Compare::String);
}

void AbstractStyleTest::textLayerGlyphCachePropertiesNotImplemented() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractStyle {
        StyleFeatures doFeatures() const override {
            return StyleFeature::TextLayer;
        }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }
    } style;

    std::ostringstream out;
    Error redirectError{&out};
    /* textLayerGlyphCacheFormat() and textLayerGlyphCachePadding() have
       defaults, tested below */
    style.textLayerGlyphCacheSize();
    CORRADE_COMPARE(out.str(), "Whee::AbstractStyle::textLayerGlyphCacheSize(): feature advertised but not implemented\n");
}

void AbstractStyleTest::textLayerGlyphCachePropertiesNotImplementedDefaults() {
    struct: AbstractStyle {
        StyleFeatures doFeatures() const override {
            return StyleFeature::TextLayer;
        }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }
    } style;

    CORRADE_COMPARE(style.textLayerGlyphCacheFormat(), PixelFormat::R8Unorm);
    /* Padding is 1 by default, consistently with Text::AbstractGlyphCache */
    CORRADE_COMPARE(style.textLayerGlyphCachePadding(), Vector2i{1});
    /* textLayerGlyphCacheSize() asserts, tested above */
}

void AbstractStyleTest::setTextLayerGlyphCacheSize() {
    struct: AbstractStyle {
        StyleFeatures doFeatures() const override {
            return StyleFeature::TextLayer;
        }
        Vector3i doTextLayerGlyphCacheSize() const override {
            return {16, 32, 8};
        }
        Vector2i doTextLayerGlyphCachePadding() const override {
            return {4, 2};
        }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Text::AbstractFont>*) const override { return {}; }
    } style;

    /* By default it returns what the style says */
    CORRADE_COMPARE(style.textLayerGlyphCacheSize(), (Vector3i{16, 32, 8}));
    CORRADE_COMPARE(style.textLayerGlyphCachePadding(), (Vector2i{4, 2}));

    /* Setting a new value */
    style.setTextLayerGlyphCacheSize({48, 56, 12}, {6, 8});
    CORRADE_COMPARE(style.textLayerGlyphCacheSize(), (Vector3i{48, 56, 12}));
    CORRADE_COMPARE(style.textLayerGlyphCachePadding(), (Vector2i{6, 8}));

    /* Setting a new but smaller value than before */
    style.setTextLayerGlyphCacheSize({24, 48, 10}, {5, 3});
    CORRADE_COMPARE(style.textLayerGlyphCacheSize(), (Vector3i{24, 48, 10}));
    CORRADE_COMPARE(style.textLayerGlyphCachePadding(), (Vector2i{5, 3}));

    /* Setting a value smaller than what style says picks the style instead */
    style.setTextLayerGlyphCacheSize({}, {});
    CORRADE_COMPARE(style.textLayerGlyphCacheSize(), (Vector3i{16, 32, 8}));
    CORRADE_COMPARE(style.textLayerGlyphCachePadding(), (Vector2i{4, 2}));

    /* Setting a new value only picks the dimensions that are actually
       larger */
    style.setTextLayerGlyphCacheSize({12, 33, 6}, {5, 1});
    CORRADE_COMPARE(style.textLayerGlyphCacheSize(), (Vector3i{16, 33, 8}));
    CORRADE_COMPARE(style.textLayerGlyphCachePadding(), (Vector2i{5, 2}));

    style.setTextLayerGlyphCacheSize({17, 24, 6}, {3, 3});
    CORRADE_COMPARE(style.textLayerGlyphCacheSize(), (Vector3i{17, 32, 8}));
    CORRADE_COMPARE(style.textLayerGlyphCachePadding(), (Vector2i{4, 3}));

    style.setTextLayerGlyphCacheSize({12, 24, 12});
    CORRADE_COMPARE(style.textLayerGlyphCacheSize(), (Vector3i{16, 32, 12}));
    CORRADE_COMPARE(style.textLayerGlyphCachePadding(), (Vector2i{4, 2}));
}

void AbstractStyleTest::apply() {
    auto&& data = ApplyData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct LayerSharedBase: BaseLayer::Shared {
        explicit LayerSharedBase(UnsignedInt styleUniformCount, UnsignedInt styleCount): BaseLayer::Shared{styleUniformCount, styleCount} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } sharedBase{3, 5};

    struct LayerBase: BaseLayer {
        explicit LayerBase(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    };

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R16F, {16, 24, 2}, {3, 1}};

    struct LayerSharedText: TextLayer::Shared {
        explicit LayerSharedText(UnsignedInt styleUniformCount, UnsignedInt styleCount): TextLayer::Shared{styleUniformCount, styleCount} {}

        using TextLayer::Shared::setGlyphCache;

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    } sharedText{2, 4};
    sharedText.setGlyphCache(cache);

    struct LayerText: TextLayer {
        explicit LayerText(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    };

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    if(data.baseLayerPresent)
        ui.setBaseLayerInstance(Containers::pointer<LayerBase>(ui.createLayer(), sharedBase));
    if(data.textLayerPresent)
        ui.setTextLayerInstance(Containers::pointer<LayerText>(ui.createLayer(), sharedText));
    if(data.eventLayerPresent)
        ui.setEventLayerInstance(Containers::pointer<EventLayer>(ui.createLayer()));

    Int applyCalled = 0;
    struct Style: AbstractStyle {
        Style(Int& applyCalled, StyleFeatures expectedFeatures, bool succeed): _applyCalled(applyCalled), _expectedFeatures{expectedFeatures}, _succeed{succeed} {}

        StyleFeatures doFeatures() const override {
            return StyleFeature::BaseLayer|StyleFeature::TextLayer|StyleFeature::EventLayer;
        }
        UnsignedInt doBaseLayerStyleUniformCount() const override { return 3; }
        UnsignedInt doBaseLayerStyleCount() const override { return 5; }
        UnsignedInt doTextLayerStyleUniformCount() const override { return 2; }
        UnsignedInt doTextLayerStyleCount() const override { return 4; }
        PixelFormat doTextLayerGlyphCacheFormat() const override { return PixelFormat::R16F; }
        Vector3i doTextLayerGlyphCacheSize() const override { return {16, 24, 2}; }
        Vector2i doTextLayerGlyphCachePadding() const override { return {3, 1}; }
        bool doApply(UserInterface&, StyleFeatures features, PluginManager::Manager<Text::AbstractFont>* fontManager) const override {
            CORRADE_COMPARE(features, _expectedFeatures);
            if(features >= StyleFeature::TextLayer)
                CORRADE_VERIFY(fontManager);
            ++_applyCalled;
            return _succeed;
        }

        Int& _applyCalled;
        StyleFeatures _expectedFeatures;
        bool _succeed;
    } style{applyCalled, data.features, data.succeed};

    CORRADE_COMPARE(style.apply(ui, data.features, data.features & StyleFeature::TextLayer ? &_fontManager : nullptr), data.succeed);
    CORRADE_COMPARE(applyCalled, 1);
}

void AbstractStyleTest::applyNoFeatures() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};

    struct: AbstractStyle {
        StyleFeatures doFeatures() const override { return StyleFeature::TextLayer; }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } style;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    std::ostringstream out;
    Error redirectError{&out};
    style.apply(ui, {}, nullptr);
    CORRADE_COMPARE(out.str(), "Whee::AbstractStyle::apply(): no features specified\n");
}

void AbstractStyleTest::applyFeaturesNotSupported() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};

    struct: AbstractStyle {
        StyleFeatures doFeatures() const override { return StyleFeature::TextLayer; }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } style;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    std::ostringstream out;
    Error redirectError{&out};
    style.apply(ui, StyleFeature::TextLayer|StyleFeature::BaseLayer, nullptr);
    CORRADE_COMPARE(out.str(), "Whee::AbstractStyle::apply(): Whee::StyleFeature::BaseLayer|Whee::StyleFeature::TextLayer not a subset of supported Whee::StyleFeature::TextLayer\n");
}

void AbstractStyleTest::applyBaseLayerNotPresent() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(UnsignedInt styleUniformCount, UnsignedInt styleCount): TextLayer::Shared{styleUniformCount, styleCount} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    } shared{1, 3};

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    };

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    ui.setTextLayerInstance(Containers::pointer<Layer>(ui.createLayer(), shared));
    ui.setEventLayerInstance(Containers::pointer<EventLayer>(ui.createLayer()));

    struct: AbstractStyle {
        StyleFeatures doFeatures() const override { return StyleFeature::BaseLayer; }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } style;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    std::ostringstream out;
    Error redirectError{&out};
    style.apply(ui, StyleFeature::BaseLayer, nullptr);
    CORRADE_COMPARE(out.str(), "Whee::AbstractStyle::apply(): base layer not present in the user interface\n");
}

void AbstractStyleTest::applyBaseLayerDifferentStyleCount() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(UnsignedInt styleUniformCount, UnsignedInt styleCount): BaseLayer::Shared{styleUniformCount, styleCount} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } shared{3, 5};

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    };

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    ui.setBaseLayerInstance(Containers::pointer<Layer>(ui.createLayer(), shared));

    struct Style: AbstractStyle {
        Style(UnsignedInt styleUniformCount, UnsignedInt styleCount): _styleUniformCount{styleUniformCount}, _styleCount{styleCount} {}

        StyleFeatures doFeatures() const override { return StyleFeature::BaseLayer; }
        UnsignedInt doBaseLayerStyleUniformCount() const override { return _styleUniformCount; }
        UnsignedInt doBaseLayerStyleCount() const override { return _styleCount; }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }

        UnsignedInt _styleUniformCount, _styleCount;
    };

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    std::ostringstream out;
    Error redirectError{&out};
    Style{4, 5}.apply(ui, StyleFeature::BaseLayer, nullptr);
    Style{3, 4}.apply(ui, StyleFeature::BaseLayer, nullptr);
    CORRADE_COMPARE(out.str(),
        "Whee::AbstractStyle::apply(): style has 4 uniforms and 5 styles but the base layer has 3 and 5\n"
        "Whee::AbstractStyle::apply(): style has 3 uniforms and 4 styles but the base layer has 3 and 5\n");
}

void AbstractStyleTest::applyTextLayerNotPresent() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(UnsignedInt styleUniformCount, UnsignedInt styleCount): BaseLayer::Shared{styleUniformCount, styleCount} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } shared{3, 5};

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, LayerShared& shared): BaseLayer{handle, shared} {}
    };

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    ui.setBaseLayerInstance(Containers::pointer<Layer>(ui.createLayer(), shared));
    ui.setEventLayerInstance(Containers::pointer<EventLayer>(ui.createLayer()));

    struct: AbstractStyle {
        StyleFeatures doFeatures() const override { return StyleFeature::TextLayer; }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } style;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    std::ostringstream out;
    Error redirectError{&out};
    style.apply(ui, StyleFeature::TextLayer, nullptr);
    CORRADE_COMPARE(out.str(), "Whee::AbstractStyle::apply(): text layer not present in the user interface\n");
}

void AbstractStyleTest::applyTextLayerDifferentStyleCount() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(UnsignedInt styleUniformCount, UnsignedInt styleCount): TextLayer::Shared{styleUniformCount, styleCount} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    } shared{3, 5};

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    };

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    ui.setTextLayerInstance(Containers::pointer<Layer>(ui.createLayer(), shared));

    struct Style: AbstractStyle {
        Style(UnsignedInt styleUniformCount, UnsignedInt styleCount): _styleUniformCount{styleUniformCount}, _styleCount{styleCount} {}

        StyleFeatures doFeatures() const override { return StyleFeature::TextLayer; }
        UnsignedInt doTextLayerStyleUniformCount() const override { return _styleUniformCount; }
        UnsignedInt doTextLayerStyleCount() const override { return _styleCount; }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }

        UnsignedInt _styleUniformCount, _styleCount;
    };

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    std::ostringstream out;
    Error redirectError{&out};
    Style{4, 5}.apply(ui, StyleFeature::TextLayer, nullptr);
    Style{3, 4}.apply(ui, StyleFeature::TextLayer, nullptr);
    CORRADE_COMPARE(out.str(),
        "Whee::AbstractStyle::apply(): style has 4 uniforms and 5 styles but the text layer has 3 and 5\n"
        "Whee::AbstractStyle::apply(): style has 3 uniforms and 4 styles but the text layer has 3 and 5\n");
}

void AbstractStyleTest::applyTextLayerNoGlyphCache() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(UnsignedInt styleUniformCount, UnsignedInt styleCount): TextLayer::Shared{styleUniformCount, styleCount} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    } shared{1, 1};

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    };

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    ui.setTextLayerInstance(Containers::pointer<Layer>(ui.createLayer(), shared));

    struct: AbstractStyle {
        StyleFeatures doFeatures() const override { return StyleFeature::TextLayer; }
        UnsignedInt doTextLayerStyleCount() const override { return 1; }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } style;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    std::ostringstream out;
    Error redirectError{&out};
    style.apply(ui, StyleFeature::TextLayer, nullptr);
    CORRADE_COMPARE(out.str(), "Whee::AbstractStyle::apply(): glyph cache not present in the text layer\n");
}

void AbstractStyleTest::applyTextLayerDifferentGlyphCache() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::RG16F, {3, 5, 2}, {4, 1}};

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(UnsignedInt styleUniformCount, UnsignedInt styleCount): TextLayer::Shared{styleUniformCount, styleCount} {}

        using TextLayer::Shared::setGlyphCache;

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    } shared{1, 1};
    shared.setGlyphCache(cache);

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    };

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    ui.setTextLayerInstance(Containers::pointer<Layer>(ui.createLayer(), shared));

    struct Style: AbstractStyle {
        Style(PixelFormat format, const Vector3i& size, const Vector2i& padding): _format{format}, _size{size}, _padding{padding} {}

        StyleFeatures doFeatures() const override { return StyleFeature::TextLayer; }
        UnsignedInt doTextLayerStyleCount() const override { return 1; }
        PixelFormat doTextLayerGlyphCacheFormat() const override { return _format; }
        Vector3i doTextLayerGlyphCacheSize() const override { return _size; }
        Vector2i doTextLayerGlyphCachePadding() const override { return _padding; }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }

        PixelFormat _format;
        Vector3i _size;
        Vector2i _padding;
    };

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    std::ostringstream out;
    Error redirectError{&out};
    Style{PixelFormat::R8Unorm, {3, 5, 2}, {4, 1}}
        .apply(ui, StyleFeature::TextLayer, nullptr);
    Style{PixelFormat::RG16F, {4, 5, 2}, {4, 1}}
        .apply(ui, StyleFeature::TextLayer, nullptr);
    Style{PixelFormat::RG16F, {3, 4, 2}, {4, 1}}
        .apply(ui, StyleFeature::TextLayer, nullptr);
    Style{PixelFormat::RG16F, {3, 5, 4}, {4, 1}}
        .apply(ui, StyleFeature::TextLayer, nullptr);
    Style{PixelFormat::RG16F, {3, 5, 2}, {3, 1}}
        .apply(ui, StyleFeature::TextLayer, nullptr);
    Style{PixelFormat::RG16F, {3, 5, 2}, {4, 2}}
        .apply(ui, StyleFeature::TextLayer, nullptr);
    CORRADE_COMPARE_AS(out.str(),
        "Whee::AbstractStyle::apply(): style has a PixelFormat::R8Unorm glyph cache of size {3, 5, 2} and padding {4, 1} but the text layer has PixelFormat::RG16F, {3, 5, 2} and {4, 1}\n"
       "Whee::AbstractStyle::apply(): style has a PixelFormat::RG16F glyph cache of size {4, 5, 2} and padding {4, 1} but the text layer has PixelFormat::RG16F, {3, 5, 2} and {4, 1}\n"
       "Whee::AbstractStyle::apply(): style has a PixelFormat::RG16F glyph cache of size {3, 4, 2} and padding {4, 1} but the text layer has PixelFormat::RG16F, {3, 5, 2} and {4, 1}\n"
       "Whee::AbstractStyle::apply(): style has a PixelFormat::RG16F glyph cache of size {3, 5, 4} and padding {4, 1} but the text layer has PixelFormat::RG16F, {3, 5, 2} and {4, 1}\n"
       "Whee::AbstractStyle::apply(): style has a PixelFormat::RG16F glyph cache of size {3, 5, 2} and padding {3, 1} but the text layer has PixelFormat::RG16F, {3, 5, 2} and {4, 1}\n"
       "Whee::AbstractStyle::apply(): style has a PixelFormat::RG16F glyph cache of size {3, 5, 2} and padding {4, 2} but the text layer has PixelFormat::RG16F, {3, 5, 2} and {4, 1}\n",
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
        explicit LayerShared(UnsignedInt styleUniformCount, UnsignedInt styleCount): TextLayer::Shared{styleUniformCount, styleCount} {}

        using TextLayer::Shared::setGlyphCache;

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    } shared{1, 1};
    shared.setGlyphCache(cache);

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    };

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    ui.setTextLayerInstance(Containers::pointer<Layer>(ui.createLayer(), shared));

    struct: AbstractStyle {
        StyleFeatures doFeatures() const override { return StyleFeature::TextLayer; }
        Vector3i doTextLayerGlyphCacheSize() const override { return {16, 16, 1}; }
        UnsignedInt doTextLayerStyleCount() const override { return 1; }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } style;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    std::ostringstream out;
    Error redirectError{&out};
    style.apply(ui, StyleFeature::TextLayer, nullptr);
    CORRADE_COMPARE(out.str(), "Whee::AbstractStyle::apply(): fontManager has to be specified for applying a text layer style\n");
}

void AbstractStyleTest::applyEventLayerNotPresent() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct LayerSharedBase: BaseLayer::Shared {
        explicit LayerSharedBase(UnsignedInt styleUniformCount, UnsignedInt styleCount): BaseLayer::Shared{styleUniformCount, styleCount} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } sharedBase{3, 5};

    struct LayerBase: BaseLayer {
        explicit LayerBase(LayerHandle handle, LayerSharedBase& shared): BaseLayer{handle, shared} {}
    };

    struct LayerSharedText: TextLayer::Shared {
        explicit LayerSharedText(UnsignedInt styleUniformCount, UnsignedInt styleCount): TextLayer::Shared{styleUniformCount, styleCount} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    } sharedText{1, 3};

    struct LayerText: TextLayer {
        explicit LayerText(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    };

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    ui.setBaseLayerInstance(Containers::pointer<LayerBase>(ui.createLayer(), sharedBase));
    ui.setTextLayerInstance(Containers::pointer<LayerText>(ui.createLayer(), sharedText));

    struct: AbstractStyle {
        StyleFeatures doFeatures() const override { return StyleFeature::EventLayer; }
        bool doApply(UserInterface&, StyleFeatures, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } style;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    std::ostringstream out;
    Error redirectError{&out};
    style.apply(ui, StyleFeature::EventLayer, nullptr);
    CORRADE_COMPARE(out.str(), "Whee::AbstractStyle::apply(): event layer not present in the user interface\n");
}

}}}}

CORRADE_TEST_MAIN(Magnum::Whee::Test::AbstractStyleTest)
