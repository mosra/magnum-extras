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
    explicit State(Shared& self, const Configuration& configuration);

    /* First 2/6 bytes overlap with padding of the base struct */

    /* Incremented every time setStyle() is called. There's a corresponding
       styleUpdateStamp variable in BaseLayer::State that doState() compares to
       this one, returning LayerState::NeedsDataUpdate if it differs. */
    UnsignedShort styleUpdateStamp = 0;

    /* Used by BaseLayerGL to expand the area used for processing the blur
       so the second and subsequent passes don't tap outside. The radius is
       always at most 31, so can be a byte. */
    UnsignedByte backgroundBlurRadius;

    BaseLayerSharedFlags flags;

    #ifndef CORRADE_NO_ASSERT
    bool setStyleCalled = false;
    #endif
    /* 1 byte free, 2 bytes free w/ CORRADE_NO_ASSERT */

    /* Can't be inferred from styleUniforms.size() as those are non-empty only
       if dynamicStyleCount is non-zero */
    UnsignedInt styleUniformCount;

    /* Used to expand quad area for smoothed-out edges. The same value is
       commonStyleUniform if dynamicStyleCount is non-zero, but saving it to a
       dedicated place to avoid unnecessarily tangled data dependencies. */
    Float smoothness;

    Containers::ArrayTuple styleStorage;
    /* Uniform mapping and padding values assigned to each style */
    Containers::ArrayView<Implementation::BaseLayerStyle> styles;
    /* Uniform values to be copied to layer-specific uniform buffers. Empty
       and unused if dynamicStyleCount is 0. */
    Containers::ArrayView<BaseLayerStyleUniform> styleUniforms;
    BaseLayerCommonStyleUniform commonStyleUniform{NoInit};
};

namespace Implementation {

struct BaseLayerData {
    Vector4 padding;
    Vector4 outlineWidth; /* left, top, right, bottom */
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

struct BaseLayerTexturedVertex {
    /* Has to be a member and not a base class because then is_standard_layout
       complains, making arrayCast() impossible. Sigh. */
    BaseLayerVertex vertex;
    Vector3 textureCoordinates;
};

struct BaseLayerSubdividedVertex {
    Vector2 position;
    Vector2 outlineWidth;
    Color3 color;
    UnsignedInt styleUniform;
    /* Used for interpolating/extrapolating the vertical gradient when
       expanding the quads */
    Float centerDistanceY;
};

struct BaseLayerSubdividedTexturedVertex {
    BaseLayerSubdividedVertex vertex;
    /* Used for interpolating/extrapolating the texture coordinates when
       expanding the quads. Put into a single vertex attribute with
       centerDistanceY in BaseLayerGL, thus expected to be right after. */
    Vector2 textureScale;
    Vector3 textureCoordinates;
};

static_assert(
    offsetof(BaseLayerSubdividedTexturedVertex, vertex) == 0 &&
    offsetof(BaseLayerSubdividedTexturedVertex, textureScale) == offsetof(BaseLayerSubdividedVertex, centerDistanceY) + sizeof(BaseLayerSubdividedVertex::centerDistanceY),
    "expected textureScale to immediately follow centerDistanceY");

}

struct BaseLayer::State: AbstractVisualLayer::State {
    explicit State(Shared::State& shared);

    /* First 2/6 bytes overlap with padding of the base struct */

    /* Is compared to Shared::styleUpdateStamp in order to detect that
       doUpdate() needs to be called to update to potentially new mappings
       between styles and uniform IDs, paddings etc. When the two are the same,
       it's assumed all style-dependent data are up-to-date.

       Gets set to the shared value on construction to not implicitly mark a
       fresh layer with no data as immediately needing an update.

       See AbstractVisualLayer::State::styleTransitionToDisabledUpdateStamp for
       discussion about when an update may get skipped by accident. */
    UnsignedShort styleUpdateStamp;
    /* Used to distinguish between needing an update of the shared part of the
       style (which is triggered by differing styleUpdateStamp) and the dynamic
       part */
    bool dynamicStyleChanged = false;

    /* 3 bytes free */

    Containers::Array<Implementation::BaseLayerData> data;
    /* Is either Implementation::BaseLayerVertex or BaseLayerTexturedVertex
       based on whether texturing is enabled */
    Containers::Array<char> vertices;
    Containers::Array<UnsignedInt> indices;

    /* Used for scaling the smoothness expansion to actual pixels, for clipping
       rects in BaseLayerGL and for expanding compositing rects for blur radius
       if BackgroundBlur is enabled */
    Vector2 uiSize;
    Vector2i framebufferSize;

    /* Used only if Flag::BackgroundBlur is enabled */
    Containers::Array<Vector2> backgroundBlurVertices;
    Containers::Array<UnsignedInt> backgroundBlurIndices;
    UnsignedInt backgroundBlurPassCount = 1;

    /* 0/4 bytes free */

    /* Used only if shared.dynamicStyleCount is non-zero */
    Containers::ArrayTuple dynamicStyleStorage;
    Containers::ArrayView<BaseLayerStyleUniform> dynamicStyleUniforms;
    Containers::ArrayView<Vector4> dynamicStylePaddings;
};

}}

#endif
