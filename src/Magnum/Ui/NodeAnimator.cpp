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

#include "NodeAnimator.h"

#include <Corrade/Containers/BitArrayView.h>
#include <Corrade/Containers/GrowableArray.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Containers/StringView.h>
#include <Magnum/Math/Functions.h>
#include <Magnum/Math/Time.h>

#include "Magnum/Ui/NodeFlags.h"
#include "Magnum/Ui/Handle.h"

namespace Magnum { namespace Ui {

namespace {

enum class NodeAnimationFlag: UnsignedShort {
    RemoveNodeAfter = 1 << 0,
    HasSourceOffsetX = 1 << 1,
    HasSourceOffsetY = 1 << 2,
    HasTargetOffsetX = 1 << 3,
    HasTargetOffsetY = 1 << 4,
    HasSourceSizeX = 1 << 5,
    HasSourceSizeY = 1 << 6,
    HasTargetSizeX = 1 << 7,
    HasTargetSizeY = 1 << 8,
    HasSourceOpacity = 1 << 9,
    HasTargetOpacity = 1 << 10
};

typedef Containers::EnumSet<NodeAnimationFlag> NodeAnimationFlags;

#ifdef CORRADE_TARGET_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#endif
CORRADE_ENUMSET_OPERATORS(NodeAnimationFlags)
#ifdef CORRADE_TARGET_CLANG
#pragma clang diagnostic pop
#endif

struct Animation {
    /* As the Animation entries get recycled, all fields have to be overwritten
       always, thus there's no point in initializing them on the first ever
       construction either */
    Vector2 sourceOffset{NoInit}, targetOffset{NoInit};
    Vector2 sourceSize{NoInit}, targetSize{NoInit};
    Float sourceOpacity, targetOpacity;
    Float(*easing)(Float);
    NodeFlags flagsAddBegin,
        flagsAddEnd,
        flagsClearBegin,
        flagsClearEnd;
    /* Offset / size / opacity components that aren't specified are taken from
       the node itself at the time it's started. Unlike with NodeAnimation
       can't just make the fields a NaN because when the animation gets
       restarted, the field would contain the actual fetched values from last
       time, so they have a NodeAnimationFlag bit set here. */
    NodeAnimationFlags flags;
    /* 2 bytes free */
};

}

struct NodeAnimator::State {
    Containers::Array<Animation> animations;
};

NodeAnimator::NodeAnimator(const AnimatorHandle handle): AbstractNodeAnimator{handle}, _state{InPlaceInit} {}

NodeAnimator::NodeAnimator(NodeAnimator&&) noexcept = default;

NodeAnimator::~NodeAnimator() = default;

NodeAnimator& NodeAnimator::operator=(NodeAnimator&&) noexcept = default;

AnimationHandle NodeAnimator::create(const NodeAnimation& animation, Float(*const easing)(Float), const Nanoseconds start, const Nanoseconds duration, const NodeHandle node, const UnsignedInt repeatCount, const AnimationFlags flags) {
    CORRADE_ASSERT(easing ||
        (Math::isNan(animation._sourceOffset).all() &&
         Math::isNan(animation._targetOffset).all() &&
         Math::isNan(animation._sourceSize).all() &&
         Math::isNan(animation._targetSize).all() &&
         Math::isNan(animation._sourceOpacity) &&
         Math::isNan(animation._targetOpacity)),
        "Ui::NodeAnimator::create(): easing expected to be non-null if animating offset, size or opacity", {});

    const AnimationHandle handle = AbstractNodeAnimator::create(start, duration, node, repeatCount, flags);

    State& state = static_cast<State&>(*_state);
    const UnsignedInt id = animationHandleId(handle);
    if(id >= state.animations.size())
        arrayResize(state.animations, NoInit, id + 1);

    Animation& data = _state->animations[animationHandleId(handle)];
    /* Copy all values, including the NaNs, for simplicity but then for each of
       the values, animate it only if at least one component is not a NaN.
       Then, for those, components that are unset here get populated as soon as
       the animation is marked as started in advance(). */
    data.sourceOffset = animation._sourceOffset;
    data.targetOffset = animation._targetOffset;
    data.sourceSize = animation._sourceSize;
    data.targetSize = animation._targetSize;
    data.sourceOpacity = animation._sourceOpacity;
    data.targetOpacity = animation._targetOpacity;
    data.easing = easing;
    data.flagsAddBegin = animation._flagsAddBegin;
    data.flagsAddEnd = animation._flagsAddEnd;
    data.flagsClearBegin = animation._flagsClearBegin;
    data.flagsClearEnd = animation._flagsClearEnd;
    data.flags = {}; /* Ensure flags don't contain stale bits from before */
    if(!Math::isNan(animation._sourceOffset.x()))
        data.flags |= NodeAnimationFlag::HasSourceOffsetX;
    if(!Math::isNan(animation._sourceOffset.y()))
        data.flags |= NodeAnimationFlag::HasSourceOffsetY;
    if(!Math::isNan(animation._targetOffset.x()))
        data.flags |= NodeAnimationFlag::HasTargetOffsetX;
    if(!Math::isNan(animation._targetOffset.y()))
        data.flags |= NodeAnimationFlag::HasTargetOffsetY;
    if(!Math::isNan(animation._sourceSize.x()))
        data.flags |= NodeAnimationFlag::HasSourceSizeX;
    if(!Math::isNan(animation._sourceSize.y()))
        data.flags |= NodeAnimationFlag::HasSourceSizeY;
    if(!Math::isNan(animation._targetSize.x()))
        data.flags |= NodeAnimationFlag::HasTargetSizeX;
    if(!Math::isNan(animation._targetSize.y()))
        data.flags |= NodeAnimationFlag::HasTargetSizeY;
    if(!Math::isNan(animation._sourceOpacity))
        data.flags |= NodeAnimationFlag::HasSourceOpacity;
    if(!Math::isNan(animation._targetOpacity))
        data.flags |= NodeAnimationFlag::HasTargetOpacity;
    if(animation._removeNodeAfter)
        data.flags |= NodeAnimationFlag::RemoveNodeAfter;
    return handle;
}

AnimationHandle NodeAnimator::create(const NodeAnimation& animation, Float(*const easing)(Float), const Nanoseconds start, const Nanoseconds duration, const NodeHandle node, const AnimationFlags flags) {
    return create(animation, easing, start, duration, node, 1, flags);
}

Containers::Pair<Vector2, Vector2> NodeAnimator::offsets(const AnimationHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::NodeAnimator::offsets(): invalid handle" << handle, {});
    return offsetsInternal(animationHandleId(handle));
}

Containers::Pair<Vector2, Vector2> NodeAnimator::offsets(const AnimatorDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::NodeAnimator::offsets(): invalid handle" << handle, {});
    return offsetsInternal(animatorDataHandleId(handle));
}

Containers::Pair<Vector2, Vector2> NodeAnimator::offsetsInternal(const UnsignedInt id) const {
    const Animation& animation = _state->animations[id];
    return {
        Math::lerp(Vector2{Constants::nan()}, animation.sourceOffset, BitVector2{UnsignedByte(
            (animation.flags >= NodeAnimationFlag::HasSourceOffsetX ? 1 : 0)|
            (animation.flags >= NodeAnimationFlag::HasSourceOffsetY ? 2 : 0))}),
        Math::lerp(Vector2{Constants::nan()}, animation.targetOffset, BitVector2{UnsignedByte(
            (animation.flags >= NodeAnimationFlag::HasTargetOffsetX ? 1 : 0)|
            (animation.flags >= NodeAnimationFlag::HasTargetOffsetY ? 2 : 0))})
    };
}

Containers::Pair<Vector2, Vector2> NodeAnimator::sizes(const AnimationHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::NodeAnimator::sizes(): invalid handle" << handle, {});
    return sizesInternal(animationHandleId(handle));
}

Containers::Pair<Vector2, Vector2> NodeAnimator::sizes(const AnimatorDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::NodeAnimator::sizes(): invalid handle" << handle, {});
    return sizesInternal(animatorDataHandleId(handle));
}

Containers::Pair<Vector2, Vector2> NodeAnimator::sizesInternal(const UnsignedInt id) const {
    const Animation& animation = _state->animations[id];
    return {
        Math::lerp(Vector2{Constants::nan()}, animation.sourceSize, BitVector2{UnsignedByte(
            (animation.flags >= NodeAnimationFlag::HasSourceSizeX ? 1 : 0)|
            (animation.flags >= NodeAnimationFlag::HasSourceSizeY ? 2 : 0))}),
        Math::lerp(Vector2{Constants::nan()}, animation.targetSize, BitVector2{UnsignedByte(
            (animation.flags >= NodeAnimationFlag::HasTargetSizeX ? 1 : 0)|
            (animation.flags >= NodeAnimationFlag::HasTargetSizeY ? 2 : 0))})
    };
}

Containers::Pair<Float, Float> NodeAnimator::opacities(const AnimationHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::NodeAnimator::opacities(): invalid handle" << handle, {});
    return opacitiesInternal(animationHandleId(handle));
}

Containers::Pair<Float, Float> NodeAnimator::opacities(const AnimatorDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::NodeAnimator::opacities(): invalid handle" << handle, {});
    return opacitiesInternal(animatorDataHandleId(handle));
}

Containers::Pair<Float, Float> NodeAnimator::opacitiesInternal(const UnsignedInt id) const {
    const Animation& animation = _state->animations[id];
    return {
        animation.flags >= NodeAnimationFlag::HasSourceOpacity ? animation.sourceOpacity : Constants::nan(),
        animation.flags >= NodeAnimationFlag::HasTargetOpacity ? animation.targetOpacity : Constants::nan(),
    };
}

Containers::Pair<NodeFlags, NodeFlags> NodeAnimator::flagsAdd(const AnimationHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::NodeAnimator::flagsAdd(): invalid handle" << handle, {});
    const Animation& animation = _state->animations[animationHandleId(handle)];
    return {animation.flagsAddBegin, animation.flagsAddEnd};
}

Containers::Pair<NodeFlags, NodeFlags> NodeAnimator::flagsAdd(const AnimatorDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::NodeAnimator::flagsAdd(): invalid handle" << handle, {});
    const Animation& animation = _state->animations[animatorDataHandleId(handle)];
    return {animation.flagsAddBegin, animation.flagsAddEnd};
}

Containers::Pair<NodeFlags, NodeFlags> NodeAnimator::flagsClear(const AnimationHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::NodeAnimator::flagsClear(): invalid handle" << handle, {});
    const Animation& animation = _state->animations[animationHandleId(handle)];
    return {animation.flagsClearBegin, animation.flagsClearEnd};
}

Containers::Pair<NodeFlags, NodeFlags> NodeAnimator::flagsClear(const AnimatorDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::NodeAnimator::flagsClear(): invalid handle" << handle, {});
    const Animation& animation = _state->animations[animatorDataHandleId(handle)];
    return {animation.flagsClearBegin, animation.flagsClearEnd};
}

bool NodeAnimator::hasRemoveNodeAfter(const AnimationHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::NodeAnimator::hasRemoveNodeAfter(): invalid handle" << handle, {});
    return _state->animations[animationHandleId(handle)].flags >= NodeAnimationFlag::RemoveNodeAfter;
}

bool NodeAnimator::hasRemoveNodeAfter(const AnimatorDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::NodeAnimator::hasRemoveNodeAfter(): invalid handle" << handle, {});
    return _state->animations[animatorDataHandleId(handle)].flags >= NodeAnimationFlag::RemoveNodeAfter;
}

auto NodeAnimator::easing(const AnimationHandle handle) const -> Float(*)(Float) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::NodeAnimator::easing(): invalid handle" << handle, {});
    return _state->animations[animationHandleId(handle)].easing;
}

auto NodeAnimator::easing(const AnimatorDataHandle handle) const -> Float(*)(Float) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::NodeAnimator::easing(): invalid handle" << handle, {});
    return _state->animations[animatorDataHandleId(handle)].easing;
}

