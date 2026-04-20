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

#include "Label.h"

#include <Corrade/Containers/StringView.h>
#include <Corrade/Containers/Function.h> /* for DataLayer::onUpdate() */
#include <Corrade/Utility/Assert.h>
#include <Magnum/Math/Time.h>
#include <Magnum/Math/Vector2.h>
#include <Magnum/Text/Alignment.h>

#include "Magnum/Ui/AbstractTheme.hpp"
#include "Magnum/Ui/Anchor.h"
#include "Magnum/Ui/BaseLayer.h"
#include "Magnum/Ui/DataLayer.h"
#include "Magnum/Ui/Formatter.h"
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/Icon.h"
#include "Magnum/Ui/LayoutLayer.h"
#include "Magnum/Ui/TextLayer.h"
#include "Magnum/Ui/TextProperties.h"
#include "Magnum/Ui/Theme.h"
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
        _c(Title)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "(" << Debug::nospace << Debug::hex << UnsignedByte(value) << Debug::nospace << ")";
}

namespace {

using Implementation::TextStyle;
using Implementation::LayoutStyle;

TextStyle textStyleIcon(const LabelStyle style) {
    switch(style) {
        #define _c(style) case LabelStyle::style: return TextStyle::Label ## style ## Icon;
        _c(Default)
        _c(Primary)
        _c(Success)
        _c(Warning)
        _c(Danger)
        _c(Info)
        _c(Dim)
        _c(Title)
        #undef _c
    }

    CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
}

TextStyle textStyleText(const LabelStyle style) {
    switch(style) {
        #define _c(style) case LabelStyle::style: return TextStyle::Label ## style ## Text;
        _c(Default)
        _c(Primary)
        _c(Success)
        _c(Warning)
        _c(Danger)
        _c(Info)
        _c(Dim)
        _c(Title)
        #undef _c
    }

    CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
}

LayoutStyle layoutStyle(const LabelStyle style) {
    switch(style) {
        case LabelStyle::Default:
        case LabelStyle::Primary:
        case LabelStyle::Success:
        case LabelStyle::Warning:
        case LabelStyle::Danger:
        case LabelStyle::Info:
        case LabelStyle::Dim:
            return LayoutStyle::Label;
        case LabelStyle::Title:
            return LayoutStyle::LabelTitle;
    }

    CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
}

}

Label::Label(const Anchor anchor, const Icon icon, const LabelStyle style): Widget{anchor}, _style{style}, _icon{icon} {
    _layoutData = dataHandleData(ui().layoutLayer().create(layoutStyle(style), node()));

    _data = icon == Icon::None ? LayerDataHandle::Null :
        dataHandleData(ui().textLayer().createGlyph(textStyleIcon(style), icon, {}, node()));

    /* No data binding in this case */
    _dataBindingData = LayerDataHandle::Null;
}

Label::Label(const Anchor anchor, const Containers::StringView text, const TextProperties& textProperties, const LabelStyle style): Widget{anchor}, _style{style}, _icon{Icon::None} {
    _layoutData = dataHandleData(ui().layoutLayer().create(layoutStyle(style), node()));

    _data = !text ? LayerDataHandle::Null :
        dataHandleData(ui().textLayer().create(textStyleText(style), text, textProperties, node()));

    /* No data binding in this case */
    _dataBindingData = LayerDataHandle::Null;
}

Label::Label(const Anchor anchor, const Containers::StringView text, const LabelStyle style): Label{anchor, text, {}, style} {}

template<class T, class Formatter> Label::Label(std::nullptr_t, const Anchor anchor, const StorageQuery<T>& query, const Formatter& formatter, const LabelStyle style): Widget{anchor}, _style{style}, _icon{Icon::None} {
    /** @todo cannot delegate to the non-data-binding constructor because it
        completely omits the text data if empty, any better solution so I don't
        duplicate this three times? */

    _layoutData = dataHandleData(ui().layoutLayer().create(layoutStyle(style), node()));

    _data = dataHandleData(ui().textLayer().create(textStyleText(style), {}, {}, node()));
    /* Leaving query validity checks on doUpdate() as that'd be a lot of
       duplication and the assumption is that StorageQuery is short-lived */
    _dataBindingData = dataHandleData(ui().dataLayer().onUpdate(query,
        /* MSVC 2017 doesn't actually make a copy with {}, subsequently failing
           because attach() cannot be called on a const reference. Lol. */
        #ifndef CORRADE_MSVC2017_COMPATIBILITY
        Formatter{formatter}
        #else
        Formatter(formatter)
        #endif
            .attach(ui().textLayer(), _data),
        node()));
}

