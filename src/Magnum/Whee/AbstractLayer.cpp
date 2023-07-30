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

#include "AbstractLayer.h"

#include <Corrade/Containers/BitArrayView.h>
#include <Corrade/Containers/EnumSet.hpp>
#include <Corrade/Containers/GrowableArray.h>
#include <Corrade/Containers/StridedArrayView.h>

#include "Magnum/Whee/Event.h"
#include "Magnum/Whee/Handle.h"

namespace Magnum { namespace Whee {

Debug& operator<<(Debug& debug, const LayerFeature value) {
    debug << "Whee::LayerFeature" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case LayerFeature::value: return debug << "::" #value;
        _c(Draw)
        _c(Event)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "(" << Debug::nospace << Debug::hex << UnsignedByte(value) << Debug::nospace << ")";
}

Debug& operator<<(Debug& debug, const LayerFeatures value) {
    return Containers::enumSetDebugOutput(debug, value, "Whee::LayerFeatures{}", {
        LayerFeature::Draw,
        LayerFeature::Event
    });
}

Debug& operator<<(Debug& debug, const LayerState value) {
    debug << "Whee::LayerState" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case LayerState::value: return debug << "::" #value;
        _c(NeedsUpdate)
        _c(NeedsClean)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "(" << Debug::nospace << Debug::hex << UnsignedByte(value) << Debug::nospace << ")";
}

Debug& operator<<(Debug& debug, const LayerStates value) {
    return Containers::enumSetDebugOutput(debug, value, "Whee::LayerStates{}", {
        LayerState::NeedsUpdate,
        LayerState::NeedsClean
    });
}

namespace {

union Data {
    explicit Data() noexcept: used{} {}

    struct Used {
        /* Together with index of this item in `data` used for creating a
           LayerDataHandle. Increased every time a handle reaches remove(). Has
           to be initially non-zero to differentiate the first ever handle
           (with index 0) from LayerDataHandle::Null. Once becomes
           `1 << LayerDataHandleGenerationBits` the handle gets disabled. */
        UnsignedShort generation = 1;

        /* Six bytes free */
    } used;

    /* Used only if the Data is among free ones */
    struct Free {
        /* The generation value has to be preserved in order to increment it
           next time it gets used */
        UnsignedShort generation;

        /* See State::firstFree for more information */
        UnsignedInt next;
    } free;
};

#ifndef CORRADE_NO_STD_IS_TRIVIALLY_TRAITS
static_assert(std::is_trivially_copyable<Data>::value, "Data not trivially copyable");
#endif
static_assert(
    offsetof(Data::Used, generation) == offsetof(Data::Free, generation),
    "Data::Used and Free layout not compatible");

}

struct AbstractLayer::State {
    LayerHandle handle;
    LayerStates state;

    /* 1 / 5 bytes free */

