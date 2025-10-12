#ifndef Magnum_Ui_NodeAnimator_h
#define Magnum_Ui_NodeAnimator_h
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

/** @file
 * @brief Class @ref Magnum::Ui::NodeAnimator, @ref Magnum::Ui::NodeAnimation
 * @m_since_latest
 */

#include <Magnum/Math/Vector2.h> /* for NodeAnimation members */

#include "Magnum/Ui/AbstractAnimator.h"

namespace Magnum { namespace Ui {

class NodeAnimation;

/**
@brief Node animator
@m_since_latest

Each animation interpolates between given node offset, size and opacity
endpoints, optionally modifying node flags and allowing to automatically remove
the node once the animation stops. If you need to execute arbitrary extra code
during a node animation, use @ref GenericNodeAnimator.

@section Ui-NodeAnimator-setup Setting up an animator instance

The animator doesn't have any shared state or configuration, so it's just about
constructing it from a fresh @ref AbstractUserInterface::createAnimator()
handle and passing it to @relativeref{AbstractUserInterface,setNodeAnimatorInstance()}.
After that, assuming @ref AbstractUserInterface::advanceAnimations() is called
in an appropriate place, it's ready to use.

@snippet Ui.cpp NodeAnimator-setup

Unlike builtin layers or layouters, the default @ref UserInterface
implementation doesn't implicitly provide a @ref NodeAnimator instance.

@section Ui-NodeAnimator-create Creating animations

An animation is created by calling @ref create() with a @ref NodeAnimation
helper that specifies which properties --- node offset, size, opacity or flags
--- should be animated, an easing function from @ref Animation::BasicEasing "Animation::Easing"
or a custom one, time at which it's meant to start, its duration and the
@ref NodeHandle it's attached to. Properties not specified in the passed
@ref NodeAnimation instance stay untouched by the animation.

The following code animates a popup sliding from the top edge of the screen to
the center. Besides that, it enables @ref NodeFlag::NoEvents at the beginning
of the animation and clears it again at the end to prevent accidental and
likely imprecise interaction while the popup is moving:

@snippet Ui.cpp NodeAnimator-create-offset

If either the source or the target offset, size or opacity value is omitted,
the animation picks the value the node has at the time the animation is being
played. In the following snippet, a context menu is being closed, shrinking it
from its current size to zero. @ref NodeFlag::Clip is enabled so the contents
get clipped during resize and, since it becomes invisible after,
@ref NodeFlag::Hidden is added at the end:

@snippet Ui.cpp NodeAnimator-create-size

It's also possible to animate just horizontal or vertical offset and size
alone, with the other direction staying unchanged, using
@ref NodeAnimation::fromOffsetX(), @relativeref{NodeAnimation,fromOffsetY()}
etc. Setting both components separately is the same as calling the vector
@relativeref{NodeAnimation,fromOffset()} etc. variants.

With @ref NodeAnimation::setRemoveNodeAfter() an animation can remove the node
afterwards, which is useful for example to automatically remove a one-time
notification once it fades out. In this case, it fades to fully transparent
after a ten-second delay:

@snippet Ui.cpp NodeAnimator-create-remove-node-after

@subsection Ui-NodeAnimation-create-reversible Reversible animations

In cases where for example opening and closing a menu is animated the same way,
just in reverse, it's possible to create a single animation and drive it either
forward or backward. In the following case, the menu is initially
@ref NodeFlag::Hidden, slides from the top and is not reacting to events until
the animation stops. To make the animation reusable, it's marked with
@ref AnimationFlag::KeepOncePlayed so it doesn't get removed after first play.

@snippet Ui.cpp NodeAnimator-create-reversible

When @ref AnimationFlag::Reverse is added and the animation is played, the
offset, size and opacity expectedly animates in reverse. For flags both the
* *order* and the *operation* is reversed --- here it'll first add
@ref NodeFlag::NoEvents, slide in the opposite direction and finally removes
@relativeref{NodeFlag,NoEvents} again and makes the node hidden:

@snippet Ui.cpp NodeAnimator-create-reversible-reverse

Furthermore, such an animation can be reversed during the time it's played
using the @ref addFlags(AnimationHandle, AnimationFlags, Nanoseconds) "addFlags()"
/ @ref clearFlags(AnimationHandle, AnimationFlags, Nanoseconds) "clearFlags()"
overloads that additionally take current time, which will make the animation
continue back from where it ended up being at given time. The following code
will first attempt to make the animation play backward assuming it's still
playing, and if it's not it plays it from the start:

@snippet Ui.cpp NodeAnimator-create-reversible-reverse-at-time

@section Ui-NodeAnimator-lifetime Animation lifetime and node attachment

As with all other animations, they're implicitly removed once they're played.
Pass @ref AnimationFlag::KeepOncePlayed to @ref create() or @ref addFlags() to
disable this behavior.

As the animations are associated with nodes they animate, when the node the
animation is attached to is removed, the animation gets removed as well. If you
want to preserve the animation when the node is removed, call
@ref attach(AnimationHandle, NodeHandle) with @ref NodeHandle::Null to detach
it from the node before removing. After that, or if you have called @ref create()
with @ref NodeHandle::Null in the first place, the animation will still play
but affect nothing.

Note that if @ref NodeAnimation::setRemoveNodeAfter() is enabled, the animation
gets implicitly removed along with the node when it stops, even with
@ref AnimationFlag::KeepOncePlayed set.

@section Ui-NodeAnimator-conflicts Multiple animations affecting a single node

Currently, if multiple animations simultaenously affect the same property of
the same node, the behavior is unspecified and will likely result in the
animations overwriting each other's output. There's no guarantee about the
order in which the animations are applied to a particular node, nor there's any
merging done.

On the other hand, having one @ref NodeAnimator animation affect for example
just an offset and another just a size or opacity will work correctly if both
animations are coming from @ref NodeAnimator. One practical use case is having
a different easing function for each. There's however no way to guarantee this
when the node animations are combined with @ref GenericNodeAnimator or custom
animators.

@section Ui-NodeAnimator-debug-integration Debug layer integration

When using @ref Ui-DebugLayer-node-inspect "DebugLayer node inspect" and
@ref DebugLayerSource::NodeAnimationDetails is enabled, passing this animator to
@ref DebugLayer::setAnimatorName(const T&, const Containers::StringView&) "DebugLayer::setAnimatorName()"
will make it list properties of a particular animation, with `?` denoting
offset, size or opacity taken from the node at the time the animation is
played. For example:

@include ui-debuglayer-nodeanimator.ansi

If given animation has @ref AnimationFlag::Reverse set, the output will be
appropriately different for clarity. The above would thus look like this when
the animation is reversed:

@include ui-debuglayer-nodeanimator-reverse.ansi
*/
class MAGNUM_UI_EXPORT NodeAnimator: public AbstractNodeAnimator {
    public:
        class DebugIntegration;

