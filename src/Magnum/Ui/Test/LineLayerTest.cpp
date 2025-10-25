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

#include <new>
#include <Corrade/Containers/Function.h> /* for debugIntegration() */
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/String.h>
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/Container.h>
#include <Corrade/TestSuite/Compare/Numeric.h>
#include <Corrade/TestSuite/Compare/String.h>
#include <Corrade/Utility/Format.h>

#include "Magnum/Ui/AbstractUserInterface.h" /* for debugIntegration() */
#include "Magnum/Ui/DebugLayer.h" /* for debugIntegration() */
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/LineLayer.h"
#include "Magnum/Ui/Implementation/lineLayerState.h"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct LineLayerTest: TestSuite::Tester {
    explicit LineLayerTest();

    template<class T> void styleUniformSizeAlignment();

    void styleUniformCommonConstructDefault();
    void styleUniformCommonConstruct();
    void styleUniformCommonConstructNoInit();
    void styleUniformCommonConstructCopy();
    void styleUniformCommonSetters();

    void styleUniformConstructDefault();
    void styleUniformConstruct();
    void styleUniformConstructNoInit();
    void styleUniformConstructCopy();
    void styleUniformSetters();

    /* Miter limit tests are copied verbatim between the Shaders and Ui
       libraries */
    void styleUniformMiterLimit();
    void styleUniformMiterLengthLimitInvalid();
    void styleUniformMiterAngleLimitInvalid();

    void debugCapStyle();
    void debugJoinStyle();
    void debugAlignment();
    void debugAlignmentPacked();

    void sharedConfigurationConstruct();
    void sharedConfigurationConstructSameStyleUniformCount();
    void sharedConfigurationConstructZeroStyleOrUniformCount();
    void sharedConfigurationConstructCopy();

    void sharedConfigurationSetters();

    void sharedConstruct();
    void sharedConstructNoCreate();
    void sharedConstructCopy();
    void sharedConstructMove();

    void sharedSetStyle();
    void sharedSetStyleImplicitPadding();
    void sharedSetStyleInvalidSize();
    void sharedSetStyleInvalidMapping();
    void sharedSetStyleImplicitMapping();
    void sharedSetStyleImplicitMappingImplicitPadding();
    void sharedSetStyleImplicitMappingInvalidSize();

    void construct();
    void constructCopy();
    void constructMove();

    /* remove() and setLine*() tested here as well */
    template<class T> void createRemoveSet();
    void createRemoveHandleRecycle();
    void createSetIndicesNeighbors();
    void createSetStripIndicesNeighbors();
    void createSetLoopIndicesNeighbors();
    void createStyleOutOfRange();

    void setColor();
    void setAlignment();
    void setPadding();

    void invalidHandle();
    void createSetInvalid();
    void createSetIndicesOutOfRange();

    void updateEmpty();
    void updateCleanDataOrder();
    void updateAlignment();
    void updatePadding();
    void updateNoStyleSet();

    void sharedNeedsUpdateStatePropagatedToLayers();

    void debugIntegration();
    void debugIntegrationNoCallback();
    void debugIntegrationLambdaStyleName();
};

using namespace Math::Literals;

const struct {
    const char* name;
    Float limit;
    const char* message;
} StyleUniformMiterLengthLimitInvalidData[]{
    {"too short", 0.9997f,
        "expected a finite value greater than or equal to 1, got 0.9997"},
    {"too long", Constants::inf(),
        "expected a finite value greater than or equal to 1, got inf"},
};

const struct {
    const char* name;
    Rad limit;
    const char* message;
} StyleUniformMiterAngleLimitInvalidData[]{
    {"too small", 0.0_degf,
        "expected a value greater than 0° and less than or equal to 180°, got 0°"},
    {"too large", 180.1_degf,
        "expected a value greater than 0° and less than or equal to 180°, got 180.1°"}
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
} CreateRemoveSetData[]{
    {"create",
        NodeHandle::Null, LayerState::NeedsDataUpdate, false},
    {"create and attach",
        nodeHandle(9872, 0xbeb), LayerState::NeedsNodeOffsetSizeUpdate|LayerState::NeedsAttachmentUpdate|LayerState::NeedsDataUpdate, false},
    {"LayerDataHandle overloads",
        NodeHandle::Null, LayerState::NeedsDataUpdate, true},
};

const struct {
    TestSuite::TestCaseDescriptionSourceLocation name;
    bool emptyUpdate;
    Vector2 node6Offset, node6Size;
    Vector4 paddingFromStyle;
    Vector4 paddingFromData;
    LayerStates states;
    bool expectIndexDataUpdated, expectVertexDataUpdated;
} UpdateCleanDataOrderData[]{
    {"empty update", true,
        {}, {}, {}, {},
        LayerState::NeedsDataUpdate, true, true},
    {"", false,
        {1.0f, 2.0f}, {10.0f, 15.0f}, {}, {},
        LayerState::NeedsDataUpdate, true, true},
    {"node offset/size update only", false,
        {1.0f, 2.0f}, {10.0f, 15.0f}, {}, {},
        LayerState::NeedsNodeOffsetSizeUpdate, false, true},
    {"node order update only", false,
        {1.0f, 2.0f}, {10.0f, 15.0f}, {}, {},
        LayerState::NeedsNodeOrderUpdate, true, false},
    {"node enabled update only", false,
        {1.0f, 2.0f}, {10.0f, 15.0f}, {}, {},
        LayerState::NeedsNodeEnabledUpdate, false, true},
    /* Cannot use NeedsNodeOpacityUpdate alone because then AbstractVisualLayer
       doUpdate() doesn't fill in calculated styles, leading to OOB errors. */
    /** @todo Which ultimately means this doesn't correctly test that the
        implementation correctly handles the NeedsNodeOpacityUpdate flag alone
        -- what can I do differently to test that? */
    {"node enabled + opacity update only", false,
        {1.0f, 2.0f}, {10.0f, 15.0f}, {}, {},
        LayerState::NeedsNodeEnabledUpdate|LayerState::NeedsNodeOpacityUpdate, false, true},
    /* These two shouldn't cause anything to be done in update(), and also no
       crashes */
    {"shared data update only", false,
        {1.0f, 2.0f}, {10.0f, 15.0f}, {}, {},
        LayerState::NeedsSharedDataUpdate, false, false},
    {"common data update only", false,
        {1.0f, 2.0f}, {10.0f, 15.0f}, {}, {},
        LayerState::NeedsCommonDataUpdate, false, false},
    {"padding from style", false,
        {-1.0f, 1.5f}, {13.0f, 17.0f},
        {2.0f, 0.5f, 1.0f, 1.5f}, {},
        LayerState::NeedsDataUpdate, true, true},
    {"padding from data", false,
        {-1.0f, 1.5f}, {13.0f, 17.0f},
        {}, {2.0f, 0.5f, 1.0f, 1.5f},
        LayerState::NeedsDataUpdate, true, true},
};

const struct {
    const char* name;
    LineAlignment alignment;
    /* Node offset is {50.5, 20.5}, size {200.8, 100.4} */
    Vector2 offset;
} UpdateAlignmentPaddingData[]{
    {"default middle center",LineAlignment::MiddleCenter,
        {50.5f + 100.4f, 20.5f + 50.2f}},
    {"middle left",LineAlignment::MiddleLeft,
        {50.5f, 20.5f + 50.2f}},
    {"top center",LineAlignment::TopCenter,
        {50.5f + 100.4f, 20.5f}},
    {"bottom right",LineAlignment::BottomRight,
        {50.5f + 200.8f, 20.5f + 100.4f}},
};

const struct {
    const char* name;
    bool styleNames;
    /* Default color is all 1s, while padding is all 0s, so it's an Optional */
    Containers::Optional<Color4> color;
    Containers::Optional<LineAlignment> alignment;
    Vector4 padding;
    const char* expected;
} DebugIntegrationData[]{
    {"",
        false, {}, {}, {},
        "Node {0x1, 0x1}\n"
        "  Data {0x6, 0x2} from layer {0x0, 0x3} with style 3"},
    {"style name mapping",
        true, {}, {}, {},
        "Node {0x1, 0x1}\n"
        "  Data {0x6, 0x2} from layer {0x0, 0x3} with style StyleName (3)"},
    {"custom color",
        false, 0x3bd26799_rgbaf, {}, {},
        "Node {0x1, 0x1}\n"
        "  Data {0x6, 0x2} from layer {0x0, 0x3} with style 3\n"
        "    Color: #3bd26799"},
    {"custom alignment",
        false, {}, LineAlignment::MiddleRight, {},
        "Node {0x1, 0x1}\n"
        "  Data {0x6, 0x2} from layer {0x0, 0x3} with style 3\n"
        "    Alignment: MiddleRight"},
    {"custom padding",
        false, {}, {}, {0.5f, 2.0f, 1.5f, 1.0f},
        "Node {0x1, 0x1}\n"
        "  Data {0x6, 0x2} from layer {0x0, 0x3} with style 3\n"
        "    Padding: {0.5, 2, 1.5, 1}"},
    {"custom padding, all edges same",
        false, {}, {}, Vector4{2.5f},
        "Node {0x1, 0x1}\n"
        "  Data {0x6, 0x2} from layer {0x0, 0x3} with style 3\n"
        "    Padding: 2.5"},
    {"custom color + padding",
        false, 0x3bd26799_rgbaf, {}, {0.5f, 2.0f, 1.5f, 1.0f},
        "Node {0x1, 0x1}\n"
        "  Data {0x6, 0x2} from layer {0x0, 0x3} with style 3\n"
        "    Color: #3bd26799\n"
        "    Padding: {0.5, 2, 1.5, 1}"},
    {"style name mapping, custom color + alignment + padding",
        true, 0x3bd26799_rgbaf, LineAlignment::MiddleRight, {0.5f, 2.0f, 1.5f, 1.0f},
        "Node {0x1, 0x1}\n"
        "  Data {0x6, 0x2} from layer {0x0, 0x3} with style StyleName (3)\n"
        "    Color: #3bd26799\n"
        "    Alignment: MiddleRight\n"
        "    Padding: {0.5, 2, 1.5, 1}"},
    /* The last case here is used in debugIntegrationNoCallback() to verify
       output w/o a callback and for visual color verification, it's expected
       to be the most complete, executing all coloring code paths */
};

LineLayerTest::LineLayerTest() {
    addTests({&LineLayerTest::styleUniformSizeAlignment<LineLayerCommonStyleUniform>,
              &LineLayerTest::styleUniformSizeAlignment<LineLayerStyleUniform>,

              &LineLayerTest::styleUniformCommonConstructDefault,
              &LineLayerTest::styleUniformCommonConstruct,
              &LineLayerTest::styleUniformCommonConstructNoInit,
              &LineLayerTest::styleUniformCommonConstructCopy,
              &LineLayerTest::styleUniformCommonSetters,

              &LineLayerTest::styleUniformConstructDefault,
              &LineLayerTest::styleUniformConstruct,
              &LineLayerTest::styleUniformConstructNoInit,
              &LineLayerTest::styleUniformConstructCopy,
              &LineLayerTest::styleUniformSetters,

              &LineLayerTest::styleUniformMiterLimit});

    addInstancedTests({&LineLayerTest::styleUniformMiterLengthLimitInvalid},
        Containers::arraySize( StyleUniformMiterLengthLimitInvalidData));

    addInstancedTests({&LineLayerTest::styleUniformMiterAngleLimitInvalid},
        Containers::arraySize( StyleUniformMiterAngleLimitInvalidData));

    addTests({&LineLayerTest::debugCapStyle,
              &LineLayerTest::debugJoinStyle,
              &LineLayerTest::debugAlignment,
              &LineLayerTest::debugAlignmentPacked,

              &LineLayerTest::sharedConfigurationConstruct,
              &LineLayerTest::sharedConfigurationConstructSameStyleUniformCount,
              &LineLayerTest::sharedConfigurationConstructZeroStyleOrUniformCount,
              &LineLayerTest::sharedConfigurationConstructCopy,

              &LineLayerTest::sharedConfigurationSetters,

              &LineLayerTest::sharedConstruct,
              &LineLayerTest::sharedConstructNoCreate,
              &LineLayerTest::sharedConstructCopy,
              &LineLayerTest::sharedConstructMove,

              &LineLayerTest::sharedSetStyle,
              &LineLayerTest::sharedSetStyleImplicitPadding,
              &LineLayerTest::sharedSetStyleInvalidSize,
              &LineLayerTest::sharedSetStyleInvalidMapping,
              &LineLayerTest::sharedSetStyleImplicitMapping,
              &LineLayerTest::sharedSetStyleImplicitMappingImplicitPadding,
              &LineLayerTest::sharedSetStyleImplicitMappingInvalidSize,

              &LineLayerTest::construct,
              &LineLayerTest::constructCopy,
              &LineLayerTest::constructMove});

    addInstancedTests<LineLayerTest>({&LineLayerTest::createRemoveSet<UnsignedInt>,
                                      &LineLayerTest::createRemoveSet<Enum>},
        Containers::arraySize(CreateRemoveSetData));

    addTests({&LineLayerTest::createRemoveHandleRecycle,
              &LineLayerTest::createSetIndicesNeighbors,
              &LineLayerTest::createSetStripIndicesNeighbors,
              &LineLayerTest::createSetLoopIndicesNeighbors,
              &LineLayerTest::createStyleOutOfRange,

              &LineLayerTest::setColor,
              &LineLayerTest::setAlignment,
              &LineLayerTest::setPadding,

              &LineLayerTest::invalidHandle,
              &LineLayerTest::createSetInvalid,
              &LineLayerTest::createSetIndicesOutOfRange,

              &LineLayerTest::updateEmpty});

    addInstancedTests({&LineLayerTest::updateCleanDataOrder},
        Containers::arraySize(UpdateCleanDataOrderData));

    addInstancedTests({&LineLayerTest::updateAlignment,
                       &LineLayerTest::updatePadding},
        Containers::arraySize(UpdateAlignmentPaddingData));

    addTests({&LineLayerTest::updateNoStyleSet,

              &LineLayerTest::sharedNeedsUpdateStatePropagatedToLayers});

    addInstancedTests({&LineLayerTest::debugIntegration},
        Containers::arraySize(DebugIntegrationData));

    addTests({&LineLayerTest::debugIntegrationNoCallback,
              &LineLayerTest::debugIntegrationLambdaStyleName});
}

template<class> struct StyleTraits;
template<> struct StyleTraits<LineLayerCommonStyleUniform> {
    static const char* name() { return "LineLayerCommonStyleUniform"; }
};
template<> struct StyleTraits<LineLayerStyleUniform> {
    static const char* name() { return "LineLayerStyleUniform"; }
};

template<class T> void LineLayerTest::styleUniformSizeAlignment() {
    setTestCaseTemplateName(StyleTraits<T>::name());

    CORRADE_FAIL_IF(sizeof(T) % sizeof(Vector4) != 0, sizeof(T) << "is not a multiple of vec4 for UBO alignment.");

    /* 48-byte structures are fine, we'll align them to 768 bytes and not
       256, but warn about that */
    CORRADE_FAIL_IF(768 % sizeof(T) != 0, sizeof(T) << "can't fit exactly into 768-byte UBO alignment.");
    if(256 % sizeof(T) != 0)
        CORRADE_WARN(sizeof(T) << "can't fit exactly into 256-byte UBO alignment, only 768.");

    CORRADE_COMPARE(alignof(T), 4);
}

void LineLayerTest::styleUniformCommonConstructDefault() {
    LineLayerCommonStyleUniform a;
    LineLayerCommonStyleUniform b{DefaultInit};
    CORRADE_COMPARE(a.smoothness, 0.0f);
    CORRADE_COMPARE(b.smoothness, 0.0f);

    constexpr LineLayerCommonStyleUniform ca;
    constexpr LineLayerCommonStyleUniform cb{DefaultInit};
    CORRADE_COMPARE(ca.smoothness, 0.0f);
    CORRADE_COMPARE(cb.smoothness, 0.0f);

    CORRADE_VERIFY(std::is_nothrow_default_constructible<LineLayerCommonStyleUniform>::value);
    CORRADE_VERIFY(std::is_nothrow_constructible<LineLayerCommonStyleUniform, DefaultInitT>::value);

    /* Implicit construction is not allowed */
    CORRADE_VERIFY(!std::is_convertible<DefaultInitT, LineLayerCommonStyleUniform>::value);
}

void LineLayerTest::styleUniformCommonConstruct() {
    LineLayerCommonStyleUniform a{3.0f};
    CORRADE_COMPARE(a.smoothness, 3.0f);

    constexpr LineLayerCommonStyleUniform ca{3.0f};
    CORRADE_COMPARE(ca.smoothness, 3.0f);
}

void LineLayerTest::styleUniformCommonConstructNoInit() {
    /* Testing only some fields, should be enough */
    LineLayerCommonStyleUniform a;
    a.smoothness = 3.0f;

    new(&a) LineLayerCommonStyleUniform{NoInit};
    {
        /* Explicitly check we're not on Clang because certain Clang-based IDEs
           inherit __GNUC__ if GCC is used instead of leaving it at 4 like
           Clang itself does */
        #if defined(CORRADE_TARGET_GCC) && !defined(CORRADE_TARGET_CLANG) && __GNUC__*100 + __GNUC_MINOR__ >= 601 && __OPTIMIZE__
        CORRADE_EXPECT_FAIL("GCC 6.1+ misoptimizes and overwrites the value.");
        #endif
        CORRADE_COMPARE(a.smoothness, 3.0f);
    }

    /* Implicit construction is not allowed */
    CORRADE_VERIFY(!std::is_convertible<NoInitT, LineLayerCommonStyleUniform>::value);
}

void LineLayerTest::styleUniformCommonConstructCopy() {
    /* Testing only some fields, should be enough */
    LineLayerCommonStyleUniform a;
    a.smoothness = 3.0f;

    LineLayerCommonStyleUniform b = a;
    CORRADE_COMPARE(b.smoothness, 3.0f);

    #ifndef CORRADE_NO_STD_IS_TRIVIALLY_TRAITS
    CORRADE_VERIFY(std::is_trivially_copy_constructible<LineLayerCommonStyleUniform>::value);
    CORRADE_VERIFY(std::is_trivially_copy_assignable<LineLayerCommonStyleUniform>::value);
    #endif
}

void LineLayerTest::styleUniformCommonSetters() {
    LineLayerCommonStyleUniform a;
    a.setSmoothness(34.0f);
    CORRADE_COMPARE(a.smoothness, 34.0f);
}

void LineLayerTest::styleUniformConstructDefault() {
    LineLayerStyleUniform a;
    LineLayerStyleUniform b{DefaultInit};
    CORRADE_COMPARE(a.color, 0xffffffff_srgbaf);
    CORRADE_COMPARE(b.color, 0xffffffff_srgbaf);
    CORRADE_COMPARE(a.width, 1.0f);
    CORRADE_COMPARE(b.width, 1.0f);
    CORRADE_COMPARE(a.smoothness, 0.0f);
    CORRADE_COMPARE(b.smoothness, 0.0f);
    CORRADE_COMPARE(a.miterLimit, 0.875f);
    CORRADE_COMPARE(b.miterLimit, 0.875f);

    constexpr LineLayerStyleUniform ca;
    constexpr LineLayerStyleUniform cb{DefaultInit};
    CORRADE_COMPARE(ca.color, 0xffffffff_srgbaf);
    CORRADE_COMPARE(cb.color, 0xffffffff_srgbaf);
    CORRADE_COMPARE(ca.width, 1.0f);
    CORRADE_COMPARE(cb.width, 1.0f);
    CORRADE_COMPARE(ca.smoothness, 0.0f);
    CORRADE_COMPARE(cb.smoothness, 0.0f);
    CORRADE_COMPARE(ca.miterLimit, 0.875f);
    CORRADE_COMPARE(cb.miterLimit, 0.875f);

    CORRADE_VERIFY(std::is_nothrow_default_constructible<LineLayerStyleUniform>::value);
    CORRADE_VERIFY(std::is_nothrow_constructible<LineLayerStyleUniform, DefaultInitT>::value);

    /* Implicit construction is not allowed */
    CORRADE_VERIFY(!std::is_convertible<DefaultInitT, LineLayerStyleUniform>::value);
}

void LineLayerTest::styleUniformConstruct() {
    LineLayerStyleUniform a{0xff336699_rgbaf, 3.0f, 15.0f, 3.7654f};
    CORRADE_COMPARE(a.color, 0xff336699_rgbaf);
    CORRADE_COMPARE(a.width, 3.0f);
    CORRADE_COMPARE(a.smoothness, 15.0f);
    CORRADE_COMPARE(a.miterLimit, 3.7654f);

    constexpr LineLayerStyleUniform ca{0xff336699_rgbaf, 3.0f, 15.0f, 3.7654f};
    CORRADE_COMPARE(ca.color, 0xff336699_rgbaf);
    CORRADE_COMPARE(ca.width, 3.0f);
    CORRADE_COMPARE(ca.smoothness, 15.0f);
    CORRADE_COMPARE(ca.miterLimit, 3.7654f);
}

void LineLayerTest::styleUniformConstructNoInit() {
    /* Testing only some fields, should be enough */
    LineLayerStyleUniform a;
    a.color = 0xff336699_rgbaf;
    a.smoothness = 3.0f;

    new(&a) LineLayerStyleUniform{NoInit};
    {
        /* Explicitly check we're not on Clang because certain Clang-based IDEs
           inherit __GNUC__ if GCC is used instead of leaving it at 4 like
           Clang itself does */
        #if defined(CORRADE_TARGET_GCC) && !defined(CORRADE_TARGET_CLANG) && __GNUC__*100 + __GNUC_MINOR__ >= 601 && __OPTIMIZE__
        CORRADE_EXPECT_FAIL("GCC 6.1+ misoptimizes and overwrites the value.");
        #endif
        CORRADE_COMPARE(a.color, 0xff336699_rgbaf);
        CORRADE_COMPARE(a.smoothness, 3.0f);
    }

    /* Implicit construction is not allowed */
    CORRADE_VERIFY(!std::is_convertible<NoInitT, LineLayerStyleUniform>::value);
}

