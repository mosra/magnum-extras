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
#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/BitArrayView.h>
#include <Corrade/Containers/Function.h>
#include <Corrade/Containers/GrowableArray.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/Utility/DebugStl.h> /** @todo remove once Debug is stream-free */
#include <Corrade/Utility/FormatStl.h> /** @todo remove once Debug is stream-free */

#include "Magnum/Ui/AbstractUserInterface.h"
#include "Magnum/Ui/AbstractVisualLayer.h"
#include "Magnum/Ui/AbstractVisualLayerAnimator.h"
#include "Magnum/Ui/Event.h"
#include "Magnum/Ui/EventLayer.h"
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/NodeFlags.h"
/* for setStyle(), eventStyleTransition*() */
#include "Magnum/Ui/Implementation/abstractVisualLayerState.h"
/* for StyleLayerStyleAnimator */
#include "Magnum/Ui/Implementation/abstractVisualLayerAnimatorState.h"

namespace Magnum { namespace Ui { namespace Test { namespace {

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
    void setTransitionedStyleInEvent();
    void invalidHandle();
    void styleOutOfRange();

    void dynamicStyleAllocateRecycle();
    void dynamicStyleAllocateNoDynamicStyles();
    void dynamicStyleRecycleInvalid();

    void eventStyleTransitionNoOp();
    void eventStyleTransition();
    void eventStyleTransitionNodeBecomesHiddenDisabledNoEvents();
    void eventStyleTransitionNodeNoLongerFocusable();
    void eventStyleTransitionNoHover();
    void eventStyleTransitionDisabled();
    void eventStyleTransitionNoCapture();
    void eventStyleTransitionOutOfRange();
    void eventStyleTransitionDynamicStyle();

    void sharedNeedsUpdateStatePropagatedToLayers();
};

using namespace Math::Literals;

enum class Enum: UnsignedShort {};

/* The enum is deliberately not 32-bit to verify the APIs can work with smaller
   types too */
enum class StyleIndex: UnsignedByte {
    /* All states for Green, disabled is below */
    Green = 0,
    GreenHover = 1,
    GreenFocused = 2,
    GreenFocusedHover = 3,
    GreenPressed = 4,
    GreenPressedHover = 5,

    /* No focus state for Red, disabled shared with Blue */
    Red = 6,
    RedHover = 7,
    RedPressed = 8,
    RedPressedHover = 9,

    /* No hover state for Blue, disabled shared with Red */
    Blue = 10,
    BlueFocused = 11,
    BluePressed = 12,

    /* No hover or focus state for White */
    White = 13,
    WhiteHover = 14,

    GreenDisabled = 15,
    /* Common for red & blue, to test that there's no inverse mapping done */
    RedBlueDisabled = 16,
};
constexpr UnsignedInt StyleCount = 18;

Debug& operator<<(Debug& debug, StyleIndex value) {
    switch(value) {
        #define _c(value) case StyleIndex::value: return debug << "StyleIndex::" #value;
        _c(Green)
        _c(GreenHover)
        _c(GreenFocused)
        _c(GreenFocusedHover)
        _c(GreenPressed)
        _c(GreenPressedHover)
        _c(Red)
        _c(RedHover)
        _c(RedPressed)
        _c(RedPressedHover)
        _c(Blue)
        _c(BlueFocused)
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
    bool dynamicAnimated;
} EventStyleTransitionNoOpData[]{
    {"", false},
    {"dynamic animated style with target being the same", true},
};

const struct {
    const char* name;
    bool update;
    bool templated;
    bool dynamicAnimated;
} EventStyleTransitionData[]{
    {"update before", true, false, false},
    {"", false, false, false},
    {"templated, update before", true, true, false},
    {"templated", false, true, false},
    {"dynamic animated style with target style being set, update before",
        true, false, true},
    {"dynamic animated style with target style being set",
        false, false, true},
};

const struct {
    const char* name;
    bool update;
    bool templated;
} EventStyleTransitionNoHoverData[]{
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
    bool disableCapture, focusable;
    StyleIndex outStyle, overStyle;
} EventStyleTransitionNoCaptureData[]{
    {"", false, false,
        StyleIndex::GreenPressed, StyleIndex::GreenPressedHover},
    {"capture disabled", true, false,
        StyleIndex::Green, StyleIndex::GreenHover},
    {"focusable", false, true,
        StyleIndex::GreenPressed, StyleIndex::GreenPressedHover},
    {"focusable, capture disabled", false, true,
        StyleIndex::GreenPressed, StyleIndex::GreenPressedHover},
};

const struct {
    const char* name;
    NodeFlags flags;
    bool clearOrder;
    StyleIndex expectedGreenStyle, expectedRedStyle, expectedBlueStyle;
    bool becomesHidden;
} EventStyleTransitionNodeBecomesHiddenDisabledNoEventsData[]{
    {"removed from top level order", {}, true,
        StyleIndex::Green, StyleIndex::Red, StyleIndex::Blue, true},
    {"hidden", NodeFlag::Hidden, false,
        StyleIndex::Green, StyleIndex::Red, StyleIndex::Blue, true},
    {"no events", NodeFlag::NoEvents, false,
        StyleIndex::Green, StyleIndex::Red, StyleIndex::Blue, false},
    {"disabled", NodeFlag::Disabled, false,
        StyleIndex::GreenDisabled, StyleIndex::RedBlueDisabled, StyleIndex::RedBlueDisabled, false}
};

const struct {
    const char* name;
    bool hovered, pressed;
    StyleIndex style, expectedStyleBefore, expectedStyleAfter;
} EventStyleTransitionNodeNoLongerFocusableData[]{
    {"", false, false, StyleIndex::Green,
        StyleIndex::GreenFocused, StyleIndex::Green},
    {"hovered", true, false, StyleIndex::Green,
        StyleIndex::GreenFocusedHover, StyleIndex::GreenHover},
    {"pressed", false, true, StyleIndex::Blue,
        /* Pressed has a priority over Focused, so there's no
           BluePressedFocused */
        StyleIndex::BluePressed, StyleIndex::BluePressed},
    {"hovered + pressed", true, true, StyleIndex::Green,
        /* Pressed has a priority over Focused, so there's no
           GreenPressedFocusedHover */
        StyleIndex::GreenPressedHover, StyleIndex::GreenPressedHover},
};

const struct {
    const char* name;
    UnsignedInt dynamicStyleCount;
    bool dynamicAnimated;
} EventStyleTransitionOutOfRangeData[]{
    {"", 0, false},
    {"dynamic styles", 5, false},
    {"dynamic animated style with target style being set", 1, true}
};

const struct {
    const char* name;
    bool animator1, animator2;
    bool animator1SetDefault;
    bool animation1, animation2;
    bool dynamicStyleAssociatedAnimation;
} EventStyleTransitionDynamicStyleData[]{
    {"",
        false, false, false, false, false, false},
    {"with assigned animator but no animation",
        true, false, true, false, false, false},
    {"with assigned animator but animation not matching its handle",
        true, true, true, false, true, true},
    {"with animation but no assigned animator",
        true, false, false, true, false, true},
    {"with assigned animator, animation matching its handle but not associated with the dynamic style",
        true, false, true, true, false, false},
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
              &AbstractVisualLayerTest::setTransitionedStyleInEvent,
              &AbstractVisualLayerTest::invalidHandle});

    addInstancedTests({&AbstractVisualLayerTest::styleOutOfRange},
        Containers::arraySize(StyleOutOfRangeData));

    addTests({&AbstractVisualLayerTest::dynamicStyleAllocateRecycle,
              &AbstractVisualLayerTest::dynamicStyleAllocateNoDynamicStyles,
              &AbstractVisualLayerTest::dynamicStyleRecycleInvalid});

    addInstancedTests({&AbstractVisualLayerTest::eventStyleTransitionNoOp},
        Containers::arraySize(EventStyleTransitionNoOpData));

    addInstancedTests({&AbstractVisualLayerTest::eventStyleTransition},
        Containers::arraySize(EventStyleTransitionData));

    addInstancedTests({&AbstractVisualLayerTest::eventStyleTransitionNoHover},
        Containers::arraySize(EventStyleTransitionNoHoverData));

    addInstancedTests({&AbstractVisualLayerTest::eventStyleTransitionDisabled},
        Containers::arraySize(EventStyleTransitionDisabledData));

    addInstancedTests({&AbstractVisualLayerTest::eventStyleTransitionNoCapture},
        Containers::arraySize(EventStyleTransitionNoCaptureData));

    addInstancedTests({&AbstractVisualLayerTest::eventStyleTransitionNodeBecomesHiddenDisabledNoEvents},
        Containers::arraySize(EventStyleTransitionNodeBecomesHiddenDisabledNoEventsData));

    addInstancedTests({&AbstractVisualLayerTest::eventStyleTransitionNodeNoLongerFocusable},
        Containers::arraySize(EventStyleTransitionNodeNoLongerFocusableData));

    addInstancedTests({&AbstractVisualLayerTest::eventStyleTransitionOutOfRange},
        Containers::arraySize(EventStyleTransitionOutOfRangeData));

    addInstancedTests({&AbstractVisualLayerTest::eventStyleTransitionDynamicStyle},
        Containers::arraySize(EventStyleTransitionDynamicStyleData));

    addTests({&AbstractVisualLayerTest::sharedNeedsUpdateStatePropagatedToLayers});
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
    MAGNUMEXTRAS_UI_ABSTRACTVISUALLAYER_SHARED_SUBCLASS_IMPLEMENTATION()
};
struct StyleLayer: AbstractVisualLayer {
    explicit StyleLayer(LayerHandle handle, Shared& shared): AbstractVisualLayer{handle, shared} {}

    using AbstractVisualLayer::assignAnimator;
    using AbstractVisualLayer::setDefaultStyleAnimator;

    LayerFeatures doFeatures() const override {
        return AbstractVisualLayer::doFeatures()|LayerFeature::AnimateStyles;
    }
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

/* This one is shared by all cases that set up style transition animations */
struct StyleLayerStyleAnimator: AbstractVisualLayerStyleAnimator {
    explicit StyleLayerStyleAnimator(AnimatorHandle handle): AbstractVisualLayerStyleAnimator{handle} {}

    /* Just saves the target style index and sync's the style arrays */
    template<class T> AnimationHandle create(T targetStyle, Nanoseconds played, Nanoseconds duration, DataHandle data, AnimationFlags flags = {}) {
        AnimationHandle handle = AbstractVisualLayerStyleAnimator::create(played, duration, data, flags);
        const UnsignedInt id = animationHandleId(handle);
        if(id >= styles.size()) {
            arrayAppend(styles, NoInit, id - styles.size() + 1);
            _state->targetStyles = stridedArrayView(styles).slice(&decltype(styles)::Type::first);
            _state->dynamicStyles = stridedArrayView(styles).slice(&decltype(styles)::Type::second);
        }
        styles[id].first() = UnsignedInt(targetStyle);
        styles[id].second() = ~UnsignedInt{};
        return handle;
    }

    Containers::Array<Containers::Pair<UnsignedInt, UnsignedInt>> styles;
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

    DataHandle layerData = layer.create(StyleCount + 0);
    CORRADE_COMPARE(layer.style(layerData), StyleCount + 0);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Setting a style marks the layer as dirty */
    layer.setStyle(layerData, T(37));
    CORRADE_COMPARE(layer.style(layerData), 37);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Testing also the other overload */
    layer.setStyle(dataHandleData(layerData), T(66));
    CORRADE_COMPARE(layer.style(layerData), 66);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);
}

void AbstractVisualLayerTest::setTransitionedStyle() {
    AbstractUserInterface ui{{100, 100}};

    enum Style {
        /* 2 is first, to avoid accidentally matching the order */
        InactiveOut2,
        InactiveOut1,
        InactiveOver2,
        InactiveOver1,
        FocusedOut2,
        FocusedOut1,
        FocusedOver2,
        FocusedOver1,
        PressedOut2,
        PressedOut1,
        PressedOver2,
        PressedOver1,
    };

    /* Style transition isn't allowed to use dynamic styles so the dynamic
       count shouldn't affect it */
    StyleLayerShared shared{12, 0};
    shared.setStyleTransition(
        [](UnsignedInt style) -> UnsignedInt {
            switch(Style(style)) {
                case InactiveOut1:
                case InactiveOver1:
                case FocusedOut1:
                case FocusedOver1:
                case PressedOut1:
                case PressedOver1:
                    return InactiveOut1;
                case InactiveOut2:
                case InactiveOver2:
                case FocusedOut2:
                case FocusedOver2:
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
                case FocusedOut1:
                case FocusedOver1:
                case PressedOut1:
                case PressedOver1:
                    return InactiveOver1;
                case InactiveOut2:
                case InactiveOver2:
                case FocusedOut2:
                case FocusedOver2:
                case PressedOut2:
                case PressedOver2:
                    return InactiveOver2;
            }
            CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        },
        [](UnsignedInt style) -> UnsignedInt {
            switch(Style(style)) {
                case InactiveOut1:
                case InactiveOver1:
                case FocusedOut1:
                case FocusedOver1:
                case PressedOut1:
                case PressedOver1:
                    return FocusedOut1;
                case InactiveOut2:
                case InactiveOver2:
                case FocusedOut2:
                case FocusedOver2:
                case PressedOut2:
                case PressedOver2:
                    return FocusedOut2;
            }
            CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        },
        [](UnsignedInt style) -> UnsignedInt {
            switch(Style(style)) {
                case InactiveOut1:
                case InactiveOver1:
                case FocusedOut1:
                case FocusedOver1:
                case PressedOut1:
                case PressedOver1:
                    return FocusedOver1;
                case InactiveOut2:
                case InactiveOver2:
                case FocusedOut2:
                case FocusedOver2:
                case PressedOut2:
                case PressedOver2:
                    return FocusedOver2;
            }
            CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        },
        [](UnsignedInt style) -> UnsignedInt {
            switch(Style(style)) {
                case InactiveOut1:
                case InactiveOver1:
                case FocusedOut1:
                case FocusedOver1:
                case PressedOut1:
                case PressedOver1:
                    return PressedOut1;
                case InactiveOut2:
                case InactiveOver2:
                case FocusedOut2:
                case FocusedOver2:
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
                case FocusedOut1:
                case FocusedOver1:
                case PressedOut1:
                case PressedOver1:
                    return PressedOver1;
                case InactiveOut2:
                case InactiveOver2:
                case FocusedOut2:
                case FocusedOver2:
                case PressedOut2:
                case PressedOver2:
                    return PressedOver2;
            }
            CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        },
        [](UnsignedInt) -> UnsignedInt {
            CORRADE_FAIL("This shouldn't be called");
            CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        });
    StyleLayer& layer = ui.setLayerInstance(Containers::pointer<StyleLayer>(ui.createLayer(), shared));

    /* Node 2 is first, to avoid accidentally matching the order, Neither of
       the two are Focusable initially, to test pointerPressEvent() without the
       implicit focus. */
    NodeHandle node2 = ui.createNode({}, {100, 50});
    NodeHandle node1 = ui.createNode({0, 50}, {100, 50});
    DataHandle data1 = layer.create(InactiveOut1, node1);
    DataHandle data2 = layer.create(InactiveOut2, node2);

    /* Nothing is hovered, pressed or focused initially */
    CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
    CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
    CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);

