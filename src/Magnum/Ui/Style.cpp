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

#include "Style.h"
#include "Style.hpp"

#include <Corrade/Containers/EnumSet.hpp>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Utility/Algorithms.h>
#include <Corrade/Utility/Resource.h>
#include <Magnum/ImageView.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/Animation/Easing.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Range.h>
#include <Magnum/Text/AbstractFont.h>
#include <Magnum/Text/AbstractGlyphCache.h>
#include <Magnum/Text/Alignment.h>
#include <Magnum/TextureTools/Atlas.h>
#include <Magnum/Trade/AbstractImporter.h>
#include <Magnum/Trade/ImageData.h>

#include "Magnum/Ui/BaseLayer.h"
#include "Magnum/Ui/BaseLayerAnimator.h"
#include "Magnum/Ui/EventLayer.h"
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/LayoutLayer.h"
#include "Magnum/Ui/SnapLayouter.h"
#include "Magnum/Ui/TextLayer.h"
#include "Magnum/Ui/TextLayerAnimator.h"
#include "Magnum/Ui/UserInterface.h"

#ifdef MAGNUM_UI_BUILD_STATIC
static void importShaderResources() {
    CORRADE_RESOURCE_INITIALIZE(MagnumUi_RESOURCES)
}
#endif

namespace Magnum { namespace Ui {

using namespace Math::Literals;

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
        #undef _cHoveredFocusedDisabled
        #undef _cHoveredPressedPressedHoveredDisabled
        #undef _cDisabled
        #undef _c
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
        _cHoveredFocusedBlinkPressedDisabled(InputDefault,)
        _cHoveredFocusedBlinkPressedDisabled(InputSuccess,)
        _cHoveredFocusedBlinkPressedDisabled(InputWarning,)
        _cHoveredFocusedBlinkPressedDisabled(InputDanger,)
        _cHoveredFocusedBlinkPressedDisabled(InputFlat,)
        #undef _cHoveredFocusedBlinkPressedDisabled
        #undef _cHoveredPressedPressedHoveredDisabled
        #undef _cPressedDisabled
        #undef _cDisabled
        #undef _c
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

namespace {

AnimationHandle styleAnimationOnLeaveBlurRelease(BaseLayerStyleAnimator& animator, const BaseStyle sourceStyle, const BaseStyle targetStyle, const Nanoseconds time, const LayerDataHandle data, AnimatorDataHandle) {
    return animator.create(sourceStyle, targetStyle, Animation::Easing::smootherstep, time, 0.5_sec, data);
}

AnimationHandle styleAnimationOnLeaveBlurRelease(TextLayerStyleAnimator& animator, const TextStyle sourceStyle, const TextStyle targetStyle, const Nanoseconds time, const LayerDataHandle data, AnimatorDataHandle) {
    return animator.create(sourceStyle, targetStyle, Animation::Easing::smootherstep, time, 0.5_sec, data);
}

/** @todo split this function up once some of these are not belonging to
    Feature::EssentialAnimations */
template<Float(*cursorBlinkEasing)(Float)> AnimationHandle styleAnimationPersistent(TextLayerStyleAnimator& animator, const TextStyle style, const Nanoseconds time, const LayerDataHandle data, const AnimatorDataHandle currentAnimation) {
    if(style == TextStyle::InputDefaultFocused ||
       style == TextStyle::InputSuccessFocused ||
       style == TextStyle::InputWarningFocused ||
       style == TextStyle::InputDangerFocused ||
       style == TextStyle::InputFlatFocused)
    {
        /* It's just +1 but let's not have it too cryptic here. The compiler
           hopefully optimizes this? */
        TextStyle sourceStyle;
        switch(style) {
            case TextStyle::InputDefaultFocused:
                sourceStyle = TextStyle::InputDefaultFocusedBlink;
                break;
            case TextStyle::InputSuccessFocused:
                sourceStyle = TextStyle::InputSuccessFocusedBlink;
                break;
            case TextStyle::InputWarningFocused:
                sourceStyle = TextStyle::InputWarningFocusedBlink;
                break;
            case TextStyle::InputDangerFocused:
                sourceStyle = TextStyle::InputDangerFocusedBlink;
                break;
            case TextStyle::InputFlatFocused:
                sourceStyle = TextStyle::InputFlatFocusedBlink;
                break;
            /* LCOV_EXCL_START */
            default:
                CORRADE_INTERNAL_ASSERT_UNREACHABLE();
            /* LCOV_EXCL_STOP */
        }

        if(currentAnimation != AnimatorDataHandle::Null)
            animator.remove(currentAnimation);
        /* Start one iteration before so it's initially at the Focused style
           already, with cursor shown. Otherwise there's a very quick blink
           right after releasing the pointer, which doesn't look nice. */
        return animator.create(sourceStyle, style, cursorBlinkEasing, time - 0.55_sec, 0.55_sec, data, 0, AnimationFlag::ReverseEveryOther);
    }

    return {};
}

}

}

Debug& operator<<(Debug& debug, const Icon value) {
    debug << "Ui::Icon" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case Icon::value: return debug << "::" #value;
        _c(None)
        _c(Yes)
        _c(No)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "(" << Debug::nospace << Debug::hex << UnsignedInt(value) << Debug::nospace << ")";
}

Debug& operator<<(Debug& debug, const McssDarkStyle::Feature value) {
    debug << "Ui::McssDarkStyle::Feature" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case McssDarkStyle::Feature::value: return debug << "::" #value;
        _c(EssentialAnimations)
        _c(Animations)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "(" << Debug::nospace << Debug::hex << UnsignedByte(value) << Debug::nospace << ")";
}

Debug& operator<<(Debug& debug, const McssDarkStyle::Features value) {
    return Containers::enumSetDebugOutput(debug, value, "Ui::McssDarkStyle::Features{}", {
        McssDarkStyle::Feature::Animations,
        /* Implied by Animations, has to be after */
        McssDarkStyle::Feature::EssentialAnimations,
    });
}

using namespace Containers::Literals;
using namespace Math::Literals;
using Implementation::BaseStyle;
using Implementation::TextStyle;
using Implementation::TextStyleUniform;

namespace {

/* 1 (true, screen)-pixel radius independently of UI scale */
constexpr BaseLayerCommonStyleUniform BaseCommonStyleUniformMcssDark{1.0f};

#ifndef CORRADE_MSVC2015_COMPATIBILITY
constexpr /* Trust me, you don't want to be on this compiler */
#endif
const BaseLayerStyleUniform BaseStyleUniformsMcssDark[]{
    #define _c(style, ...) {__VA_ARGS__},
    #include "Magnum/Ui/Implementation/baseStyleUniformsMcssDark.h"
    #undef _c
};
static_assert(Implementation::BaseStyleUniformCount == Containers::arraySize(BaseStyleUniformsMcssDark),
    "outdated BaseStyleUniformCount value");
static_assert(Implementation::BaseStyleCount == Containers::arraySize(BaseStyleUniformsMcssDark),
    "outdated BaseStyleCount value");

constexpr TextLayerCommonStyleUniform TextCommonStyleUniformMcssDark{};

#ifndef CORRADE_MSVC2015_COMPATIBILITY
constexpr /* Trust me, you don't want to be on this compiler */
#endif
const TextLayerStyleUniform TextStyleUniformsMcssDark[]{
    #define _c(style, ...) {__VA_ARGS__},
    #include "Magnum/Ui/Implementation/textStyleUniformsMcssDark.h"
    #undef _c
};
static_assert(Implementation::TextStyleUniformCount == Containers::arraySize(TextStyleUniformsMcssDark),
    "outdated TextStyleUniformCount value");

constexpr struct {
    UnsignedInt uniform;
    Text::Alignment alignment;
    Int cursorStyle, selectionStyle;
    Vector4 padding;
} TextStylesMcssDark[]{
    #define _c(style, suffix, font, alignment, ...) {UnsignedInt(TextStyleUniform::style), Text::Alignment::alignment, -1, -1, __VA_ARGS__},
    #define _s(style, suffix, selectionStyle, font, alignment, ...) {UnsignedInt(TextStyleUniform::style), Text::Alignment::alignment, -1, Int(Implementation::TextEditingStyle::selectionStyle), __VA_ARGS__},
    #define _e(style, suffix, cursorStyle, selectionStyle, font, alignment, ...) {UnsignedInt(TextStyleUniform::style), Text::Alignment::alignment, Int(Implementation::TextEditingStyle::cursorStyle), Int(Implementation::TextEditingStyle::selectionStyle), __VA_ARGS__},
    #include "Magnum/Ui/Implementation/textStyleMcssDark.h"
    #undef _e
    #undef _s
    #undef _c
};
static_assert(Implementation::TextStyleCount == Containers::arraySize(TextStylesMcssDark),
    "outdated TextStyleCount value");

/* 1 (true, screen)-pixel radius independently of UI scale */
constexpr TextLayerCommonEditingStyleUniform TextCommonEditingStyleUniformMcssDark{1.0f};

#ifndef CORRADE_MSVC2015_COMPATIBILITY
constexpr /* Trust me, you don't want to be on this compiler */
#endif
const TextLayerEditingStyleUniform TextEditingStyleUniformsMcssDark[]{
    #define _c(style, padding0, padding1, padding2, padding3, ...) {__VA_ARGS__},
    #define _s(style, textUniform, padding0, padding1, padding2, padding3, ...) {__VA_ARGS__},
    #include "Magnum/Ui/Implementation/textEditingStyleMcssDark.h"
    #undef _c
    #undef _s
};
static_assert(Implementation::TextEditingStyleUniformCount == Containers::arraySize(TextEditingStyleUniformsMcssDark),
    "outdated TextEditingStyleUniformCount value");

constexpr struct {
    Int textUniform;
    Vector4 padding;
} TextEditingStylesMcssDark[]{
    #define _c(style, padding0, padding1, padding2, padding3, ...) {-1, padding0, padding1, padding2, padding3},
    #define _s(style, textUniform, padding0, padding1, padding2, padding3, ...) {Int(TextStyleUniform::textUniform), padding0, padding1, padding2, padding3},
    #include "Magnum/Ui/Implementation/textEditingStyleMcssDark.h"
    #undef _c
    #undef _s
};
static_assert(Implementation::TextEditingStyleCount == Containers::arraySize(TextEditingStylesMcssDark),
    "outdated TextEditingStyleCount value");

/* MSVC 2015, 2017 and 2019 cannot handle this in the form of
    constexpr struct LayoutStyle {
        constexpr LayoutStyle(...);
    } LayoutStylesMcssDark[]{
        ...
    };
   complaining that the LayoutStyle constructor isn't constexpr. Splitting the
   struct and array definition makes it work. MSVC 2022 works. */
/** @todo clean this up once MSVC 2017 and 2019 is gone */
struct LayoutStyle {
    constexpr /*implicit*/ LayoutStyle(const Vector2& minSize, const Vector4& padding, const Vector4& margin): minSize{minSize}, padding{padding}, margin{margin} {}
    constexpr /*implicit*/ LayoutStyle(const Vector2& minSize, const Vector2& padding, const Vector2& margin): minSize{minSize}, padding{padding.x(), padding.y(), padding.x(), padding.y()}, margin{margin.x(), margin.y(), margin.x(), margin.y()} {}

