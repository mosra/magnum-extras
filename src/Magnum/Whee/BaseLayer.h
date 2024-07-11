#ifndef Magnum_Whee_BaseLayer_h
#define Magnum_Whee_BaseLayer_h
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
 * @brief Class @ref Magnum::Whee::BaseLayer, struct @ref Magnum::Whee::BaseLayerCommonStyleUniform, @ref Magnum::Whee::BaseLayerStyleUniform
 * @m_since_latest
 */

#include <Magnum/Math/Color.h>

#include "Magnum/Whee/AbstractVisualLayer.h"

namespace Magnum { namespace Whee {

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
    constexpr explicit BaseLayerCommonStyleUniform(DefaultInitT = DefaultInit) noexcept: smoothness{0.0f}, innerOutlineSmoothness{0.0f} {}

    /** @brief Constructor */
    constexpr /*implicit*/ BaseLayerCommonStyleUniform(Float smoothness, Float innerOutlineSmoothness): smoothness{smoothness}, innerOutlineSmoothness{innerOutlineSmoothness} {}

    /** @brief Construct with the @ref smoothness and @ref innerOutlineSmoothness fields set to the same value */
    constexpr /*implicit*/ BaseLayerCommonStyleUniform(Float smoothness): BaseLayerCommonStyleUniform{smoothness, smoothness} {}

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
    BaseLayerCommonStyleUniform& setSmoothness(Float smoothness, Float innerOutlineSmoothness) {
        this->smoothness = smoothness;
        this->innerOutlineSmoothness = innerOutlineSmoothness;
        return *this;
    }

    /**
     * @brief Set the @ref smoothness and @ref innerOutlineSmoothness fields to the same value
     * @return Reference to self (for method chaining)
     */
    BaseLayerCommonStyleUniform& setSmoothness(Float smoothness) {
        this->smoothness = smoothness;
        this->innerOutlineSmoothness = smoothness;
        return *this;
    }

    /**
     * @}
     */

    /**
     * @brief Edge smoothness radius
     *
     * In layout units, i.e. setting the value to @cpp 1.0f @ce will make the
     * smoothing extend 1 layout unit on each side of the edge. Default value
     * is @cpp 0.0f @ce.
     */
    Float smoothness;

    /**
     * @brief Inner outline edge smoothness radius
     *
     * In layout units, i.e. setting the value to @cpp 1.0f @ce will make the
     * smoothing extend 1 layout unit on each side of the edge. Default value
     * is @cpp 0.0f @ce.
     */
    Float innerOutlineSmoothness;

    #ifndef DOXYGEN_GENERATING_OUTPUT
    Int:32;
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
    BaseLayerStyleUniform& setColor(const Color4& top, const Color4& bottom) {
        topColor = top;
        bottomColor = bottom;
        return *this;
    }

    /**
     * @brief Set the @ref topColor and @ref bottomColor fields to the same value
     * @return Reference to self (for method chaining)
     */
    BaseLayerStyleUniform& setColor(const Color4& color) {
        topColor = color;
        bottomColor = color;
        return *this;
    }

    /**
     * @brief Set the @ref outlineColor field
     * @return Reference to self (for method chaining)
     */
    BaseLayerStyleUniform& setOutlineColor(const Color4& color) {
        outlineColor = color;
        return *this;
    }

    /**
     * @brief Set the @ref outlineWidth field
     * @return Reference to self (for method chaining)
     */
    BaseLayerStyleUniform& setOutlineWidth(const Vector4& width) {
        outlineWidth = width;
        return *this;
    }

    /**
     * @brief Set the @ref outlineWidth field with all edges having the same value
     * @return Reference to self (for method chaining)
     */
    BaseLayerStyleUniform& setOutlineWidth(Float width) {
        outlineWidth = Vector4{width};
        return *this;
    }

    /**
     * @brief Set the @ref cornerRadius field
     * @return Reference to self (for method chaining)
     */
    BaseLayerStyleUniform& setCornerRadius(const Vector4& radius) {
        cornerRadius = radius;
        return *this;
    }

