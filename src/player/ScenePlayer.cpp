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

#include <algorithm> /* std::sort() */
#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/GrowableArray.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/Pair.h>
#include <Corrade/Containers/Reference.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Containers/Triple.h>
#include <Corrade/Interconnect/Receiver.h>
#include <Corrade/PluginManager/Manager.h>
#include <Corrade/Utility/Arguments.h>
#include <Corrade/Utility/ConfigurationGroup.h>
#include <Corrade/Utility/DebugStl.h>
#include <Corrade/Utility/FormatStl.h>
#include <Corrade/Utility/Path.h>
#include <Magnum/Image.h>
#include <Magnum/ImageView.h>
#include <Magnum/Mesh.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/Animation/Player.h>
#include <Magnum/DebugTools/ColorMap.h>
#include <Magnum/DebugTools/FrameProfiler.h>
#include <Magnum/GL/Context.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Extensions.h>
#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/PixelFormat.h>
#include <Magnum/GL/Renderbuffer.h>
#include <Magnum/GL/RenderbufferFormat.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/Math/CubicHermite.h>
#include <Magnum/Math/FunctionsBatch.h>
#include <Magnum/Math/Swizzle.h>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/MeshTools/Duplicate.h>
#include <Magnum/MeshTools/GenerateIndices.h>
#include <Magnum/MeshTools/Transform.h>
#include <Magnum/Primitives/Axis.h>
#include <Magnum/Primitives/Crosshair.h>
#include <Magnum/Primitives/Cone.h>
#include <Magnum/Primitives/Circle.h>
#include <Magnum/Primitives/Line.h>
#include <Magnum/Primitives/UVSphere.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/SceneGraph/TranslationRotationScalingTransformation3D.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/Shaders/FlatGL.h>
#include <Magnum/Shaders/PhongGL.h>
#include <Magnum/Shaders/MeshVisualizerGL.h>
#include <Magnum/Text/Alignment.h>
#include <Magnum/Trade/AbstractImporter.h>
#include <Magnum/Trade/AnimationData.h>
#include <Magnum/Trade/CameraData.h>
#include <Magnum/Trade/ImageData.h>
#include <Magnum/Trade/LightData.h>
#include <Magnum/Trade/MeshData.h>
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
    vert.addSource(rs.getString("DepthReinterpretShader.vert"));
    frag.addSource(rs.getString("DepthReinterpretShader.frag"));

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
        objectVisualization{*this, {Ui::Snap::Bottom, shadeless, ButtonSize},
            "Object centers"},
        meshVisualization{*this, {Ui::Snap::Bottom, shadeless, ButtonSize}, "Wireframe", 16},
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
            meshVisualization,
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
    Ui::Button objectVisualization, meshVisualization;
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
    UnsignedInt objectIdCount;
    std::size_t size;
    std::string name;
    bool hasTangents, hasSeparateBitangents;
};

struct LightInfo {
    Containers::Optional<Trade::LightData> light;
    std::string name;
    std::string type;
};

struct ObjectInfo {
    Object3D* object;
    std::string name;
    std::string type;
    UnsignedInt meshId{0xffffffffu};
    UnsignedInt lightId{0xffffffffu};
    UnsignedInt childCount;
};

class MeshVisualizerDrawable;

struct Data {
    Containers::Array<MeshInfo> meshes;
    Containers::Array<LightInfo> lights;
    Containers::Array<Containers::Optional<GL::Texture2D>> textures;

    Scene3D scene;
    Object3D* cameraObject{};
    SceneGraph::Camera3D* camera;
    SceneGraph::DrawableGroup3D opaqueDrawables, transparentDrawables,
        selectedObjectDrawables, objectVisualizationDrawables, lightDrawables;
    Vector3 previousPosition;

    Containers::Array<ObjectInfo> objects;
    bool visualizeObjects = false;
    MeshVisualizerDrawable* selectedObject{};

    Containers::Array<char> animationData;
    Animation::Player<std::chrono::nanoseconds, Float> player;

    UnsignedInt lightCount{};
    Containers::Array<Vector4> lightPositions;
    Containers::Array<Color3> lightColors;

    Int elapsedTimeAnimationDestination = -1; /* So it gets updated with 0 as well */

    /* UI is recreated on window resize and we need to repopulate the info */
    /** @todo remove once the UI has relayouting */
    std::string modelInfo, objectInfo;
};

template<class T> struct EnumSetHash: std::hash<typename std::underlying_type<typename T::Type>::type> {
    std::size_t operator()(const T& value) const {
        return std::hash<typename std::underlying_type<typename T::Type>::type>::operator()(Containers::enumCastUnderlyingType(value));
    }
};

enum class Visualization: UnsignedByte {
    Begin = 0,
    Wireframe = 0,
    #ifndef MAGNUM_TARGET_GLES
    WireframeTbn,
    #endif
    WireframeObjectId,
    WireframeVertexId,
    #ifndef MAGNUM_TARGET_GLES
    WireframePrimitiveId,
    #endif
    ObjectId,
    VertexId,
    #ifndef MAGNUM_TARGET_GLES
    PrimitiveId,
    #endif
    End
};

class ScenePlayer: public AbstractPlayer, public Interconnect::Receiver {
    public:
        explicit ScenePlayer(Platform::ScreenedApplication& application, Ui::UserInterface& uiToStealFontFrom, const DebugTools::FrameProfilerGL::Values profilerValues, bool& drawUi);

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

        void cycleObjectVisualization();
        void cycleMeshVisualization();
        Shaders::MeshVisualizerGL3D::Flags setupVisualization(std::size_t meshId);

        void play();
        void pause();
        void stop();
        void backward();
        void forward();
        void updateAnimationTime(Int deciseconds);
        void updateLightColorBrightness();

        Float depthAt(const Vector2i& windowPosition);
        Vector3 unproject(const Vector2i& windowPosition, Float depth) const;

        Shaders::FlatGL3D& flatShader(Shaders::FlatGL3D::Flags flags);
        Shaders::PhongGL& phongShader(Shaders::PhongGL::Flags flags);
        Shaders::MeshVisualizerGL3D& meshVisualizerShader(Shaders::MeshVisualizerGL3D::Flags flags);

        /* Global rendering stuff */
        std::unordered_map<Shaders::FlatGL3D::Flags, Shaders::FlatGL3D, EnumSetHash<Shaders::FlatGL3D::Flags>> _flatShaders;
        std::unordered_map<Shaders::PhongGL::Flags, Shaders::PhongGL, EnumSetHash<Shaders::PhongGL::Flags>> _phongShaders;
        std::unordered_map<Shaders::MeshVisualizerGL3D::Flags, Shaders::MeshVisualizerGL3D, EnumSetHash<Shaders::MeshVisualizerGL3D::Flags>> _meshVisualizerShaders;
        GL::Texture2D _colorMapTexture;
        /* Object and light visualization */
        GL::Mesh _lightCenterMesh, _lightInnerConeMesh, _lightOuterCircleMesh,
            _lightSphereMesh, _lightDirectionMesh, _axisMesh;

