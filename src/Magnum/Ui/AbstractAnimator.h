#ifndef Magnum_Ui_AbstractAnimator_h
#define Magnum_Ui_AbstractAnimator_h
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
 * @brief Class @ref Magnum::Ui::AbstractAnimator, @ref Magnum::Ui::AbstractGenericAnimator, @ref Magnum::Ui::AbstractNodeAnimator, @ref Magnum::Ui::AbstractDataAnimator, @ref Magnum::Ui::AbstractStyleAnimator, enum @ref Magnum::Ui::AnimatorFeature, @ref Magnum::Ui::AnimatorState, @ref Magnum::Ui::AnimationFlag, @ref Magnum::Ui::AnimationState, @ref Magnum::Ui::NodeAnimatorUpdate, enum set @ref Magnum::Ui::AnimatorFeatures, @ref Magnum::Ui::AnimatorStates, @ref Magnum::Ui::AnimationFlags, @ref Magnum::Ui::NodeAnimatorUpdates
 * @m_since_latest
 */

#include <Corrade/Containers/Pointer.h>

#include "Magnum/Ui/Ui.h"
#include "Magnum/Ui/visibility.h"

namespace Magnum { namespace Ui {

/**
@brief Features supported by an animator
@m_since_latest

@see @ref AnimatorFeatures, @ref AbstractAnimator::features()
*/
enum class AnimatorFeature: UnsignedByte {
    /**
     * The animations may be attached to nodes and are meant to be
     * automatically removed when given node is removed. Mutually exclusive
     * with @ref AnimatorFeature::DataAttachment, is expected to be always
     * advertised on @ref AbstractNodeAnimator subclasses.
     * @see @ref AbstractAnimator::create(Nanoseconds, Nanoseconds, NodeHandle, UnsignedInt, AnimationFlags),
     *      @ref AbstractAnimator::node(AnimationHandle) const,
     *      @ref AbstractAnimator::cleanNodes()
     */
    NodeAttachment = 1 << 0,

    /**
     * The animations may be attached to layer data and are meant to be
     * automatically removed when given data is removed. Mutually exclusive
     * with @ref AnimatorFeature::NodeAttachment.
     * @see @ref AbstractGenericAnimator::setLayer(),
     *      @ref AbstractLayer::assignAnimator(AbstractDataAnimator&) const,
     *      @ref AbstractLayer::assignAnimator(AbstractStyleAnimator&) const,
     *      @ref AbstractAnimator::create(Nanoseconds, Nanoseconds, DataHandle, UnsignedInt, AnimationFlags),
     *      @ref AbstractAnimator::data(AnimationHandle) const,
     *      @ref AbstractAnimator::cleanData()
     */
    DataAttachment = 1 << 1,
};

/**
@debugoperatorenum{AnimatorFeature}
@m_since_latest
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, AnimatorFeature value);

/**
@brief Set of features supported by an animator
@m_since_latest

@see @ref AbstractAnimator::features()
*/
typedef Containers::EnumSet<AnimatorFeature> AnimatorFeatures;

/**
@debugoperatorenum{AnimatorFeatures}
@m_since_latest
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, AnimatorFeatures value);

CORRADE_ENUMSET_OPERATORS(AnimatorFeatures)

/**
@brief Animator state
@m_since_latest

Used to decide whether @ref AbstractGenericAnimator::advance(),
@ref AbstractNodeAnimator::advance(), @ref BaseLayerStyleAnimator::advance() or
@ref TextLayerStyleAnimator::advance() (called from
@ref AbstractUserInterface::advanceAnimations()) needs to be called. See
@ref UserInterfaceState for interface-wide state.
@see @ref AnimatorStates, @ref AbstractAnimator::state() const
*/
enum class AnimatorState: UnsignedByte {
    /**
     * @ref AbstractAnimator::update(), then optionally a corresponding
     * animator-specific advance function such as
     * @ref AbstractGenericAnimator::advance(),
     * @ref AbstractNodeAnimator::advance() or layer-specific
     * @ref AbstractLayer::advanceAnimations(), and then optionally
     * @ref AbstractAnimator::clean() (which are all called from
     * @ref AbstractUserInterface::advanceAnimations()) needs to be called to
     * advance active animations. Set implicitly after
     * @ref AbstractAnimator::create(), @relativeref{AbstractAnimator,play()},
     * @relativeref{AbstractAnimator,pause()} and
     * @relativeref{AbstractAnimator,update()} that results in at least one
     * animation being @ref AnimationState::Scheduled,
     * @ref AnimationState::Playing or @ref AnimationState::Paused, is reset
     * once @relativeref{AbstractAnimator,update()} results in no animation
     * being in that state anymore.
     */
    NeedsAdvance = 1 << 0
};

/**
@debugoperatorenum{AnimatorState}
@m_since_latest
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, AnimatorState value);

/**
@brief Animator states
@m_since_latest

@see @ref AbstractAnimator::state() const
*/
typedef Containers::EnumSet<AnimatorState> AnimatorStates;

/**
@debugoperatorenum{AnimatorStates}
@m_since_latest
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, AnimatorStates value);

CORRADE_ENUMSET_OPERATORS(AnimatorStates)

/**
@brief Animation flag
@m_since_latest

@see @ref AnimationFlags, @ref AbstractAnimator::create(),
    @ref AbstractAnimator::flags(AnimationHandle) const,
    @ref AbstractAnimator::setFlags(AnimationHandle, AnimationFlags),
    @ref AbstractAnimator::addFlags(AnimationHandle, AnimationFlags),
    @ref AbstractAnimator::clearFlags(AnimationHandle, AnimationFlags)
*/
enum class AnimationFlag: UnsignedByte {
    /**
     * Keep the animation once it's played. By default a call to
     * @ref AbstractAnimator::update() schedules all animations that reached
     * @ref AnimationState::Stopped for removal in a subsequent
     * @ref AbstractAnimator::clean() call. With this flag the animation is
     * kept and is only removable directly with
     * @ref AbstractAnimator::remove().
     */
    KeepOncePlayed = 1 << 0,

    /**
     * Play the animation in reverse direction. The interpolation
     * @ref AbstractAnimator::factor() goes from @cpp 1.0f @ce to @cpp 0.0f @ce
     * instead from @cpp 0.0f @ce to @cpp 1.0f @ce and the animation is
     * guaranteed to be called with the factor being exactly @cpp 0.0f @ce when
     * stopped. If both @ref AnimationFlag::Reverse and
     * @relativeref{AnimationFlag,ReverseEveryOther} is set, only every first,
     * third, ... repeat is reversed.
     *
     * Note that the `started` and `stopped` bits coming from
     * @ref AbstractAnimator::update() are *not* reversed, the `started` bit is
     * always set first time the animation is advanced and `stopped` the last
     * time it's advanced when stopping. Concrete animator implementations such
     * as @ref GenericAnimator or @ref NodeAnimator may expose modified
     * behavior, see their documentation for more information.
     *
     * It's possible to toggle this flag when the animation is playing using
     * @ref AbstractAnimator::setFlags(AnimationHandle, AnimationFlags, Nanoseconds)
     * and related flag APIs that take a time value. The function internally
     * adjusts the start time to make the animation smoothly continue in the
     * opposite direction from the point where it was at that time. In
     * comparison, toggling this flag with
     * @ref AbstractAnimator::setFlags(AnimationHandle, AnimationFlags) will
     * cause it to abruptly skip to a point it would be at if it was playing
     * reversed from the start.
     */
    Reverse = 1 << 1,

    /**
     * Play every other animation repeat in reverse direction. The
     * interpolation @ref AbstractAnimator::factor() goes from @cpp 0.0f @ce to
     * @cpp 1.0f @ce for the first repeat, from @cpp 1.0f @ce to @cpp 0.0f @ce
     * for the second repeat, from from @cpp 0.0f @ce to @cpp 1.0f @ce for the
     * third repeat, etc. If both @ref AnimationFlag::Reverse and
     * @relativeref{AnimationFlag,ReverseEveryOther} is set, the reverse is
     * applied on the odd repeats instead of even --- the factor goes from
     * @cpp 1.0f @ce to @cpp 0.0f @ce for the first repeat, @cpp 0.0f @ce to
     * @cpp 1.0f @ce for second, @cpp 1.0f @ce to @cpp 0.0f @ce for third, etc.
     *
     * As with @ref AnimationFlag::Reverse, the `started` and `stopped` bits
     * coming from @ref AbstractAnimator::update() are *not* affected by
     * presence of this flag.
     *
     * Unlike with @ref AnimationFlag::Reverse, toggling this flag with
     * @ref AbstractAnimator::setFlags(AnimationHandle, AnimationFlags, Nanoseconds)
     * doesn't perform any adjustments to start time as there aren't any clear
     * practical use cases for such behavior.
     */
    ReverseEveryOther = 1 << 2,
};

/**
@debugoperatorenum{AnimationFlag}
@m_since_latest
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, AnimationFlag value);

/**
@brief Animation flags
@m_since_latest

@see @ref AbstractAnimator::create(),
    @ref AbstractAnimator::flags(AnimationHandle) const,
    @ref AbstractAnimator::setFlags(AnimationHandle, AnimationFlags),
    @ref AbstractAnimator::addFlags(AnimationHandle, AnimationFlags),
    @ref AbstractAnimator::clearFlags(AnimationHandle, AnimationFlags)
*/
typedef Containers::EnumSet<AnimationFlag> AnimationFlags;

/**
@debugoperatorenum{AnimationFlags}
@m_since_latest
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, AnimationFlags value);

CORRADE_ENUMSET_OPERATORS(AnimationFlags)

/**
@brief Animation state
@m_since_latest

@see @ref AbstractAnimator::state(), @ref AbstractAnimator::isScheduled(),
    @ref AbstractAnimator::isPlaying(), @ref AbstractAnimator::isPaused(),
    @ref AbstractAnimator::isStopped()
*/
enum class AnimationState: UnsignedByte {
    /**
     * The animation is scheduled to be played. Returned if
     * @ref AbstractAnimator::stopped() is greater than
     * @ref AbstractAnimator::started() for given animation and the time the
     * animation is started at is greater than current
     * @ref AbstractAnimator::time(). Can transition to
     * @ref AnimationState::Playing, @ref AnimationState::Paused or
     * @ref AnimationState::Stopped after the next
     * @ref AbstractAnimator::update().
     */
    Scheduled,

    /**
     * The animation is currently playing. Returned if
     * @ref AbstractAnimator::stopped() is greater than
     * @ref AbstractAnimator::started() for given animation and than current
     * @ref AbstractAnimator::time(), the time the animation is started at is
     * less than or equal to current time, either
     * @ref AbstractAnimator::repeatCount() for given animation is @cpp 0 @ce
     * or @cpp started + duration*repeatCount > time @ce, where `duration` is
     * @ref AbstractAnimator::duration() for given animation, and
     * @ref AbstractAnimator::paused() for given animation is greater than
     * current time. Can transition to @ref AnimationState::Paused or
     * @ref AnimationState::Stopped after the next
     * @ref AbstractAnimator::update().
     */
    Playing,

