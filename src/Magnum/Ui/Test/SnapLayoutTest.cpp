/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023, 2024, 2025, 2026
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

#include <Corrade/Containers/ArrayView.h> /* arraySize() */
#include <Corrade/Containers/String.h>
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/String.h>

#include "Magnum/Ui/Anchor.h"
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/NodeFlags.h"
#include "Magnum/Ui/SnapLayout.h"
#include "Magnum/Ui/SnapLayouter.h"
#include "Magnum/Ui/UserInterface.h"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct SnapLayoutTest: TestSuite::Tester {
    explicit SnapLayoutTest();

    template<class T> void constructFromAnchor();
    template<class T> void constructFromNode();
    void constructImplicitLayouterFromAnchor();
    void constructImplicitLayouterFromNode();
    template<class T> void constructFromLayout();
    void constructImplicitLayouterFromLayout();
    template<class T> void constructInvalid();
    void constructInvalidImplicitLayouter();

    template<class T> void convertAnchor();
    void convertSpecialized();

    template<class T> void flags();
    template<class T> void childSnap();

    template<class T> void child();
    template<class T> void childExplicitSnap();
    template<class T> void siblingExplicitSnap();

    template<class T> void staticRootExplicitSnap();
    void staticRootExplicitSnapImplicitLayouter();
    template<class T> void staticInvalid();
    void staticInvalidImplicitLayouter();
};

const struct {
    const char* name;
    bool layoutExists;
} ConstructData[]{
    {"", false},
    {"layout already exists", true},
};

const struct {
    const char* name;
    bool fromLayout;
} ConvertSpecializedData[]{
    {"from an Anchor", false},
    {"from a SnapLayout", true}
};

SnapLayoutTest::SnapLayoutTest() {
    addInstancedTests({&SnapLayoutTest::constructFromAnchor<AbstractSnapLayout>,
                       &SnapLayoutTest::constructFromAnchor<SnapLayout>,
                       &SnapLayoutTest::constructFromNode<AbstractSnapLayout>,
                       &SnapLayoutTest::constructFromNode<SnapLayout>,
                       &SnapLayoutTest::constructImplicitLayouterFromAnchor,
                       &SnapLayoutTest::constructImplicitLayouterFromNode},
        Containers::arraySize(ConstructData));

    addTests({&SnapLayoutTest::constructFromLayout<AbstractSnapLayout>,
              &SnapLayoutTest::constructFromLayout<SnapLayout>,
              &SnapLayoutTest::constructImplicitLayouterFromLayout,
              &SnapLayoutTest::constructInvalid<AbstractSnapLayout>,
              &SnapLayoutTest::constructInvalid<SnapLayout>,
              &SnapLayoutTest::constructInvalidImplicitLayouter,

              &SnapLayoutTest::convertAnchor<AbstractSnapLayout>,
              &SnapLayoutTest::convertAnchor<SnapLayout>});

    addInstancedTests({&SnapLayoutTest::convertSpecialized},
        Containers::arraySize(ConvertSpecializedData));

    addTests({&SnapLayoutTest::flags<AbstractSnapLayout>,
              &SnapLayoutTest::flags<SnapLayout>,
              &SnapLayoutTest::childSnap<AbstractSnapLayout>,
              &SnapLayoutTest::childSnap<SnapLayout>,

              &SnapLayoutTest::child<AbstractSnapLayout>,
              &SnapLayoutTest::child<SnapLayout>,
              &SnapLayoutTest::childExplicitSnap<AbstractSnapLayout>,
              &SnapLayoutTest::childExplicitSnap<SnapLayout>,
              &SnapLayoutTest::siblingExplicitSnap<AbstractSnapLayout>,
              &SnapLayoutTest::siblingExplicitSnap<SnapLayout>,

              &SnapLayoutTest::staticRootExplicitSnap<AbstractSnapLayout>,
              &SnapLayoutTest::staticRootExplicitSnap<SnapLayout>,
              &SnapLayoutTest::staticRootExplicitSnapImplicitLayouter,
              &SnapLayoutTest::staticInvalid<AbstractSnapLayout>,
              &SnapLayoutTest::staticInvalid<SnapLayout>,
              &SnapLayoutTest::staticInvalidImplicitLayouter});
}

template<class> struct SnapLayoutTraits;
template<> struct SnapLayoutTraits<AbstractSnapLayout> {
    typedef AbstractUserInterface UserInterfaceType;
    typedef AbstractAnchor AnchorType;
    static const char* name() { return "AbstractSnapLayout"; }
};
template<> struct SnapLayoutTraits<SnapLayout> {
    typedef UserInterface UserInterfaceType;
    typedef Anchor AnchorType;
    static const char* name() { return "SnapLayout"; }
};

template<class T> void SnapLayoutTest::constructFromAnchor() {
    auto&& data = ConstructData[testCaseInstanceId()];
    setTestCaseTemplateName(SnapLayoutTraits<T>::name());
    setTestCaseDescription(data.name);

    struct Interface: SnapLayoutTraits<T>::UserInterfaceType {
        explicit Interface(NoCreateT): SnapLayoutTraits<T>::UserInterfaceType{NoCreate} {}
    } ui{NoCreate};

    SnapLayouter& layouter = ui.setLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));

    /* Create some extra nodes and layouts to not have the anchor and layout
       with trivial handles */
    layouter.add(ui.createNode({}, {}));
    layouter.add(ui.createNode({}, {}));
    layouter.add(ui.createNode({}, {}));
    layouter.remove(layouter.add(ui.createNode({}, {})));
    ui.createNode({}, {});
    ui.removeNode(ui.createNode({}, {}));
    ui.removeNode(ui.createNode({}, {}));

    typename SnapLayoutTraits<T>::AnchorType anchor{ui, {}, {}};
    CORRADE_COMPARE(anchor.node(), nodeHandle(5, 3));

    if(data.layoutExists)
        layouter.add(anchor);

    T layout{layouter, anchor};
    CORRADE_COMPARE(&layout.ui(), &ui);
    CORRADE_COMPARE(&layout.layouter(), &layouter);
    CORRADE_COMPARE(layout.node(), anchor.node());
    CORRADE_COMPARE(layout, anchor.node());
    CORRADE_COMPARE(layout.layout(), layoutHandle(layouter.handle(), 3, 2));
    CORRADE_COMPARE(layout, layout.layout());
    CORRADE_VERIFY(layouter.isHandleValid(layout));
    CORRADE_VERIFY(!layouter.hasExplicitSnap(layout));
    CORRADE_COMPARE(layouter.node(layout), anchor.node());
    CORRADE_COMPARE(layouter.flags(layout), SnapLayoutFlags{});
    CORRADE_COMPARE(layouter.childSnap(layout), Snap::Bottom);

    /* There should be no additional layouts created besides the ones known
       above */
    CORRADE_COMPARE(layouter.usedCount(), 4);
}

template<class T> void SnapLayoutTest::constructFromNode() {
    auto&& data = ConstructData[testCaseInstanceId()];
    setTestCaseTemplateName(SnapLayoutTraits<T>::name());
    setTestCaseDescription(data.name);

    struct Interface: SnapLayoutTraits<T>::UserInterfaceType {
        explicit Interface(NoCreateT): SnapLayoutTraits<T>::UserInterfaceType{NoCreate} {}
    } ui{NoCreate};

    SnapLayouter& layouter = ui.setLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));

    /* Create some extra nodes and layouts to not have the anchor and layout
       with trivial handles */
    layouter.add(ui.createNode({}, {}));
    layouter.add(ui.createNode({}, {}));
    layouter.add(ui.createNode({}, {}));
    layouter.remove(layouter.add(ui.createNode({}, {})));
    ui.createNode({}, {});
    ui.removeNode(ui.createNode({}, {}));
    ui.removeNode(ui.createNode({}, {}));

    NodeHandle node = ui.createNode({}, {});
    CORRADE_COMPARE(node, nodeHandle(5, 3));

    if(data.layoutExists)
        layouter.add(node);

    T layout{ui, layouter, node};
    CORRADE_COMPARE(&layout.ui(), &ui);
    CORRADE_COMPARE(&layout.layouter(), &layouter);
    CORRADE_COMPARE(layout.node(), node);
    CORRADE_COMPARE(layout, node);
    CORRADE_COMPARE(layout.layout(), layoutHandle(layouter.handle(), 3, 2));
    CORRADE_COMPARE(layout, layout.layout());
    CORRADE_VERIFY(layouter.isHandleValid(layout));
    CORRADE_VERIFY(!layouter.hasExplicitSnap(layout));
    CORRADE_COMPARE(layouter.node(layout), node);
    CORRADE_COMPARE(layouter.flags(layout), SnapLayoutFlags{});
    CORRADE_COMPARE(layouter.childSnap(layout), Snap::Bottom);

    /* There should be no additional layouts created besides the ones known
       above */
    CORRADE_COMPARE(layouter.usedCount(), 4);
}

