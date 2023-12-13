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

#include "TextLayer.h"

#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/BitArrayView.h>
#include <Corrade/Containers/GrowableArray.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/String.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Containers/Triple.h>
#include <Corrade/Utility/Algorithms.h>
#include <Magnum/Math/Functions.h>
#include <Magnum/Math/Matrix3.h>
#include <Magnum/Math/Swizzle.h>
#include <Magnum/Text/AbstractGlyphCache.h>
#include <Magnum/Text/Direction.h>
#include <Magnum/Text/Feature.h>
#include <Magnum/Text/Renderer.h>

#include "Magnum/Whee/Event.h"
#include "Magnum/Whee/Handle.h"
#include "Magnum/Whee/TextProperties.h"
#include "Magnum/Whee/Implementation/textLayerState.h"

namespace Magnum { namespace Whee {

Debug& operator<<(Debug& debug, const FontHandle value) {
    if(value == FontHandle::Null)
        return debug << "Whee::FontHandle::Null";
    return debug << "Whee::FontHandle(" << Debug::nospace << Debug::hex << fontHandleId(value) << Debug::nospace << "," << Debug::hex << fontHandleGeneration(value) << Debug::nospace << ")";
}

TextLayer::Shared::Shared(Containers::Pointer<State>&& state): AbstractVisualLayer::Shared{Utility::move(state)} {}

TextLayer::Shared::Shared(const UnsignedInt styleUniformCount, const UnsignedInt styleCount): Shared{Containers::pointer<State>(*this, styleUniformCount, styleCount)} {}

TextLayer::Shared::Shared(NoCreateT) noexcept: AbstractVisualLayer::Shared{NoCreate} {}

UnsignedInt TextLayer::Shared::styleUniformCount() const {
    return static_cast<const State&>(*_state).styleUniformCount;
}

TextLayer::Shared& TextLayer::Shared::setGlyphCache(Text::AbstractGlyphCache& cache) {
    State& state = static_cast<State&>(*_state);
    CORRADE_ASSERT(!state.glyphCache,
        "Whee::TextLayer::Shared::setGlyphCache(): glyph cache already set", *this);
    state.glyphCache = &cache;
    return *this;
}

bool TextLayer::Shared::hasGlyphCache() const {
    return static_cast<const State&>(*_state).glyphCache;
}

Text::AbstractGlyphCache& TextLayer::Shared::glyphCache() {
    return const_cast<Text::AbstractGlyphCache&>(const_cast<const TextLayer::Shared&>(*this).glyphCache());
}

const Text::AbstractGlyphCache& TextLayer::Shared::glyphCache() const {
    const State& state = static_cast<const State&>(*_state);
    CORRADE_ASSERT(state.glyphCache,
        "Whee::TextLayer::Shared::glyphCache(): no glyph cache set", *state.glyphCache);
    return *state.glyphCache;
}

std::size_t TextLayer::Shared::fontCount() const {
    return static_cast<const State&>(*_state).fonts.size();
}

namespace {
    /* TextLayer::setText() uses this too. It has access to the outer Shared
       API via shared() so it could call the public API directly, but this is
       two indirections less. */
    bool isHandleValid(const Containers::ArrayView<const Implementation::TextLayerFont> fonts, const FontHandle handle) {
        return fontHandleGeneration(handle) == 1 && fontHandleId(handle) < fonts.size();
    }
}

bool TextLayer::Shared::isHandleValid(const FontHandle handle) const {
    return Whee::isHandleValid(static_cast<const State&>(*_state).fonts, handle);
}

FontHandle TextLayer::Shared::addFont(Text::AbstractFont& font, const Float size) {
    State& state = static_cast<State&>(*_state);
    CORRADE_ASSERT(state.glyphCache,
        "Whee::TextLayer::Shared::addFont(): no glyph cache set", {});
    const Containers::Optional<UnsignedInt> glyphCacheFontId = state.glyphCache->findFont(font);
    CORRADE_ASSERT(glyphCacheFontId,
        "Whee::TextLayer::Shared::addFont(): font not found among" << state.glyphCache->fontCount() << "fonts in set glyph cache", {});
    CORRADE_ASSERT(state.fonts.size() < 1 << Implementation::FontHandleIdBits,
        "Whee::TextLayer::Shared::addFont(): can only have at most" << (1 << Implementation::FontHandleIdBits) << "fonts", {});
    /** @todo assert that the font is opened? doesn't prevent anybody from
        closing it, tho */

    arrayAppend(state.fonts, InPlaceInit, nullptr, &font, nullptr, size/font.size(), *glyphCacheFontId);
    return fontHandle(state.fonts.size() - 1, 1);
}

FontHandle TextLayer::Shared::addFont(Containers::Pointer<Text::AbstractFont>&& font, const Float size) {
    CORRADE_ASSERT(font,
        "Whee::TextLayer::Shared::addFont(): font is null", {});
    const FontHandle handle = addFont(*font, size);
    static_cast<State&>(*_state).fonts.back().fontStorage = Utility::move(font);
    return handle;
}

FontHandle TextLayer::Shared::addInstancelessFont(const UnsignedInt glyphCacheFontId, const Float scale) {
    State& state = static_cast<State&>(*_state);
    CORRADE_ASSERT(state.glyphCache,
        "Whee::TextLayer::Shared::addInstancelessFont(): no glyph cache set", {});
    CORRADE_ASSERT(glyphCacheFontId < state.glyphCache->fontCount(),
        "Whee::TextLayer::Shared::addInstancelessFont(): index" << glyphCacheFontId << "out of range for" << state.glyphCache->fontCount() << "fonts in set glyph cache", {});
    CORRADE_ASSERT(!state.glyphCache->fontPointer(glyphCacheFontId),
        "Whee::TextLayer::Shared::addInstancelessFont(): glyph cache font" << glyphCacheFontId << "has an instance set", {});
    CORRADE_ASSERT(state.fonts.size() < 1 << Implementation::FontHandleIdBits,
        "Whee::TextLayer::Shared::addInstancelessFont(): can only have at most" << (1 << Implementation::FontHandleIdBits) << "fonts", {});

    arrayAppend(state.fonts, InPlaceInit, nullptr, nullptr, nullptr, scale, glyphCacheFontId);
    return fontHandle(state.fonts.size() - 1, 1);
}

UnsignedInt TextLayer::Shared::glyphCacheFontId(const FontHandle handle) const {
    const State& state = static_cast<const State&>(*_state);
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::TextLayer::Shared::glyphCacheFontId(): invalid handle" << handle, {});
    return state.fonts[fontHandleId(handle)].glyphCacheFontId;
}

bool TextLayer::Shared::hasFontInstance(const FontHandle handle) const {
    const State& state = static_cast<const State&>(*_state);
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::TextLayer::Shared::hasFontInstance(): invalid handle" << handle, {});
    return state.fonts[fontHandleId(handle)].font;
}

const Text::AbstractFont& TextLayer::Shared::font(const FontHandle handle) const {
    const State& state = static_cast<const State&>(*_state);
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::TextLayer::Shared::font(): invalid handle" << handle, *state.fonts[0].font);
    Text::AbstractFont* const font = state.fonts[fontHandleId(handle)].font;
    CORRADE_ASSERT(font,
        "Whee::TextLayer::Shared::font():" << handle << "is an instance-less font", *state.fonts[0].font);
    return *font;
}

Text::AbstractFont& TextLayer::Shared::font(const FontHandle handle) {
    return const_cast<Text::AbstractFont&>(const_cast<const TextLayer::Shared&>(*this).font(handle));
}

void TextLayer::Shared::setStyleInternal(const TextLayerCommonStyleUniform& commonUniform, const Containers::ArrayView<const TextLayerStyleUniform> uniforms, const Containers::StridedArrayView1D<const FontHandle>& styleFonts, const Containers::StridedArrayView1D<const Vector4>& stylePaddings) {
    State& state = static_cast<State&>(*_state);
    /* Allocation done before the asserts so if they fail in a graceful assert
       build, we don't hit another assert in Utility::copy(styleToUniform) in
       the setStyle() below */
    if(state.styles.isEmpty())
        state.styles = Containers::Array<Implementation::TextLayerStyle>{NoInit, state.styleCount};
    CORRADE_ASSERT(uniforms.size() == state.styleUniformCount,
        "Whee::TextLayer::Shared::setStyle(): expected" << state.styleUniformCount << "uniforms, got" << uniforms.size(), );
    CORRADE_ASSERT(styleFonts.size() == state.styleCount,
        "Whee::TextLayer::Shared::setStyle(): expected" << state.styleCount << "font handles, got" << styleFonts.size(), );
    CORRADE_ASSERT(stylePaddings.isEmpty() || stylePaddings.size() == state.styleCount,
        "Whee::TextLayer::Shared::setStyle(): expected either no or" << state.styleCount << "paddings, got" << stylePaddings.size(), );
    #ifndef CORRADE_NO_ASSERT
    for(std::size_t i = 0; i != styleFonts.size(); ++i)
        CORRADE_ASSERT(isHandleValid(styleFonts[i]),
            "Whee::TextLayer::Shared::setStyle(): invalid handle" << styleFonts[i] << "at index" << i, );
    #endif
    Utility::copy(styleFonts, stridedArrayView(state.styles).slice(&Implementation::TextLayerStyle::font));
    if(stylePaddings.isEmpty()) {
        /** @todo some Utility::fill() for this */
        for(Implementation::TextLayerStyle& style: state.styles)
            style.padding = {};
    } else {
        Utility::copy(stylePaddings, stridedArrayView(state.styles).slice(&Implementation::TextLayerStyle::padding));
    }
    doSetStyle(commonUniform, uniforms);
}

TextLayer::Shared& TextLayer::Shared::setStyle(const TextLayerCommonStyleUniform& commonUniform, const Containers::ArrayView<const TextLayerStyleUniform> uniforms, const Containers::StridedArrayView1D<const UnsignedInt>& styleToUniform, const Containers::StridedArrayView1D<const FontHandle>& styleFonts, const Containers::StridedArrayView1D<const Vector4>& stylePaddings) {
    State& state = static_cast<State&>(*_state);
    CORRADE_ASSERT(styleToUniform.size() == state.styleCount,
        "Whee::TextLayer::Shared::setStyle(): expected" << state.styleCount << "style uniform indices, got" << styleToUniform.size(), *this);
    setStyleInternal(commonUniform, uniforms, styleFonts, stylePaddings);
    Utility::copy(styleToUniform, stridedArrayView(state.styles).slice(&Implementation::TextLayerStyle::uniform));
    return *this;
}

TextLayer::Shared& TextLayer::Shared::setStyle(const TextLayerCommonStyleUniform& commonUniform, const std::initializer_list<TextLayerStyleUniform> uniforms, const std::initializer_list<UnsignedInt> styleToUniform, const std::initializer_list<FontHandle> styleFonts, const std::initializer_list<Vector4> stylePaddings) {
    return setStyle(commonUniform, Containers::arrayView(uniforms), Containers::stridedArrayView(styleToUniform), Containers::stridedArrayView(styleFonts), Containers::stridedArrayView(stylePaddings));
}

TextLayer::Shared& TextLayer::Shared::setStyle(const TextLayerCommonStyleUniform& commonUniform, const Containers::ArrayView<const TextLayerStyleUniform> uniforms, const Containers::StridedArrayView1D<const FontHandle>& fonts, const Containers::StridedArrayView1D<const Vector4>& paddings) {
    State& state = static_cast<State&>(*_state);
    CORRADE_ASSERT(state.styleUniformCount == state.styleCount,
        "Whee::TextLayer::Shared::setStyle(): there's" << state.styleUniformCount << "uniforms for" << state.styleCount << "styles, provide an explicit mapping", *this);
    setStyleInternal(commonUniform, uniforms, fonts, paddings);
    for(UnsignedInt i = 0; i != state.styleCount; ++i)
        state.styles[i].uniform = i;
    return *this;
}

TextLayer::Shared& TextLayer::Shared::setStyle(const TextLayerCommonStyleUniform& commonUniform, const std::initializer_list<TextLayerStyleUniform> uniforms, const std::initializer_list<FontHandle> fonts, const std::initializer_list<Vector4> paddings) {
    return setStyle(commonUniform, Containers::arrayView(uniforms), Containers::stridedArrayView(fonts), Containers::stridedArrayView(paddings));
}

TextLayer::TextLayer(const LayerHandle handle, Containers::Pointer<State>&& state): AbstractVisualLayer{handle, Utility::move(state)} {}

TextLayer::TextLayer(const LayerHandle handle, Shared& shared): TextLayer{handle, Containers::pointer<State>(static_cast<Shared::State&>(*shared._state))} {}

void TextLayer::shapeTextInternal(
    #ifndef CORRADE_NO_ASSERT
    const char* const messagePrefix,
    #endif
    const UnsignedInt id, const UnsignedInt style, const Containers::StringView text, const TextProperties& properties)
{
    State& state = static_cast<State&>(*_state);
    Shared::State& sharedState = static_cast<Shared::State&>(state.shared);

    /* Decide on a font */
    FontHandle font = properties.font();
    if(font == FontHandle::Null) {
        CORRADE_ASSERT(!sharedState.styles.isEmpty(),
            messagePrefix << "no style data was set and no custom font was supplied", );
        font = sharedState.styles[style].font;
    } else CORRADE_ASSERT(Whee::isHandleValid(sharedState.fonts, font),
        messagePrefix << "invalid handle" << font, );

    Implementation::TextLayerFont& fontState = sharedState.fonts[fontHandleId(font)];
    CORRADE_ASSERT(fontState.font,
        messagePrefix << font << "is an instance-less font", );

    /** @todo once the TextProperties combine multiple fonts, scripts etc, this
        all should probably get wrapped in some higher level API in Text
        directly (AbstractLayouter?), which cuts the text to parts depending
        on font, script etc. and then puts all shaped runs together again? */
    /* Get a shaper instance */
    if(!fontState.shaper)
        fontState.shaper = fontState.font->createShaper();
    Text::AbstractShaper& shaper = *fontState.shaper;

    /* Shape the text */
    shaper.setScript(properties.script());
    shaper.setLanguage(properties.language());
    shaper.setDirection(properties.shapeDirection());
    const UnsignedInt glyphCount = shaper.shape(text, properties.features());

    /* Add a new glyph run. Any previous run for this data was marked as unused
       in previous remove() or in setText() right before calling this
       function. */
    const UnsignedInt glyphRun = state.glyphRuns.size();
    const UnsignedInt glyphOffset = state.glyphData.size();
    const Containers::StridedArrayView1D<Implementation::TextLayerGlyphData> glyphData = arrayAppend(state.glyphData, NoInit, glyphCount);
    arrayAppend(state.glyphRuns, InPlaceInit, glyphOffset, glyphCount, id);

    /* Query glyph offsets and advances, abuse the glyphData fields for those;
       then convert those in-place to absolute glyph positions and align
       them */
    const Containers::StridedArrayView1D<Vector2> glyphOffsetsPositions = glyphData.slice(&Implementation::TextLayerGlyphData::position);
    const Containers::StridedArrayView1D<Vector2> glyphAdvances = Containers::arrayCast<Vector2>(glyphData.slice(&Implementation::TextLayerGlyphData::glyphId));
    shaper.glyphOffsetsAdvancesInto(glyphOffsetsPositions, glyphAdvances);
    {
        Vector2 cursor;
        const Range2D lineRectangle = Text::renderLineGlyphPositionsInto(
            *fontState.font,
            fontState.scale*fontState.font->size(),
            properties.layoutDirection(),
            glyphOffsetsPositions,
            glyphAdvances,
            cursor,
            glyphOffsetsPositions);
        const Range2D blockRectangle = Text::alignRenderedLine(
            lineRectangle,
            properties.layoutDirection(),
            properties.alignment(),
            glyphOffsetsPositions);
        /** @todo use the final rectangle for min size for UI layouter */
        Text::alignRenderedBlock(
            blockRectangle,
            properties.layoutDirection(),
            properties.alignment(),
            glyphOffsetsPositions);
    }

    /* Glyph cache. The create() (or createGlyph()) should have ensured that a
       glyph cache is set, thus the subsequent setText() don't need to check again. */
    const Text::AbstractGlyphCache* const glyphCache = sharedState.glyphCache;
    CORRADE_INTERNAL_ASSERT(glyphCache);

    /* Query font-specific glyph IDs and convert them to cache-global */
    shaper.glyphIdsInto(glyphData.slice(&Implementation::TextLayerGlyphData::glyphId));
    {
        for(Implementation::TextLayerGlyphData& glyph: glyphData)
            glyph.glyphId = glyphCache->glyphId(fontState.glyphCacheFontId, glyph.glyphId);
    }

    /* Save scale, alignment and the glyph run reference */
    Implementation::TextLayerData& data = state.data[id];
    data.scale = fontState.scale;
    data.alignment = properties.alignment();
    data.glyphRun = glyphRun;
}

void TextLayer::shapeGlyphInternal(
    #ifndef CORRADE_NO_ASSERT
    const char* const messagePrefix,
    #endif
    const UnsignedInt id, const UnsignedInt style, const UnsignedInt glyphId, const TextProperties& properties)
{
    State& state = static_cast<State&>(*_state);
    Shared::State& sharedState = static_cast<Shared::State&>(state.shared);

    /* Decide on a font */
    FontHandle font = properties.font();
    if(font == FontHandle::Null) {
        CORRADE_ASSERT(!sharedState.styles.isEmpty(),
            messagePrefix << "no style data was set and no custom font was supplied", );
        font = sharedState.styles[style].font;
    } else CORRADE_ASSERT(Whee::isHandleValid(sharedState.fonts, font),
        messagePrefix << "invalid handle" << font, );

    /* The createGlyph() (or create()) should have ensured that a glyph cache
       is set, thus the subsequent setGlyph() doesn't need to check again. */
    const Implementation::TextLayerFont& fontState = sharedState.fonts[fontHandleId(font)];
    const Text::AbstractGlyphCache* const glyphCache = sharedState.glyphCache;
    CORRADE_INTERNAL_ASSERT(glyphCache);

    CORRADE_ASSERT(glyphId < glyphCache->fontGlyphCount(fontState.glyphCacheFontId),
        messagePrefix << "glyph" << glyphId << "out of range for" << glyphCache->fontGlyphCount(fontState.glyphCacheFontId) << "glyphs in glyph cache font" << fontState.glyphCacheFontId, );

    /* Query the glyph rectangle in order to align it. Compared to a regular
       text run, where the glyphs might not be present in the glyph cache yet
       (and can thus be filled in on-demand), here we require those to be
       present upfront. */
    const UnsignedInt cacheGlobalGlyphId = glyphCache->glyphId(fontState.glyphCacheFontId, glyphId);
    const Containers::Triple<Vector2i, Int, Range2Di> glyph = sharedState.glyphCache->glyph(cacheGlobalGlyphId);
    const Range2D glyphRectangle = Range2D{Range2Di::fromSize(glyph.first(), glyph.third().size())}
        .scaled(fontState.scale);

    /* Query glyph offsets and advances, abuse the glyphData fields for those;
       then convert those in-place to absolute glyph positions and align
       them */
    Vector2 glyphPosition[1]{};
    {
        const Range2D blockRectangle = Text::alignRenderedLine(
            glyphRectangle,
            /** @todo could the direction be abused for anything cool? */
            Text::LayoutDirection::HorizontalTopToBottom,
            properties.alignment(),
            glyphPosition);
        Text::alignRenderedBlock(
            blockRectangle,
            /** @todo could the direction be abused for anything cool? */
            Text::LayoutDirection::HorizontalTopToBottom,
            properties.alignment(),
            glyphPosition);
    }

    /* Add a new run containing just that one glyph. Any previous run for this
       data was marked as unused in previous remove(), or in setGlyph() right
       before calling this function. */
    const UnsignedInt glyphRun = state.glyphRuns.size();
    const UnsignedInt glyphOffset = state.glyphData.size();
    arrayAppend(state.glyphData, InPlaceInit, *glyphPosition, cacheGlobalGlyphId);
    arrayAppend(state.glyphRuns, InPlaceInit, glyphOffset, 1u, id);

    /* Save scale, alignment and the glyph run reference */
    Implementation::TextLayerData& data = state.data[id];
    data.scale = fontState.scale;
    data.alignment = properties.alignment();
    data.glyphRun = glyphRun;
}

DataHandle TextLayer::createInternal(const NodeHandle node) {
    State& state = static_cast<State&>(*_state);

    const DataHandle handle = AbstractLayer::create(node);
    const UnsignedInt id = dataHandleId(handle);
    if(id >= state.data.size()) {
        arrayAppend(state.data, NoInit, id - state.data.size() + 1);
        state.styles = stridedArrayView(state.data).slice(&Implementation::TextLayerData::style);
        state.calculatedStyles = stridedArrayView(state.data).slice(&Implementation::TextLayerData::calculatedStyle);
    }
    return handle;
}

DataHandle TextLayer::create(const UnsignedInt style, const Containers::StringView text, const TextProperties& properties, const Color3& color, const NodeHandle node) {
    State& state = static_cast<State&>(*_state);
    #ifndef CORRADE_NO_ASSERT
    Shared::State& sharedState = static_cast<Shared::State&>(state.shared);
    #endif
    CORRADE_ASSERT(sharedState.glyphCache,
        "Whee::TextLayer::create(): no glyph cache was set", {});
    CORRADE_ASSERT(style < sharedState.styleCount,
        "Whee::TextLayer::create(): style" << style << "out of range for" << sharedState.styleCount << "styles", {});

    /* Create a data */
    const DataHandle handle = createInternal(node);
    const UnsignedInt id = dataHandleId(handle);

    /* Shape the text, save its properties */
    shapeTextInternal(
        #ifndef CORRADE_NO_ASSERT
        "Whee::TextLayer::create():",
        #endif
        id, style, text, properties);
    Implementation::TextLayerData& data = state.data[id];
    /** @todo is there a way to have create() with all possible per-data
        options that doesn't make it ambiguous / impossible to extend further?
        like, having both color and padding optional is ambiguous, etc. */
    data.padding = {};
    /* glyphRun is filled by shapeInternal() */
    data.style = style;
    /* calculatedStyle is filled by AbstractVisualLayer::doUpdate() */
    data.color = color;

    return handle;
}

DataHandle TextLayer::createGlyph(const UnsignedInt style, const UnsignedInt glyph, const TextProperties& properties, const Color3& color, const NodeHandle node) {
    State& state = static_cast<State&>(*_state);
    #ifndef CORRADE_NO_ASSERT
    Shared::State& sharedState = static_cast<Shared::State&>(state.shared);
    #endif
    CORRADE_ASSERT(sharedState.glyphCache,
        "Whee::TextLayer::createGlyph(): no glyph cache was set", {});
    CORRADE_ASSERT(style < sharedState.styleCount,
        "Whee::TextLayer::createGlyph(): style" << style << "out of range for" << sharedState.styleCount << "styles", {});

    /* Create a data */
    const DataHandle handle = createInternal(node);
    const UnsignedInt id = dataHandleId(handle);

    /* Shape the glyph, save its properties */
    shapeGlyphInternal(
        #ifndef CORRADE_NO_ASSERT
        "Whee::TextLayer::createGlyph():",
        #endif
        id, style, glyph, properties);
    Implementation::TextLayerData& data = state.data[id];
    /** @todo is there a way to have createGlyph() with all possible per-data
        options that doesn't make it ambiguous / impossible to extend further?
        like, having both color and padding optional is ambiguous, etc. */
    data.padding = {};
    /* glyphRun is filled by shapeGlyphInternal() */
    data.style = style;
    /* calculatedStyle is filled by AbstractVisualLayer::doUpdate() */
    data.color = color;

    return handle;
}

void TextLayer::remove(const DataHandle handle) {
    AbstractVisualLayer::remove(handle);
    removeInternal(dataHandleId(handle));
}

void TextLayer::remove(const LayerDataHandle handle) {
    AbstractVisualLayer::remove(handle);
    removeInternal(layerDataHandleId(handle));
}

void TextLayer::removeInternal(const UnsignedInt id) {
    State& state = static_cast<State&>(*_state);

    /* Mark the glyph run as unused. It'll be removed during the next
       recompaction in doUpdate(). */
    state.glyphRuns[state.data[id].glyphRun].glyphOffset = ~UnsignedInt{};

    /* Data removal doesn't need anything to be reuploaded to continue working
       correctly, thus setNeedsUpdate() isn't called.

       Which might mean that doing a lot of remove() and then a lot of create()
       with no update() automatically triggered in between can cause high peak
       memory use. However that would happen even if update() was automatically
       scheduled but not actually called between the remove() and create(),
       such as when both happen in the same frame. So calling setNeedsUpdate()
       wouldn't really fully solve that peak memory problem anyway, and on the
       other hand choosing to trigger update() manually after a lot of removals
       can achieve lower peak use than any automagic. */
}

void TextLayer::setText(const DataHandle handle, const Containers::StringView text, const TextProperties& properties) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::TextLayer::setText(): invalid handle" << handle, );
    setTextInternal(dataHandleId(handle), text, properties);
}

void TextLayer::setText(const LayerDataHandle handle, const Containers::StringView text, const TextProperties& properties) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::TextLayer::setText(): invalid handle" << handle, );
    setTextInternal(layerDataHandleId(handle), text, properties);
}

