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

#include "AbstractUserInterface.h"

#include <Corrade/Containers/ArrayTuple.h>
#include <Corrade/Containers/BitArrayView.h>
#include <Corrade/Containers/EnumSet.hpp>
#include <Corrade/Containers/GrowableArray.h>
#include <Corrade/Containers/Optional.h>
#include <Magnum/Math/Vector2.h>

#include "Magnum/Whee/AbstractLayer.h"
#include "Magnum/Whee/Event.h"
#include "Magnum/Whee/Handle.h"
#include "Magnum/Whee/Implementation/abstractUserInterface.h"

namespace Magnum { namespace Whee {

Debug& operator<<(Debug& debug, const UserInterfaceState value) {
    /* Special case coming from the UserInterfaceStates printer. As both are a
       superset of NeedsDataClean, printing just one would result in
       `NeedsDataClean|UserInterfaceState(0x2)` in the output. */
    if(value == UserInterfaceState(UnsignedByte(UserInterfaceState::NeedsNodeUpdate|UserInterfaceState::NeedsDataClean)))
        return debug << UserInterfaceState::NeedsNodeUpdate << Debug::nospace << "|" << Debug::nospace << UserInterfaceState::NeedsDataClean;

    debug << "Whee::UserInterfaceState" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case UserInterfaceState::value: return debug << "::" #value;
        _c(NeedsDataUpdate)
        _c(NeedsDataAttachmentUpdate)
        _c(NeedsNodeLayoutUpdate)
        _c(NeedsNodeUpdate)
        _c(NeedsDataClean)
        _c(NeedsNodeClean)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "(" << Debug::nospace << Debug::hex << UnsignedByte(value) << Debug::nospace << ")";
}

Debug& operator<<(Debug& debug, const UserInterfaceStates value) {
    return Containers::enumSetDebugOutput(debug, value, "Whee::UserInterfaceStates{}", {
        UserInterfaceState::NeedsNodeClean,
        /* Both are a superset of NeedsDataClean, meaning printing just one
           would result in `NeedsDataClean|UserInterfaceState(0x2)` in the
           output. So we pass both and let the UserInterfaceState  deal with
           that. */
        UserInterfaceState(UnsignedByte(UserInterfaceState::NeedsNodeUpdate|UserInterfaceState::NeedsDataClean)),
        /* Implied by NeedsNodeClean, has to be after */
        UserInterfaceState::NeedsDataClean,
        /* Implied by NeedsNodeClean, has to be after */
        UserInterfaceState::NeedsNodeUpdate,
        /* Implied by NeedsNodeUpdate, has to be after */
        UserInterfaceState::NeedsNodeLayoutUpdate,
        /* Implied by NeedsNodeUpdate, has to be after */
        UserInterfaceState::NeedsDataAttachmentUpdate,
        /* Implied by NeedsDataAttachmentUpdate and NeedsNodeLayoutUpdate, has
           to be after */
        UserInterfaceState::NeedsDataUpdate
    });
}

Debug& operator<<(Debug& debug, const NodeFlag value) {
    debug << "Whee::NodeFlag" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case NodeFlag::value: return debug << "::" #value;
        _c(Hidden)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "(" << Debug::nospace << Debug::hex << UnsignedByte(value) << Debug::nospace << ")";
}

Debug& operator<<(Debug& debug, const NodeFlags value) {
    return Containers::enumSetDebugOutput(debug, value, "Whee::NodeFlags{}", {
        NodeFlag::Hidden
    });
}

namespace {

union Layer {
    explicit Layer() noexcept: used{} {}
    Layer(const Layer&&) = delete;
    Layer(Layer&& other) noexcept: used{Utility::move(other.used)} {}
    ~Layer() {
        used.instance.~Pointer();
    }
    Layer& operator=(const Layer&&) = delete;
    Layer& operator=(Layer&& other) noexcept {
        Utility::swap(other.used, used);
        return *this;
    }

    struct Used {
        /* Except for the generation, instance, which is reset during
           removal, and the feature set which may be checked even without any
           instance present, these have to be re-filled every time a handle is
           recycled, so it doesn't make sense to initialize them to
           anything. */

        /* Layer instance. Null for newly created layers until
           setLayerInstance() is called, set back to null in removeLayer(). */
        Containers::Pointer<AbstractLayer> instance;

        /* Together with index of this item in `layers` used for creating a
           LayerHandle. Increased every time a handle reaches removeLayer().
           Has to be initially non-zero to differentiate the first ever handle
           (with index 0) from LayerHandle::Null. Once wraps back to zero once
           the handle gets disabled. */
        UnsignedByte generation = 1;

        /* Extracted from AbstractLayer for more direct access */
        LayerFeatures features;

        /* Always meant to be non-null and valid. To make insert/remove
           operations easier the list is cyclic, so the last layers's `next` is
           the same as `_state->firstLayer`. */
        LayerHandle previous;
        LayerHandle next;

        /* 2 bytes free */
    } used;

    /* Used only if the Layer is among the free ones */
    struct Free {
        /* The instance pointer value has to be preserved in order to be able
           to distinguish between used layers with set instances and others
           (free or not set yet), which is used for iterating them in clean()
           and update(). If free, the member is always null, to not cause
           issues when moving & destructing. There's no other way to
           distinguish free and used layers apart from walking the free
           list. */
        void* instance;

        /* The generation value has to be preserved in order to increment it
           next time it gets used */
        UnsignedByte generation;

        /* See State::firstFreeLayer for more information. Has to be larger
           than 8 bits in order to distinguish between index 255 and "no next
           free layer" (which is now 65535). */
        UnsignedShort next;
    } free;
};

static_assert(
    offsetof(Layer::Used, instance) == offsetof(Layer::Free, instance) &&
    offsetof(Layer::Used, generation) == offsetof(Layer::Free, generation),
    "Layer::Used and Free layout not compatible");

union Node {
    explicit Node() noexcept: used{} {}

    struct Used {
        /* Except for the generation, these have to be re-filled every time a
           handle is recycled, so it doesn't make sense to initialize them to
           anything. */

        /* Parent node handle or top-level node order index.

           For top-level nodes the generation is set to 0 and the ID points
           inside the `nodeOrder` array, which then stores a doubly linked
           list, see the `NodeOrder` struct for details. If the ID has all bits
           set to 1, it's not included in the draw and event processing
           order. */
        NodeHandle parentOrOrder;

        /* Together with index of this item in `nodes` used for creating a
           NodeHandle. Increased every time a handle reaches removeNode(). Has
           to be initially non-zero to differentiate the first ever handle
           (with index 0) from NodeHandle::Null. Once becomes
           `1 << NodeHandleGenerationBits` the handle gets disabled. */
        UnsignedShort generation = 1;

        NodeFlags flags{NoInit};

        /* One byte free */

        /* Offset relative to parent, size of the contents for event handling
           propagation, layouting and clipping */
        Vector2 offset{NoInit}, size{NoInit};
    } used;

    /* Used only if the Node is among the free ones */
    struct Free {
        /* Free nodes need to have this preserved, with generation set to 0 and
           ID to all 1s, to avoid calling removeNode() again on free items in
           clean(). There's no other way to distinguish free and used nodes
           apart from walking the free list. */
        NodeHandle parentOrOrder;

        /* The generation value has to be preserved in order to increment it
           next time it gets used */
        UnsignedShort generation;

