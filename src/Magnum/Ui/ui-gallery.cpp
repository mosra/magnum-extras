/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019
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

#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/Reference.h>
#include <Corrade/Interconnect/Receiver.h>
#include <Corrade/PluginManager/Manager.h>
#include <Corrade/Utility/Arguments.h>
#include <Corrade/Utility/DebugStl.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Renderer.h>
#ifndef CORRADE_TARGET_ANDROID
#include <Magnum/Platform/Sdl2Application.h>
#else
#include <Magnum/Platform/AndroidApplication.h>
#endif
#include <Magnum/Text/Alignment.h>

#include "Magnum/Ui/Anchor.h"
#include "Magnum/Ui/Button.h"
#include "Magnum/Ui/Input.h"
#include "Magnum/Ui/Label.h"
#include "Magnum/Ui/Modal.h"
#include "Magnum/Ui/Plane.h"
#include "Magnum/Ui/UserInterface.h"

namespace Magnum {

/** @page magnum-ui-gallery Magnum::Ui Gallery
@brief Showcases widgets and features provided by @ref Magnum::Ui

@m_footernavigation
@m_keywords{magnum-ui-gallery ui-gallery}

@image html ui-gallery.png width=400px

@m_div{m-button m-primary} <a href="https://magnum.graphics/showcase/magnum-ui-gallery/">@m_div{m-big}Live web version @m_enddiv @m_div{m-small} uses WebAssembly & WebGL 2 @m_enddiv </a> @m_enddiv

This app is built if both `WITH_UI` and `WITH_UI_GALLERY` is enabled when
building Magnum Extras. To use this app with CMake, you need to request the
`ui-gallery` component of the `MagnumExtras` package and use the
`MagnumExtras::ui-gallery` target for example in a custom command:

@code{.cmake}
find_package(MagnumExtras REQUIRED ui-gallery)

add_custom_command(OUTPUT ... COMMAND MagnumExtras::ui-gallery ...)
@endcode

@section magnum-ui-gallery-usage Usage

@code{.sh}
magnum-ui-gallery [--magnum-...] [-h|--help] [--style STYLE]
@endcode

Arguments:

-   `-h`, `--help` --- display this help message and exit
-   `--style STYLE` --- specify style to use (default: `mcss-dark`). One of:
    -   `default` --- the default style
    -   `mcss-dark` --- dark [m.css](http://mcss.mosra.cz) theme
-   `--magnum-...` --- engine-specific options (see
    @ref GL-Context-command-line for details)

*/

using namespace Magnum::Math::Literals;

namespace {

constexpr const Float WidgetHeight{36.0f};
constexpr const Float LabelHeight{24.0f};
constexpr const Vector2 ButtonSize{96.0f, WidgetHeight};
constexpr const Vector2 LabelSize{72.0f, LabelHeight};

struct BaseUiPlane: Ui::Plane {
    explicit BaseUiPlane(Ui::UserInterface& ui):
        Ui::Plane{ui, Ui::Snap::Top|Ui::Snap::Bottom|Ui::Snap::Left|Ui::Snap::Right, 0, 50, 640},
        buttonDefault{*this, {Ui::Snap::Top|Ui::Snap::Left, Range2D::fromSize(Vector2::yAxis(-36.0f), ButtonSize)},
            "Default", Ui::Style::Default},
        buttonPrimary{*this, {Ui::Snap::Right, buttonDefault, ButtonSize},
            "Primary", Ui::Style::Primary},
        buttonDanger{*this, {Ui::Snap::Right, buttonPrimary, ButtonSize},
            "Danger", Ui::Style::Danger},
        buttonSuccess{*this, {Ui::Snap::Right, buttonDanger, ButtonSize},
            "Success", Ui::Style::Success},
        buttonWarning{*this, {Ui::Snap::Right, buttonSuccess, ButtonSize},
            "Warning", Ui::Style::Warning},
        buttonFlat{*this, {Ui::Snap::Right, buttonWarning, {60.0f, WidgetHeight}},
            "Flat", Ui::Style::Flat},