    /**
     * The animation is currently paused. Returned if
     * @ref AbstractAnimator::stopped() is greater than
     * @ref AbstractAnimator::started() for given animation and than current
     * @ref AbstractAnimator::time(), the time the animation is started at is
     * less than or equal to current time, either
     * @ref AbstractAnimator::repeatCount() for given animation is @cpp 0 @ce
     * or @cpp started + duration*repeatCount > time @ce, where `duration` is
     * @ref AbstractAnimator::duration() for given animation, and
     * @ref AbstractAnimator::paused() for given animation is less than or
     * equal to current time. Can transition to @ref AnimationState::Playing or
     * @ref AnimationState::Stopped after the next
     * @ref AbstractAnimator::update().
     */
    Paused,

    /**
     * The animation is currently stopped. Returned if
     * @ref AbstractAnimator::stopped() is less than or equal to
     * @ref AbstractAnimator::started() for given animation, if the stopped
     * time is less than or equial to current @ref AbstractAnimator::time() or
     * if @ref AbstractAnimator::started() for given animation is less than or
     * equal to current time, @ref AbstractAnimator::repeatCount() for given
     * animation is non-zero and @cpp started + duration*repeatCount <= time @ce,
     * where `duration` is @ref AbstractAnimator::duration() for given
     * animation.
     *
     * Note that @ref AbstractAnimator::update() automatically schedules
     * stopped animations for removal in a subsequent
     * @ref AbstractAnimator::clean() call unless
     * @ref AnimationFlag::KeepOncePlayed is set.
     */
    Stopped
};

/**
@debugoperatorenum{AnimationState}
@m_since_latest
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, AnimationState value);

/**
@brief Base for animators
@m_since_latest
*/
class MAGNUM_UI_EXPORT AbstractAnimator {
    public:
        #ifdef DOXYGEN_GENERATING_OUTPUT
        class DebugIntegration; /* For documentation only */
        #endif

        /**
         * @brief Constructor
         * @param handle    Handle returned by
         *      @ref AbstractUserInterface::createAnimator()
         */
        explicit AbstractAnimator(AnimatorHandle handle);

        /** @brief Copying is not allowed */
        AbstractAnimator(const AbstractAnimator&) = delete;

        /**
         * @brief Move constructor
         *
         * Performs a destructive move, i.e. the original object isn't usable
         * afterwards anymore.
         */
        AbstractAnimator(AbstractAnimator&&) noexcept;

        virtual ~AbstractAnimator();

        /** @brief Copying is not allowed */
        AbstractAnimator& operator=(const AbstractAnimator&) = delete;

        /** @brief Move assignment */
        AbstractAnimator& operator=(AbstractAnimator&&) noexcept;

        /**
         * @brief Animator handle
         *
         * Returns the handle passed to the constructor.
         */
        AnimatorHandle handle() const;

        /** @brief Features exposed by an animator */
        AnimatorFeatures features() const;

        /**
         * @brief Layer handle a data animator is associated with
         *
         * Expects that the animator supports
         * @ref AnimatorFeature::DataAttachment. If the animator isn't an
         * @ref AbstractGenericAnimator, @ref AbstractDataAnimator or an
         * @ref AbstractStyleAnimator and
         * @ref AbstractGenericAnimator::setLayer(),
         * @ref AbstractLayer::assignAnimator(AbstractDataAnimator&) const or
         * @ref AbstractLayer::assignAnimator(AbstractStyleAnimator&) const
         * wasn't called yet, returns @ref LayerHandle::Null.
         * @see @ref features()
         */
        LayerHandle layer() const;

        /**
         * @brief Animator state
         *
         * See the @ref AnimatorState enum for more information. By default no
         * flags are set.
         */
        AnimatorStates state() const;

        /**
         * @brief Animator time
         *
         * Time value last passed to @ref update(). Initial value is
         * @cpp 0_nsec @ce.
         * @see @ref AbstractUserInterface::animationTime()
         */
        Nanoseconds time() const;

        /**
         * @brief Current capacity of the data storage
         *
         * Can be at most 1048576. If @ref create() is called and there's no
         * free slots left, the internal storage gets grown.
         * @see @ref usedCount()
         */
        std::size_t capacity() const;

        /**
         * @brief Count of used items in the data storage
         *
         * Always at most @ref capacity(). Expired handles are counted among
         * used as well. The operation is done with a @f$ \mathcal{O}(n) @f$
         * complexity where @f$ n @f$ is @ref capacity().
         */
        std::size_t usedCount() const;

        /**
         * @brief Whether an animation handle is valid
         *
         * A handle is valid if it has been returned from @ref create() before
         * and @ref remove() wasn't called on it yet. For
         * @ref AnimatorDataHandle::Null always returns @cpp false @ce.
         */
        bool isHandleValid(AnimatorDataHandle handle) const;

        /**
         * @brief Whether an animation handle is valid
         *
         * A shorthand for extracting an @ref AnimatorHandle from @p handle
         * using @ref animationHandleAnimator(), comparing it to @ref handle()
         * and if it's the same, calling @ref isHandleValid(AnimatorDataHandle) const
         * with an @ref AnimatorDataHandle extracted from @p handle using
         * @ref animationHandleData(). See these functions for more
         * information. For @ref AnimationHandle::Null,
         * @ref AnimatorHandle::Null or @ref AnimatorDataHandle::Null always
         * returns @cpp false @ce.
         */
        bool isHandleValid(AnimationHandle handle) const;

        /**
         * @brief Duration of one animation play
         *
         * Expects that @p handle is valid. The duration is specified with
         * @ref create() and is always non-negative. The duration, together
         * with @ref started(AnimationHandle) const,
         * @ref paused(AnimationHandle) const,
         * @ref stopped(AnimationHandle) const,
         * @ref repeatCount(AnimationHandle) const and @ref time() is used to
         * decide on a particular @ref AnimationState for given animation. See
         * also @ref duration(AnimatorDataHandle) const which is a simpler
         * operation if the animation is already known to belong to this
         * animator.
         * @see @ref isHandleValid(AnimationHandle) const,
         *      @ref state(AnimationHandle) const
         */
        Nanoseconds duration(AnimationHandle handle) const;

        /**
         * @brief Duration of one animation play assuming it belongs to this animator
         *
         * Like @ref duration(AnimationHandle) const but without checking that
         * @p handle indeed belongs to this animator. See its documentation for
         * more information.
         * @see @ref animationHandleData()
         */
        Nanoseconds duration(AnimatorDataHandle handle) const;

        /**
         * @brief Animation repeat count
         *
         * Expects that @p handle is valid. The repeat count is specified with
         * @ref create() and can be subsequently modified with
         * @ref setRepeatCount(). Value of @cpp 0 @ce means the animation is
         * repeated indefinitely. The repeat count, together with
         * @ref duration(AnimationHandle) const,
         * @ref started(AnimationHandle) const,
         * @ref paused(AnimationHandle) const,
         * @ref stopped(AnimationHandle) const and @ref time() is used to
         * decide on a particular @ref AnimationState for given animation. See
         * also @ref repeatCount(AnimatorDataHandle) const which is a simpler
         * operation if the animation is already known to belong to this
         * animator.
         * @see @ref isHandleValid(AnimationHandle) const,
         *      @ref state(AnimationHandle) const
         */
        UnsignedInt repeatCount(AnimationHandle handle) const;

        /**
         * @brief Animation repeat count assuming it belongs to this animator
         *
         * Like @ref repeatCount(AnimationHandle) const but without checking
         * that @p handle indeed belongs to this animator. See its
         * documentation for more information.
         * @see @ref animationHandleData()
         */
        UnsignedInt repeatCount(AnimatorDataHandle handle) const;

        /**
         * @brief Set animation repeat count
         *
         * Expects that @p handle is valid. Use @cpp 0 @ce for an indefinitely
         * repeating animation. For an animation with zero duration expects
         * that the @p count is @cpp 1 @ce. The repeat count, together with
         * @ref duration(AnimationHandle) const,
         * @ref started(AnimationHandle) const,
         * @ref paused(AnimationHandle) const,
         * @ref stopped(AnimationHandle) const and @ref time() is subsequently
         * used to decide on a particular @ref AnimationState for given
         * animation. See also @ref setRepeatCount(AnimatorDataHandle, UnsignedInt)
         * which is a simpler operation if the animation is already known to
         * belong to this animator.
         *
         * Compared to @ref create() or @ref play(), a change in repeat count
         * doesn't change the animation to @ref AnimationState::Scheduled,
         * @ref AnimationState::Playing or @ref AnimationState::Paused if it
         * wasn't there already, thus calling this function never causes
         * @ref AnimatorState::NeedsAdvance to be set.
         * @see @ref isHandleValid(AnimationHandle) const,
         *      @ref state(AnimationHandle) const
         */
        void setRepeatCount(AnimationHandle handle, UnsignedInt count);

        /**
         * @brief Set animation repeat count assuming it belongs to this animator
         *
         * Like @ref setRepeatCount(AnimationHandle, UnsignedInt) but without
         * checking that @p handle indeed belongs to this animator. See its
         * documentation for more information.
         * @see @ref animationHandleData()
         */
        void setRepeatCount(AnimatorDataHandle handle, UnsignedInt count);

        /**
         * @brief Animation flags
         *
         * Expects that @p handle is valid. The flags are specified with
         * @ref create() and can be subsequently modified with
         * @ref setFlags(), @ref addFlags() or @ref clearFlags(). See also
         * @ref flags(AnimatorDataHandle) const which is a simpler operation if
         * the animation is already known to belong to this animator.
         * @see @ref isHandleValid(AnimationHandle) const
         */
        AnimationFlags flags(AnimationHandle handle) const;

        /**
         * @brief Animation flags assuming it belongs to this animator
         *
         * Like @ref flags(AnimationHandle) const but without checking that
         * @p handle indeed belongs to this animator. See its documentation for
         * more information.
         * @see @ref animationHandleData()
         */
        AnimationFlags flags(AnimatorDataHandle handle) const;

        /**
         * @brief Flags for all animations
         *
         * Meant to be used by animator implementations to query flags based on
         * animation IDs or masks without knowing their full handles,
         * application code should use @ref flags(AnimationHandle) const /
         * @ref flags(AnimatorDataHandle) const instead. Size of the returned
         * view is the same as @ref capacity(). Items that don't correspond to
         * valid handles have unspecified values.
         */
        Containers::StridedArrayView1D<const AnimationFlags> flags() const;

