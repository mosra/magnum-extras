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

#include <Corrade/PluginManager/Manager.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/TextureArray.h>
#include <Magnum/Text/AbstractFont.h>
#include <Magnum/Text/DistanceFieldGlyphCacheGL.h>
#include <Magnum/Trade/AbstractImporter.h>

#include "Magnum/Ui/BaseLayerGL.h"
#include "Magnum/Ui/BaseLayerAnimator.h"
#include "Magnum/Ui/EventLayer.h"
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/LineLayerGL.h"
#include "Magnum/Ui/RendererGL.h"
#include "Magnum/Ui/Style.h"
#include "Magnum/Ui/TextLayerGL.h"
#include "Magnum/Ui/TextLayerAnimator.h"
#include "Magnum/Ui/TextProperties.h"
#include "Magnum/Ui/UserInterfaceGL.h"

#define DOXYGEN_ELLIPSIS(...) __VA_ARGS__
#define DOXYGEN_IGNORE(...) __VA_ARGS__

using namespace Magnum;
using namespace Math::Literals;

/* Make sure the name doesn't conflict with any other snippets to avoid linker
   warnings, unlike with `int main()` there now has to be a declaration to
   avoid -Wmisssing-prototypes */
void mainUiGL();
void mainUiGL() {
{
/* Used by both AbstractUserInterface and UserInterfaceGL docs */
/* [UserInterfaceGL-setup] */
Ui::UserInterfaceGL ui{{800, 600}, Ui::McssDarkStyle{}};
/* [UserInterfaceGL-setup] */

/* [AbstractUserInterface-setup-blend] */
GL::Renderer::setBlendFunction(
    GL::Renderer::BlendFunction::One,
    GL::Renderer::BlendFunction::OneMinusSourceAlpha);
/* [AbstractUserInterface-setup-blend] */

/* [AbstractUserInterface-setup-draw] */
GL::defaultFramebuffer.clear(GL::FramebufferClear::Color);

DOXYGEN_ELLIPSIS()

ui.draw();
/* [AbstractUserInterface-setup-draw] */

/* [AbstractUserInterface-setup-draw-ondemand] */
if(ui.state()) {
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color);

    ui.draw();
}
/* [AbstractUserInterface-setup-draw-ondemand] */
}

