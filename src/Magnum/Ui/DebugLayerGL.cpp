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

#include "DebugLayerGL.h"

#include <Corrade/Containers/BitArrayView.h>
#include <Corrade/Containers/Iterable.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Utility/Resource.h>
#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/Context.h>
#include <Magnum/GL/Extensions.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/Math/Color.h>

#include "Magnum/Ui/Implementation/debugLayerState.h"

#ifdef MAGNUM_UI_BUILD_STATIC
static void importShaderResources() {
    CORRADE_RESOURCE_INITIALIZE(MagnumUi_RESOURCES)
}
#endif

namespace Magnum { namespace Ui {

using namespace Containers::Literals;
using namespace Math::Literals;

namespace {

class DebugShaderGL: public GL::AbstractShaderProgram {
    public:
        typedef GL::Attribute<0, Vector2> Position;
        typedef GL::Attribute<1, Vector4> Color4;

        explicit DebugShaderGL();

        DebugShaderGL& setProjection(const Vector2& scaling) {
            /* Y-flipped scale from the UI size to the 2x2 unit square, the
               shader then translates by (-1, 1) on its own to put the origin
               at center. */
            setUniform(_projectionUniform, Vector2{2.0f, -2.0f}/scaling);
            return *this;
        }

    private:
        Int _projectionUniform = 0;
};

DebugShaderGL::DebugShaderGL() {
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

    GL::Shader vert{version, GL::Shader::Type::Vertex};
    vert.addSource(rs.getString("compatibility.glsl"_s))
        .addSource(rs.getString("DebugShader.vert"_s));

    GL::Shader frag{version, GL::Shader::Type::Fragment};
    frag.addSource(rs.getString("compatibility.glsl"_s))
        .addSource(rs.getString("DebugShader.frag"_s));

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
}

}

struct DebugLayerGL::State: DebugLayer::State {
    explicit State(DebugLayerSources sources, DebugLayerFlags flags): DebugLayer::State{sources, flags} {}

    DebugShaderGL shader;

    GL::Buffer vertexBuffer{GL::Buffer::TargetHint::Array};
    GL::Mesh mesh{GL::MeshPrimitive::TriangleStrip};
};

DebugLayerGL::DebugLayerGL(const LayerHandle handle, const DebugLayerSources sources, const DebugLayerFlags flags): DebugLayer{handle, Containers::pointer<State>(sources, flags)} {
    auto& state = static_cast<State&>(*_state);
    state.mesh
        .addVertexBuffer(state.vertexBuffer, 0,
            DebugShaderGL::Position{},
            DebugShaderGL::Color4{})
        .setCount(4);
}

LayerFeatures DebugLayerGL::doFeatures() const {
    return DebugLayer::doFeatures()|LayerFeature::DrawUsesBlending;
}

void DebugLayerGL::doSetSize(const Vector2& size, const Vector2i&) {
    auto& state = static_cast<State&>(*_state);
    state.shader.setProjection(size);
}

void DebugLayerGL::doUpdate(const LayerStates states, const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, const Containers::StridedArrayView1D<const Float>& nodeOpacities, const Containers::BitArrayView nodesEnabled, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes, const Containers::StridedArrayView1D<const Vector2>& compositeRectOffsets, const Containers::StridedArrayView1D<const Vector2>& compositeRectSizes) {
    DebugLayer::doUpdate(states, dataIds, clipRectIds, clipRectDataCounts, nodeOffsets, nodeSizes, nodeOpacities, nodesEnabled, clipRectOffsets, clipRectSizes, compositeRectOffsets, compositeRectSizes);

    State& state = static_cast<State&>(*_state);

    /* The branching here mirrors how DebugLayer::doUpdate() restricts the
       updates. Keep in sync. */
    if(state.highlightedNodeDrawOffset != ~std::size_t{} &&
       (states >= LayerState::NeedsDataUpdate ||
        states >= LayerState::NeedsNodeOffsetSizeUpdate))
        state.vertexBuffer.setData(state.highlightedNodeVertices);
}

void DebugLayerGL::doDraw(const Containers::StridedArrayView1D<const UnsignedInt>&, std::size_t offset, std::size_t count, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const UnsignedInt>&, std::size_t, std::size_t, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Float>&, Containers::BitArrayView, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&) {
    State& state = static_cast<State&>(*_state);

    /* There's exactly one node to highlight, so draw it when it's included in
       the range defined by offset + count */
    /** @todo this would however completely prevent draw call merging (once
        that's done), figure out a way for the layer to signal that not all
        data are actually meant to be drawn (per-data features? uh...) */
    if(state.highlightedNodeDrawOffset >= offset &&
       state.highlightedNodeDrawOffset < offset + count) {
        state.shader.draw(state.mesh);
    }
}

}}
