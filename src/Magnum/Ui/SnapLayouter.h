#ifndef Magnum_Ui_SnapLayouter_h
#define Magnum_Ui_SnapLayouter_h
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
 * @brief Class @ref Magnum::Ui::SnapLayouter, enum @ref Magnum::Ui::Snap, @ref Magnum::Ui::SnapLayoutFlag, enum set @ref Magnum::Ui::Snaps, @ref Magnum::Ui::SnapLayoutFlags
 * @m_since_latest_{extras}
 */

#include "Magnum/Ui/AbstractLayouter.h"

namespace Magnum { namespace Ui {

/**
@brief Layout snap

See particular values for detailed documentation.

Specifying neither @ref Snap::Left nor @ref Snap::Right will result in
horizontal centering. Specifying both @ref Snap::Left and @ref Snap::Right (or
the @ref Snap::FillX alias) will cause the node width to match width of the
target node or the user interface. In both cases it's as if @ref Snap::InsideX
was specified as well.

Specifying neither @ref Snap::Top nor @ref Snap::Bottom will result in verical
centering. Specifying both @ref Snap::Top and @ref Snap::Bottom (or the
@ref Snap::FillX alias) will cause the node height to match height the target
node or user interface. In both cases it's as if @ref Snap::InsideY was
specified as well.

If the snap results in the node being outside of the target node, margin of
both nodes is taken into account. If the snap results in the node being inside
the target node, padding of the target node and margin of the snapped node is
taken into account.

Specifying @ref Snap::NoPadX and/or @ref Snap::NoPadY will ignore horizontal
and/or vertical padding and margin.
@see @ref Snaps,
    @ref SnapLayouter::add(NodeHandle, Snaps, LayoutHandle, SnapLayoutFlags)
*/
enum class Snap: UnsignedByte {
    /**
     * Snaps right side of a node to left side of the target node, taking
     * left-side snapped node margin and right-side target node margin into
     * account.
     *
     * If combined with @ref Snap::InsideX, snaps left side of a node to left
     * side of the target node or user interface, taking left-side target node
     * padding and left-side snapped node margin into account.
     */
    Left = 1 << 0,

    /**
     * Snaps bottom side of a node to top side of the target node, taking
     * bottom-side snapped node margin and top-side target node margin into
     * account.
     *
     * If combined with @ref Snap::InsideY, snaps top side of a node to top
     * side of the target node or user interface, taking top-side target node
     * padding and top-side snapped node margin into account.
     */
    Top = 1 << 1,

    /**
     * Snaps left side of a node to right side of the target node, taking
     * right-side snapped node margin and left-side target node margin into
     * account.
     *
     * If combined with @ref Snap::InsideX, snaps right side of a node to
     * right side of the target node or user interface, taking right-side
     * target node padding and right-side snapped node margin into account.
     */
    Right = 1 << 2,

    /**
     * Snaps top side of a node to bottom side of the target node, taking
     * bottom-side snapped node margin and top-side target node margin into
     * account.
     *
     * If combined with @ref Snap::InsideY, snaps bottom side of a node to
     * bottom side of the target node or user interface, taking bottom-side
     * target node padding and bottom-side node margin into account.
     */
    Bottom = 1 << 3,

    /**
     * Alias to specifying both @ref Snap::Top and @ref Snap::Left.
     * @m_since_latest_{extras}
     */
    TopLeft = Top|Left,

    /**
     * Alias to specifying both @ref Snap::Bottom and @ref Snap::Left.
     * @m_since_latest_{extras}
     */
    BottomLeft = Bottom|Left,

    /**
     * Alias to specifying both @ref Snap::Top and @ref Snap::Right.
     * @m_since_latest_{extras}
     */
    TopRight = Top|Right,

    /**
     * Alias to specifying both @ref Snap::Bottom and @ref Snap::Right.
     * @m_since_latest_{extras}
     */
    BottomRight = Bottom|Right,

    /**
     * Alias to specifying both @ref Snap::Left and @ref Snap::Right.
     * @m_since_latest_{extras}
     */
    FillX = Left|Right,

