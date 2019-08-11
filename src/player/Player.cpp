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
#include <Corrade/Interconnect/Receiver.h>
#include <Corrade/PluginManager/Manager.h>
#include <Corrade/PluginManager/PluginMetadata.h>
#include <Corrade/Utility/Arguments.h>
#include <Corrade/Utility/Configuration.h>
#include <Corrade/Utility/DebugStl.h>
#include <Corrade/Utility/Directory.h>
#include <Corrade/Utility/String.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/Platform/Screen.h>
#include <Magnum/Platform/ScreenedApplication.h>
#include <Magnum/Trade/AbstractImporter.h>

#include "Magnum/Ui/Anchor.h"
#include "Magnum/Ui/Button.h"
#include "Magnum/Ui/Label.h"
#include "Magnum/Ui/Plane.h"
#include "Magnum/Ui/UserInterface.h"

#ifdef CORRADE_TARGET_EMSCRIPTEN
#include <emscripten/emscripten.h>
#include <Magnum/Text/Alignment.h>

#include "Magnum/Ui/Modal.h"
#endif

#include "AbstractPlayer.h"

namespace Magnum { namespace Player {

namespace {

constexpr const Float WidgetHeight{36.0f};
constexpr const Vector2 ButtonSize{96.0f, WidgetHeight};
#ifdef CORRADE_TARGET_EMSCRIPTEN
constexpr const Float LabelHeight{36.0f};
constexpr const Vector2 LabelSize{72.0f, LabelHeight};
#endif

#ifdef CORRADE_TARGET_EMSCRIPTEN
struct ImportErrorUiPlane: Ui::Plane, Interconnect::Receiver {
    explicit ImportErrorUiPlane(Ui::UserInterface& ui):
        Ui::Plane{ui, {{}, {440, 200}}, 1, 5, 256},
        what{*this, {Ui::Snap::Top, Range2D::fromSize(Vector2::yAxis(-15.0f), LabelSize)}, "", Text::Alignment::LineCenter, 64, Ui::Style::Danger},
        close{*this, {Ui::Snap::Bottom, ButtonSize}, "Oh well", Ui::Style::Danger}
    {
        Ui::Modal{*this, {Ui::Snap::Left|Ui::Snap::Right|Ui::Snap::Bottom|Ui::Snap::Top|Ui::Snap::NoSpaceX|Ui::Snap::NoSpaceY}, Ui::Style::Danger};

        Ui::Label{*this, {Ui::Snap::Top, Range2D::fromSize(Vector2::yAxis(-60.0f), LabelSize)}, "Try with another file or check the browser\nconsole for details. Bug reports welcome.", Text::Alignment::LineCenter, Ui::Style::Dim};
    }

    Ui::Label what;
    Ui::Button close;
};
#endif

struct OverlayUiPlane: Ui::Plane {
    explicit OverlayUiPlane(Ui::UserInterface& ui):
        Ui::Plane{ui, Ui::Snap::Top|Ui::Snap::Bottom|Ui::Snap::Left|Ui::Snap::Right, 1, 50, 640},
        controls{*this, {Ui::Snap::Top|Ui::Snap::Right, ButtonSize}, "Controls", Ui::Style::Success}
        #ifdef CORRADE_TARGET_EMSCRIPTEN
        ,
        fullsize{*this, {Ui::Snap::Bottom, controls, ButtonSize}, "Full size"},
        dropHintBackground{*this, {{}, {540, 140}}, Ui::Style::Info},
        dropHint{*this, {{}, {Vector2::yAxis(30.0f), {}}}, "Drag&drop a glTF file and everything it references here to play it.", Text::Alignment::LineCenter, Ui::Style::Info},
        disclaimer{*this, {{}, {Vector2::yAxis(-10.0f), {}}}, "All data are processed and viewed locally in your\nweb browser. Nothing is uploaded to the server.", Text::Alignment::LineCenter, Ui::Style::Dim}
        #endif
    {
        #ifdef CORRADE_TARGET_EMSCRIPTEN
        /* Hide everything on Emscripten as there is a welcome screen shown
           first */
        Ui::Widget::hide({
            controls,
            fullsize});
        #endif
    }

