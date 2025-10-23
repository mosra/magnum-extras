#ifndef Magnum_Ui_AbstractLayouter_h
#define Magnum_Ui_AbstractLayouter_h
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
 * @brief Class @ref Magnum::Ui::AbstractLayouter, enum @ref Magnum::Ui::LayouterFeature, @ref Magnum::Ui::LayouterState, enum set @ref Magnum::Ui::LayouterFeatures @ref Magnum::Ui::LayouterStates
 * @m_since_latest_{extras}
 */

#include <Corrade/Containers/Pointer.h>
#include <Magnum/Magnum.h>

#include "Magnum/Ui/Ui.h"
#include "Magnum/Ui/visibility.h"

namespace Magnum { namespace Ui {

/**
@brief Features supported by a layouter
@m_since_latest_{extras}

@see @ref LayouterFeatures, @ref AbstractLayouter::features()
*/
enum class LayouterFeature: UnsignedByte {
    /**
     * The layouter has always at most one layout assigned to a particular
     * node. Such layouts can be then queried with
     * @ref AbstractUserInterface::nodeUniqueLayout(), and
     * @ref AbstractLayouter::add() expects that given node doesn't have a
     * layout from given layouter assigned yet.
     */
    UniqueLayouts = 1 << 0,
};

/**
@debugoperatorenum{LayouterFeature}
@m_since_latest_{extras}
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, LayouterFeature value);

/**
@brief Set of features supported by a layouter
@m_since_latest_{extras}

@see @ref AbstractLayouter::features()
*/
typedef Containers::EnumSet<LayouterFeature> LayouterFeatures;

/**
@debugoperatorenum{LayouterFeatures}
@m_since_latest_{extras}
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, LayouterFeatures value);

CORRADE_ENUMSET_OPERATORS(LayouterFeatures)

/**
@brief Layouter state
@m_since_latest_{extras}

Used to decide whether @ref AbstractLayouter::update() (called from
@ref AbstractUserInterface::update()) need to be called to relayout the nodes
before the interface is drawn. See @ref UserInterfaceState for interface-wide
state.
@see @ref LayouterStates, @ref AbstractLayouter::state()
*/
enum class LayouterState: UnsignedByte {
    /**
     * @ref AbstractLayouter::update() (which is called from
     * @ref AbstractUserInterface::update()) needs to be called to recalculate
     * the layout after a change. Has to be explicitly set by the layouter
     * implementation using @ref AbstractLayouter::setNeedsUpdate(), is reset
     * next time @ref AbstractLayouter::update() is called. Implied by
     * @ref LayouterState::NeedsAssignmentUpdate.
     *
     * Note that there's also interface-wide
     * @ref UserInterfaceState::NeedsLayoutAssignmentUpdate, which is set when
     * the node hierarchy or the node layout assignments changed. The two flags
     * are set independently, but both of them imply
     * @ref AbstractLayouter::update() needs to be called.
     */
    NeedsUpdate = 1 << 0,

    /**
     * @ref AbstractLayouter::update() (which is called from
     * @ref AbstractUserInterface::update()) needs to be called to refresh the
     * layouts assigned to visible node hierarchy after the assignments were
     * changed. Set implicitly after every @ref AbstractLayouter::add() and
     * @ref AbstractLayouter::remove() call, is reset next time
     * @ref AbstractLayouter::update() is called. Implies
     * @ref LayouterState::NeedsUpdate.
     */
    NeedsAssignmentUpdate = NeedsUpdate|(1 << 1),
};

/**
@debugoperatorenum{LayouterState}
@m_since_latest_{extras}
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, LayouterState value);

/**
@brief Layouter states
@m_since_latest_{extras}

@see @ref AbstractLayouter::state()
*/
typedef Containers::EnumSet<LayouterState> LayouterStates;

/**
@debugoperatorenum{LayouterStates}
@m_since_latest_{extras}
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, LayouterStates value);

CORRADE_ENUMSET_OPERATORS(LayouterStates)

/**
@brief Base for layouters
@m_since_latest_{extras}
*/
class MAGNUM_UI_EXPORT AbstractLayouter {
    public:
        #ifdef DOXYGEN_GENERATING_OUTPUT
        class DebugIntegration; /* For documentation only */
        #endif

