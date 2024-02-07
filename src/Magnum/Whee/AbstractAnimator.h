#ifndef Magnum_Whee_AbstractAnimator_h
#define Magnum_Whee_AbstractAnimator_h
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

/** @file
 * @brief Class @ref Magnum::Whee::AbstractAnimator, @ref Magnum::Whee::AbstractGenericAnimator, enum @ref Magnum::Whee::AnimatorFeature, @ref Magnum::Whee::AnimatorState, @ref Magnum::Whee::AnimationFlag, @ref Magnum::Whee::AnimationState, enum set @ref Magnum::Whee::AnimatorFeatures, @ref Magnum::Whee::AnimatorStates, @ref Magnum::Whee::AnimationFlags
 * @m_since_latest
 */

#include <Corrade/Containers/Pointer.h>

#include "Magnum/Whee/Whee.h"
#include "Magnum/Whee/visibility.h"

namespace Magnum { namespace Whee {

/**
@brief Features supported by an animator
@m_since_latest

@see @ref AnimatorFeatures, @ref AbstractAnimator::features()
*/
enum class AnimatorFeature: UnsignedByte {
};

/**
@debugoperatorenum{AnimatorFeature}
@m_since_latest
*/
MAGNUM_WHEE_EXPORT Debug& operator<<(Debug& debug, AnimatorFeature value);

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
MAGNUM_WHEE_EXPORT Debug& operator<<(Debug& debug, AnimatorFeatures value);

CORRADE_ENUMSET_OPERATORS(AnimatorFeatures)

/**
@brief Animator state
@m_since_latest

Used to decide whether @ref AbstractAnimator::advance() (called from
@ref AbstractUserInterface::advanceAnimations()) needs to be called. See
@ref UserInterfaceState for interface-wide state.
@see @ref AnimatorStates, @ref AbstractAnimator::state() const
*/
enum class AnimatorState: UnsignedByte {
    /**
     * @ref AbstractAnimator::advance() (which is called from
     * @ref AbstractUserInterface::advanceAnimations()) needs to be called to
     * advance active animations. Set implicitly after
     * @ref AbstractAnimator::create(), @relativeref{AbstractAnimator,play()},
     * @relativeref{AbstractAnimator,pause()} and
     * @relativeref{AbstractAnimator,advance()} that results in at least one
     * animation being @ref AnimationState::Scheduled,
     * @ref AnimationState::Playing or @ref AnimationState::Paused, is reset
     * once @relativeref{AbstractAnimator,advance()} results in no animation
     * being in that state anymore.
     */
    NeedsAdvance = 1 << 0
};

/**
@debugoperatorenum{AnimatorState}
@m_since_latest
*/
MAGNUM_WHEE_EXPORT Debug& operator<<(Debug& debug, AnimatorState value);

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
MAGNUM_WHEE_EXPORT Debug& operator<<(Debug& debug, AnimatorStates value);

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
     * @ref AbstractAnimator::advance() schedules all animations that reached
     * @ref AnimationState::Stopped for removal in a subsequent
     * @ref AbstractAnimator::clean() call. With this flag the animation is
     * kept and is only removable directly with
     * @ref AbstractAnimator::remove().
     */
    KeepOncePlayed = 1 << 0
};

/**
@debugoperatorenum{AnimationFlag}
@m_since_latest
*/
MAGNUM_WHEE_EXPORT Debug& operator<<(Debug& debug, AnimationFlag value);

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
MAGNUM_WHEE_EXPORT Debug& operator<<(Debug& debug, AnimationFlags value);

CORRADE_ENUMSET_OPERATORS(AnimationFlags)

/**
@brief Animation state
@m_since_latest

@see @ref AbstractAnimator::state()
*/
enum class AnimationState: UnsignedByte {
    /**
     * The animation is scheduled to be played. Returned if
     * @ref AbstractAnimator::stopped() is greater than
     * @ref AbstractAnimator::played() for given animation and the time the
     * animation is played at is greater than current
     * @ref AbstractAnimator::time(). Can transition to
     * @ref AnimationState::Playing, @ref AnimationState::Paused or
     * @ref AnimationState::Stopped after the next
     * @ref AbstractAnimator::advance().
     */
    Scheduled,

