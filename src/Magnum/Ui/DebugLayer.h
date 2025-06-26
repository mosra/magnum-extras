#ifndef Magnum_Ui_DebugLayer_h
#define Magnum_Ui_DebugLayer_h
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

/**
 * @brief Class @ref Magnum::Ui::DebugLayer, enum @ref Magnum::Ui::DebugLayerSource, @ref Magnum::Ui::DebugLayerFlag, enum set @ref Magnum::Ui::DebugLayerSources, @ref Magnum::Ui::DebugLayerFlags
 * @m_since_latest
 */

#include "Magnum/Ui/AbstractLayer.h"

namespace Magnum { namespace Ui {

/**
@brief Debug layer data source to track
@m_since_latest

@see @ref DebugLayerSources, @ref DebugLayer, @ref DebugLayer::sources()
*/
enum class DebugLayerSource: UnsignedShort {
    /**
     * Track created nodes and make it possible to name them using
     * @ref DebugLayer::setNodeName(). Subset of
     * @ref DebugLayerSource::NodeHierarchy and
     * @ref DebugLayerSource::NodeDataAttachments.
     */
    Nodes = 1 << 0,

    /**
     * Track created layers and make it possible to name them using
     * @ref DebugLayer::setLayerName(). Subset of
     * @ref DebugLayerSource::NodeDataAttachments.
     */
    Layers = 1 << 1,

    /**
     * Track node parent and child relations. Implies
     * @ref DebugLayerSource::Nodes.
     */
    NodeHierarchy = Nodes|(1 << 2),

    /**
     * Track per-node layer data attachments. Implies
     * @ref DebugLayerSource::Nodes and @ref DebugLayerSource::Layers.
     */
    NodeDataAttachments = Nodes|Layers|(1 << 3),
};

/**
@brief Debug layer data sources to track
@m_since_latest

@see @ref DebugLayer, @ref DebugLayer::sources()
*/
typedef Containers::EnumSet<DebugLayerSource> DebugLayerSources;

CORRADE_ENUMSET_OPERATORS(DebugLayerSources)

/**
@debugoperatorenum{DebugLayerSource}
@m_since_latest
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, DebugLayerSource value);

/**
@debugoperatorenum{DebugLayerSources}
@m_since_latest
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, DebugLayerSources value);

/**
@brief Debug layer flag
@m_since_latest

@see @ref DebugLayerFlags, @ref DebugLayer, @ref DebugLayer::flags()
*/
enum class DebugLayerFlag: UnsignedByte {
    /**
     * Highlight and show details of a node on pointer press. Expects that at
     * least @ref DebugLayerSource::Nodes is enabled.
     * @see @ref DebugLayer::setNodeHighlightColor(),
     *      @ref DebugLayer::setNodeHighlightGesture(),
     *      @ref DebugLayer::setNodeHighlightCallback()
     */
    NodeHighlight = 1 << 0,
};

/**
@brief Debug layer flags
@m_since_latest

@see @ref DebugLayer, @ref DebugLayer::flags()
*/
typedef Containers::EnumSet<DebugLayerFlag> DebugLayerFlags;

CORRADE_ENUMSET_OPERATORS(DebugLayerFlags)

/**
@debugoperatorenum{DebugLayerFlag}
@m_since_latest
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, DebugLayerFlag value);

/**
@debugoperatorenum{DebugLayerFlags}
@m_since_latest
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, DebugLayerFlags value);

/**
@brief Debug layer
@m_since_latest
*/
class MAGNUM_UI_EXPORT DebugLayer: public AbstractLayer {
    public:
        #ifdef DOXYGEN_GENERATING_OUTPUT
        class DebugIntegration; /* For documentation only */
        #endif

        /**
         * @brief Constructor
         * @param handle    Layer handle returned from
         *      @ref AbstractUserInterface::createLayer()
         * @param sources   Data sources to track
         * @param flags     Behavior flags
         *
         * See particular @ref DebugLayerFlag values for information about
         * which @ref DebugLayerSource is expected to be enabled for a
         * particular feature. While @p sources have to specified upfront, the
         * @p flags can be subsequently modified using @ref setFlags(),
         * @ref addFlags() and @ref clearFlags().
         *
         * Note that you can also construct the @ref DebugLayerGL subclass
         * instead to have the layer with visual feedback.
         */
        explicit DebugLayer(LayerHandle handle, DebugLayerSources sources, DebugLayerFlags flags);

