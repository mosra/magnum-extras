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

#include "Label.h"

#include <Corrade/Containers/StringView.h>
#include <Corrade/Utility/Assert.h>
#include <Magnum/Math/Vector2.h>
#include <Magnum/Text/Alignment.h>

#include "Magnum/Whee/BaseLayer.h"
#include "Magnum/Whee/Handle.h"
#include "Magnum/Whee/Style.hpp"
#include "Magnum/Whee/TextLayer.h"
#include "Magnum/Whee/TextProperties.h"
#include "Magnum/Whee/UserInterface.h"

namespace Magnum { namespace Whee {

using Implementation::BaseStyle;
using Implementation::TextStyle;

Debug& operator<<(Debug& debug, const LabelStyle value) {
    debug << "Whee::LabelStyle" << Debug::nospace;

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

TextStyle textLayerStyleIconOnly(const LabelStyle style) {
    switch(style) {
        case LabelStyle::Default:   return TextStyle::LabelDefaultIconOnly;
        case LabelStyle::Primary:   return TextStyle::LabelPrimaryIconOnly;
        case LabelStyle::Success:   return TextStyle::LabelSuccessIconOnly;
        case LabelStyle::Warning:   return TextStyle::LabelWarningIconOnly;
        case LabelStyle::Danger:    return TextStyle::LabelDangerIconOnly;
        case LabelStyle::Info:      return TextStyle::LabelInfoIconOnly;
        case LabelStyle::Dim:       return TextStyle::LabelDimIconOnly;
    }

    CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
}

TextStyle textLayerStyleTextOnly(const LabelStyle style) {
    switch(style) {
        case LabelStyle::Default:   return TextStyle::LabelDefaultTextOnly;
        case LabelStyle::Primary:   return TextStyle::LabelPrimaryTextOnly;
        case LabelStyle::Success:   return TextStyle::LabelSuccessTextOnly;
        case LabelStyle::Warning:   return TextStyle::LabelWarningTextOnly;
        case LabelStyle::Danger:    return TextStyle::LabelDangerTextOnly;
        case LabelStyle::Info:      return TextStyle::LabelInfoTextOnly;
        case LabelStyle::Dim:       return TextStyle::LabelDimTextOnly;
    }

    CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
}

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

void alignIconText(TextLayer& textLayer, const LabelStyle style, const LayerDataHandle icon, const LayerDataHandle text) {
    /* If both the text and the icon is present, shift the icon to be next to
       the text instead of in the center, and pick appropriate styles for
       correct alignment */
    if(icon != LayerDataHandle::Null && text != LayerDataHandle::Null) {
        /** @todo handle vertical / sideways labels as well, eventually */
        const Float halfTextWidth = textLayer.size(text).x()*0.5f;
        textLayer.setPadding(icon, {-halfTextWidth, 0.0f, halfTextWidth, 0.0f});
        textLayer.setStyle(icon, textLayerStyleIcon(style));
        textLayer.setStyle(text, textLayerStyleText(style));

    /* Otherwise, if there's just an icon, reset its padding back to 0, and
       pick a style for correct alignment */
    } else if(icon != LayerDataHandle::Null) {
        textLayer.setPadding(icon, {});
        textLayer.setStyle(icon, textLayerStyleIconOnly(style));

    /* Otherwise, if there's just a text, pick a style for correct alignment */
    } else if(text != LayerDataHandle::Null) {
        textLayer.setStyle(text, textLayerStyleTextOnly(style));
    }
}

struct LabelData {
    LayerDataHandle text;
    LayerDataHandle icon;
};

LabelData labelInternal(UserInterface& ui, const NodeHandle node, const LabelStyle style, const Icon icon, const Containers::StringView text, const TextProperties& properties) {
    LabelData out{};

    /* Style ID for these two is corrected in alignIconText() below */
    TextLayer& textLayer = ui.textLayer();
    if(icon != Icon::None)
        out.icon = dataHandleData(textLayer.createGlyph(textLayerStyleIconOnly(style), icon, {}, node));
    if(text)
        out.text = dataHandleData(textLayer.create(textLayerStyleTextOnly(style), text, properties, node));
    alignIconText(textLayer, style, out.icon, out.text);

    return out;
}

}

Label::Label(UserInterface& ui, const NodeHandle parent, const LabelStyle style, const Vector2& size, const Icon icon, const Containers::StringView text, const TextProperties& textProperties): Widget{ui, ui.createNode(parent, {}, size)}, _style{style}, _icon{icon} {
    LabelData data = labelInternal(ui, node(), style, icon, text, textProperties);
    _textData = data.text;
    _iconData = data.icon;
}

Label::Label(UserInterface& ui, const NodeHandle parent, const LabelStyle style, const Vector2& size, const Containers::StringView text, const TextProperties& textProperties): Label{ui, parent, style, size, Icon::None, text, textProperties} {}

Label::Label(UserInterface& ui, const NodeHandle parent, const LabelStyle style, const Vector2& size, const Icon icon, const Containers::StringView text): Label{ui, parent, style, size, icon, text, {}} {}

Label::Label(UserInterface& ui, const NodeHandle parent, const LabelStyle style, const Vector2& size, const Containers::StringView text): Label{ui, parent, style, size, Icon::None, text} {}

Label::Label(UserInterface& ui, const NodeHandle parent, const LabelStyle style, const Vector2& size, const Icon icon): Label{ui, parent, style, size, icon, {}} {}

void Label::setStyle(const LabelStyle style) {
    TextLayer& textLayer = ui().textLayer();

    _style = style;
    // TODO test this in GL test! how??
    if(_textData != LayerDataHandle::Null)
        textLayer.setStyle(_textData,
            (_iconData == LayerDataHandle::Null ? textLayerStyleTextOnly : textLayerStyleText)(style));
    if(_iconData != LayerDataHandle::Null)
        textLayer.setStyle(_iconData,
            (_textData == LayerDataHandle::Null ? textLayerStyleIconOnly : textLayerStyleIcon)(style));
}

DataHandle Label::iconData() const {
    /* The icon is implicitly from the text layer */
    return _iconData == LayerDataHandle::Null ? DataHandle::Null :
        dataHandle(ui().textLayer().handle(), _iconData);
}

void Label::setIcon(const Icon icon) {
    TextLayer& textLayer = ui().textLayer();

    _icon = icon;
    if(icon != Icon::None) {
        if(_iconData == LayerDataHandle::Null)
            /* Style ID is corrected in alignIconText() below */
            _iconData = dataHandleData(textLayer.createGlyph(textLayerStyleIconOnly(_style), icon, {}, node()));
        else textLayer.setGlyph(_iconData, icon, {});
    } else if(_iconData != LayerDataHandle::Null) {
        textLayer.remove(_iconData);
        _iconData = LayerDataHandle::Null;
    }

    alignIconText(textLayer, _style, _iconData, _textData);
}

DataHandle Label::textData() const {
    /* The text is implicitly from the text layer */
    return _textData == LayerDataHandle::Null ? DataHandle::Null :
        dataHandle(ui().textLayer().handle(), _textData);
}

void Label::setText(const Containers::StringView text, const TextProperties& textProperties) {
    TextLayer& textLayer = ui().textLayer();

    if(text) {
        if(_textData == LayerDataHandle::Null)
            /* Style ID is corrected in alignIconText() below */
            _textData = dataHandleData(textLayer.create(textLayerStyleTextOnly(_style), text, textProperties, node()));
        else
            textLayer.setText(_textData, text, textProperties);
    } else if(_textData != LayerDataHandle::Null) {
        textLayer.remove(_textData);
        _textData = LayerDataHandle::Null;
    }

    alignIconText(textLayer, _style, _iconData, _textData);
}

void Label::setText(const Containers::StringView text) {
    setText(text, {});
}

NodeHandle label(UserInterface& ui, const NodeHandle parent, const LabelStyle style, const Vector2& size, const Icon icon, const Containers::StringView text, const TextProperties& textProperties) {
    const NodeHandle node = ui.createNode(parent, {}, size);
    labelInternal(ui, node, style, icon, text, textProperties);
    return node;
}
NodeHandle label(UserInterface& ui, const NodeHandle parent, const LabelStyle style, const Vector2& size, const Containers::StringView text, const TextProperties& textProperties) {
    return label(ui, parent, style, size, Icon::None, text, textProperties);
}

NodeHandle label(UserInterface& ui, const NodeHandle parent, const LabelStyle style, const Vector2& size, const Icon icon, const Containers::StringView text) {
    return label(ui, parent, style, size, icon, text, {});
}

NodeHandle label(UserInterface& ui, const NodeHandle parent, const LabelStyle style, const Vector2& size, const Containers::StringView text) {
    return label(ui, parent, style, size, Icon::None, text);
}

NodeHandle label(UserInterface& ui, const NodeHandle parent, const LabelStyle style, const Vector2& size, const Icon icon) {
    return label(ui, parent, style, size, icon, {});
}

}}
