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

#ifdef EXPLICIT_UNIFORM_LOCATION
layout(location = 0)
#endif
uniform highp vec3 projection; /* xy = UI size to unit square scaling,
                                  z = pixel smoothness to UI size scaling */

layout(location = 0) in highp vec2 position;
layout(location = 1) in mediump vec2 centerDistance;
layout(location = 2) in mediump uint style;

flat out mediump vec2 halfQuadSize;
NOPERSPECTIVE out mediump vec2 interpolatedCenterDistance;
flat out mediump uint interpolatedStyle;

void main() {
    /* Expand the quad by the smoothness radius to avoid the edges looking cut
       off with non-zero smoothness. Similar thing is done in BaseLayer,
       although there it has to be CPU-side in order to correctly adjust
       texture coordinates as well. */
    lowp vec2 smoothnessExpansion = vec2(style_smoothness)*projection.z*sign(centerDistance);
    halfQuadSize = abs(centerDistance);
    interpolatedCenterDistance = centerDistance + smoothnessExpansion;
    interpolatedStyle = style;

    /* The projection scales from UI size to the 2x2 unit square and Y-flips,
       the (-1, 1) then translates the origin from top left to center */
    gl_Position = vec4(projection.xy*(position + smoothnessExpansion) + vec2(-1.0, 1.0), 0.0, 1.0);
}
