#ifndef Magnum_Whee_Implementation_baseLayerState_h
#define Magnum_Whee_Implementation_baseLayerState_h
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

/* Definition of the BaseLayer::State struct to be used by both BaseLayer and
   BaseLayerGL as well as BaseLayer tests, and (if this header gets published)
   eventually possibly also 3rd party renderer implementations */

#include <Corrade/Containers/Array.h>

#include "Magnum/Whee/BaseLayer.h"
#include "Magnum/Whee/Implementation/abstractVisualLayerState.h"

namespace Magnum { namespace Whee {

namespace Implementation {

struct BaseLayerStyle {
    /* Uniform index corresponding to given style */
    UnsignedInt uniform;

    Vector4 padding;
};

}

struct BaseLayer::Shared::State: AbstractVisualLayer::Shared::State {
    explicit State(Shared& self, const Configuration& configuration): AbstractVisualLayer::Shared::State{self, configuration.styleCount(), configuration.dynamicStyleCount()}, styleUniformCount{configuration.styleUniformCount()}, flags{configuration.flags()} {}

    UnsignedInt styleUniformCount;
    Flags flags;

    /* 1 byte free */

    /* Incremented every time the styleUniforms array is changed. There's a
       corresponding BaseLayer::State::styleUniformUpdateStamp variable that
       doDraw() compares to this one, triggering style uniform buffer update if
       it differs. */
    UnsignedShort styleUniformUpdateStamp = 0;

    /* Uniform mapping and padding values assigned to each style. Initially
       empty to be able to detect whether setStyle() was called. */
    Containers::Array<Implementation::BaseLayerStyle> styles;
    /* Uniform values to be copied to layer-specific uniform buffers. Initially
       empty to be able to detect whether setStyle() was called, stays empty
       and unused if dynamicStyleCount is 0. */
    /** @todo the allocation could be shared with above */
    Containers::Array<BaseLayerStyleUniform> styleUniforms;
    BaseLayerCommonStyleUniform commonStyleUniform{NoInit};
};

namespace Implementation {

struct BaseLayerData {
    Vector4 padding;
    Vector4 outlineWidth;
    Color3 color;
    /* calculatedStyle is filled by AbstractVisualLayer::doUpdate() */
    UnsignedInt style, calculatedStyle;
    Vector3 textureCoordinateOffset;
    Vector2 textureCoordinateSize;
};

struct BaseLayerVertex {
    Vector2 position;
    Vector2 centerDistance;
    Vector4 outlineWidth;
    Color3 color;
    UnsignedInt styleUniform;
};

struct BaseLayerTexturedVertex: BaseLayerVertex {
    Vector3 textureCoordinates;
};

}

struct BaseLayer::State: AbstractVisualLayer::State {
    explicit State(Shared::State& shared): AbstractVisualLayer::State{shared}, dynamicStyleUniforms{ValueInit, shared.dynamicStyleCount}, dynamicStylePaddings{ValueInit, shared.dynamicStyleCount} {}

    Containers::Array<Implementation::BaseLayerData> data;
    /* Only one of these is used at a time */
    Containers::Array<Implementation::BaseLayerVertex> vertices;
    Containers::Array<Implementation::BaseLayerTexturedVertex> texturedVertices;
    Containers::Array<UnsignedInt> indices;

    /* Used only if Flag::BackgroundBlur is enabled */
    UnsignedInt backgroundBlurPassCount = 1;
    /* Used only if shared.dynamicStyleCount is non-zero, is compared to
       Shared::styleUniformUpdateStamp in order to detect that the uniform
       buffer needs to be updated and then set to its value. When the two are
       the same, it's assumed the buffer is up-to-date. As setStyle() is forced
       to be called at least once, Shared::styleUniformUpdateStamp is initially
       1 so the first update gets triggered always.

       The only case where a style update may get skipped by accident is if the
       shared state gets 65536 style updates, wrapping back to 0, and a
       completely new layer gets created and drawn right at that point. Which I
       think is rather unlikely, but if it wouldn't the stamps could be
       expanded to 32 bits. */
    UnsignedShort styleUniformUpdateStamp = 0;
    /* True initially to trigger upload of the default-constructed dynamic
       style even if it haven't been set to anything */
    bool dynamicStyleChanged = true;

    /* 1 byte free */

    /* Used only if shared.dynamicStyleCount is non-zero */
    /** @todo the allocations could be shared */
    Containers::Array<BaseLayerStyleUniform> dynamicStyleUniforms;
    Containers::Array<Vector4> dynamicStylePaddings;
};

}}

#endif