    /* Setting a transitioned style picks InactiveOut. Switching the IDs to be
       sure it actually changed. */
    layer.setTransitionedStyle(ui, data1, PressedOut2);
    layer.setTransitionedStyle(ui, data2, InactiveOver1);
    CORRADE_COMPARE(layer.style(data1), InactiveOut2);
    CORRADE_COMPARE(layer.style(data2), InactiveOut1);

    /* Hovering node 2 causes the style to be changed to InactiveOver */
    {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        CORRADE_VERIFY(ui.pointerMoveEvent({50, 25}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), node2);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
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
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        CORRADE_VERIFY(ui.pointerPressEvent({50, 25}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), node2);
        CORRADE_COMPARE(ui.currentHoveredNode(), node2);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
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
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        CORRADE_VERIFY(ui.pointerMoveEvent({50, 75}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), node2);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
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
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        CORRADE_VERIFY(ui.pointerReleaseEvent({50, 75}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style(data2), InactiveOut2);
    }

    /* Setting a transitioned style (switching IDs again) picks InactiveOut
       for both */
    layer.setTransitionedStyle(ui, data1, PressedOut2);
    layer.setTransitionedStyle(ui, data2, InactiveOver1);
    CORRADE_COMPARE(layer.style(data1), InactiveOut2);
    CORRADE_COMPARE(layer.style(data2), InactiveOut1);

    /* Make node2 focusable for the rest of the test case */
    ui.addNodeFlags(node2, NodeFlag::Focusable);

    /* Focusing causes the style to be changed to FocusedOut */
    {
        FocusEvent event{{}};
        CORRADE_VERIFY(ui.focusEvent(node2, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), node2);
        CORRADE_COMPARE(layer.style(data2), FocusedOut1);
    }

    /* Setting a transitioned style (switching IDs again) picks FocusedOut
       for the focused node, the other stays InactiveOut */
    layer.setTransitionedStyle(ui, data1, FocusedOver1);
    layer.setTransitionedStyle(ui, data2, InactiveOut2);
    CORRADE_COMPARE(layer.style(data1), InactiveOut1);
    CORRADE_COMPARE(layer.style(data2), FocusedOut2);

    /* Pressing on node 2 causes the style to be changed to PressedOut, as it
       has a priority over focus */
    {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        CORRADE_VERIFY(ui.pointerPressEvent({50, 25}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), node2);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), node2);
        CORRADE_COMPARE(layer.style(data2), PressedOut2);
    }

    /* Setting a transitioned style (switching IDs again) should pick
       PressedOut for the focused node as well, the other stays InactiveOut */
    layer.setTransitionedStyle(ui, data1, FocusedOut2);
    layer.setTransitionedStyle(ui, data2, InactiveOver1);
    CORRADE_COMPARE(layer.style(data1), InactiveOut2);
    CORRADE_COMPARE(layer.style(data2), PressedOut1);

    /* Hovering on node 2 while being pressed & focused makes PressedOver win
       again over FocusedOver */
    {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        CORRADE_VERIFY(ui.pointerMoveEvent({50, 25}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), node2);
        CORRADE_COMPARE(ui.currentHoveredNode(), node2);
        CORRADE_COMPARE(ui.currentFocusedNode(), node2);
        CORRADE_COMPARE(layer.style(data2), PressedOver1);
    }

    /* Setting a transitioned style (switching IDs again) should pick
       PressedOver again, the other stays InactiveOut */
    layer.setTransitionedStyle(ui, data1, FocusedOver1);
    layer.setTransitionedStyle(ui, data2, InactiveOut2);
    CORRADE_COMPARE(layer.style(data1), InactiveOut1);
    CORRADE_COMPARE(layer.style(data2), PressedOver2);

    /* Releasing causes the style to be changed to FocusedOver */
    {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        CORRADE_VERIFY(ui.pointerReleaseEvent({50, 25}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), node2);
        CORRADE_COMPARE(ui.currentFocusedNode(), node2);
        CORRADE_COMPARE(layer.style(data2), FocusedOver2);
    }

    /* Setting a transitioned style (switching IDs again) picks FocusedOver for
       the focused node, the other stays InactiveOut */
    layer.setTransitionedStyle(ui, data1, FocusedOver2);
    layer.setTransitionedStyle(ui, data2, PressedOut1);
    CORRADE_COMPARE(layer.style(data1), InactiveOut2);
    CORRADE_COMPARE(layer.style(data2), FocusedOver1);
}

void AbstractVisualLayerTest::setTransitionedStyleInEvent() {
    /* Compared to setTransitionedStyle() verifies that calling the function in
       an event handler works as well, i.e. that the final style corresponds to
       the actual state.

       In reality, for press and release, the setTransitionedStyle() call will
       not have an up-to-date information about what's the currently hovered /
       pressed / focused yet, so the style will not be correct at that point,
       but it will be immediately followed by another transition that then
       makes the final result correct. For enter and leave it will do the
       correct thing already as those events are called only once the info about
       the current hovered node is updated.

       See comments in each case below for more details. */

    AbstractUserInterface ui{{100, 100}};

    enum Style {
        InactiveOut,
        InactiveOver,
        FocusedOut,
        FocusedOver,
        PressedOut,
        PressedOver,
    };

    /* Style transition isn't allowed to use dynamic styles so the dynamic
       count shouldn't affect it */
    StyleLayerShared shared{12, 0};
    shared.setStyleTransition(
        [](UnsignedInt style) -> UnsignedInt {
            switch(Style(style)) {
                case InactiveOut:
                case InactiveOver:
                case FocusedOut:
                case FocusedOver:
                case PressedOut:
                case PressedOver:
                    return InactiveOut;
            }
            CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        },
        [](UnsignedInt style) -> UnsignedInt {
            switch(Style(style)) {
                case InactiveOut:
                case InactiveOver:
                case FocusedOut:
                case FocusedOver:
                case PressedOut:
                case PressedOver:
                    return InactiveOver;
            }
            CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        },
        [](UnsignedInt style) -> UnsignedInt {
            switch(Style(style)) {
                case InactiveOut:
                case InactiveOver:
                case FocusedOut:
                case FocusedOver:
                case PressedOut:
                case PressedOver:
                    return FocusedOut;
            }
            CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        },
        [](UnsignedInt style) -> UnsignedInt {
            switch(Style(style)) {
                case InactiveOut:
                case InactiveOver:
                case FocusedOut:
                case FocusedOver:
                case PressedOut:
                case PressedOver:
                    return FocusedOver;
            }
            CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        },
        [](UnsignedInt style) -> UnsignedInt {
            switch(Style(style)) {
                case InactiveOut:
                case InactiveOver:
                case FocusedOut:
                case FocusedOver:
                case PressedOut:
                case PressedOver:
                    return PressedOut;
            }
            CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        },
        [](UnsignedInt style) -> UnsignedInt {
            switch(Style(style)) {
                case InactiveOut:
                case InactiveOver:
                case FocusedOut:
                case FocusedOver:
                case PressedOut:
                case PressedOver:
                    return PressedOver;
            }
            CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        },
        [](UnsignedInt) -> UnsignedInt {
            CORRADE_FAIL("This shouldn't be called");
            CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        });
    StyleLayer& layer = ui.setLayerInstance(Containers::pointer<StyleLayer>(ui.createLayer(), shared));

    EventLayer& eventLayer = ui.setLayerInstance(Containers::pointer<EventLayer>(ui.createLayer()));

    NodeHandle node = ui.createNode({}, {100, 50});
    DataHandle data = layer.create(InactiveOut, node);

    /* Nothing is hovered, pressed or focused initially */
    CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
    CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
    CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);

    /* Setting a transitioned style inside onEnter should pick InactiveOver */
    {
        Int called = 0;
        EventConnection connection = eventLayer.onEnterScoped(node, [&]{
            layer.setTransitionedStyle(ui, data, FocusedOut);
            /* At this point, currentHoveredNode() is already updated and thus
               the style is already transitioned to the final one. This is
               because pointerEnterEvent() is called after pointerMoveEvent()
               that updated the hovered node information. */
            CORRADE_COMPARE(ui.currentHoveredNode(), node);
            CORRADE_COMPARE(layer.style(data), InactiveOver);
            ++called;
        });

        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        CORRADE_VERIFY(ui.pointerMoveEvent({50, 25}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), node);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE(called, 1);
        CORRADE_COMPARE(layer.style(data), InactiveOver);

    /* Setting a transitioned style inside onLeave should pick InactiveOut */
    } {
        Int called = 0;
        EventConnection connection = eventLayer.onLeaveScoped(node, [&]{
            layer.setTransitionedStyle(ui, data, FocusedOver);
            /* At this point, currentHoveredNode() is again already updated and
               thus the style is already transitioned to the final one. This is
               because pointerLeaveEvent() is called after pointerMoveEvent()
               that updated the hovered node information. */
            CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
            CORRADE_COMPARE(layer.style(data), InactiveOut);
            ++called;
        });

        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        CORRADE_VERIFY(!ui.pointerMoveEvent({1000, 1000}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE(called, 1);
        CORRADE_COMPARE(layer.style(data), InactiveOut);

    /* Setting a transitioned style inside onPress should pick PressedOut */
    } {
        Int called = 0;
        EventConnection connection = eventLayer.onPressScoped(node, [&]{
            layer.setTransitionedStyle(ui, data, FocusedOver);
            /* At this point, currentPressedNode() is not updated yet because
               we don't yet know if the press events will actually be accepted.
               Which means the transition doesn't take the press into account,
               and what makes the style correct is a transition that only
               happens after, once the currentPressedNode() is updated. */
            CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
            CORRADE_COMPARE(layer.style(data), InactiveOut);
            ++called;
        });

        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        CORRADE_VERIFY(ui.pointerPressEvent({50, 25}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), node);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE(called, 1);
        CORRADE_COMPARE(layer.style(data), PressedOut);

    /* Setting a transitioned style inside onRelease should pick InactiveOut */
    } {
        Int called = 0;
        EventConnection connection = eventLayer.onReleaseScoped(node, [&]{
            layer.setTransitionedStyle(ui, data, FocusedOver);
            /* At this point, currentPressedNode() is not updated yet because
               certain other functionality such as generation of tap/click
               events relies on the knowledge of whether given node is pressed.
               Consistently with a press it's updated only after all release
               events are fired. What makes the style correct is a transition
               that only happens after, once the currentPressedNode() is
               updated. */
            CORRADE_COMPARE(ui.currentPressedNode(), node);
            CORRADE_COMPARE(layer.style(data), PressedOut);
            ++called;
        });

        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        CORRADE_VERIFY(ui.pointerReleaseEvent({50, 25}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE(called, 1);
        CORRADE_COMPARE(layer.style(data), InactiveOut);

    /* Setting a transitioned style inside onTapOrClick should pick
       InactiveOut */
    } {
        Int called = 0;
        EventConnection connection = eventLayer.onTapOrClickScoped(node, [&]{
            layer.setTransitionedStyle(ui, data, PressedOver);
            /* As this is fired from a release event, currentPressedNode() is
               not updated yet same as with onRelease() above, and it's done
               only after all release events are fired. */
            CORRADE_COMPARE(ui.currentPressedNode(), node);
            CORRADE_COMPARE(layer.style(data), PressedOut);
            ++called;
        });

        PointerEvent pressEvent{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        PointerEvent releaseEvent{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        CORRADE_VERIFY(ui.pointerPressEvent({50, 25}, pressEvent));
        CORRADE_VERIFY(ui.pointerReleaseEvent({50, 25}, releaseEvent));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE(called, 1);
        CORRADE_COMPARE(layer.style(data), InactiveOut);
    }

    /* Make the node focusable for the rest of the test */
    ui.setNodeFlags(node, NodeFlag::Focusable);

    /* Setting a transitioned style inside onFocus should pick FocusedOut */
    {
        Int called = 0;
        EventConnection connection = eventLayer.onFocusScoped(node, [&]{
            layer.setTransitionedStyle(ui, data, PressedOver);
            /* Similarly as with the press event, currentFocusedNode() isn't
               updated at this point yet. Again a second transition happens
               after, making the resulting style correct. */
            CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
            CORRADE_COMPARE(layer.style(data), InactiveOut);
            ++called;
        });

        FocusEvent event{{}};
        CORRADE_VERIFY(ui.focusEvent(node, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), node);
        CORRADE_COMPARE(called, 1);
        CORRADE_COMPARE(layer.style(data), FocusedOut);

    /* Setting a transitioned style inside onBlur should pick InactiveOut */
    } {
        Int called = 0;
        EventConnection connection = eventLayer.onBlurScoped(node, [&]{
            layer.setTransitionedStyle(ui, data, PressedOver);
            /* Similarly as with the release event, currentFocusedNode() isn't
               updated at this point yet. Again it *could* be, nevertheless a
               second transition happens after, making the resulting style
               correct. */
            CORRADE_COMPARE(ui.currentFocusedNode(), node);
            CORRADE_COMPARE(layer.style(data), FocusedOut);
            ++called;
        });

        FocusEvent event{{}};
        CORRADE_VERIFY(!ui.focusEvent(NodeHandle::Null, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE(called, 1);
        CORRADE_COMPARE(layer.style(data), InactiveOut);
    }
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
        "Ui::AbstractVisualLayer::style(): invalid handle Ui::DataHandle::Null\n"
        "Ui::AbstractVisualLayer::style(): invalid handle Ui::LayerDataHandle::Null\n"
        "Ui::AbstractVisualLayer::setStyle(): invalid handle Ui::DataHandle::Null\n"
        "Ui::AbstractVisualLayer::setStyle(): invalid handle Ui::LayerDataHandle::Null\n"
        "Ui::AbstractVisualLayer::setTransitionedStyle(): invalid handle Ui::DataHandle::Null\n"
        "Ui::AbstractVisualLayer::setTransitionedStyle(): invalid handle Ui::LayerDataHandle::Null\n");
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
        "Ui::AbstractVisualLayer::setStyle(): style 3 out of range for 3 styles\n"
        "Ui::AbstractVisualLayer::setStyle(): style 3 out of range for 3 styles\n"
        "Ui::AbstractVisualLayer::setTransitionedStyle(): style {0} out of range for {0} styles\n"
        "Ui::AbstractVisualLayer::setTransitionedStyle(): style {0} out of range for {0} styles\n", data.styleCount));
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
    CORRADE_COMPARE(layer.dynamicStyleAnimation(0), AnimationHandle::Null);

    /* Remember an associated animation handle */
    Containers::Optional<UnsignedInt> second = layer.allocateDynamicStyle(AnimationHandle(0x12ab34567cde));
    CORRADE_COMPARE(second, 1);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 2);
    CORRADE_COMPARE(layer.dynamicStyleAnimation(1), AnimationHandle(0x12ab34567cde));

    Containers::Optional<UnsignedInt> third = layer.allocateDynamicStyle();
    CORRADE_COMPARE(third, 2);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 3);
    CORRADE_COMPARE(layer.dynamicStyleAnimation(2), AnimationHandle::Null);

    Containers::Optional<UnsignedInt> fourth = layer.allocateDynamicStyle();
    CORRADE_COMPARE(fourth, 3);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 4);
    CORRADE_COMPARE(layer.dynamicStyleAnimation(3), AnimationHandle::Null);

    /* Recycle a subset in random order */
    layer.recycleDynamicStyle(*third);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 3);
    CORRADE_COMPARE(layer.dynamicStyleAnimation(*third), AnimationHandle::Null);

    /* The animation handle is cleared on recycle */
    layer.recycleDynamicStyle(*second);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 2);
    CORRADE_COMPARE(layer.dynamicStyleAnimation(*second), AnimationHandle::Null);

    layer.recycleDynamicStyle(*fourth);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 1);
    CORRADE_COMPARE(layer.dynamicStyleAnimation(*fourth), AnimationHandle::Null);

    /* Allocating new ones simply picks up the first free */
    Containers::Optional<UnsignedInt> second2 = layer.allocateDynamicStyle();
    Containers::Optional<UnsignedInt> third2 = layer.allocateDynamicStyle();
    Containers::Optional<UnsignedInt> fourth2 = layer.allocateDynamicStyle();
    CORRADE_COMPARE(second2, 1);
    CORRADE_COMPARE(third2, 2);
    CORRADE_COMPARE(fourth2, 3);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 4);
    /* The animation handle doesn't show up when the slot is reused either */
    CORRADE_COMPARE(layer.dynamicStyleAnimation(*second), AnimationHandle::Null);

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
    layer.dynamicStyleAnimation(4);
    layer.recycleDynamicStyle(4);
    CORRADE_COMPARE(out.str(),
        "Ui::AbstractVisualLayer::recycleDynamicStyle(): style 2 not allocated\n"
        "Ui::AbstractVisualLayer::dynamicStyleAnimation(): index 4 out of range for 4 dynamic styles\n"
        "Ui::AbstractVisualLayer::recycleDynamicStyle(): index 4 out of range for 4 dynamic styles\n");
}