        Float _brightness{0.5f};
        #ifndef MAGNUM_TARGET_GLES
        Float _lineLength = 0.3f;
        #endif
        bool _shadeless = false;
        Visualization _visualization = Visualization::Wireframe;

        /* Data loading */
        Containers::Optional<Data> _data;

        /* UI */
        bool& _drawUi;
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

        /* Profiling */
        DebugTools::FrameProfilerGL _profiler;
        Debug _profilerOut{Debug::Flag::NoNewlineAtTheEnd|
            (Debug::isTty() ? Debug::Flags{} : Debug::Flag::DisableColors)};
};

class FlatDrawable: public SceneGraph::Drawable3D {
    public:
        explicit FlatDrawable(Object3D& object, Shaders::FlatGL3D& shader, GL::Mesh& mesh, UnsignedInt objectId, const Color4& color, const Vector3& scale, SceneGraph::DrawableGroup3D& group): SceneGraph::Drawable3D{object, &group}, _shader(shader), _mesh(mesh), _objectId{objectId}, _color{color}, _scale{scale} {}

    private:
        void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) override;

        Shaders::FlatGL3D& _shader;
        GL::Mesh& _mesh;
        UnsignedInt _objectId;
        Color4 _color;
        Vector3 _scale;
};

class PhongDrawable: public SceneGraph::Drawable3D {
    public:
        explicit PhongDrawable(Object3D& object, Shaders::PhongGL& shader, GL::Mesh& mesh, UnsignedInt objectId, const Color4& color, GL::Texture2D* diffuseTexture, GL::Texture2D* normalTexture, Float normalTextureScale, Float alphaMask, Matrix3 textureMatrix, const bool& shadeless, SceneGraph::DrawableGroup3D& group): SceneGraph::Drawable3D{object, &group}, _shader(shader), _mesh(mesh), _objectId{objectId}, _color{color}, _diffuseTexture{diffuseTexture}, _normalTexture{normalTexture}, _normalTextureScale{normalTextureScale}, _alphaMask{alphaMask}, _textureMatrix{textureMatrix}, _shadeless(shadeless) {}

        explicit PhongDrawable(Object3D& object, Shaders::PhongGL& shader, GL::Mesh& mesh, UnsignedInt objectId, const Color4& color, const bool& shadeless, SceneGraph::DrawableGroup3D& group): SceneGraph::Drawable3D{object, &group}, _shader(shader), _mesh(mesh), _objectId{objectId}, _color{color}, _diffuseTexture{nullptr}, _normalTexture{nullptr}, _alphaMask{0.5f}, _shadeless{shadeless} {}

    private:
        void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) override;

        Shaders::PhongGL& _shader;
        GL::Mesh& _mesh;
        UnsignedInt _objectId;
        Color4 _color;
        GL::Texture2D* _diffuseTexture;
        GL::Texture2D* _normalTexture;
        Float _normalTextureScale;
        Float _alphaMask;
        Matrix3 _textureMatrix;
        const bool& _shadeless;
};

class MeshVisualizerDrawable: public SceneGraph::Drawable3D {
    public:
        explicit MeshVisualizerDrawable(Object3D& object, Shaders::MeshVisualizerGL3D& shader, GL::Mesh& mesh, std::size_t meshId, UnsignedInt objectIdCount, UnsignedInt vertexCount, UnsignedInt primitiveCount, const bool& shadeless, SceneGraph::DrawableGroup3D& group): SceneGraph::Drawable3D{object, &group}, _shader(shader), _mesh(mesh), _meshId{meshId}, _objectIdCount{objectIdCount}, _vertexCount{vertexCount}, _primitiveCount{primitiveCount}, _shadeless(shadeless) {}

        Shaders::MeshVisualizerGL3D& shader() { return _shader; }

        void setShader(Shaders::MeshVisualizerGL3D& shader) { _shader = shader; }

        std::size_t meshId() const { return _meshId; }

    private:
        void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) override;

        Containers::Reference<Shaders::MeshVisualizerGL3D> _shader;
        GL::Mesh& _mesh;
        std::size_t _meshId;
        UnsignedInt _objectIdCount, _vertexCount, _primitiveCount;
        const bool& _shadeless;
};

class LightDrawable: public SceneGraph::Drawable3D {
    public:
        explicit LightDrawable(Object3D& object, bool directional, Containers::Array<Vector4>& positions, SceneGraph::DrawableGroup3D& group):
            SceneGraph::Drawable3D{object, &group}, _directional{directional},
            /* GCC 4.8 can't handle {} here */
            _positions(positions) {}

    private:
        void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D&) override {
            arrayAppend(_positions, _directional ?
                Vector4{transformationMatrix.backward(), 0.0f} :
                Vector4{transformationMatrix.translation(), 1.0f});
        }

        bool _directional;
        Containers::Array<Vector4>& _positions;
};

