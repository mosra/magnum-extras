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

#include <Corrade/Containers/BitArrayView.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Containers/String.h>
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/Container.h>
#include <Corrade/TestSuite/Compare/String.h>
#include <Magnum/Math/Functions.h>
#include <Magnum/Math/Vector4.h>

#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/LayoutLayer.h"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct LayoutLayerTest: TestSuite::Tester {
    explicit LayoutLayerTest();

    void construct();
    void constructInvalid();
    void constructCopy();
    void constructMove();

    void setStyleData();
    void setStyleDataEmpty();
    void styleDataInvalid();

    template<class T> void createRemove();
    void createRemoveHandleRecycle();

    template<class T> void setStyle();

    void invalidHandle();
    void styleOutOfRange();

    void layoutEmpty();
    void layout();
    void layoutNoStyleSet();
};

enum class Enum: UnsignedShort {};

Debug& operator<<(Debug& debug, Enum value) {
    return debug << UnsignedInt(value);
}

const struct {
    const char* name;
    bool minSize, maxSize, aspectRatio, padding, margin;
} StyleData[]{
    {"nothing set",
        false, false, false, false, false},
    {"just min sizes",
        true, false, false, false, false},
    {"just max sizes",
        false, true, false, false, false},
    {"just aspect ratios",
        false, false, true, false, false},
    {"just paddings",
        false, false, false, true, false},
    {"just margins",
        false, false, false, false, true},
    {"everything",
        true, true, true, true, true},
};

const struct {
    const char* name;
    NodeHandle node;
    LayerStates state;
    bool layerDataHandleOverloads;
} CreateRemoveData[]{
    {"create",
        NodeHandle::Null, LayerState::NeedsDataUpdate, false},
    {"create and attach",
        nodeHandle(9872, 0xbeb), LayerState::NeedsNodeOffsetSizeUpdate|LayerState::NeedsAttachmentUpdate|LayerState::NeedsLayoutUpdate|LayerState::NeedsDataUpdate, false},
    {"LayerDataHandle overloads",
        NodeHandle::Null, LayerState::NeedsDataUpdate, true},
};

const struct {
    const char* name;
    NodeHandle node;
    LayerStates expectedInitialState, expectedState;
} SetStyleData[]{
    {"not attached",
        NodeHandle::Null, LayerState::NeedsDataUpdate, {}},
    {"attached",
        nodeHandle(9872, 0xbeb), LayerState::NeedsDataUpdate|LayerState::NeedsNodeOffsetSizeUpdate|LayerState::NeedsAttachmentUpdate|LayerState::NeedsLayoutUpdate, LayerState::NeedsLayoutUpdate},
};

const struct {
    const char* name;
    BitVector2 minSize, maxSize;
    bool aspectRatio;
    BitVector4 padding, margin;
} LayoutData[]{
    {"nothing",
        0, 0, false, 0, 0},
    {"just min X sizes",
        1, 0, false, 0, 0},
    {"just min Y sizes",
        2, 0, false, 0, 0},
    {"just min sizes",
        3, 0, false, 0, 0},
    {"just max X sizes",
        0, 1, false, 0, 0},
    {"just max Y sizes",
        0, 2, false, 0, 0},
    {"just max sizes",
        0, 3, false, 0, 0},
    {"just aspect ratios",
        0, 0, true, 0, 0},
    {"just left paddings",
        0, 0, false, 1, 0},
    {"just top paddings",
        0, 0, false, 2, 0},
    {"just right paddings",
        0, 0, false, 4, 0},
    {"just bottom paddings",
        0, 0, false, 8, 0},
    {"just paddings",
        0, 0, false, 15, 0},
    {"just left margins",
        0, 0, false, 0, 1},
    {"just top margins",
        0, 0, false, 0, 2},
    {"just right margins",
        0, 0, false, 0, 4},
    {"just bottom margins",
        0, 0, false, 0, 8},
    {"just margins",
        0, 0, false, 0, 15},
    {"everything",
        3, 3, true, 15, 15},
};

