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

#ifdef EXPLICIT_UNIFORM_LOCATION
layout(location = 0)
#endif
uniform highp mat3 transformationProjectionMatrix;

layout(location = 0) in highp vec2 position;
layout(location = 1) in mediump vec2 centerDistance;
layout(location = 2) in mediump vec4 outlineWidth;
layout(location = 3) in lowp vec4 color;
layout(location = 4) in mediump uint style;
#ifdef TEXTURED
layout(location = 5) in mediump vec3 textureCoordinates;
#endif

flat out mediump uint interpolatedStyle;
flat out mediump vec2 halfQuadSize;
flat out mediump vec4 interpolatedOutlineWidth;
out lowp vec4 interpolatedColor;
out mediump vec2 normalizedQuadPosition;
#ifdef TEXTURED
out mediump vec3 interpolatedTextureCoordinates;
#endif
#ifdef BACKGROUND_BLUR
out highp vec2 backgroundBlurTextureCoordinates;
#endif

void main() {
    interpolatedStyle = style;
    halfQuadSize = abs(centerDistance);
    interpolatedOutlineWidth = outlineWidth;
    interpolatedColor = color;
    normalizedQuadPosition = sign(centerDistance);
    #ifdef TEXTURED
    interpolatedTextureCoordinates = textureCoordinates;
    #endif

    gl_Position = vec4(transformationProjectionMatrix*vec3(position, 1.0), 0.0).xywz;

    #ifdef BACKGROUND_BLUR
    backgroundBlurTextureCoordinates = gl_Position.xy*0.5 + vec2(0.5);
    #endif
}
