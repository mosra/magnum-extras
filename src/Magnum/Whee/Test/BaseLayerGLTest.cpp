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

#include <sstream> /** @todo remove once Debug is stream-free */
#include <Corrade/Containers/BitArrayView.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Containers/StringIterable.h>
#include <Corrade/PluginManager/Manager.h>
#include <Corrade/PluginManager/PluginMetadata.h>
#include <Corrade/TestSuite/Compare/Numeric.h>
#include <Corrade/Utility/Algorithms.h>
#include <Corrade/Utility/ConfigurationGroup.h>
#include <Corrade/Utility/DebugStl.h> /** @todo remove once Debug is stream-free */
#include <Corrade/Utility/Path.h>
#include <Magnum/Image.h>
#include <Magnum/ImageView.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/DebugTools/CompareImage.h>
#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/TextureArray.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/GL/OpenGLTester.h>
#include <Magnum/Math/Vector2.h>
#include <Magnum/Trade/AbstractImporter.h>
#include <Magnum/Trade/ImageData.h>

#include "Magnum/Whee/AbstractUserInterface.h"
#include "Magnum/Whee/BaseLayerGL.h"
#include "Magnum/Whee/Event.h"
#include "Magnum/Whee/Handle.h"
#include "Magnum/Whee/NodeFlags.h"
#include "Magnum/Whee/RendererGL.h"

#include "configure.h"

namespace Magnum { namespace Whee { namespace Test { namespace {

struct BaseLayerGLTest: GL::OpenGLTester {
    explicit BaseLayerGLTest();

    void sharedConstruct();
    void sharedConstructComposite();
    /* NoCreate tested in BaseLayerGL_Test to verify it works without a GL
       context */
    void sharedConstructCopy();
    void sharedConstructMove();

    void construct();
    void constructDerived();
    void constructCopy();
    void constructMove();

    void setTextureTexturingNotEnabled();

    void drawNoSizeSet();
    void drawNoStyleSet();
    void drawNoTextureSet();

    void renderSetup();
    void renderTeardown();
    template<BaseLayerSharedFlag flag = BaseLayerSharedFlag{}> void render();
    template<BaseLayerSharedFlag flag = BaseLayerSharedFlag{}> void renderCustomColor();
    template<BaseLayerSharedFlag flag = BaseLayerSharedFlag{}> void renderCustomOutlineWidth();
    template<BaseLayerSharedFlag flag = BaseLayerSharedFlag{}> void renderPadding();
    /* The SubdividedQuads flag shouldn't cover any codepaths for style change
       that weren't already tested above, done "just in case" */
    template<BaseLayerSharedFlag flag = BaseLayerSharedFlag{}> void renderChangeStyle();
    template<BaseLayerSharedFlag flag = BaseLayerSharedFlag{}> void renderTextured();
    template<BaseLayerSharedFlag flag = BaseLayerSharedFlag{}> void renderOutlineEdgeSmoothness();
    template<BaseLayerSharedFlag flag = BaseLayerSharedFlag{}> void renderGradientOutlineEdgeSmoothness();
    template<BaseLayerSharedFlag flag = BaseLayerSharedFlag{}> void renderTexturedOutlineEdgeSmoothness();

    /* The SubdividedQuads flag shouldn't cover any codepaths for dynamic
       styles that weren't already tested above, done "just in case" */
    template<BaseLayerSharedFlag flag = BaseLayerSharedFlag{}> void renderDynamicStyles();

    void renderOrDrawCompositeSetup();
    void renderOrDrawCompositeTeardown();

    template<BaseLayerSharedFlag flag = BaseLayerSharedFlag{}> void renderComposite();
    template<BaseLayerSharedFlag flag = BaseLayerSharedFlag{}> void renderCompositeEdgeSmoothness();
    template<BaseLayerSharedFlag flag = BaseLayerSharedFlag{}> void renderCompositeTextured();
    /* Composite node rectangles aren't affected by the SubdividedQuads flag */
    void renderCompositeNodeRects();

    void drawSetup();
    void drawTeardown();
    void drawOrder();
    void drawOrderComposite();
    void drawClipping();

    void eventStyleTransition();