    /**
     * Alias to specifying both @ref Snap::Top and @ref Snap::Bottom.
     * @m_since_latest_{extras}
     */
    FillY = Top|Bottom,

    /**
     * Alias to specifying both @ref Snap::FillX and @ref Snap::FillY, or all
     * of @ref Snap::Left, @ref Snap::Right, @ref Snap::Top and
     * @ref Snap::Bottom.
     * @m_since_latest_{extras}
     */
    Fill = FillX|FillY,

    /**
     * When combined with either @ref Snap::Left or @ref Snap::Right, snaps
     * horizontally inside the target node instead of outside. Implicit when
     * either both or neither @ref Snap::Left and @ref Snap::Right are
     * specified, implicit also if snapping relative to the whole user
     * interface (i.e., when the target node is @ref NodeHandle::Null).
     */
    InsideX = 1 << 4,

    /**
     * When combined with either @ref Snap::Top or @ref Snap::Bottom, snaps
     * horizontally inside the target node instead of outside. Implicit when
     * either both or neither @ref Snap::Top and @ref Snap::Bottom are
     * specified, implicit also if snapping relative to the whole user
     * interface (i.e., when the target node is @ref NodeHandle::Null).
     */
    InsideY = 1 << 5,

    /**
     * Alias to specifying both @ref Snap::InsideX and @ref Snap::InsideY.
     * @m_since_latest_{extras}
     */
    Inside = InsideX|InsideY,

    /**
     * Ignore horizontal node margin and padding.
     * @m_since_latest_{extras}
     */
    NoPadX = 1 << 6,

    /**
     * Ignore vertical node margin and padding.
     * @m_since_latest_{extras}
     */
    NoPadY = 1 << 7,

    /**
     * Alias to specifying both @ref Snap::NoPadX and @ref Snap::NoPadY.
     * @m_since_latest_{extras}
     */
    NoPad = NoPadX|NoPadY
};

/**
@debugoperatorenum{Snap}
@m_since_latest_{extras}
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, Snap value);

/**
@brief Layout snaps

@see @ref SnapLayouter::add(NodeHandle, Snaps, LayoutHandle, SnapLayoutFlags)
*/
typedef Containers::EnumSet<Snap> Snaps;

/**
@debugoperatorenum{Snaps}
@m_since_latest_{extras}
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, Snaps value);

CORRADE_ENUMSET_OPERATORS(Snaps)

/**
@brief Snap layout flag
@m_since_latest_{extras}

@see @ref SnapLayoutFlags, @ref SnapLayouter::add()
*/
enum class SnapLayoutFlag: UnsignedByte {
    /**
     * Ignore horizontal overflow of child layouts instead of expanding node
     * width to contain them. Note that explicitly snapped layouts aren't
     * considered and may overflow regardless of this flag being set.
     */
    IgnoreOverflowX = 1 << 0,

    /**
     * Ignore horizontal overflow of child layouts instead of expanding node
     * height to contain them. Note that explicitly snapped layouts aren't
     * considered and may overflow regardless of this flag being set.
     */
    IgnoreOverflowY = 1 << 1,

    /**
     * Alias to specifying both @ref SnapLayoutFlag::IgnoreOverflowX and
     * @ref SnapLayoutFlag::IgnoreOverflowY.
     */
    IgnoreOverflow = IgnoreOverflowX|IgnoreOverflowY,
};

/**
@debugoperatorenum{SnapLayoutFlag}
@m_since_latest_{extras}
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, SnapLayoutFlag value);

/**
@brief Snap layout flags
@m_since_latest_{extras}

@see @ref SnapLayouter::add()
*/
typedef Containers::EnumSet<SnapLayoutFlag> SnapLayoutFlags;

/**
@debugoperatorenum{SnapLayoutFlags}
@m_since_latest_{extras}
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, SnapLayoutFlags value);

CORRADE_ENUMSET_OPERATORS(SnapLayoutFlags)

/**
@brief Snap layouter
@m_since_latest_{extras}

@see @ref Snap, @ref Snaps, @ref UserInterface::snapLayouter(),
    @ref UserInterface::setSnapLayouterInstance(),
    @ref StyleFeature::SnapLayouter
*/
class MAGNUM_UI_EXPORT SnapLayouter: public AbstractLayouter {
    public:
        /**
         * @brief Constructor
         * @param handle    Layouter handle returned from
         *      @ref AbstractUserInterface::createLayouter()
         */
        explicit SnapLayouter(LayouterHandle handle);

