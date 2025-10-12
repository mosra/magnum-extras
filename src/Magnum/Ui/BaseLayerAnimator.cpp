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

#include "BaseLayerAnimator.h"

#include <Corrade/Containers/ArrayTuple.h>
#include <Corrade/Containers/BitArrayView.h>
#include <Corrade/Containers/EnumSet.hpp>
#include <Corrade/Containers/GrowableArray.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Containers/Triple.h>
#include <Magnum/Math/Time.h>

#include "Magnum/Ui/BaseLayer.h"
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/Implementation/abstractVisualLayerAnimatorState.h"
#include "Magnum/Ui/Implementation/baseLayerState.h"

namespace Magnum { namespace Ui {

Debug& operator<<(Debug& debug, const BaseLayerStyleAnimatorUpdate value) {
    debug << "Ui::BaseLayerStyleAnimatorUpdate" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case BaseLayerStyleAnimatorUpdate::value: return debug << "::" #value;
        _c(Uniform)
        _c(Padding)
        _c(Style)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "(" << Debug::nospace << Debug::hex << UnsignedByte(value) << Debug::nospace << ")";
}

Debug& operator<<(Debug& debug, const BaseLayerStyleAnimatorUpdates value) {
    return Containers::enumSetDebugOutput(debug, value, "Ui::BaseLayerStyleAnimatorUpdates{}", {
        BaseLayerStyleAnimatorUpdate::Uniform,
        BaseLayerStyleAnimatorUpdate::Padding,
        BaseLayerStyleAnimatorUpdate::Style
    });
}

namespace {

struct Animation {
    /* As the Animation entries get recycled, all fields have to be overwritten
       always, thus there's no point in initializing them on the first ever
       construction either */
    BaseLayerStyleUniform sourceUniform{NoInit}, targetUniform{NoInit};
    Vector4 sourcePadding{NoInit}, targetPadding{NoInit};
    UnsignedInt expectedStyle, sourceStyle, targetStyle, dynamicStyle;
    bool uniformDifferent;
    /* 7/3 bytes free */
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

AnimationHandle BaseLayerStyleAnimator::create(const UnsignedInt sourceStyle, const UnsignedInt targetStyle, Float(*const easing)(Float), const Nanoseconds start, const Nanoseconds duration, const DataHandle data, const UnsignedInt repeatCount, const AnimationFlags flags) {
    /* AbstractAnimator::create() DataHandle overload checks the layer
       internally too, but this message is less confusing */
    CORRADE_ASSERT(static_cast<State&>(*_state).layer,
        "Ui::BaseLayerStyleAnimator::create(): no layer set", {});
    const AnimationHandle handle = AbstractStyleAnimator::create(start, duration, data, repeatCount, flags);
    createInternal(handle, sourceStyle, targetStyle, easing);
    return handle;
}

AnimationHandle BaseLayerStyleAnimator::create(const UnsignedInt sourceStyle, const UnsignedInt targetStyle, Float(*const easing)(Float), const Nanoseconds start, const Nanoseconds duration, const DataHandle data, const AnimationFlags flags) {
    return create(sourceStyle, targetStyle, easing, start, duration, data, 1, flags);
}

AnimationHandle BaseLayerStyleAnimator::create(const UnsignedInt sourceStyle, const UnsignedInt targetStyle, Float(*const easing)(Float), const Nanoseconds start, const Nanoseconds duration, const LayerDataHandle data, const UnsignedInt repeatCount, const AnimationFlags flags) {
    /* AbstractAnimator::create() DataHandle overload checks the layer
       internally too, but this message is less confusing */
    CORRADE_ASSERT(_state->layer,
        "Ui::BaseLayerStyleAnimator::create(): no layer set", {});
    const AnimationHandle handle = AbstractStyleAnimator::create(start, duration, data, repeatCount, flags);
    createInternal(handle, sourceStyle, targetStyle, easing);
    return handle;
}

AnimationHandle BaseLayerStyleAnimator::create(const UnsignedInt sourceStyle, const UnsignedInt targetStyle, Float(*const easing)(Float), const Nanoseconds start, const Nanoseconds duration, const LayerDataHandle data, const AnimationFlags flags) {
    return create(sourceStyle, targetStyle, easing, start, duration, data, 1, flags);
}

void BaseLayerStyleAnimator::createInternal(const AnimationHandle handle, const UnsignedInt sourceStyle, const UnsignedInt targetStyle, Float(*const easing)(Float)) {
    State& state = static_cast<State&>(*_state);
    /* Layer being set had to be checked in create() already */
    CORRADE_INTERNAL_ASSERT(state.layerSharedState);
    #ifndef CORRADE_NO_ASSERT
    const BaseLayer::Shared::State& layerSharedState = static_cast<const BaseLayer::Shared::State&>(*state.layerSharedState);
    #endif
    CORRADE_ASSERT(
        sourceStyle < layerSharedState.styleCount &&
        targetStyle < layerSharedState.styleCount,
        "Ui::BaseLayerStyleAnimator::create(): expected source and target style to be in range for" << layerSharedState.styleCount << "styles but got" << sourceStyle << "and" << targetStyle, );
    CORRADE_ASSERT(easing,
        "Ui::BaseLayerStyleAnimator::create(): easing is null", );

    const UnsignedInt id = animationHandleId(handle);
    if(id >= state.animations.size()) {
        arrayResize(state.animations, NoInit, id + 1);
        state.expectedStyles = stridedArrayView(state.animations).slice(&Animation::expectedStyle);
        state.sourceStyles = stridedArrayView(state.animations).slice(&Animation::sourceStyle);
        state.targetStyles = stridedArrayView(state.animations).slice(&Animation::targetStyle);
        state.dynamicStyles = stridedArrayView(state.animations).slice(&Animation::dynamicStyle);
    }
    Animation& animation = state.animations[id];
    /* expectedStyle is filled by AbstractVisualLayerStyleAnimator::advance()
       on started[i], no point in setting it here */
    animation.sourceStyle = sourceStyle;
    animation.targetStyle = targetStyle;
    animation.dynamicStyle = ~UnsignedInt{};
    animation.easing = easing;
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

BaseLayerStyleAnimatorUpdates BaseLayerStyleAnimator::advance(const Nanoseconds time, const Containers::MutableBitArrayView active, const Containers::MutableBitArrayView started, const Containers::MutableBitArrayView stopped, const Containers::StridedArrayView1D<Float>& factors, const Containers::MutableBitArrayView remove, const Containers::ArrayView<BaseLayerStyleUniform> dynamicStyleUniforms, const Containers::StridedArrayView1D<Vector4>& dynamicStylePaddings, const Containers::StridedArrayView1D<UnsignedInt>& dataStyles) {
    /* The time...remove fields are checked inside update() right below, no
       need to repeat the check here again, especially since it's an internal
       API */

    const Containers::Pair<bool, bool> needsAdvanceClean = update(time,
        active,
        started,
        stopped,
        factors,
        remove);

    /* If there are any running animations, create() had to be called
       already, which ensures the layer is already set. Otherwise just bail as
       there's nothing to do. The view size assert isn't executed in that case
       but it's better that way than to not check against the dynamic style
       count at all. */
    State& state = static_cast<State&>(*_state);
    if(!state.layerSharedState) {
        CORRADE_INTERNAL_ASSERT(!capacity() && needsAdvanceClean == Containers::pair(false, false));
        return {};
    }

    BaseLayerStyleAnimatorUpdates updates;
    Containers::Triple<bool, bool, bool> updatesBase;
    if(needsAdvanceClean.first()) {
        const BaseLayer::Shared::State& layerSharedState = static_cast<const BaseLayer::Shared::State&>(*state.layerSharedState);
        CORRADE_ASSERT(
            dynamicStyleUniforms.size() == layerSharedState.dynamicStyleCount &&
            dynamicStylePaddings.size() == layerSharedState.dynamicStyleCount,
            "Ui::BaseLayerStyleAnimator::advance(): expected dynamic style uniform and padding views to have a size of" << layerSharedState.dynamicStyleCount << "but got" << dynamicStyleUniforms.size() << "and" << dynamicStylePaddings.size(), {});
        CORRADE_ASSERT(layerSharedState.setStyleCalled,
            "Ui::BaseLayerStyleAnimator::advance(): no style data was set on the layer", {});

        /* The base implementation deals with style switching and dynamic style
           allocation, which is common for all builtin style animators */
        updatesBase = AbstractVisualLayerStyleAnimator::advance(active, started, stopped, remove, dataStyles);
        if(updatesBase.first())
            updates |= BaseLayerStyleAnimatorUpdate::Style;
        if(updatesBase.second())
            updates |= BaseLayerStyleAnimatorUpdate::Uniform;

        /** @todo some way to iterate set bits */
        for(std::size_t i = 0; i != active.size(); ++i) {
            if(!active[i])
                continue;

            Animation& animation = state.animations[i];

            /* If the animation is started, fetch the style data. This is done
               here and not in create() to make it possible to reuse created
               animations even after a style is updated.

               Unlike below in the stopped case, there's no difference for
               Reverse animations -- for those, the factor will go from 1 to 0,
               causing the source and target to be swapped already. */
            if(started[i]) {
                const Implementation::BaseLayerStyle& sourceStyleData = layerSharedState.styles[animation.sourceStyle];
                const Implementation::BaseLayerStyle& targetStyleData = layerSharedState.styles[animation.targetStyle];
                animation.sourcePadding = sourceStyleData.padding;
                animation.targetPadding = targetStyleData.padding;

                /* Remember also if the actual uniform ID is different, if not,
                   we don't need to interpolate (or upload) it. The uniform
                   *data* may still be the same even if the ID is different,
                   but checking for that is too much work and any reasonable
                   style should deduplicate those anyway. */
                animation.sourceUniform = layerSharedState.styleUniforms[sourceStyleData.uniform];
                animation.targetUniform = layerSharedState.styleUniforms[targetStyleData.uniform];
                animation.uniformDifferent = sourceStyleData.uniform != targetStyleData.uniform;
            }

            /* If the animation is stopped or we have no dynamic style to
               interpolate to, continue to next animation. Everything else was
               done by the base advance() implementation called above. Branches
               kept separate to ensure they both stay tested. */
            if(stopped[i])
                continue;
            /** @todo expose options to (1) switch to the initial style, (2)
                switch to the target style and stop, or (3) don't do anything
                in case the dynamic style cannot be allocated */
            if(animation.dynamicStyle == ~UnsignedInt{})
                continue;

            const Float factor = animation.easing(factors[i]);

            /* Interpolate the uniform. If the source and target uniforms were
               the same, just copy one of them and don't report that the
               uniforms got changed. The only exception is the first ever
               switch to the dynamic uniform in which case the data has to be
               uploaded. That's handled in the animation.styleDynamic
               allocation above. */
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
                updates |= BaseLayerStyleAnimatorUpdate::Uniform;
            } else dynamicStyleUniforms[animation.dynamicStyle] = animation.targetUniform;

            /* Interpolate the padding. Compared to the uniforms, updated
               padding causes doUpdate() to be triggered on the layer, which is
               expensive, thus trigger it only if there's actually anything
               changing. */
            const Vector4 padding = Math::lerp(animation.sourcePadding,
                                               animation.targetPadding, factor);
            if(dynamicStylePaddings[animation.dynamicStyle] != padding) {
                dynamicStylePaddings[animation.dynamicStyle] = padding;
                updates |= BaseLayerStyleAnimatorUpdate::Padding;
            }
        }
    }

    /* Perform a clean either if the update() itself has stopped animations to
       remove, or if the base advance() additionally marked animations that no
       longer affect their data for removal */
    if(needsAdvanceClean.second() || updatesBase.third())
        clean(remove);

    return updates;
}

}}
