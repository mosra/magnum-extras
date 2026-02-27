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

#include "GenericAnimator.h"

#include <Corrade/Containers/BitArrayView.h>
#include <Corrade/Containers/EnumSet.hpp>
#include <Corrade/Containers/Function.h>
#include <Corrade/Containers/GrowableArray.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Magnum/Math/Time.h>

#include "Magnum/Ui/Handle.h"

namespace Magnum { namespace Ui {

using namespace Math::Literals;

Debug& operator<<(Debug& debug, const GenericAnimationState value) {
    debug << "Ui::GenericAnimationState" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case GenericAnimationState::value: return debug << "::" #value;
        _c(Started)
        _c(Stopped)
        _c(Reverse)
        _c(Begin)
        _c(End)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "(" << Debug::nospace << Debug::hex << UnsignedByte(value) << Debug::nospace << ")";
}

Debug& operator<<(Debug& debug, const GenericAnimationStates value) {
    return Containers::enumSetDebugOutput(debug, value, debug.immediateFlags() >= Debug::Flag::Packed ? "{}" : "Ui::GenericAnimationStates{}", {
        GenericAnimationState::Started,
        GenericAnimationState::Stopped,
        GenericAnimationState::Reverse,
        GenericAnimationState::Begin,
        GenericAnimationState::End,
    });
}

namespace {

struct Animation {
    Containers::FunctionData function;
    Float(*easing)(Float);
    /** @todo ideally this would be inlined directly inside FunctionData.call,
        somehow -- e.g. an extra template argument to Function that decouples
        the actual wrapped signature from the call signature. Same is in
        the EventLayer implementation. */
    void(*call)(Animation&, NodeHandle, DataHandle, Float, GenericAnimationStates);
};

}

struct GenericAnimator::State {
    Containers::Array<Animation> animations;
};

GenericAnimator::GenericAnimator(const AnimatorHandle handle): AbstractGenericAnimator{handle}, _state{InPlaceInit} {}

GenericAnimator::GenericAnimator(GenericAnimator&&) noexcept = default;

GenericAnimator::~GenericAnimator() = default;

GenericAnimator& GenericAnimator::operator=(GenericAnimator&&) noexcept = default;

std::size_t GenericAnimator::usedAllocatedCount() const {
    std::size_t count = 0;
    for(const Animation& animation: _state->animations)
        if(animation.function.isAllocated())
            ++count;

    return count;
}

AnimationHandle GenericAnimator::create(Containers::Function<void(Float)>&& function, Float(*const easing)(Float), const Nanoseconds start, const Nanoseconds duration, const UnsignedInt repeatCount, const AnimationFlags flags) {
    CORRADE_ASSERT(function,
        "Ui::GenericAnimator::create(): function is null", {});
    CORRADE_ASSERT(easing,
        "Ui::GenericAnimator::create(): easing is null", {});

    const AnimationHandle handle = createInternal(start, duration, repeatCount, flags);

    Animation& animationData = _state->animations[animationHandleId(handle)];
    animationData.function = Utility::move(function);
    animationData.easing = easing;
    animationData.call = [](Animation& animation, NodeHandle, DataHandle, Float factor, GenericAnimationStates) {
        static_cast<Containers::Function<void(Float)>&>(animation.function)(animation.easing(factor));
    };

    return handle;
}

AnimationHandle GenericAnimator::create(Containers::Function<void(Float)>&& function, Float(*const easing)(Float), const Nanoseconds start, const Nanoseconds duration, const AnimationFlags flags) {
    return create(Utility::move(function), easing, start, duration, 1, flags);
}

AnimationHandle GenericAnimator::create(Containers::Function<void(Float, GenericAnimationStates)>&& function, Float(*const easing)(Float), const Nanoseconds start, const Nanoseconds duration, const UnsignedInt repeatCount, const AnimationFlags flags) {
    CORRADE_ASSERT(function,
        "Ui::GenericAnimator::create(): function is null", {});
    CORRADE_ASSERT(easing,
        "Ui::GenericAnimator::create(): easing is null", {});

    const AnimationHandle handle = createInternal(start, duration, repeatCount, flags);

    Animation& animationData = _state->animations[animationHandleId(handle)];
    animationData.function = Utility::move(function);
    animationData.easing = easing;
    animationData.call = [](Animation& animation, NodeHandle, DataHandle, Float factor, GenericAnimationStates state) {
        static_cast<Containers::Function<void(Float, GenericAnimationStates)>&>(animation.function)(animation.easing(factor), state);
    };

    return handle;
}

AnimationHandle GenericAnimator::create(Containers::Function<void(Float, GenericAnimationStates)>&& function, Float(*const easing)(Float), const Nanoseconds start, const Nanoseconds duration, const AnimationFlags flags) {
    return create(Utility::move(function), easing, start, duration, 1, flags);
}

AnimationHandle GenericAnimator::createInternal(const Nanoseconds start, const Nanoseconds duration, const UnsignedInt repeatCount, const AnimationFlags flags) {
    State& state = static_cast<State&>(*_state);
    const AnimationHandle handle = AbstractGenericAnimator::create(start, duration, repeatCount, flags);
    const UnsignedInt id = animationHandleId(handle);
    if(id >= state.animations.size())
        arrayResize(state.animations, id + 1);
    return handle;
}

AnimationHandle GenericAnimator::callOnce(Containers::Function<void()>&& function, const Nanoseconds at, const AnimationFlags flags) {
    CORRADE_ASSERT(function,
        "Ui::GenericAnimator::callOnce(): function is null", {});

    const AnimationHandle handle = createInternal(at, 0_nsec, 1, flags);

    Animation& animationData = _state->animations[animationHandleId(handle)];
    animationData.function = Utility::move(function);
    animationData.easing = nullptr;
    animationData.call = [](Animation& animation, NodeHandle, DataHandle, Float, GenericAnimationStates state) {
        if(state & GenericAnimationState::Stopped)
            static_cast<Containers::Function<void()>&>(animation.function)();
    };

    return handle;
}

void GenericAnimator::removeInternal(const UnsignedInt id) {
    /* Set the animation to an empty instance to call any captured state
       destructors */
    _state->animations[id].function = {};
}

void GenericAnimator::remove(const AnimationHandle handle) {
    AbstractGenericAnimator::remove(handle);
    removeInternal(animationHandleId(handle));
}

void GenericAnimator::remove(const AnimatorDataHandle handle) {
    AbstractGenericAnimator::remove(handle);
    removeInternal(animatorDataHandleId(handle));
}

bool GenericAnimator::isAllocated(const AnimationHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::GenericAnimator::isAllocated(): invalid handle" << handle, {});
    return _state->animations[animationHandleId(handle)].function.isAllocated();
}

bool GenericAnimator::isAllocated(const AnimatorDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::GenericAnimator::isAllocated(): invalid handle" << handle, {});
    return _state->animations[animatorDataHandleId(handle)].function.isAllocated();
}

auto GenericAnimator::easing(const AnimationHandle handle) const -> Float(*)(Float) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::GenericAnimator::easing(): invalid handle" << handle, {});
    return _state->animations[animationHandleId(handle)].easing;
}

auto GenericAnimator::easing(const AnimatorDataHandle handle) const -> Float(*)(Float) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::GenericAnimator::easing(): invalid handle" << handle, {});
    return _state->animations[animatorDataHandleId(handle)].easing;
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

namespace {

/* Used by GenericAnimator, GenericNodeAnimator and GenericDataAnimator */
inline GenericAnimationStates animationStates(bool started, bool stopped, AnimationFlags flags) {
    GenericAnimationStates states =
        (started ? GenericAnimationState::Started : GenericAnimationState{})|
        (stopped ? GenericAnimationState::Stopped : GenericAnimationState{});

    /* Add Reverse only if Started or Stopped is present. Otherwise we'd have
       to deal with ReverseEveryOther, figuring out whether a particular
       iteration is reversed or not. */
    if((started || stopped) && flags >= AnimationFlag::Reverse)
        states |= GenericAnimationState::Reverse;

    /* Shorthands for convenience, as checking for begin / end this way in
       application code would be way too error-prone */
    if((started && !(flags >= AnimationFlag::Reverse)) ||
       (stopped && flags >= AnimationFlag::Reverse))
        states |= GenericAnimationState::Begin;
    if((stopped && !(flags >= AnimationFlag::Reverse)) ||
       (started && flags >= AnimationFlag::Reverse))
        states |= GenericAnimationState::End;

    return states;
}

}

void GenericAnimator::doAdvance(const Containers::BitArrayView active, const Containers::BitArrayView started, const Containers::BitArrayView stopped, const Containers::StridedArrayView1D<const Float>& factors) {
    const Containers::StridedArrayView1D<const AnimationFlags> flags = this->flags();
    State& state = static_cast<State&>(*_state);
    /** @todo some way to iterate set bits */
    for(std::size_t i = 0; i != active.size(); ++i) {
        if(!active[i])
            continue;

        Animation& animation = state.animations[i];
        animation.call(animation,
            {}, {},
            factors[i],
            animationStates(started[i], stopped[i], flags[i]));
    }
}

struct GenericNodeAnimator::State {
    Containers::Array<Animation> animations;
};

GenericNodeAnimator::GenericNodeAnimator(const AnimatorHandle handle): AbstractGenericAnimator{handle}, _state{InPlaceInit} {}

GenericNodeAnimator::GenericNodeAnimator(GenericNodeAnimator&&) noexcept = default;

GenericNodeAnimator::~GenericNodeAnimator() = default;

GenericNodeAnimator& GenericNodeAnimator::operator=(GenericNodeAnimator&&) noexcept = default;

std::size_t GenericNodeAnimator::usedAllocatedCount() const {
    std::size_t count = 0;
    for(const Animation& animation: _state->animations)
        if(animation.function.isAllocated())
            ++count;

    return count;
}

AnimationHandle GenericNodeAnimator::create(Containers::Function<void(NodeHandle, Float)>&& function, Float(*const easing)(Float), const Nanoseconds start, const Nanoseconds duration, const NodeHandle node, const UnsignedInt repeatCount, const AnimationFlags flags) {
    CORRADE_ASSERT(function,
        "Ui::GenericNodeAnimator::create(): function is null", {});
    CORRADE_ASSERT(easing,
        "Ui::GenericNodeAnimator::create(): easing is null", {});

    const AnimationHandle handle = createInternal(start, duration, node, repeatCount, flags);

    Animation& animationData = _state->animations[animationHandleId(handle)];
    animationData.function = Utility::move(function);
    animationData.easing = easing;
    animationData.call = [](Animation& animation, NodeHandle node, DataHandle, Float factor, GenericAnimationStates) {
        static_cast<Containers::Function<void(NodeHandle, Float)>&>(animation.function)(node, animation.easing(factor));
    };

    return handle;
}

AnimationHandle GenericNodeAnimator::create(Containers::Function<void(NodeHandle, Float)>&& function, Float(*const easing)(Float), const Nanoseconds start, const Nanoseconds duration, const NodeHandle node, const AnimationFlags flags) {
    return create(Utility::move(function), easing, start, duration, node, 1, flags);
}

AnimationHandle GenericNodeAnimator::create(Containers::Function<void(NodeHandle, Float, GenericAnimationStates)>&& function, Float(*const easing)(Float), const Nanoseconds start, const Nanoseconds duration, const NodeHandle node, const UnsignedInt repeatCount, const AnimationFlags flags) {
    CORRADE_ASSERT(function,
        "Ui::GenericNodeAnimator::create(): function is null", {});
    CORRADE_ASSERT(easing,
        "Ui::GenericNodeAnimator::create(): easing is null", {});

    const AnimationHandle handle = createInternal(start, duration, node, repeatCount, flags);

    Animation& animationData = _state->animations[animationHandleId(handle)];
    animationData.function = Utility::move(function);
    animationData.easing = easing;
    animationData.call = [](Animation& animation, NodeHandle node, DataHandle, Float factor, GenericAnimationStates state) {
        static_cast<Containers::Function<void(NodeHandle, Float, GenericAnimationStates)>&>(animation.function)(node, animation.easing(factor), state);
    };

    return handle;
}

AnimationHandle GenericNodeAnimator::create(Containers::Function<void(NodeHandle, Float, GenericAnimationStates)>&& animation, Float(*const easing)(Float), const Nanoseconds start, const Nanoseconds duration, const NodeHandle node, const AnimationFlags flags) {
    return create(Utility::move(animation), easing, start, duration, node, 1, flags);
}

AnimationHandle GenericNodeAnimator::createInternal(const Nanoseconds start, const Nanoseconds duration, const NodeHandle node, const UnsignedInt repeatCount, const AnimationFlags flags) {
    State& state = static_cast<State&>(*_state);
    const AnimationHandle handle = AbstractGenericAnimator::create(start, duration, node, repeatCount, flags);
    const UnsignedInt id = animationHandleId(handle);
    if(id >= state.animations.size())
        arrayResize(state.animations, id + 1);
    return handle;
}

AnimationHandle GenericNodeAnimator::callOnce(Containers::Function<void(NodeHandle)>&& function, const Nanoseconds at, const NodeHandle node, const AnimationFlags flags) {
    CORRADE_ASSERT(function,
        "Ui::GenericNodeAnimator::callOnce(): function is null", {});

    const AnimationHandle handle = createInternal(at, 0_nsec, node, 1, flags);

    Animation& animationData = _state->animations[animationHandleId(handle)];
    animationData.function = Utility::move(function);
    animationData.easing = nullptr;
    animationData.call = [](Animation& animation, NodeHandle node, DataHandle, Float, GenericAnimationStates state) {
        if(state & GenericAnimationState::Stopped)
            static_cast<Containers::Function<void(NodeHandle)>&>(animation.function)(node);
    };

    return handle;
}

void GenericNodeAnimator::removeInternal(const UnsignedInt id) {
    /* Set the animation to an empty instance to call any captured state
       destructors */
    _state->animations[id].function = {};
}

void GenericNodeAnimator::remove(const AnimationHandle handle) {
    AbstractGenericAnimator::remove(handle);
    removeInternal(animationHandleId(handle));
}

void GenericNodeAnimator::remove(const AnimatorDataHandle handle) {
    AbstractGenericAnimator::remove(handle);
    removeInternal(animatorDataHandleId(handle));
}

bool GenericNodeAnimator::isAllocated(const AnimationHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::GenericNodeAnimator::isAllocated(): invalid handle" << handle, {});
    return _state->animations[animationHandleId(handle)].function.isAllocated();
}

bool GenericNodeAnimator::isAllocated(const AnimatorDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::GenericNodeAnimator::isAllocated(): invalid handle" << handle, {});
    return _state->animations[animatorDataHandleId(handle)].function.isAllocated();
}

auto GenericNodeAnimator::easing(const AnimationHandle handle) const -> Float(*)(Float) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::GenericNodeAnimator::easing(): invalid handle" << handle, {});
    return _state->animations[animationHandleId(handle)].easing;
}

