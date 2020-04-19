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
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/Reference.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Interconnect/Receiver.h>
#include <Corrade/PluginManager/Manager.h>
#include <Corrade/Utility/Arguments.h>
#include <Corrade/Utility/ConfigurationGroup.h>
#include <Corrade/Utility/DebugStl.h>
#include <Corrade/Utility/Directory.h>
#include <Corrade/Utility/FormatStl.h>
#include <Magnum/Image.h>
#include <Magnum/Mesh.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/Animation/Player.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/PixelFormat.h>
#include <Magnum/GL/Renderbuffer.h>
#include <Magnum/GL/RenderbufferFormat.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/Math/FunctionsBatch.h>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/MeshTools/Duplicate.h>
#include <Magnum/MeshTools/GenerateIndices.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/SceneGraph/TranslationRotationScalingTransformation3D.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/Shaders/Flat.h>
#include <Magnum/Shaders/Phong.h>
#include <Magnum/Shaders/MeshVisualizer.h>
#include <Magnum/Text/Alignment.h>
#include <Magnum/Trade/AbstractImporter.h>
#include <Magnum/Trade/AnimationData.h>
#include <Magnum/Trade/CameraData.h>
#include <Magnum/Trade/ImageData.h>
#include <Magnum/Trade/MeshData.h>
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
#include <Magnum/GL/Shader.h>
#include <Magnum/GL/Version.h>
#include <Magnum/GL/Renderbuffer.h>
#include <Magnum/GL/RenderbufferFormat.h>
#include <Magnum/GL/TextureFormat.h>
#endif

#include "AbstractPlayer.h"
#include "LoadImage.h"

#ifdef CORRADE_IS_DEBUG_BUILD
#include <Corrade/Utility/Tweakable.h>
#define _ CORRADE_TWEAKABLE
#else
#define _
#endif

namespace Magnum { namespace Player {

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
constexpr const Float PaddingY{10.0f}; /* same as in mcssDarkStyleConfiguration() */
constexpr const Vector2 ButtonSize{112.0f, WidgetHeight};
constexpr const Float LabelHeight{36.0f};
constexpr const Vector2 ControlSize{56.0f, WidgetHeight};
constexpr const Vector2 HalfControlSize{28.0f, WidgetHeight};
constexpr const Vector2 LabelSize{72.0f, LabelHeight};

struct BaseUiPlane: Ui::Plane {
    explicit BaseUiPlane(Ui::UserInterface& ui):
        Ui::Plane{ui, Ui::Snap::Top|Ui::Snap::Bottom|Ui::Snap::Left|Ui::Snap::Right, 1, 50, 640},
        shadeless{*this, {Ui::Snap::Top|Ui::Snap::Right,
            Range2D::fromSize(-Vector2::yAxis(WidgetHeight + PaddingY)
                #ifdef CORRADE_TARGET_EMSCRIPTEN
                *2.0f /* on Emscripten there's the fullscreen button as well */
                #endif
            , ButtonSize)}, "Shadeless", Ui::Style::Default},
        #ifndef MAGNUM_TARGET_GLES
        tangentSpace{*this, {Ui::Snap::Bottom, shadeless, ButtonSize}, "Tangent space"},
        #endif
        backward{*this, {Ui::Snap::Bottom|Ui::Snap::Left, HalfControlSize}, "«"},
        play{*this, {Ui::Snap::Right, backward, ControlSize}, "Play", Ui::Style::Success},
        pause{*this, {Ui::Snap::Right, backward, ControlSize}, "Pause", Ui::Style::Warning},
        stop{*this, {Ui::Snap::Right, play, ControlSize}, "Stop", Ui::Style::Danger},
        forward{*this, {Ui::Snap::Right, stop, HalfControlSize}, "»"},
        modelInfo{*this, {Ui::Snap::Top|Ui::Snap::Left, LabelSize}, "", Text::Alignment::LineLeft, 128, Ui::Style::Dim},
        objectInfo{*this, {Ui::Snap::Top|Ui::Snap::Left, LabelSize}, "", Text::Alignment::LineLeft, 128, Ui::Style::Dim},
        animationProgress{*this, {Ui::Snap::Right, forward, LabelSize}, "", Text::Alignment::LineLeft, 17}
    {
        /* Implicitly hide all animation controls, they get shown if there is
           an actual animation being played */
        Ui::Widget::hide({
            backward,
            play,
            pause,
            stop,
            forward,
            animationProgress
        });

        /* Hide everything that gets shown only on selection */
        Ui::Widget::hide({
            #ifndef MAGNUM_TARGET_GLES
            tangentSpace,
            #endif
            objectInfo
        });

        #ifdef CORRADE_TARGET_EMSCRIPTEN
        /* Hide everything on Emscripten as there is a welcome screen shown
           first */
        Ui::Widget::hide({
            shadeless,
            modelInfo});
        #endif
    }

    Ui::Button shadeless;
    #ifndef MAGNUM_TARGET_GLES
    Ui::Button tangentSpace;
    #endif
    Ui::Button
        backward,
        play,
        pause,
        stop,
        forward;
    Ui::Label modelInfo,
        objectInfo,
        animationProgress;
};

struct MeshInfo {
    Containers::Optional<GL::Mesh> mesh;
    UnsignedInt attributes;
    UnsignedInt vertices;
    UnsignedInt primitives;
    std::size_t size;
    std::string name;
    bool hasSeparateBitangents;
};

struct ObjectInfo {
    Object3D* object;
    std::string name;
    UnsignedInt meshId{0xffffffffu};
};

class MeshVisualizerDrawable;

struct Data {
    Containers::Array<MeshInfo> meshes;
    Containers::Array<Containers::Optional<GL::Texture2D>> textures;

    Scene3D scene;
    Object3D* cameraObject{};
    SceneGraph::Camera3D* camera;
    SceneGraph::DrawableGroup3D opaqueDrawables, transparentDrawables, selectedObjectDrawables;
    Vector3 previousPosition;

    Containers::Array<ObjectInfo> objects;
    MeshVisualizerDrawable* selectedObject{};

    Containers::Array<char> animationData;
    Animation::Player<std::chrono::nanoseconds, Float> player;

    Int elapsedTimeAnimationDestination = -1; /* So it gets updated with 0 as well */

