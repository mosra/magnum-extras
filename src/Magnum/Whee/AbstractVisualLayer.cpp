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

#include <Corrade/Containers/BitArrayView.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Utility/Algorithms.h>

#include "Magnum/Whee/AbstractUserInterface.h"
#include "Magnum/Whee/Event.h"
#include "Magnum/Whee/Handle.h"
#include "Magnum/Whee/Implementation/abstractVisualLayerState.h"

namespace Magnum { namespace Whee {

AbstractVisualLayer::Shared::Shared(Containers::Pointer<State>&& state): _state{Utility::move(state)} {}

AbstractVisualLayer::Shared::Shared(const UnsignedInt styleCount, const UnsignedInt dynamicStyleCount): Shared{Containers::pointer<State>(*this, styleCount, dynamicStyleCount)} {}

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

UnsignedInt AbstractVisualLayer::Shared::dynamicStyleCount() const {
    return _state->dynamicStyleCount;
}

UnsignedInt AbstractVisualLayer::Shared::totalStyleCount() const {
    const State& state = *_state;
    return state.styleCount + state.dynamicStyleCount;
}

AbstractVisualLayer::Shared& AbstractVisualLayer::Shared::setStyleTransition(UnsignedInt(*const toPressedBlur)(UnsignedInt), UnsignedInt(*const toPressedHover)(UnsignedInt), UnsignedInt(*const toInactiveBlur)(UnsignedInt), UnsignedInt(*const toInactiveHover)(UnsignedInt), UnsignedInt(*const toDisabled)(UnsignedInt)) {
    _state->styleTransitionToPressedBlur = toPressedBlur ? toPressedBlur :
        Implementation::styleTransitionPassthrough;
    _state->styleTransitionToPressedHover = toPressedHover ? toPressedHover :
        Implementation::styleTransitionPassthrough;
    _state->styleTransitionToInactiveBlur = toInactiveBlur ? toInactiveBlur :
        Implementation::styleTransitionPassthrough;
    _state->styleTransitionToInactiveHover = toInactiveHover ? toInactiveHover :
        Implementation::styleTransitionPassthrough;
    /* Unlike the others, this one can be nullptr, in which case the whole
       transitioning logic in doUpdate() gets replaced with a simple copy */
    _state->styleTransitionToDisabled = toDisabled;
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
    CORRADE_ASSERT(style < _state->shared.styleCount + _state->shared.dynamicStyleCount,
        "Whee::AbstractVisualLayer::setStyle(): style" << style << "out of range for" << _state->shared.styleCount + _state->shared.dynamicStyleCount << "styles", );
    setStyleInternal(dataHandleId(handle), style);
}

void AbstractVisualLayer::setStyle(const LayerDataHandle handle, const UnsignedInt style) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::AbstractVisualLayer::setStyle(): invalid handle" << handle, );
    CORRADE_ASSERT(style < _state->shared.styleCount + _state->shared.dynamicStyleCount,
        "Whee::AbstractVisualLayer::setStyle(): style" << style << "out of range for" << _state->shared.styleCount + _state->shared.dynamicStyleCount << "styles", );
    setStyleInternal(layerDataHandleId(handle), style);
}

void AbstractVisualLayer::setStyleInternal(const UnsignedInt id, const UnsignedInt style) {
    CORRADE_INTERNAL_DEBUG_ASSERT(_state->styles.size() == capacity());
    _state->styles[id] = style;
    /* _state->calculatedStyles is filled by AbstractVisualLayer::doUpdate() */
    setNeedsUpdate();
}

void AbstractVisualLayer::setTransitionedStyle(const AbstractUserInterface& ui, const DataHandle handle, const UnsignedInt style) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::AbstractVisualLayer::setTransitionedStyle(): invalid handle" << handle, );
    CORRADE_ASSERT(style < _state->shared.styleCount,
        "Whee::AbstractVisualLayer::setTransitionedStyle(): style" << style << "out of range for" << _state->shared.styleCount << "styles", );
    setTransitionedStyleInternal(ui, dataHandleData(handle), style);
}

void AbstractVisualLayer::setTransitionedStyle(const AbstractUserInterface& ui, const LayerDataHandle handle, const UnsignedInt style) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::AbstractVisualLayer::setTransitionedStyle(): invalid handle" << handle, );
    CORRADE_ASSERT(style < _state->shared.styleCount,
        "Whee::AbstractVisualLayer::setTransitionedStyle(): style" << style << "out of range for" << _state->shared.styleCount << "styles", );
    setTransitionedStyleInternal(ui, handle, style);
}

