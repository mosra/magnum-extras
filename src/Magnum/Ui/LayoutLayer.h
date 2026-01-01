#ifndef Magnum_Ui_LayoutLayer_h
#define Magnum_Ui_LayoutLayer_h
/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023, 2024, 2025, 2026
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
 * @brief Class @ref Magnum::Ui::LayoutLayer
 * @m_since_latest_{extras}
 */

#include <initializer_list>

#include "Magnum/Ui/AbstractLayer.h"

namespace Magnum { namespace Ui {

/**
@brief Layout properties layer
@m_since_latest_{extras}

@section Ui-LayoutLayer-debug-integration Debug layer integration

When using @ref Ui-DebugLayer-node-inspect "DebugLayer node inspect" and
@ref DebugLayerSource::NodeDataDetails is enabled, passing this layer to
@ref DebugLayer::setLayerName(const T&, const Containers::StringView&) "DebugLayer::setLayerName()"
will make it list style assignments of all data. For example:

@include ui-debuglayer-layoutlayer.ansi

By default only style IDs are shown, but the @ref DebugIntegration constructor
accepts an optional function to map them from an @relativeref{Magnum,UnsignedInt}
to a @relativeref{Corrade,Containers::StringView}. Assuming a `LayoutStyle`
@cpp enum @ce is used by given layer, the function could look like this,
resulting in the output below:

@snippet ui-debuglayer.cpp layoutlayer-style-names

@include ui-debuglayer-layoutlayer-style-names.ansi

<b></b>

@m_class{m-note m-success}

@par
    Names and IDs of styles used by builtin widgets are considered an
    implementation detail and thus aren't exposed, but they're included in the
    @ref magnum-ui-gallery "magnum-ui-gallery" `--debug` output if you want to
    have a peek.
*/
class MAGNUM_UI_EXPORT LayoutLayer: public AbstractLayer {
    public:
        class DebugIntegration;

        /**
         * @brief Constructor
         * @param handle        Layer handle returned from
         *      @ref AbstractUserInterface::createLayer()
         * @param styleCount    Number of distinct styles to use
         *
         * Expects that @p styleCount is not @cpp 0 @ce. In order to update the
         * UI containing this layer it's expected that @ref setStyle(const Containers::StridedArrayView1D<const Vector2>& minSizes, const Containers::StridedArrayView1D<const Vector2>& maxSizes, const Containers::StridedArrayView1D<const Float>& aspectRatios, const Containers::StridedArrayView1D<const Vector4>& paddings, const Containers::StridedArrayView1D<const Vector4>& margins) "setStyle()"
         * was called.
         */
        explicit LayoutLayer(LayerHandle handle, UnsignedInt styleCount);

        /** @brief Copying is not allowed */
        LayoutLayer(const LayoutLayer&) = delete;

        /**
         * @brief Move constructor
         *
         * Performs a destructive move, i.e. the original object isn't usable
         * afterwards anymore.
         */
        LayoutLayer(LayoutLayer&&) noexcept;

        ~LayoutLayer();

        /** @brief Copying is not allowed */
        LayoutLayer& operator=(const LayoutLayer&) = delete;

        /** @brief Move assignment */
        LayoutLayer& operator=(LayoutLayer&&) noexcept;

        /** @brief Style count */
        UnsignedInt styleCount() const;

        /**
         * @brief Minimal node size for each style
         *
         * Expects that @ref setStyle(const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Float>&, const Containers::StridedArrayView1D<const Vector4>&, const Containers::StridedArrayView1D<const Vector4>&) "setStyle()"
         * was called.
         */
        Containers::StridedArrayView1D<const Vector2> styleMinSizes() const;

        /**
         * @brief Maximal node size for each style
         *
         * Expects that @ref setStyle(const Containers::StridedArrayView1D<const Vector2>& minSizes, const Containers::StridedArrayView1D<const Vector2>& maxSizes, const Containers::StridedArrayView1D<const Float>& aspectRatios, const Containers::StridedArrayView1D<const Vector4>& paddings, const Containers::StridedArrayView1D<const Vector4>& margins) "setStyle()"
         * was called.
         */
        Containers::StridedArrayView1D<const Vector2> styleMaxSizes() const;

