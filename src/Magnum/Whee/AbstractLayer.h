#ifndef Magnum_Whee_AbstractLayer_h
#define Magnum_Whee_AbstractLayer_h
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
 * @brief Class @ref Magnum::Whee::AbstractLayer, enum @ref Magnum::Whee::LayerFeature, @ref Magnum::Whee::LayerState, enum set @ref Magnum::Whee::LayerFeatures, @ref Magnum::Whee::LayerStates
 * @m_since_latest
 */

#include <Corrade/Containers/EnumSet.h>
#include <Corrade/Containers/Pointer.h>
#include <Magnum/Magnum.h>

#include "Magnum/Whee/Whee.h"
#include "Magnum/Whee/visibility.h"

namespace Magnum { namespace Whee {

/**
@brief Features supported by a layer
@m_since_latest

@see @ref LayerFeatures, @ref AbstractLayer::features()
*/
enum class LayerFeature: UnsignedByte {
    /** Drawing using @ref AbstractLayer::draw() */
    Draw = 1 << 0,

    /**
     * Event handling using @ref AbstractLayer::pointerPressEvent(),
     * @relativeref{AbstractLayer,pointerReleaseEvent()} and
     * @relativeref{AbstractLayer,pointerMoveEvent()}.
     */
    Event = 1 << 1
};

/**
@debugoperatorenum{LayerFeature}
@m_since_latest
*/
MAGNUM_WHEE_EXPORT Debug& operator<<(Debug& debug, LayerFeature value);

/**
@brief Set of features supported by a layer
@m_since_latest

@see @ref AbstractLayer::features()
*/
typedef Containers::EnumSet<LayerFeature> LayerFeatures;

/**
@debugoperatorenum{LayerFeatures}
@m_since_latest
*/
MAGNUM_WHEE_EXPORT Debug& operator<<(Debug& debug, LayerFeatures value);

CORRADE_ENUMSET_OPERATORS(LayerFeatures)

/**
@brief Layer state
@m_since_latest

Used to decide whether @ref AbstractLayer::cleanNodes() (called from
@ref AbstractUserInterface::clean()) or @ref AbstractLayer::update() (called
from @ref AbstractUserInterface::update()) need to be called to refresh the
internal state before the interface is drawn or an event is handled. See
@ref UserInterfaceState for interface-wide state.
@see @ref LayerStates, @ref AbstractLayer::state()
*/
enum class LayerState: UnsignedByte {
    /**
     * @ref AbstractLayer::update() (which is called from
     * @ref AbstractUserInterface::update()) needs to be called to recalculate
     * or reupload data after they've been changed. Has to be explicitly set by
     * the layer implementation using @ref AbstractLayer::setNeedsUpdate(), is
     * reset next time @ref AbstractLayer::update() is called. Implied by
     * @ref LayerState::NeedsAttachmentUpdate.
     *
     * This flag *isn't* set implicitly after a @ref AbstractLayer::create()
     * call, as newly created data only become a part of the visible node
     * hierarchy with @ref AbstractLayer::attach() (or
     * @ref AbstractUserInterface::attachData()).
     *
     * Note that there's also interface-wide
     * @ref UserInterfaceState::NeedsDataAttachmentUpdate, which is set when
     * the node hierarchy or the node data attachments changed. The two flags
     * are set independently, but both of them imply
     * @ref AbstractLayer::update() needs to be called.
     */
    NeedsUpdate = 1 << 0,

    /**
     * @ref AbstractLayer::update() (which is called from
     * @ref AbstractUserInterface::update()) needs to be called to refresh the
     * data attached to visible node hierarchy after the node attachments were
     * changed. Set implicitly after every @ref AbstractLayer::attach() call,
     * is reset next time @ref AbstractLayer::update() is called. Implies
     * @ref LayerState::NeedsUpdate.
     */
    NeedsAttachmentUpdate = NeedsUpdate|(1 << 1),