        inputDefault{*this, {Ui::Snap::Top|Ui::Snap::Left,
            Range2D::fromSize(Vector2::yAxis(-282.0f), ButtonSize)},
            "Default", 8, Ui::Style::Default},
        inputDanger{*this, {Ui::Snap::Right, inputDefault, ButtonSize},
            "Danger", 8, Ui::Style::Danger},
        inputSuccess{*this, {Ui::Snap::Right, inputDanger, ButtonSize},
            "Success", 8, Ui::Style::Success},
        inputWarning{*this, {Ui::Snap::Right, inputSuccess, ButtonSize},
            "Warning", 8, Ui::Style::Warning},
        inputFlat{*this, {Ui::Snap::Right, inputWarning, ButtonSize},
            "Flat", 8, Ui::Style::Flat},

        modalDefault{*this, {Ui::Snap::Top|Ui::Snap::Left,
            Range2D::fromSize(Vector2::yAxis(-414.0f), ButtonSize)},
            "Default »"},
        modalDanger{*this, {Ui::Snap::Right, modalDefault, ButtonSize},
            "Danger »"},
        modalSuccess{*this, {Ui::Snap::Right, modalDanger, ButtonSize},
            "Success »"},
        modalWarning{*this, {Ui::Snap::Right, modalSuccess, ButtonSize},
            "Warning »"},
        modalInfo{*this, {Ui::Snap::Right, modalWarning, ButtonSize},
            "Info »"}
    {
        Ui::Button
            buttonDefaultDisabled{*this, {Ui::Snap::Bottom, buttonDefault, ButtonSize},
                "Default", Ui::Style::Default},
            buttonPrimaryDisabled{*this, {Ui::Snap::Bottom, buttonPrimary, ButtonSize},
                "Primary", Ui::Style::Primary},
            buttonDangerDisabled{*this, {Ui::Snap::Bottom, buttonDanger, ButtonSize},
                "Danger", Ui::Style::Danger},
            buttonSuccessDisabled{*this, {Ui::Snap::Bottom, buttonSuccess, ButtonSize},
                "Success", Ui::Style::Success},
            buttonWarningDisabled{*this, {Ui::Snap::Bottom, buttonWarning, ButtonSize},
                "Warning", Ui::Style::Warning},
            buttonFlatDisabled{*this, {Ui::Snap::Bottom, buttonFlat, ButtonSize},
                "Flat", Ui::Style::Flat};

        Ui::Widget::disable({
            buttonPrimaryDisabled,
            buttonDangerDisabled,
            buttonSuccessDisabled,
            buttonWarningDisabled,
            buttonFlatDisabled,
            buttonDefaultDisabled});

        Ui::Label
            labelDefault{*this, {Ui::Snap::Top|Ui::Snap::Left, Range2D::fromSize(Vector2::yAxis(-172.0f), LabelSize)},
                "Default", Text::Alignment::LineCenterIntegral, Ui::Style::Default},
            labelPrimary{*this, {Ui::Snap::Right, labelDefault, LabelSize},
                "Primary", Text::Alignment::LineCenterIntegral, Ui::Style::Primary},
            labelDanger{*this, {Ui::Snap::Right, labelPrimary, LabelSize},
                "Danger", Text::Alignment::LineCenterIntegral, Ui::Style::Danger},
            labelSuccess{*this, {Ui::Snap::Right, labelDanger, LabelSize},
                "Success", Text::Alignment::LineCenterIntegral, Ui::Style::Success},
            labelWarning{*this, {Ui::Snap::Right, labelSuccess, LabelSize},
                "Warning", Text::Alignment::LineCenterIntegral, Ui::Style::Warning},
            labelInfo{*this, {Ui::Snap::Right, labelWarning, LabelSize},
                "Info", Text::Alignment::LineCenterIntegral, Ui::Style::Info},
            labelDim{*this, {Ui::Snap::Right, labelInfo, LabelSize},
                "Dim", Text::Alignment::LineCenterIntegral, Ui::Style::Dim};

        Ui::Label
            labelDefaultDisabled{*this, {Ui::Snap::Bottom, labelDefault, LabelSize},
                "Default", Text::Alignment::LineCenterIntegral, Ui::Style::Default},
            labelPrimaryDisabled{*this, {Ui::Snap::Bottom, labelPrimary, LabelSize},
                "Primary", Text::Alignment::LineCenterIntegral, Ui::Style::Primary},
            labelDangerDisabled{*this, {Ui::Snap::Bottom, labelDanger, LabelSize},
                "Danger", Text::Alignment::LineCenterIntegral, Ui::Style::Danger},
            labelSuccessDisabled{*this, {Ui::Snap::Bottom, labelSuccess, LabelSize},
                "Success", Text::Alignment::LineCenterIntegral, Ui::Style::Success},
            labelWarningDisabled{*this, {Ui::Snap::Bottom, labelWarning, LabelSize},
                "Warning", Text::Alignment::LineCenterIntegral, Ui::Style::Warning},
            labelInfoDisabled{*this, {Ui::Snap::Bottom, labelInfo, LabelSize},
                "Info", Text::Alignment::LineCenterIntegral, Ui::Style::Info},
            labelDimDisabled{*this, {Ui::Snap::Bottom, labelDim, LabelSize},
                "Dim", Text::Alignment::LineCenterIntegral, Ui::Style::Dim};

        Ui::Widget::disable({
            labelDefaultDisabled,
            labelPrimaryDisabled,
            labelDangerDisabled,
            labelSuccessDisabled,
            labelWarningDisabled,
            labelInfoDisabled,
            labelDimDisabled});

        Ui::Input
            inputDefaultDisabled{*this, {Ui::Snap::Bottom, inputDefault, ButtonSize},
                "Default", 32, Ui::Style::Default},
            inputDangerDisabled{*this, {Ui::Snap::Bottom, inputDanger, ButtonSize},
                "Danger", 32, Ui::Style::Danger},
            inputSuccessDisabled{*this, {Ui::Snap::Bottom, inputSuccess, ButtonSize},
                "Success", 32, Ui::Style::Success},
            inputWarningDisabled{*this, {Ui::Snap::Bottom, inputWarning, ButtonSize},
                "Warning", 32, Ui::Style::Warning},
            inputFlatDisabled{*this, {Ui::Snap::Bottom, inputFlat, ButtonSize},
                "Flat", 32, Ui::Style::Flat};

        Ui::Widget::disable({
            inputDefaultDisabled,
            inputDangerDisabled,
            inputSuccessDisabled,
            inputWarningDisabled,
            inputFlatDisabled});

        Ui::Label{*this, {Ui::Snap::Top|Ui::Snap::Left|Ui::Snap::InsideX, buttonDefault, LabelSize},
            "Buttons", Text::Alignment::LineLeft, Ui::Style::Dim};
        Ui::Label{*this, {Ui::Snap::Top|Ui::Snap::Left|Ui::Snap::InsideX, labelDefault, LabelSize},
            "Labels", Text::Alignment::LineLeft, Ui::Style::Dim};
        Ui::Label{*this, {Ui::Snap::Top|Ui::Snap::Left|Ui::Snap::InsideX, inputDefault, LabelSize},
            "Inputs", Text::Alignment::LineLeft, Ui::Style::Dim};
        Ui::Label{*this, {Ui::Snap::Top|Ui::Snap::Left|Ui::Snap::InsideX, modalDefault, LabelSize},
            "Modals", Text::Alignment::LineLeft, Ui::Style::Dim};
    }

