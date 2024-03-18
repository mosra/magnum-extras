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
#include <Corrade/Containers/BitArray.h>
#include <Corrade/Containers/EnumSet.hpp>
#include <Corrade/Containers/GrowableArray.h>
#include <Corrade/Containers/Optional.h>
#include <Magnum/Math/Vector2.h>

#include "Magnum/Whee/AbstractLayer.h"
#include "Magnum/Whee/AbstractLayouter.h"
#include "Magnum/Whee/Event.h"
#include "Magnum/Whee/Handle.h"
#include "Magnum/Whee/Implementation/abstractUserInterface.h"

namespace Magnum { namespace Whee {

Debug& operator<<(Debug& debug, const UserInterfaceState value) {
    debug << "Whee::UserInterfaceState" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case UserInterfaceState::value: return debug << "::" #value;
        _c(NeedsDataUpdate)
        _c(NeedsDataAttachmentUpdate)
        _c(NeedsNodeClipUpdate)
        _c(NeedsLayoutUpdate)
        _c(NeedsLayoutAssignmentUpdate)
        _c(NeedsNodeUpdate)
        _c(NeedsNodeClean)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "(" << Debug::nospace << Debug::hex << UnsignedByte(value) << Debug::nospace << ")";
}

Debug& operator<<(Debug& debug, const UserInterfaceStates value) {
    return Containers::enumSetDebugOutput(debug, value, "Whee::UserInterfaceStates{}", {
        UserInterfaceState::NeedsNodeClean,
        /* Implied by NeedsNodeClean, has to be after */
        UserInterfaceState::NeedsNodeUpdate,
        /* Implied by NeedsNodeUpdate, has to be after */
        UserInterfaceState::NeedsLayoutAssignmentUpdate,
        /* Implied by NeedsLayoutAssignmentUpdate, has to be after */
        UserInterfaceState::NeedsLayoutUpdate,
        /* Implied by NeedsLayoutUpdate, has to be after */
        UserInterfaceState::NeedsNodeClipUpdate,
        /* Implied by NeedsNodeClipUpdate, has to be after */
        UserInterfaceState::NeedsDataAttachmentUpdate,
        /* Implied by NeedsDataAttachmentUpdate, has to be after */
        UserInterfaceState::NeedsDataUpdate
    });
}

Debug& operator<<(Debug& debug, const NodeFlag value) {
    debug << "Whee::NodeFlag" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case NodeFlag::value: return debug << "::" #value;
        _c(Hidden)
        _c(Clip)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "(" << Debug::nospace << Debug::hex << UnsignedByte(value) << Debug::nospace << ")";
}

Debug& operator<<(Debug& debug, const NodeFlags value) {
    return Containers::enumSetDebugOutput(debug, value, "Whee::NodeFlags{}", {
        NodeFlag::Hidden,
        NodeFlag::Clip
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

union Layouter {
    explicit Layouter() noexcept: used{} {}
    Layouter(const Layouter&&) = delete;
    Layouter(Layouter&& other) noexcept: used{Utility::move(other.used)} {}
    ~Layouter() {
        used.instance.~Pointer();
    }
    Layouter& operator=(const Layouter&&) = delete;
    Layouter& operator=(Layouter&& other) noexcept {
        Utility::swap(other.used, used);
        return *this;
    }

    struct Used {
        /* Except for the generation (and instance, which is reset during
           removal), these have to be re-filled every time a handle is
           recycled, so it doesn't make sense to initialize them to
           anything. */

        /* Layouter instance. Null for newly created layouters until
           setLayouterInstance() is called, set back to null in
           removeLayouter(). */
        Containers::Pointer<AbstractLayouter> instance;

        /* Together with index of this item in `layouters` used for creating a
           LayouterHandle. Increased every time a handle reaches
           removeLayouter(). Has to be initially non-zero to differentiate the
           first ever handle (with index 0) from LayouterHandle::Null. Once
           wraps back to zero once the handle gets disabled. */
        UnsignedByte generation = 1;

        /* 1 byte free */

        /* Always meant to be non-null and valid. To make insert/remove
           operations easier the list is cyclic, so the last layouters's `next`
           is the same as `_state->firstLayouter`. */
        LayouterHandle previous;
        LayouterHandle next;

        /* 2 bytes free */
    } used;

    /* Used only if the Layouter is among the free ones */
    struct Free {
        /* The instance pointer value has to be preserved in order to be able
           to distinguish between used layouters with set instances and others
           (free or not set yet), which is used for iterating them in clean()
           and update(). If free, the member is always null, to not cause
           issues when moving & destructing. There's no other way to
           distinguish free and used layouters apart from walking the free
           list. */
        void* instance;

        /* The generation value has to be preserved in order to increment it
           next time it gets used */
        UnsignedByte generation;

        /* See State::firstFreeLayouter for more information. Has to be larger
           than 8 bits in order to distinguish between index 255 and "no next
           free layouter" (which is now 65535). */
        UnsignedShort next;
    } free;
};

static_assert(
    offsetof(Layouter::Used, instance) == offsetof(Layouter::Free, instance) &&
    offsetof(Layouter::Used, generation) == offsetof(Layouter::Free, generation),
    "Layouter::Used and Free layout not compatible");

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

    /* Layouters, indexed by LayouterHandle */
    Containers::Array<Layouter> layouters;
    /* The `Layouter` then has a `next` member containing the next layouter in
       the draw order. To make insert/remove operations easier the list is
       cyclic, so the last layouter's `next` is the same as `firstLayouter`. */
    LayouterHandle firstLayouter = LayouterHandle::Null;
    /* Indices into the `layouters` array. The `Layouter` then has a `nextFree`
       member containing the next free index. To avoid repeatedly reusing the
       same handles and exhausting their generation counter too soon, new
       layouters get taken from the front and removed are put at the end. A
       value with all bits set means there's no (first/next/last) free
       layouter. */
    UnsignedShort firstFreeLayouter = 0xffffu;
    UnsignedShort lastFreeLayouter = 0xffffu;

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

    /* Set by setSize(), checked in update(), used for event scaling and
       passing to layers */
    Vector2 size;
    Vector2 windowSize;
    Vector2i framebufferSize;

    /* Tracks whether update() and clean() needs to do something */
    UserInterfaceStates state;

    /* Node on which a pointer press event was accepted and which will receive
       a pointer tap or click event on a release if it happens on its area.
       Becomes null after a release or if an uncaptured pointer move event
       leaves the node area. */
    NodeHandle pointerEventPressedNode = NodeHandle::Null;
    /* Node on which a pointer press event was accepted & captured and which
       will receive remaining pointer events until a pointer release. If null,
       a pointer isn't pressed, a capture was disabled, or the captured node
       got removed or hidden since. */
    NodeHandle pointerEventCapturedNode = NodeHandle::Null;
    /* Node on which the last pointer move event happened. The node already
       received a pointer enter event and will receive a pointer leave event on
       the next pointer move event that leaves its area. If null, no pointer
       event happened yet or the hovered node got removed or hidden since. */
    NodeHandle pointerEventHoveredNode = NodeHandle::Null;
    /* Position of the previous pointer event, scaled to the UI size. NullOpt
       if there was no pointer event yet. */
    /** @todo maintain previous position per pointer type? i.e., mouse, pen and
        finger independently? */
    Containers::Optional<Vector2> pointerEventPreviousGlobalPositionScaled;

    /* Data for updates, event handling and drawing, repopulated by clean() and
       update() */
    Containers::ArrayTuple nodeStateStorage;
    Containers::ArrayView<UnsignedInt> visibleNodeIds;
    Containers::ArrayView<UnsignedInt> visibleNodeChildrenCounts;
    Containers::StridedArrayView1D<UnsignedInt> visibleFrontToBackTopLevelNodeIndices;
    Containers::ArrayView<Vector2> nodeOffsets;
    Containers::ArrayView<Vector2> nodeSizes;
    Containers::ArrayView<Vector2> absoluteNodeOffsets;
    Containers::MutableBitArrayView visibleNodeMask;
    Containers::ArrayView<Vector2> clipRectOffsets;
    Containers::ArrayView<Vector2> clipRectSizes;
    Containers::ArrayView<UnsignedInt> clipRectNodeCounts;
    Containers::ArrayTuple layoutStateStorage;
    Containers::ArrayView<UnsignedInt> topLevelLayoutOffsets;
    Containers::ArrayView<UnsignedByte> topLevelLayoutLayouterIds;
    Containers::ArrayView<UnsignedInt> topLevelLayoutIds;
    /** @todo this is a separate allocation from layoutStateStorage, unify
        somehow */
    Containers::BitArray layoutMasks;
    Containers::ArrayTuple dataStateStorage;
    Containers::ArrayView<Containers::Pair<UnsignedInt, UnsignedInt>> dataToUpdateLayerOffsets;
    Containers::ArrayView<UnsignedInt> dataToUpdateIds;
    Containers::ArrayView<UnsignedInt> dataToUpdateClipRectIds;
    Containers::ArrayView<UnsignedInt> dataToUpdateClipRectDataCounts;
    Containers::ArrayView<UnsignedByte> dataToDrawLayerIds;
    Containers::ArrayView<UnsignedInt> dataToDrawOffsets;
    Containers::ArrayView<UnsignedInt> dataToDrawSizes;
    Containers::ArrayView<UnsignedInt> dataToDrawClipRectOffsets;
    Containers::ArrayView<UnsignedInt> dataToDrawClipRectSizes;
    /* Indexed by node ID in order to make it possible to look up node data by
       node ID, however contains data only for visible nodes */
    Containers::ArrayView<UnsignedInt> visibleNodeEventDataOffsets;
    Containers::ArrayView<DataHandle> visibleNodeEventData;
    UnsignedInt drawCount = 0, clipRectCount = 0;
};

AbstractUserInterface::AbstractUserInterface(NoCreateT): _state{InPlaceInit} {}

AbstractUserInterface::AbstractUserInterface(const Vector2& size, const Vector2& windowSize, const Vector2i& framebufferSize): AbstractUserInterface{NoCreate} {
    setSize(size, windowSize, framebufferSize);
}

AbstractUserInterface::AbstractUserInterface(const Vector2i& size): AbstractUserInterface{Vector2{size}, Vector2{size}, size} {}

AbstractUserInterface::AbstractUserInterface(AbstractUserInterface&&) noexcept = default;

AbstractUserInterface& AbstractUserInterface::operator=(AbstractUserInterface&&) noexcept = default;

AbstractUserInterface::~AbstractUserInterface() = default;

Vector2 AbstractUserInterface::size() const {
    return _state->size;
}

Vector2 AbstractUserInterface::windowSize() const {
    return _state->windowSize;
}

Vector2i AbstractUserInterface::framebufferSize() const {
    return _state->framebufferSize;
}

AbstractUserInterface& AbstractUserInterface::setSize(const Vector2& size, const Vector2& windowSize, const Vector2i& framebufferSize) {
    CORRADE_ASSERT(size.product() && windowSize.product() && framebufferSize.product(),
        "Whee::AbstractUserInterface::setSize(): expected non-zero sizes, got" << size << Debug::nospace << "," << windowSize << "and" << framebufferSize, *this);
    State& state = *_state;
    const bool sizeOrFramebufferSizeDifferent = state.size != size || state.framebufferSize != framebufferSize;
    state.size = size;
    state.windowSize = windowSize;
    state.framebufferSize = framebufferSize;

    /* If the size or framebuffer size is different, set it on all existing
       layers that have an instance (so, also aren't freed) and support
       drawing. Layers that don't have an instance set yet will get it proxied
       directly in their setLayerInstance() call. */
    if(sizeOrFramebufferSizeDifferent) for(std::size_t i = 0; i != state.layers.size(); ++i) {
        Layer& layer = state.layers[i];
        AbstractLayer* const instance = layer.used.instance.get();
        if(layer.used.features & LayerFeature::Draw && instance)
            instance->setSize(size, framebufferSize);
    }

    return *this;
}

AbstractUserInterface& AbstractUserInterface::setSize(const Vector2i& size) {
    return setSize(Vector2{size}, Vector2{size}, size);
}

UserInterfaceStates AbstractUserInterface::state() const {
    const State& state = *_state;
    UserInterfaceStates states;

    /* Unless UserInterfaceState::NeedsLayoutAssignmentUpdate is set already,
       go through all layouters and inherit the Needs* flags from them. Invalid
       (removed) layouters have instances set to nullptr, so this will skip
       them. */
    if(!(state.state >= UserInterfaceState::NeedsLayoutAssignmentUpdate)) for(const Layouter& layouter: state.layouters) {
        if(const AbstractLayouter* const instance = layouter.used.instance.get()) {
            const LayouterStates layouterState = instance->state();
            if(layouterState >= LayouterState::NeedsUpdate)
                states |= UserInterfaceState::NeedsLayoutUpdate;
            if(layouterState >= LayouterState::NeedsAssignmentUpdate)
                states |= UserInterfaceState::NeedsLayoutAssignmentUpdate;

            /* There's no broader state than this so if it's set, we can stop
               iterating further */
            if(states == UserInterfaceState::NeedsLayoutAssignmentUpdate)
                break;
        }
    }

    /* Unless UserInterfaceState::NeedsDataAttachmentUpdate is set already, go
       through all layers and inherit the Needs* flags from them. Invalid
       (removed) layers have instances set to nullptr as well, so this will
       skip them. */
    if(!(state.state >= UserInterfaceState::NeedsDataAttachmentUpdate)) for(const Layer& layer: state.layers) {
        if(const AbstractLayer* const instance = layer.used.instance.get()) {
            const LayerStates layerState = instance->state();
            if(layerState >= LayerState::NeedsUpdate)
                states |= UserInterfaceState::NeedsDataUpdate;
            if(layerState >= LayerState::NeedsAttachmentUpdate)
                states |= UserInterfaceState::NeedsDataAttachmentUpdate;

            /* There's no broader state than this so if it's set, we can stop
               iterating further */
            if(states == UserInterfaceState::NeedsDataAttachmentUpdate)
                break;
        }
    }

    return state.state|states;
}

std::size_t AbstractUserInterface::layerCapacity() const {
    return _state->layers.size();
}

std::size_t AbstractUserInterface::layerUsedCount() const {
    /* The "pointer" chasing in here is a bit nasty, but there's no other way
       to know which layers are actually used and which not. The instance is a
       nullptr for unused layers, yes, but it's also null for layers that don't
       have it set yet. */
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
        return false;
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

AbstractLayer& AbstractUserInterface::setLayerInstance(Containers::Pointer<AbstractLayer>&& instance) {
    State& state = *_state;
    CORRADE_ASSERT(instance,
        "Whee::AbstractUserInterface::setLayerInstance(): instance is null", *state.layers[0].used.instance);
    const LayerHandle handle = instance->handle();
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::AbstractUserInterface::setLayerInstance(): invalid handle" << handle, *state.layers[0].used.instance);
    const UnsignedInt id = layerHandleId(handle);
    CORRADE_ASSERT(!state.layers[id].used.instance,
        "Whee::AbstractUserInterface::setLayerInstance(): instance for" << handle << "already set", *state.layers[0].used.instance);
    Layer& layer = state.layers[id];
    layer.used.features = instance->features();
    layer.used.instance = Utility::move(instance);

    /* If the size is already set, immediately proxy it to the layer. If it
       isn't, it gets done during the next setSize() call. */
    if(!state.size.isZero() && layer.used.features & LayerFeature::Draw)
        layer.used.instance->setSize(state.size, state.framebufferSize);

    return *layer.used.instance;
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

    /* Mark the UI as needing an update() call to refresh per-node data
       lists */
    state.state |= UserInterfaceState::NeedsDataAttachmentUpdate;
}

void AbstractUserInterface::attachData(const NodeHandle node, const DataHandle data) {
    CORRADE_ASSERT(node == NodeHandle::Null || isHandleValid(node),
        "Whee::AbstractUserInterface::attachData(): invalid handle" << node, );
    CORRADE_ASSERT(isHandleValid(data),
        "Whee::AbstractUserInterface::attachData(): invalid handle" << data, );
    /** @todo this performs the data handle validity check redundantly again,
        consider using some internal assert-less helper if it proves to be a
        bottleneck */
    _state->layers[dataHandleLayerId(data)].used.instance->attach(dataHandleData(data), node);

    /* The AbstractLayer::attach() call then sets an appropriate LayerState,
       nothing to set here */
}

std::size_t AbstractUserInterface::layouterCapacity() const {
    return _state->layouters.size();
}

std::size_t AbstractUserInterface::layouterUsedCount() const {
    /* The "pointer" chasing in here is a bit nasty, but there's no other way
       to know which layouters are actually used and which not. The instance is
       a nullptr for unused layouters, yes, but it's also null for layouters
       that don't have it set yet. */
    std::size_t free = 0;
    const State& state = *_state;
    UnsignedShort index = state.firstFreeLayouter;
    /* C, FUCK your STUPID conversion rules. Why isn't ~UnsignedShort{} 65535?
       Why it evaluates to -1?! Why UnsignedShort(~UnsignedShort{}) is the only
       way to put it back to 16 bits? Why is everything so fucking awful?! Why
       do you force me to use error-prone constants, FFS?! */
    while(index != 0xffffu) {
        index = state.layouters[index].free.next;
        ++free;
    }
    return state.layouters.size() - free;
}

bool AbstractUserInterface::isHandleValid(const LayouterHandle handle) const {
    if(handle == LayouterHandle::Null)
        return false;
    const UnsignedInt index = layouterHandleId(handle);
    const State& state = *_state;
    if(index >= state.layouters.size())
        return false;
    /* Zero generation (i.e., where it wrapped around from 255) is also
       invalid.

       Note that this can still return true for manually crafted handles that
       point to free nodes with correct generation counters. The only way to
       detect that would be by either iterating the free list (slow) or by
       keeping an additional bitfield marking free items. I don't think that's
       necessary. */
    const UnsignedInt generation = layouterHandleGeneration(handle);
    return generation && generation == state.layouters[index].used.generation;
}

bool AbstractUserInterface::isHandleValid(const LayoutHandle handle) const {
    if(layoutHandleData(handle) == LayouterDataHandle::Null ||
       layoutHandleLayouter(handle) == LayouterHandle::Null)
        return false;
    const UnsignedInt layouterIndex = layoutHandleLayouterId(handle);
    const State& state = *_state;
    if(layouterIndex >= state.layouters.size())
        return false;
    const Layouter& layouter = state.layouters[layouterIndex];
    if(!layouter.used.instance)
        return false;
    return layoutHandleLayouterGeneration(handle) == layouter.used.generation && layouter.used.instance->isHandleValid(layoutHandleData(handle));
}

LayouterHandle AbstractUserInterface::layouterFirst() const {
    return _state->firstLayouter;
}

LayouterHandle AbstractUserInterface::layouterLast() const {
    const State& state = *_state;
    if(state.firstLayouter == LayouterHandle::Null)
        return LayouterHandle::Null;
    return state.layouters[layouterHandleId(state.firstLayouter)].used.previous;
}

LayouterHandle AbstractUserInterface::layouterPrevious(LayouterHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::AbstractUserInterface::layouterPrevious(): invalid handle" << handle, {});
    const State& state = *_state;
    if(state.firstLayouter == handle)
        return LayouterHandle::Null;
    return state.layouters[layouterHandleId(handle)].used.previous;
}

LayouterHandle AbstractUserInterface::layouterNext(LayouterHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::AbstractUserInterface::layouterNext(): invalid handle" << handle, {});
    const State& state = *_state;
    const LayouterHandle next = state.layouters[layouterHandleId(handle)].used.next;
    if(state.firstLayouter == next)
        return LayouterHandle::Null;
    return next;
}

LayouterHandle AbstractUserInterface::createLayouter(const LayouterHandle before) {
    CORRADE_ASSERT(before == LayouterHandle::Null || isHandleValid(before),
        "Whee::AbstractUserInterface::createLayouter(): invalid before handle" << before, {});

    State& state = *_state;

    /* Find the first free layouter if there is, update the free index to point
       to the next one (or none) */
    Layouter* layouter;
    /* C, FUCK your STUPID conversion rules. Why isn't ~UnsignedShort{} 65535?
       Why it evaluates to -1?! Why UnsignedShort(~UnsignedShort{}) is the only
       way to put it back to 16 bits? Why is everything so fucking awful?! Why
       do you force me to use error-prone constants, FFS?! */
    if(state.firstFreeLayouter != 0xffffu) {
        layouter = &state.layouters[state.firstFreeLayouter];
        /* If there's just one item in the list, make the list empty */
        if(state.firstFreeLayouter == state.lastFreeLayouter) {
            CORRADE_INTERNAL_ASSERT(layouter->free.next == 0xffffu);
            state.firstFreeLayouter = state.lastFreeLayouter = 0xffffu;
        } else {
            state.firstFreeLayouter = layouter->free.next;
        }

    /* If there isn't, allocate a new one */
    } else {
        CORRADE_ASSERT(state.layouters.size() < 1 << Implementation::LayouterHandleIdBits,
            "Whee::AbstractUserInterface::createLayouter(): can only have at most" << (1 << Implementation::LayouterHandleIdBits) << "layouters", {});
        layouter = &arrayAppend(state.layouters, InPlaceInit);
    }

    /* In both above cases the generation is already set appropriately, either
       initialized to 1, or incremented when it got remove()d (to mark existing
       handles as invalid) */
    const LayouterHandle handle = layouterHandle((layouter - state.layouters), layouter->used.generation);

    /* This is the first ever layouter, no need to connect with anything else */
    if(state.firstLayouter == LayouterHandle::Null) {
        CORRADE_INTERNAL_ASSERT(before == LayouterHandle::Null);
        layouter->used.previous = handle;
        layouter->used.next = handle;
        state.firstLayouter = handle;
        return handle;
    }

    const LayouterHandle next = before == LayouterHandle::Null ?
        state.firstLayouter : before;
    const LayouterHandle previous = state.layouters[layouterHandleId(next)].used.previous;
    layouter->used.previous = previous;
    layouter->used.next = next;
    state.layouters[layouterHandleId(next)].used.previous = handle;
    state.layouters[layouterHandleId(previous)].used.next = handle;

    /* If the `before` layouter was first, the new layouter is now first */
    if(state.firstLayouter == before)
        state.firstLayouter = handle;

    return handle;
}

AbstractLayouter& AbstractUserInterface::setLayouterInstance(Containers::Pointer<AbstractLayouter>&& instance) {
    State& state = *_state;
    CORRADE_ASSERT(instance,
        "Whee::AbstractUserInterface::setLayouterInstance(): instance is null", *state.layouters[0].used.instance);
    const LayouterHandle handle = instance->handle();
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::AbstractUserInterface::setLayouterInstance(): invalid handle" << handle, *state.layouters[0].used.instance);
    const UnsignedInt id = layouterHandleId(handle);
    CORRADE_ASSERT(!state.layouters[id].used.instance,
        "Whee::AbstractUserInterface::setLayouterInstance(): instance for" << handle << "already set", *state.layouters[0].used.instance);
    Layouter& layouter = state.layouters[id];
    layouter.used.instance = Utility::move(instance);

    return *layouter.used.instance;
}

const AbstractLayouter& AbstractUserInterface::layouter(const LayouterHandle handle) const {
    const State& state = *_state;
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::AbstractUserInterface::layouter(): invalid handle" << handle,
        *state.layouters[0].used.instance);
    const UnsignedInt id = layouterHandleId(handle);
    CORRADE_ASSERT(state.layouters[id].used.instance,
        "Whee::AbstractUserInterface::layouter():" << handle << "has no instance set", *state.layouters[0].used.instance);
    return *state.layouters[id].used.instance;
}

AbstractLayouter& AbstractUserInterface::layouter(const LayouterHandle handle) {
    return const_cast<AbstractLayouter&>(const_cast<const AbstractUserInterface&>(*this).layouter(handle));
}

void AbstractUserInterface::removeLayouter(const LayouterHandle handle) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::AbstractUserInterface::removeLayouter(): invalid handle" << handle, );
    const UnsignedInt id = layouterHandleId(handle);
    State& state = *_state;
    Layouter& layouter = state.layouters[id];

    const LayouterHandle originalPrevious = layouter.used.previous;
    const LayouterHandle originalNext = layouter.used.next;
    CORRADE_INTERNAL_ASSERT(isHandleValid(originalPrevious) && isHandleValid(originalNext));

    /* This works correctly also in case of there being just a single item in
       the list (i.e., originalPrevious == originalNext == handle), as the item
       gets unused after */
    state.layouters[layouterHandleId(originalPrevious)].used.next = originalNext;
    state.layouters[layouterHandleId(originalNext)].used.previous = originalPrevious;
    if(state.firstLayouter == handle) {
        if(handle == originalNext)
            state.firstLayouter = LayouterHandle::Null;
        else
            state.firstLayouter = originalNext;
    }

    /* Delete the instance. The instance being null then means that the
       layouter is either free or is newly created until setLayouterInstance()
       is called, which is used for iterating them in clean() and update(). */
    layouter.used.instance = nullptr;

    /* Increase the layouter generation so existing handles pointing to this
       layouter are invalidated */
    ++layouter.used.generation;

    /* Put the layouter at the end of the free list (while they're allocated
       from the front) to not exhaust the generation counter too fast. If the
       free list is empty however, update also the index of the first free
       layouter.

       Don't do this if the generation wrapped around. That makes it disabled,
       i.e. impossible to be recycled later, to avoid aliasing old handles. */
    if(layouter.used.generation != 0) {
        /* C, FUCK your STUPID conversion rules. Why isn't ~UnsignedShort{}
           65535? Why it evaluates to -1?! Why UnsignedShort(~UnsignedShort{})
           is the only way to put it back to 16 bits? Why is everything so
           fucking awful?! Why do you force me to use error-prone constants,
           FFS?! */
        layouter.free.next = 0xffffu;
        if(state.lastFreeLayouter == 0xffffu) {
            CORRADE_INTERNAL_ASSERT(
                state.firstFreeLayouter == 0xffffu &&
                state.lastFreeLayouter == 0xffffu);
            state.firstFreeLayouter = id;
        } else {
            state.layouters[state.lastFreeLayouter].free.next = id;
        }
        state.lastFreeLayouter = id;
    }

    /* Mark the UI as needing an update() call to refresh per-node layout
       lists */
    state.state |= UserInterfaceState::NeedsLayoutAssignmentUpdate;
}

std::size_t AbstractUserInterface::nodeCapacity() const {
    return _state->nodes.size();
}

std::size_t AbstractUserInterface::nodeUsedCount() const {
    /* The "pointer" chasing in here is a bit nasty, but there's no other way
       to know which nodes are actually used and which not. The parentOrOrder
       is a certain bit pattern for unused nodes, yes, but it's also the same
       pattern for top-level nodes that aren't included in draw order. */
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
    state.state |= UserInterfaceState::NeedsLayoutUpdate;
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

    /* Mark the UI as needing an update() call to refresh node layout state */
    state.state |= UserInterfaceState::NeedsLayoutUpdate;
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
    if((state.nodes[id].used.flags & NodeFlag::Clip) != (flags & NodeFlag::Clip))
        state.state |= UserInterfaceState::NeedsNodeClipUpdate;
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
    /* The "pointer" chasing in here is a bit nasty, but there's no other way
       to know which node order items are used and which not, and adding such
       field would inflate the data size for little advantage -- this function
       isn't meant to be used that often, and no other code needs this info. */
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
       clean, bail. */
    const UserInterfaceStates states = this->state();
    if(!(states >= UserInterfaceState::NeedsNodeClean))
        return *this;

    State& state = *_state;

    /* Single allocation for all temporary data */
    Containers::ArrayView<UnsignedInt> childrenOffsets;
    Containers::ArrayView<UnsignedInt> children;
    Containers::ArrayView<Int> nodeIds;
    Containers::ArrayTuple storage{
        /* Running children offset (+1) for each node including root (+1) */
        {ValueInit, state.nodes.size() + 2, childrenOffsets},
        {NoInit, state.nodes.size(), children},
        /* One more item for the -1 at the front */
        {NoInit, state.nodes.size() + 1, nodeIds},
    };

    /* If no node clean is needed, there's no need to build and iterate an
       ordered list of nodes */
    if(states >= UserInterfaceState::NeedsNodeClean) {
        /* 1. Order the whole node hierarchy */
        /** @todo here it may happen that a handle gets recycled and then
            parented to some of its original children, leading to an
            unreachable cycle and a internal assertion inside, see the
            (skipped) cleanRemoveNestedNodesRecycledHandleOrphanedCycle()
            test */
        Implementation::orderNodesBreadthFirstInto(
            stridedArrayView(state.nodes).slice(&Node::used).slice(&Node::Used::parentOrOrder),
            childrenOffsets, children, nodeIds);

        /* 2. Go through the ordered nodes (skipping the first element which is
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

        /* 3. Next perform a clean for layouter node assignments and data node
           attachments, keeping only layouts assigned to (remaining) valid node
           handles and data that are either not attached or attached to valid
           node handles. */
        const Containers::StridedArrayView1D<const UnsignedShort> nodeGenerations = stridedArrayView(state.nodes).slice(&Node::used).slice(&Node::Used::generation);

        /* In each layer remove data attached to invalid non-null nodes */
        for(Layer& layer: state.layers) if(AbstractLayer* const instance = layer.used.instance.get())
            instance->cleanNodes(nodeGenerations);

        /* In each layouter remove layouts assigned to invalid nodes */
        for(Layouter& layouter: state.layouters) if(AbstractLayouter* const instance = layouter.used.instance.get())
            instance->cleanNodes(nodeGenerations);
    }

    /* Unmark the UI as needing a clean() call, but keep the Update states
       including ones that bubbled up from layers */
    state.state = states & ~(UserInterfaceState::NeedsNodeClean & ~UserInterfaceState::NeedsNodeUpdate);
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

    /* Asserting only if there's actually something to update to avoid having
       to go through this assertion every frame. Which means it will not fire
       for a completely empty UI, but that's fine since that doesn't render
       anything anyway. */
    CORRADE_ASSERT(!state.size.isZero(),
        "Whee::AbstractUserInterface::update(): user interface size wasn't set", *this);

    /* If layout attachment update is desired, calculate the total conservative
       count of layouts in all layouters to size the output arrays.
       Conservative as it includes also freed layouts, however the assumption
       is that in majority cases there will be very little freed layouts. */
    std::size_t usedLayouterCount = 0;
    std::size_t layoutCount = 0;
    if(states >= UserInterfaceState::NeedsLayoutAssignmentUpdate) {
        for(const Layouter& layouter: state.layouters) {
            if(const AbstractLayouter* const instance = layouter.used.instance.get()) {
                ++usedLayouterCount;
                layoutCount += instance->capacity();
            }
        }
    }

    /* If node data attachment update is desired, calculate the total
       (again conservative) count of data in all layers to size the output
       arrays. Conservative as it includes also freed and non-attached data,
       however again the assumption is that in majority of cases there will be
       very little freed data and all of them attached to some node. */
    std::size_t dataCount = 0;
    if(states >= UserInterfaceState::NeedsDataAttachmentUpdate)
        for(Layer& layer: state.layers)
            if(AbstractLayer* const instance = layer.used.instance.get())
                dataCount += instance->capacity();

    /* Single allocation for all temporary data */
    /** @todo well, not really, there's one more temp array for layout mask
        calculation */
    Containers::ArrayView<UnsignedInt> childrenOffsets;
    Containers::ArrayView<UnsignedInt> children;
    Containers::ArrayView<Containers::Triple<UnsignedInt, UnsignedInt, UnsignedInt>> parentsToProcess;
    Containers::StridedArrayView2D<LayoutHandle> nodeLayouts;
    Containers::StridedArrayView2D<UnsignedInt> nodeLayoutLevels;
    Containers::ArrayView<UnsignedInt> layoutLevelOffsets;
    Containers::ArrayView<LayoutHandle> topLevelLayouts;
    Containers::ArrayView<UnsignedInt> topLevelLayoutLevels;
    Containers::ArrayView<LayoutHandle> levelPartitionedTopLevelLayouts;
    Containers::ArrayView<UnsignedInt> layouterCapacities;
    Containers::ArrayView<Containers::Triple<Vector2, Vector2, UnsignedInt>> clipStack;
    Containers::ArrayView<UnsignedInt> visibleNodeDataOffsets;
    Containers::ArrayView<UnsignedInt> visibleNodeDataIds;
    Containers::ArrayTuple storage{
        /* Running children offset (+1) for each node */
        {ValueInit, state.nodes.size() + 1, childrenOffsets},
        {NoInit, state.nodes.size(), children},
        {NoInit, state.nodes.size(), parentsToProcess},
        /* Not all nodes have layouts from all layouters, initialize to
           LayoutHandle::Null */
        {ValueInit, {state.nodes.size(), usedLayouterCount}, nodeLayouts},
        /* Zero-initialized as zeros indicate the layout (if non-null) is
           assigned to a node that's not visible */
        {ValueInit, {state.nodes.size(), usedLayouterCount}, nodeLayoutLevels},
        /* Running layout offset (+1) for each level */
        {ValueInit, layoutCount + 1, layoutLevelOffsets},
        {NoInit, layoutCount, topLevelLayouts},
        {NoInit, layoutCount, topLevelLayoutLevels},
        {NoInit, layoutCount, levelPartitionedTopLevelLayouts},
        {NoInit, state.layouters.size(), layouterCapacities},
        /* Running data offset (+1) for each item. This array gets overwritten
           from scratch for each layer so zero-initializing is done inside
           orderVisibleNodeDataInto() instead. */
        {NoInit, state.nodes.size() + 1, visibleNodeDataOffsets},
        {NoInit, state.nodes.size(), clipStack},
        {NoInit, dataCount, visibleNodeDataIds},
    };

    /* If no node update is needed, the data in `state.nodeStateStorage` and
       all views pointing to it is already up-to-date. */
    if(states >= UserInterfaceState::NeedsNodeUpdate) {
        /* Make a resident allocation for all node-related state */
        state.nodeStateStorage = Containers::ArrayTuple{
            {NoInit, state.nodes.size(), state.visibleNodeIds},
            {NoInit, state.nodes.size(), state.visibleNodeChildrenCounts},
            {NoInit, state.nodeOrder.size(), state.visibleFrontToBackTopLevelNodeIndices},
            {NoInit, state.nodes.size(), state.nodeOffsets},
            {NoInit, state.nodes.size(), state.nodeSizes},
            {NoInit, state.nodes.size(), state.absoluteNodeOffsets},
            {NoInit, state.nodes.size(), state.visibleNodeMask},
            {NoInit, state.nodes.size(), state.clipRectOffsets},
            {NoInit, state.nodes.size(), state.clipRectSizes},
            {NoInit, state.nodes.size(), state.clipRectNodeCounts},
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

    /* If no layout assignment update is needed, the
       `state.layouterStateStorage` and all views pointing to it are
       up-to-date */
    if(states >= UserInterfaceState::NeedsLayoutAssignmentUpdate) {
        /* 3. Gather all layouts assigned to a particular node, ordered by the
           layout order. */
        if(state.firstLayouter != LayouterHandle::Null) {
            LayouterHandle layouter = state.firstLayouter;
            UnsignedInt layouterIndex = 0;
            do {
                const UnsignedInt layouterId = layouterHandleId(layouter);
                const Layouter& layouterItem = state.layouters[layouterId];
                if(const AbstractLayouter* const instance = layouterItem.used.instance.get()) {
                    const Containers::StridedArrayView1D<const NodeHandle> nodes = instance->nodes();
                    for(std::size_t i = 0; i != nodes.size(); ++i) {
                        const NodeHandle node = nodes[i];
                        if(node == NodeHandle::Null)
                            continue;
                        /* The LayoutHandle generation isn't used for anything,
                           so can be arbitrary. This here also overwrites
                           multiple layouts set for the same node. */
                        nodeLayouts[{nodeHandleId(node), layouterIndex}] = layoutHandle(layouter, i, 0);
                    }
                }
                layouter = layouterItem.used.next;
                ++layouterIndex;
            } while(layouter != state.firstLayouter);
        }

        /* Make a resident allocation for all layout-related state */
        state.layoutStateStorage = Containers::ArrayTuple{
            {NoInit, layoutCount + 1, state.topLevelLayoutOffsets},
            {NoInit, layoutCount, state.topLevelLayoutLayouterIds},
            {NoInit, layoutCount, state.topLevelLayoutIds},
        };

        /* 4. Discover top-level layouts to be subsequently fed to layouter
           update() calls. */
        const Containers::Pair<UnsignedInt, std::size_t> maxLevelTopLevelLayoutOffsetCount = Implementation::discoverTopLevelLayoutNodesInto(
            stridedArrayView(state.nodes).slice(&Node::used).slice(&Node::Used::parentOrOrder),
            state.visibleNodeIds,
            state.layouters.size(),
            nodeLayouts,
            nodeLayoutLevels,
            layoutLevelOffsets,
            topLevelLayouts,
            topLevelLayoutLevels,
            levelPartitionedTopLevelLayouts,
            state.topLevelLayoutOffsets,
            state.topLevelLayoutLayouterIds,
            state.topLevelLayoutIds);
        state.topLevelLayoutOffsets = state.topLevelLayoutOffsets.prefix(maxLevelTopLevelLayoutOffsetCount.second());
        state.topLevelLayoutLayouterIds = state.topLevelLayoutLayouterIds.prefix(maxLevelTopLevelLayoutOffsetCount.second() - 1);

        /* Fill in layouter capacities */
        /** @todo a way to have them all accessible via some strided slice?
            by having the AbstractLayouter::State directly inside Layouter it
            could also save the extra PIMPL allocation for every instance */
        for(std::size_t i = 0; i != state.layouters.size(); ++i)
            if(const AbstractLayouter* const instance = state.layouters[i].used.instance.get())
                layouterCapacities[i] = instance->capacity();

        /* Calculate the total bit count for all layout masks and allocate
           them, together with a temporary mapping array */
        /** @todo while everything else is tucked into an ArrayTuple, often
            significantly overallocating, this is not -- have some bump
            allocators finally */
        std::size_t maskSize = 0;
        for(std::size_t i = 0; i != maxLevelTopLevelLayoutOffsetCount.second() - 1; ++i)
            maskSize += state.layouters[state.topLevelLayoutLayouterIds[i]].used.instance->capacity();
        state.layoutMasks = Containers::BitArray{ValueInit, maskSize};
        Containers::Array<std::size_t> layouterLevelMaskOffsets{NoInit, state.layouters.size()*maxLevelTopLevelLayoutOffsetCount.first()};

        /* 5. Fill the per-layout-update masks. */
        Implementation::fillLayoutUpdateMasksInto(
            nodeLayouts,
            nodeLayoutLevels,
            layoutLevelOffsets,
            state.topLevelLayoutOffsets,
            state.topLevelLayoutLayouterIds,
            layouterCapacities,
            stridedArrayView(layouterLevelMaskOffsets).expanded<0, 2>({maxLevelTopLevelLayoutOffsetCount.first(), state.layouters.size()}),
            state.layoutMasks);
    }

    /* If no layout update is needed, the `state.nodeOffsets`,
       `state.nodeSizes` and `state.absoluteNodeOffsets` are all
       up-to-date */
    if(states >= UserInterfaceState::NeedsLayoutUpdate) {
        /* 6. Copy the explicitly set offset + sizes to the output. */
        Utility::copy(stridedArrayView(state.nodes).slice(&Node::used).slice(&Node::Used::offset), state.nodeOffsets);
        Utility::copy(stridedArrayView(state.nodes).slice(&Node::used).slice(&Node::Used::size), state.nodeSizes);

        /* 7. Perform layout calculation for all top-level layouts. */
        std::size_t offset = 0;
        for(std::size_t i = 0; i != state.topLevelLayoutOffsets.size() - 1; ++i) {
            AbstractLayouter* const instance = state.layouters[state.topLevelLayoutLayouterIds[i]].used.instance.get();
            CORRADE_INTERNAL_ASSERT(instance);

            instance->update(
                state.layoutMasks.sliceSize(offset, instance->capacity()),
                state.topLevelLayoutIds.slice(
                    state.topLevelLayoutOffsets[i],
                    state.topLevelLayoutOffsets[i + 1]),
                state.nodeOffsets,
                state.nodeSizes);

            offset += instance->capacity();
        }
        CORRADE_INTERNAL_ASSERT(offset == state.layoutMasks.size());

        /* Call a no-op update() on layouters that have Needs*Update flags but
           have no visible layouts so update() wasn't called for them above */
        /** @todo this is nasty, think of a better solution */
        for(Layouter& layouter: state.layouters) {
            AbstractLayouter* const instance = layouter.used.instance.get();
            if(instance && instance->state() & LayouterState::NeedsAssignmentUpdate) {
                /** @todo yeah and this allocation REALLY isn't great */
                instance->update(Containers::BitArray{ValueInit, instance->capacity()}, {}, state.nodeOffsets, state.nodeSizes);
            }
        }

        /* 8. Calculate absolute offsets for visible nodes. */
        for(const UnsignedInt id: state.visibleNodeIds) {
            const Node& node = state.nodes[id];
            const Vector2 nodeOffset = state.nodeOffsets[id];
            state.absoluteNodeOffsets[id] =
                nodeHandleGeneration(node.used.parentOrOrder) == 0 ? nodeOffset :
                    state.absoluteNodeOffsets[nodeHandleId(node.used.parentOrOrder)] + nodeOffset;
        }
    }

    /* If no clip update is needed, the `state.visibleNodeMask` is all
       up-to-date */
    if(states >= UserInterfaceState::NeedsNodeClipUpdate) {
        /* 9. Cull / clip the visible nodes based on their clip rects */
        state.clipRectCount = Implementation::cullVisibleNodesInto(
            state.absoluteNodeOffsets,
            state.nodeSizes,
            stridedArrayView(state.nodes).slice(&Node::used).slice(&Node::Used::flags),
            clipStack.prefix(state.visibleNodeIds.size()),
            state.visibleNodeIds,
            state.visibleNodeChildrenCounts,
            state.visibleNodeMask,
            state.clipRectOffsets,
            state.clipRectSizes,
            state.clipRectNodeCounts);

        /** @todo might want also a layer-specific cull / clip implementation
            that gets called after the "upload" step, for line art, text runs
            and such */
    }

    /* If no data attachment update is needed, the data in
       `state.dataStateStorage` and all views pointing to it is already
       up-to-date. */
    if(states >= UserInterfaceState::NeedsDataAttachmentUpdate) {
        /* Calculate count of visible top-level nodes and layers that draw in
           order to accurately size the array with draws */
        UnsignedInt visibleTopLevelNodeCount = 0;
        for(UnsignedInt visibleTopLevelNodeIndex = 0; visibleTopLevelNodeIndex != state.visibleNodeChildrenCounts.size(); visibleTopLevelNodeIndex += state.visibleNodeChildrenCounts[visibleTopLevelNodeIndex] + 1) {
            if(!(state.nodes[state.visibleNodeIds[visibleTopLevelNodeIndex]].used.flags & NodeFlag::Hidden))
                ++visibleTopLevelNodeCount;
        }
        UnsignedInt drawLayerCount = 0;
        for(const Layer& layer: state.layers) {
            if(layer.used.features & LayerFeature::Draw)
                ++drawLayerCount;
        }

        /* Make a resident allocation for all data-related state */
        state.dataStateStorage = Containers::ArrayTuple{
            /* Running data offset (+1) for each item. Populated sequentially
               so it doesn't need to be zero-initialized. */
            {NoInit, state.layers.size() + 1, state.dataToUpdateLayerOffsets},
            {NoInit, dataCount, state.dataToUpdateIds},
            /* The orderVisibleNodeDataInto() algorithm assumes there can be
               a dedicated clip rect for every visible node. It's being run for
               all layers, so in order to fit it has to have layer count times
               visible node count elements. */
            {NoInit, state.visibleNodeIds.size()*state.layers.size(), state.dataToUpdateClipRectIds},
            {NoInit, state.visibleNodeIds.size()*state.layers.size(), state.dataToUpdateClipRectDataCounts},
            {NoInit, visibleTopLevelNodeCount*drawLayerCount, state.dataToDrawLayerIds},
            {NoInit, visibleTopLevelNodeCount*drawLayerCount, state.dataToDrawOffsets},
            {NoInit, visibleTopLevelNodeCount*drawLayerCount, state.dataToDrawSizes},
            {NoInit, visibleTopLevelNodeCount*drawLayerCount, state.dataToDrawClipRectOffsets},
            {NoInit, visibleTopLevelNodeCount*drawLayerCount, state.dataToDrawClipRectSizes},
            /* Running data offset (+1) for each item */
            {ValueInit, state.nodes.size() + 1, state.visibleNodeEventDataOffsets},
            {NoInit, dataCount, state.visibleNodeEventData},
        };

        state.dataToUpdateLayerOffsets[0] = {0, 0};
        if(state.firstLayer != LayerHandle::Null) {
            /* 10. Go through the layer draw order and order data of each layer
               that are assigned to visible nodes into a contiguous range,
               populating also the draw list and count of event data per
               visible node as a side effect. */

            /* Build a layer order map for proper draw order. The layer order
               is cyclic, so stop when reaching the first layer again. */
            LayerHandle layer = state.firstLayer;
            UnsignedInt drawLayerOrder[1 << Implementation::LayerHandleIdBits];
            UnsignedInt layerOrderIndex = 0;
            do {
                const UnsignedInt layerId = layerHandleId(layer);
                const Layer& layerItem = state.layers[layerId];
                if(layerItem.used.features & LayerFeature::Draw)
                    drawLayerOrder[layerId] = layerOrderIndex++;
                layer = layerItem.used.next;
            } while(layer != state.firstLayer);

            /* Next iterate through all layers directly, skipping ones that
               don't have an instance, and populate the to-update, to-draw and
               event data count arrays. The data order matches the visible node
               hierarchy order from above. */
            UnsignedInt offset = 0;
            UnsignedInt clipRectOffset = 0;
            for(UnsignedInt i = 0; i != state.layers.size(); ++i) {
                const Layer& layerItem = state.layers[i];

                if(const AbstractLayer* const instance = layerItem.used.instance.get()) {
                    /* The `state.dataToDrawOffsets` etc views that are sliced
                       below are filled only for the layers that support
                       LayerFeature::Draw, and are sized to separately draw all
                       top level nodes. Thus, if there are no top level nodes,
                       nothing is drawn, the views are empty and so we
                       shouldn't attempt to slice them. */
                    const bool isDrawingAnything = visibleTopLevelNodeCount && layerItem.used.features >= LayerFeature::Draw;

                    const Containers::Pair<UnsignedInt, UnsignedInt> out = Implementation::orderVisibleNodeDataInto(
                        state.visibleNodeIds,
                        state.visibleNodeChildrenCounts,
                        instance->nodes(),
                        layerItem.used.features,
                        state.visibleNodeMask,
                        state.clipRectNodeCounts.prefix(state.clipRectCount),
                        visibleNodeDataOffsets,
                        state.visibleNodeEventDataOffsets.exceptPrefix(1),
                        visibleNodeDataIds.prefix(instance->capacity()),
                        state.dataToUpdateIds,
                        state.dataToUpdateClipRectIds,
                        state.dataToUpdateClipRectDataCounts,
                        offset,
                        clipRectOffset,
                        /* If the layer has LayerFeature::Draw and there are
                           actually some top-level nodes to be drawn, it also
                           populates the draw call list for all top-level
                           nodes. This has to be interleaved with other layers
                           (thus the every() "sparsening") in order to be first
                           by the top-level node and then by layer. If the
                           layer doesn't draw anything, these aren't used. */
                        isDrawingAnything ? stridedArrayView(state.dataToDrawOffsets)
                            .exceptPrefix(drawLayerOrder[i])
                            .every(drawLayerCount) : nullptr,
                        isDrawingAnything ? stridedArrayView(state.dataToDrawSizes)
                            .exceptPrefix(drawLayerOrder[i])
                            .every(drawLayerCount) : nullptr,
                        isDrawingAnything ? stridedArrayView(state.dataToDrawClipRectOffsets)
                            .exceptPrefix(drawLayerOrder[i])
                            .every(drawLayerCount) : nullptr,
                        isDrawingAnything ? stridedArrayView(state.dataToDrawClipRectSizes)
                            .exceptPrefix(drawLayerOrder[i])
                            .every(drawLayerCount) : nullptr);
                    offset = out.first();
                    clipRectOffset = out.second();

                    /* If the layer has LayerFeature::Draw, increment to the
                       next interleaved position for the next. Also save the
                       matching layer index to have the draw information
                       complete. */
                    if(isDrawingAnything) {
                        for(UnsignedByte& j: stridedArrayView(state.dataToDrawLayerIds)
                            .exceptPrefix(drawLayerOrder[i])
                            .every(drawLayerCount)
                        )
                            j = i;
                    }
                }

                state.dataToUpdateLayerOffsets[i + 1] = {offset, clipRectOffset};
            }

            /* 11. Take the count of event data per visible node, turn that
               into an offset array and populate it. */

            /* `[state.visibleNodeEventDataOffsets[i + 1], state.visibleNodeEventDataOffsets[i + 2])`
               is now a range in which the `state.visibleNodeEventData` array
               will contain a list of event data handles for visible node with
               ID `i`. The last element (containing the end offset) is omitted
               at this step. */
            {
                UnsignedInt visibleNodeEventDataCount = 0;
                for(UnsignedInt& i: state.visibleNodeEventDataOffsets) {
                    const UnsignedInt nextOffset = visibleNodeEventDataCount + i;
                    i = visibleNodeEventDataCount;
                    visibleNodeEventDataCount = nextOffset;
                }
            }

            /* 12. Go through all event handling layers and populate the
               `state.visibleNodeEventData` array based on the
               `state.visibleNodeEventDataOffsets` populated above. Compared
               to drawing, event handling has the layers in a front-to-back
               order, multiple data from the same layer attached to the same
               node are also added in reverse way. */
            const LayerHandle lastLayer = state.layers[layerHandleId(state.firstLayer)].used.previous;
            layer = lastLayer;
            do {
                const UnsignedInt layerId = layerHandleId(layer);
                const Layer& layerItem = state.layers[layerId];

                if(layerItem.used.features & LayerFeature::Event) {
                    Implementation::orderNodeDataForEventHandlingInto(
                        layer,
                        /* If the Layer::features is non-empty, it means the
                           instance is present (from which it was taken). No
                           need to explicitly check that as well. */
                        layerItem.used.instance->nodes(),
                        state.visibleNodeEventDataOffsets,
                        state.visibleNodeMask,
                        state.visibleNodeEventData);
                }

                layer = layerItem.used.previous;
            } while(layer != lastLayer);
        }

        /* 13. Compact the draw calls by throwing away the empty ones. This
           cannot be done in the above loop directly as it'd need to go first
           by top-level node and then by layer in each. That it used to do in a
           certain way before which was much slower. */
        state.drawCount = Implementation::compactDrawsInPlace(
            state.dataToDrawLayerIds,
            state.dataToDrawOffsets,
            state.dataToDrawSizes,
            state.dataToDrawClipRectOffsets,
            state.dataToDrawClipRectSizes);
    }

    /* 14. For each layer submit an update of visible data across all visible
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
                state.dataToUpdateLayerOffsets[i].first(),
                state.dataToUpdateLayerOffsets[i + 1].first()),
            state.dataToUpdateClipRectIds.slice(
                state.dataToUpdateLayerOffsets[i].second(),
                state.dataToUpdateLayerOffsets[i + 1].second()),
            state.dataToUpdateClipRectDataCounts.slice(
                state.dataToUpdateLayerOffsets[i].second(),
                state.dataToUpdateLayerOffsets[i + 1].second()),
            /** @todo some layer implementations may eventually want relative
                offsets, not absolute, provide both? */
            state.absoluteNodeOffsets,
            state.nodeSizes,
            state.clipRectOffsets.prefix(state.clipRectCount),
            state.clipRectSizes.prefix(state.clipRectCount));
    }

    /** @todo layer-specific cull/clip step? */

    /* 15. Refresh the event handling state based on visible nodes. */
    if(states >= UserInterfaceState::NeedsNodeUpdate) {
        /* If the pressed node is no longer valid or is now invisible, reset
           it */
        if(!isHandleValid(state.pointerEventPressedNode) ||
           !state.visibleNodeMask[nodeHandleId(state.pointerEventHoveredNode)]) {
            state.pointerEventPressedNode = NodeHandle::Null;
        }

        /* If the node capturing pointer events is no longer valid or is now
           invisible, reset it */
        if(!isHandleValid(state.pointerEventCapturedNode) ||
           !state.visibleNodeMask[nodeHandleId(state.pointerEventCapturedNode)]) {
            state.pointerEventCapturedNode = NodeHandle::Null;
        }

        /* If the hovered node is no longer valid or is now invisible, reset
           it */
        if(!isHandleValid(state.pointerEventHoveredNode) ||
           !state.visibleNodeMask[nodeHandleId(state.pointerEventHoveredNode)]) {
            state.pointerEventHoveredNode = NodeHandle::Null;
        }
    }

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
                state.dataToUpdateLayerOffsets[layerId].first(),
                state.dataToUpdateLayerOffsets[layerId + 1].first()),
            /* ... and the draw offset then being relative to those */
            state.dataToDrawOffsets[i] - state.dataToUpdateLayerOffsets[layerId].first(),
            state.dataToDrawSizes[i],
            /* Same for clip rects ... */
            state.dataToUpdateClipRectIds.slice(
                state.dataToUpdateLayerOffsets[layerId].second(),
                state.dataToUpdateLayerOffsets[layerId + 1].second()),
            state.dataToUpdateClipRectDataCounts.slice(
                state.dataToUpdateLayerOffsets[layerId].second(),
                state.dataToUpdateLayerOffsets[layerId + 1].second()),
            /* ... and the clip rect offset then being relative to those */
            state.dataToDrawClipRectOffsets[i] - state.dataToUpdateLayerOffsets[layerId].second(),
            state.dataToDrawClipRectSizes[i],
            state.absoluteNodeOffsets,
            state.nodeSizes,
            state.clipRectOffsets.prefix(state.clipRectCount),
            state.clipRectSizes.prefix(state.clipRectCount));
    }