LayoutLayerTest::LayoutLayerTest() {
    addTests({&LayoutLayerTest::construct,
              &LayoutLayerTest::constructInvalid,
              &LayoutLayerTest::constructCopy,
              &LayoutLayerTest::constructMove});

    addInstancedTests({&LayoutLayerTest::setStyleData},
        Containers::arraySize(StyleData));

    addTests({&LayoutLayerTest::setStyleDataEmpty,
              &LayoutLayerTest::styleDataInvalid});

    addInstancedTests<LayoutLayerTest>({
        &LayoutLayerTest::createRemove<UnsignedInt>,
        &LayoutLayerTest::createRemove<Enum>},
        Containers::arraySize(CreateRemoveData));

    addTests({&LayoutLayerTest::createRemoveHandleRecycle});

    addInstancedTests<LayoutLayerTest>({
        &LayoutLayerTest::setStyle<UnsignedInt>,
        &LayoutLayerTest::setStyle<Enum>},
        Containers::arraySize(SetStyleData));

    addTests({&LayoutLayerTest::invalidHandle,
              &LayoutLayerTest::styleOutOfRange,

              &LayoutLayerTest::layoutEmpty});

    addInstancedTests({&LayoutLayerTest::layout},
        Containers::arraySize(LayoutData));

    addTests({&LayoutLayerTest::layoutNoStyleSet});
}

void LayoutLayerTest::construct() {
    LayoutLayer layer{layerHandle(137, 0xde), 23};
    CORRADE_COMPARE(layer.handle(), layerHandle(137, 0xde));
    CORRADE_COMPARE(layer.styleCount(), 23);
}

void LayoutLayerTest::constructCopy() {
    CORRADE_VERIFY(!std::is_copy_constructible<LayoutLayer>{});
    CORRADE_VERIFY(!std::is_copy_assignable<LayoutLayer>{});
}

void LayoutLayerTest::constructMove() {
    LayoutLayer a{layerHandle(137, 0xde), 23};

    LayoutLayer b = Utility::move(a);
    CORRADE_COMPARE(b.handle(), layerHandle(137, 0xde));
    CORRADE_COMPARE(b.styleCount(), 23);

    LayoutLayer c{layerHandle(34, 56), 78};
    c = Utility::move(b);
    CORRADE_COMPARE(c.handle(), layerHandle(137, 0xde));
    CORRADE_COMPARE(c.styleCount(), 23);

    CORRADE_VERIFY(std::is_nothrow_move_constructible<LayoutLayer>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<LayoutLayer>::value);
}

void LayoutLayerTest::constructInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    Containers::String out;
    Error redirectError{&out};
    LayoutLayer{layerHandle(0, 1), 0};
    CORRADE_COMPARE(out, "Ui::LayoutLayer: expected non-zero style count\n");
}

