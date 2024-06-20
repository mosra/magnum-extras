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

#include "BaseLayerGL.h"

#include <Corrade/Containers/BitArrayView.h>
#include <Corrade/Containers/Iterable.h>
#include <Corrade/Containers/String.h>
#include <Corrade/Utility/Format.h>
#include <Corrade/Utility/Resource.h>
#include <Magnum/Math/Matrix3.h>
#include <Magnum/Math/Range.h>
#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/Context.h>
#include <Magnum/GL/Extensions.h>
#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/TextureArray.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/GL/Version.h>

#include "Magnum/Whee/RendererGL.h"
#include "Magnum/Whee/Implementation/baseLayerState.h"
#include "Magnum/Whee/Implementation/blurCoefficients.h"
#include "Magnum/Whee/Implementation/BlurShaderGL.h"

#ifdef MAGNUM_BUILD_STATIC
static void importShaderResources() {
    CORRADE_RESOURCE_INITIALIZE(MagnumWhee_RESOURCES)
}
#endif

namespace Magnum { namespace Whee {

using namespace Containers::Literals;

namespace {

class BaseShaderGL: public GL::AbstractShaderProgram {
    private:
        enum: Int {
            StyleBufferBinding = 0,
            TextureBinding = 0,
            BackgroundBlurTextureBinding = 1,
        };

    public:
        enum Flag: UnsignedByte {
            Textured = 1 << 0,
            BackgroundBlur = 1 << 1,
            NoRoundedCorners = 1 << 2,
            NoOutline = 1 << 3,
            TextureMask = 1 << 4,
        };

        typedef Containers::EnumSet<Flag> Flags;

        typedef GL::Attribute<0, Vector2> Position;
        typedef GL::Attribute<1, Vector2> CenterDistance;
        typedef GL::Attribute<2, Vector4> OutlineWidth;
        typedef GL::Attribute<3, Vector3> Color3;
        typedef GL::Attribute<4, UnsignedInt> Style;
        typedef GL::Attribute<5, Vector3> TextureCoordinates;

        explicit BaseShaderGL(UnsignedInt styleCount);
        explicit BaseShaderGL(Flags flags, UnsignedInt styleCount);

        BaseShaderGL& setTransformationProjectionMatrix(const Matrix3& matrix) {
            setUniform(_transformationProjectionMatrixUniform, matrix);
            return *this;
        }

        BaseShaderGL& bindStyleBuffer(GL::Buffer& buffer) {
            buffer.bind(GL::Buffer::Target::Uniform, StyleBufferBinding);
            return *this;
        }

        BaseShaderGL& bindTexture(GL::Texture2DArray& texture) {
            CORRADE_INTERNAL_ASSERT(_flags & Flag::Textured);
            texture.bind(TextureBinding);
            return *this;
        }

        BaseShaderGL& bindBackgroundBlurTexture(GL::Texture2D& texture) {
            CORRADE_INTERNAL_ASSERT(_flags & Flag::BackgroundBlur);
            texture.bind(BackgroundBlurTextureBinding);
            return *this;
        }

