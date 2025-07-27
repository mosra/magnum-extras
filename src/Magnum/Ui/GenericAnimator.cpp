/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023, 2024, 2025
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

#include "GenericAnimator.h"

#include <Corrade/Containers/BitArrayView.h>
#include <Corrade/Containers/Function.h>
#include <Corrade/Containers/GrowableArray.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Magnum/Math/Time.h>

#include "Magnum/Ui/Handle.h"

namespace Magnum { namespace Ui {

struct Animation {
    Containers::Function<void(Float)> animation;
    Float(*easing)(Float);
};

struct GenericAnimator::State {
    Containers::Array<Animation> animations;
};

GenericAnimator::GenericAnimator(AnimatorHandle handle): AbstractGenericAnimator{handle}, _state{InPlaceInit} {}

GenericAnimator::GenericAnimator(GenericAnimator&&) noexcept = default;

GenericAnimator::~GenericAnimator() = default;

GenericAnimator& GenericAnimator::operator=(GenericAnimator&&) noexcept = default;

UnsignedInt GenericAnimator::usedAllocatedAnimationCount() const {
    UnsignedInt count = 0;
    for(const Animation& animation: _state->animations)
        if(animation.animation.isAllocated())
            ++count;

    return count;
}

AnimationHandle GenericAnimator::create(Containers::Function<void(Float)>&& animation, Float(*const easing)(Float), const Nanoseconds start, const Nanoseconds duration, const UnsignedInt repeatCount, const AnimationFlags flags) {
    CORRADE_ASSERT(animation,
        "Ui::GenericAnimator::create(): animation is null", {});
    CORRADE_ASSERT(easing,
        "Ui::GenericAnimator::create(): easing is null", {});

    State& state = static_cast<State&>(*_state);
    const AnimationHandle handle = AbstractGenericAnimator::create(start, duration, repeatCount, flags);
    const UnsignedInt id = animationHandleId(handle);
    if(id >= state.animations.size())
        arrayResize(state.animations, id + 1);

    Animation& animationData = state.animations[id];
    animationData.animation = Utility::move(animation);
    animationData.easing = easing;

    return handle;
}

void GenericAnimator::removeInternal(const UnsignedInt id) {
    /* Set the animation to an empty instance to call any captured state
       destructors */
    _state->animations[id].animation = {};
}

void GenericAnimator::remove(AnimationHandle handle) {
    AbstractGenericAnimator::remove(handle);
    removeInternal(animationHandleId(handle));
}

void GenericAnimator::remove(AnimatorDataHandle handle) {
    AbstractGenericAnimator::remove(handle);
    removeInternal(animatorDataHandleId(handle));
}

auto GenericAnimator::easing(const AnimationHandle handle) const -> Float(*)(Float) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::GenericAnimator::easing(): invalid handle" << handle, {});
    return static_cast<const State&>(*_state).animations[animationHandleId(handle)].easing;
}

auto GenericAnimator::easing(const AnimatorDataHandle handle) const -> Float(*)(Float) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::GenericAnimator::easing(): invalid handle" << handle, {});
    return static_cast<const State&>(*_state).animations[animatorDataHandleId(handle)].easing;
}

AnimatorFeatures GenericAnimator::doFeatures() const { return {}; }

void GenericAnimator::doClean(const Containers::BitArrayView animationIdsToRemove) {
    /** @todo some way to iterate bits */
    for(std::size_t i = 0; i != animationIdsToRemove.size(); ++i) {
        if(!animationIdsToRemove[i])
            continue;
        removeInternal(i);
    }
}

void GenericAnimator::doAdvance(const Containers::BitArrayView active, Containers::BitArrayView, Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>& factors) {
    State& state = static_cast<State&>(*_state);
    /** @todo some way to iterate set bits */
    for(std::size_t i = 0; i != active.size(); ++i) {
        if(!active[i])
            continue;

        state.animations[i].animation(factors[i]);
    }
}

struct AnimationNode {
    Containers::Function<void(NodeHandle, Float)> animation;
    Float(*easing)(Float);
};

struct GenericNodeAnimator::State {
    Containers::Array<AnimationNode> animations;
};

