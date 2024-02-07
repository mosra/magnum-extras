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

#include <chrono>
#include <Corrade/Containers/BitArrayView.h>
#include <Corrade/Containers/GrowableArray.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Magnum/Animation/Easing.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/TimeStl.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/Renderer.h>
#ifdef CORRADE_TARGET_EMSCRIPTEN
#include <Magnum/Platform/EmscriptenApplication.h>
#else
#include <Magnum/Platform/Sdl2Application.h>
#endif
#include <Magnum/Text/Alignment.h>

#include "Magnum/Whee/AbstractAnimator.h"
#include "Magnum/Whee/BaseLayerGL.h"
#include "Magnum/Whee/Button.h"
#include "Magnum/Whee/EventLayer.h"
#include "Magnum/Whee/Event.h"
#include "Magnum/Whee/Handle.h"
#include "Magnum/Whee/Label.h"
#include "Magnum/Whee/RendererGL.h"
#include "Magnum/Whee/Style.h"
#include "Magnum/Whee/TextProperties.h"
#include "Magnum/Whee/UserInterfaceGL.h"

namespace Magnum { namespace {

using namespace Containers::Literals;
using namespace Math::Literals;

Nanoseconds now() {
    return Nanoseconds{std::chrono::steady_clock::now()};
}

class NodeAnimator: public Whee::AbstractGenericAnimator {
    public:
        explicit NodeAnimator(Whee::AnimatorHandle handle, Whee::AbstractUserInterface& ui): Whee::AbstractGenericAnimator{handle}, _ui(ui) {}

        Whee::AnimationHandle create(Whee::NodeHandle node, const Vector2& offset, Nanoseconds played, Nanoseconds duration, UnsignedInt repeatCount = 1, Whee::AnimationFlags flags = {});

    private:
        struct Data {
            Whee::NodeHandle node;
            Vector2 initialOffset, initialSize;
            Vector2 offset;
        };
        Containers::Array<Data> _data;
        Whee::AbstractUserInterface& _ui;

        Whee::AnimatorFeatures doFeatures() const override { return {}; }
        void doAdvance(Containers::BitArrayView active, const Containers::StridedArrayView1D<const Float>& factors) override;
};

Whee::AnimationHandle NodeAnimator::create(Whee::NodeHandle node, const Vector2& offset, Nanoseconds played, Nanoseconds duration, UnsignedInt repeatCount, Whee::AnimationFlags flags) {
    // TODO figure out how to handle multiple animations on the same node -- find it, replace and continue from where it was before?
    // TODO or should the user side handle that? such as a Button containing animation handles that it then replaces? but that'd mean it'd have to be stateful always :(

    const Whee::AnimationHandle handle = Whee::AbstractGenericAnimator::create(played, duration, repeatCount, flags);
    const UnsignedInt id = Whee::animationHandleId(handle);
    if(id >= _data.size())
        arrayResize(_data, NoInit, id + 1);
    _data[id].node = node;
    _data[id].initialOffset = _ui.nodeOffset(node);
    _data[id].initialSize = _ui.nodeSize(node);
    _data[id].offset = offset;
    return handle;
}

void NodeAnimator::doAdvance(Containers::BitArrayView active, const Containers::StridedArrayView1D<const Float>& factors) {
    for(std::size_t i = 0; i != active.size(); ++i) {
        if(!active[i]) continue;

        const Data& data = _data[i];
        const Vector2 offset = data.offset*Animation::Easing::bounceIn(1.0f - factors[i]);
        _ui.setNodeOffset(data.node, data.initialOffset - offset);
        _ui.setNodeSize(data.node, data.initialSize + 2*offset);
    }
}

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

