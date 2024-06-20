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
#ifndef NO_OUTLINE
#define style_innerOutlineSmoothness smoothnessInnerOutlineSmoothnessBackgroundBlurAlphaReserved.y
#endif
#define style_backgroundBlurAlpha smoothnessInnerOutlineSmoothnessBackgroundBlurAlphaReserved.z

#ifdef TEXTURED
#ifdef EXPLICIT_BINDING
layout(binding = 0)
#endif
uniform lowp sampler2DArray textureData;
#endif

#ifdef BACKGROUND_BLUR
#ifdef EXPLICIT_BINDING
layout(binding = 1)
#endif
uniform lowp sampler2D backgroundBlurTextureData;
#endif

flat in mediump uint interpolatedStyle;
flat in mediump vec2 halfQuadSize;
#ifndef NO_OUTLINE
flat in mediump vec4 outlineQuadSize;
#endif
NOPERSPECTIVE in mediump vec4 interpolatedColor;
NOPERSPECTIVE in mediump vec2 interpolatedCenterDistance;
#ifdef TEXTURED
NOPERSPECTIVE in mediump vec3 interpolatedTextureCoordinates;
#endif
#ifdef BACKGROUND_BLUR
NOPERSPECTIVE in highp vec2 backgroundBlurTextureCoordinates;
#endif

out lowp vec4 fragmentColor;

void main() {
    mediump vec2 position = interpolatedCenterDistance;

    lowp float dist;
    {
        #ifndef NO_ROUNDED_CORNERS
        /* Rounded corner centers */
        mediump vec4 radius = styles[interpolatedStyle].cornerRadius;
        mediump vec2 c0 = vec2(-halfQuadSize.x + radius[0], -halfQuadSize.y + radius[0]);
        mediump vec2 c1 = vec2(-halfQuadSize.x + radius[1], +halfQuadSize.y - radius[1]);
        mediump vec2 c2 = vec2(+halfQuadSize.x - radius[2], -halfQuadSize.y + radius[2]);
        mediump vec2 c3 = vec2(+halfQuadSize.x - radius[3], +halfQuadSize.y - radius[3]);

        /* Distance from the edge */
        if(position.x < c0.x && position.y < c0.y)
            dist = (radius[0] - distance(position, c0));
        else if(position.x < c1.x && position.y > c1.y)
            dist = (radius[1] - distance(position, c1));
        else if(position.x > c2.x && position.y < c2.y)
            dist = (radius[2] - distance(position, c2));
        else if(position.x > c3.x && position.y > c3.y)
            dist = (radius[3] - distance(position, c3));
        else
        #endif
        {
            lowp vec2 edgeDistance = halfQuadSize - abs(position);
            dist = min(edgeDistance.x, edgeDistance.y);
        }
    }

    #ifndef NO_OUTLINE
    lowp float outlineDist;
    {
        #ifndef NO_ROUNDED_CORNERS
        /* Outline rounded corner centers */
        mediump vec4 radius = styles[interpolatedStyle].outlineCornerRadius;
        mediump vec2 c0 = vec2(outlineQuadSize.x + radius[0], outlineQuadSize.y + radius[0]);
        mediump vec2 c1 = vec2(outlineQuadSize.x + radius[1], outlineQuadSize.w - radius[1]);
        mediump vec2 c2 = vec2(outlineQuadSize.z - radius[2], outlineQuadSize.y + radius[2]);
        mediump vec2 c3 = vec2(outlineQuadSize.z - radius[3], outlineQuadSize.w - radius[3]);

        /* Distance from the edge */
        if(position.x < c0.x && position.y < c0.y)
            outlineDist = (radius[0] - distance(position, c0));
        else if(position.x < c1.x && position.y > c1.y)
            outlineDist = (radius[1] - distance(position, c1));
        else if(position.x > c2.x && position.y < c2.y)
            outlineDist = (radius[2] - distance(position, c2));
        else if(position.x > c3.x && position.y > c3.y)
            outlineDist = (radius[3] - distance(position, c3));
        else
        #endif
        {
            lowp vec4 edgeDistance = vec4(
                position - outlineQuadSize.xy,
                outlineQuadSize.zw - position);
            outlineDist = min(
                min(edgeDistance.x, edgeDistance.y),
                min(edgeDistance.z, edgeDistance.w));
        }
    }
    #endif

    /* Gradient, optionally textured */
    lowp vec4 gradientColor = interpolatedColor;
    #ifdef TEXTURED
    lowp vec4 textureColor = texture(textureData, interpolatedTextureCoordinates);
    gradientColor *= textureColor;
    #endif

    #ifndef NO_OUTLINE
    /* Transition to outline color */
    lowp float innerOutlineSmoothness = style_innerOutlineSmoothness;
    lowp vec4 outlineColor = styles[interpolatedStyle].outlineColor;
    #ifdef TEXTURE_MASK
    outlineColor *= textureColor.a;
    #endif
    lowp vec4 color = mix(outlineColor, gradientColor, smoothstep(-innerOutlineSmoothness, +innerOutlineSmoothness, outlineDist));
    #else
    lowp vec4 color = gradientColor;
    #endif

    #ifdef BACKGROUND_BLUR
    /* The blurred background color is first multiplied with the background
       blur alpha coming from the style. If less than 1, it achieves an effect
       where the unblurred background shines through. */
    lowp vec4 blurred = texture(backgroundBlurTextureData, backgroundBlurTextureCoordinates)*style_backgroundBlurAlpha;
    #ifdef TEXTURE_MASK
    blurred *= textureColor.a;
    #endif
    /* Blend the color with the blurred background based on the alpha channel.
       The input colors are assumed to be premultiplied so it's not a mix() but
       a simpler operation. I.e., color.rgb*color.a is already done on
       input.

       If background blur alpha coming from the style is 1, this makes the
       final alpha 1 (effectively adding the difference between color.a and 1
       to color.a), i.e. no unblurred background shining through. */
    color += blurred*(1.0 - color.a);
    #endif

    /* Final color */
    lowp float smoothness = style_smoothness;
    fragmentColor = smoothstep(-smoothness, +smoothness, dist)*color;
}