void SnapLayoutTest::constructImplicitLayouterFromAnchor() {
    auto&& data = ConstructData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};

    ui.setSnapLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));

    /* Create some extra nodes and layouts to not have the anchor and layout
       with trivial handles */
    ui.snapLayouter().add(ui.createNode({}, {}));
    ui.snapLayouter().add(ui.createNode({}, {}));
    ui.snapLayouter().add(ui.createNode({}, {}));
    ui.snapLayouter().remove(ui.snapLayouter().add(ui.createNode({}, {})));
    ui.createNode({}, {});
    ui.removeNode(ui.createNode({}, {}));
    ui.removeNode(ui.createNode({}, {}));

    Anchor anchor{ui, {}, {}};
    CORRADE_COMPARE(anchor.node(), nodeHandle(5, 3));

    if(data.layoutExists)
        ui.snapLayouter().add(anchor);

    SnapLayout layout{anchor};
    CORRADE_COMPARE(&layout.ui(), &ui);
    CORRADE_COMPARE(&layout.layouter(), &ui.snapLayouter());
    CORRADE_COMPARE(layout.node(), anchor.node());
    CORRADE_COMPARE(layout, anchor.node());
    CORRADE_COMPARE(layout.layout(), layoutHandle(ui.snapLayouter().handle(), 3, 2));
    CORRADE_COMPARE(layout, layout.layout());
    CORRADE_VERIFY(ui.snapLayouter().isHandleValid(layout));
    CORRADE_VERIFY(!ui.snapLayouter().hasExplicitSnap(layout));
    CORRADE_COMPARE(ui.snapLayouter().node(layout), anchor.node());
    CORRADE_COMPARE(ui.snapLayouter().flags(layout), SnapLayoutFlags{});
    CORRADE_COMPARE(ui.snapLayouter().childSnap(layout), Snap::Bottom);

    /* There should be no additional layouts created besides the ones known
       above */
    CORRADE_COMPARE(ui.snapLayouter().usedCount(), 4);
}

void SnapLayoutTest::constructImplicitLayouterFromNode() {
    auto&& data = ConstructData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};

    ui.setSnapLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));

    /* Create some extra nodes and layouts to not have the anchor and layout
       with trivial handles */
    ui.snapLayouter().add(ui.createNode({}, {}));
    ui.snapLayouter().add(ui.createNode({}, {}));
    ui.snapLayouter().add(ui.createNode({}, {}));
    ui.snapLayouter().remove(ui.snapLayouter().add(ui.createNode({}, {})));
    ui.createNode({}, {});
    ui.removeNode(ui.createNode({}, {}));
    ui.removeNode(ui.createNode({}, {}));

    NodeHandle node = ui.createNode({}, {});
    CORRADE_COMPARE(node, nodeHandle(5, 3));

    if(data.layoutExists)
        ui.snapLayouter().add(node);

    SnapLayout layout{ui, node};
    CORRADE_COMPARE(&layout.ui(), &ui);
    CORRADE_COMPARE(&layout.layouter(), &ui.snapLayouter());
    CORRADE_COMPARE(layout.node(), node);
    CORRADE_COMPARE(layout, node);
    CORRADE_COMPARE(layout.layout(), layoutHandle(ui.snapLayouter().handle(), 3, 2));
    CORRADE_COMPARE(layout, layout.layout());
    CORRADE_VERIFY(ui.snapLayouter().isHandleValid(layout));
    CORRADE_VERIFY(!ui.snapLayouter().hasExplicitSnap(layout));
    CORRADE_COMPARE(ui.snapLayouter().node(layout), node);
    CORRADE_COMPARE(ui.snapLayouter().flags(layout), SnapLayoutFlags{});
    CORRADE_COMPARE(ui.snapLayouter().childSnap(layout), Snap::Bottom);

    /* There should be no additional layouts created besides the ones known
       above */
    CORRADE_COMPARE(ui.snapLayouter().usedCount(), 4);
}

template<class T> void SnapLayoutTest::constructFromLayout() {
    setTestCaseTemplateName(SnapLayoutTraits<T>::name());

    struct Interface: SnapLayoutTraits<T>::UserInterfaceType {
        explicit Interface(NoCreateT): SnapLayoutTraits<T>::UserInterfaceType{NoCreate} {}
    } ui{NoCreate};

    SnapLayouter& layouter = ui.setLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));

    /* Create some extra nodes and layouts to not have the anchor and layout
       with trivial handles */
    layouter.add(ui.createNode({}, {}));
    layouter.add(ui.createNode({}, {}));
    layouter.add(ui.createNode({}, {}));
    layouter.remove(layouter.add(ui.createNode({}, {})));
    ui.createNode({}, {});
    ui.removeNode(ui.createNode({}, {}));
    ui.removeNode(ui.createNode({}, {}));

    NodeHandle node = ui.createNode({}, {});
    LayoutHandle layout = layouter.add(node);
    CORRADE_COMPARE(node, nodeHandle(5, 3));
    CORRADE_COMPARE(layout, layoutHandle(layouter.handle(), 3, 2));

    T layout1{ui, layouter, layout};
    T layout2{ui, layouter, layoutHandleData(layout)};
    CORRADE_COMPARE(&layout1.ui(), &ui);
    CORRADE_COMPARE(&layout2.ui(), &ui);
    CORRADE_COMPARE(&layout1.layouter(), &layouter);
    CORRADE_COMPARE(&layout2.layouter(), &layouter);
    CORRADE_COMPARE(layout1.node(), node);
    CORRADE_COMPARE(layout2.node(), node);
    CORRADE_COMPARE(layout1, node);
    CORRADE_COMPARE(layout2, node);
    CORRADE_COMPARE(layout1.layout(), layout);
    CORRADE_COMPARE(layout2.layout(), layout);
    CORRADE_COMPARE(layout1, layout);
    CORRADE_COMPARE(layout2, layout);

    /* There should be no additional layouts created besides the ones known
       above */
    CORRADE_COMPARE(layouter.usedCount(), 4);
}