    /**
     * @brief Set the @ref cornerRadius field with all corners having the same value
     * @return Reference to self (for method chaining)
     */
    BaseLayerStyleUniform& setCornerRadius(Float radius) {
        cornerRadius = Vector4{radius};
        return *this;
    }

    /**
     * @brief Set the @ref innerOutlineCornerRadius field
     * @return Reference to self (for method chaining)
     */
    BaseLayerStyleUniform& setInnerOutlineCornerRadius(const Vector4& radius) {
        innerOutlineCornerRadius = radius;
        return *this;
    }

    /**
     * @brief Set the @ref innerOutlineCornerRadius field with all corners having the same value
     * @return Reference to self (for method chaining)
     */
    BaseLayerStyleUniform& setInnerOutlineCornerRadius(Float radius) {
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
     * show.
     */
    Color4 outlineColor;

    /**
     * @brief Outline width
     *
     * In order left, top, right, bottom. Default value is @cpp 0.0f @ce for
     * all sides.
     */
    Vector4 outlineWidth;

    /**
     * @brief Corner radius
     *
     * In order top left, bottom left, top right, bottom right. Default value
     * is @cpp 0.0f @ce for all sides.
     */
    Vector4 cornerRadius;

    /**
     * @brief Inner outline corner radius
     *
     * In order top left, bottom left, top right, bottom right. Default value
     * is @cpp 0.0f @ce for all sides.
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
    @ref UserInterface::setBaseLayerInstance(), @ref StyleFeature::BaseLayer
*/
class MAGNUM_WHEE_EXPORT BaseLayer: public AbstractVisualLayer {
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
         * @brief Create a quad
         * @param style         Style index
         * @param node          Node to attach to
         * @return New data handle
         *
         * Expects that @p style is less than @ref Shared::styleCount(). All
         * styling is driven from the @ref BaseLayerStyleUniform at index
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
         * Expects that @p style is less than @ref Shared::styleCount().
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
         * Expects that @p style is less than @ref Shared::styleCount().
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
         * Calling this function causes @ref LayerState::NeedsUpdate to be set.
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
         * zero vector, i.e. not affecting the style in any way.
         *
         * Calling this function causes @ref LayerState::NeedsUpdate to be set.
         * @see @ref isHandleValid(DataHandle) const
         */
        void setOutlineWidth(DataHandle handle, const Vector4& width);

        /**
         * @brief Set quad custom outline width with all edges having the same value
         *
         * Expects that @p handle is valid. The @p width is added to
         * @ref BaseLayerStyleUniform::outlineWidth. By default, unless
         * specified in @ref create() already, the custom outline width is
         * zero, i.e. not affecting the style in any way.
         *
         * Calling this function causes @ref LayerState::NeedsUpdate to be set.
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
         * Calling this function causes @ref LayerState::NeedsUpdate to be set.
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
         * Calling this function causes @ref LayerState::NeedsUpdate to be set.
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

    #ifdef DOXYGEN_GENERATING_OUTPUT
    private:
    #else
    protected:
    #endif
        struct State;

        MAGNUM_WHEE_LOCAL explicit BaseLayer(LayerHandle handle, Containers::Pointer<State>&& state);
        /* Used by tests to avoid having to include / allocate the state */
        explicit BaseLayer(LayerHandle handle, Shared& shared);

        /* These can't be MAGNUM_WHEE_LOCAL otherwise deriving from this class
           in tests causes linker errors */

        /* Advertises LayerFeature::Draw but *does not* implement doDraw(),
           that's on the subclass */
        LayerFeatures doFeatures() const override;

        void doUpdate(const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, Containers::BitArrayView nodesEnabled, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes) override;

    private:
        MAGNUM_WHEE_LOCAL void setColorInternal(UnsignedInt id, const Color3& color);
        MAGNUM_WHEE_LOCAL void setOutlineWidthInternal(UnsignedInt id, const Vector4& width);
        MAGNUM_WHEE_LOCAL void setPaddingInternal(UnsignedInt id, const Vector4& padding);
};

/**
@brief Shared state for the base layer

Contains style data. You'll most likely instantiate the class through
@ref BaseLayerGL::Shared. In order to update or draw the layer it's expected
that @ref setStyle() was called.
*/
class MAGNUM_WHEE_EXPORT BaseLayer::Shared: public AbstractVisualLayer::Shared {
    public:
        class Configuration;

        /**
         * @brief Style uniform count
         *
         * Size of the style uniform buffer. May or may not be the same as
         * @ref styleCount().
         * @see @ref Configuration::Configuration(UnsignedInt, UnsignedInt),
         *      @ref setStyle()
         */
        UnsignedInt styleUniformCount() const;

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
         * as @ref styleCount(). The @p stylePaddings view is expected to
         * either have the same size as @ref styleCount() or be empty, in which
         * case all paddings are implicitly zero.
         *
         * Value of @cpp styleToUniform[i] @ce should give back an index into
         * the @p uniforms array for style @cpp i @ce. If
         * @ref styleUniformCount() and @ref styleCount() is the same and the
         * mapping is implicit, you can use the
         * @ref setStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>, const Containers::StridedArrayView1D<const Vector4>&)
         * convenience overload instead.
         */
        Shared& setStyle(const BaseLayerCommonStyleUniform& commonUniform, Containers::ArrayView<const BaseLayerStyleUniform> uniforms, const Containers::StridedArrayView1D<const UnsignedInt>& styleToUniform, const Containers::StridedArrayView1D<const Vector4>& stylePaddings);
        /** @overload */
        Shared& setStyle(const BaseLayerCommonStyleUniform& commonUniform, std::initializer_list<BaseLayerStyleUniform> uniforms, std::initializer_list<UnsignedInt> styleToUniform, std::initializer_list<Vector4> stylePaddings);

        /* Overloads to remove a WTF factor from method chaining order */
        #ifndef DOXYGEN_GENERATING_OUTPUT
        MAGNUMEXTRAS_WHEE_ABSTRACTVISUALLAYER_SHARED_SUBCLASS_IMPLEMENTATION()
        #endif

    #ifdef DOXYGEN_GENERATING_OUTPUT
    private:
    #else
    protected:
    #endif
        struct State;
        friend BaseLayer;

        MAGNUM_WHEE_LOCAL explicit Shared(Containers::Pointer<State>&& state);
        /* Used by tests to avoid having to include / allocate the state */
        explicit Shared(const Configuration& configuration);
        /* Can't be MAGNUM_WHEE_LOCAL, used by tests */
        explicit Shared(NoCreateT) noexcept;

    private:
        MAGNUM_WHEE_LOCAL void setStyleInternal(const BaseLayerCommonStyleUniform& commonUniform, Containers::ArrayView<const BaseLayerStyleUniform> styleUniforms, const Containers::StridedArrayView1D<const Vector4>& stylePaddings);

        /* The items are guaranteed to have the same size as
           styleUniformCount() */
        virtual void doSetStyle(const BaseLayerCommonStyleUniform& commonUniform, Containers::ArrayView<const BaseLayerStyleUniform> uniforms) = 0;
};

/**
@brief Configuration of a base layer shared state

@see @ref BaseLayerGL::Shared::Shared(const Configuration&)
*/
class MAGNUM_WHEE_EXPORT BaseLayer::Shared::Configuration {
    public:
        /**
         * @brief Constructor
         *
         * The @p styleUniformCount parameter specifies the size of the uniform
         * array, @p styleCount then the number of distinct styles to use for
         * drawing. The sizes are independent in order to allow styles with
         * different paddings share the same uniform data. Both
         * @p styleUniformCount and @p styleCount is expected to be non-zero.
         * Style data are then set with @ref setStyle().
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

    private:
        UnsignedInt _styleUniformCount, _styleCount;
};

inline BaseLayer::Shared& BaseLayer::shared() {
    return static_cast<Shared&>(AbstractVisualLayer::shared());
}

inline const BaseLayer::Shared& BaseLayer::shared() const {
    return static_cast<const Shared&>(AbstractVisualLayer::shared());
}

}}

#endif
