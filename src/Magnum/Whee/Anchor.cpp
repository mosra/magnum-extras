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

#include "Anchor.h"

#include <Corrade/Utility/Assert.h>
#include <Magnum/Math/Vector2.h>

#include "Magnum/Whee/AbstractLayouter.h"
#include "Magnum/Whee/Handle.h"
#include "Magnum/Whee/UserInterface.h"

namespace Magnum { namespace Whee {

Anchor::Anchor(UserInterface& ui, const NodeHandle node, const LayoutHandle layout): _ui{ui}, _node{node}, _layout{layout} {
    CORRADE_ASSERT(ui.isHandleValid(node),
        "Whee::Anchor: invalid handle" << node, );
    CORRADE_ASSERT(layout == LayoutHandle::Null || ui.isHandleValid(layout),
        "Whee::Anchor: invalid handle" << layout, );
    /** @todo might make sense to turn this into a debug assert? might become
        pretty heavy */
    CORRADE_ASSERT(layout == LayoutHandle::Null || ui.layouter(layoutHandleLayouter(layout)).node(layout) == node,
        "Whee::Anchor:" << layout << "not associated with" << node, );
}

Anchor::Anchor(UserInterface& ui, const NodeHandle parent, const Vector2& offset, const Vector2& size, const NodeFlags flags): _ui{ui}, _layout{} {
    _node = ui.createNode(parent, offset, size, flags);
}

Anchor::Anchor(UserInterface& ui, const NodeHandle parent, const Vector2& size, const NodeFlags flags): Anchor{ui, parent, {}, size, flags} {}

Anchor::Anchor(UserInterface& ui, const Vector2& offset, const Vector2& size, const NodeFlags flags): Anchor{ui, NodeHandle::Null, offset, size, flags} {}

Anchor::Anchor(UserInterface& ui, const Vector2& size, const NodeFlags flags): Anchor{ui, NodeHandle::Null, {}, size, flags} {}

Anchor::operator Magnum::Whee::LayoutHandle() const {
    CORRADE_ASSERT(_layout != LayoutHandle::Null,
        "Whee::Anchor: layout is null", {});
    return _layout;
}

}}