        /* See State::firstFreeNode for more information */
        UnsignedInt next;
    } free;
};

#ifndef CORRADE_NO_STD_IS_TRIVIALLY_TRAITS
static_assert(std::is_trivially_copyable<Node>::value, "Node not trivially copyable");
#endif
static_assert(
    offsetof(Node::Used, parentOrOrder) == offsetof(Node::Free, parentOrOrder) &&
    offsetof(Node::Used, generation) == offsetof(Node::Free, generation),
    "Node::Used and Free layout not compatible");

/* A doubly linked list is needed in order to have clearNodeOrder() work
   conveniently (so, not "clear node order for a node that's ordered after
   <handle>" like std::forward_list does) and in O(1). */
union NodeOrder {
    struct Used {
        /* These are always meant to be non-null and valid. To make
           insert/remove operations easier the list is cyclic, so the last
           node's `next` is the same as `_state->firstNodeOrder`. */
        NodeHandle previous;
        NodeHandle next;
    } used;

    /* Used only if the NodeOrder is among the free ones */
    struct Free {
        /* See State::firstFreeNodeOrder for more information. */
        UnsignedInt next;
    } free;
};

#ifndef CORRADE_NO_STD_IS_TRIVIALLY_TRAITS
static_assert(std::is_trivially_copyable<NodeOrder>::value, "NodeOrder not trivially copyable");
#endif

struct Data {
    /* Node the data belongs to */
    NodeHandle node;

    /* 4 (or maybe even six) bytes free */

    /* Data handle */
    DataHandle data;
};

#ifdef CORRADE_STD_IS_TRIVIALLY_TRAITS_SUPPORTED
static_assert(std::is_trivially_copyable<Data>::value, "Data not trivially copyable");
#endif

}

struct AbstractUserInterface::State {
    /* Layers, indexed by LayerHandle */
    Containers::Array<Layer> layers;
    /* The `Layer` then has a `next` member containing the next layer in the
       draw order. To make insert/remove operations easier the list is cyclic,
       so the last layer's `next` is the same as `firstLayer`. */
    LayerHandle firstLayer = LayerHandle::Null;
    /* Indices into the `layers` array. The `Layer` then has a `nextFree`
       member containing the next free index. To avoid repeatedly reusing the
       same handles and exhausting their generation counter too soon, new
       layers get taken from the front and removed are put at the end. A value
       with all bits set means there's no (first/next/last) free layer.

       Would use ~UnsignedShort{} but the result is int (C, FFS!!), which then
       causes signed/unsigned mismatch warnings on MSVC. */
    UnsignedShort firstFreeLayer = 0xffffu;
    UnsignedShort lastFreeLayer = 0xffffu;

    /* Nodes, indexed by NodeHandle */
    Containers::Array<Node> nodes;
    /* Indices into the `nodes` array. The `Node` then has a `nextFree`
       member containing the next free index. To avoid repeatedly reusing the
       same handles and exhausting their generation counter too soon, new
       nodes get taken from the front and removed are put at the end. A value
       with all bits set means there's no (first/next/last) free node. */
    UnsignedInt firstFreeNode = ~UnsignedInt{};
    UnsignedInt lastFreeNode = ~UnsignedInt{};

    Containers::Array<NodeOrder> nodeOrder;
    /* Doesn't point into the `nodeOrder` array but instead is a handle, for
       which then then the ID of `Node::parentOrOrder` points into the
       `nodeOrder` array. If null, there's no nodes to process at all. */
    NodeHandle firstNodeOrder = NodeHandle::Null;
    /* Index into the `nodeOrder` array. The `NodeOrder` then has a
       `nextFree` member containing the next free index. No handles are exposed
       for these, thus there's no problem with generation exhausing and the
       recycling doesn't need to be made from the opposite side. A value with
       all bits set means there's no (first/next) free node order. */
    UnsignedInt firstFreeNodeOrder = ~UnsignedInt{};

    /* Node data assignments, in no particular order */
    Containers::Array<Data> data;

    /* Tracks whether update() and clean() needs to do something */
    UserInterfaceStates state;

