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
#include "Magnum/Ui/GenericLayouter.h"
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/LayoutLayer.h"
#include "Magnum/Ui/NodeAnimator.h"
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

    void setBackgroundLayerInstance();
    void setBackgroundLayerInstanceInvalid();
    void backgroundLayerBaseFallback();
    void backgroundLayerInvalid();

    void setBackgroundLayerStyleAnimatorInstance();
    void setBackgroundLayerStyleAnimatorInstanceInvalid();
    void backgroundLayerStyleAnimatorBaseFallback();
    void backgroundLayerStyleAnimatorInvalid();

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

    void setLayoutLayerInstance();
    void setLayoutLayerInstanceInvalid();
    void layoutLayerInvalid();

    void setSnapLayouterInstance();
    void setSnapLayouterInstanceInvalid();
    void snapLayouterInvalid();

    void setGenericLayouterInstance();
    void setGenericLayouterInstanceInvalid();
    void genericLayouterInvalid();

    void setNodeAnimatorInstance();
    void setNodeAnimatorInstanceInvalid();
    void nodeAnimatorInvalid();
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

              &UserInterfaceTest::setBackgroundLayerInstance,
              &UserInterfaceTest::setBackgroundLayerInstanceInvalid,
              &UserInterfaceTest::backgroundLayerBaseFallback,
              &UserInterfaceTest::backgroundLayerInvalid});

    addInstancedTests({&UserInterfaceTest::setBackgroundLayerStyleAnimatorInstance},
        Containers::arraySize(SetStyleAnimatorInstanceData));

    addTests({&UserInterfaceTest::setBackgroundLayerStyleAnimatorInstanceInvalid,
              &UserInterfaceTest::backgroundLayerStyleAnimatorBaseFallback,
              &UserInterfaceTest::backgroundLayerStyleAnimatorInvalid,

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

              &UserInterfaceTest::setLayoutLayerInstance,
              &UserInterfaceTest::setLayoutLayerInstanceInvalid,
              &UserInterfaceTest::layoutLayerInvalid,

              &UserInterfaceTest::setSnapLayouterInstance,
              &UserInterfaceTest::setSnapLayouterInstanceInvalid,
              &UserInterfaceTest::snapLayouterInvalid,

              &UserInterfaceTest::setGenericLayouterInstance,
              &UserInterfaceTest::setGenericLayouterInstanceInvalid,
              &UserInterfaceTest::genericLayouterInvalid,

              &UserInterfaceTest::setNodeAnimatorInstance,
              &UserInterfaceTest::setNodeAnimatorInstanceInvalid,
              &UserInterfaceTest::nodeAnimatorInvalid});
}

