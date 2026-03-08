/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023, 2024, 2025, 2026
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

Debug& operator<<(Debug& debug, const SnapLayoutFlag value) {
    debug << "Ui::SnapLayoutFlag" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case SnapLayoutFlag::value: return debug << "::" #value;
        _c(IgnoreOverflowX)
        _c(IgnoreOverflowY)
        _c(IgnoreOverflow)
        _c(PropagateMarginX)
        _c(PropagateMarginY)
        _c(PropagateMargin)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "(" << Debug::nospace << Debug::hex << UnsignedByte(value) << Debug::nospace << ")";
}

Debug& operator<<(Debug& debug, const SnapLayoutFlags value) {
    return Containers::enumSetDebugOutput(debug, value, "Ui::SnapLayoutFlags{}", {
        /* Combination of IgnoreOverflowX and IgnoreOverflowY, has to be first */
        SnapLayoutFlag::IgnoreOverflow,
        SnapLayoutFlag::IgnoreOverflowX,
        SnapLayoutFlag::IgnoreOverflowY,
        /* Combination of PropagateMarginX and PropagateMarginY, has to be first */
        SnapLayoutFlag::PropagateMargin,
        SnapLayoutFlag::PropagateMarginX,
        SnapLayoutFlag::PropagateMarginY,
    });
}

namespace {

/* There are no free bytes in the Layout struct to store these two bits so cram
   them into the SnapLayoutFlags. The SnapLayoutFlagMask is then used to mask
   them off in flags() and disallow them from being set in setFlags(),
   addFlags() and clearFlags(). */
constexpr SnapLayoutFlag SnapLayoutFlagHasExplicitSnap = SnapLayoutFlag(0x40);
/* SnapLayoutFlagExplicitSnapToParent defined in Implementation/snapLayouter.h
   as it's used by the helper utils as well */
constexpr SnapLayoutFlags SnapLayoutFlagMask = ~(SnapLayoutFlagHasExplicitSnap|Implementation::SnapLayoutFlagExplicitSnapToParent);

struct Layout {
    SnapLayoutFlags flags;
    /* used only if flags contain SnapLayoutFlagHasExplicitSnap */
    Snaps explicitSnap{NoInit};
    Snaps childSnap{NoInit}, firstChildSnap{NoInit};
    LayouterDataHandle firstChild;
    LayouterDataHandle firstExplicitSnap;
    LayouterDataHandle parentOrExplicitSnapTarget;
    LayouterDataHandle previous;
    LayouterDataHandle next;
};

}

struct SnapLayouter::State {
    Containers::Array<Layout> layouts;
    Vector2 uiSize;
};

SnapLayouter::SnapLayouter(const LayouterHandle handle): AbstractLayouter{handle}, _state{InPlaceInit} {}

SnapLayouter::SnapLayouter(SnapLayouter&&) noexcept = default;

SnapLayouter::~SnapLayouter() = default;

SnapLayouter& SnapLayouter::operator=(SnapLayouter&&) noexcept = default;

LayoutHandle SnapLayouter::add(const NodeHandle node, const LayoutHandle before, const SnapLayoutFlags flags) {
    CORRADE_ASSERT(before == LayoutHandle::Null || isHandleValid(before),
        "Ui::SnapLayouter::add(): invalid before handle" << before, {});
    return addInternal(node, layoutHandleData(before), flags);
}

LayoutHandle SnapLayouter::add(const NodeHandle node, const LayouterDataHandle before, const SnapLayoutFlags flags) {
    CORRADE_ASSERT(before == LayouterDataHandle::Null || isHandleValid(before),
        "Ui::SnapLayouter::add(): invalid before handle" << before, {});
    return addInternal(node, before, flags);
}

LayoutHandle SnapLayouter::addInternal(const NodeHandle node, const LayouterDataHandle before, const SnapLayoutFlags flags) {
    /* We're abusing the top two bits for distinguishing implicitly and
       explicitly snapped layouts, don't allow them to be set */
    CORRADE_ASSERT(!(flags & ~SnapLayoutFlagMask),
        "Ui::SnapLayouter::add(): invalid flags" << flags, {});
    CORRADE_ASSERT(
        !(flags >= (SnapLayoutFlag::IgnoreOverflowX|SnapLayoutFlag::PropagateMarginX)) &&
        !(flags >= (SnapLayoutFlag::IgnoreOverflowY|SnapLayoutFlag::PropagateMarginY)),
        "Ui::SnapLayouter::add():" << (flags & SnapLayoutFlag::IgnoreOverflow) << "and" << (flags & SnapLayoutFlag::PropagateMargin) << "are mutually exclusive", {});

    State& state = *_state;

    /* Validity of `node` is checked inside AbstractLayouter::add(), no need to
       repeat that here, however need to call that function first to not have
       it blow up elsewhere */
    const LayoutHandle handle = AbstractLayouter::add(node);
    const UnsignedInt id = layoutHandleId(handle);
    if(id >= state.layouts.size())
        arrayAppend(state.layouts, NoInit, id - state.layouts.size() + 1);

    Layout& layout = state.layouts[id];
    layout.flags = flags;
    layout.childSnap = Snap::Bottom;
    layout.firstChildSnap = Snap::Top|Snap::Inside;
    layout.firstChild = LayouterDataHandle::Null;
    layout.firstExplicitSnap = LayouterDataHandle::Null;

    /* If there's no parent node or the parent node has no layout, `before` can
       only be null and we don't insert the layout anywhere */
    const NodeHandle nodeParent = ui().nodeParent(node);
    const LayouterDataHandle parentHandle = nodeParent == NodeHandle::Null ?
        LayouterDataHandle::Null : ui().nodeUniqueLayout(nodeParent, *this);
    layout.parentOrExplicitSnapTarget = parentHandle;
    if(parentHandle == LayouterDataHandle::Null) {
        CORRADE_ASSERT(before == LayouterDataHandle::Null,
            "Ui::SnapLayouter::add(): expected before to be null for" << node << "without a parent layout but got" << before, {});

        layout.previous = LayouterDataHandle::Null;
        layout.next = LayouterDataHandle::Null;

    /* Otherwise it's expected to be either null or a sibling, and we insert it
       into the siblings linked list */
    } else {
        CORRADE_ASSERT(before == LayouterDataHandle::Null || ui().nodeParent(this->node(before)) == nodeParent,
            "Ui::SnapLayouter::add(): expected before to be a child of" << nodeParent << "but" << before << "is a child of" << ui().nodeParent(this->node(before)), {});

        Layout& parentLayout = state.layouts[layouterDataHandleId(parentHandle)];

        /* If there's no siblings so far, connect to itself as the list is
           cyclic */
        if(parentLayout.firstChild == LayouterDataHandle::Null) {
            layout.previous = layout.next = layoutHandleData(handle);

        /* Otherwise insert into the existing list */
        } else {
            /* The list is cyclic, so if we're inserting at the end, we're
               inserting before the parent's first child */
            const LayouterDataHandle nextHandle = before == LayouterDataHandle::Null ?
                parentLayout.firstChild : before;
            Layout& nextLayout = state.layouts[layouterDataHandleId(nextHandle)];
            const LayouterDataHandle previousHandle = nextLayout.previous;
            Layout& previousLayout = state.layouts[layouterDataHandleId(previousHandle)];
            previousLayout.next = layoutHandleData(handle);
            layout.previous = previousHandle;
            layout.next = nextHandle;
            nextLayout.previous = layoutHandleData(handle);
        }

        /* If we're adding at the front (or adding the first ever child, in
           which case both before and firstChild is null), update also the
           parent layout's first child */
        if(parentLayout.firstChild == before)
            parentLayout.firstChild = layoutHandleData(handle);
    }

    return handle;
}

