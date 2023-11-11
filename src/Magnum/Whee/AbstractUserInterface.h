#ifndef Magnum_Whee_AbstractUserInterface_h
#define Magnum_Whee_AbstractUserInterface_h
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
 * @brief Class @ref Magnum::Whee::AbstractUserInterface, enum @ref Magnum::Whee::UserInterfaceState, @ref Magnum::Whee::NodeFlag, enum set @ref Magnum::Whee::UserInterfaceStates, @ref Magnum::Whee::NodeFlags
 * @m_since_latest
 */

#include <Corrade/Containers/EnumSet.h>
#include <Corrade/Containers/Pointer.h>
#include <Magnum/Magnum.h>

#include "Magnum/Whee/Whee.h"
#include "Magnum/Whee/visibility.h"

namespace Magnum { namespace Whee {

/**
@brief User interface state
@m_since_latest

Used to decide whether @ref AbstractUserInterface::clean() or
@ref AbstractUserInterface::update() need to be called to refresh the internal
state before the interface is drawn or an event is handled. See also
@ref LayerState for layer-specific state.
@see @ref UserInterfaceStates, @ref AbstractUserInterface::state()
*/
enum class UserInterfaceState: UnsignedByte {
    /**
     * @ref AbstractUserInterface::update() needs to be called to recalculate
     * or reupload data attached to visible node hierarchy after they've been
     * changed. Set implicitly if any of the layers have
     * @ref LayerState::NeedsUpdate set, is reset next time
     * @ref AbstractUserInterface::update() is called. Implied by
     * @ref UserInterfaceState::NeedsDataAttachmentUpdate,
     * @relativeref{UserInterfaceState,NeedsNodeClipUpdate},
     * @relativeref{UserInterfaceState,NeedsNodeLayoutUpdate},
     * @relativeref{UserInterfaceState,NeedsNodeUpdate},
     * @relativeref{UserInterfaceState,NeedsDataClean} and
     * @relativeref{UserInterfaceState,NeedsNodeClean}, so it's also set by
     * everything that sets those flags.
     */
    NeedsDataUpdate = 1 << 0,

    /**
     * @ref AbstractUserInterface::update() needs to be called to refresh the
     * data attached to visible node hierarchy after new data were attached or
     * after existing attachments were removed and
     * @ref AbstractUserInterface::clean() was called. Set implicitly after
     * every @ref AbstractUserInterface::attachData() call and after
     * @ref AbstractUserInterface::clean() if
     * @ref UserInterfaceState::NeedsDataClean was set, is reset next time
     * @ref AbstractUserInterface::update() is called. Implies
     * @ref UserInterfaceState::NeedsDataUpdate. Implied by
     * @relativeref{UserInterfaceState,NeedsNodeClipUpdate},
     * @relativeref{UserInterfaceState,NeedsNodeLayoutUpdate},
     * @relativeref{UserInterfaceState,NeedsNodeUpdate},
     * @relativeref{UserInterfaceState,NeedsDataClean} and
     * @relativeref{UserInterfaceState,NeedsNodeClean}, so it's also set by
     * everything that sets those flags.
     */
    NeedsDataAttachmentUpdate = NeedsDataUpdate|(1 << 1),

    /**
     * @ref AbstractUserInterface::update() needs to be called to refresh the
     * visible node set after node sizes changed. Set implicitly after every
     * @ref AbstractUserInterface::setNodeSize() call, is
     * reset next time @ref AbstractUserInterface::update() is called. Implies
     * @ref UserInterfaceState::NeedsDataAttachmentUpdate. Implied by
     * @relativeref{UserInterfaceState,NeedsNodeLayoutUpdate},
     * @relativeref{UserInterfaceState,NeedsNodeUpdate},
     * @relativeref{UserInterfaceState,NeedsDataClean} and
     * @relativeref{UserInterfaceState,NeedsNodeClean}, so it's also set by
     * everything that sets those flags.
     */
    NeedsNodeClipUpdate = NeedsDataAttachmentUpdate|(1 << 2),

    /**
     * @ref AbstractUserInterface::update() needs to be called to refresh the
     * visible node hierarchy layout after node sizes or offsets changed. Set
     * implicitly after every @ref AbstractUserInterface::setNodeOffset() call,
     * is reset next time @ref AbstractUserInterface::update() is called.
     * Implies @ref UserInterfaceState::NeedsNodeClipUpdate. Implied by
     * @relativeref{UserInterfaceState,NeedsNodeUpdate},
     * @relativeref{UserInterfaceState,NeedsDataClean} and
     * @relativeref{UserInterfaceState,NeedsNodeClean}, so it's also set by
     * everything that sets those flags.
     */
    NeedsNodeLayoutUpdate = NeedsNodeClipUpdate|(1 << 3),

    /**
     * @ref AbstractUserInterface::update() needs to be called to refresh the
     * visible node hierarchy and data attached to it after nodes were added or
     * removed or the top-level node order changed. Set implicitly after every
     * @ref AbstractUserInterface::createNode(),
     * @relativeref{AbstractUserInterface,setNodeFlags()},
     * @relativeref{AbstractUserInterface,addNodeFlags()},
     * @relativeref{AbstractUserInterface,clearNodeFlags()},
     * @relativeref{AbstractUserInterface,setNodeOrder()} and
     * @relativeref{AbstractUserInterface,clearNodeOrder()} call, is reset next
     * time @ref AbstractUserInterface::update() is called. Implies
     * @ref UserInterfaceState::NeedsNodeLayoutUpdate. Implied by
     * @relativeref{UserInterfaceState,NeedsNodeClean}, so it's also set by
     * everything that sets that flag.
     */
    NeedsNodeUpdate = NeedsNodeLayoutUpdate|(1 << 4),