void TextLayer::setTextInternal(const UnsignedInt id, const Containers::StringView text, const TextProperties& properties) {
    State& state = static_cast<State&>(*_state);
    Implementation::TextLayerData& data = state.data[id];

    /* Mark the original glyph run as unused. It'll be removed during the next
       recompaction in doUpdate(). */
    state.glyphRuns[data.glyphRun].glyphOffset = ~UnsignedInt{};

    /* Shape the text, mark the layer as needing an update */
    shapeTextInternal(
        #ifndef CORRADE_NO_ASSERT
        "Whee::TextLayer::setText():",
        #endif
        id, data.style, text, properties);
    setNeedsUpdate();
}

void TextLayer::setGlyph(const DataHandle handle, const UnsignedInt glyph, const TextProperties& properties) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::TextLayer::setGlyph(): invalid handle" << handle, );
    setGlyphInternal(dataHandleId(handle), glyph, properties);
}

void TextLayer::setGlyph(const LayerDataHandle handle, const UnsignedInt glyph, const TextProperties& properties) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::TextLayer::setGlyph(): invalid handle" << handle, );
    setGlyphInternal(layerDataHandleId(handle), glyph, properties);
}

void TextLayer::setGlyphInternal(const UnsignedInt id, const UnsignedInt glyph, const TextProperties& properties) {
    State& state = static_cast<State&>(*_state);
    Implementation::TextLayerData& data = state.data[id];

    /* Mark the original glyph run as unused. It'll be removed during the next
       recompaction in doUpdate(). We could also just reuse the offset in case
       the original run was 1 glyph or more (and setTextInternal() could do
       that too), but this way makes the often-updated data clustered to the
       end, allowing potential savings in data upload. */
    state.glyphRuns[data.glyphRun].glyphOffset = ~UnsignedInt{};

    /* Shape the glyph, mark the layer as needing an update */
    shapeGlyphInternal(
        #ifndef CORRADE_NO_ASSERT
        "Whee::TextLayer::setGlyph():",
        #endif
        id, data.style, glyph, properties);
    setNeedsUpdate();
}

