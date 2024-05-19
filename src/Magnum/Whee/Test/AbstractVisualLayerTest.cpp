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
#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/GrowableArray.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/Utility/DebugStl.h> /** @todo remove once Debug is stream-free */
#include <Corrade/Utility/FormatStl.h> /** @todo remove once Debug is stream-free */

#include "Magnum/Whee/AbstractUserInterface.h"
#include "Magnum/Whee/AbstractVisualLayer.h"
#include "Magnum/Whee/Event.h"
#include "Magnum/Whee/Handle.h"
#include "Magnum/Whee/NodeFlags.h"
/* for setStyle(), eventStyleTransition*() */
#include "Magnum/Whee/Implementation/abstractVisualLayerState.h"

namespace Magnum { namespace Whee { namespace Test { namespace {

struct AbstractVisualLayerTest: TestSuite::Tester {
    explicit AbstractVisualLayerTest();

    void sharedConstruct();
    void sharedConstructNoCreate();
    void sharedConstructCopy();
    void sharedConstructMove();
    void sharedConstructMoveMovedOutInstance();

    void construct();
    void constructCopy();
    void constructMove();

    template<class T> void setStyle();
    void setTransitionedStyle();
    void invalidHandle();
    void styleOutOfRange();

    void dynamicStyleAllocateRecycle();
    void dynamicStyleAllocateNoDynamicStyles();
    void dynamicStyleRecycleInvalid();

    void eventStyleTransitionNoOp();
    void eventStyleTransition();
    void eventStyleTransitionNoHover();
    void eventStyleTransitionDisabled();
    void eventStyleTransitionNoCapture();
    void eventStyleTransitionOutOfRange();
    void eventStyleTransitionDynamicStyle();
};

enum class Enum: UnsignedShort {};

/* The enum is deliberately not 32-bit to verify the APIs can work with smaller
   types too */
enum class StyleIndex: UnsignedByte {
    Green = 0,
    GreenHover = 1,
    GreenPressed = 2,
    GreenPressedHover = 3,

    Red = 4,
    RedHover = 5,
    RedPressed = 6,
    RedPressedHover = 7,

    Blue = 8,
    BluePressed = 9,

    White = 10,
    WhiteHover = 11,

    GreenDisabled = 12,
    /* Common for red & blue, to test that there's no inverse mapping done */
    RedBlueDisabled = 13
};

Debug& operator<<(Debug& debug, StyleIndex value) {
    switch(value) {
        #define _c(value) case StyleIndex::value: return debug << "StyleIndex::" #value;
        _c(Green)
        _c(GreenHover)
        _c(GreenPressed)
        _c(GreenPressedHover)
        _c(Red)
        _c(RedHover)
        _c(RedPressed)
        _c(RedPressedHover)
        _c(Blue)
        _c(BluePressed)
        _c(White)
        _c(WhiteHover)
        _c(GreenDisabled)
        _c(RedBlueDisabled)
        #undef _c
    }

    return debug << UnsignedInt(value);
}

const struct {
    const char* name;
    UnsignedInt styleCount, dynamicStyleCount;
} SetStyleData[]{
    {"", 67, 0},
    /* 37 is used as one style ID and 66 as the other, make sure the actual
       style count is less than that in both cases */
    {"dynamic styles", 29, 38},
};

const struct {
    const char* name;
    UnsignedInt styleCount, dynamicStyleCount;
} StyleOutOfRangeData[]{
    {"", 3, 0},
    {"dynamic styles", 2, 1},
};

const struct {
    const char* name;
    bool update;
    bool templated;
} EventStyleTransitionData[]{
    {"update before", true, false},
    {"", false, false},
    {"templated, update before", true, true},
    {"templated", false, true},
};

const struct {
    const char* name;
    bool templated;
} EventStyleTransitionDisabledData[]{
    {"", false},
    {"templated", true},
};

const struct {
    const char* name;
    bool disableCapture;
    StyleIndex outStyle, overStyle;
} EventStyleTransitionNoCaptureData[]{
    {"", false, StyleIndex::GreenPressed, StyleIndex::GreenPressedHover},
    {"capture disabled", true, StyleIndex::Green, StyleIndex::GreenHover},
};

const struct {
    const char* name;
    UnsignedInt dynamicStyleCount;
} EventStyleTransitionOutOfRangeData[]{
    {"", 0},
    {"dynamic styles", 5},
};

AbstractVisualLayerTest::AbstractVisualLayerTest() {
    addTests({&AbstractVisualLayerTest::sharedConstruct,
              &AbstractVisualLayerTest::sharedConstructNoCreate,
              &AbstractVisualLayerTest::sharedConstructCopy,
              &AbstractVisualLayerTest::sharedConstructMove,
              &AbstractVisualLayerTest::sharedConstructMoveMovedOutInstance,

              &AbstractVisualLayerTest::construct,
              &AbstractVisualLayerTest::constructCopy,
              &AbstractVisualLayerTest::constructMove});

    addInstancedTests<AbstractVisualLayerTest>({
        &AbstractVisualLayerTest::setStyle<UnsignedInt>,
        &AbstractVisualLayerTest::setStyle<Enum>},
        Containers::arraySize(SetStyleData));

    addTests({&AbstractVisualLayerTest::setTransitionedStyle,
              &AbstractVisualLayerTest::invalidHandle});

    addInstancedTests({&AbstractVisualLayerTest::styleOutOfRange},
        Containers::arraySize(StyleOutOfRangeData));

    addTests({&AbstractVisualLayerTest::dynamicStyleAllocateRecycle,
              &AbstractVisualLayerTest::dynamicStyleAllocateNoDynamicStyles,
              &AbstractVisualLayerTest::dynamicStyleRecycleInvalid,

              &AbstractVisualLayerTest::eventStyleTransitionNoOp});

    addInstancedTests({&AbstractVisualLayerTest::eventStyleTransition,
                       &AbstractVisualLayerTest::eventStyleTransitionNoHover},
        Containers::arraySize(EventStyleTransitionData));

    addInstancedTests({&AbstractVisualLayerTest::eventStyleTransitionDisabled},
        Containers::arraySize(EventStyleTransitionDisabledData));

    addInstancedTests({&AbstractVisualLayerTest::eventStyleTransitionNoCapture},
        Containers::arraySize(EventStyleTransitionNoCaptureData));

    addInstancedTests({&AbstractVisualLayerTest::eventStyleTransitionOutOfRange},
        Containers::arraySize(EventStyleTransitionOutOfRangeData));

    addTests({&AbstractVisualLayerTest::eventStyleTransitionDynamicStyle});
}

void AbstractVisualLayerTest::sharedConstruct() {
    AbstractVisualLayer::Shared* self;
    struct Shared: AbstractVisualLayer::Shared {
        explicit Shared(UnsignedInt styleCount, UnsignedInt dynamicStyleCount, AbstractVisualLayer::Shared*& selfPointer): AbstractVisualLayer::Shared{styleCount, dynamicStyleCount} {
            selfPointer = &*_state->self;
        }
    } shared{3, 5, self};
    CORRADE_COMPARE(shared.styleCount(), 3);
    CORRADE_COMPARE(shared.dynamicStyleCount(), 5);
    CORRADE_COMPARE(shared.totalStyleCount(), 8);
    CORRADE_COMPARE(self, &shared);
}

void AbstractVisualLayerTest::sharedConstructNoCreate() {
    struct Shared: AbstractVisualLayer::Shared {
        explicit Shared(NoCreateT): AbstractVisualLayer::Shared{NoCreate} {}
    } shared{NoCreate};

    /* Shouldn't crash */
    CORRADE_VERIFY(true);

    /* Implicit construction is not allowed */
    CORRADE_VERIFY(!std::is_convertible<NoCreateT, AbstractVisualLayer::Shared>::value);
}

void AbstractVisualLayerTest::sharedConstructCopy() {
    CORRADE_VERIFY(!std::is_copy_constructible<AbstractVisualLayer::Shared>{});
    CORRADE_VERIFY(!std::is_copy_assignable<AbstractVisualLayer::Shared>{});
}

void AbstractVisualLayerTest::sharedConstructMove() {
    struct Shared: AbstractVisualLayer::Shared {
        explicit Shared(UnsignedInt styleCount, UnsignedInt dynamicStyleCount, Containers::Reference<AbstractVisualLayer::Shared>*& selfPointer): AbstractVisualLayer::Shared{styleCount, dynamicStyleCount} {
            selfPointer = &_state->self;
        }
    };

    Containers::Reference<AbstractVisualLayer::Shared>* aSelf;
    Shared a{3, 5, aSelf};
    CORRADE_COMPARE(&**aSelf, &a);

    Shared b{Utility::move(a)};
    CORRADE_COMPARE(b.styleCount(), 3);
    CORRADE_COMPARE(b.dynamicStyleCount(), 5);
    CORRADE_COMPARE(&**aSelf, &b);

    Containers::Reference<AbstractVisualLayer::Shared>* cSelf;
    Shared c{7, 9, cSelf};
    CORRADE_COMPARE(&**cSelf, &c);

    c = Utility::move(b);
    CORRADE_COMPARE(c.styleCount(), 3);
    CORRADE_COMPARE(c.dynamicStyleCount(), 5);
    CORRADE_COMPARE(&**aSelf, &c);
    CORRADE_COMPARE(&**cSelf, &b);

    CORRADE_VERIFY(std::is_nothrow_move_constructible<AbstractVisualLayer::Shared>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<AbstractVisualLayer::Shared>::value);
}

void AbstractVisualLayerTest::sharedConstructMoveMovedOutInstance() {
    struct Shared: AbstractVisualLayer::Shared {
        explicit Shared(UnsignedInt styleCount, UnsignedInt dynamicStyleCount, Containers::Reference<AbstractVisualLayer::Shared>*& selfPointer): AbstractVisualLayer::Shared{styleCount, dynamicStyleCount} {
            selfPointer = &_state->self;
        }
    };

    Containers::Reference<AbstractVisualLayer::Shared>* aSelf;
    Shared a{3, 5, aSelf};
    Shared out{Utility::move(a)};
    CORRADE_COMPARE(&**aSelf, &out);

    /* B should be moved out as well */
    Shared b{Utility::move(a)};
    CORRADE_COMPARE(&**aSelf, &out);

    Containers::Reference<AbstractVisualLayer::Shared>* cSelf;
    Shared c{7, 9, cSelf};
    CORRADE_COMPARE(&**cSelf, &c);

    /* Moving a moved-out instance (a) to an alive instance (c) should redirect
       only the alive self */
    c = Utility::move(a);
    CORRADE_COMPARE(&**aSelf, &out);
    CORRADE_COMPARE(&**cSelf, &a);

    /* Moving an alive instance (a) to a moved-out instance (b) should again
       redirect only the alive self */
    b = Utility::move(a);
    CORRADE_COMPARE(&**aSelf, &out);
    CORRADE_COMPARE(&**cSelf, &b);

    /* Moving a moved-out instance (a) to a moved-out instance (c) shouldn't
       do anything */
    c = Utility::move(a);
    CORRADE_COMPARE(&**aSelf, &out);
    CORRADE_COMPARE(&**cSelf, &b);
}

void AbstractVisualLayerTest::construct() {
    struct LayerShared: AbstractVisualLayer::Shared {
        explicit LayerShared(UnsignedInt styleCount, UnsignedInt dynamicStyleCount): AbstractVisualLayer::Shared{styleCount, dynamicStyleCount} {}
    } shared{3, 5};

    struct Layer: AbstractVisualLayer {
        explicit Layer(LayerHandle handle, Shared& shared): AbstractVisualLayer{handle, shared} {}
    } layer{layerHandle(137, 0xfe), shared};

    /* There isn't anything to query on the AbstractVisualLayer itself */
    CORRADE_COMPARE(layer.handle(), layerHandle(137, 0xfe));
    CORRADE_COMPARE(&layer.shared(), &shared);
    /* Const overload */
    CORRADE_COMPARE(&static_cast<const Layer&>(layer).shared(), &shared);
}

void AbstractVisualLayerTest::constructCopy() {
    CORRADE_VERIFY(!std::is_copy_constructible<AbstractVisualLayer>{});
    CORRADE_VERIFY(!std::is_copy_assignable<AbstractVisualLayer>{});
}

void AbstractVisualLayerTest::constructMove() {
    struct LayerShared: AbstractVisualLayer::Shared {
        explicit LayerShared(UnsignedInt styleCount, UnsignedInt dynamicStyleCount): AbstractVisualLayer::Shared{styleCount, dynamicStyleCount} {}
    };

    struct Layer: AbstractVisualLayer {
        explicit Layer(LayerHandle handle, Shared& shared): AbstractVisualLayer{handle, shared} {}
    };

    LayerShared shared{3, 2};
    LayerShared shared2{5, 7};

    Layer a{layerHandle(137, 0xfe), shared};

    Layer b{Utility::move(a)};
    CORRADE_COMPARE(b.handle(), layerHandle(137, 0xfe));
    CORRADE_COMPARE(&b.shared(), &shared);

    Layer c{layerHandle(0, 2), shared2};
    c = Utility::move(b);
    CORRADE_COMPARE(c.handle(), layerHandle(137, 0xfe));
    CORRADE_COMPARE(&c.shared(), &shared);

    CORRADE_VERIFY(std::is_nothrow_move_constructible<AbstractVisualLayer>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<AbstractVisualLayer>::value);
}

/* These are shared by all cases that need to call create() below */
struct StyleLayerShared: AbstractVisualLayer::Shared {
    explicit StyleLayerShared(UnsignedInt styleCount, UnsignedInt dynamicStyleCount): AbstractVisualLayer::Shared{styleCount, dynamicStyleCount} {}

