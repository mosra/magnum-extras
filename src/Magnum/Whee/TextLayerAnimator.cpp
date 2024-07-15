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

#include "TextLayerAnimator.h"

#include <Corrade/Containers/ArrayTuple.h>
#include <Corrade/Containers/BitArrayView.h>
#include <Corrade/Containers/EnumSet.hpp>
#include <Corrade/Containers/GrowableArray.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Magnum/Math/Time.h>

#include "Magnum/Whee/TextLayer.h"
#include "Magnum/Whee/Handle.h"
#include "Magnum/Whee/Implementation/textLayerState.h"

namespace Magnum { namespace Whee {

Debug& operator<<(Debug& debug, const TextLayerStyleAnimation value) {
    debug << "Whee::TextLayerStyleAnimation" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case TextLayerStyleAnimation::value: return debug << "::" #value;
        _c(Uniform)
        _c(Padding)
        _c(Style)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "(" << Debug::nospace << Debug::hex << UnsignedByte(value) << Debug::nospace << ")";
}

Debug& operator<<(Debug& debug, const TextLayerStyleAnimations value) {
    return Containers::enumSetDebugOutput(debug, value, "Whee::TextLayerStyleAnimations{}", {
        TextLayerStyleAnimation::Uniform,
        TextLayerStyleAnimation::Padding,
        TextLayerStyleAnimation::Style
    });
}

namespace {

struct Animation {
    /* As the Animation entries get recycled, all fields have to be overwritten
       always, thus there's no point in initializing them on the first ever
       construction either */
    TextLayerStyleUniform sourceUniform{NoInit}, targetUniform{NoInit};
    Vector4 sourcePadding{NoInit}, targetPadding{NoInit};
    UnsignedInt targetStyle, dynamicStyle;
    bool uniformDifferent;
    /* 3/7 bytes free */
    Float(*easing)(Float);
};

}

struct TextLayerStyleAnimator::State {
    TextLayer* layer = nullptr;
    const TextLayer::Shared::State* layerSharedState = nullptr;
    Containers::Array<Animation> animations;
};

TextLayerStyleAnimator::TextLayerStyleAnimator(AnimatorHandle handle): AbstractStyleAnimator{handle}, _state{InPlaceInit} {}

TextLayerStyleAnimator::TextLayerStyleAnimator(TextLayerStyleAnimator&&) noexcept = default;

TextLayerStyleAnimator::~TextLayerStyleAnimator() = default;

TextLayerStyleAnimator& TextLayerStyleAnimator::operator=(TextLayerStyleAnimator&&) noexcept = default;

AnimationHandle TextLayerStyleAnimator::create(const UnsignedInt sourceStyle, const UnsignedInt targetStyle, Float(*const easing)(Float), const Nanoseconds played, const Nanoseconds duration, const DataHandle data, const UnsignedInt repeatCount, const AnimationFlags flags) {
    /* AbstractAnimator::create() DataHandle overload checks the layer
       internally too, but this message is less confusing */
    CORRADE_ASSERT(_state->layer,
        "Whee::TextLayerStyleAnimator::create(): no layer set", {});
    const AnimationHandle handle = AbstractStyleAnimator::create(played, duration, data, repeatCount, flags);
    createInternal(handle, sourceStyle, targetStyle, easing);
    return handle;
}

AnimationHandle TextLayerStyleAnimator::create(const UnsignedInt styleFrom, const UnsignedInt styleTo, Float(*const easing)(Float), const Nanoseconds played, const Nanoseconds duration, const DataHandle data, const AnimationFlags flags) {
    return create(styleFrom, styleTo, easing, played, duration, data, 1, flags);
}

AnimationHandle TextLayerStyleAnimator::create(const UnsignedInt sourceStyle, const UnsignedInt targetStyle, Float(*const easing)(Float), const Nanoseconds played, const Nanoseconds duration, const LayerDataHandle data, const UnsignedInt repeatCount, const AnimationFlags flags) {
    /* AbstractAnimator::create() DataHandle overload checks the layer
       internally too, but this message is less confusing */
    CORRADE_ASSERT(_state->layer,
        "Whee::TextLayerStyleAnimator::create(): no layer set", {});
    const AnimationHandle handle = AbstractStyleAnimator::create(played, duration, data, repeatCount, flags);
    createInternal(handle, sourceStyle, targetStyle, easing);
    return handle;
}

AnimationHandle TextLayerStyleAnimator::create(const UnsignedInt sourceStyle, const UnsignedInt targetStyle, Float(*const easing)(Float), const Nanoseconds played, const Nanoseconds duration, const LayerDataHandle data, const AnimationFlags flags) {
    return create(sourceStyle, targetStyle, easing, played, duration, data, 1, flags);
}

void TextLayerStyleAnimator::createInternal(const AnimationHandle handle, const UnsignedInt sourceStyle, const UnsignedInt targetStyle, Float(*const easing)(Float)) {
    State& state = *_state;
    /* Layer being set had to be checked in create() already */
    CORRADE_INTERNAL_ASSERT(state.layerSharedState);
    const TextLayer::Shared::State& layerSharedState = *state.layerSharedState;
    CORRADE_ASSERT(layerSharedState.setStyleCalled,
        "Whee::TextLayerStyleAnimator::create(): no style data was set on the layer", );
    CORRADE_ASSERT(
        sourceStyle < layerSharedState.styleCount &&
        targetStyle < layerSharedState.styleCount,
        "Whee::TextLayerStyleAnimator::create(): expected source and target style to be in range for" << layerSharedState.styleCount << "styles but got" << sourceStyle << "and" << targetStyle, );
    CORRADE_ASSERT(easing,
        "Whee::TextLayerStyleAnimator::create(): easing is null", );

    const UnsignedInt id = animationHandleId(handle);
    if(id >= state.animations.size())
        arrayResize(state.animations, NoInit, id + 1);
    Animation& animation = state.animations[id];
    animation.targetStyle = targetStyle;
    animation.dynamicStyle = ~UnsignedInt{};
    animation.easing = easing;

    const Implementation::TextLayerStyle& sourceStyleData = layerSharedState.styles[sourceStyle];
    const Implementation::TextLayerStyle& targetStyleData = layerSharedState.styles[targetStyle];
    animation.sourcePadding = sourceStyleData.padding;
    animation.targetPadding = targetStyleData.padding;

    /* Remember also if the actual uniform ID is different, if not, we don't
       need to interpolate (or upload) it. The uniform *data* may still be the
       same even if the ID is different, but checking for that is too much work
       and any reasonable style should deduplicate those anyway. */
    animation.sourceUniform = layerSharedState.styleUniforms[sourceStyleData.uniform];
    animation.targetUniform = layerSharedState.styleUniforms[targetStyleData.uniform];
    animation.uniformDifferent = sourceStyleData.uniform != targetStyleData.uniform;
}

void TextLayerStyleAnimator::remove(const AnimationHandle handle) {
    AbstractAnimator::remove(handle);
    removeInternal(animationHandleId(handle));
}

void TextLayerStyleAnimator::remove(const AnimatorDataHandle handle) {
    AbstractAnimator::remove(handle);
    removeInternal(animatorDataHandleId(handle));
}

void TextLayerStyleAnimator::removeInternal(const UnsignedInt id) {
    /* If it gets here, the removed handle was valid. Thus it was create()d
       before and so the layer and everything should be set properly. */
    State& state = *_state;
    CORRADE_INTERNAL_ASSERT(state.layer);

    /* Recycle the dynamic style if it was allocated already. It might not be
       if advance() wasn't called for this animation yet or if it was already
       stopped by the time it reached advance(). */
    if(state.animations[id].dynamicStyle != ~UnsignedInt{})
        state.layer->recycleDynamicStyle(state.animations[id].dynamicStyle);
}

UnsignedInt TextLayerStyleAnimator::targetStyle(const AnimationHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::TextLayerStyleAnimator::targetStyle(): invalid handle" << handle, {});
    return _state->animations[animationHandleId(handle)].targetStyle;
}

UnsignedInt TextLayerStyleAnimator::targetStyle(const AnimatorDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::TextLayerStyleAnimator::targetStyle(): invalid handle" << handle, {});
    return _state->animations[animatorDataHandleId(handle)].targetStyle;
}

Containers::Optional<UnsignedInt> TextLayerStyleAnimator::dynamicStyle(const AnimationHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::TextLayerStyleAnimator::dynamicStyle(): invalid handle" << handle, {});
    const UnsignedInt style = _state->animations[animationHandleId(handle)].dynamicStyle;
    return style == ~UnsignedInt{} ? Containers::NullOpt : Containers::optional(style);
}

Containers::Optional<UnsignedInt> TextLayerStyleAnimator::dynamicStyle(const AnimatorDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::TextLayerStyleAnimator::dynamicStyle(): invalid handle" << handle, {});
    const UnsignedInt style = _state->animations[animatorDataHandleId(handle)].dynamicStyle;
    return style == ~UnsignedInt{} ? Containers::NullOpt : Containers::optional(style);
}

auto TextLayerStyleAnimator::easing(const AnimationHandle handle) const -> Float(*)(Float) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::TextLayerStyleAnimator::easing(): invalid handle" << handle, {});
    return _state->animations[animationHandleId(handle)].easing;
}

auto TextLayerStyleAnimator::easing(const AnimatorDataHandle handle) const -> Float(*)(Float) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::TextLayerStyleAnimator::easing(): invalid handle" << handle, {});
    return _state->animations[animatorDataHandleId(handle)].easing;
}

