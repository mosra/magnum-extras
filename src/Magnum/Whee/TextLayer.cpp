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
#include <Corrade/Utility/Algorithms.h>
#include <Magnum/Math/Functions.h>
#include <Magnum/Math/Matrix3.h>
#include <Magnum/Math/Swizzle.h>
#include <Magnum/Text/AbstractGlyphCache.h>
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

TextLayer::Shared::Shared(const UnsignedInt styleCount): Shared{Containers::pointer<State>(styleCount)} {}

TextLayer::Shared::Shared(NoCreateT) noexcept: AbstractVisualLayer::Shared{NoCreate} {}

TextLayer::Shared& TextLayer::Shared::setGlyphCache(Text::AbstractGlyphCache& cache) {
    State& state = static_cast<State&>(*_state);
    CORRADE_ASSERT(!state.glyphCache,
        "Whee::TextLayer::Shared::setGlyphCache(): glyph cache already set", *this);
    state.glyphCache = &cache;
    return *this;
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
    /* TextLayer::setText(), which doesn't have access to the outer State
       class, needs this too */
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
    CORRADE_ASSERT(state.glyphCache->findFont(font),
        "Whee::TextLayer::Shared::addFont(): font not found among" << state.glyphCache->fontCount() << "fonts in set glyph cache", {});
    CORRADE_ASSERT(state.fonts.size() < 1 << Implementation::FontHandleIdBits,
        "Whee::TextLayer::Shared::addFont(): can only have at most" << (1 << Implementation::FontHandleIdBits) << "fonts", {});
    /** @todo assert that the font is opened? doesn't prevent anybody from
        closing it, tho */

    arrayAppend(state.fonts, InPlaceInit, nullptr, font, nullptr, size);
    return fontHandle(state.fonts.size() - 1, 1);
}

FontHandle TextLayer::Shared::addFont(Containers::Pointer<Text::AbstractFont>&& font, const Float size) {
    CORRADE_ASSERT(font,
        "Whee::TextLayer::Shared::addFont(): font is null", {});
    const FontHandle handle = addFont(*font, size);
    static_cast<State&>(*_state).fonts.back().fontStorage = Utility::move(font);
    return handle;
}

const Text::AbstractFont& TextLayer::Shared::font(const FontHandle handle) const {
    const State& state = static_cast<const State&>(*_state);
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::TextLayer::Shared::font(): invalid handle" << handle, state.fonts[0].font);
    return state.fonts[fontHandleId(handle)].font;
}

Text::AbstractFont& TextLayer::Shared::font(const FontHandle handle) {
    return const_cast<Text::AbstractFont&>(const_cast<const TextLayer::Shared&>(*this).font(handle));
}

TextLayer::Shared& TextLayer::Shared::setStyle(const TextLayerStyleCommon& common, const Containers::ArrayView<const TextLayerStyleItem> items, const Containers::StridedArrayView1D<const FontHandle>& itemFonts, const Containers::StridedArrayView1D<const Vector4>& itemPadding) {
    State& state = static_cast<State&>(*_state);
    CORRADE_ASSERT(items.size() == state.styleCount,
        "Whee::TextLayer::Shared::setStyle(): expected" << state.styleCount << "style items, got" << items.size(), *this);
    CORRADE_ASSERT(itemFonts.size() == state.styleCount,
        "Whee::TextLayer::Shared::setStyle(): expected" << state.styleCount << "font handles, got" << itemFonts.size(), *this);
    CORRADE_ASSERT(itemPadding.isEmpty() || itemPadding.size() == state.styleCount,
        "Whee::TextLayer::Shared::setStyle(): expected either no or" << state.styleCount << "paddings, got" << itemPadding.size(), *this);
    #ifndef CORRADE_NO_ASSERT
    for(std::size_t i = 0; i != itemFonts.size(); ++i)
        CORRADE_ASSERT(isHandleValid(itemFonts[i]),
            "Whee::TextLayer::Shared::setStyle(): invalid handle" << itemFonts[i] << "at index" << i, *this);
    #endif
    Utility::copy(itemFonts, stridedArrayView(state.styles).slice(&Implementation::TextLayerStyle::font));
    if(itemPadding.isEmpty()) {
        /** @todo some Utility::fill() for this */
        for(Implementation::TextLayerStyle& style: state.styles)
            style.padding = {};
    } else {
        Utility::copy(itemPadding, stridedArrayView(state.styles).slice(&Implementation::TextLayerStyle::padding));
    }
    doSetStyle(common, items);
    return *this;
}

