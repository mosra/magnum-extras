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

#include "Input.h"

#include <Corrade/Containers/StringView.h>
#include <Corrade/Utility/Assert.h>
#include <Magnum/Math/Vector2.h>
#include <Magnum/Text/Alignment.h>

#include "Magnum/Ui/BaseLayer.h"
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/NodeFlags.h"
#include "Magnum/Ui/Style.hpp"
#include "Magnum/Ui/TextLayer.h"
#include "Magnum/Ui/TextProperties.h"
#include "Magnum/Ui/UserInterface.h"

namespace Magnum { namespace Ui {

using Implementation::BaseStyle;
using Implementation::TextStyle;

Debug& operator<<(Debug& debug, const InputStyle value) {
    debug << "Ui::InputStyle" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case InputStyle::value: return debug << "::" #value;
        _c(Default)
        _c(Success)
        _c(Warning)
        _c(Danger)
        _c(Flat)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "(" << Debug::nospace << Debug::hex << UnsignedByte(value) << Debug::nospace << ")";
}

namespace {

BaseStyle baseLayerStyle(const InputStyle style) {
    switch(style) {
        #define _c(style) case InputStyle::style: return BaseStyle::Input ## style ## InactiveOut;
        _c(Default)
        _c(Success)
        _c(Warning)
        _c(Danger)
        _c(Flat)
        #undef _c
    }

    CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
}

TextStyle textLayerStyle(const InputStyle style) {
    switch(style) {
        case InputStyle::Default:
            return TextStyle::InputDefaultInactiveOut;
        case InputStyle::Success:
            return TextStyle::InputSuccessInactiveOut;
        case InputStyle::Warning:
            return TextStyle::InputWarningInactiveOut;
        case InputStyle::Danger:
            return TextStyle::InputDangerInactiveOut;
        case InputStyle::Flat:
            return TextStyle::InputFlatInactiveOut;
    }

    CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
}

}

Input::Input(const Anchor& anchor, const Containers::StringView text, const TextProperties& textProperties, const InputStyle style): Widget{anchor}, _style{style} {
    ui().addNodeFlags(node(), NodeFlag::Focusable);

    _backgroundData = dataHandleData(ui().baseLayer().create(baseLayerStyle(style), node()));
    _textData = dataHandleData(ui().textLayer().create(textLayerStyle(style), text, textProperties, TextDataFlag::Editable, node()));
}

Input::Input(const Anchor& anchor, const Containers::StringView text, const InputStyle style): Input{anchor, text, {}, style} {}

Input& Input::setStyle(const InputStyle style) {
    _style = style;
    ui().baseLayer().setTransitionedStyle(ui(), _backgroundData, baseLayerStyle(style));
    ui().textLayer().setTransitionedStyle(ui(), _textData, textLayerStyle(style));
    /** @todo re-set the text if font / alignment ... changed */
    return *this;
}

DataHandle Input::backgroundData() const {
    /* The background is implicitly from the base layer. It can be null only
       for a NoCreate'd instance, otherwise not. */
    return _backgroundData == LayerDataHandle::Null ? DataHandle::Null :
        dataHandle(ui().baseLayer().handle(), _backgroundData);
}

DataHandle Input::textData() const {
    /* The text is implicitly from the text layer. It can be null only
       for a NoCreate'd instance, otherwise not. */
    return _textData == LayerDataHandle::Null ? DataHandle::Null :
        dataHandle(ui().textLayer().handle(), _textData);
}

Containers::StringView Input::text() const {
    return ui().textLayer().text(_textData);
}

Input& Input::setText(const Containers::StringView text, const TextProperties& textProperties) {
    ui().textLayer().setText(_textData, text, textProperties);
    return *this;
}

Input& Input::setText(const Containers::StringView text) {
    return setText(text, {});
}

}}
