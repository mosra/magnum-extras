/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023, 2024, 2025, 2026
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

#include <Corrade/Containers/String.h>
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/String.h>
#include <Corrade/PluginManager/Manager.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/GL/OpenGLTester.h>
#include <Magnum/Text/AbstractFont.h>
#include <Magnum/Text/GlyphCacheGL.h>
#include <Magnum/Trade/AbstractImporter.h>

#include "Magnum/Ui/AbstractTheme.h"
#include "Magnum/Ui/BaseLayerGL.h"
#include "Magnum/Ui/BaseLayerAnimator.h"
#include "Magnum/Ui/DataLayer.h"
#include "Magnum/Ui/EventLayer.h"
#include "Magnum/Ui/GenericLayouter.h"
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/LayoutLayer.h"
#include "Magnum/Ui/NodeAnimator.h"
#include "Magnum/Ui/RendererGL.h"
#include "Magnum/Ui/SnapLayouter.h"
#include "Magnum/Ui/TextLayerGL.h"
#include "Magnum/Ui/TextLayerAnimator.h"
#include "Magnum/Ui/UserInterfaceGL.h"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct UserInterfaceGLTest: GL::OpenGLTester {
    explicit UserInterfaceGLTest();

    void construct();
    /* NoCreate tested in UserInterfaceGL_Test to verify it works without a GL
       context */
    void constructSingleSize();
    void constructCopy();
    void constructMove();

    void create();
    void createSingleSize();
    void createAlreadyCreated();
    void createFailed();

    void setTheme();
    void setThemeRendererAlreadyPresent();
    void setThemeRendererNotCompositing();
    void setThemeNoFeatures();
    void setThemeFeaturesNotSupported();
    void setThemeNoSizeSet();
    void setThemeDataLayerAlreadyPresent();
    void setThemeBackgroundLayerAlreadyPresent();
    void setThemeBackgroundLayerStyleAnimatorAlreadyPresent();
    void setThemeBackgroundLayerStyleAnimationsBackgroundLayerNotPresentNotApplied();
    void setThemeBackgroundLayerStyleAnimationsBackgroundLayerNoDynamicStyles();
    void setThemeBaseLayerAlreadyPresent();
    void setThemeBaseLayerStyleAnimatorAlreadyPresent();
    void setThemeBaseLayerStyleAnimationsBaseLayerNotPresentNotApplied();
    void setThemeBaseLayerStyleAnimationsBaseLayerNoDynamicStyles();
    void setThemeTextLayerAlreadyPresent();
    void setThemeTextLayerImagesTextLayerNotPresentNotApplied();
    void setThemeTextLayerStyleAnimatorAlreadyPresent();
    void setThemeTextLayerStyleAnimationsTextLayerNotPresentNotApplied();
    void setThemeTextLayerStyleAnimationsTextLayerNoDynamicStyles();
    void setThemeEventLayerAlreadyPresent();
    void setThemeLayoutLayerAlreadyPresent();
    void setThemeSnapLayouterAlreadyPresent();
    void setThemeGenericLayouterAlreadyPresent();
    void setThemeNodeAnimatorAlreadyPresent();

    private:
        PluginManager::Manager<Trade::AbstractImporter> _importerManager;
        PluginManager::Manager<Text::AbstractFont> _fontManager;
};

const struct {
    const char* name;
    Containers::Optional<ThemeFeatures> themeFeatures;
    UnsignedInt expectedLayerCount;
    ThemeFeatures expectedThemeFeatures;
} ConstructData[]{
    /* Only basic theme features tested here for simplicity, layouters and
       animators are tested thoroughly in setTheme*() */
    {"",
        {}, 3,
        ThemeFeature::BaseLayer|ThemeFeature::TextLayer|ThemeFeature::EventLayer|ThemeFeature(0x8000)},
    {"theme features",
        ThemeFeature::BaseLayer|ThemeFeature::EventLayer, 2,
        ThemeFeature::BaseLayer|ThemeFeature::EventLayer},
    {"theme features, nothing",
        ThemeFeatures{0x8000}, 0,
        ThemeFeatures{0x8000}},
};

const struct {
    const char* name;
    bool tryCreate;
    Containers::Optional<ThemeFeatures> themeFeatures;
    UnsignedInt expectedLayerCount;
    ThemeFeatures expectedThemeFeatures;
} CreateData[]{
    /* Only basic theme features tested here for simplicity, layouters and
       animators are tested thoroughly in setTheme*() */
    {"",
        false, {}, 3,
        ThemeFeature::BaseLayer|ThemeFeature::TextLayer|ThemeFeature::EventLayer|ThemeFeature(0x8000)},
    {"theme features",
        false, ThemeFeature::BaseLayer|ThemeFeature::EventLayer, 2,
        ThemeFeature::BaseLayer|ThemeFeature::EventLayer},
    {"theme features, nothing",
        false, ThemeFeatures{0x8000}, 0,
        ThemeFeatures{0x8000}},
    {"try",
        true, {}, 3,
        ThemeFeature::BaseLayer|ThemeFeature::TextLayer|ThemeFeature::EventLayer|ThemeFeature(0x8000)},
    {"try, theme features",
        true, ThemeFeature::BaseLayer|ThemeFeature::EventLayer, 2,
        ThemeFeature::BaseLayer|ThemeFeature::EventLayer},
    {"try, theme features, nothing",
        false, ThemeFeatures{0x8000}, 0,
        ThemeFeatures{0x8000}},
};

const struct {
    const char* name;
    bool tryCreate, hasRenderer;
    ThemeFeatures features;
} CreateAlreadyCreatedData[]{
    {"data layer present", false, false,
        ThemeFeature::DataLayer},
    {"background layer present", false, false,
        ThemeFeature::BackgroundLayer},
    {"base layer present", false, false,
        ThemeFeature::BaseLayer},
    /* The assertion is printed by tryCreate() so it doesn't need to be tested
       in all combinations */
    {"base layer present, try create", true, false,
        ThemeFeature::BaseLayer},
    /* BaseLayerAnimations not tested, as they depend on BaseLayer being
       present already, which is checked by the above */
    {"text layer present", false, false,
        ThemeFeature::TextLayer},
    /* TextLayerAnimations not tested, as they depend on TextLayer being
       present already, which is checked by the above */
    {"event layer present", false, false,
        ThemeFeature::EventLayer},
    {"layout layer present", false, false,
        ThemeFeature::LayoutLayer},
    {"snap layouter present", false, false,
        ThemeFeature::SnapLayouter},
    {"generic layouter present", false, false,
        ThemeFeature::GenericLayouter},
    {"node animator present", false, false,
        ThemeFeature::NodeAnimations},
    {"renderer present", false, true,
        ThemeFeatures{}},
    {"all layers + layouters + animators + renderer present", false, true,
        ThemeFeature::DataLayer|
        ThemeFeature::BackgroundLayer|
        ThemeFeature::BaseLayer|
        ThemeFeature::TextLayer|
        ThemeFeature::EventLayer|
        ThemeFeature::LayoutLayer|
        ThemeFeature::SnapLayouter|
        ThemeFeature::GenericLayouter|
        ThemeFeature::NodeAnimations},
};

