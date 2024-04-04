#ifndef Magnum_Whee_SnapLayouter_h
#define Magnum_Whee_SnapLayouter_h
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
 * @brief Class @ref Magnum::Whee::SnapLayouter, @ref Magnum::Whee:SnapLayout, enum @ref Magnum::Whee::Snap, enum set @ref Magnum::Whee::Snaps
 * @m_since_latest
 */

#include <Corrade/Containers/Reference.h>

#include "Magnum/Whee/AbstractLayouter.h"

namespace Magnum { namespace Whee {

    // TODO docs from the original
enum class Snap: UnsignedByte {
    // TODO sort in the order of padidng balue
    Top = 1 << 3,
    Left = 1 << 0,
    Bottom = 1 << 2,
    Right = 1 << 1,

    InsideX = 1 << 4,
    InsideY = 1 << 5,
    Inside = InsideX|InsideY,

    NoSpaceX = 1 << 6,
    NoSpaceY = 1 << 7,
    NoSpace = NoSpaceX|NoSpaceY,

    // TODO ??? there will be a shitload of combinations, no? 256!!!!!!!!!!!!
    // TopLeftInside = Top|Left|InsideX|InsideY
};

/**
@debugoperatorenum{Snap}
@m_since_latest
*/
MAGNUM_WHEE_EXPORT Debug& operator<<(Debug& debug, Snap value);

typedef Containers::EnumSet<Snap> Snaps;

/**
@debugoperatorenum{Snaps}
@m_since_latest
*/
MAGNUM_WHEE_EXPORT Debug& operator<<(Debug& debug, Snaps value);

CORRADE_ENUMSET_OPERATORS(Snaps)

// TODO fuck the name is bad now .. SnapLayouter?? uhg ahahah
class MAGNUM_WHEE_EXPORT SnapLayouter: public AbstractLayouter {
    public:
        explicit SnapLayouter(LayouterHandle handle);

        ~SnapLayouter();

        // TODO these are global, make also layout-local variants
        // TODO ugh then i will need a way to get the parent layout to query it, uhhhhhhhhhhhh
        /** @brief Left, top, right and bottom padding inside a node */
        Vector4 padding() const;

        /**
         * @brief Set different left, top, right and bottom padding inside a node
         * @return Reference to self (for method chaining)
         *
         * @see @ref setPadding(const Vector2&), @ref setPadding(Float)
         */
        // TODO document that this is a global value
        // TODO then there needs to be a layout-local value alsoé
        SnapLayouter& setPadding(const Vector4& padding);

        /**
         * @brief Set different horizontal and vertical padding inside a node
         * @return Reference to self (for method chaining)
         *
         * @see @ref setPadding(const Vector4&), @ref setPadding(Float)
         */
        SnapLayouter& setPadding(const Vector2& padding);

        /**
         * @brief Set padding inside a node
         * @return Reference to self (for method chaining)
         *
         * @see @ref setPadding(const Vector4&), @ref setPadding(const Vector2&)
         */
        SnapLayouter& setPadding(Float padding);

        /** @brief Horizontal and vertical margin between nodes */
        Vector2 margin() const;

        /**
         * @brief Set different horizontal and vertical marging between nodes
         * @return Reference to self (for method chaining)
         *
         * @see @ref setMargin(Float)
         */
        SnapLayouter& setMargin(const Vector2& margin);

        /**
         * @brief Set margin between nodes
         * @return Reference to self (for method chaining)
         *
         * @see @ref setMargin(const Vector2&)
         */
        SnapLayouter& setMargin(Float margin);

        // TODO allow to to be null? because that happens when the parent is removed, anyway
        // TODO AHHHHHH what to do?
        LayoutHandle add(NodeHandle node, Snaps snap, NodeHandle target);

        Snaps snap(LayoutHandle handle) const;
        Snaps snap(LayouterDataHandle handle) const;

        void setSnap(LayoutHandle handle, Snaps snaps);
        void setSnap(LayouterDataHandle handle, Snaps snaps);

        NodeHandle target(LayoutHandle handle) const;
        NodeHandle target(LayouterDataHandle handle) const;

