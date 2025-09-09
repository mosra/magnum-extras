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

#include "TextLayerAnimator.h"

#include <Corrade/Containers/ArrayTuple.h>
#include <Corrade/Containers/BitArrayView.h>
#include <Corrade/Containers/EnumSet.hpp>
#include <Corrade/Containers/GrowableArray.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Magnum/Math/Time.h>

#include "Magnum/Ui/TextLayer.h"
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/Implementation/abstractVisualLayerAnimatorState.h"
#include "Magnum/Ui/Implementation/textLayerState.h"

namespace Magnum { namespace Ui {

Debug& operator<<(Debug& debug, const TextLayerStyleAnimatorUpdate value) {
    debug << "Ui::TextLayerStyleAnimatorUpdate" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case TextLayerStyleAnimatorUpdate::value: return debug << "::" #value;
        _c(Uniform)
        _c(Padding)
        _c(EditingUniform)
        _c(EditingPadding)
        _c(Style)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "(" << Debug::nospace << Debug::hex << UnsignedByte(value) << Debug::nospace << ")";
}

Debug& operator<<(Debug& debug, const TextLayerStyleAnimatorUpdates value) {
    return Containers::enumSetDebugOutput(debug, value, "Ui::TextLayerStyleAnimatorUpdates{}", {
        TextLayerStyleAnimatorUpdate::Uniform,
        TextLayerStyleAnimatorUpdate::Padding,
        TextLayerStyleAnimatorUpdate::EditingUniform,
        TextLayerStyleAnimatorUpdate::EditingPadding,
        TextLayerStyleAnimatorUpdate::Style
    });
}

namespace {

struct Animation {
    /* As the Animation entries get recycled, all fields have to be overwritten
       always, thus there's no point in initializing them on the first ever
       construction either */

    TextLayerStyleUniform sourceUniform{NoInit}, targetUniform{NoInit};
    Vector4 sourcePadding{NoInit}, targetPadding{NoInit};

    TextLayerEditingStyleUniform sourceCursorUniform{NoInit},
        targetCursorUniform{NoInit};
    Vector4 sourceCursorPadding{NoInit},
        targetCursorPadding{NoInit};
    TextLayerEditingStyleUniform sourceSelectionUniform{NoInit},
        targetSelectionUniform{NoInit};
    Vector4 sourceSelectionPadding{NoInit},
        targetSelectionPadding{NoInit};
    TextLayerStyleUniform sourceSelectionTextUniform{NoInit},
        targetSelectionTextUniform{NoInit};

    /* Font, alignment and features are all taken from the source style and
       don't animate. Compared to the uniforms and paddings, which are copied
       above to avoid redoing the extra logic and uniform mapping indirections
       for all animations in every advance(), they're only used once at the
       point where dynamic style is allocated, and referencing them in the
       original style via `styleSrc` is more efficient than having to deal with
       variable-length allocation for a copy of the feature list. */

    UnsignedInt sourceStyle, targetStyle, dynamicStyle;

    /** @todo pack the booleans to a single flag */
    bool hasCursorStyle,
        hasSelectionStyle;
    bool uniformDifferent,
        cursorUniformDifferent,
        selectionUniformDifferent,
        selectionTextUniformDifferent;
    /* 6/2 bytes free */

