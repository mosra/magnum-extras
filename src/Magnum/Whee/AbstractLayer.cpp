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

#include <Corrade/Containers/BitArray.h>
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
        _c(NeedsAttachmentUpdate)
        _c(NeedsClean)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "(" << Debug::nospace << Debug::hex << UnsignedByte(value) << Debug::nospace << ")";
}

Debug& operator<<(Debug& debug, const LayerStates value) {
    return Containers::enumSetDebugOutput(debug, value, "Whee::LayerStates{}", {
        LayerState::NeedsAttachmentUpdate,
        /* Implied by NeedsAttachmentUpdate, has to be after */
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

        /* Two bytes free */

        /* Node the data is attached to. Becomes null again when the data is
           freed. Has to be re-filled every time a handle is recycled, so it
           doesn't make sense to initialize it to anything. */
        NodeHandle node;

        /* Four bytes free */
    } used;

    /* Used only if the Data is among free ones */
    struct Free {
        /* The generation value has to be preserved in order to increment it
           next time it gets used */
        UnsignedShort generation;

        /* The node field is needed to discard free items when directly
           iterating the list. */
        /** @todo any idea how to better pack this? this is a bit awful */
        NodeHandle node;

        /* See State::firstFree for more information */
        UnsignedInt next;
    } free;
};

#ifndef CORRADE_NO_STD_IS_TRIVIALLY_TRAITS
static_assert(std::is_trivially_copyable<Data>::value, "Data not trivially copyable");
#endif
static_assert(
    offsetof(Data::Used, generation) == offsetof(Data::Free, generation) &&
    offsetof(Data::Used, node) == offsetof(Data::Free, node),
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
    const State& state = *_state;
    std::size_t free = 0;
    UnsignedInt index = state.firstFree;
    while(index != ~UnsignedInt{}) {
        index = state.data[index].free.next;
        ++free;
    }
    return state.data.size() - free;
}

bool AbstractLayer::isHandleValid(const LayerDataHandle handle) const {
    if(handle == LayerDataHandle::Null)
        return false;
    const State& state = *_state;
    const UnsignedInt index = layerDataHandleId(handle);
    if(index >= state.data.size())
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
    return layerDataHandleGeneration(handle) == state.data[index].used.generation;
}

bool AbstractLayer::isHandleValid(const DataHandle handle) const {
    return dataHandleLayer(handle) == _state->handle && isHandleValid(dataHandleData(handle));
}

DataHandle AbstractLayer::create() {
    State& state = *_state;

    /* Find the first free data if there is, update the free index to point to
       the next one (or none) */
    Data* data;
    if(state.firstFree!= ~UnsignedInt{}) {
        data = &state.data[state.firstFree];

        if(state.firstFree == state.lastFree) {
            CORRADE_INTERNAL_ASSERT(data->free.next == ~UnsignedInt{});
            state.firstFree = state.lastFree = ~UnsignedInt{};
        } else {
            state.firstFree = data->free.next;
        }

    /* If there isn't, allocate a new one */
    } else {
        CORRADE_ASSERT(state.data.size() < 1 << Implementation::LayerDataHandleIdBits,
            "Whee::AbstractLayer::create(): can only have at most" << (1 << Implementation::LayerDataHandleIdBits) << "data", {});
        data = &arrayAppend(state.data, InPlaceInit);
    }

    /* Fill the data. In both above cases the generation is already set
       appropriately, either initialized to 1, or incremented when it got
       remove()d (to mark existing handles as invalid) */
    return dataHandle(state.handle, (data - state.data), data->used.generation);
}

void AbstractLayer::remove(const DataHandle handle) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::AbstractLayer::remove(): invalid handle" << handle, );

    State& state = *_state;
    state.state |= LayerState::NeedsClean;
    /* If the data was attached to a node, mark the layer also as needing a
       update() call to refresh node data attachment state, which also bubbles
       up to the UI itself */
    if(state.data[dataHandleId(handle)].used.node != NodeHandle::Null)
        state.state |= LayerState::NeedsAttachmentUpdate;

    /* Doesn't delegate to remove(LayerDataHandle) to avoid a double check;
       doesn't check just the layer portion of the handle and delegate to avoid
       a confusing assertion message if the data portion would be invalid */
    removeInternal(dataHandleId(handle));
}