    /* Data for updates, event handling and drawing, repopulated by clean() and
       update() */
    Containers::ArrayTuple nodeStateStorage;
    Containers::ArrayView<UnsignedInt> visibleNodeIds;
    Containers::ArrayView<UnsignedInt> visibleNodeChildrenCounts;
    Containers::StridedArrayView1D<UnsignedInt> visibleFrontToBackTopLevelNodeIndices;
    Containers::ArrayView<Vector2> absoluteNodeOffsets;
    Containers::ArrayTuple dataStateStorage;
    Containers::ArrayView<UnsignedInt> dataToUpdateLayerOffsets;
    Containers::ArrayView<UnsignedInt> dataToUpdateIds;
    Containers::ArrayView<UnsignedInt> dataToUpdateNodeIds;
    Containers::ArrayView<UnsignedByte> dataToDrawLayerIds;
    Containers::ArrayView<UnsignedInt> dataToDrawOffsets;
    Containers::ArrayView<UnsignedInt> dataToDrawSizes;
    /* Indexed by node ID in order to make it possible to look up node data by
       node ID, however contains data only for visible nodes */
    Containers::ArrayView<UnsignedInt> visibleNodeEventDataOffsets;
    Containers::ArrayView<DataHandle> visibleNodeEventData;
    UnsignedInt drawCount = 0;
};

AbstractUserInterface::AbstractUserInterface(): _state{InPlaceInit} {}

AbstractUserInterface::AbstractUserInterface(AbstractUserInterface&&) noexcept = default;

AbstractUserInterface& AbstractUserInterface::operator=(AbstractUserInterface&&) noexcept = default;

AbstractUserInterface::~AbstractUserInterface() = default;

UserInterfaceStates AbstractUserInterface::state() const {
    const State& state = *_state;

    /* Unless UserInterfaceState::NeedsDataClean (or its subset,
       NeedsDataUpdate) is set already, go through all layers and inherit the
       NeedsUpdate flag from them. Invalid (removed) layers have instances
       set to nullptr as well, so this will skip them. */
    UserInterfaceStates states;
    if(!(state.state >= UserInterfaceState::NeedsDataClean)) for(const Layer& layer: state.layers) {
        if(const AbstractLayer* const instance = layer.used.instance.get()) {
            const LayerStates layerState = instance->state();
            if(layerState >= LayerState::NeedsUpdate)
                states |= UserInterfaceState::NeedsDataUpdate;
            if(layerState >= LayerState::NeedsClean) {
                states |= UserInterfaceState::NeedsDataClean;
                /* There's no broader state than NeedsDataClean so we can stop
                   iterating further */
                break;
            }
        }
    }

    return state.state|states;
}

std::size_t AbstractUserInterface::layerCapacity() const {
    return _state->layers.size();
}

std::size_t AbstractUserInterface::layerUsedCount() const {
    const State& state = *_state;
    std::size_t free = 0;
    UnsignedShort index = state.firstFreeLayer;
    /* C, FUCK your STUPID conversion rules. Why isn't ~UnsignedShort{} 65535?
       Why it evaluates to -1?! Why UnsignedShort(~UnsignedShort{}) is the only
       way to put it back to 16 bits? Why is everything so fucking awful?! Why
       do you force me to use error-prone constants, FFS?! */
    while(index != 0xffffu) {
        index = state.layers[index].free.next;
        ++free;
    }
    return state.layers.size() - free;
}

bool AbstractUserInterface::isHandleValid(const LayerHandle handle) const {
    if(handle == LayerHandle::Null)
        return false;
    const State& state = *_state;
    const UnsignedInt index = layerHandleId(handle);
    if(index >= state.layers.size())
        return false;
    /* Zero generation (i.e., where it wrapped around from 255) is also
       invalid.

       Note that this can still return true for manually crafted handles that
       point to free nodes with correct generation counters. The only way to
       detect that would be by either iterating the free list (slow) or by
       keeping an additional bitfield marking free items. I don't think that's
       necessary. */
    const UnsignedInt generation = layerHandleGeneration(handle);
    return generation && generation == state.layers[index].used.generation;
}

bool AbstractUserInterface::isHandleValid(const DataHandle handle) const {
    if(dataHandleData(handle) == LayerDataHandle::Null ||
       dataHandleLayer(handle) == LayerHandle::Null)
        return false;
    const State& state = *_state;
    const UnsignedInt layerIndex = dataHandleLayerId(handle);
    if(layerIndex >= state.layers.size())
        return false;
    const Layer& layer = state.layers[layerIndex];
    if(!layer.used.instance)
        return false;
    return dataHandleLayerGeneration(handle) == layer.used.generation && layer.used.instance->isHandleValid(dataHandleData(handle));
}

LayerHandle AbstractUserInterface::layerFirst() const {
    return _state->firstLayer;
}

LayerHandle AbstractUserInterface::layerLast() const {
    const State& state = *_state;
    if(state.firstLayer == LayerHandle::Null)
        return LayerHandle::Null;
    return state.layers[layerHandleId(state.firstLayer)].used.previous;
}

LayerHandle AbstractUserInterface::layerPrevious(LayerHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::AbstractUserInterface::layerPrevious(): invalid handle" << handle, {});
    const State& state = *_state;
    if(state.firstLayer == handle)
        return LayerHandle::Null;
    return state.layers[layerHandleId(handle)].used.previous;
}

LayerHandle AbstractUserInterface::layerNext(LayerHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::AbstractUserInterface::layerNext(): invalid handle" << handle, {});
    const State& state = *_state;
    const LayerHandle next = state.layers[layerHandleId(handle)].used.next;
    if(state.firstLayer == next)
        return LayerHandle::Null;
    return next;
}

LayerHandle AbstractUserInterface::createLayer(const LayerHandle before) {
    CORRADE_ASSERT(before == LayerHandle::Null || isHandleValid(before),
        "Whee::AbstractUserInterface::createLayer(): invalid before handle" << before, {});

    /* Find the first free layer if there is, update the free index to point to
       the next one (or none) */
    State& state = *_state;
    Layer* layer;
    /* C, FUCK your STUPID conversion rules. Why isn't ~UnsignedShort{} 65535?
       Why it evaluates to -1?! Why UnsignedShort(~UnsignedShort{}) is the only
       way to put it back to 16 bits? Why is everything so fucking awful?! Why
       do you force me to use error-prone constants, FFS?! */
    if(state.firstFreeLayer != 0xffffu) {
        layer = &state.layers[state.firstFreeLayer];
        /* If there's just one item in the list, make the list empty */
        if(state.firstFreeLayer == state.lastFreeLayer) {
            CORRADE_INTERNAL_ASSERT(layer->free.next == 0xffffu);
            state.firstFreeLayer = state.lastFreeLayer = 0xffffu;
        } else {
            state.firstFreeLayer = layer->free.next;
        }

    /* If there isn't, allocate a new one */
    } else {
        CORRADE_ASSERT(state.layers.size() < 1 << Implementation::LayerHandleIdBits,
            "Whee::AbstractUserInterface::createLayer(): can only have at most" << (1 << Implementation::LayerHandleIdBits) << "layers", {});
        layer = &arrayAppend(state.layers, InPlaceInit);
    }

    /* In both above cases the generation is already set appropriately, either
       initialized to 1, or incremented when it got remove()d (to mark existing
       handles as invalid) */
    const LayerHandle handle = layerHandle((layer - state.layers), layer->used.generation);

    /* This is the first ever layer, no need to connect with anything else */
    if(state.firstLayer == LayerHandle::Null) {
        CORRADE_INTERNAL_ASSERT(before == LayerHandle::Null);
        layer->used.previous = handle;
        layer->used.next = handle;
        state.firstLayer = handle;
        return handle;
    }

    const LayerHandle next = before == LayerHandle::Null ?
        state.firstLayer : before;
    const LayerHandle previous = state.layers[layerHandleId(next)].used.previous;
    layer->used.previous = previous;
    layer->used.next = next;
    state.layers[layerHandleId(next)].used.previous = handle;
    state.layers[layerHandleId(previous)].used.next = handle;

    /* If the `before` layer was first, the new layer is now first */
    if(state.firstLayer == before)
        state.firstLayer = handle;

    return handle;
}

void AbstractUserInterface::setLayerInstance(Containers::Pointer<AbstractLayer>&& instance) {
    CORRADE_ASSERT(instance,
        "Whee::AbstractUserInterface::setLayerInstance(): instance is null", );
    const LayerHandle handle = instance->handle();
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::AbstractUserInterface::setLayerInstance(): invalid handle" << handle, );
    State& state = *_state;
    const UnsignedInt id = layerHandleId(handle);
    CORRADE_ASSERT(!state.layers[id].used.instance,
        "Whee::AbstractUserInterface::setLayerInstance(): instance for" << handle << "already set", );
    Layer& layer = state.layers[id];
    layer.used.features = instance->features();
    layer.used.instance = Utility::move(instance);
}

const AbstractLayer& AbstractUserInterface::layer(const LayerHandle handle) const {
    const State& state = *_state;
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::AbstractUserInterface::layer(): invalid handle" << handle,
        *state.layers[0].used.instance);
    const UnsignedInt id = layerHandleId(handle);
    CORRADE_ASSERT(state.layers[id].used.instance,
        "Whee::AbstractUserInterface::layer():" << handle << "has no instance set", *state.layers[0].used.instance);
    return *state.layers[id].used.instance;
}

AbstractLayer& AbstractUserInterface::layer(const LayerHandle handle) {
    return const_cast<AbstractLayer&>(const_cast<const AbstractUserInterface&>(*this).layer(handle));
}

void AbstractUserInterface::removeLayer(const LayerHandle handle) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::AbstractUserInterface::removeLayer(): invalid handle" << handle, );
    const UnsignedInt id = layerHandleId(handle);
    State& state = *_state;
    Layer& layer = state.layers[id];

    const LayerHandle originalPrevious = layer.used.previous;
    const LayerHandle originalNext = layer.used.next;
    CORRADE_INTERNAL_ASSERT(isHandleValid(originalPrevious) && isHandleValid(originalNext));

    /* This works correctly also in case of there being just a single item in
       the list (i.e., originalPrevious == originalNext == handle), as the item
       gets unused after */
    state.layers[layerHandleId(originalPrevious)].used.next = originalNext;
    state.layers[layerHandleId(originalNext)].used.previous = originalPrevious;
    if(state.firstLayer == handle) {
        if(handle == originalNext)
            state.firstLayer = LayerHandle::Null;
        else
            state.firstLayer = originalNext;
    }

    /* Delete the instance. The instance being null then means that the layer
       is either free or is newly created until setLayerInstance() is called,
       which is used for iterating them in clean() and update(). */
    layer.used.instance = nullptr;

    /* Increase the layer generation so existing handles pointing to this layer
       are invalidated */
    ++layer.used.generation;

    /* Put the layer at the end of the free list (while they're allocated from
       the front) to not exhaust the generation counter too fast. If the free
       list is empty however, update also the index of the first free layer.

       Don't do this if the generation wrapped around. That makes it disabled,
       i.e. impossible to be recycled later, to avoid aliasing old handles. */
    if(layer.used.generation != 0) {
        /* C, FUCK your STUPID conversion rules. Why isn't ~UnsignedShort{}
           65535? Why it evaluates to -1?! Why UnsignedShort(~UnsignedShort{})
           is the only way to put it back to 16 bits? Why is everything so
           fucking awful?! Why do you force me to use error-prone constants,
           FFS?! */
        layer.free.next = 0xffffu;
        if(state.lastFreeLayer == 0xffffu) {
            CORRADE_INTERNAL_ASSERT(
                state.firstFreeLayer == 0xffffu &&
                state.lastFreeLayer == 0xffffu);
            state.firstFreeLayer = id;
        } else {
            state.layers[state.lastFreeLayer].free.next = id;
        }
        state.lastFreeLayer = id;
    }