        /**
         * @brief Constructor
         * @param handle    Handle returned by
         *      @ref AbstractUserInterface::createAnimator()
         */
        explicit NodeAnimator(AnimatorHandle handle);

        /** @brief Copying is not allowed */
        NodeAnimator(const NodeAnimator&) = delete;

        /** @copydoc AbstractAnimator::AbstractAnimator(AbstractAnimator&&) */
        NodeAnimator(NodeAnimator&&) noexcept;

        ~NodeAnimator();

        /** @brief Copying is not allowed */
        NodeAnimator& operator=(const NodeAnimator&) = delete;

        /** @brief Move assignment */
        NodeAnimator& operator=(NodeAnimator&&) noexcept;

        /**
         * @brief Create an animation
         * @param animation     Offset, size, opacity and flag animation
         *      properties
         * @param easing        Easing function between @cpp 0.0f @ce and
         *      @cpp 1.0f @ce, used for offset, size and opacity. Pick one from
         *      @ref Animation::BasicEasing "Animation::Easing" or supply a
         *      custom one.
         * @param start         Time at which the animation starts. Use
         *      @ref Nanoseconds::max() for reserving an animation that
         *      doesn't get played until @ref play() is called on it.
         * @param duration      Duration of a single play of the animation
         * @param node          Node the animation is attached to. Use
         *      @ref NodeHandle::Null to create an animation that isn't
         *      attached to any node.
         * @param repeatCount   Repeat count. Use @cpp 0 @ce for an
         *      indefinitely repeating animation.
         * @param flags         Flags
         *
         * Expects that @p easing is not @cpp nullptr @ce if @p animation
         * affects node offset, size or opacity. Delegates to
         * @ref AbstractAnimator::create(Nanoseconds, Nanoseconds, NodeHandle, UnsignedInt, AnimationFlags), see its documentation for more
         * information.
         */
        AnimationHandle create(const NodeAnimation& animation, Float(*easing)(Float), Nanoseconds start, Nanoseconds duration, NodeHandle node, UnsignedInt repeatCount = 1, AnimationFlags flags = {});

