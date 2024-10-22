#ifndef Magnum_Ui_Test_ApplicationTest_hpp
#define Magnum_Ui_Test_ApplicationTest_hpp
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

#include "Magnum/Ui/Application.h"
#include "Magnum/Ui/AbstractLayer.h"
#include "Magnum/Ui/AbstractUserInterface.h"
#include "Magnum/Ui/NodeFlags.h"
#include "Magnum/Ui/RendererGL.h"

namespace Magnum { namespace Ui { namespace Test { namespace {

/* Assumes some Application header is included before this file, currently
   tests mainly just that everything compiles. See Sdl2ApplicationTest.cpp etc.
   for concrete usage and ApplicationTest.cpp for actual functional tests. */

#ifdef MAGNUM_APPLICATION_MAIN
struct ApplicationTest: Platform::Application {
    explicit ApplicationTest(const Arguments& arguments);

    void drawEvent() override {
        GL::defaultFramebuffer.clear(GL::FramebufferClear::Color);

        Debug{} << "draw event";

        _ui.draw();

        if(_ui.state()) {
            Debug{} << "redraw triggered by" << _ui.state();
            redraw();
        }

        swapBuffers();
    }

    /* Set to 0 to test the deprecated mouse events instead */
    #if 1
    void pointerPressEvent(PointerEvent& event) override {
        if(!_ui.pointerPressEvent(event))
            Debug{} << (event.isPrimary() ? "primary" : "secondary") << "pointer press event not accepted";
        if(!event.isAccepted())
            Debug{} << "pointer press event accept not propagated";

        if(_ui.state()) {
            Debug{} << "redraw triggered by" << _ui.state();
            redraw();
        }
    }
    void pointerReleaseEvent(PointerEvent& event) override {
        if(!_ui.pointerReleaseEvent(event))
            Debug{} << (event.isPrimary() ? "primary" : "secondary") << "pointer release event not accepted";
        if(!event.isAccepted())
            Debug{} << "pointer release event accept not propagated";

        if(_ui.state()) {
            Debug{} << "redraw triggered by" << _ui.state();
            redraw();
        }
    }
    void pointerMoveEvent(PointerMoveEvent& event) override {
        if(!_ui.pointerMoveEvent(event))
            Debug{} << (event.isPrimary() ? "primary" : "secondary") << "pointer move event not accepted";
        if(!event.isAccepted())
            Debug{} << "pointer move event accept not propagated";

        if(_ui.state()) {
            Debug{} << "redraw triggered by" << _ui.state();
            redraw();
        }
    }
    #else
    CORRADE_IGNORE_DEPRECATED_PUSH
    void mousePressEvent(MouseEvent& event) override {
        if(!_ui.pointerPressEvent(event))
            Debug{} << "mouse press event not accepted";
        if(!event.isAccepted())
            Debug{} << "mouse press event accept not propagated";

        if(_ui.state()) {
            Debug{} << "redraw triggered by" << _ui.state();
            redraw();
        }
    }
    void mouseReleaseEvent(MouseEvent& event) override {
        if(!_ui.pointerReleaseEvent(event))
            Debug{} << "mouse release event not accepted";
        if(!event.isAccepted())
            Debug{} << "mouse release event accept not propagated";

        if(_ui.state()) {
            Debug{} << "redraw triggered by" << _ui.state();
            redraw();
        }
    }
    void mouseMoveEvent(MouseMoveEvent& event) override {
        if(!_ui.pointerMoveEvent(event))
            Debug{} << "mouse move event not accepted";
        if(!event.isAccepted())
            Debug{} << "mouse move event accept not propagated";

        if(_ui.state()) {
            Debug{} << "redraw triggered by" << _ui.state();
            redraw();
        }
    }
    CORRADE_IGNORE_DEPRECATED_POP
    #endif

