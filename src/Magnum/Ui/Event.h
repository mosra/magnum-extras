#ifndef Magnum_Ui_Event_h
#define Magnum_Ui_Event_h
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
 * @brief Class @ref Magnum::Ui::PointerEvent, @ref Magnum::Ui::PointerMoveEvent, @ref Magnum::Ui::FocusEvent, @ref Magnum::Ui::KeyEvent, @ref Magnum::Ui::TextInputEvent, @ref Magnum::Ui::VisibilityLostEvent, enum @ref Magnum::Ui::Pointer, @ref Magnum::Ui::Key, @ref Magnum::Ui::Modifier, enum set @ref Magnum::Ui::Pointers, @ref Magnum::Ui::Modifiers
 * @m_since_latest
 */

#include <Corrade/Containers/StringView.h>
#include <Magnum/Math/Time.h>
#include <Magnum/Math/Vector2.h>

#include "Magnum/Ui/Ui.h"
#include "Magnum/Ui/visibility.h"

namespace Magnum { namespace Ui {

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
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, Pointer value);

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
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, Pointers value);

CORRADE_ENUMSET_OPERATORS(Pointers)

/**
@brief Pointer press or release event
@m_since_latest

@see @ref AbstractUserInterface::pointerPressEvent(),
    @ref AbstractUserInterface::pointerReleaseEvent(),
    @ref AbstractLayer::pointerPressEvent(),
    @ref AbstractLayer::pointerReleaseEvent(),
    @ref AbstractLayer::pointerTapOrClickEvent(), @ref FocusEvent
*/
class PointerEvent {
    public:
        /**
         * @brief Constructor
         * @param time      Time at which the event happened
         * @param type      Pointer type that got pressed or released
         *
         * The @p time may get used for UI animations. A default-constructed
         * value causes an animation play time to be in the past, thus
         * immediately transitioning to a stopped state. The position, capture
         * and hover properties are set from @ref AbstractUserInterface event
         * handler internals.
         */
        explicit PointerEvent(Nanoseconds time, Pointer type): _time{time}, _type{type} {}

        /** @brief Time at which the event happened */
        Nanoseconds time() const { return _time; }

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
         * @see @ref isHovering(), @ref isFocused()
         */
        bool isCaptured() const { return _captured; }

        /**
         * @brief Set whether to capture the event on a node
         *
         * By default, after a pointer press event, a node captures all
         * following pointer and key events until and including a pointer
         * release, even if they happen outside of the node area.
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
         * Returns @cpp true @ce if @ref AbstractUserInterface::currentHoveredNode()
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
         * @see @ref isCaptured(), @ref isFocused(), @ref setAccepted()
         */
        bool isHovering() const { return _hovering; }

        /**
         * @brief Whether the event is called on a node that's currently focused
         *
         * Returns @cpp true @ce if @ref AbstractUserInterface::currentFocusedNode()
         * is the same as the node the event is called on, @cpp false @ce
         * otherwise. Unlike @ref isHovering(), returns @cpp true @ce also if
         * the actual pointer position is outside of the area of the node the
         * event is called on, for example in case of an event capture.
         * @see @ref isCaptured()
         */
        bool isFocused() const { return _focused; }

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

        Nanoseconds _time;
        Vector2 _position;
        Pointer _type;
        bool _accepted = false;
        bool _captured = false;
        bool _hovering = false;
        bool _focused = false;
};

/**
@brief Pointer move event
@m_since_latest

@see @ref AbstractUserInterface::pointerMoveEvent(),
    @ref AbstractLayer::pointerMoveEvent(),
    @ref AbstractLayer::pointerEnterEvent(),
    @ref AbstractLayer::pointerLeaveEvent()
*/
class MAGNUM_UI_EXPORT PointerMoveEvent {
    public:
        /**
         * @brief Constructor
         * @param time      Time at which the event happened
         * @param type      Pointer type that changed in this event or
         *      @relativeref{Corrade,Containers::NullOpt}
         * @param types     Pointer types pressed in this event
         *
         * The @p time may get used for UI animations. A default-constructed
         * value causes an animation play time to be in the past, thus
         * immediately transitioning to a stopped state. The position, capture
         * and hover properties are set from @ref AbstractUserInterface event
         * handler internals.
         */
        explicit PointerMoveEvent(Nanoseconds time, Containers::Optional<Pointer> type, Pointers types);