const struct {
    TestSuite::TestCaseDescriptionSourceLocation name;
    ThemeFeatures expectedFeatures;
    ThemeFeatures supportedFeatures;
    bool succeed, explicitCompositingFramebuffer;
    UnsignedInt expectedLayerCount, expectedLayouterCount, expectedAnimatorCount;
    Containers::Array<ThemeFeatures> features;
    bool explicitImporterManager, explicitFontManager;
} SetThemeData[]{
    {"data layer only", ThemeFeature::DataLayer, ThemeFeature::DataLayer, true, false, 1, 0, 0, {InPlaceInit, {
        ThemeFeature::DataLayer
    }}, false, false},
    {"data layer only, everything supported", ThemeFeature::DataLayer, ~ThemeFeatures{}, true, false, 1, 0, 0, {InPlaceInit, {
        ThemeFeature::DataLayer
    }}, false, false},

    {"background layer only", ThemeFeature::BackgroundLayer, ThemeFeature::BackgroundLayer, true, false, 1, 0, 0, {InPlaceInit, {
        ThemeFeature::BackgroundLayer
    }}, false, false},
    {"background layer only, everything supported", ThemeFeature::BackgroundLayer, ~ThemeFeatures{}, true, false, 1, 0, 0, {InPlaceInit, {
        ThemeFeature::BackgroundLayer
    }}, false, false},
    {"background layer + animations only", ThemeFeature::BackgroundLayer|ThemeFeature::BackgroundLayerAnimations, ThemeFeature::BackgroundLayer|ThemeFeature::BackgroundLayerAnimations, true, false, 1, 0, 1, {InPlaceInit, {
        ThemeFeature::BackgroundLayer|ThemeFeature::BackgroundLayerAnimations
    }}, false, false},
    {"background layer + animations, applied gradually", ThemeFeature::BackgroundLayer|ThemeFeature::BackgroundLayerAnimations, ThemeFeature::BackgroundLayer|ThemeFeature::BackgroundLayerAnimations, true, false, 1, 0, 1, {InPlaceInit, {
        ThemeFeature::BackgroundLayer,
        ThemeFeature::BackgroundLayerAnimations
    }}, false, false},
    {"background layer + animations only, everything supported", ThemeFeature::BackgroundLayer|ThemeFeature::BackgroundLayerAnimations, ~ThemeFeatures{}, true, false, 1, 0, 1, {InPlaceInit, {
        ThemeFeature::BackgroundLayer|ThemeFeature::BackgroundLayerAnimations
    }}, false, false},

    {"base layer only", ThemeFeature::BaseLayer, ThemeFeature::BaseLayer, true, false, 1, 0, 0, {InPlaceInit, {
        ThemeFeature::BaseLayer
    }}, false, false},
    {"base layer only, everything supported", ThemeFeature::BaseLayer, ~ThemeFeatures{}, true, false, 1, 0, 0, {InPlaceInit, {
        ThemeFeature::BaseLayer
    }}, false, false},
    {"base layer + animations only", ThemeFeature::BaseLayer|ThemeFeature::BaseLayerAnimations, ThemeFeature::BaseLayer|ThemeFeature::BaseLayerAnimations, true, false, 1, 0, 1, {InPlaceInit, {
        ThemeFeature::BaseLayer|ThemeFeature::BaseLayerAnimations
    }}, false, false},
    {"base layer + animations, applied gradually", ThemeFeature::BaseLayer|ThemeFeature::BaseLayerAnimations, ThemeFeature::BaseLayer|ThemeFeature::BaseLayerAnimations, true, false, 1, 0, 1, {InPlaceInit, {
        ThemeFeature::BaseLayer,
        ThemeFeature::BaseLayerAnimations
    }}, false, false},
    {"base layer + animations only, everything supported", ThemeFeature::BaseLayer|ThemeFeature::BaseLayerAnimations, ~ThemeFeatures{}, true, false, 1, 0, 1, {InPlaceInit, {
        ThemeFeature::BaseLayer|ThemeFeature::BaseLayerAnimations
    }}, false, false},

    /* To verify that the background layer (animations) isn't treated as
       already set when base layer (animations) are present, and that setting
       base layer after doesn't overwrite the background layer. */
    {"background layer applied after base layer is set", ThemeFeature::BackgroundLayer|ThemeFeature::BaseLayer, ThemeFeature::BackgroundLayer|ThemeFeature::BaseLayer, true, true, 2, 0, 0, {InPlaceInit, {
        ThemeFeature::BaseLayer,
        /* Because this is applied later, an explicit compositing framebuffer
           has to be supplied */
        ThemeFeature::BackgroundLayer,
    }}, false, false},
    {"base layer applied after background layer is set", ThemeFeature::BackgroundLayer|ThemeFeature::BaseLayer, ThemeFeature::BackgroundLayer|ThemeFeature::BaseLayer, true, false, 2, 0, 0, {InPlaceInit, {
        ThemeFeature::BackgroundLayer,
        ThemeFeature::BaseLayer,
    }}, false, false},
    {"background layer + animations applied after base layer + animations is set", ThemeFeature::BackgroundLayer|ThemeFeature::BackgroundLayerAnimations|ThemeFeature::BaseLayer|ThemeFeature::BaseLayerAnimations, ThemeFeature::BackgroundLayer|ThemeFeature::BackgroundLayerAnimations|ThemeFeature::BaseLayer|ThemeFeature::BaseLayerAnimations, true, true, 2, 0, 2, {InPlaceInit, {
        ThemeFeature::BaseLayer,
        ThemeFeature::BaseLayerAnimations,
        /* Because this is applied later, an explicit compositing framebuffer
           has to be supplied */
        ThemeFeature::BackgroundLayer,
        ThemeFeature::BackgroundLayerAnimations,
    }}, false, false},
    {"background layer + animations applied after base layer + animations is set, different order", ThemeFeature::BackgroundLayer|ThemeFeature::BackgroundLayerAnimations|ThemeFeature::BaseLayer|ThemeFeature::BaseLayerAnimations, ThemeFeature::BackgroundLayer|ThemeFeature::BackgroundLayerAnimations|ThemeFeature::BaseLayer|ThemeFeature::BaseLayerAnimations, true, true, 2, 0, 2, {InPlaceInit, {
        ThemeFeature::BaseLayer,
        /* Because this is applied later, an explicit compositing framebuffer
           has to be supplied */
        ThemeFeature::BackgroundLayer,
        ThemeFeature::BaseLayerAnimations,
        ThemeFeature::BackgroundLayerAnimations,
    }}, false, false},
    {"base layer + animations applied after background layer + animations is set", ThemeFeature::BackgroundLayer|ThemeFeature::BackgroundLayerAnimations|ThemeFeature::BaseLayer|ThemeFeature::BaseLayerAnimations, ThemeFeature::BackgroundLayer|ThemeFeature::BackgroundLayerAnimations|ThemeFeature::BaseLayer|ThemeFeature::BaseLayerAnimations, true, false, 2, 0, 2, {InPlaceInit, {
        ThemeFeature::BackgroundLayer,
        ThemeFeature::BackgroundLayerAnimations,
        ThemeFeature::BaseLayer,
        ThemeFeature::BaseLayerAnimations,
    }}, false, false},
    {"base layer + animations applied after background layer + animations is set, different order", ThemeFeature::BackgroundLayer|ThemeFeature::BackgroundLayerAnimations|ThemeFeature::BaseLayer|ThemeFeature::BaseLayerAnimations, ThemeFeature::BackgroundLayer|ThemeFeature::BackgroundLayerAnimations|ThemeFeature::BaseLayer|ThemeFeature::BaseLayerAnimations, true, false, 2, 0, 2, {InPlaceInit, {
        ThemeFeature::BackgroundLayer,
        ThemeFeature::BaseLayer,
        ThemeFeature::BackgroundLayerAnimations,
        ThemeFeature::BaseLayerAnimations,
    }}, false, false},
    {"background layer with no animations and base layer + animations", ThemeFeature::BackgroundLayer|ThemeFeature::BaseLayer|ThemeFeature::BaseLayerAnimations, ThemeFeature::BackgroundLayer|ThemeFeature::BaseLayer|ThemeFeature::BaseLayerAnimations, true, false, 2, 0, 1, {InPlaceInit, {
        ThemeFeature::BackgroundLayer,
        ThemeFeature::BaseLayer,
        ThemeFeature::BaseLayerAnimations,
    }}, false, false},

    {"text layer only", ThemeFeature::TextLayer, ThemeFeature::TextLayer, true, false, 1, 0, 0, {InPlaceInit, {
        ThemeFeature::TextLayer
    }}, false, true},
    {"text layer only, everything supported", ThemeFeature::TextLayer, ~ThemeFeatures{}, true, false, 1, 0, 0, {InPlaceInit, {
        ThemeFeature::TextLayer
    }}, false, true},
    {"text layer + images only", ThemeFeature::TextLayer|ThemeFeature::TextLayerImages, ThemeFeature::TextLayer|ThemeFeature::TextLayerImages, true, false, 1, 0, 0, {InPlaceInit, {
        ThemeFeature::TextLayer|ThemeFeature::TextLayerImages
    }}, true, true},
    {"text layer + images, applied gradually", ThemeFeature::TextLayer|ThemeFeature::TextLayerImages, ThemeFeature::TextLayer|ThemeFeature::TextLayerImages, true, false, 1, 0, 0, {InPlaceInit, {
        ThemeFeature::TextLayer,
        ThemeFeature::TextLayerImages
    }}, true, true},
    {"text layer + images only, everything supported", ThemeFeature::TextLayer|ThemeFeature::TextLayerImages, ~ThemeFeatures{}, true, false, 1, 0, 0, {InPlaceInit, {
        ThemeFeature::TextLayer|ThemeFeature::TextLayerImages
    }}, true, true},
    {"text layer + animations only", ThemeFeature::TextLayer|ThemeFeature::TextLayerAnimations, ThemeFeature::TextLayer|ThemeFeature::TextLayerAnimations, true, false, 1, 0, 1, {InPlaceInit, {
        ThemeFeature::TextLayer|ThemeFeature::TextLayerAnimations
    }}, false, true},
    {"text layer + animations, applied gradually", ThemeFeature::TextLayer|ThemeFeature::TextLayerAnimations, ThemeFeature::TextLayer|ThemeFeature::TextLayerAnimations, true, false, 1, 0, 1, {InPlaceInit, {
        ThemeFeature::TextLayer,
        ThemeFeature::TextLayerAnimations
    }}, false, true},
    {"text layer + animations only, everything supported", ThemeFeature::TextLayer|ThemeFeature::TextLayerAnimations, ~ThemeFeatures{}, true, false, 1, 0, 1, {InPlaceInit, {
        ThemeFeature::TextLayer|ThemeFeature::TextLayerAnimations
    }}, false, true},

    {"event layer only", ThemeFeature::EventLayer, ThemeFeature::EventLayer, true, false, 1, 0, 0, {InPlaceInit, {
        ThemeFeature::EventLayer
    }}, false, false},
    {"event layer only, everything supported", ThemeFeature::EventLayer, ~ThemeFeatures{}, true, false, 1, 0, 0, {InPlaceInit, {
        ThemeFeature::EventLayer
    }}, false, false},

    {"layout layer only", ThemeFeature::LayoutLayer, ThemeFeature::LayoutLayer, true, false, 1, 0, 0, {InPlaceInit, {
        ThemeFeature::LayoutLayer
    }}, false, false},
    {"layout layer only, everything supported", ThemeFeature::LayoutLayer, ~ThemeFeatures{}, true, false, 1, 0, 0, {InPlaceInit, {
        ThemeFeature::LayoutLayer
    }}, false, false},

    {"snap layouter only", ThemeFeature::SnapLayouter, ThemeFeature::SnapLayouter, true, false, 0, 1, 0, {InPlaceInit, {
        ThemeFeature::SnapLayouter
    }}, false, false},
    {"snap layouter only, everything supported", ThemeFeature::SnapLayouter, ~ThemeFeatures{}, true, false, 0, 1, 0, {InPlaceInit, {
        ThemeFeature::SnapLayouter
    }}, false, false},

    {"generic layouter only", ThemeFeature::GenericLayouter, ThemeFeature::GenericLayouter, true, false, 0, 1, 0, {InPlaceInit, {
        ThemeFeature::GenericLayouter
    }}, false, false},
    {"generic layouter only, everything supported", ThemeFeature::GenericLayouter, ~ThemeFeatures{}, true, false, 0, 1, 0, {InPlaceInit, {
        ThemeFeature::GenericLayouter
    }}, false, false},

    {"node animations only", ThemeFeature::NodeAnimations, ThemeFeature::NodeAnimations, true, false, 0, 0, 1, {InPlaceInit, {
        ThemeFeature::NodeAnimations
    }}, false, false},
    {"node animations only, everything supported", ThemeFeature::NodeAnimations, ~ThemeFeatures{}, true, false, 0, 0, 1, {InPlaceInit, {
        ThemeFeature::NodeAnimations
    }}, false, false},

    /* Explicitly verifying the case with background layer disabled and thus
       base layer used for it instead, to catch various layer aliasing edge
       cases */
    {"everything except background layer (and its animations)", ~(ThemeFeature::BackgroundLayer|ThemeFeature::BackgroundLayerAnimations), ~(ThemeFeature::BackgroundLayer|ThemeFeature::BackgroundLayerAnimations), true, false, 5, 2, 3, {InPlaceInit, {
        ~(ThemeFeature::BackgroundLayer|ThemeFeature::BackgroundLayerAnimations)
    }}, true, true},
    {"everything except background layer (and its animations), applied gradually", ~(ThemeFeature::BackgroundLayer|ThemeFeature::BackgroundLayerAnimations), ~(ThemeFeature::BackgroundLayer|ThemeFeature::BackgroundLayerAnimations), true, false, 5, 2, 3, {InPlaceInit, {
        ThemeFeature::TextLayer,
        ThemeFeature::TextLayerImages,
        ThemeFeature::BaseLayer,
        ThemeFeature::GenericLayouter,
        ThemeFeature::DataLayer,
        ThemeFeature::LayoutLayer,
        ThemeFeature::TextLayerAnimations,
        ThemeFeature::NodeAnimations,
        ThemeFeature::SnapLayouter,
        ThemeFeature::BaseLayerAnimations,
        ThemeFeature::EventLayer,
    }}, true, true},
    {"everything except background layer (and its animations), everything supported", ~(ThemeFeature::BackgroundLayer|ThemeFeature::BackgroundLayerAnimations), ~ThemeFeatures{}, true, false, 5, 2, 3, {InPlaceInit, {
        ~(ThemeFeature::BackgroundLayer|ThemeFeature::BackgroundLayerAnimations)
    }}, true, true},

    {"everything except base layer (and its animations)", ~(ThemeFeature::BaseLayer|ThemeFeature::BaseLayerAnimations), ~(ThemeFeature::BaseLayer|ThemeFeature::BaseLayerAnimations), true, false, 5, 2, 3, {InPlaceInit, {
        ~(ThemeFeature::BaseLayer|ThemeFeature::BaseLayerAnimations)
    }}, true, true},
    {"everything except base layer (and its animations), applied gradually", ~(ThemeFeature::BaseLayer|ThemeFeature::BaseLayerAnimations), ~(ThemeFeature::BaseLayer|ThemeFeature::BaseLayerAnimations), true, true, 5, 2, 3, {InPlaceInit, {
        ThemeFeature::TextLayer,
        ThemeFeature::TextLayerImages,
        /* Because this is applied later, an explicit compositing framebuffer
           has to be supplied */
        ThemeFeature::BackgroundLayer,
        ThemeFeature::GenericLayouter,
        ThemeFeature::LayoutLayer,
        ThemeFeature::TextLayerAnimations,
        ThemeFeature::DataLayer,
        ThemeFeature::NodeAnimations,
        ThemeFeature::SnapLayouter,
        ThemeFeature::BackgroundLayerAnimations,
        ThemeFeature::EventLayer,
    }}, true, true},
    {"everything except base layer (and its animations), everything supported", ~(ThemeFeature::BaseLayer|ThemeFeature::BaseLayerAnimations), ~ThemeFeatures{}, true, false, 5, 2, 3, {InPlaceInit, {
        ~(ThemeFeature::BaseLayer|ThemeFeature::BaseLayerAnimations)
    }}, true, true},

    {"everything", ~ThemeFeatures{}, ~ThemeFeatures{}, true, false, 6, 2, 4, {InPlaceInit, {
        ~ThemeFeatures{}
    }}, true, true},
    {"everything, implicit importer manager", ~ThemeFeatures{}, ~ThemeFeatures{}, true, false, 6, 2, 4, {InPlaceInit, {
        ~ThemeFeatures{}
    }}, false, true},
    {"everything, implicit font manager", ~ThemeFeatures{}, ~ThemeFeatures{}, true, false, 6, 2, 4, {InPlaceInit, {
        ~ThemeFeatures{}
    }}, true, false},
    {"everything, implicit importer & font manager", ~ThemeFeatures{}, ~ThemeFeatures{}, true, false, 6, 2, 4, {InPlaceInit, {
        ~ThemeFeatures{}
    }}, false, false},

    {"everything, applied gradually", ~ThemeFeatures{}, ~ThemeFeatures{}, true, true, 6, 2, 4, {InPlaceInit, {
        ThemeFeature::TextLayer,
        ThemeFeature::TextLayerImages,
        /* Because this is applied later, an explicit compositing framebuffer
           has to be supplied */
        ThemeFeature::BackgroundLayer,
        ThemeFeature::GenericLayouter,
        ThemeFeature::TextLayerAnimations,
        ThemeFeature::EventLayer,
        ThemeFeature::NodeAnimations,
        ThemeFeature::DataLayer,
        ThemeFeature::SnapLayouter,
        ThemeFeature::BackgroundLayerAnimations,
        ThemeFeature::BaseLayer,
        ThemeFeature::LayoutLayer,
        ThemeFeature::BaseLayerAnimations,
    }}, true, true},
    {"everything, applied gradually, implicit importer manager", ~ThemeFeatures{}, ~ThemeFeatures{}, true, true, 6, 2, 4, {InPlaceInit, {
        ThemeFeature::TextLayer,
        ThemeFeature::TextLayerImages,
        /* Because this is applied later, an explicit compositing framebuffer
           has to be supplied */
        ThemeFeature::BackgroundLayer,
        ThemeFeature::DataLayer,
        ThemeFeature::GenericLayouter,
        ThemeFeature::TextLayerAnimations,
        ThemeFeature::EventLayer,
        ThemeFeature::NodeAnimations,
        ThemeFeature::SnapLayouter,
        ThemeFeature::BackgroundLayerAnimations,
        ThemeFeature::BaseLayer,
        ThemeFeature::LayoutLayer,
        ThemeFeature::BaseLayerAnimations,
    }}, false, true},
    {"everything, applied gradually, implicit font manager", ~ThemeFeatures{}, ~ThemeFeatures{}, true, true, 6, 2, 4, {InPlaceInit, {
        ThemeFeature::TextLayer,
        ThemeFeature::TextLayerImages,
        /* Because this is applied later, an explicit compositing framebuffer
           has to be supplied */
        ThemeFeature::BackgroundLayer,
        ThemeFeature::GenericLayouter,
        ThemeFeature::TextLayerAnimations,
        ThemeFeature::EventLayer,
        ThemeFeature::NodeAnimations,
        ThemeFeature::SnapLayouter,
        ThemeFeature::BackgroundLayerAnimations,
        ThemeFeature::DataLayer,
        ThemeFeature::BaseLayer,
        ThemeFeature::LayoutLayer,
        ThemeFeature::BaseLayerAnimations,
    }}, true, false},
    {"everything, applied gradually, implicit importer & font manager", ~ThemeFeatures{}, ~ThemeFeatures{}, true, true, 6, 2, 4, {InPlaceInit, {
        ThemeFeature::TextLayer,
        ThemeFeature::TextLayerImages,
        /* Because this is applied later, an explicit compositing framebuffer
           has to be supplied */
        ThemeFeature::BackgroundLayer,
        ThemeFeature::GenericLayouter,
        ThemeFeature::TextLayerAnimations,
        ThemeFeature::DataLayer,
        ThemeFeature::EventLayer,
        ThemeFeature::NodeAnimations,
        ThemeFeature::SnapLayouter,
        ThemeFeature::BackgroundLayerAnimations,
        ThemeFeature::BaseLayer,
        ThemeFeature::LayoutLayer,
        ThemeFeature::BaseLayerAnimations,
    }}, false, false},

    {"application failed", ThemeFeature::BaseLayer|ThemeFeature::BaseLayerAnimations|ThemeFeature::EventLayer|ThemeFeature::SnapLayouter, ThemeFeature::BaseLayer|ThemeFeature::BaseLayerAnimations|ThemeFeature::EventLayer|ThemeFeature::SnapLayouter, false, false, 2, 1, 1, {InPlaceInit, {
        ThemeFeature::BaseLayer|ThemeFeature::BaseLayerAnimations|ThemeFeature::EventLayer|ThemeFeature::SnapLayouter
    }}, true, true},

    {"everything, implicitly", ~ThemeFeatures{}, ~ThemeFeatures{}, true, false, 6, 2, 4, {}, true, true},
    {"everything, implicitl, implicit importer manager", ~ThemeFeatures{}, ~ThemeFeatures{}, true, false, 6, 2, 4, {}, false, true},
    {"everything, implicitl, implicit font manager", ~ThemeFeatures{}, ~ThemeFeatures{}, true, false, 6, 2, 4, {}, true, false},
    {"everything, implicitl, implicit importer & font manager", ~ThemeFeatures{}, ~ThemeFeatures{}, true, false, 6, 2, 4, {}, false, false},
    {"everything, implicitly, application failed", ~ThemeFeatures{}, ~ThemeFeatures{}, false, false, 6, 2, 4, {}, true, true},
    {"everything, implicitly, only unknown feature supported", ThemeFeatures{0x8000}, ThemeFeatures{0x8000}, true, false, 0, 0, 0, {}, true, true},
    {"everything, implicitly, only base layer supported", ThemeFeature::BaseLayer, ThemeFeature::BaseLayer, true, false, 1, 0, 0, {}, false, false},
    {"everything, implicitly, everything except text layer (and its images and animations) supported", ~(ThemeFeature::TextLayer|ThemeFeature::TextLayerImages|ThemeFeature::TextLayerAnimations), ~(ThemeFeature::TextLayer|ThemeFeature::TextLayerImages|ThemeFeature::TextLayerAnimations), true, false, 5, 2, 3, {}, false, false},
};

