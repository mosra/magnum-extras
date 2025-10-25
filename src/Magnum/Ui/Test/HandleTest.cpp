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

#include <Corrade/Containers/String.h>
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/String.h>

#include "Magnum/Ui/Handle.h"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct HandleTest: TestSuite::Tester {
    explicit HandleTest();

    void layer();
    void layerInvalid();
    void debugLayer();
    void debugLayerPacked();

    void layerData();
    void layerDataInvalid();
    void debugLayerData();
    void debugLayerDataPacked();

    void data();
    void dataInvalid();
    void debugData();
    void debugDataPacked();

    void node();
    void nodeInvalid();
    void debugNode();
    void debugNodePacked();

    void layouter();
    void layouterInvalid();
    void debugLayouter();
    void debugLayouterPacked();

    void layouterData();
    void layouterDataInvalid();
    void debugLayouterData();
    void debugLayouterDataPacked();

    void layout();
    void layoutInvalid();
    void debugLayout();
    void debugLayoutPacked();

    void animator();
    void animatorInvalid();
    void debugAnimator();
    void debugAnimatorPacked();

    void animatorData();
    void animatorDataInvalid();
    void debugAnimatorData();
    void debugAnimatorDataPacked();

    void animation();
    void animationInvalid();
    void debugAnimation();
    void debugAnimationPacked();
};

HandleTest::HandleTest() {
    addTests({&HandleTest::layer,
              &HandleTest::layerInvalid,
              &HandleTest::debugLayer,
              &HandleTest::debugLayerPacked,

              &HandleTest::layerData,
              &HandleTest::layerDataInvalid,
              &HandleTest::debugLayerData,
              &HandleTest::debugLayerDataPacked,

              &HandleTest::data,
              &HandleTest::dataInvalid,
              &HandleTest::debugData,
              &HandleTest::debugDataPacked,

              &HandleTest::node,
              &HandleTest::nodeInvalid,
              &HandleTest::debugNode,
              &HandleTest::debugNodePacked,

              &HandleTest::layouter,
              &HandleTest::layouterInvalid,
              &HandleTest::debugLayouter,
              &HandleTest::debugLayouterPacked,

              &HandleTest::layouterData,
              &HandleTest::layouterDataInvalid,
              &HandleTest::debugLayouterData,
              &HandleTest::debugLayouterDataPacked,

              &HandleTest::layout,
              &HandleTest::layoutInvalid,
              &HandleTest::debugLayout,
              &HandleTest::debugLayoutPacked,

              &HandleTest::animator,
              &HandleTest::animatorInvalid,
              &HandleTest::debugAnimator,
              &HandleTest::debugAnimatorPacked,

              &HandleTest::animatorData,
              &HandleTest::animatorDataInvalid,
              &HandleTest::debugAnimatorData,
              &HandleTest::debugAnimatorDataPacked,

              &HandleTest::animation,
              &HandleTest::animationInvalid,
              &HandleTest::debugAnimation,
              &HandleTest::debugAnimationPacked});
}

void HandleTest::layer() {
    CORRADE_COMPARE(LayerHandle::Null, LayerHandle{});
    CORRADE_COMPARE(layerHandle(0, 0), LayerHandle{});
    CORRADE_COMPARE(layerHandle(0xab, 0x12), LayerHandle(0x12ab));
    CORRADE_COMPARE(layerHandle(0xff, 0xff), LayerHandle(0xffff));
    CORRADE_COMPARE(layerHandleId(LayerHandle(0x12ab)), 0xab);
    CORRADE_COMPARE(layerHandleGeneration(LayerHandle::Null), 0);
    CORRADE_COMPARE(layerHandleGeneration(LayerHandle(0x12ab)), 0x12);

    constexpr LayerHandle handle = layerHandle(0xab, 0x12);
    constexpr UnsignedInt id = layerHandleId(handle);
    constexpr UnsignedInt generation = layerHandleGeneration(handle);
    CORRADE_COMPARE(handle, LayerHandle(0x12ab));
    CORRADE_COMPARE(id, 0xab);
    CORRADE_COMPARE(generation, 0x12);
}

void HandleTest::layerInvalid() {
    CORRADE_SKIP_IF_NO_DEBUG_ASSERT();

    /* Verify the zero generation check isn't off by a bit */
    layerHandleId(layerHandle(0, 1));
    layerHandleId(layerHandle(0, 1 << (Implementation::LayerHandleGenerationBits - 1)));

    Containers::String out;
    Error redirectError{&out};
    layerHandle(0x100, 0x1);
    layerHandle(0x1, 0x100);
    layerHandleId(LayerHandle::Null);
    layerHandleId(layerHandle(0xab, 0));
    CORRADE_COMPARE_AS(out,
        "Ui::layerHandle(): expected index to fit into 8 bits and generation into 8, got 0x100 and 0x1\n"
        "Ui::layerHandle(): expected index to fit into 8 bits and generation into 8, got 0x1 and 0x100\n"
        "Ui::layerHandleId(): invalid handle Ui::LayerHandle::Null\n"
        "Ui::layerHandleId(): invalid handle Ui::LayerHandle(0xab, 0x0)\n",
        TestSuite::Compare::String);
}

void HandleTest::debugLayer() {
    Containers::String out;
    Debug{&out} << LayerHandle::Null << layerHandle(0x12, 0xab);
    CORRADE_COMPARE(out, "Ui::LayerHandle::Null Ui::LayerHandle(0x12, 0xab)\n");
}

void HandleTest::debugLayerPacked() {
    Containers::String out;
    /* Last is not packed, ones before should not make any flags persistent */
    Debug{&out} << Debug::packed << LayerHandle::Null << Debug::packed << layerHandle(0x12, 0xab) << layerHandle(0x34, 0xcd);
    CORRADE_COMPARE(out, "Null {0x12, 0xab} Ui::LayerHandle(0x34, 0xcd)\n");
}

