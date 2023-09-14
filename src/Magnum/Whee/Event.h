#ifndef Magnum_Whee_Event_h
#define Magnum_Whee_Event_h
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
 * @brief Class @ref Magnum::Whee::PointerEvent, @ref Magnum::Whee::PointerMoveEvent, enum @ref Magnum::Whee::Pointer, enum set @ref Magnum::Whee::Pointers
 * @m_since_latest
 */

#include <Magnum/Math/Vector2.h>

#include "Magnum/Whee/Whee.h"
#include "Magnum/Whee/visibility.h"

namespace Magnum { namespace Whee {

/**
@brief Pointer type
@m_since_latest

@see @ref PointerEvent, @ref PointerMoveEvent
*/
enum class Pointer: UnsignedByte {
    /* Zero value is reserved for an unknown pointer. All other values are
       mutually exclusive bits to be used in the Pointers set. */

    MouseLeft = 1 << 0,     /**< Left mouse button */
    MouseMiddle = 1 << 1,   /**< Middle mouse button */
    MouseRight = 1 << 2,    /**< Right mouse button */
    Finger = 1 << 3,        /**< Finger */
    Pen = 1 << 4,           /**< Pen */
    Eraser = 1 << 5         /**< Eraser */
};

/**
@debugoperatorenum{Pointer}
@m_since_latest
*/
MAGNUM_WHEE_EXPORT Debug& operator<<(Debug& debug, Pointer value);

/**
@brief Pointer types
@m_since_latest

@see @ref PointerMoveEvent
*/
typedef Containers::EnumSet<Pointer> Pointers;

/**
@debugoperatorenum{Pointers}
@m_since_latest
*/
MAGNUM_WHEE_EXPORT Debug& operator<<(Debug& debug, Pointers value);

CORRADE_ENUMSET_OPERATORS(Pointers)

/**
@brief Pointer press or release event
@m_since_latest

@see @ref AbstractUserInterface::pointerPressEvent(),
    @ref AbstractUserInterface::pointerReleaseEvent(),
    @ref AbstractLayer::pointerPressEvent(),
    @ref AbstractLayer::pointerReleaseEvent(),
    @ref AbstractLayer::pointerTapOrClickEvent()
*/
class PointerEvent {
    public:
        /**
         * @brief Constructor
         * @param type      Pointer type that got pressed or released
         *
         * The position, capture and hover properties are set from
         * @ref AbstractUserInterface event handler internals.
         */
        explicit PointerEvent(Pointer type): _type{type} {}

        /** @brief Pointer type that got pressed or released */
        Pointer type() const { return _type; }

        /**
         * @brief Event position
         *
         * Relative to the containing node.
         */
        Vector2 position() const { return _position; }

        /**
         * @brief Whether the event is captured on a node
         *
         * On a press event is always implicitly @cpp true @ce, on a release
         * event is @cpp true @ce only if the event happens on a captured node.
         * @see @ref isHovering()
         */
        bool isCaptured() const { return _captured; }

        /**
         * @brief Set whether to capture the event on a node
         *
         * By default, after a pointer press event, a node captures all
         * following pointer events until and including a pointer release, even
         * if they happen outside of the node area.
         *
         * If capture is disabled, the events are always sent to the actual
         * node under the pointer. Which means that for example a node can
         * receive a pointer press event without a corresponding release later,
         * or a release alone.
         *
         * Calling this function only makes sense on a pointer press event, it
         * has no effect on a pointer release event or a tap or click event.
         */
        void setCaptured(bool captured) {
            _captured = captured;
        }

        /**
         * @brief Whether the event is called on a node that's currently hovered
         *
         * Returns @cpp true @ce if @ref AbstractUserInterface::pointerEventHoveredNode()
         * is the same as the node the event is called on, @cpp false @ce
         * otherwise. In particular, is @cpp false @ce for a press or release
         * event that happened without a preceding move on given node, is also
         * @cpp false @ce if a release event happens outside of a captured
         * node.
         *
         * Note that even if this function returns @cpp true @ce, the event
         * handler still controls whether the pointer is actually treated as
         * being in an active area of the node by either accepting the event or
         * not accepting it and letting it potentially fall through to other
         * nodes.
         * @see @ref isCaptured(), @ref setAccepted()
         */
        bool isHovering() const { return _hovering; }

        /**
         * @brief Whether the event is accepted
         *
         * Implicitly @cpp false @ce.
         */
        bool isAccepted() const { return _accepted; }

        /**
         * @brief Set the event as accepted
         *
         * Once an event is accepted, it doesn't propagate further.
         */
        void setAccepted(bool accepted = true) {
            _accepted = accepted;
        }

