#ifndef Magnum_Ui_BaseLayer_h
#define Magnum_Ui_BaseLayer_h
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
 * @brief Class @ref Magnum::Ui::BaseLayer, struct @ref Magnum::Ui::BaseLayerCommonStyleUniform, @ref Magnum::Ui::BaseLayerStyleUniform
 * @m_since_latest
 */

#include <Magnum/Math/Color.h>

#include "Magnum/Ui/AbstractVisualLayer.h"

namespace Magnum { namespace Ui {

/**
@brief Properties common to all @ref BaseLayer style uniforms
@m_since_latest

Together with one or more @ref BaseLayerStyleUniform instances contains style
properties that are used by the @ref BaseLayer shaders to draw the layer data,
packed in a form that allows direct usage in uniform buffers. Is uploaded
using @ref BaseLayer::Shared::setStyle(), style data that aren't used by the
shader are passed to the function separately.
*/
struct BaseLayerCommonStyleUniform {
    /** @brief Construct with default values */
    constexpr explicit BaseLayerCommonStyleUniform(DefaultInitT = DefaultInit) noexcept: smoothness{0.0f}, innerOutlineSmoothness{0.0f}, backgroundBlurAlpha{1.0f} {}

    /** @brief Constructor */
    constexpr /*implicit*/ BaseLayerCommonStyleUniform(Float smoothness, Float innerOutlineSmoothness, Float blurredBackgroundAlpha): smoothness{smoothness}, innerOutlineSmoothness{innerOutlineSmoothness}, backgroundBlurAlpha{blurredBackgroundAlpha} {}

    /** @brief Construct without blur parameters */
    constexpr /*implicit*/ BaseLayerCommonStyleUniform(Float smoothness, Float innerOutlineSmoothness): smoothness{smoothness}, innerOutlineSmoothness{innerOutlineSmoothness}, backgroundBlurAlpha{1.0f} {}

    /** @brief Construct without blur parameters with the @ref smoothness and @ref innerOutlineSmoothness fields set to the same value */
    constexpr /*implicit*/ BaseLayerCommonStyleUniform(Float smoothness): BaseLayerCommonStyleUniform{smoothness, smoothness} {}

    /** @todo once there's more than one parameters, there could be also a
        variant taking a single smoothness value and all blur parameters? */

    /** @brief Construct without initializing the contents */
    explicit BaseLayerCommonStyleUniform(NoInitT) noexcept {}

    /** @{
     * @name Convenience setters
     *
     * Provided to allow the use of method chaining for populating a structure
     * in a single expression, otherwise equivalent to accessing the fields
     * directly. Also guaranteed to provide backwards compatibility when
     * packing of the actual fields changes.
     */

    /**
     * @brief Set the @ref smoothness and @ref innerOutlineSmoothness fields
     * @return Reference to self (for method chaining)
     */
    CORRADE_CONSTEXPR14 BaseLayerCommonStyleUniform& setSmoothness(Float smoothness, Float innerOutlineSmoothness) {
        this->smoothness = smoothness;
        this->innerOutlineSmoothness = innerOutlineSmoothness;
        return *this;
    }

    /**
     * @brief Set the @ref smoothness and @ref innerOutlineSmoothness fields to the same value
     * @return Reference to self (for method chaining)
     */
    CORRADE_CONSTEXPR14 BaseLayerCommonStyleUniform& setSmoothness(Float smoothness) {
        this->smoothness = smoothness;
        this->innerOutlineSmoothness = smoothness;
        return *this;
    }

    /**
     * @brief Set the @ref backgroundBlurAlpha field
     * @return Reference to self (for method chaining)
     */
    CORRADE_CONSTEXPR14 BaseLayerCommonStyleUniform& setBackgroundBlurAlpha(Float alpha) {
        this->backgroundBlurAlpha = alpha;
        return *this;
    }

    /**
     * @}
     */

    /**
     * @brief Edge smoothness radius
     *
     * In pixels, i.e. setting the value to @cpp 1.0f @ce will make the
     * smoothing extend 1 pixel on each side of the edge. Default value is
     * @cpp 0.0f @ce.
     */
    Float smoothness;

    /**
     * @brief Inner outline edge smoothness radius
     *
     * In pixels, i.e. setting the value to @cpp 1.0f @ce will make the
     * smoothing extend 1 pixel on each side of the edge. Default value is
     * @cpp 0.0f @ce. Not used if @ref BaseLayerSharedFlag::NoOutline is
     * enabled.
     */
    Float innerOutlineSmoothness;

    /**
     * @brief Blurred background alpha
     *
     * If @ref BaseLayerSharedFlag::BackgroundBlur is enabled, the alpha value
     * of @ref BaseLayerStyleUniform::topColor,
     * @relativeref{BaseLayerStyleUniform,bottomColor} and
     * @relativeref{BaseLayerStyleUniform,outlineColor} is used to interpolate
     * between the color value and the blurred background. Making this value
     * less than @cpp 1.0f @ce makes the original unblurred framebuffer
     * contents show through as well, which can be used to achieve a glow-like
     * effect. Default value is @cpp 1.0f @ce. A similar effect can also be
     * achieved using @ref BaseLayerSharedFlag::TextureMask and pixel alpha
     * values between @cpp 0.0f @ce and @cpp 1.0f @ce.
     *
     * @m_class{m-row}
     *
     * @parblock
     *
     * @m_div{m-col-s-6 m-text-center m-nopadt m-nopadx}
     * @image html ui-baselayer-blur.png width=256px
     * Default value of @cpp 1.0f @ce
     * @m_enddiv
     *
     * @m_div{m-col-s-6 m-text-center m-nopadt m-nopadx}
     * @image html ui-baselayer-blur-alpha.png width=256px
     * Value of @cpp 0.75f @ce
     * @m_enddiv
     *
     * @endparblock
     */
    Float backgroundBlurAlpha;

    #ifndef DOXYGEN_GENERATING_OUTPUT
    Int:32;
    #endif
};

/**
@brief @ref BaseLayer style uniform
@m_since_latest

Instances of this class together with @ref BaseLayerCommonStyleUniform contain
style properties that are used by the @ref BaseLayer shaders to draw the layer
data, packed in a form that allows direct usage in uniform buffers. Total count
of styles is specified with the
@ref BaseLayer::Shared::Configuration::Configuration() constructor, uniforms
are then uploaded using @ref BaseLayer::Shared::setStyle(), style data that
aren't used by the shader are passed to the function separately.
*/
struct BaseLayerStyleUniform {
    /** @brief Construct with default values */
    constexpr explicit BaseLayerStyleUniform(DefaultInitT = DefaultInit) noexcept: topColor{1.0f}, bottomColor{1.0f}, outlineColor{1.0f}, outlineWidth{0.0f}, cornerRadius{0.0f}, innerOutlineCornerRadius{0.0f} {}

    /** @brief Constructor */
    constexpr /*implicit*/ BaseLayerStyleUniform(const Color4& topColor, const Color4& bottomColor, const Color4& outlineColor, const Vector4& outlineWidth, const Vector4& cornerRadius, const Vector4& innerOutlineCornerRadius): topColor{topColor}, bottomColor{bottomColor}, outlineColor{outlineColor}, outlineWidth{outlineWidth}, cornerRadius{cornerRadius}, innerOutlineCornerRadius{innerOutlineCornerRadius} {}