    /* To verify that the macro correctly passes everything through. The Shared
       typedef is because the macro overrides return Shared&, which if not
       defined here would mean the base class. */
    typedef StyleLayerShared Shared;
    MAGNUMEXTRAS_WHEE_ABSTRACTVISUALLAYER_SHARED_SUBCLASS_IMPLEMENTATION()
};
struct StyleLayer: AbstractVisualLayer {
    explicit StyleLayer(LayerHandle handle, Shared& shared): AbstractVisualLayer{handle, shared} {}

    const State& stateData() const { return static_cast<const State&>(*_state); }

    /* Just saves the style index and sync's the styles array */
    template<class T> DataHandle create(T style, NodeHandle node = NodeHandle::Null) {
        const DataHandle handle = AbstractVisualLayer::create(node);
        const UnsignedInt id = dataHandleId(handle);
        if(id >= data.size()) {
            arrayAppend(data, NoInit, id - data.size() + 1);
            _state->styles = stridedArrayView(data).slice(&decltype(data)::Type::first);
            _state->calculatedStyles = stridedArrayView(data).slice(&decltype(data)::Type::second);
        }
        data[id].first() = UnsignedInt(style);
        return handle;
    }

    Containers::Array<Containers::Pair<UnsignedInt, UnsignedInt>> data;
};

template<class T> void AbstractVisualLayerTest::setStyle() {
    auto&& data = SetStyleData[testCaseInstanceId()];
    setTestCaseDescription(data.name);
    setTestCaseTemplateName(std::is_same<T, Enum>::value ? "Enum" : "UnsignedInt");

    StyleLayerShared shared{data.styleCount, data.dynamicStyleCount};
    StyleLayer layer{layerHandle(0, 1), shared};

    /* Just to be sure the setters aren't picking up the first ever data
       always */
    layer.create(2);

    DataHandle layerData = layer.create(17);
    CORRADE_COMPARE(layer.style(layerData), 17);
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Setting a style marks the layer as dirty */
    layer.setStyle(layerData, T(37));
    CORRADE_COMPARE(layer.style(layerData), 37);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsUpdate);

