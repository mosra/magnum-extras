#ifndef Magnum_Ui_BaseLayerAnimator_h
#define Magnum_Ui_BaseLayerAnimator_h
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
 * @brief Class @ref Magnum::Ui::BaseLayerStyleAnimator, enum @ref Magnum::Ui::BaseLayerStyleAnimation, enum set @ref Magnum::Ui::BaseLayerStyleAnimations
 * @m_since_latest
 */

#include <Magnum/Math/Time.h>

#include "Magnum/Ui/AbstractVisualLayerAnimator.h"

namespace Magnum { namespace Ui {

/**
@brief Base layer style properties that are being animated
@m_since_latest

Depending on which of these are returned from
@ref BaseLayerStyleAnimator::advance(), causes various @ref LayerState flags
and other internal @ref AbstractLayer state to be set after an
@ref AbstractUserInterface::advanceAnimations() (and transitively
@ref AbstractLayer::advanceAnimations(Nanoseconds, Containers::MutableBitArrayView, const Containers::StridedArrayView1D<Float>&, Containers::MutableBitArrayView, const Containers::Iterable<AbstractStyleAnimator>&))
call.
@see @ref BaseLayerStyleAnimations
*/
enum class BaseLayerStyleAnimation: UnsignedByte {
    /**
     * Style uniform data. Equivalently to calling
     * @ref BaseLayer::setDynamicStyle(), causes
     * @ref LayerState::NeedsCommonDataUpdate to be set.
     */
    Uniform = 1 << 0,

    /**
     * Style padding. Equivalently to calling @ref BaseLayer::setDynamicStyle()
     * with a different padding value, causes @ref LayerState::NeedsDataUpdate
     * to be set.
     */
    Padding = 1 << 1,

    /**
     * Style assignment. Equivalently to calling @ref BaseLayer::setStyle(),
     * causes @ref LayerState::NeedsDataUpdate to be set.
     */
    Style = 1 << 2
};

/**
@debugoperatorenum{BaseLayerStyleAnimation}
@m_since_latest
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, BaseLayerStyleAnimation value);

/**
@brief Set of base layer style properties that are being animated
@m_since_latest

@see @ref BaseLayerStyleAnimator::advance()
*/
typedef Containers::EnumSet<BaseLayerStyleAnimation> BaseLayerStyleAnimations;

/**
@debugoperatorenum{BaseLayerStyleAnimations}
@m_since_latest
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, BaseLayerStyleAnimations value);

CORRADE_ENUMSET_OPERATORS(BaseLayerStyleAnimations)

/**
@brief Base layer style animator
@m_since_latest
*/
class MAGNUM_UI_EXPORT BaseLayerStyleAnimator: public AbstractVisualLayerStyleAnimator {
    public:
        /**
         * @brief Constructor
         * @param handle    Handle returned by
         *      @ref AbstractUserInterface::createAnimator()
         */
        explicit BaseLayerStyleAnimator(AnimatorHandle handle);

        /** @brief Copying is not allowed */
        BaseLayerStyleAnimator(const AbstractStyleAnimator&) = delete;

        /** @copydoc AbstractAnimator::AbstractAnimator(AbstractAnimator&&) */
        BaseLayerStyleAnimator(BaseLayerStyleAnimator&&) noexcept;

        ~BaseLayerStyleAnimator();

        /** @brief Copying is not allowed */
        BaseLayerStyleAnimator& operator=(const BaseLayerStyleAnimator&) = delete;

        /** @brief Move assignment */
        BaseLayerStyleAnimator& operator=(BaseLayerStyleAnimator&&) noexcept;