    /**
     * @ref AbstractLayer::cleanNodes() (which is called from
     * @ref AbstractUserInterface::clean()) needs to be called to prune state
     * belonging to no-longer-valid data. Set implicitly after every
     * @ref AbstractLayer::remove() call, is reset next time
     * @ref AbstractLayer::cleanNodes() is called.
     *
     * Note that there's also interface-wide
     * @ref UserInterfaceState::NeedsDataClean, which is set when nodes get
     * removed, to remove also the data attached to them. The two flags are set
     * independently, but both of them imply
     * @ref AbstractUserInterface::clean() needs to be called.
     */
    NeedsClean = 1 << 2
};

/**
@debugoperatorenum{LayerState}
@m_since_latest
*/
MAGNUM_WHEE_EXPORT Debug& operator<<(Debug& debug, LayerState value);

/**
@brief Layer states
@m_since_latest

@see @ref AbstractLayer::state()
*/
typedef Containers::EnumSet<LayerState> LayerStates;

/**
@debugoperatorenum{LayerStates}
@m_since_latest
*/
MAGNUM_WHEE_EXPORT Debug& operator<<(Debug& debug, LayerStates value);

CORRADE_ENUMSET_OPERATORS(LayerStates)

/**
@brief Base for data layers
@m_since_latest
*/
class MAGNUM_WHEE_EXPORT AbstractLayer {
    public:
        /**
         * @brief Constructor
         * @param handle    Handle returned by
         *      @ref AbstractUserInterface::createLayer()
         */
        explicit AbstractLayer(LayerHandle handle);

        /** @brief Copying is not allowed */
        AbstractLayer(const AbstractLayer&) = delete;

        /**
         * @brief Move constructor
         *
         * Performs a destructive move, i.e. the original object isn't usable
         * afterwards anymore.
         */
        AbstractLayer(AbstractLayer&&) noexcept;

        virtual ~AbstractLayer();

        /** @brief Copying is not allowed */
        AbstractLayer& operator=(const AbstractLayer&) = delete;

        /** @brief Move assignment */
        AbstractLayer& operator=(AbstractLayer&&) noexcept;

        /**
         * @brief Layer handle
         *
         * Returns the handle passed to the constructor.
         */
        LayerHandle handle() const;

        /** @brief Features exposed by a layer */
        LayerFeatures features() const { return doFeatures(); }

        /**
         * @brief Layer state
         *
         * See the @ref LayerState enum for more information. By default no
         * flags are set.
         */
        LayerStates state() const;

        /**
         * @brief Mark the layer with @ref LayerState::NeedsUpdate
         *
         * Meant to be called by layer implementations when the data get
         * modified. See the flag for more information.
         * @see @ref state(), @ref update()
         */
        void setNeedsUpdate();

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
         * @brief Whether a data handle is valid
         *
         * A handle is valid if it has been returned from @ref create() before
         * and @ref remove() wasn't called on it yet. For
         * @ref LayerDataHandle::Null always returns @cpp false @ce.
         */
        bool isHandleValid(LayerDataHandle handle) const;

        /**
         * @brief Whether a data handle is valid
         *
         * A shorthand for extracting a @ref LayerHandle from @p handle using
         * @ref dataHandleLayer(), comparing it to @ref handle() and if it's
         * the same, calling @ref isHandleValid(LayerDataHandle) const with a
         * @ref LayerDataHandle extracted from @p handle using
         * @ref dataHandleData(). See these functions for more information. For
         * @ref DataHandle::Null, @ref LayerHandle::Null or
         * @ref LayerDataHandle::Null always returns @cpp false @ce.
         */
        bool isHandleValid(DataHandle handle) const;

        /**
         * @brief Create a data
         * @return New data handle
         *
         * Allocates a new handle in a free slot in the internal storage or
         * grows the storage if there's no free slots left. Expects that
         * there's at most 1048576 data. The returned handle can be removed
         * again with @ref remove().
         */
        DataHandle create();

        /**
         * @brief Remove a data
         *
         * Expects that @p handle is valid. After this call,
         * @ref isHandleValid(DataHandle) const returns @cpp false @ce for
         * @p handle. See also @ref remove(LayerDataHandle) which is a simpler
         * operation if the data is already known to belong to this layer.
         *
         * Calling this function causes @ref LayerState::NeedsClean to be set.
         * If @p handle is attached to a node, calling this function also
         * causes @ref LayerState::NeedsAttachmentUpdate to be set.
         * @see @ref cleanNodes(), @ref node()
         */
        void remove(DataHandle handle);

