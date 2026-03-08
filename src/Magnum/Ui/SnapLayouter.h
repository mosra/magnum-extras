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
the target node, padding of the target node (but not margin of the snapped
node) is taken into account.

Specifying @ref Snap::NoPadX and/or @ref Snap::NoPadY will ignore horizontal
and/or vertical padding and margin.
@see @ref Snaps, @ref SnapLayouter::addExplicit()
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
     * Implicit when snapping to a parent.
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

@see @ref SnapLayouter::addExplicit()
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

@see @ref SnapLayoutFlags, @ref SnapLayouter::add(),
    @ref SnapLayouter::addExplicit()
*/
enum class SnapLayoutFlag: UnsignedByte {
    /**
     * Ignore horizontal overflow of child layouts instead of expanding node
     * width to contain them. Note that explicitly snapped layouts aren't
     * considered and may overflow regardless of this flag being set.
     *
     * Mutually exclusive with @ref SnapLayoutFlag::PropagateMarginX.
     */
    IgnoreOverflowX = 1 << 0,

    /**
     * Ignore horizontal overflow of child layouts instead of expanding node
     * height to contain them. Note that explicitly snapped layouts aren't
     * considered and may overflow regardless of this flag being set.
     *
     * Mutually exclusive with @ref SnapLayoutFlag::PropagateMarginY.
     */
    IgnoreOverflowY = 1 << 1,

    /**
     * Alias to specifying both @ref SnapLayoutFlag::IgnoreOverflowX and
     * @ref SnapLayoutFlag::IgnoreOverflowY. Mutually exclusive with
     * @ref SnapLayoutFlag::PropagateMargin.
     */
    IgnoreOverflow = IgnoreOverflowX|IgnoreOverflowY,

    /**
     * Propagate horizontal margin of child layouts that's larger than
     * horizontal node padding instead of expanding the node width to fit it.
     *
     * Mutually exclusive with @ref SnapLayoutFlag::IgnoreOverflowX.
     */
    PropagateMarginX = 1 << 2,

    /**
     * Propagate vertical margin of child layouts that's larger than vertical
     * node padding instead of expanding the node height to fit it.
     *
     * Mutually exclusive with @ref SnapLayoutFlag::IgnoreOverflowY.
     */
    PropagateMarginY = 1 << 3,

    /**
     * Alias to specifying both @ref SnapLayoutFlag::PropagateMarginX and
     * @ref SnapLayoutFlag::PropagateMarginY. Mutually exclusive with
     * @ref SnapLayoutFlag::IgnoreOverflow.
     */
    PropagateMargin = PropagateMarginX|PropagateMarginY,
};

