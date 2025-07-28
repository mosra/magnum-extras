#ifndef Magnum_Ui_GenericAnimator_h
#define Magnum_Ui_GenericAnimator_h
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
 * @brief Class @ref Magnum::Ui::GenericAnimator, @ref Magnum::Ui::GenericNodeAnimator, @ref Magnum::Ui::GenericDataAnimator
 * @m_since_latest
 */

#include "Magnum/Ui/AbstractAnimator.h"

namespace Magnum { namespace Ui {

/**
@brief Generic animator
@m_since_latest

Each animation is a function that gets called with an animation factor in the
@f$ [0, 1] @f$ range. The function can then call arbitrary setters on the UI
instance, on layers or elsewhere. If an animation is associated with a
particular node or layer data, you may want to use @ref GenericNodeAnimator or
@ref GenericDataAnimator instead.

@section Ui-GenericAnimator-setup Setting up an animator instance

The animator doesn't have any shared state or configuration, so it's just about
constructing it from a fresh @ref AbstractUserInterface::createAnimator()
handle and passing it to @relativeref{AbstractUserInterface,setGenericAnimatorInstance()}.
After that, assuming @ref AbstractUserInterface::advanceAnimations() is called
in an appropriate place, it's ready to use.

@snippet Ui.cpp GenericAnimator-setup

Unlike builtin layers or layouters, the default @ref UserInterface
implementation doesn't implicitly provide a @ref GenericAnimator instance.

@section Ui-GenericAnimator-create Creating animations

An animation is created by calling @ref create() with an appropriate function
taking the interpolation factor as a single argument, an easing function from
@ref Animation::BasicEasing "Animation::Easing" or a custom one, time at which
it's meant to start and its duration.

@snippet Ui.cpp GenericAnimator-create

If the function performs easing on its own, pass @ref Animation::Easing::linear
as the easing function to have the animation factor passed unchanged.

The animation function is free to do anything except for touching state related
to the animations themselves, such as playing, stopping, creating or removing
them. This isn't checked or enforced in any way, but the behavior of doing so
is undefined.

@section Ui-GenericAnimator-lifetime Animation lifetime

As with all other animations, they're implicitly removed once they're played.
Pass @ref AnimationFlag::KeepOncePlayed to @ref create() or @ref addFlags() to
disable this behavior.

The animator has no way of knowing what resources the animation function
accesses and thus the user is responsible of making sure the animation doesn't
attempt to access no longer valid handles and such. For this reason, if the
animation is associated with a particular node or layer data, it's recommended
to use @ref GenericNodeAnimator or @ref GenericDataAnimator instead, which will
ensure that as soon as the node or data the animation is attached to is removed
the animation gets removed as well.
*/
class MAGNUM_UI_EXPORT GenericAnimator: public AbstractGenericAnimator {
    public:
        /**
         * @brief Constructor
         * @param handle    Handle returned by
         *      @ref AbstractUserInterface::createAnimator()
         */
        explicit GenericAnimator(AnimatorHandle handle);

        /** @brief Copying is not allowed */
        GenericAnimator(const GenericAnimator&) = delete;

        /** @copydoc AbstractAnimator::AbstractAnimator(AbstractAnimator&&) */
        GenericAnimator(GenericAnimator&&) noexcept;

        ~GenericAnimator();

        /** @brief Copying is not allowed */
        GenericAnimator& operator=(const GenericAnimator&) = delete;

        /** @brief Move assignment */
        GenericAnimator& operator=(GenericAnimator&&) noexcept;

        /**
         * @brief Count of allocated animation functions
         *
         * Always at most @ref usedCount(). Counts all animation functions that
         * capture non-trivially-destructible state or state that's too large
         * to be stored in-place. The operation is done with a
         * @f$ \mathcal{O}(n) @f$ complexity where @f$ n @f$ is
         * @ref capacity().
         * @todoc fix the isAllocated link once Doxygen stops being shit -- it
         *      works only from Containers themselves
         * @see @ref Corrade::Containers::Function "Containers::Function<R(Args...)>::isAllocated()"
         */
        UnsignedInt usedAllocatedAnimationCount() const;

