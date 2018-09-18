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

struct OctreeBenchmark: TestSuite::Tester {
    explicit OctreeBenchmark();

    void baselineBuild();
    void benchmarkBuild();

    void baselinePointsRange();
    void benchmarkPointsRange();

    void baselinePointsFrustum();
    void benchmarkPointsFrustum();

    const size_t DATA_SIZE = 2048;
    Containers::Array<Range3D> boundingBoxes{Containers::NoInit, DATA_SIZE};
    Containers::Array<Int> entries{Containers::NoInit, DATA_SIZE};
};

OctreeBenchmark::OctreeBenchmark() {
    addBenchmarks({&OctreeBenchmark::baselineBuild,
                   &OctreeBenchmark::benchmarkBuild,

                   &OctreeBenchmark::baselinePointsRange,
                   &OctreeBenchmark::benchmarkPointsRange,

                   &OctreeBenchmark::baselinePointsFrustum,
                   &OctreeBenchmark::benchmarkPointsFrustum}, 25);


    /* Generate random data for the benchmarks */
    std::random_device rnd;
    std::mt19937 g(rnd());
    /* Position distribution */
    std::uniform_real_distribution<float> pd(-10.0f, 10.0f);
    /* Size distribution */
    std::uniform_real_distribution<float> sd(0.05f, 0.05f);

    for(int i = 0; i < DATA_SIZE; ++i) {
        const Vector3 pos{pd(g), pd(g), pd(g)};
        const Vector3 size{sd(g), sd(g), sd(g)};

        boundingBoxes[i] = {pos, pos + size};
        entries[i] = i;
    }
}

void OctreeBenchmark::baselineBuild() {
    int size = 0;
    CORRADE_BENCHMARK(25) {
        std::vector<Int> objects;
        for(Int e: entries) {
            objects.push_back(e);
            size += e;
        }

        size += objects.size();
    }

    volatile int result = size;
}

void OctreeBenchmark::benchmarkBuild() {
    Octree<Int> octree;
    CORRADE_BENCHMARK(25) {
        octree = Octree<Int>(boundingBoxes, entries, 4);
    }
}

void OctreeBenchmark::baselinePointsRange() {
    Int objectsCount = 0;
    std::vector<Int> resultData;
    resultData.reserve(entries.size());

    CORRADE_BENCHMARK(25) {
        Range3D range({-10.0f, -3.0f, -5.0}, {-4.0f, 3.0f, 1.0f});

        resultData.clear();
        for(int i = 0; i < boundingBoxes.size(); ++i) {
            auto& bb = boundingBoxes[i];
            if((bb.max() < range.min()).any()) continue;
            if((bb.min() > range.max()).any()) continue;

            resultData.push_back(entries[i]);
        }
        objectsCount += resultData.size();
    }
    volatile Int result = objectsCount;
}

void OctreeBenchmark::benchmarkPointsRange() {
    Octree<Int> octree{boundingBoxes, entries, 5};

    std::vector<Int> resultData;
    resultData.reserve(entries.size());

    const Range3D range({-10.0f, -3.0f, -5.0}, {-4.0f, 3.0f, 1.0f});

    Int objectsCount = 0;
    CORRADE_BENCHMARK(25) {
        resultData.clear();

        octree.points(resultData, range);
        objectsCount += resultData.size();
    }

    volatile Int result = objectsCount;
}

void OctreeBenchmark::baselinePointsFrustum() {
    Int objectsCount = 0;
    std::vector<Int> resultData;
    resultData.reserve(entries.size());

    const Math::Frustum<Float> frustum{
        {1.0f, 0.0f, 0.0f, 0.0f},
        {-1.0f, 0.0f, 0.0f, 10.0f},
        {0.0f, -1.0f, 0.0f, 10.0f},
        {0.0f, 1.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, -1.0f, 10.0f},
        {0.0f, 0.0f, 1.0f, 0.0f}};

    CORRADE_BENCHMARK(25) {
        resultData.clear();
        for(int i = 0; i < boundingBoxes.size(); ++i) {
            auto& bb = boundingBoxes[i];
            // TODO: rangeFrustum
            if(!Math::Geometry::Intersection::boxFrustum(bb, frustum)) continue;

            resultData.push_back(entries[i]);
        }
        objectsCount += resultData.size();
    }
    volatile Int result = objectsCount;
}

void OctreeBenchmark::benchmarkPointsFrustum() {
    Octree<Int> octree{boundingBoxes, entries, 2};

    std::vector<Int> resultData;
    resultData.reserve(entries.size());

    const Math::Frustum<Float> frustum{
        {1.0f, 0.0f, 0.0f, 0.0f},
        {-1.0f, 0.0f, 0.0f, 10.0f},
        {0.0f, -1.0f, 0.0f, 10.0f},
        {0.0f, 1.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, -1.0f, 10.0f},
        {0.0f, 0.0f, 1.0f, 0.0f}};

    Int objectsCount = 0;
    CORRADE_BENCHMARK(25) {
        resultData.clear();

        octree.points(resultData, frustum);
        objectsCount += resultData.size();
    }

    volatile Int result = objectsCount;
}

}}}

CORRADE_TEST_MAIN(Magnum::Octree::Test::OctreeBenchmark)