    Ui::Button buttonDefault,
        buttonPrimary,
        buttonDanger,
        buttonSuccess,
        buttonWarning,
        buttonFlat;

    Ui::Input inputDefault,
        inputDanger,
        inputSuccess,
        inputWarning,
        inputFlat;

    Ui::Button modalDefault,
        modalDanger,
        modalSuccess,
        modalWarning,
        modalInfo;
};

struct ModalUiPlane: Ui::Plane, Interconnect::Receiver {
    explicit ModalUiPlane(Ui::UserInterface& ui, Ui::Style style):
        Ui::Plane{ui, {{}, {320.0f, 240.0f}}, 2, 3, 128},
        message{*this, {{}, Range2D::fromSize(Vector2::yAxis(20.0f), {})},
            "This is a modal dialog.", Text::Alignment::LineCenterIntegral, style},
        close{*this, {Ui::Snap::Bottom|Ui::Snap::Right, ButtonSize},
            "Close", style == Ui::Style::Info ? Ui::Style::Default : style}
    {
        Ui::Modal{*this, Ui::Snap::Top|Ui::Snap::Bottom|Ui::Snap::Left|Ui::Snap::Right|Ui::Snap::NoSpaceX|Ui::Snap::NoSpaceY, style};

        Ui::Label{*this, {Ui::Snap::Left|Ui::Snap::Top, Range2D::fromSize(Vector2::xAxis(10.0f), {{}, WidgetHeight})},
            "Modal", Text::Alignment::LineLeft, style};
    }