ScenePlayer::ScenePlayer(Platform::ScreenedApplication& application, Ui::UserInterface& uiToStealFontFrom, DebugTools::FrameProfilerGL::Values profilerValues, bool& drawUi): AbstractPlayer{application, PropagatedEvent::Draw|PropagatedEvent::Input}, _drawUi(drawUi) {
    /* Color maps */
    _colorMapTexture
        .setMinificationFilter(SamplerFilter::Linear, SamplerMipmap::Linear)
        .setMagnificationFilter(SamplerFilter::Linear)
        .setWrapping(SamplerWrapping::Repeat)
        .setStorage(1, GL::TextureFormat::RGBA8, {256, 1})
        .setSubImage(0, {}, ImageView2D{PixelFormat::RGB8Unorm, {256, 1}, DebugTools::ColorMap::turbo()});

    /* Object and light visualizers */
    _axisMesh = MeshTools::compile(Primitives::axis3D());
    _lightCenterMesh = MeshTools::compile(Primitives::crosshair3D());
    _lightSphereMesh = MeshTools::compile(Primitives::uvSphereWireframe(32, 64));

    /* Directional light visualization is a line in the -Z direction, with a
       tip at origin. */
    _lightDirectionMesh = MeshTools::compile(Primitives::line3D({}, Vector3::zAxis(-1.0f)));

    /* Make the spotlight visualization cone center at the tip, pointing in -Z
       direction to match the spotlight defaults. The circle is visualizing the
       outer angle, put it at the position of the cone cap so we can scale it
       to the desired form as well. */
    {
        Trade::MeshData cone = Primitives::coneWireframe(32, 0.5f);
        MeshTools::transformPointsInPlace(
            Matrix4::rotationX(90.0_degf)*
            Matrix4::translation(Vector3::yAxis(-0.5f)),
            cone.mutableAttribute<Vector3>(Trade::MeshAttribute::Position));
        _lightInnerConeMesh = MeshTools::compile(cone);

        Trade::MeshData circle = Primitives::circle3DWireframe(32);
        MeshTools::transformPointsInPlace(
            Matrix4::translation(Vector3::zAxis(-1.0f)),
            circle.mutableAttribute<Vector3>(Trade::MeshAttribute::Position));
        _lightOuterCircleMesh = MeshTools::compile(circle);
    }

    /* Setup the UI, steal font etc. from the existing one to avoid having
       everything built twice */
    /** @todo this is extremely bad, there should be just one global UI (or
        not?) */
    _ui.emplace(Vector2(application.windowSize())/application.dpiScaling(), application.windowSize(), application.framebufferSize(), uiToStealFontFrom.font(), uiToStealFontFrom.glyphCache(), Ui::mcssDarkStyleConfiguration());
    initializeUi();

    /* Set up offscreen rendering for object ID retrieval */
    _selectionDepth.setStorage(GL::RenderbufferFormat::DepthComponent24, application.framebufferSize());
    _selectionObjectId.setStorage(GL::RenderbufferFormat::R16UI, application.framebufferSize());
    _selectionFramebuffer = GL::Framebuffer{{{}, application.framebufferSize()}};
    _selectionFramebuffer
        .attachRenderbuffer(GL::Framebuffer::BufferAttachment::Depth, _selectionDepth)
        .attachRenderbuffer(GL::Framebuffer::ColorAttachment{1}, _selectionObjectId);
    _selectionFramebuffer.mapForDraw({
        {Shaders::GenericGL3D::ColorOutput, GL::Framebuffer::DrawAttachment::None},
        {Shaders::GenericGL3D::ObjectIdOutput, GL::Framebuffer::ColorAttachment{1}}});
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

    #ifndef MAGNUM_TARGET_GLES
    /* Set up the profiler, filter away unsupported values */
    if(profilerValues & DebugTools::FrameProfilerGL::Value::GpuDuration && !GL::Context::current().isExtensionSupported<GL::Extensions::ARB::timer_query>()) {
        Debug{} << "ARB_timer_query not supported, GPU time profiling will be unavailable";
        profilerValues &= ~DebugTools::FrameProfilerGL::Value::GpuDuration;
    }
    if(profilerValues & (DebugTools::FrameProfilerGL::Value::VertexFetchRatio|DebugTools::FrameProfilerGL::Value::PrimitiveClipRatio) &&
        !GL::Context::current().isExtensionSupported<GL::Extensions::ARB::pipeline_statistics_query>()
    ) {
        Debug{} << "ARB_pipeline_statistics_query not supported, GPU pipeline profiling will be unavailable";
        profilerValues &= ~(DebugTools::FrameProfilerGL::Value::VertexFetchRatio|DebugTools::FrameProfilerGL::Value::PrimitiveClipRatio);
    }
    #elif !defined(MAGNUM_TARGET_WEBGL)
    if(profilerValues & DebugTools::FrameProfilerGL::Value::GpuDuration &&
        !GL::Context::current().isExtensionSupported<GL::Extensions::EXT::disjoint_timer_query>()
    ) {
        Debug{} << "EXT_disjoint_timer_query not supported, GPU time profiling will be unavailable";
        profilerValues &= ~DebugTools::FrameProfilerGL::Value::GpuDuration;
    }
    #else
    if(profilerValues & DebugTools::FrameProfilerGL::Value::GpuDuration &&
        !GL::Context::current().isExtensionSupported<GL::Extensions::EXT::disjoint_timer_query_webgl2>()
    ) {
        Debug{} << "EXT_disjoint_timer_query_webgl2 not supported, GPU time profiling will be unavailable";
        profilerValues &= ~DebugTools::FrameProfilerGL::Value::GpuDuration;
    }
    #endif

    /* Disable profiler by default */
    _profiler = DebugTools::FrameProfilerGL{profilerValues, 50};
    _profiler.disable();
}

Shaders::FlatGL3D& ScenePlayer::flatShader(Shaders::FlatGL3D::Flags flags) {
    auto found = _flatShaders.find(flags);
    if(found == _flatShaders.end())
        found = _flatShaders.emplace(flags,
            Shaders::FlatGL3D{Shaders::FlatGL3D::Configuration{}
                .setFlags(Shaders::FlatGL3D::Flag::ObjectId|flags)}).first;
    return found->second;
}

Shaders::PhongGL& ScenePlayer::phongShader(Shaders::PhongGL::Flags flags) {
    auto found = _phongShaders.find(flags);
    if(found == _phongShaders.end()) {
        found = _phongShaders.emplace(flags,
            Shaders::PhongGL{Shaders::PhongGL::Configuration{}
                .setFlags(Shaders::PhongGL::Flag::ObjectId|flags)
                .setLightCount(_data->lightCount ? _data->lightCount : 3)
        }).first;
        found->second
            .setSpecularColor(0x11111100_rgbaf)
            .setShininess(80.0f);
    }
    return found->second;
}

Shaders::MeshVisualizerGL3D& ScenePlayer::meshVisualizerShader(Shaders::MeshVisualizerGL3D::Flags flags) {
    auto found = _meshVisualizerShaders.find(flags);
    if(found == _meshVisualizerShaders.end()) {
        found = _meshVisualizerShaders.emplace(flags,
            Shaders::MeshVisualizerGL3D{Shaders::MeshVisualizerGL3D::Configuration{}
                .setFlags(flags)}).first;
        found->second
            .setViewportSize(Vector2{application().framebufferSize()});

        if(flags & Shaders::MeshVisualizerGL3D::Flag::Wireframe)
            found->second.setWireframeColor(_(0xdcdcdcff_rgbaf));
        #ifndef MAGNUM_TARGET_GLES
        if(flags & Shaders::MeshVisualizerGL3D::Flag::NormalDirection)
            found->second
                .setLineLength(_lineLength)
                .setLineWidth(2.0f);
        #endif

        if(flags & (Shaders::MeshVisualizerGL3D::Flag::InstancedObjectId|
            Shaders::MeshVisualizerGL3D::Flag::VertexId
            #ifndef MAGNUM_TARGET_GLES
            |Shaders::MeshVisualizerGL3D::Flag::PrimitiveId
            #endif
        ))
            found->second.bindColorMapTexture(_colorMapTexture);
    }
    return found->second;
}

void ScenePlayer::initializeUi() {
    _baseUiPlane.emplace(*_ui);

    if(_shadeless) _baseUiPlane->shadeless.setStyle(Ui::Style::Success);
    if(_data) {
        if(_data->visualizeObjects)
            _baseUiPlane->objectVisualization.setStyle(Ui::Style::Success);
        if(_data->selectedObject)
            setupVisualization(_data->selectedObject->meshId());
    }

    Interconnect::connect(_baseUiPlane->shadeless, &Ui::Button::tapped, *this, &ScenePlayer::toggleShadeless);
    Interconnect::connect(_baseUiPlane->objectVisualization, &Ui::Button::tapped, *this, &ScenePlayer::cycleObjectVisualization);
    #ifndef MAGNUM_TARGET_GLES
    Interconnect::connect(_baseUiPlane->meshVisualization, &Ui::Button::tapped, *this, &ScenePlayer::cycleMeshVisualization);
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

            Ui::Widget::setVisible(_data->selectedObject, {
                _baseUiPlane->meshVisualization,
                _baseUiPlane->objectInfo});
            Ui::Widget::setVisible(!_data->selectedObject, {
                _baseUiPlane->objectVisualization,
                _baseUiPlane->modelInfo});
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
            _baseUiPlane->objectVisualization,
            _baseUiPlane->meshVisualization,
            _baseUiPlane->objectInfo,
            _baseUiPlane->animationProgress});
    }
}