StyleIndex styleIndexTransitionToInactiveOut(StyleIndex index) {
    switch(index) {
        case StyleIndex::Green:
        case StyleIndex::GreenHover:
        case StyleIndex::GreenFocused:
        case StyleIndex::GreenFocusedHover:
        case StyleIndex::GreenPressed:
        case StyleIndex::GreenPressedHover:
            return StyleIndex::Green;
        case StyleIndex::Red:
        case StyleIndex::RedHover:
        case StyleIndex::RedPressed:
        case StyleIndex::RedPressedHover:
            return StyleIndex::Red;
        case StyleIndex::Blue:
        case StyleIndex::BlueFocused:
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
        case StyleIndex::GreenFocused:
        case StyleIndex::GreenFocusedHover:
        case StyleIndex::GreenPressed:
        case StyleIndex::GreenPressedHover:
            return StyleIndex::GreenHover;
        case StyleIndex::Red:
        case StyleIndex::RedHover:
        case StyleIndex::RedPressed:
        case StyleIndex::RedPressedHover:
            return StyleIndex::RedHover;
        case StyleIndex::Blue:
        case StyleIndex::BlueFocused:
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

StyleIndex styleIndexTransitionToFocusedOut(StyleIndex index) {
    switch(index) {
        case StyleIndex::Green:
        case StyleIndex::GreenHover:
        case StyleIndex::GreenFocused:
        case StyleIndex::GreenFocusedHover:
        case StyleIndex::GreenPressed:
        case StyleIndex::GreenPressedHover:
            return StyleIndex::GreenFocused;
        case StyleIndex::Red:
        case StyleIndex::RedHover:
        case StyleIndex::RedPressed:
        case StyleIndex::RedPressedHover:
            return StyleIndex::Red; /* no focus state */
        case StyleIndex::Blue:
        case StyleIndex::BlueFocused:
        case StyleIndex::BluePressed:
            return StyleIndex::BlueFocused;
        case StyleIndex::White:
        case StyleIndex::WhiteHover:
            return StyleIndex::White; /* no focus state */
        case StyleIndex::GreenDisabled:
        case StyleIndex::RedBlueDisabled:
            CORRADE_FAIL("Called with" << index);
    }
    CORRADE_FAIL("Called with" << UnsignedInt(index));
    CORRADE_INTERNAL_ASSERT_UNREACHABLE();
}

StyleIndex styleIndexTransitionToFocusedOver(StyleIndex index) {
    switch(index) {
        case StyleIndex::Green:
        case StyleIndex::GreenHover:
        case StyleIndex::GreenFocused:
        case StyleIndex::GreenFocusedHover:
        case StyleIndex::GreenPressed:
        case StyleIndex::GreenPressedHover:
            return StyleIndex::GreenFocusedHover;
        case StyleIndex::Red:
        case StyleIndex::RedHover:
        case StyleIndex::RedPressed:
        case StyleIndex::RedPressedHover:
            return StyleIndex::RedHover; /* no focus state */
        case StyleIndex::Blue:
        case StyleIndex::BlueFocused:
        case StyleIndex::BluePressed:
            return StyleIndex::Blue;
        case StyleIndex::White:
        case StyleIndex::WhiteHover:
            return StyleIndex::WhiteHover; /* no focus state */
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
        case StyleIndex::GreenFocused:
        case StyleIndex::GreenFocusedHover:
        case StyleIndex::GreenPressed:
        case StyleIndex::GreenPressedHover:
            return StyleIndex::GreenPressed;
        case StyleIndex::Red:
        case StyleIndex::RedHover:
        case StyleIndex::RedPressed:
        case StyleIndex::RedPressedHover:
            return StyleIndex::RedPressed;
        case StyleIndex::Blue:
        case StyleIndex::BlueFocused:
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
        case StyleIndex::GreenFocused:
        case StyleIndex::GreenFocusedHover:
        case StyleIndex::GreenPressed:
        case StyleIndex::GreenPressedHover:
            return StyleIndex::GreenPressedHover;
        case StyleIndex::Red:
        case StyleIndex::RedHover:
        case StyleIndex::RedPressed:
        case StyleIndex::RedPressedHover:
            return StyleIndex::RedPressedHover;
        case StyleIndex::Blue:
        case StyleIndex::BlueFocused:
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
        case StyleIndex::GreenFocused:
        case StyleIndex::GreenFocusedHover:
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
        case StyleIndex::BlueFocused:
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

StyleIndex styleIndexTransitionPassthrough(StyleIndex index) {
    return index;
}

void AbstractVisualLayerTest::eventStyleTransitionNoOp() {
    auto&& data = EventStyleTransitionNoOpData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* (Non-no-op) transition for dynamic styles tested in
       eventStyleTransitionDynamicStyle() */
    StyleLayerShared shared{StyleCount, data.dynamicAnimated ? 1u : 0u};

    AbstractUserInterface ui{{100, 100}};

    NodeHandle node = ui.createNode({1.0f, 1.0f}, {2.0f, 2.0f});
    StyleLayer& layer = ui.setLayerInstance(Containers::pointer<StyleLayer>(ui.createLayer(), shared));

    /* In case of the animated dynamic style, create the data with a dynamic
       style that points to an animation that a target style index. Not using
       ui.advanceAnimations() as that would require a lot more code in both
       StyleLayer and StyleLayerStyleAnimator (which itself would need to be
       tested), allocating the dynamic style with associated animation
       directly. */
    DataHandle nodeData;
    if(data.dynamicAnimated) {
        Containers::Pointer<StyleLayerStyleAnimator> animator{InPlaceInit, ui.createAnimator()};
        layer.assignAnimator(*animator);
        layer.setDefaultStyleAnimator(animator.get());

        AnimationHandle nodeDataAnimation = animator->create(StyleIndex::GreenPressedHover, 0_nsec, 1_nsec, DataHandle::Null);
        nodeData = layer.create(StyleCount + *layer.allocateDynamicStyle(nodeDataAnimation), node);
        CORRADE_COMPARE(layer.style(nodeData), StyleCount + 0);
        CORRADE_COMPARE(animator->targetStyle<StyleIndex>(nodeDataAnimation), StyleIndex::GreenPressedHover);
        /* The dynamic style isn't backreferenced from the animation, but
           that's fine, the layer needs only the other direction */
        CORRADE_COMPARE(animator->dynamicStyle(nodeDataAnimation), Containers::NullOpt);

        ui.setStyleAnimatorInstance(Utility::move(animator));
    } else nodeData = layer.create(StyleIndex::GreenPressedHover, node);

    ui.update();
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* In case of a dynamic animated style, the no-op transition happens on the
       targetStyle() and not on the dynamic style. But since it's no-op, the
       new style index is the same as targetStyle(), which the animation is
       eventually going to land on, so nothing is done either and the dynamic
       style stays assigned.

       Yes, this would behave the same even if given event handler didn't peek
       into animations associated with dynamic styles at all, but presence of
       that code is what the other test cases are verifying. */
    const StyleIndex expectedStyle = data.dynamicAnimated ? StyleIndex(StyleCount + 0) : StyleIndex::GreenPressedHover;

    /* Press, release, over, hovered press, hovered release, out should all do
       nothing by default */
    {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        CORRADE_VERIFY(ui.pointerPressEvent({2.0f, 2.0f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), node);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(nodeData), expectedStyle);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    } {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        CORRADE_VERIFY(ui.pointerReleaseEvent({2.5f, 2.5f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(nodeData), expectedStyle);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        CORRADE_VERIFY(ui.pointerMoveEvent({2.0f, 2.0f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), node);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(nodeData), expectedStyle);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    } {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        CORRADE_VERIFY(ui.pointerPressEvent({2.0f, 2.0f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), node);
        CORRADE_COMPARE(ui.currentHoveredNode(), node);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(nodeData), expectedStyle);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    } {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        CORRADE_VERIFY(ui.pointerReleaseEvent({2.5f, 2.5f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), node);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(nodeData), expectedStyle);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        CORRADE_VERIFY(!ui.pointerMoveEvent({5.0f, 2.0f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(nodeData), expectedStyle);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }

    /* Focus and blur with a temporarily focusable node should do nothing by
       default */
    {
        ui.addNodeFlags(node, NodeFlag::Focusable);

        FocusEvent focusEvent{{}};
        CORRADE_VERIFY(ui.focusEvent(node, focusEvent));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), node);
        CORRADE_COMPARE(layer.style<StyleIndex>(nodeData), expectedStyle);
        CORRADE_COMPARE(layer.state(), LayerStates{});

        FocusEvent blurEvent{{}};
        CORRADE_VERIFY(!ui.focusEvent(NodeHandle::Null, blurEvent));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(nodeData), expectedStyle);
        CORRADE_COMPARE(layer.state(), LayerStates{});

        ui.clearNodeFlags(node, NodeFlag::Focusable);
    }

    /* Making a hovered focused node non-focusable should do nothing by
       default */
    {
        ui.addNodeFlags(node, NodeFlag::Focusable);

        PointerMoveEvent moveEvent{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        FocusEvent focusEvent{{}};
        CORRADE_VERIFY(ui.pointerMoveEvent({2.0f, 2.0f}, moveEvent));
        CORRADE_VERIFY(ui.focusEvent(node, focusEvent));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), node);
        CORRADE_COMPARE(ui.currentFocusedNode(), node);
        CORRADE_COMPARE(layer.style<StyleIndex>(nodeData), expectedStyle);
        CORRADE_COMPARE(layer.state(), LayerStates{});

        ui.clearNodeFlags(node, NodeFlag::Focusable);
        ui.update();
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), node);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(nodeData), expectedStyle);
        CORRADE_COMPARE(layer.state(), LayerStates{});

        ui.addNodeFlags(node, NodeFlag::Disabled);
        ui.update();
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(nodeData), expectedStyle);
        CORRADE_COMPARE(layer.state(), LayerStates{});

        ui.clearNodeFlags(node, NodeFlag::Disabled);
    }

    /* Setting a null toPressedOut transition will do nothing for a press */
    shared.setStyleTransition<StyleIndex,
        styleIndexTransitionToInactiveOut,
        styleIndexTransitionToInactiveOver,
        styleIndexTransitionToFocusedOut,
        styleIndexTransitionToFocusedOver,
        nullptr,
        styleIndexTransitionToPressedOver,
        styleIndexTransitionToDisabledDoNotCall>();
    {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        CORRADE_VERIFY(ui.pointerPressEvent({2.0f, 2.0f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), node);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(nodeData), expectedStyle);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }

    /* Setting a null toInactiveOut transition will do nothing for a release */
    shared.setStyleTransition<StyleIndex,
        nullptr,
        styleIndexTransitionToInactiveOver,
        styleIndexTransitionToFocusedOut,
        styleIndexTransitionToFocusedOver,
        styleIndexTransitionToPressedOut,
        styleIndexTransitionToPressedOver,
        styleIndexTransitionToDisabledDoNotCall>();
    {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        CORRADE_VERIFY(ui.pointerReleaseEvent({2.5f, 2.5f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(nodeData), expectedStyle);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }

    /* Setting a null toInactiveOver will do nothing for a hover */
    shared.setStyleTransition<StyleIndex,
        styleIndexTransitionToInactiveOut,
        nullptr,
        styleIndexTransitionToFocusedOut,
        styleIndexTransitionToFocusedOver,
        styleIndexTransitionToPressedOut,
        styleIndexTransitionToPressedOver,
        styleIndexTransitionToDisabledDoNotCall>();
    {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        CORRADE_VERIFY(ui.pointerMoveEvent({1.5f, 2.0f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), node);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(nodeData), expectedStyle);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }

    /* Setting a null toPressedOver will do nothing for a hovered press */
    shared.setStyleTransition<StyleIndex,
        styleIndexTransitionToInactiveOut,
        styleIndexTransitionToInactiveOver,
        styleIndexTransitionToFocusedOut,
        styleIndexTransitionToFocusedOver,
        styleIndexTransitionToPressedOut,
        nullptr,
        styleIndexTransitionToDisabledDoNotCall>();
    {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        CORRADE_VERIFY(ui.pointerPressEvent({2.0f, 2.0f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), node);
        CORRADE_COMPARE(ui.currentHoveredNode(), node);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(nodeData), expectedStyle);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }

    /* Setting a null combined toPressed will do nothing for a press */
    shared.setStyleTransition<StyleIndex,
        styleIndexTransitionToInactiveOut,
        styleIndexTransitionToFocusedOut,
        nullptr,
        styleIndexTransitionToDisabledDoNotCall>();
    {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        CORRADE_VERIFY(ui.pointerPressEvent({2.5f, 2.0f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), node);
        CORRADE_COMPARE(ui.currentHoveredNode(), node);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(nodeData), expectedStyle);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }

    /* Setting a null combined toInactive will do nothing for a release */
    shared.setStyleTransition<StyleIndex,
        nullptr,
        styleIndexTransitionToFocusedOut,
        styleIndexTransitionToPressedOut,
        styleIndexTransitionToDisabledDoNotCall>();
    {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        CORRADE_VERIFY(ui.pointerReleaseEvent({2.0f, 2.0f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), node);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(nodeData), expectedStyle);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }

    /* Marking the node as Focusable for the rest of the test case */
    ui.addNodeFlags(node, NodeFlag::Focusable);

    /* Setting a null toFocusedOver will do nothing for a hovered focus */
    shared.setStyleTransition<StyleIndex,
        styleIndexTransitionToInactiveOut,
        styleIndexTransitionToInactiveOver,
        styleIndexTransitionToFocusedOut,
        nullptr,
        styleIndexTransitionToPressedOut,
        styleIndexTransitionToPressedOver,
        styleIndexTransitionToDisabledDoNotCall>();
    {
        FocusEvent event{{}};
        CORRADE_VERIFY(ui.focusEvent(node, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), node);
        CORRADE_COMPARE(ui.currentFocusedNode(), node);
        CORRADE_COMPARE(layer.style<StyleIndex>(nodeData), expectedStyle);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }

    /* Setting a null toFocusedOut will do nothing for a focused leave */
    shared.setStyleTransition<StyleIndex,
        styleIndexTransitionToInactiveOut,
        styleIndexTransitionToInactiveOver,
        nullptr,
        styleIndexTransitionToFocusedOver,
        styleIndexTransitionToPressedOut,
        styleIndexTransitionToPressedOver,
        styleIndexTransitionToDisabledDoNotCall>();
    {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        CORRADE_VERIFY(!ui.pointerMoveEvent({100.0f, 100.0f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), node);
        CORRADE_COMPARE(layer.style<StyleIndex>(nodeData), expectedStyle);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }

    /* Setting a null toInactiveOut will do nothing for a blur */
    shared.setStyleTransition<StyleIndex,
        nullptr,
        styleIndexTransitionToInactiveOver,
        styleIndexTransitionToFocusedOut,
        styleIndexTransitionToFocusedOver,
        styleIndexTransitionToPressedOut,
        styleIndexTransitionToPressedOver,
        styleIndexTransitionToDisabledDoNotCall>();
    {
        FocusEvent event{{}};
        CORRADE_VERIFY(!ui.focusEvent(NodeHandle::Null, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(nodeData), expectedStyle);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }

    /* Make the node hovered and focused again */
    shared.setStyleTransition<StyleIndex,
        styleIndexTransitionToInactiveOut,
        nullptr,
        styleIndexTransitionToFocusedOut,
        nullptr,
        styleIndexTransitionToPressedOut,
        styleIndexTransitionToPressedOver,
        styleIndexTransitionToDisabledDoNotCall>();
    {
        PointerMoveEvent moveEvent{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        FocusEvent focusEvent{{}};
        CORRADE_VERIFY(ui.pointerMoveEvent({2.0f, 2.0f}, moveEvent));
        CORRADE_VERIFY(ui.focusEvent(node, focusEvent));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), node);
        CORRADE_COMPARE(ui.currentFocusedNode(), node);
        CORRADE_COMPARE(layer.style<StyleIndex>(nodeData), expectedStyle);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }

    /* Setting a null toInactiveOver will do nothing for a visiblity loss
       event */
    shared.setStyleTransition<StyleIndex,
        styleIndexTransitionToInactiveOut,
        nullptr,
        styleIndexTransitionToFocusedOut,
        styleIndexTransitionToFocusedOver,
        styleIndexTransitionToPressedOut,
        styleIndexTransitionToPressedOver,
        styleIndexTransitionToDisabledDoNotCall>();
    {
        ui.clearNodeFlags(node, NodeFlag::Focusable);
        ui.update();
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), node);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(nodeData), expectedStyle);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }

    /* Setting a null toInactiveOut will do nothing for a visibility loss event
       and null toDisabled nothing in doUpdate() */
    shared.setStyleTransition<StyleIndex,
        nullptr,
        styleIndexTransitionToInactiveOver,
        styleIndexTransitionToFocusedOut,
        styleIndexTransitionToFocusedOver,
        styleIndexTransitionToPressedOut,
        styleIndexTransitionToPressedOver,
        nullptr>();
    {
        ui.addNodeFlags(node, NodeFlag::Disabled);
        ui.update();
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(nodeData), expectedStyle);
        CORRADE_COMPARE(layer.state(), LayerStates{});

        ui.clearNodeFlags(node, NodeFlag::Disabled);
    }

    /* Setting a non-null but passthrough will do nothing in doUpdate() as
       well */
    shared.setStyleTransition<StyleIndex,
        styleIndexTransitionToInactiveOut,
        styleIndexTransitionToInactiveOver,
        styleIndexTransitionToFocusedOut,
        styleIndexTransitionToFocusedOver,
        styleIndexTransitionToPressedOut,
        styleIndexTransitionToPressedOver,
        styleIndexTransitionPassthrough>();
    {
        ui.addNodeFlags(node, NodeFlag::Disabled);
        ui.update();
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(nodeData), expectedStyle);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }
}