UserInterfaceGLTest::UserInterfaceGLTest() {
    addInstancedTests({&UserInterfaceGLTest::construct,
                       &UserInterfaceGLTest::constructSingleSize},
        Containers::arraySize(ConstructData));

    addTests({&UserInterfaceGLTest::constructCopy,
              &UserInterfaceGLTest::constructMove});

    addInstancedTests({&UserInterfaceGLTest::create,
                       &UserInterfaceGLTest::createSingleSize},
        Containers::arraySize(CreateData));

    addInstancedTests({&UserInterfaceGLTest::createAlreadyCreated},
        Containers::arraySize(CreateAlreadyCreatedData));

    addTests({&UserInterfaceGLTest::createFailed});

    addInstancedTests({&UserInterfaceGLTest::setTheme},
        Containers::arraySize(SetThemeData));

    addTests({&UserInterfaceGLTest::setThemeRendererAlreadyPresent,
              &UserInterfaceGLTest::setThemeRendererNotCompositing,
              &UserInterfaceGLTest::setThemeNoFeatures,
              &UserInterfaceGLTest::setThemeFeaturesNotSupported,
              &UserInterfaceGLTest::setThemeNoSizeSet,
              &UserInterfaceGLTest::setThemeDataLayerAlreadyPresent,
              &UserInterfaceGLTest::setThemeBackgroundLayerAlreadyPresent,
              &UserInterfaceGLTest::setThemeBackgroundLayerStyleAnimatorAlreadyPresent,
              &UserInterfaceGLTest::setThemeBackgroundLayerStyleAnimationsBackgroundLayerNotPresentNotApplied,
              &UserInterfaceGLTest::setThemeBackgroundLayerStyleAnimationsBackgroundLayerNoDynamicStyles,
              &UserInterfaceGLTest::setThemeBaseLayerAlreadyPresent,
              &UserInterfaceGLTest::setThemeBaseLayerStyleAnimatorAlreadyPresent,
              &UserInterfaceGLTest::setThemeBaseLayerStyleAnimationsBaseLayerNotPresentNotApplied,
              &UserInterfaceGLTest::setThemeBaseLayerStyleAnimationsBaseLayerNoDynamicStyles,
              &UserInterfaceGLTest::setThemeTextLayerAlreadyPresent,
              &UserInterfaceGLTest::setThemeTextLayerImagesTextLayerNotPresentNotApplied,
              &UserInterfaceGLTest::setThemeTextLayerStyleAnimatorAlreadyPresent,
              &UserInterfaceGLTest::setThemeTextLayerStyleAnimationsTextLayerNotPresentNotApplied,
              &UserInterfaceGLTest::setThemeTextLayerStyleAnimationsTextLayerNoDynamicStyles,
              &UserInterfaceGLTest::setThemeEventLayerAlreadyPresent,
              &UserInterfaceGLTest::setThemeLayoutLayerAlreadyPresent,
              &UserInterfaceGLTest::setThemeSnapLayouterAlreadyPresent,
              &UserInterfaceGLTest::setThemeGenericLayouterAlreadyPresent,
              &UserInterfaceGLTest::setThemeNodeAnimatorAlreadyPresent});
}