    /* Testing also the other overload */
    layer.setStyle(dataHandleData(layerData), T(66));
    CORRADE_COMPARE(layer.style(layerData), 66);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsUpdate);
}

void AbstractVisualLayerTest::setTransitionedStyle() {
    AbstractUserInterface ui{{100, 100}};

    enum Style {
        /* 2 is first, to avoid accidentally matching the order */
        InactiveOut2,
        InactiveOut1,
        InactiveOver2,
        InactiveOver1,
        PressedOut2,
        PressedOut1,
        PressedOver2,
        PressedOver1,
    };

    /* Style transition isn't allowed to use dynamic styles so the dynamic
       count shouldn't affect it */
    StyleLayerShared shared{8, 0};
    shared.setStyleTransition(
        [](UnsignedInt style) -> UnsignedInt {
            switch(Style(style)) {
                case InactiveOut1:
                case InactiveOver1:
                case PressedOut1:
                case PressedOver1:
                    return PressedOut1;
                case InactiveOut2:
                case InactiveOver2:
                case PressedOut2:
                case PressedOver2:
                    return PressedOut2;
            }
            CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        },
        [](UnsignedInt style) -> UnsignedInt {
            switch(Style(style)) {
                case InactiveOut1:
                case InactiveOver1:
                case PressedOut1:
                case PressedOver1:
                    return PressedOver1;
                case InactiveOut2:
                case InactiveOver2:
                case PressedOut2:
                case PressedOver2:
                    return PressedOver2;
            }
            CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        },
        [](UnsignedInt style) -> UnsignedInt {
            switch(Style(style)) {
                case InactiveOut1:
                case InactiveOver1:
                case PressedOut1:
                case PressedOver1:
                    return InactiveOut1;
                case InactiveOut2:
                case InactiveOver2:
                case PressedOut2:
                case PressedOver2:
                    return InactiveOut2;
            }
            CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        },
        [](UnsignedInt style) -> UnsignedInt {
            switch(Style(style)) {
                case InactiveOut1:
                case InactiveOver1:
                case PressedOut1:
                case PressedOver1:
                    return InactiveOver1;
                case InactiveOut2:
                case InactiveOver2:
                case PressedOut2:
                case PressedOver2:
                    return InactiveOver2;
            }
            CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        },
        [](UnsignedInt) -> UnsignedInt {
            CORRADE_FAIL("This shouldn't be called");
            CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        });
    StyleLayer& layer = ui.setLayerInstance(Containers::pointer<StyleLayer>(ui.createLayer(), shared));

    /* Node 2 is first, to avoid accidentally matching the order */
    NodeHandle node2 = ui.createNode({}, {100, 50});
    NodeHandle node1 = ui.createNode({0, 50}, {100, 50});
    DataHandle data1 = layer.create(InactiveOut1, node1);
    DataHandle data2 = layer.create(InactiveOut2, node2);

    /* Nothing is hovered or pressed initially. */
    CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
    CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

    /* Setting a transitioned style picks InactiveOut. Switching the IDs to be
       sure it actually changed. */
    layer.setTransitionedStyle(ui, data1, PressedOut2);
    layer.setTransitionedStyle(ui, data2, InactiveOver1);
    CORRADE_COMPARE(layer.style(data1), InactiveOut2);
    CORRADE_COMPARE(layer.style(data2), InactiveOut1);

    /* Hovering node 2 causes the style to be changed to InactiveOver */
    {
        PointerMoveEvent event{{}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({50, 25}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), node2);
        CORRADE_COMPARE(layer.style(data2), InactiveOver1);
    }

    /* Setting a transitioned style (switching IDs again) picks InactiveOver
       for the hovered node, the other stays InactiveOut. Using the integer
       overload. */
    layer.setTransitionedStyle(ui, data1, UnsignedInt(InactiveOver1));
    layer.setTransitionedStyle(ui, data2, UnsignedInt(PressedOut2));
    CORRADE_COMPARE(layer.style(data1), InactiveOut1);
    CORRADE_COMPARE(layer.style(data2), InactiveOver2);

    /* Pressing on node 2 causes the style to be changed to PressedOver */
    {
        PointerEvent event{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({50, 25}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), node2);
        CORRADE_COMPARE(ui.currentHoveredNode(), node2);
        CORRADE_COMPARE(layer.style(data2), PressedOver2);
    }

    /* Setting a transitioned style (switching IDs again) picks PressedOver
       for the pressed & hovered node, the other again stays InactiveOut.
       Using the LayerDataHandle overload. */
    layer.setTransitionedStyle(ui, dataHandleData(data1), PressedOut2);
    layer.setTransitionedStyle(ui, dataHandleData(data2), InactiveOut1);
    CORRADE_COMPARE(layer.style(data1), InactiveOut2);
    CORRADE_COMPARE(layer.style(data2), PressedOver1);

    /* Moving onto node 1 causes the style to be changed to PressedOut. No
       node is hovered due to event capture on node 2. */
    {
        PointerMoveEvent event{{}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({50, 75}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), node2);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style(data2), PressedOut1);
    }

    /* Setting a transitioned style (switching IDs again) picks PressedOut
       for the pressed node, the other again stays InactiveOut. Using the
       integer + LayerDataHandle overload. */
    layer.setTransitionedStyle(ui, dataHandleData(data1), UnsignedInt(InactiveOut1));
    layer.setTransitionedStyle(ui, dataHandleData(data2), UnsignedInt(PressedOver2));
    CORRADE_COMPARE(layer.style(data1), InactiveOut1);
    CORRADE_COMPARE(layer.style(data2), PressedOut2);

    /* Releasing causes the style to be changed to InactiveOut */
    {
        PointerEvent event{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerReleaseEvent({50, 75}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style(data2), InactiveOut2);
    }

    /* Setting a transitioned style (switching IDs again) picks InactiveOut
       for both */
    layer.setTransitionedStyle(ui, data1, PressedOut2);
    layer.setTransitionedStyle(ui, data2, InactiveOver1);
    CORRADE_COMPARE(layer.style(data1), InactiveOut2);
    CORRADE_COMPARE(layer.style(data2), InactiveOut1);
}

void AbstractVisualLayerTest::invalidHandle() {
    CORRADE_SKIP_IF_NO_ASSERT();

    AbstractUserInterface ui{{100, 100}};

    struct LayerShared: AbstractVisualLayer::Shared {
        explicit LayerShared(UnsignedInt styleCount, UnsignedInt dynamicStyleCount): AbstractVisualLayer::Shared{styleCount, dynamicStyleCount} {}
    } shared{1, 0};

    struct Layer: AbstractVisualLayer {
        explicit Layer(LayerHandle handle, Shared& shared): AbstractVisualLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    std::ostringstream out;
    Error redirectError{&out};
    layer.style(DataHandle::Null);
    layer.style(LayerDataHandle::Null);
    layer.setStyle(DataHandle::Null, 0);
    layer.setStyle(LayerDataHandle::Null, 0);
    layer.setTransitionedStyle(ui, DataHandle::Null, 0);
    layer.setTransitionedStyle(ui, LayerDataHandle::Null, 0);
    CORRADE_COMPARE(out.str(),
        "Whee::AbstractVisualLayer::style(): invalid handle Whee::DataHandle::Null\n"
        "Whee::AbstractVisualLayer::style(): invalid handle Whee::LayerDataHandle::Null\n"
        "Whee::AbstractVisualLayer::setStyle(): invalid handle Whee::DataHandle::Null\n"
        "Whee::AbstractVisualLayer::setStyle(): invalid handle Whee::LayerDataHandle::Null\n"
        "Whee::AbstractVisualLayer::setTransitionedStyle(): invalid handle Whee::DataHandle::Null\n"
        "Whee::AbstractVisualLayer::setTransitionedStyle(): invalid handle Whee::LayerDataHandle::Null\n");
}

void AbstractVisualLayerTest::styleOutOfRange() {
    auto&& data = StyleOutOfRangeData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    CORRADE_SKIP_IF_NO_ASSERT();

    AbstractUserInterface ui{{100, 100}};

    struct LayerShared: AbstractVisualLayer::Shared {
        explicit LayerShared(UnsignedInt styleCount, UnsignedInt dynamicStyleCount): AbstractVisualLayer::Shared{styleCount, dynamicStyleCount} {}
    } shared{data.styleCount, data.dynamicStyleCount};

    struct Layer: AbstractVisualLayer {
        explicit Layer(LayerHandle handle, Shared& shared): AbstractVisualLayer{handle, shared} {}

        using AbstractVisualLayer::create;
    } layer{layerHandle(0, 1), shared};

    DataHandle layerData = layer.create();

    std::ostringstream out;
    Error redirectError{&out};
    layer.setStyle(layerData, 3);
    layer.setStyle(dataHandleData(layerData), 3);
    layer.setTransitionedStyle(ui, layerData, data.styleCount);
    layer.setTransitionedStyle(ui, dataHandleData(layerData), data.styleCount);
    CORRADE_COMPARE(out.str(), Utility::formatString(
        "Whee::AbstractVisualLayer::setStyle(): style 3 out of range for 3 styles\n"
        "Whee::AbstractVisualLayer::setStyle(): style 3 out of range for 3 styles\n"
        "Whee::AbstractVisualLayer::setTransitionedStyle(): style {0} out of range for {0} styles\n"
        "Whee::AbstractVisualLayer::setTransitionedStyle(): style {0} out of range for {0} styles\n", data.styleCount));
}

void AbstractVisualLayerTest::dynamicStyleAllocateRecycle() {
    struct LayerShared: AbstractVisualLayer::Shared {
        explicit LayerShared(UnsignedInt styleCount, UnsignedInt dynamicStyleCount): AbstractVisualLayer::Shared{styleCount, dynamicStyleCount} {}
    } shared{3, 5};

    struct Layer: AbstractVisualLayer {
        explicit Layer(LayerHandle handle, Shared& shared): AbstractVisualLayer{handle, shared} {}

        using AbstractVisualLayer::create;
    } layer{layerHandle(0, 1), shared};

    CORRADE_COMPARE(shared.dynamicStyleCount(), 5);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 0);

    Containers::Optional<UnsignedInt> first = layer.allocateDynamicStyle();
    CORRADE_COMPARE(first, 0);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 1);

    Containers::Optional<UnsignedInt> second = layer.allocateDynamicStyle();
    CORRADE_COMPARE(second, 1);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 2);

    Containers::Optional<UnsignedInt> third = layer.allocateDynamicStyle();
    CORRADE_COMPARE(third, 2);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 3);

    Containers::Optional<UnsignedInt> fourth = layer.allocateDynamicStyle();
    CORRADE_COMPARE(fourth, 3);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 4);

    /* Recycle a subset in random order */
    layer.recycleDynamicStyle(*third);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 3);

    layer.recycleDynamicStyle(*second);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 2);

    layer.recycleDynamicStyle(*fourth);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 1);

    /* Allocating new ones simply picks up the first free */
    Containers::Optional<UnsignedInt> second2 = layer.allocateDynamicStyle();
    Containers::Optional<UnsignedInt> third2 = layer.allocateDynamicStyle();
    Containers::Optional<UnsignedInt> fourth2 = layer.allocateDynamicStyle();
    CORRADE_COMPARE(second2, 1);
    CORRADE_COMPARE(third2, 2);
    CORRADE_COMPARE(fourth2, 3);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 4);

    /* Try recycling the first also */
    layer.recycleDynamicStyle(*first);
    Containers::Optional<UnsignedInt> first2 = layer.allocateDynamicStyle();
    CORRADE_COMPARE(first2, 0);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 4);

    /* Allocating the last free */
    Containers::Optional<UnsignedInt> fifth = layer.allocateDynamicStyle();
    CORRADE_COMPARE(fifth, 4);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 5);

    /* It's not possible to allocate any more at this point */
    CORRADE_COMPARE(layer.allocateDynamicStyle(), Containers::NullOpt);
}

