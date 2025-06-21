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

#include <Corrade/Containers/String.h>
#include <Corrade/TestSuite/Tester.h>

#include "Magnum/Ui/NodeFlags.h"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct NodeFlagsTest: TestSuite::Tester {
    explicit NodeFlagsTest();

    void debugFlag();
    void debugFlagPacked();
    void debugFlags();
    void debugFlagsPacked();
    void debugFlagsSupersets();
};

NodeFlagsTest::NodeFlagsTest() {
    addTests({&NodeFlagsTest::debugFlag,
              &NodeFlagsTest::debugFlagPacked,
              &NodeFlagsTest::debugFlags,
              &NodeFlagsTest::debugFlagsPacked,
              &NodeFlagsTest::debugFlagsSupersets});
}

void NodeFlagsTest::debugFlag() {
    Containers::String out;
    Debug{&out} << NodeFlag::Hidden << NodeFlag(0xbe);
    CORRADE_COMPARE(out, "Ui::NodeFlag::Hidden Ui::NodeFlag(0xbe)\n");
}

void NodeFlagsTest::debugFlagPacked() {
    Containers::String out;
    /* Last is not packed, ones before should not make any flags persistent */
    Debug{&out} << Debug::packed << NodeFlag::Hidden << Debug::packed << NodeFlag(0xbe) << NodeFlag::Focusable;
    CORRADE_COMPARE(out, "Hidden 0xbe Ui::NodeFlag::Focusable\n");
}

void NodeFlagsTest::debugFlags() {
    Containers::String out;
    Debug{&out} << (NodeFlag::Hidden|NodeFlag::Clip|NodeFlag(0x80)) << NodeFlags{};
    CORRADE_COMPARE(out, "Ui::NodeFlag::Hidden|Ui::NodeFlag::Clip|Ui::NodeFlag(0x80) Ui::NodeFlags{}\n");
}

void NodeFlagsTest::debugFlagsPacked() {
    Containers::String out;
    /* Last is not packed, ones before should not make any flags persistent */
    Debug{&out} << Debug::packed << (NodeFlag::Hidden|NodeFlag::Clip|NodeFlag(0x80)) << Debug::packed << NodeFlags{} << (NodeFlag::Disabled|NodeFlag::NoBlur);
    CORRADE_COMPARE(out, "Hidden|Clip|0x80 {} Ui::NodeFlag::Disabled|Ui::NodeFlag::NoBlur\n");
}

void NodeFlagsTest::debugFlagsSupersets() {
    /* Disabled is a superset of NoEvents, so only one should be printed */
    {
        Containers::String out;
        Debug{&out} << (NodeFlag::Disabled|NodeFlag::NoEvents);
        CORRADE_COMPARE(out, "Ui::NodeFlag::Disabled\n");
    }
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::NodeFlagsTest)
