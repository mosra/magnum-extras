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

#include "AbstractAnimator.h"

#include <Corrade/Containers/ArrayTuple.h>
#include <Corrade/Containers/BitArray.h>
#include <Corrade/Containers/BitArrayView.h>
#include <Corrade/Containers/EnumSet.hpp>
#include <Corrade/Containers/GrowableArray.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Magnum/Math/Functions.h>
#include <Magnum/Math/Time.h>

#include "Magnum/Ui/AbstractLayer.h"
#include "Magnum/Ui/Handle.h"

namespace Magnum { namespace Ui {

using namespace Math::Literals;

Debug& operator<<(Debug& debug, const AnimatorFeature value) {
    debug << "Ui::AnimatorFeature" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case AnimatorFeature::value: return debug << "::" #value;
        _c(NodeAttachment)
        _c(DataAttachment)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "(" << Debug::nospace << Debug::hex << UnsignedByte(value) << Debug::nospace << ")";
}

Debug& operator<<(Debug& debug, const AnimatorFeatures value) {
    return Containers::enumSetDebugOutput(debug, value, "Ui::AnimatorFeatures{}", {
        AnimatorFeature::NodeAttachment,
        AnimatorFeature::DataAttachment
    });
}

Debug& operator<<(Debug& debug, const AnimatorState value) {
    debug << "Ui::AnimatorState" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case AnimatorState::value: return debug << "::" #value;
        _c(NeedsAdvance)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "(" << Debug::nospace << Debug::hex << UnsignedByte(value) << Debug::nospace << ")";
}

Debug& operator<<(Debug& debug, const AnimatorStates value) {
    return Containers::enumSetDebugOutput(debug, value, "Ui::AnimatorStates{}", {
        AnimatorState::NeedsAdvance
    });
}

Debug& operator<<(Debug& debug, const AnimationFlag value) {
    debug << "Ui::AnimationFlag" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case AnimationFlag::value: return debug << "::" #value;
        _c(KeepOncePlayed)
        _c(Reverse)
        _c(ReverseEveryOther)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "(" << Debug::nospace << Debug::hex << UnsignedByte(value) << Debug::nospace << ")";
}

Debug& operator<<(Debug& debug, const AnimationFlags value) {
    return Containers::enumSetDebugOutput(debug, value, "Ui::AnimationFlags{}", {
        AnimationFlag::KeepOncePlayed,
        AnimationFlag::Reverse,
        AnimationFlag::ReverseEveryOther,
    });
}

Debug& operator<<(Debug& debug, const AnimationState value) {
    debug << "Ui::AnimationState" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case AnimationState::value: return debug << "::" #value;
        _c(Scheduled)
        _c(Playing)
        _c(Paused)
        _c(Stopped)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "(" << Debug::nospace << Debug::hex << UnsignedByte(value) << Debug::nospace << ")";
}

Debug& operator<<(Debug& debug, const NodeAnimatorUpdate value) {
    debug << "Ui::NodeAnimatorUpdate" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case NodeAnimatorUpdate::value: return debug << "::" #value;
        _c(OffsetSize)
        _c(Opacity)
        _c(EventMask)
        _c(Enabled)
        _c(Clip)
        _c(Visibility)
        _c(Removal)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "(" << Debug::nospace << Debug::hex << UnsignedByte(value) << Debug::nospace << ")";
}

Debug& operator<<(Debug& debug, const NodeAnimatorUpdates value) {
    return Containers::enumSetDebugOutput(debug, value, "Ui::NodeAnimatorUpdates{}", {
        NodeAnimatorUpdate::OffsetSize,
        NodeAnimatorUpdate::Opacity,
        /* Superset of EventMask, has to be before */
        NodeAnimatorUpdate::Enabled,
        NodeAnimatorUpdate::EventMask,
        NodeAnimatorUpdate::Clip,
        NodeAnimatorUpdate::Visibility,
        NodeAnimatorUpdate::Removal
    });
}

namespace {

union Animation {
    explicit Animation() noexcept: used{} {}

    struct Used {
        /* Together with index of this item in `animations` used for creating
           an AnimatorDataHandle. Increased every time a handle reaches
           remove(). Has to be initially non-zero to differentiate the first
           ever handle (with index 0) from AnimatorDataHandle::Null. Once
           becomes `1 << AnimatorDataHandleGenerationBits` the handle gets
           disabled. */
        UnsignedShort generation = 1;

        AnimationFlags flags{NoInit};
        /* One byte free */
        UnsignedInt repeatCount;

        /* Duration. 0 only when the animation is freed, otherwise it's always
           positive. isHandleValid() checks this field to correctly mark
           invalid handles if the generation matches by accident. */
        Nanoseconds duration{NoInit};

        /* Time at which the animation is started, paused, stopped. All these
           have to be re-filled every time a handle is recycled, so it doesn't
           make sense to initialize them to anything. */
        Nanoseconds started{NoInit};
        Nanoseconds paused{NoInit};
        Nanoseconds stopped{NoInit};
    } used;

    /* Used only if the Animation is among free ones */
    struct Free {
        /* The generation value has to be preserved in order to increment it
           next time it gets used */
        UnsignedShort generation;

        /* Two bytes free */

        /* See State::firstFree for more information */
        UnsignedInt next;

        /* If this is 0, the animation is freed */
        Nanoseconds duration;
    } free;
};

#ifndef CORRADE_NO_STD_IS_TRIVIALLY_TRAITS
static_assert(std::is_trivially_copyable<Animation>::value, "Animation not trivially copyable");
#endif
static_assert(
    offsetof(Animation::Used, generation) == offsetof(Animation::Free, generation) &&
    offsetof(Animation::Used, duration) == offsetof(Animation::Free, duration),
    "Animation::Used and Free layout not compatible");

}

struct AbstractAnimator::State {
    AnimatorHandle handle;
    AnimatorStates state;

    /* 1 byte free */

    /* Used only if AnimatorFeature::DataAttachment is supported. Combined with
       `layerData` to form DataHandles. */
    LayerHandle layer = LayerHandle::Null;

    Containers::Array<Animation> animations;
    /* Indices in the `animations` array. The Animation then has a nextFree
       member containing the next free index. New animations get taken from the
       front, removed are put at the end. A value of ~UnsignedInt{} means
       there's no (first/next/last) free animation. */
    UnsignedInt firstFree = ~UnsignedInt{};
    UnsignedInt lastFree = ~UnsignedInt{};

    /* Used only if AnimatorFeature::NodeAttachment is supported, has the same
       size as `animations` */
    Containers::Array<NodeHandle> nodes;

    /* Used only if AnimatorFeature::DataAttachment is supported, has the same
       size as `animations`. Combined with `layer` to form DataHandles. */
    Containers::Array<LayerDataHandle> layerData;

