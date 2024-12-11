#ifndef Magnum_Ui_EventLayer_h
#define Magnum_Ui_EventLayer_h
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
 * @brief Class @ref Magnum::Ui::EventLayer, @ref Magnum::Ui::EventConnection
 * @m_since_latest
 */

#include <Corrade/Containers/Reference.h>

#include "Magnum/Ui/AbstractLayer.h"

namespace Magnum { namespace Ui {

/**
@brief Connection in the @ref EventLayer
@m_since_latest

Performs automatic removal of a connection on destruction. Each instance with
non-null @ref data() counts towards @ref EventLayer::usedScopedConnectionCount().
*/
class MAGNUM_UI_EXPORT EventConnection {
    public:
        /** @brief Copying is not allowed */
        EventConnection(const EventConnection&) = delete;

        /** @brief Move constructor */
        EventConnection(EventConnection&& other) noexcept: _layer{other._layer}, _data{other._data} {
            /* Not using LayerDataHandle::Null to avoid including Handle.h;
               OTOH keeping the constructor inlined for debug perf */
            other._data = LayerDataHandle{};
        }

        /**
         * @brief Destructor
         *
         * If @ref data() is not @ref DataHandle::Null and is valid, removes
         * the connection from associated @ref layer().
         */
        /* MinGW needs inline here to not say "redeclared without dllimport" */
        inline ~EventConnection();

        /** @brief Copying is not allowed */
        EventConnection& operator=(const EventConnection&) = delete;

        /** @brief Move assignment */
        EventConnection& operator=(EventConnection&& other) noexcept {
            Utility::swap(other._layer, _layer);
            Utility::swap(other._data, _data);
            return *this;
        }

        /**
         * @brief Event layer containing the connection
         *
         * If @ref data() is @ref DataHandle::Null, the reference may be
         * dangling.
         */
        EventLayer& layer() { return _layer; }
        const EventLayer& layer() const { return _layer; } /**< @overload */

        /**
         * @brief Connection data handle
         *
         * If @ref DataHandle::Null, the instance is moved out or released.
         */
        DataHandle data() const;

        /**
         * @brief Release the connection
         *
         * Returns @ref data() and sets it to @ref DataHandle::Null so it's no
         * longer removed at destruction. Removing the connection using
         * @ref EventLayer::remove() before the layer itself is destructed is
         * user responsibility.
         */
        DataHandle release();

    private:
        friend EventLayer;

        /* LayerDataHandle is the lower 32 bits of DataHandle. Not using
           dataHandleData() to avoid dependency on Handle.h, additionally
           checked with a static_assert() in the source file. */
        explicit EventConnection(EventLayer& layer, DataHandle data) noexcept;

