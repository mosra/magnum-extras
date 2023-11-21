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

#include "AbstractVisualLayer.h"

#include <Corrade/Containers/StridedArrayView.h>

#include "Magnum/Whee/Event.h"
#include "Magnum/Whee/Handle.h"
#include "Magnum/Whee/Implementation/abstractVisualLayerState.h"

namespace Magnum { namespace Whee {

AbstractVisualLayer::Shared::Shared(Containers::Pointer<State>&& state): _state{Utility::move(state)} {}

AbstractVisualLayer::Shared::Shared(const UnsignedInt styleCount): Shared{Containers::pointer<State>(*this, styleCount)} {}

AbstractVisualLayer::Shared::Shared(NoCreateT) noexcept {}

AbstractVisualLayer::Shared::Shared(Shared&& other) noexcept: _state{Utility::move(other._state)} {
    /* Need to also update the self reference so it points to the new
       instance */
    if(_state)
        _state->self = *this;
}

AbstractVisualLayer::Shared::~Shared() = default;

AbstractVisualLayer::Shared& AbstractVisualLayer::Shared::operator=(Shared&& other) noexcept {
    Utility::swap(other._state, _state);
    /* Need to also update the self references so they point to the new
       instances */
    if(other._state && _state)
        Utility::swap(other._state->self, _state->self);
    else if(other._state)
        other._state->self = other;
    else if(_state)
        _state->self = *this;
    return *this;
}

UnsignedInt AbstractVisualLayer::Shared::styleCount() const {
    return _state->styleCount;
}

AbstractVisualLayer::Shared& AbstractVisualLayer::Shared::setStyleTransition(UnsignedInt(*const toPressedBlur)(UnsignedInt), UnsignedInt(*const toPressedHover)(UnsignedInt), UnsignedInt(*const toInactiveBlur)(UnsignedInt), UnsignedInt(*const toInactiveHover)(UnsignedInt)) {
    _state->styleTransitionToPressedBlur = toPressedBlur ? toPressedBlur :
        Implementation::styleTransitionPassthrough;
    _state->styleTransitionToPressedHover = toPressedHover ? toPressedHover :
        Implementation::styleTransitionPassthrough;
    _state->styleTransitionToInactiveBlur = toInactiveBlur ? toInactiveBlur :
        Implementation::styleTransitionPassthrough;
    _state->styleTransitionToInactiveHover = toInactiveHover ? toInactiveHover :
        Implementation::styleTransitionPassthrough;
    return *this;
}

AbstractVisualLayer::AbstractVisualLayer(const LayerHandle handle, Containers::Pointer<State>&& state): AbstractLayer{handle}, _state{Utility::move(state)} {}

AbstractVisualLayer::AbstractVisualLayer(const LayerHandle handle, Shared& shared): AbstractVisualLayer{handle, Containers::pointer<State>(*shared._state)} {}

AbstractVisualLayer::AbstractVisualLayer(AbstractVisualLayer&&) noexcept = default;

AbstractVisualLayer::~AbstractVisualLayer() = default;

AbstractVisualLayer& AbstractVisualLayer::operator=(AbstractVisualLayer&&) noexcept = default;

const AbstractVisualLayer::Shared& AbstractVisualLayer::shared() const {
    return _state->shared.self;
}

AbstractVisualLayer::Shared& AbstractVisualLayer::shared() {
    return _state->shared.self;
}

UnsignedInt AbstractVisualLayer::style(const DataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::AbstractVisualLayer::style(): invalid handle" << handle, {});
    CORRADE_INTERNAL_DEBUG_ASSERT(_state->styles.size() == capacity());
    return _state->styles[dataHandleId(handle)];
}

UnsignedInt AbstractVisualLayer::style(const LayerDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::AbstractVisualLayer::style(): invalid handle" << handle, {});
    CORRADE_INTERNAL_DEBUG_ASSERT(_state->styles.size() == capacity());
    return _state->styles[layerDataHandleId(handle)];
}

void AbstractVisualLayer::setStyle(const DataHandle handle, const UnsignedInt style) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::AbstractVisualLayer::setStyle(): invalid handle" << handle, );
    CORRADE_ASSERT(style < _state->shared.styleCount,
        "Whee::AbstractVisualLayer::setStyle(): style" << style << "out of range for" << _state->shared.styleCount << "styles", );
    setStyleInternal(dataHandleId(handle), style);
}

void AbstractVisualLayer::setStyle(const LayerDataHandle handle, const UnsignedInt style) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::AbstractVisualLayer::setStyle(): invalid handle" << handle, );
    CORRADE_ASSERT(style < _state->shared.styleCount,
        "Whee::AbstractVisualLayer::setStyle(): style" << style << "out of range for" << _state->shared.styleCount << "styles", );
    setStyleInternal(layerDataHandleId(handle), style);
}

void AbstractVisualLayer::setStyleInternal(const UnsignedInt id, const UnsignedInt style) {
    CORRADE_INTERNAL_DEBUG_ASSERT(_state->styles.size() == capacity());
    _state->styles[id] = style;
    setNeedsUpdate();
}