        /**
         * @brief Set animation flags
         *
         * Expects that @p handle is valid. See also
         * @ref setFlags(AnimatorDataHandle, AnimationFlags) which is a simpler
         * operation if the animation is already known to belong to this
         * animator.
         *
         * Note that toggling @ref AnimationFlag::Reverse or
         * @relativeref{AnimationFlag,ReverseEveryOther} with this function
         * while the animation is playing will cause it to abruptly jump to a
         * different point. Use @ref setFlags(AnimationHandle, AnimationFlags, Nanoseconds)
         * to make the animation continue in the opposite direction from the
         * point where it was at given time.
         *
         * Compared to @ref create(), @ref play() or @ref pause(), a change in
         * flags never causes the animation to become
         * @ref AnimationState::Scheduled, @ref AnimationState::Playing or
         * @ref AnimationState::Paused if it wasn't before already, thus
         * calling this function doesn't cause @ref AnimatorState::NeedsAdvance
         * to be set.
         * @see @ref isHandleValid(AnimationHandle) const, @ref addFlags(),
         *      @ref clearFlags()
         */
        void setFlags(AnimationHandle handle, AnimationFlags flags);

        /**
         * @brief Set animation flags assuming it belongs to this animator
         *
         * Like @ref setFlags(AnimationHandle, AnimationFlags) but without
         * checking that @p handle indeed belongs to this animator. See its
         * documentation for more information.
         * @see @ref animationHandleData()
         */
        void setFlags(AnimatorDataHandle handle, AnimationFlags flags);

        /**
         * @brief Add animation flags
         *
         * Calls @ref setFlags(AnimationHandle, AnimationFlags) with the
         * existing flags ORed with @p flags. Useful for preserving previously
         * set flags. See also @ref addFlags(AnimatorDataHandle, AnimationFlags)
         * which is a simpler operation if the animation is already known to
         * belong to this animator.
         * @see @ref clearFlags()
         */
        void addFlags(AnimationHandle handle, AnimationFlags flags);

        /**
         * @brief Add animation flags assuming it belongs to this animator
         *
         * Like @ref addFlags(AnimationHandle, AnimationFlags) but without
         * checking that @p handle indeed belongs to this animator. See its
         * documentation for more information.
         * @see @ref animationHandleData()
         */
        void addFlags(AnimatorDataHandle handle, AnimationFlags flags);

        /**
         * @brief Clear animation flags
         *
         * Calls @ref setFlags(AnimationHandle, AnimationFlags) with the
         * existing flags ANDed with the inverse of @p flags. Useful for
         * removing a subset of previously set flags. See also
         * @ref clearFlags(AnimatorDataHandle, AnimationFlags) which is a
         * simpler operation if the animation is already known to belong to
         * this animator.
         * @see @ref addFlags()
         */
        void clearFlags(AnimationHandle handle, AnimationFlags flags);

        /**
         * @brief Clear animation flags assuming it belongs to this animator
         *
         * Like @ref clearFlags(AnimationHandle, AnimationFlags) but without
         * checking that @p handle indeed belongs to this animator. See its
         * documentation for more information.
         * @see @ref animationHandleData()
         */
        void clearFlags(AnimatorDataHandle handle, AnimationFlags flags);

        /**
         * @brief Time at which an animation is started
         *
         * Expects that @p handle is valid. The time an animation is started at
         * is specified with @ref create() and is subsequently affected by
         * calling @ref play() or @ref stop(). The specified time, together
         * with @ref duration(AnimationHandle) const,
         * @ref repeatCount(AnimationHandle) const,
         * @ref paused(AnimationHandle) const,
         * @ref stopped(AnimationHandle) const and @ref time() is used to
         * decide on a particular @ref AnimationState for given animation. See
         * also @ref started(AnimatorDataHandle) const which is a simpler
         * operation if the animation is already known to belong to this
         * animator.
         *
         * Use @ref state(AnimationHandle) const, @ref isScheduled(),
         * @ref isPlaying(), @ref isPaused() or @ref isStopped() for checking
         * what state is the animation in at current @ref time().
         * @see @ref isHandleValid(AnimationHandle) const
         */
        Nanoseconds started(AnimationHandle handle) const;

        /**
         * @brief Time at which an animation is started assuming it belongs to this animator
         *
         * Like @ref started(AnimationHandle) const but without checking that
         * @p handle indeed belongs to this animator. See its documentation for
         * more information.
         * @see @ref animationHandleData()
         */
        Nanoseconds started(AnimatorDataHandle handle) const;

        /**
         * @brief Animation paused time
         *
         * Expects that @p handle is valid. The paused time is initially
         * @ref Nanoseconds::max() and is affected by calling @ref play() or
         * @ref pause(). The paused time, together with
         * @ref duration(AnimationHandle) const,
         * @ref repeatCount(AnimationHandle) const,
         * @ref started(AnimationHandle) const,
         * @ref stopped(AnimationHandle) const and @ref time() is used to
         * decide on a particular @ref AnimationState for given animation. See
         * also @ref paused(AnimatorDataHandle) const which is a simpler
         * operation if the animation is already known to belong to this
         * animator.
         *
         * Use @ref state(AnimationHandle) const, @ref isScheduled(),
         * @ref isPlaying(), @ref isPaused() or @ref isStopped() for checking
         * what state is the animation in at current @ref time().
         * @see @ref isHandleValid(AnimationHandle) const
         */
        Nanoseconds paused(AnimationHandle handle) const;

        /**
         * @brief Animation paused time assuming it belongs to this animator
         *
         * Like @ref paused(AnimationHandle) const but without checking that
         * @p handle indeed belongs to this animator. See its documentation for
         * more information.
         * @see @ref animationHandleData()
         */
        Nanoseconds paused(AnimatorDataHandle handle) const;

        /**
         * @brief Animation stopped time
         *
         * Expects that @p handle is valid. The stopped time is initially
         * @ref Nanoseconds::max() and is affected by calling @ref play() or
         * @ref stop(). The stopped time, together with
         * @ref started(AnimationHandle) const,
         * @ref paused(AnimationHandle) const,
         * @ref duration(AnimationHandle) const,
         * @ref repeatCount(AnimationHandle) const and @ref time() is used to
         * decide on a particular @ref AnimationState for given animation. See
         * also @ref stopped(AnimatorDataHandle) const which is a simpler
         * operation if the animation is already known to belong to this
         * animator.
         *
         * Use @ref state(AnimationHandle) const, @ref isScheduled(),
         * @ref isPlaying(), @ref isPaused() or @ref isStopped() for checking
         * what state is the animation in at current @ref time().
         * @see @ref isHandleValid(AnimationHandle) const
         */
        Nanoseconds stopped(AnimationHandle handle) const;

        /**
         * @brief Animation stopped time assuming it belongs to this animator
         *
         * Like @ref stopped(AnimationHandle) const but without checking that
         * @p handle indeed belongs to this animator. See its documentation for
         * more information.
         * @see @ref animationHandleData()
         */
        Nanoseconds stopped(AnimatorDataHandle handle) const;

        /**
         * @brief Attach an animation to a node
         *
         * Makes the @p animation handle tied to a particular @p node, meaning
         * it'll get scheduled for removal during the next @ref cleanNodes()
         * call when @ref AbstractUserInterface::removeNode() is called for
         * @p node or any parent node.
         *
         * Expects that @p animation is valid and that the animator supports
         * @ref AnimatorFeature::NodeAttachment. The @p node can be anything
         * including @ref NodeHandle::Null, but if it's non-null and not valid
         * the animation will be scheduled for deletion during the next
         * @ref cleanNodes() call. If the @p animation is already attached to
         * some node, this will overwrite the previous attachment --- i.e.,
         * it's not possible to have the same animation attached to multiple
         * nodes. The inverse, attaching multiple different animation handles
         * to a single node, is supported however.
         *
         * Unlike with e.g. @ref AbstractLayer::attach(), calling this function
         * does *not* cause any @ref AnimatorState to be set.
         * @see @ref isHandleValid(AnimationHandle) const,
         *      @ref create(Nanoseconds, Nanoseconds, NodeHandle, UnsignedInt, AnimationFlags),
         *      @ref AbstractUserInterface::attachAnimation(NodeHandle, AnimationHandle)
         */
        void attach(AnimationHandle animation, NodeHandle node);

        /**
         * @brief Attach an animation to a node assuming it belongs to this animator
         *
         * Like @ref attach(AnimationHandle, NodeHandle) but without checking
         * that @p animation indeed belongs to this animator. See its
         * documentation for more information.
         */
        void attach(AnimatorDataHandle animation, NodeHandle node);

        /**
         * @brief Node handle an animation is attached to
         *
         * Expects that @p handle is valid and that the animator supports
         * @ref AnimatorFeature::NodeAttachment. If the animation isn't
         * attached to any node, returns @ref NodeHandle::Null.
         *
         * The returned handle may be invalid if either the animation got
         * attached to an invalid node in the first place or the node or any of
         * its parents were removed and @ref AbstractUserInterface::clean()
         * wasn't called since.
         * @see @ref isHandleValid(AnimationHandle) const, @ref features()
         */
        NodeHandle node(AnimationHandle handle) const;

        /**
         * @brief Node handle an animation is attached to assuming the animation belongs to this animator
         *
         * Like @ref node(AnimationHandle) const but without checking that
         * @p handle indeed belongs to this animator. See its documentation for
         * more information.
         * @see @ref animationHandleData()
         */
        NodeHandle node(AnimatorDataHandle handle) const;

        /**
         * @brief Node attachments for all animations
         *
         * Expects that the animator supports
         * @ref AnimatorFeature::NodeAttachment. Meant to be used by animator
         * implementations to query node attachments based on animation IDs or
         * masks without knowing their full handles, application code should
         * use @ref node(AnimationHandle) const /
         * @ref node(AnimatorDataHandle) const instead. Size of the returned
         * view is the same as @ref capacity(). Items that are
         * @ref NodeHandle::Null are either animations with no node attachments
         * or corresponding to animations that are freed.
         */
        Containers::StridedArrayView1D<const NodeHandle> nodes() const;

        /**
         * @brief Attach an animation to a data
         *
         * Makes the @p animation handle tied to a particular @p data, meaning
         * it'll get scheduled for removal during the next @ref cleanData()
         * call when @ref AbstractLayer::remove() is called for @p data or when
         * it gets removed as a consequence of a node removal.
         *
         * Expects that @p animation is valid and that the animator supports
         * @ref AnimatorFeature::DataAttachment. The @p data is expected to be
         * either @ref DataHandle::Null or with the layer part matching
         * @ref layer() const. Other than that it can be anything but if it's
         * not @ref DataHandle::Null and not valid the animation will be
         * scheduled for deletion during the next @ref cleanData() call. If the
         * @p animation is already attached to some data, this will overwrite
         * the previous attachment --- i.e., it's not possible to have the same
         * animation attached to multiple data. The inverse, attaching multiple
         * different animation handles to a single data, is supported however.
         *
         * Unlike with e.g. @ref AbstractLayer::attach(), calling this function
         * does *not* cause any @ref AnimatorState to be set.
         * @see @ref isHandleValid(AnimationHandle) const,
         *      @ref dataHandleLayer(),
         *      @ref create(Nanoseconds, Nanoseconds, DataHandle, UnsignedInt, AnimationFlags),
         *      @ref AbstractUserInterface::attachAnimation(DataHandle, AnimationHandle)
         */
        void attach(AnimationHandle animation, DataHandle data);