    private:
        Flags _flags;
        Int _transformationProjectionMatrixUniform = 0;
};

#ifdef CORRADE_TARGET_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#endif
CORRADE_ENUMSET_OPERATORS(BaseShaderGL::Flags)
#ifdef CORRADE_TARGET_CLANG
#pragma clang diagnostic pop
#endif

BaseShaderGL::BaseShaderGL(const Flags flags, const UnsignedInt styleCount): _flags{flags} {
    GL::Context& context = GL::Context::current();
    #ifndef MAGNUM_TARGET_GLES
    MAGNUM_ASSERT_GL_EXTENSION_SUPPORTED(GL::Extensions::ARB::explicit_attrib_location);
    #endif

    #ifdef MAGNUM_BUILD_STATIC
    if(!Utility::Resource::hasGroup("MagnumWhee"_s))
        importShaderResources();
    #endif

    Utility::Resource rs{"MagnumWhee"_s};

    const GL::Version version = context.supportedVersion({
        #ifndef MAGNUM_TARGET_GLES
        GL::Version::GL330
        #else
        GL::Version::GLES300
            #ifndef MAGNUM_TARGET_WEBGL
            , GL::Version::GLES310
            #endif
        #endif
    });

    GL::Shader vert{version, GL::Shader::Type::Vertex};
    vert.addSource(Utility::format("#define STYLE_COUNT {}\n", styleCount))
        .addSource(flags & Flag::BackgroundBlur ? "#define BACKGROUND_BLUR\n"_s : ""_s)
        .addSource(flags & Flag::Textured ? "#define TEXTURED\n"_s : ""_s)
        .addSource(flags & Flag::NoOutline ? "#define NO_OUTLINE\n"_s : ""_s)
        .addSource(rs.getString("compatibility.glsl"_s))
        .addSource(rs.getString("BaseShader.vert"_s));

    GL::Shader frag{version, GL::Shader::Type::Fragment};
    frag.addSource(Utility::format("#define STYLE_COUNT {}\n", styleCount))
        .addSource(flags & Flag::BackgroundBlur ? "#define BACKGROUND_BLUR\n"_s : ""_s)
        .addSource(flags & Flag::Textured ? "#define TEXTURED\n"_s : ""_s)
        .addSource(flags & Flag::NoRoundedCorners ? "#define NO_ROUNDED_CORNERS\n"_s : ""_s)
        .addSource(flags & Flag::NoOutline ? "#define NO_OUTLINE\n"_s : ""_s)
        .addSource(flags & Flag::TextureMask ? "#define TEXTURE_MASK\n"_s : ""_s)
        .addSource(rs.getString("compatibility.glsl"_s))
        .addSource(rs.getString("BaseShader.frag"_s));

    CORRADE_INTERNAL_ASSERT(vert.compile() && frag.compile());

    attachShaders({vert, frag});
    CORRADE_INTERNAL_ASSERT(link());

    #ifndef MAGNUM_TARGET_GLES
    if(!context.isExtensionSupported<GL::Extensions::ARB::explicit_uniform_location>())
    #elif !defined(MAGNUM_TARGET_GLES2) && !defined(MAGNUM_TARGET_WEBGL)
    if(version < GL::Version::GLES310)
    #endif
    {
        _transformationProjectionMatrixUniform = uniformLocation("transformationProjectionMatrix"_s);
    }

    #ifndef MAGNUM_TARGET_GLES
    if(!context.isExtensionSupported<GL::Extensions::ARB::shading_language_420pack>())
    #elif !defined(MAGNUM_TARGET_GLES2) && !defined(MAGNUM_TARGET_WEBGL)
    if(version < GL::Version::GLES310)
    #endif
    {
        if(flags & Flag::Textured)
            setUniform(uniformLocation("textureData"_s), TextureBinding);
        if(flags & Flag::BackgroundBlur)
            setUniform(uniformLocation("backgroundBlurTextureData"_s), BackgroundBlurTextureBinding);
        setUniformBlockBinding(uniformBlockIndex("Style"_s), StyleBufferBinding);
    }
}

}

/* The BlurShaderGL is exported for easier testing, so no anonymous
   namespace. The class definition is in Implementation/BlurShaderGL.h. */
BlurShaderGL::BlurShaderGL(UnsignedInt radius, Float limit) {
    using namespace Containers::Literals;

    GL::Context& context = GL::Context::current();
    #ifndef MAGNUM_TARGET_GLES
    MAGNUM_ASSERT_GL_EXTENSION_SUPPORTED(GL::Extensions::ARB::explicit_attrib_location);
    #endif

    #ifdef MAGNUM_BUILD_STATIC
    if(!Utility::Resource::hasGroup("MagnumWhee"_s))
        importShaderResources();
    #endif

    Utility::Resource rs{"MagnumWhee"_s};

    const GL::Version version = context.supportedVersion({
        #ifndef MAGNUM_TARGET_GLES
        GL::Version::GL330
        #else
        GL::Version::GLES300
            #ifndef MAGNUM_TARGET_WEBGL
            , GL::Version::GLES310
            #endif
        #endif
    });

    Float discrete[32];
    const std::size_t count = blurCoefficientsInto(Containers::arrayView(discrete).prefix(radius + 1), limit);
    CORRADE_INTERNAL_ASSERT(count);
    const std::size_t interpolatedCount = (count + 1)/2;

    /* Save the actual sample count, it'll be used to decide whether there are
       any direction-dependent samples (for which a direction uniform has to be
       passed) */
    _sampleCount = count;

    Float weights[16];
    Float offsets[16];
    interpolateBlurCoefficientsInto(
        Containers::arrayView(discrete).prefix(count),
        Containers::arrayView(weights).prefix(interpolatedCount),
        Containers::arrayView(offsets).prefix(interpolatedCount));

    GL::Shader vert{version, GL::Shader::Type::Vertex};
    vert.addSource(rs.getString("compatibility.glsl"_s))
        .addSource(rs.getString("BlurShader.vert"_s));

    /* A suffix (*not* prefix, to have a null terminator) is taken for
       formatting placeholders. Explicitly formatting as fixed-precision
       floating-point to avoid 1.0 and 0.0 being formatted as 1 and 0, causing
       a type mismatch on GLSL ES. */
    constexpr Containers::StringView placeholders =
        "{:f}, {:f}, {:f}, {:f}, {:f}, {:f}, {:f}, {:f}, "
        "{:f}, {:f}, {:f}, {:f}, {:f}, {:f}, {:f}, {:f}"_s;
    const Containers::StringView placeholdersSuffix =
        placeholders.exceptPrefix(placeholders.size() - ((interpolatedCount - 1)*6 + 4));

    GL::Shader frag{version, GL::Shader::Type::Fragment};
    frag.addSource(rs.getString("compatibility.glsl"_s))
        .addSource(Utility::format(
            "#define COUNT {0}\n"
            "const highp float weights[{0}] = float[]({1});\n"
            "const highp float offsets[{0}] = float[]({2});\n",
            interpolatedCount,
            /* This takes only the first `interpolatedCount` elements from the
               lists, ignoring the rest */
            /** @todo use suffix() once it takes suffix length, not prefix
                length */
            Utility::format(placeholdersSuffix.data(),
                weights[ 0], weights[ 1], weights[ 2], weights[ 3],
                weights[ 4], weights[ 5], weights[ 6], weights[ 7],
                weights[ 8], weights[ 9], weights[10], weights[11],
                weights[12], weights[13], weights[14], weights[15]),
            /** @todo use suffix() once it takes suffix length, not prefix
                length */
            Utility::format(placeholdersSuffix.data(),
                offsets[ 0], offsets[ 1], offsets[ 2], offsets[ 3],
                offsets[ 4], offsets[ 5], offsets[ 6], offsets[ 7],
                offsets[ 8], offsets[ 9], offsets[10], offsets[11],
                offsets[12], offsets[13], offsets[14], offsets[15])))
        .addSource(count % 2 == 1 ? "#define FIRST_TAP_AT_CENTER\n"_s : ""_s)
        .addSource(rs.getString("BlurShader.frag"_s));

    CORRADE_INTERNAL_ASSERT(vert.compile() && frag.compile());

    attachShaders({vert, frag});
    CORRADE_INTERNAL_ASSERT(link());

    #ifndef MAGNUM_TARGET_GLES
    if(!context.isExtensionSupported<GL::Extensions::ARB::explicit_uniform_location>())
    #elif !defined(MAGNUM_TARGET_GLES2) && !defined(MAGNUM_TARGET_WEBGL)
    if(version < GL::Version::GLES310)
    #endif
    {
        /* For a zero radius we check just the center pixel, the direction
           isn't used by the shader at all. Originally it was queried always
           but some shader compilers DCE the access and some not, leading to
           "location of uniform 'direction' cannot be retrieved" warnings being
           printed to the console on certain systems, so it's instead compiled
           out always for a zero radius. */
        if(_sampleCount != 1)
            _directionUniform = uniformLocation("direction"_s);
    }

    #ifndef MAGNUM_TARGET_GLES
    if(!context.isExtensionSupported<GL::Extensions::ARB::shading_language_420pack>())
    #elif !defined(MAGNUM_TARGET_GLES2) && !defined(MAGNUM_TARGET_WEBGL)
    if(version < GL::Version::GLES310)
    #endif
    {
        setUniform(uniformLocation("textureData"_s), TextureBinding);
    }
}

struct BaseLayerGL::Shared::State: BaseLayer::Shared::State {
    explicit State(Shared& self, const Configuration& configuration);