        /**
         * @brief Create an animation
         * @param sourceStyle   Style index to animate from
         * @param targetStyle   Style index to animate to
         * @param easing        Easing function between @cpp 0.0f @ce and
         *      @cpp 1.0f @ce, used for all style uniform values as well as the
         *      padding. Pick one from @ref Animation::BasicEasing "Animation::Easing"
         *      or supply a custom one.
         * @param played        Time at which the animation is played. Use
         *      @ref Nanoseconds::max() for creating a stopped animation.
         * @param duration      Duration of a single play of the animation
         * @param data          Data the animation is attached to. Use
         *      @ref DataHandle::Null to create an animation that isn't
         *      attached to any data.
         * @param repeatCount   Repeat count. Use @cpp 0 @ce for an
         *      indefinitely repeating animation.
         * @param flags         Flags
         *
         * Expects that @ref BaseLayer::assignAnimator(BaseLayerStyleAnimator&)
         * has been already called for this animator, that both @p sourceStyle
         * and @p targetStyle are less than @ref BaseLayer::Shared::styleCount()
         * (not @ref BaseLayer::Shared::totalStyleCount() --- the style
         * animation is not allowed to use the dynamic style indices) and that
         * @p easing is not @cpp nullptr @ce. Delegates to
         * @ref AbstractAnimator::create(Nanoseconds, Nanoseconds, DataHandle, UnsignedInt, AnimationFlags), see its documentation for more
         * information.
         *
         * The animation affects the @ref BaseLayerStyleUniform and the padding
         * value, if it differs between the styles.
         */
        AnimationHandle create(UnsignedInt sourceStyle, UnsignedInt targetStyle, Float(*easing)(Float), Nanoseconds played, Nanoseconds duration, DataHandle data, UnsignedInt repeatCount = 1, AnimationFlags flags = {});

        /**
         * @brief Create an animation with a style index in a concrete enum type
         *
         * Casts @p sourceStyle and @p targetStyle to
         * @relativeref{Magnum,UnsignedInt} and delegates to
         * @ref create(UnsignedInt, UnsignedInt, Float(*)(Float), Nanoseconds, Nanoseconds, DataHandle, UnsignedInt, AnimationFlags).
         */
        template<class StyleIndex
            #ifndef DOXYGEN_GENERATING_OUTPUT
            , class = typename std::enable_if<std::is_enum<StyleIndex>::value>::type
            #endif
        > AnimationHandle create(StyleIndex sourceStyle, StyleIndex targetStyle, Float(*easing)(Float), Nanoseconds played, Nanoseconds duration, DataHandle data, UnsignedInt repeatCount = 1, AnimationFlags flags = {}) {
            return create(UnsignedInt(sourceStyle), UnsignedInt(targetStyle), easing, played, duration, data, repeatCount, flags);
        }

        /**
         * @brief Create an animation
         *
         * Same as calling @ref create(UnsignedInt, UnsignedInt, Float(*)(Float), Nanoseconds, Nanoseconds, DataHandle, UnsignedInt, AnimationFlags)
         * with @p repeatCount set to @cpp 1 @ce.
         */
        AnimationHandle create(UnsignedInt sourceStyle, UnsignedInt targetStyle, Float(*easing)(Float), Nanoseconds played, Nanoseconds duration, DataHandle data, AnimationFlags flags);

        /**
         * @brief Create an animation with a style index in a concrete enum type
         *
         * Casts @p sourceStyle and @p targetStyle to
         * @relativeref{Magnum,UnsignedInt} and delegates to
         * @ref create(UnsignedInt, UnsignedInt, Float(*)(Float), Nanoseconds, Nanoseconds, DataHandle, AnimationFlags).
         */
        template<class StyleIndex
            #ifndef DOXYGEN_GENERATING_OUTPUT
            , class = typename std::enable_if<std::is_enum<StyleIndex>::value>::type
            #endif
        > AnimationHandle create(StyleIndex sourceStyle, StyleIndex targetStyle, Float(*easing)(Float), Nanoseconds played, Nanoseconds duration, DataHandle data, AnimationFlags flags) {
            return create(UnsignedInt(sourceStyle), UnsignedInt(targetStyle), easing, played, duration, data, flags);
        }

