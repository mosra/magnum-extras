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
#include <Corrade/Containers/BitArrayView.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Containers/StringStl.h> /** @todo remove once Debug is stream-free */
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/Container.h>
#include <Corrade/TestSuite/Compare/String.h>
#include <Corrade/Utility/DebugStl.h> /** @todo remove once Debug is stream-free */

#include "Magnum/Whee/AbstractUserInterface.h"
#include "Magnum/Whee/BaseLayer.h"
#include "Magnum/Whee/Event.h"
#include "Magnum/Whee/Handle.h"
/* for dynamicStyle(), updateDataOrder() */
#include "Magnum/Whee/Implementation/baseLayerState.h"

namespace Magnum { namespace Whee { namespace Test { namespace {

struct BaseLayerTest: TestSuite::Tester {
    explicit BaseLayerTest();

    template<class T> void styleUniformSizeAlignment();

    void styleUniformCommonConstructDefault();
    void styleUniformCommonConstruct();
    void styleUniformCommonConstructNoBlurParameters();
    void styleUniformCommonConstructNoBlurParametersSingleSmoothness();
    void styleUniformCommonConstructNoInit();
    void styleUniformCommonSetters();

    void styleUniformConstructDefault();
    void styleUniformConstruct();
    void styleUniformConstructSingleRadiusWidth();
    void styleUniformConstructNoOutline();
    void styleUniformConstructNoOutlineSingleRadius();
    void styleUniformConstructNoGradient();
    void styleUniformConstructNoGradientSingleRadiusWidth();
    void styleUniformConstructNoGradientNoOutline();
    void styleUniformConstructNoGradientNoOutlineSingleRadius();
    void styleUniformConstructNoInit();
    void styleUniformSetters();

    void sharedDebugFlag();
    void sharedDebugFlags();

    void sharedConfigurationConstruct();
    void sharedConfigurationConstructSameStyleUniformCount();
    void sharedConfigurationConstructZeroStyleOrUniformCount();
    void sharedConfigurationConstructCopy();

    void sharedConfigurationSetters();
    void sharedConfigurationSettersInvalid();

    void sharedConstruct();
    void sharedConstructNoCreate();
    void sharedConstructCopy();
    void sharedConstructMove();
    void sharedConstructZeroStyleCount();

    void sharedSetStyle();
    void sharedSetStyleImplicitPadding();
    void sharedSetStyleInvalidSize();
    void sharedSetStyleImplicitMapping();
    void sharedSetStyleImplicitMappingImplicitPadding();
    void sharedSetStyleImplicitMappingInvalidSize();

    void construct();
    void constructCopy();
    void constructMove();

    void backgroundBlurPassCount();
    void backgroundBlurPassCountInvalid();

    void dynamicStyle();
    void dynamicStyleNoDynamicStyles();
    void dynamicStyleInvalid();

    template<class T> void createRemove();
    void createRemoveHandleRecycle();

    void setColor();
    void setOutlineWidth();
    void setPadding();
    void setTextureCoordinates();
    void setTextureCoordinatesInvalid();

    void invalidHandle();
    void styleOutOfRange();

    /* setAnimator() and advanceAnimations() tested in
       BaseLayerStyleAnimatorTest */

    void updateEmpty();
    void updateDataOrder();
    void updateNoStyleSet();