        /** @brief Copying is not allowed */
        SnapLayouter(const SnapLayouter&) = delete;

        /**
         * @brief Move constructor
         *
         * Performs a destructive move, i.e. the original object isn't usable
         * afterwards anymore.
         */
        SnapLayouter(SnapLayouter&&) noexcept;

        virtual ~SnapLayouter();

        /** @brief Copying is not allowed */
        SnapLayouter& operator=(const SnapLayouter&) = delete;

        /** @brief Move assignment */
        SnapLayouter& operator=(SnapLayouter&&) noexcept;

        /**
         * @brief Add a layout assigned to given node
         * @param node      Node to assign the layout to
         * @param before    A layout to order before or
         *      @ref LayoutHandle::Null if ordered as last
         * @param flags     Layout flags
         * @return New layout handle
         *
         * The @p node is expected to not have a layout assigned from this
         * layouter yet. If @p before is not @ref LayoutHandle::Null, it's
         * expected to be valid, belong to the same layouter and have the same
         * non-null parent layout as @p node.
         *
         * If @p node parent has a layout coming from this layouter, the
         * positioning is done according to @ref childSnap() defined by the
         * parent layout, and the total size and padding of all child layouts
         * then affects parent layout size and padding according to @p flags.
         * If @p node parent doesn't have a layout coming from this layouter,
         * the layout acts as a parent for child layouts or as a target for
         * explicitly snapped layouts and isn't affected by layout calculation.
         *
         * Use @ref add(NodeHandle, Snaps, LayoutHandle, SnapLayoutFlags) for
         * explicitly snapping to given layout without taking @ref childSnap()
         * into account and without affecting the parent layout size.
         *
         * Delegates to @ref AbstractLayouter::add(), see its documentation for
         * more information.
         * @see @ref isHandleValid(LayoutHandle) const,
         *      @ref Ui::AbstractUserInterface::nodeParent(),
         *      @ref Ui::AbstractUserInterface::nodeUniqueLayout()
         */
        LayoutHandle add(NodeHandle node, LayoutHandle before =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            LayoutHandle::Null
            #else
            LayoutHandle{} /* To not have to include Handle.h */
            #endif
            , SnapLayoutFlags flags = {});
        /** @overload */
        LayoutHandle add(NodeHandle node, SnapLayoutFlags flags) {
            return add(node, LayoutHandle{}, flags);
        }

        /**
         * @brief Add a layout assigned to given node, and before given layout assuming the layout belongs to this layouter
         *
         * Like @ref add(NodeHandle, LayoutHandle, SnapLayoutFlags) but without
         * checking that @p before indeed belongs to this layouter. See its
         * documentation for more information.
         * @see @ref isHandleValid(LayouterDataHandle) const,
         *      @ref layoutHandleData()
         */
        LayoutHandle add(NodeHandle node, LayouterDataHandle before, SnapLayoutFlags flags = {});

        /**
         * @brief Add a layout snapping explicitly to given target
         * @param node          Node to assign the layout to
         * @param snap          How to snap
         * @param snapTarget    Target layout to snap to or
         *      @ref LayoutHandle::Null to snap a root @p node to the UI itself
         * @param flags         Layout flags
         * @return New layout handle
         *
         * If @p node is root, @p snapTarget can be @ref LayoutHandle::Null to
         * snap to the whole UI. Otherwise @p snapTarget is expected to be
         * coming from the same layouter and assigned to either a parent or a
         * sibling of @p node.
         *
         * Note that explicitly snapped layouts aren't considered in any way
         * when calculating parent layout sizes and paddings and thus may
         * overflow the parent node area. Use @ref add(NodeHandle, LayoutHandle, SnapLayoutFlags)
         * for implicit child layouts which then propagate their total size and
         * margin to the parent.
         *
         * Delegates to @ref AbstractLayouter::add(), see its documentation for
         * more information.
         */
        LayoutHandle add(NodeHandle node, Snaps snap, LayoutHandle snapTarget, SnapLayoutFlags flags = {});

