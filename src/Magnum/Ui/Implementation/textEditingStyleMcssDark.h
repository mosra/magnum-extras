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

/* _c() is TextEditingStyle enum name, TextStyle enum name, padding, then
   TextEditingStyleUniform constructor arguments. _s() is then additionally
   with a selection override for the TextStyle as the second argument.

   Put into a dedicated file to be able to put as much as possible into
   constexpr arrays without losing the correspondence between TextEditingStyle
   and the actual items. Matching order tested in StyleTest, see all #includes
   of this file in Style.cpp for actual uses.

   So far there's no need to have a different count of styles and uniforms, so
   it's all together, and the file is included once to extract uniform data and
   once to extract other style properties. To not need to wrap the padding in
   ()s, _c is expected to take all four values of it separately. */

#if defined(_c) && defined(_s)
/* Input cursor is always --*-link-active-color. 80% opacity when hovered,
   fully opaque when focused, apart from the 80% opacity matching the outline
   and left edge color. Selection is --*-color with 40% opacity. */
_c(InputCursorDefault,                  {0.5f, 2.5f, 1.5f, 2.5f}, 0xa5c9eaff_rgbaf*0.8f, 1.5f)
_c(InputCursorFocusedDefault,           {0.5f, 3.5f, 2.5f, 3.5f}, 0xa5c9ea_rgbf, 1.5f)
_s(InputSelectionDefault,InputSelection,{1.0f, 2.0f, 1.0f, 2.0f}, 0xdcdcdcff_rgbaf*0.4f, 4.0f)

_c(InputCursorSuccess,                  {0.5f, 2.5f, 1.5f, 2.5f}, 0xacecbeff_rgbaf*0.8f, 1.5f)
_c(InputCursorFocusedSuccess,           {0.5f, 3.5f, 2.5f, 3.5f}, 0xacecbeff_rgbaf, 1.5f)
_s(InputSelectionSuccess,InputSelection,{1.0f, 2.0f, 1.0f, 2.0f}, 0x3bd267ff_rgbaf*0.4f, 4.0f)

_c(InputCursorWarning,                  {0.5f, 2.5f, 1.5f, 2.5f}, 0xe9ecaeff_rgbaf*0.8f, 1.5f)
_c(InputCursorFocusedWarning,           {0.5f, 3.5f, 2.5f, 3.5f}, 0xe9ecaeff_rgbaf, 1.5f)
_s(InputSelectionWarning,InputSelection,{1.0f, 2.0f, 1.0f, 2.0f}, 0xc7cf2fff_rgbaf*0.4f, 4.0f)

_c(InputCursorDanger,                   {0.5f, 2.5f, 1.5f, 2.5f}, 0xff9391ff_rgbaf*0.8f, 1.5f)
_c(InputCursorFocusedDanger,            {0.5f, 3.5f, 2.5f, 3.5f}, 0xff9391ff_rgbaf, 1.5f)
_s(InputSelectionDanger, InputSelection,{1.0f, 2.0f, 1.0f, 2.0f}, 0xcd3431ff_rgbaf*0.4f, 4.0f)

/* Flat input cursor is --link-color when hovered, and --link-active-color when
   focused, always opaque, matching the outline. Selection is --link-color with
   40% opacity. */
_c(InputCursorFlat,                     {0.5f, 2.5f, 1.5f, 2.5f}, 0x5b9dd9_rgbf, 1.5f)
_c(InputCursorFocusedFlat,              {0.5f, 3.5f, 2.5f, 3.5f}, 0xa5c9ea_rgbf, 1.5f)
_s(InputSelectionFlat,   InputSelection,{1.0f, 2.0f, 1.0f, 2.0f}, 0x5b9dd9ff_rgbaf*0.4f, 4.0f)
#endif