void UserInterfaceTest::construct() {
    struct Interface: UserInterface {
        explicit Interface(const Vector2i& size): UserInterface{NoCreate} {
            setSize(size);
        }
    } ui{{100, 150}};

    CORRADE_COMPARE(ui.size(), (Vector2{100.0f, 150.0f}));
    CORRADE_VERIFY(!ui.hasRendererInstance());
    CORRADE_COMPARE(ui.layerCapacity(), 0);
    CORRADE_COMPARE(ui.layerUsedCount(), 0);
    CORRADE_COMPARE(ui.layouterCapacity(), 0);
    CORRADE_COMPARE(ui.layouterUsedCount(), 0);
    CORRADE_COMPARE(ui.animatorCapacity(), 0);
    CORRADE_COMPARE(ui.animatorUsedCount(), 0);
    CORRADE_VERIFY(!ui.hasBackgroundLayer());
    CORRADE_VERIFY(!ui.hasBackgroundLayerStyleAnimator());
    CORRADE_VERIFY(!ui.hasBaseLayer());
    CORRADE_VERIFY(!ui.hasBaseLayerStyleAnimator());
    CORRADE_VERIFY(!ui.hasTextLayer());
    CORRADE_VERIFY(!ui.hasTextLayerStyleAnimator());
    CORRADE_VERIFY(!ui.hasEventLayer());
    CORRADE_VERIFY(!ui.hasLayoutLayer());
    CORRADE_VERIFY(!ui.hasSnapLayouter());
    CORRADE_VERIFY(!ui.hasGenericLayouter());
    CORRADE_VERIFY(!ui.hasNodeAnimator());
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

void UserInterfaceTest::setBackgroundLayerInstance() {
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
    const UserInterface& cui = ui;
    CORRADE_COMPARE(ui.layerCapacity(), 0);
    CORRADE_COMPARE(ui.layerUsedCount(), 0);
    CORRADE_VERIFY(!ui.hasBackgroundLayer());
    CORRADE_VERIFY(!ui.hasBaseLayer());
    CORRADE_VERIFY(!ui.hasTextLayer());
    CORRADE_VERIFY(!ui.hasEventLayer());
    CORRADE_VERIFY(!ui.hasLayoutLayer());

    LayerHandle handle = ui.createLayer();
    Containers::Pointer<Layer> layer{InPlaceInit, handle, shared};
    Layer* pointer = layer.get();
    ui.setBackgroundLayerInstance(Utility::move(layer));
    CORRADE_COMPARE(ui.layerCapacity(), 1);
    CORRADE_COMPARE(ui.layerUsedCount(), 1);
    CORRADE_VERIFY(ui.hasBackgroundLayer());
    CORRADE_VERIFY(!ui.hasBaseLayer());
    CORRADE_VERIFY(!ui.hasTextLayer());
    CORRADE_VERIFY(!ui.hasEventLayer());
    CORRADE_VERIFY(!ui.hasLayoutLayer());
    CORRADE_COMPARE(&ui.layer(handle), pointer);
    CORRADE_COMPARE(&ui.backgroundLayer(), pointer);
    CORRADE_COMPARE(&cui.backgroundLayer(), pointer);
}

void UserInterfaceTest::setBackgroundLayerInstanceInvalid() {
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
    ui.setBackgroundLayerInstance(Containers::pointer<Layer>(ui.createLayer(), shared));

    Containers::String out;
    Error redirectError{&out};
    ui.setBackgroundLayerInstance(nullptr);
    ui.setBackgroundLayerInstance(Containers::pointer<Layer>(ui.createLayer(), shared));
    CORRADE_COMPARE_AS(out,
        "Ui::UserInterface::setBackgroundLayerInstance(): instance is null\n"
        "Ui::UserInterface::setBackgroundLayerInstance(): instance already set\n",
        TestSuite::Compare::String);
}

void UserInterfaceTest::backgroundLayerBaseFallback() {
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
    CORRADE_VERIFY(ui.hasBaseLayer());

    /* The base layer is available through backgroundLayer() as well, even
       though hasBackgroundLayer() returns false */
    BaseLayer& baseLayer = ui.baseLayer();
    CORRADE_VERIFY(!ui.hasBackgroundLayer());
    CORRADE_COMPARE(&ui.backgroundLayer(), &baseLayer);

    /* Setting a background layer instance with base layer present shouldn't
       fail, and should replace the instance available through
       backgroundLayer() */
    Containers::Pointer<Layer> layer{InPlaceInit, ui.createLayer(), shared};
    Layer* pointer = layer.get();
    ui.setBackgroundLayerInstance(Utility::move(layer));
    CORRADE_VERIFY(ui.hasBackgroundLayer());
    CORRADE_COMPARE(&ui.baseLayer(), &baseLayer);
    CORRADE_COMPARE(&ui.backgroundLayer(), pointer);

    /* Going in the other direction, setting base layer *after* background
       layer shouldn't overwrite the pointer eiter */
    Interface uiWithBackgroundLayer{NoCreate};
    uiWithBackgroundLayer.setBackgroundLayerInstance(Containers::pointer<Layer>(uiWithBackgroundLayer.createLayer(), shared));
    BaseLayer& backgroundLayer = uiWithBackgroundLayer.backgroundLayer();
    uiWithBackgroundLayer.setBaseLayerInstance(Containers::pointer<Layer>(uiWithBackgroundLayer.createLayer(), shared));
    CORRADE_VERIFY(uiWithBackgroundLayer.hasBackgroundLayer());
    CORRADE_VERIFY(uiWithBackgroundLayer.hasBaseLayer());
    CORRADE_COMPARE(&uiWithBackgroundLayer.backgroundLayer(), &backgroundLayer);
    CORRADE_VERIFY(&uiWithBackgroundLayer.baseLayer() != &backgroundLayer);
}

void UserInterfaceTest::backgroundLayerInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    CORRADE_VERIFY(!ui.hasBackgroundLayer());

    Containers::String out;
    Error redirectError{&out};
    ui.backgroundLayer();
    CORRADE_COMPARE(out, "Ui::UserInterface::backgroundLayer(): no instance set\n");
}