Label::Label(const Anchor anchor, const StorageQuery<Int>& query, const DecimalFormatter& formatter, const LabelStyle style): Label{nullptr, anchor, query, formatter, style} {}

Label::Label(const Anchor anchor, const StorageQuery<UnsignedInt>& query, const DecimalFormatter& formatter, const LabelStyle style): Label{nullptr, anchor, query, formatter, style} {}

Label::Label(const Anchor anchor, const StorageQuery<Long>& query, const DecimalFormatter& formatter, const LabelStyle style): Label{nullptr, anchor, query, formatter, style} {}

Label::Label(const Anchor anchor, const StorageQuery<UnsignedLong>& query, const DecimalFormatter& formatter, const LabelStyle style): Label{nullptr, anchor, query, formatter, style} {}

Label::Label(const Anchor anchor, const StorageQuery<Int>& query, const HexadecimalFormatter& formatter, const LabelStyle style): Label{nullptr, anchor, query, formatter, style} {}

Label::Label(const Anchor anchor, const StorageQuery<UnsignedInt>& query, const HexadecimalFormatter& formatter, const LabelStyle style): Label{nullptr, anchor, query, formatter, style} {}

Label::Label(const Anchor anchor, const StorageQuery<Long>& query, const HexadecimalFormatter& formatter, const LabelStyle style): Label{nullptr, anchor, query, formatter, style} {}

Label::Label(const Anchor anchor, const StorageQuery<UnsignedLong>& query, const HexadecimalFormatter& formatter, const LabelStyle style): Label{nullptr, anchor, query, formatter, style} {}

Label::Label(const Anchor anchor, const StorageQuery<Int>& query, const LabelStyle style): Label{anchor, query, DecimalFormatter{}, style} {}

Label::Label(const Anchor anchor, const StorageQuery<UnsignedInt>& query, const LabelStyle style): Label{anchor, query, DecimalFormatter{}, style} {}

Label::Label(const Anchor anchor, const StorageQuery<Long>& query, const LabelStyle style): Label{anchor, query, DecimalFormatter{}, style} {}

Label::Label(const Anchor anchor, const StorageQuery<UnsignedLong>& query, const LabelStyle style): Label{anchor, query, DecimalFormatter{}, style} {}

Label::Label(const Anchor anchor, const StorageQuery<Float>& query, const FloatFormatter& formatter, const LabelStyle style): Label{nullptr, anchor, query, formatter, style} {}

Label::Label(const Anchor anchor, const StorageQuery<Double>& query, const FloatFormatter& formatter, const LabelStyle style): Label{nullptr, anchor, query, formatter, style} {}

Label::Label(const Anchor anchor, const StorageQuery<Float>& query, const LabelStyle style): Label{anchor, query, FloatFormatter{}, style} {}

Label::Label(const Anchor anchor, const StorageQuery<Double>& query, const LabelStyle style): Label{anchor, query, FloatFormatter{}, style} {}

Label::Label(const Anchor anchor, const StorageQuery<Containers::StringView>& query, const LabelStyle style): Widget{anchor}, _style{style}, _icon{Icon::None} {
    /** @todo cannot delegate to the non-data-binding constructor because it
        completely omits the text data if empty, any better solution so I don't
        duplicate this three times? */

    _layoutData = dataHandleData(ui().layoutLayer().create(layoutStyle(style), node()));

    _data = dataHandleData(ui().textLayer().create(textStyleText(style), {}, {}, node()));
    {
        /** @todo clean this up once I can use C++14 named captures */
        TextLayer& textLayer = ui().textLayer();
        const LayerDataHandle data = _data;
        /* Leaving query validity checks on doUpdate() as that'd be a lot of
           duplication, the assumption is that StorageQuery is short-lived */
        _dataBindingData = dataHandleData(ui().dataLayer().onUpdate(query, [&textLayer, data](const Containers::StringView value) {
            textLayer.setText(data, value, {});
        }, node()));
    }
}