    void sharedNeedsUpdateStatePropagatedToLayers();
};

const struct {
    const char* name;
    UnsignedInt dynamicStyleCount;
} SharedSetStyleData[]{
    {"", 0},
    {"dynamic styles", 17}
};

const struct {
    const char* name;
    Vector4 padding1, padding2;
    LayerStates expectedStates;
} DynamicStyleData[]{
    {"default padding",
        {}, {},
        LayerState::NeedsCommonDataUpdate},
    {"non-zero padding",
        {3.5f, 0.5f, 1.5f, 2.5f}, Vector4{2.0f},
        LayerState::NeedsCommonDataUpdate|LayerState::NeedsDataUpdate},
};

const struct {
    const char* name;
    bool emptyUpdate, textured;
    UnsignedInt styleCount, dynamicStyleCount;
    Vector2 node6Offset, node6Size;
    Vector4 paddingFromStyle;
    Vector4 paddingFromData;
    LayerStates states;
    bool expectIndexDataUpdated, expectVertexDataUpdated;
} UpdateDataOrderData[]{
    {"empty update", true, false, 5, 0,
        {}, {}, {}, {},
        LayerState::NeedsDataUpdate, true, true},
    {"empty update, textured", true, true, 5, 0,
        {}, {}, {}, {},
        LayerState::NeedsDataUpdate, true, true},
    {"", false, false, 5, 0,
        {1.0f, 2.0f}, {10.0f, 15.0f}, {}, {},
        LayerState::NeedsDataUpdate, true, true},
    {"textured", false, true, 5, 0,
        {1.0f, 2.0f}, {10.0f, 15.0f}, {}, {},
        LayerState::NeedsDataUpdate, true, true},
    {"node offset/size update only", false, false, 5, 0,
        {1.0f, 2.0f}, {10.0f, 15.0f}, {}, {},
        LayerState::NeedsNodeOffsetSizeUpdate, false, true},
    {"node order update only", false, false, 5, 0,
        {1.0f, 2.0f}, {10.0f, 15.0f}, {}, {},
        LayerState::NeedsNodeOrderUpdate, true, false},
    {"node enabled update only", false, false, 5, 0,
        {1.0f, 2.0f}, {10.0f, 15.0f}, {}, {},
        LayerState::NeedsNodeEnabledUpdate, false, true},
    /* These two shouldn't cause anything to be done in update(), and also no
       crashes */
    {"shared data update only", false, false, 5, 0,
        {1.0f, 2.0f}, {10.0f, 15.0f}, {}, {},
        LayerState::NeedsSharedDataUpdate, false, false},
    {"common data update only", false, false, 5, 0,
        {1.0f, 2.0f}, {10.0f, 15.0f}, {}, {},
        LayerState::NeedsCommonDataUpdate, false, false},
    /* This would cause an update of the dynamic style data in derived classes
       if appropriate internal flags would be set internally, but in the base
       class it's nothing */
    {"common data update only, dynamic styles", false, false, 2, 3,
        {1.0f, 2.0f}, {10.0f, 15.0f}, {}, {},
        LayerState::NeedsCommonDataUpdate, false, false},
    {"padding from style", false, false, 5, 0,
        {-1.0f, 1.5f}, {13.0f, 17.0f},
        {2.0f, 0.5f, 1.0f, 1.5f}, {},
        LayerState::NeedsDataUpdate, true, true},
    {"padding from data", false, false, 5, 0,
        {-1.0f, 1.5f}, {13.0f, 17.0f},
        {}, {2.0f, 0.5f, 1.0f, 1.5f},
        LayerState::NeedsDataUpdate, true, true},
    {"padding from both style and data", false, false, 5, 0,
        {-1.0f, 1.5f}, {13.0f, 17.0f},
        {0.5f, 0.0f, 1.0f, 0.75f}, {1.5f, 0.5f, 0.0f, 0.75f},
        LayerState::NeedsDataUpdate, true, true},
    {"unused dynamic styles", false, false, 5, 17,
        {1.0f, 2.0f}, {10.0f, 15.0f}, {}, {},
        LayerState::NeedsDataUpdate, true, true},
    {"dynamic styles", false, false, 2, 3,
        {1.0f, 2.0f}, {10.0f, 15.0f}, {}, {},
        LayerState::NeedsDataUpdate, true, true},
    {"dynamic styles, padding from dynamic style", false, false, 2, 3,
        {-1.0f, 1.5f}, {13.0f, 17.0f},
        {2.0f, 0.5f, 1.0f, 1.5f}, {},
        LayerState::NeedsDataUpdate, true, true},
    {"dynamic styles, padding from both dynamic style and data", false, false, 2, 3,
        {-1.0f, 1.5f}, {13.0f, 17.0f},
        {0.5f, 0.0f, 1.0f, 0.75f}, {1.5f, 0.5f, 0.0f, 0.75f},
        LayerState::NeedsDataUpdate, true, true},
};

enum class Enum: UnsignedShort {};

Debug& operator<<(Debug& debug, Enum value) {
    return debug << UnsignedInt(value);
}

const struct {
    const char* name;
    NodeHandle node;
    LayerStates state;
    bool layerDataHandleOverloads;
    UnsignedInt styleCount, dynamicStyleCount;
} CreateRemoveData[]{
    {"create",
        NodeHandle::Null, LayerState::NeedsDataUpdate, false, 38, 0},
    {"create and attach",
        nodeHandle(9872, 0xbeb), LayerState::NeedsNodeOffsetSizeUpdate|LayerState::NeedsAttachmentUpdate|LayerState::NeedsDataUpdate, false, 38, 0},
    {"LayerDataHandle overloads",
        NodeHandle::Null, LayerState::NeedsDataUpdate, true, 38, 0},
    {"dynamic styles",
        /* The lowest style index is 17 in this case, so all are dynamic */
        NodeHandle::Null, LayerState::NeedsDataUpdate, false, 7, 31},
};

const struct {
    const char* name;
    UnsignedInt styleCount, dynamicStyleCount;
} StyleOutOfRangeData[]{
    {"", 3, 0},
    {"dynamic styles", 1, 2},
};

const struct {
    const char* name;
    UnsignedInt dynamicStyleCount;
} UpdateNoStyleSetData[]{
    {"", 0},
    {"dynamic styles", 5}
};

const struct {
    const char* name;
    UnsignedInt dynamicStyleCount;
    LayerStates extraState;
} SharedNeedsUpdateStatePropagatedToLayersData[]{
    {"", 0, {}},
    {"dynamic styles", 5, LayerState::NeedsCommonDataUpdate}
};

BaseLayerTest::BaseLayerTest() {
    addTests({&BaseLayerTest::styleUniformSizeAlignment<BaseLayerCommonStyleUniform>,
              &BaseLayerTest::styleUniformSizeAlignment<BaseLayerStyleUniform>,

              &BaseLayerTest::styleUniformCommonConstructDefault,
              &BaseLayerTest::styleUniformCommonConstruct,
              &BaseLayerTest::styleUniformCommonConstructNoBlurParameters,
              &BaseLayerTest::styleUniformCommonConstructNoBlurParametersSingleSmoothness,
              &BaseLayerTest::styleUniformCommonConstructNoInit,
              &BaseLayerTest::styleUniformCommonSetters,

              &BaseLayerTest::styleUniformConstructDefault,
              &BaseLayerTest::styleUniformConstruct,
              &BaseLayerTest::styleUniformConstructSingleRadiusWidth,
              &BaseLayerTest::styleUniformConstructNoOutline,
              &BaseLayerTest::styleUniformConstructNoOutlineSingleRadius,
              &BaseLayerTest::styleUniformConstructNoGradient,
              &BaseLayerTest::styleUniformConstructNoGradientSingleRadiusWidth,
              &BaseLayerTest::styleUniformConstructNoGradientNoOutline,
              &BaseLayerTest::styleUniformConstructNoGradientNoOutlineSingleRadius,
              &BaseLayerTest::styleUniformConstructNoInit,
              &BaseLayerTest::styleUniformSetters,

              &BaseLayerTest::sharedDebugFlag,
              &BaseLayerTest::sharedDebugFlags,

              &BaseLayerTest::sharedConfigurationConstruct,
              &BaseLayerTest::sharedConfigurationConstructSameStyleUniformCount,
              &BaseLayerTest::sharedConfigurationConstructZeroStyleOrUniformCount,
              &BaseLayerTest::sharedConfigurationConstructCopy,

              &BaseLayerTest::sharedConfigurationSetters,
              &BaseLayerTest::sharedConfigurationSettersInvalid,

              &BaseLayerTest::sharedConstruct,
              &BaseLayerTest::sharedConstructNoCreate,
              &BaseLayerTest::sharedConstructCopy,
              &BaseLayerTest::sharedConstructMove,
              &BaseLayerTest::sharedConstructZeroStyleCount});

    addInstancedTests({&BaseLayerTest::sharedSetStyle,
                       &BaseLayerTest::sharedSetStyleImplicitPadding,
                       &BaseLayerTest::sharedSetStyleInvalidSize,
                       &BaseLayerTest::sharedSetStyleImplicitMapping,
                       &BaseLayerTest::sharedSetStyleImplicitMappingImplicitPadding,
                       &BaseLayerTest::sharedSetStyleImplicitMappingInvalidSize},
        Containers::arraySize(SharedSetStyleData));

    addTests({&BaseLayerTest::construct,
              &BaseLayerTest::constructCopy,
              &BaseLayerTest::constructMove,

              &BaseLayerTest::backgroundBlurPassCount,
              &BaseLayerTest::backgroundBlurPassCountInvalid});

    addInstancedTests({&BaseLayerTest::dynamicStyle},
        Containers::arraySize(DynamicStyleData));

    addTests({&BaseLayerTest::dynamicStyleNoDynamicStyles,
              &BaseLayerTest::dynamicStyleInvalid});

    addInstancedTests<BaseLayerTest>({&BaseLayerTest::createRemove<UnsignedInt>,
                                      &BaseLayerTest::createRemove<Enum>},
        Containers::arraySize(CreateRemoveData));

    addTests({&BaseLayerTest::createRemoveHandleRecycle,

              &BaseLayerTest::setColor,
              &BaseLayerTest::setOutlineWidth,
              &BaseLayerTest::setPadding,
              &BaseLayerTest::setTextureCoordinates,
              &BaseLayerTest::setTextureCoordinatesInvalid,

              &BaseLayerTest::invalidHandle});

    addInstancedTests({&BaseLayerTest::styleOutOfRange},
        Containers::arraySize(StyleOutOfRangeData));

    addTests({&BaseLayerTest::updateEmpty});

    addInstancedTests({&BaseLayerTest::updateDataOrder},
        Containers::arraySize(UpdateDataOrderData));

    addInstancedTests({&BaseLayerTest::updateNoStyleSet},
        Containers::arraySize(UpdateNoStyleSetData));

    addInstancedTests({&BaseLayerTest::sharedNeedsUpdateStatePropagatedToLayers},
        Containers::arraySize(SharedNeedsUpdateStatePropagatedToLayersData));
}

using namespace Math::Literals;

template<class> struct StyleTraits;
template<> struct StyleTraits<BaseLayerCommonStyleUniform> {
    static const char* name() { return "BaseLayerCommonStyleUniform"; }
};
template<> struct StyleTraits<BaseLayerStyleUniform> {
    static const char* name() { return "BaseLayerStyleUniform"; }
};

template<class T> void BaseLayerTest::styleUniformSizeAlignment() {
    setTestCaseTemplateName(StyleTraits<T>::name());

    CORRADE_FAIL_IF(sizeof(T) % sizeof(Vector4) != 0, sizeof(T) << "is not a multiple of vec4 for UBO alignment.");

    /* 48-byte structures are fine, we'll align them to 768 bytes and not
       256, but warn about that */
    CORRADE_FAIL_IF(768 % sizeof(T) != 0, sizeof(T) << "can't fit exactly into 768-byte UBO alignment.");
    if(256 % sizeof(T) != 0)
        CORRADE_WARN(sizeof(T) << "can't fit exactly into 256-byte UBO alignment, only 768.");

    CORRADE_COMPARE(alignof(T), 4);
}

void BaseLayerTest::styleUniformCommonConstructDefault() {
    BaseLayerCommonStyleUniform a;
    BaseLayerCommonStyleUniform b{DefaultInit};
    CORRADE_COMPARE(a.smoothness, 0.0f);
    CORRADE_COMPARE(b.smoothness, 0.0f);
    CORRADE_COMPARE(a.innerOutlineSmoothness, 0.0f);
    CORRADE_COMPARE(b.innerOutlineSmoothness, 0.0f);

    constexpr BaseLayerCommonStyleUniform ca;
    constexpr BaseLayerCommonStyleUniform cb{DefaultInit};
    CORRADE_COMPARE(ca.smoothness, 0.0f);
    CORRADE_COMPARE(cb.smoothness, 0.0f);
    CORRADE_COMPARE(ca.innerOutlineSmoothness, 0.0f);
    CORRADE_COMPARE(cb.innerOutlineSmoothness, 0.0f);

    CORRADE_VERIFY(std::is_nothrow_default_constructible<BaseLayerCommonStyleUniform>::value);
    CORRADE_VERIFY(std::is_nothrow_constructible<BaseLayerCommonStyleUniform, DefaultInitT>::value);

    /* Implicit construction is not allowed */
    CORRADE_VERIFY(!std::is_convertible<DefaultInitT, BaseLayerCommonStyleUniform>::value);
}

void BaseLayerTest::styleUniformCommonConstruct() {
    BaseLayerCommonStyleUniform a{3.0f, 5.0f, 0.95f};
    CORRADE_COMPARE(a.smoothness, 3.0f);
    CORRADE_COMPARE(a.innerOutlineSmoothness, 5.0f);
    CORRADE_COMPARE(a.backgroundBlurAlpha, 0.95f);

    constexpr BaseLayerCommonStyleUniform ca{3.0f, 5.0f, 0.95f};
    CORRADE_COMPARE(ca.smoothness, 3.0f);
    CORRADE_COMPARE(ca.innerOutlineSmoothness, 5.0f);
    CORRADE_COMPARE(ca.backgroundBlurAlpha, 0.95f);
}

void BaseLayerTest::styleUniformCommonConstructNoBlurParameters() {
    BaseLayerCommonStyleUniform a{3.0f, 5.0f};
    CORRADE_COMPARE(a.smoothness, 3.0f);
    CORRADE_COMPARE(a.innerOutlineSmoothness, 5.0f);
    CORRADE_COMPARE(a.backgroundBlurAlpha, 1.0f);

    constexpr BaseLayerCommonStyleUniform ca{3.0f, 5.0f};
    CORRADE_COMPARE(ca.smoothness, 3.0f);
    CORRADE_COMPARE(ca.innerOutlineSmoothness, 5.0f);
    CORRADE_COMPARE(ca.backgroundBlurAlpha, 1.0f);
}

void BaseLayerTest::styleUniformCommonConstructNoBlurParametersSingleSmoothness() {
    BaseLayerCommonStyleUniform a{4.0f};
    CORRADE_COMPARE(a.smoothness, 4.0f);
    CORRADE_COMPARE(a.innerOutlineSmoothness, 4.0f);
    CORRADE_COMPARE(a.backgroundBlurAlpha, 1.0f);

    constexpr BaseLayerCommonStyleUniform ca{4.0f};
    CORRADE_COMPARE(ca.smoothness, 4.0f);
    CORRADE_COMPARE(ca.innerOutlineSmoothness, 4.0f);
    CORRADE_COMPARE(ca.backgroundBlurAlpha, 1.0f);
}

void BaseLayerTest::styleUniformCommonConstructNoInit() {
    /* Testing only some fields, should be enough */
    BaseLayerCommonStyleUniform a;
    a.smoothness = 3.0f;
    a.innerOutlineSmoothness = 20.0f;

    new(&a) BaseLayerCommonStyleUniform{NoInit};
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

void BaseLayerTest::styleUniformCommonSetters() {
    BaseLayerCommonStyleUniform a;
    a.setSmoothness(34.0f, 12.0f);
    CORRADE_COMPARE(a.smoothness, 34.0f);
    CORRADE_COMPARE(a.innerOutlineSmoothness, 12.0f);

    /* Convenience overload setting both smoothness values */
    a.setSmoothness(2.5f);
    CORRADE_COMPARE(a.smoothness, 2.5f);
    CORRADE_COMPARE(a.innerOutlineSmoothness, 2.5f);
}

void BaseLayerTest::styleUniformConstructDefault() {
    BaseLayerStyleUniform a;
    BaseLayerStyleUniform b{DefaultInit};
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

    constexpr BaseLayerStyleUniform ca;
    constexpr BaseLayerStyleUniform cb{DefaultInit};
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

    CORRADE_VERIFY(std::is_nothrow_default_constructible<BaseLayerStyleUniform>::value);
    CORRADE_VERIFY(std::is_nothrow_constructible<BaseLayerStyleUniform, DefaultInitT>::value);

    /* Implicit construction is not allowed */
    CORRADE_VERIFY(!std::is_convertible<DefaultInitT, BaseLayerStyleUniform>::value);
}

void BaseLayerTest::styleUniformConstruct() {
    BaseLayerStyleUniform a{0xff336699_rgbaf, 0xaabbccdd_rgbaf, 0x663399cc_rgbaf, {1.0f, 2.0f, 3.0f, 4.0f}, {5.0f, 6.0f, 7.0f, 8.0f}, {0.1f, 0.2f, 0.3f, 0.4f}};
    CORRADE_COMPARE(a.topColor, 0xff336699_rgbaf);
    CORRADE_COMPARE(a.bottomColor, 0xaabbccdd_rgbaf);
    CORRADE_COMPARE(a.outlineColor, 0x663399cc_rgbaf);
    CORRADE_COMPARE(a.outlineWidth, (Vector4{1.0f, 2.0f, 3.0f, 4.0f}));
    CORRADE_COMPARE(a.cornerRadius, (Vector4{5.0f, 6.0f, 7.0f, 8.0f}));
    CORRADE_COMPARE(a.innerOutlineCornerRadius, (Vector4{0.1f, 0.2f, 0.3f, 0.4f}));

    constexpr BaseLayerStyleUniform ca{0xff336699_rgbaf, 0xaabbccdd_rgbaf, 0x663399cc_rgbaf, {1.0f, 2.0f, 3.0f, 4.0f}, {5.0f, 6.0f, 7.0f, 8.0f}, {0.1f, 0.2f, 0.3f, 0.4f}};
    CORRADE_COMPARE(ca.topColor, 0xff336699_rgbaf);
    CORRADE_COMPARE(ca.bottomColor, 0xaabbccdd_rgbaf);
    CORRADE_COMPARE(ca.outlineColor, 0x663399cc_rgbaf);
    CORRADE_COMPARE(ca.outlineWidth, (Vector4{1.0f, 2.0f, 3.0f, 4.0f}));
    CORRADE_COMPARE(ca.cornerRadius, (Vector4{5.0f, 6.0f, 7.0f, 8.0f}));
    CORRADE_COMPARE(ca.innerOutlineCornerRadius, (Vector4{0.1f, 0.2f, 0.3f, 0.4f}));
}

void BaseLayerTest::styleUniformConstructSingleRadiusWidth() {
    BaseLayerStyleUniform a{0xff336699_rgbaf, 0xaabbccdd_rgbaf, 0x663399cc_rgbaf, 2.5f, 3.5f, 4.5f};
    CORRADE_COMPARE(a.topColor, 0xff336699_rgbaf);
    CORRADE_COMPARE(a.bottomColor, 0xaabbccdd_rgbaf);
    CORRADE_COMPARE(a.outlineColor, 0x663399cc_rgbaf);
    CORRADE_COMPARE(a.outlineWidth, Vector4{2.5f});
    CORRADE_COMPARE(a.cornerRadius, Vector4{3.5f});
    CORRADE_COMPARE(a.innerOutlineCornerRadius, Vector4{4.5f});

    constexpr BaseLayerStyleUniform ca{0xff336699_rgbaf, 0xaabbccdd_rgbaf, 0x663399cc_rgbaf, 2.5f, 3.5f, 4.5f};
    CORRADE_COMPARE(ca.topColor, 0xff336699_rgbaf);
    CORRADE_COMPARE(ca.bottomColor, 0xaabbccdd_rgbaf);
    CORRADE_COMPARE(ca.outlineColor, 0x663399cc_rgbaf);
    CORRADE_COMPARE(ca.outlineWidth, Vector4{2.5f});
    CORRADE_COMPARE(ca.cornerRadius, Vector4{3.5f});
    CORRADE_COMPARE(ca.innerOutlineCornerRadius, Vector4{4.5f});
}

void BaseLayerTest::styleUniformConstructNoOutline() {
    BaseLayerStyleUniform a{0xff336699_rgbaf, 0xaabbccdd_rgbaf, {5.0f, 6.0f, 7.0f, 8.0f}};
    CORRADE_COMPARE(a.topColor, 0xff336699_rgbaf);
    CORRADE_COMPARE(a.bottomColor, 0xaabbccdd_rgbaf);
    CORRADE_COMPARE(a.outlineColor, 0xffffffff_rgbaf);
    CORRADE_COMPARE(a.outlineWidth, Vector4{0.0f});
    CORRADE_COMPARE(a.cornerRadius, (Vector4{5.0f, 6.0f, 7.0f, 8.0f}));
    CORRADE_COMPARE(a.innerOutlineCornerRadius, (Vector4{5.0f, 6.0f, 7.0f, 8.0f}));

    constexpr BaseLayerStyleUniform ca{0xff336699_rgbaf, 0xaabbccdd_rgbaf, {5.0f, 6.0f, 7.0f, 8.0f}};
    CORRADE_COMPARE(ca.topColor, 0xff336699_rgbaf);
    CORRADE_COMPARE(ca.bottomColor, 0xaabbccdd_rgbaf);
    CORRADE_COMPARE(ca.outlineColor, 0xffffffff_rgbaf);
    CORRADE_COMPARE(ca.outlineWidth, Vector4{0.0f});
    CORRADE_COMPARE(ca.cornerRadius, (Vector4{5.0f, 6.0f, 7.0f, 8.0f}));
    CORRADE_COMPARE(ca.innerOutlineCornerRadius, (Vector4{5.0f, 6.0f, 7.0f, 8.0f}));
}

void BaseLayerTest::styleUniformConstructNoOutlineSingleRadius() {
    BaseLayerStyleUniform a{0xff336699_rgbaf, 0xaabbccdd_rgbaf, 2.5f};
    CORRADE_COMPARE(a.topColor, 0xff336699_rgbaf);
    CORRADE_COMPARE(a.bottomColor, 0xaabbccdd_rgbaf);
    CORRADE_COMPARE(a.outlineColor, 0xffffffff_rgbaf);
    CORRADE_COMPARE(a.outlineWidth, Vector4{0.0f});
    CORRADE_COMPARE(a.cornerRadius, Vector4{2.5f});
    CORRADE_COMPARE(a.innerOutlineCornerRadius, Vector4{2.5f});

    constexpr BaseLayerStyleUniform ca{0xff336699_rgbaf, 0xaabbccdd_rgbaf, 2.5f};
    CORRADE_COMPARE(ca.topColor, 0xff336699_rgbaf);
    CORRADE_COMPARE(ca.bottomColor, 0xaabbccdd_rgbaf);
    CORRADE_COMPARE(ca.outlineColor, 0xffffffff_rgbaf);
    CORRADE_COMPARE(ca.outlineWidth, Vector4{0.0f});
    CORRADE_COMPARE(ca.cornerRadius, Vector4{2.5f});
    CORRADE_COMPARE(ca.innerOutlineCornerRadius, Vector4{2.5f});
}

void BaseLayerTest::styleUniformConstructNoGradient() {
    BaseLayerStyleUniform a{0xff336699_rgbaf, 0x663399cc_rgbaf, {1.0f, 2.0f, 3.0f, 4.0f}, {5.0f, 6.0f, 7.0f, 8.0f}, {0.1f, 0.2f, 0.3f, 0.4f}};
    CORRADE_COMPARE(a.topColor, 0xff336699_rgbaf);
    CORRADE_COMPARE(a.bottomColor, 0xff336699_rgbaf);
    CORRADE_COMPARE(a.outlineColor, 0x663399cc_rgbaf);
    CORRADE_COMPARE(a.outlineWidth, (Vector4{1.0f, 2.0f, 3.0f, 4.0f}));
    CORRADE_COMPARE(a.cornerRadius, (Vector4{5.0f, 6.0f, 7.0f, 8.0f}));
    CORRADE_COMPARE(a.innerOutlineCornerRadius, (Vector4{0.1f, 0.2f, 0.3f, 0.4f}));

    constexpr BaseLayerStyleUniform ca{0xff336699_rgbaf, 0x663399cc_rgbaf, {1.0f, 2.0f, 3.0f, 4.0f}, {5.0f, 6.0f, 7.0f, 8.0f}, {0.1f, 0.2f, 0.3f, 0.4f}};
    CORRADE_COMPARE(ca.topColor, 0xff336699_rgbaf);
    CORRADE_COMPARE(ca.bottomColor, 0xff336699_rgbaf);
    CORRADE_COMPARE(ca.outlineColor, 0x663399cc_rgbaf);
    CORRADE_COMPARE(ca.outlineWidth, (Vector4{1.0f, 2.0f, 3.0f, 4.0f}));
    CORRADE_COMPARE(ca.cornerRadius, (Vector4{5.0f, 6.0f, 7.0f, 8.0f}));
    CORRADE_COMPARE(ca.innerOutlineCornerRadius, (Vector4{0.1f, 0.2f, 0.3f, 0.4f}));
}

void BaseLayerTest::styleUniformConstructNoGradientSingleRadiusWidth() {
    BaseLayerStyleUniform a{0xff336699_rgbaf, 0x663399cc_rgbaf, 2.5f, 3.5f, 4.5f};
    CORRADE_COMPARE(a.topColor, 0xff336699_rgbaf);
    CORRADE_COMPARE(a.bottomColor, 0xff336699_rgbaf);
    CORRADE_COMPARE(a.outlineColor, 0x663399cc_rgbaf);
    CORRADE_COMPARE(a.outlineWidth, Vector4{2.5f});
    CORRADE_COMPARE(a.cornerRadius, Vector4{3.5f});
    CORRADE_COMPARE(a.innerOutlineCornerRadius, Vector4{4.5f});

    constexpr BaseLayerStyleUniform ca{0xff336699_rgbaf, 0x663399cc_rgbaf, 2.5f, 3.5f, 4.5f};
    CORRADE_COMPARE(ca.topColor, 0xff336699_rgbaf);
    CORRADE_COMPARE(ca.bottomColor, 0xff336699_rgbaf);
    CORRADE_COMPARE(ca.outlineColor, 0x663399cc_rgbaf);
    CORRADE_COMPARE(ca.outlineWidth, Vector4{2.5f});
    CORRADE_COMPARE(ca.cornerRadius, Vector4{3.5f});
    CORRADE_COMPARE(ca.innerOutlineCornerRadius, Vector4{4.5f});
}

void BaseLayerTest::styleUniformConstructNoGradientNoOutline() {
    BaseLayerStyleUniform a{0xff336699_rgbaf, {5.0f, 6.0f, 7.0f, 8.0f}};
    CORRADE_COMPARE(a.topColor, 0xff336699_rgbaf);
    CORRADE_COMPARE(a.bottomColor, 0xff336699_rgbaf);
    CORRADE_COMPARE(a.outlineColor, 0xffffffff_rgbaf);
    CORRADE_COMPARE(a.outlineWidth, Vector4{0.0f});
    CORRADE_COMPARE(a.cornerRadius, (Vector4{5.0f, 6.0f, 7.0f, 8.0f}));
    CORRADE_COMPARE(a.innerOutlineCornerRadius, (Vector4{5.0f, 6.0f, 7.0f, 8.0f}));

    constexpr BaseLayerStyleUniform ca{0xff336699_rgbaf, {5.0f, 6.0f, 7.0f, 8.0f}};
    CORRADE_COMPARE(ca.topColor, 0xff336699_rgbaf);
    CORRADE_COMPARE(ca.bottomColor, 0xff336699_rgbaf);
    CORRADE_COMPARE(ca.outlineColor, 0xffffffff_rgbaf);
    CORRADE_COMPARE(ca.outlineWidth, Vector4{0.0f});
    CORRADE_COMPARE(ca.cornerRadius, (Vector4{5.0f, 6.0f, 7.0f, 8.0f}));
    CORRADE_COMPARE(ca.innerOutlineCornerRadius, (Vector4{5.0f, 6.0f, 7.0f, 8.0f}));
}

void BaseLayerTest::styleUniformConstructNoGradientNoOutlineSingleRadius() {
    BaseLayerStyleUniform a{0xff336699_rgbaf, 2.5f};
    CORRADE_COMPARE(a.topColor, 0xff336699_rgbaf);
    CORRADE_COMPARE(a.bottomColor, 0xff336699_rgbaf);
    CORRADE_COMPARE(a.outlineColor, 0xffffffff_rgbaf);
    CORRADE_COMPARE(a.outlineWidth, Vector4{0.0f});
    CORRADE_COMPARE(a.cornerRadius, Vector4{2.5f});
    CORRADE_COMPARE(a.innerOutlineCornerRadius, Vector4{2.5f});

    constexpr BaseLayerStyleUniform ca{0xff336699_rgbaf, 2.5f};
    CORRADE_COMPARE(ca.topColor, 0xff336699_rgbaf);
    CORRADE_COMPARE(ca.bottomColor, 0xff336699_rgbaf);
    CORRADE_COMPARE(ca.outlineColor, 0xffffffff_rgbaf);
    CORRADE_COMPARE(ca.outlineWidth, Vector4{0.0f});
    CORRADE_COMPARE(ca.cornerRadius, Vector4{2.5f});
    CORRADE_COMPARE(ca.innerOutlineCornerRadius, Vector4{2.5f});
}

void BaseLayerTest::styleUniformConstructNoInit() {
    /* Testing only some fields, should be enough */
    BaseLayerStyleUniform a;
    a.bottomColor = 0xff3366_rgbf;
    a.innerOutlineCornerRadius = {1.0f, 2.0f, 3.0f, 4.0f};

    new(&a) BaseLayerStyleUniform{NoInit};
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

void BaseLayerTest::styleUniformSetters() {
    BaseLayerStyleUniform a;
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

void BaseLayerTest::sharedDebugFlag() {
    std::ostringstream out;
    Debug{&out} << BaseLayer::Shared::Flag::BackgroundBlur << BaseLayer::Shared::Flag(0xbe);
    CORRADE_COMPARE(out.str(), "Whee::BaseLayer::Shared::Flag::BackgroundBlur Whee::BaseLayer::Shared::Flag(0xbe)\n");
}

void BaseLayerTest::sharedDebugFlags() {
    std::ostringstream out;
    Debug{&out} << (BaseLayer::Shared::Flag::BackgroundBlur|BaseLayer::Shared::Flag(0xb0)) << BaseLayer::Shared::Flags{};
    CORRADE_COMPARE(out.str(), "Whee::BaseLayer::Shared::Flag::BackgroundBlur|Whee::BaseLayer::Shared::Flag(0xb0) Whee::BaseLayer::Shared::Flags{}\n");
}

void BaseLayerTest::sharedConfigurationConstruct() {
    BaseLayer::Shared::Configuration configuration{3, 5};
    CORRADE_COMPARE(configuration.styleUniformCount(), 3);
    CORRADE_COMPARE(configuration.styleCount(), 5);
}

void BaseLayerTest::sharedConfigurationConstructSameStyleUniformCount() {
    BaseLayer::Shared::Configuration configuration{3};
    CORRADE_COMPARE(configuration.styleUniformCount(), 3);
    CORRADE_COMPARE(configuration.styleCount(), 3);
}

void BaseLayerTest::sharedConfigurationConstructZeroStyleOrUniformCount() {
    CORRADE_SKIP_IF_NO_ASSERT();

    /* Both being zero is fine */
    BaseLayer::Shared::Configuration{0, 0};
    BaseLayer::Shared::Configuration{0};

    std::ostringstream out;
    Error redirectError{&out};
    BaseLayer::Shared::Configuration{0, 4};
    BaseLayer::Shared::Configuration{4, 0};
    CORRADE_COMPARE_AS(out.str(),
        "Whee::BaseLayer::Shared::Configuration: expected style uniform count and style count to be either both zero or both non-zero, got 0 and 4\n"
        "Whee::BaseLayer::Shared::Configuration: expected style uniform count and style count to be either both zero or both non-zero, got 4 and 0\n",
        TestSuite::Compare::String);
}

void BaseLayerTest::sharedConfigurationConstructCopy() {
    BaseLayer::Shared::Configuration a{3, 5};

    BaseLayer::Shared::Configuration b = a;
    CORRADE_COMPARE(b.styleUniformCount(), 3);
    CORRADE_COMPARE(b.styleCount(), 5);

    BaseLayer::Shared::Configuration c{7, 9};
    c = b;
    CORRADE_COMPARE(c.styleUniformCount(), 3);
    CORRADE_COMPARE(c.styleCount(), 5);

    #ifndef CORRADE_NO_STD_IS_TRIVIALLY_TRAITS
    CORRADE_VERIFY(std::is_trivially_copy_constructible<BaseLayer::Shared::Configuration>::value);
    CORRADE_VERIFY(std::is_trivially_copy_assignable<BaseLayer::Shared::Configuration>::value);
    #endif
}

void BaseLayerTest::sharedConfigurationSetters() {
    BaseLayer::Shared::Configuration configuration{3, 5};
    CORRADE_COMPARE(configuration.dynamicStyleCount(), 0);
    CORRADE_COMPARE(configuration.flags(), BaseLayer::Shared::Flags{});
    CORRADE_COMPARE(configuration.backgroundBlurRadius(), 4);
    CORRADE_COMPARE(configuration.backgroundBlurCutoff(), 0.5f/255.0f);

    configuration
        .setDynamicStyleCount(9)
        .setFlags(BaseLayer::Shared::Flag::BackgroundBlur)
        .addFlags(BaseLayer::Shared::Flag(0xe0))
        .clearFlags(BaseLayer::Shared::Flag(0x70))
        .setBackgroundBlurRadius(16, 0.1f);
    CORRADE_COMPARE(configuration.dynamicStyleCount(), 9);
    CORRADE_COMPARE(configuration.flags(), BaseLayer::Shared::Flag::BackgroundBlur|BaseLayer::Shared::Flag(0x80));
    CORRADE_COMPARE(configuration.backgroundBlurRadius(), 16);
    CORRADE_COMPARE(configuration.backgroundBlurCutoff(), 0.1f);
}

void BaseLayerTest::sharedConfigurationSettersInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    BaseLayer::Shared::Configuration configuration{3};

    /* This should be okay */
    configuration.setBackgroundBlurRadius(31);
    /* This also */
    configuration.setBackgroundBlurRadius(2, 150.0f);

    std::ostringstream out;
    Error redirectError{&out};
    configuration.setBackgroundBlurRadius(32);
    CORRADE_COMPARE(out.str(), "Whee::BaseLayer::Shared::Configuration::setBackgroundBlurRadius(): radius 32 too large\n");
}

void BaseLayerTest::sharedConstruct() {
    struct Shared: BaseLayer::Shared {
        explicit Shared(const Configuration& configuration): BaseLayer::Shared{configuration} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } shared{BaseLayer::Shared::Configuration{3, 5}
        .setDynamicStyleCount(4)
        .addFlags(BaseLayer::Shared::Flag::BackgroundBlur)
    };
    CORRADE_COMPARE(shared.styleUniformCount(), 3);
    CORRADE_COMPARE(shared.styleCount(), 5);
    CORRADE_COMPARE(shared.dynamicStyleCount(), 4);
    CORRADE_COMPARE(shared.flags(), BaseLayer::Shared::Flag::BackgroundBlur);
}

void BaseLayerTest::sharedConstructNoCreate() {
    struct Shared: BaseLayer::Shared {
        explicit Shared(NoCreateT): BaseLayer::Shared{NoCreate} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } shared{NoCreate};

    /* Shouldn't crash */
    CORRADE_VERIFY(true);

    /* Implicit construction is not allowed */
    CORRADE_VERIFY(!std::is_convertible<NoCreateT, BaseLayer::Shared>::value);
}

void BaseLayerTest::sharedConstructCopy() {
    struct Shared: BaseLayer::Shared {
        /* Clang says the constructor is unused otherwise. However I fear that
           without the constructor the type would be impossible to construct
           (and thus also to copy), leading to false positives in the trait
           check below */
        explicit CORRADE_UNUSED Shared(const Configuration& configuration): BaseLayer::Shared{Containers::pointer<BaseLayer::Shared::State>(*this, configuration)} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, const Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    };

    CORRADE_VERIFY(!std::is_copy_constructible<Shared>{});
    CORRADE_VERIFY(!std::is_copy_assignable<Shared>{});
}

void BaseLayerTest::sharedConstructMove() {
    struct Shared: BaseLayer::Shared {
        explicit Shared(const Configuration& configuration): BaseLayer::Shared{configuration} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, const Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    };

    Shared a{BaseLayer::Shared::Configuration{3, 5}
        .setDynamicStyleCount(4)
        .addFlags(BaseLayer::Shared::Flag::BackgroundBlur)};

    Shared b{Utility::move(a)};
    CORRADE_COMPARE(b.styleUniformCount(), 3);
    CORRADE_COMPARE(b.styleCount(), 5);
    CORRADE_COMPARE(b.dynamicStyleCount(), 4);
    CORRADE_COMPARE(b.flags(), BaseLayer::Shared::Flag::BackgroundBlur);

    Shared c{BaseLayer::Shared::Configuration{5, 7}};
    c = Utility::move(b);
    CORRADE_COMPARE(c.styleUniformCount(), 3);
    CORRADE_COMPARE(c.styleCount(), 5);
    CORRADE_COMPARE(c.dynamicStyleCount(), 4);
    CORRADE_COMPARE(c.flags(), BaseLayer::Shared::Flag::BackgroundBlur);

    CORRADE_VERIFY(std::is_nothrow_move_constructible<Shared>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<Shared>::value);
}

void BaseLayerTest::sharedConstructZeroStyleCount() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Shared: BaseLayer::Shared {
        explicit Shared(const Configuration& configuration): BaseLayer::Shared{configuration} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    };

    /* Zero style count or dynamic style count is fine on its own */
    Shared{Shared::Configuration{0}.setDynamicStyleCount(1)};
    Shared{Shared::Configuration{1}.setDynamicStyleCount(0)};

    std::ostringstream out;
    Error redirectError{&out};
    Shared{Shared::Configuration{0}.setDynamicStyleCount(0)};
    CORRADE_COMPARE(out.str(), "Whee::BaseLayer::Shared: expected non-zero total style count\n");
}

void BaseLayerTest::sharedSetStyle() {
    auto&& data = SharedSetStyleData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct Shared: BaseLayer::Shared {
        explicit Shared(const Configuration& configuration): BaseLayer::Shared{configuration} {}
        State& state() { return static_cast<State&>(*_state); }

        void doSetStyle(const BaseLayerCommonStyleUniform& commonUniform, Containers::ArrayView<const BaseLayerStyleUniform> uniforms) override {
            CORRADE_COMPARE(commonUniform.smoothness, 3.14f);
            CORRADE_COMPARE(uniforms.size(), 3);
            CORRADE_COMPARE(uniforms[1].outlineColor, 0xc0ffee_rgbf);
            ++setStyleCalled;
        }

        Int setStyleCalled = 0;
    } shared{BaseLayer::Shared::Configuration{3, 5}
        .setDynamicStyleCount(data.dynamicStyleCount)
    };

    /* By default the shared.state().styles array (and styleUniforms, for
       dynamic styles) is empty, it gets only filled during the setStyle()
       call. The empty state is used to detect whether setStyle() was called at
       all when calling update(). */
    CORRADE_VERIFY(shared.state().styles.isEmpty());
    CORRADE_VERIFY(shared.state().styleUniforms.isEmpty());

    shared.setStyle(
        BaseLayerCommonStyleUniform{}
            .setSmoothness(3.14f),
        {BaseLayerStyleUniform{},
         BaseLayerStyleUniform{}
            .setOutlineColor(0xc0ffee_rgbf),
         BaseLayerStyleUniform{}},
        {2, 1, 0, 0, 1},
        {{1.0f, 2.0f, 3.0f, 4.0f},
         {4.0f, 3.0f, 2.0f, 1.0f},
         {2.0f, 1.0f, 4.0f, 3.0f},
         {1.0f, 3.0f, 2.0f, 4.0f},
         {4.0f, 1.0f, 3.0f, 2.0f}});
    if(data.dynamicStyleCount == 0) {
        CORRADE_COMPARE(shared.setStyleCalled, 1);
    } else {
        CORRADE_COMPARE(shared.setStyleCalled, 0);
        /* If there are dynamic styles, it's copied into an internal array
           instead of calling doSetStyle(). The following is thus checking the
           same as doSetStyle() but on the internal array. */
        CORRADE_COMPARE(shared.state().commonStyleUniform.smoothness, 3.14f);
        CORRADE_COMPARE(shared.state().styleUniforms.size(), 3);
        CORRADE_COMPARE(shared.state().styleUniforms[1].outlineColor, 0xc0ffee_rgbf);
    }
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::BaseLayerStyle::uniform), Containers::stridedArrayView({
        2u, 1u, 0u, 0u, 1u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::BaseLayerStyle::padding), Containers::stridedArrayView({
        Vector4{1.0f, 2.0f, 3.0f, 4.0f},
        Vector4{4.0f, 3.0f, 2.0f, 1.0f},
        Vector4{2.0f, 1.0f, 4.0f, 3.0f},
        Vector4{1.0f, 3.0f, 2.0f, 4.0f},
        Vector4{4.0f, 1.0f, 3.0f, 2.0f}
    }), TestSuite::Compare::Container);
}

void BaseLayerTest::sharedSetStyleImplicitPadding() {
    auto&& data = SharedSetStyleData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct Shared: BaseLayer::Shared {
        explicit Shared(const Configuration& configuration): BaseLayer::Shared{configuration} {}
        State& state() { return static_cast<State&>(*_state); }

        void doSetStyle(const BaseLayerCommonStyleUniform& commonUniform, Containers::ArrayView<const BaseLayerStyleUniform> uniforms) override {
            CORRADE_COMPARE(commonUniform.smoothness, 3.14f);
            CORRADE_COMPARE(uniforms.size(), 3);
            CORRADE_COMPARE(uniforms[1].outlineColor, 0xc0ffee_rgbf);
            ++setStyleCalled;
        }

        Int setStyleCalled = 0;
    } shared{BaseLayer::Shared::Configuration{3, 5}
        .setDynamicStyleCount(data.dynamicStyleCount)
    };

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    shared.setStyle(
        BaseLayerCommonStyleUniform{}
            .setSmoothness(3.14f),
        {BaseLayerStyleUniform{},
         BaseLayerStyleUniform{}
            .setOutlineColor(0xc0ffee_rgbf),
         BaseLayerStyleUniform{}},
        {2, 1, 0, 0, 1},
        {});
    if(data.dynamicStyleCount == 0) {
        CORRADE_COMPARE(shared.setStyleCalled, 1);
    } else {
        CORRADE_COMPARE(shared.setStyleCalled, 0);
        /* If there are dynamic styles, it's copied into an internal array
           instead of calling doSetStyle(). The following is thus checking the
           same as doSetStyle() but on the internal array. */
        CORRADE_COMPARE(shared.state().commonStyleUniform.smoothness, 3.14f);
        CORRADE_COMPARE(shared.state().styleUniforms.size(), 3);
        CORRADE_COMPARE(shared.state().styleUniforms[1].outlineColor, 0xc0ffee_rgbf);
    }
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::BaseLayerStyle::uniform), Containers::stridedArrayView({
        2u, 1u, 0u, 0u, 1u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::BaseLayerStyle::padding), Containers::stridedArrayView({
        Vector4{},
        Vector4{},
        Vector4{},
        Vector4{},
        Vector4{}
    }), TestSuite::Compare::Container);