auto GenericNodeAnimator::easing(const AnimatorDataHandle handle) const -> Float(*)(Float) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::GenericNodeAnimator::easing(): invalid handle" << handle, {});
    return _state->animations[animatorDataHandleId(handle)].easing;
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

void GenericNodeAnimator::doAdvance(const Containers::BitArrayView active, const Containers::BitArrayView started, const Containers::BitArrayView stopped, const Containers::StridedArrayView1D<const Float>& factors) {
    const Containers::StridedArrayView1D<const NodeHandle> nodes = this->nodes();
    const Containers::StridedArrayView1D<const AnimationFlags> flags = this->flags();
    State& state = static_cast<State&>(*_state);
    /** @todo some way to iterate set bits */
    for(std::size_t i = 0; i != active.size(); ++i) {
        if(!active[i])
            continue;

        Animation& animation = state.animations[i];
        animation.call(animation,
            nodes[i], {},
            factors[i],
            animationStates(started[i], stopped[i], flags[i]));
    }
}

struct GenericDataAnimator::State {
    Containers::Array<Animation> animations;
};

GenericDataAnimator::GenericDataAnimator(const AnimatorHandle handle): AbstractGenericAnimator{handle}, _state{InPlaceInit} {}

GenericDataAnimator::GenericDataAnimator(GenericDataAnimator&&) noexcept = default;