void UserInterfaceTest::setBackgroundLayerStyleAnimatorInstance() {
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
    const UserInterface& cui = ui;
    CORRADE_COMPARE(ui.animatorCapacity(), 0);
    CORRADE_COMPARE(ui.animatorUsedCount(), 0);
    CORRADE_VERIFY(!ui.hasBackgroundLayerStyleAnimator());
    CORRADE_VERIFY(!ui.hasBaseLayerStyleAnimator());
    CORRADE_VERIFY(!ui.hasTextLayerStyleAnimator());
    CORRADE_VERIFY(!ui.hasNodeAnimator());

    ui.setBackgroundLayerInstance(Containers::pointer<Layer>(ui.createLayer(), shared));

    BaseLayerStyleAnimator anotherAnimator{animatorHandle(0, 1)};
    if(data.defaultAnimatorAlreadyExists) {
        ui.backgroundLayer().assignAnimator(anotherAnimator);
        ui.backgroundLayer().setDefaultStyleAnimator(&anotherAnimator);
    }

    AnimatorHandle handle = ui.createAnimator();
    Containers::Pointer<BaseLayerStyleAnimator> animator{InPlaceInit, handle};
    BaseLayerStyleAnimator* pointer = animator.get();
    ui.setBackgroundLayerStyleAnimatorInstance(Utility::move(animator));
    CORRADE_COMPARE(ui.animatorCapacity(), 1);
    CORRADE_COMPARE(ui.animatorUsedCount(), 1);
    CORRADE_VERIFY(ui.hasBackgroundLayerStyleAnimator());
    CORRADE_VERIFY(!ui.hasBaseLayerStyleAnimator());
    CORRADE_VERIFY(!ui.hasTextLayerStyleAnimator());
    CORRADE_VERIFY(!ui.hasNodeAnimator());
    CORRADE_COMPARE(&ui.animator(handle), pointer);
    CORRADE_COMPARE(&ui.backgroundLayerStyleAnimator(), pointer);
    CORRADE_COMPARE(&cui.backgroundLayerStyleAnimator(), pointer);
    /* The default animator gets set even if it already exists */
    CORRADE_COMPARE(ui.backgroundLayer().defaultStyleAnimator(), pointer);
}

void UserInterfaceTest::setBackgroundLayerStyleAnimatorInstanceInvalid() {
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
    ui.setBackgroundLayerInstance(Containers::pointer<Layer>(ui.createLayer(), shared));
    uiInstanceAlreadySet.setBackgroundLayerInstance(Containers::pointer<Layer>(uiInstanceAlreadySet.createLayer(), shared));
    uiInstanceAlreadySet.setBackgroundLayerStyleAnimatorInstance(Containers::pointer<BaseLayerStyleAnimator>(uiInstanceAlreadySet.createAnimator()));
    uiNoDynamicStyles.setBackgroundLayerInstance(Containers::pointer<Layer>(uiNoDynamicStyles.createLayer(), sharedNoDynamicStyles));

    BaseLayer& anotherLayer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), shared));

    Containers::Pointer<BaseLayerStyleAnimator> alreadyAssigned{InPlaceInit, animatorHandle(0, 1)};
    anotherLayer.assignAnimator(*alreadyAssigned);

    Containers::String out;
    Error redirectError{&out};
    ui.setBackgroundLayerStyleAnimatorInstance(nullptr);
    uiInstanceAlreadySet.setBackgroundLayerStyleAnimatorInstance(Containers::pointer<BaseLayerStyleAnimator>(ui.createAnimator()));
    uiNoBaseLayer.setBackgroundLayerStyleAnimatorInstance(Containers::pointer<BaseLayerStyleAnimator>(uiNoBaseLayer.createAnimator()));
    uiNoDynamicStyles.setBackgroundLayerStyleAnimatorInstance(Containers::pointer<BaseLayerStyleAnimator>(uiNoDynamicStyles.createAnimator()));
    ui.setBackgroundLayerStyleAnimatorInstance(Utility::move(alreadyAssigned));
    CORRADE_COMPARE_AS(out,
        "Ui::UserInterface::setBackgroundLayerStyleAnimatorInstance(): instance is null\n"
        "Ui::UserInterface::setBackgroundLayerStyleAnimatorInstance(): instance already set\n"
        "Ui::UserInterface::setBackgroundLayerStyleAnimatorInstance(): background layer instance not set\n"
        "Ui::UserInterface::setBackgroundLayerStyleAnimatorInstance(): can't animate a background layer with zero dynamic styles\n"
        "Ui::UserInterface::setBackgroundLayerStyleAnimatorInstance(): instance already assigned to Ui::LayerHandle(0x1, 0x1)\n",
        TestSuite::Compare::String);
}