        /**
         * @brief Constructor
         * @param handle    Handle returned by
         *      @ref AbstractUserInterface::createLayouter()
         */
        explicit AbstractLayouter(LayouterHandle handle);

        /** @brief Copying is not allowed */
        AbstractLayouter(const AbstractLayouter&) = delete;

        /**
         * @brief Move constructor
         *
         * Performs a destructive move, i.e. the original object isn't usable
         * afterwards anymore.
         */
        AbstractLayouter(AbstractLayouter&&) noexcept;

        virtual ~AbstractLayouter();

        /** @brief Copying is not allowed */
        AbstractLayouter& operator=(const AbstractLayouter&) = delete;

        /** @brief Move assignment */
        AbstractLayouter& operator=(AbstractLayouter&&) noexcept;

        /**
         * @brief Layouter handle
         *
         * Returns the handle passed to the constructor.
         */
        LayouterHandle handle() const;

        /**
         * @brief Layouter handle
         *
         * Same as @ref handle(). Useful for passing the layouter instance
         * directly to APIs accepting just a @ref LayouterHandle.
         */
        /*implicit*/ operator LayouterHandle() const;

        /** @brief Features exposed by a layouter */
        LayouterFeatures features() const { return doFeatures(); }

        /**
         * @brief Layouter state
         *
         * See the @ref LayouterState enum for more information. By default no
         * flags are set.
         */
        LayouterStates state() const;

        /**
         * @brief Mark the layouter with @ref LayouterState::NeedsUpdate
         *
         * Meant to be called by layouter implementations when the layouts get
         * modified. See the flag for more information.
         * @see @ref state(), @ref update()
         */
        void setNeedsUpdate();

        /**
         * @brief Current capacity of the layout storage
         *
         * Can be at most 1048576. If @ref add() is called and there's no free
         * slots left, the internal storage gets grown.
         * @see @ref usedCount()
         */
        std::size_t capacity() const;

        /**
         * @brief Count of used items in the layout storage
         *
         * Always at most @ref capacity(). Expired handles are counted among
         * used as well. The operation is done with a @f$ \mathcal{O}(n) @f$
         * complexity where @f$ n @f$ is @ref capacity().
         */
        std::size_t usedCount() const;

        /**
         * @brief Whether a layout handle is valid
         *
         * A handle is valid if it has been returned from @ref add() before and
         * @ref remove() wasn't called on it yet. For
         * @ref LayouterDataHandle::Null always returns @cpp false @ce.
         */
        bool isHandleValid(LayouterDataHandle handle) const;

        /**
         * @brief Whether a layout handle is valid
         *
         * A shorthand for extracting a @ref LayouterHandle from @p handle
         * using @ref layoutHandleLayouter(), comparing it to @ref handle() and
         * if it's the same, calling @ref isHandleValid(LayouterDataHandle) const
         * with a @ref LayouterDataHandle extracted from @p handle using
         * @ref layoutHandleData(). See these functions for more information.
         * For @ref LayoutHandle::Null, @ref LayouterHandle::Null or
         * @ref LayouterDataHandle::Null always returns @cpp false @ce.
         */
        bool isHandleValid(LayoutHandle handle) const;

        /**
         * @brief Node which given layout is assigned to
         *
         * Expects that @p layout is valid. The returned handle is never
         * @ref NodeHandle::Null. See also @ref node(LayouterDataHandle) const
         * which is a simpler operation if the layout is already known to
         * belong to this layouter.
         *
         * The returned handle may be invalid if either the layout got assigned
         * to an invalid node in the first place or the node or any of its
         * parents were removed and @ref AbstractUserInterface::clean() wasn't
         * called since.
         *
         * Unlike with layer data, the node assignment cannot be changed after
         * layout creation.
         * @see @ref isHandleValid(LayoutHandle) const
         */
        NodeHandle node(LayoutHandle layout) const;

