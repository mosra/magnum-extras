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

#include <Corrade/Containers/BitArrayView.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Containers/StringIterable.h>
#include <Corrade/PluginManager/Manager.h>
#include <Corrade/PluginManager/PluginMetadata.h>
#include <Corrade/TestSuite/Compare/Numeric.h>
#include <Corrade/Utility/ConfigurationGroup.h>
#include <Corrade/Utility/Path.h>
#include <Magnum/Image.h>
#include <Magnum/ImageView.h>
#include <Magnum/DebugTools/CompareImage.h>
#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/OpenGLTester.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/Trade/AbstractImporter.h>

#include "Magnum/Ui/AbstractUserInterface.h"
#include "Magnum/Ui/Event.h"
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/LineLayerGL.h"
#include "Magnum/Ui/NodeFlags.h"
#include "Magnum/Ui/RendererGL.h"

#include "configure.h"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct LineLayerGLTest: GL::OpenGLTester {
    explicit LineLayerGLTest();

    void sharedConstruct();
    void sharedConstructCopy();
    void sharedConstructMove();

    void construct();
    void constructDerived();
    void constructCopy();
    void constructMove();

    void drawNoSizeSet();
    void drawNoStyleSet();

    void renderSetup();
    void renderTeardown();
    void render();
    void renderStrip();
    void renderLoop();
    void renderSmoothness();
    void renderCustomColor();
    void renderPaddingAlignment();
    void renderChangeStyle();
    void renderChangeLine();

    void drawSetup();
    void drawTeardown();
    void drawOrder();

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
    Containers::Optional<LineCapStyle> capStyle;
    Containers::Optional<LineJoinStyle> joinStyle;
    LineLayerCommonStyleUniform styleUniformCommon;
    LineLayerStyleUniform styleUniform;
    Containers::Array<UnsignedInt> indices;
    Containers::Array<Vector2> points;
    Containers::Array<Color4> colors;
} RenderData[]{
    {"default", "default.png", {}, {},
        LineLayerCommonStyleUniform{},
        LineLayerStyleUniform{},
        {InPlaceInit, {0, 1, 1, 2, 2, 3, 4, 5, 6, 7, 8, 8}},
        {InPlaceInit, {
            {-48.0f, -16.0f},
            {-16.0f,  16.0f},
            { 16.0f, -16.0f},
            { 16.0f,  16.0f},
            /* These two lines overlap */
            { 36.0f,   0.0f},
            { 52.0f,   0.0f},
            { 44.0f,  16.0f},
            { 44.0f, -16.0f},
            /* Standalone point */
            {-16.0f, -16.0f},
        }}, {}},
    {"default joins and caps", "square-miter.png", {}, {},
        LineLayerCommonStyleUniform{}
            .setSmoothness(1.0f),
        LineLayerStyleUniform{}
            .setWidth(12.0f)
            /* Semi-transparent to verify there are no overlaps except where
               desired */
            .setColor(0xffffffff_rgbaf*0.75f),
        {InPlaceInit, {0, 1, 1, 2, 2, 3, 4, 5, 6, 7, 8, 8}},
        {InPlaceInit, {
            {-48.0f, -16.0f},
            {-16.0f,  16.0f},
            { 16.0f, -16.0f},
            { 16.0f,  16.0f},
            /* These two lines overlap */
            { 36.0f,   0.0f},
            { 52.0f,   0.0f},
            { 44.0f,  16.0f},
            { 44.0f, -16.0f},
            /* Standalone point */
            {-16.0f, -16.0f},
        }}, {}},
    {"square caps, miter joins", "square-miter.png",
        LineCapStyle::Square, LineJoinStyle::Miter,
        LineLayerCommonStyleUniform{}
            .setSmoothness(1.0f),
        LineLayerStyleUniform{}
            .setWidth(12.0f)
            /* Semi-transparent to verify there are no overlaps except where
               desired */
            .setColor(0xffffffff_rgbaf*0.75f),
        {InPlaceInit, {0, 1, 1, 2, 2, 3, 4, 5, 6, 7, 8, 8}},
        {InPlaceInit, {
            {-48.0f, -16.0f},
            {-16.0f,  16.0f},
            { 16.0f, -16.0f},
            { 16.0f,  16.0f},
            /* These two lines overlap */
            { 36.0f,   0.0f},
            { 52.0f,   0.0f},
            { 44.0f,  16.0f},
            { 44.0f, -16.0f},
            /* Standalone point */
            {-16.0f, -16.0f},
        }}, {}},
    {"triangle caps, miter joins limited to 89", "triangle-miter-limit.png",
        LineCapStyle::Triangle, LineJoinStyle::Miter,
        LineLayerCommonStyleUniform{}
            .setSmoothness(1.0f),
        LineLayerStyleUniform{}
            .setWidth(12.0f)
            .setMiterAngleLimit(89.0_degf)
            /* Semi-transparent to verify there are no overlaps except where
               desired */
            .setColor(0xffffffff_rgbaf*0.75f),
        {InPlaceInit, {0, 1, 1, 2, 2, 3, 4, 5, 6, 7, 8, 8}},
        {InPlaceInit, {
            {-48.0f, -16.0f},
            {-16.0f,  16.0f},
            { 16.0f, -16.0f},
            { 16.0f,  16.0f},
            /* These two lines overlap */
            { 36.0f,   0.0f},
            { 52.0f,   0.0f},
            { 44.0f,  16.0f},
            { 44.0f, -16.0f},
            /* Standalone point */
            {-16.0f, -16.0f},
        }}, {}},
    /** @todo change this to round joins once implemented */
    {"round caps, bevel joins", "round-bevel.png",
        LineCapStyle::Round, LineJoinStyle::Bevel,
        LineLayerCommonStyleUniform{}
            .setSmoothness(1.0f),
        LineLayerStyleUniform{}
            .setWidth(12.0f)
            /* Semi-transparent to verify there are no overlaps except where
               desired */
            .setColor(0xffffffff_rgbaf*0.75f),
        {InPlaceInit, {0, 1, 1, 2, 2, 3, 4, 5, 6, 7, 8, 8}},
        {InPlaceInit, {
            {-48.0f, -16.0f},
            {-16.0f,  16.0f},
            { 16.0f, -16.0f},
            { 16.0f,  16.0f},
            /* These two lines overlap */
            { 36.0f,   0.0f},
            { 52.0f,   0.0f},
            { 44.0f,  16.0f},
            { 44.0f, -16.0f},
            /* Standalone point */
            {-16.0f, -16.0f},
        }}, {}},
    {"butt caps, bevel joins", "butt-bevel.png",
        LineCapStyle::Butt, LineJoinStyle::Bevel,
        LineLayerCommonStyleUniform{}
            .setSmoothness(1.0f),
        LineLayerStyleUniform{}
            .setWidth(12.0f)
            /* Semi-transparent to verify there are no overlaps except where
               desired */
            .setColor(0xffffffff_rgbaf*0.75f),
        {InPlaceInit, {0, 1, 1, 2, 2, 3, 4, 5, 6, 7, 8, 8}},
        {InPlaceInit, {
            {-48.0f, -16.0f},
            {-16.0f,  16.0f},
            { 16.0f, -16.0f},
            { 16.0f,  16.0f},
            /* These two lines overlap */
            { 36.0f,   0.0f},
            { 52.0f,   0.0f},
            { 44.0f,  16.0f},
            { 44.0f, -16.0f},
            /* Standalone point */
            {-16.0f, -16.0f},
        }}, {}},
    {"per-point colors", "color.png", {}, {},
        LineLayerCommonStyleUniform{},
        LineLayerStyleUniform{}
            .setWidth(20.0f),
        {InPlaceInit, {0, 1, 2, 3, 4, 5, 5, 6, 6, 7}},
        {InPlaceInit, {
            {-32.0f, -16.0f},
            {-32.0f,  16.0f},
            { 32.0f, -16.0f},
            { 32.0f,  16.0f},
            {-48.0f, 0.0f},
            {-16.0f, 0.0f},
            { 16.0f, 0.0f},
            { 48.0f, 0.0f},
        }}, {InPlaceInit, {
            0xffffffff_rgbaf,
            0xffffffff_rgbaf,
            0xffffffff_rgbaf,
            0xffffffff_rgbaf,
            0x2f83ccff_rgbaf*1.00f,
            0x3bd267ff_rgbaf*0.75f,
            0xc7cf2fff_rgbaf*0.50f,
            0xcd3431ff_rgbaf*0.25f,
        }}},
    {"per-point colors multiplied with per-style", "color.png", {}, {},
        LineLayerCommonStyleUniform{},
        LineLayerStyleUniform{}
            .setWidth(20.0f)
            .setColor(0x336699cc_rgbaf),
        {InPlaceInit, {0, 1, 2, 3, 4, 5, 5, 6, 6, 7}},
        {InPlaceInit, {
            {-32.0f, -16.0f},
            {-32.0f,  16.0f},
            { 32.0f, -16.0f},
            { 32.0f,  16.0f},
            {-48.0f, 0.0f},
            {-16.0f, 0.0f},
            { 16.0f, 0.0f},
            { 48.0f, 0.0f},
        }}, {InPlaceInit, {
            0xffffffff_rgbaf/0x336699cc_rgbaf,
            0xffffffff_rgbaf/0x336699cc_rgbaf,
            0xffffffff_rgbaf/0x336699cc_rgbaf,
            0xffffffff_rgbaf/0x336699cc_rgbaf,
            0x2f83ccff_rgbaf*1.00f/0x336699cc_rgbaf,
            0x3bd267ff_rgbaf*0.75f/0x336699cc_rgbaf,
            0xc7cf2fff_rgbaf*0.50f/0x336699cc_rgbaf,
            0xcd3431ff_rgbaf*0.25f/0x336699cc_rgbaf,
        }}},
    /* same as renderStrip() but using an explicit index buffer here to verify
       they're visually equivalent */
    {"strip", "strip.png", {}, {},
        LineLayerCommonStyleUniform{},
        LineLayerStyleUniform{}
            .setWidth(12.0f)
            /* Semi-transparent to verify there are no overlaps except where
               desired */
            .setColor(0xffffffff_rgbaf*0.75f),
        {InPlaceInit, {0, 1, 1, 2, 2, 3}},
        {InPlaceInit, {
            {-48.0f, -16.0f},
            { 48.0f, -16.0f},
            { 48.0f,  16.0f},
            {-48.0f,  16.0f},
        }}, {}},
    /* same as renderLoop() but using an explicit index buffer here to verify
       they're visually equivalent */
    {"loop", "loop.png", {}, {},
        LineLayerCommonStyleUniform{},
        LineLayerStyleUniform{}
            .setWidth(12.0f)
            /* Semi-transparent to verify there are no overlaps except where
               desired */
            .setColor(0xffffffff_rgbaf*0.75f),
        {InPlaceInit, {0, 1, 1, 2, 2, 3, 3, 0}},
        {InPlaceInit, {
            {-48.0f, -16.0f},
            { 48.0f, -16.0f},
            { 48.0f,  16.0f},
            {-48.0f,  16.0f},
        }}, {}},
};