void AbstractVisualLayer::setTransitionedStyleInternal(const AbstractUserInterface& ui, const LayerDataHandle handle, const UnsignedInt style) {
    State& state = *_state;
    CORRADE_INTERNAL_DEBUG_ASSERT(state.styles.size() == capacity());

    const Shared::State& sharedState = state.shared;
    const NodeHandle node = this->node(handle);
    const bool hovered = ui.pointerEventHoveredNode() == node;
    UnsignedInt(*transition)(UnsignedInt);
    if(ui.pointerEventPressedNode() == node) transition = hovered ?
        sharedState.styleTransitionToPressedHover :
        sharedState.styleTransitionToPressedBlur;
    else transition = hovered ?
        sharedState.styleTransitionToInactiveHover :
        sharedState.styleTransitionToInactiveBlur;
    state.styles[layerDataHandleId(handle)] = transition(style);
    setNeedsUpdate();
}

UnsignedInt AbstractVisualLayer::dynamicStyleUsedCount() const {
    return _state->dynamicStylesUsed.count();
}

Containers::Optional<UnsignedInt> AbstractVisualLayer::allocateDynamicStyle() {
    State& state = *_state;
    /** @todo some builtin "find first unset" API, tzcnt etc */
    for(std::size_t i = 0; i != state.dynamicStylesUsed.size(); ++i) {
        if(state.dynamicStylesUsed[i])
            continue;
        state.dynamicStylesUsed.set(i);
        return i;
    }

    return {};
}

void AbstractVisualLayer::recycleDynamicStyle(const UnsignedInt id) {
    State& state = *_state;
    CORRADE_ASSERT(id < state.dynamicStylesUsed.size(),
        "Whee::AbstractVisualLayer::recycleDynamicStyle(): index" << id << "out of range for" << state.dynamicStylesUsed.size() << "dynamic styles", );
    CORRADE_ASSERT(state.dynamicStylesUsed[id],
        "Whee::AbstractVisualLayer::recycleDynamicStyle(): style" << id << "not allocated", );
    state.dynamicStylesUsed.reset(id);
}

LayerFeatures AbstractVisualLayer::doFeatures() const {
    return LayerFeature::Event;
}

void AbstractVisualLayer::doUpdate(const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, Containers::BitArrayView nodesEnabled, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&) {
    State& state = *_state;
    CORRADE_INTERNAL_ASSERT(
        state.styles.size() == capacity() &&
        state.calculatedStyles.size() == capacity());

    /* Transition to disabled styles for all data that are attached to disabled
       nodes, copy the original style index otherwise. It's a copy to avoid a
       complicated logic with transitioning back from the disabled state, which
       may not always be possible. */
    const Shared::State& sharedState = state.shared;
    if(UnsignedInt(*const toDisabled)(UnsignedInt) = sharedState.styleTransitionToDisabled) {
        const Containers::StridedArrayView1D<const NodeHandle> nodes = this->nodes();
        const UnsignedInt styleCount = sharedState.styleCount;
        for(const UnsignedInt id: dataIds) {
            const UnsignedInt style = state.styles[id];
            /** @todo Doing a function call for all data may be a bit horrible,
                also especially if the code inside is a giant switch that the
                compiler failed to turn into a LUT. Thus, ideally, the
                transition should be done only for nodes that actually changed
                their disabled status, which means recording the previous
                nodesEnabled state, XORing the current with it, and then
                performing transition only when the XOR is 1. Furthermore, that
                may often be just a very tiny portion of nodes, so ideally
                there would be a way to quickly get just the subset of *data*
                IDs that actually changed (and not node IDs), to iterate over
                them directly. */
            /* Skipping data that have dynamic styles, those are passthrough */
            if(style < styleCount && !nodesEnabled[nodeHandleId(nodes[id])]) {
                const UnsignedInt nextStyle = toDisabled(style);
                /** @todo a debug assert? or is it negligible compared to the
                    function call? */
                CORRADE_ASSERT(nextStyle < styleCount,
                    "Whee::AbstractVisualLayer::update(): style transition from" << style << "to" << nextStyle << "out of range for" << styleCount << "styles", );
                state.calculatedStyles[id] = nextStyle;
            } else {
                CORRADE_INTERNAL_DEBUG_ASSERT(style < sharedState.styleCount + sharedState.dynamicStyleCount);
                state.calculatedStyles[id] = style;
            }
        }

    /* If the transition function isn't set -- i.e., the transition is an
       identity --, just copy them over. The subclass doUpdate() / doDraw() is
       then assumed to handle that on its own, for example by applying
       desaturation and fade out globally to all data. */
    } else Utility::copy(state.styles, state.calculatedStyles);
}