/**
@debugoperatorenum{SnapLayoutFlag}
@m_since_latest_{extras}
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, SnapLayoutFlag value);

/**
@brief Snap layout flags
@m_since_latest_{extras}

@see @ref SnapLayouter::add(), @ref SnapLayouter::addExplicit()
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

Allows explicitly snapping particular nodes to corners or edges of other nodes
as well as common row and column layouts with size propagation.

@section Ui-SnapLayouter-setup Setting up a snap layouter instance

If you create a @ref UserInterfaceGL with a theme and don't exclude
@ref ThemeFeature::SnapLayouter, an implicit instance is already provided and
available through @ref UserInterface::snapLayouter(). Otherwise, the layouter
doesn't have any shared state or configuration, so it's just about constructing
it from a fresh @ref AbstractUserInterface::createLayouter() handle and passing
it to @ref UserInterface::setSnapLayouterInstance():

@snippet Ui.cpp SnapLayouter-setup-implicit

In comparison, if you want to set up a custom snap layouter that's independent
of the one exposed through @ref UserInterface::snapLayouter(), pass the newly
created instance to @ref AbstractUserInterface::setLayouterInstance() instead:

@snippet Ui.cpp SnapLayouter-setup

Afterwards, with either of the above, assuming
@ref AbstractUserInterface::draw() is called in an appropriate place, the
layouter is ready to use.

@section Ui-SnapLayouter-concepts Core concepts

Internally, the layout calculation works with nodes being *snapped* to target
nodes according to a combination of @ref Snap values. With the target node
shown as the @m_class{m-label m-info} **blue** rectangle below, snapping to its
edges and corners can be achieved by combining @ref Snap::Left,
@relativeref{Snap,Top}, @relativeref{Snap,Right}, @relativeref{Snap,Bottom}
with @ref Snap::InsideX and @relativeref{Snap,InsideY}. For convenience there
are various aliases, such as @ref Snap::TopLeft being a combination of
@relativeref{Snap,Top} and @relativeref{Snap,Left}, or @ref Snap::Inside being
@relativeref{Snap,InsideX} and @relativeref{Snap,InsideY} together.

@htmlinclude ui-snaplayouter.svg

Additionally, it's possible to make the snapped node fill the whole edge using
@ref Snap::FillX and @relativeref{Snap,FillY}. The @ref Snap::FillX is just a
convenience combination of @relativeref{Snap,Left} and @relativeref{Snap,Right},
and @ref Snap::FillY is @relativeref{Snap,Top} and @relativeref{Snap,Bottom}
together. @ref Snap::Fill is then @relativeref{Snap,FillX} and
@relativeref{Snap,FillY} together.

@htmlinclude ui-snaplayouter-fills.svg

The layout by default takes into account margin between nodes and padding
inside nodes, as supplied for example by the @ref LayoutLayer. In the diagram
below, the @m_class{m-label m-info} **blue** rectangle has both a padding,
shown as a @m_class{m-label m-warning} **yellow** outline *outside*, and
margin, shown as a @m_class{m-label m-success} **green** outline *inside*,
other nodes have just paddings. Neighboring padding and margins collapse
together, picking the larger of the two.

@htmlinclude ui-snaplayouter-paddings-margins.svg

In rare cases it may be needed to ignore the padding and margin, which can be
done by specifying @ref Snap::NoPadX, @relativeref{Snap,NoPadY} or their
combination, @relativeref{Snap,NoPad}. This is however considered a workaround
and layouts in general shouldn't need to use these.

Note that in the above diagram the margins and paddings are the same for all
sides for simplicity, however they can be specified differently for each side.
See documentation of @ref LayoutLayer for more examples.

@section Ui-SnapLayouter-add Adding and removing layouts

An *explicitly snapped* layout is added by calling @ref addExplicit() with a
@ref NodeHandle to which the layout is assigned, @ref Snaps describing how to
snap the node and a target @ref LayoutHandle to which to snap. The target
layout has to be either a sibling or a parent layout, and passing
@ref LayoutHandle::Null allows snapping a root node to the whole UI. In the
following snippet, a `popup` node with a concrete size is centered inside the
UI (where empty @ref Snaps mean center), an `accept` child node is placed to
the bottom right corner of it, and a `reject` to the left next to the `accept`.

@snippet ui-snaplayouter.cpp add

Note that @ref Snap::Inside is automatic when snapping a child node to its
parent, which is the case both with the root `popup` node that's a child of the
UI it's being snapped to, and `accept` which is a child of the `popup` it's
being snapped to. You can pass @ref Snap::Inside in those cases but it's
unnecessary. If an offset is specified in addition to node size, such as
@cpp {-10, -10} @ce in case of `accept`, it's added to the final layout offset,
and affects also dependent layouts. When visualizing node placement, for
example with @ref Ui-DebugLayer-node-highlight "Ui::DebugLayer node highlight",
the layout will look like this:

@image html ui-snaplayouter-add.png width=205px

@ref SnapLayouter layouts are *unique*, meaning a particular node can have only
at most one layout from given layouter instance assigned. This in turn means
you don't strictly need to remember a @ref LayoutHandle in order to use it
later, but can retrieve it back from its @ref NodeHandle using
@ref AbstractUserInterface::nodeUniqueLayout():

@snippet Ui.cpp SnapLayouter-query-node-unique-layout

Layouts can be subsequently removed either as a consequence of removing the
node they're assigned to, or by calling @ref remove(). Note that only layouts
that don't have sibling layouts explicitly snapped to them can be removed ---
i.e., while the `reject` layout or its node can be removed, attempting to
remove the `accept` layout is an error because `reject` would then have nowhere
to snap to.

@subsection Ui-SnapLayouter-add-implicit Implicitly snapped layouts

While explicit snapping offers the most flexibility, it may get tedius when
forming long chains of snapped nodes. Insertion or removal of a layout in the
middle of such a chain also isn't possible. An alternative approach in such
cases is specifying an *implicit child snap* using @ref setChildSnap(), and
then calling @ref add() for all child nodes without specifying anything else
for them:

@snippet ui-snaplayouter.cpp implicit-snap

In the above snippet, a `contents` layout is explicitly snapped to the center
of the popup, and inside a `title` and `details` are placed. The child snap for
those is configured so that subsequent children will be added to the bottom of
the previous and fill the parent horizontally. The result is the following:

@image html ui-snaplayouter-implicit-snap.png width=205px

By default new child layouts are appended after all previous, insertion in the
middle can be done by passing a @ref LayoutHandle to the @ref add() function,
saying *before* which existing layout to insert the new one. For example,
inserting a `subtitle` between the `title` and `details`, which would then look
like this:

@snippet ui-snaplayouter.cpp implicit-snap-insert

@image html ui-snaplayouter-implicit-snap-insert.png width=205px

Unlike with explicitly snapped layouts, calling @ref remove() on an implicitly
snapped layout simply causes it to be removed from the sequence, with following
layouts shifting into its place.

Finally, calling the implicit @ref add() on a node that has no parent layout
allows using this node as a target or parent of other layouts but doesn't cause
the node offset to be affected by the layout process in any way. For example,
if we'd want to have the original `popup` node movable by the user and placed
to the center of the UI just initially, not changing its position from the
layout in any way afterwards, such as when the window resizes, we'd do this
instead:

@snippet Ui.cpp SnapLayouter-add-initial-placement

@section Ui-SnapLayouter-size-propagation Size propagation

You might have noticed that the `contentsNode` above didn't have any size
specified. Its size was calculated automatically based on size of its
(implicitly snapped) children, resulting in a size of @cpp {300, 90} @ce. The
same works for explicitly snapped children, however at the moment only the
direct children are counted, nodes snapped to them as neighbors are not. The
resulting size is always a maximum of the hardcoded node size and size of the
contents including also any margin and padding as well as min sizes, coming for
example from the @ref LayoutLayer.

If size propagation is undesirable, for example when the contents should be
clipped instead, pass @ref SnapLayoutFlag::IgnoreOverflowX,
@relativeref{SnapLayoutFlag,IgnoreOverflowY} or the combined
@relativeref{SnapLayoutFlag,IgnoreOverflow}. This will cause the layout to
ignore size of children in given direction, leaving the node at its size given
originally.

@section Ui-SnapLayouter-margin-propagation Margin propagation

With more complex UIs, the layouts commonly start nesting within each other,
such as a column layout having row layouts inside each column:

@snippet ui-snaplayouter.cpp nested-layouts

However, if the actual nodes within then define their own margins, as shown
here by attaching a @ref LayoutLayer data with a margin to each, the margin
will collapse as expected between neighboring nodes, but not across layouts:

@snippet ui-snaplayouter.cpp nested-layouts-margins

@image html ui-snaplayouter-nested-layouts.png width=255px

While that may be desirable in certain cases, such as when either row would be
actually some visual frame around its items, often you may want to have the
items evenly spaced. Applying @ref SnapLayoutFlag::PropagateMargin (or again
the direction-specific @relativeref{SnapLayoutFlag,PropagateMarginX} /
@relativeref{SnapLayoutFlag,PropagateMarginY} variants) to both `first` and
`second` makes the total margin of its children propagated outside --- note how
the respective visualization rectangles shrink to tightly wrap the contents
---, where it gets collapsed between neighboring layouts:

@snippet ui-snaplayouter.cpp nested-layouts-margins-propagate

@image html ui-snaplayouter-nested-layouts-propagated.png width=255px

@section Ui-SnapLayouter-supported-properties Supported layout properties

At the moment, min size, padding and margin layout properties coming from
@ref LayoutLayer and other layers exposing @ref LayerFeature::Layout are taken
into account. The implementation currently doesn't take max size or aspect
ratio into account in any way.

@section Ui-SnapLayouter-snaplayout High-level APIs for widget placement

To be written.
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

        /** @copydoc AbstractLayouter::AbstractLayouter(AbstractLayouter&&) */
        SnapLayouter(SnapLayouter&&) noexcept;

        ~SnapLayouter();

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
         * Use @ref addExplicit() for explicitly snapping to given layout
         * without taking @ref childSnap() into account and without affecting
         * the parent layout size.
         *
         * Delegates to @ref AbstractLayouter::add(), see its documentation for
         * detailed description of all constraints.
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
         * sibling of @p node. If @p snapTarget is a parent, @ref Snap::Inside
         * is implicitly used in addition to what's specified in @p snap.
         *
         * Note that explicitly snapped layouts aren't fully considered when
         * calculating parent layout sizes and paddings and thus may overflow
         * the parent node area in certain cases. In particular, only sizes of
         * explicitly snapped *child* layouts are considered, not explicitly
         * snapped neighbors. Additionally, if
         * @ref SnapLayoutFlag::PropagateMargin is enabled, margins of
         * explicitly snapped layouts are completely ignored and don't
         * propagate anywhere. Use @ref add() for implicit child layouts which
         * propagate their total size and margin to the parent in all cases.
         *
         * Delegates to @ref AbstractLayouter::add(), see its documentation for
         * detailed description of all constraints.
         */
        LayoutHandle addExplicit(NodeHandle node, Snaps snap, LayoutHandle snapTarget, SnapLayoutFlags flags = {});

        /**
         * @brief Add a layout snapping explicitly to given target, assuming the target belongs to this layouter
         *
         * Like @ref addExplicit(NodeHandle, Snaps, LayoutHandle, SnapLayoutFlags)
         * but without checking that @p snapTarget indeed belongs to this
         * layouter. See its documentation for more information.
         */
        LayoutHandle addExplicit(NodeHandle node, Snaps snap, LayouterDataHandle snapTarget, SnapLayoutFlags flags = {});

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
         * documentation for detailed description of all constraints.
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
         * that were passed to @ref add() or @ref addExplicit(), which are by
         * default none.
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
         * was created using @ref addExplicit(), @cpp false @ce otherwise.
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
         * hierarchy, instead they have a snap target exposed by
         * @ref explicitSnapTarget(). If there was no layout assigned to the
         * parent node at the time @p handle was created, or @p handle is
         * assigned to a root node, returns @ref LayoutHandle::Null.
         * Otherwise, the handle is always valid and belonging to this
         * layouter.
         *
         * Similarly to @ref node(), @ref explicitSnapTarget() and
         * @ref explicitSnap(), the snap cannot be changed after creation as it
         * could break internal constraints.
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
         * @brief Next layout assuming it belongs to this layouter
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
         * if @ref explicitSnapTarget() is assigned to a node that's a parent
         * of @p handle node, @ref Snap::Inside is implicitly considered to be
         * included as well, even if not part of the actual value.
         *
         * Similarly to @ref node(), @ref parent() and @ref explicitSnapTarget(),
         * the snap cannot be changed after creation as it could break internal
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
         * Expects that @p handle is valid and has an explicit snap. Layouts
         * with implicit snap are following a parent/child hierarchy and
         * expose a @ref parent(). Returns @ref LayoutHandle::Null if the
         * layout is assigned to a root node and is snapped relative to the
         * whole user interface. Note that the returned handle may be invalid
         * if @ref AbstractUserInterface::removeNode() was called on the target
         * node or its parents and @ref AbstractUserInterface::update() hasn't
         * been called since.
         *
         * Similarly to @ref node(), @ref parent() and @ref explicitSnap(), the
         * target cannot be changed after creation as it could break internal
         * constraints.
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
        MAGNUM_UI_LOCAL LayoutHandle addExplicitInternal(NodeHandle node, Snaps snap, LayouterDataHandle target, SnapLayoutFlags flags);
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
        MAGNUM_UI_LOCAL void doLayout(Containers::BitArrayView layoutIdsToUpdate, const Containers::StridedArrayView1D<const UnsignedInt>& topLevelLayoutIds, const Containers::StridedArrayView1D<Vector2>& nodeMinSizes, const Containers::StridedArrayView1D<Vector2>& nodeMaxSizes, const Containers::StridedArrayView1D<Float>& nodeAspectRatios, const Containers::StridedArrayView1D<Vector4>& nodePaddings, const Containers::StridedArrayView1D<Vector4>& nodeMargins, const Containers::StridedArrayView1D<Vector2>& nodeOffsets, const Containers::StridedArrayView1D<Vector2>& nodeSizes) override;

        struct State;
        Containers::Pointer<State> _state;
};

}}

#endif