    /**
     * The animation is currently playing. Returned if
     * @ref AbstractAnimator::stopped() is greater than
     * @ref AbstractAnimator::played() for given animation and than current
     * @ref AbstractAnimator::time(), the time the animation is played at is
     * less than or equal to current time, either
     * @ref AbstractAnimator::repeatCount() for given animation is @cpp 0 @ce
     * or @cpp played + duration*repeatCount > time @ce, where `duration` is
     * @ref AbstractAnimator::duration() for given animation, and
     * @ref AbstractAnimator::paused() for given animation is greater than
     * current time. Can transition to @ref AnimationState::Paused or
     * @ref AnimationState::Stopped after the next
     * @ref AbstractAnimator::advance().
     */
    Playing,

    /**
     * The animation is currently paused. Returned if
     * @ref AbstractAnimator::stopped() is greater than
     * @ref AbstractAnimator::played() for given animation and than current
     * @ref AbstractAnimator::time(), the time the animation is played at is
     * less than or equal to current time, either
     * @ref AbstractAnimator::repeatCount() for given animation is @cpp 0 @ce
     * or @cpp played + duration*repeatCount > time @ce, where `duration` is
     * @ref AbstractAnimator::duration() for given animation, and
     * @ref AbstractAnimator::paused() for given animation is less than or
     * equal to current time. Can transition to @ref AnimationState::Playing or
     * @ref AnimationState::Stopped after the next
     * @ref AbstractAnimator::advance().
     */
    Paused,

    /**
     * The animation is currently stopped. Returned if
     * @ref AbstractAnimator::stopped() is less than or equal to
     * @ref AbstractAnimator::played() for given animation, if the stopped
     * time is less than or equial to current @ref AbstractAnimator::time() or
     * if @ref AbstractAnimator::played() for given animation is less than or
     * equal to current time, @ref AbstractAnimator::repeatCount() for given
     * animation is non-zero and @cpp played + duration*repeatCount <= time @ce,
     * where `duration` is @ref AbstractAnimator::duration() for given
     * animation.
     *
     * Note that @ref AbstractAnimator::advance() automatically schedules
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
MAGNUM_WHEE_EXPORT Debug& operator<<(Debug& debug, AnimationState value);

/**
@brief Base for animators
@m_since_latest
*/
class MAGNUM_WHEE_EXPORT AbstractAnimator {
    public:
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
        AnimatorFeatures features() const { return doFeatures(); }

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
         * Time value last passed to @ref advance(). Initial value is
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
         * @ref create() and is always positive. The duration, together with
         * @ref played(AnimationHandle) const,
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
         * @ref played(AnimationHandle) const,
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
         * repeating animation. The repeat count, together with
         * @ref duration(AnimationHandle) const,
         * @ref played(AnimationHandle) const,
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
         * @brief Set animation flags
         *
         * Expects that @p handle is valid. See also
         * @ref setFlags(AnimatorDataHandle, AnimationFlags) which is a simpler
         * operation if the animation is already known to belong to this
         * animator.
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
         * @brief Time at which an animation is played
         *
         * Expects that @p handle is valid. The time an animation is played at
         * is specified with @ref create() and is subsequently affected by
         * calling @ref play() or @ref stop(). The specified time, together
         * with @ref duration(AnimationHandle) const,
         * @ref repeatCount(AnimationHandle) const,
         * @ref paused(AnimationHandle) const,
         * @ref stopped(AnimationHandle) const and @ref time() is used to
         * decide on a particular @ref AnimationState for given animation. See
         * also @ref played(AnimatorDataHandle) const which is a simpler
         * operation if the animation is already known to belong to this
         * animator.
         * @see @ref isHandleValid(AnimationHandle) const,
         *      @ref state(AnimationHandle) const
         */
        Nanoseconds played(AnimationHandle handle) const;

        /**
         * @brief Time at which an animation is played assuming it belongs to this animator
         *
         * Like @ref played(AnimationHandle) const but without checking that
         * @p handle indeed belongs to this animator. See its documentation for
         * more information.
         * @see @ref animationHandleData()
         */
        Nanoseconds played(AnimatorDataHandle handle) const;