Color3 TextLayer::color(const DataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::TextLayer::color(): invalid handle" << handle, {});
    return static_cast<const State&>(*_state).data[dataHandleId(handle)].color;
}

Color3 TextLayer::color(const LayerDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::TextLayer::color(): invalid handle" << handle, {});
    return static_cast<const State&>(*_state).data[layerDataHandleId(handle)].color;
}

void TextLayer::setColor(const DataHandle handle, const Color3& color) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::TextLayer::setColor(): invalid handle" << handle, );
    setColorInternal(dataHandleId(handle), color);
}

void TextLayer::setColor(const LayerDataHandle handle, const Color3& color) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::TextLayer::setColor(): invalid handle" << handle, );
    setColorInternal(layerDataHandleId(handle), color);
}

void TextLayer::setColorInternal(const UnsignedInt id, const Color3& color) {
    static_cast<State&>(*_state).data[id].color = color;
    setNeedsUpdate();
}

Vector4 TextLayer::padding(const DataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::TextLayer::padding(): invalid handle" << handle, {});
    return static_cast<const State&>(*_state).data[dataHandleId(handle)].padding;
}

Vector4 TextLayer::padding(const LayerDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::TextLayer::padding(): invalid handle" << handle, {});
    return static_cast<const State&>(*_state).data[layerDataHandleId(handle)].padding;
}

