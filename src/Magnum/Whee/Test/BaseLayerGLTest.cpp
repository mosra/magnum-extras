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
    void constructCopy();
    void constructMove();

    void setTextureTexturingNotEnabled();

    void drawNoSizeSet();
    void drawNoStyleSet();
    void drawNoTextureSet();

    void renderSetup();
    void renderTeardown();
    void render();
    void renderCustomColor();
    void renderCustomOutlineWidth();
    void renderPadding();
    void renderChangeStyle();
    void renderTextured();

    void renderOrDrawCompositeSetup();
    void renderOrDrawCompositeTeardown();

    void renderComposite();

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

using namespace Math::Literals;

const struct {
    const char* name;
    const char* filename;
    BaseLayerGL::Shared::Flags flags;
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
        BaseLayerGL::Shared::Flag::NoRoundedCorners,
        BaseLayerCommonStyleUniform{},
            /* Smoothness omitted to match the other image */
        BaseLayerStyleUniform{}
            .setOutlineColor(0x7f7f7f_rgbf)
            .setInnerOutlineCornerRadius(8.0f)
            .setOutlineWidth(8.0f)},
    {"outline, rounded corners, different inner and outer smoothness, no outline", "rounded-corners-same.png",
        BaseLayerGL::Shared::Flag::NoOutline,
        BaseLayerCommonStyleUniform{}
            .setSmoothness(1.0f, 8.0f),
        BaseLayerStyleUniform{}
            .setOutlineColor(0x7f7f7f_rgbf)
            /* Increased from 16 to match the other image */
            .setCornerRadius(24.0f)
            .setInnerOutlineCornerRadius(8.0f)
            .setOutlineWidth(8.0f)},
    {"outline, rounded corners, different inner and outer smoothness, no rounded corners, no outline", "default-smooth.png",
        BaseLayerGL::Shared::Flag::NoRoundedCorners|BaseLayerGL::Shared::Flag::NoOutline,
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
    BaseLayerGL::Shared::Flags flags;
} RenderCustomColorOutlineWidthData[]{
    {"", "outline-same.png",
        false, false, {}},
    {"set later", "outline-same.png",
        true, false, {}},
    {"set later, partial update", "outline-same.png",
        true, true, {}},
    {"no outline", "default.png",
        false, false, BaseLayerGL::Shared::Flag::NoOutline}
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
    const char* filename;
    bool rvalue;
    Containers::Optional<Vector3> offset;
    Containers::Optional<Vector2> size;
    BaseLayerStyleUniform styleUniform;
} RenderTexturedData[]{
    {"default offset and size", "textured-default.png",
        false, {}, {},
        BaseLayerStyleUniform{}},
    {"", "textured.png",
        false,
        /* The image is 160x106, want to render the bottom right 112x48 portion
           of it to avoid nasty scaling, and to verify the offset is taken from
           the right (bottom left) origin */
        {{48.0f/160.0f, 0.0f/106.0f, 7}}, {{112.0f/160.0f, 48.0f/106.0f}},
        BaseLayerStyleUniform{}},
    {"r-value instance", "textured.png",
        true,
        {{48.0f/160.0f, 0.0f/106.0f, 7}}, {{112.0f/160.0f, 48.0f/106.0f}},
        BaseLayerStyleUniform{}},
    {"colored", "textured-colored.png",
        false,
        /* Top left part of the image instead */
        {{0.0f/160.0f, 58.0f/106.0f, 7}}, {{112.0f/160.0f, 48.0f/106.0f}},
        BaseLayerStyleUniform{}
            .setCornerRadius(12.0f)
            .setInnerOutlineCornerRadius(4.0f)
            /* The gradient should be multiplied with the texture */
            .setColor(0xffffff_rgbf, 0x333333_rgbf)
            /* The outline shouldn't be multiplied with the texture, neither
               the texture should shine through if semi-transparent */
            .setOutlineWidth(8.0f)
            .setOutlineColor(0xa5c9eaff_rgbf*0.75f)},
};