{
struct: Ui::AbstractStyle {
    Ui::StyleFeatures doFeatures() const override { return {}; }
    bool doApply(Ui::UserInterface&, Ui::StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override {
        return false;
    }
} myCustomStyle;
/* [UserInterfaceGL-setup-features] */
/* Pick everything except icons from the builtin style */
Ui::UserInterfaceGL ui{{800, 600}, Ui::McssDarkStyle{},
    ~Ui::StyleFeature::TextLayerImages};

DOXYGEN_ELLIPSIS()

/* Use icons from a custom style instead */
ui.setStyle(myCustomStyle, Ui::StyleFeature::TextLayerImages);
/* [UserInterfaceGL-setup-features] */
}

{
/* [UserInterfaceGL-setup-managers] */
PluginManager::Manager<Trade::AbstractImporter> importerManager;
PluginManager::Manager<Text::AbstractFont> fontManager;
DOXYGEN_ELLIPSIS()

Ui::UserInterfaceGL ui{DOXYGEN_ELLIPSIS({}, Ui::McssDarkStyle{}), &importerManager, &fontManager};
/* [UserInterfaceGL-setup-managers] */
}

{
/* [UserInterfaceGL-setup-renderer] */
Ui::UserInterfaceGL ui{NoCreate};

ui
    .setRendererInstance(Containers::pointer<Ui::RendererGL>(DOXYGEN_ELLIPSIS()))
    .setSize(DOXYGEN_ELLIPSIS({}))
    .setStyle(Ui::McssDarkStyle{});
/* [UserInterfaceGL-setup-renderer] */
}

{
Ui::BaseLayerGL::Shared shared{Ui::BaseLayerGL::Shared::Configuration{1}};
/* [UserInterfaceGL-setup-layer] */
Ui::UserInterfaceGL ui{DOXYGEN_ELLIPSIS({}), Ui::McssDarkStyle{},
    ~Ui::StyleFeature::BaseLayer};

DOXYGEN_ELLIPSIS()

ui.setBaseLayerInstance(Containers::pointer<Ui::BaseLayerGL>(DOXYGEN_ELLIPSIS(ui.createLayer(), shared)));
/* [UserInterfaceGL-setup-layer] */
}

{
/* [BaseLayer-setup-shared] */
Ui::BaseLayerGL::Shared baseLayerShared{
    Ui::BaseLayer::Shared::Configuration{3}
};
/* [BaseLayer-setup-shared] */

Ui::AbstractUserInterface ui{{100, 100}};
/* [BaseLayer-setup] */
Ui::BaseLayer& baseLayer = ui.setLayerInstance(
    Containers::pointer<Ui::BaseLayerGL>(ui.createLayer(), baseLayerShared));
/* [BaseLayer-setup] */
static_cast<void>(baseLayer);
}

{
Ui::UserInterfaceGL ui{NoCreate};
Ui::BaseLayerGL::Shared baseLayerShared{Ui::BaseLayer::Shared::Configuration{1}};
/* [BaseLayer-setup-implicit] */
ui.setBaseLayerInstance(
    Containers::pointer<Ui::BaseLayerGL>(ui.createLayer(), baseLayerShared));
/* [BaseLayer-setup-implicit] */
}

{
Ui::UserInterfaceGL ui{NoCreate};
/* [BaseLayer-style-textured1] */
Ui::BaseLayerGL::Shared texturedLayerShared{
    Ui::BaseLayerGL::Shared::Configuration{DOXYGEN_ELLIPSIS(1)}
        .addFlags(Ui::BaseLayerSharedFlag::Textured)
};
texturedLayerShared.setStyle(DOXYGEN_ELLIPSIS(Ui::BaseLayerCommonStyleUniform{}), {
    Ui::BaseLayerStyleUniform{}, /* 0 */
    Ui::BaseLayerStyleUniform{}  /* 1 */
        .setOutlineWidth(2.0f)
        .setOutlineColor(0xdcdcdcff_rgbaf*0.25f),
    Ui::BaseLayerStyleUniform{}  /* 2 */
        .setCornerRadius(12.0f)
}, {});

Ui::BaseLayerGL& texturedLayer = ui.setLayerInstance(
    Containers::pointer<Ui::BaseLayerGL>(ui.createLayer(), texturedLayerShared));
GL::Texture2DArray texture;
DOXYGEN_ELLIPSIS()
texturedLayer.setTexture(texture);
/* [BaseLayer-style-textured1] */

/* [BaseLayer-style-textured2] */
Ui::NodeHandle image = DOXYGEN_ELLIPSIS({});
Ui::NodeHandle outlined = DOXYGEN_ELLIPSIS({});
Ui::NodeHandle avatar = DOXYGEN_ELLIPSIS({});
texturedLayer.create(0, image);
texturedLayer.create(1, outlined);
Ui::DataHandle avatarData = texturedLayer.create(2, avatar);
texturedLayer.setTextureCoordinates(avatarData, {0.4f, 0.0f, 0.0f}, {0.25f, 0.5f});
/* [BaseLayer-style-textured2] */
}

{
Ui::UserInterfaceGL ui{NoCreate};
/* [BaseLayer-dynamic-styles] */
Ui::BaseLayerGL::Shared baseLayerShared{
    Ui::BaseLayerGL::Shared::Configuration{DOXYGEN_ELLIPSIS(1)}
        .setDynamicStyleCount(10)
};
Ui::BaseLayerGL& baseLayer = ui.setLayerInstance(
    Containers::pointer<Ui::BaseLayerGL>(ui.createLayer(), baseLayerShared));

DOXYGEN_ELLIPSIS()

UnsignedInt dynamicStyleId = DOXYGEN_ELLIPSIS(0); /* anything less than the dynamic style count */
baseLayer.setDynamicStyle(dynamicStyleId, DOXYGEN_ELLIPSIS(Ui::BaseLayerStyleUniform{}, {}));

Ui::NodeHandle node = DOXYGEN_ELLIPSIS({});
baseLayer.create(baseLayer.shared().styleCount() + dynamicStyleId, node);
/* [BaseLayer-dynamic-styles] */
}

{
Ui::UserInterfaceGL ui{NoCreate};
/* [BaseLayer-style-background-blur] */
ui.setRendererInstance(Containers::pointer<Ui::RendererGL>(
    Ui::RendererGL::Flag::CompositingFramebuffer));

Ui::BaseLayerGL::Shared blurLayerShared{
    Ui::BaseLayerGL::Shared::Configuration{DOXYGEN_ELLIPSIS(1)}
        .addFlags(Ui::BaseLayerSharedFlag::BackgroundBlur)
        .setBackgroundBlurRadius(DOXYGEN_ELLIPSIS(4))
};
blurLayerShared.setStyle(DOXYGEN_ELLIPSIS(Ui::BaseLayerCommonStyleUniform{}), {
    Ui::BaseLayerStyleUniform{}  /* 0 */
        .setCornerRadius(12.0f)
        .setColor(0xffffffff_rgbaf*0.667f)
}, {});
Ui::BaseLayer& blurLayer = ui.setLayerInstance(
    Containers::pointer<Ui::BaseLayerGL>(ui.createLayer(), blurLayerShared));

DOXYGEN_ELLIPSIS()

Ui::NodeHandle background = DOXYGEN_ELLIPSIS({});
blurLayer.create(0, background);
/* [BaseLayer-style-background-blur] */
}

{
Ui::AbstractUserInterface ui{{100, 100}};
Ui::BaseLayerStyleAnimator animator{Ui::animatorHandle(0, 1)};
/* [BaseLayerStyleAnimator-setup2] */
Ui::BaseLayerGL::Shared baseLayerShared{
    Ui::BaseLayer::Shared::Configuration{DOXYGEN_ELLIPSIS(0)}
        DOXYGEN_ELLIPSIS()
        .setDynamicStyleCount(10) /* adjust as needed */
};
Ui::BaseLayer& baseLayer = ui.setLayerInstance(
    Containers::pointer<Ui::BaseLayerGL>(ui.createLayer(), baseLayerShared));

DOXYGEN_ELLIPSIS()

baseLayer.assignAnimator(animator);
/* [BaseLayerStyleAnimator-setup2] */
}

{
/* [LineLayer-setup-shared] */
Ui::LineLayerGL::Shared lineLayerShared{
    Ui::LineLayer::Shared::Configuration{3}
};
/* [LineLayer-setup-shared] */

Ui::AbstractUserInterface ui{{100, 100}};
/* [LineLayer-setup] */
Ui::LineLayer& lineLayer = ui.setLayerInstance(
    Containers::pointer<Ui::LineLayerGL>(ui.createLayer(), lineLayerShared));
/* [LineLayer-setup] */
static_cast<void>(lineLayer);
}

{
Ui::AbstractUserInterface ui{{100, 100}};
/* [LineLayer-style-cap-join] */
Ui::LineLayerGL::Shared lineLayerSharedRound{
    Ui::LineLayer::Shared::Configuration{DOXYGEN_ELLIPSIS(1)}
        .setCapStyle(Ui::LineCapStyle::Round)
};
Ui::LineLayerGL::Shared lineLayerSharedSquare{
    Ui::LineLayer::Shared::Configuration{DOXYGEN_ELLIPSIS(1)}
        .setCapStyle(Ui::LineCapStyle::Square)
};

Ui::LineLayer& lineLayerRound = ui.setLayerInstance(
    Containers::pointer<Ui::LineLayerGL>(ui.createLayer(), lineLayerSharedRound));
Ui::LineLayer& lineLayerSquare = ui.setLayerInstance(
    Containers::pointer<Ui::LineLayerGL>(ui.createLayer(), lineLayerSharedSquare));
/* [LineLayer-style-cap-join] */
static_cast<void>(lineLayerRound);
static_cast<void>(lineLayerSquare);
}

{
/* [TextLayer-setup-glyph-cache] */
Text::GlyphCacheArrayGL glyphCache{PixelFormat::R8Unorm, {256, 256, 4}};
/* [TextLayer-setup-glyph-cache] */

/* [TextLayer-setup-shared] */
Ui::TextLayerGL::Shared textLayerShared{glyphCache,
    Ui::TextLayer::Shared::Configuration{3}
};
/* [TextLayer-setup-shared] */

Ui::AbstractUserInterface ui{{100, 100}};
/* [TextLayer-setup] */
Ui::TextLayer& textLayer = ui.setLayerInstance(
    Containers::pointer<Ui::TextLayerGL>(ui.createLayer(), textLayerShared));
/* [TextLayer-setup] */
static_cast<void>(textLayer);
}

{
Ui::UserInterfaceGL ui{NoCreate};
Text::GlyphCacheArrayGL glyphCache{PixelFormat::R8Unorm, {8, 8, 1}};
Ui::TextLayerGL::Shared textLayerShared{glyphCache, Ui::TextLayer::Shared::Configuration{1}};
/* [TextLayer-setup-implicit] */
ui.setTextLayerInstance(
    Containers::pointer<Ui::TextLayerGL>(ui.createLayer(), textLayerShared));
/* [TextLayer-setup-implicit] */
}

{
/* [TextLayer-distancefield-setup] */
Text::DistanceFieldGlyphCacheArrayGL glyphCache{{1024, 1024, 4}, {256, 256}, 20};

Ui::TextLayerGL::Shared textLayerShared{glyphCache,
    Ui::TextLayer::Shared::Configuration{3}
};
/* [TextLayer-distancefield-setup] */
}

{
Ui::UserInterfaceGL ui{NoCreate};
Text::DistanceFieldGlyphCacheArrayGL glyphCache{DOXYGEN_ELLIPSIS({}, {}, 0)};
Ui::TextLayerGL::Shared textLayerShared{glyphCache,
    Ui::TextLayer::Shared::Configuration{DOXYGEN_ELLIPSIS(0)}
};
/* [TextLayer-transformation-setup] */
Ui::TextLayerGL& textLayer = ui.setLayerInstance(
    Containers::pointer<Ui::TextLayerGL>(ui.createLayer(), textLayerShared,
                                         Ui::TextLayerFlag::Transformable));
/* [TextLayer-transformation-setup] */
static_cast<void>(textLayer);
}

{
Ui::UserInterfaceGL ui{NoCreate};
Text::GlyphCacheArrayGL glyphCache{PixelFormat::R8Unorm, {8, 8, 1}};
/* [TextLayer-dynamic-styles] */
Ui::TextLayerGL::Shared textLayerShared{glyphCache,
    Ui::TextLayerGL::Shared::Configuration{DOXYGEN_ELLIPSIS(1)}
        .setDynamicStyleCount(10)
};
Ui::TextLayerGL& textLayer = ui.setLayerInstance(
    Containers::pointer<Ui::TextLayerGL>(ui.createLayer(), textLayerShared));

DOXYGEN_ELLIPSIS()

UnsignedInt dynamicStyleId = DOXYGEN_ELLIPSIS(0); /* anything less than the dynamic style count */
textLayer.setDynamicStyle(dynamicStyleId, DOXYGEN_ELLIPSIS(Ui::TextLayerStyleUniform{}, {}, {}, {}, {}));

Ui::NodeHandle node = DOXYGEN_ELLIPSIS({});
textLayer.create(textLayer.shared().styleCount() + dynamicStyleId,
    DOXYGEN_ELLIPSIS("", {}), node);
/* [TextLayer-dynamic-styles] */
}

{
Text::GlyphCacheArrayGL glyphCache{PixelFormat::R8Unorm, {8, 8, 1}};
/* [TextLayer-editing-style-shared] */
Ui::TextLayerGL::Shared textLayerShared{glyphCache,
    Ui::TextLayer::Shared::Configuration{3}
        .setEditingStyleCount(2)
};
/* [TextLayer-editing-style-shared] */
}

{
Ui::AbstractUserInterface ui{{100, 100}};
Text::GlyphCacheArrayGL glyphCache{PixelFormat::R8Unorm, {8, 8, 1}};
Ui::TextLayerStyleAnimator animator{Ui::animatorHandle(0, 1)};
/* [TextLayerStyleAnimator-setup2] */
Ui::TextLayerGL::Shared textLayerShared{glyphCache,
    Ui::TextLayer::Shared::Configuration{DOXYGEN_ELLIPSIS(0)}
        DOXYGEN_ELLIPSIS()
        .setDynamicStyleCount(10) /* adjust as needed */
};
Ui::TextLayer& textLayer = ui.setLayerInstance(
    Containers::pointer<Ui::TextLayerGL>(ui.createLayer(), textLayerShared));

DOXYGEN_ELLIPSIS()

textLayer.assignAnimator(animator);
/* [TextLayerStyleAnimator-setup2] */
}

{
Ui::UserInterfaceGL ui{NoCreate};
/* [EventLayer-setup-implicit] */
ui.setEventLayerInstance(Containers::pointer<Ui::EventLayer>(ui.createLayer()));
/* [EventLayer-setup-implicit] */
}

{
Ui::AbstractUserInterface ui{{100, 100}};
/* [RendererGL-setup] */
ui.setRendererInstance(Containers::pointer<Ui::RendererGL>());
/* [RendererGL-setup] */
}

}
