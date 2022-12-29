/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022 Vladimír Vondruš <mosra@centrum.cz>

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
#include <Corrade/Containers/StringIterable.h>
#include <Corrade/Interconnect/Receiver.h>
#include <Corrade/PluginManager/Manager.h>
#include <Corrade/PluginManager/PluginMetadata.h>
#include <Corrade/Utility/Arguments.h>
#include <Corrade/Utility/Configuration.h>
#include <Corrade/Utility/DebugStl.h>
#include <Corrade/Utility/FormatStl.h>
#include <Corrade/Utility/Path.h>
#include <Corrade/Utility/Resource.h>
#include <Corrade/Utility/String.h>
#include <Magnum/DebugTools/FrameProfiler.h>
#include <Magnum/GL/Context.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Extensions.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/Platform/Screen.h>
#include <Magnum/Platform/ScreenedApplication.h>
#include <Magnum/Trade/AbstractImporter.h>

#include "Magnum/Ui/Anchor.h"
#include "Magnum/Ui/Button.h"
#include "Magnum/Ui/Label.h"
#include "Magnum/Ui/Plane.h"
#include "Magnum/Ui/UserInterface.h"

#ifdef CORRADE_IS_DEBUG_BUILD
#include <Corrade/Utility/Tweakable.h>
#endif

#ifdef CORRADE_TARGET_EMSCRIPTEN
#include <emscripten/emscripten.h>
#include <Corrade/Containers/Pair.h>
#include <Magnum/Text/Alignment.h>

#include "Magnum/Ui/Modal.h"
#endif

#include "AbstractPlayer.h"

namespace Magnum { namespace Player {

namespace {

constexpr const Float WidgetHeight{36.0f};
constexpr const Vector2 ButtonSize{112.0f, WidgetHeight};
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
        explicit Overlay(Platform::ScreenedApplication& application, bool& drawUi);

        /* Directly accessed from Player */
        Containers::Optional<Ui::UserInterface> ui;
        Containers::Optional<OverlayUiPlane> overlayUiPlane;
        #ifdef CORRADE_TARGET_EMSCRIPTEN
        Containers::Optional<ImportErrorUiPlane> importErrorUiPlane;
        #endif

    private:
        void drawEvent() override;
        void viewportEvent(ViewportEvent& event) override;

        void keyPressEvent(KeyEvent& event) override;
        void mousePressEvent(MouseEvent& event) override;
        void mouseReleaseEvent(MouseEvent& event) override;
        void mouseMoveEvent(MouseMoveEvent& event) override;

        void setControlsVisible(bool visible) override;

        void initializeUi();

        #ifdef CORRADE_TARGET_EMSCRIPTEN
        void toggleFullsize();

        bool _isFullsize = false;
        #endif
        bool& _drawUi;
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
        #ifdef CORRADE_IS_DEBUG_BUILD
        void tickEvent() override;
        #endif

        PluginManager::Manager<Trade::AbstractImporter> _manager;

        /* Screens */
        Containers::Optional<Overlay> _overlay;
        Containers::Pointer<AbstractPlayer> _player;

        #ifdef CORRADE_TARGET_EMSCRIPTEN
        std::unordered_map<std::string, Containers::Array<char>> _droppedFiles;
        #endif

        #ifndef CORRADE_TARGET_EMSCRIPTEN
        std::string _importer, _file;
        #if defined(CORRADE_TARGET_UNIX) || (defined(CORRADE_TARGET_WINDOWS) && !defined(CORRADE_TARGET_WINDOWS_RT))
        Containers::Array<const char, Utility::Path::MapDeleter> mapped;
        #endif
        Int _id{-1};
        #endif
        bool _controlsVisible =
            #ifndef CORRADE_TARGET_EMSCRIPTEN
            true
            #else
            false
            #endif
            ;
        bool _drawUi = true;

