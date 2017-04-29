/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017
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
#include <Corrade/TestSuite/Tester.h>

#include "Magnum/Ui/Anchor.h"
#include "Magnum/Ui/BasicPlane.hpp"
#include "Magnum/Ui/BasicUserInterface.hpp"

namespace Magnum { namespace Ui { namespace Test {

struct BasicPlaneTest: TestSuite::Tester {
    explicit BasicPlaneTest();

    void construct();

    /* Anchoring tested in AnchorTest */

    void hierarchy();
    void hierarchyActivate();
    void hierarchyActivateActivated();
    void hierarchyHide();
    void hierarchyHideHidden();
    void hierarchyHideInactive();

    void debugFlag();
    void debugFlags();
};

BasicPlaneTest::BasicPlaneTest() {
    addTests({&BasicPlaneTest::construct,

              &BasicPlaneTest::hierarchy,
              &BasicPlaneTest::hierarchyActivate,
              &BasicPlaneTest::hierarchyActivateActivated,
              &BasicPlaneTest::hierarchyHide,
              &BasicPlaneTest::hierarchyHideHidden,
              &BasicPlaneTest::hierarchyHideInactive,

              &BasicPlaneTest::debugFlag,
              &BasicPlaneTest::debugFlags});
}

namespace {
    struct UserInterface: BasicUserInterface<> {
        #ifndef CORRADE_GCC47_COMPATIBILITY
        using BasicUserInterface::BasicUserInterface;
        #else
        explicit UserInterface(const Vector2& size, const Vector2i& screenSize): BasicUserInterface<>{size, screenSize} {}
        #endif
    };

