#ifndef Magnum_Ui_SnapLayout_h
#define Magnum_Ui_SnapLayout_h
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
 * @brief Class @ref Magnum::Ui::AbstractSnapLayout, @ref Magnum::Ui::BasicSnapLayout, @ref Magnum::Ui::BasicSnapLayoutColumn, @ref Magnum::Ui::BasicSnapLayoutColumnLeft, @ref Magnum::Ui::BasicSnapLayoutColumnRight, @ref Magnum::Ui::BasicSnapLayoutColumnFill, @ref Magnum::Ui::BasicSnapLayoutRow, @ref Magnum::Ui::BasicSnapLayoutRowTop, @ref Magnum::Ui::BasicSnapLayoutRowBottom, @ref Magnum::Ui::BasicSnapLayoutRowFill, typedef @ref Magnum::Ui::SnapLayout, @ref Magnum::Ui::SnapLayoutColumn, @ref Magnum::Ui::SnapLayoutColumnLeft, @ref Magnum::Ui::SnapLayoutColumnRight, @ref Magnum::Ui::SnapLayoutColumnFill, @ref Magnum::Ui::SnapLayoutRow, @ref Magnum::Ui::SnapLayoutRowTop, @ref Magnum::Ui::SnapLayoutRowBottom, @ref Magnum::Ui::SnapLayoutRowFill
 * @m_since_latest_{extras}
 */

#include <Corrade/Containers/EnumSet.h>
#include <Magnum/Math/Vector2.h>

#include "Magnum/Ui/Ui.h"
#include "Magnum/Ui/visibility.h"

namespace Magnum { namespace Ui {

/**
@brief Base for layouts using @ref SnapLayouter
@m_since_latest_{extras}

A non-owning wrapper of an @ref AbstractUserInterface and @ref SnapLayouter
reference, @ref NodeHandle and a @ref LayoutHandle, convertible to an
@ref AbstractAnchor to construct widget instances with. The
@ref BasicSnapLayout template and the @ref SnapLayout typedef then restrict the
type to a concrete user interface instance.

Converting instances of @ref BasicSnapLayout / @ref SnapLayout to
@ref BasicSnapLayoutColumn, @ref BasicSnapLayoutRow / @ref SnapLayoutColumn,
@ref SnapLayoutRow and other typedefs specialize the layout for concrete
placement rules. Equivalent effect can be achieved by calling
@ref setChildSnap() and @ref setFlags() / @ref addFlags() / @ref clearFlags()
on the unspecialized @ref BasicSnapLayout / @ref SnapLayout.
*/
class MAGNUM_UI_EXPORT AbstractSnapLayout {
    public:
        /**
         * @brief Create a child layout
         * @param layouter      Layouter instance
         * @param parent        Parent anchor
         * @param nodeSize      Child node size
         * @param nodeFlags     Child node flags
         * @param layoutBefore  Child layout to order before or
         *      @ref LayoutHandle::Null if ordered as last
         * @param layoutFlags   Layout flags
         * @return New layout instance
         *
         * Creates a node that's child of @p parent and assigns a layout to it.
         * The child layout is positioned according to
         * @ref SnapLayouter::childSnap() defined by the @p parent layout.
         * Expects that @p layouter is part of the same user interface as
         * @p parent, and that the node in @p parent already has a layout from
         * @p layouter assigned.
         *
         * Use @ref child(SnapLayouter&, Snaps, const AbstractAnchor&, const Vector2&, NodeFlags, SnapLayoutFlags)
         * to snap a child layout explicitly, use
         * @ref sibling(SnapLayouter&, Snaps, const AbstractAnchor&, const Vector2&, NodeFlags, SnapLayoutFlags)
         * to create an explicitly snapped sibling instead of a child and
         * @ref root(AbstractUserInterface&, SnapLayouter&, Snaps, const Vector2&, NodeFlags, SnapLayoutFlags)
         * to create an explicitly snapped root layout. Creating a layout for
         * an already existing node can be done using the
         * @ref AbstractSnapLayout(SnapLayouter&, const AbstractAnchor& anchor)
         * constructor.
         *
         * Calls @ref AbstractUserInterface::createNode() and
         * @ref SnapLayouter::add(NodeHandle, LayoutHandle, SnapLayoutFlags)
         * internally, see their documentation for detailed description of all
         * constraints.
         */
        /* The AbstractAnchor could be by-value but then it'd need the Anchor.h
           include for all inline functions. Same in all cases below. */
        static AbstractSnapLayout child(SnapLayouter& layouter, const AbstractAnchor& parent, const Vector2& nodeSize = {}, NodeFlags nodeFlags = {}, LayoutHandle layoutBefore =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            LayoutHandle::Null,
            #else
            LayoutHandle{}, /* To not have to include Handle.h */
            #endif
            SnapLayoutFlags layoutFlags = {});
        /** @overload */
        static AbstractSnapLayout child(SnapLayouter& layouter, const AbstractAnchor& parent, const Vector2& nodeSize, LayoutHandle layoutBefore, SnapLayoutFlags layoutFlags = {}) {
            return child(layouter, parent, nodeSize, {}, layoutBefore, layoutFlags);
        }
        /** @overload */
        static AbstractSnapLayout child(SnapLayouter& layouter, const AbstractAnchor& parent, const Vector2& nodeSize, NodeFlags nodeFlags, SnapLayoutFlags layoutFlags) {
            return child(layouter, parent, nodeSize, nodeFlags, LayoutHandle{}, layoutFlags);
        }
        /** @overload */
        static AbstractSnapLayout child(SnapLayouter& layouter, const AbstractAnchor& parent, const Vector2& nodeSize, SnapLayoutFlags layoutFlags) {
            return child(layouter, parent, nodeSize, NodeFlags{}, layoutFlags);
        }

        /**
         * @brief Create a child layout, ordered before given layout assuming the layout belongs to the same layouter
         *
         * Like @ref child(SnapLayouter&, const AbstractAnchor&, const Vector2&, NodeFlags, LayoutHandle, SnapLayoutFlags)
         * but without checking that @p layoutBefore indeed belongs to
         * @p layouter. See its documentation for more information.
         *
         * Calls @ref AbstractUserInterface::createNode() and
         * @ref SnapLayouter::add(NodeHandle, LayouterDataHandle, SnapLayoutFlags)
         * internally, see their documentation for detailed description of all
         * constraints.
         */
        static AbstractSnapLayout child(SnapLayouter& layouter, const AbstractAnchor& parent, const Vector2& nodeSize, NodeFlags nodeFlags, LayouterDataHandle layoutBefore, SnapLayoutFlags layoutFlags = {});
        /** @overload */
        static AbstractSnapLayout child(SnapLayouter& layouter, const AbstractAnchor& parent, const Vector2& nodeSize, LayouterDataHandle layoutBefore, SnapLayoutFlags layoutFlags = {}) {
            return child(layouter, parent, nodeSize, NodeFlags{}, layoutBefore, layoutFlags);
        }

        /**
         * @brief Create an explicitly snapped root layout
         * @param ui               User interface instance
         * @param layouter         Layouter instance
         * @param snap             How to snap to the @p ui
         * @param nodeSize         Child node size
         * @param nodeFlags        Child node flags
         * @param layoutFlags      Layout flags
         * @return New layout instance
         *
         * Creates a root node and assigns it a layout explicitly snapped to
         * the user interface itself. Expects that @p layouter is part of
         * @p ui.
         *
         * Use @ref child(SnapLayouter&, Snaps, const AbstractAnchor&, const Vector2&, NodeFlags, SnapLayoutFlags)
         * to create an explicitly snapped child and
         * @ref sibling(SnapLayouter&, Snaps, const AbstractAnchor&, const Vector2&, NodeFlags, SnapLayoutFlags)
         * to create an explicitly snapped sibling node, use
         * @ref child(SnapLayouter&, const AbstractAnchor&, const Vector2&, NodeFlags, LayoutHandle, SnapLayoutFlags)
         * to add a child layout positioned according to
         * @ref SnapLayouter::childSnap() defined by a parent layout.
         *
         * Calls @ref AbstractUserInterface::createNode() and
         * @ref SnapLayouter::add(NodeHandle, Snaps, LayoutHandle, SnapLayoutFlags)
         * internally, see their documentation for detailed description of all
         * constraints.
         */
        static AbstractSnapLayout root(AbstractUserInterface& ui, SnapLayouter& layouter, Snaps snap, const Vector2& nodeSize = {}, NodeFlags nodeFlags = {}, SnapLayoutFlags layoutFlags = {});
        /** @overload */
        static AbstractSnapLayout root(AbstractUserInterface& ui, SnapLayouter& layouter, Snaps snap, const Vector2& nodeSize, SnapLayoutFlags layoutFlags) {
            return root(ui, layouter, snap, nodeSize, {}, layoutFlags);
        }

        /**
         * @brief Create an explicitly snapped child layout
         * @param layouter         Layouter instance
         * @param snap             How to snap to the @p parent
         * @param parent           Parent anchor to snap to
         * @param nodeSize         Child node size
         * @param nodeFlags        Child node flags
         * @param layoutFlags      Layout flags
         * @return New layout instance
         *
         * Creates a node that's child of @p parent and assigns a layout
         * explicitly snapped to it. Expects that @p layouter is part of the
         * same user interface as @p parent, and that the node in @p parent
         * already has a layout from @p layouter assigned.
         *
         * Use @ref sibling(SnapLayouter&, Snaps, const AbstractAnchor&, const Vector2&, NodeFlags, SnapLayoutFlags)
         * to create an explicitly snapped sibling instead of a child and
         * @ref root(AbstractUserInterface&, SnapLayouter&, Snaps, const Vector2&, NodeFlags, SnapLayoutFlags)
         * to create an explicitly snapped root node, use
         * @ref child(SnapLayouter&, const AbstractAnchor&, const Vector2&, NodeFlags, LayoutHandle, SnapLayoutFlags)
         * to add a child layout positioned according to
         * @ref SnapLayouter::childSnap() defined by a parent layout.
         *
         * Calls @ref AbstractUserInterface::createNode() and
         * @ref SnapLayouter::add(NodeHandle, Snaps, LayoutHandle, SnapLayoutFlags)
         * internally, see their documentation for detailed description of all
         * constraints.
         */
        static AbstractSnapLayout child(SnapLayouter& layouter, Snaps snap, const AbstractAnchor& parent, const Vector2& nodeSize = {}, NodeFlags nodeFlags = {}, SnapLayoutFlags layoutFlags = {});
        /** @overload */
        static AbstractSnapLayout child(SnapLayouter& layouter, Snaps snap, const AbstractAnchor& parent, const Vector2& nodeSize, SnapLayoutFlags layoutFlags) {
            return child(layouter, snap, parent, nodeSize, {}, layoutFlags);
        }

