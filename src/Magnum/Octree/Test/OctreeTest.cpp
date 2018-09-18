/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018
              Vladimír Vondruš <mosra@centrum.cz>
    Copyright © 2015 Andrea Capobianco <andrea.c.2205@gmail.com>
    Copyright © 2018 Jonathan Hale <squareys@googlemail.com>

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

#include <Magnum/Magnum.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Geometry/Intersection.h>

#include <Corrade/TestSuite/Compare/Container.h>
#include <Corrade/Containers/Array.h>

#include "Magnum/Octree/Octree.h"
#include "Magnum/Octree/Octree.hpp"

#include <random>

using namespace Corrade;

namespace Magnum { namespace Octree { namespace Test {

struct OctreeTest: TestSuite::Tester {
    explicit OctreeTest();

    void constructEmpty();

    void build();

    void pointsRange();
    void pointsFrustum();
};

OctreeTest::OctreeTest() {
    addTests({&OctreeTest::build,

              &OctreeTest::pointsRange,
              &OctreeTest::pointsFrustum});
}

void OctreeTest::constructEmpty() {
    {
        Octree<Int> o;

        CORRADE_VERIFY(o.empty());
        CORRADE_VERIFY(o.boundingBoxes().empty());
        CORRADE_VERIFY(o.data().empty());
    }
    {
        Octree<std::string> o{{}, {}, 0};
        CORRADE_COMPARE(o.data().size(), 0);
        CORRADE_VERIFY(o.boundingBoxes().empty());
        CORRADE_VERIFY(o.data().empty());
    }
}

void OctreeTest::build() {
    {
        /* Build with single element */
        Containers::Array<Range3D> boundingBoxes{Containers::DirectInit, 1, Range3D(Vector3{-1.0f}, Vector3{1.0f})};
        Containers::Array<Int> data{1};

        Octree<Int> octree{boundingBoxes, data, 1};

        CORRADE_COMPARE(octree.data().size(), 1);
    }
    {
        /* Should create a second level:
         *
         * +---+---+-------+
         * | 0 |   |       |
         * +---+---+       |
         * |   | 2 |       |
         * +---+---+-------+
         * |       |       |
         * |       |   1   |
         * |       |       |
         * +-------+-------+
         */
        Range3D boundingBoxes[]{
            Range3D(Vector3{-1.0f}, Vector3{-0.5f}),
            Range3D(Vector3{0.0f}, Vector3{1.0f}),
            Range3D(Vector3{-0.5f}, Vector3{0.0f})};
        Int data[]{0, 1, 2};

        Octree<Int> octree{Containers::arrayView(boundingBoxes),
                           Containers::arrayView(data), 3};

        CORRADE_COMPARE(octree.data().size(), 3);
    }
}

void OctreeTest::pointsRange() {
    Range3D boundingBoxes[]{
        Range3D(Vector3{-1.0f}, Vector3{-0.5f}),
        Range3D(Vector3{0.0f}, Vector3{1.0f}),
        Range3D(Vector3{-0.5f}, Vector3{0.0f})};
    Int data[]{0, 1, 2};

    Octree<Int> octree{Containers::arrayView(boundingBoxes),
                       Containers::arrayView(data), 3};

    std::vector<Int> result;
    octree.points(result, Range3D{Vector3{-1.0f}, Vector3{1.0f}});
    CORRADE_COMPARE(result.size(), 2);
    CORRADE_COMPARE(result[0], 0);
    CORRADE_COMPARE(result[1], 2);
}

void OctreeTest::pointsFrustum() {
    Range3D aabbs[]{
        {Vector3{0.0f}, Vector3{1.0f}},
        {Vector3{2.0f}, Vector3{4.0f}},
        {Vector3{1.0f}, Vector3{3.0f}} };

    char data[]{'a', 'b', 'c'};

    Octree<char> octree{Containers::arrayView(aabbs, 3),
                        Containers::arrayView(data, 3), 4};

    Math::Frustum<Float> frustum{
        {1.0f, 0.0f, 0.0f, 0.0f},
        {-1.0f, 0.0f, 0.0f, 10.0f},
        {0.0f, -1.0f, 0.0f, 10.0f},
        {0.0f, 1.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, -1.0f, 10.0f},
        {0.0f, 0.0f, 1.0f, 0.0f}};

    std::vector<char> containedData;
    octree.points(containedData, frustum);

    std::vector<char> expected{'a', 'b', 'c'};
    CORRADE_COMPARE_AS(containedData, expected, TestSuite::Compare::Container);
}

}}}

CORRADE_TEST_MAIN(Magnum::Octree::Test::OctreeTest)