    /* UI is recreated on window resize and we need to repopulate the info */
    /** @todo remove once the UI has relayouting */
    std::string modelInfo, objectInfo;
};

class ScenePlayer: public AbstractPlayer, public Interconnect::Receiver {
    public:
        explicit ScenePlayer(Platform::ScreenedApplication& application, Ui::UserInterface& uiToStealFontFrom);

    private:
        void drawEvent() override;
        void viewportEvent(ViewportEvent& event) override;

        void keyPressEvent(KeyEvent& event) override;
        void mousePressEvent(MouseEvent& event) override;
        void mouseReleaseEvent(MouseEvent& event) override;
        void mouseMoveEvent(MouseMoveEvent& event) override;
        void mouseScrollEvent(MouseScrollEvent& event) override;

        void load(const std::string& filename, Trade::AbstractImporter& importer, Int id) override;
        void setControlsVisible(bool visible) override;

        void initializeUi();

        void toggleShadeless();
        #ifndef MAGNUM_TARGET_GLES
        void toggleTangentSpace();
        #endif
        void play();
        void pause();
        void stop();
        void backward();
        void forward();
        void updateAnimationTime(Int deciseconds);

        Float depthAt(const Vector2i& windowPosition);
        Vector3 unproject(const Vector2i& windowPosition, Float depth) const;

        void addObject(Trade::AbstractImporter& importer, Containers::ArrayView<const Containers::Optional<Trade::PhongMaterialData>> materials, Containers::ArrayView<const bool> hasVertexColors, Object3D& parent, UnsignedInt i);

        /* Global rendering stuff */
        Shaders::Flat3D _flatShader{Shaders::Flat3D::Flag::ObjectId};
        Shaders::Flat3D _flatVertexColorShader{Shaders::Flat3D::Flag::ObjectId|Shaders::Flat3D::Flag::VertexColor};
        Shaders::Phong _coloredShader{Shaders::Phong::Flag::ObjectId, 3};
        Shaders::Phong _texturedShader{Shaders::Phong::Flag::ObjectId|Shaders::Phong::Flag::AmbientTexture|Shaders::Phong::Flag::DiffuseTexture|Shaders::Phong::Flag::TextureTransformation, 3};
        Shaders::Phong _texturedMaskShader{
        Shaders::Phong::Flag::ObjectId|Shaders::Phong::Flag::AmbientTexture|Shaders::Phong::Flag::DiffuseTexture|Shaders::Phong::Flag::AlphaMask|Shaders::Phong::Flag::TextureTransformation, 3};
        Shaders::Phong _vertexColorShader{Shaders::Phong::Flag::ObjectId|Shaders::Phong::Flag::VertexColor, 3};
        Shaders::MeshVisualizer3D _meshVisualizerShader{
            #ifndef MAGNUM_TARGET_WEBGL
            Shaders::MeshVisualizer3D::Flag::Wireframe
            #endif
        };
        #ifndef MAGNUM_TARGET_GLES
        Shaders::MeshVisualizer3D _tangentSpaceMeshVisualizerShader{
            Shaders::MeshVisualizer3D::Flag::Wireframe|
            Shaders::MeshVisualizer3D::Flag::NormalDirection|
            Shaders::MeshVisualizer3D::Flag::TangentDirection|
            Shaders::MeshVisualizer3D::Flag::BitangentFromTangentDirection};
        Shaders::MeshVisualizer3D _tangentSpaceSeparateBitangentMeshVisualizerShader{
            Shaders::MeshVisualizer3D::Flag::Wireframe|
            Shaders::MeshVisualizer3D::Flag::NormalDirection|
            Shaders::MeshVisualizer3D::Flag::TangentDirection|
            Shaders::MeshVisualizer3D::Flag::BitangentDirection};
        #endif
        Float _brightness{0.8f};

        /* Data loading */
        Containers::Optional<Data> _data;

        bool _shadeless;
        #ifndef MAGNUM_TARGET_GLES
        bool _tangentSpace = false;
        #endif

        /* UI */
        Containers::Optional<Ui::UserInterface> _ui;
        Containers::Optional<BaseUiPlane> _baseUiPlane;
        const std::pair<Float, Int> _elapsedTimeAnimationData[2] {
            {0.0f, 0},
            {1.0f, 10}
        };
        const Animation::TrackView<const Float, const Int> _elapsedTimeAnimation{_elapsedTimeAnimationData, Math::lerp, Animation::Extrapolation::Extrapolated};

        /* Offscreen framebuffer with object ID attachment */
        GL::Renderbuffer _selectionDepth, _selectionObjectId;
        GL::Framebuffer _selectionFramebuffer{NoCreate};

        /* Mouse interaction */
        Float _lastDepth;
        Vector2i _lastPosition{-1};
        Vector3 _rotationPoint, _translationPoint;
        #ifdef MAGNUM_TARGET_WEBGL
        GL::Framebuffer _depthResolveFramebuffer{NoCreate};
        GL::Texture2D _depthResolve{NoCreate};
        GL::Framebuffer _reinterpretFramebuffer{NoCreate};
        GL::Renderbuffer _reinterpretDepth{NoCreate};
        GL::Mesh _fullscreenTriangle{NoCreate};
        DepthReinterpretShader _reinterpretShader{NoCreate};
        #endif
};

class FlatDrawable: public SceneGraph::Drawable3D {
    public:
        explicit FlatDrawable(Object3D& object, Shaders::Flat3D& shader, GL::Mesh& mesh, UnsignedInt objectId, const Color4& color, SceneGraph::DrawableGroup3D& group): SceneGraph::Drawable3D{object, &group}, _shader(shader), _mesh(mesh), _objectId{objectId}, _color{color} {}

    private:
        void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) override;

        Shaders::Flat3D& _shader;
        GL::Mesh& _mesh;
        UnsignedInt _objectId;
        Color4 _color;
};

class ColoredDrawable: public SceneGraph::Drawable3D {
    public:
        explicit ColoredDrawable(Object3D& object, Shaders::Phong& shader, GL::Mesh& mesh, UnsignedInt objectId, const Color4& color, SceneGraph::DrawableGroup3D& group): SceneGraph::Drawable3D{object, &group}, _shader(shader), _mesh(mesh), _objectId{objectId}, _color{color} {}

    private:
        void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) override;

        Shaders::Phong& _shader;
        GL::Mesh& _mesh;
        UnsignedInt _objectId;
        Color4 _color;
};