    Ui::Button controls;
    #ifdef CORRADE_TARGET_EMSCRIPTEN
    Ui::Button fullsize;
    Ui::Modal dropHintBackground;
    Ui::Label dropHint, disclaimer;
    #endif
};

struct Overlay: public AbstractUiScreen, public Interconnect::Receiver {
    public:
        explicit Overlay(Platform::ScreenedApplication& application);

        /* Directly accessed from Player */
        Containers::Optional<Ui::UserInterface> ui;
        Containers::Optional<OverlayUiPlane> overlayUiPlane;
        #ifdef CORRADE_TARGET_EMSCRIPTEN
        Containers::Optional<ImportErrorUiPlane> importErrorUiPlane;
        #endif

    private:
        void drawEvent() override;
        void viewportEvent(ViewportEvent& event) override;

        #ifndef CORRADE_TARGET_EMSCRIPTEN
        void keyPressEvent(KeyEvent& event) override;
        #endif
        void mousePressEvent(MouseEvent& event) override;
        void mouseReleaseEvent(MouseEvent& event) override;
        void mouseMoveEvent(MouseMoveEvent& event) override;

        void setControlsVisible(bool visible) override;

        void initializeUi();

        #ifdef CORRADE_TARGET_EMSCRIPTEN
        void toggleFullsize();

        bool _isFullsize = false;
        #endif
};

}

class Player: public Platform::ScreenedApplication, public Interconnect::Receiver {
    public:
        explicit Player(const Arguments& arguments);

        #ifdef CORRADE_TARGET_EMSCRIPTEN
        /* Need to be public to be called from C (which is called from JS) */
        void loadFile(const std::size_t totalCount, const char* filename, Containers::Array<char> data);
        #endif

        bool controlsVisible() const { return _controlsVisible; }

        /* Accessed from Overlay */
        void toggleControls();
        #ifndef CORRADE_TARGET_EMSCRIPTEN
        void reload();
        #endif

    private:
        void globalViewportEvent(ViewportEvent& size) override;
        void globalDrawEvent() override;

        PluginManager::Manager<Trade::AbstractImporter> _manager;

        /* Screens */
        Containers::Optional<Overlay> _overlay;
        Containers::Pointer<AbstractPlayer> _player;

        #ifdef CORRADE_TARGET_EMSCRIPTEN
        std::unordered_map<std::string, Containers::Array<char>> _droppedFiles;
        #endif

        std::string _importer, _file;
        bool _controlsVisible =
            #ifndef CORRADE_TARGET_EMSCRIPTEN
            true
            #else
            false
            #endif
            ;
};

bool AbstractUiScreen::controlsVisible() const {
    return application<Player>().controlsVisible();
}

Overlay::Overlay(Platform::ScreenedApplication& application): AbstractUiScreen{application, PropagatedEvent::Draw|PropagatedEvent::Input} {
    ui.emplace(Vector2(application.windowSize())/application.dpiScaling(), application.windowSize(), application.framebufferSize(), Ui::mcssDarkStyleConfiguration(), "«»");
    initializeUi();
}

void Overlay::drawEvent() {
    #ifdef MAGNUM_TARGET_WEBGL /* Another FB could be bound from the depth read */
    GL::defaultFramebuffer.bind();
    #endif

    /* Draw the UI. Disable the depth buffer and enable premultiplied alpha
       blending. */
    {
        GL::Renderer::disable(GL::Renderer::Feature::DepthTest);
        GL::Renderer::enable(GL::Renderer::Feature::Blending);
        GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::One, GL::Renderer::BlendFunction::OneMinusSourceAlpha);
        ui->draw();
        GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::One, GL::Renderer::BlendFunction::Zero);
        GL::Renderer::disable(GL::Renderer::Feature::Blending);
        GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    }
}

