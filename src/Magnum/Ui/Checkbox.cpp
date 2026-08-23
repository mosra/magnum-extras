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

#include "Checkbox.h"

#include <Corrade/Containers/StringView.h>

#include "Magnum/Ui/AbstractTheme.hpp"
#include "Magnum/Ui/Anchor.h"
#include "Magnum/Ui/BaseLayer.h"
#include "Magnum/Ui/DataLayer.h"
#include "Magnum/Ui/EnumStorage.h"
#include "Magnum/Ui/EventLayer.h"
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/Icon.h"
#include "Magnum/Ui/LayoutLayer.h"
#include "Magnum/Ui/TextLayer.h"
#include "Magnum/Ui/TextProperties.h"
#include "Magnum/Ui/UserInterface.h"

namespace Magnum { namespace Ui {

using namespace Math::Literals;
using Implementation::BaseStyle;
using Implementation::TextStyle;
using Implementation::LayoutStyle;

Debug& operator<<(Debug& debug, const CheckboxStyle value) {
    debug << "Ui::CheckboxStyle" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case CheckboxStyle::value: return debug << "::" #value;
        _c(Checkbox)
        _c(RadioButton)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "(" << Debug::nospace << Debug::hex << UnsignedByte(value) << Debug::nospace << ")";
}

Checkbox::Checkbox(const Anchor anchor, const Containers::StringView text, const TextProperties& textProperties, const CheckboxStyle style): Widget{anchor}, _style{style} {
    EnumStorage<Int> storage{ui(), StorageFlag::ReferenceCounted};
    storage.setEnumSet(true);
    createInternal(storage.value<1>(), text, textProperties);
}

Checkbox::Checkbox(const Anchor anchor, const Containers::StringView text, const CheckboxStyle style): Checkbox{anchor, text, {}, style} {}

Checkbox::Checkbox(const Anchor anchor, const StorageQuery<bool>& query, const Containers::StringView text, const TextProperties& textProperties, const CheckboxStyle style): Widget{anchor}, _style{style} {
    createInternal(query, text, textProperties);
}

void Checkbox::createInternal(const StorageQuery<bool>& query, const Containers::StringView text, const TextProperties& textProperties) {
    ui().layoutLayer().create(LayoutStyle::Checkbox, node());

    _backgroundData = dataHandleData(ui().baseLayer().create(_style == CheckboxStyle::RadioButton ? BaseStyle::RadioButton : BaseStyle::Checkbox, node()));
    ui().baseLayer().setAlignment(_backgroundData, BaseLayerAlignment::Left|BaseLayerAlignment::CenterY);
    _checkboxData = dataHandleData(ui().textLayer().createGlyph(TextStyle::Checkbox, _style == CheckboxStyle::RadioButton ? Icon::RadioButton : Icon::Checkbox, {}, node()));
    _textData = dataHandleData(ui().textLayer().create(TextStyle::CheckboxLabel, text, textProperties, node()));

    {
        /** @todo clean this up once I can use C++14 named captures */
        UserInterface& ui = this->ui();
        const LayerDataHandle checkboxData = _checkboxData;
        _dataBindingData = query.onUpdate([&ui, checkboxData](bool value) {
            /** @todo switch a style between a checked and unchecked variant
                instead to make the transition styleable, once it no longer
                clashes with other transition animations */
            ui.textLayer().setColor(checkboxData, value ? 0xffffffff_rgbaf : 0x00000000_rgbaf);
        }, node());
    }

    /* If the query is mutable, make the checkbox toggleable */
    if(query.isMutable()) {
        CORRADE_ASSERT(query.operations() >= (StorageOperation::Set|StorageOperation::Toggle),
            "Ui::Checkbox:" << (~query.operations() & (StorageOperation::Set|StorageOperation::Toggle)) << "not supported for a mutable query", );
        /** @todo clean this up once I can use C++14 named captures */
        UserInterface& ui = this->ui();
        /* Capturing the layer and layer data handle separately to make the
           full capture fit into 12 bytes (instead of 8 + 4 which aligns to 16)
           on 32-bit, and the handle would have to be split up inside anyway */
        const LayerHandle dataLayer = dataHandleLayer(_dataBindingData);
        const LayerDataHandle dataBindingData = dataHandleData(_dataBindingData);
        ui.eventLayer().onTapOrClick(node(), [&ui, dataLayer, dataBindingData] {
            ui.layer<DataLayer>(dataLayer).toggle(dataBindingData);
        });
    }
}

Checkbox::Checkbox(const Anchor anchor, const StorageQuery<bool>& query,const Containers::StringView text, const CheckboxStyle style): Checkbox{anchor, query, text, {}, style} {}

Checkbox::Checkbox(NonOwnedT, const Anchor anchor, const StorageQuery<bool>& query, const Containers::StringView text, const TextProperties& textProperties, const CheckboxStyle style): Checkbox{anchor, query, text, textProperties, style} {
    makeNonOwned();
}

Checkbox::Checkbox(NonOwnedT, const Anchor anchor, const StorageQuery<bool>& query, const Containers::StringView text, const CheckboxStyle style): Checkbox{anchor, query, text, style} {
    makeNonOwned();
}

Checkbox& Checkbox::setStyle(const CheckboxStyle style) {
    _style = style;
    ui().baseLayer().transitionStyle(_backgroundData, style == CheckboxStyle::RadioButton ? BaseStyle::RadioButton : BaseStyle::Checkbox);
    ui().textLayer().setGlyph(_checkboxData, _style == CheckboxStyle::RadioButton ? Icon::RadioButton : Icon::Checkbox, {});
    return *this;
}

Checkbox& Checkbox::setText(const Containers::StringView text, const TextProperties& textProperties) {
    ui().textLayer().setText(_textData, text, textProperties);
    return *this;
}

Checkbox& Checkbox::setText(const Containers::StringView text) {
    return setText(text, {});
}

bool Checkbox::isChecked() const {
    return ui().layer<DataLayer>(dataHandleLayer(_dataBindingData)).get<bool>(_dataBindingData);
}

Checkbox& Checkbox::setChecked(const bool checked) {
    /* Do nothing if the data binding is immutable. The actual change to the
       icon glyph is done by the next onUpdate() callback, if any. */
    DataLayer& layer = ui().layer<DataLayer>(dataHandleLayer(_dataBindingData));
    if(layer.isMutable(_dataBindingData))
        layer.set(_dataBindingData, checked);
    return *this;
}

Checkbox& Checkbox::toggleChecked() {
    /* Do nothing if the data binding is immutable. The actual change to the
       icon glyph is done by the next onUpdate() callback, if any. */
    DataLayer& layer = ui().layer<DataLayer>(dataHandleLayer(_dataBindingData));
    if(layer.isMutable(_dataBindingData))
        layer.toggle(_dataBindingData);
    return *this;
}

DataHandle Checkbox::backgroundData() const {
    /* The background is implicitly from the base layer. It can be null only
       for a NoCreate'd instance, otherwise not. */
    return _backgroundData == LayerDataHandle::Null ? DataHandle::Null :
        dataHandle(ui().baseLayer(), _backgroundData);
}

DataHandle Checkbox::checkboxData() const {
    /* The checkbox is implicitly from the text layer. It can be null only for
       a NoCreate'd instance, otherwise not. */
    return _checkboxData == LayerDataHandle::Null ? DataHandle::Null :
        dataHandle(ui().textLayer(), _checkboxData);
}

DataHandle Checkbox::textData() const {
    /* The text is implicitly from the text layer. It can be null only for a
       NoCreate'd instance, otherwise not. */
    return _textData == LayerDataHandle::Null ? DataHandle::Null :
        dataHandle(ui().textLayer(), _textData);
}

Anchor checkbox(Anchor anchor, const StorageQuery<bool>& query, Containers::StringView text, const TextProperties& textProperties, CheckboxStyle style) {
    Checkbox{NonOwned, anchor, query, text, textProperties, style};
    return anchor;
}

Anchor checkbox(Anchor anchor, const StorageQuery<bool>& query, Containers::StringView text, CheckboxStyle style) {
    Checkbox{NonOwned, anchor, query, text, style};
    return anchor;
}

Anchor radioButton(Anchor anchor, const StorageQuery<bool>& query, Containers::StringView text, const TextProperties& textProperties) {
    return checkbox(anchor, query, text, textProperties, CheckboxStyle::RadioButton);
}

Anchor radioButton(Anchor anchor, const StorageQuery<bool>& query, Containers::StringView text) {
    return checkbox(anchor, query, text, CheckboxStyle::RadioButton);
}

}}