        /**
         * @brief Create an animation
         *
         * Same as calling @ref create() "create(const NodeAnimation&, Float(*)(Float), Nanoseconds, Nanoseconds, NodeHandle, UnsignedInt, AnimationFlags)"
         * with @p repeatCount set to @cpp 1 @ce.
         * @todoc fix the overload references once Doxygen can link to
         *      functions taking function pointers
         */
        AnimationHandle create(const NodeAnimation& animation, Float(*easing)(Float), Nanoseconds start, Nanoseconds duration, NodeHandle node, AnimationFlags flags);

        /**
         * @brief Remove an animation
         *
         * Delegates to @ref AbstractAnimator::remove(AnimationHandle).
         *
         * @m_class{m-note m-warning}
         *
         * @par
         *      Note that removing a currently playing animation with this
         *      function will leave the node in whatever state it was during
         *      the animation, such as with @ref NodeFlag::NoEvents temporarily
         *      set. Consider using @ref stop() instead, which first puts the
         *      node into the final animated state. Then, assuming
         *      @ref AnimationFlag::KeepOncePlayed isn't set, it will cause
         *      the animation to be removed automatically.
         */
        void remove(AnimationHandle handle) {
            AbstractNodeAnimator::remove(handle);
        }

        /**
         * @brief Remove an animation assuming it belongs to this animator
         *
         * Like @ref remove(AnimationHandle) but delegates to
         * @ref AbstractAnimator::remove(AnimatorDataHandle) instead.
         */
        void remove(AnimatorDataHandle handle) {
            AbstractNodeAnimator::remove(handle);
        }

        /**
         * @brief Animation source and target offsets
         *
         * Expects that @p handle is valid. Components that are NaN are taken
         * from the @ref AbstractUserInterface at the time the animation
         * starts. If a component is a NaN in both the source and the target
         * offset, it's not animated at all.
         * @see @ref sizes(), @ref opacities(),
         *      @ref Math::isNan(const Vector<size, T>&),
         *      @ref isHandleValid(AnimationHandle) const
         */
        Containers::Pair<Vector2, Vector2> offsets(AnimationHandle handle) const;

        /**
         * @brief Animation source and target offsets assuming it belongs to this animator
         *
         * Like @ref offsets(AnimationHandle) const but without checking that
         * @p handle indeed belongs to this animator. See its documentation for
         * more information.
         * @see @ref isHandleValid(AnimatorDataHandle) const,
         *      @ref animationHandleData()
         */
        Containers::Pair<Vector2, Vector2> offsets(AnimatorDataHandle handle) const;

        /**
         * @brief Animation source and target sizes
         *
         * Expects that @p handle is valid. Components that are NaN are taken
         * from the @ref AbstractUserInterface at the time the animation
         * starts. If a component is a NaN in both the source and the target
         * size, it's not animated at all.
         * @see @ref offsets(), @ref opacities(),
         *      @ref Math::isNan(const Vector<size, T>&),
         *      @ref isHandleValid(AnimationHandle) const
         */
        Containers::Pair<Vector2, Vector2> sizes(AnimationHandle handle) const;

        /**
         * @brief Animation source and target sizes assuming it belongs to this animator
         *
         * Like @ref sizes(AnimationHandle) const but without checking that
         * @p handle indeed belongs to this animator. See its documentation for
         * more information.
         * @see @ref isHandleValid(AnimatorDataHandle) const,
         *      @ref animationHandleData()
         */
        Containers::Pair<Vector2, Vector2> sizes(AnimatorDataHandle handle) const;

        /**
         * @brief Animation source and target opacities
         *
         * Expects that @p handle is valid. Values that are NaN are taken from
         * the @ref AbstractUserInterface at the time the animation starts. If
         * both the source and the target opacity is a NaN, it's not animated
         * at all.
         * @see @ref offsets(), @ref sizes(), @ref Math::isNan(),
         *      @ref isHandleValid(AnimationHandle) const
         */
        Containers::Pair<Float, Float> opacities(AnimationHandle handle) const;

