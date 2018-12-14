/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018
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

    enum: std::size_t { AnchorRectDataCount = 38 };

    const struct {
        const char* name;
        Snaps snaps;
        Range2D expectedRect;
    } AnchorRectData[AnchorRectDataCount] = {
        /*       100   500

                QR A M
                   E
            500   +-----+     +-----+     +-----+      +-----+
                BD|F    |     |     |     |   f | d    |hhhhh|
                  | CN  |     |     |     |   e | d    |hgggh|
                O | PU  |     |     |     |   e | d    |hgggh|
                  |   I |     |cbbbc|     |   e | d    |hgggh|
                  |    L|JH   |     |     |   f | d    |hhhhh|
            200   +-----+     +-----+     +-----+      +-----+
                       K  T
                       G  S    aaaaa */
        {"A1", Snap::Top|Snap::Left|Snap::InsideX, Range2D::fromSize({105.0f, 505.0f}, Size)},
        {"B1", Snap::Top|Snap::Left|Snap::InsideY, Range2D::fromSize({78.0f, 472.0f}, Size)},
        {"C",  Snap::Top|Snap::Left|Snap::InsideX|Snap::InsideY, Range2D::fromSize({115.0f, 467.0f}, Size)},
        {"A2", Snap::Top|Snap::Left|Snap::NoSpaceX|Snap::InsideX, Range2D::fromSize({105.0f, 505.0f}, Size)},
        {"D",  Snap::Top|Snap::Left|Snap::NoSpaceX|Snap::InsideY, Range2D::fromSize({85.0f, 472.0f}, Size)},
        {"E",  Snap::Top|Snap::Left|Snap::NoSpaceY|Snap::InsideX, Range2D::fromSize({105.0f, 502.0f}, Size)},
        {"B2", Snap::Top|Snap::Left|Snap::NoSpaceY|Snap::InsideY, Range2D::fromSize({78.0f, 472.0f}, Size)},
        {"F",  Snap::Top|Snap::Left|Snap::NoSpaceX|Snap::NoSpaceY|Snap::InsideX|Snap::InsideY,
            Range2D::fromSize({105.0f, 472.0f}, Size)},

        {"G1", Snap::Bottom|Snap::Right|Snap::InsideX, Range2D::fromSize({485.0f, 169.0f}, Size)},
        {"H2", Snap::Bottom|Snap::Right|Snap::InsideY, Range2D::fromSize({512.0f, 202.0f}, Size)},
        {"I",  Snap::Bottom|Snap::Right|Snap::InsideX|Snap::InsideY, Range2D::fromSize({470.0f, 227.0f}, Size)},
        {"G2", Snap::Bottom|Snap::Right|Snap::NoSpaceX|Snap::InsideX, Range2D::fromSize({485.0f, 169.0f}, Size)},
        {"J",  Snap::Bottom|Snap::Right|Snap::NoSpaceX|Snap::InsideY, Range2D::fromSize({505.0f, 202.0f}, Size)},
        {"K",  Snap::Bottom|Snap::Right|Snap::NoSpaceY|Snap::InsideX, Range2D::fromSize({485.0f, 172.0f}, Size)},
        {"H2", Snap::Bottom|Snap::Right|Snap::NoSpaceY|Snap::InsideY, Range2D::fromSize({512.0f, 202.0f}, Size)},
        {"L",  Snap::Bottom|Snap::Right|Snap::NoSpaceX|Snap::NoSpaceY|Snap::InsideX|Snap::InsideY,
            Range2D::fromSize({485.0f, 202.0f}, Size)},

        {"M1", Snap::Top, Range2D::fromSize({295.0f, 505.0f}, Size)},
        {"M2", Snap::Top|Snap::NoSpaceX, Range2D::fromSize({295.0f, 505.0f}, Size)},
        {"N", Snap::Top|Snap::InsideY, Range2D::fromSize({292.5f, 467.0f}, Size)},
        {"N no spacing X", Snap::Top|Snap::InsideY|Snap::NoSpaceX, Range2D::fromSize({295.0f, 467.0f}, Size)},

        {"O1", Snap::Left, Range2D::fromSize({78.0f, 337.0f}, Size)},
        {"O2", Snap::Left|Snap::NoSpaceY, Range2D::fromSize({78.0f, 337.0f}, Size)},
        {"P", Snap::Left|Snap::InsideX, Range2D::fromSize({115.0f, 347.0f}, Size)},
        {"P no spacing Y", Snap::Left|Snap::InsideX|Snap::NoSpaceY, Range2D::fromSize({115.0f, 337.0f}, Size)},

        {"Q", Snap::Top|Snap::Left, Range2D::fromSize({78.0f, 505.0f}, Size)},
        {"R", Snap::Top|Snap::Left|Snap::NoSpaceX, Range2D::fromSize({85.0f, 505.0f}, Size)},
        {"S", Snap::Bottom|Snap::Right, Range2D::fromSize({512.0f, 169.0f}, Size)},
        {"T", Snap::Bottom|Snap::Right|Snap::NoSpaceY, Range2D::fromSize({512.0f, 172.0f}, Size)},
        {"U", {}, Range2D::fromSize({292.5f, 347.0f}, Size)},
        {"U no spacing XY", Snap::NoSpaceX|Snap::NoSpaceY, Range2D::fromSize({295.0f, 337.0f}, Size)},

        {"aaa", Snap::Bottom|Snap::Left|Snap::Right, Range2D::fromSize({105.0f, 169.0f}, {400.0f, Size.y()})},
        {"bbb", Snap::Bottom|Snap::Left|Snap::Right|Snap::InsideY, Range2D::fromSize({115.0f, 227.0f}, {375.0f, Size.y()})},
        {"cbc", Snap::Bottom|Snap::Left|Snap::Right|Snap::InsideY|Snap::NoSpaceX,
            Range2D::fromSize({105.0f, 227.0f}, {400.0f, Size.y()})},

        {"ddd", Snap::Top|Snap::Bottom|Snap::Right, Range2D::fromSize({512.0f, 202.0f}, {Size.x(), 300.0f})},
        {"eee", Snap::Top|Snap::Bottom|Snap::Right|Snap::InsideX, Range2D::fromSize({470.0f, 227.0f}, {Size.x(), 270.0f})},
        {"fef", Snap::Top|Snap::Bottom|Snap::Right|Snap::InsideX|Snap::NoSpaceY,
            Range2D::fromSize({470.0f, 202.0f}, {Size.x(), 300.0f})},

        {"ggg", Snap::Top|Snap::Bottom|Snap::Left|Snap::Right,
            Range2D::fromSize({115.0f, 227.0f}, {375.0f, 270.0f})},
        {"hgh", Snap::Top|Snap::Bottom|Snap::Left|Snap::Right|Snap::NoSpaceX|Snap::NoSpaceY,
            Range2D::fromSize({105.0f, 202.0f}, {400.0f, 300.0f})}};

    struct UserInterface: BasicUserInterface<> {
        using BasicUserInterface::BasicUserInterface;
    };

    struct Plane: BasicPlane<> {
        using BasicPlane::BasicPlane;
    };

    struct Widget: Ui::Widget {
        using Ui::Widget::Widget;
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
