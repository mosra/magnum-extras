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

#include <sstream>
#include <Corrade/Containers/Optional.h>
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/Utility/DebugStl.h> /** @todo remove once Debug is stream-free */
#include <Magnum/Math/Vector2.h>

#include "Magnum/Ui/Anchor.h"
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/NodeFlags.h"
#include "Magnum/Ui/UserInterface.h"
#include "Magnum/Ui/Widget.h"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct WidgetTest: TestSuite::Tester {
    explicit WidgetTest();

    template<class T> void construct();
    template<class T> void constructInvalid();
    template<class T> void constructFromAnchor();
    template<class T> void constructNoCreate();
    template<class T> void constructCopy();
    template<class T> void constructMove();

    void destructInvalidNode();

    void hidden();
    void disabled();

    void release();
};

WidgetTest::WidgetTest() {
    addTests({&WidgetTest::construct<AbstractWidget>,
              &WidgetTest::construct<Widget>,
              &WidgetTest::constructInvalid<AbstractWidget>,
              &WidgetTest::constructInvalid<Widget>,
              &WidgetTest::constructFromAnchor<AbstractWidget>,
              &WidgetTest::constructFromAnchor<Widget>,
              &WidgetTest::constructNoCreate<AbstractWidget>,
              &WidgetTest::constructNoCreate<Widget>,
              &WidgetTest::constructCopy<AbstractWidget>,
              &WidgetTest::constructCopy<Widget>,
              &WidgetTest::constructMove<AbstractWidget>,
              &WidgetTest::constructMove<Widget>,

              &WidgetTest::destructInvalidNode,

              &WidgetTest::hidden,
              &WidgetTest::disabled,

              &WidgetTest::release});
}

template<class> struct WidgetTraits;
template<> struct WidgetTraits<AbstractWidget> {
    typedef AbstractUserInterface UserInterfaceType;
    typedef AbstractAnchor AnchorType;
    static const char* name() { return "AbstractWidget"; }
};
template<> struct WidgetTraits<Widget> {
    typedef UserInterface UserInterfaceType;
    typedef Anchor AnchorType;
    static const char* name() { return "Widget"; }
};

template<class T> void WidgetTest::construct() {
    setTestCaseTemplateName(WidgetTraits<T>::name());

    struct Interface: WidgetTraits<T>::UserInterfaceType {
        explicit Interface(NoCreateT): WidgetTraits<T>::UserInterfaceType{NoCreate} {}
    } ui{NoCreate};
    NodeHandle node = ui.createNode({}, {});

    {
        T widget{ui, node};
        CORRADE_COMPARE(&widget.ui(), &ui);
        CORRADE_COMPARE(widget.node(), node);
        CORRADE_COMPARE(NodeHandle{widget}, node);
        CORRADE_VERIFY(!widget.isHidden());

        /* The node becomes owned by the widget */
        CORRADE_VERIFY(ui.isHandleValid(node));
    }

    /* And removed on destruction */
    CORRADE_VERIFY(!ui.isHandleValid(node));
}

template<class T> void WidgetTest::constructInvalid() {
    setTestCaseTemplateName(WidgetTraits<T>::name());

    CORRADE_SKIP_IF_NO_ASSERT();

    struct Interface: WidgetTraits<T>::UserInterfaceType {
        explicit Interface(NoCreateT): WidgetTraits<T>::UserInterfaceType{NoCreate} {}
    } ui{NoCreate};

    std::ostringstream out;
    Error redirectError{&out};
    /* Releasing the (invalid) node handle so it isn't attempted to be removed
       as well */
    T{ui, nodeHandle(0x12345, 0xabc)}.release();
    CORRADE_COMPARE(out.str(), "Ui::AbstractWidget: invalid handle Ui::NodeHandle(0x12345, 0xabc)\n");
}

template<class T> void WidgetTest::constructFromAnchor() {
    setTestCaseTemplateName(WidgetTraits<T>::name());

    struct Interface: WidgetTraits<T>::UserInterfaceType {
        explicit Interface(NoCreateT): WidgetTraits<T>::UserInterfaceType{NoCreate} {}
    } ui{NoCreate};

    typename WidgetTraits<T>::AnchorType a{ui, ui.createNode({}, {}), LayoutHandle::Null};

    {
        T widget{a};
        CORRADE_COMPARE(&widget.ui(), &ui);
        CORRADE_COMPARE(widget.node(), a.node());

        /* The node becomes owned by the widget */
        CORRADE_VERIFY(ui.isHandleValid(a.node()));
    }

    /* And is removed on destruction, making the anchor invalid */
    CORRADE_VERIFY(!ui.isHandleValid(a.node()));
}

template<class T> void WidgetTest::constructNoCreate() {
    setTestCaseTemplateName(WidgetTraits<T>::name());

    struct Interface: WidgetTraits<T>::UserInterfaceType {
        explicit Interface(NoCreateT): WidgetTraits<T>::UserInterfaceType{NoCreate} {}
    } ui{NoCreate};

    T widget{NoCreate, ui};
    CORRADE_COMPARE(&widget.ui(), &ui);
    CORRADE_COMPARE(widget.node(), NodeHandle::Null);
}