        /**
         * @brief Constructor
         *
         * Meant to be used for testing purposes. The @p relativePosition gets
         * overwritten in @ref AbstractUserInterface event handler internals.
         */
        explicit PointerMoveEvent(Nanoseconds time, Containers::Optional<Pointer> type, Pointers types, const Vector2& relativePosition);

        /** @brief Time at which the event happened */
        Nanoseconds time() const { return _time; }

        /**
         * @brief Pointer type that changed in this event
         *
         * If no pointer changed in this event (i.e., all pointers that were
         * pressed before are still pressed), returns
         * @relativeref{Corrade,Containers::NullOpt}. Use @ref types() to check
         * what all pointers are pressed in this event. If @ref type() is not
         * empty and @ref types() contain @ref type(), it means given pointer
         * type was pressed, if they don't, it means it was released.
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
         * @see @ref isHovering(), @ref isFocused()
         */
        bool isCaptured() const { return _captured; }

        /**
         * @brief Set whether to capture the event on a node
         *
         * By default, after a pointer press event, a node captures all
         * following pointer and key events until and including a pointer
         * release, even if they happen outside of the node area. If capture is
         * disabled, the events are always sent to the actual node under the
         * pointer.
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
         * Returns @cpp true @ce if @ref AbstractUserInterface::currentHoveredNode()
         * is the same as the node the event is called on, @cpp false @ce
         * otherwise. In particular, is @cpp false @ce for the first move event
         * happening on a node, @cpp true @ce for the enter event and all
         * subsequent accepted move events on the same node, @cpp false @ce for
         * the leave event. On a captured move event returns @cpp false @ce for
         * if the pointer was moved outside of the node area.
         *
         * Note that even if this function returns @cpp true @ce, the event
         * handler still controls whether the node actually appears in
         * @ref AbstractUserInterface::currentHoveredNode() afterwards.
         * Accepting the event makes the node appear there. Not accepting it
         * makes the event potentially fall through to other nodes which may
         * then become hovered, if there are none then the hovered node becomes
         * null and subsequent move events called on this node will be called
         * with this function returning @cpp false @ce.
         * @see @ref isCaptured(), @ref isFocused(), @ref setAccepted()
         */
        bool isHovering() const { return _hovering; }

        /**
         * @brief Whether the event is called on a node that's currently focused
         *
         * Returns @cpp true @ce if @ref AbstractUserInterface::currentFocusedNode()
         * is the same as the node the event is called on, @cpp false @ce
         * otherwise. Unlike @ref isHovering(), returns @cpp true @ce also if
         * the actual pointer position is outside of the area of the node the
         * event is called on, for example in case of an event capture.
         * @see @ref isCaptured()
         */
        bool isFocused() const { return _focused; }

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

        Nanoseconds _time;
        Vector2 _position, _relativePosition;
        Pointer _type; /* NullOpt encoded as Pointer{} to avoid an include */
        Pointers _types;
        bool _accepted = false;
        bool _captured = false;
        bool _hovering = false;
        bool _focused = false;
};

/**
@brief Focus or blur event
@m_since_latest

@see @ref AbstractUserInterface::pointerPressEvent(),
    @ref AbstractUserInterface::focusEvent(), @ref AbstractLayer::focusEvent(),
    @ref AbstractLayer::blurEvent()
*/
class FocusEvent {
    public:
        /**
         * @brief Constructor
         * @param time      Time at which the event happened
         *
         * The @p time may get used for UI animations. A default-constructed
         * value causes an animation play time to be in the past, thus
         * immediately transitioning to a stopped state. The pressed and hover
         * properties are set from @ref AbstractUserInterface event handler
         * internals.
         */
        explicit FocusEvent(Nanoseconds time): _time{time} {}