Containers::Pair<TextLayerStyleUniform, TextLayerStyleUniform> TextLayerStyleAnimator::uniforms(const AnimationHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::TextLayerStyleAnimator::uniforms(): invalid handle" << handle, {});
    const Animation& animation = _state->animations[animationHandleId(handle)];
    return {animation.sourceUniform, animation.targetUniform};
}

Containers::Pair<TextLayerStyleUniform, TextLayerStyleUniform> TextLayerStyleAnimator::uniforms(const AnimatorDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::TextLayerStyleAnimator::uniforms(): invalid handle" << handle, {});
    const Animation& animation = _state->animations[animatorDataHandleId(handle)];
    return {animation.sourceUniform, animation.targetUniform};
}

Containers::Pair<Vector4, Vector4> TextLayerStyleAnimator::paddings(const AnimationHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::TextLayerStyleAnimator::paddings(): invalid handle" << handle, {});
    const Animation& animation = _state->animations[animationHandleId(handle)];
    return {animation.sourcePadding, animation.targetPadding};
}

Containers::Pair<Vector4, Vector4> TextLayerStyleAnimator::paddings(const AnimatorDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Whee::TextLayerStyleAnimator::paddings(): invalid handle" << handle, {});
    const Animation& animation = _state->animations[animatorDataHandleId(handle)];
    return {animation.sourcePadding, animation.targetPadding};
}