        /**
         * @brief Attach an animation to a data assuming the animation belongs to this animator
         *
         * Like @ref attach(AnimationHandle, DataHandle) but without checking
         * that @p animation indeed belongs to this animator. See its
         * documentation for more information.
         */
        void attach(AnimatorDataHandle animation, DataHandle data);

        /**
         * @brief Attach an animation to a data assuming the data belongs to a layer registered with this animator
         *
         * Like @ref attach(AnimationHandle, DataHandle) but without checking
         * that @p data is indeed coming from @ref layer() const. See its
         * documentation for more information. Calling this function with
         * @ref LayerDataHandle::Null is equivalent to calling
         * @ref attach(AnimationHandle, DataHandle) with @ref DataHandle::Null.
         */
        void attach(AnimationHandle animation, LayerDataHandle data);

        /**
         * @brief Attach an animation to a data assuming the animation belongs to this animator and the data belongs to a layer registered with this animator
         *
         * Like @ref attach(AnimationHandle, DataHandle) but without checking
         * that @p animation indeed belongs to this animator and that
         * that @p data is indeed coming from @ref layer() const. See its
         * documentation for more information. Calling this function with
         * @ref LayerDataHandle::Null is equivalent to calling
         * @ref attach(AnimationHandle, DataHandle) with @ref DataHandle::Null.
         */
        void attach(AnimatorDataHandle animation, LayerDataHandle data);

        /**
         * @brief Data handle an animation is attached to
         *
         * Expects that @p handle is valid and that the animator supports
         * @ref AnimatorFeature::DataAttachment. If the animation isn't
         * attached to any data, returns @ref DataHandle::Null, otherwise the
         * layer portion of the handle is always equal to @ref layer() const.
         *
         * The returned handle may be invalid if either the animation got
         * attached to an invalid data in the first place or the data was
         * removed and @ref AbstractUserInterface::clean() wasn't called since.
         * @see @ref isHandleValid(AnimationHandle) const, @ref features()
         */
        DataHandle data(AnimationHandle handle) const;

        /**
         * @brief Data handle an animation is attached to assuming the animation belongs to this animator
         *
         * Like @ref data(AnimationHandle) const but without checking that
         * @p handle indeed belongs to this animator. See its documentation for
         * more information.
         * @see @ref animationHandleData()
         */
        DataHandle data(AnimatorDataHandle handle) const;

        /**
         * @brief Layer data attachments for all animations
         *
         * Expects that the animator supports
         * @ref AnimatorFeature::DataAttachment. Meant to be used by animator
         * implementations to query layer data attachments based on animation
         * IDs or masks without knowing their full handles, application code
         * should use @ref data(AnimationHandle) const /
         * @ref data(AnimatorDataHandle) const instead. Size of the returned
         * view is the same as @ref capacity(). Items that are
         * @ref LayerDataHandle::Null are either animations with no data
         * attachments or corresponding to animations that are freed. Use
         * @ref dataHandle(LayerHandle, LayerDataHandle) with @ref layer() const
         * to convert given handle to a @ref DataHandle if needed.
         */
        Containers::StridedArrayView1D<const LayerDataHandle> layerData() const;

        /**
         * @brief Animation state
         *
         * Expects that @p handle is valid. Calculated based on the value of
         * @ref time() recorded in last @ref update(),
         * @ref duration(AnimationHandle) const,
         * @ref repeatCount(AnimationHandle) const,
         * @ref started(AnimationHandle) const,
         * @ref paused(AnimationHandle) const and
         * @ref stopped(AnimationHandle) const for a particular animation. See
         * also @ref state(AnimatorDataHandle) const which is a simpler
         * operation if the animation is already known to belong to this
         * animator.
         * @see @ref isHandleValid(AnimationHandle) const, @ref factor(),
         *      @ref isScheduled(), @ref isPlaying(), @ref isPaused(),
         *      @ref isStopped()
         */
        AnimationState state(AnimationHandle handle) const;

        /**
         * @brief Animation state assuming it belongs to this animator
         *
         * Like @ref state(AnimationHandle) const but without checking that
         * @p handle indeed belongs to this animator. See its documentation for
         * more information.
         * @see @ref animationHandleData()
         */
        AnimationState state(AnimatorDataHandle handle) const;

        /**
         * @brief Whether the animation is scheduled
         *
         * Convenience shorthand comparing the result of
         * @ref state(AnimationHandle) const to @ref AnimationState::Scheduled.
         * See documentation of @ref state(AnimationHandle) const for more
         * information.
         * @see @ref started()
         */
        bool isScheduled(AnimationHandle handle) const;

        /**
         * @brief Whether the animation is scheduled assuming it belongs to this animator
         *
         * Convenience shorthand comparing the result of
         * @ref state(AnimatorDataHandle) const to @ref AnimationState::Scheduled.
         * See documentation of @ref state(AnimatorDataHandle) const for more
         * information.
         * @see @ref started()
         */
        bool isScheduled(AnimatorDataHandle handle) const;

        /**
         * @brief Whether the animation is playing
         *
         * Convenience shorthand comparing the result of
         * @ref state(AnimationHandle) const to @ref AnimationState::Playing.
         * See documentation of @ref state(AnimationHandle) const for more
         * information.
         * @see @ref started()
         */
        bool isPlaying(AnimationHandle handle) const;

        /**
         * @brief Whether the animation is playing assuming it belongs to this animator
         *
         * Convenience shorthand comparing the result of
         * @ref state(AnimatorDataHandle) const to @ref AnimationState::Playing.
         * See documentation of @ref state(AnimatorDataHandle) const for more
         * information.
         * @see @ref started()
         */
        bool isPlaying(AnimatorDataHandle handle) const;

        /**
         * @brief Whether the animation is paused
         *
         * Convenience shorthand comparing the result of
         * @ref state(AnimationHandle) const to @ref AnimationState::Paused.
         * See documentation of @ref state(AnimationHandle) const for more
         * information.
         * @see @ref paused()
         */
        bool isPaused(AnimationHandle handle) const;

        /**
         * @brief Whether the animation is paused assuming it belongs to this animator
         *
         * Convenience shorthand comparing the result of
         * @ref state(AnimatorDataHandle) const to @ref AnimationState::Paused.
         * See documentation of @ref state(AnimatorDataHandle) const for more
         * information.
         * @see @ref paused()
         */
        bool isPaused(AnimatorDataHandle handle) const;

        /**
         * @brief Whether the animation is stopped
         *
         * Convenience shorthand comparing the result of
         * @ref state(AnimationHandle) const to @ref AnimationState::Stopped.
         * See documentation of @ref state(AnimationHandle) const for more
         * information.
         * @see @ref stopped()
         */
        bool isStopped(AnimationHandle handle) const;

        /**
         * @brief Whether the animation is stopped assuming it belongs to this animator
         *
         * Convenience shorthand comparing the result of
         * @ref state(AnimatorDataHandle) const to @ref AnimationState::Stopped.
         * See documentation of @ref state(AnimatorDataHandle) const for more
         * information.
         * @see @ref stopped()
         */
        bool isStopped(AnimatorDataHandle handle) const;

        /**
         * @brief Animation interpolation factor
         *
         * Expects that @p handle is valid. Calculated based on the value of
         * @ref time() recorded in last @ref update(),
         * @ref duration(AnimationHandle) const,
         * @ref repeatCount(AnimationHandle) const,
         * @ref started(AnimationHandle) const,
         * @ref paused(AnimationHandle) const and
         * @ref stopped(AnimationHandle) const for a particular animation. The
         * returned value is always in the @f$ [0, 1] @f$ range and matches
         * what would be returned from @ref update() for given animation at
         * @ref time(). For @ref AnimationState::Scheduled always returns
         * @cpp 0.0f @ce, for @ref AnimationState::Stopped returns either
         * @cpp 1.0f @ce or @cpp 0.0f @ce based on presence of the
         * @ref AnimationFlag::Reverse and @relativeref{AnimationFlag,ReverseEveryOther}
         * flags and, with the latter, also on repeat count.
         * @see @ref state(AnimationHandle) const
         */
        Float factor(AnimationHandle handle) const;

        /**
         * @brief Animation interpolation factor assuming it belongs to this animator
         *
         * Like @ref factor(AnimationHandle) const but without checking that
         * @p handle indeed belongs to this animator. See its documentation for
         * more information.
         * @see @ref animationHandleData()
         */
        Float factor(AnimatorDataHandle handle) const;

        /**
         * @brief Play an animation from start or resume a paused one
         *
         * Expects that @p handle is valid. The function effectively just
         * updates the value of @ref started(AnimationHandle) const and sets
         * both @ref paused(AnimationHandle) const and
         * @ref stopped(AnimationHandle) const to @ref Nanoseconds::max(). The
         * actual @ref AnimationState is then decided based on these three
         * times together with @ref duration(AnimationHandle) const,
         * @ref repeatCount(AnimationHandle) const and @ref time().
         *
         * If the animation is already playing, this functions makes it play
         * from start. Use @ref resume() to start the animation only if it's
         * not already playing.
         *
         * If calling this function resulted in the animation being
         * @ref AnimationState::Scheduled or @ref AnimationState::Playing, the
         * @ref AnimatorState::NeedsAdvance flag is set.
         * @see @ref isHandleValid(AnimationHandle) const,
         *      @ref state(AnimationHandle) const
         */
        void play(AnimationHandle handle, Nanoseconds time);

        /**
         * @brief Play an animation from start or resume a paused one assuming it belongs to this animator
         *
         * Like @ref play(AnimationHandle, Nanoseconds) but without checking
         * that @p handle indeed belongs to this animator. See its
         * documentation for more information.
         * @see @ref animationHandleData()
         */
        void play(AnimatorDataHandle handle, Nanoseconds time);

        /**
         * @brief Resume an animation
         *
         * Expects that @p handle is valid. If the animation is already
         * playing at @p time, does nothing. Otherwise delegates to
         * @ref play(), see its documentation for more information.
         * @see @ref isHandleValid(AnimationHandle) const,
         *      @ref state(AnimationHandle) const
         */
        void resume(AnimationHandle handle, Nanoseconds time);

        /**
         * @brief Resume an animation assuming it belongs to this animator
         *
         * Like @ref resume(AnimationHandle, Nanoseconds) but without checking
         * that @p handle indeed belongs to this animator. See its
         * documentation for more information.
         * @see @ref animationHandleData()
         */
        void resume(AnimatorDataHandle handle, Nanoseconds time);