        /**
         * @brief Remove a data assuming it belongs to this layer
         *
         * Expects that @p handle is valid. After this call,
         * @ref isHandleValid(LayerDataHandle) const returns @cpp false @ce for
         * @p handle. See also @ref remove(DataHandle) which additionally
         * checks that the data belongs to this layer.
         *
         * Calling this function causes @ref LayerState::NeedsClean to be set.
         * If @p handle is attached to a node, calling this function also
         * causes @ref LayerState::NeedsAttachmentUpdate to be set.
         * @see @ref dataHandleData(), @ref cleanNodes(), @ref node()
         */
        void remove(LayerDataHandle handle);

        /**
         * @brief Attach data to a node
         *
         * Makes the @p data handle tied to a particular @p node, meaning it
         * gets included in draw or event processing depending on node position
         * and visibility. Also, @ref AbstractUserInterface::removeNode()
         * called for @p node or any parent node will then mean that the
         * @p data gets scheduled for removal during the next @ref cleanNodes()
         * call.
         *
         * Expects that @p data is valid. The @p node can be anything including
         * @ref NodeHandle::Null, but if it's non-null and not valid the data
         * will be scheduled for deletion during the next @ref cleanNodes()
         * call. If the @p data is already attached to some node, this will
         * overwrite the previous attachment --- i.e., it's not possible to
         * have the same data attached to multiple nodes. The inverse,
         * attaching multiple different data handles to a single node, is
         * supported however.
         *
         * Calling this function causes @ref LayerState::NeedsAttachmentUpdate
         * to be set.
         * @see @ref isHandleValid(DataHandle) const,
         *      @ref AbstractUserInterface::attachData()
         */
        void attach(DataHandle data, NodeHandle node);

        /**
         * @brief Attach data to a node assuming it belongs to this layer
         *
         * Like @ref attach(DataHandle, NodeHandle) but without checking that
         * @p data indeed belongs to this layer. See its documentation for more
         * information.
         */
        void attach(LayerDataHandle data, NodeHandle node);

        /**
         * @brief Node attachment for given data
         *
         * Expects that @p data is valid. If given data isn't attached to any
         * node, returns @ref NodeHandle::Null. See also
         * @ref node(LayerDataHandle) const which is a simpler operation if the
         * data is already known to belong to this layer.
         *
         * The returned handle may be invalid if either the data got attached
         * to an invalid node in the first place or the node or any of its
         * parents were removed and @ref AbstractUserInterface::clean() wasn't
         * called since.
         * @see @ref isHandleValid(DataHandle) const
         */
        NodeHandle node(DataHandle data) const;

        /**
         * @brief Node attachment for given data assuming it belongs to this layer
         *
         * Like @ref node(DataHandle) const but without checking that @p data
         * indeed belongs to this layer. See its documentation for more
         * information.
         * @see @ref isHandleValid(LayerDataHandle) const,
         *      @ref dataHandleData()
         */
        NodeHandle node(LayerDataHandle data) const;

        /**
         * @brief Generation counters for all data
         *
         * Used internally from @ref AbstractUserInterface::update(). Size of
         * the returned view is the same as @ref capacity().
         */
        Containers::StridedArrayView1D<const UnsignedShort> generations() const;

        /**
         * @brief Node attachments for all data
         *
         * Used internally from @ref AbstractUserInterface::update(), meant to
         * be also used by @ref doUpdate() implementations to map data IDs to
         * node handles. Size of the returned view is the same as
         * @ref capacity(). Items that are @ref NodeHandle::Null are either
         * data with no node attachments or corresponding to data that are
         * freed.
         */
        Containers::StridedArrayView1D<const NodeHandle> nodes() const;

        /**
         * @brief Set user interface size
         *
         * Used internally from @ref AbstractUserInterface::setSize() and
         * @ref AbstractUserInterface::setLayerInstance(). Exposed just for
         * testing purposes, there should be no need to call this function
         * directly. Expects that the layer supports @ref LayerFeature::Draw
         * and that both sizes are non-zero. Delegates to @ref doSetSize(), see
         * its documentation for more information about the arguments.
         */
        void setSize(const Vector2& size, const Vector2i& framebufferSize);

