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

#include <Magnum/GL/Renderer.h>

#include "Magnum/Whee/BaseLayerGL.h"
#include "Magnum/Whee/TextLayerGL.h"

#define DOXYGEN_ELLIPSIS(...) __VA_ARGS__
#define DOXYGEN_IGNORE(...) __VA_ARGS__

using namespace Magnum;

/* Make sure the name doesn't conflict with any other snippets to avoid linker
   warnings, unlike with `int main()` there now has to be a declaration to
   avoid -Wmisssing-prototypes */
void mainWheeGL();
void mainWheeGL() {
{
/* [BaseLayerGL-renderer] */
GL::Renderer::enable(GL::Renderer::Feature::Blending);
GL::Renderer::setBlendFunction(
    GL::Renderer::BlendFunction::One,
    GL::Renderer::BlendFunction::OneMinusSourceAlpha);
/* [BaseLayerGL-renderer] */
}

{
/* [TextLayerGL-renderer] */
GL::Renderer::enable(GL::Renderer::Feature::Blending);
GL::Renderer::setBlendFunction(
    GL::Renderer::BlendFunction::One,
    GL::Renderer::BlendFunction::OneMinusSourceAlpha);
/* [TextLayerGL-renderer] */
}

{
/* [BaseLayerGL-setStyle] */
struct {
    Whee::BaseLayerStyleCommon common;
    Whee::BaseLayerStyleItem dialogBackground;
    Whee::BaseLayerStyleItem button;
    Whee::BaseLayerStyleItem progressBar;
} style;
DOXYGEN_ELLIPSIS()

Whee::BaseLayerGL::Shared baseLayer{3};
baseLayer.setStyle(style);
/* [BaseLayerGL-setStyle] */
}

{
/* [TextLayerGL-setStyle] */
struct {
    Whee::TextLayerStyleCommon common;
    Whee::TextLayerStyleItem body;
    Whee::TextLayerStyleItem tooltip;
    Whee::TextLayerStyleItem button;
} style;
DOXYGEN_ELLIPSIS()

Whee::TextLayerGL::Shared textLayer{3};
textLayer.setStyle(style);
/* [TextLayerGL-setStyle] */
}
}