void HandleTest::layerData() {
    CORRADE_COMPARE(LayerDataHandle::Null, LayerDataHandle{});
    CORRADE_COMPARE(layerDataHandle(0, 0), LayerDataHandle::Null);
    CORRADE_COMPARE(layerDataHandle(0xabcde, 0x123), LayerDataHandle(0x123abcde));
    CORRADE_COMPARE(layerDataHandle(0xfffff, 0xfff), LayerDataHandle(0xffffffff));
    CORRADE_COMPARE(layerDataHandleId(LayerDataHandle(0x123abcde)), 0xabcde);
    CORRADE_COMPARE(layerDataHandleGeneration(LayerDataHandle::Null), 0);
    CORRADE_COMPARE(layerDataHandleGeneration(LayerDataHandle(0x123abcde)), 0x123);

    constexpr LayerDataHandle handle = layerDataHandle(0xabcde, 0x123);
    constexpr UnsignedInt id = layerDataHandleId(handle);
    constexpr UnsignedInt generation = layerDataHandleGeneration(handle);
    CORRADE_COMPARE(handle, LayerDataHandle(0x123abcde));
    CORRADE_COMPARE(id, 0xabcde);
    CORRADE_COMPARE(generation, 0x123);
}

void HandleTest::layerDataInvalid() {
    CORRADE_SKIP_IF_NO_DEBUG_ASSERT();

    /* Verify the zero generation check isn't off by a bit */
    layerDataHandleId(layerDataHandle(0, 1));
    layerDataHandleId(layerDataHandle(0, 1 << (Implementation::LayerDataHandleGenerationBits - 1)));

    Containers::String out;
    Error redirectError{&out};
    layerDataHandle(0x100000, 0x1);
    layerDataHandle(0x1, 0x1000);
    layerDataHandleId(LayerDataHandle::Null);
    layerDataHandleId(layerDataHandle(0xabcde, 0));
    CORRADE_COMPARE_AS(out,
        "Ui::layerDataHandle(): expected index to fit into 20 bits and generation into 12, got 0x100000 and 0x1\n"
        "Ui::layerDataHandle(): expected index to fit into 20 bits and generation into 12, got 0x1 and 0x1000\n"
        "Ui::layerDataHandleId(): invalid handle Ui::LayerDataHandle::Null\n"
        "Ui::layerDataHandleId(): invalid handle Ui::LayerDataHandle(0xabcde, 0x0)\n",
        TestSuite::Compare::String);
}

void HandleTest::debugLayerData() {
    Containers::String out;
    Debug{&out} << LayerDataHandle::Null << layerDataHandle(0x12345, 0xabc);
    CORRADE_COMPARE(out, "Ui::LayerDataHandle::Null Ui::LayerDataHandle(0x12345, 0xabc)\n");
}

void HandleTest::debugLayerDataPacked() {
    Containers::String out;
    /* Last is not packed, ones before should not make any flags persistent */
    Debug{&out} << Debug::packed << LayerDataHandle::Null << Debug::packed << layerDataHandle(0x12345, 0xabc) << layerDataHandle(0x67890, 0xdef);
    CORRADE_COMPARE(out, "Null {0x12345, 0xabc} Ui::LayerDataHandle(0x67890, 0xdef)\n");
}

void HandleTest::data() {
    CORRADE_COMPARE(DataHandle::Null, DataHandle{});
    CORRADE_COMPARE(dataHandle(LayerHandle::Null, 0, 0), DataHandle::Null);
    CORRADE_COMPARE(dataHandle(LayerHandle(0x12ab), 0x34567, 0xcde), DataHandle(0x12abcde34567));
    CORRADE_COMPARE(dataHandle(LayerHandle(0xffff), 0xfffff, 0xfff), DataHandle(0xffffffffffff));
    CORRADE_COMPARE(dataHandle(LayerHandle::Null, LayerDataHandle::Null), DataHandle::Null);
    CORRADE_COMPARE(dataHandle(LayerHandle(0x12ab), LayerDataHandle(0xcde34567)), DataHandle(0x12abcde34567));
    CORRADE_COMPARE(dataHandleLayer(DataHandle::Null), LayerHandle::Null);
    CORRADE_COMPARE(dataHandleLayer(DataHandle(0x12abcde34567)), LayerHandle(0x12ab));
    CORRADE_COMPARE(dataHandleData(DataHandle::Null), LayerDataHandle::Null);
    CORRADE_COMPARE(dataHandleData(DataHandle(0x12abcde34567)), LayerDataHandle(0xcde34567));
    CORRADE_COMPARE(dataHandleLayerId(DataHandle(0x12abcde34567)), 0xab);
    CORRADE_COMPARE(dataHandleLayerGeneration(DataHandle::Null), 0);
    CORRADE_COMPARE(dataHandleLayerGeneration(DataHandle(0x12abcde34567)), 0x12);
    CORRADE_COMPARE(dataHandleId(DataHandle(0x12abcde34567)), 0x34567);
    CORRADE_COMPARE(dataHandleGeneration(DataHandle::Null), 0);
    CORRADE_COMPARE(dataHandleGeneration(DataHandle(0x12abcde34567)), 0xcde);

    constexpr DataHandle handle1 = dataHandle(LayerHandle(0x12ab), 0x34567, 0xcde);
    constexpr DataHandle handle2 = dataHandle(LayerHandle(0x12ab), LayerDataHandle(0xcde34567));
    constexpr LayerHandle layer = dataHandleLayer(handle1);
    constexpr LayerDataHandle data = dataHandleData(handle1);
    constexpr UnsignedInt layerId = dataHandleLayerId(handle1);
    constexpr UnsignedInt layerGeneration = dataHandleLayerGeneration(handle1);
    constexpr UnsignedInt id = dataHandleId(handle1);
    constexpr UnsignedInt generation = dataHandleGeneration(handle1);
    CORRADE_COMPARE(handle1, DataHandle(0x12abcde34567));
    CORRADE_COMPARE(handle2, DataHandle(0x12abcde34567));
    CORRADE_COMPARE(layer, LayerHandle(0x12ab));
    CORRADE_COMPARE(data, LayerDataHandle(0xcde34567));
    CORRADE_COMPARE(layerId, 0xab);
    CORRADE_COMPARE(layerGeneration, 0x12);
    CORRADE_COMPARE(id, 0x34567);
    CORRADE_COMPARE(generation, 0xcde);
}