class TexturedDrawable: public SceneGraph::Drawable3D {
    public:
        explicit TexturedDrawable(Object3D& object, Shaders::Phong& shader, GL::Mesh& mesh, UnsignedInt objectId, GL::Texture2D& texture, Float alphaMask, Matrix3 textureMatrix, SceneGraph::DrawableGroup3D& group): SceneGraph::Drawable3D{object, &group}, _shader(shader), _mesh(mesh), _objectId{objectId}, _texture(texture), _alphaMask{alphaMask}, _textureMatrix{textureMatrix} {}

    private:
        void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) override;

        Shaders::Phong& _shader;
        GL::Mesh& _mesh;
        UnsignedInt _objectId;
        GL::Texture2D& _texture;
        Float _alphaMask;
        Matrix3 _textureMatrix;
};

class MeshVisualizerDrawable: public SceneGraph::Drawable3D {
    public:
        explicit MeshVisualizerDrawable(Object3D& object, Shaders::MeshVisualizer3D& shader, GL::Mesh& mesh, SceneGraph::DrawableGroup3D& group): SceneGraph::Drawable3D{object, &group}, _shader(shader), _mesh(mesh) {}

        Shaders::MeshVisualizer3D& shader() { return _shader; }

        void setShader(Shaders::MeshVisualizer3D& shader) { _shader = shader; }

    private:
        void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) override;

        Containers::Reference<Shaders::MeshVisualizer3D> _shader;
        GL::Mesh& _mesh;
};

ScenePlayer::ScenePlayer(Platform::ScreenedApplication& application, Ui::UserInterface& uiToStealFontFrom): AbstractPlayer{application, PropagatedEvent::Draw|PropagatedEvent::Input} {
    /* Setup shader defaults. Partially done later in toggleShadeless(). */
    _coloredShader.setAmbientColor(0x111111_rgbf);
    _vertexColorShader.setAmbientColor(0x00000000_rgbaf);
    for(auto* shader: {&_coloredShader, &_texturedShader, &_texturedMaskShader, &_vertexColorShader}) {
        (*shader)
            .setLightPositions({
                /** @todo make this configurable */
                Vector3{10.0f, 10.0f, 10.0f}*100.0f,
                Vector3{-5.0f, -5.0f, 10.0f}*100.0f,
                Vector3{0.0f, 10.0f, -10.0f}*100.0f})
            .setLightColors({0xffffff_rgbf*_brightness,
                             0xffcccc_rgbf*_brightness,
                             0xccccff_rgbf*_brightness})
            .setSpecularColor(0x11111100_rgbaf)
            .setShininess(80.0f);
    }

    auto setup = [](ScenePlayer& self) {
        for(auto* shader: {&self._meshVisualizerShader,
            #ifndef MAGNUM_TARGET_GLES
            &self._tangentSpaceMeshVisualizerShader
            #endif
        })
            (*shader)
                .setWireframeColor(_(0xdcdcdcff_rgbaf))
                .setColor(_(0x2f83ccff_rgbaf)*_(0.5f))
                .setViewportSize(Vector2{self.application().framebufferSize()});
        #ifndef MAGNUM_TARGET_GLES
        self._tangentSpaceMeshVisualizerShader
            .setLineLength(0.3f)
            .setLineWidth(2.0f);
        #endif
    };
    #ifdef CORRADE_IS_DEBUG_BUILD
    Utility::Tweakable::instance().scope(setup, *this);
    #else
    setup(*this);
    #endif

    /* Setup the UI, steal font etc. from the existing one to avoid having
       everything built twice */
    /** @todo this is extremely bad, there should be just one global UI (or
        not?) */
    _ui.emplace(Vector2(application.windowSize())/application.dpiScaling(), application.windowSize(), application.framebufferSize(), uiToStealFontFrom.font(), uiToStealFontFrom.glyphCache(), Ui::mcssDarkStyleConfiguration());
    initializeUi();

    /* Default to shaded */
    _shadeless = true;
    toggleShadeless();

    /* Set up offscreen rendering for object ID retrieval */
    _selectionDepth.setStorage(GL::RenderbufferFormat::DepthComponent24, application.framebufferSize());
    _selectionObjectId.setStorage(GL::RenderbufferFormat::R16UI, application.framebufferSize());
    _selectionFramebuffer = GL::Framebuffer{{{}, application.framebufferSize()}};
    _selectionFramebuffer
        .attachRenderbuffer(GL::Framebuffer::BufferAttachment::Depth, _selectionDepth)
        .attachRenderbuffer(GL::Framebuffer::ColorAttachment{1}, _selectionObjectId);
    _selectionFramebuffer.mapForDraw({
        {Shaders::Generic3D::ColorOutput, GL::Framebuffer::DrawAttachment::None},
        {Shaders::Generic3D::ObjectIdOutput, GL::Framebuffer::ColorAttachment{1}}});
    CORRADE_INTERNAL_ASSERT(_selectionFramebuffer.checkStatus(GL::FramebufferTarget::Draw) == GL::Framebuffer::Status::Complete);

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
    _depthResolve = GL::Texture2D{};
    _depthResolve.setMinificationFilter(GL::SamplerFilter::Nearest)
        .setMagnificationFilter(GL::SamplerFilter::Nearest)
        .setWrapping(GL::SamplerWrapping::ClampToEdge)
        /* The format is set to combined depth/stencil in hope it will match
           the browser depth/stencil format, requested in the GLConfiguration
           above. If it won't, the blit() won't work properly. */
        .setStorage(1, GL::TextureFormat::Depth24Stencil8, application.framebufferSize());
    _depthResolveFramebuffer = GL::Framebuffer{{{}, application.framebufferSize()}};
    _depthResolveFramebuffer.attachTexture(GL::Framebuffer::BufferAttachment::Depth, _depthResolve, 0);

    _reinterpretDepth = GL::Renderbuffer{};
    _reinterpretDepth.setStorage(GL::RenderbufferFormat::RGBA8, application.framebufferSize());
    _reinterpretFramebuffer = GL::Framebuffer{{{}, application.framebufferSize()}};
    _reinterpretFramebuffer.attachRenderbuffer(GL::Framebuffer::ColorAttachment{0}, _reinterpretDepth);
    _reinterpretShader = DepthReinterpretShader{};
    _fullscreenTriangle = GL::Mesh{};
    _fullscreenTriangle.setCount(3);
    #endif
}

