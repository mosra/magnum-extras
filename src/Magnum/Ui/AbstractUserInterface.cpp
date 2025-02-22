/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023, 2024, 2025
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

#include "AbstractUserInterface.h"

#include <Corrade/Containers/ArrayTuple.h>
#include <Corrade/Containers/BitArrayView.h>
#include <Corrade/Containers/BitArray.h>
#include <Corrade/Containers/EnumSet.hpp>
#include <Corrade/Containers/GrowableArray.h>
#include <Corrade/Containers/Iterable.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/Reference.h>
#include <Magnum/Math/Time.h>
#include <Magnum/Math/Vector2.h>

#include "Magnum/Ui/AbstractAnimator.h"
#include "Magnum/Ui/AbstractLayer.h"
#include "Magnum/Ui/AbstractLayouter.h"
#include "Magnum/Ui/AbstractRenderer.h"
#include "Magnum/Ui/Event.h"
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/NodeFlags.h"
#include "Magnum/Ui/Implementation/abstractUserInterface.h"
#include "Magnum/Ui/Implementation/orderNodesBreadthFirstInto.h"

namespace Magnum { namespace Ui {

Debug& operator<<(Debug& debug, const UserInterfaceState value) {
    /* Special case coming from the UserInterfaceStates printer. As both are a
       superset of NeedsDataUpdate, printing just one would result in
       `UserInterfaceState::NeedsNodeOpacityUpdate|UserInterfaceState(0x2)` in
       the output. */
    if(value == UserInterfaceState(UnsignedShort(UserInterfaceState::NeedsDataAttachmentUpdate|UserInterfaceState::NeedsNodeOpacityUpdate)))
        return debug << UserInterfaceState::NeedsDataAttachmentUpdate << Debug::nospace << "|" << Debug::nospace << UserInterfaceState::NeedsNodeOpacityUpdate;

    debug << "Ui::UserInterfaceState" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case UserInterfaceState::value: return debug << "::" #value;
        _c(NeedsDataUpdate)
        _c(NeedsDataAttachmentUpdate)
        _c(NeedsNodeEnabledUpdate)
        _c(NeedsNodeClipUpdate)
        _c(NeedsLayoutUpdate)
        _c(NeedsLayoutAssignmentUpdate)
        _c(NeedsNodeOpacityUpdate)
        _c(NeedsNodeUpdate)
        _c(NeedsDataClean)
        _c(NeedsNodeClean)
        _c(NeedsAnimationAdvance)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "(" << Debug::nospace << Debug::hex << UnsignedShort(value) << Debug::nospace << ")";
}

Debug& operator<<(Debug& debug, const UserInterfaceStates value) {
    return Containers::enumSetDebugOutput(debug, value, "Ui::UserInterfaceStates{}", {
        UserInterfaceState::NeedsNodeClean,
        /* Implied by NeedsNodeClean, has to be after */
        UserInterfaceState::NeedsDataClean,
        /* Implied by NeedsNodeClean, has to be after */
        UserInterfaceState::NeedsNodeUpdate,
        /* Both are a superset of NeedsDataUpdate, meaning printing just one
           would result in `UserInterfaceState::NeedsNodeOpacityUpdate|
           UserInterfaceState(0x2)` in the output. So we pass both and let the
           UserInterfaceState printer deal with that. This is also implied by
           NeedsNodeUpdate, so has to be after. */
        UserInterfaceState(UnsignedShort(UserInterfaceState::NeedsDataAttachmentUpdate|UserInterfaceState::NeedsNodeOpacityUpdate)),
        /* Implied by NeedsNodeUpdate, has to be after */
        UserInterfaceState::NeedsNodeOpacityUpdate,
        /* Implied by NeedsNodeUpdate, has to be after */
        UserInterfaceState::NeedsLayoutAssignmentUpdate,
        /* Implied by NeedsLayoutAssignmentUpdate, has to be after */
        UserInterfaceState::NeedsLayoutUpdate,
        /* Implied by NeedsLayoutUpdate, has to be after */
        UserInterfaceState::NeedsNodeClipUpdate,
        /* Implied by NeedsNodeClipUpdate, has to be after */
        UserInterfaceState::NeedsNodeEnabledUpdate,
        /* Implied by NeedsNodeEnabledUpdate, has to be after */
        UserInterfaceState::NeedsDataAttachmentUpdate,
        /* Implied by NeedsDataAttachmentUpdate, has to be after */
        UserInterfaceState::NeedsDataUpdate,
        UserInterfaceState::NeedsAnimationAdvance
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

        /* Extracted from AbstractLayer for more direct access. Filled in
           setLayerInstance(), cleared in removeLayer(). */
        LayerFeatures features;

        /* Always meant to be non-null and valid. To make insert/remove
           operations easier the list is cyclic, so the last layers's `next` is
           the same as `_state->firstLayer`. */
        LayerHandle previous;
        LayerHandle next;

        /* Offsets into the `State::animatorInstances` array for this layer.
           While there can be at most 256 animators, the offsets cannot be an
           8-bit type as it would be impossible to distinguish for a layer
           having no animators whether the remaining 256 animators are after it
           (offset = 0) or before it (offset = 256). */
        UnsignedShort dataAttachmentAnimatorOffset;
        UnsignedShort dataAnimatorOffset;
        UnsignedShort styleAnimatorOffset;

        /* 0 / 4 bytes free */
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

        UnsignedShort:16;

        /* Have to be preserved as it's a running offset maintained across both
           used and free layers */
        UnsignedShort dataAttachmentAnimatorOffset;
        UnsignedShort dataAnimatorOffset;
        UnsignedShort styleAnimatorOffset;
    } free;
};

static_assert(
    offsetof(Layer::Used, instance) == offsetof(Layer::Free, instance) &&
    offsetof(Layer::Used, generation) == offsetof(Layer::Free, generation) &&
    offsetof(Layer::Used, dataAttachmentAnimatorOffset) == offsetof(Layer::Free, dataAttachmentAnimatorOffset) &&
    offsetof(Layer::Used, dataAnimatorOffset) == offsetof(Layer::Free, dataAnimatorOffset) &&
    offsetof(Layer::Used, styleAnimatorOffset) == offsetof(Layer::Free, styleAnimatorOffset),
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

union Animator {
    explicit Animator() noexcept: used{} {}
    Animator(const Animator&&) = delete;
    Animator(Animator&& other) noexcept: used{Utility::move(other.used)} {}
    ~Animator() {
        used.instance.~Pointer();
    }
    Animator& operator=(const Animator&&) = delete;
    Animator& operator=(Animator&& other) noexcept {
        Utility::swap(other.used, used);
        return *this;
    }

    struct Used {
        /* Except for the generation (and instance, which is reset during
           removal), these have to be re-filled every time a handle is
           recycled, so it doesn't make sense to initialize them to
           anything. */

        /* Animator instance. Null for newly created animators until
           set*AnimatorInstance() is called, set back to null in
           removeAnimator(). */
        Containers::Pointer<AbstractAnimator> instance;

        /* Together with index of this item in `animators` used for creating a
           AnimatorHandle. Increased every time a handle reaches
           removeAnimator(). Has to be initially non-zero to differentiate the
           first ever handle (with index 0) from AnimatorHandle::Null. Once
           wraps back to zero once the handle gets disabled. */
        UnsignedByte generation = 1;

        /* 7 byte free */
    } used;

    /* Used only if the Animator is among the free ones */
    struct Free {
        /* The instance pointer value has to be preserved in order to be able
           to distinguish between used animators with set instances and others
           (free or not set yet), which is used for iterating them in clean()
           and update(). If free, the member is always null, to not cause
           issues when moving & destructing. There's no other way to
           distinguish free and used animators apart from walking the free
           list. */
        void* instance;

        /* The generation value has to be preserved in order to increment it
           next time it gets used */
        UnsignedByte generation;

        /* See State::firstFreeAnimator for more information. Has to be larger
           than 8 bits in order to distinguish between index 255 and "no next
           free layouter" (which is now 65535). */
        UnsignedShort next;
    } free;
};

static_assert(
    offsetof(Animator::Used, instance) == offsetof(Animator::Free, instance) &&
    offsetof(Animator::Used, generation) == offsetof(Animator::Free, generation),
    "Animator::Used and Free layout not compatible");

union Node {
    explicit Node() noexcept: used{} {}

    struct Used {
        /* Except for the generation, these have to be re-filled every time a
           handle is recycled, so it doesn't make sense to initialize them to
           anything. */

        /* Parent node handle or Null for root nodes */
        NodeHandle parent;

        /* If not ~UnsignedInt{}, the node is part of the top-level node order
           and the value is index into the nodeOrder array, which then stores a
           doubly linked list, see the `NodeOrder` struct for details. If
           ~UnsignedInt{}, the node is not included in the draw and event
           processing order.

           There can be only as many ordered nodes as total nodes, so the last
           12 bits are unused. */
        UnsignedInt order;

        /* Together with index of this item in `nodes` used for creating a
           NodeHandle. Increased every time a handle reaches removeNode(). Has
           to be initially non-zero to differentiate the first ever handle
           (with index 0) from NodeHandle::Null. Once becomes
           `1 << NodeHandleGenerationBits` the handle gets disabled. */
        UnsignedShort generation = 1;

        NodeFlags flags{NoInit};

        /* One byte free */

        /* Initial offset and size passed to layouters, if present. Only the
           final offset and size produced by the whole layouter chain actually
           ends up being used for event handling propagation and clipping. */
        Vector2 offset{NoInit}, size{NoInit};

        /* Initial node opacity. The actual value passed to layers is
           multiplied with opacity of all parents. */
        Float opacity;
    } used;

    /* Used only if the Node is among the free ones */
    struct Free {
        /* Free nodes need to have this preserved, with generation set to 0 and
           ID to all 1s, to avoid calling removeNode() again on free items in
           clean(). There's no other way to distinguish free and used nodes
           apart from walking the free list. */
        NodeHandle parent;

        UnsignedInt:32;

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
    offsetof(Node::Used, parent) == offsetof(Node::Free, parent) &&
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

        /* If given top-level hierarchy contains child top-level hierarchies,
           points to the last such top-level node (which then points back to
           the `nodeOrder` array, and is reachable by iterating `next`).
           Changing the order then drags along all children as well, their
           mutual order doesn't change.

           If given top-level hierarchy doesn't contain any child top-level
           hierarchies, points to the top-level node itself. */
        NodeHandle lastNested;
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
    /* Renderer instance */
    Containers::Pointer<AbstractRenderer> renderer;

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

    /* Animators, indexed by AnimatorHandle */
    Containers::Array<Animator> animators;
    /* Indices into the `animators` array. The `Animator` then has a `nextFree`
       member containing the next free index. To avoid repeatedly reusing the
       same handles and exhausting their generation counter too soon, new
       animators get taken from the front and removed are put at the end. A
       value with all bits set means there's no (first/next/last) free
       animator. */
    UnsignedShort firstFreeAnimator = 0xffffu;
    UnsignedShort lastFreeAnimator = 0xffffu;

    /* Animator instances, partitioned by type. Inserted into by
       set*AnimatorInstance(), removed from by removeAnimator(), per-layer
       data animator offsets are in `Layer::dataAttachmentAnimatorOffset`. */
    Containers::Array<Containers::Reference<AbstractAnimator>> animatorInstances;
    /* Offset after which either AbstractGenericAnimator or
       AbstractNodeAnimator instances with AnimatorFeature::NodeAttachment
       are */
    UnsignedInt animatorInstancesNodeAttachmentOffset;
    /* Offset after which AbstractNodeAnimator instances with
       AnimatorFeature::NodeAttachment are */
    UnsignedInt animatorInstancesNodeOffset;

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
       which then then `Node::order` points into the `nodeOrder` array. If
       null, there's no nodes to process at all. */
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

    /* Used by advanceAnimations() */
    Nanoseconds animationTime{Math::ZeroInit};

    /* Node on which a pointer press event was accepted. Becomes null after a
       release or if an uncaptured pointer move event leaves the node area. */
    NodeHandle currentPressedNode = NodeHandle::Null;
    /* Node on which a pointer press event was accepted & captured and which
       will receive remaining pointer events until a pointer release. If null,
       a pointer isn't pressed, a capture was disabled, or the captured node
       got removed or hidden since. */
    NodeHandle currentCapturedNode = NodeHandle::Null;
    /* Node on which the last pointer move event happened. The node already
       received a pointer enter event and will receive a pointer leave event on
       the next pointer move event that leaves its area. If null, no pointer
       event happened yet or the hovered node got removed or hidden since. */
    NodeHandle currentHoveredNode = NodeHandle::Null;
    /* Position of the previous pointer event, scaled to the UI size. NullOpt
       if there was no pointer event yet. */
    /** @todo maintain previous position per pointer type? i.e., mouse, pen and
        finger independently? */
    Containers::Optional<Vector2> currentGlobalPointerPosition;
    /* Focused node */
    NodeHandle currentFocusedNode = NodeHandle::Null;

    /* Data for updates, event handling and drawing, repopulated by clean() and
       update() */
    Containers::ArrayTuple nodeStateStorage;
    Containers::ArrayView<UnsignedInt> visibleNodeIds;
    Containers::ArrayView<UnsignedInt> visibleNodeChildrenCounts;
    Containers::StridedArrayView1D<UnsignedInt> visibleFrontToBackTopLevelNodeIndices;
    Containers::ArrayView<Vector2> nodeOffsets;
    Containers::ArrayView<Vector2> nodeSizes;
    Containers::ArrayView<Vector2> absoluteNodeOffsets;
    Containers::ArrayView<Float> absoluteNodeOpacities;
    Containers::MutableBitArrayView visibleNodeMask;
    Containers::MutableBitArrayView visibleEventNodeMask;
    Containers::MutableBitArrayView visibleEnabledNodeMask;
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
    /* Data offset, clip rect offset, composite rect offset */
    Containers::ArrayView<Containers::Triple<UnsignedInt, UnsignedInt, UnsignedInt>> dataToUpdateLayerOffsets;
    Containers::ArrayView<UnsignedInt> dataToUpdateIds;
    Containers::ArrayView<UnsignedInt> dataToUpdateClipRectIds;
    Containers::ArrayView<UnsignedInt> dataToUpdateClipRectDataCounts;
    Containers::ArrayView<Vector2> dataToUpdateCompositeRectOffsets;
    Containers::ArrayView<Vector2> dataToUpdateCompositeRectSizes;
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
        "Ui::AbstractUserInterface::setSize(): expected non-zero sizes, got" << size << Debug::nospace << "," << windowSize << "and" << framebufferSize, *this);
    State& state = *_state;
    const bool sizeDifferent = state.size != size;
    const bool framebufferSizeDifferent = state.framebufferSize != framebufferSize;
    const bool sizeOrFramebufferSizeDifferent = sizeDifferent || framebufferSizeDifferent;
    state.size = size;
    state.windowSize = windowSize;
    state.framebufferSize = framebufferSize;

    /* If framebuffer size is different and renderer instance is already
       present, perform a framebuffer size setup. This is always done
       immediately so the renderer internals such as custom framebuffers are
       ready to be used by the application (clearing before draw, etc.). Layers
       that advertise LayerFeature::Composite then perform similar immediate
       setup themselves. */
    if(framebufferSizeDifferent && state.renderer)
        state.renderer->setupFramebuffers(framebufferSize);

    /* If the size is different, set a state flag to recalculate the set of
       visible nodes. I.e., some might now be outside of the UI area and
       hidden, some might be newly visible.

       Do this only if there are actually some nodes already. Otherwise it'd
       mean the state flag gets set upon construction with a size already,
       which isn't good. (This will also set it if all nodes are freed, but
       checking nodeUsedCount() which is an O(n) operation is a less efficient
       behavior than needlessly triggering a state update that's going to be a
       no-op anyway.) */
    if(sizeDifferent && state.nodes.size())
        state.state |= UserInterfaceState::NeedsNodeClipUpdate;

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

    /* If the size is different, set it on all existing layouters that have an
       instance (so, also aren't freed). Layouters that don't have an instance
       set yet will get it proxied directly in their setLayouterInstance()
       call. */
    if(sizeDifferent) for(std::size_t i = 0; i != state.layouters.size(); ++i) {
        Layouter& layouter = state.layouters[i];
        if(AbstractLayouter* const instance = layouter.used.instance.get())
            instance->setSize(size);
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
    if(!(state.state >= (UserInterfaceState::NeedsDataAttachmentUpdate|UserInterfaceState::NeedsDataClean))) for(const Layer& layer: state.layers) {
        if(const AbstractLayer* const instance = layer.used.instance.get()) {
            const LayerStates layerState = instance->state();
            if(layerState & (LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate|LayerState::NeedsSharedDataUpdate))
                states |= UserInterfaceState::NeedsDataUpdate;
            if(layerState >= LayerState::NeedsAttachmentUpdate)
                states |= UserInterfaceState::NeedsDataAttachmentUpdate;
            if(layerState >= LayerState::NeedsDataClean)
                states |= UserInterfaceState::NeedsDataClean;

            /* There's no broader state than this so if it's set, we can stop
               iterating further */
            if(states == (UserInterfaceState::NeedsDataAttachmentUpdate|UserInterfaceState::NeedsDataClean))
                break;
        }
    }

    /* Go through all animators and inherit the Needs* flags from them. Invalid
       (removed) animators have instances set to nullptr as well, so this will
       skip them. In contrast to layers and layouters, NeedsAnimationAdvance is
       never set on state.state itself, it's always inherited. */
    CORRADE_INTERNAL_ASSERT(!(state.state >= UserInterfaceState::NeedsAnimationAdvance));
    for(const Animator& animator: state.animators) {
        if(const AbstractAnimator* const instance = animator.used.instance.get()) {
            const AnimatorStates animatorState = instance->state();
            if(animatorState >= AnimatorState::NeedsAdvance)
                states |= UserInterfaceState::NeedsAnimationAdvance;

            /* There's no broader state than this so if it's set, we can stop
               iterating further */
            if(states == UserInterfaceState::NeedsAnimationAdvance)
                break;
        }
    }

    return state.state|states;
}

Nanoseconds AbstractUserInterface::animationTime() const {
    return _state->animationTime;
}

AbstractRenderer& AbstractUserInterface::setRendererInstance(Containers::Pointer<AbstractRenderer>&& instance) {
    State& state = *_state;
    CORRADE_ASSERT(instance,
        "Ui::AbstractUserInterface::setRendererInstance(): instance is null", *state.renderer);
    CORRADE_ASSERT(!state.renderer,
        "Ui::AbstractUserInterface::setRendererInstance(): instance already set", *instance);

    /* If the renderer doesn't support compositing, check we don't have any
       layers that need it. This is a linear loop, but with an assumption that
       the renderer is only set once, there isn't that many layers (and layers
       are usually added after) it shouldn't be a perf bottleneck. A similar
       check, verifying that a renderer supports compositing if a compositing
       layer is added, is in setLayerInstance(). */
    #ifndef CORRADE_NO_ASSERT
    if(!(instance->features() >= RendererFeature::Composite)) {
        for(const Layer& layer: state.layers)
            CORRADE_ASSERT(!layer.used.instance || !(layer.used.instance->features() >= LayerFeature::Composite),
                "Ui::AbstractUserInterface::setRendererInstance(): renderer without" << RendererFeature::Composite << "not usable with a layer that has" << layer.used.instance->features(), *instance);
    }
    #endif

    state.renderer = Utility::move(instance);
    /* If we already know the framebuffer size, perform framebuffer size
       setup. Do it immediately so the renderer internals such as custom
       framebuffers are ready to be used by the application. Only the
       subsequent setSize() calls get deferred to updateRenderer(). If a size
       isn't known yet, this is done in setSize() instead. */
    if(!state.size.isZero()) {
        CORRADE_INTERNAL_ASSERT(!state.framebufferSize.isZero());
        state.renderer->setupFramebuffers(state.framebufferSize);
    }
    return *state.renderer;
}

bool AbstractUserInterface::hasRenderer() const {
    return !!_state->renderer;
}

const AbstractRenderer& AbstractUserInterface::renderer() const {
    const State& state = *_state;
    CORRADE_ASSERT(state.renderer,
        "Ui::AbstractUserInterface::renderer(): no renderer instance set",
        /* Dereferencing get() to not hit another assert in Pointer */
        *state.renderer.get());
    return *state.renderer;
}

AbstractRenderer& AbstractUserInterface::renderer() {
    return const_cast<AbstractRenderer&>(const_cast<const AbstractUserInterface&>(*this).renderer());
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
        "Ui::AbstractUserInterface::layerPrevious(): invalid handle" << handle, {});
    const State& state = *_state;
    if(state.firstLayer == handle)
        return LayerHandle::Null;
    return state.layers[layerHandleId(handle)].used.previous;
}

LayerHandle AbstractUserInterface::layerNext(LayerHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractUserInterface::layerNext(): invalid handle" << handle, {});
    const State& state = *_state;
    const LayerHandle next = state.layers[layerHandleId(handle)].used.next;
    if(state.firstLayer == next)
        return LayerHandle::Null;
    return next;
}

LayerHandle AbstractUserInterface::createLayer(const LayerHandle before) {
    CORRADE_ASSERT(before == LayerHandle::Null || isHandleValid(before),
        "Ui::AbstractUserInterface::createLayer(): invalid before handle" << before, {});

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
            "Ui::AbstractUserInterface::createLayer(): can only have at most" << (1 << Implementation::LayerHandleIdBits) << "layers", {});
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

    /* (Re)initialize running offsets for attached data animators */
    Implementation::partitionedAnimatorsCreateLayer(state.animatorInstances,
        stridedArrayView(state.layers).slice(&Layer::used).slice(&Layer::Used::dataAttachmentAnimatorOffset),
        stridedArrayView(state.layers).slice(&Layer::used).slice(&Layer::Used::dataAnimatorOffset),
        stridedArrayView(state.layers).slice(&Layer::used).slice(&Layer::Used::styleAnimatorOffset),
        handle);

    return handle;
}

AbstractLayer& AbstractUserInterface::setLayerInstance(Containers::Pointer<AbstractLayer>&& instance) {
    State& state = *_state;
    CORRADE_ASSERT(instance,
        "Ui::AbstractUserInterface::setLayerInstance(): instance is null", *state.layers[0].used.instance);
    const LayerHandle handle = instance->handle();
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractUserInterface::setLayerInstance(): invalid handle" << handle, *state.layers[0].used.instance);
    const UnsignedInt id = layerHandleId(handle);
    CORRADE_ASSERT(!state.layers[id].used.instance,
        "Ui::AbstractUserInterface::setLayerInstance(): instance for" << handle << "already set", *state.layers[0].used.instance);
    /* A similar check, verifying that a renderer supports compositing if
       there's already a compositing layer, is in setRendererInstance() */
    CORRADE_ASSERT(!(instance->features() >= LayerFeature::Composite) || !state.renderer || (state.renderer->features() >= RendererFeature::Composite),
        "Ui::AbstractUserInterface::setLayerInstance(): layer with" << LayerFeature::Composite << "not usable with a renderer that has" << state.renderer->features(), *instance);

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
        "Ui::AbstractUserInterface::layer(): invalid handle" << handle,
        *state.layers[0].used.instance);
    const UnsignedInt id = layerHandleId(handle);
    CORRADE_ASSERT(state.layers[id].used.instance,
        "Ui::AbstractUserInterface::layer():" << handle << "has no instance set", *state.layers[0].used.instance);
    return *state.layers[id].used.instance;
}

AbstractLayer& AbstractUserInterface::layer(const LayerHandle handle) {
    return const_cast<AbstractLayer&>(const_cast<const AbstractUserInterface&>(*this).layer(handle));
}

void AbstractUserInterface::removeLayer(const LayerHandle handle) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractUserInterface::removeLayer(): invalid handle" << handle, );
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