    /**
     * @brief Construct with all corners having the same radius and all edges the same outline width
     *
     * The @ref outlineWidth, @ref cornerRadius and
     * @ref innerOutlineCornerRadius fields have all components set to the
     * values of @p outlineWidth, @p cornerRadius and
     * @p innerOutlineCornerRadius.
     */
    constexpr /*implicit*/ BaseLayerStyleUniform(const Color4& topColor, const Color4& bottomColor, const Color4& outlineColor, Float outlineWidth, Float cornerRadius, Float innerOutlineCornerRadius): BaseLayerStyleUniform{topColor, bottomColor, outlineColor, Vector4{outlineWidth}, Vector4{cornerRadius}, Vector4{innerOutlineCornerRadius}} {}

    /**
     * @brief Construct with no outline
     *
     * The @ref outlineColor field is set to @cpp 0xffffff_srgbf @ce,
     * @ref outlineWidth to a zero vector and both @ref cornerRadius and
     * @ref innerOutlineCornerRadius get a value of @p cornerRadius.
     */
    constexpr /*implicit*/ BaseLayerStyleUniform(const Color4& topColor, const Color4& bottomColor, const Vector4& cornerRadius): BaseLayerStyleUniform{topColor, bottomColor, Color4{1.0f}, Vector4{0.0f}, cornerRadius, cornerRadius} {}

    /**
     * @brief Construct with no outline and all corners having the same radius
     *
     * Delegates to @ref BaseLayerStyleUniform(const Color4&, const Color4&, const Vector4&)
     * with @p cornerRadius having all components set to the same value.
     */
    constexpr /*implicit*/ BaseLayerStyleUniform(const Color4& topColor, const Color4& bottomColor, Float cornerRadius): BaseLayerStyleUniform{topColor, bottomColor, Vector4{cornerRadius}} {}

    /**
     * @brief Construct with no gradient
     *
     * The @ref topColor and @ref bottomColor fields are both set to the value
     * of @p color.
     */
    constexpr /*implicit*/ BaseLayerStyleUniform(const Color4& color, const Color4& outlineColor, const Vector4& outlineWidth, const Vector4& cornerRadius, const Vector4& innerOutlineCornerRadius): BaseLayerStyleUniform{color, color, outlineColor, outlineWidth, cornerRadius, innerOutlineCornerRadius} {}

    /**
     * @brief Construct with no gradient, all corners having the same radius and all edges the same outline width
     *
     * Delegates to @ref BaseLayerStyleUniform(const Color4&, const Color4&, const Color4&, Float, Float, Float)
     * with @p color used for both @p topColor and @p bottomColor.
     */
    constexpr /*implicit*/ BaseLayerStyleUniform(const Color4& color, const Color4& outlineColor, Float outlineWidth, Float cornerRadius, Float innerOutlineCornerRadius): BaseLayerStyleUniform{color, color, outlineColor, outlineWidth, cornerRadius, innerOutlineCornerRadius} {}

    /**
     * @brief Construct with no gradient and no outline
     *
     * Delegates to @ref BaseLayerStyleUniform(const Color4&, const Color4&, const Vector4&)
     * with @p color used for both @p topColor and @p bottomColor.
     */
    constexpr /*implicit*/ BaseLayerStyleUniform(const Color4& color, const Vector4& cornerRadius): BaseLayerStyleUniform{color, color, cornerRadius} {}

    /**
     * @brief Construct with no gradient, no outline and all corners having the same radius
     *
     * Delegates to @ref BaseLayerStyleUniform(const Color4&, const Color4&, Float)
     * with @p color used for both @p topColor and @p bottomColor.
     */
    constexpr /*implicit*/ BaseLayerStyleUniform(const Color4& color, Float cornerRadius): BaseLayerStyleUniform{color, color, cornerRadius} {}

    /** @brief Construct without initializing the contents */
    explicit BaseLayerStyleUniform(NoInitT) noexcept: topColor{NoInit}, bottomColor{NoInit}, outlineColor{NoInit}, outlineWidth{NoInit}, cornerRadius{NoInit}, innerOutlineCornerRadius{NoInit} {}

    /** @{
     * @name Convenience setters
     *
     * Provided to allow the use of method chaining for populating a structure
     * in a single expression, otherwise equivalent to accessing the fields
     * directly. Also guaranteed to provide backwards compatibility when
     * packing of the actual fields changes.
     */

    /**
     * @brief Set the @ref topColor and @ref bottomColor fields
     * @return Reference to self (for method chaining)
     */
    CORRADE_CONSTEXPR14 BaseLayerStyleUniform& setColor(const Color4& top, const Color4& bottom) {
        topColor = top;
        bottomColor = bottom;
        return *this;
    }

    /**
     * @brief Set the @ref topColor and @ref bottomColor fields to the same value
     * @return Reference to self (for method chaining)
     */
    CORRADE_CONSTEXPR14 BaseLayerStyleUniform& setColor(const Color4& color) {
        topColor = color;
        bottomColor = color;
        return *this;
    }

    /**
     * @brief Set the @ref outlineColor field
     * @return Reference to self (for method chaining)
     */
    CORRADE_CONSTEXPR14 BaseLayerStyleUniform& setOutlineColor(const Color4& color) {
        outlineColor = color;
        return *this;
    }

    /**
     * @brief Set the @ref outlineWidth field
     * @return Reference to self (for method chaining)
     */
    CORRADE_CONSTEXPR14 BaseLayerStyleUniform& setOutlineWidth(const Vector4& width) {
        outlineWidth = width;
        return *this;
    }

    /**
     * @brief Set the @ref outlineWidth field with all edges having the same value
     * @return Reference to self (for method chaining)
     */
    CORRADE_CONSTEXPR14 BaseLayerStyleUniform& setOutlineWidth(Float width) {
        outlineWidth = Vector4{width};
        return *this;
    }

    /**
     * @brief Set the @ref cornerRadius field
     * @return Reference to self (for method chaining)
     */
    CORRADE_CONSTEXPR14 BaseLayerStyleUniform& setCornerRadius(const Vector4& radius) {
        cornerRadius = radius;
        return *this;
    }

    /**
     * @brief Set the @ref cornerRadius field with all corners having the same value
     * @return Reference to self (for method chaining)
     */
    CORRADE_CONSTEXPR14 BaseLayerStyleUniform& setCornerRadius(Float radius) {
        cornerRadius = Vector4{radius};
        return *this;
    }

    /**
     * @brief Set the @ref innerOutlineCornerRadius field
     * @return Reference to self (for method chaining)
     */
    CORRADE_CONSTEXPR14 BaseLayerStyleUniform& setInnerOutlineCornerRadius(const Vector4& radius) {
        innerOutlineCornerRadius = radius;
        return *this;
    }

    /**
     * @brief Set the @ref innerOutlineCornerRadius field with all corners having the same value
     * @return Reference to self (for method chaining)
     */
    CORRADE_CONSTEXPR14 BaseLayerStyleUniform& setInnerOutlineCornerRadius(Float radius) {
        innerOutlineCornerRadius = Vector4{radius};
        return *this;
    }