const struct {
    const char* name;
    Float uiScale;
    LineLayerCommonStyleUniform styleUniformCommon;
    LineLayerStyleUniform styleUniform;
} RenderSmoothnessData[]{
    {"common smoothness", 1.0f,
        LineLayerCommonStyleUniform{}
            .setSmoothness(8.0f),
        LineLayerStyleUniform{}
            .setWidth(12.0f)
            /* Semi-transparent to verify there are no overlaps except where
               desired */
            .setColor(0xffffffff_rgbaf*0.75f)},
    {"common smoothness, UI size 10x larger", 10.0f,
        LineLayerCommonStyleUniform{}
            .setSmoothness(8.0f),   /* in pixels, no change */
        LineLayerStyleUniform{}
            .setWidth(120.0f)
            .setColor(0xffffffff_rgbaf*0.75f)},
    {"common smoothness, UI size 10x smaller", 0.1f,
        LineLayerCommonStyleUniform{}
            .setSmoothness(8.0f),   /* in pixels, no change */
        LineLayerStyleUniform{}
            .setWidth(1.2f)
            .setColor(0xffffffff_rgbaf*0.75f)},
    {"per-style smoothness", 1.0f,
        LineLayerCommonStyleUniform{},
        LineLayerStyleUniform{}
            .setWidth(12.0f)
            .setSmoothness(8.0f)
            .setColor(0xffffffff_rgbaf*0.75f)},
    {"per-style smoothness, UI size 10x larger", 10.0f,
        LineLayerCommonStyleUniform{},
        LineLayerStyleUniform{}
            .setWidth(120.0f)       /* in UI units, so also 10x larger */
            .setSmoothness(80.0f)   /* in UI units, so also 10x larger */
            .setColor(0xffffffff_rgbaf*0.75f)},
    {"per-style smoothness, UI size 10x smaller", 0.1f,
        LineLayerCommonStyleUniform{},
        LineLayerStyleUniform{}
            .setWidth(1.2f)         /* in UI units, so also 10x smaller */
            .setSmoothness(0.8f)    /* in UI units, so also 10x smaller */
            .setColor(0xffffffff_rgbaf*0.75f)},
    {"both common and per-style smoothness, common is larger", 1.0f,
        LineLayerCommonStyleUniform{}
            .setSmoothness(8.0f),
        LineLayerStyleUniform{}
            .setWidth(12.0f)
            .setSmoothness(7.0f)
            .setColor(0xffffffff_rgbaf*0.75f)},
    {"both common and per-style smoothness, common is larger, UI size 10x larger", 10.0f,
        LineLayerCommonStyleUniform{}
            .setSmoothness(8.0f),   /* in pixels, no change */
        LineLayerStyleUniform{}
            .setWidth(120.0f)       /* in UI units, so also 10x larger */
            .setSmoothness(70.0f)   /* in UI units, so also 10x larger */
            .setColor(0xffffffff_rgbaf*0.75f)},
    {"both common and per-style smoothness, common is larger, UI size 10x smaller", 0.1f,
        LineLayerCommonStyleUniform{}
            .setSmoothness(8.0f),   /* in pixels, no change */
        LineLayerStyleUniform{}
            .setWidth(1.2f)         /* in UI units, so also 10x smaller */
            .setSmoothness(0.7f)    /* in UI units, so also 10x smaller */
            .setColor(0xffffffff_rgbaf*0.75f)},
    {"both common and per-style smoothness, per-style is larger", 1.0f,
        LineLayerCommonStyleUniform{}
            .setSmoothness(7.0f),
        LineLayerStyleUniform{}
            .setWidth(12.0f)
            .setSmoothness(8.0f)
            .setColor(0xffffffff_rgbaf*0.75f)},
    {"both common and per-style smoothness, per-style is larger, UI size 10x larger", 10.0f,
        LineLayerCommonStyleUniform{}
            .setSmoothness(7.0f),   /* in pixels, no change */
        LineLayerStyleUniform{}
            .setWidth(120.0f)       /* in UI units, so also 10x larger */
            .setSmoothness(80.0f)   /* in UI units, so also 10x larger */
            .setColor(0xffffffff_rgbaf*0.75f)},
    {"both common and per-style smoothness, per-style is larger, UI size 10x smaller", 0.1f,
        LineLayerCommonStyleUniform{}
            .setSmoothness(7.0f),   /* in pixels, no change */
        LineLayerStyleUniform{}
            .setWidth(1.2f)         /* in UI units, so also 10x smaller */
            .setSmoothness(0.8f)    /* in UI units, so also 10x smaller */
            .setColor(0xffffffff_rgbaf*0.75f)},
};

