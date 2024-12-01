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

#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Renderer.h>

#include "Magnum/Ui/BaseLayerGL.h"
#include "Magnum/Ui/BaseLayerAnimator.h"
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/RendererGL.h"
#include "Magnum/Ui/Style.h"
#include "Magnum/Ui/TextLayerGL.h"
#include "Magnum/Ui/TextLayerAnimator.h"
#include "Magnum/Ui/UserInterfaceGL.h"

#define DOXYGEN_ELLIPSIS(...) __VA_ARGS__
#define DOXYGEN_IGNORE(...) __VA_ARGS__

using namespace Magnum;

/* Make sure the name doesn't conflict with any other snippets to avoid linker
   warnings, unlike with `int main()` there now has to be a declaration to
   avoid -Wmisssing-prototypes */
void mainUiGL();
void mainUiGL() {
{
/* [AbstractUserInterface-setup] */
Ui::UserInterfaceGL ui{{800, 600}, Ui::McssDarkStyle{}};
/* [AbstractUserInterface-setup] */

/* [AbstractUserInterface-setup-blend] */
GL::Renderer::setBlendFunction(
    GL::Renderer::BlendFunction::One,
    GL::Renderer::BlendFunction::OneMinusSourceAlpha);
/* [AbstractUserInterface-setup-blend] */

/* [AbstractUserInterface-setup-draw] */
GL::defaultFramebuffer.clear(GL::FramebufferClear::Color);

DOXYGEN_ELLIPSIS()

ui.draw();
/* [AbstractUserInterface-setup-draw] */

/* [AbstractUserInterface-setup-draw-ondemand] */
if(ui.state()) {
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color);

    ui.draw();
}
/* [AbstractUserInterface-setup-draw-ondemand] */
}

{
Ui::AbstractUserInterface ui{{100, 100}};
Ui::BaseLayerStyleAnimator animator{Ui::animatorHandle(0, 1)};
/* [BaseLayerStyleAnimator-setup2] */
Ui::BaseLayerGL::Shared baseLayerShared{
    Ui::BaseLayer::Shared::Configuration{DOXYGEN_ELLIPSIS(0)}
        DOXYGEN_ELLIPSIS()
        .setDynamicStyleCount(10) /* adjust as needed */
};
Ui::BaseLayer& baseLayer = ui.setLayerInstance(
    Containers::pointer<Ui::BaseLayerGL>(ui.createLayer(), baseLayerShared));

DOXYGEN_ELLIPSIS()

baseLayer.assignAnimator(animator);
/* [BaseLayerStyleAnimator-setup2] */
}

{
Ui::AbstractUserInterface ui{{100, 100}};
Ui::TextLayerStyleAnimator animator{Ui::animatorHandle(0, 1)};
/* [TextLayerStyleAnimator-setup2] */
Ui::TextLayerGL::Shared textLayerShared{
    Ui::TextLayer::Shared::Configuration{DOXYGEN_ELLIPSIS(0)}
        DOXYGEN_ELLIPSIS()
        .setDynamicStyleCount(10) /* adjust as needed */
};
Ui::TextLayer& textLayer = ui.setLayerInstance(
    Containers::pointer<Ui::TextLayerGL>(ui.createLayer(), textLayerShared));

DOXYGEN_ELLIPSIS()

textLayer.assignAnimator(animator);
/* [TextLayerStyleAnimator-setup2] */
}

{
Ui::AbstractUserInterface ui{{100, 100}};
/* [RendererGL-setup] */
ui.setRendererInstance(Containers::pointer<Ui::RendererGL>());
/* [RendererGL-setup] */
}

}
