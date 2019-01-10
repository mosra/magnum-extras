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

#include <algorithm>
#include <Corrade/Containers/Array.h>
#include <Corrade/Interconnect/Receiver.h>
#include <Corrade/PluginManager/Manager.h>
#include <Corrade/PluginManager/PluginMetadata.h>
#include <Corrade/Utility/Arguments.h>
#include <Corrade/Utility/ConfigurationGroup.h>
#include <Corrade/Utility/Directory.h>
#include <Corrade/Utility/Format.h>
#include <Corrade/Utility/String.h>
#include <Magnum/Image.h>
#include <Magnum/Mesh.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/Animation/Player.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/PixelFormat.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/SceneGraph/TranslationRotationScalingTransformation3D.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/Shaders/Phong.h>
#include <Magnum/Text/Alignment.h>
#include <Magnum/Trade/AbstractImporter.h>
#include <Magnum/Trade/AnimationData.h>
#include <Magnum/Trade/CameraData.h>
#include <Magnum/Trade/ImageData.h>
#include <Magnum/Trade/MeshData3D.h>
#include <Magnum/Trade/MeshObjectData3D.h>
#include <Magnum/Trade/PhongMaterialData.h>
#include <Magnum/Trade/SceneData.h>
#include <Magnum/Trade/TextureData.h>

#include "Magnum/Ui/Anchor.h"
#include "Magnum/Ui/Button.h"
#include "Magnum/Ui/Label.h"
#include "Magnum/Ui/Plane.h"
#include "Magnum/Ui/UserInterface.h"

#ifdef CORRADE_TARGET_EMSCRIPTEN
#include <emscripten/emscripten.h>
#endif

#ifdef MAGNUM_TARGET_WEBGL
#include <Corrade/Utility/Resource.h>
#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/GL/Version.h>
#include <Magnum/GL/Renderbuffer.h>
#include <Magnum/GL/RenderbufferFormat.h>

#include "Magnum/Ui/Modal.h"
#endif

namespace Magnum {

namespace {

using namespace Math::Literals;

typedef SceneGraph::Object<SceneGraph::TranslationRotationScalingTransformation3D> Object3D;
typedef SceneGraph::Scene<SceneGraph::TranslationRotationScalingTransformation3D> Scene3D;

#ifdef MAGNUM_TARGET_WEBGL
class DepthReinterpretShader: public GL::AbstractShaderProgram {
    public:
        explicit DepthReinterpretShader(NoCreateT): GL::AbstractShaderProgram{NoCreate} {}
        explicit DepthReinterpretShader();

        DepthReinterpretShader& bindDepthTexture(GL::Texture2D& texture) {
            texture.bind(7);
            return *this;
        }
};

DepthReinterpretShader::DepthReinterpretShader() {
    GL::Shader vert{GL::Version::GLES300, GL::Shader::Type::Vertex};
    GL::Shader frag{GL::Version::GLES300, GL::Shader::Type::Fragment};

    Utility::Resource rs{"data"};
    vert.addSource(rs.get("DepthReinterpretShader.vert"));
    frag.addSource(rs.get("DepthReinterpretShader.frag"));

    CORRADE_INTERNAL_ASSERT_OUTPUT(GL::Shader::compile({vert, frag}));

    attachShaders({vert, frag});
    CORRADE_INTERNAL_ASSERT_OUTPUT(link());

    setUniform(uniformLocation("depthTexture"), 7);
}
#endif

constexpr const Float WidgetHeight{36.0f};
constexpr const Float LabelHeight{36.0f};
constexpr const Vector2 ButtonSize{96.0f, WidgetHeight};
constexpr const Vector2 ControlSize{56.0f, WidgetHeight};
constexpr const Vector2 HalfControlSize{28.0f, WidgetHeight};
constexpr const Vector2 LabelSize{72.0f, LabelHeight};

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

struct BaseUiPlane: Ui::Plane {
    explicit BaseUiPlane(Ui::UserInterface& ui):
        Ui::Plane{ui, Ui::Snap::Top|Ui::Snap::Bottom|Ui::Snap::Left|Ui::Snap::Right, 1, 50, 640},
        controls{*this, {Ui::Snap::Top|Ui::Snap::Right, ButtonSize}, "Controls", Ui::Style::Success},
        backward{*this, {Ui::Snap::Bottom|Ui::Snap::Left, HalfControlSize}, "«"},
        play{*this, {Ui::Snap::Right, backward, ControlSize}, "Play", Ui::Style::Success},
        pause{*this, {Ui::Snap::Right, backward, ControlSize}, "Pause", Ui::Style::Warning},
        stop{*this, {Ui::Snap::Right, play, ControlSize}, "Stop", Ui::Style::Danger},
        forward{*this, {Ui::Snap::Right, stop, HalfControlSize}, "»"},
        modelInfo{*this, {Ui::Snap::Top|Ui::Snap::Left, LabelSize}, "", Text::Alignment::LineLeft, 128, Ui::Style::Dim},
        animationProgress{*this, {Ui::Snap::Right, forward, LabelSize}, "", Text::Alignment::LineLeft, 17}
        #ifdef CORRADE_TARGET_EMSCRIPTEN
        ,
        fullsize{*this, {Ui::Snap::Bottom, controls, ButtonSize}, "Full size"},
        dropHintBackground{*this, {{}, {540, 140}}, Ui::Style::Info},
        dropHint{*this, {{}, {Vector2::yAxis(30.0f), {}}}, "Drag&drop a glTF file and everything it references here to play it.", Text::Alignment::LineCenter, Ui::Style::Info},
        disclaimer{*this, {{}, {Vector2::yAxis(-10.0f), {}}}, "All data are processed and viewed locally in your\nweb browser. Nothing is uploaded to the server.", Text::Alignment::LineCenter, Ui::Style::Dim}
        #endif
    {
        #ifdef CORRADE_TARGET_EMSCRIPTEN
        Ui::Widget::hide({
            controls,
            fullsize,
            backward,
            play,
            pause,
            stop,
            forward,
            modelInfo,
            animationProgress});
        #else
        play.hide();
        #endif
    }

    Ui::Button controls,
        backward,
        play,
        pause,
        stop,
        forward;
    Ui::Label modelInfo,
        animationProgress;
    #ifdef CORRADE_TARGET_EMSCRIPTEN
    Ui::Button fullsize;
    Ui::Modal dropHintBackground;
    Ui::Label dropHint, disclaimer;
    #endif
};

struct Data {
    Containers::Array<Containers::Optional<GL::Mesh>> meshes;
    Containers::Array<Containers::Optional<GL::Texture2D>> textures;