        /**
         * @brief Node which given layout is assigned to assuming it belongs to this layouter
         *
         * Expects that @p layout is valid. The returned handle is never
         * @ref NodeHandle::Null. See also @ref node(LayoutHandle) const which
         * additionally checks that the layout belongs to this layouter.
         *
         * The returned handle may be invalid if either the layout got assigned
         * to an invalid node in the first place or the node or any of its
         * parents were removed and @ref AbstractUserInterface::clean() wasn't
         * called since.
         * @see @ref isHandleValid(LayouterDataHandle) const
         */
        NodeHandle node(LayouterDataHandle layout) const;

        /**
         * @brief Nodes which the layouts are assigned to
         *
         * Meant to be used by layouter implementations to query node
         * assignments based on layout IDs or masks without knowing their full
         * handles, application code should use @ref node(LayoutHandle) const /
         * @ref node(LayouterDataHandle) const instead. Size of the returned
         * view is the same as @ref capacity(). Items that are
         * @ref NodeHandle::Null are corresponding to layouts that are freed.
         */
        Containers::StridedArrayView1D<const NodeHandle> nodes() const;

        /**
         * @brief Generation counters for all layouts
         *
         * Meant to be used by code that only gets layout IDs or masks but
         * needs the full handle, or for various diagnostic purposes such as
         * tracking handle recycling. Size of the returned view is the same as
         * @ref capacity(), individual items correspond to generations of
         * particular layout IDs. All values fit into the @ref LayoutHandle /
         * @ref LayouterDataHandle generation bits, @cpp 0 @ce denotes an
         * expired generation counter.
         *
         * Passing an ID along with the corresponding generation to
         * @ref layouterDataHandle() produces a @ref LayouterDataHandle,
         * passing that along with @ref handle() to @ref layoutHandle()
         * produces a @ref LayoutHandle. Use
         * @ref isHandleValid(LayouterDataHandle) const /
         * @ref isHandleValid(LayoutHandle) const to determine whether given
         * slot is actually used.
         */
        Containers::StridedArrayView1D<const UnsignedShort> generations() const;

        /**
         * @brief Set user interface size
         *
         * Used internally from @ref AbstractUserInterface::setSize() and
         * @ref AbstractUserInterface::setLayouterInstance(). Exposed just for
         * testing purposes, there should be no need to call this function
         * directly. Expects that the size is non-zero. Delegates to
         * @ref doSetSize(), see its documentation for more information about
         * the arguments.
         */
        void setSize(const Vector2& size);

        /**
         * @brief Clean layouts attached to no longer valid nodes
         *
         * Used internally from @ref AbstractUserInterface::clean(). Exposed
         * just for testing purposes, there should be no need to call this
         * function directly and doing so may cause internal
         * @ref AbstractUserInterface state update to misbehave. Assumes that
         * @p nodeHandleGenerations contains handle generation counters for all
         * nodes, where the index is implicitly the handle ID. They're used to
         * decide about node assignment validity, layouts with invalid node
         * assignment are then marked for deletion. Delegates to
         * @ref doClean(), see its documentation for more information about the
         * arguments.
         */
        void cleanNodes(const Containers::StridedArrayView1D<const UnsignedShort>& nodeHandleGenerations);

        /**
         * @brief Update selected top-level layouts
         *
         * Used internally from @ref AbstractUserInterface::update(). Exposed
         * just for testing purposes, there should be no need to call this
         * function directly and doing so may cause internal
         * @ref AbstractUserInterface state update to misbehave. Expects that
         * @ref setSize() was called at least once before this function, the
         * size of @p layoutIdsToUpdate is the same as @ref capacity(), and
         * that the @p nodeParents, @p nodeOffsets and @p nodeSizes views have
         * all the same size. The @p nodeParents, @p nodeOffsets and
         * @p nodeSizes views should be large enough to contain any valid node
         * ID. Delegates to @ref doUpdate(), see its documentation for more
         * information about the arguments.
         *
         * Calling this function resets @ref LayouterState::NeedsUpdate and
         * @ref LayouterState::NeedsAssignmentUpdate, however note that
         * behavior of this function is independent of @ref state() --- it
         * performs the update always regardless of what flags are set.
         */
        void update(Containers::BitArrayView layoutIdsToUpdate, const Containers::StridedArrayView1D<const UnsignedInt>& topLevelLayoutIds, const Containers::StridedArrayView1D<const NodeHandle>& nodeParents, const Containers::StridedArrayView1D<Vector2>& nodeOffsets, const Containers::StridedArrayView1D<Vector2>& nodeSizes);