        /**
         * @brief Animation source and target opacities assuming it belongs to this animator
         *
         * Like @ref opacities(AnimationHandle) const but without checking that
         * @p handle indeed belongs to this animator. See its documentation for
         * more information.
         * @see @ref isHandleValid(AnimatorDataHandle) const,
         *      @ref animationHandleData()
         */
        Containers::Pair<Float, Float> opacities(AnimatorDataHandle handle) const;

        /**
         * @brief Node flags to add at animation begin and end
         *
         * Expects that @p handle is valid.
         * @see @ref flagsClear(), @ref isHandleValid(AnimationHandle) const
         */
        /* Not named addFlags() as that'd make it look like a setter */
        Containers::Pair<NodeFlags, NodeFlags> flagsAdd(AnimationHandle handle) const;

        /**
         * @brief Node flags to add at animation begin and end assuming it belongs to this animator
         *
         * Like @ref flagsAdd(AnimationHandle) const but without checking that
         * @p handle indeed belongs to this animator. See its documentation for
         * more information.
         * @see @ref isHandleValid(AnimatorDataHandle) const,
         *      @ref animationHandleData()
         */
        Containers::Pair<NodeFlags, NodeFlags> flagsAdd(AnimatorDataHandle handle) const;

        /**
         * @brief Node flags to clear at animation begin and end
         *
         * Expects that @p handle is valid.
         * @see @ref flagsAdd(), @ref isHandleValid(AnimationHandle) const
         */
        /* Not named clearFlags() as that'd make it look like a setter */
        Containers::Pair<NodeFlags, NodeFlags> flagsClear(AnimationHandle handle) const;

        /**
         * @brief Node flags to clear at animation begin and end assuming it belongs to this animator
         *
         * Like @ref flagsClear(AnimationHandle) const but without checking
         * that @p handle indeed belongs to this animator. See its
         * documentation for more information.
         * @see @ref isHandleValid(AnimatorDataHandle) const,
         *      @ref animationHandleData()
         */
        Containers::Pair<NodeFlags, NodeFlags> flagsClear(AnimatorDataHandle handle) const;

        /**
         * @brief Whether the node the animation is assigned to is removed when the animation stops
         *
         * Expects that @p handle is valid.
         * @see @ref isHandleValid(AnimationHandle) const
         */
        bool hasRemoveNodeAfter(AnimationHandle handle) const;

        /**
         * @brief Whether the node the animation is assigned to is removed when the animation stops assuming it belongs to this animator
         *
         * Like @ref hasRemoveNodeAfter(AnimationHandle) const but without
         * checking that @p handle indeed belongs to this animator. See its
         * documentation for more information.
         * @see @ref isHandleValid(AnimatorDataHandle) const,
         *      @ref animationHandleData()
         */
        bool hasRemoveNodeAfter(AnimatorDataHandle handle) const;

        /**
         * @brief Animation easing function
         *
         * Expects that @p handle is valid. The returned pointer is only
         * guaranteed to not be @cpp nullptr @ce if given animation affects
         * node offset, size or opacity.
         * @see @ref isHandleValid(AnimationHandle) const
         */
        auto easing(AnimationHandle handle) const -> Float(*)(Float);

        /**
         * @brief Animation easing function assuming it belongs to this animator
         *
         * Like @ref easing(AnimationHandle) const but without checking that
         * @p handle indeed belongs to this animator. See its documentation for
         * more information.
         * @see @ref isHandleValid(AnimatorDataHandle) const,
         *      @ref animationHandleData()
         */
        auto easing(AnimatorDataHandle handle) const -> Float(*)(Float);

    private:
        MAGNUM_UI_LOCAL Containers::Pair<Vector2, Vector2> offsetsInternal(UnsignedInt id) const;
        MAGNUM_UI_LOCAL Containers::Pair<Vector2, Vector2> sizesInternal(UnsignedInt id) const;
        MAGNUM_UI_LOCAL Containers::Pair<Float, Float> opacitiesInternal(UnsignedInt id) const;
        MAGNUM_UI_LOCAL AnimatorFeatures doFeatures() const override;
        MAGNUM_UI_LOCAL NodeAnimatorUpdates doAdvance(Containers::BitArrayView active, Containers::BitArrayView started, Containers::BitArrayView stopped, const Containers::StridedArrayView1D<const Float>& factors, const Containers::StridedArrayView1D<Vector2>& nodeOffsets, const Containers::StridedArrayView1D<Vector2>& nodeSizes, const Containers::StridedArrayView1D<Float>& nodeOpacities, const Containers::StridedArrayView1D<NodeFlags>& nodeFlags, Containers::MutableBitArrayView nodesRemove) override;

