#ifndef Magnum_Ui_Application_h
#define Magnum_Ui_Application_h
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
@brief @ref Magnum::Platform::Sdl2Application "Magnum::Platform::*Application" compatibility for @ref Magnum::Ui::AbstractUserInterface
@m_since_latest

Including this header allows you to pass a @ref Magnum::Platform::Sdl2Application "Magnum::Platform::*Application" to
@ref Magnum::Ui::AbstractUserInterface constructors and
@relativeref{Magnum::Ui::AbstractUserInterface,setSize()} to query properties
from and to pass @ref Magnum::Platform::Sdl2Application::PointerEvent "Magnum::Platform::*Application::PointerEvent",
@relativeref{Magnum::Platform::Sdl2Application,PointerMoveEvent},
@relativeref{Magnum::Platform::Sdl2Application,ScrollEvent},
@relativeref{Magnum::Platform::Sdl2Application,KeyEvent} and
@relativeref{Magnum::Platform::Sdl2Application,TextInputEvent} to
@ref Magnum::Ui::AbstractUserInterface::pointerPressEvent(),
@relativeref{Magnum::Ui::AbstractUserInterface,pointerReleaseEvent()},
@relativeref{Magnum::Ui::AbstractUserInterface,pointerMoveEvent()},
@relativeref{Magnum::Ui::AbstractUserInterface,scrollEvent()},
@relativeref{Magnum::Ui::AbstractUserInterface,keyPressEvent()},
@relativeref{Magnum::Ui::AbstractUserInterface,keyReleaseEvent()} and
@relativeref{Magnum::Ui::AbstractUserInterface,textInputEvent()}. See
@ref Ui-AbstractUserInterface-application for more information.
*/

#include <Corrade/Containers/Optional.h>

#include "Magnum/Ui/AbstractUserInterface.h"
#include "Magnum/Ui/Event.h"

/* Listing these namespaces doesn't add anything to the docs, so don't */
#ifndef DOXYGEN_GENERATING_OUTPUT
namespace Magnum { namespace Ui { namespace Implementation {

/* This works with both the Application itself as well as the ViewportEvent */
template<class ApplicationOrEvent> struct ApplicationSizeConverter<ApplicationOrEvent,
    /* Clang (16, but probably others too) is only able to match this with the
       >=. Without it, it can only match if the size is 1, which is never the
       case for function pointers. I suppose the boolean conversion in SFINAE
       contexts has some funny "optimization" that doesn't take the higher bits
       into account or some such. */
    #ifdef CORRADE_TARGET_CLANG
    typename std::enable_if<sizeof(&ApplicationOrEvent::framebufferSize) >= 0>::type
    /* MSVC cannot match the above and Application doesn't have any nested type
       related to framebuffer sizes, and matching on some unrelated typedef
       doesn't feel any better than nothing at all even though that works on
       older MSVC. Fortunately this seems to be fixed in MSVC 2022 17.10+. So
       far we don't need to avoid any conflict here so just enable it always. */
    #elif defined(CORRADE_TARGET_MSVC) && _MSC_VER < 1940
    void
    /* GCC is fine */
    #else
    typename std::enable_if<sizeof(&ApplicationOrEvent::framebufferSize)>::type
    #endif
> {
    static void set(AbstractUserInterface& ui, const ApplicationOrEvent& applicationOrEvent) {
        ui.setSize(Vector2{applicationOrEvent.windowSize()}/applicationOrEvent.dpiScaling(), Vector2{applicationOrEvent.windowSize()}, applicationOrEvent.framebufferSize());
    }
};

/* Not all applications have Finger / Pen pointers or a Touch / Pen pointer
   event source, so employing a dirty SFINAE trick with an overload that
   returns false if given enum value is not available. A similar trick is
   used in ImGuiIntegration. */

#define MAGNUM_UI_OPTIONAL_POINTER_EVENT_SOURCE(source)                     \
    template<class ApplicationPointerEventSource> constexpr bool is##source##PointerEventSource(ApplicationPointerEventSource p, decltype(ApplicationPointerEventSource::source)* = nullptr) { \
        return p == ApplicationPointerEventSource::source;                  \
    }                                                                       \
    constexpr bool is##source##PointerEventSource(...) {                    \
        return false;                                                       \
    }
MAGNUM_UI_OPTIONAL_POINTER_EVENT_SOURCE(Touch)
MAGNUM_UI_OPTIONAL_POINTER_EVENT_SOURCE(Pen)
#undef MAGNUM_UI_OPTIONAL_POINTER_EVENT_SOURCE

#define MAGNUM_UI_OPTIONAL_POINTER(pointer)                                 \
    template<class ApplicationPointer> constexpr bool is##pointer##Pointer(ApplicationPointer p, decltype(ApplicationPointer::pointer)* = nullptr) { \
        return p == ApplicationPointer::pointer;                            \
    }                                                                       \
    template<class ApplicationPointers> constexpr bool has##pointer##Pointer(ApplicationPointers p, decltype(ApplicationPointers::Type::pointer)* = nullptr) { \
        return p >= ApplicationPointers::Type::pointer;                     \
    }                                                                       \
    constexpr bool is##pointer##Pointer(...) {                              \
        return false;                                                       \
    }                                                                       \
    constexpr bool has##pointer##Pointer(...) {                             \
        return false;                                                       \
    }