    Scene3D scene;
    Object3D* cameraObject{};
    SceneGraph::Camera3D* camera;
    SceneGraph::DrawableGroup3D opaqueDrawables, transparentDrawables;
    Vector3 previousPosition;

    Containers::Array<char> animationData;
    Animation::Player<std::chrono::nanoseconds, Float> player;

    Int elapsedTimeAnimationDestination = -1; /* So it gets updated with 0 as well */

    /* The UI is recreated on window resize and we need to repopulate
        the info */
    /** @todo remove once the UI has relayouting */
    std::string modelInfo;
};

}

class Player: public Platform::Application, public Interconnect::Receiver {
    public:
        explicit Player(const Arguments& arguments);

        #ifdef CORRADE_TARGET_EMSCRIPTEN
        /* Need to be public to be called from C (which is called from JS) */
        void loadFile(const std::size_t totalCount, const char* filename, Containers::Array<char> data);
        #endif

    private:
        void drawEvent() override;
        void viewportEvent(ViewportEvent& event) override;

        void keyPressEvent(KeyEvent& event) override;
        void mousePressEvent(MouseEvent& event) override;
        void mouseReleaseEvent(MouseEvent& event) override;
        void mouseMoveEvent(MouseMoveEvent& event) override;
        void mouseScrollEvent(MouseScrollEvent& event) override;

        void initializeUi();

        void toggleControls();
        void play();
        void pause();
        void stop();
        void backward();
        void forward();
        void updateAnimationTime(Int deciseconds);
        #ifdef CORRADE_TARGET_EMSCRIPTEN
        void toggleFullsize();
        #endif

        Float depthAt(const Vector2i& position);
        Vector3 unproject(const Vector2i& position, Float depth) const;

        void load(const std::string& filename, Trade::AbstractImporter& importer);
        void addObject(Trade::AbstractImporter& importer, Containers::ArrayView<Object3D*> objects, Containers::ArrayView<const Containers::Optional<Trade::PhongMaterialData>> materials, Object3D& parent, UnsignedInt i);

        /* Global rendering stuff */
        Shaders::Phong _coloredShader{NoCreate};
        Shaders::Phong _texturedShader{NoCreate};
        Shaders::Phong _texturedMaskShader{NoCreate};

        /* Data loading */
        #ifdef CORRADE_TARGET_EMSCRIPTEN
        std::unordered_map<std::string, Containers::Array<char>> _droppedFiles;
        #endif
        PluginManager::Manager<Trade::AbstractImporter> _manager;
        Containers::Optional<Data> _data;

        /* UI */
        Containers::Optional<Ui::UserInterface> _ui;
        Containers::Optional<BaseUiPlane> _baseUiPlane;
        #ifdef CORRADE_TARGET_EMSCRIPTEN
        Containers::Optional<ImportErrorUiPlane> _importErrorUiPlane;
        #endif
        bool _controlsHidden =
            #ifndef CORRADE_TARGET_EMSCRIPTEN
            false
            #else
            true
            #endif
            ;
        #ifdef CORRADE_TARGET_EMSCRIPTEN
        bool _isFullsize = false;
        #endif
        const std::pair<Float, Int> _elapsedTimeAnimationData[2] {
            {0.0f, 0},
            {1.0f, 10}
        };
        const Animation::TrackView<Float, Int> _elapsedTimeAnimation{_elapsedTimeAnimationData, Math::lerp};

        /* Mouse interaction */
        Float _lastDepth;
        Vector2i _lastPosition{-1};
        Vector3 _rotationPoint, _translationPoint;
        #ifdef MAGNUM_TARGET_WEBGL
        GL::Framebuffer _depthFramebuffer{NoCreate};
        GL::Texture2D _depth{NoCreate};
        GL::Framebuffer _reinterpretFramebuffer{NoCreate};
        GL::Renderbuffer _reinterpretDepth{NoCreate};
        GL::Mesh _fullscreenTriangle{NoCreate};
        DepthReinterpretShader _reinterpretShader{NoCreate};
        #endif
};

class ColoredDrawable: public SceneGraph::Drawable3D {
    public:
        explicit ColoredDrawable(Object3D& object, Shaders::Phong& shader, GL::Mesh& mesh, const Color4& color, SceneGraph::DrawableGroup3D& group): SceneGraph::Drawable3D{object, &group}, _shader(shader), _mesh(mesh), _color{color} {}

    private:
        void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) override;

        Shaders::Phong& _shader;
        GL::Mesh& _mesh;
        Color4 _color;
};

class TexturedDrawable: public SceneGraph::Drawable3D {
    public:
        explicit TexturedDrawable(Object3D& object, Shaders::Phong& shader, GL::Mesh& mesh, GL::Texture2D& texture, Float alphaMask, SceneGraph::DrawableGroup3D& group): SceneGraph::Drawable3D{object, &group}, _shader(shader), _mesh(mesh), _texture(texture), _alphaMask{alphaMask} {}

    private:
        void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) override;

        Shaders::Phong& _shader;
        GL::Mesh& _mesh;
        GL::Texture2D& _texture;
        Float _alphaMask;
};

#ifdef CORRADE_TARGET_EMSCRIPTEN
Player* app;
#endif

