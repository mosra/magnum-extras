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

struct StyleEntry {
    lowp vec4 topColor;
    lowp vec4 bottomColor;
    lowp vec4 outlineColor;
    mediump vec4 outlineWidth; /* left, top, right, bottom */
    mediump vec4 cornerRadius; /* top left, bottom left, top right, bottom right */
    mediump vec4 outlineCornerRadius;
};

layout(std140
    #ifdef EXPLICIT_BINDING
    , binding = 0
    #endif
) uniform Style {
    lowp vec4 smoothnessInnerOutlineSmoothnessBackgroundBlurAlphaReserved;
    StyleEntry styles[STYLE_COUNT];
};

#define style_smoothness smoothnessInnerOutlineSmoothnessBackgroundBlurAlphaReserved.x
// #ifndef NO_OUTLINE
#define style_innerOutlineSmoothness smoothnessInnerOutlineSmoothnessBackgroundBlurAlphaReserved.y
// #endif

#ifdef EXPLICIT_UNIFORM_LOCATION
layout(location = 0)
#endif
uniform highp mat3 transformationProjectionMatrix;

layout(location = 0) in highp vec2 position;
layout(location = 1) in mediump vec2 centerDistance; // TODO is this needed for subdivided quads at all? maybe only for texcoords
#ifndef SUBDIVIDED_QUADS
#ifndef NO_OUTLINE
layout(location = 2) in mediump vec4 outlineWidth;
#endif
#else
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
/* horizontal, vertical, outline horizontal, outline vertical */
NOPERSPECTIVE out mediump vec4 edgeDistance;
NOPERSPECTIVE out mediump vec2 radiusCenterDistance;
NOPERSPECTIVE out mediump vec2 outlineRadiusCenterDistance;
// TODO need only this and not the radiusCenterDistance + outlineRadiusCenterDistance
flat out mediump float cornerRadius;
flat out mediump float outlineCornerRadius;
#endif

