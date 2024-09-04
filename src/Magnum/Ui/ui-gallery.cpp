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

#include <Corrade/Utility/Arguments.h>
#include <Magnum/DebugTools/FrameProfiler.h>
#include <Magnum/Math/Color.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Extensions.h>
#include <Magnum/GL/Renderer.h>
#ifdef CORRADE_TARGET_EMSCRIPTEN
#include <Magnum/Platform/EmscriptenApplication.h>
#else
#include <Magnum/Platform/Sdl2Application.h>
#endif
#include <Magnum/Text/Alignment.h>

#include "Magnum/Ui/Anchor.h"
#include "Magnum/Ui/Application.h"
#include "Magnum/Ui/Button.h"
#include "Magnum/Ui/Event.h"
#include "Magnum/Ui/Input.h"
#include "Magnum/Ui/Label.h"
#include "Magnum/Ui/NodeFlags.h"
#include "Magnum/Ui/SnapLayouter.h"
#include "Magnum/Ui/Style.h"
#include "Magnum/Ui/TextLayer.h" /** @todo remove once Input has cursor APIs */
#include "Magnum/Ui/TextProperties.h"
#include "Magnum/Ui/UserInterfaceGL.h"

namespace Magnum {

/** @page magnum-ui-gallery Magnum::Ui Gallery
@brief Showcases widgets and features provided by @ref Magnum::Ui

@m_footernavigation
@m_keywords{magnum-ui-gallery ui-gallery}

@image html ui-gallery.png width=400px

@m_div{m-button m-primary} <a href="https://magnum.graphics/showcase/magnum-ui-gallery/">@m_div{m-big}Live web version @m_enddiv @m_div{m-small} uses WebAssembly & WebGL 2 @m_enddiv </a> @m_enddiv

This app is built if both `MAGNUM_WITH_UI` and `MAGNUM_WITH_UI_GALLERY` is
enabled when building Magnum Extras. To use this app with CMake, you need to
request the `ui-gallery` component of the `MagnumExtras` package and use the
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
-   `--subdivided-quads` --- enable
    @ref Ui::BaseLayerSharedFlag::SubdividedQuads
-   `--profile` --- enable frame profiling using
    @ref DebugTools::FrameProfilerGL printed to the console
-   `--no-vsync` --- disable VSync for frame profiling
-   `--magnum-...` --- engine-specific options (see
    @ref GL-Context-usage-command-line for details)
*/

namespace {

using namespace Containers::Literals;
using namespace Math::Literals;

class UiGallery: public Platform::Application {
    public:
        explicit UiGallery(const Arguments& arguments);

    private:
        void viewportEvent(ViewportEvent& event) override;
        void drawEvent() override;
        void mousePressEvent(MouseEvent& event) override;
        void mouseReleaseEvent(MouseEvent& event) override;
        void mouseMoveEvent(MouseMoveEvent& event) override;
        void keyPressEvent(KeyEvent& event) override;
        void keyReleaseEvent(KeyEvent& event) override;
        void textInputEvent(TextInputEvent& event) override;

        Ui::UserInterfaceGL _ui{NoCreate};