    protected:
        /**
         * @brief Whether the layouter is a part of an user interface instance
         *
         * Returns @cpp true @ce if the layouter has been already passed to
         * @ref AbstractUserInterface::setLayouterInstance(), @cpp false @ce
         * otherwise. The function isn't public as it's intended to be used
         * only by the layouter implementation itself, not user code.
         * @see @ref ui()
         */
        bool hasUi() const;

        /**
         * @brief User interface instance the layouter is part of
         *
         * Expects that the layouter has been already passed to
         * @ref AbstractUserInterface::setLayerInstance(). The function isn't
         * public as it's intended to be used only by the layouter
         * implementation itself, not user code. Additionally, to prevent
         * undesirable usage patterns, only a const reference is exposed,
         * intended just for querying UI state, not modifying it.
         * @see @ref hasUi()
         */
        const AbstractUserInterface& ui() const;

        /**
         * @brief Add a layout assigned to given node
         * @param node      Node to assign the layout to
         * @return New layout handle
         *
         * Allocates a new handle in a free slot in the internal storage or
         * grows the storage if there's no free slots left. Expects that
         * there's at most 1048576 layouts. The returned handle can be removed
         * again with @ref remove().
         *
         * If the layouter advertises @ref LayouterFeature::UniqueLayouts, the
         * function expects that the layouter has been already passed to
         * @ref AbstractUserInterface::setLayouterInstance(), that @p node is
         * valid and doesn't have a layout from this layouter assigned yet,
         * i.e. that @ref AbstractUserInterface::nodeUniqueLayout(NodeHandle, LayouterHandle) const
         * returns @ref LayouterDataHandle::Null for @p node and @ref handle().
         *
         * If the layouter doesn't advertise
         * @ref LayouterFeature::UniqueLayouts, the function only expects the
         * @p node to not be @ref NodeHandle::Null and the same node is allowed
         * to be added multiple times to the same layouter. The concrete
         * behavior depends on a particular layouter implementation.
         *
         * Calling this function causes
         * @ref LayouterState::NeedsAssignmentUpdate to be set. The subclass is
         * meant to wrap this function in a public API and perform appropriate
         * initialization work there.
         * @see @ref AbstractUserInterface::isHandleValid(NodeHandle) const
         */
        LayoutHandle add(NodeHandle node);

        /**
         * @brief Remove a node from this layouter
         *
         * Expects that @p handle is valid. After this call,
         * @ref isHandleValid(LayoutHandle) const returns @cpp false @ce for
         * @p handle. See also @ref remove(LayouterDataHandle) which is a
         * simpler operation if the node is already known to belong to this
         * layouter.
         *
         * Calling this function causes
         * @ref LayouterState::NeedsAssignmentUpdate to be set. Other than
         * that, no flag is set to trigger a subsequent @ref cleanNodes() ---
         * instead the subclass is meant to wrap this function in a public API
         * and perform appropriate cleanup work directly there.
         */
        void remove(LayoutHandle handle);

        /**
         * @brief Remove a node from this layouter assuming it belongs to it
         *
         * Expects that @p handle is valid. After this call,
         * @ref isHandleValid(LayouterDataHandle) const returns @cpp false @ce
         * for @p handle. See also @ref remove(LayouterDataHandle) which
         * additionally checks that the node belongs to this layouter.
         *
         * Calling this function causes
         * @ref LayouterState::NeedsAssignmentUpdate to be set. Other than
         * that, no flag is set to trigger a subsequent @ref cleanNodes() ---
         * instead the subclass is meant to wrap this function in a public API
         * and perform appropriate cleanup work directly there.
         * @see @ref layoutHandleData()
         */
        void remove(LayouterDataHandle handle);

