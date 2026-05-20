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

#include "Input.h"

#include <Corrade/Containers/StringView.h>
#include <Corrade/Utility/Assert.h>
#include <Magnum/Math/Time.h>
#include <Magnum/Math/Vector2.h>
#include <Magnum/Text/Alignment.h>

#include "Magnum/Ui/AbstractTheme.hpp"
#include "Magnum/Ui/Anchor.h"
#include "Magnum/Ui/BaseLayer.h"
#include "Magnum/Ui/DataLayer.h"
#include "Magnum/Ui/EventLayer.h"
#include "Magnum/Ui/Formatter.h"
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/LayoutLayer.h"
#include "Magnum/Ui/NodeFlags.h"
#include "Magnum/Ui/TextLayer.h"
#include "Magnum/Ui/TextProperties.h"
#include "Magnum/Ui/UserInterface.h"

namespace Magnum { namespace Ui {

using Implementation::BaseStyle;
using Implementation::TextStyle;
using Implementation::LayoutStyle;

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

BaseStyle baseStyle(const InputStyle style) {
    switch(style) {
        #define _c(style) case InputStyle::style: return BaseStyle::Input ## style;
        _c(Default)
        _c(Success)
        _c(Warning)
        _c(Danger)
        _c(Flat)
        #undef _c
    }

    CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
}

TextStyle textStyle(const InputStyle style) {
    switch(style) {
        #define _c(style) case InputStyle::style: return TextStyle::Input ## style;
        _c(Default)
        _c(Success)
        _c(Warning)
        _c(Danger)
        _c(Flat)
        #undef _c
    }

    CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
}

}

Input::Input(const Anchor anchor, const Containers::StringView text, const TextProperties& textProperties, const InputStyle style, Implementation::TextStyle(*const textStyle)(InputStyle)): Widget{anchor}, _style{style} {
    ui().addNodeFlags(node(), NodeFlag::Focusable);
    ui().layoutLayer().create(LayoutStyle::Input, node());

    _backgroundData = dataHandleData(ui().baseLayer().create(baseStyle(style), node()));
    _textData = dataHandleData(ui().textLayer().create(textStyle(style), text, textProperties, TextDataFlag::Editable, node()));

    /* No data binding in this case */
    _dataBindingData = DataHandle::Null;
}

Input::Input(const Anchor anchor, const Containers::StringView text, const TextProperties& textProperties, const InputStyle style): Input{anchor, text, textProperties, style, textStyle} {}

Input::Input(const Anchor anchor, const Containers::StringView text, const InputStyle style): Input{anchor, text, {}, style} {}

Input::Input(const Anchor anchor, const InputStyle style): Input{anchor, {}, style} {}

Input::Input(std::nullptr_t, const Anchor anchor, const InputStyle style): Widget{anchor}, _style{style} {
    ui().addNodeFlags(node(), NodeFlag::Focusable);
    ui().layoutLayer().create(LayoutStyle::Input, node());

    _backgroundData = dataHandleData(ui().baseLayer().create(baseStyle(style), node()));
    _textData = dataHandleData(ui().textLayer().create(textStyle(style), {}, {}, TextDataFlag::Editable, node()));

    /* The _dataBindingData handle is filled by the delegating constructor */
}

template<class T, class Formatter> Input::Input(std::nullptr_t, const Anchor anchor, const StorageQuery<T>& query, const Formatter& formatter, ParseState(*const parser)(Containers::StringView, T&), const InputStyle style): Input{nullptr, anchor, style} {
    {
        /** @todo clean this up once I can use C++14 named captures */
        UserInterface& ui = this->ui();
        const LayerDataHandle textData = _textData;
        /* Leaving query validity checks on doUpdate() as that'd be a lot of
           duplication, the assumption is that StorageQuery is short-lived */
        _dataBindingData = query.onUpdate([&ui, textData, formatter](DataHandle handle, T value) {
            /* Skip the update if the text node is currently focused (and thus
               likely being edited). Not doing so would result in e.g. `30.`
               being forcibly changed back to `30`, making it extremely
               annoying to type anything. OTOH, if the data binding itself is
               marked dirty, which is done for example by the onScroll()
               callbacks below, perform the update so the increment/decrement
               is reflected right away. */
            if(ui.isNodeFocused(ui.textLayer().node(textData)) && !ui.layer<DataLayer>(dataHandleLayer(handle)).isDirty(handle))
                return;

            /** @todo expose options to select all / move cursor at the end /
                at the beginning etc. after an update, e.g. Qt selects
                everything on increment while Gtk / Inkscape moves cursor at
                the end */
            formatter(ui.textLayer(), textData, value);

            /** @todo reset styles back to default on update directly here,
                once there's a DataLayerAnimator or some such that provides a
                time input and makes it possible to do it without delaying a
                possible transition animation by a frame (could use
                `ui.animationTime()` here, but the callback is being run during
                `ui.update()`, which is after `ui.advanceAnimations()` and thus
                any animation will happen only on next advance) */
        }, node());
    }

    /* The rest doesn't depend on the Formatter type, delegate to a
       less-templated helper to reduce generated code duplication */
    createInternal(parser ? parser : static_cast<ParseState(*)(Containers::StringView, T&)>(Formatter::parse));
}

template<class T> void Input::createInternal(ParseState(*const parser)(Containers::StringView, T&)) {
    /** @todo the captured data here are 33 bytes, 9 bytes more than can fit
        in-place on 64-bit, reevaluate once there's more insight from other
        widgets and further extending input functionality (could maybe set the
        edit callback only when focused, and then remove it again? that way
        it's one allocation globally at most, not one allocation per input
        widget; or maybe put the static data in some input-internal storage
        that's referenced from here?) */
    {
        /** @todo clean this up once I can use C++14 named captures */
        UserInterface& ui = this->ui();
        const LayerDataHandle backgroundData = _backgroundData;
        const LayerDataHandle textData = _textData;
        const DataHandle dataBindingData = _dataBindingData;
        const InputStyle style = _style;
        ui.textLayer().setTextEditCallback(_textData, [dataBindingData, &ui, parser, backgroundData, textData, style](const Nanoseconds time, const Containers::StringView text) {
            /* Parse the value */
            T value;
            const ParseState parseState = parser(text, value);

            /* Update the storage and provide visual feedback if either parsing
               or updating didn't succeed */
            InputStyle targetStyle;
            if(parseState == ParseState::Failed) {
                targetStyle = InputStyle::Danger;
            } else {
                const StorageUpdateState updateState = ui.layer<DataLayer>(dataHandleLayer(dataBindingData)).set(dataBindingData, value);
                if(parseState == ParseState::Clamped ||
                   updateState == StorageUpdateState::Clamped) {
                    targetStyle = InputStyle::Warning;
                } else {
                    CORRADE_INTERNAL_ASSERT(parseState == ParseState::Success);
                    if(updateState == StorageUpdateState::Failed)
                        targetStyle = InputStyle::Danger;
                    else if(updateState == StorageUpdateState::Approximated ||
                            updateState == StorageUpdateState::Success)
                        targetStyle = style;
                    else CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
                }
            }
            ui.baseLayer().transitionStyle(backgroundData, baseStyle(targetStyle), time);
            /** @todo use passwordTextStyle() instead of textStyle() once
            PasswordInput gets data bindings as well */
            ui.textLayer().transitionStyle(textData, textStyle(targetStyle), time);
        });
    }


    /* The rest doesn't depend on T, delegate to a non-templated helper to
       reduce generated code duplication */
    createInternal();
}

void Input::createInternal() {
    const StorageOperations operations = ui().layer<DataLayer>(dataHandleLayer(_dataBindingData)).operations(_dataBindingData);

    /* Done only here to not have it duplicated for all T and Formatter
       combinations */
    CORRADE_ASSERT(operations >= StorageOperation::Set,
        "Ui::Input:" << StorageOperation::Set << "not supported", );

    /** @todo allocates as well, see setTextEditCallback() above for details */
    {
        /** @todo clean this up once I can use C++14 named captures */
        UserInterface& ui = this->ui();
        const LayerDataHandle backgroundData = _backgroundData;
        const LayerDataHandle textData = _textData;
        const DataHandle dataBindingData = _dataBindingData;
        const InputStyle style = _style;
        ui.eventLayer().onBlur(node(), [dataBindingData, &ui, backgroundData, textData, style] {
            /* Mark the data binding as dirty to update it to the currently
               stored value, discarding any potential editing warnings or
               failures */
            ui.layer<DataLayer>(dataHandleLayer(dataBindingData)).setDirty(dataBindingData);

            /* Updating the value from storage resets any non-success edit
               state so reset the style back to default if it isn't */
            /** @todo cannot pass the time to transitionStyle() as it causes an
                animation conflict with the blur transition animation, leading
                to a wrong style something being applied (such as the text
                staying yellow, and potentially worse if the builtin theme
                supplies transition animations as well, which it so far
                doesn't), pass the time once this is resolved */
            /** @todo move this style reset to onUpdate() to not duplicate it
                in every event, once onUpdate() can handle animations */
            ui.baseLayer().transitionStyle(backgroundData, baseStyle(style));
            /** @todo use passwordTextStyle() instead of textStyle() once
                PasswordInput gets data bindings as well */
            ui.textLayer().transitionStyle(textData, textStyle(style));
        });
    }

    /* If the query supports increment/decrement, add appropriate event
       handlers */
    /** @todo allocates as well, see setTextEditCallback() above for details */
    if(operations >= (StorageOperation::Increment|StorageOperation::Decrement)) {
        /** @todo clean this up once I can use C++14 named captures */
        UserInterface& ui = this->ui();
        const LayerDataHandle backgroundData = _backgroundData;
        const LayerDataHandle textData = _textData;
        const DataHandle dataBindingData = _dataBindingData;
        const InputStyle style = _style;
        ui.eventLayer().onScroll(node(), [dataBindingData, &ui, backgroundData, textData, style](const Nanoseconds time, const Vector2&, const Vector2& offset) {
            DataLayer& dataLayer = ui.layer<DataLayer>(dataHandleLayer(dataBindingData));

            /* Horizontal scrolling is usually meant for scrolling the cursor
               left/right, so pick just the vertical delta if there's any, and
               bail if isn't. Scroll up is a positive offset and increases the
               value, which is consistent with e.g. Qt. */
            if(offset.y() > 0)
                dataLayer.increment(dataBindingData);
            else if(offset.y() < 0)
                dataLayer.decrement(dataBindingData);
            else return;

            /* The storage is marked as dirty from the above, but the text
               wouldn't update if the input is currently focused. To override
               that, mark the data binding as dirty as well. */
            dataLayer.setDirty(dataBindingData);

            /* The increment/decrement resets any non-success edit state so
               reset the style back to default if it isn't */
            /** @todo move this style reset to onUpdate() to not duplicate it
                in every event, once onUpdate() can handle animations */
            ui.baseLayer().transitionStyle(backgroundData, baseStyle(style), time);
            ui.textLayer().transitionStyle(textData, textStyle(style), time);
        });
    }
}

Input::Input(const Anchor anchor, const StorageQuery<Int>& query, const DecimalFormatter& formatter, ParseState(*const parser)(Containers::StringView, Int&), const InputStyle style): Input{nullptr, anchor, query, formatter, parser, style} {}

Input::Input(const Anchor anchor, const StorageQuery<UnsignedInt>& query, const DecimalFormatter& formatter, ParseState(*const parser)(Containers::StringView, UnsignedInt&), const InputStyle style): Input{nullptr, anchor, query, formatter, parser, style} {}

Input::Input(const Anchor anchor, const StorageQuery<Long>& query, const DecimalFormatter& formatter, ParseState(*const parser)(Containers::StringView, Long&), const InputStyle style): Input{nullptr, anchor, query, formatter, parser, style} {}

Input::Input(const Anchor anchor, const StorageQuery<UnsignedLong>& query, const DecimalFormatter& formatter, ParseState(*const parser)(Containers::StringView, UnsignedLong&), const InputStyle style): Input{nullptr, anchor, query, formatter, parser, style} {}

Input::Input(const Anchor anchor, const StorageQuery<Int>& query, const HexadecimalFormatter& formatter, ParseState(*const parser)(Containers::StringView, Int&), const InputStyle style): Input{nullptr, anchor, query, formatter, parser, style} {}

Input::Input(const Anchor anchor, const StorageQuery<UnsignedInt>& query, const HexadecimalFormatter& formatter, ParseState(*const parser)(Containers::StringView, UnsignedInt&), const InputStyle style): Input{nullptr, anchor, query, formatter, parser, style} {}

Input::Input(const Anchor anchor, const StorageQuery<Long>& query, const HexadecimalFormatter& formatter, ParseState(*const parser)(Containers::StringView, Long&), const InputStyle style): Input{nullptr, anchor, query, formatter, parser, style} {}

Input::Input(const Anchor anchor, const StorageQuery<UnsignedLong>& query, const HexadecimalFormatter& formatter, ParseState(*const parser)(Containers::StringView, UnsignedLong&), const InputStyle style): Input{nullptr, anchor, query, formatter, parser, style} {}

Input::Input(const Anchor anchor, const StorageQuery<Float>& query, const FloatFormatter& formatter, ParseState(*const parser)(Containers::StringView, Float&), const InputStyle style): Input{nullptr, anchor, query, formatter, parser, style} {}

Input::Input(const Anchor anchor, const StorageQuery<Double>& query, const FloatFormatter& formatter, ParseState(*const parser)(Containers::StringView, Double&), const InputStyle style): Input{nullptr, anchor, query, formatter, parser, style} {}

Input::Input(const Anchor anchor, const StorageQuery<Int>& query, const InputStyle style): Input{anchor, query, DecimalFormatter{}, nullptr, style} {}

Input::Input(const Anchor anchor, const StorageQuery<UnsignedInt>& query, const InputStyle style): Input{anchor, query, DecimalFormatter{}, nullptr, style} {}

Input::Input(const Anchor anchor, const StorageQuery<Long>& query, const InputStyle style): Input{anchor, query, DecimalFormatter{}, nullptr, style} {}

Input::Input(const Anchor anchor, const StorageQuery<UnsignedLong>& query, const InputStyle style): Input{anchor, query, DecimalFormatter{}, nullptr, style} {}

Input::Input(const Anchor anchor, const StorageQuery<Float>& query, const InputStyle style): Input{anchor, query, FloatFormatter{}, nullptr, style} {}

Input::Input(const Anchor anchor, const StorageQuery<Double>& query, const InputStyle style): Input{anchor, query, FloatFormatter{}, nullptr, style} {}

template<class ...Args> Input& Input::setStyleInternal(const InputStyle style, Implementation::TextStyle(*const textStyle)(InputStyle), Args... args) {
    CORRADE_ASSERT(_dataBindingData == DataHandle::Null,
        "Ui::Input::setStyle(): can't be called with a data binding present", *this);
    _style = style;
    ui().baseLayer().transitionStyle(_backgroundData, baseStyle(style), args...);
    ui().textLayer().transitionStyle(_textData, textStyle(style), args...);
    /** @todo re-set the text if font / alignment ... changed */
    return *this;
}

Input& Input::setStyle(const InputStyle style) {
    setStyleInternal(style, textStyle);
    return *this;
}

Input& Input::setStyle(const InputStyle style, Nanoseconds time) {
    setStyleInternal(style, textStyle, time);
    return *this;
}

DataHandle Input::backgroundData() const {
    /* The background is implicitly from the base layer. It can be null only
       for a NoCreate'd instance, otherwise not. */
    return _backgroundData == LayerDataHandle::Null ? DataHandle::Null :
        dataHandle(ui().baseLayer(), _backgroundData);
}

DataHandle Input::textData() const {
    /* The text is implicitly from the text layer. It can be null only
       for a NoCreate'd instance, otherwise not. */
    return _textData == LayerDataHandle::Null ? DataHandle::Null :
        dataHandle(ui().textLayer(), _textData);
}

Containers::StringView Input::text() const {
    return ui().textLayer().text(_textData);
}

Input& Input::setText(const Containers::StringView text, const TextProperties& textProperties) {
    CORRADE_ASSERT(_dataBindingData == DataHandle::Null,
        "Ui::Input::setText(): can't be called with a data binding present", *this);
    ui().textLayer().setText(_textData, text, textProperties);
    return *this;
}

Input& Input::setText(const Containers::StringView text) {
    return setText(text, {});
}

namespace {

TextStyle passwordTextStyle(const InputStyle style) {
    switch(style) {
        #define _c(style) case InputStyle::style: return TextStyle::Input ## style ## Password;
        _c(Default)
        _c(Success)
        _c(Warning)
        _c(Danger)
        _c(Flat)
        #undef _c
    }

    CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
}

}

PasswordInput::PasswordInput(const Anchor anchor, const Containers::StringView text, const TextProperties& textProperties, const InputStyle style): Input{anchor, text, textProperties, style, passwordTextStyle} {}

PasswordInput::PasswordInput(const Anchor anchor, const Containers::StringView text, const InputStyle style): PasswordInput{anchor, text, {}, style} {}

PasswordInput::PasswordInput(const Anchor anchor, const InputStyle style): PasswordInput{anchor, {}, style} {}

PasswordInput& PasswordInput::setStyle(const InputStyle style) {
    setStyleInternal(style, passwordTextStyle);
    return *this;
}

PasswordInput& PasswordInput::setStyle(const InputStyle style, const Nanoseconds time) {
    setStyleInternal(style, passwordTextStyle, time);
    return *this;
}

PasswordInput& PasswordInput::setText(const Containers::StringView text, const TextProperties& textProperties) {
    return static_cast<PasswordInput&>(Input::setText(text, textProperties));
}

PasswordInput& PasswordInput::setText(const Containers::StringView text) {
    return static_cast<PasswordInput&>(Input::setText(text));
}

}}