    /**
     * @ref AbstractUserInterface::clean() needs to be called to prune
     * no-longer-valid data attachments and data no longer used by any node.
     * Set implicitly after every @ref AbstractUserInterface::removeLayer()
     * call and if any of the layers have @ref LayerState::NeedsClean set, is
     * reset to @ref UserInterfaceState::NeedsDataAttachmentUpdate next time
     * @ref AbstractUserInterface::clean() is called. Implies
     * @ref UserInterfaceState::NeedsDataAttachmentUpdate. Implied by
     * @relativeref{UserInterfaceState,NeedsNodeClean}, so it's also set by
     * everything that sets that flag.
     */
    NeedsDataClean = NeedsDataAttachmentUpdate|(1 << 5),

    /**
     * @ref AbstractUserInterface::clean() needs to be called to prune child
     * hierarchies of removed nodes, data attached to those, and
     * no-longer-valid data attachments. Set implicitly after every
     * @relativeref{AbstractUserInterface,removeNode()} call, is reset next
     * time @ref AbstractUserInterface::clean() is called. Implies
     * @ref UserInterfaceState::NeedsNodeUpdate and
     * @ref UserInterfaceState::NeedsDataClean.
     */
    NeedsNodeClean = NeedsNodeUpdate|NeedsDataClean|(1 << 6),
};

/**
@debugoperatorenum{UserInterfaceState}
@m_since_latest
*/
MAGNUM_WHEE_EXPORT Debug& operator<<(Debug& debug, UserInterfaceState value);

/**
@brief User interface states
@m_since_latest

@see @ref AbstractUserInterface::state()
*/
typedef Containers::EnumSet<UserInterfaceState> UserInterfaceStates;

/**
@debugoperatorenum{UserInterfaceStates}
@m_since_latest
*/
MAGNUM_WHEE_EXPORT Debug& operator<<(Debug& debug, UserInterfaceStates value);

CORRADE_ENUMSET_OPERATORS(UserInterfaceStates)

/**
@brief Node behavior flag
@m_since_latest

@see @ref NodeFlags, @ref AbstractUserInterface::createNode(),
    @ref AbstractUserInterface::nodeFlags(),
    @ref AbstractUserInterface::setNodeFlags(),
    @ref AbstractUserInterface::addNodeFlags(),
    @ref AbstractUserInterface::clearNodeFlags()
*/
enum class NodeFlag: UnsignedByte {
    /**
     * The node, all nested nodes and all attached data are hidden, i.e. not
     * drawn and excluded from event processing.
     *
     * For top-level nodes (i.e., nodes for which
     * @ref AbstractUserInterface::nodeParent() is @ref NodeHandle::Null) a
     * draw order is managed in addition to the hidden state, using
     * @ref AbstractUserInterface::setNodeOrder() and
     * @relativeref{AbstractUserInterface,clearNodeOrder()}. A top-level node
     * that isn't in the draw order list behaves the same as if
     * @ref NodeFlag::Hidden was set for it. For performance reasons it's
     * however recommended to keep the draw list small rather than having it
     * full of mostly hidden nodes.
     */
    Hidden = 1 << 0
};

/**
@debugoperatorenum{NodeFlag}
@m_since_latest
*/
MAGNUM_WHEE_EXPORT Debug& operator<<(Debug& debug, NodeFlag value);

/**
@brief Node behavior flags
@m_since_latest

@see @ref AbstractUserInterface::createNode()
*/
typedef Containers::EnumSet<NodeFlag> NodeFlags;

/**
@debugoperatorenum{NodeFlags}
@m_since_latest
*/
MAGNUM_WHEE_EXPORT Debug& operator<<(Debug& debug, NodeFlags value);

CORRADE_ENUMSET_OPERATORS(NodeFlags)

/**
@brief Base for the main user interface
@m_since_latest

@section Whee-AbstractUserInterface-dpi DPI awareness

There are three separate concepts for DPI-aware UI rendering:

-   UI size --- size of the user interface to which all widgets are positioned
-   Window size --- size of the window to which all input events are related
-   Framebuffer size --- size of the framebuffer the UI is being rendered to

Depending on the platform and use case, each of these three values can be
different. For example, a game menu screen can have the UI size the same
regardless of window size. Or on Retina macOS you can have different window and
framebuffer size and the UI size might be related to window size but
independent on the framebuffer size.

When using for example @ref Platform::Sdl2Application or other `*Application`
implementations, you usually have three values at your disposal ---
@ref Platform::Sdl2Application::windowSize() "windowSize()",
@ref Platform::Sdl2Application::framebufferSize() "framebufferSize()" and
@ref Platform::Sdl2Application::dpiScaling() "dpiScaling()". If you want the UI
to have the same layout and just scale on bigger window sizes, pass a fixed
value to the UI size:

@snippet Whee-sdl2.cpp AbstractUserInterface-dpi-fixed

If you want the UI to get more room with larger window sizes and behave
properly with different DPI scaling values, pass a ratio of window size and DPI
scaling to the UI size:

@snippet Whee-sdl2.cpp AbstractUserInterface-dpi-ratio

Finally, to gracefully deal with extremely small or extremely large windows,
you can apply @ref Math::clamp() on top with some defined bounds. Windows
outside of the reasonable size range will then get a scaled version of the UI
at boundary size:

@snippet Whee-sdl2.cpp AbstractUserInterface-dpi-clamp
*/
class MAGNUM_WHEE_EXPORT AbstractUserInterface {
    public:
        /**
         * @brief Construct without creating the user interface with concrete parameters
         *
         * You're expected to call @ref setSize() afterwards in order to define
         * scaling of event coordinates, node positions and projection matrices
         * for drawing.
         */
        explicit AbstractUserInterface(NoCreateT);

        /**
         * @brief Construct
         * @param size                  Size of the user interface to which
         *      everything is positioned
         * @param windowSize            Size of the window to which all input
         *      events are related
         * @param framebufferSize       Size of the window framebuffer. On
         *      some platforms with HiDPI screens may be different from window
         *      size.
         *
         * Equivalent to constructing with @ref AbstractUserInterface(NoCreateT)
         * and then calling @ref setSize(const Vector2&, const Vector2&, const Vector2i&).
         * See its documentation for more information.
         */
        explicit AbstractUserInterface(const Vector2& size, const Vector2& windowSize, const Vector2i& framebufferSize);

