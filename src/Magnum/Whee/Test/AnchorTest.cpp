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

#include <sstream>
#include <Corrade/Containers/BitArrayView.h>
#include <Corrade/Containers/StringStl.h> /** @todo remove once Debug is stream-free */
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/String.h>
#include <Corrade/Utility/DebugStl.h> /** @todo remove once Debug is stream-free */
#include <Magnum/Math/Vector2.h>

#include "Magnum/Whee/AbstractLayouter.h"
#include "Magnum/Whee/Anchor.h"
#include "Magnum/Whee/Handle.h"
#include "Magnum/Whee/NodeFlags.h"
#include "Magnum/Whee/UserInterface.h"

namespace Magnum { namespace Whee { namespace Test { namespace {

struct AnchorTest: TestSuite::Tester {
    explicit AnchorTest();

    void construct();
    void constructInvalid();
    void constructCreateNode();
    void constructCreateNodeTopLevel();
    /* Passing an invalid parent to the node creation is asserted in
       UserInterface directly */

    void layoutInvalid();
};

AnchorTest::AnchorTest() {
    addTests({&AnchorTest::construct,
              &AnchorTest::constructInvalid,
              &AnchorTest::constructCreateNode,
              &AnchorTest::constructCreateNodeTopLevel,

              &AnchorTest::layoutInvalid});
}

void AnchorTest::construct() {
    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};

    struct Layouter: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;
        using AbstractLayouter::add;

        void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<Vector2>&, const  Containers::StridedArrayView1D<Vector2>&) override {}
    };
    Layouter& layouter = ui.setLayouterInstance(Containers::pointer<Layouter>(ui.createLayouter()));

    NodeHandle node = ui.createNode({}, {});
    LayoutHandle layout = layouter.add(node);

    Anchor a{ui, node, layout};
    CORRADE_COMPARE(&a.ui(), &ui);
    CORRADE_COMPARE(a.node(), node);
    CORRADE_COMPARE(a, node);
    CORRADE_COMPARE(a.layout(), layout);
    CORRADE_COMPARE(a, layout);

    Anchor b{ui, node, LayoutHandle::Null};
    CORRADE_COMPARE(&b.ui(), &ui);
    CORRADE_COMPARE(b.node(), node);
    CORRADE_COMPARE(b, node);
    CORRADE_COMPARE(b.layout(), LayoutHandle::Null);
    /* LayoutHandle conversion would assert here */
}

void AnchorTest::constructInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};

    struct Layouter: AbstractLayouter {
        using AbstractLayouter::AbstractLayouter;
        using AbstractLayouter::add;

        void doUpdate(Containers::BitArrayView, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<Vector2>&, const  Containers::StridedArrayView1D<Vector2>&) override {}
    };
    Layouter& layouter = ui.setLayouterInstance(Containers::pointer<Layouter>(ui.createLayouter()));

    NodeHandle node = ui.createNode({}, {});
    NodeHandle node2 = ui.createNode({}, {});
    LayoutHandle layout = layouter.add(node);
    LayoutHandle layout2 = layouter.add(node2);

    std::ostringstream out;
    Error redirectError{&out};
    Anchor{ui, nodeHandle(0x12345, 0xabc), layout};
    Anchor{ui, node, layoutHandle(layouterHandle(0x67, 0xde), 0x12345, 0xabc)};
    Anchor{ui, node, layout2};
    CORRADE_COMPARE_AS(out.str(),
        "Whee::Anchor: invalid handle Whee::NodeHandle(0x12345, 0xabc)\n"
        "Whee::Anchor: invalid handle Whee::LayoutHandle({0x67, 0xde}, {0x12345, 0xabc})\n"
        "Whee::Anchor: Whee::LayoutHandle({0x0, 0x1}, {0x1, 0x1}) not associated with Whee::NodeHandle(0x0, 0x1)\n",
        TestSuite::Compare::String);
}

void AnchorTest::constructCreateNode() {
    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};

    NodeHandle parent = ui.createNode({}, {});

    Anchor a{ui, parent, {1.0f, 2.0f}, {3.0f, 4.0f}, NodeFlag::Disabled};
    CORRADE_COMPARE(&a.ui(), &ui);
    CORRADE_COMPARE(a.layout(), LayoutHandle::Null);
    CORRADE_COMPARE(ui.nodeParent(a), parent);
    CORRADE_COMPARE(ui.nodeOffset(a), (Vector2{1.0f, 2.0f}));
    CORRADE_COMPARE(ui.nodeSize(a), (Vector2{3.0f, 4.0f}));
    CORRADE_COMPARE(ui.nodeFlags(a), NodeFlag::Disabled);

    Anchor b{ui, parent, {5.0f, 6.0f}, NodeFlag::NoEvents};
    CORRADE_COMPARE(&b.ui(), &ui);
    CORRADE_COMPARE(b.layout(), LayoutHandle::Null);
    CORRADE_COMPARE(ui.nodeParent(b), parent);
    CORRADE_COMPARE(ui.nodeOffset(b), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(b), (Vector2{5.0f, 6.0f}));
    CORRADE_COMPARE(ui.nodeFlags(b), NodeFlag::NoEvents);
}

void AnchorTest::constructCreateNodeTopLevel() {
    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};

    Anchor a{ui, {1.0f, 2.0f}, {3.0f, 4.0f}, NodeFlag::Disabled};
    CORRADE_COMPARE(&a.ui(), &ui);
    CORRADE_COMPARE(a.layout(), LayoutHandle::Null);
    CORRADE_COMPARE(ui.nodeParent(a), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOffset(a), (Vector2{1.0f, 2.0f}));
    CORRADE_COMPARE(ui.nodeSize(a), (Vector2{3.0f, 4.0f}));
    CORRADE_COMPARE(ui.nodeFlags(a), NodeFlag::Disabled);

    Anchor b{ui, {5.0f, 6.0f}, NodeFlag::NoEvents};
    CORRADE_COMPARE(&b.ui(), &ui);
    CORRADE_COMPARE(b.layout(), LayoutHandle::Null);
    CORRADE_COMPARE(ui.nodeParent(b), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOffset(b), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(b), (Vector2{5.0f, 6.0f}));
    CORRADE_COMPARE(ui.nodeFlags(b), NodeFlag::NoEvents);
}

void AnchorTest::layoutInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};

    NodeHandle node = ui.createNode({}, {});
    Anchor a{ui, node, LayoutHandle::Null};

    std::ostringstream out;
    Error redirectError{&out};
    /* LOL, LayoutHandle(a); says it's redefining a with a different type?!
       What's up with that syntax?? Now I have to add another void cast because
       otherwise it says expression result is unused. */
    static_cast<void>(static_cast<LayoutHandle>(a));
    CORRADE_COMPARE(out.str(), "Whee::Anchor: layout is null\n");
}

}}}}

CORRADE_TEST_MAIN(Magnum::Whee::Test::AnchorTest)
