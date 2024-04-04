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

#include "SnapLayouter.h"

#include <Corrade/Containers/BitArrayView.h>
#include <Corrade/Containers/GrowableArray.h>
#include <Corrade/Containers/EnumSet.hpp>
#include <Corrade/Containers/StridedArrayView.h>
#include <Magnum/Math/Vector2.h>

#include "Magnum/Whee/Anchor.h"
#include "Magnum/Whee/Handle.h"
#include "Magnum/Whee/Implementation/snapLayouter.h"
#include "Magnum/Whee/UserInterface.h" // TODO uhh maybe put it to a separate file? so the high-level stuff can be compiled away

namespace Magnum { namespace Whee {

Debug& operator<<(Debug& debug, const Snap value) {
    const bool packed = debug.immediateFlags() >= Debug::Flag::Packed;

    if(!packed)
        debug << "Whee::Snap" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case Snap::value: return debug << (packed ? "" : "::") << Debug::nospace << #value;
        _c(Top)
        _c(Left)
        _c(Bottom)
        _c(Right)
        _c(InsideX)
        _c(InsideY)
        _c(Inside)
        _c(NoSpaceX)
        _c(NoSpaceY)
        _c(NoSpace)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << (packed ? "" : "(") << Debug::nospace << Debug::hex << UnsignedByte(value) << Debug::nospace << (packed ? "" : ")");
}

Debug& operator<<(Debug& debug, const Snaps value) {
    return Containers::enumSetDebugOutput(debug, value, debug.immediateFlags() >= Debug::Flag::Packed ? "{}" : "Whee::Snaps{}", {
        Snap::Top,
        Snap::Left,
        Snap::Bottom,
        Snap::Right,
        /* Combination of InsideX and InsideY, has to be first */
        Snap::Inside,
        Snap::InsideX,
        Snap::InsideY,
        /* Combination of NoSpaceX and NoSpaceY, has to be first */
        Snap::NoSpace,
        Snap::NoSpaceX,
        Snap::NoSpaceY,
    });
}

namespace {

struct Layout {
    NodeHandle target;
    Snaps snap;
    /* 3 bytes free */
};

}

struct SnapLayouter::State {
    Vector4 padding;
    Vector2 margin;
    Containers::Array<Layout> layouts;
};

SnapLayouter::SnapLayouter(const LayouterHandle handle): AbstractLayouter{handle}, _state{InPlaceInit} {}

SnapLayouter::~SnapLayouter() = default;

Vector4 SnapLayouter::padding() const {
    return _state->padding;
}

SnapLayouter& SnapLayouter::setPadding(const Vector4& padding) {
    _state->padding = padding;
    return *this;
}

SnapLayouter& SnapLayouter::setPadding(const Vector2& padding) {
    return setPadding(Math::gather<'x', 'y', 'x', 'y'>(padding));
}

Vector2 SnapLayouter::margin() const {
    return _state->margin;
}

SnapLayouter& SnapLayouter::setMargin(const Vector2& margin) {
    _state->margin = margin;
    return *this;
}

LayoutHandle SnapLayouter::add(NodeHandle node, Snaps snap, NodeHandle target) {
    // TODO disallow to being a child of node
    // TODO or generally, shouldn't `to` be either a parent or a sibling? could also be a child of a sibling, or a child of a parent sibling, so
    // TODO there needs to be some cycle detection, which is i guess preventing `to` being a child of `node`

    // TODO document this requirement
    // TODO what if snapping to the whole UI? uhh
    CORRADE_INTERNAL_ASSERT(target != NodeHandle::Null);

    State& state = *_state;

    const LayoutHandle handle = AbstractLayouter::add(node);
    const UnsignedInt id = layoutHandleId(handle);
    if(id >= state.layouts.size())
        arrayAppend(state.layouts, NoInit, id - state.layouts.size() + 1);

    Layout& layout = state.layouts[id];
    layout.snap = snap;
    layout.target = target;
    return handle;
}

void SnapLayouter::doUpdate(Containers::BitArrayView layoutIdsToUpdate, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<Vector2>& nodeOffsets, const Containers::StridedArrayView1D<Vector2>& nodeSizes) {
    const State& state = *_state;

    // TODO don't assume the data are in hierarchy order

    // TODO ffs this needs to get the parent info as well ....

    const Containers::StridedArrayView1D<const NodeHandle> nodes = this->nodes();

    /** @todo some way to iterate set bits */
    for(std::size_t i = 0; i != layoutIdsToUpdate.size(); ++i) {
        if(!layoutIdsToUpdate[i])
            continue;

        const Layout& layout = _state->layouts[i];
        // TODO check if null, what then
        const UnsignedInt nodeId = nodeHandleId(nodes[i]);
        const UnsignedInt nodeToId = nodeHandleId(layout.target);
        const Containers::Pair<Vector2, Vector2> out = Implementation::snap(layout.snap,
            nodeOffsets[nodeToId], nodeSizes[nodeToId],
            // TODO have parent-node-local padding
            state.padding,
            state.margin,
            nodeSizes[nodeId]
        );

        // TODO wait, how is it with parent-relative transforms??
        nodeOffsets[nodeId] += out.first(); // TODO er, preserve the offset here?
        nodeSizes[nodeId] = out.second();
        !Debug{} << nodeId << nodeOffsets[nodeId] << nodeSizes[nodeId];
    }
}

struct SnapLayout::State {
    explicit State(UserInterface& ui, SnapLayouter& layouter, NodeHandle parent): ui(ui), layouter(layouter), nextParent{parent} {}