        /**
         * @brief Construct with an unscaled size
         *
         * Delegates to @ref AbstractUserInterface(const Vector2&, const Vector2&, const Vector2i&)
         * with all sizes set to @p size. Doing so assumes that the coordinate
         * system in which events are passed matches framebuffer size.
         */
        explicit AbstractUserInterface(const Vector2i& size);

        /** @brief Copying is not allowed */
        AbstractUserInterface(const AbstractUserInterface&) = delete;

        /**
         * @brief Move constructor
         *
         * Performs a destructive move, i.e. the original object isn't usable
         * afterwards anymore.
         */
        AbstractUserInterface(AbstractUserInterface&&) noexcept;

        /** @brief Copying is not allowed */
        AbstractUserInterface& operator=(const AbstractUserInterface&) = delete;

        /** @brief Move assignment */
        AbstractUserInterface& operator=(AbstractUserInterface&&) noexcept;

        ~AbstractUserInterface();

        /**
         * @brief User interface size
         *
         * Node positioning is in respect to this size. If @ref setSize() or
         * @ref AbstractUserInterface(const Vector2&, const Vector2&, const Vector2i&)
         * wasn't called yet, initial value is a zero vector.
         */
        Vector2 size() const;

        /**
         * @brief Window size
         *
         * Global event position in @ref pointerPressEvent(),
         * @ref pointerReleaseEvent() and @ref pointerMoveEvent() is in respect
         * to this size. If @ref setSize() or @ref AbstractUserInterface(const Vector2&, const Vector2&, const Vector2i&)
         * wasn't called yet, initial value is a zero vector.
         */
        Vector2 windowSize() const;

        /**
         * @brief Framebuffer size
         *
         * Rendering performed by layers is in respect to this size. If
         * @ref setSize() or
         * @ref AbstractUserInterface(const Vector2&, const Vector2&, const Vector2i&)
         * wasn't called yet, initial value is a zero vector.
         */
        Vector2i framebufferSize() const;

        /**
         * @brief Set user interface size
         * @param size                  Size of the user interface to which
         *      everything is positioned
         * @param windowSize            Size of the window to which all input
         *      events are related
         * @param framebufferSize       Size of the window framebuffer. On
         *      some platforms with HiDPI screens may be different from window
         *      size.
         * @return Reference to self (for method chaining)
         *
         * All sizes are expected to be non-zero, origin is top left for all.
         *
         * After calling this function, the @ref pointerPressEvent(),
         * @ref pointerReleaseEvent() and @ref pointerMoveEvent() functions
         * take the global event position with respect to @p windowSize, which
         * is then rescaled to match @p size when exposed through
         * @ref PointerEvent. The @p size and @p framebufferSize is passed
         * through to @ref AbstractLayer::setSize() to all layers with
         * @ref LayerFeature::Draw so they can set appropriate projection and
         * other framebuffer-related properties.
         *
         * There's no default size and this function is expected to be called
         * before the first @ref update() happens, either directly or through
         * the @ref AbstractUserInterface(const Vector2&, const Vector2&, const Vector2i&)
         * constructor. It's allowed to call this function for the first time
         * even after node, layers or data were created.
         *
         * Calling this function with new values will update the event position
         * scaling accordingly. @ref AbstractLayer::setSize() is called only if
         * @p size or @p framebufferSize changes.
         */
        AbstractUserInterface& setSize(const Vector2& size, const Vector2& windowSize, const Vector2i& framebufferSize);

        /**
         * @brief Set unscaled user interface size
         * @return Reference to self (for method chaining)
         *
         * Calls @ref setSize(const Vector2&, const Vector2&, const Vector2i&)
         * with all sizes set to @p size. Doing so assumes that the coordinate
         * system in which events are passed matches framebuffer size.
         */
        AbstractUserInterface& setSize(const Vector2i& size);

        /**
         * @brief User interface state
         *
         * See the @ref UserInterfaceState enum for more information. By
         * default no flags are set.
         */
        UserInterfaceStates state() const;

        /** @{
         * @name Layer and data management
         */

        /**
         * @brief Capacity of the layer storage
         *
         * Can be at most 256. If @ref createLayer() is called and there's no
         * free slots left, the internal storage gets grown.
         * @see @ref layerUsedCount()
         */
        std::size_t layerCapacity() const;

        /**
         * @brief Count of used items in the layer storage
         *
         * Always at most @ref layerCapacity(). Expired handles are counted
         * among used as well. The operation is done with a
         * @f$ \mathcal{O}(n) @f$ complexity where @f$ n @f$ is
         * @ref layerCapacity().
         */
        std::size_t layerUsedCount() const;

        /**
         * @brief Whether a layer handle is valid
         *
         * A handle is valid if it has been returned from @ref createLayer()
         * before and @ref removeLayer() wasn't called on it yet. Note that a
         * handle is valid even if the layer instance wasn't set with
         * @ref setLayerInstance() yet. For @ref LayerHandle::Null always
         * returns @cpp false @ce.
         */
        bool isHandleValid(LayerHandle handle) const;

        /**
         * @brief Whether a data handle is valid
         *
         * A shorthand for extracting a @ref LayerHandle from @p handle using
         * @ref dataHandleLayer(), calling @ref isHandleValid(LayerHandle) const
         * on it, if it's valid and set then retrieving the particular layer
         * instance using @ref layer() and then calling
         * @ref AbstractLayer::isHandleValid(LayerDataHandle) const with a
         * @ref LayerDataHandle extracted from @p handle using
         * @ref dataHandleData(). See these functions for more information. For
         * @ref DataHandle::Null, @ref LayerHandle::Null or
         * @ref LayerDataHandle::Null always returns @cpp false @ce.
         */
        bool isHandleValid(DataHandle handle) const;

