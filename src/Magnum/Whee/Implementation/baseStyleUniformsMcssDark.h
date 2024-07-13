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
_c(ButtonDefaultInactiveOut,    0xdcdcdcff_rgbaf*0.8f, 4.0f)
_c(ButtonDefaultInactiveOver,   0xa5c9eaff_rgbaf*0.8f, 4.0f)
_c(ButtonDefaultPressedOut,     0xa5c9eaff_rgbaf*0.8f, 4.0f)
_c(ButtonDefaultPressedOver,    0xa5c9eaff_rgbaf*0.8f, 4.0f)
_c(ButtonDefaultDisabled,       0xdcdcdcff_rgbaf*0.3f, 4.0f)

_c(ButtonPrimaryInactiveOut,    0xa5c9eaff_rgbaf*0.8f, 4.0f)
_c(ButtonPrimaryInactiveOver,   0xdcdcdcff_rgbaf*0.8f, 4.0f)
_c(ButtonPrimaryPressedOut,     0xdcdcdcff_rgbaf*0.8f, 4.0f)
_c(ButtonPrimaryPressedOver,    0xdcdcdcff_rgbaf*0.8f, 4.0f)
_c(ButtonPrimaryDisabled,       0xa5c9eaff_rgbaf*0.3f, 4.0f)

_c(ButtonSuccessInactiveOut,    0x3bd267ff_rgbaf*0.8f, 4.0f)
_c(ButtonSuccessInactiveOver,   0xacecbeff_rgbaf*0.8f, 4.0f)
_c(ButtonSuccessPressedOut,     0xacecbeff_rgbaf*0.8f, 4.0f)
_c(ButtonSuccessPressedOver,    0xacecbeff_rgbaf*0.8f, 4.0f)
_c(ButtonSuccessDisabled,       0x3bd267ff_rgbaf*0.3f, 4.0f)

_c(ButtonWarningInactiveOut,    0xc7cf2fff_rgbaf*0.8f, 4.0f)
_c(ButtonWarningInactiveOver,   0xe9ecaeff_rgbaf*0.8f, 4.0f)
_c(ButtonWarningPressedOut,     0xe9ecaeff_rgbaf*0.8f, 4.0f)
_c(ButtonWarningPressedOver,    0xe9ecaeff_rgbaf*0.8f, 4.0f)
_c(ButtonWarningDisabled,       0xc7cf2fff_rgbaf*0.3f, 4.0f)

_c(ButtonDangerInactiveOut,     0xcd3431ff_rgbaf*0.8f, 4.0f)
_c(ButtonDangerInactiveOver,    0xff9391ff_rgbaf*0.8f, 4.0f)
_c(ButtonDangerPressedOut,      0xff9391ff_rgbaf*0.8f, 4.0f)
_c(ButtonDangerPressedOver,     0xff9391ff_rgbaf*0.8f, 4.0f)
_c(ButtonDangerDisabled,        0xcd3431ff_rgbaf*0.3f, 4.0f)

_c(ButtonInfoInactiveOut,       0x2f83ccff_rgbaf*0.8f, 4.0f)
_c(ButtonInfoInactiveOver,      0xa5caebff_rgbaf*0.8f, 4.0f)
_c(ButtonInfoPressedOut,        0xa5caebff_rgbaf*0.8f, 4.0f)
_c(ButtonInfoPressedOver,       0xa5caebff_rgbaf*0.8f, 4.0f)
_c(ButtonInfoDisabled,          0x2f83ccff_rgbaf*0.3f, 4.0f)

_c(ButtonDimInactiveOut,        0x747474ff_rgbaf*0.8f, 4.0f)
_c(ButtonDimInactiveOver,       0xacacacff_rgbaf*0.8f, 4.0f)
_c(ButtonDimPressedOut,         0xacacacff_rgbaf*0.8f, 4.0f)
_c(ButtonDimPressedOver,        0xacacacff_rgbaf*0.8f, 4.0f)
_c(ButtonDimDisabled,           0x747474ff_rgbaf*0.3f, 4.0f)

_c(ButtonFlatInactiveOut,       {}, {}, {}, 0.0f, 4.0f, 0.0f)
_c(ButtonFlatInactiveOver,      {}, {}, 0xd0d9f7_rgbf, 1.5f, 4.0f, 2.5f)
_c(ButtonFlatPressedOut,        {}, {}, 0xd0d9f7_rgbf, 1.5f, 4.0f, 2.5f)
_c(ButtonFlatPressedOver,       {}, {}, 0xd0d9f7_rgbf, 1.5f, 4.0f, 2.5f)
_c(ButtonFlatDisabled,          {}, {}, {}, 0.0f, 4.0f, 0.0f)

/* Color, outline color, (left, top, right, bottom) outline width, corner
   radius, inner corner radius (top left, bottom left, top right, bottom
   right) */
_c(InputDefaultInactiveOut,     0x34424dff_rgbaf*0.8f, 0xdcdcdcff_rgbaf*0.8f,
    {4.0f, 0.0f, 0.0f, 0.0f}, Vector4{4.0f}, {})
_c(InputDefaultInactiveOver,    0x34424dff_rgbaf*0.8f, 0xa5c9eaff_rgbaf*0.8f,
    {4.0f, 0.0f, 0.0f, 0.0f}, Vector4{4.0f}, {})
_c(InputDefaultFocusedOut,      0x34424dff_rgbaf*0.8f, 0xa5c9eaff_rgbaf*0.8f,
    {4.0f, 1.0f, 1.0f, 1.0f}, Vector4{4.0f}, {1.0f, 1.0f, 3.0f, 3.0f})
_c(InputDefaultFocusedOver,     0x34424dff_rgbaf*0.8f, 0xa5c9eaff_rgbaf*0.8f,
    {4.0f, 1.0f, 1.0f, 1.0f}, Vector4{4.0f}, {1.0f, 1.0f, 3.0f, 3.0f})
_c(InputDefaultPressedOut,      0x34424dff_rgbaf*0.8f, 0xa5c9eaff_rgbaf*0.8f,
    {4.0f, 1.0f, 1.0f, 1.0f}, Vector4{4.0f}, {1.0f, 1.0f, 3.0f, 3.0f})
_c(InputDefaultPressedOver,     0x34424dff_rgbaf*0.8f, 0xa5c9eaff_rgbaf*0.8f,
    {4.0f, 1.0f, 1.0f, 1.0f}, Vector4{4.0f}, {1.0f, 1.0f, 3.0f, 3.0f})
_c(InputDefaultDisabled,        0x34424dff_rgbaf*0.3f, 0xdcdcdcff_rgbaf*0.3f,
    {4.0f, 0.0f, 0.0f, 0.0f}, Vector4{4.0f}, {})
#endif