        NodeAnimator* _nodeAnimator;
        Containers::Optional<Whee::Button> _clickMe;
};

WheeGallery::WheeGallery(const Arguments& arguments): Platform::Application{arguments, Configuration{}.setTitle("Magnum::Whee Gallery"_s).setSize({960, 600})}, _ui{NoCreate, {960, 600}, Vector2{windowSize()}, framebufferSize()} {
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

    _nodeAnimator = &_ui.setGenericAnimatorInstance(Containers::pointer<NodeAnimator>(_ui.createAnimator(), _ui));

    Whee::NodeHandle root = _ui.createNode({}, _ui.size());

    {
        Whee::NodeHandle label = Whee::label(_ui, root, Whee::LabelStyle::Dim, {96, 16},  "Buttons", Whee::TextProperties{}.setAlignment(Text::Alignment::MiddleLeft));
        _ui.setNodeOffset(label, {16, 16});

        Whee::NodeHandle buttons = _ui.createNode(root, {0, 48}, _ui.size());

        Whee::NodeHandle buttonDefault = Whee::button(_ui, buttons, Whee::ButtonStyle::Default, {96, 36}, Whee::Icon::Yes, "Default");
        _ui.setNodeOffset(buttonDefault, {16, 0});

        Whee::NodeHandle buttonPrimary = Whee::button(_ui, buttons, Whee::ButtonStyle::Primary, {96, 36}, Whee::Icon::Yes, "Primary");
        _ui.setNodeOffset(buttonPrimary, {120, 0});

        Whee::NodeHandle buttonDanger = Whee::button(_ui, buttons, Whee::ButtonStyle::Danger, {96, 36}, Whee::Icon::No, "Danger");
        _ui.setNodeOffset(buttonDanger, {224, 0});

        Whee::NodeHandle buttonSuccess = Whee::button(_ui, buttons, Whee::ButtonStyle::Success, {96, 36}, Whee::Icon::Yes, "Success");
        _ui.setNodeOffset(buttonSuccess, {328, 0});

        Whee::NodeHandle buttonWarning = Whee::button(_ui, buttons, Whee::ButtonStyle::Warning, {96, 36}, Whee::Icon::No, "Warning");
        _ui.setNodeOffset(buttonWarning, {432, 0});

        Whee::NodeHandle buttonInfo = Whee::button(_ui, buttons, Whee::ButtonStyle::Info, {96, 36}, Whee::Icon::Yes, "Info");
        _ui.setNodeOffset(buttonInfo, {536, 0});

        Whee::NodeHandle buttonDim = Whee::button(_ui, buttons, Whee::ButtonStyle::Dim, {96, 36}, Whee::Icon::No, "Dim");
        _ui.setNodeOffset(buttonDim, {640, 0});

        Whee::NodeHandle buttonFlat = Whee::button(_ui, buttons, Whee::ButtonStyle::Flat, {96, 36}, Whee::Icon::Yes, "Flat");
        _ui.setNodeOffset(buttonFlat, {744, 0});
    } {
        Whee::NodeHandle buttonsDisabled = _ui.createNode(root, {0, 94}, _ui.size());
        _ui.addNodeFlags(buttonsDisabled, Whee::NodeFlag::Disabled);

        Whee::NodeHandle buttonDefault = Whee::button(_ui, buttonsDisabled, Whee::ButtonStyle::Default, {96, 36}, Whee::Icon::Yes, "Default");
        _ui.setNodeOffset(buttonDefault, {16, 0});

        Whee::NodeHandle buttonPrimary = Whee::button(_ui, buttonsDisabled, Whee::ButtonStyle::Primary, {96, 36}, Whee::Icon::Yes, "Primary");
        _ui.setNodeOffset(buttonPrimary, {120, 0});

        Whee::NodeHandle buttonDanger = Whee::button(_ui, buttonsDisabled, Whee::ButtonStyle::Danger, {96, 36}, Whee::Icon::No, "Danger");
        _ui.setNodeOffset(buttonDanger, {224, 0});

        Whee::NodeHandle buttonSuccess = Whee::button(_ui, buttonsDisabled, Whee::ButtonStyle::Success, {96, 36}, Whee::Icon::Yes, "Success");
        _ui.setNodeOffset(buttonSuccess, {328, 0});

        Whee::NodeHandle buttonWarning = Whee::button(_ui, buttonsDisabled, Whee::ButtonStyle::Warning, {96, 36}, Whee::Icon::No, "Warning");
        _ui.setNodeOffset(buttonWarning, {432, 0});

        Whee::NodeHandle buttonInfo = Whee::button(_ui, buttonsDisabled, Whee::ButtonStyle::Info, {96, 36}, Whee::Icon::Yes, "Info");
        _ui.setNodeOffset(buttonInfo, {536, 0});

        Whee::NodeHandle buttonDim = Whee::button(_ui, buttonsDisabled, Whee::ButtonStyle::Dim, {96, 36}, Whee::Icon::No, "Dim");
        _ui.setNodeOffset(buttonDim, {640, 0});

        Whee::NodeHandle buttonFlat = Whee::button(_ui, buttonsDisabled, Whee::ButtonStyle::Flat, {96, 36}, Whee::Icon::Yes, "Flat");
        _ui.setNodeOffset(buttonFlat, {744, 0});
    }

    {
        Whee::NodeHandle label = Whee::label(_ui, root, Whee::LabelStyle::Dim, {96, 16},  "Labels", Whee::TextProperties{}.setAlignment(Text::Alignment::MiddleLeft));
        _ui.setNodeOffset(label, {16, 146});

        Whee::NodeHandle labels = _ui.createNode(root, {0, 170}, _ui.size());

        Whee::NodeHandle labelDefault = Whee::label(_ui, labels, Whee::LabelStyle::Default, {96, 36}, Whee::Icon::Yes, "Default");
        _ui.setNodeOffset(labelDefault, {16, 0});

        Whee::NodeHandle labelPrimary = Whee::label(_ui, labels, Whee::LabelStyle::Primary, {96, 36}, Whee::Icon::Yes, "Primary");
        _ui.setNodeOffset(labelPrimary, {120, 0});

        Whee::NodeHandle labelDanger = Whee::label(_ui, labels, Whee::LabelStyle::Danger, {96, 36}, Whee::Icon::No, "Danger");
        _ui.setNodeOffset(labelDanger, {224, 0});

        Whee::NodeHandle labelSuccess = Whee::label(_ui, labels, Whee::LabelStyle::Success, {96, 36}, Whee::Icon::Yes, "Success");
        _ui.setNodeOffset(labelSuccess, {328, 0});

        Whee::NodeHandle labelWarning = Whee::label(_ui, labels, Whee::LabelStyle::Warning, {96, 36}, Whee::Icon::No, "Warning");
        _ui.setNodeOffset(labelWarning, {432, 0});

        Whee::NodeHandle labelInfo = Whee::label(_ui, labels, Whee::LabelStyle::Info, {96, 36}, Whee::Icon::Yes, "Info");
        _ui.setNodeOffset(labelInfo, {536, 0});

        Whee::NodeHandle labelDim = Whee::label(_ui, labels, Whee::LabelStyle::Dim, {96, 36}, Whee::Icon::No, "Dim");
        _ui.setNodeOffset(labelDim, {640, 0});
    } {
        Whee::NodeHandle labelsDisabled = _ui.createNode(root, {0, 208}, _ui.size());
        _ui.addNodeFlags(labelsDisabled, Whee::NodeFlag::Disabled);

        Whee::NodeHandle labelDefault = Whee::label(_ui, labelsDisabled, Whee::LabelStyle::Default, {96, 36}, Whee::Icon::Yes, "Default");
        _ui.setNodeOffset(labelDefault, {16, 0});

        Whee::NodeHandle labelPrimary = Whee::label(_ui, labelsDisabled, Whee::LabelStyle::Primary, {96, 36}, Whee::Icon::Yes, "Primary");
        _ui.setNodeOffset(labelPrimary, {120, 0});

        Whee::NodeHandle labelDanger = Whee::label(_ui, labelsDisabled, Whee::LabelStyle::Danger, {96, 36}, Whee::Icon::No, "Danger");
        _ui.setNodeOffset(labelDanger, {224, 0});

        Whee::NodeHandle labelSuccess = Whee::label(_ui, labelsDisabled, Whee::LabelStyle::Success, {96, 36}, Whee::Icon::Yes, "Success");
        _ui.setNodeOffset(labelSuccess, {328, 0});

        Whee::NodeHandle labelWarning = Whee::label(_ui, labelsDisabled, Whee::LabelStyle::Warning, {96, 36}, Whee::Icon::No, "Warning");
        _ui.setNodeOffset(labelWarning, {432, 0});

        Whee::NodeHandle labelInfo = Whee::label(_ui, labelsDisabled, Whee::LabelStyle::Info, {96, 36}, Whee::Icon::Yes, "Info");
        _ui.setNodeOffset(labelInfo, {536, 0});

        Whee::NodeHandle labelDim = Whee::label(_ui, labelsDisabled, Whee::LabelStyle::Dim, {96, 36}, Whee::Icon::No, "Dim");
        _ui.setNodeOffset(labelDim, {640, 0});
    }

    _clickMe = Whee::Button{_ui, root, Whee::ButtonStyle::Default, {212, 64}, "Click me!"};
    _ui.setNodeOffset(*_clickMe, {16, 256});
    _ui.eventLayer().onTapOrClick(*_clickMe, [this]{
        if(_clickMe->style() == Whee::ButtonStyle::Dim)
            _clickMe->setStyle(Whee::ButtonStyle::Default);
        else
            _clickMe->setStyle(Whee::ButtonStyle(UnsignedInt(_clickMe->style()) + 1));
    });
    _ui.eventLayer().onTapOrClick(*_clickMe, [this]{
        _nodeAnimator->create(*_clickMe, {5.0f, 9.0f}, now(), 1.0_sec);
    });

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

    Whee::NodeHandle another = Whee::button(_ui, popup, Whee::ButtonStyle::Success, {128, 36}, "Another!");
    _ui.setNodeOffset(another, {67, 170});
    _ui.eventLayer().onTapOrClick(another, [this]{
        this->popup();
    });

    Whee::NodeHandle more = Whee::button(_ui, popup, Whee::ButtonStyle::Primary, {128, 36}, "More!");
    _ui.setNodeOffset(more, {245, 170});
    _ui.eventLayer().onTapOrClick(more, [this]{
        this->_backgroundBlurBaseLayer->setBackgroundBlurPassCount(this->_backgroundBlurBaseLayer->backgroundBlurPassCount()*2);
    });
}

void WheeGallery::drawEvent() {
    _ui.renderer().compositingFramebuffer().clear(GL::FramebufferClear::Color);

    _ui.advanceAnimations(now());

    _ui.draw();

    GL::AbstractFramebuffer::blit(_ui.renderer().compositingFramebuffer(), GL::defaultFramebuffer, GL::defaultFramebuffer.viewport(), GL::FramebufferBlit::Color);

    swapBuffers();
    if(_ui.state()) redraw();
}

void WheeGallery::mousePressEvent(MouseEvent& event) {
    if(event.button() == MouseEvent::Button::Left) {
        Whee::PointerEvent e{Whee::Pointer::MouseLeft};
        // TODO integer overload? or just have a template wrapper that does this automagically
        _ui.pointerPressEvent(Vector2{event.position()}, e);
    }

    if(_ui.state()) redraw();
}

void WheeGallery::mouseReleaseEvent(MouseEvent& event) {
    if(event.button() == MouseEvent::Button::Left) {
        Whee::PointerEvent e{Whee::Pointer::MouseLeft};
        // TODO integer overload? or just have a template wrapper that does this automagically
        _ui.pointerReleaseEvent(Vector2{event.position()}, e);
    }

    if(_ui.state()) redraw();
}

void WheeGallery::mouseMoveEvent(MouseMoveEvent& event) {
    Whee::Pointers pointers;
    if(event.buttons() & MouseMoveEvent::Button::Left)
        pointers |= Whee::Pointer::MouseLeft;
    Whee::PointerMoveEvent e{{}, pointers};
    // TODO integer overload? or just have a template wrapper that does this automagically
    _ui.pointerMoveEvent(Vector2{event.position()}, e);

    if(_ui.state()) redraw();
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::WheeGallery)