LayoutHandle SnapLayouter::addExplicit(const NodeHandle node, const Snaps snap, const LayoutHandle target, const SnapLayoutFlags flags) {
    CORRADE_ASSERT(target == LayoutHandle::Null || isHandleValid(target),
        "Ui::SnapLayouter::addExplicit(): invalid target handle" << target, {});
    return addExplicitInternal(node, snap, layoutHandleData(target), flags);
}

LayoutHandle SnapLayouter::addExplicit(const NodeHandle node, const Snaps snap, const LayouterDataHandle target, const SnapLayoutFlags flags) {
    CORRADE_ASSERT(target == LayouterDataHandle::Null || isHandleValid(target),
        "Ui::SnapLayouter::addExplicit(): invalid target handle" << target, {});
    return addExplicitInternal(node, snap, target, flags);
}

LayoutHandle SnapLayouter::addExplicitInternal(const NodeHandle node, const Snaps snap, const LayouterDataHandle target, const SnapLayoutFlags flags) {
    /* We're abusing the top two bits for distinguishing implicitly and
       explicitly snapped layouts, don't allow them to be set */
    CORRADE_ASSERT(!(flags & ~SnapLayoutFlagMask),
        "Ui::SnapLayouter::addExplicit(): invalid flags" << flags, {});
    CORRADE_ASSERT(
        !(flags >= (SnapLayoutFlag::IgnoreOverflowX|SnapLayoutFlag::PropagateMarginX)) &&
        !(flags >= (SnapLayoutFlag::IgnoreOverflowY|SnapLayoutFlag::PropagateMarginY)),
        "Ui::SnapLayouter::addExplicit():" << (flags & SnapLayoutFlag::IgnoreOverflow) << "and" << (flags & SnapLayoutFlag::PropagateMargin) << "are mutually exclusive", {});

    State& state = *_state;

    /* Validity of `node` is checked inside AbstractLayouter::add(), no need to
       repeat that here, however need to call that function first to not have
       it blow up elsewhere */
    const LayoutHandle handle = AbstractLayouter::add(node);
    const UnsignedInt id = layoutHandleId(handle);
    if(id >= state.layouts.size())
        arrayAppend(state.layouts, NoInit, id - state.layouts.size() + 1);

    const NodeHandle nodeParent = ui().nodeParent(node);
    const LayouterDataHandle parentHandle = nodeParent == NodeHandle::Null ?
        LayouterDataHandle::Null : ui().nodeUniqueLayout(nodeParent, *this);

    /* If the target is null, it means snapping either to the parent node,
       which then has to have a layout, or to the UI itself, if node is root */
    CORRADE_ASSERT(target != LayouterDataHandle::Null || nodeParent == NodeHandle::Null,
        "Ui::SnapLayouter::addExplicit(): can't snap a non-root" << node << "to the whole UI", {});

    /* If the target isn't a parent, expect that it's a sibling and it's
       snapping outside. If the target is a parent, it's snapping inside
       implicitly. */
    /** @todo here's a chance that while target is valid, the node it's
        assigned to isn't anymore, and thus querying the parent blows up, what
        to do? */
    CORRADE_ASSERT(target == parentHandle || ui().nodeParent(this->node(target)) == nodeParent,
        "Ui::SnapLayouter::addExplicit(): expected target to be assigned to either" << nodeParent << "or a child of it but" << target << "is a child of" << ui().nodeParent(this->node(target)), {});

    Layout& layout = state.layouts[id];
    layout.flags = flags|SnapLayoutFlagHasExplicitSnap|(target == parentHandle ? Implementation::SnapLayoutFlagExplicitSnapToParent : SnapLayoutFlags{});
    layout.childSnap = Snap::Bottom;
    layout.firstChildSnap = Snap::Top|Snap::Inside;
    layout.firstChild = LayouterDataHandle::Null;
    layout.firstExplicitSnap = LayouterDataHandle::Null;
    layout.explicitSnap = snap;
    layout.parentOrExplicitSnapTarget = target;

    /* If the target is null, we don't insert the layout anywhere */
    layout.parentOrExplicitSnapTarget = target;
    if(target == LayouterDataHandle::Null) {
        layout.previous = LayouterDataHandle::Null;
        layout.next = LayouterDataHandle::Null;

    /* Otherwise we insert it at the end of the target linked list */
    } else {
        Layout& targetLayout = state.layouts[layouterDataHandleId(target)];

        /* If there are no explicit snaps so far, connect to itself as the list
           is cyclic, and update the first snap handle in the target */
        if(targetLayout.firstExplicitSnap == LayouterDataHandle::Null) {
            layout.previous = layout.next = layoutHandleData(handle);
            targetLayout.firstExplicitSnap = layoutHandleData(handle);

        /* Otherwise insert before the first child. Order doesn't matter, but
           this is the simplest and makes it last in the list. */
        } else {
            /* The list is cyclic, so if we're inserting at the end, we're
               inserting before the parent's first child */
            const LayouterDataHandle nextHandle = targetLayout.firstExplicitSnap;
            Layout& nextLayout = state.layouts[layouterDataHandleId(nextHandle)];
            const LayouterDataHandle previousHandle = nextLayout.previous;
            Layout& previousLayout = state.layouts[layouterDataHandleId(previousHandle)];
            previousLayout.next = layoutHandleData(handle);
            layout.previous = previousHandle;
            layout.next = nextHandle;
            nextLayout.previous = layoutHandleData(handle);
        }
    }

    return handle;
}

