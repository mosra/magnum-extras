#ifndef Magnum_Ui_AbstractTheme_hpp
#define Magnum_Ui_AbstractTheme_hpp
/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023, 2024, 2025, 2026
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

/** @file
@brief Style IDs for builtin widgets
@m_since_latest_{extras}

Exposed for purposes of creating derived variants of builtin widgets and for
custom style implementation. There should be no need to use anything from this
header from application code.
*/

#include <Magnum/Magnum.h>

#include "Magnum/Ui/visibility.h"

/* Listing these namespaces doesn't add anything to the docs, so don't */
#ifndef DOXYGEN_GENERATING_OUTPUT
namespace Magnum { namespace Ui { namespace Implementation {

/* To be used by custom theme implementations to appropriately size the style
   data arrays. Deliberately *not* used by the builtin theme itself to allow
   for fast iterations without having to adjust these values each time a style
   gets added.

   Be sure to run UiThemeTest after adding new styles to keep these constants
   up-to-date. */
enum: UnsignedInt {
    BaseStyleCount = 79,
    TextStyleCount = 91,
    LayoutStyleCount = 14,
    IconCount = 2
};

enum class BaseStyle: UnsignedShort {
    /* All properties left at defaults. Not used by builtin widgets, meant to
       be used by application code for ad-hoc drawing. */
    Default,

    ButtonDefault,
    ButtonDefaultHovered,
    ButtonDefaultPressed,
    ButtonDefaultPressedHovered,
    ButtonDefaultDisabled,

    ButtonPrimary,
    ButtonPrimaryHovered,
    ButtonPrimaryPressed,
    ButtonPrimaryPressedHovered,
    ButtonPrimaryDisabled,

    ButtonSuccess,
    ButtonSuccessHovered,
    ButtonSuccessPressed,
    ButtonSuccessPressedHovered,
    ButtonSuccessDisabled,

    ButtonWarning,
    ButtonWarningHovered,
    ButtonWarningPressed,
    ButtonWarningPressedHovered,
    ButtonWarningDisabled,

    ButtonDanger,
    ButtonDangerHovered,
    ButtonDangerPressed,
    ButtonDangerPressedHovered,
    ButtonDangerDisabled,

    ButtonInfo,
    ButtonInfoHovered,
    ButtonInfoPressed,
    ButtonInfoPressedHovered,
    ButtonInfoDisabled,

    ButtonDim,
    ButtonDimHovered,
    ButtonDimPressed,
    ButtonDimPressedHovered,
    ButtonDimDisabled,

    ButtonFlat,
    ButtonFlatHovered,
    ButtonFlatPressed,
    ButtonFlatPressedHovered,
    ButtonFlatDisabled,

    InputDefault,
    InputDefaultHovered,
    InputDefaultFocused,
    InputDefaultDisabled,

    InputSuccess,
    InputSuccessHovered,
    InputSuccessFocused,
    InputSuccessDisabled,

    InputWarning,
    InputWarningHovered,
    InputWarningFocused,
    InputWarningDisabled,

    InputDanger,
    InputDangerHovered,
    InputDangerFocused,
    InputDangerDisabled,

    InputFlat,
    InputFlatHovered,
    InputFlatFocused,
    InputFlatDisabled,

    ScrollbarX,
    ScrollbarXHovered,
    ScrollbarXPressed,
    ScrollbarXDisabled,
    ScrollbarY,
    ScrollbarYHovered,
    ScrollbarYPressed,
    ScrollbarYDisabled,
    ScrollbarThumbX,
    ScrollbarThumbXHovered,
    ScrollbarThumbXPressed,
    ScrollbarThumbXDisabled,
    ScrollbarThumbY,
    ScrollbarThumbYHovered,
    ScrollbarThumbYPressed,
    ScrollbarThumbYDisabled,

    PanelBackground,
    PanelBackgroundDisabled,
};

MAGNUM_UI_EXPORT BaseStyle styleTransitionToInactiveOut(BaseStyle index);
MAGNUM_UI_EXPORT BaseStyle styleTransitionToInactiveOver(BaseStyle index);
MAGNUM_UI_EXPORT BaseStyle styleTransitionToFocusedOut(BaseStyle index);
MAGNUM_UI_EXPORT BaseStyle styleTransitionToFocusedOver(BaseStyle index);
MAGNUM_UI_EXPORT BaseStyle styleTransitionToPressedOut(BaseStyle index);
MAGNUM_UI_EXPORT BaseStyle styleTransitionToPressedOver(BaseStyle index);
MAGNUM_UI_EXPORT BaseStyle styleTransitionToDisabled(BaseStyle index);

enum class TextStyle: UnsignedShort {
    /* All properties left at defaults. Not used by builtin widgets, meant to
       be used by application code for ad-hoc drawing. */
    Default,

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

    ButtonFlatIconOnly,
    ButtonFlatTextOnly,
    ButtonFlatIcon,
    ButtonFlatText,
    ButtonFlatHoveredIconOnly,
    ButtonFlatHoveredTextOnly,
    ButtonFlatHoveredIcon,
    ButtonFlatHoveredText,
    ButtonFlatPressedIconOnly,
    ButtonFlatPressedTextOnly,
    ButtonFlatPressedIcon,
    ButtonFlatPressedText,
    ButtonFlatPressedHoveredIconOnly,
    ButtonFlatPressedHoveredTextOnly,
    ButtonFlatPressedHoveredIcon,
    ButtonFlatPressedHoveredText,
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

    InputDefault,
    InputDefaultHovered,
    InputDefaultFocused,
    InputDefaultFocusedBlink,
    InputDefaultPressed,
    InputDefaultDisabled,

    InputSuccess,
    InputSuccessHovered,
    InputSuccessFocused,
    InputSuccessFocusedBlink,
    InputSuccessPressed,
    InputSuccessDisabled,

    InputWarning,
    InputWarningHovered,
    InputWarningFocused,
    InputWarningFocusedBlink,
    InputWarningPressed,
    InputWarningDisabled,

    InputDanger,
    InputDangerHovered,
    InputDangerFocused,
    InputDangerFocusedBlink,
    InputDangerPressed,
    InputDangerDisabled,

    InputFlat,
    InputFlatHovered,
    InputFlatFocused,
    InputFlatFocusedBlink,
    InputFlatPressed,
    InputFlatDisabled,
};

MAGNUM_UI_EXPORT TextStyle styleTransitionToInactiveOut(TextStyle index);
MAGNUM_UI_EXPORT TextStyle styleTransitionToInactiveOver(TextStyle index);
MAGNUM_UI_EXPORT TextStyle styleTransitionToFocusedOut(TextStyle index);
MAGNUM_UI_EXPORT TextStyle styleTransitionToFocusedOver(TextStyle index);
MAGNUM_UI_EXPORT TextStyle styleTransitionToPressedOut(TextStyle index);
MAGNUM_UI_EXPORT TextStyle styleTransitionToPressedOver(TextStyle index);
MAGNUM_UI_EXPORT TextStyle styleTransitionToDisabled(TextStyle index);

enum class LayoutStyle: UnsignedShort {
    Button,
    Label,
    Input,
    Panel,

    ScrollArea,
    ScrollAreaView,
    ScrollAreaViewOnlyX,
    ScrollAreaViewOnlyY,
    ScrollbarX,
    ScrollbarOnlyX,
    ScrollbarY,
    ScrollbarOnlyY,
    ScrollbarThumbX,
    ScrollbarThumbY,
};

}}}
#endif

#endif