        /** @brief Copying is not allowed */
        DebugLayer(const DebugLayer&) = delete;

        /**
         * @brief Move constructor
         *
         * Performs a destructive move, i.e. the original object isn't usable
         * afterwards anymore.
         */
        DebugLayer(DebugLayer&&) noexcept;

        ~DebugLayer();

        /** @brief Copying is not allowed */
        DebugLayer& operator=(const DebugLayer&) = delete;

        /** @brief Move assignment */
        DebugLayer& operator=(DebugLayer&&) noexcept;

        /** @brief Tracked data sources */
        DebugLayerSources sources() const;

        /** @brief Behavior flags */
        DebugLayerFlags flags() const;

        /**
         * @brief Set behavior flags
         * @return Reference to self (for method chaining)
         *
         * See particular @ref DebugLayerFlag values for information about
         * which @ref DebugLayerSource is expected to be enabled for a
         * particular feature.
         *
         * If a node was highlighted and @ref DebugLayerFlag::NodeHighlight was
         * cleared by calling this function, the highlight gets removed.  The
         * function doesn't print anything, but if a callback is set, it's
         * called with an empty string. Additionally, if the layer is
         * instantiated as @ref DebugLayerGL, it causes
         * @ref LayerState::NeedsDataUpdate to be set.
         * @see @ref addFlags(), @ref clearFlags()
         */
        DebugLayer& setFlags(DebugLayerFlags flags);

        /**
         * @brief Add behavior flags
         * @return Reference to self (for method chaining)
         *
         * Calls @ref setFlags() with the existing flags ORed with @p flags.
         * Useful for preserving previously set flags.
         * @see @ref clearFlags()
         */
        DebugLayer& addFlags(DebugLayerFlags flags);

        /**
         * @brief Clear flags
         * @return Reference to self (for method chaining)
         *
         * Calls @ref setFlags() with the existing flags ANDed with the inverse
         * of @p flags. Useful for removing a subset of previously set flags.
         * @see @ref addFlags()
         */
        DebugLayer& clearFlags(DebugLayerFlags flags);

        /**
         * @brief Node name
         *
         * Expects that the debug layer has been already passed to
         * @ref AbstractUserInterface::setLayerInstance() and that @p handle
         * isn't @ref NodeHandle::Null, @p handle doesn't have to be valid
         * however. If @ref DebugLayerSource::Nodes isn't enabled or @p handle
         * isn't known, returns an empty string.
         *
         * If not empty, the returned string is always
         * @relativeref{Corrade,Containers::StringViewFlag::NullTerminated}.
         * Unless @ref setLayerName() was called with a
         * @relativeref{Corrade,Containers::StringViewFlag::Global} string, the
         * returned view is only guaranteed to be valid until the next call to
         * @ref setLayerName() or until the set of layers in the user interface
         * changes.
         */
        Containers::StringView nodeName(NodeHandle handle) const;

        /**
         * @brief Set node name
         * @return Reference to self (for method chaining)
         *
         * Expects that the debug layer has been already passed to
         * @ref AbstractUserInterface::setLayerInstance() and that @p handle
         * isn't @ref NodeHandle::Null, @p handle doesn't have to be valid
         * however. If @ref DebugLayerSource::Nodes is enabled, the @p name
         * will be used to annotate given @p handle, otherwise the function
         * does nothing.
         *
         * If @p name is @relativeref{Corrade,Containers::StringViewFlag::Global}
         * and @relativeref{Corrade::Containers::StringViewFlag,NullTerminated},
         * no internal copy of the string is made. If
         * @ref DebugLayerSource::Nodes isn't enabled, the function does
         * nothing.
         */
        DebugLayer& setNodeName(NodeHandle handle, Containers::StringView name);

        /**
         * @brief Layer name
         *
         * Expects that the debug layer has been already passed to
         * @ref AbstractUserInterface::setLayerInstance() and that @p handle
         * isn't @ref LayerHandle::Null, @p handle doesn't have to be valid
         * however. If @ref DebugLayerSource::Layers isn't enabled or @p handle
         * isn't known, returns an empty string. For @ref handle() returns
         * @cpp "DebugLayer" @ce if a different name wasn't set.
         *
         * If not empty, the returned string is always
         * @relativeref{Corrade,Containers::StringViewFlag::NullTerminated}.
         * Unless @ref setLayerName() was called with a
         * @relativeref{Corrade,Containers::StringViewFlag::Global} string, the
         * returned view is only guaranteed to be valid until the next call to
         * @ref setLayerName() or until the set of layers in the user interface
         * changes.
         */
        Containers::StringView layerName(LayerHandle handle) const;

