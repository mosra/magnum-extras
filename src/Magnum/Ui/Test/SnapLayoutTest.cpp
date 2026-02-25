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
    template<class T> void constructCreateLayout();
    void constructCreateLayoutImplicitLayouter();
    template<class T> void constructCreateLayoutExplicitSnap();
    void constructCreateLayoutExplicitSnapImplicitLayouter();
    template<class T> void constructInvalid();
    void constructInvalidImplicitLayouter();

    template<class T> void convertAnchor();
    void convertSpecialized();

    template<class T> void flags();
    template<class T> void childSnap();

    template<class T> void child();
    template<class T> void childExplicitSnap();
    template<class T> void siblingExplicitSnap();

    template<class T> void staticChild();
    void staticChildImplicitLayouter();
    template<class T> void staticRootExplicitSnap();
    void staticRootExplicitSnapImplicitLayouter();
    template<class T> void staticChildExplicitSnap();
    void statiicChildExplicitSnapImplicitLayouter();
    template<class T> void staticSiblingExplicitSnap();
    void staticSiblingExplicitSnapImplicitLayouter();
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

    addTests({&SnapLayoutTest::constructCreateLayout<AbstractSnapLayout>,
              &SnapLayoutTest::constructCreateLayout<SnapLayout>,
              &SnapLayoutTest::constructCreateLayoutImplicitLayouter,
              &SnapLayoutTest::constructCreateLayoutExplicitSnap<AbstractSnapLayout>,
              &SnapLayoutTest::constructCreateLayoutExplicitSnap<SnapLayout>,
              &SnapLayoutTest::constructCreateLayoutExplicitSnapImplicitLayouter,
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

              &SnapLayoutTest::staticChild<AbstractSnapLayout>,
              &SnapLayoutTest::staticChild<SnapLayout>,
              &SnapLayoutTest::staticChildImplicitLayouter,
              &SnapLayoutTest::staticRootExplicitSnap<AbstractSnapLayout>,
              &SnapLayoutTest::staticRootExplicitSnap<SnapLayout>,
              &SnapLayoutTest::staticRootExplicitSnapImplicitLayouter,
              &SnapLayoutTest::staticChildExplicitSnap<AbstractSnapLayout>,
              &SnapLayoutTest::staticChildExplicitSnap<SnapLayout>,
              &SnapLayoutTest::statiicChildExplicitSnapImplicitLayouter,
              &SnapLayoutTest::staticSiblingExplicitSnap<AbstractSnapLayout>,
              &SnapLayoutTest::staticSiblingExplicitSnap<SnapLayout>,
              &SnapLayoutTest::staticSiblingExplicitSnapImplicitLayouter,
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
    CORRADE_COMPARE(layout, layoutHandle(layouter.handle(), 3, 2));
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

    typename SnapLayoutTraits<T>::AnchorType anchor{ui, {}, {}};
    CORRADE_COMPARE(anchor.node(), nodeHandle(5, 3));

    if(data.layoutExists)
        layouter.add(anchor);

    T layout{ui, layouter, anchor.node()};
    CORRADE_COMPARE(&layout.ui(), &ui);
    CORRADE_COMPARE(&layout.layouter(), &layouter);
    CORRADE_COMPARE(layout.node(), anchor.node());
    CORRADE_COMPARE(layout, anchor.node());
    CORRADE_COMPARE(layout.layout(), layoutHandle(layouter.handle(), 3, 2));
    CORRADE_COMPARE(layout, layoutHandle(layouter.handle(), 3, 2));
    CORRADE_VERIFY(layouter.isHandleValid(layout));
    CORRADE_VERIFY(!layouter.hasExplicitSnap(layout));
    CORRADE_COMPARE(layouter.node(layout), anchor.node());
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
    CORRADE_COMPARE(layout, layoutHandle(ui.snapLayouter().handle(), 3, 2));
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

    Anchor anchor{ui, {}, {}};
    CORRADE_COMPARE(anchor.node(), nodeHandle(5, 3));

    if(data.layoutExists)
        ui.snapLayouter().add(anchor);

    SnapLayout layout{ui, anchor.node()};
    CORRADE_COMPARE(&layout.ui(), &ui);
    CORRADE_COMPARE(&layout.layouter(), &ui.snapLayouter());
    CORRADE_COMPARE(layout.node(), anchor.node());
    CORRADE_COMPARE(layout, anchor.node());
    CORRADE_COMPARE(layout.layout(), layoutHandle(ui.snapLayouter().handle(), 3, 2));
    CORRADE_COMPARE(layout, layoutHandle(ui.snapLayouter().handle(), 3, 2));
    CORRADE_VERIFY(ui.snapLayouter().isHandleValid(layout));
    CORRADE_VERIFY(!ui.snapLayouter().hasExplicitSnap(layout));
    CORRADE_COMPARE(ui.snapLayouter().node(layout), anchor.node());
    CORRADE_COMPARE(ui.snapLayouter().flags(layout), SnapLayoutFlags{});
    CORRADE_COMPARE(ui.snapLayouter().childSnap(layout), Snap::Bottom);

    /* There should be no additional layouts created besides the ones known
       above */
    CORRADE_COMPARE(ui.snapLayouter().usedCount(), 4);
}

template<class T> void SnapLayoutTest::constructCreateLayout() {
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

    /* Create a root layout so the newly created layouts become ordered
       children */
    NodeHandle rootNode = ui.createNode({}, {});
    LayoutHandle root = layouter.add(rootNode);

    typename SnapLayoutTraits<T>::AnchorType firstAnchor1{ui, rootNode, {}, {}};
    typename SnapLayoutTraits<T>::AnchorType firstAnchor2{ui, rootNode, {}, {}};
    typename SnapLayoutTraits<T>::AnchorType secondAnchor1{ui, rootNode, {}, {}};
    typename SnapLayoutTraits<T>::AnchorType secondAnchor2{ui, rootNode, {}, {}};
    typename SnapLayoutTraits<T>::AnchorType thirdAnchor1{ui, rootNode, {}, {}};
    typename SnapLayoutTraits<T>::AnchorType thirdAnchor2{ui, rootNode, {}, {}};
    CORRADE_COMPARE(firstAnchor1.node(), nodeHandle(4, 1));
    CORRADE_COMPARE(firstAnchor2.node(), nodeHandle(5, 1));
    CORRADE_COMPARE(secondAnchor1.node(), nodeHandle(6, 1));
    CORRADE_COMPARE(secondAnchor2.node(), nodeHandle(7, 1));
    CORRADE_COMPARE(thirdAnchor1.node(), nodeHandle(8, 1));
    CORRADE_COMPARE(thirdAnchor2.node(), nodeHandle(9, 1));

    /* No `before` layout */
    T first1{layouter, firstAnchor1, SnapLayoutFlag::IgnoreOverflowX};
    T first2{ui, layouter, firstAnchor2.node(), SnapLayoutFlag::IgnoreOverflowX};
    CORRADE_COMPARE(&first1.ui(), &ui);
    CORRADE_COMPARE(&first2.ui(), &ui);
    CORRADE_COMPARE(&first1.layouter(), &layouter);
    CORRADE_COMPARE(&first2.layouter(), &layouter);
    CORRADE_COMPARE(first1.node(), firstAnchor1.node());
    CORRADE_COMPARE(first2.node(), firstAnchor2.node());
    CORRADE_COMPARE(first1, firstAnchor1.node());
    CORRADE_COMPARE(first2, firstAnchor2.node());
    CORRADE_COMPARE(first1.layout(), layoutHandle(layouter.handle(), 3, 1));
    CORRADE_COMPARE(first2.layout(), layoutHandle(layouter.handle(), 4, 1));
    CORRADE_COMPARE(first1, first1.layout());
    CORRADE_COMPARE(first2, first2.layout());
    CORRADE_VERIFY(layouter.isHandleValid(first1));
    CORRADE_VERIFY(layouter.isHandleValid(first2));
    CORRADE_VERIFY(!layouter.hasExplicitSnap(first1));
    CORRADE_VERIFY(!layouter.hasExplicitSnap(first2));
    CORRADE_COMPARE(layouter.node(first1), firstAnchor1.node());
    CORRADE_COMPARE(layouter.node(first2), firstAnchor2.node());
    CORRADE_COMPARE(layouter.flags(first1), SnapLayoutFlag::IgnoreOverflowX);
    CORRADE_COMPARE(layouter.flags(first2), SnapLayoutFlag::IgnoreOverflowX);
    CORRADE_COMPARE(layouter.childSnap(first1), Snap::Bottom);
    CORRADE_COMPARE(layouter.childSnap(first2), Snap::Bottom);

    /* "Before" layout as a LayoutHandle */
    T second1{layouter, secondAnchor1, LayoutHandle{first1}, SnapLayoutFlag::PropagateMarginY};
    T second2{ui, layouter, secondAnchor2.node(), LayoutHandle{first1}, SnapLayoutFlag::PropagateMarginY};
    CORRADE_COMPARE(&second1.ui(), &ui);
    CORRADE_COMPARE(&second2.ui(), &ui);
    CORRADE_COMPARE(&second1.layouter(), &layouter);
    CORRADE_COMPARE(&second2.layouter(), &layouter);
    CORRADE_COMPARE(second1.node(), secondAnchor1.node());
    CORRADE_COMPARE(second2.node(), secondAnchor2.node());
    CORRADE_COMPARE(second1, secondAnchor1.node());
    CORRADE_COMPARE(second2, secondAnchor2.node());
    CORRADE_COMPARE(second1.layout(), layoutHandle(layouter.handle(), 5, 1));
    CORRADE_COMPARE(second2.layout(), layoutHandle(layouter.handle(), 6, 1));
    CORRADE_COMPARE(second1, second1.layout());
    CORRADE_COMPARE(second2, second2.layout());
    CORRADE_VERIFY(layouter.isHandleValid(second1));
    CORRADE_VERIFY(layouter.isHandleValid(second2));
    CORRADE_VERIFY(!layouter.hasExplicitSnap(second1));
    CORRADE_VERIFY(!layouter.hasExplicitSnap(second2));
    CORRADE_COMPARE(layouter.node(second1), secondAnchor1.node());
    CORRADE_COMPARE(layouter.node(second2), secondAnchor2.node());
    CORRADE_COMPARE(layouter.flags(second1), SnapLayoutFlag::PropagateMarginY);
    CORRADE_COMPARE(layouter.flags(second2), SnapLayoutFlag::PropagateMarginY);
    CORRADE_COMPARE(layouter.childSnap(second1), Snap::Bottom);
    CORRADE_COMPARE(layouter.childSnap(second2), Snap::Bottom);

    /* "Before" layout as a LayouterDataHandle */
    T third1{layouter, thirdAnchor1, layoutHandleData(first1), SnapLayoutFlag::IgnoreOverflow};
    T third2{ui, layouter, thirdAnchor2.node(), layoutHandleData(first1), SnapLayoutFlag::IgnoreOverflow};
    CORRADE_COMPARE(&third1.ui(), &ui);
    CORRADE_COMPARE(&third2.ui(), &ui);
    CORRADE_COMPARE(&third1.layouter(), &layouter);
    CORRADE_COMPARE(&third2.layouter(), &layouter);
    CORRADE_COMPARE(third1.node(), thirdAnchor1.node());
    CORRADE_COMPARE(third2.node(), thirdAnchor2.node());
    CORRADE_COMPARE(third1, thirdAnchor1.node());
    CORRADE_COMPARE(third2, thirdAnchor2.node());
    CORRADE_COMPARE(third1.layout(), layoutHandle(layouter.handle(), 7, 1));
    CORRADE_COMPARE(third2.layout(), layoutHandle(layouter.handle(), 8, 1));
    CORRADE_COMPARE(third1, third1.layout());
    CORRADE_COMPARE(third2, third2.layout());
    CORRADE_VERIFY(layouter.isHandleValid(third1));
    CORRADE_VERIFY(layouter.isHandleValid(third2));
    CORRADE_VERIFY(!layouter.hasExplicitSnap(third1));
    CORRADE_VERIFY(!layouter.hasExplicitSnap(third2));
    CORRADE_COMPARE(layouter.node(third1), thirdAnchor1.node());
    CORRADE_COMPARE(layouter.node(third2), thirdAnchor2.node());
    CORRADE_COMPARE(layouter.flags(third1), SnapLayoutFlag::IgnoreOverflow);
    CORRADE_COMPARE(layouter.flags(third2), SnapLayoutFlag::IgnoreOverflow);
    CORRADE_COMPARE(layouter.childSnap(third1), Snap::Bottom);
    CORRADE_COMPARE(layouter.childSnap(third2), Snap::Bottom);

    /* Verify the order arguments were actually used */
    CORRADE_COMPARE(layouter.firstChild(root), second1);
    CORRADE_COMPARE(layouter.next(second1), second2);
    CORRADE_COMPARE(layouter.next(second2), third1);
    CORRADE_COMPARE(layouter.next(third1), third2);
    CORRADE_COMPARE(layouter.next(third2), first1);
    CORRADE_COMPARE(layouter.next(first1), first2);
    CORRADE_COMPARE(layouter.next(first2), LayoutHandle::Null);

    /* There should be no additional layouts created besides the ones known
       above */
    CORRADE_COMPARE(layouter.usedCount(), 3 + 2*3);
}

void SnapLayoutTest::constructCreateLayoutImplicitLayouter() {
    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};

    ui.setSnapLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));

    /* Create some extra nodes and layouts to not have the anchor and layout
       with trivial handles */
    ui.snapLayouter().add(ui.createNode({}, {}));
    ui.snapLayouter().add(ui.createNode({}, {}));
    ui.createNode({}, {});

    /* Create a root layout so the newly created layouts become ordered
       children */
    NodeHandle rootNode = ui.createNode({}, {});
    LayoutHandle root = ui.snapLayouter().add(rootNode);

    Anchor firstAnchor1{ui, rootNode, {}, {}};
    Anchor firstAnchor2{ui, rootNode, {}, {}};
    Anchor secondAnchor1{ui, rootNode, {}, {}};
    Anchor secondAnchor2{ui, rootNode, {}, {}};
    Anchor thirdAnchor1{ui, rootNode, {}, {}};
    Anchor thirdAnchor2{ui, rootNode, {}, {}};
    CORRADE_COMPARE(firstAnchor1.node(), nodeHandle(4, 1));
    CORRADE_COMPARE(firstAnchor2.node(), nodeHandle(5, 1));
    CORRADE_COMPARE(secondAnchor1.node(), nodeHandle(6, 1));
    CORRADE_COMPARE(secondAnchor2.node(), nodeHandle(7, 1));
    CORRADE_COMPARE(thirdAnchor1.node(), nodeHandle(8, 1));
    CORRADE_COMPARE(thirdAnchor2.node(), nodeHandle(9, 1));

    /* No `before` layout */
    SnapLayout first1{firstAnchor1, SnapLayoutFlag::IgnoreOverflowX};
    SnapLayout first2{ui, firstAnchor2.node(), SnapLayoutFlag::IgnoreOverflowX};
    CORRADE_COMPARE(&first1.ui(), &ui);
    CORRADE_COMPARE(&first2.ui(), &ui);
    CORRADE_COMPARE(&first1.layouter(), &ui.snapLayouter());
    CORRADE_COMPARE(&first2.layouter(), &ui.snapLayouter());
    CORRADE_COMPARE(first1.node(), firstAnchor1.node());
    CORRADE_COMPARE(first2.node(), firstAnchor2.node());
    CORRADE_COMPARE(first1, firstAnchor1.node());
    CORRADE_COMPARE(first2, firstAnchor2.node());
    CORRADE_COMPARE(first1.layout(), layoutHandle(ui.snapLayouter().handle(), 3, 1));
    CORRADE_COMPARE(first2.layout(), layoutHandle(ui.snapLayouter().handle(), 4, 1));
    CORRADE_COMPARE(first1, first1.layout());
    CORRADE_COMPARE(first2, first2.layout());
    CORRADE_VERIFY(ui.snapLayouter().isHandleValid(first1));
    CORRADE_VERIFY(ui.snapLayouter().isHandleValid(first2));
    CORRADE_VERIFY(!ui.snapLayouter().hasExplicitSnap(first1));
    CORRADE_VERIFY(!ui.snapLayouter().hasExplicitSnap(first2));
    CORRADE_COMPARE(ui.snapLayouter().node(first1), firstAnchor1.node());
    CORRADE_COMPARE(ui.snapLayouter().node(first2), firstAnchor2.node());
    CORRADE_COMPARE(ui.snapLayouter().flags(first1), SnapLayoutFlag::IgnoreOverflowX);
    CORRADE_COMPARE(ui.snapLayouter().flags(first2), SnapLayoutFlag::IgnoreOverflowX);
    CORRADE_COMPARE(ui.snapLayouter().childSnap(first1), Snap::Bottom);
    CORRADE_COMPARE(ui.snapLayouter().childSnap(first2), Snap::Bottom);

    /* "Before" layout as a LayoutHandle */
    SnapLayout second1{secondAnchor1, LayoutHandle{first1}, SnapLayoutFlag::PropagateMarginY};
    SnapLayout second2{ui, secondAnchor2.node(), LayoutHandle{first1}, SnapLayoutFlag::PropagateMarginY};
    CORRADE_COMPARE(&second1.ui(), &ui);
    CORRADE_COMPARE(&second2.ui(), &ui);
    CORRADE_COMPARE(&second1.layouter(), &ui.snapLayouter());
    CORRADE_COMPARE(&second2.layouter(), &ui.snapLayouter());
    CORRADE_COMPARE(second1.node(), secondAnchor1.node());
    CORRADE_COMPARE(second2.node(), secondAnchor2.node());
    CORRADE_COMPARE(second1, secondAnchor1.node());
    CORRADE_COMPARE(second2, secondAnchor2.node());
    CORRADE_COMPARE(second1.layout(), layoutHandle(ui.snapLayouter().handle(), 5, 1));
    CORRADE_COMPARE(second2.layout(), layoutHandle(ui.snapLayouter().handle(), 6, 1));
    CORRADE_COMPARE(second1, second1.layout());
    CORRADE_COMPARE(second2, second2.layout());
    CORRADE_VERIFY(ui.snapLayouter().isHandleValid(second1));
    CORRADE_VERIFY(ui.snapLayouter().isHandleValid(second2));
    CORRADE_VERIFY(!ui.snapLayouter().hasExplicitSnap(second1));
    CORRADE_VERIFY(!ui.snapLayouter().hasExplicitSnap(second2));
    CORRADE_COMPARE(ui.snapLayouter().node(second1), secondAnchor1.node());
    CORRADE_COMPARE(ui.snapLayouter().node(second2), secondAnchor2.node());
    CORRADE_COMPARE(ui.snapLayouter().flags(second1), SnapLayoutFlag::PropagateMarginY);
    CORRADE_COMPARE(ui.snapLayouter().flags(second2), SnapLayoutFlag::PropagateMarginY);
    CORRADE_COMPARE(ui.snapLayouter().childSnap(second1), Snap::Bottom);
    CORRADE_COMPARE(ui.snapLayouter().childSnap(second2), Snap::Bottom);

    /* "Before" layout as a LayouterDataHandle */
    SnapLayout third1{thirdAnchor1, layoutHandleData(first1), SnapLayoutFlag::IgnoreOverflow};
    SnapLayout third2{ui, thirdAnchor2.node(), layoutHandleData(first1), SnapLayoutFlag::IgnoreOverflow};
    CORRADE_COMPARE(&third1.ui(), &ui);
    CORRADE_COMPARE(&third2.ui(), &ui);
    CORRADE_COMPARE(&third1.layouter(), &ui.snapLayouter());
    CORRADE_COMPARE(&third2.layouter(), &ui.snapLayouter());
    CORRADE_COMPARE(third1.node(), thirdAnchor1.node());
    CORRADE_COMPARE(third2.node(), thirdAnchor2.node());
    CORRADE_COMPARE(third1, thirdAnchor1.node());
    CORRADE_COMPARE(third2, thirdAnchor2.node());
    CORRADE_COMPARE(third1.layout(), layoutHandle(ui.snapLayouter().handle(), 7, 1));
    CORRADE_COMPARE(third2.layout(), layoutHandle(ui.snapLayouter().handle(), 8, 1));
    CORRADE_COMPARE(third1, third1.layout());
    CORRADE_COMPARE(third2, third2.layout());
    CORRADE_VERIFY(ui.snapLayouter().isHandleValid(third1));
    CORRADE_VERIFY(ui.snapLayouter().isHandleValid(third2));
    CORRADE_VERIFY(!ui.snapLayouter().hasExplicitSnap(third1));
    CORRADE_VERIFY(!ui.snapLayouter().hasExplicitSnap(third2));
    CORRADE_COMPARE(ui.snapLayouter().node(third1), thirdAnchor1.node());
    CORRADE_COMPARE(ui.snapLayouter().node(third2), thirdAnchor2.node());
    CORRADE_COMPARE(ui.snapLayouter().flags(third1), SnapLayoutFlag::IgnoreOverflow);
    CORRADE_COMPARE(ui.snapLayouter().flags(third2), SnapLayoutFlag::IgnoreOverflow);
    CORRADE_COMPARE(ui.snapLayouter().childSnap(third1), Snap::Bottom);
    CORRADE_COMPARE(ui.snapLayouter().childSnap(third2), Snap::Bottom);

    /* Verify the order arguments were actually used */
    CORRADE_COMPARE(ui.snapLayouter().firstChild(root), second1);
    CORRADE_COMPARE(ui.snapLayouter().next(second1), second2);
    CORRADE_COMPARE(ui.snapLayouter().next(second2), third1);
    CORRADE_COMPARE(ui.snapLayouter().next(third1), third2);
    CORRADE_COMPARE(ui.snapLayouter().next(third2), first1);
    CORRADE_COMPARE(ui.snapLayouter().next(first1), first2);
    CORRADE_COMPARE(ui.snapLayouter().next(first2), LayoutHandle::Null);

    /* There should be no additional layouts created besides the ones known
       above */
    CORRADE_COMPARE(ui.snapLayouter().usedCount(), 3 + 2*3);
}