GenericNodeAnimator::GenericNodeAnimator(AnimatorHandle handle): AbstractGenericAnimator{handle}, _state{InPlaceInit} {}

GenericNodeAnimator::GenericNodeAnimator(GenericNodeAnimator&&) noexcept = default;

GenericNodeAnimator::~GenericNodeAnimator() = default;

GenericNodeAnimator& GenericNodeAnimator::operator=(GenericNodeAnimator&&) noexcept = default;

UnsignedInt GenericNodeAnimator::usedAllocatedAnimationCount() const {
    UnsignedInt count = 0;
    for(const AnimationNode& animation: _state->animations)
        if(animation.animation.isAllocated())
            ++count;

    return count;
}

AnimationHandle GenericNodeAnimator::create(Containers::Function<void(NodeHandle, Float)>&& animation, Float(*const easing)(Float), const Nanoseconds start, const Nanoseconds duration, const NodeHandle node, const UnsignedInt repeatCount, const AnimationFlags flags) {
    CORRADE_ASSERT(animation,
        "Ui::GenericNodeAnimator::create(): animation is null", {});
    CORRADE_ASSERT(easing,
        "Ui::GenericNodeAnimator::create(): easing is null", {});

    State& state = static_cast<State&>(*_state);
    const AnimationHandle handle = AbstractGenericAnimator::create(start, duration, node, repeatCount, flags);
    const UnsignedInt id = animationHandleId(handle);
    if(id >= state.animations.size())
        arrayResize(state.animations, id + 1);

    AnimationNode& animationData = state.animations[id];
    animationData.animation = Utility::move(animation);
    animationData.easing = easing;

    return handle;
}

void GenericNodeAnimator::removeInternal(const UnsignedInt id) {
    /* Set the animation to an empty instance to call any captured state
       destructors */
    _state->animations[id].animation = {};
}

void GenericNodeAnimator::remove(AnimationHandle handle) {
    AbstractGenericAnimator::remove(handle);
    removeInternal(animationHandleId(handle));
}

void GenericNodeAnimator::remove(AnimatorDataHandle handle) {
    AbstractGenericAnimator::remove(handle);
    removeInternal(animatorDataHandleId(handle));
}

auto GenericNodeAnimator::easing(const AnimationHandle handle) const -> Float(*)(Float) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::GenericNodeAnimator::easing(): invalid handle" << handle, {});
    return static_cast<const State&>(*_state).animations[animationHandleId(handle)].easing;
}

auto GenericNodeAnimator::easing(const AnimatorDataHandle handle) const -> Float(*)(Float) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::GenericNodeAnimator::easing(): invalid handle" << handle, {});
    return static_cast<const State&>(*_state).animations[animatorDataHandleId(handle)].easing;
}

AnimatorFeatures GenericNodeAnimator::doFeatures() const {
    return AnimatorFeature::NodeAttachment;
}

void GenericNodeAnimator::doClean(const Containers::BitArrayView animationIdsToRemove) {
    /** @todo some way to iterate bits */
    for(std::size_t i = 0; i != animationIdsToRemove.size(); ++i) {
        if(!animationIdsToRemove[i])
            continue;
        removeInternal(i);
    }
}

void GenericNodeAnimator::doAdvance(const Containers::BitArrayView active, Containers::BitArrayView, Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>& factors) {
    const Containers::StridedArrayView1D<const NodeHandle> nodes = this->nodes();
    State& state = static_cast<State&>(*_state);
    /** @todo some way to iterate set bits */
    for(std::size_t i = 0; i != active.size(); ++i) {
        if(!active[i])
            continue;

        state.animations[i].animation(nodes[i], factors[i]);
    }
}

struct AnimationData {
    Containers::Function<void(DataHandle, Float)> animation;
    Float(*easing)(Float);
};

struct GenericDataAnimator::State {
    Containers::Array<AnimationData> animations;
};

GenericDataAnimator::GenericDataAnimator(AnimatorHandle handle): AbstractGenericAnimator{handle}, _state{InPlaceInit} {}

GenericDataAnimator::GenericDataAnimator(GenericDataAnimator&&) noexcept = default;

GenericDataAnimator::~GenericDataAnimator() = default;