Player::Player(const Arguments& arguments): Platform::Application{arguments, NoCreate} {
    Utility::Arguments args;
    #ifndef CORRADE_TARGET_EMSCRIPTEN
    args.addArgument("file").setHelp("file", "file to load")
        .addOption("importer", "TinyGltfImporter").setHelp("importer", "importer plugin to use");
    #endif
    args.addBooleanOption("no-merge-animations").setHelp("no-merge-animations", "don't merge glTF animations into a single clip")
        .addOption("msaa").setHelp("msaa", "MSAA level to use (if not set, defaults to 8x or 2x for HiDPI)", "N")
        .addSkippedPrefix("magnum").setHelp("engine-specific options")
        .setHelp("Displays a 3D scene file provided on command line.")
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

    /* Setup renderer and shader defaults */
    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
    _coloredShader = Shaders::Phong{{}, 3};
    _coloredShader
        .setAmbientColor(0x111111_rgbf)
        .setSpecularColor(0x11111100_rgbaf)
        .setShininess(80.0f);
    _texturedShader = Shaders::Phong{Shaders::Phong::Flag::DiffuseTexture, 3};
    _texturedShader
        .setAmbientColor(0x00000000_rgbaf)
        .setSpecularColor(0x11111100_rgbaf)
        .setShininess(80.0f);
    _texturedMaskShader = Shaders::Phong{
        Shaders::Phong::Flag::DiffuseTexture|Shaders::Phong::Flag::AlphaMask, 3};
    _texturedMaskShader
        .setAmbientColor(0x00000000_rgbaf)
        .setSpecularColor(0x11111100_rgbaf)
        .setShininess(80.0f);

    /* Setup the UI */
    _ui.emplace(Vector2(windowSize())/dpiScaling(), windowSize(), framebufferSize(), Ui::mcssDarkStyleConfiguration(), "«»");
    initializeUi();

    /* Setup the depth aware mouse interaction -- on WebGL we can't just read
       depth. The only possibility to read depth is to use a depth texture and
       read it from a shader, then reinterpret as color and write to a RGBA
       texture which can finally be read back using glReadPixels(). However,
       with a depth texture we can't use multisampling so I'm instead blitting
       the depth from the default framebuffer to another framebuffer with an
       attached depth texture and then processing that texture with a custom
       shader to reinterpret the depth as RGBA values, packing 8 bit of the
       depth into each channel. That's finally read back on the client. */
    #ifdef MAGNUM_TARGET_WEBGL
    _depth = GL::Texture2D{};
    _depth.setMinificationFilter(GL::SamplerFilter::Nearest)
        .setMagnificationFilter(GL::SamplerFilter::Nearest)
        .setWrapping(GL::SamplerWrapping::ClampToEdge)
        /* The format is set to combined depth/stencil in hope it will match
           the browser depth/stencil format, requested in the GLConfiguration
           above. If it won't, the blit() won't work properly. */
        .setStorage(1, GL::TextureFormat::Depth24Stencil8, framebufferSize());
    _depthFramebuffer = GL::Framebuffer{{{}, framebufferSize()}};
    _depthFramebuffer.attachTexture(GL::Framebuffer::BufferAttachment::Depth, _depth, 0);

    _reinterpretDepth = GL::Renderbuffer{};
    _reinterpretDepth.setStorage(GL::RenderbufferFormat::RGBA8, framebufferSize());
    _reinterpretFramebuffer = GL::Framebuffer{{{}, framebufferSize()}};
    _reinterpretFramebuffer.attachRenderbuffer(GL::Framebuffer::ColorAttachment{0}, _reinterpretDepth);
    _reinterpretShader = DepthReinterpretShader{};
    _fullscreenTriangle = GL::Mesh{};
    _fullscreenTriangle.setCount(3);
    #endif

    /* Setup plugin defaults */
    {
        PluginManager::PluginMetadata* const metadata = _manager.metadata("TinyGltfImporter");
        if(metadata) metadata->configuration().setValue("mergeAnimationClips",
            !args.isSet("no-merge-animations"));
    }

    #ifndef CORRADE_TARGET_EMSCRIPTEN
    /* Load a scene importer plugin */
    std::unique_ptr<Trade::AbstractImporter> importer =
        _manager.loadAndInstantiate(args.value("importer"));
    if(!importer) std::exit(1);

    Debug{} << "Opening file" << args.value("file");

    /* Load file */
    if(!importer->openFile(args.value("file")))
        std::exit(4);

    load(args.value("file"), *importer);
    #else
    std::unique_ptr<Trade::AbstractImporter> importer =
        _manager.loadAndInstantiate("TinyGltfImporter");
    Utility::Resource rs{"data"};
    importer->openData(rs.getRaw("artwork/default.glb"));
    load({}, *importer);
    #endif

    setSwapInterval(1);
    #ifdef CORRADE_TARGET_EMSCRIPTEN
    app = this;
    #endif
}

void Player::initializeUi() {
    _baseUiPlane.emplace(*_ui);
    #ifdef CORRADE_TARGET_EMSCRIPTEN
    _importErrorUiPlane.emplace(*_ui);
    #endif
    Interconnect::connect(_baseUiPlane->controls, &Ui::Button::tapped, *this, &Player::toggleControls);
    Interconnect::connect(_baseUiPlane->play, &Ui::Button::tapped, *this, &Player::play);
    Interconnect::connect(_baseUiPlane->pause, &Ui::Button::tapped, *this, &Player::pause);
    Interconnect::connect(_baseUiPlane->stop, &Ui::Button::tapped, *this, &Player::stop);
    Interconnect::connect(_baseUiPlane->backward, &Ui::Button::tapped, *this, &Player::backward);
    Interconnect::connect(_baseUiPlane->forward, &Ui::Button::tapped, *this, &Player::forward);
    #ifdef CORRADE_TARGET_EMSCRIPTEN
    Interconnect::connect(_baseUiPlane->fullsize, &Ui::Button::tapped, *this, &Player::toggleFullsize);
    Interconnect::connect(_importErrorUiPlane->close, &Ui::Button::tapped, *_importErrorUiPlane, &Ui::Plane::hide);
    #endif
}

void Player::toggleControls() {
    if(_controlsHidden) {
        if(_data) {
            if(!_data->player.isEmpty()) {
                if(_data->player.state() == Animation::State::Playing) {
                    _baseUiPlane->play.hide();
                    _baseUiPlane->pause.show();
                } else {
                    _baseUiPlane->play.show();
                    _baseUiPlane->pause.hide();
                }
                Ui::Widget::show({
                    _baseUiPlane->backward,
                    _baseUiPlane->stop,
                    _baseUiPlane->forward,
                    _baseUiPlane->animationProgress});
            }

            Ui::Widget::show({
                #ifdef CORRADE_TARGET_EMSCRIPTEN
                _baseUiPlane->fullsize,
                #endif
                _baseUiPlane->modelInfo});
        }

        _baseUiPlane->controls.setStyle(Ui::Style::Success);
        _controlsHidden = false;

    } else {
        Ui::Widget::hide({
            _baseUiPlane->backward,
            _baseUiPlane->play,
            _baseUiPlane->pause,
            _baseUiPlane->stop,
            _baseUiPlane->forward,
            #ifdef CORRADE_TARGET_EMSCRIPTEN
            _baseUiPlane->fullsize,
            #endif
            _baseUiPlane->modelInfo,
            _baseUiPlane->animationProgress});

        _baseUiPlane->controls.setStyle(Ui::Style::Flat);
        _controlsHidden = true;
    }
}

