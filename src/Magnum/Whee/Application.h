#ifndef Magnum_Whee_Application_h
#define Magnum_Whee_Application_h
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
@brief @ref Magnum::Platform::Sdl2Application "Magnum::Platform::*Application" compatibility for @ref Magnum::Whee::AbstractUserInterface
@m_since_latest

Including this header allows you to pass @ref Magnum::Platform::Sdl2Application::MouseEvent "Magnum::Platform::*Application::MouseEvent"
and @ref Magnum::Platform::Sdl2Application::MouseMoveEvent "Magnum::Platform::*Application::MouseMoveEvent" to
@ref Magnum::Whee::AbstractUserInterface::pointerPressEvent(),
@relativeref{Magnum::Whee::AbstractUserInterface,pointerReleaseEvent()} and
@relativeref{Magnum::Whee::AbstractUserInterface,pointerMoveEvent()}. See
@ref Whee-AbstractUserInterface-application for more information.
*/

#include <Corrade/Containers/Optional.h>

#include "Magnum/Whee/AbstractUserInterface.h"
#include "Magnum/Whee/Event.h"

/* Listing these namespaces doesn't add anything to the docs, so don't */
#ifndef DOXYGEN_GENERATING_OUTPUT
namespace Magnum { namespace Whee { namespace Implementation {

template<class Event> Pointer pointerForButton(typename Event::Button button) {
    switch(button) {
        case Event::Button::Left: return Pointer::MouseLeft;
        case Event::Button::Middle: return Pointer::MouseMiddle;
        case Event::Button::Right: return Pointer::MouseRight;
        default: return {};
    }
}

template<class Event> Pointers pointersForButtons(typename Event::Buttons buttons) {
    Pointers pointers;
    if(buttons & Event::Button::Left)
        pointers |= Pointer::MouseLeft;
    if(buttons & Event::Button::Middle)
        pointers |= Pointer::MouseMiddle;
    if(buttons & Event::Button::Right)
        pointers |= Pointer::MouseRight;
    return pointers;
}

template<class Event> struct PointerEventConverter<Event, typename std::enable_if<sizeof(typename Event::Button)
    /* Without this, Clang (16, but probably others too) is only able to match
       this if MouseEvent::Buttons is 8-bit (such as in Sdl2Application but not
       GlfwApplication or EmscriptenApplication, which have 32-bit values). I
       suppose the boolean conversion in SFINAE contexts has some funny
       "optimization" that doesn't take the higher bits into account or some
       such. */
    #ifdef CORRADE_TARGET_CLANG
    >= 0
    #endif
>::type> {
    static bool press(AbstractUserInterface& ui, Event& event) {
        /** @todo if some other buttons are pressed and this is just one
            released, translate to a move event instead -- requires the
            applications to expose a way to query all currently pressed
            buttons */

        const Pointer pointer = pointerForButton<Event>(event.button());
        if(pointer == Pointer{})
            return false;

        PointerEvent e{pointer};
        if(ui.pointerPressEvent(Vector2{event.position()}, e)) {
            event.setAccepted();
            return true;
        }

        return false;
    }

    static bool release(AbstractUserInterface& ui, Event& event) {
        /** @todo if some other buttons are pressed and this is just one
            released, translate to a move event instead -- requires the
            applications to expose a way to query all currently pressed
            buttons */

        const Pointer pointer = pointerForButton<Event>(event.button());
        if(pointer == Pointer{})
            return false;

        PointerEvent e{pointer};
        if(ui.pointerReleaseEvent(Vector2{event.position()}, e)) {
            event.setAccepted();
            return true;
        }

        return false;
    }
};

template<class Event> struct PointerMoveEventConverter<Event, typename std::enable_if<sizeof(typename Event::Buttons)
    /* Without this, Clang (16, but probably others too) is only able to match
       this if MouseMoveEvent::Buttons is 8-bit. I suppose the boolean
       conversion in SFINAE contexts has some funny "optimization" that doesn't
       take the higher bits into account or some such. */
    #ifdef CORRADE_TARGET_CLANG
    >= 0
    #endif
>::type> {
    static bool move(AbstractUserInterface& ui, Event& event) {
        const Pointers pointers = pointersForButtons<Event>(event.buttons());

        PointerMoveEvent e{{}, pointers};
        if(ui.pointerMoveEvent(Vector2{event.position()}, e)) {
            event.setAccepted();
            return true;
        }

        return false;
    }
};

}}}
#endif

#endif
