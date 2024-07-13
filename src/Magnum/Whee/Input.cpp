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

#include "Input.h"

#include <Corrade/Containers/StringView.h>
#include <Corrade/Utility/Assert.h>
#include <Magnum/Math/Vector2.h>
#include <Magnum/Text/Alignment.h>

#include "Magnum/Whee/BaseLayer.h"
#include "Magnum/Whee/Handle.h"
#include "Magnum/Whee/NodeFlags.h"
#include "Magnum/Whee/Style.hpp"
#include "Magnum/Whee/TextLayer.h"
#include "Magnum/Whee/TextProperties.h"
#include "Magnum/Whee/UserInterface.h"

namespace Magnum { namespace Whee {

using Implementation::BaseStyle;
using Implementation::TextStyle;

Debug& operator<<(Debug& debug, const InputStyle value) {
    debug << "Whee::InputStyle" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case InputStyle::value: return debug << "::" #value;
        _c(Default)
        // _c(Primary)
        // _c(Success)
        // _c(Warning)
        // _c(Danger)
        // _c(Info)
        // _c(Dim)
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
        // _c(Primary)
        // _c(Success)
        // _c(Warning)
        // _c(Danger)
        // _c(Info)
        // _c(Dim)
        #undef _c
    }

    CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
}

TextStyle textLayerStyle(const InputStyle style) {
    switch(style) {
        case InputStyle::Default:
        // case InputStyle::Primary:
        // case InputStyle::Success:
        // case InputStyle::Warning:
        // case InputStyle::Danger:
        // case InputStyle::Info:
        // case InputStyle::Dim:
            return TextStyle::InputDefaultInactiveOut;
            // TODO
    }

    CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
}

}

Input::Input(const Anchor& anchor, const InputStyle style, const Containers::StringView text, const TextProperties& textProperties): Widget{anchor}, _style{style} {
    ui().addNodeFlags(node(), NodeFlag::Focusable);

    _backgroundData = dataHandleData(ui().baseLayer().create(baseLayerStyle(style), node()));

    // TODO clipping for overflows? uhhhhhhh
    _textData = dataHandleData(ui().textLayer().create(textLayerStyle(style), text, textProperties, TextDataFlag::Editable, node()));
}

Input::Input(const Anchor& anchor, const InputStyle style, const Containers::StringView text): Input{anchor, style, text, {}} {}

void Input::setStyle(const InputStyle style) {
    _style = style;
    ui().baseLayer().setStyle(_backgroundData, baseLayerStyle(style));
    // TODO the style should be already changed to pressed/hovered if this is called during onTap etc
    // TODO test this in GL test! how??
    ui().textLayer().setTransitionedStyle(ui(), _textData, textLayerStyle(style));
    // TODO re-set the text if font changed
}

DataHandle Input::backgroundData() const {
    /* The background is implicitly from the base layer */
    return dataHandle(ui().baseLayer().handle(), _backgroundData);
}

DataHandle Input::textData() const {
    /* The text is implicitly from the text layer */
    return dataHandle(ui().textLayer().handle(), _textData);
}

void Input::setText(const Containers::StringView text, const TextProperties& textProperties) {
    ui().textLayer().setText(_textData, text, textProperties);
}

void Input::setText(const Containers::StringView text) {
    setText(text, {});
}

}}
