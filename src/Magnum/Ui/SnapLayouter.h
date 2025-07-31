#ifndef Magnum_Ui_SnapLayouter_h
#define Magnum_Ui_SnapLayouter_h
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
 * @brief Class @ref Magnum::Ui::SnapLayouter, @ref Magnum::Ui::AbstractSnapLayout, @ref Magnum::Ui::BasicSnapLayout, typedef @ref Magnum::Ui::SnapLayout, enum @ref Magnum::Ui::Snap, enum set @ref Magnum::Ui::Snaps, function @ref Magnum::Ui::snap()
 * @m_since_latest
 */

#include <Corrade/Containers/Reference.h>

#include "Magnum/Ui/AbstractLayouter.h"

namespace Magnum { namespace Ui {

/**
@brief Layout snap

See particular values for detailed documentation.

Specifying neither @ref Snap::Left nor @ref Snap::Right will result in
horizontal centering. Specifying both @ref Snap::Left and @ref Snap::Right (or
the @ref Snap::FillX alias) will cause the node width to match width of the
target node or the user interface. In both cases it's as if @ref Snap::InsideX
was specified as well, taking horizontal @ref SnapLayouter::padding() into
account.

Specifying neither @ref Snap::Top nor @ref Snap::Bottom will result in verical
centering. Specifying both @ref Snap::Top and @ref Snap::Bottom (or the
@ref Snap::FillX alias) will cause the node height to match height the target
node or user interface. In both cases it's as if @ref Snap::InsideY was
specified as well, taking vertical @ref SnapLayouter::padding() into account.

Specifying @ref Snap::NoPadX and/or @ref Snap::NoPadY will ignore horizontal
and/or vertical @ref SnapLayouter::padding() and @ref SnapLayouter::margin().
@see @ref Snaps, @ref SnapLayouter, @ref SnapLayout
*/
enum class Snap: UnsignedByte {
    /**
     * Snaps right side of a node to left side of the target node, taking
     * horizontal @ref SnapLayouter::margin() into account.
     *
     * If combined with @ref Snap::InsideX, snaps left side of a node to left
     * side of the target node or user interface, taking left-side
     * @ref SnapLayouter::padding() into account.
     */
    Left = 1 << 0,

    /**
     * Snaps bottom side of a node to top side of the target node, taking
     * vertical @ref SnapLayouter::margin() into account.
     *
     * If combined with @ref Snap::InsideY, snaps top side of a node to top
     * side of the target node or user interface, taking top-side
     * @ref SnapLayouter::padding() into account.
     */
    Top = 1 << 1,

    /**
     * Snaps left side of a node to right side of the target node, taking
     * horizontal @ref SnapLayouter::margin() into account.
     *
     * If combined with @ref Snap::InsideX, snaps right side of a node to
     * right side of the target node or user interface, taking right-side
     * @ref SnapLayouter::padding() into account.
     */
    Right = 1 << 2,

    /**
     * Snaps top side of a node to bottom side of the target node, taking
     * vertical @ref SnapLayouter::margin() into account.
     *
     * If combined with @ref Snap::InsideY, snaps bottom side of a node to
     * bottom side of the target node or user interface, taking top-side
     * @ref SnapLayouter::padding() into account.
     */
    Bottom = 1 << 3,

    /**
     * Alias to specifying both @ref Snap::Top and @ref Snap::Left.
     * @m_since_latest
     */
    TopLeft = Top|Left,

    /**
     * Alias to specifying both @ref Snap::Bottom and @ref Snap::Left.
     * @m_since_latest
     */
    BottomLeft = Bottom|Left,

    /**
     * Alias to specifying both @ref Snap::Top and @ref Snap::Right.
     * @m_since_latest
     */
    TopRight = Top|Right,

    /**
     * Alias to specifying both @ref Snap::Bottom and @ref Snap::Right.
     * @m_since_latest
     */
    BottomRight = Bottom|Right,

    /**
     * Alias to specifying both @ref Snap::Left and @ref Snap::Right.
     * @m_since_latest
     */
    FillX = Left|Right,

    /**
     * Alias to specifying both @ref Snap::Top and @ref Snap::Bottom.
     * @m_since_latest
     */
    FillY = Top|Bottom,