void LayoutLayerTest::setStyleData() {
    auto&& data = StyleData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    LayoutLayer layer{layerHandle(0, 1), 3};
    CORRADE_COMPARE(layer.state(), LayerStates{});

    Vector2 minSizes[]{
        {1.0f, 2.0f},
        {2.0f, 3.0f},
        {3.0f, 4.0f},
    };
    Vector2 maxSizes[]{
        {4.0f, 5.0f},
        {5.0f, 6.0f},
        {6.0f, 7.0f},
    };
    Float aspectRatios[]{
        8.0f, 9.0f, 10.0f
    };
    Vector4 paddings[]{
        {0.1f, 0.2f, 0.3f, 0.4f},
        {0.2f, 0.3f, 0.4f, 0.5f},
        {0.3f, 0.4f, 0.5f, 0.6f},
    };
    Vector4 margins[]{
        {0.4f, 0.5f, 0.6f, 0.7f},
        {0.5f, 0.6f, 0.7f, 0.8f},
        {0.6f, 0.7f, 0.8f, 0.9f},
    };

    /* Setting a style makes the layer dirty */
    layer.setStyle(
        data.minSize ? Containers::arrayView(minSizes) : nullptr,
        data.maxSize ? Containers::arrayView(maxSizes) : nullptr,
        data.aspectRatio ? Containers::arrayView(aspectRatios) : nullptr,
        data.padding ? Containers::arrayView(paddings) : nullptr,
        data.margin ? Containers::arrayView(margins) : nullptr);
    CORRADE_COMPARE_AS(layer.styleMinSizes(), data.minSize ?
        Containers::arrayView(minSizes) :
        Containers::arrayView<Vector2>({{}, {}, {}}),
        TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(layer.styleMaxSizes(), data.maxSize ?
        Containers::arrayView(maxSizes) :
        Containers::arrayView<Vector2>({Vector2{Constants::inf()}, Vector2{Constants::inf()}, Vector2{Constants::inf()}}),
        TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(layer.styleAspectRatios(), data.aspectRatio ?
        Containers::arrayView(aspectRatios) :
        Containers::arrayView({0.0f, 0.0f, 0.0f}),
        TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(layer.stylePaddings(), data.padding ?
        Containers::arrayView(paddings) :
        Containers::arrayView<Vector4>({{}, {}, {}}),
        TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(layer.styleMargins(), data.margin ?
        Containers::arrayView(margins) :
        Containers::arrayView<Vector4>({{}, {}, {}}),
        TestSuite::Compare::Container);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsLayoutUpdate);

    /* Clear the state flags */
    layer.update(LayerState::NeedsLayoutUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Verify the initializer-list overload, filling in everything. It
       delegates to the ArrayView overloads so it shouldn't be needed to test
       the empy cases as well. */
    layer.setStyle(
        {{999.1f, 999.2f}, {999.2f, 999.3f}, {999.3f, 999.4f}},
        {{999.4f, 999.5f}, {999.5f, 999.6f}, {999.6f, 999.7f}},
        {999.8f, 999.9f, 999.0f},
        {{-999.1f, -999.2f, -999.3f, -999.4f},
         {-999.2f, -999.3f, -999.4f, -999.5f},
         {-999.3f, -999.4f, -999.5f, -999.6f}},
        {{-999.4f, -999.5f, -999.6f, -999.7f},
         {-999.5f, -999.6f, -999.7f, -999.8f},
         {-999.6f, -999.7f, -999.8f, -999.9f}});
    CORRADE_COMPARE_AS(layer.styleMinSizes(), Containers::arrayView<Vector2>({
        {999.1f, 999.2f}, {999.2f, 999.3f}, {999.3f, 999.4f}
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(layer.styleMaxSizes(), Containers::arrayView<Vector2>({
        {999.4f, 999.5f}, {999.5f, 999.6f}, {999.6f, 999.7f}
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(layer.styleAspectRatios(), Containers::arrayView<Float>({
        999.8f, 999.9f, 999.0f
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(layer.stylePaddings(), Containers::arrayView<Vector4>({
        {-999.1f, -999.2f, -999.3f, -999.4f},
        {-999.2f, -999.3f, -999.4f, -999.5f},
        {-999.3f, -999.4f, -999.5f, -999.6f}
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(layer.styleMargins(), Containers::arrayView<Vector4>({
        {-999.4f, -999.5f, -999.6f, -999.7f},
        {-999.5f, -999.6f, -999.7f, -999.8f},
        {-999.6f, -999.7f, -999.8f, -999.9f}
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsLayoutUpdate);

    /* Clear the state flags */
    layer.update(LayerState::NeedsLayoutUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Re-setting the initial values should result in the unset properties
       being set back to defaults again, not keeping the 999s */
    layer.setStyle(
        data.minSize ? Containers::arrayView(minSizes) : nullptr,
        data.maxSize ? Containers::arrayView(maxSizes) : nullptr,
        data.aspectRatio ? Containers::arrayView(aspectRatios) : nullptr,
        data.padding ? Containers::arrayView(paddings) : nullptr,
        data.margin ? Containers::arrayView(margins) : nullptr);
    CORRADE_COMPARE_AS(layer.styleMinSizes(), data.minSize ?
        Containers::arrayView(minSizes) :
        Containers::arrayView<Vector2>({{}, {}, {}}),
        TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(layer.styleMaxSizes(), data.maxSize ?
        Containers::arrayView(maxSizes) :
        Containers::arrayView<Vector2>({Vector2{Constants::inf()}, Vector2{Constants::inf()}, Vector2{Constants::inf()}}),
        TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(layer.styleAspectRatios(), data.aspectRatio ?
        Containers::arrayView(aspectRatios) :
        Containers::arrayView({0.0f, 0.0f, 0.0f}),
        TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(layer.stylePaddings(), data.padding ?
        Containers::arrayView(paddings) :
        Containers::arrayView<Vector4>({{}, {}, {}}),
        TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(layer.styleMargins(), data.margin ?
        Containers::arrayView(margins) :
        Containers::arrayView<Vector4>({{}, {}, {}}),
        TestSuite::Compare::Container);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsLayoutUpdate);
}

void LayoutLayerTest::setStyleDataEmpty() {
    LayoutLayer layer{layerHandle(0, 1), 3};

    /* Just to verify that passing all {}s isn't ambiguous or behaving weird.
       It should cause the style data to be all defaults. */
    layer.setStyle({}, {}, {}, {}, {});
    CORRADE_COMPARE_AS(layer.styleMinSizes(), Containers::arrayView<Vector2>({
        {}, {}, {}
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(layer.styleMaxSizes(), Containers::arrayView({
        Vector2{Constants::inf()},
        Vector2{Constants::inf()},
        Vector2{Constants::inf()}
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(layer.styleAspectRatios(), Containers::arrayView({
        0.0f, 0.0f, 0.0f
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(layer.stylePaddings(), Containers::arrayView<Vector4>({
        {}, {}, {}
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(layer.styleMargins(), Containers::arrayView<Vector4>({
        {}, {}, {}
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsLayoutUpdate);
}

void LayoutLayerTest::styleDataInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    LayoutLayer layer{layerHandle(0, 1), 3};

    Vector2 sizes[3];
    Vector2 sizesInvalid[4];
    Float aspectRatios[3];
    Float aspectRatiosInvalid[4];
    Vector4 paddingsMargins[3];
    Vector4 paddingsMarginsInvalid[4];

    Containers::String out;
    Error redirectError{&out};
    layer.setStyle(sizes, sizes, aspectRatios, paddingsMargins, paddingsMarginsInvalid);
    layer.setStyle(sizes, sizes, aspectRatios, paddingsMarginsInvalid, paddingsMargins);
    layer.setStyle(sizes, sizes, aspectRatiosInvalid, paddingsMargins, paddingsMargins);
    layer.setStyle(sizes, sizesInvalid, aspectRatios, paddingsMargins, paddingsMargins);
    layer.setStyle(sizesInvalid, sizes, aspectRatios, paddingsMargins, paddingsMargins);
    CORRADE_COMPARE_AS(out,
        "Ui::LayoutLayer::setStyle(): expected min size, max size, aspect ratio, padding and margin views to be either empty or have a size of 3 but got 3, 3, 3, 3 and 4\n"
        "Ui::LayoutLayer::setStyle(): expected min size, max size, aspect ratio, padding and margin views to be either empty or have a size of 3 but got 3, 3, 3, 4 and 3\n"
        "Ui::LayoutLayer::setStyle(): expected min size, max size, aspect ratio, padding and margin views to be either empty or have a size of 3 but got 3, 3, 4, 3 and 3\n"
        "Ui::LayoutLayer::setStyle(): expected min size, max size, aspect ratio, padding and margin views to be either empty or have a size of 3 but got 3, 4, 3, 3 and 3\n"
        "Ui::LayoutLayer::setStyle(): expected min size, max size, aspect ratio, padding and margin views to be either empty or have a size of 3 but got 4, 3, 3, 3 and 3\n",
        TestSuite::Compare::String);
}

template<class T> void LayoutLayerTest::createRemove() {
    auto&& data = CreateRemoveData[testCaseInstanceId()];
    setTestCaseDescription(data.name);
    setTestCaseTemplateName(std::is_same<T, Enum>::value ? "Enum" : "UnsignedInt");

    LayoutLayer layer{layerHandle(0, 1), 38};

    /* Data creation should work without a style being set */

    DataHandle first = layer.create(T(17), data.node);
    CORRADE_COMPARE(layer.node(first), data.node);
    CORRADE_COMPARE(layer.style(first), 17);
    CORRADE_COMPARE(layer.state(), data.state);

    /* Default null node */
    DataHandle second = layer.create(T(23));
    CORRADE_COMPARE(layer.node(second), NodeHandle::Null);
    CORRADE_COMPARE(layer.style(second), 23);

    /* Testing also the getter overloads and templates */
    DataHandle third = layer.create(T(37), data.node);
    CORRADE_COMPARE(layer.node(third), data.node);
    if(data.layerDataHandleOverloads) {
        CORRADE_COMPARE(layer.style(dataHandleData(third)), 37);
        /* Can't use T, as the function restricts to enum types which would
           fail for T == UnsignedInt */
        CORRADE_COMPARE(layer.template style<Enum>(dataHandleData(third)), Enum(37));
    } else {
        CORRADE_COMPARE(layer.style(third), 37);
        /* Can't use T, as the function restricts to enum types which would
           fail for T == UnsignedInt */
        CORRADE_COMPARE(layer.template style<Enum>(third), Enum(37));
    }
    CORRADE_COMPARE(layer.state(), data.state);

    /* Removing a quad just delegates to the base implementation, nothing
       else needs to be cleaned up */
    data.layerDataHandleOverloads ?
        layer.remove(dataHandleData(second)) :
        layer.remove(second);
    CORRADE_VERIFY(!layer.isHandleValid(second));
}

void LayoutLayerTest::createRemoveHandleRecycle() {
    LayoutLayer layer{layerHandle(0, 1), 17};

    DataHandle first = layer.create(0);
    DataHandle second = layer.create(9, nodeHandle(0x12345, 0xabc));
    CORRADE_COMPARE(layer.style(first), 0);
    CORRADE_COMPARE(layer.style(second), 9);
    CORRADE_COMPARE(layer.node(second), nodeHandle(0x12345, 0xabc));

    /* Data that reuses a previous slot should have all properties (which ...
       is just the style and node now actually) set back to defaults */
    layer.remove(second);
    DataHandle second2 = layer.create(13, nodeHandle(0xabcde, 0x123));
    CORRADE_COMPARE(dataHandleId(second2), dataHandleId(second));
    CORRADE_COMPARE(layer.style(second2), 13);
    CORRADE_COMPARE(layer.node(second2), nodeHandle(0xabcde, 0x123));
}

template<class T> void LayoutLayerTest::setStyle() {
    auto&& data = SetStyleData[testCaseInstanceId()];
    setTestCaseDescription(data.name);
    setTestCaseTemplateName(std::is_same<T, Enum>::value ? "Enum" : "UnsignedInt");

    LayoutLayer layer{layerHandle(0, 1), 17};

    /* Just to be sure the setters aren't picking up the first ever data
       always */
    layer.create(2);

    DataHandle layerData = layer.create(12, data.node);
    CORRADE_COMPARE(layer.style(layerData), 12);
    CORRADE_COMPARE(layer.state(), data.expectedInitialState);

    /* Clear the state flags */
    layer.update(data.expectedInitialState, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Setting a style marks the layer as dirty */
    layer.setStyle(layerData, T(7));
    CORRADE_COMPARE(layer.style(layerData), 7);
    CORRADE_COMPARE(layer.state(), data.expectedState);

    /* Clear the state flags, if there are any */
    if(data.expectedState) {
        layer.update(data.expectedState, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }

    /* Testing also the other overload */
    layer.setStyle(dataHandleData(layerData), T(16));
    CORRADE_COMPARE(layer.style(layerData), 16);
    CORRADE_COMPARE(layer.state(), data.expectedState);
}

void LayoutLayerTest::invalidHandle() {
    CORRADE_SKIP_IF_NO_ASSERT();

    LayoutLayer layer{layerHandle(0, 1), 17};

    Containers::String out;
    Error redirectError{&out};
    layer.style(DataHandle::Null);
    layer.style(LayerDataHandle::Null);
    layer.style<Enum>(DataHandle::Null);
    layer.style<Enum>(LayerDataHandle::Null);
    layer.setStyle(DataHandle::Null, 0);
    layer.setStyle(LayerDataHandle::Null, 0);
    layer.setStyle(DataHandle::Null, Enum(0));
    layer.setStyle(LayerDataHandle::Null, Enum(0));
    CORRADE_COMPARE_AS(out,
        "Ui::LayoutLayer::style(): invalid handle Ui::DataHandle::Null\n"
        "Ui::LayoutLayer::style(): invalid handle Ui::LayerDataHandle::Null\n"
        "Ui::LayoutLayer::style(): invalid handle Ui::DataHandle::Null\n"
        "Ui::LayoutLayer::style(): invalid handle Ui::LayerDataHandle::Null\n"
        "Ui::LayoutLayer::setStyle(): invalid handle Ui::DataHandle::Null\n"
        "Ui::LayoutLayer::setStyle(): invalid handle Ui::LayerDataHandle::Null\n"
        "Ui::LayoutLayer::setStyle(): invalid handle Ui::DataHandle::Null\n"
        "Ui::LayoutLayer::setStyle(): invalid handle Ui::LayerDataHandle::Null\n",
        TestSuite::Compare::String);
}

void LayoutLayerTest::styleOutOfRange() {
    CORRADE_SKIP_IF_NO_ASSERT();

    LayoutLayer layer{layerHandle(0, 1), 17};
    DataHandle data = layer.create(9);

    Containers::String out;
    Error redirectError{&out};
    layer.create(17);
    layer.create(Enum(17));
    layer.setStyle(data, 17);
    layer.setStyle(data, Enum(17));
    layer.setStyle(dataHandleData(data), 17);
    layer.setStyle(dataHandleData(data), Enum(17));
    CORRADE_COMPARE_AS(out,
        "Ui::LayoutLayer::create(): style 17 out of range for 17 styles\n"
        "Ui::LayoutLayer::create(): style 17 out of range for 17 styles\n"
        "Ui::LayoutLayer::setStyle(): style 17 out of range for 17 styles\n"
        "Ui::LayoutLayer::setStyle(): style 17 out of range for 17 styles\n"
        "Ui::LayoutLayer::setStyle(): style 17 out of range for 17 styles\n"
        "Ui::LayoutLayer::setStyle(): style 17 out of range for 17 styles\n",
        TestSuite::Compare::String);
}

void LayoutLayerTest::layoutEmpty() {
    LayoutLayer layer{layerHandle(0, 1), 17};
    layer.setStyle({}, {}, {}, {}, {});

    /* Shouldn't crash or do anything mysterious */
    layer.layout({}, {}, {}, {}, {}, {});
    CORRADE_VERIFY(true);
}

void LayoutLayerTest::layout() {
    auto&& data = LayoutData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    LayoutLayer layer{layerHandle(0, 1), 4};

    enum Style {
        Excessive = 0,
        Defaults = 1,
        Reasonable = 2,
        Another = 3
    };

    Vector2 styleMinSizes[]{
        {1000.0f, 1000.0f},
        {},
        Math::lerp(Vector2{}, {10.0f, 20.0f}, data.minSize),
        Math::lerp(Vector2{}, {30.0f, 10.0f}, data.minSize)
    };
    Vector2 styleMaxSizes[]{
        {0.001f, 0.001f},
        Vector2{Constants::inf()},
        Math::lerp(Vector2{Constants::inf()}, {3.0f, 1.0f}, data.maxSize),
        Math::lerp(Vector2{Constants::inf()}, {1.0f, 2.0f}, data.maxSize)
    };
    Float styleAspectRatios[]{
        10000.0f,
        0.0f,
        data.aspectRatio ? 1.5f : 0.0f,
        data.aspectRatio ? 1.35f : 0.0f
    };
    Vector4 stylePaddings[]{
        {1000.0f, 1000.0f, 1000.0f, 1000.0f},
        {},
        Math::lerp(Vector4{}, {1.0f, 2.0f, 3.0f, 4.0f}, data.padding),
        Math::lerp(Vector4{}, {4.0f, 1.0f, 2.0f, 3.0f}, data.padding)
    };
    Vector4 styleMargins[]{
        {10000.0f, 10000.0f, 10000.0f, 10000.0f},
        {},
        Math::lerp(Vector4{}, {10.0f, 20.0f, 30.0f, 40.0f}, data.margin),
        Math::lerp(Vector4{}, {40.0f, 10.0f, 20.0f, 30.0f}, data.margin)
    };
    layer.setStyle(
        data.minSize.any() ? Containers::arrayView(styleMinSizes) : nullptr,
        data.maxSize.any() ? Containers::arrayView(styleMaxSizes) : nullptr,
        data.aspectRatio ? Containers::arrayView(styleAspectRatios) : nullptr,
        data.padding.any() ? Containers::arrayView(stylePaddings) : nullptr,
        data.margin.any() ? Containers::arrayView(styleMargins) : nullptr);

    NodeHandle defaults = nodeHandle(0, 0xabc);
    NodeHandle oneLayout = nodeHandle(2, 0xabc);
    /*NodeHandle noLayouts =*/ nodeHandle(3, 0xabc);
    NodeHandle twoLayouts = nodeHandle(4, 0xabc);
    NodeHandle notSelected = nodeHandle(6, 0xabc);

    /* Add the data in order that doesn't match style or node order to verify
       the indices aren't accidentally misused */
    DataHandle first = layer.create(Reasonable, oneLayout);
    DataHandle second = layer.create(Defaults, defaults);
    /* The Excessive style won't get used for anything */
    /*DataHandle third =*/ layer.create(Excessive, notSelected);
    DataHandle fourth = layer.create(Another, twoLayouts);
    /*DataHandle fifth =*/ layer.create(Excessive, NodeHandle::Null);
    DataHandle sixth = layer.create(Reasonable, twoLayouts);

    Vector2 nodeMinSizes[]{
        {9.0f, 99.0f},      /* 0, defaults */
        {999.0f, 9.0f},     /* 1 */
        {3.5f, 1.5f},       /* 2, oneLayout */
        {-99.0f, -9.0f},    /* 3, noLayouts */
        {-2.5f, 45.0f},     /* 4, twoLayouts */
        {99.0f, 9.0f},      /* 5 */
        {9.0f, 999.0f},     /* 6, notSelected */
        {99.0f, 99.0f},     /* 7 */
    };
    Vector2 nodeMaxSizes[]{
        {8.0f, 88.0f},      /* 0, defaults */
        {888.0f, 8.0f},     /* 1 */
        {3.25f, 1.25f},     /* 2, oneLayout */
        {Constants::inf(), -8.0f}, /* 3, noLayouts */
        {0.75f, Constants::inf()}, /* 4, twoLayouts */
        {88.0f, 8.0f},      /* 5 */
        {8.0f, 888.0f},     /* 6, notSelected */
        {88.0f, 88.0f},     /* 7 */
    };
    Float nodeAspectRatios[]{
        7.0f,               /* 0, defaults */
        777.0f,             /* 1 */
        0.0f,               /* 2, oneLayout */
        -77.0f,             /* 3, noLayouts */
        0.0f,               /* 4, twoLayouts */
        77.0f,              /* 5 */
        7.0f,               /* 6, notSelected */
        77.0f,              /* 7 */
    };
    Vector4 nodePaddings[]{
        {6.0f, 66.0f, 666.0f, 6666.0f}, /* 0, defaults */
        {666.0f, 6.0f, 66.0f, 66.0f},   /* 1 */
        {0.35f, 0.15f, 0.65f, 0.05f},   /* 2, oneLayout */
        {-66.0f, 6.0f, 666.0f, -66.0f}, /* 3, noLayouts */
        {-0.25f, 0.45f, 8.7f, 0.0f},    /* 4, twoLayouts */
        {66.0f, 6.0f, 6666.0f, 6.0f},   /* 5 */
        {6.0f, 666.0f, 66.0f, 6.0f},    /* 6, notSelected */
        {66.0f, 66.0f, 6666.0f, 6.0f},  /* 7 */
    };
    Vector4 nodeMargins[]{
        {5.0f, 55.0f, 556.0f, 5555.0f}, /* 0, defaults */
        {555.0f, 5.0f, 55.0f, 55.0f},   /* 1 */
        {5.0f, 15.0f, 25.0f, 35.0f},    /* 2, oneLayout */
        {-55.0f, 5.0f, 555.0f, -55.0f}, /* 3, noLayouts */
        {-25.0f, 45.0f, 17.0f, 0.0f},   /* 4, twoLayouts */
        {55.0f, 5.0f, 5555.0f, 5.0f},   /* 5 */
        {5.0f, 556.0f, 55.0f, 5.0f},    /* 6, notSelected */
        {55.0f, 55.0f, 5555.0f, 5.0f},  /* 7 */
    };

    UnsignedByte dataIdsToLayoutStorage[1]{};
    Containers::MutableBitArrayView dataIdsToLayout{dataIdsToLayoutStorage, 0, 6};
    dataIdsToLayout.set(dataHandleId(first));
    dataIdsToLayout.set(dataHandleId(second));
    dataIdsToLayout.set(dataHandleId(fourth));
    dataIdsToLayout.set(dataHandleId(sixth));

    layer.layout(dataIdsToLayout, nodeMinSizes, nodeMaxSizes, nodeAspectRatios, nodePaddings, nodeMargins);

    CORRADE_COMPARE_AS(Containers::arrayView(nodeMinSizes), Containers::arrayView<Vector2>({
        {9.0f, 99.0f},      /* 0, defaults */
        {999.0f, 9.0f},     /* 1 */
        {data.minSize[0] ? 10.0f : 3.5f,
         data.minSize[1] ? 20.0f : 1.5f}, /* 2, oneLayout */
        /* Negative values stay because this node isn't touched by any layout */
        {-99.0f, -9.0f},    /* 3, noLayouts */
        /* Negative value is clamped to 0 even if no style is set because
           that's style default. Otherwise the max of the two styles (and the
           input) is picked. */
        {data.minSize[0] ? 30.0f : 0.0f,
         45.0f}, /* 4, twoLayouts */
        {99.0f, 9.0f},      /* 5 */
        {9.0f, 999.0f},     /* 6, notSelected */
        {99.0f, 99.0f},     /* 7 */
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(nodeMaxSizes), Containers::arrayView<Vector2>({
        {8.0f, 88.0f},      /* 0, defaults */
        {888.0f, 8.0f},     /* 1 */
        {data.maxSize[0] ? 3.0f : 3.25f,
         data.maxSize[1] ? 1.0f : 1.25f},  /* 2, oneLayout */
        /* Infinities stay because this node isn't touched by any layout,
           negative values as well because that's the min */
        {Constants::inf(), -8.0f}, /* 3, noLayouts */
        /* Min of the two styles (and the input) is picked */
        {0.75f,
         data.maxSize[1] ? 1.0f : Constants::inf()}, /* 4, twoLayouts */
        {88.0f, 8.0f},      /* 5 */
        {8.0f, 888.0f},     /* 6, notSelected */
        {88.0f, 88.0f},     /* 7 */
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(nodeAspectRatios), Containers::arrayView({
        7.0f,               /* 0, defaults */
        777.0f,             /* 1 */
        data.aspectRatio ? 1.5f : 0.0f, /* 2, oneLayout */
        -77.0f,             /* 3, noLayouts */
        /* The Another style is used for a lower data ID that the Reasonable
           style, so it gets picked first */
        data.aspectRatio ? 1.35f : 0.0f, /* 4, twoLayouts */
        77.0f,              /* 5 */
        7.0f,               /* 6, notSelected */
        77.0f,              /* 7 */
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(nodePaddings), Containers::arrayView<Vector4>({
        {6.0f, 66.0f, 666.0f, 6666.0f}, /* 0, defaults */
        {666.0f, 6.0f, 66.0f, 66.0f},   /* 1 */
        {data.padding[0] ? 1.0f : 0.35f,
         data.padding[1] ? 2.0f : 0.15f,
         data.padding[2] ? 3.0f : 0.65f,
         data.padding[3] ? 4.0f : 0.05f}, /* 2, oneLayout */
        {-66.0f, 6.0f, 666.0f, -66.0f}, /* 3, noLayouts */
        /* Negative value is clamped to 0 even if no style is set because
           that's style default. Otherwise the max of the two styles (and the
           input) is picked. */
        {data.padding[0] ? 4.0f : 0.0f,
         data.padding[1] ? 2.0f : 0.45f,
         8.7f,
         data.padding[3] ? 4.0f : 0.0f}, /* 4, twoLayouts */
        {66.0f, 6.0f, 6666.0f, 6.0f},   /* 5 */
        {6.0f, 666.0f, 66.0f, 6.0f},    /* 6, notSelected */
        {66.0f, 66.0f, 6666.0f, 6.0f},  /* 7 */
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(Containers::arrayView(nodeMargins), Containers::arrayView<Vector4>({
        {5.0f, 55.0f, 556.0f, 5555.0f}, /* 0, defaults */
        {555.0f, 5.0f, 55.0f, 55.0f},   /* 1 */
        {data.margin[0] ? 10.0f : 5.0f,
         data.margin[1] ? 20.0f : 15.0f,
         data.margin[2] ? 30.0f : 25.0f,
         data.margin[3] ? 40.0f : 35.0f}, /* 2, oneLayout */
        {-55.0f, 5.0f, 555.0f, -55.0f}, /* 3, noLayouts */
        /* Negative value is clamped to 0 even if no style is set because
           that's style default. Otherwise the max of the two styles (and the
           input) is picked. */
        {data.margin[0] ? 40.0f : 0.0f,
         45.0f,
         data.margin[2] ? 30.0f : 17.0f,
         data.margin[3] ? 40.0f : 0.0f}, /* 4, twoLayouts */
        {55.0f, 5.0f, 5555.0f, 5.0f},   /* 5 */
        {5.0f, 556.0f, 55.0f, 5.0f},    /* 6, notSelected */
        {55.0f, 55.0f, 5555.0f, 5.0f},  /* 7 */
    }), TestSuite::Compare::Container);
}

void LayoutLayerTest::layoutNoStyleSet() {
    CORRADE_SKIP_IF_NO_ASSERT();

    LayoutLayer layer{layerHandle(0, 1), 17};

    Containers::String out;
    Error redirectError{&out};
    layer.layout({}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(out, "Ui::LayoutLayer::layout(): no style data was set\n");
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::LayoutLayerTest)
