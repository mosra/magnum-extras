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

#include "AbstractVisualLayer.h"

#include <Corrade/Containers/BitArrayView.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Utility/Algorithms.h>

#include "Magnum/Ui/AbstractUserInterface.h"
#include "Magnum/Ui/AbstractVisualLayerAnimator.h"
#include "Magnum/Ui/Event.h"
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/Implementation/abstractVisualLayerState.h"

namespace Magnum { namespace Ui {

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

AbstractVisualLayer::Shared& AbstractVisualLayer::Shared::setStyleTransition(UnsignedInt(*const toInactiveOut)(UnsignedInt), UnsignedInt(*const toInactiveOver)(UnsignedInt), UnsignedInt(*const toFocusedOut)(UnsignedInt), UnsignedInt(*const toFocusedOver)(UnsignedInt), UnsignedInt(*const toPressedOut)(UnsignedInt), UnsignedInt(*const toPressedOver)(UnsignedInt), UnsignedInt(*const toDisabled)(UnsignedInt)) {
    _state->styleTransitionToInactiveOut = toInactiveOut ? toInactiveOut :
        Implementation::styleTransitionPassthrough;
    _state->styleTransitionToInactiveOver = toInactiveOver ? toInactiveOver :
        Implementation::styleTransitionPassthrough;
    _state->styleTransitionToFocusedOut = toFocusedOut ? toFocusedOut :
        Implementation::styleTransitionPassthrough;
    _state->styleTransitionToFocusedOver = toFocusedOver ? toFocusedOver :
        Implementation::styleTransitionPassthrough;
    _state->styleTransitionToPressedOut = toPressedOut ? toPressedOut :
        Implementation::styleTransitionPassthrough;
    _state->styleTransitionToPressedOver = toPressedOver ? toPressedOver :
        Implementation::styleTransitionPassthrough;
    /* Unlike the others, this one can be nullptr, in which case the whole
       transitioning logic in doUpdate() gets replaced with a simple copy.
       Setting it to a different function then causes doState() in all layers
       sharing this state return NeedsDataUpdate. */
    if(_state->styleTransitionToDisabled != toDisabled) {
        _state->styleTransitionToDisabled = toDisabled;
        ++_state->styleTransitionToDisabledUpdateStamp;
    }
    return *this;
}

auto AbstractVisualLayer::Shared::styleTransitionToInactiveOut() const -> UnsignedInt(*)(UnsignedInt) {
    return _state->styleTransitionToInactiveOut;
}

auto AbstractVisualLayer::Shared::styleTransitionToInactiveOver() const -> UnsignedInt(*)(UnsignedInt) {
    return _state->styleTransitionToInactiveOver;
}

auto AbstractVisualLayer::Shared::styleTransitionToFocusedOut() const -> UnsignedInt(*)(UnsignedInt) {
    return _state->styleTransitionToFocusedOut;
}

auto AbstractVisualLayer::Shared::styleTransitionToFocusedOver() const -> UnsignedInt(*)(UnsignedInt) {
    return _state->styleTransitionToFocusedOver;
}

auto AbstractVisualLayer::Shared::styleTransitionToPressedOut() const -> UnsignedInt(*)(UnsignedInt) {
    return _state->styleTransitionToPressedOut;
}

auto AbstractVisualLayer::Shared::styleTransitionToPressedOver() const -> UnsignedInt(*)(UnsignedInt) {
    return _state->styleTransitionToPressedOver;
}

auto AbstractVisualLayer::Shared::styleTransitionToDisabled() const -> UnsignedInt(*)(UnsignedInt) {
    return _state->styleTransitionToDisabled;
}

AbstractVisualLayer::Shared& AbstractVisualLayer::Shared::setStyleAnimation(AnimationHandle(*onEnter)(AbstractVisualLayerStyleAnimator&, UnsignedInt, UnsignedInt, Nanoseconds, LayerDataHandle, AnimatorDataHandle), AnimationHandle(*onLeave)(AbstractVisualLayerStyleAnimator&, UnsignedInt, UnsignedInt, Nanoseconds, LayerDataHandle, AnimatorDataHandle), AnimationHandle(*onFocus)(AbstractVisualLayerStyleAnimator&, UnsignedInt, UnsignedInt, Nanoseconds, LayerDataHandle, AnimatorDataHandle), AnimationHandle(*onBlur)(AbstractVisualLayerStyleAnimator&, UnsignedInt, UnsignedInt, Nanoseconds, LayerDataHandle, AnimatorDataHandle), AnimationHandle(*onPress)(AbstractVisualLayerStyleAnimator&, UnsignedInt, UnsignedInt, Nanoseconds, LayerDataHandle, AnimatorDataHandle), AnimationHandle(*onRelease)(AbstractVisualLayerStyleAnimator&, UnsignedInt, UnsignedInt, Nanoseconds, LayerDataHandle, AnimatorDataHandle), AnimationHandle(*persistent)(AbstractVisualLayerStyleAnimator&, UnsignedInt, Nanoseconds, LayerDataHandle, AnimatorDataHandle)) {
    _state->styleAnimationOnEnter = onEnter;
    _state->styleAnimationOnLeave = onLeave;
    _state->styleAnimationOnFocus = onFocus;
    _state->styleAnimationOnBlur = onBlur;
    _state->styleAnimationOnPress = onPress;
    _state->styleAnimationOnRelease = onRelease;
    _state->styleAnimationPersistent = persistent;
    return *this;
}

AbstractVisualLayer::State::State(Shared::State& shared): shared(shared), styleTransitionToDisabledUpdateStamp{shared.styleTransitionToDisabledUpdateStamp} {
    dynamicStyleStorage = Containers::ArrayTuple{
        {ValueInit, shared.dynamicStyleCount, dynamicStylesUsed},
        {ValueInit, shared.dynamicStyleCount, dynamicStyleAnimations}
    };
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
        "Ui::AbstractVisualLayer::style(): invalid handle" << handle, {});
    CORRADE_INTERNAL_DEBUG_ASSERT(_state->styles.size() == capacity());
    return _state->styles[dataHandleId(handle)];
}

UnsignedInt AbstractVisualLayer::style(const LayerDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractVisualLayer::style(): invalid handle" << handle, {});
    CORRADE_INTERNAL_DEBUG_ASSERT(_state->styles.size() == capacity());
    return _state->styles[layerDataHandleId(handle)];
}

void AbstractVisualLayer::setStyle(const DataHandle handle, const UnsignedInt style) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractVisualLayer::setStyle(): invalid handle" << handle, );
    CORRADE_ASSERT(style < _state->shared.styleCount + _state->shared.dynamicStyleCount,
        "Ui::AbstractVisualLayer::setStyle(): style" << style << "out of range for" << _state->shared.styleCount + _state->shared.dynamicStyleCount << "styles", );
    setStyleInternal(dataHandleId(handle), style);
}