    return *this;
}

template<class Event, void(AbstractLayer::*function)(UnsignedInt, Event&)> bool AbstractUserInterface::callEventOnNode(const Vector2& globalPositionScaled, const UnsignedInt nodeId, Event& event, const bool rememberCaptureOnUnaccepted) {
    State& state = *_state;

    /* Set isHovering() to false if the event is called on node that actually
       isn't hovered. The caller itself may also set it to false if it is
       called on a hovered node but the event is outside of its area (such as a
       move outside of the captured node), so we can't set it
       unconditionally. */
    const bool hovering = event._hovering;
    if(state.pointerEventHoveredNode == NodeHandle::Null || nodeId != nodeHandleId(state.pointerEventHoveredNode))
        event._hovering = false;

    /* Remember the initial event capture state to reset it after each
       non-accepted event handler call */
    const bool captured = event._captured;
    bool acceptedByAnyData = false;
    for(UnsignedInt j = state.visibleNodeEventDataOffsets[nodeId], jMax = state.visibleNodeEventDataOffsets[nodeId + 1]; j != jMax; ++j) {
        const DataHandle data = state.visibleNodeEventData[j];
        event._position = globalPositionScaled - state.absoluteNodeOffsets[nodeId];
        event._accepted = false;
        ((*state.layers[dataHandleLayerId(data)].used.instance).*function)(dataHandleId(data), event);
        if(event._accepted)
            acceptedByAnyData = true;

        /* If not accepted (unless we want to remember capture also on events
           for which the accept status is ignored, like Enter or Leave) reset
           the capture state back to the initial for the next call as we're
           only interested in the capture state from the handler that accepts
           the event.

           This has to happen after every iteration and not only at the end,
           because otherwise subsequent events may get bogus isCaptured() bits
           from earlier unaccepted events and get confused. */
        if(!event._accepted && !rememberCaptureOnUnaccepted)
            event._captured = captured;
    }

    /* Reset isHovering() back to the initial state in case the event will be
       re-called on different nodes (where it could actually be true) again
       after */
    event._hovering = hovering;

    return acceptedByAnyData;
}