        /**
         * @brief Create an animation
         * @param animation     Animation function
         * @param easing        Easing function between @cpp 0.0f @ce and
         *      @cpp 1.0f @ce, applied to the @p factor passed to @p animation.
         *      Pick one from @ref Animation::BasicEasing "Animation::Easing"
         *      or supply a custom one.
         * @param start         Time at which the animation starts. Use
         *      @ref Nanoseconds::max() for creating a stopped animation.
         * @param duration      Duration of a single play of the animation
         * @param repeatCount   Repeat count. Use @cpp 0 @ce for an
         *      indefinitely repeating animation.
         * @param flags         Flags
         *
         * Expects that both @p animation and @p easing are not @cpp nullptr @ce.
         * Delegates to @ref AbstractAnimator::create(Nanoseconds, Nanoseconds, UnsignedInt, AnimationFlags),
         * see its documentation for more information.
         *
         * Assuming the @p easing function correctly maps @cpp 0.0f @ce and
         * @cpp 1.0f @ce to themselves, the animation function is guaranteed to
         * be called with @p factor being exactly @cpp 1.0f @ce once the
         * animation is stopped. Other than that, it may be an arbitrary value
         * from the @f$ [0, 1] @f$ range.
         */
        AnimationHandle create(Containers::Function<void(Float factor)>&& animation, Float(*easing)(Float), Nanoseconds start, Nanoseconds duration, UnsignedInt repeatCount = 1, AnimationFlags flags = {});

        /**
         * @brief Remove an animation
         *
         * Expects that @p handle is valid. Delegates to
         * @ref AbstractAnimator::remove(AnimationHandle), see its
         * documentation for more information.
         */
        void remove(AnimationHandle handle);

        /**
         * @brief Remove an animation assuming it belongs to this animator
         *
         * Compared to @ref remove(AnimationHandle) delegates to
         * @ref AbstractAnimator::remove(AnimatorDataHandle) instead.
         */
        void remove(AnimatorDataHandle handle);

        /**
         * @brief Animation easing function
         *
         * Expects that @p handle is valid. The returned pointer is never
         * @cpp nullptr @ce.
         */
        auto easing(AnimationHandle handle) const -> Float(*)(Float);

        /**
         * @brief Animation easing function assuming it belongs to this animator
         *
         * Like @ref easing(AnimationHandle) const but without checking that
         * @p handle indeed belongs to this animator. See its documentation for
         * more information.
         * @see @ref animationHandleData()
         */
        auto easing(AnimatorDataHandle handle) const -> Float(*)(Float);

    private:
        MAGNUM_UI_LOCAL AnimationHandle createInternal(Nanoseconds start, Nanoseconds duration, UnsignedInt repeatCount, AnimationFlags flags);
        MAGNUM_UI_LOCAL void removeInternal(UnsignedInt id);

        MAGNUM_UI_LOCAL AnimatorFeatures doFeatures() const override;
        MAGNUM_UI_LOCAL void doClean(Containers::BitArrayView animationIdsToRemove) override;
        MAGNUM_UI_LOCAL void doAdvance(Containers::BitArrayView active, Containers::BitArrayView started, Containers::BitArrayView stopped, const Containers::StridedArrayView1D<const Float>& factors) override;

