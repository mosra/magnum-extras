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

#include "Magnum/Whee/AbstractUserInterface.h"
#include "Magnum/Whee/AbstractVisualLayer.h"
#include "Magnum/Whee/Event.h"
#include "Magnum/Whee/Handle.h"
/* for setStyle(), eventStyleTransition*() */
#include "Magnum/Whee/Implementation/abstractVisualLayerState.h"

namespace Magnum { namespace Whee { namespace Test { namespace {

struct AbstractVisualLayerTest: TestSuite::Tester {
    explicit AbstractVisualLayerTest();

    void sharedConstruct();
    void sharedConstructNoCreate();
    void sharedConstructCopy();
    void sharedConstructMove();

    void construct();
    void constructCopy();
    void constructMove();

    template<class T> void setStyle();
    void invalidHandle();
    void styleOutOfRange();

    void eventStyleTransitionNoOp();
    void eventStyleTransition();
    void eventStyleTransitionNoHover();
    void eventStyleTransitionNoCapture();
    void eventStyleTransitionOutOfRange();
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
        #undef _c
    }

    return debug << UnsignedInt(value);
}

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
    bool disableCapture;
    StyleIndex blurStyle, hoverStyle;
} EventStyleTransitionNoCaptureData[]{
    {"", false, StyleIndex::GreenPressed, StyleIndex::GreenPressedHover},
    {"capture disabled", true, StyleIndex::Green, StyleIndex::GreenHover},
};

AbstractVisualLayerTest::AbstractVisualLayerTest() {
    addTests({&AbstractVisualLayerTest::sharedConstruct,
              &AbstractVisualLayerTest::sharedConstructNoCreate,
              &AbstractVisualLayerTest::sharedConstructCopy,
              &AbstractVisualLayerTest::sharedConstructMove,

              &AbstractVisualLayerTest::construct,
              &AbstractVisualLayerTest::constructCopy,
              &AbstractVisualLayerTest::constructMove,

              &AbstractVisualLayerTest::setStyle<UnsignedInt>,
              &AbstractVisualLayerTest::setStyle<Enum>,
              &AbstractVisualLayerTest::invalidHandle,
              &AbstractVisualLayerTest::styleOutOfRange,

              &AbstractVisualLayerTest::eventStyleTransitionNoOp});

    addInstancedTests({&AbstractVisualLayerTest::eventStyleTransition,
                       &AbstractVisualLayerTest::eventStyleTransitionNoHover},
        Containers::arraySize(EventStyleTransitionData));

    addInstancedTests({&AbstractVisualLayerTest::eventStyleTransitionNoCapture},
        Containers::arraySize(EventStyleTransitionNoCaptureData));

    addTests({&AbstractVisualLayerTest::eventStyleTransitionOutOfRange});
}

void AbstractVisualLayerTest::sharedConstruct() {
    struct Shared: AbstractVisualLayer::Shared {
        explicit Shared(UnsignedInt styleCount): AbstractVisualLayer::Shared{styleCount} {}
    } shared{3};
    CORRADE_COMPARE(shared.styleCount(), 3);
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
        explicit Shared(UnsignedInt styleCount): AbstractVisualLayer::Shared{styleCount} {}
    };

    Shared a{3};

    Shared b{Utility::move(a)};
    CORRADE_COMPARE(b.styleCount(), 3);

    Shared c{5};
    c = Utility::move(b);
    CORRADE_COMPARE(c.styleCount(), 3);