    /**
     * @}
     */

    /**
     * @brief Top gradient color
     *
     * Default value is @cpp 0xffffffff_srgbf @ce.
     */
    Color4 topColor;

    /**
     * @brief Bottom gradient color
     *
     * Default value is @cpp 0xffffffff_srgbf @ce.
     */
    Color4 bottomColor;

    /**
     * @brief Outline color
     *
     * Default value is @cpp 0xffffffff_srgbf @ce. Visible only if
     * @ref outlineWidth is non-zero on at least one side or if the difference
     * between @ref cornerRadius and @ref innerOutlineCornerRadius makes it
     * show. Not used if @ref BaseLayerSharedFlag::NoOutline is enabled.
     */
    Color4 outlineColor;

    /**
     * @brief Outline width
     *
     * In order left, top, right, bottom. Default value is @cpp 0.0f @ce for
     * all sides. Not used if @ref BaseLayerSharedFlag::NoOutline is enabled.
     */
    Vector4 outlineWidth;

    /**
     * @brief Corner radius
     *
     * In order top left, bottom left, top right, bottom right. Default value
     * is @cpp 0.0f @ce for all sides. Not used if
     * @ref BaseLayerSharedFlag::NoRoundedCorners is enabled.
     */
    Vector4 cornerRadius;

    /**
     * @brief Inner outline corner radius
     *
     * In order top left, bottom left, top right, bottom right. Default value
     * is @cpp 0.0f @ce for all sides. Not used if
     * @ref BaseLayerSharedFlag::NoOutline or
     * @relativeref{BaseLayerSharedFlag,NoRoundedCorners} is enabled.
     */
    Vector4 innerOutlineCornerRadius;
};

/**
@brief Base layer
@m_since_latest

Draws quads with a color gradient, variable rounded corners and outline. You'll
most likely instantiate the class through @ref BaseLayerGL, which contains a
concrete OpenGL implementation.
@see @ref UserInterface::baseLayer(),
    @ref UserInterface::setBaseLayerInstance(), @ref StyleFeature::BaseLayer,
    @ref BaseLayerStyleAnimator
*/
class MAGNUM_UI_EXPORT BaseLayer: public AbstractVisualLayer {
    public:
        class Shared;

        /**
         * @brief Shared state used by this layer
         *
         * Reference to the instance passed to @ref BaseLayerGL::BaseLayerGL(LayerHandle, Shared&).
         */
        inline Shared& shared();
        inline const Shared& shared() const; /**< @overload */

        /**
         * @brief Background blur pass count
         *
         * Expects that @ref BaseLayerSharedFlag::BackgroundBlur was enabled
         * for the shared state the layer was created with.
         */
        UnsignedInt backgroundBlurPassCount() const;

        /**
         * @brief Set background blur pass count
         * @return Reference to self (for method chaining)
         *
         * Expects that @ref BaseLayerSharedFlag::BackgroundBlur was enabled
         * for the shared state the layer was created with and that @p count is
         * at least @cpp 1 @ce. Higher values will perform the blurring process
         * several times, which has the same effect as applying a single,
         * larger, Gaussian blur. With @f$ r @f$ being the radius configured by
         * @ref Shared::Configuration::setBackgroundBlurRadius() and @f$ n @f$
         * being the @p count, the relation to the larger radius @f$ l @f$ is
         * as follows: @f[
         *      l = \sqrt{nr^2}
         * @f]
         *
         * Thus by combining the radius and pass count it's possible to achieve
         * blurring in radii larger than the limit of @cpp 31 @ce in
         * @ref Shared::Configuration::setBackgroundBlurRadius(), or
         * alternatively tune the operation based on whether the GPU is faster
         * with few passes and many texture samples each or many passes with
         * few (localized) texture samples each. For example, a radius of 36
         * can be achieved with 16 passes of radius 9, or with 4 passes of
         * radius 18, or other combinations.
         *
         * Default pass count is @cpp 1 @ce.
         *
         * Calling this function causes
         * @ref LayerState::NeedsCompositeOffsetSizeUpdate to be set.
         */
        BaseLayer& setBackgroundBlurPassCount(UnsignedInt count);

        /**
         * @brief Assign a style animator to this layer
         * @return Reference to self (for method chaining)
         *
         * Expects that @ref Shared::dynamicStyleCount() is non-zero and that
         * given @p animator wasn't passed to @ref assignAnimator() on any
         * layer yet. On the other hand, it's possible to associate multiple
         * different animators with the same layer.
         * @see @ref setDefaultStyleAnimator()
         */
        BaseLayer& assignAnimator(BaseLayerStyleAnimator& animator);

        /**
         * @brief Default style animator for this layer
         *
         * If a style animator haven't been set, returns @cpp nullptr @ce. If
         * not @cpp nullptr @ce, the returned animator is guaranteed to be
         * assigned to this layer, i.e. that
         * @ref BaseLayerStyleAnimator::layer() is equal to @ref handle().
         */
        BaseLayerStyleAnimator* defaultStyleAnimator() const;

        /**
         * @brief Set a default style animator for this layer
         * @return Reference to self (for method chaining)
         *
         * Makes @p animator used in style transitions in response to events.
         * Expects that @p animator is either @cpp nullptr @ce or is already
         * assigned to this layer, i.e. that @ref assignAnimator() was called
         * on this layer with @p animator before. Calling this function again
         * with a different animator or with @cpp nullptr @ce replaces the
         * previous one.
         * @see @ref allocateDynamicStyle()
         */
        BaseLayer& setDefaultStyleAnimator(BaseLayerStyleAnimator* animator);

        /**
         * @brief Dynamic style uniforms
         *
         * Size of the returned view is @ref Shared::dynamicStyleCount(). These
         * uniforms are used by style indices greater than or equal to
         * @ref Shared::styleCount().
         * @see @ref dynamicStylePaddings(), @ref setDynamicStyle()
         */
        Containers::ArrayView<const BaseLayerStyleUniform> dynamicStyleUniforms() const;

        /**
         * @brief Dynamic style paddings
         *
         * Size of the returned view is @ref Shared::dynamicStyleCount(). These
         * paddings are used by style indices greater than or equal to
         * @ref Shared::styleCount().
         * @see @ref dynamicStyleUniforms(), @ref setDynamicStyle()
         */
        Containers::StridedArrayView1D<const Vector4> dynamicStylePaddings() const;

        /**
         * @brief Set a dynamic style
         * @param id                Dynamic style ID
         * @param uniform           Style uniform
         * @param padding           Padding inside the node in order left, top,
         *      right, bottom
         *
         * Expects that the @p id is less than @ref Shared::dynamicStyleCount().
         * @ref Shared::styleCount() plus @p id is then a style index that can
         * be passed to @ref create() or @ref setStyle() in order to use this
         * style. Compared to @ref Shared::setStyle() the mapping between
         * dynamic styles and uniforms is implicit. All dynamic styles are
         * initially default-constructed @ref BaseLayerStyleUniform instances
         * and zero padding vectors.
         *
         * Calling this function causes @ref LayerState::NeedsCommonDataUpdate
         * to be set to trigger an upload of changed dynamic style uniform
         * data. If @p padding changed, @ref LayerState::NeedsDataUpdate gets
         * set as well.
         * @see @ref dynamicStyleUniforms(), @ref dynamicStylePaddings(),
         *      @ref allocateDynamicStyle(), @ref recycleDynamicStyle()
         */
        void setDynamicStyle(UnsignedInt id, const BaseLayerStyleUniform& uniform, const Vector4& padding);