TextLayerStyleAnimations TextLayerStyleAnimator::advance(const Nanoseconds time, const Containers::ArrayView<TextLayerStyleUniform> dynamicStyleUniforms, const Containers::StridedArrayView1D<Vector4>& dynamicStylePaddings, const Containers::StridedArrayView1D<UnsignedInt>& dataStyles) {
    CORRADE_ASSERT(dynamicStylePaddings.size() == dynamicStyleUniforms.size(),
        "Whee::TextLayerStyleAnimator::advance(): expected dynamic style uniform and padding views to have the same size but got" << dynamicStyleUniforms.size() << "and" << dynamicStylePaddings.size(), {});

    /** @todo have some bump allocator for this (doesn't make sense to have it
        as a persistent allocation as the memory could be shared among several
        animators) */
    Containers::ArrayView<Float> factors;
    Containers::MutableBitArrayView active;
    Containers::MutableBitArrayView remove;
    const Containers::ArrayTuple storage{
        {NoInit, capacity(), factors},
        {ValueInit, capacity(), active},
        {ValueInit, capacity(), remove},
    };
    const Containers::Pair<bool, bool> advanceCleanNeeded = AbstractAnimator::advance(time, active, factors, remove);

    State& state = *_state;

    TextLayerStyleAnimations animations;
    if(advanceCleanNeeded.first()) {
        /* If there are any running animations, create() had to be called
           already, which ensures the layer is already set */
        CORRADE_INTERNAL_ASSERT(state.layerSharedState);
        const TextLayer::Shared::State& layerSharedState = *state.layerSharedState;

        const Containers::StridedArrayView1D<const LayerDataHandle> layerData = this->layerData();

        /** @todo some way to iterate set bits */
        for(std::size_t i = 0; i != active.size(); ++i) {
            if(!active[i]) continue;

            Animation& animation = state.animations[i];
            /* The handle is assumed to be valid if not null, i.e. that
               appropriate dataClean() got called before advance() */
            const LayerDataHandle data = layerData[i];

            /* If the animation is scheduled for removal (and thus finished),
               switch the data to the target style, if any. No need to animate
               anything else as the dynamic style is going to get recycled
               right away in clean() below. */
            if(remove[i]) {
                CORRADE_INTERNAL_ASSERT(factors[i] == 1.0f);
                if(data != LayerDataHandle::Null) {
                    dataStyles[layerDataHandleId(data)] = animation.targetStyle;
                    animations |= TextLayerStyleAnimation::Style;
                }
                continue;
            }

            /* The animation is running, allocate a dynamic style if it isn't
               yet and switch to it. Doing it here instead of in create()
               avoids unnecessary pressure on peak used count of dynamic
               styles, especially when there's a lot of animations
               scheduled. */
            if(animation.dynamicStyle == ~UnsignedInt{}) {
                /* If dynamic style allocation fails (for example because
                   there's too many animations running at the same time), do
                   nothing -- the data stays at the original style, causing no
                   random visual glitches, and we'll try in next advance()
                   again (where some animations may already be finished,
                   freeing up some slots, and there we'll also advance to a
                   later point in the animation).

                   A better way would be to recycle the oldest running
                   animations, but there's no logic for that so far, so do the
                   second best thing at least. One could also just let it
                   assert when there's no free slots anymore, but letting a
                   program assert just because it couldn't animate feels
                   silly. */
                const Containers::Optional<UnsignedInt> style = state.layer->allocateDynamicStyle();
                if(!style)
                    continue;
                animation.dynamicStyle = *style;

                if(data != LayerDataHandle::Null) {
                    dataStyles[layerDataHandleId(data)] = layerSharedState.styleCount + animation.dynamicStyle;
                    animations |= TextLayerStyleAnimation::Style;
                    /* If the uniform IDs are the same between the source and
                       target style, the uniform interpolation below won't
                       happen. We still need to upload it at least once though,
                       so trigger it here unconditionally. */
                    animations |= TextLayerStyleAnimation::Uniform;
                }
            }

            const Float factor = animation.easing(factors[i]);

            /* Interpolate the uniform. If the source and target uniforms were
               the same, just copy one of them and don't report that the
               uniforms got changed. The only exception is the first ever
               switch to the dynamic uniform in which case the data has to be
               uploaded. That's handled in the animation.dynamicStyle
               allocation above. */
            if(animation.uniformDifferent) {
                TextLayerStyleUniform uniform{NoInit};
                #define _c(member) uniform.member = Math::lerp(             \
                    animation.sourceUniform.member,                         \
                    animation.targetUniform.member, factor);
                _c(color)
                #undef _c
                dynamicStyleUniforms[animation.dynamicStyle] = uniform;
                animations |= TextLayerStyleAnimation::Uniform;
            } else dynamicStyleUniforms[animation.dynamicStyle] = animation.targetUniform;

            /* Interpolate the padding. Compared to the uniforms, updated
               padding causes doUpdate() to be triggered on the layer, which is
               expensive, thus trigger it only if there's actually anything
               changing. */
            const Vector4 padding = Math::lerp(animation.sourcePadding,
                                               animation.targetPadding, factor);
            if(dynamicStylePaddings[animation.dynamicStyle] != padding) {
               dynamicStylePaddings[animation.dynamicStyle] = padding;
                animations |= TextLayerStyleAnimation::Padding;
            }
        }
    }

    if(advanceCleanNeeded.second())
        clean(remove);

    return animations;
}

