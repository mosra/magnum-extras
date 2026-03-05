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

#define DOXYGEN_ELLIPSIS(...) __VA_ARGS__

/* [class] */
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Ui/UserInterfaceGL.h>

using namespace Magnum;

namespace {

class MyApplication: public Platform::Application {
    public:
        explicit MyApplication(const Arguments& arguments);

    private:
        void viewportEvent(ViewportEvent& event) override;
        void drawEvent() override;
        void pointerPressEvent(PointerEvent& event) override;
        void pointerReleaseEvent(PointerEvent& event) override;
        void pointerMoveEvent(PointerMoveEvent& event) override;
        void scrollEvent(ScrollEvent& event) override;
        void keyPressEvent(KeyEvent& event) override;
        void keyReleaseEvent(KeyEvent& event) override;
        void textInputEvent(TextInputEvent& event) override;

        Ui::UserInterfaceGL _ui;
};

DOXYGEN_ELLIPSIS()

}

MAGNUM_APPLICATION_MAIN(MyApplication)
/* [class] */

/* [methods] */
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/Math/TimeStl.h>
#include <Magnum/Ui/Application.h>
#include <Magnum/Ui/Theme.h>

DOXYGEN_ELLIPSIS(namespace {)

Nanoseconds now() {
    return Nanoseconds{std::chrono::steady_clock::now()};
}

MyApplication::MyApplication(const Arguments& arguments):
    Platform::Application{arguments, Configuration{}
        .addWindowFlags(Configuration::WindowFlag::Resizable)},
    _ui{*this, Ui::DarkTheme{Ui::DarkTheme::Feature::Animations}}
{
    /* UI requires premultiplied alpha */
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::One,
                                   GL::Renderer::BlendFunction::OneMinusSourceAlpha);

    /* TODO: Add your UI and other initialization code here */
}

void MyApplication::viewportEvent(ViewportEvent& event) {
    GL::defaultFramebuffer.setViewport({{}, event.framebufferSize()});

    _ui.setSize(event);
}

void MyApplication::drawEvent() {
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color);

    /* TODO: Add your other drawing code here, if any */

    _ui
        .advanceAnimations(now())
        .draw();

    swapBuffers();
    if(_ui.state())
        redraw();
}

void MyApplication::pointerPressEvent(PointerEvent& event) {
    _ui.pointerPressEvent(event, now());

    if(_ui.state())
        redraw();
}

DOXYGEN_ELLIPSIS(void MyApplication::pointerReleaseEvent(PointerEvent& event) {
    _ui.pointerReleaseEvent(event, now());

    if(_ui.state())
        redraw();
}

void MyApplication::pointerMoveEvent(PointerMoveEvent& event) {
    _ui.pointerMoveEvent(event, now());

    if(_ui.state())
        redraw();
}

void MyApplication::scrollEvent(ScrollEvent& event) {
    _ui.scrollEvent(event, now());

    if(_ui.state())
        redraw();
}

void MyApplication::keyPressEvent(KeyEvent& event) {
    _ui.keyPressEvent(event, now());

    if(_ui.state())
        redraw();
}

void MyApplication::keyReleaseEvent(KeyEvent& event) {
    _ui.keyReleaseEvent(event, now());

    if(_ui.state())
        redraw();
}

void MyApplication::textInputEvent(TextInputEvent& event) {
    _ui.textInputEvent(event, now());

    if(_ui.state())
        redraw();
}

}
)
/* [methods] */