        Containers::Reference<EventLayer> _layer;
        LayerDataHandle _data;
};

namespace Implementation {
    enum class EventType: UnsignedByte;
}

/**
@brief Event handling layer
@m_since_latest

Provides signal/slot-like functionality, connecting events happening on nodes
with arbitrary functions handling them.

@section Ui-EventLayer-setup Setting up an event layer instance

If you create a @ref UserInterfaceGL instance with a style and don't exclude
@ref StyleFeature::EventLayer, an implicit instance is already provided and
available through @ref UserInterface::eventLayer(). If you don't, or if you
want to set up a custom event layer, pass its instance to
@ref AbstractUserInterface::setLayerInstance():

@snippet Ui.cpp EventLayer-setup

@section Ui-EventLayer-create Creating event handlers

An event handler is created by calling one of the `on*()` APIs with a
@ref NodeHandle to react on (or any @ref AbstractWidget "Widget" instance,
since it's implicitly convertible to a @ref NodeHandle as well) and a desired
function. For example, using @ref onTapOrClick() to print a message to standard
output when a button is tapped or clicked:

@snippet Ui.cpp EventLayer-create

By default the event handler is tied to lifetime of the node it's added to. You
can use the returned @ref DataHandle to @ref remove() it earlier, if needed. As
an alternative, all APIs have `on*Scoped()` variants that return a
[RAII](https://en.wikipedia.org/wiki/Resource_acquisition_is_initialization)
@ref EventConnection instance that removes the handler when destructed, which
is useful for example when the lifetime is tied to a concrete class instance.
In the following snippet, the @ref onTapOrClickScoped() in addition uses a @relativeref{Corrade,Containers::Function} created from a member function
instead of a lambda:

@snippet Ui.cpp EventLayer-create-scoped

@section Ui-EventLayer-tap-click-press-release Tap or click, press & release

A function passed to @ref onTapOrClick() gets called when a
@ref Pointer::MouseLeft, primary @ref Pointer::Finger or @ref Pointer::Pen
(i.e., a pointer type that's understood as performing the primary action) is
pressed and subsequently released on the same node. Relying on
@ref Ui-AbstractUserInterface-events-capture "pointer capture" being implicitly
enabled on a @m_class{m-label m-info} **press**, the pointer is allowed to
leave the area when being pressed. If a @m_class{m-label m-danger} **release**
then happens with the pointer back over the node, it's still recognized as a
tap or click, which improves reliability especially with imprecise touch input.
On the other hand, releasing with a pointer outside of the node will not be
recognized as a tap or click, which is a common usage pattern for canceling
accidental UI actions.

@htmlinclude ui-eventlayer-tap-or-click.svg

The @ref onMiddleClick() and @ref onRightClick() APIs then expose the same
functionality for @ref Pointer::MouseMiddle or @ref Pointer::MouseRight. Right
now, @ref Pointer::Eraser doesn't have any matching event handler. Finally, the
@ref onPress() and @ref onRelease() counterparts get called when
@ref Pointer::MouseLeft, primary @ref Pointer::Finger or @ref Pointer::Pen gets
pressed or released.

All these functions have additional overloads that pass a node-relative
position to the handler, useful in case the position matters. For example to
calculate a concrete color when pressing on a color picker:

@snippet Ui.cpp EventLayer-tap-position

@section Ui-EventLayer-drag Pointer drag

The @ref onDrag() API calls a function when @ref Pointer::MouseLeft, primary
@ref Pointer::Finger or @ref Pointer::Pen is dragged over a node, passing a
relative movement position to it. The handler is called only if the event is
@ref Ui-AbstractUserInterface-events-capture "captured on the node", i.e. when
the drag started on it in the first place. Which prevents accidental action,
such as when dragging from outside of the active UI area, however at the same
time it also allows the pointer to leave the node when dragging without the
action being aborted. Such behavior is useful especially when interacting with
narrow UI elements such as scrollbars or sliders.

@htmlinclude ui-eventlayer-drag.svg

Below is an example of a (vertical) scrollbar implementation affecting a
scrollable view. At its core, a scroll area is a node with @ref NodeFlag::Clip
enabled, containing another larger node within, and the scrolling is performed
by adjusting offset of the inner node with
@ref AbstractUserInterface::setNodeOffset(). Additionally the code makes sure
it doesn't scroll beyond either of the edges.

@snippet Ui.cpp EventLayer-drag

As with other pointer event handlers, there's also an overload taking both a
movement delta and a position within a node.

@section Ui-EventLayer-pinch Pinch zoom and rotation

On input devices supporting multi-touch, @ref onPinch() calls a function as
soon as two fingers are pressed and dragged relatively to each other, passing
it a scale, rotation and translation delta. As with @ref onDrag(), the gesture
is only recognized when it started on given node but the pointers can leave the
area when pressed. As an example, the following snippet implements two-finger
panning in a drawing application while the single-pointer @ref onDrag() would
be used for drawing:

@m_class{m-console-wrap}

@snippet Ui.cpp EventLayer-pinch

@section Ui-EventLayer-press-release-enter-leave-focus-blur Pointer enter and leave, focus and blur

With @ref onEnter() and @ref onLeave() you can implement various actions
happening when the primary pointer enters or leaves the node area. Usually
there's however just visual feedback, which is likely already handled by
@ref BaseLayer and @ref TextLayer style transitions.

For nodes that have @ref NodeFlag::Focusable enabled, @ref onFocus() and
@ref onBlur() can attach actions to an input being focused and blurred again,
for example to open a keypad or an autocompletion popup.
*/
class MAGNUM_UI_EXPORT EventLayer: public AbstractLayer {
    public:
        /**
         * @brief Constructor
         * @param handle    Layer handle returned from
         *      @ref AbstractUserInterface::createLayer()
         */
        explicit EventLayer(LayerHandle handle);

        /** @brief Copying is not allowed */
        EventLayer(const EventLayer&) = delete;