        struct State;
        Containers::Pointer<State> _state;
};

/**
@brief Node animation properties
@m_since_latest

Used to pass arguments to @ref NodeAnimator::create(). See the
@ref NodeAnimator class documentation for more information and usage examples.
*/
class MAGNUM_UI_EXPORT NodeAnimation {
    public:
        /**
         * @brief Constructor
         *
         * By default, nothing is animated. Call various offset, size, opacity
         * and flag setters to specify what to animate.
         */
        constexpr explicit NodeAnimation() = default;

        /**
         * @brief Source and target offsets
         *
         * Components that are NaN are taken from the @ref AbstractUserInterface
         * at the time the animation starts. If a component is a NaN in both
         * the source and the target offset, it's not animated at all.
         * @see @ref Math::isNan(const Vector<size, T>&),
         *      @ref AbstractUserInterface::nodeOffset()
         */
        Containers::Pair<Vector2, Vector2> offsets() const;

        /**
         * @brief Animate from given X offset
         *
         * If @ref toOffsetX() isn't called as well, the animation will move
         * the node from @p sourceOffset to the X offset the node has at the
         * time the animation starts. If neither @ref fromOffsetX() nor
         * @ref toOffsetX() is called, node X offset stays unchanged. Passing
         * @ref Constants::nan() as @p sourceOffset is equivalent to not
         * calling this function at all.
         *
         * The offset is always interpreted as relative to the node parent.
         * @see @ref fromOffsetY(), @ref fromOffset(), @ref fromSizeX(),
         *      @ref AbstractUserInterface::setNodeOffset()
         */
        CORRADE_CONSTEXPR14 NodeAnimation& fromOffsetX(Float sourceOffset) {
            _sourceOffset.x() = sourceOffset;
            return *this;
        }

        /**
         * @brief Animate from given Y offset
         *
         * If @ref toOffsetY() isn't called as well, the animation will move
         * the node from @p sourceOffset to the Y offset the node has at the
         * time the animation starts. If neither @ref fromOffsetY() nor
         * @ref toOffsetY() is called, node Y offset stays unchanged. Passing
         * @ref Constants::nan() as @p sourceOffset is equivalent to not
         * calling this function at all.
         *
         * The offset is always interpreted as relative to the node parent.
         * @see @ref fromOffsetX(), @ref fromOffset(), @ref fromSizeY(),
         *      @ref AbstractUserInterface::setNodeOffset()
         */
        CORRADE_CONSTEXPR14 NodeAnimation& fromOffsetY(Float sourceOffset) {
            _sourceOffset.y() = sourceOffset;
            return *this;
        }

        /**
         * @brief Animate from given offset
         *
         * Same as calling @ref fromOffsetX() and @ref fromOffsetY() with the
         * X and Y component of @p sourceOffset. See their documentation for
         * more information.
         */
        CORRADE_CONSTEXPR14 NodeAnimation& fromOffset(const Vector2& sourceOffset) {
            _sourceOffset = sourceOffset;
            return *this;
        }

        /**
         * @brief Animate to given X offset
         *
         * If @ref fromOffsetX() isn't called as well, the animation will move
         * the node from the X offset the node has at the time the animation
         * starts to @p targetOffset. If neither @ref fromOffsetX() nor
         * @ref toOffsetX() is called, node X offset stays unchanged. Passing
         * @ref Constants::nan() as @p targetOffset is equivalent to not
         * calling this function at all.
         *
         * The offset is always interpreted as relative to the node parent.
         * @see @ref toOffsetY(), @ref toOffset(), @ref toSizeX(),
         *      @ref AbstractUserInterface::setNodeOffset()
         */
        CORRADE_CONSTEXPR14 NodeAnimation& toOffsetX(Float targetOffset) {
            _targetOffset.x() = targetOffset;
            return *this;
        }