    /* Prune animators associated with the to-be-removed layer from the
       list */
    Implementation::partitionedAnimatorsRemoveLayer(state.animatorInstances,
        stridedArrayView(state.layers).slice(&Layer::used).slice(&Layer::Used::dataAttachmentAnimatorOffset),
        stridedArrayView(state.layers).slice(&Layer::used).slice(&Layer::Used::dataAnimatorOffset),
        stridedArrayView(state.layers).slice(&Layer::used).slice(&Layer::Used::styleAnimatorOffset),
        handle);

    /* Delete the instance. The instance being null then means that the layer
       is either free or is newly created until setLayerInstance() is called,
       which is used for iterating them in clean() and update(). */
    layer.used.instance = nullptr;
    /* Clear also the feature set, as that can be used by certain hot loops
       without checking that given layer instance is actually present */
    layer.used.features = {};

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
        "Ui::AbstractUserInterface::attachData(): invalid handle" << node, );
    CORRADE_ASSERT(isHandleValid(data),
        "Ui::AbstractUserInterface::attachData(): invalid handle" << data, );
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
        "Ui::AbstractUserInterface::layouterPrevious(): invalid handle" << handle, {});
    const State& state = *_state;
    if(state.firstLayouter == handle)
        return LayouterHandle::Null;
    return state.layouters[layouterHandleId(handle)].used.previous;
}

LayouterHandle AbstractUserInterface::layouterNext(LayouterHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractUserInterface::layouterNext(): invalid handle" << handle, {});
    const State& state = *_state;
    const LayouterHandle next = state.layouters[layouterHandleId(handle)].used.next;
    if(state.firstLayouter == next)
        return LayouterHandle::Null;
    return next;
}

LayouterHandle AbstractUserInterface::createLayouter(const LayouterHandle before) {
    CORRADE_ASSERT(before == LayouterHandle::Null || isHandleValid(before),
        "Ui::AbstractUserInterface::createLayouter(): invalid before handle" << before, {});

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
            "Ui::AbstractUserInterface::createLayouter(): can only have at most" << (1 << Implementation::LayouterHandleIdBits) << "layouters", {});
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
        "Ui::AbstractUserInterface::setLayouterInstance(): instance is null", *state.layouters[0].used.instance);
    const LayouterHandle handle = instance->handle();
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractUserInterface::setLayouterInstance(): invalid handle" << handle, *state.layouters[0].used.instance);
    const UnsignedInt id = layouterHandleId(handle);
    CORRADE_ASSERT(!state.layouters[id].used.instance,
        "Ui::AbstractUserInterface::setLayouterInstance(): instance for" << handle << "already set", *state.layouters[0].used.instance);
    Layouter& layouter = state.layouters[id];
    layouter.used.instance = Utility::move(instance);

    /* If the size is already set, immediately proxy it to the layouter. If it
       isn't, it gets done during the next setSize() call. */
    if(!state.size.isZero())
        layouter.used.instance->setSize(state.size);

    return *layouter.used.instance;
}

const AbstractLayouter& AbstractUserInterface::layouter(const LayouterHandle handle) const {
    const State& state = *_state;
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractUserInterface::layouter(): invalid handle" << handle,
        *state.layouters[0].used.instance);
    const UnsignedInt id = layouterHandleId(handle);
    CORRADE_ASSERT(state.layouters[id].used.instance,
        "Ui::AbstractUserInterface::layouter():" << handle << "has no instance set", *state.layouters[0].used.instance);
    return *state.layouters[id].used.instance;
}

AbstractLayouter& AbstractUserInterface::layouter(const LayouterHandle handle) {
    return const_cast<AbstractLayouter&>(const_cast<const AbstractUserInterface&>(*this).layouter(handle));
}

void AbstractUserInterface::removeLayouter(const LayouterHandle handle) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractUserInterface::removeLayouter(): invalid handle" << handle, );
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

std::size_t AbstractUserInterface::animatorCapacity() const {
    return _state->animators.size();
}

std::size_t AbstractUserInterface::animatorUsedCount() const {
    /* The "pointer" chasing in here is a bit nasty, but there's no other way
       to know which animators are actually used and which not. The instance is
       a nullptr for unused animators, yes, but it's also null for animators
       that don't have it set yet. */
    const State& state = *_state;
    std::size_t free = 0;
    UnsignedShort index = state.firstFreeAnimator;
    /* C, FUCK your STUPID conversion rules. Why isn't ~UnsignedShort{} 65535?
       Why it evaluates to -1?! Why UnsignedShort(~UnsignedShort{}) is the only
       way to put it back to 16 bits? Why is everything so fucking awful?! Why
       do you force me to use error-prone constants, FFS?! */
    while(index != 0xffffu) {
        index = state.animators[index].free.next;
        ++free;
    }
    return state.animators.size() - free;
}

bool AbstractUserInterface::isHandleValid(const AnimatorHandle handle) const {
    if(handle == AnimatorHandle::Null)
        return false;
    const UnsignedInt index = animatorHandleId(handle);
    const State& state = *_state;
    if(index >= state.animators.size())
        return false;
    /* Zero generation (i.e., where it wrapped around from 255) is also
       invalid.

       Note that this can still return true for manually crafted handles that
       point to free nodes with correct generation counters. The only way to
       detect that would be by either iterating the free list (slow) or by
       keeping an additional bitfield marking free items. I don't think that's
       necessary. */
    const UnsignedInt generation = animatorHandleGeneration(handle);
    return generation && generation == state.animators[index].used.generation;
}

bool AbstractUserInterface::isHandleValid(const AnimationHandle handle) const {
    if(animationHandleData(handle) == AnimatorDataHandle::Null ||
       animationHandleAnimator(handle) == AnimatorHandle::Null)
        return false;
    const UnsignedInt animatorIndex = animationHandleAnimatorId(handle);
    const State& state = *_state;
    if(animatorIndex >= state.animators.size())
        return false;
    const Animator& animator = state.animators[animatorIndex];
    if(!animator.used.instance)
        return false;
    return animationHandleAnimatorGeneration(handle) == animator.used.generation && animator.used.instance->isHandleValid(animationHandleData(handle));
}

AnimatorHandle AbstractUserInterface::createAnimator() {
    /* Find the first free animator if there is, update the free index to point
       to the next one (or none) */
    Animator* animator;
    /* C, FUCK your STUPID conversion rules. Why isn't ~UnsignedShort{} 65535?
       Why it evaluates to -1?! Why UnsignedShort(~UnsignedShort{}) is the only
       way to put it back to 16 bits? Why is everything so fucking awful?! Why
       do you force me to use error-prone constants, FFS?! */
    State& state = *_state;
    if(state.firstFreeAnimator != 0xffffu) {
        animator = &state.animators[state.firstFreeAnimator];
        /* If there's just one item in the list, make the list empty */
        if(state.firstFreeAnimator == state.lastFreeAnimator) {
            CORRADE_INTERNAL_ASSERT(animator->free.next == 0xffffu);
            state.firstFreeAnimator = state.lastFreeAnimator = 0xffffu;
        } else {
            state.firstFreeAnimator = animator->free.next;
        }

    /* If there isn't, allocate a new one */
    } else {
        CORRADE_ASSERT(state.animators.size() < 1 << Implementation::AnimatorHandleIdBits,
            "Ui::AbstractUserInterface::createAnimator(): can only have at most" << (1 << Implementation::AnimatorHandleIdBits) << "animators", {});
        animator = &arrayAppend(state.animators, InPlaceInit);
    }

    /* In both above cases the generation is already set appropriately, either
       initialized to 1, or incremented when it got remove()d (to mark existing
       handles as invalid) */
    const AnimatorHandle handle = animatorHandle((animator - state.animators), animator->used.generation);

    return handle;
}

AbstractGenericAnimator& AbstractUserInterface::setGenericAnimatorInstance(Containers::Pointer<AbstractGenericAnimator>&& instance) {
    AbstractAnimator& animator = setAnimatorInstanceInternal(
        #ifndef CORRADE_NO_ASSERT
        "Ui::AbstractUserInterface::setGenericAnimatorInstance():",
        #endif
        Utility::move(instance), Int(Implementation::AnimatorType::Generic));

    return static_cast<AbstractGenericAnimator&>(animator);
}

AbstractNodeAnimator& AbstractUserInterface::setNodeAnimatorInstance(Containers::Pointer<AbstractNodeAnimator>&& instance) {
    /* Null instance checked in setAnimatorInstanceInternal() below, avoid
       accessing it here */
    CORRADE_ASSERT(!instance || instance->features() >= AnimatorFeature::NodeAttachment,
        "Ui::AbstractUserInterface::setNodeAnimatorInstance():" << AnimatorFeature::NodeAttachment << "not advertised for a node animator", *instance);

    AbstractAnimator& animator = setAnimatorInstanceInternal(
        #ifndef CORRADE_NO_ASSERT
        "Ui::AbstractUserInterface::setNodeAnimatorInstance():",
        #endif
        Utility::move(instance), Int(Implementation::AnimatorType::Node));

    return static_cast<AbstractNodeAnimator&>(animator);
}

AbstractDataAnimator& AbstractUserInterface::setDataAnimatorInstance(Containers::Pointer<AbstractDataAnimator>&& instance) {
    /* Null instance checked in setAnimatorInstanceInternal() below, avoid
       accessing it here */
    CORRADE_ASSERT(!instance || instance->features() >= AnimatorFeature::DataAttachment,
        "Ui::AbstractUserInterface::setDataAnimatorInstance():" << AnimatorFeature::DataAttachment << "not advertised for a data animator", *instance);

    AbstractAnimator& animator = setAnimatorInstanceInternal(
        #ifndef CORRADE_NO_ASSERT
        "Ui::AbstractUserInterface::setDataAnimatorInstance():",
        #endif
        Utility::move(instance), Int(Implementation::AnimatorType::Data));

    return static_cast<AbstractDataAnimator&>(animator);
}

AbstractStyleAnimator& AbstractUserInterface::setStyleAnimatorInstance(Containers::Pointer<AbstractStyleAnimator>&& instance) {
    /* Null instance checked in setAnimatorInstanceInternal() below, avoid
       accessing it here */
    CORRADE_ASSERT(!instance || instance->features() >= AnimatorFeature::DataAttachment,
        "Ui::AbstractUserInterface::setStyleAnimatorInstance():" << AnimatorFeature::DataAttachment << "not advertised for a style animator", *instance);

    AbstractAnimator& animator = setAnimatorInstanceInternal(
        #ifndef CORRADE_NO_ASSERT
        "Ui::AbstractUserInterface::setStyleAnimatorInstance():",
        #endif
        Utility::move(instance), Int(Implementation::AnimatorType::Style));

    return static_cast<AbstractStyleAnimator&>(animator);
}

AbstractAnimator& AbstractUserInterface::setAnimatorInstanceInternal(
    #ifndef CORRADE_NO_ASSERT
    const char* const messagePrefix,
    #endif
    Containers::Pointer<AbstractAnimator>&& instance, const Int type)
{
    State& state = *_state;
    CORRADE_ASSERT(instance,
        messagePrefix << "instance is null", *state.animators[0].used.instance);
    const AnimatorHandle handle = instance->handle();
    CORRADE_ASSERT(isHandleValid(handle),
        messagePrefix << "invalid handle" << handle, *state.animators[0].used.instance);
    const UnsignedInt id = animatorHandleId(handle);
    CORRADE_ASSERT(!state.animators[id].used.instance,
        messagePrefix << "instance for" << handle << "already set", *state.animators[0].used.instance);
    CORRADE_ASSERT(!(instance->features() >= AnimatorFeature::DataAttachment) || instance->layer() != LayerHandle::Null,
        messagePrefix << "no layer set for a data attachment animator", *state.animators[0].used.instance);

    /* Insert into the partitioned animator list based on what features are
       supported */
    Implementation::partitionedAnimatorsInsert(state.animatorInstances,
        *instance,
        Implementation::AnimatorType(type),
        instance->features(),
        instance->features() >= AnimatorFeature::DataAttachment ? instance->layer() : LayerHandle::Null,
        state.animatorInstancesNodeAttachmentOffset,
        state.animatorInstancesNodeOffset,
        stridedArrayView(state.layers).slice(&Layer::used).slice(&Layer::Used::dataAttachmentAnimatorOffset),
        stridedArrayView(state.layers).slice(&Layer::used).slice(&Layer::Used::dataAnimatorOffset),
        stridedArrayView(state.layers).slice(&Layer::used).slice(&Layer::Used::styleAnimatorOffset));

    /* Take over the instance */
    Animator& animator = state.animators[id];
    animator.used.instance = Utility::move(instance);

    return *animator.used.instance;
}