        /**
         * @brief Move constructor
         *
         * Performs a destructive move, i.e. the original object isn't usable
         * afterwards anymore.
         */
        EventLayer(EventLayer&&) noexcept;

        /**
         * @brief Destructor
         *
         * Expects that @ref usedScopedConnectionCount() is @cpp 0 @ce.
         */
        ~EventLayer() override;

        /** @brief Copying is not allowed */
        EventLayer& operator=(const EventLayer&) = delete;

        /** @brief Move assignment */
        EventLayer& operator=(EventLayer&&) noexcept;

        /**
         * @brief Count of currently active @ref EventConnection instances
         *
         * Always at most @ref usedCount(). The layer is only allowed to be
         * destroyed after all scoped connections are removed, as the
         * @ref EventConnection destructors would then access a dangling layer
         * pointer.
         */
        UnsignedInt usedScopedConnectionCount() const;

        /**
         * @brief Count of allocated connections
         *
         * Always at most @ref usedCount(). Counts all connections that capture
         * non-trivially-destructible state or state that's too large to be
         * stored in-place. The operation is done with a @f$ \mathcal{O}(n) @f$
         * complexity where @f$ n @f$ is @ref capacity().
         * @todoc fix the isAllocated link once Doxygen stops being shit -- it
         *      works only from Containers themselves
         * @see @ref Corrade::Containers::Function "Containers::Function<R(Args...)>::isAllocated()"
         */
        UnsignedInt usedAllocatedConnectionCount() const;

        /**
         * @brief Connect to a finger / pen tap or left mouse press
         *
         * The @p slot, optionally receiving a node-relative position of the
         * press, is called when a @ref Pointer::MouseLeft, primary
         * @ref Pointer::Finger or @ref Pointer::Pen press happens on the
         * @p node. Expects that the @p slot is not @cpp nullptr @ce.
         *
         * Use @ref onTapOrClick() for a combined press and release. The
         * returned @ref DataHandle is automatically removed once @p node or
         * any of its parents is removed, it's the caller responsibility to
         * ensure it doesn't outlive the state captured in the @p slot. See
         * @ref onPressScoped() for a scoped alternative.
         * @see @ref Ui-EventLayer-tap-click-press-release,
         *      @ref PointerEvent::isPrimary()
         */
        DataHandle onPress(NodeHandle node, Containers::Function<void()>&& slot);

        /** @overload */
        DataHandle onPress(NodeHandle node, Containers::Function<void(const Vector2& position)>&& slot);

        /**
         * @brief Scoped connection to a finger / pen tap or left mouse press
         *
         * Compared to @ref onPress() the connection is removed automatically
         * when the returned @ref EventConnection gets destroyed.
         * @see @ref Ui-EventLayer-create
         */
        EventConnection onPressScoped(NodeHandle node, Containers::Function<void()>&& slot) {
            return EventConnection{*this, onPress(node, Utility::move(slot))};
        }

        /** @overload */
        EventConnection onPressScoped(NodeHandle node, Containers::Function<void(const Vector2& position)>&& slot) {
            return EventConnection{*this, onPress(node, Utility::move(slot))};
        }

        /**
         * @brief Connect to a finger / pen tap or left mouse release
         *
         * The @p slot, optionally receiving a node-relative position of the
         * release, is called when a @ref Pointer::MouseLeft, primary
         * @ref Pointer::Finger or @ref Pointer::Pen release happens on the
         * @p node. Expects that the @p slot is not @cpp nullptr @ce.
         *
         * Use @ref onTapOrClick() for a combined press and release. The
         * returned @ref DataHandle is automatically removed once @p node or
         * any of its parents is removed, it's the caller responsibility to
         * ensure it doesn't outlive the state captured in the @p slot. See
         * @ref onReleaseScoped() for a scoped alternative.
         * @see @ref Ui-EventLayer-tap-click-press-release,
         *      @ref PointerEvent::isPrimary()
         */
        DataHandle onRelease(NodeHandle node, Containers::Function<void()>&& slot);

        /** @overload */
        DataHandle onRelease(NodeHandle node, Containers::Function<void(const Vector2& position)>&& slot);