    CORRADE_VERIFY(std::is_nothrow_move_constructible<AbstractVisualLayer::Shared>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<AbstractVisualLayer::Shared>::value);
}

void AbstractVisualLayerTest::construct() {
    struct LayerShared: AbstractVisualLayer::Shared {
        explicit LayerShared(UnsignedInt styleCount): AbstractVisualLayer::Shared{styleCount} {}
    } shared{3};

    struct Layer: AbstractVisualLayer {
        explicit Layer(LayerHandle handle, Shared& shared): AbstractVisualLayer{handle, shared} {}
    } layer{layerHandle(137, 0xfe), shared};

    /* There isn't anything to query on the AbstractVisualLayer itself */
    CORRADE_COMPARE(layer.handle(), layerHandle(137, 0xfe));
}

void AbstractVisualLayerTest::constructCopy() {
    CORRADE_VERIFY(!std::is_copy_constructible<AbstractVisualLayer>{});
    CORRADE_VERIFY(!std::is_copy_assignable<AbstractVisualLayer>{});
}

void AbstractVisualLayerTest::constructMove() {
    struct LayerShared: AbstractVisualLayer::Shared {
        explicit LayerShared(UnsignedInt styleCount): AbstractVisualLayer::Shared{styleCount} {}
    };

    struct Layer: AbstractVisualLayer {
        explicit Layer(LayerHandle handle, Shared& shared): AbstractVisualLayer{handle, shared} {}
    };

    LayerShared shared{3};
    LayerShared shared2{5};

    Layer a{layerHandle(137, 0xfe), shared};

    Layer b{Utility::move(a)};
    /* There isn't anything to query on the AbstractVisualLayer itself */
    CORRADE_COMPARE(b.handle(), layerHandle(137, 0xfe));

    Layer c{layerHandle(0, 2), shared2};
    c = Utility::move(b);
    CORRADE_COMPARE(c.handle(), layerHandle(137, 0xfe));

    CORRADE_VERIFY(std::is_nothrow_move_constructible<AbstractVisualLayer>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<AbstractVisualLayer>::value);
}

/* These are shared by all cases that need to call create() below */
struct StyleLayerShared: AbstractVisualLayer::Shared {
    explicit StyleLayerShared(UnsignedInt styleCount): AbstractVisualLayer::Shared{styleCount} {}

    /* To verify that the macro correctly passes everything through. The Shared
       typedef is because the macro overrides return Shared&, which if not
       defined here would mean the base class. */
    typedef StyleLayerShared Shared;
    MAGNUMEXTRAS_WHEE_ABSTRACTVISUALLAYER_SHARED_SUBCLASS_IMPLEMENTATION()
};
struct StyleLayer: AbstractVisualLayer {
    explicit StyleLayer(LayerHandle handle, Shared& shared): AbstractVisualLayer{handle, shared} {}

    /* Just saves the style index and sync's the styles array */
    template<class T> DataHandle create(T style, NodeHandle node = NodeHandle::Null) {
        const DataHandle handle = AbstractVisualLayer::create(node);
        const UnsignedInt id = dataHandleId(handle);
        if(id >= data.size()) {
            arrayAppend(data, NoInit, id - data.size() + 1);
            _state->styles = data;
        }
        data[id] = UnsignedInt(style);
        return handle;
    }

    Containers::Array<UnsignedInt> data;
};

template<class T> void AbstractVisualLayerTest::setStyle() {
    setTestCaseTemplateName(std::is_same<T, Enum>::value ? "Enum" : "UnsignedInt");

    StyleLayerShared shared{67};
    StyleLayer layer{layerHandle(0, 1), shared};

    /* Just to be sure the setters aren't picking up the first ever data
       always */
    layer.create(2);

    DataHandle data = layer.create(17);
    CORRADE_COMPARE(layer.style(data), 17);
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Setting a style marks the layer as dirty */
    layer.setStyle(data, T(37));
    CORRADE_COMPARE(layer.style(data), 37);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsUpdate);

    /* Testing also the other overload */
    layer.setStyle(dataHandleData(data), T(66));
    CORRADE_COMPARE(layer.style(data), 66);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsUpdate);
}

void AbstractVisualLayerTest::invalidHandle() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct LayerShared: AbstractVisualLayer::Shared {
        explicit LayerShared(UnsignedInt styleCount): AbstractVisualLayer::Shared{styleCount} {}
    } shared{1};

    struct Layer: AbstractVisualLayer {
        explicit Layer(LayerHandle handle, Shared& shared): AbstractVisualLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    std::ostringstream out;
    Error redirectError{&out};
    layer.style(DataHandle::Null);
    layer.style(LayerDataHandle::Null);
    layer.setStyle(DataHandle::Null, 0);
    layer.setStyle(LayerDataHandle::Null, 0);
    CORRADE_COMPARE(out.str(),
        "Whee::AbstractVisualLayer::style(): invalid handle Whee::DataHandle::Null\n"
        "Whee::AbstractVisualLayer::style(): invalid handle Whee::LayerDataHandle::Null\n"
        "Whee::AbstractVisualLayer::setStyle(): invalid handle Whee::DataHandle::Null\n"
        "Whee::AbstractVisualLayer::setStyle(): invalid handle Whee::LayerDataHandle::Null\n");
}

