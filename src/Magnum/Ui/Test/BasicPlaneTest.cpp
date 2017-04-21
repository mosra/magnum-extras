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

#include "Magnum/Ui/BasicPlane.hpp"

namespace Magnum { namespace Ui { namespace Test {

struct BasicPlaneTest: TestSuite::Tester {
    explicit BasicPlaneTest();

    void debugFlag();
    void debugFlags();
};

BasicPlaneTest::BasicPlaneTest() {
    addTests({&BasicPlaneTest::debugFlag,
              &BasicPlaneTest::debugFlags});
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