void ScenePlayer::initializeUi() {
    _baseUiPlane.emplace(*_ui);

    if(_shadeless) _baseUiPlane->shadeless.setStyle(Ui::Style::Success);
    #ifndef MAGNUM_TARGET_GLES
    if(_tangentSpace) _baseUiPlane->tangentSpace.setStyle(Ui::Style::Success);
    #endif

    Interconnect::connect(_baseUiPlane->shadeless, &Ui::Button::tapped, *this, &ScenePlayer::toggleShadeless);
    #ifndef MAGNUM_TARGET_GLES
    Interconnect::connect(_baseUiPlane->tangentSpace, &Ui::Button::tapped, *this, &ScenePlayer::toggleTangentSpace);
    #endif
    Interconnect::connect(_baseUiPlane->play, &Ui::Button::tapped, *this, &ScenePlayer::play);
    Interconnect::connect(_baseUiPlane->pause, &Ui::Button::tapped, *this, &ScenePlayer::pause);
    Interconnect::connect(_baseUiPlane->stop, &Ui::Button::tapped, *this, &ScenePlayer::stop);
    Interconnect::connect(_baseUiPlane->backward, &Ui::Button::tapped, *this, &ScenePlayer::backward);
    Interconnect::connect(_baseUiPlane->forward, &Ui::Button::tapped, *this, &ScenePlayer::forward);
}

void ScenePlayer::setControlsVisible(bool visible) {
    if(visible) {
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

            _baseUiPlane->shadeless.show();

            if(_data->selectedObject) Ui::Widget::show({
                #ifndef MAGNUM_TARGET_GLES
                _baseUiPlane->tangentSpace,
                #endif
                _baseUiPlane->objectInfo});
            else _baseUiPlane->modelInfo.show();
        }

    } else {
        Ui::Widget::hide({
            _baseUiPlane->shadeless,
            _baseUiPlane->backward,
            _baseUiPlane->play,
            _baseUiPlane->pause,
            _baseUiPlane->stop,
            _baseUiPlane->forward,
            _baseUiPlane->modelInfo,
            #ifndef MAGNUM_TARGET_GLES
            _baseUiPlane->tangentSpace,
            #endif
            _baseUiPlane->objectInfo,
            _baseUiPlane->animationProgress});
    }
}

void ScenePlayer::toggleShadeless() {
    /* For shadeless, render the ambient texture at full and diffuse not at
       all so the lighting doesn't affect it */
    if((_shadeless ^= true)) {
        _texturedShader.setAmbientColor(0xffffff00_rgbaf);
        _texturedMaskShader.setAmbientColor(0xffffff00_rgbaf);
        _texturedShader.setDiffuseColor(0x00000000_rgbaf);
        _texturedMaskShader.setDiffuseColor(0x00000000_rgbaf);
        _baseUiPlane->shadeless.setStyle(Ui::Style::Success);
    } else {
        _texturedShader.setAmbientColor(0x11111100_rgbaf);
        _texturedMaskShader.setAmbientColor(0x11111100_rgbaf);
        _texturedShader.setDiffuseColor(0xffffffff_rgbaf);
        _texturedMaskShader.setDiffuseColor(0xffffffff_rgbaf);
        _baseUiPlane->shadeless.setStyle(Ui::Style::Default);
    }
}

#ifndef MAGNUM_TARGET_GLES
void ScenePlayer::toggleTangentSpace() {
    CORRADE_INTERNAL_ASSERT(_data->selectedObject);

    if(_tangentSpace ^= true) {
        _data->selectedObject->setShader(_tangentSpaceMeshVisualizerShader);
        _baseUiPlane->tangentSpace.setStyle(Ui::Style::Success);
    } else {
        _data->selectedObject->setShader(_meshVisualizerShader);
        _baseUiPlane->tangentSpace.setStyle(Ui::Style::Default);
    }
}
#endif

void ScenePlayer::play() {
    if(!_data) return;

    _baseUiPlane->play.hide();
    _baseUiPlane->pause.show();
    Ui::Widget::enable({
        _baseUiPlane->backward,
        _baseUiPlane->stop,
        _baseUiPlane->forward});
    _data->player.play(std::chrono::system_clock::now().time_since_epoch());
}

void ScenePlayer::pause() {
    if(!_data) return;

    _baseUiPlane->play.show();
    _baseUiPlane->pause.hide();
    _data->player.pause(std::chrono::system_clock::now().time_since_epoch());
}

void ScenePlayer::stop() {
    if(!_data) return;

    _data->player.stop();

    _baseUiPlane->play.show();
    _baseUiPlane->pause.hide();
    Ui::Widget::disable({
        _baseUiPlane->backward,
        _baseUiPlane->stop,
        _baseUiPlane->forward});
}

void ScenePlayer::backward() {
    _data->player.seekBy(std::chrono::nanoseconds{-33333333});
}

void ScenePlayer::forward() {
    _data->player.seekBy(std::chrono::nanoseconds{33333333});
}

void ScenePlayer::updateAnimationTime(Int deciseconds) {
    if(_baseUiPlane->animationProgress.flags() & Ui::WidgetFlag::Hidden)
        return;

    const Int duration = _data->player.duration().size()*10;
    _baseUiPlane->animationProgress.setText(Utility::formatString(
        "{:.2}:{:.2}.{:.1} / {:.2}:{:.2}.{:.1}",
        deciseconds/600, deciseconds/10%60, deciseconds%10,
        duration/600, duration/10%60, duration%10));
}

