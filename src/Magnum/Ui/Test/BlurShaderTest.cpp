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

#include <Corrade/Containers/Array.h>
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/Numeric.h>

#include "Magnum/Ui/Implementation/blurCoefficients.h"

namespace Magnum { namespace Ui { namespace {

struct BlurShaderTest: TestSuite::Tester {
    explicit BlurShaderTest();

    void blurCoefficients();
    void blurCoefficientsLimitTooLarge();

    void interpolatedBlurCoefficients();
};

const struct {
    const char* name;
    Float limit;
} BlurCoefficientsData[]{
    {"limit 0.5/255", 0.5f/255.0f},
    {"limit 0.5/65535", 0.5f/65535.0f},
    {"limit 0.5/1048576", 0.5f/1048576.0f},
    {"limit 0", 0.0f},
};

const struct {
    const char* name;
    UnsignedInt radius;
    UnsignedInt discreteCount;
    UnsignedInt interpolatedCount;
} InterpolatedBlurCoefficientsData[]{
    {"even, 6 coefficients, 3 interpolated", 8, 6, 3},
    {"odd, 7 coefficients, 4 interpolated, first at the center", 10, 7, 4},
};

BlurShaderTest::BlurShaderTest() {
    addInstancedTests({&BlurShaderTest::blurCoefficients},
        Containers::arraySize(BlurCoefficientsData));

    addTests({&BlurShaderTest::blurCoefficientsLimitTooLarge});

    addInstancedTests({&BlurShaderTest::interpolatedBlurCoefficients},
        Containers::arraySize(InterpolatedBlurCoefficientsData));
}

void BlurShaderTest::blurCoefficients() {
    auto&& data = BlurCoefficientsData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    UnsignedInt radius = 0;
    for(; radius != 32; ++radius) {
        CORRADE_ITERATION("radius" << radius);

        Containers::Array<Float> storage{NoInit, radius + 1};
        Containers::ArrayView<const Float> out = storage.prefix(blurCoefficientsInto(storage, data.limit));
        CORRADE_VERIFY(!out.isEmpty());

        /* The values should be monotonically decreasing */
        for(std::size_t i = 0; i != out.size() - 1; ++i) {
            CORRADE_ITERATION(i);
            CORRADE_COMPARE_AS(out[i], out[i + 1],
                TestSuite::Compare::Greater);
        }

        /* The total sum (it's symmetric, so all coefficients except the first
           one twice, the first one once) should be exactly 1 to avoid
           brightening/darkening the image */
        Float sum = 0.0f;
        for(Float i: out.exceptPrefix(1))
            sum += i;
        CORRADE_COMPARE(out.front() + 2.0f*sum, 1.0f);

        /* The last value should be greater than the limit */
        CORRADE_COMPARE_AS(out.back(), data.limit,
            TestSuite::Compare::GreaterOrEqual);

        /** @todo have CORRADE_VERBOSE() for this */
        // CORRADE_INFO(out);

        /* In the non-cut-off case, the values should be not too far from an
           actual sampled Gaussian. (In the other cases, the values are
           renormalized to sum to 1 again, which makes them more different.)
           Equation taken from the following detailed answer because my math is
           extremely fuzzy. The `s` is adjusted based on the radius and the
           whole series renormalized:
            https://dsp.stackexchange.com/questions/54375/how-to-approximate-gaussian-kernel-for-image-blur */
        if(data.limit == 0.0f) {
            Containers::Array<Float> sampled{NoInit, radius + 1};
            Float s = Math::sqrt((radius*2 + 1)/2.0f);
            for(std::size_t x = 0; x != sampled.size(); ++x) {
                /* The cast to float is important as otherwise it'll negate an
                   unsigned value, resulting in a range error in exp() */
                sampled[x] = 1.0f/(s*Math::sqrt(Constants::pi()))*Math::exp(-Float(x*x)/(s*s));
            }

            /* Renormalize so the sum is 1.0, similarly to the check above */
            Float sampledSum = 0.0f;
            for(Float i: sampled.exceptPrefix(1))
                sampledSum += i;
            Float renormalization = 1.0f/(sampled.front() + 2.0f*sampledSum);

            /* The bigger the radius, the closer to the sampled value it should
               be */
            for(std::size_t x = 0; x != out.size(); ++x) {
                CORRADE_ITERATION(x);
                CORRADE_COMPARE_WITH(out[x],
                    sampled[x]*renormalization,
                    TestSuite::Compare::around(Math::pow(10.0f, Math::lerp(-1.0f, -5.5f, radius/64.0f))));
            }
        }
    }
}

void BlurShaderTest::blurCoefficientsLimitTooLarge() {
    /* It always outputs at least one value, even if the limit is too large.
       Failing in this case would be worse UX. */
    Float out[16];
    CORRADE_COMPARE(blurCoefficientsInto(out, 8.95f), 1);
    CORRADE_COMPARE(out[0], 1.0f);
}

void BlurShaderTest::interpolatedBlurCoefficients() {
    auto&& data = InterpolatedBlurCoefficientsData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    Containers::Array<Float> discrete{NoInit, data.radius};
    std::size_t count = blurCoefficientsInto(discrete, 0.5f/255.0f);
    CORRADE_COMPARE(count, data.discreteCount);

    Containers::Array<Float> weights{NoInit, data.interpolatedCount};
    Containers::Array<Float> offsets{NoInit, data.interpolatedCount};
    interpolateBlurCoefficientsInto(Containers::arrayView(discrete).prefix(count), weights, offsets);

    /* If there's an odd number of discrete coefficients, the first value is
       directly at the center pixel (wasting one interpolator) */
    if(data.discreteCount % 2 == 1) {
        CORRADE_COMPARE(weights[0], discrete[0]);
        CORRADE_COMPARE(offsets[0], 0.0f);
    }

    /* Interpolating the weights at given offsets should result in the same
       values as the neighboring discrete values */
    for(std::size_t i = 0; i != weights.size(); ++i) {
        CORRADE_ITERATION(i);

        UnsignedInt discreteIndex = offsets[i];
        Float factor = offsets[i] - discreteIndex;
        /* In case this the first tap isn't at pixel center, only a half of
           the center weight is used because it's included twice */
        if(i != 0 || offsets[0] == 0.0f)
            CORRADE_COMPARE((1.0f - factor)*weights[i], discrete[discreteIndex]);
        else
            CORRADE_COMPARE((1.0f - factor)*weights[i], discrete[discreteIndex]*0.5f);

        /* There's no second interpolated value in case this is the first tap
           at pixel center */
        if(i != 0 || offsets[0] != 0.0f)
            CORRADE_COMPARE(factor*weights[i], discrete[discreteIndex + 1]);
    }
}

}}}

CORRADE_TEST_MAIN(Magnum::Ui::BlurShaderTest)