template<class T> void SnapLayoutTest::constructCreateLayoutExplicitSnap() {
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

    /* Target layout to snap to */
    NodeHandle targetNode = ui.createNode({}, {});
    LayoutHandle target = layouter.add(targetNode);

    typename SnapLayoutTraits<T>::AnchorType firstAnchor1{ui, {}, {}};
    typename SnapLayoutTraits<T>::AnchorType firstAnchor2{ui, {}, {}};
    typename SnapLayoutTraits<T>::AnchorType secondAnchor1{ui, {}, {}};
    typename SnapLayoutTraits<T>::AnchorType secondAnchor2{ui, {}, {}};
    CORRADE_COMPARE(firstAnchor1.node(), nodeHandle(4, 1));
    CORRADE_COMPARE(firstAnchor2.node(), nodeHandle(5, 1));
    CORRADE_COMPARE(secondAnchor1.node(), nodeHandle(6, 1));
    CORRADE_COMPARE(secondAnchor2.node(), nodeHandle(7, 1));

    /* LayoutHandle snap target */
    T first1{layouter, firstAnchor1, Snap::Top|Snap::NoPad, target, SnapLayoutFlag::IgnoreOverflowX};
    T first2{ui, layouter, firstAnchor2.node(), Snap::Top|Snap::NoPad, target, SnapLayoutFlag::IgnoreOverflowX};
    CORRADE_COMPARE(&first1.ui(), &ui);
    CORRADE_COMPARE(&first2.ui(), &ui);
    CORRADE_COMPARE(&first1.layouter(), &layouter);
    CORRADE_COMPARE(&first2.layouter(), &layouter);
    CORRADE_COMPARE(first1.node(), firstAnchor1.node());
    CORRADE_COMPARE(first2.node(), firstAnchor2.node());
    CORRADE_COMPARE(first1, firstAnchor1.node());
    CORRADE_COMPARE(first2, firstAnchor2.node());
    CORRADE_COMPARE(first1.layout(), layoutHandle(layouter.handle(), 3, 1));
    CORRADE_COMPARE(first2.layout(), layoutHandle(layouter.handle(), 4, 1));
    CORRADE_COMPARE(first1, first1.layout());
    CORRADE_COMPARE(first2, first2.layout());
    CORRADE_VERIFY(layouter.isHandleValid(first1));
    CORRADE_VERIFY(layouter.isHandleValid(first2));
    CORRADE_VERIFY(layouter.hasExplicitSnap(first1));
    CORRADE_VERIFY(layouter.hasExplicitSnap(first2));
    CORRADE_COMPARE(layouter.node(first1), firstAnchor1.node());
    CORRADE_COMPARE(layouter.node(first2), firstAnchor2.node());
    CORRADE_COMPARE(layouter.flags(first1), SnapLayoutFlag::IgnoreOverflowX);
    CORRADE_COMPARE(layouter.flags(first2), SnapLayoutFlag::IgnoreOverflowX);
    CORRADE_COMPARE(layouter.explicitSnap(first1), Snap::Top|Snap::NoPad);
    CORRADE_COMPARE(layouter.explicitSnap(first2), Snap::Top|Snap::NoPad);
    CORRADE_COMPARE(layouter.explicitSnapTarget(first1), target);
    CORRADE_COMPARE(layouter.explicitSnapTarget(first2), target);
    CORRADE_COMPARE(layouter.childSnap(first1), Snap::Bottom);
    CORRADE_COMPARE(layouter.childSnap(first2), Snap::Bottom);

    /* LayouterDataHandle snap target */
    T second1{layouter, secondAnchor1, Snap::BottomRight, layoutHandleData(target), SnapLayoutFlag::PropagateMarginY};
    T second2{ui, layouter, secondAnchor2.node(), Snap::BottomRight, layoutHandleData(target), SnapLayoutFlag::PropagateMarginY};
    CORRADE_COMPARE(&second1.ui(), &ui);
    CORRADE_COMPARE(&second2.ui(), &ui);
    CORRADE_COMPARE(&second1.layouter(), &layouter);
    CORRADE_COMPARE(&second2.layouter(), &layouter);
    CORRADE_COMPARE(second1.node(), secondAnchor1.node());
    CORRADE_COMPARE(second2.node(), secondAnchor2.node());
    CORRADE_COMPARE(second1, secondAnchor1.node());
    CORRADE_COMPARE(second2, secondAnchor2.node());
    CORRADE_COMPARE(second1.layout(), layoutHandle(layouter.handle(), 5, 1));
    CORRADE_COMPARE(second2.layout(), layoutHandle(layouter.handle(), 6, 1));
    CORRADE_COMPARE(second1, second1.layout());
    CORRADE_COMPARE(second2, second2.layout());
    CORRADE_VERIFY(layouter.isHandleValid(second1));
    CORRADE_VERIFY(layouter.isHandleValid(second2));
    CORRADE_VERIFY(layouter.hasExplicitSnap(second1));
    CORRADE_VERIFY(layouter.hasExplicitSnap(second2));
    CORRADE_COMPARE(layouter.node(second1), secondAnchor1.node());
    CORRADE_COMPARE(layouter.node(second2), secondAnchor2.node());
    CORRADE_COMPARE(layouter.flags(second1), SnapLayoutFlag::PropagateMarginY);
    CORRADE_COMPARE(layouter.flags(second2), SnapLayoutFlag::PropagateMarginY);
    CORRADE_COMPARE(layouter.explicitSnap(second1), Snap::BottomRight);
    CORRADE_COMPARE(layouter.explicitSnap(second2), Snap::BottomRight);
    CORRADE_COMPARE(layouter.explicitSnapTarget(second1), target);
    CORRADE_COMPARE(layouter.explicitSnapTarget(second2), target);
    CORRADE_COMPARE(layouter.childSnap(second1), Snap::Bottom);
    CORRADE_COMPARE(layouter.childSnap(second2), Snap::Bottom);

    /* There should be no additional layouts created besides the ones known
       above */
    CORRADE_COMPARE(layouter.usedCount(), 3 + 2*2);
}

