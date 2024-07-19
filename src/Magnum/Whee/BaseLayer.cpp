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

#include "BaseLayer.h"

#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/BitArrayView.h>
#include <Corrade/Containers/EnumSet.hpp>
#include <Corrade/Containers/GrowableArray.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Utility/Algorithms.h>
#include <Magnum/Math/Functions.h>
#include <Magnum/Math/Matrix3.h>
#include <Magnum/Math/Swizzle.h>

#include "Magnum/Whee/Event.h"
#include "Magnum/Whee/Handle.h"
#include "Magnum/Whee/Implementation/baseLayerState.h"

namespace Magnum { namespace Whee {

Debug& operator<<(Debug& debug, const BaseLayer::Shared::Flag value) {
    debug << "Whee::BaseLayer::Shared::Flag" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case BaseLayer::Shared::Flag::value: return debug << "::" #value;
        _c(Textured)
        _c(BackgroundBlur)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "(" << Debug::nospace << Debug::hex << UnsignedByte(value) << Debug::nospace << ")";
}

Debug& operator<<(Debug& debug, const BaseLayer::Shared::Flags value) {
    return Containers::enumSetDebugOutput(debug, value, "Whee::BaseLayer::Shared::Flags{}", {
        BaseLayer::Shared::Flag::Textured,
        BaseLayer::Shared::Flag::BackgroundBlur
    });
}

BaseLayer::Shared::Shared(Containers::Pointer<State>&& state): AbstractVisualLayer::Shared{Utility::move(state)} {}

BaseLayer::Shared::Shared(const Configuration& configuration): Shared{Containers::pointer<State>(*this, configuration)} {}

BaseLayer::Shared::Shared(NoCreateT) noexcept: AbstractVisualLayer::Shared{NoCreate} {}

UnsignedInt BaseLayer::Shared::styleUniformCount() const {
    return static_cast<const State&>(*_state).styleUniformCount;
}

BaseLayer::Shared::Flags BaseLayer::Shared::flags() const {
    return static_cast<const State&>(*_state).flags;
}

void BaseLayer::Shared::setStyleInternal(const BaseLayerCommonStyleUniform& commonUniform, const Containers::ArrayView<const BaseLayerStyleUniform> uniforms, const Containers::StridedArrayView1D<const Vector4>& stylePaddings) {
    State& state = static_cast<State&>(*_state);
    /* Allocation done before the asserts so if they fail in a graceful assert
       build, we don't hit another assert in Utility::copy(styleToUniform) in
       the setStyle() below */
    if(state.styles.isEmpty())
        state.styles = Containers::Array<Implementation::BaseLayerStyle>{NoInit, state.styleCount};
    CORRADE_ASSERT(uniforms.size() == state.styleUniformCount,
        "Whee::BaseLayer::Shared::setStyle(): expected" << state.styleUniformCount << "uniforms, got" << uniforms.size(), );
    CORRADE_ASSERT(stylePaddings.isEmpty() || stylePaddings.size() == state.styleCount,
        "Whee::BaseLayer::Shared::setStyle(): expected either no or" << state.styleCount << "paddings, got" << stylePaddings.size(), );
    if(stylePaddings.isEmpty()) {
        /** @todo some Utility::fill() for this */
        for(Implementation::BaseLayerStyle& style: state.styles)
            style.padding = {};
    } else {
        Utility::copy(stylePaddings, stridedArrayView(state.styles).slice(&Implementation::BaseLayerStyle::padding));
    }
    doSetStyle(commonUniform, uniforms);
}

BaseLayer::Shared& BaseLayer::Shared::setStyle(const BaseLayerCommonStyleUniform& commonUniform, const Containers::ArrayView<const BaseLayerStyleUniform> uniforms, const Containers::StridedArrayView1D<const UnsignedInt>& styleToUniform, const Containers::StridedArrayView1D<const Vector4>& stylePaddings) {
    State& state = static_cast<State&>(*_state);
    CORRADE_ASSERT(styleToUniform.size() == state.styleCount,
        "Whee::BaseLayer::Shared::setStyle(): expected" << state.styleCount << "style uniform indices, got" << styleToUniform.size(), *this);
    setStyleInternal(commonUniform, uniforms, stylePaddings);
    Utility::copy(styleToUniform, stridedArrayView(state.styles).slice(&Implementation::BaseLayerStyle::uniform));
    return *this;
}

BaseLayer::Shared& BaseLayer::Shared::setStyle(const BaseLayerCommonStyleUniform& commonUniform, const std::initializer_list<BaseLayerStyleUniform> uniforms, const std::initializer_list<UnsignedInt> styleToUniform, const std::initializer_list<Vector4> stylePaddings) {
    return setStyle(commonUniform, Containers::arrayView(uniforms), Containers::stridedArrayView(styleToUniform), Containers::stridedArrayView(stylePaddings));
}

BaseLayer::Shared& BaseLayer::Shared::setStyle(const BaseLayerCommonStyleUniform& commonUniform, const Containers::ArrayView<const BaseLayerStyleUniform> uniforms, const Containers::StridedArrayView1D<const Vector4>& paddings) {
    State& state = static_cast<State&>(*_state);
    CORRADE_ASSERT(state.styleUniformCount == state.styleCount,
        "Whee::BaseLayer::Shared::setStyle(): there's" << state.styleUniformCount << "uniforms for" << state.styleCount << "styles, provide an explicit mapping", *this);
    setStyleInternal(commonUniform, uniforms, paddings);
    for(UnsignedInt i = 0; i != state.styleCount; ++i)
        state.styles[i].uniform = i;
    return *this;
}

BaseLayer::Shared& BaseLayer::Shared::setStyle(const BaseLayerCommonStyleUniform& commonUniform, const std::initializer_list<BaseLayerStyleUniform> uniforms, const std::initializer_list<Vector4> paddings) {
    return setStyle(commonUniform, Containers::arrayView(uniforms), Containers::arrayView(paddings));
}

BaseLayer::Shared::Configuration::Configuration(const UnsignedInt styleUniformCount, const UnsignedInt styleCount): _styleUniformCount{styleUniformCount}, _styleCount{styleCount} {
    CORRADE_ASSERT(styleUniformCount, "Whee::BaseLayer::Shared::Configuration: expected non-zero style uniform count", );
    CORRADE_ASSERT(styleCount, "Whee::BaseLayer::Shared::Configuration: expected non-zero style count", );
}

BaseLayer::Shared::Configuration& BaseLayer::Shared::Configuration::setBackgroundBlurRadius(const UnsignedInt radius, const Float cutoff) {
    CORRADE_ASSERT(radius < 32,
        "Whee::BaseLayer::Shared::Configuration::setBackgroundBlurRadius(): radius" << radius << "too large", *this);
    _backgroundBlurRadius = radius;
    _backgroundBlurCutoff = cutoff;
    return *this;
}

BaseLayer::BaseLayer(const LayerHandle handle, Containers::Pointer<State>&& state): AbstractVisualLayer{handle, Utility::move(state)} {}

BaseLayer::BaseLayer(const LayerHandle handle, Shared& shared): BaseLayer{handle, Containers::pointer<State>(static_cast<Shared::State&>(*shared._state))} {}

UnsignedInt BaseLayer::backgroundBlurPassCount() const {
    auto& state = static_cast<const State&>(*_state);
    #ifndef CORRADE_NO_ASSERT
    auto& sharedState = static_cast<const Shared::State&>(state.shared);
    #endif
    CORRADE_ASSERT(sharedState.flags & Shared::Flag::BackgroundBlur,
        "Whee::BaseLayer::backgroundBlurPassCount(): background blur not enabled", {});
    return state.backgroundBlurPassCount;
}

BaseLayer& BaseLayer::setBackgroundBlurPassCount(UnsignedInt count) {
    auto& state = static_cast<State&>(*_state);
    #ifndef CORRADE_NO_ASSERT
    auto& sharedState = static_cast<const Shared::State&>(state.shared);
    #endif
    CORRADE_ASSERT(sharedState.flags & Shared::Flag::BackgroundBlur,
        "Whee::BaseLayer::setBackgroundBlurPassCount(): background blur not enabled", *this);
    CORRADE_ASSERT(count,
        "Whee::BaseLayer::setBackgroundBlurPassCount(): expected at least one pass", *this);
    state.backgroundBlurPassCount = count;
    return *this;
}

DataHandle BaseLayer::create(const UnsignedInt style, const Color3& color, const Vector4& outlineWidth, const NodeHandle node) {
    State& state = static_cast<State&>(*_state);
    #ifndef CORRADE_NO_ASSERT
    auto& sharedState = static_cast<Shared::State&>(state.shared);
    #endif
    CORRADE_ASSERT(style < sharedState.styleCount,
        "Whee::BaseLayer::create(): style" << style << "out of range for" << sharedState.styleCount << "styles", {});

    const DataHandle handle = AbstractLayer::create(node);
    const UnsignedInt id = dataHandleId(handle);
    if(id >= state.data.size()) {
        arrayAppend(state.data, NoInit, id - state.data.size() + 1);
        state.styles = stridedArrayView(state.data).slice(&Implementation::BaseLayerData::style);
        state.calculatedStyles = stridedArrayView(state.data).slice(&Implementation::BaseLayerData::calculatedStyle);
    }

    Implementation::BaseLayerData& data = state.data[id];
    data.outlineWidth = outlineWidth;
    /** @todo is there a way to have create() with all possible per-data
        options that doesn't make it ambiguous / impossible to extend further?
        adding a padding argument would make it kind of clash with
        outlineWidth because the type is the same and there's no clear ordering
        between the two, furthermore later adding also cornerRadius and
        possibly innerOutlineCornerRadius would make it even more error-prone
        to use, and annoying as well as it won't really be possible to specify
        just a subset without getting ambiguous ... OTOH in Python this would
        be simple and desirable to do with keyword-only arguments that all have
        a default */
    data.padding = {};
    data.color = color;
    data.style = style;
    /* calculatedStyle is filled by AbstractVisualLayer::doUpdate() */
    data.textureCoordinateOffset = {};
    data.textureCoordinateSize = Vector2{1.0f};
    return handle;
}

Color3 BaseLayer::color(const DataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::BaseLayer::color(): invalid handle" << handle, {});
    return static_cast<const State&>(*_state).data[dataHandleId(handle)].color;
}

Color3 BaseLayer::color(const LayerDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::BaseLayer::color(): invalid handle" << handle, {});
    return static_cast<const State&>(*_state).data[layerDataHandleId(handle)].color;
}

void BaseLayer::setColor(const DataHandle handle, const Color3& color) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::BaseLayer::setColor(): invalid handle" << handle, );
    setColorInternal(dataHandleId(handle), color);
}

void BaseLayer::setColor(const LayerDataHandle handle, const Color3& color) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::BaseLayer::setColor(): invalid handle" << handle, );
    setColorInternal(layerDataHandleId(handle), color);
}