void AbstractVisualLayerTest::dynamicStyleAllocateNoDynamicStyles() {
    struct LayerShared: AbstractVisualLayer::Shared {
        explicit LayerShared(UnsignedInt styleCount, UnsignedInt dynamicStyleCount): AbstractVisualLayer::Shared{styleCount, dynamicStyleCount} {}
    } shared{3, 0};

    struct Layer: AbstractVisualLayer {
        explicit Layer(LayerHandle handle, Shared& shared): AbstractVisualLayer{handle, shared} {}

        using AbstractVisualLayer::create;
    } layer{layerHandle(0, 1), shared};

    CORRADE_COMPARE(shared.dynamicStyleCount(), 0);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 0);
    CORRADE_COMPARE(layer.allocateDynamicStyle(), Containers::NullOpt);
}

void AbstractVisualLayerTest::dynamicStyleRecycleInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct LayerShared: AbstractVisualLayer::Shared {
        explicit LayerShared(UnsignedInt styleCount, UnsignedInt dynamicStyleCount): AbstractVisualLayer::Shared{styleCount, dynamicStyleCount} {}
    } shared{3, 4};

    struct Layer: AbstractVisualLayer {
        explicit Layer(LayerHandle handle, Shared& shared): AbstractVisualLayer{handle, shared} {}

        using AbstractVisualLayer::create;
    } layer{layerHandle(0, 1), shared};

    std::ostringstream out;
    Error redirectError{&out};
    layer.recycleDynamicStyle(2);
    layer.recycleDynamicStyle(4);
    CORRADE_COMPARE(out.str(),
        "Whee::AbstractVisualLayer::recycleDynamicStyle(): style 2 not allocated\n"
        "Whee::AbstractVisualLayer::recycleDynamicStyle(): index 4 out of range for 4 dynamic styles\n");
}

StyleIndex styleIndexTransitionToInactiveOut(StyleIndex index) {
    switch(index) {
        case StyleIndex::Green:
        case StyleIndex::GreenHover:
        case StyleIndex::GreenPressed:
        case StyleIndex::GreenPressedHover:
            return StyleIndex::Green;
        case StyleIndex::Red:
        case StyleIndex::RedHover:
        case StyleIndex::RedPressed:
        case StyleIndex::RedPressedHover:
            return StyleIndex::Red;
        case StyleIndex::Blue:
        case StyleIndex::BluePressed:
            return StyleIndex::Blue;
        case StyleIndex::White:
        case StyleIndex::WhiteHover:
            return StyleIndex::White;
        case StyleIndex::GreenDisabled:
        case StyleIndex::RedBlueDisabled:
            CORRADE_FAIL("Called with" << index);
    }
    CORRADE_FAIL("Called with" << UnsignedInt(index));
    CORRADE_INTERNAL_ASSERT_UNREACHABLE();
}

StyleIndex styleIndexTransitionToInactiveOver(StyleIndex index) {
    switch(index) {
        case StyleIndex::Green:
        case StyleIndex::GreenHover:
        case StyleIndex::GreenPressed:
        case StyleIndex::GreenPressedHover:
            return StyleIndex::GreenHover;
        case StyleIndex::Red:
        case StyleIndex::RedHover:
        case StyleIndex::RedPressed:
        case StyleIndex::RedPressedHover:
            return StyleIndex::RedHover;
        case StyleIndex::Blue:
        case StyleIndex::BluePressed:
            return StyleIndex::Blue;
        case StyleIndex::White:
        case StyleIndex::WhiteHover:
            return StyleIndex::WhiteHover;
        case StyleIndex::GreenDisabled:
        case StyleIndex::RedBlueDisabled:
            CORRADE_FAIL("Called with" << index);
    }
    CORRADE_FAIL("Called with" << UnsignedInt(index));
    CORRADE_INTERNAL_ASSERT_UNREACHABLE();
}

StyleIndex styleIndexTransitionToPressedOut(StyleIndex index) {
    switch(index) {
        case StyleIndex::Green:
        case StyleIndex::GreenHover:
        case StyleIndex::GreenPressed:
        case StyleIndex::GreenPressedHover:
            return StyleIndex::GreenPressed;
        case StyleIndex::Red:
        case StyleIndex::RedHover:
        case StyleIndex::RedPressed:
        case StyleIndex::RedPressedHover:
            return StyleIndex::RedPressed;
        case StyleIndex::Blue:
        case StyleIndex::BluePressed:
            return StyleIndex::BluePressed;
        case StyleIndex::White:
        case StyleIndex::WhiteHover:
            return StyleIndex::White;
        case StyleIndex::GreenDisabled:
        case StyleIndex::RedBlueDisabled:
            CORRADE_FAIL("Called with" << index);
    }
    CORRADE_FAIL("Called with" << UnsignedInt(index));
    CORRADE_INTERNAL_ASSERT_UNREACHABLE();
}

StyleIndex styleIndexTransitionToPressedOver(StyleIndex index) {
    switch(index) {
        case StyleIndex::Green:
        case StyleIndex::GreenHover:
        case StyleIndex::GreenPressed:
        case StyleIndex::GreenPressedHover:
            return StyleIndex::GreenPressedHover;
        case StyleIndex::Red:
        case StyleIndex::RedHover:
        case StyleIndex::RedPressed:
        case StyleIndex::RedPressedHover:
            return StyleIndex::RedPressedHover;
        case StyleIndex::Blue:
        case StyleIndex::BluePressed:
            return StyleIndex::BluePressed;
        case StyleIndex::White:
        case StyleIndex::WhiteHover:
            return StyleIndex::WhiteHover;
        case StyleIndex::GreenDisabled:
        case StyleIndex::RedBlueDisabled:
            CORRADE_FAIL("Called with" << index);
    }
    CORRADE_FAIL("Called with" << UnsignedInt(index));
    CORRADE_INTERNAL_ASSERT_UNREACHABLE();
}

/* The toDisabled function should only be called from doUpdate(), this verifies
   that */
StyleIndex styleIndexTransitionToDisabledDoNotCall(StyleIndex index) {
    CORRADE_FAIL("Called with" << UnsignedInt(index));
    CORRADE_INTERNAL_ASSERT_UNREACHABLE();
}

StyleIndex styleIndexTransitionToDisabled(StyleIndex index) {
    switch(index) {
        case StyleIndex::Green:
        case StyleIndex::GreenHover:
        case StyleIndex::GreenPressed:
        case StyleIndex::GreenPressedHover:
            return StyleIndex::GreenDisabled;
        /* These two collapse to a single style, to verify that the mapping is
           only ever in one direction and not back */
        case StyleIndex::Red:
        case StyleIndex::RedHover:
        case StyleIndex::RedPressed:
        case StyleIndex::RedPressedHover:
        case StyleIndex::Blue:
        case StyleIndex::BluePressed:
            return StyleIndex::RedBlueDisabled;
        /* This one has no disabled state */
        case StyleIndex::White:
        case StyleIndex::WhiteHover:
            return index;
        /* The disabled state shouldn't be the source state either */
        case StyleIndex::GreenDisabled:
        case StyleIndex::RedBlueDisabled:
            CORRADE_FAIL("Called with" << index);
    }
    CORRADE_FAIL("Called with" << UnsignedInt(index));
    CORRADE_INTERNAL_ASSERT_UNREACHABLE();
}