    Nanoseconds time{Math::ZeroInit};
};

AbstractAnimator::AbstractAnimator(const AnimatorHandle handle): _state{InPlaceInit} {
    CORRADE_ASSERT(handle != AnimatorHandle::Null,
        "Ui::AbstractAnimator: handle is null", );
    _state->handle = handle;
}

AbstractAnimator::AbstractAnimator(AbstractAnimator&&) noexcept = default;

AbstractAnimator::~AbstractAnimator() = default;

AbstractAnimator& AbstractAnimator::operator=(AbstractAnimator&&) noexcept = default;

AnimatorHandle AbstractAnimator::handle() const {
    return _state->handle;
}

AnimatorFeatures AbstractAnimator::features() const {
    const AnimatorFeatures features = doFeatures();
    CORRADE_ASSERT(!(features >= AnimatorFeature::NodeAttachment) ||
                   !(features >= AnimatorFeature::DataAttachment),
        "Ui::AbstractAnimator::features():" << AnimatorFeature::NodeAttachment << "and" << AnimatorFeature::DataAttachment << "are mutually exclusive", {});
    return features;
}

LayerHandle AbstractAnimator::layer() const {
    CORRADE_ASSERT(features() & AnimatorFeature::DataAttachment,
        "Ui::AbstractAnimator::layer(): feature not supported", {});
    return _state->layer;
}

void AbstractAnimator::setLayerInternal(const AbstractLayer& layer) {
    /* Assumes the caller already verified presence of
       AnimatorFeature::DataAttachment and that the layer isn't set yet */
    _state->layer = layer.handle();
}

AnimatorStates AbstractAnimator::state() const {
    return _state->state;
}

Nanoseconds AbstractAnimator::time() const {
    return _state->time;
}

std::size_t AbstractAnimator::capacity() const {
    return _state->animations.size();
}

std::size_t AbstractAnimator::usedCount() const {
    /* In general we can assume that the amount of free data is always either
       zero or significantly less than the capacity, and thus iterating the
       (presumably small) free list should be faster, even though it involves
       jumping around in memory. */
    const State& state = *_state;
    std::size_t free = 0;
    UnsignedInt index = state.firstFree;
    while(index != ~UnsignedInt{}) {
        index = state.animations[index].free.next;
        ++free;
    }
    return state.animations.size() - free;
}

bool AbstractAnimator::isHandleValid(const AnimatorDataHandle handle) const {
    if(handle == AnimatorDataHandle::Null)
        return false;
    const State& state = *_state;
    const UnsignedInt index = animatorDataHandleId(handle);
    if(index >= state.animations.size())
        return false;
    const UnsignedInt generation = animatorDataHandleGeneration(handle);
    const Animation& animation = state.animations[index];
    /* Zero generation handles (i.e., where it wrapped around from all bits
       set) are expected to be expired and thus with duration being -max. In
       other words, it shouldn't be needed to verify also that generation is
       non-zero. */
    CORRADE_INTERNAL_DEBUG_ASSERT(generation || animation.used.duration == Nanoseconds::min());
    return animation.used.duration != Nanoseconds::min() && generation == animation.used.generation;
}

bool AbstractAnimator::isHandleValid(const AnimationHandle handle) const {
    return animationHandleAnimator(handle) == _state->handle && isHandleValid(animationHandleData(handle));
}

namespace {

AnimationState animationState(const Animation& animation, const Nanoseconds time) {
    /* The animation is stopped if the stopped time is at or before the started
       time, returning AnimationState::Stopped below.

       Not critically important for behavior as without it the animation would
       still work correctly, eventually transitioning from Scheduled to Stopped
       without any Playing or Paused in between, but this makes it Stopped
       already, potentially avoiding the need for AnimatorState::NeedsAdvance
       and useless UI redraw. */
    if(animation.used.stopped > animation.used.started) {
        /* The animation isn't playing yet if the started time is in the
           future */
        if(animation.used.started > time) {
            return AnimationState::Scheduled;

        /* The animation isn't playing anymore if the stopped time already
           happened, falling through to AnimationState::Stopped below */
        } else if(animation.used.stopped > time) {
            CORRADE_INTERNAL_ASSERT(animation.used.started <= time);

            const Nanoseconds currentTime = Math::min(animation.used.paused, time);

            /* The animation isn't playing anymore if all repeats were already
               exhausted, falling through to AnimationState::Stopped below */
            if(animation.used.repeatCount == 0 || animation.used.started + animation.used.duration*animation.used.repeatCount > currentTime) {
                /* The animation isn't currently playing if the paused time
                   already happened */
                if(animation.used.paused > time)
                    return AnimationState::Playing;
                else
                    return AnimationState::Paused;
            }
        }
    }

    return AnimationState::Stopped;
}

}

AnimationHandle AbstractAnimator::create(const Nanoseconds start, const Nanoseconds duration, const UnsignedInt repeatCount, const AnimationFlags flags) {
    CORRADE_ASSERT(duration >= 0_nsec,
        "Ui::AbstractAnimator::create(): expected non-negative duration, got" << duration, {});
    CORRADE_ASSERT(duration != 0_nsec || repeatCount == 1,
        "Ui::AbstractAnimator::create(): expected count to be 1 for an animation with zero duration but got" << repeatCount, {});
    State& state = *_state;

    /* Find the first free animation if there is, update the free index to
       point to the next one (or none) */
    Animation* animation;
    if(state.firstFree!= ~UnsignedInt{}) {
        animation = &state.animations[state.firstFree];

        if(state.firstFree == state.lastFree) {
            CORRADE_INTERNAL_ASSERT(animation->free.next == ~UnsignedInt{});
            state.firstFree = state.lastFree = ~UnsignedInt{};
        } else {
            state.firstFree = animation->free.next;
        }

    /* If there isn't, allocate a new one */
    } else {
        CORRADE_ASSERT(state.animations.size() < 1 << Implementation::AnimatorDataHandleIdBits,
            "Ui::AbstractAnimator::create(): can only have at most" << (1 << Implementation::AnimatorDataHandleIdBits) << "animations", {});
        animation = &arrayAppend(state.animations, InPlaceInit);
        if(features() & AnimatorFeature::NodeAttachment) {
            CORRADE_INTERNAL_ASSERT(state.nodes.size() == state.animations.size() - 1);
            arrayAppend(state.nodes, NoInit, 1);
        }
        if(features() & AnimatorFeature::DataAttachment) {
            CORRADE_INTERNAL_ASSERT(state.layerData.size() == state.animations.size() - 1);
            arrayAppend(state.layerData, NoInit, 1);
        }
    }

    const UnsignedInt id = animation - state.animations;

    /* Fill the data. In both above cases the generation is already set
       appropriately, either initialized to 1, or incremented when it got
       remove()d (to mark existing handles as invalid) */
    animation->used.flags = flags;
    animation->used.repeatCount = repeatCount;
    animation->used.duration = duration;
    animation->used.started = start;
    animation->used.paused = Nanoseconds::max();
    animation->used.stopped = Nanoseconds::max();
    if(features() & AnimatorFeature::NodeAttachment)
        state.nodes[id] = NodeHandle::Null;
    if(features() & AnimatorFeature::DataAttachment)
        state.layerData[id] = LayerDataHandle::Null;

    /* Mark the animator as needing an advance() call if the new animation
       is being scheduled or is playing. Creation alone doesn't make it
       possible to make the animation paused, but if the animation is already
       stopped, mark it also to perform automatic removal. */
    const AnimationState animationState = Ui::animationState(*animation, state.time);
    CORRADE_INTERNAL_ASSERT(animationState != AnimationState::Paused);
    if(animationState == AnimationState::Scheduled ||
       animationState == AnimationState::Playing ||
      (animationState == AnimationState::Stopped && !(flags & AnimationFlag::KeepOncePlayed)))
        state.state |= AnimatorState::NeedsAdvance;

    return animationHandle(state.handle, id, animation->used.generation);
}

AnimationHandle AbstractAnimator::create(const Nanoseconds start, const Nanoseconds duration, const AnimationFlags flags) {
    return create(start, duration, 1, flags);
}

AnimationHandle AbstractAnimator::create(const Nanoseconds start, const Nanoseconds duration, const NodeHandle node, const UnsignedInt repeatCount, const AnimationFlags flags) {
    CORRADE_ASSERT(features() >= AnimatorFeature::NodeAttachment,
        "Ui::AbstractAnimator::create(): node attachment not supported", {});
    const AnimationHandle handle = create(start, duration, repeatCount, flags);
    _state->nodes[animationHandleId(handle)] = node;
    return handle;
}

AnimationHandle AbstractAnimator::create(const Nanoseconds start, const Nanoseconds duration, const NodeHandle node, const AnimationFlags flags) {
    return create(start, duration, node, 1, flags);
}

AnimationHandle AbstractAnimator::create(const Nanoseconds start, const Nanoseconds duration, const DataHandle data, const UnsignedInt repeatCount, const AnimationFlags flags) {
    CORRADE_ASSERT(features() >= AnimatorFeature::DataAttachment,
        "Ui::AbstractAnimator::create(): data attachment not supported", {});
    State& state = *_state;
    CORRADE_ASSERT(state.layer != LayerHandle::Null,
        "Ui::AbstractAnimator::create(): no layer set for data attachment", {});
    CORRADE_ASSERT(data == DataHandle::Null || state.layer == dataHandleLayer(data),
        "Ui::AbstractAnimator::create(): expected a data handle with" << state.layer << "but got" << data, {});
    const AnimationHandle handle = create(start, duration, repeatCount, flags);
    state.layerData[animationHandleId(handle)] = dataHandleData(data);
    return handle;
}

AnimationHandle AbstractAnimator::create(const Nanoseconds start, const Nanoseconds duration, const DataHandle data, const AnimationFlags flags) {
    return create(start, duration, data, 1, flags);
}

AnimationHandle AbstractAnimator::create(const Nanoseconds start, const Nanoseconds duration, const LayerDataHandle data, const UnsignedInt repeatCount, const AnimationFlags flags) {
    CORRADE_ASSERT(features() >= AnimatorFeature::DataAttachment,
        "Ui::AbstractAnimator::create(): data attachment not supported", {});
    State& state = *_state;
    CORRADE_ASSERT(state.layer != LayerHandle::Null,
        "Ui::AbstractAnimator::create(): no layer set for data attachment", {});
    const AnimationHandle handle = create(start, duration, repeatCount, flags);
    state.layerData[animationHandleId(handle)] = data;
    return handle;
}

AnimationHandle AbstractAnimator::create(const Nanoseconds start, const Nanoseconds duration, const LayerDataHandle data, const AnimationFlags flags) {
    return create(start, duration, data, 1, flags);
}

void AbstractAnimator::remove(const AnimationHandle handle) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractAnimator::remove(): invalid handle" << handle, );