        /**
         * @brief Set layer name
         * @return Reference to self (for method chaining)
         *
         * Expects that the debug layer has been already passed to
         * @ref AbstractUserInterface::setLayerInstance() and that @p layer is
         * part of the same user interface. If @ref DebugLayerSource::Layers is
         * enabled, the @p name will be used to annotate attachments from given
         * @p layer, otherwise the function does nothing.
         *
         * If @p name is @relativeref{Corrade,Containers::StringViewFlag::Global}
         * and @relativeref{Corrade::Containers::StringViewFlag,NullTerminated},
         * no internal copy of the string is made. If
         * @ref DebugLayerSource::NodeDataAttachments isn't enabled, the
         * function does nothing.
         *
         * If a concrete layer type gets passed instead of just
         * @ref AbstractLayer, the @ref setLayerName(const T&, const Containers::StringView&)
         * overload may get picked if given layer implements debug integration,
         * allowing it to provide further details.
         */
        DebugLayer& setLayerName(const AbstractLayer& layer, Containers::StringView name);

        /**
         * @brief Set name for a layer with @ref DebugIntegration implemented
         * @return Reference to self (for method chaining)
         *
         * In addition to @ref setLayerName(const AbstractLayer&, Containers::StringView)
         * the layer's @ref DebugIntegration implementation gets called for
         * each attachment, allowing it to provide further details. See
         * documentation of a particular layer for more information. Use the
         * @ref setLayerName(const T&, const Containers::StringView&, const typename T::DebugIntegration&)
         * overload to pass additional arguments to the debug integration.
         */
        template<class T
            #ifndef DOXYGEN_GENERATING_OUTPUT
            , typename std::enable_if<std::is_default_constructible<typename T::DebugIntegration>::value, int>::type = 0
            #endif
        > DebugLayer& setLayerName(const T& layer, const Containers::StringView& name) {
            /* Delegating to the r-value overload */
            return setLayerName(layer, name, typename T::DebugIntegration{});
        }

        /**
         * @brief Set name for a layer with @ref DebugIntegration implemented
         * @return Reference to self (for method chaining)
         *
         * In addition to @ref setLayerName(const AbstractLayer&, Containers::StringView)
         * the passed layer @ref DebugIntegration instance gets called for
         * each attachment, allowing it to provide further details. See
         * documentation of a particular layer for more information.
         */
        template<class T> DebugLayer& setLayerName(const T& layer, const Containers::StringView& name, const typename T::DebugIntegration& integration);
        /** @overload */
        template<class T> DebugLayer& setLayerName(const T& layer, const Containers::StringView& name, typename T::DebugIntegration&& integration);

        /** @brief Node highlight color */
        Color4 nodeHighlightColor() const;

        /**
         * @brief Set node highlight color
         * @return Reference to self (for method chaining)
         *
         * Used only if @ref DebugLayerFlag::NodeHighlight is enabled and if
         * the layer is instantiated as @ref DebugLayerGL to be able to draw
         * the highlight rectangle, ignored otherwise. Default is
         * @cpp 0xff00ffff_rgbaf*0.5f @ce.
         *
         * If the layer is instantiated as @ref DebugLayerGL, calling this
         * function causes @ref LayerState::NeedsDataUpdate to be set.
         * @see @ref setNodeHighlightGesture(),
         *      @ref setNodeHighlightCallback()
         */
        DebugLayer& setNodeHighlightColor(const Color4& color);

        /** @brief Node highlight gesture */
        Containers::Pair<Pointers, Modifiers> nodeHighlightGesture() const;

        /**
         * @brief Set node highlight gesture
         * @return Reference to self (for method chaining)
         *
         * Used only if @ref DebugLayerFlag::NodeHighlight is enabled, ignored
         * otherwise. A highlight happens on a press of a pointer that's among
         * @p pointers with modifiers being exactly @p modifiers. Pressing on a
         * different node moves the highlight to the other node, pressing on a
         * node that's currently highlighted removes the highlight. Expects
         * that @p pointers are non-empty. Default is a combination of
         * @ref Pointer::MouseRight and @ref Pointer::Eraser, and
         * @ref Modifier::Ctrl, i.e. pressing either
         * @m_class{m-label m-warning} **Ctrl**
         * @m_class{m-label m-default} **right mouse button** or
         * @m_class{m-label m-warning} **Ctrl**
         * @m_class{m-label m-default} **pen eraser** will highlight a node under
         * the pointer. The currently highlighted node is available in
         * @ref currentHighlightedNode(), you can also use @ref highlightNode()
         * to perform a node highlight programmatically.
         * @see @ref setNodeHighlightColor(),
         *      @ref setNodeHighlightCallback()
         */
        DebugLayer& setNodeHighlightGesture(Pointers pointers, Modifiers modifiers);

