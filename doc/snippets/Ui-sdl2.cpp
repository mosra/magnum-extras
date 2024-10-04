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

#include <Magnum/Platform/Sdl2Application.h>

#include "Magnum/Math/Functions.h"
#include "Magnum/Ui/Application.h"
#include "Magnum/Ui/AbstractUserInterface.h"

#define DOXYGEN_ELLIPSIS(...) __VA_ARGS__
#define DOXYGEN_IGNORE(...) __VA_ARGS__

using namespace Magnum;

struct Foo: Platform::Application {
void foo() {
{
/* [AbstractUserInterface-dpi-fixed] */
Ui::AbstractUserInterface ui(
    {800, 600},
    Vector2{windowSize()}, framebufferSize());
/* [AbstractUserInterface-dpi-fixed] */
}

{
/* [AbstractUserInterface-dpi-ratio] */
Ui::AbstractUserInterface ui(
    Vector2{windowSize()}/dpiScaling(),
    Vector2{windowSize()}, framebufferSize());
/* [AbstractUserInterface-dpi-ratio] */
}

{
/* [AbstractUserInterface-dpi-clamp] */
Ui::AbstractUserInterface ui(
    Math::clamp({640.0f, 360.0f}, {1920.0f, 1080.0f},
        Vector2{windowSize()}/dpiScaling()),
    Vector2{windowSize()}, framebufferSize());
/* [AbstractUserInterface-dpi-clamp] */
}
}
};

struct MyApplication: Platform::Application {
    void mousePressEvent(MouseEvent& event) override;
    void mouseReleaseEvent(MouseEvent& event) override;
    void mouseMoveEvent(MouseMoveEvent& event) override;
    void keyPressEvent(KeyEvent& event) override;
    void keyReleaseEvent(KeyEvent& event) override;
    void textInputEvent(TextInputEvent& event) override;
    Ui::AbstractUserInterface _ui;
};

/* The include is already above, so doing it again here should be harmless */
/* [AbstractUserInterface-application-events] */
#include <Magnum/Ui/Application.h>

DOXYGEN_ELLIPSIS()

void MyApplication::mousePressEvent(MouseEvent& event) {
    if(!_ui.pointerPressEvent(event)) {
        /* Handle an event that wasn't accepted by the UI */
    }

    DOXYGEN_ELLIPSIS()
}

void MyApplication::mouseReleaseEvent(MouseEvent& event) {
    if(!_ui.pointerReleaseEvent(event)) {
        /* Handle an event that wasn't accepted by the UI */
    }

    DOXYGEN_ELLIPSIS()
}

void MyApplication::mouseMoveEvent(MouseMoveEvent& event) {
    if(!_ui.pointerMoveEvent(event)) {
        /* Handle an event that wasn't accepted by the UI */
    }

    DOXYGEN_ELLIPSIS()
}

void MyApplication::keyPressEvent(KeyEvent& event) {
    if(!_ui.keyPressEvent(event)) {
        /* Handle an event that wasn't accepted by the UI */
    }

    DOXYGEN_ELLIPSIS()
}

void MyApplication::keyReleaseEvent(KeyEvent& event) {
    if(!_ui.keyReleaseEvent(event)) {
        /* Handle an event that wasn't accepted by the UI */
    }

    DOXYGEN_ELLIPSIS()
}

void MyApplication::textInputEvent(TextInputEvent& event) {
    if(!_ui.textInputEvent(event)) {
        /* Handle an event that wasn't accepted by the UI */
    }

    DOXYGEN_ELLIPSIS()
}

/* [AbstractUserInterface-application-events] */