void Player::play() {
    if(!_data) return;

    _baseUiPlane->play.hide();
    _baseUiPlane->pause.show();
    Ui::Widget::enable({
        _baseUiPlane->backward,
        _baseUiPlane->stop,
        _baseUiPlane->forward});
    _data->player.play(std::chrono::system_clock::now().time_since_epoch());
}

void Player::pause() {
    if(!_data) return;

    _baseUiPlane->play.show();
    _baseUiPlane->pause.hide();
    _data->player.pause(std::chrono::system_clock::now().time_since_epoch());
}

void Player::stop() {
    if(!_data) return;

    _data->player.stop();

    _baseUiPlane->play.show();
    _baseUiPlane->pause.hide();
    Ui::Widget::disable({
        _baseUiPlane->backward,
        _baseUiPlane->stop,
        _baseUiPlane->forward});
}

void Player::backward() {
    _data->player.seekBy(std::chrono::nanoseconds{-33333333});
}

void Player::forward() {
    _data->player.seekBy(std::chrono::nanoseconds{33333333});
}

#ifdef CORRADE_TARGET_EMSCRIPTEN
void Player::toggleFullsize() {
    if(_isFullsize) {
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
        EM_ASM({setFullsize(false)});
        #pragma GCC diagnostic pop
        _isFullsize = false;
        _baseUiPlane->fullsize.setStyle(Ui::Style::Default);
    } else {
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
        EM_ASM({setFullsize(true)});
        #pragma GCC diagnostic pop
        _isFullsize = true;
        _baseUiPlane->fullsize.setStyle(Ui::Style::Success);
    }
}
#endif

void Player::updateAnimationTime(Int deciseconds) {
    if(_baseUiPlane->animationProgress.flags() & Ui::WidgetFlag::Hidden)
        return;

    const Int duration = _data->player.duration().size()*10;
    _baseUiPlane->animationProgress.setText(Utility::formatString(
        "{:.2}:{:.2}.{:.1} / {:.2}:{:.2}.{:.1}",
        deciseconds/600, deciseconds/10%60, deciseconds%10,
        duration/600, duration/10%60, duration%10));
}

#ifdef CORRADE_TARGET_EMSCRIPTEN
void Player::loadFile(std::size_t totalCount, const char* filename, Containers::Array<char> data) {
    _droppedFiles.emplace(filename, std::move(data));

    /* If the error is displayed, hide it */
    /** @todo make it just _importErrorUiPlane->hide() once the bug with a
            no-op hiding is fixed */
    if(_ui->activePlane() == &*_importErrorUiPlane)
        _importErrorUiPlane->hide();

    Debug{} << "Dropped file" << _droppedFiles.size() << Debug::nospace << "/" << Debug::nospace << totalCount << filename;

    /* We don't have all files, don't do anything yet */
    if(_droppedFiles.size() != totalCount) return;

    /* We have everything, find the top-level file */
    const std::string* gltfFile = nullptr;
    for(const auto& file: _droppedFiles) {
        if(Utility::String::endsWith(file.first, ".gltf") ||
           Utility::String::endsWith(file.first, ".glb")) {
            if(gltfFile) {
                _importErrorUiPlane->what.setText("More than one glTF file dropped.");
                _importErrorUiPlane->activate();
                _droppedFiles.clear();
                redraw();
                return;
            }

            gltfFile = &file.first;
        }
    }

    if(!gltfFile) {
        _importErrorUiPlane->what.setText("No glTF file dropped.");
        _importErrorUiPlane->activate();
        _droppedFiles.clear();
        redraw();
        return;
    }

    std::unique_ptr<Trade::AbstractImporter> importer =
        _manager.loadAndInstantiate("TinyGltfImporter");
    if(!importer) std::exit(1);

    /* Make the extra files available to the importer */
    importer->setFileCallback([](const std::string& filename,
        Trade::ImporterFileCallbackPolicy, Player& player)
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
        _importErrorUiPlane->what.setText("File import failed :(");
        _importErrorUiPlane->activate();
        _droppedFiles.clear();
        redraw();
        return;
    }

    load(*gltfFile, *importer);

    /* Clear all loaded files, not needed anymore */
    _droppedFiles.clear();

    Ui::Widget::hide({
        _baseUiPlane->dropHintBackground,
        _baseUiPlane->dropHint,
        _baseUiPlane->disclaimer});
    _baseUiPlane->controls.show();

    redraw();
}
#endif

