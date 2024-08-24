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
#include <Corrade/Containers/Iterable.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Magnum/Math/Time.h>

#include "Magnum/Whee/AbstractAnimator.h"
#include "Magnum/Whee/Event.h"
#include "Magnum/Whee/Handle.h"

namespace Magnum { namespace Whee {

Debug& operator<<(Debug& debug, const LayerFeature value) {
    /* Special case coming from the LayerFeatures printer. As both flags are a
       superset of Draw, printing just one would result in
       `LayerFeature::DrawUsesBlending|LayerFeature(0x04)` in the output. */
    if(value == LayerFeature(UnsignedByte(LayerFeature::DrawUsesBlending|LayerFeature::DrawUsesScissor)))
        return debug << LayerFeature::DrawUsesBlending << Debug::nospace << "|" << Debug::nospace << LayerFeature::DrawUsesScissor;

    debug << "Whee::LayerFeature" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case LayerFeature::value: return debug << "::" #value;
        _c(Draw)
        _c(DrawUsesBlending)
        _c(DrawUsesScissor)
        _c(Composite)
        _c(Event)
        _c(AnimateData)
        _c(AnimateStyles)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "(" << Debug::nospace << Debug::hex << UnsignedByte(value) << Debug::nospace << ")";
}

Debug& operator<<(Debug& debug, const LayerFeatures value) {
    return Containers::enumSetDebugOutput(debug, value, "Whee::LayerFeatures{}", {
        /* Both are a superset of Draw, meaning printing just one
           would result in `LayerFeature::DrawUsesBlending|LayerFeature(0x04)`
           in the output. So we pass both and let the LayerFeature printer deal
           with that. */
        LayerFeature(UnsignedByte(LayerFeature::DrawUsesBlending|LayerFeature::DrawUsesScissor)),
        LayerFeature::DrawUsesBlending, /* superset of Draw */
        LayerFeature::DrawUsesScissor, /* superset of Draw */
        LayerFeature::Composite, /* superset of Draw */
        LayerFeature::Draw,
        LayerFeature::Event,
        LayerFeature::AnimateData,
        LayerFeature::AnimateStyles
    });
}

Debug& operator<<(Debug& debug, const LayerState value) {
    /* Special case coming from the LayerState printer. As both flags are a
       superset of NeedsNodeOrderUpdate, printing just one would result in
       `LayerState::NeedsNodeOrderUpdate|LayerState(0x4)` in the output. */
    if(value == LayerState(UnsignedShort(LayerState::NeedsNodeOffsetSizeUpdate|LayerState::NeedsAttachmentUpdate)))
        return debug << LayerState::NeedsNodeOffsetSizeUpdate << Debug::nospace << "|" << Debug::nospace << LayerState::NeedsAttachmentUpdate;

    debug << "Whee::LayerState" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case LayerState::value: return debug << "::" #value;
        _c(NeedsNodeEnabledUpdate)
        _c(NeedsNodeOrderUpdate)
        _c(NeedsNodeOffsetSizeUpdate)
        _c(NeedsAttachmentUpdate)
        _c(NeedsDataUpdate)
        _c(NeedsCommonDataUpdate)
        _c(NeedsSharedDataUpdate)
        _c(NeedsCompositeOffsetSizeUpdate)
        _c(NeedsDataClean)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "(" << Debug::nospace << Debug::hex << UnsignedShort(value) << Debug::nospace << ")";
}

Debug& operator<<(Debug& debug, const LayerStates value) {
    return Containers::enumSetDebugOutput(debug, value, "Whee::LayerStates{}", {
        /* Both are a superset of NeedsNodeOrderUpdate, meaning printing just
           one would result in `LayerState::NeedsNodeOrderUpdate|LayerState(0x4)`
           in the output. So we pass both and let the LayerState printer deal
           with that. */
        LayerState(UnsignedShort(LayerState::NeedsNodeOffsetSizeUpdate|LayerState::NeedsAttachmentUpdate)),
        LayerState::NeedsAttachmentUpdate,
        LayerState::NeedsNodeOffsetSizeUpdate,
        /* Implied by NeedsAttachmentUpdate and NeedsNodeOffsetSizeUpdate, has
           to be after */
        LayerState::NeedsNodeOrderUpdate,
        /* Implied by NeedsNodeOrderUpdate, has to be after */
        LayerState::NeedsNodeEnabledUpdate,
        LayerState::NeedsDataUpdate,
        LayerState::NeedsCommonDataUpdate,
        LayerState::NeedsSharedDataUpdate,
        LayerState::NeedsCompositeOffsetSizeUpdate,
        LayerState::NeedsDataClean
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

    #ifndef CORRADE_NO_ASSERT
    bool setSizeCalled = false;
    #endif
    /* 0/4 bytes free, 1/5 on a no-assert build */

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
    const LayerStates state = doState();
    #ifndef CORRADE_NO_ASSERT
    LayerStates expectedStates = LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate|LayerState::NeedsSharedDataUpdate;
    if(features() >= LayerFeature::Composite)
        expectedStates |= LayerState::NeedsCompositeOffsetSizeUpdate;
    #endif
    CORRADE_ASSERT(state <= expectedStates,
        "Whee::AbstractLayer::state(): implementation expected to return a subset of" << expectedStates << "but got" << state, {});
    return _state->state|state;
}

LayerStates AbstractLayer::doState() const { return {}; }

void AbstractLayer::setNeedsUpdate(const LayerStates state) {
    #ifndef CORRADE_NO_ASSERT
    LayerStates expectedStates = LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate|LayerState::NeedsSharedDataUpdate;
    if(features() >= LayerFeature::Composite)
        expectedStates |= LayerState::NeedsCompositeOffsetSizeUpdate;
    #endif
    CORRADE_ASSERT(state && state <= expectedStates,
        "Whee::AbstractLayer::setNeedsUpdate(): expected a non-empty subset of" << expectedStates << "but got" << state, );
    _state->state |= state;
}

std::size_t AbstractLayer::capacity() const {
    return _state->data.size();
}

std::size_t AbstractLayer::usedCount() const {
    /* The "pointer" chasing in here is a bit nasty, but there's no other way
       to know which data are actually used and which not. The node is Null
       for unused data, yes, but it's also null for data that haven't been
       attached yet. */
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

DataHandle AbstractLayer::create(const NodeHandle node) {
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
    state.state |= LayerState::NeedsDataUpdate;
    if(node != NodeHandle::Null) {
        data->used.node = node;
        state.state |= LayerState::NeedsAttachmentUpdate|
                       LayerState::NeedsNodeOffsetSizeUpdate;
        if(features() >= LayerFeature::Composite)
            state.state |= LayerState::NeedsCompositeOffsetSizeUpdate;
    }

    return dataHandle(state.handle, (data - state.data), data->used.generation);
}

void AbstractLayer::remove(const DataHandle handle) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::AbstractLayer::remove(): invalid handle" << handle, );

    State& state = *_state;
    /* Mark the layer as needing a cleanData() call for any associated
       animators */
    state.state |= LayerState::NeedsDataClean;

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
    /* Mark the layer as needing a cleanData() call for any associated
       animators */
    state.state |= LayerState::NeedsDataClean;

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
       clean() below doesn't set any state after calling removeInternal(). */
}

void AbstractLayer::setAnimator(AbstractDataAnimator& animator) const {
    CORRADE_ASSERT(features() & LayerFeature::AnimateData,
        "Whee::AbstractLayer::setAnimator(): data animation not supported", );
    CORRADE_ASSERT(animator.features() & AnimatorFeature::DataAttachment,
        "Whee::AbstractLayer::setAnimator(): data attachment not supported by the animator", );
    CORRADE_ASSERT(animator.layer() == LayerHandle::Null,
        "Whee::AbstractLayer::setAnimator(): animator already associated with" << animator.layer(), );

    animator.setLayerInternal(*this);
}

void AbstractLayer::setAnimator(AbstractStyleAnimator& animator) const {
    CORRADE_ASSERT(features() & LayerFeature::AnimateStyles,
        "Whee::AbstractLayer::setAnimator(): style animation not supported", );
    CORRADE_ASSERT(animator.features() & AnimatorFeature::DataAttachment,
        "Whee::AbstractLayer::setAnimator(): data attachment not supported by the animator", );
    CORRADE_ASSERT(animator.layer() == LayerHandle::Null,
        "Whee::AbstractLayer::setAnimator(): animator already associated with" << animator.layer(), );

    animator.setLayerInternal(*this);
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

    /* If the data is already attached to the same node, this does nothing */
    if(state.data[id].used.node == node)
        return;

    state.data[id].used.node = node;
    state.state |= LayerState::NeedsAttachmentUpdate;
    if(node != NodeHandle::Null) {
        state.state |= LayerState::NeedsNodeOffsetSizeUpdate;
        if(features() >= LayerFeature::Composite)
            state.state |= LayerState::NeedsCompositeOffsetSizeUpdate;
    }
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

Containers::StridedArrayView1D<const NodeHandle> AbstractLayer::nodes() const {
    return stridedArrayView(_state->data).slice(&Data::used).slice(&Data::Used::node);
}

void AbstractLayer::setSize(const Vector2& size, const Vector2i& framebufferSize) {
    CORRADE_ASSERT(features() & LayerFeature::Draw,
        "Whee::AbstractLayer::setSize():" << LayerFeature::Draw << "not supported", );
    CORRADE_ASSERT(size.product() && framebufferSize.product(),
        "Whee::AbstractLayer::setSize(): expected non-zero sizes, got" << size << "and" << framebufferSize, );
    #ifndef CORRADE_NO_ASSERT
    _state->setSizeCalled = true;
    #endif
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
}

void AbstractLayer::doClean(Containers::BitArrayView) {}

void AbstractLayer::cleanData(const Containers::Iterable<AbstractAnimator>& animators) {
    State& state = *_state;
    const Containers::StridedArrayView1D<const UnsignedShort> dataGenerations = stridedArrayView(state.data).slice(&Data::used).slice(&Data::Used::generation);

    for(AbstractAnimator& animator: animators) {
        CORRADE_ASSERT(animator.features() & AnimatorFeature::DataAttachment,
            "Whee::AbstractLayer::cleanData(): data attachment not supported by an animator", );
        CORRADE_ASSERT(animator.layer() != LayerHandle::Null,
            "Whee::AbstractLayer::cleanData(): animator has no layer set for data attachment", );
        CORRADE_ASSERT(animator.layer() == handle(),
            "Whee::AbstractLayer::cleanData(): expected an animator associated with" << handle() << "but got" << animator.layer(), );

        animator.cleanData(dataGenerations);
    }

    state.state &= ~LayerState::NeedsDataClean;
}

void AbstractLayer::advanceAnimations(const Nanoseconds time, const Containers::MutableBitArrayView activeStorage, const Containers::StridedArrayView1D<Float>& factorStorage, const Containers::MutableBitArrayView removeStorage, const Containers::Iterable<AbstractDataAnimator>& animators) {
    CORRADE_ASSERT(features() & LayerFeature::AnimateData,
        "Whee::AbstractLayer::advanceAnimations(): data animation not supported", );

    #ifndef CORRADE_NO_ASSERT
    std::size_t maxCapacity = 0;
    for(const AbstractDataAnimator& animator: animators) {
        CORRADE_ASSERT(animator.features() & AnimatorFeature::DataAttachment,
            "Whee::AbstractLayer::advanceAnimations(): data attachment not supported by an animator", );
        CORRADE_ASSERT(animator.layer() != LayerHandle::Null,
            "Whee::AbstractLayer::advanceAnimations(): animator has no layer set for data attachment", );
        CORRADE_ASSERT(animator.layer() == handle(),
            "Whee::AbstractLayer::advanceAnimations(): expected an animator associated with" << handle() << "but got" << animator.layer(), );
        maxCapacity = Math::max(animator.capacity(), maxCapacity);
    }
    CORRADE_ASSERT(
        activeStorage.size() >= maxCapacity &&
        factorStorage.size() == activeStorage.size() &&
        removeStorage.size() == activeStorage.size(),
        "Whee::AbstractLayer::advanceAnimations(): expected activeStorage, factorStorage and removeStorage views to have the same size of at least" << maxCapacity << "elements but got" << activeStorage.size() << Debug::nospace << "," << factorStorage.size() << "and" << removeStorage.size(), );
    #endif

    doAdvanceAnimations(time, activeStorage, factorStorage, removeStorage, animators);
}

void AbstractLayer::doAdvanceAnimations(Nanoseconds, Containers::MutableBitArrayView, const Containers::StridedArrayView1D<Float>&, Containers::MutableBitArrayView, const Containers::Iterable<AbstractDataAnimator>&) {
    CORRADE_ASSERT_UNREACHABLE("Whee::AbstractLayer::advanceAnimations(): data animation advertised but not implemented", );
}

void AbstractLayer::advanceAnimations(const Nanoseconds time, const Containers::MutableBitArrayView activeStorage, const Containers::StridedArrayView1D<Float>& factorStorage, const Containers::MutableBitArrayView removeStorage, const Containers::Iterable<AbstractStyleAnimator>& animators) {
    CORRADE_ASSERT(features() & LayerFeature::AnimateStyles,
        "Whee::AbstractLayer::advanceAnimations(): style animation not supported", );

    #ifndef CORRADE_NO_ASSERT
    std::size_t maxCapacity = 0;
    for(const AbstractStyleAnimator& animator: animators) {
        CORRADE_ASSERT(animator.features() & AnimatorFeature::DataAttachment,
            "Whee::AbstractLayer::advanceAnimations(): data attachment not supported by an animator", );
        CORRADE_ASSERT(animator.layer() != LayerHandle::Null,
            "Whee::AbstractLayer::advanceAnimations(): animator has no layer set for data attachment", );
        CORRADE_ASSERT(animator.layer() == handle(),
            "Whee::AbstractLayer::advanceAnimations(): expected an animator associated with" << handle() << "but got" << animator.layer(), );
        maxCapacity = Math::max(animator.capacity(), maxCapacity);
    }
    CORRADE_ASSERT(
        activeStorage.size() >= maxCapacity &&
        factorStorage.size() == activeStorage.size() &&
        removeStorage.size() == activeStorage.size(),
        "Whee::AbstractLayer::advanceAnimations(): expected activeStorage, factorStorage and removeStorage views to have the same size of at least" << maxCapacity << "elements but got" << activeStorage.size() << Debug::nospace << "," << factorStorage.size() << "and" << removeStorage.size(), );
    #endif

    doAdvanceAnimations(time, activeStorage, factorStorage, removeStorage, animators);
}

void AbstractLayer::doAdvanceAnimations(Nanoseconds, Containers::MutableBitArrayView, const Containers::StridedArrayView1D<Float>&, Containers::MutableBitArrayView, const Containers::Iterable<AbstractStyleAnimator>&) {
    CORRADE_ASSERT_UNREACHABLE("Whee::AbstractLayer::advanceAnimations(): style animation advertised but not implemented", );
}

void AbstractLayer::update(const LayerStates states, const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, const Containers::BitArrayView nodesEnabled, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes, const Containers::StridedArrayView1D<const Vector2>& compositeRectOffsets, const Containers::StridedArrayView1D<const Vector2>& compositeRectSizes) {
    #ifndef CORRADE_NO_ASSERT
    LayerStates expectedStates = LayerState::NeedsNodeOffsetSizeUpdate|LayerState::NeedsNodeEnabledUpdate|LayerState::NeedsNodeOrderUpdate|LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate|LayerState::NeedsSharedDataUpdate|LayerState::NeedsAttachmentUpdate;
    if(features() >= LayerFeature::Composite)
        expectedStates |= LayerState::NeedsCompositeOffsetSizeUpdate;
    #endif
    CORRADE_ASSERT(states && states <= expectedStates,
        "Whee::AbstractLayer::update(): expected a non-empty subset of" << expectedStates << "but got" << states, );
    CORRADE_ASSERT(clipRectIds.size() == clipRectDataCounts.size(),
        "Whee::AbstractLayer::update(): expected clip rect ID and data count views to have the same size but got" << clipRectIds.size() << "and" << clipRectDataCounts.size(), );
    CORRADE_ASSERT(nodeOffsets.size() == nodeSizes.size() &&
                   nodesEnabled.size() == nodeSizes.size(),
        "Whee::AbstractLayer::update(): expected node offset, size and enabled views to have the same size but got" << nodeOffsets.size() << Debug::nospace << "," << nodeSizes.size() << "and" << nodesEnabled.size(), );
    CORRADE_ASSERT(clipRectOffsets.size() == clipRectSizes.size(),
        "Whee::AbstractLayer::update(): expected clip rect offset and size views to have the same size but got" << clipRectOffsets.size() << "and" << clipRectSizes.size(), );
    CORRADE_ASSERT(compositeRectOffsets.size() == compositeRectSizes.size(),
        "Whee::AbstractLayer::update(): expected composite rect offset and size views to have the same size but got" << compositeRectOffsets.size() << "and" << compositeRectSizes.size(), );
    CORRADE_ASSERT(features() >= LayerFeature::Composite || compositeRectOffsets.isEmpty(),
        "Whee::AbstractLayer::update(): compositing not supported but got" << compositeRectOffsets.size() << "composite rects", );
    auto& state = *_state;
    CORRADE_ASSERT(!(features() >= LayerFeature::Draw) || state.setSizeCalled,
        "Whee::AbstractLayer::update(): user interface size wasn't set", );
    /* Don't pass the NeedsAttachmentUpdate bit to the implementation as it
       shouldn't need that, just NeedsNodeOrderUpdate that's a subset of it */
    doUpdate(states & ~(LayerState::NeedsAttachmentUpdate & ~LayerState::NeedsNodeOrderUpdate), dataIds, clipRectIds, clipRectDataCounts, nodeOffsets, nodeSizes, nodesEnabled, clipRectOffsets, clipRectSizes, compositeRectOffsets, compositeRectSizes);
    state.state &= ~states;
}

void AbstractLayer::doUpdate(LayerStates, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, Containers::BitArrayView, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&) {}

void AbstractLayer::composite(AbstractRenderer& renderer, const Containers::StridedArrayView1D<const Vector2>& compositeRectOffsets, const Containers::StridedArrayView1D<const Vector2>& compositeRectSizes, const std::size_t offset, const std::size_t count) {
    CORRADE_ASSERT(features() & LayerFeature::Composite,
        "Whee::AbstractLayer::composite(): feature not supported", );
    CORRADE_ASSERT(compositeRectOffsets.size() == compositeRectSizes.size(),
        "Whee::AbstractLayer::composite(): expected rect offset and size views to have the same size but got" << compositeRectOffsets.size() << "and" << compositeRectSizes.size(), );
    CORRADE_ASSERT(offset + count <= compositeRectOffsets.size(),
        "Whee::AbstractLayer::composite(): offset" << offset << "and count" << count << "out of range for" << compositeRectOffsets.size() << "items", );
    doComposite(renderer, compositeRectOffsets, compositeRectSizes, offset, count);
}

void AbstractLayer::doComposite(AbstractRenderer&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, std::size_t, std::size_t) {
    CORRADE_ASSERT_UNREACHABLE("Whee::AbstractLayer::composite(): feature advertised but not implemented", );
}

void AbstractLayer::draw(const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const std::size_t offset, const std::size_t count, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, const std::size_t clipRectOffset, const std::size_t clipRectCount, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, const Containers::BitArrayView nodesEnabled, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes) {
    CORRADE_ASSERT(features() & LayerFeature::Draw,
        "Whee::AbstractLayer::draw(): feature not supported", );
    CORRADE_ASSERT(offset + count <= dataIds.size(),
        "Whee::AbstractLayer::draw(): offset" << offset << "and count" << count << "out of range for" << dataIds.size() << "items", );
    CORRADE_ASSERT(clipRectIds.size() == clipRectDataCounts.size(),
        "Whee::AbstractLayer::draw(): expected clip rect ID and data count views to have the same size but got" << clipRectIds.size() << "and" << clipRectDataCounts.size(), );
    CORRADE_ASSERT(clipRectOffset + clipRectCount <= clipRectIds.size(),
        "Whee::AbstractLayer::draw(): clip rect offset" << clipRectOffset << "and count" << clipRectCount << "out of range for" << clipRectIds.size() << "items", );
    CORRADE_ASSERT(nodeOffsets.size() == nodeSizes.size() &&
                   nodesEnabled.size() == nodeSizes.size(),
        "Whee::AbstractLayer::draw(): expected node offset, size and enabled views to have the same size but got" << nodeOffsets.size() << Debug::nospace << "," << nodeSizes.size() << "and" << nodesEnabled.size(), );
    CORRADE_ASSERT(clipRectOffsets.size() == clipRectSizes.size(),
        "Whee::AbstractLayer::draw(): expected clip rect offset and size views to have the same size but got" << clipRectOffsets.size() << "and" << clipRectSizes.size(), );
    doDraw(dataIds, offset, count, clipRectIds, clipRectDataCounts, clipRectOffset, clipRectCount, nodeOffsets, nodeSizes, nodesEnabled, clipRectOffsets, clipRectSizes);
}

void AbstractLayer::doDraw(const Containers::StridedArrayView1D<const UnsignedInt>&, std::size_t, std::size_t, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const UnsignedInt>&, std::size_t, std::size_t, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, Containers::BitArrayView, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&) {
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

void AbstractLayer::pointerTapOrClickEvent(const UnsignedInt dataId, PointerEvent& event) {
    CORRADE_ASSERT(features() & LayerFeature::Event,
        "Whee::AbstractLayer::pointerTapOrClickEvent(): feature not supported", );
    #ifndef CORRADE_NO_ASSERT
    const State& state = *_state;
    #endif
    CORRADE_ASSERT(dataId < state.data.size(),
        "Whee::AbstractLayer::pointerTapOrClickEvent(): index" << dataId << "out of range for" << state.data.size() << "data", );
    CORRADE_ASSERT(!event.isAccepted(),
        "Whee::AbstractLayer::pointerTapOrClickEvent(): event already accepted", );
    return doPointerTapOrClickEvent(dataId, event);
}

void AbstractLayer::doPointerTapOrClickEvent(UnsignedInt, PointerEvent&) {}

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

void AbstractLayer::focusEvent(const UnsignedInt dataId, FocusEvent& event) {
    CORRADE_ASSERT(features() & LayerFeature::Event,
        "Whee::AbstractLayer::focusEvent(): feature not supported", );
    #ifndef CORRADE_NO_ASSERT
    const State& state = *_state;
    #endif
    CORRADE_ASSERT(dataId < state.data.size(),
        "Whee::AbstractLayer::focusEvent(): index" << dataId << "out of range for" << state.data.size() << "data", );
    CORRADE_ASSERT(!event.isAccepted(),
        "Whee::AbstractLayer::focusEvent(): event already accepted", );
    return doFocusEvent(dataId, event);
}

void AbstractLayer::doFocusEvent(UnsignedInt, FocusEvent&) {}

void AbstractLayer::blurEvent(const UnsignedInt dataId, FocusEvent& event) {
    CORRADE_ASSERT(features() & LayerFeature::Event,
        "Whee::AbstractLayer::blurEvent(): feature not supported", );
    #ifndef CORRADE_NO_ASSERT
    const State& state = *_state;
    #endif
    CORRADE_ASSERT(dataId < state.data.size(),
        "Whee::AbstractLayer::blurEvent(): index" << dataId << "out of range for" << state.data.size() << "data", );
    CORRADE_ASSERT(!event.isAccepted(),
        "Whee::AbstractLayer::blurEvent(): event already accepted", );
    return doBlurEvent(dataId, event);
}

void AbstractLayer::doBlurEvent(UnsignedInt, FocusEvent&) {}

void AbstractLayer::keyPressEvent(const UnsignedInt dataId, KeyEvent& event) {
    CORRADE_ASSERT(features() & LayerFeature::Event,
        "Whee::AbstractLayer::keyPressEvent(): feature not supported", );
    #ifndef CORRADE_NO_ASSERT
    const State& state = *_state;
    #endif
    CORRADE_ASSERT(dataId < state.data.size(),
        "Whee::AbstractLayer::keyPressEvent(): index" << dataId << "out of range for" << state.data.size() << "data", );
    CORRADE_ASSERT(!event.isAccepted(),
        "Whee::AbstractLayer::keyPressEvent(): event already accepted", );
    return doKeyPressEvent(dataId, event);
}

void AbstractLayer::doKeyPressEvent(UnsignedInt, KeyEvent&) {}

void AbstractLayer::keyReleaseEvent(const UnsignedInt dataId, KeyEvent& event) {
    CORRADE_ASSERT(features() & LayerFeature::Event,
        "Whee::AbstractLayer::keyReleaseEvent(): feature not supported", );
    #ifndef CORRADE_NO_ASSERT
    const State& state = *_state;
    #endif
    CORRADE_ASSERT(dataId < state.data.size(),
        "Whee::AbstractLayer::keyReleaseEvent(): index" << dataId << "out of range for" << state.data.size() << "data", );
    CORRADE_ASSERT(!event.isAccepted(),
        "Whee::AbstractLayer::keyReleaseEvent(): event already accepted", );
    return doKeyReleaseEvent(dataId, event);
}

void AbstractLayer::doKeyReleaseEvent(UnsignedInt, KeyEvent&) {}

void AbstractLayer::textInputEvent(const UnsignedInt dataId, TextInputEvent& event) {
    CORRADE_ASSERT(features() & LayerFeature::Event,
        "Whee::AbstractLayer::textInputEvent(): feature not supported", );
    #ifndef CORRADE_NO_ASSERT
    const State& state = *_state;
    #endif
    CORRADE_ASSERT(dataId < state.data.size(),
        "Whee::AbstractLayer::textInputEvent(): index" << dataId << "out of range for" << state.data.size() << "data", );
    CORRADE_ASSERT(!event.isAccepted(),
        "Whee::AbstractLayer::textInputEvent(): event already accepted", );
    return doTextInputEvent(dataId, event);
}

void AbstractLayer::doTextInputEvent(UnsignedInt, TextInputEvent&) {}

void AbstractLayer::visibilityLostEvent(const UnsignedInt dataId, VisibilityLostEvent& event) {
    CORRADE_ASSERT(features() & LayerFeature::Event,
        "Whee::AbstractLayer::visibilityLostEvent(): feature not supported", );
    #ifndef CORRADE_NO_ASSERT
    const State& state = *_state;
    #endif
    CORRADE_ASSERT(dataId < state.data.size(),
        "Whee::AbstractLayer::visibilityLostEvent(): index" << dataId << "out of range for" << state.data.size() << "data", );
    return doVisibilityLostEvent(dataId, event);
}

void AbstractLayer::doVisibilityLostEvent(UnsignedInt, VisibilityLostEvent&) {}

}}