    /* Doesn't delegate to remove(LayouterNodeHandle) to avoid a double check;
       doesn't check just the layer portion of the handle and delegate to avoid
       a confusing assertion message if the data portion would be invalid */
    removeInternal(animationHandleId(handle));
}

void AbstractAnimator::remove(const AnimatorDataHandle handle) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractAnimator::remove(): invalid handle" << handle, );

    removeInternal(animatorDataHandleId(handle));
}

void AbstractAnimator::removeInternal(const UnsignedInt id) {
    State& state = *_state;
    Animation& animation = state.animations[id];

    /* Increase the layout generation so existing handles pointing to this
       layout are invalidated. Wrap around to 0 if it goes over the generation
       bits. */
    ++animation.used.generation &= (1 << Implementation::AnimatorDataHandleGenerationBits) - 1;

    /* Set the animation duration to -max to avoid falsely recognizing this
       item as used when directly iterating the list or in isHandleValid() if
       the generation matches by accident */
    animation.used.duration = Nanoseconds::min();

    /* Clear the node attachment to have null handles in the nodes() list for
       freed animations */
    if(features() & AnimatorFeature::NodeAttachment)
        state.nodes[id] = NodeHandle::Null;
    if(features() & AnimatorFeature::DataAttachment)
        state.layerData[id] = LayerDataHandle::Null;

    /* Put the layout at the end of the free list (while they're allocated from
       the front) to not exhaust the generation counter too fast. If the free
       list is empty however, update also the index of the first free layer.

       Don't do this if the generation wrapped around. That makes it disabled,
       i.e. impossible to be recycled later, to avoid aliasing old handles. */
    if(animation.used.generation != 0) {
        animation.free.next = ~UnsignedInt{};
        if(state.lastFree == ~UnsignedInt{}) {
            CORRADE_INTERNAL_ASSERT(
                state.firstFree == ~UnsignedInt{} &&
                state.lastFree == ~UnsignedInt{});
            state.firstFree = id;
        } else {
            state.animations[state.lastFree].free.next = id;
        }
        state.lastFree = id;
    }
}

Nanoseconds AbstractAnimator::duration(const AnimationHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractAnimator::duration(): invalid handle" << handle, {});
    return _state->animations[animationHandleId(handle)].used.duration;
}

Nanoseconds AbstractAnimator::duration(const AnimatorDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractAnimator::duration(): invalid handle" << handle, {});
    return _state->animations[animatorDataHandleId(handle)].used.duration;
}