GenericDataAnimator::~GenericDataAnimator() = default;

GenericDataAnimator& GenericDataAnimator::operator=(GenericDataAnimator&&) noexcept = default;

std::size_t GenericDataAnimator::usedAllocatedCount() const {
    std::size_t count = 0;
    for(const Animation& animation: _state->animations)
        if(animation.function.isAllocated())
            ++count;

    return count;
}

AnimationHandle GenericDataAnimator::create(Containers::Function<void(DataHandle, Float)>&& function, Float(*const easing)(Float), const Nanoseconds start, const Nanoseconds duration, const DataHandle data, const UnsignedInt repeatCount, const AnimationFlags flags) {
    const AnimationHandle handle = AbstractGenericAnimator::create(start, duration, data, repeatCount, flags);
    createInternal(handle, Utility::move(function), easing);
    return handle;
}

AnimationHandle GenericDataAnimator::create(Containers::Function<void(DataHandle, Float)>&& function, Float(*const easing)(Float), const Nanoseconds start, const Nanoseconds duration, const DataHandle data, const AnimationFlags flags) {
    return create(Utility::move(function), easing, start, duration, data, 1, flags);
}

AnimationHandle GenericDataAnimator::create(Containers::Function<void(DataHandle, Float)>&& function, Float(*const easing)(Float), const Nanoseconds start, const Nanoseconds duration, const LayerDataHandle data, const UnsignedInt repeatCount, const AnimationFlags flags) {
    const AnimationHandle handle = AbstractGenericAnimator::create(start, duration, data, repeatCount, flags);
    createInternal(handle, Utility::move(function), easing);
    return handle;
}