void UserInterfaceGLTest::construct() {
    auto&& data = ConstructData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    Int applyCalled = 0;
    struct Theme: AbstractTheme {
        explicit Theme(Int& applyCalled, ThemeFeatures expectedFeatures): applyCalled(applyCalled), expectedFeatures{expectedFeatures} {}
        ThemeFeatures doFeatures() const override {
            /* Only basic theme features exposed here for simplicity, layouters
               and animators are tested thoroughly in setTheme*() */
            return ThemeFeature::BaseLayer|ThemeFeature::TextLayer|ThemeFeature::EventLayer|ThemeFeature(0x8000);
        }
        UnsignedInt doBaseLayerStyleCount() const override { return 1; }
        UnsignedInt doTextLayerStyleCount() const override { return 1; }
        Vector3i doTextLayerGlyphCacheSize(ThemeFeatures) const override {
            return Vector3i{1};
        }
        bool doApply(UserInterface&, ThemeFeatures features, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_COMPARE(features, expectedFeatures);
            ++applyCalled;
            return true;
        }

        Int& applyCalled;
        ThemeFeatures expectedFeatures;
    } theme{applyCalled, data.expectedThemeFeatures};

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::Optional<UserInterfaceGL> ui;
    if(data.themeFeatures)
        ui.emplace(Vector2{100.0f, 150.0f}, Vector2{50.0f, 75.0f}, Vector2i{200, 300}, theme, *data.themeFeatures, &_importerManager, &_fontManager);
    else
        ui.emplace(Vector2{100.0f, 150.0f}, Vector2{50.0f, 75.0f}, Vector2i{200, 300}, theme, &_importerManager, &_fontManager);
    CORRADE_COMPARE(ui->size(), (Vector2{100.0f, 150.0f}));
    CORRADE_COMPARE(ui->windowSize(), (Vector2{50.0f, 75.0f}));
    CORRADE_COMPARE(ui->framebufferSize(), (Vector2i{200, 300}));
    CORRADE_COMPARE(ui->layerCapacity(), data.expectedLayerCount);
    CORRADE_COMPARE(ui->layerUsedCount(), data.expectedLayerCount);
    CORRADE_COMPARE(ui->hasBaseLayer(), data.expectedThemeFeatures >= ThemeFeature::BaseLayer);
    CORRADE_COMPARE(ui->hasTextLayer(), data.expectedThemeFeatures >= ThemeFeature::TextLayer);
    CORRADE_COMPARE(ui->hasEventLayer(), data.expectedThemeFeatures >= ThemeFeature::EventLayer);
    CORRADE_COMPARE(applyCalled, 1);

    /* The renderer instance is set implicitly first time a theme is */
    CORRADE_VERIFY(ui->hasRendererInstance());
    CORRADE_COMPARE(ui->renderer().currentTargetState(), RendererTargetState::Initial);
    /* const overload */
    const UserInterfaceGL& cui = *ui;
    CORRADE_COMPARE(cui.renderer().currentTargetState(), RendererTargetState::Initial);
}

void UserInterfaceGLTest::constructSingleSize() {
    auto&& data = ConstructData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    Int applyCalled = 0;
    struct Theme: AbstractTheme {
        explicit Theme(Int& applyCalled, ThemeFeatures expectedFeatures): applyCalled(applyCalled), expectedFeatures{expectedFeatures} {}
        ThemeFeatures doFeatures() const override {
            /* Only basic theme features exposed here for simplicity, layouters
               and animators are tested thoroughly in setTheme*() */
            return ThemeFeature::BaseLayer|ThemeFeature::TextLayer|ThemeFeature::EventLayer|ThemeFeature(0x8000);
        }
        UnsignedInt doBaseLayerStyleCount() const override { return 1; }
        UnsignedInt doTextLayerStyleCount() const override { return 1; }
        Vector3i doTextLayerGlyphCacheSize(ThemeFeatures) const override {
            return Vector3i{1};
        }
        bool doApply(UserInterface&, ThemeFeatures features, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_COMPARE(features, expectedFeatures);
            ++applyCalled;
            return true;
        }

        Int& applyCalled;
        ThemeFeatures expectedFeatures;
    } theme{applyCalled, data.expectedThemeFeatures};

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::Optional<UserInterfaceGL> ui;
    if(data.themeFeatures)
        ui.emplace(Vector2i{200, 300}, theme, *data.themeFeatures, &_importerManager, &_fontManager);
    else
        ui.emplace(Vector2i{200, 300}, theme, &_importerManager, &_fontManager);
    CORRADE_COMPARE(ui->size(), (Vector2{200.0f, 300.0f}));
    CORRADE_COMPARE(ui->windowSize(), (Vector2{200.0f, 300.0f}));
    CORRADE_COMPARE(ui->framebufferSize(), (Vector2i{200, 300}));
    CORRADE_COMPARE(ui->layerCapacity(), data.expectedLayerCount);
    CORRADE_COMPARE(ui->layerUsedCount(), data.expectedLayerCount);
    CORRADE_COMPARE(ui->hasBaseLayer(), data.expectedThemeFeatures >= ThemeFeature::BaseLayer);
    CORRADE_COMPARE(ui->hasTextLayer(), data.expectedThemeFeatures >= ThemeFeature::TextLayer);
    CORRADE_COMPARE(ui->hasEventLayer(), data.expectedThemeFeatures >= ThemeFeature::EventLayer);
    CORRADE_COMPARE(applyCalled, 1);

    /* The renderer instance is set implicitly first time a theme is */
    CORRADE_VERIFY(ui->hasRendererInstance());
    CORRADE_COMPARE(ui->renderer().currentTargetState(), RendererTargetState::Initial);
    /* const overload */
    const UserInterfaceGL& cui = *ui;
    CORRADE_COMPARE(cui.renderer().currentTargetState(), RendererTargetState::Initial);
}

void UserInterfaceGLTest::constructCopy() {
    CORRADE_VERIFY(!std::is_copy_constructible<UserInterfaceGL>{});
    CORRADE_VERIFY(!std::is_copy_assignable<UserInterfaceGL>{});
}

void UserInterfaceGLTest::constructMove() {
    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override {
            return ThemeFeatures{0x8000};
        }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override { return true; }
    } theme;

    UserInterfaceGL a{{200, 300}, theme, &_importerManager, &_fontManager};
    a.setEventLayerInstance(Containers::pointer<EventLayer>(a.createLayer()));

    UserInterfaceGL b{Utility::move(a)};
    CORRADE_COMPARE(b.size(), (Vector2{200.0f, 300.0f}));
    CORRADE_VERIFY(b.hasEventLayer());

    UserInterfaceGL c{{10, 10}, theme, &_importerManager, &_fontManager};
    c = Utility::move(b);
    CORRADE_COMPARE(c.size(), (Vector2{200.0f, 300.0f}));
    CORRADE_VERIFY(c.hasEventLayer());

    CORRADE_VERIFY(std::is_nothrow_move_constructible<UserInterfaceGL>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<UserInterfaceGL>::value);
}

