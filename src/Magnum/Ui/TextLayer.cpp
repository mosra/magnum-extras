/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023, 2024
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

#include "TextLayer.h"

#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/BitArrayView.h>
#include <Corrade/Containers/EnumSet.hpp>
#include <Corrade/Containers/GrowableArray.h>
#include <Corrade/Containers/Iterable.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/String.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Containers/Triple.h>
#include <Corrade/Utility/Algorithms.h>
#include <Corrade/Utility/Unicode.h>
#include <Magnum/Math/Functions.h>
#include <Magnum/Math/Matrix3.h>
#include <Magnum/Math/Swizzle.h>
#include <Magnum/Math/Time.h>
#include <Magnum/Text/AbstractGlyphCache.h>
#include <Magnum/Text/Direction.h>
#include <Magnum/Text/Feature.h>
#include <Magnum/Text/Renderer.h>

#include "Magnum/Ui/TextLayerAnimator.h"
#include "Magnum/Ui/Event.h"
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/TextProperties.h"
#include "Magnum/Ui/Implementation/textLayerState.h"

namespace Magnum { namespace Ui {

Debug& operator<<(Debug& debug, const FontHandle value) {
    if(value == FontHandle::Null)
        return debug << "Ui::FontHandle::Null";
    return debug << "Ui::FontHandle(" << Debug::nospace << Debug::hex << fontHandleId(value) << Debug::nospace << "," << Debug::hex << fontHandleGeneration(value) << Debug::nospace << ")";
}

Debug& operator<<(Debug& debug, const TextDataFlag value) {
    debug << "Ui::TextDataFlag" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case TextDataFlag::value: return debug << "::" #value;
        _c(Editable)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "(" << Debug::nospace << Debug::hex << UnsignedByte(value) << Debug::nospace << ")";
}

Debug& operator<<(Debug& debug, const TextDataFlags value) {
    return Containers::enumSetDebugOutput(debug, value, "Ui::TextDataFlags{}", {
        TextDataFlag::Editable
    });
}

Debug& operator<<(Debug& debug, const TextEdit value) {
    debug << "Ui::TextEdit" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case TextEdit::value: return debug << "::" #value;
        _c(MoveCursorLeft)
        _c(ExtendSelectionLeft)
        _c(MoveCursorRight)
        _c(ExtendSelectionRight)
        _c(MoveCursorLineBegin)
        _c(ExtendSelectionLineBegin)
        _c(MoveCursorLineEnd)
        _c(ExtendSelectionLineEnd)
        _c(RemoveBeforeCursor)
        _c(RemoveAfterCursor)
        _c(InsertBeforeCursor)
        _c(InsertAfterCursor)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "(" << Debug::nospace << Debug::hex << UnsignedByte(value) << Debug::nospace << ")";
}

TextLayer::Shared::State::State(Shared& self, const Configuration& configuration): AbstractVisualLayer::Shared::State{self, configuration.styleCount(), configuration.dynamicStyleCount()}, hasEditingStyles{configuration.hasEditingStyles()}, styleUniformCount{configuration.styleUniformCount()}, editingStyleUniformCount{configuration.editingStyleUniformCount()} {
    styleStorage = Containers::ArrayTuple{
        {NoInit, configuration.styleCount(), styles},
        {NoInit, configuration.dynamicStyleCount() ? configuration.styleUniformCount() : 0, styleUniforms},
        {NoInit, configuration.editingStyleCount(), editingStyles},
        {NoInit, configuration.dynamicStyleCount() ? configuration.editingStyleUniformCount() : 0, editingStyleUniforms},
    };
}

TextLayer::Shared::Shared(Containers::Pointer<State>&& state): AbstractVisualLayer::Shared{Utility::move(state)} {
    #ifndef CORRADE_NO_ASSERT
    const State& s = static_cast<const State&>(*_state);
    #endif
    CORRADE_ASSERT(s.styleCount + s.dynamicStyleCount,
        "Ui::TextLayer::Shared: expected non-zero total style count", );
}

TextLayer::Shared::Shared(const Configuration& configuration): Shared{Containers::pointer<State>(*this, configuration)} {}

TextLayer::Shared::Shared(NoCreateT) noexcept: AbstractVisualLayer::Shared{NoCreate} {}

UnsignedInt TextLayer::Shared::styleUniformCount() const {
    return static_cast<const State&>(*_state).styleUniformCount;
}

UnsignedInt TextLayer::Shared::editingStyleUniformCount() const {
    return static_cast<const State&>(*_state).editingStyleUniformCount;
}

UnsignedInt TextLayer::Shared::editingStyleCount() const {
    return static_cast<const State&>(*_state).editingStyles.size();
}

bool TextLayer::Shared::hasEditingStyles() const {
    return static_cast<const State&>(*_state).hasEditingStyles;
}

TextLayer::Shared& TextLayer::Shared::setGlyphCache(Text::AbstractGlyphCache& cache) {
    State& state = static_cast<State&>(*_state);
    CORRADE_ASSERT(!state.glyphCache,
        "Ui::TextLayer::Shared::setGlyphCache(): glyph cache already set", *this);
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
        "Ui::TextLayer::Shared::glyphCache(): no glyph cache set", *state.glyphCache);
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
    return Ui::isHandleValid(static_cast<const State&>(*_state).fonts, handle);
}

FontHandle TextLayer::Shared::addFont(Text::AbstractFont& font, const Float size) {
    State& state = static_cast<State&>(*_state);
    CORRADE_ASSERT(state.glyphCache,
        "Ui::TextLayer::Shared::addFont(): no glyph cache set", {});
    const Containers::Optional<UnsignedInt> glyphCacheFontId = state.glyphCache->findFont(font);
    CORRADE_ASSERT(glyphCacheFontId,
        "Ui::TextLayer::Shared::addFont(): font not found among" << state.glyphCache->fontCount() << "fonts in set glyph cache", {});
    CORRADE_ASSERT(state.fonts.size() < 1 << Implementation::FontHandleIdBits,
        "Ui::TextLayer::Shared::addFont(): can only have at most" << (1 << Implementation::FontHandleIdBits) << "fonts", {});
    /** @todo assert that the font is opened? doesn't prevent anybody from
        closing it, tho */

    arrayAppend(state.fonts, InPlaceInit, nullptr, &font, nullptr, size/font.size(), *glyphCacheFontId);
    return fontHandle(state.fonts.size() - 1, 1);
}

FontHandle TextLayer::Shared::addFont(Containers::Pointer<Text::AbstractFont>&& font, const Float size) {
    CORRADE_ASSERT(font,
        "Ui::TextLayer::Shared::addFont(): font is null", {});
    const FontHandle handle = addFont(*font, size);
    static_cast<State&>(*_state).fonts.back().fontStorage = Utility::move(font);
    return handle;
}

FontHandle TextLayer::Shared::addInstancelessFont(const UnsignedInt glyphCacheFontId, const Float scale) {
    State& state = static_cast<State&>(*_state);
    CORRADE_ASSERT(state.glyphCache,
        "Ui::TextLayer::Shared::addInstancelessFont(): no glyph cache set", {});
    CORRADE_ASSERT(glyphCacheFontId < state.glyphCache->fontCount(),
        "Ui::TextLayer::Shared::addInstancelessFont(): index" << glyphCacheFontId << "out of range for" << state.glyphCache->fontCount() << "fonts in set glyph cache", {});
    CORRADE_ASSERT(!state.glyphCache->fontPointer(glyphCacheFontId),
        "Ui::TextLayer::Shared::addInstancelessFont(): glyph cache font" << glyphCacheFontId << "has an instance set", {});
    CORRADE_ASSERT(state.fonts.size() < 1 << Implementation::FontHandleIdBits,
        "Ui::TextLayer::Shared::addInstancelessFont(): can only have at most" << (1 << Implementation::FontHandleIdBits) << "fonts", {});

    arrayAppend(state.fonts, InPlaceInit, nullptr, nullptr, nullptr, scale, glyphCacheFontId);
    return fontHandle(state.fonts.size() - 1, 1);
}

UnsignedInt TextLayer::Shared::glyphCacheFontId(const FontHandle handle) const {
    const State& state = static_cast<const State&>(*_state);
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::TextLayer::Shared::glyphCacheFontId(): invalid handle" << handle, {});
    return state.fonts[fontHandleId(handle)].glyphCacheFontId;
}

bool TextLayer::Shared::hasFontInstance(const FontHandle handle) const {
    const State& state = static_cast<const State&>(*_state);
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::TextLayer::Shared::hasFontInstance(): invalid handle" << handle, {});
    return state.fonts[fontHandleId(handle)].font;
}

const Text::AbstractFont& TextLayer::Shared::font(const FontHandle handle) const {
    const State& state = static_cast<const State&>(*_state);
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::TextLayer::Shared::font(): invalid handle" << handle, *state.fonts[0].font);
    Text::AbstractFont* const font = state.fonts[fontHandleId(handle)].font;
    CORRADE_ASSERT(font,
        "Ui::TextLayer::Shared::font():" << handle << "is an instance-less font", *state.fonts[0].font);
    return *font;
}

Text::AbstractFont& TextLayer::Shared::font(const FontHandle handle) {
    return const_cast<Text::AbstractFont&>(const_cast<const TextLayer::Shared&>(*this).font(handle));
}

void TextLayer::Shared::setStyleInternal(const TextLayerCommonStyleUniform& commonUniform, const Containers::ArrayView<const TextLayerStyleUniform> uniforms, const Containers::StridedArrayView1D<const FontHandle>& styleFonts, const Containers::StridedArrayView1D<const Text::Alignment>& styleAlignments, const Containers::ArrayView<const TextFeatureValue> styleFeatures, const Containers::StridedArrayView1D<const UnsignedInt>& styleFeatureOffsets, const Containers::StridedArrayView1D<const UnsignedInt>& styleFeatureCounts, const Containers::StridedArrayView1D<const Int>& styleCursorStyles, const Containers::StridedArrayView1D<const Int>& styleSelectionStyles, const Containers::StridedArrayView1D<const Vector4>& stylePaddings) {
    State& state = static_cast<State&>(*_state);
    CORRADE_ASSERT(uniforms.size() == state.styleUniformCount,
        "Ui::TextLayer::Shared::setStyle(): expected" << state.styleUniformCount << "uniforms, got" << uniforms.size(), );
    CORRADE_ASSERT(styleFonts.size() == state.styleCount,
        "Ui::TextLayer::Shared::setStyle(): expected" << state.styleCount << "font handles, got" << styleFonts.size(), );
    CORRADE_ASSERT(styleAlignments.size() == state.styleCount,
        "Ui::TextLayer::Shared::setStyle(): expected" << state.styleCount << "alignment values, got" << styleAlignments.size(), );
    CORRADE_ASSERT(styleCursorStyles.isEmpty() || styleCursorStyles.size() == state.styleCount,
        "Ui::TextLayer::Shared::setStyle(): expected either no or" << state.styleCount << "cursor styles, got" << styleCursorStyles.size(), );
    CORRADE_ASSERT(styleSelectionStyles.isEmpty() || styleSelectionStyles.size() == state.styleCount,
        "Ui::TextLayer::Shared::setStyle(): expected either no or" << state.styleCount << "selection styles, got" << styleSelectionStyles.size(), );
    CORRADE_ASSERT(stylePaddings.isEmpty() || stylePaddings.size() == state.styleCount,
        "Ui::TextLayer::Shared::setStyle(): expected either no or" << state.styleCount << "paddings, got" << stylePaddings.size(), );
    #ifndef CORRADE_NO_ASSERT
    if(!styleFeatures.isEmpty() || !styleFeatureOffsets.isEmpty() || !styleFeatureCounts.isEmpty()) {
        CORRADE_ASSERT(styleFeatureOffsets.size() == state.styleCount,
            "Ui::TextLayer::Shared::setStyle(): expected" << state.styleCount << "feature offsets, got" << styleFeatureOffsets.size(), );
        CORRADE_ASSERT(styleFeatureCounts.size() == state.styleCount,
            "Ui::TextLayer::Shared::setStyle(): expected" << state.styleCount << "feature counts, got" << styleFeatureCounts.size(), );
    }
    for(std::size_t i = 0; i != styleFonts.size(); ++i)
        CORRADE_ASSERT(styleFonts[i] == FontHandle::Null || isHandleValid(styleFonts[i]),
            "Ui::TextLayer::Shared::setStyle(): invalid handle" << styleFonts[i] << "at index" << i, );
    for(std::size_t i = 0; i != styleAlignments.size(); ++i)
        CORRADE_ASSERT(!(UnsignedByte(styleAlignments[i]) & Text::Implementation::AlignmentGlyphBounds),
            "Ui::TextLayer::Shared::setStyle(): unsupported" << styleAlignments[i] << "at index" << i, );
    for(std::size_t i = 0; i != styleFeatureOffsets.size(); ++i)
        CORRADE_ASSERT(styleFeatureOffsets[i] + styleFeatureCounts[i] <= styleFeatures.size(),
            "Ui::TextLayer::Shared::setStyle(): feature offset" << styleFeatureOffsets[i] << "and count" << styleFeatureCounts[i] << "out of range for" << styleFeatures.size() << "features" << "at index" << i, );
    #endif
    Utility::copy(styleFonts, stridedArrayView(state.styles).slice(&Implementation::TextLayerStyle::font));
    Utility::copy(styleAlignments, stridedArrayView(state.styles).slice(&Implementation::TextLayerStyle::alignment));
    if(styleFeatureOffsets.isEmpty()) {
        arrayResize(state.styleFeatures, NoInit, 0);
        /** @todo some Utility::fill() for this */
        for(Implementation::TextLayerStyle& style: state.styles) {
            style.featureOffset = 0;
            style.featureCount = 0;
        }
    } else {
        /* Resizing the array to reuse the memory in case of subsequent style
           setting */
        arrayResize(state.styleFeatures, NoInit, styleFeatures.size());
        Utility::copy(styleFeatures, state.styleFeatures);
        Utility::copy(styleFeatureOffsets, stridedArrayView(state.styles).slice(&Implementation::TextLayerStyle::featureOffset));
        Utility::copy(styleFeatureCounts, stridedArrayView(state.styles).slice(&Implementation::TextLayerStyle::featureCount));
    }
    if(styleCursorStyles.isEmpty()) {
        /** @todo some Utility::fill() for this */
        for(Implementation::TextLayerStyle& style: state.styles)
            style.cursorStyle = -1;
    } else {
        #ifndef CORRADE_NO_ASSERT
        for(std::size_t i = 0; i != styleCursorStyles.size(); ++i)
            CORRADE_ASSERT(styleCursorStyles[i] == -1 || UnsignedInt(styleCursorStyles[i]) < state.editingStyles.size(),
                "Ui::TextLayer::Shared::setStyle(): cursor style" << styleCursorStyles[i] << "out of range for" << state.editingStyles.size() << "editing styles" << "at index" << i, );
        #endif
        Utility::copy(styleCursorStyles, stridedArrayView(state.styles).slice(&Implementation::TextLayerStyle::cursorStyle));
    }
    if(styleSelectionStyles.isEmpty()) {
        /** @todo some Utility::fill() for this */
        for(Implementation::TextLayerStyle& style: state.styles)
            style.selectionStyle = -1;
    } else {
        #ifndef CORRADE_NO_ASSERT
        for(std::size_t i = 0; i != styleSelectionStyles.size(); ++i)
            CORRADE_ASSERT(styleSelectionStyles[i] == -1 || UnsignedInt(styleSelectionStyles[i]) < state.editingStyles.size(),
                "Ui::TextLayer::Shared::setStyle(): selection style" << styleSelectionStyles[i] << "out of range for" << state.editingStyles.size() << "editing styles" << "at index" << i, );
        #endif
        Utility::copy(styleSelectionStyles, stridedArrayView(state.styles).slice(&Implementation::TextLayerStyle::selectionStyle));
    }
    if(stylePaddings.isEmpty()) {
        /** @todo some Utility::fill() for this */
        for(Implementation::TextLayerStyle& style: state.styles)
            style.padding = {};
    } else {
        Utility::copy(stylePaddings, stridedArrayView(state.styles).slice(&Implementation::TextLayerStyle::padding));
    }

    /* If there are dynamic styles, the layers will combine them with the
       static styles and upload to a single buffer, so just copy them to an
       array for the layers to reuse */
    if(state.dynamicStyleCount) {
        state.commonStyleUniform = commonUniform;
        Utility::copy(uniforms, state.styleUniforms);
    } else doSetStyle(commonUniform, uniforms);

    #ifndef CORRADE_NO_ASSERT
    /* Now it's safe to call create(), createGlyph() and update() */
    state.setStyleCalled = true;
    #endif

    /* Make doState() of all layers sharing this state return NeedsDataUpdate
       in order to update style-to-uniform mappings, paddings and such, and in
       case of dynamic styles also NeedsCommonDataUpdate to upload the changed
       per-layer uniform buffers. Setting it only if those differ would trigger
       update only if actually needed, but it may be prohibitively expensive
       compared to updating always. */
    ++state.styleUpdateStamp;
}

TextLayer::Shared& TextLayer::Shared::setStyle(const TextLayerCommonStyleUniform& commonUniform, const Containers::ArrayView<const TextLayerStyleUniform> uniforms, const Containers::StridedArrayView1D<const UnsignedInt>& styleToUniform, const Containers::StridedArrayView1D<const FontHandle>& styleFonts, const Containers::StridedArrayView1D<const Text::Alignment>& styleAlignments, const Containers::ArrayView<const TextFeatureValue> styleFeatures, const Containers::StridedArrayView1D<const UnsignedInt>& styleFeatureOffsets, const Containers::StridedArrayView1D<const UnsignedInt>& styleFeatureCounts, const Containers::StridedArrayView1D<const Int>& styleCursorStyles, const Containers::StridedArrayView1D<const Int>& styleSelectionStyles, const Containers::StridedArrayView1D<const Vector4>& stylePaddings) {
    State& state = static_cast<State&>(*_state);
    CORRADE_ASSERT(styleToUniform.size() == state.styleCount,
        "Ui::TextLayer::Shared::setStyle(): expected" << state.styleCount << "style uniform indices, got" << styleToUniform.size(), *this);
    setStyleInternal(commonUniform, uniforms, styleFonts, styleAlignments, styleFeatures, styleFeatureOffsets, styleFeatureCounts, styleCursorStyles, styleSelectionStyles, stylePaddings);
    #ifndef CORRADE_NO_ASSERT
    for(std::size_t i = 0; i != styleToUniform.size(); ++i)
        CORRADE_ASSERT(styleToUniform[i] < state.styleUniformCount,
            "Ui::TextLayer::Shared::setStyle(): uniform index" << styleToUniform[i] << "out of range for" << state.styleUniformCount << "uniforms" << "at index" << i, *this);
    #endif
    Utility::copy(styleToUniform, stridedArrayView(state.styles).slice(&Implementation::TextLayerStyle::uniform));
    return *this;
}

TextLayer::Shared& TextLayer::Shared::setStyle(const TextLayerCommonStyleUniform& commonUniform, const std::initializer_list<TextLayerStyleUniform> uniforms, const std::initializer_list<UnsignedInt> styleToUniform, const std::initializer_list<FontHandle> styleFonts, const std::initializer_list<Text::Alignment> styleAlignments, const std::initializer_list<TextFeatureValue> styleFeatures, const std::initializer_list<UnsignedInt> styleFeatureOffsets, const std::initializer_list<UnsignedInt> styleFeatureCounts, const std::initializer_list<Int> styleCursorStyles, const std::initializer_list<Int> styleSelectionStyles, const std::initializer_list<Vector4> stylePaddings) {
    return setStyle(commonUniform, Containers::arrayView(uniforms), Containers::stridedArrayView(styleToUniform), Containers::stridedArrayView(styleFonts), Containers::stridedArrayView(styleAlignments), Containers::arrayView(styleFeatures), Containers::stridedArrayView(styleFeatureOffsets), Containers::stridedArrayView(styleFeatureCounts), Containers::stridedArrayView(styleCursorStyles), Containers::stridedArrayView(styleSelectionStyles), Containers::stridedArrayView(stylePaddings));
}

TextLayer::Shared& TextLayer::Shared::setStyle(const TextLayerCommonStyleUniform& commonUniform, const Containers::ArrayView<const TextLayerStyleUniform> uniforms, const Containers::StridedArrayView1D<const FontHandle>& fonts, const Containers::StridedArrayView1D<const Text::Alignment>& alignments, const Containers::ArrayView<const TextFeatureValue> features, const Containers::StridedArrayView1D<const UnsignedInt>& featureOffsets, const Containers::StridedArrayView1D<const UnsignedInt>& featureCounts, const Containers::StridedArrayView1D<const Int>& cursorStyles, const Containers::StridedArrayView1D<const Int>& selectionStyles, const Containers::StridedArrayView1D<const Vector4>& paddings) {
    State& state = static_cast<State&>(*_state);
    CORRADE_ASSERT(state.styleUniformCount == state.styleCount,
        "Ui::TextLayer::Shared::setStyle(): there's" << state.styleUniformCount << "uniforms for" << state.styleCount << "styles, provide an explicit mapping", *this);
    setStyleInternal(commonUniform, uniforms, fonts, alignments, features, featureOffsets, featureCounts, cursorStyles, selectionStyles, paddings);
    for(UnsignedInt i = 0; i != state.styleCount; ++i)
        state.styles[i].uniform = i;
    return *this;
}

TextLayer::Shared& TextLayer::Shared::setStyle(const TextLayerCommonStyleUniform& commonUniform, const std::initializer_list<TextLayerStyleUniform> uniforms, const std::initializer_list<FontHandle> fonts, const std::initializer_list<Text::Alignment> alignments, const std::initializer_list<TextFeatureValue> features, const std::initializer_list<UnsignedInt> featureOffsets, const std::initializer_list<UnsignedInt> featureCounts, const std::initializer_list<Int> cursorStyles, const std::initializer_list<Int> selectionStyles, const std::initializer_list<Vector4> paddings) {
    return setStyle(commonUniform, Containers::arrayView(uniforms), Containers::stridedArrayView(fonts), Containers::stridedArrayView(alignments), Containers::arrayView(features), Containers::stridedArrayView(featureOffsets), Containers::stridedArrayView(featureCounts), Containers::stridedArrayView(cursorStyles), Containers::stridedArrayView(selectionStyles), Containers::stridedArrayView(paddings));
}

void TextLayer::Shared::setEditingStyleInternal(const TextLayerCommonEditingStyleUniform& commonUniform, const Containers::ArrayView<const TextLayerEditingStyleUniform> uniforms, const Containers::StridedArrayView1D<const Int>& styleTextUniforms, const Containers::StridedArrayView1D<const Vector4>& stylePaddings) {
    State& state = static_cast<State&>(*_state);
    CORRADE_ASSERT(uniforms.size() == state.editingStyleUniformCount,
        "Ui::TextLayer::Shared::setEditingStyle(): expected" << state.editingStyleUniformCount << "uniforms, got" << uniforms.size(), );
    CORRADE_ASSERT(styleTextUniforms.isEmpty() || styleTextUniforms.size() == state.editingStyles.size(),
        "Ui::TextLayer::Shared::setEditingStyle(): expected either no or" << state.editingStyles.size() << "text uniform indices, got" << styleTextUniforms.size(), );
    CORRADE_ASSERT(stylePaddings.size() == state.editingStyles.size(),
        "Ui::TextLayer::Shared::setEditingStyle(): expected" << state.editingStyles.size() << "paddings, got" << stylePaddings.size(), );
    if(styleTextUniforms.isEmpty()) {
        /** @todo some Utility::fill() for this */
        for(Implementation::TextLayerEditingStyle& style: state.editingStyles)
            style.textUniform = -1;
    } else {
        #ifndef CORRADE_NO_ASSERT
        for(std::size_t i = 0; i != styleTextUniforms.size(); ++i)
            CORRADE_ASSERT(styleTextUniforms[i] == -1 || UnsignedInt(styleTextUniforms[i]) < state.styleUniformCount,
                "Ui::TextLayer::Shared::setEditingStyle(): text uniform index" << styleTextUniforms[i] << "out of range for" << state.styleUniformCount << "uniforms" << "at index" << i, );
        #endif
        Utility::copy(styleTextUniforms, stridedArrayView(state.editingStyles).slice(&Implementation::TextLayerEditingStyle::textUniform));
    }
    Utility::copy(stylePaddings, stridedArrayView(state.editingStyles).slice(&Implementation::TextLayerEditingStyle::padding));

    /* If there are dynamic styles, the layers will combine them with the
       static styles and upload to a single buffer, so just copy them to an
       array for the layers to reuse */
    if(state.dynamicStyleCount) {
        state.commonEditingStyleUniform = commonUniform;
        Utility::copy(uniforms, state.editingStyleUniforms);
    } else doSetEditingStyle(commonUniform, uniforms);

    #ifndef CORRADE_NO_ASSERT
    /* Now it's safe to call update() */
    state.setEditingStyleCalled = true;
    #endif

    /* Make doState() of all layers sharing this state return NeedsDataUpdate
       in order to update style-to-uniform mappings, paddings and such, and in
       case of dynamic styles also NeedsCommonDataUpdate to upload the changed
       per-layer uniform buffers. Setting it only if those differ would trigger
       update only if actually needed, but it may be prohibitively expensive
       compared to updating always. */
    ++state.editingStyleUpdateStamp;
}

TextLayer::Shared& TextLayer::Shared::setEditingStyle(const TextLayerCommonEditingStyleUniform& commonUniform, const Containers::ArrayView<const TextLayerEditingStyleUniform> uniforms, const Containers::StridedArrayView1D<const UnsignedInt>& styleToUniform, const Containers::StridedArrayView1D<const Int>& styleTextUniforms, const Containers::StridedArrayView1D<const Vector4>& stylePaddings) {
    State& state = static_cast<State&>(*_state);
    CORRADE_ASSERT(styleToUniform.size() == state.editingStyles.size(),
        "Ui::TextLayer::Shared::setEditingStyle(): expected" << state.editingStyles.size() << "style uniform indices, got" << styleToUniform.size(), *this);
    setEditingStyleInternal(commonUniform, uniforms, styleTextUniforms, stylePaddings);
    #ifndef CORRADE_NO_ASSERT
    for(std::size_t i = 0; i != styleToUniform.size(); ++i)
        CORRADE_ASSERT(styleToUniform[i] < state.editingStyleUniformCount,
            "Ui::TextLayer::Shared::setEditingStyle(): uniform index" << styleToUniform[i] << "out of range for" << state.editingStyleUniformCount << "uniforms" << "at index" << i, *this);
    #endif
    Utility::copy(styleToUniform, stridedArrayView(state.editingStyles).slice(&Implementation::TextLayerEditingStyle::uniform));
    return *this;
}

TextLayer::Shared& TextLayer::Shared::setEditingStyle(const TextLayerCommonEditingStyleUniform& commonUniform, std::initializer_list<TextLayerEditingStyleUniform> uniforms, std::initializer_list<UnsignedInt> styleToUniform, std::initializer_list<Int> styleTextUniforms, std::initializer_list<Vector4> stylePaddings) {
    return setEditingStyle(commonUniform, Containers::arrayView(uniforms), Containers::stridedArrayView(styleToUniform), Containers::stridedArrayView(styleTextUniforms), Containers::stridedArrayView(stylePaddings));
}

TextLayer::Shared& TextLayer::Shared::setEditingStyle(const TextLayerCommonEditingStyleUniform& commonUniform, const Containers::ArrayView<const TextLayerEditingStyleUniform> uniforms, const Containers::StridedArrayView1D<const Int>& textUniforms, const Containers::StridedArrayView1D<const Vector4>& paddings) {
    State& state = static_cast<State&>(*_state);
    CORRADE_ASSERT(state.editingStyleUniformCount == state.editingStyles.size(),
        "Ui::TextLayer::Shared::setEditingStyle(): there's" << state.editingStyleUniformCount << "uniforms for" << state.editingStyles.size() << "styles, provide an explicit mapping", *this);
    setEditingStyleInternal(commonUniform, uniforms, textUniforms, paddings);
    for(UnsignedInt i = 0; i != state.editingStyles.size(); ++i)
        state.editingStyles[i].uniform = i;
    return *this;
}

TextLayer::Shared& TextLayer::Shared::setEditingStyle(const TextLayerCommonEditingStyleUniform& commonUniform, const std::initializer_list<TextLayerEditingStyleUniform> uniforms, const std::initializer_list<Int> textUniforms, const std::initializer_list<Vector4> paddings) {
    return setEditingStyle(commonUniform, Containers::arrayView(uniforms), Containers::stridedArrayView(textUniforms), Containers::stridedArrayView(paddings));
}

TextLayer::Shared::Configuration::Configuration(const UnsignedInt styleUniformCount, const UnsignedInt styleCount): _styleUniformCount{styleUniformCount}, _styleCount{styleCount} {
    CORRADE_ASSERT(!styleUniformCount == !styleCount,
        "Ui::TextLayer::Shared::Configuration: expected style uniform count and style count to be either both zero or both non-zero, got" << styleUniformCount << "and" << styleCount, );
}

TextLayer::Shared::Configuration& TextLayer::Shared::Configuration::setEditingStyleCount(const UnsignedInt uniformCount, const UnsignedInt count) {
    CORRADE_ASSERT(!uniformCount == !count,
        "Ui::TextLayer::Shared::Configuration::setEditingStyleCount(): expected uniform count and count to be either both zero or both non-zero, got" << uniformCount << "and" << count, *this);
    CORRADE_ASSERT(_styleCount || !count,
        "Ui::TextLayer::Shared::Configuration::setEditingStyleCount(): editing style count has to be zero if style count is zero, got" << count, *this);
    _editingStyleUniformCount = uniformCount;
    _editingStyleCount = count;
    return *this;
}

TextLayer::Shared::Configuration& TextLayer::Shared::Configuration::setDynamicStyleCount(const UnsignedInt count, const bool withEditingStyles) {
    _dynamicStyleCount = count;
    /* If there are no dynamic styles, we don't have editing styles for them
       either */
    _dynamicEditingStyles = count && withEditingStyles;
    return *this;
}

TextLayer::State::State(Shared::State& shared): AbstractVisualLayer::State{shared}, styleUpdateStamp{shared.styleUpdateStamp}, editingStyleUpdateStamp{shared.editingStyleUpdateStamp} {
    dynamicStyleStorage = Containers::ArrayTuple{
        /* If editing styles are present, the uniform array additionally stores
           also uniforms for selected text (and reserved for cursors) */
        {ValueInit, shared.dynamicStyleCount*(shared.hasEditingStyles ? 3 : 1), dynamicStyleUniforms},
        {ValueInit, shared.dynamicStyleCount, dynamicStyles},
        {ValueInit, shared.dynamicStyleCount, dynamicStyleCursorStyles},
        {ValueInit, shared.dynamicStyleCount, dynamicStyleSelectionStyles},
        /* If editing styles are present, the arrays are twice as large as
           every dynamic style can have both a cursor and a selection editing
           style */
        {ValueInit, shared.hasEditingStyles ? shared.dynamicStyleCount*2 : 0, dynamicEditingStyleUniforms},
        {ValueInit, shared.hasEditingStyles ? shared.dynamicStyleCount*2 : 0, dynamicEditingStylePaddings},
    };
}

TextLayer::TextLayer(const LayerHandle handle, Containers::Pointer<State>&& state): AbstractVisualLayer{handle, Utility::move(state)} {}

TextLayer::TextLayer(const LayerHandle handle, Shared& shared): TextLayer{handle, Containers::pointer<State>(static_cast<Shared::State&>(*shared._state))} {}

TextLayer& TextLayer::assignAnimator(TextLayerStyleAnimator& animator) {
    return static_cast<TextLayer&>(AbstractVisualLayer::assignAnimator(animator));
}

TextLayerStyleAnimator* TextLayer::defaultStyleAnimator() const {
    return static_cast<TextLayerStyleAnimator*>(_state->styleAnimator);
}

TextLayer& TextLayer::setDefaultStyleAnimator(TextLayerStyleAnimator* const animator) {
    return static_cast<TextLayer&>(AbstractVisualLayer::setDefaultStyleAnimator(animator));
}

Containers::ArrayView<const TextLayerStyleUniform> TextLayer::dynamicStyleUniforms() const {
    return static_cast<const State&>(*_state).dynamicStyleUniforms;
}

Containers::StridedArrayView1D<const FontHandle> TextLayer::dynamicStyleFonts() const {
    return stridedArrayView(static_cast<const State&>(*_state).dynamicStyles).slice(&Implementation::TextLayerDynamicStyle::font);
}

Containers::StridedArrayView1D<const Text::Alignment> TextLayer::dynamicStyleAlignments() const {
    return stridedArrayView(static_cast<const State&>(*_state).dynamicStyles).slice(&Implementation::TextLayerDynamicStyle::alignment);
}

Containers::ArrayView<const TextFeatureValue> TextLayer::dynamicStyleFeatures(const UnsignedInt id) const {
    auto& state = static_cast<const State&>(*_state);
    CORRADE_ASSERT(id < state.dynamicStyles.size(),
        "Ui::TextLayer::dynamicStyleFeatures(): index" << id << "out of range for" << state.dynamicStyles.size() << "dynamic styles", {});
    return state.dynamicStyleFeatures.sliceSize(
        state.dynamicStyles[id].featureOffset,
        state.dynamicStyles[id].featureCount);
}

Containers::BitArrayView TextLayer::dynamicStyleCursorStyles() const {
    return static_cast<const State&>(*_state).dynamicStyleCursorStyles;
}

Int TextLayer::dynamicStyleCursorStyle(const UnsignedInt id) const {
    auto& state = static_cast<const State&>(*_state);
    CORRADE_ASSERT(id < state.dynamicStyles.size(),
        "Ui::TextLayer::dynamicStyleCursorStyle(): index" << id << "out of range for" << state.dynamicStyles.size() << "dynamic styles", {});
    return state.dynamicStyleCursorStyles[id] ?
        Implementation::cursorStyleForDynamicStyle(id) : -1;
}

Containers::BitArrayView TextLayer::dynamicStyleSelectionStyles() const {
    return static_cast<const State&>(*_state).dynamicStyleSelectionStyles;
}

Int TextLayer::dynamicStyleSelectionStyle(const UnsignedInt id) const {
    auto& state = static_cast<const State&>(*_state);
    CORRADE_ASSERT(id < state.dynamicStyles.size(),
        "Ui::TextLayer::dynamicStyleSelectionStyle(): index" << id << "out of range for" << state.dynamicStyles.size() << "dynamic styles", {});
    return state.dynamicStyleSelectionStyles[id] ?
        Implementation::selectionStyleForDynamicStyle(id) : -1;
}

Int TextLayer::dynamicStyleSelectionStyleTextUniform(const UnsignedInt id) const {
    auto& state = static_cast<const State&>(*_state);
    CORRADE_ASSERT(id < state.dynamicStyles.size(),
        "Ui::TextLayer::dynamicStyleSelectionStyleTextUniform(): index" << id << "out of range for" << state.dynamicStyles.size() << "dynamic styles", {});
    return state.dynamicStyleSelectionStyles[id] ?
        Implementation::selectionStyleTextUniformForDynamicStyle(state.dynamicStyles.size(), id) : -1;
}

Containers::StridedArrayView1D<const Vector4> TextLayer::dynamicStylePaddings() const {
    return stridedArrayView(static_cast<const State&>(*_state).dynamicStyles).slice(&Implementation::TextLayerDynamicStyle::padding);
}

Containers::ArrayView<const TextLayerEditingStyleUniform> TextLayer::dynamicEditingStyleUniforms() const {
    return static_cast<const State&>(*_state).dynamicEditingStyleUniforms;
}

Containers::StridedArrayView1D<const Vector4> TextLayer::dynamicEditingStylePaddings() const {
    return static_cast<const State&>(*_state).dynamicEditingStylePaddings;
}

void TextLayer::setDynamicStyleInternal(
    #ifndef CORRADE_NO_ASSERT
    const char* const messagePrefix,
    #endif
    const UnsignedInt id, const TextLayerStyleUniform& uniform, const FontHandle font, const Text::Alignment alignment, const Containers::ArrayView<const TextFeatureValue> features, const Vector4& padding)
{
    auto& state = static_cast<State&>(*_state);
    CORRADE_ASSERT(font == FontHandle::Null || Ui::isHandleValid(static_cast<const Shared::State&>(state.shared).fonts, font),
        messagePrefix << "invalid handle" << font, );
    CORRADE_ASSERT(!(UnsignedByte(alignment) & Text::Implementation::AlignmentGlyphBounds),
        messagePrefix << alignment << "is not supported", );
    state.dynamicStyleUniforms[id] = uniform;

    /* Mark the layer as needing the dynamic style data update. The additional
       boolean is set to distinguish between needing to update the shared part
       of the style and the dynamic part, and whether the base or the editing
       style updated. */
    setNeedsUpdate(LayerState::NeedsCommonDataUpdate);
    state.dynamicStyleChanged = true;

    /* Mark the layer as changed only if the padding actually changes,
       otherwise it's not needed to trigger an update(). OTOH changing the
       font, alignment or feature list doesn't / cannot trigger an update
       because we don't keep the source text string to be able to reshape or
       realign line by line. */
    Implementation::TextLayerDynamicStyle& style = state.dynamicStyles[id];
    style.font = font;
    style.alignment = alignment;
    /* If the feature count is different from what was set for this style
       before, remove them from the array, reindex the dynamic styles after and
       make room for them at the end. This way the dynamic styles that have the
       feature list size changing the most will eventually get moved to the
       end, with the array prefix staying. */
    /** @todo it may still become an annoying bottleneck if too many styles
        with non-empty features get updated in a frame, consider setting a flag
        and moving the recompacting to doUpdate() instead */
    if(style.featureCount != features.size()) {
        arrayRemove(state.dynamicStyleFeatures, style.featureOffset, style.featureCount);
        /* The >= will change the feature offset for the style itself as well,
           so compare to a copy */
        const UnsignedInt originalFeatureOffset = style.featureOffset;
        for(Implementation::TextLayerDynamicStyle& i: state.dynamicStyles)
            if(i.featureOffset >= originalFeatureOffset)
                i.featureOffset -= style.featureCount;
        style.featureOffset = state.dynamicStyleFeatures.size();
        style.featureCount = features.size();
        arrayAppend(state.dynamicStyleFeatures, NoInit, features.size());
    }
    Utility::copy(features, state.dynamicStyleFeatures.sliceSize(style.featureOffset, style.featureCount));
    if(style.padding != padding) {
        style.padding = padding;
        setNeedsUpdate(LayerState::NeedsDataUpdate);
    }
}

void TextLayer::setDynamicCursorStyleInternal(
    #ifndef CORRADE_NO_ASSERT
    const char* const messagePrefix,
    #endif
    const UnsignedInt id, const TextLayerEditingStyleUniform& uniform, const Vector4& padding)
{
    auto& state = static_cast<State&>(*_state);
    #ifndef CORRADE_NO_ASSERT
    auto& sharedState = static_cast<Shared::State&>(state.shared);
    #endif
    CORRADE_ASSERT(sharedState.hasEditingStyles,
        messagePrefix << "editing styles are not enabled", );

    /* Cursor styles are second in the dynamic style list, after selection
       styles */
    const UnsignedInt editingId = 2*id + 1;
    state.dynamicEditingStyleUniforms[editingId] = uniform;

    /* Mark the layer as needing the dynamic style data update. The additional
       boolean is set to distinguish between needing to update the shared part
       of the style and the dynamic part, and whether the base or the editing
       style updated. */
    setNeedsUpdate(LayerState::NeedsCommonDataUpdate);
    state.dynamicEditingStyleChanged = true;

    /* Mark the layer as changed only if the padding actually changes or if the
       style didn't have a cursor style associated before, otherwise it's not
       needed to trigger an update() */
    Vector4& editingStylePadding = state.dynamicEditingStylePaddings[editingId];
    if(editingStylePadding != padding ||
       !state.dynamicStyleCursorStyles[id])
    {
        editingStylePadding = padding;
        state.dynamicStyleCursorStyles.set(id);
        setNeedsUpdate(LayerState::NeedsDataUpdate);
    }
}

void TextLayer::setDynamicSelectionStyleInternal(
    #ifndef CORRADE_NO_ASSERT
    const char* const messagePrefix,
    #endif
    const UnsignedInt id, const TextLayerEditingStyleUniform& uniform, const Containers::Optional<TextLayerStyleUniform>& textUniform, const Vector4& padding)
{
    auto& state = static_cast<State&>(*_state);
    auto& sharedState = static_cast<Shared::State&>(state.shared);
    CORRADE_ASSERT(sharedState.hasEditingStyles,
        messagePrefix << "editing styles are not enabled", );

    /* Selection styles are first in the dynamic editing style list */
    const UnsignedInt editingId = 2*id + 0;
    state.dynamicEditingStyleUniforms[editingId] = uniform;

    /* If an uniform for the selected text is supplied, update it, otherwise
       copy over the original text uniform. The textUniformId is the same in
       both cases to not have to change the data when the uniform override
       presence changes. The ID is calculated the same way as with the
       selection uniform. */
    const UnsignedInt textUniformId = sharedState.dynamicStyleCount + 2*id + 0;
    state.dynamicStyleUniforms[textUniformId] =
        textUniform ? *textUniform : state.dynamicStyleUniforms[id];

    /* Mark the layer as needing the dynamic style data update. The additional
       boolean is set to distinguish between needing to update the shared part
       of the style and the dynamic part, and whether the base or the editing
       style updated. */
    setNeedsUpdate(LayerState::NeedsCommonDataUpdate);
    state.dynamicEditingStyleChanged = true;
    /* As we updated the non-editing part of the style with the text uniform,
       the regular style needs to update as well, which should be already done
       by setDynamicStyleInternal() that's called together with this function
       in all cases */
    CORRADE_INTERNAL_ASSERT(state.dynamicStyleChanged);

    /* Mark the layer as changed only if the color or padding actually changes,
       otherwise it's not needed to trigger an update() */
    Vector4& editingStylePadding = state.dynamicEditingStylePaddings[editingId];
    if(editingStylePadding != padding ||
       !state.dynamicStyleSelectionStyles[id])
    {
        editingStylePadding = padding;
        state.dynamicStyleSelectionStyles.set(id);
        setNeedsUpdate(LayerState::NeedsDataUpdate);
    }
}

void TextLayer::setDynamicStyle(const UnsignedInt id, const TextLayerStyleUniform& uniform, const FontHandle font, const Text::Alignment alignment, const Containers::ArrayView<const TextFeatureValue> features, const Vector4& padding) {
    auto& state = static_cast<State&>(*_state);
    CORRADE_ASSERT(id < state.dynamicStyles.size(),
        "Ui::TextLayer::setDynamicStyle(): index" << id << "out of range for" << state.dynamicStyles.size() << "dynamic styles", );
    setDynamicStyleInternal(
        #ifndef CORRADE_NO_ASSERT
        "Ui::TextLayer::setDynamicStyle():",
        #endif
        id, uniform, font, alignment, features, padding);

    /* Cursor and selection style is unset in this case, mark the layer as
       changed if they weren't unset before */
    if(state.dynamicStyleCursorStyles[id]) {
        /* Reset also the style values to reduce entropy */
        const UnsignedInt cursorStyle = Implementation::cursorStyleForDynamicStyle(id);
        state.dynamicEditingStyleUniforms[cursorStyle] = TextLayerEditingStyleUniform{};
        state.dynamicEditingStylePaddings[cursorStyle] = {};
        state.dynamicStyleCursorStyles.reset(id);
        setNeedsUpdate(LayerState::NeedsDataUpdate);
    }
    if(state.dynamicStyleSelectionStyles[id]) {
        /* Reset also the style values to reduce entropy */
        const UnsignedInt selectionStyle = Implementation::selectionStyleForDynamicStyle(id);
        state.dynamicEditingStyleUniforms[selectionStyle] = TextLayerEditingStyleUniform{};
        state.dynamicEditingStylePaddings[selectionStyle] = {};
        state.dynamicStyleSelectionStyles.reset(id);
        setNeedsUpdate(LayerState::NeedsDataUpdate);
    }
}

void TextLayer::setDynamicStyle(const UnsignedInt id, const TextLayerStyleUniform& uniform, const FontHandle font, const Text::Alignment alignment, const std::initializer_list<TextFeatureValue> features, const Vector4& padding) {
    setDynamicStyle(id, uniform, font, alignment, Containers::arrayView(features), padding);
}

void TextLayer::setDynamicStyleWithCursorSelection(const UnsignedInt id, const TextLayerStyleUniform& uniform, const FontHandle font, const Text::Alignment alignment, const Containers::ArrayView<const TextFeatureValue> features, const Vector4& padding, const TextLayerEditingStyleUniform& cursorUniform, const Vector4& cursorPadding, const TextLayerEditingStyleUniform& selectionUniform, const Containers::Optional<TextLayerStyleUniform>& selectionTextUniform, const Vector4& selectionPadding) {
    #ifndef CORRADE_NO_ASSERT
    auto& state = static_cast<State&>(*_state);
    #endif
    CORRADE_ASSERT(id < state.dynamicStyles.size(),
        "Ui::TextLayer::setDynamicStyleWithCursorSelection(): index" << id << "out of range for" << state.dynamicStyles.size() << "dynamic styles", );
    setDynamicStyleInternal(
        #ifndef CORRADE_NO_ASSERT
        "Ui::TextLayer::setDynamicStyleWithCursorSelection():",
        #endif
        id, uniform, font, alignment, features, padding);
    setDynamicCursorStyleInternal(
        #ifndef CORRADE_NO_ASSERT
        "Ui::TextLayer::setDynamicStyleWithCursorSelection():",
        #endif
        id, cursorUniform, cursorPadding);
    setDynamicSelectionStyleInternal(
        #ifndef CORRADE_NO_ASSERT
        "Ui::TextLayer::setDynamicStyleWithCursorSelection():",
        #endif
        id, selectionUniform, selectionTextUniform, selectionPadding);
}

void TextLayer::setDynamicStyleWithCursorSelection(const UnsignedInt id, const TextLayerStyleUniform& uniform, const FontHandle font, const Text::Alignment alignment, const std::initializer_list<TextFeatureValue> features, const Vector4& padding, const TextLayerEditingStyleUniform& cursorUniform, const Vector4& cursorPadding, const TextLayerEditingStyleUniform& selectionUniform, const Containers::Optional<TextLayerStyleUniform>& selectionTextUniform, const Vector4& selectionPadding) {
    setDynamicStyleWithCursorSelection(id, uniform, font, alignment, Containers::arrayView(features), padding, cursorUniform, cursorPadding, selectionUniform, selectionTextUniform, selectionPadding);
}

void TextLayer::setDynamicStyleWithCursor(const UnsignedInt id, const TextLayerStyleUniform& uniform, const FontHandle font, const Text::Alignment alignment, const Containers::ArrayView<const TextFeatureValue> features, const Vector4& padding, const TextLayerEditingStyleUniform& cursorUniform, const Vector4& cursorPadding) {
    auto& state = static_cast<State&>(*_state);
    CORRADE_ASSERT(id < state.dynamicStyles.size(),
        "Ui::TextLayer::setDynamicStyleWithCursor(): index" << id << "out of range for" << state.dynamicStyles.size() << "dynamic styles", );
    setDynamicStyleInternal(
        #ifndef CORRADE_NO_ASSERT
        "Ui::TextLayer::setDynamicStyleWithCursor():",
        #endif
        id, uniform, font, alignment, features, padding);
    setDynamicCursorStyleInternal(
        #ifndef CORRADE_NO_ASSERT
        "Ui::TextLayer::setDynamicStyleWithCursor():",
        #endif
        id, cursorUniform, cursorPadding);

    /* Selection style is unset in this case, mark the layer as changed if it
       wasn't before */
    if(state.dynamicStyleSelectionStyles[id]) {
        /* Reset also the style values to reduce entropy */
        const UnsignedInt selectionStyle = Implementation::selectionStyleForDynamicStyle(id);
        state.dynamicEditingStyleUniforms[selectionStyle] = TextLayerEditingStyleUniform{};
        state.dynamicEditingStylePaddings[selectionStyle] = {};
        state.dynamicStyleSelectionStyles.reset(id);
        setNeedsUpdate(LayerState::NeedsDataUpdate);
    }
}

void TextLayer::setDynamicStyleWithCursor(const UnsignedInt id, const TextLayerStyleUniform& uniform, const FontHandle font, const Text::Alignment alignment, const std::initializer_list<TextFeatureValue> features, const Vector4& padding, const TextLayerEditingStyleUniform& cursorUniform, const Vector4& cursorPadding) {
    setDynamicStyleWithCursor(id, uniform, font, alignment, Containers::arrayView(features), padding, cursorUniform, cursorPadding);
}

void TextLayer::setDynamicStyleWithSelection(const UnsignedInt id, const TextLayerStyleUniform& uniform, const FontHandle font, const Text::Alignment alignment, const Containers::ArrayView<const TextFeatureValue> features, const Vector4& padding, const TextLayerEditingStyleUniform& selectionUniform, const Containers::Optional<TextLayerStyleUniform>& selectionTextUniform, const Vector4& selectionPadding) {
    auto& state = static_cast<State&>(*_state);
    CORRADE_ASSERT(id < state.dynamicStyles.size(),
        "Ui::TextLayer::setDynamicStyleWithSelection(): index" << id << "out of range for" << state.dynamicStyles.size() << "dynamic styles", );
    setDynamicStyleInternal(
        #ifndef CORRADE_NO_ASSERT
        "Ui::TextLayer::setDynamicStyleWithSelection():",
        #endif
        id, uniform, font, alignment, features, padding);
    setDynamicSelectionStyleInternal(
        #ifndef CORRADE_NO_ASSERT
        "Ui::TextLayer::setDynamicStyleWithSelection():",
        #endif
        id, selectionUniform, selectionTextUniform, selectionPadding);

    /* Cursor style is unset in this case, mark the layer as changed if it
       wasn't unset before */
    if(state.dynamicStyleCursorStyles[id]) {
        /* Reset also the style values to reduce entropy */
        const UnsignedInt cursorStyle = Implementation::cursorStyleForDynamicStyle(id);
        state.dynamicEditingStyleUniforms[cursorStyle] = TextLayerEditingStyleUniform{};
        state.dynamicEditingStylePaddings[cursorStyle] = {};
        state.dynamicStyleCursorStyles.reset(id);
        setNeedsUpdate(LayerState::NeedsDataUpdate);
    }
}

void TextLayer::setDynamicStyleWithSelection(const UnsignedInt id, const TextLayerStyleUniform& uniform, const FontHandle font, const Text::Alignment alignment, const std::initializer_list<TextFeatureValue> features, const Vector4& padding, const TextLayerEditingStyleUniform& selectionUniform, const Containers::Optional<TextLayerStyleUniform>& selectionTextUniform, const Vector4& selectionPadding) {
    setDynamicStyleWithSelection(id, uniform, font, alignment, Containers::arrayView(features), padding, selectionUniform, selectionTextUniform, selectionPadding);
}

void TextLayer::shapeTextInternal(const UnsignedInt id, const UnsignedInt style, const Containers::StringView text, const TextProperties& properties, const FontHandle font, const TextDataFlags flags) {
    State& state = static_cast<State&>(*_state);
    Shared::State& sharedState = static_cast<Shared::State&>(state.shared);

    /* The shapeRememberTextInternal() should originally have checked that the
       font isn't null and has an instance, editShapeTextInternal() then just
       passes what has been saved by shapeRememberTextInternal() */
    Implementation::TextLayerFont& fontState = sharedState.fonts[fontHandleId(font)];
    CORRADE_INTERNAL_ASSERT(font != FontHandle::Null && fontState.font);

    /* Decide on alignment */
    Text::Alignment alignment;
    if(!properties.alignment()) {
        if(style < sharedState.styleCount)
            alignment = sharedState.styles[style].alignment;
        else
            alignment = state.dynamicStyles[style - sharedState.styleCount].alignment;
    } else
        alignment = *properties.alignment();

    /* Put together features from the style and TextProperties. Style goes
       first to make it possible to override it. */
    /** @todo some bump allocator for this, ugh */
    Containers::ArrayView<const TextFeatureValue> styleFeatures;
    if(style < sharedState.styleCount)
        styleFeatures = sharedState.styleFeatures.sliceSize(
            sharedState.styles[style].featureOffset,
            sharedState.styles[style].featureCount);
    else
        styleFeatures = state.dynamicStyleFeatures.sliceSize(
            state.dynamicStyles[style - sharedState.styleCount].featureOffset,
            state.dynamicStyles[style - sharedState.styleCount].featureCount);
    Containers::Array<Text::FeatureRange> features{NoInit, styleFeatures.size() + properties.features().size()};
    /* This performs a conversion from TextFeatureValue to Text::FeatureRange,
       so can't use Utility::copy() */
    for(std::size_t i = 0; i != styleFeatures.size(); ++i)
        features[i] = styleFeatures[i];
    Utility::copy(properties.features(), features.exceptPrefix(styleFeatures.size()));

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
    const UnsignedInt glyphCount = shaper.shape(text, features);

    /* Resolve the alignment based on direction */
    const Text::Alignment resolvedAlignment = Text::alignmentForDirection(alignment,
        properties.layoutDirection(),
        shaper.direction());

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
    Range2D rectangle{NoInit};
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
            resolvedAlignment,
            glyphOffsetsPositions);
        rectangle = Text::alignRenderedBlock(
            blockRectangle,
            properties.layoutDirection(),
            resolvedAlignment,
            glyphOffsetsPositions);
    }

    /* Glyph cache. The create() (or createGlyph()) should have ensured that a
       glyph cache is set, thus the subsequent setText() doesn't need to check
       again. */
    const Text::AbstractGlyphCache* const glyphCache = sharedState.glyphCache;
    CORRADE_INTERNAL_ASSERT(glyphCache);

    /* Query font-specific glyph IDs and convert them to cache-global */
    shaper.glyphIdsInto(glyphData.slice(&Implementation::TextLayerGlyphData::glyphId));
    {
        for(Implementation::TextLayerGlyphData& glyph: glyphData)
            glyph.glyphId = glyphCache->glyphId(fontState.glyphCacheFontId, glyph.glyphId);
    }

    /* Save scale, size, direction-resolved alignment and the glyph run
       reference for use in doUpdate() later */
    Implementation::TextLayerData& data = state.data[id];
    data.scale = fontState.scale;
    data.rectangle = rectangle;
    data.alignment = resolvedAlignment;
    data.glyphRun = glyphRun;

    /* Save extra properties used by editable text. They occupy otherwise
       unused free space in TextLayerData and TextLayerGlyphData, see the
       member documentation for details why they're stored here and not in
       dedicated edit-only structures. */
    if(flags >= TextDataFlag::Editable) {
        data.usedDirection = shaper.direction();
        shaper.glyphClustersInto(glyphData.slice(&Implementation::TextLayerGlyphData::glyphCluster));

    /* If the text is not editable, reset the direction to prevent other code
       accidentally relying on some random value. The clusters aren't reset
       though, as that is extra overhead. */
    } else data.usedDirection = Text::ShapeDirection::Unspecified;
}

