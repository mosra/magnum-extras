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

#include <new>
#include <sstream> /** @todo remove once Debug is stream-free */
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/Container.h>
#include <Corrade/Utility/DebugStl.h> /** @todo remove once Debug is stream-free */

#include "Magnum/Whee/AbstractUserInterface.h"
#include "Magnum/Whee/BaseLayer.h"
#include "Magnum/Whee/Event.h"
#include "Magnum/Whee/Handle.h"
#include "Magnum/Whee/Implementation/baseLayerState.h" /* for updateDataOrder() */

namespace Magnum { namespace Whee { namespace Test { namespace {

struct BaseLayerTest: TestSuite::Tester {
    explicit BaseLayerTest();

    template<class T> void styleSizeAlignment();

    void styleCommonConstructDefault();
    void styleCommonConstructNoInit();
    void styleCommonSetters();

    void styleItemConstructDefault();
    void styleItemConstructNoInit();
    void styleItemSetters();

    void sharedConstruct();
    void sharedConstructNoCreate();
    void sharedConstructCopy();
    void sharedConstructMove();

    void construct();
    void constructCopy();
    void constructMove();

    template<class T> void create();
    template<class T> void setStyle();
    void setColor();
    void setOutlineWidth();

    void invalidHandle();
    void styleOutOfRange();

    void updateEmpty();
    void updateDataOrder();