    /* Mark the UI as needing a clean() call to refresh data state */
    state.state |= UserInterfaceState::NeedsDataClean;
}

std::size_t AbstractUserInterface::dataAttachmentCount() const {
    return _state->data.size();
}

void AbstractUserInterface::attachData(const NodeHandle node, const DataHandle data) {
    CORRADE_ASSERT(isHandleValid(node),
        "Whee::AbstractUserInterface::attachData(): invalid handle" << node, );
    CORRADE_ASSERT(isHandleValid(data),
        "Whee::AbstractUserInterface::attachData(): invalid handle" << data, );

    State& state = *_state;
    arrayAppend(state.data, InPlaceInit, node, data);

    /* Mark the UI as needing an update() call to refresh data attachment
       state */
    state.state |= UserInterfaceState::NeedsDataAttachmentUpdate;
}

std::size_t AbstractUserInterface::nodeCapacity() const {
    return _state->nodes.size();
}

std::size_t AbstractUserInterface::nodeUsedCount() const {
    const State& state = *_state;
    std::size_t free = 0;
    UnsignedInt index = state.firstFreeNode;
    while(index != ~UnsignedInt{}) {
        index = state.nodes[index].free.next;
        ++free;
    }
    return state.nodes.size() - free;
}

bool AbstractUserInterface::isHandleValid(const NodeHandle handle) const {
    if(handle == NodeHandle::Null)
        return false;
    const UnsignedInt index = nodeHandleId(handle);
    const State& state = *_state;
    if(index >= state.nodes.size())
        return false;
    /* Unlike isHandleValid(LayerHandle), the generation counter here is 16bit
       and a disabled handle is signalized by 0x10000, not 0, so for disabled
       handles this will always fail without having to do any extra checks.

       Note that this can still return true for manually crafted handles that
       point to free nodes with correct generation counters. The only way to
       detect that would be by either iterating the free list (slow) or by
       keeping an additional bitfield marking free items. I don't think that's
       necessary. */
    return nodeHandleGeneration(handle) == state.nodes[index].used.generation;
}

NodeHandle AbstractUserInterface::createNode(const NodeHandle parent, const Vector2& offset, const Vector2& size, const NodeFlags flags) {
    CORRADE_ASSERT(parent == NodeHandle::Null || isHandleValid(parent),
        "Whee::AbstractUserInterface::createNode(): invalid parent handle" << parent, {});

    /* Find the first free node if there is, update the free index to
       point to the next one (or none) */
    Node* node;
    State& state = *_state;
    if(state.firstFreeNode != ~UnsignedInt{}) {
        node = &state.nodes[state.firstFreeNode];

        if(state.firstFreeNode == state.lastFreeNode) {
            CORRADE_INTERNAL_ASSERT(node->free.next == ~UnsignedInt{});
            state.firstFreeNode = state.lastFreeNode = ~UnsignedInt{};
        } else {
            state.firstFreeNode = node->free.next;
        }

    /* If there isn't, allocate a new one */
    } else {
        CORRADE_ASSERT(state.nodes.size() < 1 << Implementation::NodeHandleIdBits,
            "Whee::AbstractUserInterface::createNode(): can only have at most" << (1 << Implementation::NodeHandleIdBits) << "nodes", {});
        node = &arrayAppend(state.nodes, InPlaceInit);
    }

    /* Fill the data. In both above cases the generation is already set
       appropriately, either initialized to 1, or incremented when it got
       remove()d (to mark existing handles as invalid) */
    node->used.flags = flags;
    node->used.offset = offset;
    node->used.size = size;
    const NodeHandle handle = nodeHandle(node - state.nodes, node->used.generation);

    /* If a root node, implicitly mark it as last in the node order, so
       it's drawn at the front. */
    if(parent == NodeHandle::Null) {
        node->used.parentOrOrder = nodeHandle((1 << Implementation::NodeHandleIdBits) - 1, 0);
        setNodeOrder(handle, NodeHandle::Null);
    } else node->used.parentOrOrder = parent;

    /* Mark the UI as needing an update() call to refresh node state */
    state.state |= UserInterfaceState::NeedsNodeUpdate;

    return handle;
}

NodeHandle AbstractUserInterface::createNode(const Vector2& offset, const Vector2& size, const NodeFlags flags) {
    return createNode(NodeHandle::Null, offset, size, flags);
}

NodeHandle AbstractUserInterface::nodeParent(const NodeHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::AbstractUserInterface::nodeParent(): invalid handle" << handle, {});
    /* If the parent node generation is 0, it's a root node. Its ID
       stores an index into `_state->nodeOrder`, so can't return it
       directly in that case. */
    const NodeHandle parent = _state->nodes[nodeHandleId(handle)].used.parentOrOrder;
    return nodeHandleGeneration(parent) == 0 ? NodeHandle::Null : parent;
}

Vector2 AbstractUserInterface::nodeOffset(const NodeHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::AbstractUserInterface::nodeOffset(): invalid handle" << handle, {});
    return _state->nodes[nodeHandleId(handle)].used.offset;
}

void AbstractUserInterface::setNodeOffset(const NodeHandle handle, const Vector2& offset) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::AbstractUserInterface::setNodeOffset(): invalid handle" << handle, );
    State& state = *_state;
    state.nodes[nodeHandleId(handle)].used.offset = offset;

    /* Mark the UI as needing an update() call to refresh node layout state */
    state.state |= UserInterfaceState::NeedsNodeLayoutUpdate;
}

Vector2 AbstractUserInterface::nodeSize(const NodeHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::AbstractUserInterface::nodeSize(): invalid handle" << handle, {});
    return _state->nodes[nodeHandleId(handle)].used.size;
}

void AbstractUserInterface::setNodeSize(const NodeHandle handle, const Vector2& size) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::AbstractUserInterface::setNodeSize(): invalid handle" << handle, );
    State& state = *_state;
    state.nodes[nodeHandleId(handle)].used.size = size;

    /* Mark the UI as needing an update() call to refresh data state */
    state.state |= UserInterfaceState::NeedsDataUpdate;
}

NodeFlags AbstractUserInterface::nodeFlags(const NodeHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::AbstractUserInterface::nodeFlags(): invalid handle" << handle, {});
    return _state->nodes[nodeHandleId(handle)].used.flags;
}

void AbstractUserInterface::setNodeFlagsInternal(const UnsignedInt id, const NodeFlags flags) {
    State& state = *_state;
    if((state.nodes[id].used.flags & NodeFlag::Hidden) != (flags & NodeFlag::Hidden))
        state.state |= UserInterfaceState::NeedsNodeUpdate;
    state.nodes[id].used.flags = flags;
}

void AbstractUserInterface::setNodeFlags(const NodeHandle handle, const NodeFlags flags) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::AbstractUserInterface::setNodeFlags(): invalid handle" << handle, );
    setNodeFlagsInternal(nodeHandleId(handle), flags);
}

void AbstractUserInterface::addNodeFlags(const NodeHandle handle, const NodeFlags flags) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::AbstractUserInterface::addNodeFlags(): invalid handle" << handle, );
    const UnsignedInt id = nodeHandleId(handle);
    setNodeFlagsInternal(id, _state->nodes[id].used.flags|flags);
}

void AbstractUserInterface::clearNodeFlags(const NodeHandle handle, const NodeFlags flags) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::AbstractUserInterface::clearNodeFlags(): invalid handle" << handle, );
    const UnsignedInt id = nodeHandleId(handle);
    setNodeFlagsInternal(id, _state->nodes[id].used.flags & ~flags);
}