        /**
         * @brief First layer in draw and event processing order
         *
         * The first layer gets drawn first (thus is at the back) and reacts to
         * events after all others. Returns @ref LayerHandle::Null if there's
         * no layers yet. The returned handle is always either valid or null.
         * @see @ref layerNext(), @ref layerLast(), @ref layerPrevious()
         */
        LayerHandle layerFirst() const;

        /**
         * @brief Last layer in draw and event processing order
         *
         * The last layer gets drawn last (thus is at the front) and reacts to
         * event before all others. Returns @ref LayerHandle::Null if there's
         * no layers yet. The returned handle is always either valid or null.
         * @see @ref layerPrevious(), @ref layerFirst(), @ref layerNext()
         */
        LayerHandle layerLast() const;

        /**
         * @brief Previous layer in draw and event processing order
         *
         * The previous layer gets drawn earlier (thus is behind) and reacts to
         * events later. Expects that @p handle is valid. Returns
         * @ref LayerHandle::Null if the layer is first. The returned handle
         * is always either valid or null.
         * @see @ref isHandleValid(LayerHandle) const, @ref layerNext(),
         *      @ref layerFirst(), @ref layerLast()
         */
        LayerHandle layerPrevious(LayerHandle handle) const;

        /**
         * @brief Next layer in draw and event processing order
         *
         * The next layer gets drawn later (thus is in front) and reacts to
         * events earlier. Expects that @p handle is valid. Returns
         * @ref LayerHandle::Null if the layer is last. The returned handle is
         * always either valid or null.
         * @see @ref isHandleValid(LayerHandle) const, @ref layerPrevious(),
         *      @ref layerLast(), @ref layerFirst()
         */
        LayerHandle layerNext(LayerHandle handle) const;

        /**
         * @brief Create a layer
         * @param before    A layer to order before for draw and event
         *      processing or @ref LayerHandle::Null if ordered as last (i.e.,
         *      at the front, being drawn last and receiving events first).
         *      Expected to be valid if not null.
         * @return New layer handle
         *
         * Allocates a new handle in a free slot in the internal storage or
         * grows the storage if there's no free slots left. Expects that
         * there's at most 256 layers. The returned handle is meant to be used
         * to construct an @ref AbstractLayer subclass and the instance then
         * passed to @ref setLayerInstance(). A layer can be removed again with
         * @ref removeLayer().
         * @see @ref isHandleValid(LayerHandle) const,
         *      @ref layerCapacity(), @ref layerUsedCount()
         */
        LayerHandle createLayer(LayerHandle before =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            LayerHandle::Null
            #else
            LayerHandle{} /* To not have to include Handle.h */
            #endif
        );

        /**
         * @brief Set a layer instance
         * @return Reference to @p instance
         *
         * Expects that @p instance was created with a @ref LayerHandle
         * returned from @ref createLayer() earlier, the handle is valid and
         * @ref setLayerInstance() wasn't called for the same handle yet.
         *
         * Calls @ref AbstractLayer::setSize() on the layer, unless neither
         * @ref setSize() nor @ref AbstractUserInterface(const Vector2&, const Vector2&, const Vector2i&)
         * was called yet.
         * @see @ref AbstractLayer::handle(),
         *      @ref isHandleValid(LayerHandle) const
         */
        AbstractLayer& setLayerInstance(Containers::Pointer<AbstractLayer>&& instance);
        /** @overload */
        template<class T> T& setLayerInstance(Containers::Pointer<T>&& instance) {
            return static_cast<T&>(setLayerInstance(Containers::Pointer<AbstractLayer>{Utility::move(instance)}));
        }

        /**
         * @brief Layer instance
         *
         * Expects that @p handle is valid and that @ref setLayerInstance() was
         * called for it.
         * @see @ref isHandleValid(LayerHandle) const
         */
        AbstractLayer& layer(LayerHandle handle);
        const AbstractLayer& layer(LayerHandle handle) const; /**< @overload */

        /**
         * @brief Layer instance in a concrete type
         *
         * Expects that @p handle is valid and that @ref setLayerInstance() was
         * called for it. It's the user responsibility to ensure that @p T
         * matches the actual instance type.
         * @see @ref isHandleValid(LayerHandle) const
         */
        template<class T> T& layer(LayerHandle handle) {
            return static_cast<T&>(layer(handle));
        }
        /** @overload */
        template<class T> const T& layer(LayerHandle handle) const {
            return static_cast<const T&>(layer(handle));
        }

        /**
         * @brief Remove a layer
         *
         * Expects that @p handle is valid. Data from this layer attached to
         * hierarchies with @ref attachData() are removed during the next call
         * to @ref update(). After this call, @ref isHandleValid(LayerHandle) const
         * returns @cpp false @ce for @p handle and
         * @ref isHandleValid(DataHandle) const returns @cpp false @ce for all
         * data associated with @p handle.
         *
         * Calling this function causes @ref UserInterfaceState::NeedsDataClean
         * to be set.
         * @see @ref clean()
         */
        void removeLayer(LayerHandle handle);

        /**
         * @brief Count of node data attachments
         *
         * May include also invalid data and data attached to invalid node
         * handles if either data or nodes were removed and @ref update()
         * wasn't called since.
         * @see @ref attachData()
         */
        std::size_t dataAttachmentCount() const;

        /**
         * @brief Attach data to a node
         *
         * Makes the @p data handle tied to a particular @p node, meaning
         * that @ref removeNode() of @p node or any parent node will also
         * cause the @p data to be scheduled for removal during the next
         * @ref clean() call. While it's technically possible to add the same
         * @p data handle to multiple node, the actual behavior depends on the
         * particular @ref AbstractLayer implementation the data belongs to.
         * Attaching multiple different data handles to a single @p node is
         * supported always.
         *
         * The internal representation doesn't allow removing data attached to
         * a node. Instead you can remove the data with
         * @ref AbstractLayer::remove() or remove the whole node, attachments
         * with invalid node handles or invalid data handles then get pruned
         * during the next @ref clean() call.
         *
         * Calling this function causes
         * @ref UserInterfaceState::NeedsDataAttachmentUpdate to be set.
         * @see @ref dataAttachmentCount(), @ref update()
         */
        void attachData(NodeHandle node, DataHandle data);

