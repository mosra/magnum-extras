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

#include "Theme.h"

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

#include "Magnum/Ui/AbstractTheme.hpp"
#include "Magnum/Ui/BaseLayer.h"
#include "Magnum/Ui/BaseLayerAnimator.h"
#include "Magnum/Ui/EventLayer.h"
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/LayoutLayer.h"
#include "Magnum/Ui/SnapLayouter.h"
#include "Magnum/Ui/TextLayer.h"
#include "Magnum/Ui/TextLayerAnimator.h"
#include "Magnum/Ui/UserInterface.h"
#include "Magnum/Ui/Implementation/PasswordFont.h"
#include "Magnum/Ui/Implementation/Theme.h" /* TextFont enum */

#ifdef MAGNUM_UI_BUILD_STATIC
static void importShaderResources() {
    CORRADE_RESOURCE_INITIALIZE(MagnumUi_RESOURCES)
}
#endif

namespace Magnum { namespace Ui {

using namespace Math::Literals;

namespace Implementation {

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
       style == TextStyle::InputDefaultPasswordFocused ||
       style == TextStyle::InputSuccessFocused ||
       style == TextStyle::InputSuccessPasswordFocused ||
       style == TextStyle::InputWarningFocused ||
       style == TextStyle::InputWarningPasswordFocused ||
       style == TextStyle::InputDangerFocused ||
       style == TextStyle::InputDangerPasswordFocused ||
       style == TextStyle::InputFlatFocused ||
       style == TextStyle::InputFlatPasswordFocused)
    {
        /* It's just +1 but let's not have it too cryptic here. The compiler
           hopefully optimizes this? */
        TextStyle sourceStyle;
        switch(style) {
            case TextStyle::InputDefaultFocused:
            case TextStyle::InputDefaultPasswordFocused:
                sourceStyle = TextStyle::InputDefaultFocusedBlink;
                break;
            case TextStyle::InputSuccessFocused:
            case TextStyle::InputSuccessPasswordFocused:
                sourceStyle = TextStyle::InputSuccessFocusedBlink;
                break;
            case TextStyle::InputWarningFocused:
            case TextStyle::InputWarningPasswordFocused:
                sourceStyle = TextStyle::InputWarningFocusedBlink;
                break;
            case TextStyle::InputDangerFocused:
            case TextStyle::InputDangerPasswordFocused:
                sourceStyle = TextStyle::InputDangerFocusedBlink;
                break;
            case TextStyle::InputFlatFocused:
            case TextStyle::InputFlatPasswordFocused:
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

Debug& operator<<(Debug& debug, const DarkTheme::Feature value) {
    debug << "Ui::DarkTheme::Feature" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case DarkTheme::Feature::value: return debug << "::" #value;
        _c(EssentialAnimations)
        _c(Animations)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "(" << Debug::nospace << Debug::hex << UnsignedByte(value) << Debug::nospace << ")";
}

Debug& operator<<(Debug& debug, const DarkTheme::Features value) {
    return Containers::enumSetDebugOutput(debug, value, "Ui::DarkTheme::Features{}", {
        DarkTheme::Feature::Animations,
        /* Implied by Animations, has to be after */
        DarkTheme::Feature::EssentialAnimations,
    });
}

using namespace Containers::Literals;
using namespace Math::Literals;
using Implementation::BaseStyle;
using Implementation::TextStyle;
using Implementation::TextFont;

namespace {

/* 1 (true, screen)-pixel radius independently of UI scale */
constexpr BaseLayerCommonStyleUniform BaseCommonStyleUniformDark{1.0f};

#ifndef CORRADE_MSVC2015_COMPATIBILITY
constexpr /* Trust me, you don't want to be on this compiler */
#endif
const BaseLayerStyleUniform BaseStyleUniformsDark[]{
    #define _c(style, ...) {__VA_ARGS__},
    #include "Magnum/Ui/Implementation/themeDarkBaseStyleUniforms.h"
    #undef _c
};

constexpr TextLayerCommonStyleUniform TextCommonStyleUniformDark{};

#ifndef CORRADE_MSVC2015_COMPATIBILITY
constexpr /* Trust me, you don't want to be on this compiler */
#endif
const TextLayerStyleUniform TextStyleUniformsDark[]{
    #define _c(...) {__VA_ARGS__},
    #include "Magnum/Ui/Implementation/themeDarkTextStyleUniforms.h"
    #undef _c
};

/* MSVC 2015, 2017 and 2019 cannot handle this in the form of
    constexpr struct TextLayerStyle {
        constexpr TextLayerStyle(...);
    } LayoutStylesDark[]{
        ...
    };
   complaining that the TextLayerStyle constructor isn't constexpr. Splitting
   the struct and array definition makes it work. MSVC 2022 works. */
/** @todo clean this up once MSVC 2017 and 2019 is gone */
struct TextLayerStyle {
    constexpr /*implicit*/ TextLayerStyle(Text::Alignment alignment, const Vector4& padding): alignment{alignment}, padding{padding}, cursorStyle{-1}, selectionStyle{-1} {}
    constexpr /*implicit*/ TextLayerStyle(Text::Alignment alignment, const Vector4& padding, Int cursorStyle, Int selectionStyle): alignment{alignment}, padding{padding}, cursorStyle{cursorStyle}, selectionStyle{selectionStyle} {}

    Text::Alignment alignment;
    Vector4 padding;
    Int cursorStyle, selectionStyle;
};

constexpr TextLayerStyle TextStylesDark[]{
    #define _c(style, font, alignment, ...) {Text::Alignment::alignment, __VA_ARGS__},
    #include "Magnum/Ui/Implementation/themeDarkTextStyles.h"
    #undef _c
};
constexpr UnsignedInt TextStyleUniformMappingDark[]{
    #define _u(...) __VA_ARGS__
    #include "Magnum/Ui/Implementation/themeDarkTextStyles.h"
    #undef _u
};

/* 1 (true, screen)-pixel radius independently of UI scale */
constexpr TextLayerCommonEditingStyleUniform TextCommonEditingStyleUniformDark{1.0f};

#ifndef CORRADE_MSVC2015_COMPATIBILITY
constexpr /* Trust me, you don't want to be on this compiler */
#endif
const TextLayerEditingStyleUniform TextEditingStyleUniformsDark[]{
    #define _c(padding0, padding1, padding2, padding3, ...) {__VA_ARGS__},
    #include "Magnum/Ui/Implementation/themeDarkTextEditingStyles.h"
    #undef _c
};

constexpr Vector4 TextEditingStylePaddingsDark[]{
    #define _c(padding0, padding1, padding2, padding3, ...) padding0, padding1, padding2, padding3,
    #include "Magnum/Ui/Implementation/themeDarkTextEditingStyles.h"
    #undef _c
};
constexpr Int TextEditingStyleUniformMappingDark[]{
    #define _u(...) __VA_ARGS__
    #include "Magnum/Ui/Implementation/themeDarkTextEditingStyles.h"
    #undef _u
};

/* MSVC 2015, 2017 and 2019 cannot handle this in the form of
    constexpr struct LayoutLayerStyle {
        constexpr LayoutLayerStyle(...);
    } LayoutStylesDark[]{
        ...
    };
   complaining that the LayoutLayerStyle constructor isn't constexpr. Splitting
   the struct and array definition makes it work. MSVC 2022 works. */
/** @todo clean this up once MSVC 2017 and 2019 is gone */
struct LayoutLayerStyle {
    constexpr /*implicit*/ LayoutLayerStyle(const Vector2& minSize): minSize{minSize} {}
    constexpr /*implicit*/ LayoutLayerStyle(const Vector2& minSize, const Vector4& padding, const Vector4& margin): minSize{minSize}, padding{padding}, margin{margin} {}
    constexpr /*implicit*/ LayoutLayerStyle(const Vector2& minSize, const Vector2& padding, const Vector2& margin): minSize{minSize}, padding{padding.x(), padding.y(), padding.x(), padding.y()}, margin{margin.x(), margin.y(), margin.x(), margin.y()} {}

    Vector2 minSize;
    Vector4 padding;
    Vector4 margin;
};

constexpr LayoutLayerStyle LayoutStylesDark[]{
    #define _c(style, ...) {__VA_ARGS__},
    #include "Magnum/Ui/Implementation/themeDarkLayoutStyles.h"
    #undef _c
};

}

DarkTheme::DarkTheme(const Features features): _features{features} {}

ThemeFeatures DarkTheme::doFeatures() const {
    return
        ThemeFeature::DataLayer|
        ThemeFeature::BaseLayer|
        ThemeFeature::TextLayer|
        ThemeFeature::TextLayerImages|
        ThemeFeature::EventLayer|
        ThemeFeature::LayoutLayer|
        ThemeFeature::SnapLayouter|
        ThemeFeature::GenericLayouter|
        /* Essential animations are currently just (text layer) cursor
           blinking */
        (_features >= Feature::Animations ? ThemeFeature::BaseLayerAnimations|ThemeFeature::TextLayerAnimations : ThemeFeatures{})|
        (_features >= Feature::EssentialAnimations ? ThemeFeature::TextLayerAnimations : ThemeFeatures{});
}

UnsignedInt DarkTheme::doBaseLayerStyleUniformCount() const {
    return Containers::arraySize(BaseStyleUniformsDark);
}

UnsignedInt DarkTheme::doBaseLayerStyleCount() const {
    /* There's currently no deduplication done for base layer styles */
    return Containers::arraySize(BaseStyleUniformsDark);
}

UnsignedInt DarkTheme::doBaseLayerDynamicStyleCount() const {
    return _features >= Feature::Animations ? 10 : 0;
}

UnsignedInt DarkTheme::doTextLayerStyleUniformCount() const {
    return Containers::arraySize(TextStyleUniformsDark);
}

UnsignedInt DarkTheme::doTextLayerStyleCount() const {
    return Containers::arraySize(TextStylesDark);
}

UnsignedInt DarkTheme::doTextLayerDynamicStyleCount() const {
    /* For essential animations, assuming there's just one blinking cursor at a
       time, add one dynamic style, and one more for safety */
    return _features >= Feature::Animations ? 10 :
        _features >= Feature::EssentialAnimations ? 2 : 0;
}

UnsignedInt DarkTheme::doTextLayerEditingStyleUniformCount() const {
    return Containers::arraySize(TextEditingStyleUniformsDark);
}

UnsignedInt DarkTheme::doTextLayerEditingStyleCount() const {
    return Containers::arraySize(TextEditingStyleUniformMappingDark);
}

Vector3i DarkTheme::doTextLayerGlyphCacheSize(ThemeFeatures) const {
    /** @todo Make this dependent on DPI scale */
    return {1024, 512, 1};
}

UnsignedInt DarkTheme::doLayoutLayerStyleCount() const {
    return Containers::arraySize(LayoutStylesDark);
}

bool DarkTheme::doApply(UserInterface& ui, const ThemeFeatures features, PluginManager::Manager<Trade::AbstractImporter>* importerManager, PluginManager::Manager<Text::AbstractFont>* fontManager) const {
    /* Base layer style */
    if(features >= ThemeFeature::BaseLayer) {
        ui.baseLayer().shared()
            .setStyle(
                BaseCommonStyleUniformDark,
                BaseStyleUniformsDark,
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
    if(features >= ThemeFeature::BaseLayerAnimations) {
        CORRADE_INTERNAL_ASSERT(_features >= Feature::Animations);
        ui.baseLayer().shared().setStyleAnimation<BaseStyle,
            nullptr,
            Implementation::styleAnimationOnLeaveBlurRelease,
            nullptr>();
    }

    /* List of font handles to be referenced by particular text styles using
       the TextFont enum */
    Ui::FontHandle fonts[Int(TextFont::Count)]{};

    /* Icon font. Add also if just the text layer style is applied (where it
       gets assigned to icon styles, but without any icons actually loaded). */
    if(features & (ThemeFeature::TextLayer|ThemeFeature::TextLayerImages)) {
        #ifdef MAGNUM_UI_BUILD_STATIC
        importShaderResources();
        #endif

        TextLayer::Shared& shared = ui.textLayer().shared();
        Text::AbstractGlyphCache& glyphCache = shared.glyphCache();
        /* The Icon enum reserves 0 for an invalid glyph, so add 1 */
        const UnsignedInt iconFontId = glyphCache.addFont(Implementation::IconCount + 1);
        /* The input is 64x64 squares, which are meant to be shown as 24x24 /
           32x32 squares in the UI units */
        /** @todo some DPI-aware machinery here, such as picking one of
            multiple icon images depending on the DPI scaling, or maybe just
            put these into a font */
        fonts[Int(TextFont::Icon)] = shared.addInstancelessFont(iconFontId, 24.0f/64.0f);
        fonts[Int(TextFont::LargeIcon)] = shared.addInstancelessFont(iconFontId, 32.0f/64.0f);
    }

    /* Text layer fonts and style */
    /** @todo figure out how to apply another style and replace the previous
        now-unused font *somehow*, such as by keeping track of which fonts
        correspond to which ThemeFeature, and then pruning the cache also */
    if(features >= ThemeFeature::TextLayer) {
        TextLayer::Shared& shared = ui.textLayer().shared();
        Text::AbstractGlyphCache& glyphCache = shared.glyphCache();
        const Utility::Resource rs{"MagnumUi"_s};

        Containers::Pointer<Text::AbstractFont> font = fontManager->loadAndInstantiate("TrueTypeFont");
        Containers::Pointer<Text::AbstractFont> fontLarge = fontManager->loadAndInstantiate("TrueTypeFont");
        if(!font || !fontLarge ||
           !font->openData(rs.getRaw("SourceSans3-Regular.otf"_s), 16.0f*2*(Vector2{ui.framebufferSize()}/ui.size()).max()) ||
           !fontLarge->openData(rs.getRaw("SourceSans3-Regular.otf"_s), 24.0f*2*(Vector2{ui.framebufferSize()}/ui.size()).max()))
        {
            Error{} << "Ui::DarkTheme::apply(): cannot open a font";
            return {};
        }
        /** @todo configurable way to fill the cache, or switch to on-demand
            by default once AbstractFont can fill the cache with glyph IDs */
        for(Text::AbstractFont* const i: {&*font, &*fontLarge}) if(!i->fillGlyphCache(glyphCache,
            "abcdefghijklmnopqrstuvwxyz"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "0123456789 _.,-+=*:;?!@$&#/\\|`\"'<>()[]{}%…"
            /* Bullet used by the password font */
            "•")
        ) {
            Error{} << "Ui::DarkTheme::apply(): cannot fill a glyph cache";
            return {};
        }

        /* Password font. Takes the bullet glyph from the main font. I hope the
           char32_t literal doesn't cause some bad shit on MSVC. The 1.3333f
           scale is based on vibes, default 1.0f was too terse, 2.0f is too
           wide. */
        const UnsignedInt passwordFontBulletGlyph = font->glyphId(U'•');
        CORRADE_INTERNAL_ASSERT(passwordFontBulletGlyph != 0);
        Containers::Pointer<Implementation::PasswordFont> passwordFont{InPlaceInit, glyphCache, *font, passwordFontBulletGlyph, 1.3333f};
        /** @todo create some interface for "always open" fonts, ugh */
        CORRADE_INTERNAL_ASSERT_OUTPUT(passwordFont->openData({}, font->size()));

        fonts[Int(TextFont::Main)] = shared.addFont(Utility::move(font), 16.0f);
        fonts[Int(TextFont::Password)] = shared.addFont(Utility::move(passwordFont), 16.0f);
        fonts[Int(TextFont::Large)] = shared.addFont(Utility::move(fontLarge), 24.0f);

        /* Font handles matching all styles. References either the `mainFont`
           or the `iconFont` defined above. */
        const Ui::FontHandle fontHandles[]{
            #define _c(style, font, ...) fonts[Int(TextFont::font)],
            #include "Magnum/Ui/Implementation/themeDarkTextStyles.h"
            #undef _c
        };

        shared
            .setStyle(
                TextCommonStyleUniformDark,
                TextStyleUniformsDark,
                TextStyleUniformMappingDark,
                fontHandles,
                Containers::stridedArrayView(TextStylesDark).slice(&std::remove_all_extents<decltype(TextStylesDark)>::type::alignment),
                /* No features coming from style used yet */
                {}, {}, {},
                Containers::stridedArrayView(TextStylesDark).slice(&std::remove_all_extents<decltype(TextStylesDark)>::type::cursorStyle),
                Containers::stridedArrayView(TextStylesDark).slice(&std::remove_all_extents<decltype(TextStylesDark)>::type::selectionStyle),
                Containers::stridedArrayView(TextStylesDark).slice(&std::remove_all_extents<decltype(TextStylesDark)>::type::padding))
            .setEditingStyle(
                TextCommonEditingStyleUniformDark,
                TextEditingStyleUniformsDark,
                TextEditingStyleUniformMappingDark,
                TextEditingStylePaddingsDark)
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
    if(features >= ThemeFeature::TextLayerImages) {
        TextLayer::Shared& shared = ui.textLayer().shared();
        Text::AbstractGlyphCache& glyphCache = shared.glyphCache();
        const Utility::Resource rs{"MagnumUi"_s};

        Containers::Pointer<Trade::AbstractImporter> importer = importerManager->loadAndInstantiate("AnyImageImporter");
        Containers::Optional<Trade::ImageData2D> image;
        if(!importer || !importer->openMemory(rs.getRaw("icons.png")) || !(image = importer->image2D(0))) {
            Error{} << "Ui::DarkTheme::apply(): cannot open an icon atlas";
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
            Error{} << "Ui::DarkTheme::apply(): expected" << PixelFormat::R8Unorm << "icons but got an image with" << image->format();
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
            Error{} << "Ui::DarkTheme::apply(): cannot fit" << Implementation::IconCount << "icons into the glyph cache";
            return {};
        }

        /* The font was added above, query the glyph cache ID of it */
        const UnsignedInt iconFontId = shared.glyphCacheFontId(fonts[Int(TextFont::Icon)]);

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
    if(features >= ThemeFeature::TextLayerAnimations) {
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

    /* Layout layer. So far just min sizes, paddings and margins. */
    if(features >= ThemeFeature::LayoutLayer) {
        ui.layoutLayer().setStyle(
            Containers::stridedArrayView(LayoutStylesDark).slice(&std::remove_all_extents<decltype(LayoutStylesDark)>::type::minSize),
            {},
            {},
            Containers::stridedArrayView(LayoutStylesDark).slice(&std::remove_all_extents<decltype(LayoutStylesDark)>::type::padding),
            Containers::stridedArrayView(LayoutStylesDark).slice(&std::remove_all_extents<decltype(LayoutStylesDark)>::type::margin));
    }

    /* DataLayer, EventLayer, LayoutLayer, SnapLayouter, GenericLayouter have
       nothing to be set here right now. They're present in features() mainly
       in order to make UserInterface implicitly add these for use by the
       widget implementations. */

    return true;
}

}}