        /** @brief Time at which the event happened */
        Nanoseconds time() const { return _time; }

        /**
         * @brief Whether the event is called on a node that's currently pressed
         *
         * Returns @cpp true @ce if @ref AbstractUserInterface::currentPressedNode()
         * is the same as the node the event is called on, @cpp false @ce
         * otherwise.
         * @see @ref isHovering(), @ref setAccepted()
         */
        bool isPressed() const { return _pressed; }

        /**
         * @brief Whether the event is called on a node that's currently hovered
         *
         * Returns @cpp true @ce if @ref AbstractUserInterface::currentHoveredNode()
         * is the same as the node the event is called on, @cpp false @ce
         * otherwise.
         * @see @ref isPressed(), @ref setAccepted()
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
         * The node receiving the event is treated as focused only if the event
         * is accepted.
         */
        void setAccepted(bool accepted = true) {
            _accepted = accepted;
        }

    private:
        friend AbstractUserInterface;

        Nanoseconds _time;
        bool _accepted = false;
        bool _pressed = false;
        bool _hovering = false;
};

/**
@brief Keyboard key
@m_since_latest

@see @ref KeyEvent
*/
enum class Key: UnsignedShort {
    /* Where possible, the key maps directly to the ASCII code of the character
       that would be printed. So '0' (48) for Zero, or for example 'a' (97) for
       A. Lowercase, not 'A' (65), because that one would get printed only with
       Shift pressed. As a special case, the Unknown key maps to '\0'.

       Range 128 to 255 is not used, keys not representable in the ASCII range
       have values 256 and up. Zero value is reserved for an unknown key. */

    /* 1 to 7 are not keys */
    Backspace = '\x08', /**< Backspace */
    Tab = '\t',         /**< Tab */
    Enter = '\n',       /**< Enter */
    /* 11 to 26 are not keys */
    Esc = '\x1b',       /**< Escape */
    /* 28 to 31 are not keys */
    Space = ' ',        /**< Space */
    /* 33 to 36, '!', '"', '#', '$' are not keys */

    /**
     * Percent. On the US keyboard layout this may only be representable as
     * @m_class{m-label m-warning} **Shift** @m_class{m-label m-default} **5**.
     */
    Percent = '%',

    /* 38, '&' is not a key */
    Quote = '\'',       /**< Quote (<tt>'</tt>) */
    /** @todo Sdl2Application reports 41 / SDLK_RIGHTPAREN for ')', GLFW
        reports 93 (RightBracket) instead */
    /* 40 to 42, '(', ')', '*' are not keys */

    /**
     * Plus. On the US keyboard layout this may only be representable as
     * @m_class{m-label m-warning} **Shift** @m_class{m-label m-default} **=**.
     */
    Plus = '+',