        /**
         * @brief Node aspect ratio for each style
         *
         * Expects that @ref setStyle(const Containers::StridedArrayView1D<const Vector2>& minSizes, const Containers::StridedArrayView1D<const Vector2>& maxSizes, const Containers::StridedArrayView1D<const Float>& aspectRatios, const Containers::StridedArrayView1D<const Vector4>& paddings, const Containers::StridedArrayView1D<const Vector4>& margins) "setStyle()"
         * was called.
         */
        Containers::StridedArrayView1D<const Float> styleAspectRatios() const;

        /**
         * @brief Node padding for each style
         *
         * Expects that @ref setStyle(const Containers::StridedArrayView1D<const Vector2>& minSizes, const Containers::StridedArrayView1D<const Vector2>& maxSizes, const Containers::StridedArrayView1D<const Float>& aspectRatios, const Containers::StridedArrayView1D<const Vector4>& paddings, const Containers::StridedArrayView1D<const Vector4>& margins) "setStyle()"
         * was called.
         */
        Containers::StridedArrayView1D<const Vector4> stylePaddings() const;

        /**
         * @brief Node margin for each style
         *
         * Expects that @ref setStyle(const Containers::StridedArrayView1D<const Vector2>& minSizes, const Containers::StridedArrayView1D<const Vector2>& maxSizes, const Containers::StridedArrayView1D<const Float>& aspectRatios, const Containers::StridedArrayView1D<const Vector4>& paddings, const Containers::StridedArrayView1D<const Vector4>& margins) "setStyle()"
         * was called.
         */
        Containers::StridedArrayView1D<const Vector4> styleMargins() const;

        /**
         * @brief Set style data
         * @param minSizes      Minimal node size for each style, or an empty
         *      view for no style-specific minimal node sizes
         * @param maxSizes      Maximal node size for each style, or an empty
         *      view for no style-specific maximal node sizes
         * @param aspectRatios  Node aspect ratio for each style, or an empty
         *      view for no style-specific node aspect ratios
         * @param paddings      Padding inside a node for child node placement
         *      for each style, in order left, top, right, bottom, or an empty
         *      view for no style-specific node padding
         * @param margins       Margin outside nodes for placement within
         *      parents and next to neighbor nodes for each style, in order
         *      left, top, right, bottom, or an empty view for no
         *      style-specific node margin
         *
         * All views are expected to either have the same size as
         * @ref styleCount(), or be empty, in which case a default value will
         * be used. If all views are empty, default values will be set for all
         * properties. For a data with a particular style attached to a
         * particular node, the properties are interpreted as follows:
         *
         * -    A *max* of the value coming from @p minSizes for given style
         *      and min sizes reported by any other data attached to the same
         *      node is used as a minimal node size. Specifying a min size of
         *      @cpp 0.0f @ce in either axis doesn't affect the final node min
         *      size in given axis in any way. The defaults if no @p minSizes
         *      are specified are all @cpp 0.0f @ce.
         * -    A *min* of the value coming from @p maxSizes for given style
         *      and min sizes reported by any other data attached to the same
         *      node is used as a maximal node size. Specifying a max size of
         *      @ref Constants::inf() in either axis doesn't affect the final
         *      node max size in given axis in any way. The defaults if no
         *      @p maxSizes are specified are all @ref Constants::inf().
         * -    A first non-zero value coming from @p aspectRatios for given
         *      style and aspect ratios reported by any other data attached to
         *      the same node is used as a node aspect ratio. Specifying an
         *      aspect ratio of @cpp 0.0f @ce doesn't affect the final node
         *      aspect ratio in any way. The defaults if no @p aspectRatios are
         *      specified are all @cpp 0.0f @ce.
         * -    A *max* of the value coming from @p paddings for given style
         *      and paddings reported by any other data attached to the same
         *      node is used as a node padding. Specifying a padding of
         *      @cpp 0.0f @ce at given edge doesn't affect the final node
         *      padding at given edge in any way. The defaults if no
         *      @p paddings are specified are all @cpp 0.0f @ce.
         * -    A *max* of the value coming from @p margins for given style and
         *      margins reported by any other data attached to the same node is
         *      used as a node margin. Specifying a margin of @cpp 0.0f @ce at
         *      given edge doesn't affect the final node margin at given edge
         *      in any way. The defaults if no @p margins are specified are all
         *      @cpp 0.0f @ce.
         *
         * Calling this function causes @ref LayerState::NeedsLayoutUpdate to
         * be set.
         */
        void setStyle(const Containers::StridedArrayView1D<const Vector2>& minSizes, const Containers::StridedArrayView1D<const Vector2>& maxSizes, const Containers::StridedArrayView1D<const Float>& aspectRatios, const Containers::StridedArrayView1D<const Vector4>& paddings, const Containers::StridedArrayView1D<const Vector4>& margins);
        /** @overload */
        void setStyle(std::initializer_list<Vector2> minSizes, std::initializer_list<Vector2> maxSizes, std::initializer_list<Float> aspectRatios, std::initializer_list<Vector4> paddings, std::initializer_list<Vector4> margins);