void AbstractVisualLayerTest::eventStyleTransition() {
    auto&& data = EventStyleTransitionData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Transition for dynamic styles (that don't have an animation with
       targetStyle()) tested in eventStyleTransitionDynamicStyle() */
    StyleLayerShared shared{StyleCount, data.dynamicAnimated ? 1u : 0u};

    /* StyleLayerShared uses the *_SHARED_SUBCLASS_IMPLEMENTATION() macro, this
       verifies that all the overrides do what's expected */
    StyleLayerShared* chaining;
    if(data.templated) chaining = &shared.setStyleTransition<StyleIndex,
        styleIndexTransitionToInactiveOut,
        styleIndexTransitionToInactiveOver,
        styleIndexTransitionToFocusedOut,
        styleIndexTransitionToFocusedOver,
        styleIndexTransitionToPressedOut,
        styleIndexTransitionToPressedOver,
        /* toDisabled transition is tested in eventStyleTransitionDisabled()
           instead */
        styleIndexTransitionToDisabledDoNotCall>();
    else chaining = &shared.setStyleTransition(
        [](UnsignedInt s) {
            return UnsignedInt(styleIndexTransitionToInactiveOut(StyleIndex(s)));
        },
        [](UnsignedInt s) {
            return UnsignedInt(styleIndexTransitionToInactiveOver(StyleIndex(s)));
        },
        [](UnsignedInt s) {
            return UnsignedInt(styleIndexTransitionToFocusedOut(StyleIndex(s)));
        },
        [](UnsignedInt s) {
            return UnsignedInt(styleIndexTransitionToFocusedOver(StyleIndex(s)));
        },
        [](UnsignedInt s) {
            return UnsignedInt(styleIndexTransitionToPressedOut(StyleIndex(s)));
        },
        [](UnsignedInt s) {
            return UnsignedInt(styleIndexTransitionToPressedOver(StyleIndex(s)));
        },
        /* toDisabled transition is tested in eventStyleTransitionDisabled()
           instead */
        [](UnsignedInt s) {
            return UnsignedInt(styleIndexTransitionToDisabledDoNotCall(StyleIndex(s)));
        });
    CORRADE_COMPARE(chaining, &shared);

    AbstractUserInterface ui{{100, 100}};

    /* A fallthrough node below all others. The events reach it, but it
       shouldn't react to them in any way because they're fallthrough. It's
       also focusable just in case those events would somehow reach there
       too. */
    NodeHandle nodeFallthrough = ui.createNode({}, {7.0f, 7.0f}, NodeFlag::FallthroughPointerEvents|NodeFlag::Focusable);

    /*   1  2  3  4  5  6
       2 +-----+  +-----+
       3 |green|  | red |
       4 +-----+  +-----+
       5 +-----+  +-----+
       6 |blue |  |white|
       7 +-----+  +-----+ */
    NodeHandle nodeGreen = ui.createNode(nodeFallthrough, {1.0f, 2.0f}, {2.0f, 2.0f});
    NodeHandle nodeRed = ui.createNode(nodeFallthrough, {4.0f, 2.0f}, {2.0f, 2.0f});
    NodeHandle nodeBlue = ui.createNode(nodeFallthrough, {1.0f, 5.0f}, {2.0f, 2.0f});
    NodeHandle nodeWhite = ui.createNode(nodeFallthrough, {4.0f, 5.0f}, {2.0f, 2.0f});

    StyleLayer& layer = ui.setLayerInstance(Containers::pointer<StyleLayer>(ui.createLayer(), shared));
    /* One extra data to verify it's mapping from nodes to data correctly */
    layer.create(StyleIndex::Green);
    DataHandle dataFallthrough = layer.create(StyleIndex::Green, nodeFallthrough);
    DataHandle dataGreen = layer.create(StyleIndex::Green, nodeGreen);
    DataHandle dataRed = layer.create(StyleIndex::Red, nodeRed);
    DataHandle dataBlue = layer.create(StyleIndex::Blue, nodeBlue);
    DataHandle dataWhite = layer.create(StyleIndex::White, nodeWhite);

    /* Animator. It'll be used below to temporarily replace data styles with
       dynamic ones that point to the correct one in targetStyle(). */
    StyleLayerStyleAnimator* animator{};
    if(data.dynamicAnimated) {
        Containers::Pointer<StyleLayerStyleAnimator> animatorInstance{InPlaceInit, ui.createAnimator()};
        layer.assignAnimator(*animatorInstance);
        layer.setDefaultStyleAnimator(animatorInstance.get());
        animator = &ui.setStyleAnimatorInstance(Utility::move(animatorInstance));
    }
    auto moveStyleToDynamic = [&](DataHandle data) {
        if(layer.dynamicStyleUsedCount() == 1)
            layer.recycleDynamicStyle(0);
        /* No need to attach the animation to the data */
        AnimationHandle animation = animator->create(layer.style(data), 0_nsec, 1_nsec, DataHandle::Null);
        layer.setStyle(data, StyleCount + *layer.allocateDynamicStyle(animation));
        CORRADE_COMPARE(layer.style(data), StyleCount + 0);
    };

    ui.update();
    CORRADE_COMPARE(layer.state(), LayerStates{});
    /* The style could be simply copied to calculatedStyles after an update as
       no transition is set */
    CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataFallthrough)]), StyleIndex::Green);
    CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataGreen)]), StyleIndex::Green);
    CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataRed)]), StyleIndex::Red);
    CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataBlue)]), StyleIndex::Blue);
    CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataWhite)]), StyleIndex::White);

    /* Press on the green node. The node isn't registered as hovered, so it's
       a press without a hover. Which usually happens with taps, for example,
       although it's not restricted to a particular Pointer type. */
    if(data.dynamicAnimated)
        moveStyleToDynamic(dataGreen);
    {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        CORRADE_VERIFY(ui.pointerPressEvent({2.0f, 3.0f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), nodeGreen);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentCapturedNode(), nodeGreen);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataGreen), StyleIndex::GreenPressed);
        /* No change to the fallthrough node */
        CORRADE_COMPARE(layer.style<StyleIndex>(dataFallthrough), StyleIndex::Green);
        CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);
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
        /* No change to the fallthrough node */
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataFallthrough)]), StyleIndex::Green);
    }

    /* Release on the green node. Again, the node isn't registered as hovered,
       so neither the hover stays. */
    if(data.dynamicAnimated)
        moveStyleToDynamic(dataGreen);
    {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        CORRADE_VERIFY(ui.pointerReleaseEvent({2.5f, 2.5f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataGreen), StyleIndex::Green);
        /* No change to the fallthrough node */
        CORRADE_COMPARE(layer.style<StyleIndex>(dataFallthrough), StyleIndex::Green);
        CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);
    }

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(layer.state(), LayerStates{});
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataGreen)]), StyleIndex::Green);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataRed)]), StyleIndex::Red);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataBlue)]), StyleIndex::Blue);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataWhite)]), StyleIndex::White);
        /* No change to the fallthrough node */
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataFallthrough)]), StyleIndex::Green);
    }

    /* Move on the red node makes it hovered */
    if(data.dynamicAnimated)
        moveStyleToDynamic(dataRed);
    {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        CORRADE_VERIFY(ui.pointerMoveEvent({5.0f, 3.0f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), nodeRed);
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataRed), StyleIndex::RedHover);
        /* No change to the fallthrough node */
        CORRADE_COMPARE(layer.style<StyleIndex>(dataFallthrough), StyleIndex::Green);
        CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);
    }

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(layer.state(), LayerStates{});
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataGreen)]), StyleIndex::Green);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataRed)]), StyleIndex::RedHover);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataBlue)]), StyleIndex::Blue);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataWhite)]), StyleIndex::White);
        /* No change to the fallthrough node */
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataFallthrough)]), StyleIndex::Green);
    }

    /* Tap on it makes it hovered & pressed */
    if(data.dynamicAnimated)
        moveStyleToDynamic(dataRed);
    {
        PointerEvent event{{}, PointerEventSource::Touch, Pointer::Finger, true, 0};
        CORRADE_VERIFY(ui.pointerPressEvent({4.5f, 3.5f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), nodeRed);
        CORRADE_COMPARE(ui.currentHoveredNode(), nodeRed);
        CORRADE_COMPARE(ui.currentCapturedNode(), nodeRed);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataRed), StyleIndex::RedPressedHover);
        /* No change to the fallthrough node */
        CORRADE_COMPARE(layer.style<StyleIndex>(dataFallthrough), StyleIndex::Green);
        CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);
    }

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(layer.state(), LayerStates{});
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataGreen)]), StyleIndex::Green);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataRed)]), StyleIndex::RedPressedHover);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataBlue)]), StyleIndex::Blue);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataWhite)]), StyleIndex::White);
        /* No change to the fallthrough node */
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataFallthrough)]), StyleIndex::Green);
    }

    /* Move away makes it only pressed, without hover, as implicit capture is
       in effect */
    if(data.dynamicAnimated)
        moveStyleToDynamic(dataRed);
    {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        CORRADE_VERIFY(ui.pointerMoveEvent({7.0f, 3.0f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), nodeRed);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentCapturedNode(), nodeRed);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataRed), StyleIndex::RedPressed);
        /* No change to the fallthrough node */
        CORRADE_COMPARE(layer.style<StyleIndex>(dataFallthrough), StyleIndex::Green);
        CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);
    }

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(layer.state(), LayerStates{});
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataGreen)]), StyleIndex::Green);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataRed)]), StyleIndex::RedPressed);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataBlue)]), StyleIndex::Blue);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataWhite)]), StyleIndex::White);
        /* No change to the fallthrough node */
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataFallthrough)]), StyleIndex::Green);
    }

    /* Move back makes it hovered & pressed again */
    if(data.dynamicAnimated)
        moveStyleToDynamic(dataRed);
    {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        CORRADE_VERIFY(ui.pointerMoveEvent({5.5f, 3.0f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), nodeRed);
        CORRADE_COMPARE(ui.currentHoveredNode(), nodeRed);
        CORRADE_COMPARE(ui.currentCapturedNode(), nodeRed);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataRed), StyleIndex::RedPressedHover);
        /* No change to the fallthrough node */
        CORRADE_COMPARE(layer.style<StyleIndex>(dataFallthrough), StyleIndex::Green);
        CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);
    }

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(layer.state(), LayerStates{});
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataGreen)]), StyleIndex::Green);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataRed)]), StyleIndex::RedPressedHover);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataBlue)]), StyleIndex::Blue);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataWhite)]), StyleIndex::White);
        /* No change to the fallthrough node */
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataFallthrough)]), StyleIndex::Green);
    }

    /* Release makes it only hover again */
    if(data.dynamicAnimated)
        moveStyleToDynamic(dataRed);
    {
        PointerEvent event{{}, PointerEventSource::Touch, Pointer::Finger, true, 0};
        CORRADE_VERIFY(ui.pointerReleaseEvent({5.0f, 2.5f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), nodeRed);
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataRed), StyleIndex::RedHover);
        /* No change to the fallthrough node */
        CORRADE_COMPARE(layer.style<StyleIndex>(dataFallthrough), StyleIndex::Green);
        CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);
    }

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(layer.state(), LayerStates{});
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataGreen)]), StyleIndex::Green);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataRed)]), StyleIndex::RedHover);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataBlue)]), StyleIndex::Blue);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataWhite)]), StyleIndex::White);
        /* No change to the fallthrough node */
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataFallthrough)]), StyleIndex::Green);
    }

    /* Move away makes it not hovered anymore */
    if(data.dynamicAnimated)
        moveStyleToDynamic(dataRed);
    {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        CORRADE_VERIFY(!ui.pointerMoveEvent({7.0f, 2.5f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataRed), StyleIndex::Red);
        /* No change to the fallthrough node */
        CORRADE_COMPARE(layer.style<StyleIndex>(dataFallthrough), StyleIndex::Green);
        CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);
    }

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(layer.state(), LayerStates{});
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataGreen)]), StyleIndex::Green);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataRed)]), StyleIndex::Red);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataBlue)]), StyleIndex::Blue);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataWhite)]), StyleIndex::White);
        /* No change to the fallthrough node */
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataFallthrough)]), StyleIndex::Green);
    }

    /* Make the Green, Red and Blue nodes focusable for the rest of the test
       case */
    ui.addNodeFlags(nodeGreen, NodeFlag::Focusable);
    ui.addNodeFlags(nodeRed, NodeFlag::Focusable);
    ui.addNodeFlags(nodeBlue, NodeFlag::Focusable);

    /* Focusing the green node makes it focused */
    if(data.dynamicAnimated)
        moveStyleToDynamic(dataGreen);
    {
        FocusEvent event{{}};
        CORRADE_VERIFY(ui.focusEvent(nodeGreen, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), nodeGreen);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataGreen), StyleIndex::GreenFocused);
        /* No change to the fallthrough node */
        CORRADE_COMPARE(layer.style<StyleIndex>(dataFallthrough), StyleIndex::Green);
        CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);
    }

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(layer.state(), LayerStates{});
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataGreen)]), StyleIndex::GreenFocused);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataRed)]), StyleIndex::Red);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataBlue)]), StyleIndex::Blue);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataWhite)]), StyleIndex::White);
        /* No change to the fallthrough node */
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataFallthrough)]), StyleIndex::Green);
    }

    /* Moving onto the green node makes it focused & hovered */
    if(data.dynamicAnimated)
        moveStyleToDynamic(dataGreen);
    {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        CORRADE_VERIFY(ui.pointerMoveEvent({2.0f, 2.5f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), nodeGreen);
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), nodeGreen);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataGreen), StyleIndex::GreenFocusedHover);
        /* No change to the fallthrough node */
        CORRADE_COMPARE(layer.style<StyleIndex>(dataFallthrough), StyleIndex::Green);
        CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);
    }

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(layer.state(), LayerStates{});
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataGreen)]), StyleIndex::GreenFocusedHover);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataRed)]), StyleIndex::Red);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataBlue)]), StyleIndex::Blue);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataWhite)]), StyleIndex::White);
        /* No change to the fallthrough node */
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataFallthrough)]), StyleIndex::Green);
    }

    /* Pressing on the green node makes it pressed & hovered, as that has a
       priority over focus */
    if(data.dynamicAnimated)
        moveStyleToDynamic(dataGreen);
    {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        CORRADE_VERIFY(ui.pointerPressEvent({2.5f, 2.0f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), nodeGreen);
        CORRADE_COMPARE(ui.currentHoveredNode(), nodeGreen);
        CORRADE_COMPARE(ui.currentCapturedNode(), nodeGreen);
        CORRADE_COMPARE(ui.currentFocusedNode(), nodeGreen);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataGreen), StyleIndex::GreenPressedHover);
        /* No change to the fallthrough node */
        CORRADE_COMPARE(layer.style<StyleIndex>(dataFallthrough), StyleIndex::Green);
        CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);
    }

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(layer.state(), LayerStates{});
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataGreen)]), StyleIndex::GreenPressedHover);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataRed)]), StyleIndex::Red);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataBlue)]), StyleIndex::Blue);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataWhite)]), StyleIndex::White);
        /* No change to the fallthrough node */
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataFallthrough)]), StyleIndex::Green);
    }

    /* Moving away from the green node makes it only pressed, again with that
       taking precedence over focus */
    if(data.dynamicAnimated)
        moveStyleToDynamic(dataGreen);
    {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        CORRADE_VERIFY(ui.pointerMoveEvent({100.0f, 100.0f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), nodeGreen);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentCapturedNode(), nodeGreen);
        CORRADE_COMPARE(ui.currentFocusedNode(), nodeGreen);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataGreen), StyleIndex::GreenPressed);
        /* No change to the fallthrough node */
        CORRADE_COMPARE(layer.style<StyleIndex>(dataFallthrough), StyleIndex::Green);
        CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);
    }

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(layer.state(), LayerStates{});
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataGreen)]), StyleIndex::GreenPressed);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataRed)]), StyleIndex::Red);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataBlue)]), StyleIndex::Blue);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataWhite)]), StyleIndex::White);
        /* No change to the fallthrough node */
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataFallthrough)]), StyleIndex::Green);
    }

    /* Moving back to the green node makes it again pressed & hovered, taking
       precedence over focus */
    if(data.dynamicAnimated)
        moveStyleToDynamic(dataGreen);
    {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        CORRADE_VERIFY(ui.pointerMoveEvent({2.0f, 2.5f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), nodeGreen);
        CORRADE_COMPARE(ui.currentHoveredNode(), nodeGreen);
        CORRADE_COMPARE(ui.currentCapturedNode(), nodeGreen);
        CORRADE_COMPARE(ui.currentFocusedNode(), nodeGreen);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataGreen), StyleIndex::GreenPressedHover);
        /* No change to the fallthrough node */
        CORRADE_COMPARE(layer.style<StyleIndex>(dataFallthrough), StyleIndex::Green);
        CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);
    }

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(layer.state(), LayerStates{});
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataGreen)]), StyleIndex::GreenPressedHover);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataRed)]), StyleIndex::Red);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataBlue)]), StyleIndex::Blue);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataWhite)]), StyleIndex::White);
        /* No change to the fallthrough node */
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataFallthrough)]), StyleIndex::Green);
    }

    /* Releasing on the green node makes it focused & hovered */
    if(data.dynamicAnimated)
        moveStyleToDynamic(dataGreen);
    {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        CORRADE_VERIFY(ui.pointerReleaseEvent({2.5f, 2.0f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), nodeGreen);
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), nodeGreen);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataGreen), StyleIndex::GreenFocusedHover);
        /* No change to the fallthrough node */
        CORRADE_COMPARE(layer.style<StyleIndex>(dataFallthrough), StyleIndex::Green);
        CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);
    }

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(layer.state(), LayerStates{});
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataGreen)]), StyleIndex::GreenFocusedHover);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataRed)]), StyleIndex::Red);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataBlue)]), StyleIndex::Blue);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataWhite)]), StyleIndex::White);
        /* No change to the fallthrough node */
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataFallthrough)]), StyleIndex::Green);
    }

    /* Blurring the green node makes it just focused */
    if(data.dynamicAnimated)
        moveStyleToDynamic(dataGreen);
    {
        FocusEvent event{{}};
        CORRADE_VERIFY(!ui.focusEvent(NodeHandle::Null, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), nodeGreen);
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataGreen), StyleIndex::GreenHover);
        /* No change to the fallthrough node */
        CORRADE_COMPARE(layer.style<StyleIndex>(dataFallthrough), StyleIndex::Green);
        CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);
    }

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(layer.state(), LayerStates{});
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataGreen)]), StyleIndex::GreenHover);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataRed)]), StyleIndex::Red);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataBlue)]), StyleIndex::Blue);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataWhite)]), StyleIndex::White);
        /* No change to the fallthrough node */
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataFallthrough)]), StyleIndex::Green);
    }

    /* Focusing the green node makes it focused & hovered again */
    if(data.dynamicAnimated)
        moveStyleToDynamic(dataGreen);
    {
        FocusEvent event{{}};
        CORRADE_VERIFY(ui.focusEvent(nodeGreen, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), nodeGreen);
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), nodeGreen);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataGreen), StyleIndex::GreenFocusedHover);
        /* No change to the fallthrough node */
        CORRADE_COMPARE(layer.style<StyleIndex>(dataFallthrough), StyleIndex::Green);
        CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);
    }

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(layer.state(), LayerStates{});
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataGreen)]), StyleIndex::GreenFocusedHover);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataRed)]), StyleIndex::Red);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataBlue)]), StyleIndex::Blue);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataWhite)]), StyleIndex::White);
        /* No change to the fallthrough node */
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataFallthrough)]), StyleIndex::Green);
    }

    /* Moving away from the green node makes it only focused */
    if(data.dynamicAnimated)
        moveStyleToDynamic(dataGreen);
    {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        CORRADE_VERIFY(!ui.pointerMoveEvent({100.0f, 100.0f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), nodeGreen);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataGreen), StyleIndex::GreenFocused);
        /* No change to the fallthrough node */
        CORRADE_COMPARE(layer.style<StyleIndex>(dataFallthrough), StyleIndex::Green);
        CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);
    }

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(layer.state(), LayerStates{});
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataGreen)]), StyleIndex::GreenFocused);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataRed)]), StyleIndex::Red);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataBlue)]), StyleIndex::Blue);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataWhite)]), StyleIndex::White);
        /* No change to the fallthrough node */
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataFallthrough)]), StyleIndex::Green);
    }

    /* Pressing on the green node makes it pressed, as that has again a
       priority over focus */
    if(data.dynamicAnimated)
        moveStyleToDynamic(dataGreen);
    {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        CORRADE_VERIFY(ui.pointerPressEvent({2.5f, 2.0f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), nodeGreen);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentCapturedNode(), nodeGreen);
        CORRADE_COMPARE(ui.currentFocusedNode(), nodeGreen);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataGreen), StyleIndex::GreenPressed);
        /* No change to the fallthrough node */
        CORRADE_COMPARE(layer.style<StyleIndex>(dataFallthrough), StyleIndex::Green);
        CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);
    }

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(layer.state(), LayerStates{});
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataGreen)]), StyleIndex::GreenPressed);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataRed)]), StyleIndex::Red);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataBlue)]), StyleIndex::Blue);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataWhite)]), StyleIndex::White);
        /* No change to the fallthrough node */
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataFallthrough)]), StyleIndex::Green);
    }

    /* Releasing on the green node makes it again focused */
    if(data.dynamicAnimated)
        moveStyleToDynamic(dataGreen);
    {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        CORRADE_VERIFY(ui.pointerReleaseEvent({2.5f, 2.0f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), nodeGreen);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataGreen), StyleIndex::GreenFocused);
        /* No change to the fallthrough node */
        CORRADE_COMPARE(layer.style<StyleIndex>(dataFallthrough), StyleIndex::Green);
        CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);
    }

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(layer.state(), LayerStates{});
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataGreen)]), StyleIndex::GreenFocused);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataRed)]), StyleIndex::Red);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataBlue)]), StyleIndex::Blue);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataWhite)]), StyleIndex::White);
        /* No change to the fallthrough node */
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataFallthrough)]), StyleIndex::Green);
    }

    /* Blurring the green node makes it inactive again */
    if(data.dynamicAnimated)
        moveStyleToDynamic(dataGreen);
    {
        FocusEvent event{{}};
        CORRADE_VERIFY(!ui.focusEvent(NodeHandle::Null, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataGreen), StyleIndex::Green);
        /* No change to the fallthrough node */
        CORRADE_COMPARE(layer.style<StyleIndex>(dataFallthrough), StyleIndex::Green);
        CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);
    }

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(layer.state(), LayerStates{});
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataGreen)]), StyleIndex::Green);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataRed)]), StyleIndex::Red);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataBlue)]), StyleIndex::Blue);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataWhite)]), StyleIndex::White);
        /* No change to the fallthrough node */
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataFallthrough)]), StyleIndex::Green);
    }

    /* Move on and away from the blue is accepted but makes no change to it,
       thus no update is needed. With the dynamic animated style it means the
       animation is left running, because it eventually arrives at the desired
       style. */
    if(data.dynamicAnimated)
        moveStyleToDynamic(dataBlue);
    {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        CORRADE_VERIFY(ui.pointerMoveEvent({2.0f, 6.0f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), nodeBlue);
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataBlue), data.dynamicAnimated ? StyleIndex(StyleCount + 0) : StyleIndex::Blue);
        /* No change to the fallthrough node */
        CORRADE_COMPARE(layer.style<StyleIndex>(dataFallthrough), StyleIndex::Green);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        CORRADE_VERIFY(!ui.pointerMoveEvent({2.5f, 8.0f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataBlue), data.dynamicAnimated ? StyleIndex(StyleCount + 0) : StyleIndex::Blue);
        /* No change to the fallthrough node */
        CORRADE_COMPARE(layer.style<StyleIndex>(dataFallthrough), StyleIndex::Green);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }

    /* Press and release on the white is accepted but makes no change to it,
       thus no update is needed. With the dynamic animated style it means the
       animation is left running, because it eventually arrives at the desired
       style. */
    if(data.dynamicAnimated)
        moveStyleToDynamic(dataWhite);
    {
        PointerEvent event{{}, PointerEventSource::Pen, Pointer::Pen, true, 0};
        CORRADE_VERIFY(ui.pointerPressEvent({5.0f, 5.0f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), nodeWhite);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataWhite), data.dynamicAnimated ? StyleIndex(StyleCount + 0) : StyleIndex::White);
        /* No change to the fallthrough node */
        CORRADE_COMPARE(layer.style<StyleIndex>(dataFallthrough), StyleIndex::Green);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    } {
        PointerEvent event{{}, PointerEventSource::Pen, Pointer::Pen, true, 0};
        CORRADE_VERIFY(ui.pointerReleaseEvent({5.5f, 4.5f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataWhite), data.dynamicAnimated ? StyleIndex(StyleCount + 0) : StyleIndex::White);
        /* No change to the fallthrough node */
        CORRADE_COMPARE(layer.style<StyleIndex>(dataFallthrough), StyleIndex::Green);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }

    /* Press and release on the green node again, but with a right click. Such
       event isn't even accepted and should cause no change either.

       Thus also moveStyleToDynamic() isn't called for it in case of dynamic
       animated styles because the code doesn't even get to query the
       animation. */
    {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseRight, true, 0};
        CORRADE_VERIFY(!ui.pointerPressEvent({2.0f, 3.0f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataGreen), StyleIndex::Green);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    } {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseRight, true, 0};
        CORRADE_VERIFY(!ui.pointerReleaseEvent({1.5f, 2.5f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataGreen), StyleIndex::Green);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }

    /* Move in, out, press and release with a secondary finger. Such events
       also aren't even accepted and should cause no changes. */
    {
        PointerMoveEvent event{{}, PointerEventSource::Touch, {}, Pointer::Finger, false, 0};
        CORRADE_VERIFY(!ui.pointerMoveEvent({2.0f, 3.0f}, event));
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataGreen), StyleIndex::Green);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    } {
        PointerMoveEvent event{{}, PointerEventSource::Touch, {}, Pointer::Finger, false, 0};
        CORRADE_VERIFY(!ui.pointerMoveEvent({7.0f, 2.5f}, event));
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataGreen), StyleIndex::Green);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    } {
        PointerEvent event{{}, PointerEventSource::Touch, Pointer::Finger, false, 0};
        CORRADE_VERIFY(!ui.pointerPressEvent({2.0f, 3.0f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataGreen), StyleIndex::Green);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    } {
        PointerEvent event{{}, PointerEventSource::Touch, Pointer::Finger, false, 0};
        CORRADE_VERIFY(!ui.pointerReleaseEvent({1.5f, 2.5f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataGreen), StyleIndex::Green);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }

    /* Focus and blur on the red node is accepted but makes no changes to it,
       thus no update is needed. With the dynamic animated style it means the
       animation is left running, because it eventually arrives at the desired
       style. */
    if(data.dynamicAnimated)
        moveStyleToDynamic(dataRed);
    {
        FocusEvent event{{}};
        CORRADE_VERIFY(ui.focusEvent(nodeRed, event));
        CORRADE_COMPARE(ui.currentFocusedNode(), nodeRed);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataRed), data.dynamicAnimated ? StyleIndex(StyleCount + 0) : StyleIndex::Red);
        /* No change to the fallthrough node */
        CORRADE_COMPARE(layer.style<StyleIndex>(dataFallthrough), StyleIndex::Green);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    } {
        FocusEvent event{{}};
        CORRADE_VERIFY(!ui.focusEvent(NodeHandle::Null, event));
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataRed), data.dynamicAnimated ? StyleIndex(StyleCount + 0) : StyleIndex::Red);
        /* No change to the fallthrough node */
        CORRADE_COMPARE(layer.style<StyleIndex>(dataFallthrough), StyleIndex::Green);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }
}