    Comma = ',',        /**< Comma */
    Minus = '-',        /**< Minus */
    Period = '.',       /**< Period */
    Slash = '/',        /**< Slash */
    Zero = '0',         /**< Zero */
    One = '1',          /**< One */
    Two = '2',          /**< Two */
    Three = '3',        /**< Three */
    Four = '4',         /**< Four */
    Five = '5',         /**< Five */
    Six = '6',          /**< Six */
    Seven = '7',        /**< Seven */
    Eight = '8',        /**< Eight */
    Nine = '9',         /**< Nine */
    /* 58, ':' is not a key */
    Semicolon = ';',    /**< Semicolon */
    /* 60, '<' is not a key */
    Equal = '=',        /**< Equal */
    /* 62 to 64, '>', '?', '@' are not keys */
    /* 65 to 90, (uppercase) 'A' to 'Z' are not keys */
    LeftBracket = '[',  /**< Left bracket (`[`) */
    Backslash = '\\',   /**< Backslash (`\`) */
    RightBracket = ']', /**< Right bracket (`]`) */
    /* 94 to 95, '^', '_' are not keys */
    Backquote = '`',    /**< Backquote (<tt>`</tt>) */
    A = 'a',            /**< Letter A */
    B = 'b',            /**< Letter B */
    C = 'c',            /**< Letter C */
    D = 'd',            /**< Letter D */
    E = 'e',            /**< Letter E */
    F = 'f',            /**< Letter F */
    G = 'g',            /**< Letter G */
    H = 'h',            /**< Letter H */
    I = 'i',            /**< Letter I */
    J = 'j',            /**< Letter J */
    K = 'k',            /**< Letter K */
    L = 'l',            /**< Letter L */
    M = 'm',            /**< Letter M */
    N = 'n',            /**< Letter N */
    O = 'o',            /**< Letter O */
    P = 'p',            /**< Letter P */
    Q = 'q',            /**< Letter Q */
    R = 'r',            /**< Letter R */
    S = 's',            /**< Letter S */
    T = 't',            /**< Letter T */
    U = 'u',            /**< Letter U */
    V = 'v',            /**< Letter V */
    W = 'w',            /**< Letter W */
    X = 'x',            /**< Letter X */
    Y = 'y',            /**< Letter Y */
    Z = 'z',            /**< Letter Z */
    /* 123 to 126, '{', '|', '}', '~' are not keys */
    Delete = '\x7f',    /* Delete */

    /* 128 to 255 unused */

    /**
     * Left Shift
     *
     * @see @ref Modifier::Shift
     */
    LeftShift = 256,

    /**
     * Right Shift
     *
     * @see @ref Modifier::Shift
     */
    RightShift,

    /**
     * Left Ctrl
     *
     * @see @ref Modifier::Ctrl
     */
    LeftCtrl,

    /**
     * Right Ctrl
     *
     * @see @ref Modifier::Ctrl
     */
    RightCtrl,

    /**
     * Left Alt
     *
     * @see @ref Modifier::Alt
     */
    LeftAlt,

    /**
     * Right Alt
     *
     * @see @ref Modifier::Alt
     */
    RightAlt,

    /**
     * Left Super key (Windows/⌘)
     *
     * @see @ref Modifier::Super
     */
    LeftSuper,

    /**
     * Right Super key (Windows/⌘)
     *
     * @see @ref Modifier::Super
     */
    RightSuper,

    /** @todo Sdl2Application has AltGr, but the actual AltGr triggers RightAlt
        instead, so can't really test */

    Up,                 /**< Up arrow */
    Down,               /**< Down arrow */
    Left,               /**< Left arrow */
    Right,              /**< Right arrow */
    Home,               /**< Home */
    End,                /**< End */
    PageUp,             /**< Page up */
    PageDown,           /**< Page down */
    Insert,             /**< Insert */

    F1,                 /**< F1 */
    F2,                 /**< F2 */
    F3,                 /**< F3 */
    F4,                 /**< F4 */
    F5,                 /**< F5 */
    F6,                 /**< F6 */
    F7,                 /**< F7 */
    F8,                 /**< F8 */
    F9,                 /**< F9 */
    F10,                /**< F10 */
    F11,                /**< F11 */
    F12,                /**< F12 */

    /** @todo GlfwApplication has World1 / World2 but there's no clear
        consensus on what they should map to (World1 can be the backslash next
        to left Shift, but also §: https://github.com/glfw/glfw/issues/2481),
        so I don't know how to name them */

    CapsLock,           /**< Caps lock */
    ScrollLock,         /**< Scroll lock */
    NumLock,            /**< Num lock */
    PrintScreen,        /**< Print screen */
    Pause,              /**< Pause */
    Menu,               /**< Menu */