const struct {
    const char* name;
    const char* filename;
    BaseLayerGL::Shared::Flags flags;
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
        BaseLayerGL::Shared::Flag::BackgroundBlur, {}, {}, {},
        BaseLayerCommonStyleUniform{}
            .setSmoothness(1.0f),
        BaseLayerStyleUniform{}
            .setCornerRadius(12.0f)
            .setColor(0xffffffff_rgbaf),
        0.0f, 0.0f},
    {"background blur, 50% opacity", "composite-background-blur-50.png",
        BaseLayerGL::Shared::Flag::BackgroundBlur, {}, {}, {},
        BaseLayerCommonStyleUniform{}
            .setSmoothness(1.0f),
        BaseLayerStyleUniform{}
            .setCornerRadius(12.0f)
            /* Premultiplied alpha */
            .setColor(0xffffffff_rgbaf*0.5f),
        0.0f, 0.0f},
    {"background blur, 75% opacity, colored", "composite-background-blur-75-colored.png",
        BaseLayerGL::Shared::Flag::BackgroundBlur, {}, {}, {},
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
        BaseLayerGL::Shared::Flag::BackgroundBlur, 0, {}, {},
        BaseLayerCommonStyleUniform{}
            .setSmoothness(1.0f),
        BaseLayerStyleUniform{}
            .setCornerRadius(12.0f)
            /* Premultiplied alpha */
            .setColor(0xffffffff_rgbaf*0.5f),
        0.75f, 0.236f},
    /* Should be the same as the default */
    {"background blur, 50% opacity, radius 4", "composite-background-blur-50.png",
        BaseLayerGL::Shared::Flag::BackgroundBlur, 4, {}, {},
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
        BaseLayerGL::Shared::Flag::BackgroundBlur, 2, {}, 4,
        BaseLayerCommonStyleUniform{}
            .setSmoothness(1.0f),
        BaseLayerStyleUniform{}
            .setCornerRadius(12.0f)
            /* Premultiplied alpha */
            .setColor(0xffffffff_rgbaf*0.5f),
        5.75f, 0.723f},
    /* sqrt(16*(1^2)) == 4, so should ~same as above (plus even more rounding
       errors) */
    {"background blur, 50% opacity, radius 1, 16 passes", "composite-background-blur-50.png",
        BaseLayerGL::Shared::Flag::BackgroundBlur, 1, {}, 16,
        BaseLayerCommonStyleUniform{}
            .setSmoothness(1.0f),
        BaseLayerStyleUniform{}
            .setCornerRadius(12.0f)
            /* Premultiplied alpha */
            .setColor(0xffffffff_rgbaf*0.5f),
        12.25f, 1.542f},
    {"background blur, 50% opacity, radius 31", "composite-background-blur-50-r31.png",
        BaseLayerGL::Shared::Flag::BackgroundBlur, 31, {}, {},
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
        BaseLayerGL::Shared::Flag::BackgroundBlur, 31, 0.5f/255.0f, {},
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
        BaseLayerGL::Shared::Flag::BackgroundBlur, 31, 1.0f, {},
        BaseLayerCommonStyleUniform{}
            .setSmoothness(1.0f),
        BaseLayerStyleUniform{}
            .setCornerRadius(12.0f)
            /* Premultiplied alpha */
            .setColor(0xffffffff_rgbaf*0.5f),
        0.75f, 0.236f},
    {"background blur, 50% opacity, radius 31, 80% blur opacity", "composite-background-blur-50-r31-80.png",
        BaseLayerGL::Shared::Flag::BackgroundBlur, 31, {}, {},
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
        BaseLayerGL::Shared::Flag::BackgroundBlur, 31, {}, {},
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
    bool dataInNodeOrder;
} DrawOrderData[]{
    {"data created in node order", true},
    {"data created randomly", false}
};

const struct {
    const char* name;
    const char* filename;
    BaseLayerGL::Shared::Flags flags;
} DrawOrderCompositeData[]{
    {"default", "draw-order-composite-default.png",
        {}},
    {"background blur", "draw-order-composite-background-blur.png",
        BaseLayerGL::Shared::Flag::BackgroundBlur}
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
              &BaseLayerGLTest::constructCopy,
              &BaseLayerGLTest::constructMove,

              &BaseLayerGLTest::setTextureTexturingNotEnabled,

              &BaseLayerGLTest::drawNoSizeSet,
              &BaseLayerGLTest::drawNoStyleSet,
              &BaseLayerGLTest::drawNoTextureSet});

    addInstancedTests({&BaseLayerGLTest::render},
        Containers::arraySize(RenderData),
        &BaseLayerGLTest::renderSetup,
        &BaseLayerGLTest::renderTeardown);

    addInstancedTests({&BaseLayerGLTest::renderCustomColor,
                       &BaseLayerGLTest::renderCustomOutlineWidth},
        Containers::arraySize(RenderCustomColorOutlineWidthData),
        &BaseLayerGLTest::renderSetup,
        &BaseLayerGLTest::renderTeardown);

    addInstancedTests({&BaseLayerGLTest::renderPadding},
        Containers::arraySize(RenderPaddingData),
        &BaseLayerGLTest::renderSetup,
        &BaseLayerGLTest::renderTeardown);

    addInstancedTests({&BaseLayerGLTest::renderChangeStyle},
        Containers::arraySize(RenderChangeStyleData),
        &BaseLayerGLTest::renderSetup,
        &BaseLayerGLTest::renderTeardown);

    addInstancedTests({&BaseLayerGLTest::renderTextured},
        Containers::arraySize(RenderTexturedData),
        &BaseLayerGLTest::renderSetup,
        &BaseLayerGLTest::renderTeardown);

    addInstancedTests({&BaseLayerGLTest::renderComposite},
        Containers::arraySize(RenderCompositeData),
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
    CORRADE_COMPARE(shared.flags(), BaseLayerGL::Shared::Flags{});
}