    void eventStyleTransitionNoOp();
    void eventStyleTransition();
    void eventStyleTransitionNoHover();
    void eventStyleTransitionNoCapture();
    void eventStyleTransitionOutOfRange();
};

const struct {
    const char* name;
    bool emptyUpdate;
} UpdateDataOrderData[]{
    {"empty update", true},
    {"", false},
};

enum class Enum: UnsignedShort {};

Debug& operator<<(Debug& debug, Enum value) {
    return debug << UnsignedInt(value);
}

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
    NodeHandle node;
    LayerStates state;
} CreateData[]{
    {"create", NodeHandle::Null, LayerStates{}},
    {"create and attach", nodeHandle(9872, 0xbeb), LayerState::NeedsAttachmentUpdate}
};

const struct {
    const char* name;
    bool update;
} EventStyleTransitionData[]{
    {"update before", true},
    {"", false},
};

const struct {
    const char* name;
    bool disableCapture;
    StyleIndex blurStyle, hoverStyle;
} EventStyleTransitionNoCaptureData[]{
    {"", false, StyleIndex::GreenPressed, StyleIndex::GreenPressedHover},
    {"capture disabled", true, StyleIndex::Green, StyleIndex::GreenHover},
};

BaseLayerTest::BaseLayerTest() {
    addTests({&BaseLayerTest::styleSizeAlignment<BaseLayerStyleCommon>,
              &BaseLayerTest::styleSizeAlignment<BaseLayerStyleItem>,

              &BaseLayerTest::styleCommonConstructDefault,
              &BaseLayerTest::styleCommonConstructNoInit,
              &BaseLayerTest::styleCommonSetters,

              &BaseLayerTest::styleItemConstructDefault,
              &BaseLayerTest::styleItemConstructNoInit,
              &BaseLayerTest::styleItemSetters,

              &BaseLayerTest::sharedConstruct,
              &BaseLayerTest::sharedConstructNoCreate,
              &BaseLayerTest::sharedConstructCopy,
              &BaseLayerTest::sharedConstructMove,

              &BaseLayerTest::construct,
              &BaseLayerTest::constructCopy,
              &BaseLayerTest::constructMove});

    addInstancedTests<BaseLayerTest>({&BaseLayerTest::create<UnsignedInt>,
                                      &BaseLayerTest::create<Enum>},
        Containers::arraySize(CreateData));

    addTests({&BaseLayerTest::setStyle<UnsignedInt>,
              &BaseLayerTest::setStyle<Enum>,
              &BaseLayerTest::setColor,
              &BaseLayerTest::setOutlineWidth,

              &BaseLayerTest::invalidHandle,
              &BaseLayerTest::styleOutOfRange,

              &BaseLayerTest::updateEmpty});

    addInstancedTests({&BaseLayerTest::updateDataOrder},
        Containers::arraySize(UpdateDataOrderData));

    addTests({&BaseLayerTest::eventStyleTransitionNoOp});

    addInstancedTests({&BaseLayerTest::eventStyleTransition},
        Containers::arraySize(EventStyleTransitionData));

    addTests({&BaseLayerTest::eventStyleTransitionNoHover});

    addInstancedTests({&BaseLayerTest::eventStyleTransitionNoCapture},
        Containers::arraySize(EventStyleTransitionNoCaptureData));

    addTests({&BaseLayerTest::eventStyleTransitionOutOfRange});
}

using namespace Math::Literals;

template<class> struct StyleTraits;
template<> struct StyleTraits<BaseLayerStyleCommon> {
    static const char* name() { return "BaseLayerStyleCommon"; }
};
template<> struct StyleTraits<BaseLayerStyleItem> {
    static const char* name() { return "BaseLayerStyleItem"; }
};

template<class T> void BaseLayerTest::styleSizeAlignment() {
    setTestCaseTemplateName(StyleTraits<T>::name());

    CORRADE_FAIL_IF(sizeof(T) % sizeof(Vector4) != 0, sizeof(T) << "is not a multiple of vec4 for UBO alignment.");

    /* 48-byte structures are fine, we'll align them to 768 bytes and not
       256, but warn about that */
    CORRADE_FAIL_IF(768 % sizeof(T) != 0, sizeof(T) << "can't fit exactly into 768-byte UBO alignment.");
    if(256 % sizeof(T) != 0)
        CORRADE_WARN(sizeof(T) << "can't fit exactly into 256-byte UBO alignment, only 768.");

    CORRADE_COMPARE(alignof(T), 4);
}

void BaseLayerTest::styleCommonConstructDefault() {
    BaseLayerStyleCommon a;
    BaseLayerStyleCommon b{DefaultInit};
    CORRADE_COMPARE(a.smoothness, 0.0f);
    CORRADE_COMPARE(b.smoothness, 0.0f);
    CORRADE_COMPARE(a.innerOutlineSmoothness, 0.0f);
    CORRADE_COMPARE(b.innerOutlineSmoothness, 0.0f);

    constexpr BaseLayerStyleCommon ca;
    constexpr BaseLayerStyleCommon cb{DefaultInit};
    CORRADE_COMPARE(ca.smoothness, 0.0f);
    CORRADE_COMPARE(cb.smoothness, 0.0f);
    CORRADE_COMPARE(ca.innerOutlineSmoothness, 0.0f);
    CORRADE_COMPARE(cb.innerOutlineSmoothness, 0.0f);

    CORRADE_VERIFY(std::is_nothrow_default_constructible<BaseLayerStyleCommon>::value);
    CORRADE_VERIFY(std::is_nothrow_constructible<BaseLayerStyleCommon, DefaultInitT>::value);

    /* Implicit construction is not allowed */
    CORRADE_VERIFY(!std::is_convertible<DefaultInitT, BaseLayerStyleCommon>::value);
}

void BaseLayerTest::styleCommonConstructNoInit() {
    /* Testing only some fields, should be enough */
    BaseLayerStyleCommon a;
    a.smoothness = 3.0f;
    a.innerOutlineSmoothness = 20.0f;

    new(&a) BaseLayerStyleCommon{NoInit};
    {
        /* Explicitly check we're not on Clang because certain Clang-based IDEs
           inherit __GNUC__ if GCC is used instead of leaving it at 4 like
           Clang itself does */
        #if defined(CORRADE_TARGET_GCC) && !defined(CORRADE_TARGET_CLANG) && __GNUC__*100 + __GNUC_MINOR__ >= 601 && __OPTIMIZE__
        CORRADE_EXPECT_FAIL("GCC 6.1+ misoptimizes and overwrites the value.");
        #endif
        CORRADE_COMPARE(a.smoothness, 3.0f);
        CORRADE_COMPARE(a.innerOutlineSmoothness, 20.0f);
    }
}

void BaseLayerTest::styleCommonSetters() {
    BaseLayerStyleCommon a;
    a.setSmoothness(34.0f, 12.0f);
    CORRADE_COMPARE(a.smoothness, 34.0f);
    CORRADE_COMPARE(a.innerOutlineSmoothness, 12.0f);

    /* Convenience overload setting both smoothness values */
    a.setSmoothness(2.5f);
    CORRADE_COMPARE(a.smoothness, 2.5f);
    CORRADE_COMPARE(a.innerOutlineSmoothness, 2.5f);
}

void BaseLayerTest::styleItemConstructDefault() {
    BaseLayerStyleItem a;
    BaseLayerStyleItem b{DefaultInit};
    CORRADE_COMPARE(a.topColor, 0xffffffff_srgbaf);
    CORRADE_COMPARE(b.topColor, 0xffffffff_srgbaf);
    CORRADE_COMPARE(a.bottomColor, 0xffffffff_srgbaf);
    CORRADE_COMPARE(b.bottomColor, 0xffffffff_srgbaf);
    CORRADE_COMPARE(a.outlineColor, 0xffffffff_srgbaf);
    CORRADE_COMPARE(b.outlineColor, 0xffffffff_srgbaf);
    CORRADE_COMPARE(a.outlineWidth, Vector4{0.0f});
    CORRADE_COMPARE(b.outlineWidth, Vector4{0.0f});
    CORRADE_COMPARE(a.cornerRadius, Vector4{0.0f});
    CORRADE_COMPARE(b.cornerRadius, Vector4{0.0f});
    CORRADE_COMPARE(a.innerOutlineCornerRadius, Vector4{0.0f});
    CORRADE_COMPARE(b.innerOutlineCornerRadius, Vector4{0.0f});

    constexpr BaseLayerStyleItem ca;
    constexpr BaseLayerStyleItem cb{DefaultInit};
    CORRADE_COMPARE(ca.topColor, 0xffffffff_srgbaf);
    CORRADE_COMPARE(cb.topColor, 0xffffffff_srgbaf);
    CORRADE_COMPARE(ca.bottomColor, 0xffffffff_srgbaf);
    CORRADE_COMPARE(cb.bottomColor, 0xffffffff_srgbaf);
    CORRADE_COMPARE(ca.outlineColor, 0xffffffff_srgbaf);
    CORRADE_COMPARE(cb.outlineColor, 0xffffffff_srgbaf);
    CORRADE_COMPARE(ca.outlineWidth, Vector4{0.0f});
    CORRADE_COMPARE(cb.outlineWidth, Vector4{0.0f});
    CORRADE_COMPARE(ca.cornerRadius, Vector4{0.0f});
    CORRADE_COMPARE(cb.cornerRadius, Vector4{0.0f});
    CORRADE_COMPARE(ca.innerOutlineCornerRadius, Vector4{0.0f});
    CORRADE_COMPARE(cb.innerOutlineCornerRadius, Vector4{0.0f});

    CORRADE_VERIFY(std::is_nothrow_default_constructible<BaseLayerStyleItem>::value);
    CORRADE_VERIFY(std::is_nothrow_constructible<BaseLayerStyleItem, DefaultInitT>::value);

    /* Implicit construction is not allowed */
    CORRADE_VERIFY(!std::is_convertible<DefaultInitT, BaseLayerStyleItem>::value);
}

void BaseLayerTest::styleItemConstructNoInit() {
    /* Testing only some fields, should be enough */
    BaseLayerStyleItem a;
    a.bottomColor = 0xff3366_rgbf;
    a.innerOutlineCornerRadius = {1.0f, 2.0f, 3.0f, 4.0f};

    new(&a) BaseLayerStyleItem{NoInit};
    {
        /* Explicitly check we're not on Clang because certain Clang-based IDEs
           inherit __GNUC__ if GCC is used instead of leaving it at 4 like
           Clang itself does */
        #if defined(CORRADE_TARGET_GCC) && !defined(CORRADE_TARGET_CLANG) && __GNUC__*100 + __GNUC_MINOR__ >= 601 && __OPTIMIZE__
        CORRADE_EXPECT_FAIL("GCC 6.1+ misoptimizes and overwrites the value.");
        #endif
        CORRADE_COMPARE(a.bottomColor, 0xff3366_rgbf);
        CORRADE_COMPARE(a.innerOutlineCornerRadius, (Vector4{1.0f, 2.0f, 3.0f, 4.0f}));
    }
}

void BaseLayerTest::styleItemSetters() {
    BaseLayerStyleItem a;
    a.setColor(0xff336699_rgbaf, 0xaabbccdd_rgbaf)
     .setOutlineColor(0x663399cc_rgbaf)
     .setOutlineWidth({1.0f, 2.0f, 3.0f, 4.0f})
     .setCornerRadius({5.0f, 6.0f, 7.0f, 8.0f})
     .setInnerOutlineCornerRadius({0.1f, 0.2f, 0.3f, 0.4f});
    CORRADE_COMPARE(a.topColor, 0xff336699_rgbaf);
    CORRADE_COMPARE(a.bottomColor, 0xaabbccdd_rgbaf);
    CORRADE_COMPARE(a.outlineColor, 0x663399cc_rgbaf);
    CORRADE_COMPARE(a.outlineWidth, (Vector4{1.0f, 2.0f, 3.0f, 4.0f}));
    CORRADE_COMPARE(a.cornerRadius, (Vector4{5.0f, 6.0f, 7.0f, 8.0f}));
    CORRADE_COMPARE(a.innerOutlineCornerRadius, (Vector4{0.1f, 0.2f, 0.3f, 0.4f}));

    /* Convenience overloads setting both colors and all edges/corners to the
       same value */
    a.setColor(0x11223344_rgbaf)
     .setOutlineWidth(2.75f)
     .setCornerRadius(3.25f)
     .setInnerOutlineCornerRadius(5.5f);
    CORRADE_COMPARE(a.topColor, 0x11223344_rgbaf);
    CORRADE_COMPARE(a.bottomColor, 0x11223344_rgbaf);
    CORRADE_COMPARE(a.outlineWidth, Vector4{2.75f});
    CORRADE_COMPARE(a.cornerRadius, Vector4{3.25f});
    CORRADE_COMPARE(a.innerOutlineCornerRadius, Vector4{5.5f});
}

void BaseLayerTest::sharedConstruct() {
    struct Shared: BaseLayer::Shared {
        explicit Shared(UnsignedInt styleCount): BaseLayer::Shared{styleCount} {}
    } shared{3};
    CORRADE_COMPARE(shared.styleCount(), 3);
}

void BaseLayerTest::sharedConstructNoCreate() {
    struct Shared: BaseLayer::Shared {
        explicit Shared(NoCreateT): BaseLayer::Shared{NoCreate} {}
    } shared{NoCreate};

    /* Shouldn't crash */
    CORRADE_VERIFY(true);

    /* Implicit construction is not allowed */
    CORRADE_VERIFY(!std::is_convertible<NoCreateT, BaseLayer::Shared>::value);
}

void BaseLayerTest::sharedConstructCopy() {
    CORRADE_VERIFY(!std::is_copy_constructible<BaseLayer::Shared>{});
    CORRADE_VERIFY(!std::is_copy_assignable<BaseLayer::Shared>{});
}

void BaseLayerTest::sharedConstructMove() {
    struct Shared: BaseLayer::Shared {
        explicit Shared(UnsignedInt styleCount): BaseLayer::Shared{styleCount} {}
    };

    Shared a{3};

    Shared b{Utility::move(a)};
    CORRADE_COMPARE(b.styleCount(), 3);

    Shared c{5};
    c = Utility::move(b);
    CORRADE_COMPARE(c.styleCount(), 3);

    CORRADE_VERIFY(std::is_nothrow_move_constructible<BaseLayer::Shared>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<BaseLayer::Shared>::value);
}

void BaseLayerTest::construct() {
    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(UnsignedInt styleCount): BaseLayer::Shared{styleCount} {}
    } shared{3};

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    } layer{layerHandle(137, 0xfe), shared};

