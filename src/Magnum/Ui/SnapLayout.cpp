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

#include "SnapLayout.h"
#include "SnapLayout.hpp"

#include "Magnum/Ui/AbstractUserInterface.h"
#include "Magnum/Ui/Anchor.h"
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/SnapLayouter.h"
#include "Magnum/Ui/UserInterface.h"

namespace Magnum { namespace Ui {

AbstractSnapLayout AbstractSnapLayout::root(AbstractUserInterface& ui, SnapLayouter& layouter, const Snaps snap, const Vector2& nodeSize, const NodeFlags nodeFlags, const SnapLayoutFlags layoutFlags) {
    CORRADE_ASSERT(ui.isHandleValid(layouter.handle()) && &ui.layouter(layouter.handle()) == &layouter,
        "Ui::AbstractSnapLayout::root(): layouter not part of the UI",
        (AbstractSnapLayout{ui, &layouter, {}, {}}));
    const NodeHandle root = ui.createNode({}, nodeSize, nodeFlags);
    const LayoutHandle layout = layouter.addExplicit(root, snap, LayoutHandle::Null, layoutFlags);
    return AbstractSnapLayout{ui, &layouter, root, layoutHandleData(layout)};
}

AbstractSnapLayout::AbstractSnapLayout(NoInitT, AbstractUserInterface& ui, const NodeHandle node): _ui{&ui}, _node{node} {}

AbstractSnapLayout::AbstractSnapLayout(AbstractUserInterface& ui, SnapLayouter& layouter, const NodeHandle node): AbstractSnapLayout{NoInit, ui, node} {
    CORRADE_ASSERT(ui.isHandleValid(layouter.handle()) && &ui.layouter(layouter.handle()) == &layouter,
        "Ui::AbstractSnapLayout: layouter not part of the UI", );
    addLayout(layouter);
}

AbstractSnapLayout::AbstractSnapLayout(SnapLayouter& layouter, const AbstractAnchor& anchor): AbstractSnapLayout{NoInit, anchor.ui(), anchor.node()} {
    CORRADE_ASSERT(_ui->isHandleValid(layouter.handle()) && &_ui->layouter(layouter.handle()) == &layouter,
        "Ui::AbstractSnapLayout: layouter and anchor not part of the same UI", );
    addLayout(layouter);
}

AbstractSnapLayout::AbstractSnapLayout(AbstractUserInterface& ui, SnapLayouter& layouter, const NodeHandle node, const LayoutHandle before, const SnapLayoutFlags flags): AbstractSnapLayout{NoInit, ui, node} {
    CORRADE_ASSERT(ui.isHandleValid(layouter.handle()) && &ui.layouter(layouter.handle()) == &layouter,
        "Ui::AbstractSnapLayout: layouter not part of the UI", );
    addLayout(layouter, before, flags);
}

AbstractSnapLayout::AbstractSnapLayout(SnapLayouter& layouter, const AbstractAnchor& anchor, const LayoutHandle before, const SnapLayoutFlags flags): AbstractSnapLayout{NoInit, anchor.ui(), anchor.node()} {
    CORRADE_ASSERT(_ui->isHandleValid(layouter.handle()) && &_ui->layouter(layouter.handle()) == &layouter,
        "Ui::AbstractSnapLayout: layouter and anchor not part of the same UI", );
    addLayout(layouter, before, flags);
}

AbstractSnapLayout::AbstractSnapLayout(AbstractUserInterface& ui, SnapLayouter& layouter, const NodeHandle node, const LayouterDataHandle before, const SnapLayoutFlags flags): AbstractSnapLayout{NoInit, ui, node} {
    CORRADE_ASSERT(ui.isHandleValid(layouter.handle()) && &ui.layouter(layouter.handle()) == &layouter,
        "Ui::AbstractSnapLayout: layouter not part of the UI", );
    addLayout(layouter, before, flags);
}

AbstractSnapLayout::AbstractSnapLayout(SnapLayouter& layouter, const AbstractAnchor& anchor, const LayouterDataHandle before, const SnapLayoutFlags flags): AbstractSnapLayout{NoInit, anchor.ui(), anchor.node()} {
    CORRADE_ASSERT(_ui->isHandleValid(layouter.handle()) && &_ui->layouter(layouter.handle()) == &layouter,
        "Ui::AbstractSnapLayout: layouter and anchor not part of the same UI", );
    addLayout(layouter, before, flags);
}

AbstractSnapLayout::AbstractSnapLayout(AbstractUserInterface& ui, SnapLayouter& layouter, const NodeHandle node, const Snaps snap, const LayoutHandle snapTarget, const SnapLayoutFlags flags): AbstractSnapLayout{NoInit, ui, node} {
    CORRADE_ASSERT(ui.isHandleValid(layouter.handle()) && &ui.layouter(layouter.handle()) == &layouter,
        "Ui::AbstractSnapLayout: layouter not part of the UI", );
    addExplicitLayout(layouter, snap, snapTarget, flags);
}

AbstractSnapLayout::AbstractSnapLayout(SnapLayouter& layouter, const AbstractAnchor& anchor, const Snaps snap, const LayoutHandle snapTarget, const SnapLayoutFlags flags): AbstractSnapLayout{NoInit, anchor.ui(), anchor.node()} {
    CORRADE_ASSERT(_ui->isHandleValid(layouter.handle()) && &_ui->layouter(layouter.handle()) == &layouter,
        "Ui::AbstractSnapLayout: layouter and anchor not part of the same UI", );
    addExplicitLayout(layouter, snap, snapTarget, flags);
}

AbstractSnapLayout::AbstractSnapLayout(AbstractUserInterface& ui, SnapLayouter& layouter, const NodeHandle node, const Snaps snap, const LayouterDataHandle snapTarget, const SnapLayoutFlags flags): AbstractSnapLayout{NoInit, ui, node} {
    CORRADE_ASSERT(ui.isHandleValid(layouter.handle()) && &ui.layouter(layouter.handle()) == &layouter,
        "Ui::AbstractSnapLayout: layouter not part of the UI", );
    addExplicitLayout(layouter, snap, snapTarget, flags);
}

AbstractSnapLayout::AbstractSnapLayout(SnapLayouter& layouter, const AbstractAnchor& anchor, const Snaps snap, const LayouterDataHandle snapTarget, const SnapLayoutFlags flags): AbstractSnapLayout{NoInit, anchor.ui(), anchor.node()} {
    CORRADE_ASSERT(_ui->isHandleValid(layouter.handle()) && &_ui->layouter(layouter.handle()) == &layouter,
        "Ui::AbstractSnapLayout: layouter and anchor not part of the same UI", );
    addExplicitLayout(layouter, snap, snapTarget, flags);
}

void AbstractSnapLayout::addLayout(SnapLayouter& layouter) {
    _layouter = &layouter;

    /* Use existing layout if there is for given node. The node being valid is
       already checked by AbstractAnchor that was passed to the constructor. */
    _layout = _ui->nodeUniqueLayout(_node, layouter);
    /* If there isn't, add a new one */
    if(_layout == LayouterDataHandle::Null)
        _layout = layoutHandleData(layouter.add(_node));
}

void AbstractSnapLayout::addLayout(SnapLayouter& layouter, const LayoutHandle before, const SnapLayoutFlags flags) {
    _layouter = &layouter;

    #ifndef CORRADE_NO_ASSERT
    const LayouterDataHandle existingLayout = _ui->nodeUniqueLayout(_node, layouter);
    CORRADE_ASSERT(existingLayout == LayouterDataHandle::Null,
        "Ui::AbstractSnapLayout:" << _node << "already has" << layoutHandle(_layouter->handle(), existingLayout) << "assigned", );
    #endif
    _layout = layoutHandleData(layouter.add(_node, before, flags));
}

void AbstractSnapLayout::addLayout(SnapLayouter& layouter, const LayouterDataHandle before, const SnapLayoutFlags flags) {
    _layouter = &layouter;

    #ifndef CORRADE_NO_ASSERT
    const LayouterDataHandle existingLayout = _ui->nodeUniqueLayout(_node, layouter);
    CORRADE_ASSERT(existingLayout == LayouterDataHandle::Null,
        "Ui::AbstractSnapLayout:" << _node << "already has" << layoutHandle(_layouter->handle(), existingLayout) << "assigned", );
    #endif
    _layout = layoutHandleData(layouter.add(_node, before, flags));
}

void AbstractSnapLayout::addExplicitLayout(SnapLayouter& layouter, const Snaps snap, const LayoutHandle snapTarget, const SnapLayoutFlags flags) {
    _layouter = &layouter;

    #ifndef CORRADE_NO_ASSERT
    const LayouterDataHandle existingLayout = _ui->nodeUniqueLayout(_node, layouter);
    CORRADE_ASSERT(existingLayout == LayouterDataHandle::Null,
        "Ui::AbstractSnapLayout:" << _node << "already has" << layoutHandle(_layouter->handle(), existingLayout) << "assigned", );
    #endif
    _layout = layoutHandleData(layouter.addExplicit(_node, snap, snapTarget, flags));
}

void AbstractSnapLayout::addExplicitLayout(SnapLayouter& layouter, const Snaps snap, const LayouterDataHandle snapTarget, const SnapLayoutFlags flags) {
    _layouter = &layouter;

    #ifndef CORRADE_NO_ASSERT
    const LayouterDataHandle existingLayout = _ui->nodeUniqueLayout(_node, layouter);
    CORRADE_ASSERT(existingLayout == LayouterDataHandle::Null,
        "Ui::AbstractSnapLayout:" << _node << "already has" << layoutHandle(_layouter->handle(), existingLayout) << "assigned", );
    #endif
    _layout = layoutHandleData(layouter.addExplicit(_node, snap, snapTarget, flags));
}

AbstractSnapLayout::operator AbstractAnchor() const {
    return AbstractAnchor{*_ui, _node};
}

SnapLayoutFlags AbstractSnapLayout::flags() const {
    return _layouter->flags(_layout);
}

AbstractSnapLayout& AbstractSnapLayout::setFlags(const SnapLayoutFlags flags) {
    _layouter->setFlags(_layout, flags);
    return *this;
}

AbstractSnapLayout& AbstractSnapLayout::addFlags(const SnapLayoutFlags flags) {
    _layouter->addFlags(_layout, flags);
    return *this;
}

AbstractSnapLayout& AbstractSnapLayout::clearFlags(const SnapLayoutFlags flags) {
    _layouter->clearFlags(_layout, flags);
    return *this;
}

Snaps AbstractSnapLayout::childSnap() const {
    return _layouter->childSnap(_layout);
}

AbstractSnapLayout& AbstractSnapLayout::setChildSnap(const Snaps snap) {
    _layouter->setChildSnap(_layout, snap);
    return *this;
}

AbstractSnapLayout AbstractSnapLayout::child(const Vector2& nodeSize, const NodeFlags nodeFlags, const LayoutHandle layoutBefore, const SnapLayoutFlags layoutFlags) {
    const NodeHandle child = _ui->createNode(_node, {}, nodeSize, nodeFlags);
    const LayoutHandle layout = _layouter->add(child, layoutBefore, layoutFlags);
    return AbstractSnapLayout{*_ui, _layouter, child, layoutHandleData(layout)};
}

AbstractSnapLayout AbstractSnapLayout::child(const Vector2& nodeSize, const NodeFlags nodeFlags, const LayouterDataHandle layoutBefore, const SnapLayoutFlags layoutFlags) {
    const NodeHandle child = _ui->createNode(_node, {}, nodeSize, nodeFlags);
    const LayoutHandle layout = _layouter->add(child, layoutBefore, layoutFlags);
    return AbstractSnapLayout{*_ui, _layouter, child, layoutHandleData(layout)};
}

AbstractSnapLayout AbstractSnapLayout::child(const Snaps snap, const Vector2& nodeSize, const NodeFlags nodeFlags, const SnapLayoutFlags layoutFlags) {
    const NodeHandle child = _ui->createNode(_node, {}, nodeSize, nodeFlags);
    const LayoutHandle layout = _layouter->addExplicit(child, snap, _layout, layoutFlags);
    return AbstractSnapLayout{*_ui, _layouter, child, layoutHandleData(layout)};
}

AbstractSnapLayout AbstractSnapLayout::sibling(const Snaps snap, const Vector2& nodeSize, const NodeFlags nodeFlags, const SnapLayoutFlags layoutFlags) {
    const NodeHandle sibling = _ui->createNode(_ui->nodeParent(_node), {}, nodeSize, nodeFlags);
    const LayoutHandle layout = _layouter->addExplicit(sibling, snap, _layout, layoutFlags);
    return AbstractSnapLayout{*_ui, _layouter, sibling, layoutHandleData(layout)};
}

LayoutHandle AbstractSnapLayout::layout() const {
    return layoutHandle(_layouter->handle(), _layout);
}

void AbstractSnapLayout::setDefaultPropagateMargin() {
    /* Add the PropagateMargin flag to axes which don't have IgnoreOverflow
       already, as that would conflict */
    /** @todo even though this is now done only when creating a specialized
        layout out of another layout, and not of arbitrary anchor. it feels
        like a shitty solution, better ideas? */
    SnapLayoutFlags flags = _layouter->flags(_layout);
    if(!(flags & SnapLayoutFlag::IgnoreOverflowX))
        flags |= SnapLayoutFlag::PropagateMarginX;
    if(!(flags & SnapLayoutFlag::IgnoreOverflowY))
        flags |= SnapLayoutFlag::PropagateMarginY;
    _layouter->setFlags(_layout, flags);
}

/* Depending on the compiler these need an explicit export otherwise the
   specialization doesn't get exported. Worked on x86 Linux, doesn't work on
   ARM64 Linux, macOS and Windows. Huh. */
template<> MAGNUM_UI_EXPORT void AbstractSnapLayout::setDefaultChildSnapFor<BasicSnapLayoutColumn>() {
    setChildSnap(Snap::Bottom);
}
template<> MAGNUM_UI_EXPORT void AbstractSnapLayout::setDefaultChildSnapFor<BasicSnapLayoutColumnLeft>() {
    setChildSnap(Snap::BottomLeft|Snap::InsideX);
}
template<> MAGNUM_UI_EXPORT void AbstractSnapLayout::setDefaultChildSnapFor<BasicSnapLayoutColumnRight>() {
    setChildSnap(Snap::BottomRight|Snap::InsideX);
}
template<> MAGNUM_UI_EXPORT void AbstractSnapLayout::setDefaultChildSnapFor<BasicSnapLayoutColumnFill>() {
    setChildSnap(Snap::Bottom|Snap::FillX);
}
template<> MAGNUM_UI_EXPORT void AbstractSnapLayout::setDefaultChildSnapFor<BasicSnapLayoutRow>() {
    setChildSnap(Snap::Right);
}
template<> MAGNUM_UI_EXPORT void AbstractSnapLayout::setDefaultChildSnapFor<BasicSnapLayoutRowTop>() {
    setChildSnap(Snap::TopRight|Snap::InsideY);
}
template<> MAGNUM_UI_EXPORT void AbstractSnapLayout::setDefaultChildSnapFor<BasicSnapLayoutRowBottom>() {
    setChildSnap(Snap::BottomRight|Snap::InsideY);
}
template<> MAGNUM_UI_EXPORT void AbstractSnapLayout::setDefaultChildSnapFor<BasicSnapLayoutRowFill>() {
    setChildSnap(Snap::Right|Snap::FillY);
}

template class MAGNUM_UI_EXPORT BasicSnapLayout<UserInterface>;

}}