    private:
        friend AbstractUserInterface; /* for the ui() reference */

        /**
         * @brief Implementation for @ref features()
         *
         * Note that the value returned by this function is assumed to stay
         * constant during the whole layouter lifetime.
         */
        virtual LayouterFeatures doFeatures() const = 0;

        /**
         * @brief Set user interface size
         * @param size              Size of the user interface to which
         *      everything is positioned
         *
         * Implementation for @ref setSize(), which is called from
         * @ref AbstractUserInterface::setSize() whenever the UI size changes,
         * and from @ref AbstractUserInterface::setLayouterInstance(). The
         * implementation is expected to refresh internal state that depends
         * on the UI size.
         *
         * Note that compared to @ref AbstractLayer::doSetSize(), a followup
         * @ref doUpdate() call isn't implicitly made after UI size changed
         * because the layout may not be depending on it for anything.
         * Explicitly call @ref setNeedsUpdate() in the implementation if the
         * layout *is* depending on the UI size and requires an update after.
         *
         * Default implementation does nothing.
         */
        virtual void doSetSize(const Vector2& size);

        /**
         * @brief Clean no longer valid layouts
         * @param layoutIdsToRemove   Layout IDs to remove
         *
         * Implementation for @ref cleanNodes(), which is called from
         * @ref AbstractUserInterface::clean() (and transitively from
         * @ref AbstractUserInterface::update()) whenever
         * @ref UserInterfaceState::NeedsNodeClean or any of the states that
         * imply it are present in @ref AbstractUserInterface::state().
         *
         * The @p layoutIdsToRemove view has the same size as @ref capacity()
         * and is guaranteed to have bits set only for valid layout IDs, i.e.
         * layout IDs that are already removed are not set.
         *
         * This function may get also called with @p layoutIdsToRemove having
         * all bits zero.
         *
         * Default implementation does nothing.
         */
        virtual void doClean(Containers::BitArrayView layoutIdsToRemove);

