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
NOPERSPECTIVE in mediump vec4 interpolatedColor;
#ifndef SUBDIVIDED_QUADS
flat in mediump vec2 halfQuadSize;
#ifndef NO_OUTLINE
flat in mediump vec4 outlineQuadSize;
#endif
NOPERSPECTIVE in mediump vec2 interpolatedCenterDistance;
#else
/* Horizontal, vertical, outline horizontal, outline vertical */
NOPERSPECTIVE in mediump vec4 edgeDistance;
/* These are constant in corner quads and interpolated in the edge and center
   quads. They only get used in the corner quads, because otherwise the
   edgeDistance is always at least as large, cancelling them. */
NOPERSPECTIVE in mediump float cornerRadius;
NOPERSPECTIVE in mediump float outlineCornerRadius;
#endif
#ifdef TEXTURED
NOPERSPECTIVE in mediump vec3 interpolatedTextureCoordinates;
#endif
#ifdef BACKGROUND_BLUR
NOPERSPECTIVE in highp vec2 backgroundBlurTextureCoordinates;
#endif

out lowp vec4 fragmentColor;

void main() {
    #ifndef SUBDIVIDED_QUADS
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
            /* If rounded corners are disabled, outside of the corners it has
               to be distance to the actual corner, not just to the closest
               edge */
            #ifdef NO_ROUNDED_CORNERS
            if(edgeDistance.x < 0.0 && edgeDistance.y < 0.0)
                dist = -length(edgeDistance);
            else
            #endif
            {
                dist = min(edgeDistance.x, edgeDistance.y);
            }
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
    #else
    /* Is (0, 0) in centers of outer corner radii, positive in corners,
       negative at the edges */
    lowp vec2 cornerCenterDistance = vec2(cornerRadius) - edgeDistance.xy;
    /* Distance from the actual (rounded) edge, positive inside, negative
       outside */
    lowp float dist =
        /* (Negative) distance from the corner edge, or +radius if not inside
           any corner */
        cornerRadius - length(max(cornerCenterDistance, vec2(0.0)))
        /* (Positive) distance from closest center of corner radii, or 0 if
           at the edges */
        - min(max(cornerCenterDistance.x, cornerCenterDistance.y), 0.0);

    /* And similarly for the inner outline edge */
    lowp vec2 outlineCornerCenterDistance = vec2(outlineCornerRadius) - edgeDistance.zw;
    lowp float outlineDist =
        outlineCornerRadius - length(max(outlineCornerCenterDistance, vec2(0.0)))
        - min(max(outlineCornerCenterDistance.x, outlineCornerCenterDistance.y), 0.0);
    #endif

    #if !defined(NO_OUTLINE) || defined(SUBDIVIDED_QUADS)
    /* Distance to the inner outline edge should be never larger than distance
       to the outer edge (such as in case the inner radius is smaller than
       outer) */
    outlineDist = min(dist, outlineDist);
    #endif

    /* Gradient, optionally textured */
    lowp vec4 gradientColor = interpolatedColor;
    #ifdef TEXTURED
    lowp vec4 textureColor = texture(textureData, interpolatedTextureCoordinates);
    gradientColor *= textureColor;
    #endif

    /* Outline color, optionally with the texture alpha affecting it as well */
    #ifndef NO_OUTLINE
    lowp vec4 outlineColor = styles[interpolatedStyle].outlineColor;
    #ifdef TEXTURE_MASK
    outlineColor *= textureColor.a;
    #endif
    #endif

    /* Blurred background color. First multiplied with the background blur
       alpha coming from the style. If less than 1, it achieves an effect where
       the unblurred background shines through. Again optionally with the
       texture alpha affecting it as well. */
    #ifdef BACKGROUND_BLUR
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
    gradientColor += blurred*(1.0 - gradientColor.a);
    #ifndef NO_OUTLINE
    outlineColor += blurred*(1.0 - outlineColor.a);
    #endif
    #endif

    /* Blend between the base and outline color. The goal is that, if a
       particular edge has a zero outline width, the outline color should not
       leak to the base color. In case the inner and outer smoothness is the
       same, the outline is simply drawn only if the inner smoothness factor
       is smaller than the outer. In the diagram below, the `+` denotes
       position of to the inner and outer edge, the line the actual smoothness
       curve (same in both cases), B the base color and O the outline color.

             -----   ---
            ...BBB \ OOO \
        outlineDist + OOO + dist
              ...BBB \ OOO \
                       ---   ------

       In case the smoothness curves are different, the outer smoothness is the
       ultimate limit, i.e. even if the inner smoothness is larger, it
       shouldn't go beyond the outer smoothness (and vice versa, if the inner
       smoothness is smaller, it shouldn't cut off the base color). This is
       achieved by blending between the smoothness values based on the outline
       width (so, difference between `dist` and `outlineDist`) at given
       position. If it's large enough so that both smoothness radii can fit
       there without intersecting, the inner smoothness value is used, if it's
       0, the outer smoothness value is used. See renderEdgeOutlineSmoothness()
       in BaseLayerGLTest for actual visuals generated by this. */
    lowp float smoothness = style_smoothness;
    #ifndef NO_OUTLINE
    lowp float innerOutlineSmoothness = style_innerOutlineSmoothness;
    lowp float adjustedInnerSmoothness = mix(smoothness, innerOutlineSmoothness, min((dist - outlineDist)/(smoothness + innerOutlineSmoothness), 1.0));
    lowp float innerEdgeFactor = smoothstep(-adjustedInnerSmoothness, +adjustedInnerSmoothness, outlineDist);
    lowp float outerEdgeFactor = smoothstep(-smoothness, +smoothness, dist);
    #else
    lowp float innerEdgeFactor = smoothstep(-smoothness, +smoothness, dist);
    #endif
    fragmentColor = innerEdgeFactor*gradientColor;
    #ifndef NO_OUTLINE
    fragmentColor += max(outerEdgeFactor - innerEdgeFactor, 0.0)*outlineColor;
    #endif
}