void Overlay::viewportEvent(ViewportEvent& event) {
    #ifdef CORRADE_TARGET_EMSCRIPTEN
    importErrorUiPlane = Containers::NullOpt;
    #endif
    overlayUiPlane = Containers::NullOpt;
    ui->relayout(Vector2(event.windowSize())/event.dpiScaling(), event.windowSize(), event.framebufferSize());
    initializeUi();

    { /** @todo this should be done only if we display some data */
        #ifdef CORRADE_TARGET_EMSCRIPTEN
        Ui::Widget::hide({
            overlayUiPlane->dropHintBackground,
            overlayUiPlane->dropHint,
            overlayUiPlane->disclaimer});
        overlayUiPlane->controls.show();
        if(_isFullsize) overlayUiPlane->fullsize.setStyle(Ui::Style::Success);
        #endif
    }
}

#ifndef CORRADE_TARGET_EMSCRIPTEN
void Overlay::keyPressEvent(KeyEvent& event) {
    if(event.key() == KeyEvent::Key::F5 && !(event.modifiers() & (KeyEvent::Modifier::Shift|KeyEvent::Modifier::Ctrl|KeyEvent::Modifier::Super|KeyEvent::Modifier::Alt|KeyEvent::Modifier::AltGr))) {
        application<Player>().reload();
    } else return;

    redraw();
    event.setAccepted();
}
#endif

void Overlay::mousePressEvent(MouseEvent& event) {
    if(ui->handlePressEvent(event.position())) {
        redraw();
        event.setAccepted();
        return;
    }
}

void Overlay::mouseReleaseEvent(MouseEvent& event) {
    if(ui->handleReleaseEvent(event.position())) {
        redraw();
        event.setAccepted();
        return;
    }
}

void Overlay::mouseMoveEvent(MouseMoveEvent& event) {
    if(ui->handleMoveEvent(event.position())) {
        redraw();
        event.setAccepted();
        return;
    }

    /** @todo ugh this will break the moving again */
}

void Overlay::initializeUi() {
    overlayUiPlane.emplace(*ui);
    #ifdef CORRADE_TARGET_EMSCRIPTEN
    importErrorUiPlane.emplace(*ui);
    #endif
    Interconnect::connect(overlayUiPlane->controls, &Ui::Button::tapped, application<Player>(), &Player::toggleControls);
    #ifdef CORRADE_TARGET_EMSCRIPTEN
    Interconnect::connect(overlayUiPlane->fullsize, &Ui::Button::tapped, *this, &Overlay::toggleFullsize);
    Interconnect::connect(importErrorUiPlane->close, &Ui::Button::tapped, *importErrorUiPlane, &Ui::Plane::hide);
    #endif
}

void Overlay::setControlsVisible(bool visible) {
    overlayUiPlane->controls.setStyle(visible ? Ui::Style::Success : Ui::Style::Flat);

    #ifdef CORRADE_TARGET_EMSCRIPTEN
    overlayUiPlane->fullsize.setVisible(visible);
    #endif
}

#ifdef CORRADE_TARGET_EMSCRIPTEN
void Overlay::toggleFullsize() {
    if(_isFullsize) {
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
        EM_ASM({Module['setFullsize'](false)});
        #pragma GCC diagnostic pop
        _isFullsize = false;
        overlayUiPlane->fullsize.setStyle(Ui::Style::Default);
    } else {
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
        EM_ASM({Module['setFullsize'](true)});
        #pragma GCC diagnostic pop
        _isFullsize = true;
        overlayUiPlane->fullsize.setStyle(Ui::Style::Success);
    }

    /* This function needs to be called instead of doing it all in JS in order
       to correctly propagate canvas size change */
    application().setContainerCssClass(_isFullsize ? "fullsize" : "");
}
#endif

#ifdef CORRADE_TARGET_EMSCRIPTEN
Player* app;
#endif