MAGNUM_UI_OPTIONAL_POINTER(Finger)
MAGNUM_UI_OPTIONAL_POINTER(Pen)
MAGNUM_UI_OPTIONAL_POINTER(Eraser)
#undef MAGNUM_UI_OPTIONAL_POINTER

template<class ApplicationPointerEventSource> PointerEventSource pointerEventSourceFor(const ApplicationPointerEventSource source) {
    /* Not a switch because this makes it easier to check for the not always
       available pointer event source kinds */
    if(source == ApplicationPointerEventSource::Mouse)
        return PointerEventSource::Mouse;
    if(isTouchPointerEventSource(source))
        return PointerEventSource::Touch;
    if(isPenPointerEventSource(source))
        return PointerEventSource::Pen;
    return {};
}

template<class ApplicationPointer> Pointer pointerFor(const ApplicationPointer pointer) {
    /* Not a switch because this makes it easier to check for the not always
       available pointer kinds */
    if(pointer == ApplicationPointer::MouseLeft)
        return Pointer::MouseLeft;
    if(pointer == ApplicationPointer::MouseMiddle)
        return Pointer::MouseMiddle;
    if(pointer == ApplicationPointer::MouseRight)
        return Pointer::MouseRight;
    if(isFingerPointer(pointer))
        return Pointer::Finger;
    if(isPenPointer(pointer))
        return Pointer::Pen;
    if(isEraserPointer(pointer))
        return Pointer::Eraser;
    return {};
}

template<class ApplicationPointers> Pointers pointersFor(const ApplicationPointers pointers) {
    typedef typename ApplicationPointers::Type ApplicationPointer;

    Pointers out;
    if(pointers & ApplicationPointer::MouseLeft)
        out |= Pointer::MouseLeft;
    if(pointers & ApplicationPointer::MouseMiddle)
        out |= Pointer::MouseMiddle;
    if(pointers & ApplicationPointer::MouseRight)
        out |= Pointer::MouseRight;
    if(hasFingerPointer(pointers))
        out |= Pointer::Finger;
    if(hasPenPointer(pointers))
        out |= Pointer::Pen;
    if(hasEraserPointer(pointers))
        out |= Pointer::Eraser;
    return out;
}

template<class ApplicationModifiers> Modifiers modifiersFor(ApplicationModifiers modifiers) {
    typedef typename ApplicationModifiers::Type ApplicationModifier;

    Modifiers out;
    if(modifiers & ApplicationModifier::Shift)
        out |= Modifier::Shift;
    if(modifiers & ApplicationModifier::Ctrl)
        out |= Modifier::Ctrl;
    if(modifiers & ApplicationModifier::Alt)
        out |= Modifier::Alt;
    if(modifiers & ApplicationModifier::Super)
        out |= Modifier::Super;
    return out;
}