void AbstractUserInterface::removeNode(const NodeHandle handle) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::AbstractUserInterface::removeNode(): invalid handle" << handle, );

    /* If this was a root node, remove it from the visible list (in case it
       was there) */
    State& state = *_state;
    const UnsignedInt id = nodeHandleId(handle);
    if(nodeHandleGeneration(state.nodes[id].used.parentOrOrder) == 0)
        /** @todo call some internal API instead, this repeats all asserts */
        clearNodeOrder(handle);

    removeNodeInternal(id);

    /* Mark the UI as needing a clean() call to refresh node state */
    state.state |= UserInterfaceState::NeedsNodeClean;
}

/* This doesn't handle removal of root nodes from the order list (in case it
   was there), as it's just needed for removeNode() but not clean() where
   only nested nodes with invalid parents are removed */
inline void AbstractUserInterface::removeNodeInternal(const UnsignedInt id) {
    State& state = *_state;
    Node& node = state.nodes[id];

    /* Increase the node generation so existing handles pointing to this
       node are invalidated */
    ++node.used.generation;

    /* Parent the node to the root (and not include it in the order, so all 1s)
       to prevent it from being removed again in clean() when its parents get
       removed as well. Removing more than once would lead to cycles in the
       free list. */
    node.used.parentOrOrder = nodeHandle((1 << Implementation::NodeHandleIdBits) - 1, 0);

    /* If the generation wrapped around, exit without putting it to the free
       list. That makes it disabled, i.e. impossible to be recycled later, to
       avoid aliasing old handles. */
    if(node.used.generation == 1 << Implementation::NodeHandleGenerationBits)
        return;

    /* Put the node at the end of the free list (while they're allocated
       from the front) to not exhaust the generation counter too fast. If the
       free list is empty however, update also the index of the first free
       layer. */
    node.free.next = ~UnsignedInt{};
    if(state.lastFreeNode == ~UnsignedInt{}) {
        CORRADE_INTERNAL_ASSERT(
            state.firstFreeNode == ~UnsignedInt{} &&
            state.lastFreeNode == ~UnsignedInt{});
        state.firstFreeNode = id;
    } else {
        state.nodes[state.lastFreeNode].free.next = id;
    }
    state.lastFreeNode = id;

    /* Nested nodes and data are left dangling and get cleaned up during the
       next clean() call */
}

std::size_t AbstractUserInterface::nodeOrderCapacity() const {
    return _state->nodeOrder.size();
}

std::size_t AbstractUserInterface::nodeOrderUsedCount() const {
    std::size_t free = 0;
    const State& state = *_state;
    UnsignedInt index = state.firstFreeNodeOrder;
    while(index != ~UnsignedInt{}) {
        index = state.nodeOrder[index].free.next;
        ++free;
    }
    return state.nodeOrder.size() - free;
}

NodeHandle AbstractUserInterface::nodeOrderFirst() const {
    return _state->firstNodeOrder;
}

NodeHandle AbstractUserInterface::nodeOrderLast() const {
    const State& state = *_state;
    if(state.firstNodeOrder == NodeHandle::Null)
        return NodeHandle::Null;
    const NodeHandle order = state.nodes[nodeHandleId(state.firstNodeOrder)].used.parentOrOrder;
    CORRADE_INTERNAL_ASSERT(nodeHandleGeneration(order) == 0);
    return state.nodeOrder[nodeHandleId(order)].used.previous;
}

bool AbstractUserInterface::isNodeOrdered(const NodeHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::AbstractUserInterface::isNodeOrdered(): invalid handle" << handle, {});
    const NodeHandle order = _state->nodes[nodeHandleId(handle)].used.parentOrOrder;
    CORRADE_ASSERT(nodeHandleGeneration(order) == 0,
        "Whee::AbstractUserInterface::isNodeOrdered():" << handle << "is not a root node", {});
    return nodeHandleId(order) != (1 << Implementation::NodeHandleIdBits) - 1;
}

NodeHandle AbstractUserInterface::nodeOrderPrevious(const NodeHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::AbstractUserInterface::nodeOrderPrevious(): invalid handle" << handle, {});
    const State& state = *_state;
    const NodeHandle order = state.nodes[nodeHandleId(handle)].used.parentOrOrder;
    CORRADE_ASSERT(nodeHandleGeneration(order) == 0,
        "Whee::AbstractUserInterface::nodeOrderPrevious():" << handle << "is not a root node", {});
    if(state.firstNodeOrder == handle)
        return NodeHandle::Null;
    const UnsignedInt id = nodeHandleId(order);
    if(id == (1 << Implementation::NodeHandleIdBits) - 1)
        return NodeHandle::Null;
    return state.nodeOrder[id].used.previous;
}

NodeHandle AbstractUserInterface::nodeOrderNext(const NodeHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::AbstractUserInterface::nodeOrderNext(): invalid handle" << handle, {});
    const State& state = *_state;
    const NodeHandle order = state.nodes[nodeHandleId(handle)].used.parentOrOrder;
    CORRADE_ASSERT(nodeHandleGeneration(order) == 0,
        "Whee::AbstractUserInterface::nodeOrderNext():" << handle << "is not a root node", {});
    const UnsignedInt id = nodeHandleId(order);
    if(id == (1 << Implementation::NodeHandleIdBits) - 1)
        return NodeHandle::Null;
    const NodeHandle next = state.nodeOrder[id].used.next;
    if(state.firstNodeOrder == next)
        return NodeHandle::Null;
    return next;
}

/* This only removes the node from the order list. Potential updating of the
   node `parentOrOrder` field as well as adding the `orderId` to the free
   list is responsibility of the caller -- only clearNodeOrder() needs to do
   both, setNodeOrder() maybe neither */
void AbstractUserInterface::clearNodeOrderInternal(const NodeHandle handle) {
    State& state = *_state;
    const UnsignedInt orderId = nodeHandleId(state.nodes[nodeHandleId(handle)].used.parentOrOrder);
    CORRADE_INTERNAL_ASSERT(orderId != (1 << Implementation::NodeHandleIdBits) - 1);

    const NodeHandle originalPrevious = state.nodeOrder[orderId].used.previous;
    const NodeHandle originalNext = state.nodeOrder[orderId].used.next;
    CORRADE_INTERNAL_ASSERT(isHandleValid(originalPrevious) && isHandleValid(originalNext));

    const NodeHandle originalPreviousOrder = state.nodes[nodeHandleId(originalPrevious)].used.parentOrOrder;
    const NodeHandle originalNextOrder = state.nodes[nodeHandleId(originalNext)].used.parentOrOrder;
    CORRADE_INTERNAL_ASSERT(nodeHandleGeneration(originalPreviousOrder) == 0);
    CORRADE_INTERNAL_ASSERT(nodeHandleGeneration(originalNextOrder) == 0);

    /* This works correctly also in case of there being just a single item in
       the list (i.e., originalPreviousOrder == originalNextOrder == order), as
       the nodeOrder entry gets unused after */
    state.nodeOrder[nodeHandleId(originalPreviousOrder)].used.next = originalNext;
    state.nodeOrder[nodeHandleId(originalNextOrder)].used.previous = originalPrevious;
    if(state.firstNodeOrder == handle) {
        if(handle == originalNext)
            state.firstNodeOrder = NodeHandle::Null;
        else
            state.firstNodeOrder = originalNext;
    }
}

