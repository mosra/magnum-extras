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
#include <Corrade/Containers/GrowableArray.h>
#include <Corrade/Containers/BitArrayView.h>

#include "Magnum/Whee/Handle.h"
#include "Magnum/Whee/Event.h"

namespace Magnum { namespace Whee {

/* EventConnection converts DataHandle to LayerDataHandle by taking the lower
   32 bits. Check that the bit counts didn't get out of sync since that
   assumption. */
static_assert(Implementation::LayerDataHandleIdBits + Implementation::LayerDataHandleGenerationBits == 32, "EventConnection DataHandle to LayerDataHandle conversion needs an update");

namespace {

struct Data {
    /* If connection.call is nullptr, the data is among the free ones. This is
       used in the EventLayer::usedNonTrivialConnectionCount() query. */
    Implementation::EventConnectionData connection;
    Implementation::EventType eventType;
    bool hasDestructor;
    bool hasScopedConnection;
    /* 5+ bytes free */
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

    /* Call destructors on connections that aren't free (i.e., have non-null
       call) and need it */
    if(_state) for(Data& data: _state->data) {
        if(data.connection.call && data.hasDestructor)
            data.connection.storage.functor.destruct(data.connection.storage);
    }
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

UnsignedInt EventLayer::usedNonTrivialConnectionCount() const {
    UnsignedInt count = 0;
    for(const Data& data: _state->data) {
        /* Entries with call being nullptr are free */
        if(data.connection.call && data.hasDestructor)
            ++count;
    }

    return count;
}

DataHandle EventLayer::create(const NodeHandle node, const Implementation::EventType eventType, bool hasDestructor, Implementation::EventConnectionData*& connectionData) {
    Containers::Array<Data>& data = _state->data;
    const DataHandle handle = AbstractLayer::create(node);
    const UnsignedInt id = dataHandleId(handle);
    if(id >= data.size())
        arrayAppend(data, NoInit, id - data.size() + 1);

    data[id].eventType = eventType;
    data[id].hasDestructor = hasDestructor;
    connectionData = &data[id].connection;
    data[id].hasScopedConnection = false;
    return handle;
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
    /* If the connection has a destructor, call it */
    Data& data = _state->data[id];
    if(data.hasDestructor)
        data.connection.storage.functor.destruct(data.connection.storage);

    /* If the connection was scoped, decrement the counter. No need to reset
       the hasScopedConnection bit, as the data won't be touched again until
       a subsequent create() that overwrites it */
    if(data.hasScopedConnection)
        --_state->usedScopedConnectionCount;

    /* Set the call pointer to null to be able to distinguish between used and
       free ones in usedNonTrivialConnectionCount() */
    data.connection.call = nullptr;
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
        reinterpret_cast<void(*)(Implementation::EventConnectionData::Storage&)>(data.connection.call)(data.connection.storage);
        event.setAccepted();
    }
}

void EventLayer::doPointerTapOrClickEvent(const UnsignedInt dataId, PointerEvent& event) {
    Data& data = _state->data[dataId];
    if((data.eventType == Implementation::EventType::TapOrClick && (event.type() == Pointer::MouseLeft || event.type() == Pointer::Finger || event.type() == Pointer::Pen)) ||
       (data.eventType == Implementation::EventType::MiddleClick && event.type() == Pointer::MouseMiddle) ||
       (data.eventType == Implementation::EventType::RightClick && event.type() == Pointer::MouseRight))
    {
        reinterpret_cast<void(*)(Implementation::EventConnectionData::Storage&)>(data.connection.call)(data.connection.storage);
        event.setAccepted();
    }
}

void EventLayer::doPointerMoveEvent(const UnsignedInt dataId, PointerMoveEvent& event) {
    Data& data = _state->data[dataId];
    if(data.eventType == Implementation::EventType::Drag &&             (event.types() & (Pointer::MouseLeft|Pointer::Finger|Pointer::Pen))) {
        /** @todo restrict this to a drag that actually started on the node?
            actually with capture this is implicit, so maybe just ignore the
            case when capture is not there? */
        reinterpret_cast<void(*)(Implementation::EventConnectionData::Storage&, const Vector2&)>(data.connection.call)(data.connection.storage, event.relativePosition());
        event.setAccepted();
    }
}

}}