        /**
         * @brief Create an explicitly snapped sibling layout
         * @param layouter         Layouter instance
         * @param snap             How to snap to the @p target
         * @param target           Target anchor to snap to
         * @param nodeSize         Node size
         * @param nodeFlags        Node flags
         * @param layoutFlags      Layout flags
         * @return New layout instance
         *
         * Creates a node with the same parent as @p target, and assigns it a
         * layout that's explicitly snapped to @p target. Expects that
         * @p layouter is part of the same user interface as @p target, and
         * that the node in @p target already has a layout from @p layouter
         * assigned.
         *
         * Use @ref child(SnapLayouter&, Snaps, const AbstractAnchor&, const Vector2&, NodeFlags, SnapLayoutFlags)
         * and specializations to create an explicitly snapped child instead of
         * a sibling and @ref root(AbstractUserInterface&, SnapLayouter&, Snaps, const Vector2&, NodeFlags, SnapLayoutFlags)
         * to create an explicitly snapped root node, use
         * @ref child(SnapLayouter&, const AbstractAnchor&, const Vector2&, NodeFlags, LayoutHandle, SnapLayoutFlags)
         * to add a child layout positioned according to
         * @ref SnapLayouter::childSnap() defined by a parent layout.
         *
         * Calls @ref AbstractUserInterface::createNode() and
         * @ref SnapLayouter::add(NodeHandle, Snaps, LayoutHandle, SnapLayoutFlags)
         * internally, see their documentation for detailed description of all
         * constraints.
         */
        static AbstractSnapLayout sibling(SnapLayouter& layouter, Snaps snap, const AbstractAnchor& target, const Vector2& nodeSize = {}, NodeFlags nodeFlags = {}, SnapLayoutFlags layoutFlags = {});
        /** @overload */
        static AbstractSnapLayout sibling(SnapLayouter& layouter, Snaps snap, const AbstractAnchor& target, const Vector2& nodeSize, SnapLayoutFlags layoutFlags) {
            return sibling(layouter, snap, target, nodeSize, {}, layoutFlags);
        }

        /**
         * @brief Construct with creating or reusing layout on an existing node
         * @param layouter      Layouter instance
         * @param anchor        Anchor to which to assign the layout
         *
         * Expects that @p layouter is part of the same user interface as
         * @p anchor. If the node in @p anchor already has a layout from
         * @p layouter assigned, uses it, otherwise adds a new one to it. Use
         * the @ref AbstractSnapLayout(SnapLayouter&, const AbstractAnchor& anchor, LayoutHandle, SnapLayoutFlags)
         * overload to add a new layout always together with specifying its
         * properties.
         *
         * Calls @ref SnapLayouter::add(NodeHandle, LayoutHandle, SnapLayoutFlags)
         * internally, see its documentation for detailed description of all
         * constraints.
         */
        explicit AbstractSnapLayout(SnapLayouter& layouter, const AbstractAnchor& anchor);

        /**
         * @brief Construct with creating or reusing layout on an existing node
         * @param ui            User interface instance
         * @param layouter      Layouter instance
         * @param node          Node to which to assign the layout
         *
         * Expects that @p layouter is part of @p ui and @p node is valid. If
         * the @p node already has a layout from @p layouter assigned, uses it,
         * otherwise adds a new one to it. Use
         * the @ref AbstractSnapLayout(SnapLayouter&, const AbstractAnchor& anchor, LayoutHandle, SnapLayoutFlags)
         * overload to add a new layout always together with specifying its
         * properties.
         *
         * Calls @ref SnapLayouter::add(NodeHandle, LayoutHandle, SnapLayoutFlags)
         * internally, see its documentation for detailed description of all
         * constraints.
         */
        explicit AbstractSnapLayout(AbstractUserInterface& ui, SnapLayouter& layouter, NodeHandle node);

        /**
         * @brief Construct a layout on an existing node
         * @param layouter      Layouter instance
         * @param anchor        Anchor to which to assign the layout
         * @param before        Sibling layout to order before or
         *      @ref LayoutHandle::Null if ordered as last
         * @param flags         Layout flags
         *
         * Assigns a new layout to an existing node contained in @p anchor.
         * Expects that @p layouter is part of the same user interface as
         * @p anchor, and that the node in @p anchor doesn't have a layout from
         * @p layouter assigned yet. Use the @ref AbstractSnapLayout(SnapLayouter&, const AbstractAnchor&)
         * overload to add a new layout only if it isn't assigned yet.
         *
         * Use the @ref child(), @ref root()  or @ref sibling() static
         * functions for a combined operation of creating a node and assigning
         * a layout to it, you can also call member @ref child() and
         * @ref sibling() on an existing layout instance for the same effect.
         *
         * Calls @ref SnapLayouter::add(NodeHandle, LayoutHandle, SnapLayoutFlags)
         * internally, see its documentation for detailed description of all
         * constraints.
         */
        explicit AbstractSnapLayout(SnapLayouter& layouter, const AbstractAnchor& anchor, LayoutHandle before, SnapLayoutFlags flags = {});
        /** @overload */
        explicit AbstractSnapLayout(SnapLayouter& layouter, const AbstractAnchor& anchor, SnapLayoutFlags layoutFlags): AbstractSnapLayout{layouter, anchor, LayoutHandle{}, layoutFlags} {}

        /**
         * @brief Construct a layout on an existing node, ordered before given layout assuming the layout belongs to the same layouter
         *
         * Like @ref AbstractSnapLayout(SnapLayouter&, const AbstractAnchor&, LayoutHandle, SnapLayoutFlags)
         * but without checking that @p layoutBefore indeed belongs to
         * @p layouter. See documentation for more information.
         *
         * Calls @ref SnapLayouter::add(NodeHandle, LayouterDataHandle, SnapLayoutFlags)
         * internally, see its documentation for detailed description of all
         * constraints.
         */
        explicit AbstractSnapLayout(SnapLayouter& layouter, const AbstractAnchor& anchor, LayouterDataHandle layoutBefore, SnapLayoutFlags layoutFlags = {});

        /**
         * @brief Construct a layout on an existing node
         * @param ui            User interface instance
         * @param layouter      Layouter instance
         * @param node          Node to which to assign the layout
         * @param before        Sibling layout to order before or
         *      @ref LayoutHandle::Null if ordered as last
         * @param flags         Layout flags
         *
         * Assigns a new layout to an existing @p node. Expects that
         * @p layouter is part of @p ui, @p node is valid, and that @p node
         * doesn't have a layout from @p layouter assigned yet. Use the
         * @ref AbstractSnapLayout(AbstractUserInterface&, SnapLayouter&, NodeHandle)
         * overload to add a new layout only if it isn't assigned yet.
         *
         * Use the @ref child(), @ref root()  or @ref sibling() static
         * functions for a combined operation of creating a node and assigning
         * a layout to it, you can also call member @ref child() and
         * @ref sibling() on an existing layout instance for the same effect.
         *
         * Calls @ref SnapLayouter::add(NodeHandle, LayoutHandle, SnapLayoutFlags)
         * internally, see its documentation for detailed description of all
         * constraints.
         */
        explicit AbstractSnapLayout(AbstractUserInterface& ui, SnapLayouter& layouter, NodeHandle node, LayoutHandle before, SnapLayoutFlags flags = {});
        /** @overload */
        explicit AbstractSnapLayout(AbstractUserInterface& ui, SnapLayouter& layouter, NodeHandle node, SnapLayoutFlags layoutFlags): AbstractSnapLayout{ui, layouter, node, LayoutHandle{}, layoutFlags} {}

        /**
         * @brief Construct a layout on an existing node, ordered before given layout assuming the layout belongs to the same layouter
         *
         * Like @ref AbstractSnapLayout(AbstractUserInterface&, SnapLayouter&, NodeHandle, LayoutHandle, SnapLayoutFlags)
         * but without checking that @p layoutBefore indeed belongs to
         * @p layouter. See documentation for more information.
         *
         * Calls @ref SnapLayouter::add(NodeHandle, LayouterDataHandle, SnapLayoutFlags)
         * internally, see its documentation for detailed description of all
         * constraints.
         */
        explicit AbstractSnapLayout(AbstractUserInterface& ui, SnapLayouter& layouter, NodeHandle node, LayouterDataHandle layoutBefore, SnapLayoutFlags layoutFlags = {});

        /**
         * @brief Construct an explicitly snapped layout on an existing node
         * @param layouter      Layouter instance
         * @param anchor        Anchor to which to assign the layout
         * @param snap          How to snap
         * @param snapTarget    Target to snap to or
         *      @ref LayoutHandle::Null to snap a root @p anchor to the UI
         *      itself
         * @param flags         Layout flags
         *
         * Calls @ref SnapLayouter::add(NodeHandle, Snaps, LayoutHandle, SnapLayoutFlags)
         * internally, see its documentation for detailed description of all
         * constraints.
         */
        explicit AbstractSnapLayout(SnapLayouter& layouter, const AbstractAnchor& anchor, Snaps snap, LayoutHandle snapTarget, SnapLayoutFlags flags = {});

        /**
         * @brief Construct an explicitly snapped layout on an existing node, assuming the target belongs to the same layouter
         *
         * Like @ref AbstractSnapLayout(SnapLayouter&, const AbstractAnchor&, Snaps, LayoutHandle, SnapLayoutFlags)
         * but without checking that @p snapTarget indeed belongs to
         * @p layouter. See documentation for more information.
         *
         * Calls @ref SnapLayouter::add(NodeHandle, Snaps, LayouterDataHandle, SnapLayoutFlags)
         * internally, see its documentation for detailed description of all
         * constraints.
         */
        explicit AbstractSnapLayout(SnapLayouter& layouter, const AbstractAnchor& anchor, Snaps snap, LayouterDataHandle snapTarget, SnapLayoutFlags flags = {});

        /**
         * @brief Construct an explicitly snapped layout on an existing node
         * @param ui            User interface instance
         * @param layouter      Layouter instance
         * @param node          Node to which to assign the layout
         * @param snap          How to snap
         * @param snapTarget    Target to snap to or
         *      @ref LayoutHandle::Null to snap a root @p anchor to the UI
         *      itself
         * @param flags         Layout flags
         *
         * Calls @ref SnapLayouter::add(NodeHandle, Snaps, LayoutHandle, SnapLayoutFlags)
         * internally, see its documentation for detailed description of all
         * constraints.
         */
        explicit AbstractSnapLayout(AbstractUserInterface& ui, SnapLayouter& layouter, NodeHandle node, Snaps snap, LayoutHandle snapTarget, SnapLayoutFlags flags = {});

        /**
         * @brief Construct an explicitly snapped layout on an existing node, assuming the target belongs to the same layouter
         *
         * Like @ref AbstractSnapLayout(AbstractUserInterface& ui, SnapLayouter&, NodeHandle, Snaps, LayoutHandle, SnapLayoutFlags)
         * but without checking that @p snapTarget indeed belongs to
         * @p layouter. See documentation for more information.
         *
         * Calls @ref SnapLayouter::add(NodeHandle, Snaps, LayouterDataHandle, SnapLayoutFlags)
         * internally, see its documentation for detailed description of all
         * constraints.
         */
        explicit AbstractSnapLayout(AbstractUserInterface& ui, SnapLayouter& layouter, NodeHandle node, Snaps snap, LayouterDataHandle snapTarget, SnapLayoutFlags flags = {});

        /** @brief User interface instance */
        AbstractUserInterface& ui() const { return *_ui; }

        /** @brief Layouter instance */
        SnapLayouter& layouter() const { return *_layouter; }

        /**
         * @brief Node handle
         *
         * Guaranteed to be never @ref NodeHandle::Null.
         */
        NodeHandle node() const { return _node; }
        /*implicit*/ operator NodeHandle() const { return _node; } /**< @overload */

        /**
         * @brief Layout handle
         *
         * Guaranteed to be never @ref LayoutHandle::Null.
         */
        LayoutHandle layout() const;
        /*implicit*/ operator LayoutHandle() const { return layout(); } /**< @overload */

        /** @brief Widget positioning anchor */
        /*implicit*/ operator AbstractAnchor() const;

        /** @brief Layout flags */
        SnapLayoutFlags flags() const;