void TextLayer::shapeRememberTextInternal(
    #ifndef CORRADE_NO_ASSERT
    const char* const messagePrefix,
    #endif
    const UnsignedInt id, const UnsignedInt style, const Containers::StringView text, const TextProperties& properties, const TextDataFlags flags)
{
    State& state = static_cast<State&>(*_state);
    Shared::State& sharedState = static_cast<Shared::State&>(state.shared);

    /* Decide on a font */
    FontHandle font = properties.font();
    if(font == FontHandle::Null) {
        if(style < sharedState.styleCount) {
            CORRADE_ASSERT(sharedState.styles[style].font != FontHandle::Null,
                messagePrefix << "style" << style << "has no font set and no custom font was supplied", );
            font = sharedState.styles[style].font;
        } else {
            CORRADE_INTERNAL_DEBUG_ASSERT(style < sharedState.styleCount + sharedState.dynamicStyleCount);
            font = state.dynamicStyles[style - sharedState.styleCount].font;
            CORRADE_ASSERT(font != FontHandle::Null,
                messagePrefix << "dynamic style" << style - sharedState.styleCount << "has no font set and no custom font was supplied", );
        }
    } else CORRADE_ASSERT(Ui::isHandleValid(sharedState.fonts, font),
        messagePrefix << "invalid handle" << font, );

    CORRADE_ASSERT(sharedState.fonts[fontHandleId(font)].font,
        messagePrefix << font << "is an instance-less font", );

    shapeTextInternal(id, style, text, properties, font, flags);

    Implementation::TextLayerData& data = state.data[id];
    data.flags = flags;

    /* If the text is meant to be editable, remember the input string */
    if(flags >= TextDataFlag::Editable) {
        CORRADE_ASSERT(properties.features().isEmpty(),
            messagePrefix << "passing font features for an editable text is not implemented yet, sorry", );
        CORRADE_ASSERT(
            properties.shapeDirection() != Text::ShapeDirection::TopToBottom &&
            properties.shapeDirection() != Text::ShapeDirection::BottomToTop,
            messagePrefix << "vertical shape direction for an editable text is not implemented yet, sorry", );

        /* Add a new text run. Any previous run for this data was marked as
           unused in previous remove() or in setText() before calling this
           function. */
        const UnsignedInt textRun = state.textRuns.size();
        const UnsignedInt textOffset = state.textData.size();
        arrayAppend(state.textData, text);
        Implementation::TextLayerTextRun& run = arrayAppend(state.textRuns, NoInit, 1).front();
        run.textOffset = textOffset;
        run.textSize = text.size();
        run.data = id;
        run.cursor = run.selection = text.size();

        /* Save the text properties. Copy the internals instead of saving the
           whole TextProperties instance to have the text runs trivially
           copyable. */
        Utility::copy(properties._language, run.language);
        run.script = properties._script;
        /* Save the actual font used to not have to do the above branching (and
           assertions) on every updateText() / editText() */
        run.font = font;
        run.alignment = properties._alignment;
        run.direction = properties._direction;

        /* Save the text run reference */
        data.textRun = textRun;

    /* Otherwise mark it as having no associated text run */
    } else data.textRun = ~UnsignedInt{};
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
        if(style < sharedState.styleCount) {
            CORRADE_ASSERT(sharedState.styles[style].font != FontHandle::Null,
                messagePrefix << "style" << style << "has no font set and no custom font was supplied", );
            font = sharedState.styles[style].font;
        } else {
            CORRADE_INTERNAL_DEBUG_ASSERT(style < sharedState.styleCount + sharedState.dynamicStyleCount);
            font = state.dynamicStyles[style - sharedState.styleCount].font;
            CORRADE_ASSERT(font != FontHandle::Null,
                messagePrefix << "dynamic style" << style - sharedState.styleCount << "has no font set and no custom font was supplied", );
        }
    } else CORRADE_ASSERT(Ui::isHandleValid(sharedState.fonts, font),
        messagePrefix << "invalid handle" << font, );

    /* Decide on alignment */
    Text::Alignment alignment;
    if(!properties.alignment()) {
        if(style < sharedState.styleCount)
            alignment = sharedState.styles[style].alignment;
        else
            alignment = state.dynamicStyles[style - sharedState.styleCount].alignment;
    } else
        alignment = *properties.alignment();

    /* Resolve direction-based alignment based on the information passed in
       TextProperties */
    const Text::Alignment resolvedAlignment = Text::alignmentForDirection(alignment, properties.layoutDirection(), properties.shapeDirection());

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
    Range2D rectangle{NoInit};
    {
        const Range2D blockRectangle = Text::alignRenderedLine(
            glyphRectangle,
            properties.layoutDirection(),
            resolvedAlignment,
            glyphPosition);
        rectangle = Text::alignRenderedBlock(
            blockRectangle,
            properties.layoutDirection(),
            resolvedAlignment,
            glyphPosition);
    }

    /* Add a new run containing just that one glyph. Any previous run for this
       data was marked as unused in previous remove(), or in setGlyph() right
       before calling this function. */
    const UnsignedInt glyphRun = state.glyphRuns.size();
    const UnsignedInt glyphOffset = state.glyphData.size();
    arrayAppend(state.glyphData, InPlaceInit,
        *glyphPosition, cacheGlobalGlyphId, 0u /* (Unused) cluster ID */);
    arrayAppend(state.glyphRuns, InPlaceInit, glyphOffset, 1u, id);

    /* Save scale, size, direction-resolved alignment and the glyph run
       reference for use in doUpdate() later */
    Implementation::TextLayerData& data = state.data[id];
    data.scale = fontState.scale;
    data.rectangle = rectangle;
    data.alignment = resolvedAlignment;
    data.glyphRun = glyphRun;
    data.textRun = ~UnsignedInt{};
    data.flags = {};
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

