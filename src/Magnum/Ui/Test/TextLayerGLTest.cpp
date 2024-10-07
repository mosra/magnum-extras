/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023, 2024
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

#include <sstream> /** @todo remove once Debug is stream-free */
#include <Corrade/Containers/BitArrayView.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Containers/StringIterable.h>
#include <Corrade/PluginManager/Manager.h>
#include <Corrade/PluginManager/PluginMetadata.h>
#include <Corrade/TestSuite/Compare/Numeric.h>
#include <Corrade/Utility/ConfigurationGroup.h>
#include <Corrade/Utility/DebugStl.h> /** @todo remove once Debug is stream-free */
#include <Corrade/Utility/Path.h>
#include <Magnum/Image.h>
#include <Magnum/ImageView.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/DebugTools/CompareImage.h>
#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/OpenGLTester.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/Text/AbstractFont.h>
#include <Magnum/Text/AbstractShaper.h>
#include <Magnum/Text/Alignment.h>
#include <Magnum/Text/GlyphCacheGL.h>
#include <Magnum/Trade/AbstractImporter.h>

#include "Magnum/Ui/AbstractUserInterface.h"
#include "Magnum/Ui/Event.h"
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/NodeFlags.h"
#include "Magnum/Ui/RendererGL.h"
#include "Magnum/Ui/TextLayerGL.h"
#include "Magnum/Ui/TextProperties.h"

#include "configure.h"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct TextLayerGLTest: GL::OpenGLTester {
    explicit TextLayerGLTest();

    void sharedConstruct();
    /* NoCreate tested in TextLayerGL_Test to verify it works without a GL
       context */
    void sharedConstructCopy();
    void sharedConstructMove();

    void sharedSetGlyphCache();
    void sharedSetGlyphCacheTakeOwnership();

    void construct();
    void constructDerived();
    void constructCopy();
    void constructMove();

    void drawNoSizeSet();
    void drawNoStyleSet();

    void renderSetup();
    void renderTeardown();
    void render();
    void renderEdgeSmoothness();
    void renderAlignmentPadding();
    void renderCustomColor();
    void renderChangeText();
    void renderChangeStyle();

    void renderDynamicStyles();

    void drawSetup();
    void drawTeardown();
    void drawOrder();
    void drawClipping();

    void eventStyleTransition();

    private:
        PluginManager::Manager<Text::AbstractFont> _fontManager;
        PluginManager::Manager<Trade::AbstractImporter> _importerManager;
        GL::Texture2D _color{NoCreate};
        GL::Framebuffer _framebuffer{NoCreate};

        /* stb_truetype's rasterization is extremely slow, so the cache filling
           is done just once for all tests that need it; thus also the font has
           to be shared among all */
        Containers::Pointer<Text::AbstractFont> _font;
        Text::GlyphCacheGL _fontGlyphCache{PixelFormat::R8Unorm, {64, 64}};
};

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
    bool singleGlyph;
    UnsignedInt cursor, selection;
    TextLayerStyleUniform styleUniform;
    Containers::Optional<TextLayerCommonEditingStyleUniform> styleUniformEditingCommon;
    Containers::Optional<TextLayerEditingStyleUniform> styleUniformCursor;
    Vector4 cursorPadding;
    Containers::Optional<TextLayerEditingStyleUniform> styleUniformSelection;
    Vector4 selectionPadding;
    Containers::Optional<TextLayerStyleUniform> styleUniformSelectionText;
} RenderData[]{
    {"default", "default.png", false, 0, 0,
        TextLayerStyleUniform{},
        {}, {}, {}, {}, {}, {}},
    /* Should be centered according to its bounding box, not according to the
       font metrics -- thus a lot higher than the g in Maggi in the above */
    {"default single glyph", "default-glyph.png", true, 0, 0,
        TextLayerStyleUniform{},
        {}, {}, {}, {}, {}, {}},
    {"colored", "colored.png", false, 0, 0,
        TextLayerStyleUniform{}
            .setColor(0x3bd267_rgbf),
        {}, {}, {}, {}, {}, {}},
    /* Again, should be centered according to its bounding box */
    {"colored single glyph", "colored-glyph.png", true, 0, 0,
        TextLayerStyleUniform{}
            .setColor(0x3bd267_rgbf),
        {}, {}, {}, {}, {}, {}},
    /** @todo test at least toggling kerning once StbTrueTypeFont supports
        that */
    /* The cursor has zero width so it's basically invisible */
    {"default, default cursor style", "default.png", false, 2, 2,
        TextLayerStyleUniform{},
        TextLayerCommonEditingStyleUniform{},
        TextLayerEditingStyleUniform{}, {}, {}, {}, {}},
    {"default, selection, no selection style", "default.png", false, 2, 5,
        TextLayerStyleUniform{},
        TextLayerCommonEditingStyleUniform{},
        {}, {}, {}, {}, {}},
    {"default, default selection style, empty", "default.png", false, 2, 2,
        TextLayerStyleUniform{},
        TextLayerCommonEditingStyleUniform{},
        {}, {}, TextLayerEditingStyleUniform{}, {}, {}},
    {"default, default selection style", "default-selection.png", false, 2, 5,
        TextLayerStyleUniform{},
        TextLayerCommonEditingStyleUniform{},
        {}, {}, TextLayerEditingStyleUniform{}, {}, {}},
    /* Cursor isn't visible with selection present either */
    {"default, default cursor + selection style", "default-selection.png", false, 2, 5,
        TextLayerStyleUniform{},
        TextLayerCommonEditingStyleUniform{},
        TextLayerEditingStyleUniform{}, {},
        TextLayerEditingStyleUniform{}, {},
        {}},
    /* And thus reversing the direction also doesn't change the appearance in
       any way */
    {"default, default cursor + selection style, reverse direction", "default-selection.png", false, 5, 2,
        TextLayerStyleUniform{},
        TextLayerCommonEditingStyleUniform{},
        TextLayerEditingStyleUniform{}, {},
        TextLayerEditingStyleUniform{}, {},
        {}},
    /* Overriding the selection text uniform with one that has a default value
       also doesn't change anything */
    {"default, default cursor + selection style, default selection text uniform override", "default-selection.png", false, 2, 5,
        TextLayerStyleUniform{},
        TextLayerCommonEditingStyleUniform{},
        TextLayerEditingStyleUniform{}, {},
        TextLayerEditingStyleUniform{}, {},
        TextLayerStyleUniform{}},
    /* Making the cursor and selection transparent doesn't affect the rendering
       in any way */
    {"colored, transparent cursor + selection style", "colored.png", false, 2, 5,
        TextLayerStyleUniform{}
            .setColor(0x3bd267_rgbf),
        TextLayerCommonEditingStyleUniform{},
        TextLayerEditingStyleUniform{}
            /* Non-zero cursor width to make it (transparently) visible */
            .setBackgroundColor(0x00000000_rgbaf), {5.0f, 0.0f, 5.0f, 0.0f},
        TextLayerEditingStyleUniform{}
            .setBackgroundColor(0x00000000_rgbaf), {},
            {}},
    {"colored, cursor style", "colored-cursor.png", false, 2, 2,
        TextLayerStyleUniform{}
            .setColor(0x3bd267_rgbf),
        TextLayerCommonEditingStyleUniform{},
        TextLayerEditingStyleUniform{}
            /* Extra wide to make it visible that it's in the background,
               symmetric to verify there's nothing off in the positioning.
               Off-center padding to verify it's applied to correct sides,
               tested below. */
            .setBackgroundColor(0xcd3431_rgbf), {5.0f, 0.0f, 5.0f, 0.0f},
        {}, {},
        {}},
    {"colored, cursor style, non-empty selection with no style", "colored-cursor.png", false, 2, 5,
        TextLayerStyleUniform{}
            .setColor(0x3bd267_rgbf),
        TextLayerCommonEditingStyleUniform{},
        TextLayerEditingStyleUniform{}
            .setBackgroundColor(0xcd3431_rgbf), {5.0f, 0.0f, 5.0f, 0.0f},
        {}, {},
        {}},
    {"colored, selection style", "colored-selection.png", false, 2, 5,
        TextLayerStyleUniform{}
            .setColor(0x3bd267_rgbf),
        TextLayerCommonEditingStyleUniform{},
        {}, {},
        TextLayerEditingStyleUniform{}
            /* Without padding as it's visible that it's in the background
               anyway */
            .setBackgroundColor(0xc7cf2f_rgbf), {},
        {}},
    /* Should look exactly the same as above */
    {"colored, selection style, different direction", "colored-selection.png", false, 5, 2,
        TextLayerStyleUniform{}
            .setColor(0x3bd267_rgbf),
        TextLayerCommonEditingStyleUniform{},
        {}, {},
        TextLayerEditingStyleUniform{}
            .setBackgroundColor(0xc7cf2f_rgbf), {},
        {}},
    {"colored, cursor + selection style, selection empty", "colored-cursor.png", false, 2, 2,
        TextLayerStyleUniform{}
            .setColor(0x3bd267_rgbf),
        TextLayerCommonEditingStyleUniform{},
        TextLayerEditingStyleUniform{}
            .setBackgroundColor(0xcd3431_rgbf), {5.0f, 0.0f, 5.0f, 0.0f},
        TextLayerEditingStyleUniform{}
            .setBackgroundColor(0xc7cf2f_rgbf), {},
        {}},
    {"colored, cursor + selection style", "colored-cursor-selection.png", false, 2, 5,
        TextLayerStyleUniform{}
            .setColor(0x3bd267_rgbf),
        TextLayerCommonEditingStyleUniform{},
        TextLayerEditingStyleUniform{}
            /* The red should be on the top of the yellow compared to selection
               alone */
            .setBackgroundColor(0xcd3431_rgbf), {5.0f, 0.0f, 5.0f, 0.0f},
        TextLayerEditingStyleUniform{}
            .setBackgroundColor(0xc7cf2f_rgbf), {},
        {}},
    {"colored, cursor + selection style, colored text", "colored-cursor-selection-text.png", false, 2, 5,
        TextLayerStyleUniform{}
            .setColor(0x3bd267_rgbf),
        TextLayerCommonEditingStyleUniform{},
        TextLayerEditingStyleUniform{}
            /* The red should be on the top of the yellow compared to selection
               alone */
            .setBackgroundColor(0xcd3431_rgbf), {5.0f, 0.0f, 5.0f, 0.0f},
        TextLayerEditingStyleUniform{}
            .setBackgroundColor(0xc7cf2f_rgbf), {},
        TextLayerStyleUniform{}
            .setColor(0x1f1f1f_rgbf)},
    {"colored, cursor + selection style, colored text, smooth rounded corners", "colored-cursor-selection-text-rounded.png", false, 2, 5,
        TextLayerStyleUniform{}
            .setColor(0x3bd267_rgbf),
        TextLayerCommonEditingStyleUniform{}
            .setSmoothness(1.0f),
        TextLayerEditingStyleUniform{}
            .setBackgroundColor(0xcd3431_rgbf)
            /* Off-center to verify it's applied to right edges */
            .setCornerRadius(5.5f), {10.0f, -5.0f, -1.0f, 0.0f},
        TextLayerEditingStyleUniform{}
            .setBackgroundColor(0xc7cf2f_rgbf)
            /* Off-center to verify it's applied to right edges */
            .setCornerRadius(10.0f), {5.0f, 0.0f, 7.5f, 5.0f},
        TextLayerStyleUniform{}
            .setColor(0x1f1f1f_rgbf)},
    /** @todo test padding applied swizzled for RTL text once we can use
        HarfBuzzFont (rendering `iggaM`, forcing RTL and supplying swizzled
        padding, should result in the same as above) */
};