        struct State;
        Containers::Pointer<State> _state;
};

/**
@brief Generic animator with animations attached to nodes
@m_since_latest

Each animation is a function that gets called with an associated node handle
and an animation factor in the @f$ [0, 1] @f$ range. The function can then call
arbitrary node-related setters on the UI instance or elsewhere. Use
@ref GenericDataAnimator for animations associated with a particular layer
data, @ref GenericAnimator is then for animations not tied to either.

@section Ui-GenericNodeAnimator-setup Setting up an animator instance

The animator doesn't have any shared state or configuration, so it's just about
constructing it from a fresh @ref AbstractUserInterface::createAnimator()
handle and passing it to @relativeref{AbstractUserInterface,setGenericAnimatorInstance()}.
After that, assuming @ref AbstractUserInterface::advanceAnimations() is called
in an appropriate place, it's ready to use.

@snippet Ui.cpp GenericNodeAnimator-setup

Unlike builtin layers or layouters, the default @ref UserInterface
implementation doesn't implicitly provide a @ref GenericNodeAnimator instance.

@section Ui-GenericNodeAnimator-create Creating animations

An animation is created by calling @ref create() with an appropriate function
taking the node handle and interpolation factor as arguments, an easing
function from @ref Animation::BasicEasing "Animation::Easing" or a custom one,
time at which it's meant to start, its duration and the @ref NodeHandle it's
attached to. For example, animating a dropdown opening by gradually enlarging
its height and turning it from transparent to opaque:

@snippet Ui.cpp GenericNodeAnimator-create

If the function performs easing on its own, pass @ref Animation::Easing::linear
as the easing function to have the animation factor passed unchanged.

The animation function is free to do anything except for touching state related
to the animations or associated nodes, such as playing or stopping the
animations, or creating, removing animations or nodes. This isn't checked or
enforced in any way, but the behavior of doing so is undefined.

@section Ui-GenericNodeAnimator-lifetime Animation lifetime and node attachment

As with all other animations, they're implicitly removed once they're played.
Pass @ref AnimationFlag::KeepOncePlayed to @ref create() or @ref addFlags() to
disable this behavior.

As the animations are associated with nodes they animate, when the node the
animation is attached to is removed, the animation gets removed as well. If you
want to preserve the animation when the node is removed, call
@ref attach(AnimationHandle, NodeHandle) with @ref NodeHandle::Null to detach
it from the node before removing. After that, or if you call @ref create() with
@ref NodeHandle::Null in the first place, the animation will still play, but
the animation function will get a null handle.
*/
class MAGNUM_UI_EXPORT GenericNodeAnimator: public AbstractGenericAnimator {
    public:
        /**
         * @brief Constructor
         * @param handle    Handle returned by
         *      @ref AbstractUserInterface::createAnimator()
         */
        explicit GenericNodeAnimator(AnimatorHandle handle);

        /** @brief Copying is not allowed */
        GenericNodeAnimator(const GenericNodeAnimator&) = delete;

        /** @copydoc AbstractAnimator::AbstractAnimator(AbstractAnimator&&) */
        GenericNodeAnimator(GenericNodeAnimator&&) noexcept;

        ~GenericNodeAnimator();

        /** @brief Copying is not allowed */
        GenericNodeAnimator& operator=(const GenericNodeAnimator&) = delete;

        /** @brief Move assignment */
        GenericNodeAnimator& operator=(GenericNodeAnimator&&) noexcept;

        /**
         * @brief Count of allocated animation functions
         *
         * Always at most @ref usedCount(). Counts all animation functions that
         * capture non-trivially-destructible state or state that's too large
         * to be stored in-place. The operation is done with a
         * @f$ \mathcal{O}(n) @f$ complexity where @f$ n @f$ is
         * @ref capacity().
         * @todoc fix the isAllocated link once Doxygen stops being shit -- it
         *      works only from Containers themselves
         * @see @ref Corrade::Containers::Function "Containers::Function<R(Args...)>::isAllocated()"
         */
        UnsignedInt usedAllocatedAnimationCount() const;

