/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023, 2024
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

#include "Magnum/Ui/TextLayer.h"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct TextLayerCpp14Test: TestSuite::Tester {
    explicit TextLayerCpp14Test();

    void styleUniformCommonSettersConstexpr();
    void styleUniformSettersConstexpr();
};

TextLayerCpp14Test::TextLayerCpp14Test() {
    addTests({&TextLayerCpp14Test::styleUniformCommonSettersConstexpr,
              &TextLayerCpp14Test::styleUniformSettersConstexpr});
}

using namespace Math::Literals;

void TextLayerCpp14Test::styleUniformCommonSettersConstexpr() {
    CORRADE_SKIP("No setters so far.");
}

void TextLayerCpp14Test::styleUniformSettersConstexpr() {
    constexpr TextLayerStyleUniform a = TextLayerStyleUniform{}
        .setColor(0xff663399_rgbaf);
    CORRADE_COMPARE(a.color, 0xff663399_rgbaf);
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::TextLayerCpp14Test)