void UserInterfaceTest::backgroundLayerStyleAnimatorBaseFallback() {
    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(const Configuration& configuration): BaseLayer::Shared{configuration} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } shared{BaseLayer::Shared::Configuration{3, 5}
        .setDynamicStyleCount(1)};

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    };

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    ui.setBaseLayerInstance(Containers::pointer<Layer>(ui.createLayer(), shared));
    ui.setBaseLayerStyleAnimatorInstance(Containers::pointer<BaseLayerStyleAnimator>(ui.createAnimator()));;
    CORRADE_VERIFY(ui.hasBaseLayer());
    CORRADE_VERIFY(ui.hasBaseLayerStyleAnimator());

    /* The base layer style animator is available through
       backgroundLayerStyleAnimator() as well, even though
       hasBackgroundLayerStyleAnimator() returns false */
    BaseLayerStyleAnimator& baseLayerStyleAnimator = ui.baseLayerStyleAnimator();
    CORRADE_VERIFY(!ui.hasBackgroundLayerStyleAnimator());
    CORRADE_COMPARE(&ui.backgroundLayerStyleAnimator(), &baseLayerStyleAnimator);

    /* Setting a background layer style animator instance with base layer and
       base layer animator present shouldn't fail, and should replace the
       instance available through backgroundLayerStyleAnimator() */
    ui.setBackgroundLayerInstance(Containers::pointer<Layer>(ui.createLayer(), shared));
    Containers::Pointer<BaseLayerStyleAnimator> animator{InPlaceInit, ui.createAnimator()};
    BaseLayerStyleAnimator* pointer = animator.get();
    ui.setBackgroundLayerStyleAnimatorInstance(Utility::move(animator));
    CORRADE_VERIFY(ui.hasBackgroundLayerStyleAnimator());
    CORRADE_COMPARE(&ui.baseLayerStyleAnimator(), &baseLayerStyleAnimator);
    CORRADE_COMPARE(&ui.backgroundLayerStyleAnimator(), pointer);

    /* Going in the other direction, setting base layer style animator *after*
       background layer style animator shouldn't overwrite the pointer eiter */
    Interface uiWithBackgroundLayerStyleAnimator{NoCreate};
    uiWithBackgroundLayerStyleAnimator.setBackgroundLayerInstance(Containers::pointer<Layer>(uiWithBackgroundLayerStyleAnimator.createLayer(), shared));
    uiWithBackgroundLayerStyleAnimator.setBackgroundLayerStyleAnimatorInstance(Containers::pointer<BaseLayerStyleAnimator>(uiWithBackgroundLayerStyleAnimator.createAnimator()));
    BaseLayerStyleAnimator& backgroundLayerStyleAnimator = uiWithBackgroundLayerStyleAnimator.backgroundLayerStyleAnimator();
    uiWithBackgroundLayerStyleAnimator.setBaseLayerInstance(Containers::pointer<Layer>(uiWithBackgroundLayerStyleAnimator.createLayer(), shared));
    uiWithBackgroundLayerStyleAnimator.setBaseLayerStyleAnimatorInstance(Containers::pointer<BaseLayerStyleAnimator>(uiWithBackgroundLayerStyleAnimator.createAnimator()));
    CORRADE_VERIFY(uiWithBackgroundLayerStyleAnimator.hasBackgroundLayerStyleAnimator());
    CORRADE_VERIFY(uiWithBackgroundLayerStyleAnimator.hasBaseLayerStyleAnimator());
    CORRADE_COMPARE(&uiWithBackgroundLayerStyleAnimator.backgroundLayerStyleAnimator(), &backgroundLayerStyleAnimator);
    CORRADE_VERIFY(&uiWithBackgroundLayerStyleAnimator.baseLayerStyleAnimator() != &backgroundLayerStyleAnimator);
}

