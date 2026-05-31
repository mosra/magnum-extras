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

#include "AbstractTheme.h"
#include "AbstractTheme.hpp"

#include <Corrade/Containers/EnumSet.hpp>
#include <Corrade/Utility/Assert.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/Text/AbstractGlyphCache.h>

#include "Magnum/Ui/BaseLayer.h"
#include "Magnum/Ui/LayoutLayer.h"
#include "Magnum/Ui/TextLayer.h"
#include "Magnum/Ui/UserInterface.h"

namespace Magnum { namespace Ui {

Debug& operator<<(Debug& debug, const ThemeFeature value) {
    debug << "Ui::ThemeFeature" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case ThemeFeature::value: return debug << "::" #value;
        _c(DataLayer)
        _c(BackgroundLayer)
        _c(BackgroundLayerAnimations)
        _c(BaseLayer)
        _c(BaseLayerAnimations)
        _c(TextLayer)
        _c(TextLayerAnimations)
        _c(EventLayer)
        _c(LayoutLayer)
        _c(SnapLayouter)
        _c(GenericLayouter)
        _c(NodeAnimations)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "(" << Debug::nospace << Debug::hex << UnsignedShort(value) << Debug::nospace << ")";
}

Debug& operator<<(Debug& debug, const ThemeFeatures value) {
    return Containers::enumSetDebugOutput(debug, value, "Ui::ThemeFeatures{}", {
        ThemeFeature::DataLayer,
        ThemeFeature::BackgroundLayer,
        ThemeFeature::BackgroundLayerAnimations,
        ThemeFeature::BaseLayer,
        ThemeFeature::BaseLayerAnimations,
        ThemeFeature::TextLayer,
        ThemeFeature::TextLayerAnimations,
        ThemeFeature::EventLayer,
        ThemeFeature::LayoutLayer,
        ThemeFeature::SnapLayouter,
        ThemeFeature::GenericLayouter,
        ThemeFeature::NodeAnimations,
    });
}

AbstractTheme::AbstractTheme() = default;

AbstractTheme::~AbstractTheme() = default;

ThemeFeatures AbstractTheme::features() const {
    const ThemeFeatures out = doFeatures();
    CORRADE_ASSERT(out,
        "Ui::AbstractTheme::features(): implementation returned an empty set", {});
    return out;
}

UnsignedInt AbstractTheme::backgroundLayerStyleUniformCount() const {
    CORRADE_ASSERT(features() >= ThemeFeature::BackgroundLayer,
        "Ui::AbstractTheme::backgroundLayerStyleUniformCount(): feature not supported", {});
    return doBackgroundLayerStyleUniformCount();
}

UnsignedInt AbstractTheme::doBackgroundLayerStyleUniformCount() const {
    return doBackgroundLayerStyleCount();
}

UnsignedInt AbstractTheme::backgroundLayerStyleCount() const {
    CORRADE_ASSERT(features() >= ThemeFeature::BackgroundLayer,
        "Ui::AbstractTheme::backgroundLayerStyleCount(): feature not supported", {});
    return doBackgroundLayerStyleCount();
}

UnsignedInt AbstractTheme::doBackgroundLayerStyleCount() const {
    CORRADE_ASSERT_UNREACHABLE("Ui::AbstractTheme::backgroundLayerStyleCount(): feature advertised but not implemented", {});
}

UnsignedInt AbstractTheme::backgroundLayerDynamicStyleCount() const {
    CORRADE_ASSERT(features() >= ThemeFeature::BackgroundLayer,
        "Ui::AbstractTheme::backgroundLayerDynamicStyleCount(): feature not supported", {});
    CORRADE_ASSERT(!(features() >= ThemeFeature::BackgroundLayerAnimations) || doBackgroundLayerDynamicStyleCount(),
        "Ui::AbstractTheme::backgroundLayerDynamicStyleCount(): implementation advertises" << ThemeFeature::BackgroundLayerAnimations << "but zero dynamic styles", {});
    return Math::max(doBackgroundLayerDynamicStyleCount(), _backgroundLayerDynamicStyleCount);
}

UnsignedInt AbstractTheme::doBackgroundLayerDynamicStyleCount() const {
    return 0;
}

AbstractTheme& AbstractTheme::setBackgroundLayerDynamicStyleCount(const UnsignedInt count) {
    CORRADE_ASSERT(features() >= ThemeFeature::BackgroundLayer,
        "Ui::AbstractTheme::setBackgroundLayerDynamicStyleCount(): feature not supported", *this);
    _backgroundLayerDynamicStyleCount = count;
    return *this;
}

BaseLayerSharedFlags AbstractTheme::backgroundLayerFlags() const {
    CORRADE_ASSERT(features() >= ThemeFeature::BackgroundLayer,
        "Ui::AbstractTheme::backgroundLayerFlags(): feature not supported", {});
    const BaseLayerSharedFlags flags = doBackgroundLayerFlags();
    CORRADE_ASSERT(flags <= (BaseLayerSharedFlag::NoOutline|BaseLayerSharedFlag::NoRoundedCorners|BaseLayerSharedFlag::BackgroundBlur),
        "Ui::AbstractTheme::backgroundLayerFlags(): implementation returned disallowed" << (flags & ~(BaseLayerSharedFlag::NoOutline|BaseLayerSharedFlag::NoRoundedCorners|BaseLayerSharedFlag::BackgroundBlur)), {});
    return (flags|_backgroundLayerFlagsAdd)&~_backgroundLayerFlagsClear;
}

BaseLayerSharedFlags AbstractTheme::doBackgroundLayerFlags() const {
    return {};
}

AbstractTheme& AbstractTheme::setBackgroundLayerFlags(const BaseLayerSharedFlags add, const BaseLayerSharedFlags clear) {
    CORRADE_ASSERT(features() >= ThemeFeature::BackgroundLayer,
        "Ui::AbstractTheme::setBackgroundLayerFlags(): feature not supported", *this);
    CORRADE_ASSERT(add <= BaseLayerSharedFlag::SubdividedQuads,
        "Ui::AbstractTheme::setBackgroundLayerFlags():" << (add & ~BaseLayerSharedFlag::SubdividedQuads) << "isn't allowed to be added", *this);
    CORRADE_ASSERT(clear <= (BaseLayerSharedFlag::NoOutline|BaseLayerSharedFlag::NoRoundedCorners|BaseLayerSharedFlag::BackgroundBlur),
        "Ui::AbstractTheme::setBackgroundLayerFlags():" << (clear & ~(BaseLayerSharedFlag::NoOutline|BaseLayerSharedFlag::NoRoundedCorners|BaseLayerSharedFlag::BackgroundBlur)) << "isn't allowed to be cleared", *this);
    _backgroundLayerFlagsAdd = add;
    _backgroundLayerFlagsClear = clear;
    return *this;
}

UnsignedInt AbstractTheme::backgroundLayerBlurRadius() const {
    CORRADE_ASSERT(features() >= ThemeFeature::BackgroundLayer,
        "Ui::AbstractTheme::backgroundLayerBlurRadius(): feature not supported", {});
    return doBackgroundLayerBlurRadius();
}

UnsignedInt AbstractTheme::doBackgroundLayerBlurRadius() const {
    return 4;
}

Float AbstractTheme::backgroundLayerBlurCutoff() const {
    CORRADE_ASSERT(features() >= ThemeFeature::BackgroundLayer,
        "Ui::AbstractTheme::backgroundLayerBlurCutoff(): feature not supported", {});
    return doBackgroundLayerBlurCutoff();
}

Float AbstractTheme::doBackgroundLayerBlurCutoff() const {
    return 0.5f/255.0f;
}

UnsignedInt AbstractTheme::baseLayerStyleUniformCount() const {
    CORRADE_ASSERT(features() >= ThemeFeature::BaseLayer,
        "Ui::AbstractTheme::baseLayerStyleUniformCount(): feature not supported", {});
    return doBaseLayerStyleUniformCount();
}

UnsignedInt AbstractTheme::doBaseLayerStyleUniformCount() const {
    return doBaseLayerStyleCount();
}

UnsignedInt AbstractTheme::baseLayerStyleCount() const {
    CORRADE_ASSERT(features() >= ThemeFeature::BaseLayer,
        "Ui::AbstractTheme::baseLayerStyleCount(): feature not supported", {});
    return doBaseLayerStyleCount();
}

UnsignedInt AbstractTheme::doBaseLayerStyleCount() const {
    CORRADE_ASSERT_UNREACHABLE("Ui::AbstractTheme::baseLayerStyleCount(): feature advertised but not implemented", {});
}

UnsignedInt AbstractTheme::baseLayerDynamicStyleCount() const {
    CORRADE_ASSERT(features() >= ThemeFeature::BaseLayer,
        "Ui::AbstractTheme::baseLayerDynamicStyleCount(): feature not supported", {});
    CORRADE_ASSERT(!(features() >= ThemeFeature::BaseLayerAnimations) || doBaseLayerDynamicStyleCount(),
        "Ui::AbstractTheme::baseLayerDynamicStyleCount(): implementation advertises" << ThemeFeature::BaseLayerAnimations << "but zero dynamic styles", {});
    return Math::max(doBaseLayerDynamicStyleCount(), _baseLayerDynamicStyleCount);
}

UnsignedInt AbstractTheme::doBaseLayerDynamicStyleCount() const {
    return 0;
}

AbstractTheme& AbstractTheme::setBaseLayerDynamicStyleCount(const UnsignedInt count) {
    CORRADE_ASSERT(features() >= ThemeFeature::BaseLayer,
        "Ui::AbstractTheme::setBaseLayerDynamicStyleCount(): feature not supported", *this);
    _baseLayerDynamicStyleCount = count;
    return *this;
}

BaseLayerSharedFlags AbstractTheme::baseLayerFlags() const {
    CORRADE_ASSERT(features() >= ThemeFeature::BaseLayer,
        "Ui::AbstractTheme::baseLayerFlags(): feature not supported", {});
    const BaseLayerSharedFlags flags = doBaseLayerFlags();
    CORRADE_ASSERT(flags <= (BaseLayerSharedFlag::NoOutline|BaseLayerSharedFlag::NoRoundedCorners),
        "Ui::AbstractTheme::baseLayerFlags(): implementation returned disallowed" << (flags & ~(BaseLayerSharedFlag::NoOutline|BaseLayerSharedFlag::NoRoundedCorners)), {});
    return (flags|_baseLayerFlagsAdd)&~_baseLayerFlagsClear;
}

BaseLayerSharedFlags AbstractTheme::doBaseLayerFlags() const {
    return {};
}

AbstractTheme& AbstractTheme::setBaseLayerFlags(const BaseLayerSharedFlags add, const BaseLayerSharedFlags clear) {
    CORRADE_ASSERT(features() >= ThemeFeature::BaseLayer,
        "Ui::AbstractTheme::setBaseLayerFlags(): feature not supported", *this);
    CORRADE_ASSERT(add <= BaseLayerSharedFlag::SubdividedQuads,
        "Ui::AbstractTheme::setBaseLayerFlags():" << (add & ~BaseLayerSharedFlag::SubdividedQuads) << "isn't allowed to be added", *this);
    CORRADE_ASSERT(clear <= (BaseLayerSharedFlag::NoOutline|BaseLayerSharedFlag::NoRoundedCorners),
        "Ui::AbstractTheme::setBaseLayerFlags():" << (clear & ~(BaseLayerSharedFlag::NoOutline|BaseLayerSharedFlag::NoRoundedCorners)) << "isn't allowed to be cleared", *this);
    _baseLayerFlagsAdd = add;
    _baseLayerFlagsClear = clear;
    return *this;
}

UnsignedInt AbstractTheme::textLayerStyleUniformCount() const {
    CORRADE_ASSERT(features() >= ThemeFeature::TextLayer,
        "Ui::AbstractTheme::textLayerStyleUniformCount(): feature not supported", {});
    return doTextLayerStyleUniformCount();
}

UnsignedInt AbstractTheme::doTextLayerStyleUniformCount() const {
    return doTextLayerStyleCount();
}

UnsignedInt AbstractTheme::textLayerStyleCount() const {
    CORRADE_ASSERT(features() >= ThemeFeature::TextLayer,
        "Ui::AbstractTheme::textLayerStyleCount(): feature not supported", {});
    return doTextLayerStyleCount();
}

UnsignedInt AbstractTheme::doTextLayerStyleCount() const {
    CORRADE_ASSERT_UNREACHABLE("Ui::AbstractTheme::textLayerStyleCount(): feature advertised but not implemented", {});
}

UnsignedInt AbstractTheme::textLayerEditingStyleUniformCount() const {
    CORRADE_ASSERT(features() >= ThemeFeature::TextLayer,
        "Ui::AbstractTheme::textLayerEditingStyleUniformCount(): feature not supported", {});
    return doTextLayerEditingStyleUniformCount();
}

UnsignedInt AbstractTheme::doTextLayerEditingStyleUniformCount() const {
    return doTextLayerEditingStyleCount();
}

UnsignedInt AbstractTheme::textLayerEditingStyleCount() const {
    CORRADE_ASSERT(features() >= ThemeFeature::TextLayer,
        "Ui::AbstractTheme::textLayerEditingStyleCount(): feature not supported", {});
    return doTextLayerEditingStyleCount();
}

UnsignedInt AbstractTheme::doTextLayerEditingStyleCount() const {
    return 0;
}

UnsignedInt AbstractTheme::textLayerDynamicStyleCount() const {
    CORRADE_ASSERT(features() >= ThemeFeature::TextLayer,
        "Ui::AbstractTheme::textLayerDynamicStyleCount(): feature not supported", {});
    CORRADE_ASSERT(!(features() >= ThemeFeature::TextLayerAnimations) || doTextLayerDynamicStyleCount(),
        "Ui::AbstractTheme::textLayerDynamicStyleCount(): implementation advertises" << ThemeFeature::TextLayerAnimations << "but zero dynamic styles", {});
    return Math::max(doTextLayerDynamicStyleCount(), _textLayerDynamicStyleCount);
}

UnsignedInt AbstractTheme::doTextLayerDynamicStyleCount() const {
    return 0;
}

AbstractTheme& AbstractTheme::setTextLayerDynamicStyleCount(const UnsignedInt count) {
    CORRADE_ASSERT(features() >= ThemeFeature::TextLayer,
        "Ui::AbstractTheme::setTextLayerDynamicStyleCount(): feature not supported", *this);
    _textLayerDynamicStyleCount = count;
    return *this;
}

PixelFormat AbstractTheme::textLayerGlyphCacheFormat() const {
    CORRADE_ASSERT(features() >= ThemeFeature::TextLayer,
        "Ui::AbstractTheme::textLayerGlyphCacheFormat(): feature not supported", {});
    return doTextLayerGlyphCacheFormat();
}

PixelFormat AbstractTheme::doTextLayerGlyphCacheFormat() const {
    return PixelFormat::R8Unorm;
}

Vector3i AbstractTheme::textLayerGlyphCacheSize(ThemeFeatures features) const {
    CORRADE_ASSERT(this->features() >= ThemeFeature::TextLayer,
        "Ui::AbstractTheme::textLayerGlyphCacheSize(): feature not supported", {});
    CORRADE_ASSERT(features >= ThemeFeature::TextLayer,
        "Ui::AbstractTheme::textLayerGlyphCacheSize(): expected a superset of" << ThemeFeature::TextLayer << "but got" << features, {});
    CORRADE_ASSERT(features <= this->features(),
        "Ui::AbstractTheme::textLayerGlyphCacheSize():" << features << "not a subset of supported" << this->features(), {});
    return Math::max(doTextLayerGlyphCacheSize(features), _textLayerGlyphCacheSize);
}

Vector3i AbstractTheme::doTextLayerGlyphCacheSize(ThemeFeatures) const {
    CORRADE_ASSERT_UNREACHABLE("Ui::AbstractTheme::textLayerGlyphCacheSize(): feature advertised but not implemented", {});
}

Vector2i AbstractTheme::textLayerGlyphCachePadding() const {
    CORRADE_ASSERT(features() >= ThemeFeature::TextLayer,
        "Ui::AbstractTheme::textLayerGlyphCachePadding(): feature not supported", {});
    return Math::max(doTextLayerGlyphCachePadding(), _textLayerGlyphCachePadding);
}

Vector2i AbstractTheme::doTextLayerGlyphCachePadding() const {
    return Vector2i{1};
}

AbstractTheme& AbstractTheme::setTextLayerGlyphCacheSize(const Vector3i& size, const Vector2i& padding) {
    CORRADE_ASSERT(features() >= ThemeFeature::TextLayer,
        "Ui::AbstractTheme::setTextLayerGlyphCacheSize(): feature not supported", *this);
    _textLayerGlyphCacheSize = size;
    _textLayerGlyphCachePadding = padding;
    return *this;
}

UnsignedInt AbstractTheme::layoutLayerStyleCount() const {
    CORRADE_ASSERT(features() >= ThemeFeature::LayoutLayer,
        "Ui::AbstractTheme::layoutLayerStyleCount(): feature not supported", {});
    return doLayoutLayerStyleCount();
}

UnsignedInt AbstractTheme::doLayoutLayerStyleCount() const {
    CORRADE_ASSERT_UNREACHABLE("Ui::AbstractTheme::layoutLayerStyleCount(): feature advertised but not implemented", {});
}

bool AbstractTheme::apply(UserInterface& ui, const ThemeFeatures features, PluginManager::Manager<Text::AbstractFont>* const fontManager) const {
    CORRADE_ASSERT(features,
        "Ui::AbstractTheme::apply(): no features specified", {});
    CORRADE_ASSERT(features <= this->features(),
        "Ui::AbstractTheme::apply():" << features << "not a subset of supported" << this->features(), {});
    CORRADE_ASSERT(!ui.framebufferSize().isZero(),
        "Ui::AbstractTheme::apply(): user interface size wasn't set", {});
    #ifndef CORRADE_NO_ASSERT
    if(features >= ThemeFeature::DataLayer) {
        CORRADE_ASSERT(ui.hasDataLayer(),
            "Ui::AbstractTheme::apply(): data layer not present in the user interface", {});
    }
    if(features >= ThemeFeature::BackgroundLayer) {
        CORRADE_ASSERT(ui.hasBackgroundLayer(),
            "Ui::AbstractTheme::apply(): background layer not present in the user interface", {});
        const BaseLayer::Shared& shared = ui.backgroundLayer().shared();
        /** @todo currently deliberately *not* verifying that the blur radius
            matches, as I'm not sure what the right behavior should be, and if
            it should be allowed to be overriden by the user to a different
            value (currently for perf reasons, but with a better implementation
            not even that should be needed) */
        CORRADE_ASSERT(
            shared.styleUniformCount() == backgroundLayerStyleUniformCount() &&
            shared.styleCount() == backgroundLayerStyleCount() &&
            shared.dynamicStyleCount() >= backgroundLayerDynamicStyleCount(),
            "Ui::AbstractTheme::apply(): theme wants" << backgroundLayerStyleUniformCount() << "uniforms," << backgroundLayerStyleCount() << "styles and at least" << backgroundLayerDynamicStyleCount() << "dynamic styles but the background layer has" << shared.styleUniformCount() << Debug::nospace << "," << shared.styleCount() << "and" << shared.dynamicStyleCount(), {});
    }
    if(features >= ThemeFeature::BackgroundLayerAnimations) {
        CORRADE_ASSERT(ui.hasBackgroundLayerStyleAnimator(),
            "Ui::AbstractTheme::apply(): background layer style animator not present in the user interface", {});
    }
    if(features >= ThemeFeature::BaseLayer) {
        CORRADE_ASSERT(ui.hasBaseLayer(),
            "Ui::AbstractTheme::apply(): base layer not present in the user interface", {});
        const BaseLayer::Shared& shared = ui.baseLayer().shared();
        CORRADE_ASSERT(
            shared.styleUniformCount() == baseLayerStyleUniformCount() &&
            shared.styleCount() == baseLayerStyleCount() &&
            shared.dynamicStyleCount() >= baseLayerDynamicStyleCount(),
            "Ui::AbstractTheme::apply(): theme wants" << baseLayerStyleUniformCount() << "uniforms," << baseLayerStyleCount() << "styles and at least" << baseLayerDynamicStyleCount() << "dynamic styles but the base layer has" << shared.styleUniformCount() << Debug::nospace << "," << shared.styleCount() << "and" << shared.dynamicStyleCount(), {});
    }
    if(features >= ThemeFeature::BaseLayerAnimations) {
        CORRADE_ASSERT(ui.hasBaseLayerStyleAnimator(),
            "Ui::AbstractTheme::apply(): base layer style animator not present in the user interface", {});
    }
    if(features >= ThemeFeature::TextLayer) {
        CORRADE_ASSERT(ui.hasTextLayer(),
            "Ui::AbstractTheme::apply(): text layer not present in the user interface", {});
        const TextLayer::Shared& shared = ui.textLayer().shared();
        CORRADE_ASSERT(
            shared.styleUniformCount() == textLayerStyleUniformCount() &&
            shared.styleCount() == textLayerStyleCount() &&
            shared.editingStyleUniformCount() == textLayerEditingStyleUniformCount() &&
            shared.editingStyleCount() == textLayerEditingStyleCount() &&
            shared.dynamicStyleCount() >= textLayerDynamicStyleCount(),
            "Ui::AbstractTheme::apply(): theme wants" << textLayerStyleUniformCount() << "uniforms," << textLayerStyleCount() << "styles," << textLayerEditingStyleUniformCount() << "editing uniforms," << textLayerEditingStyleCount() << "editing styles and at least" << textLayerDynamicStyleCount() << "dynamic styles but the text layer has" << shared.styleUniformCount() << Debug::nospace << "," << shared.styleCount() << Debug::nospace << "," << shared.editingStyleUniformCount() << Debug::nospace << "," << shared.editingStyleCount() << "and" << shared.dynamicStyleCount(), {});

        const Text::AbstractGlyphCache& cache = shared.glyphCache();
        const Vector3i cacheSize = textLayerGlyphCacheSize(features);
        CORRADE_ASSERT(
            cache.format() == textLayerGlyphCacheFormat() &&
            (cache.size() >= cacheSize).all() &&
            (cache.padding() >= textLayerGlyphCachePadding()).all(),
            "Ui::AbstractTheme::apply(): theme wants a" << textLayerGlyphCacheFormat() << "glyph cache of size at least" << Debug::packed << cacheSize << "and padding at least" << Debug::packed << textLayerGlyphCachePadding() << "but the text layer has" << cache.format() << Debug::nospace << "," << Debug::packed << cache.size() << "and" << Debug::packed << cache.padding(), {});

        CORRADE_ASSERT(fontManager,
            "Ui::AbstractTheme::apply(): fontManager has to be specified for applying a text layer style", {});
    }
    if(features >= ThemeFeature::TextLayerAnimations) {
        CORRADE_ASSERT(ui.hasTextLayerStyleAnimator(),
            "Ui::AbstractTheme::apply(): text layer style animator not present in the user interface", {});
    }
    if(features >= ThemeFeature::EventLayer) {
        CORRADE_ASSERT(ui.hasEventLayer(),
            "Ui::AbstractTheme::apply(): event layer not present in the user interface", {});
    }
    if(features >= ThemeFeature::LayoutLayer) {
        CORRADE_ASSERT(ui.hasLayoutLayer(),
            "Ui::AbstractTheme::apply(): layout layer not present in the user interface", {});
        const LayoutLayer& layer = ui.layoutLayer();
        CORRADE_ASSERT(
            layer.styleCount() == layoutLayerStyleCount(),
            "Ui::AbstractTheme::apply(): theme wants" << layoutLayerStyleCount() << "styles but the layout layer has" << layer.styleCount(), {});
    }
    if(features >= ThemeFeature::SnapLayouter) {
        CORRADE_ASSERT(ui.hasSnapLayouter(),
            "Ui::AbstractTheme::apply(): snap layouter not present in the user interface", {});
    }
    if(features >= ThemeFeature::GenericLayouter) {
        CORRADE_ASSERT(ui.hasGenericLayouter(),
            "Ui::AbstractTheme::apply(): generic layouter not present in the user interface", {});
    }
    if(features >= ThemeFeature::NodeAnimations) {
        CORRADE_ASSERT(ui.hasNodeAnimator(),
            "Ui::AbstractTheme::apply(): node animator not present in the user interface", {});
    }
    #endif

    return doApply(ui, features, fontManager);
}

namespace Implementation {

namespace {

template<class Style> struct Transition {
    /* All variants using the same style + disabled */
    /*implicit*/ Transition(Style style, Style disabled): inactiveOut{style}, inactiveOver{style}, focusedOut{style}, focusedOver{style}, pressedOut{style}, pressedOver{style}, disabled{disabled} {}
    /* The full set */
    /*implicit*/ Transition(Style inactiveOut, Style inactiveOver, Style focusedOut, Style focusedOver, Style pressedOut, Style pressedOver, Style disabled): inactiveOut{inactiveOut}, inactiveOver{inactiveOver}, focusedOut{focusedOut}, focusedOver{focusedOver}, pressedOut{pressedOut}, pressedOver{pressedOver}, disabled{disabled} {}
    /* No focused style */
    /*implicit*/ Transition(Style inactiveOut, Style inactiveOver, Style pressedOut, Style pressedOver, Style disabled): inactiveOut{inactiveOut}, inactiveOver{inactiveOver}, focusedOut{pressedOut}, focusedOver{pressedOver}, pressedOut{pressedOut}, pressedOver{pressedOver}, disabled{disabled} {}

    Style inactiveOut;
    Style inactiveOver;
    Style focusedOut;
    Style focusedOver;
    Style pressedOut;
    Style pressedOver;
    Style disabled;
};

Transition<BaseStyle> styleTransition(const BaseStyle index) {
    switch(index) {
        #define _c(style)                                                   \
            case BaseStyle::style:                                          \
                return {BaseStyle::style,                                   \
                        BaseStyle::style};
        #define _cDisabled(style)                                           \
            case BaseStyle::style:                                          \
                return {BaseStyle::style,                                   \
                        BaseStyle::style ## Disabled};                      \
            case BaseStyle::style ## Disabled:                              \
                CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        #define _cHoveredPressedDisabled(style)                             \
            case BaseStyle::style:                                          \
            case BaseStyle::style ## Hovered:                               \
            case BaseStyle::style ## Pressed:                               \
                return {BaseStyle::style,                                   \
                        BaseStyle::style ## Hovered,                        \
                        BaseStyle::style ## Pressed,                        \
                        BaseStyle::style ## Pressed,                        \
                        BaseStyle::style ## Disabled};                      \
            case BaseStyle::style ## Disabled:                              \
                CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        #define _cHoveredPressedPressedHoveredDisabled(style)               \
            case BaseStyle::style:                                          \
            case BaseStyle::style ## Hovered:                               \
            case BaseStyle::style ## Pressed:                               \
            case BaseStyle::style ## PressedHovered:                        \
                return {BaseStyle::style,                                   \
                        BaseStyle::style ## Hovered,                        \
                        BaseStyle::style ## Pressed,                        \
                        BaseStyle::style ## PressedHovered,                 \
                        BaseStyle::style ## Disabled};                      \
            case BaseStyle::style ## Disabled:                              \
                CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        #define _cHoveredFocusedDisabled(style)                             \
            case BaseStyle::style:                                          \
            case BaseStyle::style ## Hovered:                               \
            case BaseStyle::style ## Focused:                               \
                return {BaseStyle::style,                                   \
                        BaseStyle::style ## Hovered,                        \
                        BaseStyle::style ## Focused,                        \
                        BaseStyle::style ## Focused,                        \
                        BaseStyle::style ## Focused,                        \
                        BaseStyle::style ## Focused,                        \
                        BaseStyle::style ## Disabled};                      \
            case BaseStyle::style ## Disabled:                              \
                CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        _c(Default)
        _cHoveredPressedPressedHoveredDisabled(ButtonDefault)
        _cHoveredPressedPressedHoveredDisabled(ButtonPrimary)
        _cHoveredPressedPressedHoveredDisabled(ButtonSuccess)
        _cHoveredPressedPressedHoveredDisabled(ButtonWarning)
        _cHoveredPressedPressedHoveredDisabled(ButtonDanger)
        _cHoveredPressedPressedHoveredDisabled(ButtonInfo)
        _cHoveredPressedPressedHoveredDisabled(ButtonDim)
        _cHoveredPressedPressedHoveredDisabled(ButtonFlat)
        _cHoveredFocusedDisabled(InputDefault)
        _cHoveredFocusedDisabled(InputSuccess)
        _cHoveredFocusedDisabled(InputWarning)
        _cHoveredFocusedDisabled(InputDanger)
        _cHoveredFocusedDisabled(InputFlat)
        _cDisabled(PanelBackground)
        _cHoveredPressedDisabled(ScrollbarX)
        _cHoveredPressedDisabled(ScrollbarY)
        _cHoveredPressedDisabled(ScrollbarThumbX)
        _cHoveredPressedDisabled(ScrollbarThumbY)
        #undef _cHoveredFocusedDisabled
        #undef _cHoveredPressedPressedHoveredDisabled
        #undef _cHoveredPressedDisabled
        #undef _cDisabled
        #undef _c
        /* LCOV_EXCL_START */
        case BaseStyle::Count:
            CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        /* LCOV_EXCL_STOP */
    }

    CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
}

}

BaseStyle styleTransitionToInactiveOut(const BaseStyle index) {
    return styleTransition(index).inactiveOut;
}
BaseStyle styleTransitionToInactiveOver(const BaseStyle index) {
    return styleTransition(index).inactiveOver;
}
BaseStyle styleTransitionToFocusedOut(const BaseStyle index) {
    return styleTransition(index).focusedOut;
}
BaseStyle styleTransitionToFocusedOver(const BaseStyle index) {
    return styleTransition(index).focusedOver;
}
BaseStyle styleTransitionToPressedOut(const BaseStyle index) {
    return styleTransition(index).pressedOut;
}
BaseStyle styleTransitionToPressedOver(const BaseStyle index) {
    return styleTransition(index).pressedOver;
}
BaseStyle styleTransitionToDisabled(const BaseStyle index) {
    return styleTransition(index).disabled;
}

namespace {

Transition<TextStyle> styleTransition(const TextStyle index) {
    switch(index) {
        #define _c(style, suffix)                                           \
            case TextStyle::style ## suffix:                                \
                return {TextStyle::style ## suffix,                         \
                        TextStyle::style ## suffix};
        #define _cDisabled(style, suffix)                                   \
            case TextStyle::style ## suffix:                                \
                return {TextStyle::style ## suffix,                         \
                        TextStyle::style ## Disabled ## suffix};            \
            case TextStyle::style ## Disabled ## suffix:                    \
                CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        #define _cPressedDisabled(style, suffix)                            \
            case TextStyle::style ## suffix:                                \
            case TextStyle::style ## Pressed ## suffix:                     \
                return {TextStyle::style ## suffix,                         \
                        TextStyle::style ## suffix,                         \
                        TextStyle::style ## Pressed ## suffix,              \
                        TextStyle::style ## Pressed ## suffix,              \
                        TextStyle::style ## Disabled ## suffix};            \
            case TextStyle::style ## Disabled ## suffix:                    \
                CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        #define _cHoveredPressedPressedHoveredDisabled(style, suffix)       \
            case TextStyle::style ## suffix:                                \
            case TextStyle::style ## Hovered ## suffix:                     \
            case TextStyle::style ## Pressed ## suffix:                     \
            case TextStyle::style ## PressedHovered ## suffix:              \
                return {TextStyle::style ## suffix,                         \
                        TextStyle::style ## Hovered ## suffix,              \
                        TextStyle::style ## Pressed ## suffix,              \
                        TextStyle::style ## PressedHovered ## suffix,       \
                        TextStyle::style ## Disabled ## suffix};            \
            case TextStyle::style ## Disabled ## suffix:                    \
                CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        #define _cHoveredFocusedBlinkPressedDisabled(style, suffix)         \
            case TextStyle::style ## suffix:                                \
            case TextStyle::style ## Hovered ## suffix:                     \
            case TextStyle::style ## Focused ## suffix:                     \
            case TextStyle::style ## Pressed ## suffix:                     \
                return {TextStyle::style ## suffix,                         \
                        TextStyle::style ## Hovered ## suffix,              \
                        TextStyle::style ## Focused ## suffix,              \
                        TextStyle::style ## Focused ## suffix,              \
                        TextStyle::style ## Pressed ## suffix,              \
                        TextStyle::style ## Pressed ## suffix,              \
                        TextStyle::style ## Disabled ## suffix};            \
            case TextStyle::style ## FocusedBlink ## suffix:                \
            case TextStyle::style ## Disabled ## suffix:                    \
                CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        _c(Default,)
        _cPressedDisabled(Button,IconOnly)
        _cPressedDisabled(Button,TextOnly)
        _cPressedDisabled(Button,Icon)
        _cPressedDisabled(Button,Text)
        _cHoveredPressedPressedHoveredDisabled(ButtonFlat,IconOnly)
        _cHoveredPressedPressedHoveredDisabled(ButtonFlat,TextOnly)
        _cHoveredPressedPressedHoveredDisabled(ButtonFlat,Icon)
        _cHoveredPressedPressedHoveredDisabled(ButtonFlat,Text)
        _cDisabled(LabelDefault,Icon)
        _cDisabled(LabelDefault,Text)
        _cDisabled(LabelPrimary,Icon)
        _cDisabled(LabelPrimary,Text)
        _cDisabled(LabelSuccess,Icon)
        _cDisabled(LabelSuccess,Text)
        _cDisabled(LabelWarning,Icon)
        _cDisabled(LabelWarning,Text)
        _cDisabled(LabelDanger,Icon)
        _cDisabled(LabelDanger,Text)
        _cDisabled(LabelInfo,Icon)
        _cDisabled(LabelInfo,Text)
        _cDisabled(LabelDim,Icon)
        _cDisabled(LabelDim,Text)
        _cDisabled(LabelTitle,Icon)
        _cDisabled(LabelTitle,Text)
        _cHoveredFocusedBlinkPressedDisabled(InputDefault,)
        _cHoveredFocusedBlinkPressedDisabled(InputDefaultPassword,)
        _cHoveredFocusedBlinkPressedDisabled(InputSuccess,)
        _cHoveredFocusedBlinkPressedDisabled(InputSuccessPassword,)
        _cHoveredFocusedBlinkPressedDisabled(InputWarning,)
        _cHoveredFocusedBlinkPressedDisabled(InputWarningPassword,)
        _cHoveredFocusedBlinkPressedDisabled(InputDanger,)
        _cHoveredFocusedBlinkPressedDisabled(InputDangerPassword,)
        _cHoveredFocusedBlinkPressedDisabled(InputFlat,)
        _cHoveredFocusedBlinkPressedDisabled(InputFlatPassword,)
        #undef _cHoveredFocusedBlinkPressedDisabled
        #undef _cHoveredPressedPressedHoveredDisabled
        #undef _cPressedDisabled
        #undef _cDisabled
        #undef _c
        /* LCOV_EXCL_START */
        case TextStyle::Count:
            CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        /* LCOV_EXCL_STOP */
    }

    CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
}

}

TextStyle styleTransitionToInactiveOut(const TextStyle index) {
    return styleTransition(index).inactiveOut;
}
TextStyle styleTransitionToInactiveOver(const TextStyle index) {
    return styleTransition(index).inactiveOver;
}
TextStyle styleTransitionToFocusedOut(const TextStyle index) {
    return styleTransition(index).focusedOut;
}
TextStyle styleTransitionToFocusedOver(const TextStyle index) {
    return styleTransition(index).focusedOver;
}
TextStyle styleTransitionToPressedOut(const TextStyle index) {
    return styleTransition(index).pressedOut;
}
TextStyle styleTransitionToPressedOver(const TextStyle index) {
    return styleTransition(index).pressedOver;
}
TextStyle styleTransitionToDisabled(const TextStyle index) {
    return styleTransition(index).disabled;
}

}

}}