        /**
         * @brief Create an animation
         * @param animation     Animation function
         * @param easing        Easing function between @cpp 0.0f @ce and
         *      @cpp 1.0f @ce, applied to the @p factor passed to @p animation.
         *      Pick one from @ref Animation::BasicEasing "Animation::Easing"
         *      or supply a custom one.
         * @param start         Time at which the animation starts. Use
         *      @ref Nanoseconds::max() for creating a stopped animation.
         * @param duration      Duration of a single play of the animation
         * @param node          Node the animation is attached to. Use
         *      @ref NodeHandle::Null to create an animation that isn't
         *      attached to any node.
         * @param repeatCount   Repeat count. Use @cpp 0 @ce for an
         *      indefinitely repeating animation.
         * @param flags         Flags
         *
         * Expects that both @p animation and @p easing are not @cpp nullptr @ce.
         * Delegates to @ref AbstractAnimator::create(Nanoseconds, Nanoseconds, NodeHandle, UnsignedInt, AnimationFlags),
         * see its documentation for more information.
         *
         * Assuming the @p easing function correctly maps @cpp 0.0f @ce and
         * @cpp 1.0f @ce to themselves, the animation function is guaranteed to
         * be called with @p factor being exactly @cpp 1.0f @ce once the
         * animation is stopped. Other than that, it may be an arbitrary value
         * from the @f$ [0, 1] @f$ range.
         */
        AnimationHandle create(Containers::Function<void(NodeHandle node, Float factor)>&& animation, Float(*easing)(Float), Nanoseconds start, Nanoseconds duration, NodeHandle node, UnsignedInt repeatCount = 1, AnimationFlags flags = {});

        /**
         * @brief Remove an animation
         *
         * Expects that @p handle is valid. Delegates to
         * @ref AbstractAnimator::remove(AnimationHandle), see its
         * documentation for more information.
         */
        void remove(AnimationHandle handle);

        /**
         * @brief Remove an animation assuming it belongs to this animator
         *
         * Compared to @ref remove(AnimationHandle) delegates to
         * @ref AbstractAnimator::remove(AnimatorDataHandle) instead.
         */
        void remove(AnimatorDataHandle handle);

        /**
         * @brief Animation easing function
         *
         * Expects that @p handle is valid. The returned pointer is never
         * @cpp nullptr @ce.
         */
        auto easing(AnimationHandle handle) const -> Float(*)(Float);

        /**
         * @brief Animation easing function assuming it belongs to this animator
         *
         * Like @ref easing(AnimationHandle) const but without checking that
         * @p handle indeed belongs to this animator. See its documentation for
         * more information.
         * @see @ref animationHandleData()
         */
        auto easing(AnimatorDataHandle handle) const -> Float(*)(Float);

    private:
        MAGNUM_UI_LOCAL AnimationHandle createInternal(Nanoseconds start, Nanoseconds duration, NodeHandle node, UnsignedInt repeatCount, AnimationFlags flags);
        MAGNUM_UI_LOCAL void removeInternal(UnsignedInt id);

        MAGNUM_UI_LOCAL AnimatorFeatures doFeatures() const override;
        MAGNUM_UI_LOCAL void doClean(Containers::BitArrayView animationIdsToRemove) override;
        MAGNUM_UI_LOCAL void doAdvance(Containers::BitArrayView active, Containers::BitArrayView started, Containers::BitArrayView stopped, const Containers::StridedArrayView1D<const Float>& factors) override;