        /**
         * @brief Animate to given Y offset
         *
         * If @ref fromOffsetY() isn't called as well, the animation will move
         * the node from the Y offset the node has at the time the animation
         * starts to @p targetOffset. If neither @ref fromOffsetY() nor
         * @ref toOffsetY() is called, node Y offset stays unchanged. Passing
         * @ref Constants::nan() as @p targetOffset is equivalent to not
         * calling this function at all.
         *
         * The offset is always interpreted as relative to the node parent.
         * @see @ref toOffsetX(), @ref toOffset(), @ref toSizeY(),
         *      @ref AbstractUserInterface::setNodeOffset()
         */
        CORRADE_CONSTEXPR14 NodeAnimation& toOffsetY(Float targetOffset) {
            _targetOffset.y() = targetOffset;
            return *this;
        }

        /**
         * @brief Animate to given offset
         *
         * Same as calling @ref toOffsetX() and @ref toOffsetY() with the X and
         * Y component of @p targetOffset. See their documentation for more
         * information.
         */
        CORRADE_CONSTEXPR14 NodeAnimation& toOffset(const Vector2& targetOffset) {
            _targetOffset = targetOffset;
            return *this;
        }

        /**
         * @brief Source and target sizes
         *
         * Components that are NaN are taken from the @ref AbstractUserInterface
         * at the time the animation starts. If a component is a NaN in both
         * the source and the target offset, it's not animated at all.
         * @see @ref Math::isNan(const Vector<size, T>&),
         *      @ref AbstractUserInterface::nodeSize()
         */
        Containers::Pair<Vector2, Vector2> sizes() const;

        /**
         * @brief Animate from given width
         *
         * If @ref toSizeX() isn't called as well, the animation will resize
         * the node from @p sourceWidth to the width the node has at the time
         * the animation starts. If neither @ref fromSizeX() nor @ref toSizeX()
         * is called, node width stays unchanged. Passing @ref Constants::nan()
         * as @p sourceWidth is equivalent to not calling this function at all.
         * @see @ref fromSizeY(), @ref fromSize(), @ref fromOffsetX(),
         *      @ref AbstractUserInterface::setNodeSize()
         */
        CORRADE_CONSTEXPR14 NodeAnimation& fromSizeX(Float sourceWidth) {
            _sourceSize.x() = sourceWidth;
            return *this;
        }

        /**
         * @brief Animate from given height
         *
         * If @ref toSizeY() isn't called as well, the animation will resize
         * the node from @p sourceHeight to the height the node has at the time
         * the animation starts. If neither @ref fromSizeY() nor @ref toSizeY()
         * is called, node height stays unchanged. Passing
         * @ref Constants::nan() as @p sourceHeight is equivalent to not
         * calling this function at all.
         * @see @ref fromSizeX(), @ref fromSize(), @ref fromOffsetY(),
         *      @ref AbstractUserInterface::setNodeSize()
         */
        CORRADE_CONSTEXPR14 NodeAnimation& fromSizeY(Float sourceHeight) {
            _sourceSize.y() = sourceHeight;
            return *this;
        }

        /**
         * @brief Animate from given size
         *
         * Same as calling @ref fromSizeX() and @ref fromSizeY() with the X and
         * Y component of @p sourceSize. See their documentation for more
         * information.
         */
        CORRADE_CONSTEXPR14 NodeAnimation& fromSize(const Vector2& sourceSize) {
            _sourceSize = sourceSize;
            return *this;
        }

        /**
         * @brief Animate to given width
         *
         * If @ref fromSizeX() isn't called as well, the animation will resize
         * the node the width the node has at the time the animation starts to
         * @p targetWidth. If neither @ref fromSizeX() nor @ref toSizeX() is
         * called, node width stays unchanged. Passing @ref Constants::nan() as
         * @p targetWidth is equivalent to not calling this function at all.
         * @see @ref toSizeY(), @ref toSize(), @ref toOffsetX(),
         *      @ref AbstractUserInterface::setNodeSize()
         */
        CORRADE_CONSTEXPR14 NodeAnimation& toSizeX(Float targetWidth) {
            _targetSize.x() = targetWidth;
            return *this;
        }

        /**
         * @brief Animate to given height
         *
         * If @ref fromSizeY() isn't called as well, the animation will resize
         * the node the height the node has at the time the animation starts to
         * @p targetHeight. If neither @ref fromSizeY() nor @ref toSizeY() is
         * called, node height stays unchanged. Passing @ref Constants::nan()
         * as @p targetHeight is equivalent to not calling this function at
         * all.
         * @see @ref toSizeX(), @ref toSize(), @ref toOffsetY(),
         *      @ref AbstractUserInterface::setNodeSize()
         */
        CORRADE_CONSTEXPR14 NodeAnimation& toSizeY(Float targetHeight) {
            _targetSize.y() = targetHeight;
            return *this;
        }