DataHandle TextLayer::create(const UnsignedInt style, const Containers::StringView text, const TextProperties& properties, const Color3& color, const TextDataFlags flags, const NodeHandle node) {
    State& state = static_cast<State&>(*_state);
    #ifndef CORRADE_NO_ASSERT
    Shared::State& sharedState = static_cast<Shared::State&>(state.shared);
    #endif
    CORRADE_ASSERT(sharedState.setStyleCalled,
        "Ui::TextLayer::create(): no style data was set", {});
    /* Unlike the base style, the editing style doesn't need to be set for
       create() to work */
    CORRADE_ASSERT(style < sharedState.styleCount + sharedState.dynamicStyleCount,
        "Ui::TextLayer::create(): style" << style << "out of range for" << sharedState.styleCount + sharedState.dynamicStyleCount << "styles", {});

    /* Create a data */
    const DataHandle handle = createInternal(node);
    const UnsignedInt id = dataHandleId(handle);

    /* Shape the text, save its properties and optionally also the source
       string if it's editable */
    shapeRememberTextInternal(
        #ifndef CORRADE_NO_ASSERT
        "Ui::TextLayer::create():",
        #endif
        id, style, text, properties, flags);
    Implementation::TextLayerData& data = state.data[id];
    /** @todo is there a way to have create() with all possible per-data
        options that doesn't make it ambiguous / impossible to extend further?
        like, having both color and padding optional is ambiguous, etc. */
    data.padding = {};
    /* glyphRun, textRun and flags is filled by shapeTextInternal() */
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
    CORRADE_ASSERT(sharedState.setStyleCalled,
        "Ui::TextLayer::createGlyph(): no style data was set", {});
    CORRADE_ASSERT(style < sharedState.styleCount + sharedState.dynamicStyleCount,
        "Ui::TextLayer::createGlyph(): style" << style << "out of range for" << sharedState.styleCount + sharedState.dynamicStyleCount << "styles", {});

    /* Create a data */
    const DataHandle handle = createInternal(node);
    const UnsignedInt id = dataHandleId(handle);

    /* Shape the glyph, save its properties */
    shapeGlyphInternal(
        #ifndef CORRADE_NO_ASSERT
        "Ui::TextLayer::createGlyph():",
        #endif
        id, style, glyph, properties);
    Implementation::TextLayerData& data = state.data[id];
    /** @todo is there a way to have createGlyph() with all possible per-data
        options that doesn't make it ambiguous / impossible to extend further?
        like, having both color and padding optional is ambiguous, etc. */
    data.padding = {};
    /* glyphRun, textRun and flags is filled by shapeGlyphInternal() */
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

    /* If there's a text run, mark it as unused as well; it'll be removed in
       doUpdate() too */
    if(state.data[id].textRun != ~UnsignedInt{})
        state.textRuns[state.data[id].textRun].textOffset = ~UnsignedInt{};

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

TextDataFlags TextLayer::flags(const DataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::TextLayer::flags(): invalid handle" << handle, {});
    return static_cast<const State&>(*_state).data[dataHandleId(handle)].flags;
}

TextDataFlags TextLayer::flags(const LayerDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::TextLayer::flags(): invalid handle" << handle, {});
    return static_cast<const State&>(*_state).data[layerDataHandleId(handle)].flags;
}

UnsignedInt TextLayer::glyphCount(const DataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::TextLayer::glyphCount(): invalid handle" << handle, {});
    const State& state = static_cast<const State&>(*_state);
    return state.glyphRuns[state.data[dataHandleId(handle)].glyphRun].glyphCount;
}

UnsignedInt TextLayer::glyphCount(const LayerDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::TextLayer::glyphCount(): invalid handle" << handle, {});
    const State& state = static_cast<const State&>(*_state);
    return state.glyphRuns[state.data[layerDataHandleId(handle)].glyphRun].glyphCount;
}

Vector2 TextLayer::size(const DataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::TextLayer::size(): invalid handle" << handle, {});
    const State& state = static_cast<const State&>(*_state);
    return state.data[dataHandleId(handle)].rectangle.size();
}

Vector2 TextLayer::size(const LayerDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::TextLayer::size(): invalid handle" << handle, {});
    const State& state = static_cast<const State&>(*_state);
    return state.data[layerDataHandleId(handle)].rectangle.size();
}

Containers::Pair<UnsignedInt, UnsignedInt> TextLayer::cursor(const DataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::TextLayer::cursor(): invalid handle" << handle, {});
    return cursorInternal(dataHandleId(handle));
}

Containers::Pair<UnsignedInt, UnsignedInt> TextLayer::cursor(const LayerDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::TextLayer::cursor(): invalid handle" << handle, {});
    return cursorInternal(layerDataHandleId(handle));
}

Containers::Pair<UnsignedInt, UnsignedInt> TextLayer::cursorInternal(const UnsignedInt id) const {
    const State& state = static_cast<const State&>(*_state);
    const Implementation::TextLayerData& data = state.data[id];
    CORRADE_ASSERT(data.textRun != ~UnsignedInt{},
        "Ui::TextLayer::cursor(): text doesn't have" << TextDataFlag::Editable << "set", {});
    const Implementation::TextLayerTextRun& run = state.textRuns[data.textRun];
    CORRADE_INTERNAL_ASSERT(run.cursor <= run.textSize && run.selection <= run.textSize);
    return {run.cursor, run.selection};
}

void TextLayer::setCursor(const DataHandle handle, const UnsignedInt position, const UnsignedInt selection) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::TextLayer::setCursor(): invalid handle" << handle, );
    return setCursorInternal(dataHandleId(handle), position, selection);
}