    /* Setting a style with implicit padding after a non-implicit padding was
       set should reset it back to zeros */
    shared.setStyle(
        BaseLayerCommonStyleUniform{}
            .setSmoothness(3.14f),
        {BaseLayerStyleUniform{},
         BaseLayerStyleUniform{}
            .setOutlineColor(0xc0ffee_rgbf),
         BaseLayerStyleUniform{}},
        {2, 1, 0, 0, 1},
        {{1.0f, 2.0f, 3.0f, 4.0f},
         {4.0f, 3.0f, 2.0f, 1.0f},
         {2.0f, 1.0f, 4.0f, 3.0f},
         {1.0f, 3.0f, 2.0f, 4.0f},
         {4.0f, 1.0f, 3.0f, 2.0f}});
    shared.setStyle(
        BaseLayerCommonStyleUniform{}
            .setSmoothness(3.14f),
        {BaseLayerStyleUniform{},
         BaseLayerStyleUniform{}
            .setOutlineColor(0xc0ffee_rgbf),
         BaseLayerStyleUniform{}},
        {2, 1, 0, 0, 1},
        {});
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::BaseLayerStyle::padding), Containers::stridedArrayView({
        Vector4{},
        Vector4{},
        Vector4{},
        Vector4{},
        Vector4{}
    }), TestSuite::Compare::Container);
}

void BaseLayerTest::sharedSetStyleInvalidSize() {
    auto&& data = SharedSetStyleData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    CORRADE_SKIP_IF_NO_ASSERT();

    struct Shared: BaseLayer::Shared {
        explicit Shared(const Configuration& configuration): BaseLayer::Shared{configuration} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } shared{BaseLayer::Shared::Configuration{3, 5}
        /* The checks should all deal with just the shared style count, not be
           dependent on this */
        .setDynamicStyleCount(data.dynamicStyleCount)
    };

    std::ostringstream out;
    Error redirectError{&out};
    shared.setStyle(BaseLayerCommonStyleUniform{},
        {BaseLayerStyleUniform{}, BaseLayerStyleUniform{}},
        {0, 1, 2, 3, 4},
        {{}, {}, {}, {}, {}});
    shared.setStyle(BaseLayerCommonStyleUniform{},
        {BaseLayerStyleUniform{}, BaseLayerStyleUniform{}},
        {0, 1, 2},
        {{}, {}, {}, {}, {}});
    shared.setStyle(BaseLayerCommonStyleUniform{},
        {BaseLayerStyleUniform{}, BaseLayerStyleUniform{}, BaseLayerStyleUniform{}},
        {0, 1, 2, 3, 4},
        {{}, {}, {}});
    CORRADE_COMPARE(out.str(),
        "Whee::BaseLayer::Shared::setStyle(): expected 3 uniforms, got 2\n"
        "Whee::BaseLayer::Shared::setStyle(): expected 5 style uniform indices, got 3\n"
        "Whee::BaseLayer::Shared::setStyle(): expected either no or 5 paddings, got 3\n");
}

void BaseLayerTest::sharedSetStyleImplicitMapping() {
    auto&& data = SharedSetStyleData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct Shared: BaseLayer::Shared {
        explicit Shared(const Configuration& configuration): BaseLayer::Shared{configuration} {}
        State& state() { return static_cast<State&>(*_state); }

        void doSetStyle(const BaseLayerCommonStyleUniform& commonUniform, Containers::ArrayView<const BaseLayerStyleUniform> uniforms) override {
            CORRADE_COMPARE(commonUniform.smoothness, 3.14f);
            CORRADE_COMPARE(uniforms.size(), 3);
            CORRADE_COMPARE(uniforms[1].outlineColor, 0xc0ffee_rgbf);
            ++setStyleCalled;
        }

        Int setStyleCalled = 0;
    } shared{BaseLayer::Shared::Configuration{3}
        .setDynamicStyleCount(data.dynamicStyleCount)
    };

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    shared.setStyle(
        BaseLayerCommonStyleUniform{}
            .setSmoothness(3.14f),
        {BaseLayerStyleUniform{},
         BaseLayerStyleUniform{}
            .setOutlineColor(0xc0ffee_rgbf),
         BaseLayerStyleUniform{}},
        {{1.0f, 2.0f, 3.0f, 4.0f},
         {4.0f, 3.0f, 2.0f, 1.0f},
         {2.0f, 1.0f, 4.0f, 3.0f}});
    if(data.dynamicStyleCount == 0) {
        CORRADE_COMPARE(shared.setStyleCalled, 1);
    } else {
        CORRADE_COMPARE(shared.setStyleCalled, 0);
        /* If there are dynamic styles, it's copied into an internal array
           instead of calling doSetStyle(). The following is thus checking the
           same as doSetStyle() but on the internal array. */
        CORRADE_COMPARE(shared.state().commonStyleUniform.smoothness, 3.14f);
        CORRADE_COMPARE(shared.state().styleUniforms.size(), 3);
        CORRADE_COMPARE(shared.state().styleUniforms[1].outlineColor, 0xc0ffee_rgbf);
    }
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::BaseLayerStyle::uniform), Containers::stridedArrayView({
        0u, 1u, 2u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::BaseLayerStyle::padding), Containers::stridedArrayView({
        Vector4{1.0f, 2.0f, 3.0f, 4.0f},
        Vector4{4.0f, 3.0f, 2.0f, 1.0f},
        Vector4{2.0f, 1.0f, 4.0f, 3.0f}
    }), TestSuite::Compare::Container);
}