        /**
         * @brief Scoped connection to a finger / pen tap or left mouse release
         *
         * Compared to @ref onRelease() the connection is removed automatically
         * when the returned @ref EventConnection gets destroyed.
         * @see @ref Ui-EventLayer-create
         */
        EventConnection onReleaseScoped(NodeHandle node, Containers::Function<void()>&& slot) {
            return EventConnection{*this, onRelease(node, Utility::move(slot))};
        }

        /** @overload */
        EventConnection onReleaseScoped(NodeHandle node, Containers::Function<void(const Vector2& position)>&& slot) {
            return EventConnection{*this, onRelease(node, Utility::move(slot))};
        }

        /**
         * @brief Connect to a finger / pen tap or left mouse click
         *
         * The @p slot, optionally receiving a node-relative position of the
         * tap or click, is called when a @ref Pointer::MouseLeft, primary
         * @ref Pointer::Finger or @ref Pointer::Pen release happens on the
         * @p node after a previous primary pointer press. If event capture is
         * disabled by any event handler on given node, the slot is called only
         * if the pointer didn't leave the node area between a press and a
         * release. Expects that the @p slot is not @cpp nullptr @ce.
         *
         * Use @ref onRightClick() and @ref onMiddleClick() to handle
         * @ref Pointer::MouseRight and @ref Pointer::MouseMiddle clicks. The
         * returned @ref DataHandle is automatically removed once @p node or
         * any of its parents is removed, it's the caller responsibility to
         * ensure it doesn't outlive the state captured in the @p slot. See
         * @ref onTapOrClickScoped() for a scoped alternative.
         * @see @ref Ui-EventLayer-tap-click-press-release,
         *      @ref PointerEvent::isPrimary()
         */
        DataHandle onTapOrClick(NodeHandle node, Containers::Function<void()>&& slot);

        /** @overload */
        DataHandle onTapOrClick(NodeHandle node, Containers::Function<void(const Vector2& position)>&& slot);

        /**
         * @brief Scoped connection to a finger / pen tap or left mouse click
         *
         * Compared to @ref onTapOrClick() the connection is removed
         * automatically when the returned @ref EventConnection gets destroyed.
         * @see @ref Ui-EventLayer-create
         */
        EventConnection onTapOrClickScoped(NodeHandle node, Containers::Function<void()>&& slot) {
            return EventConnection{*this, onTapOrClick(node, Utility::move(slot))};
        }

        /** @overload */
        EventConnection onTapOrClickScoped(NodeHandle node, Containers::Function<void(const Vector2& position)>&& slot) {
            return EventConnection{*this, onTapOrClick(node, Utility::move(slot))};
        }

        /**
         * @brief Connect to a middle mouse click
         *
         * The @p slot, optionally receiving a node-relative position of the
         * click, is called when a @ref Pointer::MouseMiddle release happens on
         * the @p node after a previous pointer press. If event capture is
         * disabled by any event handler on given node, the slot is called only
         * if the pointer didn't leave the node area between a press and a
         * release. Expects that the @p slot is not @cpp nullptr @ce.
         *
         * Use @ref onTapOrClick() and @ref onRightClick() to handle
         * @ref Pointer::MouseLeft / @ref Pointer::Finger / @ref Pointer::Pen
         * and @ref Pointer::MouseRight clicks. The returned @ref DataHandle is
         * automatically removed once @p node or any of its parents is removed,
         * it's the caller responsibility to ensure it doesn't outlive the
         * state captured in the @p slot. See @ref onMiddleClickScoped() for a
         * scoped alternative.
         * @see @ref Ui-EventLayer-tap-click-press-release
         */
        DataHandle onMiddleClick(NodeHandle node, Containers::Function<void()>&& slot);

        /** @overload */
        DataHandle onMiddleClick(NodeHandle node, Containers::Function<void(const Vector2& position)>&& slot);

        /**
         * @brief Scoped connection to a middle mouse click
         *
         * Compared to @ref onMiddleClick() the connection is removed
         * automatically when the returned @ref EventConnection gets destroyed.
         * @see @ref Ui-EventLayer-create
         */
        EventConnection onMiddleClickScoped(NodeHandle node, Containers::Function<void()>&& slot) {
            return EventConnection{*this, onMiddleClick(node, Utility::move(slot))};
        }

        /** @overload */
        EventConnection onMiddleClickScoped(NodeHandle node, Containers::Function<void(const Vector2& position)>&& slot) {
            return EventConnection{*this, onMiddleClick(node, Utility::move(slot))};
        }