        /**
         * @brief Create a quad
         * @param style         Style index
         * @param node          Node to attach to
         * @return New data handle
         *
         * Expects that @p style is less than @ref Shared::totalStyleCount().
         * All styling is driven from the @ref BaseLayerStyleUniform at index
         * @p style. Use @ref create(UnsignedInt, const Color3&, NodeHandle) or
         * @ref create(UnsignedInt, const Color3&, const Vector4&, NodeHandle)
         * for creating a quad with a custom color and outline width. This
         * function is equivalent to calling them with @cpp 0xffffff_srgbf @ce
         * for the base color and a zero vector for the outline width.
         */
        DataHandle create(UnsignedInt style, NodeHandle node =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            NodeHandle::Null
            #else
            NodeHandle{} /* To not have to include Handle.h */
            #endif
        ) {
            return create(style, Color3{1.0f}, node);
        }

        /**
         * @brief Create a quad with a style index in a concrete enum type
         *
         * Casts @p style to @relativeref{Magnum,UnsignedInt} and delegates to
         * @ref create(UnsignedInt, NodeHandle).
         */
        template<class StyleIndex
            #ifndef DOXYGEN_GENERATING_OUTPUT
            , class = typename std::enable_if<std::is_enum<StyleIndex>::value>::type
            #endif
        > DataHandle create(StyleIndex style, NodeHandle node =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            NodeHandle::Null
            #else
            NodeHandle{} /* To not have to include Handle.h */
            #endif
        ) {
            return create(UnsignedInt(style), node);
        }

        /**
         * @brief Create a quad with a custom base color
         * @param style         Style index
         * @param color         Custom base color
         * @param node          Node to attach to
         * @return New data handle
         *
         * Expects that @p style is less than @ref Shared::totalStyleCount().
         * Styling is driven from the @ref BaseLayerStyleUniform at index
         * @p style, in addition @ref BaseLayerStyleUniform::topColor and
         * @relativeref{BaseLayerStyleUniform,bottomColor} is multiplied with
         * @p color. Use @ref create(UnsignedInt, const Color3&, const Vector4&, NodeHandle)
         * for creating a quad with a custom color and outline width. This
         * function is equivalent to calling it with a zero vector for the
         * outline width.
         * @see @ref create(UnsignedInt, NodeHandle)
         */
        DataHandle create(UnsignedInt style, const Color3& color, NodeHandle node =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            NodeHandle::Null
            #else
            NodeHandle{} /* To not have to include Handle.h */
            #endif
        ) {
            return create(style, color, {}, node);
        }

        /**
         * @brief Create a quad with a style index in a concrete enum type and with a custom base color
         *
         * Casts @p style to @relativeref{Magnum,UnsignedInt} and delegates to
         * @ref create(UnsignedInt, const Color3&, NodeHandle).
         */
        template<class StyleIndex
            #ifndef DOXYGEN_GENERATING_OUTPUT
            , class = typename std::enable_if<std::is_enum<StyleIndex>::value>::type
            #endif
        > DataHandle create(StyleIndex style, const Color3& color, NodeHandle node =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            NodeHandle::Null
            #else
            NodeHandle{} /* To not have to include Handle.h */
            #endif
        ) {
            return create(UnsignedInt(style), color, node);
        }

        /**
         * @brief Create a quad with a custom base color and outline width
         * @param style         Style index
         * @param color         Custom base color
         * @param outlineWidth  Custom outline width in order left, top, right,
         *      bottom
         * @param node          Node to attach to
         * @return New data handle
         *
         * Expects that @p style is less than @ref Shared::totalStyleCount().
         * Styling is driven from the @ref BaseLayerStyleUniform at index
         * @p style, in addition @ref BaseLayerStyleUniform::topColor and
         * @relativeref{BaseLayerStyleUniform,bottomColor} is multiplied with
         * @p color and @p outlineWidth is added to
         * @ref BaseLayerStyleUniform::outlineWidth.
         * @see @ref create(UnsignedInt, NodeHandle),
         *      @ref create(UnsignedInt, const Color3&, NodeHandle)
         */
        DataHandle create(UnsignedInt style, const Color3& color, const Vector4& outlineWidth, NodeHandle node =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            NodeHandle::Null
            #else
            NodeHandle{} /* To not have to include Handle.h */
            #endif
        );

        /**
         * @brief Create a quad with a style index in a concrete enum type and with a custom base color and outline width
         *
         * Casts @p style to @relativeref{Magnum,UnsignedInt} and delegates to
         * @ref create(UnsignedInt, const Color3&, const Vector4&, NodeHandle).
         */
        template<class StyleIndex
            #ifndef DOXYGEN_GENERATING_OUTPUT
            , class = typename std::enable_if<std::is_enum<StyleIndex>::value>::type
            #endif
        > DataHandle create(StyleIndex style, const Color3& color, const Vector4& outlineWidth, NodeHandle node =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            NodeHandle::Null
            #else
            NodeHandle{} /* To not have to include Handle.h */
            #endif
        ) {
            return create(UnsignedInt(style), color, outlineWidth, node);
        }

        /**
         * @brief Create a quad with a custom base color and outline width with all edges having the same value
         *
         * See @ref create(UnsignedInt, const Color3&, const Vector4&, NodeHandle)
         * for more information.
         */
        DataHandle create(UnsignedInt style, const Color3& color, Float outlineWidth, NodeHandle node =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            NodeHandle::Null
            #else
            NodeHandle{} /* To not have to include Handle.h */
            #endif
        ) {
            return create(style, color, Vector4{outlineWidth}, node);
        }

        /**
         * @brief Create a quad with a style index in a concrete enum type and a custom base color and outline width with all edges having the same value
         *
         * Casts @p style to @relativeref{Magnum,UnsignedInt} and delegates to
         * @ref create(UnsignedInt, const Color3&, Float, NodeHandle).
         */
        template<class StyleIndex
            #ifndef DOXYGEN_GENERATING_OUTPUT
            , class = typename std::enable_if<std::is_enum<StyleIndex>::value>::type
            #endif
        > DataHandle create(StyleIndex style, const Color3& color, Float outlineWidth, NodeHandle node =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            NodeHandle::Null
            #else
            NodeHandle{} /* To not have to include Handle.h */
            #endif
        ) {
            return create(UnsignedInt(style), color, outlineWidth, node);
        }

        /**
         * @brief Remove a quad
         *
         * Delegates to @ref AbstractLayer::remove(DataHandle).
         */
        void remove(DataHandle handle) {
            AbstractVisualLayer::remove(handle);
        }