    /* There isn't anything to query on the BaseLayer itself */
    CORRADE_COMPARE(layer.handle(), layerHandle(137, 0xfe));
}

void BaseLayerTest::constructCopy() {
    CORRADE_VERIFY(!std::is_copy_constructible<BaseLayer>{});
    CORRADE_VERIFY(!std::is_copy_assignable<BaseLayer>{});
}

void BaseLayerTest::constructMove() {
    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(UnsignedInt styleCount): BaseLayer::Shared{styleCount} {}
    };

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    };

    LayerShared shared{3};
    LayerShared shared2{5};

    Layer a{layerHandle(137, 0xfe), shared};

    Layer b{Utility::move(a)};
    /* There isn't anything to query on the BaseLayer itself */
    CORRADE_COMPARE(b.handle(), layerHandle(137, 0xfe));

    Layer c{layerHandle(0, 2), shared2};
    c = Utility::move(b);
    CORRADE_COMPARE(c.handle(), layerHandle(137, 0xfe));

    CORRADE_VERIFY(std::is_nothrow_move_constructible<BaseLayer>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<BaseLayer>::value);
}

template<class T> void BaseLayerTest::create() {
    auto&& data = CreateData[testCaseInstanceId()];
    setTestCaseDescription(data.name);
    setTestCaseTemplateName(std::is_same<T, Enum>::value ? "Enum" : "UnsignedInt");

    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(UnsignedInt styleCount): BaseLayer::Shared{styleCount} {}
    } shared{38};

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    /* Just to be sure the getters aren't picking up the first ever data
       always */
    layer.create(2);

    /* Default color and outline width */
    {
        DataHandle layerData = layer.create(T(17), data.node);
        CORRADE_COMPARE(layer.node(layerData), data.node);
        CORRADE_COMPARE(layer.style(layerData), 17);
        CORRADE_COMPARE(layer.color(layerData), 0xffffff_rgbf);
        CORRADE_COMPARE(layer.outlineWidth(layerData), Vector4{0.0f});
        CORRADE_COMPARE(layer.state(), data.state);

    /* Default outline width */
    } {
        DataHandle layerData = layer.create(T(23), 0xff3366_rgbf, data.node);
        CORRADE_COMPARE(layer.node(layerData), data.node);
        CORRADE_COMPARE(layer.style(layerData), 23);
        CORRADE_COMPARE(layer.color(layerData), 0xff3366_rgbf);
        CORRADE_COMPARE(layer.outlineWidth(layerData), Vector4{0.0f});
        CORRADE_COMPARE(layer.state(), data.state);

    /* Single-value outline width */
    } {
        DataHandle layerData = layer.create(T(19), 0xff3366_rgbf, 4.0f, data.node);
        CORRADE_COMPARE(layer.node(layerData), data.node);
        CORRADE_COMPARE(layer.style(layerData), 19);
        CORRADE_COMPARE(layer.color(layerData), 0xff3366_rgbf);
        CORRADE_COMPARE(layer.outlineWidth(layerData), Vector4{4.0f});
        CORRADE_COMPARE(layer.state(), data.state);

    /* Everything explicit, testing also the getter overloads and templates */
    } {
        DataHandle layerData = layer.create(T(37), 0xff3366_rgbf, {3.0f, 2.0f, 1.0f, 4.0f}, data.node);
        CORRADE_COMPARE(layer.node(layerData), data.node);
        CORRADE_COMPARE(layer.style(layerData), 37);
        /* Can't use T, as the function restricts to enum types which would
           fail for T == UnsignedInt */
        CORRADE_COMPARE(layer.template style<Enum>(layerData), Enum(37));
        CORRADE_COMPARE(layer.style(dataHandleData(layerData)), 37);
        /* Can't use T, as the function restricts to enum types which would
           fail for T == UnsignedInt */
        CORRADE_COMPARE(layer.template style<Enum>(dataHandleData(layerData)), Enum(37));
        CORRADE_COMPARE(layer.color(layerData), 0xff3366_rgbf);
        CORRADE_COMPARE(layer.color(dataHandleData(layerData)), 0xff3366_rgbf);
        CORRADE_COMPARE(layer.outlineWidth(layerData), (Vector4{3.0f, 2.0f, 1.0f, 4.0f}));
        CORRADE_COMPARE(layer.outlineWidth(dataHandleData(layerData)), (Vector4{3.0f, 2.0f, 1.0f, 4.0f}));
        CORRADE_COMPARE(layer.state(), data.state);
    }
}

