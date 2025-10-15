#ifndef Magnum_Ui_NodeFlags_h
#define Magnum_Ui_NodeFlags_h
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
 * @brief Enum @ref Magnum::Ui::NodeFlag, enum set @ref Magnum::Ui::NodeFlags
 * @m_since_latest_{extras}
 */

#include <Corrade/Containers/EnumSet.h>

#include "Magnum/Ui/Ui.h"
#include "Magnum/Ui/visibility.h"

namespace Magnum { namespace Ui {

/**
@brief Node behavior flag
@m_since_latest_{extras}

@see @ref NodeFlags, @ref Ui-AbstractUserInterface-nodes-flags,
    @ref AbstractUserInterface::createNode(),
    @ref AbstractUserInterface::nodeFlags(),
    @ref AbstractUserInterface::setNodeFlags(),
    @ref AbstractUserInterface::addNodeFlags(),
    @ref AbstractUserInterface::clearNodeFlags(),
    @ref NodeAnimation::addFlagsBegin(), @ref NodeAnimation::addFlagsEnd(),
    @ref NodeAnimation::clearFlagsBegin(), @ref NodeAnimation::clearFlagsEnd()
*/
enum class NodeFlag: UnsignedByte {
    /**
     * The node, all nested nodes including nested top-level nodes, and all
     * attached data are hidden, i.e. not drawn and excluded from event
     * processing.
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
     *
     * Changing this flag causes @ref UserInterfaceState::NeedsNodeUpdate to be
     * set.
     * @see @ref NodeFlag::NoEvents, @ref NodeFlag::Disabled
     */
    Hidden = 1 << 0,

    /**
     * The node clips its contents. When enabled, child nodes that are
     * completely outside of the node rectangle are culled and not even drawn,
     * nodes that are partially outside are clipped. Nested top-level nodes are
     * not affected by this flag.
     *
     * Changing this flag causes @ref UserInterfaceState::NeedsNodeClipUpdate
     * to be set.
     */
    Clip = 1 << 1,

    /**
     * The node, all nested nodes and all attached data don't get any events
     * even if a particular layer implements event handlers. Nested top-level
     * nodes are not affected by this flag. Doesn't have any visual effect, see
     * @ref NodeFlag::Disabled or @ref NodeFlag::Hidden for
     * alternatives. Setting this flag causes @ref NodeFlag::Focusable to be
     * ignored on the node and all its children.
     *
     * Changing this flag causes
     * @ref UserInterfaceState::NeedsNodeEnabledUpdate to be set.
     */
    NoEvents = 1 << 2,

    /**
     * The node, all nested nodes and all attached data are disabled. Implies
     * @ref NodeFlag::NoEvents and additionally has a visual effect on layers
     * that implement a disabled state. Nested top-level nodes are not affected
     * by this flag. Setting this flag causes @ref NodeFlag::Focusable to be
     * ignored on the node and all its children.
     *
     * Changing this flag causes
     * @ref UserInterfaceState::NeedsNodeEnabledUpdate to be set.
     * @see @ref NodeFlag::Hidden
     */
    Disabled = NoEvents|(1 << 3),

    /**
     * Pointer events happening on nested nodes fall through to this node,
     * which can then accept them, causing a pointer cancel event to happen on
     * the original node. See @ref Ui-AbstractUserInterface-events-fallthrough
     * for a detailed description of this behavior.
     *
     * Changing this flag doesn't cause any @ref UserInterfaceState to be set,
     * it comes to effect when the next pointer event happens.
     * @see @ref PointerEvent::isFallthrough(),
     *      @ref PointerMoveEvent::isFallthrough(), @ref PointerCancelEvent,
     *      @ref AbstractUserInterface::pointerPressEvent(),
     *      @relativeref{AbstractUserInterface,pointerReleaseEvent()},
     *      @relativeref{AbstractUserInterface,pointerMoveEvent()}
     */
    FallthroughPointerEvents = 1 << 4,

    /**
     * The node can be focused, after which all key events are directed to it
     * instead of to a node currently under pointer. Focusing is done either by
     * a primary pointer press or programmatically via
     * @ref AbstractUserInterface::focusEvent(). If @ref NodeFlag::NoEvents or
     * @ref NodeFlag::Disabled is set on the same node or any of its parents,
     * this flag is ignored. If @ref NodeFlag::NoBlur is set on the same node
     * or any of its parents, this flag gets a precedence.
     *
     * Changing this flag causes
     * @ref UserInterfaceState::NeedsNodeEnabledUpdate to be set.
     * @see @ref Ui-AbstractUserInterface-events-focus, @ref NodeFlag::NoBlur
     */
    Focusable = 1 << 5,

    /**
     * By default, a primary pointer press on a node that is different from the
     * currently focused node blurs it. This flag disables that behavior on
     * given node and all nested nodes, which is useful for example with
     * virtual keyboards, where pressing on a key shouldn't cause the input
     * field to lose focus. If @ref NodeFlag::Focusable is set on a node
     * affected by this flag, this flag is ignored.
     *
     * Changing this flag causes
     * @ref UserInterfaceState::NeedsNodeEventMaskUpdate to be set.
     * @see @ref Ui-AbstractUserInterface-events-focus
     */
    NoBlur = 1 << 6
};

/**
@debugoperatorenum{NodeFlag}
@m_since_latest_{extras}
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, NodeFlag value);

/**
@brief Node behavior flags
@m_since_latest_{extras}

@see @ref AbstractUserInterface::createNode()
*/
typedef Containers::EnumSet<NodeFlag> NodeFlags;

/**
@debugoperatorenum{NodeFlags}
@m_since_latest_{extras}
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, NodeFlags value);

CORRADE_ENUMSET_OPERATORS(NodeFlags)

}}

#endif