void TextLayer::setCursor(const LayerDataHandle handle, const UnsignedInt position, const UnsignedInt selection) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::TextLayer::setCursor(): invalid handle" << handle, );
    return setCursorInternal(layerDataHandleId(handle), position, selection);
}

void TextLayer::setCursorInternal(const UnsignedInt id, const UnsignedInt position, const UnsignedInt selection) {
    State& state = static_cast<State&>(*_state);
    const Implementation::TextLayerData& data = state.data[id];
    CORRADE_ASSERT(data.textRun != ~UnsignedInt{},
        "Ui::TextLayer::setCursor(): text doesn't have" << TextDataFlag::Editable << "set", );

    Implementation::TextLayerTextRun& run = state.textRuns[data.textRun];
    CORRADE_ASSERT(position <= run.textSize,
        "Ui::TextLayer::setCursor(): position" << position << "out of range for a text of" << run.textSize << "bytes", );
    CORRADE_ASSERT(selection <= run.textSize,
        "Ui::TextLayer::setCursor(): selection" << selection << "out of range for a text of" << run.textSize << "bytes", );

    if(position != run.cursor || selection != run.selection) {
        run.cursor = position;
        run.selection = selection;
        setNeedsUpdate(LayerState::NeedsDataUpdate);
    }
}

