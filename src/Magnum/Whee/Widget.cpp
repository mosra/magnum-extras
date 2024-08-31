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

#include "Widget.h"

#include <Corrade/Utility/Assert.h>

#include "Magnum/Whee/Anchor.h"
#include "Magnum/Whee/Handle.h"
#include "Magnum/Whee/NodeFlags.h"
#include "Magnum/Whee/AbstractUserInterface.h"

namespace Magnum { namespace Whee {

AbstractWidget::AbstractWidget(AbstractUserInterface& ui, NodeHandle node): _ui{ui}, _node{node} {
    CORRADE_ASSERT(ui.isHandleValid(node),
        "Whee::AbstractWidget: invalid handle" << node, );
}

AbstractWidget::AbstractWidget(const AbstractAnchor& anchor): _ui{anchor.ui()}, _node{anchor.node()} {
    /* No validity assertion here as the anchor takes care of that already. If
       the node got removed since the anchor was created, that's not our
       problem tho. */
}

AbstractWidget::AbstractWidget(AbstractWidget&& other) noexcept: _ui{other._ui}, _node{other._node} {
    /* not using NodeHandle::Null to not need to include Handle.h */
    other._node = NodeHandle{};
}

AbstractWidget::~AbstractWidget() {
    if(_node != NodeHandle::Null) {
        CORRADE_ASSERT(_ui->isHandleValid(_node),
            "Whee::AbstractWidget: invalid handle" << _node << "on destruction", );
        _ui->removeNode(_node);
    }
}

AbstractWidget& AbstractWidget::operator=(AbstractWidget&& other) noexcept {
    Utility::swap(_ui, other._ui);
    Utility::swap(_node, other._node);
    return *this;
}

bool AbstractWidget::isHidden() const {
    return _ui->nodeFlags(_node) >= NodeFlag::Hidden;
}

void AbstractWidget::setHidden(const bool hidden) {
    hidden ? _ui->addNodeFlags(_node, NodeFlag::Hidden) :
             _ui->clearNodeFlags(_node, NodeFlag::Hidden);
}

bool AbstractWidget::isDisabled() const {
    return _ui->nodeFlags(_node) >= NodeFlag::Disabled;
}

void AbstractWidget::setDisabled(const bool disabled) {
    disabled ? _ui->addNodeFlags(_node, NodeFlag::Disabled) :
               _ui->clearNodeFlags(_node, NodeFlag::Disabled);
}

NodeHandle AbstractWidget::release() {
    const NodeHandle out = _node;
    _node = NodeHandle::Null;
    return out;
}

}}