void HandleTest::dataInvalid() {
    CORRADE_SKIP_IF_NO_DEBUG_ASSERT();

    /* Verify the zero generation check isn't off by a bit. The other
       generation being zero shouldn't matter. */
    dataHandleLayerId(dataHandle(layerHandle(0, 1), LayerDataHandle::Null));
    dataHandleLayerId(dataHandle(layerHandle(0, 1 << (Implementation::LayerHandleGenerationBits - 1)), LayerDataHandle::Null));
    dataHandleId(dataHandle(LayerHandle::Null, 0, 1));
    dataHandleId(dataHandle(LayerHandle::Null, 0, 1 << (Implementation::LayerDataHandleGenerationBits - 1)));

    Containers::String out;
    Error redirectError{&out};
    dataHandle(LayerHandle::Null, 0x100000, 0x1);
    dataHandle(LayerHandle::Null, 0x1, 0x1000);
    dataHandleLayerId(DataHandle::Null);
    dataHandleLayerId(dataHandle(LayerHandle::Null, 0x1, 0x1));
    dataHandleLayerId(dataHandle(layerHandle(0xab, 0), 0x1, 0x1));
    dataHandleId(DataHandle::Null);
    dataHandleId(dataHandle(layerHandle(0x1, 0x1), LayerDataHandle::Null));
    dataHandleId(dataHandle(layerHandle(0x1, 0x1), layerDataHandle(0xabcde, 0)));
    CORRADE_COMPARE_AS(out,
        "Ui::dataHandle(): expected index to fit into 20 bits and generation into 12, got 0x100000 and 0x1\n"
        "Ui::dataHandle(): expected index to fit into 20 bits and generation into 12, got 0x1 and 0x1000\n"
        "Ui::dataHandleLayerId(): invalid layer portion of Ui::DataHandle::Null\n"
        "Ui::dataHandleLayerId(): invalid layer portion of Ui::DataHandle(Null, {0x1, 0x1})\n"
        "Ui::dataHandleLayerId(): invalid layer portion of Ui::DataHandle({0xab, 0x0}, {0x1, 0x1})\n"
        "Ui::dataHandleId(): invalid data portion of Ui::DataHandle::Null\n"
        "Ui::dataHandleId(): invalid data portion of Ui::DataHandle({0x1, 0x1}, Null)\n"
        "Ui::dataHandleId(): invalid data portion of Ui::DataHandle({0x1, 0x1}, {0xabcde, 0x0})\n",
        TestSuite::Compare::String);
}

void HandleTest::debugData() {
    Containers::String out;
    Debug{&out} << DataHandle::Null << dataHandle(LayerHandle::Null, layerDataHandle(0xabcde, 0x12)) << dataHandle(layerHandle(0x34, 0x56), LayerDataHandle::Null) << dataHandle(layerHandle(0x34, 0x56), 0xabcde, 0x12);
    CORRADE_COMPARE(out, "Ui::DataHandle::Null Ui::DataHandle(Null, {0xabcde, 0x12}) Ui::DataHandle({0x34, 0x56}, Null) Ui::DataHandle({0x34, 0x56}, {0xabcde, 0x12})\n");
}

void HandleTest::debugDataPacked() {
    Containers::String out;
    /* Last is not packed, ones before should not make any flags persistent */
    Debug{&out} << Debug::packed << DataHandle::Null << Debug::packed << dataHandle(LayerHandle::Null, layerDataHandle(0xabcde, 0x12)) << Debug::packed << dataHandle(layerHandle(0x34, 0x56), LayerDataHandle::Null) << Debug::packed << dataHandle(layerHandle(0x34, 0x56), 0xabcde, 0x12) << dataHandle(layerHandle(0x78, 0x90), 0xf0123, 0xab);
    CORRADE_COMPARE(out, "Null {Null, {0xabcde, 0x12}} {{0x34, 0x56}, Null} {{0x34, 0x56}, {0xabcde, 0x12}} Ui::DataHandle({0x78, 0x90}, {0xf0123, 0xab})\n");
}

void HandleTest::node() {
    CORRADE_COMPARE(NodeHandle::Null, NodeHandle{});
    CORRADE_COMPARE(nodeHandle(0, 0), NodeHandle::Null);
    CORRADE_COMPARE(nodeHandle(0xabcde, 0x123), NodeHandle(0x123abcde));
    CORRADE_COMPARE(nodeHandle(0xfffff, 0xfff), NodeHandle(0xffffffff));
    CORRADE_COMPARE(nodeHandleId(NodeHandle(0x123abcde)), 0xabcde);
    CORRADE_COMPARE(nodeHandleGeneration(NodeHandle::Null), 0);
    CORRADE_COMPARE(nodeHandleGeneration(NodeHandle(0x123abcde)), 0x123);

    constexpr NodeHandle handle = nodeHandle(0xabcde, 0x123);
    constexpr UnsignedInt id = nodeHandleId(handle);
    constexpr UnsignedInt generation = nodeHandleGeneration(handle);
    CORRADE_COMPARE(handle, NodeHandle(0x123abcde));
    CORRADE_COMPARE(id, 0xabcde);
    CORRADE_COMPARE(generation, 0x123);
}