    /**
     * Alias to specifying both @ref Snap::FillX and @ref Snap::FillY, or all
     * of @ref Snap::Left, @ref Snap::Right, @ref Snap::Top and
     * @ref Snap::Bottom.
     * @m_since_latest
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
     * @m_since_latest
     */
    Inside = InsideX|InsideY,

    /**
     * Ignore horizontal @ref SnapLayouter::padding() and
     * @ref SnapLayouter::margin().
     * @m_since_latest
     */
    NoPadX = 1 << 6,

    /**
     * Ignore vertical @ref SnapLayouter::padding() and
     * @ref SnapLayouter::margin().
     * @m_since_latest
     */
    NoPadY = 1 << 7,

    /**
     * Alias to specifying both @ref Snap::NoPadX and @ref Snap::NoPadY.
     * @m_since_latest
     */
    NoPad = NoPadX|NoPadY
};

/**
@debugoperatorenum{Snap}
@m_since_latest
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, Snap value);

/**
@brief Layout snaps

@see @ref SnapLayouter, @ref SnapLayout
*/
typedef Containers::EnumSet<Snap> Snaps;

/**
@debugoperatorenum{Snaps}
@m_since_latest
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, Snaps value);

CORRADE_ENUMSET_OPERATORS(Snaps)

/**
@brief Snap layouter
@m_since_latest

The actual layout creation is done through the @ref AbstractSnapLayout helper,
or its derived @ref BasicSnapLayout template and the @ref SnapLayout typedef
restricting it to a concrete user interface instance.
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

        /** @brief Left, top, right and bottom padding inside a node */
        Vector4 padding() const;

        /**
         * @brief Set different left, top, right and bottom padding inside a node
         * @return Reference to self (for method chaining)
         *
         * Applied globally to all layouts, i.e. when this value changes, all
         * existing layouts will have their padding changed as well. Use
         * @ref setMargin() to set margin *between* nodes. Initially the
         * padding is @cpp {0.0f, 0.0f, 0.0f, 0.0f} @ce.
         *
         * Calling this function causes @ref LayouterState::NeedsUpdate to be
         * set.
         * @see @ref setPadding(const Vector2&), @ref setPadding(Float)
         */
        SnapLayouter& setPadding(const Vector4& padding);

        /**
         * @brief Set different horizontal and vertical padding inside a node
         * @return Reference to self (for method chaining)
         *
         * Same as calling @ref setPadding(const Vector4&) with the left, right
         * and top, bottom components being the same.
         * @see @ref setPadding(Float)
         */
        SnapLayouter& setPadding(const Vector2& padding);

        /**
         * @brief Set padding inside a node
         * @return Reference to self (for method chaining)
         *
         * Same as calling @ref setPadding(const Vector4&) with all values
         * being the same.
         * @see @ref setPadding(const Vector2&)
         */
        SnapLayouter& setPadding(Float padding);

        /** @brief Horizontal and vertical margin between nodes */
        Vector2 margin() const;

        /**
         * @brief Set different horizontal and vertical marging between nodes
         * @return Reference to self (for method chaining)
         *
         * Applied globally to all layouts, i.e. when this value changes, all
         * existing layouts will have their margin changed as well. Use
         * @ref setPadding() to set padding *inside* of a node. Initially the
         * margin is @cpp {0.0f, 0.0f} @ce.
         *
         * Calling this function causes @ref LayouterState::NeedsUpdate to be
         * set.
         * @see @ref setMargin(Float)
         */
        SnapLayouter& setMargin(const Vector2& margin);

        /**
         * @brief Set margin between nodes
         * @return Reference to self (for method chaining)
         *
         * Same as calling @ref setMargin(const Vector2&) with both values
         * being the same.
         */
        SnapLayouter& setMargin(Float margin);

        /**
         * @brief Remove a layout
         *
         * Delegates to @ref AbstractLayouter::remove(LayoutHandle).
         */
        void remove(LayoutHandle handle) {
            AbstractLayouter::remove(handle);
        }