void BaseLayerTest::sharedSetStyleImplicitMappingImplicitPadding() {
    auto&& data = SharedSetStyleData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct Shared: BaseLayer::Shared {
        explicit Shared(const Configuration& configuration): BaseLayer::Shared{configuration} {}
        State& state() { return static_cast<State&>(*_state); }

        void doSetStyle(const BaseLayerCommonStyleUniform& commonUniform, Containers::ArrayView<const BaseLayerStyleUniform> uniforms) override {
            CORRADE_COMPARE(commonUniform.smoothness, 3.14f);
            CORRADE_COMPARE(uniforms.size(), 3);
            CORRADE_COMPARE(uniforms[1].outlineColor, 0xc0ffee_rgbf);
            ++setStyleCalled;
        }

        Int setStyleCalled = 0;
    } shared{BaseLayer::Shared::Configuration{3}
        .setDynamicStyleCount(data.dynamicStyleCount)
    };

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    shared.setStyle(
        BaseLayerCommonStyleUniform{}
            .setSmoothness(3.14f),
        {BaseLayerStyleUniform{},
         BaseLayerStyleUniform{}
            .setOutlineColor(0xc0ffee_rgbf),
         BaseLayerStyleUniform{}},
        {});
    if(data.dynamicStyleCount == 0) {
        CORRADE_COMPARE(shared.setStyleCalled, 1);
    } else {
        CORRADE_COMPARE(shared.setStyleCalled, 0);
        /* If there are dynamic styles, it's copied into an internal array
           instead of calling doSetStyle(). The following is thus checking the
           same as doSetStyle() but on the internal array. */
        CORRADE_COMPARE(shared.state().commonStyleUniform.smoothness, 3.14f);
        CORRADE_COMPARE(shared.state().styleUniforms.size(), 3);
        CORRADE_COMPARE(shared.state().styleUniforms[1].outlineColor, 0xc0ffee_rgbf);
    }
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::BaseLayerStyle::uniform), Containers::stridedArrayView({
        0u, 1u, 2u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::BaseLayerStyle::padding), Containers::stridedArrayView({
        Vector4{},
        Vector4{},
        Vector4{}
    }), TestSuite::Compare::Container);

    /* Setting a style with implicit padding after a non-implicit padding was
       set should reset it back to zeros */
    shared.setStyle(
        BaseLayerCommonStyleUniform{}
            .setSmoothness(3.14f),
        {BaseLayerStyleUniform{},
         BaseLayerStyleUniform{}
            .setOutlineColor(0xc0ffee_rgbf),
         BaseLayerStyleUniform{}},
        {{1.0f, 2.0f, 3.0f, 4.0f},
         {4.0f, 3.0f, 2.0f, 1.0f},
         {2.0f, 1.0f, 4.0f, 3.0f}});
    shared.setStyle(
        BaseLayerCommonStyleUniform{}
            .setSmoothness(3.14f),
        {BaseLayerStyleUniform{},
         BaseLayerStyleUniform{}
            .setOutlineColor(0xc0ffee_rgbf),
         BaseLayerStyleUniform{}},
        {});
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::BaseLayerStyle::padding), Containers::stridedArrayView({
        Vector4{},
        Vector4{},
        Vector4{}
    }), TestSuite::Compare::Container);
}