void AbstractVisualLayerTest::eventStyleTransitionNoHover() {
    auto&& data = EventStyleTransitionData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Transition for dynamic styles tested in
       eventStyleTransitionDynamicStyle(). Transition for dynamic styles that
       have an associated animation with targetStyle set not tested here, as
       that was covered well enough in eventStyleTransition() already, and this
       is only verifying that the simpler setStyleTransition() overloads with
       no hover state behave as expected. */
    StyleLayerShared shared{6, 0};

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
        styleIndexTransitionToInactiveOut,
        styleIndexTransitionToFocusedOut,
        styleIndexTransitionToPressedOut,
        /* "no hover" toDisabled transition is tested in
           eventStyleTransitionDisabled() instead */
        styleIndexTransitionToDisabledDoNotCall>();
    else chaining = &shared.setStyleTransition(
        [](UnsignedInt s) {
            return UnsignedInt(styleIndexTransitionToInactiveOut(StyleIndex(s)));
        },
        [](UnsignedInt s) {
            return UnsignedInt(styleIndexTransitionToFocusedOut(StyleIndex(s)));
        },
        [](UnsignedInt s) {
            return UnsignedInt(styleIndexTransitionToPressedOut(StyleIndex(s)));
        },
        /* "no hover" toDisabled transition is tested in
           eventStyleTransitionDisabled() instead */
        [](UnsignedInt s) {
            return UnsignedInt(styleIndexTransitionToDisabledDoNotCall(StyleIndex(s)));
        });
    CORRADE_COMPARE(chaining, &shared);

    auto testPressRelease = [&]{
        {
            PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
            CORRADE_VERIFY(ui.pointerPressEvent({2.5f, 2.0f}, event));
            CORRADE_COMPARE(layer.style<StyleIndex>(layerData), StyleIndex::GreenPressed);
            CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);
        }

        if(data.update) {
            ui.update();
            CORRADE_COMPARE(layer.state(), LayerStates{});
        }

        {
            PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
            CORRADE_VERIFY(ui.pointerReleaseEvent({2.5f, 2.5f}, event));
            CORRADE_COMPARE(layer.style<StyleIndex>(layerData), StyleIndex::Green);
            CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);
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
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        CORRADE_VERIFY(ui.pointerMoveEvent({2.0f, 2.0f}, event));
        CORRADE_COMPARE(ui.currentHoveredNode(), node);
        CORRADE_COMPARE(layer.style<StyleIndex>(layerData), StyleIndex::Green);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }

    /* Press & release with a hover should behave the same as without */
    testPressRelease();

    /* Moving away should do nothing again */
    {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        CORRADE_VERIFY(!ui.pointerMoveEvent({100.0f, 100.0f}, event));
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(layerData), StyleIndex::Green);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }

    /* Make the node focusable for the rest of the test case */
    ui.addNodeFlags(node, NodeFlag::Focusable);

    auto testFocusBlur = [&]{
        {
            FocusEvent event{{}};
            CORRADE_VERIFY(ui.focusEvent(node, event));
            CORRADE_COMPARE(layer.style<StyleIndex>(layerData), StyleIndex::GreenFocused);
            CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);
        }

        ui.update();
        CORRADE_COMPARE(layer.state(), LayerStates{});

        {
            FocusEvent event{{}};
            CORRADE_VERIFY(!ui.focusEvent(NodeHandle::Null, event));
            CORRADE_COMPARE(layer.style<StyleIndex>(layerData), StyleIndex::Green);
            CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);
        }

        ui.update();
        CORRADE_COMPARE(layer.state(), LayerStates{});
    };

    /* Test focus & blur without a hover */
    testFocusBlur();

    /* Moving onto the node should do nothing */
    {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        CORRADE_VERIFY(ui.pointerMoveEvent({2.0f, 2.0f}, event));
        CORRADE_COMPARE(ui.currentHoveredNode(), node);
        CORRADE_COMPARE(layer.style<StyleIndex>(layerData), StyleIndex::Green);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }

    /* Focus & blur with a hover should behave the same as without */
    testFocusBlur();

    /* Moving away should do nothing again */
    {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        CORRADE_VERIFY(!ui.pointerMoveEvent({100.0f, 100.0f}, event));
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
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
    StyleLayerShared shared{StyleCount, 0};
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
        nullptr,
        nullptr,
        styleIndexTransitionToDisabled>();
    else chaining = &shared.setStyleTransition(
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        [](UnsignedInt s) {
            return UnsignedInt(styleIndexTransitionToDisabled(StyleIndex(s)));
        });
    CORRADE_COMPARE(chaining, &shared);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

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
        nullptr,
        nullptr,
        nullptr>();
    else shared.setStyleTransition(
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

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
        nullptr,
        styleIndexTransitionToDisabled>();
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

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
        nullptr,
        nullptr>();
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

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
    StyleLayerShared shared{6, 0};
    shared.setStyleTransition<StyleIndex,
        styleIndexTransitionToInactiveOut,
        styleIndexTransitionToInactiveOver,
        styleIndexTransitionToFocusedOut,
        styleIndexTransitionToFocusedOver,
        styleIndexTransitionToPressedOut,
        styleIndexTransitionToPressedOver,
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

    NodeHandle node = ui.createNode({1.0f, 1.0f}, {2.0f, 2.0f}, data.focusable ? NodeFlag::Focusable : NodeFlags{});

    StyleLayer& layer = ui.setLayerInstance(Containers::pointer<StyleLayer>(ui.createLayer(), shared));
    DataHandle layerData = layer.create(StyleIndex::Green, node);

    EventLayer& eventLayer = ui.setLayerInstance(Containers::pointer<EventLayer>(ui.createLayer(), data.disableCapture));
    eventLayer.create(node);

    ui.update();
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Move onto the node is capture-independent */
    {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        CORRADE_VERIFY(ui.pointerMoveEvent({2.0f, 2.0f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), node);
        CORRADE_COMPARE(layer.style<StyleIndex>(layerData), StyleIndex::GreenHover);

    /* Press will enable the capture, maybe */
    } {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        CORRADE_VERIFY(ui.pointerPressEvent({2.5f, 2.0f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), node);
        CORRADE_COMPARE(ui.currentCapturedNode(), data.disableCapture ? NodeHandle::Null : node);
        CORRADE_COMPARE(ui.currentFocusedNode(), data.focusable ? node : NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(layerData), StyleIndex::GreenPressedHover);

    /* Move away will only preserve the press if capture is set */
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        CORRADE_COMPARE(ui.pointerMoveEvent({7.0f, 2.0f}, event), !data.disableCapture);
        CORRADE_COMPARE(ui.currentPressedNode(), data.disableCapture ? NodeHandle::Null : node);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), data.focusable ? node : NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(layerData), data.outStyle);

    /* Move back will only preserve the press if capture is set */
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        CORRADE_VERIFY(ui.pointerMoveEvent({2.0f, 2.0f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), data.disableCapture ? NodeHandle::Null : node);
        CORRADE_COMPARE(ui.currentHoveredNode(), node);
        CORRADE_COMPARE(ui.currentFocusedNode(), data.focusable ? node : NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(layerData), data.overStyle);
    }
}

void AbstractVisualLayerTest::eventStyleTransitionNodeBecomesHiddenDisabledNoEvents() {
    auto&& data = EventStyleTransitionNodeBecomesHiddenDisabledNoEventsData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Transition for dynamic styles tested in
       eventStyleTransitionDynamicStyle() instead */
    StyleLayerShared shared{StyleCount, 0};
    shared.setStyleTransition<StyleIndex,
        styleIndexTransitionToInactiveOut,
        styleIndexTransitionToInactiveOver,
        styleIndexTransitionToFocusedOut,
        styleIndexTransitionToFocusedOver,
        styleIndexTransitionToPressedOut,
        styleIndexTransitionToPressedOver,
        styleIndexTransitionToDisabled>();

    AbstractUserInterface ui{{100, 100}};

    /* Both nodes are children of the root node, on which the flags get set, to
       verify it correctly propagates downwards

         1  2  3  4  5  6
       2 +-----+  +-----+
       3 |green|  | red |
       4 +-----+  +-----+
       5 +-----+
       6 |blue |
       7 +-----+          */
    NodeHandle root = ui.createNode({}, {10.0f, 10.0f});
    NodeHandle nodeGreen = ui.createNode(root, {1.0f, 2.0f}, {2.0f, 2.0f});
    NodeHandle nodeRed = ui.createNode(root, {4.0f, 2.0f}, {2.0f, 2.0f});
    NodeHandle nodeBlue = ui.createNode(root, {1.0f, 5.0f}, {2.0f, 2.0f}, NodeFlag::Focusable);

    StyleLayer& layer = ui.setLayerInstance(Containers::pointer<StyleLayer>(ui.createLayer(), shared));
    /* One extra data to verify it's mapping from nodes to data correctly */
    layer.create(StyleIndex::Green);
    DataHandle dataGreen = layer.create(StyleIndex::Green, nodeGreen);
    DataHandle dataRed = layer.create(StyleIndex::Red, nodeRed);
    DataHandle dataBlue = layer.create(StyleIndex::Blue, nodeBlue);

    ui.update();
    CORRADE_COMPARE(layer.state(), LayerStates{});
    CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataGreen)]), StyleIndex::Green);
    CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataRed)]), StyleIndex::Red);

    /* Press on the green node, hover on the red, focus on the blue */
    {
        PointerMoveEvent moveEvent{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        PointerEvent pressEvent{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        FocusEvent focusEvent{{}};
        CORRADE_VERIFY(ui.pointerMoveEvent({5.0f, 3.0f}, moveEvent));
        CORRADE_VERIFY(ui.pointerPressEvent({2.0f, 3.0f}, pressEvent));
        CORRADE_VERIFY(ui.focusEvent(nodeBlue, focusEvent));

        CORRADE_COMPARE(ui.currentPressedNode(), nodeGreen);
        CORRADE_COMPARE(ui.currentHoveredNode(), nodeRed);
        CORRADE_COMPARE(ui.currentFocusedNode(), nodeBlue);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataGreen), StyleIndex::GreenPressed);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataRed), StyleIndex::RedHover);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataBlue), StyleIndex::BlueFocused);
        CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);
    }

    ui.update();
    CORRADE_COMPARE(layer.state(), LayerStates{});
    CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataGreen)]), StyleIndex::GreenPressed);
    CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataRed)]), StyleIndex::RedHover);
    CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataBlue)]), StyleIndex::BlueFocused);

    /* Changing the flags makes the node not react to events anymore, which
       means it should lose all pressed/hover visual state */
    if(data.flags)
        ui.addNodeFlags(root, data.flags);
    else if(data.clearOrder)
        ui.clearNodeOrder(root);
    else CORRADE_INTERNAL_ASSERT_UNREACHABLE();

    /* A single update() call should be enough, not the update itself
       scheduling another update in the doVisibilityLostEvent() */
    ui.update();
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});

    /* Unless the node is hidden (at which point the data don't get touched at
       all), the style should be updated */
    if(!data.becomesHidden) {
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataGreen)]), data.expectedGreenStyle);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataRed)]), data.expectedRedStyle);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataBlue)]), data.expectedBlueStyle);
    }

    /* Changing the flags back should not regain pressed/hover/focused state */
    if(data.flags)
        ui.clearNodeFlags(root, data.flags);
    else if(data.clearOrder)
        ui.setNodeOrder(root, NodeHandle::Null);
    else CORRADE_INTERNAL_ASSERT_UNREACHABLE();

    ui.update();
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataGreen)]), StyleIndex::Green);
    CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataRed)]), StyleIndex::Red);
    CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataBlue)]), StyleIndex::Blue);

    /* Both press & hover on the green node */
    {
        PointerMoveEvent moveEvent{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        PointerEvent pressEvent{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        CORRADE_VERIFY(ui.pointerMoveEvent({2.0f, 3.0f}, moveEvent));
        CORRADE_VERIFY(ui.pointerPressEvent({2.0f, 3.0f}, pressEvent));

        CORRADE_COMPARE(ui.currentPressedNode(), nodeGreen);
        CORRADE_COMPARE(ui.currentHoveredNode(), nodeGreen);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataGreen), StyleIndex::GreenPressedHover);
        CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);
    }

    ui.update();
    CORRADE_COMPARE(layer.state(), LayerStates{});
    CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataGreen)]), StyleIndex::GreenPressedHover);

    /* Resetting from this state again */
    if(data.flags)
        ui.addNodeFlags(root, data.flags);
    else if(data.clearOrder)
        ui.clearNodeOrder(root);
    else CORRADE_INTERNAL_ASSERT_UNREACHABLE();

    ui.update();
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});

    /* Unless the node is hidden (at which point the data don't get touched at
       all), the style should be updated */
    if(!data.becomesHidden) {
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataGreen)]), data.expectedGreenStyle);
    }

    /* Changing the flags back should not regain pressed/hover state */
    if(data.flags)
        ui.clearNodeFlags(root, data.flags);
    else if(data.clearOrder)
        ui.setNodeOrder(root, NodeHandle::Null);
    else CORRADE_INTERNAL_ASSERT_UNREACHABLE();

    ui.update();
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataGreen)]), StyleIndex::Green);

    /* Make the green node focusable until the end of the test case */
    ui.addNodeFlags(nodeGreen, NodeFlag::Focusable);

    /* Focus & hover on the green node but then marking it as disabled */
    {
        PointerMoveEvent moveEvent{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        FocusEvent focusEvent{{}};
        CORRADE_VERIFY(ui.pointerMoveEvent({2.0f, 3.0f}, moveEvent));
        CORRADE_VERIFY(ui.focusEvent(nodeGreen, focusEvent));
        CORRADE_COMPARE(ui.currentHoveredNode(), nodeGreen);
        CORRADE_COMPARE(ui.currentFocusedNode(), nodeGreen);
        CORRADE_COMPARE(layer.style<StyleIndex>(dataGreen), StyleIndex::GreenFocusedHover);
        ui.addNodeFlags(nodeGreen, NodeFlag::Disabled);
    }

    ui.update();
    CORRADE_COMPARE(layer.state(), LayerStates{});
    CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataGreen)]), StyleIndex::GreenDisabled);

    /* Resetting from this state doesn't reset the disabled bit */
    if(data.flags)
        ui.addNodeFlags(root, data.flags);
    else if(data.clearOrder)
        ui.clearNodeOrder(root);
    else CORRADE_INTERNAL_ASSERT_UNREACHABLE();

    ui.update();
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(dataGreen)]), StyleIndex::GreenDisabled);
}

