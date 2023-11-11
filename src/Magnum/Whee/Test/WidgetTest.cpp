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

#include <Corrade/Containers/Optional.h>
#include <Corrade/TestSuite/Tester.h>
#include <Magnum/Math/Vector2.h>

#include "Magnum/Whee/Handle.h"
#include "Magnum/Whee/UserInterface.h"
#include "Magnum/Whee/Widget.h"

namespace Magnum { namespace Whee { namespace Test { namespace {

struct WidgetTest: TestSuite::Tester {
    explicit WidgetTest();

    void construct();
    void constructCopy();
    void constructMove();
    void destructInvalidNode();

    void hidden();

    void release();
};

WidgetTest::WidgetTest() {
    addTests({&WidgetTest::construct,
              &WidgetTest::constructCopy,
              &WidgetTest::constructMove,
              &WidgetTest::destructInvalidNode,

              &WidgetTest::hidden,

              &WidgetTest::release});
}

void WidgetTest::construct() {
    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    NodeHandle node = ui.createNode({}, {});

    {
        Widget widget{ui, node};
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

void WidgetTest::constructCopy() {
    CORRADE_VERIFY(!std::is_copy_constructible<Widget>{});
    CORRADE_VERIFY(!std::is_copy_assignable<Widget>{});
}

void WidgetTest::constructMove() {
    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    NodeHandle node = ui.createNode({}, {});

    Widget a{ui, node};

    Widget b{Utility::move(a)};
    CORRADE_COMPARE(b.node(), node);
    CORRADE_COMPARE(a.node(), NodeHandle::Null);

    NodeHandle node2 = ui.createNode({}, {});
    Widget c{ui, node2};
    c = Utility::move(b);
    CORRADE_COMPARE(c.node(), node);
    CORRADE_COMPARE(b.node(), node2);
}

void WidgetTest::destructInvalidNode() {
    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    NodeHandle node = ui.createNode({}, {});

    {
        Widget widget{ui, node};

        /* Remove the node directly from the UI. The widget should still keep
           the original handle value. */
        ui.removeNode(node);
        CORRADE_VERIFY(!ui.isHandleValid(node));
        CORRADE_COMPARE(widget.node(), node);
    }

    /* The widget shouldn't attempt to remove the already-removed node again on
       destruction */
    CORRADE_VERIFY(!ui.isHandleValid(node));
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
    widget.setHidden();
    CORRADE_VERIFY(widget.isHidden());
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

CORRADE_TEST_MAIN(Magnum::Whee::Test::WidgetTest)
