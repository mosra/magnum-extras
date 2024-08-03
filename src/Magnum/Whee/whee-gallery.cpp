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
#include <Corrade/Containers/Function.h>
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
#include "Magnum/Whee/Anchor.h"
#include "Magnum/Whee/Application.h"
#include "Magnum/Whee/BaseLayerAnimator.h"
#include "Magnum/Whee/BaseLayerGL.h"
#include "Magnum/Whee/Button.h"
#include "Magnum/Whee/Event.h"
#include "Magnum/Whee/EventLayer.h"
#include "Magnum/Whee/Handle.h"
#include "Magnum/Whee/Input.h"
#include "Magnum/Whee/Label.h"
#include "Magnum/Whee/NodeFlags.h"
#include "Magnum/Whee/RendererGL.h"
#include "Magnum/Whee/SnapLayouter.h"
#include "Magnum/Whee/Style.h"
#include "Magnum/Whee/Style.hpp" // TODO ehhh
#include "Magnum/Whee/TextLayer.h"
#include "Magnum/Whee/TextLayerAnimator.h"
#include "Magnum/Whee/TextProperties.h"
#include "Magnum/Whee/UserInterfaceGL.h"

namespace Magnum { namespace {

using namespace Containers::Literals;
using namespace Math::Literals;

namespace Ui = Whee; // TODO yeah use this from the beginning already

Nanoseconds now() {
    return Nanoseconds{std::chrono::steady_clock::now()};
}

class NodeAnimator: public Whee::AbstractNodeAnimator {
    public:
        explicit NodeAnimator(Whee::AnimatorHandle handle, Whee::AbstractUserInterface& ui): Whee::AbstractNodeAnimator{handle}, _ui(ui) {}

        Whee::AnimationHandle create(Whee::NodeHandle node, const Vector2& offset, Nanoseconds played, Nanoseconds duration, UnsignedInt repeatCount = 1, Whee::AnimationFlags flags = {});

    private:
        struct Data {
            Vector2 initialOffset, initialSize;
            Vector2 offset;
        };
        Containers::Array<Data> _data;
        Whee::AbstractUserInterface& _ui;

        Whee::NodeAnimations doAdvance(Containers::BitArrayView active, const Containers::StridedArrayView1D<const Float>& factors, const Containers::StridedArrayView1D<Vector2>& nodeOffsets, const Containers::StridedArrayView1D<Vector2>& nodeSizes, const Containers::StridedArrayView1D<Whee::NodeFlags>& nodeFlags, Containers::MutableBitArrayView nodesRemove) override;
};

Whee::AnimationHandle NodeAnimator::create(Whee::NodeHandle node, const Vector2& offset, Nanoseconds played, Nanoseconds duration, UnsignedInt repeatCount, Whee::AnimationFlags flags) {
    // TODO figure out how to handle multiple animations on the same node -- find it, replace and continue from where it was before?
    // TODO or should the user side handle that? such as a Button containing animation handles that it then replaces? but that'd mean it'd have to be stateful always :(

    const Whee::AnimationHandle handle = Whee::AbstractNodeAnimator::create(played, duration, node, repeatCount, flags);
    const UnsignedInt id = Whee::animationHandleId(handle);
    if(id >= _data.size())
        arrayResize(_data, NoInit, id + 1);
    // TODO ugh, don't access the UI like this
    _data[id].initialOffset = _ui.nodeOffset(node);
    _data[id].initialSize = _ui.nodeSize(node);
    _data[id].offset = offset;
    return handle;
}

Whee::NodeAnimations NodeAnimator::doAdvance(Containers::BitArrayView active, const Containers::StridedArrayView1D<const Float>& factors, const Containers::StridedArrayView1D<Vector2>& nodeOffsets, const Containers::StridedArrayView1D<Vector2>& nodeSizes, const Containers::StridedArrayView1D<Whee::NodeFlags>&, Containers::MutableBitArrayView) {
    Containers::StridedArrayView1D<const Whee::NodeHandle> nodes = this->nodes();

    for(std::size_t i = 0; i != active.size(); ++i) {
        if(!active[i]) continue;

        const Data& data = _data[i];
        const Vector2 offset = data.offset*Animation::Easing::bounceIn(1.0f - factors[i]);
        const UnsignedInt nodeId = nodeHandleId(nodes[i]);
        nodeOffsets[nodeId] = data.initialOffset - offset;
        nodeSizes[nodeId] = data.initialSize + 2*offset;
    }

    return Whee::NodeAnimation::OffsetSize;
}

class WheeGallery: public Platform::Application {
    public:
        explicit WheeGallery(const Arguments& arguments);