void AbstractVisualLayerTest::styleOutOfRange() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct LayerShared: AbstractVisualLayer::Shared {
        explicit LayerShared(UnsignedInt styleCount): AbstractVisualLayer::Shared{styleCount} {}
    } shared{3};

    struct Layer: AbstractVisualLayer {
        explicit Layer(LayerHandle handle, Shared& shared): AbstractVisualLayer{handle, shared} {}

        using AbstractVisualLayer::create;
    } layer{layerHandle(0, 1), shared};

    DataHandle data = layer.create();

    std::ostringstream out;
    Error redirectError{&out};
    layer.setStyle(data, 3);
    layer.setStyle(dataHandleData(data), 3);
    CORRADE_COMPARE(out.str(),
        "Whee::AbstractVisualLayer::setStyle(): style 3 out of range for 3 styles\n"
        "Whee::AbstractVisualLayer::setStyle(): style 3 out of range for 3 styles\n");
}

StyleIndex styleIndexTransitionToInactiveBlur(StyleIndex index) {
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
    }
    CORRADE_FAIL("Called with" << UnsignedInt(index));
    CORRADE_INTERNAL_ASSERT_UNREACHABLE();
}

StyleIndex styleIndexTransitionToInactiveHover(StyleIndex index) {
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
    }
    CORRADE_FAIL("Called with" << UnsignedInt(index));
    CORRADE_INTERNAL_ASSERT_UNREACHABLE();
}

StyleIndex styleIndexTransitionToPressedBlur(StyleIndex index) {
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
    }
    CORRADE_FAIL("Called with" << UnsignedInt(index));
    CORRADE_INTERNAL_ASSERT_UNREACHABLE();
}

StyleIndex styleIndexTransitionToPressedHover(StyleIndex index) {
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
    }
    CORRADE_FAIL("Called with" << UnsignedInt(index));
    CORRADE_INTERNAL_ASSERT_UNREACHABLE();
}