void ScenePlayer::load(const std::string& filename, Trade::AbstractImporter& importer, Int id) {
    if(id >= 0 && UnsignedInt(id) >= importer.sceneCount()) {
        Fatal{} << "Cannot load a scene with ID" << id << "as there's only" << importer.sceneCount() << "scenes";
    }

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

        /* Configure the texture */
        GL::Texture2D texture;
        texture
            .setMagnificationFilter(textureData->magnificationFilter())
            .setMinificationFilter(textureData->minificationFilter(), textureData->mipmapFilter())
            .setWrapping(textureData->wrapping().xy());

        loadImage(texture, *imageData);

        _data->textures[i] = std::move(texture);
    }

    /* Load all materials. Materials that fail to load will be NullOpt. The
       data will be stored directly in objects later, so save them only
       temporarily. */
    Debug{} << "Loading" << importer.materialCount() << "materials";
    Containers::Array<Containers::Optional<Trade::PhongMaterialData>> materials{importer.materialCount()};
    for(UnsignedInt i = 0; i != importer.materialCount(); ++i) {
        Containers::Pointer<Trade::AbstractMaterialData> materialData = importer.material(i);
        if(!materialData || materialData->type() != Trade::MaterialType::Phong) {
            Warning{} << "Cannot load material" << i << importer.materialName(i);
            continue;
        }

        materials[i] = std::move(static_cast<Trade::PhongMaterialData&>(*materialData));
    }

    /* Load all meshes. Meshes that fail to load will be NullOpt. Remember
       which have vertex colors, so in case there's no material we can use that
       instead. */
    Debug{} << "Loading" << importer.meshCount() << "meshes";
    _data->meshes = Containers::Array<MeshInfo>{importer.meshCount()};
    Containers::Array<bool> hasVertexColors{Containers::DirectInit, importer.meshCount(), false};
    for(UnsignedInt i = 0; i != importer.meshCount(); ++i) {
        Containers::Optional<Trade::MeshData> meshData = importer.mesh(i);
        if(!meshData) {
            Warning{} << "Cannot load mesh" << i << importer.meshName(i);
            continue;
        }

        std::string meshName = importer.meshName(i);
        if(meshName.empty()) meshName = Utility::formatString("{}", i);

        /* Disable warnings on custom attributes, as we printed them with
           actual string names below. Generate normals for triangle meshes
           (and don't do anything for line/point meshes, there it makes no
           sense). */
        MeshTools::CompileFlags flags = MeshTools::CompileFlag::NoWarnOnCustomAttributes;
        if((meshData->primitive() == MeshPrimitive::Triangles ||
            meshData->primitive() == MeshPrimitive::TriangleStrip ||
            meshData->primitive() == MeshPrimitive::TriangleFan) &&
           !meshData->attributeCount(Trade::MeshAttribute::Normal) &&
            meshData->attributeFormat(Trade::MeshAttribute::Position) == VertexFormat::Vector3) {

            /* If the mesh is a triangle strip/fan, convert to an indexed one
               first. The tool additionally expects the mesh to be non-indexed,
               so duplicate if necessary. Generating smooth normals for those
               will most probably cause weird artifacts, so  */
            if(meshData->primitive() == MeshPrimitive::TriangleStrip ||
               meshData->primitive() == MeshPrimitive::TriangleFan) {
                Debug{} << "Mesh" << meshName << "doesn't have normals, generating flat ones for a" << meshData->primitive();
                if(meshData->isIndexed())
                    meshData = MeshTools::duplicate(*std::move(meshData));
                meshData = MeshTools::generateIndices(*std::move(meshData));
                flags |= MeshTools::CompileFlag::GenerateFlatNormals;

            /* Otherwise prefer smooth normals, if we have an index buffer
               telling us neighboring faces */
            } else if(meshData->isIndexed()) {
                Debug{} << "Mesh" << meshName << "doesn't have normals, generating smooth ones using information from the index buffer";
                flags |= MeshTools::CompileFlag::GenerateSmoothNormals;
            } else {
                Debug{} << "Mesh" << meshName << "doesn't have normals, generating flat ones";
                flags |= MeshTools::CompileFlag::GenerateFlatNormals;
            }
        }

        /* Print messages about ignored attributes / levels */
        for(UnsignedInt i = 0; i != meshData->attributeCount(); ++i) {
            const Trade::MeshAttribute name = meshData->attributeName(i);
            if(Trade::isMeshAttributeCustom(name)) {
                const std::string stringName = importer.meshAttributeName(name);
                if(!stringName.empty())
                    Warning{} << "Mesh" << meshName << "has a custom mesh attribute" << stringName << Debug::nospace << ", ignoring";
                else
                    Warning{} << "Mesh" << meshName << "has a custom mesh attribute" << name << Debug::nospace << ", ignoring";
                continue;
            }

            const VertexFormat format = meshData->attributeFormat(i);
            if(isVertexFormatImplementationSpecific(format))
                Warning{} << "Mesh" << meshName << "has" << name << "of format" << format << Debug::nospace << ", ignoring";
        }
        const UnsignedInt meshLevels = importer.meshLevelCount(i);
        if(meshLevels > 1)
            Warning{} << "Mesh" << meshName << "has" << meshLevels - 1 << "additional mesh levels, ignoring";

        hasVertexColors[i] = meshData->hasAttribute(Trade::MeshAttribute::Color);

        /* Save metadata, compile the mesh */
        _data->meshes[i].attributes = meshData->attributeCount();
        _data->meshes[i].vertices = meshData->vertexCount();
        _data->meshes[i].size = meshData->vertexData().size();
        if(meshData->isIndexed()) {
            _data->meshes[i].primitives = MeshTools::primitiveCount(meshData->primitive(), meshData->indexCount());
            _data->meshes[i].size += meshData->indexData().size();
        } else _data->meshes[i].primitives = MeshTools::primitiveCount(meshData->primitive(), meshData->vertexCount());
        /* Needed to decide how to visualize tangent space */
        _data->meshes[i].hasSeparateBitangents = meshData->hasAttribute(Trade::MeshAttribute::Bitangent);
        _data->meshes[i].mesh = MeshTools::compile(*meshData, flags);
        _data->meshes[i].name = importer.meshName(i);
        if(_data->meshes[i].name.empty())
            _data->meshes[i].name = Utility::formatString("mesh #{}", i);
    }

    /* Load the scene. Save the object pointers in an array for easier mapping
       of animations later. */
    Debug{} << "Loading" << importer.object3DCount() << "objects";
    if((id < 0 && importer.defaultScene() != -1) || id >= 0) {
        if(id < 0) id = importer.defaultScene();
        Debug{} << "Loading scene" << id << importer.sceneName(id);

        Containers::Optional<Trade::SceneData> sceneData = importer.scene(id);
        if(!sceneData) {
            Error{} << "Cannot load the scene, aborting";
            return;
        }

        /* Recursively add all children */
        _data->objects = Containers::Array<ObjectInfo>{Containers::ValueInit, importer.object3DCount()};
        for(UnsignedInt objectId: sceneData->children3D())
            addObject(importer, materials, hasVertexColors, _data->scene, objectId);

    /* The format has no scene support, display just the first loaded mesh with
       a default material and be done with it */
    } else if(!_data->meshes.empty() && _data->meshes[0].mesh) {
        _data->objects = Containers::Array<ObjectInfo>{Containers::ValueInit, 1};
        _data->objects[0].object = &_data->scene;
        _data->objects[0].meshId = 0;
        _data->objects[0].name = "object #0";
        new ColoredDrawable{_data->scene, hasVertexColors[0] ? _vertexColorShader : _coloredShader, *_data->meshes[0].mesh, 0, 0xffffff_rgbf, _data->opaqueDrawables};
    }

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
            if(animation->trackTarget(j) >= _data->objects.size() || !_data->objects[animation->trackTarget(j)].object)
                continue;

            Object3D& animatedObject = *_data->objects[animation->trackTarget(j)].object;

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
        "{}: {} objs, {} cams, {} meshes, {} mats, {}/{} texs, {} anims",
        Utility::Directory::filename(filename).substr(0, 32),
        importer.object3DCount(),
        importer.cameraCount(),
        importer.meshCount(),
        importer.materialCount(),
        importer.textureCount(),
        importer.image2DCount(),
        importer.animationCount()));

    if(!_data->player.isEmpty()) {
        /* Animate the elapsed time -- trigger update every 1/10th a second */
        _data->player.addWithCallbackOnChange(_elapsedTimeAnimation, [](Float, const Int& elapsed, ScenePlayer& player) {
            player.updateAnimationTime(elapsed);
        }, _data->elapsedTimeAnimationDestination, *this);

        /* Start the animation */
        _data->player.play(std::chrono::system_clock::now().time_since_epoch());
    }

    /* If this is not the initial animation, make it repeat indefinitely and
       show the controls. Otherwise just play it once and without controls. */
    if(!filename.empty()) {
        _data->player.setPlayCount(0);
        setControlsVisible(true);
    }
}