Label::Label(const Anchor anchor, const StorageQuery<Icon>& query, const LabelStyle style): Widget{anchor}, _style{style}, _icon{Icon::None} {
    /** @todo cannot delegate to the non-data-binding constructor because it
        completely omits the text data if empty, any better solution so I don't
        duplicate this three times? */

    _layoutData = dataHandleData(ui().layoutLayer().create(layoutStyle(style), node()));

    _data = dataHandleData(ui().textLayer().createGlyph(textStyleIcon(style), 0, {}, node()));
    {
        /** @todo clean this up once I can use C++14 named captures */
        TextLayer& textLayer = ui().textLayer();
        const LayerDataHandle data = _data;
        /* Leaving query validity checks on doUpdate() as that'd be a lot of
           duplication, the assumption is that StorageQuery is short-lived */
        _dataBindingData = dataHandleData(ui().dataLayer().onUpdate(query, [&textLayer, data](const Icon value) {
            textLayer.setGlyph(data, value, {});
        }, node()));
    }
}

Label::Label(NonOwnedT, const Anchor anchor, const Icon icon, const LabelStyle style): Label{anchor, icon, style} {
    makeNonOwned();
}

Label::Label(NonOwnedT, const Anchor anchor, const Containers::StringView text, const TextProperties& textProperties, const LabelStyle style): Label{anchor, text, textProperties, style} {
    makeNonOwned();
}

Label::Label(NonOwnedT, const Anchor anchor, const Containers::StringView text, const LabelStyle style): Label{anchor, text, style} {
    makeNonOwned();
}

Label& Label::setStyle(const LabelStyle style) {
    return setStyleInternal(style);
}

Label& Label::setStyle(const LabelStyle style, const Nanoseconds time) {
    return setStyleInternal(style, time);
}

template<class ...Args> Label& Label::setStyleInternal(const LabelStyle style, Args... args) {
    _style = style;
    ui().layoutLayer().setStyle(_layoutData, layoutStyle(style));
    if(_data != LayerDataHandle::Null)
        ui().textLayer().transitionStyle(_data, (_icon == Icon::None ? textStyleText : textStyleIcon)(style), args...);
    return *this;
}

Label& Label::setIcon(const Icon icon) {
    CORRADE_ASSERT(_dataBindingData == LayerDataHandle::Null,
        "Ui::Label::setIcon(): can't be called with a data binding present", *this);

    TextLayer& textLayer = ui().textLayer();

    _icon = icon;
    if(icon != Icon::None) {
        if(_data == LayerDataHandle::Null)
            _data = dataHandleData(textLayer.createGlyph(textStyleIcon(_style), icon, {}, node()));
        else textLayer.setGlyph(_data, icon, {});
    } else if(_data != LayerDataHandle::Null) {
        textLayer.remove(_data);
        _data = LayerDataHandle::Null;
    }

    return *this;
}

Label& Label::setText(const Containers::StringView text, const TextProperties& textProperties) {
    CORRADE_ASSERT(_dataBindingData == LayerDataHandle::Null,
        "Ui::Label::setText(): can't be called with a data binding present", *this);

    TextLayer& textLayer = ui().textLayer();

    _icon = Icon::None;
    if(text) {
        if(_data == LayerDataHandle::Null)
            _data = dataHandleData(textLayer.create(textStyleText(_style), text, textProperties, node()));
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

DataHandle Label::layoutData() const {
    /* The data is implicitly from the layout layer. The _layoutData can be
       null only with a NoCreated instance. */
    return _layoutData == LayerDataHandle::Null ? DataHandle::Null :
        dataHandle(ui().layoutLayer(), _layoutData);
}

DataHandle Label::dataBindingData() const {
    /* The data is implicitly from the data layer */
    return _dataBindingData == LayerDataHandle::Null ? DataHandle::Null :
        dataHandle(ui().dataLayer(), _dataBindingData);
}

Anchor label(const Anchor anchor, const Containers::StringView text, const TextProperties& textProperties, const LabelStyle style) {
    Label{NonOwned, anchor, text, textProperties, style};
    return anchor;
}

Anchor label(const Anchor anchor, const Containers::StringView text, const LabelStyle style) {
    return label(anchor, text, {}, style);
}

Anchor label(const Anchor anchor, const Icon icon, const LabelStyle style) {
    Label{NonOwned, anchor, icon, style};
    return anchor;
}

}}