void BaseLayerTest::sharedSetStyleImplicitMappingInvalidSize() {
    auto&& data = SharedSetStyleData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    CORRADE_SKIP_IF_NO_ASSERT();

    struct Shared: BaseLayer::Shared {
        explicit Shared(const Configuration& configuration): BaseLayer::Shared{configuration} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } shared{BaseLayer::Shared::Configuration{3, 5}
        /* The checks should all deal with just the shared style count, not be
           dependent on this */
        .setDynamicStyleCount(data.dynamicStyleCount)
    };

    std::ostringstream out;
    Error redirectError{&out};
    shared.setStyle(BaseLayerCommonStyleUniform{},
        {BaseLayerStyleUniform{}, BaseLayerStyleUniform{}},
        {{}, {}, {}, {}, {}});
    CORRADE_COMPARE(out.str(),
        "Whee::BaseLayer::Shared::setStyle(): there's 3 uniforms for 5 styles, provide an explicit mapping\n");
}

void BaseLayerTest::construct() {
    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(const Configuration& configuration): BaseLayer::Shared{configuration} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } shared{BaseLayer::Shared::Configuration{3, 5}};

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    } layer{layerHandle(137, 0xfe), shared};

    CORRADE_COMPARE(layer.handle(), layerHandle(137, 0xfe));
    CORRADE_COMPARE(&layer.shared(), &shared);
    /* Const overload */
    CORRADE_COMPARE(&static_cast<const Layer&>(layer).shared(), &shared);
}

void BaseLayerTest::constructCopy() {
    CORRADE_VERIFY(!std::is_copy_constructible<BaseLayer>{});
    CORRADE_VERIFY(!std::is_copy_assignable<BaseLayer>{});
}

void BaseLayerTest::constructMove() {
    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(const Configuration& configuration): BaseLayer::Shared{configuration} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    };

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    };

    LayerShared shared{BaseLayer::Shared::Configuration{1, 3}};
    LayerShared shared2{BaseLayer::Shared::Configuration{5, 7}};

    Layer a{layerHandle(137, 0xfe), shared};

    Layer b{Utility::move(a)};
    CORRADE_COMPARE(b.handle(), layerHandle(137, 0xfe));
    CORRADE_COMPARE(&b.shared(), &shared);

    Layer c{layerHandle(0, 2), shared2};
    c = Utility::move(b);
    CORRADE_COMPARE(c.handle(), layerHandle(137, 0xfe));
    CORRADE_COMPARE(&c.shared(), &shared);

    CORRADE_VERIFY(std::is_nothrow_move_constructible<BaseLayer>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<BaseLayer>::value);
}

void BaseLayerTest::backgroundBlurPassCount() {
    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(const Configuration& configuration): BaseLayer::Shared{configuration} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } shared{BaseLayer::Shared::Configuration{3, 3}
        .addFlags(BaseLayer::Shared::Flag::BackgroundBlur)
    };

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};
    CORRADE_COMPARE(layer.backgroundBlurPassCount(), 1);

    layer.setBackgroundBlurPassCount(11);
    CORRADE_COMPARE(layer.backgroundBlurPassCount(), 11);
}

void BaseLayerTest::backgroundBlurPassCountInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(const Configuration& configuration): BaseLayer::Shared{configuration} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    };
    LayerShared sharedNoBlur{BaseLayer::Shared::Configuration{3}};
    LayerShared sharedBlur{BaseLayer::Shared::Configuration{3}
        .addFlags(BaseLayer::Shared::Flag::BackgroundBlur)};

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    };
    Layer noBlur{layerHandle(0, 1), sharedNoBlur};
    Layer blur{layerHandle(0, 1), sharedBlur};

    std::ostringstream out;
    Error redirectError{&out};
    noBlur.backgroundBlurPassCount();
    noBlur.setBackgroundBlurPassCount(2);
    blur.setBackgroundBlurPassCount(0);
    CORRADE_COMPARE_AS(out.str(),
        "Whee::BaseLayer::backgroundBlurPassCount(): background blur not enabled\n"
        "Whee::BaseLayer::setBackgroundBlurPassCount(): background blur not enabled\n"
        "Whee::BaseLayer::setBackgroundBlurPassCount(): expected at least one pass\n",
        TestSuite::Compare::String);
}