        /**
         * @brief Create an animation assuming the data it's attached to belongs to the layer the animator is registered with
         *
         * Compared to @ref create(UnsignedInt, UnsignedInt, Float(*)(Float), Nanoseconds, Nanoseconds, DataHandle, UnsignedInt, AnimationFlags)
         * delegates to @ref AbstractAnimator::create(Nanoseconds, Nanoseconds, LayerDataHandle, UnsignedInt, AnimationFlags)
         * instead.
         */
        AnimationHandle create(UnsignedInt sourceStyle, UnsignedInt targetStyle, Float(*easing)(Float), Nanoseconds played, Nanoseconds duration, LayerDataHandle data, UnsignedInt repeatCount = 1, AnimationFlags flags = {});

        /**
         * @brief Create an animation with a style index in a concrete enum type assuming the data it's attached to belongs to the layer the animator is registered with
         *
         * Casts @p sourceStyle and @p targetStyle to
         * @relativeref{Magnum,UnsignedInt} and delegates to
         * @ref create(UnsignedInt, UnsignedInt, Float(*)(Float), Nanoseconds, Nanoseconds, LayerDataHandle, UnsignedInt, AnimationFlags).
         */
        template<class StyleIndex
            #ifndef DOXYGEN_GENERATING_OUTPUT
            , class = typename std::enable_if<std::is_enum<StyleIndex>::value>::type
            #endif
        > AnimationHandle create(StyleIndex sourceStyle, StyleIndex targetStyle, Float(*easing)(Float), Nanoseconds played, Nanoseconds duration, LayerDataHandle data, UnsignedInt repeatCount = 1, AnimationFlags flags = {}) {
            return create(UnsignedInt(sourceStyle), UnsignedInt(targetStyle), easing, played, duration, data, repeatCount, flags);
        }

        /**
         * @brief Create an animation assuming the data it's attached to belongs to the layer the animator is registered with
         *
         * Same as calling @ref create(UnsignedInt, UnsignedInt, Float(*)(Float), Nanoseconds, Nanoseconds, LayerDataHandle, UnsignedInt, AnimationFlags)
         * with @p repeatCount set to @cpp 1 @ce.
         */
        AnimationHandle create(UnsignedInt sourceStyle, UnsignedInt targetStyle, Float(*easing)(Float), Nanoseconds played, Nanoseconds duration, LayerDataHandle data, AnimationFlags flags);

        /**
         * @brief Create an animation with a style index in a concrete enum type assuming the data it's attached to belongs to the layer the animator is registered with
         *
         * Casts @p sourceStyle and @p targetStyle to
         * @relativeref{Magnum,UnsignedInt} and delegates to
         * @ref create(UnsignedInt, UnsignedInt, Float(*)(Float), Nanoseconds, Nanoseconds, LayerDataHandle, UnsignedInt, AnimationFlags).
         */
        template<class StyleIndex
            #ifndef DOXYGEN_GENERATING_OUTPUT
            , class = typename std::enable_if<std::is_enum<StyleIndex>::value>::type
            #endif
        > AnimationHandle create(StyleIndex sourceStyle, StyleIndex targetStyle, Float(*easing)(Float), Nanoseconds played, Nanoseconds duration, LayerDataHandle data, AnimationFlags flags) {
            return create(UnsignedInt(sourceStyle), UnsignedInt(targetStyle), easing, played, duration, data, flags);
        }

        /**
         * @brief Remove an animation
         *
         * Expects that @p handle is valid. Recycles a dynamic style used by
         * given animation with @ref BaseLayer::recycleDynamicStyle() and
         * delegates to @ref AbstractAnimator::remove(AnimationHandle), see its
         * documentation for more information.
         *
         * @m_class{m-note m-warning}
         *
         * @par
         *      Note that removing an animation with this function doesn't
         *      cause any change to the style index of a @ref DataHandle it's
         *      attached to, if any. In other words, given data will still keep
         *      the original (dynamic) style index even after it's reused by a
         *      different animation. To fix this, either call
         *      @ref BaseLayer::setStyle() to change the style to a different
         *      one afterwards or @ref stop() the animation instead ---
         *      assuming @ref AnimationFlag::KeepOncePlayed isn't set, it will
         *      cause the animation to gracefully switch to the target style
         *      during the next @ref advance(), and then be removed
         *      automatically.
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
        auto easing(AnimationHandle) const -> Float(*)(Float);

        /**
         * @brief Animation easing function assuming it belongs to this animator
         *
         * Like @ref easing(AnimationHandle) const but without checking that
         * @p handle indeed belongs to this animator. See its documentation for
         * more information.
         * @see @ref animationHandleData()
         */
        auto easing(AnimatorDataHandle) const -> Float(*)(Float);