void SnapLayoutTest::constructCreateLayoutExplicitSnapImplicitLayouter() {
    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};

    ui.setSnapLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));

    /* Create some extra nodes and layouts to not have the anchor and layout
       with trivial handles */
    ui.snapLayouter().add(ui.createNode({}, {}));
    ui.snapLayouter().add(ui.createNode({}, {}));
    ui.createNode({}, {});

    /* Target layout to snap to */
    NodeHandle targetNode = ui.createNode({}, {});
    LayoutHandle target = ui.snapLayouter().add(targetNode);

    Anchor firstAnchor1{ui, {}, {}};
    Anchor firstAnchor2{ui, {}, {}};
    Anchor secondAnchor1{ui, {}, {}};
    Anchor secondAnchor2{ui, {}, {}};
    CORRADE_COMPARE(firstAnchor1.node(), nodeHandle(4, 1));
    CORRADE_COMPARE(firstAnchor2.node(), nodeHandle(5, 1));
    CORRADE_COMPARE(secondAnchor1.node(), nodeHandle(6, 1));
    CORRADE_COMPARE(secondAnchor2.node(), nodeHandle(7, 1));

    /* LayoutHandle snap target */
    SnapLayout first1{firstAnchor1, Snap::Top|Snap::NoPad, target, SnapLayoutFlag::IgnoreOverflowX};
    SnapLayout first2{ui, firstAnchor2.node(), Snap::Top|Snap::NoPad, target, SnapLayoutFlag::IgnoreOverflowX};
    CORRADE_COMPARE(&first1.ui(), &ui);
    CORRADE_COMPARE(&first2.ui(), &ui);
    CORRADE_COMPARE(&first1.layouter(), &ui.snapLayouter());
    CORRADE_COMPARE(&first2.layouter(), &ui.snapLayouter());
    CORRADE_COMPARE(first1.node(), firstAnchor1.node());
    CORRADE_COMPARE(first2.node(), firstAnchor2.node());
    CORRADE_COMPARE(first1, firstAnchor1.node());
    CORRADE_COMPARE(first2, firstAnchor2.node());
    CORRADE_COMPARE(first1.layout(), layoutHandle(ui.snapLayouter().handle(), 3, 1));
    CORRADE_COMPARE(first2.layout(), layoutHandle(ui.snapLayouter().handle(), 4, 1));
    CORRADE_COMPARE(first1, first1.layout());
    CORRADE_COMPARE(first2, first2.layout());
    CORRADE_VERIFY(ui.snapLayouter().isHandleValid(first1));
    CORRADE_VERIFY(ui.snapLayouter().isHandleValid(first2));
    CORRADE_VERIFY(ui.snapLayouter().hasExplicitSnap(first1));
    CORRADE_VERIFY(ui.snapLayouter().hasExplicitSnap(first2));
    CORRADE_COMPARE(ui.snapLayouter().node(first1), firstAnchor1.node());
    CORRADE_COMPARE(ui.snapLayouter().node(first2), firstAnchor2.node());
    CORRADE_COMPARE(ui.snapLayouter().flags(first1), SnapLayoutFlag::IgnoreOverflowX);
    CORRADE_COMPARE(ui.snapLayouter().flags(first2), SnapLayoutFlag::IgnoreOverflowX);
    CORRADE_COMPARE(ui.snapLayouter().explicitSnap(first1), Snap::Top|Snap::NoPad);
    CORRADE_COMPARE(ui.snapLayouter().explicitSnap(first2), Snap::Top|Snap::NoPad);
    CORRADE_COMPARE(ui.snapLayouter().explicitSnapTarget(first1), target);
    CORRADE_COMPARE(ui.snapLayouter().explicitSnapTarget(first2), target);
    CORRADE_COMPARE(ui.snapLayouter().childSnap(first1), Snap::Bottom);
    CORRADE_COMPARE(ui.snapLayouter().childSnap(first2), Snap::Bottom);

    /* LayouterDataHandle snap target */
    SnapLayout second1{secondAnchor1, Snap::BottomRight, layoutHandleData(target), SnapLayoutFlag::PropagateMarginY};
    SnapLayout second2{ui, secondAnchor2.node(), Snap::BottomRight, layoutHandleData(target), SnapLayoutFlag::PropagateMarginY};
    CORRADE_COMPARE(&second1.ui(), &ui);
    CORRADE_COMPARE(&second2.ui(), &ui);
    CORRADE_COMPARE(&second1.layouter(), &ui.snapLayouter());
    CORRADE_COMPARE(&second2.layouter(), &ui.snapLayouter());
    CORRADE_COMPARE(second1.node(), secondAnchor1.node());
    CORRADE_COMPARE(second2.node(), secondAnchor2.node());
    CORRADE_COMPARE(second1, secondAnchor1.node());
    CORRADE_COMPARE(second2, secondAnchor2.node());
    CORRADE_COMPARE(second1.layout(), layoutHandle(ui.snapLayouter().handle(), 5, 1));
    CORRADE_COMPARE(second2.layout(), layoutHandle(ui.snapLayouter().handle(), 6, 1));
    CORRADE_COMPARE(second1, second1.layout());
    CORRADE_COMPARE(second2, second2.layout());
    CORRADE_VERIFY(ui.snapLayouter().isHandleValid(second1));
    CORRADE_VERIFY(ui.snapLayouter().isHandleValid(second2));
    CORRADE_VERIFY(ui.snapLayouter().hasExplicitSnap(second1));
    CORRADE_VERIFY(ui.snapLayouter().hasExplicitSnap(second2));
    CORRADE_COMPARE(ui.snapLayouter().node(second1), secondAnchor1.node());
    CORRADE_COMPARE(ui.snapLayouter().node(second2), secondAnchor2.node());
    CORRADE_COMPARE(ui.snapLayouter().flags(second1), SnapLayoutFlag::PropagateMarginY);
    CORRADE_COMPARE(ui.snapLayouter().flags(second2), SnapLayoutFlag::PropagateMarginY);
    CORRADE_COMPARE(ui.snapLayouter().explicitSnap(second1), Snap::BottomRight);
    CORRADE_COMPARE(ui.snapLayouter().explicitSnap(second2), Snap::BottomRight);
    CORRADE_COMPARE(ui.snapLayouter().explicitSnapTarget(second1), target);
    CORRADE_COMPARE(ui.snapLayouter().explicitSnapTarget(second2), target);
    CORRADE_COMPARE(ui.snapLayouter().childSnap(second1), Snap::Bottom);
    CORRADE_COMPARE(ui.snapLayouter().childSnap(second2), Snap::Bottom);

    /* There should be no additional layouts created besides the ones known
       above */
    CORRADE_COMPARE(ui.snapLayouter().usedCount(), 3 + 2*2);
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
    /* Layouter with a handle that isn't even valid in the UI, to verify it
       isn't blindly accessng it */
    SnapLayouter layouterInvalidHandle{layouterHandle(0xab, 0x12)};
    /* Layouter with the same handle as the right one, but in another UI, to
       verify it's checking the actual instance as well */
    SnapLayouter& layouterAnotherUi = anotherUi.setLayouterInstance(Containers::pointer<SnapLayouter>(anotherUi.createLayouter()));
    CORRADE_COMPARE(layouterAnotherUi.handle(), layouter.handle());

    /* Create some extra nodes and layouts to not have the anchor and layout
       with trivial handles */
    layouter.add(ui.createNode({}, {}));
    layouter.remove(layouter.add(ui.createNode({}, {})));
    ui.createNode({}, {});
    ui.removeNode(ui.createNode({}, {}));
    ui.removeNode(ui.createNode({}, {}));
    ui.removeNode(ui.createNode({}, {}));

    typename SnapLayoutTraits<T>::AnchorType anchorLayoutAlreadyPresent{ui, {}, {}};
    layouter.add(anchorLayoutAlreadyPresent);
    typename SnapLayoutTraits<T>::AnchorType anchor{ui, {}, {}};

    Containers::String out;
    Error redirectError{&out};
    /* Layouter handle not valid in the anchor UI */
    T{layouterInvalidHandle, anchor};
    T{layouterInvalidHandle, anchor, LayoutHandle::Null};
    T{layouterInvalidHandle, anchor, LayouterDataHandle::Null};
    T{layouterInvalidHandle, anchor, SnapLayoutFlags{}};
    T{layouterInvalidHandle, anchor, Snaps{}, LayoutHandle::Null};
    T{layouterInvalidHandle, anchor, Snaps{}, LayouterDataHandle::Null};
    /* Same, but with a node instead of an anchor */
    T{ui, layouterInvalidHandle, anchor.node()};
    T{ui, layouterInvalidHandle, anchor.node(), LayoutHandle::Null};
    T{ui, layouterInvalidHandle, anchor.node(), LayouterDataHandle::Null};
    T{ui, layouterInvalidHandle, anchor.node(), SnapLayoutFlags{}};
    T{ui, layouterInvalidHandle, anchor.node(), Snaps{}, LayoutHandle::Null};
    T{ui, layouterInvalidHandle, anchor.node(), Snaps{}, LayouterDataHandle::Null};
    /* Valid handle, but different instance */
    T{layouterAnotherUi, anchor};
    T{layouterAnotherUi, anchor, LayoutHandle::Null};
    T{layouterAnotherUi, anchor, LayouterDataHandle::Null};
    T{layouterAnotherUi, anchor, SnapLayoutFlags{}};
    T{layouterAnotherUi, anchor, Snaps{}, LayoutHandle::Null};
    T{layouterAnotherUi, anchor, Snaps{}, LayouterDataHandle::Null};
    /* Same, but with a node instead of an anchor */
    T{ui, layouterAnotherUi, anchor.node()};
    T{ui, layouterAnotherUi, anchor.node(), LayoutHandle::Null};
    T{ui, layouterAnotherUi, anchor.node(), LayouterDataHandle::Null};
    T{ui, layouterAnotherUi, anchor.node(), SnapLayoutFlags{}};
    T{ui, layouterAnotherUi, anchor.node(), Snaps{}, LayoutHandle::Null};
    T{ui, layouterAnotherUi, anchor.node(), Snaps{}, LayouterDataHandle::Null};
    /* Layout already present. The layouter + anchor / ui + layouter + node
       constructor is the only which allows this. */
    T{layouter, anchorLayoutAlreadyPresent, LayoutHandle::Null};
    T{layouter, anchorLayoutAlreadyPresent, LayouterDataHandle::Null};
    T{layouter, anchorLayoutAlreadyPresent, SnapLayoutFlags{}};
    T{layouter, anchorLayoutAlreadyPresent, Snaps{}, LayoutHandle::Null};
    T{layouter, anchorLayoutAlreadyPresent, Snaps{}, LayouterDataHandle::Null};
    /* Same, but with a node instead of an anchor */
    T{ui, layouter, anchorLayoutAlreadyPresent.node(), LayoutHandle::Null};
    T{ui, layouter, anchorLayoutAlreadyPresent.node(), LayouterDataHandle::Null};
    T{ui, layouter, anchorLayoutAlreadyPresent.node(), SnapLayoutFlags{}};
    T{ui, layouter, anchorLayoutAlreadyPresent.node(), Snaps{}, LayoutHandle::Null};
    T{ui, layouter, anchorLayoutAlreadyPresent.node(), Snaps{}, LayouterDataHandle::Null};
    CORRADE_COMPARE_AS(out,
        "Ui::AbstractSnapLayout: layouter and anchor not part of the same UI\n"
        "Ui::AbstractSnapLayout: layouter and anchor not part of the same UI\n"
        "Ui::AbstractSnapLayout: layouter and anchor not part of the same UI\n"
        "Ui::AbstractSnapLayout: layouter and anchor not part of the same UI\n"
        "Ui::AbstractSnapLayout: layouter and anchor not part of the same UI\n"
        "Ui::AbstractSnapLayout: layouter and anchor not part of the same UI\n"

        "Ui::AbstractSnapLayout: layouter not part of the UI\n"
        "Ui::AbstractSnapLayout: layouter not part of the UI\n"
        "Ui::AbstractSnapLayout: layouter not part of the UI\n"
        "Ui::AbstractSnapLayout: layouter not part of the UI\n"
        "Ui::AbstractSnapLayout: layouter not part of the UI\n"
        "Ui::AbstractSnapLayout: layouter not part of the UI\n"

        "Ui::AbstractSnapLayout: layouter and anchor not part of the same UI\n"
        "Ui::AbstractSnapLayout: layouter and anchor not part of the same UI\n"
        "Ui::AbstractSnapLayout: layouter and anchor not part of the same UI\n"
        "Ui::AbstractSnapLayout: layouter and anchor not part of the same UI\n"
        "Ui::AbstractSnapLayout: layouter and anchor not part of the same UI\n"
        "Ui::AbstractSnapLayout: layouter and anchor not part of the same UI\n"

        "Ui::AbstractSnapLayout: layouter not part of the UI\n"
        "Ui::AbstractSnapLayout: layouter not part of the UI\n"
        "Ui::AbstractSnapLayout: layouter not part of the UI\n"
        "Ui::AbstractSnapLayout: layouter not part of the UI\n"
        "Ui::AbstractSnapLayout: layouter not part of the UI\n"
        "Ui::AbstractSnapLayout: layouter not part of the UI\n"

        "Ui::AbstractSnapLayout: Ui::NodeHandle(0x3, 0x4) already has Ui::LayoutHandle({0x2, 0x3}, {0x1, 0x2}) assigned\n"
        "Ui::AbstractSnapLayout: Ui::NodeHandle(0x3, 0x4) already has Ui::LayoutHandle({0x2, 0x3}, {0x1, 0x2}) assigned\n"
        "Ui::AbstractSnapLayout: Ui::NodeHandle(0x3, 0x4) already has Ui::LayoutHandle({0x2, 0x3}, {0x1, 0x2}) assigned\n"
        "Ui::AbstractSnapLayout: Ui::NodeHandle(0x3, 0x4) already has Ui::LayoutHandle({0x2, 0x3}, {0x1, 0x2}) assigned\n"
        "Ui::AbstractSnapLayout: Ui::NodeHandle(0x3, 0x4) already has Ui::LayoutHandle({0x2, 0x3}, {0x1, 0x2}) assigned\n"

        "Ui::AbstractSnapLayout: Ui::NodeHandle(0x3, 0x4) already has Ui::LayoutHandle({0x2, 0x3}, {0x1, 0x2}) assigned\n"
        "Ui::AbstractSnapLayout: Ui::NodeHandle(0x3, 0x4) already has Ui::LayoutHandle({0x2, 0x3}, {0x1, 0x2}) assigned\n"
        "Ui::AbstractSnapLayout: Ui::NodeHandle(0x3, 0x4) already has Ui::LayoutHandle({0x2, 0x3}, {0x1, 0x2}) assigned\n"
        "Ui::AbstractSnapLayout: Ui::NodeHandle(0x3, 0x4) already has Ui::LayoutHandle({0x2, 0x3}, {0x1, 0x2}) assigned\n"
        "Ui::AbstractSnapLayout: Ui::NodeHandle(0x3, 0x4) already has Ui::LayoutHandle({0x2, 0x3}, {0x1, 0x2}) assigned\n",
        TestSuite::Compare::String);
}

