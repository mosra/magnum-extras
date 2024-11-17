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

#include "EventLayer.h"

#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/BitArrayView.h>
#include <Corrade/Containers/GrowableArray.h>
#include <Corrade/Containers/Function.h>
#include <Magnum/Platform/Gesture.h>

#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/Event.h"

namespace Magnum { namespace Ui {

/* EventConnection converts DataHandle to LayerDataHandle by taking the lower
   32 bits. Check that the bit counts didn't get out of sync since that
   assumption. */
static_assert(Implementation::LayerDataHandleIdBits + Implementation::LayerDataHandleGenerationBits == 32, "EventConnection DataHandle to LayerDataHandle conversion needs an update");

namespace Implementation {
    enum class EventType: UnsignedByte {
        Enter,
        Leave,
        Press,
        Release,
        Focus,
        Blur,
        TapOrClick,
        MiddleClick,
        RightClick,
        Drag,
        Pinch
    };
}

namespace {

struct Data {
    Containers::FunctionData slot;
    Implementation::EventType eventType;
    bool hasScopedConnection;
    /* 6+ bytes free */
    /** @todo ideally this would be inlined directly inside FunctionData.call,
        somehow -- e.g. an extra template argument to Function that decouples
        the actual wrapped signature from the call signature */
    void(*call)();
};

}

struct EventLayer::State {
    Containers::Array<Data> data;

    Platform::TwoFingerGesture twoFingerGesture;
    UnsignedInt twoFingerGestureData = ~UnsignedInt{};