void Player::load(const std::string& filename, Trade::AbstractImporter& importer) {
    _data.emplace();

    /* Load all textures. Textures that fail to load will be NullOpt. */
    Debug{} << "Loading" << importer.textureCount() << "textures";
    _data->textures = Containers::Array<Containers::Optional<GL::Texture2D>>{importer.textureCount()};
    for(UnsignedInt i = 0; i != importer.textureCount(); ++i) {
        Containers::Optional<Trade::TextureData> textureData = importer.texture(i);
        if(!textureData || textureData->type() != Trade::TextureData::Type::Texture2D) {
            Warning{} << "Cannot load texture" << i << importer.textureName(i);
            continue;
        }

        Containers::Optional<Trade::ImageData2D> imageData = importer.image2D(textureData->image());
        GL::TextureFormat format;
        if(imageData && imageData->format() == PixelFormat::RGB8Unorm) {
            #ifndef MAGNUM_TARGET_GLES2
            format = GL::TextureFormat::RGB8;
            #else
            format = GL::TextureFormat::RGB;
            #endif
        } else if(imageData && imageData->format() == PixelFormat::RGBA8Unorm) {
            #ifndef MAGNUM_TARGET_GLES2
            format = GL::TextureFormat::RGBA8;
            #else
            format = GL::TextureFormat::RGBA;
            #endif
        } else {
            Warning{} << "Cannot load texture image" << textureData->image() << importer.image2DName(textureData->image());
            continue;
        }

        /* Configure the texture */
        GL::Texture2D texture;
        texture
            .setMagnificationFilter(textureData->magnificationFilter())
            .setMinificationFilter(textureData->minificationFilter(), textureData->mipmapFilter())
            .setWrapping(textureData->wrapping().xy())
            .setStorage(Math::log2(imageData->size().max()) + 1, format, imageData->size())
            .setSubImage(0, {}, *imageData)
            .generateMipmap();

        _data->textures[i] = std::move(texture);
    }

    /* Load all materials. Materials that fail to load will be NullOpt. The
       data will be stored directly in objects later, so save them only
       temporarily. */
    Debug{} << "Loading" << importer.materialCount() << "materials";
    Containers::Array<Containers::Optional<Trade::PhongMaterialData>> materials{importer.materialCount()};
    for(UnsignedInt i = 0; i != importer.materialCount(); ++i) {
        std::unique_ptr<Trade::AbstractMaterialData> materialData = importer.material(i);
        if(!materialData || materialData->type() != Trade::MaterialType::Phong) {
            Warning{} << "Cannot load material" << i << importer.materialName(i);
            continue;
        }

        materials[i] = std::move(static_cast<Trade::PhongMaterialData&>(*materialData));
    }

    /* Load all meshes. Meshes that fail to load will be NullOpt. */
    Debug{} << "Loading" << importer.mesh3DCount() << "meshes";
    _data->meshes = Containers::Array<Containers::Optional<GL::Mesh>>{importer.mesh3DCount()};
    for(UnsignedInt i = 0; i != importer.mesh3DCount(); ++i) {
        Containers::Optional<Trade::MeshData3D> meshData = importer.mesh3D(i);
        if(!meshData || meshData->primitive() != MeshPrimitive::Triangles) {
            Warning{} << "Cannot load mesh" << i << importer.mesh3DName(i);
            continue;
        }

        /** @todo do something about this? */
        if(!meshData->normalArrayCount())
            Warning{} << "The mesh doesn't have normals, might render improperly";

        /* Compile the mesh */
        _data->meshes[i] = MeshTools::compile(*meshData);
    }

    /* Load the scene. Save the object pointers in an array for easier mapping
       of animations later. */
    Debug{} << "Loading" << importer.object3DCount() << "objects";
    Containers::Array<Object3D*> objects{Containers::ValueInit, importer.object3DCount()};
    if(importer.defaultScene() != -1) {
        Debug{} << "Adding default scene" << importer.sceneName(importer.defaultScene());

        Containers::Optional<Trade::SceneData> sceneData = importer.scene(importer.defaultScene());
        if(!sceneData) {
            Error{} << "Cannot load the scene, aborting";
            return;
        }

        /* Recursively add all children */
        for(UnsignedInt objectId: sceneData->children3D())
            addObject(importer, objects, materials, _data->scene, objectId);

    /* The format has no scene support, display just the first loaded mesh with
       a default material and be done with it */
    } else if(!_data->meshes.empty() && _data->meshes[0])
        new ColoredDrawable{_data->scene, _coloredShader, *_data->meshes[0], 0xffffff_rgbf, _data->opaqueDrawables};

    /* Create a camera object in case it wasn't present in the scene already */
    if(!_data->cameraObject) {
        _data->cameraObject = new Object3D{&_data->scene};
        _data->cameraObject->translate(Vector3::zAxis(5.0f));
    }

    /* Basic camera setup */
    (*(_data->camera = new SceneGraph::Camera3D{*_data->cameraObject}))
        .setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
        .setProjectionMatrix(Matrix4::perspectiveProjection(75.0_degf, 1.0f, 0.01f, 1000.0f))
        .setViewport(GL::defaultFramebuffer.viewport().size());

    /* Use the settings with parameters of the camera in the model, if any,
       otherwise just used the hardcoded setup from above */
    if(importer.cameraCount()) {
        Containers::Optional<Trade::CameraData> camera = importer.camera(0);
        if(camera) _data->camera->setProjectionMatrix(Matrix4::perspectiveProjection(camera->fov(), 1.0f, camera->near(), camera->far()));
    }

    /* Import animations */
    if(importer.animationCount())
        Debug{} << "Importing the first animation out of" << importer.animationCount();
    for(UnsignedInt i = 0; i != importer.animationCount(); ++i) {
        Containers::Optional<Trade::AnimationData> animation = importer.animation(i);
        if(!animation) {
            Warning{} << "Cannot load animation" << i << importer.animationName(i);
            continue;
        }

        for(UnsignedInt j = 0; j != animation->trackCount(); ++j) {
            if(animation->trackTarget(j) >= objects.size() || !objects[animation->trackTarget(j)])
                continue;

            Object3D& animatedObject = *objects[animation->trackTarget(j)];

            if(animation->trackTargetType(j) == Trade::AnimationTrackTargetType::Translation3D) {
                const auto callback = [](Float, const Vector3& translation, Object3D& object) {
                    object.setTranslation(translation);
                };
                if(animation->trackType(j) == Trade::AnimationTrackType::CubicHermite3D) {
                    _data->player.addWithCallback(animation->track<CubicHermite3D>(j),
                        callback, animatedObject);
                } else {
                    CORRADE_INTERNAL_ASSERT(animation->trackType(j) == Trade::AnimationTrackType::Vector3);
                    _data->player.addWithCallback(animation->track<Vector3>(j),
                        callback, animatedObject);
                }
            } else if(animation->trackTargetType(j) == Trade::AnimationTrackTargetType::Rotation3D) {
                const auto callback = [](Float, const Quaternion& rotation, Object3D& object) {
                    object.setRotation(rotation);
                };
                if(animation->trackType(j) == Trade::AnimationTrackType::CubicHermiteQuaternion) {
                    _data->player.addWithCallback(animation->track<CubicHermiteQuaternion>(j),
                        callback, animatedObject);
                } else {
                    CORRADE_INTERNAL_ASSERT(animation->trackType(j) == Trade::AnimationTrackType::Quaternion);
                    _data->player.addWithCallback(animation->track<Quaternion>(j),
                        callback, animatedObject);
                }
            } else if(animation->trackTargetType(j) == Trade::AnimationTrackTargetType::Scaling3D) {
                const auto callback = [](Float, const Vector3& scaling, Object3D& object) {
                    object.setScaling(scaling);
                };
                if(animation->trackType(j) == Trade::AnimationTrackType::CubicHermite3D) {
                    _data->player.addWithCallback(animation->track<CubicHermite3D>(j),
                        callback, animatedObject);
                } else {
                    CORRADE_INTERNAL_ASSERT(animation->trackType(j) == Trade::AnimationTrackType::Vector3);
                    _data->player.addWithCallback(animation->track<Vector3>(j),
                        callback, animatedObject);
                }
            } else CORRADE_ASSERT_UNREACHABLE();
        }
        _data->animationData = animation->release();

        /* Load only the first animation at the moment */
        break;
    }

    /* Populate the model info */
    _baseUiPlane->modelInfo.setText(_data->modelInfo = Utility::formatString(
        "{}: {} objs, {} cams, {} meshes, {} mats, {} texs, {} anims",
        Utility::Directory::filename(filename).substr(0, 32),
        importer.object3DCount(),
        importer.cameraCount(),
        importer.mesh3DCount(),
        importer.materialCount(),
        importer.textureCount(),
        importer.animationCount()));

    if(!_data->player.isEmpty()) {
        /* Animate the elapsed time -- trigger update every 1/10th a second */
        _data->player.addWithCallbackOnChange(_elapsedTimeAnimation, [](Float, const Int& elapsed, Player& player) {
            player.updateAnimationTime(elapsed);
        }, _data->elapsedTimeAnimationDestination, *this);

        /* Start the animation */
        _data->player.play(std::chrono::system_clock::now().time_since_epoch());
    }

    /* If this is not the initial animation, make it repeat indefinitely and
       show the controls. Otherwise just play it once and without controls. */
    if(!filename.empty()) {
        _data->player.setPlayCount(0);
        _controlsHidden = true;
        toggleControls();
    }
}