AnimationHandle GenericDataAnimator::create(Containers::Function<void(DataHandle, Float)>&& function, Float(*const easing)(Float), const Nanoseconds start, const Nanoseconds duration, const LayerDataHandle data, const AnimationFlags flags) {
    return create(Utility::move(function), easing, start, duration, data, 1, flags);
}

AnimationHandle GenericDataAnimator::create(Containers::Function<void(DataHandle, Float, GenericAnimationStates)>&& function, Float(*const easing)(Float), const Nanoseconds start, const Nanoseconds duration, const DataHandle data, const UnsignedInt repeatCount, const AnimationFlags flags) {
    const AnimationHandle handle = AbstractGenericAnimator::create(start, duration, data, repeatCount, flags);
    createInternal(handle, Utility::move(function), easing);
    return handle;
}

AnimationHandle GenericDataAnimator::create(Containers::Function<void(DataHandle, Float, GenericAnimationStates)>&& function, Float(*const easing)(Float), const Nanoseconds start, const Nanoseconds duration, const DataHandle data, const AnimationFlags flags) {
    return create(Utility::move(function), easing, start, duration, data, 1, flags);
}

AnimationHandle GenericDataAnimator::create(Containers::Function<void(DataHandle, Float, GenericAnimationStates)>&& function, Float(*const easing)(Float), const Nanoseconds start, const Nanoseconds duration, const LayerDataHandle data, const UnsignedInt repeatCount, const AnimationFlags flags) {
    const AnimationHandle handle = AbstractGenericAnimator::create(start, duration, data, repeatCount, flags);
    createInternal(handle, Utility::move(function), easing);
    return handle;
}