        /**
         * @brief Remove a quad assuming it belongs to this layer
         *
         * Delegates to @ref AbstractLayer::remove(LayerDataHandle).
         */
        void remove(LayerDataHandle handle) {
            AbstractVisualLayer::remove(handle);
        }

        /**
         * @brief Quad custom base color
         *
         * Expects that @p handle is valid.
         * @see @ref isHandleValid(DataHandle) const
         */
        Color3 color(DataHandle handle) const;

        /**
         * @brief Quad custom base color assuming it belongs to this layer
         *
         * Expects that @p handle is valid.
         * @see @ref isHandleValid(LayerDataHandle) const
         */
        Color3 color(LayerDataHandle handle) const;

        /**
         * @brief Set quad custom base color
         *
         * Expects that @p handle is valid.
         * @ref BaseLayerStyleUniform::topColor and
         * @relativeref{BaseLayerStyleUniform,bottomColor} is multiplied with
         * @p color. By default, unless specified in @ref create() already, the
         * custom color is @cpp 0xffffff_srgbf @ce, i.e. not affecting the
         * style in any way.
         *
         * Calling this function causes @ref LayerState::NeedsDataUpdate to be
         * set.
         * @see @ref isHandleValid(DataHandle) const
         */
        void setColor(DataHandle handle, const Color3& color);

        /**
         * @brief Set quad custom base color assuming it belongs to this layer
         *
         * Like @ref setColor(DataHandle, const Color3&) but without checking
         * that @p handle indeed belongs to this layer. See its documentation
         * for more information.
         */
        void setColor(LayerDataHandle handle, const Color3& color);

        /**
         * @brief Quad custom outline width
         *
         * In order left, top. right, bottom. Expects that @p handle is valid.
         * @see @ref isHandleValid(DataHandle) const
         */
        Vector4 outlineWidth(DataHandle handle) const;

        /**
         * @brief Quad custom outline width assuming it belongs to this layer
         *
         * In order left, top. right, bottom. Expects that @p handle is valid.
         * @see @ref isHandleValid(LayerDataHandle) const
         */
        Vector4 outlineWidth(LayerDataHandle handle) const;

        /**
         * @brief Set quad custom outline width
         *
         * Expects that @p handle is valid. The @p width is in order left, top,
         * right, bottom and is added to
         * @ref BaseLayerStyleUniform::outlineWidth. By default, unless
         * specified in @ref create() already, the custom outline width is a
         * zero vector, i.e. not affecting the style in any way. Has no visual
         * effect if @ref BaseLayerSharedFlag::NoOutline is enabled.
         *
         * Calling this function causes @ref LayerState::NeedsDataUpdate to be
         * set.
         * @see @ref isHandleValid(DataHandle) const
         */
        void setOutlineWidth(DataHandle handle, const Vector4& width);

        /**
         * @brief Set quad custom outline width with all edges having the same value
         *
         * Expects that @p handle is valid. The @p width is added to
         * @ref BaseLayerStyleUniform::outlineWidth. By default, unless
         * specified in @ref create() already, the custom outline width is
         * zero, i.e. not affecting the style in any way. Has no visual effect
         * if @ref BaseLayerSharedFlag::NoOutline is enabled.
         *
         * Calling this function causes @ref LayerState::NeedsDataUpdate to be
         * set.
         * @see @ref isHandleValid(DataHandle) const
         */
        void setOutlineWidth(DataHandle handle, Float width) {
            setOutlineWidth(handle, Vector4{width});
        }

        /**
         * @brief Set quad custom outline width assuming it belongs to this layer
         *
         * Like @ref setOutlineWidth(DataHandle, const Vector4&) but without
         * checking that @p handle indeed belongs to this layer. See its
         * documentation for more information.
         */
        void setOutlineWidth(LayerDataHandle handle, const Vector4& width);

        /**
         * @brief Set quad custom outline width with all edges having the same value assuming it belongs to this layer
         *
         * Like @ref setOutlineWidth(DataHandle, Float) but without checking
         * that @p handle indeed belongs to this layer. See its documentation
         * for more information.
         */
        void setOutlineWidth(LayerDataHandle handle, Float width) {
            setOutlineWidth(handle, Vector4{width});
        }

        /**
         * @brief Quad custom padding
         *
         * In order left, top. right, bottom. Expects that @p handle is valid.
         * @see @ref isHandleValid(DataHandle) const
         */
        Vector4 padding(DataHandle handle) const;

        /**
         * @brief Quad custom padding assuming it belongs to this layer
         *
         * In order left, top. right, bottom. Expects that @p handle is valid.
         * @see @ref isHandleValid(LayerDataHandle) const
         */
        Vector4 padding(LayerDataHandle handle) const;

        /**
         * @brief Set quad custom padding
         *
         * Expects that @p handle is valid. The @p padding is in order left,
         * top, right, bottom and is added to the per-style padding values
         * specified in @ref Shared::setStyle(). By default the padding is a
         * zero vector, i.e. the quad fills the node area completely.
         *
         * Calling this function causes @ref LayerState::NeedsDataUpdate to be
         * set.
         * @see @ref isHandleValid(DataHandle) const
         */
        void setPadding(DataHandle handle, const Vector4& padding);

        /**
         * @brief Set quad custom padding assuming it belongs to this layer
         *
         * Like @ref setPadding(DataHandle, const Vector4&) but without
         * checking that @p handle indeed belongs to this layer. See its
         * documentation for more information.
         */
        void setPadding(LayerDataHandle handle, const Vector4& padding);

        /**
         * @brief Set quad custom padding with all edges having the same value
         *
         * Expects that @p handle is valid. The @p padding is added to the
         * per-style padding values specified in @ref Shared::setStyle(). By
         * default there's zero padding, i.e. the quad fills the node area
         * completely.
         *
         * Calling this function causes @ref LayerState::NeedsDataUpdate to be
         * set.
         * @see @ref isHandleValid(DataHandle) const
         */
        void setPadding(DataHandle handle, Float padding) {
            setPadding(handle, Vector4{padding});
        }

        /**
         * @brief Set quad custom padding with all edges having the same value assuming it belongs to this layer
         *
         * Like @ref setPadding(DataHandle, Float) but without checking that
         * @p handle indeed belongs to this layer. See its documentation for
         * more information.
         */
        void setPadding(LayerDataHandle handle, Float padding) {
            setPadding(handle, Vector4{padding});
        }

        /**
         * @brief Quad texture coordinate offset
         *
         * The third coordinate is array layer. Expects that @p handle is
         * valid and that @ref BaseLayerSharedFlag::Textured was enabled for
         * the shared state the layer was created with.
         * @see @ref isHandleValid(DataHandle) const
         */
        Vector3 textureCoordinateOffset(DataHandle handle) const;

        /**
         * @brief Quad texture coordinate offset assuming it belongs to this layer
         *
         * The third coordinate is array layer. Expects that @p handle is
         * valid and that @ref BaseLayerSharedFlag::Textured was enabled for
         * the shared state the layer was created with.
         * @see @ref isHandleValid(LayerDataHandle) const
         */
        Vector3 textureCoordinateOffset(LayerDataHandle handle) const;