    private:
        void drawEvent() override;
        void mousePressEvent(MouseEvent& event) override;
        void mouseReleaseEvent(MouseEvent& event) override;
        void mouseMoveEvent(MouseMoveEvent& event) override;
        void keyPressEvent(KeyEvent& event) override;
        void keyReleaseEvent(KeyEvent& event) override;
        void textInputEvent(TextInputEvent& event) override;

        void popup();

        Whee::UserInterfaceGL _ui;
        Whee::SnapLayouter* _layouter;

        Whee::BaseLayerGL::Shared _backgroundBlurBaseLayerShared{NoCreate};
        Whee::BaseLayerGL* _backgroundBlurBaseLayer;

        NodeAnimator* _nodeAnimator;
        Whee::BaseLayerStyleAnimator* _styleAnimator;
        Whee::TextLayerStyleAnimator* _textStyleAnimator;
        Containers::Optional<Whee::Button> _clickMe;

        Whee::AnimationHandle _inputCursorAnimation;
};

WheeGallery::WheeGallery(const Arguments& arguments): Platform::Application{arguments, Configuration{}.setTitle("Magnum::Whee Gallery"_s).setSize({900, 600})}, _ui{NoCreate, {900, 600}, Vector2{windowSize()}, framebufferSize()} {
    /* Renderer with a compositing framebuffer enabled */
    _ui.setRendererInstance(Containers::pointer<Whee::RendererGL>(Whee::RendererGL::Flag::CompositingFramebuffer));

    /* Set a style. Has to be done after creating the renderer as it otherwise
       adds its own. */
    _ui.setStyle(
        Whee::McssDarkStyle{}
            .setTextLayerDynamicStyleCount(2)
    );

    {
        Whee::BaseLayerCommonStyleUniform commonStyleUniform;
        commonStyleUniform
            .setSmoothness(0.75f)
            .setBackgroundBlurAlpha(0.95f);
        Whee::BaseLayerStyleUniform styleUniforms[3];
        styleUniforms[0]
            .setCornerRadius({16.0f, 4.0f, 16.0f, 4.0f})
            .setInnerOutlineCornerRadius({2.0f, 2.0f, 2.0f, 2.0f})
            .setOutlineWidth({0.0f, 32.0f, 0.0f, 2.0f})
            // .setColor(0x2f363fee_rgbaf)
            .setColor(0xdcdcdcdc_rgbaf*0.8f)
            // .setOutlineColor(0x282e36cc_rgbaf)
            .setOutlineColor(0xefefefef_rgbaf*0.4f);
        /* A derived copy */
        styleUniforms[1] = Whee::BaseLayerStyleUniform{styleUniforms[0]}
            .setColor(0xdcdcdcdc_rgbaf*0.2f)
            .setOutlineColor(0x3bd267_rgbf)
            .setOutlineWidth({2.0f, 34.0f, 2.0f, 4.0f});
        styleUniforms[2]
            .setCornerRadius(4.0f)
            .setColor(0x1f1f1fff_rgbaf*0.5f)
            // TODO ewww artifacts without this
            .setOutlineColor(0x1f1f1fff_rgbaf*0.5f);
        _backgroundBlurBaseLayerShared = Whee::BaseLayerGL::Shared{
            Whee::BaseLayerGL::Shared::Configuration{3}
                .setDynamicStyleCount(10)
                .setFlags(Whee::BaseLayerGL::Shared::Flag::BackgroundBlur)
                .addFlags(Whee::BaseLayerGL::Shared::Flag::SubdividedQuads)
                .setBackgroundBlurRadius(31)}
                ;
        _backgroundBlurBaseLayerShared
            .setStyle(commonStyleUniform, styleUniforms, {});

        /* It's drawn before all other layers */
        _backgroundBlurBaseLayer = &_ui.setLayerInstance(Containers::pointer<Whee::BaseLayerGL>(_ui.createLayer(_ui.baseLayer().handle()), _backgroundBlurBaseLayerShared));
        _backgroundBlurBaseLayer->setBackgroundBlurPassCount(2);
    }


    _backgroundBlurBaseLayer->setDynamicStyle(0, Whee::BaseLayerStyleUniform{}, {});

    _nodeAnimator = &_ui.setNodeAnimatorInstance(Containers::pointer<NodeAnimator>(_ui.createAnimator(), _ui));
    Containers::Pointer<Whee::BaseLayerStyleAnimator> styleAnimator{InPlaceInit, _ui.createAnimator()};
    _backgroundBlurBaseLayer->setAnimator(*styleAnimator);
    _styleAnimator = &_ui.setStyleAnimatorInstance(Utility::move(styleAnimator));

    Containers::Pointer<Whee::TextLayerStyleAnimator> textStyleAnimator{InPlaceInit, _ui.createAnimator()};
    _ui.textLayer().setAnimator(*textStyleAnimator);
    _textStyleAnimator = &_ui.setStyleAnimatorInstance(Utility::move(textStyleAnimator));

    _layouter = &_ui.setLayouterInstance(Containers::pointer<Whee::SnapLayouter>(_ui.createLayouter()));
    _layouter->setMargin({8.0f, 10.0f});
    _layouter->setPadding({16.0f, 16.0f});

    Ui::NodeHandle root = _ui.createNode({}, _ui.size());

    {
        Ui::SnapLayout snap{_ui, *_layouter, root};

        /* Buttons */
        Ui::label(
            snap(Ui::Snap::Top|Ui::Snap::Left|Ui::Snap::Inside, root, {96, 16}),
            Ui::LabelStyle::Dim, "Buttons", Text::Alignment::MiddleLeft);
        snap.setNextSize({96, 36})
            .setNextSnap(Ui::Snap::Right);

        Ui::NodeHandle buttonDefault = Ui::button(
            snap(Ui::Snap::Bottom|Ui::Snap::Left|Ui::Snap::InsideX),
            Ui::ButtonStyle::Default, Ui::Icon::Yes, "Default");
        Ui::button(snap, Ui::ButtonStyle::Primary, Ui::Icon::Yes, "Primary");
        Ui::button(snap, Ui::ButtonStyle::Danger, Ui::Icon::No, "Danger");
        Ui::button(snap, Ui::ButtonStyle::Success, Ui::Icon::Yes, "Success");
        Ui::button(snap, Ui::ButtonStyle::Warning, Ui::Icon::No, "Warning");
        Ui::button(snap, Ui::ButtonStyle::Info, Ui::Icon::Yes, "Info");
        Ui::button(snap, Ui::ButtonStyle::Dim, Ui::Icon::No, "Dim");
        Ui::button(snap, Ui::ButtonStyle::Flat, Ui::Icon::Yes, "Flat");

        Ui::NodeHandle buttonDefaultDisabled = Ui::button(
            snap(Ui::Snap::Bottom|Ui::Snap::Left|Ui::Snap::InsideX,
                 buttonDefault, Ui::NodeFlag::Disabled),
            Ui::ButtonStyle::Default, Ui::Icon::Yes, "Default");
        Ui::button(snap(Ui::NodeFlag::Disabled), Ui::ButtonStyle::Primary, Ui::Icon::Yes, "Primary");
        Ui::button(snap(Ui::NodeFlag::Disabled), Ui::ButtonStyle::Danger, Ui::Icon::No, "Danger");
        Ui::button(snap(Ui::NodeFlag::Disabled), Ui::ButtonStyle::Success, Ui::Icon::Yes, "Success");
        Ui::button(snap(Ui::NodeFlag::Disabled), Ui::ButtonStyle::Warning, Ui::Icon::No, "Warning");
        Ui::button(snap(Ui::NodeFlag::Disabled), Ui::ButtonStyle::Info, Ui::Icon::Yes, "Info");
        Ui::button(snap(Ui::NodeFlag::Disabled), Ui::ButtonStyle::Dim, Ui::Icon::No, "Dim");
        Ui::button(snap(Ui::NodeFlag::Disabled), Ui::ButtonStyle::Flat, Ui::Icon::Yes, "Flat");

        /* Labels */
        Ui::label(
            snap(Ui::Snap::Bottom|Ui::Snap::Left|Ui::Snap::InsideX,
                 buttonDefaultDisabled, {0, 16}, {96, 16}),
            Ui::LabelStyle::Dim, "Labels", Text::Alignment::MiddleLeft);
        snap.setNextSize({96, 28});

        Ui::NodeHandle labelDefault = Ui::label(
            snap(Ui::Snap::Bottom|Ui::Snap::Left|Ui::Snap::InsideX),
            Ui::LabelStyle::Default, "Default");
        Ui::label(snap, Ui::LabelStyle::Primary, "Primary");
        Ui::label(snap, Ui::LabelStyle::Danger, "Danger");
        Ui::label(snap, Ui::LabelStyle::Success, "Success");
        Ui::label(snap, Ui::LabelStyle::Warning, "Warning");
        Ui::label(snap, Ui::LabelStyle::Info, "Info");
        Ui::label(snap, Ui::LabelStyle::Dim, "Dim");

        Ui::NodeHandle labelDefaultDisabled = Ui::label(
            snap(Ui::Snap::Bottom|Ui::Snap::Left|Ui::Snap::InsideX,
                 labelDefault, Ui::NodeFlag::Disabled),
            Ui::LabelStyle::Default, "Default");
        Ui::label(snap(Ui::NodeFlag::Disabled), Ui::LabelStyle::Primary, "Primary");
        Ui::label(snap(Ui::NodeFlag::Disabled), Ui::LabelStyle::Danger, "Danger");
        Ui::label(snap(Ui::NodeFlag::Disabled), Ui::LabelStyle::Success, "Success");
        Ui::label(snap(Ui::NodeFlag::Disabled), Ui::LabelStyle::Warning, "Warning");
        Ui::label(snap(Ui::NodeFlag::Disabled), Ui::LabelStyle::Info, "Info");
        Ui::label(snap(Ui::NodeFlag::Disabled), Ui::LabelStyle::Dim, "Dim");

        /* Inputs */
        Ui::label(
            snap(Ui::Snap::Bottom|Ui::Snap::Left|Ui::Snap::InsideX,
                 labelDefaultDisabled, {0, 16}, {96, 16}),
            Ui::LabelStyle::Dim, "Inputs", Text::Alignment::MiddleLeft);
        snap.setNextSize({208, 36});

        Ui::Input inputDefault{
            snap(Ui::Snap::Bottom|Ui::Snap::Left|Ui::Snap::InsideX),
            Ui::InputStyle::Default, "Hello! Type in me."};
        Ui::DataHandle inputDefaultTextHandle = inputDefault.textData();
        _ui.eventLayer().onFocus(inputDefault, [this, inputDefaultTextHandle]{
            if(_inputCursorAnimation != Whee::AnimationHandle::Null)
                return;
            _inputCursorAnimation = _textStyleAnimator->create(
                Whee::Implementation::TextStyle::InputDefaultFocused,
                Whee::Implementation::TextStyle::InputDefaultFocusedBlink,
                Animation::Easing::bounceIn, now(), 0.5_sec, inputDefaultTextHandle, 0);
        });
        _ui.eventLayer().onBlur(inputDefault, [this, inputDefaultTextHandle]{
            if(_inputCursorAnimation == Whee::AnimationHandle::Null)
                return;
            _textStyleAnimator->stop(_inputCursorAnimation, now());
            _textStyleAnimator->create(
                Whee::Implementation::TextStyle::InputDefaultFocusedBlink,
                Whee::Implementation::TextStyle::InputDefaultFocused,
                Animation::Easing::bounceOut, now(), 1.0_sec, inputDefaultTextHandle, 1);
            _textStyleAnimator->create(
                Whee::Implementation::TextStyle::InputDefaultFocused,
                Whee::Implementation::TextStyle::InputDefaultFocusedFadeOut,
                Animation::Easing::smoothstep, now() + 1.0_sec, 0.2_sec, inputDefaultTextHandle, 1);
            _inputCursorAnimation = Whee::AnimationHandle::Null;
        });
        // TODO cursor setter on the input ffs
        _ui.textLayer().setCursor(inputDefault.textData(), 11, 7);
        inputDefault.release();

        _clickMe = Whee::Button{
            snap(Ui::Snap::Bottom|Ui::Snap::Left|Ui::Snap::InsideX, {0, 16}, {208, 64}),
            Whee::ButtonStyle::Default, "Click me!"};
        _ui.eventLayer().onTapOrClick(*_clickMe, [this]{
            if(_clickMe->style() == Whee::ButtonStyle::Dim)
                _clickMe->setStyle(Whee::ButtonStyle::Default);
            else
                _clickMe->setStyle(Whee::ButtonStyle(UnsignedInt(_clickMe->style()) + 1));
        });
        _ui.eventLayer().onTapOrClick(*_clickMe, [this]{
            _nodeAnimator->create(*_clickMe, {5.0f, 9.0f}, now(), 1.0_sec);
        });
    }

    popup();

    #ifdef CORRADE_TARGET_EMSCRIPTEN
    startTextInput(); // TODO AHHHHHHH
    #endif

    GL::Renderer::setClearColor(0x22272e_rgbf);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::One, GL::Renderer::BlendFunction::OneMinusSourceAlpha);
}

void WheeGallery::popup() {
    // TODO make null parent default?, and null parent implying null target node (and disallowed otherwise?)
    Ui::SnapLayout snap{_ui, *_layouter, Ui::NodeHandle::Null};

    // TODO drop this extra root node, the snap layouter should have a knowledge about the UI size on its own if passed a null target
    Ui::NodeHandle root = _ui.createNode({}, _ui.size());
    Whee::NodeHandle popup = snap(Ui::Snaps{}, root, {440, 240});
    Whee::DataHandle popupBackground = _backgroundBlurBaseLayer->create(0, popup);
    _ui.eventLayer().onDrag(popup, [this, popup](const Vector2& offset){
        _ui.setNodeOffset(popup, _ui.nodeOffset(popup) + offset);
    });
    _ui.eventLayer().onPress(popup, [this, popup, popupBackground]{
        _styleAnimator->create(0, 1, Animation::Easing::circularIn, now(), 0.3_sec, popupBackground);
        _ui.setNodeOrder(popup, Whee::NodeHandle::Null);
    });
    _ui.eventLayer().onRelease(popup, [this, popup, popupBackground]{
        _styleAnimator->create(1, 0, Animation::Easing::smoothstep, now(), 1.3_sec, popupBackground);
        _ui.setNodeOrder(popup, Whee::NodeHandle::Null);
    });

    #if 1
    Whee::NodeHandle another = Whee::button({_ui, popup, {67, 170}, {128, 36}},
        Whee::ButtonStyle::Success, "Another!");
    Whee::NodeHandle more = Whee::button({_ui, popup, {245, 170}, {128, 36}},
        Whee::ButtonStyle::Primary, "More!");
    #else
    // TODO doesn't work because the layouter doesn't calculate parent-relative offsets because it doesn't know what's the parent
    // TODO besides that, how does one align two things to the center?? it seems completely incapable of such a thing ffs
    // TODO that implementation is just useless as a whole
    Whee::NodeHandle another = Whee::button(
        snap(Ui::Snap::Bottom|Ui::Snap::Inside, popup, {-72, 0}, {128, 36}),
        Whee::ButtonStyle::Success, "Another!");
    Whee::NodeHandle more = Whee::button(
        snap(Ui::Snap::Bottom|Ui::Snap::Inside, popup, {+72, 0}, {128, 36}),
        Whee::ButtonStyle::Primary, "More!");
    #endif
    _ui.eventLayer().onTapOrClick(another, [this]{
        this->popup();
    });

    auto makeTooltipFor = [&](Ui::NodeHandle button, Containers::StringView text) {
        Ui::NodeHandle tooltip = _ui.createNode(button, {16, 32}, {});
        _ui.setNodeOrder(tooltip, Ui::NodeHandle::Null);
        _ui.clearNodeOrder(tooltip); // TODO hmm, couldn't this do it implicitly without having to setNodeOrder first? or maybe just rename the function to something reasonable
        _backgroundBlurBaseLayer->create(2, tooltip);
        Ui::DataHandle textData = _ui.textLayer().create(Ui::Implementation::TextStyle::InputDefaultInactiveOut, text, Text::Alignment::MiddleCenter, tooltip);
        _ui.setNodeSize(tooltip, _ui.textLayer().size(textData) + Vector2{16.0f, 10.0f});
        _ui.eventLayer().onEnter(button, [this, tooltip]{
            _ui.setNodeOrder(tooltip, Ui::NodeHandle::Null);
        });
        _ui.eventLayer().onLeave(button, [this, tooltip]{
            if(_ui.currentHoveredNode() != tooltip)
                _ui.clearNodeOrder(tooltip);
        });
        // TODO ehhhh
        _ui.eventLayer().onLeave(tooltip, [this, tooltip, button]{
            if(_ui.currentHoveredNode() != button)
                _ui.clearNodeOrder(tooltip);
        });
    };
    makeTooltipFor(another, "Open another popup.");
    makeTooltipFor(more, "Blur even more. Until you fry the GPU.");

    _ui.eventLayer().onTapOrClick(more, [this]{
        _backgroundBlurBaseLayer->setBackgroundBlurPassCount(_backgroundBlurBaseLayer->backgroundBlurPassCount()*2);
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

void WheeGallery::keyPressEvent(KeyEvent& event) {
    _ui.keyPressEvent(event);

    if(_ui.state()) redraw();
}

void WheeGallery::keyReleaseEvent(KeyEvent& event) {
    _ui.keyReleaseEvent(event);

    if(_ui.state()) redraw();
}

void WheeGallery::textInputEvent(TextInputEvent& event) {
    _ui.textInputEvent(event);

    if(_ui.state()) redraw();
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::WheeGallery)