void UserInterfaceGLTest::create() {
    auto&& data = CreateData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    Int applyCalled = 0;
    struct Theme: AbstractTheme {
        explicit Theme(Int& applyCalled, ThemeFeatures expectedFeatures): applyCalled(applyCalled), expectedFeatures{expectedFeatures} {}
        ThemeFeatures doFeatures() const override {
            /* Only basic theme features exposed here for simplicity, layouters
               and animators are tested thoroughly in setTheme*() */
            return ThemeFeature::BaseLayer|ThemeFeature::TextLayer|ThemeFeature::EventLayer|ThemeFeature(0x8000);
        }
        UnsignedInt doBaseLayerStyleCount() const override { return 1; }
        UnsignedInt doTextLayerStyleCount() const override { return 1; }
        Vector3i doTextLayerGlyphCacheSize(ThemeFeatures) const override {
            return Vector3i{1};
        }
        bool doApply(UserInterface&, ThemeFeatures features, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_COMPARE(features, expectedFeatures);
            ++applyCalled;
            return true;
        }

        Int& applyCalled;
        ThemeFeatures expectedFeatures;
    } theme{applyCalled, data.expectedThemeFeatures};

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    UserInterfaceGL ui{NoCreate};
    if(data.tryCreate) {
        if(data.themeFeatures)
            CORRADE_VERIFY(ui.tryCreate({100.0f, 150.0f}, {50.0f, 75.0f}, {200, 300}, theme, *data.themeFeatures, &_importerManager, &_fontManager));
        else
            CORRADE_VERIFY(ui.tryCreate({100.0f, 150.0f}, {50.0f, 75.0f}, {200, 300}, theme, &_importerManager, &_fontManager));
    } else {
        if(data.themeFeatures)
            ui.create({100.0f, 150.0f}, {50.0f, 75.0f}, {200, 300}, theme, *data.themeFeatures, &_importerManager, &_fontManager);
        else
            ui.create({100.0f, 150.0f}, {50.0f, 75.0f}, {200, 300}, theme, &_importerManager, &_fontManager);
    }
    CORRADE_COMPARE(ui.size(), (Vector2{100.0f, 150.0f}));
    CORRADE_COMPARE(ui.windowSize(), (Vector2{50.0f, 75.0f}));
    CORRADE_COMPARE(ui.framebufferSize(), (Vector2i{200, 300}));
    CORRADE_COMPARE(ui.layerCapacity(), data.expectedLayerCount);
    CORRADE_COMPARE(ui.layerUsedCount(), data.expectedLayerCount);
    CORRADE_COMPARE(ui.hasBaseLayer(), data.expectedThemeFeatures >= ThemeFeature::BaseLayer);
    CORRADE_COMPARE(ui.hasTextLayer(), data.expectedThemeFeatures >= ThemeFeature::TextLayer);
    CORRADE_COMPARE(ui.hasEventLayer(), data.expectedThemeFeatures >= ThemeFeature::EventLayer);
    CORRADE_COMPARE(applyCalled, 1);

    /* The renderer instance is set implicitly first time a theme is */
    CORRADE_VERIFY(ui.hasRendererInstance());
    CORRADE_COMPARE(ui.renderer().currentTargetState(), RendererTargetState::Initial);
    /* const overload */
    const UserInterfaceGL& cui = ui;
    CORRADE_COMPARE(cui.renderer().currentTargetState(), RendererTargetState::Initial);
}

void UserInterfaceGLTest::createSingleSize() {
    auto&& data = CreateData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    Int applyCalled = 0;
    struct Theme: AbstractTheme {
        explicit Theme(Int& applyCalled, ThemeFeatures expectedFeatures): applyCalled(applyCalled), expectedFeatures{expectedFeatures} {}
        ThemeFeatures doFeatures() const override {
            return ThemeFeature::BaseLayer|ThemeFeature::TextLayer|ThemeFeature::EventLayer|ThemeFeature(0x8000);
        }
        UnsignedInt doBaseLayerStyleCount() const override { return 1; }
        UnsignedInt doTextLayerStyleCount() const override { return 1; }
        Vector3i doTextLayerGlyphCacheSize(ThemeFeatures) const override {
            return Vector3i{1};
        }
        bool doApply(UserInterface&, ThemeFeatures features, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_COMPARE(features, expectedFeatures);
            ++applyCalled;
            return true;
        }

        Int& applyCalled;
        ThemeFeatures expectedFeatures;
    } theme{applyCalled, data.expectedThemeFeatures};

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    UserInterfaceGL ui{NoCreate};
    if(data.tryCreate) {
        if(data.themeFeatures)
            CORRADE_VERIFY(ui.tryCreate({200, 300}, theme, *data.themeFeatures, &_importerManager, &_fontManager));
        else
            CORRADE_VERIFY(ui.tryCreate({200, 300}, theme, &_importerManager, &_fontManager));
    } else {
        if(data.themeFeatures)
            ui.create({200, 300}, theme, *data.themeFeatures, &_importerManager, &_fontManager);
        else
            ui.create({200, 300}, theme, &_importerManager, &_fontManager);
    }
    CORRADE_COMPARE(ui.size(), (Vector2{200.0f, 300.0f}));
    CORRADE_COMPARE(ui.windowSize(), (Vector2{200.0f, 300.0f}));
    CORRADE_COMPARE(ui.framebufferSize(), (Vector2i{200, 300}));
    CORRADE_COMPARE(ui.layerCapacity(), data.expectedLayerCount);
    CORRADE_COMPARE(ui.layerUsedCount(), data.expectedLayerCount);
    CORRADE_COMPARE(ui.hasBaseLayer(), data.expectedThemeFeatures >= ThemeFeature::BaseLayer);
    CORRADE_COMPARE(ui.hasTextLayer(), data.expectedThemeFeatures >= ThemeFeature::TextLayer);
    CORRADE_COMPARE(ui.hasEventLayer(), data.expectedThemeFeatures >= ThemeFeature::EventLayer);
    CORRADE_COMPARE(applyCalled, 1);

    /* The renderer instance is set implicitly first time a theme is */
    CORRADE_VERIFY(ui.hasRendererInstance());
    CORRADE_COMPARE(ui.renderer().currentTargetState(), RendererTargetState::Initial);
    /* const overload */
    const UserInterfaceGL& cui = ui;
    CORRADE_COMPARE(cui.renderer().currentTargetState(), RendererTargetState::Initial);
}

void UserInterfaceGLTest::createAlreadyCreated() {
    auto&& data = CreateAlreadyCreatedData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override {
            return ThemeFeature::BaseLayer|ThemeFeature::TextLayer|ThemeFeature::EventLayer|ThemeFeature::LayoutLayer|ThemeFeature::SnapLayouter;
        }
        UnsignedInt doBaseLayerStyleCount() const override { return 1; }
        UnsignedInt doTextLayerStyleCount() const override { return 1; }
        UnsignedInt doLayoutLayerStyleCount() const override { return 1; }
        Vector3i doTextLayerGlyphCacheSize(ThemeFeatures) const override {
            return {100, 100, 1};
        }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override {
            return true;
        }
    } theme;

    UserInterfaceGL ui{NoCreate};
    ui.setSize({100, 100});
    if(data.hasRenderer)
        ui.setRendererInstance(Containers::pointer<RendererGL>());

    /* Can't just do a ui.setStyle() with data.features because it always
       implicitly creates the renderer, and thus the rest of the condition
       would never be verified */
    Text::GlyphCacheArrayGL cache{PixelFormat::R8Unorm, {32, 32, 1}};
    BaseLayerGL::Shared baseLayerShared{BaseLayerGL::Shared::Configuration{1}};
    TextLayerGL::Shared textLayerShared{cache, TextLayerGL::Shared::Configuration{1}};
    if(data.features >= ThemeFeature::DataLayer)
        ui.setDataLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));
    if(data.features >= ThemeFeature::BackgroundLayer)
        ui.setBackgroundLayerInstance(Containers::pointer<BaseLayerGL>(ui.createLayer(), baseLayerShared));
    if(data.features >= ThemeFeature::BaseLayer)
        ui.setBaseLayerInstance(Containers::pointer<BaseLayerGL>(ui.createLayer(), baseLayerShared));
    if(data.features >= ThemeFeature::TextLayer)
        ui.setTextLayerInstance(Containers::pointer<TextLayerGL>(ui.createLayer(), textLayerShared));
    if(data.features >= ThemeFeature::EventLayer)
        ui.setEventLayerInstance(Containers::pointer<EventLayer>(ui.createLayer()));
    if(data.features >= ThemeFeature::LayoutLayer)
        ui.setLayoutLayerInstance(Containers::pointer<LayoutLayer>(ui.createLayer(), 1u));
    if(data.features >= ThemeFeature::SnapLayouter)
        ui.setSnapLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));
    if(data.features >= ThemeFeature::GenericLayouter)
        ui.setGenericLayouterInstance(Containers::pointer<GenericLayouter>(ui.createLayouter()));
    if(data.features >= ThemeFeature::NodeAnimations)
        ui.setNodeAnimatorInstance(Containers::pointer<NodeAnimator>(ui.createAnimator()));

    Containers::String out;
    Error redirectError{&out};
    if(data.tryCreate)
        ui.tryCreate({100, 100}, theme);
    else
        ui.create({100, 100}, theme);
    /* The message is printed by tryCreate() always */
    CORRADE_COMPARE(out, "Ui::UserInterfaceGL::tryCreate(): user interface already created\n");
}

void UserInterfaceGLTest::createFailed() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override {
            return ThemeFeature::EventLayer;
        }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override {
            return false;
        }
    } theme;

    UserInterfaceGL ui1{NoCreate}, ui2{NoCreate};
    CORRADE_VERIFY(!ui1.tryCreate({200, 300}, theme));
    /* Testing on another instance because the above has the EventLayer already
       created at this point */
    CORRADE_VERIFY(!ui2.tryCreate({100.0f, 150.0f}, {50.0f, 75.0f}, {200, 300}, theme));
}