void BaseLayer::setColorInternal(const UnsignedInt id, const Color3& color) {
    static_cast<State&>(*_state).data[id].color = color;
    setNeedsUpdate();
}

void BaseLayer::setOutlineWidth(const DataHandle handle, const Vector4& width) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::BaseLayer::setOutlineWidth(): invalid handle" << handle, );
    setOutlineWidthInternal(dataHandleId(handle), width);
}

Vector4 BaseLayer::outlineWidth(const DataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::BaseLayer::outlineWidth(): invalid handle" << handle, {});
    return static_cast<const State&>(*_state).data[dataHandleId(handle)].outlineWidth;
}

Vector4 BaseLayer::outlineWidth(const LayerDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::BaseLayer::outlineWidth(): invalid handle" << handle, {});
    return static_cast<const State&>(*_state).data[layerDataHandleId(handle)].outlineWidth;
}

void BaseLayer::setOutlineWidth(const LayerDataHandle handle, const Vector4& width) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::BaseLayer::setOutlineWidth(): invalid handle" << handle, );
    setOutlineWidthInternal(layerDataHandleId(handle), width);
}

void BaseLayer::setOutlineWidthInternal(const UnsignedInt id, const Vector4& width) {
    static_cast<State&>(*_state).data[id].outlineWidth = width;
    setNeedsUpdate();
}

