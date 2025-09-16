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

    /* Recycle the dynamic style if it's allocated. It might not be if
       advance() wasn't called for this animation yet or if it was already
       stopped by the time it's removed. */
    if(state.dynamicStyles[id] != ~UnsignedInt{})
        state.layer->recycleDynamicStyle(state.dynamicStyles[id]);
}

Containers::Pair<UnsignedInt, UnsignedInt> AbstractVisualLayerStyleAnimator::styles(const AnimationHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractVisualLayerStyleAnimator::styles(): invalid handle" << handle, {});
    const State& state = *_state;
    return {state.sourceStyles[animationHandleId(handle)], state.targetStyles[animationHandleId(handle)]};
}

Containers::Pair<UnsignedInt, UnsignedInt> AbstractVisualLayerStyleAnimator::styles(const AnimatorDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractVisualLayerStyleAnimator::styles(): invalid handle" << handle, {});
    const State& state = *_state;
    return {state.sourceStyles[animatorDataHandleId(handle)], state.targetStyles[animatorDataHandleId(handle)]};
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

Containers::Pair<bool, bool> AbstractVisualLayerStyleAnimator::advance(const Containers::BitArrayView active, const Containers::BitArrayView started, const Containers::BitArrayView stopped, const Containers::StridedArrayView1D<UnsignedInt>& dataStyles) {
    /* This function should only be called if there's a layer set already. The
       sizes should be already checked by subclasses along with factors and
       other layer-specific inputs. */
    State& state = *_state;
    CORRADE_INTERNAL_DEBUG_ASSERT(state.layerSharedState &&
        active.size() == capacity() &&
        started.size() == capacity() &&
        stopped.size() == capacity());

    const Containers::StridedArrayView1D<const LayerDataHandle> layerData = this->layerData();
    const Containers::StridedArrayView1D<const AnimationFlags> flags = this->flags();

    bool updatedStyle = false;
    bool updatedUniform = false;
    /** @todo some way to iterate set bits */
    for(std::size_t i = 0; i != active.size(); ++i) {
        if(!active[i])
            continue;

        UnsignedInt& dynamicStyle = state.dynamicStyles[i];
        /* The handle is assumed to be valid if not null, i.e. that appropriate
           dataClean() got called before advance() */
        const LayerDataHandle data = layerData[i];

        /* If the animation is started, remember what style ID the data has
           now. It gets compared against once the dynamic style is about to be
           allocated (which can happen either immediately or at a later time if
           there are no free styles), ensuring a stale animation isn't going to
           get played if the style changed in the meantime.

           As the animation can become started at any point, such as when an
           already playing animation is being restarted, the previous value of
           expectedStyle can be just anything and thus no consistency asserts
           are here. */
        if(started[i]) {
            if(data != LayerDataHandle::Null)
                state.expectedStyles[i] = dataStyles[layerDataHandleId(data)];
            /* If the data is null, no style ID is going to be switched
               anywhere and so we don't need to remember the style. Reset the
               variable so it doesn't contain a stale value in case it's a
               recycled / restarted slot, which could lead to accidentally
               switching styles that should stay untouched. */
            else
                state.expectedStyles[i] = ~UnsignedInt{};
        }

        /* If the animation is stopped, switch the data to the target style, if
           any. No need to animate anything else as the dynamic style is going
           to get recycled right away. */
        if(stopped[i]) {
            if(data != LayerDataHandle::Null) {
                /* Switch to the target style only if the style didn't change
                   from the expected one, as we'd break animations and style
                   changes that happened since this animation started. The
                   expectedStyles[i] is usually equal to dynamicStyle, but
                   could be also the original style if there was no free
                   dynamic style to use during the whole animation duration.

                   The expectedStyles[i] can also be ~UnsignedInt{} in case the
                   animation got attached to a data only later after it
                   started. In that case this branch will never be taken,
                   resulting in the animation never actually applied to the
                   data it got attached to. */
                if(dataStyles[layerDataHandleId(data)] == state.expectedStyles[i]) {
                    dataStyles[layerDataHandleId(data)] = flags[i] >= AnimationFlag::Reverse ?
                        state.sourceStyles[i] : state.targetStyles[i];
                    updatedStyle = true;
                }
            }

            /* Recycle the dynamic style if it was allocated already. It might
               not be if advance() wasn't called for this animation yet or if
               it was already stopped by the time it reached advance(). */
            if(dynamicStyle != ~UnsignedInt{}) {
                state.layer->recycleDynamicStyle(dynamicStyle);
                dynamicStyle = ~UnsignedInt{};
            }

            continue;
        }

        /* The animation is running, allocate a dynamic style if it isn't yet
           and switch to it. Doing it here instead of in create() avoids
           unnecessary pressure on peak used count of dynamic styles,
           especially when there's a lot of animations scheduled. */
        if(dynamicStyle == ~UnsignedInt{}) {
            /* If we're attached to data and its style assignment changed since
               start, bail without allocating a dynamic style. Same as in the
               stopped case above, if we'd switch the style we'd break
               animations and style changes that happened since this
               animation started.

               Also reset the expected style to ensure the animation doesn't
               get suddenly revived when the data coincidentally happens to
               switch to the previously expected style. */
            if(data != LayerDataHandle::Null && state.expectedStyles[i] != dataStyles[layerDataHandleId(data)]) {
                state.expectedStyles[i] = ~UnsignedInt{};
                continue;
            }

            /* If dynamic style allocation fails (for example because there's
               too many animations running at the same time), do nothing -- the
               data stays at the original style, causing no random visual
               glitches, and we'll try in next advance() again (where some
               animations may already be finished, freeing up some slots, and
               there we'll also advance to a later point in the animation).

               A better way would be to recycle the oldest running animations,
               but there's no logic for that so far, so do the second best
               thing at least. One could also just let it assert when there's
               no free slots anymore, but letting a program assert just because
               it couldn't animate feels silly. */
            const Containers::Optional<UnsignedInt> style = state.layer->allocateDynamicStyle(animationHandle(handle(), i, generations()[i]));
            if(!style)
                continue;
            dynamicStyle = *style;

            if(data != LayerDataHandle::Null) {
                dataStyles[layerDataHandleId(data)] = state.layerSharedState->styleCount + dynamicStyle;
                state.expectedStyles[i] = state.layerSharedState->styleCount + dynamicStyle;
                updatedStyle = true;
            }

            /* If the uniform IDs are the same between the source and target
               style, the uniform interpolation below won't happen. We still
               need to upload it at least once though, so trigger it here
               unconditionally, and do it even with no attachment, as the
               dynamic style can be used in some way in that case as well. */
            updatedUniform = true;
        }
    }

    return {updatedStyle, updatedUniform};
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

        /* Recycle the dynamic style if it's allocated. It might not be if
           advance() wasn't called for this animation yet or if it was already
           stopped by the time it's removed. */
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