    struct Plane: BasicPlane<> {
        #ifndef CORRADE_GCC47_COMPATIBILITY
        using BasicPlane::BasicPlane;
        #else
        explicit Plane(UserInterface& ui, const Anchor& anchor, const Range2D& padding, const Vector2& margin): BasicPlane<>{ui, anchor, padding, margin} {}
        #endif
    };
}

void BasicPlaneTest::construct() {
    UserInterface ui{{800, 600}, {1600, 900}};
    Plane plane{ui, {Snap::Left|Snap::Top, {400.0f, 300.0f}}, {{10.0f, 25.0f}, {-15.0f, -5.0f}}, {7.0f, 3.0f}};

    CORRADE_COMPARE(&plane.ui(), &ui);
    CORRADE_COMPARE(ui.activePlane(), &plane);
    CORRADE_COMPARE(plane.rect(), Range2D::fromSize({0, 300.0f}, {400.0f, 300.0f}));
    CORRADE_COMPARE(plane.padding(), (Range2D{{10.0f, 25.0f}, {-15.0f, -5.0f}}));
    CORRADE_COMPARE(plane.margin(), (Vector2{7.0f, 3.0f}));
    CORRADE_COMPARE(plane.flags(), PlaneFlags{});

    /* Just to test the const overload */
    const UserInterface& cui = ui;
    const Plane& cplane = plane;
    CORRADE_COMPARE(&cplane.ui(), &cui);
    CORRADE_COMPARE(cui.activePlane(), &cplane);
}

void BasicPlaneTest::hierarchy() {
    UserInterface ui{{800, 600}, {800, 600}};
    CORRADE_COMPARE(ui.activePlane(), nullptr);

    Plane a{ui, {{}, {800.0f, 600.0f}}, {}, {}};
    CORRADE_COMPARE(ui.activePlane(), &a);
    CORRADE_COMPARE(a.flags(), PlaneFlags{});
    CORRADE_COMPARE(a.previousActivePlane(), nullptr);
    CORRADE_COMPARE(a.nextActivePlane(), nullptr);

    Plane b{ui, {{}, {800.0f, 600.0f}}, {}, {}};
    CORRADE_COMPARE(ui.activePlane(), &a);
    CORRADE_COMPARE(a.flags(), PlaneFlags{});
    CORRADE_COMPARE(a.previousActivePlane(), nullptr);
    CORRADE_COMPARE(a.nextActivePlane(), nullptr);
    CORRADE_COMPARE(b.flags(), PlaneFlag::Hidden);
    CORRADE_COMPARE(b.previousActivePlane(), nullptr);
    CORRADE_COMPARE(b.nextActivePlane(), nullptr);

    Plane c{ui, {{}, {800.0f, 600.0f}}, {}, {}};
    CORRADE_COMPARE(ui.activePlane(), &a);
    CORRADE_COMPARE(a.flags(), PlaneFlags{});
    CORRADE_COMPARE(a.previousActivePlane(), nullptr);
    CORRADE_COMPARE(a.nextActivePlane(), nullptr);
    CORRADE_COMPARE(b.flags(), PlaneFlag::Hidden);
    CORRADE_COMPARE(b.previousActivePlane(), nullptr);
    CORRADE_COMPARE(b.nextActivePlane(), nullptr);
    CORRADE_COMPARE(c.flags(), PlaneFlag::Hidden);
    CORRADE_COMPARE(c.previousActivePlane(), nullptr);
    CORRADE_COMPARE(c.nextActivePlane(), nullptr);
}

void BasicPlaneTest::hierarchyActivate() {
    UserInterface ui{{800, 600}, {800, 600}};

    Plane a{ui, {{}, {800.0f, 600.0f}}, {}, {}};
    CORRADE_COMPARE(ui.activePlane(), &a);
    CORRADE_COMPARE(a.flags(), PlaneFlags{});
    CORRADE_COMPARE(a.previousActivePlane(), nullptr);
    CORRADE_COMPARE(a.nextActivePlane(), nullptr);

    Plane b{ui, {{}, {800.0f, 600.0f}}, {}, {}};
    CORRADE_COMPARE(ui.activePlane(), &a);
    CORRADE_COMPARE(b.flags(), PlaneFlag::Hidden);
    CORRADE_COMPARE(b.previousActivePlane(), nullptr);
    CORRADE_COMPARE(b.nextActivePlane(), nullptr);

    b.activate();
    CORRADE_COMPARE(ui.activePlane(), &b);
    CORRADE_COMPARE(a.flags(), PlaneFlags{});
    CORRADE_COMPARE(a.previousActivePlane(), nullptr);
    CORRADE_COMPARE(a.nextActivePlane(), &b);
    CORRADE_COMPARE(b.flags(), PlaneFlags{});
    CORRADE_COMPARE(b.previousActivePlane(), &a);
    CORRADE_COMPARE(b.nextActivePlane(), nullptr);

    /* Just to test the const overload */
    const Plane& cb = b;
    CORRADE_COMPARE(cb.previousActivePlane(), &a);
    CORRADE_COMPARE(cb.nextActivePlane(), nullptr);

    a.activate();
    CORRADE_COMPARE(ui.activePlane(), &a);
    CORRADE_COMPARE(a.flags(), PlaneFlags{});
    CORRADE_COMPARE(a.previousActivePlane(), &b);
    CORRADE_COMPARE(a.nextActivePlane(), nullptr);
    CORRADE_COMPARE(b.flags(), PlaneFlags{});
    CORRADE_COMPARE(b.previousActivePlane(), nullptr);
    CORRADE_COMPARE(b.nextActivePlane(), &a);
}

void BasicPlaneTest::hierarchyActivateActivated() {
    UserInterface ui{{800, 600}, {800, 600}};

    Plane a{ui, {{}, {800.0f, 600.0f}}, {}, {}};
    CORRADE_COMPARE(ui.activePlane(), &a);
    CORRADE_COMPARE(a.flags(), PlaneFlags{});
    CORRADE_COMPARE(a.previousActivePlane(), nullptr);
    CORRADE_COMPARE(a.nextActivePlane(), nullptr);

    Plane b{ui, {{}, {800.0f, 600.0f}}, {}, {}};
    CORRADE_COMPARE(ui.activePlane(), &a);
    CORRADE_COMPARE(b.flags(), PlaneFlag::Hidden);
    CORRADE_COMPARE(b.previousActivePlane(), nullptr);
    CORRADE_COMPARE(b.nextActivePlane(), nullptr);

    a.activate();
    CORRADE_COMPARE(ui.activePlane(), &a);
    CORRADE_COMPARE(a.flags(), PlaneFlags{});
    CORRADE_COMPARE(a.previousActivePlane(), nullptr);
    CORRADE_COMPARE(a.nextActivePlane(), nullptr);
    CORRADE_COMPARE(b.flags(), PlaneFlag::Hidden);
    CORRADE_COMPARE(b.previousActivePlane(), nullptr);
    CORRADE_COMPARE(b.nextActivePlane(), nullptr);
}

void BasicPlaneTest::hierarchyHide() {
    UserInterface ui{{800, 600}, {800, 600}};

    Plane a{ui, {{}, {800.0f, 600.0f}}, {}, {}};
    CORRADE_COMPARE(ui.activePlane(), &a);
    CORRADE_COMPARE(a.flags(), PlaneFlags{});
    CORRADE_COMPARE(a.previousActivePlane(), nullptr);
    CORRADE_COMPARE(a.nextActivePlane(), nullptr);

    Plane b{ui, {{}, {800.0f, 600.0f}}, {}, {}};
    CORRADE_COMPARE(ui.activePlane(), &a);
    CORRADE_COMPARE(b.flags(), PlaneFlag::Hidden);
    CORRADE_COMPARE(b.previousActivePlane(), nullptr);
    CORRADE_COMPARE(b.nextActivePlane(), nullptr);

    a.hide();
    CORRADE_COMPARE(ui.activePlane(), nullptr);
    CORRADE_COMPARE(a.flags(), PlaneFlag::Hidden);
    CORRADE_COMPARE(a.previousActivePlane(), nullptr);
    CORRADE_COMPARE(a.nextActivePlane(), nullptr);
    CORRADE_COMPARE(b.flags(), PlaneFlag::Hidden);
    CORRADE_COMPARE(b.previousActivePlane(), nullptr);
    CORRADE_COMPARE(b.nextActivePlane(), nullptr);
}

void BasicPlaneTest::hierarchyHideHidden() {
    UserInterface ui{{800, 600}, {800, 600}};

    Plane a{ui, {{}, {800.0f, 600.0f}}, {}, {}};
    CORRADE_COMPARE(ui.activePlane(), &a);
    CORRADE_COMPARE(a.flags(), PlaneFlags{});
    CORRADE_COMPARE(a.previousActivePlane(), nullptr);
    CORRADE_COMPARE(a.nextActivePlane(), nullptr);

    Plane b{ui, {{}, {800.0f, 600.0f}}, {}, {}};
    CORRADE_COMPARE(ui.activePlane(), &a);
    CORRADE_COMPARE(b.flags(), PlaneFlag::Hidden);
    CORRADE_COMPARE(b.previousActivePlane(), nullptr);
    CORRADE_COMPARE(b.nextActivePlane(), nullptr);

    b.hide();
    CORRADE_COMPARE(ui.activePlane(), &a);
    CORRADE_COMPARE(a.flags(), PlaneFlags{});
    CORRADE_COMPARE(a.previousActivePlane(), nullptr);
    CORRADE_COMPARE(a.nextActivePlane(), nullptr);
    CORRADE_COMPARE(b.flags(), PlaneFlag::Hidden);
    CORRADE_COMPARE(b.previousActivePlane(), nullptr);
    CORRADE_COMPARE(b.nextActivePlane(), nullptr);
}

void BasicPlaneTest::hierarchyHideInactive() {
    UserInterface ui{{800, 600}, {800, 600}};

    Plane a{ui, {{}, {800.0f, 600.0f}}, {}, {}};
    CORRADE_COMPARE(ui.activePlane(), &a);
    CORRADE_COMPARE(a.flags(), PlaneFlags{});
    CORRADE_COMPARE(a.previousActivePlane(), nullptr);
    CORRADE_COMPARE(a.nextActivePlane(), nullptr);

    Plane b{ui, {{}, {800.0f, 600.0f}}, {}, {}};
    CORRADE_COMPARE(ui.activePlane(), &a);
    CORRADE_COMPARE(b.flags(), PlaneFlag::Hidden);
    CORRADE_COMPARE(b.previousActivePlane(), nullptr);
    CORRADE_COMPARE(b.nextActivePlane(), nullptr);

    b.activate();
    CORRADE_COMPARE(ui.activePlane(), &b);
    CORRADE_COMPARE(a.flags(), PlaneFlags{});
    CORRADE_COMPARE(b.flags(), PlaneFlags{});
    CORRADE_COMPARE(b.previousActivePlane(), &a);
    CORRADE_COMPARE(b.nextActivePlane(), nullptr);

    a.hide();
    CORRADE_COMPARE(ui.activePlane(), &b);
    CORRADE_COMPARE(a.flags(), PlaneFlag::Hidden);
    CORRADE_COMPARE(b.flags(), PlaneFlags{});
    CORRADE_COMPARE(b.previousActivePlane(), nullptr);
    CORRADE_COMPARE(b.nextActivePlane(), nullptr);
}

void BasicPlaneTest::debugFlag() {
    std::ostringstream out;

    Debug{&out} << PlaneFlag::Hidden << PlaneFlag(0xdeadbabe);
    CORRADE_COMPARE(out.str(), "Ui::PlaneFlag::Hidden Ui::PlaneFlag(0xdeadbabe)\n");
}

void BasicPlaneTest::debugFlags() {
    std::ostringstream out;

    Debug{&out} << PlaneFlags{} << PlaneFlag::Hidden << (PlaneFlag(0xdead0000)|PlaneFlag::Hidden);
    CORRADE_COMPARE(out.str(), "Ui::PlaneFlags{} Ui::PlaneFlag::Hidden Ui::PlaneFlag::Hidden|Ui::PlaneFlag(0xdead0000)\n");
}

}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::BasicPlaneTest)