TextProperties TextLayer::textProperties(const DataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::TextLayer::textProperties(): invalid handle" << handle, {});
    return textPropertiesInternal(dataHandleId(handle));
}

TextProperties TextLayer::textProperties(const LayerDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::TextLayer::textProperties(): invalid handle" << handle, {});
    return textPropertiesInternal(layerDataHandleId(handle));
}

TextProperties TextLayer::textPropertiesInternal(const UnsignedInt id) const {
    const State& state = static_cast<const State&>(*_state);
    const Implementation::TextLayerData& data = state.data[id];
    CORRADE_ASSERT(data.textRun != ~UnsignedInt{},
        "Ui::TextLayer::textProperties(): text doesn't have" << TextDataFlag::Editable << "set", {});
    const Implementation::TextLayerTextRun& run = state.textRuns[data.textRun];

    TextProperties properties{NoInit};
    Utility::copy(run.language, properties._language);
    properties._script = run.script;
    /* Contrary to what was passed to create() or setText(), the font is always
       non-null here. We'd have to maintain an additional state bit to
       distinguish between font being taken from the style or from the
       TextProperties, then do all the extra font selection logic, then handle
       cases of the style font suddenly becoming null ... Not worth it. */
    properties._font = run.font;
    properties._alignment = run.alignment;
    properties._direction = run.direction;
    return properties;
}

Containers::StringView TextLayer::text(const DataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::TextLayer::text(): invalid handle" << handle, {});
    return textInternal(dataHandleId(handle));
}

Containers::StringView TextLayer::text(const LayerDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::TextLayer::text(): invalid handle" << handle, {});
    return textInternal(layerDataHandleId(handle));
}

Containers::StringView TextLayer::textInternal(const UnsignedInt id) const {
    auto& state = static_cast<const State&>(*_state);
    const Implementation::TextLayerData& data = state.data[id];
    CORRADE_ASSERT(data.textRun != ~UnsignedInt{},
        "Ui::TextLayer::text(): text doesn't have" << TextDataFlag::Editable << "set", {});

    CORRADE_INTERNAL_ASSERT(data.textRun != ~UnsignedInt{});
    return state.textData.sliceSize(state.textRuns[data.textRun].textOffset,
                                    state.textRuns[data.textRun].textSize);
}

void TextLayer::setText(const DataHandle handle, const Containers::StringView text, const TextProperties& properties, const TextDataFlags flags) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::TextLayer::setText(): invalid handle" << handle, );
    setTextInternal(dataHandleId(handle), text, properties, flags);
}

void TextLayer::setText(const DataHandle handle, const Containers::StringView text, const TextProperties& properties) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::TextLayer::setText(): invalid handle" << handle, );
    setTextInternal(dataHandleId(handle), text, properties, static_cast<const State&>(*_state).data[dataHandleId(handle)].flags);
}

void TextLayer::setText(const LayerDataHandle handle, const Containers::StringView text, const TextProperties& properties, const TextDataFlags flags) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::TextLayer::setText(): invalid handle" << handle, );
    setTextInternal(layerDataHandleId(handle), text, properties, flags);
}

void TextLayer::setText(const LayerDataHandle handle, const Containers::StringView text, const TextProperties& properties) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::TextLayer::setText(): invalid handle" << handle, );
    setTextInternal(layerDataHandleId(handle), text, properties, static_cast<const State&>(*_state).data[layerDataHandleId(handle)].flags);
}

