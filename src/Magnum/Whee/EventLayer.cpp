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

#include "EventLayer.h"

#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/BitArrayView.h>
#include <Corrade/Containers/GrowableArray.h>
#include <Corrade/Containers/Function.h>

#include "Magnum/Whee/Handle.h"
#include "Magnum/Whee/Event.h"

namespace Magnum { namespace Whee {

/* EventConnection converts DataHandle to LayerDataHandle by taking the lower
   32 bits. Check that the bit counts didn't get out of sync since that
   assumption. */
static_assert(Implementation::LayerDataHandleIdBits + Implementation::LayerDataHandleGenerationBits == 32, "EventConnection DataHandle to LayerDataHandle conversion needs an update");

namespace Implementation {
    enum class EventType: UnsignedByte {
        Press,
        TapOrClick,
        MiddleClick,
        RightClick,
        Drag
    };
}

namespace {

struct Data {
    /* If connection.call is nullptr, the data is among the free ones. This is
       used in the EventLayer::usedNonTrivialConnectionCount() query. */
    Containers::FunctionData slot;
    Implementation::EventType eventType;
    bool hasScopedConnection;
    /* 6+ bytes free */
};

}

struct EventLayer::State {
    Containers::Array<Data> data;

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
        "Whee::EventLayer:" << _state->usedScopedConnectionCount << "scoped connections already active, can't move", );
}

EventLayer::~EventLayer() {
    /* _state may be nullptr after a (destructive) move */
    CORRADE_ASSERT(!_state || !_state->usedScopedConnectionCount,
        "Whee::EventLayer: destructed with" << _state->usedScopedConnectionCount << "scoped connections still active", );

    /* Destructors on any state captured in slots are called automatically on
       the Array destruction */
}

EventLayer& EventLayer::operator=(EventLayer&& other) noexcept {
    /* either _state may be nullptr after a (destructive) move */
    CORRADE_ASSERT(!_state || !_state->usedScopedConnectionCount,
        "Whee::EventLayer:" << _state->usedScopedConnectionCount << "scoped connections already active in the moved-to object, can't move", *this);
    CORRADE_ASSERT(!other._state || !other._state->usedScopedConnectionCount,
        "Whee::EventLayer:" << other._state->usedScopedConnectionCount << "scoped connections already active in the moved-from object, can't move", *this);

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

DataHandle EventLayer::create(const NodeHandle node, const Implementation::EventType eventType, Containers::FunctionData&& slot) {
    Containers::Array<Data>& data = _state->data;
    const DataHandle handle = AbstractLayer::create(node);
    const UnsignedInt id = dataHandleId(handle);
    if(id >= data.size())
        /* Can't use arrayAppend(NoInit) because the Function has to be
           zero-initialized */
        /** @todo some arrayAppend(DefaultInit, std::size_t)? */
        arrayResize(data, id + 1);

    data[id].eventType = eventType;
    data[id].slot = Utility::move(slot);
    data[id].hasScopedConnection = false;
    return handle;
}

DataHandle EventLayer::onPress(const NodeHandle node, Containers::Function<void()>&& slot) {
    return create(node, Implementation::EventType::Press, Utility::move(slot));
}

DataHandle EventLayer::onTapOrClick(const NodeHandle node, Containers::Function<void()>&& slot) {
    return create(node, Implementation::EventType::TapOrClick, Utility::move(slot));
}

DataHandle EventLayer::onMiddleClick(const NodeHandle node, Containers::Function<void()>&& slot) {
    return create(node, Implementation::EventType::MiddleClick, Utility::move(slot));
}

DataHandle EventLayer::onRightClick(const NodeHandle node, Containers::Function<void()>&& slot) {
    return create(node, Implementation::EventType::RightClick, Utility::move(slot));
}

DataHandle EventLayer::onDrag(const NodeHandle node, Containers::Function<void(const Vector2&)>&& slot) {
    return create(node, Implementation::EventType::Drag, Utility::move(slot));
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
    Data& data = _state->data[id];

    /* Set the slot to an empty instance to call any captured state
       destructors */
    data.slot = {};

    /* If the connection was scoped, decrement the counter. No need to reset
       the hasScopedConnection bit, as the data won't be touched again until
       a subsequent create() that overwrites it */
    if(data.hasScopedConnection)
        --_state->usedScopedConnectionCount;
}

void EventLayer::doClean(const Containers::BitArrayView dataIdsToRemove) {
    /** @todo some way to iterate bits */
    for(std::size_t i = 0; i != dataIdsToRemove.size(); ++i) {
        if(!dataIdsToRemove[i]) continue;
        removeInternal(i);
    }
}

void EventLayer::doPointerPressEvent(const UnsignedInt dataId, PointerEvent& event) {
    Data& data = _state->data[dataId];
    if(data.eventType == Implementation::EventType::Press && (event.type() == Pointer::MouseLeft || event.type() == Pointer::Finger || event.type() == Pointer::Pen)) {
        static_cast<Containers::Function<void()>&>(data.slot)();
        event.setAccepted();
    }
}

void EventLayer::doPointerTapOrClickEvent(const UnsignedInt dataId, PointerEvent& event) {
    Data& data = _state->data[dataId];
    if((data.eventType == Implementation::EventType::TapOrClick &&
            (event.type() == Pointer::MouseLeft ||
             event.type() == Pointer::Finger ||
             event.type() == Pointer::Pen)) ||
       (data.eventType == Implementation::EventType::MiddleClick &&
            event.type() == Pointer::MouseMiddle) ||
       (data.eventType == Implementation::EventType::RightClick &&
            event.type() == Pointer::MouseRight))
    {
        static_cast<Containers::Function<void()>&>(data.slot)();
        event.setAccepted();
    }
}

void EventLayer::doPointerMoveEvent(const UnsignedInt dataId, PointerMoveEvent& event) {
    Data& data = _state->data[dataId];
    if(data.eventType == Implementation::EventType::Drag &&
        (event.types() & (Pointer::MouseLeft|Pointer::Finger|Pointer::Pen)))
    {
        /** @todo restrict this to a drag that actually started on the node?
            actually with capture this is implicit, so maybe just ignore the
            case when capture is not there? */
        static_cast<Containers::Function<void(const Vector2&)>&>(data.slot)(event.relativePosition());
        event.setAccepted();
    }
}

}}