void HandleTest::nodeInvalid() {
    CORRADE_SKIP_IF_NO_DEBUG_ASSERT();

    /* Verify the zero generation check isn't off by a bit */
    nodeHandleId(nodeHandle(0, 1));
    nodeHandleId(nodeHandle(0, 1 << (Implementation::NodeHandleGenerationBits - 1)));

    Containers::String out;
    Error redirectError{&out};
    nodeHandle(0x100000, 0x1);
    nodeHandle(0x1, 0x1000);
    nodeHandleId(NodeHandle::Null);
    nodeHandleId(nodeHandle(0xabcde, 0));
    CORRADE_COMPARE_AS(out,
        "Ui::nodeHandle(): expected index to fit into 20 bits and generation into 12, got 0x100000 and 0x1\n"
        "Ui::nodeHandle(): expected index to fit into 20 bits and generation into 12, got 0x1 and 0x1000\n"
        "Ui::nodeHandleId(): invalid handle Ui::NodeHandle::Null\n"
        "Ui::nodeHandleId(): invalid handle Ui::NodeHandle(0xabcde, 0x0)\n",
        TestSuite::Compare::String);
}

void HandleTest::debugNode() {
    Containers::String out;
    Debug{&out} << NodeHandle::Null << nodeHandle(0x12345, 0xabc);
    CORRADE_COMPARE(out, "Ui::NodeHandle::Null Ui::NodeHandle(0x12345, 0xabc)\n");
}

void HandleTest::debugNodePacked() {
    Containers::String out;
    /* Last is not packed, ones before should not make any flags persistent */
    Debug{&out} << Debug::packed << NodeHandle::Null << Debug::packed << nodeHandle(0x12345, 0xabc) << nodeHandle(0x67890, 0xdef);
    CORRADE_COMPARE(out, "Null {0x12345, 0xabc} Ui::NodeHandle(0x67890, 0xdef)\n");
}

void HandleTest::layouter() {
    CORRADE_COMPARE(LayouterHandle::Null, LayouterHandle{});
    CORRADE_COMPARE(layouterHandle(0, 0), LayouterHandle{});
    CORRADE_COMPARE(layouterHandle(0xab, 0x12), LayouterHandle(0x12ab));
    CORRADE_COMPARE(layouterHandle(0xff, 0xff), LayouterHandle(0xffff));
    CORRADE_COMPARE(layouterHandleId(LayouterHandle(0x12ab)), 0xab);
    CORRADE_COMPARE(layouterHandleGeneration(LayouterHandle::Null), 0);
    CORRADE_COMPARE(layouterHandleGeneration(LayouterHandle(0x12ab)), 0x12);

    constexpr LayouterHandle handle = layouterHandle(0xab, 0x12);
    constexpr UnsignedInt id = layouterHandleId(handle);
    constexpr UnsignedInt generation = layouterHandleGeneration(handle);
    CORRADE_COMPARE(handle, LayouterHandle(0x12ab));
    CORRADE_COMPARE(id, 0xab);
    CORRADE_COMPARE(generation, 0x12);
}

void HandleTest::layouterInvalid() {
    CORRADE_SKIP_IF_NO_DEBUG_ASSERT();

    /* Verify the zero generation check isn't off by a bit */
    layouterHandleId(layouterHandle(0, 1));
    layouterHandleId(layouterHandle(0, 1 << (Implementation::LayouterHandleGenerationBits - 1)));

    Containers::String out;
    Error redirectError{&out};
    layouterHandle(0x100, 0x1);
    layouterHandle(0x1, 0x100);
    layouterHandleId(LayouterHandle::Null);
    layouterHandleId(layouterHandle(0xab, 0));
    CORRADE_COMPARE_AS(out,
        "Ui::layouterHandle(): expected index to fit into 8 bits and generation into 8, got 0x100 and 0x1\n"
        "Ui::layouterHandle(): expected index to fit into 8 bits and generation into 8, got 0x1 and 0x100\n"
        "Ui::layouterHandleId(): invalid handle Ui::LayouterHandle::Null\n"
        "Ui::layouterHandleId(): invalid handle Ui::LayouterHandle(0xab, 0x0)\n",
        TestSuite::Compare::String);
}

void HandleTest::debugLayouter() {
    Containers::String out;
    Debug{&out} << LayouterHandle::Null << layouterHandle(0x12, 0xab);
    CORRADE_COMPARE(out, "Ui::LayouterHandle::Null Ui::LayouterHandle(0x12, 0xab)\n");
}

void HandleTest::debugLayouterPacked() {
    Containers::String out;
    /* Last is not packed, ones before should not make any flags persistent */
    Debug{&out} << Debug::packed << LayouterHandle::Null << Debug::packed << layouterHandle(0x12, 0xab) << layouterHandle(0x34, 0xcd);
    CORRADE_COMPARE(out, "Null {0x12, 0xab} Ui::LayouterHandle(0x34, 0xcd)\n");
}

void HandleTest::layouterData() {
    CORRADE_COMPARE(LayouterDataHandle::Null, LayouterDataHandle{});
    CORRADE_COMPARE(layouterDataHandle(0, 0), LayouterDataHandle::Null);
    CORRADE_COMPARE(layouterDataHandle(0xabcde, 0x123), LayouterDataHandle(0x123abcde));
    CORRADE_COMPARE(layouterDataHandle(0xfffff, 0xfff), LayouterDataHandle(0xffffffff));
    CORRADE_COMPARE(layouterDataHandleId(LayouterDataHandle(0x123abcde)), 0xabcde);
    CORRADE_COMPARE(layouterDataHandleGeneration(LayouterDataHandle::Null), 0);
    CORRADE_COMPARE(layouterDataHandleGeneration(LayouterDataHandle(0x123abcde)), 0x123);

    constexpr LayouterDataHandle handle = layouterDataHandle(0xabcde, 0x123);
    constexpr UnsignedInt id = layouterDataHandleId(handle);
    constexpr UnsignedInt generation = layouterDataHandleGeneration(handle);
    CORRADE_COMPARE(handle, LayouterDataHandle(0x123abcde));
    CORRADE_COMPARE(id, 0xabcde);
    CORRADE_COMPARE(generation, 0x123);
}