void main() {
    interpolatedStyle = style;
    #ifndef SUBDIVIDED_QUADS
    /* The center distance already contains the smoothness expansion, together
       with position and texture coordinates. The halfQuadSize passed to the
       fragment shader however needs to be without the expansion to correctly
       know where the edges are. */
    halfQuadSize = abs(centerDistance) - vec2(style_smoothness);
    #ifndef NO_OUTLINE
    /* Calculate the outline quad size here already to save a vec4 load in each
       fragment shader invocation */
    mediump vec4 combinedOutlineWidth = styles[style].outlineWidth + outlineWidth;
    outlineQuadSize = vec4(-halfQuadSize + combinedOutlineWidth.xy,
                           +halfQuadSize - combinedOutlineWidth.zw);
    #endif
    /* Calculate the gradient here already to save two vec4 loads in each
       fragment shader invocation. Have to extrapolate to undo the quad
       expansion, i.e. at a top/bottom edge it should still be exactly the
       (alpha-faded) top/bottom color no matter what the smoothness is. */
    interpolatedColor = mix(styles[style].topColor, styles[style].bottomColor, 0.5*centerDistance.y/halfQuadSize.y + 0.5)*color;
    interpolatedCenterDistance = centerDistance;
    #ifdef TEXTURED
    interpolatedTextureCoordinates = textureCoordinates;
    #endif

    gl_Position = vec4(transformationProjectionMatrix*vec3(position, 1.0), 0.0).xywz;
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
        8---9---13-12
    */
//     float cornerRadius;
//     float outlineCornerRadius;
    mediump vec2 totalOutlineWidth = outlineWidth;
    lowp int vertexId = gl_VertexID & 15;
    lowp int cornerId = vertexId >> 2;
//     vec2 shiftDirection;

    // TODO nvidia driver complains that these may be unused, what to do? at
    //  least one of the four branches below is always taken
    cornerRadius = 0.0f;
    outlineCornerRadius = 0.0f;


    /* Top left */
    if(cornerId == 0) { // TODO could be also depending on centerDistance, maybe one attribute less that way?
        cornerRadius = styles[style].cornerRadius.x;
        outlineCornerRadius = styles[style].outlineCornerRadius.x;
        totalOutlineWidth += vec2(styles[style].outlineWidth.x,
                                  styles[style].outlineWidth.y);
//         shiftDirection = vec2(+1.0, +1.0);
    } // TODO or else??
    /* Top right */
    if(cornerId == 1) {
        cornerRadius = styles[style].cornerRadius.z;
        outlineCornerRadius = styles[style].outlineCornerRadius.z;
        totalOutlineWidth += vec2(styles[style].outlineWidth.z,
                                  styles[style].outlineWidth.y);
//         shiftDirection = vec2(-1.0, +1.0);
    }
    /* Bottom left */
    // TODO could also be dynamic indexing, no? the driver should support that
    // TODO need to fix the vertex order to match that tho .. or swizzle? that's stupid
    if(cornerId == 2) {
        cornerRadius = styles[style].cornerRadius.y;
        outlineCornerRadius = styles[style].outlineCornerRadius.y;
        totalOutlineWidth += vec2(styles[style].outlineWidth.x,
                                  styles[style].outlineWidth.w);
//         shiftDirection = vec2(+1.0, -1.0);
    }
    /* Bottom right */
    if(cornerId == 3) {
        cornerRadius = styles[style].cornerRadius.w;
        outlineCornerRadius = styles[style].outlineCornerRadius.w;
        totalOutlineWidth += vec2(styles[style].outlineWidth.z,
                                  styles[style].outlineWidth.w);
//         shiftDirection = vec2(-1.0, -1.0);
    }

    /* If not a corner point, move the position so it includes both radii and
       outline width. Horizontal shift ... */
    mediump vec2 shift = vec2(0.0);
    edgeDistance = vec4(0.0, 0.0, -totalOutlineWidth);
//     vec2 innerEdgeDistance = vec2(0.0);
    if((vertexId & 1) == 1) {
        // TODO could abuse sign() which returns 0 for 0
        mediump float direction = (cornerId & 1) == 0 ? +1.0 : -1.0;
        shift.x = direction*max(cornerRadius + style_smoothness, outlineCornerRadius + style_innerOutlineSmoothness + totalOutlineWidth.x);
        // TODO needs to include smoothness radius also
        // TODO simplify, the abs is unneeded
        edgeDistance.x = max(1.0, abs(shift.x)); // TODO ?! why the 1
        edgeDistance.z = max(1.0, abs(shift.x) - totalOutlineWidth.x); // TODO the 1 makes a mess!
    }
    /* ... vertical shift */
    if((vertexId & 3) == 2 || (vertexId & 3) == 3) {
        mediump float direction = (cornerId & 2) == 0 ? +1.0 : -1.0;
        shift.y = direction*max(cornerRadius + style_smoothness, outlineCornerRadius + style_innerOutlineSmoothness + totalOutlineWidth.y);
        // TODO needs to include smoothness radius also
        // TODO simplify, the abs is unneeded
        edgeDistance.y = max(1.0, abs(shift.y)); // TODO ?! why the 1
        edgeDistance.w = max(1.0, abs(shift.y) - totalOutlineWidth.y); // TODO the 1 makes a mess!
    }

    gl_Position = vec4(transformationProjectionMatrix*vec3(shift + position, 1.0), 0.0).xywz;

    /* Fill the output attributes. For outer points edge distance is 0 and
       outline edge distance -outlineWidth. */
    mediump vec2 absShift = abs(shift);
//     edgeDistance = vec4(
//         // TODO abs??
//         outerEdgeDistance, // TODO ???
//         -(outerEdgeDistance - outlineWidth)//, vec2(0.0)) // TODO doesn't work at all; also where's inner outline smoothness??
//     );
    /* For points inside the radius the center is positive in both coordinates,
       for points outside it's negative */
    // TODO abs??
    radiusCenterDistance = vec2(cornerRadius) - abs(shift);
    outlineRadiusCenterDistance = totalOutlineWidth + vec2(outlineCornerRadius) - abs(shift);

    // TODO properly interpolate the gradient for inner vertices
    // TODO ugh which means need a quadsize for this, but fortunately just one direction .. UHG
    // TODO tho for texture coords i need both!
    interpolatedColor = color;
    if(cornerId < 2)
        interpolatedColor *= styles[style].topColor;
    else
        interpolatedColor *= styles[style].bottomColor;

    // TODO interpolatedTextureCoordinates, how to even do that?? pass a whole f matrix?! actually no, that's stupid, the user-facing API is restricted to offset+size, not 4 arbitrary coordinates, PHEW, so need just a vec2 that gives me a translation from pixel offset to texture offset
    #endif

    #ifdef BACKGROUND_BLUR
    backgroundBlurTextureCoordinates = gl_Position.xy*0.5 + vec2(0.5);
    #endif
}