    Containers::Array<Data> data;
    /* Indices in the data array. The Data then has a nextFree member
       containing the next free index. New data get taken from the front,
       removed are put at the end. A value of ~UnsignedInt{} means there's no
       (first/next/last) free data. */
    UnsignedInt firstFree = ~UnsignedInt{};
    UnsignedInt lastFree = ~UnsignedInt{};
};

AbstractLayer::AbstractLayer(const LayerHandle handle): _state{InPlaceInit} {
    CORRADE_ASSERT(handle != LayerHandle::Null,
        "Whee::AbstractLayer: handle is null", );
    _state->handle = handle;
}

AbstractLayer::AbstractLayer(AbstractLayer&&) noexcept = default;

AbstractLayer::~AbstractLayer() = default;

AbstractLayer& AbstractLayer::operator=(AbstractLayer&&) noexcept = default;

LayerHandle AbstractLayer::handle() const {
    return _state->handle;
}

LayerStates AbstractLayer::state() const {
    return _state->state;
}

void AbstractLayer::setNeedsUpdate() {
    _state->state |= LayerState::NeedsUpdate;
}

std::size_t AbstractLayer::capacity() const {
    return _state->data.size();
}

std::size_t AbstractLayer::usedCount() const {
    std::size_t free = 0;
    UnsignedInt index = _state->firstFree;
    while(index != ~UnsignedInt{}) {
        index = _state->data[index].free.next;
        ++free;
    }
    return _state->data.size() - free;
}

bool AbstractLayer::isHandleValid(const LayerDataHandle handle) const {
    if(handle == LayerDataHandle::Null)
        return false;
    const UnsignedInt index = layerDataHandleId(handle);
    if(index >= _state->data.size())
        return false;
    /* Unlike UserInterface::isHandleValid(LayerHandle), the generation counter
       here is 16bit and a disabled handle is signalized by 0x10000, not 0, so
       for disabled handles this will always fail without having to do any
       extra checks.

       Note that this can still return true for manually crafted handles that
       point to free data with correct generation counters. The only way to
       detect that would be by either iterating the free list (slow) or by
       keeping an additional bitfield marking free items. I don't think that's
       necessary. */
    return layerDataHandleGeneration(handle) == _state->data[index].used.generation;
}

bool AbstractLayer::isHandleValid(const DataHandle handle) const {
    return dataHandleLayer(handle) == _state->handle && isHandleValid(dataHandleData(handle));
}

DataHandle AbstractLayer::create() {
    /* Find the first free data if there is, update the free index to point to
       the next one (or none) */
    Data* data;
    if(_state->firstFree!= ~UnsignedInt{}) {
        data = &_state->data[_state->firstFree];

        if(_state->firstFree == _state->lastFree) {
            CORRADE_INTERNAL_ASSERT(data->free.next == ~UnsignedInt{});
            _state->firstFree = _state->lastFree = ~UnsignedInt{};
        } else {
            _state->firstFree = data->free.next;
        }

    /* If there isn't, allocate a new one */
    } else {
        CORRADE_ASSERT(_state->data.size() < 1 << Implementation::LayerDataHandleIdBits,
            "Whee::AbstractLayer::create(): can only have at most" << (1 << Implementation::LayerDataHandleIdBits) << "data", {});
        data = &arrayAppend(_state->data, InPlaceInit);
    }

    /* Fill the data. In both above cases the generation is already set
       appropriately, either initialized to 1, or incremented when it got
       remove()d (to mark existing handles as invalid) */
    return dataHandle(_state->handle, (data - _state->data), data->used.generation);
}

void AbstractLayer::remove(const DataHandle handle) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::AbstractLayer::remove(): invalid handle" << handle, );
    /* Doesn't delegate to remove(LayerDataHandle) to avoid a double check;
       doesn't check just the layer portion of the handle and delegate to avoid
       a confusing assertion message if the data portion would be invalid */
    removeInternal(dataHandleId(handle));

    /* Mark the layer as needing a clean() call to refresh its state, which
       also bubbles up to the UI itself */
    _state->state |= LayerState::NeedsClean;
}

void AbstractLayer::remove(const LayerDataHandle handle) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::AbstractLayer::remove(): invalid handle" << handle, );
    removeInternal(layerDataHandleId(handle));

    /* Mark the layer as needing a clean() call to refresh its state, which
       also bubbles up to the UI itself */
    _state->state |= LayerState::NeedsClean;
}