void HandleTest::layouterDataInvalid() {
    CORRADE_SKIP_IF_NO_DEBUG_ASSERT();

    /* Verify the zero generation check isn't off by a bit */
    layouterDataHandleId(layouterDataHandle(0, 1));
    layouterDataHandleId(layouterDataHandle(0, 1 << (Implementation::LayouterDataHandleGenerationBits - 1)));

    Containers::String out;
    Error redirectError{&out};
    layouterDataHandle(0x100000, 0x1);
    layouterDataHandle(0x1, 0x1000);
    layouterDataHandleId(LayouterDataHandle::Null);
    layouterDataHandleId(layouterDataHandle(0xabcde, 0));
    CORRADE_COMPARE_AS(out,
        "Ui::layouterDataHandle(): expected index to fit into 20 bits and generation into 12, got 0x100000 and 0x1\n"
        "Ui::layouterDataHandle(): expected index to fit into 20 bits and generation into 12, got 0x1 and 0x1000\n"
        "Ui::layouterDataHandleId(): invalid handle Ui::LayouterDataHandle::Null\n"
        "Ui::layouterDataHandleId(): invalid handle Ui::LayouterDataHandle(0xabcde, 0x0)\n",
        TestSuite::Compare::String);
}

void HandleTest::debugLayouterData() {
    Containers::String out;
    Debug{&out} << LayouterDataHandle::Null << layouterDataHandle(0x12345, 0xabc);
    CORRADE_COMPARE(out, "Ui::LayouterDataHandle::Null Ui::LayouterDataHandle(0x12345, 0xabc)\n");
}

void HandleTest::debugLayouterDataPacked() {
    Containers::String out;
    /* Last is not packed, ones before should not make any flags persistent */
    Debug{&out} << Debug::packed << LayouterDataHandle::Null << Debug::packed << layouterDataHandle(0x12345, 0xabc) << layouterDataHandle(0x67890, 0xdef);
    CORRADE_COMPARE(out, "Null {0x12345, 0xabc} Ui::LayouterDataHandle(0x67890, 0xdef)\n");
}

void HandleTest::layout() {
    CORRADE_COMPARE(LayoutHandle::Null, LayoutHandle{});
    CORRADE_COMPARE(layoutHandle(LayouterHandle::Null, 0, 0), LayoutHandle::Null);
    CORRADE_COMPARE(layoutHandle(LayouterHandle(0x12ab), 0x34567, 0xcde), LayoutHandle(0x12abcde34567));
    CORRADE_COMPARE(layoutHandle(LayouterHandle(0xffff), 0xfffff, 0xfff), LayoutHandle(0xffffffffffff));
    CORRADE_COMPARE(layoutHandle(LayouterHandle::Null, LayouterDataHandle::Null), LayoutHandle::Null);
    CORRADE_COMPARE(layoutHandle(LayouterHandle(0x12ab), LayouterDataHandle(0xcde34567)), LayoutHandle(0x12abcde34567));
    CORRADE_COMPARE(layoutHandleLayouter(LayoutHandle::Null), LayouterHandle::Null);
    CORRADE_COMPARE(layoutHandleLayouter(LayoutHandle(0x12abcde34567)), LayouterHandle(0x12ab));
    CORRADE_COMPARE(layoutHandleData(LayoutHandle::Null), LayouterDataHandle::Null);
    CORRADE_COMPARE(layoutHandleData(LayoutHandle(0x12abcde34567)), LayouterDataHandle(0xcde34567));
    CORRADE_COMPARE(layoutHandleLayouterId(LayoutHandle(0x12abcde34567)), 0xab);
    CORRADE_COMPARE(layoutHandleLayouterGeneration(LayoutHandle::Null), 0);
    CORRADE_COMPARE(layoutHandleLayouterGeneration(LayoutHandle(0x12abcde34567)), 0x12);
    CORRADE_COMPARE(layoutHandleId(LayoutHandle(0x12abcde34567)), 0x34567);
    CORRADE_COMPARE(layoutHandleGeneration(LayoutHandle::Null), 0);
    CORRADE_COMPARE(layoutHandleGeneration(LayoutHandle(0x12abcde34567)), 0xcde);

    constexpr LayoutHandle handle1 = layoutHandle(LayouterHandle(0x12ab), 0x34567, 0xcde);
    constexpr LayoutHandle handle2 = layoutHandle(LayouterHandle(0x12ab), LayouterDataHandle(0xcde34567));
    constexpr LayouterHandle layouter = layoutHandleLayouter(handle1);
    constexpr LayouterDataHandle data = layoutHandleData(handle1);
    constexpr UnsignedInt layouterId = layoutHandleLayouterId(handle1);
    constexpr UnsignedInt layouterGeneration = layoutHandleLayouterGeneration(handle1);
    constexpr UnsignedInt id = layoutHandleId(handle1);
    constexpr UnsignedInt generation = layoutHandleGeneration(handle1);
    CORRADE_COMPARE(handle1, LayoutHandle(0x12abcde34567));
    CORRADE_COMPARE(handle2, LayoutHandle(0x12abcde34567));
    CORRADE_COMPARE(layouter, LayouterHandle(0x12ab));
    CORRADE_COMPARE(data, LayouterDataHandle(0xcde34567));
    CORRADE_COMPARE(layouterId, 0xab);
    CORRADE_COMPARE(layouterGeneration, 0x12);
    CORRADE_COMPARE(id, 0x34567);
    CORRADE_COMPARE(generation, 0xcde);
}