        /**
         * @brief Add a layout snapping explicitly to given target, assuming the target belongs to this layouter
         *
         * Like @ref add(NodeHandle, Snaps, LayoutHandle, SnapLayoutFlags) but
         * without checking that @p snapTarget indeed belongs to this layouter.
         * See its documentation for more information.
         */
        LayoutHandle add(NodeHandle node, Snaps snap, LayouterDataHandle snapTarget, SnapLayoutFlags flags = {});

        /**
         * @brief Remove a layout
         *
         * Expects that @p handle is valid and has no child layouts or
         * explicitly snapped layouts from the same layouter. To remove a
         * layout that has dependent layouts, either remove the dependent
         * layouts first or remove the whole node along with its children using
         * @ref Ui::AbstractUserInterface::removeNode().
         *
         * Delegates to @ref AbstractLayouter::remove(LayoutHandle), see its
         * documentation for more information.
         * @see @ref isHandleValid(LayoutHandle) const
         */
        void remove(LayoutHandle handle);

        /**
         * @brief Remove a layout assuming it belongs to this layer
         *
         * Compared to @ref remove(LayoutHandle) delegates to
         * @ref AbstractLayouter::remove(LayouterDataHandle) instead.
         * @see @ref isHandleValid(LayouterDataHandle) const,
         *      @ref layoutHandleData()
         */
        void remove(LayouterDataHandle handle);

        /**
         * @brief Layout flags
         *
         * Expects that @p handle is valid.
         * @see @ref isHandleValid(LayoutHandle) const
         */
        SnapLayoutFlags flags(LayoutHandle handle) const;

        /**
         * @brief Layout flags assuming it belongs to this layouter
         *
         * Like @ref flags(LayoutHandle) const but without checking that
         * @p handle indeed belongs to this layouter. See its documentation for
         * more information.
         * @see @ref isHandleValid(LayouterDataHandle) const,
         *      @ref layoutHandleData()
         */
        SnapLayoutFlags flags(LayouterDataHandle handle) const;

        /**
         * @brief Set layout flags
         *
         * Expects that @p handle is valid. Initially, a layout has the flags
         * that were passed to @ref add(), which are by default none.
         *
         * Calling this function causes @ref LayouterState::NeedsUpdate to be
         * set.
         * @see @ref isHandleValid(LayoutHandle) const, @ref addFlags(),
         *      @ref clearFlags()
         */
        void setFlags(LayoutHandle handle, SnapLayoutFlags flags);

        /**
         * @brief Set layout flags assuming it belongs to this layouter
         *
         * Like @ref setFlags(LayoutHandle, SnapLayoutFlags) but without
         * checking that @p handle indeed belongs to this layouter. See its
         * documentation for more information.
         * @see @ref isHandleValid(LayouterDataHandle) const,
         *      @ref layoutHandleData()
         */
        void setFlags(LayouterDataHandle handle, SnapLayoutFlags flags);

        /**
         * @brief Add layout flags
         *
         * Calls @ref setFlags() with the existing flags ORed with @p flags.
         * Useful for preserving previously set flags.
         * @see @ref clearFlags()
         */
        void addFlags(LayoutHandle handle, SnapLayoutFlags flags);

        /**
         * @brief Add layout flags assuming it belongs to this layouter
         *
         * Like @ref addFlags(LayoutHandle, SnapLayoutFlags) but without
         * checking that @p handle indeed belongs to this layouter. See its
         * documentation for more information.
         * @see @ref isHandleValid(LayouterDataHandle) const,
         *      @ref layoutHandleData()
         */
        void addFlags(LayouterDataHandle handle, SnapLayoutFlags flags);

        /**
         * @brief Clear layout flags
         *
         * Calls @ref setFlags() with the existing flags ANDed with the inverse
         * of @p flags. Useful for removing a subset of previously set flags.
         * @see @ref addFlags()
         */
        void clearFlags(LayoutHandle handle, SnapLayoutFlags flags);