void AbstractLayer::removeInternal(const UnsignedInt id) {
    Data& data = _state->data[id];

    /* Increase the data generation so existing handles pointing to this data
       are invalidated */
    ++data.used.generation;

    /* Put the data at the end of the free list (while they're allocated from
       the front) to not exhaust the generation counter too fast. If the free
       list is empty however, update also the index of the first free layer.

       Don't do this if the generation wrapped around. That makes it disabled,
       i.e. impossible to be recycled later, to avoid aliasing old handles. */
    if(data.used.generation != 1 << Implementation::LayerDataHandleGenerationBits) {
        data.free.next = ~UnsignedInt{};
        if(_state->lastFree == ~UnsignedInt{}) {
            CORRADE_INTERNAL_ASSERT(
                _state->firstFree == ~UnsignedInt{} &&
                _state->lastFree == ~UnsignedInt{});
            _state->firstFree = id;
        } else {
            _state->data[_state->lastFree].free.next = id;
        }
        _state->lastFree = id;
    }

    /* Updating LayerState (or not) is caller's responsibility. For example,
       clean() below *unsets* NeedsClean instead of setting it. */
}

void AbstractLayer::clean(const Containers::BitArrayView dataIdsToRemove) {
    CORRADE_ASSERT(dataIdsToRemove.size() == _state->data.size(),
        "Whee::AbstractLayer::clean(): expected" << _state->data.size() << "bits but got" << dataIdsToRemove.size(), );
    /** @todo some way to efficiently iterate set bits */
    for(std::size_t i = 0; i != dataIdsToRemove.size(); ++i) {
        if(dataIdsToRemove[i]) removeInternal(i);
    }
    doClean(dataIdsToRemove);
    /* Unmark the UI as needing a clean() call. Unlike in UserInterface, the
       NeedsUpdate state is independent of this flag. */
    _state->state &= ~LayerState::NeedsClean;
}

void AbstractLayer::doClean(Containers::BitArrayView) {}

void AbstractLayer::update(const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const Containers::StridedArrayView1D<const UnsignedInt>& dataNodeIds, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes) {
    CORRADE_ASSERT(dataIds.size() == dataNodeIds.size(),
        "Whee::AbstractLayer::update(): expected data and node ID views to have the same size but got" << dataIds.size() << "and" << dataNodeIds.size(), );
    CORRADE_ASSERT(nodeOffsets.size() == nodeSizes.size(),
        "Whee::AbstractLayer::update(): expected node offset and size views to have the same size but got" << nodeOffsets.size() << "and" << nodeSizes.size(), );
    doUpdate(dataIds, dataNodeIds, nodeOffsets, nodeSizes);
    _state->state &= ~LayerState::NeedsUpdate;
}

void AbstractLayer::doUpdate(const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&) {}

void AbstractLayer::draw(const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const Containers::StridedArrayView1D<const UnsignedInt>& dataNodeIds, const std::size_t offset, const std::size_t count, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes) {
    CORRADE_ASSERT(features() & LayerFeature::Draw,
        "Whee::AbstractLayer::draw(): feature not supported", );
    CORRADE_ASSERT(dataIds.size() == dataNodeIds.size(),
        "Whee::AbstractLayer::draw(): expected data and node ID views to have the same size but got" << dataIds.size() << "and" << dataNodeIds.size(), );
    CORRADE_ASSERT(offset + count <= dataIds.size(),
        "Whee::AbstractLayer::draw(): offset" << offset << "and count" << count << "out of range for" << dataIds.size() << "items", );
    CORRADE_ASSERT(nodeOffsets.size() == nodeSizes.size(),
        "Whee::AbstractLayer::draw(): expected node offset and size views to have the same size but got" << nodeOffsets.size() << "and" << nodeSizes.size(), );
    doDraw(dataIds, dataNodeIds, offset, count, nodeOffsets, nodeSizes);
}

void AbstractLayer::doDraw(const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const UnsignedInt>&, std::size_t, std::size_t, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&) {
    CORRADE_ASSERT_UNREACHABLE("Whee::AbstractLayer::draw(): feature advertised but not implemented", );
}

void AbstractLayer::pointerPressEvent(const UnsignedInt dataId, PointerEvent& event) {
    CORRADE_ASSERT(features() & LayerFeature::Event,
        "Whee::AbstractLayer::pointerPressEvent(): feature not supported", );
    CORRADE_ASSERT(dataId < _state->data.size(),
        "Whee::AbstractLayer::pointerPressEvent(): index" << dataId << "out of range for" << _state->data.size() << "data", );
    CORRADE_ASSERT(!event.isAccepted(),
        "Whee::AbstractLayer::pointerPressEvent(): event already accepted", );
    return doPointerPressEvent(dataId, event);
}

