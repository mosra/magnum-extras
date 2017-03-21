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

#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/Container.h>

#include "Magnum/Ui/BasicInstancedLayer.hpp"

namespace Magnum { namespace Ui { namespace Test {

struct BasicInstancedLayerTest: TestSuite::Tester {
    explicit BasicInstancedLayerTest();

    void construct();

    void addElement();
    void addElementLast();
    void reset();
    void resetNoRealloc();
    void modifyElement();
};

BasicInstancedLayerTest::BasicInstancedLayerTest() {
    addTests({&BasicInstancedLayerTest::construct,

              &BasicInstancedLayerTest::addElement,
              &BasicInstancedLayerTest::addElementLast,
              &BasicInstancedLayerTest::reset,
              &BasicInstancedLayerTest::resetNoRealloc,
              &BasicInstancedLayerTest::modifyElement});
}

namespace {
    struct InstancedLayer: BasicInstancedLayer<Int> {
        #ifndef CORRADE_GCC47_COMPATIBILITY
        using BasicInstancedLayer<Int>::BasicInstancedLayer;
        #else
        explicit InstancedLayer() {}
        #endif
    };
}

void BasicInstancedLayerTest::construct() {
    InstancedLayer layer;

    CORRADE_COMPARE(layer.capacity(), 0);
    CORRADE_COMPARE(layer.size(), 0);
    CORRADE_VERIFY(layer.data().empty());
    CORRADE_VERIFY(layer.modified().size().isZero());
}

void BasicInstancedLayerTest::addElement() {
    InstancedLayer layer;
    layer.reset(42);

    CORRADE_COMPARE(layer.addElement(13), 0);
    CORRADE_COMPARE(layer.addElement(-7), 1);
    CORRADE_COMPARE(layer.addElement(2), 2);

    CORRADE_COMPARE(layer.capacity(), 42);
    CORRADE_COMPARE(layer.size(), 3);
    CORRADE_COMPARE_AS(layer.data(),
        (Containers::Array<Int>{Containers::InPlaceInit, {13, -7, 2}}),
        TestSuite::Compare::Container);
    CORRADE_COMPARE(layer.elementData(0), 13);
    CORRADE_COMPARE(layer.elementData(1), -7);
    CORRADE_COMPARE(layer.elementData(2), 2);
    CORRADE_COMPARE(layer.modified(), (Math::Range1D<std::size_t>{0, 3}));
}

void BasicInstancedLayerTest::addElementLast() {
    InstancedLayer layer;
    layer.reset(1);

    CORRADE_COMPARE(layer.addElement(13), 0);
    CORRADE_COMPARE(layer.capacity(), 1);
    CORRADE_COMPARE(layer.size(), 1);
}

void BasicInstancedLayerTest::reset() {
    InstancedLayer layer;
    layer.reset(10);

    CORRADE_COMPARE(layer.addElement(-7), 0);
    CORRADE_COMPARE(layer.capacity(), 10);
    CORRADE_COMPARE(layer.size(), 1);
    CORRADE_COMPARE_AS(layer.data(),
        (Containers::Array<Int>{Containers::InPlaceInit, {-7}}),
        TestSuite::Compare::Container);
    CORRADE_COMPARE(layer.modified(), (Math::Range1D<std::size_t>{0, 1}));

    layer.reset(13);

    /* The sizes and ranges should be back to zero after reset */
    CORRADE_COMPARE(layer.capacity(), 13);
    CORRADE_COMPARE(layer.size(), 0);
    CORRADE_VERIFY(layer.data().empty());
    CORRADE_VERIFY(layer.modified().size().isZero());

    /* First element after reset should be zero again */
    CORRADE_COMPARE(layer.addElement(-7), 0);
}

void BasicInstancedLayerTest::resetNoRealloc() {
    InstancedLayer layer;
    layer.reset(10);

    CORRADE_COMPARE(layer.addElement(-7), 0);
    CORRADE_COMPARE(layer.capacity(), 10);
    CORRADE_COMPARE(layer.size(), 1);

    layer.reset(3);

    /* The capacity stays the same if it was larger before */
    CORRADE_COMPARE(layer.capacity(), 10);
    CORRADE_COMPARE(layer.size(), 0);
    CORRADE_VERIFY(layer.data().empty());
    CORRADE_VERIFY(layer.modified().size().isZero());
}

void BasicInstancedLayerTest::modifyElement() {
    InstancedLayer layer;
    layer.reset(42);

    CORRADE_COMPARE(layer.addElement(13), 0);
    CORRADE_COMPARE(layer.addElement(-7), 1);
    CORRADE_COMPARE(layer.addElement(2), 2);
    CORRADE_COMPARE(layer.modified(), (Math::Range1D<std::size_t>{0, 3}));

    layer.resetModified();
    CORRADE_VERIFY(layer.modified().size().isZero());

    layer.modifyElement(2) = 17;
    layer.modifyElement(1) = 1337;

    CORRADE_COMPARE_AS(layer.data(),
        (Containers::Array<Int>{Containers::InPlaceInit, {13, 1337, 17}}),
        TestSuite::Compare::Container);
    /* The modified range should be just the two changed elements */
    CORRADE_COMPARE(layer.modified(), (Math::Range1D<std::size_t>{1, 3}));
}

}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::BasicInstancedLayerTest)