        /**
         * @brief Clean data attached to no longer valid nodes
         *
         * Used internally from @ref AbstractUserInterface::clean(). Exposed
         * just for testing purposes, there should be no need to call this
         * function directly and doing so may cause internal
         * @ref AbstractUserInterface state update to misbehave. Assumes that
         * @p nodeHandleGenerations contains handle generation counters for all
         * nodes, where the index is implicitly the handle ID. They're used to
         * decide about node attachment validity, data with invalid node
         * attachments are then removed. Delegates to @ref doClean(), see its
         * documentation for more information about the arguments.
         *
         * Calling this function resets @ref LayerState::NeedsClean, however
         * note that behavior of this function is independent of @ref state()
         * --- it performs the clean always regardless of what flags are set.
         */
        void cleanNodes(const Containers::StridedArrayView1D<const UnsignedShort>& nodeHandleGenerations);

        /**
         * @brief Update visible layer data to given offsets and positions
         *
         * Used internally from @ref AbstractUserInterface::update(). Exposed
         * just for testing purposes, there should be no need to call this
         * function directly and doing so may cause internal
         * @ref AbstractUserInterface state update to misbehave. Expects that
         * the @p dataIds and @p dataNodeIds views have the same size and the
         * @p nodeOffsets and @p nodeSizes have the same size. The
         * @p nodeOffsets and @p nodeSizes views should be large enough to
         * contain any ID from @p dataNodeIds. Delegates to @ref doUpdate(),
         * see its documentation for more information about the arguments.
         *
         * Calling this function resets @ref LayerState::NeedsUpdate and
         * @ref LayerState::NeedsAttachmentUpdate, however note that behavior
         * of this function is independent of @ref state() --- it performs the
         * update always regardless of what flags are set. The function can be
         * called even in case @ref LayerState::NeedsClean is set as long as
         * @p dataIds contain only IDs that aren't scheduled for removal, the
         * two states are independent.
         */
        void update(const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const Containers::StridedArrayView1D<const UnsignedInt>& nodeIds, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes);

        /**
         * @brief Draw a sub-range of visible layer data
         *
         * Used internally from @ref AbstractUserInterface::draw(). Exposed
         * just for testing purposes, there should be no need to call this
         * function directly. Expects that the layer supports
         * @ref LayerFeature::Draw, the @p dataIds and @p dataNodeIds views
         * have the same size, @p offset and @p count fits into their size, and
         * that the @p nodeOffsets and @p nodeSizes view have the same size.
         * The @p nodeOffsets and @p nodeSizes views should be large enough to
         * contain any ID from @p dataNodeIds. Delegates to @ref doDraw(), see
         * its documentation for more information about the arguments.
         * @see @ref features()
         */
        void draw(const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const Containers::StridedArrayView1D<const UnsignedInt>& nodeIds, std::size_t offset, std::size_t count, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes);

        /**
         * @brief Handle a pointer press event
         *
         * Used internally from @ref AbstractUserInterface::pointerPressEvent().
         * Exposed just for testing purposes, there should be no need to call
         * this function directly. Expects that the layer supports
         * @ref LayerFeature::Event and @p dataId is less than @ref capacity(),
         * with the assumption that the ID points to a valid data and
         * @ref PointerEvent::position() is relative to the node to which the
         * data is attached. The event is expected to not be accepted yet.
         * Delegates to @ref doPointerPressEvent(), see its documentation for
         * more information.
         * @see @ref PointerEvent::isAccepted(),
         *      @ref PointerEvent::setAccepted()
         */
        void pointerPressEvent(UnsignedInt dataId, PointerEvent& event);

        /**
         * @brief Handle a pointer release event
         *
         * Used internally from @ref AbstractUserInterface::pointerReleaseEvent().
         * Exposed just for testing purposes, there should be no need to call
         * this function directly. Expects that the layer supports
         * @ref LayerFeature::Event and @p dataId is less than @ref capacity(),
         * with the assumption that the ID points to a valid data and
         * @ref PointerEvent::position() is relative to the node to which the
         * data is attached. The event is expected to not be accepted yet.
         * Delegates to @ref doPointerReleaseEvent(), see its documentation for
         * more information.
         * @see @ref PointerEvent::isAccepted(),
         *      @ref PointerEvent::setAccepted()
         */
        void pointerReleaseEvent(UnsignedInt dataId, PointerEvent& event);