void AbstractVisualLayerTest::eventStyleTransitionNoOp() {
    /* Transition for dynamic styles tested in
       eventStyleTransitionDynamicStyle() instead */
    StyleLayerShared shared{14, 0};

    AbstractUserInterface ui{{100, 100}};

    NodeHandle node = ui.createNode({1.0f, 1.0f}, {2.0f, 2.0f});

    StyleLayer& layer = ui.setLayerInstance(Containers::pointer<StyleLayer>(ui.createLayer(), shared));
    /* Deliberately setting a style that isn't the "default" */
    DataHandle data = layer.create(StyleIndex::GreenPressedHover, node);

    ui.update();
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Press, release, over, hovered press, hovered release, out should all do
       nothing by default */
    {
        PointerEvent event{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({2.0f, 2.0f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), node);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(data), StyleIndex::GreenPressedHover);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    } {
        PointerEvent event{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerReleaseEvent({2.5f, 2.5f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(data), StyleIndex::GreenPressedHover);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    } {
        PointerMoveEvent event{{}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({2.0f, 2.0f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), node);
        CORRADE_COMPARE(layer.style<StyleIndex>(data), StyleIndex::GreenPressedHover);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    } {
        PointerEvent event{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({2.0f, 2.0f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), node);
        CORRADE_COMPARE(ui.currentHoveredNode(), node);
        CORRADE_COMPARE(layer.style<StyleIndex>(data), StyleIndex::GreenPressedHover);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    } {
        PointerEvent event{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerReleaseEvent({2.5f, 2.5f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), node);
        CORRADE_COMPARE(layer.style<StyleIndex>(data), StyleIndex::GreenPressedHover);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    } {
        PointerMoveEvent event{{}, {}};
        CORRADE_VERIFY(!ui.pointerMoveEvent({5.0f, 2.0f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(data), StyleIndex::GreenPressedHover);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }

    /* Setting a null toPressedOut transition will do nothing for a press */
    shared.setStyleTransition<StyleIndex,
        nullptr,
        styleIndexTransitionToPressedOver,
        styleIndexTransitionToInactiveOut,
        styleIndexTransitionToInactiveOver,
        styleIndexTransitionToDisabledDoNotCall>();
    {
        PointerEvent event{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({2.0f, 2.0f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), node);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(data), StyleIndex::GreenPressedHover);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }

    /* Setting a null toInactiveOut transition will do nothing for a release */
    shared.setStyleTransition<StyleIndex,
        styleIndexTransitionToPressedOut,
        styleIndexTransitionToPressedOver,
        nullptr,
        styleIndexTransitionToInactiveOver,
        styleIndexTransitionToDisabledDoNotCall>();
    {
        PointerEvent event{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerReleaseEvent({2.5f, 2.5f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(data), StyleIndex::GreenPressedHover);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }

    /* Setting a null toInactiveOver will do nothing for a hover */
    shared.setStyleTransition<StyleIndex,
        styleIndexTransitionToPressedOut,
        styleIndexTransitionToPressedOver,
        styleIndexTransitionToInactiveOut,
        nullptr,
        styleIndexTransitionToDisabledDoNotCall>();
    {
        PointerMoveEvent event{{}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({1.5f, 2.0f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), node);
        CORRADE_COMPARE(layer.style<StyleIndex>(data), StyleIndex::GreenPressedHover);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }

    /* Setting a null toPressedOver will do nothing for a hovered press */
    shared.setStyleTransition<StyleIndex,
        styleIndexTransitionToPressedOut,
        nullptr,
        styleIndexTransitionToInactiveOut,
        styleIndexTransitionToInactiveOver,
        styleIndexTransitionToDisabledDoNotCall>();
    {
        PointerEvent event{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({2.0f, 2.0f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), node);
        CORRADE_COMPARE(ui.currentHoveredNode(), node);
        CORRADE_COMPARE(layer.style<StyleIndex>(data), StyleIndex::GreenPressedHover);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }

    /* Setting a null combined toPressed will do nothing for a press */
    shared.setStyleTransition<StyleIndex,
        nullptr,
        styleIndexTransitionToInactiveOut,
        styleIndexTransitionToDisabledDoNotCall>();
    {
        PointerEvent event{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({2.5f, 2.0f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), node);
        CORRADE_COMPARE(ui.currentHoveredNode(), node);
        CORRADE_COMPARE(layer.style<StyleIndex>(data), StyleIndex::GreenPressedHover);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }

    /* Setting a null combined toInactive will do nothing for a release */
    shared.setStyleTransition<StyleIndex,
        styleIndexTransitionToPressedOut,
        nullptr,
        styleIndexTransitionToDisabledDoNotCall>();
    {
        PointerEvent event{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerReleaseEvent({2.0f, 2.0f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), node);
        CORRADE_COMPARE(layer.style<StyleIndex>(data), StyleIndex::GreenPressedHover);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }

    /* toDisabled no-op transition is tested in
       eventStyleTransitionDisabled() */
}

void AbstractVisualLayerTest::eventStyleTransition() {
    auto&& data = EventStyleTransitionData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Transition for dynamic styles tested in
       eventStyleTransitionDynamicStyle() instead */
    StyleLayerShared shared{14, 0};

    /* StyleLayerShared uses the *_SHARED_SUBCLASS_IMPLEMENTATION() macro, this
       verifies that all the overrides do what's expected */
    StyleLayerShared* chaining;
    if(data.templated) chaining = &shared.setStyleTransition<StyleIndex,
        styleIndexTransitionToPressedOut,
        styleIndexTransitionToPressedOver,
        styleIndexTransitionToInactiveOut,
        styleIndexTransitionToInactiveOver,
        /* toDisabled transition is tested in eventStyleTransitionDisabled()
           instead */
        styleIndexTransitionToDisabledDoNotCall>();
    else chaining = &shared.setStyleTransition(
        [](UnsignedInt s) {
            return UnsignedInt(styleIndexTransitionToPressedOut(StyleIndex(s)));
        },
        [](UnsignedInt s) {
            return UnsignedInt(styleIndexTransitionToPressedOver(StyleIndex(s)));
        },
        [](UnsignedInt s) {
            return UnsignedInt(styleIndexTransitionToInactiveOut(StyleIndex(s)));
        },
        [](UnsignedInt s) {
            return UnsignedInt(styleIndexTransitionToInactiveOver(StyleIndex(s)));
        },
        /* toDisabled transition is tested in eventStyleTransitionDisabled()
           instead */
        [](UnsignedInt s) {
            return UnsignedInt(styleIndexTransitionToDisabledDoNotCall(StyleIndex(s)));
        });
    CORRADE_COMPARE(chaining, &shared);

    AbstractUserInterface ui{{100, 100}};

    /*   1  2  3  4  5  6
       2 +-----+  +-----+
       3 |green|  | red |
       4 +-----+  +-----+
       5 +-----+  +-----+
       6 |blue |  |white|
       7 +-----+  +-----+ */
    NodeHandle nodeGreen = ui.createNode({1.0f, 2.0f}, {2.0f, 2.0f});
    NodeHandle nodeRed = ui.createNode({4.0f, 2.0f}, {2.0f, 2.0f});
    NodeHandle nodeBlue = ui.createNode({1.0f, 5.0f}, {2.0f, 2.0f});
    NodeHandle nodeWhite = ui.createNode({4.0f, 5.0f}, {2.0f, 2.0f});

    StyleLayer& layer = ui.setLayerInstance(Containers::pointer<StyleLayer>(ui.createLayer(), shared));
    /* One extra data to verify it's mapping from nodes to data correctly */
    layer.create(StyleIndex::Green);
    DataHandle dataGreen = layer.create(StyleIndex::Green, nodeGreen);
    DataHandle dataRed = layer.create(StyleIndex::Red, nodeRed);
    DataHandle dataBlue = layer.create(StyleIndex::Blue, nodeBlue);
    DataHandle dataWhite = layer.create(StyleIndex::White, nodeWhite);

    ui.update();
    CORRADE_COMPARE(layer.state(), LayerStates{});
    /* The style could be simply copied to calculatedStyles after an update as
       no transition is set */
    CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataGreen)]), StyleIndex::Green);
    CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataRed)]), StyleIndex::Red);
    CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataBlue)]), StyleIndex::Blue);
    CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataWhite)]), StyleIndex::White);

    /* Press on the green node. The node isn't registered as hovered, so it's
       a press without a hover. Which usually happens with taps, for example,
       although it's not restricted to a particular Pointer type. */
    {
        PointerEvent event{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({2.0f, 3.0f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), nodeGreen);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataGreen), StyleIndex::GreenPressed);
        CORRADE_COMPARE(layer.state(), LayerState::NeedsUpdate);
    }

    /* Presence (or not) of the update call tests two things -- that the
       NeedsUpdate flag is set for each event properly, and that the style is
       changed independently of whether the layer needs update or not */
    if(data.update) {
        ui.update();
        CORRADE_COMPARE(layer.state(), LayerStates{});
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataGreen)]), StyleIndex::GreenPressed);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataRed)]), StyleIndex::Red);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataBlue)]), StyleIndex::Blue);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataWhite)]), StyleIndex::White);
    }

    /* Release on the green node. Again, the node isn't registered as hovered,
       so neither the hover stays. */
    {
        PointerEvent event{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerReleaseEvent({2.5f, 2.5f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataGreen), StyleIndex::Green);
        CORRADE_COMPARE(layer.state(), LayerState::NeedsUpdate);
    }

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(layer.state(), LayerStates{});
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataGreen)]), StyleIndex::Green);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataRed)]), StyleIndex::Red);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataBlue)]), StyleIndex::Blue);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataWhite)]), StyleIndex::White);
    }

    /* Move on the red node makes it hovered */
    {
        PointerMoveEvent event{{}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({5.0f, 3.0f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), nodeRed);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataRed), StyleIndex::RedHover);
        CORRADE_COMPARE(layer.state(), LayerState::NeedsUpdate);
    }

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(layer.state(), LayerStates{});
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataGreen)]), StyleIndex::Green);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataRed)]), StyleIndex::RedHover);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataBlue)]), StyleIndex::Blue);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataWhite)]), StyleIndex::White);
    }

    /* Tap on it makes it hovered & pressed */
    {
        PointerEvent event{Pointer::Finger};
        CORRADE_VERIFY(ui.pointerPressEvent({4.5f, 3.5f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), nodeRed);
        CORRADE_COMPARE(ui.currentHoveredNode(), nodeRed);
        CORRADE_COMPARE(ui.currentCapturedNode(), nodeRed);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataRed), StyleIndex::RedPressedHover);
        CORRADE_COMPARE(layer.state(), LayerState::NeedsUpdate);
    }

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(layer.state(), LayerStates{});
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataGreen)]), StyleIndex::Green);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataRed)]), StyleIndex::RedPressedHover);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataBlue)]), StyleIndex::Blue);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataWhite)]), StyleIndex::White);
    }

    /* Move away makes it only pressed, without hover, as implicit capture is
       in effect */
    {
        PointerMoveEvent event{{}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({7.0f, 3.0f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), nodeRed);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentCapturedNode(), nodeRed);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataRed), StyleIndex::RedPressed);
        CORRADE_COMPARE(layer.state(), LayerState::NeedsUpdate);
    }

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(layer.state(), LayerStates{});
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataGreen)]), StyleIndex::Green);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataRed)]), StyleIndex::RedPressed);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataBlue)]), StyleIndex::Blue);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataWhite)]), StyleIndex::White);
    }

    /* Move back makes it hovered & pressed again */
    {
        PointerMoveEvent event{{}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({5.5f, 3.0f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), nodeRed);
        CORRADE_COMPARE(ui.currentHoveredNode(), nodeRed);
        CORRADE_COMPARE(ui.currentCapturedNode(), nodeRed);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataRed), StyleIndex::RedPressedHover);
        CORRADE_COMPARE(layer.state(), LayerState::NeedsUpdate);
    }

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(layer.state(), LayerStates{});
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataGreen)]), StyleIndex::Green);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataRed)]), StyleIndex::RedPressedHover);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataBlue)]), StyleIndex::Blue);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataWhite)]), StyleIndex::White);
    }

    /* Release makes it only hover again */
    {
        PointerEvent event{Pointer::Finger};
        CORRADE_VERIFY(ui.pointerReleaseEvent({5.0f, 2.5f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), nodeRed);
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataRed), StyleIndex::RedHover);
        CORRADE_COMPARE(layer.state(), LayerState::NeedsUpdate);
    }

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(layer.state(), LayerStates{});
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataGreen)]), StyleIndex::Green);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataRed)]), StyleIndex::RedHover);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataBlue)]), StyleIndex::Blue);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataWhite)]), StyleIndex::White);
    }

    /* Move away makes it not hovered anymore */
    {
        PointerMoveEvent event{{}, {}};
        CORRADE_VERIFY(!ui.pointerMoveEvent({7.0f, 2.5f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataRed), StyleIndex::Red);
        CORRADE_COMPARE(layer.state(), LayerState::NeedsUpdate);
    }

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(layer.state(), LayerStates{});
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataGreen)]), StyleIndex::Green);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataRed)]), StyleIndex::Red);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataBlue)]), StyleIndex::Blue);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataWhite)]), StyleIndex::White);
    }

    /* Move on and away from the blue is accepted but makes no change to it,
       thus no update is needed */
    {
        PointerMoveEvent event{{}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({2.0f, 6.0f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), nodeBlue);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataBlue), StyleIndex::Blue);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    } {
        PointerMoveEvent event{{}, {}};
        CORRADE_VERIFY(!ui.pointerMoveEvent({2.5f, 8.0f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataBlue), StyleIndex::Blue);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }

    /* Press and release on the white is accepted but makes no change to it,
       thus no update is needed */
    {
        PointerEvent event{Pointer::Pen};
        CORRADE_VERIFY(ui.pointerPressEvent({5.0f, 5.0f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), nodeWhite);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentCapturedNode(), nodeWhite);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataWhite), StyleIndex::White);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    } {
        PointerEvent event{Pointer::Pen};
        CORRADE_VERIFY(ui.pointerReleaseEvent({5.5f, 4.5f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataWhite), StyleIndex::White);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }

    /* Press and release on the green node again, but with a right click. Such
       event isn't even accepted and should cause no change either. */
    {
        PointerEvent event{Pointer::MouseRight};
        CORRADE_VERIFY(!ui.pointerPressEvent({2.0f, 3.0f}, event));
        CORRADE_COMPARE(layer.style<StyleIndex>(dataGreen), StyleIndex::Green);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    } {
        PointerEvent event{Pointer::MouseRight};
        CORRADE_VERIFY(!ui.pointerReleaseEvent({1.5f, 2.5f}, event));
        CORRADE_COMPARE(layer.style<StyleIndex>(dataGreen), StyleIndex::Green);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }
}