Vector4 BaseLayer::padding(const DataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::BaseLayer::padding(): invalid handle" << handle, {});
    return static_cast<const State&>(*_state).data[dataHandleId(handle)].padding;
}

Vector4 BaseLayer::padding(const LayerDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::BaseLayer::padding(): invalid handle" << handle, {});
    return static_cast<const State&>(*_state).data[layerDataHandleId(handle)].padding;
}

void BaseLayer::setPadding(const DataHandle handle, const Vector4& padding) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::BaseLayer::setPadding(): invalid handle" << handle, );
    setPaddingInternal(dataHandleId(handle), padding);
}

void BaseLayer::setPadding(const LayerDataHandle handle, const Vector4& padding) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::BaseLayer::setPadding(): invalid handle" << handle, );
    setPaddingInternal(layerDataHandleId(handle), padding);
}

void BaseLayer::setPaddingInternal(const UnsignedInt id, const Vector4& padding) {
    static_cast<State&>(*_state).data[id].padding = padding;
    setNeedsUpdate();
}

Vector3 BaseLayer::textureCoordinateOffset(const DataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::BaseLayer::textureCoordinateOffset(): invalid handle" << handle, {});
    return textureCoordinateOffsetInternal(dataHandleId(handle));
}

Vector3 BaseLayer::textureCoordinateOffset(const LayerDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::BaseLayer::textureCoordinateOffset(): invalid handle" << handle, {});
    return textureCoordinateOffsetInternal(layerDataHandleId(handle));
}