UnsignedInt AbstractAnimator::repeatCount(const AnimationHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractAnimator::repeatCount(): invalid handle" << handle, {});
    return _state->animations[animationHandleId(handle)].used.repeatCount;
}

UnsignedInt AbstractAnimator::repeatCount(const AnimatorDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractAnimator::repeatCount(): invalid handle" << handle, {});
    return _state->animations[animatorDataHandleId(handle)].used.repeatCount;
}

void AbstractAnimator::setRepeatCount(const AnimationHandle handle, const UnsignedInt count) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractAnimator::setRepeatCount(): invalid handle" << handle, );
    setRepeatCountInternal(animationHandleId(handle), count);
}

void AbstractAnimator::setRepeatCount(const AnimatorDataHandle handle, const UnsignedInt count) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractAnimator::setRepeatCount(): invalid handle" << handle, );
    setRepeatCountInternal(animatorDataHandleId(handle), count);
}

void AbstractAnimator::setRepeatCountInternal(const UnsignedInt id, const UnsignedInt count) {
    CORRADE_ASSERT(_state->animations[id].used.duration != 0_nsec || count == 1,
        "Ui::AbstractAnimator::setRepeatCount(): expected count to be 1 for an animation with zero duration but got" << count, );
    _state->animations[id].used.repeatCount = count;
    /* No AnimatorState needs to be updated, it doesn't cause any
       already-stopped animations to start playing */
}

AnimationFlags AbstractAnimator::flags(const AnimationHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractAnimator::flags(): invalid handle" << handle, {});
    return _state->animations[animationHandleId(handle)].used.flags;
}

AnimationFlags AbstractAnimator::flags(const AnimatorDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractAnimator::flags(): invalid handle" << handle, {});
    return _state->animations[animatorDataHandleId(handle)].used.flags;
}

void AbstractAnimator::setFlags(const AnimationHandle handle, const AnimationFlags flags) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractAnimator::setFlags(): invalid handle" << handle, );
    setFlagsInternal(animationHandleId(handle), flags);
}

void AbstractAnimator::setFlags(const AnimatorDataHandle handle, const AnimationFlags flags) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractAnimator::setFlags(): invalid handle" << handle, );
    setFlagsInternal(animatorDataHandleId(handle), flags);
}

void AbstractAnimator::addFlags(const AnimationHandle handle, const AnimationFlags flags) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractAnimator::addFlags(): invalid handle" << handle, );
    const UnsignedInt id = animationHandleId(handle);
    setFlagsInternal(id, _state->animations[id].used.flags|flags);
}

void AbstractAnimator::addFlags(const AnimatorDataHandle handle, const AnimationFlags flags) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractAnimator::addFlags(): invalid handle" << handle, );
    const UnsignedInt id = animatorDataHandleId(handle);
    setFlagsInternal(id, _state->animations[id].used.flags|flags);
}

void AbstractAnimator::clearFlags(const AnimationHandle handle, const AnimationFlags flags) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractAnimator::clearFlags(): invalid handle" << handle, );
    const UnsignedInt id = animationHandleId(handle);
    setFlagsInternal(id, _state->animations[id].used.flags & ~flags);
}

void AbstractAnimator::clearFlags(const AnimatorDataHandle handle, const AnimationFlags flags) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractAnimator::clearFlags(): invalid handle" << handle, );
    const UnsignedInt id = animatorDataHandleId(handle);
    setFlagsInternal(id, _state->animations[id].used.flags & ~flags);
}

void AbstractAnimator::setFlagsInternal(const UnsignedInt id, const AnimationFlags flags) {
    _state->animations[id].used.flags = flags;
}

Nanoseconds AbstractAnimator::started(const AnimationHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractAnimator::started(): invalid handle" << handle, {});
    return _state->animations[animationHandleId(handle)].used.started;
}

Nanoseconds AbstractAnimator::started(const AnimatorDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractAnimator::started(): invalid handle" << handle, {});
    return _state->animations[animatorDataHandleId(handle)].used.started;
}

Nanoseconds AbstractAnimator::paused(const AnimationHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractAnimator::paused(): invalid handle" << handle, {});
    return _state->animations[animationHandleId(handle)].used.paused;
}

Nanoseconds AbstractAnimator::paused(const AnimatorDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractAnimator::paused(): invalid handle" << handle, {});
    return _state->animations[animatorDataHandleId(handle)].used.paused;
}

Nanoseconds AbstractAnimator::stopped(const AnimationHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractAnimator::stopped(): invalid handle" << handle, {});
    return _state->animations[animationHandleId(handle)].used.stopped;
}

Nanoseconds AbstractAnimator::stopped(const AnimatorDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractAnimator::stopped(): invalid handle" << handle, {});
    return _state->animations[animatorDataHandleId(handle)].used.stopped;
}

void AbstractAnimator::attach(const AnimationHandle animation, const NodeHandle node) {
    CORRADE_ASSERT(isHandleValid(animation),
        "Ui::AbstractAnimator::attach(): invalid handle" << animation, );
    attachInternal(animationHandleId(animation), node);
}

void AbstractAnimator::attach(const AnimatorDataHandle animation, const NodeHandle node) {
    CORRADE_ASSERT(isHandleValid(animation),
        "Ui::AbstractAnimator::attach(): invalid handle" << animation, );
    attachInternal(animatorDataHandleId(animation), node);
}

void AbstractAnimator::attachInternal(const UnsignedInt id, const NodeHandle node) {
    CORRADE_ASSERT(features() >= AnimatorFeature::NodeAttachment,
        "Ui::AbstractAnimator::attach(): node attachment not supported", );
    _state->nodes[id] = node;
}

NodeHandle AbstractAnimator::node(const AnimationHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractAnimator::node(): invalid handle" << handle, {});
    return nodeInternal(animationHandleId(handle));
}

NodeHandle AbstractAnimator::node(const AnimatorDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractAnimator::node(): invalid handle" << handle, {});
    return nodeInternal(animatorDataHandleId(handle));
}

NodeHandle AbstractAnimator::nodeInternal(const UnsignedInt id) const {
    CORRADE_ASSERT(features() >= AnimatorFeature::NodeAttachment,
        "Ui::AbstractAnimator::node(): feature not supported", {});
    return _state->nodes[id];
}

Containers::StridedArrayView1D<const NodeHandle> AbstractAnimator::nodes() const {
    CORRADE_ASSERT(features() >= AnimatorFeature::NodeAttachment,
        "Ui::AbstractAnimator::nodes(): feature not supported", {});
    const State& state = *_state;
    CORRADE_INTERNAL_ASSERT(state.nodes.size() == state.animations.size());
    return state.nodes;
}

