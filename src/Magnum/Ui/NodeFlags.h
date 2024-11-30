#ifndef Magnum_Ui_NodeFlags_h
#define Magnum_Ui_NodeFlags_h
/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023, 2024
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
 * @m_since_latest
 */

#include <Corrade/Containers/EnumSet.h>

#include "Magnum/Ui/Ui.h"
#include "Magnum/Ui/visibility.h"

namespace Magnum { namespace Ui {

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
     * The node can be focused, after which all key events are directed to it
     * instead of to a node currently under pointer. Focusing is done either by
     * a primary pointer press or programmatically via
     * @ref AbstractUserInterface::focusEvent(). If @ref NodeFlag::NoEvents or
     * @ref NodeFlag::Disabled is set on the same node or any of its parents,
     * this flag is ignored.
     *
     * Changing this flag causes
     * @ref UserInterfaceState::NeedsNodeEnabledUpdate to be set.
     */
    Focusable = 1 << 4,
};

/**
@debugoperatorenum{NodeFlag}
@m_since_latest
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, NodeFlag value);

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
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, NodeFlags value);

CORRADE_ENUMSET_OPERATORS(NodeFlags)

}}

#endif