void AbstractVisualLayer::doPointerPressEvent(const UnsignedInt dataId, PointerEvent& event) {
    /* Only reacting to pointer types typically used to click/tap on things */
    if(event.type() != Pointer::MouseLeft &&
       event.type() != Pointer::Finger &&
       event.type() != Pointer::Pen)
        return;

    const State& state = *_state;
    const Shared::State& sharedState = state.shared;
    CORRADE_INTERNAL_DEBUG_ASSERT(state.styles.size() == capacity());
    UnsignedInt& style = state.styles[dataId];

    /* Transition the style to pressed if it's not dynamic */
    if(style >= sharedState.styleCount) {
        CORRADE_INTERNAL_DEBUG_ASSERT(style < sharedState.styleCount + sharedState.dynamicStyleCount);
    } else {
        /* A press can be not hovering if it happened without a preceding move
           event (such as for pointer types that don't support hover like
           touches, or if move events aren't propagated from the
           application) */
        UnsignedInt(*const transition)(UnsignedInt) = event.isHovering() ?
            sharedState.styleTransitionToPressedHover :
            sharedState.styleTransitionToPressedBlur;
        const UnsignedInt nextStyle = transition(style);
        CORRADE_ASSERT(nextStyle < sharedState.styleCount,
            "Whee::AbstractVisualLayer::pointerPressEvent(): style transition from" << style << "to" << nextStyle << "out of range for" << sharedState.styleCount << "styles", );
        if(nextStyle != style) {
            style = nextStyle;
            setNeedsUpdate();
        }
    }

    event.setAccepted();
}

void AbstractVisualLayer::doPointerReleaseEvent(const UnsignedInt dataId, PointerEvent& event) {
    /* Only reacting to pointer types typically used to click/tap on things */
    if(event.type() != Pointer::MouseLeft &&
       event.type() != Pointer::Finger &&
       event.type() != Pointer::Pen)
        return;

    const State& state = *_state;
    const Shared::State& sharedState = state.shared;
    CORRADE_INTERNAL_DEBUG_ASSERT(state.styles.size() == capacity());
    UnsignedInt& style = state.styles[dataId];

    /* Transition the style to released if it's not dynamic */
    if(style >= sharedState.styleCount) {
        CORRADE_INTERNAL_DEBUG_ASSERT(style < sharedState.styleCount + sharedState.dynamicStyleCount);
    } else {
        /* A release can be not hovering if it happened without a preceding
           move event (such as for pointer types that don't support hover like
           touches, or if move events aren't propagated from the
           application) */
        UnsignedInt(*const transition)(UnsignedInt) = event.isHovering() ?
            sharedState.styleTransitionToInactiveHover :
            sharedState.styleTransitionToInactiveBlur;
        const UnsignedInt nextStyle = transition(style);
        CORRADE_ASSERT(nextStyle < sharedState.styleCount,
            "Whee::AbstractVisualLayer::pointerReleaseEvent(): style transition from" << style << "to" << nextStyle << "out of range for" << sharedState.styleCount << "styles", );
        if(nextStyle != style) {
            style = nextStyle;
            setNeedsUpdate();
        }
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
    CORRADE_INTERNAL_DEBUG_ASSERT(state.styles.size() == capacity());
    UnsignedInt& style = state.styles[dataId];

    /* Transition the style to hover if it's not dynamic */
    if(style >= sharedState.styleCount) {
        CORRADE_INTERNAL_DEBUG_ASSERT(style < sharedState.styleCount + sharedState.dynamicStyleCount);
    } else {
        UnsignedInt(*const transition)(UnsignedInt) = event.isCaptured() ?
            sharedState.styleTransitionToPressedHover :
            sharedState.styleTransitionToInactiveHover;
        const UnsignedInt nextStyle = transition(style);
        CORRADE_ASSERT(nextStyle < sharedState.styleCount,
            "Whee::AbstractVisualLayer::pointerEnterEvent(): style transition from" << style << "to" << nextStyle << "out of range for" << sharedState.styleCount << "styles", );
        if(nextStyle != style) {
            style = nextStyle;
            setNeedsUpdate();
        }
    }
}

void AbstractVisualLayer::doPointerLeaveEvent(const UnsignedInt dataId, PointerMoveEvent& event) {
    const State& state = *_state;
    const Shared::State& sharedState = state.shared;
    CORRADE_INTERNAL_DEBUG_ASSERT(state.styles.size() == capacity());
    UnsignedInt& style = state.styles[dataId];

    /* Transition the style from hover if it's not dynamic */
    if(style >= sharedState.styleCount) {
        CORRADE_INTERNAL_DEBUG_ASSERT(style < sharedState.styleCount + sharedState.dynamicStyleCount);
    } else {
        UnsignedInt(*const transition)(UnsignedInt) = event.isCaptured() ?
            sharedState.styleTransitionToPressedBlur :
            sharedState.styleTransitionToInactiveBlur;
        const UnsignedInt nextStyle = transition(style);
        CORRADE_ASSERT(nextStyle < sharedState.styleCount,
            "Whee::AbstractVisualLayer::pointerLeaveEvent(): style transition from" << style << "to" << nextStyle << "out of range for" << sharedState.styleCount << "styles", );
        if(nextStyle != style) {
            style = nextStyle;
            setNeedsUpdate();
        }
    }
}

}}