void AbstractAnimator::attach(const AnimationHandle animation, const DataHandle data) {
    CORRADE_ASSERT(isHandleValid(animation),
        "Ui::AbstractAnimator::attach(): invalid handle" << animation, );
    attachInternal(animationHandleId(animation), data);
}

void AbstractAnimator::attach(const AnimatorDataHandle animation, const DataHandle data) {
    CORRADE_ASSERT(isHandleValid(animation),
        "Ui::AbstractAnimator::attach(): invalid handle" << animation, );
    attachInternal(animatorDataHandleId(animation), data);
}

void AbstractAnimator::attachInternal(const UnsignedInt id, const DataHandle data) {
    CORRADE_ASSERT(features() >= AnimatorFeature::DataAttachment,
        "Ui::AbstractAnimator::attach(): data attachment not supported", );
    State& state = *_state;
    CORRADE_ASSERT(state.layer != LayerHandle::Null,
        "Ui::AbstractAnimator::attach(): no layer set for data attachment", );
    CORRADE_ASSERT(data == DataHandle::Null || state.layer == dataHandleLayer(data),
        "Ui::AbstractAnimator::attach(): expected a data handle with" << state.layer << "but got" << data, );
    state.layerData[id] = dataHandleData(data);
}

void AbstractAnimator::attach(const AnimationHandle animation, const LayerDataHandle data) {
    CORRADE_ASSERT(isHandleValid(animation),
        "Ui::AbstractAnimator::attach(): invalid handle" << animation, );
    attachInternal(animationHandleId(animation), data);
}

void AbstractAnimator::attach(const AnimatorDataHandle animation, const LayerDataHandle data) {
    CORRADE_ASSERT(isHandleValid(animation),
        "Ui::AbstractAnimator::attach(): invalid handle" << animation, );
    attachInternal(animatorDataHandleId(animation), data);
}

void AbstractAnimator::attachInternal(const UnsignedInt id, const LayerDataHandle data) {
    CORRADE_ASSERT(features() >= AnimatorFeature::DataAttachment,
        "Ui::AbstractAnimator::attach(): data attachment not supported", );
    State& state = *_state;
    CORRADE_ASSERT(state.layer != LayerHandle::Null,
        "Ui::AbstractAnimator::attach(): no layer set for data attachment", );
    state.layerData[id] = data;
}

DataHandle AbstractAnimator::data(const AnimationHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractAnimator::data(): invalid handle" << handle, {});
    return dataInternal(animationHandleId(handle));
}

DataHandle AbstractAnimator::data(const AnimatorDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractAnimator::data(): invalid handle" << handle, {});
    return dataInternal(animatorDataHandleId(handle));
}

DataHandle AbstractAnimator::dataInternal(const UnsignedInt id) const {
    CORRADE_ASSERT(features() >= AnimatorFeature::DataAttachment,
        "Ui::AbstractAnimator::data(): feature not supported", {});
    const State& state = *_state;
    const LayerDataHandle data = state.layerData[id];
    if(data == LayerDataHandle::Null)
        return DataHandle::Null;
    /* attach() isn't possible to be called without a layer set, so the layer
       should always be a non-null handle at this point */
    const LayerHandle layer = state.layer;
    CORRADE_INTERNAL_ASSERT(layer != LayerHandle::Null);
    return dataHandle(layer, data);
}

Containers::StridedArrayView1D<const LayerDataHandle> AbstractAnimator::layerData() const {
    CORRADE_ASSERT(features() >= AnimatorFeature::DataAttachment,
        "Ui::AbstractAnimator::layerData(): feature not supported", {});
    const State& state = *_state;
    CORRADE_INTERNAL_ASSERT(state.layerData.size() == state.animations.size());
    return state.layerData;
}

AnimationState AbstractAnimator::state(const AnimationHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractAnimator::state(): invalid handle" << handle, {});
    const State& state = *_state;
    return animationState(state.animations[animationHandleId(handle)], state.time);
}

AnimationState AbstractAnimator::state(const AnimatorDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractAnimator::state(): invalid handle" << handle, {});
    const State& state = *_state;
    return animationState(state.animations[animatorDataHandleId(handle)], state.time);
}

namespace {

inline Float animationFactor(const Nanoseconds duration, const Nanoseconds started, const AnimationFlags flags, const Nanoseconds time) {
    CORRADE_INTERNAL_DEBUG_ASSERT(duration != 0_nsec && time >= started);
    const Nanoseconds played = time - started;
    /* Using doubles for the division to avoid precision loss even though
       floats seem to work even for the 292 year duration */
    const Float factor = Double(Long(played % duration))/Double(Long(duration));
    bool reverse = flags >= AnimationFlag::Reverse;
    /* If ReverseEveryOther is set, the reverse status is reversed for every
       other repeat */
    if(flags >= AnimationFlag::ReverseEveryOther) {
        const UnsignedLong repeat = played/duration;
        reverse ^= (repeat & 1);
    }
    return reverse ? 1.0f - factor : factor;
}

/* Shared between factorInternal() and advance() */
inline Float animationFactor(const Animation& animation, const Nanoseconds time, const AnimationState state) {
    if(state == AnimationState::Playing)
        return animationFactor(animation.used.duration, animation.used.started, animation.used.flags, time);
    if(state == AnimationState::Paused)
        return animationFactor(animation.used.duration, animation.used.started, animation.used.flags, animation.used.paused);

    /* Animations with zero duration should always resolve to Stopped state in
       animationState(), so for them the division by duration in the above call
       to animationFactor() doesn't happen. Moreover, they have repeats
       disabled, so in case they're reversed, the `repeat` below would be
       always 0, not dividing by duration either. */
    if(state == AnimationState::Stopped) {
        bool reverse = animation.used.flags >= AnimationFlag::Reverse;
        /* If ReverseEveryOther is set, the reverse status is reversed for
           every other repeat. Yes, that's a sentence I wrote. */
        if(animation.used.flags >= AnimationFlag::ReverseEveryOther) {
            /* Time at which the animation stops implicitly or. For zero
               duration the implicit stop is equivalent to `started`. */
            const Nanoseconds stoppedImplicit =
                animation.used.repeatCount == 0 ? Nanoseconds::max() :
                    animation.used.started +
                    animation.used.duration*animation.used.repeatCount;

            /* Calculate current repeat index and flip the `reverse` bit if
               it's odd. If the implicit stop happens before the explicit one
               (or there's no explicit stop, i.e. it's Nanoseconds::max()), the
               repeat count is equivalent to the actual repeat count of the
               animation. */
            if(stoppedImplicit < animation.used.stopped) {
                /* This can happen only if we're not repeating indefinitely.
                   Zero-duration animations also always have repeat count equal
                   to 1, meaning we never flip the `reverse` bit for those. */
                CORRADE_INTERNAL_DEBUG_ASSERT(animation.used.repeatCount != 0 && (animation.used.duration != 0_nsec || animation.used.repeatCount == 1));
                reverse ^= !(animation.used.repeatCount & 1);

            /* If explicit stop happens earlier than implicit, stop at the end
               of the repeat that's currently in progress. */
            } else {
                /* If the animation stopped before it actually started, there's
                   no repeats so far. This is always the case for zero-duration animations. */
                UnsignedLong repeat;
                if(animation.used.stopped <= animation.used.started) {
                    repeat = 0;

                /* Otherwise the in-progress repeat is the distance between
                   stopped and started time divided by duration and rounded
                   up. */
                } else {
                    CORRADE_INTERNAL_DEBUG_ASSERT(animation.used.duration != 0_nsec);
                    repeat = (animation.used.stopped + animation.used.duration - 1_nsec - animation.used.started)/animation.used.duration;
                }
                reverse ^= !(repeat & 1);
            }
        }
        return reverse ? 0.0f: 1.0f;
    }

    CORRADE_INTERNAL_DEBUG_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
}

}

