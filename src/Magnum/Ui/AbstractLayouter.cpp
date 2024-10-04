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

#include "AbstractLayouter.h"

#include <Corrade/Containers/BitArray.h>
#include <Corrade/Containers/BitArrayView.h>
#include <Corrade/Containers/GrowableArray.h>
#include <Corrade/Containers/EnumSet.hpp>
#include <Corrade/Containers/StridedArrayView.h>
#include <Magnum/Math/Vector2.h>

#include "Magnum/Ui/Handle.h"

namespace Magnum { namespace Ui {

Debug& operator<<(Debug& debug, const LayouterState value) {
    debug << "Ui::LayouterState" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case LayouterState::value: return debug << "::" #value;
        _c(NeedsUpdate)
        _c(NeedsAssignmentUpdate)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "(" << Debug::nospace << Debug::hex << UnsignedByte(value) << Debug::nospace << ")";
}

Debug& operator<<(Debug& debug, const LayouterStates value) {
    return Containers::enumSetDebugOutput(debug, value, "Ui::LayouterStates{}", {
        LayouterState::NeedsAssignmentUpdate,
        /* Implied by NeedsAssignmentUpdate, has to be after */
        LayouterState::NeedsUpdate
    });
}

namespace {

union Layout {
    explicit Layout() noexcept: used{} {}

    struct Used {
        /* Together with index of this item in `layouts` used for creating a
           LayouterDataHandle. Increased every time a handle reaches remove().
           Has to be initially non-zero to differentiate the first ever handle
           (with index 0) from LayouterDataHandle::Null. Once becomes
           `1 << LayouterDataHandleGenerationBits` the handle gets disabled. */
        UnsignedShort generation = 1;

        /* Two bytes free */

        /* Node the layout is assigned to. Is null only when the layout is
           freed. Has to be re-filled every time a handle is recycled, so it
           doesn't make sense to initialize it to anything. */
        NodeHandle node;

        /* Four bytes free */
    } used;

    /* Used only if the Layout is among free ones */
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
static_assert(std::is_trivially_copyable<Layout>::value, "Layout not trivially copyable");
#endif
static_assert(
    offsetof(Layout::Used, generation) == offsetof(Layout::Free, generation) &&
    offsetof(Layout::Used, node) == offsetof(Layout::Free, node),
    "Layout::Used and Free layout not compatible");

}

struct AbstractLayouter::State {
    LayouterHandle handle;
    LayouterStates state;

    #ifndef CORRADE_NO_ASSERT
    bool setSizeCalled = false;
    #endif
    /* 0/4 bytes free, 1/5 on a no-assert build */

