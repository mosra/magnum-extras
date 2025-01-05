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

#include "UserInterface.h"

#include <Corrade/Utility/Assert.h>
#include <Magnum/Math/Time.h>

#include "Magnum/Ui/BaseLayer.h"
#include "Magnum/Ui/EventLayer.h"
#include "Magnum/Ui/SnapLayouter.h"
#include "Magnum/Ui/TextLayer.h"
#include "Magnum/Ui/Implementation/userInterfaceState.h"

namespace Magnum { namespace Ui {

UserInterface::UserInterface(NoCreateT, Containers::Pointer<State>&& state): AbstractUserInterface{NoCreate}, _state{Utility::move(state)} {}

UserInterface::UserInterface(NoCreateT): UserInterface{NoCreate, Containers::pointer<State>()} {}

UserInterface::UserInterface(UserInterface&&) noexcept = default;

UserInterface::~UserInterface() = default;

UserInterface& UserInterface::operator=(UserInterface&&) noexcept = default;

UserInterface& UserInterface::setBaseLayerInstance(Containers::Pointer<BaseLayer>&& instance) {
    CORRADE_ASSERT(instance,
        "Ui::UserInterface::setBaseLayerInstance(): instance is null", *this);
    CORRADE_ASSERT(!_state->baseLayer,
        "Ui::UserInterface::setBaseLayerInstance(): instance already set", *this);
    _state->baseLayer = instance.get();
    setLayerInstance(Utility::move(instance));
    return *this;
}

UserInterface& UserInterface::setTextLayerInstance(Containers::Pointer<TextLayer>&& instance) {
    CORRADE_ASSERT(instance,
        "Ui::UserInterface::setTextLayerInstance(): instance is null", *this);
    CORRADE_ASSERT(!_state->textLayer,
        "Ui::UserInterface::setTextLayerInstance(): instance already set", *this);
    _state->textLayer = instance.get();
    setLayerInstance(Utility::move(instance));
    return *this;
}

UserInterface& UserInterface::setEventLayerInstance(Containers::Pointer<EventLayer>&& instance) {
    CORRADE_ASSERT(instance,
        "Ui::UserInterface::setEventLayerInstance(): instance is null", *this);
    CORRADE_ASSERT(!_state->eventLayer,
        "Ui::UserInterface::setEventLayerInstance(): instance already set", *this);
    _state->eventLayer = instance.get();
    setLayerInstance(Utility::move(instance));
    return *this;
}

UserInterface& UserInterface::setSnapLayouterInstance(Containers::Pointer<SnapLayouter>&& instance) {
    CORRADE_ASSERT(instance,
        "Ui::UserInterface::setSnapLayouterInstance(): instance is null", *this);
    CORRADE_ASSERT(!_state->snapLayouter,
        "Ui::UserInterface::setSnapLayouterInstance(): instance already set", *this);
    _state->snapLayouter = instance.get();
    setLayouterInstance(Utility::move(instance));
    return *this;
}

bool UserInterface::hasBaseLayer() const {
    return _state->baseLayer;
}

const BaseLayer& UserInterface::baseLayer() const {
    CORRADE_ASSERT(_state->baseLayer,
        "Ui::UserInterface::baseLayer(): no instance set", *_state->baseLayer);
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
        "Ui::UserInterface::textLayer(): no instance set", *_state->textLayer);
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
        "Ui::UserInterface::eventLayer(): no instance set", *_state->eventLayer);
    return *_state->eventLayer;
}

EventLayer& UserInterface::eventLayer() {
    return const_cast<EventLayer&>(const_cast<const UserInterface&>(*this).eventLayer());
}

bool UserInterface::hasSnapLayouter() const {
    return _state->snapLayouter;
}

const SnapLayouter& UserInterface::snapLayouter() const {
    CORRADE_ASSERT(_state->snapLayouter,
        "Ui::UserInterface::snapLayouter(): no instance set", *_state->snapLayouter);
    return *_state->snapLayouter;
}

SnapLayouter& UserInterface::snapLayouter() {
    return const_cast<SnapLayouter&>(const_cast<const UserInterface&>(*this).snapLayouter());
}

UserInterface& UserInterface::advanceAnimations(const Nanoseconds time) {
    return static_cast<UserInterface&>(AbstractUserInterface::advanceAnimations(time));
}

}}