        /**
         * @brief Clear layout flags assuming it belongs to this layouter
         *
         * Like @ref addFlags(LayoutHandle, SnapLayoutFlags) but without
         * checking that @p handle indeed belongs to this layouter. See its
         * documentation for more information.
         * @see @ref isHandleValid(LayouterDataHandle) const,
         *      @ref layoutHandleData()
         */
        void clearFlags(LayouterDataHandle handle, SnapLayoutFlags flags);

        /**
         * @brief Snap for child layouts
         *
         * Expects that @p handle is valid.
         * @see @ref isHandleValid(LayoutHandle) const
         */
        Snaps childSnap(LayoutHandle handle) const;

        /**
         * @brief Snap for child layouts assuming it belongs to this layouter
         *
         * Like @ref childSnap(LayoutHandle) const but without checking that
         * @p handle indeed belongs to this layouter. See its documentation for
         * more information.
         * @see @ref isHandleValid(LayouterDataHandle) const,
         *      @ref layoutHandleData()
         */
        Snaps childSnap(LayouterDataHandle handle) const;

        /**
         * @brief Set snap for child layouts
         *
         * Expects that @p handle is valid and that @p snap produces a
         * non-overlapping purely horizontal or vertical order. Default is
         * @ref Snap::Bottom, i.e. child nodes get placed one after each other
         * in a column.
         *
         * Calling this function causes @ref LayouterState::NeedsUpdate to be
         * set.
         * @see @ref isHandleValid(LayoutHandle) const
         */
        void setChildSnap(LayoutHandle handle, Snaps snap);

        /**
         * @brief Set snap for child layouts assuming it belongs to this layouter
         *
         * Like @ref setChildSnap(LayouterDataHandle, Snaps) but without
         * checking that @p handle indeed belongs to this layouter. See its
         * documentation for more information.
         * @see @ref isHandleValid(LayouterDataHandle) const,
         *      @ref layoutHandleData()
         */
        void setChildSnap(LayouterDataHandle handle, Snaps snap);

        /**
         * @brief First child layout
         *
         * Expects that @p handle is valid. If the layout has no children or if
         * all nested layouts have an explicit snap, returns
         * @ref LayoutHandle::Null. Otherwise, the handle is always valid and
         * belonging to this layouter.
         * @see @ref isHandleValid(LayoutHandle) const,
         *      @ref firstExplicitSnap(), @ref previous(), @ref next()
         */
        LayoutHandle firstChild(LayoutHandle handle) const;

        /**
         * @brief First child layout assuming it belongs to this layouter
         *
         * Like @ref firstChild(LayoutHandle) const but without checking that
         * @p handle indeed belongs to this layouter. See its documentation for
         * more information.
         * @see @ref isHandleValid(LayouterDataHandle) const,
         *      @ref layoutHandleData()
         */
        LayoutHandle firstChild(LayouterDataHandle handle) const;

        /**
         * @brief First layout explicitly snapped to this one
         *
         * Expects that @p handle is valid. If no other layouts are explicitly
         * snapped to this layout, returns @ref LayoutHandle::Null. Otherwise,
         * the handle is always valid and belonging to this layouter.
         * @see @ref isHandleValid(LayoutHandle) const,
         *      @ref firstExplicitSnap(), @ref previous(), @ref next()
         */
        LayoutHandle firstExplicitSnap(LayoutHandle handle) const;

        /**
         * @brief First layout explicitly snapped to this one assuming it belongs to this layouter
         *
         * Like @ref firstExplicitSnap(LayoutHandle) const but without checking
         * that @p handle indeed belongs to this layouter. See its
         * documentation for more information.
         * @see @ref isHandleValid(LayouterDataHandle) const,
         *      @ref layoutHandleData()
         */
        LayoutHandle firstExplicitSnap(LayouterDataHandle handle) const;

        /**
         * @brief Whether the layout has an explicit snap
         *
         * Expects that @p handle is valid. Returns @cpp true @ce if the layout
         * was created using @ref add(NodeHandle, Snaps, LayoutHandle, SnapLayoutFlags),
         * @cpp false @ce otherwise.
         */
        bool hasExplicitSnap(LayoutHandle handle) const;

