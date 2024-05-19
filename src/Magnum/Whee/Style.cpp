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

#include "Style.h"
#include "Style.hpp"

#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/StaticArray.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Utility/Algorithms.h>
#include <Corrade/Utility/Resource.h>
#include <Magnum/ImageView.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Range.h>
#include <Magnum/Text/AbstractFont.h>
#include <Magnum/Text/AbstractGlyphCache.h>
#include <Magnum/Text/Alignment.h>
#include <Magnum/TextureTools/Atlas.h>
#include <Magnum/Trade/AbstractImporter.h>
#include <Magnum/Trade/ImageData.h>

#include "Magnum/Whee/BaseLayer.h"
#include "Magnum/Whee/EventLayer.h"
#include "Magnum/Whee/TextLayer.h"
#include "Magnum/Whee/UserInterface.h"

#ifdef MAGNUM_BUILD_STATIC
static void importShaderResources() {
    CORRADE_RESOURCE_INITIALIZE(MagnumWhee_RESOURCES)
}
#endif

namespace Magnum { namespace Whee {

namespace Implementation {

namespace {

/* The returned values are in order InactiveOut, InactiveOver, FocusedOut,
   FocusedOver, PressedOut, PressedOver, Disabled (i.e., the same order as the
   arguments in setStyleTransition()). Styles that don't have a focused variant
   reuse the inactive one there. */
Containers::StaticArray<7, BaseStyle> styleTransition(const BaseStyle index) {
    /* LCOV_EXCL_START. Even if I manage to hit all cases here, it still
       reports just one line from each return expression as covered. Which adds
       so much false positives to the output that it's not worth for me to try
       to fix this. */
    /** @todo look at this again when gcov / lcov fixes itself */
    switch(index) {
        case BaseStyle::ButtonDefaultInactiveOut:
        case BaseStyle::ButtonDefaultInactiveOver:
        case BaseStyle::ButtonDefaultPressedOut:
        case BaseStyle::ButtonDefaultPressedOver:
            return {BaseStyle::ButtonDefaultInactiveOut,
                    BaseStyle::ButtonDefaultInactiveOver,
                    BaseStyle::ButtonDefaultInactiveOut,
                    BaseStyle::ButtonDefaultInactiveOver,
                    BaseStyle::ButtonDefaultPressedOut,
                    BaseStyle::ButtonDefaultPressedOver,
                    BaseStyle::ButtonDefaultDisabled};
        case BaseStyle::ButtonPrimaryInactiveOut:
        case BaseStyle::ButtonPrimaryInactiveOver:
        case BaseStyle::ButtonPrimaryPressedOut:
        case BaseStyle::ButtonPrimaryPressedOver:
            return {BaseStyle::ButtonPrimaryInactiveOut,
                    BaseStyle::ButtonPrimaryInactiveOver,
                    BaseStyle::ButtonPrimaryInactiveOut,
                    BaseStyle::ButtonPrimaryInactiveOver,
                    BaseStyle::ButtonPrimaryPressedOut,
                    BaseStyle::ButtonPrimaryPressedOver,
                    BaseStyle::ButtonPrimaryDisabled};
        case BaseStyle::ButtonSuccessInactiveOut:
        case BaseStyle::ButtonSuccessInactiveOver:
        case BaseStyle::ButtonSuccessPressedOut:
        case BaseStyle::ButtonSuccessPressedOver:
            return {BaseStyle::ButtonSuccessInactiveOut,
                    BaseStyle::ButtonSuccessInactiveOver,
                    BaseStyle::ButtonSuccessInactiveOut,
                    BaseStyle::ButtonSuccessInactiveOver,
                    BaseStyle::ButtonSuccessPressedOut,
                    BaseStyle::ButtonSuccessPressedOver,
                    BaseStyle::ButtonSuccessDisabled};
        case BaseStyle::ButtonWarningInactiveOut:
        case BaseStyle::ButtonWarningInactiveOver:
        case BaseStyle::ButtonWarningPressedOut:
        case BaseStyle::ButtonWarningPressedOver:
            return {BaseStyle::ButtonWarningInactiveOut,
                    BaseStyle::ButtonWarningInactiveOver,
                    BaseStyle::ButtonWarningInactiveOut,
                    BaseStyle::ButtonWarningInactiveOver,
                    BaseStyle::ButtonWarningPressedOut,
                    BaseStyle::ButtonWarningPressedOver,
                    BaseStyle::ButtonWarningDisabled};
        case BaseStyle::ButtonDangerInactiveOut:
        case BaseStyle::ButtonDangerInactiveOver:
        case BaseStyle::ButtonDangerPressedOut:
        case BaseStyle::ButtonDangerPressedOver:
            return {BaseStyle::ButtonDangerInactiveOut,
                    BaseStyle::ButtonDangerInactiveOver,
                    BaseStyle::ButtonDangerInactiveOut,
                    BaseStyle::ButtonDangerInactiveOver,
                    BaseStyle::ButtonDangerPressedOut,
                    BaseStyle::ButtonDangerPressedOver,
                    BaseStyle::ButtonDangerDisabled};
        case BaseStyle::ButtonInfoInactiveOut:
        case BaseStyle::ButtonInfoInactiveOver:
        case BaseStyle::ButtonInfoPressedOut:
        case BaseStyle::ButtonInfoPressedOver:
            return {BaseStyle::ButtonInfoInactiveOut,
                    BaseStyle::ButtonInfoInactiveOver,
                    BaseStyle::ButtonInfoInactiveOut,
                    BaseStyle::ButtonInfoInactiveOver,
                    BaseStyle::ButtonInfoPressedOut,
                    BaseStyle::ButtonInfoPressedOver,
                    BaseStyle::ButtonInfoDisabled};
        case BaseStyle::ButtonDimInactiveOut:
        case BaseStyle::ButtonDimInactiveOver:
        case BaseStyle::ButtonDimPressedOut:
        case BaseStyle::ButtonDimPressedOver:
            return {BaseStyle::ButtonDimInactiveOut,
                    BaseStyle::ButtonDimInactiveOver,
                    BaseStyle::ButtonDimInactiveOut,
                    BaseStyle::ButtonDimInactiveOver,
                    BaseStyle::ButtonDimPressedOut,
                    BaseStyle::ButtonDimPressedOver,
                    BaseStyle::ButtonDimDisabled};
        case BaseStyle::ButtonFlatInactiveOut:
        case BaseStyle::ButtonFlatInactiveOver:
        case BaseStyle::ButtonFlatPressedOut:
        case BaseStyle::ButtonFlatPressedOver:
            return {BaseStyle::ButtonFlatInactiveOut,
                    BaseStyle::ButtonFlatInactiveOver,
                    BaseStyle::ButtonFlatInactiveOut,
                    BaseStyle::ButtonFlatInactiveOver,
                    BaseStyle::ButtonFlatPressedOut,
                    BaseStyle::ButtonFlatPressedOver,
                    BaseStyle::ButtonFlatDisabled};
        case BaseStyle::ButtonDefaultDisabled:
        case BaseStyle::ButtonPrimaryDisabled:
        case BaseStyle::ButtonSuccessDisabled:
        case BaseStyle::ButtonWarningDisabled:
        case BaseStyle::ButtonDangerDisabled:
        case BaseStyle::ButtonInfoDisabled:
        case BaseStyle::ButtonDimDisabled:
        case BaseStyle::ButtonFlatDisabled:
            CORRADE_INTERNAL_ASSERT_UNREACHABLE();
    }
    /* LCOV_EXCL_STOP */

    CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
}

}

BaseStyle styleTransitionToInactiveOut(const BaseStyle index) {
    return styleTransition(index)[0];
}
BaseStyle styleTransitionToInactiveOver(const BaseStyle index) {
    return styleTransition(index)[1];
}
BaseStyle styleTransitionToFocusedOut(const BaseStyle index) {
    return styleTransition(index)[2];
}
BaseStyle styleTransitionToFocusedOver(const BaseStyle index) {
    return styleTransition(index)[3];
}
BaseStyle styleTransitionToPressedOut(const BaseStyle index) {
    return styleTransition(index)[4];
}
BaseStyle styleTransitionToPressedOver(const BaseStyle index) {
    return styleTransition(index)[5];
}
BaseStyle styleTransitionToDisabled(const BaseStyle index) {
    return styleTransition(index)[6];
}

namespace {

/* The returned values are in order InactiveOut, InactiveOver, FocusedOut,
   FocusedOver, PressedOut, PressedOver, Disabled (i.e., the same order as the
   arguments in setStyleTransition()). Styles that don't have a focused variant
   reuse the inactive one there. */
Containers::StaticArray<7, TextStyle> styleTransition(const TextStyle index) {
    /* LCOV_EXCL_START. Even if I manage to hit all cases here, it still
       reports just one line from each return expression as covered. Which adds
       so much false positives to the output that it's not worth for me to try
       to fix this. */
    /** @todo look at this again when gcov / lcov fixes itself */
    switch(index) {
        case TextStyle::ButtonIconOnly:
        case TextStyle::ButtonPressedIconOnly:
            return {TextStyle::ButtonIconOnly,
                    TextStyle::ButtonIconOnly,
                    TextStyle::ButtonIconOnly,
                    TextStyle::ButtonIconOnly,
                    TextStyle::ButtonPressedIconOnly,
                    TextStyle::ButtonPressedIconOnly,
                    TextStyle::ButtonDisabledIconOnly};
        case TextStyle::ButtonTextOnly:
        case TextStyle::ButtonPressedTextOnly:
            return {TextStyle::ButtonTextOnly,
                    TextStyle::ButtonTextOnly,
                    TextStyle::ButtonTextOnly,
                    TextStyle::ButtonTextOnly,
                    TextStyle::ButtonPressedTextOnly,
                    TextStyle::ButtonPressedTextOnly,
                    TextStyle::ButtonDisabledTextOnly};
        case TextStyle::ButtonIcon:
        case TextStyle::ButtonPressedIcon:
            return {TextStyle::ButtonIcon,
                    TextStyle::ButtonIcon,
                    TextStyle::ButtonIcon,
                    TextStyle::ButtonIcon,
                    TextStyle::ButtonPressedIcon,
                    TextStyle::ButtonPressedIcon,
                    TextStyle::ButtonDisabledIcon};
        case TextStyle::ButtonText:
        case TextStyle::ButtonPressedText:
            return {TextStyle::ButtonText,
                    TextStyle::ButtonText,
                    TextStyle::ButtonText,
                    TextStyle::ButtonText,
                    TextStyle::ButtonPressedText,
                    TextStyle::ButtonPressedText,
                    TextStyle::ButtonDisabledText};
        case TextStyle::ButtonFlatInactiveOutIconOnly:
        case TextStyle::ButtonFlatInactiveOverIconOnly:
        case TextStyle::ButtonFlatPressedOutIconOnly:
        case TextStyle::ButtonFlatPressedOverIconOnly:
            return {TextStyle::ButtonFlatInactiveOutIconOnly,
                    TextStyle::ButtonFlatInactiveOverIconOnly,
                    TextStyle::ButtonFlatInactiveOutIconOnly,
                    TextStyle::ButtonFlatInactiveOverIconOnly,
                    TextStyle::ButtonFlatPressedOutIconOnly,
                    TextStyle::ButtonFlatPressedOverIconOnly,
                    TextStyle::ButtonFlatDisabledIconOnly};
        case TextStyle::ButtonFlatInactiveOutTextOnly:
        case TextStyle::ButtonFlatInactiveOverTextOnly:
        case TextStyle::ButtonFlatPressedOutTextOnly:
        case TextStyle::ButtonFlatPressedOverTextOnly:
            return {TextStyle::ButtonFlatInactiveOutTextOnly,
                    TextStyle::ButtonFlatInactiveOverTextOnly,
                    TextStyle::ButtonFlatInactiveOutTextOnly,
                    TextStyle::ButtonFlatInactiveOverTextOnly,
                    TextStyle::ButtonFlatPressedOutTextOnly,
                    TextStyle::ButtonFlatPressedOverTextOnly,
                    TextStyle::ButtonFlatDisabledTextOnly};
        case TextStyle::ButtonFlatInactiveOutIcon:
        case TextStyle::ButtonFlatInactiveOverIcon:
        case TextStyle::ButtonFlatPressedOutIcon:
        case TextStyle::ButtonFlatPressedOverIcon:
            return {TextStyle::ButtonFlatInactiveOutIcon,
                    TextStyle::ButtonFlatInactiveOverIcon,
                    TextStyle::ButtonFlatInactiveOutIcon,
                    TextStyle::ButtonFlatInactiveOverIcon,
                    TextStyle::ButtonFlatPressedOutIcon,
                    TextStyle::ButtonFlatPressedOverIcon,
                    TextStyle::ButtonFlatDisabledIcon};
        case TextStyle::ButtonFlatInactiveOutText:
        case TextStyle::ButtonFlatInactiveOverText:
        case TextStyle::ButtonFlatPressedOutText:
        case TextStyle::ButtonFlatPressedOverText:
            return {TextStyle::ButtonFlatInactiveOutText,
                    TextStyle::ButtonFlatInactiveOverText,
                    TextStyle::ButtonFlatInactiveOutText,
                    TextStyle::ButtonFlatInactiveOverText,
                    TextStyle::ButtonFlatPressedOutText,
                    TextStyle::ButtonFlatPressedOverText,
                    TextStyle::ButtonFlatDisabledText};
        case TextStyle::ButtonDisabledIconOnly:
        case TextStyle::ButtonDisabledTextOnly:
        case TextStyle::ButtonDisabledIcon:
        case TextStyle::ButtonDisabledText:
        case TextStyle::ButtonFlatDisabledIconOnly:
        case TextStyle::ButtonFlatDisabledTextOnly:
        case TextStyle::ButtonFlatDisabledIcon:
        case TextStyle::ButtonFlatDisabledText:
            CORRADE_INTERNAL_ASSERT_UNREACHABLE();
    }
    /* LCOV_EXCL_STOP */

    CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
}

}

TextStyle styleTransitionToInactiveOut(const TextStyle index) {
    return styleTransition(index)[0];
}
TextStyle styleTransitionToInactiveOver(const TextStyle index) {
    return styleTransition(index)[1];
}
TextStyle styleTransitionToFocusedOut(const TextStyle index) {
    return styleTransition(index)[2];
}
TextStyle styleTransitionToFocusedOver(const TextStyle index) {
    return styleTransition(index)[3];
}
TextStyle styleTransitionToPressedOut(const TextStyle index) {
    return styleTransition(index)[4];
}
TextStyle styleTransitionToPressedOver(const TextStyle index) {
    return styleTransition(index)[5];
}
TextStyle styleTransitionToDisabled(const TextStyle index) {
    return styleTransition(index)[6];
}

}

Debug& operator<<(Debug& debug, const Icon value) {
    debug << "Whee::Icon" << Debug::nospace;

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
    #include "Magnum/Whee/Implementation/baseStyleUniformsMcssDark.h"
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
    #include "Magnum/Whee/Implementation/textStyleUniformsMcssDark.h"
    #undef _c
};
static_assert(Implementation::TextStyleUniformCount == Containers::arraySize(TextStyleUniformsMcssDark),
    "outdated TextStyleUniformCount value");

constexpr struct {
    UnsignedInt uniform;
    Text::Alignment alignment;
    Vector4 padding;
} TextStylesMcssDark[]{
    #define _c(style, suffix, font, alignment, ...) {UnsignedInt(TextStyleUniform::style), Text::Alignment::alignment, __VA_ARGS__},
    #include "Magnum/Whee/Implementation/textStyleMcssDark.h"
    #undef _c
};
static_assert(Implementation::TextStyleCount == Containers::arraySize(TextStylesMcssDark),
    "outdated TextStyleCount value");

}

StyleFeatures McssDarkStyle::doFeatures() const {
    return StyleFeature::BaseLayer|
           StyleFeature::TextLayer|
           StyleFeature::TextLayerImages|
           StyleFeature::EventLayer;
}

UnsignedInt McssDarkStyle::doBaseLayerStyleUniformCount() const {
    return Implementation::BaseStyleCount;
}

UnsignedInt McssDarkStyle::doBaseLayerStyleCount() const {
    return Implementation::BaseStyleUniformCount;
}

UnsignedInt McssDarkStyle::doTextLayerStyleUniformCount() const {
    return Implementation::TextStyleUniformCount;
}

UnsignedInt McssDarkStyle::doTextLayerStyleCount() const {
    return Implementation::TextStyleCount;
}

Vector3i McssDarkStyle::doTextLayerGlyphCacheSize(StyleFeatures) const {
    /* 256x256 is enough only for DPI scale of 1, adding some extra space */
    /** @todo Make this dependent on DPI scale */
    return {512, 256, 1};
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

    /* Icon font. Add also if just the text layer style is applied (where it
       gets assigned to icon styles, but without any icons actually loaded).

       MSVC says iconFont might be used uninitialized without the {}. It won't,
       the thing is just stupid. */
    Whee::FontHandle iconFont{};
    if(features & (StyleFeature::TextLayer|StyleFeature::TextLayerImages)) {
        #ifdef MAGNUM_BUILD_STATIC
        if(!Utility::Resource::hasGroup("MagnumWhee"_s))
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
        const Utility::Resource rs{"MagnumWhee"_s};

        Containers::Pointer<Text::AbstractFont> font = fontManager->loadAndInstantiate("TrueTypeFont");
        if(!font || !font->openData(rs.getRaw("SourceSansPro-Regular.ttf"_s), 16.0f*2*(Vector2{ui.framebufferSize()}/ui.size()).max())) {
            Error{} << "Whee::McssDarkStyle::apply(): cannot open a font";
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
        const Whee::FontHandle mainFont = shared.addFont(Utility::move(font), 16.0f);

        /* Font handles matching all styles. References either the `mainFont`
           or the `iconFont` defined above. */
        const Whee::FontHandle fontHandles[]{
            #define _c(style, suffix, font, ...) font,
            #include "Magnum/Whee/Implementation/textStyleMcssDark.h"
            #undef _c
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
                /* No cursor / selection styles used yet */
                {}, {},
                Containers::stridedArrayView(TextStylesMcssDark).slice(&std::remove_all_extents<decltype(TextStylesMcssDark)>::type::padding))
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
        const Utility::Resource rs{"MagnumWhee"_s};

        Containers::Pointer<Trade::AbstractImporter> importer = importerManager->loadAndInstantiate("AnyImageImporter");
        Containers::Optional<Trade::ImageData2D> image;
        if(!importer || !importer->openMemory(rs.getRaw("icons.png")) || !(image = importer->image2D(0))) {
            Error{} << "Whee::McssDarkStyle::apply(): cannot open an icon atlas";
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
            Error{} << "Whee::McssDarkStyle::apply(): expected" << PixelFormat::R8Unorm << "icons but got an image with" << image->format();
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
            Error{} << "Whee::McssDarkStyle::apply(): cannot fit" << Implementation::IconCount << "icons into the glyph cache";
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

    /* Event layer */
    if(features >= StyleFeature::EventLayer) {
        /* Right now nothing to set here. It's present in features() mainly in
           order to make UserInterface implicitly add this layer for use by the
           application. */
    }

    return true;
}

}}