        /**
         * @brief Animation source and target uniforms
         *
         * Expects that @p handle is valid. The uniforms are queried from
         * @ref BaseLayer::Shared based on style IDs passed to @ref create().
         * @see @ref paddings()
         */
        Containers::Pair<BaseLayerStyleUniform, BaseLayerStyleUniform> uniforms(AnimationHandle handle) const;

        /**
         * @brief Animation source and target uniforms assuming it belongs to this animator
         *
         * Like @ref uniforms(AnimationHandle) const but without checking that
         * @p handle indeed belongs to this animator. See its documentation for
         * more information.
         * @see @ref animationHandleData()
         */
        Containers::Pair<BaseLayerStyleUniform, BaseLayerStyleUniform> uniforms(AnimatorDataHandle handle) const;

        /**
         * @brief Animation source and target paddings
         *
         * Expects that @p handle is valid. The paddings are queried from
         * @ref BaseLayer::Shared based on style IDs passed to @ref create().
         * @see @ref uniforms()
         */
        Containers::Pair<Vector4, Vector4> paddings(AnimationHandle handle) const;

        /**
         * @brief Animation source and target paddings assuming it belongs to this animator
         *
         * Like @ref paddings(AnimationHandle) const but without checking that
         * @p handle indeed belongs to this animator. See its documentation for
         * more information.
         * @see @ref animationHandleData()
         */
        Containers::Pair<Vector4, Vector4> paddings(AnimatorDataHandle handle) const;

        /**
         * @brief Advance the animations
         * @param[in] active                    Animation IDs that are active
         * @param[in] factors                   Interpolation factors indexed
         *      by animation ID
         * @param[in] remove                    Animation IDs to be removed
         * @param[in,out] dynamicStyleUniforms  Uniforms to animate indexed by
         *      dynamic style ID
         * @param[in,out] dynamicStylePaddings  Paddings to animate indexed by
         *      dynamic style ID
         * @param[in,out] dataStyles            Style assignments of all layer
         *      data indexed by data ID
         * @return Style properties that were affected by the animation
         *
         * Used internally from @ref BaseLayer::advanceAnimations(Nanoseconds, Containers::MutableBitArrayView, const Containers::StridedArrayView1D<Float>&, Containers::MutableBitArrayView, const Containers::Iterable<AbstractStyleAnimator>&),
         * which is called from @ref AbstractUserInterface::advanceAnimations().
         * Exposed just for testing purposes, there should be no need to call
         * this function directly and doing so may cause internal
         * @ref AbstractUserInterface state update to misbehave.
         *
         * Expects that size of @p active, @p factors and @p remove matches
         * @ref capacity(), it's assumed that their contents were filled by
         * @ref update() before. Expects that @p dynamicStyleUniforms and
         * @p dynamicStylePaddings have the same size, the views should be
         * large enough to contain any valid dynamic style ID. The
         * @p dataStyles view should be large enough to contain any valid layer
         * data ID.
         */
        BaseLayerStyleAnimations advance(Containers::BitArrayView active, const Containers::StridedArrayView1D<const Float>& factors, Containers::BitArrayView remove, Containers::ArrayView<BaseLayerStyleUniform> dynamicStyleUniforms, const Containers::StridedArrayView1D<Vector4>& dynamicStylePaddings, const Containers::StridedArrayView1D<UnsignedInt>& dataStyles);

    private:
        struct State;

        MAGNUM_UI_LOCAL void createInternal(AnimationHandle handle, UnsignedInt sourceStyle, UnsignedInt targetStyle, Float(*easing)(Float));
};

}}

#endif