template<class Event, void(AbstractLayer::*function)(UnsignedInt, Event&)> NodeHandle AbstractUserInterface::callEvent(const Vector2& globalPositionScaled, const UnsignedInt visibleNodeIndex, Event& event) {
    /* The accept state should be initially false as we exit once it becomes
       true. */
    CORRADE_INTERNAL_ASSERT(!event._accepted);
    State& state = *_state;
    const UnsignedInt nodeId = state.visibleNodeIds[visibleNodeIndex];

    /* If the position is outside the node, we got nothing */
    const Vector2 nodeOffset = state.absoluteNodeOffsets[nodeId];
    if((globalPositionScaled < nodeOffset).any() ||
       (globalPositionScaled >= nodeOffset + state.nodeSizes[nodeId]).any())
        return {};

    /* If the position is inside, recurse into *direct* children. If the event
       is handled there, we're done. */
    /** @todo maintain some info about how many actual event handlers is in
        particular subtrees, to not have to do complex hit testing when there's
        nothing to call anyway? especially for move events and such */
    for(UnsignedInt i = 1, iMax = state.visibleNodeChildrenCounts[visibleNodeIndex] + 1; i != iMax; i += state.visibleNodeChildrenCounts[visibleNodeIndex + i] + 1) {
        const NodeHandle called = callEvent<Event, function>(globalPositionScaled, visibleNodeIndex + i, event);
        if(called != NodeHandle::Null)
            return called;
    }

    /* Only if children didn't handle the event, look into this node data */
    if(callEventOnNode<Event, function>(globalPositionScaled, nodeId, event))
        return nodeHandle(nodeId, state.nodes[nodeId].used.generation);

    return {};
}