    Float(*easing)(Float);
};

}

struct TextLayerStyleAnimator::State: AbstractVisualLayerStyleAnimator::State {
    Containers::Array<Animation> animations;
};

TextLayerStyleAnimator::TextLayerStyleAnimator(AnimatorHandle handle): AbstractVisualLayerStyleAnimator{handle, Containers::pointer<State>()} {}

TextLayerStyleAnimator::TextLayerStyleAnimator(TextLayerStyleAnimator&&) noexcept = default;

TextLayerStyleAnimator::~TextLayerStyleAnimator() = default;

TextLayerStyleAnimator& TextLayerStyleAnimator::operator=(TextLayerStyleAnimator&&) noexcept = default;

AnimationHandle TextLayerStyleAnimator::create(const UnsignedInt sourceStyle, const UnsignedInt targetStyle, Float(*const easing)(Float), const Nanoseconds start, const Nanoseconds duration, const DataHandle data, const UnsignedInt repeatCount, const AnimationFlags flags) {
    /* AbstractAnimator::create() DataHandle overload checks the layer
       internally too, but this message is less confusing */
    CORRADE_ASSERT(_state->layer,
        "Ui::TextLayerStyleAnimator::create(): no layer set", {});
    const AnimationHandle handle = AbstractStyleAnimator::create(start, duration, data, repeatCount, flags);
    createInternal(handle, sourceStyle, targetStyle, easing);
    return handle;
}

AnimationHandle TextLayerStyleAnimator::create(const UnsignedInt sourceStyle, const UnsignedInt targetStyle, Float(*const easing)(Float), const Nanoseconds start, const Nanoseconds duration, const DataHandle data, const AnimationFlags flags) {
    return create(sourceStyle, targetStyle, easing, start, duration, data, 1, flags);
}

AnimationHandle TextLayerStyleAnimator::create(const UnsignedInt sourceStyle, const UnsignedInt targetStyle, Float(*const easing)(Float), const Nanoseconds start, const Nanoseconds duration, const LayerDataHandle data, const UnsignedInt repeatCount, const AnimationFlags flags) {
    /* AbstractAnimator::create() DataHandle overload checks the layer
       internally too, but this message is less confusing */
    CORRADE_ASSERT(_state->layer,
        "Ui::TextLayerStyleAnimator::create(): no layer set", {});
    const AnimationHandle handle = AbstractStyleAnimator::create(start, duration, data, repeatCount, flags);
    createInternal(handle, sourceStyle, targetStyle, easing);
    return handle;
}

AnimationHandle TextLayerStyleAnimator::create(const UnsignedInt sourceStyle, const UnsignedInt targetStyle, Float(*const easing)(Float), const Nanoseconds start, const Nanoseconds duration, const LayerDataHandle data, const AnimationFlags flags) {
    return create(sourceStyle, targetStyle, easing, start, duration, data, 1, flags);
}

void TextLayerStyleAnimator::createInternal(const AnimationHandle handle, const UnsignedInt sourceStyle, const UnsignedInt targetStyle, Float(*const easing)(Float)) {
    State& state = static_cast<State&>(*_state);
    /* Layer being set had to be checked in create() already */
    CORRADE_INTERNAL_ASSERT(state.layerSharedState);
    const TextLayer::Shared::State& layerSharedState = static_cast<const TextLayer::Shared::State&>(*state.layerSharedState);
    CORRADE_ASSERT(layerSharedState.setStyleCalled,
        "Ui::TextLayerStyleAnimator::create(): no style data was set on the layer", );
    /* Like in TextLayer::doUpdate(), technically needed only if there's any
       actual editable style to animate, but require it always for consistency */
    CORRADE_ASSERT(!layerSharedState.hasEditingStyles || layerSharedState.setEditingStyleCalled,
        "Ui::TextLayerStyleAnimator::create(): no editing style data was set on the layer", );
    CORRADE_ASSERT(
        sourceStyle < layerSharedState.styleCount &&
        targetStyle < layerSharedState.styleCount,
        "Ui::TextLayerStyleAnimator::create(): expected source and target style to be in range for" << layerSharedState.styleCount << "styles but got" << sourceStyle << "and" << targetStyle, );
    CORRADE_ASSERT(easing,
        "Ui::TextLayerStyleAnimator::create(): easing is null", );

    const UnsignedInt id = animationHandleId(handle);
    if(id >= state.animations.size()) {
        arrayResize(state.animations, NoInit, id + 1);
        state.sourceStyles = stridedArrayView(state.animations).slice(&Animation::sourceStyle);
        state.targetStyles = stridedArrayView(state.animations).slice(&Animation::targetStyle);
        state.dynamicStyles = stridedArrayView(state.animations).slice(&Animation::dynamicStyle);
    }
    Animation& animation = state.animations[id];
    animation.sourceStyle = sourceStyle;
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

    /* Animate also cursor style, if present */
    if(sourceStyleData.cursorStyle != -1 || targetStyleData.cursorStyle != -1) {
        CORRADE_ASSERT(sourceStyleData.cursorStyle != -1 && targetStyleData.cursorStyle != -1,
            "Ui::TextLayerStyleAnimator::create(): expected style" << targetStyle << (targetStyleData.cursorStyle == -1 ? "to" : "to not") << "reference a cursor style like style" << sourceStyle, );

        const Implementation::TextLayerEditingStyle& sourceEditingStyleData = layerSharedState.editingStyles[sourceStyleData.cursorStyle];
        const Implementation::TextLayerEditingStyle& targetEditingStyleData = layerSharedState.editingStyles[targetStyleData.cursorStyle];
        animation.sourceCursorPadding = sourceEditingStyleData.padding;
        animation.targetCursorPadding = targetEditingStyleData.padding;

        /* Like with the base, remember if the actual uniform ID is different
           to skip the interpolation */
        animation.sourceCursorUniform = layerSharedState.editingStyleUniforms[sourceEditingStyleData.uniform];
        animation.targetCursorUniform = layerSharedState.editingStyleUniforms[targetEditingStyleData.uniform];
        animation.cursorUniformDifferent = sourceEditingStyleData.uniform != targetEditingStyleData.uniform;

        animation.hasCursorStyle = true;
    } else animation.hasCursorStyle = false;

    /* Animate also selection style, if present */
    if(sourceStyleData.selectionStyle != -1 || targetStyleData.selectionStyle != -1) {
        CORRADE_ASSERT(sourceStyleData.selectionStyle != -1 && targetStyleData.selectionStyle != -1,
            "Ui::TextLayerStyleAnimator::create(): expected style" << targetStyle << (targetStyleData.selectionStyle == -1 ? "to" : "to not") << "reference a selection style like style" << sourceStyle, );

        const Implementation::TextLayerEditingStyle& sourceEditingStyleData = layerSharedState.editingStyles[sourceStyleData.selectionStyle];
        const Implementation::TextLayerEditingStyle& targetEditingStyleData = layerSharedState.editingStyles[targetStyleData.selectionStyle];
        animation.sourceSelectionPadding = sourceEditingStyleData.padding;
        animation.targetSelectionPadding = targetEditingStyleData.padding;

        /* Like with the base, remember if the actual uniform ID is different
           to skip the interpolation. OR that with the difference from the
           cursor, as both lead to upload of the same uniform buffer. */
        animation.sourceSelectionUniform = layerSharedState.editingStyleUniforms[sourceEditingStyleData.uniform];
        animation.targetSelectionUniform = layerSharedState.editingStyleUniforms[targetEditingStyleData.uniform];
        animation.selectionUniformDifferent = sourceEditingStyleData.uniform != targetEditingStyleData.uniform;

        /* Finally, if the selection style references an override for the text
           uniform, save that too, and again remember if it's different, ORing
           with the base style uniform difference. */
        const UnsignedInt sourceTextUniform = sourceEditingStyleData.textUniform != -1 ? sourceEditingStyleData.textUniform : sourceStyleData.uniform;
        const UnsignedInt targetTextUniform = targetEditingStyleData.textUniform != -1 ? targetEditingStyleData.textUniform : targetStyleData.uniform;
        animation.sourceSelectionTextUniform = layerSharedState.styleUniforms[sourceTextUniform];
        animation.targetSelectionTextUniform = layerSharedState.styleUniforms[targetTextUniform];
        animation.selectionTextUniformDifferent = sourceTextUniform != targetTextUniform;

        animation.hasSelectionStyle = true;
    } else animation.hasSelectionStyle = false;
}

void TextLayerStyleAnimator::remove(const AnimationHandle handle) {
    AbstractAnimator::remove(handle);
    removeInternal(animationHandleId(handle));
}

void TextLayerStyleAnimator::remove(const AnimatorDataHandle handle) {
    AbstractAnimator::remove(handle);
    removeInternal(animatorDataHandleId(handle));
}

Containers::Pair<TextLayerStyleUniform, TextLayerStyleUniform> TextLayerStyleAnimator::uniforms(const AnimationHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::TextLayerStyleAnimator::uniforms(): invalid handle" << handle, {});
    const Animation& animation = static_cast<const State&>(*_state).animations[animationHandleId(handle)];
    return {animation.sourceUniform, animation.targetUniform};
}

Containers::Pair<TextLayerStyleUniform, TextLayerStyleUniform> TextLayerStyleAnimator::uniforms(const AnimatorDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::TextLayerStyleAnimator::uniforms(): invalid handle" << handle, {});
    const Animation& animation = static_cast<const State&>(*_state).animations[animatorDataHandleId(handle)];
    return {animation.sourceUniform, animation.targetUniform};
}

Containers::Pair<Vector4, Vector4> TextLayerStyleAnimator::paddings(const AnimationHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::TextLayerStyleAnimator::paddings(): invalid handle" << handle, {});
    const Animation& animation = static_cast<const State&>(*_state).animations[animationHandleId(handle)];
    return {animation.sourcePadding, animation.targetPadding};
}

Containers::Pair<Vector4, Vector4> TextLayerStyleAnimator::paddings(const AnimatorDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::TextLayerStyleAnimator::paddings(): invalid handle" << handle, {});
    const Animation& animation = static_cast<const State&>(*_state).animations[animatorDataHandleId(handle)];
    return {animation.sourcePadding, animation.targetPadding};
}

Containers::Optional<Containers::Pair<TextLayerEditingStyleUniform, TextLayerEditingStyleUniform>> TextLayerStyleAnimator::cursorUniforms(const AnimationHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::TextLayerStyleAnimator::cursorUniforms(): invalid handle" << handle, {});
    return cursorUniformsInternal(animationHandleId(handle));
}

Containers::Optional<Containers::Pair<TextLayerEditingStyleUniform, TextLayerEditingStyleUniform>> TextLayerStyleAnimator::cursorUniforms(const AnimatorDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::TextLayerStyleAnimator::cursorUniforms(): invalid handle" << handle, {});
    return cursorUniformsInternal(animatorDataHandleId(handle));
}

Containers::Optional<Containers::Pair<TextLayerEditingStyleUniform, TextLayerEditingStyleUniform>> TextLayerStyleAnimator::cursorUniformsInternal(const UnsignedInt id) const {
    const Animation& animation = static_cast<const State&>(*_state).animations[id];
    if(!animation.hasCursorStyle)
        return {};
    return Containers::pair(animation.sourceCursorUniform, animation.targetCursorUniform);
}

Containers::Optional<Containers::Pair<Vector4, Vector4>> TextLayerStyleAnimator::cursorPaddings(const AnimationHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::TextLayerStyleAnimator::cursorPaddings(): invalid handle" << handle, {});
    return cursorPaddingsInternal(animationHandleId(handle));
}

Containers::Optional<Containers::Pair<Vector4, Vector4>> TextLayerStyleAnimator::cursorPaddings(const AnimatorDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::TextLayerStyleAnimator::cursorPaddings(): invalid handle" << handle, {});
    return cursorPaddingsInternal(animatorDataHandleId(handle));
}

Containers::Optional<Containers::Pair<Vector4, Vector4>> TextLayerStyleAnimator::cursorPaddingsInternal(const UnsignedInt id) const {
    const Animation& animation = static_cast<const State&>(*_state).animations[id];
    if(!animation.hasCursorStyle)
        return {};
    return Containers::pair(animation.sourceCursorPadding, animation.targetCursorPadding);
}

Containers::Optional<Containers::Pair<TextLayerEditingStyleUniform, TextLayerEditingStyleUniform>> TextLayerStyleAnimator::selectionUniforms(const AnimationHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::TextLayerStyleAnimator::selectionUniforms(): invalid handle" << handle, {});
    return selectionUniformsInternal(animationHandleId(handle));
}

Containers::Optional<Containers::Pair<TextLayerEditingStyleUniform, TextLayerEditingStyleUniform>> TextLayerStyleAnimator::selectionUniforms(const AnimatorDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::TextLayerStyleAnimator::selectionUniforms(): invalid handle" << handle, {});
    return selectionUniformsInternal(animatorDataHandleId(handle));
}

Containers::Optional<Containers::Pair<TextLayerEditingStyleUniform, TextLayerEditingStyleUniform>> TextLayerStyleAnimator::selectionUniformsInternal(const UnsignedInt id) const {
    const Animation& animation = static_cast<const State&>(*_state).animations[id];
    if(!animation.hasSelectionStyle)
        return {};
    return Containers::pair(animation.sourceSelectionUniform, animation.targetSelectionUniform);
}

Containers::Optional<Containers::Pair<Vector4, Vector4>> TextLayerStyleAnimator::selectionPaddings(const AnimationHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::TextLayerStyleAnimator::selectionPaddings(): invalid handle" << handle, {});
    return selectionPaddingsInternal(animationHandleId(handle));
}

Containers::Optional<Containers::Pair<Vector4, Vector4>> TextLayerStyleAnimator::selectionPaddings(const AnimatorDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::TextLayerStyleAnimator::selectionPaddings(): invalid handle" << handle, {});
    return selectionPaddingsInternal(animatorDataHandleId(handle));
}

Containers::Optional<Containers::Pair<Vector4, Vector4>> TextLayerStyleAnimator::selectionPaddingsInternal(const UnsignedInt id) const {
    const Animation& animation = static_cast<const State&>(*_state).animations[id];
    if(!animation.hasSelectionStyle)
        return {};
    return Containers::pair(animation.sourceSelectionPadding, animation.targetSelectionPadding);
}

Containers::Optional<Containers::Pair<TextLayerStyleUniform, TextLayerStyleUniform>> TextLayerStyleAnimator::selectionTextUniforms(const AnimationHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::TextLayerStyleAnimator::selectionTextUniforms(): invalid handle" << handle, {});
    return selectionTextUniformsInternal(animationHandleId(handle));
}

Containers::Optional<Containers::Pair<TextLayerStyleUniform, TextLayerStyleUniform>> TextLayerStyleAnimator::selectionTextUniforms(const AnimatorDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::TextLayerStyleAnimator::selectionTextUniforms(): invalid handle" << handle, {});
    return selectionTextUniformsInternal(animatorDataHandleId(handle));
}

Containers::Optional<Containers::Pair<TextLayerStyleUniform, TextLayerStyleUniform>> TextLayerStyleAnimator::selectionTextUniformsInternal(const UnsignedInt id) const {
    const Animation& animation = static_cast<const State&>(*_state).animations[id];
    if(!animation.hasSelectionStyle)
        return {};
    return Containers::pair(animation.sourceSelectionTextUniform, animation.targetSelectionTextUniform);
}

auto TextLayerStyleAnimator::easing(const AnimationHandle handle) const -> Float(*)(Float) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::TextLayerStyleAnimator::easing(): invalid handle" << handle, {});
    return static_cast<const State&>(*_state).animations[animationHandleId(handle)].easing;
}

auto TextLayerStyleAnimator::easing(const AnimatorDataHandle handle) const -> Float(*)(Float) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::TextLayerStyleAnimator::easing(): invalid handle" << handle, {});
    return static_cast<const State&>(*_state).animations[animatorDataHandleId(handle)].easing;
}