void UserInterfaceGLTest::setTheme() {
    auto&& data = SetThemeData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    Int applyCalled = 0;
    ThemeFeatures glyphCacheSizeQueriedFeatures, actualFeatures;
    struct Theme: AbstractTheme {
        Theme(Int& applyCalled, ThemeFeatures& glyphCacheSizeQueriedFeatures, ThemeFeatures& actualFeatures, ThemeFeatures supportedFeatures, bool succeed): _applyCalled(applyCalled), _glyphCacheSizeQueriedFeatures(glyphCacheSizeQueriedFeatures), _actualFeatures(actualFeatures), _supportedFeatures{supportedFeatures}, _succeed{succeed} {}

        ThemeFeatures doFeatures() const override {
            return _supportedFeatures;
        }
        BaseLayerSharedFlags doBackgroundLayerFlags() const override {
            /* This flag is important to be present in order to test corner
               cases with compositing-enabled renderer */
            return BaseLayerSharedFlag::BackgroundBlur;
        }
        UnsignedInt doBackgroundLayerStyleUniformCount() const override { return 19; }
        UnsignedInt doBackgroundLayerStyleCount() const override { return 17; }
        UnsignedInt doBackgroundLayerDynamicStyleCount() const override { return 23; }
        UnsignedInt doBackgroundLayerBlurRadius() const override { return 2; }
        Float doBackgroundLayerBlurCutoff() const override { return 0.5f; }
        BaseLayerSharedFlags doBaseLayerFlags() const override {
            return BaseLayerSharedFlag::NoRoundedCorners;
        }
        UnsignedInt doBaseLayerStyleUniformCount() const override { return 3; }
        UnsignedInt doBaseLayerStyleCount() const override { return 5; }
        UnsignedInt doBaseLayerDynamicStyleCount() const override { return 11; }
        UnsignedInt doTextLayerStyleUniformCount() const override { return 2; }
        UnsignedInt doTextLayerStyleCount() const override { return 4; }
        UnsignedInt doTextLayerEditingStyleUniformCount() const override { return 6; }
        UnsignedInt doTextLayerEditingStyleCount() const override { return 7; }
        UnsignedInt doTextLayerDynamicStyleCount() const override { return 13; }
        PixelFormat doTextLayerGlyphCacheFormat() const override { return PixelFormat::R16F; }
        Vector3i doTextLayerGlyphCacheSize(ThemeFeatures features) const override {
            _glyphCacheSizeQueriedFeatures = features;
            return {16, 24, 3};
        }
        Vector2i doTextLayerGlyphCachePadding() const override { return {3, 1}; }
        UnsignedInt doLayoutLayerStyleCount() const override { return 15; }
        bool doApply(UserInterface&, ThemeFeatures features, PluginManager::Manager<Trade::AbstractImporter>* importerManager, PluginManager::Manager<Text::AbstractFont>* fontManager) const override {
            /* The features passed to this function and to the
               doTextLayerGlyphCacheSize() query, if called, should match */
            if(_glyphCacheSizeQueriedFeatures)
                CORRADE_COMPARE(features, _glyphCacheSizeQueriedFeatures);
            _glyphCacheSizeQueriedFeatures = {};

            _actualFeatures |= features;
            if(features >= ThemeFeature::TextLayer)
                CORRADE_VERIFY(fontManager);
            if(features >= ThemeFeature::TextLayerImages)
                CORRADE_VERIFY(importerManager);
            ++_applyCalled;
            return _succeed;
        }

        Int& _applyCalled;
        ThemeFeatures& _glyphCacheSizeQueriedFeatures;
        ThemeFeatures& _actualFeatures;
        ThemeFeatures _supportedFeatures;
        bool _succeed;
    } theme{applyCalled, glyphCacheSizeQueriedFeatures, actualFeatures, data.supportedFeatures, data.succeed};

    UserInterfaceGL ui{NoCreate};
    ui.setSize({200, 300});
    CORRADE_VERIFY(!ui.hasRendererInstance());
    CORRADE_COMPARE(ui.layerUsedCount(), 0);

    /* If the compositing-enabled background layer is added too late, we might
       not have a compositing-enabled renderer for it. In that case supply it
       explicitly first. */
    if(data.explicitCompositingFramebuffer) {
        ui.setRendererInstance(Containers::pointer<RendererGL>(RendererGL::Flag::CompositingFramebuffer));
    }

    if(data.features.isEmpty())
        CORRADE_COMPARE(ui.trySetTheme(theme,
            data.explicitImporterManager ? &_importerManager : nullptr,
            data.explicitFontManager ? &_fontManager : nullptr), data.succeed);
    else for(ThemeFeatures features: data.features)
        CORRADE_COMPARE(ui.trySetTheme(theme, features,
            data.explicitImporterManager ? &_importerManager : nullptr,
            data.explicitFontManager ? &_fontManager : nullptr), data.succeed);
    CORRADE_COMPARE(ui.layerUsedCount(), data.expectedLayerCount);
    CORRADE_COMPARE(ui.layouterUsedCount(), data.expectedLayouterCount);
    CORRADE_COMPARE(ui.animatorUsedCount(), data.expectedAnimatorCount);
    CORRADE_COMPARE(applyCalled, data.features.isEmpty() ? 1 : data.features.size());
    CORRADE_COMPARE(actualFeatures, data.expectedFeatures);

    /* The renderer instance is set implicitly first time a theme is, and only
       if not already */
    CORRADE_VERIFY(ui.hasRendererInstance());

    CORRADE_COMPARE(ui.hasBackgroundLayer(), data.expectedFeatures >= ThemeFeature::BackgroundLayer);
    if(data.expectedFeatures >= ThemeFeature::BackgroundLayer) {
        CORRADE_COMPARE(ui.backgroundLayer().shared().styleUniformCount(), 19);
        CORRADE_COMPARE(ui.backgroundLayer().shared().styleCount(), 17);
        CORRADE_COMPARE(ui.backgroundLayer().shared().dynamicStyleCount(), 23);
        CORRADE_COMPARE(ui.backgroundLayer().shared().flags(), BaseLayerSharedFlag::BackgroundBlur);
        CORRADE_COMPARE(ui.backgroundLayer().shared().backgroundBlurRadius(), 2);
        /** @todo check also the cutoff, once there's a getter */
    }

    CORRADE_COMPARE(ui.hasBackgroundLayerStyleAnimator(), data.expectedFeatures >= ThemeFeature::BackgroundLayerAnimations);

    CORRADE_COMPARE(ui.hasBaseLayer(), data.expectedFeatures >= ThemeFeature::BaseLayer);
    if(data.expectedFeatures >= ThemeFeature::BaseLayer) {
        CORRADE_COMPARE(ui.baseLayer().shared().styleUniformCount(), 3);
        CORRADE_COMPARE(ui.baseLayer().shared().styleCount(), 5);
        CORRADE_COMPARE(ui.baseLayer().shared().dynamicStyleCount(), 11);
        CORRADE_COMPARE(ui.baseLayer().shared().flags(), BaseLayerSharedFlag::NoRoundedCorners);
    }

    CORRADE_COMPARE(ui.hasBaseLayerStyleAnimator(), data.expectedFeatures >= ThemeFeature::BaseLayerAnimations);

    /* If we have a base layer but no background, the background should be an
       alias to it, with the same properties */
    if(data.expectedFeatures >= ThemeFeature::BaseLayer && !(data.expectedFeatures >= ThemeFeature::BackgroundLayer)) {
        CORRADE_COMPARE(&ui.backgroundLayer(), &ui.baseLayer());
        CORRADE_COMPARE(ui.backgroundLayer().shared().styleUniformCount(), 3);
        CORRADE_COMPARE(ui.backgroundLayer().shared().styleCount(), 5);
        CORRADE_COMPARE(ui.backgroundLayer().shared().dynamicStyleCount(), 11);
        CORRADE_COMPARE(ui.backgroundLayer().shared().flags(), BaseLayerSharedFlag::NoRoundedCorners);
    }

    /* Similarly for the animator */
    if(data.expectedFeatures >= ThemeFeature::BaseLayerAnimations && !(data.expectedFeatures >= ThemeFeature::BackgroundLayer)) {
        CORRADE_COMPARE(&ui.backgroundLayerStyleAnimator(), &ui.baseLayerStyleAnimator());
        CORRADE_COMPARE(ui.backgroundLayerStyleAnimator().layer(), ui.baseLayer().handle());
    }

    CORRADE_COMPARE(ui.hasTextLayer(), data.expectedFeatures >= ThemeFeature::TextLayer);
    if(data.expectedFeatures >= ThemeFeature::TextLayer) {
        CORRADE_COMPARE(ui.textLayer().shared().styleUniformCount(), 2);
        CORRADE_COMPARE(ui.textLayer().shared().styleCount(), 4);
        CORRADE_COMPARE(ui.textLayer().shared().editingStyleUniformCount(), 6);
        CORRADE_COMPARE(ui.textLayer().shared().editingStyleCount(), 7);
        CORRADE_COMPARE(ui.textLayer().shared().dynamicStyleCount(), 13);

        CORRADE_COMPARE(ui.textLayer().shared().glyphCache().format(), PixelFormat::R16F);
        CORRADE_COMPARE(ui.textLayer().shared().glyphCache().size(), (Vector3i{16, 24, 3}));
        CORRADE_COMPARE(ui.textLayer().shared().glyphCache().padding(), (Vector2i{3, 1}));
    }

    CORRADE_COMPARE(ui.hasTextLayerStyleAnimator(), data.expectedFeatures >= ThemeFeature::TextLayerAnimations);

    CORRADE_COMPARE(ui.hasEventLayer(), data.expectedFeatures >= ThemeFeature::EventLayer);

    CORRADE_COMPARE(ui.hasLayoutLayer(), data.expectedFeatures >= ThemeFeature::LayoutLayer);
    if(data.expectedFeatures >= ThemeFeature::LayoutLayer) {
        CORRADE_COMPARE(ui.layoutLayer().styleCount(), 15);
    }

    CORRADE_COMPARE(ui.hasSnapLayouter(), data.expectedFeatures >= ThemeFeature::SnapLayouter);

    CORRADE_COMPARE(ui.hasGenericLayouter(), data.expectedFeatures >= ThemeFeature::GenericLayouter);

    CORRADE_COMPARE(ui.hasNodeAnimator(), data.expectedFeatures >= ThemeFeature::NodeAnimations);
}