void AbstractVisualLayerTest::eventStyleTransitionNodeNoLongerFocusable() {
    auto&& data = EventStyleTransitionNodeNoLongerFocusableData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* A variant of eventStyleTransitionNodeBecomesHiddenDisabledNoEvents()
       that verifies behavior specific to the Focusable flag and focused
       nodes */

    /* Transition for dynamic styles tested in
       eventStyleTransitionDynamicStyle() instead */
    StyleLayerShared shared{StyleCount, 0};
    shared.setStyleTransition<StyleIndex,
        styleIndexTransitionToInactiveOut,
        styleIndexTransitionToInactiveOver,
        styleIndexTransitionToFocusedOut,
        styleIndexTransitionToFocusedOver,
        styleIndexTransitionToPressedOut,
        styleIndexTransitionToPressedOver,
        styleIndexTransitionToDisabled>();

    AbstractUserInterface ui{{100, 100}};

    NodeHandle node = ui.createNode({1.0f, 1.0f}, {2.0f, 2.0f});

    StyleLayer& layer = ui.setLayerInstance(Containers::pointer<StyleLayer>(ui.createLayer(), shared));
    DataHandle nodeData = layer.create(data.style, node);

    if(data.hovered) {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        CORRADE_VERIFY(ui.pointerMoveEvent({2.0f, 1.0f}, event));
        CORRADE_COMPARE(ui.currentHoveredNode(), node);
    }

    /* Doing a press on a non-focusable node so it doesn't imply a focus */
    if(data.pressed) {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        CORRADE_VERIFY(ui.pointerPressEvent({1.0f, 2.0f}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), node);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
    }

    /* Make the node focusable and focus it */
    {
        ui.addNodeFlags(node, NodeFlag::Focusable);

        FocusEvent focusEvent{{}};
        CORRADE_VERIFY(ui.focusEvent(node, focusEvent));
        CORRADE_COMPARE(ui.currentFocusedNode(), node);
    }

    CORRADE_COMPARE(layer.style<StyleIndex>(nodeData), data.expectedStyleBefore);

    ui.update();
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(ui.currentHoveredNode(), data.hovered ? node : NodeHandle::Null);
    CORRADE_COMPARE(ui.currentPressedNode(), data.pressed ? node : NodeHandle::Null);
    CORRADE_COMPARE(ui.currentFocusedNode(), node);
    CORRADE_COMPARE(layer.state(), LayerStates{});
    CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(nodeData)]), data.expectedStyleBefore);

    /* The node is no longer focusable, but the hovered/pressed state should
       stay */
    ui.clearNodeFlags(node, NodeFlag::Focusable);

    /* A single update() call should be enough, not the update itself
       scheduling another update in the doVisibilityLostEvent() */
    ui.update();
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(ui.currentHoveredNode(), data.hovered ? node : NodeHandle::Null);
    CORRADE_COMPARE(ui.currentPressedNode(), data.pressed ? node : NodeHandle::Null);
    CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
    CORRADE_COMPARE(StyleIndex(layer.style(nodeData)), data.expectedStyleAfter);
    CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(nodeData)]), data.expectedStyleAfter);
}

