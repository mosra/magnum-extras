/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023, 2024, 2025
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
#include <Magnum/GL/Framebuffer.h>
#include <Magnum/Platform/Sdl2Application.h>

#include "Magnum/Math/Functions.h"
#include "Magnum/Ui/Application.h"
#include "Magnum/Ui/RendererGL.h"
#include "Magnum/Ui/Style.h"
#include "Magnum/Ui/UserInterfaceGL.h"

#define DOXYGEN_ELLIPSIS(...) __VA_ARGS__
#define DOXYGEN_IGNORE(...) __VA_ARGS__

using namespace Magnum;

struct Foo: Platform::Application {
void foo() {
{
/* [AbstractUserInterface-dpi-ratio] */
Ui::UserInterfaceGL ui{
    Vector2{windowSize()}/dpiScaling(),
    Vector2{windowSize()}, framebufferSize(), DOXYGEN_ELLIPSIS(Ui::McssDarkStyle{})};
/* [AbstractUserInterface-dpi-ratio] */
}

{
/* [AbstractUserInterface-dpi-clamp] */
Ui::UserInterfaceGL ui{
    Math::clamp({640.0f, 360.0f}, {1920.0f, 1080.0f},
        Vector2{windowSize()}/dpiScaling()),
    Vector2{windowSize()}, framebufferSize(), DOXYGEN_ELLIPSIS(Ui::McssDarkStyle{})};
/* [AbstractUserInterface-dpi-clamp] */
}

{
/* [AbstractUserInterface-dpi-fixed] */
Ui::UserInterfaceGL ui(
    {800, 600},
    Vector2{windowSize()}, framebufferSize(), DOXYGEN_ELLIPSIS(Ui::McssDarkStyle{}));
/* [AbstractUserInterface-dpi-fixed] */
}
}
};

namespace A {

/* The include is already above, so doing it again here should be harmless */
/* [AbstractUserInterface-application-construct-viewport] */
DOXYGEN_ELLIPSIS()
#include <Magnum/Ui/Application.h>

class MyApplication: public Platform::Application {
    public:
        explicit MyApplication(const Arguments& arguments):
            Platform::Application{arguments}, _ui{*this, DOXYGEN_ELLIPSIS(Ui::McssDarkStyle{})} { DOXYGEN_ELLIPSIS() }

        void viewportEvent(ViewportEvent& event) override {
            _ui.setSize(event);
        }

        void drawEvent() override {
            GL::defaultFramebuffer.clear(GL::FramebufferClear::Color);

            _ui.draw();

            swapBuffers();
            if(_ui.state())
                redraw();
        }

        DOXYGEN_ELLIPSIS(void pointerPressEvent(PointerEvent& event) override;
        void pointerReleaseEvent(PointerEvent& event) override;
        void pointerMoveEvent(PointerMoveEvent& event) override;
        void keyPressEvent(KeyEvent& event) override;
        void keyReleaseEvent(KeyEvent& event) override;
        void textInputEvent(TextInputEvent& event) override;)

    private:
        Ui::UserInterfaceGL _ui;
};
/* [AbstractUserInterface-application-construct-viewport] */

/* [AbstractUserInterface-application-events] */
void MyApplication::pointerPressEvent(PointerEvent& event) {
    if(!_ui.pointerPressEvent(event)) {
        DOXYGEN_ELLIPSIS()
    }
    if(_ui.state()) redraw();
}
void MyApplication::pointerReleaseEvent(PointerEvent& event) {
    if(!_ui.pointerReleaseEvent(event)) {
        DOXYGEN_ELLIPSIS()
    }
    if(_ui.state()) redraw();
}
void MyApplication::pointerMoveEvent(PointerMoveEvent& event) {
    if(!_ui.pointerMoveEvent(event)) {
        DOXYGEN_ELLIPSIS()
    }
    if(_ui.state()) redraw();
}
void MyApplication::keyPressEvent(KeyEvent& event) {
    if(!_ui.keyPressEvent(event)) {
        DOXYGEN_ELLIPSIS()
    }
    if(_ui.state()) redraw();
}
void MyApplication::keyReleaseEvent(KeyEvent& event) {
    if(!_ui.keyReleaseEvent(event)) {
        DOXYGEN_ELLIPSIS()
    }
    if(_ui.state()) redraw();
}
void MyApplication::textInputEvent(TextInputEvent& event) {
    if(!_ui.textInputEvent(event)) {
        DOXYGEN_ELLIPSIS()
    }
    if(_ui.state()) redraw();
}
/* [AbstractUserInterface-application-events] */

}

namespace B {

struct MyApplication: Platform::Application {
    void pointerMoveEvent(PointerMoveEvent& event) override;

    Ui::AbstractUserInterface _ui;
    bool _modelLoaded;
    void rotateModel(const Vector2&);
};

/* [AbstractUserInterface-events-application-fallthrough] */
void MyApplication::pointerMoveEvent(PointerMoveEvent& event) {
    if(_ui.pointerMoveEvent(event))
        return;

    if(_modelLoaded && event.pointers()) {
        rotateModel(event.relativePosition());
        event.setAccepted();
        return;
    }

    /* Otherwise propagating the event to the OS */
}
/* [AbstractUserInterface-events-application-fallthrough] */

}

namespace C {

/* [UserInterfaceGL-setup-delayed] */
class MyApplication: public Platform::Application {
    DOXYGEN_ELLIPSIS(explicit MyApplication(const Arguments& arguments);)

    private:
        Ui::UserInterfaceGL _ui{NoCreate};
};

MyApplication::MyApplication(const Arguments& arguments):
    Platform::Application{arguments, NoCreate}
{
    DOXYGEN_ELLIPSIS()

    create(DOXYGEN_ELLIPSIS());
    _ui.create(DOXYGEN_ELLIPSIS(*this), Ui::McssDarkStyle{});
}
/* [UserInterfaceGL-setup-delayed] */

}

namespace D {

/* [RendererGL-compositing-framebuffer] */
class MyApplication: public Platform::Application {
    DOXYGEN_ELLIPSIS(explicit MyApplication(const Arguments& arguments);
    void drawEvent();
    void swapBuffers();
    void redraw();)

    private:
        Ui::UserInterfaceGL _ui{NoCreate};
};

MyApplication::MyApplication(const Arguments& arguments): Platform::Application{DOXYGEN_ELLIPSIS(arguments)} {
    /* Create a renderer with a compositing framebuffer as the first thing */
    _ui.setRendererInstance(Containers::pointer<Ui::RendererGL>(
        Ui::RendererGL::Flag::CompositingFramebuffer));

    /* Then add appropriate compositing layers, set a style, etc */
    _ui
        .setSize(DOXYGEN_ELLIPSIS({}))
        .setStyle(DOXYGEN_ELLIPSIS(Ui::McssDarkStyle{}))
        DOXYGEN_ELLIPSIS();

    DOXYGEN_ELLIPSIS()
}
/* [RendererGL-compositing-framebuffer] */

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

}
