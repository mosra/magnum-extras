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
    lowp vec4 backgroundColor;
    mediump vec4 cornerRadiusReserved;
};

layout(std140
    #ifdef EXPLICIT_BINDING
    , binding = 1
    #endif
) uniform Style {
    lowp vec4 smoothnessReserved;
    StyleEntry styles[STYLE_COUNT];
};

#define style_smoothness smoothnessReserved.x
#define style_cornerRadius cornerRadiusReserved.x

#ifdef EXPLICIT_UNIFORM_LOCATION
layout(location = 0)
#endif
uniform highp vec3 projection; /* xy = UI size to unit square scaling,
                                  z = pixel smoothness to UI size scaling */

flat in mediump vec2 halfQuadSize;
NOPERSPECTIVE in mediump vec2 interpolatedCenterDistance;
flat in lowp float interpolatedOpacity;
flat in mediump uint interpolatedStyle;

out lowp vec4 fragmentColor;

void main() {
    mediump float radius = styles[interpolatedStyle].style_cornerRadius;

    /* Is (0, 0) in centers of corner radii, positive in corners, negative at
       the edges */
    lowp vec2 cornerCenterDistance = abs(interpolatedCenterDistance) - halfQuadSize + vec2(radius);
    /* Distance from the actual (rounded) edge, positive inside, negative
       outside */
    lowp float edgeDistance =
        /* (Negative) distance from the corner edge, or +radius if not inside
           any corner */
        radius - length(max(cornerCenterDistance, vec2(0.0)))
        /* (Positive) distance from closest center of corner radii, or 0 if
           at the edges */
        - min(max(cornerCenterDistance.x, cornerCenterDistance.y), 0.0);

    /* Final color */
    lowp float smoothness = style_smoothness*projection.z;
    fragmentColor = smoothstep(-smoothness, +smoothness, edgeDistance)*styles[interpolatedStyle].backgroundColor*interpolatedOpacity;
}