void AbstractLayer::remove(const LayerDataHandle handle) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::AbstractLayer::remove(): invalid handle" << handle, );

    State& state = *_state;
    state.state |= LayerState::NeedsClean;
    /* If the data was attached to a node, mark the layer also as needing a
       update() call to refresh node data attachment state, which also bubbles
       up to the UI itself */
    if(state.data[layerDataHandleId(handle)].used.node != NodeHandle::Null)
        state.state |= LayerState::NeedsAttachmentUpdate;

    removeInternal(layerDataHandleId(handle));
}

void AbstractLayer::removeInternal(const UnsignedInt id) {
    State& state = *_state;
    Data& data = state.data[id];

    /* Increase the data generation so existing handles pointing to this data
       are invalidated */
    ++data.used.generation;

    /* Set the node attachment to null to avoid falsely recognizing this item
       as used when directly iterating the list */
    data.used.node = NodeHandle::Null;

    /* Put the data at the end of the free list (while they're allocated from
       the front) to not exhaust the generation counter too fast. If the free
       list is empty however, update also the index of the first free layer.

       Don't do this if the generation wrapped around. That makes it disabled,
       i.e. impossible to be recycled later, to avoid aliasing old handles. */
    if(data.used.generation != 1 << Implementation::LayerDataHandleGenerationBits) {
        data.free.next = ~UnsignedInt{};
        if(state.lastFree == ~UnsignedInt{}) {
            CORRADE_INTERNAL_ASSERT(
                state.firstFree == ~UnsignedInt{} &&
                state.lastFree == ~UnsignedInt{});
            state.firstFree = id;
        } else {
            state.data[state.lastFree].free.next = id;
        }
        state.lastFree = id;
    }

    /* Updating LayerState (or not) is caller's responsibility. For example,
       clean() below *unsets* NeedsClean instead of setting it. */
}

void AbstractLayer::attach(DataHandle data, NodeHandle node) {
    CORRADE_ASSERT(isHandleValid(data),
        "Whee::AbstractLayer::attach(): invalid handle" << data, );
    attachInternal(dataHandleId(data), node);
}

void AbstractLayer::attach(LayerDataHandle data, NodeHandle node) {
    CORRADE_ASSERT(isHandleValid(data),
        "Whee::AbstractLayer::attach(): invalid handle" << data, );
    attachInternal(layerDataHandleId(data), node);
}

void AbstractLayer::attachInternal(const UnsignedInt id, const NodeHandle node) {
    State& state = *_state;
    state.data[id].used.node = node;
    state.state |= LayerState::NeedsAttachmentUpdate;
}

NodeHandle AbstractLayer::node(DataHandle data) const {
    CORRADE_ASSERT(isHandleValid(data),
        "Whee::AbstractLayer::node(): invalid handle" << data, {});
    return _state->data[dataHandleId(data)].used.node;
}

NodeHandle AbstractLayer::node(LayerDataHandle data) const {
    CORRADE_ASSERT(isHandleValid(data),
        "Whee::AbstractLayer::node(): invalid handle" << data, {});
    return _state->data[layerDataHandleId(data)].used.node;
}

Containers::StridedArrayView1D<const UnsignedShort> AbstractLayer::generations() const {
    return stridedArrayView(_state->data).slice(&Data::used).slice(&Data::Used::generation);
}

Containers::StridedArrayView1D<const NodeHandle> AbstractLayer::nodes() const {
    return stridedArrayView(_state->data).slice(&Data::used).slice(&Data::Used::node);
}

void AbstractLayer::setSize(const Vector2& size, const Vector2i& framebufferSize) {
    CORRADE_ASSERT(features() & LayerFeature::Draw,
        "Whee::AbstractLayer::setSize():" << LayerFeature::Draw << "not supported", );
    CORRADE_ASSERT(size.product() && framebufferSize.product(),
        "Whee::AbstractLayer::setSize(): expected non-zero sizes, got" << size << "and" << framebufferSize, );
    doSetSize(size, framebufferSize);
}

void AbstractLayer::doSetSize(const Vector2&, const Vector2i&) {}

