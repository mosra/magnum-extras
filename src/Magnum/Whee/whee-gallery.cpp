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

#include <Corrade/Containers/Function.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Magnum/Math/Color.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/Renderer.h>
#ifdef CORRADE_TARGET_EMSCRIPTEN
#include <Magnum/Platform/EmscriptenApplication.h>
#else
#include <Magnum/Platform/Sdl2Application.h>
#endif
#include <Magnum/Text/Alignment.h>

#include "Magnum/Whee/Anchor.h"
#include "Magnum/Whee/Application.h"
#include "Magnum/Whee/BaseLayerGL.h"
#include "Magnum/Whee/Button.h"
#include "Magnum/Whee/Event.h"
#include "Magnum/Whee/EventLayer.h"
#include "Magnum/Whee/Handle.h"
#include "Magnum/Whee/Label.h"
#include "Magnum/Whee/NodeFlags.h"
#include "Magnum/Whee/RendererGL.h"
#include "Magnum/Whee/Style.h"
#include "Magnum/Whee/TextProperties.h"
#include "Magnum/Whee/UserInterfaceGL.h"

namespace Magnum { namespace {

using namespace Containers::Literals;
using namespace Math::Literals;

class WheeGallery: public Platform::Application {
    public:
        explicit WheeGallery(const Arguments& arguments);

    private:
        void drawEvent() override;
        void mousePressEvent(MouseEvent& event) override;
        void mouseReleaseEvent(MouseEvent& event) override;
        void mouseMoveEvent(MouseMoveEvent& event) override;

        void popup();

