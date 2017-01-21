/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016
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

#include <Corrade/TestSuite/Tester.h>

#include "Magnum/Ui/Anchor.h"
#include "Magnum/Ui/BasicPlane.hpp"
#include "Magnum/Ui/BasicUserInterface.hpp"
#include "Magnum/Ui/Widget.h"

namespace Magnum { namespace Ui { namespace Test {

struct AnchorTest: TestSuite::Tester {
    explicit AnchorTest();

    void anchorRect();

    void anchorPlaneToUi();
    void anchorWidgetToPlane();
    void anchorWidgetToWidget();
};

namespace {
    const Vector2 Size{20.0f, 30.0f};

    enum: std::size_t { AnchorRectDataCount = 28 };

    const struct {
        const char* name;
        Snaps snaps;
        Range2D expectedRect;
    } AnchorRectData[AnchorRectDataCount] = {
        #define _c(v) #v, v
        {_c(Snap::Top|Snap::Left|Snap::InsideX), Range2D::fromSize({105.0f, 505.0f}, Size)},
        {_c(Snap::Top|Snap::Left|Snap::InsideY), Range2D::fromSize({78.0f, 472.0f}, Size)},
        {_c(Snap::Top|Snap::Left|Snap::InsideX|Snap::InsideY), Range2D::fromSize({115.0f, 467.0f}, Size)},
        {_c(Snap::Top|Snap::Left|Snap::NoSpaceX|Snap::InsideX), Range2D::fromSize({105.0f, 505.0f}, Size)},
        {_c(Snap::Top|Snap::Left|Snap::NoSpaceX|Snap::InsideY), Range2D::fromSize({85.0f, 472.0f}, Size)},
        {_c(Snap::Top|Snap::Left|Snap::NoSpaceY|Snap::InsideX), Range2D::fromSize({105.0f, 502.0f}, Size)},
        {_c(Snap::Top|Snap::Left|Snap::NoSpaceY|Snap::InsideY), Range2D::fromSize({78.0f, 472.0f}, Size)},
        {_c(Snap::Top|Snap::Left|Snap::NoSpaceX|Snap::NoSpaceY|Snap::InsideX|Snap::InsideY),
            Range2D::fromSize({105.0f, 472.0f}, Size)},

        {_c(Snap::Bottom|Snap::Right|Snap::InsideX), Range2D::fromSize({485.0f, 169.0f}, Size)},
        {_c(Snap::Bottom|Snap::Right|Snap::InsideY), Range2D::fromSize({512.0f, 202.0f}, Size)},
        {_c(Snap::Bottom|Snap::Right|Snap::InsideX|Snap::InsideY), Range2D::fromSize({470.0f, 227.0f}, Size)},
        {_c(Snap::Bottom|Snap::Right|Snap::NoSpaceX|Snap::InsideX), Range2D::fromSize({485.0f, 169.0f}, Size)},
        {_c(Snap::Bottom|Snap::Right|Snap::NoSpaceX|Snap::InsideY), Range2D::fromSize({505.0f, 202.0f}, Size)},
        {_c(Snap::Bottom|Snap::Right|Snap::NoSpaceY|Snap::InsideX), Range2D::fromSize({485.0f, 172.0f}, Size)},
        {_c(Snap::Bottom|Snap::Right|Snap::NoSpaceY|Snap::InsideY), Range2D::fromSize({512.0f, 202.0f}, Size)},
        {_c(Snap::Bottom|Snap::Right|Snap::NoSpaceX|Snap::NoSpaceY|Snap::InsideX|Snap::InsideY),
            Range2D::fromSize({485.0f, 202.0f}, Size)},

        {_c(Snap::Top), Range2D::fromSize({295.0f, 505.0f}, Size)},
        {_c(Snap::Top|Snap::InsideY), Range2D::fromSize({292.5f, 467.0f}, Size)},
        {_c(Snap::Top|Snap::InsideY|Snap::NoSpaceX), Range2D::fromSize({295.0f, 467.0f}, Size)},

        {_c(Snap::Bottom|Snap::Left|Snap::Right), Range2D::fromSize({105.0f, 169.0f}, {400.0f, Size.y()})},
        {_c(Snap::Bottom|Snap::Left|Snap::Right|Snap::InsideY), Range2D::fromSize({115.0f, 227.0f}, {375.0f, Size.y()})},
        {_c(Snap::Bottom|Snap::Left|Snap::Right|Snap::InsideY|Snap::NoSpaceX),
            Range2D::fromSize({105.0f, 227.0f}, {400.0f, Size.y()})},

        {_c(Snap::Left), Range2D::fromSize({78.0f, 337.0f}, Size)},
        {_c(Snap::Left|Snap::InsideX), Range2D::fromSize({115.0f, 347.0f}, Size)},
        {_c(Snap::Left|Snap::InsideX|Snap::NoSpaceY), Range2D::fromSize({115.0f, 337.0f}, Size)},

        {_c(Snap::Top|Snap::Bottom|Snap::Right), Range2D::fromSize({512.0f, 202.0f}, {Size.x(), 300.0f})},
        {_c(Snap::Top|Snap::Bottom|Snap::Right|Snap::InsideX), Range2D::fromSize({470.0f, 227.0f}, {Size.x(), 270.0f})},
        {_c(Snap::Top|Snap::Bottom|Snap::Right|Snap::InsideX|Snap::NoSpaceY),
            Range2D::fromSize({470.0f, 202.0f}, {Size.x(), 300.0f})}
        #undef _c
    };

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