AnimationHandle GenericDataAnimator::create(Containers::Function<void(DataHandle, Float, GenericAnimationStates)>&& function, Float(*const easing)(Float), const Nanoseconds start, const Nanoseconds duration, const LayerDataHandle data, const AnimationFlags flags) {
    return create(Utility::move(function), easing, start, duration, data, 1, flags);
}

void GenericDataAnimator::createInternal(const AnimationHandle handle) {
    State& state = static_cast<State&>(*_state);
    const UnsignedInt id = animationHandleId(handle);
    if(id >= state.animations.size())
        arrayResize(state.animations, id + 1);
}

void GenericDataAnimator::createInternal(const AnimationHandle handle, Containers::Function<void(DataHandle, Float)>&& function, Float(*const easing)(Float)) {
    CORRADE_ASSERT(function,
        "Ui::GenericDataAnimator::create(): function is null", );
    CORRADE_ASSERT(easing,
        "Ui::GenericDataAnimator::create(): easing is null", );

    createInternal(handle);

    Animation& animationData = _state->animations[animationHandleId(handle)];
    animationData.function = Utility::move(function);
    animationData.easing = easing;
    animationData.call = [](Animation& animation, NodeHandle, DataHandle data, Float factor, GenericAnimationStates) {
        static_cast<Containers::Function<void(DataHandle, Float)>&>(animation.function)(data, animation.easing(factor));
    };
}