void ScenePlayer::addObject(Trade::AbstractImporter& importer, Containers::ArrayView<const Containers::Optional<Trade::PhongMaterialData>> materials, Containers::ArrayView<const bool> hasVertexColors, Object3D& parent, UnsignedInt i) {
    Containers::Pointer<Trade::ObjectData3D> objectData = importer.object3D(i);
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

    /* Save it to the ID -> pointer mapping array for animation target and
       object selection */
    _data->objects[i].object = object;

    /* Add a drawable if the object has a mesh and the mesh is loaded */
    if(objectData->instanceType() == Trade::ObjectInstanceType3D::Mesh && objectData->instance() != -1 && _data->meshes[objectData->instance()].mesh) {
        const Int materialId = static_cast<Trade::MeshObjectData3D*>(objectData.get())->material();

        /* Save the mesh pointer as well, so we know what to draw for object
           selection */
        _data->objects[i].meshId = objectData->instance();
        _data->objects[i].name = importer.object3DName(i);
        if(_data->objects[i].name.empty())
            _data->objects[i].name = Utility::formatString("object #{}", i);

        GL::Mesh& mesh = *_data->meshes[objectData->instance()].mesh;

        /* Material not available / not loaded. If the mesh has vertex colors,
           use that, otherwise apply a default material; use a flat shader for
           lines / points */
        if(materialId == -1 || !materials[materialId]) {
            if(mesh.primitive() == GL::MeshPrimitive::Triangles ||
               mesh.primitive() == GL::MeshPrimitive::TriangleStrip ||
               mesh.primitive() == GL::MeshPrimitive::TriangleFan)
                new ColoredDrawable{*object, hasVertexColors[objectData->instance()] ? _vertexColorShader : _coloredShader, mesh, i, 0xffffff_rgbf, _data->opaqueDrawables};
            else
                new FlatDrawable{*object, hasVertexColors[objectData->instance()] ? _flatVertexColorShader : _flatShader, mesh, i, 0xffffff_rgbf, _data->opaqueDrawables};

        /* Material available */
        } else {
            const Trade::PhongMaterialData& material = *materials[materialId];

            /* Textured material. If the texture failed to load, again just use
               a default-colored material. */
            if(material.flags() & Trade::PhongMaterialData::Flag::DiffuseTexture) {
                Containers::Optional<GL::Texture2D>& texture = _data->textures[material.diffuseTexture()];
                if(texture) new TexturedDrawable{*object,
                    material.alphaMode() == Trade::MaterialAlphaMode::Mask ?
                        _texturedMaskShader : _texturedShader,
                    mesh, i, *texture, material.alphaMask(), material.textureMatrix(),
                    material.alphaMode() == Trade::MaterialAlphaMode::Blend ?
                        _data->transparentDrawables : _data->opaqueDrawables};
                else
                    new ColoredDrawable{*object, _coloredShader, mesh, i, 0xffffff_rgbf, _data->opaqueDrawables};

            /* Vertex color / color-only material */
            } else {
                new ColoredDrawable{*object, hasVertexColors[objectData->instance()] ? _vertexColorShader : _coloredShader, mesh, i, material.diffuseColor(), _data->opaqueDrawables};
            }
        }

    /* This is a node that holds the default camera -> assign the object to the
       global camera pointer */
    } else if(objectData->instanceType() == Trade::ObjectInstanceType3D::Camera && objectData->instance() == 0) {
        _data->cameraObject = object;
    }

    /* Recursively add children */
    for(std::size_t id: objectData->children())
        addObject(importer, materials, hasVertexColors, *object, id);
}

void FlatDrawable::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) {
    _shader
        .setColor(_color)
        .setTransformationProjectionMatrix(camera.projectionMatrix()*transformationMatrix)
        .setObjectId(_objectId)
        .draw(_mesh);
}

void ColoredDrawable::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) {
    _shader
        .setDiffuseColor(_color)
        .setTransformationMatrix(transformationMatrix)
        .setNormalMatrix(transformationMatrix.normalMatrix())
        .setProjectionMatrix(camera.projectionMatrix())
        .setObjectId(_objectId)
        .draw(_mesh);
}

void TexturedDrawable::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) {
    _shader
        .setTransformationMatrix(transformationMatrix)
        .setNormalMatrix(transformationMatrix.normalMatrix())
        .setProjectionMatrix(camera.projectionMatrix())
        .setTextureMatrix(_textureMatrix)
        .setObjectId(_objectId)
        .bindAmbientTexture(_texture)
        .bindDiffuseTexture(_texture);

    if(_shader.flags() & Shaders::Phong::Flag::AlphaMask)
        _shader.setAlphaMask(_alphaMask);

    _shader.draw(_mesh);
}