        /**
         * @brief Animation paused time
         *
         * Expects that @p handle is valid. The paused time is initially
         * @ref Nanoseconds::max() and is affected by calling @ref play() or
         * @ref pause(). The paused time, together with
         * @ref duration(AnimationHandle) const,
         * @ref repeatCount(AnimationHandle) const,
         * @ref played(AnimationHandle) const,
         * @ref stopped(AnimationHandle) const and @ref time() is used to
         * decide on a particular @ref AnimationState for given animation. See
         * also @ref paused(AnimatorDataHandle) const which is a simpler
         * operation if the animation is already known to belong to this
         * animator.
         * @see @ref isHandleValid(AnimationHandle) const,
         *      @ref state(AnimationHandle) const
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
         * @ref played(AnimationHandle) const,
         * @ref paused(AnimationHandle) const,
         * @ref duration(AnimationHandle) const,
         * @ref repeatCount(AnimationHandle) const and @ref time() is used to
         * decide on a particular @ref AnimationState for given animation. See
         * also @ref stopped(AnimatorDataHandle) const which is a simpler
         * operation if the animation is already known to belong to this
         * animator.
         * @see @ref isHandleValid(AnimationHandle) const,
         *      @ref state(AnimationHandle) const
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
         * @brief Animation state
         *
         * Expects that @p handle is valid. Calculated based on the value of
         * @ref time() recorded in last @ref advance(),
         * @ref duration(AnimationHandle) const,
         * @ref repeatCount(AnimationHandle) const,
         * @ref played(AnimationHandle) const,
         * @ref paused(AnimationHandle) const and
         * @ref stopped(AnimationHandle) const for a particular animation. See
         * also @ref state(AnimatorDataHandle) const which is a simpler
         * operation if the animation is already known to belong to this
         * animator.
         * @see @ref isHandleValid(AnimationHandle) const, @ref factor()
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
         * @brief Animation interpolation factor
         *
         * Expects that @p handle is valid. Calculated based on the value of
         * @ref time() recorded in last @ref advance(),
         * @ref duration(AnimationHandle) const,
         * @ref repeatCount(AnimationHandle) const,
         * @ref played(AnimationHandle) const,
         * @ref paused(AnimationHandle) const and
         * @ref stopped(AnimationHandle) const for a particular animation. The
         * returned value is always in the @f$ [0, 1] @f$ range and matches
         * what would be returned from @ref advance() for given animation at
         * @ref time(). For @ref AnimationState::Scheduled always returns
         * @cpp 0.0f @ce, for @ref AnimationState::Stopped @cpp 1.0f @ce.
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
         * @brief Play an animation or resume a paused one
         *
         * Expects that @p handle is valid. The function effectively just
         * updates the value of @ref played(AnimationHandle) const and sets
         * both @ref paused(AnimationHandle) const and
         * @ref stopped(AnimationHandle) const to @ref Nanoseconds::max(). The
         * actual @ref AnimationState is then decided based on these three
         * times together with @ref duration(AnimationHandle) const,
         * @ref repeatCount(AnimationHandle) const and @ref time().
         *
         * If calling this function resulted in the animation being
         * @ref AnimationState::Scheduled or @ref AnimationState::Playing, the
         * @ref AnimatorState::NeedsAdvance flag is set.
         * @see @ref isHandleValid(AnimationHandle) const,
         *      @ref state(AnimationHandle) const
         */
        void play(AnimationHandle handle, Nanoseconds time);

        /**
         * @brief Play an animation or resume a paused one assuming it belongs to this animator
         *
         * Like @ref play(AnimationHandle, Nanoseconds) but without checking
         * that @p handle indeed belongs to this animator. See its
         * documentation for more information.
         * @see @ref animationHandleData()
         */
        void play(AnimatorDataHandle handle, Nanoseconds time);