Vector3 BaseLayer::textureCoordinateOffsetInternal(const UnsignedInt id) const {
    auto& state = static_cast<const State&>(*_state);
    CORRADE_ASSERT(static_cast<const Shared::State&>(state.shared).flags & Shared::Flag::Textured,
        "Whee::BaseLayer::textureCoordinateOffset(): texturing not enabled", {});
    return state.data[id].textureCoordinateOffset;
}

Vector2 BaseLayer::textureCoordinateSize(const DataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::BaseLayer::textureCoordinateSize(): invalid handle" << handle, {});
    return textureCoordinateSizeInternal(dataHandleId(handle));
}

Vector2 BaseLayer::textureCoordinateSize(const LayerDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::BaseLayer::textureCoordinateSize(): invalid handle" << handle, {});
    return textureCoordinateSizeInternal(layerDataHandleId(handle));
}

Vector2 BaseLayer::textureCoordinateSizeInternal(const UnsignedInt id) const {
    auto& state = static_cast<const State&>(*_state);
    CORRADE_ASSERT(static_cast<const Shared::State&>(state.shared).flags & Shared::Flag::Textured,
        "Whee::BaseLayer::textureCoordinateSize(): texturing not enabled", {});
    return state.data[id].textureCoordinateSize;
}

void BaseLayer::setTextureCoordinates(const DataHandle handle, const Vector3& offset, const Vector2& size) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::BaseLayer::setTextureCoordinates(): invalid handle" << handle, );
    setTextureCoordinatesInternal(dataHandleId(handle), offset, size);
}

void BaseLayer::setTextureCoordinates(const LayerDataHandle handle, const Vector3& offset, const Vector2& size) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::BaseLayer::setTextureCoordinates(): invalid handle" << handle, );
    setTextureCoordinatesInternal(layerDataHandleId(handle), offset, size);
}

void BaseLayer::setTextureCoordinatesInternal(const UnsignedInt id, const Vector3& offset, const Vector2& size) {
    auto& state = static_cast<State&>(*_state);
    CORRADE_ASSERT(static_cast<const Shared::State&>(state.shared).flags & Shared::Flag::Textured,
        "Whee::BaseLayer::setTextureCoordinates(): texturing not enabled", );
    Implementation::BaseLayerData& data = state.data[id];
    data.textureCoordinateOffset = offset;
    data.textureCoordinateSize = size;
    setNeedsUpdate();
}

LayerFeatures BaseLayer::doFeatures() const {
    return AbstractVisualLayer::doFeatures()|LayerFeature::Draw|(static_cast<const Shared::State&>(_state->shared).flags & Shared::Flag::BackgroundBlur ? LayerFeature::Composite : LayerFeatures{});
}