        /**
         * @brief Create a data with given style index
         * @param style         Style index
         * @param node          Node to attach to
         * @return New data handle
         *
         * Expects that @p style is less than @ref styleCount(). It is allowed
         * to call this function before any styles are set with
         * @ref setStyle(const Containers::StridedArrayView1D<const Vector2>& minSizes, const Containers::StridedArrayView1D<const Vector2>& maxSizes, const Containers::StridedArrayView1D<const Float>& aspectRatios, const Containers::StridedArrayView1D<const Vector4>& paddings, const Containers::StridedArrayView1D<const Vector4>& margins) "setStyle()".
         * Once a style is set, layout properties are driven from given style.
         * There is currently no way to customize the layout properties on a
         * per-data basis.
         *
         * Delegates to @ref AbstractLayer::create(), see its documentation for
         * more information.
         */
        DataHandle create(UnsignedInt style, NodeHandle node =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            NodeHandle::Null
            #else
            NodeHandle{} /* To not have to include Handle.h */
            #endif
        );

        /**
         * @brief Create a data with given style index in a concrete enum type
         *
         * Casts @p style to @relativeref{Magnum,UnsignedInt} and delegates to
         * @ref create(UnsignedInt, NodeHandle).
         */
        template<class StyleIndex
            #ifndef DOXYGEN_GENERATING_OUTPUT
            /* Accept any enum except NodeHandle to prevent create(node) from
               being called by mistake */
            , typename std::enable_if<std::is_enum<StyleIndex>::value && !std::is_same<StyleIndex, NodeHandle>::value, int>::type = 0
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
         * @brief Remove a data
         *
         * Delegates to @ref AbstractLayer::remove(DataHandle).
         */
        void remove(DataHandle handle) {
            AbstractLayer::remove(handle);
        }

        /**
         * @brief Remove a data assuming it belongs to this layer
         *
         * Delegates to @ref AbstractLayer::remove(LayerDataHandle).
         */
        void remove(LayerDataHandle handle) {
            AbstractLayer::remove(handle);
        }

        /**
         * @brief Type-erased data style index
         *
         * Expects that @p handle is valid. The index is guaranteed to be less
         * than @ref styleCount().
         * @see @ref isHandleValid(DataHandle) const
         */
        UnsignedInt style(DataHandle handle) const;

        /**
         * @brief Data style index in a concrete enum type
         *
         * Expects that @p handle is valid. The index is guaranteed to be less
         * than @ref styleCount().
         * @see @ref isHandleValid(DataHandle) const
         */
        template<class StyleIndex> StyleIndex style(DataHandle handle) const {
            static_assert(std::is_enum<StyleIndex>::value, "expected an enum type");
            return StyleIndex(style(handle));
        }

        /**
         * @brief Type-erased data style index assuming it belongs to this layer
         *
         * Like @ref style(DataHandle) const but without checking that
         * @p handle indeed belongs to this layer. See its documentation for
         * more information.
         * @see @ref isHandleValid(LayerDataHandle) const
         */
        UnsignedInt style(LayerDataHandle handle) const;

        /**
         * @brief Data style index in a concrete enum type assuming it belongs to this layer
         *
         * Like @ref style(DataHandle) const but without checking that
         * @p handle indeed belongs to this layer. See its documentation for
         * more information.
         * @see @ref isHandleValid(LayerDataHandle) const
         */
        template<class StyleIndex> StyleIndex style(LayerDataHandle handle) const {
            static_assert(std::is_enum<StyleIndex>::value, "expected an enum type");
            return StyleIndex(style(handle));
        }