    private:
        friend AbstractUserInterface;

        Vector2 _position;
        Pointer _type;
        bool _accepted = false;
        bool _captured = false;
        bool _hovering = false;
};

/**
@brief Pointer move event
@m_since_latest

@see @ref AbstractUserInterface::pointerMoveEvent(),
    @ref AbstractLayer::pointerMoveEvent(),
    @ref AbstractLayer::pointerEnterEvent(),
    @ref AbstractLayer::pointerLeaveEvent()
*/
class MAGNUM_WHEE_EXPORT PointerMoveEvent {
    public:
        /**
         * @brief Constructor
         * @param type      Pointer type that changed in this event or
         *      @ref Containers::NullOpt
         * @param types     Pointer types pressed in this event
         *
         * The position, capture and hover properties are set from
         * @ref AbstractUserInterface event handler internals.
         */
        explicit PointerMoveEvent(Containers::Optional<Pointer> type, Pointers types);

        /**
         * @brief Pointer type that changed in this event
         *
         * If no pointer changed in this event (i.e., all pointers that were
         * pressed before are still pressed), returns @ref Containers::NullOpt.
         * Use @ref types() to check what all pointers are pressed in this
         * event. If @ref type() is not empty and @ref types() contain
         * @ref type(), it means given pointer type was pressed, if they don't,
         * it means it was released.
         */
        Containers::Optional<Pointer> type() const;

        /**
         * @brief Pointer types pressed in this event
         *
         * Returns an empty set if no pointers are pressed, which happens for
         * example when a mouse is just moved around.
         */
        Pointers types() const { return _types; }

        /**
         * @brief Event position
         *
         * Relative to the containing node.
         */
        Vector2 position() const { return _position; }

        /**
         * @brief Position relative to previous pointer event
         *
         * Relative to the previous pointer event. If no pointer event happened
         * before, is a zero vector. For pointer enter and leave events it's a
         * zero vector always, as they happen immediately after another event.
         */
        Vector2 relativePosition() const { return _relativePosition; }

        /**
         * @brief Whether the event is captured on a node
         *
         * Is implicitly @cpp true @ce if the event happens on a captured node,
         * @cpp false @ce otherwise.
         * @see @ref isHovering()
         */
        bool isCaptured() const { return _captured; }

        /**
         * @brief Set whether to capture the event on a node
         *
         * By default, after a pointer press event, a node captures all
         * following pointer events until and including a pointer release, even
         * if they happen outside of the node area. If capture is disabled, the
         * events are always sent to the actual node under the pointer.
         *
         * The capture can be both disabled and enabled again for all pointer
         * move, enter and leave events, each time it's enabled again it'll
         * capture the actual node under the pointer. Calling this function has
         * no effect on a pointer leave event that isn't captured.
         */
        void setCaptured(bool captured) {
            _captured = captured;
        }

        /**
         * @brief Whether the event is called on a node that's currently hovered
         *
         * Returns @cpp true @ce if @ref AbstractUserInterface::pointerEventHoveredNode()
         * is the same as the node the event is called on, @cpp false @ce
         * otherwise. In particular, is @cpp false @ce for the first move event
         * happening on a node, @cpp true @ce for the enter event and all
         * subsequent accepted move events on the same node, @cpp false @ce for
         * the leave event. On a captured move event returns @cpp false @ce for
         * if the pointer was moved outside of the node area.
         *
         * Note that even if this function returns @cpp true @ce, the event
         * handler still controls whether the node actually appears in
         * @ref AbstractUserInterface::pointerEventHoveredNode() afterwards.
         * Accepting the event makes the node appear there. Not accepting it
         * makes the event potentially fall through to other nodes which may
         * then become hovered, if there are none then the hovered node becomes
         * null and subsequent move events called on this node will be called
         * with this function returning @cpp false @ce.
         * @see @ref isCaptured(), @ref setAccepted()
         */
        bool isHovering() const { return _hovering; }

        /**
         * @brief Whether the event is accepted
         *
         * Implicitly @cpp false @ce.
         */
        bool isAccepted() const { return _accepted; }

        /**
         * @brief Set the event as accepted
         *
         * Once an event is accepted, it doesn't propagate further.
         */
        void setAccepted(bool accepted = true) {
            _accepted = accepted;
        }

    private:
        friend AbstractUserInterface;

        Vector2 _position, _relativePosition;
        Pointer _type; /* NullOpt encoded as Pointer{} to avoid an include */
        Pointers _types;
        bool _accepted = false;
        bool _captured = false;
        bool _hovering = false;
};

}}

#endif