        /**
         * @brief Set layout flags
         * @return Reference to self (for method chaining)
         *
         * Initially, a layout has the flags that were passed to the
         * constructor, to @ref root() @ref child() or @ref sibling(), which
         * are by default none. The @ref BasicSnapLayoutColumn,
         * @ref BasicSnapLayoutColumnLeft, @ref BasicSnapLayoutColumnRight,
         * @ref BasicSnapLayoutColumnFill, @ref BasicSnapLayoutRow,
         * @ref BasicSnapLayoutRowTop, @ref BasicSnapLayoutRowBottom and
         * @ref BasicSnapLayoutRowFill subclasses implicitly add
         * @ref SnapLayoutFlag::PropagateMarginX and
         * @relativeref{SnapLayoutFlag,PropagateMarginY} unless
         * @relativeref{SnapLayoutFlag,IgnoreOverflowX} and
         * @relativeref{SnapLayoutFlag,IgnoreOverflowY} is already specified in
         * given direction.
         *
         * Calls @ref SnapLayouter::setFlags() internally, see its
         * documentation for more information.
         */
        AbstractSnapLayout& setFlags(SnapLayoutFlags flags);

        /**
         * @brief Add layout flags
         *
         * Calls @ref setFlags() with the existing flags ORed with @p flags.
         * Useful for preserving previously set flags.
         * @see @ref clearFlags()
         */
        AbstractSnapLayout& addFlags(SnapLayoutFlags flags);

        /**
         * @brief Clear layout flags
         *
         * Calls @ref setFlags() with the existing flags ANDed with the inverse
         * of @p flags. Useful for removing a subset of previously set flags.
         * @see @ref addFlags()
         */
        AbstractSnapLayout& clearFlags(SnapLayoutFlags flags);

        /** @brief Snap for child layouts */
        Snaps childSnap() const;

        /**
         * @brief Set snap for child layouts
         * @return Reference to self (for method chaining)
         *
         * Default is @ref Snap::Bottom unless overriden by a subclass:
         *
         * -    @relativeref{Snap,Bottom} for @ref BasicSnapLayoutColumn
         * -    @relativeref{Snap,BottomLeft}|@relativeref{Snap,InsideX} for
         *      @ref BasicSnapLayoutColumnLeft
         * -    @relativeref{Snap,BottomRight}|@relativeref{Snap,InsideX} for
         *      @ref BasicSnapLayoutColumnRight
         * -    @relativeref{Snap,Bottom}|@relativeref{Snap,FillX} for
         *      @ref BasicSnapLayoutColumnFill
         * -    @relativeref{Snap,Right} for @ref BasicSnapLayoutRow
         * -    @relativeref{Snap,TopRight}|@relativeref{Snap,InsideY} for
         *      @ref BasicSnapLayoutRowTop
         * -    @relativeref{Snap,BottomRight}|@relativeref{Snap,InsideY} for
         *      @ref BasicSnapLayoutRowBottom
         * -    @relativeref{Snap,Right}|@relativeref{Snap,FillY} for
         *      @ref BasicSnapLayoutRowFill
         *
         * Calls @ref SnapLayouter::setChildSnap() internally, see its
         * documentation for detailed description of all constraints.
         */
        AbstractSnapLayout& setChildSnap(Snaps snap);

        /**
         * @brief Create a child layout
         * @param nodeSize      Child node size
         * @param nodeFlags     Child node flags
         * @param layoutBefore  Child layout to order before or
         *      @ref LayoutHandle::Null if ordered as last
         * @param layoutFlags   Layout flags
         * @return New layout instance
         *
         * Creates a node that's a child of @ref node() and assigns a layout to
         * it. The child layout is positioned according to @ref childSnap().
         * Use @ref child(Snaps, const Vector2&, NodeFlags, SnapLayoutFlags) to
         * snap a child layout explicitly, use
         * @ref sibling(Snaps, const Vector2&, NodeFlags, SnapLayoutFlags) to
         * create an explicitly snapped sibling instead of a child.
         *
         * The @ref BasicSnapLayout subclass and its various typedefs such as
         * @ref SnapLayout return a concrete layout instance. You can also use
         * the static @ref child(SnapLayouter&, const AbstractAnchor&, const Vector2&, NodeFlags, LayoutHandle, SnapLayoutFlags)
         * function to create a child layout without an existing
         * @ref AbstractSnapLayout instance.
         *
         * Calls @ref AbstractUserInterface::createNode() and
         * @ref SnapLayouter::add(NodeHandle, LayoutHandle, SnapLayoutFlags)
         * internally, see their documentation for detailed description of all
         * constraints.
         */
        AbstractSnapLayout child(const Vector2& nodeSize = {}, NodeFlags nodeFlags = {}, LayoutHandle layoutBefore =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            LayoutHandle::Null,
            #else
            LayoutHandle{}, /* To not have to include Handle.h */
            #endif
            SnapLayoutFlags layoutFlags = {});
        /** @overload */
        AbstractSnapLayout child(const Vector2& nodeSize, LayoutHandle layoutBefore, SnapLayoutFlags layoutFlags = {}) {
            return child(nodeSize, {}, layoutBefore, layoutFlags);
        }
        /** @overload */
        AbstractSnapLayout child(const Vector2& nodeSize, NodeFlags nodeFlags, SnapLayoutFlags layoutFlags) {
            return child(nodeSize, nodeFlags, LayoutHandle{}, layoutFlags);
        }
        /** @overload */
        AbstractSnapLayout child(const Vector2& nodeSize, SnapLayoutFlags layoutFlags) {
            return child(nodeSize, LayoutHandle{}, layoutFlags);
        }

        /**
         * @brief Create a child layout, ordered before given layout assuming the layout belongs to the same layouter
         *
         * Like @ref child(const Vector2&, NodeFlags, LayoutHandle, SnapLayoutFlags)
         * but without checking that @p layoutBefore indeed belongs to
         * @ref layouter(). See its documentation for more information.
         *
         * Calls @ref AbstractUserInterface::createNode() and
         * @ref SnapLayouter::add(NodeHandle, LayouterDataHandle, SnapLayoutFlags)
         * internally, see their documentation for detailed description of all
         * constraints.
         */
        AbstractSnapLayout child(const Vector2& nodeSize, NodeFlags nodeFlags, LayouterDataHandle layoutBefore, SnapLayoutFlags layoutFlags = {});
        /** @overload */
        AbstractSnapLayout child(const Vector2& nodeSize, LayouterDataHandle layoutBefore, SnapLayoutFlags layoutFlags = {}) {
            return child(nodeSize, {}, layoutBefore, layoutFlags);
        }

        /**
         * @brief Create an explicitly snapped child layout
         * @param snap          How to snap
         * @param nodeSize      Child node size
         * @param nodeFlags     Child node flags
         * @param layoutFlags   Layout flags
         * @return New layout instance
         *
         * Creates a node that's a child of @ref node(), and assigns a layout
         * explicitly snapped to it. Use
         * @ref sibling(Snaps, const Vector2&, NodeFlags, SnapLayoutFlags) to
         * create an explicitly snapped sibling instead of a child, use
         * @ref child(const Vector2&, NodeFlags, LayoutHandle, SnapLayoutFlags)
         * to add a child layout positioned implicitly according to
         * @ref childSnap().
         *
         * The @ref BasicSnapLayout subclass and its various typedefs such as
         * @ref SnapLayout return a concrete layout instance. You can also use
         * the static @ref child(SnapLayouter&, Snaps, const AbstractAnchor&, const Vector2&, NodeFlags, SnapLayoutFlags)
         * function to create an explicitly snapped child layout without an
         * existing @ref AbstractSnapLayout instance.
         *
         * Calls @ref AbstractUserInterface::createNode() and
         * @ref SnapLayouter::add(NodeHandle, Snaps, LayoutHandle, SnapLayoutFlags)
         * internally, see their documentation for detailed description of all
         * constraints.
         */
        AbstractSnapLayout child(Snaps snap, const Vector2& nodeSize = {}, NodeFlags nodeFlags = {}, SnapLayoutFlags layoutFlags = {});
        /** @overload */
        AbstractSnapLayout child(Snaps snap, const Vector2& nodeSize, SnapLayoutFlags layoutFlags) {
            return child(snap, nodeSize, {}, layoutFlags);
        }

        /**
         * @brief Create an explicitly snapped sibling layout
         * @param snap          How to snap
         * @param nodeSize      Child node size
         * @param nodeFlags     Child node flags
         * @param layoutFlags   Layout flags
         * @return New layout instance
         *
         * Creates a node with the same parent as @ref node(), and assigns a
         * layout explicitly snapped to it. Use
         * @ref child(Snaps, const Vector2&, NodeFlags, SnapLayoutFlags) to
         * create an explicitly snapped child instead of a sibling, use
         * @ref child(const Vector2&, NodeFlags, LayoutHandle, SnapLayoutFlags)
         * to add a child layout positioned implicitly according to
         * @ref childSnap().
         *
         * The @ref BasicSnapLayout subclass and its various typedefs such as
         * @ref SnapLayout return a concrete layout instance. You can also use
         * the static @ref sibling(SnapLayouter&, Snaps, const AbstractAnchor&, const Vector2&, NodeFlags, SnapLayoutFlags)
         * function to create an explicitly snapped sibling layout without an
         * existing @ref AbstractSnapLayout instance.
         *
         * Calls @ref AbstractUserInterface::createNode() and
         * @ref SnapLayouter::add(NodeHandle, Snaps, LayoutHandle, SnapLayoutFlags)
         * internally, see their documentation for detailed description of all
         * constraints.
         */
        AbstractSnapLayout sibling(Snaps snap, const Vector2& nodeSize = {}, NodeFlags nodeFlags = {}, SnapLayoutFlags layoutFlags = {});
        /** @overload */
        AbstractSnapLayout sibling(Snaps snap, const Vector2& nodeSize, SnapLayoutFlags layoutFlags) {
            return sibling(snap, nodeSize, {}, layoutFlags);
        }

    #ifdef DOXYGEN_GENERATING_OUTPUT
    private:
    #else
    protected:
    #endif
        /* Delegated to by all constructors, including BasicSnapLayout
           constructors. Populates just the _ui and _node members, _layouter
           and _layout is left uninitialized. */
        explicit AbstractSnapLayout(NoInitT, AbstractUserInterface& ui, NodeHandle node);
        /* Takes layouter as a pointer and not a reference so it can be used
           also in assertions for UserInterface::snapLayouter() not existing.
           Used also by BasicSnapLayout assertions so it's protected. */
        explicit AbstractSnapLayout(AbstractUserInterface& ui, SnapLayouter* layouter, NodeHandle node, LayouterDataHandle layout): _ui{&ui}, _layouter{layouter}, _node{node}, _layout{layout} {}

        /* Common code used by all constructor variants as well as
           BasicSnapLayout constructors, as regular functions so various
           asserts can happen before initializing the members */
        void addLayout(SnapLayouter& layouter);
        void addLayout(SnapLayouter& layouter, LayoutHandle before, SnapLayoutFlags flags);
        void addLayout(SnapLayouter& layouter, LayouterDataHandle before, SnapLayoutFlags flags);
        void addLayout(SnapLayouter& layouter, Snaps snap, LayoutHandle snapTarget, SnapLayoutFlags flags);
        void addLayout(SnapLayouter& layouter, Snaps snap, LayouterDataHandle snapTarget, SnapLayoutFlags flags);

        /* Deinlined specializations for BasicSnapLayoutColumn and such in
           SnapLayout.cpp */
        template<template<class> class> void setDefaultChildSnapFor();
        /* Called from specialized BasicSnapLayoutColumn etc. constructors */
        void setDefaultPropagateMargin();