        /**
         * @}
         */

        /** @{
         * @name Node management
         */

        /**
         * @brief Current capacity of the node storage
         *
         * Can be at most 1048576. If @ref createNode() is called and there's
         * no free slots left, the internal storage gets grown.
         * @see @ref nodeUsedCount()
         */
        std::size_t nodeCapacity() const;

        /**
         * @brief Count of used items in the node storage
         *
         * Always at most @ref nodeCapacity(). Expired handles are counted
         * among used as well. The operation is done with a
         * @f$ \mathcal{O}(n) @f$ complexity where @f$ n @f$ is
         * @ref nodeCapacity().
         */
        std::size_t nodeUsedCount() const;

        /**
         * @brief Whether a node handle is valid
         *
         * A handle is valid if it has been returned from @ref createNode()
         * before, @ref removeNode() wasn't called on it yet and it wasn't
         * removed inside @ref update() due to a parent node being removed
         * earlier. For @ref NodeHandle::Null always returns @cpp false @ce.
         */
        bool isHandleValid(NodeHandle handle) const;

        /**
         * @brief Create a node
         * @param parent    Parent node to attach to or
         *      @ref NodeHandle::Null for a new root node. Expected to be valid
         *      if not null.
         * @param offset    Offset relative to the parent node
         * @param size      Size of the node contents. Used for layouting,
         *      clipping and event handling.
         * @param flags     Initial node flags
         * @return New node handle
         *
         * Allocates a new handle in a free slot in the internal storage or
         * grows the storage if there's no free slots left. Expects that
         * there's at most 1048576 nodes. The returned handle can be then
         * removed again with @ref removeNode().
         *
         * If @p parent is @ref NodeHandle::Null, the node is added at the
         * back of the draw and event processing list, i.e. drawn last (thus
         * at the front) and reacting to events before all others. Use
         * @ref setNodeOrder() and @ref clearNodeOrder() to adjust the draw
         * and event processing order or remove it from the list of visible
         * top-level nodes.
         *
         * Calling this function causes @ref UserInterfaceState::NeedsNodeUpdate
         * to be set.
         * @see @ref isHandleValid(NodeHandle) const, @ref nodeCapacity(),
         *      @ref nodeUsedCount(), @ref update()
         */
        NodeHandle createNode(NodeHandle parent, const Vector2& offset, const Vector2& size, NodeFlags flags = {});

        /**
         * @brief Create a root node
         *
         * Equivalent to calling @ref createNode(NodeHandle, const Vector2&, const Vector2&, NodeFlags)
         * with @ref NodeHandle::Null as the parent.
         */
        NodeHandle createNode(const Vector2& offset, const Vector2& size, NodeFlags flags = {});

        /**
         * @brief Node parent
         *
         * Expects that @p handle is valid. Returns @ref NodeHandle::Null if
         * it's a root node, in which case @ref nodeOrderPrevious(),
         * @ref nodeOrderNext(), @ref nodeOrderFirst() and @ref nodeOrderLast()
         * can be used to inspect the draw and envent processing order.
         *
         * Note that the returned handle may be invalid if @ref removeNode()
         * was called on one of the parent nodes and @ref update() hasn't
         * been called since.
         *
         * Unlike other node properties, the parent cannot be changed after
         * creation.
         * @see @ref isHandleValid(NodeHandle) const
         */
        NodeHandle nodeParent(NodeHandle handle) const;

        /**
         * @brief Node offset relative to its parent
         *
         * Expects that @p handle is valid.
         * @see @ref isHandleValid(NodeHandle) const, @ref nodeParent()
         */
        Vector2 nodeOffset(NodeHandle handle) const;

        /**
         * @brief Set node offset relative to its parent
         *
         * Expects that @p handle is valid.
         *
         * Calling this function causes
         * @ref UserInterfaceState::NeedsNodeLayoutUpdate to be set.
         * @see @ref isHandleValid(NodeHandle) const, @ref nodeParent()
         */
        void setNodeOffset(NodeHandle handle, const Vector2& offset);

        /**
         * @brief Node size
         *
         * Expects that @p handle is valid.
         * @see @ref isHandleValid(NodeHandle) const
         */
        Vector2 nodeSize(NodeHandle handle) const;

        /**
         * @brief Set node size
         *
         * Expects that @p handle is valid.
         *
         * Calling this function causes
         * @ref UserInterfaceState::NeedsNodeClipUpdate to be set.
         * @see @ref isHandleValid(NodeHandle) const
         */
        void setNodeSize(NodeHandle handle, const Vector2& size);

        /**
         * @brief Node flags
         *
         * Expects that @p handle is valid.
         * @see @ref isHandleValid(NodeHandle) const, @ref setNodeFlags(),
         *      @ref addNodeFlags(), @ref clearNodeFlags()
         */
        NodeFlags nodeFlags(NodeHandle handle) const;

        /**
         * @brief Set node flags
         *
         * Expects that @p handle is valid.
         *
         * If @ref NodeFlag::Hidden was added or cleared by calling this
         * function, it causes @ref UserInterfaceState::NeedsNodeUpdate to be
         * set.
         * @see @ref isHandleValid(NodeHandle) const, @ref addNodeFlags(),
         *      @ref clearNodeFlags()
         */
        void setNodeFlags(NodeHandle handle, NodeFlags flags);

        /**
         * @brief Add node flags
         *
         * Calls @ref setNodeFlags() with the existing flags ORed with
         * @p flags. Useful for preserving previously set flags.
         * @see @ref clearNodeFlags()
         */
        void addNodeFlags(NodeHandle handle, NodeFlags flags);

