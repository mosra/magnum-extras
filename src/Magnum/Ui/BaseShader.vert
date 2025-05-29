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

struct StyleEntry {
    lowp vec4 topColor;
    lowp vec4 bottomColor;
    lowp vec4 outlineColor;
    mediump vec4 outlineWidth; /* left, top, right, bottom */
    mediump vec4 cornerRadius; /* top left, bottom left, top right, bottom right */
    mediump vec4 innerOutlineCornerRadius;
};

layout(std140
    #ifdef EXPLICIT_BINDING
    , binding = 0
    #endif
) uniform Style {
    lowp vec4 smoothnessInnerOutlineSmoothnessBackgroundBlurAlphaReserved;
    StyleEntry styles[STYLE_COUNT];
};

#define commonStyle_smoothness smoothnessInnerOutlineSmoothnessBackgroundBlurAlphaReserved.x
#ifndef NO_OUTLINE
#define commonStyle_innerOutlineSmoothness smoothnessInnerOutlineSmoothnessBackgroundBlurAlphaReserved.y
#endif

#ifdef EXPLICIT_UNIFORM_LOCATION
layout(location = 0)
#endif
uniform highp vec3 projection; /* xy = UI size to unit square scaling,
                                  z = pixel smoothness to UI size scaling */

layout(location = 0) in highp vec2 position;
#ifndef SUBDIVIDED_QUADS
layout(location = 1) in mediump vec2 centerDistance;
#ifndef NO_OUTLINE
layout(location = 2) in mediump vec4 outlineWidth;
#endif
#else
#ifndef TEXTURED
layout(location = 1) in mediump float centerDistanceY;
#else
layout(location = 1) in mediump vec3 centerDistanceYTextureScale;
#define centerDistanceY centerDistanceYTextureScale.x
#define textureScale centerDistanceYTextureScale.yz
#endif
layout(location = 2) in mediump vec2 outlineWidth;
#endif
layout(location = 3) in lowp vec4 color;
layout(location = 4) in mediump uint style;
#ifdef TEXTURED
layout(location = 5) in mediump vec3 textureCoordinates;
#endif

flat out mediump uint interpolatedStyle;
NOPERSPECTIVE out lowp vec4 interpolatedColor;
#ifdef TEXTURED
NOPERSPECTIVE out mediump vec3 interpolatedTextureCoordinates;
#endif
#ifdef BACKGROUND_BLUR
NOPERSPECTIVE out highp vec2 backgroundBlurTextureCoordinates;
#endif
#ifndef SUBDIVIDED_QUADS
flat out mediump vec2 halfQuadSize;
flat out mediump vec4 outlineQuadSize;
NOPERSPECTIVE out mediump vec2 interpolatedCenterDistance;
#else
/* Horizontal, vertical, outline horizontal, outline vertical */
NOPERSPECTIVE out mediump vec4 edgeDistance;
NOPERSPECTIVE out mediump float cornerRadius;
NOPERSPECTIVE out mediump float innerOutlineCornerRadius;
#endif