void HandleTest::layoutInvalid() {
    CORRADE_SKIP_IF_NO_DEBUG_ASSERT();

    /* Verify the zero generation check isn't off by a bit. The other
       generation being zero shouldn't matter. */
    layoutHandleLayouterId(layoutHandle(layouterHandle(0, 1), LayouterDataHandle::Null));
    layoutHandleLayouterId(layoutHandle(layouterHandle(0, 1 << (Implementation::LayouterHandleGenerationBits - 1)), LayouterDataHandle::Null));
    layoutHandleId(layoutHandle(LayouterHandle::Null, 0, 1));
    layoutHandleId(layoutHandle(LayouterHandle::Null, 0, 1 << (Implementation::LayouterDataHandleGenerationBits - 1)));

    Containers::String out;
    Error redirectError{&out};
    layoutHandle(LayouterHandle::Null, 0x100000, 0x1);
    layoutHandle(LayouterHandle::Null, 0x1, 0x1000);
    layoutHandleLayouterId(LayoutHandle::Null);
    layoutHandleLayouterId(layoutHandle(LayouterHandle::Null, 0x1, 0x1));
    layoutHandleLayouterId(layoutHandle(layouterHandle(0xab, 0), 0x1, 0x1));
    layoutHandleId(LayoutHandle::Null);
    layoutHandleId(layoutHandle(layouterHandle(0x1, 0x1), LayouterDataHandle::Null));
    layoutHandleId(layoutHandle(layouterHandle(0x1, 0x1), layouterDataHandle(0xabcde, 0)));
    CORRADE_COMPARE_AS(out,
        "Ui::layoutHandle(): expected index to fit into 20 bits and generation into 12, got 0x100000 and 0x1\n"
        "Ui::layoutHandle(): expected index to fit into 20 bits and generation into 12, got 0x1 and 0x1000\n"
        "Ui::layoutHandleLayouterId(): invalid layouter portion of Ui::LayoutHandle::Null\n"
        "Ui::layoutHandleLayouterId(): invalid layouter portion of Ui::LayoutHandle(Null, {0x1, 0x1})\n"
        "Ui::layoutHandleLayouterId(): invalid layouter portion of Ui::LayoutHandle({0xab, 0x0}, {0x1, 0x1})\n"
        "Ui::layoutHandleId(): invalid data portion of Ui::LayoutHandle::Null\n"
        "Ui::layoutHandleId(): invalid data portion of Ui::LayoutHandle({0x1, 0x1}, Null)\n"
        "Ui::layoutHandleId(): invalid data portion of Ui::LayoutHandle({0x1, 0x1}, {0xabcde, 0x0})\n",
        TestSuite::Compare::String);
}

void HandleTest::debugLayout() {
    Containers::String out;
    Debug{&out} << LayoutHandle::Null << layoutHandle(LayouterHandle::Null, layouterDataHandle(0xabcde, 0x12)) << layoutHandle(layouterHandle(0x34, 0x56), LayouterDataHandle::Null) << layoutHandle(layouterHandle(0x34, 0x56), 0xabcde, 0x12);
    CORRADE_COMPARE(out, "Ui::LayoutHandle::Null Ui::LayoutHandle(Null, {0xabcde, 0x12}) Ui::LayoutHandle({0x34, 0x56}, Null) Ui::LayoutHandle({0x34, 0x56}, {0xabcde, 0x12})\n");
}

void HandleTest::debugLayoutPacked() {
    Containers::String out;
    /* Last is not packed, ones before should not make any flags persistent */
    Debug{&out} << Debug::packed << LayoutHandle::Null << Debug::packed << layoutHandle(LayouterHandle::Null, layouterDataHandle(0xabcde, 0x12)) << Debug::packed << layoutHandle(layouterHandle(0x34, 0x56), LayouterDataHandle::Null) << Debug::packed << layoutHandle(layouterHandle(0x34, 0x56), 0xabcde, 0x12) << layoutHandle(layouterHandle(0x78, 0x90), 0xf0123, 0xab);
    CORRADE_COMPARE(out, "Null {Null, {0xabcde, 0x12}} {{0x34, 0x56}, Null} {{0x34, 0x56}, {0xabcde, 0x12}} Ui::LayoutHandle({0x78, 0x90}, {0xf0123, 0xab})\n");
}

void HandleTest::animator() {
    CORRADE_COMPARE(AnimatorHandle::Null, AnimatorHandle{});
    CORRADE_COMPARE(animatorHandle(0, 0), AnimatorHandle{});
    CORRADE_COMPARE(animatorHandle(0xab, 0x12), AnimatorHandle(0x12ab));
    CORRADE_COMPARE(animatorHandle(0xff, 0xff), AnimatorHandle(0xffff));
    CORRADE_COMPARE(animatorHandleId(AnimatorHandle(0x12ab)), 0xab);
    CORRADE_COMPARE(animatorHandleGeneration(AnimatorHandle::Null), 0);
    CORRADE_COMPARE(animatorHandleGeneration(AnimatorHandle(0x12ab)), 0x12);

    constexpr AnimatorHandle handle = animatorHandle(0xab, 0x12);
    constexpr UnsignedInt id = animatorHandleId(handle);
    constexpr UnsignedInt generation = animatorHandleGeneration(handle);
    CORRADE_COMPARE(handle, AnimatorHandle(0x12ab));
    CORRADE_COMPARE(id, 0xab);
    CORRADE_COMPARE(generation, 0x12);
}

void HandleTest::animatorInvalid() {
    CORRADE_SKIP_IF_NO_DEBUG_ASSERT();

    /* Verify the zero generation check isn't off by a bit */
    animatorHandleId(animatorHandle(0, 1));
    animatorHandleId(animatorHandle(0, 1 << (Implementation::AnimatorHandleGenerationBits - 1)));

    Containers::String out;
    Error redirectError{&out};
    animatorHandle(0x100, 0x1);
    animatorHandle(0x1, 0x100);
    animatorHandleId(AnimatorHandle::Null);
    animatorHandleId(animatorHandle(0xab, 0));
    CORRADE_COMPARE_AS(out,
        "Ui::animatorHandle(): expected index to fit into 8 bits and generation into 8, got 0x100 and 0x1\n"
        "Ui::animatorHandle(): expected index to fit into 8 bits and generation into 8, got 0x1 and 0x100\n"
        "Ui::animatorHandleId(): invalid handle Ui::AnimatorHandle::Null\n"
        "Ui::animatorHandleId(): invalid handle Ui::AnimatorHandle(0xab, 0x0)\n",
        TestSuite::Compare::String);
}