        // TODO allow this? null???
        void setTarget(LayoutHandle handle, NodeHandle target);
        void setTarget(LayouterDataHandle handle, NodeHandle target);

        // TODO publicize remove also? or not
        // TODO needs to deal with broken chains

    private:
        // TODO orders by "parent" // TODO what is this TODO???
        MAGNUM_WHEE_LOCAL void doUpdate(Containers::BitArrayView layoutIdsToUpdate, const Containers::StridedArrayView1D<const UnsignedInt>& topLevelLayoutIds, const Containers::StridedArrayView1D<Vector2>& nodeOffsets, const Containers::StridedArrayView1D<Vector2>& nodeSizes) override;

        struct State;
        Containers::Pointer<State> _state;
};

class MAGNUM_WHEE_EXPORT SnapLayout {
    public:
        // TODO mention both ui an dlayouter anre expected to outlinve thuis instance
        explicit SnapLayout(UserInterface& ui, SnapLayouter& layouter, NodeHandle parent);
        // TODO ehhhhh so this creates a new bleh bleh and eh eh
        // TODO a new node (why???? duplicated functionality) and sets it as parent
        // TODO drop, instead it should have setNextParent()
        // explicit SnapLayout(UserInterface& ui, SnapLayouter& layouter, NodeHandle parent,  Snaps snaps, NodeHandle target, const Vector2& offset, const Vector2& size, NodeFlags flags = {});
        // explicit SnapLayout(UserInterface& ui, SnapLayouter& layouter, NodeHandle parent,  Snaps snaps, const Vector2& offset, const Vector2& size, NodeFlags flags = {});
        // explicit SnapLayout(UserInterface& ui, SnapLayouter& layouter, NodeHandle parent, Snaps snaps, NodeHandle target, const Vector2& size, NodeFlags flags = {});
        // explicit SnapLayout(UserInterface& ui, SnapLayouter& layouter, NodeHandle parent, Snaps snaps, const Vector2& size, NodeFlags flags = {});

        SnapLayout(const SnapLayout&) = delete;
        // TODO mention it's destructive
        SnapLayout(SnapLayout&&) noexcept;

        ~SnapLayout();

        SnapLayout& operator=(const SnapLayout&) = delete;
        // TODO mention it's destructive
        SnapLayout& operator=(SnapLayout&&) noexcept;

        NodeHandle nextParent() const;
        SnapLayout& setNextParent(NodeHandle handle);

        // TODO what for? the problem is that anything that references a node with that offset will cause everything else to get shifted
        // TODO might not be if it's applied after everything
        // TODO test!
        // TODO it feels kind of useless tho
        Vector2 nextOffset() const;
        SnapLayout& setNextOffset(const Vector2& offset);

        Vector2 nextSize() const;
        SnapLayout& setNextSize(const Vector2& size); // TODO setNextSize?

        Snaps nextSnap() const;
        SnapLayout& setNextSnap(Snaps snap); // TODO setNextSnap?

        // TODO uh?
        NodeHandle nextTarget() const;
        // TODO setNextTarget(), if null then falls back to the node created by last snap()?

        // TODO all these op() also? shorter
        Anchor operator()(Snaps snap, NodeHandle target, const Vector2& offset, const Vector2& size, NodeFlags flags = {});
        // TODO should this pick nextOffset or zero??
        Anchor operator()(Snaps snap, NodeHandle target, const Vector2& size, NodeFlags flags = {});
        Anchor operator()(Snaps snap, NodeHandle target, NodeFlags flags = {});
        Anchor operator()(Snaps snap, const Vector2& offset, const Vector2& size, NodeFlags flags = {});
        Anchor operator()(Snaps snap, const Vector2& size, NodeFlags flags = {});
        Anchor operator()(Snaps snap, NodeFlags flags = {});
        Anchor operator()(const Vector2& offset, const Vector2& size, NodeFlags flags = {});
        Anchor operator()(const Vector2& size, NodeFlags flags = {});
        Anchor operator()(NodeFlags flags = {});

        // TODO expects last & snap is set
        // TODO flags???
        operator Anchor();

    private:
        struct State;
        Containers::Pointer<State> _state;
};

}}

#endif