void BaseLayerTest::dynamicStyle() {
    auto&& data = DynamicStyleData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(const Configuration& configuration): BaseLayer::Shared{configuration} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } shared{BaseLayer::Shared::Configuration{12, 2}
        .setDynamicStyleCount(3)
    };

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}

        BaseLayer::State& stateData() {
            return static_cast<BaseLayer::State&>(*_state);
        }
    } layer{layerHandle(0, 1), shared};

    /* All styles should be set to their defaults initially. Checking just a
       subset of the uniform properties, should be enough. */
    CORRADE_COMPARE_AS(stridedArrayView(layer.dynamicStyleUniforms()).slice(&BaseLayerStyleUniform::bottomColor), Containers::arrayView({
        0xffffffff_rgbaf,
        0xffffffff_rgbaf,
        0xffffffff_rgbaf
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.dynamicStyleUniforms()).slice(&BaseLayerStyleUniform::cornerRadius), Containers::arrayView({
        Vector4{0.0f},
        Vector4{0.0f},
        Vector4{0.0f}
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(layer.dynamicStylePaddings(), Containers::arrayView({
        Vector4{0.0f},
        Vector4{0.0f},
        Vector4{0.0f}
    }), TestSuite::Compare::Container);
    /* Neither LayerState nor the state bit is set initially, the initial
       upload is done implicitly on the first update */
    CORRADE_COMPARE(layer.state(), LayerStates{});
    CORRADE_VERIFY(!layer.stateData().dynamicStyleChanged);

    /* Setting a style should change these and flip the state bit on again */
    layer.stateData().dynamicStyleChanged = false;
    layer.setDynamicStyle(1,
        BaseLayerStyleUniform{}
            .setColor(0xff3366_rgbf, 0x11223344_rgbaf)
            .setCornerRadius(4.0f),
        data.padding1);
    layer.setDynamicStyle(2,
        BaseLayerStyleUniform{}
            .setColor(0x11223344_rgbaf, 0xff3366_rgbf)
            .setCornerRadius(1.0f),
        data.padding2);
    CORRADE_COMPARE_AS(stridedArrayView(layer.dynamicStyleUniforms()).slice(&BaseLayerStyleUniform::bottomColor), Containers::arrayView({
        0xffffffff_rgbaf,
        0x11223344_rgbaf,
        0xff3366ff_rgbaf
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.dynamicStyleUniforms()).slice(&BaseLayerStyleUniform::cornerRadius), Containers::arrayView({
        Vector4{0.0f},
        Vector4{4.0f},
        Vector4{1.0f}
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(layer.dynamicStylePaddings(), Containers::arrayView({
        Vector4{0.0f},
        data.padding1,
        data.padding2
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE(layer.state(), data.expectedStates);
    CORRADE_VERIFY(layer.stateData().dynamicStyleChanged);
}

void BaseLayerTest::dynamicStyleNoDynamicStyles() {
    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(const Configuration& configuration): BaseLayer::Shared{configuration} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } shared{BaseLayer::Shared::Configuration{12, 2}};

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    CORRADE_COMPARE(layer.dynamicStyleUniforms().size(), 0);
    CORRADE_COMPARE(layer.dynamicStylePaddings().size(), 0);
}

void BaseLayerTest::dynamicStyleInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(const Configuration& configuration): BaseLayer::Shared{configuration} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } shared{BaseLayer::Shared::Configuration{12, 7}
        /* Making sure it's less than both style count and uniform count to
           verify it's not checked against those */
        .setDynamicStyleCount(3)
    };

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    std::ostringstream out;
    Error redirectError{&out};
    layer.setDynamicStyle(3, BaseLayerStyleUniform{}, {});
    CORRADE_COMPARE(out.str(), "Whee::BaseLayer::setDynamicStyle(): index 3 out of range for 3 dynamic styles\n");
}

template<class T> void BaseLayerTest::createRemove() {
    auto&& data = CreateRemoveData[testCaseInstanceId()];
    setTestCaseDescription(data.name);
    setTestCaseTemplateName(std::is_same<T, Enum>::value ? "Enum" : "UnsignedInt");

    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(const Configuration& configuration): BaseLayer::Shared{configuration} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } shared{BaseLayer::Shared::Configuration{12, data.styleCount}
        .setDynamicStyleCount(data.dynamicStyleCount)
    };

    /* Not setting any padding via style -- tested in setPadding() instead */

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    /* Default color and outline width */
    DataHandle first = layer.create(T(17), data.node);
    CORRADE_COMPARE(layer.node(first), data.node);
    CORRADE_COMPARE(layer.style(first), 17);
    CORRADE_COMPARE(layer.color(first), 0xffffff_rgbf);
    CORRADE_COMPARE(layer.outlineWidth(first), Vector4{0.0f});
    CORRADE_COMPARE(layer.padding(first), Vector4{0.0f});
    CORRADE_COMPARE(layer.state(), data.state);

    /* Default outline width */
    DataHandle second = layer.create(T(23), 0xff3366_rgbf, data.node);
    CORRADE_COMPARE(layer.node(second), data.node);
    CORRADE_COMPARE(layer.style(second), 23);
    CORRADE_COMPARE(layer.color(second), 0xff3366_rgbf);
    CORRADE_COMPARE(layer.outlineWidth(second), Vector4{0.0f});
    CORRADE_COMPARE(layer.padding(second), Vector4{0.0f});
    CORRADE_COMPARE(layer.state(), data.state);

    /* Single-value outline width */
    DataHandle third = layer.create(T(19), 0xff3366_rgbf, 4.0f, data.node);
    CORRADE_COMPARE(layer.node(third), data.node);
    CORRADE_COMPARE(layer.style(third), 19);
    CORRADE_COMPARE(layer.color(third), 0xff3366_rgbf);
    CORRADE_COMPARE(layer.outlineWidth(third), Vector4{4.0f});
    CORRADE_COMPARE(layer.padding(third), Vector4{0.0f});
    CORRADE_COMPARE(layer.state(), data.state);

    /* Everything explicit, testing also the getter overloads and templates */
    DataHandle fourth = layer.create(T(37), 0xff3366_rgbf, {3.0f, 2.0f, 1.0f, 4.0f}, data.node);
    CORRADE_COMPARE(layer.node(fourth), data.node);
    if(data.layerDataHandleOverloads) {
        CORRADE_COMPARE(layer.style(dataHandleData(fourth)), 37);
        /* Can't use T, as the function restricts to enum types which would
           fail for T == UnsignedInt */
        CORRADE_COMPARE(layer.template style<Enum>(dataHandleData(fourth)), Enum(37));
        CORRADE_COMPARE(layer.color(dataHandleData(fourth)), 0xff3366_rgbf);
        CORRADE_COMPARE(layer.outlineWidth(dataHandleData(fourth)), (Vector4{3.0f, 2.0f, 1.0f, 4.0f}));
        CORRADE_COMPARE(layer.padding(dataHandleData(fourth)), Vector4{0.0f});
    } else {
        CORRADE_COMPARE(layer.style(fourth), 37);
        /* Can't use T, as the function restricts to enum types which would
            fail for T == UnsignedInt */
        CORRADE_COMPARE(layer.template style<Enum>(fourth), Enum(37));
        CORRADE_COMPARE(layer.color(fourth), 0xff3366_rgbf);
        CORRADE_COMPARE(layer.outlineWidth(fourth), (Vector4{3.0f, 2.0f, 1.0f, 4.0f}));
        CORRADE_COMPARE(layer.padding(fourth), Vector4{0.0f});
    }
    CORRADE_COMPARE(layer.state(), data.state);

    /* Removing a quad just delegates to the base implementation, nothing
       else needs to be cleaned up */
    data.layerDataHandleOverloads ?
        layer.remove(dataHandleData(third)) :
        layer.remove(third);
    CORRADE_VERIFY(!layer.isHandleValid(third));
}

void BaseLayerTest::createRemoveHandleRecycle() {
    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(const Configuration& configuration): BaseLayer::Shared{configuration} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } shared{BaseLayer::Shared::Configuration{1, 3}};

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    DataHandle first = layer.create(0);
    DataHandle second = layer.create(0);
    layer.setPadding(second, Vector4{5.0f});
    CORRADE_COMPARE(layer.padding(first), Vector4{0.0f});
    CORRADE_COMPARE(layer.padding(second), Vector4{5.0f});

    /* Data that reuses a previous slot should have the padding cleared */
    layer.remove(second);
    DataHandle second2 = layer.create(0);
    CORRADE_COMPARE(dataHandleId(second2), dataHandleId(second));
    CORRADE_COMPARE(layer.padding(second2), Vector4{0.0f});
}

void BaseLayerTest::setColor() {
    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(const Configuration& configuration): BaseLayer::Shared{configuration} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } shared{BaseLayer::Shared::Configuration{1, 3}};

    /* Needed in order to be able to call update() */
    shared.setStyle(BaseLayerCommonStyleUniform{},
        {BaseLayerStyleUniform{}},
        {0, 0, 0},
        {});

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    /* Just to be sure the setters aren't picking up the first ever data
       always */
    layer.create(1);

    DataHandle data = layer.create(2, 0xff3366_rgbf);
    CORRADE_COMPARE(layer.color(data), 0xff3366_rgbf);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Setting a color marks the layer as dirty */
    layer.setColor(data, 0xaabbc_rgbf);
    CORRADE_COMPARE(layer.color(data), 0xaabbc_rgbf);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Testing also the other overload */
    layer.setColor(dataHandleData(data), 0x112233_rgbf);
    CORRADE_COMPARE(layer.color(data), 0x112233_rgbf);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);
}

void BaseLayerTest::setOutlineWidth() {
    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(const Configuration& configuration): BaseLayer::Shared{configuration} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } shared{BaseLayer::Shared::Configuration{2, 3}};

    /* Needed in order to be able to call update() */
    shared.setStyle(BaseLayerCommonStyleUniform{},
        {BaseLayerStyleUniform{},
         BaseLayerStyleUniform{}},
        {0, 0, 0},
        {});

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    /* Just to be sure the setters aren't picking up the first ever data
       always */
    layer.create(2);

    DataHandle data = layer.create(1, 0xff3366_rgbf, {3.0f, 1.0f, 2.0f, 4.0f});
    CORRADE_COMPARE(layer.outlineWidth(data), (Vector4{3.0f, 1.0f, 2.0f, 4.0f}));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Setting an outline width marks the layer as dirty */
    layer.setOutlineWidth(data, {2.0f, 4.0f, 3.0f, 1.0f});
    CORRADE_COMPARE(layer.outlineWidth(data), (Vector4{2.0f, 4.0f, 3.0f, 1.0f}));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Testing also the other overload */
    layer.setOutlineWidth(dataHandleData(data), {1.0f, 2.0f, 3.0f, 4.0f});
    CORRADE_COMPARE(layer.outlineWidth(data), (Vector4{1.0f, 2.0f, 3.0f, 4.0f}));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Single-value width */
    layer.setOutlineWidth(data, 4.0f);
    CORRADE_COMPARE(layer.outlineWidth(data), Vector4{4.0f});
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Testing also the other overload */
    layer.setOutlineWidth(dataHandleData(data), 3.0f);
    CORRADE_COMPARE(layer.outlineWidth(data), Vector4{3.0f});
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);
}

void BaseLayerTest::setPadding() {
    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(const Configuration& configuration): BaseLayer::Shared{configuration} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } shared{BaseLayer::Shared::Configuration{2, 3}};

    /* Needed in order to be able to call update() */
    shared.setStyle(BaseLayerCommonStyleUniform{},
        {BaseLayerStyleUniform{},
         BaseLayerStyleUniform{}},
        {0, 0, 0},
        {});

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    /* Just to be sure the setters aren't picking up the first ever data
       always */
    layer.create(2);

    DataHandle data = layer.create(1, 0xff3366_rgbf);
    CORRADE_COMPARE(layer.padding(data), Vector4{0.0f});
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Setting a padding marks the layer as dirty */
    layer.setPadding(data, {2.0f, 4.0f, 3.0f, 1.0f});
    CORRADE_COMPARE(layer.padding(data), (Vector4{2.0f, 4.0f, 3.0f, 1.0f}));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Testing also the other overload */
    layer.setPadding(dataHandleData(data), {1.0f, 2.0f, 3.0f, 4.0f});
    CORRADE_COMPARE(layer.padding(dataHandleData(data)), (Vector4{1.0f, 2.0f, 3.0f, 4.0f}));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Single-value padding */
    layer.setPadding(data, 4.0f);
    CORRADE_COMPARE(layer.padding(data), Vector4{4.0f});
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Testing also the other overload */
    layer.setPadding(dataHandleData(data), 3.0f);
    CORRADE_COMPARE(layer.padding(dataHandleData(data)), Vector4{3.0f});
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);
}

