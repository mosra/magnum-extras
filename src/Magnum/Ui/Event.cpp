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

#include "Event.h"

#include <Corrade/Containers/EnumSet.hpp>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Utility/Debug.h>

namespace Magnum { namespace Ui {

Debug& operator<<(Debug& debug, const Pointer value) {
    debug << "Ui::Pointer" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case Pointer::value: return debug << "::" #value;
        _c(MouseLeft)
        _c(MouseMiddle)
        _c(MouseRight)
        _c(Finger)
        _c(Pen)
        _c(Eraser)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "(" << Debug::nospace << Debug::hex << UnsignedByte(value) << Debug::nospace << ")";
}

Debug& operator<<(Debug& debug, const Pointers value) {
    return Containers::enumSetDebugOutput(debug, value, "Ui::Pointers{}", {
        Pointer::MouseLeft,
        Pointer::MouseMiddle,
        Pointer::MouseRight,
        Pointer::Finger,
        Pointer::Pen,
        Pointer::Eraser
    });
}

PointerMoveEvent::PointerMoveEvent(const Nanoseconds time, const Containers::Optional<Pointer> type, const Pointers types): PointerMoveEvent{time, type, types, {}} {}

PointerMoveEvent::PointerMoveEvent(const Nanoseconds time, const Containers::Optional<Pointer> type, const Pointers types, const Vector2& relativePosition): _time{time}, _relativePosition{relativePosition}, _type{type ? *type : Pointer{}}, _types{types} {}

Containers::Optional<Pointer> PointerMoveEvent::type() const {
    return _type == Pointer{} ? Containers::NullOpt : Containers::optional(_type);
}

Debug& operator<<(Debug& debug, const Key value) {
    debug << "Ui::Key" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case Key::value: return debug << "::" #value;
        _c(Backspace)
        _c(Tab)
        _c(Enter)
        _c(Esc)
        _c(Space)
        _c(Percent)
        _c(Quote)
        _c(Comma)
        _c(Minus)
        _c(Plus)
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
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "(" << Debug::nospace << Debug::hex << UnsignedShort(value) << Debug::nospace << ")";
}

Debug& operator<<(Debug& debug, const Modifier value) {
    debug << "Ui::Modifier" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case Modifier::value: return debug << "::" #value;
        _c(Shift)
        _c(Ctrl)
        _c(Alt)
        _c(Super)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "(" << Debug::nospace << Debug::hex << UnsignedByte(value) << Debug::nospace << ")";
}

Debug& operator<<(Debug& debug, const Modifiers value) {
    return Containers::enumSetDebugOutput(debug, value, "Ui::Modifiers{}", {
        Modifier::Shift,
        Modifier::Ctrl,
        Modifier::Alt,
        Modifier::Super
    });
}

Containers::Optional<Vector2> KeyEvent::position() const {
    /* It should be enough to test just one component for NaN */
    return _position.x() != _position.x() ? Containers::NullOpt :
        Containers::optional(_position);
}

}}