#ifndef CORRADE_NO_ASSERT
namespace {

UnsignedInt layoutChildCount(const Containers::ArrayView<const Layout> layouts, const LayouterDataHandle firstChild) {
    if(firstChild == LayouterDataHandle::Null)
        return 0;

    LayouterDataHandle layout = firstChild;
    UnsignedInt count = 0;
    do {
        ++count;
        layout = layouts[layouterDataHandleId(layout)].next;
    } while(layout != firstChild);

    return count;
}

}
#endif

void SnapLayouter::remove(const LayoutHandle handle) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::SnapLayouter::remove(): invalid handle" << handle, );
    #ifndef CORRADE_NO_ASSERT
    const State& state = *_state;
    const Layout& layout = state.layouts[layoutHandleId(handle)];
    CORRADE_ASSERT(layout.firstChild == LayouterDataHandle::Null && layout.firstExplicitSnap == LayouterDataHandle::Null,
        "Ui::SnapLayouter::remove(): cannot remove" << handle << "with" << layoutChildCount(state.layouts, layout.firstChild) << "children and" << layoutChildCount(state.layouts, layout.firstExplicitSnap) << "dependent layouts", );
    #endif
    AbstractLayouter::remove(handle);
    removeInternal(layoutHandleId(handle));
}

void SnapLayouter::remove(const LayouterDataHandle handle) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::SnapLayouter::remove(): invalid handle" << handle, );
    #ifndef CORRADE_NO_ASSERT
    const State& state = *_state;
    const Layout& layout = state.layouts[layouterDataHandleId(handle)];
    CORRADE_ASSERT(layout.firstChild == LayouterDataHandle::Null && layout.firstExplicitSnap == LayouterDataHandle::Null,
        "Ui::SnapLayouter::remove(): cannot remove" << handle << "with" << layoutChildCount(state.layouts, layout.firstChild) << "children and" << layoutChildCount(state.layouts, layout.firstExplicitSnap) << "dependent layouts", );
    #endif
    AbstractLayouter::remove(handle);
    removeInternal(layouterDataHandleId(handle));
}

void SnapLayouter::removeInternal(const UnsignedInt id) {
    State& state = *_state;

    Layout& layout = state.layouts[id];

    /* If the layout has a parent or is explicitly snapped to a target node,
       disconnect it from the corresponding linked list */
    if(layout.parentOrExplicitSnapTarget != LayouterDataHandle::Null) {
        /* The list is cyclic, so if it's just two items left, the remaining
           one is connected to itself. If it's just one item left, the
           prev/next handle points to itself already and will stay so. */
        const LayouterDataHandle previousHandle = state.layouts[id].previous;
        Layout& previousLayout = state.layouts[layouterDataHandleId(previousHandle)];
        const LayouterDataHandle nextHandle = state.layouts[id].next;
        Layout& nextLayout = state.layouts[layouterDataHandleId(nextHandle)];
        previousLayout.next = nextHandle;
        nextLayout.previous = previousHandle;

        /* If this was the first implicitly snapped child in the parent / first
           explicitly snapped node to the target, update the parent / target to
           point to the next instead, if any, or null if this is the last. */
        Layout& parentOrExplicitSnapTargetLayout = state.layouts[layouterDataHandleId(layout.parentOrExplicitSnapTarget)];
        LayouterDataHandle& firstChildOrExplicitSnap = layout.flags >= SnapLayoutFlagHasExplicitSnap ?
            parentOrExplicitSnapTargetLayout.firstExplicitSnap :
            parentOrExplicitSnapTargetLayout.firstChild;
        if(layouterDataHandleId(firstChildOrExplicitSnap) == id)
            firstChildOrExplicitSnap = id == layouterDataHandleId(nextHandle) ?
                LayouterDataHandle::Null : nextHandle;

    /* If it isn't, the prev/next handles should be null */
    } else CORRADE_INTERNAL_DEBUG_ASSERT(layout.previous == LayouterDataHandle::Null && layout.next == LayouterDataHandle::Null);

    /* Set the `parentOrExplicitSnapTarget` and `next` fields to null so
       Implementation::orderLayoutsBreadthFirstInto() treats the removed
       entries as root layouts. Do *not* clear the `firstChild` and
       `firstExplicitSnap` fields however, as doing so would cause an assert in
       layouterDataHandleId() above when removeInternal() gets called for the
       child / dependent layouts after being called for the parent / target
       layout first. These fields will nevertheless get correctly cleared when
       the last child / dependent layout gets removed.

       Finally, the `previous` field isn't used by the
       orderLayoutsBreadthFirstInto() helper, so it doesn't need to be
       cleared. */
    layout.parentOrExplicitSnapTarget = LayouterDataHandle::Null;
    layout.next = LayouterDataHandle::Null;
}