Player::Player(const Arguments& arguments): Platform::ScreenedApplication{arguments, NoCreate} {
    Utility::Arguments args;
    #ifndef CORRADE_TARGET_EMSCRIPTEN
    args.addArgument("file").setHelp("file", "file to load")
        .addOption("importer", "AnySceneImporter").setHelp("importer", "importer plugin to use");
    #endif
    args.addBooleanOption("no-merge-animations").setHelp("no-merge-animations", "don't merge glTF animations into a single clip")
        .addOption("msaa").setHelp("msaa", "MSAA level to use (if not set, defaults to 8x or 2x for HiDPI)", "N")
        .addSkippedPrefix("magnum", "engine-specific options")
        .setGlobalHelp("Displays a 3D scene file provided on command line.")
        .parse(arguments.argc, arguments.argv);

    /* Try 8x MSAA, fall back to zero samples if not possible. Enable only 2x
       MSAA if we have enough DPI. */
    {
        const Vector2 dpiScaling = this->dpiScaling({});
        Configuration conf;
        conf.setTitle("Magnum Player")
            .setWindowFlags(Configuration::WindowFlag::Resizable)
            .setSize(conf.size(), dpiScaling);
        GLConfiguration glConf;
        glConf.setSampleCount(args.value("msaa").empty() ? dpiScaling.max() < 2.0f ? 8 : 2 : args.value<Int>("msaa"));
        #ifdef MAGNUM_TARGET_WEBGL
        /* Needed to ensure the canvas depth buffer is always Depth24Stencil8,
           stencil size is 0 by default, some browser enable stencil for that
           (Chrome) and some don't (Firefox) and thus our texture format for
           blitting might not always match. */
        glConf.setDepthBufferSize(24)
            .setStencilBufferSize(8);
        #endif
        if(!tryCreate(conf, glConf))
            create(conf, glConf.setSampleCount(0));
    }

    /* Prefer TinyGltfImporter (otherwise AssimpImporter would get selected
       first) */
    _manager.setPreferredPlugins("GltfImporter", {"TinyGltfImporter"});

    /* Set up plugin defaults */
    {
        PluginManager::PluginMetadata* const metadata = _manager.metadata("TinyGltfImporter");
        if(metadata) metadata->configuration().setValue("mergeAnimationClips",
            !args.isSet("no-merge-animations"));
    }

    /* Set up the screens */
    _overlay.emplace(*this);

    #ifndef CORRADE_TARGET_EMSCRIPTEN
    _file = args.value("file");

    /* Load a scene importer plugin */
    Containers::Pointer<Trade::AbstractImporter> importer =
        _manager.loadAndInstantiate(args.value("importer"));

    Debug{} << "Opening file" << _file;

    /* Load file. If fails and this was not a custom importer, try loading it
       as an image instead */
    /** @todo redo once canOpen*() is implemented */
    if(importer && importer->openFile(_file)) {
        /* If we passed a custom importer, try to figure out if it's an image
           or a scene */
        /** @todo ugh the importer should have an API for that */
        if(args.value("importer") != "AnySceneImporter" && !importer->object3DCount() && !importer->mesh3DCount() && importer->image2DCount() == 1)
            _player = createImagePlayer(*this, *_overlay->ui);
        else
            _player = createScenePlayer(*this, *_overlay->ui);
        _player->load(_file, *importer);
        _importer = args.value("importer");
    } else if(args.value("importer") == "AnySceneImporter") {
        Debug{} << "Opening as a scene failed, trying as an image...";
        Containers::Pointer<Trade::AbstractImporter> imageImporter = _manager.loadAndInstantiate("AnyImageImporter");
        if(imageImporter && imageImporter->openFile(_file)) {
            _player = createImagePlayer(*this, *_overlay->ui);
            _player->load(_file, *imageImporter);
            _importer = "AnyImageImporter";
        } else std::exit(2);
    } else std::exit(1);
    #else
    Containers::Pointer<Trade::AbstractImporter> importer =
        _manager.loadAndInstantiate("TinyGltfImporter");
    Utility::Resource rs{"data"};
    importer->openData(rs.getRaw("artwork/default.glb"));
    _player = createScenePlayer(*this, *_overlay->ui);
    _player->load({}, *importer);
    #endif

    #ifndef CORRADE_TARGET_EMSCRIPTEN
    setSwapInterval(1);
    #endif

    #ifdef CORRADE_TARGET_EMSCRIPTEN
    app = this;
    #endif
}

#ifndef CORRADE_TARGET_EMSCRIPTEN
void Player::reload() {
    Containers::Pointer<Trade::AbstractImporter> importer =
        _manager.loadAndInstantiate(_importer);
    if(importer && importer->openFile(_file))
        _player->load(_file, *importer);
}
#endif