void SnapLayoutTest::constructImplicitLayouterFromLayout() {
    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};

    ui.setSnapLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));

    /* Create some extra nodes and layouts to not have the anchor and layout
       with trivial handles */
    ui.snapLayouter().add(ui.createNode({}, {}));
    ui.snapLayouter().add(ui.createNode({}, {}));
    ui.snapLayouter().add(ui.createNode({}, {}));
    ui.snapLayouter().remove(ui.snapLayouter().add(ui.createNode({}, {})));
    ui.createNode({}, {});
    ui.removeNode(ui.createNode({}, {}));
    ui.removeNode(ui.createNode({}, {}));

    NodeHandle node = ui.createNode({}, {});
    LayoutHandle layout = ui.snapLayouter().add(node);
    CORRADE_COMPARE(node, nodeHandle(5, 3));
    CORRADE_COMPARE(layout, layoutHandle(ui.snapLayouter().handle(), 3, 2));

    SnapLayout layout1{ui, layout};
    SnapLayout layout2{ui, layoutHandleData(layout)};
    CORRADE_COMPARE(&layout1.ui(), &ui);
    CORRADE_COMPARE(&layout2.ui(), &ui);
    CORRADE_COMPARE(&layout1.layouter(), &ui.snapLayouter());
    CORRADE_COMPARE(&layout2.layouter(), &ui.snapLayouter());
    CORRADE_COMPARE(layout1.node(), node);
    CORRADE_COMPARE(layout2.node(), node);
    CORRADE_COMPARE(layout1, node);
    CORRADE_COMPARE(layout2, node);
    CORRADE_COMPARE(layout1.layout(), layout);
    CORRADE_COMPARE(layout2.layout(), layout);
    CORRADE_COMPARE(layout1, layout);
    CORRADE_COMPARE(layout2, layout);

    /* There should be no additional layouts created besides the ones known
       above */
    CORRADE_COMPARE(ui.snapLayouter().usedCount(), 4);
}

template<class T> void SnapLayoutTest::constructInvalid() {
    setTestCaseTemplateName(SnapLayoutTraits<T>::name());

    CORRADE_SKIP_IF_NO_ASSERT();

    struct Interface: SnapLayoutTraits<T>::UserInterfaceType {
        explicit Interface(NoCreateT): SnapLayoutTraits<T>::UserInterfaceType{NoCreate} {}
    } ui{NoCreate},
      anotherUi{NoCreate};

    /* Create some extra layouters to not have it with a trivial handle. Same
       has to be done for the other UI instance to have the handles match
       below. */
    ui.createLayouter();
    ui.createLayouter();
    ui.removeLayouter(ui.createLayouter());
    ui.removeLayouter(ui.createLayouter());
    anotherUi.createLayouter();
    anotherUi.createLayouter();
    anotherUi.removeLayouter(anotherUi.createLayouter());
    anotherUi.removeLayouter(anotherUi.createLayouter());

    SnapLayouter& layouter = ui.setLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));
    SnapLayouter& anotherLayouter = ui.setLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));
    /* Layouter with a handle that isn't even valid in the UI, to verify it
       isn't blindly accessng it */
    SnapLayouter layouterInvalidHandle{layouterHandle(0xab, 0x12)};
    /* Layouter with the same handle as the right one, but in another UI, to
       verify it's checking the actual instance as well */
    SnapLayouter& layouterAnotherUi = anotherUi.setLayouterInstance(Containers::pointer<SnapLayouter>(anotherUi.createLayouter()));
    CORRADE_COMPARE(layouterAnotherUi.handle(), layouter.handle());

    /* Create some extra nodes and layouts to not have the anchor and layout
       with trivial handles. Again has to be done for the other layouter
       instance to have the handles match below. */
    layouter.add(ui.createNode({}, {}));
    layouter.remove(layouter.add(ui.createNode({}, {})));
    anotherLayouter.add(ui.createNode({}, {}));
    anotherLayouter.remove(anotherLayouter.add(ui.createNode({}, {})));

    /* Layout with the same handle as the right one, but in another layouter,
       to verify it isn't comparing just the LayouterDataHandle part */
    LayoutHandle layout = layouter.add(ui.createNode({}, {}));
    LayoutHandle anotherLayout = anotherLayouter.add(ui.createNode({}, {}));
    CORRADE_COMPARE(layoutHandleData(anotherLayout), layoutHandleData(layout));

    typename SnapLayoutTraits<T>::AnchorType anchor{ui, {}, {}};
    typename SnapLayoutTraits<T>::AnchorType anchorRemoved{ui, {}, {}};
    ui.removeNode(anchorRemoved);

    Containers::String out;
    Error redirectError{&out};
    /* Layouter handle not valid in the anchor UI / node UI */
    T{layouterInvalidHandle, anchor};
    T{ui, layouterInvalidHandle, anchor.node()};
    /* Valid handle, but different instance */
    T{layouterAnotherUi, anchor};
    T{ui, layouterAnotherUi, anchor.node()};
    /* Invalid anchor / node handle. Can't form a completely bogus anchor as
       its constructor already checks for handle validity, so it's a removed
       handle instead. */
    T{layouter, anchorRemoved};
    T{ui, layouter, nodeHandle(0x12345, 0xabc)};
    /* Layout not part of the layouter */
    T{ui, layouter, anotherLayout};
    /* Valid layouter, invalid data handle */
    T{ui, layouter, layoutHandle(layouter.handle(), 0x12345, 0xabc)};
    T{ui, layouter, layouterDataHandle(0x12345, 0xabc)};
    CORRADE_COMPARE_AS(out,
        "Ui::AbstractSnapLayout: layouter and anchor not part of the same UI\n"
        "Ui::AbstractSnapLayout: layouter not part of the UI\n"

        "Ui::AbstractSnapLayout: layouter and anchor not part of the same UI\n"
        "Ui::AbstractSnapLayout: layouter not part of the UI\n"

        "Ui::AbstractSnapLayout: invalid handle Ui::NodeHandle(0x7, 0x1)\n"
        "Ui::AbstractSnapLayout: invalid handle Ui::NodeHandle(0x12345, 0xabc)\n"

        "Ui::AbstractSnapLayout: Ui::LayoutHandle({0x3, 0x1}, {0x1, 0x2}) not coming from Ui::LayouterHandle(0x2, 0x3)\n"
        "Ui::AbstractSnapLayout: invalid handle Ui::LayoutHandle({0x2, 0x3}, {0x12345, 0xabc})\n"
        "Ui::AbstractSnapLayout: invalid handle Ui::LayouterDataHandle(0x12345, 0xabc)\n",
        TestSuite::Compare::String);
}

void SnapLayoutTest::constructInvalidImplicitLayouter() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } uiNoSnapLayouter{NoCreate};

    Anchor anchorUiNoSnapLayouter{uiNoSnapLayouter, {}, {}};

    Containers::String out;
    Error redirectError{&out};
    /* Layouter not present; with a node, with a layout */
    SnapLayout{anchorUiNoSnapLayouter};
    SnapLayout{uiNoSnapLayouter, anchorUiNoSnapLayouter.node()};
    SnapLayout{uiNoSnapLayouter, LayoutHandle::Null};
    SnapLayout{uiNoSnapLayouter, LayouterDataHandle::Null};
    CORRADE_COMPARE_AS(out,
        "Ui::BasicSnapLayout: SnapLayouter not present in the UI\n"
        "Ui::BasicSnapLayout: SnapLayouter not present in the UI\n"
        "Ui::BasicSnapLayout: SnapLayouter not present in the UI\n"
        "Ui::BasicSnapLayout: SnapLayouter not present in the UI\n",
        TestSuite::Compare::String);
}

template<class T> void SnapLayoutTest::convertAnchor() {
    setTestCaseTemplateName(SnapLayoutTraits<T>::name());

    struct Interface: SnapLayoutTraits<T>::UserInterfaceType {
        explicit Interface(NoCreateT): SnapLayoutTraits<T>::UserInterfaceType{NoCreate} {}
    } ui{NoCreate};

    SnapLayouter& layouter = ui.setLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));

    /* Create some extra nodes and layouts to not have the anchor and layout
       with trivial handles */
    layouter.add(ui.createNode({}, {}));
    layouter.add(ui.createNode({}, {}));
    layouter.add(ui.createNode({}, {}));
    layouter.remove(layouter.add(ui.createNode({}, {})));
    ui.createNode({}, {});
    ui.removeNode(ui.createNode({}, {}));
    ui.removeNode(ui.createNode({}, {}));

    typename SnapLayoutTraits<T>::AnchorType anchor{ui, {}, {}};
    CORRADE_COMPARE(anchor.node(), nodeHandle(5, 3));

    T layout{layouter, anchor};
    CORRADE_COMPARE(layout.node(), anchor.node());
    CORRADE_COMPARE(layout.layout(), layoutHandle(layouter.handle(), 3, 2));

    typename SnapLayoutTraits<T>::AnchorType converted = layout;
    CORRADE_COMPARE(&converted.ui(), &ui);
    CORRADE_COMPARE(converted.node(), anchor.node());
}