AnimatorFeatures NodeAnimator::doFeatures() const {
    return AnimatorFeature::NodeAttachment;
}

namespace {

NodeAnimatorUpdates updatesForFlags(NodeFlags flagsBefore, NodeFlags flags) {
    NodeAnimatorUpdates updates;
    /* Not check just if the flag was in flagsAdd / flagsClear because that'd
       unnecessarily trigger an update even if a flag that's already present is
       added or a flag that isn't present is cleared */
    if((flags ^ flagsBefore) & NodeFlag::NoBlur)
        updates |= NodeAnimatorUpdate::EventMask;
    /* This correctly handles also the case where Disabled is replaced with
       NoEvents (which is its subset) and vice versa. See
       NodeAnimatorTest::advanceProperties() for test cases. */
    if((flags ^ flagsBefore) & (NodeFlag::NoEvents|NodeFlag::Disabled|NodeFlag::Focusable))
        updates |= NodeAnimatorUpdate::Enabled;
    if((flags ^ flagsBefore) & NodeFlag::Clip)
        updates |= NodeAnimatorUpdate::Clip;
    if((flags ^ flagsBefore) & NodeFlag::Hidden)
        updates |= NodeAnimatorUpdate::Visibility;
    return updates;
}

}

NodeAnimatorUpdates NodeAnimator::doAdvance(const Containers::BitArrayView active, const Containers::BitArrayView started, const Containers::BitArrayView stopped, const Containers::StridedArrayView1D<const Float>& factors, const Containers::StridedArrayView1D<Vector2>& nodeOffsets, const Containers::StridedArrayView1D<Vector2>& nodeSizes, const Containers::StridedArrayView1D<Float>& nodeOpacities, const Containers::StridedArrayView1D<NodeFlags>& nodeFlags, const Containers::MutableBitArrayView nodesRemove) {
    State& state = static_cast<State&>(*_state);
    const Containers::StridedArrayView1D<const NodeHandle> nodes = this->nodes();
    const Containers::StridedArrayView1D<const AnimationFlags> flags = this->flags();

    /** @todo some way to iterate set bits */
    NodeAnimatorUpdates updates;
    for(std::size_t i = 0; i != active.size(); ++i) {
        /* Besides animations that aren't active, there's also nothing to do if
           there's no node to affect */
        if(!active[i] || nodes[i] == NodeHandle::Null)
            continue;

        Animation& animation = state.animations[i];
        const UnsignedInt nodeId = nodeHandleId(nodes[i]);
        const bool reverse = flags[i] >= AnimationFlag::Reverse;

        /* Operations to do only at animation start */
        if(started[i]) {
            /* If an animation animates the X offset (i.e., at least one from
               source or target is set), but either the source or target is
               meant to be the current node offset, save it  */
            if(!(animation.flags >= NodeAnimationFlag::HasSourceOffsetX) &&
                 animation.flags >= NodeAnimationFlag::HasTargetOffsetX)
                animation.sourceOffset.x() = nodeOffsets[nodeId].x();
            else if(animation.flags >= NodeAnimationFlag::HasSourceOffsetX &&
                  !(animation.flags >= NodeAnimationFlag::HasTargetOffsetX))
                animation.targetOffset.x() = nodeOffsets[nodeId].x();
            /* Same for Y */
            if(!(animation.flags >= NodeAnimationFlag::HasSourceOffsetY) &&
                 animation.flags >= NodeAnimationFlag::HasTargetOffsetY)
                animation.sourceOffset.y() = nodeOffsets[nodeId].y();
            else if(animation.flags >= NodeAnimationFlag::HasSourceOffsetY &&
                  !(animation.flags >= NodeAnimationFlag::HasTargetOffsetY))
                animation.targetOffset.y() = nodeOffsets[nodeId].y();

            /* Same for size X ... */
            if(!(animation.flags >= NodeAnimationFlag::HasSourceSizeX) &&
                 animation.flags >= NodeAnimationFlag::HasTargetSizeX)
                animation.sourceSize.x() = nodeSizes[nodeId].x();
            else if(animation.flags >= NodeAnimationFlag::HasSourceSizeX &&
                  !(animation.flags >= NodeAnimationFlag::HasTargetSizeX))
                animation.targetSize.x() = nodeSizes[nodeId].x();
            /* Same for Y */
            if(!(animation.flags >= NodeAnimationFlag::HasSourceSizeY) &&
                 animation.flags >= NodeAnimationFlag::HasTargetSizeY)
                animation.sourceSize.y() = nodeSizes[nodeId].y();
            else if(animation.flags >= NodeAnimationFlag::HasSourceSizeY &&
                  !(animation.flags >= NodeAnimationFlag::HasTargetSizeY))
                animation.targetSize.y() = nodeSizes[nodeId].y();

            /* ... and opacity */
            if(!(animation.flags >= NodeAnimationFlag::HasSourceOpacity) &&
                 animation.flags >= NodeAnimationFlag::HasTargetOpacity)
                animation.sourceOpacity = nodeOpacities[nodeId];
            else if(animation.flags >= NodeAnimationFlag::HasSourceOpacity &&
                  !(animation.flags >= NodeAnimationFlag::HasTargetOpacity))
                animation.targetOpacity = nodeOpacities[nodeId];

            /* Flags to add or clear at the start. If the animation is
               reversed, take the flags from the end instead of the begin and
               swap the add and clear operations for those. So, for example, if
               a NodeFlag::NoEvents is to be added at begin, it gets cleared
               when the reversed animation stops. */
            const NodeFlags flagsAddStart = reverse ?
                animation.flagsClearEnd : animation.flagsAddBegin;
            const NodeFlags flagsClearStart = reverse ?
                animation.flagsAddEnd : animation.flagsClearBegin;
            if(flagsAddStart || flagsClearStart) {
                NodeFlags& flags = nodeFlags[nodeId];
                const NodeFlags flagsBefore = flags;

                /* Clear first so it's possible to implicitly clear all flags
                   and then add a subset back */
                flags &= ~flagsClearStart;
                flags |= flagsAddStart;

                /* If presence of certain flags changed, reflect that in the
                   output NodeAnimatorUpdates */
                updates |= updatesForFlags(flagsBefore, flags);
            }
        }

        /* Actual animation of node offset, size and opacity */
        /** @todo maybe skip this if the node is about to be removed? test by
            verifying that the NodeAnimatorUpdate flags aren't set (and fields
            not filled) in that case */
        if(animation.flags & (
            NodeAnimationFlag::HasSourceOffsetX|
            NodeAnimationFlag::HasSourceOffsetY|
            NodeAnimationFlag::HasTargetOffsetX|
            NodeAnimationFlag::HasTargetOffsetY|
            NodeAnimationFlag::HasSourceSizeX|
            NodeAnimationFlag::HasSourceSizeY|
            NodeAnimationFlag::HasTargetSizeX|
            NodeAnimationFlag::HasTargetSizeY|
            NodeAnimationFlag::HasSourceOpacity|
            NodeAnimationFlag::HasTargetOpacity
        )) {
            /* The easing is guaranteed to be non-null if offset, size or
               opacity is animated */
            const Float factor = animation.easing(factors[i]);

            /* Interpolate if the animation animates offset / size / opacity.
               For source/target values that are meant to be taken from the
               node, it was fetched above already. */
            if(animation.flags & (NodeAnimationFlag::HasSourceOffsetX|
                                  NodeAnimationFlag::HasTargetOffsetX)) {
                nodeOffsets[nodeId].x() = Math::lerp(animation.sourceOffset.x(), animation.targetOffset.x(), factor);
                updates |= NodeAnimatorUpdate::OffsetSize;
            }
            if(animation.flags & (NodeAnimationFlag::HasSourceOffsetY|
                                  NodeAnimationFlag::HasTargetOffsetY)) {
                nodeOffsets[nodeId].y() = Math::lerp(animation.sourceOffset.y(), animation.targetOffset.y(), factor);
                updates |= NodeAnimatorUpdate::OffsetSize;
            }
            if(animation.flags & (NodeAnimationFlag::HasSourceSizeX|
                                  NodeAnimationFlag::HasTargetSizeX)) {
                nodeSizes[nodeId].x() = Math::lerp(animation.sourceSize.x(), animation.targetSize.x(), factor);
                updates |= NodeAnimatorUpdate::OffsetSize;
            }
            if(animation.flags & (NodeAnimationFlag::HasSourceSizeY|
                                  NodeAnimationFlag::HasTargetSizeY)) {
                nodeSizes[nodeId].y() = Math::lerp(animation.sourceSize.y(), animation.targetSize.y(), factor);
                updates |= NodeAnimatorUpdate::OffsetSize;
            }
            if(animation.flags & (NodeAnimationFlag::HasSourceOpacity|
                                  NodeAnimationFlag::HasTargetOpacity)) {
                nodeOpacities[nodeId] = Math::lerp(animation.sourceOpacity, animation.targetOpacity, factor);
                updates |= NodeAnimatorUpdate::Opacity;
            }
        }

        /* Operations to do only at animation stop */
        if(stopped[i]) {
            /* Flags to add or clear at the stop. If the animation is
               reversed, take the flags from the begin instead of the end and
               swap the add and clear operations for those. So, for example, if
               a NodeFlag::Hidden is to be added at end, it gets cleared when
               the reversed animation starts. */
            const NodeFlags flagsAddStop = reverse ?
                animation.flagsClearBegin : animation.flagsAddEnd;
            const NodeFlags flagsClearStop = reverse ?
                animation.flagsAddBegin : animation.flagsClearEnd;
            /** @todo maybe skip this if the node is about to be removed? I.e.,
                do all this before the interpolations? */
            if(flagsAddStop || flagsClearStop) {
                NodeFlags& flags = nodeFlags[nodeId];
                const NodeFlags flagsBefore = flags;

                /* Clear first so it's possible to implicitly clear all flags
                   and then add a subset back */
                flags &= ~flagsClearStop;
                flags |= flagsAddStop;

                /* If presence of certain flags changed, reflect that in the
                   output NodeAnimatorUpdates */
                updates |= updatesForFlags(flagsBefore, flags);
            }

            /* Remove the node afterwards if requested */
            if(animation.flags >= NodeAnimationFlag::RemoveNodeAfter) {
                updates |= NodeAnimatorUpdate::Removal;
                nodesRemove.set(nodeId);
            }
        }
    }

    return updates;
}