    BaseShaderGL shader;
    /* The buffer is NoCreate'd at first to be able to detect whether
       setStyle() was called at all -- it's created in doSetStyle(). In case
       dynamic styles are present, this buffer is unused and each layer has its
       own copy instead. Detection of whether setStyle() was called is then
       done by checking the styleUniforms array, which is empty at first. */
    GL::Buffer styleBuffer{NoCreate};

    /* These are created only if Flag::BackgroundBlur is enabled */
    GL::Texture2D backgroundBlurTextureVertical{NoCreate},
                  backgroundBlurTextureHorizontal{NoCreate};
    GL::Framebuffer backgroundBlurFramebufferVertical{NoCreate},
                    backgroundBlurFramebufferHorizontal{NoCreate};
    BlurShaderGL backgroundBlurShader{NoCreate};
};

BaseLayerGL::Shared::State::State(Shared& self, const Configuration& configuration): BaseLayer::Shared::State{self, configuration}, shader{
    #define _c(flag) (flags >= Flag::flag ? BaseShaderGL::Flag::flag : BaseShaderGL::Flags{})
    _c(BackgroundBlur)|
    _c(Textured)|
    _c(NoRoundedCorners)|
    _c(NoOutline)|
    _c(TextureMask),
    #undef _c
    configuration.styleUniformCount() + configuration.dynamicStyleCount()}
{
    if(configuration.flags() & Flag::BackgroundBlur)
        backgroundBlurShader = BlurShaderGL{configuration.backgroundBlurRadius(), configuration.backgroundBlurCutoff()};
}

BaseLayerGL::Shared::Shared(const Configuration& configuration): BaseLayer::Shared{Containers::pointer<State>(*this, configuration)} {}

BaseLayerGL::Shared::Shared(NoCreateT) noexcept: BaseLayer::Shared{NoCreate} {}

BaseLayerGL::Shared& BaseLayerGL::Shared::setStyle(const BaseLayerCommonStyleUniform& commonUniform, const Containers::ArrayView<const BaseLayerStyleUniform> uniforms, const Containers::StridedArrayView1D<const Vector4>& paddings) {
    return static_cast<Shared&>(BaseLayer::Shared::setStyle(commonUniform, uniforms, paddings));
}

BaseLayerGL::Shared& BaseLayerGL::Shared::setStyle(const BaseLayerCommonStyleUniform& commonUniform, const std::initializer_list<BaseLayerStyleUniform> uniforms, const std::initializer_list<Vector4> paddings) {
    return static_cast<Shared&>(BaseLayer::Shared::setStyle(commonUniform, uniforms, paddings));
}

BaseLayerGL::Shared& BaseLayerGL::Shared::setStyle(const BaseLayerCommonStyleUniform& commonUniform, const Containers::ArrayView<const BaseLayerStyleUniform> uniforms, const Containers::StridedArrayView1D<const UnsignedInt>& styleToUniform, const Containers::StridedArrayView1D<const Vector4>& stylePaddings) {
    return static_cast<Shared&>(BaseLayer::Shared::setStyle(commonUniform, uniforms, styleToUniform, stylePaddings));
}

BaseLayerGL::Shared& BaseLayerGL::Shared::setStyle(const BaseLayerCommonStyleUniform& commonUniform, const std::initializer_list<BaseLayerStyleUniform> uniforms, const std::initializer_list<UnsignedInt> styleToUniform, const std::initializer_list<Vector4> stylePaddings) {
    return static_cast<Shared&>(BaseLayer::Shared::setStyle(commonUniform, uniforms, styleToUniform, stylePaddings));
}

void BaseLayerGL::Shared::doSetStyle(const BaseLayerCommonStyleUniform& commonUniform, const Containers::ArrayView<const BaseLayerStyleUniform> uniforms) {
    /* This function should get called only if the dynamic style count is 0 */
    auto& state = static_cast<State&>(*_state);
    CORRADE_INTERNAL_ASSERT(state.dynamicStyleCount == 0);

    /* The buffer is NoCreate'd at first to be able to detect whether
       setStyle() was called at all */
    if(!state.styleBuffer.id())
        state.styleBuffer = GL::Buffer{GL::Buffer::TargetHint::Uniform, {nullptr, sizeof(BaseLayerCommonStyleUniform) + sizeof(BaseLayerStyleUniform)*state.styleUniformCount}};

    state.styleBuffer.setSubData(0, {&commonUniform, 1});
    state.styleBuffer.setSubData(sizeof(BaseLayerCommonStyleUniform), uniforms);
}

struct BaseLayerGL::State: BaseLayer::State {
    explicit State(Shared::State& shared): BaseLayer::State{shared} {}