void AbstractVisualLayer::setStyle(const LayerDataHandle handle, const UnsignedInt style) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractVisualLayer::setStyle(): invalid handle" << handle, );
    CORRADE_ASSERT(style < _state->shared.styleCount + _state->shared.dynamicStyleCount,
        "Ui::AbstractVisualLayer::setStyle(): style" << style << "out of range for" << _state->shared.styleCount + _state->shared.dynamicStyleCount << "styles", );
    setStyleInternal(layerDataHandleId(handle), style);
}

void AbstractVisualLayer::setStyleInternal(const UnsignedInt id, const UnsignedInt style) {
    CORRADE_INTERNAL_DEBUG_ASSERT(_state->styles.size() == capacity());
    _state->styles[id] = style;
    /* _state->calculatedStyles is filled by AbstractVisualLayer::doUpdate() */
    setNeedsUpdate(LayerState::NeedsDataUpdate);
}

void AbstractVisualLayer::setTransitionedStyle(const AbstractUserInterface& ui, const DataHandle handle, const UnsignedInt style) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractVisualLayer::setTransitionedStyle(): invalid handle" << handle, );
    CORRADE_ASSERT(style < _state->shared.styleCount,
        "Ui::AbstractVisualLayer::setTransitionedStyle(): style" << style << "out of range for" << _state->shared.styleCount << "styles", );
    setTransitionedStyleInternal(ui, dataHandleData(handle), style);
}

void AbstractVisualLayer::setTransitionedStyle(const AbstractUserInterface& ui, const LayerDataHandle handle, const UnsignedInt style) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractVisualLayer::setTransitionedStyle(): invalid handle" << handle, );
    CORRADE_ASSERT(style < _state->shared.styleCount,
        "Ui::AbstractVisualLayer::setTransitionedStyle(): style" << style << "out of range for" << _state->shared.styleCount << "styles", );
    setTransitionedStyleInternal(ui, handle, style);
}