        struct State;
        Containers::Pointer<State> _state;
};

/**
@brief Generic animator with animations attached to layer data
@m_since_latest

Each animation is a function that gets called with an associated node handle
and an animation factor in the @f$ [0, 1] @f$ range. The function can then call
arbitrary data-related setters on the UI instance, on layers or elsewhere. Use
@ref GenericNodeAnimator for animations associated with just nodes,
@ref GenericAnimator is then for animations not tied to either.

@section Ui-GenericDataAnimator-setup Setting up an animator instance

The animator doesn't have any shared state or configuration, so it's just about
constructing it from a fresh @ref AbstractUserInterface::createAnimator()
handle and passing it to @relativeref{AbstractUserInterface,setGenericAnimatorInstance()}. After that,
use @ref setLayer() to register the animator with a concrete layer instance.
Then, assuming @ref AbstractUserInterface::advanceAnimations() is called in an
appropriate place, the animator is ready to use.

@snippet Ui.cpp GenericDataAnimator-setup

Unlike builtin layers or layouters, the default @ref UserInterface
implementation doesn't implicitly provide a @ref GenericDataAnimator instance.

@section Ui-GenericDataAnimator-create Creating animations

An animation is created by calling @ref create() with an appropriate function
taking the data handle and interpolation factor as arguments, an easing
function from @ref Animation::BasicEasing "Animation::Easing" or a custom one,
time at which it's meant to start, its duration and the @ref DataHandle it's
attached to. For example, animating a progress bar change, where the value is
visualized using custom @ref BaseLayer padding on the left side:

@snippet Ui.cpp GenericDataAnimator-create

If the function performs easing on its own, pass @ref Animation::Easing::linear
as the easing function to have the animation factor passed unchanged.

The animation function is free to do anything except for touching state related
to the animations or associated data or nodes, such as playing or stopping the
animations, or creating, removing animations, data or nodes. This isn't checked
or enforced in any way, but the behavior of doing so is undefined.

@section Ui-GenericDataAnimator-lifetime Animation lifetime and data attachment

As with all other animations, they're implicitly removed once they're played.
Pass @ref AnimationFlag::KeepOncePlayed to @ref create() or @ref addFlags() to
disable this behavior.

As the animations are associated with data they animate, when the data the
animation is attached to, or the node the data is attached to, is removed, the
animation gets removed as well. If you want to preserve the animation when the
data is removed, call @ref attach(AnimationHandle, DataHandle) with
@ref DataHandle::Null to detach it from the data before removing. After that,
or if you call @ref create() with @ref DataHandle::Null in the first place, the
animation will still play, but the animation function will get a null handle.
*/
class MAGNUM_UI_EXPORT GenericDataAnimator: public AbstractGenericAnimator {
    public:
        /**
         * @brief Constructor
         * @param handle    Handle returned by
         *      @ref AbstractUserInterface::createAnimator()
         */
        explicit GenericDataAnimator(AnimatorHandle handle);

        /** @brief Copying is not allowed */
        GenericDataAnimator(const GenericDataAnimator&) = delete;

        /** @copydoc AbstractAnimator::AbstractAnimator(AbstractAnimator&&) */
        GenericDataAnimator(GenericDataAnimator&&) noexcept;

        ~GenericDataAnimator();

        /** @brief Copying is not allowed */
        GenericDataAnimator& operator=(const GenericDataAnimator&) = delete;

        /** @brief Move assignment */
        GenericDataAnimator& operator=(GenericDataAnimator&&) noexcept;

        /**
         * @brief Set a layer associated with this animator
         *
         * Expects that this function hasn't been called yet. The associated
         * layer handle is subsequently available in @ref layer() const.
         */
        void setLayer(const AbstractLayer& layer) {
            AbstractGenericAnimator::setLayer(layer);
        }

        /**
         * @brief Count of allocated animation functions
         *
         * Always at most @ref usedCount(). Counts all animation functions that
         * capture non-trivially-destructible state or state that's too large
         * to be stored in-place. The operation is done with a
         * @f$ \mathcal{O}(n) @f$ complexity where @f$ n @f$ is
         * @ref capacity().
         * @todoc fix the isAllocated link once Doxygen stops being shit -- it
         *      works only from Containers themselves
         * @see @ref Corrade::Containers::Function "Containers::Function<R(Args...)>::isAllocated()"
         */
        UnsignedInt usedAllocatedAnimationCount() const;

