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

#include <Corrade/TestSuite/Tester.h>

#include "Magnum/Ui/LineLayer.h"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct LineLayerCpp14Test: TestSuite::Tester {
    explicit LineLayerCpp14Test();

    void styleUniformCommonSettersConstexpr();
    void styleUniformSettersConstexpr();
};

LineLayerCpp14Test::LineLayerCpp14Test() {
    addTests({&LineLayerCpp14Test::styleUniformCommonSettersConstexpr,
              &LineLayerCpp14Test::styleUniformSettersConstexpr});
}

using namespace Math::Literals;

void LineLayerCpp14Test::styleUniformCommonSettersConstexpr() {
    constexpr LineLayerCommonStyleUniform a = LineLayerCommonStyleUniform{}
        .setSmoothness(1.0f);
    CORRADE_COMPARE(a.smoothness, 1.0f);
}

void LineLayerCpp14Test::styleUniformSettersConstexpr() {
    constexpr LineLayerStyleUniform a = LineLayerStyleUniform{}
        .setColor(0xff336699_rgbaf)
        .setWidth(3.0f)
        .setSmoothness(15.0f)
        .setMiterLimit(3.7654f);
    CORRADE_COMPARE(a.color, 0xff336699_rgbaf);
    CORRADE_COMPARE(a.width, 3.0f);
    CORRADE_COMPARE(a.smoothness, 15.0f);
    CORRADE_COMPARE(a.miterLimit, 3.7654f);
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::LineLayerCpp14Test)
