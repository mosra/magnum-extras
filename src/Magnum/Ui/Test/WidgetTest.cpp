/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019
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
#include <Corrade/Containers/Reference.h>
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/Utility/DebugStl.h>

#include "Magnum/Ui/Anchor.h"
#include "Magnum/Ui/BasicUserInterface.hpp"
#include "Magnum/Ui/BasicPlane.hpp"
#include "Magnum/Ui/Widget.h"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct WidgetTest: TestSuite::Tester {
    explicit WidgetTest();

    void construct();

    void flags();

    void debugWidgetFlag();
    void debugWidgetFlags();
};

WidgetTest::WidgetTest() {
    addTests({&WidgetTest::construct,

              &WidgetTest::flags,

              &WidgetTest::debugWidgetFlag,
              &WidgetTest::debugWidgetFlags});
}

struct UserInterface: BasicUserInterface<> {
    using BasicUserInterface::BasicUserInterface;
};

struct Plane: BasicPlane<> {
    using BasicPlane::BasicPlane;
};

struct Widget: Ui::Widget {
    using Ui::Widget::Widget;

    Plane& plane() { return static_cast<Plane&>(Ui::Widget::plane()); }
    const Plane& plane() const { return static_cast<const Plane&>(Ui::Widget::plane()); }
};

void WidgetTest::construct() {
    UserInterface ui{{800.0f, 600.0f}, {800, 600}};
    Plane plane{ui, {Snap::Left|Snap::Top, {400.0f, 300.0f}}, {}, {}};
    Widget a{plane, {Snap::Bottom|Snap::Right, {100.0f, 100.0f}}, {{7.5f, -0.3f}, {1.5f, 0.7f}}};

    CORRADE_COMPARE(a.rect(), Range2D::fromSize({300, 0}, {100, 100}));
    CORRADE_COMPARE(a.padding(), (Range2D{{7.5f, -0.3f}, {1.5f, 0.7f}}));
    CORRADE_COMPARE(a.flags(), WidgetFlags{});
    CORRADE_COMPARE(&a.plane(), &plane);

    /* Verify also the const overloads */
    const Widget& ca = a;
    CORRADE_COMPARE(&ca.plane(), &plane);
}

void WidgetTest::flags() {
    UserInterface ui{{800.0f, 600.0f}, {800, 600}};
    Plane plane{ui, {Snap::Left|Snap::Top, {400.0f, 300.0f}}, {}, {}};
    Widget a{plane, {Snap::Bottom|Snap::Right, {100.0f, 100.0f}}};
    Widget b{plane, {Snap::Left, a, {100.0f, 100.0f}}};
    Widget c{plane, {Snap::Left, b, {100.0f, 100.0f}}};

    CORRADE_COMPARE(a.flags(), WidgetFlags{});
    CORRADE_COMPARE(b.flags(), WidgetFlags{});
    CORRADE_COMPARE(c.flags(), WidgetFlags{});

    a.disable();
    a.hide();
    CORRADE_COMPARE(a.flags(), WidgetFlag::Disabled|WidgetFlag::Hidden);

    Widget::disable({b, c});
    CORRADE_COMPARE(b.flags(), WidgetFlag::Disabled);
    CORRADE_COMPARE(c.flags(), WidgetFlag::Disabled);

    Widget::enable({a, b});
    CORRADE_COMPARE(a.flags(), WidgetFlag::Hidden);
    CORRADE_COMPARE(b.flags(), WidgetFlags{});

    Widget::hide({b, c});
    CORRADE_COMPARE(b.flags(), WidgetFlag::Hidden);
    CORRADE_COMPARE(c.flags(), WidgetFlag::Hidden|WidgetFlag::Disabled);

    c.enable();
    CORRADE_COMPARE(c.flags(), WidgetFlag::Hidden);

    Widget::show({a, c});
    b.show();
    CORRADE_COMPARE(a.flags(), WidgetFlags{});
    CORRADE_COMPARE(b.flags(), WidgetFlags{});
    CORRADE_COMPARE(c.flags(), WidgetFlags{});

    Widget::setEnabled(false, {c, a});
    Widget::setVisible(false, {b, c});
    CORRADE_COMPARE(a.flags(), WidgetFlag::Disabled);
    CORRADE_COMPARE(b.flags(), WidgetFlag::Hidden);
    CORRADE_COMPARE(c.flags(), WidgetFlag::Hidden|WidgetFlag::Disabled);

    a.setVisible(false);
    b.setEnabled(false);
    c.setVisible(true);
    CORRADE_COMPARE(a.flags(), WidgetFlag::Hidden|WidgetFlag::Disabled);
    CORRADE_COMPARE(b.flags(), WidgetFlag::Hidden|WidgetFlag::Disabled);
    CORRADE_COMPARE(c.flags(), WidgetFlag::Disabled);

    a.setEnabled(true);
    Widget::setEnabled(true, {b});
    CORRADE_COMPARE(a.flags(), WidgetFlag::Hidden);
    CORRADE_COMPARE(b.flags(), WidgetFlag::Hidden);

    Widget::setVisible(true, {b});
    CORRADE_COMPARE(b.flags(), WidgetFlags{});
}

void WidgetTest::debugWidgetFlag() {
    std::ostringstream out;

    Debug{&out} << WidgetFlag::Hidden << WidgetFlag(0xdeadbabe);
    CORRADE_COMPARE(out.str(), "Ui::WidgetFlag::Hidden Ui::WidgetFlag(0xdeadbabe)\n");
}

void WidgetTest::debugWidgetFlags() {
    std::ostringstream out;

    Debug{&out} << WidgetFlags{} << (WidgetFlag::Disabled|WidgetFlag::Active) << (WidgetFlag(0xdead0000)|WidgetFlag::Hovered);
    CORRADE_COMPARE(out.str(), "Ui::WidgetFlags{} Ui::WidgetFlag::Active|Ui::WidgetFlag::Disabled Ui::WidgetFlag::Hovered|Ui::WidgetFlag(0xdead0000)\n");
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::WidgetTest)
