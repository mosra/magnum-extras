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
 * @brief Class @ref Magnum::Whee::BaseLayer, struct @ref Magnum::Whee::BaseLayerStyleCommon, @ref Magnum::Whee::BaseLayerStyleItem
 * @m_since_latest
 */

#include <Magnum/Math/Color.h>

#include "Magnum/Whee/AbstractVisualLayer.h"

namespace Magnum { namespace Whee {

/**
@brief Common style properties for all @ref BaseLayer data
@m_since_latest

@see @ref BaseLayerStyleItem, @ref BaseLayerGL::Shared::setStyle()
*/
struct BaseLayerStyleCommon {
    /** @brief Construct with default values */
    constexpr explicit BaseLayerStyleCommon(DefaultInitT = DefaultInit) noexcept: smoothness{0.0f}, innerOutlineSmoothness{0.0f} {}

    /** @brief Construct without initializing the contents */
    explicit BaseLayerStyleCommon(NoInitT) noexcept {}

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
    BaseLayerStyleCommon& setSmoothness(Float smoothness, Float innerOutlineSmoothness) {
        this->smoothness = smoothness;
        this->innerOutlineSmoothness = innerOutlineSmoothness;
        return *this;
    }

    /**
     * @brief Set the @ref smoothness and @ref innerOutlineSmoothness fields to the same value
     * @return Reference to self (for method chaining)
     */
    BaseLayerStyleCommon& setSmoothness(Float smoothness) {
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
@brief Varying style properties for @ref BaseLayer data
@m_since_latest

@see @ref BaseLayerStyleCommon, @ref BaseLayerGL::Shared::setStyle()
*/
struct BaseLayerStyleItem {
    /** @brief Construct with default values */
    constexpr explicit BaseLayerStyleItem(DefaultInitT = DefaultInit) noexcept: topColor{1.0f}, bottomColor{1.0f}, outlineColor{1.0f}, outlineWidth{0.0f}, cornerRadius{0.0f}, innerOutlineCornerRadius{0.0f} {}

    /** @brief Construct without initializing the contents */
    explicit BaseLayerStyleItem(NoInitT) noexcept: topColor{NoInit}, bottomColor{NoInit}, outlineColor{NoInit}, outlineWidth{NoInit}, cornerRadius{NoInit}, innerOutlineCornerRadius{NoInit} {}

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
    BaseLayerStyleItem& setColor(const Color4& top, const Color4& bottom) {
        topColor = top;
        bottomColor = bottom;
        return *this;
    }

    /**
     * @brief Set the @ref topColor and @ref bottomColor fields to the same value
     * @return Reference to self (for method chaining)
     */
    BaseLayerStyleItem& setColor(const Color4& color) {
        topColor = color;
        bottomColor = color;
        return *this;
    }

    /**
     * @brief Set the @ref outlineColor field
     * @return Reference to self (for method chaining)
     */
    BaseLayerStyleItem& setOutlineColor(const Color4& color) {
        outlineColor = color;
        return *this;
    }

    /**
     * @brief Set the @ref outlineWidth field
     * @return Reference to self (for method chaining)
     */
    BaseLayerStyleItem& setOutlineWidth(const Vector4& width) {
        outlineWidth = width;
        return *this;
    }

    /**
     * @brief Set the @ref outlineWidth field with all edges having the same value
     * @return Reference to self (for method chaining)
     */
    BaseLayerStyleItem& setOutlineWidth(Float width) {
        outlineWidth = Vector4{width};
        return *this;
    }

    /**
     * @brief Set the @ref cornerRadius field
     * @return Reference to self (for method chaining)
     */
    BaseLayerStyleItem& setCornerRadius(const Vector4& radius) {
        cornerRadius = radius;
        return *this;
    }

    /**
     * @brief Set the @ref cornerRadius field with all corners having the same value
     * @return Reference to self (for method chaining)
     */
    BaseLayerStyleItem& setCornerRadius(Float radius) {
        cornerRadius = Vector4{radius};
        return *this;
    }

    /**
     * @brief Set the @ref innerOutlineCornerRadius field
     * @return Reference to self (for method chaining)
     */
    BaseLayerStyleItem& setInnerOutlineCornerRadius(const Vector4& radius) {
        innerOutlineCornerRadius = radius;
        return *this;
    }

    /**
     * @brief Set the @ref innerOutlineCornerRadius field with all corners having the same value
     * @return Reference to self (for method chaining)
     */
    BaseLayerStyleItem& setInnerOutlineCornerRadius(Float radius) {
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
     * Default value is @cpp 0xffffffff_srgbf @ce. Is multiplied with the
     * @ref topColor and @ref bottomColor gradient. Visible only if
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
*/
class MAGNUM_WHEE_EXPORT BaseLayer: public AbstractVisualLayer {
    public:
        class Shared;

        /**
         * @brief Create a quad
         * @param style         Style index
         * @param node          Node to attach to
         * @return New data handle
         *
         * Expects that @p style is less than @ref Shared::styleCount(). All
         * styling is driven from the @ref BaseLayerStyleItem at index
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
         * Styling is driven from the @ref BaseLayerStyleItem at index
         * @p style, in addition @ref BaseLayerStyleItem::topColor and
         * @relativeref{BaseLayerStyleItem,bottomColor} is multiplied with
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
         * Styling is driven from the @ref BaseLayerStyleItem at index
         * @p style, in addition @ref BaseLayerStyleItem::topColor and
         * @relativeref{BaseLayerStyleItem,bottomColor} is multiplied with
         * @p color and @p outlineWidth is added to
         * @ref BaseLayerStyleItem::outlineWidth.
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
         * Expects that @p handle is valid. @ref BaseLayerStyleItem::topColor
         * and @relativeref{BaseLayerStyleItem,bottomColor} is multiplied with
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
         * right, bottom and is added to @ref BaseLayerStyleItem::outlineWidth.
         * By default, unless specified in @ref create() already, the custom
         * outline width is a zero vector, i.e. not affecting the style in any
         * way.
         *
         * Calling this function causes @ref LayerState::NeedsUpdate to be set.
         * @see @ref isHandleValid(DataHandle) const
         */
        void setOutlineWidth(DataHandle handle, const Vector4& width);

        /**
         * @brief Set quad custom outline width with all edges having the same value
         *
         * Expects that @p handle is valid. The @p width is added to
         * @ref BaseLayerStyleItem::outlineWidth. By default, unless specified
         * in @ref create() already, the custom outline width is zero, i.e. not
         * affecting the style in any way.
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

    #ifdef DOXYGEN_GENERATING_OUTPUT
    private:
    #else
    protected:
    #endif
        struct State;

        MAGNUM_WHEE_LOCAL explicit BaseLayer(LayerHandle handle, Containers::Pointer<State>&& state);
        /* Used by tests to avoid having to include / allocate the state */
        explicit BaseLayer(LayerHandle handle, Shared& shared);

        /* Can't be MAGNUM_WHEE_LOCAL otherwise deriving from this class in
           tests causes linker errors */
        void doUpdate(const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes) override;

    private:
        MAGNUM_WHEE_LOCAL void setColorInternal(UnsignedInt id, const Color3& color);
        MAGNUM_WHEE_LOCAL void setOutlineWidthInternal(UnsignedInt id, const Vector4& width);

        /* Can't be MAGNUM_WHEE_LOCAL otherwise deriving from this class in
           tests causes linker errors */
        /* Advertises LayerFeature::Draw but *does not* implement doDraw(),
           that's on the subclass */
        LayerFeatures doFeatures() const override;
};

/**
@brief Shared state for the base layer

You'll most likely instantiate the class through @ref BaseLayerGL::Shared,
which contains a concrete OpenGL implementation.
*/
class MAGNUM_WHEE_EXPORT BaseLayer::Shared: public AbstractVisualLayer::Shared {
    #ifdef DOXYGEN_GENERATING_OUTPUT
    private:
    #else
    protected:
    #endif
        friend BaseLayer;

        MAGNUM_WHEE_LOCAL explicit Shared(Containers::Pointer<State>&& state);
        /* Used by tests to avoid having to include / allocate the state */
        explicit Shared(UnsignedInt styleCount);
        /* Can't be MAGNUM_WHEE_LOCAL, used by tests */
        explicit Shared(NoCreateT) noexcept;
};

}}

#endif