void ScenePlayer::toggleShadeless() {
    /* _shadeless is used by the drawables to set up the shaders differently */
    _baseUiPlane->shadeless.setStyle((_shadeless ^= true) ? Ui::Style::Success : Ui::Style::Default);
}

void ScenePlayer::cycleObjectVisualization() {
    _baseUiPlane->objectVisualization.setStyle((_data->visualizeObjects ^= true) ? Ui::Style::Success : Ui::Style::Default);
}

void ScenePlayer::cycleMeshVisualization() {
    CORRADE_INTERNAL_ASSERT(_data->selectedObject);

    /* Advance through the options */
    _visualization = Visualization(UnsignedByte(_visualization) + 1);

    _data->selectedObject->setShader(meshVisualizerShader(setupVisualization( _data->selectedObject->meshId())));
}

Shaders::MeshVisualizerGL3D::Flags ScenePlayer::setupVisualization(std::size_t meshId) {
    const MeshInfo& info = _data->meshes[meshId];

    #ifndef MAGNUM_TARGET_GLES
    if(_visualization == Visualization::WireframeTbn && info.primitives >= 100000) {
        Warning{} << "Mesh has" << info.primitives << "primitives, skipping TBN visualization";
        _visualization = Visualization(UnsignedByte(_visualization) + 1);
    }
    #endif

    /* If visualizing object ID, make sure the object actually has that */
    if((_visualization == Visualization::ObjectId ||
        _visualization == Visualization::WireframeObjectId) &&
        !_data->meshes[_data->selectedObject->meshId()].objectIdCount)
        _visualization = Visualization(UnsignedByte(_visualization) + 1);

    /* Wrap around */
    if(_visualization == Visualization::End)
        _visualization = Visualization::Begin;

    if(_visualization == Visualization::Wireframe) {
        _baseUiPlane->meshVisualization.setText("Wireframe");
        return Shaders::MeshVisualizerGL3D::Flag::Wireframe;
    }
    #ifndef MAGNUM_TARGET_GLES
    if(_visualization == Visualization::WireframeTbn) {
        _baseUiPlane->meshVisualization.setText("Wire + TBN");
        Shaders::MeshVisualizerGL3D::Flags flags =
            Shaders::MeshVisualizerGL3D::Flag::Wireframe|
            Shaders::MeshVisualizerGL3D::Flag::TangentDirection|
            Shaders::MeshVisualizerGL3D::Flag::NormalDirection;
        if(info.hasSeparateBitangents)
            flags |= Shaders::MeshVisualizerGL3D::Flag::BitangentDirection;
        else
            flags |= Shaders::MeshVisualizerGL3D::Flag::BitangentFromTangentDirection;
        return flags;
    }
    #endif

    if(_visualization == Visualization::WireframeObjectId) {
        _baseUiPlane->meshVisualization.setText("Wire + Object ID");
        return Shaders::MeshVisualizerGL3D::Flag::Wireframe|Shaders::MeshVisualizerGL3D::Flag::InstancedObjectId;
    }

    if(_visualization == Visualization::WireframeVertexId) {
        _baseUiPlane->meshVisualization.setText("Wire + Vertex ID");
        return Shaders::MeshVisualizerGL3D::Flag::Wireframe|Shaders::MeshVisualizerGL3D::Flag::VertexId;
    }

    #ifndef MAGNUM_TARGET_GLES
    if(_visualization == Visualization::WireframePrimitiveId) {
        _baseUiPlane->meshVisualization.setText("Wire + Prim ID");
        return Shaders::MeshVisualizerGL3D::Flag::Wireframe|Shaders::MeshVisualizerGL3D::Flag::PrimitiveId;
    }
    #endif

    if(_visualization == Visualization::ObjectId) {
        _baseUiPlane->meshVisualization.setText("Object ID");
        return Shaders::MeshVisualizerGL3D::Flag::InstancedObjectId;
    }

    if(_visualization == Visualization::VertexId) {
        _baseUiPlane->meshVisualization.setText("Vertex ID");
        return Shaders::MeshVisualizerGL3D::Flag::VertexId;
    }

    #ifndef MAGNUM_TARGET_GLES
    if(_visualization == Visualization::PrimitiveId) {
        _baseUiPlane->meshVisualization.setText("Primitive ID");
        return Shaders::MeshVisualizerGL3D::Flag::PrimitiveId;
    }
    #endif

    CORRADE_INTERNAL_ASSERT_UNREACHABLE();
}

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

