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

/* TextStyleUniform enum name, TextStyle enum name suffix, font handle,
   alignment, padding. Put into a dedicated file to be able to put as much as
   possible into constexpr arrays without losing the correspondence between
   TextStyle and the actual items. Matching order tested in StyleTest, see all
   #includes of this file in Style.cpp for actual uses. */

#ifdef _c
_c(Button,IconOnly,                 iconFont, MiddleCenter, { 6.0f, 8.0f,   6.0f, 8.0f})
_c(Button,TextOnly,                 mainFont, MiddleCenter, { 6.0f, 8.0f,   6.0f, 8.0f})
/* Assuming a 24x24 icon, a zero-size text and 6-pixel padding between, the
   icon has to be shifted by 3 pixels to the left compared to icon-only state
   (so its left side is 15 pixels from the center) and the text has to be
   shifted by 15 pixels to the right compared to text-only state (so its right
   side is 15 pixels from the center) to have both centered as a whole. Button
   internals then shift the icon further to the left by half of text size,
   which together with the text equivalently expanding to the right by half of
   its size keeps both centered with a 6-pixel padding in between. */
_c(Button,Icon,                     iconFont, MiddleCenter, { 3.0f, 8.0f,   9.0f, 8.0f})
_c(Button,Text,                     mainFont, MiddleCenter, {21.0f, 8.0f,  -9.0f, 8.0f})
/* Pressed buttons have their icons and text shifted down by 1 pixel */
_c(Button,PressedIconOnly,          mainFont, MiddleCenter, { 6.0f, 9.0f,   6.0f, 7.0f})
_c(Button,PressedTextOnly,          mainFont, MiddleCenter, { 6.0f, 9.0f,   6.0f, 7.0f})
_c(Button,PressedIcon,              iconFont, MiddleCenter, { 3.0f, 9.0f,   9.0f, 7.0f})
_c(Button,PressedText,              mainFont, MiddleCenter, {21.0f, 9.0f,  -9.0f, 7.0f})
_c(ButtonDisabled,IconOnly,         iconFont, MiddleCenter, { 6.0f, 8.0f,   6.0f, 8.0f})
_c(ButtonDisabled,TextOnly,         mainFont, MiddleCenter, { 6.0f, 8.0f,   6.0f, 8.0f})
_c(ButtonDisabled,Icon,             iconFont, MiddleCenter, { 3.0f, 8.0f,   9.0f, 8.0f})
_c(ButtonDisabled,Text,             mainFont, MiddleCenter, {21.0f, 8.0f,  -9.0f, 8.0f})

_c(ButtonFlatInactiveOut,IconOnly,  iconFont, MiddleCenter, { 6.0f, 8.0f,   6.0f, 8.0f})
_c(ButtonFlatInactiveOut,TextOnly,  mainFont, MiddleCenter, { 6.0f, 8.0f,   6.0f, 8.0f})
_c(ButtonFlatInactiveOut,Icon,      iconFont, MiddleCenter, { 3.0f, 8.0f,   9.0f, 8.0f})
_c(ButtonFlatInactiveOut,Text,      mainFont, MiddleCenter, {21.0f, 8.0f,  -9.0f, 8.0f})
_c(ButtonFlatInactiveOver,IconOnly, iconFont, MiddleCenter, { 6.0f, 8.0f,   6.0f, 8.0f})
_c(ButtonFlatInactiveOver,TextOnly, mainFont, MiddleCenter, { 6.0f, 8.0f,   6.0f, 8.0f})
_c(ButtonFlatInactiveOver,Icon,     iconFont, MiddleCenter, { 3.0f, 8.0f,   9.0f, 8.0f})
_c(ButtonFlatInactiveOver,Text,     mainFont, MiddleCenter, {21.0f, 8.0f,  -9.0f, 8.0f})
_c(ButtonFlatPressedOut,IconOnly,   mainFont, MiddleCenter, { 6.0f, 9.0f,   6.0f, 7.0f})
_c(ButtonFlatPressedOut,TextOnly,   mainFont, MiddleCenter, { 6.0f, 9.0f,   6.0f, 7.0f})
_c(ButtonFlatPressedOut,Icon,       iconFont, MiddleCenter, { 3.0f, 9.0f,   9.0f, 7.0f})
_c(ButtonFlatPressedOut,Text,       mainFont, MiddleCenter, {21.0f, 9.0f,  -9.0f, 7.0f})
_c(ButtonFlatPressedOver,IconOnly,  mainFont, MiddleCenter, { 6.0f, 9.0f,   6.0f, 7.0f})
_c(ButtonFlatPressedOver,TextOnly,  mainFont, MiddleCenter, { 6.0f, 9.0f,   6.0f, 7.0f})
_c(ButtonFlatPressedOver,Icon,      iconFont, MiddleCenter, { 3.0f, 9.0f,   9.0f, 7.0f})
_c(ButtonFlatPressedOver,Text,      mainFont, MiddleCenter, {21.0f, 9.0f,  -9.0f, 7.0f})
_c(ButtonFlatDisabled,IconOnly,     iconFont, MiddleCenter, { 6.0f, 8.0f,   6.0f, 8.0f})
_c(ButtonFlatDisabled,TextOnly,     mainFont, MiddleCenter, { 6.0f, 8.0f,   6.0f, 8.0f})
_c(ButtonFlatDisabled,Icon,         iconFont, MiddleCenter, { 3.0f, 8.0f,   9.0f, 8.0f})
_c(ButtonFlatDisabled,Text,         mainFont, MiddleCenter, {21.0f, 8.0f,  -9.0f, 8.0f})