LayerFeatures AbstractVisualLayer::doFeatures() const {
    return LayerFeature::Event;
}

void AbstractVisualLayer::doPointerPressEvent(const UnsignedInt dataId, PointerEvent& event) {
    /* Only reacting to pointer types typically used to click/tap on things */
    if(event.type() != Pointer::MouseLeft &&
       event.type() != Pointer::Finger &&
       event.type() != Pointer::Pen)
        return;

    /* A press can be not hovering if it happened without a preceding move
       event (such as for pointer types that don't support hover like touches,
       or if move events aren't propagated from the application) */
    const State& state = *_state;
    const Shared::State& sharedState = state.shared;
    UnsignedInt(*const transition)(UnsignedInt) = event.isHovering() ?
        sharedState.styleTransitionToPressedHover :
        sharedState.styleTransitionToPressedBlur;

    CORRADE_INTERNAL_DEBUG_ASSERT(state.styles.size() == capacity());
    UnsignedInt& style = state.styles[dataId];
    const UnsignedInt nextStyle = transition(style);
    CORRADE_ASSERT(nextStyle < sharedState.styleCount,
        "Whee::AbstractVisualLayer::pointerPressEvent(): style transition from" << style << "to" << nextStyle << "out of range for" << sharedState.styleCount << "styles", );
    if(nextStyle != style) {
        style = nextStyle;
        setNeedsUpdate();
    }
    event.setAccepted();
}

void AbstractVisualLayer::doPointerReleaseEvent(const UnsignedInt dataId, PointerEvent& event) {
    /* Only reacting to pointer types typically used to click/tap on things */
    if(event.type() != Pointer::MouseLeft &&
       event.type() != Pointer::Finger &&
       event.type() != Pointer::Pen)
        return;

    /* A release can be not hovering if it happened without a preceding move
       event (such as for pointer types that don't support hover like touches,
       or if move events aren't propagated from the application) */
    const State& state = *_state;
    const Shared::State& sharedState = state.shared;
    UnsignedInt(*const transition)(UnsignedInt) = event.isHovering() ?
        sharedState.styleTransitionToInactiveHover :
        sharedState.styleTransitionToInactiveBlur;

    CORRADE_INTERNAL_DEBUG_ASSERT(state.styles.size() == capacity());
    UnsignedInt& style = state.styles[dataId];
    const UnsignedInt nextStyle = transition(style);
    CORRADE_ASSERT(nextStyle < sharedState.styleCount,
        "Whee::AbstractVisualLayer::pointerReleaseEvent(): style transition from" << style << "to" << nextStyle << "out of range for" << sharedState.styleCount << "styles", );
    if(nextStyle != style) {
        style = nextStyle;
        setNeedsUpdate();
    }
    event.setAccepted();
}

void AbstractVisualLayer::doPointerMoveEvent(UnsignedInt, PointerMoveEvent& event) {
    /* In order to have Enter/Leave emitted as well */
    event.setAccepted();
}

void AbstractVisualLayer::doPointerEnterEvent(const UnsignedInt dataId, PointerMoveEvent& event) {
    const State& state = *_state;
    const Shared::State& sharedState = state.shared;
    UnsignedInt(*const transition)(UnsignedInt) = event.isCaptured() ?
        sharedState.styleTransitionToPressedHover :
        sharedState.styleTransitionToInactiveHover;

    CORRADE_INTERNAL_DEBUG_ASSERT(state.styles.size() == capacity());
    UnsignedInt& style = state.styles[dataId];
    const UnsignedInt nextStyle = transition(style);
    CORRADE_ASSERT(nextStyle < sharedState.styleCount,
        "Whee::AbstractVisualLayer::pointerEnterEvent(): style transition from" << style << "to" << nextStyle << "out of range for" << sharedState.styleCount << "styles", );
    if(nextStyle != style) {
        style = nextStyle;
        setNeedsUpdate();
    }
}

void AbstractVisualLayer::doPointerLeaveEvent(const UnsignedInt dataId, PointerMoveEvent& event) {
    const State& state = *_state;
    const Shared::State& sharedState = state.shared;
    UnsignedInt(*const transition)(UnsignedInt) = event.isCaptured() ?
        sharedState.styleTransitionToPressedBlur :
        sharedState.styleTransitionToInactiveBlur;

    CORRADE_INTERNAL_DEBUG_ASSERT(state.styles.size() == capacity());
    UnsignedInt& style = state.styles[dataId];
    const UnsignedInt nextStyle = transition(style);
    CORRADE_ASSERT(nextStyle < sharedState.styleCount,
        "Whee::AbstractVisualLayer::pointerLeaveEvent(): style transition from" << style << "to" << nextStyle << "out of range for" << sharedState.styleCount << "styles", );
    if(nextStyle != style) {
        style = nextStyle;
        setNeedsUpdate();
    }
}

}}