    private:
        PluginManager::Manager<Trade::AbstractImporter> _manager;
        GL::Texture2D _color{NoCreate};
        GL::Framebuffer _framebuffer{NoCreate};
};

using namespace Containers::Literals;
using namespace Math::Literals;

const struct {
    const char* name;
    UnsignedInt dynamicStyleCount;
} DrawNoStyleSetData[]{
    {"", 0},
    {"dynamic styles", 5}
};

const struct {
    const char* name;
    const char* filename;
    BaseLayerSharedFlags flags;
    BaseLayerCommonStyleUniform styleUniformCommon;
    BaseLayerStyleUniform styleUniform;
} RenderData[]{
    {"default", "default.png", {},
        BaseLayerCommonStyleUniform{},
        BaseLayerStyleUniform{}},
    {"default, smooth", "default-smooth.png", {},
        BaseLayerCommonStyleUniform{}
            .setSmoothness(1.0f),
        BaseLayerStyleUniform{}},
    {"gradient", "gradient.png", {},
        BaseLayerCommonStyleUniform{},
        BaseLayerStyleUniform{}
            .setColor(0xeeddaa_rgbf, 0x774422_rgbf)},
    /* Interaction of the gradient with outline width and smoothness radius
       tested in renderGradientOutlineEdgeSmoothness() */
    {"rounded corners, all same, default smoothness", "rounded-corners-same-hard.png", {},
        BaseLayerCommonStyleUniform{},
        BaseLayerStyleUniform{}
            .setCornerRadius(24.0f)},
    {"rounded corners, all same", "rounded-corners-same.png", {},
        BaseLayerCommonStyleUniform{}
            .setSmoothness(1.0f),
        BaseLayerStyleUniform{}
            .setCornerRadius(24.0f)},
    {"rounded corners, different", "rounded-corners-different.png", {},
        BaseLayerCommonStyleUniform{}
            .setSmoothness(1.0f),
        BaseLayerStyleUniform{}
            /* Top left, bottom left, top right, bottom right; one radius is
               more than half of the height, one is zero */
            .setCornerRadius({4.0f, 44.0f, 24.0f, 0.0f})},
    {"outline, default color", "default.png", {},
        BaseLayerCommonStyleUniform{},
        BaseLayerStyleUniform{}
            .setOutlineWidth(8.0f)},
    {"outline, all sides same", "outline-same.png", {},
        BaseLayerCommonStyleUniform{},
        BaseLayerStyleUniform{}
            .setOutlineColor(0x7f7f7f_rgbf)
            .setOutlineWidth(8.0f)},
    {"outline, different", "outline-different.png", {},
        BaseLayerCommonStyleUniform{},
        BaseLayerStyleUniform{}
            .setOutlineColor(0x7f7f7f_rgbf)
            /* Left, top, right, bottom; one side is going over the center,
               one is zero */
            .setOutlineWidth({8.0f, 4.0f, 0.0f, 32.0f})},
    {"outline, rounded corners inside", "outline-rounded-corners-inside.png", {},
        BaseLayerCommonStyleUniform{}
            .setSmoothness(1.0f),
        BaseLayerStyleUniform{}
            .setOutlineColor(0x7f7f7f_rgbf)
            .setInnerOutlineCornerRadius(8.0f)
            .setOutlineWidth(8.0f)},
    {"outline, rounded corners, different", "outline-rounded-corners-both-different.png", {},
        BaseLayerCommonStyleUniform{}
            .setSmoothness(1.0f),
        BaseLayerStyleUniform{}
            .setOutlineColor(0x7f7f7f_rgbf)
            /* Top left, bottom left, top right, bottom right */
            .setCornerRadius({36.0f, 12.0f, 4.0f, 0.0f})
            /* Important is that the right side with zero outline width has at
               least one rounded corner, to verify it doesn't get clipped away
               due to the zero outline */
            .setInnerOutlineCornerRadius({18.0f, 6.0f, 0.0f, 18.0f})
            /* Left, top, right, bottom  */
            .setOutlineWidth({18.0f, 8.0f, 0.0f, 4.0f})},
    {"outline, rounded corners, different inner and outer smoothness", "outline-rounded-corners-different-smoothness.png", {},
        BaseLayerCommonStyleUniform{}
            .setSmoothness(1.0f, 8.0f),
        BaseLayerStyleUniform{}
            .setOutlineColor(0x7f7f7f_rgbf)
            .setCornerRadius(16.0f)
            .setInnerOutlineCornerRadius(8.0f)
            .setOutlineWidth(8.0f)},
    {"outline with gradient", "outline-gradient.png", {},
        BaseLayerCommonStyleUniform{},
        BaseLayerStyleUniform{}
            .setColor(0xffffff_rgbf, 0x333333_rgbf)
            .setOutlineColor(0x3333ff_rgbf)
            .setOutlineWidth(8.0f)},
    {"outline, rounded corners inside, no rounded corners", "outline-same.png",
        BaseLayerSharedFlag::NoRoundedCorners,
        BaseLayerCommonStyleUniform{},
            /* Smoothness omitted to match the other image */
        BaseLayerStyleUniform{}
            .setOutlineColor(0x7f7f7f_rgbf)
            .setInnerOutlineCornerRadius(8.0f)
            .setOutlineWidth(8.0f)},
    {"outline, rounded corners, different inner and outer smoothness, no outline", "rounded-corners-same.png",
        BaseLayerSharedFlag::NoOutline,
        BaseLayerCommonStyleUniform{}
            .setSmoothness(1.0f, 8.0f),
        BaseLayerStyleUniform{}
            .setOutlineColor(0x7f7f7f_rgbf)
            /* Increased from 16 to match the other image */
            .setCornerRadius(24.0f)
            .setInnerOutlineCornerRadius(8.0f)
            .setOutlineWidth(8.0f)},
    {"outline, rounded corners, different inner and outer smoothness, no rounded corners, no outline", "default-smooth.png",
        BaseLayerSharedFlag::NoRoundedCorners|BaseLayerSharedFlag::NoOutline,
        BaseLayerCommonStyleUniform{}
            .setSmoothness(1.0f, 8.0f),
        BaseLayerStyleUniform{}
            .setOutlineColor(0x7f7f7f_rgbf)
            .setCornerRadius(16.0f)
            .setInnerOutlineCornerRadius(8.0f)
            .setOutlineWidth(8.0f)},
};

const struct {
    const char* name;
    const char* filename;
    bool setLater;
    bool partialUpdate;
    BaseLayerSharedFlags flags;
} RenderCustomColorOutlineWidthData[]{
    {"", "outline-same.png",
        false, false, {}},
    {"set later", "outline-same.png",
        true, false, {}},
    {"set later, partial update", "outline-same.png",
        true, true, {}},
    {"no outline", "default.png",
        false, false, BaseLayerSharedFlag::NoOutline}
};

const struct {
    const char* name;
    bool partialUpdate;
    Vector2 nodeOffset, nodeSize;
    Vector4 paddingFromStyle, paddingFromData;
} RenderPaddingData[]{
    {"no padding", false,
        {8.0f, 8.0f}, {112.0f, 48.0f}, {}, {}},
    /* Deliberately having one excessively shifted to left/top and the other to
       bottom/right. It shouldn't cause any strange artifacts. */
    {"from style", false,
        {-64.0f, -128.0f}, {192.0f, 192.0f},
        {72.0f, 136.0f, 8.0f, 8.0f}, {}},
    {"from data", false,
        {0.0f, 0.0f}, {192.0f, 192.0f},
        {}, {8.0f, 8.0f, 72.0f, 136.0f}},
    {"from both", false,
        {0.0f, 0.0f}, {128.0f, 64.0f},
        {4.0f, 8.0f, 0.0f, 4.0f}, {4.0f, 0.0f, 8.0f, 4.0f}},
    {"from both, partial update", true,
        {0.0f, 0.0f}, {128.0f, 64.0f},
        {4.0f, 8.0f, 0.0f, 4.0f}, {4.0f, 0.0f, 8.0f, 4.0f}},
};

const struct {
    const char* name;
    bool partialUpdate;
} RenderChangeStyleData[]{
    {"", false},
    {"partial update", true},
};

const struct {
    const char* name;
    const char* textureFilename;
    const char* expectedFilename;
    bool rvalue;
    Color4 clearColor;
    Containers::Optional<Vector3> offset;
    Containers::Optional<Vector2> size;
    BaseLayerSharedFlags extraFlags;
    BaseLayerStyleUniform styleUniform;
} RenderTexturedData[]{
    {"default offset and size", "blur-input.png", "textured-default.png",
        false, 0x1f1f1f_rgbf, {}, {}, {},
        BaseLayerStyleUniform{}},
    {"", "blur-input.png", "textured.png",
        false, 0x1f1f1f_rgbf,
        /* The image is 160x106, want to render the bottom right 112x48 portion
           of it to avoid nasty scaling, and to verify the offset is taken from
           the right (bottom left) origin */
        {{48.0f/160.0f, 0.0f/106.0f, 7}}, {{112.0f/160.0f, 48.0f/106.0f}}, {},
        BaseLayerStyleUniform{}},
    /* Interaction of the texture with outline width and smoothness radius
       tested in renderTexturedOutlineEdgeSmoothness() */
    {"r-value instance", "blur-input.png", "textured.png",
        true, 0x1f1f1f_rgbf,
        {{48.0f/160.0f, 0.0f/106.0f, 7}}, {{112.0f/160.0f, 48.0f/106.0f}}, {},
        BaseLayerStyleUniform{}},
    {"colored", "blur-input.png", "textured-colored.png",
        false, 0x1f1f1f_rgbf,
        /* Top left part of the image instead */
        {{0.0f/160.0f, 58.0f/106.0f, 7}}, {{112.0f/160.0f, 48.0f/106.0f}}, {},
        BaseLayerStyleUniform{}
            .setCornerRadius(12.0f)
            .setInnerOutlineCornerRadius(4.0f)
            /* The gradient should be multiplied with the texture */
            .setColor(0xffffff_rgbf, 0x333333_rgbf)
            /* The outline shouldn't be multiplied with the texture, neither
               the texture should shine through if semi-transparent */
            .setOutlineWidth(8.0f)
            .setOutlineColor(0xa5c9eaff_rgbf*0.75f)},
    {"alpha mask", "mask-premultiplied.png", "textured-mask.png",
        /* Brighter than default clear color to verify the masked out parts
           aren't just black */
        false, 0x999999_rgbf,
        /* The image is 128x64, rendering the actually visible 112x48 part of
           it to avoid nasty scaling */
        {{8.0f/128.0f, 8.0f/64.0f, 7}}, {{112.0f/128.0f, 48.0f/64.0f}}, {},
        BaseLayerStyleUniform{}
            .setCornerRadius(12.0f)
            .setColor(0xeeddaa_rgbf, 0x774422_rgbf)},
    /* Should cause no difference compared to above */
    {"alpha mask, TextureMask", "mask-premultiplied.png", "textured-mask.png",
        false, 0x999999_rgbf,
        {{8.0f/128.0f, 8.0f/64.0f, 7}}, {{112.0f/128.0f, 48.0f/64.0f}},
        BaseLayerSharedFlag::TextureMask,
        BaseLayerStyleUniform{}
            .setCornerRadius(12.0f)
            .setColor(0xeeddaa_rgbf, 0x774422_rgbf)},
    {"alpha mask, colored", "mask-colored-premultiplied.png", "textured-mask-colored.png",
        false, 0x999999_rgbf,
        {{8.0f/128.0f, 8.0f/64.0f, 7}}, {{112.0f/128.0f, 48.0f/64.0f}}, {},
        BaseLayerStyleUniform{}
            .setCornerRadius(12.0f)
            .setColor(0xffffff_rgbf)},
    /* Should cause no difference compared to above */
    {"alpha mask, colored, TextureMask", "mask-colored-premultiplied.png", "textured-mask-colored.png",
        false, 0x999999_rgbf,
        {{8.0f/128.0f, 8.0f/64.0f, 7}}, {{112.0f/128.0f, 48.0f/64.0f}},
        BaseLayerSharedFlag::TextureMask,
        BaseLayerStyleUniform{}
            .setCornerRadius(12.0f)
            .setColor(0xffffff_rgbf)},
    /* The outline is by default not affected by the mask */
    {"alpha mask, outline", "mask-premultiplied.png", "textured-mask-outline-default.png",
        false, 0x999999_rgbf,
        {{8.0f/128.0f, 8.0f/64.0f, 7}}, {{112.0f/128.0f, 48.0f/64.0f}}, {},
        BaseLayerStyleUniform{}
            .setCornerRadius(12.0f)
            .setInnerOutlineCornerRadius(4.0f)
            .setOutlineWidth(8.0f)
            .setColor(0xeeddaa_rgbf, 0x774422_rgbf)
            .setOutlineColor(0xf1e77f_rgbf)},
    {"alpha mask, outline, TextureMask", "mask-premultiplied.png", "textured-mask-outline-mask.png",
        false, 0x999999_rgbf,
        {{8.0f/128.0f, 8.0f/64.0f, 7}}, {{112.0f/128.0f, 48.0f/64.0f}},
        BaseLayerSharedFlag::TextureMask,
        BaseLayerStyleUniform{}
            .setCornerRadius(12.0f)
            .setInnerOutlineCornerRadius(4.0f)
            .setOutlineWidth(8.0f)
            .setColor(0xeeddaa_rgbf, 0x774422_rgbf)
            .setOutlineColor(0xf1e77f_rgbf)},
};

const struct {
    const char* name;
    const char* filename;
    Float smoothness, innerOutlineSmoothness, scale;
    bool setSizeLater;
} RenderOutlineEdgeSmoothnessData[]{
    {"",
        "edge-smoothness-same.png", 8.0f, 8.0f, 1.0f, false},
    {"UI 100x larger than framebuffer",
        "edge-smoothness-same.png", 8.0f, 8.0f, 100.0f, false},
    {"UI 100x larger than framebuffer, set later",
        "edge-smoothness-same.png", 8.0f, 8.0f, 100.0f, true},
    {"UI 100x smaller than framebuffer",
        "edge-smoothness-same.png", 8.0f, 8.0f, 0.01f, false},
    {"UI 100x smaller than framebuffer, set later",
        "edge-smoothness-same.png", 8.0f, 8.0f, 0.01f, true},
    {"inner smoothness larger",
        "edge-smoothness-inner-larger.png", 1.0f, 8.0f, 1.0f, false},
    {"inner smoothness larger, UI 100x larger than framebuffer",
        "edge-smoothness-inner-larger.png", 1.0f, 8.0f, 100.0f, false},
    {"inner smoothness larger, UI 100x smaller than framebuffer",
        "edge-smoothness-inner-larger.png", 1.0f, 8.0f, 0.01f, false},
    {"inner smoothness smaller",
        "edge-smoothness-inner-smaller.png", 8.0f, 1.0f, 1.0f, false},
    {"inner smoothness smaller, UI 100x larger than framebuffer",
        "edge-smoothness-inner-smaller.png", 8.0f, 1.0f, 100.0f, false},
    {"inner smoothness smaller, UI 100x smaller than framebuffer",
        "edge-smoothness-inner-smaller.png", 8.0f, 1.0f, 0.01f, false},
};

const struct {
    const char* name;
    Float smoothness, scale;
    bool setSizeLater;
} RenderGradientOutlineEdgeSmoothnessData[]{
    {"", 0.0f, 1.0f, false},
    /* Like above, but with the outer smoothness matching the outline width.
       The quad area gets expanded for it, but the gradient shouldn't. */
    {"large outer smoothness",
        8.0f, 1.0f, false},
    {"large outer smoothness, UI 100x larger than framebuffer",
        8.0f, 100.0f, false},
    {"large outer smoothness, UI 100x larger than framebuffer, set later",
        8.0f, 100.0f, true},
    {"large outer smoothness, UI 100x smaller than framebuffer",
        8.0f, 0.01f, false},
    {"large outer smoothness, UI 100x smaller than framebuffer, set later",
        8.0f, 0.01f, true},
};

const struct {
    const char* name;
    Float smoothness, scale;
    bool setSizeLater;
} RenderTexturedOutlineEdgeSmoothnessData[]{
    {"", 1.0f, 1.0f, false},
    /* The quad gets expanded for the outer smoothness to not cut off, the
       texture coordinates should get adjusted as well to not change the
       texture scale. There's however a transparent outline so it should look
       exactly as above. */
    {"large outer smoothness",
        7.0f, 1.0f, false},
    {"large outer smoothness, UI 100x larger than framebuffer",
        7.0f, 100.0f, false},
    {"large outer smoothness, UI 100x larger than framebuffer, set later",
        7.0f, 100.0f, true},
    {"large outer smoothness, UI 100x smaller than framebuffer",
        7.0f, 0.01f, false},
    {"large outer smoothness, UI 100x smaller than framebuffer, set later",
        7.0f, 0.01f, true},
};

const struct {
    const char* name;
    const char* filename;
    UnsignedInt styleIndex;
    BaseLayerStyleUniform styleUniform;
    Float leftPadding;
    Containers::Optional<BaseLayerStyleUniform> dynamicStyleUniform;
    Float dynamicLeftPadding;
    bool createLayerAfterSetStyle;
    bool secondaryStyleUpload;
    bool secondaryDynamicStyleUpload;
    bool noBaseStyles;
} RenderDynamicStylesData[]{
    {"default, static", "default.png", 1,
        BaseLayerStyleUniform{}, 0.0f,
        {}, 0.0f,
        false, false, false, false},
    {"default, static, create layer after setStyle()", "default.png", 1,
        BaseLayerStyleUniform{}, 0.0f,
        {}, 0.0f,
        true, false, false, false},
    {"default, dynamic with no upload", "default.png", 5,
        BaseLayerStyleUniform{}, 0.0f,
        {}, 0.0f,
        false, false, false, false},
    {"default, dynamic", "default.png", 5,
        BaseLayerStyleUniform{}, 0.0f,
        BaseLayerStyleUniform{}, 0.0f,
        false, false, false, false},
    {"default, only dynamic styles", "default.png", 1,
        BaseLayerStyleUniform{}, 0.0f,
        BaseLayerStyleUniform{}, 0.0f,
        false, false, false, true},
    {"styled, static", "outline-gradient.png", 1,
        BaseLayerStyleUniform{}
            .setColor(0xffffff_rgbf, 0x333333_rgbf)
            .setOutlineColor(0x3333ff_rgbf)
            .setOutlineWidth(8.0f), 0.0f,
        {}, 0.0f,
        false, false, false, false},
    {"styled, static, create layer after setStyle()", "outline-gradient.png", 1,
        BaseLayerStyleUniform{}
            .setColor(0xffffff_rgbf, 0x333333_rgbf)
            .setOutlineColor(0x3333ff_rgbf)
            .setOutlineWidth(8.0f), 0.0f,
        {}, 0.0f,
        true, false, false, false},
    {"styled, static with padding", "outline-gradient.png", 1,
        BaseLayerStyleUniform{}
            .setColor(0xffffff_rgbf, 0x333333_rgbf)
            .setOutlineColor(0x3333ff_rgbf)
            .setOutlineWidth(8.0f), 128.0f,
        {}, 0.0f,
        false, false, false, false},
    {"styled, dynamic", "outline-gradient.png", 5,
        BaseLayerStyleUniform{}, 0.0f,
        BaseLayerStyleUniform{}
            .setColor(0xffffff_rgbf, 0x333333_rgbf)
            .setOutlineColor(0x3333ff_rgbf)
            .setOutlineWidth(8.0f), 0.0f,
        false, false, false, false},
    {"styled, dynamic with padding", "outline-gradient.png", 5,
        BaseLayerStyleUniform{}, 0.0f,
        BaseLayerStyleUniform{}
            .setColor(0xffffff_rgbf, 0x333333_rgbf)
            .setOutlineColor(0x3333ff_rgbf)
            .setOutlineWidth(8.0f), 128.0f,
        false, false, false, false},
    {"styled, static, secondary upload", "outline-gradient.png", 1,
        BaseLayerStyleUniform{}
            .setColor(0xffffff_rgbf, 0x333333_rgbf)
            .setOutlineColor(0x3333ff_rgbf)
            .setOutlineWidth(8.0f), 0.0f,
        {}, 0.0f,
        false, true, false, false},
    {"styled, static, secondary dynamic upload", "outline-gradient.png", 1,
        BaseLayerStyleUniform{}
            .setColor(0xffffff_rgbf, 0x333333_rgbf)
            .setOutlineColor(0x3333ff_rgbf)
            .setOutlineWidth(8.0f), 0.0f,
        BaseLayerStyleUniform{}, 0.0f,
        false, false, true, false},
    {"styled, dynamic, secondary upload", "outline-gradient.png", 5,
        BaseLayerStyleUniform{}, 0.0f,
        BaseLayerStyleUniform{}
            .setColor(0xffffff_rgbf, 0x333333_rgbf)
            .setOutlineColor(0x3333ff_rgbf)
            .setOutlineWidth(8.0f), 0.0f,
        false, false, true, false},
    {"styled, dynamic, secondary static upload", "outline-gradient.png", 5,
        BaseLayerStyleUniform{}, 0.0f,
        BaseLayerStyleUniform{}
            .setColor(0xffffff_rgbf, 0x333333_rgbf)
            .setOutlineColor(0x3333ff_rgbf)
            .setOutlineWidth(8.0f), 0.0f,
        false, true, false, false},
    {"styled, only dynamic styles", "outline-gradient.png", 1,
        BaseLayerStyleUniform{}, 0.0f,
        BaseLayerStyleUniform{}
            .setColor(0xffffff_rgbf, 0x333333_rgbf)
            .setOutlineColor(0x3333ff_rgbf)
            .setOutlineWidth(8.0f), 0.0f,
        false, false, false, true},
};

const struct {
    const char* name;
    const char* filename;
    BaseLayerSharedFlags flags;
    Containers::Optional<UnsignedInt> backgroundBlurRadius;
    Containers::Optional<Float> backgroundBlurCutoff;
    Containers::Optional<UnsignedInt> backgroundBlurPassCount;
    BaseLayerCommonStyleUniform styleCommon;
    BaseLayerStyleUniform styleUniform;
    Float maxThreshold, meanThreshold;
} RenderCompositeData[]{
    {"default, 50% opacity", "composite-default-50.png",
        {}, {}, {}, {},
        BaseLayerCommonStyleUniform{}
            .setSmoothness(1.0f),
        BaseLayerStyleUniform{}
            .setCornerRadius(12.0f)
            /* Premultiplied alpha */
            .setColor(0xffffffff_rgbaf*0.5f),
        0.0f, 0.0f},
    {"background blur, 0% opacity", "composite-background-blur-0.png",
        BaseLayerSharedFlag::BackgroundBlur, {}, {}, {},
        BaseLayerCommonStyleUniform{}
            .setSmoothness(1.0f),
        BaseLayerStyleUniform{}
            .setCornerRadius(12.0f)
            .setColor(0xffffffff_rgbaf),
        0.0f, 0.0f},
    {"background blur, 50% opacity", "composite-background-blur-50.png",
        BaseLayerSharedFlag::BackgroundBlur, {}, {}, {},
        BaseLayerCommonStyleUniform{}
            .setSmoothness(1.0f),
        BaseLayerStyleUniform{}
            .setCornerRadius(12.0f)
            /* Premultiplied alpha */
            .setColor(0xffffffff_rgbaf*0.5f),
        0.0f, 0.0f},
    /* Interaction of the compositing quads with smoothness radius tested in
       renderCompositeEdgeSmoothness() */
    {"background blur, 75% opacity, colored", "composite-background-blur-75-colored.png",
        BaseLayerSharedFlag::BackgroundBlur, {}, {}, {},
        BaseLayerCommonStyleUniform{}
            .setSmoothness(1.0f),
        BaseLayerStyleUniform{}
            .setCornerRadius(12.0f)
            .setInnerOutlineCornerRadius(4.0f)
            .setOutlineWidth(8.0f)
            /* Premultiplied alpha */
            .setColor(0x747474ff_rgbaf*0.75f, 0xdcdcdcff_rgbaf*0.75f)
            .setOutlineColor(0xa5c9eaff_rgbaf*0.75f),
        0.0f, 0.0f},
    /* This should look the same as if no compositing is done, including the
       same blend operation and everything. In reality there's a slight
       difference possibly due to the blend operation being done a bit
       differently? */
    {"background blur, 50% opacity, radius 0", "composite-default-50.png",
        BaseLayerSharedFlag::BackgroundBlur, 0, {}, {},
        BaseLayerCommonStyleUniform{}
            .setSmoothness(1.0f),
        BaseLayerStyleUniform{}
            .setCornerRadius(12.0f)
            /* Premultiplied alpha */
            .setColor(0xffffffff_rgbaf*0.5f),
        0.75f, 0.247f},
    /* Should be the same as the default */
    {"background blur, 50% opacity, radius 4", "composite-background-blur-50.png",
        BaseLayerSharedFlag::BackgroundBlur, 4, {}, {},
        BaseLayerCommonStyleUniform{}
            .setSmoothness(1.0f),
        BaseLayerStyleUniform{}
            .setCornerRadius(12.0f)
            /* Premultiplied alpha */
            .setColor(0xffffffff_rgbaf*0.5f),
        0.0f, 0.0f},
    /* sqrt(4*(2^2)) == 4, so should be ~same as above (plus rounding
       errors) */
    {"background blur, 50% opacity, radius 2, 4 passes", "composite-background-blur-50.png",
        BaseLayerSharedFlag::BackgroundBlur, 2, {}, 4,
        BaseLayerCommonStyleUniform{}
            .setSmoothness(1.0f),
        BaseLayerStyleUniform{}
            .setCornerRadius(12.0f)
            /* Premultiplied alpha */
            .setColor(0xffffffff_rgbaf*0.5f),
        5.75f, 0.728f},
    /* sqrt(16*(1^2)) == 4, so should ~same as above (plus even more rounding
       errors) */
    {"background blur, 50% opacity, radius 1, 16 passes", "composite-background-blur-50.png",
        BaseLayerSharedFlag::BackgroundBlur, 1, {}, 16,
        BaseLayerCommonStyleUniform{}
            .setSmoothness(1.0f),
        BaseLayerStyleUniform{}
            .setCornerRadius(12.0f)
            /* Premultiplied alpha */
            .setColor(0xffffffff_rgbaf*0.5f),
        12.25f, 1.555f},
    {"background blur, 50% opacity, radius 31", "composite-background-blur-50-r31.png",
        BaseLayerSharedFlag::BackgroundBlur, 31, {}, {},
        BaseLayerCommonStyleUniform{}
            .setSmoothness(1.0f),
        BaseLayerStyleUniform{}
            .setCornerRadius(12.0f)
            /* Premultiplied alpha */
            .setColor(0xffffffff_rgbaf*0.5f),
        0.0f, 0.0f},
    /* This shouldn't make any visible difference to the above but is using
       considerably less samples */
    {"background blur, 50% opacity, radius 31, cutoff 0.5/255", "composite-background-blur-50-r31.png",
        BaseLayerSharedFlag::BackgroundBlur, 31, 0.5f/255.0f, {},
        BaseLayerCommonStyleUniform{}
            .setSmoothness(1.0f),
        BaseLayerStyleUniform{}
            .setCornerRadius(12.0f)
            /* Premultiplied alpha */
            .setColor(0xffffffff_rgbaf*0.5f),
        0.0f, 0.0f},
    /* This should again look the same as if no compositing is done as all the
       extra samples get discarded due to being less than the cutoff In reality
       there's a slight difference possibly due to the blend operation being
       done a bit differently? */
    {"background blur, 50% opacity, radius 31, cutoff 1", "composite-default-50.png",
        BaseLayerSharedFlag::BackgroundBlur, 31, 1.0f, {},
        BaseLayerCommonStyleUniform{}
            .setSmoothness(1.0f),
        BaseLayerStyleUniform{}
            .setCornerRadius(12.0f)
            /* Premultiplied alpha */
            .setColor(0xffffffff_rgbaf*0.5f),
        0.75f, 0.247f},
    {"background blur, 50% opacity, radius 31, 80% blur opacity", "composite-background-blur-50-r31-80.png",
        BaseLayerSharedFlag::BackgroundBlur, 31, {}, {},
        BaseLayerCommonStyleUniform{}
            .setSmoothness(1.0f)
            .setBackgroundBlurAlpha(0.8f),
        BaseLayerStyleUniform{}
            .setCornerRadius(12.0f)
            /* Premultiplied alpha */
            .setColor(0xffffffff_rgbaf*0.5f),
        0.0f, 0.0f},
    /* This should again look the same as if no compositing is done, as the
       blurred background contributes in no way to the output */
    {"background blur, 50% opacity, radius 31, 0% blur opacity", "composite-default-50.png",
        BaseLayerSharedFlag::BackgroundBlur, 31, {}, {},
        BaseLayerCommonStyleUniform{}
            .setSmoothness(1.0f)
            .setBackgroundBlurAlpha(0.0f),
        BaseLayerStyleUniform{}
            .setCornerRadius(12.0f)
            /* Premultiplied alpha */
            .setColor(0xffffffff_rgbaf*0.5f),
        0.0f, 0.0f},
};

const struct {
    const char* name;
    Float scale;
    bool setSizeLater;
} RenderCompositeEdgeSmoothnessData[]{
    {"", 1.0f, false},
    {"UI 100x larger than framebuffer",
        100.0f, false},
    {"UI 100x larger than framebuffer, set later",
        100.0f, true},
    {"UI 100x smaller than framebuffer",
        0.01f, false},
    {"UI 100x smaller than framebuffer, set later",
        0.01f, true},
};

const struct {
    const char* name;
    const char* textureFilename;
    const char* expectedFilename;
    Vector3 offset;
    Vector2 size;
    BaseLayerSharedFlags extraFlags;
    BaseLayerStyleUniform styleItem;
} RenderCompositeTexturedData[]{
    /* Transparent areas are blurred by default as well */
    {"background blur, 50% opacity, radius 31, alpha mask",
        "mask-premultiplied.png", "composite-background-blur-50-r31-mask-default.png",
        /* The image is 128x64, rendering the actually visible 112x48 part of
           it to avoid nasty scaling */
        {8.0f/128.0f, 8.0f/64.0f, 7}, {112.0f/128.0f, 48.0f/64.0f}, {},
        BaseLayerStyleUniform{}
            .setCornerRadius(12.0f)
            /* Premultiplied alpha */
            .setColor(0xffffffff_rgbaf*0.5f)},
    {"background blur, 50% opacity, radius 31, alpha mask, TextureMask",
        "mask-premultiplied.png", "composite-background-blur-50-r31-mask-mask.png",
        {8.0f/128.0f, 8.0f/64.0f, 7}, {112.0f/128.0f, 48.0f/64.0f},
        BaseLayerSharedFlag::TextureMask,
        BaseLayerStyleUniform{}
            .setCornerRadius(12.0f)
            /* Premultiplied alpha */
            .setColor(0xffffffff_rgbaf*0.5f)},
    {"background blur, 50% opacity, radius 31, alpha mask, colored",
        "mask-colored-premultiplied.png", "composite-background-blur-50-r31-mask-colored-default.png",
        {8.0f/128.0f, 8.0f/64.0f, 7}, {112.0f/128.0f, 48.0f/64.0f}, {},
        BaseLayerStyleUniform{}
            .setCornerRadius(12.0f)
            /* Premultiplied alpha */
            .setColor(0xffffffff_rgbaf*0.5f)},
    {"background blur, 50% opacity, radius 31, alpha mask, colored, TextureMask",
        "mask-colored-premultiplied.png", "composite-background-blur-50-r31-mask-colored-mask.png",
        {8.0f/128.0f, 8.0f/64.0f, 7}, {112.0f/128.0f, 48.0f/64.0f},
        BaseLayerSharedFlag::TextureMask,
        BaseLayerStyleUniform{}
            .setCornerRadius(12.0f)
            /* Premultiplied alpha */
            .setColor(0xffffffff_rgbaf*0.5f)},
    /* Just to verify the outline is masked in this case as well, there should
       be no interaction between these two features in the shader code tho */
    {"background blur, 50% opacity, radius 31, alpha mask, outline, TextureMask",
        "mask-premultiplied.png", "composite-background-blur-50-r31-mask-outline-mask.png",
        {8.0f/128.0f, 8.0f/64.0f, 7}, {112.0f/128.0f, 48.0f/64.0f},
        BaseLayerSharedFlag::TextureMask,
        BaseLayerStyleUniform{}
            .setCornerRadius(12.0f)
            .setInnerOutlineCornerRadius(4.0f)
            .setOutlineWidth(8.0f)
            /* Premultiplied alpha */
            .setColor(0xffffffff_rgbaf*0.5f)
            .setOutlineColor(0x2f83ccff_rgbaf*0.667f)},
};

const struct {
    const char* name;
    const char* filename;
    Vector2 uiScale;
    UnsignedInt backgroundBlurRadius, backgroundBlurPassCount;
    Float maxThreshold, meanThreshold;
} RenderCompositeNodeRectsData[]{
    {"radius 0", "composite-node-rects-background-blur-r0.png",
        {1.0f, 1.0f}, 0, 1, 0.0f, 0.0f},
    {"radius 0, UI size different from framebuffer", "composite-node-rects-background-blur-r0.png",
        {0.1f, 10.0f}, 0, 1, 0.0f, 0.0f},
    /* Small radius to verify the compositing node area is correctly expanded.
       If it wouldn't be, the cyan background would shine through. */
    {"radius 1", "composite-node-rects-background-blur-r1.png",
        {1.0f, 1.0f}, 1, 1, 0.0f, 0.0f},
    {"radius 1, UI size different from framebuffer", "composite-node-rects-background-blur-r1.png",
        {0.1f, 10.0f}, 1, 1, 0.0f, 0.0f},
    {"radius 30", "composite-node-rects-background-blur-r30.png",
        {1.0f, 1.0f}, 30, 1, 0.0f, 0.0f},
    {"radius 30, UI size different from framebuffer", "composite-node-rects-background-blur-r30.png",
        {0.1f, 10.0f}, 30, 1, 0.0f, 0.0f},
    /* Should look roughly the same (minus rounding errors) with no apparent
       edge artifacts caused by not including pass count into the padding */
    {"radius 15, 4 passes", "composite-node-rects-background-blur-r30.png",
        {1.0f, 1.0f}, 15, 4, 6.25f, 0.753f},
};

const struct {
    const char* name;
    bool dataInNodeOrder;
} DrawOrderData[]{
    {"data created in node order", true},
    {"data created randomly", false}
};

const struct {
    const char* name;
    const char* filename;
    BaseLayerSharedFlags flags;
} DrawOrderCompositeData[]{
    {"default", "draw-order-composite-default.png",
        {}},
    {"background blur", "draw-order-composite-background-blur.png",
        BaseLayerSharedFlag::BackgroundBlur}
};

const struct {
    const char* name;
    const char* filename;
    bool clip;
    bool singleTopLevel;
    bool flipOrder;
} DrawClippingData[]{
    {"clipping disabled", "clipping-disabled.png",
        false, false, false},
    {"clipping top-level nodes", "clipping-enabled.png",
        true, false, false},
    {"clipping top-level nodes, different node order", "clipping-enabled.png",
        true, false, true},
    {"single top-level node with clipping subnodes", "clipping-enabled.png",
        true, true, false},
};

BaseLayerGLTest::BaseLayerGLTest() {
    addTests({&BaseLayerGLTest::sharedConstruct,
              &BaseLayerGLTest::sharedConstructComposite,
              &BaseLayerGLTest::sharedConstructCopy,
              &BaseLayerGLTest::sharedConstructMove,

              &BaseLayerGLTest::construct,
              &BaseLayerGLTest::constructDerived,
              &BaseLayerGLTest::constructCopy,
              &BaseLayerGLTest::constructMove,

              &BaseLayerGLTest::setTextureTexturingNotEnabled,

              &BaseLayerGLTest::drawNoSizeSet});

    addInstancedTests({&BaseLayerGLTest::drawNoStyleSet},
        Containers::arraySize(DrawNoStyleSetData));

    addTests({&BaseLayerGLTest::drawNoTextureSet});

    /* MSVC needs explicit type due to default template args */
    addInstancedTests<BaseLayerGLTest>({
        &BaseLayerGLTest::render,
        &BaseLayerGLTest::render<BaseLayerSharedFlag::SubdividedQuads>},
        Containers::arraySize(RenderData),
        &BaseLayerGLTest::renderSetup,
        &BaseLayerGLTest::renderTeardown);

    addInstancedTests<BaseLayerGLTest>({
        &BaseLayerGLTest::renderCustomColor,
        &BaseLayerGLTest::renderCustomColor<BaseLayerSharedFlag::SubdividedQuads>,
        &BaseLayerGLTest::renderCustomOutlineWidth,
        &BaseLayerGLTest::renderCustomOutlineWidth<BaseLayerSharedFlag::SubdividedQuads>},
        Containers::arraySize(RenderCustomColorOutlineWidthData),
        &BaseLayerGLTest::renderSetup,
        &BaseLayerGLTest::renderTeardown);

    addInstancedTests<BaseLayerGLTest>({
        &BaseLayerGLTest::renderPadding,
        &BaseLayerGLTest::renderPadding<BaseLayerSharedFlag::SubdividedQuads>},
        Containers::arraySize(RenderPaddingData),
        &BaseLayerGLTest::renderSetup,
        &BaseLayerGLTest::renderTeardown);

    addInstancedTests<BaseLayerGLTest>({
        &BaseLayerGLTest::renderChangeStyle,
        &BaseLayerGLTest::renderChangeStyle<BaseLayerSharedFlag::SubdividedQuads>},
        Containers::arraySize(RenderChangeStyleData),
        &BaseLayerGLTest::renderSetup,
        &BaseLayerGLTest::renderTeardown);

    addInstancedTests<BaseLayerGLTest>({
        &BaseLayerGLTest::renderTextured,
        &BaseLayerGLTest::renderTextured<BaseLayerSharedFlag::SubdividedQuads>},
        Containers::arraySize(RenderTexturedData),
        &BaseLayerGLTest::renderSetup,
        &BaseLayerGLTest::renderTeardown);

    addInstancedTests<BaseLayerGLTest>({
        &BaseLayerGLTest::renderOutlineEdgeSmoothness,
        &BaseLayerGLTest::renderOutlineEdgeSmoothness<BaseLayerSharedFlag::SubdividedQuads>},
        Containers::arraySize(RenderOutlineEdgeSmoothnessData),
        &BaseLayerGLTest::renderSetup,
        &BaseLayerGLTest::renderTeardown);

    addInstancedTests<BaseLayerGLTest>({
        &BaseLayerGLTest::renderGradientOutlineEdgeSmoothness,
        &BaseLayerGLTest::renderGradientOutlineEdgeSmoothness<BaseLayerSharedFlag::SubdividedQuads>},
        Containers::arraySize(RenderGradientOutlineEdgeSmoothnessData),
        &BaseLayerGLTest::renderSetup,
        &BaseLayerGLTest::renderTeardown);

    addInstancedTests<BaseLayerGLTest>({
        &BaseLayerGLTest::renderTexturedOutlineEdgeSmoothness,
        &BaseLayerGLTest::renderTexturedOutlineEdgeSmoothness<BaseLayerSharedFlag::SubdividedQuads>},
        Containers::arraySize(RenderTexturedOutlineEdgeSmoothnessData),
        &BaseLayerGLTest::renderSetup,
        &BaseLayerGLTest::renderTeardown);

    addInstancedTests<BaseLayerGLTest>({
        &BaseLayerGLTest::renderDynamicStyles,
        &BaseLayerGLTest::renderDynamicStyles<BaseLayerSharedFlag::SubdividedQuads>},
        Containers::arraySize(RenderDynamicStylesData),
        &BaseLayerGLTest::renderSetup,
        &BaseLayerGLTest::renderTeardown);

    addInstancedTests<BaseLayerGLTest>({
        &BaseLayerGLTest::renderComposite,
        &BaseLayerGLTest::renderComposite<BaseLayerSharedFlag::SubdividedQuads>},
        Containers::arraySize(RenderCompositeData),
        &BaseLayerGLTest::renderOrDrawCompositeSetup,
        &BaseLayerGLTest::renderOrDrawCompositeTeardown);

    addInstancedTests<BaseLayerGLTest>({
        &BaseLayerGLTest::renderCompositeEdgeSmoothness,
        &BaseLayerGLTest::renderCompositeEdgeSmoothness<BaseLayerSharedFlag::SubdividedQuads>},
        Containers::arraySize(RenderCompositeEdgeSmoothnessData),
        &BaseLayerGLTest::renderOrDrawCompositeSetup,
        &BaseLayerGLTest::renderOrDrawCompositeTeardown);

    addInstancedTests<BaseLayerGLTest>({
        &BaseLayerGLTest::renderCompositeTextured,
        &BaseLayerGLTest::renderCompositeTextured<BaseLayerSharedFlag::SubdividedQuads>},
        Containers::arraySize(RenderCompositeTexturedData),
        &BaseLayerGLTest::renderOrDrawCompositeSetup,
        &BaseLayerGLTest::renderOrDrawCompositeTeardown);

    addInstancedTests({&BaseLayerGLTest::renderCompositeNodeRects},
        Containers::arraySize(RenderCompositeNodeRectsData),
        &BaseLayerGLTest::renderOrDrawCompositeSetup,
        &BaseLayerGLTest::renderOrDrawCompositeTeardown);

    addInstancedTests({&BaseLayerGLTest::drawOrder},
        Containers::arraySize(DrawOrderData),
        &BaseLayerGLTest::drawSetup,
        &BaseLayerGLTest::drawTeardown);

    addInstancedTests({&BaseLayerGLTest::drawOrderComposite},
        Containers::arraySize(DrawOrderData),
        &BaseLayerGLTest::renderOrDrawCompositeSetup,
        &BaseLayerGLTest::renderOrDrawCompositeTeardown);

    addInstancedTests({&BaseLayerGLTest::drawClipping},
        Containers::arraySize(DrawClippingData),
        &BaseLayerGLTest::drawSetup,
        &BaseLayerGLTest::drawTeardown);

    addTests({&BaseLayerGLTest::eventStyleTransition},
        &BaseLayerGLTest::renderSetup,
        &BaseLayerGLTest::renderTeardown);

    /* Prefer the StbImageImporter so we can keep files small but always import
       them as four-channel */
    if(PluginManager::PluginMetadata* metadata = _manager.metadata("StbImageImporter")) {
        metadata->configuration().setValue("forceChannelCount", 4);
        _manager.setPreferredPlugins("PngImporter", {"StbImageImporter"});
    }
}

void BaseLayerGLTest::sharedConstruct() {
    BaseLayerGL::Shared shared{BaseLayer::Shared::Configuration{3, 5}};
    CORRADE_COMPARE(shared.styleUniformCount(), 3);
    CORRADE_COMPARE(shared.styleCount(), 5);
    CORRADE_COMPARE(shared.flags(), BaseLayerSharedFlags{});
}

void BaseLayerGLTest::sharedConstructComposite() {
    BaseLayerGL::Shared shared{
        BaseLayerGL::Shared::Configuration{3, 5}
            .addFlags(BaseLayerSharedFlag::BackgroundBlur)};
    CORRADE_COMPARE(shared.styleUniformCount(), 3);
    CORRADE_COMPARE(shared.styleCount(), 5);
    CORRADE_COMPARE(shared.flags(), BaseLayerSharedFlag::BackgroundBlur);
}

void BaseLayerGLTest::sharedConstructCopy() {
    CORRADE_VERIFY(!std::is_copy_constructible<BaseLayerGL::Shared>{});
    CORRADE_VERIFY(!std::is_copy_assignable<BaseLayerGL::Shared>{});
}

void BaseLayerGLTest::sharedConstructMove() {
    BaseLayerGL::Shared a{BaseLayer::Shared::Configuration{3}};

    BaseLayerGL::Shared b{Utility::move(a)};
    CORRADE_COMPARE(b.styleCount(), 3);

    BaseLayerGL::Shared c{BaseLayer::Shared::Configuration{5}};
    c = Utility::move(b);
    CORRADE_COMPARE(c.styleCount(), 3);

    CORRADE_VERIFY(std::is_nothrow_move_constructible<BaseLayerGL::Shared>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<BaseLayerGL::Shared>::value);
}

void BaseLayerGLTest::construct() {
    BaseLayerGL::Shared shared{BaseLayer::Shared::Configuration{3}};

    BaseLayerGL layer{layerHandle(137, 0xfe), shared};
    CORRADE_COMPARE(layer.handle(), layerHandle(137, 0xfe));
    CORRADE_COMPARE(&layer.shared(), &shared);
    /* Const overload */
    CORRADE_COMPARE(&static_cast<const BaseLayerGL&>(layer).shared(), &shared);
}

void BaseLayerGLTest::constructDerived() {
    BaseLayerGL::Shared shared{BaseLayer::Shared::Configuration{3}};

    /* Verify just that subclassing works without hitting linker errors due to
       virtual symbols not being exported or due to the delegated-to functions
       being private */
    struct: BaseLayerGL {
        using BaseLayerGL::BaseLayerGL;

        void doComposite(AbstractRenderer& renderer, const Containers::StridedArrayView1D<const Vector2>& compositeRectOffsets, const Containers::StridedArrayView1D<const Vector2>& compositeRectSizes, std::size_t offset, std::size_t count) override {
            return BaseLayerGL::doComposite(renderer, compositeRectOffsets, compositeRectSizes, offset, count);
        }

        void doDraw(const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, std::size_t offset, std::size_t count, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, std::size_t clipRectOffset, std::size_t clipRectCount, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, Containers::BitArrayView nodesEnabled, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes) override {
            return BaseLayerGL::doDraw(dataIds, offset, count, clipRectIds, clipRectDataCounts, clipRectOffset, clipRectCount, nodeOffsets, nodeSizes, nodesEnabled, clipRectOffsets, clipRectSizes);
        }
    } layer{layerHandle(137, 0xfe), shared};
    CORRADE_COMPARE(layer.handle(), layerHandle(137, 0xfe));
}

void BaseLayerGLTest::constructCopy() {
    CORRADE_VERIFY(!std::is_copy_constructible<BaseLayerGL>{});
    CORRADE_VERIFY(!std::is_copy_assignable<BaseLayerGL>{});
}

void BaseLayerGLTest::constructMove() {
    BaseLayerGL::Shared shared{BaseLayer::Shared::Configuration{3}};
    BaseLayerGL::Shared shared2{BaseLayer::Shared::Configuration{5}};

    BaseLayerGL a{layerHandle(137, 0xfe), shared};

    BaseLayerGL b{Utility::move(a)};
    CORRADE_COMPARE(b.handle(), layerHandle(137, 0xfe));
    CORRADE_COMPARE(&b.shared(), &shared);

    BaseLayerGL c{layerHandle(0, 2), shared2};
    c = Utility::move(b);
    CORRADE_COMPARE(c.handle(), layerHandle(137, 0xfe));
    CORRADE_COMPARE(&c.shared(), &shared);

    CORRADE_VERIFY(std::is_nothrow_move_constructible<BaseLayerGL>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<BaseLayerGL>::value);
}

void BaseLayerGLTest::setTextureTexturingNotEnabled() {
    CORRADE_SKIP_IF_NO_ASSERT();

    BaseLayerGL::Shared shared{BaseLayer::Shared::Configuration{1}};

    BaseLayerGL layer{layerHandle(137, 0xfe), shared};

    GL::Texture2DArray texture;

    std::ostringstream out;
    Error redirectError{&out};
    layer.setTexture(texture);
    layer.setTexture(GL::Texture2DArray{}); /* R-value overload */
    CORRADE_COMPARE(out.str(),
        "Whee::BaseLayerGL::setTexture(): texturing not enabled\n"
        "Whee::BaseLayerGL::setTexture(): texturing not enabled\n");
}

void BaseLayerGLTest::drawNoSizeSet() {
    CORRADE_SKIP_IF_NO_ASSERT();

    BaseLayerGL::Shared shared{BaseLayer::Shared::Configuration{3}};
    BaseLayerGL layer{layerHandle(0, 1), shared};

    std::ostringstream out;
    Error redirectError{&out};
    layer.draw({}, 0, 0, {}, {}, 0, 0, {}, {}, {}, {}, {});
    CORRADE_COMPARE(out.str(), "Whee::BaseLayerGL::draw(): user interface size wasn't set\n");
}

void BaseLayerGLTest::drawNoStyleSet() {
    auto&& data = DrawNoStyleSetData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    CORRADE_SKIP_IF_NO_ASSERT();

    BaseLayerGL::Shared shared{BaseLayer::Shared::Configuration{3}
        .setDynamicStyleCount(data.dynamicStyleCount)};
    BaseLayerGL layer{layerHandle(0, 1), shared};

    layer.setSize({10, 10}, {10, 10});

    std::ostringstream out;
    Error redirectError{&out};
    layer.draw({}, 0, 0, {}, {}, 0, 0, {}, {}, {}, {}, {});
    CORRADE_COMPARE(out.str(), "Whee::BaseLayerGL::draw(): no style data was set\n");
}

void BaseLayerGLTest::drawNoTextureSet() {
    CORRADE_SKIP_IF_NO_ASSERT();

    BaseLayerGL::Shared shared{BaseLayer::Shared::Configuration{1}
        .addFlags(BaseLayerSharedFlag::Textured)};
    shared.setStyle(
        BaseLayerCommonStyleUniform{},
        {BaseLayerStyleUniform{}},
        {});

    BaseLayerGL layer{layerHandle(0, 1), shared};
    layer.setSize({10, 10}, {10, 10});

    std::ostringstream out;
    Error redirectError{&out};
    layer.draw({}, 0, 0, {}, {}, 0, 0, {}, {}, {}, {}, {});
    CORRADE_COMPARE(out.str(), "Whee::BaseLayerGL::draw(): no texture to draw with was set\n");
}

constexpr Vector2i RenderSize{128, 64};

void BaseLayerGLTest::renderSetup() {
    _color = GL::Texture2D{};
    _color.setStorage(1, GL::TextureFormat::RGBA8, RenderSize);
    _framebuffer = GL::Framebuffer{{{}, RenderSize}};
    _framebuffer
        .attachTexture(GL::Framebuffer::ColorAttachment{0}, _color, 0)
        .clear(GL::FramebufferClear::Color)
        .bind();

    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::One, GL::Renderer::BlendFunction::OneMinusSourceAlpha);
    /* The RendererGL should enable these on its own if needed */
    GL::Renderer::disable(GL::Renderer::Feature::ScissorTest);
    GL::Renderer::disable(GL::Renderer::Feature::Blending);
}

void BaseLayerGLTest::renderTeardown() {
    _framebuffer = GL::Framebuffer{NoCreate};
    _color = GL::Texture2D{NoCreate};

    GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::disable(GL::Renderer::Feature::ScissorTest);
    GL::Renderer::disable(GL::Renderer::Feature::Blending);
}

template<BaseLayerSharedFlag flag> void BaseLayerGLTest::render() {
    auto&& data = RenderData[testCaseInstanceId()];
    setTestCaseDescription(data.name);
    setTestCaseTemplateName(flag == BaseLayerSharedFlag::SubdividedQuads ? "Flag::SubdividedQuads" : "");

    if(flag == BaseLayerSharedFlag::SubdividedQuads && (data.flags & (BaseLayerSharedFlag::NoOutline|BaseLayerSharedFlag::NoRoundedCorners)))
        CORRADE_SKIP(flag << "and" << data.flags << "are mutually exclusive");

    AbstractUserInterface ui{RenderSize};
    ui.setRendererInstance(Containers::pointer<RendererGL>());

    /* Testing the ArrayView overload, others cases use the initializer list */
    BaseLayerStyleUniform styleUniforms[]{
        /* To verify it's not always picking the first uniform */
        BaseLayerStyleUniform{},
        BaseLayerStyleUniform{},
        data.styleUniform,
    };
    UnsignedInt styleToUniform[]{
        /* To verify it's not using the style ID as uniform ID */
        1, 2, 0, 1, 0
    };
    BaseLayerGL::Shared layerShared{BaseLayer::Shared::Configuration{
        UnsignedInt(Containers::arraySize(styleUniforms)),
        UnsignedInt(Containers::arraySize(styleToUniform))}
            .setFlags(data.flags|flag)
    };
    /* The (lack of any) effect of padding on rendered output is tested
       thoroughly in renderPadding() */
    layerShared.setStyle(data.styleUniformCommon,
        styleUniforms,
        styleToUniform,
        {});

    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<BaseLayerGL>(layer, layerShared));

