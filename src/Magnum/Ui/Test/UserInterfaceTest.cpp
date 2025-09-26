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

#include <Corrade/Containers/ArrayView.h>
#include <Corrade/Containers/String.h>
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/String.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/Math/Vector2.h>
#include <Magnum/Text/AbstractGlyphCache.h>

#include "Magnum/Ui/BaseLayer.h"
#include "Magnum/Ui/BaseLayerAnimator.h"
#include "Magnum/Ui/EventLayer.h"
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/SnapLayouter.h"
#include "Magnum/Ui/TextLayer.h"
#include "Magnum/Ui/TextLayerAnimator.h"
#include "Magnum/Ui/UserInterface.h"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct UserInterfaceTest: TestSuite::Tester {
    explicit UserInterfaceTest();

    void construct();
    void constructNoCreate();
    void constructCopy();
    void constructMove();

    void setBaseLayerInstance();
    void setBaseLayerInstanceInvalid();
    void baseLayerInvalid();

    void setBaseLayerStyleAnimatorInstance();
    void setBaseLayerStyleAnimatorInstanceInvalid();
    void baseLayerStyleAnimatorInvalid();

    void setTextLayerInstance();
    void setTextLayerInstanceInvalid();
    void textLayerInvalid();

    void setTextLayerStyleAnimatorInstance();
    void setTextLayerStyleAnimatorInstanceInvalid();
    void textLayerStyleAnimatorInvalid();

    void setEventLayerInstance();
    void setEventLayerInstanceInvalid();
    void eventLayerInvalid();

    void setSnapLayouterInstance();
    void setSnapLayouterInstanceInvalid();
    void snapLayouterInvalid();
};

const struct {
    const char* name;
    bool defaultAnimatorAlreadyExists;
} SetStyleAnimatorInstanceData[]{
    {"", false},
    {"default animator already exists", true}
};

UserInterfaceTest::UserInterfaceTest() {
    addTests({&UserInterfaceTest::construct,
              &UserInterfaceTest::constructNoCreate,
              &UserInterfaceTest::constructCopy,
              &UserInterfaceTest::constructMove,

              &UserInterfaceTest::setBaseLayerInstance,
              &UserInterfaceTest::setBaseLayerInstanceInvalid,
              &UserInterfaceTest::baseLayerInvalid});

    addInstancedTests({&UserInterfaceTest::setBaseLayerStyleAnimatorInstance},
        Containers::arraySize(SetStyleAnimatorInstanceData));

    addTests({&UserInterfaceTest::setBaseLayerStyleAnimatorInstanceInvalid,
              &UserInterfaceTest::baseLayerStyleAnimatorInvalid,

              &UserInterfaceTest::setTextLayerInstance,
              &UserInterfaceTest::setTextLayerInstanceInvalid,
              &UserInterfaceTest::textLayerInvalid});

    addInstancedTests({&UserInterfaceTest::setTextLayerStyleAnimatorInstance},
        Containers::arraySize(SetStyleAnimatorInstanceData));

    addTests({&UserInterfaceTest::setTextLayerStyleAnimatorInstanceInvalid,
              &UserInterfaceTest::textLayerStyleAnimatorInvalid,

              &UserInterfaceTest::setEventLayerInstance,
              &UserInterfaceTest::setEventLayerInstanceInvalid,
              &UserInterfaceTest::eventLayerInvalid,

              &UserInterfaceTest::setSnapLayouterInstance,
              &UserInterfaceTest::setSnapLayouterInstanceInvalid,
              &UserInterfaceTest::snapLayouterInvalid});
}

void UserInterfaceTest::construct() {
    struct Interface: UserInterface {
        explicit Interface(const Vector2i& size): UserInterface{NoCreate} {
            setSize(size);
        }
    } ui{{100, 150}};

    CORRADE_COMPARE(ui.size(), (Vector2{100.0f, 150.0f}));
    CORRADE_COMPARE(ui.layerCapacity(), 0);
    CORRADE_COMPARE(ui.layerUsedCount(), 0);
    CORRADE_VERIFY(!ui.hasBaseLayer());
    CORRADE_VERIFY(!ui.hasTextLayer());
    CORRADE_VERIFY(!ui.hasEventLayer());
    CORRADE_VERIFY(!ui.hasSnapLayouter());
}

