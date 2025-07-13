/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023, 2024, 2025
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

#include "LineLayerGL.h"

#include <Corrade/Containers/BitArrayView.h>
#include <Corrade/Containers/Iterable.h>
#include <Corrade/Containers/String.h>
#include <Corrade/Utility/Format.h>
#include <Corrade/Utility/Resource.h>
#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/Context.h>
#ifndef MAGNUM_TARGET_GLES
#include <Magnum/GL/Extensions.h>
#endif
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/GL/Version.h>

#include "Magnum/Ui/Implementation/lineLayerState.h"

#ifdef MAGNUM_UI_BUILD_STATIC
static void importShaderResources() {
    CORRADE_RESOURCE_INITIALIZE(MagnumUi_RESOURCES)
}
#endif

namespace Magnum { namespace Ui {

using namespace Containers::Literals;

namespace {

class LineShaderGL: public GL::AbstractShaderProgram {
    private:
        enum: Int {
            StyleBufferBinding = 0,
        };

    public:
        typedef GL::Attribute<0, Vector2> Position;
        typedef GL::Attribute<1, Vector2> PreviousPosition;
        typedef GL::Attribute<2, Vector2> NextPosition;
        typedef GL::Attribute<3, Vector4> Color4;
        typedef GL::Attribute<4, UnsignedInt> AnnotationStyle;

        explicit LineShaderGL(UnsignedInt styleCount, LineCapStyle capStyle, LineJoinStyle joinStyle);

        LineShaderGL& setProjection(const Vector2& scaling, const Float pixelScaling) {
            /* XY is Y-flipped scale from the UI size to the 2x2 unit square,
               the shader then translates by (-1, 1) on its own to put the
               origin at center. Z is multiplied with the pixel smoothness
               value to get the smoothness in actual UI units. */
            setUniform(_projectionUniform, Vector3{Vector2{2.0f, -2.0f}/scaling, pixelScaling});
            return *this;
        }

        LineShaderGL& bindStyleBuffer(GL::Buffer& buffer) {
            buffer.bind(GL::Buffer::Target::Uniform, StyleBufferBinding);
            return *this;
        }