        /**
         * @brief Update selected top-level layouts
         * @param[in] layoutIdsToUpdate Layout IDs to update
         * @param[in] topLevelLayoutIds Top-level layout IDs to update from
         * @param[in] nodeParents       Node parents indexed by node ID
         * @param[in,out] nodeOffsets   Node offsets indexed by node ID
         * @param[in,out] nodeSizes     Node sizes indexed by node ID
         *
         * Implementation for @ref update(), which is called from
         * @ref AbstractUserInterface::update() whenever
         * @ref UserInterfaceState::NeedsLayoutUpdate or any of the states that
         * imply it are present in @ref AbstractUserInterface::state(). Is
         * always called after @ref doClean(), with at least one
         * @ref doSetSize() call happening at some point before.
         *
         * The @p layoutIdsToUpdate view has the same size as @ref capacity()
         * and is guaranteed to have bits set only for valid layout IDs
         * assigned to nodes visible at the time this function is called. Node
         * handles corresponding to @p topLevelLayoutIds are available in
         * @ref nodes(), node IDs can be then extracted from the handles using
         * @ref nodeHandleId(). The node IDs then index into the
         * @p nodeParents, @p nodeOffsets and @p nodeSizes views, which all
         * have the same size and are guaranteed to be large enough to contain
         * any valid node ID. All @ref nodes() at indices corresponding to
         * @p topLevelLayoutIds are guaranteed to not be @ref NodeHandle::Null
         * at the time this function is called. The @p topLevelLayoutIds are
         * mutually disjoint hierarchies that don't depend on each other in any
         * way and thus are and can be processed in an arbitrary order. For
         * each top-level layout, the calculation should be done in a way that
         * satisfies the bounding size in @p nodeSizes for the node to which
         * the layout is assigned.
         *
         * The @p nodeOffsets and @p nodeSizes arrays contain offsets and sizes
         * set either directly via @ref AbstractUserInterface::setNodeOffset()
         * / @relativeref{AbstractUserInterface,setNodeSize()} or modified by a
         * layouter ran on the same node in an earlier step. The implementation
         * can then either take the offset and size and modify it, or overwrite
         * it with a completely different value. The arrays contain also
         * offsets and sizes for nodes different than those referenced from
         * @p layoutIdsToUpdate, such as nodes controlled by different
         * layouters or nodes positioned directly, and the implementation
         * should not modify those in any way. On the other hand it's expected
         * to update *all* layouts set in the mask, failing to do so will leave
         * particular node offsets and sizes at values that were set initially
         * or at intermediate values coming from a previous layouter.
         *
         * The @p nodeOffsets are expected to be relative to the parent node.
         * If the internal layout representation doesn't match the node
         * hierarchy, the implementation can make use of @p nodeParents to find
         * out appropriate parent node offset to relate to.
         *
         * Unlike @ref AbstractLayer::doUpdate(), calls to this function may
         * happen several times with different @p layoutIdsToUpdate and
         * @p topLevelLayoutIds, ordered relative to calls to other layouters
         * on which output this layouter may depend or wich may depend on
         * output of this layouter. The set of layout IDs in both arguments is
         * disjoint among the calls to this functions, i.e. the function is
         * never called twice with the same ID present. This function may get
         * also called with @p layoutIdsToUpdate having all bits zero and
         * @p topLevelLayoutIds being empty, for example when
         * @ref setNeedsUpdate() was called but the layouter doesn't have any
         * layouts currently visible.
         */
        virtual void doUpdate(Containers::BitArrayView layoutIdsToUpdate, const Containers::StridedArrayView1D<const UnsignedInt>& topLevelLayoutIds, const Containers::StridedArrayView1D<const NodeHandle>& nodeParents, const Containers::StridedArrayView1D<Vector2>& nodeOffsets, const Containers::StridedArrayView1D<Vector2>& nodeSizes) = 0;

        /* Common implementation for remove(LayoutHandle) and
           remove(LayouterDataHandle) */
        MAGNUM_UI_LOCAL void removeWithUniqueLayoutInternal(UnsignedInt id);
        /* Common implementation for removeWithUniqueLayoutInternal() and
           doClean(), as doClean() doesn't need to deal with unique layouts */
        MAGNUM_UI_LOCAL void removeInternal(UnsignedInt id);

        struct State;
        Containers::Pointer<State> _state;
};

#ifdef DOXYGEN_GENERATING_OUTPUT
/**
@brief Debug layer integration

If an inner type with this name is implemented on a layouter that's passed to
@ref DebugLayer::setLayouterName(const T&, const Containers::StringView&), the
@ref print() function is used by the @ref DebugLayerFlag::NodeInspect
functionality to provide additional details about all layout assignments coming
from given layouter. See @ref Ui-DebugLayer-integration for more information.
*/
/* While it'd be significantly simpler both for the library and for layouters
   to have this as a virtual base class that then gets subclassed with
   interfaces implemented, it's deliberately not done to avoid header
   dependencies as well as make it possible to DCE all debug-layer-related code
   if it isn't used. */
class AbstractLayouter::DebugIntegration {
    public:
        /**
         * @brief Print details about a particular layout
         * @param debug     Debug output where to print
         * @param layouter  Layouter associated with given @p layout. The
         *      implementation can use either the layouter type this class is
         *      part of or any base type.
         * @param layouterName  Layouter name that was passed to
         *      @ref DebugLayer::setLayouterName(const T&, const Containers::StringView&)
         * @param layout    Layout to print info about. Guaranteed to be valid.
         *
         * Used internally by @ref DebugLayer. To fit among other info provided
         * by @ref DebugLayer itself, the implementation is expected to indent
         * the output by at least two spaces and end with a newline (i.e.,
         * @relativeref{Magnum,Debug::newline}).
         */
        void print(Debug& debug, const Layouter& layouter, Containers::StringView layouterName, LayouterDataHandle layout);
};
#endif

}}

#endif