        /**
         * @brief Remove a layout assuming it belongs to this layer
         *
         * Delegates to @ref AbstractLayouter::remove(LayouterDataHandle).
         */
        void remove(LayouterDataHandle handle) {
            AbstractLayouter::remove(handle);
        }

        /**
         * @brief Layout snap
         *
         * Expects that @p handle is valid. Note that if @ref target() is
         * @ref NodeHandle::Null, @ref Snap::Inside is implicitly considered to
         * be included as well, even if not part of the actual value.
         *
         * Similarly to @ref node() and @ref target(), the snap cannot be
         * changed after creation as it could break internal constraints.
         * @see @ref isHandleValid(LayoutHandle) const
         */
        Snaps snap(LayoutHandle handle) const;

        /**
         * @brief Layout snap assuming it belongs to this layouter
         *
         * Like @ref snap(LayoutHandle) const but without checking that
         * @p handle indeed belongs to this layouter. See its documentation for
         * more information.
         */
        Snaps snap(LayouterDataHandle handle) const;

        /**
         * @brief Layout target node
         *
         * Expects that @p handle is valid. Returns @ref NodeHandle::Null if
         * the layout is positioned relative to the whole user interface. Note
         * that the returned handle may be invalid if
         * @ref AbstractUserInterface::removeNode() was called on it or its
         * parent nodes and @ref AbstractUserInterface::update() hasn't been
         * called since.
         *
         * Similarly to @ref node() and @ref snap(), the target cannot be
         * changed after creation as it could break internal constraints.
         * @see @ref isHandleValid(LayoutHandle) const
         */
        NodeHandle target(LayoutHandle handle) const;

        /**
         * @brief Layout target node assuming it belongs to this layouter
         *
         * Like @ref target(LayoutHandle) const but without checking that
         * @p handle indeed belongs to this layouter. See its documentation for
         * more information.
         */
        NodeHandle target(LayouterDataHandle handle) const;

    private:
        friend AbstractSnapLayout;
        friend MAGNUM_UI_EXPORT AbstractAnchor snap(AbstractUserInterface&, SnapLayouter&, Snaps, NodeHandle, const Vector2&, const Vector2&, NodeFlags);

        /* Used only through SnapLayout, which ensures parenting consistency */
        MAGNUM_UI_LOCAL LayoutHandle add(NodeHandle node, Snaps snap, NodeHandle target);

        MAGNUM_UI_LOCAL void doSetSize(const Vector2& size) override;
        MAGNUM_UI_LOCAL void doUpdate(Containers::BitArrayView layoutIdsToUpdate, const Containers::StridedArrayView1D<const UnsignedInt>& topLevelLayoutIds, const Containers::StridedArrayView1D<const NodeHandle>& nodeParents, const Containers::StridedArrayView1D<Vector2>& nodeOffsets, const Containers::StridedArrayView1D<Vector2>& nodeSizes) override;

        struct State;
        Containers::Pointer<State> _state;
};

/**
@brief Base for @ref SnapLayouter layout creation helpers
@m_since_latest

Combines @ref SnapLayouter together with a reference to
@ref AbstractUserInterface in order to create @ref AbstractAnchor instances to
directly feed into @ref AbstractWidget constructors. Similarly to
@ref Anchor and @ref Widget, the @ref BasicSnapLayout template and the
@ref SnapLayout typedef then restrict the type to a concrete user interface
instance such as @ref UserInterface.

This class is just a transient helper to aid with laying out nodes relative to
each other. After they're created, the @ref AbstractSnapLayout instance isn't
needed for anything anymore and can be removed. For snapping individual nodes
you can use the stateless @ref snap() functions instead.
*/
class MAGNUM_UI_EXPORT AbstractSnapLayout {
    public:
        /**
         * @brief Constructor
         * @param ui        User interface instance
         * @param layouter  Layouter instance
         * @param snapFirst How to snap the first layout
         * @param target    Target to which to snap the first layout
         * @param snapNext  How to snap the next layouts
         *
         * The @p snapFirst is used for creating the first ever layout,
         * relative to @p target. After that, the following layouts are all
         * snapped to the previous created one according to @p snapNext.
         *
         * Since it makes little sense to layout multiple root nodes relative
         * to each other, the @p target is expected to be a valid non-null
         * handle. If @p snapFirst snaps inside of the node, the @p target is
         * used as a parent of the newly created nodes, if @p snapFirst snaps
         * outside of the node, the parent of @p target is used as a parent of
         * the newly created nodes, in which case @p target is also expected to
         * not be a root node.
         *
         * Layouting a root node relative to the user interface itself is
         * possible with @ref snap(AbstractUserInterface&, SnapLayouter&, Snaps, const Vector2&, const Vector2&, NodeFlags)
         * and overloads.
         * @see @ref AbstractUserInterface::isHandleValid(NodeHandle) const
         */
        explicit AbstractSnapLayout(AbstractUserInterface& ui, SnapLayouter& layouter, Snaps snapFirst, NodeHandle target, Snaps snapNext);