void AbstractVisualLayer::setTransitionedStyleInternal(const AbstractUserInterface& ui, const LayerDataHandle handle, const UnsignedInt style) {
    State& state = *_state;
    CORRADE_INTERNAL_DEBUG_ASSERT(state.styles.size() == capacity());

    const Shared::State& sharedState = state.shared;
    const NodeHandle node = this->node(handle);
    const bool hovered = ui.currentHoveredNode() == node;
    UnsignedInt(*transition)(UnsignedInt);
    if(ui.currentPressedNode() == node) transition = hovered ?
        sharedState.styleTransitionToPressedOver :
        sharedState.styleTransitionToPressedOut;
    else if(ui.currentFocusedNode() == node) transition = hovered ?
        sharedState.styleTransitionToFocusedOver :
        sharedState.styleTransitionToFocusedOut;
    else transition = hovered ?
        sharedState.styleTransitionToInactiveOver :
        sharedState.styleTransitionToInactiveOut;
    state.styles[layerDataHandleId(handle)] = transition(style);
    setNeedsUpdate(LayerState::NeedsDataUpdate);
}

UnsignedInt AbstractVisualLayer::dynamicStyleUsedCount() const {
    return _state->dynamicStylesUsed.count();
}

Containers::Optional<UnsignedInt> AbstractVisualLayer::allocateDynamicStyle(const AnimationHandle animation) {
    State& state = *_state;
    /** @todo some builtin "find first unset" API, tzcnt etc */
    for(std::size_t i = 0; i != state.dynamicStylesUsed.size(); ++i) {
        if(state.dynamicStylesUsed[i])
            continue;
        state.dynamicStylesUsed.set(i);
        state.dynamicStyleAnimations[i] = animation;
        return i;
    }

    return {};
}

AnimationHandle AbstractVisualLayer::dynamicStyleAnimation(const UnsignedInt id) const {
    const State& state = *_state;
    CORRADE_ASSERT(id < state.dynamicStylesUsed.size(),
        "Ui::AbstractVisualLayer::dynamicStyleAnimation(): index" << id << "out of range for" << state.dynamicStylesUsed.size() << "dynamic styles", {});
    return state.dynamicStyleAnimations[id];
}

void AbstractVisualLayer::recycleDynamicStyle(const UnsignedInt id) {
    State& state = *_state;
    CORRADE_ASSERT(id < state.dynamicStylesUsed.size(),
        "Ui::AbstractVisualLayer::recycleDynamicStyle(): index" << id << "out of range for" << state.dynamicStylesUsed.size() << "dynamic styles", );
    CORRADE_ASSERT(state.dynamicStylesUsed[id],
        "Ui::AbstractVisualLayer::recycleDynamicStyle(): style" << id << "not allocated", );
    state.dynamicStylesUsed.reset(id);
    state.dynamicStyleAnimations[id] = AnimationHandle::Null;
}

AbstractVisualLayer& AbstractVisualLayer::assignAnimator(AbstractVisualLayerStyleAnimator& animator) {
    CORRADE_ASSERT(_state->shared.dynamicStyleCount,
        "Ui::AbstractVisualLayer::assignAnimator(): can't animate a layer with zero dynamic styles", *this);

    AbstractLayer::assignAnimator(animator);
    animator.setLayerInstance(*this, &_state->shared);
    return *this;
}

AbstractVisualLayerStyleAnimator* AbstractVisualLayer::defaultStyleAnimator() const {
    return _state->styleAnimator;
}

AbstractVisualLayer& AbstractVisualLayer::setDefaultStyleAnimator(AbstractVisualLayerStyleAnimator* animator) {
    CORRADE_ASSERT(!animator || animator->layer() != LayerHandle::Null,
        "Ui::AbstractVisualLayer::setDefaultStyleAnimator(): animator isn't assigned to any layer", *this);
    CORRADE_ASSERT(!animator || animator->layer() == handle(),
        "Ui::AbstractVisualLayer::setDefaultStyleAnimator(): expected an animator assigned to" << handle() << "but got" << animator->layer(), *this);
    _state->styleAnimator = animator;
    return *this;
}

LayerFeatures AbstractVisualLayer::doFeatures() const {
    return LayerFeature::Event;
}

LayerStates AbstractVisualLayer::doState() const {
    const State& state = *_state;
    const Shared::State& sharedState = state.shared;
    if(state.styleTransitionToDisabledUpdateStamp != sharedState.styleTransitionToDisabledUpdateStamp)
        return LayerState::NeedsDataUpdate;
    return {};
}