void UserInterfaceTest::constructNoCreate() {
    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};

    /* Shouldn't crash */
    CORRADE_VERIFY(true);

    /* Implicit construction is not allowed */
    CORRADE_VERIFY(!std::is_convertible<NoCreateT, Interface>::value);
}

void UserInterfaceTest::constructCopy() {
    CORRADE_VERIFY(!std::is_copy_constructible<UserInterface>{});
    CORRADE_VERIFY(!std::is_copy_assignable<UserInterface>{});
}

void UserInterfaceTest::constructMove() {
    struct Interface: UserInterface {
        explicit Interface(const Vector2i& size): UserInterface{NoCreate} {
            setSize(size);
        }
    };

    Interface a{{100, 150}};
    CORRADE_VERIFY(!a.hasEventLayer());

    a.setEventLayerInstance(Containers::pointer<EventLayer>(a.createLayer()));
    CORRADE_VERIFY(a.hasEventLayer());

    Interface b{Utility::move(a)};
    CORRADE_COMPARE(b.size(), (Vector2{100.0f, 150.0f}));
    CORRADE_VERIFY(b.hasEventLayer());

    Interface c{{10, 10}};
    c = Utility::move(b);
    CORRADE_COMPARE(c.size(), (Vector2{100.0f, 150.0f}));
    CORRADE_VERIFY(c.hasEventLayer());

    CORRADE_VERIFY(std::is_nothrow_move_constructible<UserInterface>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<UserInterface>::value);
}

void UserInterfaceTest::setBaseLayerInstance() {
    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(const Configuration& configuration): BaseLayer::Shared{configuration} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } shared{BaseLayer::Shared::Configuration{1, 3}};

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    };

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    CORRADE_COMPARE(ui.layerCapacity(), 0);
    CORRADE_COMPARE(ui.layerUsedCount(), 0);
    CORRADE_VERIFY(!ui.hasBaseLayer());
    CORRADE_VERIFY(!ui.hasTextLayer());
    CORRADE_VERIFY(!ui.hasEventLayer());

    LayerHandle handle = ui.createLayer();
    Containers::Pointer<Layer> layer{InPlaceInit, handle, shared};
    Layer* pointer = layer.get();
    ui.setBaseLayerInstance(Utility::move(layer));
    CORRADE_COMPARE(ui.layerCapacity(), 1);
    CORRADE_COMPARE(ui.layerUsedCount(), 1);
    CORRADE_VERIFY(ui.hasBaseLayer());
    CORRADE_VERIFY(!ui.hasTextLayer());
    CORRADE_VERIFY(!ui.hasEventLayer());
    CORRADE_COMPARE(&ui.layer(handle), pointer);
    CORRADE_COMPARE(&ui.baseLayer(), pointer);
}

void UserInterfaceTest::setBaseLayerInstanceInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(const Configuration& configuration): BaseLayer::Shared{configuration} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } shared{BaseLayer::Shared::Configuration{3, 5}};

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    };

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    ui.setBaseLayerInstance(Containers::pointer<Layer>(ui.createLayer(), shared));

    Containers::String out;
    Error redirectError{&out};
    ui.setBaseLayerInstance(nullptr);
    ui.setBaseLayerInstance(Containers::pointer<Layer>(ui.createLayer(), shared));
    CORRADE_COMPARE(out,
        "Ui::UserInterface::setBaseLayerInstance(): instance is null\n"
        "Ui::UserInterface::setBaseLayerInstance(): instance already set\n");
}

void UserInterfaceTest::baseLayerInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    CORRADE_VERIFY(!ui.hasBaseLayer());

    Containers::String out;
    Error redirectError{&out};
    ui.baseLayer();
    CORRADE_COMPARE(out, "Ui::UserInterface::baseLayer(): no instance set\n");
}