void HandleTest::debugAnimator() {
    Containers::String out;
    Debug{&out} << AnimatorHandle::Null << animatorHandle(0x12, 0xab);
    CORRADE_COMPARE(out, "Ui::AnimatorHandle::Null Ui::AnimatorHandle(0x12, 0xab)\n");
}

void HandleTest::debugAnimatorPacked() {
    Containers::String out;
    /* Last is not packed, ones before should not make any flags persistent */
    Debug{&out} << Debug::packed << AnimatorHandle::Null << Debug::packed << animatorHandle(0x12, 0xab) << animatorHandle(0x34, 0xcd);
    CORRADE_COMPARE(out, "Null {0x12, 0xab} Ui::AnimatorHandle(0x34, 0xcd)\n");
}

void HandleTest::animatorData() {
    CORRADE_COMPARE(AnimatorDataHandle::Null, AnimatorDataHandle{});
    CORRADE_COMPARE(animatorDataHandle(0, 0), AnimatorDataHandle::Null);
    CORRADE_COMPARE(animatorDataHandle(0xabcde, 0x123), AnimatorDataHandle(0x123abcde));
    CORRADE_COMPARE(animatorDataHandle(0xfffff, 0xfff), AnimatorDataHandle(0xffffffff));
    CORRADE_COMPARE(animatorDataHandleId(AnimatorDataHandle(0x123abcde)), 0xabcde);
    CORRADE_COMPARE(animatorDataHandleGeneration(AnimatorDataHandle::Null), 0);
    CORRADE_COMPARE(animatorDataHandleGeneration(AnimatorDataHandle(0x123abcde)), 0x123);

    constexpr AnimatorDataHandle handle = animatorDataHandle(0xabcde, 0x123);
    constexpr UnsignedInt id = animatorDataHandleId(handle);
    constexpr UnsignedInt generation = animatorDataHandleGeneration(handle);
    CORRADE_COMPARE(handle, AnimatorDataHandle(0x123abcde));
    CORRADE_COMPARE(id, 0xabcde);
    CORRADE_COMPARE(generation, 0x123);
}

void HandleTest::animatorDataInvalid() {
    CORRADE_SKIP_IF_NO_DEBUG_ASSERT();

    /* Verify the zero generation check isn't off by a bit */
    animatorDataHandleId(animatorDataHandle(0, 1));
    animatorDataHandleId(animatorDataHandle(0, 1 << (Implementation::AnimatorDataHandleGenerationBits - 1)));

    Containers::String out;
    Error redirectError{&out};
    animatorDataHandle(0x100000, 0x1);
    animatorDataHandle(0x1, 0x1000);
    animatorDataHandleId(AnimatorDataHandle::Null);
    animatorDataHandleId(animatorDataHandle(0xabcde, 0));
    CORRADE_COMPARE_AS(out,
        "Ui::animatorDataHandle(): expected index to fit into 20 bits and generation into 12, got 0x100000 and 0x1\n"
        "Ui::animatorDataHandle(): expected index to fit into 20 bits and generation into 12, got 0x1 and 0x1000\n"
        "Ui::animatorDataHandleId(): invalid handle Ui::AnimatorDataHandle::Null\n"
        "Ui::animatorDataHandleId(): invalid handle Ui::AnimatorDataHandle(0xabcde, 0x0)\n",
        TestSuite::Compare::String);
}

void HandleTest::debugAnimatorData() {
    Containers::String out;
    Debug{&out} << AnimatorDataHandle::Null << animatorDataHandle(0x12345, 0xabc);
    CORRADE_COMPARE(out, "Ui::AnimatorDataHandle::Null Ui::AnimatorDataHandle(0x12345, 0xabc)\n");
}

void HandleTest::debugAnimatorDataPacked() {
    Containers::String out;
    /* Last is not packed, ones before should not make any flags persistent */
    Debug{&out} << Debug::packed << AnimatorDataHandle::Null << Debug::packed << animatorDataHandle(0x12345, 0xabc) << animatorDataHandle(0x67890, 0xdef);
    CORRADE_COMPARE(out, "Null {0x12345, 0xabc} Ui::AnimatorDataHandle(0x67890, 0xdef)\n");
}

void HandleTest::animation() {
    CORRADE_COMPARE(AnimationHandle::Null, AnimationHandle{});
    CORRADE_COMPARE(animationHandle(AnimatorHandle::Null, 0, 0), AnimationHandle::Null);
    CORRADE_COMPARE(animationHandle(AnimatorHandle(0x12ab), 0x34567, 0xcde), AnimationHandle(0x12abcde34567));
    CORRADE_COMPARE(animationHandle(AnimatorHandle(0xffff), 0xfffff, 0xfff), AnimationHandle(0xffffffffffff));
    CORRADE_COMPARE(animationHandle(AnimatorHandle::Null, AnimatorDataHandle::Null), AnimationHandle::Null);
    CORRADE_COMPARE(animationHandle(AnimatorHandle(0x12ab), AnimatorDataHandle(0xcde34567)), AnimationHandle(0x12abcde34567));
    CORRADE_COMPARE(animationHandleAnimator(AnimationHandle::Null), AnimatorHandle::Null);
    CORRADE_COMPARE(animationHandleAnimator(AnimationHandle(0x12abcde34567)), AnimatorHandle(0x12ab));
    CORRADE_COMPARE(animationHandleData(AnimationHandle::Null), AnimatorDataHandle::Null);
    CORRADE_COMPARE(animationHandleData(AnimationHandle(0x12abcde34567)), AnimatorDataHandle(0xcde34567));
    CORRADE_COMPARE(animationHandleAnimatorId(AnimationHandle(0x12abcde34567)), 0xab);
    CORRADE_COMPARE(animationHandleAnimatorGeneration(AnimationHandle::Null), 0);
    CORRADE_COMPARE(animationHandleAnimatorGeneration(AnimationHandle(0x12abcde34567)), 0x12);
    CORRADE_COMPARE(animationHandleId(AnimationHandle(0x12abcde34567)), 0x34567);
    CORRADE_COMPARE(animationHandleGeneration(AnimationHandle::Null), 0);
    CORRADE_COMPARE(animationHandleGeneration(AnimationHandle(0x12abcde34567)), 0xcde);

    constexpr AnimationHandle handle1 = animationHandle(AnimatorHandle(0x12ab), 0x34567, 0xcde);
    constexpr AnimationHandle handle2 = animationHandle(AnimatorHandle(0x12ab), AnimatorDataHandle(0xcde34567));
    constexpr AnimatorHandle animator = animationHandleAnimator(handle1);
    constexpr AnimatorDataHandle data = animationHandleData(handle1);
    constexpr UnsignedInt animatorId = animationHandleAnimatorId(handle1);
    constexpr UnsignedInt animatorGeneration = animationHandleAnimatorGeneration(handle1);
    constexpr UnsignedInt id = animationHandleId(handle1);
    constexpr UnsignedInt generation = animationHandleGeneration(handle1);
    CORRADE_COMPARE(handle1, AnimationHandle(0x12abcde34567));
    CORRADE_COMPARE(handle2, AnimationHandle(0x12abcde34567));
    CORRADE_COMPARE(animator, AnimatorHandle(0x12ab));
    CORRADE_COMPARE(data, AnimatorDataHandle(0xcde34567));
    CORRADE_COMPARE(animatorId, 0xab);
    CORRADE_COMPARE(animatorGeneration, 0x12);
    CORRADE_COMPARE(id, 0x34567);
    CORRADE_COMPARE(generation, 0xcde);
}