#ifdef CORRADE_TARGET_EMSCRIPTEN
void Player::loadFile(std::size_t totalCount, const char* filename, Containers::Array<char> data) {
    _droppedFiles.emplace(filename, std::move(data));

    /* If the error is displayed, hide it */
    /** @todo make it just _importErrorUiPlane->hide() once the bug with a
            no-op hiding is fixed */
    if(_overlay->ui->activePlane() == &*_overlay->importErrorUiPlane)
        _overlay->importErrorUiPlane->hide();

    Debug{} << "Dropped file" << _droppedFiles.size() << Debug::nospace << "/" << Debug::nospace << totalCount << filename;

    /* We don't have all files, don't do anything yet */
    if(_droppedFiles.size() != totalCount) return;

    /* We have everything, find the top-level file */
    const std::string* gltfFile = nullptr;
    for(const auto& file: _droppedFiles) {
        if(Utility::String::endsWith(file.first, ".gltf") ||
           Utility::String::endsWith(file.first, ".glb")) {
            if(gltfFile) {
                _overlay->importErrorUiPlane->what.setText("More than one glTF file dropped.");
                _overlay->importErrorUiPlane->activate();
                _droppedFiles.clear();
                redraw();
                return;
            }

            gltfFile = &file.first;
        }
    }

    if(!gltfFile) {
        _overlay->importErrorUiPlane->what.setText("No glTF file dropped.");
        _overlay->importErrorUiPlane->activate();
        _droppedFiles.clear();
        redraw();
        return;
    }

    Containers::Pointer<Trade::AbstractImporter> importer =
        _manager.loadAndInstantiate("TinyGltfImporter");
    if(!importer) std::exit(1);

    /* Make the extra files available to the importer */
    importer->setFileCallback([](const std::string& filename,
        InputFileCallbackPolicy, Player& player)
            -> Containers::Optional<Containers::ArrayView<const char>>
        {
            auto found = player._droppedFiles.find(filename);

            /* Not found: maybe it's referencing something from a subdirectory,
               try just the filename */
            if(found == player._droppedFiles.end()) {
                const std::string relative = Utility::Directory::filename(filename);
                found = player._droppedFiles.find(relative);
                if(found == player._droppedFiles.end()) return {};

                Warning{} << filename << "was not found, supplying" << relative << "instead";
            }
            return Containers::ArrayView<const char>{found->second};
        }, *this);

    Debug{} << "Loading glTF file" << *gltfFile;

    /* Load file */
    if(!importer->openFile(*gltfFile)) {
        _overlay->importErrorUiPlane->what.setText("File import failed :(");
        _overlay->importErrorUiPlane->activate();
        _droppedFiles.clear();
        redraw();
        return;
    }

    _player->load(*gltfFile, *importer);

    /* Clear all loaded files, not needed anymore */
    _droppedFiles.clear();

    Ui::Widget::hide({
        _overlay->overlayUiPlane->dropHintBackground,
        _overlay->overlayUiPlane->dropHint,
        _overlay->overlayUiPlane->disclaimer});
    _overlay->overlayUiPlane->controls.show();
    toggleControls();

    redraw();
}
#endif

void Player::globalViewportEvent(ViewportEvent& event) {
    GL::defaultFramebuffer.setViewport({{}, event.framebufferSize()});
}

void Player::globalDrawEvent() {
    swapBuffers();
}

void Player::toggleControls() {
    _controlsVisible ^= true;
    for(Platform::Screen& screen: screens())
        static_cast<AbstractUiScreen&>(screen).setControlsVisible(_controlsVisible);
}

}}

#ifdef CORRADE_TARGET_EMSCRIPTEN
extern "C" {
    EMSCRIPTEN_KEEPALIVE void loadFile(std::size_t totalCount, const char* name, char* data, std::size_t dataSize);
    void loadFile(const std::size_t totalCount, const char* name, char* data, const std::size_t dataSize) {
        /* Add the data to the storage and take ownership of it. Memory was
           allocated using malloc() on JS side, so it needs to be freed using
           free() on C++ side. */
        Magnum::Player::app->loadFile(totalCount, name, Corrade::Containers::Array<char>{
            data, dataSize, [](char* ptr, std::size_t) { free(ptr); }});
    }
}
#endif

MAGNUM_APPLICATION_MAIN(Magnum::Player::Player)