Float AbstractAnimator::factor(const AnimationHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractAnimator::factor(): invalid handle" << handle, {});
    return factorInternal(animationHandleId(handle));
}

Float AbstractAnimator::factor(const AnimatorDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractAnimator::factor(): invalid handle" << handle, {});
    return factorInternal(animatorDataHandleId(handle));
}

Float AbstractAnimator::factorInternal(const UnsignedInt id) const {
    const State& state = *_state;
    const Animation& animation = state.animations[id];
    const AnimationState animationState = Ui::animationState(animation, state.time);
    if(animationState == AnimationState::Scheduled)
        return 0.0f;
    return animationFactor(animation, state.time, animationState);
}

void AbstractAnimator::play(const AnimationHandle handle, const Nanoseconds time) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractAnimator::play(): invalid handle" << handle, );
    playInternal(animationHandleId(handle), time);
}

void AbstractAnimator::play(const AnimatorDataHandle handle, const Nanoseconds time) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractAnimator::play(): invalid handle" << handle, );
    playInternal(animatorDataHandleId(handle), time);
}

void AbstractAnimator::playInternal(const UnsignedInt id, const Nanoseconds time) {
    State& state = *_state;
    Animation& animation = state.animations[id];

    /* If the animation
        - wasn't paused before or was stopped earlier than paused (paused time
          is Nanoseconds::max() or is >= stopped time),
        - or was paused earlier than actually started,
        - or we resume before the actual pause happens,
        - or we resume after it was stopped,
       play it from the start */
    if(animation.used.paused >= animation.used.stopped ||
       animation.used.started >= animation.used.paused ||
       animation.used.paused >= time ||
       time >= animation.used.stopped)
    {
        animation.used.started = time;

    /* Otherwise the started time is shortened by the duration for which it
       already played, i.e. `started = time - (paused - started)`, and the
       duration is non-negative */
    } else {
        CORRADE_INTERNAL_ASSERT(animation.used.paused > animation.used.started);
        animation.used.started += time - animation.used.paused;
    }

    animation.used.paused = Nanoseconds::max();
    animation.used.stopped = Nanoseconds::max();

    /* Mark the animator as needing advance() if the animation is now scheduled
       or playing. Can't be paused because the paused time was reset above. */
    const AnimationState animationStateAfter = animationState(animation, state.time);
    CORRADE_INTERNAL_ASSERT(animationStateAfter != AnimationState::Paused);
    if(animationStateAfter == AnimationState::Scheduled ||
       animationStateAfter == AnimationState::Playing)
        state.state |= AnimatorState::NeedsAdvance;
}

void AbstractAnimator::pause(const AnimationHandle handle, const Nanoseconds time) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractAnimator::pause(): invalid handle" << handle, );
    pauseInternal(animationHandleId(handle), time);
}

void AbstractAnimator::pause(const AnimatorDataHandle handle, const Nanoseconds time) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractAnimator::pause(): invalid handle" << handle, );
    pauseInternal(animatorDataHandleId(handle), time);
}

void AbstractAnimator::pauseInternal(const UnsignedInt id, const Nanoseconds time) {
    State& state = *_state;
    Animation& animation = state.animations[id];
    #ifndef CORRADE_NO_ASSERT
    const AnimationState stateBefore = animationState(animation, state.time);
    #endif
    animation.used.paused = time;

    #ifndef CORRADE_NO_ASSERT
    /* If the animation was scheduled, playing or paused before, it should be
       now as well, i.e. no need to mark the animator as needing advance() if
       it didn't need it before already. */
    const AnimationState animationStateAfter = animationState(animation, state.time);
    if(stateBefore != AnimationState::Stopped)
        CORRADE_INTERNAL_ASSERT(animationStateAfter != AnimationState::Stopped && state.state & AnimatorState::NeedsAdvance);
    #endif
}

void AbstractAnimator::stop(const AnimationHandle handle, const Nanoseconds time) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractAnimator::stop(): invalid handle" << handle, );
    stopInternal(animationHandleId(handle), time);
}

void AbstractAnimator::stop(const AnimatorDataHandle handle, const Nanoseconds time) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractAnimator::stop(): invalid handle" << handle, );
    stopInternal(animatorDataHandleId(handle), time);
}

void AbstractAnimator::stopInternal(const UnsignedInt id, const Nanoseconds time) {
    State& state = *_state;
    Animation& animation = state.animations[id];
    #ifndef CORRADE_NO_ASSERT
    const AnimationState stateBefore = animationState(animation, state.time);
    #endif
    animation.used.stopped = time;

    #ifndef CORRADE_NO_ASSERT
    /* If the animation was stopped before, it should be now as well, i.e. no
       need to mark the animator as needing advance() if it didn't need it
       before already. */
    const AnimationState animationStateAfter = animationState(animation, state.time);
    if(stateBefore == AnimationState::Stopped)
        CORRADE_INTERNAL_ASSERT(animationStateAfter == AnimationState::Stopped);
    else
        CORRADE_INTERNAL_ASSERT(state.state & AnimatorState::NeedsAdvance);
    #endif
}

Containers::StridedArrayView1D<const UnsignedShort> AbstractAnimator::generations() const {
    return stridedArrayView(_state->animations).slice(&Animation::used).slice(&Animation::Used::generation);
}