void AbstractLayer::doPointerPressEvent(UnsignedInt, PointerEvent&) {}

void AbstractLayer::pointerReleaseEvent(const UnsignedInt dataId, PointerEvent& event) {
    CORRADE_ASSERT(features() & LayerFeature::Event,
        "Whee::AbstractLayer::pointerReleaseEvent(): feature not supported", );
    CORRADE_ASSERT(dataId < _state->data.size(),
        "Whee::AbstractLayer::pointerReleaseEvent(): index" << dataId << "out of range for" << _state->data.size() << "data", );
    CORRADE_ASSERT(!event.isAccepted(),
        "Whee::AbstractLayer::pointerReleaseEvent(): event already accepted", );
    return doPointerReleaseEvent(dataId, event);
}

void AbstractLayer::doPointerReleaseEvent(UnsignedInt, PointerEvent&) {}

void AbstractLayer::pointerMoveEvent(const UnsignedInt dataId, PointerMoveEvent& event) {
    CORRADE_ASSERT(features() & LayerFeature::Event,
        "Whee::AbstractLayer::pointerMoveEvent(): feature not supported", );
    CORRADE_ASSERT(dataId < _state->data.size(),
        "Whee::AbstractLayer::pointerMoveEvent(): index" << dataId << "out of range for" << _state->data.size() << "data", );
    CORRADE_ASSERT(!event.isAccepted(),
        "Whee::AbstractLayer::pointerMoveEvent(): event already accepted", );
    return doPointerMoveEvent(dataId, event);
}

void AbstractLayer::doPointerMoveEvent(UnsignedInt, PointerMoveEvent&) {}

void AbstractLayer::pointerEnterEvent(const UnsignedInt dataId, PointerMoveEvent& event) {
    CORRADE_ASSERT(features() & LayerFeature::Event,
        "Whee::AbstractLayer::pointerEnterEvent(): feature not supported", );
    CORRADE_ASSERT(dataId < _state->data.size(),
        "Whee::AbstractLayer::pointerEnterEvent(): index" << dataId << "out of range for" << _state->data.size() << "data", );
    CORRADE_ASSERT(!event.isAccepted(),
        "Whee::AbstractLayer::pointerEnterEvent(): event already accepted", );
    /* This isn't triggerable from public code so can be an internal assert,
       verifying just that the UserInterface internals don't mess up */
    CORRADE_INTERNAL_ASSERT(event.relativePosition().isZero());
    return doPointerEnterEvent(dataId, event);
}

void AbstractLayer::doPointerEnterEvent(UnsignedInt, PointerMoveEvent&) {}

void AbstractLayer::pointerLeaveEvent(const UnsignedInt dataId, PointerMoveEvent& event) {
    CORRADE_ASSERT(features() & LayerFeature::Event,
        "Whee::AbstractLayer::pointerLeaveEvent(): feature not supported", );
    CORRADE_ASSERT(dataId < _state->data.size(),
        "Whee::AbstractLayer::pointerLeaveEvent(): index" << dataId << "out of range for" << _state->data.size() << "data", );
    CORRADE_ASSERT(!event.isAccepted(),
        "Whee::AbstractLayer::pointerLeaveEvent(): event already accepted", );
    /* This isn't triggerable from public code so can be an internal assert,
       verifying just that the UserInterface internals don't mess up */
    CORRADE_INTERNAL_ASSERT(event.relativePosition().isZero());
    return doPointerLeaveEvent(dataId, event);
}

void AbstractLayer::doPointerLeaveEvent(UnsignedInt, PointerMoveEvent&) {}

}}
