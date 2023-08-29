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
    lowp vec4 smoothnessInnerOutlineSmoothnessReservedReserved;
    StyleEntry styles[STYLE_COUNT];
};

#define style_smoothness smoothnessInnerOutlineSmoothnessReservedReserved.x
#define style_innerOutlineSmoothness smoothnessInnerOutlineSmoothnessReservedReserved.y

flat in mediump uint interpolatedStyle;
flat in mediump vec2 halfQuadSize;
flat in mediump vec4 interpolatedOutlineWidth;
in mediump vec4 interpolatedColor;
in mediump vec2 normalizedQuadPosition; /* -1 to +1 in both coordinates */

out lowp vec4 fragmentColor;

void main() {
    mediump vec2 position = normalizedQuadPosition*halfQuadSize;

    lowp float dist;
    {
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
        else {
            lowp vec2 edgeDistance = halfQuadSize - abs(position);
            dist = min(edgeDistance.x, edgeDistance.y);
        }
    }

    lowp float outlineDist;
    {
        mediump vec4 outlineWidth = styles[interpolatedStyle].outlineWidth + interpolatedOutlineWidth;
        mediump vec4 outlineQuadSize = vec4(
            -halfQuadSize + outlineWidth.xy,
            +halfQuadSize - outlineWidth.zw);

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
        else {
            lowp vec4 edgeDistance = vec4(
                position - outlineQuadSize.xy,
                outlineQuadSize.zw - position);
            outlineDist = min(
                min(edgeDistance.x, edgeDistance.y),
                min(edgeDistance.z, edgeDistance.w));
        }
    }

    /* Gradient */
    lowp vec4 gradientColor = mix(
        styles[interpolatedStyle].topColor,
        styles[interpolatedStyle].bottomColor,
        (normalizedQuadPosition.y + 1.0)*0.5)*interpolatedColor;

    /* Mix with outline color */
    lowp float innerOutlineSmoothness = style_innerOutlineSmoothness;
    lowp vec4 outlineColor = styles[interpolatedStyle].outlineColor*gradientColor;
    lowp vec4 color = mix(outlineColor, gradientColor, smoothstep(-innerOutlineSmoothness, +innerOutlineSmoothness, outlineDist));

    /* Final color */
    lowp float smoothness = style_smoothness;
    fragmentColor = smoothstep(-smoothness, +smoothness, dist)*color;
}