/** @todo remove once the deprecated MouseEvent is gone */
#if defined(CORRADE_TARGET_MSVC) && !defined(CORRADE_TARGET_CLANG) && _MSC_VER < 1940
CORRADE_HAS_TYPE(IsPointerEvent, decltype(std::declval<T>().pointer()));
CORRADE_HAS_TYPE(IsPointerMoveEvent, decltype(std::declval<T>().pointers()));
#endif

template<class Event> struct PointerEventConverter<Event, typename std::enable_if<
    /* Clang (16, but probably others too) is only able to match this with the
       >=. Without it, it can only match if the size is 1, which was only the
       case with the now-deprecated MouseEvent::Button. I suppose the boolean
       conversion in SFINAE contexts has some funny "optimization" that doesn't
       take the higher bits into account or some such. */
    #ifdef CORRADE_TARGET_CLANG
    sizeof(&Event::pointer) >= 0
    /* MSVC cannot match the above and PointerEvent doesn't have any nested
       type, only the now-deprecated MouseEvent::Button is what works on older
       MSVC. Fortunately this seems to be fixed in MSVC 2022 17.10+. Would do
       the same as in TextInputEventConverter but this needs to not conflict
       with the PointerEventConverter for MouseEvent below. */
    /** @todo replace with just void once the deprecated MouseEvent is gone */
    #elif defined(CORRADE_TARGET_MSVC) && _MSC_VER < 1940
    IsPointerEvent<Event>::value
    /* GCC is fine */
    #else
    sizeof(&Event::pointer)
    #endif
>::type> {
    static bool press(AbstractUserInterface& ui, Event& event, const Nanoseconds time = {}) {
        const PointerEventSource source = pointerEventSourceFor(event.source());
        const Pointer pointer = pointerFor(event.pointer());
        if(source == PointerEventSource{} || pointer == Pointer{})
            return false;

        PointerEvent e{time, source, pointer, event.isPrimary(), event.id(), modifiersFor(event.modifiers())};
        if(ui.pointerPressEvent(event.position(), e)) {
            event.setAccepted();
            return true;
        }

        return false;
    }

    static bool release(AbstractUserInterface& ui, Event& event, const Nanoseconds time = {}) {
        const PointerEventSource source = pointerEventSourceFor(event.source());
        const Pointer pointer = pointerFor(event.pointer());
        if(source == PointerEventSource{} || pointer == Pointer{})
            return false;

        PointerEvent e{time, source, pointer, event.isPrimary(), event.id(), modifiersFor(event.modifiers())};
        if(ui.pointerReleaseEvent(event.position(), e)) {
            event.setAccepted();
            return true;
        }

        return false;
    }
};

template<class Event> struct ScrollEventConverter<Event,
    /* Clang (16, but probably others too) is only able to match this with the
       >=. Without it, it can only match if the size is 1, which was only the
       case with the now-deprecated MouseEvent::Button. I suppose the boolean
       conversion in SFINAE contexts has some funny "optimization" that doesn't
       take the higher bits into account or some such. */
    #ifdef CORRADE_TARGET_CLANG
    typename std::enable_if<sizeof(&Event::offset) >= 0>::type
    /* MSVC cannot match the above and ScrollEvent doesn't have any nested
       type, only the now-deprecated MouseEvent::Button is what works on older
       MSVC. Fortunately this seems to be fixed in MSVC 2022 17.10+. Compared
       to PointerEvent we don't need to avoid any conflict here so just enable
       it always. */
    #elif defined(CORRADE_TARGET_MSVC) && _MSC_VER < 1940
    void
    /* GCC is fine */
    #else
    typename std::enable_if<sizeof(&Event::offset)>::type
    #endif
> {
    static bool trigger(AbstractUserInterface& ui, Event& event, const Nanoseconds time = {}) {
        ScrollEvent e{time, event.offset(), modifiersFor(event.modifiers())};
        if(ui.scrollEvent(
            /* This converter can be called with the deprecated
               MouseScrollEvent as well, as there's no easy way to distinguish
               them. The only difference between the two is that the position
               is an integer vector in the deprecated one. Add a cast to
               Vector2 to make it work with both. */
            /** @todo remove the case once the deprecated MouseScrollEvent is
                gone */
            #ifdef MAGNUM_BUILD_DEPRECATED
            Vector2{event.position()}
            #else
            event.position()
            #endif
        , e)) {
            event.setAccepted();
            return true;
        }

        return false;
    }
};