        /**
         * @brief Whether the layout has an explicit snap assuming it belongs to this layouter
         *
         * Like @ref hasExplicitSnap(LayoutHandle) const but without checking
         * that @p handle indeed belongs to this layouter. See its
         * documentation for more information.
         * @see @ref isHandleValid(LayouterDataHandle) const,
         *      @ref layoutHandleData()
         */
        bool hasExplicitSnap(LayouterDataHandle handle) const;

        /**
         * @brief Parent layout
         *
         * Expects that @p handle is valid and doesn't have an explicit snap.
         * Layouts with an explicit snap are not following any parent/child
         * hierarchy. If there was no layout assigned to the parent node at the
         * time @p handle was created, or @p handle is assigned to a root node,
         * returns @ref LayoutHandle::Null. Otherwise, the handle is always
         * valid and belonging to this layouter.
         * @see @ref isHandleValid(LayoutHandle) const, @ref hasExplicitSnap(),
         *      @ref previous(), @ref next(), @ref firstChild()
         */
        LayoutHandle parent(LayoutHandle handle) const;

        /**
         * @brief Parent layout assuming it belongs to this layouter
         *
         * Like @ref parent(LayoutHandle) const but without checking that
         * @p handle indeed belongs to this layouter. See its documentation for
         * more information.
         * @see @ref isHandleValid(LayouterDataHandle) const,
         *      @ref layoutHandleData()
         */
        LayoutHandle parent(LayouterDataHandle handle) const;

        /**
         * @brief Previous layout
         *
         * Expects that @p handle is valid. If the handle doesn't have an
         * explicit snap, returns a previous sibling with the same @ref parent()
         * in layout order. If the handle has an explicit snap, returns a
         * previous layout with the same @ref explicitSnapTarget(), but their
         * relative order doesn't affect the layouting result in any way. If
         * the layout is first in given list, returns @ref LayoutHandle::Null.
         * Otherwise, the handle is always valid and belonging to this
         * layouter.
         * @see @ref isHandleValid(LayoutHandle) const, @ref hasExplicitSnap(),
         *      @ref next(), @ref firstChild(), @ref firstExplicitSnap()
         */
        LayoutHandle previous(LayoutHandle handle) const;

        /**
         * @brief Previous layout assuming it belongs to this layouter
         *
         * Like @ref previous(LayoutHandle) const but without checking that
         * @p handle indeed belongs to this layouter. See its documentation for
         * more information.
         * @see @ref isHandleValid(LayouterDataHandle) const,
         *      @ref layoutHandleData()
         */
        LayoutHandle previous(LayouterDataHandle handle) const;

        /**
         * @brief Next layout
         *
         * Expects that @p handle is valid. If the handle doesn't have an
         * explicit snap, returns a next sibling with the same @ref parent() in
         * layout order. If the handle has an explicit snap, returns a next
         * layout with the same @ref explicitSnapTarget(), but their relative
         * order doesn't affect the layouting result in any way. If the layout
         * is last among its siblings, returns @ref LayoutHandle::Null.
         * Otherwise, the handle is always valid and belonging to this layouter.
         * @see @ref isHandleValid(LayoutHandle) const, @ref hasExplicitSnap(),
         *      @ref previous(), @ref firstChild(), @ref firstExplicitSnap()
         */
        LayoutHandle next(LayoutHandle handle) const;

        /**
         * @brief Next sibling layout assuming it belongs to this layouter
         *
         * Like @ref next(LayoutHandle) const but without checking that
         * @p handle indeed belongs to this layouter. See its documentation for
         * more information.
         * @see @ref isHandleValid(LayouterDataHandle) const,
         *      @ref layoutHandleData()
         */
        LayoutHandle next(LayouterDataHandle handle) const;

        /**
         * @brief Explicit layout snap
         *
         * Expects that @p handle is valid and has an explicit snap. Note that
         * if @ref explicitSnapTarget() is @ref NodeHandle::Null,
         * @ref Snap::Inside is implicitly considered to be included as well,
         * even if not part of the actual value.
         *
         * Similarly to @ref node() and @ref explicitSnapTarget(), the snap
         * cannot be changed after creation as it could break internal
         * constraints.
         * @see @ref isHandleValid(LayoutHandle) const, @ref hasExplicitSnap()
         */
        Snaps explicitSnap(LayoutHandle handle) const;