    private:
        AbstractUserInterface* _ui;
        SnapLayouter* _layouter;
        NodeHandle _node;
        LayouterDataHandle _layout;
};

/**
@brief Layout using @ref SnapLayouter
@m_since_latest_{extras}

Restricts @ref AbstractSnapLayout to a concrete user interface instance. Meant
to be used through concrete a typedef such as @ref SnapLayout. See the base
class documentation for more information.
@see @ref BasicSnapLayoutColumn, @ref BasicSnapLayoutColumnLeft,
    @ref BasicSnapLayoutColumnRight, @ref BasicSnapLayoutColumnFill,
    @ref BasicSnapLayoutRow, @ref BasicSnapLayoutRowTop,
    @ref BasicSnapLayoutRowBottom, @ref BasicSnapLayoutRowFill,
    @ref SnapLayoutColumn, @ref SnapLayoutColumnLeft,
    @ref SnapLayoutColumnRight, @ref SnapLayoutRow, @ref SnapLayoutRowTop,
    @ref SnapLayoutRowBottom, @ref SnapLayoutRowFill
*/
template<class UserInterface> class BasicSnapLayout: public AbstractSnapLayout {
    public:
        /**
         * @brief @copybrief AbstractSnapLayout::child(SnapLayouter&, const AbstractAnchor&, const Vector2&, NodeFlags, LayoutHandle, SnapLayoutFlags)
         *
         * Like @ref AbstractSnapLayout::child(SnapLayouter&, const AbstractAnchor&, const Vector2&, NodeFlags, LayoutHandle, SnapLayoutFlags)
         * but returning a concrete layout instance. Use
         * @ref child(const BasicAnchor<UserInterface>&, const Vector2&, NodeFlags, LayoutHandle, SnapLayoutFlags)
         * to use the default @ref SnapLayouter instance available through
         * @ref UserInterface::snapLayouter(). If you have an exising layout
         * instance, there's @ref child(const Vector2&, NodeFlags, LayoutHandle, SnapLayoutFlags)
         * which is equivalent to this function.
         */
        /* The BasicAnchor could be by-value but then it'd need the Anchor.h
           include for all inline functions. Same in all cases below. */
        static BasicSnapLayout<UserInterface> child(SnapLayouter& layouter, const BasicAnchor<UserInterface>& parent, const Vector2& nodeSize = {}, NodeFlags nodeFlags = {}, LayoutHandle layoutBefore =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            LayoutHandle::Null,
            #else
            LayoutHandle{}, /* To not have to include Handle.h */
            #endif
            SnapLayoutFlags layoutFlags = {});
        /** @overload */
        static BasicSnapLayout<UserInterface> child(SnapLayouter& layouter, const BasicAnchor<UserInterface>& parent, const Vector2& nodeSize, LayoutHandle layoutBefore, SnapLayoutFlags layoutFlags = {}) {
            return child(layouter, parent, nodeSize, {}, layoutBefore, layoutFlags);
        }
        /** @overload */
        static BasicSnapLayout<UserInterface> child(SnapLayouter& layouter, const BasicAnchor<UserInterface>& parent, const Vector2& nodeSize, NodeFlags nodeFlags, SnapLayoutFlags layoutFlags) {
            return child(layouter, parent, nodeSize, nodeFlags, LayoutHandle{}, layoutFlags);
        }
        /** @overload */
        static BasicSnapLayout<UserInterface> child(SnapLayouter& layouter, const BasicAnchor<UserInterface>& parent, const Vector2& nodeSize, SnapLayoutFlags layoutFlags) {
            return child(layouter, parent, nodeSize, NodeFlags{}, layoutFlags);
        }

        /**
         * @brief @copybrief AbstractSnapLayout::child(SnapLayouter&, const AbstractAnchor&, const Vector2&, NodeFlags, LayouterDataHandle, SnapLayoutFlags)
         *
         * Like @ref AbstractSnapLayout::child(SnapLayouter&, const AbstractAnchor&, const Vector2&, NodeFlags, LayouterDataHandle, SnapLayoutFlags)
         * but returning a concrete layout instance. If you have an exising
         * layout instance, there's @ref child(const Vector2&, NodeFlags, LayouterDataHandle, SnapLayoutFlags)
         * which is equivalent to this function.
         */
        static BasicSnapLayout<UserInterface> child(SnapLayouter& layouter, const BasicAnchor<UserInterface>& parent, const Vector2& nodeSize, NodeFlags nodeFlags, LayouterDataHandle layoutBefore, SnapLayoutFlags layoutFlags = {});
        /** @overload */
        static BasicSnapLayout<UserInterface> child(SnapLayouter& layouter, const BasicAnchor<UserInterface>& parent, const Vector2& nodeSize, LayouterDataHandle layoutBefore, SnapLayoutFlags layoutFlags = {}) {
            return child(layouter, parent, nodeSize, NodeFlags{}, layoutBefore, layoutFlags);
        }

        /**
         * @brief Create a child layout using the default layouter in given user interface
         *
         * Like @ref AbstractSnapLayout::child(SnapLayouter&, const AbstractAnchor&, const Vector2&, NodeFlags, LayoutHandle, SnapLayoutFlags)
         * but using the default @ref SnapLayouter instance available through
         * @ref UserInterface::snapLayouter(), and returning a concrete layout
         * instance. Expects that the user interface referenced in @p parent
         * contains a @ref SnapLayouter instance.
         *
         * If you have an exising layout instance, there's
         * @ref child(const Vector2&, NodeFlags, LayoutHandle, SnapLayoutFlags)
         * which is equivalent to this function.
         * @see @ref UserInterface::hasSnapLayouter()
         */
        static BasicSnapLayout<UserInterface> child(const BasicAnchor<UserInterface>& parent, const Vector2& nodeSize = {}, NodeFlags nodeFlags = {}, LayoutHandle layoutBefore =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            LayoutHandle::Null,
            #else
            LayoutHandle{}, /* To not have to include Handle.h */
            #endif
            SnapLayoutFlags layoutFlags = {});
        /** @overload */
        static BasicSnapLayout<UserInterface> child(const BasicAnchor<UserInterface>& parent, const Vector2& nodeSize, LayoutHandle layoutBefore, SnapLayoutFlags layoutFlags = {}) {
            return child(parent, nodeSize, {}, layoutBefore, layoutFlags);
        }
        /** @overload */
        static BasicSnapLayout<UserInterface> child(const BasicAnchor<UserInterface>& parent, const Vector2& nodeSize, NodeFlags nodeFlags, SnapLayoutFlags layoutFlags) {
            return child(parent, nodeSize, nodeFlags, LayoutHandle{}, layoutFlags);
        }
        /** @overload */
        static BasicSnapLayout<UserInterface> child(const BasicAnchor<UserInterface>& parent, const Vector2& nodeSize, SnapLayoutFlags layoutFlags) {
            return child(parent, nodeSize, NodeFlags{}, layoutFlags);
        }

        /**
         * @brief Create a child layout using the default layouter in given user interface, ordered before given layout assuming the layout belongs to the same layouter
         *
         * Like @ref AbstractSnapLayout::child(SnapLayouter&, const AbstractAnchor&, const Vector2&, NodeFlags, LayouterDataHandle, SnapLayoutFlags)
         * but using the default @ref SnapLayouter instance available through
         * @ref UserInterface::snapLayouter(), and returning a concrete layout
         * instance. Expects that the user interface referenced in @p parent
         * contains a @ref SnapLayouter instance.
         *
         * If you have an exising layout instance, there's
         * @ref child(const Vector2&, NodeFlags, LayouterDataHandle, SnapLayoutFlags)
         * which is equivalent to this function.
         * @see @ref UserInterface::hasSnapLayouter()
         */
        static BasicSnapLayout<UserInterface> child(const BasicAnchor<UserInterface>& parent, const Vector2& nodeSize, NodeFlags nodeFlags, LayouterDataHandle layoutBefore, SnapLayoutFlags layoutFlags = {});
        /** @overload */
        static BasicSnapLayout<UserInterface> child(const BasicAnchor<UserInterface>& parent, const Vector2& nodeSize, LayouterDataHandle layoutBefore, SnapLayoutFlags layoutFlags = {}) {
            return child(parent, nodeSize, {}, layoutBefore, layoutFlags);
        }

        /**
         * @brief @copybrief AbstractSnapLayout::root(AbstractUserInterface&, SnapLayouter&, Snaps, const Vector2&, NodeFlags, SnapLayoutFlags)
         *
         * Like @ref AbstractSnapLayout::root(AbstractUserInterface&, SnapLayouter&, Snaps, const Vector2&, NodeFlags, SnapLayoutFlags)
         * but returning a concrete layout instance. Use @ref root(UserInterface&, Snaps, const Vector2&, NodeFlags, SnapLayoutFlags)
         * to use the default @ref SnapLayouter instance available through
         * @ref UserInterface::snapLayouter().
         */
        static BasicSnapLayout<UserInterface> root(UserInterface& ui, SnapLayouter& layouter, Snaps snap, const Vector2& nodeSize = {}, NodeFlags nodeFlags = {}, SnapLayoutFlags layoutFlags = {}) {
            return BasicSnapLayout<UserInterface>{AbstractSnapLayout::root(ui, layouter, snap, nodeSize, nodeFlags, layoutFlags)};
        }
        /** @overload */
        static BasicSnapLayout<UserInterface> root(UserInterface& ui, SnapLayouter& layouter, Snaps snap, const Vector2& nodeSize, SnapLayoutFlags layoutFlags) {
            return root(ui, layouter, snap, nodeSize, {}, layoutFlags);
        }

        /**
         * @brief Create an explicitly snapped root layout using the default layouter in given user interface
         *
         * Like @ref AbstractSnapLayout::root(AbstractUserInterface&, SnapLayouter&, Snaps, const Vector2&, NodeFlags, SnapLayoutFlags)
         * but using the default @ref SnapLayouter instance available through
         * @ref UserInterface::snapLayouter(), and returning a concrete layout
         * instance. Expects that @p ui contains a @ref SnapLayouter instance.
         * @see @ref UserInterface::hasSnapLayouter()
         */
        static BasicSnapLayout<UserInterface> root(UserInterface& ui, Snaps snap, const Vector2& nodeSize = {}, NodeFlags nodeFlags = {}, SnapLayoutFlags layoutFlags = {});
        /** @overload */
        static BasicSnapLayout<UserInterface> root(UserInterface& ui, Snaps snap, const Vector2& nodeSize, SnapLayoutFlags layoutFlags) {
            return root(ui, snap, nodeSize, {}, layoutFlags);
        }

        /**
         * @brief @copybrief AbstractSnapLayout::child(SnapLayouter&, Snaps, const AbstractAnchor&, const Vector2&, NodeFlags, SnapLayoutFlags)
         *
         * Like @ref AbstractSnapLayout::child(SnapLayouter&, Snaps, const AbstractAnchor&, const Vector2&, NodeFlags, SnapLayoutFlags)
         * but returning a concrete layout instance. Use
         * @ref child(Snaps, const BasicAnchor<UserInterface>&, const Vector2&, NodeFlags, SnapLayoutFlags)
         * to use the default @ref SnapLayouter instance available through
         * @ref UserInterface::snapLayouter(). If you have an existing layout
         * instance, there's @ref child(Snaps, const Vector2&, NodeFlags, SnapLayoutFlags)
         * which is equivalent to this function.
         */
        static BasicSnapLayout<UserInterface> child(SnapLayouter& layouter, Snaps snap, const BasicAnchor<UserInterface>& parent, const Vector2& nodeSize = {}, NodeFlags nodeFlags = {}, SnapLayoutFlags layoutFlags = {});
        /** @overload */
        static BasicSnapLayout<UserInterface> child(SnapLayouter& layouter, Snaps snap, const BasicAnchor<UserInterface>& parent, const Vector2& nodeSize, SnapLayoutFlags layoutFlags) {
            return child(layouter, snap, parent, nodeSize, {}, layoutFlags);
        }

        /**
         * @brief Create an explicitly snapped child layout using the default layouter in given user interface
         *
         * Like @ref AbstractSnapLayout::child(SnapLayouter&, Snaps, const AbstractAnchor&, const Vector2&, NodeFlags, SnapLayoutFlags)
         * but using the default @ref SnapLayouter instance available through
         * @ref UserInterface::snapLayouter(), and returning a concrete layout
         * instance. Expects that the user interface referenced in @p parent
         * contains a @ref SnapLayouter instance.
         *
         * If you have an existing layout instance, there's
         * @ref child(Snaps, const Vector2&, NodeFlags, SnapLayoutFlags)
         * which is equivalent to this function.
         * @see @ref UserInterface::hasSnapLayouter()
         */
        static BasicSnapLayout<UserInterface> child(Snaps snap, const BasicAnchor<UserInterface>& parent, const Vector2& nodeSize = {}, NodeFlags nodeFlags = {}, SnapLayoutFlags layoutFlags = {});
        /** @overload */
        static BasicSnapLayout<UserInterface> child(Snaps snap, const BasicAnchor<UserInterface>& parent, const Vector2& nodeSize, SnapLayoutFlags layoutFlags) {
            return child(snap, parent, nodeSize, {}, layoutFlags);
        }

        /**
         * @brief @copybrief AbstractSnapLayout::sibling(SnapLayouter&, Snaps, const AbstractAnchor&, const Vector2&, NodeFlags, SnapLayoutFlags)
         *
         * Like @ref AbstractSnapLayout::sibling(SnapLayouter&, Snaps, const AbstractAnchor&, const Vector2&, NodeFlags, SnapLayoutFlags)
         * but returning a concrete layout instance. Use @ref sibling(Snaps, const BasicAnchor<UserInterface>&, const Vector2&, NodeFlags, SnapLayoutFlags)
         * to use the default @ref SnapLayouter instance available through
         * @ref UserInterface::snapLayouter(). If you have an existing layout
         * instance, there's @ref sibling(Snaps, const Vector2&, NodeFlags, SnapLayoutFlags)
         * which is equivalent to this function.
         */
        static BasicSnapLayout<UserInterface> sibling(SnapLayouter& layouter, Snaps snap, const BasicAnchor<UserInterface>& target, const Vector2& nodeSize = {}, NodeFlags nodeFlags = {}, SnapLayoutFlags layoutFlags = {});
        /** @overload */
        static BasicSnapLayout<UserInterface> sibling(SnapLayouter& layouter, Snaps snap, const BasicAnchor<UserInterface>& target, const Vector2& nodeSize, SnapLayoutFlags layoutFlags) {
            return sibling(layouter, snap, target, nodeSize, {}, layoutFlags);
        }

        /**
         * @brief Create an explicitly snapped sibling layout using the default layouter in given user interface
         *
         * Like @ref AbstractSnapLayout::sibling(SnapLayouter&, Snaps, const AbstractAnchor&, const Vector2&, NodeFlags, SnapLayoutFlags)
         * but using the default @ref SnapLayouter instance available through
         * @ref UserInterface::snapLayouter(), and returning a concrete layout
         * instance. Expects that the user interface referenced in @p target
         * contains a @ref SnapLayouter instance.
         * @see @ref UserInterface::hasSnapLayouter()
         */
        static BasicSnapLayout<UserInterface> sibling(Snaps snap, const BasicAnchor<UserInterface>& target, const Vector2& nodeSize = {}, NodeFlags nodeFlags = {}, SnapLayoutFlags layoutFlags = {});
        /** @overload */
        static BasicSnapLayout<UserInterface> sibling(Snaps snap, const BasicAnchor<UserInterface>& target, const Vector2& nodeSize, SnapLayoutFlags layoutFlags) {
            return sibling(snap, target, nodeSize, {}, layoutFlags);
        }

        /**
         * @brief @copybrief AbstractSnapLayout::AbstractSnapLayout(SnapLayouter&, const AbstractAnchor&)
         *
         * Like @ref AbstractSnapLayout::AbstractSnapLayout(SnapLayouter&, const AbstractAnchor&)
         * but accepting a concrete anchor instance.
         */
        explicit BasicSnapLayout(SnapLayouter& layouter, const BasicAnchor<UserInterface>& anchor): AbstractSnapLayout{layouter, anchor} {}

        /**
         * @brief @copybrief AbstractSnapLayout::AbstractSnapLayout(AbstractUserInterface&, SnapLayouter&, NodeHandle)
         *
         * Like @ref AbstractSnapLayout::AbstractSnapLayout(AbstractUserInterface&, SnapLayouter&, NodeHandle)
         * but accepting a concrete user interface instance.
         */
        explicit BasicSnapLayout(UserInterface& ui, SnapLayouter& layouter, NodeHandle node): AbstractSnapLayout{ui, layouter, node} {}

        /**
         * @brief @copybrief AbstractSnapLayout::AbstractSnapLayout(SnapLayouter&, const AbstractAnchor&)
         *
         * Like @ref AbstractSnapLayout::AbstractSnapLayout(SnapLayouter&, const AbstractAnchor&)
         * but using the default @ref SnapLayouter instance available through
         * @ref UserInterface::snapLayouter() and accepting a concrete anchor
         * instance. Expects that the user interface referenced in @p anchor
         * contains a @ref SnapLayouter instance.
         * @see @ref UserInterface::hasSnapLayouter()
         */
        explicit BasicSnapLayout(const BasicAnchor<UserInterface>& anchor);

        /**
         * @brief @copybrief AbstractSnapLayout::AbstractSnapLayout(AbstractUserInterface&, SnapLayouter&, NodeHandle)
         *
         * Like @ref AbstractSnapLayout::AbstractSnapLayout(AbstractUserInterface&, SnapLayouter&, NodeHandle)
         * but accepting a concrete user interface instance and using the
         * default @ref SnapLayouter available through
         * @ref UserInterface::snapLayouter(). Expects that the @p ui contains
         * a @ref SnapLayouter instance.
         * @see @ref UserInterface::hasSnapLayouter()
         */
        explicit BasicSnapLayout(UserInterface& ui, NodeHandle node);

        /**
         * @brief @copybrief AbstractSnapLayout::AbstractSnapLayout(SnapLayouter&, const AbstractAnchor&, LayoutHandle, SnapLayoutFlags)
         *
         * Like @ref AbstractSnapLayout::AbstractSnapLayout(SnapLayouter&, const AbstractAnchor&, LayoutHandle, SnapLayoutFlags)
         * but accepting a concrete anchor instance.
         */
        explicit BasicSnapLayout(SnapLayouter& layouter, const BasicAnchor<UserInterface>& anchor, LayoutHandle before, SnapLayoutFlags flags = {}): AbstractSnapLayout{layouter, anchor, before, flags} {}
        /** @overload */
        explicit BasicSnapLayout(SnapLayouter& layouter, const BasicAnchor<UserInterface>& anchor, SnapLayoutFlags layoutFlags): BasicSnapLayout{layouter, anchor, LayoutHandle{}, layoutFlags} {}

        /**
         * @brief @copybrief AbstractSnapLayout::AbstractSnapLayout(SnapLayouter&, const AbstractAnchor&, LayouterDataHandle, SnapLayoutFlags)
         *
         * Like @ref AbstractSnapLayout::AbstractSnapLayout(SnapLayouter&, const AbstractAnchor&, LayouterDataHandle, SnapLayoutFlags)
         * but accepting a concrete anchor instance.
         */
        explicit BasicSnapLayout(SnapLayouter& layouter, const BasicAnchor<UserInterface>& anchor, LayouterDataHandle before, SnapLayoutFlags flags = {}): AbstractSnapLayout{layouter, anchor, before, flags} {}

        /**
         * @brief @copybrief AbstractSnapLayout::AbstractSnapLayout(AbstractUserInterface&, SnapLayouter&, NodeHandle, LayoutHandle, SnapLayoutFlags)
         *
         * Like @ref AbstractSnapLayout::AbstractSnapLayout(AbstractUserInterface&, SnapLayouter&, NodeHandle, LayoutHandle, SnapLayoutFlags)
         * but accepting a concrete user interface instance.
         */
        explicit BasicSnapLayout(UserInterface& ui, SnapLayouter& layouter, NodeHandle node, LayoutHandle before, SnapLayoutFlags flags = {}): AbstractSnapLayout{ui, layouter, node, before, flags} {}
        /** @overload */
        explicit BasicSnapLayout(UserInterface& ui, SnapLayouter& layouter, NodeHandle node, SnapLayoutFlags layoutFlags): BasicSnapLayout{ui, layouter, node, LayoutHandle{}, layoutFlags} {}

        /**
         * @brief @copybrief AbstractSnapLayout::AbstractSnapLayout(AbstractUserInterface&, SnapLayouter&, NodeHandle, LayouterDataHandle, SnapLayoutFlags)
         *
         * Like @ref AbstractSnapLayout::AbstractSnapLayout(AbstractUserInterface&, SnapLayouter&, NodeHandle, LayouterDataHandle, SnapLayoutFlags)
         * but accepting a concrete user interface instance.
         */
        explicit BasicSnapLayout(UserInterface& ui, SnapLayouter& layouter, NodeHandle node, LayouterDataHandle before, SnapLayoutFlags flags = {}): AbstractSnapLayout{ui, layouter, node, before, flags} {}

        /**
         * @brief @copybrief AbstractSnapLayout::AbstractSnapLayout(SnapLayouter&, const AbstractAnchor&, LayoutHandle, SnapLayoutFlags)
         *
         * Like @ref AbstractSnapLayout::AbstractSnapLayout(SnapLayouter&, const AbstractAnchor&, LayoutHandle, SnapLayoutFlags)
         * but using the default @ref SnapLayouter instance available through
         * @ref UserInterface::snapLayouter() and accepting a concrete anchor
         * instance. Expects that the user interface referenced in @p anchor
         * contains a @ref SnapLayouter instance.
         * @see @ref UserInterface::hasSnapLayouter()
         */
        explicit BasicSnapLayout(const BasicAnchor<UserInterface>& anchor, LayoutHandle before, SnapLayoutFlags flags = {});
        /** @overload */
        explicit BasicSnapLayout(const BasicAnchor<UserInterface>& anchor, SnapLayoutFlags layoutFlags): BasicSnapLayout{anchor, LayoutHandle{}, layoutFlags} {}

        /**
         * @brief @copybrief AbstractSnapLayout::AbstractSnapLayout(SnapLayouter&, const AbstractAnchor&, LayouterDataHandle, SnapLayoutFlags)
         *
         * Like @ref AbstractSnapLayout::AbstractSnapLayout(SnapLayouter&, const AbstractAnchor&, LayouterDataHandle, SnapLayoutFlags)
         * but using the default @ref SnapLayouter instance available through
         * @ref UserInterface::snapLayouter() and accepting a concrete anchor
         * instance. Expects that the user interface referenced in @p anchor
         * contains a @ref SnapLayouter instance.
         * @see @ref UserInterface::hasSnapLayouter()
         */
        explicit BasicSnapLayout(const BasicAnchor<UserInterface>& anchor, LayouterDataHandle before, SnapLayoutFlags flags = {});

        /**
         * @brief @copybrief AbstractSnapLayout::AbstractSnapLayout(AbstractUserInterface&, SnapLayouter&, NodeHandle, LayoutHandle, SnapLayoutFlags)
         *
         * Like @ref AbstractSnapLayout::AbstractSnapLayout(AbstractUserInterface&, SnapLayouter&, NodeHandle, LayoutHandle, SnapLayoutFlags)
         * but accepting a concrete user interface instance and using the
         * default @ref SnapLayouter available through
         * @ref UserInterface::snapLayouter(). Expects that the @p ui contains
         * a @ref SnapLayouter instance.
         * @see @ref UserInterface::hasSnapLayouter()
         */
        explicit BasicSnapLayout(UserInterface& ui, NodeHandle node, LayoutHandle before, SnapLayoutFlags flags = {});
        /** @overload */
        explicit BasicSnapLayout(UserInterface& ui, NodeHandle node, SnapLayoutFlags layoutFlags): BasicSnapLayout{ui, node, LayoutHandle{}, layoutFlags} {}

        /**
         * @brief @copybrief AbstractSnapLayout::AbstractSnapLayout(AbstractUserInterface&, SnapLayouter&, NodeHandle, LayouterDataHandle, SnapLayoutFlags)
         *
         * Like @ref AbstractSnapLayout::AbstractSnapLayout(AbstractUserInterface&, SnapLayouter&, NodeHandle, LayouterDataHandle, SnapLayoutFlags)
         * but accepting a concrete user interface instance and using the
         * default @ref SnapLayouter available through
         * @ref UserInterface::snapLayouter(). Expects that the @p ui contains
         * a @ref SnapLayouter instance.
         * @see @ref UserInterface::hasSnapLayouter()
         */
        explicit BasicSnapLayout(UserInterface& ui, NodeHandle node, LayouterDataHandle before, SnapLayoutFlags flags = {});

        /**
         * @brief @copybrief AbstractSnapLayout::AbstractSnapLayout(SnapLayouter&, const AbstractAnchor&, Snaps, LayoutHandle, SnapLayoutFlags)
         *
         * Like @ref AbstractSnapLayout::AbstractSnapLayout(SnapLayouter&, const AbstractAnchor&, Snaps, LayoutHandle, SnapLayoutFlags)
         * but accepting a concrete anchor instance.
         */
        explicit BasicSnapLayout(SnapLayouter& layouter, const BasicAnchor<UserInterface>& anchor, Snaps snap, LayoutHandle snapTarget, SnapLayoutFlags flags = {}): AbstractSnapLayout{layouter, anchor, snap, snapTarget, flags} {}

        /**
         * @brief @copybrief AbstractSnapLayout::AbstractSnapLayout(SnapLayouter&, const AbstractAnchor&, Snaps, LayouterDataHandle, SnapLayoutFlags)
         *
         * Like @ref AbstractSnapLayout::AbstractSnapLayout(SnapLayouter&, const AbstractAnchor&, Snaps, LayouterDataHandle, SnapLayoutFlags)
         * but accepting a concrete anchor instance.
         */
        explicit BasicSnapLayout(SnapLayouter& layouter, const BasicAnchor<UserInterface>& anchor, Snaps snap, LayouterDataHandle snapTarget, SnapLayoutFlags flags = {}): AbstractSnapLayout{layouter, anchor, snap, snapTarget, flags} {}

        /**
         * @brief @copybrief AbstractSnapLayout::AbstractSnapLayout(AbstractUserInterface&, SnapLayouter&, NodeHandle, Snaps, LayoutHandle, SnapLayoutFlags)
         *
         * Like @ref AbstractSnapLayout::AbstractSnapLayout(AbstractUserInterface&, SnapLayouter&, NodeHandle, Snaps, LayoutHandle, SnapLayoutFlags)
         * but accepting a concrete user interface instance.
         */
        explicit BasicSnapLayout(UserInterface& ui, SnapLayouter& layouter, NodeHandle node, Snaps snap, LayoutHandle snapTarget, SnapLayoutFlags flags = {}): AbstractSnapLayout{ui, layouter, node, snap, snapTarget, flags} {}

        /**
         * @brief @copybrief AbstractSnapLayout::AbstractSnapLayout(AbstractUserInterface&, SnapLayouter&, NodeHandle, Snaps, LayouterDataHandle, SnapLayoutFlags)
         *
         * Like @ref AbstractSnapLayout::AbstractSnapLayout(AbstractUserInterface&, SnapLayouter&, NodeHandle, Snaps, LayouterDataHandle, SnapLayoutFlags)
         * but accepting a concrete user interface instance.
         */
        explicit BasicSnapLayout(UserInterface& ui, SnapLayouter& layouter, NodeHandle node, Snaps snap, LayouterDataHandle snapTarget, SnapLayoutFlags flags = {}): AbstractSnapLayout{ui, layouter, node, snap, snapTarget, flags} {}

        /**
         * @brief @copybrief AbstractSnapLayout::AbstractSnapLayout(SnapLayouter&, const AbstractAnchor&, Snaps, LayoutHandle, SnapLayoutFlags)
         *
         * Like @ref AbstractSnapLayout::AbstractSnapLayout(SnapLayouter&, const AbstractAnchor&, Snaps, LayoutHandle, SnapLayoutFlags)
         * but using the default @ref SnapLayouter instance available through
         * @ref UserInterface::snapLayouter() and accepting a concrete anchor
         * instance. Expects that the user interface referenced in @p anchor
         * contains a @ref SnapLayouter instance.
         * @see @ref UserInterface::hasSnapLayouter()
         */
        explicit BasicSnapLayout(const BasicAnchor<UserInterface>& anchor, Snaps snap, LayoutHandle snapTarget, SnapLayoutFlags flags = {});

        /**
         * @brief @copybrief AbstractSnapLayout::AbstractSnapLayout(SnapLayouter&, const AbstractAnchor&, Snaps, LayouterDataHandle, SnapLayoutFlags)
         *
         * Like @ref AbstractSnapLayout::AbstractSnapLayout(SnapLayouter&, const AbstractAnchor&, Snaps, LayouterDataHandle, SnapLayoutFlags)
         * but using the default @ref SnapLayouter instance available through
         * @ref UserInterface::snapLayouter() and accepting a concrete anchor
         * instance. Expects that the user interface referenced in @p anchor
         * contains a @ref SnapLayouter instance.
         * @see @ref UserInterface::hasSnapLayouter()
         */
        explicit BasicSnapLayout(const BasicAnchor<UserInterface>& anchor, Snaps snap, LayouterDataHandle snapTarget, SnapLayoutFlags flags = {});

        /**
         * @brief @copybrief AbstractSnapLayout::AbstractSnapLayout(AbstractUserInterface&, SnapLayouter&, NodeHandle, Snaps, LayoutHandle, SnapLayoutFlags)
         *
         * Like @ref AbstractSnapLayout::AbstractSnapLayout(AbstractUserInterface&, SnapLayouter&, NodeHandle, Snaps, LayoutHandle, SnapLayoutFlags)
         * but accepting a concrete user interface instance and using the
         * default @ref SnapLayouter available through
         * @ref UserInterface::snapLayouter(). Expects that the @p ui contains
         * a @ref SnapLayouter instance.
         * @see @ref UserInterface::hasSnapLayouter()
         */
        explicit BasicSnapLayout(UserInterface& ui, NodeHandle node, Snaps snap, LayoutHandle snapTarget, SnapLayoutFlags flags = {});

        /**
         * @brief @copybrief AbstractSnapLayout::AbstractSnapLayout(AbstractUserInterface&, SnapLayouter&, NodeHandle, Snaps, LayouterDataHandle, SnapLayoutFlags)
         *
         * Like @ref AbstractSnapLayout::AbstractSnapLayout(AbstractUserInterface&, SnapLayouter&, NodeHandle, Snaps, LayouterDataHandle, SnapLayoutFlags)
         * but accepting a concrete user interface instance and using the
         * default @ref SnapLayouter available through
         * @ref UserInterface::snapLayouter(). Expects that the @p ui contains
         * a @ref SnapLayouter instance.
         * @see @ref UserInterface::hasSnapLayouter()
         */
        explicit BasicSnapLayout(UserInterface& ui, NodeHandle node, Snaps snap, LayouterDataHandle snapTarget, SnapLayoutFlags flags = {});

        /** @brief User interface instance */
        UserInterface& ui() const;

        /** @brief Widget positioning anchor */
        /*implicit*/ operator BasicAnchor<UserInterface>() const;

        /**
         * @brief @copybrief AbstractSnapLayout::child(const Vector2&, NodeFlags, LayoutHandle, SnapLayoutFlags)
         *
         * Like @ref AbstractSnapLayout::child(const Vector2&, NodeFlags, LayoutHandle, SnapLayoutFlags)
         * but returning a concrete layout instance.
         */
        BasicSnapLayout<UserInterface> child(const Vector2& nodeSize = {}, NodeFlags nodeFlags = {}, LayoutHandle layoutBefore =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            LayoutHandle::Null,
            #else
            LayoutHandle{}, /* To not have to include Handle.h */
            #endif
            SnapLayoutFlags layoutFlags = {})
        {
            return BasicSnapLayout<UserInterface>{AbstractSnapLayout::child(nodeSize, nodeFlags, layoutBefore, layoutFlags)};
        }
        /** @overload */
        BasicSnapLayout<UserInterface> child(const Vector2& nodeSize, LayoutHandle layoutBefore, SnapLayoutFlags layoutFlags = {}) {
            return child(nodeSize, NodeFlags{}, layoutBefore, layoutFlags);
        }
        /** @overload */
        BasicSnapLayout<UserInterface> child(const Vector2& nodeSize, NodeFlags nodeFlags, SnapLayoutFlags layoutFlags) {
            return child(nodeSize, nodeFlags, LayoutHandle{}, layoutFlags);
        }
        /** @overload */
        BasicSnapLayout<UserInterface> child(const Vector2& nodeSize, SnapLayoutFlags layoutFlags) {
            return child(nodeSize, NodeFlags{}, layoutFlags);
        }

        /**
         * @brief @copybrief AbstractSnapLayout::child(const Vector2&, NodeFlags, LayouterDataHandle, SnapLayoutFlags)
         *
         * Like @ref AbstractSnapLayout::child(const Vector2&, NodeFlags, LayouterDataHandle, SnapLayoutFlags)
         * but returning a concrete layout instance.
         */
        BasicSnapLayout<UserInterface> child(const Vector2& nodeSize, NodeFlags nodeFlags, LayouterDataHandle layoutBefore, SnapLayoutFlags layoutFlags = {}) {
            return BasicSnapLayout<UserInterface>{AbstractSnapLayout::child(nodeSize, nodeFlags, layoutBefore, layoutFlags)};
        }
        /** @overload */
        BasicSnapLayout<UserInterface> child(const Vector2& nodeSize, LayouterDataHandle layoutBefore, SnapLayoutFlags layoutFlags = {}) {
            return child(nodeSize, NodeFlags{}, layoutBefore, layoutFlags);
        }

        /**
         * @brief @copybrief AbstractSnapLayout::child(Snaps, const Vector2&, NodeFlags, SnapLayoutFlags)
         *
         * Like @ref AbstractSnapLayout::child(Snaps, const Vector2&, NodeFlags, SnapLayoutFlags)
         * but returning a concrete layout instance.
         */
        BasicSnapLayout<UserInterface> child(Snaps snap, const Vector2& nodeSize = {}, NodeFlags nodeFlags = {}, SnapLayoutFlags layoutFlags = {}) {
            return BasicSnapLayout<UserInterface>{AbstractSnapLayout::child(snap, nodeSize, nodeFlags, layoutFlags)};
        }
        /** @overload */
        BasicSnapLayout<UserInterface> child(Snaps snap, const Vector2& nodeSize, SnapLayoutFlags layoutFlags) {
            return child(snap, nodeSize, NodeFlags{}, layoutFlags);
        }

        /**
         * @brief @copybrief AbstractSnapLayout::sibling(Snaps, const Vector2&, NodeFlags, SnapLayoutFlags)
         *
         * Like @ref AbstractSnapLayout::sibling(Snaps, const Vector2&, NodeFlags, SnapLayoutFlags)
         * but returning a concrete layout instance.
         */
        BasicSnapLayout<UserInterface> sibling(Snaps snap, const Vector2& nodeSize = {}, NodeFlags nodeFlags = {}, SnapLayoutFlags layoutFlags = {}) {
            return BasicSnapLayout<UserInterface>{AbstractSnapLayout::sibling(snap, nodeSize, nodeFlags, layoutFlags)};
        }
        /** @overload */
        BasicSnapLayout<UserInterface> sibling(Snaps snap, const Vector2& nodeSize, SnapLayoutFlags layoutFlags) {
            return sibling(snap, nodeSize, NodeFlags{}, layoutFlags);
        }

        /* Overloads to remove a WTF factor from method chaining order */
        #ifndef DOXYGEN_GENERATING_OUTPUT
        BasicSnapLayout<UserInterface>& setChildSnap(Snaps snap) {
            return static_cast<BasicSnapLayout<UserInterface>&>(AbstractSnapLayout::setChildSnap(snap));
        }
        BasicSnapLayout<UserInterface>& setFlags(SnapLayoutFlags flags) {
            return static_cast<BasicSnapLayout<UserInterface>&>(AbstractSnapLayout::setFlags(flags));
        }
        BasicSnapLayout<UserInterface>& addFlags(SnapLayoutFlags flags) {
            return static_cast<BasicSnapLayout<UserInterface>&>(AbstractSnapLayout::addFlags(flags));
        }
        BasicSnapLayout<UserInterface>& clearFlags(SnapLayoutFlags flags) {
            return static_cast<BasicSnapLayout<UserInterface>&>(AbstractSnapLayout::clearFlags(flags));
        }
        #endif

    #ifdef DOXYGEN_GENERATING_OUTPUT
    private:
    #else
    protected:
    #endif
        /* Used by subclasses to call AbstractSnapLayout::setChildSnapFor(),
           which has deinlined specializations in SnapLayout.cpp, during
           construction */
        template<template<class> class T> explicit BasicSnapLayout(const BasicSnapLayout<UserInterface>& other, T<UserInterface>&): AbstractSnapLayout{other} {
            setDefaultChildSnapFor<T>();
            setDefaultPropagateMargin();
        }
        template<template<class> class T> explicit BasicSnapLayout(const BasicAnchor<UserInterface>& anchor, T<UserInterface>&): BasicSnapLayout{anchor} {
            setDefaultChildSnapFor<T>();
        }

    private:
        /* Used by child(), sibling(), root() member & static functions */
        explicit BasicSnapLayout(const AbstractSnapLayout& layout): AbstractSnapLayout{layout} {}
        /* Used by child(), sibling(), root() assertions */
        #ifndef CORRADE_NO_ASSERT
        explicit BasicSnapLayout(UserInterface& ui, SnapLayouter* layouter, NodeHandle node, LayouterDataHandle layout): AbstractSnapLayout{ui, layouter, node, layout} {}
        #endif
};

/**
@brief Column layout using @ref SnapLayouter
@m_since_latest_{extras}

Places children in a column from top to bottom, aligned to the center
horizontally. Meant to be used through concrete a typedef such as
@ref SnapLayoutColumn.
@see @ref BasicSnapLayoutColumnLeft, @ref BasicSnapLayoutColumnRight,
    @ref BasicSnapLayoutColumnFill, @ref BasicSnapLayoutRow
*/
template<class UserInterface> class BasicSnapLayoutColumn: public BasicSnapLayout<UserInterface> {
    public:
        /**
         * @brief Construct from an anchor
         *
         * Calls @ref setChildSnap() with @ref Snap::Bottom.
         */
        /*implicit*/ BasicSnapLayoutColumn(const BasicAnchor<UserInterface>& anchor): BasicSnapLayout<UserInterface>{anchor, *this} {}

        /**
         * @brief Construct from an unspecialized layout
         *
         * Calls @ref setChildSnap() with @ref Snap::Bottom and @ref addFlags()
         * with @ref SnapLayoutFlag::PropagateMarginX and
         * @relativeref{SnapLayoutFlag,PropagateMarginY} unless
         * @relativeref{SnapLayoutFlag,IgnoreOverflowX} and
         * @relativeref{SnapLayoutFlag,IgnoreOverflowY} is already specified in
         * given direction, allowing margins from nested widgets to be
         * collapsed with parent's neigbors as well.
         */
        /*implicit*/ BasicSnapLayoutColumn(const BasicSnapLayout<UserInterface>& layout): BasicSnapLayout<UserInterface>{layout, *this} {}

        /** @brief Conversion from another specialized layout is not allowed */
        template<class T, typename std::enable_if<std::is_base_of<BasicSnapLayout<UserInterface>, T>::value, int>::type = 0> BasicSnapLayoutColumn(const T&) = delete;
};

/**
@brief Left-aligned column layout using @ref SnapLayouter
@m_since_latest_{extras}

Places children in a column from top to bottom, aligned to the left. Meant to
be used through concrete a typedef such as @ref SnapLayoutColumnLeft.
@see @ref BasicSnapLayoutColumn, @ref BasicSnapLayoutColumnRight,
    @ref BasicSnapLayoutColumnFill, @ref BasicSnapLayoutRowTop
*/
template<class UserInterface> class BasicSnapLayoutColumnLeft: public BasicSnapLayout<UserInterface> {
    public:
        /**
         * @brief Construct from an anchor
         *
         * Calls @ref setChildSnap() with
         * @link Snap::BottomLeft @endlink|@ref Snap::InsideX.
         */
        /*implicit*/ BasicSnapLayoutColumnLeft(const BasicAnchor<UserInterface>& anchor): BasicSnapLayout<UserInterface>{anchor, *this} {}

        /**
         * @brief Construct from an unspecialized layout
         *
         * Calls @ref setChildSnap() with
         * @link Snap::BottomLeft @endlink|@ref Snap::InsideX and
         * @ref addFlags() with @ref SnapLayoutFlag::PropagateMarginX and
         * @relativeref{SnapLayoutFlag,PropagateMarginY} unless
         * @relativeref{SnapLayoutFlag,IgnoreOverflowX} and
         * @relativeref{SnapLayoutFlag,IgnoreOverflowY} is already specified in
         * given direction, allowing margins from nested widgets to be
         * collapsed with parent's neigbors as well.
         */
        /*implicit*/ BasicSnapLayoutColumnLeft(const BasicSnapLayout<UserInterface>& layout): BasicSnapLayout<UserInterface>{layout, *this} {}

        /** @brief Conversion from another specialized layout is not allowed */
        template<class T, typename std::enable_if<std::is_base_of<BasicSnapLayout<UserInterface>, T>::value, int>::type = 0> BasicSnapLayoutColumnLeft(const T&) = delete;
};

/**
@brief Right-aligned column layout using @ref SnapLayouter
@m_since_latest_{extras}

Places children in a column from top to bottom, aligned to the right. Meant to
be used through concrete a typedef such as @ref SnapLayoutColumnRight.
@see @ref BasicSnapLayoutColumn, @ref BasicSnapLayoutColumnLeft,
    @ref BasicSnapLayoutColumnFill, @ref BasicSnapLayoutRowBottom
*/
template<class UserInterface> class BasicSnapLayoutColumnRight: public BasicSnapLayout<UserInterface> {
    public:
        /**
         * @brief Construct from an anchor
         *
         * Calls @ref setChildSnap() with
         * @link Snap::BottomRight @endlink|@ref Snap::InsideX.
         */
        /*implicit*/ BasicSnapLayoutColumnRight(const BasicAnchor<UserInterface>& anchor): BasicSnapLayout<UserInterface>{anchor, *this} {}

        /**
         * @brief Construct from an unspecialized layout
         *
         * Calls @ref setChildSnap() with
         * @link Snap::BottomRight @endlink|@ref Snap::InsideX and
         * @ref addFlags() with @ref SnapLayoutFlag::PropagateMarginX and
         * @relativeref{SnapLayoutFlag,PropagateMarginY} unless
         * @relativeref{SnapLayoutFlag,IgnoreOverflowX} and
         * @relativeref{SnapLayoutFlag,IgnoreOverflowY} is already specified in
         * given direction, allowing margins from nested widgets to be
         * collapsed with parent's neigbors as well.
         */
        /*implicit*/ BasicSnapLayoutColumnRight(const BasicSnapLayout<UserInterface>& layout): BasicSnapLayout<UserInterface>{layout, *this} {}

        /** @brief Conversion from another specialized layout is not allowed */
        template<class T, typename std::enable_if<std::is_base_of<BasicSnapLayout<UserInterface>, T>::value, int>::type = 0> BasicSnapLayoutColumnRight(const T&) = delete;
};

/**
@brief Filled column layout using @ref SnapLayouter
@m_since_latest_{extras}

Places children in a column from top to bottom, filling the available width.
Meant to be used through concrete a typedef such as @ref SnapLayoutColumnFill.
@see @ref BasicSnapLayoutColumn, @ref BasicSnapLayoutColumnLeft,
    @ref BasicSnapLayoutColumnRight, @ref BasicSnapLayoutRowFill
*/
template<class UserInterface> class BasicSnapLayoutColumnFill: public BasicSnapLayout<UserInterface> {
    public:
        /**
         * @brief Construct from an anchor
         *
         * Calls @ref setChildSnap() with
         * @link Snap::Bottom @endlink|@ref Snap::FillX.
         */
        /*implicit*/ BasicSnapLayoutColumnFill(const BasicAnchor<UserInterface>& anchor): BasicSnapLayout<UserInterface>{anchor, *this} {}

        /**
         * @brief Construct from an unspecialized layout
         *
         * Calls @ref setChildSnap() with
         * @link Snap::Bottom @endlink|@ref Snap::FillX and @ref addFlags()
         * with @ref SnapLayoutFlag::PropagateMarginX and
         * @relativeref{SnapLayoutFlag,PropagateMarginY} unless
         * @relativeref{SnapLayoutFlag,IgnoreOverflowX} and
         * @relativeref{SnapLayoutFlag,IgnoreOverflowY} is already specified in
         * given direction, allowing margins from nested widgets to be
         * collapsed with parent's neigbors as well.
         */
        /*implicit*/ BasicSnapLayoutColumnFill(const BasicSnapLayout<UserInterface>& layout): BasicSnapLayout<UserInterface>{layout, *this} {}

        /** @brief Conversion from another specialized layout is not allowed */
        template<class T, typename std::enable_if<std::is_base_of<BasicSnapLayout<UserInterface>, T>::value, int>::type = 0> BasicSnapLayoutColumnFill(const T&) = delete;
};

/**
@brief Row layout using @ref SnapLayouter
@m_since_latest_{extras}

Places children in a row from left to right, aligned to the center vertically.
Meant to be used through concrete a typedef such as @ref SnapLayoutRow.
@see @ref BasicSnapLayoutRowTop, @ref BasicSnapLayoutRowBottom,
    @ref BasicSnapLayoutRowFill, @ref BasicSnapLayoutColumn
*/
template<class UserInterface> class BasicSnapLayoutRow: public BasicSnapLayout<UserInterface> {
    public:
        /**
         * @brief Construct from an anchor
         *
         * Calls @ref setChildSnap() with @ref Snap::Right.
         */
        /*implicit*/ BasicSnapLayoutRow(const BasicAnchor<UserInterface>& anchor): BasicSnapLayout<UserInterface>{anchor, *this} {}

        /**
         * @brief Construct from an unspecialized layout
         *
         * Calls @ref setChildSnap() with @ref Snap::Right and @ref addFlags()
         * with @ref SnapLayoutFlag::PropagateMarginX and
         * @relativeref{SnapLayoutFlag,PropagateMarginY} unless
         * @relativeref{SnapLayoutFlag,IgnoreOverflowX} and
         * @relativeref{SnapLayoutFlag,IgnoreOverflowY} is already specified in
         * given direction, allowing margins from nested widgets to be
         * collapsed with parent's neigbors as well.
         */
        /*implicit*/ BasicSnapLayoutRow(const BasicSnapLayout<UserInterface>& layout): BasicSnapLayout<UserInterface>{layout, *this} {}

        /** @brief Conversion from another specialized layout is not allowed */
        template<class T, typename std::enable_if<std::is_base_of<BasicSnapLayout<UserInterface>, T>::value, int>::type = 0> BasicSnapLayoutRow(const T&) = delete;
};

/**
@brief Top-aligned row layout using @ref SnapLayouter
@m_since_latest_{extras}

Places children in a row from left to right, aligned to the top. Meant to be
used through concrete a typedef such as @ref SnapLayoutRowTop.
@see @ref BasicSnapLayoutRow, @ref BasicSnapLayoutRowBottom,
    @ref BasicSnapLayoutRowFill, @ref BasicSnapLayoutColumnLeft
*/
template<class UserInterface> class BasicSnapLayoutRowTop: public BasicSnapLayout<UserInterface> {
    public:
        /**
         * @brief Construct from an anchor
         *
         * Calls @ref setChildSnap() with
         * @link Snap::TopRight @endlink|@ref Snap::InsideY.
         */
        /*implicit*/ BasicSnapLayoutRowTop(const BasicAnchor<UserInterface>& anchor): BasicSnapLayout<UserInterface>{anchor, *this} {}

        /**
         * @brief Construct from an unspecialized layout
         *
         * Calls @ref setChildSnap() with
         * @link Snap::TopRight @endlink|@ref Snap::InsideY and @ref addFlags()
         * with @ref SnapLayoutFlag::PropagateMarginX and
         * @relativeref{SnapLayoutFlag,PropagateMarginY} unless
         * @relativeref{SnapLayoutFlag,IgnoreOverflowX} and
         * @relativeref{SnapLayoutFlag,IgnoreOverflowY} is already specified in
         * given direction, allowing margins from nested widgets to be
         * collapsed with parent's neigbors as well.
         */
        /*implicit*/ BasicSnapLayoutRowTop(const BasicSnapLayout<UserInterface>& layout): BasicSnapLayout<UserInterface>{layout, *this} {}

        /** @brief Conversion from another specialized layout is not allowed */
        template<class T, typename std::enable_if<std::is_base_of<BasicSnapLayout<UserInterface>, T>::value, int>::type = 0> BasicSnapLayoutRowTop(const T&) = delete;
};

/**
@brief Bottom-aligned row layout using @ref SnapLayouter
@m_since_latest_{extras}

Places children in a row from left to right, aligned to the bottom. Meant
to be used through concrete a typedef such as @ref SnapLayoutRowBottom.
@see @ref BasicSnapLayoutRow, @ref BasicSnapLayoutRowTop,
    @ref BasicSnapLayoutRowFill, @ref BasicSnapLayoutColumnRight
*/
template<class UserInterface> class BasicSnapLayoutRowBottom: public BasicSnapLayout<UserInterface> {
    public:
        /**
         * @brief Construct from an anchor
         *
         * Calls @ref setChildSnap() with
         * @link Snap::BottomRight @endlink|@ref Snap::InsideY.
         */
        /*implicit*/ BasicSnapLayoutRowBottom(const BasicAnchor<UserInterface>& anchor): BasicSnapLayout<UserInterface>{anchor, *this} {}

        /**
         * @brief Construct from an unspecialized layout
         *
         * Calls @ref setChildSnap() with
         * @link Snap::BottomRight @endlink|@ref Snap::InsideY and
         * @ref addFlags() with @ref SnapLayoutFlag::PropagateMarginX and
         * @relativeref{SnapLayoutFlag,PropagateMarginY} unless
         * @relativeref{SnapLayoutFlag,IgnoreOverflowX} and
         * @relativeref{SnapLayoutFlag,IgnoreOverflowY} is already specified in
         * given direction, allowing margins from nested widgets to be
         * collapsed with parent's neigbors as well.
         */
        /*implicit*/ BasicSnapLayoutRowBottom(const BasicSnapLayout<UserInterface>& layout): BasicSnapLayout<UserInterface>{layout, *this} {}

        /** @brief Conversion from another specialized layout is not allowed */
        template<class T, typename std::enable_if<std::is_base_of<BasicSnapLayout<UserInterface>, T>::value, int>::type = 0> BasicSnapLayoutRowBottom(const T&) = delete;
};

/**
@brief Filled row layout using @ref SnapLayouter
@m_since_latest_{extras}

Places children in a row from left to right, filling the available height.
Meant to be used through concrete a typedef such as @ref SnapLayoutRowFill.
@see @ref BasicSnapLayoutRow, @ref BasicSnapLayoutRowTop,
    @ref BasicSnapLayoutRowBottom, @ref BasicSnapLayoutColumnFill
*/
template<class UserInterface> class BasicSnapLayoutRowFill: public BasicSnapLayout<UserInterface> {
    public:
        /**
         * @brief Construct from an anchor
         *
         * Calls @ref setChildSnap() with
         * @link Snap::Right @endlink|@ref Snap::FillY.
         */
        /*implicit*/ BasicSnapLayoutRowFill(const BasicAnchor<UserInterface>& anchor): BasicSnapLayout<UserInterface>{anchor, *this} {}

        /**
         * @brief Construct from an unspecialized layout
         *
         * Calls @ref setChildSnap() with
         * @link Snap::Right @endlink|@ref Snap::FillY and
         * @ref addFlags() with @ref SnapLayoutFlag::PropagateMarginX and
         * @relativeref{SnapLayoutFlag,PropagateMarginY} unless
         * @relativeref{SnapLayoutFlag,IgnoreOverflowX} and
         * @relativeref{SnapLayoutFlag,IgnoreOverflowY} is already specified in
         * given direction, allowing margins from nested widgets to be
         * collapsed with parent's neigbors as well.
         */
        /*implicit*/ BasicSnapLayoutRowFill(const BasicSnapLayout<UserInterface>& layout): BasicSnapLayout<UserInterface>{layout, *this} {}

        /** @brief Conversion from another specialized layout is not allowed */
        template<class T, typename std::enable_if<std::is_base_of<BasicSnapLayout<UserInterface>, T>::value, int>::type = 0> BasicSnapLayoutRowFill(const T&) = delete;
};

/**
@brief Layout using @ref SnapLayouter for use with @ref UserInterface
@m_since_latest_{extras}

Converting instance of this type to @ref SnapLayoutColumn, @ref SnapLayoutRow,
@ref SnapLayoutColumn, @ref SnapLayoutRow and other typedefs specialize the
layout for concrete placement rules. See the @ref AbstractSnapLayout
documentation for more information.
*/
typedef BasicSnapLayout<UserInterface> SnapLayout;

/**
@brief Column layout using @ref SnapLayouter for use with @ref UserInterface
@m_since_latest_{extras}

Places children in a column from top to bottom, aligned to the center
horizontally.
@see @ref SnapLayoutColumnLeft, @ref SnapLayoutColumnRight,
    @ref SnapLayoutColumnFill, @ref SnapLayoutRow
*/
typedef BasicSnapLayoutColumn<UserInterface> SnapLayoutColumn;

/**
@brief Left-aligned column layout using @ref SnapLayouter for use with @ref UserInterface
@m_since_latest_{extras}

Places children in a column from top to bottom, aligned to the left.
@see @ref SnapLayoutColumn, @ref SnapLayoutColumnRight,
    @ref SnapLayoutColumnFill, @ref SnapLayoutRowTop
*/
typedef BasicSnapLayoutColumnLeft<UserInterface> SnapLayoutColumnLeft;

/**
@brief Right-aligned column layout using @ref SnapLayouter for use with @ref UserInterface
@m_since_latest_{extras}

Places children in a column from top to bottom, aligned to the right.
@see @ref SnapLayoutColumn, @ref SnapLayoutColumnLeft,
    @ref SnapLayoutColumnFill, @ref SnapLayoutRowBottom
*/
typedef BasicSnapLayoutColumnRight<UserInterface> SnapLayoutColumnRight;

/**
@brief Filled column layout using @ref SnapLayouter for use with @ref UserInterface
@m_since_latest_{extras}

Places children in a column from top to bottom, filling the available width.
@see @ref SnapLayoutColumn, @ref SnapLayoutColumnLeft,
    @ref SnapLayoutColumnRight, @ref SnapLayoutRowFill
*/
typedef BasicSnapLayoutColumnFill<UserInterface> SnapLayoutColumnFill;

/**
@brief Row layout using @ref SnapLayouter for use with @ref UserInterface
@m_since_latest_{extras}

Places children in a row from left to right, aligned to the center vertically.
@see @ref SnapLayoutRowTop, @ref SnapLayoutRowBottom, @ref SnapLayoutRowFill,
    @ref SnapLayoutColumn
*/
typedef BasicSnapLayoutRow<UserInterface> SnapLayoutRow;

/**
@brief Top-aligned row layout using @ref SnapLayouter for use with @ref UserInterface
@m_since_latest_{extras}

Places children in a row from left to right, aligned to the top.
@see @ref SnapLayoutRow, @ref SnapLayoutRowBottom, @ref SnapLayoutRowFill,
    @ref SnapLayoutColumnLeft
*/
typedef BasicSnapLayoutRowTop<UserInterface> SnapLayoutRowTop;

/**
@brief Bottom-aligned row layout using @ref SnapLayouter for use with @ref UserInterface
@m_since_latest_{extras}

Places children in a row from left to right, aligned to the bottom.
@see @ref SnapLayoutRow, @ref SnapLayoutRowTop, @ref SnapLayoutRowFill,
    @ref SnapLayoutColumnRight
*/
typedef BasicSnapLayoutRowBottom<UserInterface> SnapLayoutRowBottom;

/**
@brief Filled row layout using @ref SnapLayouter for use with @ref UserInterface
@m_since_latest_{extras}

Places children in a row from left to right, filling the available height.
@see @ref SnapLayoutRow, @ref SnapLayoutRowTop, @ref SnapLayoutRowBottom,
    @ref SnapLayoutColumnFill
*/
typedef BasicSnapLayoutRowFill<UserInterface> SnapLayoutRowFill;

}}

#endif