void AbstractVisualLayerTest::eventStyleTransitionNoHover() {
    auto&& data = EventStyleTransitionData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Transition for dynamic styles tested in
       eventStyleTransitionDynamicStyle() instead */
    StyleLayerShared shared{4, 0};

    AbstractUserInterface ui{{100, 100}};

    NodeHandle node = ui.createNode({1.0f, 1.0f}, {2.0f, 2.0f});

    StyleLayer& layer = ui.setLayerInstance(Containers::pointer<StyleLayer>(ui.createLayer(), shared));
    DataHandle layerData = layer.create(StyleIndex::Green, node);

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }

    /* StyleLayerShared uses the *_SHARED_SUBCLASS_IMPLEMENTATION() macro, this
       verifies that all the overrides do what's expected */
    StyleLayerShared* chaining;
    if(data.templated) chaining = &shared.setStyleTransition<StyleIndex,
        styleIndexTransitionToPressedOut,
        styleIndexTransitionToInactiveOut,
        /* "no hover" toDisabled transition is tested in
           eventStyleTransitionDisabled() instead */
        styleIndexTransitionToDisabledDoNotCall>();
    else chaining = &shared.setStyleTransition(
        [](UnsignedInt s) {
            return UnsignedInt(styleIndexTransitionToPressedOut(StyleIndex(s)));
        },
        [](UnsignedInt s) {
            return UnsignedInt(styleIndexTransitionToInactiveOut(StyleIndex(s)));
        },
        /* "no hover" toDisabled transition is tested in
           eventStyleTransitionDisabled() instead */
        [](UnsignedInt s) {
            return UnsignedInt(styleIndexTransitionToDisabledDoNotCall(StyleIndex(s)));
        });
    CORRADE_COMPARE(chaining, &shared);

    auto testPressRelease = [&]{
        {
            PointerEvent event{Pointer::MouseLeft};
            CORRADE_VERIFY(ui.pointerPressEvent({2.5f, 2.0f}, event));
            CORRADE_COMPARE(layer.style<StyleIndex>(layerData), StyleIndex::GreenPressed);
            CORRADE_COMPARE(layer.state(), LayerState::NeedsUpdate);
        }

        if(data.update) {
            ui.update();
            CORRADE_COMPARE(layer.state(), LayerStates{});
        }

        {
            PointerEvent event{Pointer::MouseLeft};
            CORRADE_VERIFY(ui.pointerReleaseEvent({2.5f, 2.5f}, event));
            CORRADE_COMPARE(layer.style<StyleIndex>(layerData), StyleIndex::Green);
            CORRADE_COMPARE(layer.state(), LayerState::NeedsUpdate);
        }

        if(data.update) {
            ui.update();
            CORRADE_COMPARE(layer.state(), LayerStates{});
        }
    };

    /* Test press & release without a hover */
    testPressRelease();

    /* Moving onto the node should do nothing */
    {
        PointerMoveEvent event{{}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({2.0f, 2.0f}, event));
        CORRADE_COMPARE(ui.currentHoveredNode(), node);
        CORRADE_COMPARE(layer.style<StyleIndex>(layerData), StyleIndex::Green);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }

    /* Press & release with a hover should behave the same as without */
    testPressRelease();

    /* Moving away should do nothing again */
    {
        PointerMoveEvent event{{}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({2.0f, 2.0f}, event));
        CORRADE_COMPARE(ui.currentHoveredNode(), node);
        CORRADE_COMPARE(layer.style<StyleIndex>(layerData), StyleIndex::Green);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }
}