        /**
         * @brief Set data style index
         *
         * Expects that @p handle is valid and @p style is less than
         * @ref styleCount().
         *
         * If @p handle is attached to a non-null node, calling this function
         * causes @ref LayerState::NeedsLayoutUpdate to be set.
         * @see @ref isHandleValid(DataHandle) const
         */
        void setStyle(DataHandle handle, UnsignedInt style);

        /**
         * @brief Set data style index in a concrete enum type
         *
         * Casts @p style to @relativeref{Magnum,UnsignedInt} and delegates to
         * @ref setStyle(DataHandle, UnsignedInt).
         */
        template<class StyleIndex
            #ifndef DOXYGEN_GENERATING_OUTPUT
            , typename std::enable_if<std::is_enum<StyleIndex>::value, int>::type = 0
            #endif
        > void setStyle(DataHandle handle, StyleIndex style) {
            setStyle(handle, UnsignedInt(style));
        }

        /**
         * @brief Set data style index assuming it belongs to this layer
         *
         * Like @ref setStyle(DataHandle, UnsignedInt) but without checking
         * that @p handle indeed belongs to this layer. See its documentation
         * for more information.
         */
        void setStyle(LayerDataHandle handle, UnsignedInt style);

        /**
         * @brief Set data style index in a concrete enum type assuming it belongs to this layer
         *
         * Casts @p style to @relativeref{Magnum,UnsignedInt} and delegates to
         * @ref setStyle(LayerDataHandle, UnsignedInt).
         */
        template<class StyleIndex
            #ifndef DOXYGEN_GENERATING_OUTPUT
            , typename std::enable_if<std::is_enum<StyleIndex>::value, int>::type = 0
            #endif
        > void setStyle(LayerDataHandle handle, StyleIndex style) {
            setStyle(handle, UnsignedInt(style));
        }

    private:
        struct State;
        Containers::Pointer<State> _state;

        MAGNUM_UI_LOCAL void setStyleInternal(UnsignedInt id, UnsignedInt style);

        MAGNUM_UI_LOCAL LayerFeatures doFeatures() const override;
        MAGNUM_UI_LOCAL void doLayout(Containers::BitArrayView dataIdsToLayout, const Containers::StridedArrayView1D<Vector2>& nodeMinSizes, const Containers::StridedArrayView1D<Vector2>& nodeMaxSizes, const Containers::StridedArrayView1D<Float>& nodeAspectRatios, const Containers::StridedArrayView1D<Vector4>& nodePaddings, const Containers::StridedArrayView1D<Vector4>& nodeMargins) override;
};

/**
@brief Debug layer integration

Integrates the layer with @ref DebugLayer. See
@ref Ui-LayoutLayer-debug-integration "LayoutLayer debug layer integration" for
more information and example usage.
*/
class MAGNUM_UI_EXPORT LayoutLayer::DebugIntegration {
    public:
        /**
         * @brief Constructor
         *
         * The @p styleName function, if set, is used to retrieve names for
         * styles assigned to particular data. If it returns an empty string
         * for a particular style ID, it's treated as if the function wasn't
         * used for given style at all, showing just the ID alone. The @p style
         * passed to the function is guaranteed to be less than
         * @ref LayoutLayer::styleCount() for given layer.
         */
        /*implicit*/ DebugIntegration(Containers::StringView(*styleName)(UnsignedInt style) = nullptr): _styleName{styleName} {}
        #ifndef DOXYGEN_GENERATING_OUTPUT
        /* Needed because while passing a lambda directly to the constructor
           works, passing it to DebugLayer::setLayerName() doesn't, as there's
           too many conversions or some such. Sigh, C++. */
        template<class T, typename std::enable_if<std::is_convertible<T, Containers::StringView(*)(UnsignedInt)>::value, int>::type = 0> /*implicit*/ DebugIntegration(T styleName): DebugIntegration{static_cast<Containers::StringView(*)(UnsignedInt)>(styleName)} {}
        #endif

        #ifndef DOXYGEN_GENERATING_OUTPUT
        /* Used internally by DebugLayer, no point in documenting it here */
        void print(Debug& debug, const LayoutLayer& layer, const Containers::StringView& layerName, LayerDataHandle data);
        #endif

    private:
        Containers::StringView(*_styleName)(UnsignedInt);
};

}}

#endif
