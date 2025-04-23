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

#ifdef DISTANCE_FIELD
struct StyleEntry {
    lowp vec4 color;
    lowp vec4 outlineColor;
    mediump vec4 outlineWidthEdgeOffsetSmoothnessReserved;
};

layout(std140
    #ifdef EXPLICIT_BINDING
    , binding = 0
    #endif
) uniform Style {
    lowp vec4 smoothnessReserved;
    StyleEntry styles[STYLE_COUNT];
};

#define commonStyle_smoothness smoothnessReserved.x
#define style_outlineWidth outlineWidthEdgeOffsetSmoothnessReserved.x
#define style_edgeOffset outlineWidthEdgeOffsetSmoothnessReserved.y
#define style_smoothness outlineWidthEdgeOffsetSmoothnessReserved.z

#ifdef EXPLICIT_UNIFORM_LOCATION
layout(location = 0)
#endif
uniform highp vec4 projection; /* xy = UI size to unit square scaling,
                                  z = one pixel as a distance value delta,
                                  w = one UI unit as a distance value delta */
#endif

#ifdef EXPLICIT_BINDING
layout(binding = 0)
#endif
uniform lowp sampler2DArray glyphTextureData;

NOPERSPECTIVE in mediump vec3 interpolatedTextureCoordinates;
flat in lowp vec4 interpolatedColor;
#ifdef DISTANCE_FIELD
flat in mediump uint interpolatedStyle;
flat in mediump float interpolatedInvertedRunScale;
#endif

out lowp vec4 fragmentColor;

void main() {
    lowp float factor = texture(glyphTextureData, interpolatedTextureCoordinates).r;
    #ifndef DISTANCE_FIELD
    fragmentColor = interpolatedColor*factor;
    #else
    /* Scale from the style-supplied values that are in UI units to distance
       field value delta */
    lowp float scale = projection.w*interpolatedInvertedRunScale;
    /* Similarly to LineLayer, the common smoothness is in pixels, treated the
       same way as in BaseLayer and TextLayer. The per-style smoothness is in
       UI units for glows and such. Pick one that's larger when both are
       converted to the UI units and scale both of those values to distance
       field value delta. */
    lowp float smoothness = max(
        commonStyle_smoothness*projection.z*interpolatedInvertedRunScale,
        styles[interpolatedStyle].style_smoothness*scale);

    /* For consistency with SVG, the outline is by default centered on the
       edge, instead of just growing outwards. Thus, assuming distance field
       value 0.5 represents the edge and larger values are inside, the actual
       inner edge is *minus* the edge offset (scaled to distance field value
       delta) and *minus* (again scaled) half of outline width. */
    lowp float outlineWidth = styles[interpolatedStyle].style_outlineWidth*scale;
    lowp float innerEdge = 0.5 - styles[interpolatedStyle].style_edgeOffset*scale + 0.5*outlineWidth;
    /* The outer edge is then the inner edge plus the whole outline width. Need
       to ensure that it's always zero for distance value 0.0, as otherwise the
       whole quad would be colored if either the edge offset or the outline
       width overflows what's representable in given distance field radius. */
    lowp float outerEdge = max(innerEdge - outlineWidth, smoothness);

    /* Thanks to the clamp, the outer edge factor is guaranteed to be always
       larger than zero for non-zero distance field values. The inner edge
       factor is then clamped to be always at most the outer edge factor (in
       which case only the outline is drawn but not the abse color). If the
       smoothness is larger than what's representable by the radius or the
       distance field art is too thin, the inner factor may not even reach
       1. */
    lowp float outerEdgeFactor = smoothstep(outerEdge - smoothness,
                                            outerEdge + smoothness, factor);
    lowp float innerEdgeFactor = min(smoothstep(innerEdge - smoothness,
                                                innerEdge + smoothness, factor),
                                     outerEdgeFactor);

    /* Blend between the base and outline color. Approach similar to
       BaseShader.frag, except that we're not dealing with different inner and
       outer smoothness or different per-edge outline width so it's way
       simpler. But the goal is again that if there's zero outline width, the
       outline color shouldn't leak anywhere, and if the edge offset makes the
       inside disappear, the base color doesn't leak anywhere either. */
    fragmentColor = innerEdgeFactor*interpolatedColor + (outerEdgeFactor - innerEdgeFactor)*styles[interpolatedStyle].outlineColor;
    #endif
}