    Containers::Array<Layout> layouts;
    /* Indices in the `layouts` array. The Layout then has a nextFree member
       containing the next free index. New layouts get taken from the front,
       removed are put at the end. A value of ~UnsignedInt{} means there's no
       (first/next/last) free layout. */
    UnsignedInt firstFree = ~UnsignedInt{};
    UnsignedInt lastFree = ~UnsignedInt{};
};

AbstractLayouter::AbstractLayouter(const LayouterHandle handle): _state{InPlaceInit} {
    CORRADE_ASSERT(handle != LayouterHandle::Null,
        "Ui::AbstractLayouter: handle is null", );
    _state->handle = handle;
}

AbstractLayouter::AbstractLayouter(AbstractLayouter&&) noexcept = default;

AbstractLayouter::~AbstractLayouter() = default;

AbstractLayouter& AbstractLayouter::operator=(AbstractLayouter&&) noexcept = default;

LayouterHandle AbstractLayouter::handle() const {
    return _state->handle;
}

LayouterStates AbstractLayouter::state() const {
    return _state->state;
}

void AbstractLayouter::setNeedsUpdate() {
    _state->state |= LayouterState::NeedsUpdate;
}

std::size_t AbstractLayouter::capacity() const {
    return _state->layouts.size();
}

std::size_t AbstractLayouter::usedCount() const {
    /* The node is null only for free layouts, so compared to all other
       usedCount() implementations we can iterate directly instead of going
       through the linked list. Need to also check for disabled nodes however,
       as those are not part of the free list either. */
    const State& state = *_state;
    std::size_t free = 0;
    for(const Layout& i: state.layouts)
        if(i.used.node == NodeHandle::Null && i.used.generation != 1 << Implementation::LayouterDataHandleGenerationBits)
            ++free;
    return state.layouts.size() - free;
}

bool AbstractLayouter::isHandleValid(const LayouterDataHandle handle) const {
    if(handle == LayouterDataHandle::Null)
        return false;
    const State& state = *_state;
    const UnsignedInt index = layouterDataHandleId(handle);
    if(index >= state.layouts.size())
        return false;
    /* Unlike UserInterface::isHandleValid(LayouterHandle), the generation
       counter here is 16bit and a disabled handle is signalized by 0x10000,
       not 0, so for disabled handles this will always fail without having to
       do any extra checks.

       Note that this can still return true for manually crafted handles that
       point to free layouts with correct generation counters. That could be
       detected by checking that the node reference is not null, but as no
       other isHandleValid() is capable of that without adding extra state I
       don't think making a single variant tighter is going to make any
       difference. */
    return layouterDataHandleGeneration(handle) == state.layouts[index].used.generation;
}

bool AbstractLayouter::isHandleValid(const LayoutHandle handle) const {
    return layoutHandleLayouter(handle) == _state->handle && isHandleValid(layoutHandleData(handle));
}

LayoutHandle AbstractLayouter::add(const NodeHandle node) {
    CORRADE_ASSERT(node != NodeHandle::Null,
        "Ui::AbstractLayouter::add(): invalid handle" << node, {});

    State& state = *_state;

    /* Find the first free layout if there is, update the free index to point
       to the next one (or none) */
    Layout* layout;
    if(state.firstFree!= ~UnsignedInt{}) {
        layout = &state.layouts[state.firstFree];

        if(state.firstFree == state.lastFree) {
            CORRADE_INTERNAL_ASSERT(layout->free.next == ~UnsignedInt{});
            state.firstFree = state.lastFree = ~UnsignedInt{};
        } else {
            state.firstFree = layout->free.next;
        }

    /* If there isn't, allocate a new one */
    } else {
        CORRADE_ASSERT(state.layouts.size() < 1 << Implementation::LayouterDataHandleIdBits,
            "Ui::AbstractLayouter::add(): can only have at most" << (1 << Implementation::LayouterDataHandleIdBits) << "layouts", {});
        layout = &arrayAppend(state.layouts, InPlaceInit);
    }

    /* Fill the data. In both above cases the generation is already set
       appropriately, either initialized to 1, or incremented when it got
       remove()d (to mark existing handles as invalid) */
    layout->used.node = node;
    state.state |= LayouterState::NeedsAssignmentUpdate;

    return layoutHandle(state.handle, (layout - state.layouts), layout->used.generation);
}

void AbstractLayouter::remove(const LayoutHandle handle) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractLayouter::remove(): invalid handle" << handle, );

    _state->state |= LayouterState::NeedsAssignmentUpdate;

    /* Doesn't delegate to remove(LayouterNodeHandle) to avoid a double check;
       doesn't check just the layouter portion of the handle and delegate to
       avoid a confusing assertion message if the data portion would be
       invalid */
    removeInternal(layoutHandleId(handle));
}

void AbstractLayouter::remove(const LayouterDataHandle handle) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractLayouter::remove(): invalid handle" << handle, );

    _state->state |= LayouterState::NeedsAssignmentUpdate;