void SnapLayoutTest::convertSpecialized() {
    auto&& data = ConvertSpecializedData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};

    ui.setSnapLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));

    Anchor anchors[]{
        Anchor{ui, {}, Vector2{}},
        Anchor{ui, {}, Vector2{}},
        Anchor{ui, {}, Vector2{}},
        Anchor{ui, {}, Vector2{}},
        Anchor{ui, {}, Vector2{}},
        Anchor{ui, {}, Vector2{}},
        Anchor{ui, {}, Vector2{}},
        Anchor{ui, {}, Vector2{}},
    };
    SnapLayout layouts[]{
        SnapLayout{anchors[0]},
        SnapLayout{anchors[1]},
        SnapLayout{anchors[2]},
        SnapLayout{anchors[3]},
        SnapLayout{anchors[4]},
        SnapLayout{anchors[5]},
        SnapLayout{anchors[6]},
        SnapLayout{anchors[7]},
    };
    for(auto&& layout: layouts) {
        CORRADE_ITERATION(layout.node());
        CORRADE_COMPARE(layout.flags(), SnapLayoutFlags{});
        CORRADE_COMPARE(layout.childSnap(), Snap::Bottom);
    }

    SnapLayoutColumn layoutColumn = data.fromLayout ?
        SnapLayoutColumn{layouts[0]} : SnapLayoutColumn{anchors[0]};
    SnapLayoutColumnLeft layoutColumnLeft = data.fromLayout ?
        SnapLayoutColumnLeft{layouts[1]} : SnapLayoutColumnLeft{anchors[1]};
    SnapLayoutColumnRight layoutColumnRight = data.fromLayout ?
        SnapLayoutColumnRight{layouts[2]} : SnapLayoutColumnRight{anchors[2]};
    SnapLayoutColumnFill layoutColumnFill = data.fromLayout ?
        SnapLayoutColumnFill{layouts[3]} : SnapLayoutColumnFill{anchors[3]};
    SnapLayoutRow layoutRow = data.fromLayout ?
        SnapLayoutRow{layouts[4]} : SnapLayoutRow{anchors[4]};
    SnapLayoutRowTop layoutRowTop = data.fromLayout ?
        SnapLayoutRowTop{layouts[5]} : SnapLayoutRowTop{anchors[5]};
    SnapLayoutRowBottom layoutRowBottom = data.fromLayout ?
        SnapLayoutRowBottom{layouts[6]} : SnapLayoutRowBottom{anchors[6]};
    SnapLayoutRowFill layoutRowFill = data.fromLayout ?
        SnapLayoutRowFill{layouts[7]} : SnapLayoutRowFill{anchors[7]};

    /* Common properties that should be set for all */
    Int i = 0;
    for(auto&& layout: {static_cast<SnapLayout&>(layoutColumn),
                        static_cast<SnapLayout&>(layoutColumnLeft),
                        static_cast<SnapLayout&>(layoutColumnRight),
                        static_cast<SnapLayout&>(layoutColumnFill),
                        static_cast<SnapLayout&>(layoutRow),
                        static_cast<SnapLayout&>(layoutRowTop),
                        static_cast<SnapLayout&>(layoutRowBottom),
                        static_cast<SnapLayout&>(layoutRowFill)}) {
        CORRADE_ITERATION(layout.node());
        CORRADE_COMPARE(&layout.ui(), &ui);
        CORRADE_COMPARE(&layout.layouter(), &ui.snapLayouter());
        CORRADE_COMPARE(layout.node(), layouts[i].node());
        CORRADE_COMPARE(layout.layout(), layouts[i].layout());
        /* Margin propagation should be set only if creating from a layout */
        CORRADE_COMPARE(layout.flags(), data.fromLayout ?
            SnapLayoutFlag::PropagateMargin : SnapLayoutFlags{});
        ++i;
    }

    /* Individual properties */
    CORRADE_COMPARE(layoutColumn.childSnap(), Snap::Bottom);
    CORRADE_COMPARE(layoutColumnLeft.childSnap(), Snap::BottomLeft|Snap::InsideX);
    CORRADE_COMPARE(layoutColumnRight.childSnap(), Snap::BottomRight|Snap::InsideX);
    CORRADE_COMPARE(layoutColumnFill.childSnap(), Snap::Bottom|Snap::FillX);
    CORRADE_COMPARE(layoutRow.childSnap(), Snap::Right);
    CORRADE_COMPARE(layoutRowTop.childSnap(), Snap::TopRight|Snap::InsideY);
    CORRADE_COMPARE(layoutRowBottom.childSnap(), Snap::BottomRight|Snap::InsideY);
    CORRADE_COMPARE(layoutRowFill.childSnap(), Snap::Right|Snap::FillY);

    /* Layout that already has IgnoreOverflow* flags shouldn't have them
       overwritten with PropagateMargin* */
    SnapLayout layoutFlagsX{ui, ui.snapLayouter().add(ui.createNode({}, {}), SnapLayoutFlag::IgnoreOverflowX)};
    SnapLayout layoutFlagsY{ui, ui.snapLayouter().add(ui.createNode({}, {}), SnapLayoutFlag::IgnoreOverflowY)};
    SnapLayout layoutFlags{ui, ui.snapLayouter().add(ui.createNode({}, {}), SnapLayoutFlag::IgnoreOverflow)};
    SnapLayoutRow layoutRowFlags = layoutFlagsX;
    SnapLayoutColumnFill layoutColumnFillFlags = layoutFlagsY;
    SnapLayoutRowTop layoutRowTopFlags = layoutFlags;
    CORRADE_COMPARE(layoutRowFlags.flags(), SnapLayoutFlag::IgnoreOverflowX|SnapLayoutFlag::PropagateMarginY);
    CORRADE_COMPARE(layoutColumnFillFlags.flags(), SnapLayoutFlag::IgnoreOverflowY|SnapLayoutFlag::PropagateMarginX);
    CORRADE_COMPARE(layoutRowTopFlags.flags(), SnapLayoutFlag::IgnoreOverflow);

    /* Converting layouts between different types shouldn't be allowed, only
       to a specialized type and back. Using is_constructible as it's a
       stronger guarantee that is_convertible. These fail on MSVC 2015 (but
       work on 2017 and all other), it's not my problem if you're still
       building with MSVC 2015 as the only compiler. */
    #ifndef CORRADE_MSVC2015_COMPATIBILITY
    CORRADE_VERIFY(std::is_constructible<SnapLayoutColumn, SnapLayout>::value);
    CORRADE_VERIFY(std::is_constructible<SnapLayout, SnapLayoutColumn>::value);
    CORRADE_VERIFY(!std::is_constructible<SnapLayoutColumn, SnapLayoutRowFill>::value);
    CORRADE_VERIFY(!std::is_constructible<SnapLayoutColumnLeft, SnapLayoutColumnRight>::value);
    CORRADE_VERIFY(!std::is_constructible<SnapLayoutColumnRight, SnapLayoutRow>::value);
    CORRADE_VERIFY(!std::is_constructible<SnapLayoutColumnFill, SnapLayoutRowTop>::value);
    CORRADE_VERIFY(!std::is_constructible<SnapLayoutRow, SnapLayoutColumn>::value);
    CORRADE_VERIFY(!std::is_constructible<SnapLayoutRowTop, SnapLayoutRowBottom>::value);
    CORRADE_VERIFY(!std::is_constructible<SnapLayoutRowBottom, SnapLayoutColumnLeft>::value);
    CORRADE_VERIFY(!std::is_constructible<SnapLayoutRowFill, SnapLayoutColumnFill>::value);
    #endif
}

