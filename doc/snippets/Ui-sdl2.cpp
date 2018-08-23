/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018
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

#include <Magnum/Platform/Sdl2Application.h>

#include "Magnum/Ui/UserInterface.h"

using namespace Magnum;

struct Foo: Platform::Application {
void foo() {
{
/* [UserInterface-dpi-fixed] */
Ui::UserInterface ui({800, 600}, windowSize(), framebufferSize());
/* [UserInterface-dpi-fixed] */
}

{
/* [UserInterface-dpi-ratio] */
Ui::UserInterface ui(Vector2{windowSize()}/dpiScaling(),
    windowSize(), framebufferSize());
/* [UserInterface-dpi-ratio] */
}

{
/* [UserInterface-dpi-clamp] */
Ui::UserInterface ui(Math::clamp({640.0f, 360.0f}, {1920.0f, 1080.0f},
    Vector2{windowSize()}/dpiScaling()), windowSize(), framebufferSize());
/* [UserInterface-dpi-clamp] */
}
}
};