void AbstractVisualLayerTest::eventStyleTransitionDisabled() {
    auto&& data = EventStyleTransitionDisabledData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    AbstractUserInterface ui{{100, 100}};

    /* Mark every other node as disabled */
    NodeHandle nodeGreen = ui.createNode({}, {100, 100});
    NodeHandle nodeRed = ui.createNode({}, {100, 100}, NodeFlag::Disabled);
    NodeHandle nodeBlue = ui.createNode({}, {100, 100});
    NodeHandle nodeWhite = ui.createNode({}, {100, 100}, NodeFlag::Disabled);

    /* Transition for dynamic styles tested in
       eventStyleTransitionDynamicStyle() instead */
    StyleLayerShared shared{14, 0};
    StyleLayer& layer = ui.setLayerInstance(Containers::pointer<StyleLayer>(ui.createLayer(), shared));
    /* One extra data to verify it's mapping from nodes to data correctly */
    layer.create(StyleIndex::Green);
    DataHandle dataGreen = layer.create(StyleIndex::Green, nodeGreen);
    DataHandle dataRed = layer.create(StyleIndex::Red, nodeRed);
    DataHandle dataBlue = layer.create(StyleIndex::Blue, nodeBlue);
    DataHandle dataWhite = layer.create(StyleIndex::White, nodeWhite);

    /* There should be no style change from the input to the calculated by
       default */
    ui.update();
    CORRADE_COMPARE(layer.style<StyleIndex>(dataGreen), StyleIndex::Green);
    CORRADE_COMPARE(layer.style<StyleIndex>(dataRed), StyleIndex::Red);
    CORRADE_COMPARE(layer.style<StyleIndex>(dataBlue), StyleIndex::Blue);
    CORRADE_COMPARE(layer.style<StyleIndex>(dataWhite), StyleIndex::White);
    CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataGreen)]), StyleIndex::Green);
    CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataRed)]), StyleIndex::Red);
    CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataBlue)]), StyleIndex::Blue);
    CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataWhite)]), StyleIndex::White);

    /* Set a style transition. Only the nodes that are marked as Disabled
       should change now. StyleLayerShared uses the
       *_SHARED_SUBCLASS_IMPLEMENTATION() macro, this verifies that all the
       overrides do what's expected */
    StyleLayerShared* chaining;
    if(data.templated) chaining =
    &shared.setStyleTransition<StyleIndex,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        styleIndexTransitionToDisabled>();
    else chaining = &shared.setStyleTransition(
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        [](UnsignedInt s) {
            return UnsignedInt(styleIndexTransitionToDisabled(StyleIndex(s)));
        });
    CORRADE_COMPARE(chaining, &shared);
    /** @todo make this implicit from setStyleTransition() somehow? */
    layer.setNeedsUpdate();
    ui.update();
    CORRADE_COMPARE(layer.style<StyleIndex>(dataGreen), StyleIndex::Green);
    CORRADE_COMPARE(layer.style<StyleIndex>(dataRed), StyleIndex::Red);
    CORRADE_COMPARE(layer.style<StyleIndex>(dataBlue), StyleIndex::Blue);
    CORRADE_COMPARE(layer.style<StyleIndex>(dataWhite), StyleIndex::White);
    CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataGreen)]), StyleIndex::Green);
    CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataRed)]), StyleIndex::RedBlueDisabled);
    CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataBlue)]), StyleIndex::Blue);
    /* White doesn't have any transition implemented */
    CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataWhite)]), StyleIndex::White);

    /* Changing the flags should result in the other nodes being marked */
    ui.setNodeFlags(nodeGreen, NodeFlag::Disabled);
    /* NoEvents shouldn't be treated the same as Disabled */
    ui.setNodeFlags(nodeRed, NodeFlag::NoEvents);
    ui.setNodeFlags(nodeBlue, NodeFlag::Disabled);
    ui.setNodeFlags(nodeWhite, NodeFlags{});
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeEnabledUpdate);

    ui.update();
    CORRADE_COMPARE(layer.style<StyleIndex>(dataGreen), StyleIndex::Green);
    CORRADE_COMPARE(layer.style<StyleIndex>(dataRed), StyleIndex::Red);
    CORRADE_COMPARE(layer.style<StyleIndex>(dataBlue), StyleIndex::Blue);
    CORRADE_COMPARE(layer.style<StyleIndex>(dataWhite), StyleIndex::White);
    CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataGreen)]), StyleIndex::GreenDisabled);
    CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataRed)]), StyleIndex::Red);
    CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataBlue)]), StyleIndex::RedBlueDisabled);
    CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataWhite)]), StyleIndex::White);

    /* Setting a no-op transition should revert back */
    if(data.templated) shared.setStyleTransition<StyleIndex,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr>();
    else shared.setStyleTransition(
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr);
    /** @todo make this implicit from setStyleTransition() somehow? */
    layer.setNeedsUpdate();
    ui.update();
    CORRADE_COMPARE(layer.style<StyleIndex>(dataGreen), StyleIndex::Green);
    CORRADE_COMPARE(layer.style<StyleIndex>(dataRed), StyleIndex::Red);
    CORRADE_COMPARE(layer.style<StyleIndex>(dataBlue), StyleIndex::Blue);
    CORRADE_COMPARE(layer.style<StyleIndex>(dataWhite), StyleIndex::White);
    CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataGreen)]), StyleIndex::Green);
    CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataRed)]), StyleIndex::Red);
    CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataBlue)]), StyleIndex::Blue);
    CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataWhite)]), StyleIndex::White);

    /* Set a no-hover style transition. The nodes that are marked as Disabled
       should change back again. */
    shared.setStyleTransition<StyleIndex,
        nullptr,
        nullptr,
        styleIndexTransitionToDisabled>();
    /** @todo make this implicit from setStyleTransition() somehow? */
    layer.setNeedsUpdate();
    ui.update();
    CORRADE_COMPARE(layer.style<StyleIndex>(dataGreen), StyleIndex::Green);
    CORRADE_COMPARE(layer.style<StyleIndex>(dataRed), StyleIndex::Red);
    CORRADE_COMPARE(layer.style<StyleIndex>(dataBlue), StyleIndex::Blue);
    CORRADE_COMPARE(layer.style<StyleIndex>(dataWhite), StyleIndex::White);
    CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataGreen)]), StyleIndex::GreenDisabled);
    CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataRed)]), StyleIndex::Red);
    CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataBlue)]), StyleIndex::RedBlueDisabled);
    /* White doesn't have any transition implemented */
    CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataWhite)]), StyleIndex::White);

    /* Setting a no-op no-hover transition should revert back again */
    shared.setStyleTransition<StyleIndex,
        nullptr,
        nullptr,
        nullptr>();
    /** @todo make this implicit from setStyleTransition() somehow? */
    layer.setNeedsUpdate();
    ui.update();
    CORRADE_COMPARE(layer.style<StyleIndex>(dataGreen), StyleIndex::Green);
    CORRADE_COMPARE(layer.style<StyleIndex>(dataRed), StyleIndex::Red);
    CORRADE_COMPARE(layer.style<StyleIndex>(dataBlue), StyleIndex::Blue);
    CORRADE_COMPARE(layer.style<StyleIndex>(dataWhite), StyleIndex::White);
    CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataGreen)]), StyleIndex::Green);
    CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataRed)]), StyleIndex::Red);
    CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataBlue)]), StyleIndex::Blue);
    CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataWhite)]), StyleIndex::White);
}