        /**
         * @brief Create an animation
         * @param animation     Animation function
         * @param easing        Easing function between @cpp 0.0f @ce and
         *      @cpp 1.0f @ce, applied to the @p factor passed to @p animation.
         *      Pick one from @ref Animation::BasicEasing "Animation::Easing"
         *      or supply a custom one.
         * @param start         Time at which the animation starts. Use
         *      @ref Nanoseconds::max() for creating a stopped animation.
         * @param duration      Duration of a single play of the animation
         * @param data          Data the animation is attached to. Use
         *      @ref DataHandle::Null to create an animation that isn't
         *      attached to any data.
         * @param repeatCount   Repeat count. Use @cpp 0 @ce for an
         *      indefinitely repeating animation.
         * @param flags         Flags
         *
         * Expects that @ref setLayer() has been already called and that both
         * @p animation and @p easing are not @cpp nullptr @ce. Delegates to
         * @ref AbstractAnimator::create(Nanoseconds, Nanoseconds, DataHandle, UnsignedInt, AnimationFlags),
         * see its documentation for more information.
         *
         * Unless @p data is @ref DataHandle::Null or the animation is
         * subsequently detached from the data, the layer portion of the
         * @ref DataHandle passed to @p animator is matching the layer handle
         * passed to @ref setLayer(). Assuming the @p easing function correctly
         * maps @cpp 0.0f @ce and @cpp 1.0f @ce to themselves, the animation
         * function is guaranteed to be called with @p factor being exactly
         * @cpp 1.0f @ce once the animation is stopped. Other than that, it may
         * be an arbitrary value from the @f$ [0, 1] @f$ range.
         */
        AnimationHandle create(Containers::Function<void(DataHandle data, Float factor)>&& animation, Float(*easing)(Float), Nanoseconds start, Nanoseconds duration, DataHandle data, UnsignedInt repeatCount = 1, AnimationFlags flags = {});

        /**
         * @brief Create an animation assuming the data it's attached to belongs to the layer the animator is registered with
         *
         * Compared to @ref create(Containers::Function<void(DataHandle, Float)>&&, Float(*)(Float), Nanoseconds, Nanoseconds, DataHandle, UnsignedInt, AnimationFlags)
         * delegates to @ref AbstractAnimator::create(Nanoseconds, Nanoseconds, LayerDataHandle, UnsignedInt, AnimationFlags)
         * instead.
         *
         * Unless @p data is @ref LayerDataHandle::Null or the animation is
         * subsequently detached from the data, the layer portion of the
         * @ref DataHandle passed to @p animator is matching the layer handle
         * passed to @ref setLayer().
         */
        AnimationHandle create(Containers::Function<void(DataHandle data, Float factor)>&& animator, Float(*easing)(Float), Nanoseconds start, Nanoseconds duration, LayerDataHandle data, UnsignedInt repeatCount = 1, AnimationFlags flags = {});

        /**
         * @brief Remove an animation
         *
         * Expects that @p handle is valid. Delegates to
         * @ref AbstractAnimator::remove(AnimationHandle), see its
         * documentation for more information.
         */
        void remove(AnimationHandle handle);

        /**
         * @brief Remove an animation assuming it belongs to this animator
         *
         * Compared to @ref remove(AnimationHandle) delegates to
         * @ref AbstractAnimator::remove(AnimatorDataHandle) instead.
         */
        void remove(AnimatorDataHandle handle);

        /**
         * @brief Animation easing function
         *
         * Expects that @p handle is valid. The returned pointer is never
         * @cpp nullptr @ce.
         */
        auto easing(AnimationHandle handle) const -> Float(*)(Float);

        /**
         * @brief Animation easing function assuming it belongs to this animator
         *
         * Like @ref easing(AnimationHandle) const but without checking that
         * @p handle indeed belongs to this animator. See its documentation for
         * more information.
         * @see @ref animationHandleData()
         */
        auto easing(AnimatorDataHandle handle) const -> Float(*)(Float);

    private:
        MAGNUM_UI_LOCAL void createInternal(const AnimationHandle handle);
        MAGNUM_UI_LOCAL void createInternal(const AnimationHandle handle, Containers::Function<void(DataHandle, Float)>&& animation, Float(*const easing)(Float));
        MAGNUM_UI_LOCAL void removeInternal(UnsignedInt id);

        MAGNUM_UI_LOCAL AnimatorFeatures doFeatures() const override;
        MAGNUM_UI_LOCAL void doClean(Containers::BitArrayView animationIdsToRemove) override;
        MAGNUM_UI_LOCAL void doAdvance(Containers::BitArrayView active, Containers::BitArrayView started, Containers::BitArrayView stopped, const Containers::StridedArrayView1D<const Float>& factors) override;

        struct State;
        Containers::Pointer<State> _state;
};

}}

#endif