template<class Event, void(AbstractLayer::*function)(UnsignedInt, Event&)> NodeHandle AbstractUserInterface::callEvent(const Vector2& globalPositionScaled, Event& event) {
    /* Call update implicitly in order to make the internal state ready for
       event processing. Is a no-op if there's nothing to update or clean. */
    update();

    for(const UnsignedInt visibleTopLevelNodeIndex: _state->visibleFrontToBackTopLevelNodeIndices) {
        const NodeHandle called = callEvent<Event, function>(globalPositionScaled, visibleTopLevelNodeIndex, event);
        if(called != NodeHandle::Null)
            return called;
    }

    return {};
}

bool AbstractUserInterface::pointerPressEvent(const Vector2& globalPosition, PointerEvent& event) {
    CORRADE_ASSERT(!event._accepted,
        "Whee::AbstractUserInterface::pointerPressEvent(): event already accepted", {});

    State& state = *_state;

    /* Press event has isCaptured() set always. Can have isHovering() set if
       it ends up being called on the currently hovered node. */
    event._captured = true;
    event._hovering = true;

    /* This will be invalid if setSize() wasn't called yet, but callEvent() has
       a call to update() inside which will then assert */
    const Vector2 globalPositionScaled = globalPosition*state.size/state.windowSize;

    const NodeHandle called = callEvent<PointerEvent, &AbstractLayer::pointerPressEvent>(globalPositionScaled, event);

    /* If the event was accepted by any node, remember that node for potential
       future tap or click event */
    if(called != NodeHandle::Null) {
        state.pointerEventPressedNode = called;

        /* If capture is desired, remember the concrete node for it as well */
        if(event._captured)
            state.pointerEventCapturedNode = called;
    }

    /* Update the last relative position with this one */
    state.pointerEventPreviousGlobalPositionScaled = globalPositionScaled;

    return called != NodeHandle::Null;
}