void LineLayerTest::styleUniformConstructCopy() {
    /* Testing only some fields, should be enough */
    LineLayerStyleUniform a;
    a.color = 0xff336699_rgbaf;
    a.smoothness = 3.0f;

    LineLayerStyleUniform b = a;
    CORRADE_COMPARE(b.color, 0xff336699_rgbaf);
    CORRADE_COMPARE(b.smoothness, 3.0f);

    #ifndef CORRADE_NO_STD_IS_TRIVIALLY_TRAITS
    CORRADE_VERIFY(std::is_trivially_copy_constructible<LineLayerStyleUniform>::value);
    CORRADE_VERIFY(std::is_trivially_copy_assignable<LineLayerStyleUniform>::value);
    #endif
}

void LineLayerTest::styleUniformSetters() {
    LineLayerStyleUniform a;
    a.setColor(0xff336699_rgbaf)
     .setWidth(3.0f)
     .setSmoothness(15.0f)
     .setMiterLimit(3.7654f);
    CORRADE_COMPARE(a.color, 0xff336699_rgbaf);
    CORRADE_COMPARE(a.width, 3.0f);
    CORRADE_COMPARE(a.smoothness, 15.0f);
    CORRADE_COMPARE(a.miterLimit, 3.7654f);
}

void LineLayerTest::styleUniformMiterLimit() {
    LineLayerStyleUniform a;

    /* Verifying documented relation of the default to angle/length */
    CORRADE_COMPARE(a.miterLimit, 0.875f);
    a.setMiterLengthLimit(4.0f);
    CORRADE_COMPARE(a.miterLimit, 0.875f);
    a.setMiterAngleLimit(28.955_degf);
    CORRADE_COMPARE(a.miterLimit, 0.875f);

    a.setMiterLengthLimit(25.0f);
    CORRADE_COMPARE(a.miterLimit, 0.9968f);

    a.setMiterAngleLimit(35.0_degf);
    CORRADE_COMPARE(a.miterLimit, 0.819152f);
}

void LineLayerTest::styleUniformMiterLengthLimitInvalid() {
    auto&& data = StyleUniformMiterLengthLimitInvalidData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    CORRADE_SKIP_IF_NO_ASSERT();

    LineLayerStyleUniform a;

    Containers::String out;
    Error redirectError{&out};
    a.setMiterLengthLimit(data.limit);
    CORRADE_COMPARE(out, Utility::format("Ui::LineLayerStyleUniform::setMiterLengthLimit(): {}\n", data.message));
}

void LineLayerTest::styleUniformMiterAngleLimitInvalid() {
    auto&& data = StyleUniformMiterAngleLimitInvalidData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    CORRADE_SKIP_IF_NO_ASSERT();

    LineLayerStyleUniform a;

    Containers::String out;
    Error redirectError{&out};
    a.setMiterAngleLimit(data.limit);
    CORRADE_COMPARE(out, Utility::format("Ui::LineLayerStyleUniform::setMiterAngleLimit(): {}\n", data.message));
}

void LineLayerTest::debugCapStyle() {
    Containers::String out;
    Debug{&out} << LineCapStyle::Square << LineCapStyle(0xb0);
    CORRADE_COMPARE(out, "Ui::LineCapStyle::Square Ui::LineCapStyle(0xb0)\n");
}

void LineLayerTest::debugJoinStyle() {
    Containers::String out;
    Debug{&out} << LineJoinStyle::Bevel << LineJoinStyle(0xb0);
    CORRADE_COMPARE(out, "Ui::LineJoinStyle::Bevel Ui::LineJoinStyle(0xb0)\n");
}

void LineLayerTest::debugAlignment() {
    Containers::String out;
    Debug{&out} << LineAlignment::MiddleRight << LineAlignment(0xb0);
    CORRADE_COMPARE(out, "Ui::LineAlignment::MiddleRight Ui::LineAlignment(0xb0)\n");
}

void LineLayerTest::debugAlignmentPacked() {
    Containers::String out;
    /* Last is not packed, ones before should not make any flags persistent */
    Debug{&out} << Debug::packed << LineAlignment::MiddleRight << Debug::packed << LineAlignment(0xb0) << LineAlignment::BottomCenter;
    CORRADE_COMPARE(out, "MiddleRight 0xb0 Ui::LineAlignment::BottomCenter\n");
}

void LineLayerTest::sharedConfigurationConstruct() {
    LineLayer::Shared::Configuration configuration{3, 5};
    CORRADE_COMPARE(configuration.styleUniformCount(), 3);
    CORRADE_COMPARE(configuration.styleCount(), 5);
}

void LineLayerTest::sharedConfigurationConstructSameStyleUniformCount() {
    LineLayer::Shared::Configuration configuration{3};
    CORRADE_COMPARE(configuration.styleUniformCount(), 3);
    CORRADE_COMPARE(configuration.styleCount(), 3);
}

void LineLayerTest::sharedConfigurationConstructZeroStyleOrUniformCount() {
    CORRADE_SKIP_IF_NO_ASSERT();

    Containers::String out;
    Error redirectError{&out};
    LineLayer::Shared::Configuration{0, 0};
    LineLayer::Shared::Configuration{0};
    LineLayer::Shared::Configuration{0, 4};
    LineLayer::Shared::Configuration{4, 0};
    CORRADE_COMPARE_AS(out,
        "Ui::LineLayer::Shared::Configuration: expected non-zero style uniform count\n"
        "Ui::LineLayer::Shared::Configuration: expected non-zero style uniform count\n"
        "Ui::LineLayer::Shared::Configuration: expected non-zero style uniform count\n"
        "Ui::LineLayer::Shared::Configuration: expected non-zero style count\n",
        TestSuite::Compare::String);
}

void LineLayerTest::sharedConfigurationConstructCopy() {
    /* Testing just some properties, it's an implicitly generated copy */
    LineLayer::Shared::Configuration a{3, 5};

    LineLayer::Shared::Configuration b = a;
    CORRADE_COMPARE(b.styleUniformCount(), 3);
    CORRADE_COMPARE(b.styleCount(), 5);

    LineLayer::Shared::Configuration c{7, 9};
    c = b;
    CORRADE_COMPARE(c.styleUniformCount(), 3);
    CORRADE_COMPARE(c.styleCount(), 5);

    #ifndef CORRADE_NO_STD_IS_TRIVIALLY_TRAITS
    CORRADE_VERIFY(std::is_trivially_copy_constructible<LineLayer::Shared::Configuration>::value);
    CORRADE_VERIFY(std::is_trivially_copy_assignable<LineLayer::Shared::Configuration>::value);
    #endif
}

void LineLayerTest::sharedConfigurationSetters() {
    LineLayer::Shared::Configuration configuration{3, 5};
    CORRADE_COMPARE(configuration.capStyle(), LineCapStyle::Square);
    CORRADE_COMPARE(configuration.joinStyle(), LineJoinStyle::Miter);

    configuration
        .setCapStyle(LineCapStyle::Butt)
        .setJoinStyle(LineJoinStyle::Bevel);
    CORRADE_COMPARE(configuration.capStyle(), LineCapStyle::Butt);
    CORRADE_COMPARE(configuration.joinStyle(), LineJoinStyle::Bevel);
}

void LineLayerTest::sharedConstruct() {
    struct Shared: LineLayer::Shared {
        explicit Shared(const Configuration& configuration): LineLayer::Shared{configuration} {}

        void doSetStyle(const LineLayerCommonStyleUniform&, Containers::ArrayView<const LineLayerStyleUniform>) override {}
    } shared{LineLayer::Shared::Configuration{3, 5}
        .setCapStyle(LineCapStyle::Butt)
        .setJoinStyle(LineJoinStyle::Bevel)
    };
    CORRADE_COMPARE(shared.styleUniformCount(), 3);
    CORRADE_COMPARE(shared.styleCount(), 5);
    CORRADE_COMPARE(shared.capStyle(), LineCapStyle::Butt);
    CORRADE_COMPARE(shared.joinStyle(), LineJoinStyle::Bevel);
}

void LineLayerTest::sharedConstructNoCreate() {
    struct Shared: LineLayer::Shared {
        explicit Shared(NoCreateT): LineLayer::Shared{NoCreate} {}

        void doSetStyle(const LineLayerCommonStyleUniform&, Containers::ArrayView<const LineLayerStyleUniform>) override {}
    } shared{NoCreate};

    /* Shouldn't crash */
    CORRADE_VERIFY(true);

    /* Implicit construction is not allowed */
    CORRADE_VERIFY(!std::is_convertible<NoCreateT, LineLayer::Shared>::value);
}

void LineLayerTest::sharedConstructCopy() {
    struct Shared: LineLayer::Shared {
        /* Clang says the constructor is unused otherwise. However I fear that
           without the constructor the type would be impossible to construct
           (and thus also to copy), leading to false positives in the trait
           check below */
        explicit CORRADE_UNUSED Shared(const Configuration& configuration): LineLayer::Shared{configuration} {}

        void doSetStyle(const LineLayerCommonStyleUniform&, const Containers::ArrayView<const LineLayerStyleUniform>) override {}
    };

    CORRADE_VERIFY(!std::is_copy_constructible<Shared>{});
    CORRADE_VERIFY(!std::is_copy_assignable<Shared>{});
}

void LineLayerTest::sharedConstructMove() {
    struct Shared: LineLayer::Shared {
        explicit Shared(const Configuration& configuration): LineLayer::Shared{configuration} {}

        void doSetStyle(const LineLayerCommonStyleUniform&, const Containers::ArrayView<const LineLayerStyleUniform>) override {}
    };

    Shared a{LineLayer::Shared::Configuration{3, 5}
        .setCapStyle(LineCapStyle::Butt)
        .setJoinStyle(LineJoinStyle::Bevel)};

    Shared b{Utility::move(a)};
    CORRADE_COMPARE(b.styleUniformCount(), 3);
    CORRADE_COMPARE(b.styleCount(), 5);
    CORRADE_COMPARE(b.capStyle(), LineCapStyle::Butt);
    CORRADE_COMPARE(b.joinStyle(), LineJoinStyle::Bevel);

    Shared c{LineLayer::Shared::Configuration{5, 7}};
    c = Utility::move(b);
    CORRADE_COMPARE(c.styleUniformCount(), 3);
    CORRADE_COMPARE(c.styleCount(), 5);
    CORRADE_COMPARE(c.capStyle(), LineCapStyle::Butt);
    CORRADE_COMPARE(c.joinStyle(), LineJoinStyle::Bevel);

    CORRADE_VERIFY(std::is_nothrow_move_constructible<Shared>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<Shared>::value);
}

void LineLayerTest::sharedSetStyle() {
    struct Shared: LineLayer::Shared {
        explicit Shared(const Configuration& configuration): LineLayer::Shared{configuration} {}
        State& state() { return static_cast<State&>(*_state); }

        void doSetStyle(const LineLayerCommonStyleUniform& commonUniform, Containers::ArrayView<const LineLayerStyleUniform> uniforms) override {
            CORRADE_COMPARE(commonUniform.smoothness, 3.14f);
            CORRADE_COMPARE(uniforms.size(), 3);
            CORRADE_COMPARE(uniforms[1].color, 0xc0ffee_rgbf);
            ++setStyleCalled;
        }

        Int setStyleCalled = 0;
    } shared{LineLayer::Shared::Configuration{3, 5}};

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    shared.setStyle(
        LineLayerCommonStyleUniform{}
            .setSmoothness(3.14f),
        {LineLayerStyleUniform{},
         LineLayerStyleUniform{}
            .setColor(0xc0ffee_rgbf),
         LineLayerStyleUniform{}},
        {2, 1, 0, 0, 1},
        {LineAlignment::MiddleCenter,
         LineAlignment::TopRight,
         LineAlignment::BottomLeft,
         LineAlignment::TopLeft,
         LineAlignment::MiddleCenter},
        {{1.0f, 2.0f, 3.0f, 4.0f},
         {4.0f, 3.0f, 2.0f, 1.0f},
         {2.0f, 1.0f, 4.0f, 3.0f},
         {1.0f, 3.0f, 2.0f, 4.0f},
         {4.0f, 1.0f, 3.0f, 2.0f}});
    CORRADE_COMPARE(shared.setStyleCalled, 1);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::LineLayerStyle::uniform), Containers::stridedArrayView({
        2u, 1u, 0u, 0u, 1u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::LineLayerStyle::alignment), Containers::stridedArrayView({
        LineAlignment::MiddleCenter,
        LineAlignment::TopRight,
        LineAlignment::BottomLeft,
        LineAlignment::TopLeft,
        LineAlignment::MiddleCenter
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::LineLayerStyle::padding), Containers::stridedArrayView({
        Vector4{1.0f, 2.0f, 3.0f, 4.0f},
        Vector4{4.0f, 3.0f, 2.0f, 1.0f},
        Vector4{2.0f, 1.0f, 4.0f, 3.0f},
        Vector4{1.0f, 3.0f, 2.0f, 4.0f},
        Vector4{4.0f, 1.0f, 3.0f, 2.0f}
    }), TestSuite::Compare::Container);
}

void LineLayerTest::sharedSetStyleImplicitPadding() {
    struct Shared: LineLayer::Shared {
        explicit Shared(const Configuration& configuration): LineLayer::Shared{configuration} {}
        State& state() { return static_cast<State&>(*_state); }

        void doSetStyle(const LineLayerCommonStyleUniform& commonUniform, Containers::ArrayView<const LineLayerStyleUniform> uniforms) override {
            CORRADE_COMPARE(commonUniform.smoothness, 3.14f);
            CORRADE_COMPARE(uniforms.size(), 3);
            CORRADE_COMPARE(uniforms[1].color, 0xc0ffee_rgbf);
            ++setStyleCalled;
        }

        Int setStyleCalled = 0;
    } shared{LineLayer::Shared::Configuration{3, 5}};

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    shared.setStyle(
        LineLayerCommonStyleUniform{}
            .setSmoothness(3.14f),
        {LineLayerStyleUniform{},
         LineLayerStyleUniform{}
            .setColor(0xc0ffee_rgbf),
         LineLayerStyleUniform{}},
        {2, 1, 0, 0, 1},
        {LineAlignment::MiddleCenter,
         LineAlignment::TopRight,
         LineAlignment::BottomLeft,
         LineAlignment::TopLeft,
         LineAlignment::MiddleCenter},
        {});
    CORRADE_COMPARE(shared.setStyleCalled, 1);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::LineLayerStyle::uniform), Containers::stridedArrayView({
        2u, 1u, 0u, 0u, 1u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::LineLayerStyle::alignment), Containers::stridedArrayView({
        LineAlignment::MiddleCenter,
        LineAlignment::TopRight,
        LineAlignment::BottomLeft,
        LineAlignment::TopLeft,
        LineAlignment::MiddleCenter
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::LineLayerStyle::padding), Containers::stridedArrayView({
        Vector4{},
        Vector4{},
        Vector4{},
        Vector4{},
        Vector4{}
    }), TestSuite::Compare::Container);

    /* Setting a style with implicit padding after a non-implicit padding was
       set should reset it back to zeros */
    shared.setStyle(
        LineLayerCommonStyleUniform{}
            .setSmoothness(3.14f),
        {LineLayerStyleUniform{},
         LineLayerStyleUniform{}
            .setColor(0xc0ffee_rgbf),
         LineLayerStyleUniform{}},
        {2, 1, 0, 0, 1},
        {LineAlignment::MiddleCenter,
         LineAlignment::TopRight,
         LineAlignment::BottomLeft,
         LineAlignment::TopLeft,
         LineAlignment::MiddleCenter},
        {{1.0f, 2.0f, 3.0f, 4.0f},
         {4.0f, 3.0f, 2.0f, 1.0f},
         {2.0f, 1.0f, 4.0f, 3.0f},
         {1.0f, 3.0f, 2.0f, 4.0f},
         {4.0f, 1.0f, 3.0f, 2.0f}});
    shared.setStyle(
        LineLayerCommonStyleUniform{}
            .setSmoothness(3.14f),
        {LineLayerStyleUniform{},
         LineLayerStyleUniform{}
            .setColor(0xc0ffee_rgbf),
         LineLayerStyleUniform{}},
        {2, 1, 0, 0, 1},
        {LineAlignment::MiddleCenter,
         LineAlignment::TopRight,
         LineAlignment::BottomLeft,
         LineAlignment::TopLeft,
         LineAlignment::MiddleCenter},
        {});
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::LineLayerStyle::padding), Containers::stridedArrayView({
        Vector4{},
        Vector4{},
        Vector4{},
        Vector4{},
        Vector4{}
    }), TestSuite::Compare::Container);
}

void LineLayerTest::sharedSetStyleInvalidSize() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Shared: LineLayer::Shared {
        explicit Shared(const Configuration& configuration): LineLayer::Shared{configuration} {}

        void doSetStyle(const LineLayerCommonStyleUniform&, Containers::ArrayView<const LineLayerStyleUniform>) override {}
    } shared{LineLayer::Shared::Configuration{3, 5}};

    Containers::String out;
    Error redirectError{&out};
    shared.setStyle(LineLayerCommonStyleUniform{},
        {LineLayerStyleUniform{}, LineLayerStyleUniform{}},
        {0, 1, 2, 1, 0},
        {{}, {}, {}, {}, {}},
        {{}, {}, {}, {}, {}});
    shared.setStyle(LineLayerCommonStyleUniform{},
        {LineLayerStyleUniform{}, LineLayerStyleUniform{}, LineLayerStyleUniform{}},
        {0, 1, 2},
        {{}, {}, {}, {}, {}},
        {{}, {}, {}, {}, {}});
    shared.setStyle(LineLayerCommonStyleUniform{},
        {LineLayerStyleUniform{}, LineLayerStyleUniform{}, LineLayerStyleUniform{}},
        {0, 1, 2, 1, 0},
        {{}, {}, {}},
        {{}, {}, {}, {}, {}});
    shared.setStyle(LineLayerCommonStyleUniform{},
        {LineLayerStyleUniform{}, LineLayerStyleUniform{}, LineLayerStyleUniform{}},
        {0, 1, 2, 1, 0},
        {{}, {}, {}, {}, {}},
        {{}, {}, {}});
    CORRADE_COMPARE_AS(out,
        "Ui::LineLayer::Shared::setStyle(): expected 3 uniforms, got 2\n"
        "Ui::LineLayer::Shared::setStyle(): expected 5 style uniform indices, got 3\n"
        "Ui::LineLayer::Shared::setStyle(): expected 5 alignment values, got 3\n"
        "Ui::LineLayer::Shared::setStyle(): expected either no or 5 paddings, got 3\n",
        TestSuite::Compare::String);
}

void LineLayerTest::sharedSetStyleInvalidMapping() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Shared: LineLayer::Shared {
        explicit Shared(const Configuration& configuration): LineLayer::Shared{configuration} {}

        void doSetStyle(const LineLayerCommonStyleUniform&, Containers::ArrayView<const LineLayerStyleUniform>) override {}
    } shared{LineLayer::Shared::Configuration{3, 6}};

    Containers::String out;
    Error redirectError{&out};
    shared.setStyle(LineLayerCommonStyleUniform{},
        {LineLayerStyleUniform{}, LineLayerStyleUniform{}, LineLayerStyleUniform{}},
        {0, 1, 2, 1, 3, 2},
        {{}, {}, {}, {}, {}, {}},
        {});
    CORRADE_COMPARE_AS(out,
        "Ui::LineLayer::Shared::setStyle(): uniform index 3 out of range for 3 uniforms at index 4\n",
        TestSuite::Compare::String);
}

void LineLayerTest::sharedSetStyleImplicitMapping() {
    struct Shared: LineLayer::Shared {
        explicit Shared(const Configuration& configuration): LineLayer::Shared{configuration} {}
        State& state() { return static_cast<State&>(*_state); }

        void doSetStyle(const LineLayerCommonStyleUniform& commonUniform, Containers::ArrayView<const LineLayerStyleUniform> uniforms) override {
            CORRADE_COMPARE(commonUniform.smoothness, 3.14f);
            CORRADE_COMPARE(uniforms.size(), 3);
            CORRADE_COMPARE(uniforms[1].color, 0xc0ffee_rgbf);
            ++setStyleCalled;
        }

        Int setStyleCalled = 0;
    } shared{LineLayer::Shared::Configuration{3}};

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    shared.setStyle(
        LineLayerCommonStyleUniform{}
            .setSmoothness(3.14f),
        {LineLayerStyleUniform{},
         LineLayerStyleUniform{}
            .setColor(0xc0ffee_rgbf),
         LineLayerStyleUniform{}},
        {LineAlignment::MiddleCenter,
         LineAlignment::TopRight,
         LineAlignment::BottomLeft},
        {{1.0f, 2.0f, 3.0f, 4.0f},
         {4.0f, 3.0f, 2.0f, 1.0f},
         {2.0f, 1.0f, 4.0f, 3.0f}});
    CORRADE_COMPARE(shared.setStyleCalled, 1);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::LineLayerStyle::uniform), Containers::stridedArrayView({
        0u, 1u, 2u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::LineLayerStyle::alignment), Containers::stridedArrayView({
        LineAlignment::MiddleCenter,
        LineAlignment::TopRight,
        LineAlignment::BottomLeft
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::LineLayerStyle::padding), Containers::stridedArrayView({
        Vector4{1.0f, 2.0f, 3.0f, 4.0f},
        Vector4{4.0f, 3.0f, 2.0f, 1.0f},
        Vector4{2.0f, 1.0f, 4.0f, 3.0f}
    }), TestSuite::Compare::Container);
}