void UserInterfaceTest::backgroundLayerStyleAnimatorInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(const Configuration& configuration): BaseLayer::Shared{configuration} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } shared{BaseLayer::Shared::Configuration{3, 5}
        .setDynamicStyleCount(1)};

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    };

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate},
      uiWithBaseLayerButNoAnimator{NoCreate},
      uiWithBaseLayerAnimatorAndBackgroundLayerButNoBackgroundAnimator{NoCreate};
    uiWithBaseLayerButNoAnimator.setBaseLayerInstance(Containers::pointer<Layer>(uiWithBaseLayerButNoAnimator.createLayer(), shared));
    uiWithBaseLayerAnimatorAndBackgroundLayerButNoBackgroundAnimator.setBaseLayerInstance(Containers::pointer<Layer>(uiWithBaseLayerAnimatorAndBackgroundLayerButNoBackgroundAnimator.createLayer(), shared));
    uiWithBaseLayerAnimatorAndBackgroundLayerButNoBackgroundAnimator.setBaseLayerStyleAnimatorInstance(Containers::pointer<BaseLayerStyleAnimator>(uiWithBaseLayerAnimatorAndBackgroundLayerButNoBackgroundAnimator.createAnimator()));
    uiWithBaseLayerAnimatorAndBackgroundLayerButNoBackgroundAnimator.setBackgroundLayerInstance(Containers::pointer<Layer>(uiWithBaseLayerAnimatorAndBackgroundLayerButNoBackgroundAnimator.createLayer(), shared));

    /* Obvious cases */
    CORRADE_VERIFY(!ui.hasBackgroundLayerStyleAnimator());
    CORRADE_VERIFY(!uiWithBaseLayerButNoAnimator.hasBackgroundLayerStyleAnimator());

    /* If there's base layer with an animator and also a background layer but
       with no animator, it should fail (and assert) as well */
    CORRADE_VERIFY(!uiWithBaseLayerButNoAnimator.hasBackgroundLayerStyleAnimator());
    CORRADE_VERIFY(uiWithBaseLayerAnimatorAndBackgroundLayerButNoBackgroundAnimator.hasBaseLayer());
    CORRADE_VERIFY(uiWithBaseLayerAnimatorAndBackgroundLayerButNoBackgroundAnimator.hasBaseLayerStyleAnimator());
    CORRADE_VERIFY(uiWithBaseLayerAnimatorAndBackgroundLayerButNoBackgroundAnimator.hasBackgroundLayer());
    CORRADE_VERIFY(!uiWithBaseLayerAnimatorAndBackgroundLayerButNoBackgroundAnimator.hasBackgroundLayerStyleAnimator());

    /* If there's a background layer without an animator, and *then* a base
       layer animator is set, it shouldn't make the base layer animator act as
       if it belongs to the background layer */
    Interface uiWithBackgroundLayer{NoCreate};
    uiWithBackgroundLayer.setBackgroundLayerInstance(Containers::pointer<Layer>(uiWithBackgroundLayer.createLayer(), shared));
    uiWithBackgroundLayer.setBaseLayerInstance(Containers::pointer<Layer>(uiWithBackgroundLayer.createLayer(), shared));
    /* It's important that this is called *after* setBackgroundLayerInstance(),
       not before, to verify the edge case */
    uiWithBackgroundLayer.setBaseLayerStyleAnimatorInstance(Containers::pointer<BaseLayerStyleAnimator>(uiWithBackgroundLayer.createAnimator()));
    CORRADE_VERIFY(uiWithBackgroundLayer.hasBaseLayerStyleAnimator());
    CORRADE_VERIFY(!uiWithBackgroundLayer.hasBackgroundLayerStyleAnimator());

    Containers::String out;
    Error redirectError{&out};
    ui.backgroundLayerStyleAnimator();
    uiWithBaseLayerButNoAnimator.backgroundLayerStyleAnimator();
    uiWithBaseLayerAnimatorAndBackgroundLayerButNoBackgroundAnimator.backgroundLayerStyleAnimator();
    uiWithBackgroundLayer.backgroundLayerStyleAnimator();
    CORRADE_COMPARE_AS(out,
        "Ui::UserInterface::backgroundLayerStyleAnimator(): no instance set\n"
        "Ui::UserInterface::backgroundLayerStyleAnimator(): no instance set\n"
        "Ui::UserInterface::backgroundLayerStyleAnimator(): no instance set\n"
        "Ui::UserInterface::backgroundLayerStyleAnimator(): no instance set\n",
        TestSuite::Compare::String);
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
    const UserInterface& cui = ui;
    CORRADE_COMPARE(ui.layerCapacity(), 0);
    CORRADE_COMPARE(ui.layerUsedCount(), 0);
    CORRADE_VERIFY(!ui.hasBackgroundLayer());
    CORRADE_VERIFY(!ui.hasBaseLayer());
    CORRADE_VERIFY(!ui.hasTextLayer());
    CORRADE_VERIFY(!ui.hasEventLayer());
    CORRADE_VERIFY(!ui.hasLayoutLayer());

    LayerHandle handle = ui.createLayer();
    Containers::Pointer<Layer> layer{InPlaceInit, handle, shared};
    Layer* pointer = layer.get();
    ui.setBaseLayerInstance(Utility::move(layer));
    CORRADE_COMPARE(ui.layerCapacity(), 1);
    CORRADE_COMPARE(ui.layerUsedCount(), 1);
    CORRADE_VERIFY(!ui.hasBackgroundLayer());
    CORRADE_VERIFY(ui.hasBaseLayer());
    CORRADE_VERIFY(!ui.hasTextLayer());
    CORRADE_VERIFY(!ui.hasEventLayer());
    CORRADE_VERIFY(!ui.hasLayoutLayer());
    CORRADE_COMPARE(&ui.layer(handle), pointer);
    CORRADE_COMPARE(&ui.baseLayer(), pointer);
    CORRADE_COMPARE(&cui.baseLayer(), pointer);
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
    CORRADE_COMPARE_AS(out,
        "Ui::UserInterface::setBaseLayerInstance(): instance is null\n"
        "Ui::UserInterface::setBaseLayerInstance(): instance already set\n",
        TestSuite::Compare::String);
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
    const UserInterface& cui = ui;
    CORRADE_COMPARE(ui.animatorCapacity(), 0);
    CORRADE_COMPARE(ui.animatorUsedCount(), 0);
    CORRADE_VERIFY(!ui.hasBackgroundLayerStyleAnimator());
    CORRADE_VERIFY(!ui.hasBaseLayerStyleAnimator());
    CORRADE_VERIFY(!ui.hasTextLayerStyleAnimator());
    CORRADE_VERIFY(!ui.hasNodeAnimator());

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
    CORRADE_VERIFY(!ui.hasBackgroundLayerStyleAnimator());
    CORRADE_VERIFY(ui.hasBaseLayerStyleAnimator());
    CORRADE_VERIFY(!ui.hasTextLayerStyleAnimator());
    CORRADE_VERIFY(!ui.hasNodeAnimator());
    CORRADE_COMPARE(&ui.animator(handle), pointer);
    CORRADE_COMPARE(&ui.baseLayerStyleAnimator(), pointer);
    CORRADE_COMPARE(&cui.baseLayerStyleAnimator(), pointer);
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
    const UserInterface& cui = ui;
    CORRADE_COMPARE(ui.layerCapacity(), 0);
    CORRADE_COMPARE(ui.layerUsedCount(), 0);
    CORRADE_VERIFY(!ui.hasBackgroundLayer());
    CORRADE_VERIFY(!ui.hasBaseLayer());
    CORRADE_VERIFY(!ui.hasTextLayer());
    CORRADE_VERIFY(!ui.hasEventLayer());
    CORRADE_VERIFY(!ui.hasLayoutLayer());

    LayerHandle handle = ui.createLayer();
    Containers::Pointer<Layer> layer{InPlaceInit, handle, shared};
    Layer* pointer = layer.get();
    ui.setTextLayerInstance(Utility::move(layer));
    CORRADE_COMPARE(ui.layerCapacity(), 1);
    CORRADE_COMPARE(ui.layerUsedCount(), 1);
    CORRADE_VERIFY(!ui.hasBackgroundLayer());
    CORRADE_VERIFY(!ui.hasBaseLayer());
    CORRADE_VERIFY(ui.hasTextLayer());
    CORRADE_VERIFY(!ui.hasEventLayer());
    CORRADE_VERIFY(!ui.hasLayoutLayer());
    CORRADE_COMPARE(&ui.layer(handle), pointer);
    CORRADE_COMPARE(&ui.textLayer(), pointer);
    CORRADE_COMPARE(&cui.textLayer(), pointer);
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
    CORRADE_COMPARE_AS(out,
        "Ui::UserInterface::setTextLayerInstance(): instance is null\n"
        "Ui::UserInterface::setTextLayerInstance(): instance already set\n",
        TestSuite::Compare::String);
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
    const UserInterface& cui = ui;
    CORRADE_COMPARE(ui.animatorCapacity(), 0);
    CORRADE_COMPARE(ui.animatorUsedCount(), 0);
    CORRADE_VERIFY(!ui.hasBackgroundLayerStyleAnimator());
    CORRADE_VERIFY(!ui.hasBaseLayerStyleAnimator());
    CORRADE_VERIFY(!ui.hasTextLayerStyleAnimator());
    CORRADE_VERIFY(!ui.hasNodeAnimator());

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
    CORRADE_VERIFY(!ui.hasBackgroundLayerStyleAnimator());
    CORRADE_VERIFY(!ui.hasBaseLayerStyleAnimator());
    CORRADE_VERIFY(ui.hasTextLayerStyleAnimator());
    CORRADE_VERIFY(!ui.hasNodeAnimator());
    CORRADE_COMPARE(&ui.animator(handle), pointer);
    CORRADE_COMPARE(&ui.textLayerStyleAnimator(), pointer);
    CORRADE_COMPARE(&cui.textLayerStyleAnimator(), pointer);
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
    const UserInterface& cui = ui;
    CORRADE_COMPARE(ui.layerCapacity(), 0);
    CORRADE_COMPARE(ui.layerUsedCount(), 0);
    CORRADE_VERIFY(!ui.hasBackgroundLayer());
    CORRADE_VERIFY(!ui.hasBaseLayer());
    CORRADE_VERIFY(!ui.hasTextLayer());
    CORRADE_VERIFY(!ui.hasEventLayer());
    CORRADE_VERIFY(!ui.hasLayoutLayer());

    LayerHandle handle = ui.createLayer();
    Containers::Pointer<EventLayer> layer{InPlaceInit, handle};
    EventLayer* pointer = layer.get();
    ui.setEventLayerInstance(Utility::move(layer));
    CORRADE_COMPARE(ui.layerCapacity(), 1);
    CORRADE_COMPARE(ui.layerUsedCount(), 1);
    CORRADE_VERIFY(!ui.hasBackgroundLayer());
    CORRADE_VERIFY(!ui.hasBaseLayer());
    CORRADE_VERIFY(!ui.hasTextLayer());
    CORRADE_VERIFY(ui.hasEventLayer());
    CORRADE_VERIFY(!ui.hasLayoutLayer());
    CORRADE_COMPARE(&ui.layer(handle), pointer);
    CORRADE_COMPARE(&ui.eventLayer(), pointer);
    CORRADE_COMPARE(&cui.eventLayer(), pointer);
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
    CORRADE_COMPARE_AS(out,
        "Ui::UserInterface::setEventLayerInstance(): instance is null\n"
        "Ui::UserInterface::setEventLayerInstance(): instance already set\n",
        TestSuite::Compare::String);
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

