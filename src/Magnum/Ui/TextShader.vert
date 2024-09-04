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
    lowp vec4 color;
};

layout(std140
    #ifdef EXPLICIT_BINDING
    , binding = 0
    #endif
) uniform Style {
    /* Reserved so users set up the style data from a placeholder
       TextLayerStyleCommon and TextLayerStyleItem[] instead of just items
       alone, which would make future code adapting rather error-prone */
    lowp vec4 reserved;
    StyleEntry styles[STYLE_COUNT];
};

#ifdef EXPLICIT_UNIFORM_LOCATION
layout(location = 0)
#endif
uniform highp vec2 projection;

layout(location = 0) in highp vec2 position;
layout(location = 1) in mediump vec3 textureCoordinates;
layout(location = 2) in lowp vec4 color;
layout(location = 3) in mediump uint style;

NOPERSPECTIVE out mediump vec3 interpolatedTextureCoordinates;
NOPERSPECTIVE out lowp vec4 interpolatedColor;

void main() {
    interpolatedTextureCoordinates = textureCoordinates;
    /* Calculate the combined color here already to save a vec4 load in each
       fragment shader invocation */
    interpolatedColor = styles[style].color*color;

    /* The projection scales from UI size to the 2x2 unit square and Y-flips,
       the (-1, 1) then translates the origin from top left to center */
    gl_Position = vec4(projection*position + vec2(-1.0, 1.0), 0.0, 1.0);
}
