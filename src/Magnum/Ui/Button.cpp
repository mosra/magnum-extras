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

#include "Button.h"

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

Debug& operator<<(Debug& debug, const ButtonStyle value) {
    debug << "Ui::ButtonStyle" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case ButtonStyle::value: return debug << "::" #value;
        _c(Default)
        _c(Primary)
        _c(Success)
        _c(Warning)
        _c(Danger)
        _c(Info)
        _c(Dim)
        _c(Flat)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "(" << Debug::nospace << Debug::hex << UnsignedByte(value) << Debug::nospace << ")";
}

namespace {

using Implementation::BaseStyle;
using Implementation::TextStyle;

BaseStyle baseLayerStyle(const ButtonStyle style) {
    switch(style) {
        #define _c(style) case ButtonStyle::style: return BaseStyle::Button ## style ## InactiveOut;
        _c(Default)
        _c(Primary)
        _c(Success)
        _c(Warning)
        _c(Danger)
        _c(Info)
        _c(Dim)
        _c(Flat)
        #undef _c
    }

    CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
}

TextStyle textLayerStyleIconOnly(const ButtonStyle style) {
    switch(style) {
        case ButtonStyle::Default:
        case ButtonStyle::Primary:
        case ButtonStyle::Success:
        case ButtonStyle::Warning:
        case ButtonStyle::Danger:
        case ButtonStyle::Info:
        case ButtonStyle::Dim:
            return TextStyle::ButtonIconOnly;
        case ButtonStyle::Flat:
            return TextStyle::ButtonFlatInactiveOutIconOnly;
    }

    CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
}

TextStyle textLayerStyleTextOnly(const ButtonStyle style) {
    switch(style) {
        case ButtonStyle::Default:
        case ButtonStyle::Primary:
        case ButtonStyle::Success:
        case ButtonStyle::Warning:
        case ButtonStyle::Danger:
        case ButtonStyle::Info:
        case ButtonStyle::Dim:
            return TextStyle::ButtonTextOnly;
        case ButtonStyle::Flat:
            return TextStyle::ButtonFlatInactiveOutTextOnly;
    }

    CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
}

TextStyle textLayerStyleIcon(const ButtonStyle style) {
    switch(style) {
        case ButtonStyle::Default:
        case ButtonStyle::Primary:
        case ButtonStyle::Success:
        case ButtonStyle::Warning:
        case ButtonStyle::Danger:
        case ButtonStyle::Info:
        case ButtonStyle::Dim:
            return TextStyle::ButtonIcon;
        case ButtonStyle::Flat:
            return TextStyle::ButtonFlatInactiveOutIcon;
    }

    CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
}

TextStyle textLayerStyleText(const ButtonStyle style) {
    switch(style) {
        case ButtonStyle::Default:
        case ButtonStyle::Primary:
        case ButtonStyle::Success:
        case ButtonStyle::Warning:
        case ButtonStyle::Danger:
        case ButtonStyle::Info:
        case ButtonStyle::Dim:
            return TextStyle::ButtonText;
        case ButtonStyle::Flat:
            return TextStyle::ButtonFlatInactiveOutText;
    }

    CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
}

void alignIconText(TextLayer& textLayer, const ButtonStyle style, const LayerDataHandle icon, const LayerDataHandle text) {
    /* If both the text and the icon is present, shift the icon to be next to
       the text instead of in the center, and pick appropriate styles for
       correct alignment */
    if(icon != LayerDataHandle::Null && text != LayerDataHandle::Null) {
        /** @todo handle vertical / sideways buttons as well, eventually */
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

struct ButtonData {
    LayerDataHandle background;
    LayerDataHandle text;
    LayerDataHandle icon;
};

ButtonData buttonInternal(UserInterface& ui, const NodeHandle node, const Icon icon, const Containers::StringView text, const TextProperties& properties, const ButtonStyle style) {
    ButtonData out{};

    out.background = dataHandleData(ui.baseLayer().create(baseLayerStyle(style), node));

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

Button::Button(const Anchor& anchor, const Icon icon, const Containers::StringView text, const TextProperties& textProperties, const ButtonStyle style): Widget{anchor}, _style{style}, _icon{icon} {
    ButtonData data = buttonInternal(ui(), node(), icon, text, textProperties, style);
    _backgroundData = data.background;
    _textData = data.text;
    _iconData = data.icon;
}

Button::Button(const Anchor& anchor, const Icon icon, const Containers::StringView text, const ButtonStyle style): Button{anchor, icon, text, {}, style} {}

Button::Button(const Anchor& anchor, const Icon icon, const ButtonStyle style): Button{anchor, icon, {}, {}, style} {}

Button::Button(const Anchor& anchor, const Containers::StringView text, const TextProperties& textProperties, const ButtonStyle style): Button{anchor, Icon::None, text, textProperties, style} {}

Button::Button(const Anchor& anchor, const Containers::StringView text, const ButtonStyle style): Button{anchor, text, {}, style} {}

Button& Button::setStyle(const ButtonStyle style) {
    _style = style;
    ui().baseLayer().setTransitionedStyle(ui(), _backgroundData, baseLayerStyle(style));
    if(_textData != LayerDataHandle::Null)
        ui().textLayer().setTransitionedStyle(ui(), _textData,
            (_iconData == LayerDataHandle::Null ? textLayerStyleTextOnly : textLayerStyleText)(style));
    if(_iconData != LayerDataHandle::Null)
        ui().textLayer().setTransitionedStyle(ui(), _iconData,
            (_textData == LayerDataHandle::Null ? textLayerStyleIconOnly : textLayerStyleIcon)(style));
    return *this;
}

Button& Button::setIcon(const Icon icon) {
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
    return *this;
}

Button& Button::setText(const Containers::StringView text, const TextProperties& textProperties) {
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
    return *this;
}

Button& Button::setText(const Containers::StringView text) {
    return setText(text, {});
}

DataHandle Button::backgroundData() const {
    /* The background is implicitly from the base layer. It can be null only
       for a NoCreate'd instance, otherwise not. */
    return _backgroundData == LayerDataHandle::Null ? DataHandle::Null :
        dataHandle(ui().baseLayer(), _backgroundData);
}

DataHandle Button::iconData() const {
    /* The icon is implicitly from the text layer */
    return _iconData == LayerDataHandle::Null ? DataHandle::Null :
        dataHandle(ui().textLayer(), _iconData);
}

DataHandle Button::textData() const {
    /* The text is implicitly from the text layer */
    return _textData == LayerDataHandle::Null ? DataHandle::Null :
        dataHandle(ui().textLayer(), _textData);
}

Anchor button(const Anchor& anchor, const Icon icon, const Containers::StringView text, const TextProperties& textProperties, const ButtonStyle style) {
    buttonInternal(anchor.ui(), anchor.node(), icon, text, textProperties, style);
    return anchor;
}

Anchor button(const Anchor& anchor, const Icon icon, const Containers::StringView text, const ButtonStyle style) {
    return button(anchor, icon, text, {}, style);
}

Anchor button(const Anchor& anchor, const Icon icon, const ButtonStyle style) {
    return button(anchor, icon, {}, {}, style);
}

Anchor button(const Anchor& anchor, const Containers::StringView text, const TextProperties& textProperties, const ButtonStyle style) {
    return button(anchor, Icon::None, text, textProperties, style);
}

Anchor button(const Anchor& anchor, const Containers::StringView text, const ButtonStyle style) {
    return button(anchor, text, {}, style);
}

}}