void SnapLayoutTest::constructInvalidImplicitLayouter() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate},
      uiNoSnapLayouter{NoCreate};

    /* Create some extra layouters to not have it with a trivial handle */
    ui.createLayouter();
    ui.createLayouter();
    ui.removeLayouter(ui.createLayouter());
    ui.removeLayouter(ui.createLayouter());

    ui.setSnapLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));

    /* Create some extra nodes and layouts to not have the anchor and layout
       with trivial handles */
    ui.snapLayouter().add(ui.createNode({}, {}));
    ui.snapLayouter().remove(ui.snapLayouter().add(ui.createNode({}, {})));
    ui.createNode({}, {});
    ui.removeNode(ui.createNode({}, {}));
    ui.removeNode(ui.createNode({}, {}));
    ui.removeNode(ui.createNode({}, {}));

    Anchor anchorLayoutAlreadyPresent{ui, {}, {}};
    ui.snapLayouter().add(anchorLayoutAlreadyPresent);
    Anchor anchorUiNoSnapLayouter{uiNoSnapLayouter, {}, {}};

    Containers::String out;
    Error redirectError{&out};
    /* Layouter not present */
    SnapLayout{anchorUiNoSnapLayouter};
    SnapLayout{anchorUiNoSnapLayouter, LayoutHandle::Null};
    SnapLayout{anchorUiNoSnapLayouter, LayouterDataHandle::Null};
    SnapLayout{anchorUiNoSnapLayouter, SnapLayoutFlags{}};
    SnapLayout{anchorUiNoSnapLayouter, Snaps{}, LayoutHandle::Null};
    SnapLayout{anchorUiNoSnapLayouter, Snaps{}, LayouterDataHandle::Null};
    /* Same, but with a node instead of an anchor */
    SnapLayout{uiNoSnapLayouter, anchorUiNoSnapLayouter.node()};
    SnapLayout{uiNoSnapLayouter, anchorUiNoSnapLayouter.node(), LayoutHandle::Null};
    SnapLayout{uiNoSnapLayouter, anchorUiNoSnapLayouter.node(), LayouterDataHandle::Null};
    SnapLayout{uiNoSnapLayouter, anchorUiNoSnapLayouter.node(), SnapLayoutFlags{}};
    SnapLayout{uiNoSnapLayouter, anchorUiNoSnapLayouter.node(), Snaps{}, LayoutHandle::Null};
    SnapLayout{uiNoSnapLayouter, anchorUiNoSnapLayouter.node(), Snaps{}, LayouterDataHandle::Null};
    /* Layout already present. The layouter + anchor / ui + layouter + node
       constructor is the only which allows this. */
    SnapLayout{anchorLayoutAlreadyPresent, LayoutHandle::Null};
    SnapLayout{anchorLayoutAlreadyPresent, LayouterDataHandle::Null};
    SnapLayout{anchorLayoutAlreadyPresent, SnapLayoutFlags{}};
    SnapLayout{anchorLayoutAlreadyPresent, Snaps{}, LayoutHandle::Null};
    SnapLayout{anchorLayoutAlreadyPresent, Snaps{}, LayouterDataHandle::Null};
    /* Same, but with a node instead of an anchor */
    SnapLayout{ui, anchorLayoutAlreadyPresent.node(), LayoutHandle::Null};
    SnapLayout{ui, anchorLayoutAlreadyPresent.node(), LayouterDataHandle::Null};
    SnapLayout{ui, anchorLayoutAlreadyPresent.node(), SnapLayoutFlags{}};
    SnapLayout{ui, anchorLayoutAlreadyPresent.node(), Snaps{}, LayoutHandle::Null};
    SnapLayout{ui, anchorLayoutAlreadyPresent.node(), Snaps{}, LayouterDataHandle::Null};
    CORRADE_COMPARE_AS(out,
        "Ui::BasicSnapLayout: SnapLayouter not present in the UI\n"
        "Ui::BasicSnapLayout: SnapLayouter not present in the UI\n"
        "Ui::BasicSnapLayout: SnapLayouter not present in the UI\n"
        "Ui::BasicSnapLayout: SnapLayouter not present in the UI\n"
        "Ui::BasicSnapLayout: SnapLayouter not present in the UI\n"
        "Ui::BasicSnapLayout: SnapLayouter not present in the UI\n"

        "Ui::BasicSnapLayout: SnapLayouter not present in the UI\n"
        "Ui::BasicSnapLayout: SnapLayouter not present in the UI\n"
        "Ui::BasicSnapLayout: SnapLayouter not present in the UI\n"
        "Ui::BasicSnapLayout: SnapLayouter not present in the UI\n"
        "Ui::BasicSnapLayout: SnapLayouter not present in the UI\n"
        "Ui::BasicSnapLayout: SnapLayouter not present in the UI\n"

        "Ui::AbstractSnapLayout: Ui::NodeHandle(0x3, 0x4) already has Ui::LayoutHandle({0x2, 0x3}, {0x1, 0x2}) assigned\n"
        "Ui::AbstractSnapLayout: Ui::NodeHandle(0x3, 0x4) already has Ui::LayoutHandle({0x2, 0x3}, {0x1, 0x2}) assigned\n"
        "Ui::AbstractSnapLayout: Ui::NodeHandle(0x3, 0x4) already has Ui::LayoutHandle({0x2, 0x3}, {0x1, 0x2}) assigned\n"
        "Ui::AbstractSnapLayout: Ui::NodeHandle(0x3, 0x4) already has Ui::LayoutHandle({0x2, 0x3}, {0x1, 0x2}) assigned\n"
        "Ui::AbstractSnapLayout: Ui::NodeHandle(0x3, 0x4) already has Ui::LayoutHandle({0x2, 0x3}, {0x1, 0x2}) assigned\n"

        "Ui::AbstractSnapLayout: Ui::NodeHandle(0x3, 0x4) already has Ui::LayoutHandle({0x2, 0x3}, {0x1, 0x2}) assigned\n"
        "Ui::AbstractSnapLayout: Ui::NodeHandle(0x3, 0x4) already has Ui::LayoutHandle({0x2, 0x3}, {0x1, 0x2}) assigned\n"
        "Ui::AbstractSnapLayout: Ui::NodeHandle(0x3, 0x4) already has Ui::LayoutHandle({0x2, 0x3}, {0x1, 0x2}) assigned\n"
        "Ui::AbstractSnapLayout: Ui::NodeHandle(0x3, 0x4) already has Ui::LayoutHandle({0x2, 0x3}, {0x1, 0x2}) assigned\n"
        "Ui::AbstractSnapLayout: Ui::NodeHandle(0x3, 0x4) already has Ui::LayoutHandle({0x2, 0x3}, {0x1, 0x2}) assigned\n",
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
    SnapLayout layoutFlagsX{Anchor{ui, {}, Vector2{}}, SnapLayoutFlag::IgnoreOverflowX};
    SnapLayout layoutFlagsY{Anchor{ui, {}, Vector2{}}, SnapLayoutFlag::IgnoreOverflowY};
    SnapLayout layoutFlags{Anchor{ui, {}, Vector2{}}, SnapLayoutFlag::IgnoreOverflow};
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

    T layout{layouter, typename SnapLayoutTraits<T>::AnchorType{ui, {}, {}}, SnapLayoutFlag::PropagateMarginY};
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

template<class T> void SnapLayoutTest::staticChild() {
    setTestCaseTemplateName(SnapLayoutTraits<T>::name());

    /* Basically like SnapLayoutTest::child() but using the snapChild() API */

    struct Interface: SnapLayoutTraits<T>::UserInterfaceType {
        explicit Interface(NoCreateT): SnapLayoutTraits<T>::UserInterfaceType{NoCreate} {}
    } ui{NoCreate};

    SnapLayouter& layouter = ui.setLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));

    /* Create some extra nodes and layouts to not have the anchor and layout
       with trivial handles */
    layouter.add(ui.createNode({}, {}));
    layouter.add(ui.createNode({}, {}));
    ui.createNode({}, {});

    typename SnapLayoutTraits<T>::AnchorType anchor{ui, {}, {}};
    LayoutHandle layout = layouter.add(anchor);
    CORRADE_COMPARE(anchor.node(), nodeHandle(3, 1));
    CORRADE_COMPARE(layout, layoutHandle(layouter.handle(), 2, 1));

    /* No "before" layout */
    T child1 = T::child(layouter, anchor, {3.0f, 5.0f}, NodeFlag::NoEvents, SnapLayoutFlag::IgnoreOverflowY);
    CORRADE_COMPARE(&child1.ui(), &ui);
    CORRADE_COMPARE(&child1.layouter(), &layouter);
    CORRADE_COMPARE(child1.node(), nodeHandle(4, 1));
    CORRADE_COMPARE(child1.layout(), layoutHandle(layouter.handle(), 3, 1));
    CORRADE_VERIFY(ui.isHandleValid(child1.node()));
    CORRADE_COMPARE(ui.nodeParent(child1), anchor);
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
    T child2 = T::child(layouter, anchor, {4.0f, 6.0f}, SnapLayoutFlag::PropagateMarginX);
    CORRADE_COMPARE(&child2.ui(), &ui);
    CORRADE_COMPARE(&child2.layouter(), &layouter);
    CORRADE_COMPARE(child2.node(), nodeHandle(5, 1));
    CORRADE_COMPARE(child2.layout(), layoutHandle(layouter.handle(), 4, 1));
    CORRADE_VERIFY(ui.isHandleValid(child2.node()));
    CORRADE_COMPARE(ui.nodeParent(child2), anchor);
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
    T child3 = T::child(layouter, anchor, {5.0f, 7.0f}, NodeFlag::FallthroughPointerEvents, child2, SnapLayoutFlag::IgnoreOverflowX);
    CORRADE_COMPARE(&child3.ui(), &ui);
    CORRADE_COMPARE(&child3.layouter(), &layouter);
    CORRADE_COMPARE(child3.node(), nodeHandle(6, 1));
    CORRADE_COMPARE(child3.layout(), layoutHandle(layouter.handle(), 5, 1));
    CORRADE_VERIFY(ui.isHandleValid(child3.node()));
    CORRADE_COMPARE(ui.nodeParent(child3), anchor);
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
    T child4 = T::child(layouter, anchor, {6.0f, 8.0f}, child1, SnapLayoutFlag::IgnoreOverflow);
    CORRADE_COMPARE(&child4.ui(), &ui);
    CORRADE_COMPARE(&child4.layouter(), &layouter);
    CORRADE_COMPARE(child4.node(), nodeHandle(7, 1));
    CORRADE_COMPARE(child4.layout(), layoutHandle(layouter.handle(), 6, 1));
    CORRADE_VERIFY(ui.isHandleValid(child4.node()));
    CORRADE_COMPARE(ui.nodeParent(child4), anchor);
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
    T child5 = T::child(layouter, anchor, {7.0f, 9.0f}, NodeFlag::Focusable, layoutHandleData(child3), SnapLayoutFlag::PropagateMargin);
    CORRADE_COMPARE(&child5.ui(), &ui);
    CORRADE_COMPARE(&child5.layouter(), &layouter);
    CORRADE_COMPARE(child5.node(), nodeHandle(8, 1));
    CORRADE_COMPARE(child5.layout(), layoutHandle(layouter.handle(), 7, 1));
    CORRADE_VERIFY(ui.isHandleValid(child5.node()));
    CORRADE_COMPARE(ui.nodeParent(child5), anchor);
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
    T child6 = T::child(layouter, anchor, {8.0f, 1.0f}, layoutHandleData(child1), SnapLayoutFlag::PropagateMarginY);
    CORRADE_COMPARE(&child6.ui(), &ui);
    CORRADE_COMPARE(&child6.layouter(), &layouter);
    CORRADE_COMPARE(child6.node(), nodeHandle(9, 1));
    CORRADE_COMPARE(child6.layout(), layoutHandle(layouter.handle(), 8, 1));
    CORRADE_VERIFY(ui.isHandleValid(child6.node()));
    CORRADE_COMPARE(ui.nodeParent(child6), anchor);
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