void UserInterfaceGLTest::setThemeRendererAlreadyPresent() {
    UserInterfaceGL ui{NoCreate};
    ui.setSize({200, 300});
    CORRADE_VERIFY(!ui.hasRendererInstance());

    ui.setRendererInstance(Containers::pointer<RendererGL>());
    CORRADE_VERIFY(ui.hasRendererInstance());

    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override {
            return ThemeFeatures{0x8000};
        }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override { return true; }
    } theme;

    /* Setting a theme shouldn't attempt to set a renderer instance again if
       it's already there */
    ui.setTheme(theme);
    CORRADE_VERIFY(ui.hasRendererInstance());
}

void UserInterfaceGLTest::setThemeRendererNotCompositing() {
    CORRADE_SKIP_IF_NO_ASSERT();

    UserInterfaceGL ui{NoCreate};
    ui.setSize({200, 300});
    ui.setRendererInstance(Containers::pointer<RendererGL>());
    CORRADE_VERIFY(ui.hasRendererInstance());

    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override {
            return ThemeFeature::BackgroundLayer;
        }
        BaseLayerSharedFlags doBackgroundLayerFlags() const override {
            return BaseLayerSharedFlag::BackgroundBlur;
        }
        UnsignedInt doBackgroundLayerStyleCount() const override { return 1; }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override { return true; }
    } theme;

    Containers::String out;
    Error redirectError{&out};
    ui.trySetTheme(theme, &_importerManager, &_fontManager);
    CORRADE_COMPARE(out, "Ui::UserInterfaceGL::trySetTheme(): background layer style requires a framebuffer with Ui::RendererGL::Flag::CompositingFramebuffer enabled\n");
}

void UserInterfaceGLTest::setThemeNoFeatures() {
    CORRADE_SKIP_IF_NO_ASSERT();

    UserInterfaceGL ui{NoCreate};
    ui.setSize({200, 300});

    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override { return ThemeFeature::BaseLayer; }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } theme;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::String out;
    Error redirectError{&out};
    ui.trySetTheme(theme, {}, &_importerManager, &_fontManager);
    CORRADE_COMPARE(out, "Ui::UserInterfaceGL::trySetTheme(): no features specified\n");
}

void UserInterfaceGLTest::setThemeFeaturesNotSupported() {
    CORRADE_SKIP_IF_NO_ASSERT();

    UserInterfaceGL ui{NoCreate};
    ui.setSize({200, 300});

    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override { return ThemeFeature::BaseLayer; }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } theme;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::String out;
    Error redirectError{&out};
    ui.trySetTheme(theme, ThemeFeature::BaseLayer|ThemeFeature::TextLayer, &_importerManager, &_fontManager);
    CORRADE_COMPARE(out, "Ui::UserInterfaceGL::trySetTheme(): Ui::ThemeFeature::BaseLayer|Ui::ThemeFeature::TextLayer not a subset of supported Ui::ThemeFeature::BaseLayer\n");
}

void UserInterfaceGLTest::setThemeNoSizeSet() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override {
            return ThemeFeature::EventLayer;
        }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override {
            return false;
        }
    } theme;

    UserInterfaceGL ui{NoCreate};

    Containers::String out;
    Error redirectError{&out};
    ui.setTheme(theme);
    ui.trySetTheme(theme);
    CORRADE_COMPARE(out,
        "Ui::UserInterfaceGL::trySetTheme(): user interface size wasn't set\n"
        "Ui::UserInterfaceGL::trySetTheme(): user interface size wasn't set\n");
}

void UserInterfaceGLTest::setThemeDataLayerAlreadyPresent() {
    CORRADE_SKIP_IF_NO_ASSERT();

    UserInterfaceGL ui{NoCreate};
    ui.setSize({200, 300})
      .setDataLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));

    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override { return ThemeFeature::DataLayer; }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } theme;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::String out;
    Error redirectError{&out};
    ui.trySetTheme(theme, &_importerManager, &_fontManager);
    CORRADE_COMPARE(out, "Ui::UserInterfaceGL::trySetTheme(): data layer already present\n");
}

void UserInterfaceGLTest::setThemeBackgroundLayerAlreadyPresent() {
    CORRADE_SKIP_IF_NO_ASSERT();

    BaseLayerGL::Shared shared{BaseLayer::Shared::Configuration{1}};
    UserInterfaceGL ui{NoCreate};
    ui.setSize({200, 300})
      .setBackgroundLayerInstance(Containers::pointer<BaseLayerGL>(ui.createLayer(), shared));

    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override {
            return ThemeFeature::BackgroundLayer;
        }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } theme;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::String out;
    Error redirectError{&out};
    ui.trySetTheme(theme, &_importerManager, &_fontManager);
    CORRADE_COMPARE(out, "Ui::UserInterfaceGL::trySetTheme(): background layer already present\n");
}

void UserInterfaceGLTest::setThemeBackgroundLayerStyleAnimatorAlreadyPresent() {
    CORRADE_SKIP_IF_NO_ASSERT();

    BaseLayerGL::Shared shared{BaseLayer::Shared::Configuration{1}
        .setDynamicStyleCount(1)
    };

    UserInterfaceGL ui{NoCreate};
    ui.setSize({200, 300})
      .setBackgroundLayerInstance(Containers::pointer<BaseLayerGL>(ui.createLayer(), shared))
      .setBackgroundLayerStyleAnimatorInstance(Containers::pointer<BaseLayerStyleAnimator>(ui.createAnimator()));

    UserInterfaceGL uiWithBaseLayerAnimator{NoCreate};
    uiWithBaseLayerAnimator
        .setSize({200, 300})
        .setBaseLayerInstance(Containers::pointer<BaseLayerGL>(uiWithBaseLayerAnimator.createLayer(), shared))
        .setBaseLayerStyleAnimatorInstance(Containers::pointer<BaseLayerStyleAnimator>(uiWithBaseLayerAnimator.createAnimator()));

    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override {
            return ThemeFeature::BackgroundLayerAnimations;
        }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } theme;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::String out;
    Error redirectError{&out};
    ui.trySetTheme(theme, &_importerManager, &_fontManager);
    /* This assertion should *not* be triggered if the base layer animator
       (which implicitly aliases to background layer animator) is present,
       instead it should fall through to the other assertions */
    uiWithBaseLayerAnimator.trySetTheme(theme, &_importerManager, &_fontManager);
    CORRADE_COMPARE_AS(out,
        "Ui::UserInterfaceGL::trySetTheme(): background layer style animator already present\n"
        "Ui::UserInterfaceGL::trySetTheme(): background layer not present and Ui::ThemeFeature::BackgroundLayer isn't being applied as well for Ui::ThemeFeature::BackgroundLayerAnimations\n",
        TestSuite::Compare::String);
}

void UserInterfaceGLTest::setThemeBackgroundLayerStyleAnimationsBackgroundLayerNotPresentNotApplied() {
    CORRADE_SKIP_IF_NO_ASSERT();

    BaseLayerGL::Shared shared{BaseLayer::Shared::Configuration{1}
        .setDynamicStyleCount(1)
    };

    UserInterfaceGL ui{NoCreate};
    ui.setSize({200, 300});

    UserInterfaceGL uiWithBaseLayer{NoCreate};
    uiWithBaseLayer
        .setSize({200, 300})
        .setBaseLayerInstance(Containers::pointer<BaseLayerGL>(uiWithBaseLayer.createLayer(), shared));

    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override {
            return ThemeFeature::BackgroundLayerAnimations;
        }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } theme;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::String out;
    Error redirectError{&out};
    ui.trySetTheme(theme, &_importerManager, &_fontManager);
    /* The same assertion should be triggered even if base layer (which
       implicitly aliases to background layer) is present */
    uiWithBaseLayer.trySetTheme(theme, &_importerManager, &_fontManager);
    CORRADE_COMPARE_AS(out,
        "Ui::UserInterfaceGL::trySetTheme(): background layer not present and Ui::ThemeFeature::BackgroundLayer isn't being applied as well for Ui::ThemeFeature::BackgroundLayerAnimations\n"
        "Ui::UserInterfaceGL::trySetTheme(): background layer not present and Ui::ThemeFeature::BackgroundLayer isn't being applied as well for Ui::ThemeFeature::BackgroundLayerAnimations\n",
        TestSuite::Compare::String);
}

void UserInterfaceGLTest::setThemeBackgroundLayerStyleAnimationsBackgroundLayerNoDynamicStyles() {
    CORRADE_SKIP_IF_NO_ASSERT();

    /* No dynamic styles enabled here */
    BaseLayerGL::Shared shared{BaseLayer::Shared::Configuration{1}};
    UserInterfaceGL ui{NoCreate};
    ui.setSize({200, 300})
      .setBackgroundLayerInstance(Containers::pointer<BaseLayerGL>(ui.createLayer(), shared));

    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override {
            return ThemeFeature::BackgroundLayerAnimations;
        }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } theme;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::String out;
    Error redirectError{&out};
    ui.trySetTheme(theme, &_importerManager, &_fontManager);
    CORRADE_COMPARE(out, "Ui::UserInterfaceGL::trySetTheme(): Ui::ThemeFeature::BackgroundLayerAnimations requires the background layer to have least one dynamic style\n");
}

void UserInterfaceGLTest::setThemeBaseLayerAlreadyPresent() {
    CORRADE_SKIP_IF_NO_ASSERT();

    BaseLayerGL::Shared shared{BaseLayer::Shared::Configuration{1}};
    UserInterfaceGL ui{NoCreate};
    ui.setSize({200, 300})
      .setBaseLayerInstance(Containers::pointer<BaseLayerGL>(ui.createLayer(), shared));

    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override { return ThemeFeature::BaseLayer; }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } theme;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::String out;
    Error redirectError{&out};
    ui.trySetTheme(theme, &_importerManager, &_fontManager);
    CORRADE_COMPARE(out, "Ui::UserInterfaceGL::trySetTheme(): base layer already present\n");
}