        /**
         * @brief Quad texture coordinate size
         *
         * Expects that @p handle is valid and that
         * @ref BaseLayerSharedFlag::Textured was enabled for the shared state
         * the layer was created with.
         * @see @ref isHandleValid(DataHandle) const
         */
        Vector2 textureCoordinateSize(DataHandle handle) const;

        /**
         * @brief Quad texture coordinate size assuming it belongs to this layer
         *
         * Expects that @p handle is valid and that
         * @ref BaseLayerSharedFlag::Textured was enabled for the shared state
         * the layer was created with.
         * @see @ref isHandleValid(LayerDataHandle) const
         */
        Vector2 textureCoordinateSize(LayerDataHandle handle) const;

        /**
         * @brief Set quad texture coordinates
         *
         * The third coordinate of @p offset is array layer. Expects that
         * @p handle is valid and that @ref BaseLayerSharedFlag::Textured was
         * enabled for the shared state the layer was created with. By default
         * the offset is @cpp {0.0f, 0.0f, 0.0f} @ce and size is
         * @cpp {1.0f, 1.0f} @ce, i.e. covering the whole first slice of the
         * texture.
         *
         * Calling this function causes @ref LayerState::NeedsDataUpdate to be
         * set.
         * @see @ref BaseLayerGL::setTexture()
         */
        void setTextureCoordinates(DataHandle handle, const Vector3& offset, const Vector2& size);

        /**
         * @brief Set quad texture coordinates assuming it belongs to this layer
         *
         * Like @ref setTextureCoordinates(DataHandle, const Vector3&, const Vector2&)
         * but without checking that @p handle indeed belongs to this layer.
         * See its documentation for more information.
         */
        void setTextureCoordinates(LayerDataHandle handle, const Vector3& offset, const Vector2& size);

    #ifdef DOXYGEN_GENERATING_OUTPUT
    private:
    #else
    protected:
    #endif
        struct State;

        MAGNUM_UI_LOCAL explicit BaseLayer(LayerHandle handle, Containers::Pointer<State>&& state);
        /* Used by tests to avoid having to include / allocate the state */
        explicit BaseLayer(LayerHandle handle, Shared& shared);

        /* These can't be MAGNUM_UI_LOCAL otherwise deriving from this class
           in tests causes linker errors */

        /* Advertises LayerFeature::Draw (and Composite if BackgroundBlur is
           enabled) but *does not* implement doDraw() or doComposite(), that's
           on the subclass */
        LayerFeatures doFeatures() const override;

        LayerStates doState() const override;
        void doSetSize(const Vector2& size, const Vector2i& framebufferSize) override;
        void doUpdate(LayerStates states, const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, Containers::BitArrayView nodesEnabled, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes, const Containers::StridedArrayView1D<const Vector2>& compositeRectOffsets, const Containers::StridedArrayView1D<const Vector2>& compositeRectSizes) override;

    private:
        MAGNUM_UI_LOCAL void setColorInternal(UnsignedInt id, const Color3& color);
        MAGNUM_UI_LOCAL void setOutlineWidthInternal(UnsignedInt id, const Vector4& width);
        MAGNUM_UI_LOCAL void setPaddingInternal(UnsignedInt id, const Vector4& padding);
        MAGNUM_UI_LOCAL Vector3 textureCoordinateOffsetInternal(UnsignedInt id) const;
        MAGNUM_UI_LOCAL Vector2 textureCoordinateSizeInternal(UnsignedInt id) const;
        MAGNUM_UI_LOCAL void setTextureCoordinatesInternal(UnsignedInt id, const Vector3& offset, const Vector2& size);

        /* Can't be MAGNUM_UI_LOCAL otherwise deriving from this class in
           tests causes linker errors */
        void doAdvanceAnimations(Nanoseconds time, Containers::MutableBitArrayView activeStorage, const Containers::StridedArrayView1D<Float>& factorStorage, Containers::MutableBitArrayView removeStorage, const Containers::Iterable<AbstractStyleAnimator>& animators) override;
};

/**
@brief Base layer shared state flag
@m_since_latest

@see @ref BaseLayerSharedFlags,
    @ref BaseLayer::Shared::Configuration::setFlags(),
    @ref BaseLayer::Shared::flags()
*/
enum class BaseLayerSharedFlag: UnsignedByte {
    /**
     * Textured drawing. If enabled, the @ref BaseLayerStyleUniform::topColor
     * and @relativeref{BaseLayerStyleUniform,bottomColor} is multiplied with a
     * color coming from a texture set in @ref BaseLayerGL::setTexture() and
     * texture coordinates specified with @ref BaseLayer::setTextureCoordinates().
     * @see @ref BaseLayerSharedFlag::TextureMask
     */
    Textured = 1 << 0,

    /**
     * Blur the background of semi-transparent quads. If enabled, the
     * alpha value of @ref BaseLayerStyleUniform::topColor,
     * @relativeref{BaseLayerStyleUniform,bottomColor} and
     * @relativeref{BaseLayerStyleUniform,outlineColor} is used to interpolate
     * between the color and the blurred background, instead of performing a
     * classical blending of the color and the framebuffer contents underneath.
     *
     * @m_class{m-row}
     *
     * @parblock
     *
     * @m_div{m-col-m-6 m-text-center m-nopadt m-nopadx}
     * @image html ui-baselayer-default.png width=256px
     * Without @ref BaseLayerSharedFlag::BackgroundBlur
     * @m_enddiv
     *
     * @m_div{m-col-m-6 m-text-center m-nopadt m-nopadx}
     * @image html ui-baselayer-blur.png width=256px
     * With @ref BaseLayerSharedFlag::BackgroundBlur
     * @m_enddiv
     *
     * @endparblock
     *
     * Use @ref BaseLayer::Shared::Configuration::setBackgroundBlurRadius() and
     * @ref BaseLayer::setBackgroundBlurPassCount() to control the blur radius
     * and @ref BaseLayerCommonStyleUniform::backgroundBlurAlpha to achieve
     * additional effects.
     *
     * @see @ref BaseLayerSharedFlag::TextureMask
     */
    BackgroundBlur = 1 << 1,

    /**
     * Disable support for rounded corners. If set, the
     * @ref BaseLayerStyleUniform::cornerRadius and
     * @relativeref{BaseLayerStyleUniform,innerOutlineCornerRadius} fields are
     * not used and the behavior is the same as if they were both set to
     * @cpp 0.0f @ce. Can result in rendering performance improvement, useful
     * for example when @ref BaseLayerSharedFlag::Textured is enabled and the
     * layer is used just to draw (alpha blended) images, or if a particular
     * widget style doesn't use rounded corners at all.
     *
     * Mutually exclusive with the @ref BaseLayerSharedFlag::SubdividedQuads
     * optimization.
     * @see @ref BaseLayerSharedFlag::NoOutline
     */
    NoRoundedCorners = 1 << 2,