bool AbstractUserInterface::pointerReleaseEvent(const Vector2& globalPosition, PointerEvent& event) {
    CORRADE_ASSERT(!event._accepted,
        "Whee::AbstractUserInterface::pointerReleaseEvent(): event already accepted", {});

    /* Update so we don't have stale pointerEventCapture{Node,Data}. Otherwise
       the update() gets called only later in callEvent(). */
    update();

    State& state = *_state;
    const Vector2 globalPositionScaled = globalPosition*state.size/state.windowSize;

    /* If there's a node capturing pointer events, call the event on it
       directly. Given that update() was called, it should be either null or
       valid. */
    bool releaseAcceptedByAnyData;
    bool callTapOrClick;
    if(state.pointerEventCapturedNode != NodeHandle::Null) {
        CORRADE_INTERNAL_ASSERT(isHandleValid(state.pointerEventCapturedNode));

        const UnsignedInt capturedNodeId = nodeHandleId(state.pointerEventCapturedNode);
        const Vector2 capturedNodeMin = state.absoluteNodeOffsets[capturedNodeId];
        const Vector2 capturedNodeMax = capturedNodeMin + state.nodeSizes[capturedNodeId];
        const bool insideCapturedNode = (globalPositionScaled >= capturedNodeMin).all() && (globalPositionScaled < capturedNodeMax).all();

        /* Called on a captured node, so isCaptured() should be true,
           isHovering() can be true if it's inside it. As the release event
           always implicitly releases the capture, any potential capture state
           changed by the event handler is ignored. */
        event._captured = true;
        event._hovering = insideCapturedNode;

        releaseAcceptedByAnyData = callEventOnNode<PointerEvent, &AbstractLayer::pointerReleaseEvent>(globalPositionScaled, nodeHandleId(state.pointerEventCapturedNode), event);

        /* Call tap or click event if the release happened inside the captured
           node area and was accepted (i.e., it wasn't outside of the *actual*
           active area), and the node release was called on is the same as node
           the press was called on (because, e.g., something could disable and re-enable capture in the middle of a move, changing the captured
           node to something else, or the captured node ) */
        callTapOrClick = insideCapturedNode && releaseAcceptedByAnyData && state.pointerEventPressedNode == state.pointerEventCapturedNode;

    /* Otherwise the usual hit testing etc. */
    } else {
        /* Not called on a captured node, isCaptured() should be false and thus
           isHovering() can be true */
        event._captured = false;
        event._hovering = true;

        const NodeHandle calledNode = callEvent<PointerEvent, &AbstractLayer::pointerReleaseEvent>(globalPositionScaled, event);
        releaseAcceptedByAnyData = calledNode != NodeHandle::Null;

        /* Call tap or click event if the release was accepted (i.e., it wasn't
           outside of the *actual* active area), the pointer didn't leave the
           area of the originally pressed node and the node the release was
           called on the same node as the original press (i.e.., it didn't
           happen on some child node inside of the originally pressed node) */
        callTapOrClick = releaseAcceptedByAnyData && state.pointerEventPressedNode != NodeHandle::Null && state.pointerEventPressedNode == calledNode;
    }

    /* Emit a TapOrClick event if needed. Reusing the same event instance, just
       resetting the accept status. Both the accept and the capture status is
       subsequently ignored. */
    if(callTapOrClick) {
        CORRADE_INTERNAL_ASSERT(isHandleValid(state.pointerEventPressedNode));

        event._accepted = false;
        callEventOnNode<PointerEvent, &AbstractLayer::pointerTapOrClickEvent>(globalPositionScaled, nodeHandleId(state.pointerEventPressedNode), event);
    }

    /* After a release, there should be no pressed node anymore, and neither a
       captured node */
    state.pointerEventPressedNode = NodeHandle::Null;
    state.pointerEventCapturedNode = NodeHandle::Null;

    /* Update the last relative position with this one */
    state.pointerEventPreviousGlobalPositionScaled = globalPositionScaled;

    return releaseAcceptedByAnyData;
}