    removeInternal(layouterDataHandleId(handle));
}

void AbstractLayouter::removeInternal(const UnsignedInt id) {
    State& state = *_state;
    Layout& layout = state.layouts[id];

    /* Increase the layout generation so existing handles pointing to this
       layout are invalidated */
    ++layout.used.generation;

    /* Set the node attachment to null to avoid falsely recognizing this item
       as used when directly iterating the list */
    layout.used.node = NodeHandle::Null;

    /* Put the layout at the end of the free list (while they're allocated from
       the front) to not exhaust the generation counter too fast. If the free
       list is empty however, update also the index of the first free layer.

       Don't do this if the generation wrapped around. That makes it disabled,
       i.e. impossible to be recycled later, to avoid aliasing old handles. */
    if(layout.used.generation != 1 << Implementation::LayouterDataHandleGenerationBits) {
        layout.free.next = ~UnsignedInt{};
        if(state.lastFree == ~UnsignedInt{}) {
            CORRADE_INTERNAL_ASSERT(
                state.firstFree == ~UnsignedInt{} &&
                state.lastFree == ~UnsignedInt{});
            state.firstFree = id;
        } else {
            state.layouts[state.lastFree].free.next = id;
        }
        state.lastFree = id;
    }

    /* Updating LayouterState (or not) is caller's responsibility. For example,
       clean() below doesn't set any state after calling removeInternal(). */
}

NodeHandle AbstractLayouter::node(LayoutHandle layout) const {
    CORRADE_ASSERT(isHandleValid(layout),
        "Ui::AbstractLayouter::node(): invalid handle" << layout, {});
    return _state->layouts[layoutHandleId(layout)].used.node;
}

NodeHandle AbstractLayouter::node(LayouterDataHandle layout) const {
    CORRADE_ASSERT(isHandleValid(layout),
        "Ui::AbstractLayouter::node(): invalid handle" << layout, {});
    return _state->layouts[layouterDataHandleId(layout)].used.node;
}

Containers::StridedArrayView1D<const NodeHandle> AbstractLayouter::nodes() const {
    return stridedArrayView(_state->layouts).slice(&Layout::used).slice(&Layout::Used::node);
}

Containers::StridedArrayView1D<const UnsignedShort> AbstractLayouter::generations() const {
    return stridedArrayView(_state->layouts).slice(&Layout::used).slice(&Layout::Used::generation);
}

void AbstractLayouter::setSize(const Vector2& size) {
    CORRADE_ASSERT(size.product(),
        "Ui::AbstractLayouter::setSize(): expected a non-zero size, got" << size, );
    #ifndef CORRADE_NO_ASSERT
    _state->setSizeCalled = true;
    #endif
    doSetSize(size);
}

void AbstractLayouter::doSetSize(const Vector2&) {}

void AbstractLayouter::cleanNodes(const Containers::StridedArrayView1D<const UnsignedShort>& nodeHandleGenerations) {
    State& state = *_state;
    /** @todo have some bump allocator for this */
    Containers::BitArray layoutIdsToRemove{ValueInit, state.layouts.size()};

    for(std::size_t i = 0; i != state.layouts.size(); ++i) {
        const Layout& layout = state.layouts[i];

        /* Skip layouts that are free */
        if(layout.used.node == NodeHandle::Null)
            continue;

        /* For used layout compare the generation of the node they're attached
           to. If it differs, remove the layout and mark the corresponding
           index so the implementation can do its own cleanup in doClean(). */
        /** @todo check that the ID is in bounds and if it's not, remove as
            well? to avoid OOB access if the layout is accidentally attached to
            a NodeHandle from a different UI instance that has more nodes */
        if(nodeHandleGeneration(layout.used.node) != nodeHandleGenerations[nodeHandleId(layout.used.node)]) {
            removeInternal(i);
            layoutIdsToRemove.set(i);
        }
    }

    doClean(layoutIdsToRemove);
}

void AbstractLayouter::doClean(Containers::BitArrayView) {}

void AbstractLayouter::update(const Containers::BitArrayView layoutIdsToUpdate, const Containers::StridedArrayView1D<const UnsignedInt>& topLevelLayoutIds, const Containers::StridedArrayView1D<const NodeHandle>& nodeParents, const Containers::StridedArrayView1D<Vector2>& nodeOffsets, const Containers::StridedArrayView1D<Vector2>& nodeSizes) {
    CORRADE_ASSERT(layoutIdsToUpdate.size() == capacity(),
        "Ui::AbstractLayouter::update(): expected layoutIdsToUpdate to have" << capacity() << "bits but got" << layoutIdsToUpdate.size(), );
    CORRADE_ASSERT(nodeOffsets.size() == nodeParents.size() && nodeSizes.size() == nodeParents.size(),
        "Ui::AbstractLayouter::update(): expected node parent, offset and size views to have the same size but got" << nodeParents.size() << Debug::nospace << "," << nodeOffsets.size() << "and" << nodeSizes.size(), );
    State& state = *_state;
    CORRADE_ASSERT(state.setSizeCalled,
        "Ui::AbstractLayouter::update(): user interface size wasn't set", );
    doUpdate(layoutIdsToUpdate, topLevelLayoutIds, nodeParents, nodeOffsets, nodeSizes);
    state.state &= ~LayouterState::NeedsAssignmentUpdate;
}

}}