void ScenePlayer::updateLightColorBrightness() {
    Containers::Array<Color3> lightColorsBrightness{NoInit, _data->lightColors.size()};
    for(UnsignedInt i = 0; i != lightColorsBrightness.size(); ++i)
        lightColorsBrightness[i] = _data->lightColors[i]*_brightness;
    for(auto& shader: _phongShaders)
        shader.second.setLightColors(lightColorsBrightness);
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
        if(!textureData || textureData->type() != Trade::TextureType::Texture2D) {
            Warning{} << "Cannot load texture" << i << importer.textureName(i);
            continue;
        }

        Containers::Optional<Trade::ImageData2D> imageData = importer.image2D(textureData->image());
        if(!imageData) {
            Warning{} << "Cannot load texture" << i << importer.image2DName(textureData->image());
            continue;
        }

        /* Configure the texture */
        GL::Texture2D texture;
        texture
            .setMagnificationFilter(textureData->magnificationFilter())
            .setMinificationFilter(textureData->minificationFilter(), textureData->mipmapFilter())
            .setWrapping(textureData->wrapping().xy());

        loadImage(texture, *imageData);

        _data->textures[i] = std::move(texture);
    }

    /* Load all lights. Lights that fail to load will be NullOpt, saving the
       whole imported data so we can populate the selection info later. */
    Debug{} << "Loading" << importer.lightCount() << "lights";
    _data->lights = Containers::Array<LightInfo>{importer.lightCount()};
    for(UnsignedInt i = 0; i != importer.lightCount(); ++i) {
        _data->lights[i].name = importer.lightName(i);
        if(_data->lights[i].name.empty())
            _data->lights[i].name = Utility::formatString("#{}", i);

        Containers::Optional<Trade::LightData> light = importer.light(i);
        if(light) {
            switch(light->type()) {
                case Trade::LightData::Type::Ambient:
                    _data->lights[i].type = "ambient light";
                    break;
                case Trade::LightData::Type::Directional:
                    _data->lights[i].type = "directional light";
                    break;
                case Trade::LightData::Type::Point:
                    _data->lights[i].type = "point light";
                    break;
                case Trade::LightData::Type::Spot:
                    _data->lights[i].type = "spot light";
                    break;
            }

            _data->lights[i].light = std::move(light);
        }
    }

    /* Load all materials. Materials that fail to load will be NullOpt. The
       data will be stored directly in objects later, so save them only
       temporarily. */
    Debug{} << "Loading" << importer.materialCount() << "materials";
    Containers::Array<Containers::Optional<Trade::PhongMaterialData>> materials{importer.materialCount()};
    for(UnsignedInt i = 0; i != importer.materialCount(); ++i) {
        Containers::Optional<Trade::MaterialData> materialData = importer.material(i);
        if(!materialData || !(materialData->types() & Trade::MaterialType::Phong) || (materialData->as<Trade::PhongMaterialData>().hasTextureTransformation() && !materialData->as<Trade::PhongMaterialData>().hasCommonTextureTransformation()) || materialData->as<Trade::PhongMaterialData>().hasTextureCoordinates()) {
            Warning{} << "Cannot load material" << i << importer.materialName(i);
            continue;
        }

        materials[i] = std::move(*materialData).as<Trade::PhongMaterialData>();
    }

    /* Load all meshes. Meshes that fail to load will be NullOpt. Remember
       which have vertex colors, so in case there's no material we can use that
       instead. */
    Debug{} << "Loading" << importer.meshCount() << "meshes";
    _data->meshes = Containers::Array<MeshInfo>{importer.meshCount()};
    Containers::Array<bool> hasVertexColors{DirectInit, importer.meshCount(), false};
    for(UnsignedInt i = 0; i != importer.meshCount(); ++i) {
        Containers::Optional<Trade::MeshData> meshData = importer.mesh(i);
        if(!meshData) {
            Warning{} << "Cannot load mesh" << i << importer.meshName(i);
            continue;
        }

        std::string meshName = importer.meshName(i);
        if(meshName.empty()) meshName = Utility::formatString("#{}", i);

        /* Disable warnings on custom attributes, as we printed them with
           actual string names below. Generate normals for triangle meshes
           (and don't do anything for line/point meshes, there it makes no
           sense). */
        MeshTools::CompileFlags flags = MeshTools::CompileFlag::NoWarnOnCustomAttributes;
        if((meshData->primitive() == MeshPrimitive::Triangles ||
            meshData->primitive() == MeshPrimitive::TriangleStrip ||
            meshData->primitive() == MeshPrimitive::TriangleFan) &&
           !meshData->attributeCount(Trade::MeshAttribute::Normal) &&
            meshData->hasAttribute(Trade::MeshAttribute::Position) &&
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
        /* Needed for a warning when using a mesh with no tangents with a
           normal map (as, unlike with normals, we have no builtin way to
           generate tangents right now) */
        _data->meshes[i].hasTangents = meshData->hasAttribute(Trade::MeshAttribute::Tangent);
        /* Needed to decide how to visualize tangent space */
        _data->meshes[i].hasSeparateBitangents = meshData->hasAttribute(Trade::MeshAttribute::Bitangent);
        if(meshData->hasAttribute(Trade::MeshAttribute::ObjectId)) {
            _data->meshes[i].objectIdCount = Math::max(meshData->objectIdsAsArray());
        } else _data->meshes[i].objectIdCount = 0;
        _data->meshes[i].mesh = MeshTools::compile(*meshData, flags);
        _data->meshes[i].name = std::move(meshName);
    }

    /* Load the scene. Save the object pointers in an array for easier mapping
       of animations later. */
    if((id < 0 && importer.sceneCount()) || id >= 0) {
        /* If there's no default scene, load the first one */
        if(id < 0)
            id = importer.defaultScene() == -1 ? 0 : importer.defaultScene();
        Debug{} << "Loading scene" << id << importer.sceneName(id);

        Containers::Optional<Trade::SceneData> scene = importer.scene(id);
        if(!scene ||
           !scene->is3D() ||
           !scene->hasField(Trade::SceneField::Parent)) {
            Error{} << "Cannot load the scene, aborting";
            return;
        }

        /* Allocate objects that are part of the hierarchy and fill their
           implicit info */
        _data->objects = Containers::Array<ObjectInfo>{ValueInit, std::size_t(scene->mappingBound())};
        const Containers::Array<Containers::Pair<UnsignedInt, Int>> parents = scene->parentsAsArray();
        for(const Containers::Pair<UnsignedInt, Int>& parent: parents) {
            const UnsignedInt objectId = parent.first();

            _data->objects[objectId].object = new Object3D{};
            _data->objects[objectId].type = "empty";
            _data->objects[objectId].name = importer.objectName(objectId);
            if(_data->objects[objectId].name.empty())
                _data->objects[objectId].name = Utility::formatString("object #{}", objectId);
        }

        /* Assign parent references, separately because there's no guarantee
           that a parent was allocated already when it's referenced */
        for(const Containers::Pair<UnsignedInt, Int>& parent: parents) {
            const UnsignedInt objectId = parent.first();
            _data->objects[objectId].object->setParent(parent.second() == -1 ? &_data->scene : _data->objects[parent.second()].object);

            if(parent.second() != -1)
                ++_data->objects[parent.second()].childCount;
        }

        /* Set transformations. Objects that are not part of the hierarchy are
           ignored, objects that have no transformation entry retain an
           identity transformation. */
        for(const Containers::Pair<UnsignedInt, Matrix4>& transformation: scene->transformations3DAsArray())
            if(Object3D* object = _data->objects[transformation.first()].object)
                object->setTransformation(transformation.second());

        /* Import all lights so we know which shaders to instantiate */
        if(scene->hasField(Trade::SceneField::Light)) for(const Containers::Pair<UnsignedInt, UnsignedInt>& lightReference: scene->lightsAsArray()) {
            const UnsignedInt objectId = lightReference.first();
            const UnsignedInt lightId = lightReference.second();
            Object3D* const object = _data->objects[objectId].object;
            if(!object) continue;

            ++_data->lightCount;

            /* Save the light pointer as well, so we know what to print for
               object selection. Lights have their own info text, so not
               setting the type. */
            /** @todo this doesn't handle multi-light objects */
            _data->objects[objectId].lightId = lightId;

            /* Add a light drawable, which puts correct camera-relative
               position to _data->lightPositions. Light colors don't change so
               add that directly. */
            const Trade::LightData& light = *_data->lights[lightId].light;
            new LightDrawable{*object, light.type() == Trade::LightData::Type::Directional ? true : false, _data->lightPositions, _data->lightDrawables};
            arrayAppend(_data->lightColors, InPlaceInit, light.color()*light.intensity());

            /* Visualization of the center */
            new FlatDrawable{*object, flatShader({}), _lightCenterMesh, objectId, light.color(), Vector3{0.25f}, _data->objectVisualizationDrawables};

            /* If the range is infinite, display it at distance = 5. It's not
               great as it's quite misleading, but better than nothing. */
            /** @todo make this runtime-changeable like with TBN visualizers */
            Float range;
            if(light.range() != Constants::inf()) range = light.range();
            else range = 5.0f;

            /* Point light has a sphere around */
            if(light.type() == Trade::LightData::Type::Point) {
                new FlatDrawable{*object, flatShader({}), _lightSphereMesh, objectId, light.color(), Vector3{range}, _data->objectVisualizationDrawables};

            /* Spotlight has a cone visualizing the inner angle and a circle at
               the end visualizing the outer angle */
            } else if(light.type() == Trade::LightData::Type::Spot) {
                new FlatDrawable{*object, flatShader({}), _lightInnerConeMesh, objectId, light.color(),
                    Math::gather<'x', 'x', 'y'>(Vector2{
                        range*Math::tan(light.innerConeAngle()*0.5f), range
                    }), _data->objectVisualizationDrawables};
                new FlatDrawable{*object, flatShader({}), _lightOuterCircleMesh, objectId, light.color(),
                    Math::gather<'x', 'x', 'y'>(Vector2{
                        range*Math::tan(light.outerConeAngle()*0.5f), range
                    }), _data->objectVisualizationDrawables};

            /* Directional has a circle and a line in its direction. The range
               is always infinite, so the line has always a length of 15. */
            } else if(light.type() == Trade::LightData::Type::Directional) {
                new FlatDrawable{*object, flatShader({}), _lightOuterCircleMesh, objectId, light.color(), Vector3{0.25f, 0.25f, 0.0f}, _data->objectVisualizationDrawables};
                new FlatDrawable{*object, flatShader({}), _lightDirectionMesh, objectId, light.color(), Vector3{5.0f}, _data->objectVisualizationDrawables};

            /* Ambient lights are defined just by the center */
            } else if(light.type() == Trade::LightData::Type::Ambient) {

            /** @todo handle area lights when those are implemented */
            } else CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        }

        /* Import cameras, the first camera will be treated as the default one */
        if(scene->hasField(Trade::SceneField::Camera)) for(const Containers::Pair<UnsignedInt, UnsignedInt>& cameraReference: scene->camerasAsArray()) {
            const UnsignedInt objectId = cameraReference.first();
            Object3D* const object = _data->objects[objectId].object;
            if(!object) continue;

            /** @todo this doesn't handle objects with multiple
                camera/mesh/light/... assignments correctly */
            _data->objects[objectId].type = "camera";

            if(cameraReference.second() == 0) _data->cameraObject = object;

            /** @todo visualize the camera, not just for the default but for all */
        }

        /* Object orientation visualizers, except for lights, which have their own */
        for(std::size_t i = 0; i != _data->objects.size(); ++i) {
            Object3D* const object = _data->objects[i].object;
            if(!object) continue;

            if(_data->objects[i].lightId != 0xffffffffu) continue;

            new FlatDrawable{*object, flatShader(Shaders::FlatGL3D::Flag::VertexColor), _axisMesh, UnsignedInt(i), 0xffffff_rgbf, Vector3{1.0f}, _data->objectVisualizationDrawables};
        }

        /* Add drawables for objects that have a mesh, again ignoring objects
           that are not part of the hierarchy. There can be multiple mesh
           assignments for one object, simply add one drawable for each. */
        if(scene->hasField(Trade::SceneField::Mesh)) for(const Containers::Pair<UnsignedInt, Containers::Pair<UnsignedInt, Int>>& meshMaterial: scene->meshesMaterialsAsArray()) {
            const UnsignedInt objectId = meshMaterial.first();
            Object3D* const object = _data->objects[objectId].object;
            const UnsignedInt meshId = meshMaterial.second().first();
            const Int materialId = meshMaterial.second().second();
            Containers::Optional<GL::Mesh>& mesh = _data->meshes[meshId].mesh;
            if(!object || !mesh) continue;

            /* Save the mesh pointer as well, so we know what to draw for object
               selection */
            _data->objects[objectId].meshId = meshId;

            Shaders::PhongGL::Flags flags;
            if(hasVertexColors[meshId])
                flags |= Shaders::PhongGL::Flag::VertexColor;
            if(_data->meshes[meshId].hasSeparateBitangents)
                flags |= Shaders::PhongGL::Flag::Bitangent;

            /* Material not available / not loaded. If the mesh has vertex
               colors, use that, otherwise apply a default material; use a flat
               shader for lines / points */
            if(materialId == -1 || !materials[materialId]) {
                if(mesh->primitive() == GL::MeshPrimitive::Triangles ||
                   mesh->primitive() == GL::MeshPrimitive::TriangleStrip ||
                   mesh->primitive() == GL::MeshPrimitive::TriangleFan)
                    new PhongDrawable{*object, phongShader(flags), *mesh, objectId, 0xffffff_rgbf, _shadeless, _data->opaqueDrawables};
                else
                    new FlatDrawable{*object, flatShader(hasVertexColors[meshId] ? Shaders::FlatGL3D::Flag::VertexColor : Shaders::FlatGL3D::Flags{}), *mesh, objectId, 0xffffff_rgbf, Vector3{Constants::nan()}, _data->opaqueDrawables};

            /* Material available */
            } else {
                const Trade::PhongMaterialData& material = *materials[materialId];

                /* Textured material. If the texture failed to load, again just
                   use a default-colored material. */
                GL::Texture2D* diffuseTexture = nullptr;
                GL::Texture2D* normalTexture = nullptr;
                Float normalTextureScale = 1.0f;
                if(material.hasAttribute(Trade::MaterialAttribute::DiffuseTexture)) {
                    Containers::Optional<GL::Texture2D>& texture = _data->textures[material.diffuseTexture()];
                    if(texture) {
                        diffuseTexture = &*texture;
                        flags |= Shaders::PhongGL::Flag::AmbientTexture|
                                 Shaders::PhongGL::Flag::DiffuseTexture;
                        if(material.hasTextureTransformation())
                            flags |= Shaders::PhongGL::Flag::TextureTransformation;
                        if(material.alphaMode() == Trade::MaterialAlphaMode::Mask)
                            flags |= Shaders::PhongGL::Flag::AlphaMask;
                    }
                }

                /* Normal textured material. If the textures fail to load,
                   again just use a default-colored material. */
                if(material.hasAttribute(Trade::MaterialAttribute::NormalTexture)) {
                    Containers::Optional<GL::Texture2D>& texture = _data->textures[material.normalTexture()];
                    /* If there are no tangents, the mesh would render all
                       black. Ignore the normal map in that case. */
                    /** @todo generate tangents instead once we have the algo */
                    if(!_data->meshes[meshId].hasTangents) {
                        Warning{} << "Mesh" << _data->meshes[meshId].name << "doesn't have tangents and Magnum can't generate them yet, ignoring a normal map";
                    } else if(texture) {
                        normalTexture = &*texture;
                        normalTextureScale = material.normalTextureScale();
                        flags |= Shaders::PhongGL::Flag::NormalTexture;
                        if(material.hasTextureTransformation())
                            flags |= Shaders::PhongGL::Flag::TextureTransformation;
                    }
                }

                new PhongDrawable{*object, phongShader(flags),
                    *mesh, objectId,
                    material.diffuseColor(), diffuseTexture, normalTexture, normalTextureScale,
                    material.alphaMask(), material.commonTextureMatrix(), _shadeless,
                    material.alphaMode() == Trade::MaterialAlphaMode::Blend ?
                        _data->transparentDrawables : _data->opaqueDrawables};
            }
        }

    /* The format has no scene support, display just the first loaded mesh with
       a default material and be done with it */
    } else if(!_data->meshes.isEmpty() && _data->meshes[0].mesh) {
        Debug{} << "No scene, loading the first mesh";

        _data->objects = Containers::Array<ObjectInfo>{ValueInit, 1};
        _data->objects[0].object = &_data->scene;
        _data->objects[0].meshId = 0;
        _data->objects[0].name = "object #0";
        new PhongDrawable{_data->scene, phongShader(hasVertexColors[0] ? Shaders::PhongGL::Flag::VertexColor : Shaders::PhongGL::Flags{}), *_data->meshes[0].mesh, 0, 0xffffff_rgbf, _shadeless, _data->opaqueDrawables};
    }

    /* Create a camera object in case it wasn't present in the scene already */
    if(!_data->cameraObject) {
        _data->cameraObject = new Object3D{&_data->scene};
        _data->cameraObject->translate(Vector3::zAxis(5.0f));
    }

    /* Create default camera-relative lights in case they weren't present in
       the scene already. Don't add any visualization for those. */
    if(_data->lightCount == 0) {
        _data->lightCount = 3;

        Object3D* first = new Object3D{_data->cameraObject};
        first->translate({10.0f, 10.0f, 10.0f});
        new LightDrawable{*first, true, _data->lightPositions, _data->lightDrawables};

        Object3D* second = new Object3D{_data->cameraObject};
        first->translate(Vector3{-5.0f, -5.0f, 10.0f}*100.0f);
        new LightDrawable{*second, true, _data->lightPositions, _data->lightDrawables};

        Object3D* third = new Object3D{_data->cameraObject};
        third->translate(Vector3{0.0f, 10.0f, -10.0f}*100.0f);
        new LightDrawable{*third, true, _data->lightPositions, _data->lightDrawables};

        _data->lightColors = Containers::array({
            0xffffff_rgbf,
            0xffcccc_rgbf,
            0xccccff_rgbf
        });
    }

    /* Initialize light colors for all instantiated shaders */
    updateLightColorBrightness();

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
            } else CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        }
        _data->animationData = animation->release();

        /* Load only the first animation at the moment */
        break;
    }

    /* Populate the model info */
    _baseUiPlane->modelInfo.setText(_data->modelInfo = Utility::formatString(
        "{}: {} objs, {} cams, {} meshes, {} mats, {}/{} texs, {} anims",
        /* Apparently STL doesn't fail if substr count is past the end, so
           abuse that to shorten overly long names */
        /** @todo trash fire!! this whole thing is a trash fire */
        std::string{Utility::Path::split(filename).second()}.substr(0, 32),
        importer.objectCount(),
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

void FlatDrawable::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) {
    /* Override the inherited scale, if requested */
    Matrix4 transformation;
    if(_scale == _scale) transformation =
        Matrix4::from(transformationMatrix.rotationShear(), transformationMatrix.translation())*
        Matrix4::scaling(Vector3{_scale});
    else transformation = transformationMatrix;

    _shader
        .setColor(_color)
        .setTransformationProjectionMatrix(camera.projectionMatrix()*transformation)
        .setObjectId(_objectId)
        .draw(_mesh);
}

void PhongDrawable::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) {
    _shader
        .setTransformationMatrix(transformationMatrix)
        .setNormalMatrix(transformationMatrix.normalMatrix())
        .setProjectionMatrix(camera.projectionMatrix())
        .setObjectId(_objectId);

    if(_diffuseTexture) _shader
        .bindAmbientTexture(*_diffuseTexture)
        .bindDiffuseTexture(*_diffuseTexture);
    if(_normalTexture) _shader
        .bindNormalTexture(*_normalTexture)
        .setNormalTextureScale(_normalTextureScale);

    if(_shadeless) _shader
        .setAmbientColor(_color)
        .setDiffuseColor(0x00000000_rgbaf)
        .setSpecularColor(0x00000000_rgbaf);
    else _shader
        .setAmbientColor(_color*0.06f)
        .setDiffuseColor(_color)
        .setSpecularColor(0x11111100_rgbaf);

    if(_shader.flags() & Shaders::PhongGL::Flag::TextureTransformation)
        _shader.setTextureMatrix(_textureMatrix);
    if(_shader.flags() & Shaders::PhongGL::Flag::AlphaMask)
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
    if(_shader->flags() & Shaders::MeshVisualizerGL3D::Flag::NormalDirection)
        _shader->setNormalMatrix(transformationMatrix.normalMatrix());
    #endif

    if(_shader->flags() & (Shaders::MeshVisualizerGL3D::Flag::InstancedObjectId|
        Shaders::MeshVisualizerGL3D::Flag::VertexId
        #ifndef MAGNUM_TARGET_GLES
        |Shaders::MeshVisualizerGL3D::Flag::PrimitiveId
        #endif
    ))
        _shader->setColor(0xffffffff_rgbaf*(_shadeless ? 1.0f : 0.66667f));
    else
        _shader->setColor(0x2f83ccff_rgbaf*0.5f);

    if(_shader->flags() & Shaders::MeshVisualizerGL3D::Flag::InstancedObjectId)
        _shader->setColorMapTransformation(0.0f, 1.0f/_objectIdCount);
    if(_shader->flags() & Shaders::MeshVisualizerGL3D::Flag::VertexId)
        _shader->setColorMapTransformation(0.0f, 1.0f/_vertexCount);
    #ifndef MAGNUM_TARGET_GLES
    if(_shader->flags() & Shaders::MeshVisualizerGL3D::Flag::PrimitiveId)
        _shader->setColorMapTransformation(0.0f, 1.0f/_primitiveCount);
    #endif

    _shader->draw(_mesh);

    GL::Renderer::setPolygonOffset(0.0f, 0.0f);
    GL::Renderer::disable(GL::Renderer::Feature::PolygonOffsetFill);
}