void HandleTest::animationInvalid() {
    CORRADE_SKIP_IF_NO_DEBUG_ASSERT();

    /* Verify the zero generation check isn't off by a bit. The other
       generation being zero shouldn't matter. */
    animationHandleAnimatorId(animationHandle(animatorHandle(0, 1), AnimatorDataHandle::Null));
    animationHandleAnimatorId(animationHandle(animatorHandle(0, 1 << (Implementation::AnimatorHandleGenerationBits - 1)), AnimatorDataHandle::Null));
    animationHandleId(animationHandle(AnimatorHandle::Null, 0, 1));
    animationHandleId(animationHandle(AnimatorHandle::Null, 0, 1 << (Implementation::AnimatorDataHandleGenerationBits - 1)));

    Containers::String out;
    Error redirectError{&out};
    animationHandle(AnimatorHandle::Null, 0x100000, 0x1);
    animationHandle(AnimatorHandle::Null, 0x1, 0x1000);
    animationHandleAnimatorId(AnimationHandle::Null);
    animationHandleAnimatorId(animationHandle(AnimatorHandle::Null, 0x1, 0x1));
    animationHandleAnimatorId(animationHandle(animatorHandle(0xab, 0), 0x1, 0x1));
    animationHandleId(AnimationHandle::Null);
    animationHandleId(animationHandle(animatorHandle(0x1, 0x1), AnimatorDataHandle::Null));
    animationHandleId(animationHandle(animatorHandle(0x1, 0x1), animatorDataHandle(0xabcde, 0)));
    CORRADE_COMPARE_AS(out,
        "Ui::animationHandle(): expected index to fit into 20 bits and generation into 12, got 0x100000 and 0x1\n"
        "Ui::animationHandle(): expected index to fit into 20 bits and generation into 12, got 0x1 and 0x1000\n"
        "Ui::animationHandleAnimatorId(): invalid animator portion of Ui::AnimationHandle::Null\n"
        "Ui::animationHandleAnimatorId(): invalid animator portion of Ui::AnimationHandle(Null, {0x1, 0x1})\n"
        "Ui::animationHandleAnimatorId(): invalid animator portion of Ui::AnimationHandle({0xab, 0x0}, {0x1, 0x1})\n"
        "Ui::animationHandleId(): invalid data portion of Ui::AnimationHandle::Null\n"
        "Ui::animationHandleId(): invalid data portion of Ui::AnimationHandle({0x1, 0x1}, Null)\n"
        "Ui::animationHandleId(): invalid data portion of Ui::AnimationHandle({0x1, 0x1}, {0xabcde, 0x0})\n",
        TestSuite::Compare::String);
}

void HandleTest::debugAnimation() {
    Containers::String out;
    Debug{&out} << AnimationHandle::Null << animationHandle(AnimatorHandle::Null, animatorDataHandle(0xabcde, 0x12)) << animationHandle(animatorHandle(0x34, 0x56), AnimatorDataHandle::Null) << animationHandle(animatorHandle(0x34, 0x56), 0xabcde, 0x12);
    CORRADE_COMPARE(out, "Ui::AnimationHandle::Null Ui::AnimationHandle(Null, {0xabcde, 0x12}) Ui::AnimationHandle({0x34, 0x56}, Null) Ui::AnimationHandle({0x34, 0x56}, {0xabcde, 0x12})\n");
}

void HandleTest::debugAnimationPacked() {
    Containers::String out;
    /* Last is not packed, ones before should not make any flags persistent */
    Debug{&out} << Debug::packed << AnimationHandle::Null << Debug::packed << animationHandle(AnimatorHandle::Null, animatorDataHandle(0xabcde, 0x12)) << Debug::packed << animationHandle(animatorHandle(0x34, 0x56), AnimatorDataHandle::Null) << Debug::packed << animationHandle(animatorHandle(0x34, 0x56), 0xabcde, 0x12) << animationHandle(animatorHandle(0x78, 0x90), 0xf0123, 0xab);
    CORRADE_COMPARE(out, "Null {Null, {0xabcde, 0x12}} {{0x34, 0x56}, Null} {{0x34, 0x56}, {0xabcde, 0x12}} Ui::AnimationHandle({0x78, 0x90}, {0xf0123, 0xab})\n");
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::HandleTest)