        Whee::UserInterfaceGL _ui;
        Whee::BaseLayerGL::Shared _backgroundBlurBaseLayerShared{NoCreate};
        Whee::BaseLayerGL* _backgroundBlurBaseLayer;
};

WheeGallery::WheeGallery(const Arguments& arguments): Platform::Application{arguments, Configuration{}.setTitle("Magnum::Whee Gallery"_s).setSize({900, 600})}, _ui{NoCreate, {900, 600}, Vector2{windowSize()}, framebufferSize()} {
    /* Renderer with a compositing framebuffer enabled */
    _ui.setRendererInstance(Containers::pointer<Whee::RendererGL>(Whee::RendererGL::Flag::CompositingFramebuffer));

    /* Set a style. Has to be done after creating the renderer as it otherwise
       adds its own. */
    _ui.setStyle(Whee::McssDarkStyle{});

    {
        Whee::BaseLayerCommonStyleUniform commonStyleUniform;
        commonStyleUniform
            .setSmoothness(0.75f)
            .setBackgroundBlurAlpha(0.95f);
        Whee::BaseLayerStyleUniform styleUniforms[1];
        styleUniforms[0]
            .setCornerRadius({16.0f, 4.0f, 16.0f, 4.0f})
            .setInnerOutlineCornerRadius({2.0f, 2.0f, 2.0f, 2.0f})
            .setOutlineWidth({0.0f, 32.0f, 0.0f, 2.0f})
            // .setColor(0x2f363fee_rgbaf)
            .setColor(0xdcdcdcdc_rgbaf*0.8f)
            // .setOutlineColor(0x282e36cc_rgbaf)
            .setOutlineColor(0xefefefef_rgbaf*0.4f);
        _backgroundBlurBaseLayerShared = Whee::BaseLayerGL::Shared{
            Whee::BaseLayerGL::Shared::Configuration{1}
                .setFlags(Whee::BaseLayerGL::Shared::Flag::BackgroundBlur)
                .setBackgroundBlurRadius(31)}
                ;
        _backgroundBlurBaseLayerShared
            .setStyle(commonStyleUniform, styleUniforms, {});

        /* It's drawn before all other layers */
        _backgroundBlurBaseLayer = &_ui.setLayerInstance(Containers::pointer<Whee::BaseLayerGL>(_ui.createLayer(_ui.baseLayer().handle()), _backgroundBlurBaseLayerShared));
        _backgroundBlurBaseLayer->setBackgroundBlurPassCount(2);
    }

    Whee::NodeHandle root = _ui.createNode({}, _ui.size());

    {
        Whee::label({_ui, root, {16, 16}, {96, 16}},
            Whee::LabelStyle::Dim, "Buttons", Text::Alignment::MiddleLeft);
        Whee::NodeHandle buttons = _ui.createNode(root, {0, 48}, _ui.size());

        Whee::button({_ui, buttons, { 16, 0}, {96, 36}},
            Whee::ButtonStyle::Default, Whee::Icon::Yes, "Default");
        Whee::button({_ui, buttons, {120, 0}, {96, 36}},
            Whee::ButtonStyle::Primary, Whee::Icon::Yes, "Primary");
        Whee::button({_ui, buttons, {224, 0}, {96, 36}},
            Whee::ButtonStyle::Danger, Whee::Icon::No, "Danger");
        Whee::button({_ui, buttons, {328, 0}, {96, 36}},
            Whee::ButtonStyle::Success, Whee::Icon::Yes, "Success");
        Whee::button({_ui, buttons, {432, 0}, {96, 36}},
            Whee::ButtonStyle::Warning, Whee::Icon::No, "Warning");
        Whee::button({_ui, buttons, {536, 0}, {96, 36}},
            Whee::ButtonStyle::Info, Whee::Icon::Yes, "Info");
        Whee::button({_ui, buttons, {640, 0}, {96, 36}},
            Whee::ButtonStyle::Dim, Whee::Icon::No, "Dim");
        Whee::button({_ui, buttons, {744, 0}, {96, 36}},
            Whee::ButtonStyle::Flat, Whee::Icon::Yes, "Flat");
    } {
        Whee::NodeHandle buttonsDisabled = _ui.createNode(root,
            {0, 94}, _ui.size(), Whee::NodeFlag::Disabled);

        Whee::button({_ui, buttonsDisabled, { 16, 0}, {96, 36}},
            Whee::ButtonStyle::Default, Whee::Icon::Yes, "Default");
        Whee::button({_ui, buttonsDisabled, {120, 0}, {96, 36}},
            Whee::ButtonStyle::Primary, Whee::Icon::Yes, "Primary");
        Whee::button({_ui, buttonsDisabled, {224, 0}, {96, 36}},
            Whee::ButtonStyle::Danger, Whee::Icon::No, "Danger");
        Whee::button({_ui, buttonsDisabled, {328, 0}, {96, 36}},
            Whee::ButtonStyle::Success, Whee::Icon::Yes, "Success");
        Whee::button({_ui, buttonsDisabled, {432, 0}, {96, 36}},
            Whee::ButtonStyle::Warning, Whee::Icon::No, "Warning");
        Whee::button({_ui, buttonsDisabled, {536, 0}, {96, 36}},
            Whee::ButtonStyle::Info, Whee::Icon::Yes, "Info");
        Whee::button({_ui, buttonsDisabled, {640, 0}, {96, 36}},
            Whee::ButtonStyle::Dim, Whee::Icon::No, "Dim");
        Whee::button({_ui, buttonsDisabled, {744, 0}, {96, 36}},
            Whee::ButtonStyle::Flat, Whee::Icon::Yes, "Flat");
    }

    {
        Whee::label({_ui, root, {16, 146}, {96, 16}},
            Whee::LabelStyle::Dim, "Labels", Text::Alignment::MiddleLeft);
        Whee::NodeHandle labels = _ui.createNode(root, {0, 170}, _ui.size());

        Whee::label({_ui, labels, { 16, 0}, {96, 36}},
            Whee::LabelStyle::Default, "Default");
        Whee::label({_ui, labels, {120, 0}, {96, 36}},
            Whee::LabelStyle::Primary, "Primary");
        Whee::label({_ui, labels, {224, 0}, {96, 36}},
            Whee::LabelStyle::Danger, "Danger");
        Whee::label({_ui, labels, {328, 0}, {96, 36}},
            Whee::LabelStyle::Success, "Success");
        Whee::label({_ui, labels, {432, 0}, {96, 36}},
            Whee::LabelStyle::Warning, "Warning");
        Whee::label({_ui, labels, {526, 0}, {96, 36}},
            Whee::LabelStyle::Info, "Info");
        Whee::label({_ui, labels, {640, 0}, {96, 36}},
            Whee::LabelStyle::Dim, "Dim");
    } {
        Whee::NodeHandle labelsDisabled = _ui.createNode(root,
            {0, 208}, _ui.size(), Whee::NodeFlag::Disabled);

        Whee::label({_ui, labelsDisabled, { 16, 0}, {96, 36}},
            Whee::LabelStyle::Default, "Default");
        Whee::label({_ui, labelsDisabled, {120, 0}, {96, 36}},
            Whee::LabelStyle::Primary, "Primary");
        Whee::label({_ui, labelsDisabled, {224, 0}, {96, 36}},
            Whee::LabelStyle::Danger, "Danger");
        Whee::label({_ui, labelsDisabled, {328, 0}, {96, 36}},
            Whee::LabelStyle::Success, "Success");
        Whee::label({_ui, labelsDisabled, {432, 0}, {96, 36}},
            Whee::LabelStyle::Warning, "Warning");
        Whee::label({_ui, labelsDisabled, {526, 0}, {96, 36}},
            Whee::LabelStyle::Info, "Info");
        Whee::label({_ui, labelsDisabled, {640, 0}, {96, 36}},
            Whee::LabelStyle::Dim, "Dim");
    }

    popup();

    GL::Renderer::setClearColor(0x22272e_rgbf);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::One, GL::Renderer::BlendFunction::OneMinusSourceAlpha);
}

void WheeGallery::popup() {
    Whee::NodeHandle popup = _ui.createNode({180, 180}, {440, 240});
    _backgroundBlurBaseLayer->create(0, popup);
    _ui.eventLayer().onDrag(popup, [this, popup](const Vector2& offset){
        _ui.setNodeOffset(popup, _ui.nodeOffset(popup) + offset);
    });
    _ui.eventLayer().onPress(popup, [this, popup]{
        _ui.setNodeOrder(popup, Whee::NodeHandle::Null);
    });

    Whee::NodeHandle another = Whee::button({_ui, popup, {67, 170}, {128, 36}},
        Whee::ButtonStyle::Success, "Another!");
    _ui.eventLayer().onTapOrClick(another, [this]{
        this->popup();
    });

    Whee::NodeHandle more = Whee::button({_ui, popup, {245, 170}, {128, 36}},
        Whee::ButtonStyle::Primary, "More!");
    _ui.eventLayer().onTapOrClick(more, [this]{
        _backgroundBlurBaseLayer->setBackgroundBlurPassCount(_backgroundBlurBaseLayer->backgroundBlurPassCount()*2);
    });
}

void WheeGallery::drawEvent() {
    _ui.renderer().compositingFramebuffer().clear(GL::FramebufferClear::Color);

    _ui.draw();

    GL::AbstractFramebuffer::blit(_ui.renderer().compositingFramebuffer(), GL::defaultFramebuffer, GL::defaultFramebuffer.viewport(), GL::FramebufferBlit::Color);

    swapBuffers();
    if(_ui.state()) redraw();
}

void WheeGallery::mousePressEvent(MouseEvent& event) {
    _ui.pointerPressEvent(event);

    if(_ui.state()) redraw();
}

void WheeGallery::mouseReleaseEvent(MouseEvent& event) {
    _ui.pointerReleaseEvent(event);

    if(_ui.state()) redraw();
}

void WheeGallery::mouseMoveEvent(MouseMoveEvent& event) {
    _ui.pointerMoveEvent(event);

    if(_ui.state()) redraw();
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::WheeGallery)