        /**
         * @brief Clear node flags
         *
         * Calls @ref setNodeFlags() with the existing flags ANDed with the
         * inverse of @p flags. Useful for removing a subset of previously set
         * flags.
         * @see @ref addNodeFlags()
         */
        void clearNodeFlags(NodeHandle handle, NodeFlags flags);

        /**
         * @brief Remove a node
         *
         * Expects that @p handle is valid. Nested nodes and data attached to
         * any of the nodes are then removed during the next call to
         * @ref update(). After this call, @ref isHandleValid(NodeHandle) const
         * returns @cpp false @ce for @p handle.
         *
         * Calling this function causes @ref UserInterfaceState::NeedsNodeClean
         * to be set.
         * @see @ref clean()
         */
        void removeNode(NodeHandle handle);

        /**
         * @}
         */

        /** @{
         * @name Top-level node draw and event processing order management
         */

        /**
         * @brief Capacity of the node order storage
         *
         * @ref nodeOrderUsedCount()
         */
        std::size_t nodeOrderCapacity() const;

        /**
         * @brief Count of used items in the node order storage
         *
         * Always at most @ref nodeOrderCapacity(). The operation is done
         * with a @f$ \mathcal{O}(n) @f$ complexity where @f$ n @f$ is
         * @ref nodeOrderCapacity().
         */
        std::size_t nodeOrderUsedCount() const;

        /**
         * @brief First node in draw and event processing order
         *
         * The first node gets drawn first (thus is at the back) and reacts
         * to events after all others. Returns @ref NodeHandle::Null if
         * there's no nodes included in the draw and event processing order.
         * The returned handle is always either valid or null.
         * @see @ref nodeOrderNext(), @ref nodeOrderLast(),
         *      @ref nodeOrderPrevious(), @ref isNodeOrdered()
         */
        NodeHandle nodeOrderFirst() const;

        /**
         * @brief Last node in draw and event processing order
         *
         * The last node gets drawn last (thus is at the front) and reacts to
         * events before all others. Returns @ref NodeHandle::Null if there's
         * no nodes included in the draw and event processing order. The
         * returned handle is always either valid or null.
         * @see @ref nodeOrderPrevious(), @ref nodeOrderFirst(),
         *      @ref nodeOrderNext(), @ref isNodeOrdered()
         */
        NodeHandle nodeOrderLast() const;

        /**
         * @brief Whether a node is included in a draw and event processing order
         *
         * If not included, the node and all its children are not drawn and
         * don't react to events. Expects that @p handle is valid and is a
         * root node.
         * @see @ref isHandleValid(NodeHandle) const, @ref nodeParent(),
         *      @ref nodeOrderNext(), @ref setNodeOrder(),
         *      @ref clearNodeOrder()
         */
        bool isNodeOrdered(NodeHandle handle) const;

        /**
         * @brief Previous node in draw and event processing order
         *
         * The previous node gets drawn earlier (thus is behind) and reacts
         * to events later. Expects that @p handle is valid and is a root node.
         * Returns @ref NodeHandle::Null if the node is either first or not
         * included in the draw and event processing order. The returned handle
         * is always either valid or null.
         * @see @ref isHandleValid(NodeHandle) const, @ref nodeParent(),
         *      @ref nodeOrderNext(), @ref nodeOrderFirst(),
         *      @ref nodeOrderLast(), @ref isNodeOrdered()
         */
        NodeHandle nodeOrderPrevious(NodeHandle handle) const;

        /**
         * @brief Next node in draw and event processing order
         *
         * The next node gets drawn later (thus is in front) and reacts to
         * events earlier. Expects that @p handle is valid and is a root node.
         * Returns @ref NodeHandle::Null if the node is either last or not
         * included in the draw and event processing order. The returned handle
         * is always either valid or null.
         * @see @ref isHandleValid(NodeHandle) const, @ref nodeParent(),
         *      @ref nodeOrderPrevious(), @ref nodeOrderLast(),
         *      @ref nodeOrderFirst(), @ref isNodeOrdered()
         */
        NodeHandle nodeOrderNext(NodeHandle handle) const;

        /**
         * @brief Order a node for draw and event processing
         *
         * The @p handle gets ordered to be drawn earlier (thus behind) and
         * react to event later than @p before. Expects that @p handle is valid
         * and is a root node, and that @p before is either
         * @ref NodeHandle::Null or valid root node that's included in the draw
         * and event processing order. If the node was previously in a
         * different position in the draw and event processing order, it's
         * moved, if it wasn't previously in the draw and event processing
         * order, it's inserted.
         *
         * Calling this function causes @ref UserInterfaceState::NeedsNodeUpdate
         * to be set.
         * @see @ref isHandleValid(NodeHandle) const, @ref nodeParent(),
         *      @ref clearNodeOrder(), @ref update()
         */
        void setNodeOrder(NodeHandle handle, NodeHandle before);

        /**
         * @brief Clear a node from the draw and event processing order
         *
         * Expects that @p handle is valid and is a root node. If the node
         * wasn't previously in the draw and processing order, the function is
         * a no-op.
         *
         * If not a no-op, calling this function causes
         * @ref UserInterfaceState::NeedsNodeUpdate to be set.
         * @see @ref isHandleValid(NodeHandle) const, @ref nodeParent(),
         *      @ref setNodeOrder(), @ref update()
         */
        void clearNodeOrder(NodeHandle handle);

        /**
         * @}
         */

