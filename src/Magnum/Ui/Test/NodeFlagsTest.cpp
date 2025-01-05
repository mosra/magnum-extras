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

#include <sstream> /** @todo remove once Debug is stream-free */
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/Utility/DebugStl.h> /** @todo remove once Debug is stream-free */

#include "Magnum/Ui/NodeFlags.h"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct NodeFlagsTest: TestSuite::Tester {
    explicit NodeFlagsTest();

    void debugFlag();
    void debugFlags();
    void debugFlagsSupersets();
};

NodeFlagsTest::NodeFlagsTest() {
    addTests({&NodeFlagsTest::debugFlag,
              &NodeFlagsTest::debugFlags,
              &NodeFlagsTest::debugFlagsSupersets});
}

void NodeFlagsTest::debugFlag() {
    std::ostringstream out;
    Debug{&out} << NodeFlag::Hidden << NodeFlag(0xbe);
    CORRADE_COMPARE(out.str(), "Ui::NodeFlag::Hidden Ui::NodeFlag(0xbe)\n");
}

void NodeFlagsTest::debugFlags() {
    std::ostringstream out;
    Debug{&out} << (NodeFlag::Hidden|NodeFlag(0x80)) << NodeFlags{};
    CORRADE_COMPARE(out.str(), "Ui::NodeFlag::Hidden|Ui::NodeFlag(0x80) Ui::NodeFlags{}\n");
}

void NodeFlagsTest::debugFlagsSupersets() {
    /* Disabled is a superset of NoEvents, so only one should be printed */
    {
        std::ostringstream out;
        Debug{&out} << (NodeFlag::Disabled|NodeFlag::NoEvents);
        CORRADE_COMPARE(out.str(), "Ui::NodeFlag::Disabled\n");
    }
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::NodeFlagsTest)