void AbstractVisualLayerTest::eventStyleTransitionNoOp() {
    StyleLayerShared shared{12};

    AbstractUserInterface ui{{100, 100}};

    NodeHandle node = ui.createNode({1.0f, 1.0f}, {2.0f, 2.0f});

    StyleLayer& layer = ui.setLayerInstance(Containers::pointer<StyleLayer>(ui.createLayer(), shared));
    /* Deliberately setting a style that isn't the "default" */
    DataHandle data = layer.create(StyleIndex::GreenPressedHover, node);

    ui.update();
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Press, release, hover, hovered press, hovered release, blur should all
       do nothing by default */
    {
        PointerEvent event{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({2.0f, 2.0f}, event));
        CORRADE_COMPARE(ui.pointerEventPressedNode(), node);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(data), StyleIndex::GreenPressedHover);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    } {
        PointerEvent event{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerReleaseEvent({2.5f, 2.5f}, event));
        CORRADE_COMPARE(ui.pointerEventPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(data), StyleIndex::GreenPressedHover);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    } {
        PointerMoveEvent event{{}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({2.0f, 2.0f}, event));
        CORRADE_COMPARE(ui.pointerEventPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), node);
        CORRADE_COMPARE(layer.style<StyleIndex>(data), StyleIndex::GreenPressedHover);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    } {
        PointerEvent event{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({2.0f, 2.0f}, event));
        CORRADE_COMPARE(ui.pointerEventPressedNode(), node);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), node);
        CORRADE_COMPARE(layer.style<StyleIndex>(data), StyleIndex::GreenPressedHover);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    } {
        PointerEvent event{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerReleaseEvent({2.5f, 2.5f}, event));
        CORRADE_COMPARE(ui.pointerEventPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), node);
        CORRADE_COMPARE(layer.style<StyleIndex>(data), StyleIndex::GreenPressedHover);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    } {
        PointerMoveEvent event{{}, {}};
        CORRADE_VERIFY(!ui.pointerMoveEvent({5.0f, 2.0f}, event));
        CORRADE_COMPARE(ui.pointerEventPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(data), StyleIndex::GreenPressedHover);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }

    /* Setting a null toPressedBlur transition will do nothing for a press */
    shared.setStyleTransition<StyleIndex,
        nullptr,
        styleIndexTransitionToPressedHover,
        styleIndexTransitionToInactiveBlur,
        styleIndexTransitionToInactiveHover>();
    {
        PointerEvent event{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({2.0f, 2.0f}, event));
        CORRADE_COMPARE(ui.pointerEventPressedNode(), node);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(data), StyleIndex::GreenPressedHover);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }

    /* Setting a null toInactiveBlur transition will do nothing for a release */
    shared.setStyleTransition<StyleIndex,
        styleIndexTransitionToPressedBlur,
        styleIndexTransitionToPressedHover,
        nullptr,
        styleIndexTransitionToInactiveHover>();
    {
        PointerEvent event{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerReleaseEvent({2.5f, 2.5f}, event));
        CORRADE_COMPARE(ui.pointerEventPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(data), StyleIndex::GreenPressedHover);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }

    /* Setting a null toInactiveHover will do nothing for a hover */
    shared.setStyleTransition<StyleIndex,
        styleIndexTransitionToPressedBlur,
        styleIndexTransitionToPressedHover,
        styleIndexTransitionToInactiveBlur,
        nullptr>();
    {
        PointerMoveEvent event{{}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({1.5f, 2.0f}, event));
        CORRADE_COMPARE(ui.pointerEventPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), node);
        CORRADE_COMPARE(layer.style<StyleIndex>(data), StyleIndex::GreenPressedHover);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }

    /* Setting a null toPressedHover will do nothing for a hovered press */
    shared.setStyleTransition<StyleIndex,
        styleIndexTransitionToPressedBlur,
        nullptr,
        styleIndexTransitionToInactiveBlur,
        styleIndexTransitionToInactiveHover>();
    {
        PointerEvent event{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({2.0f, 2.0f}, event));
        CORRADE_COMPARE(ui.pointerEventPressedNode(), node);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), node);
        CORRADE_COMPARE(layer.style<StyleIndex>(data), StyleIndex::GreenPressedHover);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }

    /* Setting a null combined toPressed will do nothing for a press */
    shared.setStyleTransition<StyleIndex,
        nullptr,
        styleIndexTransitionToInactiveBlur>();
    {
        PointerEvent event{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({2.5f, 2.0f}, event));
        CORRADE_COMPARE(ui.pointerEventPressedNode(), node);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), node);
        CORRADE_COMPARE(layer.style<StyleIndex>(data), StyleIndex::GreenPressedHover);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }

    /* Setting a null combined toInactive will do nothing for a release */
    shared.setStyleTransition<StyleIndex,
        styleIndexTransitionToPressedBlur,
        nullptr>();
    {
        PointerEvent event{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerReleaseEvent({2.0f, 2.0f}, event));
        CORRADE_COMPARE(ui.pointerEventPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), node);
        CORRADE_COMPARE(layer.style<StyleIndex>(data), StyleIndex::GreenPressedHover);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }
}