    void keyPressEvent(KeyEvent& event) override {
        if(!_ui.keyPressEvent(event))
            Debug{} << "key press event not accepted";
        if(!event.isAccepted())
            Debug{} << "key press event accept not propagated";

        if(_ui.state()) {
            Debug{} << "redraw triggered by" << _ui.state();
            redraw();
        }
    }

    void keyReleaseEvent(KeyEvent& event) override {
        if(!_ui.keyReleaseEvent(event))
            Debug{} << "key release event not accepted";
        if(!event.isAccepted())
            Debug{} << "key release event accept not propagated";

        if(_ui.state()) {
            Debug{} << "redraw triggered by" << _ui.state();
            redraw();
        }
    }

    void textInputEvent(TextInputEvent& event) override {
        if(!_ui.textInputEvent(event))
            Debug{} << "text input event not accepted";
        if(!event.isAccepted())
            Debug{} << "text input event accept not propagated";

        if(_ui.state()) {
            Debug{} << "redraw triggered by" << _ui.state();
            redraw();
        }
    }

    AbstractUserInterface _ui;
};

ApplicationTest::ApplicationTest(const Arguments& arguments): Platform::Application{arguments}, _ui{Vector2{windowSize()}/dpiScaling(), Vector2{windowSize()}, framebufferSize()} {
    Debug{} << "UI of" << Debug::packed << _ui.size() << "in a" << Debug::packed << _ui.windowSize() << "window and a" << Debug::packed << _ui.framebufferSize() << "framebuffer";

    _ui.setRendererInstance(Containers::pointer<RendererGL>());

    /* Layer capturing all events by default */
    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Event;
        }

        void doPointerPressEvent(UnsignedInt, Ui::PointerEvent& event) override {
            Debug{} << event.pointer() << "press at" << Debug::packed << event.position();
            event.setAccepted();
        }

        void doPointerReleaseEvent(UnsignedInt, Ui::PointerEvent& event) override {
            Debug{} << event.pointer() << "release at" << Debug::packed << event.position();
            event.setAccepted();
        }

        void doPointerMoveEvent(UnsignedInt, Ui::PointerMoveEvent& event) override {
            Debug{} << event.pointers() << "move at" << Debug::packed << event.position();
            event.setAccepted();
        }

        void doPointerEnterEvent(UnsignedInt, Ui::PointerMoveEvent& event) override {
            Debug{} << event.pointers() << "enter at" << Debug::packed << event.position();
            event.setAccepted();
        }

        void doPointerLeaveEvent(UnsignedInt, Ui::PointerMoveEvent& event) override {
            Debug{} << event.pointers() << "leave at" << Debug::packed << event.position();
            event.setAccepted();
        }

        void doPointerTapOrClickEvent(UnsignedInt, Ui::PointerEvent& event) override {
            Debug{} << event.pointer() << "tap or click at" << Debug::packed << event.position();
            event.setAccepted();
        }

        void doFocusEvent(UnsignedInt, FocusEvent& event) override {
            Debug{} << "Focus event";
            event.setAccepted();
        }

        void doBlurEvent(UnsignedInt, FocusEvent& event) override {
            Debug{} << "Blur event";
            event.setAccepted();
        }

        void doKeyPressEvent(UnsignedInt, Ui::KeyEvent& event) override {
            Debug{} << event.key() << "press with" << event.modifiers();
            event.setAccepted();
        }

        void doKeyReleaseEvent(UnsignedInt, Ui::KeyEvent& event) override {
            Debug{} << event.key() << "release with" << event.modifiers();
            event.setAccepted();
        }

        void doTextInputEvent(UnsignedInt, Ui::TextInputEvent& event) override {
            Debug{} << "Text input:" << event.text();
            event.setAccepted();
        }
    };
    Layer& layer = _ui.setLayerInstance(Containers::pointer<Layer>(_ui.createLayer()));

    /* Create a single node covering 75% of the window */
    layer.create(_ui.createNode(_ui.size()*0.125f, _ui.size()*0.75f, NodeFlag::Focusable));
}
#endif

}}}}

#endif