void UserInterfaceTest::setBaseLayerStyleAnimatorInstance() {
    auto&& data = SetStyleAnimatorInstanceData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(const Configuration& configuration): BaseLayer::Shared{configuration} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } shared{BaseLayer::Shared::Configuration{1, 3}
        .setDynamicStyleCount(1)
    };

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    };

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    CORRADE_COMPARE(ui.animatorCapacity(), 0);
    CORRADE_COMPARE(ui.animatorUsedCount(), 0);
    CORRADE_VERIFY(!ui.hasBaseLayerStyleAnimator());
    CORRADE_VERIFY(!ui.hasTextLayerStyleAnimator());

    ui.setBaseLayerInstance(Containers::pointer<Layer>(ui.createLayer(), shared));

    BaseLayerStyleAnimator anotherAnimator{animatorHandle(0, 1)};
    if(data.defaultAnimatorAlreadyExists) {
        ui.baseLayer().assignAnimator(anotherAnimator);
        ui.baseLayer().setDefaultStyleAnimator(&anotherAnimator);
    }

    AnimatorHandle handle = ui.createAnimator();
    Containers::Pointer<BaseLayerStyleAnimator> animator{InPlaceInit, handle};
    BaseLayerStyleAnimator* pointer = animator.get();
    ui.setBaseLayerStyleAnimatorInstance(Utility::move(animator));
    CORRADE_COMPARE(ui.animatorCapacity(), 1);
    CORRADE_COMPARE(ui.animatorUsedCount(), 1);
    CORRADE_VERIFY(ui.hasBaseLayerStyleAnimator());
    CORRADE_VERIFY(!ui.hasTextLayerStyleAnimator());
    CORRADE_COMPARE(&ui.animator(handle), pointer);
    CORRADE_COMPARE(&ui.baseLayerStyleAnimator(), pointer);
    /* The default animator gets set even if it already exists */
    CORRADE_COMPARE(ui.baseLayer().defaultStyleAnimator(), pointer);
}

void UserInterfaceTest::setBaseLayerStyleAnimatorInstanceInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(const Configuration& configuration): BaseLayer::Shared{configuration} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } sharedNoDynamicStyles{BaseLayer::Shared::Configuration{3, 5}},
      shared{BaseLayer::Shared::Configuration{3, 5}
        .setDynamicStyleCount(1)};

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    };

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate},
      uiInstanceAlreadySet{NoCreate},
      uiNoBaseLayer{NoCreate},
      uiNoDynamicStyles{NoCreate};
    ui.setBaseLayerInstance(Containers::pointer<Layer>(ui.createLayer(), shared));
    uiInstanceAlreadySet.setBaseLayerInstance(Containers::pointer<Layer>(uiInstanceAlreadySet.createLayer(), shared));
    uiInstanceAlreadySet.setBaseLayerStyleAnimatorInstance(Containers::pointer<BaseLayerStyleAnimator>(uiInstanceAlreadySet.createAnimator()));
    uiNoDynamicStyles.setBaseLayerInstance(Containers::pointer<Layer>(uiNoDynamicStyles.createLayer(), sharedNoDynamicStyles));

    BaseLayer& anotherLayer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), shared));

    Containers::Pointer<BaseLayerStyleAnimator> alreadyAssigned{InPlaceInit, animatorHandle(0, 1)};
    anotherLayer.assignAnimator(*alreadyAssigned);

    Containers::String out;
    Error redirectError{&out};
    ui.setBaseLayerStyleAnimatorInstance(nullptr);
    uiInstanceAlreadySet.setBaseLayerStyleAnimatorInstance(Containers::pointer<BaseLayerStyleAnimator>(ui.createAnimator()));
    uiNoBaseLayer.setBaseLayerStyleAnimatorInstance(Containers::pointer<BaseLayerStyleAnimator>(uiNoBaseLayer.createAnimator()));
    uiNoDynamicStyles.setBaseLayerStyleAnimatorInstance(Containers::pointer<BaseLayerStyleAnimator>(uiNoDynamicStyles.createAnimator()));
    ui.setBaseLayerStyleAnimatorInstance(Utility::move(alreadyAssigned));
    CORRADE_COMPARE_AS(out,
        "Ui::UserInterface::setBaseLayerStyleAnimatorInstance(): instance is null\n"
        "Ui::UserInterface::setBaseLayerStyleAnimatorInstance(): instance already set\n"
        "Ui::UserInterface::setBaseLayerStyleAnimatorInstance(): base layer instance not set\n"
        "Ui::UserInterface::setBaseLayerStyleAnimatorInstance(): can't animate a base layer with zero dynamic styles\n"
        "Ui::UserInterface::setBaseLayerStyleAnimatorInstance(): instance already assigned to Ui::LayerHandle(0x1, 0x1)\n",
        TestSuite::Compare::String);
}