const AbstractAnimator& AbstractUserInterface::animator(const AnimatorHandle handle) const {
    const State& state = *_state;
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractUserInterface::animator(): invalid handle" << handle,
        *state.animators[0].used.instance);
    const UnsignedInt id = animatorHandleId(handle);
    CORRADE_ASSERT(state.animators[id].used.instance,
        "Ui::AbstractUserInterface::animator():" << handle << "has no instance set", *state.animators[0].used.instance);
    return *state.animators[id].used.instance;
}

AbstractAnimator& AbstractUserInterface::animator(const AnimatorHandle handle) {
    return const_cast<AbstractAnimator&>(const_cast<const AbstractUserInterface&>(*this).animator(handle));
}

void AbstractUserInterface::removeAnimator(const AnimatorHandle handle) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractUserInterface::removeAnimator(): invalid handle" << handle, );
    const UnsignedInt id = animatorHandleId(handle);
    State& state = *_state;
    Animator& animator = state.animators[id];

    /* If the animator has an instance, find it in the partitioned instance
       list and remove */
    if(const AbstractAnimator* const instance = animator.used.instance.get())
        Implementation::partitionedAnimatorsRemove(state.animatorInstances,
            *instance,
            instance->features(),
            instance->features() >= AnimatorFeature::DataAttachment ? instance->layer() : LayerHandle::Null,
            state.animatorInstancesNodeAttachmentOffset,
            state.animatorInstancesNodeOffset,
            stridedArrayView(state.layers).slice(&Layer::used).slice(&Layer::Used::dataAttachmentAnimatorOffset),
            stridedArrayView(state.layers).slice(&Layer::used).slice(&Layer::Used::dataAnimatorOffset),
            stridedArrayView(state.layers).slice(&Layer::used).slice(&Layer::Used::styleAnimatorOffset));

    /* Delete the instance. The instance being null then means that the
       animator is either free or is newly created until set*AnimatorInstance()
       is called, which is used for iterating them in clean() and update(). */
    animator.used.instance = nullptr;

    /* Increase the animator generation so existing handles pointing to this
       animator are invalidated */
    ++animator.used.generation;

    /* Put the animator at the end of the free list (while they're allocated
       from the front) to not exhaust the generation counter too fast. If the
       free list is empty however, update also the index of the first free
       animator.

       Don't do this if the generation wrapped around. That makes it disabled,
       i.e. impossible to be recycled later, to avoid aliasing old handles. */
    if(animator.used.generation != 0) {
        /* C, FUCK your STUPID conversion rules. Why isn't ~UnsignedShort{}
           65535? Why it evaluates to -1?! Why UnsignedShort(~UnsignedShort{})
           is the only way to put it back to 16 bits? Why is everything so
           fucking awful?! Why do you force me to use error-prone constants,
           FFS?! */
        animator.free.next = 0xffffu;
        if(state.lastFreeAnimator == 0xffffu) {
            CORRADE_INTERNAL_ASSERT(
                state.firstFreeAnimator == 0xffffu &&
                state.lastFreeAnimator == 0xffffu);
            state.firstFreeAnimator = id;
        } else {
            state.animators[state.lastFreeAnimator].free.next = id;
        }
        state.lastFreeAnimator = id;
    }

    /* Unlike layers or layouters, an animator being removed doesn't cause any
       visual change -- it's just that things that used to change as a result
       of an animation aren't changing anymore, which doesn't need any state
       flag update */
}

void AbstractUserInterface::attachAnimation(const NodeHandle node, const AnimationHandle animation) {
    CORRADE_ASSERT(node == NodeHandle::Null || isHandleValid(node),
        "Ui::AbstractUserInterface::attachAnimation(): invalid handle" << node, );
    CORRADE_ASSERT(isHandleValid(animation),
        "Ui::AbstractUserInterface::attachAnimation(): invalid handle" << animation, );
    State& state = *_state;
    AbstractAnimator& instance = *state.animators[animationHandleAnimatorId(animation)].used.instance;
    CORRADE_ASSERT(instance.features() & AnimatorFeature::NodeAttachment,
        "Ui::AbstractUserInterface::attachAnimation(): node attachment not supported by this animator", );
    /** @todo this performs the animation handle & feature validity check
        redundantly again, consider using some internal assert-less helper if
        it proves to be a bottleneck */
    instance.attach(animationHandleData(animation), node);

    /* There's no state flag set by AbstractAnimator::attach(), nothing to do
       here either */
}

void AbstractUserInterface::attachAnimation(const DataHandle data, const AnimationHandle animation) {
    CORRADE_ASSERT(data == DataHandle::Null || isHandleValid(data),
        "Ui::AbstractUserInterface::attachAnimation(): invalid handle" << data, );
    CORRADE_ASSERT(isHandleValid(animation),
        "Ui::AbstractUserInterface::attachAnimation(): invalid handle" << animation, );
    AbstractAnimator& instance = *_state->animators[animationHandleAnimatorId(animation)].used.instance;
    CORRADE_ASSERT(instance.features() & AnimatorFeature::DataAttachment,
        "Ui::AbstractUserInterface::attachAnimation(): data attachment not supported by this animator", );
    /* The instance is enforced to have a layer set in set*AnimatorInstance()
       already, no need to check that again here */
    CORRADE_INTERNAL_ASSERT(instance.layer() != LayerHandle::Null);
    CORRADE_ASSERT(data == DataHandle::Null || instance.layer() == dataHandleLayer(data),
        "Ui::AbstractUserInterface::attachAnimation(): expected a data handle with" << instance.layer() << "but got" << data, );
    /** @todo this performs the animation handle, feature & layer validity
        check redundantly again, consider using some internal assert-less
        helper if it proves to be a bottleneck */
    instance.attach(animationHandleData(animation), data);

    /* There's no state flag set by AbstractAnimator::attach(), nothing to do
       here either */
}

std::size_t AbstractUserInterface::nodeCapacity() const {
    return _state->nodes.size();
}

std::size_t AbstractUserInterface::nodeUsedCount() const {
    /* The "pointer" chasing in here is a bit nasty, but there's no other way
       to know which nodes are actually used and which not. The parent is Null
       for unused nodes, yes, but it's also Null for top-level nodes, and
       changing it to some other bit pattern such as generation being 0 would
       mean orphaned node removal in clean() has to do a more complex check
       than just comparing against Null. */
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
        "Ui::AbstractUserInterface::createNode(): invalid parent handle" << parent, {});

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
            "Ui::AbstractUserInterface::createNode(): can only have at most" << (1 << Implementation::NodeHandleIdBits) << "nodes", {});
        node = &arrayAppend(state.nodes, InPlaceInit);
    }

    /* Fill the data. In both above cases the generation is already set
       appropriately, either initialized to 1, or incremented when it got
       remove()d (to mark existing handles as invalid) */
    node->used.parent = parent;
    node->used.flags = flags;
    node->used.offset = offset;
    node->used.size = size;
    node->used.opacity = 1.0f;
    const NodeHandle handle = nodeHandle(node - state.nodes, node->used.generation);

    /* If a root node, implicitly mark it as last in the node order, so
       it's drawn at the front. The setNodeOrder() internally reconnects, so
       set ~UnsignedInt{} first to mark it as not in top-level order yet. */
    node->used.order = ~UnsignedInt{};
    if(parent == NodeHandle::Null)
        setNodeOrder(handle, NodeHandle::Null);

    /* Mark the UI as needing an update() call to refresh node state */
    state.state |= UserInterfaceState::NeedsNodeUpdate;

    return handle;
}

NodeHandle AbstractUserInterface::createNode(const Vector2& offset, const Vector2& size, const NodeFlags flags) {
    return createNode(NodeHandle::Null, offset, size, flags);
}

NodeHandle AbstractUserInterface::nodeParent(const NodeHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractUserInterface::nodeParent(): invalid handle" << handle, {});
    return _state->nodes[nodeHandleId(handle)].used.parent;
}

Vector2 AbstractUserInterface::nodeOffset(const NodeHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractUserInterface::nodeOffset(): invalid handle" << handle, {});
    return _state->nodes[nodeHandleId(handle)].used.offset;
}

void AbstractUserInterface::setNodeOffset(const NodeHandle handle, const Vector2& offset) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractUserInterface::setNodeOffset(): invalid handle" << handle, );
    State& state = *_state;
    state.nodes[nodeHandleId(handle)].used.offset = offset;

    /* Mark the UI as needing an update() call to refresh node layout state */
    state.state |= UserInterfaceState::NeedsLayoutUpdate;
}

Vector2 AbstractUserInterface::nodeSize(const NodeHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractUserInterface::nodeSize(): invalid handle" << handle, {});
    return _state->nodes[nodeHandleId(handle)].used.size;
}

void AbstractUserInterface::setNodeSize(const NodeHandle handle, const Vector2& size) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractUserInterface::setNodeSize(): invalid handle" << handle, );
    State& state = *_state;
    state.nodes[nodeHandleId(handle)].used.size = size;

    /* Mark the UI as needing an update() call to refresh node layout state */
    state.state |= UserInterfaceState::NeedsLayoutUpdate;
}

Float AbstractUserInterface::nodeOpacity(const NodeHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractUserInterface::nodeOpacity(): invalid handle" << handle, {});
    return _state->nodes[nodeHandleId(handle)].used.opacity;
}

void AbstractUserInterface::setNodeOpacity(const NodeHandle handle, const Float opacity) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractUserInterface::setNodeOpacity(): invalid handle" << handle, );
    State& state = *_state;
    state.nodes[nodeHandleId(handle)].used.opacity = opacity;

    /* Mark the UI as needing an update() call to refresh calculated node
       opacities */
    state.state |= UserInterfaceState::NeedsNodeOpacityUpdate;
}

NodeFlags AbstractUserInterface::nodeFlags(const NodeHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractUserInterface::nodeFlags(): invalid handle" << handle, {});
    return _state->nodes[nodeHandleId(handle)].used.flags;
}

void AbstractUserInterface::setNodeFlagsInternal(const UnsignedInt id, const NodeFlags flags) {
    State& state = *_state;
    if((state.nodes[id].used.flags & NodeFlag::Hidden) != (flags & NodeFlag::Hidden))
        state.state |= UserInterfaceState::NeedsNodeUpdate;
    if((state.nodes[id].used.flags & NodeFlag::Clip) != (flags & NodeFlag::Clip))
        state.state |= UserInterfaceState::NeedsNodeClipUpdate;
    /* Right now Focusable wouldn't need the full NeedsNodeEnabledUpdate, just
       something that triggers state.currentFocusedNode update. But eventually
       there will be focusable node fallbacks / trees (where pressing on a node
       that's not focusable itself but its parent is focuses the parent), which
       then will need the full process as NoEvents and Disabled as well. */
    if((state.nodes[id].used.flags & (NodeFlag::NoEvents|NodeFlag::Disabled|NodeFlag::Focusable)) != (flags & (NodeFlag::NoEvents|NodeFlag::Disabled|NodeFlag::Focusable)))
        state.state |= UserInterfaceState::NeedsNodeEnabledUpdate;
    state.nodes[id].used.flags = flags;
}

void AbstractUserInterface::setNodeFlags(const NodeHandle handle, const NodeFlags flags) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractUserInterface::setNodeFlags(): invalid handle" << handle, );
    setNodeFlagsInternal(nodeHandleId(handle), flags);
}

void AbstractUserInterface::addNodeFlags(const NodeHandle handle, const NodeFlags flags) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractUserInterface::addNodeFlags(): invalid handle" << handle, );
    const UnsignedInt id = nodeHandleId(handle);
    setNodeFlagsInternal(id, _state->nodes[id].used.flags|flags);
}

void AbstractUserInterface::clearNodeFlags(const NodeHandle handle, const NodeFlags flags) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractUserInterface::clearNodeFlags(): invalid handle" << handle, );
    const UnsignedInt id = nodeHandleId(handle);
    setNodeFlagsInternal(id, _state->nodes[id].used.flags & ~flags);
}

void AbstractUserInterface::removeNode(const NodeHandle handle) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractUserInterface::removeNode(): invalid handle" << handle, );

    removeNodeInternal(nodeHandleId(handle));

    /* Mark the UI as needing a clean() call to refresh node state */
    _state->state |= UserInterfaceState::NeedsNodeClean;
}

inline void AbstractUserInterface::removeNodeInternal(const UnsignedInt id) {
    State& state = *_state;
    Node& node = state.nodes[id];

    /* If this was a top-level node, disconnect it from the node order list and
       put (it including its potential nested top-level nodes) to the free
       list.

       It's done for all nested top-level nodes at once instead of deferring to
       clean() because this way they can get all directly freed with no
       reconnection needing to be done. */
    if(node.used.order != ~UnsignedInt{}) {
        NodeHandle handle = nodeHandle(id, node.used.generation);
        clearNodeOrderInternal(handle);

        /* Go through this node as well as all potential child top-level nodes
           until we reach a null next handle, which clearNodeOrderInternal()
           set for the lastNested */
        while(handle != NodeHandle::Null) {
            Node& childOrderNode = state.nodes[nodeHandleId(handle)];
            UnsignedInt order = childOrderNode.used.order;
            CORRADE_INTERNAL_ASSERT(order != ~UnsignedInt{});
            handle = state.nodeOrder[order].used.next;
            state.nodeOrder[order].free.next = state.firstFreeNodeOrder;
            state.firstFreeNodeOrder = order;
            childOrderNode.used.order = ~UnsignedInt{};
        }

        /* NeedsNodeUpdate gets set by either removeNode() (implied by
           NeedsNodeClean) or is already set (again as a consequence of
           NeedsNodeClean) in order to even enter clean(), which calls here */
    }

    /* Increase the node generation so existing handles pointing to this
       node are invalidated */
    ++node.used.generation;

    /* Parent the node to the root to prevent it from being removed again in
       clean() when its parents get removed as well. Removing more than once
       would lead to cycles in the free list. */
    node.used.parent = NodeHandle::Null;

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
    const UnsignedInt order = state.nodes[nodeHandleId(state.firstNodeOrder)].used.order;
    CORRADE_INTERNAL_ASSERT(order != ~UnsignedInt{});
    return state.nodeOrder[order].used.previous;
}

bool AbstractUserInterface::isNodeTopLevel(const NodeHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractUserInterface::isNodeTopLevel(): invalid handle" << handle, {});
    return _state->nodes[nodeHandleId(handle)].used.order != ~UnsignedInt{};
}

bool AbstractUserInterface::isNodeOrdered(const NodeHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractUserInterface::isNodeOrdered(): invalid handle" << handle, {});
    const State& state = *_state;
    const Node& node = state.nodes[nodeHandleId(handle)];
    return node.used.order != ~UnsignedInt{} &&
        state.nodeOrder[node.used.order].used.previous != NodeHandle::Null;
}

NodeHandle AbstractUserInterface::nodeOrderPrevious(const NodeHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractUserInterface::nodeOrderPrevious(): invalid handle" << handle, {});
    const State& state = *_state;
    const Node& node = state.nodes[nodeHandleId(handle)];
    if(node.used.order == ~UnsignedInt{})
        return NodeHandle::Null;
    if(state.firstNodeOrder == handle)
        return NodeHandle::Null;
    return state.nodeOrder[node.used.order].used.previous;
}

NodeHandle AbstractUserInterface::nodeOrderNext(const NodeHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractUserInterface::nodeOrderNext(): invalid handle" << handle, {});
    const State& state = *_state;
    const Node& node = state.nodes[nodeHandleId(handle)];
    if(node.used.order == ~UnsignedInt{})
        return NodeHandle::Null;
    const NodeHandle next = state.nodeOrder[node.used.order].used.next;
    if(state.firstNodeOrder == next)
        return NodeHandle::Null;
    return next;
}

NodeHandle AbstractUserInterface::nodeOrderLastNested(const NodeHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractUserInterface::nodeOrderNext(): invalid handle" << handle, {});
    const State& state = *_state;
    const Node& node = state.nodes[nodeHandleId(handle)];
    if(node.used.order == ~UnsignedInt{})
        return handle;
    return state.nodeOrder[node.used.order].used.lastNested;
}

namespace {

/* Used by clearNodeOrderInternal(), setNodeOrder() and flattenNodeOrder(). Not
   all tests for each of the 3 exercise all corner cases (while vs if, break
   with/without else), but in total they do. */
void updateParentLastNestedOrderTo(const Containers::ArrayView<const Node> nodes, const Containers::ArrayView<NodeOrder> nodeOrder, NodeHandle parent, const NodeHandle lastNested, const NodeHandle replace) {
    while(parent != NodeHandle::Null) {
        const Node& parentNode = nodes[nodeHandleId(parent)];
        if(parentNode.used.order != ~UnsignedInt{}) {
            NodeOrder& parentOrder = nodeOrder[parentNode.used.order];
            if(parentOrder.used.lastNested == lastNested)
                parentOrder.used.lastNested = replace;
            /* If the last nested isn't matching ours, it means it's after. As
               any further parent top-level nested ranges have to include this
               whole range as well, it means there will be no other parents
               ending at lastNested, so we can stop here. */
            else break;
        }
        parent = parentNode.used.parent;
    }
}

}

