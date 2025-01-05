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

#include <sstream> /** @todo remove once Debug is stream-free */
#include <Corrade/TestSuite/Tester.h>
#include <Magnum/Math/Vector2.h>
#include <Corrade/Utility/DebugStl.h> /** @todo remove once Debug is stream-free */

#include "Magnum/Ui/BaseLayer.h"
#include "Magnum/Ui/EventLayer.h"
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/SnapLayouter.h"
#include "Magnum/Ui/TextLayer.h"
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

    void setTextLayerInstance();
    void setTextLayerInstanceInvalid();
    void textLayerInvalid();

    void setEventLayerInstance();
    void setEventLayerInstanceInvalid();
    void eventLayerInvalid();

    void setSnapLayouterInstance();
    void setSnapLayouterInstanceInvalid();
    void snapLayouterInvalid();
};

UserInterfaceTest::UserInterfaceTest() {
    addTests({&UserInterfaceTest::construct,
              &UserInterfaceTest::constructNoCreate,
              &UserInterfaceTest::constructCopy,
              &UserInterfaceTest::constructMove,

              &UserInterfaceTest::setBaseLayerInstance,
              &UserInterfaceTest::setBaseLayerInstanceInvalid,
              &UserInterfaceTest::baseLayerInvalid,

              &UserInterfaceTest::setTextLayerInstance,
              &UserInterfaceTest::setTextLayerInstanceInvalid,
              &UserInterfaceTest::textLayerInvalid,

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

    std::ostringstream out;
    Error redirectError{&out};
    ui.setBaseLayerInstance(nullptr);
    ui.setBaseLayerInstance(Containers::pointer<Layer>(ui.createLayer(), shared));
    CORRADE_COMPARE(out.str(),
        "Ui::UserInterface::setBaseLayerInstance(): instance is null\n"
        "Ui::UserInterface::setBaseLayerInstance(): instance already set\n");
}

void UserInterfaceTest::baseLayerInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    CORRADE_VERIFY(!ui.hasBaseLayer());

    std::ostringstream out;
    Error redirectError{&out};
    ui.baseLayer();
    CORRADE_COMPARE(out.str(), "Ui::UserInterface::baseLayer(): no instance set\n");
}

void UserInterfaceTest::setTextLayerInstance() {
    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(const Configuration& configuration): TextLayer::Shared{configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{TextLayer::Shared::Configuration{1, 3}};

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

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(const Configuration& configuration): TextLayer::Shared{configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{TextLayer::Shared::Configuration{3, 5}};

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    };

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    ui.setTextLayerInstance(Containers::pointer<Layer>(ui.createLayer(), shared));

    std::ostringstream out;
    Error redirectError{&out};
    ui.setTextLayerInstance(nullptr);
    ui.setTextLayerInstance(Containers::pointer<Layer>(ui.createLayer(), shared));
    CORRADE_COMPARE(out.str(),
        "Ui::UserInterface::setTextLayerInstance(): instance is null\n"
        "Ui::UserInterface::setTextLayerInstance(): instance already set\n");
}

void UserInterfaceTest::textLayerInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};

    std::ostringstream out;
    Error redirectError{&out};
    ui.textLayer();
    CORRADE_COMPARE(out.str(), "Ui::UserInterface::textLayer(): no instance set\n");
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

    std::ostringstream out;
    Error redirectError{&out};
    ui.setEventLayerInstance(nullptr);
    ui.setEventLayerInstance(Containers::pointer<EventLayer>(ui.createLayer()));
    CORRADE_COMPARE(out.str(),
        "Ui::UserInterface::setEventLayerInstance(): instance is null\n"
        "Ui::UserInterface::setEventLayerInstance(): instance already set\n");
}

void UserInterfaceTest::eventLayerInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};

    std::ostringstream out;
    Error redirectError{&out};
    ui.eventLayer();
    CORRADE_COMPARE(out.str(), "Ui::UserInterface::eventLayer(): no instance set\n");
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

    std::ostringstream out;
    Error redirectError{&out};
    ui.setSnapLayouterInstance(nullptr);
    ui.setSnapLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));
    CORRADE_COMPARE(out.str(),
        "Ui::UserInterface::setSnapLayouterInstance(): instance is null\n"
        "Ui::UserInterface::setSnapLayouterInstance(): instance already set\n");
}

void UserInterfaceTest::snapLayouterInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};

    std::ostringstream out;
    Error redirectError{&out};
    ui.snapLayouter();
    CORRADE_COMPARE(out.str(), "Ui::UserInterface::snapLayouter(): no instance set\n");
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::UserInterfaceTest)