void LineLayerTest::sharedSetStyleImplicitMappingImplicitPadding() {
    struct Shared: LineLayer::Shared {
        explicit Shared(const Configuration& configuration): LineLayer::Shared{configuration} {}
        State& state() { return static_cast<State&>(*_state); }

        void doSetStyle(const LineLayerCommonStyleUniform& commonUniform, Containers::ArrayView<const LineLayerStyleUniform> uniforms) override {
            CORRADE_COMPARE(commonUniform.smoothness, 3.14f);
            CORRADE_COMPARE(uniforms.size(), 3);
            CORRADE_COMPARE(uniforms[1].color, 0xc0ffee_rgbf);
            ++setStyleCalled;
        }

        Int setStyleCalled = 0;
    } shared{LineLayer::Shared::Configuration{3}};

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    shared.setStyle(
        LineLayerCommonStyleUniform{}
            .setSmoothness(3.14f),
        {LineLayerStyleUniform{},
         LineLayerStyleUniform{}
            .setColor(0xc0ffee_rgbf),
         LineLayerStyleUniform{}},
        {LineAlignment::MiddleCenter,
         LineAlignment::TopRight,
         LineAlignment::BottomLeft},
        {});
    CORRADE_COMPARE(shared.setStyleCalled, 1);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::LineLayerStyle::uniform), Containers::stridedArrayView({
        0u, 1u, 2u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::LineLayerStyle::alignment), Containers::stridedArrayView({
        LineAlignment::MiddleCenter,
        LineAlignment::TopRight,
        LineAlignment::BottomLeft
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::LineLayerStyle::padding), Containers::stridedArrayView({
        Vector4{},
        Vector4{},
        Vector4{}
    }), TestSuite::Compare::Container);

    /* Setting a style with implicit padding after a non-implicit padding was
       set should reset it back to zeros */
    shared.setStyle(
        LineLayerCommonStyleUniform{}
            .setSmoothness(3.14f),
        {LineLayerStyleUniform{},
         LineLayerStyleUniform{}
            .setColor(0xc0ffee_rgbf),
         LineLayerStyleUniform{}},
        {LineAlignment::MiddleCenter,
         LineAlignment::TopRight,
         LineAlignment::BottomLeft},
        {{1.0f, 2.0f, 3.0f, 4.0f},
         {4.0f, 3.0f, 2.0f, 1.0f},
         {2.0f, 1.0f, 4.0f, 3.0f}});
    shared.setStyle(
        LineLayerCommonStyleUniform{}
            .setSmoothness(3.14f),
        {LineLayerStyleUniform{},
         LineLayerStyleUniform{}
            .setColor(0xc0ffee_rgbf),
         LineLayerStyleUniform{}},
        {LineAlignment::MiddleCenter,
         LineAlignment::TopRight,
         LineAlignment::BottomLeft},
        {});
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::LineLayerStyle::padding), Containers::stridedArrayView({
        Vector4{},
        Vector4{},
        Vector4{}
    }), TestSuite::Compare::Container);
}

void LineLayerTest::sharedSetStyleImplicitMappingInvalidSize() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Shared: LineLayer::Shared {
        explicit Shared(const Configuration& configuration): LineLayer::Shared{configuration} {}

        void doSetStyle(const LineLayerCommonStyleUniform&, Containers::ArrayView<const LineLayerStyleUniform>) override {}
    } shared{LineLayer::Shared::Configuration{3, 5}};

    Containers::String out;
    Error redirectError{&out};
    shared.setStyle(LineLayerCommonStyleUniform{},
        {LineLayerStyleUniform{}, LineLayerStyleUniform{}},
        {{}, {}, {}, {}, {}},
        {{}, {}, {}, {}, {}});
    CORRADE_COMPARE(out,
        "Ui::LineLayer::Shared::setStyle(): there's 3 uniforms for 5 styles, provide an explicit mapping\n");
}

void LineLayerTest::construct() {
    struct LayerShared: LineLayer::Shared {
        explicit LayerShared(const Configuration& configuration): LineLayer::Shared{configuration} {}

        void doSetStyle(const LineLayerCommonStyleUniform&, Containers::ArrayView<const LineLayerStyleUniform>) override {}
    } shared{LineLayer::Shared::Configuration{3, 5}};

    struct Layer: LineLayer {
        explicit Layer(LayerHandle handle, Shared& shared): LineLayer{handle, shared} {}
    } layer{layerHandle(137, 0xfe), shared};

    CORRADE_COMPARE(layer.handle(), layerHandle(137, 0xfe));
    CORRADE_COMPARE(&layer.shared(), &shared);
    /* Const overload */
    CORRADE_COMPARE(&static_cast<const Layer&>(layer).shared(), &shared);
}

void LineLayerTest::constructCopy() {
    CORRADE_VERIFY(!std::is_copy_constructible<LineLayer>{});
    CORRADE_VERIFY(!std::is_copy_assignable<LineLayer>{});
}

void LineLayerTest::constructMove() {
    struct LayerShared: LineLayer::Shared {
        explicit LayerShared(const Configuration& configuration): LineLayer::Shared{configuration} {}

        void doSetStyle(const LineLayerCommonStyleUniform&, Containers::ArrayView<const LineLayerStyleUniform>) override {}
    };

    struct Layer: LineLayer {
        explicit Layer(LayerHandle handle, Shared& shared): LineLayer{handle, shared} {}
    };

    LayerShared shared{LineLayer::Shared::Configuration{1, 3}};
    LayerShared shared2{LineLayer::Shared::Configuration{5, 7}};

    Layer a{layerHandle(137, 0xfe), shared};

    Layer b{Utility::move(a)};
    CORRADE_COMPARE(b.handle(), layerHandle(137, 0xfe));
    CORRADE_COMPARE(&b.shared(), &shared);

    Layer c{layerHandle(0, 2), shared2};
    c = Utility::move(b);
    CORRADE_COMPARE(c.handle(), layerHandle(137, 0xfe));
    CORRADE_COMPARE(&c.shared(), &shared);

    CORRADE_VERIFY(std::is_nothrow_move_constructible<LineLayer>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<LineLayer>::value);
}

