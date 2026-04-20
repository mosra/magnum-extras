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

#include "Panel.h"

#include <Corrade/Utility/Assert.h>
#include <Magnum/Math/Time.h>

#include "Magnum/Ui/AbstractTheme.hpp"
#include "Magnum/Ui/Anchor.h"
#include "Magnum/Ui/BaseLayer.h"
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/LayoutLayer.h"
#include "Magnum/Ui/UserInterface.h"

namespace Magnum { namespace Ui {

Debug& operator<<(Debug& debug, const PanelStyle value) {
    debug << "Ui::PanelStyle" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case PanelStyle::value: return debug << "::" #value;
        _c(Default)
        _c(Filled)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "(" << Debug::nospace << Debug::hex << UnsignedByte(value) << Debug::nospace << ")";
}

using Implementation::BaseStyle;
using Implementation::LayoutStyle;

Panel::Panel(const Anchor anchor, const PanelStyle style): Widget{anchor}, _style{style} {
    /* The LayoutLayer data aren't stored because currently they're never
       updated */
    ui().layoutLayer().create(LayoutStyle::Panel, node());

    _backgroundData = style == PanelStyle::Default ? LayerDataHandle::Null :
        dataHandleData(ui().baseLayer().create(BaseStyle::PanelBackground, node()));
}

Panel::Panel(NonOwnedT, const Anchor anchor, const PanelStyle style): Panel{anchor, style} {
    makeNonOwned();
}

Panel& Panel::setStyle(const PanelStyle style) {
    return setStyleInternal(style);
}

Panel& Panel::setStyle(const PanelStyle style, const Nanoseconds time) {
    return setStyleInternal(style, time);
}

template<class ...Args> Panel& Panel::setStyleInternal(const PanelStyle style, Args...) {
    /** @todo use the `args` once there's actually any transitionStyle() done
        besides just creating or removing data */

    /* Create a data for the background, if switching from an empty
       background */
    if(_style == PanelStyle::Default && style == PanelStyle::Filled) {
        CORRADE_INTERNAL_DEBUG_ASSERT(_backgroundData == LayerDataHandle::Null);
        _backgroundData = dataHandleData(ui().baseLayer().create(BaseStyle::PanelBackground, node()));

    /* Remove the data, if switching back */
    } else if(_style == PanelStyle::Filled && style == PanelStyle::Default) {
        CORRADE_INTERNAL_DEBUG_ASSERT(_backgroundData != LayerDataHandle::Null);
        ui().baseLayer().remove(_backgroundData);
        _backgroundData = LayerDataHandle::Null;

    /* There's currently no other variant */
    } else CORRADE_INTERNAL_ASSERT(style == _style);

    _style = style;
    return *this;
}

Anchor Panel::contents() const {
    /* Yes, it's currently the panel node itself. But this makes the usage
       consistent with the ScrollArea and other APIs. */
    return {ui(), node()};
}

DataHandle Panel::backgroundData() const {
    /* The data is implicitly from the base layer */
    return _backgroundData == LayerDataHandle::Null ? DataHandle::Null :
        dataHandle(ui().baseLayer(), _backgroundData);
}

}}
