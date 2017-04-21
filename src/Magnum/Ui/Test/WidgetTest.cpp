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

#include "Magnum/Ui/Widget.h"

namespace Magnum { namespace Ui { namespace Test {

struct WidgetTest: TestSuite::Tester {
    explicit WidgetTest();

    void debugWidgetFlag();
    void debugWidgetFlags();
};

WidgetTest::WidgetTest() {
    addTests({&WidgetTest::debugWidgetFlag,
              &WidgetTest::debugWidgetFlags});
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

}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::WidgetTest)