    UserInterface& ui;
    SnapLayouter& layouter;
    NodeHandle nextParent;
    NodeHandle nextTarget = NodeHandle::Null;
    Vector2 nextOffset, nextSize;
    Snaps nextSnap;
};

SnapLayout::SnapLayout(UserInterface& ui, SnapLayouter& layouter, NodeHandle parent): _state{InPlaceInit, ui, layouter, parent} {}

SnapLayout::SnapLayout(SnapLayout&&) noexcept = default;

SnapLayout::~SnapLayout() = default;

SnapLayout& SnapLayout::operator=(SnapLayout&&) noexcept = default;

NodeHandle SnapLayout::nextParent() const { return _state->nextParent; }

SnapLayout& SnapLayout::setNextParent(const NodeHandle parent) {
    // TODO a non-null check? or not at all
    _state->nextParent = parent;
    return *this;
}

Vector2 SnapLayout::nextOffset() const { return _state->nextOffset; }

SnapLayout& SnapLayout::setNextOffset(const Vector2& offset) {
    _state->nextOffset = offset;
    return *this;
}

Vector2 SnapLayout::nextSize() const { return _state->nextSize; }

SnapLayout& SnapLayout::setNextSize(const Vector2& size) {
    _state->nextSize = size;
    return *this;
}

Snaps SnapLayout::nextSnap() const { return _state->nextSnap; }

SnapLayout& SnapLayout::setNextSnap(const Snaps snap) {
    _state->nextSnap = snap;
    return *this;
}

NodeHandle SnapLayout::nextTarget() const { return _state->nextTarget; }

// TODO setNextTarget?

Anchor SnapLayout::operator()(const Snaps snap, const NodeHandle target, const Vector2& offset, const Vector2& size, const NodeFlags flags) {
    State& state = *_state;
    const NodeHandle node = state.ui.createNode(state.nextParent, offset, size, flags);
    const LayoutHandle layout = state.layouter.add(node, snap, target);
    state.nextTarget = node;
    return Anchor{state.ui, node, layout};
}

Anchor SnapLayout::operator()(const Snaps snap, const NodeHandle target, const Vector2& size, const NodeFlags flags) {
    // TODO should this pick nextOffset or zero??
    return operator()(snap, target, {}, size, flags);
}

Anchor SnapLayout::operator()(const Snaps snap, const NodeHandle target, const NodeFlags flags) {
    State& state = *_state;
    return operator()(snap, target, state.nextOffset, state.nextSize, flags);
}

Anchor SnapLayout::operator()(const Snaps snap, const Vector2& offset, const Vector2& size, const NodeFlags flags) {
    State& state = *_state;
    // TODO nextTarget may be null, assert on that
    return operator()(snap, state.nextTarget, offset, size, flags);
}

Anchor SnapLayout::operator()(const Snaps snap, const Vector2& size, const NodeFlags flags) {
    State& state = *_state;
    // TODO nextTarget may be null, assert on that
    // TODO should this pick nextOffset or zero??
    return operator()(snap, state.nextTarget, {}, size, flags);
}

Anchor SnapLayout::operator()(const Snaps snap, const NodeFlags flags) {
    State& state = *_state;
    // TODO nextTarget may be null, assert on that
    return operator()(snap, state.nextTarget, state.nextOffset, state.nextSize, flags);
}

Anchor SnapLayout::operator()(const Vector2& offset, const Vector2& size, const NodeFlags flags) {
    State& state = *_state;
    // TODO nextTarget may be null, assert on that
    return operator()(state.nextSnap, state.nextTarget, offset, size, flags);
}

Anchor SnapLayout::operator()(const Vector2& size, const NodeFlags flags) {
    State& state = *_state;
    // TODO nextTarget may be null, assert on that
    // TODO should this pick nextOffset or zero??
    return operator()(state.nextSnap, state.nextTarget, {}, size, flags);
}

Anchor SnapLayout::operator()(const NodeFlags flags) {
    State& state = *_state;
    // TODO nextTarget may be null, assert on that
    return operator()(state.nextSnap, state.nextTarget, state.nextOffset, state.nextSize, flags);
}

// TODO why this? dop
SnapLayout::operator Anchor() {
    State& state = *_state;
    // TODO nextTarget may be null, assert on that
    return operator()(state.nextSnap, state.nextTarget, state.nextOffset, state.nextSize);
}

}}