    private:
        Int _projectionUniform = 0;
};

LineShaderGL::LineShaderGL(const UnsignedInt styleCount, const LineCapStyle capStyle, const LineJoinStyle joinStyle) {
    GL::Context& context = GL::Context::current();
    #ifndef MAGNUM_TARGET_GLES
    MAGNUM_ASSERT_GL_EXTENSION_SUPPORTED(GL::Extensions::ARB::explicit_attrib_location);
    #endif

    #ifdef MAGNUM_UI_BUILD_STATIC
    if(!Utility::Resource::hasGroup("MagnumUi"_s))
        importShaderResources();
    #endif

    Utility::Resource rs{"MagnumUi"_s};

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

    /* Cap and join style is needed by both the vertex and fragment shader,
       prepare their defines just once for both. This snippet is shared between
       Shaders::LineGL and Ui::LineLayerGL, keep in sync. */
    Containers::StringView capStyleDefine, joinStyleDefine;
    switch(capStyle) {
        case LineCapStyle::Butt:
            capStyleDefine = "#define CAP_STYLE_BUTT\n"_s;
            break;
        case LineCapStyle::Square:
            capStyleDefine = "#define CAP_STYLE_SQUARE\n"_s;
            break;
        case LineCapStyle::Round:
            capStyleDefine = "#define CAP_STYLE_ROUND\n"_s;
            break;
        case LineCapStyle::Triangle:
            capStyleDefine = "#define CAP_STYLE_TRIANGLE\n"_s;
            break;
    }
    switch(joinStyle) {
        case LineJoinStyle::Miter:
            joinStyleDefine = "#define JOIN_STYLE_MITER\n"_s;
            break;
        case LineJoinStyle::Bevel:
            joinStyleDefine = "#define JOIN_STYLE_BEVEL\n"_s;
            break;
    }
    CORRADE_INTERNAL_ASSERT(capStyleDefine);
    CORRADE_INTERNAL_ASSERT(joinStyleDefine);

    GL::Shader vert{version, GL::Shader::Type::Vertex};
    vert.addSource(Utility::format("#define STYLE_COUNT {}\n", styleCount))
        .addSource(capStyleDefine)
        .addSource(joinStyleDefine)
        .addSource(rs.getString("compatibility.glsl"_s))
        .addSource(rs.getString("LineShader.vert"_s))
        .addSource(rs.getString("LineShader.in.vert"_s));

    GL::Shader frag{version, GL::Shader::Type::Fragment};

    frag.addSource(Utility::format("#define STYLE_COUNT {}\n", styleCount))
        .addSource(capStyleDefine)
        .addSource(joinStyleDefine)
        .addSource(rs.getString("compatibility.glsl"_s))
        .addSource(rs.getString("LineShader.frag"_s))
        .addSource(rs.getString("LineShader.in.frag"_s));

    CORRADE_INTERNAL_ASSERT_OUTPUT(vert.compile() && frag.compile());

    attachShaders({vert, frag});
    CORRADE_INTERNAL_ASSERT_OUTPUT(link());

    #ifndef MAGNUM_TARGET_GLES
    if(!context.isExtensionSupported<GL::Extensions::ARB::explicit_uniform_location>())
    #elif !defined(MAGNUM_TARGET_GLES2) && !defined(MAGNUM_TARGET_WEBGL)
    if(version < GL::Version::GLES310)
    #endif
    {
        _projectionUniform = uniformLocation("projection"_s);
    }

    #ifndef MAGNUM_TARGET_GLES
    if(!context.isExtensionSupported<GL::Extensions::ARB::shading_language_420pack>())
    #elif !defined(MAGNUM_TARGET_GLES2) && !defined(MAGNUM_TARGET_WEBGL)
    if(version < GL::Version::GLES310)
    #endif
    {
        setUniformBlockBinding(uniformBlockIndex("Style"_s), StyleBufferBinding);
    }
}

}

struct LineLayerGL::Shared::State: LineLayer::Shared::State {
    explicit State(Shared& self, const Configuration& configuration);

    LineShaderGL shader;
    GL::Buffer styleBuffer{GL::Buffer::TargetHint::Uniform};
};

LineLayerGL::Shared::State::State(Shared& self, const Configuration& configuration): LineLayer::Shared::State{self, configuration}, shader{configuration.styleUniformCount(), configuration.capStyle(), configuration.joinStyle()} {
    styleBuffer.setData({nullptr, sizeof(LineLayerCommonStyleUniform) + sizeof(LineLayerStyleUniform)*styleUniformCount}, GL::BufferUsage::StaticDraw);
}

LineLayerGL::Shared::Shared(const Configuration& configuration): LineLayer::Shared{Containers::pointer<State>(*this, configuration)} {}

LineLayerGL::Shared::Shared(NoCreateT) noexcept: LineLayer::Shared{NoCreate} {}

void LineLayerGL::Shared::doSetStyle(const LineLayerCommonStyleUniform& commonUniform, const Containers::ArrayView<const LineLayerStyleUniform> uniforms) {
    auto& state = static_cast<State&>(*_state);
    state.styleBuffer.setSubData(0, {&commonUniform, 1});
    state.styleBuffer.setSubData(sizeof(LineLayerCommonStyleUniform), uniforms);
}

struct LineLayerGL::State: LineLayer::State {
    explicit State(Shared::State& shared): LineLayer::State{shared} {}

    GL::Buffer vertexBuffer{GL::Buffer::TargetHint::Array},
        indexBuffer{GL::Buffer::TargetHint::ElementArray};
    GL::Mesh mesh;