template<class T> void LineLayerTest::createRemoveSet() {
    auto&& data = CreateRemoveSetData[testCaseInstanceId()];
    setTestCaseDescription(data.name);
    setTestCaseTemplateName(std::is_same<T, Enum>::value ? "Enum" : "UnsignedInt");

    struct LayerShared: LineLayer::Shared {
        explicit LayerShared(const Configuration& configuration): LineLayer::Shared{configuration} {}

        void doSetStyle(const LineLayerCommonStyleUniform&, Containers::ArrayView<const LineLayerStyleUniform>) override {}
    } shared{LineLayer::Shared::Configuration{12, 38}};

    /* Not setting any alignment or padding via style -- tested in
       setAlignment() and setPadding() instead */

    struct Layer: LineLayer {
        explicit Layer(LayerHandle handle, Shared& shared): LineLayer{handle, shared} {}

        const State& stateData() const {
            return static_cast<const State&>(*_state);
        }
    } layer{layerHandle(0, 1), shared};

    /* Indexed with index count matching a strip, implicit colors */
    DataHandle first = layer.create(T(17),
        {3, 1, 2, 0, 1, 2},
        {{1.0f, 2.0f}, {3.0f, 4.0f}, {5.0f, 6.0f}, {7.0f, 8.0f}},
        {},
        data.node);
    CORRADE_COMPARE(layer.node(first), data.node);
    CORRADE_COMPARE(layer.style(first), 17);
    CORRADE_COMPARE(layer.indexCount(first), 6);
    CORRADE_COMPARE(layer.pointCount(first), 4);
    CORRADE_COMPARE(layer.color(first), 0xffffff_rgbf);
    CORRADE_COMPARE(layer.alignment(first), Containers::NullOpt);
    CORRADE_COMPARE(layer.padding(first), Vector4{0.0f});
    CORRADE_COMPARE(layer.state(), data.state);

    /* Explicit colors, default null node. Testing the view-taking API and also
       the getter overloads and templates. */
    const UnsignedInt secondIndices[]{1, 2};
    const Vector2 secondPoints[]{{0.1f, 0.2f}, {0.3f, 0.4f}, {0.5f, 0.6f}};
    const Color4 secondColors[]{0x33ff6699_rgbaf, 0xff339966_rgbaf, 0x669933ff_rgbaf};
    DataHandle second = layer.create(T(22), secondIndices, secondPoints, secondColors);
    CORRADE_COMPARE(layer.node(second), NodeHandle::Null);
    if(data.layerDataHandleOverloads) {
        CORRADE_COMPARE(layer.style(dataHandleData(second)), 22);
        CORRADE_COMPARE(layer.indexCount(dataHandleData(second)), 2);
        CORRADE_COMPARE(layer.pointCount(dataHandleData(second)), 3);
        /* Can't use StyleIndex, as the function restricts to enum types which
           would fail for StyleIndex == UnsignedInt */
        CORRADE_COMPARE(layer.template style<Enum>(dataHandleData(second)), Enum(22));
        CORRADE_COMPARE(layer.color(dataHandleData(second)), 0xffffff_rgbf);
        CORRADE_COMPARE(layer.alignment(dataHandleData(second)), Containers::NullOpt);
        CORRADE_COMPARE(layer.padding(dataHandleData(second)), Vector4{0.0f});
    } else {
        CORRADE_COMPARE(layer.style(second), 22);
        CORRADE_COMPARE(layer.indexCount(second), 2);
        CORRADE_COMPARE(layer.pointCount(second), 3);
        /* Can't use StyleIndex, as the function restricts to enum types which
           would fail for StyleIndex == UnsignedInt */
        CORRADE_COMPARE(layer.template style<Enum>(second), Enum(22));
        CORRADE_COMPARE(layer.color(second), 0xffffff_rgbf);
        CORRADE_COMPARE(layer.alignment(second), Containers::NullOpt);
        CORRADE_COMPARE(layer.padding(second), Vector4{0.0f});
    }
    CORRADE_COMPARE(layer.state(), data.state);

    /* Strip, explicit colors, default null node */
    DataHandle third = layer.createStrip(T(2),
        {{1.0f, 0.5f}, {0.5f, 1.0f}, {0.5f, 0.5f}, {1.0f, 1.0f}},
        {0x33006600_rgbaf, 0x66003300_rgbaf, 0x00330066_rgbaf, 0x00003366_rgbaf});
    CORRADE_COMPARE(layer.node(third), NodeHandle::Null);
    CORRADE_COMPARE(layer.style(third), 2);
    CORRADE_COMPARE(layer.indexCount(third), 6);
    CORRADE_COMPARE(layer.pointCount(third), 4);
    CORRADE_COMPARE(layer.color(third), 0xffffff_rgbf);
    CORRADE_COMPARE(layer.alignment(third), Containers::NullOpt);
    CORRADE_COMPARE(layer.padding(third), Vector4{0.0f});
    CORRADE_COMPARE(layer.state(), data.state);

    /* Loop, implicit colors. Testing the view-taking overload. */
    const Vector2 fourthPoints[]{{-1.0f, 1.0f}, {1.0f, -1.0f}, {1.0f, 1.0f}};
    DataHandle fourth = layer.createLoop(T(5), fourthPoints, nullptr, data.node);
    CORRADE_COMPARE(layer.node(fourth), data.node);
    CORRADE_COMPARE(layer.style(fourth), 5);
    CORRADE_COMPARE(layer.indexCount(fourth), 6);
    CORRADE_COMPARE(layer.pointCount(fourth), 3);
    CORRADE_COMPARE(layer.color(fourth), 0xffffff_rgbf);
    CORRADE_COMPARE(layer.alignment(fourth), Containers::NullOpt);
    CORRADE_COMPARE(layer.padding(fourth), Vector4{0.0f});
    CORRADE_COMPARE(layer.state(), data.state);

    /* Empty line. Empty strip and loop is thoroughly tested in
       createEmpty(). */
    DataHandle fifth = layer.create(T(11),
        {},
        {},
        {},
        data.node);
    CORRADE_COMPARE(layer.node(fifth), data.node);
    CORRADE_COMPARE(layer.style(fifth), 11);
    CORRADE_COMPARE(layer.indexCount(fifth), 0);
    CORRADE_COMPARE(layer.pointCount(fifth), 0);
    CORRADE_COMPARE(layer.color(fifth), 0xffffff_rgbf);
    CORRADE_COMPARE(layer.alignment(fifth), Containers::NullOpt);
    CORRADE_COMPARE(layer.padding(fifth), Vector4{0.0f});
    CORRADE_COMPARE(layer.state(), data.state);

    /* Strip, implicit colors. Testing the view-taking overload. */
    const Vector2 sixthPoints[]{{0.0f, 1.0f}, {1.0f, 0.0f}};
    DataHandle sixth = layer.createStrip(T(25), sixthPoints, nullptr, data.node);
    CORRADE_COMPARE(layer.node(sixth), data.node);
    CORRADE_COMPARE(layer.style(sixth), 25);
    CORRADE_COMPARE(layer.indexCount(sixth), 2);
    CORRADE_COMPARE(layer.pointCount(sixth), 2);
    CORRADE_COMPARE(layer.color(sixth), 0xffffff_rgbf);
    CORRADE_COMPARE(layer.alignment(sixth), Containers::NullOpt);
    CORRADE_COMPARE(layer.padding(sixth), Vector4{0.0f});
    CORRADE_COMPARE(layer.state(), data.state);

    /* Loop, explicit colors, default null node */
    DataHandle seventh = layer.createLoop(T(0),
        {{0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}},
        {0xff00ff00_rgbaf, 0x00ff00ff_rgbaf, 0x00ffff00_rgbaf, 0xff0000ff_rgbaf});
    CORRADE_COMPARE(layer.node(seventh), NodeHandle::Null);
    CORRADE_COMPARE(layer.style(seventh), 0);
    CORRADE_COMPARE(layer.indexCount(seventh), 8);
    CORRADE_COMPARE(layer.pointCount(seventh), 4);
    CORRADE_COMPARE(layer.color(seventh), 0xffffff_rgbf);
    CORRADE_COMPARE(layer.alignment(seventh), Containers::NullOpt);
    CORRADE_COMPARE(layer.padding(seventh), Vector4{0.0f});
    CORRADE_COMPARE(layer.state(), data.state);

    /* Indexed with index count matching a loop, explicit colors, default null
       node */
    DataHandle eighth = layer.create(T(0),
        {0, 1, 0, 2, 1, 1},
        {{2.0f, 3.0f}, {3.0f, 3.0f}, {3.0f, 2.0f}},
        {0x99009900_rgbaf, 0x00990099_rgbaf, 0x00999900_rgbaf});
    CORRADE_COMPARE(layer.node(eighth), NodeHandle::Null);
    CORRADE_COMPARE(layer.style(eighth), 0);
    CORRADE_COMPARE(layer.indexCount(eighth), 6);
    CORRADE_COMPARE(layer.pointCount(eighth), 3);
    CORRADE_COMPARE(layer.color(eighth), 0xffffff_rgbf);
    CORRADE_COMPARE(layer.alignment(eighth), Containers::NullOpt);
    CORRADE_COMPARE(layer.padding(eighth), Vector4{0.0f});
    CORRADE_COMPARE(layer.state(), data.state);

    /* There should be eight runs, assigned to the eight data */
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().data).slice(&Implementation::LineLayerData::run), Containers::arrayView({
        0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::indexCount), Containers::arrayView({
        6u, 2u, 6u,  6u,  0u,  2u,  8u,  6u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::indexOffset), Containers::arrayView({
        0u, 6u, 8u, 14u, 20u, 20u, 22u, 30u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::pointCount), Containers::arrayView({
        4u, 3u, 4u,  3u,  0u,  2u,  4u,  3u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::pointOffset), Containers::arrayView({
        0u, 4u, 7u, 11u, 14u, 14u, 16u, 20u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::data), Containers::arrayView({
        0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u
    }), TestSuite::Compare::Container);
    /* Verifying just the indices, not the neighbors, that's done in
       createSetIndicesNeighbors() and createSetStripLoopIndicesNeighbors() */
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().pointIndices).slice(&Implementation::LineLayerPointIndex::index), Containers::arrayView<UnsignedInt>({
        3, 1, 2, 0, 1, 2,
        1, 2,
        0, 1, 1, 2, 2, 3,       /* strip */
        0, 1, 1, 2, 2, 0,       /* loop */
                                /* empty */
        0, 1,                   /* strip */
        0, 1, 1, 2, 2, 3, 3, 0, /* loop */
        0, 1, 0, 2, 1, 1
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().points).slice(&Implementation::LineLayerPoint::position), Containers::arrayView<Vector2>({
        {1.0f, 2.0f}, {3.0f, 4.0f}, {5.0f, 6.0f}, {7.0f, 8.0f},
        {0.1f, 0.2f}, {0.3f, 0.4f}, {0.5f, 0.6f},
        {1.0f, 0.5f}, {0.5f, 1.0f}, {0.5f, 0.5f}, {1.0f, 1.0f},
        {-1.0f, 1.0f}, {1.0f, -1.0f}, {1.0f, 1.0f},
        /* empty */
        {0.0f, 1.0f}, {1.0f, 0.0f},
        {0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
        {2.0f, 3.0f}, {3.0f, 3.0f}, {3.0f, 2.0f}
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().points).slice(&Implementation::LineLayerPoint::color), Containers::arrayView<Color4>({
        0xffffff_rgbf, 0xffffff_rgbf, 0xffffff_rgbf, 0xffffff_rgbf, /* implicit */
        0x33ff6699_rgbaf, 0xff339966_rgbaf, 0x669933ff_rgbaf,
        0x33006600_rgbaf, 0x66003300_rgbaf, 0x00330066_rgbaf, 0x00003366_rgbaf,
        0xffffff_rgbf, 0xffffff_rgbf, 0xffffff_rgbf, /* implicit */
        /* empty */
        0xffffff_rgbf, 0xffffff_rgbf, /* implicit */
        0xff00ff00_rgbaf, 0x00ff00ff_rgbaf, 0x00ffff00_rgbaf, 0xff0000ff_rgbaf,
        0x99009900_rgbaf, 0x00990099_rgbaf, 0x00999900_rgbaf
    }), TestSuite::Compare::Container);

    /* Removing a line marks the original run as unused, and as it's not
       attached to any node, doesn't set any state flag. The remaining data
       don't need any refresh, they still draw correctly. */
    data.layerDataHandleOverloads ?
        layer.remove(dataHandleData(third)) :
        layer.remove(third);
    CORRADE_COMPARE(layer.state(), data.state|LayerState::NeedsDataClean);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().data).slice(&Implementation::LineLayerData::run), Containers::arrayView({
        0u, 1u, 2u /*unused*/, 3u, 4u, 5u, 6u, 7u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::indexCount), Containers::arrayView({
        6u, 2u, 6u /*unused*/, 6u, 0u, 2u, 8u, 6u,
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::indexOffset), Containers::arrayView({
        0u, 6u, 0xffffffffu, 14u, 20u, 20u, 22u, 30u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::pointCount), Containers::arrayView({
        4u, 3u, 4u /*unused*/, 3u, 0u, 2u, 4u, 3u,
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::pointOffset), Containers::arrayView({
        0u, 4u, 0xffffffffu, 11u, 14u, 14u, 16u, 20u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::data), Containers::arrayView({
        0u, 1u, 2u /*unused*/, 3u, 4u, 5u, 6u, 7u
    }), TestSuite::Compare::Container);

    /* Setting a line with the same point + index count will reuse the same
       run. It doesn't matter if the previous was an indexed line, strip or
       loop, only the counts get checked. Positions get changed to negative,
       colors get changed from implicit to explicit and vice versa. */
    if(!data.layerDataHandleOverloads) {
        layer.setLineStrip(first, /* used to be indexed with implicit colors */
            {{-1.0f, -2.0f}, {-3.0f, -4.0f}, {-5.0f, -6.0f}, {-7.0f, -8.0f}},
            {0x33663366_rgbaf, 0x66336633_rgbaf, 0x33336666_rgbaf, 0x66333366_rgbaf});
        layer.setLineLoop(eighth, /* used to be indexed with explicit colors */
            {{-2.0f, -3.0f}, {-3.0f, -3.0f}, {-3.0f, -2.0f}},
            {});
        layer.setLine(fifth, {}, {}, {}); /* was empty before, is now as well */
        layer.setLine(sixth, /* used to be a strip with implicit colors */
            {1, 0},
            {{-0.0f, -1.0f}, {-1.0f, -0.0f}},
            {0xff33ff33_rgbaf, 0x33ff33ff_rgbaf});
    } else {
        layer.setLineStrip(dataHandleData(first),
            {{-1.0f, -2.0f}, {-3.0f, -4.0f}, {-5.0f, -6.0f}, {-7.0f, -8.0f}},
            {0x33663366_rgbaf, 0x66336633_rgbaf, 0x33336666_rgbaf, 0x66333366_rgbaf});
        layer.setLineLoop(dataHandleData(eighth),
            {{-2.0f, -3.0f}, {-3.0f, -3.0f}, {-3.0f, -2.0f}},
            {});
        layer.setLine(dataHandleData(fifth), {}, {}, {});
        layer.setLine(dataHandleData(sixth),
            {1, 0},
            {{-0.0f, -1.0f}, {-1.0f, -0.0f}},
            {0xff33ff33_rgbaf, 0x33ff33ff_rgbaf});
    }
    CORRADE_COMPARE(layer.state(), data.state|LayerState::NeedsDataClean|LayerState::NeedsDataUpdate);
    /* Runs, counts and offsets are the same as above */
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().data).slice(&Implementation::LineLayerData::run), Containers::arrayView({
        0u, 1u, 2u /*unused*/, 3u, 4u, 5u, 6u, 7u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::indexCount), Containers::arrayView({
        6u, 2u, 6u /*unused*/, 6u, 0u, 2u, 8u, 6u,
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::indexOffset), Containers::arrayView({
        0u, 6u, 0xffffffffu, 14u, 20u, 20u, 22u, 30u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::pointCount), Containers::arrayView({
        4u, 3u, 4u /*unused*/, 3u, 0u, 2u, 4u, 3u,
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::pointOffset), Containers::arrayView({
        0u, 4u, 0xffffffffu, 11u, 14u, 14u, 16u, 20u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::data), Containers::arrayView({
        0u, 1u, 2u /*unused*/, 3u, 4u, 5u, 6u, 7u
    }), TestSuite::Compare::Container);
    /* Contents are changed */
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().pointIndices).slice(&Implementation::LineLayerPointIndex::index), Containers::arrayView<UnsignedInt>({
        0, 1, 1, 2, 2, 3,       /* set */
        1, 2,
        0, 1, 1, 2, 2, 3,       /* unused */
        0, 1, 1, 2, 2, 0,
        /* empty, set again */
        1, 0,                   /* set */
        0, 1, 1, 2, 2, 3, 3, 0,
        0, 1, 1, 2, 2, 0,       /* set */
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().points).slice(&Implementation::LineLayerPoint::position), Containers::arrayView<Vector2>({
        {-1.0f, -2.0f}, {-3.0f, -4.0f}, {-5.0f, -6.0f}, {-7.0f, -8.0f}, /* set */
        {0.1f, 0.2f}, {0.3f, 0.4f}, {0.5f, 0.6f},
        {1.0f, 0.5f}, {0.5f, 1.0f}, {0.5f, 0.5f}, {1.0f, 1.0f}, /* unused */
        {-1.0f, 1.0f}, {1.0f, -1.0f}, {1.0f, 1.0f},
        /* empty, set again */
        {-0.0f, -1.0f}, {-1.0f, -0.0f}, /* set */
        {0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
        {-2.0f, -3.0f}, {-3.0f, -3.0f}, {-3.0f, -2.0f}
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().points).slice(&Implementation::LineLayerPoint::color), Containers::arrayView<Color4>({
        0x33663366_rgbaf, 0x66336633_rgbaf, 0x33336666_rgbaf, 0x66333366_rgbaf, /* set */
        0x33ff6699_rgbaf, 0xff339966_rgbaf, 0x669933ff_rgbaf,
        0x33006600_rgbaf, 0x66003300_rgbaf, 0x00330066_rgbaf, 0x00003366_rgbaf, /* unused */
        0xffffff_rgbf, 0xffffff_rgbf, 0xffffff_rgbf,
        /* empty, set again */
        0xff33ff33_rgbaf, 0x33ff33ff_rgbaf, /* set */
        0xff00ff00_rgbaf, 0x00ff00ff_rgbaf, 0x00ffff00_rgbaf, 0xff0000ff_rgbaf,
        0xffffff_rgbf, 0xffffff_rgbf, 0xffffff_rgbf /* set to implicit */
    }), TestSuite::Compare::Container);

    /* Setting lines where either the index count or the point count differs,
       but not both, to verify that a new run is created for each. Colors are
       sometimes set and sometimes not, should get filled even for the new
       run. */
    layer.setLineLoop(sixth, /* same index count, smaller point count */
        {{0.0f, 1.0f}},
        {});
    layer.setLineLoop(first, /* same point count, larger index count */
        {{1.0f, 2.0f}, {3.0f, 4.0f}, {5.0f, 6.0f}, {7.0f, 8.0f}},
        {0x33006600_rgbaf, 0x66003300_rgbaf, 0x00330066_rgbaf, 0x00660033_rgbaf});
    layer.setLineStrip(second, /* same index count, smaller point count */
        {{0.1f, 0.2f}, {0.3f, 0.4f}},
        {0x33ff6699_rgbaf, 0xff339966_rgbaf});
    layer.setLineStrip(fourth, /* same point count, smaller index count */
        {{-1.0f, 1.0f}, {1.0f, -1.0f}, {1.0f, 1.0f}},
        {});
    layer.setLine(seventh, /* same index count, smaller point count */
        {2, 0, 1, 2, 0, 0, 1, 2},
        {{0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f}},
        {});
    layer.setLine(eighth, /* same point count, smaller index count */
        {1, 2},
        {{2.0f, 3.0f}, {3.0f, 3.0f}, {3.0f, 2.0f}},
        {0x99009900_rgbaf, 0x00990099_rgbaf, 0x00999900_rgbaf});
    /* Original runs are marked as unused, new added */
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().data).slice(&Implementation::LineLayerData::run), Containers::arrayView({
        9u, 10u, 2u /*unused*/, 11u, 4u, 8u, 12u, 13u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::indexCount), Containers::arrayView({
     /* vv--vv--vv--vv------vv--vv--vv-- unused */
        6u, 2u, 6u, 6u, 0u, 2u, 8u, 6u,
        2u, 8u, 2u, 4u, 8u, 2u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::indexOffset), Containers::arrayView({
        0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 20u, 0xffffffffu, 0xffffffffu, 0xffffffffu,
        36u, 38u, 46u, 48u, 52u, 60u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::pointCount), Containers::arrayView({
     /* vv--vv--vv--vv------vv--vv--vv-- unused */
        4u, 3u, 4u, 3u, 0u, 2u, 4u, 3u,
        1u, 4u, 2u, 3u, 3u, 3u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::pointOffset), Containers::arrayView({
        0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 14u, 0xffffffffu, 0xffffffffu, 0xffffffffu,
        23u, 24u, 28u, 30u, 33u, 36u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::data), Containers::arrayView({
     /* vv--vv--vv--vv------vv--vv--vv-- unused */
        0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u,
        5u, 0u, 1u, 3u, 6u, 7u
    }), TestSuite::Compare::Container);
    /* Prefix of the contents is the same, new runs at the end */
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().pointIndices).slice(&Implementation::LineLayerPointIndex::index), Containers::arrayView<UnsignedInt>({
        0, 1, 1, 2, 2, 3,       /* unused */
        1, 2,                   /* unused */
        0, 1, 1, 2, 2, 3,       /* unused */
        0, 1, 1, 2, 2, 0,       /* unused */
        /* empty */
        1, 0,                   /* unused */
        0, 1, 1, 2, 2, 3, 3, 0, /* unused */
        0, 1, 1, 2, 2, 0,       /* unused */
        0, 0,                   /* loop */
        0, 1, 1, 2, 2, 3, 3, 0, /* loop */
        0, 1,                   /* strip */
        0, 1, 1, 2,             /* strip */
        2, 0, 1, 2, 0, 0, 1, 2,
        1, 2
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().points).slice(&Implementation::LineLayerPoint::position), Containers::arrayView<Vector2>({
        {-1.0f, -2.0f}, {-3.0f, -4.0f}, {-5.0f, -6.0f}, {-7.0f, -8.0f}, /* unused */
        {0.1f, 0.2f}, {0.3f, 0.4f}, {0.5f, 0.6f},               /* unused */
        {1.0f, 0.5f}, {0.5f, 1.0f}, {0.5f, 0.5f}, {1.0f, 1.0f}, /* unused */
        {-1.0f, 1.0f}, {1.0f, -1.0f}, {1.0f, 1.0f},             /* unused */
        /* empty */
        {-0.0f, -1.0f}, {-1.0f, -0.0f},                         /* unused */
        {0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, /* unused */
        {-2.0f, -3.0f}, {-3.0f, -3.0f}, {-3.0f, -2.0f},         /* unused */
        {0.0f, 1.0f},
        {1.0f, 2.0f}, {3.0f, 4.0f}, {5.0f, 6.0f}, {7.0f, 8.0f},
        {0.1f, 0.2f}, {0.3f, 0.4f},
        {-1.0f, 1.0f}, {1.0f, -1.0f}, {1.0f, 1.0f},
        {0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f},
        {2.0f, 3.0f}, {3.0f, 3.0f}, {3.0f, 2.0f},
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().points).slice(&Implementation::LineLayerPoint::color), Containers::arrayView<Color4>({
        0x33663366_rgbaf, 0x66336633_rgbaf, 0x33336666_rgbaf, 0x66333366_rgbaf, /* unused */
        0x33ff6699_rgbaf, 0xff339966_rgbaf, 0x669933ff_rgbaf, /* unused */
        0x33006600_rgbaf, 0x66003300_rgbaf, 0x00330066_rgbaf, 0x00003366_rgbaf, /* unused */
        0xffffff_rgbf, 0xffffff_rgbf, 0xffffff_rgbf, /* unused */
        /* empty */
        0xff33ff33_rgbaf, 0x33ff33ff_rgbaf, /* unused */
        0xff00ff00_rgbaf, 0x00ff00ff_rgbaf, 0x00ffff00_rgbaf, 0xff0000ff_rgbaf,
        0xffffff_rgbf, 0xffffff_rgbf, 0xffffff_rgbf, /* unused */
        0xffffff_rgbf, /* implicit */
        0x33006600_rgbaf, 0x66003300_rgbaf, 0x00330066_rgbaf, 0x00660033_rgbaf,
        0x33ff6699_rgbaf, 0xff339966_rgbaf,
        0xffffff_rgbf, 0xffffff_rgbf, 0xffffff_rgbf, /* implicit */
        0xffffff_rgbf, 0xffffff_rgbf, 0xffffff_rgbf, /* implicit */
        0x99009900_rgbaf, 0x00990099_rgbaf, 0x00999900_rgbaf
    }), TestSuite::Compare::Container);
}

void LineLayerTest::createRemoveHandleRecycle() {
    struct LayerShared: LineLayer::Shared {
        explicit LayerShared(const Configuration& configuration): LineLayer::Shared{configuration} {}

        void doSetStyle(const LineLayerCommonStyleUniform&, Containers::ArrayView<const LineLayerStyleUniform>) override {}
    } shared{LineLayer::Shared::Configuration{1, 3}};

    struct Layer: LineLayer {
        explicit Layer(LayerHandle handle, Shared& shared): LineLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    DataHandle first = layer.create(0, {}, {}, {});
    DataHandle second = layer.create(0, {}, {}, {});
    layer.setColor(second, 0xff3366_rgbf);
    layer.setAlignment(second, LineAlignment::BottomRight);
    layer.setPadding(second, Vector4{5.0f});
    CORRADE_COMPARE(layer.color(first), 0xffffff_rgbf);
    CORRADE_COMPARE(layer.alignment(first), Containers::NullOpt);
    CORRADE_COMPARE(layer.padding(first), Vector4{0.0f});
    CORRADE_COMPARE(layer.color(second), 0xff3366_rgbf);
    CORRADE_COMPARE(layer.alignment(second), LineAlignment::BottomRight);
    CORRADE_COMPARE(layer.padding(second), Vector4{5.0f});

    /* Data that reuses a previous slot should have all properties cleared */
    layer.remove(second);
    DataHandle second2 = layer.create(0, {}, {}, {});
    CORRADE_COMPARE(dataHandleId(second2), dataHandleId(second));
    CORRADE_COMPARE(layer.color(second2), 0xffffff_rgbf);
    CORRADE_COMPARE(layer.alignment(second2), Containers::NullOpt);
    CORRADE_COMPARE(layer.padding(second2), Vector4{0.0f});
}

void LineLayerTest::createSetIndicesNeighbors() {
    /* Verifies neighbor calculation and various edge cases for indexed
       points */

    struct LayerShared: LineLayer::Shared {
        explicit LayerShared(const Configuration& configuration): LineLayer::Shared{configuration} {}

        void doSetStyle(const LineLayerCommonStyleUniform&, Containers::ArrayView<const LineLayerStyleUniform>) override {}
    } shared{LineLayer::Shared::Configuration{1}};

    /* Needed in order to be able to call update() */
    shared.setStyle(LineLayerCommonStyleUniform{},
        {LineLayerStyleUniform{}},
        {{}},
        {});

    struct Layer: LineLayer {
        explicit Layer(LayerHandle handle, Shared& shared): LineLayer{handle, shared} {}

        const State& stateData() const {
            return static_cast<const State&>(*_state);
        }
    } layer{layerHandle(0, 1), shared};

    /* Required to be called before update() (because AbstractUserInterface
       guarantees the same on a higher level), not needed for anything here */
    layer.setSize({1, 1}, {1, 1});

    /* Empty line shouldn't produce anything. Attaching all of these to a
       non-null NodeHandle to make update() below work. */
    DataHandle empty = layer.create(0, {}, {}, {}, nodeHandle(0, 1));

    /* The line could however also have some points but no indices. Those
       points are not used for anything either. */
    DataHandle emptyIndices = layer.create(0, {}, {{}, {}, {}}, {}, nodeHandle(0, 1));

    /* A point, equivalent to the one from createSetLoopIndicesNeighbors()
       below */
    DataHandle singlePoint = layer.create(0, {0, 0}, {{}}, {}, nodeHandle(0, 1));

    /* An explicitly indexed strip / loop, equivalent to the ones from
       createSet{Strip,Loop}IndicesNeighbors() below */
    DataHandle largeStrip = layer.create(0, {
        0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10
    }, {
        {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}
    }, {}, nodeHandle(0, 1));
    DataHandle largeLoop = layer.create(0, {
        0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 0
    }, {
        {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}
    }, {}, nodeHandle(0, 1));

    /* A 3-point loop, 3-point strip and a point together */
    DataHandle loopStripPoint = layer.create(0, {
        0, 1, 1, 2, 2, 0,
        3, 4, 4, 5,
        6, 6
    }, {
        {}, {}, {}, {}, {}, {}, {}
    }, {}, nodeHandle(0, 1));

    /* A point referenced three times (1 and 4) is not going to be marked as a
       join in any of these, but doesn't prevent creation of other joins (in
       3, 2 and 4, 7) */
    DataHandle threeSegmentJoin = layer.create(0, {
        0, 1, 2, 1, 1, 3, 3, 2,
        /* Same case but the second reference is also the first in the pair,
           and the order of the last pair is swapped */
        4, 5, 4, 6, 4, 7, 6, 7
    }, {
        {}, {}, {}, {}, {}, {}, {}, {}
    }, {}, nodeHandle(0, 1));

    /* The same segment listed twice is not going to be treated as a loop */
    DataHandle twoPointLoop = layer.create(0, {
        0, 1, 1, 0,
        /* Same case but the second occurence is in the same order */
        2, 3, 2, 3
    }, {
        {}, {}, {}, {}
    }, {}, nodeHandle(0, 1));

    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().data).slice(&Implementation::LineLayerData::run), Containers::arrayView({
        0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::indexCount), Containers::arrayView({
        0u, 0u, 2u, 20u, 22u, 12u, 16u,  8u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::indexOffset), Containers::arrayView({
        0u, 0u, 0u,  2u, 22u, 44u, 56u, 72u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::pointCount), Containers::arrayView({
        0u, 3u, 1u, 11u, 11u,  7u,  8u,  4u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::pointOffset), Containers::arrayView({
        0u, 0u, 3u,  4u, 15u, 26u, 33u, 41u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::data), Containers::arrayView({
        0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::joinCount), Containers::arrayView({
        0u, 0u, 0u, 9*2u, 11*2u, 3*2u + 2u, 4*2u, 0u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS((Containers::arrayCast<Containers::Pair<UnsignedInt, UnsignedInt>>(layer.stateData().pointIndices)),(Containers::arrayView<Containers::Pair<UnsignedInt, UnsignedInt>>({
        /* empty */
        /* empty */

        /* singlePoint */
        {0, 0xffffffffu}, {0, 0xffffffffu},

        /* largeStrip */
        {0, 0xffffffffu}, {1, 3},   /* 0*/
        /* The neighbor is always the *index* location where the neighbor point
           is, not the point location. In this case, neighbor of first item (1)
           is at index 0, which refers to 0, and neighbor of second item (2) is
           at index 5, which refers to 7. */
        {1,  0}, {2,  5},           /* 2*/
        {2,  2}, {3,  7},           /* 4*/
        {3,  4}, {4,  9},           /* 6*/
        {4,  6}, {5, 11},           /* 8*/
        {5,  8}, {6, 13},           /*10*/
        {6, 10}, {7, 15},           /*12*/
        {7, 12}, {8, 17},           /*14*/
        {8, 14}, {9, 19},           /*16*/
        {9, 16}, {10, 0xffffffffu}, /*18*/

        /* largeLoop */
        { 0, 20}, { 1,  3},         /* 0*/
        { 1,  0}, { 2,  5},         /* 2*/
        { 2,  2}, { 3,  7},         /* 4*/
        { 3,  4}, { 4,  9},         /* 6*/
        { 4,  6}, { 5, 11},         /* 8*/
        { 5,  8}, { 6, 13},         /*10*/
        { 6, 10}, { 7, 15},         /*12*/
        { 7, 12}, { 8, 17},         /*14*/
        { 8, 14}, { 9, 19},         /*16*/
        { 9, 16}, {10, 21},         /*18*/
        {10, 18}, { 0,  1},         /*20*/

        /* loopStripPoint */
        {0, 4}, {1, 3},             /*0*/
        {1, 0}, {2, 5},             /*2*/
        {2, 2}, {0, 1},             /*4*/
        {3, 0xffffffffu}, {4, 9},   /*6*/
        {4, 6}, {5, 0xffffffffu},   /*8*/
        {6, 0xffffffffu}, {6, 0xffffffffu},

        /* threeSegmentJoin. Index 1 is used three times, which causes no
           neighbors recorded for it. Index 2 and 3 are used two times however,
           so both get the neighbors filled. */
        {0, 0xffffffffu}, {1, 0xffffffffu},
        {2, 6}, {1, 0xffffffffu},   /* 2*/
        {1, 0xffffffffu}, {3, 7},   /* 4*/
        {3, 4}, {2, 3},             /* 6*/
        /* Similarly for index 4, which is used three times, but 5 and 7
           twice */
        {4, 0xffffffffu}, {5, 0xffffffffu},
        {4, 0xffffffffu}, {6, 15},  /*10*/
        {4, 0xffffffffu}, {7, 14},  /*12*/
        {6, 10}, {7, 12},           /*14*/

        /* twoPointLoop. Because the same pair is used twice, it isn't turned
           into a loop. */
        {0, 0xffffffffu}, {1, 0xffffffffu},
        {1, 0xffffffffu}, {0, 0xffffffffu},
        /* Similarly here, just the order is the same */
        {2, 0xffffffffu}, {3, 0xffffffffu},
        {2, 0xffffffffu}, {3, 0xffffffffu},
    })), TestSuite::Compare::Container);
    /* Just to verify the point data get copied / initialized at all */
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().points).slice(&Implementation::LineLayerPoint::position),
        Containers::stridedArrayView<Vector2>({{}}).broadcasted<0>(45),
        TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().points).slice(&Implementation::LineLayerPoint::color),
        Containers::stridedArrayView<Color4>({0xffffff_rgbf}).broadcasted<0>(45),
        TestSuite::Compare::Container);

    CORRADE_COMPARE(layer.state(), LayerState::NeedsNodeOffsetSizeUpdate|LayerState::NeedsAttachmentUpdate|LayerState::NeedsDataUpdate);

    /* Perform an update to clear state flags as well as verify the index /
       vertex buffer population doesn't trip up on any of these. Explicitly
       listing the data to make it possible to test just a subset in case
       update() blows up on some of those. */
    UnsignedInt dataIds[]{
        dataHandleId(empty),
        dataHandleId(emptyIndices),
        dataHandleId(singlePoint),
        dataHandleId(largeStrip),
        dataHandleId(largeLoop),
        dataHandleId(loopStripPoint),
        dataHandleId(threeSegmentJoin),
        dataHandleId(twoPointLoop)
    };
    CORRADE_COMPARE(Containers::arraySize(dataIds), layer.usedCount());
    Vector2 nodeOffsets[1];
    Vector2 nodeSizes[1];
    Float nodeOpacities[1];
    UnsignedByte nodesEnabled[1]{};
    layer.update(layer.state(), dataIds, {}, {}, nodeOffsets, nodeSizes, nodeOpacities, Containers::BitArrayView{nodesEnabled, 0, 1}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Even an empty call should mark the layer as dirty. Compared to
       createSet{Strip,Loop}IndicesNeighbors() there isn't any complex logic
       involved with comparing index count so we can only check that the run
       IDs stay the same. */

    layer.setLine(empty, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().data).slice(&Implementation::LineLayerData::run), Containers::arrayView({
        0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u
    }), TestSuite::Compare::Container);
}

void LineLayerTest::createSetStripIndicesNeighbors() {
    /* Verifies neighbor calculation and various edge cases for strips */

    struct LayerShared: LineLayer::Shared {
        explicit LayerShared(const Configuration& configuration): LineLayer::Shared{configuration} {}

        void doSetStyle(const LineLayerCommonStyleUniform&, Containers::ArrayView<const LineLayerStyleUniform>) override {}
    } shared{LineLayer::Shared::Configuration{1}};

    /* Needed in order to be able to call update() */
    shared.setStyle(LineLayerCommonStyleUniform{},
        {LineLayerStyleUniform{}},
        {{}},
        {});

    struct Layer: LineLayer {
        explicit Layer(LayerHandle handle, Shared& shared): LineLayer{handle, shared} {}

        const State& stateData() const {
            return static_cast<const State&>(*_state);
        }
    } layer{layerHandle(0, 1), shared};

    /* Required to be called before update() (because AbstractUserInterface
       guarantees the same on a higher level), not needed for anything here */
    layer.setSize({1, 1}, {1, 1});

    /* Empty strip shouldn't produce anything. Attaching all of these to a
       non-null NodeHandle to make update() below work. */
    DataHandle emptyStrip = layer.createStrip(0, {}, {}, nodeHandle(0, 1));

    /* Strip with just two points is a single segment. Strip with one point
       is invalid, tested in createSetInvalid() below. */
    DataHandle singleStripSegment = layer.createStrip(0, {{}, {}}, {}, nodeHandle(0, 1));

    /* Verify index calculation works even for strips larger than few points */
    DataHandle largeStrip = layer.createStrip(0, {
        {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}
    }, {}, nodeHandle(0, 1));

    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().data).slice(&Implementation::LineLayerData::run), Containers::arrayView({
        0u, 1u, 2u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::indexCount), Containers::arrayView({
        0u, 2u, 20u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::indexOffset), Containers::arrayView({
        0u, 0u, 2u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::pointCount), Containers::arrayView({
        0u, 2u, 11u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::pointOffset), Containers::arrayView({
        0u, 0u, 2u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::data), Containers::arrayView({
        0u, 1u, 2u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::joinCount), Containers::arrayView({
        0u, 0u, 9*2u
    }), TestSuite::Compare::Container);
    Containers::Pair<UnsignedInt, UnsignedInt> expectedPointIndices[]{
        /* empty */

        /* singleStripSegment */
        {0, 0xffffffffu}, {1, 0xffffffffu},

        /* largeStrip. Matches what's in createSetIndicesNeighbors(), see there
           for details. */
        {0, 0xffffffffu}, {1, 3},   /* 0*/
        {1,  0}, {2,  5},           /* 2*/
        {2,  2}, {3,  7},           /* 4*/
        {3,  4}, {4,  9},           /* 6*/
        {4,  6}, {5, 11},           /* 8*/
        {5,  8}, {6, 13},           /*10*/
        {6, 10}, {7, 15},           /*12*/
        {7, 12}, {8, 17},           /*14*/
        {8, 14}, {9, 19},           /*16*/
        {9, 16}, {10, 0xffffffffu}, /*18*/
    };
    CORRADE_COMPARE_AS((Containers::arrayCast<Containers::Pair<UnsignedInt, UnsignedInt>>(layer.stateData().pointIndices)),
        Containers::arrayView(expectedPointIndices),
        TestSuite::Compare::Container);
    /* Just to verify the point data get copied / initialized at all */
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().points).slice(&Implementation::LineLayerPoint::position),
        Containers::stridedArrayView<Vector2>({{}}).broadcasted<0>(13),
        TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().points).slice(&Implementation::LineLayerPoint::color),
        Containers::stridedArrayView<Color4>({0xffffff_rgbf}).broadcasted<0>(13),
        TestSuite::Compare::Container);

    CORRADE_COMPARE(layer.state(), LayerState::NeedsNodeOffsetSizeUpdate|LayerState::NeedsAttachmentUpdate|LayerState::NeedsDataUpdate);

    /* Perform an update to clear state flags as well as verify the index /
       vertex buffer population doesn't trip up on any of these. Explicitly
       listing the data to make it possible to test just a subset in case
       update() blows up on some of those. */
    UnsignedInt dataIds[]{
        dataHandleId(emptyStrip),
        dataHandleId(singleStripSegment),
        dataHandleId(largeStrip)
    };
    CORRADE_COMPARE(Containers::arraySize(dataIds), layer.usedCount());
    Vector2 nodeOffsets[1];
    Vector2 nodeSizes[1];
    Float nodeOpacities[1];
    UnsignedByte nodesEnabled[1]{};
    layer.update(layer.state(), dataIds, {}, {}, nodeOffsets, nodeSizes, nodeOpacities, Containers::BitArrayView{nodesEnabled, 0, 1}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Setting the data to the same lines should change nothing compared to
       above, but even an empty call should mark the layer as dirty */

    layer.setLineStrip(emptyStrip, {}, {});
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    layer.setLineStrip(singleStripSegment, {{}, {}}, {});
    layer.setLineStrip(largeStrip, {
        {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}
    }, {});
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().data).slice(&Implementation::LineLayerData::run), Containers::arrayView({
        0u, 1u, 2u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::indexCount), Containers::arrayView({
        0u, 2u, 20u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::indexOffset), Containers::arrayView({
        0u, 0u, 2u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::pointCount), Containers::arrayView({
        0u, 2u, 11u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::pointOffset), Containers::arrayView({
        0u, 0u, 2u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::data), Containers::arrayView({
        0u, 1u, 2u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS((Containers::arrayCast<Containers::Pair<UnsignedInt, UnsignedInt>>(layer.stateData().pointIndices)),
        Containers::arrayView(expectedPointIndices),
        TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().points).slice(&Implementation::LineLayerPoint::position),
        Containers::stridedArrayView<Vector2>({{}}).broadcasted<0>(13),
        TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().points).slice(&Implementation::LineLayerPoint::color),
        Containers::stridedArrayView<Color4>({0xffffff_rgbf}).broadcasted<0>(13),
        TestSuite::Compare::Container);
}

void LineLayerTest::createSetLoopIndicesNeighbors() {
    /* Verifies neighbor calculation and various edge cases for loops */

    struct LayerShared: LineLayer::Shared {
        explicit LayerShared(const Configuration& configuration): LineLayer::Shared{configuration} {}

        void doSetStyle(const LineLayerCommonStyleUniform&, Containers::ArrayView<const LineLayerStyleUniform>) override {}
    } shared{LineLayer::Shared::Configuration{1}};

    /* Needed in order to be able to call update() */
    shared.setStyle(LineLayerCommonStyleUniform{},
        {LineLayerStyleUniform{}},
        {{}},
        {});

    struct Layer: LineLayer {
        explicit Layer(LayerHandle handle, Shared& shared): LineLayer{handle, shared} {}

        const State& stateData() const {
            return static_cast<const State&>(*_state);
        }
    } layer{layerHandle(0, 1), shared};

    /* Required to be called before update() (because AbstractUserInterface
       guarantees the same on a higher level), not needed for anything here */
    layer.setSize({1, 1}, {1, 1});

    /* Empty loop shouldn't produce anything. Attaching all of these to a
       non-null NodeHandle to make update() below work. */
    DataHandle emptyLoop = layer.createLoop(0, {}, {}, nodeHandle(0, 1));

    /* Loop with just a single point is a point, loop with two points is then
       coming back to the first */
    DataHandle singlePoint = layer.createLoop(0, {{}}, {}, nodeHandle(0, 1));

    /* Verify inde & neighbor calculation works even for loops larger than few
       points */
    DataHandle largeLoop = layer.createLoop(0, {
        {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}
    }, {}, nodeHandle(0, 1));

    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().data).slice(&Implementation::LineLayerData::run), Containers::arrayView({
        0u, 1u, 2u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::indexCount), Containers::arrayView({
        0u, 2u, 22u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::indexOffset), Containers::arrayView({
        0u, 0u, 2u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::pointCount), Containers::arrayView({
        0u, 1u, 11u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::pointOffset), Containers::arrayView({
        0u, 0u, 1u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::data), Containers::arrayView({
        0u, 1u, 2u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::joinCount), Containers::arrayView({
        0u, 0u, 11*2u
    }), TestSuite::Compare::Container);
    Containers::Pair<UnsignedInt, UnsignedInt> expectedPointIndices[]{
        /* empty */

        /* singlePoint */
        {0, 0xffffffffu}, {0, 0xffffffffu},

        /* largeLoop. Matches what's in createSetIndicesNeighbors(), see there
           for details. */
        { 0, 20}, { 1,  3},         /* 0*/
        { 1,  0}, { 2,  5},         /* 2*/
        { 2,  2}, { 3,  7},         /* 4*/
        { 3,  4}, { 4,  9},         /* 6*/
        { 4,  6}, { 5, 11},         /* 8*/
        { 5,  8}, { 6, 13},         /*10*/
        { 6, 10}, { 7, 15},         /*12*/
        { 7, 12}, { 8, 17},         /*14*/
        { 8, 14}, { 9, 19},         /*16*/
        { 9, 16}, {10, 21},         /*18*/
        {10, 18}, { 0,  1},         /*20*/
    };
    CORRADE_COMPARE_AS((Containers::arrayCast<Containers::Pair<UnsignedInt, UnsignedInt>>(layer.stateData().pointIndices)),
        Containers::arrayView(expectedPointIndices),
        TestSuite::Compare::Container);
    /* Just to verify the point data get copied / initialized at all */
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().points).slice(&Implementation::LineLayerPoint::position),
        Containers::stridedArrayView<Vector2>({{}}).broadcasted<0>(12),
        TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().points).slice(&Implementation::LineLayerPoint::color),
        Containers::stridedArrayView<Color4>({0xffffff_rgbf}).broadcasted<0>(12),
        TestSuite::Compare::Container);

    CORRADE_COMPARE(layer.state(), LayerState::NeedsNodeOffsetSizeUpdate|LayerState::NeedsAttachmentUpdate|LayerState::NeedsDataUpdate);

    /* Perform an update to clear state flags as well as verify the index /
       vertex buffer population doesn't trip up on any of these. Explicitly
       listing the data to make it possible to test just a subset in case
       update() blows up on some of those. */
    UnsignedInt dataIds[]{
        dataHandleId(emptyLoop),
        dataHandleId(singlePoint),
        dataHandleId(largeLoop)
    };
    CORRADE_COMPARE(Containers::arraySize(dataIds), layer.usedCount());
    Vector2 nodeOffsets[1];
    Vector2 nodeSizes[1];
    Float nodeOpacities[1];
    UnsignedByte nodesEnabled[1]{};
    layer.update(layer.state(), dataIds, {}, {}, nodeOffsets, nodeSizes, nodeOpacities, Containers::BitArrayView{nodesEnabled, 0, 1}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Setting the data to the same lines should change nothing compared to
       above, but even an empty call should mark the layer as dirty */

    layer.setLineLoop(emptyLoop, {}, {});
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    layer.setLineLoop(singlePoint, {{}}, {});
    layer.setLineLoop(largeLoop, {
        {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}
    }, {});
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().data).slice(&Implementation::LineLayerData::run), Containers::arrayView({
        0u, 1u, 2u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::indexCount), Containers::arrayView({
        0u, 2u, 22u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::indexOffset), Containers::arrayView({
        0u, 0u, 2u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::pointCount), Containers::arrayView({
        0u, 1u, 11u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::pointOffset), Containers::arrayView({
        0u, 0u, 1u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::data), Containers::arrayView({
        0u, 1u, 2u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS((Containers::arrayCast<Containers::Pair<UnsignedInt, UnsignedInt>>(layer.stateData().pointIndices)),
        Containers::arrayView(expectedPointIndices),
        TestSuite::Compare::Container);
    /* Just to verify the point data get copied / initialized at all */
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().points).slice(&Implementation::LineLayerPoint::position),
        Containers::stridedArrayView<Vector2>({{}}).broadcasted<0>(12),
        TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().points).slice(&Implementation::LineLayerPoint::color),
        Containers::stridedArrayView<Color4>({0xffffff_rgbf}).broadcasted<0>(12),
        TestSuite::Compare::Container);
}

void LineLayerTest::createStyleOutOfRange() {
    CORRADE_SKIP_IF_NO_ASSERT();

    /* In this case the uniform count is higher than the style count, which is
       unlikely to happen in practice. It's to verify the check happens against
       the style count, not uniform count. */
    struct LayerShared: LineLayer::Shared {
        explicit LayerShared(const Configuration& configuration): LineLayer::Shared{configuration} {}

        void doSetStyle(const LineLayerCommonStyleUniform&, Containers::ArrayView<const LineLayerStyleUniform>) override {}
    } shared{LineLayer::Shared::Configuration{6, 3}};

    struct Layer: LineLayer {
        explicit Layer(LayerHandle handle, Shared& shared): LineLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    Containers::String out;
    Error redirectError{&out};
    layer.create(3, {}, {}, {});
    layer.createStrip(3, {}, {});
    layer.createLoop(3, {}, {});
    CORRADE_COMPARE_AS(out,
        "Ui::LineLayer::create(): style 3 out of range for 3 styles\n"
        "Ui::LineLayer::createStrip(): style 3 out of range for 3 styles\n"
        "Ui::LineLayer::createLoop(): style 3 out of range for 3 styles\n",
        TestSuite::Compare::String);
}

void LineLayerTest::setColor() {
    struct LayerShared: LineLayer::Shared {
        explicit LayerShared(const Configuration& configuration): LineLayer::Shared{configuration} {}

        void doSetStyle(const LineLayerCommonStyleUniform&, Containers::ArrayView<const LineLayerStyleUniform>) override {}
    } shared{LineLayer::Shared::Configuration{1}};

    /* Needed in order to be able to call update() */
    shared.setStyle(LineLayerCommonStyleUniform{},
        {LineLayerStyleUniform{}},
        {{}},
        {});

    struct Layer: LineLayer {
        explicit Layer(LayerHandle handle, Shared& shared): LineLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    /* Required to be called before update() (because AbstractUserInterface
       guarantees the same on a higher level), not needed for anything here */
    layer.setSize({1, 1}, {1, 1});

    /* Just to be sure the setters aren't picking up the first ever data
       always */
    layer.create(0, {}, {}, {});

    DataHandle data = layer.create(0, {}, {}, {});
    CORRADE_COMPARE(layer.color(data), 0xffffffff_rgbaf);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Setting a color marks the layer as dirty */
    layer.setColor(data, 0xaabbccdd_rgbaf);
    CORRADE_COMPARE(layer.color(data), 0xaabbccdd_rgbaf);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Testing also the other overload */
    layer.setColor(dataHandleData(data), 0x11223344_rgbaf);
    CORRADE_COMPARE(layer.color(dataHandleData(data)), 0x11223344_rgbaf);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);
}

void LineLayerTest::setAlignment() {
    struct LayerShared: LineLayer::Shared {
        explicit LayerShared(const Configuration& configuration): LineLayer::Shared{configuration} {}

        void doSetStyle(const LineLayerCommonStyleUniform&, Containers::ArrayView<const LineLayerStyleUniform>) override {}
    } shared{LineLayer::Shared::Configuration{1}};

    /* Needed in order to be able to call update() */
    shared.setStyle(LineLayerCommonStyleUniform{},
        {LineLayerStyleUniform{}},
        {{}},
        {});

    struct Layer: LineLayer {
        explicit Layer(LayerHandle handle, Shared& shared): LineLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    /* Required to be called before update() (because AbstractUserInterface
       guarantees the same on a higher level), not needed for anything here */
    layer.setSize({1, 1}, {1, 1});

    /* Just to be sure the setters aren't picking up the first ever data
       always */
    layer.create(0, {}, {}, {});

    DataHandle data = layer.create(0, {}, {}, {});
    CORRADE_COMPARE(layer.alignment(data), Containers::NullOpt);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Setting an alignment marks the layer as dirty */
    layer.setAlignment(data, LineAlignment::MiddleRight);
    CORRADE_COMPARE(layer.alignment(data), LineAlignment::MiddleRight);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Testing also the other overload */
    layer.setAlignment(dataHandleData(data), LineAlignment::BottomCenter);
    CORRADE_COMPARE(layer.alignment(dataHandleData(data)), LineAlignment::BottomCenter);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Resetting back to style-provided alignment marks the layer as dirty as
       well */
    layer.setAlignment(data, {});
    CORRADE_COMPARE(layer.alignment(data), Containers::NullOpt);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);
}

void LineLayerTest::setPadding() {
    struct LayerShared: LineLayer::Shared {
        explicit LayerShared(const Configuration& configuration): LineLayer::Shared{configuration} {}

        void doSetStyle(const LineLayerCommonStyleUniform&, Containers::ArrayView<const LineLayerStyleUniform>) override {}
    } shared{LineLayer::Shared::Configuration{1}};

    /* Needed in order to be able to call update() */
    shared.setStyle(LineLayerCommonStyleUniform{},
        {LineLayerStyleUniform{}},
        {{}},
        {});

    struct Layer: LineLayer {
        explicit Layer(LayerHandle handle, Shared& shared): LineLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    /* Required to be called before update() (because AbstractUserInterface
       guarantees the same on a higher level), not needed for anything here */
    layer.setSize({1, 1}, {1, 1});

    /* Just to be sure the setters aren't picking up the first ever data
       always */
    layer.create(0, {}, {}, {});

    DataHandle data = layer.create(0, {}, {}, {});
    CORRADE_COMPARE(layer.padding(data), Vector4{0.0f});
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Setting a padding marks the layer as dirty */
    layer.setPadding(data, {2.0f, 4.0f, 3.0f, 1.0f});
    CORRADE_COMPARE(layer.padding(data), (Vector4{2.0f, 4.0f, 3.0f, 1.0f}));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Testing also the other overload */
    layer.setPadding(dataHandleData(data), {1.0f, 2.0f, 3.0f, 4.0f});
    CORRADE_COMPARE(layer.padding(dataHandleData(data)), (Vector4{1.0f, 2.0f, 3.0f, 4.0f}));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Single-value padding */
    layer.setPadding(data, 4.0f);
    CORRADE_COMPARE(layer.padding(data), Vector4{4.0f});
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Testing also the other overload */
    layer.setPadding(dataHandleData(data), 3.0f);
    CORRADE_COMPARE(layer.padding(dataHandleData(data)), Vector4{3.0f});
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);
}

void LineLayerTest::invalidHandle() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct LayerShared: LineLayer::Shared {
        explicit LayerShared(const Configuration& configuration): LineLayer::Shared{configuration} {}

        void doSetStyle(const LineLayerCommonStyleUniform&, Containers::ArrayView<const LineLayerStyleUniform>) override {}
    } shared{LineLayer::Shared::Configuration{1}};

    struct Layer: LineLayer {
        explicit Layer(LayerHandle handle, Shared& shared): LineLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    Containers::String out;
    Error redirectError{&out};
    layer.indexCount(DataHandle::Null);
    layer.indexCount(LayerDataHandle::Null);
    layer.pointCount(DataHandle::Null);
    layer.pointCount(LayerDataHandle::Null);
    layer.setLine(DataHandle::Null, {}, {}, {});
    layer.setLine(LayerDataHandle::Null, {}, {}, {});
    layer.setLineStrip(DataHandle::Null, {}, {});
    layer.setLineStrip(LayerDataHandle::Null, {}, {});
    layer.setLineLoop(DataHandle::Null, {}, {});
    layer.setLineLoop(LayerDataHandle::Null, {}, {});
    layer.color(DataHandle::Null);
    layer.color(LayerDataHandle::Null);
    layer.setColor(DataHandle::Null, {});
    layer.setColor(LayerDataHandle::Null, {});
    layer.alignment(DataHandle::Null);
    layer.alignment(LayerDataHandle::Null);
    layer.setAlignment(DataHandle::Null, {});
    layer.setAlignment(LayerDataHandle::Null, {});
    layer.padding(DataHandle::Null);
    layer.padding(LayerDataHandle::Null);
    layer.setPadding(DataHandle::Null, {});
    layer.setPadding(LayerDataHandle::Null, {});
    CORRADE_COMPARE_AS(out,
        "Ui::LineLayer::indexCount(): invalid handle Ui::DataHandle::Null\n"
        "Ui::LineLayer::indexCount(): invalid handle Ui::LayerDataHandle::Null\n"
        "Ui::LineLayer::pointCount(): invalid handle Ui::DataHandle::Null\n"
        "Ui::LineLayer::pointCount(): invalid handle Ui::LayerDataHandle::Null\n"
        "Ui::LineLayer::setLine(): invalid handle Ui::DataHandle::Null\n"
        "Ui::LineLayer::setLine(): invalid handle Ui::LayerDataHandle::Null\n"
        "Ui::LineLayer::setLineStrip(): invalid handle Ui::DataHandle::Null\n"
        "Ui::LineLayer::setLineStrip(): invalid handle Ui::LayerDataHandle::Null\n"
        "Ui::LineLayer::setLineLoop(): invalid handle Ui::DataHandle::Null\n"
        "Ui::LineLayer::setLineLoop(): invalid handle Ui::LayerDataHandle::Null\n"
        "Ui::LineLayer::color(): invalid handle Ui::DataHandle::Null\n"
        "Ui::LineLayer::color(): invalid handle Ui::LayerDataHandle::Null\n"
        "Ui::LineLayer::setColor(): invalid handle Ui::DataHandle::Null\n"
        "Ui::LineLayer::setColor(): invalid handle Ui::LayerDataHandle::Null\n"
        "Ui::LineLayer::alignment(): invalid handle Ui::DataHandle::Null\n"
        "Ui::LineLayer::alignment(): invalid handle Ui::LayerDataHandle::Null\n"
        "Ui::LineLayer::setAlignment(): invalid handle Ui::DataHandle::Null\n"
        "Ui::LineLayer::setAlignment(): invalid handle Ui::LayerDataHandle::Null\n"
        "Ui::LineLayer::padding(): invalid handle Ui::DataHandle::Null\n"
        "Ui::LineLayer::padding(): invalid handle Ui::LayerDataHandle::Null\n"
        "Ui::LineLayer::setPadding(): invalid handle Ui::DataHandle::Null\n"
        "Ui::LineLayer::setPadding(): invalid handle Ui::LayerDataHandle::Null\n",
        TestSuite::Compare::String);
}

void LineLayerTest::createSetInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct LayerShared: LineLayer::Shared {
        explicit LayerShared(const Configuration& configuration): LineLayer::Shared{configuration} {}

        void doSetStyle(const LineLayerCommonStyleUniform&, Containers::ArrayView<const LineLayerStyleUniform>) override {}
    } shared{LineLayer::Shared::Configuration{1}};

    struct Layer: LineLayer {
        explicit Layer(LayerHandle handle, Shared& shared): LineLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    UnsignedInt indices[8]{};
    UnsignedInt indicesWrong[7]{};
    Vector2 points[5];
    Vector2 onePoint[1];
    Vector2 twoPoints[2];
    Color4 colors[5];
    Color4 colorsWrong[6];

    DataHandle data = layer.create(0, indices, points, colors);
    DataHandle data2 = layer.create(0, indices, points, colors);

    /* Supplying no colors is okay */
    layer.create(0, indices, points, {});
    layer.createStrip(0, points, {});
    layer.createLoop(0, points, {});

    Containers::String out;
    Error redirectError{&out};
    layer.create(0, indicesWrong, points, colors);
    layer.setLine(data, indicesWrong, points, colors);
    /* The above creates a new run with a (wrong) index count so setting the
       data again won't fire the same index assert. Use another data for it. */
    layer.setLine(dataHandleData(data2), indicesWrong, points, colors);
    layer.create(0, indices, points, colorsWrong);
    layer.setLine(data, indices, points, colorsWrong);
    layer.setLine(dataHandleData(data), indices, points, colorsWrong);
    /* Indices out of range tested in createSetIndicesOutOfRange() to verify
       behavior with setLine() as well */
    layer.createStrip(0, points, colorsWrong);
    layer.setLineStrip(data, points, colorsWrong);
    layer.setLineStrip(dataHandleData(data), points, colorsWrong);
    layer.createLoop(0, points, colorsWrong);
    layer.setLineLoop(data, points, colorsWrong);
    layer.setLineLoop(dataHandleData(data), points, colorsWrong);
    layer.createStrip(0, onePoint, {});
    layer.setLineStrip(data, onePoint, {});
    layer.setLineStrip(dataHandleData(data), onePoint, {});
    layer.createLoop(0, twoPoints, {});
    layer.setLineLoop(data, twoPoints, {});
    layer.setLineLoop(dataHandleData(data), twoPoints, {});
    CORRADE_COMPARE_AS(out,
        "Ui::LineLayer::create(): expected index count to be divisible by 2 but got 7\n"
        "Ui::LineLayer::setLine(): expected index count to be divisible by 2 but got 7\n"
        "Ui::LineLayer::setLine(): expected index count to be divisible by 2 but got 7\n"
        "Ui::LineLayer::create(): expected either no or 5 colors, got 6\n"
        "Ui::LineLayer::setLine(): expected either no or 5 colors, got 6\n"
        "Ui::LineLayer::setLine(): expected either no or 5 colors, got 6\n"
        "Ui::LineLayer::createStrip(): expected either no or 5 colors, got 6\n"
        "Ui::LineLayer::setLineStrip(): expected either no or 5 colors, got 6\n"
        "Ui::LineLayer::setLineStrip(): expected either no or 5 colors, got 6\n"
        "Ui::LineLayer::createLoop(): expected either no or 5 colors, got 6\n"
        "Ui::LineLayer::setLineLoop(): expected either no or 5 colors, got 6\n"
        "Ui::LineLayer::setLineLoop(): expected either no or 5 colors, got 6\n"
        "Ui::LineLayer::createStrip(): expected either no or at least two points, got 1\n"
        "Ui::LineLayer::setLineStrip(): expected either no or at least two points, got 1\n"
        "Ui::LineLayer::setLineStrip(): expected either no or at least two points, got 1\n"
        "Ui::LineLayer::createLoop(): expected either no, one or at least three points, got 2\n"
        "Ui::LineLayer::setLineLoop(): expected either no, one or at least three points, got 2\n"
        "Ui::LineLayer::setLineLoop(): expected either no, one or at least three points, got 2\n",
        TestSuite::Compare::String);
}

void LineLayerTest::createSetIndicesOutOfRange() {
    CORRADE_SKIP_IF_NO_DEBUG_ASSERT();

    /* Is not in createInvalid() because the assert is debug-only */

    struct LayerShared: LineLayer::Shared {
        explicit LayerShared(const Configuration& configuration): LineLayer::Shared{configuration} {}

        void doSetStyle(const LineLayerCommonStyleUniform&, Containers::ArrayView<const LineLayerStyleUniform>) override {}
    } shared{LineLayer::Shared::Configuration{1}};

    struct Layer: LineLayer {
        explicit Layer(LayerHandle handle, Shared& shared): LineLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    DataHandle data = layer.create(0, {}, {}, {});

    Containers::String out;
    Error redirectError{&out};
    layer.create(0, {0, 2, 3, 1, 0, 4, 2, 1}, {{}, {}, {}, {}}, {});
    layer.setLine(data, {0, 2, 3, 1, 0, 4, 2, 1}, {{}, {}, {}, {}}, {});
    layer.setLine(dataHandleData(data), {0, 2, 3, 1, 0, 4, 2, 1}, {{}, {}, {}, {}}, {});
    CORRADE_COMPARE_AS(out,
        "Ui::LineLayer::create(): index 4 out of range for 4 points at index 5\n"
        "Ui::LineLayer::setLine(): index 4 out of range for 4 points at index 5\n"
        "Ui::LineLayer::setLine(): index 4 out of range for 4 points at index 5\n",
        TestSuite::Compare::String);
}

void LineLayerTest::updateEmpty() {
    struct LayerShared: LineLayer::Shared {
        explicit LayerShared(const Configuration& configuration): LineLayer::Shared{configuration} {}

        void doSetStyle(const LineLayerCommonStyleUniform&, Containers::ArrayView<const LineLayerStyleUniform>) override {}
    } shared{LineLayer::Shared::Configuration{1}};
    shared.setStyle(LineLayerCommonStyleUniform{},
        {LineLayerStyleUniform{}},
        {{}},
        {});

    struct Layer: LineLayer {
        explicit Layer(LayerHandle handle, Shared& shared): LineLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    /* Required to be called before update() (because AbstractUserInterface
       guarantees the same on a higher level), not needed for anything here */
    layer.setSize({1, 1}, {1, 1});

    /* Shouldn't crash or do anything weird */
    layer.update(LayerState::NeedsNodeEnabledUpdate|LayerState::NeedsNodeOpacityUpdate|LayerState::NeedsNodeOrderUpdate|LayerState::NeedsNodeOffsetSizeUpdate|LayerState::NeedsAttachmentUpdate|LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate|LayerState::NeedsSharedDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_VERIFY(true);
}

void LineLayerTest::updateCleanDataOrder() {
    auto&& data = UpdateCleanDataOrderData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Does just extremely basic verification that the vertex and index data
       get filled with correct contents and in correct order depending on
       LayerStates passed in. The actual visual output is checked in
       LineLayerGLTest. */

    struct LayerShared: LineLayer::Shared {
        explicit LayerShared(const Configuration& configuration): LineLayer::Shared{configuration} {}

        void doSetStyle(const LineLayerCommonStyleUniform&, Containers::ArrayView<const LineLayerStyleUniform>) override {}
    } shared{LineLayer::Shared::Configuration{4, 6}};

    shared.setStyle(LineLayerCommonStyleUniform{},
        {LineLayerStyleUniform{}, LineLayerStyleUniform{},
         LineLayerStyleUniform{}, LineLayerStyleUniform{}},
        /* Style 5 doesn't get used (gets transitioned to 2), use an
           otherwise unused uniform index and weird alignment + padding to
           verify it doesn't get picked. Style 0 isn't used now either. */
        {1, 2, 0, 1, 1, 3},
        {LineAlignment(0xff),
         LineAlignment::MiddleLeft,
         LineAlignment::BottomRight,
         LineAlignment::TopLeft,
         LineAlignment::TopCenter,
         LineAlignment(0xff)},
        {Vector4{666}, {}, data.paddingFromStyle, {}, data.paddingFromStyle, Vector4{666}});

    /* Transition for disabled node6, which is done below through the
       nodesEnabled (that has only node15 enabled) view passed to update() */
    shared.setStyleTransition(
        nullptr,
        nullptr,
        nullptr,
        [](UnsignedInt style) {
            return style == 5 ? 2u : style;
        }
    );

    struct Layer: LineLayer {
        explicit Layer(LayerHandle handle, Shared& shared): LineLayer{handle, shared} {}

        const State& stateData() const {
            return static_cast<const State&>(*_state);
        }
    } layer{layerHandle(0, 1), shared};

    /* Required to be called before update() (because AbstractUserInterface
       guarantees the same on a higher level), not needed for anything here */
    layer.setSize({1, 1}, {1, 1});

    /* Two node handles to attach the data to. Generation doesn't matter, just
       has to be non-zero. */
    NodeHandle node6 = nodeHandle(6, 0x111);
    NodeHandle node15 = nodeHandle(15, 0xccc);

    /* Create 10 data handles. Four get filled and actually used, two more are
       filled to be non-empty but aren't checked in any way, just there to
       verify that the recompaction works correctly. */
    layer.create(0, {}, {}, {});            /* 0 */
    layer.create(0, {0, 0}, {{}}, {});      /* 1, quad 0 */
    layer.create(0, {}, {}, {});            /* 2 */
    /* Node 6 is disabled, so style 5 should get transitioned to 2. There
       should be one join and two caps. */
    DataHandle data3 = layer.createStrip(5, /* 3, quad 1 to 2 */
        {{1.0f, 2.0f}, {3.0f, 4.0f}, {5.0f, 6.0f}}, {}, node6);
    layer.create(0, {}, {}, {});            /* 4 */
    /* Node 6 is disabled, but style 4 has no disabled transition so this stays
       the same. This is two separate segments with per-point colors. */
    DataHandle data5 = layer.create(4,      /* 5, quad 3 to 4 */
        {2, 1, 3, 0},
        {{0.1f, 0.2f}, {0.3f, 0.4f}, {0.5f, 0.6f}, {0.7f, 0.8f}},
        {0x333333_rgbf, 0x666666_rgbf, 0x999999_rgbf, 0xcccccc_rgbf},
        node6);
    layer.create(0, {0, 0}, {{}}, {});      /* 6, quad 5 */
    /* Three segments with three joins and no cap */
    DataHandle data7 = layer.createLoop(1,  /* 7, quad 6 to 8 */
        {{-1.0f, -1.0f}, {+1.0f, -1.0f}, {0.0f, +1.0f}},
        {},
        node15);
    /* This one has two points but no indices. Shouldn't cause any issues
       during recompaction, the quads also don't appear in the vertex data */
    layer.create(0, {}, {{}, {}}, {});      /* 8 */
    /* Three segments connecting at the middle with no joins and six caps */
    DataHandle data9 = layer.create(3,      /* 9, quad 9 to 11 */
        {0, 1, 0, 2, 0, 3},
        {{}, {1.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f}},
        {},
        node15);

    /* These are further multiplied by the node opacities */
    layer.setColor(data3, 0xff336699_rgbaf);
    layer.setColor(data5, 0xcceeff00_rgbaf);
    layer.setColor(data7, 0x11223344_rgbaf);
    layer.setColor(data9, 0x663399_rgbf);

    if(!data.paddingFromData.isZero()) {
        layer.setPadding(data3, data.paddingFromData);
        layer.setPadding(data5, data.paddingFromData);
    }

    /* There should be 10 runs, assigned to the 10 data. These are just for
       verification, the actual processing in doUpdate() then duplicates them,
       removing all association with the input index buffer. */
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().data).slice(&Implementation::LineLayerData::run), Containers::arrayView({
        0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::indexCount), Containers::arrayView({
        0u, 2u, 0u, 4u, 0u, 4u, 2u, 6u, 0u, 6u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::indexOffset), Containers::arrayView({
        0u, 0u, 2u, 2u, 6u, 6u, 10u, 12u, 18u, 18u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::pointCount), Containers::arrayView({
        0u, 1u, 0u, 3u, 0u, 4u, 1u, 3u, 2u, 4u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::pointOffset), Containers::arrayView({
        0u, 0u, 1u, 1u, 4u, 4u, 8u, 9u, 12u, 14u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::data), Containers::arrayView({
        0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u
    }), TestSuite::Compare::Container);

    Vector2 nodeOffsets[16];
    Vector2 nodeSizes[16];
    Float nodeOpacities[16];
    UnsignedByte nodesEnabledData[2]{};
    Containers::MutableBitArrayView nodesEnabled{nodesEnabledData, 0, 16};
    nodeOffsets[6] = data.node6Offset;
    nodeSizes[6] = data.node6Size;
    nodeOpacities[6] = 0.4f;
    nodeOffsets[15] = {3.0f, 4.0f};
    nodeSizes[15] = {20.0f, 5.0f};
    nodeOpacities[15] = 0.9f;
    nodesEnabled.set(15);

    /* An empty update should generate an empty draw list */
    if(data.emptyUpdate) {
        layer.update(data.states, {}, {}, {}, nodeOffsets, nodeSizes, nodeOpacities, nodesEnabled, {}, {}, {}, {});
        CORRADE_VERIFY(data.expectIndexDataUpdated);
        CORRADE_COMPARE_AS(layer.stateData().indices,
            Containers::ArrayView<const UnsignedInt>{},
            TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(layer.stateData().indexDrawOffsets, (Containers::arrayView<UnsignedInt>({
            0,
        })), TestSuite::Compare::Container);
        return;
    }

    /* Just the filled subset is getting updated, and just what was selected in
       states */
    UnsignedInt dataIds[]{9, 5, 7, 3};
    layer.update(data.states, dataIds, {}, {}, nodeOffsets, nodeSizes, nodeOpacities, nodesEnabled, {}, {}, {}, {});

    if(data.expectIndexDataUpdated) {
        /* The indices should be filled just for the four items */
        CORRADE_COMPARE_AS(layer.stateData().indices, Containers::arrayView<UnsignedInt>({
            /* Line 9, quads 9 to 11 */
             9*4 + 2,  9*4 + 0,  9*4 + 1,  9*4 + 1,  9*4 + 3,  9*4 + 2,
            10*4 + 2, 10*4 + 0, 10*4 + 1, 10*4 + 1, 10*4 + 3, 10*4 + 2,
            11*4 + 2, 11*4 + 0, 11*4 + 1, 11*4 + 1, 11*4 + 3, 11*4 + 2,

            /* Line 5, quads 3 to 4 */
             3*4 + 2,  3*4 + 0,  3*4 + 1,  3*4 + 1,  3*4 + 3,  3*4 + 2,
             4*4 + 2,  4*4 + 0,  4*4 + 1,  4*4 + 1,  4*4 + 3,  4*4 + 2,

            /* Line 7, quads 6 to 8, three joins in between */
             6*4 + 2,  6*4 + 0,  6*4 + 1,  6*4 + 1,  6*4 + 3,  6*4 + 2,
             6*4 + 2,  6*4 + 3,  8*4 + 2,  8*4 + 2,  6*4 + 3,  8*4 + 3, /* join */
             6*4 + 2,  6*4 + 3,  7*4 + 0,  7*4 + 0,  6*4 + 3,  7*4 + 1, /* join */
             7*4 + 2,  7*4 + 0,  7*4 + 1,  7*4 + 1,  7*4 + 3,  7*4 + 2,
             7*4 + 2,  7*4 + 3,  8*4 + 0,  8*4 + 0,  7*4 + 3,  8*4 + 1, /* join */
             8*4 + 2,  8*4 + 0,  8*4 + 1,  8*4 + 1,  8*4 + 3,  8*4 + 2,

            /* Line 3, quad 1 to 2, one join in between */
             1*4 + 2,  1*4 + 0,  1*4 + 1,  1*4 + 1,  1*4 + 3,  1*4 + 2,
             1*4 + 2,  1*4 + 3,  2*4 + 0,  2*4 + 0,  1*4 + 3,  2*4 + 1, /* join */
             2*4 + 2,  2*4 + 0,  2*4 + 1,  2*4 + 1,  2*4 + 3,  2*4 + 2,
        }), TestSuite::Compare::Container);
    }

    constexpr UnsignedInt Begin = Implementation::LineVertexAnnotationBegin;
    constexpr UnsignedInt Up = Implementation::LineVertexAnnotationUp;
    constexpr UnsignedInt Join = Implementation::LineVertexAnnotationJoin;
    if(data.expectVertexDataUpdated) {
        /* The vertices are there for all data, but only the actually used are
           filled */
        CORRADE_COMPARE(layer.stateData().vertices.size(), 12*4);

        /* Line 3, quad 1 to 2; 7, quad 6 to 8; and 9, quad 9 to 11, have all
           default white per-point color, but it's multiplied by the per-data
           color, and by node opacity */

        /* Line 3, quad 1 to 2, has a join marked in annotations; together with
           a style that's transitioned from 5 to 2, which is uniform 0 */
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().vertices.slice(1*4, 3*4)).slice(&Implementation::LineLayerVertex::annotationStyleUniform), Containers::arrayView<UnsignedInt>({
            (0 << 3)|Begin|Up,
            (0 << 3)|Begin,
            (0 << 3)|Up|Join,
            (0 << 3)|Join,

            (0 << 3)|Begin|Up|Join,
            (0 << 3)|Begin|Join,
            (0 << 3)|Up,
            (0 << 3),
        }), TestSuite::Compare::Container);
        /* Attached to node 6 and style 2, which aligns to bottom right, thus
           the origin is shifted to {11, 17} */
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().vertices.slice(1*4, 3*4)).slice(&Implementation::LineLayerVertex::position), Containers::arrayView<Vector2>({
            {11.0f + 1.0f, 17.0f + 2.0f},
            {11.0f + 1.0f, 17.0f + 2.0f},
            {11.0f + 3.0f, 17.0f + 4.0f},
            {11.0f + 3.0f, 17.0f + 4.0f},

            {11.0f + 3.0f, 17.0f + 4.0f},
            {11.0f + 3.0f, 17.0f + 4.0f},
            {11.0f + 5.0f, 17.0f + 6.0f},
            {11.0f + 5.0f, 17.0f + 6.0f},
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().vertices.slice(1*4, 3*4)).slice(&Implementation::LineLayerVertex::previousPosition), Containers::arrayView<Vector2>({
            {11.0f, 17.0f}, /* unused, just the shift alone */
            {11.0f, 17.0f}, /* unused, just the shift alone */
            {11.0f + 1.0f, 17.0f + 2.0f},
            {11.0f + 1.0f, 17.0f + 2.0f},

            {11.0f + 1.0f, 17.0f + 2.0f},
            {11.0f + 1.0f, 17.0f + 2.0f},
            {11.0f + 3.0f, 17.0f + 4.0f},
            {11.0f + 3.0f, 17.0f + 4.0f},
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().vertices.slice(1*4, 3*4)).slice(&Implementation::LineLayerVertex::nextPosition), Containers::arrayView<Vector2>({
            {11.0f + 3.0f, 17.0f + 4.0f},
            {11.0f + 3.0f, 17.0f + 4.0f},
            {11.0f + 5.0f, 17.0f + 6.0f},
            {11.0f + 5.0f, 17.0f + 6.0f},

            {11.0f + 5.0f, 17.0f + 6.0f},
            {11.0f + 5.0f, 17.0f + 6.0f},
            {11.0f, 17.0f}, /* unused, just the shift alone */
            {11.0f, 17.0f}, /* unused, just the shift alone */
        }), TestSuite::Compare::Container);
        /* Default white per-point color, but it's multiplied by the per-data
           color, and by node6 opacity */
        for(std::size_t i: {1, 2}) {
            CORRADE_ITERATION(i);
            for(std::size_t j = 0; j != 4; ++j) {
                CORRADE_ITERATION(j);
                CORRADE_COMPARE(layer.stateData().vertices[i*4 + j].color, 0xff336699_rgbaf*0.4f);
            }
        }

        /* Line 5, quad 3 to 4, has style 4 with no transition, which is
           uniform 1. No join. */
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().vertices.slice(3*4, 5*4)).slice(&Implementation::LineLayerVertex::annotationStyleUniform), Containers::arrayView<UnsignedInt>({
            (1 << 3)|Begin|Up,
            (1 << 3)|Begin,
            (1 << 3)|Up,
            (1 << 3),

            (1 << 3)|Begin|Up,
            (1 << 3)|Begin,
            (1 << 3)|Up,
            (1 << 3),
        }), TestSuite::Compare::Container);
        /* Attached to node 6 and style 4, which aligns to top center, thus
           the origin is shifted to {6, 2} */
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().vertices.slice(3*4, 5*4)).slice(&Implementation::LineLayerVertex::position), Containers::arrayView<Vector2>({
            {6.0f + 0.5f, 2.0f + 0.6f},
            {6.0f + 0.5f, 2.0f + 0.6f},
            {6.0f + 0.3f, 2.0f + 0.4f},
            {6.0f + 0.3f, 2.0f + 0.4f},

            {6.0f + 0.7f, 2.0f + 0.8f},
            {6.0f + 0.7f, 2.0f + 0.8f},
            {6.0f + 0.1f, 2.0f + 0.2f},
            {6.0f + 0.1f, 2.0f + 0.2f},
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().vertices.slice(3*4, 5*4)).slice(&Implementation::LineLayerVertex::previousPosition), Containers::arrayView<Vector2>({
            {6.0f, 2.0f}, /* unused, just the shift alone */
            {6.0f, 2.0f}, /* unused, just the shift alone */
            {6.0f + 0.5f, 2.0f + 0.6f},
            {6.0f + 0.5f, 2.0f + 0.6f},

            {6.0f, 2.0f}, /* unused, just the shift alone */
            {6.0f, 2.0f}, /* unused, just the shift alone */
            {6.0f + 0.7f, 2.0f + 0.8f},
            {6.0f + 0.7f, 2.0f + 0.8f},
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().vertices.slice(3*4, 5*4)).slice(&Implementation::LineLayerVertex::nextPosition), Containers::arrayView<Vector2>({
            {6.0f + 0.3f, 2.0f + 0.4f},
            {6.0f + 0.3f, 2.0f + 0.4f},
            {6.0f, 2.0f}, /* unused, just the shift alone */
            {6.0f, 2.0f}, /* unused, just the shift alone */

            {6.0f + 0.1f, 2.0f + 0.2f},
            {6.0f + 0.1f, 2.0f + 0.2f},
            {6.0f, 2.0f}, /* unused, just the shift alone */
            {6.0f, 2.0f}, /* unused, just the shift alone */
        }), TestSuite::Compare::Container);
        /* Custom per-point color in addition to the per-data and opacity. It
           also has a custom index buffer which reshuffles the colors. */
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().vertices.slice(3*4, 5*4)).slice(&Implementation::LineLayerVertex::color), Containers::arrayView<Color4>({
            0x999999ff_rgbaf*0xcceeff00_rgbaf*0.4f,
            0x999999ff_rgbaf*0xcceeff00_rgbaf*0.4f,
            0x666666ff_rgbaf*0xcceeff00_rgbaf*0.4f,
            0x666666ff_rgbaf*0xcceeff00_rgbaf*0.4f,

            0xccccccff_rgbaf*0xcceeff00_rgbaf*0.4f,
            0xccccccff_rgbaf*0xcceeff00_rgbaf*0.4f,
            0x333333ff_rgbaf*0xcceeff00_rgbaf*0.4f,
            0x333333ff_rgbaf*0xcceeff00_rgbaf*0.4f,
        }), TestSuite::Compare::Container);

        /* Line 7, quad 6 to 8, is a loop; plus style 1, which is uniform 2 */
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().vertices.slice(6*4, 9*4)).slice(&Implementation::LineLayerVertex::annotationStyleUniform), Containers::arrayView<UnsignedInt>({
            (2 << 3)|Begin|Up|Join,
            (2 << 3)|Begin|Join,
            (2 << 3)|Up|Join,
            (2 << 3)|Join,

            (2 << 3)|Begin|Up|Join,
            (2 << 3)|Begin|Join,
            (2 << 3)|Up|Join,
            (2 << 3)|Join,

            (2 << 3)|Begin|Up|Join,
            (2 << 3)|Begin|Join,
            (2 << 3)|Up|Join,
            (2 << 3)|Join,
        }), TestSuite::Compare::Container);
        /* Attached to node 15 and style 1, which aligns to middle left, thus
           the origin is shifted to {3, 6.5} */
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().vertices.slice(6*4, 9*4)).slice(&Implementation::LineLayerVertex::position), Containers::arrayView<Vector2>({
            {3.0f - 1.0f, 6.5f - 1.0f},
            {3.0f - 1.0f, 6.5f - 1.0f},
            {3.0f + 1.0f, 6.5f - 1.0f},
            {3.0f + 1.0f, 6.5f - 1.0f},

            {3.0f + 1.0f, 6.5f - 1.0f},
            {3.0f + 1.0f, 6.5f - 1.0f},
            {3.0f + 0.0f, 6.5f + 1.0f},
            {3.0f + 0.0f, 6.5f + 1.0f},

            {3.0f + 0.0f, 6.5f + 1.0f},
            {3.0f + 0.0f, 6.5f + 1.0f},
            {3.0f - 1.0f, 6.5f - 1.0f},
            {3.0f - 1.0f, 6.5f - 1.0f},
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().vertices.slice(6*4, 9*4)).slice(&Implementation::LineLayerVertex::previousPosition), Containers::arrayView<Vector2>({
            {3.0f + 0.0f, 6.5f + 1.0f}, /* positions rotated by four lines + */
            {3.0f + 0.0f, 6.5f + 1.0f},
            {3.0f - 1.0f, 6.5f - 1.0f},
            {3.0f - 1.0f, 6.5f - 1.0f},

            {3.0f - 1.0f, 6.5f - 1.0f},
            {3.0f - 1.0f, 6.5f - 1.0f},
            {3.0f + 1.0f, 6.5f - 1.0f},
            {3.0f + 1.0f, 6.5f - 1.0f},

            {3.0f + 1.0f, 6.5f - 1.0f},
            {3.0f + 1.0f, 6.5f - 1.0f},
            {3.0f + 0.0f, 6.5f + 1.0f},
            {3.0f + 0.0f, 6.5f + 1.0f},
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().vertices.slice(6*4, 9*4)).slice(&Implementation::LineLayerVertex::nextPosition), Containers::arrayView<Vector2>({
            {3.0f + 1.0f, 6.5f - 1.0f}, /* positions rotated by four lines - */
            {3.0f + 1.0f, 6.5f - 1.0f},
            {3.0f + 0.0f, 6.5f + 1.0f},
            {3.0f + 0.0f, 6.5f + 1.0f},

            {3.0f + 0.0f, 6.5f + 1.0f},
            {3.0f + 0.0f, 6.5f + 1.0f},
            {3.0f - 1.0f, 6.5f - 1.0f},
            {3.0f - 1.0f, 6.5f - 1.0f},

            {3.0f - 1.0f, 6.5f - 1.0f},
            {3.0f - 1.0f, 6.5f - 1.0f},
            {3.0f + 1.0f, 6.5f - 1.0f},
            {3.0f + 1.0f, 6.5f - 1.0f},
        }), TestSuite::Compare::Container);
        /* Default white per-point color, but it's multiplied by the per-data
           color, and by node15 opacity */
        for(std::size_t i: {6, 7, 8}) {
            CORRADE_ITERATION(i);
            for(std::size_t j = 0; j != 4; ++j) {
                CORRADE_ITERATION(j);
                CORRADE_COMPARE(layer.stateData().vertices[i*4 + j].color, 0x11223344_rgbaf*0.9f);
            }
        }

        /* Line 9, quad 9 to 11 has no joins; style 3, which is uniform 1 */
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().vertices.slice(9*4, 12*4)).slice(&Implementation::LineLayerVertex::annotationStyleUniform), Containers::arrayView<UnsignedInt>({
            (1 << 3)|Begin|Up,
            (1 << 3)|Begin,
            (1 << 3)|Up,
            (1 << 3),

            (1 << 3)|Begin|Up,
            (1 << 3)|Begin,
            (1 << 3)|Up,
            (1 << 3),

            (1 << 3)|Begin|Up,
            (1 << 3)|Begin,
            (1 << 3)|Up,
            (1 << 3),
        }), TestSuite::Compare::Container);
        /* Attached to node 15 and style 3, which aligns to top left, thus
           the origin is shifted to {3, 4} */
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().vertices.slice(9*4, 12*4)).slice(&Implementation::LineLayerVertex::position), Containers::arrayView<Vector2>({
            {3.0f + 0.0f, 4.0f + 0.0f},
            {3.0f + 0.0f, 4.0f + 0.0f},
            {3.0f + 1.0f, 4.0f + 0.0f},
            {3.0f + 1.0f, 4.0f + 0.0f},

            {3.0f + 0.0f, 4.0f + 0.0f},
            {3.0f + 0.0f, 4.0f + 0.0f},
            {3.0f + 0.0f, 4.0f + 1.0f},
            {3.0f + 0.0f, 4.0f + 1.0f},

            {3.0f + 0.0f, 4.0f + 0.0f},
            {3.0f + 0.0f, 4.0f + 0.0f},
            {3.0f + 1.0f, 4.0f + 1.0f},
            {3.0f + 1.0f, 4.0f + 1.0f},
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().vertices.slice(9*4, 12*4)).slice(&Implementation::LineLayerVertex::previousPosition), Containers::arrayView<Vector2>({
            {3.0f, 4.0f}, /* unused, just the shift alone */
            {3.0f, 4.0f}, /* unused, just the shift alone */
            {3.0f, 4.0f}, /* unused, just the shift alone */
            {3.0f, 4.0f}, /* unused, just the shift alone */

            {3.0f, 4.0f}, /* unused, just the shift alone */
            {3.0f, 4.0f}, /* unused, just the shift alone */
            {3.0f, 4.0f}, /* unused, just the shift alone */
            {3.0f, 4.0f}, /* unused, just the shift alone */

            {3.0f, 4.0f}, /* unused, just the shift alone */
            {3.0f, 4.0f}, /* unused, just the shift alone */
            {3.0f, 4.0f}, /* unused, just the shift alone */
            {3.0f, 4.0f}, /* unused, just the shift alone */
        }), TestSuite::Compare::Container);
        /* Default white per-point color, but it's multiplied by the per-data
           color, and by node15 opacity */
        for(std::size_t i: {9, 10, 11}) {
            CORRADE_ITERATION(i);
            for(std::size_t j = 0; j != 4; ++j) {
                CORRADE_ITERATION(j);
                CORRADE_COMPARE(layer.stateData().vertices[i*4 + j].color, 0x663399ff_rgbaf*0.9f);
            }
        }
    }

    /* Removing a node with cleanNodes() marks the corresponding run as unused,
       and update() recompacts again */
    {
        UnsignedShort nodeGenerations[16];
        nodeGenerations[6] = nodeHandleGeneration(node6) + 1;
        nodeGenerations[15] = nodeHandleGeneration(node15);
        layer.cleanNodes(nodeGenerations);
        /* Node 6 was disabled before already, so the nodesEnabled mask doesn't
           need to be updated */
    }

    /* The run corresponding to the removed data should be marked as unused,
       the rest stays the same */
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().data).slice(&Implementation::LineLayerData::run), Containers::arrayView({
        0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::indexCount), Containers::arrayView({
        0u, 2u, 0u, 4u, 0u, 4u, 2u, 6u, 0u, 6u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::indexOffset), Containers::arrayView({
        0u, 0u, 2u, 0xffffffffu, 6u, 0xffffffffu, 10u, 12u, 18u, 18u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::pointCount), Containers::arrayView({
        0u, 1u, 0u, 3u, 0u, 4u, 1u, 3u, 2u, 4u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::pointOffset), Containers::arrayView({
        0u, 0u, 1u, 0xffffffffu, 4u, 0xffffffffu, 8u, 9u, 12u, 14u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::data), Containers::arrayView({
        0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u
    }), TestSuite::Compare::Container);

    /* Note that this adds LayerState::NeedsDataUpdate in order to force the
       glyph run recompaction, thus we also don't branch on
       data.expectIndexDataUpdated / data.expectVertexDataUpdated anymore */
    UnsignedInt dataIdsPostClean[]{9, 7};
    layer.update(data.states|LayerState::NeedsDataUpdate, dataIdsPostClean, {}, {}, nodeOffsets, nodeSizes, nodeOpacities, nodesEnabled, {}, {}, {}, {});

    /* There should be just 8 runs now, assigned to the remaining 8 data */
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().data).slice(&Implementation::LineLayerData::run), Containers::arrayView({
        0u, 1u, 2u, 3u /* free data */, 3u, 5u /* free data */, 4u, 5u, 6u, 7u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::indexCount), Containers::arrayView({
        0u, 2u, 0u, 0u, 2u, 6u, 0u, 6u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::indexOffset), Containers::arrayView({
        0u, 0u, 2u, 2u, 2u, 4u, 10u, 10u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::pointCount), Containers::arrayView({
        0u, 1u, 0u, 0u, 1u, 3u, 2u, 4u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::pointOffset), Containers::arrayView({
        0u, 0u, 1u, 1u, 1u, 2u, 5u, 7u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::data), Containers::arrayView({
        0u, 1u, 2u, 4u, 6u, 7u, 8u, 9u
    }), TestSuite::Compare::Container);

    /* The index / point count queries should still match */
    CORRADE_COMPARE(layer.indexCount(data7), 6);
    CORRADE_COMPARE(layer.pointCount(data7), 3);
    CORRADE_COMPARE(layer.indexCount(data9), 6);
    CORRADE_COMPARE(layer.pointCount(data9), 4);

    /* Indices for remaining 2 visible lines */
    CORRADE_COMPARE_AS(layer.stateData().indices, Containers::arrayView<UnsignedInt>({
        /* Line 9, quads 5 to 7 */
        5*4 + 2, 5*4 + 0, 5*4 + 1, 5*4 + 1, 5*4 + 3, 5*4 + 2,
        6*4 + 2, 6*4 + 0, 6*4 + 1, 6*4 + 1, 6*4 + 3, 6*4 + 2,
        7*4 + 2, 7*4 + 0, 7*4 + 1, 7*4 + 1, 7*4 + 3, 7*4 + 2,

        /* Line 7, quads 2 to 4, three joins in between */
        2*4 + 2, 2*4 + 0, 2*4 + 1, 2*4 + 1, 2*4 + 3, 2*4 + 2,
        2*4 + 2, 2*4 + 3, 4*4 + 2, 4*4 + 2, 2*4 + 3, 4*4 + 3, /* join */
        2*4 + 2, 2*4 + 3, 3*4 + 0, 3*4 + 0, 2*4 + 3, 3*4 + 1, /* join */
        3*4 + 2, 3*4 + 0, 3*4 + 1, 3*4 + 1, 3*4 + 3, 3*4 + 2,
        3*4 + 2, 3*4 + 3, 4*4 + 0, 4*4 + 0, 3*4 + 3, 4*4 + 1, /* join */
        4*4 + 2, 4*4 + 0, 4*4 + 1, 4*4 + 1, 4*4 + 3, 4*4 + 2,
    }), TestSuite::Compare::Container);

    /* Vertices for remaining 2 visible lines */

    /* Line 7, quad 2 to 4 */
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().vertices.slice(2*4, 5*4)).slice(&Implementation::LineLayerVertex::annotationStyleUniform), Containers::arrayView<UnsignedInt>({
        (2 << 3)|Begin|Up|Join,
        (2 << 3)|Begin|Join,
        (2 << 3)|Up|Join,
        (2 << 3)|Join,

        (2 << 3)|Begin|Up|Join,
        (2 << 3)|Begin|Join,
        (2 << 3)|Up|Join,
        (2 << 3)|Join,

        (2 << 3)|Begin|Up|Join,
        (2 << 3)|Begin|Join,
        (2 << 3)|Up|Join,
        (2 << 3)|Join,
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().vertices.slice(2*4, 5*4)).slice(&Implementation::LineLayerVertex::position), Containers::arrayView<Vector2>({
        {3.0f - 1.0f, 6.5f - 1.0f},
        {3.0f - 1.0f, 6.5f - 1.0f},
        {3.0f + 1.0f, 6.5f - 1.0f},
        {3.0f + 1.0f, 6.5f - 1.0f},

        {3.0f + 1.0f, 6.5f - 1.0f},
        {3.0f + 1.0f, 6.5f - 1.0f},
        {3.0f + 0.0f, 6.5f + 1.0f},
        {3.0f + 0.0f, 6.5f + 1.0f},

        {3.0f + 0.0f, 6.5f + 1.0f},
        {3.0f + 0.0f, 6.5f + 1.0f},
        {3.0f - 1.0f, 6.5f - 1.0f},
        {3.0f - 1.0f, 6.5f - 1.0f},
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().vertices.slice(2*4, 5*4)).slice(&Implementation::LineLayerVertex::previousPosition), Containers::arrayView<Vector2>({
        {3.0f + 0.0f, 6.5f + 1.0f},
        {3.0f + 0.0f, 6.5f + 1.0f},
        {3.0f - 1.0f, 6.5f - 1.0f},
        {3.0f - 1.0f, 6.5f - 1.0f},

        {3.0f - 1.0f, 6.5f - 1.0f},
        {3.0f - 1.0f, 6.5f - 1.0f},
        {3.0f + 1.0f, 6.5f - 1.0f},
        {3.0f + 1.0f, 6.5f - 1.0f},

        {3.0f + 1.0f, 6.5f - 1.0f},
        {3.0f + 1.0f, 6.5f - 1.0f},
        {3.0f + 0.0f, 6.5f + 1.0f},
        {3.0f + 0.0f, 6.5f + 1.0f},
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().vertices.slice(2*4, 5*4)).slice(&Implementation::LineLayerVertex::nextPosition), Containers::arrayView<Vector2>({
        {3.0f + 1.0f, 6.5f - 1.0f},
        {3.0f + 1.0f, 6.5f - 1.0f},
        {3.0f + 0.0f, 6.5f + 1.0f},
        {3.0f + 0.0f, 6.5f + 1.0f},

        {3.0f + 0.0f, 6.5f + 1.0f},
        {3.0f + 0.0f, 6.5f + 1.0f},
        {3.0f - 1.0f, 6.5f - 1.0f},
        {3.0f - 1.0f, 6.5f - 1.0f},

        {3.0f - 1.0f, 6.5f - 1.0f},
        {3.0f - 1.0f, 6.5f - 1.0f},
        {3.0f + 1.0f, 6.5f - 1.0f},
        {3.0f + 1.0f, 6.5f - 1.0f},
    }), TestSuite::Compare::Container);
    for(std::size_t i: {2, 3, 4}) {
        CORRADE_ITERATION(i);
        for(std::size_t j = 0; j != 4; ++j) {
            CORRADE_ITERATION(j);
            CORRADE_COMPARE(layer.stateData().vertices[i*4 + j].color, 0x11223344_rgbaf*0.9f);
        }
    }

    /* Line 9, quad 5 to 7 */
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().vertices.slice(5*4, 8*4)).slice(&Implementation::LineLayerVertex::annotationStyleUniform), Containers::arrayView<UnsignedInt>({
        (1 << 3)|Begin|Up,
        (1 << 3)|Begin,
        (1 << 3)|Up,
        (1 << 3),

        (1 << 3)|Begin|Up,
        (1 << 3)|Begin,
        (1 << 3)|Up,
        (1 << 3),

        (1 << 3)|Begin|Up,
        (1 << 3)|Begin,
        (1 << 3)|Up,
        (1 << 3),
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().vertices.slice(5*4, 8*4)).slice(&Implementation::LineLayerVertex::position), Containers::arrayView<Vector2>({
        {3.0f + 0.0f, 4.0f + 0.0f},
        {3.0f + 0.0f, 4.0f + 0.0f},
        {3.0f + 1.0f, 4.0f + 0.0f},
        {3.0f + 1.0f, 4.0f + 0.0f},

        {3.0f + 0.0f, 4.0f + 0.0f},
        {3.0f + 0.0f, 4.0f + 0.0f},
        {3.0f + 0.0f, 4.0f + 1.0f},
        {3.0f + 0.0f, 4.0f + 1.0f},

        {3.0f + 0.0f, 4.0f + 0.0f},
        {3.0f + 0.0f, 4.0f + 0.0f},
        {3.0f + 1.0f, 4.0f + 1.0f},
        {3.0f + 1.0f, 4.0f + 1.0f},
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().vertices.slice(5*4, 8*4)).slice(&Implementation::LineLayerVertex::previousPosition), Containers::arrayView<Vector2>({
        {3.0f, 4.0f},
        {3.0f, 4.0f},
        {3.0f, 4.0f},
        {3.0f, 4.0f},

        {3.0f, 4.0f},
        {3.0f, 4.0f},
        {3.0f, 4.0f},
        {3.0f, 4.0f},

        {3.0f, 4.0f},
        {3.0f, 4.0f},
        {3.0f, 4.0f},
        {3.0f, 4.0f},
    }), TestSuite::Compare::Container);
    for(std::size_t i: {5, 6, 7}) {
        CORRADE_ITERATION(i);
        for(std::size_t j = 0; j != 4; ++j) {
            CORRADE_ITERATION(j);
            CORRADE_COMPARE(layer.stateData().vertices[i*4 + j].color, 0x663399ff_rgbaf*0.9f);
        }
    }

    /* Removing a text marks the corresponding run as unused, the next update()
       then recompacts it */
    layer.remove(data7);
    /* state() can additionally contain LayerState::NeedsNodeOffsetSizeUpdate
       if we didn't pass it to the update() above, so test just that it
       contains at least these flags */
    CORRADE_COMPARE_AS(layer.state(),
        LayerState::NeedsNodeEnabledUpdate|LayerState::NeedsAttachmentUpdate|LayerState::NeedsDataClean,
        TestSuite::Compare::GreaterOrEqual);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::indexOffset), Containers::arrayView({
        0u, 0u, 2u, 2u, 2u, 0xffffffffu, 10u, 10u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::pointOffset), Containers::arrayView({
        0u, 0u, 1u, 1u, 1u, 0xffffffffu, 5u, 7u
    }), TestSuite::Compare::Container);

    /* Again this explicitly adds NeedsDataUpdate to force recompaction */
    UnsignedInt dataIdsPostRemoval[]{9};
    layer.update(data.states|LayerState::NeedsDataUpdate, dataIdsPostRemoval, {}, {}, nodeOffsets, nodeSizes, nodeOpacities, nodesEnabled, {}, {}, {}, {});

    /* There should be just 7 runs, assigned to the remaining 7 data */
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().data).slice(&Implementation::LineLayerData::run), Containers::arrayView({
        0u, 1u, 2u, 3u /* free data */, 3u, 5u /* free data */, 4u,
        5u /* free data */, 5u, 6u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::indexCount), Containers::arrayView({
        0u, 2u, 0u, 0u, 2u, 0u, 6u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::indexOffset), Containers::arrayView({
        0u, 0u, 2u, 2u, 2u, 4u, 4u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::pointCount), Containers::arrayView({
        0u, 1u, 0u, 0u, 1u, 2u, 4u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::pointOffset), Containers::arrayView({
        0u, 0u, 1u, 1u, 1u, 2u, 4u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().runs).slice(&Implementation::LineLayerRun::data), Containers::arrayView({
        0u, 1u, 2u, 4u, 6u, 8u, 9u
    }), TestSuite::Compare::Container);

    /* Indices for remaining 1 visible line */
    CORRADE_COMPARE_AS(layer.stateData().indices, Containers::arrayView<UnsignedInt>({
        /* Line 9, quads 2 to 4 */
        2*4 + 2, 2*4 + 0, 2*4 + 1, 2*4 + 1, 2*4 + 3, 2*4 + 2,
        3*4 + 2, 3*4 + 0, 3*4 + 1, 3*4 + 1, 3*4 + 3, 3*4 + 2,
        4*4 + 2, 4*4 + 0, 4*4 + 1, 4*4 + 1, 4*4 + 3, 4*4 + 2,
    }), TestSuite::Compare::Container);

    /* Vertices for remaining 1 visible line 9, quads 2 to 4 */
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().vertices.slice(2*4, 5*4)).slice(&Implementation::LineLayerVertex::annotationStyleUniform), Containers::arrayView<UnsignedInt>({
        (1 << 3)|Begin|Up,
        (1 << 3)|Begin,
        (1 << 3)|Up,
        (1 << 3),

        (1 << 3)|Begin|Up,
        (1 << 3)|Begin,
        (1 << 3)|Up,
        (1 << 3),

        (1 << 3)|Begin|Up,
        (1 << 3)|Begin,
        (1 << 3)|Up,
        (1 << 3),
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().vertices.slice(2*4, 5*4)).slice(&Implementation::LineLayerVertex::position), Containers::arrayView<Vector2>({
        {3.0f + 0.0f, 4.0f + 0.0f},
        {3.0f + 0.0f, 4.0f + 0.0f},
        {3.0f + 1.0f, 4.0f + 0.0f},
        {3.0f + 1.0f, 4.0f + 0.0f},

        {3.0f + 0.0f, 4.0f + 0.0f},
        {3.0f + 0.0f, 4.0f + 0.0f},
        {3.0f + 0.0f, 4.0f + 1.0f},
        {3.0f + 0.0f, 4.0f + 1.0f},

        {3.0f + 0.0f, 4.0f + 0.0f},
        {3.0f + 0.0f, 4.0f + 0.0f},
        {3.0f + 1.0f, 4.0f + 1.0f},
        {3.0f + 1.0f, 4.0f + 1.0f},
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().vertices.slice(2*4, 5*4)).slice(&Implementation::LineLayerVertex::previousPosition), Containers::arrayView<Vector2>({
        {3.0f, 4.0f},
        {3.0f, 4.0f},
        {3.0f, 4.0f},
        {3.0f, 4.0f},

        {3.0f, 4.0f},
        {3.0f, 4.0f},
        {3.0f, 4.0f},
        {3.0f, 4.0f},

        {3.0f, 4.0f},
        {3.0f, 4.0f},
        {3.0f, 4.0f},
        {3.0f, 4.0f},
    }), TestSuite::Compare::Container);
    for(std::size_t i: {2, 3, 4}) {
        CORRADE_ITERATION(i);
        for(std::size_t j = 0; j != 4; ++j) {
            CORRADE_ITERATION(j);
            CORRADE_COMPARE(layer.stateData().vertices[i*4 + j].color, 0x663399ff_rgbaf*0.9f);
        }
    }
}

void LineLayerTest::updateAlignment() {
    auto&& data = UpdateAlignmentPaddingData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct LayerShared: LineLayer::Shared {
        explicit LayerShared(const Configuration& configuration): LineLayer::Shared{configuration} {}

        void doSetStyle(const LineLayerCommonStyleUniform&, Containers::ArrayView<const LineLayerStyleUniform>) override {}
    } shared{LineLayer::Shared::Configuration{1}};

    shared.setStyle(LineLayerCommonStyleUniform{},
        {LineLayerStyleUniform{}},
        /* Alignment supplied in the style; alignment in data tested in
           updatePadding() below, combination of both with a subset of the
           alignment values in updateCleanDataOrder() above */
        {data.alignment,},
        {});

    struct Layer: LineLayer {
        explicit Layer(LayerHandle handle, Shared& shared): LineLayer{handle, shared} {}

        const State& stateData() const {
            return static_cast<const State&>(*_state);
        }
    } layer{layerHandle(0, 1), shared};

    /* Required to be called before update() (because AbstractUserInterface
       guarantees the same on a higher level), not needed for anything here */
    layer.setSize({1, 1}, {1, 1});

    /* Generation doesn't matter, just has to be non-zero */
    NodeHandle node3 = nodeHandle(3, 0xeee);

    layer.create(0,
        {0, 1},
        {{-3.0f, 4.0f}, {5.0f, -6.0f}},
        {},
        node3);

    Vector2 nodeOffsets[4];
    Vector2 nodeSizes[4];
    Float nodeOpacities[4]{};
    UnsignedByte nodesEnabledData[1]{};
    Containers::BitArrayView nodesEnabled{nodesEnabledData, 0, 4};
    nodeOffsets[3] = {50.5f, 20.5f};
    nodeSizes[3] = {200.8f, 100.4f};
    UnsignedInt dataIds[]{0};
    layer.update(LayerState::NeedsDataUpdate, dataIds, {}, {}, nodeOffsets, nodeSizes, nodeOpacities, nodesEnabled, {}, {}, {}, {});

    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().vertices).slice(&Implementation::LineLayerVertex::position), Containers::arrayView<Vector2>({
        Vector2{-3.0f, 4.0f} + data.offset,
        Vector2{-3.0f, 4.0f} + data.offset,
        Vector2{5.0f, -6.0f} + data.offset,
        Vector2{5.0f, -6.0f} + data.offset
    }), TestSuite::Compare::Container);
}

void LineLayerTest::updatePadding() {
    auto&& data = UpdateAlignmentPaddingData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Same as updateAlignment(), except that the node offset & size is
       different and only matches the original if padding is applied
       correctly from both the data and the style. Additionally, in comparison
       to updateAlignment(), the style-supplied alignment is a bogus value and
       alignment from the data is used instead. */

    struct LayerShared: LineLayer::Shared {
        explicit LayerShared(const Configuration& configuration): LineLayer::Shared{configuration} {}

        void doSetStyle(const LineLayerCommonStyleUniform&, Containers::ArrayView<const LineLayerStyleUniform>) override {}
    } shared{LineLayer::Shared::Configuration{1}};

    shared.setStyle(LineLayerCommonStyleUniform{},
        {LineLayerStyleUniform{}},
        /* Alignment set to an arbitrary value, the one from TextProperties
           should get used instead */
        {LineAlignment::BottomLeft},
        {{10.0f, 5.0f, 20.0f, 10.0f}});

    struct Layer: LineLayer {
        explicit Layer(LayerHandle handle, Shared& shared): LineLayer{handle, shared} {}

        const State& stateData() const {
            return static_cast<const State&>(*_state);
        }
    } layer{layerHandle(0, 1), shared};

    /* Required to be called before update() (because AbstractUserInterface
       guarantees the same on a higher level), not needed for anything here */
    layer.setSize({1, 1}, {1, 1});

    /* Generation doesn't matter, just has to be non-zero */
    NodeHandle node3 = nodeHandle(3, 0xeee);

    DataHandle node3Data = layer.create(0,
        {0, 1},
        {{-3.0f, 4.0f}, {5.0f, -6.0f}},
        {},
        node3);
    layer.setAlignment(node3Data, data.alignment);
    layer.setPadding(node3Data, {20.0f, 5.0f, 50.0f, 30.0f});

    Vector2 nodeOffsets[4];
    Vector2 nodeSizes[4];
    Float nodeOpacities[4]{};
    UnsignedByte nodesEnabledData[1]{};
    Containers::BitArrayView nodesEnabled{nodesEnabledData, 0, 4};
    nodeOffsets[3] = {20.5f, 10.5f};
    nodeSizes[3] = {300.8f, 150.4f};
    UnsignedInt dataIds[]{0};
    layer.update(LayerState::NeedsDataUpdate, dataIds, {}, {}, nodeOffsets, nodeSizes, nodeOpacities, nodesEnabled, {}, {}, {}, {});

    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().vertices).slice(&Implementation::LineLayerVertex::position), Containers::arrayView<Vector2>({
        Vector2{-3.0f, 4.0f} + data.offset,
        Vector2{-3.0f, 4.0f} + data.offset,
        Vector2{5.0f, -6.0f} + data.offset,
        Vector2{5.0f, -6.0f} + data.offset
    }), TestSuite::Compare::Container);
}

void LineLayerTest::updateNoStyleSet() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct LayerShared: LineLayer::Shared {
        explicit LayerShared(const Configuration& configuration): LineLayer::Shared{configuration} {}

        void doSetStyle(const LineLayerCommonStyleUniform&, Containers::ArrayView<const LineLayerStyleUniform>) override {}
    } shared{LineLayer::Shared::Configuration{3}};

    struct Layer: LineLayer {
        explicit Layer(LayerHandle handle, Shared& shared): LineLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    /* Required to be called before update() (because AbstractUserInterface
       guarantees the same on a higher level), not needed for anything here */
    layer.setSize({1, 1}, {1, 1});

    Containers::String out;
    Error redirectError{&out};
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(out, "Ui::LineLayer::update(): no style data was set\n");
}

void LineLayerTest::sharedNeedsUpdateStatePropagatedToLayers() {
    struct LayerShared: LineLayer::Shared {
        explicit LayerShared(const Configuration& configuration): LineLayer::Shared{configuration} {}

        void doSetStyle(const LineLayerCommonStyleUniform&, Containers::ArrayView<const LineLayerStyleUniform>) override {}
    } shared{LineLayer::Shared::Configuration{1}};

    struct Layer: LineLayer {
        explicit Layer(LayerHandle handle, Shared& shared): LineLayer{handle, shared} {}
    };

    /* Initially no state is set */
    Layer layer1{layerHandle(0, 1), shared};
    Layer layer2{layerHandle(0, 1), shared};
    Layer layer3{layerHandle(0, 1), shared};
    CORRADE_COMPARE(layer1.state(), LayerStates{});
    CORRADE_COMPARE(layer2.state(), LayerStates{});
    CORRADE_COMPARE(layer3.state(), LayerStates{});

    /* Required to be called before update() (because AbstractUserInterface
       guarantees the same on a higher level), not needed for anything here */
    layer1.setSize({1, 1}, {1, 1});
    layer2.setSize({1, 1}, {1, 1});
    layer3.setSize({1, 1}, {1, 1});

    /* Explicitly set a non-trivial state on some of the layers */
    layer1.setNeedsUpdate(LayerState::NeedsCommonDataUpdate);
    layer3.setNeedsUpdate(LayerState::NeedsSharedDataUpdate);

    /* Calling setStyle() sets LayerState::Needs*DataUpdate on all layers */
    shared.setStyle(LineLayerCommonStyleUniform{},
        {LineLayerStyleUniform{}},
        {{}},
        {});
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

    /* Calling setStyle() again sets LayerState::Needs*DataUpdate again, even
       if the data may be the same, as checking differences would be
       unnecessarily expensive compared to just doing the update always */
    shared.setStyle(LineLayerCommonStyleUniform{},
        {LineLayerStyleUniform{}},
        {{}},
        {});
    CORRADE_COMPARE(layer1.state(), LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate);
    CORRADE_COMPARE(layer2.state(), LayerState::NeedsDataUpdate);
    CORRADE_COMPARE(layer3.state(), LayerState::NeedsDataUpdate|LayerState::NeedsSharedDataUpdate);

    /* Creating a new layer with the shared state that had setStyle() called a
       few times doesn't mark it as needing an update because there's no data
       that would need it yet and the layer should do all other
       shared-state-dependent setup during construction already. For dynamic
       styles it'll perform the upload on the first update() regardless on the
       LayerState. */
    Layer layer4{layerHandle(0, 1), shared};
    CORRADE_COMPARE(layer4.state(), LayerStates{});

    /* But calling setStyle() next time will */
    shared.setStyle(LineLayerCommonStyleUniform{},
        {LineLayerStyleUniform{}},
        {{}},
        {});
    CORRADE_COMPARE(layer1.state(), LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate);
    CORRADE_COMPARE(layer2.state(), LayerState::NeedsDataUpdate);
    CORRADE_COMPARE(layer3.state(), LayerState::NeedsDataUpdate|LayerState::NeedsSharedDataUpdate);
    CORRADE_COMPARE(layer4.state(), LayerState::NeedsDataUpdate);

    /* Updating again resets just one */
    layer3.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer1.state(), LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate);
    CORRADE_COMPARE(layer2.state(), LayerState::NeedsDataUpdate);
    CORRADE_COMPARE(layer3.state(), LayerState::NeedsSharedDataUpdate);
    CORRADE_COMPARE(layer4.state(), LayerState::NeedsDataUpdate);

    /* Calling the AbstractVisualLayer setStyleTransition() should still cause
       LayerState to be updated as well, i.e. the class should correctly
       propagate to the parent doState() as well */
    shared.setStyleTransition(
        nullptr,
        nullptr,
        nullptr,
        [](UnsignedInt a) { return a + 1; });
    CORRADE_COMPARE(layer1.state(), LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate);
    CORRADE_COMPARE(layer2.state(), LayerState::NeedsDataUpdate);
    /* This one has NeedsDataUpdate set again, not the extraState though as
       that comes only from setStyle() depending on dynamic styles being
       present */
    CORRADE_COMPARE(layer3.state(), LayerState::NeedsDataUpdate|LayerState::NeedsSharedDataUpdate);
    CORRADE_COMPARE(layer4.state(), LayerState::NeedsDataUpdate);
}

Containers::StringView debugIntegrationStyleName(UnsignedInt style) {
    return style == 3 ? "StyleName" : "Wrong";
}

void LineLayerTest::debugIntegration() {
    auto&& data = DebugIntegrationData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    AbstractUserInterface ui{{100, 100}};
    NodeHandle root = ui.createNode({}, {100, 100});
    NodeHandle node = ui.createNode(root, {}, {100, 100});

    struct LayerShared: LineLayer::Shared {
        explicit LayerShared(const Configuration& configuration): LineLayer::Shared{configuration} {}

        void doSetStyle(const LineLayerCommonStyleUniform&, Containers::ArrayView<const LineLayerStyleUniform>) override {}
    } shared{LineLayer::Shared::Configuration{4}};
    shared.setStyle(LineLayerCommonStyleUniform{},
        {LineLayerStyleUniform{},
         LineLayerStyleUniform{},
         LineLayerStyleUniform{},
         LineLayerStyleUniform{}},
        {LineAlignment::MiddleCenter,
         LineAlignment::MiddleCenter,
         LineAlignment::MiddleCenter,
         LineAlignment::MiddleCenter},
        {});

    struct Layer: LineLayer {
        explicit Layer(LayerHandle handle, Shared& shared): LineLayer{handle, shared} {}
    };

    /* Create and remove a bunch of layers first to have the handle with a
       non-trivial value */
    ui.removeLayer(ui.createLayer());
    ui.removeLayer(ui.createLayer());
    LineLayer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), shared));
    /* And also some more data to not list a trivial data handle */
    layer.createLoop(0, {{}}, {});
    layer.createLoop(0, {{}}, {});
    layer.createLoop(0, {{}}, {});
    layer.createLoop(0, {{}}, {});
    layer.createLoop(0, {{}}, {});
    layer.createLoop(0, {{}}, {});
    layer.remove(layer.createLoop(0, {{}}, {}));
    DataHandle layerData = layer.createLoop(3, {{}}, {}, node);
    if(data.color)
        layer.setColor(layerData, *data.color);
    if(data.alignment)
        layer.setAlignment(layerData, data.alignment);
    layer.setPadding(layerData, data.padding);

    DebugLayer& debugLayer = ui.setLayerInstance(Containers::pointer<DebugLayer>(ui.createLayer(), DebugLayerSource::NodeDataDetails, DebugLayerFlag::NodeInspect));

    Containers::String out;
    debugLayer.setNodeInspectCallback([&out](Containers::StringView message) {
        out = message;
    });
    if(data.styleNames)
        debugLayer.setLayerName(layer, "", debugIntegrationStyleName);
    else
        debugLayer.setLayerName(layer, "");

    /* Make the debug layer aware of everything */
    ui.update();

    CORRADE_VERIFY(debugLayer.inspectNode(node));
    CORRADE_COMPARE_AS(out, data.expected, TestSuite::Compare::String);
}

void LineLayerTest::debugIntegrationNoCallback() {
    AbstractUserInterface ui{{100, 100}};
    NodeHandle root = ui.createNode({}, {100, 100});
    NodeHandle node = ui.createNode(root, {}, {100, 100});

    struct LayerShared: LineLayer::Shared {
        explicit LayerShared(const Configuration& configuration): LineLayer::Shared{configuration} {}

        void doSetStyle(const LineLayerCommonStyleUniform&, Containers::ArrayView<const LineLayerStyleUniform>) override {}
    } shared{LineLayer::Shared::Configuration{4}};
    shared.setStyle(LineLayerCommonStyleUniform{},
        {LineLayerStyleUniform{},
         LineLayerStyleUniform{},
         LineLayerStyleUniform{},
         LineLayerStyleUniform{}},
        {LineAlignment::MiddleCenter,
         LineAlignment::MiddleCenter,
         LineAlignment::MiddleCenter,
         LineAlignment::MiddleCenter},
        {});

    struct Layer: LineLayer {
        explicit Layer(LayerHandle handle, Shared& shared): LineLayer{handle, shared} {}
    };

    /* Just to match the layer handle in debugIntegration() above */
    ui.removeLayer(ui.createLayer());
    ui.removeLayer(ui.createLayer());
    LineLayer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), shared));
    /* ... and the data handle also */
    layer.createLoop(0, {{}}, {});
    layer.createLoop(0, {{}}, {});
    layer.createLoop(0, {{}}, {});
    layer.createLoop(0, {{}}, {});
    layer.createLoop(0, {{}}, {});
    layer.createLoop(0, {{}}, {});
    layer.remove(layer.createLoop(0, {{}}, {}));
    DataHandle layerData = layer.createLoop(3, {{}}, {}, node);
    layer.setColor(layerData, 0x3bd26799_rgbaf);
    layer.setAlignment(layerData, LineAlignment::MiddleRight);
    layer.setPadding(layerData, {0.5f, 2.0f, 1.5f, 1.0f});

    DebugLayer& debugLayer = ui.setLayerInstance(Containers::pointer<DebugLayer>(ui.createLayer(), DebugLayerSource::NodeDataDetails, DebugLayerFlag::NodeInspect));

    debugLayer.setLayerName(layer, "", debugIntegrationStyleName);

    /* Make the debug layer aware of everything */
    ui.update();

    /* Inspect the node for visual color verification */
    {
        Debug{} << "======================== visual color verification start =======================";

        debugLayer.addFlags(DebugLayerFlag::ColorAlways);

        CORRADE_VERIFY(debugLayer.inspectNode(node));

        debugLayer.clearFlags(DebugLayerFlag::ColorAlways);

        Debug{} << "======================== visual color verification end =========================";
    }

    /* Do the same, but this time with output redirection to verify the
       contents. The internals automatically disable coloring if they detect
       the output isn't a TTY. */
    {
        Containers::String out;
        Debug redirectOutput{&out};
        CORRADE_VERIFY(debugLayer.inspectNode(node));
        /* The output always has a newline at the end which cannot be disabled
           so strip it to have the comparison match the debugIntegration()
           case */
        CORRADE_COMPARE_AS(out,
            "\n",
            TestSuite::Compare::StringHasSuffix);
        CORRADE_COMPARE_AS(out.exceptSuffix("\n"),
            Containers::arrayView(DebugIntegrationData).back().expected,
            TestSuite::Compare::String);
    }
}

void LineLayerTest::debugIntegrationLambdaStyleName() {
    /* Like AbstractVisualLayerTest::debugIntegrationLambdaStyleName(), just
       verifying that the construction with a lambda works even with the
       BaseLayer subclass of DebugIntegration */

    AbstractUserInterface ui{{100, 100}};
    NodeHandle node = ui.createNode({}, {100, 100});

    struct LayerShared: LineLayer::Shared {
        explicit LayerShared(const Configuration& configuration): LineLayer::Shared{configuration} {}

        void doSetStyle(const LineLayerCommonStyleUniform&, Containers::ArrayView<const LineLayerStyleUniform>) override {}
    } shared{LineLayer::Shared::Configuration{4}};
    shared.setStyle(LineLayerCommonStyleUniform{},
        {LineLayerStyleUniform{},
         LineLayerStyleUniform{},
         LineLayerStyleUniform{},
         LineLayerStyleUniform{}},
        {LineAlignment::MiddleCenter,
         LineAlignment::MiddleCenter,
         LineAlignment::MiddleCenter,
         LineAlignment::MiddleCenter},
        {});

    struct Layer: LineLayer {
        explicit Layer(LayerHandle handle, Shared& shared): LineLayer{handle, shared} {}
    };

    LineLayer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), shared));
    layer.createLoop(3, {{}}, {}, node);

    DebugLayer& debugLayer = ui.setLayerInstance(Containers::pointer<DebugLayer>(ui.createLayer(), DebugLayerSource::NodeDataDetails, DebugLayerFlag::NodeInspect));
    debugLayer.setLayerName(layer, "", [](UnsignedInt style) -> Containers::StringView {
        return style == 3 ? "LambdaStyle" : "Wrong";
    });

    /* Make the debug layer aware of everything */
    ui.update();

    Containers::String out;
    {
        Debug redirectOutput{&out};
        CORRADE_VERIFY(debugLayer.inspectNode(node));
    }
    CORRADE_COMPARE_AS(out,
        "Top-level node {0x0, 0x1}\n"
        "  Data {0x0, 0x1} from layer {0x0, 0x1} with style LambdaStyle (3)\n",
        TestSuite::Compare::String);
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::LineLayerTest)