    UnsignedInt usedScopedConnectionCount = 0;
};

EventConnection::EventConnection(EventLayer& layer, DataHandle data) noexcept: _layer{layer}, _data{LayerDataHandle(UnsignedLong(data))} {
    layer._state->data[dataHandleId(data)].hasScopedConnection = true;
    ++layer._state->usedScopedConnectionCount;
}

DataHandle EventConnection::data() const {
    return _data == LayerDataHandle::Null ? DataHandle::Null : dataHandle(_layer->handle(), _data);
}

DataHandle EventConnection::release() {
    if(_data != LayerDataHandle::Null) {
        CORRADE_INTERNAL_ASSERT(_layer->_state->data[layerDataHandleId(_data)].hasScopedConnection && _layer->_state->usedScopedConnectionCount);
        _layer->_state->data[layerDataHandleId(_data)].hasScopedConnection = false;
        --_layer->_state->usedScopedConnectionCount;
    }

    /* Becomes DataHandle::Null if _data is LayerDataHandle::Null */
    const DataHandle data = this->data();
    _data = LayerDataHandle::Null;
    return data;
}

EventLayer::EventLayer(LayerHandle handle): AbstractLayer{handle}, _state{InPlaceInit} {}

EventLayer::EventLayer(EventLayer&& other) noexcept: AbstractLayer{Utility::move(other)}, _state{Utility::move(other._state)} {
    /* _state may be nullptr after a (destructive) move */
    CORRADE_ASSERT(!_state || !_state->usedScopedConnectionCount,
        "Ui::EventLayer:" << _state->usedScopedConnectionCount << "scoped connections already active, can't move", );
}

EventLayer::~EventLayer() {
    /* _state may be nullptr after a (destructive) move */
    CORRADE_ASSERT(!_state || !_state->usedScopedConnectionCount,
        "Ui::EventLayer: destructed with" << _state->usedScopedConnectionCount << "scoped connections still active", );

    /* Destructors on any state captured in slots are called automatically on
       the Array destruction */
}

EventLayer& EventLayer::operator=(EventLayer&& other) noexcept {
    /* either _state may be nullptr after a (destructive) move */
    CORRADE_ASSERT(!_state || !_state->usedScopedConnectionCount,
        "Ui::EventLayer:" << _state->usedScopedConnectionCount << "scoped connections already active in the moved-to object, can't move", *this);
    CORRADE_ASSERT(!other._state || !other._state->usedScopedConnectionCount,
        "Ui::EventLayer:" << other._state->usedScopedConnectionCount << "scoped connections already active in the moved-from object, can't move", *this);

    AbstractLayer::operator=(Utility::move(other));
    using Utility::swap;
    swap(other._state, _state);
    return *this;
}

LayerFeatures EventLayer::doFeatures() const {
    return LayerFeature::Event;
}

UnsignedInt EventLayer::usedScopedConnectionCount() const {
    return _state->usedScopedConnectionCount;
}

UnsignedInt EventLayer::usedAllocatedConnectionCount() const {
    UnsignedInt count = 0;
    for(const Data& data: _state->data)
        if(data.slot.isAllocated())
            ++count;

    return count;
}

DataHandle EventLayer::create(const NodeHandle node, const Implementation::EventType eventType, Containers::FunctionData&& slot, void(*call)()) {
    CORRADE_ASSERT(slot,
        /* saying create() would be confusing, and passing onPress(),
           onPressScoped() etc as a string just for the assert from all
           variants seems excessive */
        "Ui::EventLayer: slot is null", {});

    State& state = static_cast<State&>(*_state);
    const DataHandle handle = AbstractLayer::create(node);
    const UnsignedInt id = dataHandleId(handle);
    if(id >= state.data.size())
        /* Can't use arrayAppend(NoInit) because the Function has to be
           zero-initialized */
        /** @todo some arrayAppend(DefaultInit, std::size_t)? */
        arrayResize(state.data, id + 1);

    Data& data = state.data[id];
    data.eventType = eventType;
    data.slot = Utility::move(slot);
    data.hasScopedConnection = false;
    data.call = call;
    return handle;
}

DataHandle EventLayer::onPress(const NodeHandle node, Containers::Function<void()>&& slot) {
    return create(node, Implementation::EventType::Press, Utility::move(slot),
        reinterpret_cast<void(*)()>(
            /* The + is to decay the lambda to a function pointer so we can
               cast it. MSVC 2015 says "error C2593: 'operator +' is ambiguous"
               so there it's an explicit cast to the function pointer type (and
               the parentheses are for both to have only one ifdef). */
            #ifndef CORRADE_MSVC2015_COMPATIBILITY
            +
            #else
            static_cast<void(*)(Containers::FunctionData&, const PointerEvent&)>
            #endif
        ([](Containers::FunctionData& slot, const PointerEvent&) {
            static_cast<Containers::Function<void()>&>(slot)();
        })));
}

DataHandle EventLayer::onPress(const NodeHandle node, Containers::Function<void(const Vector2&)>&& slot) {
    return create(node, Implementation::EventType::Press, Utility::move(slot),
        reinterpret_cast<void(*)()>(
            #ifndef CORRADE_MSVC2015_COMPATIBILITY
            +
            #else
            static_cast<void(*)(Containers::FunctionData&, const PointerEvent&)>
            #endif
        ([](Containers::FunctionData& slot, const PointerEvent& event) {
            static_cast<Containers::Function<void(const Vector2&)>&>(slot)(event.position());
        })));
}

DataHandle EventLayer::onRelease(const NodeHandle node, Containers::Function<void()>&& slot) {
    return create(node, Implementation::EventType::Release, Utility::move(slot),
        reinterpret_cast<void(*)()>(
            #ifndef CORRADE_MSVC2015_COMPATIBILITY
            +
            #else
            static_cast<void(*)(Containers::FunctionData&, const PointerEvent&)>
            #endif
        ([](Containers::FunctionData& slot, const PointerEvent&) {
            static_cast<Containers::Function<void()>&>(slot)();
        })));
}

DataHandle EventLayer::onRelease(const NodeHandle node, Containers::Function<void(const Vector2&)>&& slot) {
    return create(node, Implementation::EventType::Release, Utility::move(slot),
        reinterpret_cast<void(*)()>(
            #ifndef CORRADE_MSVC2015_COMPATIBILITY
            +
            #else
            static_cast<void(*)(Containers::FunctionData&, const PointerEvent&)>
            #endif
        ([](Containers::FunctionData& slot, const PointerEvent& event) {
            static_cast<Containers::Function<void(const Vector2&)>&>(slot)(event.position());
        })));
}

DataHandle EventLayer::onTapOrClick(const NodeHandle node, Containers::Function<void()>&& slot) {
    return create(node, Implementation::EventType::TapOrClick, Utility::move(slot),
        reinterpret_cast<void(*)()>(
            #ifndef CORRADE_MSVC2015_COMPATIBILITY
            +
            #else
            static_cast<void(*)(Containers::FunctionData&, const PointerEvent&)>
            #endif
        ([](Containers::FunctionData& slot, const PointerEvent&) {
            static_cast<Containers::Function<void()>&>(slot)();
        })));
}

DataHandle EventLayer::onTapOrClick(const NodeHandle node, Containers::Function<void(const Vector2&)>&& slot) {
    return create(node, Implementation::EventType::TapOrClick, Utility::move(slot),
        reinterpret_cast<void(*)()>(
            #ifndef CORRADE_MSVC2015_COMPATIBILITY
            +
            #else
            static_cast<void(*)(Containers::FunctionData&, const PointerEvent&)>
            #endif
        ([](Containers::FunctionData& slot, const PointerEvent& event) {
            static_cast<Containers::Function<void(const Vector2&)>&>(slot)(event.position());
        })));
}

DataHandle EventLayer::onMiddleClick(const NodeHandle node, Containers::Function<void()>&& slot) {
    return create(node, Implementation::EventType::MiddleClick, Utility::move(slot),
        reinterpret_cast<void(*)()>(
            #ifndef CORRADE_MSVC2015_COMPATIBILITY
            +
            #else
            static_cast<void(*)(Containers::FunctionData&, const PointerEvent&)>
            #endif
        ([](Containers::FunctionData& slot, const PointerEvent&) {
            static_cast<Containers::Function<void()>&>(slot)();
        })));
}

DataHandle EventLayer::onMiddleClick(const NodeHandle node, Containers::Function<void(const Vector2&)>&& slot) {
    return create(node, Implementation::EventType::MiddleClick, Utility::move(slot),
        reinterpret_cast<void(*)()>(
            #ifndef CORRADE_MSVC2015_COMPATIBILITY
            +
            #else
            static_cast<void(*)(Containers::FunctionData&, const PointerEvent&)>
            #endif
        ([](Containers::FunctionData& slot, const PointerEvent& event) {
            static_cast<Containers::Function<void(const Vector2&)>&>(slot)(event.position());
        })));
}

DataHandle EventLayer::onRightClick(const NodeHandle node, Containers::Function<void()>&& slot) {
    return create(node, Implementation::EventType::RightClick, Utility::move(slot),
        reinterpret_cast<void(*)()>(
            #ifndef CORRADE_MSVC2015_COMPATIBILITY
            +
            #else
            static_cast<void(*)(Containers::FunctionData&, const PointerEvent&)>
            #endif
        ([](Containers::FunctionData& slot, const PointerEvent&) {
            static_cast<Containers::Function<void()>&>(slot)();
        })));
}

DataHandle EventLayer::onRightClick(const NodeHandle node, Containers::Function<void(const Vector2&)>&& slot) {
    return create(node, Implementation::EventType::RightClick, Utility::move(slot),
        reinterpret_cast<void(*)()>(
            #ifndef CORRADE_MSVC2015_COMPATIBILITY
            +
            #else
            static_cast<void(*)(Containers::FunctionData&, const PointerEvent&)>
            #endif
        ([](Containers::FunctionData& slot, const PointerEvent& event) {
            static_cast<Containers::Function<void(const Vector2&)>&>(slot)(event.position());
        })));
}

DataHandle EventLayer::onDrag(const NodeHandle node, Containers::Function<void(const Vector2&)>&& slot) {
    return create(node, Implementation::EventType::Drag, Utility::move(slot),
        reinterpret_cast<void(*)()>(
            #ifndef CORRADE_MSVC2015_COMPATIBILITY
            +
            #else
            static_cast<void(*)(Containers::FunctionData&, const PointerMoveEvent&)>
            #endif
        ([](Containers::FunctionData& slot, const PointerMoveEvent& event) {
            static_cast<Containers::Function<void(const Vector2&)>&>(slot)(event.relativePosition());
        })));
}

DataHandle EventLayer::onDrag(const NodeHandle node, Containers::Function<void(const Vector2&, const Vector2&)>&& slot) {
    return create(node, Implementation::EventType::Drag, Utility::move(slot),
        reinterpret_cast<void(*)()>(
            #ifndef CORRADE_MSVC2015_COMPATIBILITY
            +
            #else
            static_cast<void(*)(Containers::FunctionData&, const PointerMoveEvent&)>
            #endif
        ([](Containers::FunctionData& slot, const PointerMoveEvent& event) {
            static_cast<Containers::Function<void(const Vector2&, const Vector2&)>&>(slot)(event.position(), event.relativePosition());
        })));
}

DataHandle EventLayer::onPinch(const NodeHandle node, Containers::Function<void(const Vector2&, const Vector2&, const Complex&, Float)>&& slot) {
    return create(node, Implementation::EventType::Pinch, Utility::move(slot),
        reinterpret_cast<void(*)()>(
            #ifndef CORRADE_MSVC2015_COMPATIBILITY
            +
            #else
            static_cast<void(*)(Containers::FunctionData&, const Platform::TwoFingerGesture&)>
            #endif
        ([](Containers::FunctionData& slot, const Platform::TwoFingerGesture& gesture) {
            static_cast<Containers::Function<void(const Vector2&, const Vector2&, const Complex&, Float)>&>(slot)(gesture.position(), gesture.relativeTranslation(), gesture.relativeRotation(), gesture.relativeScaling());
        })));
}

DataHandle EventLayer::onEnter(const NodeHandle node, Containers::Function<void()>&& slot) {
    return create(node, Implementation::EventType::Enter, Utility::move(slot),
        reinterpret_cast<void(*)()>(
            #ifndef CORRADE_MSVC2015_COMPATIBILITY
            +
            #else
            static_cast<void(*)(Containers::FunctionData&, const PointerMoveEvent&)>
            #endif
        ([](Containers::FunctionData& slot, const PointerMoveEvent&) {
            static_cast<Containers::Function<void()>&>(slot)();
        })));
}

DataHandle EventLayer::onLeave(const NodeHandle node, Containers::Function<void()>&& slot) {
    return create(node, Implementation::EventType::Leave, Utility::move(slot),
        reinterpret_cast<void(*)()>(
            #ifndef CORRADE_MSVC2015_COMPATIBILITY
            +
            #else
            static_cast<void(*)(Containers::FunctionData&, const PointerMoveEvent&)>
            #endif
        ([](Containers::FunctionData& slot, const PointerMoveEvent&) {
            static_cast<Containers::Function<void()>&>(slot)();
        })));
}

DataHandle EventLayer::onFocus(const NodeHandle node, Containers::Function<void()>&& slot) {
    return create(node, Implementation::EventType::Focus, Utility::move(slot),
        reinterpret_cast<void(*)()>(
            #ifndef CORRADE_MSVC2015_COMPATIBILITY
            +
            #else
            static_cast<void(*)(Containers::FunctionData&, const FocusEvent&)>
            #endif
        ([](Containers::FunctionData& slot, const FocusEvent&) {
            static_cast<Containers::Function<void()>&>(slot)();
        })));
}

DataHandle EventLayer::onBlur(const NodeHandle node, Containers::Function<void()>&& slot) {
    return create(node, Implementation::EventType::Blur, Utility::move(slot),
        reinterpret_cast<void(*)()>(
            #ifndef CORRADE_MSVC2015_COMPATIBILITY
            +
            #else
            static_cast<void(*)(Containers::FunctionData&, const FocusEvent&)>
            #endif
        ([](Containers::FunctionData& slot, const FocusEvent&) {
            static_cast<Containers::Function<void()>&>(slot)();
        })));
}

void EventLayer::remove(DataHandle handle) {
    AbstractLayer::remove(handle);
    removeInternal(dataHandleId(handle));
}

void EventLayer::remove(LayerDataHandle handle) {
    AbstractLayer::remove(handle);
    removeInternal(layerDataHandleId(handle));
}

void EventLayer::removeInternal(const UnsignedInt id) {
    State& state = *_state;
    Data& data = state.data[id];

    /* Set the slot to an empty instance to call any captured state
       destructors */
    data.slot = {};

    /* If the connection was scoped, decrement the counter. No need to reset
       the hasScopedConnection bit, as the data won't be touched again until
       a subsequent create() that overwrites it */
    if(data.hasScopedConnection)
        --_state->usedScopedConnectionCount;

    /* If data for which a gesture is tracked is removed, reset it */
    if(data.eventType == Implementation::EventType::Pinch && state.twoFingerGestureData == id) {
        state.twoFingerGestureData = ~UnsignedInt{};
        state.twoFingerGesture = Platform::TwoFingerGesture{};
    }
}

void EventLayer::doClean(const Containers::BitArrayView dataIdsToRemove) {
    /** @todo some way to iterate bits */
    for(std::size_t i = 0; i != dataIdsToRemove.size(); ++i) {
        if(!dataIdsToRemove[i]) continue;
        removeInternal(i);
    }
}

void EventLayer::doPointerPressEvent(const UnsignedInt dataId, PointerEvent& event) {
    State& state = *_state;
    Data& data = state.data[dataId];

    /* If there's a pinch, feed the gesture recognition */
    /** @todo what's the desired behavior when the underlying node position is
        animated? if the gesture would be recognized globally, only the final
        calculated position() could be offset appropriately to undo the offset
        from the animation, but here the inputs themselves will be offset... */
    if(data.eventType == Implementation::EventType::Pinch) {
        /* If it was previously happening on some other data, it means that the
           capture was removed for example due to a primary pointer release.
           Reset it in that case to prevent stale data from affecting the
           gesture. Do that only for a touch input tho, I feel it should still
           be possible to do a pinch gesture while clicking around with a mouse
           or a pen. */
        /** @todo this will be broken if there's more than one onPinch()
            attached to the same node -- needs the events to be fired for all
            data attached to given node, not for individual data (and
            remembering the node doesn't work because then it'd record the
            press / move multiple times instead of just once, being broken even
            more) */
        if(dataId != state.twoFingerGestureData && event.source() == PointerEventSource::Touch) {
            state.twoFingerGestureData = dataId;
            state.twoFingerGesture = Platform::TwoFingerGesture{};
        }

        /* If the event is actually used (i.e., it's a finger, etc.), mark the
           event as accepted so it doesn't fall through, causing some other
           node to be captured */
        if(state.twoFingerGesture.pressEvent(event))
            event.setAccepted();

        return;
    }

    /* All other cases are only for primary input events */
    if(!event.isPrimary())
        return;

    if(data.eventType == Implementation::EventType::Press &&
        event.pointer() & (Pointer::MouseLeft|Pointer::Finger|Pointer::Pen))
    {
        reinterpret_cast<void(*)(Containers::FunctionData&, const PointerEvent&)>(data.call)(data.slot, event);
        event.setAccepted();
        return;
    }

    /* Accept also a press of appropriate pointers that precede a tap/click,
       drag, focus, right click or middle click. Otherwise it could get
       propagated further, causing the subsequent release or move to get called
       on some entirely other node. */
    if(
        ((data.eventType == Implementation::EventType::TapOrClick ||
          data.eventType == Implementation::EventType::Drag ||
          data.eventType == Implementation::EventType::Focus) &&
            event.pointer() & (Pointer::MouseLeft|Pointer::Finger|Pointer::Pen)) ||
        (data.eventType == Implementation::EventType::MiddleClick &&
            event.pointer() == Pointer::MouseMiddle) ||
        (data.eventType == Implementation::EventType::RightClick &&
            event.pointer() == Pointer::MouseRight)
    )
        event.setAccepted();
}

void EventLayer::doPointerReleaseEvent(const UnsignedInt dataId, PointerEvent& event) {
    State& state = *_state;
    Data& data = state.data[dataId];

    /* If there's a pinch, feed the gesture recognition. Same logic as in
       doPointerPressEvent(), see above for details. */
    if(data.eventType == Implementation::EventType::Pinch) {
        if(dataId != state.twoFingerGestureData && event.source() == PointerEventSource::Touch) {
            state.twoFingerGestureData = dataId;
            state.twoFingerGesture = Platform::TwoFingerGesture{};
        }

        if(state.twoFingerGesture.releaseEvent(event))
            event.setAccepted();

        return;
    }

    /* All other cases are only for primary input events */
    if(!event.isPrimary())
        return;

    if(data.eventType == Implementation::EventType::Release &&
        event.pointer() & (Pointer::MouseLeft|Pointer::Finger|Pointer::Pen))
    {
        reinterpret_cast<void(*)(Containers::FunctionData&, const PointerEvent&)>(data.call)(data.slot, event);
        event.setAccepted();
        return;
    }

    /* Accept also a release of appropriate pointers that precede a tap/click,
       middle click or right click. Otherwise it could get propagated further,
       causing the subsequent tap/click to not get called at all. */
    if(
        (data.eventType == Implementation::EventType::TapOrClick &&
            event.pointer() & (Pointer::MouseLeft|Pointer::Finger|Pointer::Pen)) ||
        (data.eventType == Implementation::EventType::MiddleClick &&
            event.pointer() == Pointer::MouseMiddle) ||
        (data.eventType == Implementation::EventType::RightClick &&
            event.pointer() == Pointer::MouseRight)
    )
        event.setAccepted();
}

void EventLayer::doPointerTapOrClickEvent(const UnsignedInt dataId, PointerEvent& event) {
    /* event is guaranteed to be primary by AbstractLayer */

    Data& data = _state->data[dataId];
    if((data.eventType == Implementation::EventType::TapOrClick &&
            event.pointer() & (Pointer::MouseLeft|Pointer::Finger|Pointer::Pen)) ||
       (data.eventType == Implementation::EventType::MiddleClick &&
            event.pointer() == Pointer::MouseMiddle) ||
       (data.eventType == Implementation::EventType::RightClick &&
            event.pointer() == Pointer::MouseRight))
    {
        reinterpret_cast<void(*)(Containers::FunctionData&, const PointerEvent&)>(data.call)(data.slot, event);
        event.setAccepted();
    }
}

void EventLayer::doPointerMoveEvent(const UnsignedInt dataId, PointerMoveEvent& event) {
    State& state = *_state;
    Data& data = state.data[dataId];

    /* If there's a pinch, feed the gesture recognition. Same initial logic as
       in doPointerPressEvent() and doPointerReleaseEvent(), see above for
       details... */
    if(data.eventType == Implementation::EventType::Pinch) {
        if(dataId != state.twoFingerGestureData && event.source() == PointerEventSource::Touch) {
            state.twoFingerGestureData = dataId;
            state.twoFingerGesture = Platform::TwoFingerGesture{};
        }

        /* ... but afterwards, if this event was actually used and the gesture
           is recognized, we also call the slot. If the event wasn't used, the
           gesture could still be recognized, but it'd repeatedly give the same
           data unrelated to the actual input event, leading to a broken
           behavior. */
        if(state.twoFingerGesture.moveEvent(event)) {
            event.setAccepted();

            if(state.twoFingerGesture)
                reinterpret_cast<void(*)(Containers::FunctionData&, const Platform::TwoFingerGesture&)>(data.call)(data.slot, state.twoFingerGesture);
        }

        return;
    }

    /* All other cases are only for primary input events */
    if(!event.isPrimary())
        return;

    if(data.eventType == Implementation::EventType::Drag &&
        (event.pointers() & (Pointer::MouseLeft|Pointer::Finger|Pointer::Pen)) &&
        event.isCaptured())
    {
        reinterpret_cast<void(*)(Containers::FunctionData&, const PointerMoveEvent&)>(data.call)(data.slot, event);
        event.setAccepted();
    }

    /* Accept also a move that's needed in order to synthesize an enter/leave
       event */
    if(data.eventType == Implementation::EventType::Enter ||
       data.eventType == Implementation::EventType::Leave)
        event.setAccepted();
}

void EventLayer::doPointerEnterEvent(const UnsignedInt dataId, PointerMoveEvent& event) {
    /* event is guaranteed to be primary by AbstractLayer */

    Data& data = _state->data[dataId];
    if(data.eventType == Implementation::EventType::Enter) {
        reinterpret_cast<void(*)(Containers::FunctionData&, const PointerMoveEvent&)>(data.call)(data.slot, event);
        /* Accept status is ignored on enter/leave events, no need to call
           setAccepted() */
    }
}

void EventLayer::doPointerLeaveEvent(const UnsignedInt dataId, PointerMoveEvent& event) {
    /* event is guaranteed to be primary by AbstractLayer */

    Data& data = _state->data[dataId];
    if(data.eventType == Implementation::EventType::Leave) {
        reinterpret_cast<void(*)(Containers::FunctionData&, const PointerMoveEvent&)>(data.call)(data.slot, event);
        /* Accept status is ignored on enter/leave events, no need to call
           setAccepted() */
    }
}

void EventLayer::doFocusEvent(const UnsignedInt dataId, FocusEvent& event) {
    Data& data = _state->data[dataId];
    if(data.eventType == Implementation::EventType::Focus) {
        reinterpret_cast<void(*)(Containers::FunctionData&, const FocusEvent&)>(data.call)(data.slot, event);
        event.setAccepted();
    }
}

void EventLayer::doBlurEvent(const UnsignedInt dataId, FocusEvent& event) {
    Data& data = _state->data[dataId];
    if(data.eventType == Implementation::EventType::Blur) {
        reinterpret_cast<void(*)(Containers::FunctionData&, const FocusEvent&)>(data.call)(data.slot, event);
        /* Accept status is ignored on blur events, no need to call
           setAccepted() */
    }
}

void EventLayer::doVisibilityLostEvent(const UnsignedInt dataId, VisibilityLostEvent&) {
    State& state = *_state;
    Data& data = state.data[dataId];

    /* If visibility was lost on data for which a gesture is tracked, reset
       it */
    if(data.eventType == Implementation::EventType::Pinch && state.twoFingerGestureData == dataId) {
        state.twoFingerGestureData = ~UnsignedInt{};
        state.twoFingerGesture = Platform::TwoFingerGesture{};
    }
}

}}