void UserInterfaceTest::setLayoutLayerInstance() {
    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    const UserInterface& cui = ui;
    CORRADE_COMPARE(ui.layerCapacity(), 0);
    CORRADE_COMPARE(ui.layerUsedCount(), 0);
    CORRADE_VERIFY(!ui.hasBackgroundLayer());
    CORRADE_VERIFY(!ui.hasBaseLayer());
    CORRADE_VERIFY(!ui.hasTextLayer());
    CORRADE_VERIFY(!ui.hasEventLayer());
    CORRADE_VERIFY(!ui.hasLayoutLayer());

    LayerHandle handle = ui.createLayer();
    Containers::Pointer<LayoutLayer> layer{InPlaceInit, handle, 3u};
    LayoutLayer* pointer = layer.get();
    ui.setLayoutLayerInstance(Utility::move(layer));
    CORRADE_COMPARE(ui.layerCapacity(), 1);
    CORRADE_COMPARE(ui.layerUsedCount(), 1);
    CORRADE_VERIFY(!ui.hasBackgroundLayer());
    CORRADE_VERIFY(!ui.hasBaseLayer());
    CORRADE_VERIFY(!ui.hasTextLayer());
    CORRADE_VERIFY(!ui.hasEventLayer());
    CORRADE_VERIFY(ui.hasLayoutLayer());
    CORRADE_COMPARE(&ui.layer(handle), pointer);
    CORRADE_COMPARE(&ui.layoutLayer(), pointer);
    CORRADE_COMPARE(&cui.layoutLayer(), pointer);
}