        /**
         * @brief Animate to given size
         *
         * Same as calling @ref toSizeX() and @ref toSizeY() with the X and
         * Y component of @p targetSize. See their documentation for more
         * information.
         */
        CORRADE_CONSTEXPR14 NodeAnimation& toSize(const Vector2& targetSize) {
            _targetSize = targetSize;
            return *this;
        }

        /**
         * @brief Source and target opacities
         *
         * Values that are NaN are taken from the @ref AbstractUserInterface at
         * the time the animation starts. If both the source and the target
         * opacity is a NaN, it's not animated at all.
         * @see @ref Math::isNan(), @ref AbstractUserInterface::nodeOpacity()
         */
        Containers::Pair<Float, Float> opacities() const;

        /**
         * @brief Animate from given opacity
         *
         * If @ref toOpacity() isn't called as well, the animation will move
         * the node from @p sourceOpacity to the opacity the node has at the
         * time the animation starts. If neither @ref fromOpacity() nor
         * @ref toOpacity() is called, node opacity stays unchanged.
         * @see @ref AbstractUserInterface::setNodeOpacity()
         */
        CORRADE_CONSTEXPR14 NodeAnimation& fromOpacity(Float sourceOpacity) {
            _sourceOpacity = sourceOpacity;
            return *this;
        }

        /**
         * @brief Animate to given opacity
         *
         * If @ref fromOpacity() isn't called as well, the animation will move
         * the node from the opacity the node has at the time the animation
         * starts to @p targetOpacity. If neither @ref fromOpacity() nor
         * @ref toOpacity() is called, node opacity stays unchanged.
         * @see @ref AbstractUserInterface::setNodeOpacity()
         */
        CORRADE_CONSTEXPR14 NodeAnimation& toOpacity(Float targetOpacity) {
            _targetOpacity = targetOpacity;
            return *this;
        }

        /** @brief Node flags to add at animation begin and end */
        Containers::Pair<NodeFlags, NodeFlags> flagsAdd() const;

        /**
         * @brief Add node flags when the animation begins
         *
         * Adds @p flags to the set of flags the node has at the beginning of
         * the animation. If @ref clearFlagsBegin() is called as well, the
         * clear happens before the add. If none of @ref addFlagsBegin(),
         * @ref clearFlagsBegin(), @ref addFlagsEnd(), @ref clearFlagsEnd() is
         * called, node flags stay unchanged. If the animation has
         * @ref AnimationFlag::Reverse set, the flags are *cleared* at
         * animation stop instead of being added at start. Presence of
         * @ref AnimationFlag::ReverseEveryOther has no effect on this
         * behavior. If the animation has multiple repeats, the flags are
         * updated only for the very first / very last repeat.
         *
         * Note that, unlike with @ref AbstractUserInterface::addNodeFlags(),
         * calling this function multiple times *replaces* the set of flags to
         * add, doesn't merge into it.
         */
        CORRADE_CONSTEXPR14 NodeAnimation& addFlagsBegin(NodeFlags flags) {
            _flagsAddBegin = flags;
            return *this;
        }

        /**
         * @brief Add node flags when the animation ends
         *
         * Adds @p flags to the set of flags the node has at the end of the
         * animation. If @ref clearFlagsEnd() is called as well, the clear
         * happens before adding @p flags. If none of @ref addFlagsBegin(),
         * @ref clearFlagsBegin(), @ref addFlagsEnd(), @ref clearFlagsEnd() is
         * called, node flags stay unchanged. If the animation has
         * @ref AnimationFlag::Reverse set, the flags are *cleared* at
         * animation start instead of being added at stop. Presence of
         * @ref AnimationFlag::ReverseEveryOther has no effect on this
         * behavior. If the animation has multiple repeats, the flags are
         * updated only for the very last / very first repeat.
         *
         * Note that, unlike with @ref AbstractUserInterface::addNodeFlags(),
         * calling this function multiple times *replaces* the set of flags to
         * add, doesn't merge into it.
         */
        CORRADE_CONSTEXPR14 NodeAnimation& addFlagsEnd(NodeFlags flags) {
            _flagsAddEnd = flags;
            return *this;
        }

        /** @brief Node flags to clear at animation begin and end */
        Containers::Pair<NodeFlags, NodeFlags> flagsClear() const;