void MeshVisualizerDrawable::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) {
    GL::Renderer::enable(GL::Renderer::Feature::PolygonOffsetFill);
    GL::Renderer::setPolygonOffset(_(-5.0f), _(-5.0f));

    (*_shader)
        .setProjectionMatrix(camera.projectionMatrix())
        .setTransformationMatrix(transformationMatrix);

    #ifndef MAGNUM_TARGET_GLES
    if(_shader->flags() & Shaders::MeshVisualizer3D::Flag::NormalDirection)
        _shader->setNormalMatrix(transformationMatrix.normalMatrix());
    #endif

    _shader->draw(_mesh);

    GL::Renderer::setPolygonOffset(0.0f, 0.0f);
    GL::Renderer::disable(GL::Renderer::Feature::PolygonOffsetFill);
}

void ScenePlayer::drawEvent() {
    /* Another FB could be bound from a depth / object ID read (moreover with
       color output disabled), set it back to the default framebuffer */
    GL::defaultFramebuffer.bind(); /** @todo mapForDraw() should bind implicitly */
    GL::defaultFramebuffer
        .mapForDraw({{Shaders::Phong::ColorOutput, GL::DefaultFramebuffer::DrawAttachment::Back}})
        .clear(GL::FramebufferClear::Color|GL::FramebufferClear::Depth);

    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);

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

        /* Draw selected object. This needs a depth buffer test again in order
           to correctly order the tangent space visualizers. */
        if(!_data->selectedObjectDrawables.isEmpty()) {;
            GL::Renderer::enable(GL::Renderer::Feature::Blending);
            /* Ugh non-premultiplied alpha */
            GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::SourceAlpha, GL::Renderer::BlendFunction::OneMinusSourceAlpha);

            _data->camera->draw(_data->selectedObjectDrawables);

            GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::One, GL::Renderer::BlendFunction::Zero);
            GL::Renderer::disable(GL::Renderer::Feature::Blending);
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
    GL::Framebuffer::blit(GL::defaultFramebuffer, _depthResolveFramebuffer, GL::defaultFramebuffer.viewport(), GL::FramebufferBlit::Depth);
    #endif
}

void ScenePlayer::viewportEvent(ViewportEvent& event) {
    _baseUiPlane = Containers::NullOpt;
    _ui->relayout(Vector2(event.windowSize())/event.dpiScaling(), event.windowSize(), event.framebufferSize());
    initializeUi();

    if(_data) {
        /* Refresh proper state of all controls -- keep hidden if hidden, show
           if shown, animation state only when there's an animation */
        setControlsVisible(controlsVisible());

        _data->camera->setViewport(event.framebufferSize());
        _baseUiPlane->modelInfo.setText(_data->modelInfo);
        _baseUiPlane->objectInfo.setText(_data->objectInfo);
        updateAnimationTime(_data->elapsedTimeAnimationDestination);
    }

    _meshVisualizerShader.setViewportSize(Vector2{event.framebufferSize()});
    #ifndef MAGNUM_TARGET_GLES
    _tangentSpaceMeshVisualizerShader.setViewportSize(Vector2{event.framebufferSize()});
    #endif

    /* Recreate object ID reading renderbuffers that depend on viewport size */
    _selectionDepth = GL::Renderbuffer{};
    _selectionDepth.setStorage(GL::RenderbufferFormat::DepthComponent24, event.framebufferSize());
    _selectionObjectId = GL::Renderbuffer{};
    _selectionObjectId.setStorage(GL::RenderbufferFormat::R16UI, event.framebufferSize());
    _selectionFramebuffer
        .attachRenderbuffer(GL::Framebuffer::BufferAttachment::Depth, _selectionDepth)
        .attachRenderbuffer(GL::Framebuffer::ColorAttachment{1}, _selectionObjectId)
        .setViewport({{}, event.framebufferSize()});

    /* Recreate depth reading textures and renderbuffers that depend on
       viewport size */
    #ifdef MAGNUM_TARGET_WEBGL
    _depthResolve = GL::Texture2D{};
    _depthResolve.setMinificationFilter(GL::SamplerFilter::Nearest)
        .setMagnificationFilter(GL::SamplerFilter::Nearest)
        .setWrapping(GL::SamplerWrapping::ClampToEdge)
        .setStorage(1, GL::TextureFormat::Depth24Stencil8, event.framebufferSize());
    _depthResolveFramebuffer.attachTexture(GL::Framebuffer::BufferAttachment::Depth, _depthResolve, 0);

    _reinterpretDepth = GL::Renderbuffer{};
    _reinterpretDepth.setStorage(GL::RenderbufferFormat::RGBA8, event.framebufferSize());
    _reinterpretFramebuffer.attachRenderbuffer(GL::Framebuffer::ColorAttachment{0}, _reinterpretDepth);

    _reinterpretFramebuffer.setViewport({{}, event.framebufferSize()});
    #endif
}

Float ScenePlayer::depthAt(const Vector2i& windowPosition) {
    /* First scale the position from being relative to window size to being
       relative to framebuffer size as those two can be different on HiDPI
       systems */
    const Vector2i position = windowPosition*Vector2{application().framebufferSize()}/Vector2{application().windowSize()};
    const Vector2i fbPosition{position.x(), GL::defaultFramebuffer.viewport().sizeY() - position.y() - 1};
    const Range2Di area = Range2Di::fromSize(fbPosition, Vector2i{1}).padded(Vector2i{2});

    /* Easy on sane platforms */
    #ifndef MAGNUM_TARGET_WEBGL
    GL::defaultFramebuffer.mapForRead(GL::DefaultFramebuffer::ReadAttachment::Front);
    Image2D image = GL::defaultFramebuffer.read(area, {GL::PixelFormat::DepthComponent, GL::PixelType::Float});

    return Math::min<Float>(Containers::arrayCast<const Float>(image.data()));

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
    _reinterpretShader.bindDepthTexture(_depthResolve);
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

Vector3 ScenePlayer::unproject(const Vector2i& windowPosition, Float depth) const {
    /* We have to take window size, not framebuffer size, since the position is
       in window coordinates and the two can be different on HiDPI systems */
    const Vector2i viewSize = application().windowSize();
    const Vector2i viewPosition{windowPosition.x(), viewSize.y() - windowPosition.y() - 1};
    const Vector3 in{2*Vector2{viewPosition}/Vector2{viewSize} - Vector2{1.0f}, depth*2.0f - 1.0f};

    return _data->camera->projectionMatrix().inverted().transformPoint(in);
}

void ScenePlayer::keyPressEvent(KeyEvent& event) {
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

    /* Adjust brightness */
    } else if(event.key() == KeyEvent::Key::NumAdd ||
              event.key() == KeyEvent::Key::NumSubtract ||
              event.key() == KeyEvent::Key::Plus ||
              event.key() == KeyEvent::Key::Minus) {
        _brightness *= (event.key() == KeyEvent::Key::NumAdd ||
                        event.key() == KeyEvent::Key::Plus) ? 1.1f : 1/1.1f;
        for(auto* shader: {&_coloredShader, &_texturedShader, &_texturedMaskShader, &_vertexColorShader}) {
            shader->setLightColors({
                0xffffff_rgbf*_brightness,
                0xffcccc_rgbf*_brightness,
                0xccccff_rgbf*_brightness});
        }

    } else return;

    event.setAccepted();
    redraw();
}