    NodeHandle node = ui.createNode({8.0f, 8.0f}, {112.0f, 48.0f});
    ui.layer<BaseLayerGL>(layer).create(1, node);

    ui.draw();

    MAGNUM_VERIFY_NO_GL_ERROR();

    if(!(_manager.load("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_manager.load("StbImageImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / StbImageImporter plugins not found.");

    #if defined(MAGNUM_TARGET_GLES) && !defined(MAGNUM_TARGET_WEBGL)
    /* Same problem is with all builtin shaders, so this doesn't seem to be a
       bug in the base layer shader code */
    if(GL::Context::current().detectedDriver() & GL::Context::DetectedDriver::SwiftShader)
        CORRADE_SKIP("UBOs with dynamically indexed arrays don't seem to work on SwiftShader, can't test.");
    #endif
    CORRADE_COMPARE_WITH(_framebuffer.read({{}, RenderSize}, {PixelFormat::RGBA8Unorm}),
        Utility::Path::join({WHEE_TEST_DIR, "BaseLayerTestFiles", data.filename}),
        DebugTools::CompareImageToFile{_manager});
}

template<BaseLayerSharedFlag flag> void BaseLayerGLTest::renderCustomColor() {
    auto&& data = RenderCustomColorOutlineWidthData[testCaseInstanceId()];
    setTestCaseDescription(data.name);
    setTestCaseTemplateName(flag == BaseLayerSharedFlag::SubdividedQuads ? "Flag::SubdividedQuads" : "");

    /* Basically the same as the "gradient" case in render(), except that the
       color is additionally taken from the per-vertex data as well */

    AbstractUserInterface ui{RenderSize};
    ui.setRendererInstance(Containers::pointer<RendererGL>());

    BaseLayerGL::Shared layerShared{BaseLayer::Shared::Configuration{1}
        .addFlags(flag)
    };
    layerShared.setStyle(BaseLayerCommonStyleUniform{}, {
        BaseLayerStyleUniform{}
            .setColor(0xeeddaa_rgbf/0x336699_rgbf, 0x774422_rgbf/0x336699_rgbf)
    }, {});

    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<BaseLayerGL>(layer, layerShared));

    NodeHandle node = ui.createNode({8.0f, 8.0f}, {112.0f, 48.0f});
    DataHandle nodeData = data.setLater ?
        ui.layer<BaseLayerGL>(layer).create(0, node) :
        ui.layer<BaseLayerGL>(layer).create(0, 0x336699_rgbf, node);

    if(data.partialUpdate) {
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    if(data.setLater) {
        ui.layer<BaseLayerGL>(layer).setColor(nodeData, 0x336699_rgbf);
        CORRADE_COMPARE_AS(ui.state(),
            UserInterfaceState::NeedsDataUpdate,
            TestSuite::Compare::GreaterOrEqual);
    }

    ui.draw();

    MAGNUM_VERIFY_NO_GL_ERROR();

    if(!(_manager.load("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_manager.load("StbImageImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / StbImageImporter plugins not found.");

    #if defined(MAGNUM_TARGET_GLES) && !defined(MAGNUM_TARGET_WEBGL)
    /* Same problem is with all builtin shaders, so this doesn't seem to be a
       bug in the base layer shader code */
    if(GL::Context::current().detectedDriver() & GL::Context::DetectedDriver::SwiftShader)
        CORRADE_SKIP("UBOs with dynamically indexed arrays don't seem to work on SwiftShader, can't test.");
    #endif
    CORRADE_COMPARE_WITH(_framebuffer.read({{}, RenderSize}, {PixelFormat::RGBA8Unorm}),
        Utility::Path::join(WHEE_TEST_DIR, "BaseLayerTestFiles/gradient.png"),
        DebugTools::CompareImageToFile{_manager});
}

template<BaseLayerSharedFlag flag> void BaseLayerGLTest::renderCustomOutlineWidth() {
    auto&& data = RenderCustomColorOutlineWidthData[testCaseInstanceId()];
    setTestCaseDescription(data.name);
    setTestCaseTemplateName(flag == BaseLayerSharedFlag::SubdividedQuads ? "Flag::SubdividedQuads" : "");

    if(flag == BaseLayerSharedFlag::SubdividedQuads && (data.flags & (BaseLayerSharedFlag::NoOutline|BaseLayerSharedFlag::NoRoundedCorners)))
        CORRADE_SKIP(flag << "and" << data.flags << "are mutually exclusive");

    /* Like the "outline, all sides same" case in render(), except that the
       width is additionally taken from the per-vertex data as well. And tests
       that the custom outline specified in the data isn't taken into account
       in any way when outlines are disabled. */

    AbstractUserInterface ui{RenderSize};
    ui.setRendererInstance(Containers::pointer<RendererGL>());

    BaseLayerGL::Shared layerShared{BaseLayer::Shared::Configuration{1}
        .setFlags(data.flags|flag)
    };
    layerShared.setStyle(BaseLayerCommonStyleUniform{}, {
        BaseLayerStyleUniform{}
            .setOutlineColor(0x7f7f7f_rgbf)
            .setOutlineWidth({16.0f, 2.0f, 4.0f, 0.0f})
    }, {});

    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<BaseLayerGL>(layer, layerShared));

    NodeHandle node = ui.createNode({8.0f, 8.0f}, {112.0f, 48.0f});
    DataHandle nodeData;
    if(data.setLater)
        nodeData = ui.layer<BaseLayerGL>(layer).create(0, node);
    else
        nodeData = ui.layer<BaseLayerGL>(layer).create(0, 0xffffff_rgbf, {-8.0f, 6.0f, 4.0f, 8.0f}, node);

    if(data.partialUpdate) {
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    if(data.setLater) {
        ui.layer<BaseLayerGL>(layer).setOutlineWidth(nodeData, {-8.0f, 6.0f, 4.0f, 8.0f});
        CORRADE_COMPARE_AS(ui.state(),
            UserInterfaceState::NeedsDataUpdate,
            TestSuite::Compare::GreaterOrEqual);
    }

    ui.draw();

    MAGNUM_VERIFY_NO_GL_ERROR();

    if(!(_manager.load("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_manager.load("StbImageImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / StbImageImporter plugins not found.");

    #if defined(MAGNUM_TARGET_GLES) && !defined(MAGNUM_TARGET_WEBGL)
    /* Same problem is with all builtin shaders, so this doesn't seem to be a
       bug in the base layer shader code */
    if(GL::Context::current().detectedDriver() & GL::Context::DetectedDriver::SwiftShader)
        CORRADE_SKIP("UBOs with dynamically indexed arrays don't seem to work on SwiftShader, can't test.");
    #endif
    CORRADE_COMPARE_WITH(_framebuffer.read({{}, RenderSize}, {PixelFormat::RGBA8Unorm}),
        Utility::Path::join({WHEE_TEST_DIR, "BaseLayerTestFiles", data.filename}),
        DebugTools::CompareImageToFile{_manager});
}

template<BaseLayerSharedFlag flag> void BaseLayerGLTest::renderPadding() {
    auto&& data = RenderPaddingData[testCaseInstanceId()];
    setTestCaseDescription(data.name);
    setTestCaseTemplateName(flag == BaseLayerSharedFlag::SubdividedQuads ? "Flag::SubdividedQuads" : "");

    /* Basically the same as the "outline, rounded corners, different" case in
       render(), except that the node offset, size and style or data padding
       changes. The result should always be the same as if the padding was
       applied directly to the node offset and size itself. */

    AbstractUserInterface ui{RenderSize};
    ui.setRendererInstance(Containers::pointer<RendererGL>());

    BaseLayerGL::Shared layerShared{BaseLayer::Shared::Configuration{1}
        .addFlags(flag)
    };
    layerShared.setStyle(
        BaseLayerCommonStyleUniform{}
            .setSmoothness(1.0f),
        {BaseLayerStyleUniform{}
            .setOutlineColor(0x7f7f7f_rgbf)
            /* Top left, bottom left, top right, bottom right */
            .setCornerRadius({36.0f, 12.0f, 4.0f, 0.0f})
            .setInnerOutlineCornerRadius({18.0f, 6.0f, 0.0f, 18.0f})
            /* Left, top, right, bottom  */
            .setOutlineWidth({18.0f, 8.0f, 0.0f, 4.0f})},
        {data.paddingFromStyle});

    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<BaseLayerGL>(layer, layerShared));

    NodeHandle node = ui.createNode(data.nodeOffset, data.nodeSize);
    DataHandle nodeData = ui.layer<BaseLayerGL>(layer).create(0, node);

    if(data.partialUpdate) {
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    if(!data.paddingFromData.isZero()) {
        ui.layer<BaseLayerGL>(layer).setPadding(nodeData, data.paddingFromData);
        CORRADE_COMPARE_AS(ui.state(),
            UserInterfaceState::NeedsDataUpdate,
            TestSuite::Compare::GreaterOrEqual);
    }

    ui.draw();

    MAGNUM_VERIFY_NO_GL_ERROR();

    if(!(_manager.load("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_manager.load("StbImageImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / StbImageImporter plugins not found.");

    #if defined(MAGNUM_TARGET_GLES) && !defined(MAGNUM_TARGET_WEBGL)
    /* Same problem is with all builtin shaders, so this doesn't seem to be a
       bug in the base layer shader code */
    if(GL::Context::current().detectedDriver() & GL::Context::DetectedDriver::SwiftShader)
        CORRADE_SKIP("UBOs with dynamically indexed arrays don't seem to work on SwiftShader, can't test.");
    #endif
    CORRADE_COMPARE_WITH(_framebuffer.read({{}, RenderSize}, {PixelFormat::RGBA8Unorm}),
        Utility::Path::join(WHEE_TEST_DIR, "BaseLayerTestFiles/outline-rounded-corners-both-different.png"),
        DebugTools::CompareImageToFile{_manager});
}

template<BaseLayerSharedFlag flag> void BaseLayerGLTest::renderChangeStyle() {
    auto&& data = RenderChangeStyleData[testCaseInstanceId()];
    setTestCaseDescription(data.name);
    setTestCaseTemplateName(flag == BaseLayerSharedFlag::SubdividedQuads ? "Flag::SubdividedQuads" : "");

    /* Basically the same as the "gradient" case in render(), except that the
       style ID is changed to it only later. */

    AbstractUserInterface ui{RenderSize};
    ui.setRendererInstance(Containers::pointer<RendererGL>());

    BaseLayerGL::Shared layerShared{BaseLayer::Shared::Configuration{2}
        .addFlags(flag)
    };
    layerShared.setStyle(BaseLayerCommonStyleUniform{}, {
        BaseLayerStyleUniform{},
        BaseLayerStyleUniform{}
            .setColor(0xeeddaa_rgbf, 0x774422_rgbf)
    }, {});

    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<BaseLayerGL>(layer, layerShared));

    NodeHandle node = ui.createNode({8.0f, 8.0f}, {112.0f, 48.0f});
    DataHandle nodeData = ui.layer<BaseLayerGL>(layer).create(0, node);

    if(data.partialUpdate) {
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    ui.layer<BaseLayerGL>(layer).setStyle(nodeData, 1);
    CORRADE_COMPARE_AS(ui.state(),
        UserInterfaceState::NeedsDataUpdate,
        TestSuite::Compare::GreaterOrEqual);

    ui.draw();

    MAGNUM_VERIFY_NO_GL_ERROR();

    if(!(_manager.load("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_manager.load("StbImageImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / StbImageImporter plugins not found.");

    #if defined(MAGNUM_TARGET_GLES) && !defined(MAGNUM_TARGET_WEBGL)
    /* Same problem is with all builtin shaders, so this doesn't seem to be a
       bug in the base layer shader code */
    if(GL::Context::current().detectedDriver() & GL::Context::DetectedDriver::SwiftShader)
        CORRADE_SKIP("UBOs with dynamically indexed arrays don't seem to work on SwiftShader, can't test.");
    #endif
    CORRADE_COMPARE_WITH(_framebuffer.read({{}, RenderSize}, {PixelFormat::RGBA8Unorm}),
        Utility::Path::join(WHEE_TEST_DIR, "BaseLayerTestFiles/gradient.png"),
        DebugTools::CompareImageToFile{_manager});
}

template<BaseLayerSharedFlag flag> void BaseLayerGLTest::renderTextured() {
    auto&& data = RenderTexturedData[testCaseInstanceId()];
    setTestCaseDescription(data.name);
    setTestCaseTemplateName(flag == BaseLayerSharedFlag::SubdividedQuads ? "Flag::SubdividedQuads" : "");

    if(!(_manager.load("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_manager.load("StbImageImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / StbImageImporter plugins not found.");

    Containers::Pointer<Trade::AbstractImporter> importer = _manager.loadAndInstantiate("AnyImageImporter");
    CORRADE_VERIFY(importer->openFile(Utility::Path::join({WHEE_TEST_DIR, "BaseLayerTestFiles", data.textureFilename})));

    Containers::Optional<Trade::ImageData2D> image = importer->image2D(0);
    CORRADE_VERIFY(image);

    GL::Texture2DArray texture;
    texture
        .setMinificationFilter(GL::SamplerFilter::Linear)
        .setMagnificationFilter(GL::SamplerFilter::Linear)
        .setWrapping(GL::SamplerWrapping::ClampToEdge)
        .setStorage(1, GL::textureFormat(image->format()), Vector3i{image->size(), 8})
        .setSubImage(0, {0, 0, data.offset ? Int(data.offset->z()) : 0}, ImageView2D{*image});

    AbstractUserInterface ui{RenderSize};
    ui.setRendererInstance(Containers::pointer<RendererGL>());

    BaseLayerGL::Shared layerShared{
        BaseLayer::Shared::Configuration{2}
            .addFlags(BaseLayerSharedFlag::Textured|data.extraFlags|flag)};
    layerShared.setStyle(
        BaseLayerCommonStyleUniform{}
            .setSmoothness(1.0f),
        /* To verify it's not always picking the first uniform */
        {BaseLayerStyleUniform{}, data.styleUniform},
        {});

    BaseLayerGL& layer = ui.setLayerInstance(Containers::pointer<BaseLayerGL>(ui.createLayer(), layerShared));
    if(data.rvalue) {
        layer.setTexture(Utility::move(texture));
        CORRADE_VERIFY(!texture.id());
    } else {
        layer.setTexture(texture);
    }

    NodeHandle node = ui.createNode({8.0f, 8.0f}, {112.0f, 48.0f});
    DataHandle nodeData = layer.create(1, node);
    if(data.offset)
        layer.setTextureCoordinates(nodeData, *data.offset, *data.size);

    _framebuffer.clearColor(0, data.clearColor);

    ui.draw();

    MAGNUM_VERIFY_NO_GL_ERROR();

    if(!(_manager.load("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_manager.load("StbImageImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / StbImageImporter plugins not found.");

    #if defined(MAGNUM_TARGET_GLES) && !defined(MAGNUM_TARGET_WEBGL)
    /* Same problem is with all builtin shaders, so this doesn't seem to be a
       bug in the base layer shader code */
    if(GL::Context::current().detectedDriver() & GL::Context::DetectedDriver::SwiftShader)
        CORRADE_SKIP("UBOs with dynamically indexed arrays don't seem to work on SwiftShader, can't test.");
    #endif
    {
        CORRADE_EXPECT_FAIL_IF(flag == BaseLayerSharedFlag::SubdividedQuads && data.expectedFilename == "textured-colored.png"_s,
            "A single differing pixel with" << BaseLayerSharedFlag::SubdividedQuads << "enabled, not sure why");
        CORRADE_COMPARE_WITH(_framebuffer.read({{}, RenderSize}, {PixelFormat::RGBA8Unorm}),
            Utility::Path::join({WHEE_TEST_DIR, "BaseLayerTestFiles", data.expectedFilename}),
            DebugTools::CompareImageToFile{_manager});
    }
    /* Verify that it's indeed just that one pixel */
    if(flag == BaseLayerSharedFlag::SubdividedQuads && data.expectedFilename == "textured-colored.png"_s)
        CORRADE_COMPARE_WITH(_framebuffer.read({{}, RenderSize}, {PixelFormat::RGBA8Unorm}),
            Utility::Path::join({WHEE_TEST_DIR, "BaseLayerTestFiles", data.expectedFilename}),
            (DebugTools::CompareImageToFile{_manager, 0.25f, 0.00004f}));
}

template<BaseLayerSharedFlag flag> void BaseLayerGLTest::renderOutlineEdgeSmoothness() {
    auto&& data = RenderOutlineEdgeSmoothnessData[testCaseInstanceId()];
    setTestCaseDescription(data.name);
    setTestCaseTemplateName(flag == BaseLayerSharedFlag::SubdividedQuads ? "Flag::SubdividedQuads" : "");

    /* It should produce the same result (8 *pixel* smoothness either inside or
       outside or both) regardless of the actual UI size */

    /* Window size isn't used for anything here, can be arbitrary. If the size
       is meant to be set later, start with the framebuffer being 1x1. The UI
       size has to stay unchanged, otherwise it'll set NeedsNodeClipUpdate,
       which triggers data regeneration always. */
    AbstractUserInterface ui{data.scale*Vector2{RenderSize}, {1, 1}, (data.setSizeLater ? Vector2i{1} : RenderSize)};
    ui.setRendererInstance(Containers::pointer<RendererGL>());

    BaseLayerGL::Shared layerShared{BaseLayer::Shared::Configuration{2}
        .addFlags(flag)
    };
    layerShared.setStyle(
        BaseLayerCommonStyleUniform{}
            .setSmoothness(data.smoothness, data.innerOutlineSmoothness),
        {BaseLayerStyleUniform{}
            .setColor(0x2f83ccff_rgbaf)
            .setOutlineColor(0xc7cf2fff_rgbaf)
            .setCornerRadius(4.0f*data.scale)
            .setInnerOutlineCornerRadius(12.0f*data.scale)
            .setOutlineWidth(Vector4{0.0f, 0.0f, 0.0f, 8.0f}*data.scale),
         BaseLayerStyleUniform{}
            .setColor(0x2f83ccff_rgbaf)
            .setOutlineColor(0xc7cf2fff_rgbaf)
            .setCornerRadius(4.0f*data.scale)
            .setInnerOutlineCornerRadius(12.0f*data.scale)
            .setOutlineWidth(Vector4{0.0f, 0.0f, 8.0f, 0.0f}*data.scale)},
        {});
    BaseLayer& layer = ui.setLayerInstance(Containers::pointer<BaseLayerGL>(ui.createLayer(), layerShared));

    /* Two nodes right next to each other, with some but not all edges /
       corners having an outline. The smoothness is set excessively high.

        - The data expanding outside of the node area, instead of being cut off
        - The quads thus overlapping, although the transition still being
          visible due to the way (One, OneMinusSourceAlpha) blending works,
          where it doesn't take 100% of the background. It'd only stay a
          constant color if (One, One) would be used.
        - The outline color not leaking into the base color on the edges that
          don't have it, unless the inner smoothness is even larger */

    NodeHandle node1 = ui.createNode(Vector2{12.0f, 12.0f}*data.scale,
                                     Vector2{52.0f, 40.0f}*data.scale);
    NodeHandle node2 = ui.createNode(Vector2{64.0f, 12.0f}*data.scale,
                                     Vector2{52.0f, 40.0f}*data.scale);
    layer.create(0, node1);
    layer.create(1, node2);

    if(data.setSizeLater) {
        /* Make sure everything is already processed before updating the size,
           otherwise it'd be all deferred to draw() below, circumventing what
           we want to test */
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});

        /* Setting the size should correctly regenerate whatever needs to be
           regenerated (except for SubdividedQuads, where it's all done in the
           shader) in order to adapt to the new DPI scaling. All state flag
           possibilities are tested in BaseLayerTest::setSize(). */
        ui.setSize(data.scale*Vector2{RenderSize}, {1, 1}, RenderSize);
        CORRADE_COMPARE(ui.state(), flag == BaseLayerSharedFlag::SubdividedQuads ? UserInterfaceStates{} : UserInterfaceState::NeedsDataUpdate);
    }

    ui.draw();

    MAGNUM_VERIFY_NO_GL_ERROR();

    if(!(_manager.load("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_manager.load("StbImageImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / StbImageImporter plugins not found.");

    #if defined(MAGNUM_TARGET_GLES) && !defined(MAGNUM_TARGET_WEBGL)
    /* Same problem is with all builtin shaders, so this doesn't seem to be a
       bug in the base layer shader code */
    if(GL::Context::current().detectedDriver() & GL::Context::DetectedDriver::SwiftShader)
        CORRADE_SKIP("UBOs with dynamically indexed arrays don't seem to work on SwiftShader, can't test.");
    #endif
    {
        /* Note that this is, in particular, *not* due to the expansion not
           being enough -- the bottom / right quads are shifted by 12 + 8
           pixels in total (which is verifiable if you remove the inner quad
           from the index buffer), and the difference is only about 16 pixels
           from the edge. */
        CORRADE_EXPECT_FAIL_IF(flag == BaseLayerSharedFlag::SubdividedQuads && data.filename == "edge-smoothness-inner-larger.png"_s,
            "Differing pixels on the innermost side of the smooth outline with" << BaseLayerSharedFlag::SubdividedQuads << "enabled, not sure why");
        CORRADE_COMPARE_WITH(_framebuffer.read({{}, RenderSize}, {PixelFormat::RGBA8Unorm}),
            Utility::Path::join({WHEE_TEST_DIR, "BaseLayerTestFiles", data.filename}),
            DebugTools::CompareImageToFile{_manager});
        }
    /* Verify that it's indeed just those pixels */
    if(flag == BaseLayerSharedFlag::SubdividedQuads && data.filename == "edge-smoothness-inner-larger.png"_s)
        CORRADE_COMPARE_WITH(_framebuffer.read({{}, RenderSize}, {PixelFormat::RGBA8Unorm}),
            Utility::Path::join({WHEE_TEST_DIR, "BaseLayerTestFiles", data.filename}),
            (DebugTools::CompareImageToFile{_manager, 3.25f, 0.028f}));
}

template<BaseLayerSharedFlag flag> void BaseLayerGLTest::renderGradientOutlineEdgeSmoothness() {
    auto&& data = RenderGradientOutlineEdgeSmoothnessData[testCaseInstanceId()];
    setTestCaseDescription(data.name);
    setTestCaseTemplateName(flag == BaseLayerSharedFlag::SubdividedQuads ? "Flag::SubdividedQuads" : "");

    /* The gradient should extend also under the outline. Testing by expanding
       the quad outside with negative padding, cancelling that with a
       transparent outline, and extrapolating the gradient accordingly for the
       outline. The result should be the same as render(gradient) above, it
       should also produce the same result regardless of the actual UI size. */

    /* Window size isn't used for anything here, can be arbitrary. If the size
       is meant to be set later, start with the framebuffer being 1x1. The UI
       size has to stay unchanged, otherwise it'll set NeedsNodeClipUpdate,
       which triggers data regeneration always. */
    AbstractUserInterface ui{data.scale*Vector2{RenderSize}, {1, 1}, (data.setSizeLater ? Vector2i{1} : RenderSize)};
    ui.setRendererInstance(Containers::pointer<RendererGL>());

    BaseLayerGL::Shared layerShared{BaseLayer::Shared::Configuration{1}
        .setFlags(flag)
    };
    layerShared.setStyle(
        BaseLayerCommonStyleUniform{}
            .setSmoothness(data.smoothness, 0.0f),
        {BaseLayerStyleUniform{}
            .setOutlineWidth(8.0f*data.scale)
            .setOutlineColor(0x00000000_rgbaf)
            .setColor(Math::lerp(0x774422_rgbf, 0xeeddaa_rgbf, 56.0f/48.0f),
                      Math::lerp(0xeeddaa_rgbf, 0x774422_rgbf, 56.0f/48.0f))},
        {Vector4{-8.0f}*data.scale});

    BaseLayer& layer = ui.setLayerInstance(Containers::pointer<BaseLayerGL>(ui.createLayer(), layerShared));

    NodeHandle node = ui.createNode(Vector2{8.0f, 8.0f}*data.scale,
                                    Vector2{112.0f, 48.0f}*data.scale);
    layer.create(0, node);

    if(data.setSizeLater) {
        /* Make sure everything is already processed before updating the size,
           otherwise it'd be all deferred to draw() below, circumventing what
           we want to test */
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});

        /* Setting the size should correctly regenerate whatever needs to be
           regenerated (except for SubdividedQuads, where it's all done in the
           shader) in order to adapt to the new DPI scaling. All state flag
           possibilities are tested in BaseLayerTest::setSize(). */
        ui.setSize(data.scale*Vector2{RenderSize}, {1, 1}, RenderSize);
        CORRADE_COMPARE(ui.state(), flag == BaseLayerSharedFlag::SubdividedQuads ? UserInterfaceStates{} : UserInterfaceState::NeedsDataUpdate);
    }

    ui.draw();

    MAGNUM_VERIFY_NO_GL_ERROR();

    if(!(_manager.load("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_manager.load("StbImageImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / StbImageImporter plugins not found.");

    #if defined(MAGNUM_TARGET_GLES) && !defined(MAGNUM_TARGET_WEBGL)
    /* Same problem is with all builtin shaders, so this doesn't seem to be a
       bug in the base layer shader code */
    if(GL::Context::current().detectedDriver() & GL::Context::DetectedDriver::SwiftShader)
        CORRADE_SKIP("UBOs with dynamically indexed arrays don't seem to work on SwiftShader, can't test.");
    #endif
    CORRADE_COMPARE_WITH(_framebuffer.read({{}, RenderSize}, {PixelFormat::RGBA8Unorm}),
        Utility::Path::join(WHEE_TEST_DIR, "BaseLayerTestFiles/gradient.png"),
        DebugTools::CompareImageToFile{_manager});
}

template<BaseLayerSharedFlag flag> void BaseLayerGLTest::renderTexturedOutlineEdgeSmoothness() {
    auto&& data = RenderTexturedOutlineEdgeSmoothnessData[testCaseInstanceId()];
    setTestCaseDescription(data.name);
    setTestCaseTemplateName(flag == BaseLayerSharedFlag::SubdividedQuads ? "Flag::SubdividedQuads" : "");

    /* The texture should extend also under the outline. Testing by expanding
       the quad outside with negative padding, cancelling that with a
       transparent outline, and extrapolating the texture coordinates
       accordingly for the outline. The result should be the same as
       renderTextured() above, it should also produce the same result
       regardless of the actual UI size. */

    if(!(_manager.load("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_manager.load("StbImageImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / StbImageImporter plugins not found.");

    Containers::Pointer<Trade::AbstractImporter> importer = _manager.loadAndInstantiate("AnyImageImporter");
    CORRADE_VERIFY(importer->openFile(Utility::Path::join(WHEE_TEST_DIR, "BaseLayerTestFiles/blur-input.png")));

    Containers::Optional<Trade::ImageData2D> image = importer->image2D(0);
    CORRADE_VERIFY(image);

    GL::Texture2DArray texture;
    texture
        .setMinificationFilter(GL::SamplerFilter::Linear)
        .setMagnificationFilter(GL::SamplerFilter::Linear)
        .setWrapping(GL::SamplerWrapping::ClampToEdge)
        .setStorage(1, GL::textureFormat(image->format()), Vector3i{image->size(), 1})
        .setSubImage(0, {}, ImageView2D{*image});

    /* Window size isn't used for anything here, can be arbitrary. If the size
       is meant to be set later, start with the framebuffer being 1x1. The UI
       size has to stay unchanged, otherwise it'll set NeedsNodeClipUpdate,
       which triggers data regeneration always. */
    AbstractUserInterface ui{data.scale*Vector2{RenderSize}, {1, 1}, (data.setSizeLater ? Vector2i{1} : RenderSize)};
    ui.setRendererInstance(Containers::pointer<RendererGL>());

    BaseLayerGL::Shared layerShared{BaseLayer::Shared::Configuration{1}
        .setFlags(BaseLayerSharedFlag::Textured|flag)
    };
    layerShared.setStyle(
        BaseLayerCommonStyleUniform{}
            .setSmoothness(data.smoothness, 1.0f),
        {BaseLayerStyleUniform{}
            .setOutlineWidth(8.0f*data.scale)
            .setOutlineColor(0x00000000_rgbaf)},
        {Vector4{-8.0f}*data.scale});

    BaseLayerGL& layer = ui.setLayerInstance(Containers::pointer<BaseLayerGL>(ui.createLayer(), layerShared));
    layer.setTexture(texture);

    NodeHandle node = ui.createNode(Vector2{8.0f, 8.0f}*data.scale,
                                    Vector2{112.0f, 48.0f}*data.scale);
    DataHandle nodeData = layer.create(0, node);
    layer.setTextureCoordinates(nodeData, {40.0f/160.0f, -8.0f/106.0f, 0}, {128.0f/160.0f, 64.0f/106.0f});

    if(data.setSizeLater) {
        /* Make sure everything is already processed before updating the size,
           otherwise it'd be all deferred to draw() below, circumventing what
           we want to test */
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});

        /* Setting the size should correctly regenerate whatever needs to be
           regenerated (except for SubdividedQuads, where it's all done in the
           shader) in order to adapt to the new DPI scaling. All state flag
           possibilities are tested in BaseLayerTest::setSize(). */
        ui.setSize(data.scale*Vector2{RenderSize}, {1, 1}, RenderSize);
        CORRADE_COMPARE(ui.state(), flag == BaseLayerSharedFlag::SubdividedQuads ? UserInterfaceStates{} : UserInterfaceState::NeedsDataUpdate);
    }

    ui.draw();

    MAGNUM_VERIFY_NO_GL_ERROR();

    #if defined(MAGNUM_TARGET_GLES) && !defined(MAGNUM_TARGET_WEBGL)
    /* Same problem is with all builtin shaders, so this doesn't seem to be a
       bug in the base layer shader code */
    if(GL::Context::current().detectedDriver() & GL::Context::DetectedDriver::SwiftShader)
        CORRADE_SKIP("UBOs with dynamically indexed arrays don't seem to work on SwiftShader, can't test.");
    #endif
    CORRADE_COMPARE_WITH(_framebuffer.read({{}, RenderSize}, {PixelFormat::RGBA8Unorm}),
        Utility::Path::join(WHEE_TEST_DIR, "BaseLayerTestFiles/textured.png"),
        /* One pixel has one channel off-by-one in the 100x smaller case */
        (DebugTools::CompareImageToFile{_manager, 0.25f, 0.0001f}));
}

template<BaseLayerSharedFlag flag> void BaseLayerGLTest::renderDynamicStyles() {
    auto&& data = RenderDynamicStylesData[testCaseInstanceId()];
    setTestCaseDescription(data.name);
    setTestCaseTemplateName(flag == BaseLayerSharedFlag::SubdividedQuads ? "Flag::SubdividedQuads" : "");

    AbstractUserInterface ui{RenderSize};
    ui.setRendererInstance(Containers::pointer<RendererGL>());

    BaseLayerGL::Shared layerShared{
        BaseLayer::Shared::Configuration{data.noBaseStyles ? 0u : 3u,
                                         data.noBaseStyles ? 0u : 4u}
            .addFlags(flag)
            .setDynamicStyleCount(2)
    };

    BaseLayerGL* layer{};
    if(!data.createLayerAfterSetStyle)
        layer = &ui.setLayerInstance(Containers::pointer<BaseLayerGL>(ui.createLayer(), layerShared));

    /* If the style is being uploaded second time, upload just a default state
       at first */
    if(data.secondaryStyleUpload) {
        layerShared.setStyle(BaseLayerCommonStyleUniform{},
            {BaseLayerStyleUniform{}, BaseLayerStyleUniform{}, BaseLayerStyleUniform{}},
            /* The mapping is deliberately different, the secondary upload
               should cause it to be updated */
            {2, 1, 1, 0},
            {});
    } else if(data.noBaseStyles) {
        layerShared.setStyle(BaseLayerCommonStyleUniform{},
            {},
            {});
    } else {
        layerShared.setStyle(BaseLayerCommonStyleUniform{},
            {BaseLayerStyleUniform{}, BaseLayerStyleUniform{}, data.styleUniform},
            {1, 2, 0, 1},
            {{}, {data.leftPadding, 0.0f, 0.0f, 0.0f}, {}, {}});
    }

    /* If the layer is created after the setStyle() call, it should have no
       LayerStates set implicitly, otherwise setStyle() causes the state to be
       set on all existing layers */
    if(data.createLayerAfterSetStyle) {
        layer = &ui.setLayerInstance(Containers::pointer<BaseLayerGL>(ui.createLayer(), layerShared));
        CORRADE_COMPARE(layer->state(), LayerStates{});
    } else {
        CORRADE_COMPARE(layer->state(), LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate);
    }

    if(data.dynamicStyleUniform) {
        /* Again, if the dynamic style  is being uploaded second time, upload
           just a default state at first */
        if(data.secondaryDynamicStyleUpload) {
            layer->setDynamicStyle(1,
                BaseLayerStyleUniform{},
                {});
        } else {
            layer->setDynamicStyle(1,
                *data.dynamicStyleUniform,
                {data.dynamicLeftPadding, 0.0f, 0.0f, 0.0f});
        }

        /* The NeedsDataUpdate is from an earlier setStyle() */
        CORRADE_COMPARE(layer->state(), LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate);
    }

    /* Undo the padding coming from the style to have the result always the
       same */
    NodeHandle node = ui.createNode(
        {8.0f - data.leftPadding - data.dynamicLeftPadding, 8.0f},
        {112.0f + data.leftPadding + data.dynamicLeftPadding, 48.0f});
    layer->create(data.styleIndex, node);

    /* If there's a secondary upload, draw & clear to force the first upload */
    if(data.secondaryStyleUpload || data.secondaryDynamicStyleUpload) {
        ui.draw();
        CORRADE_COMPARE(layer->state(), LayerStates{});
        _framebuffer.clear(GL::FramebufferClear::Color);
    }

    /* Upload the actual style data only second time if desired */
    if(data.secondaryStyleUpload) {
        layerShared.setStyle(BaseLayerCommonStyleUniform{},
            {BaseLayerStyleUniform{}, BaseLayerStyleUniform{}, data.styleUniform},
            {1, 2, 0, 1},
            {{}, {data.leftPadding, 0.0f, 0.0f, 0.0f}, {}, {}});
        CORRADE_COMPARE(layer->state(), LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate);
    }
    if(data.secondaryDynamicStyleUpload) {
        layer->setDynamicStyle(1,
            *data.dynamicStyleUniform,
            {data.dynamicLeftPadding, 0.0f, 0.0f, 0.0f});
        CORRADE_COMPARE(layer->state(), LayerState::NeedsCommonDataUpdate);
    }

    ui.draw();
    CORRADE_COMPARE(layer->state(), LayerStates{});

    MAGNUM_VERIFY_NO_GL_ERROR();

    if(!(_manager.load("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_manager.load("StbImageImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / StbImageImporter plugins not found.");

    #if defined(MAGNUM_TARGET_GLES) && !defined(MAGNUM_TARGET_WEBGL)
    /* Same problem is with all builtin shaders, so this doesn't seem to be a
       bug in the base layer shader code */
    if(GL::Context::current().detectedDriver() & GL::Context::DetectedDriver::SwiftShader)
        CORRADE_SKIP("UBOs with dynamically indexed arrays don't seem to work on SwiftShader, can't test.");
    #endif
    CORRADE_COMPARE_WITH(_framebuffer.read({{}, RenderSize}, {PixelFormat::RGBA8Unorm}),
        Utility::Path::join({WHEE_TEST_DIR, "BaseLayerTestFiles", data.filename}),
        DebugTools::CompareImageToFile{_manager});
}

void BaseLayerGLTest::renderOrDrawCompositeSetup() {
    /* Using the framebuffer inside the RendererGL instead, thus this can be
       also shared for all render*() and draw*() cases */

    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::One, GL::Renderer::BlendFunction::OneMinusSourceAlpha);
    /* The RendererGL should enable these on its own if needed */
    GL::Renderer::disable(GL::Renderer::Feature::ScissorTest);
    GL::Renderer::disable(GL::Renderer::Feature::Blending);
}

void BaseLayerGLTest::renderOrDrawCompositeTeardown() {
    /* Using the framebuffer inside the RendererGL instead, thus this can be
       also shared for all render*() and draw*() cases */

    GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::disable(GL::Renderer::Feature::ScissorTest);
    GL::Renderer::disable(GL::Renderer::Feature::Blending);
}

template<BaseLayerSharedFlag flag> void BaseLayerGLTest::renderComposite() {
    auto&& data = RenderCompositeData[testCaseInstanceId()];
    setTestCaseDescription(data.name);
    setTestCaseTemplateName(flag == BaseLayerSharedFlag::SubdividedQuads ? "Flag::SubdividedQuads" : "");

    if(!(_manager.load("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_manager.load("StbImageImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / StbImageImporter plugins not found.");

    AbstractUserInterface ui{RenderSize};
    RendererGL& renderer = ui.setRendererInstance(Containers::pointer<RendererGL>(RendererGL::Flag::CompositingFramebuffer));

    /* Upload (a crop of) the blur source image as a framebuffer background */
    Containers::Pointer<Trade::AbstractImporter> importer = _manager.loadAndInstantiate("AnyImageImporter");
    CORRADE_VERIFY(importer->openFile(Utility::Path::join(WHEE_TEST_DIR, "BaseLayerTestFiles/blur-input.png")));

    Containers::Optional<Trade::ImageData2D> image = importer->image2D(0);
    CORRADE_VERIFY(image);
    CORRADE_COMPARE(image->format(), PixelFormat::RGBA8Unorm);
    CORRADE_COMPARE_AS(image->size(), RenderSize,
        TestSuite::Compare::GreaterOrEqual);

    Image2D imageCropped{PixelFormat::RGBA8Unorm, RenderSize, Containers::Array<char>{NoInit, std::size_t(RenderSize.product()*4)}};
    Utility::copy(image->pixels<Color4ub>().prefix({
        std::size_t(RenderSize.y()),
        std::size_t(RenderSize.x()),
    }), imageCropped.pixels<Color4ub>());

    renderer.compositingTexture().setSubImage(0, {}, imageCropped);

    BaseLayerGL::Shared::Configuration configuration{2};
    configuration.addFlags(data.flags|flag);
    if(data.backgroundBlurRadius) {
        if(data.backgroundBlurCutoff)
            configuration.setBackgroundBlurRadius(*data.backgroundBlurRadius, *data.backgroundBlurCutoff);
        else
            configuration.setBackgroundBlurRadius(*data.backgroundBlurRadius);
    }

    BaseLayerGL::Shared layerShared{configuration};
    layerShared.setStyle(data.styleCommon,
        /* To verify it's not always picking the first uniform */
        {BaseLayerStyleUniform{}, data.styleUniform},
        {});

    BaseLayerGL& layer = ui.setLayerInstance(Containers::pointer<BaseLayerGL>(ui.createLayer(), layerShared));
    if(data.backgroundBlurPassCount)
        layer.setBackgroundBlurPassCount(*data.backgroundBlurPassCount);

    NodeHandle node = ui.createNode({8.0f, 8.0f}, {112.0f, 48.0f});
    layer.create(1, node);

    ui.draw();

    MAGNUM_VERIFY_NO_GL_ERROR();

    #if defined(MAGNUM_TARGET_GLES) && !defined(MAGNUM_TARGET_WEBGL)
    /* Same problem is with all builtin shaders, so this doesn't seem to be a
       bug in the base layer shader code */
    if(GL::Context::current().detectedDriver() & GL::Context::DetectedDriver::SwiftShader)
        CORRADE_SKIP("UBOs with dynamically indexed arrays don't seem to work on SwiftShader, can't test.");
    #endif
    CORRADE_COMPARE_WITH(renderer.compositingFramebuffer().read({{}, RenderSize}, {PixelFormat::RGBA8Unorm}),
        Utility::Path::join({WHEE_TEST_DIR, "BaseLayerTestFiles", data.filename}),
        (DebugTools::CompareImageToFile{_manager, data.maxThreshold, data.meanThreshold}));
}

template<BaseLayerSharedFlag flag> void BaseLayerGLTest::renderCompositeEdgeSmoothness() {
    auto&& data = RenderCompositeEdgeSmoothnessData[testCaseInstanceId()];
    setTestCaseDescription(data.name);
    setTestCaseTemplateName(flag == BaseLayerSharedFlag::SubdividedQuads ? "Flag::SubdividedQuads" : "");

    if(!(_manager.load("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_manager.load("StbImageImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / StbImageImporter plugins not found.");

    /* The background blur should be calculated on quads including the extra
       smoothness (thus in this case the whole UI size), otherwise the smooth
       edges get random values. Picking a radius that's not too large as
       otherwise the random noise would smoothen out, making the comparison
       more likely to pass. It should also produce the same result regardless
       of the actual UI size. */

    /* Window size isn't used for anything here, can be arbitrary. If the size
       is meant to be set later, start with the framebuffer being 1x1. The UI
       size has to stay unchanged, otherwise it'll set NeedsNodeClipUpdate,
       which triggers data regeneration always. */
    AbstractUserInterface ui{data.scale*Vector2{RenderSize}, {1, 1}, (data.setSizeLater ? Vector2i{1} : RenderSize)};
    RendererGL& renderer = ui.setRendererInstance(Containers::pointer<RendererGL>(RendererGL::Flag::CompositingFramebuffer));

    /* Upload (a crop of) the blur source image as a framebuffer background */
    Containers::Pointer<Trade::AbstractImporter> importer = _manager.loadAndInstantiate("AnyImageImporter");
    CORRADE_VERIFY(importer->openFile(Utility::Path::join(WHEE_TEST_DIR, "BaseLayerTestFiles/blur-input.png")));

    Containers::Optional<Trade::ImageData2D> image = importer->image2D(0);
    CORRADE_VERIFY(image);
    CORRADE_COMPARE(image->format(), PixelFormat::RGBA8Unorm);
    CORRADE_COMPARE_AS(image->size(), RenderSize,
        TestSuite::Compare::GreaterOrEqual);

    Image2D imageCropped{PixelFormat::RGBA8Unorm, RenderSize, Containers::Array<char>{NoInit, std::size_t(RenderSize.product()*4)}};
    Utility::copy(image->pixels<Color4ub>().prefix({
        std::size_t(RenderSize.y()),
        std::size_t(RenderSize.x()),
    }), imageCropped.pixels<Color4ub>());

    BaseLayerGL::Shared layerShared{BaseLayerGL::Shared::Configuration{1}
        .addFlags(BaseLayerSharedFlag::BackgroundBlur|flag)
        .setBackgroundBlurRadius(2)
    };
    layerShared.setStyle(
        BaseLayerCommonStyleUniform{}
            .setSmoothness(8.0f),
        {BaseLayerStyleUniform{}
            .setCornerRadius(24.0f*data.scale)
            /* Premultiplied alpha */
            .setColor(0xffffffff_rgbaf*0.5f)},
        {});

    BaseLayerGL& layer = ui.setLayerInstance(Containers::pointer<BaseLayerGL>(ui.createLayer(), layerShared));

    NodeHandle node = ui.createNode(Vector2{8.0f, 8.0f}*data.scale,
                                    Vector2{112.0f, 48.0f}*data.scale);
    layer.create(0, node);

    if(data.setSizeLater) {
        /* Make sure everything is already processed before updating the size,
           otherwise it'd be all deferred to draw() below, circumventing what
           we want to test */
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});

        /* Setting the size should correctly regenerate whatever needs to be
           regenerated (except for SubdividedQuads, where it's all done in the
           shader) in order to adapt to the new DPI scaling. All state flag
           possibilities are tested in BaseLayerTest::setSize(). */
        ui.setSize(data.scale*Vector2{RenderSize}, {1, 1}, RenderSize);
        CORRADE_COMPARE(ui.state(), flag == BaseLayerSharedFlag::SubdividedQuads ? UserInterfaceStates{} : UserInterfaceState::NeedsDataUpdate);
    }

    /* Set the background image only after setSize() to make sure it's uploaded
       to the current framebuffer */
    renderer.compositingTexture().setSubImage(0, {}, imageCropped);

    ui.draw();

    MAGNUM_VERIFY_NO_GL_ERROR();

    #if defined(MAGNUM_TARGET_GLES) && !defined(MAGNUM_TARGET_WEBGL)
    /* Same problem is with all builtin shaders, so this doesn't seem to be a
       bug in the base layer shader code */
    if(GL::Context::current().detectedDriver() & GL::Context::DetectedDriver::SwiftShader)
        CORRADE_SKIP("UBOs with dynamically indexed arrays don't seem to work on SwiftShader, can't test.");
    #endif
    CORRADE_COMPARE_WITH(renderer.compositingFramebuffer().read({{}, RenderSize}, {PixelFormat::RGBA8Unorm}),
        Utility::Path::join(WHEE_TEST_DIR, "BaseLayerTestFiles/composite-background-blur-50-smooth.png"),
        DebugTools::CompareImageToFile{_manager});
}

template<BaseLayerSharedFlag flag> void BaseLayerGLTest::renderCompositeTextured() {
    auto&& data = RenderCompositeTexturedData[testCaseInstanceId()];
    setTestCaseDescription(data.name);
    setTestCaseTemplateName(flag == BaseLayerSharedFlag::SubdividedQuads ? "Flag::SubdividedQuads" : "");

    if(!(_manager.load("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_manager.load("StbImageImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / StbImageImporter plugins not found.");

    AbstractUserInterface ui{RenderSize};
    RendererGL& renderer = ui.setRendererInstance(Containers::pointer<RendererGL>(RendererGL::Flag::CompositingFramebuffer));

    Containers::Pointer<Trade::AbstractImporter> importer = _manager.loadAndInstantiate("AnyImageImporter");

    CORRADE_VERIFY(importer->openFile(Utility::Path::join({WHEE_TEST_DIR, "BaseLayerTestFiles", data.textureFilename})));

    Containers::Optional<Trade::ImageData2D> image = importer->image2D(0);
    CORRADE_VERIFY(image);

    GL::Texture2DArray texture;
    texture
        .setMinificationFilter(GL::SamplerFilter::Linear)
        .setMagnificationFilter(GL::SamplerFilter::Linear)
        .setStorage(1, GL::textureFormat(image->format()), Vector3i{image->size(), 8})
        .setSubImage(0, {0, 0, Int(data.offset.z())}, ImageView2D{*image});

    BaseLayerGL::Shared layerShared{BaseLayer::Shared::Configuration{2}
        .addFlags(BaseLayerSharedFlag::BackgroundBlur|
                  BaseLayerSharedFlag::Textured|
                  data.extraFlags|flag)
        .setBackgroundBlurRadius(31)};
    layerShared.setStyle(
        BaseLayerCommonStyleUniform{}
            .setSmoothness(1.0f),
        /* To verify it's not always picking the first uniform */
        {BaseLayerStyleUniform{}, data.styleItem},
        {});

    BaseLayerGL& layer = ui.setLayerInstance(Containers::pointer<BaseLayerGL>(ui.createLayer(), layerShared));
    layer.setTexture(texture);

    /* Upload (a crop of) the blur source image as a framebuffer background */
    CORRADE_VERIFY(importer->openFile(Utility::Path::join(WHEE_TEST_DIR, "BaseLayerTestFiles/blur-input.png")));

    Containers::Optional<Trade::ImageData2D> backgroundImage = importer->image2D(0);
    CORRADE_VERIFY(backgroundImage);
    CORRADE_COMPARE(backgroundImage->format(), PixelFormat::RGBA8Unorm);
    CORRADE_COMPARE_AS(backgroundImage->size(), RenderSize,
        TestSuite::Compare::GreaterOrEqual);

    Image2D imageCropped{PixelFormat::RGBA8Unorm, RenderSize, Containers::Array<char>{NoInit, std::size_t(RenderSize.product()*4)}};
    Utility::copy(backgroundImage->pixels<Color4ub>().prefix({
        std::size_t(RenderSize.y()),
        std::size_t(RenderSize.x()),
    }), imageCropped.pixels<Color4ub>());

    renderer.compositingTexture().setSubImage(0, {}, imageCropped);

    NodeHandle node = ui.createNode({8.0f, 8.0f}, {112.0f, 48.0f});
    DataHandle nodeData = layer.create(1, node);
    layer.setTextureCoordinates(nodeData, data.offset, data.size);

    ui.draw();

    MAGNUM_VERIFY_NO_GL_ERROR();

    #if defined(MAGNUM_TARGET_GLES) && !defined(MAGNUM_TARGET_WEBGL)
    /* Same problem is with all builtin shaders, so this doesn't seem to be a
       bug in the base layer shader code */
    if(GL::Context::current().detectedDriver() & GL::Context::DetectedDriver::SwiftShader)
        CORRADE_SKIP("UBOs with dynamically indexed arrays don't seem to work on SwiftShader, can't test.");
    #endif
    CORRADE_COMPARE_WITH(renderer.compositingFramebuffer().read({{}, RenderSize}, {PixelFormat::RGBA8Unorm}),
        Utility::Path::join({WHEE_TEST_DIR, "BaseLayerTestFiles", data.expectedFilename}),
        (DebugTools::CompareImageToFile{_manager}));
}

void BaseLayerGLTest::renderCompositeNodeRects() {
    auto&& data = RenderCompositeNodeRectsData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    if(!(_manager.load("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_manager.load("StbImageImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / StbImageImporter plugins not found.");

    /* Window size used only for events, can be anything */
    AbstractUserInterface ui{Vector2{RenderSize}*data.uiScale, {345, 678}, RenderSize};
    RendererGL& renderer = ui.setRendererInstance(Containers::pointer<RendererGL>(RendererGL::Flag::CompositingFramebuffer));

    /* Upload (a crop of) the blur source image as a framebuffer background */
    Containers::Pointer<Trade::AbstractImporter> importer = _manager.loadAndInstantiate("AnyImageImporter");
    CORRADE_VERIFY(importer->openFile(Utility::Path::join(WHEE_TEST_DIR, "BaseLayerTestFiles/blur-input.png")));

    Containers::Optional<Trade::ImageData2D> image = importer->image2D(0);
    CORRADE_VERIFY(image);
    CORRADE_COMPARE(image->format(), PixelFormat::RGBA8Unorm);
    CORRADE_COMPARE_AS(image->size(), RenderSize,
        TestSuite::Compare::GreaterOrEqual);

    Image2D imageCropped{PixelFormat::RGBA8Unorm, RenderSize, Containers::Array<char>{NoInit, std::size_t(RenderSize.product()*4)}};
    Utility::copy(image->pixels<Color4ub>().prefix({
        std::size_t(RenderSize.y()),
        std::size_t(RenderSize.x()),
    }), imageCropped.pixels<Color4ub>());

    BaseLayerGL::Shared layerShared{BaseLayerGL::Shared::Configuration{1}
        .addFlags(BaseLayerSharedFlag::BackgroundBlur)
        .setBackgroundBlurRadius(data.backgroundBlurRadius)};
    layerShared.setStyle(
        BaseLayerCommonStyleUniform{},
        {BaseLayerStyleUniform{}
            .setColor(0xffffffff_rgbaf*0.25f)},
        {});

    BaseLayerGL& layer = ui.setLayerInstance(Containers::pointer<BaseLayerGL>(ui.createLayer(), layerShared));
    layer.setBackgroundBlurPassCount(data.backgroundBlurPassCount);

    NodeHandle root = ui.createNode({}, ui.size());

    /* Skipping early because can't really test the actual rendering anyway,
       and the blur seems to be not drawn at all, giving back #00ffff again.
       FFS, SwiftShader. */
    #if defined(MAGNUM_TARGET_GLES) && !defined(MAGNUM_TARGET_WEBGL)
    /* Same problem is with all builtin shaders, so this doesn't seem to be a
       bug in the base layer shader code */
    if(GL::Context::current().detectedDriver() & GL::Context::DetectedDriver::SwiftShader)
        CORRADE_SKIP("UBOs with dynamically indexed arrays don't seem to work on SwiftShader, can't test.");
    #endif

    /* To make accidentally too small border better visible, first fill the
       background with a cyan color and run it as a whole though the blur
       process. */
    renderer.compositingFramebuffer().clearColor(0, 0x00ffff_rgbf);
    DataHandle wholeUiBlack = layer.create(0, root);
    ui.draw();
    CORRADE_COMPARE(renderer.compositingFramebuffer().read(Range2Di::fromSize(RenderSize/2, {1, 1}), {PixelFormat::RGBA8Unorm}).pixels<Color4ub>()[0][0], 0x40ffff_rgb);

    /* Then remove the whole-UI blur, and replace the background with an
       image */
    layer.remove(wholeUiBlack);
    renderer.compositingTexture().setSubImage(0, {}, imageCropped);

    /* Four nodes in locations that:

        -   are all the same top-level node hierarchy
        -   are deliberately assymetric to verify the blur coordinates don't
            get Y flipped, wrongly scaled or some such
        -   have enough uncovered space to verify that the blurred background
            area is properly expanded for large radii
        -   expand beyond the UI size to verify it doesn't cause any strange
            artifacts
        -   partially overlap, but as they're both rendered at the same level
            with the same blurred background it doesn't get lighter
    */

    /* This one expands out of the UI on -X */
    layer.create(0, ui.createNode(root, data.uiScale*Vector2{-8.0f, 4.0f},
                                        data.uiScale*Vector2{64.0f, 32.0f}));
    /* These two are right above each other */
    layer.create(0, ui.createNode(root, data.uiScale*Vector2{80.0f, 40.0f},
                                        data.uiScale*Vector2{16.0f, 8.0f}));
    /* This one expands out of the UI on +Y */
    layer.create(0, ui.createNode(root, data.uiScale*Vector2{80.0f, 48.0f},
                                        data.uiScale*Vector2{16.0f, 900.0f}));
    /* This one is over the above node */
    layer.create(0, ui.createNode(root, data.uiScale*Vector2{64.0f, 48.0f},
                                        data.uiScale*Vector2{48.0f, 8.0f}));

    ui.draw();

    MAGNUM_VERIFY_NO_GL_ERROR();

    CORRADE_COMPARE_WITH(renderer.compositingFramebuffer().read({{}, RenderSize}, {PixelFormat::RGBA8Unorm}),
        Utility::Path::join({WHEE_TEST_DIR, "BaseLayerTestFiles", data.filename}),
        (DebugTools::CompareImageToFile{_manager, data.maxThreshold, data.meanThreshold}));
}

constexpr Vector2i DrawSize{64, 64};

void BaseLayerGLTest::drawSetup() {
    _color = GL::Texture2D{};
    _color.setStorage(1, GL::TextureFormat::RGBA8, DrawSize);
    _framebuffer = GL::Framebuffer{{{}, DrawSize}};
    _framebuffer
        .attachTexture(GL::Framebuffer::ColorAttachment{0}, _color, 0)
        .clear(GL::FramebufferClear::Color)
        .bind();

    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::One, GL::Renderer::BlendFunction::OneMinusSourceAlpha);
    /* The RendererGL should enable these on its own if needed */
    GL::Renderer::disable(GL::Renderer::Feature::ScissorTest);
    GL::Renderer::disable(GL::Renderer::Feature::Blending);
}

void BaseLayerGLTest::drawTeardown() {
    _framebuffer = GL::Framebuffer{NoCreate};
    _color = GL::Texture2D{NoCreate};

    GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::disable(GL::Renderer::Feature::ScissorTest);
    GL::Renderer::disable(GL::Renderer::Feature::Blending);
}

void BaseLayerGLTest::drawOrder() {
    auto&& data = DrawOrderData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    AbstractUserInterface ui{DrawSize};
    ui.setRendererInstance(Containers::pointer<RendererGL>());

    BaseLayerGL::Shared layerShared{BaseLayer::Shared::Configuration{3}};
    /* Testing the styleToUniform initializer list overload, others cases use
       implicit mapping initializer list overloads */
    layerShared.setStyle(BaseLayerCommonStyleUniform{}, {
        BaseLayerStyleUniform{}         /* 0, red */
            .setColor(0xff0000_rgbf),
        BaseLayerStyleUniform{}         /* 1, green */
            .setColor(0x00ff00_rgbf),
        BaseLayerStyleUniform{}         /* 2, blue */
            .setColor(0x0000ff_rgbf)
    }, {0, 1, 2}, {});

    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<BaseLayerGL>(layer, layerShared));

    NodeHandle topLevelOnTopGreen = ui.createNode({8.0f, 8.0f}, {32.0f, 32.0f});

    NodeHandle topLevelBelowRed = ui.createNode({24.0f, 24.0f}, {32.0f, 32.0f});
    ui.setNodeOrder(topLevelBelowRed, topLevelOnTopGreen);

    NodeHandle topLevelHiddenBlue = ui.createNode({24.0f, 8.0f}, {32.0f, 32.0f}, NodeFlag::Hidden);

    NodeHandle childBelowBlue = ui.createNode(topLevelOnTopGreen, {12.0f, 4.0f}, {16.0f, 16.0f});
    NodeHandle childAboveRed = ui.createNode(childBelowBlue, {-8.0f, 8.0f}, {16.0f, 16.0f});

    if(data.dataInNodeOrder) {
        ui.layer<BaseLayerGL>(layer).create(0, topLevelBelowRed);
        ui.layer<BaseLayerGL>(layer).create(1, topLevelOnTopGreen);
        ui.layer<BaseLayerGL>(layer).create(2, topLevelHiddenBlue);
        ui.layer<BaseLayerGL>(layer).create(2, childBelowBlue);
        ui.layer<BaseLayerGL>(layer).create(0, childAboveRed);
    } else {
        ui.layer<BaseLayerGL>(layer).create(1, topLevelOnTopGreen);
        ui.layer<BaseLayerGL>(layer).create(2, topLevelHiddenBlue);
        ui.layer<BaseLayerGL>(layer).create(0, topLevelBelowRed);
        ui.layer<BaseLayerGL>(layer).create(0, childAboveRed);
        ui.layer<BaseLayerGL>(layer).create(2, childBelowBlue);
    }

    ui.draw();

    MAGNUM_VERIFY_NO_GL_ERROR();

    if(!(_manager.load("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_manager.load("StbImageImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / StbImageImporter plugins not found.");

    #if defined(MAGNUM_TARGET_GLES) && !defined(MAGNUM_TARGET_WEBGL)
    /* Same problem is with all builtin shaders, so this doesn't seem to be a
       bug in the base layer shader code */
    if(GL::Context::current().detectedDriver() & GL::Context::DetectedDriver::SwiftShader)
        CORRADE_SKIP("UBOs with dynamically indexed arrays don't seem to work on SwiftShader, can't test.");
    #endif
    CORRADE_COMPARE_WITH(_framebuffer.read({{}, DrawSize}, {PixelFormat::RGBA8Unorm}),
        Utility::Path::join(WHEE_TEST_DIR, "BaseLayerTestFiles/draw-order.png"),
        DebugTools::CompareImageToFile{_manager});
}

void BaseLayerGLTest::drawOrderComposite() {
    auto&& data = DrawOrderCompositeData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* A reduced variant of drawOrder() that only tests the effect of (stacked)
       compositing, that it's applied on top-level nodes and its children as a
       whole and not on each node separately. As it's meant to be used only on
       non-overlapping nodes within a top-level node, the blending behaves
       differently between the non-composited and composited case, in
       particular the blue doesn't shine through the red. */

    AbstractUserInterface ui{DrawSize};
    RendererGL& renderer = ui.setRendererInstance(Containers::pointer<RendererGL>(RendererGL::Flag::CompositingFramebuffer));

    /* Clear the framebuffer so we can draw to it */
    renderer.compositingFramebuffer().clear(GL::FramebufferClear::Color);

    BaseLayerGL::Shared layerShared{
        BaseLayerGL::Shared::Configuration{4}
            .addFlags(data.flags)
            .setBackgroundBlurRadius(16)
    };
    layerShared.setStyle(BaseLayerCommonStyleUniform{}, {
        BaseLayerStyleUniform{}         /* 0, red */
            .setColor(0xff0000ff_rgbaf*0.5f),
        BaseLayerStyleUniform{}         /* 1, green */
            .setColor(0x00ff00ff_rgbaf*0.5f),
        BaseLayerStyleUniform{}         /* 2, blue */
            .setColor(0x0000ffff_rgbaf*0.5f),
        BaseLayerStyleUniform{}         /* 3, white */
            .setColor(0xffffffff_rgbaf)
    }, {});

    BaseLayerGL& layer = ui.setLayerInstance(Containers::pointer<BaseLayerGL>(ui.createLayer(), layerShared));

    NodeHandle topLevelBelowWhite = ui.createNode({28.0f, 28.0f}, {48.0f, 48.0f});

    NodeHandle topLevelOnTopGreen = ui.createNode({8.0f, 8.0f}, {48.0f, 48.0f});

    NodeHandle childBelowBlue = ui.createNode(topLevelOnTopGreen, {12.0f, 4.0f}, {32.0f, 32.0f});
    NodeHandle childAboveRed = ui.createNode(childBelowBlue, {-8.0f, 8.0f}, {32.0f, 32.0f});

    layer.create(3, topLevelBelowWhite);
    layer.create(1, topLevelOnTopGreen);
    layer.create(2, childBelowBlue);
    layer.create(0, childAboveRed);

    ui.draw();

    MAGNUM_VERIFY_NO_GL_ERROR();

    if(!(_manager.load("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_manager.load("StbImageImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / StbImageImporter plugins not found.");

    #if defined(MAGNUM_TARGET_GLES) && !defined(MAGNUM_TARGET_WEBGL)
    /* Same problem is with all builtin shaders, so this doesn't seem to be a
       bug in the base layer shader code */
    if(GL::Context::current().detectedDriver() & GL::Context::DetectedDriver::SwiftShader)
        CORRADE_SKIP("UBOs with dynamically indexed arrays don't seem to work on SwiftShader, can't test.");
    #endif
    CORRADE_COMPARE_WITH(renderer.compositingFramebuffer().read({{}, DrawSize}, {PixelFormat::RGBA8Unorm}),
        Utility::Path::join({WHEE_TEST_DIR, "BaseLayerTestFiles", data.filename}),
        DebugTools::CompareImageToFile{_manager});
}

void BaseLayerGLTest::drawClipping() {
    auto&& data = DrawClippingData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* X is divided by 10, Y by 100 when rendering. Window size (for events)
       isn't used for anything here. */
    AbstractUserInterface ui{{640.0f, 6400.0f}, {1.0f, 1.0f}, DrawSize};
    ui.setRendererInstance(Containers::pointer<RendererGL>());

    BaseLayerGL::Shared layerShared{BaseLayer::Shared::Configuration{3}};
    layerShared.setStyle(BaseLayerCommonStyleUniform{}, {
        BaseLayerStyleUniform{}         /* 0, red */
            .setColor(0xff0000_rgbf),
        BaseLayerStyleUniform{}         /* 1, green */
            .setColor(0x00ff00_rgbf),
        BaseLayerStyleUniform{}         /* 2, blue */
            .setColor(0x0000ff_rgbf)
    }, {});

    BaseLayerGL& layer = ui.setLayerInstance(Containers::pointer<BaseLayerGL>(ui.createLayer(), layerShared));

    /* Two main clip nodes, each containing subnodes that have custom white
       outline that shouldn't be visible if clipping is enabled. They're either
       top-level nodes with possibly swapped order, in which case they're
       submitted in two separate draws, or they're sub-nodes of a single
       top-level node in which case they're drawn together with two clip rect
       ranges. */
    NodeHandle parent = NodeHandle::Null;
    if(data.singleTopLevel) {
        parent = ui.createNode({}, {});
    }

    NodeHandle leftTop = ui.createNode(parent, {60.0f, 600.0f}, {320.0f, 3200.0f});
    NodeHandle leftTop1 = ui.createNode(leftTop, {-20.0f, -200.0f}, {360.0f, 1800.0f});
    NodeHandle leftTop2 = ui.createNode(leftTop, {-20.0f, 1600.0f}, {360.0f, 1800.0f});
    /* Child of leftTop2, but should only be clipped against leftTop, not
       leftTop2 */
    NodeHandle leftTop21 = ui.createNode(leftTop2, {140.0f, -400.0f}, {80.0f, 2400.0f});
    layer.create(0, 0xffffff_rgbf, {20.0f, 200.0f, 20.0f, 000.0f}, leftTop1);
    layer.create(1, 0xffffff_rgbf, {20.0f, 000.0f, 20.0f, 200.0f}, leftTop2);
    layer.create(2, 0xffffff_rgbf, {00.0f, 000.0f, 00.0f, 400.0f}, leftTop21);

    NodeHandle rightBottom = ui.createNode(parent, {380.0f, 3800.0f}, {200.0f, 2000.0f});
    NodeHandle rightBottom1 = ui.createNode(rightBottom, {-40.0f, -400.0f}, {140.0f, 2800.0f});
    /* Completely outside the rightBottom area, should get culled, i.e. not
       even passed to draw() */
    NodeHandle rightBottom11 = ui.createNode(rightBottom1, {-300.0f, 2000.0f}, {80.0f, 800.0f});
    /* Data added to the clip node should get clipped as well */
    DataHandle rightBottomData = layer.create(0, 0xffffff_rgbf, {40.0f, 400.0f, 40.0f, 400.0f}, rightBottom);
    layer.setPadding(rightBottomData, {-40.0f, -400.0f, -40.0f, -400.0f});
    layer.create(2, 0xffffff_rgbf, {40.0f, 400.0f, 00.0f, 400.0f}, rightBottom1);
    layer.create(1, 0xffffff_rgbf, {10.0f, 100.0f, 10.0f, 100.0f}, rightBottom11);

    if(data.flipOrder) {
        CORRADE_COMPARE(ui.nodeOrderNext(rightBottom), NodeHandle::Null);
        ui.setNodeOrder(rightBottom, leftTop);
        CORRADE_COMPARE(ui.nodeOrderNext(rightBottom), leftTop);
    }

    if(data.clip) {
        ui.addNodeFlags(leftTop, NodeFlag::Clip);
        ui.addNodeFlags(rightBottom, NodeFlag::Clip);
    }

    ui.draw();

    MAGNUM_VERIFY_NO_GL_ERROR();

    if(!(_manager.load("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_manager.load("StbImageImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / StbImageImporter plugins not found.");

    #if defined(MAGNUM_TARGET_GLES) && !defined(MAGNUM_TARGET_WEBGL)
    /* Same problem is with all builtin shaders, so this doesn't seem to be a
       bug in the base layer shader code */
    if(GL::Context::current().detectedDriver() & GL::Context::DetectedDriver::SwiftShader)
        CORRADE_SKIP("UBOs with dynamically indexed arrays don't seem to work on SwiftShader, can't test.");
    #endif
    CORRADE_COMPARE_WITH(_framebuffer.read({{}, DrawSize}, {PixelFormat::RGBA8Unorm}),
        Utility::Path::join({WHEE_TEST_DIR, "BaseLayerTestFiles", data.filename}),
        DebugTools::CompareImageToFile{_manager});
}

void BaseLayerGLTest::eventStyleTransition() {
    /* Switches between the "default" and "gradient" cases from render() after
       a press event. Everything else is tested in AbstractVisualLayerTest
       already. */

    AbstractUserInterface ui{RenderSize};
    ui.setRendererInstance(Containers::pointer<RendererGL>());

    BaseLayerGL::Shared layerShared{BaseLayer::Shared::Configuration{2}};
    layerShared
        .setStyle(BaseLayerCommonStyleUniform{}, {
            BaseLayerStyleUniform{},        /* default */
            BaseLayerStyleUniform{}         /* gradient */
                .setColor(0xeeddaa_rgbf, 0x774422_rgbf)
        }, {})
        .setStyleTransition(
            [](UnsignedInt) -> UnsignedInt {
                CORRADE_INTERNAL_ASSERT_UNREACHABLE();
            },
            [](UnsignedInt) -> UnsignedInt {
                CORRADE_INTERNAL_ASSERT_UNREACHABLE();
            },
            [](UnsignedInt style) -> UnsignedInt {
                if(style == 0) return 1;
                CORRADE_INTERNAL_ASSERT_UNREACHABLE();
            },
            [](UnsignedInt) -> UnsignedInt {
                CORRADE_INTERNAL_ASSERT_UNREACHABLE();
            });

    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<BaseLayerGL>(layer, layerShared));

    NodeHandle node = ui.createNode({8.0f, 8.0f}, {112.0f, 48.0f});
    ui.layer<BaseLayerGL>(layer).create(0, node);

    ui.draw();

    MAGNUM_VERIFY_NO_GL_ERROR();
    Image2D before = _framebuffer.read({{}, RenderSize}, {PixelFormat::RGBA8Unorm});

    PointerEvent event{Pointer::MouseLeft};
    CORRADE_VERIFY(ui.pointerPressEvent({64.0f, 24.0f}, event));
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataUpdate);

    ui.draw();

    MAGNUM_VERIFY_NO_GL_ERROR();
    Image2D after = _framebuffer.read({{}, RenderSize}, {PixelFormat::RGBA8Unorm});

    if(!(_manager.load("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_manager.load("StbImageImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / StbImageImporter plugins not found.");

    #if defined(MAGNUM_TARGET_GLES) && !defined(MAGNUM_TARGET_WEBGL)
    /* Same problem is with all builtin shaders, so this doesn't seem to be a
       bug in the base layer shader code */
    if(GL::Context::current().detectedDriver() & GL::Context::DetectedDriver::SwiftShader)
        CORRADE_SKIP("UBOs with dynamically indexed arrays don't seem to work on SwiftShader, can't test.");
    #endif
    CORRADE_COMPARE_WITH(before,
        Utility::Path::join(WHEE_TEST_DIR, "BaseLayerTestFiles/default.png"),
        DebugTools::CompareImageToFile{_manager});
    CORRADE_COMPARE_WITH(after,
        Utility::Path::join(WHEE_TEST_DIR, "BaseLayerTestFiles/gradient.png"),
        DebugTools::CompareImageToFile{_manager});
}

}}}}

CORRADE_TEST_MAIN(Magnum::Whee::Test::BaseLayerGLTest)