        /**
         * @brief Connect to a right mouse click
         *
         * The @p slot, optionally receiving a node-relative position of the
         * click, is called when a @ref Pointer::MouseRight release happens on
         * the @p node after a previous pointer press. If event capture is
         * disabled by any event handler on given node, the slot is called only
         * if the pointer didn't leave the node area between a press and a
         * release. Expects that the @p slot is not @cpp nullptr @ce.
         *
         * Use @ref onTapOrClick() and @ref onRightClick() to handle
         * @ref Pointer::MouseLeft / @ref Pointer::Finger / @ref Pointer::Pen
         * and @ref Pointer::MouseRight clicks. The returned @ref DataHandle is
         * automatically removed once @p node or any of its parents is removed,
         * it's the caller responsibility to ensure it doesn't outlive the
         * state captured in the @p slot. See @ref onMiddleClickScoped() for a
         * scoped alternative.
         * @see @ref Ui-EventLayer-tap-click-press-release
         */
        DataHandle onRightClick(NodeHandle node, Containers::Function<void()>&& slot);

        /** @overload */
        DataHandle onRightClick(NodeHandle node, Containers::Function<void(const Vector2& position)>&& slot);

        /**
         * @brief Scoped connection to a right mouse click
         *
         * Compared to @ref onRightClick() the connection is removed
         * automatically when the returned @ref EventConnection gets destroyed.
         * @see @ref Ui-EventLayer-create
         */
        EventConnection onRightClickScoped(NodeHandle node, Containers::Function<void()>&& slot) {
            return EventConnection{*this, onRightClick(node, Utility::move(slot))};
        }

        /** @overload */
        EventConnection onRightClickScoped(NodeHandle node, Containers::Function<void(const Vector2& position)>&& slot) {
            return EventConnection{*this, onRightClick(node, Utility::move(slot))};
        }

        /**
         * @brief Connect to a drag
         *
         * The @p slot, receiving the movement delta and optionally also a
         * node-relative position at which the move happened, is called when a
         * @ref Pointer::MouseLeft, primary @ref Pointer::Finger or
         * @ref Pointer::Pen move happens on the @p node. To prevent the slot
         * from being triggered by drags that originated outside of @p node,
         * it's called only if the move event is captured on given node.
         * Expects that the @p slot is not @cpp nullptr @ce.
         *
         * Use @ref onPinch() to handle two-finger gestures. The returned
         * @ref DataHandle is automatically removed once @p node or any of its
         * parents is removed, it's the caller responsibility to ensure it
         * doesn't outlive the state captured in the @p slot. See
         * @ref onDragScoped() for a scoped alternative.
         * @see @ref Ui-EventLayer-drag, @ref PointerMoveEvent::isPrimary()
         */
        DataHandle onDrag(NodeHandle node, Containers::Function<void(const Vector2& relativePosition)>&& slot);

        /** @overload */
        DataHandle onDrag(NodeHandle node, Containers::Function<void(const Vector2& position, const Vector2& relativePosition)>&& slot);

        /**
         * @brief Scoped connection to a drag
         *
         * Compared to @ref onDrag() the connection is removed automatically
         * when the returned @ref EventConnection gets destroyed.
         * @see @ref Ui-EventLayer-create
         */
        EventConnection onDragScoped(NodeHandle node, Containers::Function<void(const Vector2& relativePosition)>&& slot) {
            return EventConnection{*this, onDrag(node, Utility::move(slot))};
        }

        /** @overload */
        EventConnection onDragScoped(NodeHandle node, Containers::Function<void(const Vector2& position, const Vector2& relativePosition)>&& slot) {
            return EventConnection{*this, onDrag(node, Utility::move(slot))};
        }