void TextLayer::setTextInternal(const UnsignedInt id, const Containers::StringView text, const TextProperties& properties, const TextDataFlags flags) {
    State& state = static_cast<State&>(*_state);
    Implementation::TextLayerData& data = state.data[id];

    /* Mark the original glyph run as unused. It'll be removed during the next
       recompaction in doUpdate(). */
    state.glyphRuns[data.glyphRun].glyphOffset = ~UnsignedInt{};

    /* If there's a text run, mark it as unused as well; it'll be removed in
       doUpdate() too */
    if(state.data[id].textRun != ~UnsignedInt{})
        state.textRuns[state.data[id].textRun].textOffset = ~UnsignedInt{};

    /* Shape the text, save its properties and optionally also the source
       string if it's editable; mark the layer as needing an update */
    shapeRememberTextInternal(
        #ifndef CORRADE_NO_ASSERT
        "Ui::TextLayer::setText():",
        #endif
        id, data.style, text, properties, flags);
    setNeedsUpdate(LayerState::NeedsDataUpdate);
}

void TextLayer::updateText(const DataHandle handle, const UnsignedInt removeOffset, const UnsignedInt removeSize, const UnsignedInt insertOffset, const Containers::StringView insertText, const UnsignedInt cursor, const UnsignedInt selection) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::TextLayer::updateText(): invalid handle" << handle, );
    updateTextInternal(dataHandleId(handle), removeOffset, removeSize, insertOffset, insertText, cursor, selection);
}

void TextLayer::updateText(const LayerDataHandle handle, const UnsignedInt removeOffset, const UnsignedInt removeSize, const UnsignedInt insertOffset, const Containers::StringView insertText, const UnsignedInt cursor, const UnsignedInt selection) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::TextLayer::updateText(): invalid handle" << handle, );
    updateTextInternal(layerDataHandleId(handle), removeOffset, removeSize, insertOffset, insertText, cursor, selection);
}

void TextLayer::updateTextInternal(const UnsignedInt id, const UnsignedInt removeOffset, const UnsignedInt removeSize, const UnsignedInt insertOffset, const Containers::StringView insertText, const UnsignedInt cursor, const UnsignedInt selection) {
    State& state = static_cast<State&>(*_state);
    Implementation::TextLayerData& data = state.data[id];
    CORRADE_ASSERT(data.textRun != ~UnsignedInt{},
        "Ui::TextLayer::updateText(): text doesn't have" << TextDataFlag::Editable << "set", );

    /* Getting a copy of the previous run and not a reference, as the textRuns
       array is possibly getting reallocated below */
    const Implementation::TextLayerTextRun previousRun = state.textRuns[data.textRun];
    /* Not `removeOffset + removeSize <= previousRun.textSize` as that could
       overflow and pass the check, for example if garbage memory is passed */
    CORRADE_ASSERT(removeOffset <= previousRun.textSize && removeSize <= previousRun.textSize - removeOffset,
        "Ui::TextLayer::updateText(): remove offset" << removeOffset << "and size" << removeSize << "out of range for a text of" << previousRun.textSize << "bytes", );
    const UnsignedInt textSizeBeforeInsert = previousRun.textSize - removeSize;
    CORRADE_ASSERT(insertOffset <= textSizeBeforeInsert,
        "Ui::TextLayer::updateText(): insert offset" << insertOffset << "out of range for a text of" << textSizeBeforeInsert << "bytes", );
    const UnsignedInt textSize = textSizeBeforeInsert + insertText.size();
    CORRADE_ASSERT(cursor <= textSize,
        "Ui::TextLayer::updateText(): cursor position" << cursor << "out of range for a text of" << textSize << "bytes", );
    CORRADE_ASSERT(selection <= textSize,
        "Ui::TextLayer::updateText(): selection position" << selection << "out of range for a text of" << textSize << "bytes", );

    /* If there's nothing to remove or insert, update just the cursor and
       bail */
    if(!removeSize && !insertText) {
        setCursorInternal(id, cursor, selection);
        return;
    }

    /* Check if the text is a slice of our internal text array (i.e., coming
       from another widget, possibly). In that case we'll have to relocate the
       view when we copy() it below. Other cases in create() and setText() are
       handled by the relocating logic in arrayAppend() directly, but as here
       the growing and copying is decoupled, we have to handle it directly.

       Checking against the capacity and not size for consistency with
       arrayAppend(), see comments in its implementation for details why. */
    std::size_t insertTextRelocateOffset = std::size_t(insertText.data() - state.textData.data());
    if(insertTextRelocateOffset >= arrayCapacity(state.textData))
        insertTextRelocateOffset = ~std::size_t{};

    /* Add a new text run for the modified contents */
    const UnsignedInt textRun = state.textRuns.size();
    const UnsignedInt textOffset = state.textData.size();
    Containers::ArrayView<char> text = arrayAppend(state.textData, NoInit, textSize);
    Implementation::TextLayerTextRun& run = arrayAppend(state.textRuns, NoInit, 1).front();

    /* Fill the new run properties */
    run.textOffset = textOffset;
    run.textSize = textSize;
    run.data = id;
    /* run.cursor updated by setCursorInternal() at the end */

    /* Copy the TextProperties internals verbatim */
    Utility::copy(previousRun.language, run.language);
    run.script = previousRun.script;
    run.font = previousRun.font;
    run.alignment = previousRun.alignment;
    run.direction = previousRun.direction;

    /* We can insert either before the removed range, in which case the copy
       before the removed range has to be split */
    UnsignedInt copySrcBegin[3];
    UnsignedInt copyDstBegin[3];
    UnsignedInt copySrcEnd[3];
    copySrcBegin[0] = 0;
    copyDstBegin[0] = 0;
    if(insertOffset < removeOffset) {
        copySrcEnd[0] = insertOffset;

        copySrcBegin[1] = insertOffset;
        copyDstBegin[1] = insertOffset + insertText.size();
        copySrcEnd[1] = removeOffset;

        copySrcBegin[2] = removeOffset + removeSize;
        copyDstBegin[2] = removeOffset + insertText.size();

    /* Or insert after the removed range, in which case the copy after the
       removed range has to be split (and the offsets there include the removed
       size as well because the source doesn't have it removed yet) */
    } else {
        copySrcEnd[0] = removeOffset;

        copySrcBegin[1] = removeOffset + removeSize;
        copyDstBegin[1] = removeOffset;
        copySrcEnd[1] = removeSize + insertOffset;

        copySrcBegin[2] = removeSize + insertOffset;
        copyDstBegin[2] = insertOffset + insertText.size();
    }
    copySrcEnd[2] = previousRun.textSize;

    /* Copy the bits of the previous text, if not empty */
    const Containers::StringView previousText = state.textData.sliceSize(previousRun.textOffset, previousRun.textSize);
    for(std::size_t i: {0, 1, 2}) {
        const UnsignedInt size = copySrcEnd[i] - copySrcBegin[i];
        if(size) Utility::copy(
            previousText.slice(copySrcBegin[i], copySrcEnd[i]),
            text.sliceSize(copyDstBegin[i], size));
    }

    /* Copy the inserted text, if not empty */
    if(insertText) {
        Utility::copy(
            /* If text to insert was a slice of our textData array, relocate
               the view relative to the (potentially) reallocated array */
            insertTextRelocateOffset != ~std::size_t{} ? state.textData.sliceSize(insertTextRelocateOffset, insertText.size())
                : insertText,
            text.sliceSize(insertOffset, insertText.size()));
    }

    /* Mark the previous run (potentially reallocated somewhere) as unused.
       It'll be removed during the next recompaction run in doUpdate(). Save
       the new run reference. */
    state.textRuns[data.textRun].textOffset = ~UnsignedInt{};
    data.textRun = textRun;

    /* Shape the new text using properties saved in the run and mark the layer
       as needing an update. Forming a TextProperties from the internal state
       that was saved earlier in shapeRememberTextInternal() above. */
    TextProperties properties{NoInit};
    Utility::copy(run.language, properties._language);
    properties._script = run.script;
    /* The font is passed through an argument, shouldn't be taken from here */
    properties._font = FontHandle::Null;
    /* The saved alignment has a special value denoting NullOpt, so just
       verbatinm copying it back */
    properties._alignment = run.alignment;
    /* Similarly, the direction is both the layout and shape directions
       together, verbatim copy them back */
    properties._direction = run.direction;
    shapeTextInternal(id, data.style, text, properties, run.font, data.flags);

    /* Update the cursor position and all related state */
    setCursorInternal(id, cursor, selection);

    setNeedsUpdate(LayerState::NeedsDataUpdate);
}

void TextLayer::editText(const DataHandle handle, const TextEdit edit, const Containers::StringView insert) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::TextLayer::editText(): invalid handle" << handle, );
    return editTextInternal(dataHandleId(handle), edit, insert);
}

void TextLayer::editText(const LayerDataHandle handle, const TextEdit edit, const Containers::StringView insert) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::TextLayer::editText(): invalid handle" << handle, );
    return editTextInternal(layerDataHandleId(handle), edit, insert);
}

void TextLayer::editTextInternal(const UnsignedInt id, const TextEdit edit, const Containers::StringView insert) {
    CORRADE_ASSERT(!insert ||
        edit == TextEdit::InsertBeforeCursor ||
        edit == TextEdit::InsertAfterCursor,
        "Ui::TextLayer::editText():" << edit << "requires no text to insert", );

    State& state = static_cast<State&>(*_state);
    const Implementation::TextLayerData& data = state.data[id];
    CORRADE_ASSERT(data.textRun != ~UnsignedInt{},
        "Ui::TextLayer::editText(): text doesn't have" << TextDataFlag::Editable << "set", );

    Implementation::TextLayerTextRun& run = state.textRuns[data.textRun];
    const Containers::StringView text = state.textData.sliceSize(run.textOffset, run.textSize);

    /* Simple cursor / selection movement, delegate to setCursor() */
    if(edit == TextEdit::MoveCursorLineBegin ||
       edit == TextEdit::MoveCursorLineEnd ||
       edit == TextEdit::MoveCursorLeft ||
       edit == TextEdit::MoveCursorRight ||
       edit == TextEdit::ExtendSelectionLineBegin ||
       edit == TextEdit::ExtendSelectionLineEnd ||
       edit == TextEdit::ExtendSelectionLeft ||
       edit == TextEdit::ExtendSelectionRight)
    {
        UnsignedInt cursor = run.cursor;

        /* Line begin / end movement has no special-casing for RTL direction
           -- it moves at the begin/end of the byte stream in both cases and
           differs only optically */
        if(edit == TextEdit::MoveCursorLineBegin ||
           edit == TextEdit::ExtendSelectionLineBegin)
            cursor = 0;
        else if(edit == TextEdit::MoveCursorLineEnd ||
                edit == TextEdit::ExtendSelectionLineEnd)
            cursor = run.textSize;
        /* Cursor left / right movement has special-casing for RTL tho, the
           intent is for movement left to always go left, and not right, and
           vice versa */
        else if(run.cursor > 0 && (
                ((edit == TextEdit::MoveCursorLeft ||
                  edit == TextEdit::ExtendSelectionLeft)
                    && data.usedDirection != Text::ShapeDirection::RightToLeft) ||
                ((edit == TextEdit::MoveCursorRight ||
                  edit == TextEdit::ExtendSelectionRight)
                    && data.usedDirection == Text::ShapeDirection::RightToLeft)))
            cursor = Utility::Unicode::prevChar(text, run.cursor).second();
        else if(run.cursor < run.textSize && (
                ((edit == TextEdit::MoveCursorRight ||
                  edit == TextEdit::ExtendSelectionRight)
                    && data.usedDirection != Text::ShapeDirection::RightToLeft) ||
                ((edit == TextEdit::MoveCursorLeft ||
                  edit == TextEdit::ExtendSelectionLeft)
                    && data.usedDirection == Text::ShapeDirection::RightToLeft)))
            cursor = Utility::Unicode::nextChar(text, run.cursor).second();

        /* If we're extending the selection, the other end of it stays,
           otherwise the selection gets reset by setting both to the same
           value */
        UnsignedInt selection;
        if(edit == TextEdit::ExtendSelectionLineBegin ||
           edit == TextEdit::ExtendSelectionLineEnd ||
           edit == TextEdit::ExtendSelectionLeft ||
           edit == TextEdit::ExtendSelectionRight)
            selection = run.selection;
        else
            selection = cursor;

        /* The function takes care of updating all needed data, LayerState etc
           if the cursor position / selection actually changes */
        setCursorInternal(id, cursor, selection);

    /* Text removal & insertion with cursor adjustment */
    } else if(edit == TextEdit::RemoveBeforeCursor ||
              edit == TextEdit::RemoveAfterCursor ||
              edit == TextEdit::InsertBeforeCursor ||
              edit == TextEdit::InsertAfterCursor)
    {
        UnsignedInt removeOffset = 0;
        UnsignedInt removeSize = 0;
        UnsignedInt insertOffset = 0;
        UnsignedInt cursor = run.cursor;

        /* No selection active */
        if(run.cursor == run.selection) {
            /* Insertion has no special-casing for RTL -- it just inserts the
               data at the place of the cursor and then either moves the cursor
               after the inserted bytes or leaves it where it was, the
               difference is only optical */
            if(edit == TextEdit::InsertBeforeCursor) {
                insertOffset = run.cursor;
                cursor = run.cursor + insert.size();
            } else if(edit == TextEdit::InsertAfterCursor) {
                insertOffset = run.cursor;
                cursor = run.cursor;
            /* Deletion as well -- the difference is only optical, and compared
               to left/right arrow keys the backspace and delete keys don't
               have any implicit optical direction in the name that would need
               matching */
            } else if(edit == TextEdit::RemoveBeforeCursor && run.cursor > 0) {
                removeOffset = Utility::Unicode::prevChar(text, run.cursor).second();
                removeSize = run.cursor - removeOffset;
                cursor = removeOffset;
            } else if(edit == TextEdit::RemoveAfterCursor && run.cursor < run.textSize) {
                removeOffset = run.cursor;
                removeSize = Utility::Unicode::nextChar(text, run.cursor).second() - run.cursor;
                cursor = run.cursor;
            }

        /* With selection active it replaces it */
        } else {
            const Containers::Pair<UnsignedInt, UnsignedInt> selection = Math::minmax(run.cursor, run.selection);
            removeOffset = selection.first();
            removeSize = selection.second() - selection.first();

            /* The rest is like above, just that the cursor is now always at
               the now-removed-selection start, no matter whether the cursor
               was at the beginning or end of the selection originally */
            if(edit == TextEdit::InsertBeforeCursor) {
                insertOffset = removeOffset;
                cursor = removeOffset + insert.size();
            } else if(edit == TextEdit::InsertAfterCursor) {
                insertOffset = removeOffset;
                cursor = removeOffset;
            } else if(edit == TextEdit::RemoveBeforeCursor ||
                      edit == TextEdit::RemoveAfterCursor) {
                cursor = removeOffset;
            }
        }

        /* All edit operations discard the selection */
        updateTextInternal(id, removeOffset, removeSize, insertOffset, insert, cursor, cursor);

    } else CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
}