        /**
         * @brief Clean orphaned nodes, data and no longer valid data attachments
         * @return Reference to self (for method chaining)
         *
         * Called implicitly from @ref update() and subsequently also from
         * @ref draw() and all event processing functions. If @ref state()
         * contains neither @ref UserInterfaceState::NeedsDataClean nor
         * @ref UserInterfaceState::NeedsNodeClean, this function is a no-op,
         * otherwise it performs a subset of the following depending on the
         * state:
         *
         * -    Removes nodes with an invalid (removed) parent node
         * -    Removes data attached to invalid nodes
         * -    Removes attachments with invalid (removed) data
         * -    Calls @ref AbstractLayer::clean() with the newly removed data
         *
         * After calling this function, @ref state() contains neither
         * @ref UserInterfaceState::NeedsDataClean nor
         * @ref UserInterfaceState::NeedsNodeClean; @ref nodeUsedCount(),
         * @ref dataAttachmentCount() and @ref AbstractLayer::usedCount() may
         * get smaller.
         */
        AbstractUserInterface& clean();

        /**
         * @brief Update node hierarchy, data order and data contents for drawing and event processing
         * @return Reference to self (for method chaining)
         *
         * Expects that either @ref setSize() was called or the
         * @ref AbstractUserInterface(const Vector2&, const Vector2&, const Vector2i&)
         * constructor was used.
         *
         * Implicitly calls @ref clean(); called implicitly from @ref draw()
         * and all event processing functions. If @ref state() contains none of
         * @ref UserInterfaceState::NeedsDataUpdate,
         * @ref UserInterfaceState::NeedsDataAttachmentUpdate,
         * @ref UserInterfaceState::NeedsNodeClipUpdate,
         * @ref UserInterfaceState::NeedsNodeLayoutUpdate or
         * @ref UserInterfaceState::NeedsNodeUpdate, this function is a no-op,
         * otherwise it performs a subset of the following depending on the
         * state:
         *
         * -    Orders visible nodes back-to-front for drawing, front-to-back
         *      for event processing and collects their data attachments
         * -    Calculates absolute offsets for visible nodes
         * -    Culls invisible nodes
         * -    Partitions node data attachments by the layer and then by draw
         *      order
         * -    Calls @ref AbstractLayer::update() with the partitioned data
         * -    Resets @ref pointerEventCapturedNode() or
         *      @ref pointerEventHoveredNode() if the nodes no longer exist or
         *      are not visible
         *
         * After calling this function, @ref state() is empty.
         */
        AbstractUserInterface& update();

        /**
         * @brief Draw the user interface
         * @return Reference to self (for method chaining)
         *
         * Implicitly calls @ref update() and @ref clean(). Calls
         * @ref AbstractLayer::draw() on layers that support
         * @ref LayerFeature::Draw in a back-to-front order.
         */
        AbstractUserInterface& draw();

        /**
         * @brief Handle a pointer press event
         *
         * Implicitly calls @ref update() and @ref clean(). The
         * @p globalPosition is assumed to be in respect to @ref windowSize(),
         * and is internally scaled to match @ref size() before being set to
         * @ref PointerEvent.
         *
         * Finds the front-most nodes under (scaled) @p globalPosition and then
         * backtracks to parent nodes. For each such node calls
         * @ref AbstractLayer::pointerPressEvent() on all data belonging to
         * layers that support @ref LayerFeature::Event until one of them
         * accepts the event. For each call the event position is made relative
         * to the node to which the data is attached. Returns @cpp true @ce if
         * the event was accepted, @cpp false @ce if it wasn't or there wasn't
         * any visible event handling node at given position and thus the event
         * should be propagated further.
         *
         * The node that accepted the event implicitly captures all further
         * pointer events until and including a @ref pointerReleaseEvent() even
         * if they happen outside of its area, unless
         * @ref PointerEvent::setCaptured() is called by the implementation to
         * disable this behavior. The capture can be also removed from a later
         * @ref pointerMoveEvent(). Any node that was already captured when
         * calling this function is ignored.
         *
         * Expects that the event is not accepted yet.
         * @see @ref PointerEvent::isAccepted(),
         *      @ref PointerEvent::setAccepted(),
         *      @ref pointerEventCapturedNode()
         */
        bool pointerPressEvent(const Vector2& globalPosition, PointerEvent& event);

        /**
         * @brief Handle a pointer release event
         *
         * Implicitly calls @ref update() and @ref clean(). The
         * @p globalPosition is assumed to be in respect to @ref windowSize(),
         * and is internally scaled to match @ref size() before being set to
         * @ref PointerEvent.
         *
         * If a node was captured by a previous @ref pointerPressEvent() or
         * @ref pointerMoveEvent(), @ref pointerReleaseEvent() wasn't called
         * yet and the node wasn't removed since, calls
         * @ref AbstractLayer::pointerReleaseEvent() on that node even if it
         * happens outside of its area, with the event position made relative
         * to the node. Returns @cpp true @ce if the event was accepted by the
         * captured node, @cpp false @ce if it wasn't and thus the event should
         * be propagated further. The capture is implicitly released after
         * calling this function independently of whether
         * @ref PointerEvent::setCaptured() was set by the implementation.
         *
         * Otherwise, if a node wasn't captured, finds the front-most nodes
         * under (scaled) @p globalPosition and then backtracks to parent
         * nodes. For each such node calls
         * @ref AbstractLayer::pointerReleaseEvent() on all data belonging to
         * layers that support @ref LayerFeature::Event until one of them
         * accepts the event. For each call the event position is made relative
         * to the node to which the data is attached. Returns @cpp true @ce if
         * the event was accepted, @cpp false @ce if it wasn't or there wasn't
         * any visible event handling node at given position and thus the event
         * should be propagated further.
         *
         * Expects that the event is not accepted yet.
         * @see @ref PointerEvent::isAccepted(),
         *      @ref PointerEvent::setAccepted(),
         *      @ref pointerEventCapturedNode()
         */
        bool pointerReleaseEvent(const Vector2& globalPosition, PointerEvent& event);

