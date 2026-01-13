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

#include <Corrade/Containers/BitArrayView.h>
#include <Corrade/Containers/String.h>
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/String.h>
#include <Magnum/Math/Vector2.h>

#include "Magnum/Ui/Anchor.h"
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/NodeFlags.h"
#include "Magnum/Ui/UserInterface.h"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct AnchorTest: TestSuite::Tester {
    explicit AnchorTest();

    template<class T> void construct();
    template<class T> void constructInvalid();
    template<class T> void constructCreateNode();
    template<class T> void constructCreateNodeRoot();
    /* Passing an invalid parent to the node creation is asserted in
       UserInterface directly */
};

AnchorTest::AnchorTest() {
    addTests<AnchorTest>({&AnchorTest::construct<AbstractAnchor>,
                          &AnchorTest::construct<Anchor>,
                          &AnchorTest::constructInvalid<AbstractAnchor>,
                          &AnchorTest::constructInvalid<Anchor>,
                          &AnchorTest::constructCreateNode<AbstractAnchor>,
                          &AnchorTest::constructCreateNode<Anchor>,
                          &AnchorTest::constructCreateNodeRoot<AbstractAnchor>,
                          &AnchorTest::constructCreateNodeRoot<Anchor>});
}

template<class> struct AnchorTraits;
template<> struct AnchorTraits<AbstractAnchor> {
    typedef AbstractUserInterface UserInterfaceType;
    static const char* name() { return "AbstractAnchor"; }
};
template<> struct AnchorTraits<Anchor> {
    typedef UserInterface UserInterfaceType;
    static const char* name() { return "Anchor"; }
};

template<class T> void AnchorTest::construct() {
    setTestCaseTemplateName(AnchorTraits<T>::name());

    struct Interface: AnchorTraits<T>::UserInterfaceType {
        explicit Interface(NoCreateT): AnchorTraits<T>::UserInterfaceType{NoCreate} {}
    } ui{NoCreate};

    NodeHandle node = ui.createNode({}, {});

    T anchor = {ui, node};
    CORRADE_COMPARE(&anchor.ui(), &ui);
    CORRADE_COMPARE(anchor.node(), node);
    CORRADE_COMPARE(anchor, node);
}

template<class T> void AnchorTest::constructInvalid() {
    setTestCaseTemplateName(AnchorTraits<T>::name());

    CORRADE_SKIP_IF_NO_ASSERT();

    struct Interface: AnchorTraits<T>::UserInterfaceType {
        explicit Interface(NoCreateT): AnchorTraits<T>::UserInterfaceType{NoCreate} {}
    } ui{NoCreate};

    Containers::String out;
    Error redirectError{&out};
    T{ui, nodeHandle(0x12345, 0xabc)};
    CORRADE_COMPARE(out, "Ui::AbstractAnchor: invalid handle Ui::NodeHandle(0x12345, 0xabc)\n");
}

template<class T> void AnchorTest::constructCreateNode() {
    setTestCaseTemplateName(AnchorTraits<T>::name());

    struct Interface: AnchorTraits<T>::UserInterfaceType {
        explicit Interface(NoCreateT): AnchorTraits<T>::UserInterfaceType{NoCreate} {}
    } ui{NoCreate};

    NodeHandle parent = ui.createNode({}, {});

    T anchor{ui, parent, {1.0f, 2.0f}, {3.0f, 4.0f}, NodeFlag::Disabled};
    CORRADE_COMPARE(&anchor.ui(), &ui);
    CORRADE_COMPARE(ui.nodeParent(anchor), parent);
    CORRADE_COMPARE(ui.nodeOffset(anchor), (Vector2{1.0f, 2.0f}));
    CORRADE_COMPARE(ui.nodeSize(anchor), (Vector2{3.0f, 4.0f}));
    CORRADE_COMPARE(ui.nodeFlags(anchor), NodeFlag::Disabled);
}

template<class T> void AnchorTest::constructCreateNodeRoot() {
    setTestCaseTemplateName(AnchorTraits<T>::name());

    struct Interface: AnchorTraits<T>::UserInterfaceType {
        explicit Interface(NoCreateT): AnchorTraits<T>::UserInterfaceType{NoCreate} {}
    } ui{NoCreate};

    T anchor{ui, {1.0f, 2.0f}, {3.0f, 4.0f}, NodeFlag::Disabled};
    CORRADE_COMPARE(&anchor.ui(), &ui);
    CORRADE_COMPARE(ui.nodeParent(anchor), NodeHandle::Null);
    CORRADE_COMPARE(ui.nodeOffset(anchor), (Vector2{1.0f, 2.0f}));
    CORRADE_COMPARE(ui.nodeSize(anchor), (Vector2{3.0f, 4.0f}));
    CORRADE_COMPARE(ui.nodeFlags(anchor), NodeFlag::Disabled);
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::AnchorTest)