GenericDataAnimator& GenericDataAnimator::operator=(GenericDataAnimator&&) noexcept = default;

UnsignedInt GenericDataAnimator::usedAllocatedAnimationCount() const {
    UnsignedInt count = 0;
    for(const AnimationData& animation: _state->animations)
        if(animation.animation.isAllocated())
            ++count;

    return count;
}

AnimationHandle GenericDataAnimator::create(Containers::Function<void(DataHandle, Float)>&& animation, Float(*const easing)(Float), const Nanoseconds start, const Nanoseconds duration, const DataHandle data, const UnsignedInt repeatCount, const AnimationFlags flags) {
    const AnimationHandle handle = AbstractGenericAnimator::create(start, duration, data, repeatCount, flags);
    createInternal(handle, Utility::move(animation), easing);
    return handle;
}

AnimationHandle GenericDataAnimator::create(Containers::Function<void(DataHandle, Float)>&& animation, Float(*const easing)(Float), const Nanoseconds start, const Nanoseconds duration, const LayerDataHandle data, const UnsignedInt repeatCount, const AnimationFlags flags) {
    const AnimationHandle handle = AbstractGenericAnimator::create(start, duration, data, repeatCount, flags);
    createInternal(handle, Utility::move(animation), easing);
    return handle;
}

void GenericDataAnimator::createInternal(const AnimationHandle handle, Containers::Function<void(DataHandle, Float)>&& animation, Float(*const easing)(Float)) {
    CORRADE_ASSERT(animation,
        "Ui::GenericDataAnimator::create(): animation is null", );
    CORRADE_ASSERT(easing,
        "Ui::GenericDataAnimator::create(): easing is null", );

    State& state = static_cast<State&>(*_state);
    const UnsignedInt id = animationHandleId(handle);
    if(id >= state.animations.size())
        arrayResize(state.animations, id + 1);

    AnimationData& animationData = state.animations[id];
    animationData.animation = Utility::move(animation);
    animationData.easing = easing;
}

void GenericDataAnimator::removeInternal(const UnsignedInt id) {
    /* Set the animation to an empty instance to call any captured state
       destructors */
    _state->animations[id].animation = {};
}

void GenericDataAnimator::remove(AnimationHandle handle) {
    AbstractGenericAnimator::remove(handle);
    removeInternal(animationHandleId(handle));
}

void GenericDataAnimator::remove(AnimatorDataHandle handle) {
    AbstractGenericAnimator::remove(handle);
    removeInternal(animatorDataHandleId(handle));
}

auto GenericDataAnimator::easing(const AnimationHandle handle) const -> Float(*)(Float) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::GenericDataAnimator::easing(): invalid handle" << handle, {});
    return static_cast<const State&>(*_state).animations[animationHandleId(handle)].easing;
}

auto GenericDataAnimator::easing(const AnimatorDataHandle handle) const -> Float(*)(Float) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::GenericDataAnimator::easing(): invalid handle" << handle, {});
    return static_cast<const State&>(*_state).animations[animatorDataHandleId(handle)].easing;
}

AnimatorFeatures GenericDataAnimator::doFeatures() const {
    return AnimatorFeature::DataAttachment;
}

void GenericDataAnimator::doClean(const Containers::BitArrayView animationIdsToRemove) {
    /** @todo some way to iterate bits */
    for(std::size_t i = 0; i != animationIdsToRemove.size(); ++i) {
        if(!animationIdsToRemove[i])
            continue;
        removeInternal(i);
    }
}

void GenericDataAnimator::doAdvance(const Containers::BitArrayView active, Containers::BitArrayView, Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>& factors) {
    const Containers::StridedArrayView1D<const LayerDataHandle> layerData = this->layerData();
    State& state = static_cast<State&>(*_state);
    /** @todo some way to iterate set bits */
    for(std::size_t i = 0; i != active.size(); ++i) {
        if(!active[i])
            continue;

        /* If not associated with any data, pass a null instead of combining it
           with the layer handle */
        state.animations[i].animation(
            layerData[i] == LayerDataHandle::Null ?
                DataHandle::Null : dataHandle(layer(), layerData[i]),
            factors[i]);
    }
}

}}
