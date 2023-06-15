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
 * @brief Class @ref Magnum::Whee::PointerEvent, enum @ref Magnum::Whee::Pointer
 * @m_since_latest
 */

#include <Magnum/Math/Vector2.h>

#include "Magnum/Whee/visibility.h"

namespace Magnum { namespace Whee {

/**
@brief Pointer type
@m_since_latest

@see @ref PointerEvent
*/
enum class Pointer: UnsignedByte {
    MouseLeft,      /**< Left mouse button */
    MouseMiddle,    /**< Middle mouse button */
    MouseRight,     /**< Right mouse button */
    Finger,         /**< Finger */
    Pen,            /**< Pen */
    Eraser          /**< Eraser */
};

/**
@debugoperatorenum{Pointer}
@m_since_latest
*/
MAGNUM_WHEE_EXPORT Debug& operator<<(Debug& debug, Pointer value);

/**
@brief Pointer event
@m_since_latest

@see @ref AbstractUserInterface::pointerPressEvent(),
    @ref AbstractUserInterface::pointerReleaseEvent(),
    @ref AbstractLayer::pointerPressEvent(),
    @ref AbstractLayer::pointerReleaseEvent()
*/
class PointerEvent {
    public:
        /**
         * @brief Constructor
         * @param type      Pointer type that got pressed or released
         *
         * The position is set from @ref AbstractUserInterface event handler
         * internals.
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
         * @brief Set event position
         *
         * Used internally from @ref AbstractUserInterface event handlers.
         * Exposed just for testing purposes, there should be no need to call
         * this function directly.
         */
        void setPosition(const Vector2& position) {
            _position = position;
        }

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
        Vector2 _position;
        Pointer _type;
        bool _accepted = false;
};

}}

#endif
