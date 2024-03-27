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

#include <Magnum/Magnum.h>

#include "Magnum/Whee/visibility.h"

/* Listing these namespaces doesn't add anything to the docs, so don't */
#ifndef DOXYGEN_GENERATING_OUTPUT
namespace Magnum { namespace Whee { namespace Implementation {

enum class BaseStyle: UnsignedShort {
    ButtonDefaultInactiveOut,
    ButtonDefaultInactiveOver,
    ButtonDefaultPressedOut,
    ButtonDefaultPressedOver,
    ButtonDefaultDisabled,

    ButtonPrimaryInactiveOut,
    ButtonPrimaryInactiveOver,
    ButtonPrimaryPressedOut,
    ButtonPrimaryPressedOver,
    ButtonPrimaryDisabled,

    ButtonSuccessInactiveOut,
    ButtonSuccessInactiveOver,
    ButtonSuccessPressedOut,
    ButtonSuccessPressedOver,
    ButtonSuccessDisabled,

    ButtonWarningInactiveOut,
    ButtonWarningInactiveOver,
    ButtonWarningPressedOut,
    ButtonWarningPressedOver,
    ButtonWarningDisabled,

    ButtonDangerInactiveOut,
    ButtonDangerInactiveOver,
    ButtonDangerPressedOut,
    ButtonDangerPressedOver,
    ButtonDangerDisabled,

    ButtonInfoInactiveOut,
    ButtonInfoInactiveOver,
    ButtonInfoPressedOut,
    ButtonInfoPressedOver,
    ButtonInfoDisabled,

    ButtonDimInactiveOut,
    ButtonDimInactiveOver,
    ButtonDimPressedOut,
    ButtonDimPressedOver,
    ButtonDimDisabled,

    ButtonFlatInactiveOut,
    ButtonFlatInactiveOver,
    ButtonFlatPressedOut,
    ButtonFlatPressedOver,
    ButtonFlatDisabled,
};

MAGNUM_WHEE_EXPORT BaseStyle styleTransitionToInactiveOut(BaseStyle index);
MAGNUM_WHEE_EXPORT BaseStyle styleTransitionToInactiveOver(BaseStyle index);
MAGNUM_WHEE_EXPORT BaseStyle styleTransitionToFocusedOut(BaseStyle index);
MAGNUM_WHEE_EXPORT BaseStyle styleTransitionToFocusedOver(BaseStyle index);
MAGNUM_WHEE_EXPORT BaseStyle styleTransitionToPressedOut(BaseStyle index);
MAGNUM_WHEE_EXPORT BaseStyle styleTransitionToPressedOver(BaseStyle index);
MAGNUM_WHEE_EXPORT BaseStyle styleTransitionToDisabled(BaseStyle index);

enum class TextStyleUniform: UnsignedInt {
    Button,
    ButtonDisabled,

    ButtonFlatInactiveOut,
    ButtonFlatInactiveOver,
    ButtonFlatPressedOut,
    ButtonFlatPressedOver,
    ButtonFlatDisabled,

    LabelDefault,
    LabelDefaultDisabled,
    LabelPrimary,
    LabelPrimaryDisabled,
    LabelSuccess,
    LabelSuccessDisabled,
    LabelWarning,
    LabelWarningDisabled,
    LabelDanger,
    LabelDangerDisabled,
    LabelInfo,
    LabelInfoDisabled,
    LabelDim,
    LabelDimDisabled,
};

enum class TextStyle: UnsignedShort {
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

    ButtonFlatInactiveOutIconOnly,
    ButtonFlatInactiveOutTextOnly,
    ButtonFlatInactiveOutIcon,
    ButtonFlatInactiveOutText,
    ButtonFlatInactiveOverIconOnly,
    ButtonFlatInactiveOverTextOnly,
    ButtonFlatInactiveOverIcon,
    ButtonFlatInactiveOverText,
    ButtonFlatPressedOutIconOnly,
    ButtonFlatPressedOutTextOnly,
    ButtonFlatPressedOutIcon,
    ButtonFlatPressedOutText,
    ButtonFlatPressedOverIconOnly,
    ButtonFlatPressedOverTextOnly,
    ButtonFlatPressedOverIcon,
    ButtonFlatPressedOverText,
    ButtonFlatDisabledIconOnly,
    ButtonFlatDisabledTextOnly,
    ButtonFlatDisabledIcon,
    ButtonFlatDisabledText,

    LabelDefaultIcon,
    LabelDefaultText,
    LabelDefaultDisabledIcon,
    LabelDefaultDisabledText,
    LabelPrimaryIcon,
    LabelPrimaryText,
    LabelPrimaryDisabledIcon,
    LabelPrimaryDisabledText,
    LabelSuccessIcon,
    LabelSuccessText,
    LabelSuccessDisabledIcon,
    LabelSuccessDisabledText,
    LabelWarningIcon,
    LabelWarningText,
    LabelWarningDisabledIcon,
    LabelWarningDisabledText,
    LabelDangerIcon,
    LabelDangerText,
    LabelDangerDisabledIcon,
    LabelDangerDisabledText,
    LabelInfoIcon,
    LabelInfoText,
    LabelInfoDisabledIcon,
    LabelInfoDisabledText,
    LabelDimIcon,
    LabelDimText,
    LabelDimDisabledIcon,
    LabelDimDisabledText,
};

MAGNUM_WHEE_EXPORT TextStyle styleTransitionToInactiveOut(TextStyle index);
MAGNUM_WHEE_EXPORT TextStyle styleTransitionToInactiveOver(TextStyle index);
MAGNUM_WHEE_EXPORT TextStyle styleTransitionToFocusedOut(TextStyle index);
MAGNUM_WHEE_EXPORT TextStyle styleTransitionToFocusedOver(TextStyle index);
MAGNUM_WHEE_EXPORT TextStyle styleTransitionToPressedOut(TextStyle index);
MAGNUM_WHEE_EXPORT TextStyle styleTransitionToPressedOver(TextStyle index);
MAGNUM_WHEE_EXPORT TextStyle styleTransitionToDisabled(TextStyle index);

}}}
#endif

#endif