void AbstractUserInterface::setNodeOrder(const NodeHandle handle, const NodeHandle before) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::AbstractUserInterface::setNodeOrder(): invalid handle" << handle, );
    const UnsignedInt id = nodeHandleId(handle);
    State& state = *_state;
    NodeHandle& order = state.nodes[id].used.parentOrOrder;
    CORRADE_ASSERT(nodeHandleGeneration(order) == 0,
        "Whee::AbstractUserInterface::setNodeOrder():" << handle << "is not a root node", );
    #ifndef CORRADE_NO_ASSERT
    if(before != NodeHandle::Null) {
        CORRADE_ASSERT(isHandleValid(before),
            "Whee::AbstractUserInterface::setNodeOrder(): invalid before handle" << before, );
        CORRADE_ASSERT(handle != before,
            "Whee::AbstractUserInterface::setNodeOrder(): can't order" << handle << "before itself", );
        const NodeHandle nextOrder = state.nodes[nodeHandleId(before)].used.parentOrOrder;
        CORRADE_ASSERT(nodeHandleGeneration(nextOrder) == 0,
            "Whee::AbstractUserInterface::setNodeOrder():" << before << "is not a root node", );
        CORRADE_ASSERT(nodeHandleId(nextOrder) != (1 << Implementation::NodeHandleIdBits) - 1,
            "Whee::AbstractUserInterface::setNodeOrder():" << before << "is not ordered", );
    }
    #endif

    /* If the node isn't in the order yet, add it */
    UnsignedInt orderId = nodeHandleId(order);
    if(orderId == (1 << Implementation::NodeHandleIdBits) - 1) {
        /* Find the first free slot if there is, update the free index to point
           to the next one (or none) */
        if(state.firstFreeNodeOrder != ~UnsignedInt{}) {
            orderId = state.firstFreeNodeOrder;
            state.firstFreeNodeOrder = state.nodeOrder[state.firstFreeNodeOrder].free.next;

        /* If there isn't, allocate a new one */
        } else {
            /* Unlike when adding nodes / layers / ..., we don't need to
               check against max size -- because in that case there wouldn't be
               any free node handles left to call this function with
               anyway */
            orderId = state.nodeOrder.size();
            arrayAppend(state.nodeOrder, NoInit, 1);
        }

        /* Update the ID in the node itself, keeping the generation at 0 */
        order = nodeHandle(orderId, 0);

    /* Otherwise remove it from the previous location in the linked list. The
       `orderId` stays the same -- it's reused. */
    } else clearNodeOrderInternal(handle);

    /* This is the first ever node to be in the order, no need to connect
       with anything else */
    if(state.firstNodeOrder == NodeHandle::Null) {
        CORRADE_INTERNAL_ASSERT(before == NodeHandle::Null);
        state.nodeOrder[orderId].used.previous = handle;
        state.nodeOrder[orderId].used.next = handle;
        state.firstNodeOrder = handle;

    /* Otherwise connect to both previous and next */
    } else {
        const NodeHandle next = before == NodeHandle::Null ?
            state.firstNodeOrder : before;
        const NodeHandle nextOrder = state.nodes[nodeHandleId(next)].used.parentOrOrder;
        CORRADE_INTERNAL_ASSERT(nodeHandleGeneration(nextOrder) == 0);
        const UnsignedInt nextOrderId = nodeHandleId(nextOrder);
        const NodeHandle previous = state.nodeOrder[nextOrderId].used.previous;
        const NodeHandle previousOrder = state.nodes[nodeHandleId(previous)].used.parentOrOrder;
        CORRADE_INTERNAL_ASSERT(nodeHandleGeneration(previousOrder) == 0);

        state.nodeOrder[orderId].used.previous = previous;
        state.nodeOrder[orderId].used.next = next;
        state.nodeOrder[nodeHandleId(previousOrder)].used.next = handle;
        state.nodeOrder[nextOrderId].used.previous = handle;

        /* If the `before` node was first, the new node is now first */
        if(state.firstNodeOrder == before)
            state.firstNodeOrder = handle;
    }

    /* Mark the UI as needing an update() call to refresh node state */
    state.state |= UserInterfaceState::NeedsNodeUpdate;
}

void AbstractUserInterface::clearNodeOrder(const NodeHandle handle) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::AbstractUserInterface::clearNodeOrder(): invalid handle" << handle, );
    State& state = *_state;
    const UnsignedInt id = nodeHandleId(handle);
    const NodeHandle order = state.nodes[id].used.parentOrOrder;
    CORRADE_ASSERT(nodeHandleGeneration(order) == 0,
        "Whee::AbstractUserInterface::clearNodeOrder():" << handle << "is not a root node", );

    /* If the node isn't in the order, nothing to do */
    const UnsignedInt orderId = nodeHandleId(order);
    if(orderId == (1 << Implementation::NodeHandleIdBits) - 1)
        return;

    /* Otherwise remove it from the linked list, put the index to the free list
       and mark the node as not in the order */
    clearNodeOrderInternal(handle);
    state.nodeOrder[orderId].free.next = state.firstFreeNodeOrder;
    state.firstFreeNodeOrder = orderId;
    state.nodes[id].used.parentOrOrder = nodeHandle((1 << Implementation::NodeHandleIdBits) - 1, 0);

    /* Mark the UI as needing an update() call to refresh node state */
    state.state |= UserInterfaceState::NeedsNodeUpdate;
}

AbstractUserInterface& AbstractUserInterface::clean() {
    /* Get the state including what bubbles from layers. If there's nothing to
       clean, bail. No other states should be left after that. */
    const UserInterfaceStates states = this->state();
    if(!(states >= UserInterfaceState::NeedsDataClean)) {
        CORRADE_INTERNAL_ASSERT(!(states >= UserInterfaceState::NeedsNodeClean));
        return *this;
    }

    /* Get total data count in all layers. Invalid (removed) layers have
       instances set to nullptr as well, so this will skip them. */
    std::size_t dataCount = 0;
    State& state = *_state;
    for(const Layer& layer: state.layers) {
        if(const AbstractLayer* const instance = layer.used.instance.get())
            dataCount += instance->capacity();
    }

    /* Single allocation for all temporary data */
    Containers::ArrayView<UnsignedInt> childrenOffsets;
    Containers::ArrayView<UnsignedInt> children;
    Containers::ArrayView<Int> nodeIds;
    Containers::ArrayView<std::size_t> layerDataIdsOffsets;
    Containers::MutableBitArrayView dataIdsToRemove;
    Containers::ArrayTuple storage{
        /* Running children offset (+1) for each node including root (+1) */
        {ValueInit, state.nodes.size() + 2, childrenOffsets},
        {NoInit, state.nodes.size(), children},
        /* One more item for the -1 at the front */
        {NoInit, state.nodes.size() + 1, nodeIds},
        {NoInit, state.layers.size(), layerDataIdsOffsets},
        {ValueInit, dataCount, dataIdsToRemove}
    };

    /* 1. Populate offsets into the bit array view for each layer. Sizes are
       implicitly taken from the layers themselves, so there's no need for any
       extra steps for running offsets. */
    {
        std::size_t dataOffset = 0;
        for(std::size_t i = 0; i != state.layers.size(); ++i) {
            const Layer& layer = state.layers[i];
            if(const AbstractLayer* const instance = layer.used.instance.get()) {
                layerDataIdsOffsets[i] = dataOffset;
                dataOffset += instance->capacity();
            }
        }
        CORRADE_INTERNAL_ASSERT(dataOffset == dataCount);
    }

    /* If no node clean is needed, there's no need to build and iterate an
       ordered list of nodes */
    if(states >= UserInterfaceState::NeedsNodeClean) {
        /* 2. Order the whole node hierarchy */
        /** @todo here it may happen that a handle gets recycled and then
            parented to some of its original children, leading to an
            unreachable cycle and a internal assertion inside, see the
            (skipped) cleanRemoveNestedNodesRecycledHandleOrphanedCycle()
            test */
        Implementation::orderNodesBreadthFirstInto(
            stridedArrayView(state.nodes).slice(&Node::used).slice(&Node::Used::parentOrOrder),
            childrenOffsets, children, nodeIds);

        /* 3. Go through the ordered nodes (skipping the first element which is
           -1) and remove ones that have an invalid parent. Since children are
           ordered after their parents, they'll get subsequently removed as
           well.

           Removed nodes are parented to the root, which prevents them from
           being removed more than once which would lead to a cycle in the free
           list. */
        for(const UnsignedInt id: nodeIds.exceptPrefix(1)) {
            const Node& node = state.nodes[id];
            if(nodeHandleGeneration(node.used.parentOrOrder) != 0 && !isHandleValid(node.used.parentOrOrder))
                removeNodeInternal(id);
        }
    }

    /* 4. Next filter the node data assignments and keep only ones that are
       both attached to (remaining) valid node handles and are valid
       themselves. If no data update is needed, skip this altogether. */
    if(states >= UserInterfaceState::NeedsDataClean) {
        /* Go through all data assignments, mark data attached to invalid nodes
           for deletion, and move valid assignments to a contiguous range at
           the front */
        std::size_t end = 0;
        for(std::size_t i = 0; i != state.data.size(); ++i) {
            const Data& data = state.data[i];
            if(!isHandleValid(data.node)) {
                /* If it's valid data that belongs to a removed node, mark it
                   for removal */
                /** @todo isHandleValid() is still a lot of unnecessary
                    indirections to randomly ordered layers, replacing with
                    something that first populates a "possible removal
                    candidate" list of handles per layer and then passes that
                    to the layer itself where it just compares generations to
                    cull away the already-removed data might be faster ... but
                    maybe also not? */
                if(isHandleValid(data.data))
                    dataIdsToRemove.set(layerDataIdsOffsets[dataHandleLayerId(data.data)] + dataHandleId(data.data));
                continue;
            }
            if(!isHandleValid(data.data))
                continue;
            /* Don't copy to itself */
            if(i != end)
                state.data[end] = state.data[i];
            ++end;
        }
        arrayResize(state.data, end);

        /* Call clean() on all layers, passing a corresponding range of the
           mask to each */
        std::size_t dataOffset = 0;
        for(Layer& layer: state.layers) if(AbstractLayer* const instance = layer.used.instance.get()) {
            instance->clean(dataIdsToRemove.sliceSize(dataOffset, instance->capacity()));
            dataOffset += instance->capacity();
        }
        CORRADE_INTERNAL_ASSERT(dataOffset == dataCount);
    }

    /* Unmark the UI as needing a clean() call, but keep the Update states
       including ones that bubbled up from layers */
    state.state = states & ~((UserInterfaceState::NeedsDataClean|UserInterfaceState::NeedsNodeClean) & ~UserInterfaceState::NeedsNodeUpdate);
    return *this;
}

