#ifndef Magnum_Ui_BaseLayerAnimator_h
#define Magnum_Ui_BaseLayerAnimator_h
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
 * @brief Class @ref Magnum::Ui::BaseLayerStyleAnimator, enum @ref Magnum::Ui::BaseLayerStyleAnimatorUpdate, enum set @ref Magnum::Ui::BaseLayerStyleAnimatorUpdates
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
@ref AbstractLayer::advanceAnimations(Nanoseconds, Containers::MutableBitArrayView, Containers::MutableBitArrayView, Containers::MutableBitArrayView, const Containers::StridedArrayView1D<Float>&, Containers::MutableBitArrayView, const Containers::Iterable<AbstractStyleAnimator>&))
call.
@see @ref BaseLayerStyleAnimatorUpdates
*/
enum class BaseLayerStyleAnimatorUpdate: UnsignedByte {
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
@debugoperatorenum{BaseLayerStyleAnimatorUpdate}
@m_since_latest
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, BaseLayerStyleAnimatorUpdate value);

/**
@brief Set of base layer style properties that are being animated
@m_since_latest

@see @ref BaseLayerStyleAnimator::advance()
*/
typedef Containers::EnumSet<BaseLayerStyleAnimatorUpdate> BaseLayerStyleAnimatorUpdates;