void Player::addObject(Trade::AbstractImporter& importer, Containers::ArrayView<Object3D*> objects, Containers::ArrayView<const Containers::Optional<Trade::PhongMaterialData>> materials, Object3D& parent, UnsignedInt i) {
    std::unique_ptr<Trade::ObjectData3D> objectData = importer.object3D(i);
    if(!objectData) {
        Error{} << "Cannot import object" << i << importer.object3DName(i);
        return;
    }

    /* Add the object to the scene and set its transformation. If it has a
       separate TRS, use that to avoid precision issues. */
    auto* object = new Object3D{&parent};
    if(objectData->flags() & Trade::ObjectFlag3D::HasTranslationRotationScaling)
        (*object).setTranslation(objectData->translation())
                 .setRotation(objectData->rotation())
                 .setScaling(objectData->scaling());
    else object->setTransformation(objectData->transformation());

    /* Save it to the ID -> pointer mapping array for animation targets */
    objects[i] = object;

    /* Add a drawable if the object has a mesh and the mesh is loaded */
    if(objectData->instanceType() == Trade::ObjectInstanceType3D::Mesh && objectData->instance() != -1 && _data->meshes[objectData->instance()]) {
        const Int materialId = static_cast<Trade::MeshObjectData3D*>(objectData.get())->material();

        /* Material not available / not loaded, use a default material */
        if(materialId == -1 || !materials[materialId]) {
            new ColoredDrawable{*object, _coloredShader, *_data->meshes[objectData->instance()], 0xffffff_rgbf, _data->opaqueDrawables};
        } else {
            const Trade::PhongMaterialData& material = *materials[materialId];

            /* Textured material. If the texture failed to load, again just use
               a default-colored material. */
            if(material.flags() & Trade::PhongMaterialData::Flag::DiffuseTexture) {
                Containers::Optional<GL::Texture2D>& texture = _data->textures[material.diffuseTexture()];
                if(texture) new TexturedDrawable{*object,
                    material.alphaMode() == Trade::MaterialAlphaMode::Mask ?
                        _texturedMaskShader : _texturedShader,
                    *_data->meshes[objectData->instance()], *texture, material.alphaMask(),
                    material.alphaMode() == Trade::MaterialAlphaMode::Blend ?
                        _data->transparentDrawables : _data->opaqueDrawables};
                else
                    new ColoredDrawable{*object, _coloredShader, *_data->meshes[objectData->instance()], 0xffffff_rgbf, _data->opaqueDrawables};

            /* Color-only material */
            } else {
                new ColoredDrawable{*object, _coloredShader, *_data->meshes[objectData->instance()], material.diffuseColor(), _data->opaqueDrawables};
            }
        }

    /* This is a node that holds the default camera -> assign the object to the
       global camera pointer */
    } else if(objectData->instanceType() == Trade::ObjectInstanceType3D::Camera && objectData->instance() == 0) {
        _data->cameraObject = object;
    }

    /* Recursively add children */
    for(std::size_t id: objectData->children())
        addObject(importer, objects, materials, *object, id);
}

void ColoredDrawable::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) {
    _shader
        .setDiffuseColor(_color)
        .setLightPositions({
            /** @todo make this configurable, deduplicate and calculate only once */
            Vector3{10.0f, 10.0f, 10.0f}*100.0f,
            Vector3{-5.0f, -5.0f, 10.0f}*100.0f,
            Vector3{0.0f, 10.0f, -10.0f}*100.0f})
        .setLightColors({0xffffff_rgbf, 0xff9999_rgbf, 0x9999ff_rgbf})
        .setTransformationMatrix(transformationMatrix)
        .setNormalMatrix(transformationMatrix.rotationScaling())
        .setProjectionMatrix(camera.projectionMatrix());

    _mesh.draw(_shader);
}

void TexturedDrawable::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) {
    _shader
        .setLightPositions({
            /** @todo make this configurable, deduplicate and calculate only once */
            Vector3{10.0f, 10.0f, 10.0f}*100.0f,
            Vector3{-5.0f, -5.0f, 10.0f}*100.0f,
            Vector3{0.0f, 30.0f, -10.0f}*100.0f})
        .setLightColors({0xffffff_rgbf, 0xff9999_rgbf, 0x9999ff_rgbf})
        .setTransformationMatrix(transformationMatrix)
        .setNormalMatrix(transformationMatrix.rotationScaling())
        .setProjectionMatrix(camera.projectionMatrix())
        .bindDiffuseTexture(_texture);

    if(_shader.flags() & Shaders::Phong::Flag::AlphaMask)
        _shader.setAlphaMask(_alphaMask);

    _mesh.draw(_shader);
}

