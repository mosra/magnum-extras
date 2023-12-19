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

#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/PluginManager/Manager.h>
#include <Corrade/Utility/Format.h>
#include <Corrade/Utility/Resource.h>
#include <Magnum/Mesh.h>
#include <Magnum/Math/Color.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Renderer.h>
#ifdef CORRADE_TARGET_EMSCRIPTEN
#include <Magnum/Platform/EmscriptenApplication.h>
#else
#include <Magnum/Platform/Sdl2Application.h>
#endif
#include <Magnum/Text/Alignment.h>
#include <Magnum/Text/AbstractFont.h>
#include <Magnum/Text/GlyphCache.h>
#include <Magnum/TextureTools/Atlas.h>

#include "Magnum/Whee/AbstractUserInterface.h"
#include "Magnum/Whee/BaseLayerGL.h"
#include "Magnum/Whee/EventLayer.h"
#include "Magnum/Whee/Event.h"
#include "Magnum/Whee/Handle.h"
#include "Magnum/Whee/TextLayerGL.h"
#include "Magnum/Whee/TextProperties.h"

namespace Magnum { namespace {

using namespace Containers::Literals;
using namespace Math::Literals;

enum class BaseStyleIndex: UnsignedInt {
    DialogBackground,
    DialogTitleButton,
    DialogTitleButtonHovered,
    DialogTitleButtonPressedHovered,
    Button,
    ButtonHovered,
    ButtonPressedHovered,
    ButtonDisabled,
    ProgressBar,

    Count
};

enum class TextStyleIndex: UnsignedInt {
    DefaultDark,
    DefaultLight,
    Title,