/* Compared to clearNodeOrder() this doesn't have handle validity assertions */
bool AbstractUserInterface::clearNodeOrderInternal(const NodeHandle handle) {
    State& state = *_state;
    Node& node = state.nodes[nodeHandleId(handle)];
    const UnsignedInt order = node.used.order;
    CORRADE_INTERNAL_ASSERT(order != ~UnsignedInt{});

    NodeHandle& originalPrevious = state.nodeOrder[order].used.previous;
    const NodeHandle lastNested = state.nodeOrder[order].used.lastNested;
    CORRADE_INTERNAL_ASSERT(isHandleValid(lastNested));
    const UnsignedInt lastNestedOrder = state.nodes[nodeHandleId(lastNested)].used.order;
    CORRADE_INTERNAL_ASSERT(lastNestedOrder != ~UnsignedInt{});
    NodeHandle& originalNext = state.nodeOrder[lastNestedOrder].used.next;

    /* If the node isn't connected from either side, nothing to do. Otherwise
       the node is either connected from both sides, or is a child of a
       disconnected parent, in which case it has only previous but not next. */
    if(originalPrevious == NodeHandle::Null) {
        CORRADE_INTERNAL_ASSERT(originalNext == NodeHandle::Null);
        return false;
    }
    CORRADE_INTERNAL_ASSERT(isHandleValid(originalPrevious) && (originalNext == NodeHandle::Null || isHandleValid(originalNext)));

    /* This works correctly also in case of there being just a single item in
       the list (i.e., originalPreviousOrder == originalNextOrder == order), as
       the nodeOrder entry gets unused after */
    const UnsignedInt originalPreviousOrder = state.nodes[nodeHandleId(originalPrevious)].used.order;
    state.nodeOrder[originalPreviousOrder].used.next = originalNext;
    if(originalNext != NodeHandle::Null) {
        const UnsignedInt originalNextOrder = state.nodes[nodeHandleId(originalNext)].used.order;
        state.nodeOrder[originalNextOrder].used.previous = originalPrevious;
    }
    if(state.firstNodeOrder == handle) {
        /* The node can be first in order only if it's a root node */
        CORRADE_INTERNAL_ASSERT(node.used.parent == NodeHandle::Null);
        if(handle == originalNext)
            state.firstNodeOrder = NodeHandle::Null;
        else
            state.firstNodeOrder = originalNext;
    }

    /* If lastNested was the last nested in any parent order, update it to
       point to the previous. Same logic is in flattenNodeOrder(). */
    updateParentLastNestedOrderTo(state.nodes, state.nodeOrder, node.used.parent, lastNested, originalPrevious);

    /* Once we don't need the prev/next handles anymore, set them to null to
       mark the top-level node as not connected. The order is only recycled
       when the node is removed, to avoid the need to allocate it again once
       it's put back into the top-level order. */
    originalPrevious = NodeHandle::Null;
    originalNext = NodeHandle::Null;

    return true;
}

namespace {

NodeHandle closestTopLevelParent(Containers::ArrayView<const Node> nodes, NodeHandle node) {
    /* Root nodes have `order` always allocated, so it should stop at those. */
    NodeHandle parent = nodes[nodeHandleId(node)].used.parent;
    for(;;) {
        const Node& parentNode = nodes[nodeHandleId(parent)];
        if(parentNode.used.order != ~UnsignedInt{})
            return parent;
        parent = parentNode.used.parent;
    }

    CORRADE_INTERNAL_ASSERT_UNREACHABLE();
}

}

void AbstractUserInterface::setNodeOrder(const NodeHandle handle, const NodeHandle behind) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractUserInterface::setNodeOrder(): invalid handle" << handle, );
    State& state = *_state;
    Node& node = state.nodes[nodeHandleId(handle)];
    #ifndef CORRADE_NO_ASSERT
    if(behind != NodeHandle::Null) {
        CORRADE_ASSERT(isHandleValid(behind),
            "Ui::AbstractUserInterface::setNodeOrder(): invalid behind handle" << behind, );
        CORRADE_ASSERT(handle != behind,
            "Ui::AbstractUserInterface::setNodeOrder(): can't order" << handle << "behind itself", );
        const Node& next = state.nodes[nodeHandleId(behind)];
        /* Next of lastNested should also be non-null for consistency, but
           that's too expensive to check for */
        CORRADE_ASSERT(next.used.order != ~UnsignedInt{} && state.nodeOrder[next.used.order].used.previous != NodeHandle::Null,
            "Ui::AbstractUserInterface::setNodeOrder():" << behind << "is not ordered", );
        CORRADE_ASSERT((state.nodes[nodeHandleId(behind)].used.parent == NodeHandle::Null) == (node.used.parent == NodeHandle::Null),
            "Ui::AbstractUserInterface::setNodeOrder():" << handle << (node.used.parent == NodeHandle::Null ? "is a root node but" : "is not a root node but") << behind << (node.used.parent == NodeHandle::Null ? "is not" : "is"), );
    }
    #endif

    /* If the node isn't in the order yet, add it. That happens when calling
       setNodeOrder() for a root node from within createNode(), or when setting
       order on a non-root order for the first time. */
    if(node.used.order == ~UnsignedInt{}) {
        /* Find the first free slot if there is, update the free index to point
           to the next one (or none) */
        if(state.firstFreeNodeOrder != ~UnsignedInt{}) {
            node.used.order = state.firstFreeNodeOrder;
            state.firstFreeNodeOrder = state.nodeOrder[state.firstFreeNodeOrder].free.next;

        /* If there isn't, allocate a new one */
        } else {
            /* Unlike when adding nodes / layers / ..., we don't need to
               check against max size -- because in that case there wouldn't be
               any free node handles left to call this function with
               anyway */
            node.used.order = state.nodeOrder.size();
            arrayAppend(state.nodeOrder, NoInit, 1);
        }

        /* If this is a root node, initially there are no nested top-level
           nodes (well, no nested nodes at all), because setNodeOrder() is
           called during creation of the root node itself.

           Otherwise, if this is a non-root node, set the lastNested handle to
           null to differentiate this from nodes that had their order cleared
           below (in which case lastNested is at the very least the node handle
           itself), as it needs to discover its nested nodes first. */
        state.nodeOrder[node.used.order].used.lastNested =
            node.used.parent == NodeHandle::Null ? handle : NodeHandle::Null;

    /* Otherwise remove it from the previous location in the linked list, if
       connected. The `node.used.order` stays the same -- it's reused. */
    } else clearNodeOrderInternal(handle);

    /* At this point, with the node order not being connected (yet or not
       anymore), we can figure out where to connect. The previous node gets
       saved directly to `order.used.previous`, but the next is connected to
       `lastNested` (which may not be known at this time yet), so it's put into
       a variable instead. */
    NodeOrder& order = state.nodeOrder[node.used.order];
    NodeHandle next;

    /* At this point, with the node order not being connected (yet or not
       anymore), we can figure out where to connect it. A root node can only
       connect to other root nodes, so this case is simpler. */
    if(node.used.parent == NodeHandle::Null) {
        /* If last, it gets attached after the last node and before the first
           node as the list is cyclic. If this is the first ordered node so
           far, the previous and next one is the node itself. */
        if(behind == NodeHandle::Null) {
            order.used.previous = state.firstNodeOrder == NodeHandle::Null ?
                handle : state.nodeOrder[state.nodes[nodeHandleId(state.firstNodeOrder)].used.order].used.previous;
            next = state.firstNodeOrder == NodeHandle::Null ?
                handle : state.firstNodeOrder;

        /* Otherwise it gets attached before the specified node, and after a
           node originally before the specified node */
        } else {
            order.used.previous = state.nodeOrder[state.nodes[nodeHandleId(behind)].used.order].used.previous;
            next = behind;
        }

    /* For a non-root node we have to find the closest top-level parent
       first */
    } else {
        const NodeHandle topLevelParent = closestTopLevelParent(state.nodes, handle);
        const NodeHandle topLevelParentLastNested = state.nodeOrder[state.nodes[nodeHandleId(topLevelParent)].used.order].used.lastNested;
        const NodeHandle topLevelParentLastNestedNext = state.nodeOrder[state.nodes[nodeHandleId(topLevelParentLastNested)].used.order].used.next;

        /* If it's going to be put last, it's the node that's next to the last
           nested. */
        if(behind == NodeHandle::Null) {
            order.used.previous = topLevelParentLastNested;
            next = topLevelParentLastNestedNext;
        /* Otherwise the node it's ordered before should be under the same
           nearest top-level parent */
        } else {
            CORRADE_ASSERT(closestTopLevelParent(state.nodes, behind) == topLevelParent,
                "Ui::AbstractUserInterface::setNodeOrder():" << behind << "doesn't share the nearest top-level parent with" << handle, );
            order.used.previous = state.nodeOrder[state.nodes[nodeHandleId(behind)].used.order].used.previous;
            next = behind;
        }

        /* If this is a fresh new top-level node (marked by null above), we
           check that there aren't any top-level nodes already nested
           underneath. Handling those would mean a lot of extra logic and
           caveats (basically cutting them out of the flow and reordering them
           to whatever place the new top-level node is, and they can be
           arbitrarily mixed up with others, so the visibile order would change
           as well), so that asserts now and the user has to clear the order
           on those first. */
        if(order.used.lastNested == NodeHandle::Null) {
            /* Right now, lastNested can only be the handle itself, any other
               case leads to an assertion below. Once this is implemented,
               `lastNested` is moved past all (potentially reconnected) nested
               top-level nodes. */
            order.used.lastNested = handle;

            #ifndef CORRADE_NO_ASSERT
            NodeHandle topLevelParentNested = state.nodeOrder[state.nodes[nodeHandleId(topLevelParent)].used.order].used.next;
            while(topLevelParentNested != topLevelParentLastNestedNext) {
                const NodeHandle topLevelParentNestedLastNested = state.nodeOrder[state.nodes[nodeHandleId(topLevelParentNested)].used.order].used.lastNested;
                const NodeHandle topLevelParentNestedLastNestedNext = state.nodeOrder[state.nodes[nodeHandleId(topLevelParentNestedLastNested)].used.order].used.next;
                CORRADE_ASSERT(closestTopLevelParent(state.nodes, topLevelParentNested) != handle,
                    "Ui::AbstractUserInterface::setNodeOrder(): creating a new top-level node with existing nested top-level nodes isn't implemented yet, sorry; clear the order or flatten it first", );
                topLevelParentNested = topLevelParentNestedLastNestedNext;
            }
            #endif
        }
    }

    /* Connect to the `previous` and `next` nodes picked above. There's no
       difference in handling for root and nested top-level nodes, except for
       `next` possibly being null if connecting a nested top-level node to an
       otherwise disconnected parent. */
    {
        /* Point the previous node's next handle to this node */
        const UnsignedInt previousOrder = state.nodes[nodeHandleId(order.used.previous)].used.order;
        state.nodeOrder[previousOrder].used.next = handle;

        /* Point the last nested node's next handle to next node (or null) */
        const UnsignedInt lastNestedOrder = state.nodes[nodeHandleId(order.used.lastNested)].used.order;
        state.nodeOrder[lastNestedOrder].used.next = next;

        /* Point the next node's (if any) previous handle to this node */
        if(next != NodeHandle::Null) {
            const UnsignedInt nextOrder = state.nodes[nodeHandleId(next)].used.order;
            state.nodeOrder[nextOrder].used.previous = order.used.lastNested;
        }
    }

    /* If this is a root node, may need to adjust the pointer to the first
       node as well. If this is not, firstNodeOrder can as well be null, for
       example if only connecting nested top-level nodes but the UI as a whole
       still hidden. */
    if(node.used.parent == NodeHandle::Null) {
        /* This is the first ever node to be in the order */
        if(state.firstNodeOrder == NodeHandle::Null)
            state.firstNodeOrder = handle;
        /* If the `behind` node was first, the new node is now first. If
           `behind` was Null, either the above branch was picked already or
           neither of the branches is taken. */
        else if(state.firstNodeOrder == behind) {
            CORRADE_INTERNAL_ASSERT(behind != NodeHandle::Null);
            state.firstNodeOrder = handle;
        }

    /* If this is not a root node and it was inserted at the end, may need to
       adjust lastNested of parents to point to lastNested of this node. If it
       wasn't inserted at the end, the previous lastNested all stay like
       before, so nothing needs to be adjusted. */
    } else if(behind == NodeHandle::Null) {
        updateParentLastNestedOrderTo(state.nodes, state.nodeOrder, node.used.parent, order.used.previous, order.used.lastNested);
    }

    /* Mark the UI as needing an update() call to refresh node state */
    state.state |= UserInterfaceState::NeedsNodeUpdate;
}

void AbstractUserInterface::clearNodeOrder(const NodeHandle handle) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractUserInterface::clearNodeOrder(): invalid handle" << handle, );

    /* If the node has no order allocated at all, this function is a no-op */
    State& state = *_state;
    Node& node = state.nodes[nodeHandleId(handle)];
    if(node.used.order == ~UnsignedInt{})
        return;

    /* Remove it from the linked list. If not connected, this function is a
       no-op as well. */
    if(!clearNodeOrderInternal(handle))
        return;

    /* Mark the UI as needing an update() call to refresh node state */
    state.state |= UserInterfaceState::NeedsNodeUpdate;
}

void AbstractUserInterface::flattenNodeOrder(const NodeHandle handle) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractUserInterface::flattenNodeOrder(): invalid handle" << handle, );
    State& state = *_state;
    Node& node = state.nodes[nodeHandleId(handle)];
    CORRADE_ASSERT(node.used.parent != NodeHandle::Null,
        "Ui::AbstractUserInterface::flattenNodeOrder():" << handle << "is a root node", );

    if(node.used.order == ~UnsignedInt{})
        return;

    NodeOrder& order = state.nodeOrder[node.used.order];

    if(order.used.previous != NodeHandle::Null)
        state.nodeOrder[state.nodes[nodeHandleId(order.used.previous)].used.order].used.next = order.used.next;
    if(order.used.next != NodeHandle::Null)
        state.nodeOrder[state.nodes[nodeHandleId(order.used.next)].used.order].used.previous = order.used.previous;

    /* If lastNested was the last nested in any parent order, update it to
       point to the previous */
    updateParentLastNestedOrderTo(state.nodes, state.nodeOrder, node.used.parent, order.used.lastNested, order.used.previous);

    order.free.next = state.firstFreeNodeOrder;
    state.firstFreeNodeOrder = node.used.order;

    node.used.order = ~UnsignedInt{};

    /* Mark the UI as needing an update() call to refresh node state */
    /** @todo in this case only the draw / event processing order changes, but
        nothing that would affect layouters or cause node offsets/sizes to
        change -- is there a better state flag that would cover this? */
    state.state |= UserInterfaceState::NeedsNodeUpdate;
}

AbstractUserInterface& AbstractUserInterface::clean() {
    /* Get the state including what bubbles from layers. If there's nothing to
       clean, bail. */
    const UserInterfaceStates states = this->state();
    if(!(states >= UserInterfaceState::NeedsDataClean)) {
        CORRADE_INTERNAL_ASSERT(!(states >= UserInterfaceState::NeedsNodeClean));
        return *this;
    }

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
            stridedArrayView(state.nodes).slice(&Node::used).slice(&Node::Used::parent),
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
            if(node.used.parent != NodeHandle::Null && !isHandleValid(node.used.parent))
                removeNodeInternal(id);
        }

        /* 3. Next perform a clean for layouter node assignments and data and
           animation node attachments, keeping only layouts assigned to
           (remaining) valid node handles and data/animations that are either
           not attached or attached to valid node handles. */
        const Containers::StridedArrayView1D<const UnsignedShort> nodeGenerations = stridedArrayView(state.nodes).slice(&Node::used).slice(&Node::Used::generation);

        /* In each layer remove data attached to invalid non-null nodes */
        for(Layer& layer: state.layers) if(AbstractLayer* const instance = layer.used.instance.get())
            instance->cleanNodes(nodeGenerations);

        /* In each layouter remove layouts assigned to invalid nodes */
        for(Layouter& layouter: state.layouters) if(AbstractLayouter* const instance = layouter.used.instance.get())
            instance->cleanNodes(nodeGenerations);

        /* For all animators with node attachments remove animations attached
           to invalid non-null nodes */
        for(AbstractAnimator& animator: Implementation::partitionedAnimatorsAnyNodeAttachment(state.animatorInstances, state.animatorInstancesNodeAttachmentOffset, stridedArrayView(state.layers).slice(&Layer::used).slice(&Layer::Used::dataAttachmentAnimatorOffset)))
            animator.cleanNodes(nodeGenerations);
    }

    /* If no data clean is needed, we don't need to iterate the layers to
       discover which ones need it */
    if(states >= UserInterfaceState::NeedsDataClean) {
        /* Call cleanData() only on layers that themselves set
           LayerState::NeedsDataClean or if UserInterfaceState::NeedsDataClean
           is set on the UI itself (for example implied by NeedsNodeClean), it
           doesn't make sense to do otherwise */
        for(Layer& layer: state.layers) if(AbstractLayer* const instance = layer.used.instance.get()) {
            if(state.state >= UserInterfaceState::NeedsDataClean || instance->state() >= LayerState::NeedsDataClean)
                instance->cleanData(Implementation::partitionedAnimatorsAnyDataAttachment(state.animatorInstances, stridedArrayView(state.layers).slice(&Layer::used).slice(&Layer::Used::dataAttachmentAnimatorOffset), instance->handle()));
        }
    }

    /* Unmark the UI as needing a clean() call, but keep the Update states
       including ones that bubbled up from layers. States that aren't a subset
       of NeedsNodeClean, such as NeedsRendererSizeSetup, are unaffected.
       NeedsAnimationAdvance is only propagated from the animators in state(),
       never present directly in _state->state, so clear it as well. */
    state.state = states & ~((UserInterfaceState::NeedsNodeClean|UserInterfaceState::NeedsAnimationAdvance) & ~UserInterfaceState::NeedsNodeUpdate);
    return *this;
}