void ScenePlayer::drawEvent() {
    _profiler.beginFrame();

    /* Another FB could be bound from a depth / object ID read (moreover with
       color output disabled), set it back to the default framebuffer */
    GL::defaultFramebuffer.bind(); /** @todo mapForDraw() should bind implicitly */
    GL::defaultFramebuffer
        .mapForDraw({{Shaders::PhongGL::ColorOutput, GL::DefaultFramebuffer::DrawAttachment::Back}})
        .clear(GL::FramebufferClear::Color|GL::FramebufferClear::Depth);

    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);

    if(_data) {
        _data->player.advance(std::chrono::system_clock::now().time_since_epoch());

        /* Calculate light positions first, upload them to all shaders -- all
           of them are there only if they are actually used, so it's not doing
           any wasteful work */
        arrayResize(_data->lightPositions, 0);
        _data->camera->draw(_data->lightDrawables);
        CORRADE_INTERNAL_ASSERT(_data->lightPositions.size() == _data->lightCount);
        for(auto&& shader: _phongShaders)
            shader.second.setLightPositions(_data->lightPositions);

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

        /* Draw object visualization w/o a depth buffer */
        if(_data->visualizeObjects) {
            GL::Renderer::disable(GL::Renderer::Feature::DepthTest);
            _data->camera->draw(_data->objectVisualizationDrawables);
            GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
        }
    }

    /* Don't profile UI drawing */
    _profiler.endFrame();
    _profiler.printStatistics(_profilerOut, 10);

    /* Draw the UI. Disable the depth buffer and enable premultiplied alpha
       blending. */
    if(_drawUi) {
        GL::Renderer::disable(GL::Renderer::Feature::DepthTest);
        GL::Renderer::enable(GL::Renderer::Feature::Blending);
        GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::One, GL::Renderer::BlendFunction::OneMinusSourceAlpha);
        _ui->draw();
        GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::One, GL::Renderer::BlendFunction::Zero);
        GL::Renderer::disable(GL::Renderer::Feature::Blending);
        GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    }

    /* Schedule a redraw only if profiling is enabled or the player is playing
       to avoid hogging the CPU */
    if(_profiler.isEnabled() || (_data && _data->player.state() == Animation::State::Playing))
        redraw();

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

    for(auto& i: _meshVisualizerShaders)
        i.second.setViewportSize(Vector2{event.framebufferSize()});

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
    _reinterpretShader.draw(_fullscreenTriangle);
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

        Quaternion rotation{NoInit};
        if(event.key() == KeyEvent::Key::NumSeven) /* Top/bottom */
            rotation = Quaternion::rotation(-90.0_degf*multiplier, Vector3::xAxis());
        else if(event.key() == KeyEvent::Key::NumOne) /* Front/back */
            rotation = Quaternion::rotation(90.0_degf - 90.0_degf*multiplier, Vector3::yAxis());
        else if(event.key() == KeyEvent::Key::NumThree) /* Right/left */
            rotation = Quaternion::rotation(90.0_degf*multiplier, Vector3::yAxis());
        else CORRADE_INTERNAL_ASSERT_UNREACHABLE();

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
        updateLightColorBrightness();

    /* Toggle profiling */
    } else if(event.key() == KeyEvent::Key::P) {
        _profiler.isEnabled() ? _profiler.disable() : _profiler.enable();

    } else return;

    event.setAccepted();
    redraw();
}