bool SnapLayouter::hasExplicitSnap(const LayoutHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::SnapLayouter::hasExplicitSnap(): invalid handle" << handle, {});
    return _state->layouts[layoutHandleId(handle)].flags >= SnapLayoutFlagHasExplicitSnap;
}

bool SnapLayouter::hasExplicitSnap(const LayouterDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::SnapLayouter::hasExplicitSnap(): invalid handle" << handle, {});
    return _state->layouts[layouterDataHandleId(handle)].flags >= SnapLayoutFlagHasExplicitSnap;
}

SnapLayoutFlags SnapLayouter::flags(const LayoutHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::SnapLayouter::flags(): invalid handle" << handle, {});
    return flagsInternal(layoutHandleId(handle));
}

SnapLayoutFlags SnapLayouter::flags(const LayouterDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::SnapLayouter::flags(): invalid handle" << handle, {});
    return flagsInternal(layouterDataHandleId(handle));
}

SnapLayoutFlags SnapLayouter::flagsInternal(const UnsignedInt id) const {
    return _state->layouts[id].flags & SnapLayoutFlagMask;
}

void SnapLayouter::setFlags(const LayoutHandle handle, const SnapLayoutFlags flags) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::SnapLayouter::setFlags(): invalid handle" << handle, );
    setFlagsInternal(layoutHandleId(handle), flags);
}

void SnapLayouter::setFlags(const LayouterDataHandle handle, const SnapLayoutFlags flags) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::SnapLayouter::setFlags(): invalid handle" << handle, );
    setFlagsInternal(layouterDataHandleId(handle), flags);
}

void SnapLayouter::setFlagsInternal(const UnsignedInt id, const SnapLayoutFlags flags) {
    CORRADE_ASSERT(!(flags & ~SnapLayoutFlagMask),
        "Ui::SnapLayouter::setFlags(): invalid flags" << flags, );
    Layout& layout = _state->layouts[id];
    CORRADE_ASSERT(
        !(flags >= (SnapLayoutFlag::IgnoreOverflowX|SnapLayoutFlag::PropagateMarginX)) &&
        !(flags >= (SnapLayoutFlag::IgnoreOverflowY|SnapLayoutFlag::PropagateMarginY)),
        "Ui::SnapLayouter::setFlags():" << (flags & SnapLayoutFlag::IgnoreOverflow) << "and" << (flags & SnapLayoutFlag::PropagateMargin) << "are mutually exclusive", );
    layout.flags = (layout.flags & ~SnapLayoutFlagMask)|flags;
    setNeedsUpdate();
}

void SnapLayouter::addFlags(const LayoutHandle handle, const SnapLayoutFlags flags) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::SnapLayouter::addFlags(): invalid handle" << handle, );
    addFlagsInternal(layoutHandleId(handle), flags);
}

void SnapLayouter::addFlags(const LayouterDataHandle handle, const SnapLayoutFlags flags) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::SnapLayouter::addFlags(): invalid handle" << handle, );
    addFlagsInternal(layouterDataHandleId(handle), flags);
}

void SnapLayouter::addFlagsInternal(const UnsignedInt id, const SnapLayoutFlags flags) {
    CORRADE_ASSERT(!(flags & ~SnapLayoutFlagMask),
        "Ui::SnapLayouter::addFlags(): invalid flags" << flags, );
    Layout& layout = _state->layouts[id];
    CORRADE_ASSERT(
        !((layout.flags|flags) >= (SnapLayoutFlag::IgnoreOverflowX|SnapLayoutFlag::PropagateMarginX)) &&
        !((layout.flags|flags) >= (SnapLayoutFlag::IgnoreOverflowY|SnapLayoutFlag::PropagateMarginY)),
        "Ui::SnapLayouter::addFlags():" << ((layout.flags|flags) & SnapLayoutFlag::IgnoreOverflow) << "and" << ((layout.flags|flags) & SnapLayoutFlag::PropagateMargin) << "are mutually exclusive", );
    /* Not delegating to setFlagsInternal() because this is a simpler operation
       than preserving certain bits while replacing the rest */
    layout.flags |= flags;
    setNeedsUpdate();
}

void SnapLayouter::clearFlags(const LayoutHandle handle, const SnapLayoutFlags flags) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::SnapLayouter::clearFlags(): invalid handle" << handle, );
    clearFlagsInternal(layoutHandleId(handle), flags);
}

void SnapLayouter::clearFlags(const LayouterDataHandle handle, const SnapLayoutFlags flags) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::SnapLayouter::clearFlags(): invalid handle" << handle, );
    clearFlagsInternal(layouterDataHandleId(handle), flags);
}

void SnapLayouter::clearFlagsInternal(const UnsignedInt id, const SnapLayoutFlags flags) {
    CORRADE_ASSERT(!(flags & ~SnapLayoutFlagMask),
        "Ui::SnapLayouter::clearFlags(): invalid flags" << flags, );
    Layout& layout = _state->layouts[id];
    /* Not delegating to setFlagsInternal() because this is a simpler operation
       than preserving certain bits while replacing the rest */
    layout.flags &= ~flags;
    setNeedsUpdate();
}