void AbstractVisualLayerTest::eventStyleTransition() {
    auto&& data = EventStyleTransitionData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    StyleLayerShared shared{12};

    /* StyleLayerShared uses the *_SHARED_SUBCLASS_IMPLEMENTATION() macro, this
       verifies that all the overrides do what's expected */
    StyleLayerShared* chaining;
    if(data.templated) chaining = &shared.setStyleTransition<StyleIndex,
        styleIndexTransitionToPressedBlur,
        styleIndexTransitionToPressedHover,
        styleIndexTransitionToInactiveBlur,
        styleIndexTransitionToInactiveHover>();
    else chaining = &shared.setStyleTransition(
        [](UnsignedInt s) {
            return UnsignedInt(styleIndexTransitionToPressedBlur(StyleIndex(s)));
        },
        [](UnsignedInt s) {
            return UnsignedInt(styleIndexTransitionToPressedHover(StyleIndex(s)));
        },
        [](UnsignedInt s) {
            return UnsignedInt(styleIndexTransitionToInactiveBlur(StyleIndex(s)));
        },
        [](UnsignedInt s) {
            return UnsignedInt(styleIndexTransitionToInactiveHover(StyleIndex(s)));
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

    /* Press on the green node. The node isn't registered as hovered, so it's
       a press without a hover. Which usually happens with taps, for example,
       although it's not restricted to a particular Pointer type. */
    {
        PointerEvent event{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({2.0f, 3.0f}, event));
        CORRADE_COMPARE(ui.pointerEventPressedNode(), nodeGreen);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataGreen), StyleIndex::GreenPressed);
        CORRADE_COMPARE(layer.state(), LayerState::NeedsUpdate);
    }

    /* Presence (or not) of the update call tests two things -- that the
       NeedsUpdate flag is set for each event properly, and that the style is
       changed independently of whether the layer needs update or not */
    if(data.update) {
        ui.update();
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }

    /* Release on the green node. Again, the node isn't registered as hovered,
       so neither the hover stays. */
    {
        PointerEvent event{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerReleaseEvent({2.5f, 2.5f}, event));
        CORRADE_COMPARE(ui.pointerEventPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataGreen), StyleIndex::Green);
        CORRADE_COMPARE(layer.state(), LayerState::NeedsUpdate);
    }

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }

    /* Move on the red node makes it hovered */
    {
        PointerMoveEvent event{{}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({5.0f, 3.0f}, event));
        CORRADE_COMPARE(ui.pointerEventPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), nodeRed);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataRed), StyleIndex::RedHover);
        CORRADE_COMPARE(layer.state(), LayerState::NeedsUpdate);
    }

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }

    /* Tap on it makes it hovered & pressed */
    {
        PointerEvent event{Pointer::Finger};
        CORRADE_VERIFY(ui.pointerPressEvent({4.5f, 3.5f}, event));
        CORRADE_COMPARE(ui.pointerEventPressedNode(), nodeRed);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), nodeRed);
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), nodeRed);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataRed), StyleIndex::RedPressedHover);
        CORRADE_COMPARE(layer.state(), LayerState::NeedsUpdate);
    }

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }

    /* Move away makes it only pressed, without hover, as implicit capture is
       in effect */
    {
        PointerMoveEvent event{{}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({7.0f, 3.0f}, event));
        CORRADE_COMPARE(ui.pointerEventPressedNode(), nodeRed);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), nodeRed);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataRed), StyleIndex::RedPressed);
        CORRADE_COMPARE(layer.state(), LayerState::NeedsUpdate);
    }

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }

    /* Move back makes it hovered & pressed again */
    {
        PointerMoveEvent event{{}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({5.5f, 3.0f}, event));
        CORRADE_COMPARE(ui.pointerEventPressedNode(), nodeRed);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), nodeRed);
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), nodeRed);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataRed), StyleIndex::RedPressedHover);
        CORRADE_COMPARE(layer.state(), LayerState::NeedsUpdate);
    }

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }

    /* Release makes it only hover again */
    {
        PointerEvent event{Pointer::Finger};
        CORRADE_VERIFY(ui.pointerReleaseEvent({5.0f, 2.5f}, event));
        CORRADE_COMPARE(ui.pointerEventPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), nodeRed);
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataRed), StyleIndex::RedHover);
        CORRADE_COMPARE(layer.state(), LayerState::NeedsUpdate);
    }

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }

    /* Move away makes it not hovered anymore */
    {
        PointerMoveEvent event{{}, {}};
        CORRADE_VERIFY(!ui.pointerMoveEvent({7.0f, 2.5f}, event));
        CORRADE_COMPARE(ui.pointerEventPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataRed), StyleIndex::Red);
        CORRADE_COMPARE(layer.state(), LayerState::NeedsUpdate);
    }

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }

    /* Move on and away from the blue is accepted but makes no change to it,
       thus no update is needed */
    {
        PointerMoveEvent event{{}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({2.0f, 6.0f}, event));
        CORRADE_COMPARE(ui.pointerEventPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), nodeBlue);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataBlue), StyleIndex::Blue);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    } {
        PointerMoveEvent event{{}, {}};
        CORRADE_VERIFY(!ui.pointerMoveEvent({2.5f, 8.0f}, event));
        CORRADE_COMPARE(ui.pointerEventPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataBlue), StyleIndex::Blue);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }

    /* Press and release on the white is accepted but makes no change to it,
       thus no update is needed */
    {
        PointerEvent event{Pointer::Pen};
        CORRADE_VERIFY(ui.pointerPressEvent({5.0f, 5.0f}, event));
        CORRADE_COMPARE(ui.pointerEventPressedNode(), nodeWhite);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), nodeWhite);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataWhite), StyleIndex::White);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    } {
        PointerEvent event{Pointer::Pen};
        CORRADE_VERIFY(ui.pointerReleaseEvent({5.5f, 4.5f}, event));
        CORRADE_COMPARE(ui.pointerEventPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), NodeHandle::Null);
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

    StyleLayerShared shared{4};

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
        styleIndexTransitionToPressedBlur,
        styleIndexTransitionToInactiveBlur>();
    else chaining = &shared.setStyleTransition(
        [](UnsignedInt s) {
            return UnsignedInt(styleIndexTransitionToPressedBlur(StyleIndex(s)));
        },
        [](UnsignedInt s) {
            return UnsignedInt(styleIndexTransitionToInactiveBlur(StyleIndex(s)));
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
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), node);
        CORRADE_COMPARE(layer.style<StyleIndex>(layerData), StyleIndex::Green);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }

    /* Press & release with a hover should behave the same as without */
    testPressRelease();

    /* Moving away should do nothing again */
    {
        PointerMoveEvent event{{}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({2.0f, 2.0f}, event));
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), node);
        CORRADE_COMPARE(layer.style<StyleIndex>(layerData), StyleIndex::Green);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }
}