        /**
         * @brief Handle a pointer move event
         *
         * Implicitly calls @ref update() and @ref clean(). The
         * @p globalPosition is assumed to be in respect to @ref windowSize(),
         * and is internally scaled to match @ref size() before being set to
         * @ref PointerEvent.
         *
         * If a node was captured by a previous @ref pointerPressEvent() or
         * @ref pointerMoveEvent(), @ref pointerReleaseEvent() wasn't called
         * yet and the node wasn't removed since, calls
         * @ref AbstractLayer::pointerMoveEvent() on that node even if it
         * happens outside of its area, with the event position made relative
         * to the node. If the move event happened inside the node area and is
         * accepted, the node is treated as hovered, otherwise as not hovered.
         * An @ref AbstractLayer::pointerEnterEvent() or
         * @relativeref{AbstractLayer,pointerLeaveEvent()} is then called for
         * the node if the node hover status changed with the same @p event
         * except for @ref PointerMoveEvent::relativePosition() which is reset
         * to a zero vector. No corresponding leave / enter event is called for
         * any other node in this case. Returns @cpp true @ce if the move event
         * was accepted by the captured node, @cpp false @ce if it wasn't and
         * thus the event should be propagated further; accept status of the
         * enter and leave events is ignored. If the move, enter or leave event
         * implementations called @ref PointerMoveEvent::setCaptured()
         * resulting in it being @cpp false @ce, the capture is released after
         * this function, otherwise it stays unchanged.
         *
         * Otherwise, if a node wasn't captured, finds the front-most nodes
         * under (scaled) @p globalPosition and then backtracks to parent
         * nodes. For each such node calls
         * @ref AbstractLayer::pointerMoveEvent() on all data belonging to
         * layers that support @ref LayerFeature::Event until one of them
         * accepts the event, in which case given node is then treated as
         * hovered; if no nodes accept the event, there's no hovered node. For
         * each call the event position is made relative to the node to which
         * the data is attached. If the currently hovered node changed, an
         * @ref AbstractLayer::pointerLeaveEvent() is called for a previously
         * hovered node if it exists, and then a corresponding
         * @ref AbstractLayer::pointerEnterEvent() is called on the current
         * node if it exists, in both cases with the same @p event except for
         * @ref PointerMoveEvent::relativePosition() which is reset to a zero
         * vector. Returns @cpp true @ce if the move event was accepted by any
         * node, @cpp false @ce if it wasn't or there wasn't any visible event
         * handling node at given position and thus the event should be
         * propagated further; accept status of the enter and leave events is
         * ignored. If the accepted move event or the enter event called
         * @ref PointerMoveEvent::setCaptured() resulting in it being
         * @cpp true @ce, the containing node implicitly captures all further
         * pointer events until and including a @ref pointerReleaseEvent() even
         * if they happen outside of its area, or until the capture is released
         * in a @ref pointerMoveEvent() again. Calling
         * @ref PointerMoveEvent::setCaptured() in the leave event has no
         * effect in this case.
         *
         * Expects that the event is not accepted yet.
         * @see @ref PointerEvent::isAccepted(),
         *      @ref PointerEvent::setAccepted(),
         *      @ref pointerEventCapturedNode(),
         *      @ref pointerEventHoveredNode()
         */
        bool pointerMoveEvent(const Vector2& globalPosition, PointerMoveEvent& event);

        /**
         * @brief Node captured by last pointer event
         *
         * Returns handle of a node that captured the last
         * @ref pointerPressEvent() or @ref pointerMoveEvent(). The captured
         * node then receives all following pointer events until and including
         * a @ref pointerReleaseEvent() even if they happen outside of its
         * area, or until the capture is released in a @ref pointerMoveEvent()
         * again.
         *
         * If no pointer press event was called yet, if the event wasn't
         * accepted by any node, if the capture was disabled or subsequently
         * released with @ref PointerEvent::setCaptured() or if a
         * @ref pointerReleaseEvent() was called since, returns
         * @ref NodeHandle::Null. It also becomes @ref NodeHandle::Null if the
         * node, any of its parents or data attached to the node were removed
         * or hidden and @ref update() was called since.
         *
         * The returned handle may be invalid if the node or any of its parents
         * were removed and @ref update() wasn't called since.
         */
        NodeHandle pointerEventCapturedNode() const;

        /**
         * @brief Node hovered by last pointer event
         *
         * Returns handle of a node that was under the pointer for the last
         * @ref pointerMoveEvent(). Such node already received a
         * @ref AbstractLayer::pointerEnterEvent(). Once a
         * @ref pointerMoveEvent() leaves its area, it will receive a
         * @ref AbstractLayer::pointerLeaveEvent(), and this becomes
         * @ref NodeHandle::Null. It's also @ref NodeHandle::Null if no pointer
         * move event was called yet or if the node, any of its parents or data
         * attached to the node were removed or hidden and @ref update() was
         * called since.
         *
         * The returned handle may be invalid if the node or any of its parents
         * were removed and @ref update() wasn't called since.
         */
        NodeHandle pointerEventHoveredNode() const;

    private:
        /* Used by removeNode() and clean() */
        MAGNUM_WHEE_LOCAL void removeNodeInternal(UnsignedInt id);
        /* Used by setNodeFlags(), addNodeFlags() and clearNodeFlags() */
        MAGNUM_WHEE_LOCAL void setNodeFlagsInternal(UnsignedInt id, NodeFlags flags);
        /* Used by setNodeOrder() and clearNodeOrder() */
        MAGNUM_WHEE_LOCAL void clearNodeOrderInternal(NodeHandle handle);
        /* Used by *Event() functions, returns a NodeHandle and a corresponding
           DataHandle on which an event was accepted */
        template<class Event, void(AbstractLayer::*function)(UnsignedInt, Event&)> MAGNUM_WHEE_LOCAL Containers::Pair<NodeHandle, DataHandle> callEvent(const Vector2& globalPositionScaled, UnsignedInt visibleNodeIndex, Event& event);
        template<class Event, void(AbstractLayer::*function)(UnsignedInt, Event&)> MAGNUM_WHEE_LOCAL Containers::Pair<NodeHandle, DataHandle> callEvent(const Vector2& globalPositionScaled, Event& event);

        struct State;
        Containers::Pointer<State> _state;
};

}}

#endif