void BaseLayerTest::setTextureCoordinates() {
    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(const Configuration& configuration): BaseLayer::Shared{configuration} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } shared{BaseLayer::Shared::Configuration{1}
        .addFlags(BaseLayer::Shared::Flag::Textured)};

    /* Needed in order to be able to call update() */
    shared.setStyle(BaseLayerCommonStyleUniform{},
        {BaseLayerStyleUniform{}},
        {});

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    DataHandle data = layer.create(0);
    CORRADE_COMPARE(layer.textureCoordinateOffset(data), Vector3{0.0f});
    CORRADE_COMPARE(layer.textureCoordinateSize(data), Vector2{1.0f});
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Setting texture coordinates marks the layer as dirty */
    layer.setTextureCoordinates(data, {0.5f, 0.75f, 35.0f}, {0.25f, 0.125f});
    CORRADE_COMPARE(layer.textureCoordinateOffset(data), (Vector3{0.5f, 0.75f, 35.0f}));
    CORRADE_COMPARE(layer.textureCoordinateSize(data), (Vector2{0.25f, 0.125f}));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Testing also the other overload */
    layer.setTextureCoordinates(dataHandleData(data), {0.25f, 0.5f, 5.0f}, {0.75f, 0.5f});
    CORRADE_COMPARE(layer.textureCoordinateOffset(data), (Vector3{0.25f, 0.5f, 5.0f}));
    CORRADE_COMPARE(layer.textureCoordinateSize(data), (Vector2{0.75f, 0.5f}));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);
}

void BaseLayerTest::setTextureCoordinatesInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(const Configuration& configuration): BaseLayer::Shared{configuration} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } shared{BaseLayer::Shared::Configuration{1}};

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    DataHandle data = layer.create(0);

    std::ostringstream out;
    Error redirectError{&out};
    layer.textureCoordinateOffset(data);
    layer.textureCoordinateOffset(dataHandleData(data));
    layer.textureCoordinateSize(data);
    layer.textureCoordinateSize(dataHandleData(data));
    layer.setTextureCoordinates(data, {}, {});
    layer.setTextureCoordinates(dataHandleData(data), {}, {});
    CORRADE_COMPARE_AS(out.str(),
        "Whee::BaseLayer::textureCoordinateOffset(): texturing not enabled\n"
        "Whee::BaseLayer::textureCoordinateOffset(): texturing not enabled\n"
        "Whee::BaseLayer::textureCoordinateSize(): texturing not enabled\n"
        "Whee::BaseLayer::textureCoordinateSize(): texturing not enabled\n"
        "Whee::BaseLayer::setTextureCoordinates(): texturing not enabled\n"
        "Whee::BaseLayer::setTextureCoordinates(): texturing not enabled\n",
        TestSuite::Compare::String);
}

void BaseLayerTest::invalidHandle() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(const Configuration& configuration): BaseLayer::Shared{configuration} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } shared{BaseLayer::Shared::Configuration{1}
        .addFlags(BaseLayer::Shared::Flag::Textured)
    };

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    std::ostringstream out;
    Error redirectError{&out};
    layer.color(DataHandle::Null);
    layer.color(LayerDataHandle::Null);
    layer.setColor(DataHandle::Null, {});
    layer.setColor(LayerDataHandle::Null, {});
    layer.outlineWidth(DataHandle::Null);
    layer.outlineWidth(LayerDataHandle::Null);
    layer.setOutlineWidth(DataHandle::Null, {});
    layer.setOutlineWidth(LayerDataHandle::Null, {});
    layer.padding(DataHandle::Null);
    layer.padding(LayerDataHandle::Null);
    layer.setPadding(DataHandle::Null, {});
    layer.setPadding(LayerDataHandle::Null, {});
    layer.textureCoordinateOffset(DataHandle::Null);
    layer.textureCoordinateOffset(LayerDataHandle::Null);
    layer.textureCoordinateSize(DataHandle::Null);
    layer.textureCoordinateSize(LayerDataHandle::Null);
    layer.setTextureCoordinates(DataHandle::Null, {}, {});
    layer.setTextureCoordinates(LayerDataHandle::Null, {}, {});
    CORRADE_COMPARE_AS(out.str(),
        "Whee::BaseLayer::color(): invalid handle Whee::DataHandle::Null\n"
        "Whee::BaseLayer::color(): invalid handle Whee::LayerDataHandle::Null\n"
        "Whee::BaseLayer::setColor(): invalid handle Whee::DataHandle::Null\n"
        "Whee::BaseLayer::setColor(): invalid handle Whee::LayerDataHandle::Null\n"
        "Whee::BaseLayer::outlineWidth(): invalid handle Whee::DataHandle::Null\n"
        "Whee::BaseLayer::outlineWidth(): invalid handle Whee::LayerDataHandle::Null\n"
        "Whee::BaseLayer::setOutlineWidth(): invalid handle Whee::DataHandle::Null\n"
        "Whee::BaseLayer::setOutlineWidth(): invalid handle Whee::LayerDataHandle::Null\n"
        "Whee::BaseLayer::padding(): invalid handle Whee::DataHandle::Null\n"
        "Whee::BaseLayer::padding(): invalid handle Whee::LayerDataHandle::Null\n"
        "Whee::BaseLayer::setPadding(): invalid handle Whee::DataHandle::Null\n"
        "Whee::BaseLayer::setPadding(): invalid handle Whee::LayerDataHandle::Null\n"
        "Whee::BaseLayer::textureCoordinateOffset(): invalid handle Whee::DataHandle::Null\n"
        "Whee::BaseLayer::textureCoordinateOffset(): invalid handle Whee::LayerDataHandle::Null\n"
        "Whee::BaseLayer::textureCoordinateSize(): invalid handle Whee::DataHandle::Null\n"
        "Whee::BaseLayer::textureCoordinateSize(): invalid handle Whee::LayerDataHandle::Null\n"
        "Whee::BaseLayer::setTextureCoordinates(): invalid handle Whee::DataHandle::Null\n"
        "Whee::BaseLayer::setTextureCoordinates(): invalid handle Whee::LayerDataHandle::Null\n",
        TestSuite::Compare::String);
}

void BaseLayerTest::styleOutOfRange() {
    auto&& data = StyleOutOfRangeData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    CORRADE_SKIP_IF_NO_ASSERT();

    /* In this case the uniform count is higher than the style count, which is
       unlikely to happen in practice. It's to verify the check happens against
       the style count, not uniform count. */
    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(const Configuration& configuration): BaseLayer::Shared{configuration} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } shared{BaseLayer::Shared::Configuration{6, data.styleCount}
        .setDynamicStyleCount(data.dynamicStyleCount)
    };

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    std::ostringstream out;
    Error redirectError{&out};
    layer.create(3);
    CORRADE_COMPARE(out.str(),
        "Whee::BaseLayer::create(): style 3 out of range for 3 styles\n");
}

void BaseLayerTest::updateEmpty() {
    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(const Configuration& configuration): BaseLayer::Shared{configuration} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } shared{BaseLayer::Shared::Configuration{1}};
    shared.setStyle(
        BaseLayerCommonStyleUniform{},
        {BaseLayerStyleUniform{}},
        {});

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0, 1), shared};

    /* Shouldn't crash or do anything weird */
    layer.update(LayerState::NeedsNodeEnabledUpdate|LayerState::NeedsNodeOrderUpdate|LayerState::NeedsNodeOffsetSizeUpdate|LayerState::NeedsAttachmentUpdate|LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate|LayerState::NeedsSharedDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_VERIFY(true);
}