        /** @brief Whether a node highlight callback is set */
        bool hasNodeHighlightCallback() const;

        /**
         * @brief Set node highlight callback
         * @return Reference to self (for method chaining)
         *
         * Used only if @ref DebugLayerFlag::NodeHighlight is enabled, ignored
         * otherwise. The @p callback receives a UTF-8 @p message with details
         * when a highlight happens on a pointer press, and an empty string if
         * a highlight is removed again. If not empty, the @p message is
         * guaranteed to be @relativeref{Corrade,Containers::StringViewFlag::NullTerminated}.
         *
         * If the callback is not set or if set to @cpp nullptr @ce, details
         * about the highlighted node are printed to @relativeref{Magnum,Debug}
         * instead.
         * @see @ref setNodeHighlightColor(),
         *      @ref setNodeHighlightGesture()
         */
        DebugLayer& setNodeHighlightCallback(Containers::Function<void(Containers::StringView message)>&& callback);

        /**
         * @brief Node highlighted by last pointer press
         *
         * Expects that @ref DebugLayerFlag::NodeHighlight is enabled. If no
         * node is currently highlighted, returns @ref NodeHandle::Null.
         *
         * The returned handle may be invalid if the node or any of its parents
         * were removed and @ref AbstractUserInterface::clean() wasn't called
         * since.
         */
        NodeHandle currentHighlightedNode() const;

        /**
         * @brief Highlight a node
         *
         * Expects that @ref DebugLayerFlag::NodeHighlight is enabled and the
         * layer has been already passed to
         * @ref AbstractUserInterface::setLayerInstance().
         *
         * If @p node is a known handle, the function performs similarly to the
         * node highlight gesture using a pointer press ---
         * @ref currentHighlightedNode() is set to @p node, details about the
         * node are printed to @relativeref{Magnum,Debug} or passed to a
         * callback if set, the node is visually higlighted if this is a
         * @ref DebugLayerGL instance, and the function returns @cpp true @ce.
         *
         * If @p node is @ref NodeHandle::Null or it's not a known handle (for
         * example an invalid handle of a now-removed node, or a handle of a
         * newly created node but @ref AbstractUserInterface::update() wasn't
         * called since) and there's a current highlight, it's removed. The
         * function doesn't print anything, but if a callback is set, it's
         * called with an empty string. If there's no current highlight, the
         * callback isn't called. The functions returns @cpp true @ce if
         * @p node is @ref NodeHandle::Null and @cpp false @ce if the handle is
         * unknown.
         *
         * Note that, compared to the node highlight gesture, where the node
         * details are always extracted from an up-to-date UI state, this
         * function only operates with the state known at the last call to
         * @ref AbstractUserInterface::update(). As such, for example nodes or
         * layers added since the last update won't be included in the output.
         *
         * If the layer is instantiated as @ref DebugLayerGL, calling this
         * function causes @ref LayerState::NeedsDataUpdate to be set.
         * @see @ref setNodeHighlightColor()
         *      @ref setNodeHighlightGesture(),
         *      @ref setNodeHighlightCallback()
         */
        bool highlightNode(NodeHandle node);

    #ifdef DOXYGEN_GENERATING_OUTPUT
    private:
    #else
    protected:
    #endif
        struct State;

        MAGNUM_UI_LOCAL explicit DebugLayer(LayerHandle handle, Containers::Pointer<State>&& state);

        /* These can't be MAGNUM_UI_LOCAL otherwise deriving from this class
           in tests causes linker errors */
        LayerFeatures doFeatures() const override;
        void doUpdate(LayerStates state, const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, const Containers::StridedArrayView1D<const Float>& nodeOpacities, Containers::BitArrayView nodesEnabled, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes, const Containers::StridedArrayView1D<const Vector2>& compositeRectOffsets, const Containers::StridedArrayView1D<const Vector2>& compositeRectSizes) override;