AbstractUserInterface& AbstractUserInterface::update() {
    /* Call clean implicitly in order to make the internal state ready for
       update. Is a no-op if there's nothing to clean. */
    clean();

    /* Get the state after the clean call including what bubbles from layers.
       If there's nothing to update, bail. No other states should be left after
       that. */
    const UserInterfaceStates states = this->state();
    State& state = *_state;
    if(!(states & UserInterfaceState::NeedsNodeUpdate)) {
        CORRADE_INTERNAL_ASSERT(!state.state);
        return *this;
    }

    /* Single allocation for all temporary data */
    Containers::ArrayView<UnsignedInt> childrenOffsets;
    Containers::ArrayView<UnsignedInt> children;
    Containers::ArrayView<Containers::Triple<UnsignedInt, UnsignedInt, UnsignedInt>> parentsToProcess;
    Containers::MutableBitArrayView visibleNodeMask;
    Containers::ArrayView<UnsignedInt> visibleNodeDataOffsets;
    Containers::ArrayView<DataHandle> visibleNodeData;
    Containers::ArrayView<UnsignedInt> previousDataToUpdateLayerOffsets;
    Containers::ArrayTuple storage{
        /* Running children offset (+1) for each node */
        {ValueInit, state.nodes.size() + 1, childrenOffsets},
        {NoInit, state.nodes.size(), children},
        {NoInit, state.nodes.size(), parentsToProcess},
        {ValueInit, state.nodes.size(), visibleNodeMask},
        /* Running data offset (+1) for each item */
        {ValueInit, state.nodes.size() + 1, visibleNodeDataOffsets},
        {NoInit, state.data.size(), visibleNodeData},
        /* Running data offset (+1) for each item */
        {ValueInit, state.layers.size() + 1, previousDataToUpdateLayerOffsets},
    };

    /* If no node update is needed, the data in `state.nodeStateStorage` and
       all views pointing to it is already up-to-date. */
    if(states >= UserInterfaceState::NeedsNodeUpdate) {
        /* Make a resident allocation for all node-related state */
        state.nodeStateStorage = Containers::ArrayTuple{
            {NoInit, state.nodes.size(), state.visibleNodeIds},
            {NoInit, state.nodes.size(), state.visibleNodeChildrenCounts},
            {NoInit, state.nodeOrder.size(), state.visibleFrontToBackTopLevelNodeIndices},
            {NoInit, state.nodes.size(), state.absoluteNodeOffsets},
        };

        /* 1. Order the visible node hierarchy. */
        {
            const std::size_t visibleCount = Implementation::orderVisibleNodesDepthFirstInto(
                stridedArrayView(state.nodes).slice(&Node::used).slice(&Node::Used::parentOrOrder),
                stridedArrayView(state.nodes).slice(&Node::used).slice(&Node::Used::flags),
                stridedArrayView(state.nodeOrder).slice(&NodeOrder::used).slice(&NodeOrder::Used::next),
                state.firstNodeOrder, childrenOffsets, children,
                parentsToProcess, state.visibleNodeIds, state.visibleNodeChildrenCounts);
            state.visibleNodeIds = state.visibleNodeIds.prefix(visibleCount);
            state.visibleNodeChildrenCounts = state.visibleNodeChildrenCounts.prefix(visibleCount);
        }

        /* 2. Create a front-to-back index map for visible top-level nodes,
           i.e. populate it in a flipped order. */
        {
            const std::size_t count = Implementation::visibleTopLevelNodeIndicesInto(state.visibleNodeChildrenCounts, state.visibleFrontToBackTopLevelNodeIndices.flipped<0>());
            state.visibleFrontToBackTopLevelNodeIndices = state.visibleFrontToBackTopLevelNodeIndices.exceptPrefix(state.visibleFrontToBackTopLevelNodeIndices.size() - count);
        }
    }

    /* If no layout update is needed, the `state.absoluteNodeOffsets` are all
       up-to-date */
    if(states >= UserInterfaceState::NeedsNodeLayoutUpdate) {
        /* 3. Calculate absolute offsets and clip rects for visible nodes. */
        for(const UnsignedInt id: state.visibleNodeIds) {
            const Node& node = state.nodes[id];
            state.absoluteNodeOffsets[id] =
                nodeHandleGeneration(node.used.parentOrOrder) == 0 ? node.used.offset :
                    state.absoluteNodeOffsets[nodeHandleId(node.used.parentOrOrder)] + node.used.offset;
        }

        /* 4. Cull / clip the visible nodes based on their clip rects */

        /** @todo implement; might want also a layer-specific cull / clip
            implementation that gets called after the "upload" step */
    }

    /* If no data attachment update is needed, the data in
       `state.dataStateStorage` and all views pointing to it is already
       up-to-date. */
    if(states >= UserInterfaceState::NeedsDataAttachmentUpdate) {
        /* Make a resident allocation for all node-related state */
        state.dataStateStorage = Containers::ArrayTuple{
            /* Running data offset (+1) for each item */
            {ValueInit, state.layers.size() + 1, state.dataToUpdateLayerOffsets},
            {NoInit, state.data.size(), state.dataToUpdateIds},
            {NoInit, state.data.size(), state.dataToUpdateNodeIds},
            {NoInit, state.data.size(), state.dataToDrawLayerIds},
            {NoInit, state.data.size(), state.dataToDrawOffsets},
            {NoInit, state.data.size(), state.dataToDrawSizes},
            /* Running data offset (+1) for each item */
            {ValueInit, state.nodes.size() + 1, state.visibleNodeEventDataOffsets},
            {NoInit, state.data.size(), state.visibleNodeEventData},
        };

        /* 5. Order data assignments of visible nodes so each layer is a
           contiguous range, and inside the layer the order follows the visible
           node hierarchy order from above. */
        state.drawCount = Implementation::orderVisibleNodeDataInto(
            state.visibleNodeIds,
            state.visibleNodeChildrenCounts,
            stridedArrayView(state.data).slice(&Data::node),
            stridedArrayView(state.data).slice(&Data::data),
            stridedArrayView(state.layers).slice(&Layer::used).slice(&Layer::Used::next),
            state.firstLayer,
            stridedArrayView(state.layers).slice(&Layer::used).slice(&Layer::Used::features),
            visibleNodeMask,
            visibleNodeDataOffsets,
            visibleNodeData,
            state.dataToUpdateLayerOffsets,
            previousDataToUpdateLayerOffsets,
            state.dataToUpdateIds,
            state.dataToUpdateNodeIds,
            state.dataToDrawLayerIds,
            state.dataToDrawOffsets,
            state.dataToDrawSizes);

        /* 6. Filter data in visible nodes in a front-to-back order for event
           handling. */
        Implementation::orderNodeDataForEventHandling(
            visibleNodeDataOffsets,
            visibleNodeData,
            stridedArrayView(state.layers).slice(&Layer::used).slice(&Layer::Used::previous),
            layerLast(),
            stridedArrayView(state.layers).slice(&Layer::used).slice(&Layer::Used::features),
            state.visibleNodeEventDataOffsets,
            state.visibleNodeEventData);
    }

    /* 7. For each layer submit an update of visible data across all visible
       top-level nodes. If no data update is needed, the data in layers is
       already up-to-date. */
    if(states >= UserInterfaceState::NeedsDataUpdate) for(std::size_t i = 0; i != state.layers.size(); ++i) {
        /* Invalid (removed) layers have instances set to nullptr as well, so
           this will skip them. */
        AbstractLayer* const instance = state.layers[i].used.instance.get();
        if(!instance)
            continue;

        /* Call update() on the layer even if the particular data range is
           empty in order to allow the implementations to do various cleanups.
           Plus the update() call resets the NeedsUpdate state on the layer,
           if it's set. */
        /** @todo include a bitmask of what node offsets/sizes actually
            changed */
        instance->update(
            state.dataToUpdateIds.slice(
                state.dataToUpdateLayerOffsets[i],
                state.dataToUpdateLayerOffsets[i + 1]),
            state.dataToUpdateNodeIds.slice(
                state.dataToUpdateLayerOffsets[i],
                state.dataToUpdateLayerOffsets[i + 1]),
            /** @todo some layer implementations may eventually want relative
                offsets, not absolute, provide both? */
            state.absoluteNodeOffsets,
            stridedArrayView(state.nodes).slice(&Node::used).slice(&Node::Used::size));
    }

    /** @todo layer-specific cull/clip step? */

    /* Unmark the UI as needing an update() call. No other states should be
       left after that, i.e. the UI should be ready for drawing and event
       processing. */
    state.state &= ~UserInterfaceState::NeedsNodeUpdate;
    CORRADE_INTERNAL_ASSERT(!state.state);
    return *this;
}