void UserInterfaceTest::setLayoutLayerInstanceInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    ui.setLayoutLayerInstance(Containers::pointer<LayoutLayer>(ui.createLayer(), 3u));

    Containers::String out;
    Error redirectError{&out};
    ui.setLayoutLayerInstance(nullptr);
    ui.setLayoutLayerInstance(Containers::pointer<LayoutLayer>(ui.createLayer(), 3u));
    CORRADE_COMPARE_AS(out,
        "Ui::UserInterface::setLayoutLayerInstance(): instance is null\n"
        "Ui::UserInterface::setLayoutLayerInstance(): instance already set\n",
        TestSuite::Compare::String);
}

void UserInterfaceTest::layoutLayerInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};

    Containers::String out;
    Error redirectError{&out};
    ui.layoutLayer();
    CORRADE_COMPARE(out, "Ui::UserInterface::layoutLayer(): no instance set\n");
}

void UserInterfaceTest::setSnapLayouterInstance() {
    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    const UserInterface& cui = ui;
    CORRADE_COMPARE(ui.layouterCapacity(), 0);
    CORRADE_COMPARE(ui.layouterUsedCount(), 0);
    CORRADE_VERIFY(!ui.hasSnapLayouter());
    CORRADE_VERIFY(!ui.hasGenericLayouter());

    LayouterHandle handle = ui.createLayouter();
    Containers::Pointer<SnapLayouter> layouter{InPlaceInit, handle};
    SnapLayouter* pointer = layouter.get();
    ui.setSnapLayouterInstance(Utility::move(layouter));
    CORRADE_COMPARE(ui.layouterCapacity(), 1);
    CORRADE_COMPARE(ui.layouterUsedCount(), 1);
    CORRADE_VERIFY(ui.hasSnapLayouter());
    CORRADE_VERIFY(!ui.hasGenericLayouter());
    CORRADE_COMPARE(&ui.layouter(handle), pointer);
    CORRADE_COMPARE(&ui.snapLayouter(), pointer);
    CORRADE_COMPARE(&cui.snapLayouter(), pointer);
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
    CORRADE_COMPARE_AS(out,
        "Ui::UserInterface::setSnapLayouterInstance(): instance is null\n"
        "Ui::UserInterface::setSnapLayouterInstance(): instance already set\n",
        TestSuite::Compare::String);
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

void UserInterfaceTest::setGenericLayouterInstance() {
    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    const UserInterface& cui = ui;
    CORRADE_COMPARE(ui.layouterCapacity(), 0);
    CORRADE_COMPARE(ui.layouterUsedCount(), 0);
    CORRADE_VERIFY(!ui.hasSnapLayouter());
    CORRADE_VERIFY(!ui.hasGenericLayouter());

    LayouterHandle handle = ui.createLayouter();
    Containers::Pointer<GenericLayouter> layouter{InPlaceInit, handle};
    GenericLayouter* pointer = layouter.get();
    ui.setGenericLayouterInstance(Utility::move(layouter));
    CORRADE_COMPARE(ui.layouterCapacity(), 1);
    CORRADE_COMPARE(ui.layouterUsedCount(), 1);
    CORRADE_VERIFY(!ui.hasSnapLayouter());
    CORRADE_VERIFY(ui.hasGenericLayouter());
    CORRADE_COMPARE(&ui.layouter(handle), pointer);
    CORRADE_COMPARE(&ui.genericLayouter(), pointer);
    CORRADE_COMPARE(&cui.genericLayouter(), pointer);
}

void UserInterfaceTest::setGenericLayouterInstanceInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    ui.setGenericLayouterInstance(Containers::pointer<GenericLayouter>(ui.createLayouter()));

    Containers::String out;
    Error redirectError{&out};
    ui.setGenericLayouterInstance(nullptr);
    ui.setGenericLayouterInstance(Containers::pointer<GenericLayouter>(ui.createLayouter()));
    CORRADE_COMPARE_AS(out,
        "Ui::UserInterface::setGenericLayouterInstance(): instance is null\n"
        "Ui::UserInterface::setGenericLayouterInstance(): instance already set\n",
        TestSuite::Compare::String);
}