template<class T> void BaseLayerTest::setStyle() {
    setTestCaseTemplateName(std::is_same<T, Enum>::value ? "Enum" : "UnsignedInt");

    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(UnsignedInt styleCount): BaseLayer::Shared{styleCount} {}
    } shared{67};

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

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

void BaseLayerTest::setColor() {
    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(UnsignedInt styleCount): BaseLayer::Shared{styleCount} {}
    } shared{3};

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    /* Just to be sure the setters aren't picking up the first ever data
       always */
    layer.create(1);

    DataHandle data = layer.create(2, 0xff3366_rgbf);
    CORRADE_COMPARE(layer.color(data), 0xff3366_rgbf);
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Setting a color marks the layer as dirty */
    layer.setColor(data, 0xaabbc_rgbf);
    CORRADE_COMPARE(layer.color(data), 0xaabbc_rgbf);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsUpdate);

    /* Testing also the other overload */
    layer.setColor(dataHandleData(data), 0x112233_rgbf);
    CORRADE_COMPARE(layer.color(data), 0x112233_rgbf);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsUpdate);
}

void BaseLayerTest::setOutlineWidth() {
    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(UnsignedInt styleCount): BaseLayer::Shared{styleCount} {}
    } shared{3};

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    /* Just to be sure the setters aren't picking up the first ever data
       always */
    layer.create(2);

    DataHandle data = layer.create(1, 0xff3366_rgbf, {3.0f, 1.0f, 2.0f, 4.0f});
    CORRADE_COMPARE(layer.outlineWidth(data), (Vector4{3.0f, 1.0f, 2.0f, 4.0f}));
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Setting an outline width marks the layer as dirty */
    layer.setOutlineWidth(data, {2.0f, 4.0f, 3.0f, 1.0f});
    CORRADE_COMPARE(layer.outlineWidth(data), (Vector4{2.0f, 4.0f, 3.0f, 1.0f}));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsUpdate);

    /* Testing also the other overload */
    layer.setOutlineWidth(dataHandleData(data), {1.0f, 2.0f, 3.0f, 4.0f});
    CORRADE_COMPARE(layer.outlineWidth(data), (Vector4{1.0f, 2.0f, 3.0f, 4.0f}));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsUpdate);

    /* Single-value width */
    layer.setOutlineWidth(data, 4.0f);
    CORRADE_COMPARE(layer.outlineWidth(data), Vector4{4.0f});
    CORRADE_COMPARE(layer.state(), LayerState::NeedsUpdate);

    /* Testing also the other overload */
    layer.setOutlineWidth(dataHandleData(data), 3.0f);
    CORRADE_COMPARE(layer.outlineWidth(data), Vector4{3.0f});
    CORRADE_COMPARE(layer.state(), LayerState::NeedsUpdate);
}