    Count
};

BaseStyleIndex styleIndexTransitionToPressedBlur(BaseStyleIndex index) {
    switch(index) {
        case BaseStyleIndex::Button:
        case BaseStyleIndex::ButtonHovered:
        case BaseStyleIndex::ButtonPressedHovered:
            return BaseStyleIndex::ButtonPressedHovered;
        case BaseStyleIndex::DialogTitleButton:
        case BaseStyleIndex::DialogTitleButtonHovered:
        case BaseStyleIndex::DialogTitleButtonPressedHovered:
            return BaseStyleIndex::DialogTitleButtonPressedHovered;
        case BaseStyleIndex::DialogBackground:
        case BaseStyleIndex::ButtonDisabled:
        case BaseStyleIndex::ProgressBar:
            return index;
        case BaseStyleIndex::Count:
            CORRADE_INTERNAL_ASSERT_UNREACHABLE();
    }

    CORRADE_INTERNAL_ASSERT_UNREACHABLE();
}

BaseStyleIndex styleIndexTransitionToPressedHover(BaseStyleIndex index) {
    switch(index) {
        case BaseStyleIndex::Button:
        case BaseStyleIndex::ButtonHovered:
        case BaseStyleIndex::ButtonPressedHovered:
            return BaseStyleIndex::ButtonPressedHovered;
        case BaseStyleIndex::DialogTitleButton:
        case BaseStyleIndex::DialogTitleButtonHovered:
        case BaseStyleIndex::DialogTitleButtonPressedHovered:
            return BaseStyleIndex::DialogTitleButtonPressedHovered;
        case BaseStyleIndex::DialogBackground:
        case BaseStyleIndex::ButtonDisabled:
        case BaseStyleIndex::ProgressBar:
            return index;
        case BaseStyleIndex::Count:
            CORRADE_INTERNAL_ASSERT_UNREACHABLE();
    }

    CORRADE_INTERNAL_ASSERT_UNREACHABLE();
}

BaseStyleIndex styleIndexTransitionToInactiveBlur(BaseStyleIndex index) {
    switch(index) {
        case BaseStyleIndex::Button:
        case BaseStyleIndex::ButtonHovered:
        case BaseStyleIndex::ButtonPressedHovered:
            return BaseStyleIndex::Button;
        case BaseStyleIndex::DialogTitleButton:
        case BaseStyleIndex::DialogTitleButtonHovered:
        case BaseStyleIndex::DialogTitleButtonPressedHovered:
            return BaseStyleIndex::DialogTitleButton;
        case BaseStyleIndex::DialogBackground:
        case BaseStyleIndex::ButtonDisabled:
        case BaseStyleIndex::ProgressBar:
            return index;
        case BaseStyleIndex::Count:
            CORRADE_INTERNAL_ASSERT_UNREACHABLE();
    }

    CORRADE_INTERNAL_ASSERT_UNREACHABLE();
}

BaseStyleIndex styleIndexTransitionToInactiveHover(BaseStyleIndex index) {
    switch(index) {
        case BaseStyleIndex::Button:
        case BaseStyleIndex::ButtonHovered:
        case BaseStyleIndex::ButtonPressedHovered:
            return BaseStyleIndex::ButtonHovered;
        case BaseStyleIndex::DialogTitleButton:
        case BaseStyleIndex::DialogTitleButtonHovered:
        case BaseStyleIndex::DialogTitleButtonPressedHovered:
            return BaseStyleIndex::DialogTitleButtonHovered;
        case BaseStyleIndex::DialogBackground:
        case BaseStyleIndex::ButtonDisabled:
        case BaseStyleIndex::ProgressBar:
            return index;
        case BaseStyleIndex::Count:
            CORRADE_INTERNAL_ASSERT_UNREACHABLE();
    }

    CORRADE_INTERNAL_ASSERT_UNREACHABLE();
}

class UserInterface: public Whee::AbstractUserInterface {
    public:
        explicit UserInterface(const Vector2& size, const Vector2& windowSize, const Vector2i& framebufferSize): Whee::AbstractUserInterface{size, windowSize, framebufferSize} {
            Whee::BaseLayerCommonStyleUniform baseLayerCommonStyleUniform;
            baseLayerCommonStyleUniform.setSmoothness(0.5f);
            Whee::BaseLayerStyleUniform baseLayerStyleUniforms[UnsignedInt(BaseStyleIndex::Count)];
            baseLayerStyleUniforms[UnsignedInt(BaseStyleIndex::DialogBackground)]
                .setCornerRadius({16.0f, 4.0f, 16.0f, 4.0f})
                .setInnerOutlineCornerRadius({2.0f, 2.0f, 2.0f, 2.0f})
                .setOutlineWidth({0.0f, 32.0f, 0.0f, 2.0f})
                .setColor(0x2f363fee_rgbaf)
                .setOutlineColor(0x282e36cc_rgbaf);
            baseLayerStyleUniforms[UnsignedInt(BaseStyleIndex::DialogTitleButton)]
                .setCornerRadius(8.0f)
                .setColor(0xffffff_rgbf, 0xdcdcdc_rgbf);
            baseLayerStyleUniforms[UnsignedInt(BaseStyleIndex::DialogTitleButtonHovered)]
                .setCornerRadius(8.0f)
                .setColor(0xffffff_rgbf, 0xffffff_rgbf);
            baseLayerStyleUniforms[UnsignedInt(BaseStyleIndex::DialogTitleButtonPressedHovered)]
                .setCornerRadius(8.0f)
                .setColor(0xdcdcdc_rgbf, 0xffffff_rgbf);
            baseLayerStyleUniforms[UnsignedInt(BaseStyleIndex::Button)]
                .setCornerRadius(16.0f)
                .setColor(0x3bd267_rgbf, 0x3bd267_rgbf*0xdcdcdc_rgbf);
            baseLayerStyleUniforms[UnsignedInt(BaseStyleIndex::ButtonHovered)]
                .setCornerRadius(16.0f)
                .setColor(0x3bd267_rgbf, 0x3bd267_rgbf);
            baseLayerStyleUniforms[UnsignedInt(BaseStyleIndex::ButtonPressedHovered)]
                .setCornerRadius(16.0f)
                .setColor(0x3bd267_rgbf*0xdcdcdc_rgbf, 0x3bd267_rgbf);
            baseLayerStyleUniforms[UnsignedInt(BaseStyleIndex::ButtonDisabled)]
                .setCornerRadius(16.0f)
                .setColor(0x3bd267ff_rgbaf*0.3f, 0x3bd267ff_rgbaf*0.3f);
            baseLayerStyleUniforms[UnsignedInt(BaseStyleIndex::ProgressBar)]
                .setCornerRadius(12.0f)
                .setOutlineWidth(2.0f)
                .setInnerOutlineCornerRadius(10.0f)
                .setColor(0xa5c9ea_rgbf, 0xa5c9ea_rgbf*0xdcdcdc_rgbf)
                .setOutlineColor(0x34424d_rgbf/(0xa5c9ea_rgbf*0xdcdcdc_rgbf));
            _baseLayerShared = Whee::BaseLayerGL::Shared{UnsignedInt(Containers::arraySize(baseLayerStyleUniforms))};
            _baseLayerShared
                .setStyle(baseLayerCommonStyleUniform, baseLayerStyleUniforms, {})
                .setStyleTransition<BaseStyleIndex,
                    styleIndexTransitionToPressedBlur,
                    styleIndexTransitionToPressedHover,
                    styleIndexTransitionToInactiveBlur,
                    styleIndexTransitionToInactiveHover,
                    nullptr>();

            const Utility::Resource rs{"MagnumWheeGallery"};

            // TODO artifacts!
            Text::GlyphCache glyphCache{Vector2i{256}};
            Containers::Pointer<Text::AbstractFont> font = _fontManager.loadAndInstantiate("TrueTypeFont");
            // TODO account for DPI scaling
            CORRADE_INTERNAL_ASSERT_OUTPUT(font && font->openData(rs.getRaw("SourceSansPro-Regular.ttf"), 16.0f*2));
            font->fillGlyphCache(glyphCache,
                "abcdefghijklmnopqrstuvwxyz"
                "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                "0123456789 _.,-+=*:;?!@$&#/\\|`\"'<>()[]{}%…");

            Containers::Pointer<Text::AbstractFont> fontSmall = _fontManager.loadAndInstantiate("TrueTypeFont");
            CORRADE_INTERNAL_ASSERT_OUTPUT(fontSmall && fontSmall->openData(rs.getRaw("SourceSansPro-Regular.ttf"), 13.0f*2));
            fontSmall->fillGlyphCache(glyphCache,
                "abcdefghijklmnopqrstuvwxyz"
                "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                "0123456789 _.,-+=*:;?!@$&#/\\|`\"'<>()[]{}%…");

            Debug{} << "Glyph cache filled to" << glyphCache.atlas().filledSize().y()*100.0f/glyphCache.atlas().size().y() << Debug::nospace << "%";

            Whee::TextLayerStyleUniform textLayerStyleUniforms[UnsignedInt(TextStyleIndex::Count)];
            textLayerStyleUniforms[UnsignedInt(TextStyleIndex::DefaultDark)]
                .setColor(0x22272e_rgbf);
            textLayerStyleUniforms[UnsignedInt(TextStyleIndex::DefaultLight)]
                .setColor(0xdcdcdc_rgbf);
            textLayerStyleUniforms[UnsignedInt(TextStyleIndex::Title)]
                .setColor(0xdcdcdc_rgbf);

            _textLayerShared = Whee::TextLayerGL::Shared{UnsignedInt(Containers::arraySize(textLayerStyleUniforms))};
            _textLayerShared.setGlyphCache(Utility::move(glyphCache));
            Whee::FontHandle fontHandle = _textLayerShared.addFont(Utility::move(font), 16.0f);
            Whee::FontHandle fontSmallHandle = _textLayerShared.addFont(Utility::move(fontSmall), 13.0f);
            _textLayerShared.setStyle(Whee::TextLayerCommonStyleUniform{},
                textLayerStyleUniforms,
                Containers::arrayView({fontHandle, fontHandle, fontSmallHandle}),
                {});

            _baseLayer = &setLayerInstance(Containers::pointer<Whee::BaseLayerGL>(createLayer(), _baseLayerShared));

            _textLayer = &setLayerInstance(Containers::pointer<Whee::TextLayerGL>(createLayer(), _textLayerShared));

            _eventLayer = &setLayerInstance(Containers::pointer<Whee::EventLayer>(createLayer()));
        }

        Whee::BaseLayer& baseLayer() { return *_baseLayer; }
        Whee::TextLayer& textLayer() { return *_textLayer; }
        Whee::EventLayer& eventLayer() { return *_eventLayer; }

    private:
        PluginManager::Manager<Text::AbstractFont> _fontManager;

        Whee::BaseLayerGL::Shared _baseLayerShared{NoCreate};
        Whee::TextLayerGL::Shared _textLayerShared{NoCreate};
        Whee::BaseLayer* _baseLayer;
        Whee::TextLayer* _textLayer;
        Whee::EventLayer* _eventLayer;
};

class Popup {
    public:
        explicit Popup(UserInterface& ui, Float progress, bool closeable) {
            Whee::NodeHandle popup = _popup = ui.createNode({180, 180}, {440, 240});
            ui.baseLayer().create(BaseStyleIndex::DialogBackground, _popup);
            ui.eventLayer().onDrag(_popup, [&ui, popup](const Vector2& offset){
                ui.setNodeOffset(popup, ui.nodeOffset(popup) + offset);
            });
            ui.eventLayer().onPress(_popup, [&ui, popup]{
                ui.setNodeOrder(popup, Whee::NodeHandle::Null);
            });
            Whee::NodeHandle title = ui.createNode(popup, {24, 0}, {380, 32});
            // TODO NodeFlag::NoEvent (Inactive? ReadOnly? ugh) to make this
            //  text not react to press
            ui.textLayer().create(TextStyleIndex::Title,
                Utility::format("Progress bar with {:.1f}%", progress*100.0f),
                Whee::TextProperties{}
                    .setAlignment(Text::Alignment::MiddleLeft),
                title);

            if(closeable) {
                Whee::NodeHandle closeButton = ui.createNode(_popup, {415, 8}, {16, 16});
                ui.baseLayer().create(BaseStyleIndex::DialogTitleButton, 0xcd3431_rgbf, closeButton);
                ui.eventLayer().onTapOrClick(closeButton, [&ui, popup]{
                    ui.removeNode(popup);
                });
            } else {
                Whee::NodeHandle minimizeButton = ui.createNode(_popup, {390, 8}, {16, 16});
                ui.baseLayer().create(BaseStyleIndex::DialogTitleButton, 0xc7cf2f_rgbf, minimizeButton);
                ui.eventLayer().onTapOrClick(minimizeButton, [&ui, popup]{
                    ui.clearNodeOrder(popup);
                });
            }

            Float progressOutlineWidth = Math::min(296.0f, 316.0f*(1.0f - progress));

            Whee::NodeHandle progressBar = ui.createNode(_popup, {60, 100}, {320, 24});
            ui.baseLayer().create(BaseStyleIndex::ProgressBar, 0xffffff_rgbf, {0.0f, 0.0f, progressOutlineWidth, 0.0f}, progressBar);

            Whee::NodeHandle button = ui.createNode(_popup, {156, 170}, {128, 32});
            ui.textLayer().create(TextStyleIndex::DefaultDark, Utility::format("Go to {:.1f}%", progress*80.0f), {}, button);
            if(progressOutlineWidth < 296.0f) {
                ui.baseLayer().create(BaseStyleIndex::Button, button);
                ui.eventLayer().onTapOrClick(button, [&ui, progress]{
                    Popup{ui, progress*0.8f, true};
                });
            } else {
                ui.baseLayer().create(BaseStyleIndex::ButtonDisabled, button);
            }
        }

        // TODO not needed for anything, actually the whole class could be just a function, heh
        operator Whee::NodeHandle() const { return _popup; }

    private:
        Whee::NodeHandle _popup;
};

class WheeGallery: public Platform::Application {
    public:
        explicit WheeGallery(const Arguments& arguments);

    private:
        void drawEvent() override;
        void mousePressEvent(MouseEvent& event) override;
        void mouseReleaseEvent(MouseEvent& event) override;
        void mouseMoveEvent(MouseMoveEvent& event) override;

        UserInterface _ui;
        Containers::Optional<Popup> _popup;
};

WheeGallery::WheeGallery(const Arguments& arguments): Platform::Application{arguments, Configuration{}.setTitle("Magnum::Whee Gallery"_s)}, _ui{{800, 600}, Vector2{windowSize()}, framebufferSize()} {
    _popup.emplace(_ui, 0.7f, false);

    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::One, GL::Renderer::BlendFunction::OneMinusSourceAlpha);
}

void WheeGallery::drawEvent() {
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color|GL::FramebufferClear::Depth);

    _ui.draw();

    swapBuffers();
}

void WheeGallery::mousePressEvent(MouseEvent& event) {
    if(event.button() == MouseEvent::Button::Left) {
        Whee::PointerEvent e{Whee::Pointer::MouseLeft};
        // TODO integer overload? or just have a template wrapper that does this automagically
        if(_ui.pointerPressEvent(Vector2{event.position()}, e))
            ;
        else if(!_ui.isNodeOrdered(*_popup))
            _ui.setNodeOrder(*_popup, Whee::NodeHandle::Null);
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