void BaseLayer::doUpdate(const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, const Containers::BitArrayView nodesEnabled, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes) {
    /* The base implementation populates data.calculatedStyle */
    AbstractVisualLayer::doUpdate(dataIds, clipRectIds, clipRectDataCounts, nodeOffsets, nodeSizes, nodesEnabled, clipRectOffsets, clipRectSizes);

    auto& state = static_cast<State&>(*_state);
    auto& sharedState = static_cast<Shared::State&>(state.shared);
    /* Technically needed only if there's any actual data to update, but
       require it always for consistency (and easier testing) */
    CORRADE_ASSERT(!sharedState.styles.isEmpty(),
        "Whee::BaseLayer::update(): no style data was set", );

    /* Fill in indices in desired order */
    arrayResize(state.indices, NoInit, dataIds.size()*6);
    for(UnsignedInt i = 0; i != dataIds.size(); ++i) {
        const UnsignedInt vertexOffset = dataIds[i]*4;
        UnsignedInt indexOffset = i*6;

        /* 0---1 0---2 5
           |   | |  / /|
           |   | | / / |
           |   | |/ /  |
           2---3 1 3---4 */
        state.indices[indexOffset++] = vertexOffset + 0;
        state.indices[indexOffset++] = vertexOffset + 2;
        state.indices[indexOffset++] = vertexOffset + 1;
        state.indices[indexOffset++] = vertexOffset + 2;
        state.indices[indexOffset++] = vertexOffset + 3;
        state.indices[indexOffset++] = vertexOffset + 1;
    }

    const Containers::StridedArrayView1D<const NodeHandle> nodes = this->nodes();

    Containers::StridedArrayView1D<Implementation::BaseLayerVertex> vertices;
    if(sharedState.flags & Shared::Flag::Textured) {
        arrayResize(state.texturedVertices, NoInit, capacity()*4);
        /** @todo arrayCast() doesn't work because the derived type is not
            standard layout, so some slice<T>() to base or whatever? */
        vertices = {
            state.texturedVertices,
            state.texturedVertices.begin(),
            state.texturedVertices.size(), sizeof(Implementation::BaseLayerTexturedVertex)
        };
    } else {
        arrayResize(state.vertices, NoInit, capacity()*4);
        vertices = state.vertices;
    }

    /* Fill in quad corner positions and colors */
    for(const UnsignedInt dataId: dataIds) {
        const UnsignedInt nodeId = nodeHandleId(nodes[dataId]);
        const Implementation::BaseLayerData& data = state.data[dataId];

        /* 0---1
           |   |
           |   |
           |   |
           2---3 */
        const Vector4 padding = sharedState.styles[data.calculatedStyle].padding + data.padding;
        const Vector2 offset = nodeOffsets[nodeId];
        const Vector2 min = offset + padding.xy();
        const Vector2 max = offset + nodeSizes[nodeId] - Math::gather<'z', 'w'>(padding);
        const Vector2 sizeHalf = (max - min)*0.5f;
        const Vector2 sizeHalfNegative = -sizeHalf;
        for(UnsignedByte i = 0; i != 4; ++i) {
            Implementation::BaseLayerVertex& vertex = vertices[dataId*4 + i];

            /* ✨ */
            vertex.position = Math::lerp(min, max, BitVector2{i});
            vertex.centerDistance = Math::lerp(sizeHalfNegative, sizeHalf, BitVector2{i});
            vertex.outlineWidth = data.outlineWidth;
            vertex.color = data.color;
            vertex.styleUniform = sharedState.styles[data.calculatedStyle].uniform;
        }
    }

    /* Fill in also quad texture coordinates if enabled */
    if(sharedState.flags & Shared::Flag::Textured) {
        for(const UnsignedInt dataId: dataIds) {
            const Implementation::BaseLayerData& data = state.data[dataId];

            /* The texture coordinates are Y-flipped compared to the positions
               to account for Y-down (positions) vs Y-up (GL textures) */
            /** @todo which may get annoying with non-GL renderers that don't
                Y-flip the projection, reconsider? */
            const Vector2 min = data.textureCoordinateOffset.xy() + Vector2::yAxis(data.textureCoordinateSize.y());
            const Vector2 max = data.textureCoordinateOffset.xy() + Vector2::xAxis(data.textureCoordinateSize.x());
            for(UnsignedByte i = 0; i != 4; ++i)
                state.texturedVertices[dataId*4 + i].textureCoordinates = {Math::lerp(min, max, BitVector2{i}), data.textureCoordinateOffset.z()};
        }
    }
}

}}
