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

/* Not all application classes have the same set of keys, but we want to handle
   even the ones that are available just in a subset. This SFINAE magic lets us
   pass through valid values or fall back to unique, hopefully unused values
   otherwise. A similar trick is used in AssimpImporter to skip aiTextureType
   values not available in earlier versions. Make sure to add an explicit test
   for each. */
#define WHEE_OPTIONAL_APPLICATION_KEY(name)                                 \
    template<class U> struct WheeOptionalApplicationKey_ ## name {          \
        template<class T> constexpr static U get(T*, decltype(T::name)* = nullptr) { \
            return T::name;                                                 \
        }                                                                   \
        constexpr static U get(...) {                                       \
            return U(1000000 + __LINE__);                                   \
        }                                                                   \
        constexpr static U Value = get(static_cast<U*>(nullptr));           \
    };

WHEE_OPTIONAL_APPLICATION_KEY(World1)
WHEE_OPTIONAL_APPLICATION_KEY(World2)
WHEE_OPTIONAL_APPLICATION_KEY(AltGr)

template<class Event> Key keyFor(typename Event::Key key) {
    switch(key) {
        /* LCOV_EXCL_START */
        #define _c(key) case Event::Key::key: return Key::key;
        _c(Backspace)
        _c(Tab)
        _c(Enter)
        _c(Esc)
        _c(Space)
        _c(Quote)
        _c(Comma)
        _c(Minus)
        _c(Period)
        _c(Slash)
        _c(Zero)
        _c(One)
        _c(Two)
        _c(Three)
        _c(Four)
        _c(Five)
        _c(Six)
        _c(Seven)
        _c(Eight)
        _c(Nine)
        _c(Semicolon)
        _c(Equal)
        _c(LeftBracket)
        _c(Backslash)
        _c(RightBracket)
        _c(Backquote)
        _c(A)
        _c(B)
        _c(C)
        _c(D)
        _c(E)
        _c(F)
        _c(G)
        _c(H)
        _c(I)
        _c(J)
        _c(K)
        _c(L)
        _c(M)
        _c(N)
        _c(O)
        _c(P)
        _c(Q)
        _c(R)
        _c(S)
        _c(T)
        _c(U)
        _c(V)
        _c(W)
        _c(X)
        _c(Y)
        _c(Z)
        _c(Delete)
        _c(LeftShift)
        _c(RightShift)
        _c(LeftCtrl)
        _c(RightCtrl)
        _c(LeftAlt)
        _c(RightAlt)
        _c(LeftSuper)
        _c(RightSuper)
        _c(Up)
        _c(Down)
        _c(Left)
        _c(Right)
        _c(Home)
        _c(End)
        _c(PageUp)
        _c(PageDown)
        _c(Insert)
        _c(F1)
        _c(F2)
        _c(F3)
        _c(F4)
        _c(F5)
        _c(F6)
        _c(F7)
        _c(F8)
        _c(F9)
        _c(F10)
        _c(F11)
        _c(F12)
        _c(CapsLock)
        _c(ScrollLock)
        _c(NumLock)
        _c(PrintScreen)
        _c(Pause)
        _c(Menu)
        _c(NumZero)
        _c(NumOne)
        _c(NumTwo)
        _c(NumThree)
        _c(NumFour)
        _c(NumFive)
        _c(NumSix)
        _c(NumSeven)
        _c(NumEight)
        _c(NumNine)
        _c(NumDecimal)
        _c(NumDivide)
        _c(NumMultiply)
        _c(NumSubtract)
        _c(NumAdd)
        _c(NumEnter)
        _c(NumEqual)
        _c(Percent)
        _c(Plus)
        #undef _c
        /* LCOV_EXCL_STOP */

        /* If the key is unknown, don't propagate the event at all */
        case Event::Key::Unknown:
            return {};

        /* Key values that may not be available in all applications. Needs
           SFINAE magic to produce unique, unreachable fallback values. A
           similar trick is used in AssimpImporter to skip aiTextureType values
           not available in earlier versions. Right now all those are skipped
           because it's unclear what they actually map to. Make sure to add an
           explicit test for each. */
        #define _s(key) case WheeOptionalApplicationKey_ ## key <typename Event::Key>::Value:
        #ifdef CORRADE_TARGET_GCC
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wswitch" /* case value not in enum */
        #elif defined(CORRADE_TARGET_MSVC)
        #pragma warning(push)
        #pragma warning(disable: 4063) /* not a valid value for switch */
        #endif
        _s(World1) /* GlfwApplication only, not sure what it maps to */
        _s(World2) /* GlfwApplication only, not sure what it maps to */
        _s(AltGr)  /* Sdl2Application only, the actual AltGr maps to RightAlt */
        #ifdef CORRADE_TARGET_GCC
        #pragma GCC diagnostic pop
        #elif defined(CORRADE_TARGET_MSVC)
        #pragma warning(pop)
        #endif
        #undef _s
            return {};
    }

    /* If the key is not recognized in the enum, treat it the same as Unknown,
       i.e. don't propagate the event at all */
    return {};
}

#undef WHEE_OPTIONAL_APPLICATION_KEY

template<class Event> Modifiers modifiersFor(typename Event::Modifiers modifiers) {
    Modifiers out;
    if(modifiers & Event::Modifier::Shift)
        out |= Modifier::Shift;
    if(modifiers & Event::Modifier::Ctrl)
        out |= Modifier::Ctrl;
    if(modifiers & Event::Modifier::Alt)
        out |= Modifier::Alt;
    if(modifiers & Event::Modifier::Super)
        out |= Modifier::Super;
    return out;
}

template<class Event> struct KeyEventConverter<Event, typename std::enable_if<sizeof(typename Event::Key)
    /* Without this, Clang (16, but probably others too) is only able to match
       this if KeyEvent::Key is 8-bit and not 32. I suppose the boolean
       conversion in SFINAE contexts has some funny "optimization" that doesn't
       take the higher bits into account or some such. Suspiciously enough, the
       exact same thing seems to be happening on MSVC, with SDL2 at least. */
    #if defined(CORRADE_TARGET_CLANG) || defined(CORRADE_TARGET_MSVC)
    >= 0
    #endif
>::type> {
    static bool press(AbstractUserInterface& ui, Event& event) {
        const Key key = keyFor<Event>(event.key());
        if(key == Key{})
            return false;

        KeyEvent e{key, modifiersFor<Event>(event.modifiers())};
        if(ui.keyPressEvent(e)) {
            event.setAccepted();
            return true;
        }

        return false;
    }

    static bool release(AbstractUserInterface& ui, Event& event) {
        const Key key = keyFor<Event>(event.key());
        if(key == Key{})
            return false;

        KeyEvent e{key, modifiersFor<Event>(event.modifiers())};
        if(ui.keyReleaseEvent(e)) {
            event.setAccepted();
            return true;
        }

        return false;
    }
};

template<class Event> struct TextInputEventConverter<Event,
    /* Unfortunately TextInputEvent doesn't have any nested type like other
       event classes, this doesn't work on MSVC and neither does
       sizeof(std::declval<Event>().text()), so for now I'm leaving this
       specialization unrestricted. Needs a real fix this once a different
       TextInputEventConverter needs to be added for an incompatible type. */
    #ifndef CORRADE_TARGET_MSVC
    typename std::enable_if<sizeof(&Event::text) >= 0>::type
    #else
    void
    #endif
> {
    static bool trigger(AbstractUserInterface& ui, Event& event) {
        TextInputEvent e{event.text()};
        if(ui.textInputEvent(e)) {
            event.setAccepted();
            return true;
        }

        return false;
    }
};

}}}
#endif

#endif