    #ifndef CORRADE_NO_ASSERT
    bool setSizeCalled = false;
    #endif
    /* 3 / 7 bytes free, 0 w/ CORRADE_NO_ASSERT */
};

LineLayerGL::LineLayerGL(const LayerHandle handle, Shared& sharedState_): LineLayer{handle, Containers::pointer<State>(static_cast<Shared::State&>(*sharedState_._state))} {
    auto& state = static_cast<State&>(*_state);
    state.mesh.addVertexBuffer(state.vertexBuffer, 0,
        LineShaderGL::Position{},
        LineShaderGL::PreviousPosition{},
        LineShaderGL::NextPosition{},
        LineShaderGL::Color4{},
        LineShaderGL::AnnotationStyle{});
    state.mesh.setIndexBuffer(state.indexBuffer, 0, GL::MeshIndexType::UnsignedInt);
}

LayerFeatures LineLayerGL::doFeatures() const {
    return LineLayer::doFeatures()|LayerFeature::DrawUsesBlending;
}

void LineLayerGL::doSetSize(const Vector2& size, const Vector2i& framebufferSize) {
    auto& state = static_cast<State&>(*_state);
    auto& sharedState = static_cast<Shared::State&>(state.shared);

    /** @todo Max or min? Should I even bother with non-square scaling? */
    sharedState.shader.setProjection(size, (size/Vector2{framebufferSize}).max());

    #ifndef CORRADE_NO_ASSERT
    /* Now it's safe to call draw() */
    state.setSizeCalled = true;
    #endif
}

void LineLayerGL::doUpdate(const LayerStates states, const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, const Containers::StridedArrayView1D<const Float>& nodeOpacities, const Containers::BitArrayView nodesEnabled, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes, const Containers::StridedArrayView1D<const Vector2>& compositeRectOffsets, const Containers::StridedArrayView1D<const Vector2>& compositeRectSizes) {
    State& state = static_cast<State&>(*_state);

    LineLayer::doUpdate(states, dataIds, clipRectIds, clipRectDataCounts, nodeOffsets, nodeSizes, nodeOpacities, nodesEnabled, clipRectOffsets, clipRectSizes, compositeRectOffsets, compositeRectSizes);

    /* The branching here mirrors how LineLayer::doUpdate() restricts the
       updates */
    if(states >= LayerState::NeedsNodeOrderUpdate ||
       states >= LayerState::NeedsDataUpdate)
    {
        state.indexBuffer.setData(state.indices);
        state.mesh.setCount(state.indices.size());
    }
    if(states >= LayerState::NeedsNodeOffsetSizeUpdate ||
       states >= LayerState::NeedsNodeEnabledUpdate ||
       states >= LayerState::NeedsNodeOpacityUpdate ||
       states >= LayerState::NeedsDataUpdate

    )
    {
        state.vertexBuffer.setData(state.vertices);
    }
}

void LineLayerGL::doDraw(const Containers::StridedArrayView1D<const UnsignedInt>&, const std::size_t offset, const std::size_t count, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const UnsignedInt>&, const std::size_t, const std::size_t, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Float>&, Containers::BitArrayView, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&) {
    auto& state = static_cast<State&>(*_state);
    CORRADE_ASSERT(state.setSizeCalled,
        "Ui::LineLayerGL::draw(): user interface size wasn't set", );

    auto& sharedState = static_cast<Shared::State&>(state.shared);
    CORRADE_ASSERT(sharedState.setStyleCalled,
        "Ui::LineLayerGL::draw(): no style data was set", );

    /* If there are dynamic styles, bind the layer-specific buffer that
       contains them, otherwise bind the shared buffer */
    sharedState.shader.bindStyleBuffer(sharedState.styleBuffer);

    state.mesh
        .setIndexOffset(state.indexDrawOffsets[offset])
        .setCount(state.indexDrawOffsets[offset + count] - state.indexDrawOffsets[offset]);
    sharedState.shader
        .draw(state.mesh);
}

}}