void TextLayerStyleAnimator::setLayerInstance(TextLayer& instance, const void* sharedState) {
    /* This is called from TextLayer::setAnimator(), which should itself
       prevent the layer from being set more than once */
    CORRADE_INTERNAL_ASSERT(!_state->layer && sharedState);
    _state->layer = &instance;
    _state->layerSharedState = reinterpret_cast<const TextLayer::Shared::State*>(sharedState);
}

void TextLayerStyleAnimator::doClean(const Containers::BitArrayView animationIdsToRemove) {
    State& state = *_state;
    /* If any animations were created, the layer was ensured to be set by
       create() already. Otherwise it doesn't need to be as the loop below is
       empty. */
    CORRADE_INTERNAL_ASSERT(animationIdsToRemove.isEmpty() || state.layer);

    /** @todo some way to iterate set bits */
    for(std::size_t i = 0; i != animationIdsToRemove.size(); ++i) {
        if(!animationIdsToRemove[i]) continue;

        /* Recycle the dynamic style if it was allocated already. It might not
           be if advance() wasn't called for this animation yet or if it was
           already stopped by the time it reached advance(). */
        if(state.animations[i].dynamicStyle != ~UnsignedInt{})
            state.layer->recycleDynamicStyle(state.animations[i].dynamicStyle);

        /* As doClean() is only ever called from within advance() or from
           cleanData() (i.e., when the data the animation is attached to is
           removed), there's no need to deal with resetting the style away from
           the now-recycled dynamic one here -- it was either already done in
           advance() or there's no point in doing it as the data itself is
           removed already */
    }
}

}}