const struct {
    const char* name;
    bool partialUpdate;
    Float opacity;
} RenderCustomColorData[]{
    {"", false, 1.0f},
    {"partial update", true, 1.0f},
    {"node opacity", false, 0.75f},
    {"node opacity, partial update", true, 0.75f},
};

const struct {
    const char* name;
    bool partialUpdate;
    Vector2 nodeOffset, nodeSize;
    Vector4 paddingFromStyle, paddingFromData;
    LineAlignment alignmentFromStyle;
    Containers::Optional<LineAlignment> alignmentFromData;
    Vector2 pointOffset;
} RenderPaddingAlignmentData[]{
    {"no padding, default middle center alignment from style", false,
        {8.0f, 8.0f}, {112.0f, 48.0f}, {}, {},
        LineAlignment{}, {}, {}},
    /* Deliberately having one excessively shifted to left/top and the other to
       bottom/right. It shouldn't cause any strange artifacts. */
    {"padding from style", false,
        {-64.0f, -128.0f}, {192.0f, 192.0f},
        {72.0f, 136.0f, 8.0f, 8.0f}, {},
        LineAlignment{}, {}, {}},
    {"padding from data", false,
        {0.0f, 0.0f}, {192.0f, 192.0f},
        {}, {8.0f, 8.0f, 72.0f, 136.0f},
        LineAlignment{}, {}, {}},
    {"padding from both", false,
        {0.0f, 0.0f}, {128.0f, 64.0f},
        {4.0f, 8.0f, 0.0f, 4.0f}, {4.0f, 0.0f, 8.0f, 4.0f},
        LineAlignment{}, {}, {}},
    {"alignment from style", false,
        {8.0f, 8.0f}, {112.0f, 48.0f}, {}, {},
        LineAlignment::BottomLeft, {}, {56.0f, -24.0f}},
    {"alignment from data", false,
        {8.0f, 8.0f}, {112.0f, 48.0f}, {}, {},
        /* The alignment from data should be picked */
        LineAlignment{}, LineAlignment::TopRight, {-56.0f, 24.0f}},
    {"alignment from both", false,
        {8.0f, 8.0f}, {112.0f, 48.0f}, {}, {},
        /* The alignment from data should be picked */
        LineAlignment::TopLeft, LineAlignment::BottomRight, {-56.0f, -24.0f}},
    {"padding and alignment from style", false,
        {-64.0f, -128.0f}, {192.0f, 192.0f},
        {72.0f, 136.0f, 8.0f, 8.0f}, {},
        LineAlignment::BottomLeft, {}, {56.0f, -24.0f}},
    {"padding and alignment from data", false,
        {0.0f, 0.0f}, {192.0f, 192.0f},
        {}, {8.0f, 8.0f, 72.0f, 136.0f},
        /* The alignment from data should be picked */
        LineAlignment{}, LineAlignment::TopRight, {-56.0f, 24.0f}},
    {"padding and alignment from both", false,
        {0.0f, 0.0f}, {128.0f, 64.0f},
        {4.0f, 8.0f, 0.0f, 4.0f}, {4.0f, 0.0f, 8.0f, 4.0f},
        /* The alignment from data should be picked */
        LineAlignment::TopLeft, LineAlignment::BottomRight, {-56.0f, -24.0f}},
    {"padding and alignment from both, partial update", true,
        {0.0f, 0.0f}, {128.0f, 64.0f},
        {4.0f, 8.0f, 0.0f, 4.0f}, {4.0f, 0.0f, 8.0f, 4.0f},
        /* The alignment from data should be picked */
        LineAlignment::TopLeft, LineAlignment::BottomRight, {-56.0f, -24.0f}},
    {"padding from both, alignment from style only, partial update", true,
        {0.0f, 0.0f}, {128.0f, 64.0f},
        {4.0f, 8.0f, 0.0f, 4.0f}, {4.0f, 0.0f, 8.0f, 4.0f},
        LineAlignment::BottomRight, {}, {-56.0f, -24.0f}},
    {"alignment from both, padding from style only, partial update", true,
        {-64.0f, -128.0f}, {192.0f, 192.0f},
        {72.0f, 136.0f, 8.0f, 8.0f}, {},
        /* The alignment from data should be picked */
        LineAlignment::TopLeft, LineAlignment::BottomRight, {-56.0f, -24.0f}},
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
    bool indexed, strip, loop;
    bool partialUpdate;
} RenderChangeLineData[]{
    {"to indexed", "strip.png", true, false, false, false},
    {"to indexed, partial update", "strip.png", true, false, false, true},
    {"to a strip", "strip.png", false, true, false, false},
    {"to a strip, partial update", "strip.png", false, true, false, true},
    {"to a loop", "loop.png", false, false, true, false},
    {"to a loop, partial update", "loop.png", false, false, true, true},
};

const struct {
    const char* name;
    bool dataInNodeOrder;
} DrawOrderData[]{
    {"data created in node order", true},
    {"data created randomly", false}
};

