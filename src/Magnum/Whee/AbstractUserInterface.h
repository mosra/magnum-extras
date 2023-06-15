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
     * @ref LayerState::NeedsUpdate set, is implicitly reset once they no
     * longer have it set, which happens next time
     * @ref AbstractUserInterface::update() is called. Implied by
     * @ref UserInterfaceState::NeedsDataAttachmentUpdate,
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
     * @relativeref{UserInterfaceState,NeedsNodeUpdate},
     * @relativeref{UserInterfaceState,NeedsDataClean} and
     * @relativeref{UserInterfaceState,NeedsNodeClean}, so it's also set by
     * everything that sets those flags.
     */
    NeedsDataAttachmentUpdate = NeedsDataUpdate|(1 << 1),

    /**
     * @ref AbstractUserInterface::update() needs to be called to refresh the
     * visible node hierarchy and data attached to it after nodes were added or
     * removed, the top-level node order changed or node layout changed. Set
     * implicitly after every @ref AbstractUserInterface::createNode(),
     * @relativeref{AbstractUserInterface,setNodeFlags()},
     * @relativeref{AbstractUserInterface,addNodeFlags()},
     * @relativeref{AbstractUserInterface,clearNodeFlags()},
     * @relativeref{AbstractUserInterface,setNodeOrder()} and
     * @relativeref{AbstractUserInterface,clearNodeOrder()} call, is reset next
     * time @ref AbstractUserInterface::update() is called. Implies
     * @ref UserInterfaceState::NeedsDataAttachmentUpdate. Implied by
     * @relativeref{UserInterfaceState,NeedsNodeClean}, so it's also set by
     * everything that sets that flag.
     */
    NeedsNodeUpdate = NeedsDataAttachmentUpdate|(1 << 2),

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
    NeedsDataClean = NeedsDataAttachmentUpdate|(1 << 3),

    /**
     * @ref AbstractUserInterface::clean() needs to be called to prune child
     * hierarchies of removed nodes, data attached to those, and
     * no-longer-valid data attachments. Set implicitly after every
     * @relativeref{AbstractUserInterface,removeNode()} call, is reset next
     * time @ref AbstractUserInterface::clean() is called. Implies
     * @ref UserInterfaceState::NeedsNodeUpdate and
     * @ref UserInterfaceState::NeedsDataClean.
     */
    NeedsNodeClean = NeedsNodeUpdate|NeedsDataClean|(1 << 4),
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
*/
class MAGNUM_WHEE_EXPORT AbstractUserInterface {
    public:
        /** @brief Constructor */
        explicit AbstractUserInterface();

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
         *
         * Expects that @p instance was created with a @ref LayerHandle
         * returned from @ref createLayer() earlier, the handle is valid and
         * @ref setLayerInstance() wasn't called for the same handle yet.
         * @see @ref AbstractLayer::handle(),
         *      @ref isHandleValid(LayerHandle) const
         */
        void setLayerInstance(Containers::Pointer<AbstractLayer>&& instance);

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
         * @brief Node size
         *
         * Expects that @p handle is valid.
         * @see @ref isHandleValid(NodeHandle) const
         */
        Vector2 nodeSize(NodeHandle handle) const;

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
         * Implicitly calls @ref clean(); called implicitly from @ref draw()
         * and all event processing functions. If @ref state() contains none of
         * @ref UserInterfaceState::NeedsDataUpdate,
         * @ref UserInterfaceState::NeedsDataAttachmentUpdate or
         * @ref UserInterfaceState::NeedsNodeUpdate, this function is a no-op,
         * otherwise it performs a subset of the following depending on the
         * state:
         *
         * -    Orders visible nodes back-to-front for drawing, front-to-back
         *      for event processing and collects their data attachments
         * -    Calculates absolute offsets for visible nodes
         * -    Partitions node data attachments by the layer and then by draw
         *      order
         * -    Calls @ref AbstractLayer::update() with the partitioned data
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
         * Implicitly calls @ref update() and @ref clean(). Finds the
         * front-most nodes under @p globalPosition and then backtracks to
         * parent nodes. For each such node calls
         * @ref AbstractLayer::pointerPressEvent() on all data belonging to
         * layers that support @ref LayerFeature::Event until one of them
         * accepts the event. For each call the event position is made relative
         * to the node to which the data is attached. Returns @cpp true @ce if
         * the event was accepted, @cpp false @ce if it wasn't or there wasn't
         * any visible event handling node at given position and thus the event
         * should be propagated further.
         *
         * Expects that the event is not accepted yet.
         * @see @ref PointerEvent::isAccepted(),
         *      @ref PointerEvent::setAccepted(), @ref pointerReleaseEvent()
         */
        bool pointerPressEvent(const Vector2& globalPosition, PointerEvent& event);

        /**
         * @brief Handle a pointer release event
         *
         * Implicitly calls @ref update() and @ref clean(). Finds the
         * front-most nodes under @p globalPosition and then backtracks to
         * parent nodes. For each such node calls
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
         *      @ref PointerEvent::setAccepted(), @ref pointerPressEvent()
         */
        bool pointerReleaseEvent(const Vector2& globalPosition, PointerEvent& event);

    private:
        /* Used by removeNode() and clean() */
        MAGNUM_WHEE_LOCAL void removeNodeInternal(UnsignedInt id);
        /* Used by setNodeFlags(), addNodeFlags() and clearNodeFlags() */
        MAGNUM_WHEE_LOCAL void setNodeFlagsInternal(UnsignedInt id, NodeFlags flags);
        /* Used by setNodeOrder() and clearNodeOrder() */
        MAGNUM_WHEE_LOCAL void clearNodeOrderInternal(NodeHandle handle);
        /* Used by *Event() functions */
        template<class Event, void(AbstractLayer::*function)(UnsignedInt, Event&)> MAGNUM_WHEE_LOCAL bool callEvent(const Vector2& globalPosition, UnsignedInt visibleNodeIndex, Event& event);
        template<class Event, void(AbstractLayer::*function)(UnsignedInt, Event&)> MAGNUM_WHEE_LOCAL bool callEvent(const Vector2& globalPosition, Event& event);

        struct State;
        Containers::Pointer<State> _state;
};

}}

#endif