    NumZero,            /**< Numpad zero */
    NumOne,             /**< Numpad one */
    NumTwo,             /**< Numpad two */
    NumThree,           /**< Numpad three */
    NumFour,            /**< Numpad four */
    NumFive,            /**< Numpad five */
    NumSix,             /**< Numpad six */
    NumSeven,           /**< Numpad seven */
    NumEight,           /**< Numpad eight */
    NumNine,            /**< Numpad nine */
    NumDecimal,         /**< Numpad decimal */
    NumDivide,          /**< Numpad divide */
    NumMultiply,        /**< Numpad multiply */
    NumSubtract,        /**< Numpad subtract */
    NumAdd,             /**< Numpad add */
    NumEnter,           /**< Numpad enter */
    NumEqual            /**< Numpad equal */
};

/**
@debugoperatorenum{Key}
@m_since_latest
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, Key value);

/**
@brief Keyboard modifier
@m_since_latest

@see @ref Modifiers, @ref KeyEvent, @ref Key
*/
enum class Modifier: UnsignedByte {
    /**
     * Shift
     *
     * @see @ref Key::LeftShift, @ref Key::RightShift
     */
    Shift = 1 << 0,

    /**
     * Ctrl
     *
     * @see @ref Key::LeftCtrl, @ref Key::RightCtrl
     */
    Ctrl = 1 << 1,

    /**
     * Alt
     *
     * @see @ref Key::LeftAlt, @ref Key::RightAlt
     */
    Alt = 1 << 2,

    /**
     * Super key (Windows/⌘)
     *
     * @see @ref Key::LeftSuper, @ref Key::RightSuper
     */
    Super = 1 << 3
};

/**
@debugoperatorenum{Modifier}
@m_since_latest
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, Modifier value);

/**
@brief Set of keyboard modifiers
@m_since_latest

@see @ref KeyEvent
*/
typedef Containers::EnumSet<Modifier> Modifiers;

/**
@debugoperatorenum{Modifiers}
@m_since_latest
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, Modifiers value);

CORRADE_ENUMSET_OPERATORS(Modifiers)

/**
@brief Key press or release event
@m_since_latest

@see @ref AbstractUserInterface::keyPressEvent(),
    @ref AbstractUserInterface::keyReleaseEvent(),
    @ref AbstractLayer::keyPressEvent(),
    @ref AbstractLayer::keyReleaseEvent()
*/
class MAGNUM_UI_EXPORT KeyEvent {
    public:
        /**
         * @brief Constructor
         * @param time          Time at which the event happened
         * @param key           Key that got pressed or released
         * @param modifiers     Active keyboard modifiers
         *
         * The @p time may get used for UI animations. A default-constructed
         * value causes an animation play time to be in the past, thus
         * immediately transitioning to a stopped state. The position, capture
         * and hover properties are set from @ref AbstractUserInterface event
         * handler internals.
         */
        explicit KeyEvent(Nanoseconds time, Key key, Modifiers modifiers): _time{time}, _key{key}, _modifiers{modifiers} {}

        /** @brief Time at which the event happened */
        Nanoseconds time() const { return _time; }

        /** @brief Key that got pressed or released */
        Key key() const { return _key; }

        /** @brief Active keyboard modifiers */
        Modifiers modifiers() const { return _modifiers; }

        /**
         * @brief Event position
         *
         * If the event was called on a
         * @ref AbstractUserInterface::currentFocusedNode(), returns
         * @relativeref{Corrade,Containers::NullOpt}. Otherwise the node was
         * picked based on pointer position from a preceding pointer press,
         * release or move event, and the function returns a position relative
         * to that node.
         * @see @ref isFocused()
         */
        Containers::Optional<Vector2> position() const;

        /**
         * @brief Whether the event is captured on a node
         *
         * If the event is called on a
         * @ref AbstractUserInterface::currentFocusedNode(), returns
         * @cpp false @ce. Otherwise returns @cpp true @ce if
         * @ref AbstractUserInterface::currentCapturedNode() is the same as the
         * node the event is called on, @cpp false @ce otherwise. Unlike
         * @ref PointerEvent or @ref PointerMoveEvent, key events don't have a
         * possibility to modify the captured status.
         * @see @ref isFocused(), @ref isHovering()
         */
        bool isCaptured() const { return _captured; }

