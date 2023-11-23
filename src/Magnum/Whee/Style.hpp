#ifndef Magnum_Whee_Style_hpp
#define Magnum_Whee_Style_hpp
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

/** @file
@brief Style IDs for builtin widgets
@m_since_latest

Exposed for purposes of creating derived variants of builtin widgets. There
should be no need to use anything from this header from application code.
*/

#include "Magnum/Whee/Style.h"

/* Listing these namespaces doesn't add anything to the docs, so don't */
#ifndef DOXYGEN_GENERATING_OUTPUT
namespace Magnum { namespace Whee { namespace Implementation {

enum class BaseStyle: UnsignedInt {
    ButtonDefaultInactiveBlur,
    ButtonDefaultInactiveHover,
    ButtonDefaultPressedBlur,
    ButtonDefaultPressedHover,
    ButtonDefaultDisabled,

    ButtonPrimaryInactiveBlur,
    ButtonPrimaryInactiveHover,
    ButtonPrimaryPressedBlur,
    ButtonPrimaryPressedHover,
    ButtonPrimaryDisabled,

    ButtonSuccessInactiveBlur,
    ButtonSuccessInactiveHover,
    ButtonSuccessPressedBlur,
    ButtonSuccessPressedHover,
    ButtonSuccessDisabled,

    ButtonWarningInactiveBlur,
    ButtonWarningInactiveHover,
    ButtonWarningPressedBlur,
    ButtonWarningPressedHover,
    ButtonWarningDisabled,

    ButtonDangerInactiveBlur,
    ButtonDangerInactiveHover,
    ButtonDangerPressedBlur,
    ButtonDangerPressedHover,
    ButtonDangerDisabled,

    ButtonInfoInactiveBlur,
    ButtonInfoInactiveHover,
    ButtonInfoPressedBlur,
    ButtonInfoPressedHover,
    ButtonInfoDisabled,

    ButtonDimInactiveBlur,
    ButtonDimInactiveHover,
    ButtonDimPressedBlur,
    ButtonDimPressedHover,
    ButtonDimDisabled,

    ButtonFlatInactiveBlur,
    ButtonFlatInactiveHover,
    ButtonFlatPressedBlur,
    ButtonFlatPressedHover,
    ButtonFlatDisabled,
};

enum class TextStyleUniform: UnsignedInt {
    Button,
    ButtonDisabled,

    ButtonFlatInactiveBlur,
    ButtonFlatInactiveHover,
    ButtonFlatPressedBlur,
    ButtonFlatPressedHover,
    ButtonFlatDisabled,
};

enum class TextStyle: UnsignedInt {
    ButtonIconOnly,
    ButtonTextOnly,
    ButtonIcon,
    ButtonText,
    ButtonPressedIconOnly,
    ButtonPressedTextOnly,
    ButtonPressedIcon,
    ButtonPressedText,
    ButtonDisabledIconOnly,
    ButtonDisabledTextOnly,
    ButtonDisabledIcon,
    ButtonDisabledText,

    ButtonFlatInactiveBlurIconOnly,
    ButtonFlatInactiveBlurTextOnly,
    ButtonFlatInactiveBlurIcon,
    ButtonFlatInactiveBlurText,
    ButtonFlatInactiveHoverIconOnly,
    ButtonFlatInactiveHoverTextOnly,
    ButtonFlatInactiveHoverIcon,
    ButtonFlatInactiveHoverText,
    ButtonFlatPressedBlurIconOnly,
    ButtonFlatPressedBlurTextOnly,
    ButtonFlatPressedBlurIcon,
    ButtonFlatPressedBlurText,
    ButtonFlatPressedHoverIconOnly,
    ButtonFlatPressedHoverTextOnly,
    ButtonFlatPressedHoverIcon,
    ButtonFlatPressedHoverText,
    ButtonFlatDisabledIconOnly,
    ButtonFlatDisabledTextOnly,
    ButtonFlatDisabledIcon,
    ButtonFlatDisabledText,
};

}}}
#endif

#endif