        /**
         * @brief Handle a pointer move event
         *
         * Used internally from @ref AbstractUserInterface::pointerMoveEvent().
         * Exposed just for testing purposes, there should be no need to call
         * this function directly. Expects that the layer supports
         * @ref LayerFeature::Event and @p dataId is less than @ref capacity(),
         * with the assumption that the ID points to a valid data and
         * @ref PointerMoveEvent::position() is relative to the node to which
         * the data is attached. The event is expected to not be accepted yet.
         * Delegates to @ref doPointerMoveEvent(), see its documentation for
         * more information.
         * @see @ref PointerMoveEvent::isAccepted(),
         *      @ref PointerMoveEvent::setAccepted()
         */
        void pointerMoveEvent(UnsignedInt dataId, PointerMoveEvent& event);

        /**
         * @brief Handle a pointer enter event
         *
         * Used internally from @ref AbstractUserInterface::pointerMoveEvent().
         * Exposed just for testing purposes, there should be no need to call
         * this function directly. Expects that the layer supports
         * @ref LayerFeature::Event and @p dataId is less than @ref capacity(),
         * with the assumption that the ID points to a valid data and
         * @ref PointerMoveEvent::position() is relative to the node to which
         * the data is attached, and @ref PointerMoveEvent::relativePosition()
         * is a zero vector, given that the event is meant to happen right
         * after another event. The event is expected to not be accepted yet.
         * Delegates to @ref doPointerEnterEvent(), see its documentation for
         * more information.
         * @see @ref PointerMoveEvent::isAccepted(),
         *      @ref PointerMoveEvent::setAccepted()
         */
        void pointerEnterEvent(UnsignedInt dataId, PointerMoveEvent& event);

        /**
         * @brief Handle a pointer leave event
         *
         * Used internally from @ref AbstractUserInterface::pointerMoveEvent().
         * Exposed just for testing purposes, there should be no need to call
         * this function directly. Expects that the layer supports
         * @ref LayerFeature::Event and @p dataId is less than @ref capacity(),
         * with the assumption that the ID points to a valid data and
         * @ref PointerMoveEvent::position() is relative to the node to which
         * the data is attached, and @ref PointerMoveEvent::relativePosition()
         * is a zero vector, given that the event is meant to happen right
         * after another event. The event is expected to not be accepted yet.
         * Delegates to @ref doPointerLeaveEvent(), see its documentation for
         * more information.
         * @see @ref PointerMoveEvent::isAccepted(),
         *      @ref PointerMoveEvent::setAccepted()
         */
        void pointerLeaveEvent(UnsignedInt dataId, PointerMoveEvent& event);

    private:
        /** @brief Implementation for @ref features() */
        virtual LayerFeatures doFeatures() const = 0;

        /**
         * @brief Set user interface size
         * @param size              Size of the user interface to which
         *      everything including events is positioned
         * @param framebufferSize   Size of the window framebuffer
         *
         * Implementation for @ref setSize(), which is called from
         * @ref AbstractUserInterface::setSize() whenever the UI size or the
         * framebuffer size changes, and from
         * @ref AbstractUserInterface::setLayerInstance() if the UI size is
         * already set at the time the function is called. Called only if
         * @ref LayerFeature::Draw is supported. The implementation is expected
         * to update its internal rendering state such as projection matrices
         * and other framebuffer-related state. If the size change led to a
         * change in node positioning, the implementation can expect a followup
         * @ref doUpdate() call with up-to-date node positions and sizes.
         *
         * Default implementation does nothing.
         */
        virtual void doSetSize(const Vector2& size, const Vector2i& framebufferSize);

        /**
         * @brief Clean no longer valid layer data
         * @param dataIdsToRemove   Data IDs to remove
         *
         * Implementation for @ref cleanNodes(), which is called from
         * @ref AbstractUserInterface::clean(). The @p dataIdsToRemove view has
         * the same size as @ref capacity() and is guaranteed to have bits set
         * only for valid data IDs, i.e. data IDs that are already removed are
         * not set.
         *
         * Default implementation does nothing.
         */
        virtual void doClean(Containers::BitArrayView dataIdsToRemove);

