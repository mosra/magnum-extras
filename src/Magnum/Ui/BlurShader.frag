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

#if !defined(FIRST_TAP_AT_CENTER) || COUNT > 1
#ifdef EXPLICIT_UNIFORM_LOCATION
layout(location = 1)
#endif
uniform highp vec2 direction;
#endif

#ifdef EXPLICIT_BINDING
layout(binding = 6)
#endif
uniform lowp sampler2D textureData;

in mediump vec2 textureCoordinates;

out lowp vec4 fragmentColor;

void main() {
    #ifdef FIRST_TAP_AT_CENTER
    fragmentColor.rgb = texture(textureData, textureCoordinates).rgb*weights[0];
    #else
    fragmentColor.rgb = vec3(0.0);
    #endif

    #if !defined(FIRST_TAP_AT_CENTER) || COUNT > 1
    for(int i =
        #ifdef FIRST_TAP_AT_CENTER
        1
        #else
        0
        #endif
        ; i < COUNT; ++i)
    {
        fragmentColor.rgb += texture(textureData, textureCoordinates + offsets[i]*direction).rgb*weights[i];
        fragmentColor.rgb += texture(textureData, textureCoordinates - offsets[i]*direction).rgb*weights[i];
    }
    #endif

    fragmentColor.a = 1.0;
}