    struct Widget: Ui::Widget {
        #ifndef CORRADE_GCC47_COMPATIBILITY
        using Ui::Widget::Widget;
        #else
        explicit Widget(AbstractPlane& plane, const Anchor& anchor, const Range2D& padding = {}): Ui::Widget{plane, anchor, padding} {}
        #endif
    };
}

AnchorTest::AnchorTest() {
    addInstancedTests({&AnchorTest::anchorRect}, AnchorRectDataCount);

    addTests({&AnchorTest::anchorPlaneToUi,
              &AnchorTest::anchorWidgetToPlane,
              &AnchorTest::anchorWidgetToWidget});
}

void AnchorTest::anchorRect() {
    setTestCaseDescription(AnchorRectData[testCaseInstanceId()].name);

    const Range2D a = Implementation::anchorRect(AnchorRectData[testCaseInstanceId()].snaps,
        Range2D::fromSize({100.0f, 200.0f}, {400.0f, 300.0f}),
        {{10.0f, 25.0f}, {-15.0f, -5.0f}},
        {7.0f, 3.0f},
        Range2D::fromSize({5.0f, 2.0f}, Size));
    CORRADE_COMPARE(a, AnchorRectData[testCaseInstanceId()].expectedRect);
}

void AnchorTest::anchorPlaneToUi() {
    UserInterface ui{{400.0f, 300.0f}, {400, 300}};
    Anchor a{Snap::Top|Snap::Left, Size};
    CORRADE_COMPARE(a.rect(ui), Range2D::fromSize({0.0f, 270.0f}, Size));
}

void AnchorTest::anchorWidgetToPlane() {
    UserInterface ui{{400.0f, 300.0f}, {400, 300}};
    Plane plane{ui, {{}, {400.0f, 300.0f}}, {{10.0f, 25.0f}, {-15.0f, -5.0f}}, {7.0f, 3.0f}};
    Anchor a{Snap::Top|Snap::Left, Size};
    CORRADE_COMPARE(a.rect(plane), Range2D::fromSize({10.0f, 265.0f}, Size));
}

void AnchorTest::anchorWidgetToWidget() {
    UserInterface ui{{400.0f, 300.0f}, {400, 300}};
    Plane plane{ui, {{}, {400.0f, 300.0f}}, {{10.0f, 25.0f}, {-15.0f, -5.0f}}, {7.0f, 3.0f}};
    Widget widget{plane, {Snap::Bottom|Snap::Right, {200.0f, 200.0f}}};
    Anchor a{Snap::Top|Snap::Left|Snap::InsideY, widget, Size};
    CORRADE_COMPARE(a.rect(plane), Range2D::fromSize({158.0f, 195.0f}, Size));
}

}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::AnchorTest)