namespace {

/* Used for both base and editing text uniforms and for both cursor and
   selection uniforms, extracted here. I feel like this is better than a lambda
   because it doesn't need any capture. */
TextLayerStyleUniform interpolateUniform(const TextLayerStyleUniform& source, const TextLayerStyleUniform& target, Float factor) {
    TextLayerStyleUniform uniform{NoInit};
    #define _c(member) uniform.member = Math::lerp(source.member, target.member, factor);
    _c(color)
    #undef _c
    return uniform;
}
TextLayerEditingStyleUniform interpolateUniform(const TextLayerEditingStyleUniform& source, const TextLayerEditingStyleUniform& target, Float factor) {
    TextLayerEditingStyleUniform uniform{NoInit};
    #define _c(member) uniform.member = Math::lerp(source.member, target.member, factor);
    _c(backgroundColor)
    _c(cornerRadius)
    #undef _c
    return uniform;
}

}

TextLayerStyleAnimatorUpdates TextLayerStyleAnimator::advance(const Containers::BitArrayView active, const Containers::BitArrayView stopped, const Containers::StridedArrayView1D<const Float>& factors, const Containers::ArrayView<TextLayerStyleUniform> dynamicStyleUniforms, const Containers::MutableBitArrayView dynamicStyleCursorStyles, const Containers::MutableBitArrayView dynamicStyleSelectionStyles, const Containers::StridedArrayView1D<Vector4>& dynamicStylePaddings, const Containers::ArrayView<TextLayerEditingStyleUniform> dynamicEditingStyleUniforms, const Containers::StridedArrayView1D<Vector4>& dynamicEditingStylePaddings, const Containers::StridedArrayView1D<UnsignedInt>& dataStyles) {
    CORRADE_ASSERT(active.size() == capacity() &&
                   stopped.size() == capacity() &&
                   factors.size() == capacity(),
        "Ui::TextLayerStyleAnimator::advance(): expected active, stopped and factors views to have a size of" << capacity() << "but got" << active.size() << Debug::nospace << "," << stopped.size() << "and" << factors.size(), {});

    /* If there are any running animations, create() had to be called
       already, which ensures the layer is already set. Otherwise just bail as
       there's nothing to do. The view size assert isn't executed in that case
       but it's better that way than to not check against the dynamic style
       count at all. */
    State& state = static_cast<State&>(*_state);
    if(!state.layerSharedState) {
        CORRADE_INTERNAL_ASSERT(!capacity());
        return {};
    }

    const TextLayer::Shared::State& layerSharedState = static_cast<const TextLayer::Shared::State&>(*state.layerSharedState);
    #ifndef CORRADE_NO_ASSERT
    /* If there are no editing styles, the base style views are all required to
       have the same size, and the editing views empty */
    if(!layerSharedState.hasEditingStyles) {
        CORRADE_ASSERT(
            dynamicStyleUniforms.size() == layerSharedState.dynamicStyleCount &&
            dynamicStyleCursorStyles.size() == layerSharedState.dynamicStyleCount &&
            dynamicStyleSelectionStyles.size() == layerSharedState.dynamicStyleCount &&
            dynamicStylePaddings.size() == layerSharedState.dynamicStyleCount &&
            dynamicEditingStyleUniforms.isEmpty() &&
            dynamicEditingStylePaddings.isEmpty(),
            "Ui::TextLayerStyleAnimator::advance(): expected dynamic style uniform, cursor style, selection style and padding views to have a size of" << layerSharedState.dynamicStyleCount << Debug::nospace << ", and the dynamic editing style uniform and paddings empty, but got" << dynamicStyleUniforms.size() << Debug::nospace << "," << dynamicStyleCursorStyles.size() << Debug::nospace << "," << dynamicStyleSelectionStyles.size() << Debug::nospace << "," << dynamicStylePaddings.size() << Debug::nospace << ";" << dynamicEditingStyleUniforms.size() << "and" << dynamicEditingStylePaddings.size(), {});
    } else {
        CORRADE_ASSERT(
            dynamicStyleCursorStyles.size() == layerSharedState.dynamicStyleCount &&
            dynamicStyleSelectionStyles.size() == layerSharedState.dynamicStyleCount &&
            dynamicStylePaddings.size() == layerSharedState.dynamicStyleCount &&
            dynamicStyleUniforms.size() == layerSharedState.dynamicStyleCount*3 &&
            dynamicEditingStyleUniforms.size() == layerSharedState.dynamicStyleCount*2 &&
            dynamicEditingStylePaddings.size() == layerSharedState.dynamicStyleCount*2,
            "Ui::TextLayerStyleAnimator::advance(): expected dynamic style cursor style, selection style and padding views to have a size of" << layerSharedState.dynamicStyleCount << Debug::nospace << ", the dynamic style uniform view a size of" << layerSharedState.dynamicStyleCount*3 << Debug::nospace << ", and the dynamic editing style uniform and padding views a size of" << layerSharedState.dynamicStyleCount*2 << Debug::nospace << ", but got" << dynamicStyleCursorStyles.size() << Debug::nospace << "," << dynamicStyleSelectionStyles.size() << Debug::nospace << "," << dynamicStylePaddings.size() << Debug::nospace << ";" << dynamicStyleUniforms.size() << Debug::nospace << ";" << dynamicEditingStyleUniforms.size() << "and" << dynamicEditingStylePaddings.size(), {});
    }
    #endif

    const Containers::StridedArrayView1D<const LayerDataHandle> layerData = this->layerData();

    TextLayerStyleAnimatorUpdates updates;
    /** @todo some way to iterate set bits */
    for(std::size_t i = 0; i != active.size(); ++i) {
        if(!active[i])
            continue;

        Animation& animation = state.animations[i];
        /* The handle is assumed to be valid if not null, i.e. that appropriate
           dataClean() got called before advance() */
        const LayerDataHandle data = layerData[i];

        /* If the animation is stopped, switch the data to the target style, if
           any. No need to animate anything else as the dynamic style is going
           to get recycled right away. */
        if(stopped[i]) {
            CORRADE_INTERNAL_ASSERT(factors[i] == 1.0f);
            if(data != LayerDataHandle::Null) {
                dataStyles[layerDataHandleId(data)] = animation.targetStyle;
                updates |= TextLayerStyleAnimatorUpdate::Style;
            }

            /* Recycle the dynamic style if it was allocated already. It might
               not be if advance() wasn't called for this animation yet or if
               it was already stopped by the time it reached advance(). */
            if(animation.dynamicStyle != ~UnsignedInt{}) {
                state.layer->recycleDynamicStyle(animation.dynamicStyle);
                animation.dynamicStyle = ~UnsignedInt{};
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

            /* Initialize the dynamic style font, alignment and features from
               the source style. Those can't reasonably get animated in any
               way, but the dynamic style has to contain them so calls to
               setText(), updateText() and editText() while the style is being
               animated don't behave differently. The uniform and padding is
               left at the default-constructed state as it's filled through the
               `dynamicStyleUniforms` and `dynamicStylePaddings` views right
               after. */
            {
                const Implementation::TextLayerStyle& styleData = layerSharedState.styles[animation.sourceStyle];
                static_cast<TextLayer&>(*state.layer).setDynamicStyle(*style,
                    TextLayerStyleUniform{},
                    styleData.font,
                    styleData.alignment, layerSharedState.styleFeatures.sliceSize(styleData.featureOffset, styleData.featureCount),
                    {});
            }

            animation.dynamicStyle = *style;

            if(data != LayerDataHandle::Null) {
                dataStyles[layerDataHandleId(data)] = layerSharedState.styleCount + animation.dynamicStyle;
                updates |= TextLayerStyleAnimatorUpdate::Style;
                /* If the uniform IDs are the same between the source and
                   target style, the uniform interpolation below won't happen.
                   We still need to upload it at least once though, so trigger
                   it here unconditionally. */
                updates |= TextLayerStyleAnimatorUpdate::Uniform;
                /* Same for the editing uniform buffer, if there's an editing
                   style */
                if(animation.hasCursorStyle || animation.hasSelectionStyle)
                    updates |= TextLayerStyleAnimatorUpdate::EditingUniform;
            }

            /* If the animation is attached to some data, the above already
               triggers a Style update, which results in appropriate editing
               quads being made. If the animation isn't attached to any data,
               there's nothing to be done based on those so there's no reason
               to set any TextLayerStyleAnimatorUpdate. */
            dynamicStyleCursorStyles.set(animation.dynamicStyle, animation.hasCursorStyle);
            dynamicStyleSelectionStyles.set(animation.dynamicStyle, animation.hasSelectionStyle);
        }

        const Float factor = animation.easing(factors[i]);

        /* Interpolate the uniform. If the source and target uniforms were the
           same, just copy one of them and don't report that the uniforms got
           changed. The only exception is the first ever switch to the dynamic
           uniform in which case the data has to be uploaded. That's handled in
           the animation.styleDynamic allocation above. */
        if(animation.uniformDifferent) {
            dynamicStyleUniforms[animation.dynamicStyle] = interpolateUniform(animation.sourceUniform, animation.targetUniform, factor);
            updates |= TextLayerStyleAnimatorUpdate::Uniform;
        } else dynamicStyleUniforms[animation.dynamicStyle] = animation.targetUniform;

        /* Interpolate the padding. Compared to the uniforms, updated padding
           causes doUpdate() to be triggered on the layer, which is expensive,
           thus trigger it only if there's actually anything changing. */
        const Vector4 padding = Math::lerp(animation.sourcePadding,
                                           animation.targetPadding, factor);
        if(dynamicStylePaddings[animation.dynamicStyle] != padding) {
            dynamicStylePaddings[animation.dynamicStyle] = padding;
            updates |= TextLayerStyleAnimatorUpdate::Padding;
        }

        /* If there's a cursor, interpolate it as well. Logic same as above. */
        if(animation.hasCursorStyle) {
            const UnsignedInt editingStyleId = Implementation::cursorStyleForDynamicStyle(animation.dynamicStyle);

            if(animation.cursorUniformDifferent) {
                dynamicEditingStyleUniforms[editingStyleId] = interpolateUniform(animation.sourceCursorUniform, animation.targetCursorUniform, factor);
                updates |= TextLayerStyleAnimatorUpdate::EditingUniform;
            } else dynamicEditingStyleUniforms[editingStyleId] = animation.targetCursorUniform;

            const Vector4 cursorPadding = Math::lerp(
                animation.sourceCursorPadding,
                animation.targetCursorPadding, factor);
            if(dynamicEditingStylePaddings[editingStyleId] != cursorPadding) {
                dynamicEditingStylePaddings[editingStyleId] = cursorPadding;
                updates |= TextLayerStyleAnimatorUpdate::EditingPadding;
            }
        }

        /* If there's a selection, interpolate it as well. Logic same as
           above. */
        if(animation.hasSelectionStyle) {
            const UnsignedInt editingStyleId = Implementation::selectionStyleForDynamicStyle(animation.dynamicStyle);

            if(animation.selectionUniformDifferent) {
                dynamicEditingStyleUniforms[editingStyleId] = interpolateUniform(animation.sourceSelectionUniform, animation.targetSelectionUniform, factor);
                updates |= TextLayerStyleAnimatorUpdate::EditingUniform;
            } else dynamicEditingStyleUniforms[editingStyleId] = animation.targetSelectionUniform;

            const Vector4 selectionPadding = Math::lerp(
                animation.sourceSelectionPadding,
                animation.targetSelectionPadding, factor);
            if(dynamicEditingStylePaddings[editingStyleId] != selectionPadding) {
                dynamicEditingStylePaddings[editingStyleId] = selectionPadding;
                updates |= TextLayerStyleAnimatorUpdate::EditingPadding;
            }

            const UnsignedInt textStyleId = Implementation::selectionStyleTextUniformForDynamicStyle(layerSharedState.dynamicStyleCount, animation.dynamicStyle);
            if(animation.selectionTextUniformDifferent) {
                dynamicStyleUniforms[textStyleId] = interpolateUniform(animation.sourceSelectionTextUniform, animation.targetSelectionTextUniform, factor);
                updates |= TextLayerStyleAnimatorUpdate::Uniform;
            } else dynamicStyleUniforms[textStyleId] = animation.targetSelectionTextUniform;
        }
    }

    return updates;
}

}}
