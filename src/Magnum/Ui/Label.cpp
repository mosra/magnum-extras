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

#include "Label.h"

#include <Corrade/Containers/StringView.h>
#include <Corrade/Utility/Assert.h>
#include <Magnum/Math/Vector2.h>
#include <Magnum/Text/Alignment.h>

#include "Magnum/Ui/Anchor.h"
#include "Magnum/Ui/BaseLayer.h"
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/Style.h"
#include "Magnum/Ui/Style.hpp"
#include "Magnum/Ui/TextLayer.h"
#include "Magnum/Ui/TextProperties.h"
#include "Magnum/Ui/UserInterface.h"

namespace Magnum { namespace Ui {

Debug& operator<<(Debug& debug, const LabelStyle value) {
    debug << "Ui::LabelStyle" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case LabelStyle::value: return debug << "::" #value;
        _c(Default)
        _c(Primary)
        _c(Success)
        _c(Warning)
        _c(Danger)
        _c(Info)
        _c(Dim)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "(" << Debug::nospace << Debug::hex << UnsignedByte(value) << Debug::nospace << ")";
}

namespace {

using Implementation::TextStyle;

TextStyle textLayerStyleIcon(const LabelStyle style) {
    switch(style) {
        case LabelStyle::Default:   return TextStyle::LabelDefaultIcon;
        case LabelStyle::Primary:   return TextStyle::LabelPrimaryIcon;
        case LabelStyle::Success:   return TextStyle::LabelSuccessIcon;
        case LabelStyle::Warning:   return TextStyle::LabelWarningIcon;
        case LabelStyle::Danger:    return TextStyle::LabelDangerIcon;
        case LabelStyle::Info:      return TextStyle::LabelInfoIcon;
        case LabelStyle::Dim:       return TextStyle::LabelDimIcon;
    }

    CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
}

TextStyle textLayerStyleText(const LabelStyle style) {
    switch(style) {
        case LabelStyle::Default:   return TextStyle::LabelDefaultText;
        case LabelStyle::Primary:   return TextStyle::LabelPrimaryText;
        case LabelStyle::Success:   return TextStyle::LabelSuccessText;
        case LabelStyle::Warning:   return TextStyle::LabelWarningText;
        case LabelStyle::Danger:    return TextStyle::LabelDangerText;
        case LabelStyle::Info:      return TextStyle::LabelInfoText;
        case LabelStyle::Dim:       return TextStyle::LabelDimText;
    }

    CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
}

}

Label::Label(const Anchor& anchor, const Icon icon, const LabelStyle style): Widget{anchor}, _style{style}, _icon{icon} {
    _data = icon == Icon::None ? LayerDataHandle::Null :
        dataHandleData(ui().textLayer().createGlyph(textLayerStyleIcon(style), icon, {}, node()));
}

Label::Label(const Anchor& anchor, const Containers::StringView text, const TextProperties& textProperties, const LabelStyle style): Widget{anchor}, _style{style}, _icon{Icon::None} {
    _data = !text ? LayerDataHandle::Null :
        dataHandleData(ui().textLayer().create(textLayerStyleText(style), text, textProperties, node()));
}

Label::Label(const Anchor& anchor, const Containers::StringView text, const LabelStyle style): Label{anchor, text, {}, style} {}

Label& Label::setStyle(const LabelStyle style) {
    _style = style;
    if(_data != LayerDataHandle::Null)
        ui().textLayer().setStyle(_data, (_icon == Icon::None ? textLayerStyleText : textLayerStyleIcon)(style));
    return *this;
}

Label& Label::setIcon(const Icon icon) {
    TextLayer& textLayer = ui().textLayer();

    _icon = icon;
    if(icon != Icon::None) {
        if(_data == LayerDataHandle::Null)
            _data = dataHandleData(textLayer.createGlyph(textLayerStyleIcon(_style), icon, {}, node()));
        else textLayer.setGlyph(_data, icon, {});
    } else if(_data != LayerDataHandle::Null) {
        textLayer.remove(_data);
        _data = LayerDataHandle::Null;
    }

    return *this;
}

Label& Label::setText(const Containers::StringView text, const TextProperties& textProperties) {
    TextLayer& textLayer = ui().textLayer();

    _icon = Icon::None;
    if(text) {
        if(_data == LayerDataHandle::Null)
            _data = dataHandleData(textLayer.create(textLayerStyleText(_style), text, textProperties, node()));
        else
            textLayer.setText(_data, text, textProperties);
    } else if(_data != LayerDataHandle::Null) {
        textLayer.remove(_data);
        _data = LayerDataHandle::Null;
    }

    return *this;
}

Label& Label::setText(const Containers::StringView text) {
    return setText(text, {});
}

DataHandle Label::data() const {
    /* The data is implicitly from the text layer */
    return _data == LayerDataHandle::Null ? DataHandle::Null :
        dataHandle(ui().textLayer(), _data);
}

Anchor label(const Anchor& anchor, const Containers::StringView text, const TextProperties& textProperties, const LabelStyle style) {
    if(text)
        anchor.ui().textLayer().create(textLayerStyleText(style), text, textProperties, anchor.node());
    return anchor;
}

Anchor label(const Anchor& anchor, const Containers::StringView text, const LabelStyle style) {
    return label(anchor, text, {}, style);
}

Anchor label(const Anchor& anchor, const Icon icon, const LabelStyle style) {
    if(icon != Icon::None)
        anchor.ui().textLayer().createGlyph(textLayerStyleIcon(style), icon, {}, anchor.node());
    return anchor;
}

}}