void SnapLayoutTest::staticChildImplicitLayouter() {
    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};

    ui.setSnapLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));

    /* Create some extra nodes and layouts to not have the anchor and layout
       with trivial handles */
    ui.snapLayouter().add(ui.createNode({}, {}));
    ui.snapLayouter().add(ui.createNode({}, {}));
    ui.createNode({}, {});

    Anchor anchor{ui, {}, {}};
    LayoutHandle layout = ui.snapLayouter().add(anchor);
    CORRADE_COMPARE(anchor.node(), nodeHandle(3, 1));
    CORRADE_COMPARE(layout, layoutHandle(ui.snapLayouter().handle(), 2, 1));

    /* No "before" layout */
    SnapLayout child1 = SnapLayout::child(anchor, {3.0f, 5.0f}, NodeFlag::NoEvents, SnapLayoutFlag::IgnoreOverflowY);
    CORRADE_COMPARE(&child1.ui(), &ui);
    CORRADE_COMPARE(&child1.layouter(), &ui.snapLayouter());
    CORRADE_COMPARE(child1.node(), nodeHandle(4, 1));
    CORRADE_COMPARE(child1.layout(), layoutHandle(ui.snapLayouter().handle(), 3, 1));
    CORRADE_VERIFY(ui.isHandleValid(child1.node()));
    CORRADE_COMPARE(ui.nodeParent(child1), anchor);
    CORRADE_COMPARE(ui.nodeOffset(child1), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(child1), (Vector2{3.0f, 5.0f}));
    CORRADE_COMPARE(ui.nodeFlags(child1), NodeFlag::NoEvents);
    CORRADE_VERIFY(ui.snapLayouter().isHandleValid(child1));
    CORRADE_VERIFY(!ui.snapLayouter().hasExplicitSnap(child1));
    CORRADE_COMPARE(ui.snapLayouter().node(child1), child1.node());
    CORRADE_COMPARE(ui.snapLayouter().parent(child1), layout);
    CORRADE_COMPARE(ui.snapLayouter().flags(child1), SnapLayoutFlag::IgnoreOverflowY);
    CORRADE_COMPARE(ui.snapLayouter().childSnap(child1), Snap::Bottom);

    /* No "before" layout, no NodeFlags */
    SnapLayout child2 = SnapLayout::child(anchor, {4.0f, 6.0f}, SnapLayoutFlag::PropagateMarginX);
    CORRADE_COMPARE(&child2.ui(), &ui);
    CORRADE_COMPARE(&child2.layouter(), &ui.snapLayouter());
    CORRADE_COMPARE(child2.node(), nodeHandle(5, 1));
    CORRADE_COMPARE(child2.layout(), layoutHandle(ui.snapLayouter().handle(), 4, 1));
    CORRADE_VERIFY(ui.isHandleValid(child2.node()));
    CORRADE_COMPARE(ui.nodeParent(child2), anchor);
    CORRADE_COMPARE(ui.nodeOffset(child2), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(child2), (Vector2{4.0f, 6.0f}));
    CORRADE_COMPARE(ui.nodeFlags(child2), NodeFlags{});
    CORRADE_VERIFY(ui.snapLayouter().isHandleValid(child2));
    CORRADE_VERIFY(!ui.snapLayouter().hasExplicitSnap(child2));
    CORRADE_COMPARE(ui.snapLayouter().node(child2), child2.node());
    CORRADE_COMPARE(ui.snapLayouter().parent(child2), layout);
    CORRADE_COMPARE(ui.snapLayouter().flags(child2), SnapLayoutFlag::PropagateMarginX);
    CORRADE_COMPARE(ui.snapLayouter().childSnap(child2), Snap::Bottom);

    /* "Before" layout as LayoutHandle */
    SnapLayout child3 = SnapLayout::child(anchor, {5.0f, 7.0f}, NodeFlag::FallthroughPointerEvents, child2, SnapLayoutFlag::IgnoreOverflowX);
    CORRADE_COMPARE(&child3.ui(), &ui);
    CORRADE_COMPARE(&child3.layouter(), &ui.snapLayouter());
    CORRADE_COMPARE(child3.node(), nodeHandle(6, 1));
    CORRADE_COMPARE(child3.layout(), layoutHandle(ui.snapLayouter().handle(), 5, 1));
    CORRADE_VERIFY(ui.isHandleValid(child3.node()));
    CORRADE_COMPARE(ui.nodeParent(child3), anchor);
    CORRADE_COMPARE(ui.nodeOffset(child3), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(child3), (Vector2{5.0f, 7.0f}));
    CORRADE_COMPARE(ui.nodeFlags(child3), NodeFlag::FallthroughPointerEvents);
    CORRADE_VERIFY(ui.snapLayouter().isHandleValid(child3));
    CORRADE_VERIFY(!ui.snapLayouter().hasExplicitSnap(child3));
    CORRADE_COMPARE(ui.snapLayouter().node(child3), child3.node());
    CORRADE_COMPARE(ui.snapLayouter().parent(child3), layout);
    CORRADE_COMPARE(ui.snapLayouter().flags(child3), SnapLayoutFlag::IgnoreOverflowX);
    CORRADE_COMPARE(ui.snapLayouter().childSnap(child3), Snap::Bottom);

    /* "Before" layout as LayoutHandle, no NodeFlags */
    SnapLayout child4 = SnapLayout::child(anchor, {6.0f, 8.0f}, child1, SnapLayoutFlag::IgnoreOverflow);
    CORRADE_COMPARE(&child4.ui(), &ui);
    CORRADE_COMPARE(&child4.layouter(), &ui.snapLayouter());
    CORRADE_COMPARE(child4.node(), nodeHandle(7, 1));
    CORRADE_COMPARE(child4.layout(), layoutHandle(ui.snapLayouter().handle(), 6, 1));
    CORRADE_VERIFY(ui.isHandleValid(child4.node()));
    CORRADE_COMPARE(ui.nodeParent(child4), anchor);
    CORRADE_COMPARE(ui.nodeOffset(child4), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(child4), (Vector2{6.0f, 8.0f}));
    CORRADE_COMPARE(ui.nodeFlags(child4), NodeFlags{});
    CORRADE_VERIFY(ui.snapLayouter().isHandleValid(child4));
    CORRADE_VERIFY(!ui.snapLayouter().hasExplicitSnap(child4));
    CORRADE_COMPARE(ui.snapLayouter().node(child4), child4.node());
    CORRADE_COMPARE(ui.snapLayouter().parent(child4), layout);
    CORRADE_COMPARE(ui.snapLayouter().flags(child4), SnapLayoutFlag::IgnoreOverflow);
    CORRADE_COMPARE(ui.snapLayouter().childSnap(child4), Snap::Bottom);

    /* "Before" layout as LayouterDataHandle */
    SnapLayout child5 = SnapLayout::child(anchor, {7.0f, 9.0f}, NodeFlag::Focusable, layoutHandleData(child3), SnapLayoutFlag::PropagateMargin);
    CORRADE_COMPARE(&child5.ui(), &ui);
    CORRADE_COMPARE(&child5.layouter(), &ui.snapLayouter());
    CORRADE_COMPARE(child5.node(), nodeHandle(8, 1));
    CORRADE_COMPARE(child5.layout(), layoutHandle(ui.snapLayouter().handle(), 7, 1));
    CORRADE_VERIFY(ui.isHandleValid(child5.node()));
    CORRADE_COMPARE(ui.nodeParent(child5), anchor);
    CORRADE_COMPARE(ui.nodeOffset(child5), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(child5), (Vector2{7.0f, 9.0f}));
    CORRADE_COMPARE(ui.nodeFlags(child5), NodeFlag::Focusable);
    CORRADE_VERIFY(ui.snapLayouter().isHandleValid(child5));
    CORRADE_VERIFY(!ui.snapLayouter().hasExplicitSnap(child5));
    CORRADE_COMPARE(ui.snapLayouter().node(child5), child5.node());
    CORRADE_COMPARE(ui.snapLayouter().parent(child5), layout);
    CORRADE_COMPARE(ui.snapLayouter().flags(child5), SnapLayoutFlag::PropagateMargin);
    CORRADE_COMPARE(ui.snapLayouter().childSnap(child5), Snap::Bottom);

    /* "Before" layout as LayouterDataHandle, no NodeFlags */
    SnapLayout child6 = SnapLayout::child(anchor, {8.0f, 1.0f}, layoutHandleData(child1), SnapLayoutFlag::PropagateMarginY);
    CORRADE_COMPARE(&child6.ui(), &ui);
    CORRADE_COMPARE(&child6.layouter(), &ui.snapLayouter());
    CORRADE_COMPARE(child6.node(), nodeHandle(9, 1));
    CORRADE_COMPARE(child6.layout(), layoutHandle(ui.snapLayouter().handle(), 8, 1));
    CORRADE_VERIFY(ui.isHandleValid(child6.node()));
    CORRADE_COMPARE(ui.nodeParent(child6), anchor);
    CORRADE_COMPARE(ui.nodeOffset(child6), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(child6), (Vector2{8.0f, 1.0f}));
    CORRADE_COMPARE(ui.nodeFlags(child6), NodeFlags{});
    CORRADE_VERIFY(ui.snapLayouter().isHandleValid(child6));
    CORRADE_VERIFY(!ui.snapLayouter().hasExplicitSnap(child6));
    CORRADE_COMPARE(ui.snapLayouter().node(child6), child6.node());
    CORRADE_COMPARE(ui.snapLayouter().parent(child6), layout);
    CORRADE_COMPARE(ui.snapLayouter().flags(child6), SnapLayoutFlag::PropagateMarginY);
    CORRADE_COMPARE(ui.snapLayouter().childSnap(child6), Snap::Bottom);

    /* Verify the order arguments were actually used */
    CORRADE_COMPARE(ui.snapLayouter().firstChild(layout), child4);
    CORRADE_COMPARE(ui.snapLayouter().next(child4), child6);
    CORRADE_COMPARE(ui.snapLayouter().next(child6), child1);
    CORRADE_COMPARE(ui.snapLayouter().next(child1), child5);
    CORRADE_COMPARE(ui.snapLayouter().next(child5), child3);
    CORRADE_COMPARE(ui.snapLayouter().next(child3), child2);
    CORRADE_COMPARE(ui.snapLayouter().next(child2), LayoutHandle::Null);

    /* There should be no additional nodes or layouts created besides the ones
       known above */
    CORRADE_COMPARE(ui.nodeUsedCount(), 10);
    CORRADE_COMPARE(ui.snapLayouter().usedCount(), 9);
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