void UserInterfaceTest::baseLayerStyleAnimatorInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    CORRADE_VERIFY(!ui.hasBaseLayerStyleAnimator());

    Containers::String out;
    Error redirectError{&out};
    ui.baseLayerStyleAnimator();
    CORRADE_COMPARE(out, "Ui::UserInterface::baseLayerStyleAnimator(): no instance set\n");
}

void UserInterfaceTest::setTextLayerInstance() {
    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32}};

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{1, 3}};

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    };

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    CORRADE_COMPARE(ui.layerCapacity(), 0);
    CORRADE_COMPARE(ui.layerUsedCount(), 0);
    CORRADE_VERIFY(!ui.hasBaseLayer());
    CORRADE_VERIFY(!ui.hasTextLayer());
    CORRADE_VERIFY(!ui.hasEventLayer());

    LayerHandle handle = ui.createLayer();
    Containers::Pointer<Layer> layer{InPlaceInit, handle, shared};
    Layer* pointer = layer.get();
    ui.setTextLayerInstance(Utility::move(layer));
    CORRADE_COMPARE(ui.layerCapacity(), 1);
    CORRADE_COMPARE(ui.layerUsedCount(), 1);
    CORRADE_VERIFY(!ui.hasBaseLayer());
    CORRADE_VERIFY(ui.hasTextLayer());
    CORRADE_VERIFY(!ui.hasEventLayer());
    CORRADE_COMPARE(&ui.layer(handle), pointer);
    CORRADE_COMPARE(&ui.textLayer(), pointer);
}

void UserInterfaceTest::setTextLayerInstanceInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32}};

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{3, 5}};

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    };

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    ui.setTextLayerInstance(Containers::pointer<Layer>(ui.createLayer(), shared));

    Containers::String out;
    Error redirectError{&out};
    ui.setTextLayerInstance(nullptr);
    ui.setTextLayerInstance(Containers::pointer<Layer>(ui.createLayer(), shared));
    CORRADE_COMPARE(out,
        "Ui::UserInterface::setTextLayerInstance(): instance is null\n"
        "Ui::UserInterface::setTextLayerInstance(): instance already set\n");
}

void UserInterfaceTest::textLayerInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};

    Containers::String out;
    Error redirectError{&out};
    ui.textLayer();
    CORRADE_COMPARE(out, "Ui::UserInterface::textLayer(): no instance set\n");
}