        /**
         * @brief Explicit layout snap assuming it belongs to this layouter
         *
         * Like @ref explicitSnap(LayoutHandle) const but without checking that
         * @p handle indeed belongs to this layouter. See its documentation for
         * more information.
         * @see @ref isHandleValid(LayouterDataHandle) const,
         *      @ref layoutHandleData()
         */
        Snaps explicitSnap(LayouterDataHandle handle) const;

        /**
         * @brief Explicit layout snap target
         *
         * Expects that @p handle is valid and has an explicit snap. Returns
         * @ref LayoutHandle::Null if the layout is assigned to a root node and
         * is snapped relative to the whole user interface. Note that the
         * returned handle may be invalid if
         * @ref AbstractUserInterface::removeNode() was called on the target
         * node or its parents and @ref AbstractUserInterface::update() hasn't
         * been called since.
         *
         * Similarly to @ref node() and @ref explicitSnap(), the target cannot be
         * changed after creation as it could break internal constraints.
         * @see @ref isHandleValid(LayoutHandle) const, @ref hasExplicitSnap()
         */
        LayoutHandle explicitSnapTarget(LayoutHandle handle) const;

        /**
         * @brief Explicit layout snap target node assuming it belongs to this layouter
         *
         * Like @ref explicitSnapTarget(LayoutHandle) const but without
         * checking that @p handle indeed belongs to this layouter. See its
         * documentation for more information.
         * @see @ref isHandleValid(LayouterDataHandle) const,
         *      @ref layoutHandleData()
         */
        LayoutHandle explicitSnapTarget(LayouterDataHandle handle) const;

    private:
        MAGNUM_UI_LOCAL LayoutHandle addInternal(NodeHandle node, LayouterDataHandle before, SnapLayoutFlags flags);
        MAGNUM_UI_LOCAL LayoutHandle addInternal(NodeHandle node, Snaps snap, LayouterDataHandle target, SnapLayoutFlags flags);
        MAGNUM_UI_LOCAL void removeInternal(UnsignedInt id);
        MAGNUM_UI_LOCAL SnapLayoutFlags flagsInternal(UnsignedInt id) const;
        MAGNUM_UI_LOCAL void setFlagsInternal(UnsignedInt id, SnapLayoutFlags flags);
        MAGNUM_UI_LOCAL void addFlagsInternal(UnsignedInt id, SnapLayoutFlags flags);
        MAGNUM_UI_LOCAL void clearFlagsInternal(UnsignedInt id, SnapLayoutFlags flags);
        MAGNUM_UI_LOCAL void setChildSnapInternal(UnsignedInt id, Snaps snap);
        MAGNUM_UI_LOCAL LayoutHandle previousInternal(LayouterDataHandle handle) const;
        MAGNUM_UI_LOCAL LayoutHandle nextInternal(LayouterDataHandle handle) const;

        MAGNUM_UI_LOCAL LayouterFeatures doFeatures() const override;
        MAGNUM_UI_LOCAL void doSetSize(const Vector2& size) override;
        MAGNUM_UI_LOCAL void doClean(Containers::BitArrayView layoutIdsToRemove) override;
        MAGNUM_UI_LOCAL void doUpdate(Containers::BitArrayView layoutIdsToUpdate, const Containers::StridedArrayView1D<const UnsignedInt>& topLevelLayoutIds, const Containers::StridedArrayView1D<Vector2>& nodeMinSizes, const Containers::StridedArrayView1D<Vector2>& nodeMaxSizes, const Containers::StridedArrayView1D<Float>& nodeAspectRatios, const Containers::StridedArrayView1D<Vector4>& nodePaddings, const Containers::StridedArrayView1D<Vector4>& nodeMargins, const Containers::StridedArrayView1D<Vector2>& nodeOffsets, const Containers::StridedArrayView1D<Vector2>& nodeSizes) override;

        struct State;
        Containers::Pointer<State> _state;
};

}}

#endif