StyleIndex styleIndexTransitionOutOfRange(StyleIndex) {
    return StyleIndex(StyleCount);
}

void AbstractVisualLayerTest::eventStyleTransitionOutOfRange() {
    auto&& data = EventStyleTransitionOutOfRangeData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    CORRADE_SKIP_IF_NO_ASSERT();

    /* Style transition isn't performed on dynamic styles so this shouldn't
       affect it */
    StyleLayerShared shared{StyleCount, data.dynamicStyleCount};

    AbstractUserInterface ui{{100, 100}};

    NodeHandle node = ui.createNode({1.0f, 1.0f}, {2.0f, 2.0f}, NodeFlag::Focusable);

    StyleLayer& layer = ui.setLayerInstance(Containers::pointer<StyleLayer>(ui.createLayer(), shared));

    DataHandle nodeData;
    StyleLayerStyleAnimator* animator{};
    if(data.dynamicAnimated) {
        Containers::Pointer<StyleLayerStyleAnimator> animatorInstance{InPlaceInit, ui.createAnimator()};
        layer.assignAnimator(*animatorInstance);
        layer.setDefaultStyleAnimator(animatorInstance.get());
        animator = &ui.setStyleAnimatorInstance(Utility::move(animatorInstance));

        AnimationHandle nodeDataAnimation = animator->create(StyleIndex::Red, -100_nsec, 1_nsec, DataHandle::Null, AnimationFlag::KeepOncePlayed);
        nodeData = layer.create(StyleCount + *layer.allocateDynamicStyle(nodeDataAnimation), node);
        CORRADE_COMPARE(animator->targetStyle<StyleIndex>(nodeDataAnimation), StyleIndex::Red);

    } else nodeData = layer.create(StyleIndex::Red, node);

    ui.update();
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Tests an OOB condition happening in any of the four functions, and
       checked in any of the four event handlers. Does not exhaustively test
       all possible combinations, as that should not be needed.

       The same logic is used in eventStyleTransitionDynamicStyle() to exhaust
       all possibilities, keep in sync. */

    /* OOB toPressedOut transition */
    shared.setStyleTransition<StyleIndex,
        styleIndexTransitionToInactiveOut,
        styleIndexTransitionToInactiveOver,
        styleIndexTransitionToFocusedOut,
        styleIndexTransitionToFocusedOver,
        styleIndexTransitionOutOfRange,
        styleIndexTransitionToPressedOver,
        styleIndexTransitionToDisabledDoNotCall>();
    {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};

        std::ostringstream out;
        Error redirectError{&out};
        ui.pointerPressEvent({2.0f, 2.0f}, event);
        CORRADE_COMPARE(out.str(), Utility::formatString( "Ui::AbstractVisualLayer::pointerPressEvent(): style transition from {0} to {1} out of range for {1} styles\n", UnsignedByte(StyleIndex::Red), StyleCount));
    }

    /* OOB toPressedOver transition in the press event */
    shared.setStyleTransition<StyleIndex,
        styleIndexTransitionToInactiveOut,
        styleIndexTransitionToInactiveOver,
        styleIndexTransitionToFocusedOut,
        styleIndexTransitionToFocusedOver,
        styleIndexTransitionToPressedOut,
        styleIndexTransitionOutOfRange,
        styleIndexTransitionToDisabledDoNotCall>();
    {
        /* Doing a (non-asserting) move before so the hovered node is properly
           registered. Which then for dynamic animated styles needs to be
           followed by an animation that puts it back into the targetStyle. */
        PointerMoveEvent moveEvent{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        ui.pointerMoveEvent({1.5f, 2.0f}, moveEvent);
        if(data.dynamicAnimated) {
            layer.recycleDynamicStyle(0);
            AnimationHandle nodeDataAnimation = animator->create(StyleIndex::RedHover, -100_nsec, 1_nsec, DataHandle::Null, AnimationFlag::KeepOncePlayed);
            layer.setStyle(nodeData, StyleCount + *layer.allocateDynamicStyle(nodeDataAnimation));
            CORRADE_COMPARE(animator->targetStyle<StyleIndex>(nodeDataAnimation), StyleIndex::RedHover);
        }

        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};

        std::ostringstream out;
        Error redirectError{&out};
        ui.pointerPressEvent({2.0f, 2.0f}, event);
        CORRADE_COMPARE(out.str(), Utility::formatString("Ui::AbstractVisualLayer::pointerPressEvent(): style transition from {0} to {1} out of range for {1} styles\n", UnsignedByte(StyleIndex::RedHover), StyleCount));
    }

    /* OOB toInactiveOver transition */
    shared.setStyleTransition<StyleIndex,
        styleIndexTransitionToInactiveOut,
        styleIndexTransitionOutOfRange,
        styleIndexTransitionToFocusedOut,
        styleIndexTransitionToFocusedOver,
        styleIndexTransitionToPressedOut,
        styleIndexTransitionToPressedOver,
        styleIndexTransitionToDisabledDoNotCall>();
    {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};

        std::ostringstream out;
        Error redirectError{&out};
        ui.pointerReleaseEvent({1.5f, 2.5f}, event);
        CORRADE_COMPARE(out.str(), Utility::formatString("Ui::AbstractVisualLayer::pointerReleaseEvent(): style transition from {0} to {1} out of range for {1} styles\n", UnsignedInt(StyleIndex::RedHover), StyleCount));
    }

    /* OOB toInactiveOut transition in the leave event */
    shared.setStyleTransition<StyleIndex,
        styleIndexTransitionOutOfRange,
        styleIndexTransitionToInactiveOver,
        styleIndexTransitionToFocusedOut,
        styleIndexTransitionToFocusedOver,
        styleIndexTransitionToPressedOut,
        styleIndexTransitionToPressedOver,
        styleIndexTransitionToDisabledDoNotCall>();
    {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, {}, true, 0};

        std::ostringstream out;
        Error redirectError{&out};
        ui.pointerMoveEvent({8.5f, 2.0f}, event);
        CORRADE_COMPARE(out.str(), Utility::formatString("Ui::AbstractVisualLayer::pointerLeaveEvent(): style transition from {0} to {1} out of range for {1} styles\n", UnsignedInt(StyleIndex::RedHover), StyleCount));
    }

    /* OOB toInactiveOver transition in the enter event */
    shared.setStyleTransition<StyleIndex,
        styleIndexTransitionToInactiveOut,
        styleIndexTransitionOutOfRange,
        styleIndexTransitionToFocusedOut,
        styleIndexTransitionToFocusedOver,
        styleIndexTransitionToPressedOut,
        styleIndexTransitionToPressedOver,
        styleIndexTransitionToDisabledDoNotCall>();
    {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, {}, true, 0};

        std::ostringstream out;
        Error redirectError{&out};
        ui.pointerMoveEvent({1.5f, 2.0f}, event);
        CORRADE_COMPARE(out.str(), Utility::formatString("Ui::AbstractVisualLayer::pointerEnterEvent(): style transition from {0} to {1} out of range for {1} styles\n", UnsignedInt(StyleIndex::RedHover), StyleCount));
    }

    /* OOB toFocusedOver transition in the focus event */
    shared.setStyleTransition<StyleIndex,
        styleIndexTransitionToInactiveOut,
        styleIndexTransitionToInactiveOver,
        styleIndexTransitionToFocusedOut,
        styleIndexTransitionOutOfRange,
        styleIndexTransitionToPressedOut,
        styleIndexTransitionToPressedOver,
        styleIndexTransitionToDisabledDoNotCall>();
    {
        FocusEvent event{{}};

        std::ostringstream out;
        Error redirectError{&out};
        ui.focusEvent(node, event);
        CORRADE_COMPARE(out.str(), Utility::formatString("Ui::AbstractVisualLayer::focusEvent(): style transition from {0} to {1} out of range for {1} styles\n", UnsignedInt(StyleIndex::RedHover), StyleCount));
    }

    /* OOB toInactiveOver transition in the blur event. Doing a
       (non-asserting) focus before so the focused node is properly
       registered. */
    shared.setStyleTransition<StyleIndex,
        styleIndexTransitionToInactiveOut,
        styleIndexTransitionOutOfRange,
        styleIndexTransitionToFocusedOut,
        styleIndexTransitionToFocusedOver,
        styleIndexTransitionToPressedOut,
        styleIndexTransitionToPressedOver,
        styleIndexTransitionToDisabledDoNotCall>();
    {
        FocusEvent event1{{}};
        ui.focusEvent(node, event1);
        FocusEvent event2{{}};

        std::ostringstream out;
        Error redirectError{&out};
        ui.focusEvent(NodeHandle::Null, event2);
        CORRADE_COMPARE(out.str(), Utility::formatString("Ui::AbstractVisualLayer::blurEvent(): style transition from {0} to {1} out of range for {1} styles\n", UnsignedInt(StyleIndex::RedHover), StyleCount));
    }

    /* OOB toInactiveOut transition in the visibility lost event */
    shared.setStyleTransition<StyleIndex,
        styleIndexTransitionOutOfRange,
        styleIndexTransitionToInactiveOver,
        styleIndexTransitionToFocusedOut,
        styleIndexTransitionToFocusedOver,
        styleIndexTransitionToPressedOut,
        styleIndexTransitionToPressedOver,
        styleIndexTransitionToDisabledDoNotCall>();
    ui.addNodeFlags(node, NodeFlag::NoEvents);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeEnabledUpdate);
    {
        std::ostringstream out;
        Error redirectError{&out};
        ui.update();
        CORRADE_COMPARE(out.str(), Utility::formatString("Ui::AbstractVisualLayer::visibilityLostEvent(): style transition from {0} to {1} out of range for {1} styles\n", UnsignedInt(StyleIndex::RedHover), StyleCount));
    }

    /* OOB toDisabled transition in doUpdate() */
    shared.setStyleTransition<StyleIndex,
        styleIndexTransitionToInactiveOut,
        styleIndexTransitionToInactiveOver,
        styleIndexTransitionToFocusedOut,
        styleIndexTransitionToFocusedOver,
        styleIndexTransitionToPressedOut,
        styleIndexTransitionToPressedOver,
        styleIndexTransitionOutOfRange>();
    ui.addNodeFlags(node, NodeFlag::Disabled);
    CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeEnabledUpdate);
    {
        std::ostringstream out;
        Error redirectError{&out};
        ui.update();
        CORRADE_COMPARE(out.str(), Utility::formatString("Ui::AbstractVisualLayer::update(): style transition from {0} to {1} out of range for {1} styles\n", UnsignedInt(StyleIndex::RedHover), StyleCount));
    }
}