        /** @brief Copying is not allowed */
        AbstractSnapLayout(const AbstractSnapLayout&) = delete;

        /** @brief Move constructor */
        AbstractSnapLayout(AbstractSnapLayout&&) noexcept = default;

        /** @brief Copying is not allowed */
        AbstractSnapLayout& operator=(const AbstractSnapLayout&) = delete;

        /** @brief Move assignment */
        AbstractSnapLayout& operator=(AbstractSnapLayout&&) noexcept = default;

        /** @brief User interface instance the nodes are created in */
        AbstractUserInterface& ui() const { return _ui; }

        /** @brief Layouter instance */
        SnapLayouter& layouter() const { return _layouter; }

        /**
         * @brief Node to which all created nodes are parented
         *
         * Never a @ref NodeHandle::Null.
         */
        NodeHandle parent() const { return _parent; }

        /**
         * @brief How to snap the first layout
         *
         * @see @ref targetFirst()
         */
        Snaps snapFirst() const { return _snapFirst; }

        /**
         * @brief How to snap the next layouts
         *
         * @see @ref targetNext()
         */
        Snaps snapNext() const { return _snapNext; }

        /**
         * @brief Target node for the first layout
         *
         * Used as a target for layouting the first node according to
         * @ref snapFirst(). Each subsequent nodes is then targeted to the
         * previously created one, which is exposed in @ref targetNext(). Never
         * a @ref NodeHandle::Null.
         */
        NodeHandle targetFirst() const { return _targetFirst; }

        /**
         * @brief Target node for the next layout
         *
         * Contains a handle of the latest created node, which is then used as
         * a target for layouting the next node according to @ref snapNext().
         * If @ref NodeHandle::Null, no nodes were created yet, and the first
         * one is layouted against @ref targetFirst() according to
         * @ref snapFirst().
         */
        NodeHandle targetNext() const { return _targetNext; }

        /**
         * @brief Create a layouted node
         * @param size      Desired node size
         * @param flags     Flags to create the node with
         * @return Anchor containing the new node and associated layout
         *
         * Creates a node parented to @ref parent() with an associated layout.
         * The @p size is ignored in horizontal / vertical direction if the
         * layout is snapped to the whole width / height of the target node.
         * Calls @ref AbstractUserInterface::createNode() and
         * @ref AbstractLayouter::add() internally.
         * @see @ref snapFirst(), @ref snapNext()
         */
        AbstractAnchor operator()(const Vector2& size, NodeFlags flags = {});

        /**
         * @brief Create a layouted node with custom offset
         *
         * Compared to @ref operator()(const Vector2&, NodeFlags) the @p offset
         * is added to the offset calculated by the layout.
         */
        AbstractAnchor operator()(const Vector2& offset, const Vector2& size, NodeFlags flags = {});