bool AbstractUserInterface::pointerMoveEvent(const Vector2& globalPosition, PointerMoveEvent& event) {
    CORRADE_ASSERT(!event._accepted,
        "Whee::AbstractUserInterface::pointerMoveEvent(): event already accepted", {});

    /* Update so we don't have stale pointerEventCapture{Node,Data}. Otherwise
       the update() gets called only later in callEvent(). */
    update();

    State& state = *_state;
    const Vector2 globalPositionScaled = globalPosition*state.size/state.windowSize;

    /* Fill in position relative to the previous event, if there was any. Since
       the value is event-relative and not node-relative, it doesn't need any
       further updates in the callEvent() code. */
    event._relativePosition = state.pointerEventPreviousGlobalPositionScaled ?
        globalPositionScaled - *state.pointerEventPreviousGlobalPositionScaled : Vector2{};

    /* If there's a node capturing pointer events, call the event on it
       directly. Given that update() was called, it should be either null or
       valid. */
    bool moveAcceptedByAnyData;
    NodeHandle calledNode;
    bool insideNodeArea;
    if(state.pointerEventCapturedNode != NodeHandle::Null) {
        CORRADE_INTERNAL_ASSERT(isHandleValid(state.pointerEventCapturedNode));

        const UnsignedInt capturedNodeId = nodeHandleId(state.pointerEventCapturedNode);
        const Vector2 capturedNodeMin = state.absoluteNodeOffsets[capturedNodeId];
        const Vector2 capturedNodeMax = capturedNodeMin + state.nodeSizes[capturedNodeId];
        insideNodeArea = (globalPositionScaled >= capturedNodeMin).all() && (globalPositionScaled < capturedNodeMax).all();

        /* Called on a captured node, so isCaptured() should be true,
           isHovering() can be true if it's inside it */
        event._captured = true;
        event._hovering = insideNodeArea;

        /* It should be possible to reset the capture in this event
           independently of whether it's accepted or not (for example if
           outside of some acceptable bounds for a capture) */
        moveAcceptedByAnyData = callEventOnNode<PointerMoveEvent, &AbstractLayer::pointerMoveEvent>(globalPositionScaled, nodeHandleId(state.pointerEventCapturedNode), event, /*rememberCaptureOnUnaccepted*/ true);
        calledNode = state.pointerEventCapturedNode;

    /* Otherwise the usual hit testing etc. */
    } else {
        /* Which makes the event always called inside node area */
        insideNodeArea = true;

        /* Not called on a captured node, isCaptured() should be false and thus
           isHovering() can be true */
        event._captured = false;
        event._hovering = true;

        calledNode = callEvent<PointerMoveEvent, &AbstractLayer::pointerMoveEvent>(globalPositionScaled, event);
        moveAcceptedByAnyData = calledNode != NodeHandle::Null;
    }

    /* Decide about currently hovered node and whether to call Enter / Leave.
       If the move event was called on a captured node ... */
    NodeHandle callLeaveOnNode = NodeHandle::Null;
    NodeHandle callEnterOnNode = NodeHandle::Null;
    if(state.pointerEventCapturedNode != NodeHandle::Null) {
        CORRADE_INTERNAL_ASSERT(calledNode == state.pointerEventCapturedNode);

        /* Call Leave if the captured node was previously hovered and the
           pointer is now outside or was not accepted */
        if(state.pointerEventHoveredNode == calledNode && (!insideNodeArea || !moveAcceptedByAnyData)) {
            callLeaveOnNode = calledNode;
        /* Leave also if some other node was previously hovered */
        } else if(state.pointerEventHoveredNode != NodeHandle::Null &&
                  state.pointerEventHoveredNode != calledNode) {
            CORRADE_INTERNAL_ASSERT(isHandleValid(state.pointerEventHoveredNode));
            callLeaveOnNode = state.pointerEventHoveredNode;
        }

        /* Call Enter if the captured node wasn't previously hovered and the
           pointer is now inside and was accepted. Calls Enter also in case
           some other node was previously hovered. */
        if(state.pointerEventHoveredNode != calledNode && (insideNodeArea && moveAcceptedByAnyData)) {
            callEnterOnNode = calledNode;
        }

        /* The now-hovered node is the captured node if the pointer was inside
           and the event was accepted */
        if(insideNodeArea && moveAcceptedByAnyData) {
            state.pointerEventHoveredNode = calledNode;
        } else {
            state.pointerEventHoveredNode = NodeHandle::Null;
        }

    /* Otherwise, call Enter / Leave event if the move event was called on a
       node that's different from the previously hovered */
    } else if(state.pointerEventHoveredNode != calledNode) {
        /* Leave if the previously hovered node isn't null */
        if(state.pointerEventHoveredNode != NodeHandle::Null) {
            CORRADE_INTERNAL_ASSERT(isHandleValid(state.pointerEventHoveredNode));
            callLeaveOnNode = state.pointerEventHoveredNode;
        }
        /* Enter if the current node isn't null */
        if(calledNode != NodeHandle::Null) {
            callEnterOnNode = calledNode;
        }

        /* The now-hovered node is the one that accepted the move event */
        state.pointerEventHoveredNode = calledNode;
    }

    /* Emit a Leave event if needed. Reusing the same event instance, just
       resetting the accept status, relative position (it has to be zero since
       it's relative to the move that happened right before) and capture, as
       Leave events should not be able to affect it. Both the accept and the
       capture status is subsequently ignored. */
    if(callLeaveOnNode != NodeHandle::Null) {
        event._accepted = false;

        /* Leave events are by definition never hovering the node they are
           called on */
        event._hovering = false;

        /* Leave events can only change capture status if they're called on the
           actual captured node, otherwise the capture status is false and is
           also reset back to false below */
        const bool captured = event._captured;
        if(state.pointerEventCapturedNode != callLeaveOnNode)
            event._captured = false;
        event._relativePosition = {};
        /* The accept status is ignored for the Enter/Leave events, which means
           we remember the capture state even if not explicitly accepted */
        callEventOnNode<PointerMoveEvent, &AbstractLayer::pointerLeaveEvent>(globalPositionScaled, nodeHandleId(callLeaveOnNode), event, /*rememberCaptureOnUnaccepted*/ true);

        if(state.pointerEventCapturedNode != callLeaveOnNode)
            event._captured = captured;
    }

    /* Emit Enter event. Again reusing the same event instance, with accept and
       relative position reset. The accept status is subsequently ignored, the
       capture isn't. */
    if(callEnterOnNode != NodeHandle::Null) {
        event._accepted = false;

        /* Enter events are by definition always hovering the node they are
           called on. As the pointerEventHoveredNode was updated above, the callEventOnNode() should thus not reset this back to false. */
        event._hovering = true;

        event._relativePosition = {};
        /* The accept status is ignored for the Enter/Leave events, which means
           we remember the capture state even if not explicitly accepted */
        callEventOnNode<PointerMoveEvent, &AbstractLayer::pointerEnterEvent>(globalPositionScaled, nodeHandleId(callEnterOnNode), event, /*rememberCaptureOnUnaccepted*/ true);
    }

    /* Update the captured node based on what's desired. If the captured state
       was the same before, this is a no op, i.e. assigning the same value. */
    if(event._captured) {
        /* If the captured state was set, the event was either called on a
           captured node (and then either accepted, or not, which caused it to
           stay set), or was accepted on a non-captured node */
        CORRADE_INTERNAL_ASSERT(
            (state.pointerEventCapturedNode != NodeHandle::Null || moveAcceptedByAnyData) &&
            calledNode != NodeHandle::Null);
        state.pointerEventCapturedNode = calledNode;
    } else {
        state.pointerEventCapturedNode = NodeHandle::Null;
    }

    /* If pointer capture is not active (either it wasn't at all, or the move
       reset it), pointerEventPressedNode gets reset if the event happened on a
       different node, happened outside of a (previously captured) node area or
       was not accepted by any data (i.e., it's outside of node active area).
       If pointer capture is active, it's not changed in any way in order to
       make it possible for the pointer to return to the node area and then
       perform a release, resulting in a tap or click. */
    if(state.pointerEventCapturedNode == NodeHandle::Null && (calledNode != state.pointerEventPressedNode || !insideNodeArea || !moveAcceptedByAnyData))
        state.pointerEventPressedNode = NodeHandle::Null;

    /* Update the last relative position with this one */
    state.pointerEventPreviousGlobalPositionScaled = globalPositionScaled;

    return moveAcceptedByAnyData;
}

NodeHandle AbstractUserInterface::pointerEventPressedNode() const {
    return _state->pointerEventPressedNode;
}

NodeHandle AbstractUserInterface::pointerEventCapturedNode() const {
    return _state->pointerEventCapturedNode;
}

NodeHandle AbstractUserInterface::pointerEventHoveredNode() const {
    return _state->pointerEventHoveredNode;
}

}}