void GenericDataAnimator::createInternal(const AnimationHandle handle, Containers::Function<void(DataHandle, Float, GenericAnimationStates)>&& function, Float(*const easing)(Float)) {
    CORRADE_ASSERT(function,
        "Ui::GenericDataAnimator::create(): function is null", );
    CORRADE_ASSERT(easing,
        "Ui::GenericDataAnimator::create(): easing is null", );

    createInternal(handle);

    Animation& animationData = _state->animations[animationHandleId(handle)];
    animationData.function = Utility::move(function);
    animationData.easing = easing;
    animationData.call = [](Animation& animation, NodeHandle, DataHandle data, Float factor, GenericAnimationStates state) {
        static_cast<Containers::Function<void(DataHandle, Float, GenericAnimationStates)>&>(animation.function)(data, animation.easing(factor), state);
    };
}

AnimationHandle GenericDataAnimator::callOnce(Containers::Function<void(DataHandle)>&& function, const Nanoseconds at, const DataHandle data, const AnimationFlags flags) {
    const AnimationHandle handle = AbstractGenericAnimator::create(at, 0_nsec, data, flags);
    callOnceInternal(handle, Utility::move(function));
    return handle;
}

AnimationHandle GenericDataAnimator::callOnce(Containers::Function<void(DataHandle)>&& function, const Nanoseconds at, const LayerDataHandle data, const AnimationFlags flags) {
    const AnimationHandle handle = AbstractGenericAnimator::create(at, 0_nsec, data, flags);
    callOnceInternal(handle, Utility::move(function));
    return handle;
}

void GenericDataAnimator::callOnceInternal(const AnimationHandle handle, Containers::Function<void(DataHandle)>&& function) {
    CORRADE_ASSERT(function,
        "Ui::GenericDataAnimator::callOnce(): function is null", );

    createInternal(handle);

    Animation& animationData = _state->animations[animationHandleId(handle)];
    animationData.function = Utility::move(function);
    animationData.easing = nullptr;
    animationData.call = [](Animation& animation, NodeHandle, DataHandle data, Float, GenericAnimationStates state) {
        if(state & GenericAnimationState::Stopped)
            static_cast<Containers::Function<void(DataHandle)>&>(animation.function)(data);
    };
}

void GenericDataAnimator::removeInternal(const UnsignedInt id) {
    /* Set the animation to an empty instance to call any captured state
       destructors */
    _state->animations[id].function = {};
}

void GenericDataAnimator::remove(const AnimationHandle handle) {
    AbstractGenericAnimator::remove(handle);
    removeInternal(animationHandleId(handle));
}

void GenericDataAnimator::remove(const AnimatorDataHandle handle) {
    AbstractGenericAnimator::remove(handle);
    removeInternal(animatorDataHandleId(handle));
}

bool GenericDataAnimator::isAllocated(const AnimationHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::GenericDataAnimator::isAllocated(): invalid handle" << handle, {});
    return _state->animations[animationHandleId(handle)].function.isAllocated();
}

bool GenericDataAnimator::isAllocated(const AnimatorDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::GenericDataAnimator::isAllocated(): invalid handle" << handle, {});
    return _state->animations[animatorDataHandleId(handle)].function.isAllocated();
}

auto GenericDataAnimator::easing(const AnimationHandle handle) const -> Float(*)(Float) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::GenericDataAnimator::easing(): invalid handle" << handle, {});
    return _state->animations[animationHandleId(handle)].easing;
}

auto GenericDataAnimator::easing(const AnimatorDataHandle handle) const -> Float(*)(Float) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::GenericDataAnimator::easing(): invalid handle" << handle, {});
    return _state->animations[animatorDataHandleId(handle)].easing;
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

void GenericDataAnimator::doAdvance(const Containers::BitArrayView active, const Containers::BitArrayView started, const Containers::BitArrayView stopped, const Containers::StridedArrayView1D<const Float>& factors) {
    const Containers::StridedArrayView1D<const LayerDataHandle> layerData = this->layerData();
    const Containers::StridedArrayView1D<const AnimationFlags> flags = this->flags();
    State& state = static_cast<State&>(*_state);
    /** @todo some way to iterate set bits */
    for(std::size_t i = 0; i != active.size(); ++i) {
        if(!active[i])
            continue;

        /* If not associated with any data, pass a null instead of combining it
           with the layer handle */
        Animation& animation = state.animations[i];
        animation.call(animation,
            {},
            layerData[i] == LayerDataHandle::Null ?
                DataHandle::Null : dataHandle(layer(), layerData[i]),
            factors[i],
            animationStates(started[i], stopped[i], flags[i]));
    }
}

}}