void AbstractAnimator::clean(const Containers::BitArrayView animationIdsToRemove) {
    #ifndef CORRADE_NO_ASSERT
    State& state = *_state;
    #endif
    CORRADE_ASSERT(animationIdsToRemove.size() == state.animations.size(),
        "Ui::AbstractAnimator::clean(): expected" << state.animations.size() << "bits but got" << animationIdsToRemove.size(), );

    /* Call into the implementation before removing the animations in order to
       still have node / data attachments for the implementation to use */
    doClean(animationIdsToRemove);

    /** @todo some way to efficiently iterate set bits */
    for(std::size_t i = 0; i != animationIdsToRemove.size(); ++i) {
        if(animationIdsToRemove[i]) removeInternal(i);
    }
}

void AbstractAnimator::doClean(Containers::BitArrayView) {}

void AbstractAnimator::cleanNodes(const Containers::StridedArrayView1D<const UnsignedShort>& nodeHandleGenerations) {
    CORRADE_ASSERT(features() >= AnimatorFeature::NodeAttachment,
        "Ui::AbstractAnimator::cleanNodes(): feature not supported", );

    State& state = *_state;
    /** @todo have some bump allocator for this */
    Containers::BitArray animationIdsToRemove{ValueInit, state.animations.size()};

    CORRADE_INTERNAL_ASSERT(state.nodes.size() == state.animations.size());
    for(std::size_t i = 0; i != state.nodes.size(); ++i) {
        const NodeHandle node = state.nodes[i];

        /* Skip animations that are free or that aren't attached to any node */
        if(node == NodeHandle::Null)
            continue;

        /* For used & attached animations compare the generation of the node
           they're attached to. If it differs, remove the animation and mark
           the corresponding index so the implementation can do its own cleanup
           in doClean(). */
        /** @todo check that the ID is in bounds and if it's not, remove as
            well? to avoid OOB access if the animation is accidentally attached
            to a NodeHandle from a different UI instance that has more nodes */
        if(nodeHandleGeneration(node) != nodeHandleGenerations[nodeHandleId(node)]) {
            removeInternal(i);
            animationIdsToRemove.set(i);
        }
    }

    /* As removeInternal() was already called in the above loop, we don't need
       to delegate to clean() but can call doClean() directly. Also, compared to
       clean(), the implementation is called _after_ the animations are
       removed, not before, because it's assumed that at this point the node
       handles are invalid anyway, so it doesn't make sense to access them
       from the implementation. */
    doClean(animationIdsToRemove);
}

void AbstractAnimator::cleanData(const Containers::StridedArrayView1D<const UnsignedShort>& dataHandleGenerations) {
    CORRADE_ASSERT(features() >= AnimatorFeature::DataAttachment,
        "Ui::AbstractAnimator::cleanData(): feature not supported", );
    State& state = *_state;
    CORRADE_ASSERT(state.layer != LayerHandle::Null,
        "Ui::AbstractAnimator::cleanData(): no layer set for data attachment", );

    /** @todo have some bump allocator for this */
    Containers::BitArray animationIdsToRemove{ValueInit, state.animations.size()};

    CORRADE_INTERNAL_ASSERT(state.layerData.size() == state.animations.size());
    for(std::size_t i = 0; i != state.layerData.size(); ++i) {
        const LayerDataHandle data = state.layerData[i];

        /* Skip animations that are free or that aren't attached to any data */
        if(data == LayerDataHandle::Null)
            continue;

        /* For used & attached animations compare the generation of the data
           they're attached to. If it differs, remove the animation and mark
           the corresponding index so the implementation can do its own cleanup
           in doClean(). */
        /** @todo check that the ID is in bounds and if it's not, remove as
            well? to avoid OOB access if the animation is accidentally attached
            to a LayerDataHandle from a different layer that has more data */
        if(layerDataHandleGeneration(data) != dataHandleGenerations[layerDataHandleId(data)]) {
            removeInternal(i);
            animationIdsToRemove.set(i);
        }
    }

    /* As removeInternal() was already called in the above loop, we don't need
       to delegate to clean() but can call doClean() directly. Also, compared to
       clean(), the implementation is called _after_ the animations are
       removed, not before, because it's assumed that at this point the node
       handles are invalid anyway, so it doesn't make sense to access them
       from the implementation. */
    doClean(animationIdsToRemove);
}

Containers::Pair<bool, bool> AbstractAnimator::update(const Nanoseconds time, const Containers::MutableBitArrayView active, const Containers::MutableBitArrayView started, const Containers::MutableBitArrayView stopped, const Containers::StridedArrayView1D<Float>& factors, const Containers::MutableBitArrayView remove) {
    State& state = *_state;
    CORRADE_ASSERT(active.size() == state.animations.size() &&
                   started.size() == state.animations.size() &&
                   stopped.size() == state.animations.size() &&
                   factors.size() == state.animations.size() &&
                   remove.size() == state.animations.size(),
        "Ui::AbstractAnimator::update(): expected active, started, stopped, factors and remove views to have a size of" << state.animations.size() << "but got" << active.size() << Debug::nospace << "," << started.size() << Debug::nospace << "," << stopped.size() << Debug::nospace << "," << factors.size() << "and" << remove.size(), {});
    CORRADE_ASSERT(time >= state.time,
        "Ui::AbstractAnimator::update(): expected a time at least" << state.time << "but got" << time, {});

    /* Zero all bitmasks. AbstractUserInterface::advanceAnimations()
       repeatedly reuses this memory, so without this it'd have to do an
       explicit clear in each case there otherwise. */
    active.resetAll();
    started.resetAll();
    stopped.resetAll();
    remove.resetAll();

    const Nanoseconds timeBefore = state.time;
    bool cleanNeeded = false;
    bool advanceNeeded = false;
    bool anotherAdvanceNeeded = false;
    for(std::size_t i = 0; i != state.animations.size(); ++i) {
        /* Animations with -max duration are freed items, skip */
        const Animation& animation = state.animations[i];
        if(animation.used.duration == Nanoseconds::min())
            continue;

        const AnimationState stateBefore = animationState(animation, timeBefore);
        const AnimationState stateAfter = animationState(animation, time);

        /* AnimationState has 4 values so there should be 16 different cases */
        switch((UnsignedShort(stateBefore) << 8)|UnsignedShort(stateAfter)) {
            #define _c(before, after) case (UnsignedShort(AnimationState::before) << 8)|UnsignedShort(AnimationState::after):
            _c(Scheduled,Playing)
            _c(Playing,Playing)
            _c(Scheduled,Paused)
            _c(Playing,Paused)
            _c(Scheduled,Stopped)
            _c(Playing,Stopped)
            _c(Paused,Stopped)
                active.set(i);
                started.set(i, stateBefore == AnimationState::Scheduled);
                stopped.set(i, stateAfter == AnimationState::Stopped);
                advanceNeeded = true;
                factors[i] = animationFactor(animation, time, stateAfter);
                break;

            /* These don't get advanced in any way */
            _c(Scheduled,Scheduled)
            _c(Paused,Paused)
            _c(Stopped,Stopped)
                break;

            /* These transitions shouldn't happen */
            /* LCOV_EXCL_START */
            _c(Playing,Scheduled)
            _c(Paused,Scheduled)
            _c(Paused,Playing)
            _c(Stopped,Scheduled)
            _c(Stopped,Playing)
            _c(Stopped,Paused)
                CORRADE_INTERNAL_ASSERT_UNREACHABLE();
            /* LCOV_EXCL_STOP */
            #undef _c
        }

        /* If the animation was stopped and isn't meant to be kept, schedule it
           for removal. In this case it isn't needed to ensure that it's only
           removed once, as in next advance() it'll be freed already and thus
           skipped. */
        if(stateAfter == AnimationState::Stopped && !(animation.used.flags & AnimationFlag::KeepOncePlayed)) {
            remove.set(i);
            cleanNeeded = true;
        }

        /* If the animation is still active, request another advance() */
        if(stateAfter == AnimationState::Scheduled ||
           stateAfter == AnimationState::Playing ||
           stateAfter == AnimationState::Paused)
            anotherAdvanceNeeded = true;
    }

    /* Update current time, mark the animator as needing an advance() call only
       if there are any actually active animations left */
    state.time = time;
    if(anotherAdvanceNeeded)
        state.state |= AnimatorState::NeedsAdvance;
    else
        state.state &= ~AnimatorState::NeedsAdvance;

    return {advanceNeeded, cleanNeeded};
}