AbstractUserInterface& AbstractUserInterface::draw() {
    /* Call update implicitly in order to make the internal state ready for
       drawing. Is a no-op if there's nothing to update or clean. */
    update();

    /* Then submit draws in the correct back-to-front order, i.e. for every
       top-level node and then for every layer used by its children */
    State& state = *_state;
    for(std::size_t i = 0; i != state.drawCount; ++i) {
        const UnsignedInt layerId = state.dataToDrawLayerIds[i];
        state.layers[layerId].used.instance->draw(
            /* The views should be exactly the same as passed to update()
               before ... */
            state.dataToUpdateIds.slice(
                state.dataToUpdateLayerOffsets[layerId],
                state.dataToUpdateLayerOffsets[layerId + 1]),
            state.dataToUpdateNodeIds.slice(
                state.dataToUpdateLayerOffsets[layerId],
                state.dataToUpdateLayerOffsets[layerId + 1]),
            /* ... and the draw offset then being relative to those */
            state.dataToDrawOffsets[i] - state.dataToUpdateLayerOffsets[layerId],
            state.dataToDrawSizes[i],
            state.absoluteNodeOffsets,
            stridedArrayView(state.nodes).slice(&Node::used).slice(&Node::Used::size));
    }

    return *this;
}

template<class Event, void(AbstractLayer::*function)(UnsignedInt, Event&)> bool AbstractUserInterface::callEvent(const Vector2& globalPosition, UnsignedInt visibleNodeIndex, Event& event) {
    CORRADE_INTERNAL_ASSERT(!event.isAccepted());
    State& state = *_state;
    const UnsignedInt nodeId = state.visibleNodeIds[visibleNodeIndex];

    /* If the position is outside the node, we got nothing */
    const Vector2 nodeOffset = state.absoluteNodeOffsets[nodeId];
    if((globalPosition < nodeOffset).any() ||
       (globalPosition >= nodeOffset + state.nodes[nodeId].used.size).any())
        return false;

    /* If the position is inside, recurse into *direct* children. If the event
       is handled there, we're done. */
    for(UnsignedInt i = 1, iMax = state.visibleNodeChildrenCounts[visibleNodeIndex] + 1; i != iMax; i += state.visibleNodeChildrenCounts[visibleNodeIndex + i] + 1)
        if(callEvent<Event, function>(globalPosition, visibleNodeIndex + i, event))
            return true;

    /* Only if children didn't handle the event, look into this node data */
    const Vector2 position = globalPosition - nodeOffset;
    for(UnsignedInt j = state.visibleNodeEventDataOffsets[nodeId], jMax = state.visibleNodeEventDataOffsets[nodeId + 1]; j != jMax; ++j) {
        const DataHandle data = state.visibleNodeEventData[j];
        event._position = position;
        ((*state.layers[dataHandleLayerId(data)].used.instance).*function)(dataHandleId(data), event);
        if(event.isAccepted())
            return true;
    }

    return false;
}

template<class Event, void(AbstractLayer::*function)(UnsignedInt, Event&)> bool AbstractUserInterface::callEvent(const Vector2& globalPosition, Event& event) {
    /* Call update implicitly in order to make the internal state ready for
       event processing. Is a no-op if there's nothing to update or clean. */
    update();

    for(const UnsignedInt visibleTopLevelNodeIndex: _state->visibleFrontToBackTopLevelNodeIndices)
        if(callEvent<Event, function>(globalPosition, visibleTopLevelNodeIndex, event))
            return true;

    return false;
}

bool AbstractUserInterface::pointerPressEvent(const Vector2& globalPosition, PointerEvent& event) {
    CORRADE_ASSERT(!event.isAccepted(),
        "Whee::AbstractUserInterface::pointerPressEvent(): event already accepted", {});

    return callEvent<PointerEvent, &AbstractLayer::pointerPressEvent>(globalPosition, event);
}

bool AbstractUserInterface::pointerReleaseEvent(const Vector2& globalPosition, PointerEvent& event) {
    CORRADE_ASSERT(!event.isAccepted(),
        "Whee::AbstractUserInterface::pointerReleaseEvent(): event already accepted", {});

    return callEvent<PointerEvent, &AbstractLayer::pointerReleaseEvent>(globalPosition, event);
}

}}