_c(LabelDefault,Icon,               iconFont, MiddleCenter, { 0.0f, 8.0f,   0.0f, 8.0f})
_c(LabelDefault,Text,               mainFont, MiddleCenter, { 0.0f, 8.0f,   0.0f, 8.0f})
_c(LabelDefaultDisabled,Icon,       iconFont, MiddleCenter, { 0.0f, 8.0f,   0.0f, 8.0f})
_c(LabelDefaultDisabled,Text,       mainFont, MiddleCenter, { 0.0f, 8.0f,   0.0f, 8.0f})
_c(LabelPrimary,Icon,               iconFont, MiddleCenter, { 0.0f, 8.0f,   0.0f, 8.0f})
_c(LabelPrimary,Text,               mainFont, MiddleCenter, { 0.0f, 8.0f,   0.0f, 8.0f})
_c(LabelPrimaryDisabled,Icon,       iconFont, MiddleCenter, { 0.0f, 8.0f,   0.0f, 8.0f})
_c(LabelPrimaryDisabled,Text,       mainFont, MiddleCenter, { 0.0f, 8.0f,   0.0f, 8.0f})
_c(LabelSuccess,Icon,               iconFont, MiddleCenter, { 0.0f, 8.0f,   0.0f, 8.0f})
_c(LabelSuccess,Text,               mainFont, MiddleCenter, { 0.0f, 8.0f,   0.0f, 8.0f})
_c(LabelSuccessDisabled,Icon,       iconFont, MiddleCenter, { 0.0f, 8.0f,   0.0f, 8.0f})
_c(LabelSuccessDisabled,Text,       mainFont, MiddleCenter, { 0.0f, 8.0f,   0.0f, 8.0f})
_c(LabelWarning,Icon,               iconFont, MiddleCenter, { 0.0f, 8.0f,   0.0f, 8.0f})
_c(LabelWarning,Text,               mainFont, MiddleCenter, { 0.0f, 8.0f,   0.0f, 8.0f})
_c(LabelWarningDisabled,Icon,       iconFont, MiddleCenter, { 0.0f, 8.0f,   0.0f, 8.0f})
_c(LabelWarningDisabled,Text,       mainFont, MiddleCenter, { 0.0f, 8.0f,   0.0f, 8.0f})
_c(LabelDanger,Icon,                iconFont, MiddleCenter, { 0.0f, 8.0f,   0.0f, 8.0f})
_c(LabelDanger,Text,                mainFont, MiddleCenter, { 0.0f, 8.0f,   0.0f, 8.0f})
_c(LabelDangerDisabled,Icon,        iconFont, MiddleCenter, { 0.0f, 8.0f,   0.0f, 8.0f})
_c(LabelDangerDisabled,Text,        mainFont, MiddleCenter, { 0.0f, 8.0f,   0.0f, 8.0f})
_c(LabelInfo,Icon,                  iconFont, MiddleCenter, { 0.0f, 8.0f,   0.0f, 8.0f})
_c(LabelInfo,Text,                  mainFont, MiddleCenter, { 0.0f, 8.0f,   0.0f, 8.0f})
_c(LabelInfoDisabled,Icon,          iconFont, MiddleCenter, { 0.0f, 8.0f,   0.0f, 8.0f})
_c(LabelInfoDisabled,Text,          mainFont, MiddleCenter, { 0.0f, 8.0f,   0.0f, 8.0f})
_c(LabelDim,Icon,                   iconFont, MiddleCenter, { 0.0f, 8.0f,   0.0f, 8.0f})
_c(LabelDim,Text,                   mainFont, MiddleCenter, { 0.0f, 8.0f,   0.0f, 8.0f})
_c(LabelDimDisabled,Icon,           iconFont, MiddleCenter, { 0.0f, 8.0f,   0.0f, 8.0f})
_c(LabelDimDisabled,Text,           mainFont, MiddleCenter, { 0.0f, 8.0f,   0.0f, 8.0f})
#endif