AbstractGenericAnimator::AbstractGenericAnimator(AnimatorHandle handle): AbstractAnimator{handle} {}

AbstractGenericAnimator::AbstractGenericAnimator(AbstractGenericAnimator&&) noexcept = default;

AbstractGenericAnimator::~AbstractGenericAnimator() = default;

AbstractGenericAnimator& AbstractGenericAnimator::operator=(AbstractGenericAnimator&&) noexcept = default;

void AbstractGenericAnimator::setLayer(const AbstractLayer& layer) {
    CORRADE_ASSERT(features() & AnimatorFeature::DataAttachment,
        "Ui::AbstractGenericAnimator::setLayer(): feature not supported", );
    CORRADE_ASSERT(this->layer() == LayerHandle::Null,
        "Ui::AbstractGenericAnimator::setLayer(): layer already set to" << this->layer(), );

    setLayerInternal(layer);
}

void AbstractGenericAnimator::advance(const Containers::BitArrayView active, const Containers::BitArrayView started, const Containers::BitArrayView stopped, const Containers::StridedArrayView1D<const Float>& factors) {
    CORRADE_ASSERT(active.size() == capacity() &&
                   started.size() == capacity() &&
                   stopped.size() == capacity() &&
                   factors.size() == capacity(),
        "Ui::AbstractGenericAnimator::advance(): expected active, started, stopped and factors views to have a size of" << capacity() << "but got" << active.size() << Debug::nospace << "," << started.size() << Debug::nospace << "," << stopped.size() << "and" << factors.size(), );
    doAdvance(active, started, stopped, factors);
}

AbstractNodeAnimator::AbstractNodeAnimator(AnimatorHandle handle): AbstractAnimator{handle} {}

AbstractNodeAnimator::AbstractNodeAnimator(AbstractNodeAnimator&&) noexcept = default;

AbstractNodeAnimator::~AbstractNodeAnimator() = default;

AbstractNodeAnimator& AbstractNodeAnimator::operator=(AbstractNodeAnimator&&) noexcept = default;

AnimatorFeatures AbstractNodeAnimator::doFeatures() const {
    return AnimatorFeature::NodeAttachment;
}

NodeAnimatorUpdates AbstractNodeAnimator::advance(const Containers::BitArrayView active, const Containers::BitArrayView started, const Containers::BitArrayView stopped, const Containers::StridedArrayView1D<const Float>& factors, const Containers::StridedArrayView1D<Vector2>& nodeOffsets, const Containers::StridedArrayView1D<Vector2>& nodeSizes, const Containers::StridedArrayView1D<Float>& nodeOpacities, const Containers::StridedArrayView1D<NodeFlags>& nodeFlags, const Containers::MutableBitArrayView nodesRemove) {
    CORRADE_ASSERT(active.size() == capacity() &&
                   started.size() == capacity() &&
                   stopped.size() == capacity() &&
                   factors.size() == capacity(),
        "Ui::AbstractNodeAnimator::advance(): expected active, started, stopped and factors views to have a size of" << capacity() << "but got" << active.size() << Debug::nospace << "," << started.size() << Debug::nospace << "," << stopped.size() << "and" << factors.size(), {});
    CORRADE_ASSERT(nodeOffsets.size() == nodeSizes.size() &&
                   nodeOpacities.size() == nodeSizes.size() &&
                   nodeFlags.size() == nodeSizes.size() &&
                   nodesRemove.size() == nodeSizes.size(),
        "Ui::AbstractNodeAnimator::advance(): expected node offset, size, opacity, flags and remove views to have the same size but got" << nodeOffsets.size() << Debug::nospace << "," << nodeSizes.size() << Debug::nospace << "," << nodeOpacities.size() << Debug::nospace << "," << nodeFlags.size() << "and" << nodesRemove.size(), {});
    return doAdvance(active, started, stopped, factors, nodeOffsets, nodeSizes, nodeOpacities, nodeFlags, nodesRemove);
}

AbstractDataAnimator::AbstractDataAnimator(AnimatorHandle handle): AbstractAnimator{handle} {}

AbstractDataAnimator::AbstractDataAnimator(AbstractDataAnimator&&) noexcept = default;

AbstractDataAnimator::~AbstractDataAnimator() = default;

AbstractDataAnimator& AbstractDataAnimator::operator=(AbstractDataAnimator&&) noexcept = default;

AnimatorFeatures AbstractDataAnimator::doFeatures() const {
    return AnimatorFeature::DataAttachment;
}

AbstractStyleAnimator::AbstractStyleAnimator(AnimatorHandle handle): AbstractAnimator{handle} {}

AbstractStyleAnimator::AbstractStyleAnimator(AbstractStyleAnimator&&) noexcept = default;

AbstractStyleAnimator::~AbstractStyleAnimator() = default;

AbstractStyleAnimator& AbstractStyleAnimator::operator=(AbstractStyleAnimator&&) noexcept = default;

AnimatorFeatures AbstractStyleAnimator::doFeatures() const {
    return AnimatorFeature::DataAttachment;
}

}}