template<class T> void SnapLayoutTest::flags() {
    setTestCaseTemplateName(SnapLayoutTraits<T>::name());

    /* Testing both the base class and the template because the setters have
       overrides returning the derived type reference for method chaining */

    struct Interface: SnapLayoutTraits<T>::UserInterfaceType {
        explicit Interface(NoCreateT): SnapLayoutTraits<T>::UserInterfaceType{NoCreate} {}
    } ui{NoCreate};

    SnapLayouter& layouter = ui.setLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));

    T layout{ui, layouter, layouter.add(ui.createNode({}, {}), SnapLayoutFlag::PropagateMarginY)};
    CORRADE_COMPARE(layout.flags(), SnapLayoutFlag::PropagateMarginY);
    CORRADE_COMPARE(layouter.flags(layout), SnapLayoutFlag::PropagateMarginY);

    layout.setFlags(SnapLayoutFlags{0x30});
    CORRADE_COMPARE(layout.flags(), SnapLayoutFlags{0x30});
    CORRADE_COMPARE(layouter.flags(layout), SnapLayoutFlags{0x30});

    layout.addFlags(SnapLayoutFlag::IgnoreOverflow);
    CORRADE_COMPARE(layout.flags(), SnapLayoutFlag::IgnoreOverflow|SnapLayoutFlags{0x30});
    CORRADE_COMPARE(layouter.flags(layout), SnapLayoutFlag::IgnoreOverflow|SnapLayoutFlags{0x30});

    layout.clearFlags(SnapLayoutFlag::IgnoreOverflowX|SnapLayoutFlags{0x30});
    CORRADE_COMPARE(layout.flags(), SnapLayoutFlag::IgnoreOverflowY);
    CORRADE_COMPARE(layouter.flags(layout), SnapLayoutFlag::IgnoreOverflowY);
}

template<class T> void SnapLayoutTest::childSnap() {
    setTestCaseTemplateName(SnapLayoutTraits<T>::name());

    /* Testing both the base class and the template because the setters have
       overrides returning the derived type reference for method chaining */

    struct Interface: SnapLayoutTraits<T>::UserInterfaceType {
        explicit Interface(NoCreateT): SnapLayoutTraits<T>::UserInterfaceType{NoCreate} {}
    } ui{NoCreate};

    SnapLayouter& layouter = ui.setLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));

    T layout{layouter, typename SnapLayoutTraits<T>::AnchorType{ui, {}, {}}};
    CORRADE_COMPARE(layout.childSnap(), Snap::Bottom);
    CORRADE_COMPARE(layouter.childSnap(layout), Snap::Bottom);

    layout.setChildSnap(Snap::Right|Snap::FillY);
    CORRADE_COMPARE(layout.childSnap(), Snap::Right|Snap::FillY);
    CORRADE_COMPARE(layouter.childSnap(layout), Snap::Right|Snap::FillY);
}

template<class T> void SnapLayoutTest::child() {
    setTestCaseTemplateName(SnapLayoutTraits<T>::name());

    struct Interface: SnapLayoutTraits<T>::UserInterfaceType {
        explicit Interface(NoCreateT): SnapLayoutTraits<T>::UserInterfaceType{NoCreate} {}
    } ui{NoCreate};

    SnapLayouter& layouter = ui.setLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));

    /* Create some extra nodes and layouts to not have the anchor and layout
       with trivial handles */
    layouter.add(ui.createNode({}, {}));
    layouter.add(ui.createNode({}, {}));
    ui.createNode({}, {});

    T layout{layouter, typename SnapLayoutTraits<T>::AnchorType{ui, {}, {}}};
    CORRADE_COMPARE(layout.node(), nodeHandle(3, 1));
    CORRADE_COMPARE(layout.layout(), layoutHandle(layouter.handle(), 2, 1));

    /* No "before" layout */
    T child1 = layout.child({3.0f, 5.0f}, NodeFlag::NoEvents, SnapLayoutFlag::IgnoreOverflowY);
    CORRADE_COMPARE(&child1.ui(), &ui);
    CORRADE_COMPARE(&child1.layouter(), &layouter);
    CORRADE_COMPARE(child1.node(), nodeHandle(4, 1));
    CORRADE_COMPARE(child1.layout(), layoutHandle(layouter.handle(), 3, 1));
    CORRADE_VERIFY(ui.isHandleValid(child1.node()));
    CORRADE_COMPARE(ui.nodeParent(child1), layout);
    CORRADE_COMPARE(ui.nodeOffset(child1), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(child1), (Vector2{3.0f, 5.0f}));
    CORRADE_COMPARE(ui.nodeFlags(child1), NodeFlag::NoEvents);
    CORRADE_VERIFY(layouter.isHandleValid(child1));
    CORRADE_VERIFY(!layouter.hasExplicitSnap(child1));
    CORRADE_COMPARE(layouter.node(child1), child1.node());
    CORRADE_COMPARE(layouter.parent(child1), layout);
    CORRADE_COMPARE(layouter.flags(child1), SnapLayoutFlag::IgnoreOverflowY);
    CORRADE_COMPARE(layouter.childSnap(child1), Snap::Bottom);

    /* No "before" layout, no NodeFlags */
    T child2 = layout.child({4.0f, 6.0f}, SnapLayoutFlag::PropagateMarginX);
    CORRADE_COMPARE(&child2.ui(), &ui);
    CORRADE_COMPARE(&child2.layouter(), &layouter);
    CORRADE_COMPARE(child2.node(), nodeHandle(5, 1));
    CORRADE_COMPARE(child2.layout(), layoutHandle(layouter.handle(), 4, 1));
    CORRADE_VERIFY(ui.isHandleValid(child2.node()));
    CORRADE_COMPARE(ui.nodeParent(child2), layout);
    CORRADE_COMPARE(ui.nodeOffset(child2), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(child2), (Vector2{4.0f, 6.0f}));
    CORRADE_COMPARE(ui.nodeFlags(child2), NodeFlags{});
    CORRADE_VERIFY(layouter.isHandleValid(child2));
    CORRADE_VERIFY(!layouter.hasExplicitSnap(child2));
    CORRADE_COMPARE(layouter.node(child2), child2.node());
    CORRADE_COMPARE(layouter.parent(child2), layout);
    CORRADE_COMPARE(layouter.flags(child2), SnapLayoutFlag::PropagateMarginX);
    CORRADE_COMPARE(layouter.childSnap(child2), Snap::Bottom);

    /* "Before" layout as LayoutHandle */
    T child3 = layout.child({5.0f, 7.0f}, NodeFlag::FallthroughPointerEvents, child2, SnapLayoutFlag::IgnoreOverflowX);
    CORRADE_COMPARE(&child3.ui(), &ui);
    CORRADE_COMPARE(&child3.layouter(), &layouter);
    CORRADE_COMPARE(child3.node(), nodeHandle(6, 1));
    CORRADE_COMPARE(child3.layout(), layoutHandle(layouter.handle(), 5, 1));
    CORRADE_VERIFY(ui.isHandleValid(child3.node()));
    CORRADE_COMPARE(ui.nodeParent(child3), layout);
    CORRADE_COMPARE(ui.nodeOffset(child3), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(child3), (Vector2{5.0f, 7.0f}));
    CORRADE_COMPARE(ui.nodeFlags(child3), NodeFlag::FallthroughPointerEvents);
    CORRADE_VERIFY(layouter.isHandleValid(child3));
    CORRADE_VERIFY(!layouter.hasExplicitSnap(child3));
    CORRADE_COMPARE(layouter.node(child3), child3.node());
    CORRADE_COMPARE(layouter.parent(child3), layout);
    CORRADE_COMPARE(layouter.flags(child3), SnapLayoutFlag::IgnoreOverflowX);
    CORRADE_COMPARE(layouter.childSnap(child3), Snap::Bottom);

    /* "Before" layout as LayoutHandle, no NodeFlags */
    T child4 = layout.child({6.0f, 8.0f}, child1, SnapLayoutFlag::IgnoreOverflow);
    CORRADE_COMPARE(&child4.ui(), &ui);
    CORRADE_COMPARE(&child4.layouter(), &layouter);
    CORRADE_COMPARE(child4.node(), nodeHandle(7, 1));
    CORRADE_COMPARE(child4.layout(), layoutHandle(layouter.handle(), 6, 1));
    CORRADE_VERIFY(ui.isHandleValid(child4.node()));
    CORRADE_COMPARE(ui.nodeParent(child4), layout);
    CORRADE_COMPARE(ui.nodeOffset(child4), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(child4), (Vector2{6.0f, 8.0f}));
    CORRADE_COMPARE(ui.nodeFlags(child4), NodeFlags{});
    CORRADE_VERIFY(layouter.isHandleValid(child4));
    CORRADE_VERIFY(!layouter.hasExplicitSnap(child4));
    CORRADE_COMPARE(layouter.node(child4), child4.node());
    CORRADE_COMPARE(layouter.parent(child4), layout);
    CORRADE_COMPARE(layouter.flags(child4), SnapLayoutFlag::IgnoreOverflow);
    CORRADE_COMPARE(layouter.childSnap(child4), Snap::Bottom);

    /* "Before" layout as LayouterDataHandle */
    T child5 = layout.child({7.0f, 9.0f}, NodeFlag::Focusable, layoutHandleData(child3), SnapLayoutFlag::PropagateMargin);
    CORRADE_COMPARE(&child5.ui(), &ui);
    CORRADE_COMPARE(&child5.layouter(), &layouter);
    CORRADE_COMPARE(child5.node(), nodeHandle(8, 1));
    CORRADE_COMPARE(child5.layout(), layoutHandle(layouter.handle(), 7, 1));
    CORRADE_VERIFY(ui.isHandleValid(child5.node()));
    CORRADE_COMPARE(ui.nodeParent(child5), layout);
    CORRADE_COMPARE(ui.nodeOffset(child5), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(child5), (Vector2{7.0f, 9.0f}));
    CORRADE_COMPARE(ui.nodeFlags(child5), NodeFlag::Focusable);
    CORRADE_VERIFY(layouter.isHandleValid(child5));
    CORRADE_VERIFY(!layouter.hasExplicitSnap(child5));
    CORRADE_COMPARE(layouter.node(child5), child5.node());
    CORRADE_COMPARE(layouter.parent(child5), layout);
    CORRADE_COMPARE(layouter.flags(child5), SnapLayoutFlag::PropagateMargin);
    CORRADE_COMPARE(layouter.childSnap(child5), Snap::Bottom);

    /* "Before" layout as LayouterDataHandle, no NodeFlags */
    T child6 = layout.child({8.0f, 1.0f}, layoutHandleData(child1), SnapLayoutFlag::PropagateMarginY);
    CORRADE_COMPARE(&child6.ui(), &ui);
    CORRADE_COMPARE(&child6.layouter(), &layouter);
    CORRADE_COMPARE(child6.node(), nodeHandle(9, 1));
    CORRADE_COMPARE(child6.layout(), layoutHandle(layouter.handle(), 8, 1));
    CORRADE_VERIFY(ui.isHandleValid(child6.node()));
    CORRADE_COMPARE(ui.nodeParent(child6), layout);
    CORRADE_COMPARE(ui.nodeOffset(child6), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(child6), (Vector2{8.0f, 1.0f}));
    CORRADE_COMPARE(ui.nodeFlags(child6), NodeFlags{});
    CORRADE_VERIFY(layouter.isHandleValid(child6));
    CORRADE_VERIFY(!layouter.hasExplicitSnap(child6));
    CORRADE_COMPARE(layouter.node(child6), child6.node());
    CORRADE_COMPARE(layouter.parent(child6), layout);
    CORRADE_COMPARE(layouter.flags(child6), SnapLayoutFlag::PropagateMarginY);
    CORRADE_COMPARE(layouter.childSnap(child6), Snap::Bottom);

    /* Verify the order arguments were actually used */
    CORRADE_COMPARE(layouter.firstChild(layout), child4);
    CORRADE_COMPARE(layouter.next(child4), child6);
    CORRADE_COMPARE(layouter.next(child6), child1);
    CORRADE_COMPARE(layouter.next(child1), child5);
    CORRADE_COMPARE(layouter.next(child5), child3);
    CORRADE_COMPARE(layouter.next(child3), child2);
    CORRADE_COMPARE(layouter.next(child2), LayoutHandle::Null);

    /* There should be no additional nodes or layouts created besides the ones
       known above */
    CORRADE_COMPARE(ui.nodeUsedCount(), 10);
    CORRADE_COMPARE(layouter.usedCount(), 9);
}