        /**
         * @brief Whether the event is called on a node that's currently hovered
         *
         * Returns @cpp true @ce if @ref AbstractUserInterface::currentHoveredNode()
         * is the same as the node the event is called on, @cpp false @ce
         * otherwise.
         * @see @ref isCaptured(), @ref isFocused()
         */
        bool isHovering() const { return _hovering; }

        /**
         * @brief Whether the event is called on a node that's currently focused
         *
         * Returns @cpp true @ce if @ref AbstractUserInterface::currentFocusedNode()
         * is the same as the node the event is called on, @cpp false @ce
         * otherwise. Unlike @ref isHovering(), returns @cpp true @ce also if
         * the actual pointer position is outside of the area of the node the
         * event is called on, for example in case of an event capture.
         * @see @ref isCaptured(), @ref isHovering()
         */
        bool isFocused() const { return _focused; }

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

        Nanoseconds _time;
        /* NullOpt encoded as NaNs to avoid an include */
        Vector2 _position{Constants::nan()};
        Key _key;
        Modifiers _modifiers;
        bool _accepted = false;
        bool _captured = false;
        bool _hovering = false;
        bool _focused = false;
};

/**
@brief Text input event
@m_since_latest

@see @ref AbstractUserInterface::textInputEvent(),
    @ref AbstractLayer::textInputEvent()
*/
class TextInputEvent {
    public:
        /**
         * @brief Constructor
         * @param time      Time at which the event happened
         * @param text      Text produced by the event
         *
         * The @p time may get used for UI animations. A default-constructed
         * value causes an animation play time to be in the past, thus
         * immediately transitioning to a stopped state. Expects that @p text
         * is valid for the whole lifetime of the text input event.
         */
        explicit TextInputEvent(Nanoseconds time, Containers::StringView text): _time{time}, _text{text} {}

        /** @brief Time at which the event happened */
        Nanoseconds time() const { return _time; }

        /** @brief Input text */
        Containers::StringView text() const { return _text; }

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

        Nanoseconds _time;
        Containers::StringView _text;
        bool _accepted = false;
};

/**
@brief Visibility lost event
@m_since_latest

Unlike all other events, this event is fired from
@ref AbstractUserInterface::update() and is without any relation to events
coming from the application. As such it also doesn't carry a timestamp, thus
visual changes done in response to this event don't animate.
@see @ref AbstractLayer::visibilityLostEvent()
*/
class VisibilityLostEvent {
    public:
        /**
         * @brief Constructor
         *
         * The pressed and hover properties are set from
         * @ref AbstractUserInterface event handler internals.
         */
        explicit VisibilityLostEvent() = default;

        /**
         * @brief Whether the event is called on a node that's currently pressed
         *
         * Returns @cpp true @ce only if the event was called in response to
         * @ref AbstractUserInterface::currentFocusedNode() no longer being
         * @ref NodeFlag::Focusable and
         * @ref AbstractUserInterface::currentPressedNode() is the same as the
         * node the event is called on. In all other cases (node becoming
         * invisible, @ref NodeFlag::Disabled or @ref NodeFlag::NoEvents)
         * returns @cpp false @ce.
         * @see @ref isHovering()
         */
        bool isPressed() const { return _pressed; }

        /**
         * @brief Whether the event is called on a node that's currently hovered
         *
         * Returns @cpp true @ce only if the event was called in response to
         * @ref AbstractUserInterface::currentFocusedNode() no longer being
         * @ref NodeFlag::Focusable and
         * @ref AbstractUserInterface::currentHoveredNode() is the same as the
         * node the event is called on. In all other cases (node becoming
         * invisible, @ref NodeFlag::Disabled or @ref NodeFlag::NoEvents)
         * returns @cpp false @ce.
         * @see @ref isPressed()
         */
        bool isHovering() const { return _hovering; }

    private:
        friend AbstractUserInterface;

        bool _pressed = false;
        bool _hovering = false;
};

}}

#endif