template<class T> void SnapLayoutTest::staticChildExplicitSnap() {
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

    typename SnapLayoutTraits<T>::AnchorType anchor{ui, {}, {}};
    LayoutHandle layout = layouter.add(anchor);
    CORRADE_COMPARE(anchor.node(), nodeHandle(3, 1));
    CORRADE_COMPARE(layout, layoutHandle(layouter.handle(), 2, 1));

    /* With NodeFlags */
    T child1 = T::child(layouter, Snap::TopRight, anchor, {3.0f, 5.0f}, NodeFlag::Disabled, SnapLayoutFlag::IgnoreOverflowY);
    CORRADE_COMPARE(&child1.ui(), &ui);
    CORRADE_COMPARE(&child1.layouter(), &layouter);
    CORRADE_COMPARE(child1.node(), nodeHandle(4, 1));
    CORRADE_COMPARE(child1.layout(), layoutHandle(layouter.handle(), 3, 1));
    CORRADE_VERIFY(ui.isHandleValid(child1.node()));
    CORRADE_COMPARE(ui.nodeParent(child1), anchor);
    CORRADE_COMPARE(ui.nodeOffset(child1), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(child1), (Vector2{3.0f, 5.0f}));
    CORRADE_COMPARE(ui.nodeFlags(child1), NodeFlag::Disabled);
    CORRADE_VERIFY(layouter.isHandleValid(child1));
    CORRADE_VERIFY(layouter.hasExplicitSnap(child1));
    CORRADE_COMPARE(layouter.node(child1), child1.node());
    CORRADE_COMPARE(layouter.explicitSnap(child1), Snap::TopRight);
    CORRADE_COMPARE(layouter.explicitSnapTarget(child1), layout);
    CORRADE_COMPARE(layouter.flags(child1), SnapLayoutFlag::IgnoreOverflowY);
    CORRADE_COMPARE(layouter.childSnap(child1), Snap::Bottom);

    /* Without NodeFlags */
    T child2 = T::child(layouter, Snap::Fill, anchor, {4.0f, 6.0f}, SnapLayoutFlag::PropagateMarginX);
    CORRADE_COMPARE(&child2.ui(), &ui);
    CORRADE_COMPARE(&child2.layouter(), &layouter);
    CORRADE_COMPARE(child2.node(), nodeHandle(5, 1));
    CORRADE_COMPARE(child2.layout(), layoutHandle(layouter.handle(), 4, 1));
    CORRADE_VERIFY(ui.isHandleValid(child2.node()));
    CORRADE_COMPARE(ui.nodeParent(child2), anchor);
    CORRADE_COMPARE(ui.nodeOffset(child2), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(child2), (Vector2{4.0f, 6.0f}));
    CORRADE_COMPARE(ui.nodeFlags(child2), NodeFlags{});
    CORRADE_VERIFY(layouter.isHandleValid(child2));
    CORRADE_VERIFY(layouter.hasExplicitSnap(child2));
    CORRADE_COMPARE(layouter.node(child2), child2.node());
    CORRADE_COMPARE(layouter.explicitSnap(child2), Snap::Fill);
    CORRADE_COMPARE(layouter.explicitSnapTarget(child2), layout);
    CORRADE_COMPARE(layouter.flags(child2), SnapLayoutFlag::PropagateMarginX);
    CORRADE_COMPARE(layouter.childSnap(child2), Snap::Bottom);

    /* There should be no additional nodes or layouts created besides the ones
       known above */
    CORRADE_COMPARE(ui.nodeUsedCount(), 6);
    CORRADE_COMPARE(layouter.usedCount(), 5);
}

