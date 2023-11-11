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

#include "UserInterface.h"

#include <Corrade/Utility/Algorithms.h>
#include <Corrade/Utility/Resource.h>
#include <Magnum/Text/AbstractFont.h>
#include <Magnum/Text/GlyphCache.h>
#include <Magnum/Math/Vector2.h>

#include "Magnum/Whee/BaseLayer.h"
#include "Magnum/Whee/EventLayer.h"
#include "Magnum/Whee/TextLayer.h"
#include "Magnum/Whee/Implementation/userInterfaceState.h"

namespace Magnum { namespace Whee {

UserInterface::UserInterface(NoCreateT, Containers::Pointer<State>&& state): AbstractUserInterface{NoCreate}, _state{Utility::move(state)} {}

UserInterface::UserInterface(NoCreateT): UserInterface{NoCreate, Containers::pointer<State>()} {}

UserInterface::UserInterface(UserInterface&&) noexcept = default;

UserInterface::~UserInterface() = default;

UserInterface& UserInterface::operator=(UserInterface&&) noexcept = default;

UserInterface& UserInterface::setBaseLayerInstance(Containers::Pointer<BaseLayer>&& instance) {
    CORRADE_ASSERT(instance,
        "Whee::UserInterface::setBaseLayerInstance(): instance is null", *this);
    CORRADE_ASSERT(!_state->baseLayer,
        "Whee::UserInterface::setBaseLayerInstance(): instance already set", *this);
    _state->baseLayer = instance.get();
    setLayerInstance(Utility::move(instance));
    return *this;
}

UserInterface& UserInterface::setTextLayerInstance(Containers::Pointer<TextLayer>&& instance) {
    CORRADE_ASSERT(instance,
        "Whee::UserInterface::setTextLayerInstance(): instance is null", *this);
    CORRADE_ASSERT(!_state->textLayer,
        "Whee::UserInterface::setTextLayerInstance(): instance already set", *this);
    _state->textLayer = instance.get();
    setLayerInstance(Utility::move(instance));
    return *this;
}

UserInterface& UserInterface::setEventLayerInstance(Containers::Pointer<EventLayer>&& instance) {
    CORRADE_ASSERT(instance,
        "Whee::UserInterface::setEventLayerInstance(): instance is null", *this);
    CORRADE_ASSERT(!_state->eventLayer,
        "Whee::UserInterface::setEventLayerInstance(): instance already set", *this);
    _state->eventLayer = instance.get();
    setLayerInstance(Utility::move(instance));
    return *this;
}

bool UserInterface::hasBaseLayer() const {
    return _state->baseLayer;
}

const BaseLayer& UserInterface::baseLayer() const {
    CORRADE_ASSERT(_state->baseLayer,
        "Whee::UserInterface::baseLayer(): no instance set", *_state->baseLayer);
    return *_state->baseLayer;
}

BaseLayer& UserInterface::baseLayer() {
    return const_cast<BaseLayer&>(const_cast<const UserInterface&>(*this).baseLayer());
}

bool UserInterface::hasTextLayer() const {
    return _state->textLayer;
}

const TextLayer& UserInterface::textLayer() const {
    CORRADE_ASSERT(_state->textLayer,
        "Whee::UserInterface::textLayer(): no instance set", *_state->textLayer);
    return *_state->textLayer;
}

TextLayer& UserInterface::textLayer() {
    return const_cast<TextLayer&>(const_cast<const UserInterface&>(*this).textLayer());
}

bool UserInterface::hasEventLayer() const {
    return _state->eventLayer;
}

const EventLayer& UserInterface::eventLayer() const {
    CORRADE_ASSERT(_state->eventLayer,
        "Whee::UserInterface::eventLayer(): no instance set", *_state->eventLayer);
    return *_state->eventLayer;
}

EventLayer& UserInterface::eventLayer() {
    return const_cast<EventLayer&>(const_cast<const UserInterface&>(*this).eventLayer());
}

}}