    Ui::Label message;
    Ui::Button close;
};

}

class Gallery: public Platform::Application, public Interconnect::Receiver {
    public:
        explicit Gallery(const Arguments& arguments);

    private:
        void drawEvent() override;
        void mousePressEvent(MouseEvent& event) override;
        void mouseReleaseEvent(MouseEvent& event) override;
        void mouseMoveEvent(MouseMoveEvent& event) override;
        #ifndef CORRADE_TARGET_ANDROID
        void keyPressEvent(KeyEvent& event) override;
        void textInputEvent(TextInputEvent& event) override;
        #endif

        Containers::Optional<Ui::UserInterface> _ui;
        Containers::Optional<BaseUiPlane> _baseUiPlane;
        Containers::Optional<ModalUiPlane> _defaultModalUiPlane,
            _dangerModalUiPlane,
            _successModalUiPlane,
            _warningModalUiPlane,
            _infoModalUiPlane;
};

Gallery::Gallery(const Arguments& arguments): Platform::Application{arguments, Configuration{}.setTitle("Magnum::Ui Gallery")
    #ifndef CORRADE_TARGET_ANDROID
    .setSize({640, 480})
    #endif
    #ifdef CORRADE_TARGET_IOS
    .setWindowFlags(Configuration::WindowFlag::Borderless|Configuration::WindowFlag::AllowHighDpi)
    #endif
    }
{
    Utility::Arguments args;
    args.addOption("style", "mcss-dark").setHelp("style", "specify style to use")
        .addSkippedPrefix("magnum", "engine-specific options")
        .setGlobalHelp(
R"(Showcases different widgets in the Magnum::Ui library. The --style option can
be one of:
  default       the default style
  mcss-dark     dark m.css theme from http://mcss.mosra.cz)")
        #ifndef CORRADE_TARGET_ANDROID
        .parse(arguments.argc, arguments.argv)
        #else
        .parse(0, nullptr)
        #endif
        ;

    /* Enable blending with premultiplied alpha */
    GL::Renderer::enable(GL::Renderer::Feature::Blending);
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::One, GL::Renderer::BlendFunction::OneMinusSourceAlpha);
    GL::Renderer::setBlendEquation(GL::Renderer::BlendEquation::Add, GL::Renderer::BlendEquation::Add);

    #if !defined(MAGNUM_TARGET_WEBGL) && !defined(CORRADE_TARGET_ANDROID)
    /* Have some sane speed, please */
    setMinimalLoopPeriod(16);
    #endif

    /* Decide about style to use */
    Ui::StyleConfiguration style;
    if(args.value("style") == "default")
        style = Ui::defaultStyleConfiguration();
    else if(args.value("style") == "mcss-dark") {
        style = Ui::mcssDarkStyleConfiguration();
        GL::Renderer::setClearColor(0x22272e_rgbf);
    } else Debug{} << "Unrecognized --style option" << args.value("style");

    /* Create the UI. It should get at least some screen space, but also
       shouldn't be tucked away into a small corner on ultra-dense displays. */
    _ui.emplace(Math::clamp({640.0f, 480.0f}, {1024.0f, 576.0f}, Vector2(windowSize()/dpiScaling())), windowSize(), framebufferSize(), style, "»");
    #ifndef CORRADE_TARGET_ANDROID
    Interconnect::connect(*_ui, &Ui::UserInterface::inputWidgetFocused, *this, &Gallery::startTextInput);
    Interconnect::connect(*_ui, &Ui::UserInterface::inputWidgetBlurred, *this, &Gallery::stopTextInput);
    #endif

    /* Create base UI plane */
    _baseUiPlane.emplace(*_ui);

    /* Create modals */
    _defaultModalUiPlane.emplace(*_ui, Ui::Style::Default);
    _dangerModalUiPlane.emplace(*_ui, Ui::Style::Danger);
    _successModalUiPlane.emplace(*_ui, Ui::Style::Success);
    _warningModalUiPlane.emplace(*_ui, Ui::Style::Warning);
    _infoModalUiPlane.emplace(*_ui, Ui::Style::Info);
    Interconnect::connect(_baseUiPlane->modalDefault, &Ui::Button::tapped, *_defaultModalUiPlane, &Ui::Plane::activate);
    Interconnect::connect(_baseUiPlane->modalDanger, &Ui::Button::tapped, *_dangerModalUiPlane, &Ui::Plane::activate);
    Interconnect::connect(_baseUiPlane->modalSuccess, &Ui::Button::tapped, *_successModalUiPlane, &Ui::Plane::activate);
    Interconnect::connect(_baseUiPlane->modalWarning, &Ui::Button::tapped, *_warningModalUiPlane, &Ui::Plane::activate);
    Interconnect::connect(_baseUiPlane->modalInfo, &Ui::Button::tapped, *_infoModalUiPlane, &Ui::Plane::activate);

    Interconnect::connect(_defaultModalUiPlane->close, &Ui::Button::tapped, *_defaultModalUiPlane, &Ui::Plane::hide);
    Interconnect::connect(_dangerModalUiPlane->close, &Ui::Button::tapped, *_dangerModalUiPlane, &Ui::Plane::hide);
    Interconnect::connect(_successModalUiPlane->close, &Ui::Button::tapped, *_successModalUiPlane, &Ui::Plane::hide);
    Interconnect::connect(_warningModalUiPlane->close, &Ui::Button::tapped, *_warningModalUiPlane, &Ui::Plane::hide);
    Interconnect::connect(_infoModalUiPlane->close, &Ui::Button::tapped, *_infoModalUiPlane, &Ui::Plane::hide);
}

void Gallery::drawEvent() {
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color|GL::FramebufferClear::Depth);

    _ui->draw();

    swapBuffers();
}

void Gallery::mousePressEvent(MouseEvent& event) {
    if(!_ui->handlePressEvent(event.position())) return;

    event.setAccepted();
    redraw();
}

void Gallery::mouseReleaseEvent(MouseEvent& event) {
    if(!_ui->handleReleaseEvent(event.position())) return;

    event.setAccepted();
    redraw();
}

void Gallery::mouseMoveEvent(MouseMoveEvent& event) {
    if(!_ui->handleMoveEvent(event.position())) return;

    event.setAccepted();
    redraw();
}

#ifndef CORRADE_TARGET_ANDROID
void Gallery::keyPressEvent(KeyEvent& event) {
    if(isTextInputActive() && _ui->focusedInputWidget() && _ui->focusedInputWidget()->handleKeyPress(event))
        redraw();
}

void Gallery::textInputEvent(TextInputEvent& event) {
    if(isTextInputActive() && _ui->focusedInputWidget() && _ui->focusedInputWidget()->handleTextInput(event))
        redraw();
}
#endif

}

MAGNUM_APPLICATION_MAIN(Magnum::Gallery)