void UserInterfaceTest::setTextLayerStyleAnimatorInstance() {
    auto&& data = SetStyleAnimatorInstanceData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32}};

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{1, 3}
        .setDynamicStyleCount(1)
    };

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    };

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    CORRADE_COMPARE(ui.animatorCapacity(), 0);
    CORRADE_COMPARE(ui.animatorUsedCount(), 0);
    CORRADE_VERIFY(!ui.hasBaseLayerStyleAnimator());
    CORRADE_VERIFY(!ui.hasTextLayerStyleAnimator());

    ui.setTextLayerInstance(Containers::pointer<Layer>(ui.createLayer(), shared));

    TextLayerStyleAnimator anotherAnimator{animatorHandle(0, 1)};
    if(data.defaultAnimatorAlreadyExists) {
        ui.textLayer().assignAnimator(anotherAnimator);
        ui.textLayer().setDefaultStyleAnimator(&anotherAnimator);
    }

    AnimatorHandle handle = ui.createAnimator();
    Containers::Pointer<TextLayerStyleAnimator> animator{InPlaceInit, handle};
    TextLayerStyleAnimator* pointer = animator.get();
    ui.setTextLayerStyleAnimatorInstance(Utility::move(animator));
    CORRADE_COMPARE(ui.animatorCapacity(), 1);
    CORRADE_COMPARE(ui.animatorUsedCount(), 1);
    CORRADE_VERIFY(!ui.hasBaseLayerStyleAnimator());
    CORRADE_VERIFY(ui.hasTextLayerStyleAnimator());
    CORRADE_COMPARE(&ui.animator(handle), pointer);
    CORRADE_COMPARE(&ui.textLayerStyleAnimator(), pointer);
    /* The default animator gets set even if it already exists */
    CORRADE_COMPARE(ui.textLayer().defaultStyleAnimator(), pointer);
}

void UserInterfaceTest::setTextLayerStyleAnimatorInstanceInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32}};

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } sharedNoDynamicStyles{cache, TextLayer::Shared::Configuration{1, 3}},
      shared{cache, TextLayer::Shared::Configuration{1, 3}
        .setDynamicStyleCount(1)};

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    };

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate},
      uiInstanceAlreadySet{NoCreate},
      uiNoTextLayer{NoCreate},
      uiNoDynamicStyles{NoCreate};
    ui.setTextLayerInstance(Containers::pointer<Layer>(ui.createLayer(), shared));
    uiInstanceAlreadySet.setTextLayerInstance(Containers::pointer<Layer>(uiInstanceAlreadySet.createLayer(), shared));
    uiInstanceAlreadySet.setTextLayerStyleAnimatorInstance(Containers::pointer<TextLayerStyleAnimator>(uiInstanceAlreadySet.createAnimator()));
    uiNoDynamicStyles.setTextLayerInstance(Containers::pointer<Layer>(uiNoDynamicStyles.createLayer(), sharedNoDynamicStyles));

    TextLayer& anotherLayer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), shared));

    Containers::Pointer<TextLayerStyleAnimator> alreadyAssigned{InPlaceInit, animatorHandle(0, 1)};
    anotherLayer.assignAnimator(*alreadyAssigned);

    Containers::String out;
    Error redirectError{&out};
    ui.setTextLayerStyleAnimatorInstance(nullptr);
    uiInstanceAlreadySet.setTextLayerStyleAnimatorInstance(Containers::pointer<TextLayerStyleAnimator>(ui.createAnimator()));
    uiNoTextLayer.setTextLayerStyleAnimatorInstance(Containers::pointer<TextLayerStyleAnimator>(ui.createAnimator()));
    uiNoDynamicStyles.setTextLayerStyleAnimatorInstance(Containers::pointer<TextLayerStyleAnimator>(uiNoDynamicStyles.createAnimator()));
    ui.setTextLayerStyleAnimatorInstance(Utility::move(alreadyAssigned));
    CORRADE_COMPARE_AS(out,
        "Ui::UserInterface::setTextLayerStyleAnimatorInstance(): instance is null\n"
        "Ui::UserInterface::setTextLayerStyleAnimatorInstance(): instance already set\n"
        "Ui::UserInterface::setTextLayerStyleAnimatorInstance(): text layer instance not set\n"
        "Ui::UserInterface::setTextLayerStyleAnimatorInstance(): can't animate a text layer with zero dynamic styles\n"
        "Ui::UserInterface::setTextLayerStyleAnimatorInstance(): instance already assigned to Ui::LayerHandle(0x1, 0x1)\n",
        TestSuite::Compare::String);
}