const struct {
    const char* name;
    Float scale;
    bool setSizeLater;
} RenderEdgeSmoothnessData[]{
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

constexpr Vector2i RenderSize{128, 64};

/* Bounding box reported by Text::renderLineGlyphPositionsInto(). May change
   when StbTrueTypeFont gets kerning implemented, a different font or a
   different text is used. */
constexpr Range2D RenderAlignmentBoundingBox{{0.0f, -9.26651f}, {84.6205f, 33.4002f}};

const struct {
    const char* name;
    Text::Alignment alignment;
    bool partialUpdate, editable;
    Vector2 nodeOffset, nodeSize;
    Vector4 paddingFromStyle, paddingFromData;
} RenderAlignmentPaddingData[]{
    /* Same as the "default" in RenderData */
    {"middle center, no padding",
        Text::Alignment::MiddleCenter, false, true,
        {8.0f, 8.0f}, {112.0f, 48.0f}, {}, {}},
    /* Deliberately having one excessively shifted to left/top and the other to
       bottom/right. It shouldn't cause any strange artifacts. */
    {"middle center, padding from style",
        Text::Alignment::MiddleCenter, false, true,
        {-64.0f, -128.0f}, {192.0f, 192.0f},
        {72.0f, 136.0f, 8.0f, 8.0f}, {}},
    {"middle center, padding from data",
        Text::Alignment::MiddleCenter, false, true,
        {0.0f, 0.0f}, {192.0f, 192.0f},
        {}, {8.0f, 8.0f, 72.0f, 136.0f}},
    {"middle center, padding from both",
        Text::Alignment::MiddleCenter, false, true,
        {0.0f, 0.0f}, {128.0f, 64.0f},
        {4.0f, 8.0f, 0.0f, 4.0f}, {4.0f, 0.0f, 8.0f, 4.0f}},
    {"middle center, padding from both, partial update",
        Text::Alignment::MiddleCenter, true, true,
        {0.0f, 0.0f}, {128.0f, 64.0f},
        {4.0f, 8.0f, 0.0f, 4.0f}, {4.0f, 0.0f, 8.0f, 4.0f}},
    {"middle center, padding from both, not editable",
        Text::Alignment::MiddleCenter, false, false,
        {0.0f, 0.0f}, {128.0f, 64.0f},
        {4.0f, 8.0f, 0.0f, 4.0f}, {4.0f, 0.0f, 8.0f, 4.0f}},
    {"middle center, padding from both, not editable, oartial update",
        Text::Alignment::MiddleCenter, true, false,
        {0.0f, 0.0f}, {128.0f, 64.0f},
        {4.0f, 8.0f, 0.0f, 4.0f}, {4.0f, 0.0f, 8.0f, 4.0f}},
    /* The size isn't used for anything in this case so can be excessive */
    {"top left, no padding",
        Text::Alignment::TopLeft, false, true,
        (Vector2{RenderSize} - RenderAlignmentBoundingBox.size())/2.0f, {256.0f, 128.0f},
        {}, {}},
    {"top left, padding from data",
        Text::Alignment::TopLeft, false, true,
        {0.0f, 0.0f}, {256.0f, 128.0f},
        {}, {(RenderSize.x() - RenderAlignmentBoundingBox.size().x())/2.0f,
             (RenderSize.y() - RenderAlignmentBoundingBox.size().y())/2.0f,
             0.0f, 0.0f}},
    /* The min offset isn't used for anything in this case so can be
       excessive */
    {"bottom right, no padding",
        Text::Alignment::BottomRight, false, true,
        {-128.0f, -256.0f}, Vector2{128.0f, 256.0f} + (Vector2{RenderSize} + RenderAlignmentBoundingBox.size())/2.0f,
        {}, {}},
    {"bottom right, padding from style",
        Text::Alignment::BottomRight, false, true,
        {-128.0f, -256.0f}, Vector2{256.0f, 512.0f} + (Vector2{RenderSize} + RenderAlignmentBoundingBox.size())/2.0f,
        {0.0f, 0.0f, 128.0f, 256.0f}, {}},
    {"line right, no padding",
        Text::Alignment::LineRight, false, true,
        {0.0f, RenderSize.y()/2.0f + RenderAlignmentBoundingBox.max().y() - RenderAlignmentBoundingBox.sizeY()},
        {(RenderSize.x() + RenderAlignmentBoundingBox.sizeX())/2.0f, RenderAlignmentBoundingBox.sizeY()},
        {}, {}},
    {"line right, padding from both",
        Text::Alignment::LineRight, false, true,
        {0.0f, -RenderAlignmentBoundingBox.sizeY()},
        {(RenderSize.x() + RenderAlignmentBoundingBox.sizeX())/2.0f, RenderAlignmentBoundingBox.sizeY() + RenderSize.y()/2.0f + RenderAlignmentBoundingBox.max().y()},
        {0.0f, RenderSize.y()/2.0f, 0.0f, 0.0f},
        {0.0f, RenderAlignmentBoundingBox.max().y(), 0.0f, 0.0f}},
};

const struct {
    const char* name;
    bool editable;
    bool partialUpdate;
} RenderCustomColorData[]{
    {"", false, false},
    {"partial update", false, true},
    {"editable", true, false},
    {"editable, partial update", true, true},
};

const struct {
    const char* name;
    bool partialUpdate, editableBefore, editableAfter;
} RenderChangeStyleData[]{
    {"", false, false, false},
    {"partial update", true, false, false},
    {"editable", false, true, true},
    {"editable, partial update", true, true, true},
    {"editable, editing style only before", false, true, false},
    {"editable, editing style only before, partial update", true, true, false},
    {"editable, editing style only after", false, false, true},
    {"editable, editing style only after, partial update", true, false, true}
};

const struct {
    const char* name;
    bool partialUpdate, editableBefore, editableAfter;
} RenderChangeTextData[]{
    {"", false, false, false},
    {"partial update", true, false, false},
    {"editable", false, true, true},
    {"editable, partial update", true, true, true},
    {"editable only before", false, true, false},
    {"editable only before, partial update", true, true, false},
    {"editable only after", false, false, true},
    {"editable only after, partial update", true, false, true},
};

const struct {
    const char* name;
    const char* filename;
    UnsignedInt styleIndex;
    TextLayerStyleUniform styleUniform;
    Float leftPadding;
    Containers::Optional<TextLayerStyleUniform> dynamicStyleUniform;
    Float dynamicLeftPadding;
    Containers::Optional<TextLayerEditingStyleUniform> styleUniformCursor;
    Vector4 cursorPadding;
    Containers::Optional<TextLayerEditingStyleUniform> styleUniformSelection;
    Containers::Optional<TextLayerStyleUniform> styleUniformSelectionText;
    Containers::Optional<TextLayerEditingStyleUniform> dynamicStyleUniformCursor;
    Vector4 dynamicCursorPadding;
    Containers::Optional<TextLayerEditingStyleUniform> dynamicStyleUniformSelection;
    Containers::Optional<TextLayerStyleUniform> dynamicStyleUniformSelectionText;
    bool createLayerAfterSetStyle;
    bool secondaryStyleUpload;
    bool secondaryDynamicStyleUpload;
    bool noBaseStyles;
    bool explicitFont, explicitAlignment;
} RenderDynamicStylesData[]{
    {"default, static", "default.png", 1,
        TextLayerStyleUniform{}, 0.0f,
        {}, 0.0f,
        {}, {}, {}, {}, {}, {}, {}, {},
        false, false, false, false, false, false},
    {"default, static, editing", "default-selection.png", 1,
        TextLayerStyleUniform{}, 0.0f,
        {}, 0.0f,
        TextLayerEditingStyleUniform{}, {},
        TextLayerEditingStyleUniform{}, {}, {}, {}, {}, {},
        false, false, false, false, false, false},
    {"default, static, create layer after setStyle()", "default.png", 1,
        TextLayerStyleUniform{}, 0.0f,
        {}, 0.0f,
        {}, {}, {}, {}, {}, {}, {}, {},
        true, false, false, false, false, false},
    {"default, static, create layer after setStyle(), editing", "default-selection.png", 1,
        TextLayerStyleUniform{}, 0.0f,
        {}, 0.0f,
        TextLayerEditingStyleUniform{}, {},
        TextLayerEditingStyleUniform{}, {}, {}, {}, {}, {},
        true, false, false, false, false, false},

    {"default, dynamic with no upload", "default.png", 5,
        TextLayerStyleUniform{}, 0.0f,
        {}, 0.0f,
        {}, {}, {}, {}, {}, {}, {}, {},
        /* Default dynamic alignment is MiddleCenter as well, so it doesn't
           need to be passed explicitly */
        false, false, false, false, true, false},
    /* If no dynamic style is uploaded, the editing style isn't present by
       default */
    {"default, dynamic with no upload, editing", "default.png", 5,
        TextLayerStyleUniform{}, 0.0f,
        {}, 0.0f,
        /* At least one editing Optional has to be set to trigger a call to
           setEditingStyle() */
        TextLayerEditingStyleUniform{}, {}, {}, {}, {}, {}, {}, {},
        false, false, false, false, true, false},
    {"default, dynamic", "default.png", 5,
        TextLayerStyleUniform{}, 0.0f,
        TextLayerStyleUniform{}, 0.0f,
        {}, {}, {}, {}, {}, {}, {}, {},
        false, false, false, false, false, false},
    {"default, dynamic, editing", "default-selection.png", 5,
        TextLayerStyleUniform{}, 0.0f,
        TextLayerStyleUniform{}, 0.0f,
        {}, {}, {}, {},
        TextLayerEditingStyleUniform{}, {},
        TextLayerEditingStyleUniform{}, {},
        false, false, false, false, false, false},
    {"default, only dynamic styles", "default.png", 1,
        TextLayerStyleUniform{}, 0.0f,
        TextLayerStyleUniform{}, 0.0f,
        {}, {}, {}, {}, {}, {}, {}, {},
        /* Passing an explicit font because there's otherwise none by
           default */
        false, false, false, true, true, false},
    {"default, only dynamic styles, editing", "default-selection.png", 1,
        TextLayerStyleUniform{}, 0.0f,
        TextLayerStyleUniform{}, 0.0f,
        {}, {}, {}, {},
        TextLayerEditingStyleUniform{}, {},
        TextLayerEditingStyleUniform{}, {},
        /* Passing an explicit font because there's otherwise none by
           default */
        false, false, false, true, true, false},

    {"styled, static", "colored.png", 1,
        TextLayerStyleUniform{}
            .setColor(0x3bd267_rgbf), 0.0f,
        {}, 0.0f,
        {}, {}, {}, {}, {}, {}, {}, {},
        false, false, false, false, false, false},
    {"styled, static, cursor", "colored-cursor.png", 1,
        TextLayerStyleUniform{}
            .setColor(0x3bd267_rgbf), 0.0f,
        {}, 0.0f,
        TextLayerEditingStyleUniform{}
            .setBackgroundColor(0xcd3431_rgbf), {5.0f, 0.0f, 5.0f, 0.0f},
        {}, {}, {}, {}, {}, {},
        false, false, false, false, false, false},
    {"styled, static, selection", "colored-selection.png", 1,
        TextLayerStyleUniform{}
            .setColor(0x3bd267_rgbf), 0.0f,
        {}, 0.0f,
        {}, {},
        TextLayerEditingStyleUniform{}
            .setBackgroundColor(0xc7cf2f_rgbf), {},
        {}, {}, {}, {},
        false, false, false, false, false, false},
    {"styled, static, cursor + selection, colored text", "colored-cursor-selection-text.png", 1,
        TextLayerStyleUniform{}
            .setColor(0x3bd267_rgbf), 0.0f,
        {}, 0.0f,
        TextLayerEditingStyleUniform{}
            .setBackgroundColor(0xcd3431_rgbf), {5.0f, 0.0f, 5.0f, 0.0f},
        TextLayerEditingStyleUniform{}
            .setBackgroundColor(0xc7cf2f_rgbf),
        TextLayerStyleUniform{}
            .setColor(0x1f1f1f_rgbf),
        {}, {}, {}, {},
        false, false, false, false, false, false},
    {"styled, static, create layer after setStyle()", "colored.png", 1,
        TextLayerStyleUniform{}
            .setColor(0x3bd267_rgbf), 0.0f,
        {}, 0.0f,
        {}, {}, {}, {}, {}, {}, {}, {},
        true, false, false, false, false, false},
    {"styled, static, create layer after setStyle(), cursor + selection, colored text", "colored-cursor-selection-text.png", 1,
        TextLayerStyleUniform{}
            .setColor(0x3bd267_rgbf), 0.0f,
        {}, 0.0f,
        TextLayerEditingStyleUniform{}
            .setBackgroundColor(0xcd3431_rgbf), {5.0f, 0.0f, 5.0f, 0.0f},
        TextLayerEditingStyleUniform{}
            .setBackgroundColor(0xc7cf2f_rgbf),
        TextLayerStyleUniform{}
            .setColor(0x1f1f1f_rgbf),
        {}, {}, {}, {},
        true, false, false, false, false, false},
    {"styled, static with padding", "colored.png", 1,
        TextLayerStyleUniform{}
            .setColor(0x3bd267_rgbf), 128.0f,
        {}, 0.0f,
        {}, {}, {}, {}, {}, {}, {}, {},
        false, false, false, false, false, false},
    {"styled, static with padding, cursor + selection", "colored-cursor-selection.png", 1,
        TextLayerStyleUniform{}
            .setColor(0x3bd267_rgbf), 128.0f,
        {}, 0.0f,
        TextLayerEditingStyleUniform{}
            .setBackgroundColor(0xcd3431_rgbf), {5.0f, 0.0f, 5.0f, 0.0f},
        TextLayerEditingStyleUniform{}
            .setBackgroundColor(0xc7cf2f_rgbf), {},
        {}, {}, {}, {},
        false, false, false, false, false, false},

    {"styled, dynamic", "colored.png", 5,
        TextLayerStyleUniform{}, 0.0f,
        TextLayerStyleUniform{}
            .setColor(0x3bd267_rgbf), 0.0f,
        {}, {}, {}, {}, {}, {}, {}, {},
        false, false, false, false, false, false},
    {"styled, dynamic, cursor", "colored-cursor.png", 5,
        TextLayerStyleUniform{}, 0.0f,
        TextLayerStyleUniform{}
            .setColor(0x3bd267_rgbf), 0.0f,
        {}, {}, {}, {},
        TextLayerEditingStyleUniform{}
            .setBackgroundColor(0xcd3431_rgbf), {5.0f, 0.0f, 5.0f, 0.0f},
        {}, {},
        false, false, false, false, false, false},
    {"styled, dynamic, selection", "colored-selection.png", 5,
        TextLayerStyleUniform{}, 0.0f,
        TextLayerStyleUniform{}
            .setColor(0x3bd267_rgbf), 0.0f,
        {}, {}, {}, {},
        {}, {},
        TextLayerEditingStyleUniform{}
            .setBackgroundColor(0xc7cf2f_rgbf), {},
        false, false, false, false, false, false},
    {"styled, dynamic, cursor + selection, colored text", "colored-cursor-selection-text.png", 5,
        TextLayerStyleUniform{}, 0.0f,
        TextLayerStyleUniform{}
            .setColor(0x3bd267_rgbf), 0.0f,
        {}, {}, {}, {},
        TextLayerEditingStyleUniform{}
            .setBackgroundColor(0xcd3431_rgbf), {5.0f, 0.0f, 5.0f, 0.0f},
        TextLayerEditingStyleUniform{}
            .setBackgroundColor(0xc7cf2f_rgbf),
        TextLayerStyleUniform{}
            .setColor(0x1f1f1f_rgbf),
        false, false, false, false, false, false},
    {"styled, dynamic with padding", "colored.png", 5,
        TextLayerStyleUniform{}, 0.0f,
        TextLayerStyleUniform{}
            .setColor(0x3bd267_rgbf), 128.0f,
        {}, {}, {}, {}, {}, {}, {}, {},
        false, false, false, false, false, false},
    {"styled, dynamic with padding, cursor + selection", "colored-cursor-selection.png", 5,
        TextLayerStyleUniform{}, 0.0f,
        TextLayerStyleUniform{}
            .setColor(0x3bd267_rgbf), 128.0f,
        {}, {}, {}, {},
        TextLayerEditingStyleUniform{}
            .setBackgroundColor(0xcd3431_rgbf), {5.0f, 0.0f, 5.0f, 0.0f},
        TextLayerEditingStyleUniform{}
            .setBackgroundColor(0xc7cf2f_rgbf), {},
        false, false, false, false, false, false},
    {"styled, static, secondary upload", "colored.png", 1,
        TextLayerStyleUniform{}
            .setColor(0x3bd267_rgbf), 0.0f,
        {}, 0.0f,
        {}, {}, {}, {}, {}, {}, {}, {},
        false, true, false, false, true, true},
    {"styled, static, secondary upload, cursor + selection, colored text", "colored-cursor-selection-text.png", 1,
        TextLayerStyleUniform{}
            .setColor(0x3bd267_rgbf), 0.0f,
        {}, 0.0f,
        TextLayerEditingStyleUniform{}
            .setBackgroundColor(0xcd3431_rgbf), {5.0f, 0.0f, 5.0f, 0.0f},
        TextLayerEditingStyleUniform{}
            .setBackgroundColor(0xc7cf2f_rgbf),
        TextLayerStyleUniform{}
            .setColor(0x1f1f1f_rgbf),
        {}, {}, {}, {},
        false, true, false, false, true, true},
    {"styled, static, secondary dynamic upload", "colored.png", 1,
        TextLayerStyleUniform{}
            .setColor(0x3bd267_rgbf), 0.0f,
        TextLayerStyleUniform{}, 0.0f,
        {}, {}, {}, {}, {}, {}, {}, {},
        false, false, true, false, false, false},
    {"styled, static, secondary dynamic upload, cursor + selection, colored text", "colored-cursor-selection-text.png", 1,
        TextLayerStyleUniform{}
            .setColor(0x3bd267_rgbf), 0.0f,
        TextLayerStyleUniform{}, 0.0f,
        TextLayerEditingStyleUniform{}
            .setBackgroundColor(0xcd3431_rgbf), {5.0f, 0.0f, 5.0f, 0.0f},
        TextLayerEditingStyleUniform{}
            .setBackgroundColor(0xc7cf2f_rgbf),
        TextLayerStyleUniform{}
            .setColor(0x1f1f1f_rgbf),
        /* The dynamic style is unused, so its upload is non-editing */
        {}, {}, {}, {},
        false, false, true, false, false, false},
    {"styled, dynamic, secondary upload", "colored.png", 5,
        TextLayerStyleUniform{}, 0.0f,
        TextLayerStyleUniform{}
            .setColor(0x3bd267_rgbf), 0.0f,
        {}, {}, {}, {}, {}, {}, {}, {},
        false, false, true, false, true, true},
    {"styled, dynamic, secondary upload, cursor", "colored-cursor.png", 5,
        TextLayerStyleUniform{}, 0.0f,
        TextLayerStyleUniform{}
            .setColor(0x3bd267_rgbf), 0.0f,
        {}, {}, {}, {},
        TextLayerEditingStyleUniform{}
            .setBackgroundColor(0xcd3431_rgbf), {5.0f, 0.0f, 5.0f, 0.0f},
        {}, {},
        false, false, true, false, true, true},
    {"styled, dynamic, secondary upload, selection", "colored-selection.png", 5,
        TextLayerStyleUniform{}, 0.0f,
        TextLayerStyleUniform{}
            .setColor(0x3bd267_rgbf), 0.0f,
        {}, {}, {}, {},
        {}, {},
        TextLayerEditingStyleUniform{}
            .setBackgroundColor(0xc7cf2f_rgbf), {},
        false, false, true, false, true, true},
    {"styled, dynamic, secondary upload, cursor + selection, colored text", "colored-cursor-selection-text.png", 5,
        TextLayerStyleUniform{}, 0.0f,
        TextLayerStyleUniform{}
            .setColor(0x3bd267_rgbf), 0.0f,
        {}, {}, {}, {},
        TextLayerEditingStyleUniform{}
            .setBackgroundColor(0xcd3431_rgbf), {5.0f, 0.0f, 5.0f, 0.0f},
        TextLayerEditingStyleUniform{}
            .setBackgroundColor(0xc7cf2f_rgbf),
        TextLayerStyleUniform{}
            .setColor(0x1f1f1f_rgbf),
        false, false, true, false, true, true},
    {"styled, dynamic, secondary static upload", "colored.png", 5,
        TextLayerStyleUniform{}, 0.0f,
        TextLayerStyleUniform{}
            .setColor(0x3bd267_rgbf), 0.0f,
        {}, {}, {}, {}, {}, {}, {}, {},
        false, true, false, false, false, false},
    {"styled, dynamic, secondary static upload, cursor + selection, colored text", "colored-cursor-selection-text.png", 5,
        TextLayerStyleUniform{}, 0.0f,
        TextLayerStyleUniform{}
            .setColor(0x3bd267_rgbf), 0.0f,
        /* The static style is unused, so its upload is non-editing */
        {}, {}, {}, {},
        TextLayerEditingStyleUniform{}
            .setBackgroundColor(0xcd3431_rgbf), {5.0f, 0.0f, 5.0f, 0.0f},
        TextLayerEditingStyleUniform{}
            .setBackgroundColor(0xc7cf2f_rgbf),
        TextLayerStyleUniform{}
            .setColor(0x1f1f1f_rgbf),
        false, true, false, false, false, false},
    {"styled, only dynamic styles", "colored.png", 1,
        TextLayerStyleUniform{}, 0.0f,
        TextLayerStyleUniform{}
            .setColor(0x3bd267_rgbf), 0.0f,
        {}, {}, {}, {}, {}, {}, {}, {},
        false, false, false, true, false, false},
    {"styled, only dynamic styles, cursor + selection, colored text", "colored-cursor-selection-text.png", 1,
        TextLayerStyleUniform{}, 0.0f,
        TextLayerStyleUniform{}
            .setColor(0x3bd267_rgbf), 0.0f,
        {}, {}, {}, {},
        TextLayerEditingStyleUniform{}
            .setBackgroundColor(0xcd3431_rgbf), {5.0f, 0.0f, 5.0f, 0.0f},
        TextLayerEditingStyleUniform{}
            .setBackgroundColor(0xc7cf2f_rgbf),
        TextLayerStyleUniform{}
            .setColor(0x1f1f1f_rgbf),
        false, false, false, true, false, false},
};

const struct {
    const char* name;
    bool editable;
    bool dataInNodeOrder;
} DrawOrderData[]{
    {"data created in node order", false, true},
    {"data created randomly", false, false},
    {"editable, data created in node order", true, true},
    {"editable, data created randomly", true, false},
};

const struct {
    const char* name;
    const char* filename;
    bool editable;
    bool clip;
    bool singleTopLevel;
    bool flipOrder;
} DrawClippingData[]{
    {"clipping disabled", "clipping-disabled.png",
        false, false, false, false},
    {"clipping top-level nodes", "clipping-enabled.png",
        false, true, false, false},
    {"clipping top-level nodes, different node order", "clipping-enabled.png",
        false, true, false, true},
    {"single top-level node with clipping subnodes", "clipping-enabled.png",
        false, true, true, false},
    {"editable, clipping disabled", "clipping-disabled-editable.png",
        true, false, false, false},
    {"editable, clipping top-level nodes", "clipping-enabled-editable.png",
        true, true, false, false},
    {"editable, clipping top-level nodes, different node order", "clipping-enabled-editable.png",
        true, true, false, true},
    {"editable, single top-level node with clipping subnodes", "clipping-enabled-editable.png",
        true, true, true, false},
};

const struct {
    const char* name;
    bool editableBefore, editableAfter;
    UnsignedInt(*transition)(UnsignedInt);
} EventStyleTransitionData[]{
    {"", false, false,
        [](UnsignedInt style) -> UnsignedInt {
            if(style == 0) return 2;
            CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        }},
    {"editable", true, true,
        [](UnsignedInt style) -> UnsignedInt {
            if(style == 1) return 3;
            CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        }},
    {"editable, editing style only before", true, false,
        [](UnsignedInt style) -> UnsignedInt {
            if(style == 1) return 2;
            CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        }},
    {"editable, editing style only after", false, true,
        [](UnsignedInt style) -> UnsignedInt {
            if(style == 0) return 3;
            CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        }},
};

TextLayerGLTest::TextLayerGLTest() {
    addTests({&TextLayerGLTest::sharedConstruct,
              &TextLayerGLTest::sharedConstructCopy,
              &TextLayerGLTest::sharedConstructMove,

              &TextLayerGLTest::sharedSetGlyphCache,
              &TextLayerGLTest::sharedSetGlyphCacheTakeOwnership,

              &TextLayerGLTest::construct,
              &TextLayerGLTest::constructDerived,
              &TextLayerGLTest::constructCopy,
              &TextLayerGLTest::constructMove,

              &TextLayerGLTest::drawNoSizeSet});

    addInstancedTests({&TextLayerGLTest::drawNoStyleSet},
        Containers::arraySize(DrawNoStyleSetData));

    addInstancedTests({&TextLayerGLTest::render},
        Containers::arraySize(RenderData),
        &TextLayerGLTest::renderSetup,
        &TextLayerGLTest::renderTeardown);

    addInstancedTests({&TextLayerGLTest::renderEdgeSmoothness},
        Containers::arraySize(RenderEdgeSmoothnessData),
        &TextLayerGLTest::renderSetup,
        &TextLayerGLTest::renderTeardown);

    addInstancedTests({&TextLayerGLTest::renderAlignmentPadding},
        Containers::arraySize(RenderAlignmentPaddingData),
        &TextLayerGLTest::renderSetup,
        &TextLayerGLTest::renderTeardown);

    addInstancedTests({&TextLayerGLTest::renderCustomColor},
        Containers::arraySize(RenderCustomColorData),
        &TextLayerGLTest::renderSetup,
        &TextLayerGLTest::renderTeardown);

    addInstancedTests({&TextLayerGLTest::renderChangeStyle},
        Containers::arraySize(RenderChangeStyleData),
        &TextLayerGLTest::renderSetup,
        &TextLayerGLTest::renderTeardown);

    addInstancedTests({&TextLayerGLTest::renderChangeText},
        Containers::arraySize(RenderChangeTextData),
        &TextLayerGLTest::renderSetup,
        &TextLayerGLTest::renderTeardown);

    addInstancedTests({&TextLayerGLTest::renderDynamicStyles},
        Containers::arraySize(RenderDynamicStylesData),
        &TextLayerGLTest::renderSetup,
        &TextLayerGLTest::renderTeardown);

    addInstancedTests({&TextLayerGLTest::drawOrder},
        Containers::arraySize(DrawOrderData),
        &TextLayerGLTest::drawSetup,
        &TextLayerGLTest::drawTeardown);

    addInstancedTests({&TextLayerGLTest::drawClipping},
        Containers::arraySize(DrawClippingData),
        &TextLayerGLTest::drawSetup,
        &TextLayerGLTest::drawTeardown);

    addInstancedTests({&TextLayerGLTest::eventStyleTransition},
        Containers::arraySize(EventStyleTransitionData),
        &TextLayerGLTest::renderSetup,
        &TextLayerGLTest::renderTeardown);

    /* Prefer the StbImageImporter so we can keep files small but always import
       them as four-channel */
    if(PluginManager::PluginMetadata* metadata = _importerManager.metadata("StbImageImporter")) {
        metadata->configuration().setValue("forceChannelCount", 4);
            _importerManager.setPreferredPlugins("PngImporter", {"StbImageImporter"});
    }

    /* Open the font and pre-fill the glyph cache so each test iteration
       doesn't have to suffer stb_truetype's extreme rasterization slowness
       again and again. They only check that the font was opened afterwards. */
    if((_font = _fontManager.loadAndInstantiate("StbTrueTypeFont")) &&
       _font->openFile(Utility::Path::join(UI_DIR, "SourceSansPro-Regular.ttf"), 32.0f))
        _font->fillGlyphCache(_fontGlyphCache, "Magi");
}

void TextLayerGLTest::sharedConstruct() {
    TextLayerGL::Shared shared{TextLayer::Shared::Configuration{3, 5}};
    CORRADE_COMPARE(shared.styleUniformCount(), 3);
    CORRADE_COMPARE(shared.styleCount(), 5);
}

void TextLayerGLTest::sharedConstructCopy() {
    CORRADE_VERIFY(!std::is_copy_constructible<TextLayerGL::Shared>{});
    CORRADE_VERIFY(!std::is_copy_assignable<TextLayerGL::Shared>{});
}

void TextLayerGLTest::sharedConstructMove() {
    TextLayerGL::Shared a{TextLayer::Shared::Configuration{3}};

    TextLayerGL::Shared b{Utility::move(a)};
    CORRADE_COMPARE(b.styleCount(), 3);

    TextLayerGL::Shared c{TextLayer::Shared::Configuration{5}};
    c = Utility::move(b);
    CORRADE_COMPARE(c.styleCount(), 3);

    CORRADE_VERIFY(std::is_nothrow_move_constructible<TextLayerGL::Shared>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<TextLayerGL::Shared>::value);
}

void TextLayerGLTest::sharedSetGlyphCache() {
    Text::GlyphCacheGL cache{PixelFormat::R8Unorm, {32, 32}};
    CORRADE_VERIFY(cache.texture().id());

    {
        TextLayerGL::Shared shared{TextLayer::Shared::Configuration{3}};
        shared.setGlyphCache(cache);
        CORRADE_COMPARE(&shared.glyphCache(), &cache);
    }

    /* It shouldn't get accidentally moved in and deleted */
    CORRADE_VERIFY(cache.texture().id());
}

void TextLayerGLTest::sharedSetGlyphCacheTakeOwnership() {
    Text::GlyphCacheGL cache{PixelFormat::R8Unorm, {32, 32}};
    CORRADE_VERIFY(cache.texture().id());

    {
        TextLayerGL::Shared shared{TextLayer::Shared::Configuration{3}};
        shared.setGlyphCache(Utility::move(cache));

        /* It should get moved in */
        CORRADE_VERIFY(!cache.texture().id());
        CORRADE_VERIFY(&shared.glyphCache() != &cache);
        CORRADE_COMPARE(shared.glyphCache().size(), (Vector3i{32, 32, 1}));
    }

    /** @todo any way to check that a deletion happened? */
}

void TextLayerGLTest::construct() {
    TextLayerGL::Shared shared{TextLayer::Shared::Configuration{3}};

    TextLayerGL layer{layerHandle(137, 0xfe), shared};
    CORRADE_COMPARE(layer.handle(), layerHandle(137, 0xfe));
    CORRADE_COMPARE(&layer.shared(), &shared);
    /* Const overload */
    CORRADE_COMPARE(&static_cast<const TextLayerGL&>(layer).shared(), &shared);
}

void TextLayerGLTest::constructDerived() {
    TextLayerGL::Shared shared{TextLayer::Shared::Configuration{3}};

    /* Verify just that subclassing works without hitting linker errors due to
       virtual symbols not being exported or due to the delegated-to functions
       being private */
    struct: TextLayerGL {
        using TextLayerGL::TextLayerGL;

        void doDraw(const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, std::size_t offset, std::size_t count, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, std::size_t clipRectOffset, std::size_t clipRectCount, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, Containers::BitArrayView nodesEnabled, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes) override {
            return TextLayerGL::doDraw(dataIds, offset, count, clipRectIds, clipRectDataCounts, clipRectOffset, clipRectCount, nodeOffsets, nodeSizes, nodesEnabled, clipRectOffsets, clipRectSizes);
        }
    } layer{layerHandle(137, 0xfe), shared};
    CORRADE_COMPARE(layer.handle(), layerHandle(137, 0xfe));
}

void TextLayerGLTest::constructCopy() {
    CORRADE_VERIFY(!std::is_copy_constructible<TextLayerGL>{});
    CORRADE_VERIFY(!std::is_copy_assignable<TextLayerGL>{});
}

void TextLayerGLTest::constructMove() {
    TextLayerGL::Shared shared{TextLayer::Shared::Configuration{3}};
    TextLayerGL::Shared shared2{TextLayer::Shared::Configuration{5}};

    TextLayerGL a{layerHandle(137, 0xfe), shared};

    TextLayerGL b{Utility::move(a)};
    CORRADE_COMPARE(b.handle(), layerHandle(137, 0xfe));
    CORRADE_COMPARE(&b.shared(), &shared);

    TextLayerGL c{layerHandle(0, 2), shared2};
    c = Utility::move(b);
    CORRADE_COMPARE(c.handle(), layerHandle(137, 0xfe));
    CORRADE_COMPARE(&c.shared(), &shared);

    CORRADE_VERIFY(std::is_nothrow_move_constructible<TextLayerGL>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<TextLayerGL>::value);
}

void TextLayerGLTest::drawNoSizeSet() {
    CORRADE_SKIP_IF_NO_ASSERT();

    TextLayerGL::Shared shared{TextLayer::Shared::Configuration{3}};
    TextLayerGL layer{layerHandle(0, 1), shared};

    std::ostringstream out;
    Error redirectError{&out};
    layer.draw({}, 0, 0, {}, {}, 0, 0, {}, {}, {}, {}, {});
    CORRADE_COMPARE(out.str(), "Ui::TextLayerGL::draw(): user interface size wasn't set\n");
}

void TextLayerGLTest::drawNoStyleSet() {
    auto&& data = DrawNoStyleSetData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    CORRADE_SKIP_IF_NO_ASSERT();

    TextLayerGL::Shared shared{TextLayer::Shared::Configuration{3}
        .setDynamicStyleCount(data.dynamicStyleCount)};
    TextLayerGL layer{layerHandle(0, 1), shared};

    layer.setSize({10, 10}, {10, 10});

    std::ostringstream out;
    Error redirectError{&out};
    layer.draw({}, 0, 0, {}, {}, 0, 0, {}, {}, {}, {}, {});
    CORRADE_COMPARE(out.str(), "Ui::TextLayerGL::draw(): no style data was set\n");
}

void TextLayerGLTest::renderSetup() {
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

void TextLayerGLTest::renderTeardown() {
    _framebuffer = GL::Framebuffer{NoCreate};
    _color = GL::Texture2D{NoCreate};

    GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::disable(GL::Renderer::Feature::ScissorTest);
    GL::Renderer::disable(GL::Renderer::Feature::Blending);
}

void TextLayerGLTest::render() {
    auto&& data = RenderData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    if(!(_fontManager.load("StbTrueTypeFont") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("StbTrueTypeFont plugin not found.");

    AbstractUserInterface ui{RenderSize};
    ui.setRendererInstance(Containers::pointer<RendererGL>());

    /* Opened in the constructor together with cache filling to circumvent
       stb_truetype's extreme rasterization slowness */
    CORRADE_VERIFY(_font && _font->isOpened());

    /* Testing the ArrayView overload, others cases use the initializer list */
    TextLayerStyleUniform styleUniforms[]{
        /* To verify it's not always picking the first uniform */
        TextLayerStyleUniform{},
        TextLayerStyleUniform{},
        data.styleUniform,
        /* Optionally used to override selected text uniform */
        data.styleUniformSelectionText ?
            *data.styleUniformSelectionText : TextLayerStyleUniform{}
    };
    UnsignedInt styleToUniform[]{
        /* To verify it's not using the style ID as uniform ID */
        1, 2, 0, 1, 0
    };
    Int styleCursorStyles[]{
        -1,
         data.styleUniformCursor ? 2 : -1,
         -1,
         -1,
         -1
    };
    Int styleSelectionStyles[]{
        -1,
         data.styleUniformSelection ? 0 : -1,
         -1,
         -1,
         -1
    };
    TextLayerEditingStyleUniform editingStyleUniforms[]{
        /* Again to verify it's not always picking constant IDs */
        TextLayerEditingStyleUniform{},
        data.styleUniformCursor ?
            *data.styleUniformCursor : TextLayerEditingStyleUniform{},
        TextLayerEditingStyleUniform{},
        data.styleUniformSelection ?
            *data.styleUniformSelection : TextLayerEditingStyleUniform{},
    };
    UnsignedInt editingStyleToUniform[]{
        /* Again to verify it's not using the style ID as uniform ID */
        3, 0, 1
    };
    Int editingStyleTextUniforms[]{
        data.styleUniformSelectionText ? 3 : -1,
        -1,
        -1
    };
    Vector4 editingStylePaddings[]{
        data.selectionPadding,
        {},
        data.cursorPadding
    };
    TextLayerGL::Shared layerShared{TextLayer::Shared::Configuration{
        UnsignedInt(Containers::arraySize(styleUniforms)),
        UnsignedInt(Containers::arraySize(styleToUniform))}
            .setEditingStyleCount(
                data.styleUniformEditingCommon ? Containers::arraySize(editingStyleUniforms) : 0,
                data.styleUniformEditingCommon ? Containers::arraySize(editingStyleToUniform) : 0)
    };
    layerShared.setGlyphCache(_fontGlyphCache);
    FontHandle fontHandle[]{
        layerShared.addFont(*_font, 32.0f)
    };
    Text::Alignment alignment[]{
        Text::Alignment::MiddleCenter
    };
    /* The (lack of any) effect of padding on rendered output is tested
       thoroughly in renderAlignmentPadding() */
    layerShared.setStyle(TextLayerCommonStyleUniform{},
        styleUniforms,
        styleToUniform,
        Containers::stridedArrayView(fontHandle).broadcasted<0>(5),
        Containers::stridedArrayView(alignment).broadcasted<0>(5),
        /* There's nothing in features that would affect rendering in a way
           that isn't already tested in TextLayerTest */
        {}, {}, {},
        styleCursorStyles,
        styleSelectionStyles,
        {});
    if(data.styleUniformEditingCommon) {
        layerShared.setEditingStyle(*data.styleUniformEditingCommon,
            editingStyleUniforms,
            editingStyleToUniform,
            editingStyleTextUniforms,
            editingStylePaddings);
    }

    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<TextLayerGL>(layer, layerShared));

    NodeHandle node = ui.createNode({8.0f, 8.0f}, {112.0f, 48.0f});
    /* Using a text that has glyphs both above and below line and doesn't need
       too many glyphs */
    if(data.singleGlyph) {
        ui.layer<TextLayerGL>(layer).createGlyph(1, _font->glyphId('g'), {}, node);
        CORRADE_INTERNAL_ASSERT(!data.styleUniformEditingCommon);
    } else {
        DataHandle nodeData = ui.layer<TextLayerGL>(layer).create(1, "Maggi", {},
            data.styleUniformEditingCommon ? TextDataFlag::Editable : TextDataFlags{},
            node);
        if(data.styleUniformEditingCommon)
            ui.layer<TextLayerGL>(layer).setCursor(nodeData, data.cursor, data.selection);
    }

    ui.draw();

    MAGNUM_VERIFY_NO_GL_ERROR();

    if(!(_importerManager.load("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_importerManager.load("StbImageImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / StbImageImporter plugins not found.");

    #if defined(MAGNUM_TARGET_GLES) && !defined(MAGNUM_TARGET_WEBGL)
    /* Same problem is with all builtin shaders, so this doesn't seem to be a
       bug in the text layer shader code */
    if(GL::Context::current().detectedDriver() & GL::Context::DetectedDriver::SwiftShader)
        CORRADE_SKIP("UBOs with dynamically indexed arrays don't seem to work on SwiftShader, can't test.");
    #endif
    CORRADE_COMPARE_WITH(_framebuffer.read({{}, RenderSize}, {PixelFormat::RGBA8Unorm}),
        Utility::Path::join({UI_TEST_DIR, "TextLayerTestFiles", data.filename}),
        DebugTools::CompareImageToFile{_importerManager});
}

void TextLayerGLTest::renderEdgeSmoothness() {
    auto&& data = RenderEdgeSmoothnessData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    if(!(_fontManager.load("StbTrueTypeFont") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("StbTrueTypeFont plugin not found.");

    /* A stripped-down variant of render(colored, cursor + selection style)
       that has excessive smoothness to test doesn't get cut off. It should
       produce the same result (5 *pixel* smoothness) regardless of the actual
       UI size. */

    /* Window size isn't used for anything here, can be arbitrary. If the size
       is meant to be set later, start with the framebuffer being 1x1. The UI
       size has to stay unchanged, otherwise it'll set NeedsNodeClipUpdate,
       which triggers data regeneration always. */
    AbstractUserInterface ui{data.scale*Vector2{RenderSize}, {1, 1}, (data.setSizeLater ? Vector2i{1} : RenderSize)};
    ui.setRendererInstance(Containers::pointer<RendererGL>());

    /* Opened in the constructor together with cache filling to circumvent
       stb_truetype's extreme rasterization slowness */
    CORRADE_VERIFY(_font && _font->isOpened());

    TextLayerGL::Shared layerShared{TextLayer::Shared::Configuration{1}
        .setEditingStyleCount(2)};
    layerShared.setGlyphCache(_fontGlyphCache);
    /* Verifying the ArrayView overload with implicit uniform mapping as that's
       not tested anywhere else */
    TextLayerStyleUniform uniforms[]{
        TextLayerStyleUniform{}
            .setColor(0x3bd267_rgbf),
    };
    FontHandle fonts[]{
        layerShared.addFont(*_font, 32.0f*data.scale),
    };
    Text::Alignment alignment[]{
        Text::Alignment::MiddleCenter,
    };
    Int cursorStyles[]{0};
    Int selectionStyles[]{1};
    layerShared.setStyle(
        TextLayerCommonStyleUniform{},
        uniforms,
        fonts,
        alignment,
        {}, {}, {},
        cursorStyles,
        selectionStyles,
        {});
    layerShared.setEditingStyle(
        TextLayerCommonEditingStyleUniform{}
            .setSmoothness(5.0f),
        {TextLayerEditingStyleUniform{}
            .setBackgroundColor(0xcd3431_rgbf)
            .setCornerRadius(5.5f*data.scale),
         TextLayerEditingStyleUniform{}
            .setBackgroundColor(0xc7cf2f_rgbf)
            .setCornerRadius(10.0f*data.scale)},
        {},
        {Vector4{10.0f, -5.0f, -1.0f, 0.0f}*data.scale,
         Vector4{5.0f, 0.0f, 7.5f, 5.0f}*data.scale});

    TextLayer& layer = ui.setLayerInstance(Containers::pointer<TextLayerGL>(ui.createLayer(), layerShared));

    NodeHandle node = ui.createNode(Vector2{8.0f, 8.0f}*data.scale,
                                    Vector2{112.0f, 48.0f}*data.scale);
    DataHandle nodeData = layer.create(0, "Maggi", {}, TextDataFlag::Editable, node);
    layer.setCursor(nodeData, 2, 5);

    if(data.setSizeLater) {
        /* Make sure everything is already processed before updating the size,
           otherwise it'd be all deferred to draw() below, circumventing what
           we want to test */
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});

        /* Unlike with BaseLayer, setting the size doesn't need to trigger any
           data regeneration as the pixel-ratio-dependent smoothness expansion
           is done directly in the shader */
        ui.setSize(data.scale*Vector2{RenderSize}, {1, 1}, RenderSize);
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    ui.draw();

    MAGNUM_VERIFY_NO_GL_ERROR();

    if(!(_importerManager.load("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_importerManager.load("StbImageImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / StbImageImporter plugins not found.");

    #if defined(MAGNUM_TARGET_GLES) && !defined(MAGNUM_TARGET_WEBGL)
    /* Same problem is with all builtin shaders, so this doesn't seem to be a
       bug in the text layer shader code */
    if(GL::Context::current().detectedDriver() & GL::Context::DetectedDriver::SwiftShader)
        CORRADE_SKIP("UBOs with dynamically indexed arrays don't seem to work on SwiftShader, can't test.");
    #endif
    CORRADE_COMPARE_WITH(_framebuffer.read({{}, RenderSize}, {PixelFormat::RGBA8Unorm}),
        Utility::Path::join(UI_TEST_DIR, "TextLayerTestFiles/colored-cursor-selection-rounded-smooth.png"),
        DebugTools::CompareImageToFile{_importerManager});
}

void TextLayerGLTest::renderAlignmentPadding() {
    auto&& data = RenderAlignmentPaddingData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Matches either the "colored" or the  "colored, cursor + selection style"
       case in render(). Verifying also the non-editable variant to avoid an
       accident where alignment would be done fully only for editing style. */

    if(!(_fontManager.load("StbTrueTypeFont") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("StbTrueTypeFont plugin not found.");

    AbstractUserInterface ui{RenderSize};
    ui.setRendererInstance(Containers::pointer<RendererGL>());

    /* Opened in the constructor together with cache filling to circumvent
       stb_truetype's extreme rasterization slowness */
    CORRADE_VERIFY(_font && _font->isOpened());

    TextLayerGL::Shared layerShared{TextLayer::Shared::Configuration{1}
        .setEditingStyleCount(data.editable ? 2 : 0)
    };
    layerShared.setGlyphCache(_fontGlyphCache);

    FontHandle fontHandle = layerShared.addFont(*_font, 32.0f);
    layerShared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}
            .setColor(0x3bd267_rgbf)},
        {fontHandle},
        {data.alignment},
        {}, {}, {},
        {data.editable ? 0 : -1}, {data.editable ? 1 : -1},
        {data.paddingFromStyle});
    if(data.editable) layerShared.setEditingStyle(
        TextLayerCommonEditingStyleUniform{},
        {TextLayerEditingStyleUniform{}
            .setBackgroundColor(0xcd3431_rgbf),
         TextLayerEditingStyleUniform{}
            .setBackgroundColor(0xc7cf2f_rgbf)},
        {},
        {{5.0f, 0.0f, 5.0f, 0.0f}, {}});

    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<TextLayerGL>(layer, layerShared));

    NodeHandle node = ui.createNode(data.nodeOffset, data.nodeSize);
    DataHandle nodeData = ui.layer<TextLayerGL>(layer).create(0, "Maggi", {},
        data.editable ? TextDataFlag::Editable : TextDataFlags{},
        node);
    if(data.editable)
        ui.layer<TextLayerGL>(layer).setCursor(nodeData, 2, 5);

    if(data.partialUpdate) {
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    if(!data.paddingFromData.isZero()) {
        ui.layer<TextLayerGL>(layer).setPadding(nodeData, data.paddingFromData);
        CORRADE_COMPARE_AS(ui.state(),
            UserInterfaceState::NeedsDataUpdate,
            TestSuite::Compare::GreaterOrEqual);
    }

    ui.draw();

    MAGNUM_VERIFY_NO_GL_ERROR();

    if(!(_importerManager.load("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_importerManager.load("StbImageImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / StbImageImporter plugins not found.");

    #if defined(MAGNUM_TARGET_GLES) && !defined(MAGNUM_TARGET_WEBGL)
    /* Same problem is with all builtin shaders, so this doesn't seem to be a
       bug in the text layer shader code */
    if(GL::Context::current().detectedDriver() & GL::Context::DetectedDriver::SwiftShader)
        CORRADE_SKIP("UBOs with dynamically indexed arrays don't seem to work on SwiftShader, can't test.");
    #endif
    CORRADE_COMPARE_WITH(_framebuffer.read({{}, RenderSize}, {PixelFormat::RGBA8Unorm}),
        Utility::Path::join(UI_TEST_DIR,
            data.editable ?
                "TextLayerTestFiles/colored-cursor-selection.png" :
                "TextLayerTestFiles/colored.png"),
        DebugTools::CompareImageToFile{_importerManager});
}

void TextLayerGLTest::renderCustomColor() {
    auto&& data = RenderCustomColorData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Basically the same as the "colored" case in render(), except that the
       color is additionally taken from the per-vertex data as well */

    if(!(_fontManager.load("StbTrueTypeFont") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("StbTrueTypeFont plugin not found.");

    AbstractUserInterface ui{RenderSize};
    ui.setRendererInstance(Containers::pointer<RendererGL>());

    /* Opened in the constructor together with cache filling to circumvent
       stb_truetype's extreme rasterization slowness */
    CORRADE_VERIFY(_font && _font->isOpened());

    TextLayerGL::Shared layerShared{TextLayer::Shared::Configuration{2, 1}
        .setEditingStyleCount(data.editable ? 2 : 0)
    };
    layerShared.setGlyphCache(_fontGlyphCache);

    FontHandle fontHandle = layerShared.addFont(*_font, 32.0f);
    layerShared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}
            .setColor(0x3bd267_rgbf/0x336699_rgbf),
         TextLayerStyleUniform{}
            /* This may become an issue with lowp, let's hope it won't */
            .setColor(0x1f1f1f_rgbf/0x336699_rgbf)},
        {0},
        {fontHandle},
        {Text::Alignment::MiddleCenter},
        {}, {}, {},
        {data.editable ? 0 : -1}, {data.editable ? 1 : -1},
        {});
    if(data.editable) layerShared.setEditingStyle(
        TextLayerCommonEditingStyleUniform{},
        {TextLayerEditingStyleUniform{} /* not affected by the color */
            .setBackgroundColor(0xcd3431_rgbf),
         TextLayerEditingStyleUniform{} /* not affected by the color either */
            .setBackgroundColor(0xc7cf2f_rgbf)},
        {-1, 1},
        {{5.0f, 0.0f, 5.0f, 0.0f}, {}});

    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<TextLayerGL>(layer, layerShared));

    NodeHandle node = ui.createNode({8.0f, 8.0f}, {112.0f, 48.0f});
    TextDataFlags flags = data.editable ? TextDataFlag::Editable : TextDataFlags{};
    DataHandle nodeData = ui.layer<TextLayerGL>(layer).create(0, "Maggi", {}, flags, node);
    if(data.editable)
        ui.layer<TextLayerGL>(layer).setCursor(nodeData, 2, 5);

    if(data.partialUpdate) {
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    ui.layer<TextLayerGL>(layer).setColor(nodeData, 0x336699_rgbf);
    CORRADE_COMPARE_AS(ui.state(),
        UserInterfaceState::NeedsDataUpdate,
        TestSuite::Compare::GreaterOrEqual);

    ui.draw();

    MAGNUM_VERIFY_NO_GL_ERROR();

    if(!(_importerManager.load("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_importerManager.load("StbImageImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / StbImageImporter plugins not found.");

    #if defined(MAGNUM_TARGET_GLES) && !defined(MAGNUM_TARGET_WEBGL)
    /* Same problem is with all builtin shaders, so this doesn't seem to be a
       bug in the text layer shader code */
    if(GL::Context::current().detectedDriver() & GL::Context::DetectedDriver::SwiftShader)
        CORRADE_SKIP("UBOs with dynamically indexed arrays don't seem to work on SwiftShader, can't test.");
    #endif
    CORRADE_COMPARE_WITH(_framebuffer.read({{}, RenderSize}, {PixelFormat::RGBA8Unorm}),
        Utility::Path::join(UI_TEST_DIR, data.editable ?
            "TextLayerTestFiles/colored-cursor-selection-text.png" :
            "TextLayerTestFiles/colored.png"),
        DebugTools::CompareImageToFile{_importerManager});
}

void TextLayerGLTest::renderChangeStyle() {
    auto&& data = RenderChangeStyleData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Basically the same as the "colored" /
       "colored, cursor + selection style, colored text" cases in render(),
       except that the style ID is changed to it only later. Verifying also the
       non-editable variant to avoid an accident where state update would be
       done fully only for editing style, also testing getting or losing the
       editing style. */

    if(!(_fontManager.load("StbTrueTypeFont") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("StbTrueTypeFont plugin not found.");

    AbstractUserInterface ui{RenderSize};
    ui.setRendererInstance(Containers::pointer<RendererGL>());

    /* Opened in the constructor together with cache filling to circumvent
       stb_truetype's extreme rasterization slowness */
    CORRADE_VERIFY(_font && _font->isOpened());

    TextLayerGL::Shared layerShared{TextLayer::Shared::Configuration{4, 3}
        .setEditingStyleCount(data.editableBefore || data.editableAfter ? 3 : 0)
    };
    layerShared.setGlyphCache(_fontGlyphCache);

    FontHandle fontHandle = layerShared.addFont(*_font, 32.0f);
    layerShared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{},
         TextLayerStyleUniform{}        /* colored */
            .setColor(0x3bd267_rgbf),
         TextLayerStyleUniform{}         /* colored, cursor + selection style */
            .setColor(0x3bd267_rgbf),
         TextLayerStyleUniform{}         /* Selected text override */
            .setColor(0x1f1f1f_rgbf)},
        {0, 1, 2},
        {fontHandle, fontHandle, fontHandle},
        {Text::Alignment::MiddleCenter,
         Text::Alignment::MiddleCenter,
         Text::Alignment::MiddleCenter},
        {}, {}, {},
        {data.editableBefore ? 0 : -1,
         -1,
         data.editableAfter ? 1 : -1},
        {data.editableBefore ? 0 : -1,
         -1,
         data.editableAfter ? 2 : -1},
        {});
    if(data.editableBefore || data.editableAfter) layerShared.setEditingStyle(
        TextLayerCommonEditingStyleUniform{},
        {TextLayerEditingStyleUniform{},
         TextLayerEditingStyleUniform{}
            .setBackgroundColor(0xcd3431_rgbf),
         TextLayerEditingStyleUniform{}
            .setBackgroundColor(0xc7cf2f_rgbf)},
        {-1, -1, 3},
        {{}, {5.0f, 0.0f, 5.0f, 0.0f}, {}});

    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<TextLayerGL>(layer, layerShared));

    NodeHandle node = ui.createNode({8.0f, 8.0f}, {112.0f, 48.0f});
    DataHandle nodeData = ui.layer<TextLayerGL>(layer).create(0, "Maggi", {},
        data.editableBefore || data.editableAfter ? TextDataFlag::Editable : TextDataFlags{},
        node);
    if(data.editableBefore || data.editableAfter)
        ui.layer<TextLayerGL>(layer).setCursor(nodeData, 2, 5);

    if(data.partialUpdate) {
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    ui.layer<TextLayerGL>(layer).setStyle(nodeData, data.editableAfter ? 2 : 1);
    CORRADE_COMPARE_AS(ui.state(),
        UserInterfaceState::NeedsDataUpdate,
        TestSuite::Compare::GreaterOrEqual);

    ui.draw();

    MAGNUM_VERIFY_NO_GL_ERROR();

    if(!(_importerManager.load("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_importerManager.load("StbImageImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / StbImageImporter plugins not found.");

    #if defined(MAGNUM_TARGET_GLES) && !defined(MAGNUM_TARGET_WEBGL)
    /* Same problem is with all builtin shaders, so this doesn't seem to be a
       bug in the text layer shader code */
    if(GL::Context::current().detectedDriver() & GL::Context::DetectedDriver::SwiftShader)
        CORRADE_SKIP("UBOs with dynamically indexed arrays don't seem to work on SwiftShader, can't test.");
    #endif
    CORRADE_COMPARE_WITH(_framebuffer.read({{}, RenderSize}, {PixelFormat::RGBA8Unorm}),
        Utility::Path::join(UI_TEST_DIR,
            data.editableAfter ?
                "TextLayerTestFiles/colored-cursor-selection-text.png" :
                "TextLayerTestFiles/colored.png"),
        DebugTools::CompareImageToFile{_importerManager});
}

void TextLayerGLTest::renderChangeText() {
    auto&& data = RenderChangeTextData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Basically the same as the "colored" /
       "colored, cursor + selection style, colored text" cases in render(),
       except that the text is changed only subsequently. Verifying also the
       non-editable variant to avoid an accident where state update would be
       done fully only for editing style, and a case where a non-editable text
       becomes editable and vice versa. */

    if(!(_fontManager.load("StbTrueTypeFont") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("StbTrueTypeFont plugin not found.");

    AbstractUserInterface ui{RenderSize};
    ui.setRendererInstance(Containers::pointer<RendererGL>());

    /* Opened in the constructor together with cache filling to circumvent
       stb_truetype's extreme rasterization slowness */
    CORRADE_VERIFY(_font && _font->isOpened());

    TextLayerGL::Shared layerShared{TextLayer::Shared::Configuration{2, 1}
        .setEditingStyleCount(data.editableBefore || data.editableAfter ? 2 : 0)
    };
    layerShared.setGlyphCache(_fontGlyphCache);

    FontHandle fontHandle = layerShared.addFont(*_font, 32.0f);
    layerShared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}
            .setColor(0x3bd267_rgbf),
         TextLayerStyleUniform{}         /* Selected text override */
            .setColor(0x1f1f1f_rgbf)},
        {0},
        {fontHandle},
        {Text::Alignment::MiddleCenter},
        {}, {}, {},
        {data.editableBefore || data.editableAfter ? 0 : -1},
        {data.editableBefore || data.editableAfter ? 1 : -1},
        {});
    if(data.editableBefore || data.editableAfter) layerShared.setEditingStyle(
        TextLayerCommonEditingStyleUniform{},
        {TextLayerEditingStyleUniform{}
            .setBackgroundColor(0xcd3431_rgbf),
         TextLayerEditingStyleUniform{}
            .setBackgroundColor(0xc7cf2f_rgbf)},
        {-1, 1},
        {{5.0f, 0.0f, 5.0f, 0.0f}, {}});

    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<TextLayerGL>(layer, layerShared));

    NodeHandle node = ui.createNode({8.0f, 8.0f}, {112.0f, 48.0f});
    DataHandle nodeData = ui.layer<TextLayerGL>(layer).create(0, "gM!", {},
        data.editableBefore ? TextDataFlag::Editable : TextDataFlags{},
        node);
    if(data.editableBefore)
        ui.layer<TextLayerGL>(layer).setCursor(nodeData, 1, 3);

    if(data.partialUpdate) {
        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    }

    /* If no flags are specified, they're carried over */
    if(data.editableAfter == data.editableBefore)
        ui.layer<TextLayerGL>(layer).setText(nodeData, "Maggi", {});
    else
        ui.layer<TextLayerGL>(layer).setText(nodeData, "Maggi", {},
            data.editableAfter ? TextDataFlag::Editable : TextDataFlags{});
    if(data.editableAfter)
        ui.layer<TextLayerGL>(layer).setCursor(nodeData, 2, 5);
    CORRADE_COMPARE_AS(ui.state(),
        UserInterfaceState::NeedsDataUpdate,
        TestSuite::Compare::GreaterOrEqual);

    ui.draw();

    MAGNUM_VERIFY_NO_GL_ERROR();

    if(!(_importerManager.load("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_importerManager.load("StbImageImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / StbImageImporter plugins not found.");

    #if defined(MAGNUM_TARGET_GLES) && !defined(MAGNUM_TARGET_WEBGL)
    /* Same problem is with all builtin shaders, so this doesn't seem to be a
       bug in the text layer shader code */
    if(GL::Context::current().detectedDriver() & GL::Context::DetectedDriver::SwiftShader)
        CORRADE_SKIP("UBOs with dynamically indexed arrays don't seem to work on SwiftShader, can't test.");
    #endif
    CORRADE_COMPARE_WITH(_framebuffer.read({{}, RenderSize}, {PixelFormat::RGBA8Unorm}),
        Utility::Path::join(UI_TEST_DIR,
            data.editableAfter ?
                "TextLayerTestFiles/colored-cursor-selection-text.png" :
                "TextLayerTestFiles/colored.png"),
        DebugTools::CompareImageToFile{_importerManager});
}

void TextLayerGLTest::renderDynamicStyles() {
    auto&& data = RenderDynamicStylesData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    if(!(_fontManager.load("StbTrueTypeFont") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("StbTrueTypeFont plugin not found.");

    AbstractUserInterface ui{RenderSize};
    ui.setRendererInstance(Containers::pointer<RendererGL>());

    /* Opened in the constructor together with cache filling to circumvent
       stb_truetype's extreme rasterization slowness */
    CORRADE_VERIFY(_font && _font->isOpened());

    TextLayerGL::Shared layerShared{
        TextLayer::Shared::Configuration{data.noBaseStyles ? 0u : 3u,
                                         data.noBaseStyles ? 0u : 4u}
            /* Include editing styles only if there's any set and base styles
               aren't turned off altogether */
            .setEditingStyleCount(
                data.noBaseStyles || (!data.styleUniformCursor && !data.styleUniformSelection && !data.dynamicStyleUniformCursor && !data.dynamicStyleUniformSelection) ? 0 : 3,
                data.noBaseStyles || (!data.styleUniformCursor && !data.styleUniformSelection && !data.dynamicStyleUniformCursor && !data.dynamicStyleUniformSelection) ? 0 : 4)
            /* Include dynamic editing styles if there's any */
            .setDynamicStyleCount(2, data.dynamicStyleUniformCursor || data.dynamicStyleUniformSelection)
    };
    layerShared.setGlyphCache(_fontGlyphCache);
    FontHandle fontHandle = layerShared.addFont(*_font, 32.0f);

    TextLayerGL* layer{};
    if(!data.createLayerAfterSetStyle)
        layer = &ui.setLayerInstance(Containers::pointer<TextLayerGL>(ui.createLayer(), layerShared));

    /* If the style is being uploaded second time, upload just a default state
       at first */
    if(data.secondaryStyleUpload) {
        layerShared.setStyle(TextLayerCommonStyleUniform{},
            {TextLayerStyleUniform{}, TextLayerStyleUniform{}, TextLayerStyleUniform{}},
            /* The mapping is deliberately different, the secondary upload
               should cause it to be updated */
            {2, 1, 1, 0},
            {FontHandle::Null, FontHandle::Null, FontHandle::Null, FontHandle::Null},
            {Text::Alignment{}, Text::Alignment{}, Text::Alignment{}, Text::Alignment{}},
            {}, {}, {},
            {}, {},
            {});
        if(data.styleUniformCursor || data.styleUniformSelection || data.dynamicStyleUniformCursor || data.dynamicStyleUniformSelection)
            layerShared.setEditingStyle(TextLayerCommonEditingStyleUniform{},
                {TextLayerEditingStyleUniform{}, TextLayerEditingStyleUniform{}, TextLayerEditingStyleUniform{}},
                /* The mapping is again deliberately different, the secondary
                   upload should cause it to be updated */
                {0, 1, 0, 2},
                {},
                {{}, {}, {}, {}});
    } else if(data.noBaseStyles) {
        layerShared.setStyle(TextLayerCommonStyleUniform{},
            {},
            {},
            {},
            {}, {}, {},
            {}, {},
            {});
        if(data.styleUniformCursor || data.styleUniformSelection || data.dynamicStyleUniformCursor || data.dynamicStyleUniformSelection)
            layerShared.setEditingStyle(TextLayerCommonEditingStyleUniform{},
                {},
                {},
                {});
    } else {
        layerShared.setStyle(TextLayerCommonStyleUniform{},
            {TextLayerStyleUniform{},
             data.styleUniformSelectionText ?
                *data.styleUniformSelectionText : TextLayerStyleUniform{},
             data.styleUniform},
            {1, 2, 0, 1},
            {FontHandle::Null, fontHandle, FontHandle::Null, FontHandle::Null},
            {Text::Alignment::BottomRight,
             Text::Alignment::MiddleCenter,
             Text::Alignment::TopCenterIntegral,
             Text::Alignment::LineLeft},
            /* There's nothing in features that would affect rendering in a way
               that isn't already tested in TextLayerTest */
            {}, {}, {},
            {-1, data.styleUniformCursor ? 2 : -1, -1, -1},
            {-1, data.styleUniformSelection ? 0 : -1, -1, -1},
            {{}, {data.leftPadding, 0.0f, 0.0f, 0.0f}, {}, {}});
        if(data.styleUniformCursor || data.styleUniformSelection || data.dynamicStyleUniformCursor || data.dynamicStyleUniformSelection) {
            layerShared.setEditingStyle(TextLayerCommonEditingStyleUniform{},
                {TextLayerEditingStyleUniform{},
                 data.styleUniformCursor ?
                    *data.styleUniformCursor : TextLayerEditingStyleUniform{},
                 data.styleUniformSelection ?
                    *data.styleUniformSelection : TextLayerEditingStyleUniform{}},
                {2, 0, 1, 0},
                {data.styleUniformSelectionText ? 1 : -1, -1, -1, -1},
                {{}, {}, data.cursorPadding, {}});
        }
    }

    /* If the layer is created after the setStyle() call, it should have no
       LayerStates set implicitly, otherwise setStyle() causes the state to be
       set on all existing layers */
    if(data.createLayerAfterSetStyle) {
        layer = &ui.setLayerInstance(Containers::pointer<TextLayerGL>(ui.createLayer(), layerShared));
        CORRADE_COMPARE(layer->state(), LayerStates{});
    } else {
        CORRADE_COMPARE(layer->state(), LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate);
    }

    if(data.dynamicStyleUniform) {
        /* Again, if the dynamic style  is being uploaded second time, upload
           just a default state at first. Preserve the presence of cursor /
           editing style tho, to not trigger NeedsDataUpdate as well. */
        if(data.secondaryDynamicStyleUpload) {
            if(data.dynamicStyleUniformCursor && data.dynamicStyleUniformSelection) {
                layer->setDynamicStyleWithCursorSelection(1,
                    TextLayerStyleUniform{},
                    FontHandle::Null,
                    Text::Alignment{},
                    {}, {},
                    /* Changing padding triggers NeedsDataUpdate also, so set
                       it initially already */
                    TextLayerEditingStyleUniform{}, data.dynamicCursorPadding,
                    TextLayerEditingStyleUniform{}, {}, {});
            } else if(data.dynamicStyleUniformCursor) {
                layer->setDynamicStyleWithCursor(1,
                    TextLayerStyleUniform{},
                    FontHandle::Null,
                    Text::Alignment{},
                    {}, {},
                    /* Changing padding triggers NeedsDataUpdate also, so set
                       it initially already */
                    TextLayerEditingStyleUniform{}, data.dynamicCursorPadding);
            } else if(data.dynamicStyleUniformSelection) {
                layer->setDynamicStyleWithSelection(1,
                    TextLayerStyleUniform{},
                    FontHandle::Null,
                    Text::Alignment{},
                    {}, {},
                    TextLayerEditingStyleUniform{}, {}, {});
            } else {
                layer->setDynamicStyle(1,
                    TextLayerStyleUniform{},
                    FontHandle::Null,
                    Text::Alignment{},
                    {}, {});
            }
        } else if(data.dynamicStyleUniformCursor && data.dynamicStyleUniformSelection) {
            layer->setDynamicStyleWithCursorSelection(1,
                *data.dynamicStyleUniform,
                fontHandle,
                Text::Alignment::MiddleCenter,
                {},
                {data.dynamicLeftPadding, 0.0f, 0.0f, 0.0f},
                *data.dynamicStyleUniformCursor, data.dynamicCursorPadding,
                *data.dynamicStyleUniformSelection, data.dynamicStyleUniformSelectionText, {});
        } else if(data.dynamicStyleUniformCursor) {
            layer->setDynamicStyleWithCursor(1,
                *data.dynamicStyleUniform,
                fontHandle,
                Text::Alignment::MiddleCenter,
                {},
                {data.dynamicLeftPadding, 0.0f, 0.0f, 0.0f},
                *data.dynamicStyleUniformCursor, data.dynamicCursorPadding);
        } else if(data.dynamicStyleUniformSelection) {
            layer->setDynamicStyleWithSelection(1,
                *data.dynamicStyleUniform,
                fontHandle,
                Text::Alignment::MiddleCenter,
                {},
                {data.dynamicLeftPadding, 0.0f, 0.0f, 0.0f},
                *data.dynamicStyleUniformSelection, data.dynamicStyleUniformSelectionText, {});
        } else {
            layer->setDynamicStyle(1,
                *data.dynamicStyleUniform,
                fontHandle,
                Text::Alignment::MiddleCenter,
                {},
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
    /* If a dynamic style with a null font handle / bogus alignment is used,
       need to pass it explicitly to create() instead */
    TextProperties properties;
    if(data.explicitFont)
        properties.setFont(fontHandle);
    if(data.explicitAlignment)
        properties.setAlignment(Text::Alignment::MiddleCenter);
    /* There isn't any difference in handling of text ḿade with create() or
       createGlyph() inside draw() so this tests just one */
    DataHandle nodeData = layer->create(data.styleIndex, "Maggi", properties,
        data.styleUniformCursor || data.styleUniformSelection || data.dynamicStyleUniformCursor || data.dynamicStyleUniformSelection ?
            TextDataFlag::Editable : TextDataFlags{},
        node);
    if(data.styleUniformCursor || data.styleUniformSelection || data.dynamicStyleUniformCursor || data.dynamicStyleUniformSelection)
        layer->setCursor(nodeData, 2, 5);

    /* If there's a secondary upload, draw & clear to force the first upload */
    if(data.secondaryStyleUpload || data.secondaryDynamicStyleUpload) {
        ui.draw();
        CORRADE_COMPARE(layer->state(), LayerStates{});
        _framebuffer.clear(GL::FramebufferClear::Color);
    }

    /* Upload the actual style data only second time if desired */
    if(data.secondaryStyleUpload) {
        layerShared.setStyle(TextLayerCommonStyleUniform{},
            {TextLayerStyleUniform{},
             data.styleUniformSelectionText ?
                *data.styleUniformSelectionText : TextLayerStyleUniform{},
             data.styleUniform},
            {1, 2, 0, 1},
            {FontHandle::Null, fontHandle, FontHandle::Null, FontHandle::Null},
            {Text::Alignment::BottomRight,
             Text::Alignment::MiddleCenter,
             Text::Alignment::TopCenterIntegral,
             Text::Alignment::LineLeft},
            /* There's nothing in features that would affect rendering in a way
               that isn't already tested in TextLayerTest */
            {}, {}, {},
            {-1, data.styleUniformCursor ? 2 : -1, -1, -1},
            {-1, data.styleUniformSelection ? 0 : -1, -1, -1},
            {{}, {data.leftPadding, 0.0f, 0.0f, 0.0f}, {}, {}});
        if(data.styleUniformCursor || data.styleUniformSelection || data.dynamicStyleUniformCursor || data.dynamicStyleUniformSelection) {
            layerShared.setEditingStyle(TextLayerCommonEditingStyleUniform{},
                {TextLayerEditingStyleUniform{},
                 data.styleUniformCursor ?
                    *data.styleUniformCursor : TextLayerEditingStyleUniform{},
                 data.styleUniformSelection ?
                    *data.styleUniformSelection : TextLayerEditingStyleUniform{}},
                {2, 0, 1, 0},
                {data.styleUniformSelectionText ? 1 : -1, -1, -1, -1},
                {{}, {}, data.cursorPadding, {}});
        }
        CORRADE_COMPARE(layer->state(), LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate);
    }
    if(data.secondaryDynamicStyleUpload) {
        if(data.dynamicStyleUniformCursor && data.dynamicStyleUniformSelection) {
            layer->setDynamicStyleWithCursorSelection(1,
                *data.dynamicStyleUniform,
                fontHandle,
                Text::Alignment::MiddleCenter,
                {},
                {data.dynamicLeftPadding, 0.0f, 0.0f, 0.0f},
                *data.dynamicStyleUniformCursor, data.dynamicCursorPadding,
                *data.dynamicStyleUniformSelection, data.dynamicStyleUniformSelectionText, {});
        } else if(data.dynamicStyleUniformCursor) {
            layer->setDynamicStyleWithCursor(1,
                *data.dynamicStyleUniform,
                fontHandle,
                Text::Alignment::MiddleCenter,
                {},
                {data.dynamicLeftPadding, 0.0f, 0.0f, 0.0f},
                *data.dynamicStyleUniformCursor, data.dynamicCursorPadding);
        } else if(data.dynamicStyleUniformSelection) {
            layer->setDynamicStyleWithSelection(1,
                *data.dynamicStyleUniform,
                fontHandle,
                Text::Alignment::MiddleCenter,
                {},
                {data.dynamicLeftPadding, 0.0f, 0.0f, 0.0f},
                *data.dynamicStyleUniformSelection, data.dynamicStyleUniformSelectionText, {});
        } else {
            layer->setDynamicStyle(1,
                *data.dynamicStyleUniform,
                fontHandle,
                Text::Alignment::MiddleCenter,
                {},
                {data.dynamicLeftPadding, 0.0f, 0.0f, 0.0f});
        }
        CORRADE_COMPARE(layer->state(), LayerState::NeedsCommonDataUpdate);
    }

    ui.draw();
    CORRADE_COMPARE(layer->state(), LayerStates{});

    MAGNUM_VERIFY_NO_GL_ERROR();

    if(!(_importerManager.load("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_importerManager.load("StbImageImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / StbImageImporter plugins not found.");

    #if defined(MAGNUM_TARGET_GLES) && !defined(MAGNUM_TARGET_WEBGL)
    /* Same problem is with all builtin shaders, so this doesn't seem to be a
       bug in the base layer shader code */
    if(GL::Context::current().detectedDriver() & GL::Context::DetectedDriver::SwiftShader)
        CORRADE_SKIP("UBOs with dynamically indexed arrays don't seem to work on SwiftShader, can't test.");
    #endif
    CORRADE_COMPARE_WITH(_framebuffer.read({{}, RenderSize}, {PixelFormat::RGBA8Unorm}),
        Utility::Path::join({UI_TEST_DIR, "TextLayerTestFiles", data.filename}),
        DebugTools::CompareImageToFile{_importerManager});
}

constexpr Vector2i DrawSize{64, 64};

void TextLayerGLTest::drawSetup() {
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

void TextLayerGLTest::drawTeardown() {
    _framebuffer = GL::Framebuffer{NoCreate};
    _color = GL::Texture2D{NoCreate};

    GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::disable(GL::Renderer::Feature::ScissorTest);
    GL::Renderer::disable(GL::Renderer::Feature::Blending);
}

void TextLayerGLTest::drawOrder() {
    auto&& data = DrawOrderData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Based on BaseLayerGLTest::drawOrder(), with additional variability due
       to each text having a different size, and editing styles included.

       Right now, if clip rects are not present, all selections and cursors are
       drawn before all the text for every top-level node, so they're
       underneath the green+blue+red but above the large red.

       This may or may not change in the future. With clip rects more
       interleaving happens, tested in drawClipping(). */

    AbstractUserInterface ui{DrawSize};
    ui.setRendererInstance(Containers::pointer<RendererGL>());

    /* A font that just produces glyph ID 0 spaced 16 units apart */
    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return _opened; }
        Properties doOpenFile(Containers::StringView, Float) override {
            _opened = true;
            return {8.0f, 8.0f, -8.0f, 16.0f, 1};
        }
        void doClose() override { _opened = false; }

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override {
            struct Shaper: Text::AbstractShaper {
                using Text::AbstractShaper::AbstractShaper;

                UnsignedInt doShape(Containers::StringView text, UnsignedInt, UnsignedInt, Containers::ArrayView<const Text::FeatureRange>) override {
                    return text.size();
                }
                void doGlyphIdsInto(const Containers::StridedArrayView1D<UnsignedInt>& ids) const override {
                    for(std::size_t i = 0; i != ids.size(); ++i)
                        ids[i] = 0;
                }
                void doGlyphOffsetsAdvancesInto(const Containers::StridedArrayView1D<Vector2>& offsets, const Containers::StridedArrayView1D<Vector2>& advances) const override {
                    for(std::size_t i = 0; i != offsets.size(); ++i) {
                        offsets[i] = {0.0f, -8.0f};
                        advances[i] = {8.0f, 0.0f};
                    }
                }
                void doGlyphClustersInto(const Containers::StridedArrayView1D<UnsignedInt>& clusters) const override {
                    /* Just a trivial 1:1 mapping */
                    for(std::size_t i = 0; i != clusters.size(); ++i)
                        clusters[i] = i;
                }
            };
            return Containers::pointer<Shaper>(*this);
        }

        bool _opened = false;
    } font;
    font.openFile({}, 8.0f);

    /* A full-white glyph cache, containing just one 7x16 glyph. Default
       padding is 1, resetting to 0 to make this work. */
    Text::GlyphCacheGL cache{PixelFormat::R8Unorm, {8, 16}, {}};
    for(auto row: cache.image().pixels<UnsignedByte>()[0])
        for(UnsignedByte& pixel: row)
            pixel = 255;
    cache.flushImage({{}, {8, 16}});
    cache.addGlyph(cache.addFont(1, &font), 0, {}, {{}, {7, 16}});

    TextLayerGL::Shared layerShared{TextLayer::Shared::Configuration{3, 4}
        .setEditingStyleCount(6)
    };
    layerShared.setGlyphCache(cache);

    FontHandle fontHandleLarge = layerShared.addFont(font, 16.0f);
    FontHandle fontHandleSmall = layerShared.addFont(font, 8.0f);
    /* Testing the styleToUniform initializer list overload, others cases use
       implicit mapping initializer list overloads */
    layerShared.setStyle(TextLayerCommonStyleUniform{}, {
        TextLayerStyleUniform{}
            .setColor(0xff0000_rgbf),
        TextLayerStyleUniform{}
            .setColor(0x00ff00_rgbf),
        TextLayerStyleUniform{}
            .setColor(0x0000ff_rgbf)
    }, {
        0, /* 0, red large */
        0, /* 1, red small */
        1, /* 2, green large */
        2, /* 3, blue small */
    }, {
        fontHandleLarge,
        fontHandleSmall,
        fontHandleLarge,
        fontHandleSmall
    }, {
        Text::Alignment::MiddleCenter,
        Text::Alignment::MiddleCenter,
        Text::Alignment::MiddleCenter,
        Text::Alignment::MiddleCenter
    }, {}, {}, {}, {
        data.editable ? 0 : -1,
        data.editable ? 0 : -1,
        data.editable ? 1 : -1,
        data.editable ? 2 : -1,
    }, {
        data.editable ? 3 : -1,
        data.editable ? 3 : -1,
        data.editable ? 4 : -1,
        data.editable ? 5 : -1,
    }, {});
    layerShared.setEditingStyle(TextLayerCommonEditingStyleUniform{}, {
        TextLayerEditingStyleUniform{}
            .setBackgroundColor(0x00cccc99_rgbaf),
        TextLayerEditingStyleUniform{}
            .setBackgroundColor(0xcc00cc99_rgbaf),
        TextLayerEditingStyleUniform{}
            .setBackgroundColor(0xcccc0099_rgbaf),
        TextLayerEditingStyleUniform{}
            .setBackgroundColor(0x00666699_rgbaf),
        TextLayerEditingStyleUniform{}
            .setBackgroundColor(0x66006699_rgbaf),
        TextLayerEditingStyleUniform{}
            .setBackgroundColor(0x66660099_rgbaf)
    }, {}, {
        /* Padding to not have the selection align exactly with the glyphs */
        Vector4{1.0f}, Vector4{1.0f}, Vector4{1.0f},
        Vector4{1.0f}, Vector4{1.0f}, Vector4{1.0f}
    });

    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<TextLayerGL>(layer, layerShared));

    NodeHandle topLevelOnTopGreen = ui.createNode({12.0f, 8.0f}, {32.0f, 32.0f});

    NodeHandle topLevelBelowRed = ui.createNode({28.0f, 24.0f}, {32.0f, 32.0f});
    ui.setNodeOrder(topLevelBelowRed, topLevelOnTopGreen);

    NodeHandle topLevelHiddenBlue = ui.createNode({24.0f, 8.0f}, {32.0f, 32.0f}, NodeFlag::Hidden);

    NodeHandle childBelowBlue = ui.createNode(topLevelOnTopGreen, {13.0f, 4.0f}, {16.0f, 16.0f});
    NodeHandle childAboveRed = ui.createNode(childBelowBlue, {-7.0f, 8.0f}, {16.0f, 16.0f});

    TextDataFlags flags = data.editable ? TextDataFlag::Editable : TextDataFlags{};
    DataHandle texts[5];
    if(data.dataInNodeOrder) {
        texts[0] = ui.layer<TextLayerGL>(layer).create(0, "ab", {}, flags, topLevelBelowRed);
        texts[1] = ui.layer<TextLayerGL>(layer).create(2, "abc", {}, flags, topLevelOnTopGreen);
        texts[2] = ui.layer<TextLayerGL>(layer).create(3, "abcdef", {}, flags, topLevelHiddenBlue);
        texts[3] = ui.layer<TextLayerGL>(layer).create(3, "abcd", {}, flags, childBelowBlue);
        texts[4] = ui.layer<TextLayerGL>(layer).create(1, "abcde", {}, flags, childAboveRed);
    } else {
        texts[0] = ui.layer<TextLayerGL>(layer).create(2, "abc", {}, flags, topLevelOnTopGreen);
        texts[1] = ui.layer<TextLayerGL>(layer).create(3, "abcdef", {}, flags, topLevelHiddenBlue);
        texts[2] = ui.layer<TextLayerGL>(layer).create(0, "ab", {}, flags, topLevelBelowRed);
        texts[3] = ui.layer<TextLayerGL>(layer).create(1, "abcde", {}, flags, childAboveRed);
        texts[4] = ui.layer<TextLayerGL>(layer).create(3, "abcd", {}, flags, childBelowBlue);
    }

    /* Make all fully selected if editable */
    if(data.editable) for(DataHandle text: texts)
        ui.layer<TextLayerGL>(layer).setCursor(text, 0, ui.layer<TextLayerGL>(layer).text(text).size());

    ui.draw();

    MAGNUM_VERIFY_NO_GL_ERROR();

    if(!(_importerManager.load("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_importerManager.load("StbImageImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / StbImageImporter plugins not found.");

    #if defined(MAGNUM_TARGET_GLES) && !defined(MAGNUM_TARGET_WEBGL)
    /* Same problem is with all builtin shaders, so this doesn't seem to be a
       bug in the base layer shader code */
    if(GL::Context::current().detectedDriver() & GL::Context::DetectedDriver::SwiftShader)
        CORRADE_SKIP("UBOs with dynamically indexed arrays don't seem to work on SwiftShader, can't test.");
    #endif
    CORRADE_COMPARE_WITH(_framebuffer.read({{}, DrawSize}, {PixelFormat::RGBA8Unorm}),
        Utility::Path::join(UI_TEST_DIR,
            data.editable ?
                "TextLayerTestFiles/draw-order-editable.png" :
                "TextLayerTestFiles/draw-order.png"),
        DebugTools::CompareImageToFile{_importerManager});
}

void TextLayerGLTest::drawClipping() {
    auto&& data = DrawClippingData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Based on BaseLayerGLTest::drawClipping(), with additional variability
       due to each text having a different size, and editing styles included.

       Right now, for each clip rect, all selection and cursors are drawn
       first, and then all text. This may eventually change to draw selection
       and cursors first for all clip rects, and then all text for all clip
       rects, as that's less shader switching if there's many editable texts
       visible at once. */

    /* X is divided by 10, Y by 100 when rendering. Window size (for events)
       isn't used for anything here. */
    AbstractUserInterface ui{{640.0f, 6400.0f}, {1.0f, 1.0f}, DrawSize};
    ui.setRendererInstance(Containers::pointer<RendererGL>());

    /* A font that just produces glyph ID 0 spaced 16 units apart */
    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return _opened; }
        Properties doOpenFile(Containers::StringView, Float) override {
            _opened = true;
            return {8.0f, 80.0f, -80.0f, 160.0f, 1};
        }
        void doClose() override { _opened = false; }

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override {
            struct Shaper: Text::AbstractShaper {
                using Text::AbstractShaper::AbstractShaper;

                UnsignedInt doShape(Containers::StringView text, UnsignedInt, UnsignedInt, Containers::ArrayView<const Text::FeatureRange>) override {
                    return text.size();
                }
                void doGlyphIdsInto(const Containers::StridedArrayView1D<UnsignedInt>& ids) const override {
                    for(std::size_t i = 0; i != ids.size(); ++i)
                        ids[i] = 0;
                }
                void doGlyphOffsetsAdvancesInto(const Containers::StridedArrayView1D<Vector2>& offsets, const Containers::StridedArrayView1D<Vector2>& advances) const override {
                    for(std::size_t i = 0; i != offsets.size(); ++i) {
                        offsets[i] = {0.0f, -80.0f};
                        advances[i] = {8.0f, 0.0f};
                    }
                }
                void doGlyphClustersInto(const Containers::StridedArrayView1D<UnsignedInt>& clusters) const override {
                    /* Just a trivial 1:1 mapping */
                    for(std::size_t i = 0; i != clusters.size(); ++i)
                        clusters[i] = i;
                }
            };
            return Containers::pointer<Shaper>(*this);
        }

        bool _opened = false;
    } font;
    font.openFile({}, 8.0f);

    /* A full-white glyph cache, containing just one 7x160 glyph. Default
       padding is 1, resetting to 0 to make this work. */
    Text::GlyphCacheGL cache{PixelFormat::R8Unorm, {8, 160}, {}};
    for(auto row: cache.image().pixels<UnsignedByte>()[0])
        for(UnsignedByte& pixel: row)
            pixel = 255;
    cache.flushImage({{}, {8, 160}});
    cache.addGlyph(cache.addFont(1, &font), 0, {}, {{}, {7, 160}});

    TextLayerGL::Shared layerShared{TextLayer::Shared::Configuration{3, 5}
        .setEditingStyleCount(6)
    };
    layerShared.setGlyphCache(cache);

    FontHandle fontHandleLarge = layerShared.addFont(font, 160.0f);
    FontHandle fontHandleSmall = layerShared.addFont(font, 80.0f);
    /* Testing the styleToUniform initializer list overload, others cases use
       implicit mapping initializer list overloads */
    layerShared.setStyle(TextLayerCommonStyleUniform{}, {
        TextLayerStyleUniform{}
            .setColor(0xff0000_rgbf),
        TextLayerStyleUniform{}
            .setColor(0x00ff00_rgbf),
        TextLayerStyleUniform{}
            .setColor(0x0000ff_rgbf)
    }, {
        0, /* 0, red large */
        0, /* 1, red small */
        1, /* 2, green small */
        2, /* 3, blue large */
        2, /* 4, blue small */
    }, {
        fontHandleLarge,
        fontHandleSmall,
        fontHandleSmall,
        fontHandleLarge,
        fontHandleSmall
    }, {
        Text::Alignment::MiddleCenter,
        Text::Alignment::MiddleCenter,
        Text::Alignment::MiddleCenter,
        Text::Alignment::MiddleCenter,
        Text::Alignment::MiddleCenter
    }, {}, {}, {}, {
        data.editable ? 0 : -1,
        data.editable ? 0 : -1,
        data.editable ? 1 : -1,
        data.editable ? 2 : -1,
        data.editable ? 2 : -1,
    }, {
        data.editable ? 3 : -1,
        data.editable ? 3 : -1,
        data.editable ? 4 : -1,
        data.editable ? 5 : -1,
        data.editable ? 5 : -1,
    }, {});
    layerShared.setEditingStyle(TextLayerCommonEditingStyleUniform{}, {
        TextLayerEditingStyleUniform{}
            .setBackgroundColor(0x00cccc99_rgbaf),
        TextLayerEditingStyleUniform{}
            .setBackgroundColor(0xcc00cc99_rgbaf),
        TextLayerEditingStyleUniform{}
            .setBackgroundColor(0xcccc0099_rgbaf),
        TextLayerEditingStyleUniform{}
            .setBackgroundColor(0x00666699_rgbaf),
        TextLayerEditingStyleUniform{}
            .setBackgroundColor(0x66006699_rgbaf),
        TextLayerEditingStyleUniform{}
            .setBackgroundColor(0x66660099_rgbaf)
    }, {}, {
        /* Padding to not have the selection align exactly with the glyphs.
           Has to be scaled 10x on X and 100x on Y to match the UI scaling. */
        {10.0f, 100.0f, 10.0f, 100.0f},
        {10.0f, 100.0f, 10.0f, 100.0f},
        {10.0f, 100.0f, 10.0f, 100.0f},
        {10.0f, 100.0f, 10.0f, 100.0f},
        {10.0f, 100.0f, 10.0f, 100.0f},
        {10.0f, 100.0f, 10.0f, 100.0f},
    });

    TextLayerGL& layer = ui.setLayerInstance(Containers::pointer<TextLayerGL>(ui.createLayer(), layerShared));

    /* Two main clip nodes, each containing subnodes which areas should touch
       each other but not overlap if clipping is enabled. They're either
       top-level nodes with possibly swapped order, in which case they're
       submitted in two separate draws, or they're sub-nodes of a single
       top-level node in which case they're drawn together with two clip rect
       ranges. */
    NodeHandle parent = NodeHandle::Null;
    if(data.singleTopLevel) {
        parent = ui.createNode({}, {});
    }

    TextDataFlags flags = data.editable ? TextDataFlag::Editable : TextDataFlags{};

    NodeHandle leftTop = ui.createNode(parent, {100.0f, 600.0f}, {320.0f, 3200.0f});
    NodeHandle leftTop1 = ui.createNode(leftTop, {0.0f, 0.0f}, {320.0f, 2400.0f});
    NodeHandle leftTop2 = ui.createNode(leftTop, {0.0f, 2400.0f}, {320.0f, 800.0f});
    /* Child of leftTop2, but should only be clipped against leftTop, not
       leftTop2 */
    NodeHandle leftTop21 = ui.createNode(leftTop2, {60.0f, -800.0f}, {80.0f, 2400.0f});
    DataHandle leftTop1Data = layer.create(0, "abc", {}, flags, leftTop1);
    DataHandle leftTop2Data = layer.create(2, "abcdef", {}, flags, leftTop2);
    DataHandle leftTop21Data = layer.create(3, "a", {}, flags, leftTop21);

    NodeHandle rightBottom = ui.createNode(parent, {420.0f, 3600.0f}, {160.0f, 2000.0f});
    NodeHandle rightBottom1 = ui.createNode(rightBottom, {0.0f, 0.0f}, {80.0f, 2000.0f});
    /* Completely outside the rightBottom area, should get culled, i.e. not
       even passed to draw() */
    NodeHandle rightBottom11 = ui.createNode(rightBottom1, {-400.0f, 1400.0f}, {80.0f, 800.0f});
    /* Data added to the clip node should get clipped as well */
    DataHandle rightBottomData = layer.create(4, "abc", {}, flags, rightBottom);
    layer.setPadding(rightBottomData, {20.0f, 1600.0f, 0.0f, 0.0f});
    DataHandle rightBottom1Data = layer.create(1, "abcd", {}, flags, rightBottom1);
    DataHandle rightBottom11Data = layer.create(2, "a", {}, flags, rightBottom11);

    /* Make all fully selected if editable */
    if(data.editable) for(DataHandle text: {
        leftTop1Data,
        leftTop2Data,
        leftTop21Data,
        rightBottomData,
        rightBottom1Data,
        rightBottom11Data
    })
        layer.setCursor(text, 0, layer.text(text).size());

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

    if(!(_importerManager.load("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_importerManager.load("StbImageImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / StbImageImporter plugins not found.");

    #if defined(MAGNUM_TARGET_GLES) && !defined(MAGNUM_TARGET_WEBGL)
    /* Same problem is with all builtin shaders, so this doesn't seem to be a
       bug in the base layer shader code */
    if(GL::Context::current().detectedDriver() & GL::Context::DetectedDriver::SwiftShader)
        CORRADE_SKIP("UBOs with dynamically indexed arrays don't seem to work on SwiftShader, can't test.");
    #endif
    CORRADE_COMPARE_WITH(_framebuffer.read({{}, DrawSize}, {PixelFormat::RGBA8Unorm}),
        Utility::Path::join({UI_TEST_DIR, "TextLayerTestFiles", data.filename}),
        DebugTools::CompareImageToFile{_importerManager});
}

void TextLayerGLTest::eventStyleTransition() {
    auto&& data = EventStyleTransitionData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Switches between the "default" / "default, default selection style" and
       "colored" / "colored, cursor + selection style, colored text" cases from
       render() after a press event, also with getting or losing the editing
       style. Everything else is tested in AbstractVisualLayerTest already. */

    if(!(_fontManager.load("StbTrueTypeFont") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("StbTrueTypeFont plugin not found.");

    AbstractUserInterface ui{RenderSize};
    ui.setRendererInstance(Containers::pointer<RendererGL>());

    /* Opened in the constructor together with cache filling to circumvent
       stb_truetype's extreme rasterization slowness */
    CORRADE_VERIFY(_font && _font->isOpened());

    TextLayerGL::Shared layerShared{TextLayer::Shared::Configuration{5, 4}
        .setEditingStyleCount(data.editableBefore || data.editableAfter ? 3 : 0)
    };
    layerShared.setGlyphCache(_fontGlyphCache);

    FontHandle fontHandle = layerShared.addFont(*_font, 32.0f);
    layerShared
        .setStyle(TextLayerCommonStyleUniform{}, {
            TextLayerStyleUniform{},        /* default */
            TextLayerStyleUniform{},        /* default, default selection style */
            TextLayerStyleUniform{}         /* colored */
                .setColor(0x3bd267_rgbf),
            TextLayerStyleUniform{}         /* colored, cursor + selection style */
                .setColor(0x3bd267_rgbf),
            TextLayerStyleUniform{}         /* Selected text override */
                .setColor(0x1f1f1f_rgbf),
        }, {0, 1, 2, 3}, {fontHandle, fontHandle, fontHandle, fontHandle}, {
            Text::Alignment::MiddleCenter,
            Text::Alignment::MiddleCenter,
            Text::Alignment::MiddleCenter,
            Text::Alignment::MiddleCenter
        }, {}, {}, {},
        {-1,
         data.editableBefore ? 0 : -1,
         -1,
         data.editableAfter ? 1 : -1},
        {-1,
         data.editableBefore ? 0 : -1,
         -1,
         data.editableAfter ? 2 : -1},
        {})
        .setStyleTransition(
            [](UnsignedInt) -> UnsignedInt {
                CORRADE_INTERNAL_ASSERT_UNREACHABLE();
            },
            [](UnsignedInt) -> UnsignedInt {
                CORRADE_INTERNAL_ASSERT_UNREACHABLE();
            },
            data.transition,
            [](UnsignedInt) -> UnsignedInt {
                CORRADE_INTERNAL_ASSERT_UNREACHABLE();
            });
    if(data.editableBefore || data.editableAfter) layerShared.setEditingStyle(
        TextLayerCommonEditingStyleUniform{},
        {TextLayerEditingStyleUniform{},
         TextLayerEditingStyleUniform{}
            .setBackgroundColor(0xcd3431_rgbf),
         TextLayerEditingStyleUniform{}
            .setBackgroundColor(0xc7cf2f_rgbf)},
        {-1, -1, 4},
        {{}, {5.0f, 0.0f, 5.0f, 0.0f}, {}});

    LayerHandle layer = ui.createLayer();
    ui.setLayerInstance(Containers::pointer<TextLayerGL>(layer, layerShared));

    NodeHandle node = ui.createNode({8.0f, 8.0f}, {112.0f, 48.0f});
    /* Using a text that has glyphs both above and below line and doesn't need
       too many glyphs */
    DataHandle nodeData = ui.layer<TextLayerGL>(layer).create(
        data.editableBefore ? 1 : 0,
        "Maggi", {},
        data.editableBefore || data.editableAfter ? TextDataFlag::Editable : TextDataFlags{},
        node);
    if(data.editableBefore || data.editableAfter)
        ui.layer<TextLayerGL>(layer).setCursor(nodeData, 2, 5);

    ui.draw();

    MAGNUM_VERIFY_NO_GL_ERROR();
    Image2D before = _framebuffer.read({{}, RenderSize}, {PixelFormat::RGBA8Unorm});

    PointerEvent event{{}, Pointer::MouseLeft};
    CORRADE_VERIFY(ui.pointerPressEvent({64.0f, 24.0f}, event));
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsDataUpdate);

    /* We have blending enabled, which means a subsequent draw would try to
       blend with the previous, causing unwanted difference */
    _framebuffer.clear(GL::FramebufferClear::Color);

    ui.draw();

    MAGNUM_VERIFY_NO_GL_ERROR();
    Image2D after = _framebuffer.read({{}, RenderSize}, {PixelFormat::RGBA8Unorm});

    if(!(_importerManager.load("AnyImageImporter") & PluginManager::LoadState::Loaded) ||
       !(_importerManager.load("StbImageImporter") & PluginManager::LoadState::Loaded))
        CORRADE_SKIP("AnyImageImporter / StbImageImporter plugins not found.");

    #if defined(MAGNUM_TARGET_GLES) && !defined(MAGNUM_TARGET_WEBGL)
    /* Same problem is with all builtin shaders, so this doesn't seem to be a
       bug in the base layer shader code */
    if(GL::Context::current().detectedDriver() & GL::Context::DetectedDriver::SwiftShader)
        CORRADE_SKIP("UBOs with dynamically indexed arrays don't seem to work on SwiftShader, can't test.");
    #endif
    CORRADE_COMPARE_WITH(before,
        Utility::Path::join(UI_TEST_DIR,
            data.editableBefore ?
                "TextLayerTestFiles/default-selection.png" :
                "TextLayerTestFiles/default.png"),
        DebugTools::CompareImageToFile{_importerManager});
    CORRADE_COMPARE_WITH(after,
        Utility::Path::join(UI_TEST_DIR,
            data.editableAfter ?
                "TextLayerTestFiles/colored-cursor-selection-text.png" :
                "TextLayerTestFiles/colored.png"),
        DebugTools::CompareImageToFile{_importerManager});
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::TextLayerGLTest)