template<class Event> struct PointerMoveEventConverter<Event, typename std::enable_if<
    /* Clang (16, but probably others too) is only able to match this with the
       >=. Without it, it can only match if the size is 1, which was only the
       case with the now-deprecated MouseEvent::Button. I suppose the boolean
       conversion in SFINAE contexts has some funny "optimization" that doesn't
       take the higher bits into account or some such. */
    #ifdef CORRADE_TARGET_CLANG
    sizeof(&Event::pointers) >= 0
    /* MSVC cannot match the above and PointerMoveEvent doesn't have any nested
       type, only the now-deprecated MouseEvent::Button is what works on older
       MSVC. Fortunately this seems to be fixed in MSVC 2022 17.10+. Would do
       the same as in TextInputEventConverter but this needs to not conflict
       with the PointerMoveEventConverter for MouseMoveEvent below. */
    /** @todo replace with just void once the deprecated MouseEvent is gone */
    #elif defined(CORRADE_TARGET_MSVC) && _MSC_VER < 1940
    IsPointerMoveEvent<Event>::value
    /* GCC is fine */
    #else
    sizeof(&Event::pointers)
    #endif
>::type> {
    static bool move(AbstractUserInterface& ui, Event& event, const Nanoseconds time = {}) {
        const PointerEventSource source = pointerEventSourceFor(event.source());
        const Pointers pointers = pointersFor(event.pointers());
        if(source == PointerEventSource{})
            return false;

        /* If pressed/released pointer translation fails (zero value returned),
           propagate this as just a plain move */
        Containers::Optional<Pointer> pointer;
        if(event.pointer()) {
            const Pointer translated = pointerFor(*event.pointer());
            if(translated != Pointer{})
                pointer = translated;
        }

        PointerMoveEvent e{time, source, pointer, pointers, event.isPrimary(), event.id(), modifiersFor(event.modifiers())};
        if(ui.pointerMoveEvent(event.position(), e)) {
            event.setAccepted();
            return true;
        }

        return false;
    }
};

/* These handle the deprecated MouseEvent and MouseMoveEvent classes */
/** @todo remove once they're gone */
#ifdef MAGNUM_BUILD_DEPRECATED
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
    static bool press(AbstractUserInterface& ui, Event& event, const Nanoseconds time = {}) {
        const Pointer pointer = pointerForButton<Event>(event.button());
        if(pointer == Pointer{})
            return false;

        PointerEvent e{time, PointerEventSource::Mouse, pointer, true, 0, modifiersFor(event.modifiers())};
        if(ui.pointerPressEvent(Vector2{event.position()}, e)) {
            event.setAccepted();
            return true;
        }

        return false;
    }

    static bool release(AbstractUserInterface& ui, Event& event, const Nanoseconds time = {}) {
        const Pointer pointer = pointerForButton<Event>(event.button());
        if(pointer == Pointer{})
            return false;

        PointerEvent e{time, PointerEventSource::Mouse, pointer, true, 0, modifiersFor(event.modifiers())};
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
    static bool move(AbstractUserInterface& ui, Event& event, const Nanoseconds time = {}) {
        const Pointers pointers = pointersForButtons<Event>(event.buttons());

        PointerMoveEvent e{time, PointerEventSource::Mouse, {}, pointers, true, 0, modifiersFor(event.modifiers())};
        if(ui.pointerMoveEvent(Vector2{event.position()}, e)) {
            event.setAccepted();
            return true;
        }

        return false;
    }
};
#endif

/* Not all application classes have the same set of keys, but we want to handle
   even the ones that are available just in a subset. This SFINAE magic lets us
   pass through valid values or fall back to unique, hopefully unused values
   otherwise. A similar trick is used in AssimpImporter to skip aiTextureType
   values not available in earlier versions. Make sure to add an explicit test
   for each. */