void main() {
    interpolatedStyle = style;

    /* Case with just a single quad -- the position, center distance and
       texture coordinates all already contain the smoothness expansion */
    #ifndef SUBDIVIDED_QUADS
    /* The halfQuadSize passed to the fragment shader needs to be *without* the
       expansion to correctly know where the edges are */
    halfQuadSize = abs(centerDistance) - vec2(commonStyle_smoothness*projection.z);
    #ifndef NO_OUTLINE
    /* Calculate the outline quad size here already to save a vec4 load in each
       fragment shader invocation */
    mediump vec4 combinedOutlineWidth = styles[style].outlineWidth + outlineWidth;
    outlineQuadSize = vec4(-halfQuadSize + combinedOutlineWidth.xy,
                           +halfQuadSize - combinedOutlineWidth.zw);
    #endif
    /* Calculate the gradient here already to save two vec4 loads in each
       fragment shader invocation. Have to extrapolate to again undo the quad
       expansion, i.e. at a top/bottom edge it should still be exactly the
       (alpha-faded) top/bottom color no matter what the smoothness is. */
    interpolatedColor = mix(styles[style].topColor, styles[style].bottomColor, 0.5*centerDistance.y/halfQuadSize.y + 0.5)*color;
    interpolatedCenterDistance = centerDistance;
    #ifdef TEXTURED
    /* Texture coordinates are already containing the smoothness expansion,
       pass then unchanged */
    interpolatedTextureCoordinates = textureCoordinates;
    #endif

    /* The projection scales from UI size to the 2x2 unit square and Y-flips,
       the (-1, 1) then translates the origin from top left to center */
    gl_Position = vec4(projection.xy*position + vec2(-1.0, 1.0), 0.0, 1.0);

    /* Case with 16 subdivided quads. They're all initially positioned in the
       corners and get expanded based on corner radii, outline width and
       smoothness. */
    #else
    /* Pick corner radii and horizontal/vertical outline width belonging to
       this corner based on vertex ID

        0---1---5---4
        |   |   |   |
        2---3---7---6
        |   |   |   |
        |   |   |   |
        |   |   |   |
        10-11---15-14
        |   |   |   |
        8---9---13-12 */
    lowp int vertexId = gl_VertexID & 15;
    lowp int cornerId = vertexId >> 2;
    mediump vec2 totalOutlineWidth = outlineWidth;
    /* Could use vector component indexing into a xzyw / xzxz / yyww swizzle to
       deduplicate, unfortunately Mesa (or Intel?) seems to incorrectly
       interpret xzyw as xyzw, swapping bottom left and top right corner. NV
       doesn't. One solution could be to reorder the vertices to not need that
       first swizzle, but benchmark shows that component indexing and these
       branches have comparable speed so I won't bother, plus certain GPUs and
       drivers don't allow access with a dynamic index, so this works
       everywhere. */
    if(cornerId == 0) {         /* Top left */
        cornerRadius = styles[style].cornerRadius.x;
        innerOutlineCornerRadius = styles[style].innerOutlineCornerRadius.x;
        totalOutlineWidth += vec2(styles[style].outlineWidth.x,
                                  styles[style].outlineWidth.y);
    } else if(cornerId == 1) {  /* Top right */
        cornerRadius = styles[style].cornerRadius.z;
        innerOutlineCornerRadius = styles[style].innerOutlineCornerRadius.z;
        totalOutlineWidth += vec2(styles[style].outlineWidth.z,
                                  styles[style].outlineWidth.y);
    } else if(cornerId == 2) {  /* Bottom left */
        cornerRadius = styles[style].cornerRadius.y;
        innerOutlineCornerRadius = styles[style].innerOutlineCornerRadius.y;
        totalOutlineWidth += vec2(styles[style].outlineWidth.x,
                                  styles[style].outlineWidth.w);
    } else if(cornerId == 3) {  /* Bottom right */
        cornerRadius = styles[style].cornerRadius.w;
        innerOutlineCornerRadius = styles[style].innerOutlineCornerRadius.w;
        totalOutlineWidth += vec2(styles[style].outlineWidth.z,
                                  styles[style].outlineWidth.w);
    } else {
        /* Without this, NV says "warning C7050: "<ver>" might be used before
           being initialized". It isn't, as cornerId is only ever those four
           values. */
        cornerRadius = 0.0f;
        innerOutlineCornerRadius = 0.0f;
    }

    /* Minimal inner quad shift needed to cover the inner/outer corner radius
       or the smoothness, whichever is largest. At the very least it has to be
       shifted by 1 *pixel* (so, also multiplied by projection.z) in addition
       to the outline width in order to have a non-zero `edgeDistance` value
       for the inner vertices. If they'd be zero, all pixels in the center quad
       would classify as being on the inner outline edge, causing the fragment
       shader to draw the whole thing with just the outline color. */
    lowp float smoothness = commonStyle_smoothness*projection.z;
    lowp float innerOutlineSmoothness = commonStyle_innerOutlineSmoothness*projection.z;
    mediump float radiusOrSmoothnessShift = max(cornerRadius, smoothness);
    mediump float innerRadiusOrSmoothnessShift = max(1.0*projection.z, max(innerOutlineCornerRadius, innerOutlineSmoothness));

    /* Calculate how far to shift so the inner vertices include both radii and
       corresponding smoothness and the total outline width, and the outer
       vertices include the outer smoothness in the opposite direction. */
    if((vertexId & 1) == 1) { /* Horizontal shift */
        edgeDistance.x = max(radiusOrSmoothnessShift, innerRadiusOrSmoothnessShift + totalOutlineWidth.x);
    } else {
        edgeDistance.x = -smoothness;
    }
    if((vertexId & 2) == 2) { /* Vertical shift */
        edgeDistance.y = max(radiusOrSmoothnessShift, innerRadiusOrSmoothnessShift + totalOutlineWidth.y);
    } else {
        edgeDistance.y = -smoothness;
    }

    /* Inner edge distance is then with the outline width subtracted */
    edgeDistance.zw = edgeDistance.xy - totalOutlineWidth.xy;

    /* And the actual signed shift value is based on whether it's the top /
       left or bottom / right corner */
    mediump vec2 shift = edgeDistance.xy*vec2(
        (cornerId & 1) == 0 ? +1.0 : -1.0,
        (cornerId & 2) == 0 ? +1.0 : -1.0);

    /* The projection scales from UI size to the 2x2 unit square and Y-flips,
       the (-1, 1) then translates the origin from top left to center */
    gl_Position = vec4(projection.xy*(shift + position) + vec2(-1.0, 1.0), 0.0, 1.0);

    /* Compared to the non-SUBDIVIDED_QUADS case above, here it's both
       interpolated and extrapolated */
    interpolatedColor = mix(styles[style].topColor, styles[style].bottomColor, 0.5*(centerDistanceY + shift.y)/abs(centerDistanceY) + 0.5)*color;

    #ifdef TEXTURED
    interpolatedTextureCoordinates = textureCoordinates + vec3(shift*textureScale, 0.0);
    #endif
    #endif

    #ifdef BACKGROUND_BLUR
    backgroundBlurTextureCoordinates = gl_Position.xy*0.5 + vec2(0.5);
    #endif
}
