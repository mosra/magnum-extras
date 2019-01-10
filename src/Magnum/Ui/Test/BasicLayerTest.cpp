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

#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/Container.h>

#include "Magnum/Ui/BasicLayer.hpp"

namespace Magnum { namespace Ui { namespace Test {

struct BasicLayerTest: TestSuite::Tester {
    explicit BasicLayerTest();

    void construct();

    void addElement();
    void addElementLast();
    void reset();
    void resetNoReallocData();
    void resetNoReallocElementData();
    void modifyElement();
};

BasicLayerTest::BasicLayerTest() {
    addTests({&BasicLayerTest::construct,

              &BasicLayerTest::addElement,
              &BasicLayerTest::addElementLast,
              &BasicLayerTest::reset,
              &BasicLayerTest::resetNoReallocData,
              &BasicLayerTest::resetNoReallocElementData,
              &BasicLayerTest::modifyElement});
}

namespace {
    struct Layer: BasicLayer<Int> {
        using BasicLayer<Int>::BasicLayer;
    };
}

void BasicLayerTest::construct() {
    Layer layer;

    CORRADE_COMPARE(layer.capacity(), 0);
    CORRADE_COMPARE(layer.elementCapacity(), 0);
    CORRADE_COMPARE(layer.size(), 0);
    CORRADE_COMPARE(layer.elementCount(), 0);
    CORRADE_COMPARE(layer.indexCount(), 0);
    CORRADE_VERIFY(layer.data().empty());
    CORRADE_VERIFY(!layer.modified().size());
}

void BasicLayerTest::addElement() {
    Layer layer;
    layer.reset(7, 42);

    CORRADE_COMPARE(layer.addElement(
        Containers::Array<Int>{Containers::InPlaceInit, {13, -5, 27}}, 3), 0);
    CORRADE_COMPARE(layer.addElement(
        Containers::Array<Int>{Containers::InPlaceInit, {23, 17, 57, 0}}, 6), 1);
    CORRADE_COMPARE(layer.addElement(
        Containers::Array<Int>{Containers::InPlaceInit, {1}}, 1), 2);

    CORRADE_COMPARE(layer.capacity(), 42);
    CORRADE_COMPARE(layer.elementCapacity(), 7);
    CORRADE_COMPARE(layer.size(), 8);
    CORRADE_COMPARE(layer.elementCount(), 3);
    CORRADE_COMPARE(layer.indexCount(), 10);
    CORRADE_COMPARE_AS(layer.data(),
        (Containers::Array<Int>{Containers::InPlaceInit, {13, -5, 27, 23, 17, 57, 0, 1}}),
        TestSuite::Compare::Container);
    CORRADE_COMPARE(layer.elementSize(0), 3);
    CORRADE_COMPARE_AS(layer.elementData(0),
        (Containers::Array<Int>{Containers::InPlaceInit, {13, -5, 27}}),
        TestSuite::Compare::Container);
    CORRADE_COMPARE(layer.elementSize(1), 4);
    CORRADE_COMPARE_AS(layer.elementData(1),
        (Containers::Array<Int>{Containers::InPlaceInit, {23, 17, 57, 0}}),
        TestSuite::Compare::Container);
    CORRADE_COMPARE(layer.elementSize(2), 1);
    CORRADE_COMPARE_AS(layer.elementData(2),
        (Containers::Array<Int>{Containers::InPlaceInit, {1}}),
        TestSuite::Compare::Container);
    CORRADE_COMPARE(layer.modified(), (Math::Range1D<std::size_t>{0, 8}));
}

void BasicLayerTest::addElementLast() {
    Layer layer;
    layer.reset(1, 4);

    CORRADE_COMPARE(layer.addElement(
        Containers::Array<Int>{Containers::InPlaceInit, {23, 17, 57, 0}}, 6), 0);
    CORRADE_COMPARE(layer.capacity(), 4);
    CORRADE_COMPARE(layer.elementCapacity(), 1);
    CORRADE_COMPARE(layer.size(), 4);
    CORRADE_COMPARE(layer.elementCount(), 1);
    CORRADE_COMPARE(layer.indexCount(), 6);
}

void BasicLayerTest::reset() {
    Layer layer;
    layer.reset(3, 10);

    CORRADE_COMPARE(layer.addElement(
        Containers::Array<Int>{Containers::InPlaceInit, {-7}}, 11), 0);
    CORRADE_COMPARE(layer.capacity(), 10);
    CORRADE_COMPARE(layer.elementCapacity(), 3);
    CORRADE_COMPARE(layer.size(), 1);
    CORRADE_COMPARE(layer.elementCount(), 1);
    CORRADE_COMPARE(layer.indexCount(), 11);
    CORRADE_COMPARE_AS(layer.data(),
        (Containers::Array<Int>{Containers::InPlaceInit, {-7}}),
        TestSuite::Compare::Container);
    CORRADE_COMPARE(layer.modified(), (Math::Range1D<std::size_t>{0, 1}));

    layer.reset(5, 13);

    /* The sizes, ranges and counts should be back to zero after reset */
    CORRADE_COMPARE(layer.capacity(), 13);
    CORRADE_COMPARE(layer.elementCapacity(), 5);
    CORRADE_COMPARE(layer.size(), 0);
    CORRADE_COMPARE(layer.elementCount(), 0);
    CORRADE_COMPARE(layer.indexCount(), 0);
    CORRADE_VERIFY(layer.data().empty());
    CORRADE_VERIFY(!layer.modified().size());

    /* First element after reset should be zero again */
    CORRADE_COMPARE(layer.addElement(
        Containers::Array<Int>{Containers::InPlaceInit, {-7}}, 11), 0);
}

void BasicLayerTest::resetNoReallocData() {
    Layer layer;
    layer.reset(3, 42);

    CORRADE_COMPARE(layer.addElement(
        Containers::Array<Int>{Containers::InPlaceInit, {-7}}, 11), 0);
    CORRADE_COMPARE(layer.capacity(), 42);
    CORRADE_COMPARE(layer.elementCapacity(), 3);
    CORRADE_COMPARE(layer.size(), 1);
    CORRADE_COMPARE(layer.elementCount(), 1);
    CORRADE_COMPARE(layer.indexCount(), 11);

    layer.reset(7, 15);

    CORRADE_COMPARE(layer.capacity(), 42);
    CORRADE_COMPARE(layer.elementCapacity(), 7);
    CORRADE_COMPARE(layer.size(), 0);
    CORRADE_COMPARE(layer.elementCount(), 0);
    CORRADE_COMPARE(layer.indexCount(), 0);
    CORRADE_VERIFY(layer.data().empty());
    CORRADE_VERIFY(!layer.modified().size());
}

void BasicLayerTest::resetNoReallocElementData() {
    Layer layer;
    layer.reset(3, 10);

    CORRADE_COMPARE(layer.addElement(
        Containers::Array<Int>{Containers::InPlaceInit, {-7}}, 11), 0);
    CORRADE_COMPARE(layer.capacity(), 10);
    CORRADE_COMPARE(layer.elementCapacity(), 3);
    CORRADE_COMPARE(layer.size(), 1);
    CORRADE_COMPARE(layer.elementCount(), 1);
    CORRADE_COMPARE(layer.indexCount(), 11);

    layer.reset(1, 15);

    CORRADE_COMPARE(layer.capacity(), 15);
    CORRADE_COMPARE(layer.elementCapacity(), 3);
    CORRADE_COMPARE(layer.size(), 0);
    CORRADE_COMPARE(layer.elementCount(), 0);
    CORRADE_COMPARE(layer.indexCount(), 0);
    CORRADE_VERIFY(layer.data().empty());
    CORRADE_VERIFY(!layer.modified().size());
}

void BasicLayerTest::modifyElement() {
    Layer layer;
    layer.reset(17, 42);

    CORRADE_COMPARE(layer.addElement(
        Containers::Array<Int>{Containers::InPlaceInit, {13, -5, 27}}, 3), 0);
    CORRADE_COMPARE(layer.addElement(
        Containers::Array<Int>{Containers::InPlaceInit, {23, 17, 57, 0}}, 6), 1);
    CORRADE_COMPARE(layer.addElement(
        Containers::Array<Int>{Containers::InPlaceInit, {1}}, 1), 2);
    CORRADE_COMPARE(layer.modified(), (Math::Range1D<std::size_t>{0, 8}));

    layer.resetModified();
    CORRADE_VERIFY(!layer.modified().size());

    layer.modifyElement(2)[0] = 2555;
    layer.modifyElement(1)[2] = 5704;

    CORRADE_COMPARE_AS(layer.data(),
        (Containers::Array<Int>{Containers::InPlaceInit, {
            13, -5, 27, 23, 17, 5704, 0, 2555}}),
        TestSuite::Compare::Container);
    /* The modified range should be just the two changed elements */
    CORRADE_COMPARE(layer.modified(), (Math::Range1D<std::size_t>{3, 8}));
}

}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::BasicLayerTest)