void BaseLayerTest::updateDataOrder() {
    auto&& data = UpdateDataOrderData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Does just extremely basic verification that the vertex and index data
       get filled with correct contents and in correct order depending on
       LayerStates passed in. The actual visual output is checked in
       BaseLayerGLTest. */

    BaseLayer::Shared::Configuration configuration{3, data.styleCount};
    if(data.textured)
        configuration.addFlags(BaseLayer::Shared::Flag::Textured);
    if(data.dynamicStyleCount)
        configuration.setDynamicStyleCount(data.dynamicStyleCount);

    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(const Configuration& configuration): BaseLayer::Shared{configuration} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } shared{configuration};

    if(data.styleCount == 5) {
        shared.setStyle(
            BaseLayerCommonStyleUniform{},
            {BaseLayerStyleUniform{}, BaseLayerStyleUniform{}, BaseLayerStyleUniform{}},
            /* Style 4 doesn't get used (gets transitioned to 2), use a weird
               uniform index and padding to verify it doesn't get picked */
            {1, 2, 0, 1, 666},
            {{}, {}, data.paddingFromStyle, {}, Vector4{666}});
    } else if(data.styleCount == 2) {
        shared.setStyle(
            BaseLayerCommonStyleUniform{},
            {BaseLayerStyleUniform{}, BaseLayerStyleUniform{}, BaseLayerStyleUniform{}},
            {1, 2},
            {{}, {}});
    } else CORRADE_INTERNAL_ASSERT_UNREACHABLE();

    shared.setStyleTransition(
        nullptr,
        nullptr,
        [](UnsignedInt style) {
            return style == 4 ? 2u : style;
        }
    );

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
        const BaseLayer::State& stateData() const {
            return static_cast<const BaseLayer::State&>(*_state);
        }
    } layer{layerHandle(0, 1), shared};

    /* Two node handles to attach the data to */
    NodeHandle node6 = nodeHandle(6, 0);
    NodeHandle node15 = nodeHandle(15, 0);

    /* Create 10 data handles. Only three get filled and actually used. */
    layer.create(0);                                                    /* 0 */
    layer.create(0);                                                    /* 1 */
    layer.create(0);                                                    /* 2 */
    /* Node 6 is disabled, so style 4 should get transitioned to 2 if not
       dynamic */
    DataHandle data3 = layer.create(4, 0xff3366_rgbf, {1.0f, 2.0f, 3.0f, 4.0f}, node6);
    layer.create(0);                                                    /* 4 */
    layer.create(0);                                                    /* 5 */
    layer.create(0);                                                    /* 6 */
    DataHandle data7 = layer.create(1, 0x112233_rgbf, Vector4{2.0f}, node15);
    layer.create(0);                                                    /* 8 */
    layer.create(3, 0x663399_rgbf, {3.0f, 2.0f, 1.0f, 4.0f}, node15);   /* 9 */

    if(!data.paddingFromData.isZero())
        layer.setPadding(data3, data.paddingFromData);

    if(data.textured)
        layer.setTextureCoordinates(data7, {0.25f, 0.5f, 37.0f}, {0.5f, 0.125f});

    if(data.styleCount < 5 && data.dynamicStyleCount) {
        /* Dynamic style 2 is style 4, which is used by data3 (so the same case
           as with padding from non-dynamic style or from data) */
        CORRADE_COMPARE(data.styleCount + 2, 4);
        layer.setDynamicStyle(2, BaseLayerStyleUniform{}, data.paddingFromStyle);
    }

    Vector2 nodeOffsets[16];
    Vector2 nodeSizes[16];
    UnsignedByte nodesEnabledData[2]{};
    Containers::MutableBitArrayView nodesEnabled{nodesEnabledData, 0, 16};
    nodeOffsets[6] = data.node6Offset;
    nodeSizes[6] = data.node6Size;
    nodeOffsets[15] = {3.0f, 4.0f};
    nodeSizes[15] = {20.0f, 5.0f};
    nodesEnabled.set(15);

    /* An empty update should generate an empty draw list */
    if(data.emptyUpdate) {
        layer.update(data.states, {}, {}, {}, nodeOffsets, nodeSizes, nodesEnabled, {}, {}, {}, {});
        CORRADE_VERIFY(data.expectIndexDataUpdated);
        CORRADE_COMPARE_AS(layer.stateData().indices,
            Containers::ArrayView<const UnsignedInt>{},
            TestSuite::Compare::Container);
        return;
    }

    /* Just the filled subset is getting updated, and just what was selected in
       states */
    UnsignedInt dataIds[]{9, 7, 3};
    layer.update(data.states, dataIds, {}, {}, nodeOffsets, nodeSizes, nodesEnabled, {}, {}, {}, {});

    /* If nothing is to be done, we got nothing to check. Capture the test
       function name at least in that case. */
    if(!data.expectIndexDataUpdated && !data.expectVertexDataUpdated)
        CORRADE_VERIFY(true);

    if(data.expectIndexDataUpdated) {
        /* The indices should be filled just for the three items */
        CORRADE_COMPARE_AS(layer.stateData().indices, Containers::arrayView<UnsignedInt>({
            9*4 + 0, 9*4 + 2, 9*4 + 1, 9*4 + 2, 9*4 + 3, 9*4 + 1, /* quad 9 */
            7*4 + 0, 7*4 + 2, 7*4 + 1, 7*4 + 2, 7*4 + 3, 7*4 + 1, /* quad 7 */
            3*4 + 0, 3*4 + 2, 3*4 + 1, 3*4 + 2, 3*4 + 3, 3*4 + 1, /* quad 3 */
        }), TestSuite::Compare::Container);
    }

    if(data.expectVertexDataUpdated) {
        /* Depending on whether texturing is enabled the vertex data contain a
           different type. Make a view on the common type prefix. */
        std::size_t typeSize = data.textured ?
            sizeof(Implementation::BaseLayerTexturedVertex) :
            sizeof(Implementation::BaseLayerVertex);
        Containers::StridedArrayView1D<const Implementation::BaseLayerVertex> vertices{
            layer.stateData().vertices,
            reinterpret_cast<const Implementation::BaseLayerVertex*>(layer.stateData().vertices.data()),
            layer.stateData().vertices.size()/typeSize,
            std::ptrdiff_t(typeSize)};
        CORRADE_COMPARE(vertices.size(), 10*4);

        /* The vertices are there for all data, but only the actually used are
           filled */
        for(std::size_t i = 0; i != 4; ++i) {
            CORRADE_ITERATION(i);
            CORRADE_COMPARE(vertices[3*4 + i].color, 0xff3366_rgbf);
            CORRADE_COMPARE(vertices[3*4 + i].outlineWidth, (Vector4{1.0f, 2.0f, 3.0f, 4.0f}));
            /* Created with style 4, which if not dynamic is transitioned to 2
               as the node is disabled, which is mapped to uniform 0. If
               dynamic, it's implicitly `uniformCount + (id - styleCount)`,
               thus 5. */
            if(data.styleCount == 5)
                CORRADE_COMPARE(vertices[3*4 + i].styleUniform, 0);
            else if(data.styleCount == 2)
                CORRADE_COMPARE(vertices[3*4 + i].styleUniform, 5);
            else CORRADE_INTERNAL_ASSERT_UNREACHABLE();

            CORRADE_COMPARE(vertices[7*4 + i].color, 0x112233_rgbf);
            CORRADE_COMPARE(vertices[7*4 + i].outlineWidth, Vector4{2.0f});
            /* Created with style 1, which is mapped to uniform 2 */
            CORRADE_COMPARE(vertices[7*4 + i].styleUniform, 2);

            CORRADE_COMPARE(vertices[9*4 + i].color, 0x663399_rgbf);
            CORRADE_COMPARE(vertices[9*4 + i].outlineWidth, (Vector4{3.0f, 2.0f, 1.0f, 4.0f}));
            /* Created with style 3, which if not dynamic is mapped to uniform
               1. If dynamic, it's implicitly `uniformCount + (id - styleCount)`,
               thus 4. */
            if(data.styleCount == 5)
                CORRADE_COMPARE(vertices[9*4 + i].styleUniform, 1);
            else if(data.styleCount == 2)
                CORRADE_COMPARE(vertices[9*4 + i].styleUniform, 4);
            else CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        }

        Containers::StridedArrayView1D<const Vector2> positions = vertices.slice(&Implementation::BaseLayerVertex::position);
        Containers::StridedArrayView1D<const Vector2> centerDistances = vertices.slice(&Implementation::BaseLayerVertex::centerDistance);

        /* Data 3 is attached to node 6 */
        CORRADE_COMPARE_AS(positions.sliceSize(3*4, 4), Containers::arrayView<Vector2>({
            { 1.0f,  2.0f},
            {11.0f,  2.0f},
            { 1.0f, 17.0f},
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
                { 3.0f, 4.0f},
                {23.0f, 4.0f},
                { 3.0f, 9.0f},
                {23.0f, 9.0f},
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(centerDistances.sliceSize(i*4, 4), Containers::arrayView<Vector2>({
                {-10.0f, -2.5f},
                { 10.0f, -2.5f},
                {-10.0f,  2.5f},
                { 10.0f,  2.5f},
            }), TestSuite::Compare::Container);
        }

        /* If textured, data 7 has texture coordinates set, the other two have
           the default. The coordinates are Y-flipped compared to positions --
           positions are Y down, while textures are with the Y up convention
           matching GL. */
        /** @todo which may get annoying with non-GL renderers that don't
            Y-flip the projection, reconsider? */
        if(data.textured) {
            Containers::StridedArrayView1D<const Vector3> textureCoordinates = Containers::arrayCast<const Implementation::BaseLayerTexturedVertex>(vertices).slice(&Implementation::BaseLayerTexturedVertex::textureCoordinates);

            CORRADE_COMPARE_AS(textureCoordinates.sliceSize(7*4, 4), Containers::arrayView<Vector3>({
                {0.25f, 0.625f, 37.0f},
                {0.75f, 0.625f, 37.0f},
                {0.25f, 0.5f, 37.0f},
                {0.75f, 0.5f, 37.0f},
            }), TestSuite::Compare::Container);

            for(std::size_t i: {3, 9}) {
                CORRADE_COMPARE_AS(textureCoordinates.sliceSize(i*4, 4), Containers::arrayView<Vector3>({
                    {0.0f, 1.0f, 0.0f},
                    {1.0f, 1.0f, 0.0f},
                    {0.0f, 0.0f, 0.0f},
                    {1.0f, 0.0f, 0.0f},
                }), TestSuite::Compare::Container);
            }
        }
    }
}

void BaseLayerTest::updateNoStyleSet() {
    auto&& data = UpdateNoStyleSetData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    CORRADE_SKIP_IF_NO_ASSERT();

    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(const Configuration& configuration): BaseLayer::Shared{configuration} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } shared{BaseLayer::Shared::Configuration{1}
        /* The check should work correctly even with dynamic styles, where
           different state gets filled */
        .setDynamicStyleCount(data.dynamicStyleCount)
    };

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    std::ostringstream out;
    Error redirectError{&out};
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(out.str(), "Whee::BaseLayer::update(): no style data was set\n");
}

void BaseLayerTest::sharedNeedsUpdateStatePropagatedToLayers() {
    auto&& data = SharedNeedsUpdateStatePropagatedToLayersData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct LayerShared: BaseLayer::Shared {
        explicit LayerShared(const Configuration& configuration): BaseLayer::Shared{configuration} {}

        void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
    } shared{BaseLayer::Shared::Configuration{1}
        .setDynamicStyleCount(data.dynamicStyleCount)
    };

    struct Layer: BaseLayer {
        explicit Layer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
    };

    /* Initially no state is set */
    Layer layer1{layerHandle(0, 1), shared};
    Layer layer2{layerHandle(0, 1), shared};
    Layer layer3{layerHandle(0, 1), shared};
    CORRADE_COMPARE(layer1.state(), LayerStates{});
    CORRADE_COMPARE(layer2.state(), LayerStates{});
    CORRADE_COMPARE(layer3.state(), LayerStates{});

    /* Explicitly set a non-trivial state on some of the layers */
    layer1.setNeedsUpdate(LayerState::NeedsCommonDataUpdate);
    layer3.setNeedsUpdate(LayerState::NeedsSharedDataUpdate);

    /* Calling setStyle() sets LayerState::Needs*DataUpdate on all layers */
    shared.setStyle(BaseLayerCommonStyleUniform{},
        {BaseLayerStyleUniform{}},
        {});
    CORRADE_COMPARE(layer1.state(), LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate|data.extraState);
    CORRADE_COMPARE(layer2.state(), LayerState::NeedsDataUpdate|data.extraState);
    CORRADE_COMPARE(layer3.state(), LayerState::NeedsDataUpdate|LayerState::NeedsSharedDataUpdate|data.extraState);

    /* Updating one doesn't cause the flag to be reset on others */
    layer2.update(LayerState::NeedsDataUpdate|data.extraState, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer1.state(), LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate|data.extraState);
    CORRADE_COMPARE(layer2.state(), LayerStates{});
    CORRADE_COMPARE(layer3.state(), LayerState::NeedsDataUpdate|LayerState::NeedsSharedDataUpdate|data.extraState);

    /* Updating another still doesn't */
    layer1.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer1.state(), LayerState::NeedsCommonDataUpdate);
    CORRADE_COMPARE(layer2.state(), LayerStates{});
    CORRADE_COMPARE(layer3.state(), LayerState::NeedsDataUpdate|LayerState::NeedsSharedDataUpdate|data.extraState);

    /* Calling setStyle() again sets LayerState::Needs*DataUpdate again, even
       if the data may be the same, as checking differences would be
       unnecessarily expensive compared to just doing the update always */
    shared.setStyle(BaseLayerCommonStyleUniform{},
        {BaseLayerStyleUniform{}},
        {});
    CORRADE_COMPARE(layer1.state(), LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate|data.extraState);
    CORRADE_COMPARE(layer2.state(), LayerState::NeedsDataUpdate|data.extraState);
    CORRADE_COMPARE(layer3.state(), LayerState::NeedsDataUpdate|LayerState::NeedsSharedDataUpdate|data.extraState);

    /* Creating a new layer with the shared state that had setStyle() called a
       few times doesn't mark it as needing an update because there's no data
       that would need it yet and the layer should do all other
       shared-state-dependent setup during construction already. For dynamic
       styles it'll perform the upload on the first update() regardless on the
       LayerState. */
    Layer layer4{layerHandle(0, 1), shared};
    CORRADE_COMPARE(layer4.state(), LayerStates{});

    /* But calling setStyle() next time will */
    shared.setStyle(BaseLayerCommonStyleUniform{},
        {BaseLayerStyleUniform{}},
        {});
    CORRADE_COMPARE(layer1.state(), LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate|data.extraState);
    CORRADE_COMPARE(layer2.state(), LayerState::NeedsDataUpdate|data.extraState);
    CORRADE_COMPARE(layer3.state(), LayerState::NeedsDataUpdate|LayerState::NeedsSharedDataUpdate|data.extraState);
    CORRADE_COMPARE(layer4.state(), LayerState::NeedsDataUpdate|data.extraState);

    /* Updating again resets just one */
    layer3.update(LayerState::NeedsDataUpdate|data.extraState, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer1.state(), LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate|data.extraState);
    CORRADE_COMPARE(layer2.state(), LayerState::NeedsDataUpdate|data.extraState);
    CORRADE_COMPARE(layer3.state(), LayerState::NeedsSharedDataUpdate);
    CORRADE_COMPARE(layer4.state(), LayerState::NeedsDataUpdate|data.extraState);

    /* Calling the AbstractVisualLayer setStyleTransition() should still cause
       LayerState to be updated as well, i.e. the class should correctly
       propagate to the parent doState() as well */
    shared.setStyleTransition(
        nullptr,
        nullptr,
        [](UnsignedInt a) { return a + 1; });
    CORRADE_COMPARE(layer1.state(), LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate|data.extraState);
    CORRADE_COMPARE(layer2.state(), LayerState::NeedsDataUpdate|data.extraState);
    /* This one has NeedsDataUpdate set again, not the extraState though as
       that comes only from setStyle() depending on dynamic styles being
       present */
    CORRADE_COMPARE(layer3.state(), LayerState::NeedsDataUpdate|LayerState::NeedsSharedDataUpdate);
    CORRADE_COMPARE(layer4.state(), LayerState::NeedsDataUpdate|data.extraState);
}

}}}}

CORRADE_TEST_MAIN(Magnum::Whee::Test::BaseLayerTest)