        /**
         * @brief Update visible layer data to given offsets and positions
         * @param dataIds           Data IDs to update, in order that matches
         *      the draw order
         * @param dataNodeIds       Node IDs to which the data are attached
         * @param nodeOffsets       Absolute node offsets
         * @param nodeSizes         Node sizes
         *
         * Implementation for @ref update(), which is called from
         * @ref AbstractUserInterface::update(). The @p dataIds and
         * @p dataNodeIds views have the same size, the node IDs then index
         * into the @p nodeOffsets and @p nodeSizes views. The @p nodeOffsets
         * and @p nodeSizes have the same size and are guaranteed to be large
         * enough to contain any ID from @p dataNodeIds. Note that, however,
         * the arrays may contain random or uninitialized values for nodes not
         * referenced from @p dataNodeIds, such as for nodes that are not
         * currently visible or freed node handles.
         *
         * Default implementation does nothing. Data passed to this function
         * are subsequently passed to @ref doDraw() calls as well, the only
         * difference is that @ref doUpdate() gets called just once with all
         * data to update, while @ref doDraw() is called several times with
         * different sub-ranges of the data based on desired draw order.
         */
        virtual void doUpdate(const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const Containers::StridedArrayView1D<const UnsignedInt>& dataNodeIds, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes);

        /**
         * @brief Draw a sub-range of visible layer data
         * @param dataIds           Data IDs to update, in order that matches
         *      the draw order. Same as the view passed to @ref doUpdate()
         *      earlier.
         * @param dataNodeIds       Node IDs to which the data handles are
         *      attached. Same as the view passed to @ref doUpdate() earlier.
         * @param offset            Offset into @p dataIds and @p dataNodeIds
         * @param count             Count of @p dataIds and @p dataNodeIds to
         *      draw
         * @param nodeOffsets       Absolute node offsets. Same as the view
         *      passed to @ref doUpdate() earlier.
         * @param nodeSizes         Node sizes. Same as the view passed to
         *      @ref doUpdate() earlier.
         *
         * Implementation for @ref draw(), which is called from
         * @ref AbstractUserInterface::draw(). Called only if
         * @ref LayerFeature::Draw is supported, it's guaranteed that
         * @ref doUpdate() was called at some point before this function with
         * the exact same views passed to @p dataIds, @p dataNodeIds,
         * @p nodeOffsets and @p nodeSizes, see its documentation for their
         * relations and constraints.
         *
         * This function usually gets called several times with the same views
         * but different @p offset and @p count values in order to interleave
         * the draws for a correct back-to-front order.
         */
        virtual void doDraw(const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const Containers::StridedArrayView1D<const UnsignedInt>& dataNodeIds, std::size_t offset, std::size_t count, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes);

        /**
         * @brief Handle a pointer press event
         * @param dataId            Data ID the event happens on. Guaranteed to
         *      be less than @ref capacity() and point to a valid data.
         * @param event             Event data, with
         *      @ref PointerEvent::position() relative to the node to which the
         *      data is attached. The position is guaranteed to be within the
         *      area of the node.
         *
         * Implementation for @ref pointerPressEvent(), which is called from
         * @ref AbstractUserInterface::pointerPressEvent(). See its
         * documentation for more information about pointer event behavior,
         * especially event capture. It's guaranteed that @ref doUpdate() was
         * called before this function with up-to-date data for @p dataId.
         *
         * If the implementation handles the event, it's expected to call
         * @ref PointerEvent::setAccepted() on it to prevent it from being
         * propagated further. To disable implicit pointer event capture, call
         * @ref PointerEvent::setCaptured().
         *
         * Default implementation does nothing, i.e. the @p event gets
         * implicitly propagated further.
         */
        virtual void doPointerPressEvent(UnsignedInt dataId, PointerEvent& event);

        /**
         * @brief Handle a pointer release event
         * @param dataId            Data ID the event happens on. Guaranteed to
         *      be less than @ref capacity() and point to a valid data.
         * @param event             Event data, with
         *      @ref PointerEvent::position() relative to the node to which the
         *      data is attached. If pointer event capture is active, the
         *      position can be outside of the area of the node.
         *
         * Implementation for @ref pointerReleaseEvent(), which is called from
         * @ref AbstractUserInterface::pointerReleaseEvent(). See its
         * documentation for more information about pointer event behavior,
         * especially event capture. It's guaranteed that @ref doUpdate() was
         * called before this function with up-to-date data for @p dataId.
         *
         * If the implementation handles the event, it's expected to call
         * @ref PointerEvent::setAccepted() on it to prevent
         * it from being propagated further. Pointer capture is implicitly
         * released after this event, thus calling
         * @ref PointerEvent::setCaptured() has no effect.
         *
         * Default implementation does nothing, i.e. the @p event gets
         * implicitly propagated further.
         */
        virtual void doPointerReleaseEvent(UnsignedInt dataId, PointerEvent& event);

