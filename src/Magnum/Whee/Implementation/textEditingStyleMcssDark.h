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

/* TextEditingStyle enum name, TextStyle enum name, padding, then
   TextEditingStyleUniform constructor arguments. Put into a dedicated file to
   be able to put as much as possible into constexpr arrays without losing the
   correspondence between TextEditingStyle and the actual items. Matching order
   tested in StyleTest, see all #includes of this file in Style.cpp for actual
   uses.

   So far there's no need to have a different count of styles and uniforms, so
   it's all together, and the file is included once to extract uniform data and
   once to extract other style properties. To not need to wrap the padding in
   ()s, _c is expected to take all four values of it separately. */

#ifdef _c
_c(InputCursor,           Unchanged,      {0.5f, 3.5f, 2.5f, 3.5f}, 0xdcdcdc_rgbf, 1.5f)
_c(InputCursorFocused,    Unchanged,      {0.5f, 3.5f, 2.5f, 3.5f}, 0xa5c9ea_rgbf, 1.5f)
_c(InputSelection,        InputSelection, {1.0f, 2.0f, 1.0f, 2.0f}, 0x747474_rgbf, 4.0f)
_c(InputSelectionFocused, InputSelection, {1.0f, 2.0f, 1.0f, 2.0f}, 0x2f83cc_rgbf, 4.0f)
// TODO disabled selection??
#endif
