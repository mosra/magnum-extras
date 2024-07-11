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

#include "TextLayerGL.h"

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
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/Version.h>
#include <Magnum/Text/GlyphCache.h>

#include "Magnum/Whee/Implementation/textLayerState.h"

#ifdef MAGNUM_BUILD_STATIC
static void importShaderResources() {
    CORRADE_RESOURCE_INITIALIZE(MagnumWhee_RESOURCES)
}
#endif

namespace Magnum { namespace Whee {

using namespace Containers::Literals;

namespace {

class TextShaderGL: public GL::AbstractShaderProgram {
    private:
        enum: Int {
            GlyphTextureBinding = 0,
            StyleBufferBinding = 0
        };

    public:
        typedef GL::Attribute<0, Vector2> Position;
        typedef GL::Attribute<1, Vector3> TextureCoordinates;
        typedef GL::Attribute<2, Vector3> Color3;
        typedef GL::Attribute<3, UnsignedInt> Style;

        explicit TextShaderGL(UnsignedInt styleCount);

        TextShaderGL& setTransformationProjectionMatrix(const Matrix3& matrix) {
            setUniform(_transformationProjectionMatrixUniform, matrix);
            return *this;
        }

        TextShaderGL& bindGlyphTexture(GL::Texture2D& texture) {
            texture.bind(GlyphTextureBinding);
            return *this;
        }

        TextShaderGL& bindStyleBuffer(GL::Buffer& buffer) {
            buffer.bind(GL::Buffer::Target::Uniform, StyleBufferBinding);
            return *this;
        }

    private:
        Int _transformationProjectionMatrixUniform = 0;
};

TextShaderGL::TextShaderGL(const UnsignedInt styleCount) {
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
    vert.addSource(rs.getString("compatibility.glsl"_s))
        .addSource(rs.getString("TextShader.vert"_s));

    GL::Shader frag{version, GL::Shader::Type::Fragment};
    frag.addSource(Utility::format("#define STYLE_COUNT {}\n", styleCount))
        .addSource(rs.getString("compatibility.glsl"_s))
        .addSource(rs.getString("TextShader.frag"_s));

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
        setUniform(uniformLocation("glyphTextureData"_s), GlyphTextureBinding);
        setUniformBlockBinding(uniformBlockIndex("Style"_s), StyleBufferBinding);
    }
}

}

struct TextLayerGL::Shared::State: TextLayer::Shared::State {
    explicit State(Shared& self, const Configuration& configuration): TextLayer::Shared::State{self, configuration}, shader{configuration.styleUniformCount()} {}

    /* Never used directly, only owns the instance passed to
       setGlyphCache(GlyphCache&&) if it got called instead of
       setGlyphCache(GlyphCache&). The actual used glyph cache pointer is in
       the base state struct. */
    Containers::Optional<Text::GlyphCache> glyphCacheStorage;
    TextShaderGL shader;
    /* The buffer is NoCreate'd at first to be able to detect whether
       setStyle() was called at all */
    GL::Buffer styleBuffer{NoCreate};
};

TextLayerGL::Shared::Shared(const Configuration& configuration): TextLayer::Shared{Containers::pointer<State>(*this, configuration)} {}

TextLayerGL::Shared::Shared(NoCreateT) noexcept: TextLayer::Shared{NoCreate} {}

TextLayerGL::Shared& TextLayerGL::Shared::setGlyphCache(Text::GlyphCache& cache) {
    TextLayer::Shared::setGlyphCache(cache);
    return *this;
}

TextLayerGL::Shared& TextLayerGL::Shared::setGlyphCache(Text::GlyphCache&& cache) {
    auto& state = static_cast<State&>(*_state);
    state.glyphCacheStorage = Utility::move(cache);
    return setGlyphCache(*state.glyphCacheStorage);
}

TextLayerGL::Shared& TextLayerGL::Shared::setStyle(const TextLayerCommonStyleUniform& commonUniform, const Containers::ArrayView<const TextLayerStyleUniform> uniforms, const Containers::StridedArrayView1D<const FontHandle>& fonts, const Containers::StridedArrayView1D<const Vector4>& paddings) {
    return static_cast<Shared&>(TextLayer::Shared::setStyle(commonUniform, uniforms, fonts, paddings));
}

TextLayerGL::Shared& TextLayerGL::Shared::setStyle(const TextLayerCommonStyleUniform& commonUniform, const std::initializer_list<TextLayerStyleUniform> uniforms, const std::initializer_list<FontHandle> fonts, const std::initializer_list<Vector4> paddings) {
    return static_cast<Shared&>(TextLayer::Shared::setStyle(commonUniform, uniforms, fonts, paddings));
}

TextLayerGL::Shared& TextLayerGL::Shared::setStyle(const TextLayerCommonStyleUniform& commonUniform, const Containers::ArrayView<const TextLayerStyleUniform> uniforms, const Containers::StridedArrayView1D<const UnsignedInt>& styleToUniform, const Containers::StridedArrayView1D<const FontHandle>& styleFonts, const Containers::StridedArrayView1D<const Vector4>& stylePaddings) {
    return static_cast<Shared&>(TextLayer::Shared::setStyle(commonUniform, uniforms, styleToUniform, styleFonts, stylePaddings));
}

TextLayerGL::Shared& TextLayerGL::Shared::setStyle(const TextLayerCommonStyleUniform& commonUniform, const std::initializer_list<TextLayerStyleUniform> uniforms, const std::initializer_list<UnsignedInt> styleToUniform, const std::initializer_list<FontHandle> styleFonts, const std::initializer_list<Vector4> stylePaddings) {
    return static_cast<Shared&>(TextLayer::Shared::setStyle(commonUniform, uniforms, styleToUniform, styleFonts, stylePaddings));
}

void TextLayerGL::Shared::doSetStyle(const TextLayerCommonStyleUniform& commonUniform, const Containers::ArrayView<const TextLayerStyleUniform> uniforms) {
    auto& state = static_cast<State&>(*_state);
    /* The buffer is NoCreate'd at first to be able to detect whether
       setStyle() was called at all */
    if(!state.styleBuffer.id())
        state.styleBuffer = GL::Buffer{GL::Buffer::TargetHint::Uniform, {nullptr, sizeof(TextLayerCommonStyleUniform) + sizeof(TextLayerStyleUniform)*state.styleUniformCount}};

    /** @todo the common stuff wouldn't even need to be uploaded, skipping it
        causes no breakage in the shader */
    state.styleBuffer.setSubData(0, {&commonUniform, 1});
    state.styleBuffer.setSubData(sizeof(TextLayerCommonStyleUniform), uniforms);
}

struct TextLayerGL::State: TextLayer::State {
    explicit State(Shared::State& shared): TextLayer::State{shared} {}