        DebugTools::FrameProfilerGL _profiler;
};

constexpr const Float WidgetHeight = 36.0f;
constexpr const Float LabelHeight = 24.0f;
constexpr const Vector2 LabelSize{72.0f, LabelHeight};

UiGallery::UiGallery(const Arguments& arguments): Platform::Application{arguments, NoCreate} {
    Utility::Arguments args;
    args.addBooleanOption("subdivided-quads").setHelp("subdivided-quads", "enable BaseLayerSharedFlag::SubdividedQuads")
        .addBooleanOption("profile").setHelp("profile", "enable frame profiling printed to the console")
        #ifndef CORRADE_TARGET_EMSCRIPTEN
        .addBooleanOption("no-vsync").setHelp("no-vsync", "disable VSync for frame profiling")
        #endif
        .addSkippedPrefix("magnum", "engine-specific options")
        .parse(arguments.argc, arguments.argv);

    /* Create a GL context and the UI after the arguments were parsed to not
       have a flickering window and console noise if --help is requested,
       parsing fails, etc. */
    create(Configuration{}
        .setTitle("Magnum::Ui Gallery"_s)
        .setWindowFlags(Configuration::WindowFlag::Resizable));

    _ui.create(Vector2{windowSize()}/dpiScaling(), Vector2{windowSize()}, framebufferSize(), Ui::McssDarkStyle{});

    /* Set up the profiler, if enabled */
    if(args.isSet("profile")) {
        _profiler.setup(
            DebugTools::FrameProfilerGL::Value::FrameTime|
            DebugTools::FrameProfilerGL::Value::CpuDuration|(
                #ifndef MAGNUM_TARGET_GLES
                GL::Context::current().isExtensionSupported<GL::Extensions::ARB::timer_query>()
                #elif !defined(MAGNUM_TARGET_WEBGL)
                GL::Context::current().isExtensionSupported<GL::Extensions::EXT::disjoint_timer_query>()
                #else
                GL::Context::current().isExtensionSupported<GL::Extensions::EXT::disjoint_timer_query_webgl2>()
                #endif
                ?
                    DebugTools::FrameProfilerGL::Value::GpuDuration : DebugTools::FrameProfilerGL::Values{}),
            50);
    } else _profiler.disable(); /** @todo why this isn't a default? */

    Ui::NodeHandle root = _ui.createNode({}, _ui.size());

    {
        /* Buttons */
        Ui::NodeHandle buttons = Ui::label(
            Ui::snap(_ui, Ui::Snap::Top|Ui::Snap::Left|Ui::Snap::Inside, root, LabelSize),
            "Buttons", Text::Alignment::MiddleLeft, Ui::LabelStyle::Dim);

        Ui::SnapLayout snap{_ui,
            Ui::Snap::Bottom|Ui::Snap::Left|Ui::Snap::InsideX, buttons,
            Ui::Snap::Right};
        Ui::NodeHandle buttonDefault = Ui::button(snap({80, WidgetHeight}),
            "Default", Ui::ButtonStyle::Default);
        Ui::button(snap({80, WidgetHeight}),
            "Primary", Ui::ButtonStyle::Primary);
        Ui::button(snap({96, WidgetHeight}),
            Ui::Icon::Yes, "Success", Ui::ButtonStyle::Success);
        Ui::button(snap({96, WidgetHeight}),
            Ui::Icon::No, "Warning", Ui::ButtonStyle::Warning);
        Ui::button(snap({96, WidgetHeight}),
            Ui::Icon::No, "Danger", Ui::ButtonStyle::Danger);
        Ui::button(snap({80, WidgetHeight}),
            "Info", Ui::ButtonStyle::Info);
        Ui::button(snap({80, WidgetHeight}),
            "Dim", Ui::ButtonStyle::Dim);
        Ui::button(snap({80, WidgetHeight}),
            Ui::Icon::Yes, "Flat", Ui::ButtonStyle::Flat);

        snap = Ui::SnapLayout{_ui,
            Ui::Snap::Bottom|Ui::Snap::Left|Ui::Snap::InsideX, buttonDefault,
            Ui::Snap::Right};
        Ui::NodeHandle buttonDefaultDisabled = Ui::button(
            snap({80, WidgetHeight}, Ui::NodeFlag::Disabled),
            "Default", Ui::ButtonStyle::Default);
        Ui::button(snap({80, WidgetHeight}, Ui::NodeFlag::Disabled),
            "Primary", Ui::ButtonStyle::Primary);
        Ui::button(snap({96, WidgetHeight}, Ui::NodeFlag::Disabled),
            Ui::Icon::Yes, "Success", Ui::ButtonStyle::Success);
        Ui::button(snap({96, WidgetHeight}, Ui::NodeFlag::Disabled),
            Ui::Icon::No, "Warning", Ui::ButtonStyle::Warning);
        Ui::button(snap({96, WidgetHeight}, Ui::NodeFlag::Disabled),
            Ui::Icon::No, "Danger", Ui::ButtonStyle::Danger);
        Ui::button(snap({80, WidgetHeight}, Ui::NodeFlag::Disabled),
            "Info", Ui::ButtonStyle::Info);
        Ui::button(snap({80, WidgetHeight}, Ui::NodeFlag::Disabled),
            "Dim", Ui::ButtonStyle::Dim);
        Ui::button(snap({80, WidgetHeight}, Ui::NodeFlag::Disabled),
            Ui::Icon::Yes, "Flat", Ui::ButtonStyle::Flat);

        /* Labels */
        Ui::NodeHandle labels = Ui::label(
            Ui::snap(_ui, Ui::Snap::Bottom|Ui::Snap::Left|Ui::Snap::InsideX, buttonDefaultDisabled, {0, 8}, LabelSize),
            "Labels", Text::Alignment::MiddleLeft, Ui::LabelStyle::Dim);

        snap = Ui::SnapLayout{_ui,
            Ui::Snap::Bottom|Ui::Snap::Left|Ui::Snap::InsideX, labels,
            Ui::Snap::Right};
        Ui::NodeHandle labelDefault = Ui::label(snap(LabelSize),
            "Default", Ui::LabelStyle::Default);
        Ui::label(snap(LabelSize),
            "Primary", Ui::LabelStyle::Primary);
        Ui::label(snap(LabelSize),
            "Success", Ui::LabelStyle::Success);
        Ui::label(snap(LabelSize),
            "Warning", Ui::LabelStyle::Warning);
        Ui::label(snap(LabelSize),
            "Danger", Ui::LabelStyle::Danger);
        Ui::label(snap(LabelSize),
            "Info", Ui::LabelStyle::Info);
        Ui::label(snap(LabelSize),
            "Dim", Ui::LabelStyle::Dim);

        snap = Ui::SnapLayout{_ui,
            Ui::Snap::Bottom|Ui::Snap::Left|Ui::Snap::InsideX, labelDefault,
            Ui::Snap::Right};
        Ui::NodeHandle labelDefaultDisabled = Ui::label(
            snap(LabelSize, Ui::NodeFlag::Disabled),
            "Default", Ui::LabelStyle::Default);
        Ui::label(snap(LabelSize, Ui::NodeFlag::Disabled),
            "Primary", Ui::LabelStyle::Primary);
        Ui::label(snap(LabelSize, Ui::NodeFlag::Disabled),
            "Success", Ui::LabelStyle::Success);
        Ui::label(snap(LabelSize, Ui::NodeFlag::Disabled),
            "Warning", Ui::LabelStyle::Warning);
        Ui::label(snap(LabelSize, Ui::NodeFlag::Disabled),
            "Danger", Ui::LabelStyle::Danger);
        Ui::label(snap(LabelSize, Ui::NodeFlag::Disabled),
            "Info", Ui::LabelStyle::Info);
        Ui::label(snap(LabelSize, Ui::NodeFlag::Disabled),
            "Dim", Ui::LabelStyle::Dim);

        /* Inputs */
        Ui::NodeHandle inputs = Ui::label(
            Ui::snap(_ui, Ui::Snap::Bottom|Ui::Snap::Left|Ui::Snap::InsideX, labelDefaultDisabled, {0, 8}, LabelSize),
            "Inputs", Text::Alignment::MiddleLeft, Ui::LabelStyle::Dim);

        snap = Ui::SnapLayout{_ui,
            Ui::Snap::Bottom|Ui::Snap::Left|Ui::Snap::InsideX, inputs,
            Ui::Snap::Right};
        Ui::Input inputDefault{snap({128, WidgetHeight}),
            "Default", Ui::InputStyle::Default};
        Ui::Input inputSuccess{snap({128, WidgetHeight}),
            "Success", Ui::InputStyle::Success};
        Ui::Input inputWarning{snap({128, WidgetHeight}),
            "Warning", Ui::InputStyle::Warning};
        Ui::Input inputDanger{snap({128, WidgetHeight}),
            "Danger", Ui::InputStyle::Danger};
        Ui::Input inputFlat{snap({128, WidgetHeight}),
            "Flat", Ui::InputStyle::Flat};

        snap = Ui::SnapLayout{_ui,
            Ui::Snap::Bottom|Ui::Snap::Left|Ui::Snap::InsideX, inputDefault,
            Ui::Snap::Right};
        Ui::Input{snap({128, WidgetHeight}, Ui::NodeFlag::Disabled),
            "Default", Ui::InputStyle::Default}.release();
        Ui::Input{snap({128, WidgetHeight}, Ui::NodeFlag::Disabled),
            "Succes", Ui::InputStyle::Success}.release();
        Ui::Input{snap({128, WidgetHeight}, Ui::NodeFlag::Disabled),
            "Warning", Ui::InputStyle::Warning}.release();
        Ui::Input{snap({128, WidgetHeight}, Ui::NodeFlag::Disabled),
            "Danger", Ui::InputStyle::Danger}.release();
        Ui::Input{snap({128, WidgetHeight}, Ui::NodeFlag::Disabled),
            "Flat", Ui::InputStyle::Flat}.release();

        /** @todo provide some APIs on the Input directly */
        _ui.textLayer().setCursor(inputDefault.textData(), 7, 2);
        _ui.textLayer().setCursor(inputSuccess.textData(), 3, 6);
        _ui.textLayer().setCursor(inputWarning.textData(), 7, 0);
        _ui.textLayer().setCursor(inputDanger.textData(), 0, 3);
        _ui.textLayer().setCursor(inputFlat.textData(), 3, 1);
        inputDefault.release();
        inputSuccess.release();
        inputWarning.release();
        inputDanger.release();
        inputFlat.release();
    }

    #ifdef CORRADE_TARGET_EMSCRIPTEN
    /** @todo remove once inputs can do this on focus themselves,
        EmscriptenApplication otherwise doesn't accept any text input at all */
    startTextInput();
    #endif

    #ifndef CORRADE_TARGET_EMSCRIPTEN
    if(args.isSet("no-vsync"))
        setSwapInterval(0);
    #endif

    GL::Renderer::setClearColor(0x22272e_rgbf);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::One, GL::Renderer::BlendFunction::OneMinusSourceAlpha);
}

void UiGallery::viewportEvent(ViewportEvent& event) {
    GL::defaultFramebuffer.setViewport({{}, event.framebufferSize()});

    _ui.setSize(Vector2{windowSize()}/dpiScaling(), Vector2{windowSize()}, framebufferSize());
}

void UiGallery::drawEvent() {
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color|GL::FramebufferClear::Depth);

    _profiler.beginFrame();

    _ui.draw();

    _profiler.endFrame();

    _profiler.printStatistics(50);

    swapBuffers();
    if(_ui.state() || _profiler.isEnabled()) redraw();
}

void UiGallery::mousePressEvent(MouseEvent& event) {
    _ui.pointerPressEvent(event);

    if(_ui.state()) redraw();
}

void UiGallery::mouseReleaseEvent(MouseEvent& event) {
    _ui.pointerReleaseEvent(event);

    if(_ui.state()) redraw();
}

void UiGallery::mouseMoveEvent(MouseMoveEvent& event) {
    _ui.pointerMoveEvent(event);

    if(_ui.state()) redraw();
}

void UiGallery::keyPressEvent(KeyEvent& event) {
    _ui.keyPressEvent(event);

    if(_ui.state()) redraw();
}

void UiGallery::keyReleaseEvent(KeyEvent& event) {
    _ui.keyReleaseEvent(event);

    if(_ui.state()) redraw();
}

void UiGallery::textInputEvent(TextInputEvent& event) {
    _ui.textInputEvent(event);

    if(_ui.state()) redraw();
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::UiGallery)