        /**
         * @brief Pause an animation
         *
         * Expects that @p handle is valid. The function effectively just
         * updates the value of @ref paused(AnimationHandle) const; the actual
         * @ref AnimationState is then decided based on the paused time
         * together with @ref duration(AnimationHandle) const,
         * @ref repeatCount(AnimationHandle) const,
         * @ref started(AnimationHandle) const,
         * @ref stopped(AnimationHandle) const and @ref time().
         *
         * If calling this function resulted in the animation being
         * @ref AnimationState::Scheduled, @ref AnimationState::Playing (for
         * example if the pause time is moved to the future) or
         * @ref AnimationState::Paused, the @ref AnimatorState::NeedsAdvance
         * flag is set.
         * @see @ref isHandleValid(AnimationHandle) const,
         *      @ref state(AnimationHandle) const
         */
        void pause(AnimationHandle handle, Nanoseconds time);

        /**
         * @brief Pause an animation assuming it belongs to this animator
         *
         * Like @ref pause(AnimationHandle, Nanoseconds) but without checking
         * that @p handle indeed belongs to this animator. See its
         * documentation for more information.
         * @see @ref animationHandleData()
         */
        void pause(AnimatorDataHandle handle, Nanoseconds time);

        /**
         * @brief Stop an animation
         *
         * Expects that @p handle is valid. The function effectively just
         * updates the value of @ref stopped(AnimationHandle) const; the actual
         * @ref AnimationState is then decided based on the stopped time
         * together with @ref duration(AnimationHandle) const,
         * @ref repeatCount(AnimationHandle) const,
         * @ref started(AnimationHandle) const,
         * @ref paused(AnimationHandle) const and @ref time().
         *
         * Compared to @ref create(), @ref play() or @ref pause(), a change in
         * stopped time never causes the animation to become
         * @ref AnimationState::Scheduled, @ref AnimationState::Playing or
         * @ref AnimationState::Paused if it wasn't before already, thus
         * calling this function doesn't cause @ref AnimatorState::NeedsAdvance
         * to be set.
         * @see @ref isHandleValid(AnimationHandle) const,
         *      @ref state(AnimationHandle) const
         */
        void stop(AnimationHandle handle, Nanoseconds time);

        /**
         * @brief Stop an animation assuming it belongs to this animator
         *
         * Like @ref stop(AnimationHandle, Nanoseconds) but without checking
         * that @p handle indeed belongs to this animator. See its
         * documentation for more information.
         * @see @ref animationHandleData()
         */
        void stop(AnimatorDataHandle handle, Nanoseconds time);

        /**
         * @brief Set animation flags at given time
         *
         * Expects that @p handle is valid. Compared to
         * @ref setFlags(AnimationHandle, AnimationFlags), toggling
         * @ref AnimationFlag::Reverse with this function while the animation
         * is playing will adjust its start time to make it continue in the
         * opposite direction from the point where it was at given @p time.
         * With repeated animations only the time within a single iteration is
         * adjusted, not the remaining repeat count. Toggling any other flags
         * has no difference compared to @ref setFlags(AnimationHandle, AnimationFlags).
         * See also @ref setFlags(AnimatorDataHandle, AnimationFlags, Nanoseconds)
         * which is a simpler operation if the animation is already known to
         * belong to this animator.
         *
         * Compared to @ref create(), @ref play() or @ref pause(), a change in
         * flags never causes the animation to become
         * @ref AnimationState::Scheduled, @ref AnimationState::Playing or
         * @ref AnimationState::Paused if it wasn't before already, thus
         * calling this function doesn't cause @ref AnimatorState::NeedsAdvance
         * to be set.
         * @see @ref isHandleValid(AnimationHandle) const,
         *      @ref addFlags(AnimationHandle, AnimationFlags, Nanoseconds),
         *      @ref clearFlags(AnimationHandle, AnimationFlags, Nanoseconds)
         */
        void setFlags(AnimationHandle handle, AnimationFlags flags, Nanoseconds time);

        /**
         * @brief Set animation flags at given time assuming it belongs to this animator
         *
         * Like @ref setFlags(AnimationHandle, AnimationFlags, Nanoseconds) but
         * without checking that @p handle indeed belongs to this animator. See
         * its documentation for more information.
         * @see @ref animationHandleData()
         */
        void setFlags(AnimatorDataHandle handle, AnimationFlags flags, Nanoseconds time);

        /**
         * @brief Add animation flags at given time
         *
         * Calls @ref setFlags(AnimationHandle, AnimationFlags, Nanoseconds)
         * with the existing flags ORed with @p flags. Useful for preserving
         * previously set flags. See also
         * @ref addFlags(AnimatorDataHandle, AnimationFlags, Nanoseconds) which
         * is a simpler operation if the animation is already known to belong
         * to this animator.
         * @see @ref clearFlags(AnimationHandle, AnimationFlags, Nanoseconds)
         */
        void addFlags(AnimationHandle handle, AnimationFlags flags, Nanoseconds time);

        /**
         * @brief Add animation flags at given time assuming it belongs to this animator
         *
         * Like @ref addFlags(AnimationHandle, AnimationFlags, Nanoseconds) but
         * without checking that @p handle indeed belongs to this animator. See
         * its documentation for more information.
         * @see @ref animationHandleData()
         */
        void addFlags(AnimatorDataHandle handle, AnimationFlags flags, Nanoseconds time);

        /**
         * @brief Clear animation flags at given time
         *
         * Calls @ref setFlags(AnimationHandle, AnimationFlags, Nanoseconds)
         * with the existing flags ANDed with the inverse of @p flags. Useful
         * for removing a subset of previously set flags. See also
         * @ref clearFlags(AnimatorDataHandle, AnimationFlags, Nanoseconds)
         * which is a simpler operation if the animation is already known to
         * belong to this animator.
         * @see @ref addFlags(AnimationHandle, AnimationFlags, Nanoseconds)
         */
        void clearFlags(AnimationHandle handle, AnimationFlags flags, Nanoseconds time);

        /**
         * @brief Clear animation flags at given time assuming it belongs to this animator
         *
         * Like @ref clearFlags(AnimationHandle, AnimationFlags, Nanoseconds)
         * but without checking that @p handle indeed belongs to this animator.
         * See its documentation for more information.
         * @see @ref animationHandleData()
         */
        void clearFlags(AnimatorDataHandle handle, AnimationFlags flags, Nanoseconds time);

        /**
         * @brief Generation counters for all animations
         *
         * Meant to be used by code that only gets animation IDs or masks but
         * needs the full handle, or for various diagnostic purposes such as
         * tracking handle recycling. Size of the returned view is the same as
         * @ref capacity(), individual items correspond to generations of
         * particular animation IDs. All values fit into the
         * @ref AnimationHandle / @ref AnimatorDataHandle generation bits,
         * @cpp 0 @ce denotes an expired generation counter.
         *
         * Passing an ID along with the corresponding generation to
         * @ref animatorDataHandle() produces an @ref AnimatorDataHandle,
         * passing that along with @ref handle() to @ref animationHandle()
         * produces an @ref AnimationHandle. Use
         * @ref isHandleValid(AnimatorDataHandle) const /
         * @ref isHandleValid(AnimationHandle) const to determine whether given
         * slot is actually used.
         */
        Containers::StridedArrayView1D<const UnsignedShort> generations() const;

        /**
         * @brief Clean no longer valid animations
         *
         * Used internally from subclass implementations such as
         * @ref AbstractGenericAnimator::advance(), which is called from
         * @ref AbstractUserInterface::advanceAnimations(). Exposed just for
         * testing purposes, there should be no need to call this function
         * directly and doing so may cause internal @ref AbstractUserInterface
         * state update to misbehave. Expects that the @p animationIdsToRemove
         * view has the same size as @ref capacity().
         *
         * Animations which have a corresponding bit set in
         * @p animationIdsToRemove are removed. Delegates to @ref doClean(),
         * see its documentation for more information about the arguments.
         */
        void clean(Containers::BitArrayView animationIdsToRemove);

        /**
         * @brief Clean animations attached to no longer valid nodes
         *
         * Used internally from @ref AbstractUserInterface::clean(). Exposed
         * just for testing purposes, there should be no need to call this
         * function directly and doing so may cause internal
         * @ref AbstractUserInterface state update to misbehave. Expects that
         * the animator supports @ref AnimatorFeature::NodeAttachment, assumes
         * that @p nodeHandleGenerations contains handle generation counters
         * for all nodes, where the index is implicitly the handle ID. They're
         * used to decide about node attachment validity, animations with
         * invalid node attachments are then removed. Delegates to
         * @ref doClean(), see its documentation for more information about the
         * arguments.
         */
        void cleanNodes(const Containers::StridedArrayView1D<const UnsignedShort>& nodeHandleGenerations);

        /**
         * @brief Clean animations attached to no longer valid data
         *
         * Used internally from @ref AbstractUserInterface::clean(). Exposed
         * just for testing purposes, there should be no need to call this
         * function directly and doing so may cause internal
         * @ref AbstractUserInterface state update to misbehave. Expects that
         * the animator supports @ref AnimatorFeature::DataAttachment, assumes
         * that @p dataHandleGenerations contains handle generation counters
         * for all data in layer matching @ref layer() const, where the index
         * is implicitly the handle ID. They're used to decide about data
         * attachment validity, animations with invalid data attachments are
         * then removed. Delegates to @ref clean() and subsequently
         * @ref doClean(), see their documentation for more information.
         */
        void cleanData(const Containers::StridedArrayView1D<const UnsignedShort>& dataHandleGenerations);