void UserInterfaceTest::textLayerStyleAnimatorInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    CORRADE_VERIFY(!ui.hasTextLayerStyleAnimator());

    Containers::String out;
    Error redirectError{&out};
    ui.textLayerStyleAnimator();
    CORRADE_COMPARE(out, "Ui::UserInterface::textLayerStyleAnimator(): no instance set\n");
}

void UserInterfaceTest::setEventLayerInstance() {
    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    CORRADE_COMPARE(ui.layerCapacity(), 0);
    CORRADE_COMPARE(ui.layerUsedCount(), 0);
    CORRADE_VERIFY(!ui.hasBaseLayer());
    CORRADE_VERIFY(!ui.hasTextLayer());
    CORRADE_VERIFY(!ui.hasEventLayer());

    LayerHandle handle = ui.createLayer();
    Containers::Pointer<EventLayer> layer{InPlaceInit, handle};
    EventLayer* pointer = layer.get();
    ui.setEventLayerInstance(Utility::move(layer));
    CORRADE_COMPARE(ui.layerCapacity(), 1);
    CORRADE_COMPARE(ui.layerUsedCount(), 1);
    CORRADE_VERIFY(!ui.hasBaseLayer());
    CORRADE_VERIFY(!ui.hasTextLayer());
    CORRADE_VERIFY(ui.hasEventLayer());
    CORRADE_COMPARE(&ui.layer(handle), pointer);
    CORRADE_COMPARE(&ui.eventLayer(), pointer);
}

void UserInterfaceTest::setEventLayerInstanceInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    ui.setEventLayerInstance(Containers::pointer<EventLayer>(ui.createLayer()));

    Containers::String out;
    Error redirectError{&out};
    ui.setEventLayerInstance(nullptr);
    ui.setEventLayerInstance(Containers::pointer<EventLayer>(ui.createLayer()));
    CORRADE_COMPARE(out,
        "Ui::UserInterface::setEventLayerInstance(): instance is null\n"
        "Ui::UserInterface::setEventLayerInstance(): instance already set\n");
}

void UserInterfaceTest::eventLayerInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};

    Containers::String out;
    Error redirectError{&out};
    ui.eventLayer();
    CORRADE_COMPARE(out, "Ui::UserInterface::eventLayer(): no instance set\n");
}

void UserInterfaceTest::setSnapLayouterInstance() {
    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    CORRADE_COMPARE(ui.layouterCapacity(), 0);
    CORRADE_COMPARE(ui.layouterUsedCount(), 0);
    CORRADE_VERIFY(!ui.hasSnapLayouter());

    LayouterHandle handle = ui.createLayouter();
    Containers::Pointer<SnapLayouter> layouter{InPlaceInit, handle};
    SnapLayouter* pointer = layouter.get();
    ui.setSnapLayouterInstance(Utility::move(layouter));
    CORRADE_COMPARE(ui.layouterCapacity(), 1);
    CORRADE_COMPARE(ui.layouterUsedCount(), 1);
    CORRADE_VERIFY(ui.hasSnapLayouter());
    CORRADE_COMPARE(&ui.layouter(handle), pointer);
    CORRADE_COMPARE(&ui.snapLayouter(), pointer);
}

void UserInterfaceTest::setSnapLayouterInstanceInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    ui.setSnapLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));

    Containers::String out;
    Error redirectError{&out};
    ui.setSnapLayouterInstance(nullptr);
    ui.setSnapLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));
    CORRADE_COMPARE(out,
        "Ui::UserInterface::setSnapLayouterInstance(): instance is null\n"
        "Ui::UserInterface::setSnapLayouterInstance(): instance already set\n");
}

void UserInterfaceTest::snapLayouterInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};

    Containers::String out;
    Error redirectError{&out};
    ui.snapLayouter();
    CORRADE_COMPARE(out, "Ui::UserInterface::snapLayouter(): no instance set\n");
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::UserInterfaceTest)