        /**
         * @brief Pause an animation
         *
         * Expects that @p handle is valid. The function effectively just
         * updates the value of @ref paused(AnimationHandle) const; the actual
         * @ref AnimationState is then decided based on the paused time
         * together with @ref duration(AnimationHandle) const,
         * @ref repeatCount(AnimationHandle) const,
         * @ref played(AnimationHandle) const,
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
         * @ref played(AnimationHandle) const,
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
         * @brief Advance the animations
         * @param[in]  time     Time to which to advance
         * @param[out] active   Where to put a mask of active animations
         * @param[out] factors  Where to put animation interpolation factors
         * @param[out] remove   Where to put a mask of animations to remove
         * @return Whether any bits are set in @p active and in @p remove
         *
         * Used internally from subclass implementations such as
         * @ref AbstractGenericAnimator::advance(), which is then called from
         * @ref AbstractUserInterface::advanceAnimations(). Exposed just for
         * testing purposes, there should be no need to call this function
         * directly and doing so may cause internal @ref AbstractUserInterface
         * state update to misbehave. Expects that @p time is greater or equal
         * to @ref time(), size of @p active, @p factors and @p remove is
         * the same as @ref capacity(), and that the @p active and @p remove
         * views are zero-initialized.
         *
         * The @p active view gets filled with a mask of animations that are
         * @ref AnimationState::Playing at @p time or which changed to
         * @ref AnimationState::Paused or @ref AnimationState::Stopped at
         * @p time compared to @ref time(), @p factors get filled with
         * interpolation factors for active animations and @p remove gets
         * filled with a mask of animations that are
         * @ref AnimationState::Stopped at @p time and don't have
         * @ref AnimationFlag::KeepOncePlayed. See documentation of
         * @ref AnimationState values for how the state transition behaves.
         *
         * If the first return value is @cpp true @ce, the @p active and
         * @p factors views are meant to be passed to subclass implementations
         * such as @ref AbstractGenericAnimator::doAdvance(), if the second
         * return value is @cpp true @ce, the @p remove view is then meant to
         * be passed to @ref clean().
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
        Containers::Pair<bool, bool> advance(Nanoseconds time, Containers::MutableBitArrayView active, const Containers::StridedArrayView1D<Float>& factors, Containers::MutableBitArrayView remove);

    protected:
        /**
         * @brief Create an animation
         * @param played        Time at which the animation is played. Use
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
         * The @p duration is expected to be a positive value. The value of
         * @p played, @p duration and @p repeatCount together with @ref time()
         * is then used to decide on a particular @ref AnimationState for given
         * animation; if it results in @ref AnimationState::Scheduled or
         * @ref AnimationState::Playing, the @ref AnimatorState::NeedsAdvance
         * flag is set. The subclass is meant to wrap this function in a public
         * API and perform appropriate initialization work there.
         */
        AnimationHandle create(Nanoseconds played, Nanoseconds duration, UnsignedInt repeatCount = 1, AnimationFlags flags = {});

        /**
         * @brief Create an animation
         *
         * Same as calling @ref create(Nanoseconds, Nanoseconds, UnsignedInt, AnimationFlags)
         * with @p repeatCount set to @cpp 1 @ce.
         */
        AnimationHandle create(Nanoseconds played, Nanoseconds duration, AnimationFlags flags);

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

    private:
        /** @brief Implementation for @ref features() */
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
         * the @ref AnimationFlag::KeepOncePlayed.
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
        MAGNUM_WHEE_LOCAL void removeInternal(UnsignedInt id);
        MAGNUM_WHEE_LOCAL void setFlagsInternal(UnsignedInt id, AnimationFlags flags);
        MAGNUM_WHEE_LOCAL Float factorInternal(UnsignedInt id) const;
        MAGNUM_WHEE_LOCAL void playInternal(UnsignedInt id, Nanoseconds time);
        MAGNUM_WHEE_LOCAL void pauseInternal(UnsignedInt id, Nanoseconds time);
        MAGNUM_WHEE_LOCAL void stopInternal(UnsignedInt id, Nanoseconds time);

        struct State;
        Containers::Pointer<State> _state;
};

/**
@brief Base for generic animators
@m_since_latest

@see @ref AbstractUserInterface::setGenericAnimatorInstance()
*/
class MAGNUM_WHEE_EXPORT AbstractGenericAnimator: public AbstractAnimator {
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
         * @ref AbstractUserInterface state update to misbehave. Delegates into
         * @ref AbstractAnimator::advance() and subsequently to
         * @ref doAdvance() and @ref clean(), see their documentation for more
         * information.
         */
        void advance(Nanoseconds time);

    private:
        /**
         * @brief Advance the animations
         * @param active        Animation IDs that are active
         * @param factors       Interpolation factors indexed by animation ID
         *
         * Implementation for @ref advance(), which is called from
         * @ref AbstractUserInterface::advanceAnimations() whenever
         * @ref AnimatorState::NeedsAdvance is present in @ref state().
         *
         * The @p active and @p factors views are guaranteed to have the same
         * size as @ref capacity(). The @p factors array is guaranteed to
         * contain values in the @f$ [0, 1] @f$ range for animations that have
         * a corresponding bit set in @p active, calculated equivalently to
         * @ref factor(AnimationHandle) const, and may contain random or
         * uninitialized values for others. This function is always called with
         * at least one @p active bit set.
         */
        virtual void doAdvance(Containers::BitArrayView active, const Containers::StridedArrayView1D<const Float>& factors) = 0;
};

}}

#endif