        /**
         * @brief Update the internal state and calculate factors for animation advance
         * @param[in]  time     Time to which to calculate the factors
         * @param[out] active   Where to put a mask of active animations
         * @param[out] started  Where to put a mask of animations that started
         *      playing since last time
         * @param[out] stopped  Where to put a mask of animations that stopped
         *      playing since last time
         * @param[out] factors  Where to put animation interpolation factors
         * @param[out] remove   Where to put a mask of animations to remove
         * @return Whether any bits are set in @p active and in @p remove
         *
         * Used by @ref AbstractUserInterface::advanceAnimations() and by
         * @ref AbstractLayer::advanceAnimations() implementations to generate
         * data to subsequently pass to animator implementations. Expects that
         * @p time is greater or equal to @ref time() and size of @p active,
         * @p started, @p stopped, @p factors and @p remove is the same as
         * @ref capacity().
         *
         * The @p active view gets filled with a mask of animations that are
         * @ref AnimationState::Playing at @p time or which changed to
         * @ref AnimationState::Paused or @ref AnimationState::Stopped at
         * @p time compared to @ref time(), @p started gets filled with a mask
         * of animations that were @ref AnimationState::Scheduled at
         * @ref time() and are not at @p time, @p stopped gets filled with a
         * mask of animations that were not @ref AnimationState::Stopped at
         * @ref time() and are at @p time. The @p factors get filled with
         * interpolation factors for active animations and @p remove gets
         * filled with a mask of animations that are
         * @ref AnimationState::Stopped at @p time and don't have
         * @ref AnimationFlag::KeepOncePlayed. See documentation of
         * @ref AnimationState values for how the state transition behaves.
         *
         * In particular, any animation that's @p started or @p stopped is
         * @p active as well. An animation can be both @p started and
         * @p stopped, in which case it played in full between @ref time() and
         * @p time. The @p started bit is only set for animations that begun
         * playing, i.e. animations that transition from
         * @ref AnimationState::Paused to @ref AnimationState::Playing don't
         * have it set. Similarly, @p stopped isn't set for animations that
         * transitioned from @ref AnimationState::Playing to
         * @ref AnimationState::Paused. Animations that are repeated don't get
         * @p started or @p stopped set when they restart. Presence of
         * @ref AnimationFlag::Reverse or
         * @relativeref{AnimationFlag,ReverseEveryOther} doesn't affect the
         * @p started / @p stopped bits in any way, @p started is always set
         * first time the animation is advanced and @p stopped the last time
         * it's advanced when stopping.
         *
         * If the first return value is @cpp true @ce, the @p active,
         * @p started, @p stopped and @p factors views are meant to be passed
         * to subclass implementations such as
         * @ref AbstractGenericAnimator::advance() or
         * @ref AbstractNodeAnimator::advance(), if the second return value is
         * @cpp true @ce, the @p remove view is then meant to be passed to
         * @ref clean().
         *
         * Calling this function updates @ref time() and sets
         * @ref AnimatorState::NeedsAdvance if and only if there are
         * @ref AnimationState::Scheduled, @ref AnimationState::Playing or
         * @ref AnimationState::Paused animations at @p time. However note that
         * behavior of this function is independent of @ref state() --- it
         * performs the update and fills the output views regardless of what
         * flags are set.
         * @see @ref state(AnimationHandle) const
         */
        Containers::Pair<bool, bool> update(Nanoseconds time, Containers::MutableBitArrayView active, Containers::MutableBitArrayView started, Containers::MutableBitArrayView stopped, const Containers::StridedArrayView1D<Float>& factors, Containers::MutableBitArrayView remove);

    protected:
        /**
         * @brief Create an animation
         * @param start         Time at which the animation starts. Use
         *      @ref Nanoseconds::max() for creating a stopped animation.
         * @param duration      Duration of a single play of the animation
         * @param repeatCount   Repeat count. Use @cpp 0 @ce for an
         *      indefinitely repeating animation.
         * @param flags         Flags
         * @return New animation handle
         *
         * Allocates a new handle in a free slot in the internal storage or
         * grows the storage if there's no free slots left. Expects that
         * there's at most 1048576 animations. The returned handle can be
         * removed again with @ref remove().
         *
         * The @p duration is expected to be a non-negative value. If
         * @p duration is zero, @p repeatCount is expected to be @cpp 1 @ce and
         * the animation is advanced at most once before being stopped. The
         * value of @p start, @p duration and @p repeatCount together with
         * @ref time() is then used to decide on a particular
         * @ref AnimationState for given animation; if it results in
         * @ref AnimationState::Scheduled or @ref AnimationState::Playing, the
         * @ref AnimatorState::NeedsAdvance flag is set. The subclass is meant
         * to wrap this function in a public API and perform appropriate
         * initialization work there.
         *
         * If the animator advertises @ref AnimatorFeature::NodeAttachment, the
         * node attachment is set to @ref NodeHandle::Null; if the animator
         * advertises @ref AnimatorFeature::DataAttachment, the data attachment
         * is set to @ref DataHandle::Null. Use
         * @ref create(Nanoseconds, Nanoseconds, NodeHandle, UnsignedInt, AnimationFlags)
         * / @ref create(Nanoseconds, Nanoseconds, DataHandle, UnsignedInt, AnimationFlags)
         * to directly attach to a node / data, or call
         * @ref attach(AnimationHandle, NodeHandle) /
         * @ref attach(AnimationHandle, DataHandle) to attach the animation
         * afterwards.
         */
        AnimationHandle create(Nanoseconds start, Nanoseconds duration, UnsignedInt repeatCount = 1, AnimationFlags flags = {});

        /**
         * @brief Create an animation
         *
         * Same as calling @ref create(Nanoseconds, Nanoseconds, UnsignedInt, AnimationFlags)
         * with @p repeatCount set to @cpp 1 @ce.
         */
        AnimationHandle create(Nanoseconds start, Nanoseconds duration, AnimationFlags flags);

        /**
         * @brief Create an animation attached to a node
         * @param start         Time at which the animation starts. Use
         *      @ref Nanoseconds::max() for creating a stopped animation.
         * @param node          Node the animation is attached to. Use
         *      @ref NodeHandle::Null to create an animation that isn't
         *      attached to any node.
         * @param duration      Duration of a single play of the animation
         * @param repeatCount   Repeat count. Use @cpp 0 @ce for an
         *      indefinitely repeating animation.
         * @param flags         Flags
         * @return New animation handle
         *
         * Expects that the animator supports
         * @ref AnimatorFeature::NodeAttachment. Apart from that behaves the
         * same as @ref create(Nanoseconds, Nanoseconds, UnsignedInt, AnimationFlags),
         * see its documentation for more information. If @p node is not
         * @ref NodeHandle::Null, directly attaches the created animation to
         * given animation, equivalent to calling @ref attach(AnimationHandle, NodeHandle).
         */
        AnimationHandle create(Nanoseconds start, Nanoseconds duration, NodeHandle node, UnsignedInt repeatCount = 1, AnimationFlags flags = {});

        /**
         * @brief Create an animation attached to a node
         *
         * Same as calling @ref create(Nanoseconds, Nanoseconds, NodeHandle, UnsignedInt, AnimationFlags)
         * with @p repeatCount set to @cpp 1 @ce.
         */
        AnimationHandle create(Nanoseconds start, Nanoseconds duration, NodeHandle node, AnimationFlags flags);

        /**
         * @brief Create an animation attached to a data
         * @param start         Time at which the animation starts. Use
         *      @ref Nanoseconds::max() for creating a stopped animation.
         * @param duration      Duration of a single play of the animation
         * @param data          Data the animation is attached to. Use
         *      @ref DataHandle::Null to create an animation that isn't
         *      attached to any data.
         * @param repeatCount   Repeat count. Use @cpp 0 @ce for an
         *      indefinitely repeating animation.
         * @param flags         Flags
         * @return New animation handle
         *
         * Expects that the animator supports
         * @ref AnimatorFeature::DataAttachment, that @ref layer() const isn't
         * @ref LayerHandle::Null and that @p data is either
         * @ref DataHandle::Null or the layer part of it matches
         * @ref layer() const. Apart from that behaves the same as
         * @ref create(Nanoseconds, Nanoseconds, UnsignedInt, AnimationFlags),
         * see its documentation for more information. If @p data is not
         * @ref DataHandle::Null, directly attaches the created animation to
         * given data, equivalent to calling @ref attach(AnimationHandle, DataHandle).
         * @see @ref dataHandleLayer()
         */
        AnimationHandle create(Nanoseconds start, Nanoseconds duration, DataHandle data, UnsignedInt repeatCount = 1, AnimationFlags flags = {});

        /**
         * @brief Create an animation attached to a data
         *
         * Same as calling @ref create(Nanoseconds, Nanoseconds, DataHandle, UnsignedInt, AnimationFlags)
         * with @p repeatCount set to @cpp 1 @ce.
         */
        AnimationHandle create(Nanoseconds start, Nanoseconds duration, DataHandle data, AnimationFlags flags);

        /**
         * @brief Create an animation attached to a data assuming the data belongs to the layer the animator is registered with
         *
         * Compared to @ref create(Nanoseconds, Nanoseconds, DataHandle, UnsignedInt, AnimationFlags)
         * also requires that @ref layer() const isn't @ref LayerHandle::Null
         * but then assumes the @p data is coming from a layer with a handle
         * equal to @ref layer() const. Calling this function with
         * @ref LayerDataHandle::Null is equivalent to calling
         * @ref create(Nanoseconds, Nanoseconds, DataHandle, UnsignedInt, AnimationFlags)
         * with @ref DataHandle::Null.
         */
        AnimationHandle create(Nanoseconds start, Nanoseconds duration, LayerDataHandle data, UnsignedInt repeatCount = 1, AnimationFlags flags = {});

        /**
         * @brief Create an animation attached to a data assuming the data belongs to the layer the animator is registered with
         *
         * Same as calling @ref create(Nanoseconds, Nanoseconds, LayerDataHandle, UnsignedInt, AnimationFlags)
         * with @p repeatCount set to @cpp 1 @ce.
         */
        AnimationHandle create(Nanoseconds start, Nanoseconds duration, LayerDataHandle data, AnimationFlags flags);

        /**
         * @brief Remove an animation
         *
         * Expects that @p handle is valid. After this call,
         * @ref isHandleValid(AnimationHandle) const returns @cpp false @ce for
         * @p handle. See also @ref remove(AnimatorDataHandle) which is a
         * simpler operation if the data is already known to belong to this
         * animator.
         *
         * Compared to @ref create(), @ref play() or @ref pause(), removing an
         * animation never causes any animation to become
         * @ref AnimationState::Scheduled, @ref AnimationState::Playing or
         * @ref AnimationState::Paused if it wasn't before already, thus
         * calling this function doesn't cause @ref AnimatorState::NeedsAdvance
         * to be set. Which means nothing triggers a subsequent @ref clean()
         * --- instead the subclass is meant to wrap this function in a public
         * API and perform appropriate cleanup work on its own data there.
         */
        void remove(AnimationHandle handle);

        /**
         * @brief Remove an animation assuming it belongs to this animator
         *
         * Like @ref remove(AnimationHandle) but without checking that
         * @p handle indeed belongs to this animator. See its documentation for
         * more information.
         * @see @ref animationHandleData()
         */
        void remove(AnimatorDataHandle handle);

    #ifdef DOXYGEN_GENERATING_OUTPUT
    private:
    #else
    protected:
    #endif
        /* Used by AbstractGenericAnimator::setLayer() and
           AbstractLayer::assignAnimator() */
        MAGNUM_UI_LOCAL void setLayerInternal(const AbstractLayer& layer);

    private:
        /* Calls setLayerInternal() (yeah, it's ew, sorry) */
        friend AbstractLayer;

        /**
         * @brief Implementation for @ref features()
         *
         * Note that the value returned by this function is assumed to stay
         * constant during the whole animator lifetime.
         */
        virtual AnimatorFeatures doFeatures() const = 0;

