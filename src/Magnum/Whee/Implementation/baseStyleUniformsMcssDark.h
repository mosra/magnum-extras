/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023 Vladimír Vondruš <mosra@centrum.cz>

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

/* BaseStyle enum name, then BaseLayerStyleUniform constructor arguments. Put
   into a dedicated file to be able to put as much as possible into constexpr
   arrays without losing the correspondence between BaseStyle and the actual
   items. Matching order tested in StyleTest, see all #includes of this file in
   Style.cpp for actual uses. */

#ifdef _c
_c(ButtonDefaultInactiveBlur,   0xdcdcdcff_rgbaf*0.8f, 4.0f)
_c(ButtonDefaultInactiveHover,  0xa5c9eaff_rgbaf*0.8f, 4.0f)
_c(ButtonDefaultPressedBlur,    0xa5c9eaff_rgbaf*0.8f, 4.0f)
_c(ButtonDefaultPressedHover,   0xa5c9eaff_rgbaf*0.8f, 4.0f)
_c(ButtonDefaultDisabled,       0xdcdcdcff_rgbaf*0.3f, 4.0f)

_c(ButtonPrimaryInactiveBlur,   0xa5c9eaff_rgbaf*0.8f, 4.0f)
_c(ButtonPrimaryInactiveHover,  0xdcdcdcff_rgbaf*0.8f, 4.0f)
_c(ButtonPrimaryPressedBlur,    0xdcdcdcff_rgbaf*0.8f, 4.0f)
_c(ButtonPrimaryPressedHover,   0xdcdcdcff_rgbaf*0.8f, 4.0f)
_c(ButtonPrimaryDisabled,       0xa5c9eaff_rgbaf*0.3f, 4.0f)

_c(ButtonSuccessInactiveBlur,   0x3bd267ff_rgbaf*0.8f, 4.0f)
_c(ButtonSuccessInactiveHover,  0xacecbeff_rgbaf*0.8f, 4.0f)
_c(ButtonSuccessPressedBlur,    0xacecbeff_rgbaf*0.8f, 4.0f)
_c(ButtonSuccessPressedHover,   0xacecbeff_rgbaf*0.8f, 4.0f)
_c(ButtonSuccessDisabled,       0x3bd267ff_rgbaf*0.3f, 4.0f)

_c(ButtonWarningInactiveBlur,   0xc7cf2fff_rgbaf*0.8f, 4.0f)
_c(ButtonWarningInactiveHover,  0xe9ecaeff_rgbaf*0.8f, 4.0f)
_c(ButtonWarningPressedBlur,    0xe9ecaeff_rgbaf*0.8f, 4.0f)
_c(ButtonWarningPressedHover,   0xe9ecaeff_rgbaf*0.8f, 4.0f)
_c(ButtonWarningDisabled,       0xc7cf2fff_rgbaf*0.3f, 4.0f)

_c(ButtonDangerInactiveBlur,    0xcd3431ff_rgbaf*0.8f, 4.0f)
_c(ButtonDangerInactiveHover,   0xff9391ff_rgbaf*0.8f, 4.0f)
_c(ButtonDangerPressedBlur,     0xff9391ff_rgbaf*0.8f, 4.0f)
_c(ButtonDangerPressedHover,    0xff9391ff_rgbaf*0.8f, 4.0f)
_c(ButtonDangerDisabled,        0xcd3431ff_rgbaf*0.3f, 4.0f)

_c(ButtonInfoInactiveBlur,      0x2f83ccff_rgbaf*0.8f, 4.0f)
_c(ButtonInfoInactiveHover,     0xa5caebff_rgbaf*0.8f, 4.0f)
_c(ButtonInfoPressedBlur,       0xa5caebff_rgbaf*0.8f, 4.0f)
_c(ButtonInfoPressedHover,      0xa5caebff_rgbaf*0.8f, 4.0f)
_c(ButtonInfoDisabled,          0x2f83ccff_rgbaf*0.3f, 4.0f)

_c(ButtonDimInactiveBlur,       0x747474ff_rgbaf*0.8f, 4.0f)
_c(ButtonDimInactiveHover,      0xacacacff_rgbaf*0.8f, 4.0f)
_c(ButtonDimPressedBlur,        0xacacacff_rgbaf*0.8f, 4.0f)
_c(ButtonDimPressedHover,       0xacacacff_rgbaf*0.8f, 4.0f)
_c(ButtonDimDisabled,           0x747474ff_rgbaf*0.3f, 4.0f)

_c(ButtonFlatInactiveBlur,      {}, {}, {}, 0.0f, 4.0f, 0.0f)
_c(ButtonFlatInactiveHover,     {}, {}, 0xd0d9f7_rgbf, 1.5f, 4.0f, 2.5f)
_c(ButtonFlatPressedBlur,       {}, {}, 0xd0d9f7_rgbf, 1.5f, 4.0f, 2.5f)
_c(ButtonFlatPressedHover,      {}, {}, 0xd0d9f7_rgbf, 1.5f, 4.0f, 2.5f)
_c(ButtonFlatDisabled,          {}, {}, {}, 0.0f, 4.0f, 0.0f)
#endif