    GL::Buffer vertexBuffer{GL::Buffer::TargetHint::Array}, indexBuffer{GL::Buffer::TargetHint::ElementArray};
    GL::Mesh mesh;
    Vector2 clipScale;
    Vector2i framebufferSize;
};

TextLayerGL::TextLayerGL(const LayerHandle handle, Shared& sharedState): TextLayer{handle, Containers::pointer<State>(static_cast<Shared::State&>(*sharedState._state))} {
    auto& state = static_cast<State&>(*_state);
    state.mesh.addVertexBuffer(state.vertexBuffer, 0,
        TextShaderGL::Position{},
        TextShaderGL::TextureCoordinates{},
        TextShaderGL::Color3{},
        TextShaderGL::Style{});
    state.mesh.setIndexBuffer(state.indexBuffer, 0, GL::MeshIndexType::UnsignedInt);
}

LayerFeatures TextLayerGL::doFeatures() const {
    return TextLayer::doFeatures()|LayerFeature::DrawUsesBlending|LayerFeature::DrawUsesScissor;
}

void TextLayerGL::doSetSize(const Vector2& size, const Vector2i& framebufferSize) {
    auto& state = static_cast<State&>(*_state);
    auto& sharedState = static_cast<Shared::State&>(state.shared);

    /* The TextLayer populates the data expecting the origin is top left and Y
       down */
    const Matrix3 projectionMatrix =
        Matrix3::scaling({1.0f, -1.0f})*
        Matrix3::translation({-1.0f, -1.0f})*
        Matrix3::projection(size);
    sharedState.shader.setTransformationProjectionMatrix(projectionMatrix);

    /* For scaling and Y-flipping the clip rects in doDraw() */
    state.clipScale = Vector2{framebufferSize}/size;
    state.framebufferSize = framebufferSize;
}

void TextLayerGL::doUpdate(const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, const Containers::BitArrayView nodesEnabled, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes) {
    TextLayer::doUpdate(dataIds, clipRectIds, clipRectDataCounts, nodeOffsets, nodeSizes, nodesEnabled, clipRectOffsets, clipRectSizes);

    State& state = static_cast<State&>(*_state);
    state.indexBuffer.setData(state.indices);
    state.vertexBuffer.setData(state.vertices);
    state.mesh.setCount(state.indices.size());
}

void TextLayerGL::doDraw(const Containers::StridedArrayView1D<const UnsignedInt>&, const std::size_t offset, const std::size_t count, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, const std::size_t clipRectOffset, const std::size_t clipRectCount, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, Containers::BitArrayView, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes) {
    auto& state = static_cast<State&>(*_state);
    CORRADE_ASSERT(!state.framebufferSize.isZero() && !state.clipScale.isZero(),
        "Whee::TextLayerGL::draw(): user interface size wasn't set", );

    auto& sharedState = static_cast<Shared::State&>(state.shared);
    CORRADE_ASSERT(sharedState.styleBuffer.id(),
        "Whee::TextLayerGL::draw(): no style data was set", );

    sharedState.shader
        .bindGlyphTexture(static_cast<Text::GlyphCache&>(*sharedState.glyphCache).texture())
        .bindStyleBuffer(sharedState.styleBuffer);

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
            .setIndexOffset(state.indexDrawOffsets[clipDataOffset])
            .setCount(state.indexDrawOffsets[clipDataOffset + clipRectDataCount] - state.indexDrawOffsets[clipDataOffset]);
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