void BaseLayerTest::invalidHandle() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(UnsignedInt styleCount): BaseLayer::Shared{styleCount} {}
    } shared{1};

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    std::ostringstream out;
    Error redirectError{&out};
    layer.style(DataHandle::Null);
    layer.style(LayerDataHandle::Null);
    layer.setStyle(DataHandle::Null, 0);
    layer.setStyle(LayerDataHandle::Null, 0);
    layer.color(DataHandle::Null);
    layer.color(LayerDataHandle::Null);
    layer.setColor(DataHandle::Null, {});
    layer.setColor(LayerDataHandle::Null, {});
    layer.outlineWidth(DataHandle::Null);
    layer.outlineWidth(LayerDataHandle::Null);
    layer.setOutlineWidth(DataHandle::Null, {});
    layer.setOutlineWidth(LayerDataHandle::Null, {});
    CORRADE_COMPARE(out.str(),
        "Whee::BaseLayer::style(): invalid handle Whee::DataHandle::Null\n"
        "Whee::BaseLayer::style(): invalid handle Whee::LayerDataHandle::Null\n"
        "Whee::BaseLayer::setStyle(): invalid handle Whee::DataHandle::Null\n"
        "Whee::BaseLayer::setStyle(): invalid handle Whee::LayerDataHandle::Null\n"
        "Whee::BaseLayer::color(): invalid handle Whee::DataHandle::Null\n"
        "Whee::BaseLayer::color(): invalid handle Whee::LayerDataHandle::Null\n"
        "Whee::BaseLayer::setColor(): invalid handle Whee::DataHandle::Null\n"
        "Whee::BaseLayer::setColor(): invalid handle Whee::LayerDataHandle::Null\n"
        "Whee::BaseLayer::outlineWidth(): invalid handle Whee::DataHandle::Null\n"
        "Whee::BaseLayer::outlineWidth(): invalid handle Whee::LayerDataHandle::Null\n"
        "Whee::BaseLayer::setOutlineWidth(): invalid handle Whee::DataHandle::Null\n"
        "Whee::BaseLayer::setOutlineWidth(): invalid handle Whee::LayerDataHandle::Null\n");
}

