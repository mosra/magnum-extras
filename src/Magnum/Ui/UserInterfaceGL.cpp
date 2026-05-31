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

#include "UserInterfaceGL.h"

#include <Corrade/Utility/Assert.h>
#include <Magnum/Math/Time.h>
#include <Magnum/Math/Vector2.h>
#include <Magnum/Text/GlyphCacheGL.h>

#include "Magnum/Ui/AbstractTheme.h"
#include "Magnum/Ui/BaseLayerGL.h"
#include "Magnum/Ui/BaseLayerAnimator.h"
#include "Magnum/Ui/DataLayer.h"
#include "Magnum/Ui/EventLayer.h"
#include "Magnum/Ui/GenericLayouter.h"
#include "Magnum/Ui/LayoutLayer.h"
#include "Magnum/Ui/NodeAnimator.h"
#include "Magnum/Ui/RendererGL.h"
#include "Magnum/Ui/SnapLayouter.h"
#include "Magnum/Ui/TextLayerGL.h"
#include "Magnum/Ui/TextLayerAnimator.h"
#include "Magnum/Ui/Implementation/userInterfaceState.h"

namespace Magnum { namespace Ui {

struct UserInterfaceGL::State: UserInterface::State {
    /* Not created by default in order to make the NoCreate constructor work
       without a GL context */
    BaseLayerGL::Shared
        backgroundLayerShared{NoCreate},
        baseLayerShared{NoCreate};
    TextLayerGL::Shared textLayerShared{NoCreate};
};

UserInterfaceGL::UserInterfaceGL(NoCreateT): UserInterface{NoCreate, Containers::pointer<State>()} {}

UserInterfaceGL::UserInterfaceGL(const Vector2i& size, const AbstractTheme& theme, const ThemeFeatures themeFeatures, PluginManager::Manager<Text::AbstractFont>* const fontManager): UserInterfaceGL{Vector2{size}, Vector2{size}, size, theme, themeFeatures, fontManager} {}

UserInterfaceGL::UserInterfaceGL(const Vector2i& size, const AbstractTheme& theme, PluginManager::Manager<Text::AbstractFont>* const fontManager): UserInterfaceGL{size, theme, theme.features(), fontManager} {}

UserInterfaceGL& UserInterfaceGL::createInternal(const AbstractTheme& theme, const ThemeFeatures themeFeatures, PluginManager::Manager<Text::AbstractFont>* const fontManager) {
    if(!tryCreateInternal(theme, themeFeatures, fontManager))
        std::exit(1); /* LCOV_EXCL_LINE */
    return *this;
}

UserInterfaceGL& UserInterfaceGL::createInternal(const AbstractTheme& theme, PluginManager::Manager<Text::AbstractFont>* const fontManager) {
    return createInternal(theme, theme.features(), fontManager);
}

UserInterfaceGL& UserInterfaceGL::create(const Vector2i& size, const AbstractTheme& theme, const ThemeFeatures themeFeatures, PluginManager::Manager<Text::AbstractFont>* const fontManager) {
    return create(Vector2{size}, Vector2{size}, size, theme, themeFeatures, fontManager);
}

UserInterfaceGL& UserInterfaceGL::create(const Vector2i& size, const AbstractTheme& theme, PluginManager::Manager<Text::AbstractFont>* const fontManager) {
    return create(size, theme, theme.features(), fontManager);
}

bool UserInterfaceGL::tryCreateInternal(const AbstractTheme& theme, const ThemeFeatures themeFeatures, PluginManager::Manager<Text::AbstractFont>* const fontManager) {
    #ifndef CORRADE_NO_ASSERT
    State& state = static_cast<State&>(*_state);
    #endif
    /* No need to test for backgroundLayerStyleAnimator /
       baseLayerStyleAnimator / textLayerStyleAnimator as those can be present
       only if backgroundLayer / baseLayer / textLayer is there already. Also
       no need to special-case for the background and base layer aliasing, as
       this is meant to fail even if just one of them is present. */
    CORRADE_ASSERT(!hasRendererInstance() &&
        !state.dataLayer &&
        !state.backgroundLayer &&
        !state.baseLayer &&
        !state.textLayer &&
        !state.eventLayer &&
        !state.layoutLayer &&
        !state.snapLayouter &&
        !state.genericLayouter &&
        !state.nodeAnimator,
        "Ui::UserInterfaceGL::tryCreate(): user interface already created",
        /* Has to return true with CORRADE_GRACEFUL_ASSERT so when tested
           through create() it doesn't std::exit() the whole executable */
        true);
    return trySetTheme(theme, themeFeatures, fontManager);
}

bool UserInterfaceGL::tryCreateInternal(const AbstractTheme& theme, PluginManager::Manager<Text::AbstractFont>* const fontManager) {
    return tryCreateInternal(theme, theme.features(), fontManager);
}

bool UserInterfaceGL::tryCreate(const Vector2i& size, const AbstractTheme& theme, const ThemeFeatures themeFeatures, PluginManager::Manager<Text::AbstractFont>* const fontManager) {
    return tryCreate(Vector2{size}, Vector2{size}, size, theme, themeFeatures, fontManager);
}

bool UserInterfaceGL::tryCreate(const Vector2i& size, const AbstractTheme& theme, PluginManager::Manager<Text::AbstractFont>* const fontManager) {
    return tryCreate(size, theme, theme.features(), fontManager);
}

UserInterfaceGL& UserInterfaceGL::setRendererInstance(Containers::Pointer<RendererGL>&& instance) {
    UserInterface::setRendererInstance(Utility::move(instance));
    return *this;
}

RendererGL& UserInterfaceGL::renderer() {
    return static_cast<RendererGL&>(UserInterface::renderer());
}

const RendererGL& UserInterfaceGL::renderer() const {
    return static_cast<const RendererGL&>(UserInterface::renderer());
}

bool UserInterfaceGL::trySetTheme(const AbstractTheme& theme, const ThemeFeatures features, PluginManager::Manager<Text::AbstractFont>* const fontManager) {
    CORRADE_ASSERT(features,
        "Ui::UserInterfaceGL::trySetTheme(): no features specified", {});
    CORRADE_ASSERT(features <= theme.features(),
        "Ui::UserInterfaceGL::trySetTheme():" << features << "not a subset of supported" << theme.features(), {});
    CORRADE_ASSERT(!framebufferSize().isZero(),
        "Ui::UserInterfaceGL::trySetTheme(): user interface size wasn't set",
        /* Has to return true with CORRADE_GRACEFUL_ASSERT so when tested
           through setTheme() it doesn't std::exit() the whole executable */
        true);

    State& state = static_cast<State&>(*_state);

    /* Create a background layer instance first to figure out what may need to
       be enabled for the renderer. Could also just check for BackgroundBlur
       being present in backgroundLayerFlags(), but this future-proofs it for
       when other effects are present. */
    Containers::Pointer<BaseLayerGL> backgroundLayer;
    if(features >= ThemeFeature::BackgroundLayer) {
        /* Querying the pointer alone isn't sufficient due to the base layer
           fallback, see hasBackgroundLayer() for details */
        CORRADE_ASSERT(!hasBackgroundLayer(),
            "Ui::UserInterfaceGL::trySetTheme(): background layer already present", {});
        state.backgroundLayerShared = BaseLayerGL::Shared{
            BaseLayer::Shared::Configuration{theme.backgroundLayerStyleUniformCount(),
                                             theme.backgroundLayerStyleCount()}
                .setDynamicStyleCount(theme.backgroundLayerDynamicStyleCount())
                .addFlags(theme.backgroundLayerFlags())
                .setBackgroundBlurRadius(theme.backgroundLayerBlurRadius(), theme.backgroundLayerBlurCutoff())};
        backgroundLayer.emplace(createLayer(), state.backgroundLayerShared);
    }

    /* Create a renderer, if not already, and enable compositing for it if the
       background layer needs it. Otherwise check that it has compositing
       enabled to provide a more helpful message to users than an assert coming
       from setRendererInstance() directly. */
    if(!hasRendererInstance())
        setRendererInstance(Containers::pointer<RendererGL>(
            backgroundLayer && backgroundLayer->features() >= LayerFeature::Composite ?
                RendererGL::Flag::CompositingFramebuffer : RendererGL::Flags{}
        ));
    else CORRADE_ASSERT(!backgroundLayer || !(backgroundLayer->features() >= LayerFeature::Composite) || renderer().flags() >= RendererGL::Flag::CompositingFramebuffer,
        "Ui::UserInterfaceGL::trySetTheme(): background layer style requires a framebuffer with" << RendererGL::Flag::CompositingFramebuffer << "enabled", {});

    /* Create layers, layouters and animators based on what features are
       wanted */
    if(features >= ThemeFeature::DataLayer) {
        CORRADE_ASSERT(!state.dataLayer,
            "Ui::UserInterfaceGL::trySetTheme(): data layer already present", {});
        setDataLayerInstance(Containers::pointer<DataLayer>(createLayer()));
    }
    if(features >= ThemeFeature::BackgroundLayer) {
        /* Created above already */
        setBackgroundLayerInstance(Utility::move(backgroundLayer));
    }
    if(features >= ThemeFeature::BackgroundLayerAnimations) {
        CORRADE_ASSERT(!hasBackgroundLayerStyleAnimator(),
            "Ui::UserInterfaceGL::trySetTheme(): background layer style animator already present", {});
        /* If features contain ThemeFeature::BackgroundLayer,
           state.backgroundLayer was already added above, so it's enough to
           check background layer presence alone. However, querying the pointer
           alone isn't sufficient due to the base layer fallback, see
           hasBackgroundLayer() for details. Also, mention the StateFeature
           as well to hint that they can be also applied both together. */
        CORRADE_ASSERT(hasBackgroundLayer(),
            "Ui::UserInterfaceGL::trySetTheme(): background layer not present and" << ThemeFeature::BackgroundLayer << "isn't being applied as well for" << ThemeFeature::BackgroundLayerAnimations, {});
        CORRADE_ASSERT(state.backgroundLayer->shared().dynamicStyleCount(),
            "Ui::UserInterfaceGL::trySetTheme():" << ThemeFeature::BackgroundLayerAnimations << "requires the background layer to have least one dynamic style", {});
        setBackgroundLayerStyleAnimatorInstance(Containers::pointer<BaseLayerStyleAnimator>(createAnimator()));
    }
    if(features >= ThemeFeature::BaseLayer) {
        CORRADE_ASSERT(!state.baseLayer,
            "Ui::UserInterfaceGL::trySetTheme(): base layer already present", {});
        state.baseLayerShared = BaseLayerGL::Shared{
            BaseLayer::Shared::Configuration{theme.baseLayerStyleUniformCount(),
                                             theme.baseLayerStyleCount()}
                .setDynamicStyleCount(theme.baseLayerDynamicStyleCount())
                .addFlags(theme.baseLayerFlags())};
        setBaseLayerInstance(Containers::pointer<BaseLayerGL>(createLayer(), state.baseLayerShared));
    }
    if(features >= ThemeFeature::BaseLayerAnimations) {
        CORRADE_ASSERT(!state.baseLayerStyleAnimator,
            "Ui::UserInterfaceGL::trySetTheme(): base layer style animator already present", {});
        /* If features contain ThemeFeature::BaseLayer, state.baseLayer was
           already added above, so it's enough to check state.baseLayer alone.
           However, mention the StateFeature as well to hint that they can be
           also applied both together. */
        CORRADE_ASSERT(state.baseLayer,
            "Ui::UserInterfaceGL::trySetTheme(): base layer not present and" << ThemeFeature::BaseLayer << "isn't being applied as well for" << ThemeFeature::BaseLayerAnimations, {});
        CORRADE_ASSERT(state.baseLayer->shared().dynamicStyleCount(),
            "Ui::UserInterfaceGL::trySetTheme():" << ThemeFeature::BaseLayerAnimations << "requires the base layer to have least one dynamic style", {});
        setBaseLayerStyleAnimatorInstance(Containers::pointer<BaseLayerStyleAnimator>(createAnimator()));
    }
    if(features >= ThemeFeature::TextLayer) {
        CORRADE_ASSERT(!state.textLayer,
            "Ui::UserInterfaceGL::trySetTheme(): text layer already present", {});
        state.textLayerShared = TextLayerGL::Shared{
            Text::GlyphCacheArrayGL{
                theme.textLayerGlyphCacheFormat(),
                theme.textLayerGlyphCacheSize(features),
                theme.textLayerGlyphCachePadding()},
            TextLayer::Shared::Configuration{theme.textLayerStyleUniformCount(),
                                             theme.textLayerStyleCount()}
                .setEditingStyleCount(theme.textLayerEditingStyleUniformCount(),
                                      theme.textLayerEditingStyleCount())
                .setDynamicStyleCount(theme.textLayerDynamicStyleCount())};
        setTextLayerInstance(Containers::pointer<TextLayerGL>(createLayer(), state.textLayerShared));

        /* Create a local font plugin manager if external wasn't passed. If the
           text layer isn't present, the manager shouldn't be present
           either. */
        CORRADE_INTERNAL_ASSERT(!_state->fontManager);
        if(fontManager) {
            _state->fontManager = fontManager;
        } else {
            _state->fontManagerStorage.emplace();
            _state->fontManager = &*_state->fontManagerStorage;
        }
    }
    if(features >= ThemeFeature::TextLayerAnimations) {
        CORRADE_ASSERT(!state.textLayerStyleAnimator,
            "Ui::UserInterfaceGL::trySetTheme(): text layer style animator already present", {});
        /* If features contain ThemeFeature::TextLayer, state.textLayer was
           already added above, so it's enough to check state.textLayer alone.
           However, mention the StateFeature as well to hint that they can be
           also applied both together. */
        CORRADE_ASSERT(state.textLayer,
            "Ui::UserInterfaceGL::trySetTheme(): text layer not present and" << ThemeFeature::TextLayer << "isn't being applied as well for" << ThemeFeature::TextLayerAnimations, {});
        CORRADE_ASSERT(state.textLayer->shared().dynamicStyleCount(),
            "Ui::UserInterfaceGL::trySetTheme():" << ThemeFeature::TextLayerAnimations << "requires the text layer to have least one dynamic style", {});
        setTextLayerStyleAnimatorInstance(Containers::pointer<TextLayerStyleAnimator>(createAnimator()));
    }
    if(features >= ThemeFeature::EventLayer) {
        CORRADE_ASSERT(!state.eventLayer,
            "Ui::UserInterfaceGL::trySetTheme(): event layer already present", {});
        setEventLayerInstance(Containers::pointer<EventLayer>(createLayer()));
    }
    if(features >= ThemeFeature::LayoutLayer) {
        CORRADE_ASSERT(!state.layoutLayer,
            "Ui::UserInterfaceGL::trySetTheme(): layout layer already present", {});
        setLayoutLayerInstance(Containers::pointer<LayoutLayer>(createLayer(), theme.layoutLayerStyleCount()));
    }
    if(features >= ThemeFeature::SnapLayouter) {
        CORRADE_ASSERT(!state.snapLayouter,
            "Ui::UserInterfaceGL::trySetTheme(): snap layouter already present", {});
        setSnapLayouterInstance(Containers::pointer<SnapLayouter>(createLayouter()));
    }
    if(features >= ThemeFeature::GenericLayouter) {
        CORRADE_ASSERT(!state.genericLayouter,
            "Ui::UserInterfaceGL::trySetTheme(): generic layouter already present", {});
        setGenericLayouterInstance(Containers::pointer<GenericLayouter>(createLayouter()));
    }
    if(features >= ThemeFeature::NodeAnimations) {
        CORRADE_ASSERT(!state.nodeAnimator,
            "Ui::UserInterfaceGL::trySetTheme(): node animator already present", {});
        setNodeAnimatorInstance(Containers::pointer<NodeAnimator>(createAnimator()));
    }

    return theme.apply(*this, features, _state->fontManager);
}

bool UserInterfaceGL::trySetTheme(const AbstractTheme& theme, PluginManager::Manager<Text::AbstractFont>* const fontManager) {
    return trySetTheme(theme, theme.features(), fontManager);
}

UserInterfaceGL& UserInterfaceGL::setTheme(const AbstractTheme& theme, const ThemeFeatures features, PluginManager::Manager<Text::AbstractFont>* const fontManager) {
    if(!trySetTheme(theme, features, fontManager))
        std::exit(1); /* LCOV_EXCL_LINE */
    return *this;
}

UserInterfaceGL& UserInterfaceGL::setTheme(const AbstractTheme& theme, PluginManager::Manager<Text::AbstractFont>* const fontManager) {
    return setTheme(theme, theme.features(), fontManager);
}

UserInterfaceGL& UserInterfaceGL::setBackgroundLayerInstance(Containers::Pointer<BaseLayerGL>&& instance) {
    return static_cast<UserInterfaceGL&>(UserInterface::setBackgroundLayerInstance(Utility::move(instance)));
}

UserInterfaceGL& UserInterfaceGL::setBaseLayerInstance(Containers::Pointer<BaseLayerGL>&& instance) {
    return static_cast<UserInterfaceGL&>(UserInterface::setBaseLayerInstance(Utility::move(instance)));
}

UserInterfaceGL& UserInterfaceGL::setTextLayerInstance(Containers::Pointer<TextLayerGL>&& instance) {
    return static_cast<UserInterfaceGL&>(UserInterface::setTextLayerInstance(Utility::move(instance)));
}

UserInterfaceGL& UserInterfaceGL::advanceAnimations(const Nanoseconds time) {
    return static_cast<UserInterfaceGL&>(UserInterface::advanceAnimations(time));
}

}}