    private:
        Containers::Reference<AbstractUserInterface> _ui;
        Containers::Reference<SnapLayouter> _layouter;
        NodeHandle _parent, _targetFirst;
        NodeHandle _targetNext{};
        Snaps _snapFirst, _snapNext;
};

/**
@brief Templated base for @ref SnapLayouter layout creation helpers
@m_since_latest

Restricts @ref AbstractSnapLayout to a concrete user interface instance. See
the base class documentation for more information.
@see @ref SnapLayout
*/
template<class UserInterface> class BasicSnapLayout: public AbstractSnapLayout {
    public:
        /** @copydoc AbstractSnapLayout::AbstractSnapLayout(AbstractUserInterface&, SnapLayouter&, Snaps, NodeHandle, Snaps) */
        explicit BasicSnapLayout(UserInterface& ui, SnapLayouter& layouter, Snaps snapFirst, NodeHandle target, Snaps snapNext): AbstractSnapLayout{ui, layouter, snapFirst, target, snapNext} {}

        /**
         * @brief Construct using the default @ref SnapLayouter
         *
         * Delegates to @ref BasicSnapLayout(UserInterface&, SnapLayouter&, Snaps, NodeHandle, Snaps)
         * with the instance coming from @ref UserInterface::snapLayouter().
         */
        explicit BasicSnapLayout(UserInterface& ui, Snaps snapFirst, NodeHandle target, Snaps snapNext): BasicSnapLayout{ui, ui.snapLayouter(), snapFirst, target, snapNext} {}

        /** @brief Copying is not allowed */
        BasicSnapLayout(const BasicSnapLayout&) = delete;

        /** @brief Move constructor */
        BasicSnapLayout(BasicSnapLayout&&) noexcept = default;

        /** @brief Copying is not allowed */
        BasicSnapLayout& operator=(const BasicSnapLayout&) = delete;

        /** @brief Move assignment */
        BasicSnapLayout& operator=(BasicSnapLayout&&) noexcept = default;

        /** @brief User interface instance the nodes are created in */
        UserInterface& ui() const {
            return static_cast<UserInterface&>(AbstractSnapLayout::ui());
        }

        /** @copydoc AbstractSnapLayout::operator()(const Vector2&, NodeFlags) */
        BasicAnchor<UserInterface> operator()(const Vector2& size, NodeFlags flags = {}) {
            return BasicAnchor<UserInterface>{static_cast<UserInterface&>(AbstractSnapLayout::ui()), AbstractSnapLayout::operator()(size, flags)};
        }

        /** @copydoc AbstractSnapLayout::operator()(const Vector2&, const Vector2&, NodeFlags) */
        BasicAnchor<UserInterface> operator()(const Vector2& offset, const Vector2& size, NodeFlags flags = {}) {
            return BasicAnchor<UserInterface>{static_cast<UserInterface&>(AbstractSnapLayout::ui()), AbstractSnapLayout::operator()(offset, size, flags)};
        }
};

/**
@brief @ref SnapLayouter layout creation helper for use with @ref UserInterface
@m_since_latest

See the @ref AbstractSnapLayout class documentation for more information.
*/
typedef BasicSnapLayout<UserInterface> SnapLayout;

/**
@brief Create a node layouted with @ref SnapLayouter
@param ui           User interface instance
@param layouter     Layouter instance
@param snap         How to snap the layout
@param target       Target to which to snap the layout
@param size         Desired node size
@param flags        Flags to create the node with
@return Anchor containing the new node and associated layout
@m_since_latest

If @p target is @ref NodeHandle::Null, creates a root node and interprets
@p snap relative to the whole user interface, with it implicitly treated as if
@ref Snap::Inside was included.

If @p target is not null, it's expected to be valid. Then, if @p snap is inside
of the node, the @p target is used as a parent of the newly created node.
Otherwise, the parent of @p target is used as a parent of the newly created
node. If @p target is a root node, the newly created node is made as a root
node as well.

The @p size is ignored in horizontal / vertical direction if the layout is
snapped to the whole width / height of the target node. Calls
@ref AbstractUserInterface::createNode() and @ref AbstractLayouter::add()
internally.

Use @ref AbstractSnapLayout if you want to layout multiple nodes relative to
each other.
*/
MAGNUM_UI_EXPORT AbstractAnchor snap(AbstractUserInterface& ui, SnapLayouter& layouter, Snaps snap, NodeHandle target, const Vector2& size, NodeFlags flags = {});
/**
@overload
@m_since_latest
*/
MAGNUM_UI_EXPORT Anchor snap(UserInterface& ui, SnapLayouter& layouter, Snaps snap, NodeHandle target, const Vector2& size, NodeFlags flags = {});

/**
@brief Create a node layouted with the default @ref SnapLayouter
@m_since_latest

Delegates into @ref snap(UserInterface&, SnapLayouter&, Snaps, NodeHandle, const Vector2&, NodeFlags)
with the instance coming from @ref UserInterface::snapLayouter().
*/
MAGNUM_UI_EXPORT Anchor snap(UserInterface& ui, Snaps snap, NodeHandle target, const Vector2& size, NodeFlags flags = {});

/**
@brief Create a node layouted with @ref SnapLayouter with custom offset
@m_since_latest

Compared to @ref snap(AbstractUserInterface&, SnapLayouter&, Snaps, NodeHandle, const Vector2&, NodeFlags)
the @p offset is added to the offset calculated by the layout.
*/
MAGNUM_UI_EXPORT AbstractAnchor snap(AbstractUserInterface& ui, SnapLayouter& layouter, Snaps snap, NodeHandle target, const Vector2& offset, const Vector2& size, NodeFlags flags = {});
/**
@overload
@m_since_latest
*/
MAGNUM_UI_EXPORT Anchor snap(UserInterface& ui, SnapLayouter& layouter, Snaps snap, NodeHandle target, const Vector2& offset, const Vector2& size, NodeFlags flags = {});

/**
@brief Create a node layouted with the default @ref SnapLayouter with custom offset
@m_since_latest

Delegates into @ref snap(UserInterface&, SnapLayouter&, Snaps, NodeHandle, const Vector2&, const Vector2&, NodeFlags)
with the instance coming from @ref UserInterface::snapLayouter().
*/
MAGNUM_UI_EXPORT Anchor snap(UserInterface& ui, Snaps snap, NodeHandle target, const Vector2& offset, const Vector2& size, NodeFlags flags = {});

/**
@brief Create a root node layouted with @ref SnapLayouter
@m_since_latest

Equivalent to calling @ref snap(AbstractUserInterface&, SnapLayouter&, Snaps, NodeHandle, const Vector2&, NodeFlags)
with @p target being @ref NodeHandle::Null.
*/
MAGNUM_UI_EXPORT AbstractAnchor snap(AbstractUserInterface& ui, SnapLayouter& layouter, Snaps snap, const Vector2& size, NodeFlags flags = {});
/**
@overload
@m_since_latest
*/
MAGNUM_UI_EXPORT Anchor snap(UserInterface& ui, SnapLayouter& layouter, Snaps snap, const Vector2& size, NodeFlags flags = {});

/**
@brief Create a root node layouted with the default @ref SnapLayouter
@m_since_latest

Delegates into @ref snap(UserInterface&, SnapLayouter&, Snaps, const Vector2&, NodeFlags)
with the instance coming from @ref UserInterface::snapLayouter().
*/
MAGNUM_UI_EXPORT Anchor snap(UserInterface& ui, Snaps snap, const Vector2& size, NodeFlags flags = {});

/**
@brief Create a root node layouted with @ref SnapLayouter with custom offset
@m_since_latest

Compared to @ref snap(AbstractUserInterface&, SnapLayouter&, Snaps, const Vector2&, NodeFlags)
the @p offset is added to the offset calculated by the layout.
*/
MAGNUM_UI_EXPORT AbstractAnchor snap(AbstractUserInterface& ui, SnapLayouter& layouter, Snaps snap, const Vector2& offset, const Vector2& size, NodeFlags flags = {});
/**
@overload
@m_since_latest
*/
MAGNUM_UI_EXPORT Anchor snap(UserInterface& ui, SnapLayouter& layouter, Snaps snap, const Vector2& offset, const Vector2& size, NodeFlags flags = {});

/**
@brief Create a root node layouted with the default @ref SnapLayouter with custom offset
@m_since_latest

Delegates into @ref snap(UserInterface&, SnapLayouter&, Snaps, const Vector2&, const Vector2&, NodeFlags)
with the instance coming from @ref UserInterface::snapLayouter().
*/
MAGNUM_UI_EXPORT Anchor snap(UserInterface& ui, Snaps snap, const Vector2& offset, const Vector2& size, NodeFlags flags = {});

}}

#endif