void AbstractVisualLayerTest::eventStyleTransitionNoCapture() {
    auto&& data = EventStyleTransitionNoCaptureData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    StyleLayerShared shared{4};
    shared.setStyleTransition<StyleIndex,
        styleIndexTransitionToPressedBlur,
        styleIndexTransitionToPressedHover,
        styleIndexTransitionToInactiveBlur,
        styleIndexTransitionToInactiveHover>();

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
        CORRADE_COMPARE(ui.pointerEventPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), node);
        CORRADE_COMPARE(layer.style<StyleIndex>(layerData), StyleIndex::GreenHover);

    /* Press will enable the capture, maybe */
    } {
        PointerEvent event{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({2.5f, 2.0f}, event));
        CORRADE_COMPARE(ui.pointerEventPressedNode(), node);
        CORRADE_COMPARE(ui.pointerEventCapturedNode(), data.disableCapture ? NodeHandle::Null : node);
        CORRADE_COMPARE(layer.style<StyleIndex>(layerData), StyleIndex::GreenPressedHover);

    /* Move away will only preserve the press if capture is set */
    } {
        PointerMoveEvent event{{}, {}};
        CORRADE_COMPARE(ui.pointerMoveEvent({7.0f, 2.0f}, event), !data.disableCapture);
        CORRADE_COMPARE(ui.pointerEventPressedNode(), data.disableCapture ? NodeHandle::Null : node);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(layerData), data.blurStyle);

    /* Move back will only preserve the press if capture is set */
    } {
        PointerMoveEvent event{{}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({2.0f, 2.0f}, event));
        CORRADE_COMPARE(ui.pointerEventPressedNode(), data.disableCapture ? NodeHandle::Null : node);
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), node);
        CORRADE_COMPARE(layer.style<StyleIndex>(layerData), data.hoverStyle);
    }
}

StyleIndex styleIndexTransitionOutOfRange(StyleIndex) {
    return StyleIndex(12);
}