        /**
         * @brief Connect to a two-finger pinch, zoom or pan gesture
         *
         * The @p slot is called when two or more @ref Pointer::Finger are
         * pressed and move over the @p node area. It receives a node-relative
         * centroid position between the two presses, translation of the
         * centroid relative to previous state of the two fingers, their
         * relative rotation and scaling. @ref Platform::TwoFingerGesture is
         * used internally, see its documentation for more information. Expects
         * that the @p slot is not @cpp nullptr @ce.
         *
         * By default, the gesture is tracked as long as the primary finger is
         * pressed, even if the fingers are outside of the node area. If
         * event capture is disabled by any event handler on given node, the
         * gesture stops being tracked when any of the fingers leave the node
         * area. The gesture also stops being tracked when given node loses
         * visibility to events.
         *
         * Use @ref onDrag() to handle a regular single-finger, mouse or pen
         * drag. The returned @ref DataHandle is automatically removed once
         * @p node or any of its parents is removed, it's the caller
         * responsibility to ensure it doesn't outlive the state captured in
         * the @p slot. See @ref onPinchScoped() for a scoped alternative.
         * @see @ref Ui-EventLayer-pinch
         */
        DataHandle onPinch(NodeHandle node, Containers::Function<void(const Vector2& position, const Vector2& relativeTranslation, const Complex& relativeRotation, Float relativeScaling)>&& slot);

        /**
         * @brief Scoped connection to a two-finger pinch, zoom or pan gesture
         *
         * Compared to @ref onPinch() the connection is removed automatically
         * when the returned @ref EventConnection gets destroyed.
         * @see @ref Ui-EventLayer-create
         */
        EventConnection onPinchScoped(NodeHandle node, Containers::Function<void(const Vector2& position, const Vector2& relativeTranslation, const Complex& relativeRotation, Float relativeScaling)>&& slot) {
            return EventConnection{*this, onPinch(node, Utility::move(slot))};
        }

        /**
         * @brief Connect to a pointer enter
         *
         * The @p slot is called when a primary pointer moves over the
         * @p node area. Expects that the @p slot is not @cpp nullptr @ce.
         *
         * Use @ref onLeave() to hadle the opposite case. The returned
         * @ref DataHandle is automatically removed once @p node or any of its
         * parents is removed, it's the caller responsibility to ensure it
         * doesn't outlive the state captured in the @p slot. See
         * @ref onEnterScoped() for a scoped alternative.
         * @see @ref Ui-EventLayer-press-release-enter-leave-focus-blur,
         *      @ref PointerMoveEvent::isPrimary()
         */
        DataHandle onEnter(NodeHandle node, Containers::Function<void()>&& slot);

        /**
         * @brief Scoped connection to a pointer enter
         *
         * Compared to @ref onEnter() the connection is removed automatically
         * when the returned @ref EventConnection gets destroyed.
         * @see @ref Ui-EventLayer-create
         */
        EventConnection onEnterScoped(NodeHandle node, Containers::Function<void()>&& slot) {
            return EventConnection{*this, onEnter(node, Utility::move(slot))};
        }

        /**
         * @brief Connect to a pointer leave
         *
         * The @p slot is called when a primary pointer moves out of the
         * @p node area. Expects that the @p slot is not @cpp nullptr @ce.
         *
         * Use @ref onEnter() to hadle the opposite case. The returned
         * @ref DataHandle is automatically removed once @p node or any of its
         * parents is removed, it's the caller responsibility to ensure it
         * doesn't outlive the state captured in the @p slot. See
         * @ref onLeaveScoped() for a scoped alternative.
         * @see @ref Ui-EventLayer-press-release-enter-leave-focus-blur,
         *      @ref PointerMoveEvent::isPrimary()
         */
        DataHandle onLeave(NodeHandle node, Containers::Function<void()>&& slot);

        /**
         * @brief Scoped connection to a pointer leave
         *
         * Compared to @ref onLeave() the connection is removed automatically
         * when the returned @ref EventConnection gets destroyed.
         * @see @ref Ui-EventLayer-create
         */
        EventConnection onLeaveScoped(NodeHandle node, Containers::Function<void()>&& slot) {
            return EventConnection{*this, onLeave(node, Utility::move(slot))};
        }

        /**
         * @brief Connect to a focus
         *
         * The @p slot is called when a @p node is focused. Expects that the
         * @p slot is not @cpp nullptr @ce.
         *
         * Use @ref onBlur() to hadle the opposite case. The returned
         * @ref DataHandle is automatically removed once @p node or any of its
         * parents is removed, it's the caller responsibility to ensure it
         * doesn't outlive the state captured in the @p slot. See
         * @ref onFocusScoped() for a scoped alternative.
         * @see @ref Ui-EventLayer-press-release-enter-leave-focus-blur
         */
        DataHandle onFocus(NodeHandle node, Containers::Function<void()>&& slot);