        /**
         * @brief Handle a pointer move event
         * @param dataId            Data ID the event happens on. Guaranteed to
         *      be less than @ref capacity() and point to a valid data.
         * @param event             Event data, with
         *      @ref PointerMoveEvent::position() relative to the node to which
         *      the data is attached. If pointer event capture is active, the
         *      position can be outside of the area of the node.
         *
         * Implementation for @ref pointerMoveEvent(), which is called from
         * @ref AbstractUserInterface::pointerMoveEvent(). See its
         * documentation for more information about pointer event behavior,
         * especially event capture, hover and relation to
         * @ref doPointerEnterEvent() and @ref doPointerLeaveEvent(). It's
         * guaranteed that @ref doUpdate() was called before this function with
         * up-to-date data for @p dataId.
         *
         * If the implementation handles the event, it's expected to call
         * @ref PointerEvent::setAccepted() on it to prevent it from being
         * propagated further. Pointer capture behavior for remaining pointer
         * events can be changed using @ref PointerMoveEvent::setCaptured().
         *
         * Default implementation does nothing, i.e. the @p event gets
         * implicitly propagated further. That also implies the node is never
         * marked as hovered and enter / leave events are not emitted for it.
         */
        virtual void doPointerMoveEvent(UnsignedInt dataId, PointerMoveEvent& event);

        /**
         * @brief Handle a pointer enter event
         * @param dataId            Data ID the event happens on. Guaranteed to
         *      be less than @ref capacity() and point to a valid data.
         * @param event             Event data, with
         *      @ref PointerMoveEvent::position() relative to the node to which
         *      the data is attached. If pointer event capture is active, the
         *      position can be outside of the area of the node.
         *
         * Implementation for @ref pointerEnterEvent(), which is called from
         * @ref AbstractUserInterface::pointerMoveEvent() if the currently
         * hovered node changed to one containing @p dataId. See its
         * documentation for more information about relation of pointer
         * enter/leave events to @ref doPointerMoveEvent(). It's guaranteed
         * that @ref doUpdate() was called before this function with up-to-date
         * data for @p dataId.
         *
         * Unlike @ref doPointerMoveEvent(), the accept status is ignored for
         * enter and leave events, as the event isn't propagated anywhere if
         * it's not handled. Thus calling @ref PointerEvent::setAccepted() has
         * no effect here. On the other hand, pointer capture behavior for
         * remaining pointer events can be changed using
         * @ref PointerMoveEvent::setCaptured() here as well.
         *
         * Default implementation does nothing.
         */
        virtual void doPointerEnterEvent(UnsignedInt dataId, PointerMoveEvent& event);

        /**
         * @brief Handle a pointer leave event
         * @param dataId            Data ID the event happens on. Guaranteed to
         *      be less than @ref capacity() and point to a valid data.
         * @param event             Event data, with
         *      @ref PointerMoveEvent::position() relative to the node to which
         *      the data is attached. If pointer event capture is active, the
         *      position can be outside of the area of the node.
         *
         * Implementation for @ref pointerEnterEvent(), which is called from
         * @ref AbstractUserInterface::pointerMoveEvent() if the currently
         * hovered node changed away from one containing @p dataId. See its
         * documentation for more information about relation of pointer
         * enter/leave events to @ref doPointerMoveEvent(). It's guaranteed
         * that @ref doUpdate() was called before this function with up-to-date
         * data for @p dataId.
         *
         * Unlike @ref doPointerMoveEvent(), the accept status is ignored for
         * enter and leave events, as the event isn't propagated anywhere if
         * it's not handled. Thus calling @ref PointerEvent::setAccepted() has
         * no effect here. On the other hand, pointer capture behavior for
         * remaining pointer events can be changed using
         * @ref PointerMoveEvent::setCaptured() here as well.
         *
         * Default implementation does nothing.
         */
        virtual void doPointerLeaveEvent(UnsignedInt dataId, PointerMoveEvent& event);

        /* Common implementations for foo(DataHandle, ...) and
           foo(LayerDataHandle, ...) */
        MAGNUM_WHEE_LOCAL void attachInternal(UnsignedInt id, NodeHandle node);
        MAGNUM_WHEE_LOCAL void removeInternal(UnsignedInt id);

        struct State;
        Containers::Pointer<State> _state;
};

}}

#endif