    /**
     * Disable support for rounded corners. If set, the
     * @ref BaseLayerCommonStyleUniform::innerOutlineSmoothness,
     * @ref BaseLayerStyleUniform::outlineColor,
     * @relativeref{BaseLayerStyleUniform,outlineWidth} and
     * @relativeref{BaseLayerStyleUniform,innerOutlineCornerRadius} fields are
     * not used and @ref BaseLayer::setOutlineWidth() has no effect. The
     * behavior is then the same as if the outline width was set to
     * @cpp 0.0f @ce on all sides both in the style and for all data. Can
     * result in rendering performance improvement, useful for example when
     * @ref BaseLayerSharedFlag::Textured is enabled and the layer is used just
     * to draw (alpha blended) images, or if a particular widget style doesn't
     * use outlines at all.
     *
     * Mutually exclusive with the @ref BaseLayerSharedFlag::SubdividedQuads
     * optimization.
     * @see @ref BaseLayerSharedFlag::NoRoundedCorners
     */
    NoOutline = 1 << 3,

    /**
     * Use alpha channel of the texture to mask out the outline and background
     * blur. By default the outline is drawn over the texture without taking
     * the texture color or alpha into account; and the background is blurred
     * for the whole area of the quad, with transparent areas of the texture
     * causing just the blurred background to be shown. Enabling this flag
     * causes the transparent areas to make holes in both the outline and the
     * blurred background. Implies @ref BaseLayerSharedFlag::Textured.
     *
     * @m_class{m-row}
     *
     * @parblock
     *
     * @m_div{m-col-m-6 m-text-center m-nopadt m-nopadx}
     * @image html ui-baselayer-blur-textured.png width=256px
     * Default @ref BaseLayerSharedFlag::Textured behavior
     * @m_enddiv
     *
     * @m_div{m-col-m-6 m-text-center m-nopadt m-nopadx}
     * @image html ui-baselayer-blur-textured-mask.png width=256px
     * With @ref BaseLayerSharedFlag::TextureMask
     * @m_enddiv
     *
     * @endparblock
     *
     * @see @ref BaseLayerSharedFlag::BackgroundBlur,
     *      @ref BaseLayerCommonStyleUniform::backgroundBlurAlpha
     */
    TextureMask = Textured|(1 << 4),

    /**
     * Render the quads subdivided into 9 quads, where each contains either a
     * corner, an edge with an outline or the inside. This significantly
     * simplifies the fragment shader at the cost of uploading much more data
     * to the GPU. May improve rendering speed on low performance GPUs but the
     * extra bandwidth may have a negative effect on GPUs where the rendering
     * wasn't bottlenecked by fragment shading.
     *
     * Mutually exclusive with the @ref BaseLayerSharedFlag::NoRoundedCorners
     * and @relativeref{BaseLayerSharedFlag,NoOutline} optimizations.
     */
    SubdividedQuads = 1 << 5,
};

/**
@brief Base layer shared state flag
@m_since_latest

@see @ref BaseLayer::Shared::Configuration::setFlags(),
    @ref BaseLayer::Shared::flags()
*/
typedef Containers::EnumSet<BaseLayerSharedFlag> BaseLayerSharedFlags;

CORRADE_ENUMSET_OPERATORS(BaseLayerSharedFlags)

/**
@debugoperatorenum{BaseLayerSharedFlag}
@m_since_latest
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, BaseLayerSharedFlag value);

/**
@debugoperatorenum{BaseLayerSharedFlags}
@m_since_latest
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, BaseLayerSharedFlags value);

/**
@brief Shared state for the base layer

Contains style data. You'll most likely instantiate the class through
@ref BaseLayerGL::Shared. In order to update or draw the layer it's expected
that @ref setStyle() was called.
*/
class MAGNUM_UI_EXPORT BaseLayer::Shared: public AbstractVisualLayer::Shared {
    public:
        class Configuration;

        /**
         * @brief Style uniform count
         *
         * Size of the style uniform buffer excluding dynamic styles. May or
         * may not be the same as @ref styleCount(). For dynamic styles, the
         * style uniform count is the same as @ref dynamicStyleCount().
         * @see @ref Configuration::Configuration(UnsignedInt, UnsignedInt),
         *      @ref setStyle()
         */
        UnsignedInt styleUniformCount() const;

        /** @brief Flags */
        BaseLayerSharedFlags flags() const;

        /**
         * @brief Set style data with implicit mapping between styles and uniforms
         * @param commonUniform Common style uniform data
         * @param uniforms      Style uniforms
         * @param paddings      Padding inside the node in order left, top,
         *      right, bottom corresponding to style uniforms
         * @return Reference to self (for method chaining)
         *
         * The @p uniforms view is expected to have the same size as
         * @ref styleUniformCount(). The @p paddings view is expected to either
         * have the same size as @ref styleCount() or be empty, in which case
         * all paddings are implicitly zero.
         *
         * Can only be called if @ref styleUniformCount() and @ref styleCount()
         * were set to the same value in @ref Configuration passed to the
         * constructor, otherwise you have to additionally provide a mapping
         * from styles to uniforms using
         * @ref setStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const Vector4>&)
         * instead.
         *
         * Calling this function causes @ref LayerState::NeedsDataUpdate to be
         * set on all layers that are constructed using this shared instance.
         * If @ref dynamicStyleCount() is non-zero,
         * @ref LayerState::NeedsCommonDataUpdate is set as well to trigger an
         * upload of changed dynamic style uniform data.
         */
        Shared& setStyle(const BaseLayerCommonStyleUniform& commonUniform, Containers::ArrayView<const BaseLayerStyleUniform> uniforms, const Containers::StridedArrayView1D<const Vector4>& paddings);
        /** @overload */
        Shared& setStyle(const BaseLayerCommonStyleUniform& commonUniform, std::initializer_list<BaseLayerStyleUniform> uniforms, std::initializer_list<Vector4> paddings);

        /**
         * @brief Set style data
         * @param commonUniform     Common style uniform data
         * @param uniforms          Style uniforms
         * @param styleToUniform    Style to style uniform mapping
         * @param stylePaddings     Per-style padding inside the node in order
         *      left, top, right, bottom
         * @return Reference to self (for method chaining)
         *
         * The @p uniforms view is expected to have the same size as
         * @ref styleUniformCount(), the @p styleToUniform view the same size
         * as @ref styleCount(). All uniform indices are expected to be less
         * than @ref styleUniformCount().
         *
         * The @p stylePaddings view is expected to either have the same size
         * as @ref styleCount() or be empty, in which case all paddings are
         * implicitly zero.
         *
         * Value of @cpp styleToUniform[i] @ce should give back an index into
         * the @p uniforms array for style @cpp i @ce. If
         * @ref styleUniformCount() and @ref styleCount() is the same and the
         * mapping is implicit, you can use the
         * @ref setStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>, const Containers::StridedArrayView1D<const Vector4>&)
         * convenience overload instead.
         *
         * Calling this function causes @ref LayerState::NeedsDataUpdate to be
         * set on all layers that are constructed using this shared instance.
         * If @ref dynamicStyleCount() is non-zero,
         * @ref LayerState::NeedsCommonDataUpdate is set as well to trigger an
         * upload of changed dynamic style uniform data.
         */
        Shared& setStyle(const BaseLayerCommonStyleUniform& commonUniform, Containers::ArrayView<const BaseLayerStyleUniform> uniforms, const Containers::StridedArrayView1D<const UnsignedInt>& styleToUniform, const Containers::StridedArrayView1D<const Vector4>& stylePaddings);
        /** @overload */
        Shared& setStyle(const BaseLayerCommonStyleUniform& commonUniform, std::initializer_list<BaseLayerStyleUniform> uniforms, std::initializer_list<UnsignedInt> styleToUniform, std::initializer_list<Vector4> stylePaddings);

