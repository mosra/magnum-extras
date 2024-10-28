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
with aribtrary functions handling them.
@see @ref UserInterface::eventLayer(),
    @ref UserInterface::setEventLayerInstance(), @ref StyleFeature::EventLayer
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
         * @see @ref Corrade::Containers::Function<R(Args...)::isAllocated()
         */
        UnsignedInt usedAllocatedConnectionCount() const;

        /**
         * @brief Connect to a finger / pen tap or left mouse press
         *
         * The @p slot, optionally receiving a node-relative position of the
         * press, is called when a @ref Pointer::MouseLeft, primary
         * @ref Pointer::Finger or @ref Pointer::Pen press happens on the
         * @p node.
         *
         * Use @ref onTapOrClick() for a combined press and release. The
         * returned @ref DataHandle is automatically removed once @p node or
         * any of its parents is removed, it's the caller responsibility to
         * ensure it doesn't outlive the state captured in the @p slot. See
         * @ref onPressScoped() for a scoped alternative.
         * @ref PointerEvent::isPrimary()
         */
        DataHandle onPress(NodeHandle node, Containers::Function<void()>&& slot);

        /** @overload */
        DataHandle onPress(NodeHandle node, Containers::Function<void(const Vector2& position)>&& slot);

        /**
         * @brief Scoped connection to a finger / pen tap or left mouse press
         *
         * Compared to @ref onPress() the connection is removed automatically
         * when the returned @ref EventConnection gets destroyed.
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
         * @p node.
         *
         * Use @ref onTapOrClick() for a combined press and release. The
         * returned @ref DataHandle is automatically removed once @p node or
         * any of its parents is removed, it's the caller responsibility to
         * ensure it doesn't outlive the state captured in the @p slot. See
         * @ref onReleaseScoped() for a scoped alternative.
         * @ref PointerEvent::isPrimary()
         */
        DataHandle onRelease(NodeHandle node, Containers::Function<void()>&& slot);

        /** @overload */
        DataHandle onRelease(NodeHandle node, Containers::Function<void(const Vector2& position)>&& slot);

        /**
         * @brief Scoped connection to a finger / pen tap or left mouse release
         *
         * Compared to @ref onRelease() the connection is removed automatically
         * when the returned @ref EventConnection gets destroyed.
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
         * release.
         *
         * Use @ref onRightClick() and @ref onMiddleClick() to handle
         * @ref Pointer::MouseRight and @ref Pointer::MouseMiddle clicks. The
         * returned @ref DataHandle is automatically removed once @p node or
         * any of its parents is removed, it's the caller responsibility to
         * ensure it doesn't outlive the state captured in the @p slot. See
         * @ref onTapOrClickScoped() for a scoped alternative.
         * @ref PointerEvent::isPrimary()
         */
        DataHandle onTapOrClick(NodeHandle node, Containers::Function<void()>&& slot);

        /** @overload */
        DataHandle onTapOrClick(NodeHandle node, Containers::Function<void(const Vector2& position)>&& slot);

        /**
         * @brief Scoped connection to a finger / pen tap or left mouse click
         *
         * Compared to @ref onTapOrClick() the connection is removed
         * automatically when the returned @ref EventConnection gets destroyed.
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
         * release.
         *
         * Use @ref onTapOrClick() and @ref onRightClick() to handle
         * @ref Pointer::MouseLeft / @ref Pointer::Finger / @ref Pointer::Pen
         * and @ref Pointer::MouseRight clicks. The returned @ref DataHandle is
         * automatically removed once @p node or any of its parents is removed,
         * it's the caller responsibility to ensure it doesn't outlive the
         * state captured in the @p slot. See @ref onMiddleClickScoped() for a
         * scoped alternative.
         */
        DataHandle onMiddleClick(NodeHandle node, Containers::Function<void()>&& slot);

        /** @overload */
        DataHandle onMiddleClick(NodeHandle node, Containers::Function<void(const Vector2& position)>&& slot);

        /**
         * @brief Scoped connection to a middle mouse click
         *
         * Compared to @ref onMiddleClick() the connection is removed
         * automatically when the returned @ref EventConnection gets destroyed.
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
         * release.
         *
         * Use @ref onTapOrClick() and @ref onRightClick() to handle
         * @ref Pointer::MouseLeft / @ref Pointer::Finger / @ref Pointer::Pen
         * and @ref Pointer::MouseRight clicks. The returned @ref DataHandle is
         * automatically removed once @p node or any of its parents is removed,
         * it's the caller responsibility to ensure it doesn't outlive the
         * state captured in the @p slot. See @ref onMiddleClickScoped() for a
         * scoped alternative.
         */
        DataHandle onRightClick(NodeHandle node, Containers::Function<void()>&& slot);

        /** @overload */
        DataHandle onRightClick(NodeHandle node, Containers::Function<void(const Vector2& position)>&& slot);

        /**
         * @brief Scoped connection to a right mouse click
         *
         * Compared to @ref onRightClick() the connection is removed
         * automatically when the returned @ref EventConnection gets destroyed.
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
         *
         * Use @ref onPinch() to handle two-finger gestures. The returned
         * @ref DataHandle is automatically removed once @p node or any of its
         * parents is removed, it's the caller responsibility to ensure it
         * doesn't outlive the state captured in the @p slot. See
         * @ref onDragScoped() for a scoped alternative.
         * @ref PointerMoveEvent::isPrimary()
         */
        DataHandle onDrag(NodeHandle node, Containers::Function<void(const Vector2& relativePosition)>&& slot);

        /** @overload */
        DataHandle onDrag(NodeHandle node, Containers::Function<void(const Vector2& position, const Vector2& relativePosition)>&& slot);

        /**
         * @brief Scoped connection to a drag
         *
         * Compared to @ref onDrag() the connection is removed automatically
         * when the returned @ref EventConnection gets destroyed.
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
         * used internally, see its documentation for more information.
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
         */
        DataHandle onPinch(NodeHandle node, Containers::Function<void(const Vector2& position, const Vector2& relativeTranslation, const Complex& relativeRotation, Float relativeScaling)>&& slot);

        /**
         * @brief Scoped connection to a two-finger pinch, zoom or pan gesture
         *
         * Compared to @ref onPinch() the connection is removed automatically
         * when the returned @ref EventConnection gets destroyed.
         */
        EventConnection onPinchScoped(NodeHandle node, Containers::Function<void(const Vector2& position, const Vector2& relativeTranslation, const Complex& relativeRotation, Float relativeScaling)>&& slot) {
            return EventConnection{*this, onPinch(node, Utility::move(slot))};
        }

        /**
         * @brief Connect to a pointer enter
         *
         * The @p slot is called when a primary pointer moves over the
         * @p node area.
         *
         * Use @ref onLeave() to hadle the opposite case. The returned
         * @ref DataHandle is automatically removed once @p node or any of its
         * parents is removed, it's the caller responsibility to ensure it
         * doesn't outlive the state captured in the @p slot. See
         * @ref onEnterScoped() for a scoped alternative.
         * @ref PointerMoveEvent::isPrimary()
         */
        DataHandle onEnter(NodeHandle node, Containers::Function<void()>&& slot);

        /**
         * @brief Scoped connection to a pointer enter
         *
         * Compared to @ref onEnter() the connection is removed automatically
         * when the returned @ref EventConnection gets destroyed.
         */
        EventConnection onEnterScoped(NodeHandle node, Containers::Function<void()>&& slot) {
            return EventConnection{*this, onEnter(node, Utility::move(slot))};
        }

        /**
         * @brief Connect to a pointer leave
         *
         * The @p slot is called when a primary pointer moves out of the
         * @p node area.
         *
         * Use @ref onEnter() to hadle the opposite case. The returned
         * @ref DataHandle is automatically removed once @p node or any of its
         * parents is removed, it's the caller responsibility to ensure it
         * doesn't outlive the state captured in the @p slot. See
         * @ref onLeaveScoped() for a scoped alternative.
         * @ref PointerMoveEvent::isPrimary()
         */
        DataHandle onLeave(NodeHandle node, Containers::Function<void()>&& slot);

        /**
         * @brief Scoped connection to a pointer leave
         *
         * Compared to @ref onLeave() the connection is removed automatically
         * when the returned @ref EventConnection gets destroyed.
         */
        EventConnection onLeaveScoped(NodeHandle node, Containers::Function<void()>&& slot) {
            return EventConnection{*this, onLeave(node, Utility::move(slot))};
        }

        /**
         * @brief Connect to a focus
         *
         * The @p slot is called when a @p node is focused.
         *
         * Use @ref onBlur() to hadle the opposite case. The returned
         * @ref DataHandle is automatically removed once @p node or any of its
         * parents is removed, it's the caller responsibility to ensure it
         * doesn't outlive the state captured in the @p slot. See
         * @ref onFocusScoped() for a scoped alternative.
         */
        DataHandle onFocus(NodeHandle node, Containers::Function<void()>&& slot);

        /**
         * @brief Scoped connection to a focus
         *
         * Compared to @ref onFocus() the connection is removed automatically
         * when the returned @ref EventConnection gets destroyed.
         */
        EventConnection onFocusScoped(NodeHandle node, Containers::Function<void()>&& slot) {
            return EventConnection{*this, onFocus(node, Utility::move(slot))};
        }

        /**
         * @brief Connect to a blur
         *
         * The @p slot is called when the @p node is blurred.
         *
         * Use @ref onFocus() to hadle the opposite case. The returned
         * @ref DataHandle is automatically removed once @p node or any of its
         * parents is removed, it's the caller responsibility to ensure it
         * doesn't outlive the state captured in the @p slot. See
         * @ref onBlurScoped() for a scoped alternative.
         */
        DataHandle onBlur(NodeHandle node, Containers::Function<void()>&& slot);

        /**
         * @brief Scoped connection to a pointer leave
         *
         * Compared to @ref onBlur() the connection is removed automatically
         * when the returned @ref EventConnection gets destroyed.
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
        MAGNUM_UI_LOCAL void doPointerTapOrClickEvent(UnsignedInt dataId, PointerEvent& event) override;
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