void AbstractLayer::cleanNodes(const Containers::StridedArrayView1D<const UnsignedShort>& nodeHandleGenerations) {
    State& state = *_state;
    /** @todo have some bump allocator for this */
    Containers::BitArray dataIdsToRemove{ValueInit, state.data.size()};

    for(std::size_t i = 0; i != state.data.size(); ++i) {
        const Data& data = state.data[i];

        /* Skip data that are free or that aren't attached to any node */
        if(data.used.node == NodeHandle::Null)
            continue;

        /* For used & attached data compare the generation of the node they're
           attached to. If it differs, remove the data and mark the
           corresponding index so the implementation can do its own cleanup in
           doClean(). */
        /** @todo check that the ID is in bounds and if it's not, remove as
            well? to avoid OOB access if the data is accidentally attached to a
            NodeHandle from a different UI instance that has more nodes */
        if(nodeHandleGeneration(data.used.node) != nodeHandleGenerations[nodeHandleId(data.used.node)]) {
            removeInternal(i);
            dataIdsToRemove.set(i);
        }
    }

    doClean(dataIdsToRemove);
    /* Unmark the UI as needing a clean() call. Unlike in UserInterface, the
       NeedsUpdate state is independent of this flag. */
    state.state &= ~LayerState::NeedsClean;
}

void AbstractLayer::doClean(Containers::BitArrayView) {}

void AbstractLayer::update(const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes) {
    CORRADE_ASSERT(clipRectIds.size() == clipRectDataCounts.size(),
        "Whee::AbstractLayer::update(): expected clip rect ID and data count views to have the same size but got" << clipRectIds.size() << "and" << clipRectDataCounts.size(), );
    CORRADE_ASSERT(nodeOffsets.size() == nodeSizes.size(),
        "Whee::AbstractLayer::update(): expected node offset and size views to have the same size but got" << nodeOffsets.size() << "and" << nodeSizes.size(), );
    CORRADE_ASSERT(clipRectOffsets.size() == clipRectSizes.size(),
        "Whee::AbstractLayer::update(): expected clip rect offset and size views to have the same size but got" << clipRectOffsets.size() << "and" << clipRectSizes.size(), );
    doUpdate(dataIds, clipRectIds, clipRectDataCounts, nodeOffsets, nodeSizes, clipRectOffsets, clipRectSizes);
    _state->state &= ~LayerState::NeedsAttachmentUpdate;
}

void AbstractLayer::doUpdate(const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&) {}

void AbstractLayer::draw(const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const std::size_t offset, const std::size_t count, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, const std::size_t clipRectOffset, const std::size_t clipRectCount, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes) {
    CORRADE_ASSERT(features() & LayerFeature::Draw,
        "Whee::AbstractLayer::draw(): feature not supported", );
    CORRADE_ASSERT(offset + count <= dataIds.size(),
        "Whee::AbstractLayer::draw(): offset" << offset << "and count" << count << "out of range for" << dataIds.size() << "items", );
    CORRADE_ASSERT(clipRectIds.size() == clipRectDataCounts.size(),
        "Whee::AbstractLayer::draw(): expected clip rect ID and data count views to have the same size but got" << clipRectIds.size() << "and" << clipRectDataCounts.size(), );
    CORRADE_ASSERT(clipRectOffset + clipRectCount <= clipRectIds.size(),
        "Whee::AbstractLayer::draw(): clip rect offset" << clipRectOffset << "and count" << clipRectCount << "out of range for" << clipRectIds.size() << "items", );
    CORRADE_ASSERT(nodeOffsets.size() == nodeSizes.size(),
        "Whee::AbstractLayer::draw(): expected node offset and size views to have the same size but got" << nodeOffsets.size() << "and" << nodeSizes.size(), );
    CORRADE_ASSERT(clipRectOffsets.size() == clipRectSizes.size(),
        "Whee::AbstractLayer::draw(): expected clip rect offset and size views to have the same size but got" << clipRectOffsets.size() << "and" << clipRectSizes.size(), );
    doDraw(dataIds, offset, count, clipRectIds, clipRectDataCounts, clipRectOffset, clipRectCount, nodeOffsets, nodeSizes, clipRectOffsets, clipRectSizes);
}

