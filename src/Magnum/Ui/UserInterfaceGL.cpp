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

#include "UserInterfaceGL.h"

#include <Corrade/Utility/Assert.h>
#include <Magnum/Math/Time.h>
#include <Magnum/Math/Vector2.h>
#include <Magnum/Text/GlyphCacheGL.h>

#include "Magnum/Ui/AbstractStyle.h"
#include "Magnum/Ui/BaseLayerGL.h"
#include "Magnum/Ui/EventLayer.h"
#include "Magnum/Ui/RendererGL.h"
#include "Magnum/Ui/SnapLayouter.h"
#include "Magnum/Ui/TextLayerGL.h"
#include "Magnum/Ui/Implementation/userInterfaceState.h"

namespace Magnum { namespace Ui {

struct UserInterfaceGL::State: UserInterface::State {
    /* Not created by default in order to make the NoCreate constructor work
       without a GL context */
    BaseLayerGL::Shared baseLayerShared{NoCreate};
    TextLayerGL::Shared textLayerShared{NoCreate};
};

UserInterfaceGL::UserInterfaceGL(NoCreateT): UserInterface{NoCreate, Containers::pointer<State>()} {}

UserInterfaceGL::UserInterfaceGL(const Vector2i& size, const AbstractStyle& style, const StyleFeatures styleFeatures, PluginManager::Manager<Trade::AbstractImporter>* const importerManager, PluginManager::Manager<Text::AbstractFont>* const fontManager): UserInterfaceGL{Vector2{size}, Vector2{size}, size, style, styleFeatures, importerManager, fontManager} {}

UserInterfaceGL::UserInterfaceGL(const Vector2i& size, const AbstractStyle& style, PluginManager::Manager<Trade::AbstractImporter>* const importerManager, PluginManager::Manager<Text::AbstractFont>* const fontManager): UserInterfaceGL{size, style, style.features(), importerManager, fontManager} {}

UserInterfaceGL& UserInterfaceGL::createInternal(const AbstractStyle& style, const StyleFeatures styleFeatures, PluginManager::Manager<Trade::AbstractImporter>* const importerManager, PluginManager::Manager<Text::AbstractFont>* const fontManager) {
    if(!tryCreateInternal(style, styleFeatures, importerManager, fontManager))
        std::exit(1); /* LCOV_EXCL_LINE */
    return *this;
}

UserInterfaceGL& UserInterfaceGL::createInternal(const AbstractStyle& style, PluginManager::Manager<Trade::AbstractImporter>* const importerManager, PluginManager::Manager<Text::AbstractFont>* const fontManager) {
    return createInternal(style, style.features(), importerManager, fontManager);
}

UserInterfaceGL& UserInterfaceGL::create(const Vector2i& size, const AbstractStyle& style, const StyleFeatures styleFeatures, PluginManager::Manager<Trade::AbstractImporter>* const importerManager, PluginManager::Manager<Text::AbstractFont>* const fontManager) {
    return create(Vector2{size}, Vector2{size}, size, style, styleFeatures, importerManager, fontManager);
}

UserInterfaceGL& UserInterfaceGL::create(const Vector2i& size, const AbstractStyle& style, PluginManager::Manager<Trade::AbstractImporter>* const importerManager, PluginManager::Manager<Text::AbstractFont>* const fontManager) {
    return create(size, style, style.features(), importerManager, fontManager);
}

bool UserInterfaceGL::tryCreateInternal(const AbstractStyle& style, const StyleFeatures styleFeatures, PluginManager::Manager<Trade::AbstractImporter>* const importerManager, PluginManager::Manager<Text::AbstractFont>* const fontManager) {
    #ifndef CORRADE_NO_ASSERT
    State& state = static_cast<State&>(*_state);
    #endif
    CORRADE_ASSERT(!hasRendererInstance() && !state.baseLayer && !state.textLayer && !state.eventLayer && !state.snapLayouter,
        "Ui::UserInterfaceGL::tryCreate(): user interface already created",
        /* Has to return true with CORRADE_GRACEFUL_ASSERT so when tested
           through create() it doesn't std::exit() the whole executable */
        true);
    return trySetStyle(style, styleFeatures, importerManager, fontManager);
}

bool UserInterfaceGL::tryCreateInternal(const AbstractStyle& style, PluginManager::Manager<Trade::AbstractImporter>* const importerManager, PluginManager::Manager<Text::AbstractFont>* const fontManager) {
    return tryCreateInternal(style, style.features(), importerManager, fontManager);
}

bool UserInterfaceGL::tryCreate(const Vector2i& size, const AbstractStyle& style, const StyleFeatures styleFeatures, PluginManager::Manager<Trade::AbstractImporter>* const importerManager, PluginManager::Manager<Text::AbstractFont>* const fontManager) {
    return tryCreate(Vector2{size}, Vector2{size}, size, style, styleFeatures, importerManager, fontManager);
}

bool UserInterfaceGL::tryCreate(const Vector2i& size, const AbstractStyle& style, PluginManager::Manager<Trade::AbstractImporter>* const importerManager, PluginManager::Manager<Text::AbstractFont>* const fontManager) {
    return tryCreate(size, style, style.features(), importerManager, fontManager);
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

bool UserInterfaceGL::trySetStyle(const AbstractStyle& style, const StyleFeatures features, PluginManager::Manager<Trade::AbstractImporter>* const importerManager, PluginManager::Manager<Text::AbstractFont>* const fontManager) {
    CORRADE_ASSERT(features,
        "Ui::UserInterfaceGL::trySetStyle(): no features specified", {});
    CORRADE_ASSERT(features <= style.features(),
        "Ui::UserInterfaceGL::trySetStyle():" << features << "not a subset of supported" << style.features(), {});
    CORRADE_ASSERT(!framebufferSize().isZero(),
        "Ui::UserInterfaceGL::trySetStyle(): user interface size wasn't set",
        /* Has to return true with CORRADE_GRACEFUL_ASSERT so when tested
           through setStyle() it doesn't std::exit() the whole executable */
        true);

    State& state = static_cast<State&>(*_state);

    /* Create a renderer, if not already */
    if(!hasRendererInstance())
        setRendererInstance(Containers::pointer<RendererGL>());

    /* Create layers based on what features are wanted */
    if(features >= StyleFeature::BaseLayer) {
        CORRADE_ASSERT(!state.baseLayer,
            "Ui::UserInterfaceGL::trySetStyle(): base layer already present", {});
        state.baseLayerShared = BaseLayerGL::Shared{
            BaseLayer::Shared::Configuration{style.baseLayerStyleUniformCount(),
                                             style.baseLayerStyleCount()}
                .setDynamicStyleCount(style.baseLayerDynamicStyleCount())
                .addFlags(style.baseLayerFlags())};
        setBaseLayerInstance(Containers::pointer<BaseLayerGL>(createLayer(), state.baseLayerShared));
    }
    if(features >= StyleFeature::TextLayer) {
        CORRADE_ASSERT(!state.textLayer,
            "Ui::UserInterfaceGL::trySetStyle(): text layer already present", {});
        state.textLayerShared = TextLayerGL::Shared{
            Text::GlyphCacheArrayGL{
                style.textLayerGlyphCacheFormat(),
                style.textLayerGlyphCacheSize(features),
                style.textLayerGlyphCachePadding()},
            TextLayer::Shared::Configuration{style.textLayerStyleUniformCount(),
                                             style.textLayerStyleCount()}
                .setEditingStyleCount(style.textLayerEditingStyleUniformCount(),
                                      style.textLayerEditingStyleCount())
                .setDynamicStyleCount(style.textLayerDynamicStyleCount())};
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
    if(features >= StyleFeature::TextLayerImages) {
        /* If features contain StyleFeature::TextLayer, state.textLayer was
           already added above, so it's enough to check state.textLayer alone.
           However, mention the StateFeature as well to hint that they can be
           also applied both together. */
        CORRADE_ASSERT(state.textLayer,
            "Ui::UserInterfaceGL::trySetStyle(): text layer not present and" << StyleFeature::TextLayer << "isn't being applied as well", {});

        /* Create a local importer plugin manager if external wasn't passed. If
           the text layer isn't present, the manager shouldn't be present
           either. */
        CORRADE_INTERNAL_ASSERT(!_state->importerManager);
        if(importerManager) {
            _state->importerManager = importerManager;
        } else {
            _state->importerManagerStorage.emplace();
            _state->importerManager = &*_state->importerManagerStorage;
        }
    }
    if(features >= StyleFeature::EventLayer) {
        CORRADE_ASSERT(!state.eventLayer,
            "Ui::UserInterfaceGL::trySetStyle(): event layer already present", {});
        setEventLayerInstance(Containers::pointer<EventLayer>(createLayer()));
    }
    if(features >= StyleFeature::SnapLayouter) {
        CORRADE_ASSERT(!state.snapLayouter,
            "Ui::UserInterfaceGL::trySetStyle(): snap layouter already present", {});
        setSnapLayouterInstance(Containers::pointer<SnapLayouter>(createLayouter()));
    }

    return style.apply(*this, features, _state->importerManager, _state->fontManager);
}

bool UserInterfaceGL::trySetStyle(const AbstractStyle& style, PluginManager::Manager<Trade::AbstractImporter>* const importerManager, PluginManager::Manager<Text::AbstractFont>* const fontManager) {
    return trySetStyle(style, style.features(), importerManager, fontManager);
}

UserInterfaceGL& UserInterfaceGL::setStyle(const AbstractStyle& style, const StyleFeatures features, PluginManager::Manager<Trade::AbstractImporter>* const importerManager, PluginManager::Manager<Text::AbstractFont>* const fontManager) {
    if(!trySetStyle(style, features, importerManager, fontManager))
        std::exit(1); /* LCOV_EXCL_LINE */
    return *this;
}

UserInterfaceGL& UserInterfaceGL::setStyle(const AbstractStyle& style, PluginManager::Manager<Trade::AbstractImporter>* const importerManager, PluginManager::Manager<Text::AbstractFont>* const fontManager) {
    return setStyle(style, style.features(), importerManager, fontManager);
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