void Player::drawEvent() {
    #ifdef MAGNUM_TARGET_WEBGL /* Another FB could be bound from the depth read */
    GL::defaultFramebuffer.bind();
    #endif
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color|GL::FramebufferClear::Depth);

    if(_data) {
        _data->player.advance(std::chrono::system_clock::now().time_since_epoch());

        /* Draw opaque stuff as usual */
        _data->camera->draw(_data->opaqueDrawables);

        /* Draw transparent stuff back-to-front with blending enabled */
        if(!_data->transparentDrawables.isEmpty()) {
            GL::Renderer::setDepthMask(false);
            GL::Renderer::enable(GL::Renderer::Feature::Blending);
            /* Ugh non-premultiplied alpha */
            GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::SourceAlpha, GL::Renderer::BlendFunction::OneMinusSourceAlpha);

            std::vector<std::pair<std::reference_wrapper<SceneGraph::Drawable3D>, Matrix4>>
                drawableTransformations = _data->camera->drawableTransformations(_data->transparentDrawables);
            std::sort(drawableTransformations.begin(), drawableTransformations.end(),
                [](const std::pair<std::reference_wrapper<SceneGraph::Drawable3D>, Matrix4>& a,
                   const std::pair<std::reference_wrapper<SceneGraph::Drawable3D>, Matrix4>& b) {
                    return a.second.translation().z() > b.second.translation().z();
                });
            _data->camera->draw(drawableTransformations);

            GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::One, GL::Renderer::BlendFunction::Zero);
            GL::Renderer::disable(GL::Renderer::Feature::Blending);
            GL::Renderer::setDepthMask(true);
        }
    }

    /* Draw the UI. Disable the depth buffer and enable premultiplied alpha
       blending. */
    {
        GL::Renderer::disable(GL::Renderer::Feature::DepthTest);
        GL::Renderer::enable(GL::Renderer::Feature::Blending);
        GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::One, GL::Renderer::BlendFunction::OneMinusSourceAlpha);
        _ui->draw();
        GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::One, GL::Renderer::BlendFunction::Zero);
        GL::Renderer::disable(GL::Renderer::Feature::Blending);
        GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    }

    /* Schedule a redraw only if the player is playing to avoid hogging the
       CPU */
    if(_data && _data->player.state() == Animation::State::Playing) redraw();

    #ifdef MAGNUM_TARGET_WEBGL
    /* The rendered depth buffer might get lost later, so resolve it to our
       depth texture before swapping it to the canvas */
    GL::Framebuffer::blit(GL::defaultFramebuffer, _depthFramebuffer, GL::defaultFramebuffer.viewport(), GL::FramebufferBlit::Depth);
    #endif

    swapBuffers();
}

void Player::viewportEvent(ViewportEvent& event) {
    GL::defaultFramebuffer.setViewport({{}, event.framebufferSize()});

    /* Recreate the UI for the new size */
    /** @todo Slow and ugly, redo once UI has full relayouting support */
    #ifdef CORRADE_TARGET_EMSCRIPTEN
    _importErrorUiPlane = Containers::NullOpt;
    #endif
    _baseUiPlane = Containers::NullOpt;
    _ui->relayout(Vector2(event.windowSize())/event.dpiScaling(), event.windowSize(), event.framebufferSize());
    initializeUi();

    if(_data) {
        #ifdef CORRADE_TARGET_EMSCRIPTEN
        Ui::Widget::hide({
            _baseUiPlane->dropHintBackground,
            _baseUiPlane->dropHint,
            _baseUiPlane->disclaimer});
        _baseUiPlane->controls.show();
        if(_isFullsize) _baseUiPlane->fullsize.setStyle(Ui::Style::Success);
        _controlsHidden = true;
        toggleControls();
        #endif

        _data->camera->setViewport(event.framebufferSize());
        _baseUiPlane->modelInfo.setText(_data->modelInfo);
        updateAnimationTime(_data->elapsedTimeAnimationDestination);
    }

    /* Recreate depth reading textures and renderbuffers that depend on
       viewport size */
    #ifdef MAGNUM_TARGET_WEBGL
    _depth = GL::Texture2D{};
    _depth.setMinificationFilter(GL::SamplerFilter::Nearest)
        .setMagnificationFilter(GL::SamplerFilter::Nearest)
        .setWrapping(GL::SamplerWrapping::ClampToEdge)
        .setStorage(1, GL::TextureFormat::Depth24Stencil8, event.framebufferSize());
    _depthFramebuffer.attachTexture(GL::Framebuffer::BufferAttachment::Depth, _depth, 0);

    _reinterpretDepth = GL::Renderbuffer{};
    _reinterpretDepth.setStorage(GL::RenderbufferFormat::RGBA8, event.framebufferSize());
    _reinterpretFramebuffer.attachRenderbuffer(GL::Framebuffer::ColorAttachment{0}, _reinterpretDepth);

    _reinterpretFramebuffer.setViewport({{}, event.framebufferSize()});
    #endif
}

Float Player::depthAt(const Vector2i& position) {
    const Vector2i fbPosition{position.x(), GL::defaultFramebuffer.viewport().sizeY() - position.y() - 1};
    const Range2Di area = Range2Di::fromSize(fbPosition, Vector2i{1}).padded(Vector2i{2});

    /* Easy on sane platforms */
    #ifndef MAGNUM_TARGET_WEBGL
    GL::defaultFramebuffer.mapForRead(GL::DefaultFramebuffer::ReadAttachment::Front);
    Image2D image = GL::defaultFramebuffer.read(area, {GL::PixelFormat::DepthComponent, GL::PixelType::Float});

    return Math::min(Containers::arrayCast<const Float>(image.data()));

    /* On WebGL we first need to resolve the multisampled backbuffer depth to a
       texture -- that needs to be done right in the draw event otherwise the
       data might get lost -- then read that via a custom shader and manually
       pack the 24 depth bits to a RGBA8 output. It's not possible to just
       glReadPixels() the depth, we need to read a color, moreover Firefox
       doesn't allow us to read anything else than RGBA8 so we can't just use
       floatBitsToUint() and read R32UI back, we have to pack the values. */
    #else
    _reinterpretFramebuffer.clearColor(0, Vector4{})
        .bind();
    _reinterpretShader.bindDepthTexture(_depth);
    GL::Renderer::enable(GL::Renderer::Feature::ScissorTest);
    GL::Renderer::setScissor(area);
    _fullscreenTriangle.draw(_reinterpretShader);
    GL::Renderer::disable(GL::Renderer::Feature::ScissorTest);

    Image2D image = _reinterpretFramebuffer.read(area, {PixelFormat::RGBA8Unorm});

    /* Unpack the values back. Can't just use UnsignedInt as the values are
       packed as big-endian. */
    Float depth[25];
    auto packed = Containers::arrayCast<const Math::Vector4<UnsignedByte>>(image.data());
    for(std::size_t i = 0; i != packed.size(); ++i)
        depth[i] = Math::unpack<Float, UnsignedInt, 24>((packed[i].x() << 16) | (packed[i].y() << 8) | packed[i].z());

    return Math::min(depth);
    #endif
}