void ScenePlayer::mousePressEvent(MouseEvent& event) {
    if(_ui->handlePressEvent(event.position())) {
        redraw();
        event.setAccepted();
        return;
    }

    /* RMB to select */
    if(event.button() == MouseEvent::Button::Right && _data) {
        _selectionFramebuffer.bind(); /** @todo mapForDraw() should bind implicitly */
        _selectionFramebuffer.mapForDraw({
                {Shaders::Generic3D::ColorOutput, GL::Framebuffer::DrawAttachment::None},
                {Shaders::Generic3D::ObjectIdOutput, GL::Framebuffer::ColorAttachment{1}}})
            .clearDepth(1.0f)
            .clearColor(1, Vector4ui{0xffff});
        CORRADE_INTERNAL_ASSERT(_selectionFramebuffer.checkStatus(GL::FramebufferTarget::Draw) == GL::Framebuffer::Status::Complete);

        /* If there's a selected object already, remove it */
        if(_data->selectedObject) {
            delete _data->selectedObject;
            _data->selectedObject = nullptr;
        }

        /** @todo reduce duplication in the below code */

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

        /* Read the ID back */
        _selectionFramebuffer.mapForRead(GL::Framebuffer::ColorAttachment{1});
        CORRADE_INTERNAL_ASSERT(_selectionFramebuffer.checkStatus(GL::FramebufferTarget::Read) == GL::Framebuffer::Status::Complete);

        /* First scale the position from being relative to window size to being
           relative to framebuffer size as those two can be different on HiDPI
           systems */
        const Vector2i position = event.position()*Vector2{application().framebufferSize()}/Vector2{application().windowSize()};
        const Vector2i fbPosition{position.x(), _selectionFramebuffer.viewport().sizeY() - position.y() - 1};
        const Range2Di area = Range2Di::fromSize(fbPosition, Vector2i{1});

        const UnsignedInt selectedId = _selectionFramebuffer.read(area, {PixelFormat::R16UI}).pixels<UnsignedShort>()[0][0];
        /* If nothing is selected, reset the info text */
        if(selectedId == 0xffff) {
            _baseUiPlane->objectInfo.hide();
            _baseUiPlane->modelInfo.show();

        /* Otherwise add a visualizer and update the info */
        } else {
            CORRADE_INTERNAL_ASSERT(!_data->selectedObject);
            CORRADE_INTERNAL_ASSERT(selectedId < _data->objects.size());
            CORRADE_INTERNAL_ASSERT(_data->objects[selectedId].object && _data->objects[selectedId].meshId != 0xffffffffu && _data->meshes[_data->objects[selectedId].meshId].mesh);

            ObjectInfo& objectInfo = _data->objects[selectedId];
            MeshInfo& meshInfo = _data->meshes[_data->objects[selectedId].meshId];

            /* Create a visualizer for the selected object */
            _data->selectedObject = new MeshVisualizerDrawable{*objectInfo.object,
                #ifndef MAGNUM_TARGET_GLES
                _tangentSpace ? (meshInfo.hasSeparateBitangents ?
                    _tangentSpaceSeparateBitangentMeshVisualizerShader :
                    _tangentSpaceMeshVisualizerShader
                    ) : _meshVisualizerShader,
                #else
                _meshVisualizerShader,
                #endif
                *meshInfo.mesh, _data->selectedObjectDrawables};

            /* Show object & mesh info */
            _baseUiPlane->modelInfo.hide();
            Ui::Widget::show({
                _baseUiPlane->objectInfo,
                #ifndef MAGNUM_TARGET_GLES
                _baseUiPlane->tangentSpace
                #endif
            });
            _baseUiPlane->objectInfo.setText(_data->objectInfo = Utility::formatString(
                "{}: {}, indexed, {} attribs, {} verts, {} prims, {:.1f} kB",
                objectInfo.name,
                meshInfo.name,
                meshInfo.attributes,
                meshInfo.vertices,
                meshInfo.primitives,
                meshInfo.size/1024.0f));
        }

        event.setAccepted();
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

void ScenePlayer::mouseReleaseEvent(MouseEvent& event) {
    if(_ui->handleReleaseEvent(event.position())) {
        redraw();
        event.setAccepted();
        return;
    }
}

void ScenePlayer::mouseMoveEvent(MouseMoveEvent& event) {
    /* In some cases (when focusing a window by a click) the browser reports a
       move event with pressed buttons *before* the corresponding press event.
       To avoid jumpy behavior in that case, make sure the last position is
       always up-to-date by calculating it every time. The same applies for UI
       as well -- if the user grabs over a button, it should ignore the
       movement while the mouse is over the button but then shouldn't jump when
       the mouse leaves the button again. */
    if(_lastPosition == Vector2i{-1}) _lastPosition = event.position();
    const Vector2i delta = event.position() - _lastPosition;
    _lastPosition = event.position();

    if(_ui->handleMoveEvent(event.position())) {
        redraw();
        event.setAccepted();
        return;
    }

    /* Due to compatibility reasons, scroll is also reported as a press event,
       so filter that out */
    if(!(event.buttons() & (MouseMoveEvent::Button::Left|
                            MouseMoveEvent::Button::Middle)) || !_data) return;

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

void ScenePlayer::mouseScrollEvent(MouseScrollEvent& event) {
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

    /* Move towards/backwards the rotation point in cam coords */
    _data->cameraObject->translateLocal(
        _data->cameraObject->rotation().transformVector(_rotationPoint*event.offset().y()*0.1f));

    event.setAccepted();
    redraw();
}

}

Containers::Pointer<AbstractPlayer> createScenePlayer(Platform::ScreenedApplication& application, Ui::UserInterface& uiToStealFontFrom) {
    return Containers::Pointer<ScenePlayer>{Containers::InPlaceInit, application, uiToStealFontFrom};
}

}}
