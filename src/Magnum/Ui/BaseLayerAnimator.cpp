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

#include "BaseLayerAnimator.h"

#include <Corrade/Containers/ArrayTuple.h>
#include <Corrade/Containers/BitArrayView.h>
#include <Corrade/Containers/EnumSet.hpp>
#include <Corrade/Containers/GrowableArray.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Magnum/Math/Time.h>

#include "Magnum/Ui/BaseLayer.h"
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/Implementation/abstractVisualLayerAnimatorState.h"
#include "Magnum/Ui/Implementation/baseLayerState.h"

namespace Magnum { namespace Ui {

Debug& operator<<(Debug& debug, const BaseLayerStyleAnimation value) {
    debug << "Ui::BaseLayerStyleAnimation" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case BaseLayerStyleAnimation::value: return debug << "::" #value;
        _c(Uniform)
        _c(Padding)
        _c(Style)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "(" << Debug::nospace << Debug::hex << UnsignedByte(value) << Debug::nospace << ")";
}

Debug& operator<<(Debug& debug, const BaseLayerStyleAnimations value) {
    return Containers::enumSetDebugOutput(debug, value, "Ui::BaseLayerStyleAnimations{}", {
        BaseLayerStyleAnimation::Uniform,
        BaseLayerStyleAnimation::Padding,
        BaseLayerStyleAnimation::Style
    });
}

namespace {

struct Animation {
    /* As the Animation entries get recycled, all fields have to be overwritten
       always, thus there's no point in initializing them on the first ever
       construction either */
    BaseLayerStyleUniform sourceUniform{NoInit}, targetUniform{NoInit};
    Vector4 sourcePadding{NoInit}, targetPadding{NoInit};
    UnsignedInt targetStyle, dynamicStyle;
    bool uniformDifferent;
    /* 3/7 bytes free */
    Float(*easing)(Float);
};

}

struct BaseLayerStyleAnimator::State: AbstractVisualLayerStyleAnimator::State {
    Containers::Array<Animation> animations;
};

BaseLayerStyleAnimator::BaseLayerStyleAnimator(AnimatorHandle handle): AbstractVisualLayerStyleAnimator{handle, Containers::pointer<State>()} {}

BaseLayerStyleAnimator::BaseLayerStyleAnimator(BaseLayerStyleAnimator&&) noexcept = default;

BaseLayerStyleAnimator::~BaseLayerStyleAnimator() = default;

BaseLayerStyleAnimator& BaseLayerStyleAnimator::operator=(BaseLayerStyleAnimator&&) noexcept = default;

AnimationHandle BaseLayerStyleAnimator::create(const UnsignedInt sourceStyle, const UnsignedInt targetStyle, Float(*const easing)(Float), const Nanoseconds played, const Nanoseconds duration, const DataHandle data, const UnsignedInt repeatCount, const AnimationFlags flags) {
    /* AbstractAnimator::create() DataHandle overload checks the layer
       internally too, but this message is less confusing */
    CORRADE_ASSERT(static_cast<State&>(*_state).layer,
        "Ui::BaseLayerStyleAnimator::create(): no layer set", {});
    const AnimationHandle handle = AbstractStyleAnimator::create(played, duration, data, repeatCount, flags);
    createInternal(handle, sourceStyle, targetStyle, easing);
    return handle;
}

AnimationHandle BaseLayerStyleAnimator::create(const UnsignedInt styleFrom, const UnsignedInt styleTo, Float(*const easing)(Float), const Nanoseconds played, const Nanoseconds duration, const DataHandle data, const AnimationFlags flags) {
    return create(styleFrom, styleTo, easing, played, duration, data, 1, flags);
}

AnimationHandle BaseLayerStyleAnimator::create(const UnsignedInt sourceStyle, const UnsignedInt targetStyle, Float(*const easing)(Float), const Nanoseconds played, const Nanoseconds duration, const LayerDataHandle data, const UnsignedInt repeatCount, const AnimationFlags flags) {
    /* AbstractAnimator::create() DataHandle overload checks the layer
       internally too, but this message is less confusing */
    CORRADE_ASSERT(_state->layer,
        "Ui::BaseLayerStyleAnimator::create(): no layer set", {});
    const AnimationHandle handle = AbstractStyleAnimator::create(played, duration, data, repeatCount, flags);
    createInternal(handle, sourceStyle, targetStyle, easing);
    return handle;
}

AnimationHandle BaseLayerStyleAnimator::create(const UnsignedInt sourceStyle, const UnsignedInt targetStyle, Float(*const easing)(Float), const Nanoseconds played, const Nanoseconds duration, const LayerDataHandle data, const AnimationFlags flags) {
    return create(sourceStyle, targetStyle, easing, played, duration, data, 1, flags);
}

void BaseLayerStyleAnimator::createInternal(const AnimationHandle handle, const UnsignedInt sourceStyle, const UnsignedInt targetStyle, Float(*const easing)(Float)) {
    State& state = static_cast<State&>(*_state);
    /* Layer being set had to be checked in create() already */
    CORRADE_INTERNAL_ASSERT(state.layerSharedState);
    const BaseLayer::Shared::State& layerSharedState = static_cast<const BaseLayer::Shared::State&>(*state.layerSharedState);
    CORRADE_ASSERT(layerSharedState.setStyleCalled,
        "Ui::BaseLayerStyleAnimator::create(): no style data was set on the layer", );
    CORRADE_ASSERT(
        sourceStyle < layerSharedState.styleCount &&
        targetStyle < layerSharedState.styleCount,
        "Ui::BaseLayerStyleAnimator::create(): expected source and target style to be in range for" << layerSharedState.styleCount << "styles but got" << sourceStyle << "and" << targetStyle, );
    CORRADE_ASSERT(easing,
        "Ui::BaseLayerStyleAnimator::create(): easing is null", );

    const UnsignedInt id = animationHandleId(handle);
    if(id >= state.animations.size()) {
        arrayResize(state.animations, NoInit, id + 1);
        state.targetStyles = stridedArrayView(state.animations).slice(&Animation::targetStyle);
        state.dynamicStyles = stridedArrayView(state.animations).slice(&Animation::dynamicStyle);
    }
    Animation& animation = state.animations[id];
    animation.targetStyle = targetStyle;
    animation.dynamicStyle = ~UnsignedInt{};
    animation.easing = easing;

    const Implementation::BaseLayerStyle& sourceStyleData = layerSharedState.styles[sourceStyle];
    const Implementation::BaseLayerStyle& targetStyleData = layerSharedState.styles[targetStyle];
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

void BaseLayerStyleAnimator::remove(const AnimationHandle handle) {
    AbstractAnimator::remove(handle);
    removeInternal(animationHandleId(handle));
}

void BaseLayerStyleAnimator::remove(const AnimatorDataHandle handle) {
    AbstractAnimator::remove(handle);
    removeInternal(animatorDataHandleId(handle));
}

auto BaseLayerStyleAnimator::easing(const AnimationHandle handle) const -> Float(*)(Float) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::BaseLayerStyleAnimator::easing(): invalid handle" << handle, {});
    return static_cast<const State&>(*_state).animations[animationHandleId(handle)].easing;
}

auto BaseLayerStyleAnimator::easing(const AnimatorDataHandle handle) const -> Float(*)(Float) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::BaseLayerStyleAnimator::easing(): invalid handle" << handle, {});
    return static_cast<const State&>(*_state).animations[animatorDataHandleId(handle)].easing;
}

Containers::Pair<BaseLayerStyleUniform, BaseLayerStyleUniform> BaseLayerStyleAnimator::uniforms(const AnimationHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::BaseLayerStyleAnimator::uniforms(): invalid handle" << handle, {});
    const Animation& animation = static_cast<const State&>(*_state).animations[animationHandleId(handle)];
    return {animation.sourceUniform, animation.targetUniform};
}

Containers::Pair<BaseLayerStyleUniform, BaseLayerStyleUniform> BaseLayerStyleAnimator::uniforms(const AnimatorDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::BaseLayerStyleAnimator::uniforms(): invalid handle" << handle, {});
    const Animation& animation = static_cast<const State&>(*_state).animations[animatorDataHandleId(handle)];
    return {animation.sourceUniform, animation.targetUniform};
}

Containers::Pair<Vector4, Vector4> BaseLayerStyleAnimator::paddings(const AnimationHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::BaseLayerStyleAnimator::paddings(): invalid handle" << handle, {});
    const Animation& animation = static_cast<const State&>(*_state).animations[animationHandleId(handle)];
    return {animation.sourcePadding, animation.targetPadding};
}

Containers::Pair<Vector4, Vector4> BaseLayerStyleAnimator::paddings(const AnimatorDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::BaseLayerStyleAnimator::paddings(): invalid handle" << handle, {});
    const Animation& animation = static_cast<const State&>(*_state).animations[animatorDataHandleId(handle)];
    return {animation.sourcePadding, animation.targetPadding};
}

BaseLayerStyleAnimations BaseLayerStyleAnimator::advance(const Containers::BitArrayView active, const Containers::StridedArrayView1D<const Float>& factors, const Containers::BitArrayView remove, const Containers::ArrayView<BaseLayerStyleUniform> dynamicStyleUniforms, const Containers::StridedArrayView1D<Vector4>& dynamicStylePaddings, const Containers::StridedArrayView1D<UnsignedInt>& dataStyles) {
    CORRADE_ASSERT(active.size() == capacity() &&
                   factors.size() == capacity() &&
                   remove.size() == capacity(),
        "Ui::BaseLayerStyleAnimator::advance(): expected active, factors and remove views to have a size of" << capacity() << "but got" << active.size() << Debug::nospace << "," << factors.size() << "and" << remove.size(), {});
    CORRADE_ASSERT(dynamicStylePaddings.size() == dynamicStyleUniforms.size(),
        "Ui::BaseLayerStyleAnimator::advance(): expected dynamic style uniform and padding views to have the same size but got" << dynamicStyleUniforms.size() << "and" << dynamicStylePaddings.size(), {});

    State& state = static_cast<State&>(*_state);

    /* If there are any running animations, create() had to be called
       already, which ensures the layer is already set */
    CORRADE_INTERNAL_ASSERT(!capacity() || state.layerSharedState);
    const BaseLayer::Shared::State& layerSharedState = static_cast<const BaseLayer::Shared::State&>(*state.layerSharedState);
    const Containers::StridedArrayView1D<const LayerDataHandle> layerData = this->layerData();

    BaseLayerStyleAnimations animations;
    /** @todo some way to iterate set bits */
    for(std::size_t i = 0; i != active.size(); ++i) {
        if(!active[i]) continue;

        Animation& animation = state.animations[i];
        /* The handle is assumed to be valid if not null, i.e. that appropriate
           dataClean() got called before advance() */
        const LayerDataHandle data = layerData[i];

        /* If the animation is scheduled for removal (and thus finished),
           switch the data to the target style, if any. No need to animate
           anything else as the dynamic style is going to get recycled right
           away in clean() after. */
        if(remove[i]) {
            CORRADE_INTERNAL_ASSERT(factors[i] == 1.0f);
            if(data != LayerDataHandle::Null) {
                dataStyles[layerDataHandleId(data)] = animation.targetStyle;
                animations |= BaseLayerStyleAnimation::Style;
            }
            continue;
        }

        /* The animation is running, allocate a dynamic style if it isn't yet
           and switch to it. Doing it here instead of in create() avoids
           unnecessary pressure on peak used count of dynamic styles,
           especially when there's a lot of animations scheduled. */
        if(animation.dynamicStyle == ~UnsignedInt{}) {
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
            animation.dynamicStyle = *style;

            if(data != LayerDataHandle::Null) {
                dataStyles[layerDataHandleId(data)] = layerSharedState.styleCount + animation.dynamicStyle;
                animations |= BaseLayerStyleAnimation::Style;
                /* If the uniform IDs are the same between the source and
                   target style, the uniform interpolation below won't happen.
                   We still need to upload it at least once though, so trigger
                   it here unconditionally. */
                animations |= BaseLayerStyleAnimation::Uniform;
            }
        }

        const Float factor = animation.easing(factors[i]);

        /* Interpolate the uniform. If the source and target uniforms were the
           same, just copy one of them and don't report that the uniforms got
           changed. The only exception is the first ever switch to the dynamic
           uniform in which case the data has to be uploaded. That's handled in
           the animation.styleDynamic allocation above. */
        if(animation.uniformDifferent) {
            BaseLayerStyleUniform uniform{NoInit};
            #define _c(member) uniform.member = Math::lerp(             \
                animation.sourceUniform.member,                         \
                animation.targetUniform.member, factor);
            _c(topColor)
            _c(bottomColor)
            _c(outlineColor)
            _c(outlineWidth)
            _c(cornerRadius)
            _c(innerOutlineCornerRadius)
            #undef _c
            dynamicStyleUniforms[animation.dynamicStyle] = uniform;
            animations |= BaseLayerStyleAnimation::Uniform;
        } else dynamicStyleUniforms[animation.dynamicStyle] = animation.targetUniform;

        /* Interpolate the padding. Compared to the uniforms, updated padding
           causes doUpdate() to be triggered on the layer, which is expensive,
           thus trigger it only if there's actually anything changing. */
        const Vector4 padding = Math::lerp(animation.sourcePadding,
                                           animation.targetPadding, factor);
        if(dynamicStylePaddings[animation.dynamicStyle] != padding) {
           dynamicStylePaddings[animation.dynamicStyle] = padding;
            animations |= BaseLayerStyleAnimation::Padding;
        }
    }

    return animations;
}

}}