void BaseLayerTest::styleOutOfRange() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(UnsignedInt styleCount): BaseLayer::Shared{styleCount} {}
    } shared{3};

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    DataHandle data = layer.create(2);

    std::ostringstream out;
    Error redirectError{&out};
    layer.create(3);
    layer.setStyle(data, 3);
    layer.setStyle(dataHandleData(data), 3);
    CORRADE_COMPARE(out.str(),
        "Whee::BaseLayer::create(): style 3 out of range for 3 styles\n"
        "Whee::BaseLayer::setStyle(): style 3 out of range for 3 styles\n"
        "Whee::BaseLayer::setStyle(): style 3 out of range for 3 styles\n");
}

void BaseLayerTest::updateEmpty() {
    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(UnsignedInt styleCount): BaseLayer::Shared{styleCount} {}
    } shared{3};

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0, 1), shared};

    /* Shouldn't crash or do anything weird */
    layer.update({}, {}, {}, {}, {}, {}, {});
    CORRADE_VERIFY(true);
}

void BaseLayerTest::updateDataOrder() {
    auto&& data = UpdateDataOrderData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Does just extremely basic verification that the vertex and index data
       get filled with correct contents and in correct order. The actual visual
       output is checked in BaseLayerGLTest. */

    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(UnsignedInt styleCount): BaseLayer::Shared{styleCount} {}
    } shared{4};

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
        const BaseLayer::State& stateData() const { return *_state; }
    } layer{layerHandle(0, 1), shared};

    /* Two node handles to attach the data to */
    NodeHandle node6 = nodeHandle(6, 0);
    NodeHandle node15 = nodeHandle(15, 0);

    /* Create 10 data handles. Only three get filled and actually used. */
    layer.create(0);                                                    /* 0 */
    layer.create(0);                                                    /* 1 */
    layer.create(0);                                                    /* 2 */
    layer.create(2, 0xff3366_rgbf, {1.0f, 2.0f, 3.0f, 4.0f}, node6);    /* 3 */
    layer.create(0);                                                    /* 4 */
    layer.create(0);                                                    /* 5 */
    layer.create(0);                                                    /* 6 */
    layer.create(1, 0x112233_rgbf, Vector4{2.0f}, node15);              /* 7 */
    layer.create(0);                                                    /* 8 */
    layer.create(3, 0x663399_rgbf, {3.0f, 2.0f, 1.0f, 4.0f}, node15);   /* 9 */

    Vector2 nodeOffsets[16];
    Vector2 nodeSizes[16];
    nodeOffsets[6] = {1.0f, 2.0f};
    nodeSizes[6] = {10.0f, 15.0f};
    nodeOffsets[15] = {3.0f, 4.0f};
    nodeSizes[15] = {20.0f, 5.0f};

    /* An empty update should generate an empty draw list */
    if(data.emptyUpdate) {
        layer.update({}, {}, {}, nodeOffsets, nodeSizes, {}, {});
        CORRADE_COMPARE_AS(layer.stateData().indices,
            Containers::ArrayView<const UnsignedInt>{},
            TestSuite::Compare::Container);
        return;
    }

    /* Just the filled subset is getting updated */
    UnsignedInt dataIds[]{9, 7, 3};
    layer.update(dataIds, {}, {}, nodeOffsets, nodeSizes, {}, {});

    /* The indices should be filled just for the three items */
    CORRADE_COMPARE_AS(layer.stateData().indices, Containers::arrayView<UnsignedInt>({
        9*4 + 0, 9*4 + 2, 9*4 + 1, 9*4 + 2, 9*4 + 3, 9*4 + 1, /* quad 9 */
        7*4 + 0, 7*4 + 2, 7*4 + 1, 7*4 + 2, 7*4 + 3, 7*4 + 1, /* quad 7 */
        3*4 + 0, 3*4 + 2, 3*4 + 1, 3*4 + 2, 3*4 + 3, 3*4 + 1, /* quad 3 */
    }), TestSuite::Compare::Container);

    /* The vertices are there for all data, but only the actually used are
       filled */
    CORRADE_COMPARE(layer.stateData().vertices.size(), 10*4);
    for(std::size_t i = 0; i != 4; ++i) {
        CORRADE_ITERATION(i);
        CORRADE_COMPARE(layer.stateData().vertices[3*4 + i].color, 0xff3366_rgbf);
        CORRADE_COMPARE(layer.stateData().vertices[3*4 + i].outlineWidth, (Vector4{1.0f, 2.0f, 3.0f, 4.0f}));
        CORRADE_COMPARE(layer.stateData().vertices[3*4 + i].style, 2);

        CORRADE_COMPARE(layer.stateData().vertices[7*4 + i].color, 0x112233_rgbf);
        CORRADE_COMPARE(layer.stateData().vertices[7*4 + i].outlineWidth, Vector4{2.0f});
        CORRADE_COMPARE(layer.stateData().vertices[7*4 + i].style, 1);

        CORRADE_COMPARE(layer.stateData().vertices[9*4 + i].color, 0x663399_rgbf);
        CORRADE_COMPARE(layer.stateData().vertices[9*4 + i].outlineWidth, (Vector4{3.0f, 2.0f, 1.0f, 4.0f}));
        CORRADE_COMPARE(layer.stateData().vertices[9*4 + i].style, 3);
    }

    Containers::StridedArrayView1D<const Vector2> positions = stridedArrayView(layer.stateData().vertices).slice(&Implementation::BaseLayerVertex::position);
    Containers::StridedArrayView1D<const Vector2> centerDistances = stridedArrayView(layer.stateData().vertices).slice(&Implementation::BaseLayerVertex::centerDistance);

    /* Data 3 is attached to node 6 */
    CORRADE_COMPARE_AS(positions.sliceSize(3*4, 4), Containers::arrayView<Vector2>({
        {1.0f, 2.0f},
        {11.0f, 2.0f},
        {1.0f, 17.0f},
        {11.0f, 17.0f},
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(centerDistances.sliceSize(3*4, 4), Containers::arrayView<Vector2>({
        {-5.0f, -7.5f},
        { 5.0f, -7.5f},
        {-5.0f,  7.5f},
        { 5.0f,  7.5f},
    }), TestSuite::Compare::Container);

    /* Data 7 and 9 are both attached to node 15 */
    for(std::size_t i: {7, 9}) {
        CORRADE_COMPARE_AS(positions.sliceSize(i*4, 4), Containers::arrayView<Vector2>({
            {3.0f, 4.0f},
            {23.0f, 4.0f},
            {3.0f, 9.0f},
            {23.0f, 9.0f},
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(centerDistances.sliceSize(i*4, 4), Containers::arrayView<Vector2>({
            {-10.0f, -2.5f},
            { 10.0f, -2.5f},
            {-10.0f,  2.5f},
            { 10.0f,  2.5f},
        }), TestSuite::Compare::Container);
    }
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

void BaseLayerTest::eventStyleTransitionNoOp() {
    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(UnsignedInt styleCount): BaseLayer::Shared{styleCount} {}
    } shared{4};

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    };

    AbstractUserInterface ui{{100, 100}};

    NodeHandle node = ui.createNode({1.0f, 1.0f}, {2.0f, 2.0f});

    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), shared));
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

void BaseLayerTest::eventStyleTransition() {
    auto&& data = EventStyleTransitionData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(UnsignedInt styleCount): BaseLayer::Shared{styleCount} {}
    } shared{12};
    shared.setStyleTransition<StyleIndex,
        styleIndexTransitionToPressedBlur,
        styleIndexTransitionToPressedHover,
        styleIndexTransitionToInactiveBlur,
        styleIndexTransitionToInactiveHover>();

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    };

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

    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), shared));
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