void TextLayer::setPadding(const DataHandle handle, const Vector4& padding) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::TextLayer::setPadding(): invalid handle" << handle, );
    setPaddingInternal(dataHandleId(handle), padding);
}

void TextLayer::setPadding(const LayerDataHandle handle, const Vector4& padding) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::TextLayer::setPadding(): invalid handle" << handle, );
    setPaddingInternal(layerDataHandleId(handle), padding);
}

void TextLayer::setPaddingInternal(const UnsignedInt id, const Vector4& padding) {
    static_cast<State&>(*_state).data[id].padding = padding;
    setNeedsUpdate();
}

LayerFeatures TextLayer::doFeatures() const {
    return AbstractVisualLayer::doFeatures()|LayerFeature::Draw;
}

void TextLayer::doClean(const Containers::BitArrayView dataIdsToRemove) {
    State& state = static_cast<State&>(*_state);

    /* Mark glyph runs attached to removed data as unused. They'll get removed
       during the next recompaction in doUpdate(). */
    /** @todo some way to iterate set bits */
    for(std::size_t i = 0; i != dataIdsToRemove.size(); ++i)
        if(dataIdsToRemove[i])
            state.glyphRuns[state.data[i].glyphRun].glyphOffset = ~UnsignedInt{};

    /* Data removal doesn't need anything to be reuploaded to continue working
       correctly, thus setNeedsUpdate() isn't called, and neither is in
       remove(). See a comment there for more information. */
}