AbstractUserInterface& AbstractUserInterface::advanceAnimations(const Nanoseconds time) {
    State& state = *_state;
    CORRADE_ASSERT(time >= state.animationTime,
        "Ui::AbstractUserInterface::advanceAnimations(): expected a time at least" << state.animationTime << "but got" << time, *this);

    /* Call clean implicitly in order to make the internal state ready for
       animation advance, i.e. no stale nodes or data anywhere. Is a no-op if
       there's nothing to clean. */
    clean();

    /* Storage for temporary data needed by animators, sized to cover the
       largest capacity */
    std::size_t maxCapacity = 0;
    for(const Animator& animator: state.animators) {
        if(const AbstractAnimator* const instance = animator.used.instance.get())
            maxCapacity = Math::max(instance->capacity(), maxCapacity);
    }
    Containers::MutableBitArrayView active;
    Containers::MutableBitArrayView remove;
    Containers::ArrayView<Float> factors;
    Containers::MutableBitArrayView nodesRemove;
    Containers::ArrayTuple storage{
        {NoInit, maxCapacity, active},
        {NoInit, maxCapacity, remove},
        {NoInit, maxCapacity, factors},
        {ValueInit, state.nodes.size(), nodesRemove}
    };

    /* Get the state including what bubbles from animators, then go through
       them only if there's something to advance */
    const UserInterfaceStates states = this->state();
    if(states >= UserInterfaceState::NeedsAnimationAdvance) {
        /* Common code for advancing AbstractGenericAnimator instances. It's
           done in three separate loops because generic animators are not
           contiguous in the `state.animatorInstances` array, instead they're
           grouped by whether they have node or data attachments to make the
           implementation in clean() simpler */
        const auto advanceGenericAnimator = [&](AbstractAnimator& instance) {
            if(!(instance.state() & AnimatorState::NeedsAdvance))
                return;

            const std::size_t capacity = instance.capacity();
            const Containers::Pair<bool, bool> needsAdvanceClean = instance.update(time,
                active.prefix(capacity),
                factors.prefix(capacity),
                remove.prefix(capacity));

            if(needsAdvanceClean.first())
                static_cast<AbstractGenericAnimator&>(instance).advance(
                    active.prefix(capacity),
                    factors.prefix(capacity));
            if(needsAdvanceClean.second())
                instance.clean(remove.prefix(capacity));
        };

        /* Go through all generic animators with neither NodeAttachment nor
           DataAttachment and advance ones that need it */
        for(AbstractAnimator& instance: Implementation::partitionedAnimatorsNone(state.animatorInstances, state.animatorInstancesNodeAttachmentOffset))
            advanceGenericAnimator(instance);

        /* Then all generic animators with NodeAttachment */
        const Containers::StridedArrayView1D<const UnsignedShort> dataAttachmentAnimatorOffsets = stridedArrayView(state.layers).slice(&Layer::used).slice(&Layer::Used::dataAttachmentAnimatorOffset);
        for(AbstractAnimator& instance: Implementation::partitionedAnimatorsGenericNodeAttachment(state.animatorInstances, state.animatorInstancesNodeAttachmentOffset, state.animatorInstancesNodeOffset, dataAttachmentAnimatorOffsets))
            advanceGenericAnimator(instance);

        /* Then, for each layer all generic animators with DataAttachment */
        const Containers::StridedArrayView1D<const UnsignedShort> dataAnimatorOffsets = stridedArrayView(state.layers).slice(&Layer::used).slice(&Layer::Used::dataAnimatorOffset);
        const Containers::StridedArrayView1D<const UnsignedShort> styleAnimatorOffsets = stridedArrayView(state.layers).slice(&Layer::used).slice(&Layer::Used::styleAnimatorOffset);
        for(std::size_t i = 0; i != state.layers.size(); ++i)
            for(AbstractAnimator& instance: Implementation::partitionedAnimatorsGenericDataAttachment(state.animatorInstances, dataAttachmentAnimatorOffsets, dataAnimatorOffsets, styleAnimatorOffsets, layerHandle(i, state.layers[i].used.generation)))
                advanceGenericAnimator(instance);

        /* After that, all AbstractNodeAnimator instances, remembering what all
           they modified */
        const Containers::StridedArrayView1D<Vector2> nodeOffsets = stridedArrayView(state.nodes).slice(&Node::used).slice(&Node::Used::offset);
        const Containers::StridedArrayView1D<Vector2> nodeSizes = stridedArrayView(state.nodes).slice(&Node::used).slice(&Node::Used::size);
        const Containers::StridedArrayView1D<NodeFlags> nodeFlags = stridedArrayView(state.nodes).slice(&Node::used).slice(&Node::Used::flags);
        NodeAnimations nodeAnimations;
        for(AbstractAnimator& instance: Implementation::partitionedAnimatorsNodeNodeAttachment(state.animatorInstances, state.animatorInstancesNodeAttachmentOffset, state.animatorInstancesNodeOffset, dataAttachmentAnimatorOffsets)) {
            if(!(instance.state() & AnimatorState::NeedsAdvance))
                continue;

            const std::size_t capacity = instance.capacity();
            const Containers::Pair<bool, bool> needsAdvanceClean = instance.update(time,
                active.prefix(capacity),
                factors.prefix(capacity),
                remove.prefix(capacity));

            if(needsAdvanceClean.first())
                nodeAnimations |= static_cast<AbstractNodeAnimator&>(instance).advance(
                    active.prefix(capacity),
                    factors.prefix(capacity),
                    nodeOffsets,
                    nodeSizes,
                    nodeFlags,
                    nodesRemove);
            if(needsAdvanceClean.second())
                instance.clean(remove.prefix(capacity));
        }

        /* Propagate to the global state */
        if(nodeAnimations >= NodeAnimation::OffsetSize)
            state.state |= UserInterfaceState::NeedsLayoutUpdate;
        if(nodeAnimations >= NodeAnimation::Enabled)
            state.state |= UserInterfaceState::NeedsNodeEnabledUpdate;
        if(nodeAnimations >= NodeAnimation::Clip)
            state.state |= UserInterfaceState::NeedsNodeClipUpdate;
        if(nodeAnimations >= NodeAnimation::Removal) {
            state.state |= UserInterfaceState::NeedsNodeClean;
            /** @todo some way to efficiently iterate set bits */
            for(std::size_t i = 0; i != nodesRemove.size(); ++i)
                if(nodesRemove[i]) removeNodeInternal(i);
        }

        /* Then, for each layer ... */
        /** @todo I still don't really like how much busywork has to be done
            by each layer's advanceAnimations() implementation to call
            animator.update() and then pass some of that to animator advance()
            and some to clean()... */
        for(std::size_t i = 0; i != state.layers.size(); ++i) {
            Layer& layer = state.layers[i];

            /* ... all AbstractDataAnimator instances */
            const Containers::ArrayView<const Containers::Reference<AbstractAnimator>> dataAnimators = Implementation::partitionedAnimatorsDataDataAttachment(state.animatorInstances, dataAttachmentAnimatorOffsets, dataAnimatorOffsets, styleAnimatorOffsets, layerHandle(i, layer.used.generation));
            if(!dataAnimators.isEmpty()) {
                /* If there are any animators partitioned for this layer, it
                   implies that the layer supports data animation */
                CORRADE_INTERNAL_ASSERT(layer.used.features >= LayerFeature::AnimateData);
                state.layers[i].used.instance->advanceAnimations(time,
                    /* Pass the whole arrays, the internals will slice them up
                       as needed before passing to individual animators */
                    active,
                    factors,
                    remove,
                    /* The cast is a bit ew, yeah */
                    Containers::arrayView(reinterpret_cast<Containers::Reference<AbstractDataAnimator>*>(const_cast<Containers::Reference<AbstractAnimator>*>(dataAnimators.data())), dataAnimators.size()));
            }

            /* ... and all AbstractStyleAnimator instances */
            const Containers::ArrayView<const Containers::Reference<AbstractAnimator>> styleAnimators = Implementation::partitionedAnimatorsStyleDataAttachment(state.animatorInstances, dataAttachmentAnimatorOffsets, dataAnimatorOffsets, styleAnimatorOffsets, layerHandle(i, layer.used.generation));
            if(!styleAnimators.isEmpty()) {
                /* If there are any animators partitioned for this layer, it
                   implies that the layer supports style animation */
                CORRADE_INTERNAL_ASSERT(layer.used.features >= LayerFeature::AnimateStyles);
                state.layers[i].used.instance->advanceAnimations(time,
                    /* Pass the whole arrays, the internals will slice them up
                       as needed before passing to individual animators */
                    active,
                    factors,
                    remove,
                    /* The cast is a bit ew, yeah */
                    Containers::arrayView(reinterpret_cast<Containers::Reference<AbstractStyleAnimator>*>(const_cast<Containers::Reference<AbstractAnimator>*>(styleAnimators.data())), styleAnimators.size()));
            }
        }
    }

    /* Update current time. This is done even if no advance() was called. */
    state.animationTime = time;

    /* As the NeedsAnimatorAdvance state is implicitly propagated from the
       animators, this function doesn't need to perform any additional state
       logic. */

    return *this;
}