/**
@debugoperatorenum{BaseLayerStyleAnimatorUpdates}
@m_since_latest
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, BaseLayerStyleAnimatorUpdates value);

CORRADE_ENUMSET_OPERATORS(BaseLayerStyleAnimatorUpdates)

/**
@brief Base layer style animator
@m_since_latest

Each animation is a transition between two @ref BaseLayer styles, with
individual properties interpolated with an easing function.
@ref TextLayerStyleAnimator is a matching animator for the @ref TextLayer.

@section Ui-BaseLayerStyleAnimator-setup Setting up an animator instance

The animator doesn't have any shared state or configuration, so it's just about
constructing it from a fresh @ref AbstractUserInterface::createAnimator()
handle and passing it to @relativeref{AbstractUserInterface,setStyleAnimatorInstance()}.

@snippet Ui.cpp BaseLayerStyleAnimator-setup1

After that, the animator has to be registered with a concrete layer instance.
The animations make use of dynamic styles, so the base layer is expected to
have at least one dynamic style enabled with
@ref BaseLayer::Shared::Configuration::setDynamicStyleCount(). The more dynamic
styles are enabled, the more style animations can be running for given layer at
the same time, but also more data need to get uploaded to the GPU every frame.
Finally, call @ref BaseLayer::assignAnimator(BaseLayerStyleAnimator&) to assign
the animator to the layer instance. Then, assuming
@ref AbstractUserInterface::advanceAnimations() is called in an appropriate
place, the animator is ready to use.

@snippet Ui-gl.cpp BaseLayerStyleAnimator-setup2

Unlike builtin layers or layouters, the default @ref UserInterface
implementation doesn't implicitly provide a @ref BaseLayerStyleAnimator
instance.

@todoc setDefaultStyleAnimator(), once it's actually used by styles; then also
    update the sentence about UserInterface not having any animator instance

@section Ui-BaseLayerStyleAnimator-create Creating animations

Assuming an enum is used to index the styles defined in @ref BaseLayer::Shared
of the associated layer instance, an animation is created by calling
@ref create() with the source and target style indices, an easing function from
@ref Animation::BasicEasing "Animation::Easing" or a custom one, time at which
it's meant to start, its duration, and a @ref DataHandle which the style
animation should affect. Here, for example, to fade out a button hover style
over half a second:

@snippet Ui.cpp BaseLayerStyleAnimator-create

Internally, once the animation starts playing, the animator allocates a new
dynamic style index using @ref BaseLayer::allocateDynamicStyle() and switches
the style index of given @ref DataHandle to the allocated dynamic style with
@ref BaseLayer::setStyle(). During the animation the style data are updated to
corresponding interpolation between the source and target styles, equivalent to
calling @ref BaseLayer::setDynamicStyle(). When the animation stops, the data
style index is switched to the target ID specified in @ref create() and the
dynamic style index is recycled with @ref BaseLayer::recycleDynamicStyle() is
called for the dynamic style.

If the animator runs out of dynamic styles, newly started animations are left
at the source style index until another dynamic style is recycled. If no
dynamic style gets recycled until the animation ends, the data gets switched
directly to the target style without animating.

The animation interpolates all properties of @ref BaseLayerStyleUniform
including outline width and corner radius, as well as the style padding value.
At the moment, only animation between predefined styles is possible.

@section Ui-BaseLayerStyleAnimator-lifetime Animation lifetime and data attachment

As with all other animations, they're implicitly removed once they're played.
Pass @ref AnimationFlag::KeepOncePlayed to @ref create() or @ref addFlags() to
disable this behavior.

Style animations are associated with data they animate, and thus as soon as the
data or node the data is attached to is removed, the animation gets removed as
well. If you want to preserve the animation when the data is removed, call
@ref attach(AnimationHandle, DataHandle) with @ref DataHandle::Null to detach
it from the data before removing.

If you call @ref create() with @ref DataHandle::Null, the animation will still
allocate and interpolate a dynamic style, but the style won't be used anywhere.
You can then retrieve the dynamic style index with @ref dynamicStyle() and use
it for example to make the same style animation on multiple different data.
Note that in this case you're also responsible also for switching to the target
style once the animation finishes --- once the dynamic style is recycled, the
same index may get used for arbitrary other style either by this animator or
any other code, causing visual bugs.

@todoc update this section once there's robustness against switching styles
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
         * Expects that @ref BaseLayer::assignAnimator(BaseLayerStyleAnimator&)
         * has been already called for this animator, that both @p sourceStyle
         * and @p targetStyle are less than @ref BaseLayer::Shared::styleCount()
         * (not @ref BaseLayer::Shared::totalStyleCount() --- the style
         * animation is not allowed to use the dynamic style indices) and that
         * @p easing is not @cpp nullptr @ce. Delegates to
         * @ref AbstractAnimator::create(Nanoseconds, Nanoseconds, DataHandle, UnsignedInt, AnimationFlags),
         * see its documentation for more information.
         *
         * The animation affects the @ref BaseLayerStyleUniform and the padding
         * value, if it differs between the styles.
         */
        AnimationHandle create(UnsignedInt sourceStyle, UnsignedInt targetStyle, Float(*easing)(Float), Nanoseconds start, Nanoseconds duration, DataHandle data, UnsignedInt repeatCount = 1, AnimationFlags flags = {});

        /**
         * @brief Create an animation with a style index in a concrete enum type
         *
         * Casts @p sourceStyle and @p targetStyle to
         * @relativeref{Magnum,UnsignedInt} and delegates to
         * @ref create() "create(UnsignedInt, UnsignedInt, Float(*)(Float), Nanoseconds, Nanoseconds, DataHandle, UnsignedInt, AnimationFlags)".
         * @todoc fix the overload references once Doxygen can link to
         *      functions taking function pointers
         */
        template<class StyleIndex
            #ifndef DOXYGEN_GENERATING_OUTPUT
            , typename std::enable_if<std::is_enum<StyleIndex>::value, int>::type = 0
            #endif
        > AnimationHandle create(StyleIndex sourceStyle, StyleIndex targetStyle, Float(*easing)(Float), Nanoseconds start, Nanoseconds duration, DataHandle data, UnsignedInt repeatCount = 1, AnimationFlags flags = {}) {
            return create(UnsignedInt(sourceStyle), UnsignedInt(targetStyle), easing, start, duration, data, repeatCount, flags);
        }

        /**
         * @brief Create an animation
         *
         * Same as calling @ref create() "create(UnsignedInt, UnsignedInt, Float(*)(Float), Nanoseconds, Nanoseconds, DataHandle, UnsignedInt, AnimationFlags)"
         * with @p repeatCount set to @cpp 1 @ce.
         */
        AnimationHandle create(UnsignedInt sourceStyle, UnsignedInt targetStyle, Float(*easing)(Float), Nanoseconds start, Nanoseconds duration, DataHandle data, AnimationFlags flags);

        /**
         * @brief Create an animation with a style index in a concrete enum type
         *
         * Casts @p sourceStyle and @p targetStyle to
         * @relativeref{Magnum,UnsignedInt} and delegates to
         * @ref create() "create(UnsignedInt, UnsignedInt, Float(*)(Float), Nanoseconds, Nanoseconds, DataHandle, AnimationFlags)".
         */
        template<class StyleIndex
            #ifndef DOXYGEN_GENERATING_OUTPUT
            , typename std::enable_if<std::is_enum<StyleIndex>::value, int>::type = 0
            #endif
        > AnimationHandle create(StyleIndex sourceStyle, StyleIndex targetStyle, Float(*easing)(Float), Nanoseconds start, Nanoseconds duration, DataHandle data, AnimationFlags flags) {
            return create(UnsignedInt(sourceStyle), UnsignedInt(targetStyle), easing, start, duration, data, flags);
        }

        /**
         * @brief Create an animation assuming the data it's attached to belongs to the layer the animator is registered with
         *
         * Compared to @ref create() "create(UnsignedInt, UnsignedInt, Float(*)(Float), Nanoseconds, Nanoseconds, DataHandle, UnsignedInt, AnimationFlags)"
         * delegates to @ref AbstractAnimator::create(Nanoseconds, Nanoseconds, LayerDataHandle, UnsignedInt, AnimationFlags)
         * instead.
         */
        AnimationHandle create(UnsignedInt sourceStyle, UnsignedInt targetStyle, Float(*easing)(Float), Nanoseconds start, Nanoseconds duration, LayerDataHandle data, UnsignedInt repeatCount = 1, AnimationFlags flags = {});

        /**
         * @brief Create an animation with a style index in a concrete enum type assuming the data it's attached to belongs to the layer the animator is registered with
         *
         * Casts @p sourceStyle and @p targetStyle to
         * @relativeref{Magnum,UnsignedInt} and delegates to
         * @ref create() "create(UnsignedInt, UnsignedInt, Float(*)(Float), Nanoseconds, Nanoseconds, LayerDataHandle, UnsignedInt, AnimationFlags)".
         */
        template<class StyleIndex
            #ifndef DOXYGEN_GENERATING_OUTPUT
            , typename std::enable_if<std::is_enum<StyleIndex>::value, int>::type = 0
            #endif
        > AnimationHandle create(StyleIndex sourceStyle, StyleIndex targetStyle, Float(*easing)(Float), Nanoseconds start, Nanoseconds duration, LayerDataHandle data, UnsignedInt repeatCount = 1, AnimationFlags flags = {}) {
            return create(UnsignedInt(sourceStyle), UnsignedInt(targetStyle), easing, start, duration, data, repeatCount, flags);
        }

        /**
         * @brief Create an animation assuming the data it's attached to belongs to the layer the animator is registered with
         *
         * Same as calling @ref create() "create(UnsignedInt, UnsignedInt, Float(*)(Float), Nanoseconds, Nanoseconds, LayerDataHandle, UnsignedInt, AnimationFlags)"
         * with @p repeatCount set to @cpp 1 @ce.
         */
        AnimationHandle create(UnsignedInt sourceStyle, UnsignedInt targetStyle, Float(*easing)(Float), Nanoseconds start, Nanoseconds duration, LayerDataHandle data, AnimationFlags flags);

        /**
         * @brief Create an animation with a style index in a concrete enum type assuming the data it's attached to belongs to the layer the animator is registered with
         *
         * Casts @p sourceStyle and @p targetStyle to
         * @relativeref{Magnum,UnsignedInt} and delegates to
         * @ref create() "create(UnsignedInt, UnsignedInt, Float(*)(Float), Nanoseconds, Nanoseconds, LayerDataHandle, UnsignedInt, AnimationFlags)".
         */
        template<class StyleIndex
            #ifndef DOXYGEN_GENERATING_OUTPUT
            , typename std::enable_if<std::is_enum<StyleIndex>::value, int>::type = 0
            #endif
        > AnimationHandle create(StyleIndex sourceStyle, StyleIndex targetStyle, Float(*easing)(Float), Nanoseconds start, Nanoseconds duration, LayerDataHandle data, AnimationFlags flags) {
            return create(UnsignedInt(sourceStyle), UnsignedInt(targetStyle), easing, start, duration, data, flags);
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
         *      Note that removing a currently playing animation with this
         *      function doesn't cause any change to the style index of a
         *      @ref DataHandle it's attached to, if any. In other words, given
         *      data will still keep the original (dynamic) style index even
         *      after it's reused by a different animation. To fix this, either
         *      call @ref TextLayer::setStyle() to change the style to a
         *      different one afterwards or @ref stop() the animation instead
         *      --- assuming @ref AnimationFlag::KeepOncePlayed isn't set, it
         *      will cause the animation to gracefully switch to the target
         *      style during the next @ref advance(), and then be removed
         *      automatically.
         *
         * @see @ref isHandleValid(AnimationHandle) const
         */
        void remove(AnimationHandle handle);

        /**
         * @brief Remove an animation assuming it belongs to this animator
         *
         * Compared to @ref remove(AnimationHandle) delegates to
         * @ref AbstractAnimator::remove(AnimatorDataHandle) instead.
         * @see @ref isHandleValid(AnimatorDataHandle) const,
         *      @ref animationHandleData()
         */
        void remove(AnimatorDataHandle handle);

        /**
         * @brief Animation source and target uniforms
         *
         * Expects that @p handle is valid. The uniforms are queried from
         * @ref BaseLayer::Shared based on style IDs passed to @ref create().
         * @see @ref paddings(), @ref isHandleValid(AnimationHandle) const
         */
        Containers::Pair<BaseLayerStyleUniform, BaseLayerStyleUniform> uniforms(AnimationHandle handle) const;

        /**
         * @brief Animation source and target uniforms assuming it belongs to this animator
         *
         * Like @ref uniforms(AnimationHandle) const but without checking that
         * @p handle indeed belongs to this animator. See its documentation for
         * more information.
         * @see @ref isHandleValid(AnimatorDataHandle) const,
         *      @ref animationHandleData()
         */
        Containers::Pair<BaseLayerStyleUniform, BaseLayerStyleUniform> uniforms(AnimatorDataHandle handle) const;

        /**
         * @brief Animation source and target paddings
         *
         * Expects that @p handle is valid. The paddings are queried from
         * @ref BaseLayer::Shared based on style IDs passed to @ref create().
         * @see @ref uniforms(), @ref isHandleValid(AnimationHandle) const
         */
        Containers::Pair<Vector4, Vector4> paddings(AnimationHandle handle) const;

        /**
         * @brief Animation source and target paddings assuming it belongs to this animator
         *
         * Like @ref paddings(AnimationHandle) const but without checking that
         * @p handle indeed belongs to this animator. See its documentation for
         * more information.
         * @see @ref isHandleValid(AnimatorDataHandle) const,
         *      @ref animationHandleData()
         */
        Containers::Pair<Vector4, Vector4> paddings(AnimatorDataHandle handle) const;

        /**
         * @brief Animation easing function
         *
         * Expects that @p handle is valid. The returned pointer is never
         * @cpp nullptr @ce.
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
         * @return Style properties that were updated by the animation
         *
         * Used internally from @ref BaseLayer::advanceAnimations(Nanoseconds, Containers::MutableBitArrayView, Containers::MutableBitArrayView, Containers::MutableBitArrayView, const Containers::StridedArrayView1D<Float>&, Containers::MutableBitArrayView, const Containers::Iterable<AbstractStyleAnimator>&),
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
        BaseLayerStyleAnimatorUpdates advance(Containers::BitArrayView active, const Containers::StridedArrayView1D<const Float>& factors, Containers::BitArrayView remove, Containers::ArrayView<BaseLayerStyleUniform> dynamicStyleUniforms, const Containers::StridedArrayView1D<Vector4>& dynamicStylePaddings, const Containers::StridedArrayView1D<UnsignedInt>& dataStyles);

    private:
        struct State;

        MAGNUM_UI_LOCAL void createInternal(AnimationHandle handle, UnsignedInt sourceStyle, UnsignedInt targetStyle, Float(*easing)(Float));
};

}}

#endif