void AbstractVisualLayerTest::eventStyleTransitionNoCapture() {
    auto&& data = EventStyleTransitionNoCaptureData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Transition for dynamic styles tested in
       eventStyleTransitionDynamicStyle() instead */
    StyleLayerShared shared{4, 0};
    shared.setStyleTransition<StyleIndex,
        styleIndexTransitionToPressedOut,
        styleIndexTransitionToPressedOver,
        styleIndexTransitionToInactiveOut,
        styleIndexTransitionToInactiveOver,
        styleIndexTransitionToDisabled>();

    struct EventLayer: AbstractLayer {
        explicit EventLayer(LayerHandle handle, bool disableCapture): AbstractLayer{handle}, disableCapture{disableCapture} {}

        using AbstractLayer::create;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Event;
        }

        void doPointerPressEvent(UnsignedInt, PointerEvent& event) override {
            if(disableCapture) {
                event.setCaptured(false);
                event.setAccepted();
            }
        }

        bool disableCapture;
    };

    AbstractUserInterface ui{{100, 100}};

    NodeHandle node = ui.createNode({1.0f, 1.0f}, {2.0f, 2.0f});

    StyleLayer& layer = ui.setLayerInstance(Containers::pointer<StyleLayer>(ui.createLayer(), shared));
    DataHandle layerData = layer.create(StyleIndex::Green, node);

    EventLayer& eventLayer = ui.setLayerInstance(Containers::pointer<EventLayer>(ui.createLayer(), data.disableCapture));
    eventLayer.create(node);

    ui.update();
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Move onto the node is capture-independent */
    {
        PointerMoveEvent event{{}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({2.0f, 2.0f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), node);
        CORRADE_COMPARE(layer.style<StyleIndex>(layerData), StyleIndex::GreenHover);

    /* Press will enable the capture, maybe */
    } {
        PointerEvent event{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({2.5f, 2.0f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), node);
        CORRADE_COMPARE(ui.currentCapturedNode(), data.disableCapture ? NodeHandle::Null : node);
        CORRADE_COMPARE(layer.style<StyleIndex>(layerData), StyleIndex::GreenPressedHover);

    /* Move away will only preserve the press if capture is set */
    } {
        PointerMoveEvent event{{}, {}};
        CORRADE_COMPARE(ui.pointerMoveEvent({7.0f, 2.0f}, event), !data.disableCapture);
        CORRADE_COMPARE(ui.currentPressedNode(), data.disableCapture ? NodeHandle::Null : node);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(layerData), data.outStyle);

    /* Move back will only preserve the press if capture is set */
    } {
        PointerMoveEvent event{{}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({2.0f, 2.0f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), data.disableCapture ? NodeHandle::Null : node);
        CORRADE_COMPARE(ui.currentHoveredNode(), node);
        CORRADE_COMPARE(layer.style<StyleIndex>(layerData), data.overStyle);
    }
}

StyleIndex styleIndexTransitionOutOfRange(StyleIndex) {
    return StyleIndex(14);
}

void AbstractVisualLayerTest::eventStyleTransitionOutOfRange() {
    auto&& data = EventStyleTransitionOutOfRangeData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    CORRADE_SKIP_IF_NO_ASSERT();

    /* Style transition isn't performed on dynamic styles so this shouldn't
       affect it */
    StyleLayerShared shared{14, data.dynamicStyleCount};

    AbstractUserInterface ui{{100, 100}};

    NodeHandle node = ui.createNode({1.0f, 1.0f}, {2.0f, 2.0f});

    StyleLayer& layer = ui.setLayerInstance(Containers::pointer<StyleLayer>(ui.createLayer(), shared));
    layer.create(StyleIndex::Red, node);

    ui.update();
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Tests an OOB condition happening in any of the four functions, and
       checked in any of the four event handlers. Does not exhaustively test
       all possible combinations, as that should not be needed.

       The same logic is used in eventStyleTransitionDynamicStyle() to exhaust
       all possibilities, keep in sync. */

    /* OOB toPressedOut transition */
    shared.setStyleTransition<StyleIndex,
        styleIndexTransitionOutOfRange,
        styleIndexTransitionToPressedOver,
        styleIndexTransitionToInactiveOut,
        styleIndexTransitionToInactiveOver,
        styleIndexTransitionToDisabledDoNotCall>();
    {
        PointerEvent event{Pointer::MouseLeft};

        std::ostringstream out;
        Error redirectError{&out};
        ui.pointerPressEvent({2.0f, 2.0f}, event);
        CORRADE_COMPARE(out.str(), "Whee::AbstractVisualLayer::pointerPressEvent(): style transition from 4 to 14 out of range for 14 styles\n");
    }

    /* OOB toPressedOver transition in the press event. Doing a
       (non-asserting) move before so the hovered node is properly
       registered. */
    shared.setStyleTransition<StyleIndex,
        styleIndexTransitionToPressedOut,
        styleIndexTransitionOutOfRange,
        styleIndexTransitionToInactiveOut,
        styleIndexTransitionToInactiveOver,
        styleIndexTransitionToDisabledDoNotCall>();
    {
        PointerMoveEvent moveEvent{{}, {}};
        ui.pointerMoveEvent({1.5f, 2.0f}, moveEvent);
        PointerEvent event{Pointer::MouseLeft};

        std::ostringstream out;
        Error redirectError{&out};
        ui.pointerPressEvent({2.0f, 2.0f}, event);
        CORRADE_COMPARE(out.str(), "Whee::AbstractVisualLayer::pointerPressEvent(): style transition from 5 to 14 out of range for 14 styles\n");
    }

    /* OOB toInactiveOver transition */
    shared.setStyleTransition<StyleIndex,
        styleIndexTransitionToPressedOut,
        styleIndexTransitionToPressedOver,
        styleIndexTransitionToInactiveOut,
        styleIndexTransitionOutOfRange,
        styleIndexTransitionToDisabledDoNotCall>();
    {
        PointerEvent event{Pointer::MouseLeft};

        std::ostringstream out;
        Error redirectError{&out};
        ui.pointerReleaseEvent({1.5f, 2.5f}, event);
        CORRADE_COMPARE(out.str(), "Whee::AbstractVisualLayer::pointerReleaseEvent(): style transition from 5 to 14 out of range for 14 styles\n");
    }

    /* OOB toInactiveOut transition in the leave event */
    shared.setStyleTransition<StyleIndex,
        styleIndexTransitionToPressedOut,
        styleIndexTransitionToPressedOver,
        styleIndexTransitionOutOfRange,
        styleIndexTransitionToInactiveOver,
        styleIndexTransitionToDisabledDoNotCall>();
    {
        PointerMoveEvent event{{}, {}};

        std::ostringstream out;
        Error redirectError{&out};
        ui.pointerMoveEvent({8.5f, 2.0f}, event);
        CORRADE_COMPARE(out.str(), "Whee::AbstractVisualLayer::pointerLeaveEvent(): style transition from 5 to 14 out of range for 14 styles\n");
    }

    /* OOB toInactiveOver transition in the enter event */
    shared.setStyleTransition<StyleIndex,
        styleIndexTransitionToPressedOut,
        styleIndexTransitionToPressedOver,
        styleIndexTransitionToInactiveOut,
        styleIndexTransitionOutOfRange,
        styleIndexTransitionToDisabledDoNotCall>();
    {
        PointerMoveEvent event{{}, {}};

        std::ostringstream out;
        Error redirectError{&out};
        ui.pointerMoveEvent({1.5f, 2.0f}, event);
        CORRADE_COMPARE(out.str(), "Whee::AbstractVisualLayer::pointerEnterEvent(): style transition from 5 to 14 out of range for 14 styles\n");
    }

    /* OOB toDisabled transition in doUpdate() */
    shared.setStyleTransition<StyleIndex,
        styleIndexTransitionToPressedOut,
        styleIndexTransitionToPressedOver,
        styleIndexTransitionToInactiveOut,
        styleIndexTransitionToInactiveOver,
        styleIndexTransitionOutOfRange>();
    ui.addNodeFlags(node, NodeFlag::Disabled);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeEnabledUpdate);
    {
        std::ostringstream out;
        Error redirectError{&out};
        ui.update();
        CORRADE_COMPARE(out.str(), "Whee::AbstractVisualLayer::update(): style transition from 5 to 14 out of range for 14 styles\n");
    }
}

void AbstractVisualLayerTest::eventStyleTransitionDynamicStyle() {
    /* There's 14 styles, style ID 14 is a dynamic style */
    StyleLayerShared shared{14, 1};

    AbstractUserInterface ui{{100, 100}};

    NodeHandle node = ui.createNode({1.0f, 1.0f}, {2.0f, 2.0f});

    StyleLayer& layer = ui.setLayerInstance(Containers::pointer<StyleLayer>(ui.createLayer(), shared));
    DataHandle data = layer.create(StyleIndex::Green, node);
    DataHandle dataDynamic = layer.create(14, node);

    ui.update();
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* All these should get called only for the non-dynamic style. Logic the
       same as in eventStyleTransitionOutOfRange(), just not asserting in this
       case. Keep the two in sync. */
    shared.setStyleTransition<StyleIndex,
        styleIndexTransitionToPressedOut,
        styleIndexTransitionToPressedOver,
        styleIndexTransitionToInactiveOut,
        styleIndexTransitionToInactiveOver,
        styleIndexTransitionToDisabled>();

    /* toPressedOut transition */
    {
        PointerEvent event{Pointer::MouseLeft};
        ui.pointerPressEvent({2.0f, 2.0f}, event);
        CORRADE_COMPARE(ui.currentPressedNode(), node);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(data), StyleIndex::GreenPressed);
        CORRADE_COMPARE(layer.style(dataDynamic), 14);

    /* toPressedOver transition in the press event. Doing a move before so the
       hovered node is properly registered. */
    } {
        PointerMoveEvent moveEvent{{}, {}};
        ui.pointerMoveEvent({1.5f, 2.0f}, moveEvent);

        PointerEvent event{Pointer::MouseLeft};
        ui.pointerPressEvent({2.0f, 2.0f}, event);
        CORRADE_COMPARE(ui.currentPressedNode(), node);
        CORRADE_COMPARE(ui.currentHoveredNode(), node);
        CORRADE_COMPARE(layer.style<StyleIndex>(data), StyleIndex::GreenPressedHover);
        CORRADE_COMPARE(layer.style(dataDynamic), 14);

    /* toInactiveOver transition */
    } {
        PointerEvent event{Pointer::MouseLeft};
        ui.pointerReleaseEvent({1.5f, 2.5f}, event);
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), node);
        CORRADE_COMPARE(layer.style<StyleIndex>(data), StyleIndex::GreenHover);
        CORRADE_COMPARE(layer.style(dataDynamic), 14);

    /* toInactiveOut transition in the leave event */
    } {
        PointerMoveEvent event{{}, {}};
        ui.pointerMoveEvent({8.5f, 2.0f}, event);
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(data), StyleIndex::Green);
        CORRADE_COMPARE(layer.style(dataDynamic), 14);

    /* toInactiveOver transition in the enter event */
    } {
        PointerMoveEvent event{{}, {}};
        ui.pointerMoveEvent({1.5f, 2.0f}, event);
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), node);
        CORRADE_COMPARE(layer.style<StyleIndex>(data), StyleIndex::GreenHover);
        CORRADE_COMPARE(layer.style(dataDynamic), 14);

    /* toDisabled transition in doUpdate() */
    } {
        ui.addNodeFlags(node, NodeFlag::Disabled);
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeEnabledUpdate);

        /* Only the calculated style changes, not the public one */
        ui.update();
        CORRADE_COMPARE(layer.style<StyleIndex>(data), StyleIndex::GreenHover);
        CORRADE_COMPARE(layer.style(dataDynamic), 14);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(data)]), StyleIndex::GreenDisabled);
        CORRADE_COMPARE(layer.stateData().calculatedStyles[dataHandleId(dataDynamic)], 14);
    }
}

}}}}

CORRADE_TEST_MAIN(Magnum::Whee::Test::AbstractVisualLayerTest)