        /**
         * @brief Clean no longer valid animations
         * @param animationIdsToRemove Animation IDs to remove
         *
         * Implementation for @ref clean(), which is called from subclass
         * implementations such as @ref AbstractGenericAnimator::advance(),
         * which is then called from
         * @ref AbstractUserInterface::advanceAnimations() whenever there are
         * any stopped animations that are meant to be removed, i.e. without
         * the @ref AnimationFlag::KeepOncePlayed. If the animator supports
         * @ref AnimatorFeature::NodeAttachment /
         * @relativeref{AnimatorFeature,DataAttachment}, attachments of
         * to-be-removed animations accessible via @ref nodes() /
         * @ref layerData(), are still preserved when this function is called,
         * and set to null only afterwards.
         *
         * This function is also called from @ref cleanNodes() /
         * @ref cleanData(), which is called from
         * @ref AbstractUserInterface::clean() (and transitively from
         * @ref AbstractUserInterface::update()) whenever
         * @ref UserInterfaceState::NeedsNodeClean /
         * @relativeref{UserInterfaceState,NeedsDataClean} or any of the states
         * that imply it are present in @ref AbstractUserInterface::state(). In
         * that case however, node / data attachments of to-be-removed
         * animations are already set to null when this function is called, as
         * it's assumed @ref cleanNodes() / @ref cleanData() is called when
         * the handles are already invalid.
         *
         * The @p animationIdsToRemove view has the same size as
         * @ref capacity() and is guaranteed to have bits set only for valid
         * animation IDs, i.e. animation IDs that are already removed are not
         * set.
         *
         * This function may get also called with @p animationIdsToRemove
         * having all bits zero.
         *
         * Default implementation does nothing.
         */
        virtual void doClean(Containers::BitArrayView animationIdsToRemove);

        /* Common implementations for foo(AnimationHandle) and
           foo(AnimatorDataHandle) */
        MAGNUM_UI_LOCAL void removeInternal(UnsignedInt id);
        MAGNUM_UI_LOCAL void setRepeatCountInternal(UnsignedInt id, UnsignedInt count);
        MAGNUM_UI_LOCAL void setFlagsInternal(UnsignedInt id, AnimationFlags flags);
        MAGNUM_UI_LOCAL void attachInternal(UnsignedInt id, NodeHandle node);
        MAGNUM_UI_LOCAL NodeHandle nodeInternal(UnsignedInt id) const;
        MAGNUM_UI_LOCAL void attachInternal(UnsignedInt id, DataHandle data);
        MAGNUM_UI_LOCAL void attachInternal(UnsignedInt id, LayerDataHandle data);
        MAGNUM_UI_LOCAL DataHandle dataInternal(UnsignedInt id) const;
        MAGNUM_UI_LOCAL Float factorInternal(UnsignedInt id) const;
        MAGNUM_UI_LOCAL void playInternal(UnsignedInt id, Nanoseconds time);
        MAGNUM_UI_LOCAL void resumeInternal(UnsignedInt id, Nanoseconds time);
        MAGNUM_UI_LOCAL void pauseInternal(UnsignedInt id, Nanoseconds time);
        MAGNUM_UI_LOCAL void stopInternal(UnsignedInt id, Nanoseconds time);
        MAGNUM_UI_LOCAL void setFlagsInternal(UnsignedInt id, AnimationFlags flags, Nanoseconds time);

        struct State;
        Containers::Pointer<State> _state;
};

#ifdef DOXYGEN_GENERATING_OUTPUT
/**
@brief Debug layer integration

If an inner type with this name is implemented on an animator that's passed to
@ref DebugLayer::setAnimatorName(const T&, const Containers::StringView&), the
@ref print() function is used by the @ref DebugLayerFlag::NodeHighlight
functionality to provide additional details about all animation attachments
coming from given animator. See @ref Ui-DebugLayer-integration for more
information.
*/
/* While it'd be significantly simpler both for the library and for animators
   to have this as a virtual base class that then gets subclassed with
   interfaces implemented, it's deliberately not done to avoid header
   dependencies as well as make it possible to DCE all debug-layer-related code
   if it isn't used. */
class AbstractAnimator::DebugIntegration {
    public:
        /**
         * @brief Print details about a particular animation
         * @param debug     Debug output where to print
         * @param animator  Animator associated with given @p animation. The
         *      implementation can use either the animator type this class is
         *      part of or any base type.
         * @param animatorName  Animator name that was passed to
         *      @ref DebugLayer::setAnimatorName(const T&, const Containers::StringView&)
         * @param animation Animation to print info about. Guaranteed to be
         *      valid.
         *
         * Used internally by @ref DebugLayer. To fit among other info provided
         * by @ref DebugLayer itself, the implementation is expected to indent
         * the output by at least two spaces and end with a newline (i.e.,
         * @relativeref{Magnum,Debug::newline}).
         */
        void print(Debug& debug, const Animator& animator, Containers::StringView animatorName, AnimatorDataHandle animation);
};
#endif

/**
@brief Base for generic animators
@m_since_latest

@see @ref AbstractUserInterface::setGenericAnimatorInstance()
*/
class MAGNUM_UI_EXPORT AbstractGenericAnimator: public AbstractAnimator {
    public:
        /**
         * @brief Constructor
         * @param handle    Handle returned by
         *      @ref AbstractUserInterface::createAnimator()
         */
        explicit AbstractGenericAnimator(AnimatorHandle handle);

        /** @brief Copying is not allowed */
        AbstractGenericAnimator(const AbstractGenericAnimator&) = delete;

        /** @copydoc AbstractAnimator::AbstractAnimator(AbstractAnimator&&) */
        AbstractGenericAnimator(AbstractGenericAnimator&&) noexcept;

        ~AbstractGenericAnimator();

        /** @brief Copying is not allowed */
        AbstractGenericAnimator& operator=(const AbstractGenericAnimator&) = delete;

        /** @brief Move assignment */
        AbstractGenericAnimator& operator=(AbstractGenericAnimator&&) noexcept;

        /**
         * @brief Advance the animations
         *
         * Used internally from @ref AbstractUserInterface::advanceAnimations().
         * Exposed just for testing purposes, there should be no need to call
         * this function directly and doing so may cause internal
         * @ref AbstractUserInterface state update to misbehave.
         *
         * Expects that size of @p active, @p started, @p stopped and
         * @p factors matches @ref capacity(), it's assumed that their contents
         * were filled by @ref update() before. Delegates to @ref doAdvance(),
         * see its documentation for more information.
         */
        void advance(Containers::BitArrayView active, Containers::BitArrayView started, Containers::BitArrayView stopped, const Containers::StridedArrayView1D<const Float>& factors);

    protected:
        /**
         * @brief Set a layer associated with this animator
         *
         * Expects that the animator supports
         * @ref AnimatorFeature::DataAttachment and that this function hasn't
         * been called yet. Saves @ref AbstractLayer::handle() of @p layer into
         * @ref layer() const, making it possible to call
         * @ref create(Nanoseconds, Nanoseconds, DataHandle, UnsignedInt, AnimationFlags),
         * @ref create(Nanoseconds, Nanoseconds, LayerDataHandle, UnsignedInt, AnimationFlags),
         * @ref attach(AnimationHandle, DataHandle),
         * @ref attach(AnimationHandle, LayerDataHandle),
         * @ref attach(AnimatorDataHandle, DataHandle) and
         * @ref attach(AnimatorDataHandle, LayerDataHandle).
         *
         * A concrete subclass exposing @ref AnimatorFeature::DataAttachment is
         * meant to wrap this function in a public API, optionally restricting
         * to a more concrete layer type, and performing any needed
         * initialization work there, or alternatively taking an appropriately
         * typed layer in a constructor and passing it to this function.
         *
         * A corresponding API for an @ref AbstractDataAnimator /
         * @ref AbstractStyleAnimator is
         * @ref AbstractLayer::assignAnimator(AbstractDataAnimator&) const /
         * @ref AbstractLayer::assignAnimator(AbstractStyleAnimator&) const,
         * where the layer has the control over a concrete animator type
         * instead.
         */
        void setLayer(const AbstractLayer& layer);

    private:
        /**
         * @brief Advance the animations
         * @param active        Animation IDs that are active
         * @param started       Animation IDs that started playing since last
         *      time
         * @param stopped       Animation IDs that stopped playing since last
         *      time
         * @param factors       Interpolation factors indexed by animation ID
         *
         * Implementation for @ref advance(), which is called from
         * @ref AbstractUserInterface::advanceAnimations() whenever
         * @ref AnimatorState::NeedsAdvance is present in @ref state().
         *
         * The @p active, @p started, @p stopped and @p factors views are
         * guaranteed to have the same size as @ref capacity(). The @p factors
         * array is guaranteed to contain values in the @f$ [0, 1] @f$ range
         * for animations that have a corresponding bit set in @p active,
         * calculated equivalently to @ref factor(AnimationHandle) const, and
         * may contain random or uninitialized values for others. This function
         * is always called with at least one @p active bit set.
         */
        virtual void doAdvance(Containers::BitArrayView active, Containers::BitArrayView started, Containers::BitArrayView stopped, const Containers::StridedArrayView1D<const Float>& factors) = 0;
};

/**
@brief Node properties updated by a node animator
@m_since_latest

Depending on which of these are returned from
@ref AbstractNodeAnimator::advance(), causes various @ref UserInterfaceState
flags to be set after an @ref AbstractUserInterface::advanceAnimations() call.
@see @ref NodeAnimatorUpdates
*/
enum class NodeAnimatorUpdate: UnsignedByte {
    /**
     * Node offset or size. Equivalently to calling
     * @ref AbstractUserInterface::setNodeOffset() /
     * @relativeref{AbstractUserInterface,setNodeSize()}, causes
     * @ref UserInterfaceState::NeedsLayoutUpdate to be set.
     */
    OffsetSize = 1 << 0,

    /**
     * Node opacity. Equivalently to calling
     * @ref AbstractUserInterface::setNodeOpacity(), causes
     * @ref UserInterfaceState::NeedsNodeOpacityUpdate to be set.
     */
    Opacity = 1 << 1,

    /**
     * @ref NodeFlag::NoBlur being added or cleared. Equivalently to calling
     * @ref AbstractUserInterface::setNodeFlags() with that flag, causes
     * @ref UserInterfaceState::NeedsNodeEventMaskUpdate to be set. Subset of
     * @ref NodeAnimatorUpdate::Enabled.
     */
    EventMask = 1 << 2,

    /**
     * @ref NodeFlag::NoEvents, @ref NodeFlag::Disabled or
     * @ref NodeFlag::Focusable being added or cleared. Equivalently to calling
     * @ref AbstractUserInterface::setNodeFlags() with these, causes
     * @ref UserInterfaceState::NeedsNodeEnabledUpdate to be set. Superset of
     * @ref NodeAnimatorUpdate::EventMask.
     */
    Enabled = EventMask|(1 << 3),

