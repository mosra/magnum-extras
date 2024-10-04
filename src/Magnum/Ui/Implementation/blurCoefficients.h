#ifndef Magnum_Ui_Implementation_blurCoefficients_h
#define Magnum_Ui_Implementation_blurCoefficients_h
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

#include <Corrade/Containers/ArrayView.h>
#include <Magnum/Math/Functions.h>

/* Extracted out of BaseLayerGL for easier testing */

namespace Magnum { namespace Ui { namespace {

/* Generates Gaussian blur weights for a radius given by size of the output
   field, in a descending order (i.e., so the first item is the coefficient at
   the center, the subsequent ones are further away). Fills only a prefix of
   the array where the coefficients are larger than given limit, returning
   prefix size. If the limit is so large that all coefficients fall below it,
   returns 0.

   Calculated using binomial coefficients, excluding weights that contribute
   less than given limit. Based on https://www.rastergrid.com/blog/2010/09/efficient-gaussian-blur-with-linear-sampling/ ;
   see https://dsp.stackexchange.com/questions/54375/how-to-approximate-gaussian-kernel-for-image-blur
   for details about how far the binomial coefficients are from the actual
   Gaussian. */
std::size_t blurCoefficientsInto(const Containers::ArrayView<Float> out, const Float limit) {
    /* Need a coefficient for the center at least. The math operates on 64-bit
       ints and the sum would overflow if N goes over 63. */
    CORRADE_INTERNAL_ASSERT(out.size() >= 1 && out.size() <= 32);
    const UnsignedInt n = 2*(out.size() - 1);

    /* Calculate the count and sum of coefficients that fall below the limit */
    const Float inverseSum = 1.0f/(1ull << n);
    UnsignedLong belowLimitSum = 0;
    UnsignedInt belowLimit = 0;
    for(; belowLimit != n/2; ++belowLimit) {
        const UnsignedLong coefficient = Math::binomialCoefficient(n, belowLimit);
        if(coefficient*inverseSum >= limit)
            break;
        belowLimitSum += coefficient;
    }

    /* There's always at least one value left due to how n is calculated, and
       it gets correctly normalized to 1 below so there doesn't need to be any
       special casing */
    CORRADE_INTERNAL_ASSERT(belowLimit < out.size());

    /* Generate all coefficients in a descending order, divide them by the
       total sum excluding the values below limit to keep the total sum of all
       coefficients equal to 1 */
    const Float inverseSumExceptBelowLimit = 1.0f/((1ull << n) - belowLimitSum*2);
    for(std::size_t i = 0; i != out.size() - belowLimit; ++i)
        out[out.size()- belowLimit - i - 1] = Math::binomialCoefficient(n, i + belowLimit)*inverseSumExceptBelowLimit;

    return out.size() - belowLimit;
}

/* Linear interpolation from https://www.rastergrid.com/blog/2010/09/efficient-gaussian-blur-with-linear-sampling/,
   further explanation at https://www.intel.com/content/www/us/en/developer/articles/technical/an-investigation-of-fast-real-time-gpu-based-image-blur-algorithms.html */
void interpolateBlurCoefficientsInto(const Containers::ArrayView<const Float> discrete, const Containers::ArrayView<Float> weights, const Containers::ArrayView<Float> offsets) {
    CORRADE_INTERNAL_ASSERT(discrete.size() >= 1 && weights.size() == (discrete.size() + 1)/2 && offsets.size() == weights.size());

    /* If there's an odd count of discrete coefficients, the first is tapped
       directly without interpolating */
    std::size_t i;
    if(discrete.size() % 2 == 1) {
        weights[0] = discrete[0];
        offsets[0] = 0.0f;
        i = 1;

    /* Otherwise the first weight includes only a half of the center weight as
       it's included twice */
    } else {
        weights[0] = discrete[0]*0.5f + discrete[1];
        /* Equation further simplified / const-propagated from the loop
           below */
        offsets[0] = discrete[1]/weights[0];
        i = 2;
    }

    /* From the rest take always a pair of two and find an offset between them
       that interpolates them at the same ratio as their weights are. The
       resulting weight for the interpolated sample is then a sum of the two
       weights. */
    for(std::size_t j = 1; j != weights.size(); ++j, i += 2) {
        weights[j] = discrete[i] + discrete[i + 1];
        /* Simplified from `(i*d[i] + (i + 1)*d[i + 1])/w[j]` which was in the
           article, makes more sense as the interpolation factor isn't really
           depending on the offset but rather the ratio of the neighboring
           discrete weights */
        offsets[j] = i + discrete[i + 1]/weights[j];
    }
    CORRADE_INTERNAL_ASSERT(i == discrete.size());
}

}}}

#endif