        Containers::Pointer<State> _state;

    private:
        /* Returned value is a pointer where to save the allocated
           DebugIntegration instance */
        void** setLayerNameDebugIntegration(const AbstractLayer& instance, const Containers::StringView& name, void(*deleter)(void*), void(*print)(void*, Debug&, const AbstractLayer&, const Containers::StringView&, LayerDataHandle));

        /* These can't be MAGNUM_UI_LOCAL otherwise deriving from this class
           in tests causes linker errors */
        LayerStates doState() const override;
        void doClean(Containers::BitArrayView dataIdsToRemove) override;
        void doPreUpdate(LayerStates state) override;
        void doPointerPressEvent(UnsignedInt dataId, PointerEvent& event) override;
};

#ifdef DOXYGEN_GENERATING_OUTPUT
/**
@brief Debug layer integration

If an inner type with this name is implemented on a layer that's passed to
@ref DebugLayer::setLayerName(const T&, const Containers::StringView&), the
@ref print() function is used by the @ref DebugLayerFlag::NodeHighlight
functionality to provide additional details about all data attachments coming
from given layer.
*/
/* While it'd be significantly simpler both for the library and for layers to
   have this as a virtual base class that then gets subclassed with interfaces
   implemented, it's deliberately not done to avoid header dependencies as well
   as make it possible to DCE all debug-layer-related code if it isn't used. */
class DebugLayer::DebugIntegration {
    public:
        /**
         * @brief Print details about particular data
         * @param debug     Debug output where to print
         * @param layer     Layer associated with given @p data. The
         *      implementation can use either the layer type this class is part
         *      of or any base type.
         * @param layerName Layer name that was passed to
         *      @ref DebugLayer::setLayerName(const T&, const Containers::StringView&)
         * @param data      Data to print info about. Guaranteed to be valid.
         *
         * Used internally by @ref DebugLayer. To fit among other info provided
         * by @ref DebugLayer itself, the implementation is expected to indent
         * the output by at least two spaces and end with a newline (i.e.,
         * @relativeref{Magnum,Debug::newline}).
         */
        void print(Debug& debug, const Layer& layer, Containers::StringView layerName, LayerDataHandle data);
};
#endif

template<class T> DebugLayer& DebugLayer::setLayerName(const T& layer, const Containers::StringView& name, const typename T::DebugIntegration& integration) {
    /* Delegating to the r-value overload */
    return setLayerName(layer, name,
        #if defined(CORRADE_TARGET_GCC) && !defined(CORRADE_TARGET_CLANG) && __GNUC__ < 5
        /* Can't use {} because for plain structs it would attempt to
           initialize the first member with `integration` instead of calling
           the copy constructor. Similar case is in Containers::Array etc., see
           layerNameDebugIntegrationCopyConstructPlainStruct() for details. */
        typename T::DebugIntegration(integration)
        #else
        typename T::DebugIntegration{integration}
        #endif
    );
}

template<class T> DebugLayer& DebugLayer::setLayerName(const T& layer, const Containers::StringView& name, typename T::DebugIntegration&& integration) {
    void** instance = setLayerNameDebugIntegration(
        /* This cast isn't strictly necessary but it makes the error clearer in
           case a completely unrelated type (such as a layouter) is passed */
        static_cast<const AbstractLayer&>(layer), name,
        [](void* integration) {
            delete static_cast<typename T::DebugIntegration*>(integration);
        },
        [](void* integration, Debug& out, const AbstractLayer& layer, const Containers::StringView& name, LayerDataHandle data) {
            static_cast<typename T::DebugIntegration*>(integration)->print(out, static_cast<const T&>(layer), name, data);
        });
    /* Done this way so failing graceful asserts in tests don't cause a leak.
       Yes, shitty, I know. */
    #ifdef CORRADE_GRACEFUL_ASSERT
    if(instance)
    #endif
    {
        #if defined(CORRADE_TARGET_GCC) && !defined(CORRADE_TARGET_CLANG) && __GNUC__ < 5
        /* Can't use {} because for plain structs it would attempt to
           initialize the first member with `integration` instead of calling
           the move constructor. Similar case is in Containers::Array etc., see
           layerNameDebugIntegrationMoveConstructPlainStruct() for details. */
        *instance = new typename T::DebugIntegration(Utility::move(integration));
        #else
        *instance = new typename T::DebugIntegration{Utility::move(integration)};
        #endif
    }
    return *this;
}

}}

#endif