void UserInterfaceGLTest::setThemeBaseLayerStyleAnimatorAlreadyPresent() {
    CORRADE_SKIP_IF_NO_ASSERT();

    BaseLayerGL::Shared shared{BaseLayer::Shared::Configuration{1}
        .setDynamicStyleCount(1)
    };
    UserInterfaceGL ui{NoCreate};
    ui.setSize({200, 300})
      .setBaseLayerInstance(Containers::pointer<BaseLayerGL>(ui.createLayer(), shared))
      .setBaseLayerStyleAnimatorInstance(Containers::pointer<BaseLayerStyleAnimator>(ui.createAnimator()));

    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override { return ThemeFeature::BaseLayerAnimations; }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } theme;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::String out;
    Error redirectError{&out};
    ui.trySetTheme(theme, &_importerManager, &_fontManager);
    CORRADE_COMPARE(out, "Ui::UserInterfaceGL::trySetTheme(): base layer style animator already present\n");
}

void UserInterfaceGLTest::setThemeBaseLayerStyleAnimationsBaseLayerNotPresentNotApplied() {
    CORRADE_SKIP_IF_NO_ASSERT();

    UserInterfaceGL ui{NoCreate};
    ui.setSize({200, 300});

    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override { return ThemeFeature::BaseLayerAnimations; }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } theme;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::String out;
    Error redirectError{&out};
    ui.trySetTheme(theme, &_importerManager, &_fontManager);
    CORRADE_COMPARE(out, "Ui::UserInterfaceGL::trySetTheme(): base layer not present and Ui::ThemeFeature::BaseLayer isn't being applied as well for Ui::ThemeFeature::BaseLayerAnimations\n");
}

void UserInterfaceGLTest::setThemeBaseLayerStyleAnimationsBaseLayerNoDynamicStyles() {
    CORRADE_SKIP_IF_NO_ASSERT();

    /* No dynamic styles enabled here */
    BaseLayerGL::Shared shared{BaseLayer::Shared::Configuration{1}};
    UserInterfaceGL ui{NoCreate};
    ui.setSize({200, 300})
      .setBaseLayerInstance(Containers::pointer<BaseLayerGL>(ui.createLayer(), shared));

    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override { return ThemeFeature::BaseLayerAnimations; }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } theme;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::String out;
    Error redirectError{&out};
    ui.trySetTheme(theme, &_importerManager, &_fontManager);
    CORRADE_COMPARE(out, "Ui::UserInterfaceGL::trySetTheme(): Ui::ThemeFeature::BaseLayerAnimations requires the base layer to have least one dynamic style\n");
}

void UserInterfaceGLTest::setThemeTextLayerAlreadyPresent() {
    CORRADE_SKIP_IF_NO_ASSERT();

    Text::GlyphCacheArrayGL cache{PixelFormat::R8Unorm, {32, 32, 1}};

    TextLayerGL::Shared shared{cache, TextLayer::Shared::Configuration{1}};
    UserInterfaceGL ui{NoCreate};
    ui.setSize({200, 300})
      .setTextLayerInstance(Containers::pointer<TextLayerGL>(ui.createLayer(), shared));

    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override { return ThemeFeature::TextLayer; }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } theme;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::String out;
    Error redirectError{&out};
    ui.trySetTheme(theme, &_importerManager, &_fontManager);
    CORRADE_COMPARE(out, "Ui::UserInterfaceGL::trySetTheme(): text layer already present\n");
}

void UserInterfaceGLTest::setThemeTextLayerImagesTextLayerNotPresentNotApplied() {
    CORRADE_SKIP_IF_NO_ASSERT();

    UserInterfaceGL ui{NoCreate};
    ui.setSize({200, 300});

    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override { return ThemeFeature::TextLayerImages; }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } theme;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::String out;
    Error redirectError{&out};
    ui.trySetTheme(theme, &_importerManager, &_fontManager);
    CORRADE_COMPARE(out, "Ui::UserInterfaceGL::trySetTheme(): text layer not present and Ui::ThemeFeature::TextLayer isn't being applied as well for Ui::ThemeFeature::TextLayerImages\n");
}

void UserInterfaceGLTest::setThemeTextLayerStyleAnimatorAlreadyPresent() {
    CORRADE_SKIP_IF_NO_ASSERT();

    Text::GlyphCacheArrayGL cache{PixelFormat::R8Unorm, {32, 32, 1}};

    TextLayerGL::Shared shared{cache, TextLayer::Shared::Configuration{1}
        .setDynamicStyleCount(1)
    };
    UserInterfaceGL ui{NoCreate};
    ui.setSize({200, 300})
      .setTextLayerInstance(Containers::pointer<TextLayerGL>(ui.createLayer(), shared))
      .setTextLayerStyleAnimatorInstance(Containers::pointer<TextLayerStyleAnimator>(ui.createAnimator()));

    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override { return ThemeFeature::TextLayerAnimations; }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } theme;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::String out;
    Error redirectError{&out};
    ui.trySetTheme(theme, &_importerManager, &_fontManager);
    CORRADE_COMPARE(out, "Ui::UserInterfaceGL::trySetTheme(): text layer style animator already present\n");
}

void UserInterfaceGLTest::setThemeTextLayerStyleAnimationsTextLayerNotPresentNotApplied() {
    CORRADE_SKIP_IF_NO_ASSERT();

    UserInterfaceGL ui{NoCreate};
    ui.setSize({200, 300});

    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override { return ThemeFeature::TextLayerAnimations; }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } theme;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::String out;
    Error redirectError{&out};
    ui.trySetTheme(theme, &_importerManager, &_fontManager);
    CORRADE_COMPARE(out, "Ui::UserInterfaceGL::trySetTheme(): text layer not present and Ui::ThemeFeature::TextLayer isn't being applied as well for Ui::ThemeFeature::TextLayerAnimations\n");
}

void UserInterfaceGLTest::setThemeTextLayerStyleAnimationsTextLayerNoDynamicStyles() {
    CORRADE_SKIP_IF_NO_ASSERT();

    Text::GlyphCacheArrayGL cache{PixelFormat::R8Unorm, {32, 32, 1}};

    /* No dynamic styles enabled here */
    TextLayerGL::Shared shared{cache, TextLayer::Shared::Configuration{1}};
    UserInterfaceGL ui{NoCreate};
    ui.setSize({200, 300})
      .setTextLayerInstance(Containers::pointer<TextLayerGL>(ui.createLayer(), shared));

    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override { return ThemeFeature::TextLayerAnimations; }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } theme;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::String out;
    Error redirectError{&out};
    ui.trySetTheme(theme, &_importerManager, &_fontManager);
    CORRADE_COMPARE(out, "Ui::UserInterfaceGL::trySetTheme(): Ui::ThemeFeature::TextLayerAnimations requires the text layer to have least one dynamic style\n");
}

void UserInterfaceGLTest::setThemeEventLayerAlreadyPresent() {
    CORRADE_SKIP_IF_NO_ASSERT();

    UserInterfaceGL ui{NoCreate};
    ui.setSize({200, 300})
      .setEventLayerInstance(Containers::pointer<EventLayer>(ui.createLayer()));

    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override { return ThemeFeature::EventLayer; }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } theme;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::String out;
    Error redirectError{&out};
    ui.trySetTheme(theme, &_importerManager, &_fontManager);
    CORRADE_COMPARE(out, "Ui::UserInterfaceGL::trySetTheme(): event layer already present\n");
}

void UserInterfaceGLTest::setThemeLayoutLayerAlreadyPresent() {
    CORRADE_SKIP_IF_NO_ASSERT();

    UserInterfaceGL ui{NoCreate};
    ui.setSize({200, 300})
      .setLayoutLayerInstance(Containers::pointer<LayoutLayer>(ui.createLayer(), 7u));

    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override { return ThemeFeature::LayoutLayer; }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } theme;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::String out;
    Error redirectError{&out};
    ui.trySetTheme(theme, &_importerManager, &_fontManager);
    CORRADE_COMPARE(out, "Ui::UserInterfaceGL::trySetTheme(): layout layer already present\n");
}

void UserInterfaceGLTest::setThemeSnapLayouterAlreadyPresent() {
    CORRADE_SKIP_IF_NO_ASSERT();

    UserInterfaceGL ui{NoCreate};
    ui.setSize({200, 300})
      .setSnapLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));

    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override { return ThemeFeature::SnapLayouter; }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } theme;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::String out;
    Error redirectError{&out};
    ui.trySetTheme(theme, &_importerManager, &_fontManager);
    CORRADE_COMPARE(out, "Ui::UserInterfaceGL::trySetTheme(): snap layouter already present\n");
}

void UserInterfaceGLTest::setThemeGenericLayouterAlreadyPresent() {
    CORRADE_SKIP_IF_NO_ASSERT();

    UserInterfaceGL ui{NoCreate};
    ui.setSize({200, 300})
      .setGenericLayouterInstance(Containers::pointer<GenericLayouter>(ui.createLayouter()));

    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override {
            return ThemeFeature::GenericLayouter;
        }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } theme;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::String out;
    Error redirectError{&out};
    ui.trySetTheme(theme, &_importerManager, &_fontManager);
    CORRADE_COMPARE(out, "Ui::UserInterfaceGL::trySetTheme(): generic layouter already present\n");
}

void UserInterfaceGLTest::setThemeNodeAnimatorAlreadyPresent() {
    CORRADE_SKIP_IF_NO_ASSERT();

    UserInterfaceGL ui{NoCreate};
    ui.setSize({200, 300})
      .setNodeAnimatorInstance(Containers::pointer<NodeAnimator>(ui.createAnimator()));

    struct: AbstractTheme {
        ThemeFeatures doFeatures() const override {
            return ThemeFeature::NodeAnimations;
        }
        bool doApply(UserInterface&, ThemeFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override {
            CORRADE_FAIL("This shouldn't get called.");
            return {};
        }
    } theme;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Containers::String out;
    Error redirectError{&out};
    ui.trySetTheme(theme, &_importerManager, &_fontManager);
    CORRADE_COMPARE(out, "Ui::UserInterfaceGL::trySetTheme(): node animator already present\n");
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::UserInterfaceGLTest)