void AbstractVisualLayer::doUpdate(const LayerStates states, const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Float>&, Containers::BitArrayView nodesEnabled, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&) {
    State& state = *_state;
    CORRADE_INTERNAL_ASSERT(
        state.styles.size() == capacity() &&
        state.calculatedStyles.size() == capacity());

    /* Transition to disabled styles for all data that are attached to disabled
       nodes, copy the original style index otherwise. It's a copy to avoid a
       complicated logic with transitioning back from the disabled state, which
       may not always be possible.

       Do this only if the data changed (i.e., possibly including style
       assignment) or if the node enablement changed. */
    if(states & (LayerState::NeedsNodeEnabledUpdate|LayerState::NeedsDataUpdate)) {
        const Shared::State& sharedState = state.shared;
        if(UnsignedInt(*const toDisabled)(UnsignedInt) = sharedState.styleTransitionToDisabled) {
            const Containers::StridedArrayView1D<const NodeHandle> nodes = this->nodes();
            const UnsignedInt styleCount = sharedState.styleCount;
            for(const UnsignedInt id: dataIds) {
                /* Can't use the transitionStyle() helper here as it updates
                   state.styles and not state.calculatedStyles */
                const UnsignedInt style = state.styles[id];

                /* If the style is dynamic, maybe it has an animation with a
                   target style index assigned, which we can use as the
                   (soon-to-be-)current style index to transition from. We're
                   not animating here, so the second return value is
                   ignored. */
                const UnsignedInt currentStyle = styleOrAnimationTargetStyle(style).first();

                /** @todo Doing a function call for all data may be a bit
                    horrible, also especially if the code inside is a giant
                    switch that the compiler failed to turn into a LUT. Thus,
                    ideally, the transition should be done only for nodes that
                    actually changed their disabled status, which means
                    recording the previous nodesEnabled state, XORing the
                    current with it, and then performing transition only when
                    the XOR is 1. Furthermore, that may often be just a very
                    tiny portion of nodes, so ideally there would be a way to
                    quickly get just the subset of *data* IDs that actually
                    changed (and not node IDs), to iterate over them
                    directly. */
                /* Skipping data that have dynamic styles, those are
                   passthrough */
                if(currentStyle < styleCount && !nodesEnabled[nodeHandleId(nodes[id])]) {
                    const UnsignedInt nextStyle = toDisabled(currentStyle);
                    /** @todo a debug assert? or is it negligible compared to
                        the function call? */
                    CORRADE_ASSERT(nextStyle < styleCount,
                        "Ui::AbstractVisualLayer::update(): style transition from" << currentStyle << "to" << nextStyle << "out of range for" << styleCount << "styles", );
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

        /* Sync the style transition update stamp to not have doState() return
           NeedsDataUpdate again next time it's asked */
        state.styleTransitionToDisabledUpdateStamp = sharedState.styleTransitionToDisabledUpdateStamp;
    }
}

Containers::Pair<UnsignedInt, AnimatorDataHandle> AbstractVisualLayer::styleOrAnimationTargetStyle(const UnsignedInt style) const {
    const State& state = *_state;
    const Shared::State& sharedState = state.shared;

    /* If the style is dynamic, maybe it has an animation with a target style
       index assigned */
    if(style >= sharedState.styleCount) {
        CORRADE_INTERNAL_DEBUG_ASSERT(style < sharedState.styleCount + sharedState.dynamicStyleCount);
        const AnimationHandle animation = state.dynamicStyleAnimations[style - sharedState.styleCount];
        /* The target style is useful only if the animation is from our default
           style animator. If it's some other animator, better not touch it at
           all. If the animation is Reverse, then it's being switched to the
           source style instead. */
        if(animation != AnimationHandle::Null && state.styleAnimator && animationHandleAnimator(animation) == state.styleAnimator->handle()) {
            const Containers::Pair<UnsignedInt, UnsignedInt> styles = state.styleAnimator->styles(animation);
            return {
                state.styleAnimator->flags(animation) >= AnimationFlag::Reverse ?
                    styles.first() : styles.second(),
                animationHandleData(animation)
            };
        }
    }

    /* Otherwise return the original style verbatim, and no animation */
    return {style, AnimatorDataHandle::Null};
}

void AbstractVisualLayer::transitionStyle(
    #ifndef CORRADE_NO_ASSERT
    const char* messagePrefix,
    #endif
    const UnsignedInt dataId, UnsignedInt(*const transition)(UnsignedInt), const Nanoseconds time, AnimationHandle(*transitionAnimation)(AbstractVisualLayerStyleAnimator&, UnsignedInt, UnsignedInt, Nanoseconds, LayerDataHandle, AnimatorDataHandle)
) {
    const State& state = *_state;
    const Shared::State& sharedState = state.shared;
    CORRADE_INTERNAL_DEBUG_ASSERT(state.styles.size() == capacity());
    UnsignedInt& style = state.styles[dataId];

    /* If the style is dynamic, maybe it has an animation with a target style
       index assigned, which we can use as the (soon-to-be-)current style index
       to transition from. If not, there's nothing to transition. */
    const Containers::Pair<UnsignedInt, AnimatorDataHandle> currentStyleAnimation = styleOrAnimationTargetStyle(style);
    if(currentStyleAnimation.first() >= sharedState.styleCount)
        return;

    const UnsignedInt nextStyle = transition(currentStyleAnimation.first());
    CORRADE_ASSERT(nextStyle < sharedState.styleCount,
        messagePrefix << "style transition from" << currentStyleAnimation.first() << "to" << nextStyle << "out of range for" << sharedState.styleCount << "styles", );

    /* If the next style is the same as the current, nothing left to do */
    if(nextStyle == currentStyleAnimation.first())
        return;

    /* If we have a default style animator, we can animate the style */
    AnimationHandle animation = AnimationHandle::Null;
    AnimationHandle persistentAnimation = AnimationHandle::Null;
    if(state.styleAnimator) {
        /* Try animating the style transition first */
        if(transitionAnimation)
            animation = transitionAnimation(*state.styleAnimator, currentStyleAnimation.first(), nextStyle, time, layerDataHandle(dataId, generations()[dataId]), currentStyleAnimation.second());

        /* All of those are debug-only assertions because it's quite a lot
            of checking */
        #ifndef CORRADE_NO_DEBUG_ASSERT
        if(animation != AnimationHandle::Null) {
            CORRADE_DEBUG_ASSERT(state.styleAnimator->isHandleValid(animation),
                messagePrefix << "expected style transition animation to be either null or valid and coming from" << state.styleAnimator->handle() << "but got" << animation, );
            CORRADE_DEBUG_ASSERT(state.styleAnimator->styles(animation).second() == nextStyle,
                messagePrefix << "expected style transition animation to have" << nextStyle << "as target style but got" << state.styleAnimator->styles(animation).second(), );
            CORRADE_DEBUG_ASSERT(state.styleAnimator->started(animation) == time,
                messagePrefix << "expected style transition animation to start at" << time << "but got" << state.styleAnimator->started(animation), );
            CORRADE_DEBUG_ASSERT(dataHandleId(state.styleAnimator->data(animation)) == dataId,
                messagePrefix << "expected style transition animation to be attached to" << layerDataHandle(dataId, generations()[dataId]) << "but got" << dataHandleData(state.styleAnimator->data(animation)), );
            CORRADE_DEBUG_ASSERT(!(state.styleAnimator->flags(animation) & (AnimationFlag::KeepOncePlayed|AnimationFlag::Reverse)),
                messagePrefix << "style transition animation cannot have" << (state.styleAnimator->flags(animation) & (AnimationFlag::KeepOncePlayed|AnimationFlag::Reverse)) << "set", );
        }
        #endif

        /* Then try a persistent animation for given style */
        if(sharedState.styleAnimationPersistent)
            persistentAnimation = sharedState.styleAnimationPersistent(*state.styleAnimator, nextStyle, time, layerDataHandle(dataId, generations()[dataId]), animationHandleData(animation));

        /* Again all of those are debug-only assertions because it's quite a
           lot of checking */
        #ifndef CORRADE_NO_DEBUG_ASSERT
        if(persistentAnimation != AnimationHandle::Null) {
            CORRADE_DEBUG_ASSERT(state.styleAnimator->isHandleValid(persistentAnimation),
                messagePrefix << "expected persistent style animation to be either null or valid and coming from" << state.styleAnimator->handle() << "but got" << persistentAnimation, );
            CORRADE_DEBUG_ASSERT(state.styleAnimator->styles(persistentAnimation).second() == nextStyle,
                messagePrefix << "expected persistent style animation to have" << nextStyle << "as target style but got" << state.styleAnimator->styles(persistentAnimation).second(), );
            CORRADE_DEBUG_ASSERT(state.styleAnimator->started(persistentAnimation) == time,
                messagePrefix << "expected persistent style animation to start at" << time << "but got" << state.styleAnimator->started(persistentAnimation), );
            CORRADE_DEBUG_ASSERT(dataHandleId(state.styleAnimator->data(persistentAnimation)) == dataId,
                messagePrefix << "expected persistent style animation to be attached to" << layerDataHandle(dataId, generations()[dataId]) << "but got" << dataHandleData(state.styleAnimator->data(persistentAnimation)), );
            CORRADE_DEBUG_ASSERT(!(state.styleAnimator->flags(persistentAnimation) & (AnimationFlag::KeepOncePlayed|AnimationFlag::Reverse)),
                messagePrefix << "persistent style animation cannot have" << (state.styleAnimator->flags(persistentAnimation) & (AnimationFlag::KeepOncePlayed|AnimationFlag::Reverse)) << "set", );
            CORRADE_DEBUG_ASSERT(animation == AnimationHandle::Null || !state.styleAnimator->isHandleValid(animation),
                messagePrefix << "persistent style animation is expected to remove the transition animation to avoid conflicts", );
        } else CORRADE_DEBUG_ASSERT(animation == AnimationHandle::Null || state.styleAnimator->isHandleValid(animation),
            messagePrefix << "persistent style animation is only expected to remove the transition animation if replacing it", );
        #endif
    }

    /* If there's neither a transition animation nor a persistent animation,
       switch the style directly. The above asserts ensure that the transition
       animation gets removed if and only if a persistent animation is created,
       so if any of them is non-null it means it's valid. */
    if(animation == AnimationHandle::Null && persistentAnimation == AnimationHandle::Null) {
        style = nextStyle;
        setNeedsUpdate(LayerState::NeedsDataUpdate);
    }
}

void AbstractVisualLayer::doPointerPressEvent(const UnsignedInt dataId, PointerEvent& event) {
    /* Not dealing with fallthrough events; only reacting to primary pointer
       types typically used to click/tap on things */
    if(event.isFallthrough() || !event.isPrimary() ||
        (event.pointer() != Pointer::MouseLeft &&
         event.pointer() != Pointer::Finger &&
         event.pointer() != Pointer::Pen))
        return;

    /* Transition the style to pressed. A press can be not hovering if it
       happened without a preceding move event (such as for pointer types that
       don't support hover like touches, or if move events aren't propagated
       from the application). Pressed state has a priority over focused state,
       so isNodeFocused() is ignored in this case. */
    const Shared::State& sharedState = _state->shared;
    UnsignedInt(*const transition)(UnsignedInt) = event.isNodeHovered() ?
        sharedState.styleTransitionToPressedOver :
        sharedState.styleTransitionToPressedOut;
    transitionStyle(
        #ifndef CORRADE_NO_ASSERT
        "Ui::AbstractVisualLayer::pointerPressEvent():",
        #endif
        dataId, transition, event.time(), sharedState.styleAnimationOnPress);

    event.setAccepted();
}

void AbstractVisualLayer::doPointerReleaseEvent(const UnsignedInt dataId, PointerEvent& event) {
    /* Not dealing with fallthrough events; only reacting to primary pointer
       types typically used to click/tap on things */
    if(event.isFallthrough() || !event.isPrimary() ||
        (event.pointer() != Pointer::MouseLeft &&
         event.pointer() != Pointer::Finger &&
         event.pointer() != Pointer::Pen))
        return;

    /* Transition the style to released. A release can be not hovering if it
       happened without a preceding move event (such as for pointer types that
       don't support hover like touches, or if move events aren't propagated
       from the application) */
    const Shared::State& sharedState = _state->shared;
    UnsignedInt(*const transition)(UnsignedInt) = event.isNodeFocused() ?
        event.isNodeHovered() ?
            sharedState.styleTransitionToFocusedOver :
            sharedState.styleTransitionToFocusedOut :
        event.isNodeHovered() ?
            sharedState.styleTransitionToInactiveOver :
            sharedState.styleTransitionToInactiveOut;
    transitionStyle(
        #ifndef CORRADE_NO_ASSERT
        "Ui::AbstractVisualLayer::pointerReleaseEvent():",
        #endif
        dataId, transition, event.time(), sharedState.styleAnimationOnRelease);

    event.setAccepted();
}

void AbstractVisualLayer::doPointerMoveEvent(UnsignedInt, PointerMoveEvent& event) {
    /* Not dealing with fallthrough events; only reacting to primary pointer
       types */
    if(event.isFallthrough() || !event.isPrimary())
        return;

    /* In order to have Enter/Leave emitted as well */
    event.setAccepted();
}

void AbstractVisualLayer::doPointerEnterEvent(const UnsignedInt dataId, PointerMoveEvent& event) {
    /* Right now, fallthrough enter/leave events are not sent by anything, so
       just assume they never arrive */
    CORRADE_INTERNAL_DEBUG_ASSERT(!event.isFallthrough());

    /* Transition the style to over */
    const Shared::State& sharedState = _state->shared;
    UnsignedInt(*const transition)(UnsignedInt) = event.isCaptured() ?
        sharedState.styleTransitionToPressedOver : event.isNodeFocused() ?
            sharedState.styleTransitionToFocusedOver :
            sharedState.styleTransitionToInactiveOver;
    transitionStyle(
        #ifndef CORRADE_NO_ASSERT
        "Ui::AbstractVisualLayer::pointerEnterEvent():",
        #endif
        dataId, transition, event.time(), sharedState.styleAnimationOnEnter);
}

void AbstractVisualLayer::doPointerLeaveEvent(const UnsignedInt dataId, PointerMoveEvent& event) {
    /* Right now, fallthrough enter/leave events are not sent by anything, so
       just assume they never arrive */
    CORRADE_INTERNAL_DEBUG_ASSERT(!event.isFallthrough());

    /* Transition the style to out */
    const Shared::State& sharedState = _state->shared;
    UnsignedInt(*const transition)(UnsignedInt) = event.isCaptured() ?
        sharedState.styleTransitionToPressedOut : event.isNodeFocused() ?
            sharedState.styleTransitionToFocusedOut :
            sharedState.styleTransitionToInactiveOut;
    transitionStyle(
        #ifndef CORRADE_NO_ASSERT
        "Ui::AbstractVisualLayer::pointerLeaveEvent():",
        #endif
        dataId, transition, event.time(), sharedState.styleAnimationOnLeave);
}

void AbstractVisualLayer::doPointerCancelEvent(const UnsignedInt dataId, PointerCancelEvent& event) {
    /* Transition the style to inactive out. This transition has no associated
       animation but the inactive out style may still have a persistent
       animation. */
    transitionStyle(
        #ifndef CORRADE_NO_ASSERT
        "Ui::AbstractVisualLayer::pointerCancelEvent():",
        #endif
        dataId, _state->shared.styleTransitionToInactiveOut, event.time(), nullptr);
}

void AbstractVisualLayer::doFocusEvent(const UnsignedInt dataId, FocusEvent& event) {
    /* Transition the style to focused if it's not pressed as well, as pressed
       style gets a priority */
    if(!event.isNodePressed()) {
        const Shared::State& sharedState = _state->shared;
        UnsignedInt(*const transition)(UnsignedInt) = event.isNodeHovered() ?
            sharedState.styleTransitionToFocusedOver :
            sharedState.styleTransitionToFocusedOut;
        transitionStyle(
            #ifndef CORRADE_NO_ASSERT
            "Ui::AbstractVisualLayer::focusEvent():",
            #endif
            dataId, transition, event.time(), sharedState.styleAnimationOnFocus);
    }

    event.setAccepted();
}

void AbstractVisualLayer::doBlurEvent(const UnsignedInt dataId, FocusEvent& event) {
    /* Transition the style to blurred if it's not pressed as well, as pressed
       style gets a priority */
    if(!event.isNodePressed()) {
        const Shared::State& sharedState = _state->shared;
        UnsignedInt(*const transition)(UnsignedInt) = event.isNodeHovered() ?
            sharedState.styleTransitionToInactiveOver :
            sharedState.styleTransitionToInactiveOut;
        transitionStyle(
            #ifndef CORRADE_NO_ASSERT
            "Ui::AbstractVisualLayer::blurEvent():",
            #endif
            dataId, transition, event.time(), sharedState.styleAnimationOnBlur);
    }

    event.setAccepted();
}

void AbstractVisualLayer::doVisibilityLostEvent(const UnsignedInt dataId, VisibilityLostEvent& event) {
    const State& state = *_state;
    const Shared::State& sharedState = state.shared;
    CORRADE_INTERNAL_DEBUG_ASSERT(state.styles.size() == capacity());
    UnsignedInt& style = state.styles[dataId];

    /* If the style is dynamic, maybe it has an animation with a target style
       index assigned, which we can use as the (soon-to-be-)current style index
       to transition from. We're not animating here, so the second return value
       is ignored. */
    const UnsignedInt currentStyle = styleOrAnimationTargetStyle(style).first();

    /* Transition the style to inactive if it's not dynamic and only if it's
       not a formerly focused node that's now pressed, in which case it stays
       pressed. */
    if(currentStyle < sharedState.styleCount && !event.isNodePressed()) {
        UnsignedInt(*const transition)(UnsignedInt) = event.isNodeHovered() ?
            sharedState.styleTransitionToInactiveOver :
            sharedState.styleTransitionToInactiveOut;
        /* Not using transitionStyle() in this case because this function is
           called from within update(), meaning one can't just fire animations
           like a madman in the middle of _that_ */
        const UnsignedInt nextStyle = transition(currentStyle);
        CORRADE_ASSERT(nextStyle < sharedState.styleCount,
            "Ui::AbstractVisualLayer::visibilityLostEvent(): style transition from" << currentStyle << "to" << nextStyle << "out of range for" << sharedState.styleCount << "styles", );
        /* If the transitioned style is different from the current one (or the
           one that's the animation target), update it */
        if(nextStyle != currentStyle) {
            style = nextStyle;
            setNeedsUpdate(LayerState::NeedsDataUpdate);
        }
    }
}

void AbstractVisualLayer::DebugIntegration::print(Debug& debug, const AbstractVisualLayer& layer, const Containers::StringView& layerName, LayerDataHandle data) {
    debug << "  Data" << Debug::packed << data << "from layer" << Debug::packed << layer.handle();
    if(layerName)
        debug << Debug::color(Debug::Color::Yellow) << layerName << Debug::resetColor;

    const UnsignedInt style = layer.style(data);
    if(style >= layer.shared().styleCount()) {
        CORRADE_INTERNAL_DEBUG_ASSERT(style <= layer.shared().totalStyleCount());
        debug << "with dynamic style" << style - layer.shared().styleCount() << Debug::newline;
    } else {
        /* Collect all transitioned styles */
        const UnsignedInt styleInactiveOut = layer.shared().styleTransitionToInactiveOut()(style);
        const UnsignedInt styleInactiveOver = layer.shared().styleTransitionToInactiveOver()(style);
        const UnsignedInt styleFocusedOut = layer.shared().styleTransitionToFocusedOut()(style);
        const UnsignedInt styleFocusedOver = layer.shared().styleTransitionToFocusedOver()(style);
        const UnsignedInt stylePressedOut = layer.shared().styleTransitionToPressedOut()(style);
        const UnsignedInt stylePressedOver = layer.shared().styleTransitionToPressedOver()(style);
        /* If disabled transition isn't set, assume it's the same as inactive
           out (which it should as when a node gets disabled it goes through
           pointer out and pointer release) */
        const UnsignedInt styleDisabled =
            layer.shared().styleTransitionToDisabled() ?
                layer.shared().styleTransitionToDisabled()(style) : styleInactiveOut;

        const auto printStyle = [&debug, this](UnsignedInt style) {
            Containers::StringView name;
            if(_styleName)
                name = _styleName(style);
            if(name)
                debug << Debug::color(Debug::Color::Yellow) << name << Debug::resetColor << "(" << Debug::nospace << style << Debug::nospace << ")";
            else
                debug << style;
            debug << Debug::newline;
        };

        /* If they're all the same as the current one, print just one */
        if(styleInactiveOut == style &&
           styleInactiveOver == style &&
           styleFocusedOut == style &&
           styleFocusedOver == style &&
           stylePressedOut == style &&
           stylePressedOver == style &&
           styleDisabled == style) {
            debug << "with style";
            printStyle(style);

        } else {
            /* Print the current style if it's not in any transitions, and just
               a newline if it is */
            if(styleInactiveOut != style &&
               styleInactiveOver != style &&
               styleFocusedOut != style &&
               styleFocusedOver != style &&
               stylePressedOut != style &&
               stylePressedOver != style &&
               styleDisabled != style) {
                debug << "with style";
                printStyle(style);
            } else debug << Debug::newline;

            /* Inactive style is shown always, but if out and over is the same,
               print just one */
            if(styleInactiveOver == styleInactiveOut) {
                debug << "    Inactive style:";
                printStyle(styleInactiveOut);
            } else {
                debug << "    Inactive out style:";
                printStyle(styleInactiveOut);
                debug << "    Inactive over style:";
                printStyle(styleInactiveOver);
            }

            /* Print the focused style only if different from inactive */
            if(styleFocusedOut != styleInactiveOut ||
               styleFocusedOver != styleInactiveOver) {
                /* If out and over is the same, print just one */
                if(styleFocusedOver == styleFocusedOut) {
                    debug << "    Focused style:";
                    printStyle(styleFocusedOut);
                } else {
                    debug << "    Focused out style:";
                    printStyle(styleFocusedOut);
                    debug << "    Focused over style:";
                    printStyle(styleFocusedOver);
                }
            }

            /* Print the pressed style only if different from inactive */
            if(stylePressedOut != styleInactiveOut ||
               stylePressedOver != styleInactiveOver) {
                /* If out and over is the same, print just one */
                if(stylePressedOver == stylePressedOut) {
                    debug << "    Pressed style:";
                    printStyle(stylePressedOut);
                } else {
                    debug << "    Pressed out style:";
                    printStyle(stylePressedOut);
                    debug << "    Pressed over style:";
                    printStyle(stylePressedOver);
                }
            }

            /* Print the disabled style only if different from inactive out */
            if(styleDisabled != styleInactiveOut) {
                debug << "    Disabled style:";
                printStyle(styleDisabled);
            }
        }
    }
}

}}