        /**
         * @brief Clear node flags when the animation begins
         *
         * Clears @p flags from the set of flags the node has at the beginning
         * of the animation. If @ref addFlagsBegin() is called as well, the
         * clear happens before the add. If none of @ref addFlagsBegin(),
         * @ref clearFlagsBegin(), @ref addFlagsEnd(), @ref clearFlagsEnd() is
         * called, node flags stay unchanged. If the animation has
         * @ref AnimationFlag::Reverse set, the flags are *added* at animation
         * stop instead of being cleared at start. Presence of
         * @ref AnimationFlag::ReverseEveryOther has no effect on this
         * behavior. If the animation has multiple repeats, the flags are
         * updated only for the very first / very last repeat.
         *
         * Note that, unlike with @ref AbstractUserInterface::clearNodeFlags(),
         * calling this function multiple times *replaces* the set of flags to
         * clear, doesn't merge into it.
         */
        CORRADE_CONSTEXPR14 NodeAnimation& clearFlagsBegin(NodeFlags flags) {
            _flagsClearBegin = flags;
            return *this;
        }

        /**
         * @brief Clear node flags when the animation ends
         *
         * Clears @p flags from the set of flags the node has at the end of the
         * animation. If @ref addFlagsEnd() is called as well, the clear
         * happens before the add. If none of @ref addFlagsBegin(),
         * @ref clearFlagsBegin(), @ref addFlagsEnd(), @ref clearFlagsEnd() is
         * called, node flags stay unchanged. If the animation has
         * @ref AnimationFlag::Reverse set, the flags are *added* at animation
         * start instead of being cleared at stop. Presence of
         * @ref AnimationFlag::ReverseEveryOther has no effect on this
         * behavior. If the animation has multiple repeats, the flags are
         * updated only for the very last / very first repeat.
         *
         * Note that, unlike with @ref AbstractUserInterface::clearNodeFlags(),
         * calling this function multiple times *replaces* the set of flags to
         * clear, doesn't merge into it.
         */
        CORRADE_CONSTEXPR14 NodeAnimation& clearFlagsEnd(NodeFlags flags) {
            _flagsClearEnd = flags;
            return *this;
        }

        /** @brief Whether the node is removed when the animation stops */
        constexpr bool hasRemoveNodeAfter() const {
            return _removeNodeAfter;
        }

        /**
         * @brief Remove a node when the animation stops
         *
         * If @p remove is @cpp true @ce and the animation attachment isn't
         * @ref NodeHandle::Null, the node gets automatically removed once the
         * animation stops. If @cpp false @ce or if this function isn't called
         * at all, the node doesn't get removed. @ref AnimationFlag::Reverse
         * and @ref AnimationFlag::ReverseEveryOther don't affect the behavior
         * in any way, the removal is done once the animation stops regardless
         * of these being present.
         *
         * Note that, if removal is enabled, since the animation is attached to
         * the node, it gets subsequently removed as well, regardless of
         * whether @ref AnimationFlag::KeepOncePlayed is specified in
         * @ref NodeAnimator::create().
         */
        CORRADE_CONSTEXPR14 NodeAnimation& setRemoveNodeAfter(bool remove = true) {
            _removeNodeAfter = remove;
            return *this;
        }

    private:
        friend NodeAnimator;

        Vector2 _sourceOffset{Constants::nan()},
            _targetOffset{Constants::nan()};
        Vector2 _sourceSize{Constants::nan()},
            _targetSize{Constants::nan()};
        Float _sourceOpacity{Constants::nan()},
            _targetOpacity{Constants::nan()};

        NodeFlags _flagsAddBegin,
            _flagsAddEnd,
            _flagsClearBegin,
            _flagsClearEnd;

        bool _removeNodeAfter = false;
};

/**
@brief Debug layer integration

Integrates the animator with @ref DebugLayer. See
@ref Ui-NodeAnimator-debug-integration "NodeAnimator debug layer integration"
for more information and example usage.
*/
class MAGNUM_UI_EXPORT NodeAnimator::DebugIntegration {
    public:
        #ifndef DOXYGEN_GENERATING_OUTPUT
        /* Used internally by DebugLayer, no point in documenting it here */
        void print(Debug& debug, const NodeAnimator& animator, const Containers::StringView& animatorName, AnimatorDataHandle animation);
        #endif
};

}}

#endif