#define UI_OPTIONAL_APPLICATION_KEY(name)                                   \
    template<class U> struct UiOptionalApplicationKey_ ## name {            \
        template<class T> constexpr static U get(T*, decltype(T::name)* = nullptr) { \
            return T::name;                                                 \
        }                                                                   \
        constexpr static U get(...) {                                       \
            return U(1000000 + __LINE__);                                   \
        }                                                                   \
        constexpr static U Value = get(static_cast<U*>(nullptr));           \
    };

UI_OPTIONAL_APPLICATION_KEY(World1)
UI_OPTIONAL_APPLICATION_KEY(World2)
UI_OPTIONAL_APPLICATION_KEY(AltGr)

template<class ApplicationKey> Key keyFor(ApplicationKey key) {
    switch(key) {
        /* LCOV_EXCL_START */
        #define _c(key) case ApplicationKey::key: return Key::key;
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
        case ApplicationKey::Unknown:
            return {};

        /* Key values that may not be available in all applications. Needs
           SFINAE magic to produce unique, unreachable fallback values. A
           similar trick is used in AssimpImporter to skip aiTextureType values
           not available in earlier versions. Right now all those are skipped
           because it's unclear what they actually map to. Make sure to add an
           explicit test for each. */
        #define _s(key) case UiOptionalApplicationKey_ ## key <ApplicationKey>::Value:
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

#undef UI_OPTIONAL_APPLICATION_KEY

template<class Event> struct KeyEventConverter<Event,
    /* Clang (16, but probably others too) is only able to match this with the
       >=. Without it, it can only match if the size is 1, which was only the
       case with the now-deprecated MouseEvent::Button. I suppose the boolean
       conversion in SFINAE contexts has some funny "optimization" that doesn't
       take the higher bits into account or some such. */
    #ifdef CORRADE_TARGET_CLANG
    typename std::enable_if<sizeof(&Event::key) >= 0>::type
    /* MSVC cannot match the above and KeyEvent doesn't have any nested type,
       only the now-deprecated Key is what works on older MSVC. Fortunately
       this seems to be fixed in MSVC 2022 17.10+. Compared to PointerEvent we
       don't need to avoid any conflict here so just enable it always. */
    #elif defined(CORRADE_TARGET_MSVC) && _MSC_VER < 1940
    void
    /* GCC is fine */
    #else
    typename std::enable_if<sizeof(&Event::key)>::type
    #endif
> {
    static bool press(AbstractUserInterface& ui, Event& event, const Nanoseconds time = {}) {
        const Key key = keyFor(event.key());
        if(key == Key{})
            return false;

        KeyEvent e{time, key, modifiersFor(event.modifiers())};
        if(ui.keyPressEvent(e)) {
            event.setAccepted();
            return true;
        }

        return false;
    }

    static bool release(AbstractUserInterface& ui, Event& event, const Nanoseconds time = {}) {
        const Key key = keyFor(event.key());
        if(key == Key{})
            return false;

        KeyEvent e{time, key, modifiersFor(event.modifiers())};
        if(ui.keyReleaseEvent(e)) {
            event.setAccepted();
            return true;
        }

        return false;
    }
};

template<class Event> struct TextInputEventConverter<Event,
    /* Clang (16, but probably others too) is only able to match this with the
       >=. Without it, it can only match if the size is 1, which was only the
       case with the now-deprecated MouseEvent::Button. I suppose the boolean
       conversion in SFINAE contexts has some funny "optimization" that doesn't
       take the higher bits into account or some such. */
    #ifdef CORRADE_TARGET_CLANG
    typename std::enable_if<sizeof(&Event::text) >= 0>::type
    /* MSVC cannot match the above and TextInputEvent doesn't have any nested
       type. Fortunately this seems to be fixed in MSVC 2022 17.10+. Compared
       to PointerEvent we don't need to avoid any conflict here so just enable
       it always. */
    #elif defined(CORRADE_TARGET_MSVC) && _MSC_VER < 1940
    void
    /* GCC is fine */
    #else
    typename std::enable_if<sizeof(&Event::text)>::type
    #endif
> {
    static bool trigger(AbstractUserInterface& ui, Event& event, const Nanoseconds time = {}) {
        TextInputEvent e{time, event.text()};
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