        /**
         * @brief Scoped connection to a focus
         *
         * Compared to @ref onFocus() the connection is removed automatically
         * when the returned @ref EventConnection gets destroyed.
         * @see @ref Ui-EventLayer-create
         */
        EventConnection onFocusScoped(NodeHandle node, Containers::Function<void()>&& slot) {
            return EventConnection{*this, onFocus(node, Utility::move(slot))};
        }

        /**
         * @brief Connect to a blur
         *
         * The @p slot is called when the @p node is blurred. Expects that the
         * @p slot is not @cpp nullptr @ce.
         *
         * Use @ref onFocus() to hadle the opposite case. The returned
         * @ref DataHandle is automatically removed once @p node or any of its
         * parents is removed, it's the caller responsibility to ensure it
         * doesn't outlive the state captured in the @p slot. See
         * @ref onBlurScoped() for a scoped alternative.
         * @see @ref Ui-EventLayer-press-release-enter-leave-focus-blur
         */
        DataHandle onBlur(NodeHandle node, Containers::Function<void()>&& slot);

        /**
         * @brief Scoped connection to a pointer leave
         *
         * Compared to @ref onBlur() the connection is removed automatically
         * when the returned @ref EventConnection gets destroyed.
         * @see @ref Ui-EventLayer-create
         */
        EventConnection onBlurScoped(NodeHandle node, Containers::Function<void()>&& slot) {
            return EventConnection{*this, onBlur(node, Utility::move(slot))};
        }

        /**
         * @brief Remove a connection
         *
         * Delegates to @ref AbstractLayer::remove(DataHandle) and additionally
         * calls a destructor on the captured function state, if it's not
         * trivially destructible. The @p handle becomes invalid, which means
         * that any existing @ref EventConnection instance for it will not
         * attempt to remove it again.
         * @see @ref usedAllocatedConnectionCount()
         */
        void remove(DataHandle handle);

        /**
         * @brief Remove a connection assuming it belongs to this layer
         *
         * Delegates to @ref AbstractLayer::remove(LayerDataHandle) and
         * additionally calls a destructor on the captured function state, if
         * it's not trivially destructible. The @p handle becomes invalid,
         * which means that any existing @ref EventConnection instance for it
         * will not attempt to remove it again.
         * @see @ref usedAllocatedConnectionCount()
         */
        void remove(LayerDataHandle handle);

    private:
        /* Updates usedScopedConnectionCount */
        friend EventConnection;

        /* Used internally from all templated create() overloads below */
        MAGNUM_UI_LOCAL DataHandle create(NodeHandle node, Implementation::EventType eventType, Containers::FunctionData&& slot, void(*call)());
        MAGNUM_UI_LOCAL void removeInternal(UnsignedInt id);

        MAGNUM_UI_LOCAL LayerFeatures doFeatures() const override;
        MAGNUM_UI_LOCAL void doClean(Containers::BitArrayView dataIdsToRemove) override;

        MAGNUM_UI_LOCAL void doPointerPressEvent(UnsignedInt dataId, PointerEvent& event) override;
        MAGNUM_UI_LOCAL void doPointerReleaseEvent(UnsignedInt dataId, PointerEvent& event) override;
        MAGNUM_UI_LOCAL void doPointerMoveEvent(UnsignedInt dataId, PointerMoveEvent& event) override;
        MAGNUM_UI_LOCAL void doPointerEnterEvent(UnsignedInt dataId, PointerMoveEvent& event) override;
        MAGNUM_UI_LOCAL void doPointerLeaveEvent(UnsignedInt dataId, PointerMoveEvent& event) override;
        MAGNUM_UI_LOCAL void doFocusEvent(UnsignedInt dataId, FocusEvent& event) override;
        MAGNUM_UI_LOCAL void doBlurEvent(UnsignedInt dataId, FocusEvent& event) override;
        MAGNUM_UI_LOCAL void doVisibilityLostEvent(UnsignedInt dataId, VisibilityLostEvent& event) override;

        struct State;
        Containers::Pointer<State> _state;
};

inline EventConnection::~EventConnection()  {
    /* Not using LayerDataHandle::Null to avoid including Handle.h; OTOH
       keeping the destructor inlined for debug perf */
    if(_data != LayerDataHandle{} && _layer->isHandleValid(_data))
        _layer->remove(_data);
}

}}

#endif