void AbstractVisualLayerTest::eventStyleTransitionDynamicStyle() {
    auto&& data = EventStyleTransitionDynamicStyleData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    StyleLayerShared shared{StyleCount, 1};

    AbstractUserInterface ui{{100, 100}};

    NodeHandle node = ui.createNode({1.0f, 1.0f}, {2.0f, 2.0f});
    NodeHandle nodeFocusable = ui.createNode({3.0f, 3.0f}, {2.0f, 2.0f}, NodeFlag::Focusable);

    StyleLayer& layer = ui.setLayerInstance(Containers::pointer<StyleLayer>(ui.createLayer(), shared));
    DataHandle nodeData = layer.create(StyleIndex::Green, node);
    DataHandle nodeFocusableData = layer.create(StyleIndex::Green, nodeFocusable);

    /* Optionally create animators that are or aren't set as default in the
       layer */
    StyleLayerStyleAnimator *animator1{}, *animator2{};
    if(data.animator1) {
        Containers::Pointer<StyleLayerStyleAnimator> animatorInstance{InPlaceInit, ui.createAnimator()};
        layer.assignAnimator(*animatorInstance);
        if(data.animator1SetDefault)
            layer.setDefaultStyleAnimator(animatorInstance.get());
        animator1 = &ui.setStyleAnimatorInstance(Utility::move(animatorInstance));
    } else CORRADE_INTERNAL_ASSERT(!data.animator1SetDefault && !data.animation1);
    if(data.animator2) {
        Containers::Pointer<StyleLayerStyleAnimator> animatorInstance{InPlaceInit, ui.createAnimator()};
        layer.assignAnimator(*animatorInstance);
        animator2 = &ui.setStyleAnimatorInstance(Utility::move(animatorInstance));
    } else CORRADE_INTERNAL_ASSERT(!data.animation2);

    /* And then, if there's an animator, create an animation that has a target
       (non-dynamic) style assigned. In all cases, there should be something
       missing or different, so the actual target style doesn't get used and
       the dynamic style stays untouched by the transitions. */
    DataHandle nodeDataDynamic;
    if(data.animation1 || data.animation2) {
        CORRADE_INTERNAL_ASSERT(data.animation1 != data.animation2);

        AnimationHandle nodeDataDynamicAnimation = (data.animation1 ? animator1 : animator2)->create(StyleIndex::Green, -100_nsec, 1_nsec, DataHandle::Null, AnimationFlag::KeepOncePlayed);
        nodeDataDynamic = layer.create(StyleCount + *layer.allocateDynamicStyle(data.dynamicStyleAssociatedAnimation ? nodeDataDynamicAnimation : AnimationHandle::Null), node);
    } else {
        CORRADE_INTERNAL_ASSERT(!data.dynamicStyleAssociatedAnimation);
        nodeDataDynamic = layer.create(StyleCount + 0, node);
    }

    /* This one reuses the same dynamic style, thus there's potentially the
       same animation with the same target style */
    DataHandle nodeFocusableDataDynamic = layer.create(StyleCount + 0, nodeFocusable);

    ui.update();
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* All these should get called only for the non-dynamic style. Logic the
       same as in eventStyleTransitionOutOfRange(), just not asserting in this
       case. Keep the two in sync. */
    shared.setStyleTransition<StyleIndex,
        styleIndexTransitionToInactiveOut,
        styleIndexTransitionToInactiveOver,
        styleIndexTransitionToFocusedOut,
        styleIndexTransitionToFocusedOver,
        styleIndexTransitionToPressedOut,
        styleIndexTransitionToPressedOver,
        styleIndexTransitionToDisabled>();

    /* toPressedOut transition */
    {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        ui.pointerPressEvent({2.0f, 2.0f}, event);
        CORRADE_COMPARE(ui.currentPressedNode(), node);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(nodeData), StyleIndex::GreenPressed);
        CORRADE_COMPARE(layer.style(nodeDataDynamic), StyleCount + 0);

    /* toPressedOver transition in the press event. Doing a move before so the
       hovered node is properly registered. */
    } {
        PointerMoveEvent moveEvent{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        ui.pointerMoveEvent({1.5f, 2.0f}, moveEvent);

        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        ui.pointerPressEvent({2.0f, 2.0f}, event);
        CORRADE_COMPARE(ui.currentPressedNode(), node);
        CORRADE_COMPARE(ui.currentHoveredNode(), node);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(nodeData), StyleIndex::GreenPressedHover);
        CORRADE_COMPARE(layer.style(nodeDataDynamic), StyleCount + 0);

    /* toInactiveOver transition */
    } {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        ui.pointerReleaseEvent({1.5f, 2.5f}, event);
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), node);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(nodeData), StyleIndex::GreenHover);
        CORRADE_COMPARE(layer.style(nodeDataDynamic), StyleCount + 0);

    /* toInactiveOut transition in the leave event */
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        ui.pointerMoveEvent({8.5f, 2.0f}, event);
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(nodeData), StyleIndex::Green);
        CORRADE_COMPARE(layer.style(nodeDataDynamic), StyleCount + 0);

    /* toInactiveOver transition in the enter event */
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        ui.pointerMoveEvent({1.5f, 2.0f}, event);
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), node);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(nodeData), StyleIndex::GreenHover);
        CORRADE_COMPARE(layer.style(nodeDataDynamic), StyleCount + 0);

    /* toFocused transition in the focus event */
    } {
        FocusEvent event{{}};
        ui.focusEvent(nodeFocusable, event);
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), node);
        CORRADE_COMPARE(ui.currentFocusedNode(), nodeFocusable);
        CORRADE_COMPARE(layer.style<StyleIndex>(nodeFocusableData), StyleIndex::GreenFocused);
        CORRADE_COMPARE(layer.style(nodeFocusableDataDynamic), StyleCount + 0);

    /* toInactive transition in the blur event */
    } {
        FocusEvent event{{}};
        ui.focusEvent(NodeHandle::Null, event);
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), node);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(nodeFocusableData), StyleIndex::Green);
        CORRADE_COMPARE(layer.style(nodeFocusableDataDynamic), StyleCount + 0);

    /* toInactiveOver transition in doUpdate(), from a focused hovered node */
    } {
        PointerMoveEvent moveEvent{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        ui.pointerMoveEvent({3.5f, 4.0f}, moveEvent);

        FocusEvent event{{}};
        ui.focusEvent(nodeFocusable, event);
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), nodeFocusable);
        CORRADE_COMPARE(ui.currentFocusedNode(), nodeFocusable);
        CORRADE_COMPARE(layer.style<StyleIndex>(nodeFocusableData), StyleIndex::GreenFocusedHover);
        CORRADE_COMPARE(layer.style(nodeFocusableDataDynamic), StyleCount + 0);

        ui.clearNodeFlags(nodeFocusable, NodeFlag::Focusable);
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeEnabledUpdate);

        ui.update();
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), nodeFocusable);
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE(layer.style<StyleIndex>(nodeFocusableData), StyleIndex::GreenHover);
        CORRADE_COMPARE(layer.style(nodeFocusableDataDynamic), StyleCount + 0);

    /* toInactiveOut transition in doUpdate() */
    } {
        ui.addNodeFlags(node, NodeFlag::NoEvents);
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeEnabledUpdate);

        ui.update();
        CORRADE_COMPARE(layer.style<StyleIndex>(nodeData), StyleIndex::Green);
        CORRADE_COMPARE(layer.style(nodeDataDynamic), StyleCount + 0);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(nodeData)]), StyleIndex::Green);
        CORRADE_COMPARE(layer.stateData().calculatedStyles[dataHandleId(nodeDataDynamic)], StyleCount + 0);

    /* toDisabled transition in doUpdate() */
    } {
        ui.addNodeFlags(node, NodeFlag::Disabled);
        CORRADE_COMPARE(ui.state(), UserInterfaceState::NeedsNodeEnabledUpdate);

        /* Only the calculated style changes, not the public one */
        ui.update();
        CORRADE_COMPARE(layer.style<StyleIndex>(nodeData), StyleIndex::Green);
        CORRADE_COMPARE(layer.style(nodeDataDynamic), StyleCount + 0);
        CORRADE_COMPARE(StyleIndex(layer.stateData().calculatedStyles[dataHandleId(nodeData)]), StyleIndex::GreenDisabled);
        CORRADE_COMPARE(layer.stateData().calculatedStyles[dataHandleId(nodeDataDynamic)], StyleCount + 0);
    }
}

/* Cannot use styleIndexTransitionToDisabled etc. in the test below because on
   debug builds the wrapper lambdas may have a different function pointer each
   time they're created */
UnsignedInt typeErasedTransition1(UnsignedInt style) {
    return style*2;
}
UnsignedInt typeErasedTransition2(UnsignedInt style) {
    return style*3;
}

void AbstractVisualLayerTest::sharedNeedsUpdateStatePropagatedToLayers() {
    struct LayerShared: AbstractVisualLayer::Shared {
        explicit LayerShared(UnsignedInt styleCount, UnsignedInt dynamicStyleCount): AbstractVisualLayer::Shared{styleCount, dynamicStyleCount} {}
    } shared{1, 0};

    struct Layer: AbstractVisualLayer {
        explicit Layer(LayerHandle handle, Shared& shared): AbstractVisualLayer{handle, shared} {}
    };

    /* Initially no state is set */
    Layer layer1{layerHandle(0, 1), shared};
    Layer layer2{layerHandle(0, 1), shared};
    Layer layer3{layerHandle(0, 1), shared};
    CORRADE_COMPARE(layer1.state(), LayerStates{});
    CORRADE_COMPARE(layer2.state(), LayerStates{});
    CORRADE_COMPARE(layer3.state(), LayerStates{});

    /* Setting a nullptr transition (i.e., the default) doesn't cause
       NeedsDataUpdate to be set */
    shared.setStyleTransition(
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr);
    CORRADE_COMPARE(layer1.state(), LayerStates{});
    CORRADE_COMPARE(layer2.state(), LayerStates{});
    CORRADE_COMPARE(layer3.state(), LayerStates{});

    /* Setting any other transition except toDisabled doesn't cause
       NeedsDataUpdate to be set either */
    shared.setStyleTransition(
        typeErasedTransition1,
        typeErasedTransition2,
        typeErasedTransition1,
        typeErasedTransition2,
        typeErasedTransition1,
        typeErasedTransition2,
        nullptr);
    CORRADE_COMPARE(layer1.state(), LayerStates{});
    CORRADE_COMPARE(layer2.state(), LayerStates{});
    CORRADE_COMPARE(layer3.state(), LayerStates{});

    /* Explicitly set a non-trivial state on some of the layers */
    layer1.setNeedsUpdate(LayerState::NeedsCommonDataUpdate);
    layer3.setNeedsUpdate(LayerState::NeedsSharedDataUpdate);

    /* Setting a toDisabled transition sets LayerState::NeedsDataUpdate on all
       layers */
    shared.setStyleTransition(
        nullptr,
        nullptr,
        nullptr,
        typeErasedTransition1);
    CORRADE_COMPARE(layer1.state(), LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate);
    CORRADE_COMPARE(layer2.state(), LayerState::NeedsDataUpdate);
    CORRADE_COMPARE(layer3.state(), LayerState::NeedsDataUpdate|LayerState::NeedsSharedDataUpdate);

    /* Updating one doesn't cause the flag to be reset on others */
    layer2.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer1.state(), LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate);
    CORRADE_COMPARE(layer2.state(), LayerStates{});
    CORRADE_COMPARE(layer3.state(), LayerState::NeedsDataUpdate|LayerState::NeedsSharedDataUpdate);

    /* Updating another still doesn't */
    layer1.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer1.state(), LayerState::NeedsCommonDataUpdate);
    CORRADE_COMPARE(layer2.state(), LayerStates{});
    CORRADE_COMPARE(layer3.state(), LayerState::NeedsDataUpdate|LayerState::NeedsSharedDataUpdate);

    /* Setting the same toDisabled transition doesn't cause NeedsDataUpdate to
       be set again */
    shared.setStyleTransition(
        nullptr,
        nullptr,
        nullptr,
        typeErasedTransition1);
    CORRADE_COMPARE(layer1.state(), LayerState::NeedsCommonDataUpdate);
    CORRADE_COMPARE(layer2.state(), LayerStates{});
    CORRADE_COMPARE(layer3.state(), LayerState::NeedsDataUpdate|LayerState::NeedsSharedDataUpdate);

    /* Setting a different one does. The third layer has the state still set
       from before, there it doesn't get reset back. */
    shared.setStyleTransition(
        nullptr,
        nullptr,
        nullptr,
        typeErasedTransition2);
    CORRADE_COMPARE(layer1.state(), LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate);
    CORRADE_COMPARE(layer2.state(), LayerState::NeedsDataUpdate);
    CORRADE_COMPARE(layer3.state(), LayerState::NeedsDataUpdate|LayerState::NeedsSharedDataUpdate);

    /* Creating a new layer with the shared state that had setStyleTransition()
       called a few times doesn't mark it as needing an update because there's
       no data that would need it yet and the layer should do all other
       shared-state-dependent setup during construction already */
    Layer layer4{layerHandle(0, 1), shared};
    CORRADE_COMPARE(layer4.state(), LayerStates{});

    /* But calling setStyleTransition() next time will */
    shared.setStyleTransition(
        nullptr,
        nullptr,
        nullptr,
        typeErasedTransition1);
    CORRADE_COMPARE(layer1.state(), LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate);
    CORRADE_COMPARE(layer2.state(), LayerState::NeedsDataUpdate);
    CORRADE_COMPARE(layer3.state(), LayerState::NeedsDataUpdate|LayerState::NeedsSharedDataUpdate);
    CORRADE_COMPARE(layer4.state(), LayerState::NeedsDataUpdate);
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::AbstractVisualLayerTest)
