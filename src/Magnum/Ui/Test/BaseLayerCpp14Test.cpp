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

#include "Magnum/Ui/BaseLayer.h"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct BaseLayerCpp14Test: TestSuite::Tester {
    explicit BaseLayerCpp14Test();

    void styleUniformCommonSettersConstexpr();
    void styleUniformSettersConstexpr();
};

BaseLayerCpp14Test::BaseLayerCpp14Test() {
    addTests({&BaseLayerCpp14Test::styleUniformCommonSettersConstexpr,
              &BaseLayerCpp14Test::styleUniformSettersConstexpr});
}

using namespace Math::Literals;

void BaseLayerCpp14Test::styleUniformCommonSettersConstexpr() {
    constexpr BaseLayerCommonStyleUniform a = BaseLayerCommonStyleUniform{}
        .setSmoothness(1.0f, 2.0f)
        .setBackgroundBlurAlpha(0.3f);
    CORRADE_COMPARE(a.smoothness, 1.0f);
    CORRADE_COMPARE(a.innerOutlineSmoothness, 2.0f);
    CORRADE_COMPARE(a.backgroundBlurAlpha, 0.3f);

    /* Single smoothness value */
    constexpr BaseLayerCommonStyleUniform b = BaseLayerCommonStyleUniform{}
        .setSmoothness(1.5f);
    CORRADE_COMPARE(b.smoothness, 1.5f);
    CORRADE_COMPARE(b.innerOutlineSmoothness, 1.5f);
}

void BaseLayerCpp14Test::styleUniformSettersConstexpr() {
    constexpr BaseLayerStyleUniform a = BaseLayerStyleUniform{}
        .setColor(0xff336699_rgbaf, 0x996633ff_rgbaf)
        .setOutlineColor(0xaabbccdd_rgbaf)
        .setOutlineWidth({1.0f, 2.0f, 3.0f, 4.0f})
        .setCornerRadius({0.1f, 0.2f, 0.3f, 0.4f})
        .setInnerOutlineCornerRadius({5.0f, 6.0f, 7.0f, 8.0f});
    CORRADE_COMPARE(a.topColor, 0xff336699_rgbaf);
    CORRADE_COMPARE(a.bottomColor, 0x996633ff_rgbaf);
    CORRADE_COMPARE(a.outlineColor, 0xaabbccdd_rgbaf);
    CORRADE_COMPARE(a.outlineWidth, (Vector4{1.0f, 2.0f, 3.0f, 4.0f}));
    CORRADE_COMPARE(a.cornerRadius, (Vector4{0.1f, 0.2f, 0.3f, 0.4f}));
    CORRADE_COMPARE(a.innerOutlineCornerRadius, (Vector4{5.0f, 6.0f, 7.0f, 8.0f}));

    /* Single color */
    constexpr BaseLayerStyleUniform b = BaseLayerStyleUniform{}
        .setColor(0xff663399_rgbaf);
    CORRADE_COMPARE(b.topColor, 0xff663399_rgbaf);
    CORRADE_COMPARE(b.bottomColor, 0xff663399_rgbaf);

    /* Single corner radius and outline width value */
    constexpr BaseLayerStyleUniform c = BaseLayerStyleUniform{}
        .setOutlineWidth(2.5f)
        .setCornerRadius(3.5f)
        .setInnerOutlineCornerRadius(1.5f);
    CORRADE_COMPARE(c.outlineWidth, Vector4{2.5f});
    CORRADE_COMPARE(c.cornerRadius, Vector4{3.5f});
    CORRADE_COMPARE(c.innerOutlineCornerRadius, Vector4{1.5f});
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::BaseLayerCpp14Test)
