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

#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Renderer.h>

#include "Magnum/Whee/RendererGL.h"
#include "Magnum/Whee/UserInterfaceGL.h"

#define DOXYGEN_ELLIPSIS(...) __VA_ARGS__
#define DOXYGEN_IGNORE(...) __VA_ARGS__

using namespace Magnum;

struct MyApplication {
    void drawEvent();
    void swapBuffers();
    void redraw();

    Whee::UserInterfaceGL _ui{NoCreate};
};

/* [RendererGL-compositing-framebuffer-draw] */
void MyApplication::drawEvent() {
    _ui.renderer().compositingFramebuffer().clear(GL::FramebufferClear::Color);

    // Render content underneath the UI to the compositing framebuffer here ...

    _ui.draw();

    GL::AbstractFramebuffer::blit(
        _ui.renderer().compositingFramebuffer(),
        GL::defaultFramebuffer,
        GL::defaultFramebuffer.viewport(),
        GL::FramebufferBlit::Color);

    swapBuffers();
    redraw();
}
/* [RendererGL-compositing-framebuffer-draw] */

/* Make sure the name doesn't conflict with any other snippets to avoid linker
   warnings, unlike with `int main()` there now has to be a declaration to
   avoid -Wmisssing-prototypes */
void mainWheeGL();
void mainWheeGL() {
{
/* [RendererGL] */
GL::Renderer::setBlendFunction(
    GL::Renderer::BlendFunction::One,
    GL::Renderer::BlendFunction::OneMinusSourceAlpha);
/* [RendererGL] */
}

{
/* [RendererGL-compositing-framebuffer] */
Whee::UserInterfaceGL _ui{NoCreate};

DOXYGEN_ELLIPSIS()

/* Create a renderer with a compositing framebuffer as the first thing */
_ui.setRendererInstance(Containers::pointer<Whee::RendererGL>(
    Whee::RendererGL::Flag::CompositingFramebuffer));

/* Then add appropriate compositing layers, etc */
_ui
    .setSize(DOXYGEN_ELLIPSIS({}))
    DOXYGEN_ELLIPSIS();
/* [RendererGL-compositing-framebuffer] */
}
}