void TextLayer::doUpdate(const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, const Containers::BitArrayView nodesEnabled, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes) {
    /* The base implementation populates data.calculatedStyle */
    AbstractVisualLayer::doUpdate(dataIds, clipRectIds, clipRectDataCounts, nodeOffsets, nodeSizes, nodesEnabled, clipRectOffsets, clipRectSizes);

    State& state = static_cast<State&>(*_state);
    Shared::State& sharedState = static_cast<Shared::State&>(state.shared);
    /* Technically needed only if there's any actual data to update, but
       require it always for consistency (and easier testing) */
    CORRADE_ASSERT(!sharedState.styles.isEmpty(),
        "Whee::TextLayer::update(): no style data was set", );

    /* Recompact the glyph data by removing unused runs */
    {
        std::size_t outputGlyphDataOffset = 0;
        std::size_t outputGlyphRunOffset = 0;
        for(std::size_t i = 0; i != state.glyphRuns.size(); ++i) {
            Implementation::TextLayerGlyphRun& run = state.glyphRuns[i];
            if(run.glyphOffset == ~UnsignedInt{})
                continue;

            /* Move the glyph data earlier if there were skipped runs before,
               update the reference to it in the run */
            if(run.glyphOffset != outputGlyphDataOffset) {
                CORRADE_INTERNAL_DEBUG_ASSERT(run.glyphOffset > outputGlyphDataOffset);
                CORRADE_INTERNAL_DEBUG_ASSERT(i != outputGlyphRunOffset);

                std::memmove(state.glyphData.data() + outputGlyphDataOffset,
                             state.glyphData.data() + run.glyphOffset,
                             run.glyphCount*sizeof(Implementation::TextLayerGlyphData));
                run.glyphOffset = outputGlyphDataOffset;
            }
            outputGlyphDataOffset += run.glyphCount;

            /* Move the glyph run info earlier if there were skipped runs
               before, update the reference to it in the data */
            if(i != outputGlyphRunOffset) {
                CORRADE_INTERNAL_DEBUG_ASSERT(i > outputGlyphRunOffset);
                state.data[run.data].glyphRun = outputGlyphRunOffset;
                state.glyphRuns[outputGlyphRunOffset] = run;
            }
            ++outputGlyphRunOffset;
        }

        /* Remove the now-unused data from the end */
        CORRADE_INTERNAL_ASSERT(outputGlyphDataOffset <= state.glyphData.size());
        CORRADE_INTERNAL_ASSERT(outputGlyphRunOffset <= state.glyphRuns.size());
        arrayResize(state.glyphData, outputGlyphDataOffset);
        arrayResize(state.glyphRuns, outputGlyphRunOffset);
    }

    /* Index offsets for each run, plus one more for the last run */
    arrayResize(state.indexDrawOffsets, NoInit, dataIds.size() + 1);

    /* Calculate how many glyphs there are in total and how many we'll draw */
    UnsignedInt totalGlyphCount = 0;
    UnsignedInt drawGlyphCount = 0;
    for(const Implementation::TextLayerGlyphRun& run: state.glyphRuns)
        totalGlyphCount += run.glyphCount;
    for(const UnsignedInt id: dataIds)
        drawGlyphCount += state.glyphRuns[state.data[id].glyphRun].glyphCount;

    const Containers::StridedArrayView1D<const Whee::NodeHandle> nodes = this->nodes();

    /* Generate vertex and index data */
    arrayResize(state.vertices, NoInit, totalGlyphCount*4);
    arrayResize(state.indices, NoInit, drawGlyphCount*6);
    UnsignedInt indexOffset = 0;
    for(std::size_t i = 0; i != dataIds.size(); ++i) {
        const UnsignedInt dataId = dataIds[i];
        const UnsignedInt nodeId = nodeHandleId(nodes[dataId]);
        const Implementation::TextLayerData& data = state.data[dataId];
        const Implementation::TextLayerGlyphRun& glyphRun = state.glyphRuns[data.glyphRun];

        /* Fill in quad vertices in the same order as the original text runs */
        /** @todo ideally this would only be done if some text actually
            changes, not on every visibility change */
        const Containers::StridedArrayView1D<const Implementation::TextLayerGlyphData> glyphData = state.glyphData.sliceSize(glyphRun.glyphOffset, glyphRun.glyphCount);
        const Containers::StridedArrayView1D<Implementation::TextLayerVertex> vertexData = state.vertices.sliceSize(glyphRun.glyphOffset*4, glyphRun.glyphCount*4);
        Text::renderGlyphQuadsInto(
            *sharedState.glyphCache,
            data.scale,
            glyphData.slice(&Implementation::TextLayerGlyphData::position),
            glyphData.slice(&Implementation::TextLayerGlyphData::glyphId),
            vertexData.slice(&Implementation::TextLayerVertex::position),
            vertexData.slice(&Implementation::TextLayerVertex::textureCoordinates));

        /* Align the glyph run relative to the node area */
        const Vector4 padding = sharedState.styles[data.calculatedStyle].padding + data.padding;
        Vector2 offset = nodeOffsets[nodeId] + padding.xy();
        const Vector2 size = nodeSizes[nodeId] - padding.xy() - Math::gather<'z', 'w'>(padding);
        const UnsignedByte alignmentHorizontal = (UnsignedByte(data.alignment) & Text::Implementation::AlignmentHorizontal);
        if(alignmentHorizontal == Text::Implementation::AlignmentLeft) {
            offset.x() += 0.0f;
        } else if(alignmentHorizontal == Text::Implementation::AlignmentRight) {
            offset.x() += size.x();
        } else if(alignmentHorizontal == Text::Implementation::AlignmentCenter) {
            if(UnsignedByte(data.alignment) & Text::Implementation::AlignmentIntegral)
                offset.x() += Math::round(size.x()*0.5f);
            else
                offset.x() += size.x()*0.5f;
        }
        const UnsignedByte alignmentVertical = (UnsignedByte(data.alignment) & Text::Implementation::AlignmentVertical);
        /* For Line/Middle it's aligning either the line or bounding box middle
           (which is already at y=0 by the Text::alignRenderedLine()) to node
           middle */
        if(alignmentVertical == Text::Implementation::AlignmentTop) {
            offset.y() += 0.0f;
        } else if(alignmentVertical == Text::Implementation::AlignmentBottom) {
            offset.y() += size.y();
        } else if(alignmentVertical == Text::Implementation::AlignmentLine ||
                  alignmentVertical == Text::Implementation::AlignmentMiddle) {
            if(UnsignedByte(data.alignment) & Text::Implementation::AlignmentIntegral)
                offset.y() += Math::round(size.y()*0.5f);
            else
                offset.y() += size.y()*0.5f;
        }

        /* Translate the (aligned) glyph run, fill color and style */
        for(Implementation::TextLayerVertex& vertex: vertexData) {
            vertex.position = vertex.position*Vector2::yScale(-1.0f) + offset;
            vertex.color = data.color;
            vertex.styleUniform = sharedState.styles[data.calculatedStyle].uniform;
        }

        /* Generate indices in draw order. Remeber the offset for each data to
           draw from later. */
        state.indexDrawOffsets[i] = indexOffset;
        const Containers::ArrayView<UnsignedInt> indexData = state.indices.sliceSize(indexOffset, glyphRun.glyphCount*6);
        Text::renderGlyphQuadIndicesInto(glyphRun.glyphOffset, indexData);
        indexOffset += indexData.size();
    }

    CORRADE_INTERNAL_ASSERT(indexOffset == drawGlyphCount*6);
    state.indexDrawOffsets[dataIds.size()] = indexOffset;
}

}}
