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

#include "AbstractStyle.h"

#include <Corrade/Containers/EnumSet.hpp>
#include <Corrade/Utility/Assert.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/Text/AbstractGlyphCache.h>

#include "Magnum/Whee/BaseLayer.h"
#include "Magnum/Whee/TextLayer.h"
#include "Magnum/Whee/UserInterface.h"

namespace Magnum { namespace Whee {

Debug& operator<<(Debug& debug, const StyleFeature value) {
    debug << "Whee::StyleFeature" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case StyleFeature::value: return debug << "::" #value;
        _c(BaseLayer)
        _c(TextLayer)
        _c(TextLayerImages)
        _c(EventLayer)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "(" << Debug::nospace << Debug::hex << UnsignedByte(value) << Debug::nospace << ")";
}

Debug& operator<<(Debug& debug, const StyleFeatures value) {
    return Containers::enumSetDebugOutput(debug, value, "Whee::StyleFeatures{}", {
        StyleFeature::BaseLayer,
        StyleFeature::TextLayer,
        StyleFeature::TextLayerImages,
        StyleFeature::EventLayer,
    });
}

AbstractStyle::AbstractStyle() = default;

AbstractStyle::~AbstractStyle() = default;

StyleFeatures AbstractStyle::features() const {
    const StyleFeatures out = doFeatures();
    CORRADE_ASSERT(out,
        "Whee::AbstractStyle::features(): implementation returned an empty set", {});
    return out;
}

UnsignedInt AbstractStyle::baseLayerStyleUniformCount() const {
    CORRADE_ASSERT(features() >= StyleFeature::BaseLayer,
        "Whee::AbstractStyle::baseLayerStyleUniformCount(): feature not supported", {});
    return doBaseLayerStyleUniformCount();
}

UnsignedInt AbstractStyle::doBaseLayerStyleUniformCount() const {
    return doBaseLayerStyleCount();
}

UnsignedInt AbstractStyle::baseLayerStyleCount() const {
    CORRADE_ASSERT(features() >= StyleFeature::BaseLayer,
        "Whee::AbstractStyle::baseLayerStyleCount(): feature not supported", {});
    return doBaseLayerStyleCount();
}

UnsignedInt AbstractStyle::doBaseLayerStyleCount() const {
    CORRADE_ASSERT_UNREACHABLE("Whee::AbstractStyle::baseLayerStyleCount(): feature advertised but not implemented", {});
}

UnsignedInt AbstractStyle::textLayerStyleUniformCount() const {
    CORRADE_ASSERT(features() >= StyleFeature::TextLayer,
        "Whee::AbstractStyle::textLayerStyleUniformCount(): feature not supported", {});
    return doTextLayerStyleUniformCount();
}

UnsignedInt AbstractStyle::doTextLayerStyleUniformCount() const {
    return doTextLayerStyleCount();
}

UnsignedInt AbstractStyle::textLayerStyleCount() const {
    CORRADE_ASSERT(features() >= StyleFeature::TextLayer,
        "Whee::AbstractStyle::textLayerStyleCount(): feature not supported", {});
    return doTextLayerStyleCount();
}

UnsignedInt AbstractStyle::doTextLayerStyleCount() const {
    CORRADE_ASSERT_UNREACHABLE("Whee::AbstractStyle::textLayerStyleCount(): feature advertised but not implemented", {});
}

PixelFormat AbstractStyle::textLayerGlyphCacheFormat() const {
    CORRADE_ASSERT(features() >= StyleFeature::TextLayer,
        "Whee::AbstractStyle::textLayerGlyphCacheFormat(): feature not supported", {});
    return doTextLayerGlyphCacheFormat();
}

PixelFormat AbstractStyle::doTextLayerGlyphCacheFormat() const {
    return PixelFormat::R8Unorm;
}

Vector3i AbstractStyle::textLayerGlyphCacheSize(StyleFeatures features) const {
    CORRADE_ASSERT(this->features() >= StyleFeature::TextLayer,
        "Whee::AbstractStyle::textLayerGlyphCacheSize(): feature not supported", {});
    CORRADE_ASSERT(features >= StyleFeature::TextLayer,
        "Whee::AbstractStyle::textLayerGlyphCacheSize(): expected a superset of" << StyleFeature::TextLayer << "but got" << features, {});
    CORRADE_ASSERT(features <= this->features(),
        "Whee::AbstractStyle::textLayerGlyphCacheSize():" << features << "not a subset of supported" << this->features(), {});
    return Math::max(doTextLayerGlyphCacheSize(features), _textLayerGlyphCacheSize);
}

Vector3i AbstractStyle::doTextLayerGlyphCacheSize(StyleFeatures) const {
    CORRADE_ASSERT_UNREACHABLE("Whee::AbstractStyle::textLayerGlyphCacheSize(): feature advertised but not implemented", {});
}

Vector2i AbstractStyle::textLayerGlyphCachePadding() const {
    CORRADE_ASSERT(features() >= StyleFeature::TextLayer,
        "Whee::AbstractStyle::textLayerGlyphCachePadding(): feature not supported", {});
    return Math::max(doTextLayerGlyphCachePadding(), _textLayerGlyphCachePadding);
}

Vector2i AbstractStyle::doTextLayerGlyphCachePadding() const {
    return Vector2i{1};
}

AbstractStyle& AbstractStyle::setTextLayerGlyphCacheSize(const Vector3i& size, const Vector2i& padding) {
    _textLayerGlyphCacheSize = size;
    _textLayerGlyphCachePadding = padding;
    return *this;
}

bool AbstractStyle::apply(UserInterface& ui, const StyleFeatures features, PluginManager::Manager<Trade::AbstractImporter>* const importerManager, PluginManager::Manager<Text::AbstractFont>* const fontManager) const {
    CORRADE_ASSERT(features,
        "Whee::AbstractStyle::apply(): no features specified", {});
    CORRADE_ASSERT(features <= this->features(),
        "Whee::AbstractStyle::apply():" << features << "not a subset of supported" << this->features(), {});
    #ifndef CORRADE_NO_ASSERT
    if(features >= StyleFeature::BaseLayer) {
        CORRADE_ASSERT(ui.hasBaseLayer(),
            "Whee::AbstractStyle::apply(): base layer not present in the user interface", {});
        const BaseLayer::Shared& shared = ui.baseLayer().shared();
        CORRADE_ASSERT(
            shared.styleUniformCount() == baseLayerStyleUniformCount() &&
            shared.styleCount() == baseLayerStyleCount(),
            "Whee::AbstractStyle::apply(): style has" << baseLayerStyleUniformCount() << "uniforms and" << baseLayerStyleCount() << "styles but the base layer has" << shared.styleUniformCount() << "and" << shared.styleCount(), {});
    }
    if(features >= StyleFeature::TextLayer) {
        CORRADE_ASSERT(ui.hasTextLayer(),
            "Whee::AbstractStyle::apply(): text layer not present in the user interface", {});
        const TextLayer::Shared& shared = ui.textLayer().shared();
        CORRADE_ASSERT(
            shared.styleUniformCount() == textLayerStyleUniformCount() &&
            shared.styleCount() == textLayerStyleCount(),
            "Whee::AbstractStyle::apply(): style has" << textLayerStyleUniformCount() << "uniforms and" << textLayerStyleCount() << "styles but the text layer has" << shared.styleUniformCount() << "and" << shared.styleCount(), {});

        CORRADE_ASSERT(shared.hasGlyphCache(),
            "Whee::AbstractStyle::apply(): glyph cache not present in the text layer", {});
        const Text::AbstractGlyphCache& cache = shared.glyphCache();
        const Vector3i cacheSize = textLayerGlyphCacheSize(features);
        CORRADE_ASSERT(
            cache.format() == textLayerGlyphCacheFormat() &&
            cache.size() == cacheSize &&
            cache.padding() == textLayerGlyphCachePadding(),
            "Whee::AbstractStyle::apply(): style has a" << textLayerGlyphCacheFormat() << "glyph cache of size" << Debug::packed << cacheSize << "and padding" << Debug::packed << textLayerGlyphCachePadding() << "but the text layer has" << cache.format() << Debug::nospace << "," << Debug::packed << cache.size() << "and" << Debug::packed << cache.padding(), {});

        CORRADE_ASSERT(fontManager,
            "Whee::AbstractStyle::apply(): fontManager has to be specified for applying a text layer style", {});
    }
    if(features >= StyleFeature::TextLayerImages) {
        CORRADE_ASSERT(ui.hasTextLayer(),
            "Whee::AbstractStyle::apply(): text layer not present in the user interface", {});
        CORRADE_ASSERT(importerManager,
            "Whee::AbstractStyle::apply(): importerManager has to be specified for applying text layer style images", {});
    }
    if(features >= StyleFeature::EventLayer) {
        CORRADE_ASSERT(ui.hasEventLayer(),
            "Whee::AbstractStyle::apply(): event layer not present in the user interface", {});
    }
    #endif

    return doApply(ui, features, importerManager, fontManager);
}

}}