void ScenePlayer::mousePressEvent(MouseEvent& event) {
    if(_drawUi && _ui->handlePressEvent(event.position())) {
        redraw();
        event.setAccepted();
        return;
    }

    /* RMB to select */
    if(event.button() == MouseEvent::Button::Right && _data) {
        _selectionFramebuffer.bind(); /** @todo mapForDraw() should bind implicitly */
        _selectionFramebuffer.mapForDraw({
                {Shaders::GenericGL3D::ColorOutput, GL::Framebuffer::DrawAttachment::None},
                {Shaders::GenericGL3D::ObjectIdOutput, GL::Framebuffer::ColorAttachment{1}}})
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

        /* Draw object visualization w/o a depth buffer */
        if(_data->visualizeObjects) {
            GL::Renderer::disable(GL::Renderer::Feature::DepthTest);
            _data->camera->draw(_data->objectVisualizationDrawables);
            GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
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

        /* Show either global or object-specific widgets */
        Ui::Widget::setVisible(selectedId < _data->objects.size(), {
            _baseUiPlane->objectInfo,
            _baseUiPlane->meshVisualization
        });
        Ui::Widget::setVisible(selectedId >= _data->objects.size(), {
            _baseUiPlane->modelInfo,
            _baseUiPlane->objectVisualization
        });

        /* If nothing is selected, the global info is shown */
        if(selectedId >= _data->objects.size()) {
            /* 0xffff is the background, but anything else is just wrong */
            if(selectedId != 0xffff)
                Warning{} << "Selected ID" << selectedId << "out of bounds for" << _data->objects.size() << "objects, ignoring";

        /* Otherwise add a visualizer and update the info */
        } else {
            CORRADE_INTERNAL_ASSERT(!_data->selectedObject);
            CORRADE_INTERNAL_ASSERT(selectedId < _data->objects.size());
            CORRADE_INTERNAL_ASSERT(_data->objects[selectedId].object);

            ObjectInfo& objectInfo = _data->objects[selectedId];

            /* A mesh is selected */
            if(_data->objects[selectedId].meshId != 0xffffffffu) {
                CORRADE_INTERNAL_ASSERT(_data->meshes[_data->objects[selectedId].meshId].mesh);
                MeshInfo& meshInfo = _data->meshes[_data->objects[selectedId].meshId];

                /* Create a visualizer for the selected object */
                const Shaders::MeshVisualizerGL3D::Flags flags = setupVisualization(_data->objects[selectedId].meshId);
                _data->selectedObject = new MeshVisualizerDrawable{
                    *objectInfo.object, meshVisualizerShader(flags),
                    *meshInfo.mesh, _data->objects[selectedId].meshId,
                    meshInfo.objectIdCount, meshInfo.vertices, meshInfo.primitives,
                    _shadeless, _data->selectedObjectDrawables};

                /* Show mesh info */
                _baseUiPlane->objectInfo.setText(_data->objectInfo = Utility::formatString(
                    "{}: mesh {}, indexed, {} attribs, {} verts, {} prims, {:.1f} kB",
                    objectInfo.name,
                    meshInfo.name,
                    meshInfo.attributes,
                    meshInfo.vertices,
                    meshInfo.primitives,
                    meshInfo.size/1024.0f).substr(0, 128) /** @todo fix in the Ui library */);

            /* A light is selected */
            } else if(_data->objects[selectedId].lightId != 0xffffffffu) {
                CORRADE_INTERNAL_ASSERT(_data->lights[_data->objects[selectedId].lightId].light);
                LightInfo& lightInfo = _data->lights[_data->objects[selectedId].lightId];

                _baseUiPlane->objectInfo.setText(_data->objectInfo = Utility::formatString(
                    "{}: {} {}, range {}, intensity {}",
                    objectInfo.name,
                    lightInfo.type,
                    lightInfo.name,
                    lightInfo.light->range(),
                    lightInfo.light->intensity()).substr(0, 128) /** @todo fix in the Ui library */);

            /* Something else is selected from object visualization, display
               just generic info */
            } else {
                _baseUiPlane->objectInfo.setText(_data->objectInfo = Utility::formatString(
                    "{}: {}, {} children",
                    objectInfo.name,
                    objectInfo.type,
                    objectInfo.childCount).substr(0, 128) /** @todo fix in the Ui library */);
            }
        }

        event.setAccepted();
        redraw();
        return;
    }

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
    if(_drawUi && _ui->handleReleaseEvent(event.position())) {
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

    if(_drawUi && _ui->handleMoveEvent(event.position())) {
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

    #ifndef MAGNUM_TARGET_GLES
    /* Adjust TBN visualization length with Ctrl-scroll if it's currently shown */
    if((event.modifiers() & MouseScrollEvent::Modifier::Ctrl) && _data->selectedObject && (_data->selectedObject->shader().flags() & Shaders::MeshVisualizerGL3D::Flag::NormalDirection)) {
        _lineLength = Math::max(_lineLength *= (1.0f + event.offset().y()*0.1f), 0.0f);
        _data->selectedObject->shader().setLineLength(_lineLength);
        event.setAccepted();
        redraw();
        return;
    }
    #endif

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

Containers::Pointer<AbstractPlayer> createScenePlayer(Platform::ScreenedApplication& application, Ui::UserInterface& uiToStealFontFrom, const DebugTools::FrameProfilerGL::Values profilerValues, bool& drawUi) {
    return Containers::Pointer<ScenePlayer>{InPlaceInit, application, uiToStealFontFrom, profilerValues, drawUi};
}

}}