LineLayerGLTest::LineLayerGLTest() {
    addTests({&LineLayerGLTest::sharedConstruct,
              &LineLayerGLTest::sharedConstructCopy,
              &LineLayerGLTest::sharedConstructMove,

              &LineLayerGLTest::construct,
              &LineLayerGLTest::constructDerived,
              &LineLayerGLTest::constructCopy,
              &LineLayerGLTest::constructMove,

              &LineLayerGLTest::drawNoSizeSet,
              &LineLayerGLTest::drawNoStyleSet});

    addInstancedTests({&LineLayerGLTest::render},
        Containers::arraySize(RenderData),
        &LineLayerGLTest::renderSetup,
        &LineLayerGLTest::renderTeardown);

    addTests({&LineLayerGLTest::renderStrip,
              &LineLayerGLTest::renderLoop},
        &LineLayerGLTest::renderSetup,
        &LineLayerGLTest::renderTeardown);

    addInstancedTests({&LineLayerGLTest::renderSmoothness},
        Containers::arraySize(RenderSmoothnessData),
        &LineLayerGLTest::renderSetup,
        &LineLayerGLTest::renderTeardown);

    addInstancedTests({&LineLayerGLTest::renderCustomColor},
        Containers::arraySize(RenderCustomColorData),
        &LineLayerGLTest::renderSetup,
        &LineLayerGLTest::renderTeardown);

    addInstancedTests({&LineLayerGLTest::renderPaddingAlignment},
        Containers::arraySize(RenderPaddingAlignmentData),
        &LineLayerGLTest::renderSetup,
        &LineLayerGLTest::renderTeardown);

    addInstancedTests({&LineLayerGLTest::renderChangeStyle},
        Containers::arraySize(RenderChangeStyleData),
        &LineLayerGLTest::renderSetup,
        &LineLayerGLTest::renderTeardown);

    addInstancedTests({&LineLayerGLTest::renderChangeLine},
        Containers::arraySize(RenderChangeLineData),
        &LineLayerGLTest::renderSetup,
        &LineLayerGLTest::renderTeardown);

    addInstancedTests({&LineLayerGLTest::drawOrder},
        Containers::arraySize(DrawOrderData),
        &LineLayerGLTest::drawSetup,
        &LineLayerGLTest::drawTeardown);

    addTests({&LineLayerGLTest::eventStyleTransition},
        &LineLayerGLTest::renderSetup,
        &LineLayerGLTest::renderTeardown);

    /* Prefer the StbImageImporter so we can keep files small but always import
       them as four-channel */
    if(PluginManager::PluginMetadata* metadata = _manager.metadata("StbImageImporter")) {
        metadata->configuration().setValue("forceChannelCount", 4);
        _manager.setPreferredPlugins("PngImporter", {"StbImageImporter"});
    }
}

void LineLayerGLTest::sharedConstruct() {
    LineLayerGL::Shared shared{LineLayer::Shared::Configuration{3, 5}};
    CORRADE_COMPARE(shared.styleUniformCount(), 3);
    CORRADE_COMPARE(shared.styleCount(), 5);
}

void LineLayerGLTest::sharedConstructCopy() {
    CORRADE_VERIFY(!std::is_copy_constructible<LineLayerGL::Shared>{});
    CORRADE_VERIFY(!std::is_copy_assignable<LineLayerGL::Shared>{});
}

void LineLayerGLTest::sharedConstructMove() {
    LineLayerGL::Shared a{LineLayer::Shared::Configuration{3}};

    LineLayerGL::Shared b{Utility::move(a)};
    CORRADE_COMPARE(b.styleCount(), 3);

    LineLayerGL::Shared c{LineLayer::Shared::Configuration{5}};
    c = Utility::move(b);
    CORRADE_COMPARE(c.styleCount(), 3);

    CORRADE_VERIFY(std::is_nothrow_move_constructible<LineLayerGL::Shared>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<LineLayerGL::Shared>::value);
}

void LineLayerGLTest::construct() {
    LineLayerGL::Shared shared{LineLayer::Shared::Configuration{3}};

    LineLayerGL layer{layerHandle(137, 0xfe), shared};
    CORRADE_COMPARE(layer.handle(), layerHandle(137, 0xfe));
    CORRADE_COMPARE(&layer.shared(), &shared);
    /* Const overload */
    CORRADE_COMPARE(&static_cast<const LineLayerGL&>(layer).shared(), &shared);
}

void LineLayerGLTest::constructDerived() {
    LineLayerGL::Shared shared{LineLayer::Shared::Configuration{3}};

    /* Verify just that subclassing works without hitting linker errors due to
       virtual symbols not being exported or due to the delegated-to functions
       being private */
    struct: LineLayerGL {
        using LineLayerGL::LineLayerGL;

        void doDraw(const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, std::size_t offset, std::size_t count, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, std::size_t clipRectOffset, std::size_t clipRectCount, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, const Containers::StridedArrayView1D<const Float>& nodeOpacities, Containers::BitArrayView nodesEnabled, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes) override {
            return LineLayerGL::doDraw(dataIds, offset, count, clipRectIds, clipRectDataCounts, clipRectOffset, clipRectCount, nodeOffsets, nodeSizes, nodeOpacities, nodesEnabled, clipRectOffsets, clipRectSizes);
        }
    } layer{layerHandle(137, 0xfe), shared};
    CORRADE_COMPARE(layer.handle(), layerHandle(137, 0xfe));
}

void LineLayerGLTest::constructCopy() {
    CORRADE_VERIFY(!std::is_copy_constructible<LineLayerGL>{});
    CORRADE_VERIFY(!std::is_copy_assignable<LineLayerGL>{});
}

void LineLayerGLTest::constructMove() {
    LineLayerGL::Shared shared{LineLayer::Shared::Configuration{3}};
    LineLayerGL::Shared shared2{LineLayer::Shared::Configuration{5}};

    LineLayerGL a{layerHandle(137, 0xfe), shared};

    LineLayerGL b{Utility::move(a)};
    CORRADE_COMPARE(b.handle(), layerHandle(137, 0xfe));
    CORRADE_COMPARE(&b.shared(), &shared);

    LineLayerGL c{layerHandle(0, 2), shared2};
    c = Utility::move(b);
    CORRADE_COMPARE(c.handle(), layerHandle(137, 0xfe));
    CORRADE_COMPARE(&c.shared(), &shared);

    CORRADE_VERIFY(std::is_nothrow_move_constructible<LineLayerGL>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<LineLayerGL>::value);
}

