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

#include "Anchor.h"

#include <Corrade/Utility/Assert.h>
#include <Magnum/Math/Vector2.h>

#include "Magnum/Ui/AbstractLayouter.h"
#include "Magnum/Ui/AbstractUserInterface.h"
#include "Magnum/Ui/Handle.h"

namespace Magnum { namespace Ui {

AbstractAnchor::AbstractAnchor(AbstractUserInterface& ui, const NodeHandle node, const LayoutHandle layout): _ui{ui}, _node{node}, _layout{layout} {
    CORRADE_ASSERT(ui.isHandleValid(node),
        "Ui::AbstractAnchor: invalid handle" << node, );
    CORRADE_ASSERT(layout == LayoutHandle::Null || ui.isHandleValid(layout),
        "Ui::AbstractAnchor: invalid handle" << layout, );
    /** @todo might make sense to turn this into a debug assert? might become
        pretty heavy */
    CORRADE_ASSERT(layout == LayoutHandle::Null || ui.layouter(layoutHandleLayouter(layout)).node(layout) == node,
        "Ui::AbstractAnchor:" << layout << "not associated with" << node, );
}

AbstractAnchor::AbstractAnchor(AbstractUserInterface& ui, const NodeHandle parent, const Vector2& offset, const Vector2& size, const NodeFlags flags): _ui{ui}, _layout{} {
    _node = ui.createNode(parent, offset, size, flags);
}

AbstractAnchor::AbstractAnchor(AbstractUserInterface& ui, const NodeHandle parent, const Vector2& size, const NodeFlags flags): AbstractAnchor{ui, parent, {}, size, flags} {}

AbstractAnchor::AbstractAnchor(AbstractUserInterface& ui, const Vector2& offset, const Vector2& size, const NodeFlags flags): AbstractAnchor{ui, NodeHandle::Null, offset, size, flags} {}

AbstractAnchor::AbstractAnchor(AbstractUserInterface& ui, const Vector2& size, const NodeFlags flags): AbstractAnchor{ui, NodeHandle::Null, {}, size, flags} {}

AbstractAnchor::operator Magnum::Ui::LayoutHandle() const {
    CORRADE_ASSERT(_layout != LayoutHandle::Null,
        "Ui::AbstractAnchor: layout is null", {});
    return _layout;
}

}}
