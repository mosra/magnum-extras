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

/* TextStyleUniform enum name, then TextLayerStyleUniform constructor
   arguments. Put into a dedicated file to be able to put as much as possible
   into constexpr arrays without losing the correspondence between TextStyle
   and the actual items. Matching order tested in StyleTest, see all #includes
   of this file in Style.cpp for actual uses. */

#ifdef _c
/* Button text color is always --button-background-color. Compared to the
   background the text is fully opaque. Disabled text color is
   --background-color with 80% opacity. */
_c(Button,                      0x22272e_rgbf)
_c(ButtonDisabled,              0x2f363fff_rgbaf*0.8f)

/* Flat button is styled the same as links (--link-color, --link-active-color).
   Disabled state is 30% opacity same as with button background color. */
_c(ButtonFlatInactiveOut,       0x5b9dd9_rgbf)
_c(ButtonFlatInactiveOver,      0xa5c9ea_rgbf)
_c(ButtonFlatPressedOut,        0x5b9dd9_rgbf)
_c(ButtonFlatPressedOver,       0xa5c9ea_rgbf)
_c(ButtonFlatDisabled,          0x5b9dd9ff_rgbaf*0.3f)
#endif