void SnapLayoutTest::statiicChildExplicitSnapImplicitLayouter() {
    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};

    ui.setSnapLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));

    /* Create some extra nodes and layouts to not have the anchor and layout
       with trivial handles */
    ui.snapLayouter().add(ui.createNode({}, {}));
    ui.snapLayouter().add(ui.createNode({}, {}));
    ui.createNode({}, {});

    Anchor anchor{ui, {}, {}};
    LayoutHandle layout = ui.snapLayouter().add(anchor);
    CORRADE_COMPARE(anchor.node(), nodeHandle(3, 1));
    CORRADE_COMPARE(layout, layoutHandle(ui.snapLayouter().handle(), 2, 1));

    /* With NodeFlags */
    SnapLayout child1 = SnapLayout::child(Snap::TopRight, anchor, {3.0f, 5.0f}, NodeFlag::Disabled, SnapLayoutFlag::IgnoreOverflowY);
    CORRADE_COMPARE(&child1.ui(), &ui);
    CORRADE_COMPARE(&child1.layouter(), &ui.snapLayouter());
    CORRADE_COMPARE(child1.node(), nodeHandle(4, 1));
    CORRADE_COMPARE(child1.layout(), layoutHandle(ui.snapLayouter().handle(), 3, 1));
    CORRADE_VERIFY(ui.isHandleValid(child1.node()));
    CORRADE_COMPARE(ui.nodeParent(child1), anchor);
    CORRADE_COMPARE(ui.nodeOffset(child1), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(child1), (Vector2{3.0f, 5.0f}));
    CORRADE_COMPARE(ui.nodeFlags(child1), NodeFlag::Disabled);
    CORRADE_VERIFY(ui.snapLayouter().isHandleValid(child1));
    CORRADE_VERIFY(ui.snapLayouter().hasExplicitSnap(child1));
    CORRADE_COMPARE(ui.snapLayouter().node(child1), child1.node());
    CORRADE_COMPARE(ui.snapLayouter().explicitSnap(child1), Snap::TopRight);
    CORRADE_COMPARE(ui.snapLayouter().explicitSnapTarget(child1), layout);
    CORRADE_COMPARE(ui.snapLayouter().flags(child1), SnapLayoutFlag::IgnoreOverflowY);
    CORRADE_COMPARE(ui.snapLayouter().childSnap(child1), Snap::Bottom);

    /* Without NodeFlags */
    SnapLayout child2 = SnapLayout::child(Snap::Fill, anchor, {4.0f, 6.0f}, SnapLayoutFlag::PropagateMarginX);
    CORRADE_COMPARE(&child2.ui(), &ui);
    CORRADE_COMPARE(&child2.layouter(), &ui.snapLayouter());
    CORRADE_COMPARE(child2.node(), nodeHandle(5, 1));
    CORRADE_COMPARE(child2.layout(), layoutHandle(ui.snapLayouter().handle(), 4, 1));
    CORRADE_VERIFY(ui.isHandleValid(child2.node()));
    CORRADE_COMPARE(ui.nodeParent(child2), anchor);
    CORRADE_COMPARE(ui.nodeOffset(child2), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(child2), (Vector2{4.0f, 6.0f}));
    CORRADE_COMPARE(ui.nodeFlags(child2), NodeFlags{});
    CORRADE_VERIFY(ui.snapLayouter().isHandleValid(child2));
    CORRADE_VERIFY(ui.snapLayouter().hasExplicitSnap(child2));
    CORRADE_COMPARE(ui.snapLayouter().node(child2), child2.node());
    CORRADE_COMPARE(ui.snapLayouter().explicitSnap(child2), Snap::Fill);
    CORRADE_COMPARE(ui.snapLayouter().explicitSnapTarget(child2), layout);
    CORRADE_COMPARE(ui.snapLayouter().flags(child2), SnapLayoutFlag::PropagateMarginX);
    CORRADE_COMPARE(ui.snapLayouter().childSnap(child2), Snap::Bottom);

    /* There should be no additional nodes or layouts created besides the ones
       known above */
    CORRADE_COMPARE(ui.nodeUsedCount(), 6);
    CORRADE_COMPARE(ui.snapLayouter().usedCount(), 5);
}

template<class T> void SnapLayoutTest::staticSiblingExplicitSnap() {
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

    typename SnapLayoutTraits<T>::AnchorType anchor{ui, root, {}, {}};
    LayoutHandle layout = layouter.add(anchor);
    CORRADE_COMPARE(anchor.node(), nodeHandle(4, 1));
    CORRADE_COMPARE(layout, layoutHandle(layouter.handle(), 2, 1));

    /* With NodeFlags */
    T sibling1 = T::sibling(layouter, Snap::TopRight, anchor, {3.0f, 5.0f}, NodeFlag::Disabled, SnapLayoutFlag::IgnoreOverflowY);
    CORRADE_COMPARE(&sibling1.ui(), &ui);
    CORRADE_COMPARE(&sibling1.layouter(), &layouter);
    CORRADE_COMPARE(sibling1.node(), nodeHandle(5, 1));
    CORRADE_COMPARE(sibling1.layout(), layoutHandle(layouter.handle(), 3, 1));
    CORRADE_VERIFY(ui.isHandleValid(sibling1.node()));
    CORRADE_COMPARE(ui.nodeParent(sibling1), root);
    CORRADE_COMPARE(ui.nodeOffset(sibling1), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(sibling1), (Vector2{3.0f, 5.0f}));
    CORRADE_COMPARE(ui.nodeFlags(sibling1), NodeFlag::Disabled);
    CORRADE_VERIFY(layouter.isHandleValid(sibling1));
    CORRADE_VERIFY(layouter.hasExplicitSnap(sibling1));
    CORRADE_COMPARE(layouter.node(sibling1), sibling1.node());
    CORRADE_COMPARE(layouter.explicitSnap(sibling1), Snap::TopRight);
    CORRADE_COMPARE(layouter.explicitSnapTarget(sibling1), layout);
    CORRADE_COMPARE(layouter.flags(sibling1), SnapLayoutFlag::IgnoreOverflowY);
    CORRADE_COMPARE(layouter.childSnap(sibling1), Snap::Bottom);

    /* Without NodeFlags */
    T sibling2 = T::sibling(layouter, Snap::Fill, anchor, {4.0f, 6.0f}, SnapLayoutFlag::PropagateMarginX);
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
    CORRADE_COMPARE(layouter.explicitSnap(sibling2), Snap::Fill);
    CORRADE_COMPARE(layouter.explicitSnapTarget(sibling2), layout);
    CORRADE_COMPARE(layouter.flags(sibling2), SnapLayoutFlag::PropagateMarginX);
    CORRADE_COMPARE(layouter.childSnap(sibling2), Snap::Bottom);

    /* There should be no additional nodes or layouts created besides the ones
       known above */
    CORRADE_COMPARE(ui.nodeUsedCount(), 7);
    CORRADE_COMPARE(layouter.usedCount(), 5);
}

void SnapLayoutTest::staticSiblingExplicitSnapImplicitLayouter() {
    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};

    ui.setSnapLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));

    /* Create some extra nodes and layouts to not have the anchor and layout
       with trivial handles */
    ui.snapLayouter().add(ui.createNode({}, {}));
    ui.snapLayouter().add(ui.createNode({}, {}));
    ui.createNode({}, {});

    NodeHandle root = ui.createNode({}, {});

    Anchor anchor{ui, root, {}, {}};
    LayoutHandle layout = ui.snapLayouter().add(anchor);
    CORRADE_COMPARE(anchor.node(), nodeHandle(4, 1));
    CORRADE_COMPARE(layout, layoutHandle(ui.snapLayouter().handle(), 2, 1));

    /* With NodeFlags */
    SnapLayout sibling1 = SnapLayout::sibling(Snap::TopRight, anchor, {3.0f, 5.0f}, NodeFlag::Disabled, SnapLayoutFlag::IgnoreOverflowY);
    CORRADE_COMPARE(&sibling1.ui(), &ui);
    CORRADE_COMPARE(&sibling1.layouter(), &ui.snapLayouter());
    CORRADE_COMPARE(sibling1.node(), nodeHandle(5, 1));
    CORRADE_COMPARE(sibling1.layout(), layoutHandle(ui.snapLayouter().handle(), 3, 1));
    CORRADE_VERIFY(ui.isHandleValid(sibling1.node()));
    CORRADE_COMPARE(ui.nodeParent(sibling1), root);
    CORRADE_COMPARE(ui.nodeOffset(sibling1), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(sibling1), (Vector2{3.0f, 5.0f}));
    CORRADE_COMPARE(ui.nodeFlags(sibling1), NodeFlag::Disabled);
    CORRADE_VERIFY(ui.snapLayouter().isHandleValid(sibling1));
    CORRADE_VERIFY(ui.snapLayouter().hasExplicitSnap(sibling1));
    CORRADE_COMPARE(ui.snapLayouter().node(sibling1), sibling1.node());
    CORRADE_COMPARE(ui.snapLayouter().explicitSnap(sibling1), Snap::TopRight);
    CORRADE_COMPARE(ui.snapLayouter().explicitSnapTarget(sibling1), layout);
    CORRADE_COMPARE(ui.snapLayouter().flags(sibling1), SnapLayoutFlag::IgnoreOverflowY);
    CORRADE_COMPARE(ui.snapLayouter().childSnap(sibling1), Snap::Bottom);

    /* Without NodeFlags */
    SnapLayout sibling2 = SnapLayout::sibling(Snap::Fill, anchor, {4.0f, 6.0f}, SnapLayoutFlag::PropagateMarginX);
    CORRADE_COMPARE(&sibling2.ui(), &ui);
    CORRADE_COMPARE(&sibling2.layouter(), &ui.snapLayouter());
    CORRADE_COMPARE(sibling2.node(), nodeHandle(6, 1));
    CORRADE_COMPARE(sibling2.layout(), layoutHandle(ui.snapLayouter().handle(), 4, 1));
    CORRADE_VERIFY(ui.isHandleValid(sibling2.node()));
    CORRADE_COMPARE(ui.nodeParent(sibling2), root);
    CORRADE_COMPARE(ui.nodeOffset(sibling2), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(sibling2), (Vector2{4.0f, 6.0f}));
    CORRADE_COMPARE(ui.nodeFlags(sibling2), NodeFlags{});
    CORRADE_VERIFY(ui.snapLayouter().isHandleValid(sibling2));
    CORRADE_VERIFY(ui.snapLayouter().hasExplicitSnap(sibling2));
    CORRADE_COMPARE(ui.snapLayouter().node(sibling2), sibling2.node());
    CORRADE_COMPARE(ui.snapLayouter().explicitSnap(sibling2), Snap::Fill);
    CORRADE_COMPARE(ui.snapLayouter().explicitSnapTarget(sibling2), layout);
    CORRADE_COMPARE(ui.snapLayouter().flags(sibling2), SnapLayoutFlag::PropagateMarginX);
    CORRADE_COMPARE(ui.snapLayouter().childSnap(sibling2), Snap::Bottom);

    /* There should be no additional nodes or layouts created besides the ones
       known above */
    CORRADE_COMPARE(ui.nodeUsedCount(), 7);
    CORRADE_COMPARE(ui.snapLayouter().usedCount(), 5);
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

    /* Create some extra nodes to not have the anchor with a trivial handle */
    ui.createNode({}, {});
    ui.createNode({}, {});
    ui.createNode({}, {});
    ui.removeNode(ui.createNode({}, {}));
    ui.removeNode(ui.createNode({}, {}));
    ui.removeNode(ui.createNode({}, {}));

    typename SnapLayoutTraits<T>::AnchorType anchorNoLayout{ui, {}, {}};
    typename SnapLayoutTraits<T>::AnchorType anchor{ui, {}, {}};
    layouter.add(anchor);

    Containers::String out;
    Error redirectError{&out};
    /* Layouter handle not valid in the anchor UI */
    T::child(layouterInvalidHandle, anchor, {}, NodeFlags{});
    T::child(layouterInvalidHandle, anchor, {}, SnapLayoutFlags{});
    T::child(layouterInvalidHandle, anchor, {}, NodeFlags{}, LayoutHandle::Null);
    T::child(layouterInvalidHandle, anchor, {}, LayoutHandle::Null);
    T::child(layouterInvalidHandle, anchor, {}, NodeFlags{}, LayouterDataHandle::Null);
    T::child(layouterInvalidHandle, anchor, {}, LayouterDataHandle::Null);
    T::root(ui, layouterInvalidHandle, Snaps{}, {}, NodeFlags{});
    T::root(ui, layouterInvalidHandle, Snaps{}, {}, SnapLayoutFlags{});
    T::child(layouterInvalidHandle, Snaps{}, anchor, {}, NodeFlags{});
    T::child(layouterInvalidHandle, Snaps{}, anchor, {}, SnapLayoutFlags{});
    T::sibling(layouterInvalidHandle, Snaps{}, anchor, {}, NodeFlags{});
    T::sibling(layouterInvalidHandle, Snaps{}, anchor, {}, SnapLayoutFlags{});
    /* Valid handle, but different instance */
    T::child(layouterAnotherUi, anchor, {}, NodeFlags{});
    T::child(layouterAnotherUi, anchor, {}, SnapLayoutFlags{});
    T::child(layouterAnotherUi, anchor, {}, NodeFlags{}, LayoutHandle::Null);
    T::child(layouterAnotherUi, anchor, {}, LayoutHandle::Null);
    T::child(layouterAnotherUi, anchor, {}, NodeFlags{}, LayouterDataHandle::Null);
    T::child(layouterAnotherUi, anchor, {}, LayouterDataHandle::Null);
    T::root(ui, layouterAnotherUi, Snaps{}, {}, NodeFlags{});
    T::root(ui, layouterAnotherUi, Snaps{}, {}, SnapLayoutFlags{});
    T::child(layouterAnotherUi, Snaps{}, anchor, {}, NodeFlags{});
    T::child(layouterAnotherUi, Snaps{}, anchor, {}, SnapLayoutFlags{});
    T::sibling(layouterAnotherUi, Snaps{}, anchor, {}, NodeFlags{});
    T::sibling(layouterAnotherUi, Snaps{}, anchor, {}, SnapLayoutFlags{});
    /* Parent / target without a layout */
    T::child(layouter, anchorNoLayout, {}, NodeFlags{});
    T::child(layouter, anchorNoLayout, {}, SnapLayoutFlags{});
    T::child(layouter, anchorNoLayout, {}, NodeFlags{}, LayoutHandle::Null);
    T::child(layouter, anchorNoLayout, {}, LayoutHandle::Null);
    T::child(layouter, anchorNoLayout, {}, NodeFlags{}, LayouterDataHandle::Null);
    T::child(layouter, anchorNoLayout, {}, LayouterDataHandle::Null);
    /* snapRoot() doesn't have any target so it cannot fail here */
    T::child(layouter, Snaps{}, anchorNoLayout, {}, NodeFlags{});
    T::child(layouter, Snaps{}, anchorNoLayout, {}, SnapLayoutFlags{});
    T::sibling(layouter, Snaps{}, anchorNoLayout, {}, NodeFlags{});
    T::sibling(layouter, Snaps{}, anchorNoLayout, {}, SnapLayoutFlags{});
    CORRADE_COMPARE_AS(out,
        "Ui::AbstractSnapLayout::child(): layouter and parent not part of the same UI\n"
        "Ui::AbstractSnapLayout::child(): layouter and parent not part of the same UI\n"
        "Ui::AbstractSnapLayout::child(): layouter and parent not part of the same UI\n"
        "Ui::AbstractSnapLayout::child(): layouter and parent not part of the same UI\n"
        "Ui::AbstractSnapLayout::child(): layouter and parent not part of the same UI\n"
        "Ui::AbstractSnapLayout::child(): layouter and parent not part of the same UI\n"
        "Ui::AbstractSnapLayout::root(): layouter not part of the UI\n"
        "Ui::AbstractSnapLayout::root(): layouter not part of the UI\n"
        "Ui::AbstractSnapLayout::child(): layouter and parent not part of the same UI\n"
        "Ui::AbstractSnapLayout::child(): layouter and parent not part of the same UI\n"
        "Ui::AbstractSnapLayout::sibling(): layouter and target not part of the same UI\n"
        "Ui::AbstractSnapLayout::sibling(): layouter and target not part of the same UI\n"

        "Ui::AbstractSnapLayout::child(): layouter and parent not part of the same UI\n"
        "Ui::AbstractSnapLayout::child(): layouter and parent not part of the same UI\n"
        "Ui::AbstractSnapLayout::child(): layouter and parent not part of the same UI\n"
        "Ui::AbstractSnapLayout::child(): layouter and parent not part of the same UI\n"
        "Ui::AbstractSnapLayout::child(): layouter and parent not part of the same UI\n"
        "Ui::AbstractSnapLayout::child(): layouter and parent not part of the same UI\n"
        "Ui::AbstractSnapLayout::root(): layouter not part of the UI\n"
        "Ui::AbstractSnapLayout::root(): layouter not part of the UI\n"
        "Ui::AbstractSnapLayout::child(): layouter and parent not part of the same UI\n"
        "Ui::AbstractSnapLayout::child(): layouter and parent not part of the same UI\n"
        "Ui::AbstractSnapLayout::sibling(): layouter and target not part of the same UI\n"
        "Ui::AbstractSnapLayout::sibling(): layouter and target not part of the same UI\n"

        "Ui::AbstractSnapLayout::child(): Ui::NodeHandle(0x3, 0x4) doesn't have any layout from Ui::LayouterHandle(0x2, 0x3) assigned\n"
        "Ui::AbstractSnapLayout::child(): Ui::NodeHandle(0x3, 0x4) doesn't have any layout from Ui::LayouterHandle(0x2, 0x3) assigned\n"
        "Ui::AbstractSnapLayout::child(): Ui::NodeHandle(0x3, 0x4) doesn't have any layout from Ui::LayouterHandle(0x2, 0x3) assigned\n"
        "Ui::AbstractSnapLayout::child(): Ui::NodeHandle(0x3, 0x4) doesn't have any layout from Ui::LayouterHandle(0x2, 0x3) assigned\n"
        "Ui::AbstractSnapLayout::child(): Ui::NodeHandle(0x3, 0x4) doesn't have any layout from Ui::LayouterHandle(0x2, 0x3) assigned\n"
        "Ui::AbstractSnapLayout::child(): Ui::NodeHandle(0x3, 0x4) doesn't have any layout from Ui::LayouterHandle(0x2, 0x3) assigned\n"
        "Ui::AbstractSnapLayout::child(): Ui::NodeHandle(0x3, 0x4) doesn't have any layout from Ui::LayouterHandle(0x2, 0x3) assigned\n"
        "Ui::AbstractSnapLayout::child(): Ui::NodeHandle(0x3, 0x4) doesn't have any layout from Ui::LayouterHandle(0x2, 0x3) assigned\n"
        "Ui::AbstractSnapLayout::sibling(): Ui::NodeHandle(0x3, 0x4) doesn't have any layout from Ui::LayouterHandle(0x2, 0x3) assigned\n"
        "Ui::AbstractSnapLayout::sibling(): Ui::NodeHandle(0x3, 0x4) doesn't have any layout from Ui::LayouterHandle(0x2, 0x3) assigned\n",
        TestSuite::Compare::String);
}