Snaps SnapLayouter::explicitSnap(const LayoutHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::SnapLayouter::explicitSnap(): invalid handle" << handle, {});
    const Layout& layout = _state->layouts[layoutHandleId(handle)];
    CORRADE_ASSERT(layout.flags >= SnapLayoutFlagHasExplicitSnap,
        "Ui::SnapLayouter::explicitSnap():" << handle << "doesn't have an explicit snap", {});
    return layout.explicitSnap;
}

Snaps SnapLayouter::explicitSnap(const LayouterDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::SnapLayouter::explicitSnap(): invalid handle" << handle, {});
    const Layout& layout = _state->layouts[layouterDataHandleId(handle)];
    CORRADE_ASSERT(layout.flags >= SnapLayoutFlagHasExplicitSnap,
        "Ui::SnapLayouter::explicitSnap():" << handle << "doesn't have an explicit snap", {});
    return layout.explicitSnap;
}

Snaps SnapLayouter::childSnap(const LayoutHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::SnapLayouter::childSnap(): invalid handle" << handle, {});
    return _state->layouts[layoutHandleId(handle)].childSnap;
}

Snaps SnapLayouter::childSnap(const LayouterDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::SnapLayouter::childSnap(): invalid handle" << handle, {});
    return _state->layouts[layouterDataHandleId(handle)].childSnap;
}

void SnapLayouter::setChildSnap(const LayoutHandle handle, const Snaps snap) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::SnapLayouter::setChildSnap(): invalid handle" << handle, );
    setChildSnapInternal(layoutHandleId(handle), snap);
}

void SnapLayouter::setChildSnap(const LayouterDataHandle handle, const Snaps snap) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::SnapLayouter::setChildSnap(): invalid handle" << handle, );
    setChildSnapInternal(layouterDataHandleId(handle), snap);
}

void SnapLayouter::setChildSnapInternal(const UnsignedInt id, const Snaps snap) {
    Layout& layout = _state->layouts[id];
    layout.childSnap = snap;
    layout.firstChildSnap = Implementation::firstChildSnap(snap);
    CORRADE_ASSERT(layout.firstChildSnap,
        "Ui::SnapLayouter::setChildSnap():" << snap << "doesn't produce a non-overlapping purely horizontal or vertical order", );
    setNeedsUpdate();
}

LayoutHandle SnapLayouter::parent(const LayoutHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::SnapLayouter::parent(): invalid handle" << handle, {});
    const Layout& layout = _state->layouts[layoutHandleId(handle)];
    CORRADE_ASSERT(!(layout.flags >= SnapLayoutFlagHasExplicitSnap),
        "Ui::SnapLayouter::parent():" << handle << "has an explicit snap", {});
    const LayouterDataHandle parent = layout.parentOrExplicitSnapTarget;
    return parent == LayouterDataHandle::Null ?
        LayoutHandle::Null : layoutHandle(this->handle(), parent);
}

LayoutHandle SnapLayouter::parent(const LayouterDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::SnapLayouter::parent(): invalid handle" << handle, {});
    const Layout& layout = _state->layouts[layouterDataHandleId(handle)];
    CORRADE_ASSERT(!(layout.flags >= SnapLayoutFlagHasExplicitSnap),
        "Ui::SnapLayouter::parent():" << handle << "has an explicit snap", {});
    const LayouterDataHandle parent = layout.parentOrExplicitSnapTarget;
    return parent == LayouterDataHandle::Null ?
        LayoutHandle::Null : layoutHandle(this->handle(), parent);
}

LayoutHandle SnapLayouter::explicitSnapTarget(const LayoutHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::SnapLayouter::explicitSnapTarget(): invalid handle" << handle, {});
    const Layout& layout = _state->layouts[layoutHandleId(handle)];
    CORRADE_ASSERT(layout.flags >= SnapLayoutFlagHasExplicitSnap,
        "Ui::SnapLayouter::explicitSnapTarget():" << handle << "doesn't have an explicit snap", {});
    const LayouterDataHandle target = layout.parentOrExplicitSnapTarget;
    return target == LayouterDataHandle::Null ?
        LayoutHandle::Null : layoutHandle(this->handle(), target);
}

LayoutHandle SnapLayouter::explicitSnapTarget(const LayouterDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::SnapLayouter::explicitSnapTarget(): invalid handle" << handle, {});
    const Layout& layout = _state->layouts[layouterDataHandleId(handle)];
    CORRADE_ASSERT(layout.flags >= SnapLayoutFlagHasExplicitSnap,
        "Ui::SnapLayouter::explicitSnapTarget():" << handle << "doesn't have an explicit snap", {});
    const LayouterDataHandle target = layout.parentOrExplicitSnapTarget;
    return target == LayouterDataHandle::Null ?
        LayoutHandle::Null : layoutHandle(this->handle(), target);
}

LayoutHandle SnapLayouter::firstChild(const LayoutHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::SnapLayouter::firstChild(): invalid handle" << handle, {});
    const LayouterDataHandle firstChild = _state->layouts[layoutHandleId(handle)].firstChild;
    return firstChild == LayouterDataHandle::Null ?
        LayoutHandle::Null : layoutHandle(this->handle(), firstChild);
}

LayoutHandle SnapLayouter::firstChild(const LayouterDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::SnapLayouter::firstChild(): invalid handle" << handle, {});
    const LayouterDataHandle firstChild = _state->layouts[layouterDataHandleId(handle)].firstChild;
    return firstChild == LayouterDataHandle::Null ?
        LayoutHandle::Null : layoutHandle(this->handle(), firstChild);
}