        /* Overloads to remove a WTF factor from method chaining order */
        #ifndef DOXYGEN_GENERATING_OUTPUT
        MAGNUMEXTRAS_UI_ABSTRACTVISUALLAYER_SHARED_SUBCLASS_IMPLEMENTATION()
        #endif

    #ifdef DOXYGEN_GENERATING_OUTPUT
    private:
    #else
    protected:
    #endif
        struct State;
        friend BaseLayer;
        friend BaseLayerStyleAnimator;

        MAGNUM_UI_LOCAL explicit Shared(Containers::Pointer<State>&& state);
        /* Used by tests to avoid having to include / allocate the state */
        explicit Shared(const Configuration& configuration);
        /* Can't be MAGNUM_UI_LOCAL, used by tests */
        explicit Shared(NoCreateT) noexcept;

    private:
        MAGNUM_UI_LOCAL void setStyleInternal(const BaseLayerCommonStyleUniform& commonUniform, Containers::ArrayView<const BaseLayerStyleUniform> styleUniforms, const Containers::StridedArrayView1D<const Vector4>& stylePaddings);

        /* The items are guaranteed to have the same size as
           styleUniformCount(). Called only if there are no dynamic styles,
           otherwise the data are copied to internal arrays to be subsequently
           combined with dynamic uniforms and uploaded together in doDraw(). */
        virtual void doSetStyle(const BaseLayerCommonStyleUniform& commonUniform, Containers::ArrayView<const BaseLayerStyleUniform> uniforms) = 0;
};

/**
@brief Configuration of a base layer shared state

@see @ref BaseLayerGL::Shared::Shared(const Configuration&)
*/
class MAGNUM_UI_EXPORT BaseLayer::Shared::Configuration {
    public:
        /**
         * @brief Constructor
         *
         * The @p styleUniformCount parameter specifies the size of the uniform
         * array, @p styleCount then the number of distinct styles to use for
         * drawing. The sizes are independent in order to allow styles with
         * different paddings share the same uniform data. Either both
         * @p styleUniformCount and @p styleCount is expected to be non-zero,
         * or both zero with a non-zero dynamic style count specified with
         * @ref setDynamicStyleCount(). Style data are then set with
         * @ref setStyle(), dynamic style data then with
         * @ref BaseLayer::setDynamicStyle().
         */
        explicit Configuration(UnsignedInt styleUniformCount, UnsignedInt styleCount);

        /**
         * @brief Construct with style uniform count being the same as style count
         *
         * Equivalent to calling @ref Configuration(UnsignedInt, UnsignedInt)
         * with both parameters set to @p styleCount.
         */
        explicit Configuration(UnsignedInt styleCount): Configuration{styleCount, styleCount} {}

        /** @brief Style uniform count */
        UnsignedInt styleUniformCount() const { return _styleUniformCount; }

        /** @brief Style count */
        UnsignedInt styleCount() const { return _styleCount; }

        /** @brief Dynamic style count */
        UnsignedInt dynamicStyleCount() const { return _dynamicStyleCount; }

        /**
         * @brief Set dynamic style count
         * @return Reference to self (for method chaining)
         *
         * Initial count is @cpp 0 @ce.
         * @see @ref Configuration(UnsignedInt, UnsignedInt),
         *      @ref AbstractVisualLayer::allocateDynamicStyle(),
         *      @ref AbstractVisualLayer::recycleDynamicStyle(),
         *      @ref AbstractVisualLayer::dynamicStyleUsedCount()
         */
        Configuration& setDynamicStyleCount(UnsignedInt count) {
            _dynamicStyleCount = count;
            return *this;
        }

        /** @brief Flags */
        BaseLayerSharedFlags flags() const { return _flags; }

        /**
         * @brief Set flags
         * @return Reference to self (for method chaining)
         *
         * By default no flags are set.
         * @see @ref addFlags(), @ref clearFlags()
         */
        Configuration& setFlags(BaseLayerSharedFlags flags) {
            _flags = flags;
            return *this;
        }

        /**
         * @brief Add flags
         * @return Reference to self (for method chaining)
         *
         * Calls @ref setFlags() with the existing flags ORed with @p flags.
         * Useful for preserving previously set flags.
         * @see @ref clearFlags()
         */
        Configuration& addFlags(BaseLayerSharedFlags flags) {
            return setFlags(_flags|flags);
        }

        /**
         * @brief Clear flags
         * @return Reference to self (for method chaining)
         *
         * Calls @ref setFlags() with the existing flags ANDed with the inverse
         * of @p flags. Useful for removing a subset of previously set flags.
         * @see @ref addFlags()
         */
        Configuration& clearFlags(BaseLayerSharedFlags flags) {
            return setFlags(_flags & ~flags);
        }

        /** @brief Background blur radius */
        UnsignedInt backgroundBlurRadius() const {
            return _backgroundBlurRadius;
        }

        /** @brief Background blur sampling cutoff */
        Float backgroundBlurCutoff() const { return _backgroundBlurCutoff; }

        /**
         * @brief Set background blur radius and sampling cutoff
         * @param radius    Gaussian blur radius
         * @param cutoff    Cutoff for weight values after which no sampling is
         *      done
         * @return Reference to self (for method chaining)
         *
         * Expects that the @p radius is less than @cpp 31 @ce, value of
         * @cpp 0 @ce makes the blur sample only the center pixel, effectively
         * not blurring anything. The @p cutoff value controls a balance
         * between speed and precision, @cpp 0.0f @ce samples all pixels in
         * given radius even though they may have a weight that doesn't result
         * in any contribution to the quantized output, higher values gradually
         * reduce the number of sampled pixels down to just one.
         *
         * While the radius is a static setting that has to be set upfront,
         * there's also pass count that can be changed dynamically at runtime.
         * See @ref BaseLayer::setBackgroundBlurPassCount() for more
         * information and details about how it interacts with blur radius.
         *
         * Initial @p radius is @cpp 4 @ce and @p cutoff is
         * @cpp 0.5f/255.0f @ce, i.e. weights that don't contribute any value
         * even when combined from both sides of the blur circle for a 8bpp
         * render target are ignored.
         */
        Configuration& setBackgroundBlurRadius(UnsignedInt radius, Float cutoff = 0.5f/255.0f);

    private:
        UnsignedInt _styleUniformCount, _styleCount;
        UnsignedInt _dynamicStyleCount = 0;
        BaseLayerSharedFlags _flags;
        UnsignedInt _backgroundBlurRadius = 4;
        Float _backgroundBlurCutoff = 0.5f/255.0f;
};

inline BaseLayer::Shared& BaseLayer::shared() {
    return static_cast<Shared&>(AbstractVisualLayer::shared());
}

inline const BaseLayer::Shared& BaseLayer::shared() const {
    return static_cast<const Shared&>(AbstractVisualLayer::shared());
}

}}

#endif