template<class T> void SnapLayoutTest::childExplicitSnap() {
    setTestCaseTemplateName(SnapLayoutTraits<T>::name());

    struct Interface: SnapLayoutTraits<T>::UserInterfaceType {
        explicit Interface(NoCreateT): SnapLayoutTraits<T>::UserInterfaceType{NoCreate} {}
    } ui{NoCreate};

    SnapLayouter& layouter = ui.setLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));

    /* Create some extra nodes and layouts to not have the anchor and layout
       with trivial handles */
    layouter.add(ui.createNode({}, {}));
    layouter.add(ui.createNode({}, {}));
    ui.createNode({}, {});

    T layout{layouter, typename SnapLayoutTraits<T>::AnchorType{ui, {}, {}}};
    CORRADE_COMPARE(layout.node(), nodeHandle(3, 1));
    CORRADE_COMPARE(layout.layout(), layoutHandle(layouter.handle(), 2, 1));

    /* With NodeFlags */
    T child1 = layout.child(Snap::TopRight, {3.0f, 5.0f}, NodeFlag::NoEvents, SnapLayoutFlag::IgnoreOverflowY);
    CORRADE_COMPARE(&child1.ui(), &ui);
    CORRADE_COMPARE(&child1.layouter(), &layouter);
    CORRADE_COMPARE(child1.node(), nodeHandle(4, 1));
    CORRADE_COMPARE(child1.layout(), layoutHandle(layouter.handle(), 3, 1));
    CORRADE_VERIFY(ui.isHandleValid(child1.node()));
    CORRADE_COMPARE(ui.nodeParent(child1), layout);
    CORRADE_COMPARE(ui.nodeOffset(child1), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(child1), (Vector2{3.0f, 5.0f}));
    CORRADE_COMPARE(ui.nodeFlags(child1), NodeFlag::NoEvents);
    CORRADE_VERIFY(layouter.isHandleValid(child1));
    CORRADE_VERIFY(layouter.hasExplicitSnap(child1));
    CORRADE_COMPARE(layouter.node(child1), child1.node());
    CORRADE_COMPARE(layouter.explicitSnap(child1), Snap::TopRight);
    CORRADE_COMPARE(layouter.explicitSnapTarget(child1), layout);
    CORRADE_COMPARE(layouter.flags(child1), SnapLayoutFlag::IgnoreOverflowY);
    CORRADE_COMPARE(layouter.childSnap(child1), Snap::Bottom);

    /* No NodeFlags */
    T child2 = layout.child(Snap::Bottom|Snap::FillX|Snap::NoPad, {4.0f, 6.0f}, SnapLayoutFlag::PropagateMarginX);
    CORRADE_COMPARE(&child2.ui(), &ui);
    CORRADE_COMPARE(&child2.layouter(), &layouter);
    CORRADE_COMPARE(child2.node(), nodeHandle(5, 1));
    CORRADE_COMPARE(child2.layout(), layoutHandle(layouter.handle(), 4, 1));
    CORRADE_VERIFY(ui.isHandleValid(child2.node()));
    CORRADE_COMPARE(ui.nodeParent(child2), layout);
    CORRADE_COMPARE(ui.nodeOffset(child2), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(child2), (Vector2{4.0f, 6.0f}));
    CORRADE_COMPARE(ui.nodeFlags(child2), NodeFlags{});
    CORRADE_VERIFY(layouter.isHandleValid(child2));
    CORRADE_VERIFY(layouter.hasExplicitSnap(child2));
    CORRADE_COMPARE(layouter.node(child2), child2.node());
    CORRADE_COMPARE(layouter.explicitSnap(child2), Snap::Bottom|Snap::FillX|Snap::NoPad);
    CORRADE_COMPARE(layouter.explicitSnapTarget(child2), layout);
    CORRADE_COMPARE(layouter.flags(child2), SnapLayoutFlag::PropagateMarginX);
    CORRADE_COMPARE(layouter.childSnap(child2), Snap::Bottom);

    /* There should be no additional nodes or layouts created besides the ones
       known above */
    CORRADE_COMPARE(ui.nodeUsedCount(), 6);
    CORRADE_COMPARE(layouter.usedCount(), 5);
}

