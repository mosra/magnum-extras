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

#include "SnapLayouter.h"

#include <Corrade/Containers/ArrayTuple.h>
#include <Corrade/Containers/BitArrayView.h>
#include <Corrade/Containers/GrowableArray.h>
#include <Corrade/Containers/EnumSet.hpp>
#include <Corrade/Containers/StridedArrayView.h>
#include <Magnum/Math/Vector2.h>

#include "Magnum/Ui/Anchor.h"
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/Implementation/orderNodesBreadthFirstInto.h"
#include "Magnum/Ui/Implementation/snapLayouter.h"
#include "Magnum/Ui/UserInterface.h"

namespace Magnum { namespace Ui {

Debug& operator<<(Debug& debug, const Snap value) {
    const bool packed = debug.immediateFlags() >= Debug::Flag::Packed;

    if(!packed)
        debug << "Ui::Snap" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case Snap::value: return debug << (packed ? "" : "::") << Debug::nospace << #value;
        _c(Top)
        _c(Left)
        _c(Bottom)
        _c(Right)
        _c(TopLeft)
        _c(BottomLeft)
        _c(TopRight)
        _c(BottomRight)
        _c(FillX)
        _c(FillY)
        _c(Fill)
        _c(InsideX)
        _c(InsideY)
        _c(Inside)
        _c(NoPadX)
        _c(NoPadY)
        _c(NoPad)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << (packed ? "" : "(") << Debug::nospace << Debug::hex << UnsignedByte(value) << Debug::nospace << (packed ? "" : ")");
}

Debug& operator<<(Debug& debug, const Snaps value) {
    return Containers::enumSetDebugOutput(debug, value, debug.immediateFlags() >= Debug::Flag::Packed ? "{}" : "Ui::Snaps{}", {
        /* Combination of FillX and FillY, has to be first */
        Snap::Fill,
        /* Combinations of Left, Right and Top, Bottom, has to be first */
        Snap::FillX,
        Snap::FillY,
        /* Combinations of Top, Left, Bottom, Right, have to be first, but
           below Fill as that makes more sense in the output */
        Snap::TopLeft,
        Snap::BottomLeft,
        Snap::TopRight,
        Snap::BottomRight,
        Snap::Top,
        Snap::Left,
        Snap::Bottom,
        Snap::Right,
        /* Combination of InsideX and InsideY, has to be first */
        Snap::Inside,
        Snap::InsideX,
        Snap::InsideY,
        /* Combination of NoPadX and NoPadY, has to be first */
        Snap::NoPad,
        Snap::NoPadX,
        Snap::NoPadY,
    });
}

namespace {

struct Layout {
    NodeHandle target;
    Snaps snap{NoInit};
    /* 3 bytes free */
};

}

struct SnapLayouter::State {
    Vector4 padding;
    Vector2 margin;
    Containers::Array<Layout> layouts;
    Vector2 uiSize;
};

SnapLayouter::SnapLayouter(const LayouterHandle handle): AbstractLayouter{handle}, _state{InPlaceInit} {}

SnapLayouter::SnapLayouter(SnapLayouter&&) noexcept = default;

SnapLayouter::~SnapLayouter() = default;

SnapLayouter& SnapLayouter::operator=(SnapLayouter&&) noexcept = default;

Vector4 SnapLayouter::padding() const { return _state->padding; }

SnapLayouter& SnapLayouter::setPadding(const Vector4& padding) {
    _state->padding = padding;
    setNeedsUpdate();
    return *this;
}

SnapLayouter& SnapLayouter::setPadding(const Vector2& padding) {
    return setPadding(Math::gather<'x', 'y', 'x', 'y'>(padding));
}

SnapLayouter& SnapLayouter::setPadding(const Float padding) {
    return setPadding(Vector4{padding});
}

Vector2 SnapLayouter::margin() const { return _state->margin; }

SnapLayouter& SnapLayouter::setMargin(const Vector2& margin) {
    _state->margin = margin;
    setNeedsUpdate();
    return *this;
}

SnapLayouter& SnapLayouter::setMargin(const Float margin) {
    return setMargin(Vector2{margin});
}

Snaps SnapLayouter::snap(const LayoutHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::SnapLayouter::snap(): invalid handle" << handle, {});
    return _state->layouts[layoutHandleId(handle)].snap;
}

Snaps SnapLayouter::snap(const LayouterDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::SnapLayouter::snap(): invalid handle" << handle, {});
    return _state->layouts[layouterDataHandleId(handle)].snap;
}

NodeHandle SnapLayouter::target(const LayoutHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::SnapLayouter::target(): invalid handle" << handle, {});
    return _state->layouts[layoutHandleId(handle)].target;
}

NodeHandle SnapLayouter::target(const LayouterDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::SnapLayouter::target(): invalid handle" << handle, {});
    return _state->layouts[layouterDataHandleId(handle)].target;
}

LayoutHandle SnapLayouter::add(const NodeHandle node, const Snaps snap, const NodeHandle target) {
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

LayouterFeatures SnapLayouter::doFeatures() const {
    return {};
}

void SnapLayouter::doSetSize(const Vector2& size) {
    State& state = *_state;
    state.uiSize = size;

    /* Mark the layouter as needing an update. This could also be set only if
       there are any layouts snapped directly to the UI itself, but right now
       I'd say that's >90% of use cases so it doesn't make sense to try to
       make the rest more efficient -- for that there would need to be some
       bitmask of (non-freed) layouts snapped to the UI, which then gets
       updated on every remove() and in doClean(), and that's just a lot of
       code for questinable gains.

       It's also set even if the size is the same as before, as
       AbstractUserInterface itself makes sure that setSize() is called only
       when the value is different, so it doesn't make sense to duplicate that
       logic here. */
    setNeedsUpdate();
}

void SnapLayouter::doUpdate(const Containers::BitArrayView layoutIdsToUpdate, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const NodeHandle>& nodeParents, const Containers::StridedArrayView1D<Vector2>& nodeOffsets, const Containers::StridedArrayView1D<Vector2>& nodeSizes) {
    const State& state = *_state;

    /* Order layouts breadth first in dependency order to ensure the parent
       node offset / size is known when calculating child node layout */
    /** @todo The childrenOffsets and children have a non-overlapping lifetime
        with layoutOffsets and layouts, but handling that manually is too messy
        -- clean up once a bump allocator exists */
    /** @todo If other layouters start needing this, it may be beneficial to
        do this in AbstractUserInterface already and pass an ordered list of
        layout IDs to update. If not, it might be beneficial to split this
        function into update() + layout(), where the former gets the *full*
        mask of layouts and can perform this ordering just once, not for every
        call */
    Containers::ArrayView<UnsignedInt> childrenOffsets;
    Containers::ArrayView<UnsignedInt> children;
    Containers::ArrayView<Int> nodeIdsBreadthFirst;
    Containers::ArrayView<UnsignedInt> layoutOffsets;
    Containers::ArrayView<UnsignedInt> layouts;
    Containers::ArrayView<UnsignedInt> layoutIds;
    Containers::ArrayTuple storage{
        /* +1 for the last offset, +1 for root nodes */
        {ValueInit, nodeParents.size() + 2, childrenOffsets},
        {NoInit, nodeParents.size(), children},
        /* +1 for the first element which is -1 indicating a root */
        {NoInit, nodeParents.size() + 1, nodeIdsBreadthFirst},
        /* +1 for the last offset, +1 for layouts that target the UI */
        {ValueInit, nodeParents.size() + 2, layoutOffsets},
        {NoInit, layoutIdsToUpdate.size(), layouts},
        {NoInit, layoutIdsToUpdate.size(), layoutIds},
    };
    /* First order the nodes themselves ... */
    Implementation::orderNodesBreadthFirstInto(
        nodeParents,
        childrenOffsets, children, nodeIdsBreadthFirst);
    /* Then use the ordered nodes to order the layouts */
    const std::size_t count = Implementation::orderLayoutsBreadthFirstInto(
        layoutIdsToUpdate,
        stridedArrayView(state.layouts).slice(&Layout::target),
        nodeIdsBreadthFirst,
        layoutOffsets,
        layouts,
        layoutIds);

    const Containers::StridedArrayView1D<const NodeHandle> nodes = this->nodes();

    /* Go through the layouts in their dependency order, skipping the first
       item which was -1 */
    for(const std::size_t i: layoutIds.prefix(count)) {
        const Layout& layout = _state->layouts[i];
        const UnsignedInt nodeId = nodeHandleId(nodes[i]);

        /* If the target is null, we're snapping to the whole UI */
        Snaps snap = layout.snap;
        Vector2 targetOffset{NoInit}, targetSize{NoInit};
        if(layout.target == NodeHandle::Null) {
            /* This was ensured by the snap() helper itself, which makes the
               parent null if the target is null */
            CORRADE_INTERNAL_DEBUG_ASSERT(nodeParents[nodeId] == NodeHandle::Null);
            snap |= Snap::Inside;
            targetOffset = {};
            targetSize = state.uiSize;

        /* Otherwise we're snapping relative to the parent node, which should
           have the layout already calculated at this point thanks to the
           dependency ordering */
        } else {
            const UnsignedInt nodeTargetId = nodeHandleId(layout.target);
            targetSize = nodeSizes[nodeTargetId];
            /* If the nodes are siblings, include the target offset in the
               calculation */
            if(nodeParents[nodeId] == nodeParents[nodeTargetId])
                targetOffset = nodeOffsets[nodeTargetId];
            /* Otherwise, if the target is a parent, don't */
            else if(nodeParents[nodeId] == layout.target)
                targetOffset = Vector2{};
            /* There's no other possible case, again ensured by the SnapLayout
               or the snap() helper, which makes the node either a sibling or
               a child of the target */
            else CORRADE_INTERNAL_DEBUG_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
        }

        const Containers::Pair<Vector2, Vector2> out = Implementation::snap(snap,
            targetOffset, targetSize,
            state.padding,
            state.margin,
            nodeSizes[nodeId]);

        /* The original node offset is added to the calculated layout, size
           may be (partially) replaced */
        nodeOffsets[nodeId] += out.first();
        nodeSizes[nodeId] = out.second();
    }
}

AbstractSnapLayout::AbstractSnapLayout(AbstractUserInterface& ui, SnapLayouter& layouter, const Snaps snapFirst, const NodeHandle target, const Snaps snapNext): _ui{ui}, _layouter{layouter}, _targetFirst{target}, _snapFirst{snapFirst}, _snapNext{snapNext} {
    CORRADE_ASSERT(_ui->isHandleValid(target),
        "Ui::AbstractSnapLayout: invalid target handle" << target, );
    if(Implementation::snapInside(snapFirst).all())
        _parent = target;
    else {
        _parent = _ui->nodeParent(target);
        CORRADE_ASSERT(_parent != NodeHandle::Null,
            "Ui::AbstractSnapLayout: target cannot be a root node for" << snapFirst, );
    }
}

AbstractAnchor AbstractSnapLayout::operator()(const Vector2& offset, const Vector2& size, const NodeFlags flags) {
    const NodeHandle node = _ui->createNode(_parent, offset, size, flags);
    const LayoutHandle layout = _layouter->add(node,
        _targetNext == NodeHandle::Null ? _snapFirst : _snapNext,
        _targetNext == NodeHandle::Null ? _targetFirst : _targetNext);
    _targetNext = node;
    return AbstractAnchor{_ui, node, layout};
}

AbstractAnchor AbstractSnapLayout::operator()(const Vector2& size, const NodeFlags flags) {
    return operator()({}, size, flags);
}

AbstractAnchor snap(AbstractUserInterface& ui, SnapLayouter& layouter, /*mutable*/ Snaps snap, const NodeHandle target, const Vector2& offset, const Vector2& size, const NodeFlags flags) {
    NodeHandle parent;
    if(target == NodeHandle::Null) {
        parent = NodeHandle::Null;
    } else {
        CORRADE_ASSERT(ui.isHandleValid(target),
            "Ui::snap(): invalid target handle" << target, (AbstractAnchor{ui, {}}));
        if(Implementation::snapInside(snap).all())
            parent = target;
        else
            parent = ui.nodeParent(target);
    }

    const NodeHandle node = ui.createNode(parent, offset, size, flags);
    const LayoutHandle layout = layouter.add(node, snap, target);
    return AbstractAnchor{ui, node, layout};
}

AbstractAnchor snap(AbstractUserInterface& ui, SnapLayouter& layouter, const Snaps snap, const NodeHandle target, const Vector2& size, const NodeFlags flags) {
    return Ui::snap(ui, layouter, snap, target, {}, size, flags);
}

AbstractAnchor snap(AbstractUserInterface& ui, SnapLayouter& layouter, const Snaps snap, const Vector2& offset, const Vector2& size, const NodeFlags flags) {
    return Ui::snap(ui, layouter, snap, NodeHandle::Null, offset, size, flags);
}

AbstractAnchor snap(AbstractUserInterface& ui, SnapLayouter& layouter, const Snaps snap, const Vector2& size, const NodeFlags flags) {
    return Ui::snap(ui, layouter, snap, {}, size, flags);
}

Anchor snap(UserInterface& ui, SnapLayouter& layouter, const Snaps snap, const NodeHandle target, const Vector2& offset, const Vector2& size, const NodeFlags flags) {
    return Anchor{ui, Ui::snap(static_cast<AbstractUserInterface&>(ui), layouter, snap, target, offset, size, flags)};
}

Anchor snap(UserInterface& ui, const Snaps snap, const NodeHandle target, const Vector2& offset, const Vector2& size, const NodeFlags flags) {
    return Ui::snap(ui, ui.snapLayouter(), snap, target, offset, size, flags);
}

Anchor snap(UserInterface& ui, SnapLayouter& layouter, const Snaps snap, const NodeHandle target, const Vector2& size, const NodeFlags flags) {
    return Ui::snap(ui, layouter, snap, target, {}, size, flags);
}

Anchor snap(UserInterface& ui, const Snaps snap, const NodeHandle target, const Vector2& size, const NodeFlags flags) {
    return Ui::snap(ui, ui.snapLayouter(), snap, target, {}, size, flags);
}

Anchor snap(UserInterface& ui, SnapLayouter& layouter, const Snaps snap, const Vector2& offset, const Vector2& size, const NodeFlags flags) {
    return Ui::snap(ui, layouter, snap, NodeHandle::Null, offset, size, flags);
}

Anchor snap(UserInterface& ui, const Snaps snap, const Vector2& offset, const Vector2& size, const NodeFlags flags) {
    return Ui::snap(ui, ui.snapLayouter(), snap, NodeHandle::Null, offset, size, flags);
}

Anchor snap(UserInterface& ui, SnapLayouter& layouter, const Snaps snap, const Vector2& size, const NodeFlags flags) {
    return Ui::snap(ui, layouter, snap, {}, size, flags);
}

Anchor snap(UserInterface& ui, const Snaps snap, const Vector2& size, const NodeFlags flags) {
    return Ui::snap(ui, ui.snapLayouter(), snap, {}, size, flags);
}

}}