Containers::Pair<Vector2, Vector2> NodeAnimation::offsets() const {
    return {_sourceOffset, _targetOffset};
}

Containers::Pair<Vector2, Vector2> NodeAnimation::sizes() const {
    return {_sourceSize, _targetSize};
}

Containers::Pair<Float, Float> NodeAnimation::opacities() const {
    return {_sourceOpacity, _targetOpacity};
}

Containers::Pair<NodeFlags, NodeFlags> NodeAnimation::flagsAdd() const {
    return {_flagsAddBegin, _flagsAddEnd};
}

Containers::Pair<NodeFlags, NodeFlags> NodeAnimation::flagsClear() const {
    return {_flagsClearBegin, _flagsClearEnd};
}

void NodeAnimator::DebugIntegration::print(Debug& debug, const NodeAnimator& animator, const Containers::StringView& animatorName, AnimatorDataHandle animation) {
    debug << " " << Debug::color(Debug::Color::Cyan) << Debug::packed << animator.state(animation) << Debug::resetColor << "animation" << Debug::packed << animation << "from animator" << Debug::packed << animator.handle();
    if(animatorName)
        debug << Debug::color(Debug::Color::Yellow) << animatorName << Debug::resetColor;
    debug << Debug::newline;

    const AnimationFlags flags = animator.flags(animation);
    if(flags) {
        debug << "    Animation flags:" << Debug::color(Debug::Color::Cyan) << Debug::packed << flags << Debug::resetColor << Debug::newline;
    }

    /* For offset, size and opacity the reversed direction is indicated by
       different arrows instead of swapping the values, as with
       ReverseEveryOther the direction changes every iteration and the order
       could be already out of date when printed. There's also no special
       indication for Reverse|ReverseEveryOther, but the animation flags are
       printed at the top for the user to decide. */
    const char* direction;
    if(flags & AnimationFlag::ReverseEveryOther)
        direction = "<->";
    else if(flags & AnimationFlag::Reverse)
        direction = "<-";
    else
        direction = "->";

    /* Node offset */
    {
        const Containers::Pair<Vector2, Vector2> offsets = animator.offsets(animation);
        if(!(Math::isNan(offsets.first()) &
             Math::isNan(offsets.second())).any())
        {
            debug << "    Offset: {" << Debug::nospace;
            if(Math::isNan(offsets.first().x()))
                debug << "?";
            else
                debug << offsets.first().x();
            debug << Debug::nospace << ",";
            if(Math::isNan(offsets.first().y()))
                debug << "?";
            else
                debug << offsets.first().y();
            debug << Debug::nospace << "}" << direction << "{" << Debug::nospace;
            if(Math::isNan(offsets.second().x()))
                debug << "?";
            else
                debug << offsets.second().x();
            debug << Debug::nospace << ",";
            if(Math::isNan(offsets.second().y()))
                debug << "?";
            else
                debug << offsets.second().y();
            debug << Debug::nospace << "}" << Debug::newline;
        } else if(!Math::isNan(offsets.first().x()) ||
                  !Math::isNan(offsets.second().x()))
        {
            debug << "    Offset X:";
            if(Math::isNan(offsets.first().x()))
                debug << "?";
            else
                debug << offsets.first().x();
            debug << direction;
            if(Math::isNan(offsets.second().x()))
                debug << "?";
            else
                debug << offsets.second().x();
            debug << Debug::newline;
        } else if(!Math::isNan(offsets.first().y()) ||
                  !Math::isNan(offsets.second().y()))
        {
            debug << "    Offset Y:";
            if(Math::isNan(offsets.first().y()))
                debug << "?";
            else
                debug << offsets.first().y();
            debug << direction;
            if(Math::isNan(offsets.second().y()))
                debug << "?";
            else
                debug << offsets.second().y();
            debug << Debug::newline;
        } else CORRADE_INTERNAL_ASSERT(Math::isNan(offsets.first()).all() &&
                                       Math::isNan(offsets.second()).all());
    }

    /* Node size */
    {
        const Containers::Pair<Vector2, Vector2> sizes = animator.sizes(animation);
        if(!(Math::isNan(sizes.first()) &
             Math::isNan(sizes.second())).any())
        {
            debug << "    Size: {" << Debug::nospace;
            if(Math::isNan(sizes.first().x()))
                debug << "?";
            else
                debug << sizes.first().x();
            debug << Debug::nospace << ",";
            if(Math::isNan(sizes.first().y()))
                debug << "?";
            else
                debug << sizes.first().y();
            debug << Debug::nospace << "}" << direction << "{" << Debug::nospace;
            if(Math::isNan(sizes.second().x()))
                debug << "?";
            else
                debug << sizes.second().x();
            debug << Debug::nospace << ",";
            if(Math::isNan(sizes.second().y()))
                debug << "?";
            else
                debug << sizes.second().y();
            debug << Debug::nospace << "}" << Debug::newline;
        } else if(!Math::isNan(sizes.first().x()) ||
                  !Math::isNan(sizes.second().x()))
        {
            debug << "    Size X:";
            if(Math::isNan(sizes.first().x()))
                debug << "?";
            else
                debug << sizes.first().x();
            debug << direction;
            if(Math::isNan(sizes.second().x()))
                debug << "?";
            else
                debug << sizes.second().x();
            debug << Debug::newline;
        } else if(!Math::isNan(sizes.first().y()) ||
                  !Math::isNan(sizes.second().y()))
        {
            debug << "    Size Y:";
            if(Math::isNan(sizes.first().y()))
                debug << "?";
            else
                debug << sizes.first().y();
            debug << direction;
            if(Math::isNan(sizes.second().y()))
                debug << "?";
            else
                debug << sizes.second().y();
            debug << Debug::newline;
        } else CORRADE_INTERNAL_ASSERT(Math::isNan(sizes.first()).all() &&
                                       Math::isNan(sizes.second()).all());
    }

    /* Node opacity */
    {
        const Containers::Pair<Float, Float> opacities = animator.opacities(animation);
        if(!Math::isNan(opacities.first()) ||
           !Math::isNan(opacities.second()))
        {
            debug << "    Opacity:";
            if(Math::isNan(opacities.first()))
                debug << "?";
            else
                debug << opacities.first();
            debug << direction;
            if(Math::isNan(opacities.second()))
                debug << "?";
            else
                debug << opacities.second();
            debug << Debug::newline;
        }
    }

    /* Node flags to add or clear */
    {
        /* If the animation is reversed, the flags are applied in reverse as
           well -- cleared at the end instead of being added at the begin,
           etc... */
        Containers::Pair<NodeFlags, NodeFlags> flagsAdd = animator.flagsAdd(animation);
        Containers::Pair<NodeFlags, NodeFlags> flagsClear = animator.flagsClear(animation);
        if(flags & AnimationFlag::Reverse) {
            Utility::swap(flagsAdd.first(), flagsAdd.second());
            Utility::swap(flagsClear.first(), flagsClear.second());
            Utility::swap(flagsAdd, flagsClear);
        }

        /* ... which is also why this is using start/stop and not begin/end,
           because due to the swap above it describes what happens at start and
           stop even if the animation is reversed */
        if(flagsAdd.first() && flagsAdd.second()) {
            debug << "    Add flags start:" << Debug::color(Debug::Color::Cyan) << Debug::packed << flagsAdd.first() << Debug::resetColor << Debug::nospace << ", stop:" << Debug::color(Debug::Color::Cyan) << Debug::packed << flagsAdd.second() << Debug::resetColor << Debug::newline;
        } else if(flagsAdd.first()) {
            debug << "    Add flags start:" << Debug::color(Debug::Color::Cyan) << Debug::packed << flagsAdd.first() << Debug::resetColor << Debug::newline;
        } else if(flagsAdd.second()) {
            debug << "    Add flags stop:" << Debug::color(Debug::Color::Cyan) << Debug::packed << flagsAdd.second() << Debug::resetColor << Debug::newline;
        }
        if(flagsClear.first() && flagsClear.second()) {
            debug << "    Clear flags start:" << Debug::color(Debug::Color::Cyan) << Debug::packed << flagsClear.first() << Debug::resetColor << Debug::nospace << ", stop:" << Debug::color(Debug::Color::Cyan) << Debug::packed << flagsClear.second() << Debug::resetColor << Debug::newline;
        } else if(flagsClear.first()) {
            debug << "    Clear flags start:" << Debug::color(Debug::Color::Cyan) << Debug::packed << flagsClear.first() << Debug::resetColor << Debug::newline;
        } else if(flagsClear.second()) {
            debug << "    Clear flags stop:" << Debug::color(Debug::Color::Cyan) << Debug::packed << flagsClear.second() << Debug::resetColor << Debug::newline;
        }
    }

    /* Remove node after */
    if(animator.hasRemoveNodeAfter(animation))
        debug << "    " << Debug::color(Debug::Color::Magenta) << Debug::nospace << "Remove node after" << Debug::resetColor << Debug::newline;
}

}}