AbstractUserInterface& AbstractUserInterface::update() {
    /* Call clean implicitly in order to make the internal state ready for
       update. Is a no-op if there's nothing to clean. */
    clean();

    /* Get the state after the clean call including what bubbles from layers.
       If there's nothing to update, bail. No other states should be left after
       that -- NeedsAnimationAdvance is only propagated from the animators in
       state(), never present directly in state.state. */
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
        "Ui::AbstractUserInterface::update(): user interface size wasn't set", *this);

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
    if(states >= UserInterfaceState::NeedsDataAttachmentUpdate ||
       /* Trigger this branch also if NeedsDataUpdate is set but size of
          `state.dataToUpdateLayerOffsets` isn't in sync with `state.layers`
          size, which happens for example if setNeedsUpdate() is called on a
          layer but there's nothing attached to any node in the UI at all. The
          same condition is below, and it depends on dataCount being correctly
          calculated here in order to size visibleNodeDataIds, which then gets
          sliced in the NeedsDataUpdate branch below. */
       /** @todo FFS this is rather horrible, exhibit 1 of 2 */
       (states >= UserInterfaceState::NeedsDataUpdate && state.layers.size() + 1 != state.dataToUpdateLayerOffsets.size()))
    {
        for(Layer& layer: state.layers)
            if(AbstractLayer* const instance = layer.used.instance.get())
                dataCount += instance->capacity();
    }

    /* Single allocation for all temporary data */
    /** @todo well, not really, there's one more temp array for layout mask
        calculation */
    Containers::MutableBitArrayView visibleNodes;
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
    /* Contains a copy of state.visibleEventNodeMask (allocated below) together
       with additional bits set for nodes that need visibility lost events
       emitted. The bits used for visibility lost events are gradually cleared
       to avoid calling the same event multiple times, so this mask isn't
       usable for anything else afterwards. */
    Containers::MutableBitArrayView visibleOrVisibilityLostEventNodeMask;
    Containers::ArrayTuple storage{
        {ValueInit, state.nodes.size(), visibleNodes},
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
        /* One more item for the stack root, which is the whole UI size */
        {NoInit, state.nodes.size() + 1, clipStack},
        {NoInit, dataCount, visibleNodeDataIds},
        {NoInit, state.nodes.size(), visibleOrVisibilityLostEventNodeMask},
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
            {NoInit, state.nodes.size(), state.absoluteNodeOpacities},
            {NoInit, state.nodes.size(), state.visibleNodeMask},
            {NoInit, state.nodes.size(), state.visibleEventNodeMask},
            {NoInit, state.nodes.size(), state.visibleEnabledNodeMask},
            {NoInit, state.nodes.size(), state.clipRectOffsets},
            {NoInit, state.nodes.size(), state.clipRectSizes},
            {NoInit, state.nodes.size(), state.clipRectNodeCounts},
        };

        /* 1. Order the visible node hierarchy. */
        {
            const std::size_t visibleCount = Implementation::orderVisibleNodesDepthFirstInto(
                stridedArrayView(state.nodes).slice(&Node::used).slice(&Node::Used::parent),
                stridedArrayView(state.nodes).slice(&Node::used).slice(&Node::Used::order),
                stridedArrayView(state.nodes).slice(&Node::used).slice(&Node::Used::flags),
                stridedArrayView(state.nodeOrder).slice(&NodeOrder::used).slice(&NodeOrder::Used::next),
                state.firstNodeOrder, visibleNodes, childrenOffsets, children,
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
                           so can be arbitrary (but not 0, as that'd make
                           layoutHandleId() assert). This here also overwrites
                           multiple layouts set for the same node. */
                        nodeLayouts[{nodeHandleId(node), layouterIndex}] = layoutHandle(layouter, i, 0xfff);
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
            stridedArrayView(state.nodes).slice(&Node::used).slice(&Node::Used::parent),
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
                stridedArrayView(state.nodes).slice(&Node::used).slice(&Node::Used::parent),
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
                instance->update(
                    /** @todo yeah and this allocation REALLY isn't great */
                    Containers::BitArray{ValueInit, instance->capacity()},
                    {},
                    stridedArrayView(state.nodes).slice(&Node::used).slice(&Node::Used::parent),
                    state.nodeOffsets, state.nodeSizes);
            }
        }

        /* 8. Calculate absolute offsets for visible nodes. */
        for(const UnsignedInt id: state.visibleNodeIds) {
            const Node& node = state.nodes[id];
            const Vector2 nodeOffset = state.nodeOffsets[id];
            state.absoluteNodeOffsets[id] =
                node.used.parent == NodeHandle::Null ? nodeOffset :
                    state.absoluteNodeOffsets[nodeHandleId(node.used.parent)] + nodeOffset;
        }
    }

    /* If no opacity update is needed, the `state.absoluteNodeOpacities` are
       all up-to-date */
    if(states >= UserInterfaceState::NeedsNodeOpacityUpdate) {
        for(const UnsignedInt id: state.visibleNodeIds) {
            const Node& node = state.nodes[id];
            const Float nodeOpacity = node.used.opacity;
            state.absoluteNodeOpacities[id] =
                node.used.parent == NodeHandle::Null ? nodeOpacity :
                    state.absoluteNodeOpacities[nodeHandleId(node.used.parent)]*nodeOpacity;
        }
    }

    /* If no clip update is needed, the `state.visibleNodeMask` is all
       up-to-date */
    if(states >= UserInterfaceState::NeedsNodeClipUpdate) {
        /* 9. Cull / clip the visible nodes based on their clip rects and the
           offset + size of the whole UI (window / screen area) */
        state.clipRectCount = Implementation::cullVisibleNodesInto(
            /** @todo might be useful to make the offset configurable as well,
                for example when the UI is always occupying just a part of the
                window to get the nodes outside of that rectangle culled as
                well */
            {}, state.size,
            state.absoluteNodeOffsets,
            state.nodeSizes,
            stridedArrayView(state.nodes).slice(&Node::used).slice(&Node::Used::flags),
            clipStack.prefix(state.visibleNodeIds.size() + 1),
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

    /* If no node enabled state update is needed, the `state.visibleNodeMask`
       and `state.visibleEnabledNodeMask` are up-to-date.

       Note that `visibleOrVisibilityLostEventNodeMask` is *not* up-to-date as
       it got allocated anew in each update() allocation. It's used only in the
       `NeedsDataAttachmentUpdate` branch below, so it's also filled there. */
    if(states >= UserInterfaceState::NeedsNodeEnabledUpdate) {
        /** @todo copy() for a BitArrayView, finally */
        CORRADE_INTERNAL_ASSERT(
            state.visibleNodeMask.offset() == 0 &&
            state.visibleEventNodeMask.offset() == 0 &&
            state.visibleEnabledNodeMask.offset() == 0);
        const std::size_t sizeWholeBytes = (state.visibleNodeMask.size() + 7)/8;
        Utility::copy(
            Containers::arrayView(state.visibleNodeMask.data(), sizeWholeBytes),
            Containers::arrayView(state.visibleEventNodeMask.data(), sizeWholeBytes));
        Utility::copy(
            Containers::arrayView(state.visibleNodeMask.data(), sizeWholeBytes),
            Containers::arrayView(state.visibleEnabledNodeMask.data(), sizeWholeBytes));
        Implementation::propagateNodeFlagToChildrenInto<NodeFlag::NoEvents>(
            stridedArrayView(state.nodes).slice(&Node::used).slice(&Node::Used::flags),
            state.visibleNodeIds,
            state.visibleNodeChildrenCounts,
            state.visibleEventNodeMask);
        Implementation::propagateNodeFlagToChildrenInto<NodeFlag::Disabled>(
            stridedArrayView(state.nodes).slice(&Node::used).slice(&Node::Used::flags),
            state.visibleNodeIds,
            state.visibleNodeChildrenCounts,
            state.visibleEnabledNodeMask);
    }

    /* If no data attachment update is needed, the data in
       `state.dataStateStorage` and all views pointing to it is already
       up-to-date. */
    if(states >= UserInterfaceState::NeedsDataAttachmentUpdate ||
       /* Trigger this branch also if NeedsDataUpdate is set but size of
          `state.dataToUpdateLayerOffsets` isn't in sync with `state.layers`
          size, which happens for example if setNeedsUpdate() is called on a
          layer but there's nothing attached to any node in the UI at all */
       /** @todo FFS this is rather horrible, exhibit 2 of 2 */
       (states >= UserInterfaceState::NeedsDataUpdate && state.layers.size() + 1 != state.dataToUpdateLayerOffsets.size()))
    {
        /* Make visibleOrVisibilityLostEventNodeMask a copy of
           visibleEventNodeMask with additional bits set for state.current*Node
           that are valid but possibly now hidden or not taking events. This
           mask will get used to ensure data IDs are collected for those in
           case visibilityLostEvent() needs to be called below.

           Cannot be done in the UserInterfaceState::NeedsNodeEnabledUpdate
           branch above because the visibleOrVisibilityLostEventNodeMask is
           allocated anew every update() call, so with just
           NeedsDataAttachmentUpdate set it'd be left at random garbage. */
        /** @todo copy() for a BitArrayView, finally */
        CORRADE_INTERNAL_ASSERT(visibleOrVisibilityLostEventNodeMask.offset() == 0);
        {
            const std::size_t sizeWholeBytes = (state.visibleNodeMask.size() + 7)/8;
            Utility::copy(
                Containers::arrayView(state.visibleEventNodeMask.data(), sizeWholeBytes),
                Containers::arrayView(visibleOrVisibilityLostEventNodeMask.data(), sizeWholeBytes));
        }
        for(const NodeHandle node: {state.currentPressedNode,
                                    state.currentCapturedNode,
                                    state.currentHoveredNode,
                                    state.currentFocusedNode})
            if(isHandleValid(node))
                visibleOrVisibilityLostEventNodeMask.set(nodeHandleId(node));

        /* Calculate count of visible top-level nodes and layers that draw in
           order to accurately size the array with draws */
        UnsignedInt visibleTopLevelNodeCount = 0;
        for(UnsignedInt visibleTopLevelNodeIndex = 0; visibleTopLevelNodeIndex != state.visibleNodeChildrenCounts.size(); visibleTopLevelNodeIndex += state.visibleNodeChildrenCounts[visibleTopLevelNodeIndex] + 1) {
            if(!(state.nodes[state.visibleNodeIds[visibleTopLevelNodeIndex]].used.flags & NodeFlag::Hidden))
                ++visibleTopLevelNodeCount;
        }
        UnsignedInt drawLayerCount = 0;
        std::size_t compositingDataCount = 0;
        for(const Layer& layer: state.layers) {
            /* This assumes that freed layers (or recycled layers without any
               instance set yet) have the features cleared to an empty set
               in removeLayer(). Otherwise it'd have to check for presence of
               an instance as well. */
            if(layer.used.features & LayerFeature::Draw)
                ++drawLayerCount;
            if(layer.used.features & LayerFeature::Composite)
                compositingDataCount += layer.used.instance->capacity();
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
            {NoInit, compositingDataCount, state.dataToUpdateCompositeRectOffsets},
            {NoInit, compositingDataCount, state.dataToUpdateCompositeRectSizes},
            {NoInit, visibleTopLevelNodeCount*drawLayerCount, state.dataToDrawLayerIds},
            {NoInit, visibleTopLevelNodeCount*drawLayerCount, state.dataToDrawOffsets},
            {NoInit, visibleTopLevelNodeCount*drawLayerCount, state.dataToDrawSizes},
            {NoInit, visibleTopLevelNodeCount*drawLayerCount, state.dataToDrawClipRectOffsets},
            {NoInit, visibleTopLevelNodeCount*drawLayerCount, state.dataToDrawClipRectSizes},
            /* Running data offset (+1) for each item */
            {ValueInit, state.nodes.size() + 1, state.visibleNodeEventDataOffsets},
            {NoInit, dataCount, state.visibleNodeEventData},
        };

        state.dataToUpdateLayerOffsets[0] = {0, 0, 0};
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
            UnsignedInt compositeRectOffset = 0;
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
                    const UnsignedInt nextOffset = out.first();
                    const UnsignedInt nextClipRectOffset = out.second();

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

                    /* If the layer has LayerFeature::Event, count the data for
                       it, accumulating them across all event layers. The
                       visibleOrVisibilityLostEventNodeMask is used instead of
                       state.visibleEventNodeMask to make sure data get
                       collected also for nodes that may no longer participate
                       in event handling but still need visibilityLostEvent()
                       called. */
                    if(layerItem.used.features >= LayerFeature::Event)
                        Implementation::countNodeDataForEventHandlingInto(
                            instance->nodes(),
                            state.visibleNodeEventDataOffsets,
                            visibleOrVisibilityLostEventNodeMask);

                    /* If the layer has LayerFeature::Composite, calculate
                       rects for compositing */
                    if(layerItem.used.features >= LayerFeature::Composite) {
                        Implementation::compositeRectsInto(
                            /** @todo might be useful to make the offset
                                configurable as well, likewise in the
                                cullVisibleNodesInto() call above */
                            {}, state.size,
                            state.dataToUpdateIds.slice(offset, nextOffset),
                            state.dataToUpdateClipRectIds.slice(clipRectOffset, nextClipRectOffset),
                            state.dataToUpdateClipRectDataCounts.slice(clipRectOffset, nextClipRectOffset),
                            instance->nodes(),
                            state.absoluteNodeOffsets,
                            state.nodeSizes,
                            state.clipRectOffsets.prefix(state.clipRectCount),
                            state.clipRectSizes.prefix(state.clipRectCount),
                            state.dataToUpdateCompositeRectOffsets.sliceSize(compositeRectOffset, nextOffset - offset),
                            state.dataToUpdateCompositeRectSizes.sliceSize(compositeRectOffset, nextOffset - offset));
                        compositeRectOffset += nextOffset - offset;
                    }

                    offset = nextOffset;
                    clipRectOffset = nextClipRectOffset;
                }

                state.dataToUpdateLayerOffsets[i + 1] = {offset, clipRectOffset, compositeRectOffset};
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

                /* This assumes that freed layers (or recycled layers without
                   any instance set yet) have the features cleared to an empty
                   set in removeLayer(). Otherwise it'd have to check for
                   presence of an instance as well. */
                if(layerItem.used.features & LayerFeature::Event) {
                    Implementation::orderNodeDataForEventHandlingInto(
                        layer,
                        /* If the Layer::features is non-empty, it means the
                           instance is present (from which it was taken). No
                           need to explicitly check that as well. */
                        layerItem.used.instance->nodes(),
                        state.visibleNodeEventDataOffsets,
                        /* Again the visibleOrVisibilityLostEventNodeMask is
                           used instead of state.visibleEventNodeMask to make
                           sure data get collected also for nodes that may no
                           longer participate in event handling but still need
                           visibilityLostEvent() called. */
                        visibleOrVisibilityLostEventNodeMask,
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

    /* 14. Refresh the event handling state based on visible nodes. Because
       this may call visibilityLostEvent() on layer data, do it before calling
       layer update() so any changes from the events can be directly reflected
       in the update. */
    /** @todo may want to restrict what the event handler can do, like, it
        definitely shouldn't attempt to remove anything */
    if(states >= UserInterfaceState::NeedsNodeEnabledUpdate) {
        /* If the pressed / captured / hovered node is no longer valid, is now
           invisible or doesn't react to events, call visibilityLostEvent() on
           it and reset it */
        for(NodeHandle& node: {Containers::reference(state.currentPressedNode),
                               Containers::reference(state.currentCapturedNode),
                               Containers::reference(state.currentHoveredNode)}) {
            const bool valid = isHandleValid(node);
            /* nodeHandleId() on NodeHandle::Null would assert, don't */
            const UnsignedInt nodeId = valid ? nodeHandleId(node) : ~UnsignedInt{};
            if(valid && state.visibleEventNodeMask[nodeId])
                continue;

            /* Call visibilityLostEvent() only if it wasn't called for this
               node yet -- initially the `visibleOrVisibilityLostEventNodeMask`
               has the bits set for all valid `state.current*Event` nodes but
               after each `visibilityLostEvent()` call we reset the
               corresponding bit to not have it called multiple times if the
               same node was pressed, hovered, captured and focused at the same
               time, e.g.. */
            if(valid && visibleOrVisibilityLostEventNodeMask[nodeId]) {
                VisibilityLostEvent event;
                /* isNodePressed() / isNodeHovered() can never be true in this
                   case */
                callVisibilityLostEventOnNode(node, event, false);
                visibleOrVisibilityLostEventNodeMask.reset(nodeId);
            }

            node = NodeHandle::Null;
        }

        /* If the focused node is no longer valid, is now invisible, doesn't
           react to events or is no longer Focusable, call
           visibilityLostEvent() on it and reset it. Compared to above, the
           only difference is the extra check for the Focusable flag. */
        {
            const bool valid = isHandleValid(state.currentFocusedNode);
            /* nodeHandleId() on NodeHandle::Null would assert, don't */
            const UnsignedInt nodeId = valid ? nodeHandleId(state.currentFocusedNode) : ~UnsignedInt{};
            if(!valid ||
               !state.visibleEventNodeMask[nodeId] ||
               !(state.nodes[nodeId].used.flags >= NodeFlag::Focusable))
            {
                /* Again, call visibilityLostEvent() only if it wasn't called
                   for this node yet in any of the iterations above */
                if(valid && visibleOrVisibilityLostEventNodeMask[nodeId]) {
                    VisibilityLostEvent event;
                    /* isNodePressed() / isNodeHovered() can be true in this
                       case */
                    callVisibilityLostEventOnNode(state.currentFocusedNode, event, true);
                    visibleOrVisibilityLostEventNodeMask.reset(nodeId);
                }

                state.currentFocusedNode = NodeHandle::Null;
            }
        }
    }

    /* As this mask might have gotten some bits reset above, it's not really
       reliably useful for anything after this point. Reset it to be sure it
       doesn't get used. */
    visibleOrVisibilityLostEventNodeMask = {};

    /* 15. Decide what all to update on all layers */
    LayerStates allLayerStateToUpdate;
    LayerStates allCompositeLayerStateToUpdate;
    /** @todo might be worth to have a dedicated state bit for just the
        order, not sure if it's feasible to have a bit for just node offsets
        and sizes and not the visible mask due to the visibility depending on
        them */
    if(states >= UserInterfaceState::NeedsLayoutUpdate) {
        /* NeedsNodeOrderUpdate is implied by this as well, as this is a
           superset of NeedsNodeClipUpdate */
        CORRADE_INTERNAL_ASSERT(states >= UserInterfaceState::NeedsNodeClipUpdate);
        allLayerStateToUpdate |= LayerState::NeedsNodeOffsetSizeUpdate;
        allCompositeLayerStateToUpdate |= LayerState::NeedsCompositeOffsetSizeUpdate;
    }
    if(states >= UserInterfaceState::NeedsNodeClipUpdate)
        allLayerStateToUpdate |= LayerState::NeedsNodeOrderUpdate;
    if(states >= UserInterfaceState::NeedsNodeEnabledUpdate)
        allLayerStateToUpdate |= LayerState::NeedsNodeEnabledUpdate;
    if(states >= UserInterfaceState::NeedsNodeOpacityUpdate)
        allLayerStateToUpdate |= LayerState::NeedsNodeOpacityUpdate;
    /** @todo state.state doesn't contain anything from the layers, what
        difference would that make? */
    if(states >= UserInterfaceState::NeedsDataAttachmentUpdate)
        /* The implementation doesn't need to get NeedsAttachmentUpdate for
           anything as it's meant to be used by the layer to signalize a need
           to update , supply just the subset it should care about */
        allLayerStateToUpdate |= LayerState::NeedsNodeOrderUpdate;

    /* 16. For each layer (if there are actually any) submit an update of
       visible data across all visible top-level nodes. If no data update is
       needed, the data in layers is already up-to-date. */
    if(states >= UserInterfaceState::NeedsDataUpdate && state.firstLayer != LayerHandle::Null) {
        /* Make the update calls follow layer order so the implementations can
           rely on a consistent order of operations compared to going through
           whatever was the order they were created in */
        LayerHandle layer = state.firstLayer;
        do {
            const UnsignedInt layerId = layerHandleId(layer);
            Layer& layerItem = state.layers[layerId];

            /* Decide what all to update on this layer. If nothing is in the
               global enum and nothing here either, skip it. Note that it
               should never happen that we iterate through all layers here and
               skip all because in that case the `states` wouldn't contain
               NeedsDataUpdate and it wouldn't even get here. */
            AbstractLayer* const instance = layerItem.used.instance.get();
            LayerStates layerStateToUpdate = allLayerStateToUpdate;
            if(instance) {
                layerStateToUpdate |= instance->state();
                if(layerItem.used.features >= LayerFeature::Composite)
                    layerStateToUpdate |= allCompositeLayerStateToUpdate;
            }

            /* If the layer has an instance (as layers may have been created
               but without instances set yet) and there's something to update,
               call update() on it */
            /** @todo include a bitmask of what data actually changed */
            if(instance && layerStateToUpdate) instance->update(
                layerStateToUpdate,
                state.dataToUpdateIds.slice(
                    state.dataToUpdateLayerOffsets[layerId].first(),
                    state.dataToUpdateLayerOffsets[layerId + 1].first()),
                state.dataToUpdateClipRectIds.slice(
                    state.dataToUpdateLayerOffsets[layerId].second(),
                    state.dataToUpdateLayerOffsets[layerId + 1].second()),
                state.dataToUpdateClipRectDataCounts.slice(
                    state.dataToUpdateLayerOffsets[layerId].second(),
                    state.dataToUpdateLayerOffsets[layerId + 1].second()),
                /** @todo some layer implementations may eventually want
                    relative offsets, not absolute, provide both? */
                /** @todo once the changed mask is there, might be useful to
                    have (opt-in?) offsets relative to the clip rect -- it can
                    be a different draw anyway, so it might be easier to just
                    add an extra transform at draw time without triggering a
                    full data update for all offsets; what changes is the
                    culling tho, which still needs at least the index buffer
                    update, not everything */
                state.absoluteNodeOffsets,
                state.nodeSizes,
                state.absoluteNodeOpacities,
                state.visibleEnabledNodeMask,
                state.clipRectOffsets.prefix(state.clipRectCount),
                state.clipRectSizes.prefix(state.clipRectCount),
                state.dataToUpdateCompositeRectOffsets.slice(
                    state.dataToUpdateLayerOffsets[layerId].third(),
                    state.dataToUpdateLayerOffsets[layerId + 1].third()),
                state.dataToUpdateCompositeRectSizes.slice(
                    state.dataToUpdateLayerOffsets[layerId].third(),
                    state.dataToUpdateLayerOffsets[layerId + 1].third()));

            layer = layerItem.used.next;
        } while(layer != state.firstLayer);
    }

    /** @todo layer-specific cull/clip step? */

    /* Unmark the UI as needing an update() call. No other states should be
       left after that, i.e. the UI should be ready for drawing and event
       processing. Not even NeedsRendererSizeSetup should be left, as that was
       cleared by updateRenderer() that was called above; NeedsAnimationAdvance
       is only propagated from the animators in state(), never present directly
       in state.state. */
    state.state &= ~UserInterfaceState::NeedsNodeUpdate;
    CORRADE_INTERNAL_ASSERT(!state.state);
    return *this;
}

AbstractUserInterface& AbstractUserInterface::draw() {
    State& state = *_state;
    CORRADE_ASSERT(state.renderer,
        "Ui::AbstractUserInterface::draw(): no renderer instance set", *this);

    /* Call update implicitly in order to make the internal state ready for
       drawing. Is a no-op if there's nothing to update or clean. */
    update();

    /* Transition the renderer to the initial state if it was in Final. If it's
       already there, this is a no-op. */
    AbstractRenderer& renderer = *state.renderer;
    renderer.transition(RendererTargetState::Initial, {});

    /* Then submit draws in the correct back-to-front order, i.e. for every
       top-level node and then for every layer used by its children */
    for(std::size_t i = 0; i != state.drawCount; ++i) {
        const UnsignedInt layerId = state.dataToDrawLayerIds[i];
        const LayerFeatures features = state.layers[layerId].used.features;
        AbstractLayer& instance = *state.layers[layerId].used.instance;

        /* Transition to composite and composite, if the layer advertises it */
        /** @todo have Composite independent of the Draw? for example a color /
            screenshot picker might want to have just doComposite() and some
            event handling implemented, but not drawing ... would require the
            draw call collection to be changed to consider Composite alone as
            well or something */
        if(features >= LayerFeature::Composite) {
            renderer.transition(RendererTargetState::Composite, {});

            instance.composite(renderer,
                /* The views should be exactly the same as passed to update()
                   before ... */
                state.dataToUpdateCompositeRectOffsets.slice(
                    state.dataToUpdateLayerOffsets[layerId].third(),
                    state.dataToUpdateLayerOffsets[layerId + 1].third()),
                state.dataToUpdateCompositeRectSizes.slice(
                    state.dataToUpdateLayerOffsets[layerId].third(),
                    state.dataToUpdateLayerOffsets[layerId + 1].third()),
                /* ... and the offset then being relative to those */
                state.dataToDrawOffsets[i] - state.dataToUpdateLayerOffsets[layerId].first(),
                state.dataToDrawSizes[i]);
        }

        /* Transition between draw states. If they're the same, it's a no-op in
           the renderer. */
        RendererDrawStates rendererDrawStates;
        if(features >= LayerFeature::DrawUsesBlending)
            rendererDrawStates |= RendererDrawState::Blending;
        if(features >= LayerFeature::DrawUsesScissor)
            rendererDrawStates |= RendererDrawState::Scissor;
        renderer.transition(RendererTargetState::Draw, rendererDrawStates);

        instance.draw(
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
            state.absoluteNodeOpacities,
            state.visibleEnabledNodeMask,
            state.clipRectOffsets.prefix(state.clipRectCount),
            state.clipRectSizes.prefix(state.clipRectCount));
    }

    /* Transition the renderer to the final state. If no layers were drawn,
       it goes just from Initial to Final. */
    renderer.transition(RendererTargetState::Final, {});
    return *this;
}

/* Used only in update() but put here to have the loops and other event-related
   handling of all call*Event*() APIs together */
void AbstractUserInterface::callVisibilityLostEventOnNode(const NodeHandle node, VisibilityLostEvent& event, const bool canBePressedOrHovering) {
    State& state = *_state;
    /* Set isNodePressed() / isNodeHovered() if the event is called on node
       that is pressed / hovered and it's allowed, which is only in case a
       focused node is no longer focusable, in all other cases where it's not
       visible, disabled or doesn't receive events it isn't allowed.  */
    event._nodePressed = canBePressedOrHovering && node == state.currentPressedNode;
    event._nodeHovered = canBePressedOrHovering && node == state.currentHoveredNode;

    /* Note that unlike callEvent() below, here it *does not* check the
       `state.visibleEventNodeMask` for the `nodeId` because we may actually
       want to call visibilityLostEvent() on nodes that no longer accept
       events. */
    const UnsignedInt nodeId = nodeHandleId(node);
    for(UnsignedInt j = state.visibleNodeEventDataOffsets[nodeId], jMax = state.visibleNodeEventDataOffsets[nodeId + 1]; j != jMax; ++j) {
        const DataHandle data = state.visibleNodeEventData[j];
        state.layers[dataHandleLayerId(data)].used.instance->visibilityLostEvent(dataHandleId(data), event);
    }
}

template<void(AbstractLayer::*function)(UnsignedInt, FocusEvent&)> bool AbstractUserInterface::callFocusEventOnNode(const NodeHandle node, FocusEvent& event) {
    /* Set isNodePressed() / isNodeHovered() if the event is called on node
       that is pressed / hovered. Unlike callEventOnNode() below, this is set
       unconditionally as these events don't have any associated position. */
    State& state = *_state;
    event._nodePressed = node == state.currentPressedNode;
    event._nodeHovered = node == state.currentHoveredNode;

    const UnsignedInt nodeId = nodeHandleId(node);
    bool acceptedByAnyData = false;
    for(UnsignedInt j = state.visibleNodeEventDataOffsets[nodeId], jMax = state.visibleNodeEventDataOffsets[nodeId + 1]; j != jMax; ++j) {
        const DataHandle data = state.visibleNodeEventData[j];
        event._accepted = false;
        ((*state.layers[dataHandleLayerId(data)].used.instance).*function)(dataHandleId(data), event);

        if(event._accepted)
            acceptedByAnyData = true;
    }

    return acceptedByAnyData;
}

/* Used only in keyPressOrReleaseEvent() but put here to have the loops and
   other event-related handling of all call*Event*() APIs together */
template<void(AbstractLayer::*function)(UnsignedInt, KeyEvent&)> bool AbstractUserInterface::callKeyEventOnNode(const NodeHandle node, KeyEvent& event) {
    /* Set isNodePressed() / isNodeHovered() / isNodeFocused() if the event is
       called on node that is pressed / hovered / focused. Unlike
       callEventOnNode() below, this is set unconditionally for all three as
       these events don't have any associated position. */
    State& state = *_state;
    event._nodePressed = node == state.currentPressedNode;
    event._nodeHovered = node == state.currentHoveredNode;
    event._nodeFocused = node == state.currentFocusedNode;

    const UnsignedInt nodeId = nodeHandleId(node);
    bool acceptedByAnyData = false;
    for(UnsignedInt j = state.visibleNodeEventDataOffsets[nodeId], jMax = state.visibleNodeEventDataOffsets[nodeId + 1]; j != jMax; ++j) {
        const DataHandle data = state.visibleNodeEventData[j];
        event._accepted = false;
        ((*state.layers[dataHandleLayerId(data)].used.instance).*function)(dataHandleId(data), event);
        if(event._accepted)
            acceptedByAnyData = true;

        /* So far this function is only used for KeyEvent called on a focused
           node. The KeyEvent doesn't have any way to change the capture, and
           on a focused node the isCaptured() is always false */
        CORRADE_INTERNAL_ASSERT(!event._captured);
    }

    return acceptedByAnyData;
}

/* Used only in textInputEvent() but put here to have the loops and other
   event-related handling of all call*Event*() APIs together */
bool AbstractUserInterface::callTextInputEventOnNode(const NodeHandle node, TextInputEvent& event) {
    State& state = *_state;

    const UnsignedInt nodeId = nodeHandleId(node);
    bool acceptedByAnyData = false;
    for(UnsignedInt j = state.visibleNodeEventDataOffsets[nodeId], jMax = state.visibleNodeEventDataOffsets[nodeId + 1]; j != jMax; ++j) {
        const DataHandle data = state.visibleNodeEventData[j];
        event._accepted = false;
        state.layers[dataHandleLayerId(data)].used.instance->textInputEvent(dataHandleId(data), event);

        if(event._accepted)
            acceptedByAnyData = true;
    }

    return acceptedByAnyData;
}

/* If this is called for fallthrough events, `targetNode` is the node on which
   the original event was accepted (to mark the pressed / hovered / captured
   bits appropriately), and `node` is the fallthrough node. In all other cases
   they're the same. */
template<class Event, void(AbstractLayer::*function)(UnsignedInt, Event&)> bool AbstractUserInterface::callEventOnNode(const Vector2& globalPositionScaled, const NodeHandle node, const NodeHandle targetNode, Event& event, const bool rememberCaptureOnUnaccepted) {
    State& state = *_state;

    /* Set isNodeHovered() to false if the event is called on node that
       actually isn't hovered. The caller itself may also set it to false if it
       is called on a hovered node but the event is outside of its area (such
       as a move outside of the captured node), so we can't set it
       unconditionally. */
    const bool nodeHovered = event._nodeHovered;
    if(targetNode != state.currentHoveredNode)
        event._nodeHovered = false;

    /* On the other hand, isNodePressed() / isNodeFocused() is set
       unconditionally -- the event handler can then check if it was called on
       a pressed / focused node but outside of it by looking at
       isNodeHovered(), no need to encode that information redundantly in
       multiple properties. */
    event._nodePressed = targetNode == state.currentPressedNode;
    /* While hovered and pressed attributes get inherited to fallthrough events
       (since if a child is pressed / hovered, the parent transitively is also)
       it doesn't hold for focus. Thus the comparison is done only against the
       node the event is currently called on, not on the node the original of
       the fallthrough event was called on. */
    event._nodeFocused = node == state.currentFocusedNode;

    const UnsignedInt nodeId = nodeHandleId(node);
    bool acceptedByAnyData = false;
    for(UnsignedInt j = state.visibleNodeEventDataOffsets[nodeId], jMax = state.visibleNodeEventDataOffsets[nodeId + 1]; j != jMax; ++j) {
        const DataHandle data = state.visibleNodeEventData[j];
        /* Remember the previous event capture state to reset it after each
           non-accepted event handler call. Has to be done here in the inner
           loop and not outside so the capture state changes aren't lost when
           an accepted event is followed by a non-accepted one. */
        const bool captured = event._captured;
        event._position = globalPositionScaled - state.absoluteNodeOffsets[nodeId];
        event._nodeSize = state.nodeSizes[nodeId];
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

    /* Reset isNodeHovered() back to the initial state in case the event will
       be re-called on different nodes (where it could actually be true) again
       after */
    event._nodeHovered = nodeHovered;

    return acceptedByAnyData;
}

template<class Event, void(AbstractLayer::*function)(UnsignedInt, Event&)> NodeHandle AbstractUserInterface::callEvent(const Vector2& globalPositionScaled, const UnsignedInt visibleNodeIndex, Event& event) {
    /* The accept state should be initially false as we exit once it becomes
       true. */
    CORRADE_INTERNAL_ASSERT(!event._accepted);
    State& state = *_state;

    /* If the node isn't in the set of visible nodes accepting events (so for
       example has NodeFlag::NoEvents or Disabled set), do nothing. If we
       wouldn't return early, it wouldn't call anything anyway because the
       `state.visibleNodeEventDataOffsets` ranges for these is empty but why do
       all that extra work in the first place. */
    const UnsignedInt nodeId = state.visibleNodeIds[visibleNodeIndex];
    if(!state.visibleEventNodeMask[nodeId])
        return {};

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
    const NodeHandle node = nodeHandle(nodeId, state.nodes[nodeId].used.generation);
    if(callEventOnNode<Event, function>(globalPositionScaled, node, event))
        return node;

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

template<class Event, void(AbstractLayer::*function)(UnsignedInt, Event&)> MAGNUM_UI_LOCAL void AbstractUserInterface::callFallthroughPointerEvents(/*mutable*/ NodeHandle targetNode, const Vector2& globalPositionScaled, Event& event, const bool allowCapture) {
    State& state = *_state;

    /* Mark the event as a fallthrough one from now on. The assumption is that
       it won't get used for a non-fallthrough event anymore. */
    event._fallthrough = true;

    /* Go through parent nodes and call fallthrough events on all nodes that
       want them */
    NodeHandle parent = state.nodes[nodeHandleId(targetNode)].used.parent;
    while(parent != NodeHandle::Null) {
        /* If the event is primary and is accepted, make the fallthrough node
           take over the current pressed / hovered / ... nodes. Secondary
           events don't affect that, so for them nothing is done if they're
           accepted. */
        const UnsignedInt parentId = nodeHandleId(parent);
        if(state.nodes[parentId].used.flags >= NodeFlag::FallthroughPointerEvents && callEventOnNode<Event, function>(globalPositionScaled, parent, targetNode, event)) {
            /* Call a pointer cancel event on previous pressed / hovered /
               focused nodes if the event is primary. Call a pointer cancel on
               the previously captured node always, even for secondary events
               (for which, if accepted, the assumption is that they want to
               change the capture, because there's no other observable effect
               if those get accepted). If the same node is listed in multiple
               current entries, make sure it's called just once. */
            const NodeHandle nodes[]{
                event._primary ? state.currentPressedNode : NodeHandle::Null,
                event._primary ? state.currentHoveredNode : NodeHandle::Null,
                event._primary ? state.currentFocusedNode : NodeHandle::Null,
                state.currentCapturedNode
            };
            for(std::size_t i = 0; i != Containers::arraySize(nodes); ++i) {
                NodeHandle node = nodes[i];
                for(std::size_t j = 0; j != i; ++j) {
                    /* ... only once for each node if the same node appears in
                       multiple of these */
                    if(nodes[j] == node) {
                        node = NodeHandle::Null;
                        break;
                    }
                }
                /* ... and only if given node isn't the (parent) node on which
                   the fallthrough event was called, in which case it stays */
                if(node == NodeHandle::Null || node == parent)
                    continue;

                const UnsignedInt nodeId = nodeHandleId(node);
                for(UnsignedInt j = state.visibleNodeEventDataOffsets[nodeId], jMax = state.visibleNodeEventDataOffsets[nodeId + 1]; j != jMax; ++j) {
                    const DataHandle data = state.visibleNodeEventData[j];
                    PointerCancelEvent cancelEvent{event.time()};
                    state.layers[dataHandleLayerId(data)].used.instance->pointerCancelEvent(dataHandleId(data), cancelEvent);
                }
            }

            /* For a primary event, a cancel event was called above for the
               nodes listed in these. Update them. */
            if(event._primary) {
                /* If the pressed / hovered node was the node the event was
                   originally called on, make the node that accepted the
                   fallback event currently pressed / hovered. Otherwise reset
                   it to null. */
                state.currentPressedNode = state.currentPressedNode == targetNode ?
                    parent : NodeHandle::Null;
                state.currentHoveredNode = state.currentHoveredNode == targetNode ?
                    parent : NodeHandle::Null;

                /* Focused node gets currently unconditionally reset, unless
                   it's the node the fallthrough event was actually called
                   on. */
                if(state.currentFocusedNode != parent)
                    state.currentFocusedNode = NodeHandle::Null;

                /* Since the event was accepted and currently pressed / hovered
                   / ... state was moved to this node, it becomes the target
                   node for all subsequent fallthrough events */
                targetNode = parent;
            }

            /* If the fallback event wants to capture the node and it's allowed
               (i.e., it isn't a primary release event), make it so. If it
               doesn't, reset it. Unlike with press / hover, this is done
               regardless of whether the original node was captured or not. */
            if(event._captured && allowCapture)
                state.currentCapturedNode = parent;
            else
                state.currentCapturedNode = NodeHandle::Null;
        }

        parent = state.nodes[parentId].used.parent;
    }
}

bool AbstractUserInterface::pointerPressEvent(const Vector2& globalPosition, PointerEvent& event) {
    CORRADE_ASSERT(!event._accepted,
        "Ui::AbstractUserInterface::pointerPressEvent(): event already accepted", {});

    State& state = *_state;

    /* This will be invalid if setSize() wasn't called yet, but callEvent() has
       a call to update() inside which will then assert */
    const Vector2 globalPositionScaled = globalPosition*state.size/state.windowSize;

    /* If this is a non-primary event and there's a node capturing pointer
       events, call the event on it directly. Given that update() was called,
       it should be either null or valid. */
    bool pressAcceptedByAnyData;
    NodeHandle calledNode;
    if(!event.isPrimary() && state.currentCapturedNode != NodeHandle::Null) {
        CORRADE_INTERNAL_ASSERT(isHandleValid(state.currentCapturedNode));

        const UnsignedInt capturedNodeId = nodeHandleId(state.currentCapturedNode);
        const Vector2 capturedNodeMin = state.absoluteNodeOffsets[capturedNodeId];
        const Vector2 capturedNodeMax = capturedNodeMin + state.nodeSizes[capturedNodeId];
        const bool insideCapturedNode = (globalPositionScaled >= capturedNodeMin).all() && (globalPositionScaled < capturedNodeMax).all();

        /* Called on a captured node, so isCaptured() should be true,
           isNodeHovered() can be true if it's inside it */
        event._captured = true;
        event._nodeHovered = insideCapturedNode;

        pressAcceptedByAnyData = callEventOnNode<PointerEvent, &AbstractLayer::pointerPressEvent>(globalPositionScaled, state.currentCapturedNode, event);
        calledNode = state.currentCapturedNode;

    /* Otherwise, if this is either a primary event (which changes the capture)
       or there's no capture, do the usual hit testing etc. */
    } else {
        /* A primary press event has isCaptured() set always. Non-primary
           doesn't by default, but the implementation can enable it. */
        event._captured = event.isPrimary();
        /* Can have isNodeHovered() set if it ends up being called on the
           currently hovered node. */
        event._nodeHovered = true;

        calledNode = callEvent<PointerEvent, &AbstractLayer::pointerPressEvent>(globalPositionScaled, event);
        pressAcceptedByAnyData = calledNode != NodeHandle::Null;
    }

    /* If the event was called on any node and capture is desired (or still
       desired, or it's a secondary event which doesn't implicitly capture but
       the capture was requested), remember the concrete node for it (or, if
       still desired, the calledNode is the same as currentCapturedNode and
       thus it stays). Otherwise, if the event was accepted but the capture was
       removed, reset it. Otherwise the event was not accepted at all, in which
       case nothing changes.

       This is done before calling fallthrough events as they should have a
       chance to further adjust the capture, both ways. */
    if(pressAcceptedByAnyData) {
        if(event._captured)
            state.currentCapturedNode = calledNode;
        else
            state.currentCapturedNode = NodeHandle::Null;
    }

    /* If the event is primary, it affects current pressed node and current
       pointer position and focus */
    if(event.isPrimary()) {
        /* Remember the node that accepted the event as pressed */
        state.currentPressedNode = pressAcceptedByAnyData ? calledNode : NodeHandle::Null;

        /* Update the last relative position with this one */
        /** @todo update this before calling any event functions so the event
            handlers can rely on this information being up to date if needed */
        state.currentGlobalPointerPosition = globalPositionScaled;
    }

    /* If the press happened with a "default" pointer and a primary event, deal
       with focus. With other pointer types and secondary events nothing gets
       focused  but also they don't blur anything. */
    if(event.isPrimary() && (event.pointer() & (Pointer::MouseLeft|Pointer::Finger|Pointer::Pen))) {
        /* Call a focus event if the press was accepted and on a node that's
           focusable */
        const NodeHandle nodeToFocus =
            pressAcceptedByAnyData &&
            state.nodes[nodeHandleId(calledNode)].used.flags >= NodeFlag::Focusable &&
            state.visibleEventNodeMask[nodeHandleId(calledNode)] ?
                calledNode : NodeHandle::Null;

        /* If the node to be focused is different from the currently focused
           one, call a blur event on the original, if there's any. */
        if(nodeToFocus != state.currentFocusedNode && state.currentFocusedNode != NodeHandle::Null) {
            FocusEvent blurEvent{event.time()};
            callFocusEventOnNode<&AbstractLayer::blurEvent>(state.currentFocusedNode, blurEvent);
        }

        /* Then emit a focus event if the node is actually focusable; do it
           even if the node is already focused. If it gets accepted, update the
           currently focused node, otherwise set it to null. */
        if(nodeToFocus != NodeHandle::Null) {
            FocusEvent focusEvent{event.time()};
            if(callFocusEventOnNode<&AbstractLayer::focusEvent>(nodeToFocus, focusEvent))
                state.currentFocusedNode = nodeToFocus;
            else {
                /* If the unaccepted focus event happened on an already focused
                   node, call a blur event for it. */
                if(state.currentFocusedNode == nodeToFocus)
                    callFocusEventOnNode<&AbstractLayer::blurEvent>(state.currentFocusedNode, focusEvent);
                state.currentFocusedNode = NodeHandle::Null;
            }
        } else state.currentFocusedNode = NodeHandle::Null;
    }

    /* Fire fallthrough events on all parent nodes that have
       FallthroughPointerEvents set. Do it either if the press was accepted or
       if the event wasn't accepted but there's a captured node (as in that
       case it wouldn't propagate anywhere, making it impossible for the
       fallthrough nodes to catch such events). */
    if(pressAcceptedByAnyData || state.currentCapturedNode != NodeHandle::Null)
        callFallthroughPointerEvents<PointerEvent, &AbstractLayer::pointerPressEvent>(
            calledNode != NodeHandle::Null ? calledNode : state.currentCapturedNode,
            globalPositionScaled, event, /*allowCapture*/ true);

    return pressAcceptedByAnyData;
}

bool AbstractUserInterface::pointerReleaseEvent(const Vector2& globalPosition, PointerEvent& event) {
    CORRADE_ASSERT(!event._accepted,
        "Ui::AbstractUserInterface::pointerReleaseEvent(): event already accepted", {});

    /* Update so we don't have stale pointerEventCapture{Node,Data}. Otherwise
       the update() gets called only later in callEvent(). */
    update();

    State& state = *_state;
    const Vector2 globalPositionScaled = globalPosition*state.size/state.windowSize;

    /* If there's a node capturing pointer events, call the event on it
       directly. Given that update() was called, it should be either null or
       valid. */
    bool releaseAcceptedByAnyData;
    NodeHandle calledNode;
    if(state.currentCapturedNode != NodeHandle::Null) {
        CORRADE_INTERNAL_ASSERT(isHandleValid(state.currentCapturedNode));

        const UnsignedInt capturedNodeId = nodeHandleId(state.currentCapturedNode);
        const Vector2 capturedNodeMin = state.absoluteNodeOffsets[capturedNodeId];
        const Vector2 capturedNodeMax = capturedNodeMin + state.nodeSizes[capturedNodeId];
        const bool insideCapturedNode = (globalPositionScaled >= capturedNodeMin).all() && (globalPositionScaled < capturedNodeMax).all();

        /* Called on a captured node, so isCaptured() should be true,
           isNodeHovered() can be true if it's inside it. As the release event
           always implicitly releases the capture, any potential capture state
           changed by the event handler is ignored. */
        event._captured = true;
        event._nodeHovered = insideCapturedNode;

        releaseAcceptedByAnyData = callEventOnNode<PointerEvent, &AbstractLayer::pointerReleaseEvent>(globalPositionScaled, state.currentCapturedNode, event);
        calledNode = releaseAcceptedByAnyData ? state.currentCapturedNode : NodeHandle::Null;

    /* Otherwise the usual hit testing etc. */
    } else {
        /* Not called on a captured node, isCaptured() should be false and thus
           isNodeHovered() can be true */
        event._captured = false;
        event._nodeHovered = true;

        calledNode = callEvent<PointerEvent, &AbstractLayer::pointerReleaseEvent>(globalPositionScaled, event);
        releaseAcceptedByAnyData = calledNode != NodeHandle::Null;
    }

    /* Update the last relative position with this one if it's a primary
       event */
    /** @todo update this before calling any event functions so the event
        handlers can rely on this information being up to date if needed */
    if(event.isPrimary())
        state.currentGlobalPointerPosition = globalPositionScaled;

    /* After a release coming from a primary event, there should be no captured
       node anymore either. Non-primary fallthrough events that are fired at
       the end may subsequently change the capture again.

       This is done before calling fallthrough events as they should have a
       chance to further adjust the capture, both ways. */
    if(event.isPrimary()) {
        state.currentCapturedNode = NodeHandle::Null;

     /* If the event isn't primary, the event -- if accepted -- is allowed to
        change the capture state both directions, similarly as in the press
        event. */
    } else if(calledNode != NodeHandle::Null) {
        state.currentCapturedNode = event._captured ? calledNode : NodeHandle::Null;
    }

    /* Fire fallthrough events on all parent nodes that have
       FallthroughPointerEvents set. Do it either if the press was accepted or
       if the event wasn't accepted but there's a captured node (as in that
       case it wouldn't propagate anywhere, making it impossible for the
       fallthrough nodes to catch such events). Allow them to change capture
       only if they're secondary release events. */
    if(releaseAcceptedByAnyData || state.currentCapturedNode != NodeHandle::Null)
        callFallthroughPointerEvents<PointerEvent, &AbstractLayer::pointerReleaseEvent>(
            calledNode != NodeHandle::Null ? calledNode : state.currentCapturedNode,
            globalPositionScaled, event, /*allowCapture*/ !event.isPrimary());

    /* After a release that's a primary event, there should be no pressed node
       anymore. Reset only after the fallthrough events are called to have them
       receive isNodePressed() the same as the original events did. */
    if(event.isPrimary())
        state.currentPressedNode = NodeHandle::Null;

    return releaseAcceptedByAnyData;
}

bool AbstractUserInterface::pointerMoveEvent(const Vector2& globalPosition, PointerMoveEvent& event) {
    CORRADE_ASSERT(!event._accepted,
        "Ui::AbstractUserInterface::pointerMoveEvent(): event already accepted", {});

    /* Update so we don't have stale pointerEventCapture{Node,Data}. Otherwise
       the update() gets called only later in callEvent(). */
    update();

    State& state = *_state;
    const Vector2 globalPositionScaled = globalPosition*state.size/state.windowSize;

    /* Fill in position relative to the previous event, if there was any. Since
       the value is event-relative and not node-relative, it doesn't need any
       further updates in the callEvent() code.

       This is currently done just for primary events, as the library would
       need to track the position for an arbitrary number of secondary touches
       otherwise. It's subsequently being reset for enter/leave events, and
       then set back for fallthrough events. */
    const Vector2 relativePosition = event.isPrimary() && state.currentGlobalPointerPosition ?
        globalPositionScaled - *state.currentGlobalPointerPosition : Vector2{};
    event._relativePosition = relativePosition;

    /* If there's a node capturing pointer events, call the event on it
       directly. Given that update() was called, it should be either null or
       valid. */
    bool moveAcceptedByAnyData;
    NodeHandle calledNode;
    bool insideNodeArea;
    if(state.currentCapturedNode != NodeHandle::Null) {
        CORRADE_INTERNAL_ASSERT(isHandleValid(state.currentCapturedNode));

        const UnsignedInt capturedNodeId = nodeHandleId(state.currentCapturedNode);
        const Vector2 capturedNodeMin = state.absoluteNodeOffsets[capturedNodeId];
        const Vector2 capturedNodeMax = capturedNodeMin + state.nodeSizes[capturedNodeId];
        insideNodeArea = (globalPositionScaled >= capturedNodeMin).all() && (globalPositionScaled < capturedNodeMax).all();

        /* Called on a captured node, so isCaptured() should be true,
           isNodeHovered() can be true if it's inside it */
        event._captured = true;
        event._nodeHovered = insideNodeArea;

        /* For a primary event, it should be possible to reset the capture
           independently of whether it's accepted or not (for example if
           outside of some acceptable bounds for a capture). Leave non-primary
           events without such capability for simplicity. */
        /** @todo any use case for a non-primary non-accepted event to reset
            the capture? it should just accept in that case, why not */
        moveAcceptedByAnyData = callEventOnNode<PointerMoveEvent, &AbstractLayer::pointerMoveEvent>(globalPositionScaled, state.currentCapturedNode, event, /*rememberCaptureOnUnaccepted*/ event.isPrimary());
        calledNode = state.currentCapturedNode;

    /* Otherwise the usual hit testing etc. */
    } else {
        /* Which makes the event always called inside node area */
        insideNodeArea = true;

        /* Not called on a captured node, isCaptured() should be false and thus
           isNodeHovered() can be true */
        event._captured = false;
        event._nodeHovered = true;

        calledNode = callEvent<PointerMoveEvent, &AbstractLayer::pointerMoveEvent>(globalPositionScaled, event);
        moveAcceptedByAnyData = calledNode != NodeHandle::Null;
    }

    /* Decide about currently hovered node and whether to call Enter / Leave.
       Non-primary events don't affect hover and thus never cause Enter / Leave
       to be called. */
    NodeHandle callLeaveOnNode = NodeHandle::Null;
    NodeHandle callEnterOnNode = NodeHandle::Null;
    if(event.isPrimary()) {
        /* If the move event was called on a captured node ... */
        if(state.currentCapturedNode != NodeHandle::Null) {
            CORRADE_INTERNAL_ASSERT(calledNode == state.currentCapturedNode);

            /* Call Leave if the captured node was previously hovered and the
               pointer is now outside or was not accepted */
            if(state.currentHoveredNode == calledNode && (!insideNodeArea || !moveAcceptedByAnyData)) {
                callLeaveOnNode = calledNode;
            /* Leave also if some other node was previously hovered */
            } else if(state.currentHoveredNode != NodeHandle::Null &&
                      state.currentHoveredNode != calledNode) {
                CORRADE_INTERNAL_ASSERT(isHandleValid(state.currentHoveredNode));
                callLeaveOnNode = state.currentHoveredNode;
            }

            /* Call Enter if the captured node wasn't previously hovered and
               the pointer is now inside and was accepted. Calls Enter also in
               case some other node was previously hovered. */
            if(state.currentHoveredNode != calledNode && (insideNodeArea && moveAcceptedByAnyData)) {
                callEnterOnNode = calledNode;
            }

            /* The now-hovered node is the captured node if the pointer was
               inside and the event was accepted */
            if(insideNodeArea && moveAcceptedByAnyData) {
                state.currentHoveredNode = calledNode;
            } else {
                state.currentHoveredNode = NodeHandle::Null;
            }

        /* Otherwise, call Enter / Leave event if the move event was called on a
           node that's different from the previously hovered */
        } else if(state.currentHoveredNode != calledNode) {
            /* Leave if the previously hovered node isn't null */
            if(state.currentHoveredNode != NodeHandle::Null) {
                CORRADE_INTERNAL_ASSERT(isHandleValid(state.currentHoveredNode));
                callLeaveOnNode = state.currentHoveredNode;
            }
            /* Enter if the current node isn't null */
            if(calledNode != NodeHandle::Null) {
                callEnterOnNode = calledNode;
            }

            /* The now-hovered node is the one that accepted the move event */
            state.currentHoveredNode = calledNode;
        }
    }

    /* Emit a Leave event if needed. Reusing the same event instance, just
       resetting the accept status, relative position (it has to be zero since
       it's relative to the move that happened right before) and capture, as
       Leave events should not be able to affect it. Both the accept and the
       capture status is subsequently ignored. */
    if(callLeaveOnNode != NodeHandle::Null) {
        /** @todo Given that the enter / leave events don't need to be
            accepted, setting this to false means the function may return true
            but the original event isAccepted() is still false -- do
            `event._accepted = moveAcceptedByAnyData` before returning? Not
            doing so may cause the same event class to be accidentally used
            multiple times, is that a problem? */
        event._accepted = false;

        /* Leave events are by definition never hovering the node they are
           called on */
        event._nodeHovered = false;

        /* Leave events can only change capture status if they're called on the
           actual captured node, otherwise the capture status is false and is
           also reset back to false below */
        const bool captured = event._captured;
        if(state.currentCapturedNode != callLeaveOnNode)
            event._captured = false;
        event._relativePosition = {};
        /* The accept status is ignored for the Enter/Leave events, which means
           we remember the capture state even if not explicitly accepted */
        callEventOnNode<PointerMoveEvent, &AbstractLayer::pointerLeaveEvent>(globalPositionScaled, callLeaveOnNode, event, /*rememberCaptureOnUnaccepted*/ true);

        if(state.currentCapturedNode != callLeaveOnNode)
            event._captured = captured;
    }

    /* Emit Enter event. Again reusing the same event instance, with accept and
       relative position reset. The accept status is subsequently ignored, the
       capture isn't. */
    if(callEnterOnNode != NodeHandle::Null) {
        /** @todo Given that the enter / leave events don't need to be
            accepted, setting this to false means the function may return true
            but the original event isAccepted() is still false -- do
            `event._accepted = moveAcceptedByAnyData` before returning? Not
            doing so may cause the same event class to be accidentally used
            multiple times, is that a problem? */
        event._accepted = false;

        /* Enter events are by definition always hovering the node they are
           called on. As the currentHoveredNode was updated above, the
           callEventOnNode() should thus not reset this back to false. */
        event._nodeHovered = true;

        event._relativePosition = {};
        /* The accept status is ignored for the Enter/Leave events, which means
           we remember the capture state even if not explicitly accepted */
        callEventOnNode<PointerMoveEvent, &AbstractLayer::pointerEnterEvent>(globalPositionScaled, callEnterOnNode, event, /*rememberCaptureOnUnaccepted*/ true);
    }

    /* Update the captured node based on what's desired. If the captured state
       was the same before, this is a no op, i.e. assigning the same value. The
       fallthrough events that are fired at the end may subsequently change the
       capture again.

       This is done before calling fallthrough events as they should have a
       chance to further adjust the capture, both ways. */
    if(event._captured) {
        /* If the captured state was set, the event was either called on a
           captured node (and then either accepted, or not, which caused it to
           stay set), or was accepted on a non-captured node */
        CORRADE_INTERNAL_ASSERT(
            (state.currentCapturedNode != NodeHandle::Null || moveAcceptedByAnyData) &&
            calledNode != NodeHandle::Null);
        state.currentCapturedNode = calledNode;
    } else {
        state.currentCapturedNode = NodeHandle::Null;
    }

    /* Non-primary events don't affect the current pressed node or current
       pointer position state */
    if(event.isPrimary()) {
        /* If pointer capture is not active (either it wasn't at all, or the
           move reset it), currentPressedNode gets reset if the event happened
           on a different node, happened outside of a (previously captured)
           node area or was not accepted by any data (i.e., it's outside of
           node active area). If pointer capture is active, it's not changed in
           any way in order to make it possible for the pointer to return to
           the node area and then perform a release, still causing it to be
           interpreted as a tap or click for example. */
        /** @todo Once a press generates an enter event etc., it should no
            longer happen that there's a different pressed and hovered node;
            after that this will no longer cause the pressed node to be reset
            too soon for a fallthrough event. See corresponding XFAILs in the
            eventPointerFallthrough() test. */
        if(event.isPrimary() && state.currentCapturedNode == NodeHandle::Null && (calledNode != state.currentPressedNode || !insideNodeArea || !moveAcceptedByAnyData))
            state.currentPressedNode = NodeHandle::Null;

        /* Update the last relative position with this one */
        /** @todo update this before calling any event functions so the event
            handlers can rely on this information being up to date if needed */
        state.currentGlobalPointerPosition = globalPositionScaled;
    }

    /* Fire fallthrough events on all parent nodes that have
       FallthroughPointerEvents set. Do it either if the press was accepted or
       if the event wasn't accepted but there's a captured node (as in that
       case it wouldn't propagate anywhere, making it impossible for the
       fallthrough nodes to catch such events). Those should also still get the
       relative position like the original event (which was potentially
       subsequently cleared for emitting enter/leave events). */
    if(moveAcceptedByAnyData || state.currentCapturedNode != NodeHandle::Null) {
        event._relativePosition = relativePosition;
        callFallthroughPointerEvents<PointerMoveEvent, &AbstractLayer::pointerMoveEvent>(
            calledNode != NodeHandle::Null ? calledNode : state.currentCapturedNode,
            globalPositionScaled, event, /*allowCapture*/ true);
    }

    return moveAcceptedByAnyData;
}

bool AbstractUserInterface::focusEvent(const NodeHandle node, FocusEvent& event) {
    CORRADE_ASSERT(!event._accepted,
        "Ui::AbstractUserInterface::focusEvent(): event already accepted", {});
    CORRADE_ASSERT(node == NodeHandle::Null || isHandleValid(node),
        "Ui::AbstractUserInterface::focusEvent(): invalid handle" << node, {});
    State& state = *_state;
    CORRADE_ASSERT(node == NodeHandle::Null || state.nodes[nodeHandleId(node)].used.flags >= NodeFlag::Focusable,
        "Ui::AbstractUserInterface::focusEvent(): node not focusable", {});

    /* Do an update. That may cause the currently focused node to be cleared,
       for example because it's now in a disabled/hidden hierarchy. */
    update();

    /* If a non-null node was meant to be focused but it's not focusable, the
       function is a no-op, i.e. not even calling a blur event on the
       previous */
    if(node != NodeHandle::Null && !state.visibleEventNodeMask[nodeHandleId(node)])
        return false;

    /* If the node to focus isn't null, call focusEvent() on it. event._pressed
       and event._hovering is set by callNonPositionedNonCapturingEventOnNode()
       itself */
    const bool focusAccepted = node != NodeHandle::Null && callFocusEventOnNode<&AbstractLayer::focusEvent>(node, event);

    /* Call the blur event and update the current focused node if ... */
    if(
        /* either the node to focus is null, */
        node == NodeHandle::Null ||
        /* or the focus event was accepted and the node is different from the
           previously focused one, */
        (focusAccepted && state.currentFocusedNode != node) ||
        /* or the focus event wasn't accepted and the node is the same as
           previously focused node (i.e., it decided to not accept focus
           anymore) */
        (!focusAccepted && state.currentFocusedNode == node)
    ) {
        /* event._pressed and event._hovering is set by
           callNonPositionedNonCapturingEventOnNode() itself */
        if(state.currentFocusedNode != NodeHandle::Null)
            callFocusEventOnNode<&AbstractLayer::blurEvent>(state.currentFocusedNode, event);

        /* The current focused node is now the `node` (which can be null), or
           as a special case null if a focus event wasn't accepted on a current
           focused node. */
        state.currentFocusedNode = !focusAccepted && state.currentFocusedNode == node ?
            NodeHandle::Null : node;
    }

    /* In particular, if a focus event on a different node wasn't accepted, the
       above branch is never entered, causing neither the blur event nor the
       current focused node to be updated. */

    return focusAccepted;
}

template<void(AbstractLayer::*function)(UnsignedInt, KeyEvent&)> bool AbstractUserInterface::keyPressOrReleaseEvent(KeyEvent& event) {
    /* Common code for keyPressEvent() and keyReleaseEvent() */

    update();

    State& state = *_state;

    /* If there's a focused node, direct the event there */
    bool acceptedByAnyData = false;
    if(state.currentFocusedNode != NodeHandle::Null) {
        CORRADE_INTERNAL_ASSERT(isHandleValid(state.currentFocusedNode));

        /* event._captured is false, event._hovering is set by
           callKeyEventOnNode() itself */

        acceptedByAnyData = callKeyEventOnNode<function>(state.currentFocusedNode, event);

        /* Changing the capture state isn't possible from a key event, and for
           the event being called on a focused node it's always false */
        CORRADE_INTERNAL_ASSERT(!event._captured);

    /* Otherwise, if we have a pointer position from a previous pointer event,
       send the key event based on that */
    } else if(state.currentGlobalPointerPosition) {
        /* If there's a node capturing events, call the event on it directly.
           Given that update() was called, it should be either null or
           valid. */
        if(state.currentCapturedNode != NodeHandle::Null) {
            CORRADE_INTERNAL_ASSERT(isHandleValid(state.currentCapturedNode));

            /* Called on a captured node, so isCaptured() should be true,
               isNodeHovered() is true if it's also currently hovered */
            event._captured = true;
            event._nodeHovered = state.currentHoveredNode == state.currentCapturedNode;

            acceptedByAnyData = callEventOnNode<KeyEvent, function>(*state.currentGlobalPointerPosition, state.currentCapturedNode, event);

        /* Otherwise call it on the currently hovered node, if there is. Again,
           at this point it should be either null or valid. */
        } else if(state.currentHoveredNode != NodeHandle::Null) {
            CORRADE_INTERNAL_ASSERT(isHandleValid(state.currentHoveredNode));

            /* Not called on a captured node, but on a hovered node */
            event._captured = false;
            event._nodeHovered = true;

            acceptedByAnyData = callEventOnNode<KeyEvent, function>(*state.currentGlobalPointerPosition, state.currentHoveredNode, event);
        }

        /* Changing the capture state isn't possible from a key event, as that
           would need to potentially emit a pointer leave and pointer enter
           event which isn't really possible now. */
        CORRADE_INTERNAL_ASSERT(event._captured == (state.currentGlobalPointerPosition && state.currentCapturedNode != NodeHandle::Null));
    }

    return acceptedByAnyData;
}

bool AbstractUserInterface::keyPressEvent(KeyEvent& event) {
    CORRADE_ASSERT(!event._accepted,
        "Ui::AbstractUserInterface::keyPressEvent(): event already accepted", {});

    return keyPressOrReleaseEvent<&AbstractLayer::keyPressEvent>(event);
}

bool AbstractUserInterface::keyReleaseEvent(KeyEvent& event) {
    CORRADE_ASSERT(!event._accepted,
        "Ui::AbstractUserInterface::keyReleaseEvent(): event already accepted", {});

    return keyPressOrReleaseEvent<&AbstractLayer::keyReleaseEvent>(event);
}

bool AbstractUserInterface::textInputEvent(TextInputEvent& event) {
    CORRADE_ASSERT(!event._accepted,
        "Ui::AbstractUserInterface::textInputEvent(): event already accepted", {});

    /* Do an update. That may cause the currently focused node to be cleared,
       for example because it's now in a disabled/hidden hierarchy. */
    update();

    State& state = *_state;

    /* If no node is focused, the function is a no-op */
    if(state.currentFocusedNode == NodeHandle::Null)
        return false;

    /* Call the event on the focused node. There's no fallback to anywhere else
       at the moment. */
    return callTextInputEventOnNode(state.currentFocusedNode, event);
}

NodeHandle AbstractUserInterface::currentPressedNode() const {
    return _state->currentPressedNode;
}

NodeHandle AbstractUserInterface::currentCapturedNode() const {
    return _state->currentCapturedNode;
}

NodeHandle AbstractUserInterface::currentHoveredNode() const {
    return _state->currentHoveredNode;
}

NodeHandle AbstractUserInterface::currentFocusedNode() const {
    return _state->currentFocusedNode;
}

Containers::Optional<Vector2> AbstractUserInterface::currentGlobalPointerPosition() const {
    return _state->currentGlobalPointerPosition;
}

}}