void SnapLayoutTest::staticInvalidImplicitLayouter() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate},
      uiNoSnapLayouter{NoCreate};

    /* Create some extra layouters to not have it with a trivial handle */
    ui.createLayouter();
    ui.createLayouter();
    ui.removeLayouter(ui.createLayouter());
    ui.removeLayouter(ui.createLayouter());

    ui.setSnapLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()));

    /* Create some extra nodes to not have the anchor with a trivial handle */
    ui.createNode({}, {});
    ui.createNode({}, {});
    ui.createNode({}, {});
    ui.removeNode(ui.createNode({}, {}));
    ui.removeNode(ui.createNode({}, {}));
    ui.removeNode(ui.createNode({}, {}));

    Anchor anchorNoLayout{ui, {}, {}};
    Anchor anchorUiNoSnapLayouter{uiNoSnapLayouter, {}, {}};

    Containers::String out;
    Error redirectError{&out};
    /* Layouter not present */
    SnapLayout::child(anchorUiNoSnapLayouter, {}, NodeFlags{});
    SnapLayout::child(anchorUiNoSnapLayouter, {}, SnapLayoutFlags{});
    SnapLayout::child(anchorUiNoSnapLayouter, {}, NodeFlags{}, LayoutHandle::Null);
    SnapLayout::child(anchorUiNoSnapLayouter, {}, LayoutHandle::Null);
    SnapLayout::child(anchorUiNoSnapLayouter, {}, NodeFlags{}, LayouterDataHandle::Null);
    SnapLayout::child(anchorUiNoSnapLayouter, {}, LayouterDataHandle::Null);
    SnapLayout::root(uiNoSnapLayouter, Snaps{}, {}, NodeFlags{});
    SnapLayout::root(uiNoSnapLayouter, Snaps{}, {}, SnapLayoutFlags{});
    SnapLayout::child(Snaps{}, anchorUiNoSnapLayouter, {}, NodeFlags{});
    SnapLayout::child(Snaps{}, anchorUiNoSnapLayouter, {}, SnapLayoutFlags{});
    SnapLayout::sibling(Snaps{}, anchorUiNoSnapLayouter, {}, NodeFlags{});
    SnapLayout::sibling(Snaps{}, anchorUiNoSnapLayouter, {}, SnapLayoutFlags{});
    /* Parent / target without a layout */
    SnapLayout::child(anchorNoLayout, {}, NodeFlags{});
    SnapLayout::child(anchorNoLayout, {}, SnapLayoutFlags{});
    SnapLayout::child(anchorNoLayout, {}, NodeFlags{}, LayoutHandle::Null);
    SnapLayout::child(anchorNoLayout, {}, LayoutHandle::Null);
    SnapLayout::child(anchorNoLayout, {}, NodeFlags{}, LayouterDataHandle::Null);
    SnapLayout::child(anchorNoLayout, {}, LayouterDataHandle::Null);
    /* snapRoot() doesn't have any target so it cannot fail here */
    SnapLayout::child(Snaps{}, anchorNoLayout, {}, NodeFlags{});
    SnapLayout::child(Snaps{}, anchorNoLayout, {}, SnapLayoutFlags{});
    SnapLayout::sibling(Snaps{}, anchorNoLayout, {}, NodeFlags{});
    SnapLayout::sibling(Snaps{}, anchorNoLayout, {}, SnapLayoutFlags{});
    CORRADE_COMPARE_AS(out,
        "Ui::BasicSnapLayout::child(): SnapLayouter not present in the UI\n"
        "Ui::BasicSnapLayout::child(): SnapLayouter not present in the UI\n"
        "Ui::BasicSnapLayout::child(): SnapLayouter not present in the UI\n"
        "Ui::BasicSnapLayout::child(): SnapLayouter not present in the UI\n"
        "Ui::BasicSnapLayout::child(): SnapLayouter not present in the UI\n"
        "Ui::BasicSnapLayout::child(): SnapLayouter not present in the UI\n"
        "Ui::BasicSnapLayout::root(): SnapLayouter not present in the UI\n"
        "Ui::BasicSnapLayout::root(): SnapLayouter not present in the UI\n"
        "Ui::BasicSnapLayout::child(): SnapLayouter not present in the UI\n"
        "Ui::BasicSnapLayout::child(): SnapLayouter not present in the UI\n"
        "Ui::BasicSnapLayout::sibling(): SnapLayouter not present in the UI\n"
        "Ui::BasicSnapLayout::sibling(): SnapLayouter not present in the UI\n"

        "Ui::AbstractSnapLayout::child(): Ui::NodeHandle(0x3, 0x4) doesn't have any layout from Ui::LayouterHandle(0x2, 0x3) assigned\n"
        "Ui::AbstractSnapLayout::child(): Ui::NodeHandle(0x3, 0x4) doesn't have any layout from Ui::LayouterHandle(0x2, 0x3) assigned\n"
        "Ui::AbstractSnapLayout::child(): Ui::NodeHandle(0x3, 0x4) doesn't have any layout from Ui::LayouterHandle(0x2, 0x3) assigned\n"
        "Ui::AbstractSnapLayout::child(): Ui::NodeHandle(0x3, 0x4) doesn't have any layout from Ui::LayouterHandle(0x2, 0x3) assigned\n"
        "Ui::AbstractSnapLayout::child(): Ui::NodeHandle(0x3, 0x4) doesn't have any layout from Ui::LayouterHandle(0x2, 0x3) assigned\n"
        "Ui::AbstractSnapLayout::child(): Ui::NodeHandle(0x3, 0x4) doesn't have any layout from Ui::LayouterHandle(0x2, 0x3) assigned\n"
        "Ui::AbstractSnapLayout::child(): Ui::NodeHandle(0x3, 0x4) doesn't have any layout from Ui::LayouterHandle(0x2, 0x3) assigned\n"
        "Ui::AbstractSnapLayout::child(): Ui::NodeHandle(0x3, 0x4) doesn't have any layout from Ui::LayouterHandle(0x2, 0x3) assigned\n"
        "Ui::AbstractSnapLayout::sibling(): Ui::NodeHandle(0x3, 0x4) doesn't have any layout from Ui::LayouterHandle(0x2, 0x3) assigned\n"
        "Ui::AbstractSnapLayout::sibling(): Ui::NodeHandle(0x3, 0x4) doesn't have any layout from Ui::LayouterHandle(0x2, 0x3) assigned\n",
        TestSuite::Compare::String);
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::SnapLayoutTest)