LayoutHandle SnapLayouter::firstExplicitSnap(const LayoutHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::SnapLayouter::firstExplicitSnap(): invalid handle" << handle, {});
    const LayouterDataHandle firstExplicitSnap = _state->layouts[layoutHandleId(handle)].firstExplicitSnap;
    return firstExplicitSnap == LayouterDataHandle::Null ?
        LayoutHandle::Null : layoutHandle(this->handle(), firstExplicitSnap);
}

LayoutHandle SnapLayouter::firstExplicitSnap(const LayouterDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::SnapLayouter::firstExplicitSnap(): invalid handle" << handle, {});
    const LayouterDataHandle firstExplicitSnap = _state->layouts[layouterDataHandleId(handle)].firstExplicitSnap;
    return firstExplicitSnap == LayouterDataHandle::Null ?
        LayoutHandle::Null : layoutHandle(this->handle(), firstExplicitSnap);
}

LayoutHandle SnapLayouter::previous(const LayoutHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::SnapLayouter::previous(): invalid handle" << handle, {});
    return previousInternal(layoutHandleData(handle));
}

LayoutHandle SnapLayouter::previous(const LayouterDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::SnapLayouter::previous(): invalid handle" << handle, {});
    return previousInternal(handle);
}

LayoutHandle SnapLayouter::previousInternal(const LayouterDataHandle handle) const {
    const State& state = *_state;
    const Layout& layout = state.layouts[layouterDataHandleId(handle)];
    /* The list is cyclic, so the handle is never null */
    const LayouterDataHandle previous = layout.previous;

    /* Return the previous handle only if there's a parent / target layout and
       the handle is not the same as the first child of parent / first explicit
       snap node of the target, i.e. the handle is first in the list */
    const LayouterDataHandle parentOrExplicitSnapTarget = layout.parentOrExplicitSnapTarget;
    if(parentOrExplicitSnapTarget != LayouterDataHandle::Null) {
        const Layout& parentOrExplicitSnapTargetLayout = state.layouts[layouterDataHandleId(parentOrExplicitSnapTarget)];
        if((!(layout.flags >= SnapLayoutFlagHasExplicitSnap) && parentOrExplicitSnapTargetLayout.firstChild != handle) ||
           (layout.flags >= SnapLayoutFlagHasExplicitSnap && parentOrExplicitSnapTargetLayout.firstExplicitSnap != handle))
            return layoutHandle(this->handle(), previous);
    }

    /* In case there's no parent or target (and thus there's no list), or the
       handle is first in the list, return null */
    return LayoutHandle::Null;
}

LayoutHandle SnapLayouter::next(const LayoutHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::SnapLayouter::next(): invalid handle" << handle, {});
    return nextInternal(layoutHandleData(handle));
}

LayoutHandle SnapLayouter::next(const LayouterDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::SnapLayouter::next(): invalid handle" << handle, {});
    return nextInternal(handle);
}

LayoutHandle SnapLayouter::nextInternal(const LayouterDataHandle handle) const {
    const State& state = *_state;
    const Layout& layout = _state->layouts[layouterDataHandleId(handle)];
    /* The list is cyclic, so the handle is never null */
    const LayouterDataHandle next = layout.next;

    /* Return the next handle only if there's a parent / target layout and the
       next handle is not the same as the first child of parent / first
       explicit snap node of the target, i.e. the handle is last in the list */
    const LayouterDataHandle parentOrExplicitSnapTarget = layout.parentOrExplicitSnapTarget;
    if(parentOrExplicitSnapTarget != LayouterDataHandle::Null) {
        const Layout& parentOrExplicitSnapTargetLayout = state.layouts[layouterDataHandleId(parentOrExplicitSnapTarget)];
        if((!(layout.flags >= SnapLayoutFlagHasExplicitSnap) && parentOrExplicitSnapTargetLayout.firstChild != next) ||
           (layout.flags >= SnapLayoutFlagHasExplicitSnap && parentOrExplicitSnapTargetLayout.firstExplicitSnap != next))
            return layoutHandle(this->handle(), next);
    }

    /* In case there's no parent or target (and thus there's no list), or the
       handle is last in the list, return null */
    return LayoutHandle::Null;
}