TextLayer::Shared& TextLayer::Shared::setStyle(const TextLayerStyleCommon& common, const std::initializer_list<TextLayerStyleItem> items, const std::initializer_list<FontHandle> itemFonts, const std::initializer_list<Vector4> itemPadding) {
    return setStyle(common, Containers::arrayView(items), Containers::stridedArrayView(itemFonts), Containers::stridedArrayView(itemPadding));
}

TextLayer::TextLayer(const LayerHandle handle, Containers::Pointer<State>&& state): AbstractVisualLayer{handle, Utility::move(state)} {}

TextLayer::TextLayer(const LayerHandle handle, Shared& shared): TextLayer{handle, Containers::pointer<State>(static_cast<Shared::State&>(*shared._state))} {}

void TextLayer::shapeInternal(
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
        /* This assumes that fonts can never be deleted, otherwise it'd have to
           call isHandleValid() on the Shared class somehow. setStyle() checks
           that all handles are non-null (and valid), so if a handle is null it
           means setStyle() wasn't called yet. */
        font = sharedState.styles[style].font;
        CORRADE_ASSERT(font != FontHandle::Null,
            messagePrefix << "no style data was set and no custom font was supplied", );
    } else CORRADE_ASSERT(Whee::isHandleValid(sharedState.fonts, font),
        messagePrefix << "invalid handle" << font, );

    /** @todo once the TextProperties combine multiple fonts, scripts etc, this
        all should probably get wrapped in some higher level API in Text
        directly (AbstractLayouter?), which cuts the text to parts depending
        on font, script etc. and then puts all shaped runs together again? */
    /* Get a shaper instance */
    Implementation::TextLayerFont& fontState = sharedState.fonts[fontHandleId(font)];
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
            fontState.font,
            fontState.size,
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

    /* Query font-specific glyph IDs and convert them to cache-global */
    shaper.glyphIdsInto(glyphData.slice(&Implementation::TextLayerGlyphData::glyphId));
    {
        /* Shared::addFont() already ensured that the font is contained in the
           cache, so this should never fail */
        const Containers::Optional<UnsignedInt> fontId = sharedState.glyphCache->findFont(*fontState.font);
        CORRADE_INTERNAL_ASSERT(fontId);
        for(Implementation::TextLayerGlyphData& glyph: glyphData)
            glyph.glyphId = sharedState.glyphCache->glyphId(*fontId, glyph.glyphId);
    }

    /* Save font, alignment and the glyph run reference */
    Implementation::TextLayerData& data = state.data[id];
    data.font = font;
    data.alignment = properties.alignment();
    data.glyphRun = glyphRun;
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
    const DataHandle handle = AbstractLayer::create(node);
    const UnsignedInt id = dataHandleId(handle);
    if(id >= state.data.size()) {
        arrayAppend(state.data, NoInit, id - state.data.size() + 1);
        state.styles = stridedArrayView(state.data).slice(&Implementation::TextLayerData::style);
    }

    /* Shape the text, save its properties */
    shapeInternal(
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
    shapeInternal(
        #ifndef CORRADE_NO_ASSERT
        "Whee::TextLayer::setText():",
        #endif
        id, data.style, text, properties);
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

void TextLayer::doUpdate(const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&) {
    State& state = static_cast<State&>(*_state);
    Shared::State& sharedState = static_cast<Shared::State&>(state.shared);

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
        const Implementation::TextLayerFont& font = sharedState.fonts[fontHandleId(data.font)];
        const Implementation::TextLayerGlyphRun& glyphRun = state.glyphRuns[data.glyphRun];

        /* Fill in quad vertices in the same order as the original text runs */
        /** @todo ideally this would only be done if some text actually
            changes, not on every visibility change */
        const Containers::StridedArrayView1D<const Implementation::TextLayerGlyphData> glyphData = state.glyphData.sliceSize(glyphRun.glyphOffset, glyphRun.glyphCount);
        const Containers::StridedArrayView1D<Implementation::TextLayerVertex> vertexData = state.vertices.sliceSize(glyphRun.glyphOffset*4, glyphRun.glyphCount*4);
        Text::renderGlyphQuadsInto(
            *sharedState.glyphCache,
            font.size/font.font->size(),
            glyphData.slice(&Implementation::TextLayerGlyphData::position),
            glyphData.slice(&Implementation::TextLayerGlyphData::glyphId),
            vertexData.slice(&Implementation::TextLayerVertex::position),
            vertexData.slice(&Implementation::TextLayerVertex::textureCoordinates));

        /* Align the glyph run relative to the node area */
        const Vector4 padding = sharedState.styles[data.style].padding + data.padding;
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
            vertex.style = data.style;
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