void BaseLayerTest::eventStyleTransitionNoHover() {
    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(UnsignedInt styleCount): BaseLayer::Shared{styleCount} {}
        State& state() { return *_state; }
    } shared{4};

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    };

    AbstractUserInterface ui{{100, 100}};

    NodeHandle node = ui.createNode({1.0f, 1.0f}, {2.0f, 2.0f});

    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), shared));
    DataHandle data = layer.create(StyleIndex::Green, node);

    ui.update();
    CORRADE_COMPARE(layer.state(), LayerStates{});

    shared.setStyleTransition<StyleIndex,
        styleIndexTransitionToPressedBlur,
        styleIndexTransitionToInactiveBlur>();

    auto testPressRelease = [&]{
        {
            PointerEvent event{Pointer::MouseLeft};
            CORRADE_VERIFY(ui.pointerPressEvent({2.5f, 2.0f}, event));
            CORRADE_COMPARE(layer.style<StyleIndex>(data), StyleIndex::GreenPressed);
            CORRADE_COMPARE(layer.state(), LayerState::NeedsUpdate);
        }

        ui.update();
        CORRADE_COMPARE(layer.state(), LayerStates{});

        {
            PointerEvent event{Pointer::MouseLeft};
            CORRADE_VERIFY(ui.pointerReleaseEvent({2.5f, 2.5f}, event));
            CORRADE_COMPARE(layer.style<StyleIndex>(data), StyleIndex::Green);
            CORRADE_COMPARE(layer.state(), LayerState::NeedsUpdate);
        }

        ui.update();
        CORRADE_COMPARE(layer.state(), LayerStates{});
    };

    /* Test press & release without a hover */
    testPressRelease();

    /* Moving onto the node should do nothing */
    {
        PointerMoveEvent event{{}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({2.0f, 2.0f}, event));
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), node);
        CORRADE_COMPARE(layer.style<StyleIndex>(data), StyleIndex::Green);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }

    /* Press & release with a hover should behave the same as without */
    testPressRelease();

    /* Moving away should do nothing again */
    {
        PointerMoveEvent event{{}, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({2.0f, 2.0f}, event));
        CORRADE_COMPARE(ui.pointerEventHoveredNode(), node);
        CORRADE_COMPARE(layer.style<StyleIndex>(data), StyleIndex::Green);
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }
}