void LineLayerGLTest::drawNoSizeSet() {
    CORRADE_SKIP_IF_NO_ASSERT();

    LineLayerGL::Shared shared{LineLayer::Shared::Configuration{3}};
    LineLayerGL layer{layerHandle(0, 1), shared};

    Containers::String out;
    Error redirectError{&out};
    layer.draw({}, 0, 0, {}, {}, 0, 0, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(out, "Ui::LineLayerGL::draw(): user interface size wasn't set\n");
}

void LineLayerGLTest::drawNoStyleSet() {
    CORRADE_SKIP_IF_NO_ASSERT();

    LineLayerGL::Shared shared{LineLayer::Shared::Configuration{3}};
    LineLayerGL layer{layerHandle(0, 1), shared};

    layer.setSize({10, 10}, {10, 10});

    Containers::String out;
    Error redirectError{&out};
    layer.draw({}, 0, 0, {}, {}, 0, 0, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(out, "Ui::LineLayerGL::draw(): no style data was set\n");
}

constexpr Vector2i RenderSize{128, 64};

void LineLayerGLTest::renderSetup() {
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

void LineLayerGLTest::renderTeardown() {
    _framebuffer = GL::Framebuffer{NoCreate};
    _color = GL::Texture2D{NoCreate};

    GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::disable(GL::Renderer::Feature::ScissorTest);
    GL::Renderer::disable(GL::Renderer::Feature::Blending);
}

void LineLayerGLTest::render() {
    auto&& data = RenderData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    AbstractUserInterface ui{RenderSize};
    ui.setRendererInstance(Containers::pointer<RendererGL>());

    /* Testing the ArrayView overload, others cases use the initializer list */
    LineLayerStyleUniform styleUniforms[]{
        /* To verify it's not always picking the first uniform */
        LineLayerStyleUniform{},
        LineLayerStyleUniform{},
        data.styleUniform,
    };
    UnsignedInt styleToUniform[]{
        /* To verify it's not using the style ID as uniform ID */
        1, 2, 0, 1, 0
    };
    LineAlignment styleAlignment[Containers::arraySize(styleToUniform)]{};
    /* The (lack of any) effect of alignment or padding on rendered output is
       tested thoroughly in renderPadding() */
    LineLayer::Shared::Configuration configuration{
        UnsignedInt(Containers::arraySize(styleUniforms)),
        UnsignedInt(Containers::arraySize(styleToUniform))};
    if(data.capStyle)
        configuration.setCapStyle(*data.capStyle);
    if(data.joinStyle)
        configuration.setJoinStyle(*data.joinStyle);

    LineLayerGL::Shared layerShared{configuration};
    layerShared.setStyle(data.styleUniformCommon,
        styleUniforms,
        styleToUniform,
        styleAlignment,
        {});
    LineLayer& layer = ui.setLayerInstance(Containers::pointer<LineLayerGL>(ui.createLayer(), layerShared));

    NodeHandle node = ui.createNode({8.0f, 8.0f}, {112.0f, 48.0f});
    layer.create(1, data.indices, data.points, data.colors, node);

    ui.draw();

    MAGNUM_VERIFY_NO_GL_ERROR();

    if(!(_manager.load("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_manager.load("StbImageImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / StbImageImporter plugins not found.");

    #if defined(MAGNUM_TARGET_GLES) && !defined(MAGNUM_TARGET_WEBGL)
    /* Same problem is with all builtin shaders, so this doesn't seem to be a
       bug in the line layer shader code */
    if(GL::Context::current().detectedDriver() & GL::Context::DetectedDriver::SwiftShader)
        CORRADE_SKIP("UBOs with dynamically indexed arrays don't seem to work on SwiftShader, can't test.");
    #endif
    CORRADE_COMPARE_WITH(_framebuffer.read({{}, RenderSize}, {PixelFormat::RGBA8Unorm}),
        Utility::Path::join({UI_TEST_DIR, "LineLayerTestFiles", data.filename}),
        DebugTools::CompareImageToFile{_manager});
}

void LineLayerGLTest::renderStrip() {
    /* Like render(strip) but using createStrip() instead of an explicit index
       buffer to verify both behave the same */

    AbstractUserInterface ui{RenderSize};
    ui.setRendererInstance(Containers::pointer<RendererGL>());

    LineLayerGL::Shared layerShared{LineLayer::Shared::Configuration{1}};
    layerShared.setStyle(
        LineLayerCommonStyleUniform{},
        {LineLayerStyleUniform{}
            .setWidth(12.0f)
            /* Semi-transparent to verify there are no overlaps except where
               desired */
            .setColor(0xffffffff_rgbaf*0.75f)},
        {LineAlignment{}},
        {});
    LineLayer& layer = ui.setLayerInstance(Containers::pointer<LineLayerGL>(ui.createLayer(), layerShared));

    NodeHandle node = ui.createNode({8.0f, 8.0f}, {112.0f, 48.0f});
    layer.createStrip(0, {
        {-48.0f, -16.0f},
        { 48.0f, -16.0f},
        { 48.0f,  16.0f},
        {-48.0f,  16.0f},
    }, {}, node);

    ui.draw();

    MAGNUM_VERIFY_NO_GL_ERROR();

    if(!(_manager.load("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_manager.load("StbImageImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / StbImageImporter plugins not found.");

    #if defined(MAGNUM_TARGET_GLES) && !defined(MAGNUM_TARGET_WEBGL)
    /* Same problem is with all builtin shaders, so this doesn't seem to be a
       bug in the line layer shader code */
    if(GL::Context::current().detectedDriver() & GL::Context::DetectedDriver::SwiftShader)
        CORRADE_SKIP("UBOs with dynamically indexed arrays don't seem to work on SwiftShader, can't test.");
    #endif
    CORRADE_COMPARE_WITH(_framebuffer.read({{}, RenderSize}, {PixelFormat::RGBA8Unorm}),
        Utility::Path::join(UI_TEST_DIR, "LineLayerTestFiles/strip.png"),
        DebugTools::CompareImageToFile{_manager});
}

void LineLayerGLTest::renderLoop() {
    /* Like render(loop) but using createLoop() instead of an explicit index
       buffer to verify both behave the same */

    AbstractUserInterface ui{RenderSize};
    ui.setRendererInstance(Containers::pointer<RendererGL>());

    LineLayerGL::Shared layerShared{LineLayer::Shared::Configuration{1}};
    layerShared.setStyle(
        LineLayerCommonStyleUniform{},
        {LineLayerStyleUniform{}
            .setWidth(12.0f)
            /* Semi-transparent to verify there are no overlaps except where
               desired */
            .setColor(0xffffffff_rgbaf*0.75f)},
        {LineAlignment{}},
        {});
    LineLayer& layer = ui.setLayerInstance(Containers::pointer<LineLayerGL>(ui.createLayer(), layerShared));

    NodeHandle node = ui.createNode({8.0f, 8.0f}, {112.0f, 48.0f});
    layer.createLoop(0, {
        {-48.0f, -16.0f},
        { 48.0f, -16.0f},
        { 48.0f,  16.0f},
        {-48.0f,  16.0f},
    }, {}, node);

    ui.draw();

    MAGNUM_VERIFY_NO_GL_ERROR();

    if(!(_manager.load("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_manager.load("StbImageImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / StbImageImporter plugins not found.");

    #if defined(MAGNUM_TARGET_GLES) && !defined(MAGNUM_TARGET_WEBGL)
    /* Same problem is with all builtin shaders, so this doesn't seem to be a
       bug in the line layer shader code */
    if(GL::Context::current().detectedDriver() & GL::Context::DetectedDriver::SwiftShader)
        CORRADE_SKIP("UBOs with dynamically indexed arrays don't seem to work on SwiftShader, can't test.");
    #endif
    CORRADE_COMPARE_WITH(_framebuffer.read({{}, RenderSize}, {PixelFormat::RGBA8Unorm}),
        Utility::Path::join(UI_TEST_DIR, "LineLayerTestFiles/loop.png"),
        DebugTools::CompareImageToFile{_manager});
}

void LineLayerGLTest::renderSmoothness() {
    auto&& data = RenderSmoothnessData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* It should produce the same result (8 *pixel* smoothness) regardless of
       the actual UI size */

    /* Event handling size not used for anything, can stay arbitrary */
    AbstractUserInterface ui{Vector2{RenderSize}*data.uiScale, {1, 1}, RenderSize};
    ui.setRendererInstance(Containers::pointer<RendererGL>());

    LineLayerGL::Shared layerShared{LineLayer::Shared::Configuration{3, 2}};
    layerShared.setStyle(data.styleUniformCommon,
        /* To verify it's not always picking the first uniform */
        {LineLayerStyleUniform{},
         LineLayerStyleUniform{},
         data.styleUniform},
        /* To verify it's not using the style ID as uniform ID */
        {1, 2},
        {LineAlignment{}, LineAlignment{}},
        {});
    LineLayer& layer = ui.setLayerInstance(Containers::pointer<LineLayerGL>(ui.createLayer(), layerShared));

    NodeHandle node = ui.createNode(
        Vector2{8.0f, 8.0f}*data.uiScale,
        Vector2{112.0f, 48.0f}*data.uiScale);
    layer.create(1, {
        0, 1, 2, 3
    }, {
        Vector2{-48.0f, 0.0f}*data.uiScale,
        Vector2{ 48.0f, 0.0f}*data.uiScale,
        Vector2{0.0f, -16.0f}*data.uiScale,
        Vector2{0.0f,  16.0f}*data.uiScale,
    }, {}, node);

    ui.draw();

    MAGNUM_VERIFY_NO_GL_ERROR();

    if(!(_manager.load("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_manager.load("StbImageImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / StbImageImporter plugins not found.");

    #if defined(MAGNUM_TARGET_GLES) && !defined(MAGNUM_TARGET_WEBGL)
    /* Same problem is with all builtin shaders, so this doesn't seem to be a
       bug in the line layer shader code */
    if(GL::Context::current().detectedDriver() & GL::Context::DetectedDriver::SwiftShader)
        CORRADE_SKIP("UBOs with dynamically indexed arrays don't seem to work on SwiftShader, can't test.");
    #endif
    CORRADE_COMPARE_WITH(_framebuffer.read({{}, RenderSize}, {PixelFormat::RGBA8Unorm}),
        Utility::Path::join(UI_TEST_DIR, "LineLayerTestFiles/smooth.png"),
        DebugTools::CompareImageToFile{_manager});
}

void LineLayerGLTest::renderCustomColor() {
    auto&& data = RenderCustomColorData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Basically the same as the "per-point colors multiplied with per-style"
       case in render(), except that the color is additionally taken from the
       data and node opacity as well */

    AbstractUserInterface ui{RenderSize};
    ui.setRendererInstance(Containers::pointer<RendererGL>());

    LineLayerGL::Shared layerShared{LineLayer::Shared::Configuration{1}};
    layerShared.setStyle(
        LineLayerCommonStyleUniform{},
        {LineLayerStyleUniform{}
            .setWidth(20.0f)
            .setColor(0x336699cc_rgbaf/0x6633aa99_rgbaf)},
        {LineAlignment{}},
        {});
    LineLayer& layer = ui.setLayerInstance(Containers::pointer<LineLayerGL>(ui.createLayer(), layerShared));

    NodeHandle node = ui.createNode({8.0f, 8.0f}, {112.0f, 48.0f});
    DataHandle nodeData = layer.create(0, {
        0, 1, 2, 3, 4, 5, 5, 6, 6, 7
    }, {
        {-32.0f, -16.0f},
        {-32.0f,  16.0f},
        { 32.0f, -16.0f},
        { 32.0f,  16.0f},
        {-48.0f, 0.0f},
        {-16.0f, 0.0f},
        { 16.0f, 0.0f},
        { 48.0f, 0.0f},
    }, {
        0xffffffff_rgbaf/0x336699cc_rgbaf,
        0xffffffff_rgbaf/0x336699cc_rgbaf,
        0xffffffff_rgbaf/0x336699cc_rgbaf,
        0xffffffff_rgbaf/0x336699cc_rgbaf,
        0x2f83ccff_rgbaf*1.00f/0x336699cc_rgbaf,
        0x3bd267ff_rgbaf*0.75f/0x336699cc_rgbaf,
        0xc7cf2fff_rgbaf*0.50f/0x336699cc_rgbaf,
        0xcd3431ff_rgbaf*0.25f/0x336699cc_rgbaf,
    }, node);

    if(data.partialUpdate) {
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    layer.setColor(nodeData, 0x6633aa99_rgbaf/data.opacity);
    CORRADE_COMPARE_AS(ui.state(),
        UserInterfaceState::NeedsDataUpdate,
        TestSuite::Compare::GreaterOrEqual);

    if(data.opacity != 1.0f) {
        /* Update to verify that the opacity change alone triggers data upload
           as well */
        if(data.partialUpdate) {
            ui.update();
            CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
        }

        ui.setNodeOpacity(node, data.opacity);
        CORRADE_COMPARE_AS(ui.state(),
            UserInterfaceState::NeedsNodeOpacityUpdate,
            TestSuite::Compare::GreaterOrEqual);
    }

    ui.draw();

    MAGNUM_VERIFY_NO_GL_ERROR();

    if(!(_manager.load("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_manager.load("StbImageImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / StbImageImporter plugins not found.");

    #if defined(MAGNUM_TARGET_GLES) && !defined(MAGNUM_TARGET_WEBGL)
    /* Same problem is with all builtin shaders, so this doesn't seem to be a
       bug in the line layer shader code */
    if(GL::Context::current().detectedDriver() & GL::Context::DetectedDriver::SwiftShader)
        CORRADE_SKIP("UBOs with dynamically indexed arrays don't seem to work on SwiftShader, can't test.");
    #endif
    CORRADE_COMPARE_WITH(_framebuffer.read({{}, RenderSize}, {PixelFormat::RGBA8Unorm}),
        Utility::Path::join(UI_TEST_DIR, "LineLayerTestFiles/color.png"),
        DebugTools::CompareImageToFile{_manager});
}

void LineLayerGLTest::renderPaddingAlignment() {
    auto&& data = RenderPaddingAlignmentData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Basically the same as renderStrip(), except that the node offset, size
       and style or data padding / alignment changes. The result should always
       be the same as if the padding was applied directly to the node offset
       and size itself, and alignment offset to the line points. */

    AbstractUserInterface ui{RenderSize};
    ui.setRendererInstance(Containers::pointer<RendererGL>());

    LineLayerGL::Shared layerShared{LineLayer::Shared::Configuration{1}};
    layerShared.setStyle(
        LineLayerCommonStyleUniform{},
        {LineLayerStyleUniform{}
            .setWidth(12.0f)
            .setColor(0xffffffff_rgbaf*0.75f)},
        {data.alignmentFromStyle},
        {data.paddingFromStyle});
    LineLayer& layer = ui.setLayerInstance(Containers::pointer<LineLayerGL>(ui.createLayer(), layerShared));

    NodeHandle node = ui.createNode(data.nodeOffset, data.nodeSize);
    DataHandle nodeData = layer.createStrip(0, {
        Vector2{-48.0f, -16.0f} + data.pointOffset,
        Vector2{ 48.0f, -16.0f} + data.pointOffset,
        Vector2{ 48.0f,  16.0f} + data.pointOffset,
        Vector2{-48.0f,  16.0f} + data.pointOffset,
    }, {}, node);

    if(data.partialUpdate) {
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    if(!data.paddingFromData.isZero()) {
        layer.setPadding(nodeData, data.paddingFromData);
        CORRADE_COMPARE_AS(ui.state(),
            UserInterfaceState::NeedsDataUpdate,
            TestSuite::Compare::GreaterOrEqual);
    }
    if(data.alignmentFromData) {
        layer.setAlignment(nodeData, data.alignmentFromData);
        CORRADE_COMPARE_AS(ui.state(),
            UserInterfaceState::NeedsDataUpdate,
            TestSuite::Compare::GreaterOrEqual);
    }

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
       bug in the line layer shader code */
    if(GL::Context::current().detectedDriver() & GL::Context::DetectedDriver::SwiftShader)
        CORRADE_SKIP("UBOs with dynamically indexed arrays don't seem to work on SwiftShader, can't test.");
    #endif
    CORRADE_COMPARE_WITH(_framebuffer.read({{}, RenderSize}, {PixelFormat::RGBA8Unorm}),
        Utility::Path::join(UI_TEST_DIR, "LineLayerTestFiles/strip.png"),
        DebugTools::CompareImageToFile{_manager});
}

void LineLayerGLTest::renderChangeStyle() {
    auto&& data = RenderChangeStyleData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Basically the same as renderStrip(), except that the style ID is changed
       to the style only later. */

    AbstractUserInterface ui{RenderSize};
    ui.setRendererInstance(Containers::pointer<RendererGL>());

    LineLayerGL::Shared layerShared{LineLayer::Shared::Configuration{2}};
    layerShared.setStyle(
        LineLayerCommonStyleUniform{},
        {LineLayerStyleUniform{},
         LineLayerStyleUniform{}
            .setWidth(12.0f)
            .setColor(0xffffffff_rgbaf*0.75f)},
        {LineAlignment::BottomRight, LineAlignment::MiddleCenter},
        {});
    LineLayer& layer = ui.setLayerInstance(Containers::pointer<LineLayerGL>(ui.createLayer(), layerShared));

    NodeHandle node = ui.createNode({8.0f, 8.0f}, {112.0f, 48.0f});
    DataHandle nodeData = layer.createStrip(0, {
        {-48.0f, -16.0f},
        { 48.0f, -16.0f},
        { 48.0f,  16.0f},
        {-48.0f,  16.0f},
    }, {}, node);

    if(data.partialUpdate) {
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    layer.setStyle(nodeData, 1);
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
       bug in the line layer shader code */
    if(GL::Context::current().detectedDriver() & GL::Context::DetectedDriver::SwiftShader)
        CORRADE_SKIP("UBOs with dynamically indexed arrays don't seem to work on SwiftShader, can't test.");
    #endif
    CORRADE_COMPARE_WITH(_framebuffer.read({{}, RenderSize}, {PixelFormat::RGBA8Unorm}),
        Utility::Path::join(UI_TEST_DIR, "LineLayerTestFiles/strip.png"),
        DebugTools::CompareImageToFile{_manager});
}

void LineLayerGLTest::renderChangeLine() {
    auto&& data = RenderChangeLineData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Basically the same as renderStrip() / renderLoop(), except that the line
       is changed only subsequently, via one of the three setLine*() APIs. */

    AbstractUserInterface ui{RenderSize};
    ui.setRendererInstance(Containers::pointer<RendererGL>());

    LineLayerGL::Shared layerShared{LineLayer::Shared::Configuration{1}};
    layerShared.setStyle(
        LineLayerCommonStyleUniform{},
        {LineLayerStyleUniform{}
            .setWidth(12.0f)
            .setColor(0xffffffff_rgbaf*0.75f)},
        {LineAlignment::MiddleCenter},
        {});
    LineLayer& layer = ui.setLayerInstance(Containers::pointer<LineLayerGL>(ui.createLayer(), layerShared));

    NodeHandle node = ui.createNode({8.0f, 8.0f}, {112.0f, 48.0f});
    DataHandle nodeData = layer.create(0, {0, 1}, {
        {-16.0f, 0.0f}, {16.0f, 0.0f}
    }, {0xff3366_rgbf, 0x3366ff_rgbf}, node);

    if(data.partialUpdate) {
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    if(data.indexed) {
        layer.setLine(nodeData, {
            0, 1, 1, 2, 2, 3
        }, {
            {-48.0f, -16.0f},
            { 48.0f, -16.0f},
            { 48.0f,  16.0f},
            {-48.0f,  16.0f},
        }, {});
    } else if(data.strip) {
        layer.setLineStrip(nodeData, {
            {-48.0f, -16.0f},
            { 48.0f, -16.0f},
            { 48.0f,  16.0f},
            {-48.0f,  16.0f},
        }, {});
    } else if(data.loop) {
        layer.setLineLoop(nodeData, {
            {-48.0f, -16.0f},
            { 48.0f, -16.0f},
            { 48.0f,  16.0f},
            {-48.0f,  16.0f},
        }, {});
    }
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
       bug in the line layer shader code */
    if(GL::Context::current().detectedDriver() & GL::Context::DetectedDriver::SwiftShader)
        CORRADE_SKIP("UBOs with dynamically indexed arrays don't seem to work on SwiftShader, can't test.");
    #endif
    CORRADE_COMPARE_WITH(_framebuffer.read({{}, RenderSize}, {PixelFormat::RGBA8Unorm}),
        Utility::Path::join({UI_TEST_DIR, "LineLayerTestFiles", data.filename}),
        DebugTools::CompareImageToFile{_manager});
}

constexpr Vector2i DrawSize{64, 64};

void LineLayerGLTest::drawSetup() {
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

void LineLayerGLTest::drawTeardown() {
    _framebuffer = GL::Framebuffer{NoCreate};
    _color = GL::Texture2D{NoCreate};

    GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::disable(GL::Renderer::Feature::ScissorTest);
    GL::Renderer::disable(GL::Renderer::Feature::Blending);
}

void LineLayerGLTest::drawOrder() {
    auto&& data = DrawOrderData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    AbstractUserInterface ui{DrawSize};
    ui.setRendererInstance(Containers::pointer<RendererGL>());

    LineLayerGL::Shared layerShared{LineLayer::Shared::Configuration{4}};
    /* Testing the styleToUniform initializer list overload, others cases use
       implicit mapping initializer list overloads */
    layerShared.setStyle(LineLayerCommonStyleUniform{}, {
        LineLayerStyleUniform{}         /* 0, red narrow */
            .setColor(0xff0000_rgbf)
            .setWidth(6.0f),
        LineLayerStyleUniform{}         /* 1, green */
            .setColor(0x00ff00_rgbf)
            .setWidth(12.0f),
        LineLayerStyleUniform{}         /* 2, blue */
            .setColor(0x0000ff_rgbf)
            .setWidth(6.0f),
        LineLayerStyleUniform{}         /* 3, red wide */
            .setColor(0xff0000_rgbf)
            .setWidth(12.0f),
    }, {0, 1, 3, 2}, {
        LineAlignment{}, LineAlignment{}, LineAlignment{}, LineAlignment{}
    }, {});

    LineLayer& layer = ui.setLayerInstance(Containers::pointer<LineLayerGL>(ui.createLayer(), layerShared));
    NodeHandle topLevelOnTopGreen = ui.createNode({8.0f, 8.0f}, {32.0f, 32.0f});

    NodeHandle topLevelBelowRed = ui.createNode({24.0f, 24.0f}, {32.0f, 32.0f});
    ui.setNodeOrder(topLevelBelowRed, topLevelOnTopGreen);

    NodeHandle topLevelHiddenBlue = ui.createNode({24.0f, 8.0f}, {32.0f, 32.0f}, NodeFlag::Hidden);

    NodeHandle childBelowBlue = ui.createNode(topLevelOnTopGreen, {12.0f, 4.0f}, {16.0f, 16.0f});
    NodeHandle childAboveRed = ui.createNode(childBelowBlue, {-8.0f, 8.0f}, {16.0f, 16.0f});

    Vector2 points32[]{
        {-10.0f, -10.0f},
        { 10.0f, -10.0f},
        { 10.0f,  10.0f},
        {-10.0f,  10.0f},
    };
    Vector2 points16[]{
        {-5.0f, -5.0f},
        { 5.0f, -5.0f},
        { 5.0f,  5.0f},
        {-5.0f,  5.0f},
    };
    UnsignedInt stripOpenTop[]{0, 3, 3, 2, 2, 1};
    UnsignedInt stripOpenLeft[]{0, 1, 1, 2, 2, 3};
    UnsignedInt stripOpenRight[]{1, 0, 0, 3, 3, 2};
    UnsignedInt stripOpenBottom[]{3, 0, 0, 1, 1, 2};

    if(data.dataInNodeOrder) {
        layer.create(2, stripOpenBottom, points32, {}, topLevelBelowRed);
        layer.create(1, stripOpenRight, points32, {}, topLevelOnTopGreen);
        layer.create(2, stripOpenRight, points32, {}, topLevelHiddenBlue);
        layer.create(3, stripOpenTop, points16, {}, childBelowBlue);
        layer.create(0, stripOpenLeft, points16, {}, childAboveRed);
    } else {
        layer.create(1, stripOpenRight, points32, {}, topLevelOnTopGreen);
        layer.create(2, stripOpenRight, points32, {}, topLevelHiddenBlue);
        layer.create(2, stripOpenBottom, points32, {}, topLevelBelowRed);
        layer.create(0, stripOpenLeft, points16, {}, childAboveRed);
        layer.create(3, stripOpenTop, points16, {}, childBelowBlue);
    }

    ui.draw();

    MAGNUM_VERIFY_NO_GL_ERROR();

    if(!(_manager.load("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_manager.load("StbImageImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / StbImageImporter plugins not found.");

    #if defined(MAGNUM_TARGET_GLES) && !defined(MAGNUM_TARGET_WEBGL)
    /* Same problem is with all builtin shaders, so this doesn't seem to be a
       bug in the line layer shader code */
    if(GL::Context::current().detectedDriver() & GL::Context::DetectedDriver::SwiftShader)
        CORRADE_SKIP("UBOs with dynamically indexed arrays don't seem to work on SwiftShader, can't test.");
    #endif
    CORRADE_COMPARE_WITH(_framebuffer.read({{}, DrawSize}, {PixelFormat::RGBA8Unorm}),
        Utility::Path::join(UI_TEST_DIR, "LineLayerTestFiles/draw-order.png"),
        DebugTools::CompareImageToFile{_manager});
}

void LineLayerGLTest::eventStyleTransition() {
    /* Switches between the "default" and "default joins and caps" cases from
       render() after a press event, and subsequently to a disabled style,
       which is "default" again. Everything else is tested in AbstractVisualLayerTest already. */

    AbstractUserInterface ui{RenderSize};
    ui.setRendererInstance(Containers::pointer<RendererGL>());

    LineLayerGL::Shared layerShared{LineLayer::Shared::Configuration{2}};
    layerShared
        .setStyle(LineLayerCommonStyleUniform{}, {
            LineLayerStyleUniform{},        /* default */
            LineLayerStyleUniform{}         /* default joins and caps */
                /* Cannot have smoothness on the common style, as it'd affect
                   the default as well */
                .setSmoothness(1.0f)
                .setWidth(12.0f)
                .setColor(0xffffffff_rgbaf*0.75f)
        }, {LineAlignment{}, LineAlignment{}}, {})
        .setStyleTransition(
            [](UnsignedInt style) -> UnsignedInt {
                /* Gets triggered right before disabled transition */
                if(style == 1) return 1;
                CORRADE_INTERNAL_ASSERT_UNREACHABLE();
            },
            [](UnsignedInt) -> UnsignedInt {
                CORRADE_INTERNAL_ASSERT_UNREACHABLE();
            },
            [](UnsignedInt style) -> UnsignedInt {
                if(style == 0) return 1;
                CORRADE_INTERNAL_ASSERT_UNREACHABLE();
            },
            [](UnsignedInt style) -> UnsignedInt {
                if(style == 1) return 0;
                CORRADE_INTERNAL_ASSERT_UNREACHABLE();
            });

    LineLayer& layer = ui.setLayerInstance(Containers::pointer<LineLayerGL>(ui.createLayer(), layerShared));

    NodeHandle node = ui.createNode({8.0f, 8.0f}, {112.0f, 48.0f});
    layer.create(0, {
        0, 1, 1, 2, 2, 3, 4, 5, 6, 7, 8, 8
    }, {
        {-48.0f, -16.0f},
        {-16.0f,  16.0f},
        { 16.0f, -16.0f},
        { 16.0f,  16.0f},
        /* These two lines overlap */
        { 36.0f,   0.0f},
        { 52.0f,   0.0f},
        { 44.0f,  16.0f},
        { 44.0f, -16.0f},
        /* Standalone point */
        {-16.0f, -16.0f},
    }, {}, node);

    ui.draw();

    MAGNUM_VERIFY_NO_GL_ERROR();
    Image2D before = _framebuffer.read({{}, RenderSize}, {PixelFormat::RGBA8Unorm});

    PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
    CORRADE_VERIFY(ui.pointerPressEvent({64.0f, 24.0f}, event));
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataUpdate);

    /* We have blending enabled, which means a subsequent draw would try to
       blend with the previous, causing unwanted difference */
    _framebuffer.clear(GL::FramebufferClear::Color);
    ui.draw();

    MAGNUM_VERIFY_NO_GL_ERROR();
    Image2D after = _framebuffer.read({{}, RenderSize}, {PixelFormat::RGBA8Unorm});

    /* Verify that node disabling alone causes a proper render data update as
       well */
    ui.addNodeFlags(node, NodeFlag::Disabled);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeEnabledUpdate);

    /* We have blending enabled, which means a subsequent draw would try to
       blend with the previous, causing unwanted difference */
    _framebuffer.clear(GL::FramebufferClear::Color);
    ui.draw();

    MAGNUM_VERIFY_NO_GL_ERROR();
    Image2D disabled = _framebuffer.read({{}, RenderSize}, {PixelFormat::RGBA8Unorm});

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
        Utility::Path::join(UI_TEST_DIR, "LineLayerTestFiles/default.png"),
        DebugTools::CompareImageToFile{_manager});
    CORRADE_COMPARE_WITH(after,
        Utility::Path::join(UI_TEST_DIR, "LineLayerTestFiles/square-miter.png"),
        DebugTools::CompareImageToFile{_manager});
    CORRADE_COMPARE_WITH(disabled,
        Utility::Path::join(UI_TEST_DIR, "LineLayerTestFiles/default.png"),
        DebugTools::CompareImageToFile{_manager});
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::LineLayerGLTest)