    /**
     * @ref NodeFlag::Clip being added or cleared. Equivalently to calling
     * @ref AbstractUserInterface::setNodeFlags() with that flag, causes
     * @ref UserInterfaceState::NeedsNodeClipUpdate to be set.
     */
    Clip = 1 << 4,

    /**
     * @ref NodeFlag::Hidden being added or cleared. Equivalently to calling
     * @ref AbstractUserInterface::setNodeFlags() with that flag, causes
     * @ref UserInterfaceState::NeedsNodeUpdate to be set.
     */
    Visibility = 1 << 5,

    /**
     * Scheduling a node for removal. Equivalently to calling
     * @ref AbstractUserInterface::removeNode(), causes
     * @ref UserInterfaceState::NeedsNodeClean to be set.
     */
    Removal = 1 << 6
};

/**
@debugoperatorenum{NodeAnimatorUpdate}
@m_since_latest
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, NodeAnimatorUpdate value);

/**
@brief Set of node properties updated by a node animator
@m_since_latest

@see @ref AbstractNodeAnimator::advance()
*/
typedef Containers::EnumSet<NodeAnimatorUpdate> NodeAnimatorUpdates;

/**
@debugoperatorenum{NodeAnimatorUpdates}
@m_since_latest
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, NodeAnimatorUpdates value);

CORRADE_ENUMSET_OPERATORS(NodeAnimatorUpdates)

/**
@brief Base for node animators
@m_since_latest

@see @ref AbstractUserInterface::setNodeAnimatorInstance()
*/
class MAGNUM_UI_EXPORT AbstractNodeAnimator: public AbstractAnimator {
    public:
        /**
         * @brief Constructor
         * @param handle    Handle returned by
         *      @ref AbstractUserInterface::createAnimator()
         */
        explicit AbstractNodeAnimator(AnimatorHandle handle);

        /** @brief Copying is not allowed */
        AbstractNodeAnimator(const AbstractNodeAnimator&) = delete;

        /** @copydoc AbstractAnimator::AbstractAnimator(AbstractAnimator&&) */
        AbstractNodeAnimator(AbstractNodeAnimator&&) noexcept;

        ~AbstractNodeAnimator();

        /** @brief Copying is not allowed */
        AbstractNodeAnimator& operator=(const AbstractNodeAnimator&) = delete;

        /** @brief Move assignment */
        AbstractNodeAnimator& operator=(AbstractNodeAnimator&&) noexcept;

        /**
         * @brief Advance the animations
         *
         * Used internally from @ref AbstractUserInterface::advanceAnimations().
         * Exposed just for testing purposes, there should be no need to call
         * this function directly and doing so may cause internal
         * @ref AbstractUserInterface state update to misbehave.
         *
         * Expects that size of @p active, @p started, @p stopped and
         * @p factors matches @ref capacity(), it's assumed that their contents
         * were filled by @ref update() before. Expects that @p nodeOffsets,
         * @p nodeSizes, @p nodeOpacities, @p nodeFlags and @p nodesRemove have
         * the same size, the views should be large enough to contain any valid
         * node ID. Delegates to @ref doAdvance(), see its documentation for
         * more information.
         */
        NodeAnimatorUpdates advance(Containers::BitArrayView active, Containers::BitArrayView started, Containers::BitArrayView stopped, const Containers::StridedArrayView1D<const Float>& factors, const Containers::StridedArrayView1D<Vector2>& nodeOffsets, const Containers::StridedArrayView1D<Vector2>& nodeSizes, const Containers::StridedArrayView1D<Float>& nodeOpacities, const Containers::StridedArrayView1D<NodeFlags>& nodeFlags, Containers::MutableBitArrayView nodesRemove);

    protected:
        /**
         * @brief Implementation for @ref features()
         *
         * Exposes @ref AnimatorFeature::NodeAttachment. If a subclass override
         * exposes additional features, it's expected to OR them with this
         * function.
         */
        AnimatorFeatures doFeatures() const override;

    private:
        /**
         * @brief Advance the animations
         * @param[in] active            Animation IDs that are active
         * @param[in] started           Animation IDs that started playing
         *      since last time
         * @param[in] stopped           Animation IDs that stopped playing
         *      since last time
         * @param[in] factors           Interpolation factors indexed by
         *      animation ID
         * @param[in,out] nodeOffsets   Node offsets to animate indexed by node
         *      ID
         * @param[in,out] nodeSizes     Node sizes to animate indexed by node
         *      ID
         * @param[in,out] nodeOpacities Node opacities to animate indexed by
         *      node ID
         * @param[in,out] nodeFlags     Node flags to animate indexed by node
         *      ID
         * @param[out] nodesRemove      Which nodes to remove as a consequence
         *      of the animation
         * @return Node properties that were updated by the animation
         *
         * Implementation for @ref advance(), which is called from
         * @ref AbstractUserInterface::advanceAnimations() whenever
         * @ref AnimatorState::NeedsAdvance is present in @ref state().
         *
         * The @p active, @p started, @p stopped and @p factors views are
         * guaranteed to have the same size as @ref capacity(). The @p factors
         * array is guaranteed to contain values in the @f$ [0, 1] @f$ range
         * for animations that have a corresponding bit set in @p active,
         * calculated equivalently to @ref factor(AnimationHandle) const, and
         * may contain random or uninitialized values for others. This function
         * is always called with at least one @p active bit set.
         *
         * Node handles corresponding to animation IDs are available in
         * @ref nodes(), node IDs can be then extracted from the handles using
         * @ref nodeHandleId(). The node IDs then index into the
         * @p nodeOffsets, @p nodeSizes, @p nodeOpacities, @p nodeFlags and
         * @p nodesRemove views. The @p nodeOffsets, @p nodeSizes,
         * @p nodeOpacities, @p nodeFlags and @p nodesRemove views have the
         * same size and are guaranteed to contain any valid node ID.
         *
         * The @p nodeOffsets, @p nodeSizes, @p nodeOpacities and @p nodeFlags
         * are views directly onto the actual node properties exposed by
         * @ref AbstractUserInterface::nodeOffset(),
         * @relativeref{AbstractUserInterface,nodeSize()},
         * @relativeref{AbstractUserInterface,nodeOpacity()} and
         * @relativeref{AbstractUserInterface,nodeFlags()}. The @p nodesRemove
         * is all zeros initially before getting passed to the first node
         * animator, subsequent animators get it in the state it was left in by
         * the previous. The implementation is expected to only modify values
         * that correspond to node IDs for active animations, returning a set
         * of @ref NodeAnimation values depending on what got changed. The
         * return value is subsequently used to set corresponding
         * @ref UserInterfaceState flags to trigger internal state update and
         * failing to correctly advertise what was modified may lead to strange
         * behavior. On the other hand, conservatively returning more
         * @ref NodeAnimation values than was actually modified may lead to the
         * internal state update doing a lot of otherwise unnecessary work
         * every frame, negatively affecting performance.
         */
        virtual NodeAnimatorUpdates doAdvance(Containers::BitArrayView active, Containers::BitArrayView started, Containers::BitArrayView stopped, const Containers::StridedArrayView1D<const Float>& factors, const Containers::StridedArrayView1D<Vector2>& nodeOffsets, const Containers::StridedArrayView1D<Vector2>& nodeSizes, const Containers::StridedArrayView1D<Float>& nodeOpacities, const Containers::StridedArrayView1D<NodeFlags>& nodeFlags, Containers::MutableBitArrayView nodesRemove) = 0;
};

/**
@brief Base for data animators
@m_since_latest

@see @ref AbstractUserInterface::setDataAnimatorInstance(),
    @ref AbstractLayer::assignAnimator(AbstractDataAnimator&) const,
    @ref AbstractLayer::advanceAnimations(Nanoseconds, Containers::MutableBitArrayView, Containers::MutableBitArrayView, Containers::MutableBitArrayView, const Containers::StridedArrayView1D<Float>&, Containers::MutableBitArrayView, const Containers::Iterable<AbstractDataAnimator>&)
*/
class MAGNUM_UI_EXPORT AbstractDataAnimator: public AbstractAnimator {
    public:
        /**
         * @brief Constructor
         * @param handle    Handle returned by
         *      @ref AbstractUserInterface::createAnimator()
         */
        explicit AbstractDataAnimator(AnimatorHandle handle);

        /** @brief Copying is not allowed */
        AbstractDataAnimator(const AbstractDataAnimator&) = delete;

        /** @copydoc AbstractAnimator::AbstractAnimator(AbstractAnimator&&) */
        AbstractDataAnimator(AbstractDataAnimator&&) noexcept;

        ~AbstractDataAnimator();

        /** @brief Copying is not allowed */
        AbstractDataAnimator& operator=(const AbstractDataAnimator&) = delete;

        /** @brief Move assignment */
        AbstractDataAnimator& operator=(AbstractDataAnimator&&) noexcept;

    protected:
        /**
         * @brief Implementation for @ref features()
         *
         * Exposes @ref AnimatorFeature::DataAttachment. If a subclass override
         * exposes additional features, it's expected to OR them with this
         * function.
         */
        AnimatorFeatures doFeatures() const override;
};

/**
@brief Base for style animators
@m_since_latest

@see @ref AbstractUserInterface::setStyleAnimatorInstance(),
    @ref AbstractLayer::assignAnimator(AbstractStyleAnimator&) const,
    @ref AbstractLayer::advanceAnimations(Nanoseconds, Containers::MutableBitArrayView, Containers::MutableBitArrayView, Containers::MutableBitArrayView, const Containers::StridedArrayView1D<Float>&, Containers::MutableBitArrayView, const Containers::Iterable<AbstractStyleAnimator>&)
*/
class MAGNUM_UI_EXPORT AbstractStyleAnimator: public AbstractAnimator {
    public:
        /**
         * @brief Constructor
         * @param handle    Handle returned by
         *      @ref AbstractUserInterface::createAnimator()
         */
        explicit AbstractStyleAnimator(AnimatorHandle handle);

        /** @brief Copying is not allowed */
        AbstractStyleAnimator(const AbstractStyleAnimator&) = delete;

        /** @copydoc AbstractAnimator::AbstractAnimator(AbstractAnimator&&) */
        AbstractStyleAnimator(AbstractStyleAnimator&&) noexcept;

        ~AbstractStyleAnimator();

        /** @brief Copying is not allowed */
        AbstractStyleAnimator& operator=(const AbstractStyleAnimator&) = delete;

        /** @brief Move assignment */
        AbstractStyleAnimator& operator=(AbstractStyleAnimator&&) noexcept;

    protected:
        /**
         * @brief Implementation for @ref features()
         *
         * Exposes @ref AnimatorFeature::DataAttachment. If a subclass override
         * exposes additional features, it's expected to OR them with this
         * function.
         */
        AnimatorFeatures doFeatures() const override;
};

}}

#endif