Vector3 Player::unproject(const Vector2i& position, Float depth) const {
    const Range2Di view = GL::defaultFramebuffer.viewport();
    const Vector2i fbPosition{position.x(), view.sizeY() - position.y() - 1};
    const Vector3 in{2*Vector2{fbPosition - view.min()}/Vector2{view.size()} - Vector2{1.0f}, depth*2.0f - 1.0f};

    return _data->camera->projectionMatrix().inverted().transformPoint(in);
}

void Player::keyPressEvent(KeyEvent& event) {
    if(!_data) return;

    /* Reset the transformation to the original view */
    if(event.key() == KeyEvent::Key::NumZero) {
        (*_data->cameraObject)
            .resetTransformation()
            .translate(Vector3::zAxis(5.0f));

    /* Axis-aligned view */
    } else if(event.key() == KeyEvent::Key::NumOne ||
              event.key() == KeyEvent::Key::NumThree ||
              event.key() == KeyEvent::Key::NumSeven)
    {
        /* Start with current camera translation with the rotation inverted */
        const Vector3 viewTranslation = _data->cameraObject->rotation().inverted().transformVector(_data->cameraObject->translation());

        /* Front/back */
        const Float multiplier = event.modifiers() & KeyEvent::Modifier::Ctrl ? -1.0f : 1.0f;

        Quaternion rotation{Math::NoInit};
        if(event.key() == KeyEvent::Key::NumSeven) /* Top/bottom */
            rotation = Quaternion::rotation(-90.0_degf*multiplier, Vector3::xAxis());
        else if(event.key() == KeyEvent::Key::NumOne) /* Front/back */
            rotation = Quaternion::rotation(90.0_degf - 90.0_degf*multiplier, Vector3::yAxis());
        else if(event.key() == KeyEvent::Key::NumThree) /* Right/left */
            rotation = Quaternion::rotation(90.0_degf*multiplier, Vector3::yAxis());
        else CORRADE_ASSERT_UNREACHABLE();

        (*_data->cameraObject)
            .setRotation(rotation)
            .setTranslation(rotation.transformVector(viewTranslation));

    /* Pause/seek the animation */
    } else if(event.key() == KeyEvent::Key::Space) {
        _data->player.state() == Animation::State::Paused ? play() : pause();
    } else if(event.key() == KeyEvent::Key::Left) {
        backward();
    } else if(event.key() == KeyEvent::Key::Right) {
        forward();

    } else return;

    redraw();
}

void Player::mousePressEvent(MouseEvent& event) {
    if(_ui->handlePressEvent(event.position())) {
        redraw();
        return;
    }

    /* Due to compatibility reasons, scroll is also reported as a press event,
       so filter that out */
    if((event.button() != MouseEvent::Button::Left &&
        event.button() != MouseEvent::Button::Middle) || !_data) return;

    _lastPosition = event.position();

    const Float currentDepth = depthAt(event.position());
    const Float depth = currentDepth == 1.0f ? _lastDepth : currentDepth;
    _translationPoint = unproject(event.position(), depth);
    /* Update the rotation point only if we're not zooming against infinite
       depth or if the original rotation point is not yet initialized */
    if(currentDepth != 1.0f || _rotationPoint.isZero()) {
        _rotationPoint = _translationPoint;
        _lastDepth = depth;
    }
}

void Player::mouseReleaseEvent(MouseEvent& event) {
    if(_ui->handleReleaseEvent(event.position())) {
        redraw();
        return;
    }
}

void Player::mouseMoveEvent(MouseMoveEvent& event) {
    if(_ui->handleMoveEvent(event.position())) {
        redraw();
        return;
    }

    if(!(event.buttons() & (MouseMoveEvent::Button::Left|
                            MouseMoveEvent::Button::Middle)) || !_data) return;

    if(_lastPosition == Vector2i{-1}) _lastPosition = event.position();
    const Vector2i delta = event.position() - _lastPosition;
    _lastPosition = event.position();


    /* Translate */
    if(event.modifiers() & MouseMoveEvent::Modifier::Shift) {
        const Vector3 p = unproject(event.position(), _lastDepth);

        _data->cameraObject->translateLocal(
            _data->cameraObject->rotation().transformVector(_translationPoint - p));

        _translationPoint = p;

    /* Rotate around rotation point */
    } else {
        const auto r =
            Quaternion::rotation(-0.01_radf*delta.y(), Vector3::xAxis())*
            Quaternion::rotation(-0.01_radf*delta.x(), Vector3::yAxis());
        (*_data->cameraObject)
            .translateLocal(_data->cameraObject->rotation()
                .transformVector(_rotationPoint + r.transformVector(-_rotationPoint)))
            .rotateLocal(r);
    }

    redraw();
}

void Player::mouseScrollEvent(MouseScrollEvent& event) {
    if(!_data || !event.offset().y()) return;

    const Float currentDepth = depthAt(event.position());
    const Float depth = currentDepth == 1.0f ? _lastDepth : currentDepth;
    const Vector3 p = unproject(event.position(), depth);
    /* Update the rotation point only if we're not zooming against infinite
       depth or if the original rotation point is not yet initialized */
    if(currentDepth != 1.0f || _rotationPoint.isZero()) {
        _rotationPoint = p;
        _lastDepth = depth;
    }

    const Int direction = event.offset().y();
    if(!direction) return;

    /* Move towards/backwards the rotation point in cam coords */
    _data->cameraObject->translateLocal(
        _data->cameraObject->rotation().transformVector(_rotationPoint*(direction < 0 ? -1.0f : 1.0f)*0.1f));

    redraw();
}

}

#ifdef CORRADE_TARGET_EMSCRIPTEN
extern "C" {
    EMSCRIPTEN_KEEPALIVE void loadFile(std::size_t totalCount, const char* name, char* data, std::size_t dataSize);
    void loadFile(const std::size_t totalCount, const char* name, char* data, const std::size_t dataSize) {
        /* Add the data to the storage and take ownership of it. Memory was
           allocated using malloc() on JS side, so it needs to be freed using
           free() on C++ side. */
        Magnum::app->loadFile(totalCount, name, Corrade::Containers::Array<char>{
            data, dataSize, [](char* ptr, std::size_t) { free(ptr); }});
    }
}
#endif

MAGNUM_APPLICATION_MAIN(Magnum::Player)