void BaseLayerTest::eventStyleTransitionNoCapture() {
    auto&& data = EventStyleTransitionNoCaptureData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(UnsignedInt styleCount): BaseLayer::Shared{styleCount} {}
    } shared{4};
    shared.setStyleTransition<StyleIndex,
        styleIndexTransitionToPressedBlur,
        styleIndexTransitionToPressedHover,
        styleIndexTransitionToInactiveBlur,
        styleIndexTransitionToInactiveHover>();

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    };

    struct EventLayer: AbstractLayer {
        explicit EventLayer(LayerHandle handle, bool disableCapture): AbstractLayer{handle}, disableCapture{disableCapture} {}

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

    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), shared));
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

void BaseLayerTest::eventStyleTransitionOutOfRange() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(UnsignedInt styleCount): BaseLayer::Shared{styleCount} {}
        State& state() { return *_state; }
    } shared{12};

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    };

    AbstractUserInterface ui{{100, 100}};

    NodeHandle node = ui.createNode({1.0f, 1.0f}, {2.0f, 2.0f});

    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), shared));
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
        CORRADE_COMPARE(out.str(), "Whee::BaseLayer::pointerPressEvent(): style transition from 4 to 12 out of range for 12 styles\n");
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
        CORRADE_COMPARE(out.str(), "Whee::BaseLayer::pointerPressEvent(): style transition from 5 to 12 out of range for 12 styles\n");
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
        CORRADE_COMPARE(out.str(), "Whee::BaseLayer::pointerReleaseEvent(): style transition from 5 to 12 out of range for 12 styles\n");
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
        CORRADE_COMPARE(out.str(), "Whee::BaseLayer::pointerLeaveEvent(): style transition from 5 to 12 out of range for 12 styles\n");
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
        CORRADE_COMPARE(out.str(), "Whee::BaseLayer::pointerEnterEvent(): style transition from 5 to 12 out of range for 12 styles\n");
    }
}

}}}}

CORRADE_TEST_MAIN(Magnum::Whee::Test::BaseLayerTest)