void AbstractLayer::doDraw(const Containers::StridedArrayView1D<const UnsignedInt>&, std::size_t, std::size_t, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const UnsignedInt>&, std::size_t, std::size_t, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&) {
    CORRADE_ASSERT_UNREACHABLE("Whee::AbstractLayer::draw(): feature advertised but not implemented", );
}

void AbstractLayer::pointerPressEvent(const UnsignedInt dataId, PointerEvent& event) {
    CORRADE_ASSERT(features() & LayerFeature::Event,
        "Whee::AbstractLayer::pointerPressEvent(): feature not supported", );
    #ifndef CORRADE_NO_ASSERT
    const State& state = *_state;
    #endif
    CORRADE_ASSERT(dataId < state.data.size(),
        "Whee::AbstractLayer::pointerPressEvent(): index" << dataId << "out of range for" << state.data.size() << "data", );
    CORRADE_ASSERT(!event.isAccepted(),
        "Whee::AbstractLayer::pointerPressEvent(): event already accepted", );
    return doPointerPressEvent(dataId, event);
}

void AbstractLayer::doPointerPressEvent(UnsignedInt, PointerEvent&) {}

void AbstractLayer::pointerReleaseEvent(const UnsignedInt dataId, PointerEvent& event) {
    CORRADE_ASSERT(features() & LayerFeature::Event,
        "Whee::AbstractLayer::pointerReleaseEvent(): feature not supported", );
    #ifndef CORRADE_NO_ASSERT
    const State& state = *_state;
    #endif
    CORRADE_ASSERT(dataId < state.data.size(),
        "Whee::AbstractLayer::pointerReleaseEvent(): index" << dataId << "out of range for" << state.data.size() << "data", );
    CORRADE_ASSERT(!event.isAccepted(),
        "Whee::AbstractLayer::pointerReleaseEvent(): event already accepted", );
    return doPointerReleaseEvent(dataId, event);
}

void AbstractLayer::doPointerReleaseEvent(UnsignedInt, PointerEvent&) {}

void AbstractLayer::pointerMoveEvent(const UnsignedInt dataId, PointerMoveEvent& event) {
    CORRADE_ASSERT(features() & LayerFeature::Event,
        "Whee::AbstractLayer::pointerMoveEvent(): feature not supported", );
    #ifndef CORRADE_NO_ASSERT
    const State& state = *_state;
    #endif
    CORRADE_ASSERT(dataId < state.data.size(),
        "Whee::AbstractLayer::pointerMoveEvent(): index" << dataId << "out of range for" << state.data.size() << "data", );
    CORRADE_ASSERT(!event.isAccepted(),
        "Whee::AbstractLayer::pointerMoveEvent(): event already accepted", );
    return doPointerMoveEvent(dataId, event);
}

void AbstractLayer::doPointerMoveEvent(UnsignedInt, PointerMoveEvent&) {}

void AbstractLayer::pointerEnterEvent(const UnsignedInt dataId, PointerMoveEvent& event) {
    CORRADE_ASSERT(features() & LayerFeature::Event,
        "Whee::AbstractLayer::pointerEnterEvent(): feature not supported", );
    #ifndef CORRADE_NO_ASSERT
    const State& state = *_state;
    #endif
    CORRADE_ASSERT(dataId < state.data.size(),
        "Whee::AbstractLayer::pointerEnterEvent(): index" << dataId << "out of range for" << state.data.size() << "data", );
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
    #ifndef CORRADE_NO_ASSERT
    const State& state = *_state;
    #endif
    CORRADE_ASSERT(dataId < state.data.size(),
        "Whee::AbstractLayer::pointerLeaveEvent(): index" << dataId << "out of range for" << state.data.size() << "data", );
    CORRADE_ASSERT(!event.isAccepted(),
        "Whee::AbstractLayer::pointerLeaveEvent(): event already accepted", );
    /* This isn't triggerable from public code so can be an internal assert,
       verifying just that the UserInterface internals don't mess up */
    CORRADE_INTERNAL_ASSERT(event.relativePosition().isZero());
    return doPointerLeaveEvent(dataId, event);
}

void AbstractLayer::doPointerLeaveEvent(UnsignedInt, PointerMoveEvent&) {}

}}