void TextLayer::setGlyph(const DataHandle handle, const UnsignedInt glyph, const TextProperties& properties) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::TextLayer::setGlyph(): invalid handle" << handle, );
    setGlyphInternal(dataHandleId(handle), glyph, properties);
}

void TextLayer::setGlyph(const LayerDataHandle handle, const UnsignedInt glyph, const TextProperties& properties) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::TextLayer::setGlyph(): invalid handle" << handle, );
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

    /* If there's a text run, mark it as unused as well; it'll be removed in
       doUpdate() too */
    if(state.data[id].textRun != ~UnsignedInt{})
        state.textRuns[state.data[id].textRun].textOffset = ~UnsignedInt{};

    /* Shape the glyph, mark the layer as needing an update */
    shapeGlyphInternal(
        #ifndef CORRADE_NO_ASSERT
        "Ui::TextLayer::setGlyph():",
        #endif
        id, data.style, glyph, properties);
    setNeedsUpdate(LayerState::NeedsDataUpdate);
}

Color3 TextLayer::color(const DataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::TextLayer::color(): invalid handle" << handle, {});
    return static_cast<const State&>(*_state).data[dataHandleId(handle)].color;
}

Color3 TextLayer::color(const LayerDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::TextLayer::color(): invalid handle" << handle, {});
    return static_cast<const State&>(*_state).data[layerDataHandleId(handle)].color;
}

void TextLayer::setColor(const DataHandle handle, const Color3& color) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::TextLayer::setColor(): invalid handle" << handle, );
    setColorInternal(dataHandleId(handle), color);
}

void TextLayer::setColor(const LayerDataHandle handle, const Color3& color) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::TextLayer::setColor(): invalid handle" << handle, );
    setColorInternal(layerDataHandleId(handle), color);
}

void TextLayer::setColorInternal(const UnsignedInt id, const Color3& color) {
    static_cast<State&>(*_state).data[id].color = color;
    setNeedsUpdate(LayerState::NeedsDataUpdate);
}

Vector4 TextLayer::padding(const DataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::TextLayer::padding(): invalid handle" << handle, {});
    return static_cast<const State&>(*_state).data[dataHandleId(handle)].padding;
}

Vector4 TextLayer::padding(const LayerDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::TextLayer::padding(): invalid handle" << handle, {});
    return static_cast<const State&>(*_state).data[layerDataHandleId(handle)].padding;
}

void TextLayer::setPadding(const DataHandle handle, const Vector4& padding) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::TextLayer::setPadding(): invalid handle" << handle, );
    setPaddingInternal(dataHandleId(handle), padding);
}

void TextLayer::setPadding(const LayerDataHandle handle, const Vector4& padding) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::TextLayer::setPadding(): invalid handle" << handle, );
    setPaddingInternal(layerDataHandleId(handle), padding);
}

void TextLayer::setPaddingInternal(const UnsignedInt id, const Vector4& padding) {
    static_cast<State&>(*_state).data[id].padding = padding;
    setNeedsUpdate(LayerState::NeedsDataUpdate);
}

LayerFeatures TextLayer::doFeatures() const {
    return AbstractVisualLayer::doFeatures()|(static_cast<const Shared::State&>(_state->shared).dynamicStyleCount ? LayerFeature::AnimateStyles : LayerFeatures{})|LayerFeature::Draw;
}

LayerStates TextLayer::doState() const {
    LayerStates states = AbstractVisualLayer::doState();

    auto& state = static_cast<const State&>(*_state);
    auto& sharedState = static_cast<const Shared::State&>(state.shared);
    if(state.styleUpdateStamp != sharedState.styleUpdateStamp ||
       state.editingStyleUpdateStamp != sharedState.editingStyleUpdateStamp)
    {
        /* Needed because uniform mapping and paddings can change */
        states |= LayerState::NeedsDataUpdate;
        /* If there are dynamic styles, each layer also needs to upload the
           style uniform buffer */
        if(sharedState.dynamicStyleCount)
            states |= LayerState::NeedsCommonDataUpdate;
    }
    return states;
}

void TextLayer::doClean(const Containers::BitArrayView dataIdsToRemove) {
    State& state = static_cast<State&>(*_state);

    /* Mark glyph / text runs attached to removed data as unused. They'll get
       removed during the next recompaction in doUpdate(). */
    /** @todo some way to iterate set bits */
    for(std::size_t i = 0; i != dataIdsToRemove.size(); ++i) {
        if(!dataIdsToRemove[i])
            continue;

        state.glyphRuns[state.data[i].glyphRun].glyphOffset = ~UnsignedInt{};
        if(state.data[i].textRun != ~UnsignedInt{})
            state.textRuns[state.data[i].textRun].textOffset = ~UnsignedInt{};
    }

    /* Data removal doesn't need anything to be reuploaded to continue working
       correctly, thus setNeedsUpdate() isn't called, and neither is in
       remove(). See a comment there for more information. */
}

void TextLayer::doAdvanceAnimations(const Nanoseconds time, const Containers::MutableBitArrayView activeStorage, const Containers::StridedArrayView1D<Float>& factorStorage, const Containers::MutableBitArrayView removeStorage, const Containers::Iterable<AbstractStyleAnimator>& animators) {
    auto& state = static_cast<State&>(*_state);

    TextLayerStyleAnimations animations;
    for(AbstractStyleAnimator& animator: animators) {
        if(!(animator.state() >= AnimatorState::NeedsAdvance))
            continue;

        const std::size_t capacity = animator.capacity();
        const Containers::Pair<bool, bool> needsAdvanceClean = animator.update(time,
            activeStorage.prefix(capacity),
            factorStorage.prefix(capacity),
            removeStorage.prefix(capacity));

        if(needsAdvanceClean.first())
            animations |= static_cast<TextLayerStyleAnimator&>(animator).advance(
                activeStorage.prefix(capacity),
                factorStorage.prefix(capacity),
                removeStorage.prefix(capacity),
                state.dynamicStyleUniforms,
                state.dynamicStyleCursorStyles,
                state.dynamicStyleSelectionStyles,
                stridedArrayView(state.dynamicStyles).slice(&Implementation::TextLayerDynamicStyle::padding),
                state.dynamicEditingStyleUniforms,
                state.dynamicEditingStylePaddings,
                stridedArrayView(state.data).slice(&Implementation::TextLayerData::style));
        if(needsAdvanceClean.second())
            animator.clean(removeStorage.prefix(capacity));
    }

    if(animations & (TextLayerStyleAnimation::Style|TextLayerStyleAnimation::Padding|TextLayerStyleAnimation::EditingPadding))
        setNeedsUpdate(LayerState::NeedsDataUpdate);
    if(animations >= TextLayerStyleAnimation::Uniform) {
        setNeedsUpdate(LayerState::NeedsCommonDataUpdate);
        state.dynamicStyleChanged = true;
    }
    if(animations >= TextLayerStyleAnimation::EditingUniform) {
        setNeedsUpdate(LayerState::NeedsCommonDataUpdate);
        state.dynamicEditingStyleChanged = true;
    }
}

