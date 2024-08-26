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
#include <Corrade/Containers/Iterable.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Utility/Algorithms.h>
#include <Magnum/Math/Functions.h>
#include <Magnum/Math/Matrix3.h>
#include <Magnum/Math/Swizzle.h>
#include <Magnum/Math/Time.h>

#include "Magnum/Whee/BaseLayerAnimator.h"
#include "Magnum/Whee/Event.h"
#include "Magnum/Whee/Handle.h"
#include "Magnum/Whee/Implementation/baseLayerState.h"

namespace Magnum { namespace Whee {

Debug& operator<<(Debug& debug, const BaseLayerSharedFlag value) {
    debug << "Whee::BaseLayerSharedFlag" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case BaseLayerSharedFlag::value: return debug << "::" #value;
        _c(Textured)
        _c(BackgroundBlur)
        _c(NoRoundedCorners)
        _c(NoOutline)
        _c(TextureMask)
        _c(SubdividedQuads)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "(" << Debug::nospace << Debug::hex << UnsignedByte(value) << Debug::nospace << ")";
}

Debug& operator<<(Debug& debug, const BaseLayerSharedFlags value) {
    return Containers::enumSetDebugOutput(debug, value, "Whee::BaseLayerSharedFlags{}", {
        BaseLayerSharedFlag::TextureMask,
        /* Implied by TextureMask, has to be after */
        BaseLayerSharedFlag::Textured,
        BaseLayerSharedFlag::BackgroundBlur,
        BaseLayerSharedFlag::NoRoundedCorners,
        BaseLayerSharedFlag::NoOutline,
        BaseLayerSharedFlag::SubdividedQuads
    });
}

BaseLayer::Shared::State::State(Shared& self, const Configuration& configuration): AbstractVisualLayer::Shared::State{self, configuration.styleCount(), configuration.dynamicStyleCount()},
    /* The radius is always at most 31, so can be a byte */
    backgroundBlurRadius{UnsignedByte(configuration.backgroundBlurRadius())},
    flags{configuration.flags()},
    styleUniformCount{configuration.styleUniformCount()}
{
    styleStorage = Containers::ArrayTuple{
        {NoInit, configuration.styleCount(), styles},
        {NoInit, configuration.dynamicStyleCount() ? configuration.styleUniformCount() : 0, styleUniforms}
    };
}

BaseLayer::Shared::Shared(Containers::Pointer<State>&& state): AbstractVisualLayer::Shared{Utility::move(state)} {
    #ifndef CORRADE_NO_ASSERT
    const State& s = static_cast<const State&>(*_state);
    #endif
    CORRADE_ASSERT(s.styleCount + s.dynamicStyleCount,
        "Whee::BaseLayer::Shared: expected non-zero total style count", );
    CORRADE_ASSERT(!(s.flags & BaseLayerSharedFlag::SubdividedQuads) || !(s.flags & (BaseLayerSharedFlag::NoOutline|BaseLayerSharedFlag::NoRoundedCorners)),
        "Whee::BaseLayer::Shared:" << BaseLayerSharedFlag::SubdividedQuads << "and" << (s.flags & (BaseLayerSharedFlag::NoOutline|BaseLayerSharedFlag::NoRoundedCorners)) << "are mutually exclusive", );
}

BaseLayer::Shared::Shared(const Configuration& configuration): Shared{Containers::pointer<State>(*this, configuration)} {}

BaseLayer::Shared::Shared(NoCreateT) noexcept: AbstractVisualLayer::Shared{NoCreate} {}

UnsignedInt BaseLayer::Shared::styleUniformCount() const {
    return static_cast<const State&>(*_state).styleUniformCount;
}

BaseLayerSharedFlags BaseLayer::Shared::flags() const {
    return static_cast<const State&>(*_state).flags;
}

void BaseLayer::Shared::setStyleInternal(const BaseLayerCommonStyleUniform& commonUniform, const Containers::ArrayView<const BaseLayerStyleUniform> uniforms, const Containers::StridedArrayView1D<const Vector4>& stylePaddings) {
    State& state = static_cast<State&>(*_state);
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

    /* If there are dynamic styles, the layers will combine them with the
       static styles and upload to a single buffer, so just copy them to an
       array for the layers to reuse */
    if(state.dynamicStyleCount) {
        state.commonStyleUniform = commonUniform;
        Utility::copy(uniforms, state.styleUniforms);
    } else doSetStyle(commonUniform, uniforms);

    /* Save the smoothness value that we'll use for expanding quad area. See
       the variable comment for why the uniform isn't used instead. */
    state.smoothness = commonUniform.smoothness;

    #ifndef CORRADE_NO_ASSERT
    /* Now it's safe to call update() */
    state.setStyleCalled = true;
    #endif

    /* Make doState() of all layers sharing this state return NeedsDataUpdate
       in order to update style-to-uniform mappings, paddings and also
       smoothness-dependent quad expansion. In case of dynamic styles also
       NeedsCommonDataUpdate to upload the changed per-layer uniform buffers.
       Setting it only if those differ would trigger update only if actually
       needed, but it may be prohibitively expensive compared to updating
       always. */
    ++state.styleUpdateStamp;
}

BaseLayer::Shared& BaseLayer::Shared::setStyle(const BaseLayerCommonStyleUniform& commonUniform, const Containers::ArrayView<const BaseLayerStyleUniform> uniforms, const Containers::StridedArrayView1D<const UnsignedInt>& styleToUniform, const Containers::StridedArrayView1D<const Vector4>& stylePaddings) {
    State& state = static_cast<State&>(*_state);
    CORRADE_ASSERT(styleToUniform.size() == state.styleCount,
        "Whee::BaseLayer::Shared::setStyle(): expected" << state.styleCount << "style uniform indices, got" << styleToUniform.size(), *this);
    setStyleInternal(commonUniform, uniforms, stylePaddings);
    #ifndef CORRADE_NO_ASSERT
    for(std::size_t i = 0; i != styleToUniform.size(); ++i)
        CORRADE_ASSERT(styleToUniform[i] < state.styleUniformCount,
            "Whee::BaseLayer::Shared::setStyle(): uniform index" << styleToUniform[i] << "out of range for" << state.styleUniformCount << "uniforms" << "at index" << i, *this);
    #endif
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
    CORRADE_ASSERT(!styleUniformCount == !styleCount,
        "Whee::BaseLayer::Shared::Configuration: expected style uniform count and style count to be either both zero or both non-zero, got" << styleUniformCount << "and" << styleCount, );
}

BaseLayer::Shared::Configuration& BaseLayer::Shared::Configuration::setBackgroundBlurRadius(const UnsignedInt radius, const Float cutoff) {
    CORRADE_ASSERT(radius < 32,
        "Whee::BaseLayer::Shared::Configuration::setBackgroundBlurRadius(): radius" << radius << "too large", *this);
    _backgroundBlurRadius = radius;
    _backgroundBlurCutoff = cutoff;
    return *this;
}

BaseLayer::State::State(Shared::State& shared): AbstractVisualLayer::State{shared}, styleUpdateStamp{shared.styleUpdateStamp} {
    dynamicStyleStorage = Containers::ArrayTuple{
        {ValueInit, shared.dynamicStyleCount, dynamicStyleUniforms},
        {ValueInit, shared.dynamicStyleCount, dynamicStylePaddings},
    };
}

BaseLayer::BaseLayer(const LayerHandle handle, Containers::Pointer<State>&& state): AbstractVisualLayer{handle, Utility::move(state)} {}

BaseLayer::BaseLayer(const LayerHandle handle, Shared& shared): BaseLayer{handle, Containers::pointer<State>(static_cast<Shared::State&>(*shared._state))} {}

UnsignedInt BaseLayer::backgroundBlurPassCount() const {
    auto& state = static_cast<const State&>(*_state);
    #ifndef CORRADE_NO_ASSERT
    auto& sharedState = static_cast<const Shared::State&>(state.shared);
    #endif
    CORRADE_ASSERT(sharedState.flags & BaseLayerSharedFlag::BackgroundBlur,
        "Whee::BaseLayer::backgroundBlurPassCount(): background blur not enabled", {});
    return state.backgroundBlurPassCount;
}

BaseLayer& BaseLayer::setBackgroundBlurPassCount(UnsignedInt count) {
    auto& state = static_cast<State&>(*_state);
    #ifndef CORRADE_NO_ASSERT
    auto& sharedState = static_cast<const Shared::State&>(state.shared);
    #endif
    CORRADE_ASSERT(sharedState.flags & BaseLayerSharedFlag::BackgroundBlur,
        "Whee::BaseLayer::setBackgroundBlurPassCount(): background blur not enabled", *this);
    CORRADE_ASSERT(count,
        "Whee::BaseLayer::setBackgroundBlurPassCount(): expected at least one pass", *this);
    state.backgroundBlurPassCount = count;
    setNeedsUpdate(LayerState::NeedsCompositeOffsetSizeUpdate);
    return *this;
}

BaseLayer& BaseLayer::assignAnimator(BaseLayerStyleAnimator& animator) {
    CORRADE_ASSERT(static_cast<const Shared::State&>(_state->shared).dynamicStyleCount,
        "Whee::BaseLayer::assignAnimator(): can't animate a layer with zero dynamic styles", *this);

    AbstractLayer::assignAnimator(animator);
    animator.setLayerInstance(*this, &_state->shared);
    return *this;
}

Containers::ArrayView<const BaseLayerStyleUniform> BaseLayer::dynamicStyleUniforms() const {
    return static_cast<const State&>(*_state).dynamicStyleUniforms;
}

Containers::StridedArrayView1D<const Vector4> BaseLayer::dynamicStylePaddings() const {
    return static_cast<const State&>(*_state).dynamicStylePaddings;
}

void BaseLayer::setDynamicStyle(const UnsignedInt id, const BaseLayerStyleUniform& uniform, const Vector4& padding) {
    auto& state = static_cast<State&>(*_state);
    CORRADE_ASSERT(id < state.dynamicStyleUniforms.size(),
        "Whee::BaseLayer::setDynamicStyle(): index" << id << "out of range for" << state.dynamicStyleUniforms.size() << "dynamic styles", );
    state.dynamicStyleUniforms[id] = uniform;

    /* Mark the layer as needing the dynamic style data update. The additional
       boolean is set to distinguish between needing to update the shared part
       of the style and the dynamic part. */
    setNeedsUpdate(LayerState::NeedsCommonDataUpdate);
    state.dynamicStyleChanged = true;

    /* Mark the layer as needing a full data update only if the padding
       actually changes, otherwise it's enough to just upload the uniforms */
    if(state.dynamicStylePaddings[id] != padding) {
        state.dynamicStylePaddings[id] = padding;
        setNeedsUpdate(LayerState::NeedsDataUpdate);
    }
}

DataHandle BaseLayer::create(const UnsignedInt style, const Color3& color, const Vector4& outlineWidth, const NodeHandle node) {
    State& state = static_cast<State&>(*_state);
    #ifndef CORRADE_NO_ASSERT
    auto& sharedState = static_cast<Shared::State&>(state.shared);
    #endif
    CORRADE_ASSERT(style < sharedState.styleCount + sharedState.dynamicStyleCount,
        "Whee::BaseLayer::create(): style" << style << "out of range for" << sharedState.styleCount + sharedState.dynamicStyleCount << "styles", {});

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
    setNeedsUpdate(LayerState::NeedsDataUpdate);
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
    setNeedsUpdate(LayerState::NeedsDataUpdate);
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
    setNeedsUpdate(LayerState::NeedsDataUpdate);
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
    CORRADE_ASSERT(static_cast<const Shared::State&>(state.shared).flags & BaseLayerSharedFlag::Textured,
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
    CORRADE_ASSERT(static_cast<const Shared::State&>(state.shared).flags & BaseLayerSharedFlag::Textured,
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
    CORRADE_ASSERT(static_cast<const Shared::State&>(state.shared).flags & BaseLayerSharedFlag::Textured,
        "Whee::BaseLayer::setTextureCoordinates(): texturing not enabled", );
    Implementation::BaseLayerData& data = state.data[id];
    data.textureCoordinateOffset = offset;
    data.textureCoordinateSize = size;
    setNeedsUpdate(LayerState::NeedsDataUpdate);
}

LayerFeatures BaseLayer::doFeatures() const {
    auto& sharedState = static_cast<const Shared::State&>(_state->shared);
    return AbstractVisualLayer::doFeatures()|(sharedState.dynamicStyleCount ? LayerFeature::AnimateStyles : LayerFeatures{})|LayerFeature::Draw|(sharedState.flags & BaseLayerSharedFlag::BackgroundBlur ? LayerFeature::Composite : LayerFeatures{});
}

void BaseLayer::doSetSize(const Vector2& size, const Vector2i& framebufferSize) {
    auto& state = static_cast<State&>(*_state);
    auto& sharedState = static_cast<Shared::State&>(state.shared);

    /* UI and framebuffer size is used for scaling smoothness expansion to
       actual pixels, framebuffer size is used by background blur but also
       subsequently by BaseLayerGL for scaling and Y-flipping clip rects, so
       not wrapping these in any condition.

       If their ratio differs and there are any data already that are affected,
       trigger a data update. It affects also background blur */
    if(size/Vector2{framebufferSize} != state.uiSize/Vector2{state.framebufferSize} && !state.data.isEmpty()) {
        /* Subdivided quads do smoothness expansion in the shader, so they
           don't need any data update */
        if(!(sharedState.flags >= BaseLayerSharedFlag::SubdividedQuads))
            setNeedsUpdate(LayerState::NeedsDataUpdate);
        /* Background blur quads have smoothness expansion as well */
        if(sharedState.flags >= BaseLayerSharedFlag::BackgroundBlur)
            setNeedsUpdate(LayerState::NeedsCompositeOffsetSizeUpdate);
    }
    state.uiSize = size;
    state.framebufferSize = framebufferSize;
}

void BaseLayer::doAdvanceAnimations(const Nanoseconds time, const Containers::MutableBitArrayView activeStorage, const Containers::StridedArrayView1D<Float>& factorStorage, const Containers::MutableBitArrayView removeStorage, const Containers::Iterable<AbstractStyleAnimator>& animators) {
    auto& state = static_cast<State&>(*_state);

    BaseLayerStyleAnimations animations;
    for(AbstractStyleAnimator& animator: animators) {
        if(!(animator.state() >= AnimatorState::NeedsAdvance))
            continue;

        const std::size_t capacity = animator.capacity();
        const Containers::Pair<bool, bool> needsAdvanceClean = animator.update(time,
            activeStorage.prefix(capacity),
            factorStorage.prefix(capacity),
            removeStorage.prefix(capacity));

        if(needsAdvanceClean.first())
            animations |= static_cast<BaseLayerStyleAnimator&>(animator).advance(
                activeStorage.prefix(capacity),
                factorStorage.prefix(capacity),
                removeStorage.prefix(capacity),
                state.dynamicStyleUniforms,
                state.dynamicStylePaddings,
                stridedArrayView(state.data).slice(&Implementation::BaseLayerData::style));
        if(needsAdvanceClean.second())
            animator.clean(removeStorage.prefix(capacity));
    }

    if(animations & (BaseLayerStyleAnimation::Style|BaseLayerStyleAnimation::Padding))
        setNeedsUpdate(LayerState::NeedsDataUpdate);
    if(animations >= BaseLayerStyleAnimation::Uniform) {
        setNeedsUpdate(LayerState::NeedsCommonDataUpdate);
        state.dynamicStyleChanged = true;
    }
}

LayerStates BaseLayer::doState() const {
    LayerStates states = AbstractVisualLayer::doState();

    auto& state = static_cast<const State&>(*_state);
    auto& sharedState = static_cast<const Shared::State&>(state.shared);
    if(state.styleUpdateStamp != sharedState.styleUpdateStamp) {
        /* Needed because uniform mapping and paddings can change, and
           additionally also smoothness-dependent quad expansion */
        states |= LayerState::NeedsDataUpdate;
        /* If there are dynamic styles, each layer also needs to upload the
           style uniform buffer */
        if(sharedState.dynamicStyleCount)
            states |= LayerState::NeedsCommonDataUpdate;
        /* If background blur is enabled, the quads are also expanded based on
           smoothness */
        if(sharedState.flags >= BaseLayerSharedFlag::BackgroundBlur)
            states |= LayerState::NeedsCompositeOffsetSizeUpdate;
    }
    return states;
}

void BaseLayer::doUpdate(const LayerStates states, const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, const Containers::BitArrayView nodesEnabled, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes, const Containers::StridedArrayView1D<const Vector2>& compositeRectOffsets, const Containers::StridedArrayView1D<const Vector2>& compositeRectSizes) {
    /* The base implementation populates data.calculatedStyle */
    AbstractVisualLayer::doUpdate(states, dataIds, clipRectIds, clipRectDataCounts, nodeOffsets, nodeSizes, nodesEnabled, clipRectOffsets, clipRectSizes, compositeRectOffsets, compositeRectSizes);

    auto& state = static_cast<State&>(*_state);
    auto& sharedState = static_cast<Shared::State&>(state.shared);
    /* Technically needed only if there's any actual data to update, but
       require it always for consistency (and easier testing) */
    CORRADE_ASSERT(sharedState.setStyleCalled,
        "Whee::BaseLayer::update(): no style data was set", );

    /* Fill in indices in desired order if either the data themselves or the
       node order changed. Flattening the logic for less indentation, first the
       less-data-heavy case with just a single quad for every data but a more
       complicated fragment shader. */
    const bool updateIndices =
        states >= LayerState::NeedsNodeOrderUpdate ||
        states >= LayerState::NeedsDataUpdate;
    if(updateIndices && !(sharedState.flags >= BaseLayerSharedFlag::SubdividedQuads)) {
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

    /* Then the more data-heavy case with 9 quads for every data, but a simpler
       fragment shader */
    } else if(updateIndices && sharedState.flags >= BaseLayerSharedFlag::SubdividedQuads) {
        /* Vertex IDs divisible by 4 are the outer corners, ID % 4 == 3 are the
           inner corners. ID % 4 == 2 are outer vertical edges, ID % 4 == 1 are
           outer horizontal edges.

            0---1---5---4   0---2  5 6---8 11 12-14 17
            |   |   |   |   | /  / | | /  / | | /  / |
            2---3---7---6   1  3---4 7  9--10-13 15 16
            |   |   |   |   18-20 23 24-26 29 30-32 35
            |   |   |   |   | /  / | | /  / | | /  / |
            |   |   |   |   19 21-22 25 27-28 31 33-34
            10-11---15-14   36-38 41 42-44 47 48-50 53
            |   |   |   |   | /  / | | /  / | | /  / |
            8---9---13-12   37 39-40 43 45-46 49 51-52 */
        arrayResize(state.indices, NoInit, dataIds.size()*6*9);
        for(UnsignedInt i = 0; i != dataIds.size(); ++i) {
            const UnsignedInt vertexOffset = dataIds[i]*16;
            UnsignedInt indexOffset = i*54;

            state.indices[indexOffset +  0] = vertexOffset +  0;
            state.indices[indexOffset +  1] = vertexOffset +  2;
            state.indices[indexOffset +  2] = vertexOffset +  1;
            state.indices[indexOffset +  3] = vertexOffset +  2;
            state.indices[indexOffset +  4] = vertexOffset +  3;
            state.indices[indexOffset +  5] = vertexOffset +  1;

            state.indices[indexOffset +  6] = vertexOffset +  1;
            state.indices[indexOffset +  7] = vertexOffset +  3;
            state.indices[indexOffset +  8] = vertexOffset +  5;
            state.indices[indexOffset +  9] = vertexOffset +  3;
            state.indices[indexOffset + 10] = vertexOffset +  7;
            state.indices[indexOffset + 11] = vertexOffset +  5;

            state.indices[indexOffset + 12] = vertexOffset +  5;
            state.indices[indexOffset + 13] = vertexOffset +  7;
            state.indices[indexOffset + 14] = vertexOffset +  4;
            state.indices[indexOffset + 15] = vertexOffset +  7;
            state.indices[indexOffset + 16] = vertexOffset +  6;
            state.indices[indexOffset + 17] = vertexOffset +  4;

            state.indices[indexOffset + 18] = vertexOffset +  2;
            state.indices[indexOffset + 19] = vertexOffset + 10;
            state.indices[indexOffset + 20] = vertexOffset +  3;
            state.indices[indexOffset + 21] = vertexOffset + 10;
            state.indices[indexOffset + 22] = vertexOffset + 11;
            state.indices[indexOffset + 23] = vertexOffset +  3;

            state.indices[indexOffset + 24] = vertexOffset +  3;
            state.indices[indexOffset + 25] = vertexOffset + 11;
            state.indices[indexOffset + 26] = vertexOffset +  7;
            state.indices[indexOffset + 27] = vertexOffset + 11;
            state.indices[indexOffset + 28] = vertexOffset + 15;
            state.indices[indexOffset + 29] = vertexOffset +  7;

            state.indices[indexOffset + 30] = vertexOffset +  7;
            state.indices[indexOffset + 31] = vertexOffset + 15;
            state.indices[indexOffset + 32] = vertexOffset +  6;
            state.indices[indexOffset + 33] = vertexOffset + 15;
            state.indices[indexOffset + 34] = vertexOffset + 14;
            state.indices[indexOffset + 35] = vertexOffset +  6;

            state.indices[indexOffset + 36] = vertexOffset + 10;
            state.indices[indexOffset + 37] = vertexOffset +  8;
            state.indices[indexOffset + 38] = vertexOffset + 11;
            state.indices[indexOffset + 39] = vertexOffset +  8;
            state.indices[indexOffset + 40] = vertexOffset +  9;
            state.indices[indexOffset + 41] = vertexOffset + 11;

            state.indices[indexOffset + 42] = vertexOffset + 11;
            state.indices[indexOffset + 43] = vertexOffset +  9;
            state.indices[indexOffset + 44] = vertexOffset + 15;
            state.indices[indexOffset + 45] = vertexOffset +  9;
            state.indices[indexOffset + 46] = vertexOffset + 13;
            state.indices[indexOffset + 47] = vertexOffset + 15;

            state.indices[indexOffset + 48] = vertexOffset + 15;
            state.indices[indexOffset + 49] = vertexOffset + 13;
            state.indices[indexOffset + 50] = vertexOffset + 14;
            state.indices[indexOffset + 51] = vertexOffset + 13;
            state.indices[indexOffset + 52] = vertexOffset + 12;
            state.indices[indexOffset + 53] = vertexOffset + 14;
        }
    }

    /* Fill in vertex data if the data themselves, the node offset/size or node
       enablement (and thus calculated styles) changed. Again flattening the
       logic for less indentation, first the less-data-heavy case with just a
       single quad for every data. */
    /** @todo split this further to just position-related data update and other
        data if it shows to help with perf */
    const bool updateVertices =
        states >= LayerState::NeedsNodeOffsetSizeUpdate ||
        states >= LayerState::NeedsNodeEnabledUpdate ||
        states >= LayerState::NeedsDataUpdate;
    if(updateVertices && !(sharedState.flags >= BaseLayerSharedFlag::SubdividedQuads)) {
        /* Resize the vertex array to fit all data, make a view on the common
           type prefix */
        const std::size_t typeSize = sharedState.flags & BaseLayerSharedFlag::Textured ?
            sizeof(Implementation::BaseLayerTexturedVertex) :
            sizeof(Implementation::BaseLayerVertex);
        arrayResize(state.vertices, NoInit, capacity()*4*typeSize);
        const Containers::StridedArrayView1D<Implementation::BaseLayerVertex> vertices{
            state.vertices,
            reinterpret_cast<Implementation::BaseLayerVertex*>(state.vertices.data()),
            state.vertices.size()/typeSize,
            std::ptrdiff_t(typeSize)};

        /* Convert smoothness from a pixel value to the UI coordinates */
        const Float smoothness = sharedState.smoothness*(state.uiSize/Vector2{state.framebufferSize}).max();

        /* Fill in quad corner positions and colors */
        const Containers::StridedArrayView1D<const NodeHandle> nodes = this->nodes();
        for(const UnsignedInt dataId: dataIds) {
            const UnsignedInt nodeId = nodeHandleId(nodes[dataId]);
            const Implementation::BaseLayerData& data = state.data[dataId];

            /* Padding together with an adjustment for quad smoothness in order
               to prevent the edges from looking cut off. Cannot do such an
               expansion in the shader because a similar operation needs to be
               done for texture coordinates, which may have a different scale
               altogether and the shader would need to get such a scale as
               an additional input. Doing this here would also work for
               potential future rotation, where, again, the shader would need
               to get a 2D "smoothness expansion vector" value, different for
               every data (and then another for textures), instead of just a
               single smoothness uniform for all. */
            Vector4 padding = data.padding - Vector4{smoothness};
            if(data.calculatedStyle < sharedState.styleCount)
                padding += sharedState.styles[data.calculatedStyle].padding;
            else {
                CORRADE_INTERNAL_DEBUG_ASSERT(data.calculatedStyle < sharedState.styleCount + sharedState.dynamicStyleCount);
                padding += state.dynamicStylePaddings[data.calculatedStyle - sharedState.styleCount];
            }

            /* 0---1
               |   |
               |   |
               |   |
               2---3 */
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
                /* For dynamic styles the uniform mapping is implicit and
                   they're placed right after all non-dynamic styles */
                vertex.styleUniform = data.calculatedStyle < sharedState.styleCount ?
                    sharedState.styles[data.calculatedStyle].uniform :
                    sharedState.styleUniformCount + data.calculatedStyle - sharedState.styleCount;
            }
        }

        /* Fill in also quad texture coordinates if enabled */
        if(sharedState.flags & BaseLayerSharedFlag::Textured) {
            const Containers::ArrayView<Implementation::BaseLayerTexturedVertex> texturedVertices = Containers::arrayCast<Implementation::BaseLayerTexturedVertex>(vertices).asContiguous();

            for(const UnsignedInt dataId: dataIds) {
                const Implementation::BaseLayerData& data = state.data[dataId];

                /* Expand the texture coordinates to match the position
                   expansion. It's calculated as the texture size multiplied by
                   the ratio of the smoothness expansion to the (pre-expanded)
                   quad size. If the texture size is 0 in any direction, the
                   expansion is 0 as well.

                   Taking the actual vertex positions instead of nodeSizes
                   because I'd have to do all the dance with padding
                   calculation again, now I just undo the smoothness. And using
                   those is also nice to the cache because they're literally
                   next to where I'm writing. */
                const Vector2 paddedQuadSizeWithoutSmoothness = vertices[dataId*4 + 3].position - vertices[dataId*4 + 0].position - Vector2{2.0f*smoothness};
                const Vector2 smoothnessExpansion = data.textureCoordinateSize*smoothness/paddedQuadSizeWithoutSmoothness*Vector2::yScale(-1.0f);

                /* The texture coordinates are Y-flipped compared to the
                   positions to account for Y-down (positions) vs Y-up (GL
                   textures) */
                /** @todo which may get annoying with non-GL renderers that
                    don't Y-flip the projection, reconsider? */
                const Vector2 min = data.textureCoordinateOffset.xy() + Vector2::yAxis(data.textureCoordinateSize.y()) - smoothnessExpansion;
                const Vector2 max = data.textureCoordinateOffset.xy() + Vector2::xAxis(data.textureCoordinateSize.x()) + smoothnessExpansion;
                for(UnsignedByte i = 0; i != 4; ++i)
                    texturedVertices[dataId*4 + i].textureCoordinates = {Math::lerp(min, max, BitVector2{i}), data.textureCoordinateOffset.z()};
            }
        }

    /* And then again the more data-heavy case with 9 quads for every data */
    } else if(updateVertices && sharedState.flags >= BaseLayerSharedFlag::SubdividedQuads) {
        /* Resize the vertex array to fit all data, make a view on the common type
           prefix */
        const std::size_t typeSize = sharedState.flags & BaseLayerSharedFlag::Textured ?
            sizeof(Implementation::BaseLayerSubdividedTexturedVertex) :
            sizeof(Implementation::BaseLayerSubdividedVertex);
        arrayResize(state.vertices, NoInit, capacity()*16*typeSize);
        const Containers::StridedArrayView1D<Implementation::BaseLayerSubdividedVertex> vertices{
            state.vertices,
            reinterpret_cast<Implementation::BaseLayerSubdividedVertex*>(state.vertices.data()),
            state.vertices.size()/typeSize,
            std::ptrdiff_t(typeSize)};

        /* Fill in the vertex data. In each corner the vertices are collapsed
           to a single point that's in the position of points 0, 4, 8 and 12,
           the inner vertices then get shifted to contain the inner & outer
           radius and outline width.

            0---1---5---4
            |   |   |   |
            2---3---7---6
            |   |   |   |
            |   |   |   |
            |   |   |   |
            10-11---15-14
            |   |   |   |
            8---9---13-12 */
        const Containers::StridedArrayView1D<const NodeHandle> nodes = this->nodes();
        for(const UnsignedInt dataId: dataIds) {
            const UnsignedInt nodeId = nodeHandleId(nodes[dataId]);
            const Implementation::BaseLayerData& data = state.data[dataId];

            /* All 16 vertices get the same color and style */
            for(std::size_t i = 0; i != 16; ++i) {
                Implementation::BaseLayerSubdividedVertex& vertex = vertices[dataId*16 + i];

                vertex.color = data.color;
                /* For dynamic styles the uniform mapping is implicit and
                   they're placed right after all non-dynamic styles */
                vertex.styleUniform = data.calculatedStyle < sharedState.styleCount ?
                    sharedState.styles[data.calculatedStyle].uniform :
                    sharedState.styleUniformCount + data.calculatedStyle - sharedState.styleCount;
            }

            /* Note that here, compared to the non-SubdividedQuads case above,
               the padding *does not* include the smoothness expansion. This is
               because the shader has to do expansion for outline width and
               corner radii on its own anyway, and doing the outer smoothness
               expansion there as well makes the code more understandable. */
            Vector4 padding = data.padding;
            if(data.calculatedStyle < sharedState.styleCount)
                padding += sharedState.styles[data.calculatedStyle].padding;
            else {
                CORRADE_INTERNAL_DEBUG_ASSERT(data.calculatedStyle < sharedState.styleCount + sharedState.dynamicStyleCount);
                padding += state.dynamicStylePaddings[data.calculatedStyle - sharedState.styleCount];
            }

            /* All four vertices in each corner get set to the same position
               and center distance */
            const Vector2 offset = nodeOffsets[nodeId];
            const Vector2 min = offset + padding.xy();
            const Vector2 max = offset + nodeSizes[nodeId] - Math::gather<'z', 'w'>(padding);
            const Float sizeHalfY = (max.y() - min.y())*0.5f;
            const Float sizeHalfYNegative = -sizeHalfY;
            for(UnsignedByte i = 0; i != 4; ++i) {
                /* ✨ */
                const Vector2 position = Math::lerp(min, max, BitVector2{i});
                const Float centerDistanceY = Math::lerp(sizeHalfYNegative, sizeHalfY, i >> 1);
                for(std::size_t j = 0; j != 4; ++j) {
                    Implementation::BaseLayerSubdividedVertex& vertex = vertices[dataId*16 + i *4 + j];
                    vertex.position = position;
                    vertex.centerDistanceY = centerDistanceY;
                }
            }

            /* All left vertices get the left outline width in the x coordinate
               and all right vertices get the right outline width */
            for(const std::size_t i: {0, 1, 2, 3, 8, 9, 10, 11})
                vertices[dataId*16 + i].outlineWidth.x() = data.outlineWidth.x();
            for(const std::size_t i: {4, 5, 6, 7, 12, 13, 14, 15})
                vertices[dataId*16 + i].outlineWidth.x() = data.outlineWidth.z();

            /* All top vertices get the top outline width in the y coordinate
               and all bottom vertices get the bottom width */
            for(const std::size_t i: {0, 1, 2, 3, 4, 5, 6, 7})
                vertices[dataId*16 + i].outlineWidth.y() = data.outlineWidth.y();
            for(const std::size_t i: {8, 9, 10, 11, 12, 13, 14, 15})
                vertices[dataId*16 + i].outlineWidth.y() = data.outlineWidth.w();
        }

        /* Fill in also quad texture coordinates if enabled */
        if(sharedState.flags & BaseLayerSharedFlag::Textured) {
            const Containers::ArrayView<Implementation::BaseLayerSubdividedTexturedVertex> texturedVertices = Containers::arrayCast<Implementation::BaseLayerSubdividedTexturedVertex>(vertices).asContiguous();

            for(const UnsignedInt dataId: dataIds) {
                const Implementation::BaseLayerData& data = state.data[dataId];

                /* The texture coordinates are Y-flipped compared to the
                   positions to account for Y-down (positions) vs Y-up (GL
                   textures) */
                /** @todo which may get annoying with non-GL renderers that
                    don't Y-flip the projection, reconsider? */
                const Vector2 min = data.textureCoordinateOffset.xy() + Vector2::yAxis(data.textureCoordinateSize.y());
                const Vector2 max = data.textureCoordinateOffset.xy() + Vector2::xAxis(data.textureCoordinateSize.x());

                /* Calculate texture scale relative to one projection unit in
                   order to correctly inter/extrapolate texture coordinates for
                   expanded quads. Similarly to the non-SubdividedQuads case,
                   taking the actual vertex positions to not have to deal with
                   the padding logic again, this time the size is without the
                   smoothness expansion so don't need to undo anything. The
                   scale is also passed through an extra vertex attribute
                   instead of being applied to the vertex data as the shader
                   needs to combine it with the actual expansion size based on
                   corner radius, outline size etc.

                   Here the scale is again Y flipping. */
                const Vector2 paddedQuadSize = vertices[dataId*16 + 12].position - vertices[dataId*16 + 0].position;
                const Vector2 textureScale = data.textureCoordinateSize/paddedQuadSize*Vector2::yScale(-1.0f);

                for(UnsignedByte i = 0; i != 4; ++i) {
                    const std::size_t index = dataId*16 + i *4;
                    const Vector3 coordinate{Math::lerp(min, max, BitVector2{i}), data.textureCoordinateOffset.z()};
                    for(std::size_t j = 0; j != 4; ++j) {
                        texturedVertices[index + j].textureScale = textureScale;
                        texturedVertices[index + j].textureCoordinates = coordinate;
                    }
                }
            }
        }
    }

    /* Fill in quads for background blur. They're present only if the layer has
       background blur (and thus compositing) enabled and need to be updated
       only if the compositing rects actually changed */
    if(states >= LayerState::NeedsCompositeOffsetSizeUpdate && sharedState.flags >= BaseLayerSharedFlag::BackgroundBlur) {
        arrayResize(state.backgroundBlurVertices, NoInit, compositeRectOffsets.size()*4);
        arrayResize(state.backgroundBlurIndices, NoInit, compositeRectOffsets.size()*6);

        /* Expand the quads to include the total blur radius among all passes,
           which is calculated as sqrt(passCount*radius*radius), plus extra
           padding to match smoothness expansion of the rendered quads. The
           radius is in pixels, convert it to match the [-1, +1] coordinates,
           i.e. multiply by 2.

           Note that both the `sharedState.backgroundBlurRadius` as well as
           `sharedState.smoothness` are in pixels so they don't need any
           additional adjustment, unlike above, where the smoothness is
           converted to be UI-size-relative. */
        /** @todo exclude the cutoff from this? how does the sqrt count into
            that? take a max of count*radiusWithCutoff and this? */
        const Vector2 blurRadiusPadding = Math::sqrt(Float(state.backgroundBlurPassCount))*(sharedState.backgroundBlurRadius + sharedState.smoothness)*state.uiSize/Vector2{state.framebufferSize};

        for(std::size_t i = 0; i != compositeRectOffsets.size(); ++i) {
            const Vector2 min = compositeRectOffsets[i] - blurRadiusPadding;
            const Vector2 max = compositeRectOffsets[i] + compositeRectSizes[i] + blurRadiusPadding;
            const UnsignedInt vertexOffset = i*4;

            /* 0---1 0---2 5
               |   | |  / /|
               |   | | / / |
               |   | |/ /  |
               2---3 1 3---4 */
            for(UnsignedByte j = 0; j != 4; ++j)
                /* ✨ */
                state.backgroundBlurVertices[vertexOffset + j] = Math::lerp(min, max, BitVector2{j});

            UnsignedInt indexOffset = i*6;
            state.backgroundBlurIndices[indexOffset++] = vertexOffset + 0;
            state.backgroundBlurIndices[indexOffset++] = vertexOffset + 2;
            state.backgroundBlurIndices[indexOffset++] = vertexOffset + 1;
            state.backgroundBlurIndices[indexOffset++] = vertexOffset + 2;
            state.backgroundBlurIndices[indexOffset++] = vertexOffset + 3;
            state.backgroundBlurIndices[indexOffset++] = vertexOffset + 1;
        }
    }

    /* Sync the style update stamp to not have doState() return NeedsDataUpdate
       / NeedsCommonDataUpdate again next time it's asked */
    if(states >= LayerState::NeedsDataUpdate ||
       states >= LayerState::NeedsCommonDataUpdate)
        state.styleUpdateStamp = sharedState.styleUpdateStamp;
}

}}