void BaseLayerGLTest::sharedConstructComposite() {
    BaseLayerGL::Shared shared{
        BaseLayerGL::Shared::Configuration{3, 5}
            .addFlags(BaseLayerGL::Shared::Flag::BackgroundBlur)};
    CORRADE_COMPARE(shared.styleUniformCount(), 3);
    CORRADE_COMPARE(shared.styleCount(), 5);
    CORRADE_COMPARE(shared.flags(), BaseLayerGL::Shared::Flag::BackgroundBlur);
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
    CORRADE_SKIP_IF_NO_ASSERT();

    BaseLayerGL::Shared shared{BaseLayer::Shared::Configuration{3}};
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
        .addFlags(BaseLayer::Shared::Flag::Textured)};
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

void BaseLayerGLTest::render() {
    auto&& data = RenderData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

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
            .setFlags(data.flags)
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

void BaseLayerGLTest::renderCustomColor() {
    auto&& data = RenderCustomColorOutlineWidthData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Basically the same as the "gradient" case in render(), except that the
       color is additionally taken from the per-vertex data as well */

    AbstractUserInterface ui{RenderSize};
    ui.setRendererInstance(Containers::pointer<RendererGL>());

    BaseLayerGL::Shared layerShared{BaseLayer::Shared::Configuration{1}};
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

void BaseLayerGLTest::renderCustomOutlineWidth() {
    auto&& data = RenderCustomColorOutlineWidthData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Like the "outline, all sides same" case in render(), except that the
       width is additionally taken from the per-vertex data as well. And tests
       that the custom outline specified in the data isn't taken into account
       in any way when outlines are disabled. */

    AbstractUserInterface ui{RenderSize};
    ui.setRendererInstance(Containers::pointer<RendererGL>());

    BaseLayerGL::Shared layerShared{BaseLayer::Shared::Configuration{1}
        .setFlags(data.flags)
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

void BaseLayerGLTest::renderPadding() {
    auto&& data = RenderPaddingData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Basically the same as the "outline, rounded corners, different" case in
       render(), except that the node offset, size and style or data padding
       changes. The result should always be the same as if the padding was
       applied directly to the node offset and size itself. */

    AbstractUserInterface ui{RenderSize};
    ui.setRendererInstance(Containers::pointer<RendererGL>());

    BaseLayerGL::Shared layerShared{BaseLayer::Shared::Configuration{1}};
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

void BaseLayerGLTest::renderChangeStyle() {
    auto&& data = RenderChangeStyleData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Basically the same as the "gradient" case in render(), except that the
       style ID is changed to it only later. */

    AbstractUserInterface ui{RenderSize};
    ui.setRendererInstance(Containers::pointer<RendererGL>());

    BaseLayerGL::Shared layerShared{BaseLayer::Shared::Configuration{2}};
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

void BaseLayerGLTest::renderTextured() {
    auto&& data = RenderTexturedData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    if(!(_manager.load("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_manager.load("StbImageImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / StbImageImporter plugins not found.");

    /* Abusing the blur input image for a texture test */
    Containers::Pointer<Trade::AbstractImporter> importer = _manager.loadAndInstantiate("AnyImageImporter");
    CORRADE_VERIFY(importer->openFile(Utility::Path::join(WHEE_TEST_DIR, "BaseLayerTestFiles/blur-input.png")));

    Containers::Optional<Trade::ImageData2D> image = importer->image2D(0);
    CORRADE_VERIFY(image);

    GL::Texture2DArray texture;
    texture
        .setMinificationFilter(GL::SamplerFilter::Linear)
        .setMagnificationFilter(GL::SamplerFilter::Linear)
        .setStorage(1, GL::textureFormat(image->format()), Vector3i{image->size(), 8})
        .setSubImage(0, {0, 0, data.offset ? Int(data.offset->z()) : 0}, ImageView2D{*image});

    AbstractUserInterface ui{RenderSize};
    ui.setRendererInstance(Containers::pointer<RendererGL>());

    BaseLayerGL::Shared layerShared{
        BaseLayer::Shared::Configuration{2}
            .addFlags(BaseLayer::Shared::Flag::Textured)};
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

void BaseLayerGLTest::renderComposite() {
    auto&& data = RenderCompositeData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

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
    configuration.addFlags(data.flags);
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
            [](UnsignedInt style) -> UnsignedInt {
                if(style == 0) return 1;
                CORRADE_INTERNAL_ASSERT_UNREACHABLE();
            },
            [](UnsignedInt) -> UnsignedInt {
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
