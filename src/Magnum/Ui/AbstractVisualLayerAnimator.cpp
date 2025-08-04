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

#include "AbstractVisualLayerAnimator.h"

#include <Corrade/Containers/Optional.h>

#include "Magnum/Ui/AbstractVisualLayer.h"
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/Implementation/abstractVisualLayerAnimatorState.h"

namespace Magnum { namespace Ui {

AbstractVisualLayerStyleAnimator::AbstractVisualLayerStyleAnimator(AnimatorHandle handle, Containers::Pointer<State>&& state): AbstractStyleAnimator{handle}, _state{Utility::move(state)} {}

AbstractVisualLayerStyleAnimator::AbstractVisualLayerStyleAnimator(AnimatorHandle handle): AbstractVisualLayerStyleAnimator{handle, Containers::pointer<State>()} {}

AbstractVisualLayerStyleAnimator::AbstractVisualLayerStyleAnimator(AbstractVisualLayerStyleAnimator&&) noexcept = default;

AbstractVisualLayerStyleAnimator::~AbstractVisualLayerStyleAnimator() = default;

AbstractVisualLayerStyleAnimator& AbstractVisualLayerStyleAnimator::operator=(AbstractVisualLayerStyleAnimator&&) noexcept = default;

void AbstractVisualLayerStyleAnimator::removeInternal(const UnsignedInt id) {
    /* If it gets here, the removed handle was valid. Thus it was create()d
       before and so the layer and everything should be set properly. */
    State& state = *_state;
    CORRADE_INTERNAL_ASSERT(state.layer && state.dynamicStyles.size() == capacity());

    /* Recycle the dynamic style if it was allocated already. It might not be
       if advance() wasn't called for this animation yet or if it was already
       stopped by the time it reached advance(). */
    if(state.dynamicStyles[id] != ~UnsignedInt{})
        state.layer->recycleDynamicStyle(state.dynamicStyles[id]);
}

UnsignedInt AbstractVisualLayerStyleAnimator::targetStyle(const AnimationHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractVisualLayerStyleAnimator::targetStyle(): invalid handle" << handle, {});
    const State& state = *_state;
    CORRADE_INTERNAL_ASSERT(state.targetStyles.size() == capacity());
    return state.targetStyles[animationHandleId(handle)];
}

UnsignedInt AbstractVisualLayerStyleAnimator::targetStyle(const AnimatorDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractVisualLayerStyleAnimator::targetStyle(): invalid handle" << handle, {});
    const State& state = *_state;
    CORRADE_INTERNAL_ASSERT(state.targetStyles.size() == capacity());
    return state.targetStyles[animatorDataHandleId(handle)];
}

Containers::Optional<UnsignedInt> AbstractVisualLayerStyleAnimator::dynamicStyle(const AnimationHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractVisualLayerStyleAnimator::dynamicStyle(): invalid handle" << handle, {});
    const State& state = *_state;
    CORRADE_INTERNAL_ASSERT(state.dynamicStyles.size() == capacity());
    const UnsignedInt style = state.dynamicStyles[animationHandleId(handle)];
    return style == ~UnsignedInt{} ? Containers::NullOpt : Containers::optional(style);
}

Containers::Optional<UnsignedInt> AbstractVisualLayerStyleAnimator::dynamicStyle(const AnimatorDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractVisualLayerStyleAnimator::dynamicStyle(): invalid handle" << handle, {});
    const State& state = *_state;
    CORRADE_INTERNAL_ASSERT(state.dynamicStyles.size() == capacity());
    const UnsignedInt style = state.dynamicStyles[animatorDataHandleId(handle)];
    return style == ~UnsignedInt{} ? Containers::NullOpt : Containers::optional(style);
}

void AbstractVisualLayerStyleAnimator::setLayerInstance(AbstractVisualLayer& instance, const void* sharedState) {
    /* This is called from AbstractVisualLayer::assignAnimator(), which should
       itself prevent the layer from being set more than once */
    CORRADE_INTERNAL_ASSERT(!_state->layer && sharedState);
    _state->layer = &instance;
    _state->layerSharedState = reinterpret_cast<const AbstractVisualLayer::Shared::State*>(sharedState);
}

void AbstractVisualLayerStyleAnimator::doClean(const Containers::BitArrayView animationIdsToRemove) {
    State& state = *_state;
    /* If any animations were created, the layer was ensured to be set by
       create() already. Otherwise it doesn't need to be as the loop below is
       empty. */
    CORRADE_INTERNAL_ASSERT(animationIdsToRemove.isEmpty() || (state.layer && state.dynamicStyles.size() == capacity()));

    /** @todo some way to iterate set bits */
    for(std::size_t i = 0; i != animationIdsToRemove.size(); ++i) {
        if(!animationIdsToRemove[i])
            continue;

        /* Recycle the dynamic style if it was allocated already. It might not
           be if advance() wasn't called for this animation yet or if it was
           already stopped by the time it reached advance(). */
        if(state.dynamicStyles[i] != ~UnsignedInt{})
            state.layer->recycleDynamicStyle(state.dynamicStyles[i]);

        /* As doClean() is only ever called from within advance() or from
           cleanData() (i.e., when the data the animation is attached to is
           removed), there's no need to deal with resetting the style away from
           the now-recycled dynamic one here -- it was either already done in
           advance() or there's no point in doing it as the data itself is
           removed already */
    }
}

}}