void TextLayer::doUpdate(const LayerStates states, const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, const Containers::BitArrayView nodesEnabled, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes, const Containers::StridedArrayView1D<const Vector2>& compositeRectOffsets, const Containers::StridedArrayView1D<const Vector2>& compositeRectSizes) {
    /* The base implementation populates data.calculatedStyle */
    AbstractVisualLayer::doUpdate(states, dataIds, clipRectIds, clipRectDataCounts, nodeOffsets, nodeSizes, nodesEnabled, clipRectOffsets, clipRectSizes, compositeRectOffsets, compositeRectSizes);

    State& state = static_cast<State&>(*_state);
    Shared::State& sharedState = static_cast<Shared::State&>(state.shared);
    /* Technically needed only if there's any actual data to update, but
       require it always for consistency (and easier testing) */
    CORRADE_ASSERT(sharedState.setStyleCalled,
        "Ui::TextLayer::update(): no style data was set", );
    /* Also, technically needed only if there's any actual editable data with
       editing styles to update, but require it always for consistency */
    CORRADE_ASSERT(!sharedState.hasEditingStyles || sharedState.setEditingStyleCalled,
        "Ui::TextLayer::update(): no editing style data was set", );

    /* Recompact the glyph / text data by removing unused runs. Do this only if
       data actually change, this isn't affected by anything node-related */
    /** @todo further restrict this to just NeedsCommonDataUpdate which gets
        set by setText(), remove() etc that actually produces unused runs, but
        not setColor() and such? the recompaction however implies a need to
        update the actual index buffer etc anyway, so a dedicated state won't
        make that update any smaller, and we'd now trigger it from clean() and
        remove() as well, which we didn't need to before */
    if(states >= LayerState::NeedsDataUpdate) {
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
    /* Another scope to avoid accidental variable reuse, flattening it to avoid
       excessive indentation */
    if(states & LayerState::NeedsDataUpdate) {
        std::size_t outputTextDataOffset = 0;
        std::size_t outputTextRunOffset = 0;
        for(std::size_t i = 0; i != state.textRuns.size(); ++i) {
            Implementation::TextLayerTextRun& run = state.textRuns[i];
            if(run.textOffset == ~UnsignedInt{})
                continue;

            /* Move the text data earlier if there were skipped runs before,
               update the reference to it in the run */
            if(run.textOffset != outputTextDataOffset) {
                CORRADE_INTERNAL_DEBUG_ASSERT(run.textOffset > outputTextDataOffset);
                CORRADE_INTERNAL_DEBUG_ASSERT(i != outputTextRunOffset);

                std::memmove(state.textData.data() + outputTextDataOffset,
                             state.textData.data() + run.textOffset,
                             run.textSize);
                run.textOffset = outputTextDataOffset;
            }
            outputTextDataOffset += run.textSize;

            /* Move the text run info earlier if there were skipped runs
               before, update the reference to it in the data */
            if(i != outputTextRunOffset) {
                CORRADE_INTERNAL_DEBUG_ASSERT(i > outputTextRunOffset);
                CORRADE_INTERNAL_DEBUG_ASSERT(state.data[run.data].textRun != ~UnsignedInt{});
                state.data[run.data].textRun = outputTextRunOffset;
                state.textRuns[outputTextRunOffset] = run;
            }
            ++outputTextRunOffset;
        }

        /* Remove the now-unused data from the end */
        CORRADE_INTERNAL_ASSERT(outputTextDataOffset <= state.textData.size());
        CORRADE_INTERNAL_ASSERT(outputTextRunOffset <= state.textRuns.size());
        arrayResize(state.textData, outputTextDataOffset);
        arrayResize(state.textRuns, outputTextRunOffset);
    }

    /* Fill in indices in desired order if either the data themselves or the
       node order changed */
    if(states >= LayerState::NeedsNodeOrderUpdate ||
       states >= LayerState::NeedsDataUpdate)
    {
        /* Index offsets for each run, plus one more for the last run */
        arrayResize(state.indexDrawOffsets, NoInit, dataIds.size() + 1);

        /* Calculate how many glyphs we'll draw, and how many cursor and
           selection rectangles */
        UnsignedInt drawGlyphCount = 0;
        UnsignedInt drawEditingRectCount = 0;
        for(const UnsignedInt id: dataIds) {
            const Implementation::TextLayerData& data = state.data[id];
            const Implementation::TextLayerGlyphRun& glyphRun = state.glyphRuns[data.glyphRun];
            drawGlyphCount += glyphRun.glyphCount;
            if(data.textRun != ~UnsignedInt{}) {
                Int cursorStyle, selectionStyle;
                /** @todo ugh, this is duplicated three times */
                if(data.calculatedStyle < sharedState.styleCount) {
                    const Implementation::TextLayerStyle& style = sharedState.styles[data.calculatedStyle];
                    cursorStyle = style.cursorStyle;
                    selectionStyle = style.selectionStyle;
                } else {
                    CORRADE_INTERNAL_DEBUG_ASSERT(data.calculatedStyle < sharedState.styleCount + sharedState.dynamicStyleCount);
                    const UnsignedInt dynamicStyleId = data.calculatedStyle - sharedState.styleCount;
                    cursorStyle = state.dynamicStyleCursorStyles[dynamicStyleId] ? Implementation::cursorStyleForDynamicStyle(dynamicStyleId) : -1;
                    selectionStyle = state.dynamicStyleSelectionStyles[dynamicStyleId] ? Implementation::selectionStyleForDynamicStyle(dynamicStyleId) : -1;
                }
                const Implementation::TextLayerTextRun& textRun = state.textRuns[data.textRun];
                if(selectionStyle != -1 && textRun.selection != textRun.cursor)
                    ++drawEditingRectCount;
                if(cursorStyle != -1)
                    ++drawEditingRectCount;
            }
        }

        /* Generate index data */
        arrayResize(state.indices, NoInit, drawGlyphCount*6);
        arrayResize(state.editingIndices, NoInit, drawEditingRectCount*6);
        UnsignedInt indexOffset = 0;
        UnsignedInt editingRectOffset = 0;
        for(std::size_t i = 0; i != dataIds.size(); ++i) {
            const Implementation::TextLayerData& data = state.data[dataIds[i]];
            const Implementation::TextLayerGlyphRun& glyphRun = state.glyphRuns[data.glyphRun];

            /* Generate indices in draw order. Remeber the offset for each data
               to draw from later. */
            state.indexDrawOffsets[i] = {indexOffset, editingRectOffset*6};
            const Containers::ArrayView<UnsignedInt> indexData = state.indices.sliceSize(indexOffset, glyphRun.glyphCount*6);
            Text::renderGlyphQuadIndicesInto(glyphRun.glyphOffset, indexData);
            indexOffset += indexData.size();

            /* If the text is editable, generate indices for cursor and
               selection as well. They're currently both drawn in the same
               call, with selection first and cursor on top. This may
               eventually get split for the cursor to be drawn as an inverse
               rectangle like is usual in editors, on the other hand the style
               can fine-tune the look for each text style so that doesn't seem
               really important. */
            if(data.textRun != ~UnsignedInt{}) {
                Int cursorStyle, selectionStyle;
                /** @todo ugh, this is duplicated three times */
                if(data.calculatedStyle < sharedState.styleCount) {
                    const Implementation::TextLayerStyle& style = sharedState.styles[data.calculatedStyle];
                    cursorStyle = style.cursorStyle;
                    selectionStyle = style.selectionStyle;
                } else {
                    CORRADE_INTERNAL_DEBUG_ASSERT(data.calculatedStyle < sharedState.styleCount + sharedState.dynamicStyleCount);
                    const UnsignedInt dynamicStyleId = data.calculatedStyle - sharedState.styleCount;
                    cursorStyle = state.dynamicStyleCursorStyles[dynamicStyleId] ? Implementation::cursorStyleForDynamicStyle(dynamicStyleId) : -1;
                    selectionStyle = state.dynamicStyleSelectionStyles[dynamicStyleId] ? Implementation::selectionStyleForDynamicStyle(dynamicStyleId) : -1;
                }
                const Implementation::TextLayerTextRun& textRun = state.textRuns[data.textRun];
                const auto createEditingQuadIndices = [&state, &editingRectOffset](const UnsignedInt vertexOffset) {
                    UnsignedInt indexOffset = editingRectOffset*6;

                    /* The index order matches BaseLayer, not
                       Text::renderGlyphQuadIndices(), as there it involves
                       also flipping from Y up to Y down

                       0---1 0---2 5
                       |   | |  / /|
                       |   | | / / |
                       |   | |/ /  |
                       2---3 1 3---4 */
                    state.editingIndices[indexOffset++] = vertexOffset + 0;
                    state.editingIndices[indexOffset++] = vertexOffset + 2;
                    state.editingIndices[indexOffset++] = vertexOffset + 1;
                    state.editingIndices[indexOffset++] = vertexOffset + 2;
                    state.editingIndices[indexOffset++] = vertexOffset + 3;
                    state.editingIndices[indexOffset++] = vertexOffset + 1;

                    ++editingRectOffset;
                };

                /* The selection is shown only if there's a style for it and
                   something is actually selected, and is drawn first */
                if(selectionStyle != -1 && textRun.selection != textRun.cursor)
                    createEditingQuadIndices(data.textRun*8);
                /* The cursor only if there's a style for it, and is drawn
                   after the selection */
                if(cursorStyle != -1)
                    createEditingQuadIndices(data.textRun*8 + 4);
            }
        }

        CORRADE_INTERNAL_ASSERT(indexOffset == drawGlyphCount*6);
        CORRADE_INTERNAL_ASSERT(editingRectOffset == drawEditingRectCount);
        state.indexDrawOffsets[dataIds.size()] = {indexOffset, editingRectOffset*6};
    }

    /* Fill in vertex data if the data themselves, the node offset/size or node
       enablement (and thus calculated styles) changed */
    /** @todo split this further to just position-related data update and other
        data if it shows to help with perf */
    if(states >= LayerState::NeedsNodeOffsetSizeUpdate ||
       states >= LayerState::NeedsNodeEnabledUpdate ||
       states >= LayerState::NeedsDataUpdate)
    {
        /* Calculate how many glyphs there are in total */
        UnsignedInt totalGlyphCount = 0;
        for(const Implementation::TextLayerGlyphRun& run: state.glyphRuns)
            totalGlyphCount += run.glyphCount;

        const Containers::StridedArrayView1D<const Ui::NodeHandle> nodes = this->nodes();

        /* Generate vertex data */
        arrayResize(state.vertices, NoInit, totalGlyphCount*4);
        if(sharedState.hasEditingStyles)
            arrayResize(state.editingVertices, NoInit, state.textRuns.size()*2*4);
        for(const UnsignedInt dataId: dataIds) {
            const UnsignedInt nodeId = nodeHandleId(nodes[dataId]);
            const Implementation::TextLayerData& data = state.data[dataId];
            const Implementation::TextLayerGlyphRun& glyphRun = state.glyphRuns[data.glyphRun];

            /* Fill in quad vertices in the same order as the original text
               runs */
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
            Vector4 padding = data.padding;
            if(data.calculatedStyle < sharedState.styleCount)
                padding += sharedState.styles[data.calculatedStyle].padding;
            else {
                CORRADE_INTERNAL_DEBUG_ASSERT(data.calculatedStyle < sharedState.styleCount + sharedState.dynamicStyleCount);
                padding += state.dynamicStyles[data.calculatedStyle - sharedState.styleCount].padding;
            }
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
            /* For Line/Middle it's aligning either the line or bounding box
               middle (which is already at y=0 by the Text::alignRenderedLine())
               to node middle */
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
                /* For dynamic styles the uniform mapping is implicit and they're
                   placed right after all non-dynamic styles */
                vertex.styleUniform = data.calculatedStyle < sharedState.styleCount ?
                    sharedState.styles[data.calculatedStyle].uniform :
                    sharedState.styleUniformCount + data.calculatedStyle - sharedState.styleCount;
            }

            /* If the text is editable, generate also the cursor and selection
               mesh, unless they don't have any style */
            if(data.textRun != ~UnsignedInt{}) {
                Int cursorStyle, selectionStyle;
                /** @todo ugh, this is duplicated three times */
                if(data.calculatedStyle < sharedState.styleCount) {
                    const Implementation::TextLayerStyle& style = sharedState.styles[data.calculatedStyle];
                    cursorStyle = style.cursorStyle;
                    selectionStyle = style.selectionStyle;
                } else {
                    CORRADE_INTERNAL_DEBUG_ASSERT(data.calculatedStyle < sharedState.styleCount + sharedState.dynamicStyleCount);
                    const UnsignedInt dynamicStyleId = data.calculatedStyle - sharedState.styleCount;
                    cursorStyle = state.dynamicStyleCursorStyles[dynamicStyleId] ? Implementation::cursorStyleForDynamicStyle(dynamicStyleId) : -1;
                    selectionStyle = state.dynamicStyleSelectionStyles[dynamicStyleId] ? Implementation::selectionStyleForDynamicStyle(dynamicStyleId) : -1;
                }
                const Implementation::TextLayerTextRun& textRun = state.textRuns[data.textRun];
                const Containers::Pair<UnsignedInt, UnsignedInt> glyphRangeForCursorSelection = Text::glyphRangeForBytes(glyphData.slice(&Implementation::TextLayerGlyphData::glyphCluster), textRun.cursor, textRun.selection);

                /* The rectangle is Y-up, which means the max() is the top and
                   we need to subtract it from the offset, and min() is bottom,
                   negative, and thus we need to subtract it also */
                /** @todo use the other coordinate if the shape direction is
                    vertical */
                const Vector2 lineBottom = offset - Vector2::yAxis(data.rectangle.min().y());
                const Vector2 lineTop = offset - Vector2::yAxis(data.rectangle.max().y());
                const auto cursorPositionForGlyph = [&data, &glyphData](const UnsignedInt glyph) {
                    /** @todo The glyph position includes also the additional
                        shaper offset, which isn't desirable for cursor
                        placement. Often it's just 0, but sometimes it could be
                        different e.g. for diacritics placement, and then the
                        cursor could be weirdly shifted. Ideally the offset
                        would be stored separately and not included here, but
                        that's one extra float per glyph :/ The Y offset is
                        already ignored as only the X is taken. */
                    return Vector2::xAxis(glyph == glyphData.size() ?
                        data.rectangle.max().x() : glyphData[glyph].position.x());
                };
                const auto createEditingQuad = [&state, &sharedState, &lineTop, &lineBottom, &cursorPositionForGlyph, &vertexData](const bool dynamicEditingStyle, const UnsignedInt editingStyleId, const UnsignedInt glyphBegin, const UnsignedInt glyphEnd, const UnsignedInt vertexOffset, Text::ShapeDirection direction) {
                    Vector4 padding{NoInit};
                    UnsignedInt uniform;
                    Int textUniform;
                    if(!dynamicEditingStyle) {
                        CORRADE_INTERNAL_DEBUG_ASSERT(editingStyleId < sharedState.editingStyles.size());
                        const Implementation::TextLayerEditingStyle& editingStyle = sharedState.editingStyles[editingStyleId];
                        padding = editingStyle.padding;
                        uniform = editingStyle.uniform;
                        textUniform = editingStyle.textUniform;
                    } else {
                        CORRADE_INTERNAL_DEBUG_ASSERT(editingStyleId < sharedState.dynamicStyleCount*2);
                        /* Contrary to data.calculatedStyle, dynamic
                           editingStyleId doesn't have any extra offset because
                           it's never controlled from outside where it could
                           get mixed up with static styles */
                        padding = state.dynamicEditingStylePaddings[editingStyleId];
                        /* Thus it's ID is also directly the uniform index
                           *after* static styles */
                        uniform = sharedState.editingStyleUniformCount + editingStyleId;
                        /* And the text uniform also points after static
                           styles */
                        textUniform = sharedState.styleUniformCount + Implementation::textUniformForEditingStyle(sharedState.dynamicStyleCount, editingStyleId);
                    }

                    /* LTR text interprets padding as left, top, right, bottom,
                       RTL as right, top, left, bottom */
                    if(direction == Text::ShapeDirection::RightToLeft)
                        padding = Math::gather<'z', 'y', 'x', 'w'>(padding);

                    /* 0---1
                       |   |
                       |   |
                       |   |
                       2---3 */
                    const Vector2 min = lineTop + cursorPositionForGlyph(glyphBegin) - padding.xy();
                    const Vector2 max = lineBottom + cursorPositionForGlyph(glyphEnd) + Math::gather<'z', 'w'>(padding);
                    const Vector2 sizeHalf = (max - min)*0.5f;
                    const Vector2 sizeHalfNegative = -sizeHalf;

                    for(UnsignedByte j = 0; j != 4; ++j) {
                        Implementation::TextLayerEditingVertex& vertex = state.editingVertices[vertexOffset + j];

                        /* ✨ */
                        vertex.position = Math::lerp(min, max, BitVector2{j});
                        vertex.centerDistance = Math::lerp(sizeHalfNegative, sizeHalf, BitVector2{j});
                        vertex.styleUniform = uniform;
                    }

                    /* If the editing style has an override for the text
                       uniform, apply it to the selected range */
                    if(textUniform != -1) {
                        for(Implementation::TextLayerVertex& vertex: vertexData.slice(glyphBegin*4, glyphEnd*4))
                            vertex.styleUniform = textUniform;
                    }
                };

                /* Create a selection quad, if it has a style and there's a
                   non-empty selection. It's drawn below the cursor, so it's
                   first in the vertex buffer for given run (and first in the
                   index buffer also). */
                if(selectionStyle != -1 && textRun.selection != textRun.cursor) {
                    const Containers::Pair<UnsignedInt, UnsignedInt> selection = Math::minmax(glyphRangeForCursorSelection.first(), glyphRangeForCursorSelection.second());
                    createEditingQuad(
                        data.calculatedStyle >= sharedState.styleCount,
                        selectionStyle,
                        selection.first(),
                        selection.second(),
                        data.textRun*2*4,
                        data.usedDirection);
                }
                /* Create a cursor quad, if it has a style. It's drawn on top
                   of the selection, so it's later in the vertex buffer for
                   given run (and later in index buffer also) */
                if(cursorStyle != -1) {
                    createEditingQuad(
                        data.calculatedStyle >= sharedState.styleCount,
                        cursorStyle,
                        glyphRangeForCursorSelection.first(),
                        glyphRangeForCursorSelection.first(),
                        data.textRun*2*4 + 4,
                        data.usedDirection);
                }
            }
        }
    }

    /* Sync the style update stamp to not have doState() return NeedsDataUpdate
       / NeedsCommonDataUpdate again next time it's asked */
    if(states >= LayerState::NeedsDataUpdate ||
       states >= LayerState::NeedsCommonDataUpdate) {
        state.styleUpdateStamp = sharedState.styleUpdateStamp;
        state.editingStyleUpdateStamp = sharedState.editingStyleUpdateStamp;
    }
}

void TextLayer::doKeyPressEvent(const UnsignedInt dataId, KeyEvent& event) {
    State& state = static_cast<State&>(*_state);
    Implementation::TextLayerData& data = state.data[dataId];
    if(!(data.flags >= TextDataFlag::Editable))
        return;

    /* Key events are implicitly passed also to nodes under cursor, restrict
       the editing and cursor movement to just when the node is focused to
       avoid strange behavior */
    if(event.isFocused()) {
        if(!event.modifiers()) {
            if(event.key() == Key::Backspace) {
                editTextInternal(dataId, TextEdit::RemoveBeforeCursor, {});
            } else if(event.key() == Key::Delete) {
                editTextInternal(dataId, TextEdit::RemoveAfterCursor, {});
            } else if(event.key() == Key::Home) {
                editTextInternal(dataId, TextEdit::MoveCursorLineBegin, {});
            } else if(event.key() == Key::End) {
                editTextInternal(dataId, TextEdit::MoveCursorLineEnd, {});
            } else if(event.key() == Key::Left) {
                editTextInternal(dataId, TextEdit::MoveCursorLeft, {});
            } else if(event.key() == Key::Right) {
                editTextInternal(dataId, TextEdit::MoveCursorRight, {});
            } else return;
        } else if(event.modifiers() == Modifier::Shift) {
            if(event.key() == Key::Home) {
                editTextInternal(dataId, TextEdit::ExtendSelectionLineBegin, {});
            } else if(event.key() == Key::End) {
                editTextInternal(dataId, TextEdit::ExtendSelectionLineEnd, {});
            } else if(event.key() == Key::Left) {
                editTextInternal(dataId, TextEdit::ExtendSelectionLeft, {});
            } else if(event.key() == Key::Right) {
                editTextInternal(dataId, TextEdit::ExtendSelectionRight, {});
            } else return;
        } else return;

        event.setAccepted();
    }
}

void TextLayer::doTextInputEvent(const UnsignedInt dataId, TextInputEvent& event) {
    State& state = static_cast<State&>(*_state);
    Implementation::TextLayerData& data = state.data[dataId];
    if(!(data.flags >= TextDataFlag::Editable))
        return;

    editTextInternal(dataId, TextEdit::InsertBeforeCursor, event.text());

    event.setAccepted();
}

}}