void AbstractVisualLayerTest::eventStyleTransitionOutOfRange() {
    CORRADE_SKIP_IF_NO_ASSERT();

    StyleLayerShared shared{12};

    AbstractUserInterface ui{{100, 100}};

    NodeHandle node = ui.createNode({1.0f, 1.0f}, {2.0f, 2.0f});

    StyleLayer& layer = ui.setLayerInstance(Containers::pointer<StyleLayer>(ui.createLayer(), shared));
    layer.create(StyleIndex::Red, node);

    ui.update();
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Tests an OOB condition happening in any of the four functions, and
       checked in any of the four event handlers. Does not exhaustively test
       all possible combinations, as that should not be needed. */

    /* OOB toPressedBlur transition */
    shared.setStyleTransition<StyleIndex,
        styleIndexTransitionOutOfRange,
        styleIndexTransitionToPressedHover,
        styleIndexTransitionToInactiveBlur,
        styleIndexTransitionToInactiveHover>();
    {
        PointerEvent event{Pointer::MouseLeft};

        std::ostringstream out;
        Error redirectError{&out};
        ui.pointerPressEvent({2.0f, 2.0f}, event);
        CORRADE_COMPARE(out.str(), "Whee::AbstractVisualLayer::pointerPressEvent(): style transition from 4 to 12 out of range for 12 styles\n");
    }

    /* OOB toPressedHover transition in the press event. Doing a
       (non-asserting) move before so the hovered node is properly
       registered. */
    shared.setStyleTransition<StyleIndex,
        styleIndexTransitionToPressedBlur,
        styleIndexTransitionOutOfRange,
        styleIndexTransitionToInactiveBlur,
        styleIndexTransitionToInactiveHover>();
    {
        PointerMoveEvent moveEvent{{}, {}};
        ui.pointerMoveEvent({1.5f, 2.0f}, moveEvent);
        PointerEvent event{Pointer::MouseLeft};

        std::ostringstream out;
        Error redirectError{&out};
        ui.pointerPressEvent({2.0f, 2.0f}, event);
        CORRADE_COMPARE(out.str(), "Whee::AbstractVisualLayer::pointerPressEvent(): style transition from 5 to 12 out of range for 12 styles\n");
    }

    /* OOB toInactiveHover transition */
    shared.setStyleTransition<StyleIndex,
        styleIndexTransitionToPressedBlur,
        styleIndexTransitionToPressedHover,
        styleIndexTransitionToInactiveBlur,
        styleIndexTransitionOutOfRange>();
    {
        PointerEvent event{Pointer::MouseLeft};

        std::ostringstream out;
        Error redirectError{&out};
        ui.pointerReleaseEvent({1.5f, 2.5f}, event);
        CORRADE_COMPARE(out.str(), "Whee::AbstractVisualLayer::pointerReleaseEvent(): style transition from 5 to 12 out of range for 12 styles\n");
    }

    /* OOB toInactiveBlur transition in the leave event */
    shared.setStyleTransition<StyleIndex,
        styleIndexTransitionToPressedBlur,
        styleIndexTransitionToPressedHover,
        styleIndexTransitionOutOfRange,
        styleIndexTransitionToInactiveHover>();
    {
        PointerMoveEvent event{{}, {}};

        std::ostringstream out;
        Error redirectError{&out};
        ui.pointerMoveEvent({8.5f, 2.0f}, event);
        CORRADE_COMPARE(out.str(), "Whee::AbstractVisualLayer::pointerLeaveEvent(): style transition from 5 to 12 out of range for 12 styles\n");
    }

    /* OOB toInactiveHover transition in the enter event */
    shared.setStyleTransition<StyleIndex,
        styleIndexTransitionToPressedBlur,
        styleIndexTransitionToPressedHover,
        styleIndexTransitionToInactiveBlur,
        styleIndexTransitionOutOfRange>();
    {
        PointerMoveEvent event{{}, {}};

        std::ostringstream out;
        Error redirectError{&out};
        ui.pointerMoveEvent({1.5f, 2.0f}, event);
        CORRADE_COMPARE(out.str(), "Whee::AbstractVisualLayer::pointerEnterEvent(): style transition from 5 to 12 out of range for 12 styles\n");
    }
}

}}}}

CORRADE_TEST_MAIN(Magnum::Whee::Test::AbstractVisualLayerTest)