template<class T> void SnapLayoutTest::siblingExplicitSnap() {
    setTestCaseTemplateName(SnapLayoutTraits<T>::name());

    struct Interface: SnapLayoutTraits<T>::UserInterfaceType {
        explicit Interface(NoCreateT): SnapLayoutTraits<T>::UserInterfaceType{NoCreate} {}
    } ui{NoCreate};

    SnapLayouter& layouter = ui.setLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));

    /* Create some extra nodes and layouts to not have the anchor and layout
       with trivial handles */
    layouter.add(ui.createNode({}, {}));
    layouter.add(ui.createNode({}, {}));
    ui.createNode({}, {});

    NodeHandle root = ui.createNode({}, {});

    T layout{layouter, typename SnapLayoutTraits<T>::AnchorType{ui, root, {}, {}}};
    CORRADE_COMPARE(layout.node(), nodeHandle(4, 1));
    CORRADE_COMPARE(layout.layout(), layoutHandle(layouter.handle(), 2, 1));

    /* With NodeFlags */
    T sibling1 = layout.sibling(Snap::TopRight, {3.0f, 5.0f}, NodeFlag::NoEvents, SnapLayoutFlag::IgnoreOverflowY);
    CORRADE_COMPARE(&sibling1.ui(), &ui);
    CORRADE_COMPARE(&sibling1.layouter(), &layouter);
    CORRADE_COMPARE(sibling1.node(), nodeHandle(5, 1));
    CORRADE_COMPARE(sibling1.layout(), layoutHandle(layouter.handle(), 3, 1));
    CORRADE_VERIFY(ui.isHandleValid(sibling1.node()));
    CORRADE_COMPARE(ui.nodeParent(sibling1), root);
    CORRADE_COMPARE(ui.nodeOffset(sibling1), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(sibling1), (Vector2{3.0f, 5.0f}));
    CORRADE_COMPARE(ui.nodeFlags(sibling1), NodeFlag::NoEvents);
    CORRADE_VERIFY(layouter.isHandleValid(sibling1));
    CORRADE_VERIFY(layouter.hasExplicitSnap(sibling1));
    CORRADE_COMPARE(layouter.node(sibling1), sibling1.node());
    CORRADE_COMPARE(layouter.explicitSnap(sibling1), Snap::TopRight);
    CORRADE_COMPARE(layouter.explicitSnapTarget(sibling1), layout);
    CORRADE_COMPARE(layouter.flags(sibling1), SnapLayoutFlag::IgnoreOverflowY);
    CORRADE_COMPARE(layouter.childSnap(sibling1), Snap::Bottom);

    /* No NodeFlags */
    T sibling2 = layout.sibling(Snap::Bottom|Snap::FillX|Snap::NoPad, {4.0f, 6.0f}, SnapLayoutFlag::PropagateMarginX);
    CORRADE_COMPARE(&sibling2.ui(), &ui);
    CORRADE_COMPARE(&sibling2.layouter(), &layouter);
    CORRADE_COMPARE(sibling2.node(), nodeHandle(6, 1));
    CORRADE_COMPARE(sibling2.layout(), layoutHandle(layouter.handle(), 4, 1));
    CORRADE_VERIFY(ui.isHandleValid(sibling2.node()));
    CORRADE_COMPARE(ui.nodeParent(sibling2), root);
    CORRADE_COMPARE(ui.nodeOffset(sibling2), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(sibling2), (Vector2{4.0f, 6.0f}));
    CORRADE_COMPARE(ui.nodeFlags(sibling2), NodeFlags{});
    CORRADE_VERIFY(layouter.isHandleValid(sibling2));
    CORRADE_VERIFY(layouter.hasExplicitSnap(sibling2));
    CORRADE_COMPARE(layouter.node(sibling2), sibling2.node());
    CORRADE_COMPARE(layouter.explicitSnap(sibling2), Snap::Bottom|Snap::FillX|Snap::NoPad);
    CORRADE_COMPARE(layouter.explicitSnapTarget(sibling2), layout);
    CORRADE_COMPARE(layouter.flags(sibling2), SnapLayoutFlag::PropagateMarginX);
    CORRADE_COMPARE(layouter.childSnap(sibling2), Snap::Bottom);

    /* There should be no additional nodes or layouts created besides the ones
       known above */
    CORRADE_COMPARE(ui.nodeUsedCount(), 7);
    CORRADE_COMPARE(layouter.usedCount(), 5);
}

template<class T> void SnapLayoutTest::staticRootExplicitSnap() {
    setTestCaseTemplateName(SnapLayoutTraits<T>::name());

    struct Interface: SnapLayoutTraits<T>::UserInterfaceType {
        explicit Interface(NoCreateT): SnapLayoutTraits<T>::UserInterfaceType{NoCreate} {}
    } ui{NoCreate};

    SnapLayouter& layouter = ui.setLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));

    /* Create some extra nodes and layouts to not have the anchor and layout
       with trivial handles */
    layouter.add(ui.createNode({}, {}));
    layouter.add(ui.createNode({}, {}));
    ui.createNode({}, {});

    /* With NodeFlags */
    T root1 = T::root(ui, layouter, Snap::TopRight, {3.0f, 5.0f}, NodeFlag::Disabled, SnapLayoutFlag::IgnoreOverflowY);
    CORRADE_COMPARE(&root1.ui(), &ui);
    CORRADE_COMPARE(&root1.layouter(), &layouter);
    CORRADE_COMPARE(root1.node(), nodeHandle(3, 1));
    CORRADE_COMPARE(root1.layout(), layoutHandle(layouter.handle(), 2, 1));
    CORRADE_VERIFY(ui.isHandleValid(root1.node()));
    CORRADE_COMPARE(ui.nodeParent(root1), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOffset(root1), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(root1), (Vector2{3.0f, 5.0f}));
    CORRADE_COMPARE(ui.nodeFlags(root1), NodeFlag::Disabled);
    CORRADE_VERIFY(layouter.isHandleValid(root1));
    CORRADE_VERIFY(layouter.hasExplicitSnap(root1));
    CORRADE_COMPARE(layouter.node(root1), root1.node());
    CORRADE_COMPARE(layouter.explicitSnap(root1), Snap::TopRight);
    CORRADE_COMPARE(layouter.explicitSnapTarget(root1), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.flags(root1), SnapLayoutFlag::IgnoreOverflowY);
    CORRADE_COMPARE(layouter.childSnap(root1), Snap::Bottom);

    /* Without NodeFlags */
    T root2 = T::root(ui, layouter, Snap::Fill, {4.0f, 6.0f}, SnapLayoutFlag::PropagateMarginX);
    CORRADE_COMPARE(&root2.ui(), &ui);
    CORRADE_COMPARE(&root2.layouter(), &layouter);
    CORRADE_COMPARE(root2.node(), nodeHandle(4, 1));
    CORRADE_COMPARE(root2.layout(), layoutHandle(layouter.handle(), 3, 1));
    CORRADE_VERIFY(ui.isHandleValid(root2.node()));
    CORRADE_COMPARE(ui.nodeParent(root2), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOffset(root2), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(root2), (Vector2{4.0f, 6.0f}));
    CORRADE_COMPARE(ui.nodeFlags(root2), NodeFlags{});
    CORRADE_VERIFY(layouter.isHandleValid(root2));
    CORRADE_VERIFY(layouter.hasExplicitSnap(root2));
    CORRADE_COMPARE(layouter.node(root2), root2.node());
    CORRADE_COMPARE(layouter.explicitSnap(root2), Snap::Fill);
    CORRADE_COMPARE(layouter.explicitSnapTarget(root2), LayoutHandle::Null);
    CORRADE_COMPARE(layouter.flags(root2), SnapLayoutFlag::PropagateMarginX);
    CORRADE_COMPARE(layouter.childSnap(root2), Snap::Bottom);

    /* There should be no additional nodes or layouts created besides the ones
       known above */
    CORRADE_COMPARE(ui.nodeUsedCount(), 5);
    CORRADE_COMPARE(layouter.usedCount(), 4);
}