void UserInterfaceTest::genericLayouterInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};

    Containers::String out;
    Error redirectError{&out};
    ui.genericLayouter();
    CORRADE_COMPARE(out, "Ui::UserInterface::genericLayouter(): no instance set\n");
}

void UserInterfaceTest::setNodeAnimatorInstance() {
    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    const UserInterface& cui = ui;
    CORRADE_COMPARE(ui.layouterCapacity(), 0);
    CORRADE_COMPARE(ui.layouterUsedCount(), 0);
    CORRADE_VERIFY(!ui.hasBackgroundLayerStyleAnimator());
    CORRADE_VERIFY(!ui.hasBaseLayerStyleAnimator());
    CORRADE_VERIFY(!ui.hasTextLayerStyleAnimator());
    CORRADE_VERIFY(!ui.hasNodeAnimator());

    AnimatorHandle handle = ui.createAnimator();
    Containers::Pointer<NodeAnimator> animator{InPlaceInit, handle};
    NodeAnimator* pointer = animator.get();
    ui.setNodeAnimatorInstance(Utility::move(animator));
    CORRADE_COMPARE(ui.animatorCapacity(), 1);
    CORRADE_COMPARE(ui.animatorUsedCount(), 1);
    CORRADE_VERIFY(!ui.hasBackgroundLayerStyleAnimator());
    CORRADE_VERIFY(!ui.hasBaseLayerStyleAnimator());
    CORRADE_VERIFY(!ui.hasTextLayerStyleAnimator());
    CORRADE_VERIFY(ui.hasNodeAnimator());
    CORRADE_COMPARE(&ui.animator(handle), pointer);
    CORRADE_COMPARE(&ui.nodeAnimator(), pointer);
    CORRADE_COMPARE(&cui.nodeAnimator(), pointer);
}

void UserInterfaceTest::setNodeAnimatorInstanceInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    ui.setNodeAnimatorInstance(Containers::pointer<NodeAnimator>(ui.createAnimator()));

    Containers::String out;
    Error redirectError{&out};
    ui.setNodeAnimatorInstance(nullptr);
    ui.setNodeAnimatorInstance(Containers::pointer<NodeAnimator>(ui.createAnimator()));
    CORRADE_COMPARE_AS(out,
        "Ui::UserInterface::setNodeAnimatorInstance(): instance is null\n"
        "Ui::UserInterface::setNodeAnimatorInstance(): instance already set\n",
        TestSuite::Compare::String);
}

void UserInterfaceTest::nodeAnimatorInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};

    Containers::String out;
    Error redirectError{&out};
    ui.nodeAnimator();
    CORRADE_COMPARE(out, "Ui::UserInterface::nodeAnimator(): no instance set\n");
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::UserInterfaceTest)