LayouterFeatures SnapLayouter::doFeatures() const {
    return LayouterFeature::UniqueLayouts;
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

void SnapLayouter::doClean(const Containers::BitArrayView layoutIdsToRemove) {
    #ifndef CORRADE_NO_ASSERT
    const State& state = *_state;
    #endif

    /** @todo some way to iterate bits */
    for(std::size_t i = 0; i != layoutIdsToRemove.size(); ++i) {
        if(!layoutIdsToRemove[i])
            continue;

        #ifndef CORRADE_NO_ASSERT
        /* At the moment it's an error to remove layouts with explicit snaps
           remaining. Go through them and check that they're being removed as
           well. This is not a problem with implicit children, as anything
           attached to child nodes gets removed along with the parent as well
           when clean() is called. The explicit snaps can be children too (and
           thus could be skipped in this check), but it's simpler to just check
           all. */
        const LayouterDataHandle firstExplicitSnap = state.layouts[i].firstExplicitSnap;
        if(firstExplicitSnap != LayouterDataHandle::Null) {
            LayouterDataHandle layout = firstExplicitSnap;
            UnsignedInt count = 0;
            do {
                const UnsignedInt id = layouterDataHandleId(layout);
                if(!layoutIdsToRemove[id])
                    ++count;
                layout = state.layouts[id].next;
            } while(layout != firstExplicitSnap);

            /* The generation is already bumped at this point, so subtract 1 to
               correctly show the pre-removed handle */
            CORRADE_ASSERT(!count,
                "Ui::SnapLayouter::clean(): cannot remove" << layouterDataHandle(i, generations()[i] - 1) << "that still has" << count << "dependent layouts", );
        }
        #endif

        removeInternal(i);
    }
}

void SnapLayouter::doLayout(const Containers::BitArrayView layoutIdsToUpdate, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<Vector2>& nodeMinSizes, const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Float>&, const Containers::StridedArrayView1D<Vector4>& nodePaddings, const Containers::StridedArrayView1D<Vector4>& nodeMargins, const Containers::StridedArrayView1D<Vector2>& nodeOffsets, const Containers::StridedArrayView1D<Vector2>& nodeSizes) {
    const State& state = *_state;

    /* Order layouts breadth first in dependency order to ensure the parent /
       target layout offset / size is known when calculating child / dependent
       layout */
    /** @todo It might be beneficial to split this function into update() +
        layout(), where the former gets the *full* mask of all layouts and can
        perform this ordering just once, not for every call */
    Containers::ArrayView<UnsignedInt> childrenOffsets;
    Containers::ArrayView<UnsignedInt> children;
    Containers::ArrayView<Int> layoutIds;
    /** @todo the childLayoutPaddings are used only once children and
        childrenOffsets are not needed anymore, make use of that once a bump
        allocator is a thing */
    Containers::ArrayView<Vector4> childLayoutPaddings;
    Containers::ArrayTuple storage{
        /* +1 for the last offset, +1 for root nodes */
        {ValueInit, state.layouts.size() + 2, childrenOffsets},
        {NoInit, state.layouts.size(), children},
        /* +1 for the first element which is -1 indicating a root */
        {NoInit, state.layouts.size() + 1, layoutIds},
        {NoInit, state.layouts.size(), childLayoutPaddings},
    };
    Implementation::orderLayoutsBreadthFirstInto(
        stridedArrayView(state.layouts).slice(&Layout::parentOrExplicitSnapTarget),
        stridedArrayView(state.layouts).slice(&Layout::firstChild),
        stridedArrayView(state.layouts).slice(&Layout::firstExplicitSnap),
        stridedArrayView(state.layouts).slice(&Layout::next),
        childrenOffsets,
        children,
        layoutIds);

    const Containers::StridedArrayView1D<const NodeHandle> nodes = this->nodes();

    /* Go through the layouts in their *reverse* dependency order, skipping
       also the first item which was -1, for each calculate a max of its size,
       min size and size of all implicitly snapped children including relevant
       paddings and margins */
    for(std::size_t i = layoutIds.size(); i != 1; --i) {
        const UnsignedInt layoutId = layoutIds[i - 1];
        const Layout& layout = state.layouts[layoutId];

        /* If there are no children and no explicitly snapped children either,
           nothing to calculate */
        if(layout.firstChild == LayouterDataHandle::Null &&
           layout.firstExplicitSnap == LayouterDataHandle::Null)
            continue;

        /* If the layout has no children or ignores overflow in both
           directions, we can't / don't need to calculate the child size and
           padding at all, as it won't get used anyway. */
        const UnsignedInt nodeId = nodeHandleId(nodes[layoutId]);
        const Containers::Pair<Vector2, Vector4> childLayoutSizeMargin =
            layout.firstChild == LayouterDataHandle::Null ||
            layout.flags >= SnapLayoutFlag::IgnoreOverflow ?
                Containers::Pair<Vector2, Vector4>{} :
                Implementation::childLayoutSizeMargin(
                    layout.childSnap,
                    nodeMinSizes,
                    nodeMargins,
                    nodeSizes,
                    layout.firstChild,
                    nodes,
                    stridedArrayView(state.layouts).slice(&Layout::next));

        /* Calculate the actual layout size, padding and margin from the layout
           properties and child layout size and margin (optionally) calculated
           above */
        const Containers::Triple<Vector2, Vector4, Vector4> layoutSizePaddingMargin =
            Implementation::layoutSizePaddingMargin(
                layout.flags,
                layout.childSnap,
                nodeSizes[nodeId],
                nodePaddings[nodeId],
                nodeMargins[nodeId],
                childLayoutSizeMargin.first(),
                childLayoutSizeMargin.second());

        /* Consider also explicitly snapped child nodes for the size, again not
           even call the function if there are no explicitly snapped children
           or overflow is ignored in both directions. Their margin currently
           isn't considered for propagation, it's really just the sizes with
           the max of margin and padding used. */
        const Vector2 explicitlySnappedChildLayoutSize =
            layout.firstExplicitSnap == LayouterDataHandle::Null ||
            layout.flags >= SnapLayoutFlag::IgnoreOverflow ?
                Vector2{} :
                Implementation::explicitlySnappedChildLayoutSize(
                    layout.flags,
                    nodePaddings[nodeId],
                    nodeMinSizes,
                    nodeMargins,
                    nodeSizes,
                    layout.firstExplicitSnap,
                    nodes,
                    stridedArrayView(state.layouts).slice(&Layout::flags),
                    stridedArrayView(state.layouts).slice(&Layout::explicitSnap),
                    stridedArrayView(state.layouts).slice(&Layout::next));

        nodeSizes[nodeId] = Math::max(layoutSizePaddingMargin.first(), explicitlySnappedChildLayoutSize);
        childLayoutPaddings[layoutId] = layoutSizePaddingMargin.second();
        nodeMargins[nodeId] = layoutSizePaddingMargin.third();
    }

    /* Go through the layouts in their dependency order, skipping the first
       item which was -1, and snap them to final positions */
    for(const UnsignedInt layoutId: layoutIds.exceptPrefix(1)) {
        /** @todo some way to iterate set bits */
        if(!layoutIdsToUpdate[layoutId])
            continue;

        const Layout& layout = state.layouts[layoutId];
        const UnsignedInt nodeId = nodeHandleId(nodes[layoutId]);

        /* If we're a parentless node without an explicit snap, nothing to do,
           everything is done by the dependent layouts */
        if(!(layout.flags >= SnapLayoutFlagHasExplicitSnap) && layout.parentOrExplicitSnapTarget == LayouterDataHandle::Null)
            continue;

        /* If we're implicitly snapping to the previous sibling, figure out the
           snap target and parameters */
        Snaps snap{NoInit};
        Vector2 targetOffset{NoInit}, targetSize{NoInit};
        /* GCC is overly "helpful" and warns that targetPadding / targetMargin
           might be used uninitialized in some cases in Release builds. It
           isn't, and in Debug builds I'm poisoning the values with NaNs below
           to be sure they aren't. Thus zero-init on Release to silence the
           damn thing, even though it's completely unnecessary. */
        #ifdef CORRADE_IS_DEBUG_BUILD
        Vector4 targetPadding{NoInit}, targetMargin{NoInit};
        #else
        Vector4 targetPadding{Math::ZeroInit}, targetMargin{Math::ZeroInit};
        #endif
        Vector4 nodeMargin{NoInit};
        if(!(layout.flags >= SnapLayoutFlagHasExplicitSnap)) {
            /* If this is the first child layout in the list, snap to the
               parent */
            const UnsignedInt parentLayoutId = layouterDataHandleId(layout.parentOrExplicitSnapTarget);
            const Layout& parentLayout = state.layouts[parentLayoutId];
            if(layouterDataHandleId(parentLayout.firstChild) == layoutId) {
                snap = parentLayout.firstChildSnap;
                const UnsignedInt parentNodeId = nodeHandleId(nodes[parentLayoutId]);
                /* We're snapping to a parent, so its offset isn't included */
                targetOffset = {};
                targetSize = nodeSizes[parentNodeId];
                /* Padding is overriden from nodePaddings[parentNodeId] to have
                   all child layouts align properly */
                targetPadding = childLayoutPaddings[parentLayoutId];
                /* Margin isn't used becase we're snapping inside, poison it
                   with NaNs on debug builds and leave uninitialized
                   otherwise */
                #ifdef CORRADE_IS_DEBUG_BUILD
                targetMargin = Vector4{Constants::nan()};
                #endif

                /* If propagating margin in either direction, don't use it for
                   positioning of the first node */
                Math::scatterInto<0, 2>(nodeMargin,
                    parentLayout.flags >= SnapLayoutFlag::PropagateMarginX ?
                        Vector2{} :
                        Math::gather<0, 2>(nodeMargins[nodeId]));
                Math::scatterInto<1, 3>(nodeMargin,
                    parentLayout.flags >= SnapLayoutFlag::PropagateMarginY ?
                        Vector2{} :
                        Math::gather<1, 3>(nodeMargins[nodeId]));

            /* Otherwise snap to the previous node */
            } else {
                snap = parentLayout.childSnap;
                const UnsignedInt previousNodeId = nodeHandleId(nodes[layouterDataHandleId(layout.previous)]);
                /* We're snapping to a sibling, so its offset is included */
                targetOffset = nodeOffsets[previousNodeId];
                targetSize = nodeSizes[previousNodeId];
                /* Padding isn't used because we're snapping outside, poison it
                   with NaNs on debug builds and leave uninitialized
                   otherwise */
                #ifdef CORRADE_IS_DEBUG_BUILD
                targetPadding = Vector4{Constants::nan()};
                #endif
                targetMargin = nodeMargins[previousNodeId];
                nodeMargin = nodeMargins[nodeId];
            }

        /* Otherwise we're snapping to an explicit target. If the target is
           null, we're snapping to the whole UI. */
        } else if(layout.parentOrExplicitSnapTarget == LayouterDataHandle::Null) {
            snap = layout.explicitSnap|Snap::Inside;
            targetOffset = {};
            targetSize = state.uiSize;

            /* The whole UI on its own doesn't have any padding defined, only
               margin of root nodes is used. */
            targetPadding = {};
            /* Margin shouldn't get used for anything, poison it with NaNs on
               debug builds and leave uninitialized otherwise */
            #ifdef CORRADE_IS_DEBUG_BUILD
            targetMargin = Vector4{Constants::nan()};
            #endif
            nodeMargin = nodeMargins[nodeId];

        /* Otherwise we're snapping relative to the target node, which should
           have the layout already calculated at this point thanks to the
           dependency ordering */
        } else {
            const NodeHandle targetNode = nodes[layouterDataHandleId(layout.parentOrExplicitSnapTarget)];
            const UnsignedInt targetNodeId = nodeHandleId(targetNode);
            snap = layout.explicitSnap;
            targetSize = nodeSizes[targetNodeId];

            /* If the target is a parent, don't include its offset in the
               calculation, and implicitly snap inside */
            if(layout.flags >= Implementation::SnapLayoutFlagExplicitSnapToParent) {
                targetOffset = {};
                snap |= Snap::Inside;

            /* Otherwise the nodes are siblings, in which case do include it.
               The snap is taken as-is, if it contains Snap::Inside then it's
               on the user to ensure it isn't prone to draw/event ordering
               issues. */
            } else targetOffset = nodeOffsets[targetNodeId];

            targetPadding = nodePaddings[targetNodeId];
            targetMargin = nodeMargins[targetNodeId];
            nodeMargin = nodeMargins[nodeId];
        }

        const Containers::Pair<Vector2, Vector2> out = Implementation::snap(snap,
            targetOffset, targetSize,
            targetPadding, targetMargin, nodeMargin,
            Math::max(nodeSizes[nodeId], nodeMinSizes[nodeId]));

        /* The original node offset is added to the calculated layout, size
           may be (partially) replaced */
        nodeOffsets[nodeId] += out.first();
        nodeSizes[nodeId] = out.second();
    }
}

}}