void SnapLayoutTest::staticRootExplicitSnapImplicitLayouter() {
    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};

    ui.setSnapLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));

    /* Create some extra nodes and layouts to not have the anchor and layout
       with trivial handles */
    ui.snapLayouter().add(ui.createNode({}, {}));
    ui.snapLayouter().add(ui.createNode({}, {}));
    ui.createNode({}, {});

    /* With NodeFlags */
    SnapLayout root1 = SnapLayout::root(ui, Snap::TopRight, {3.0f, 5.0f}, NodeFlag::Disabled, SnapLayoutFlag::IgnoreOverflowY);
    CORRADE_COMPARE(&root1.ui(), &ui);
    CORRADE_COMPARE(&root1.layouter(), &ui.snapLayouter());
    CORRADE_COMPARE(root1.node(), nodeHandle(3, 1));
    CORRADE_COMPARE(root1.layout(), layoutHandle(ui.snapLayouter().handle(), 2, 1));
    CORRADE_VERIFY(ui.isHandleValid(root1.node()));
    CORRADE_COMPARE(ui.nodeParent(root1), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOffset(root1), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(root1), (Vector2{3.0f, 5.0f}));
    CORRADE_COMPARE(ui.nodeFlags(root1), NodeFlag::Disabled);
    CORRADE_VERIFY(ui.snapLayouter().isHandleValid(root1));
    CORRADE_VERIFY(ui.snapLayouter().hasExplicitSnap(root1));
    CORRADE_COMPARE(ui.snapLayouter().node(root1), root1.node());
    CORRADE_COMPARE(ui.snapLayouter().explicitSnap(root1), Snap::TopRight);
    CORRADE_COMPARE(ui.snapLayouter().explicitSnapTarget(root1), LayoutHandle::Null);
    CORRADE_COMPARE(ui.snapLayouter().flags(root1), SnapLayoutFlag::IgnoreOverflowY);
    CORRADE_COMPARE(ui.snapLayouter().childSnap(root1), Snap::Bottom);

    /* Without NodeFlags */
    SnapLayout root2 = SnapLayout::root(ui, Snap::Fill, {4.0f, 6.0f}, SnapLayoutFlag::PropagateMarginX);
    CORRADE_COMPARE(&root2.ui(), &ui);
    CORRADE_COMPARE(&root2.layouter(), &ui.snapLayouter());
    CORRADE_COMPARE(root2.node(), nodeHandle(4, 1));
    CORRADE_COMPARE(root2.layout(), layoutHandle(ui.snapLayouter().handle(), 3, 1));
    CORRADE_VERIFY(ui.isHandleValid(root2.node()));
    CORRADE_COMPARE(ui.nodeParent(root2), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOffset(root2), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(root2), (Vector2{4.0f, 6.0f}));
    CORRADE_COMPARE(ui.nodeFlags(root2), NodeFlags{});
    CORRADE_VERIFY(ui.snapLayouter().isHandleValid(root2));
    CORRADE_VERIFY(ui.snapLayouter().hasExplicitSnap(root2));
    CORRADE_COMPARE(ui.snapLayouter().node(root2), root2.node());
    CORRADE_COMPARE(ui.snapLayouter().explicitSnap(root2), Snap::Fill);
    CORRADE_COMPARE(ui.snapLayouter().explicitSnapTarget(root2), LayoutHandle::Null);
    CORRADE_COMPARE(ui.snapLayouter().flags(root2), SnapLayoutFlag::PropagateMarginX);
    CORRADE_COMPARE(ui.snapLayouter().childSnap(root2), Snap::Bottom);

    /* There should be no additional nodes or layouts created besides the ones
       known above */
    CORRADE_COMPARE(ui.nodeUsedCount(), 5);
    CORRADE_COMPARE(ui.snapLayouter().usedCount(), 4);
}

template<class T> void SnapLayoutTest::staticInvalid() {
    setTestCaseTemplateName(SnapLayoutTraits<T>::name());

    CORRADE_SKIP_IF_NO_ASSERT();

    struct Interface: SnapLayoutTraits<T>::UserInterfaceType {
        explicit Interface(NoCreateT): SnapLayoutTraits<T>::UserInterfaceType{NoCreate} {}
    } ui{NoCreate},
      anotherUi{NoCreate};

    /* Create some extra layouters to not have it with a trivial handle. Same
       has to be done for the other UI instance to have the handles match
       below. */
    ui.createLayouter();
    ui.createLayouter();
    ui.removeLayouter(ui.createLayouter());
    ui.removeLayouter(ui.createLayouter());
    anotherUi.createLayouter();
    anotherUi.createLayouter();
    anotherUi.removeLayouter(anotherUi.createLayouter());
    anotherUi.removeLayouter(anotherUi.createLayouter());

    SnapLayouter& layouter = ui.setLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));
    /* Layouter with a handle that isn't even valid in the UI, to verify it
       isn't blindly accessng it */
    SnapLayouter layouterInvalidHandle{layouterHandle(0xab, 0x12)};
    /* Layouter with the same handle as the right one, but in another UI, to
       verify it's checking the actual instance as well */
    SnapLayouter& layouterAnotherUi = anotherUi.setLayouterInstance(Containers::pointer<SnapLayouter>(anotherUi.createLayouter()));
    CORRADE_COMPARE(layouterAnotherUi.handle(), layouter.handle());

    Containers::String out;
    Error redirectError{&out};
    /* Layouter handle not valid in the UI */
    T::root(ui, layouterInvalidHandle, Snaps{}, {}, NodeFlags{});
    T::root(ui, layouterInvalidHandle, Snaps{}, {}, SnapLayoutFlags{});
    /* Valid handle, but different instance */
    T::root(ui, layouterAnotherUi, Snaps{}, {}, NodeFlags{});
    T::root(ui, layouterAnotherUi, Snaps{}, {}, SnapLayoutFlags{});
    CORRADE_COMPARE_AS(out,
        "Ui::AbstractSnapLayout::root(): layouter not part of the UI\n"
        "Ui::AbstractSnapLayout::root(): layouter not part of the UI\n"

        "Ui::AbstractSnapLayout::root(): layouter not part of the UI\n"
        "Ui::AbstractSnapLayout::root(): layouter not part of the UI\n",
        TestSuite::Compare::String);
}

void SnapLayoutTest::staticInvalidImplicitLayouter() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate},
      uiNoSnapLayouter{NoCreate};

    Containers::String out;
    Error redirectError{&out};
    SnapLayout::root(uiNoSnapLayouter, Snaps{}, {}, NodeFlags{});
    SnapLayout::root(uiNoSnapLayouter, Snaps{}, {}, SnapLayoutFlags{});
    CORRADE_COMPARE_AS(out,
        "Ui::BasicSnapLayout::root(): SnapLayouter not present in the UI\n"
        "Ui::BasicSnapLayout::root(): SnapLayouter not present in the UI\n",
        TestSuite::Compare::String);
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::SnapLayoutTest)