        DebugTools::FrameProfilerGL::Values _profilerValues;
        #ifdef CORRADE_IS_DEBUG_BUILD
        Utility::Tweakable _tweakable;
        #endif
        Trade::ImporterFlags _importerFlags;
};

bool AbstractUiScreen::controlsVisible() const {
    return application<Player>().controlsVisible();
}

Overlay::Overlay(Platform::ScreenedApplication& application, bool& drawUi): AbstractUiScreen{application, PropagatedEvent::Draw|PropagatedEvent::Input}, _drawUi(drawUi) {
    ui.emplace(Vector2(application.windowSize())/application.dpiScaling(), application.windowSize(), application.framebufferSize(), Ui::mcssDarkStyleConfiguration(), "«»");
    initializeUi();
}

void Overlay::drawEvent() {
    #ifdef MAGNUM_TARGET_WEBGL /* Another FB could be bound from the depth read */
    GL::defaultFramebuffer.bind();
    #endif

    /* Draw the UI. Disable the depth buffer and enable premultiplied alpha
       blending. */
    if(_drawUi) {
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

    setControlsVisible(application<Player>().controlsVisible());
}

void Overlay::keyPressEvent(KeyEvent& event) {
    #ifndef CORRADE_TARGET_EMSCRIPTEN
    if(event.key() == KeyEvent::Key::F5 && !(event.modifiers() & (KeyEvent::Modifier::Shift|KeyEvent::Modifier::Ctrl|KeyEvent::Modifier::Super|KeyEvent::Modifier::Alt|KeyEvent::Modifier::AltGr))) {
        application<Player>().reload();
    } else
    #endif
    /* Toggle UI drawing (useful for screenshots) */
    if(event.key() == KeyEvent::Key::Esc) {
        _drawUi ^= true;
    } else return;

    redraw();
    event.setAccepted();
}

void Overlay::mousePressEvent(MouseEvent& event) {
    if(_drawUi && ui->handlePressEvent(event.position())) {
        redraw();
        event.setAccepted();
        return;
    }
}

void Overlay::mouseReleaseEvent(MouseEvent& event) {
    if(_drawUi && ui->handleReleaseEvent(event.position())) {
        redraw();
        event.setAccepted();
        return;
    }
}

void Overlay::mouseMoveEvent(MouseMoveEvent& event) {
    if(_drawUi && ui->handleMoveEvent(event.position())) {
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
    /* Can't be inside the branch because then this cursed message happens:
        Fatal: Unexpected arg0 type (select) in call to: emscripten_asm_const_int
    */
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdollar-in-identifier-extension"
    EM_ASM_({Module['setFullsize']($0)}, !_isFullsize);
    #pragma GCC diagnostic pop

    if(_isFullsize) {
        _isFullsize = false;
        overlayUiPlane->fullsize.setStyle(Ui::Style::Default);
    } else {
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
        .addOption('I', "importer", "AnySceneImporter").setHelp("importer", "importer plugin to use")
        .addOption('i', "importer-options").setHelp("importer-options", "configuration options to pass to the importer", "key=val,key2=val2,…")
        #if defined(CORRADE_TARGET_UNIX) || (defined(CORRADE_TARGET_WINDOWS) && !defined(CORRADE_TARGET_WINDOWS_RT))
        .addBooleanOption("map").setHelp("map", "memory-map the input for zero-copy import (works only for standalone files)")
        #endif
        .addOption("id").setHelp("id", "image or scene ID to import");
    #endif
    args.addBooleanOption("no-merge-animations").setHelp("no-merge-animations", "don't merge glTF animations into a single clip")
        .addOption("msaa").setHelp("msaa", "MSAA level to use (if not set, defaults to 8x or 2x for HiDPI)", "N")
        .addOption("profile", "FrameTime CpuDuration GpuDuration").setHelp("profile", "profile the rendering", "VALUES")
        #ifdef CORRADE_IS_DEBUG_BUILD
        .addBooleanOption("tweakable").setHelp("tweakable", "enable live source tweakability")
        #endif
        .addBooleanOption('v', "verbose").setHelp("verbose", "verbose output from importer plugins")
        .addSkippedPrefix("magnum", "engine-specific options")
        .setGlobalHelp(R"(Displays a 3D scene file provided on command line.

The -i / --importer-options argument accepts a comma-separated list of
key/value pairs to set in the importer plugin configuration. If the = character
is omitted, it's equivalent to saying key=true; you can specify configuration
subgroups using a slash.

The --profile option accepts a space-separated list of measured values.
Available values are FrameTime, CpuDuration, GpuDuration, VertexFetchRatio and
PrimitiveClipRatio.)")
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

    _profilerValues = args.value<DebugTools::FrameProfilerGL::Values>("profile");

    #ifdef CORRADE_IS_DEBUG_BUILD
    if(args.isSet("tweakable")) _tweakable.enable();
    #endif
    if(args.isSet("verbose")) _importerFlags |= Trade::ImporterFlag::Verbose;

    /* Setup renderer defaults */
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);

    /* For OBJ and FBX UfbxImporter is better than Magnum's builtin ObjImporter
       or Assimp */
    if(_manager.loadState("FbxImporter") != PluginManager::LoadState::NotFound)
        _manager.setPreferredPlugins("FbxImporter", {"UfbxImporter"});
    if(_manager.loadState("ObjImporter") != PluginManager::LoadState::NotFound)
        _manager.setPreferredPlugins("ObjImporter", {"UfbxImporter"});

    /* Set up plugin defaults */
    if(PluginManager::PluginMetadata* const metadata = _manager.metadata("AssimpImporter")) {
        metadata->configuration().setValue("compatibilitySkinningAttributes", false);
    }
    if(PluginManager::PluginMetadata* const metadata = _manager.metadata("GltfImporter")) {
        metadata->configuration().setValue("compatibilitySkinningAttributes", false);
        metadata->configuration().setValue("mergeAnimationClips",
            !args.isSet("no-merge-animations"));
    }

    /* Set Basis target format, but only if it wasn't forced on command line
       (which isn't possible on the web) */
    #ifndef CORRADE_TARGET_EMSCRIPTEN
    if(!Utility::String::beginsWith(args.value("importer"), "BasisImporter"))
    #endif
    {
        if(PluginManager::PluginMetadata* const metadata = _manager.metadata("BasisImporter")) {
            GL::Context& context = GL::Context::current();
            #ifdef MAGNUM_TARGET_WEBGL
            if(context.isExtensionSupported<GL::Extensions::WEBGL::compressed_texture_astc>())
            #else
            if(context.isExtensionSupported<GL::Extensions::KHR::texture_compression_astc_ldr>())
            #endif
            {
                Debug{} << "Importing Basis files as ASTC 4x4";
                metadata->configuration().setValue("format", "Astc4x4RGBA");
            }
            #ifdef MAGNUM_TARGET_GLES
            else if(context.isExtensionSupported<GL::Extensions::EXT::texture_compression_bptc>())
            #else
            else if(context.isExtensionSupported<GL::Extensions::ARB::texture_compression_bptc>())
            #endif
            {
                Debug{} << "Importing Basis files as BC7";
                metadata->configuration().setValue("format", "Bc7RGBA");
            }
            #ifdef MAGNUM_TARGET_WEBGL
            else if(context.isExtensionSupported<GL::Extensions::WEBGL::compressed_texture_s3tc>())
            #elif defined(MAGNUM_TARGET_GLES)
            else if(context.isExtensionSupported<GL::Extensions::EXT::texture_compression_s3tc>() || context.isExtensionSupported<GL::Extensions::ANGLE::texture_compression_dxt5>())
            #else
            else if(context.isExtensionSupported<GL::Extensions::EXT::texture_compression_s3tc>())
            #endif
            {
                Debug{} << "Importing Basis files as BC3";
                metadata->configuration().setValue("format", "Bc3RGBA");
            }
            #ifndef MAGNUM_TARGET_GLES2
            else
            #ifndef MAGNUM_TARGET_GLES
            if(context.isExtensionSupported<GL::Extensions::ARB::ES3_compatibility>())
            #endif
            {
                Debug{} << "Importing Basis files as ETC2";
                metadata->configuration().setValue("format", "Etc2RGBA");
            }
            #else /* For ES2, fall back to PVRTC as ETC2 is not available */
            else
            #ifdef MAGNUM_TARGET_WEBGL
            if(context.isExtensionSupported<WEBGL::compressed_texture_pvrtc>())
            #else
            if(context.isExtensionSupported<IMG::texture_compression_pvrtc>())
            #endif
            {
                Debug{} << "Importing Basis files as PVRTC 4bpp";
                metadata->configuration().setValue("format", "PvrtcRGBA4bpp");
            }
            #endif
            #if defined(MAGNUM_TARGET_GLES2) || !defined(MAGNUM_TARGET_GLES)
            else /* ES3 has ETC2 always */
            {
                Warning{} << "No supported GPU compressed texture format detected, Basis images will get imported as RGBA8";
                metadata->configuration().setValue("format", "RGBA8");
            }
            #endif
        }
    }

    /* Set up the screens */
    _overlay.emplace(*this, _drawUi);

    #ifndef CORRADE_TARGET_EMSCRIPTEN
    _file = args.value("file");

    /* Scene / image ID to load. If not specified, -1 is used. */
    if(!args.value("id").empty()) _id = args.value<Int>("id");

    /* Load a scene importer plugin */
    Containers::Pointer<Trade::AbstractImporter> importer =
        _manager.loadAndInstantiate(args.value("importer"));
    if(importer) importer->addFlags(_importerFlags);

    /* Propagate user-defined options from the command line */
    if(importer) for(const std::string& option: Utility::String::splitWithoutEmptyParts(args.value("importer-options"), ',')) {
        auto keyValue = Utility::String::partition(option, '=');
        Utility::String::trimInPlace(keyValue[0]);
        Utility::String::trimInPlace(keyValue[2]);

        std::vector<std::string> keyParts = Utility::String::split(keyValue[0], '/');
        CORRADE_INTERNAL_ASSERT(!keyParts.empty());
        Utility::ConfigurationGroup* group = &importer->configuration();
        bool groupNotRecognized = false;
        for(std::size_t i = 0; i != keyParts.size() - 1; ++i) {
            Utility::ConfigurationGroup* subgroup = group->group(keyParts[i]);
            if(!subgroup) {
                groupNotRecognized = true;
                subgroup = group->addGroup(keyParts[i]);
            }
            group = subgroup;
        }

        /* Provide a warning message in case the plugin doesn't define given
           option in its default config. The plugin is not *required* to have
           those tho (could be backward compatibility entries, for example), so
           not an error. */
        if(groupNotRecognized || !group->hasValue(keyParts.back()))
            Warning{} << "Option" << keyValue[0] << "not recognized by" << importer->plugin();

        /* If the option doesn't have an =, treat it as a boolean flag that's
           set to true. While there's no similar way to do an inverse, it's
           still nicer than causing a fatal error with those. */
        if(keyValue[1].empty())
            group->setValue(keyParts.back(), true);
        else
            group->setValue(keyParts.back(), keyValue[2]);
    }

    Debug{} << "Opening file" << _file;

    /* Load file. If fails and this was not a custom importer, try loading it
       as an image instead */
    /** @todo redo once canOpen*() is implemented */
    bool success;
    if(args.isSet("map")) {
        Containers::Optional<Containers::Array<const char, Utility::Path::MapDeleter>> maybeMapped;
        success = importer && (maybeMapped = Utility::Path::mapRead(args.value("file"))) && importer->openMemory(*maybeMapped);
        mapped = *std::move(maybeMapped);
    } else {
        success = importer && importer->openFile(_file);
    }
    if(success) {
        /* If we passed a custom importer, try to figure out if it's an image
           or a scene */
        /** @todo ugh the importer should have an API for that */
        if(args.value("importer") != "AnySceneImporter" && !importer->objectCount() && !importer->meshCount() && importer->image2DCount() >= 1)
            _player = createImagePlayer(*this, *_overlay->ui, _drawUi);
        else
            _player = createScenePlayer(*this, *_overlay->ui, _profilerValues, _drawUi);
        _player->load(_file, *importer, _id);
        _importer = args.value("importer");
    } else if(args.value("importer") == "AnySceneImporter") {
        Debug{} << "Opening as a scene failed, trying as an image...";
        Containers::Pointer<Trade::AbstractImporter> imageImporter = _manager.loadAndInstantiate("AnyImageImporter");
        if(imageImporter) imageImporter->addFlags(_importerFlags);
        if(imageImporter && imageImporter->openFile(_file)) {
            if(!imageImporter->image2DCount()) {
                Error{} << "No 2D images found in the file";
                std::exit(3);
            }
            _player = createImagePlayer(*this, *_overlay->ui, _drawUi);
            _player->load(_file, *imageImporter, _id);
            _importer = "AnyImageImporter";
        } else std::exit(2);
    } else std::exit(1);
    #else
    Containers::Pointer<Trade::AbstractImporter> importer =
        _manager.loadAndInstantiate("GltfImporter");
    importer->addFlags(_importerFlags);
    Utility::Resource rs{"data"};
    importer->openData(rs.getRaw("artwork/default.glb"));
    _player = createScenePlayer(*this, *_overlay->ui, _profilerValues, _drawUi);
    _player->load({}, *importer, -1);
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
        _player->load(_file, *importer, _id);
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

    /* There's a glTF file, load it */
    if(gltfFile) {
        Containers::Pointer<Trade::AbstractImporter> importer =
            _manager.loadAndInstantiate("GltfImporter");
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
                    const Containers::StringView relative = Utility::Path::split(filename).second();
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

        _player = createScenePlayer(*this, *_overlay->ui, _profilerValues, _drawUi);
        _player->load(*gltfFile, *importer, -1);

    /* If there's just one non-glTF file, try to load it as an image instead */
    } else if(_droppedFiles.size() == 1) {
        Containers::Pointer<Trade::AbstractImporter> imageImporter = _manager.loadAndInstantiate("AnyImageImporter");
        if(imageImporter->openData(_droppedFiles.begin()->second) && imageImporter->image2DCount()) {
            _player = createImagePlayer(*this, *_overlay->ui, _drawUi);
            _player->load(_droppedFiles.begin()->first, *imageImporter, -1);
        } else {
            _overlay->importErrorUiPlane->what.setText("No recognizable file dropped.");
            _overlay->importErrorUiPlane->activate();
            _droppedFiles.clear();
            redraw();
            return;
        }

    /* Otherwise it's doomed */
    } else {
        _overlay->importErrorUiPlane->what.setText("No recognizable file dropped.");
        _overlay->importErrorUiPlane->activate();
        _droppedFiles.clear();
        redraw();
        return;
    }

    /* Clear all loaded files, not needed anymore */
    _droppedFiles.clear();

    Ui::Widget::hide({
        _overlay->overlayUiPlane->dropHintBackground,
        _overlay->overlayUiPlane->dropHint,
        _overlay->overlayUiPlane->disclaimer});
    _overlay->overlayUiPlane->controls.show();
    _controlsVisible = false;
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

#ifdef CORRADE_IS_DEBUG_BUILD
void Player::tickEvent() {
    /* If tweakable is not enabled, call the base tick event implementation,
       which effectively stops it from being called again */
    if(!_tweakable.isEnabled()) {
        Platform::ScreenedApplication::tickEvent();
        return;
    }

    _tweakable.update();
}
#endif

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