template<class T> void WidgetTest::constructCopy() {
    setTestCaseTemplateName(WidgetTraits<T>::name());

    CORRADE_VERIFY(!std::is_copy_constructible<T>{});
    CORRADE_VERIFY(!std::is_copy_assignable<T>{});
}

template<class T> void WidgetTest::constructMove() {
    setTestCaseTemplateName(WidgetTraits<T>::name());

    struct Interface: WidgetTraits<T>::UserInterfaceType {
        explicit Interface(NoCreateT): WidgetTraits<T>::UserInterfaceType{NoCreate} {}
    } ui{NoCreate};
    NodeHandle node = ui.createNode({}, {});

    T a{ui, node};

    T b{Utility::move(a)};
    CORRADE_COMPARE(b.node(), node);
    CORRADE_COMPARE(a.node(), NodeHandle::Null);

    NodeHandle node2 = ui.createNode({}, {});
    T c{ui, node2};
    c = Utility::move(b);
    CORRADE_COMPARE(c.node(), node);
    CORRADE_COMPARE(b.node(), node2);
}

void WidgetTest::destructInvalidNode() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    NodeHandle node = ui.createNode({}, {});

    Containers::Optional<Widget> widget{InPlaceInit, ui, node};

    /* Remove the node directly from the UI. The widget should still keep the
       original handle value. */
    ui.removeNode(node);
    CORRADE_VERIFY(!ui.isHandleValid(node));
    CORRADE_COMPARE(widget->node(), node);

    std::ostringstream out;
    Error redirectError{&out};
    widget = Containers::NullOpt;
    CORRADE_COMPARE(out.str(), "Ui::AbstractWidget: invalid handle Ui::NodeHandle(0x0, 0x1) on destruction\n");
}

void WidgetTest::hidden() {
    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    NodeHandle node = ui.createNode({}, {});

    Widget widget{ui, node};

    /* Not hidden by default */
    CORRADE_VERIFY(!widget.isHidden());
    CORRADE_COMPARE(ui.nodeFlags(node), NodeFlags{});

    /* Making the widget hidden correctly reflects that in the UI */
    widget.setHidden(true);
    CORRADE_VERIFY(widget.isHidden());
    CORRADE_VERIFY(!widget.isDisabled());
    CORRADE_COMPARE(ui.nodeFlags(node), NodeFlag::Hidden);

    /* ... and back */
    widget.setHidden(false);
    CORRADE_VERIFY(!widget.isHidden());
    CORRADE_COMPARE(ui.nodeFlags(node), NodeFlags{});

    /* Hiding it directly on the UI correctly reflects that in the widget as
       well */
    ui.addNodeFlags(node, NodeFlag::Hidden);
    CORRADE_VERIFY(widget.isHidden());

    /* ... and back */
    ui.clearNodeFlags(node, NodeFlag::Hidden);
    CORRADE_VERIFY(!widget.isHidden());
}

void WidgetTest::disabled() {
    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    NodeHandle node = ui.createNode({}, {});

    Widget widget{ui, node};

    /* Not disabled by default */
    CORRADE_VERIFY(!widget.isDisabled());
    CORRADE_COMPARE(ui.nodeFlags(node), NodeFlags{});

    /* Making the widget disabled correctly reflects that in the UI */
    widget.setDisabled(true);
    CORRADE_VERIFY(widget.isDisabled());
    CORRADE_VERIFY(!widget.isHidden());
    CORRADE_COMPARE(ui.nodeFlags(node), NodeFlag::Disabled);

    /* ... and back */
    widget.setDisabled(false);
    CORRADE_VERIFY(!widget.isDisabled());
    CORRADE_COMPARE(ui.nodeFlags(node), NodeFlags{});

    /* Disabling it directly on the UI correctly reflects that in the widget as
       well */
    ui.addNodeFlags(node, NodeFlag::Disabled);
    CORRADE_VERIFY(widget.isDisabled());

    /* ... and back */
    ui.clearNodeFlags(node, NodeFlag::Disabled);
    CORRADE_VERIFY(!widget.isDisabled());
}

void WidgetTest::release() {
    Containers::Optional<Widget> widget;

    {
        struct Interface: UserInterface {
            explicit Interface(NoCreateT): UserInterface{NoCreate} {}
        } ui{NoCreate};
        NodeHandle node = ui.createNode({}, {});

        widget = Widget{ui, node};

        NodeHandle released = widget->release();
        CORRADE_COMPARE(released, node);
        CORRADE_COMPARE(widget->node(), NodeHandle::Null);
    }

    /* Destructing a released widget once the UI is gone should be possible
       too -- i.e., it shouldn't try to access it in any way */
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::WidgetTest)