    Vector2 minSize;
    Vector4 padding;
    Vector4 margin;
};

constexpr LayoutStyle LayoutStylesMcssDark[]{
    #define _c(style, ...) {__VA_ARGS__},
    #include "Magnum/Ui/Implementation/layoutStyleMcssDark.h"
    #undef _c
};
static_assert(Implementation::LayoutStyleCount == Containers::arraySize(LayoutStylesMcssDark),
    "outdated LayoutStyleCount value");

}

McssDarkStyle::McssDarkStyle(const Features features): _features{features} {}

StyleFeatures McssDarkStyle::doFeatures() const {
    return StyleFeature::BaseLayer|
           StyleFeature::TextLayer|
           StyleFeature::TextLayerImages|
           StyleFeature::EventLayer|
           StyleFeature::LayoutLayer|
           StyleFeature::SnapLayouter|
           /* Essential animations are currently just (text layer) cursor
              blinking */
           (_features >= Feature::Animations ? StyleFeature::BaseLayerAnimations|StyleFeature::TextLayerAnimations : StyleFeatures{})|
           (_features >= Feature::EssentialAnimations ? StyleFeature::TextLayerAnimations : StyleFeatures{});
}

UnsignedInt McssDarkStyle::doBaseLayerStyleUniformCount() const {
    return Implementation::BaseStyleUniformCount;
}

UnsignedInt McssDarkStyle::doBaseLayerStyleCount() const {
    return Implementation::BaseStyleCount;
}

UnsignedInt McssDarkStyle::doBaseLayerDynamicStyleCount() const {
    return _features >= Feature::Animations ? 10 : 0;
}

UnsignedInt McssDarkStyle::doTextLayerStyleUniformCount() const {
    return Implementation::TextStyleUniformCount;
}

UnsignedInt McssDarkStyle::doTextLayerStyleCount() const {
    return Implementation::TextStyleCount;
}

UnsignedInt McssDarkStyle::doTextLayerDynamicStyleCount() const {
    /* For essential animations, assuming there's just one blinking cursor at a
       time, add one dynamic style, and one more for safety */
    return _features >= Feature::Animations ? 10 :
        _features >= Feature::EssentialAnimations ? 2 : 0;
}

UnsignedInt McssDarkStyle::doTextLayerEditingStyleUniformCount() const {
    return Implementation::TextEditingStyleUniformCount;
}

UnsignedInt McssDarkStyle::doTextLayerEditingStyleCount() const {
    return Implementation::TextEditingStyleCount;
}

Vector3i McssDarkStyle::doTextLayerGlyphCacheSize(StyleFeatures) const {
    /* 256x256 is enough only for DPI scale of 1, adding some extra space */
    /** @todo Make this dependent on DPI scale */
    return {512, 512, 1};
}

UnsignedInt McssDarkStyle::doLayoutLayerStyleCount() const {
    return Implementation::LayoutStyleCount;
}

bool McssDarkStyle::doApply(UserInterface& ui, const StyleFeatures features, PluginManager::Manager<Trade::AbstractImporter>* importerManager, PluginManager::Manager<Text::AbstractFont>* fontManager) const {
    /* Base layer style */
    if(features >= StyleFeature::BaseLayer) {
        ui.baseLayer().shared()
            .setStyle(
                BaseCommonStyleUniformMcssDark,
                BaseStyleUniformsMcssDark,
                {})
            .setStyleTransition<BaseStyle,
                Implementation::styleTransitionToInactiveOut,
                Implementation::styleTransitionToInactiveOver,
                Implementation::styleTransitionToFocusedOut,
                Implementation::styleTransitionToFocusedOver,
                Implementation::styleTransitionToPressedOut,
                Implementation::styleTransitionToPressedOver,
                Implementation::styleTransitionToDisabled>();
    }

    /* Animations for the base layer. Advertised only if they were enabled
       during construction. */
    if(features >= StyleFeature::BaseLayerAnimations) {
        CORRADE_INTERNAL_ASSERT(_features >= Feature::Animations);
        ui.baseLayer().shared().setStyleAnimation<BaseStyle,
            nullptr,
            Implementation::styleAnimationOnLeaveBlurRelease,
            nullptr>();
    }

    /* Icon font. Add also if just the text layer style is applied (where it
       gets assigned to icon styles, but without any icons actually loaded).

       MSVC says iconFont might be used uninitialized without the {}. It won't,
       the thing is just stupid. */
    Ui::FontHandle iconFont{};
    if(features & (StyleFeature::TextLayer|StyleFeature::TextLayerImages)) {
        #ifdef MAGNUM_UI_BUILD_STATIC
        importShaderResources();
        #endif

        TextLayer::Shared& shared = ui.textLayer().shared();
        Text::AbstractGlyphCache& glyphCache = shared.glyphCache();
        /* The Icon enum reserves 0 for an invalid glyph, so add 1 */
        const UnsignedInt iconFontId = glyphCache.addFont(Implementation::IconCount + 1);
        /* The input is 64x64 squares, which are meant to be shown as 24x24
           squares in the UI units */
        /** @todo some DPI-aware machinery here, such as picking one of
            multiple icon images depending on the DPI scaling, or maybe just
            put these into a font */
        iconFont = shared.addInstancelessFont(iconFontId, 24.0f/64.0f);
    }

    /* Text layer fonts and style */
    /** @todo figure out how to apply another style and replace the previous
        now-unused font *somehow*, such as by keeping track of which fonts
        correspond to which StyleFeature, and then pruning the cache also */
    if(features >= StyleFeature::TextLayer) {
        TextLayer::Shared& shared = ui.textLayer().shared();
        Text::AbstractGlyphCache& glyphCache = shared.glyphCache();
        const Utility::Resource rs{"MagnumUi"_s};

        Containers::Pointer<Text::AbstractFont> font = fontManager->loadAndInstantiate("TrueTypeFont");
        if(!font || !font->openData(rs.getRaw("SourceSans3-Regular.otf"_s), 16.0f*2*(Vector2{ui.framebufferSize()}/ui.size()).max())) {
            Error{} << "Ui::McssDarkStyle::apply(): cannot open a font";
            return {};
        }
        /** @todo fail if this fails, once the function doesn't return void */
        /** @todo configurable way to fill the cache, or switch to on-demand
            by default once AbstractFont can fill the cache with glyph IDs */
        font->fillGlyphCache(glyphCache,
            "abcdefghijklmnopqrstuvwxyz"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "0123456789 _.,-+=*:;?!@$&#/\\|`\"'<>()[]{}%…");

        /* Main font */
        const Ui::FontHandle mainFont = shared.addFont(Utility::move(font), 16.0f);

        /* Font handles matching all styles. References either the `mainFont`
           or the `iconFont` defined above. */
        const Ui::FontHandle fontHandles[]{
            #define _c(style, suffix, font, ...) font,
            #define _e(style, suffix, cursorStyle, selectionStyle, font, ...) font,
            #define _s(style, suffix, selectionStyle, font, ...) font,
            #include "Magnum/Ui/Implementation/textStyleMcssDark.h"
            #undef _c
            #undef _e
            #undef _s
        };

        shared
            .setStyle(
                TextCommonStyleUniformMcssDark,
                TextStyleUniformsMcssDark,
                Containers::stridedArrayView(TextStylesMcssDark).slice(&std::remove_all_extents<decltype(TextStylesMcssDark)>::type::uniform),
                fontHandles,
                Containers::stridedArrayView(TextStylesMcssDark).slice(&std::remove_all_extents<decltype(TextStylesMcssDark)>::type::alignment),
                /* No features coming from style used yet */
                {}, {}, {},
                Containers::stridedArrayView(TextStylesMcssDark).slice(&std::remove_all_extents<decltype(TextStylesMcssDark)>::type::cursorStyle),
                Containers::stridedArrayView(TextStylesMcssDark).slice(&std::remove_all_extents<decltype(TextStylesMcssDark)>::type::selectionStyle),
                Containers::stridedArrayView(TextStylesMcssDark).slice(&std::remove_all_extents<decltype(TextStylesMcssDark)>::type::padding))
            .setEditingStyle(
                TextCommonEditingStyleUniformMcssDark,
                TextEditingStyleUniformsMcssDark,
                Containers::stridedArrayView(TextEditingStylesMcssDark).slice(&std::remove_all_extents<decltype(TextEditingStylesMcssDark)>::type::textUniform),
                Containers::stridedArrayView(TextEditingStylesMcssDark).slice(&std::remove_all_extents<decltype(TextEditingStylesMcssDark)>::type::padding))
            .setStyleTransition<TextStyle,
                Implementation::styleTransitionToInactiveOut,
                Implementation::styleTransitionToInactiveOver,
                Implementation::styleTransitionToFocusedOut,
                Implementation::styleTransitionToFocusedOver,
                Implementation::styleTransitionToPressedOut,
                Implementation::styleTransitionToPressedOver,
                Implementation::styleTransitionToDisabled>();
    }

    /* Text layer images */
    if(features >= StyleFeature::TextLayerImages) {
        TextLayer::Shared& shared = ui.textLayer().shared();
        Text::AbstractGlyphCache& glyphCache = shared.glyphCache();
        const Utility::Resource rs{"MagnumUi"_s};

        Containers::Pointer<Trade::AbstractImporter> importer = importerManager->loadAndInstantiate("AnyImageImporter");
        Containers::Optional<Trade::ImageData2D> image;
        if(!importer || !importer->openMemory(rs.getRaw("icons.png")) || !(image = importer->image2D(0))) {
            Error{} << "Ui::McssDarkStyle::apply(): cannot open an icon atlas";
            return {};
        }

        /* The image is originally grayscale 8-bit, expect that it's still
           imported with 8-bit channels. The importer can be globally
           configured to import them with more channels (which is fine, for
           example in testing context, where we might always want to compare to
           a RGBA image even if the on-disk representation has the alpha
           dropped), in which case just the red channel is taken, but it's
           important that it isn't expanded to 16 bits or to floats, for
           example. */
        if(pixelFormatChannelFormat(image->format()) != PixelFormat::R8Unorm) {
            Error{} << "Ui::McssDarkStyle::apply(): expected" << PixelFormat::R8Unorm << "icons but got an image with" << image->format();
            return {};
        }
        const std::size_t channelSize = image->pixelSize()/pixelFormatChannelCount(image->format());

        /* At the moment it's a single row of square icons, with the image
           height denoting the square size, and the order matching the Icon
           enum. Reserve space for all of them in the glyph cache. */
        const Vector2i imageSize{image->size().y()};
        CORRADE_INTERNAL_ASSERT(image->size().x() % image->size().y() == 0);
        Vector3i offsets[Implementation::IconCount];
        if(!glyphCache.atlas().add(Containers::stridedArrayView(&imageSize, 1).broadcasted<0>(Implementation::IconCount), offsets)) {
            Error{} << "Ui::McssDarkStyle::apply(): cannot fit" << Implementation::IconCount << "icons into the glyph cache";
            return {};
        }

        /* The font was added above, query the glyph cache ID of it */
        const UnsignedInt iconFontId = shared.glyphCacheFontId(iconFont);

        /* Copy the image data */
        Containers::StridedArrayView3D<const char> src = image->pixels();
        Containers::StridedArrayView4D<char> dst = glyphCache.image().pixels();
        Range2Di updated;
        for(UnsignedInt i = 0; i != Implementation::IconCount; ++i) {
            Range2Di rectangle = Range2Di::fromSize(offsets[i].xy(),
                                                    imageSize);
            /* The Icon enum reserves 0 for an invalid glyph, so add 1 */
            glyphCache.addGlyph(iconFontId, i + 1, {}, rectangle);

            /* Copy assuming all input images have the same pixel format */
            const Containers::Size3D size{
                std::size_t(imageSize.y()),
                std::size_t(imageSize.x()),
                channelSize};
            Utility::copy(
                src.sliceSize({0, std::size_t(i*imageSize.x()), 0}, size),
                dst[offsets[i].z()].sliceSize({std::size_t(offsets[i].y()),
                                                std::size_t(offsets[i].x()),
                                                0}, size));

            /* Maintain a range that was updated in the glyph cache */
            updated = Math::join(updated, rectangle);
        }

        /* Reflect the image data update to the actual GPU-side texture */
        glyphCache.flushImage(updated);
    }

    /* Animations for the text layer. Advertised only if they were enabled
       during construction. */
    if(features >= StyleFeature::TextLayerAnimations) {
        if(_features >= Feature::Animations)
            ui.textLayer().shared().setStyleAnimation<TextStyle,
                nullptr,
                Implementation::styleAnimationOnLeaveBlurRelease,
                Implementation::styleAnimationPersistent<Animation::Easing::exponentialInOut>>();
        else if(_features >= Feature::EssentialAnimations)
            ui.textLayer().shared().setStyleAnimation<TextStyle,
                nullptr,
                nullptr,
                Implementation::styleAnimationPersistent<Animation::Easing::step>>();
        else CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
    }

    /* Event layer */
    if(features >= StyleFeature::EventLayer) {
        /* Right now nothing to set here. It's present in features() mainly in
           order to make UserInterface implicitly add this layer for use by the
           application. */
    }

    /* Layout layer. So far just min sizes, paddings and margins. */
    if(features >= StyleFeature::LayoutLayer) {
        ui.layoutLayer().setStyle(
            Containers::stridedArrayView(LayoutStylesMcssDark).slice(&std::remove_all_extents<decltype(LayoutStylesMcssDark)>::type::minSize),
            {},
            {},
            Containers::stridedArrayView(LayoutStylesMcssDark).slice(&std::remove_all_extents<decltype(LayoutStylesMcssDark)>::type::padding),
            Containers::stridedArrayView(LayoutStylesMcssDark).slice(&std::remove_all_extents<decltype(LayoutStylesMcssDark)>::type::margin));
    }

    /* Snap layouter */
    if(features >= StyleFeature::SnapLayouter) {
        /* Right now nothing to set here. It's present in features() mainly in
           order to make UserInterface implicitly add this layer for use by the
           application. */
    }

    return true;
}

}}