    GL::Buffer vertexBuffer{GL::Buffer::TargetHint::Array}, indexBuffer{GL::Buffer::TargetHint::ElementArray};
    GL::Mesh mesh;
    Vector2 clipScale;

    /* Used only if Flag::Textured is enabled. Is non-owning if
       setTexture(GL::Texture2DArray&) was called, owning if
       setTexture(GL::Texture2DArray&&). */
    GL::Texture2DArray texture{NoCreate};

    /* Used only if shared.dynamicStyleCount is non-zero, in which case it's
       created during the first doUpdate(). Even though the size is known in
       advance, the NoCreate'd state is used to correctly perform the first
       ever style upload without having to implicitly set any LayerStates. */
    GL::Buffer styleBuffer{NoCreate};

    /* Used only if Flag::BackgroundBlur is enabled */
    GL::Buffer backgroundBlurVertexBuffer{NoCreate};
    GL::Buffer backgroundBlurIndexBuffer{NoCreate};
    GL::Mesh backgroundBlurMesh{NoCreate};
};

BaseLayerGL::BaseLayerGL(const LayerHandle handle, Shared& sharedState_): BaseLayer{handle, Containers::pointer<State>(static_cast<Shared::State&>(*sharedState_._state))} {
    auto& state = static_cast<State&>(*_state);
    Shared::State& sharedState = static_cast<Shared::State&>(state.shared);
    if(sharedState.flags >= Shared::Flag::Textured) {
        state.mesh.addVertexBuffer(state.vertexBuffer, 0,
            BaseShaderGL::Position{},
            BaseShaderGL::CenterDistance{},
            BaseShaderGL::OutlineWidth{},
            BaseShaderGL::Color3{},
            BaseShaderGL::Style{},
            BaseShaderGL::TextureCoordinates{});
    } else {
        state.mesh.addVertexBuffer(state.vertexBuffer, 0,
            BaseShaderGL::Position{},
            BaseShaderGL::CenterDistance{},
            BaseShaderGL::OutlineWidth{},
            BaseShaderGL::Color3{},
            BaseShaderGL::Style{});
    }
    state.mesh.setIndexBuffer(state.indexBuffer, 0, GL::MeshIndexType::UnsignedInt);

    if(sharedState.flags >= Shared::Flag::BackgroundBlur) {
        state.backgroundBlurVertexBuffer = GL::Buffer{GL::Buffer::TargetHint::Array};
        state.backgroundBlurIndexBuffer = GL::Buffer{GL::Buffer::TargetHint::ElementArray};
        (state.backgroundBlurMesh = GL::Mesh{})
            .addVertexBuffer(state.backgroundBlurVertexBuffer, 0, BlurShaderGL::Position{})
            .setIndexBuffer(state.backgroundBlurIndexBuffer, 0, GL::MeshIndexType::UnsignedInt);
    }
}

BaseLayerGL& BaseLayerGL::setTexture(GL::Texture2DArray& texture) {
    return setTexture(GL::Texture2DArray::wrap(texture.id()));
}

BaseLayerGL& BaseLayerGL::setTexture(GL::Texture2DArray&& texture) {
    auto& state = static_cast<State&>(*_state);
    #ifndef CORRADE_NO_ASSERT
    auto& sharedState = static_cast<Shared::State&>(state.shared);
    #endif
    CORRADE_ASSERT(sharedState.flags & Shared::Flag::Textured,
        "Whee::BaseLayerGL::setTexture(): texturing not enabled", *this);
    state.texture = Utility::move(texture);
    return *this;
}

LayerFeatures BaseLayerGL::doFeatures() const {
    return BaseLayer::doFeatures()|LayerFeature::DrawUsesBlending|LayerFeature::DrawUsesScissor;
}

void BaseLayerGL::doSetSize(const Vector2& size, const Vector2i& framebufferSize) {
    BaseLayer::doSetSize(size, framebufferSize);

    auto& state = static_cast<State&>(*_state);
    auto& sharedState = static_cast<Shared::State&>(state.shared);

    /* The BaseLayer populates the data expecting the origin is top left and Y
       down */
    const Matrix3 projectionMatrix =
        Matrix3::scaling({1.0f, -1.0f})*
        Matrix3::translation({-1.0f, -1.0f})*
        Matrix3::projection(size);
    sharedState.shader.setTransformationProjectionMatrix(projectionMatrix);

    /* For scaling and Y-flipping the clip rects in doDraw() */
    state.clipScale = Vector2{framebufferSize}/size;

    if(sharedState.flags & Shared::Flag::BackgroundBlur) {
        (sharedState.backgroundBlurTextureVertical = GL::Texture2D{})
            .setWrapping(GL::SamplerWrapping::ClampToEdge)
            .setStorage(1, GL::TextureFormat::RGBA8, framebufferSize);
        (sharedState.backgroundBlurTextureHorizontal = GL::Texture2D{})
            .setWrapping(GL::SamplerWrapping::ClampToEdge)
            .setStorage(1, GL::TextureFormat::RGBA8, framebufferSize);

        (sharedState.backgroundBlurFramebufferVertical = GL::Framebuffer{{{}, framebufferSize}})
            .attachTexture(GL::Framebuffer::ColorAttachment{0}, sharedState.backgroundBlurTextureVertical, 0);
        (sharedState.backgroundBlurFramebufferHorizontal = GL::Framebuffer{{{}, framebufferSize}})
            .attachTexture(GL::Framebuffer::ColorAttachment{0}, sharedState.backgroundBlurTextureHorizontal, 0);
    }
}

void BaseLayerGL::doUpdate(const LayerStates states, const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, const Containers::BitArrayView nodesEnabled, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes, const Containers::StridedArrayView1D<const Vector2>& compositeRectOffsets, const Containers::StridedArrayView1D<const Vector2>& compositeRectSizes) {
    State& state = static_cast<State&>(*_state);
    Shared::State& sharedState = static_cast<Shared::State&>(state.shared);

    /* Check whether the shared styles changed before calling into the base
       doUpdate() that syncs the stamps. For dynamic styles, if the style
       changed, it should be accompanied by NeedsCommonDataUpdate being set in
       order to be correctly handled below. */
    const bool sharedStyleChanged = sharedState.styleUpdateStamp != state.styleUpdateStamp;
    CORRADE_INTERNAL_ASSERT(!sharedState.dynamicStyleCount || (!sharedStyleChanged && !state.dynamicStyleChanged) || states >= LayerState::NeedsCommonDataUpdate);

    BaseLayer::doUpdate(states, dataIds, clipRectIds, clipRectDataCounts, nodeOffsets, nodeSizes, nodesEnabled, clipRectOffsets, clipRectSizes, compositeRectOffsets, compositeRectSizes);

    /* The branching here mirrors how BaseLayer::doUpdate() restricts the
       updates */
    if(states >= LayerState::NeedsNodeOrderUpdate ||
       states >= LayerState::NeedsDataUpdate)
    {
        state.indexBuffer.setData(state.indices);
        state.mesh.setCount(state.indices.size());
    }
    if(states >= LayerState::NeedsNodeOffsetSizeUpdate ||
       states >= LayerState::NeedsNodeEnabledUpdate ||
       states >= LayerState::NeedsDataUpdate)
    {
        state.vertexBuffer.setData(state.vertices);
    }
    if(states >= LayerState::NeedsCompositeOffsetSizeUpdate && sharedState.flags & Shared::Flag::BackgroundBlur) {
        state.backgroundBlurIndexBuffer.setData(state.backgroundBlurIndices);
        state.backgroundBlurVertexBuffer.setData(state.backgroundBlurVertices);
        state.backgroundBlurMesh.setCount(state.backgroundBlurIndices.size());
    }

    /* If we have dynamic styles and either NeedsCommonDataUpdate is set
       (meaning either the static style or the dynamic style changed) or
       they haven't been uploaded yet at all, upload them. */
    if(sharedState.dynamicStyleCount && ((states >= LayerState::NeedsCommonDataUpdate) || !state.styleBuffer.id())) {
        const bool needsFirstUpload = !state.styleBuffer.id();
        if(needsFirstUpload) {
            /** @todo check if DynamicDraw has any effect on perf */
            state.styleBuffer = GL::Buffer{GL::Buffer::TargetHint::Uniform, {nullptr, sizeof(BaseLayerCommonStyleUniform) + sizeof(BaseLayerStyleUniform)*(sharedState.styleUniformCount + sharedState.dynamicStyleCount)}, GL::BufferUsage::DynamicDraw};
        }
        if(needsFirstUpload || sharedStyleChanged) {
            state.styleBuffer.setSubData(0, {&sharedState.commonStyleUniform, 1});
            state.styleBuffer.setSubData(sizeof(BaseLayerCommonStyleUniform), sharedState.styleUniforms);
        }
        if(needsFirstUpload || state.dynamicStyleChanged) {
            state.styleBuffer.setSubData(sizeof(BaseLayerCommonStyleUniform) + sizeof(BaseLayerStyleUniform)*sharedState.styleUniformCount, state.dynamicStyleUniforms);
            state.dynamicStyleChanged = false;
        }
    }
}

void BaseLayerGL::doComposite(AbstractRenderer& renderer, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, std::size_t offset, std::size_t count) {
    State& state = static_cast<State&>(*_state);
    Shared::State& sharedState = static_cast<Shared::State&>(state.shared);
    RendererGL& rendererGL = static_cast<RendererGL&>(renderer);

    state.backgroundBlurMesh
        .setIndexOffset(offset*6)
        .setCount(count*6);

    /* Perform the blur in as many passes as desired. For the first pass the
       input is the compositing framebuffer texture, successive passes take
       output of the previous horizontal blur for the next vertical blur. */
    GL::Texture2D* input = &rendererGL.compositingTexture();
    for(UnsignedInt i = 0; i != state.backgroundBlurPassCount; ++i) {
        sharedState.backgroundBlurFramebufferVertical.bind();
        sharedState.backgroundBlurShader
            .setDirection(Vector2::yAxis(1.0f/state.framebufferSize.y()))
            .bindTexture(*input)
            .draw(state.backgroundBlurMesh);

        sharedState.backgroundBlurFramebufferHorizontal.bind();
        sharedState.backgroundBlurShader
            .setDirection(Vector2::xAxis(1.0f/state.framebufferSize.x()))
            .bindTexture(sharedState.backgroundBlurTextureVertical)
            .draw(state.backgroundBlurMesh);

        input = &sharedState.backgroundBlurTextureHorizontal;
    }
}

void BaseLayerGL::doDraw(const Containers::StridedArrayView1D<const UnsignedInt>&, const std::size_t offset, const std::size_t count, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, const std::size_t clipRectOffset, const std::size_t clipRectCount, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, Containers::BitArrayView, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes) {
    auto& state = static_cast<State&>(*_state);
    CORRADE_ASSERT(!state.framebufferSize.isZero() && !state.clipScale.isZero(),
        "Whee::BaseLayerGL::draw(): user interface size wasn't set", );

    auto& sharedState = static_cast<Shared::State&>(state.shared);
    /* With dynamic styles, Shared::setStyle() fills styleUniforms instead of
       creating the styleBuffer */
    CORRADE_ASSERT(
        (!sharedState.dynamicStyleCount && sharedState.styleBuffer.id()) ||
        (sharedState.dynamicStyleCount && !sharedState.styleUniforms.isEmpty()),
        "Whee::BaseLayerGL::draw(): no style data was set", );
    CORRADE_ASSERT(!(sharedState.flags & Shared::Flag::Textured) || state.texture.id(),
        "Whee::BaseLayerGL::draw(): no texture to draw with was set", );

    /* If there are dynamic styles, bind the layer-specific buffer that
       contains them, otherwise bind the shared buffer */
    sharedState.shader.bindStyleBuffer(sharedState.dynamicStyleCount ?
        state.styleBuffer : sharedState.styleBuffer);

    if(sharedState.flags & Shared::Flag::Textured)
        sharedState.shader.bindTexture(state.texture);
    if(sharedState.flags & Shared::Flag::BackgroundBlur)
        sharedState.shader.bindBackgroundBlurTexture(sharedState.backgroundBlurTextureHorizontal);

    std::size_t clipDataOffset = offset;
    for(std::size_t i = 0; i != clipRectCount; ++i) {
        const UnsignedInt clipRectId = clipRectIds[clipRectOffset + i];
        const UnsignedInt clipRectDataCount = clipRectDataCounts[clipRectOffset + i];
        const Vector2i clipRectOffset_ = Vector2i{clipRectOffsets[clipRectId]*state.clipScale};
        const Vector2i clipRectSize = clipRectSizes[clipRectId].isZero() ?
            state.framebufferSize : Vector2i{clipRectSizes[clipRectId]*state.clipScale};

        GL::Renderer::setScissor(Range2Di::fromSize(
            {clipRectOffset_.x(), state.framebufferSize.y() - clipRectOffset_.y() - clipRectSize.y()},
            clipRectSize));

        state.mesh
            .setIndexOffset(clipDataOffset*6)
            .setCount(clipRectDataCount*6);
        sharedState.shader
            .draw(state.mesh);

        clipDataOffset += clipRectDataCount;
    }

    CORRADE_INTERNAL_ASSERT(clipDataOffset == offset + count);
    #ifdef CORRADE_NO_ASSERT
    static_cast<void>(count);
    #endif
}

}}
