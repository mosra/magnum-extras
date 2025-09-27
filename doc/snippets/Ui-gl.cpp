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

#include <Corrade/Containers/BitArrayView.h>
#include <Corrade/Containers/GrowableArray.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/PluginManager/Manager.h>
#include <Corrade/Utility/Algorithms.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/TextureArray.h>
#include <Magnum/Math/Matrix3.h>
#include <Magnum/Shaders/FlatGL.h>
#include <Magnum/Text/AbstractFont.h>
#include <Magnum/Text/DistanceFieldGlyphCacheGL.h>
#include <Magnum/Trade/AbstractImporter.h>

#include "Magnum/Ui/BaseLayerGL.h"
#include "Magnum/Ui/BaseLayerAnimator.h"
#include "Magnum/Ui/DebugLayerGL.h"
#include "Magnum/Ui/EventLayer.h"
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/LineLayerGL.h"
#include "Magnum/Ui/RendererGL.h"
#include "Magnum/Ui/Style.h"
#include "Magnum/Ui/TextLayerGL.h"
#include "Magnum/Ui/TextLayerAnimator.h"
#include "Magnum/Ui/TextProperties.h"
#include "Magnum/Ui/UserInterfaceGL.h"

#define DOXYGEN_ELLIPSIS(...) __VA_ARGS__
#define DOXYGEN_IGNORE(...) __VA_ARGS__

using namespace Magnum;
using namespace Math::Literals;

namespace A {

/* [AbstractLayer-custom] */
class QuadLayer: public Ui::AbstractLayer {
    public:
        explicit QuadLayer(Ui::LayerHandle handle);

        Ui::DataHandle create(const Color3& color,
                              Ui::NodeHandle node = Ui::NodeHandle::Null);
        void remove(Ui::DataHandle handle);
        void remove(Ui::LayerDataHandle handle);DOXYGEN_IGNORE(
            void setColor(Ui::DataHandle data, const Color3& color);
            void setColor(Ui::LayerDataHandle data, const Color3& color);
        )

    private:
        Ui::LayerFeatures doFeatures() const override {
            return Ui::LayerFeature::Draw;
        }
        DOXYGEN_ELLIPSIS(
            void doUpdate(Ui::LayerStates state, const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, const Containers::StridedArrayView1D<const Float>& nodeOpacities, Containers::BitArrayView nodesEnabled, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes, const Containers::StridedArrayView1D<const Vector2>& compositeRectOffsets, const Containers::StridedArrayView1D<const Vector2>& compositeRectSizes) override;
            void doSetSize(const Vector2& size, const Vector2i& framebufferSize) override;
            void doDraw(const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, std::size_t offset, std::size_t count, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, std::size_t clipRectOffset, std::size_t clipRectCount, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, const Containers::StridedArrayView1D<const Float>& nodeOpacities, Containers::BitArrayView nodesEnabled, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes) override;
        )

        struct Vertex {
            Vector2 position;
            Color3 color;
        };
        GL::Buffer _indices, _vertices;
        GL::Mesh _mesh;
        Shaders::FlatGL2D _shader{Shaders::FlatGL2D::Configuration{}
            .setFlags(Shaders::FlatGL2D::Flag::VertexColor)};

        Containers::Array<Color3> _colors;
};
/* [AbstractLayer-custom] */

/* [AbstractLayer-custom-constructor] */
QuadLayer::QuadLayer(Ui::LayerHandle handle): Ui::AbstractLayer{handle} {
    _mesh.addVertexBuffer(_vertices, 0,
            Shaders::FlatGL2D::Position{},
            Shaders::FlatGL2D::Color3{})
         .setIndexBuffer(_indices, 0, GL::MeshIndexType::UnsignedInt);
}
/* [AbstractLayer-custom-constructor] */

/* [AbstractLayer-custom-create] */
Ui::DataHandle QuadLayer::create(const Color3& color, Ui::NodeHandle node) {
    Ui::DataHandle handle = Ui::AbstractLayer::create(node);
    UnsignedInt dataId = dataHandleId(handle);
    if(dataId >= _colors.size())
        arrayResize(_colors, dataId + 1);

    _colors[dataId] = color;
    return handle;
}
/* [AbstractLayer-custom-create] */

/* [AbstractLayer-custom-remove] */
void QuadLayer::remove(Ui::DataHandle handle) {
    Ui::AbstractLayer::remove(handle);
}

void QuadLayer::remove(Ui::LayerDataHandle handle) {
    Ui::AbstractLayer::remove(handle);
}
/* [AbstractLayer-custom-remove] */

/* [AbstractLayer-custom-update-signature] */
void QuadLayer::doUpdate(Ui::LayerStates,
    const Containers::StridedArrayView1D<const UnsignedInt>& dataIds,
    const Containers::StridedArrayView1D<const UnsignedInt>&,
    const Containers::StridedArrayView1D<const UnsignedInt>&,
    const Containers::StridedArrayView1D<const Vector2>& nodeOffsets,
    const Containers::StridedArrayView1D<const Vector2>& nodeSizes,
    const Containers::StridedArrayView1D<const Float>&, Containers::BitArrayView,
    const Containers::StridedArrayView1D<const Vector2>&,
    const Containers::StridedArrayView1D<const Vector2>&,
    const Containers::StridedArrayView1D<const Vector2>&,
    const Containers::StridedArrayView1D<const Vector2>&)
/* [AbstractLayer-custom-update-signature] */
/* [AbstractLayer-custom-update] */
{
    Containers::Array<Vertex> vertexData{NoInit, dataIds.size()*4};
    Containers::Array<UnsignedInt> indexData{NoInit, dataIds.size()*6};

    Containers::StridedArrayView1D<const Ui::NodeHandle> nodes = this->nodes();
    for(UnsignedInt i = 0; i != dataIds.size(); ++i) {
        UnsignedInt dataId = dataIds[i];
        UnsignedInt nodeId = nodeHandleId(nodes[dataId]);
        Range2D rect = Range2D::fromSize(nodeOffsets[nodeId], nodeSizes[nodeId]);

        /*           0--1          0-2 3
           vertices: |  | indices: |/ /|
                     2--3          1 4-5 */
        for(UnsignedInt j = 0; j != 4; ++j) {
            vertexData[i*4 + j].position = Math::lerp(rect.min(), rect.max(), j);
            vertexData[i*4 + j].color = _colors[dataId];
        }
        Utility::copy({i*4 + 0, i*4 + 2, i*4 + 1,
                       i*4 + 1, i*4 + 2, i*4 + 3},
                      indexData.sliceSize(i*6, 6));
    }

    _vertices.setData(vertexData);
    _indices.setData(indexData);
    _mesh.setCount(indexData.size());
}
/* [AbstractLayer-custom-update] */

/* [AbstractLayer-custom-setsize] */
void QuadLayer::doSetSize(const Vector2& size, const Vector2i&) {
    _shader.setTransformationProjectionMatrix(
        Matrix3::scaling(Vector2::yScale(-1.0f))*
        Matrix3::translation({-1.0f, -1.0f})*
        Matrix3::projection(size));
}
/* [AbstractLayer-custom-setsize] */

/* [AbstractLayer-custom-draw] */
void QuadLayer::doDraw(
    const Containers::StridedArrayView1D<const UnsignedInt>&,
    std::size_t offset, std::size_t count,
    const Containers::StridedArrayView1D<const UnsignedInt>&,
    const Containers::StridedArrayView1D<const UnsignedInt>&,
    std::size_t, std::size_t,
    const Containers::StridedArrayView1D<const Vector2>&,
    const Containers::StridedArrayView1D<const Vector2>&,
    const Containers::StridedArrayView1D<const Float>&, Containers::BitArrayView,
    const Containers::StridedArrayView1D<const Vector2>&,
    const Containers::StridedArrayView1D<const Vector2>&)
{
    _mesh
        .setIndexOffset(offset*6)
        .setCount(count*6);
    _shader.draw(_mesh);
}
/* [AbstractLayer-custom-draw] */

/* [AbstractLayer-custom-setters] */
void QuadLayer::setColor(Ui::DataHandle handle, const Color3& color) {
    CORRADE_ASSERT(isHandleValid(handle),
        "QuadLayer::setColor(): invalid handle" << handle, );
    _colors[Ui::dataHandleId(handle)] = color;
    setNeedsUpdate(Ui::LayerState::NeedsDataUpdate);
}
/* [AbstractLayer-custom-setters] */

/* [AbstractLayer-custom-setters-layerdatahandle] */
void QuadLayer::setColor(Ui::LayerDataHandle handle, const Color3& color) {
    CORRADE_ASSERT(isHandleValid(handle),
        "QuadLayer::setColor(): invalid handle" << handle, );
    _colors[Ui::layerDataHandleId(handle)] = color;
    setNeedsUpdate(Ui::LayerState::NeedsDataUpdate);
}
/* [AbstractLayer-custom-setters-layerdatahandle] */

}

namespace B {

/* [AbstractLayer-custom-blending] */
class QuadLayer: public Ui::AbstractLayer {
    public:
        explicit QuadLayer(Ui::LayerHandle handle): Ui::AbstractLayer{handle} {
            _mesh.addVertexBuffer(_vertices, 0,
                Shaders::FlatGL2D::Position{},
                Shaders::FlatGL2D::Color4{});
            DOXYGEN_ELLIPSIS()
        }

        Ui::DataHandle create(const Color4& color,
                              Ui::NodeHandle node = Ui::NodeHandle::Null);
        DOXYGEN_ELLIPSIS()

    private:
        Ui::LayerFeatures doFeatures() const override {
            return Ui::LayerFeature::Draw|
                   Ui::LayerFeature::DrawUsesBlending;
        }
        DOXYGEN_ELLIPSIS(
            void doUpdate(Ui::LayerStates state, const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, const Containers::StridedArrayView1D<const Float>& nodeOpacities, Containers::BitArrayView nodesEnabled, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes, const Containers::StridedArrayView1D<const Vector2>& compositeRectOffsets, const Containers::StridedArrayView1D<const Vector2>& compositeRectSizes) override;
        )

        struct Vertex {
            Vector2 position;
            Color4 color;
        };
        DOXYGEN_ELLIPSIS(GL::Buffer _vertices; GL::Mesh _mesh;)

        Containers::Array<Color4> _colors;
};
/* [AbstractLayer-custom-blending] */

/* [AbstractLayer-custom-node-opacity-enabled] */
void QuadLayer::doUpdate(Ui::LayerStates,
    const Containers::StridedArrayView1D<const UnsignedInt>& dataIds,
    const Containers::StridedArrayView1D<const UnsignedInt>&,
    const Containers::StridedArrayView1D<const UnsignedInt>&,
    const Containers::StridedArrayView1D<const Vector2>& nodeOffsets,
    const Containers::StridedArrayView1D<const Vector2>& nodeSizes,
    const Containers::StridedArrayView1D<const Float>& nodeOpacities,
    Containers::BitArrayView nodesEnabled,
    const Containers::StridedArrayView1D<const Vector2>&,
    const Containers::StridedArrayView1D<const Vector2>&,
    const Containers::StridedArrayView1D<const Vector2>&,
    const Containers::StridedArrayView1D<const Vector2>&)
{
    DOXYGEN_ELLIPSIS(Containers::Array<Vertex> vertexData;)
    for(UnsignedInt i = 0; i != dataIds.size(); ++i) {
        DOXYGEN_ELLIPSIS(UnsignedInt dataId{}, nodeId{};)

        Color4 color = _colors[dataId];
        if(!nodesEnabled[nodeId])
            color.rgb() = Color3{color.value()*0.75f};
        color *= nodeOpacities[nodeId];
        for(UnsignedInt j = 0; j != 4; ++j)
            vertexData[i*4 + j].color = color;
    }

    DOXYGEN_ELLIPSIS(
        static_cast<void>(nodeOffsets);
        static_cast<void>(nodeSizes);
    )
}
/* [AbstractLayer-custom-node-opacity-enabled] */

}

namespace C {

class QuadLayer: public Ui::AbstractLayer {
    public:
        explicit QuadLayer(Ui::LayerHandle handle): Ui::AbstractLayer{handle} {}

    private:
        Ui::LayerFeatures doFeatures() const override { return {}; }
        void doUpdate(Ui::LayerStates state, const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, const Containers::StridedArrayView1D<const Float>& nodeOpacities, Containers::BitArrayView nodesEnabled, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes, const Containers::StridedArrayView1D<const Vector2>& compositeRectOffsets, const Containers::StridedArrayView1D<const Vector2>& compositeRectSizes) override;

        struct Vertex {
            Vector2 position;
        };
};

/* [AbstractLayer-custom-clip] */
void QuadLayer::doUpdate(Ui::LayerStates,
    const Containers::StridedArrayView1D<const UnsignedInt>& dataIds,
    const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds,
    const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts,
    const Containers::StridedArrayView1D<const Vector2>& nodeOffsets,
    const Containers::StridedArrayView1D<const Vector2>& nodeSizes,
    const Containers::StridedArrayView1D<const Float>&, Containers::BitArrayView,
    const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets,
    const Containers::StridedArrayView1D<const Vector2>& clipRectSizes,
    const Containers::StridedArrayView1D<const Vector2>&,
    const Containers::StridedArrayView1D<const Vector2>&)
{
    DOXYGEN_ELLIPSIS(Containers::Array<Vertex> vertexData;)

    UnsignedInt clipRect = 0;
    UnsignedInt clipRectDataCount = 0;
    for(UnsignedInt i = 0; i != dataIds.size(); ++i) {
        DOXYGEN_ELLIPSIS(UnsignedInt nodeId{};)

        /* If the clip rectangle is empty, no clipping is active */
        Range2D rect = Range2D::fromSize(nodeOffsets[nodeId],
                                         nodeSizes[nodeId]);
        Range2D clip = Range2D::fromSize(clipRectOffsets[clipRectIds[clipRect]],
                                         clipRectSizes[clipRectIds[clipRect]]);
        if(!clip.size().isZero())
            rect = Math::intersect(rect, clip);
        for(UnsignedInt j = 0; j != 4; ++j)
            vertexData[i*4 + j].position = Math::lerp(rect.min(), rect.max(), j);

        /* The clip rect got applied to all data it affects, move to the next */
        if(++clipRectDataCount == clipRectDataCounts[clipRect]) {
            ++clipRect;
            clipRectDataCount = 0;
        }
    }
}
/* [AbstractLayer-custom-clip] */

}

namespace D {

class QuadLayer: public Ui::AbstractLayer {
    public:
        explicit QuadLayer(Ui::LayerHandle handle): Ui::AbstractLayer{handle} {}

    private:
        Ui::LayerFeatures doFeatures() const override;
        void doSetSize(const Vector2& size, const Vector2i& framebufferSize) override;
        void doDraw(const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, std::size_t offset, std::size_t count, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, std::size_t clipRectOffset, std::size_t clipRectCount, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, const Containers::StridedArrayView1D<const Float>& nodeOpacities, Containers::BitArrayView nodesEnabled, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes) override;

        Vector2 _size;
        Vector2i _framebufferSize;
        GL::Mesh _mesh;
        Shaders::FlatGL2D _shader;
};

/* [AbstractLayer-custom-clip-scissor] */
Ui::LayerFeatures QuadLayer::doFeatures() const {
    return DOXYGEN_ELLIPSIS(Ui::LayerFeatures{})|Ui::LayerFeature::DrawUsesScissor;
}

void QuadLayer::doSetSize(const Vector2& size, const Vector2i& framebufferSize) {
    DOXYGEN_ELLIPSIS()
    _size = size;
    _framebufferSize = framebufferSize;
}
/* [AbstractLayer-custom-clip-scissor] */

/* [AbstractLayer-custom-clip-scissor-draw] */
void QuadLayer::doDraw(
    const Containers::StridedArrayView1D<const UnsignedInt>&,
    std::size_t offset, std::size_t,
    const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds,
    const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts,
    std::size_t clipRectOffset, std::size_t clipRectCount,
    const Containers::StridedArrayView1D<const Vector2>&,
    const Containers::StridedArrayView1D<const Vector2>&,
    const Containers::StridedArrayView1D<const Float>&, Containers::BitArrayView,
    const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets,
    const Containers::StridedArrayView1D<const Vector2>& clipRectSizes)
{
    std::size_t clipDataOffset = offset;
    for(std::size_t i = 0; i != clipRectCount; ++i) {
        UnsignedInt clipRectId = clipRectIds[clipRectOffset + i];
        UnsignedInt clipRectDataCount = clipRectDataCounts[clipRectOffset + i];
        Vector2i clipOffset = clipRectOffsets[clipRectId]*_framebufferSize/_size;
        Vector2i clipSize = clipRectSizes[clipRectId]*_framebufferSize/_size;

        /* If the clip rectangle is empty, not clipping anything, reset the
           scissor back to the whole framebuffer */
        GL::Renderer::setScissor(clipSize.isZero() ?
            Range2Di::fromSize({}, _framebufferSize) :
            Range2Di::fromSize(
                {clipOffset.x(), _framebufferSize.y() - clipOffset.y() - clipSize.y()},
                clipSize));

        _mesh
            .setIndexOffset(clipDataOffset*6)
            .setCount(clipRectDataCount*6);
        _shader.draw(_mesh);

        clipDataOffset += clipRectDataCount;
    }
}
/* [AbstractLayer-custom-clip-scissor-draw] */

}

namespace E {

class QuadLayer: public Ui::AbstractLayer {
    public:
        explicit QuadLayer(Ui::LayerHandle handle);

    private:
        Ui::LayerFeatures doFeatures() const override { return {}; }
        void doUpdate(Ui::LayerStates state, const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, const Containers::StridedArrayView1D<const Float>& nodeOpacities, Containers::BitArrayView nodesEnabled, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes, const Containers::StridedArrayView1D<const Vector2>& compositeRectOffsets, const Containers::StridedArrayView1D<const Vector2>& compositeRectSizes) override;

        struct Vertex {
            Vector2 position;
        };
        GL::Buffer _indices, _vertices;
        GL::Mesh _mesh;
};

#ifndef MAGNUM_TARGET_WEBGL /* No buffer mapping on WebGL */
/* [AbstractLayer-custom-update-in-data-order] */
void QuadLayer::doUpdate(DOXYGEN_ELLIPSIS(Ui::LayerStates,
    const Containers::StridedArrayView1D<const UnsignedInt>& dataIds,
    const Containers::StridedArrayView1D<const UnsignedInt>&,
    const Containers::StridedArrayView1D<const UnsignedInt>&,
    const Containers::StridedArrayView1D<const Vector2>& nodeOffsets,
    const Containers::StridedArrayView1D<const Vector2>& nodeSizes,
    const Containers::StridedArrayView1D<const Float>&, Containers::BitArrayView,
    const Containers::StridedArrayView1D<const Vector2>&,
    const Containers::StridedArrayView1D<const Vector2>&,
    const Containers::StridedArrayView1D<const Vector2>&,
    const Containers::StridedArrayView1D<const Vector2>&)) {
    /* vertices[i*4] to vertices[i*4 + 4] is a quad for data i */
    Containers::ArrayView<Vertex> vertices = Containers::arrayCast<Vertex>(
        _vertices.map(0, capacity()*sizeof(Vertex)*4, GL::Buffer::MapFlag::Write));
    Containers::StridedArrayView1D<const Ui::NodeHandle> nodes = this->nodes();
    for(UnsignedInt i = 0; i != dataIds.size(); ++i) {
        UnsignedInt dataId = dataIds[i];
        UnsignedInt nodeId = nodeHandleId(nodes[dataId]);
        Range2D rect = Range2D::fromSize(nodeOffsets[nodeId], nodeSizes[nodeId]);
        for(UnsignedInt j = 0; j != 4; ++j)
            vertices[dataId*4 + j].position = Math::lerp(rect.min(), rect.max(), j);
    }
    _vertices.unmap();

    /* indexData[i*6] to indexData[i*6 + 6] draws a quad for dataIds[i] */
    Containers::Array<UnsignedInt> indexData{NoInit, dataIds.size()*6};
    for(UnsignedInt i = 0; i != dataIds.size(); ++i) {
        UnsignedInt dataId = dataIds[i];
        Utility::copy({dataId*4 + 0, dataId*4 + 2, dataId*4 + 1,
                       dataId*4 + 1, dataId*4 + 2, dataId*4 + 3},
                      indexData.sliceSize(i*6, 6));
    }
    _indices.setData(indexData);

    _mesh.setCount(indexData.size());
}
/* [AbstractLayer-custom-update-in-data-order] */
#endif

}

namespace F {

class QuadLayer: public Ui::AbstractLayer {
    public:
        explicit QuadLayer(Ui::LayerHandle handle);

    private:
        Ui::LayerFeatures doFeatures() const override { return {}; }
        void doUpdate(Ui::LayerStates state, const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, const Containers::StridedArrayView1D<const Float>& nodeOpacities, Containers::BitArrayView nodesEnabled, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes, const Containers::StridedArrayView1D<const Vector2>& compositeRectOffsets, const Containers::StridedArrayView1D<const Vector2>& compositeRectSizes) override;
};

/* [AbstractLayer-custom-update-states] */
void QuadLayer::doUpdate(Ui::LayerStates state, DOXYGEN_ELLIPSIS(
    const Containers::StridedArrayView1D<const UnsignedInt>&,
    const Containers::StridedArrayView1D<const UnsignedInt>&,
    const Containers::StridedArrayView1D<const UnsignedInt>&,
    const Containers::StridedArrayView1D<const Vector2>&,
    const Containers::StridedArrayView1D<const Vector2>&,
    const Containers::StridedArrayView1D<const Float>&, Containers::BitArrayView,
    const Containers::StridedArrayView1D<const Vector2>&,
    const Containers::StridedArrayView1D<const Vector2>&,
    const Containers::StridedArrayView1D<const Vector2>&,
    const Containers::StridedArrayView1D<const Vector2>&)) {
    if(state & Ui::LayerState::NeedsNodeOffsetSizeUpdate) {
        /* Perform updates to vertex positions */
    }

    if(state & (Ui::LayerState::NeedsNodeEnabledUpdate|
                Ui::LayerState::NeedsNodeOpacityUpdate|
                Ui::LayerState::NeedsDataUpdate)) {
        /* Perform updates to vertex colors */
    }

    if(state & Ui::LayerState::NeedsNodeOrderUpdate) {
        /* Perform updates to the index buffer */
    }
}
/* [AbstractLayer-custom-update-states] */

}

namespace G {

class QuadLayer: public Ui::AbstractLayer {
    public:
        explicit QuadLayer(Ui::LayerHandle handle);

        Ui::DataHandle create(const Color3& color, Ui::NodeHandle node);

    private:
        Ui::LayerFeatures doFeatures() const override { return {}; }
        Ui::LayerStates doState() const override;
        void doUpdate(Ui::LayerStates state, const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, const Containers::StridedArrayView1D<const Float>& nodeOpacities, Containers::BitArrayView nodesEnabled, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes, const Containers::StridedArrayView1D<const Vector2>& compositeRectOffsets, const Containers::StridedArrayView1D<const Vector2>& compositeRectSizes) override;

        GL::Buffer _indices;
};

/* [AbstractLayer-custom-update-states-common] */
Ui::DataHandle QuadLayer::create(const Color3& color, Ui::NodeHandle node) {
    UnsignedInt capacityBefore = capacity();
    Ui::DataHandle handle = Ui::AbstractLayer::create(node);
    UnsignedInt dataId = dataHandleId(handle);
    if(dataId >= capacityBefore)
        setNeedsUpdate(Ui::LayerState::NeedsCommonDataUpdate);

    DOXYGEN_ELLIPSIS(static_cast<void>(color);)
    return handle;
}

void QuadLayer::doUpdate(Ui::LayerStates state, DOXYGEN_ELLIPSIS(
    const Containers::StridedArrayView1D<const UnsignedInt>&,
    const Containers::StridedArrayView1D<const UnsignedInt>&,
    const Containers::StridedArrayView1D<const UnsignedInt>&,
    const Containers::StridedArrayView1D<const Vector2>&,
    const Containers::StridedArrayView1D<const Vector2>&,
    const Containers::StridedArrayView1D<const Float>&, Containers::BitArrayView,
    const Containers::StridedArrayView1D<const Vector2>&,
    const Containers::StridedArrayView1D<const Vector2>&,
    const Containers::StridedArrayView1D<const Vector2>&,
    const Containers::StridedArrayView1D<const Vector2>&)) {
    if(state & Ui::LayerState::NeedsCommonDataUpdate) {
        Containers::Array<UnsignedInt> indexData{NoInit, capacity()*6};
        for(UnsignedInt i = 0; i != capacity(); ++i) {
            Utility::copy({i*4 + 0, i*4 + 2, i*4 + 1,
                           i*4 + 1, i*4 + 2, i*4 + 3},
                          indexData.sliceSize(i*6, 6));
        }
        _indices.setData(indexData);
    }

    DOXYGEN_ELLIPSIS()
}
/* [AbstractLayer-custom-update-states-common] */

}

namespace H {

class QuadLayer: public Ui::AbstractLayer {
    public:
        explicit QuadLayer(Ui::LayerHandle handle);

    private:
        Ui::LayerFeatures doFeatures() const override { return {}; }
        Ui::LayerStates doState() const override;
        void doUpdate(Ui::LayerStates state, const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, const Containers::StridedArrayView1D<const Float>& nodeOpacities, Containers::BitArrayView nodesEnabled, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes, const Containers::StridedArrayView1D<const Vector2>& compositeRectOffsets, const Containers::StridedArrayView1D<const Vector2>& compositeRectSizes) override;

        struct {
            Nanoseconds lastUpdate() const { return {}; }
        } _externalColors;
        Nanoseconds _lastUpdate;
};


/* [AbstractLayer-custom-update-states-timestamp] */
Ui::LayerStates QuadLayer::doState() const {
    if(_lastUpdate != _externalColors.lastUpdate())
        return Ui::LayerState::NeedsDataUpdate;
    return {};
}

void QuadLayer::doUpdate(Ui::LayerStates state,  DOXYGEN_ELLIPSIS(
    const Containers::StridedArrayView1D<const UnsignedInt>&,
    const Containers::StridedArrayView1D<const UnsignedInt>&,
    const Containers::StridedArrayView1D<const UnsignedInt>&,
    const Containers::StridedArrayView1D<const Vector2>&,
    const Containers::StridedArrayView1D<const Vector2>&,
    const Containers::StridedArrayView1D<const Float>&, Containers::BitArrayView,
    const Containers::StridedArrayView1D<const Vector2>&,
    const Containers::StridedArrayView1D<const Vector2>&,
    const Containers::StridedArrayView1D<const Vector2>&,
    const Containers::StridedArrayView1D<const Vector2>&)) {
    if(state & Ui::LayerState::NeedsDataUpdate) {
        _lastUpdate = _externalColors.lastUpdate();
        DOXYGEN_ELLIPSIS()
    }

    DOXYGEN_ELLIPSIS()
}
/* [AbstractLayer-custom-update-states-timestamp] */

}

namespace I {

/* [AbstractLayer-custom-resource-cleanup-remove] */
class QuadLayer: public Ui::AbstractLayer {
    DOXYGEN_ELLIPSIS(
        explicit QuadLayer(Ui::LayerHandle handle);
        void remove(Ui::DataHandle handle);
        Ui::LayerFeatures doFeatures() const override { return {}; }
        void doClean(Containers::BitArrayView dataIdsToRemove) override;
    )

    private:
        Containers::Array<Containers::Optional<GL::Texture2D>> _textures;
};

void QuadLayer::remove(Ui::DataHandle handle) {
    Ui::AbstractLayer::remove(handle);
    _textures[dataHandleId(handle)] = Containers::NullOpt;
}
/* [AbstractLayer-custom-resource-cleanup-remove] */

/* [AbstractLayer-custom-resource-cleanup-clean] */
void QuadLayer::doClean(Containers::BitArrayView dataIdsToRemove) {
    for(UnsignedInt i = 0; i != dataIdsToRemove.size(); ++i) {
        if(i)
            _textures[i] = Containers::NullOpt;
    }
}
/* [AbstractLayer-custom-resource-cleanup-clean] */

}

/* Make sure the name doesn't conflict with any other snippets to avoid linker
   warnings, unlike with `int main()` there now has to be a declaration to
   avoid -Wmisssing-prototypes */
void mainUiGL();
void mainUiGL() {
{
/* Used by both AbstractUserInterface and UserInterfaceGL docs */
/* [UserInterfaceGL-setup] */
Ui::UserInterfaceGL ui{{800, 600}, Ui::McssDarkStyle{}};
/* [UserInterfaceGL-setup] */

/* [AbstractUserInterface-setup-blend] */
GL::Renderer::setBlendFunction(
    GL::Renderer::BlendFunction::One,
    GL::Renderer::BlendFunction::OneMinusSourceAlpha);
/* [AbstractUserInterface-setup-blend] */

/* [AbstractUserInterface-setup-draw] */
GL::defaultFramebuffer.clear(GL::FramebufferClear::Color);

DOXYGEN_ELLIPSIS()

ui.draw();
/* [AbstractUserInterface-setup-draw] */

/* [AbstractUserInterface-setup-draw-ondemand] */
if(ui.state()) {
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color);

    ui.draw();
}
/* [AbstractUserInterface-setup-draw-ondemand] */
}

{
Ui::AbstractUserInterface ui{{100, 100}};
/* [AbstractUserInterface-renderer] */
ui.setRendererInstance(Containers::pointer<Ui::RendererGL>());
/* [AbstractUserInterface-renderer] */
}

{
struct: Ui::AbstractStyle {
    Ui::StyleFeatures doFeatures() const override { return {}; }
    bool doApply(Ui::UserInterface&, Ui::StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*) const override {
        return false;
    }
} myCustomStyle;
/* [UserInterfaceGL-setup-features] */
/* Pick everything except icons from the builtin style */
Ui::UserInterfaceGL ui{{800, 600}, Ui::McssDarkStyle{},
    ~Ui::StyleFeature::TextLayerImages};

DOXYGEN_ELLIPSIS()

/* Use icons from a custom style instead */
ui.setStyle(myCustomStyle, Ui::StyleFeature::TextLayerImages);
/* [UserInterfaceGL-setup-features] */
}

{
/* [UserInterfaceGL-setup-managers] */
PluginManager::Manager<Trade::AbstractImporter> importerManager;
PluginManager::Manager<Text::AbstractFont> fontManager;
DOXYGEN_ELLIPSIS()

Ui::UserInterfaceGL ui{DOXYGEN_ELLIPSIS({}, Ui::McssDarkStyle{}), &importerManager, &fontManager};
/* [UserInterfaceGL-setup-managers] */
}

{
/* [UserInterfaceGL-setup-renderer] */
Ui::UserInterfaceGL ui{NoCreate};

ui
    .setRendererInstance(Containers::pointer<Ui::RendererGL>(DOXYGEN_ELLIPSIS()))
    .setSize(DOXYGEN_ELLIPSIS({}))
    .setStyle(Ui::McssDarkStyle{});
/* [UserInterfaceGL-setup-renderer] */
}

{
Ui::BaseLayerGL::Shared shared{Ui::BaseLayerGL::Shared::Configuration{1}};
/* [UserInterfaceGL-setup-layer] */
Ui::UserInterfaceGL ui{DOXYGEN_ELLIPSIS({}), Ui::McssDarkStyle{},
    ~Ui::StyleFeature::BaseLayer};

DOXYGEN_ELLIPSIS()

ui.setBaseLayerInstance(Containers::pointer<Ui::BaseLayerGL>(DOXYGEN_ELLIPSIS(ui.createLayer(), shared)));
/* [UserInterfaceGL-setup-layer] */
}

{
/* [BaseLayer-setup-shared] */
Ui::BaseLayerGL::Shared baseLayerShared{
    Ui::BaseLayer::Shared::Configuration{3}
};
/* [BaseLayer-setup-shared] */

Ui::AbstractUserInterface ui{{100, 100}};
/* [BaseLayer-setup] */
Ui::BaseLayer& baseLayer = ui.setLayerInstance(
    Containers::pointer<Ui::BaseLayerGL>(ui.createLayer(), baseLayerShared));
/* [BaseLayer-setup] */
static_cast<void>(baseLayer);
}

{
Ui::UserInterfaceGL ui{NoCreate};
Ui::BaseLayerGL::Shared baseLayerShared{Ui::BaseLayer::Shared::Configuration{1}};
/* [BaseLayer-setup-implicit] */
ui.setBaseLayerInstance(
    Containers::pointer<Ui::BaseLayerGL>(ui.createLayer(), baseLayerShared));
/* [BaseLayer-setup-implicit] */
}

{
Ui::UserInterfaceGL ui{NoCreate};
/* [BaseLayer-style-textured1] */
Ui::BaseLayerGL::Shared texturedLayerShared{
    Ui::BaseLayerGL::Shared::Configuration{DOXYGEN_ELLIPSIS(1)}
        .addFlags(Ui::BaseLayerSharedFlag::Textured)
};
texturedLayerShared.setStyle(DOXYGEN_ELLIPSIS(Ui::BaseLayerCommonStyleUniform{}), {
    Ui::BaseLayerStyleUniform{}, /* 0 */
    Ui::BaseLayerStyleUniform{}  /* 1 */
        .setOutlineWidth(2.0f)
        .setOutlineColor(0xdcdcdcff_rgbaf*0.25f),
    Ui::BaseLayerStyleUniform{}  /* 2 */
        .setCornerRadius(12.0f)
}, {});

Ui::BaseLayerGL& texturedLayer = ui.setLayerInstance(
    Containers::pointer<Ui::BaseLayerGL>(ui.createLayer(), texturedLayerShared));
GL::Texture2DArray texture;
DOXYGEN_ELLIPSIS()
texturedLayer.setTexture(texture);
/* [BaseLayer-style-textured1] */

/* [BaseLayer-style-textured2] */
Ui::NodeHandle image = DOXYGEN_ELLIPSIS({});
Ui::NodeHandle outlined = DOXYGEN_ELLIPSIS({});
Ui::NodeHandle avatar = DOXYGEN_ELLIPSIS({});
texturedLayer.create(0, image);
texturedLayer.create(1, outlined);
Ui::DataHandle avatarData = texturedLayer.create(2, avatar);
texturedLayer.setTextureCoordinates(avatarData, {0.4f, 0.0f, 0.0f}, {0.25f, 0.5f});
/* [BaseLayer-style-textured2] */
}

{
Ui::UserInterfaceGL ui{NoCreate};
/* [BaseLayer-dynamic-styles] */
Ui::BaseLayerGL::Shared baseLayerShared{
    Ui::BaseLayerGL::Shared::Configuration{DOXYGEN_ELLIPSIS(1)}
        .setDynamicStyleCount(10)
};
Ui::BaseLayerGL& baseLayer = ui.setLayerInstance(
    Containers::pointer<Ui::BaseLayerGL>(ui.createLayer(), baseLayerShared));

DOXYGEN_ELLIPSIS()

UnsignedInt dynamicStyleId = DOXYGEN_ELLIPSIS(0); /* anything less than the dynamic style count */
baseLayer.setDynamicStyle(dynamicStyleId, DOXYGEN_ELLIPSIS(Ui::BaseLayerStyleUniform{}, {}));

Ui::NodeHandle node = DOXYGEN_ELLIPSIS({});
baseLayer.create(baseLayer.shared().styleCount() + dynamicStyleId, node);
/* [BaseLayer-dynamic-styles] */
}

{
Ui::UserInterfaceGL ui{NoCreate};
/* [BaseLayer-style-background-blur] */
ui.setRendererInstance(Containers::pointer<Ui::RendererGL>(
    Ui::RendererGL::Flag::CompositingFramebuffer));

Ui::BaseLayerGL::Shared blurLayerShared{
    Ui::BaseLayerGL::Shared::Configuration{DOXYGEN_ELLIPSIS(1)}
        .addFlags(Ui::BaseLayerSharedFlag::BackgroundBlur)
        .setBackgroundBlurRadius(DOXYGEN_ELLIPSIS(4))
};
blurLayerShared.setStyle(DOXYGEN_ELLIPSIS(Ui::BaseLayerCommonStyleUniform{}), {
    Ui::BaseLayerStyleUniform{}  /* 0 */
        .setCornerRadius(12.0f)
        .setColor(0xffffffff_rgbaf*0.667f)
}, {});
Ui::BaseLayer& blurLayer = ui.setLayerInstance(
    Containers::pointer<Ui::BaseLayerGL>(ui.createLayer(), blurLayerShared));

DOXYGEN_ELLIPSIS()

Ui::NodeHandle background = DOXYGEN_ELLIPSIS({});
blurLayer.create(0, background);
/* [BaseLayer-style-background-blur] */
}

{
Ui::AbstractUserInterface ui{{100, 100}};
/* [BaseLayerStyleAnimator-setup1] */
Containers::Pointer<Ui::BaseLayerStyleAnimator> animatorInstance{InPlaceInit,
    ui.createAnimator()};

Ui::BaseLayerGL::Shared baseLayerShared{
    Ui::BaseLayer::Shared::Configuration{DOXYGEN_ELLIPSIS(0)}
        DOXYGEN_ELLIPSIS()
        .setDynamicStyleCount(10) /* adjust as needed */
};
Ui::BaseLayer& baseLayer = ui.setLayerInstance(
    Containers::pointer<Ui::BaseLayerGL>(ui.createLayer(), baseLayerShared));
/* [BaseLayerStyleAnimator-setup1] */
static_cast<void>(baseLayer);
}

{
Ui::AbstractUserInterface ui{{100, 100}};
/* [DebugLayer-setup] */
ui.setLayerInstance(Containers::pointer<Ui::DebugLayerGL>(
    ui.createLayer(),
    Ui::DebugLayerSource::NodeHierarchy|Ui::DebugLayerSource::NodeData,
    Ui::DebugLayerFlag::NodeHighlight));
/* [DebugLayer-setup] */
}

{
/* [LineLayer-setup-shared] */
Ui::LineLayerGL::Shared lineLayerShared{
    Ui::LineLayer::Shared::Configuration{3}
};
/* [LineLayer-setup-shared] */

Ui::AbstractUserInterface ui{{100, 100}};
/* [LineLayer-setup] */
Ui::LineLayer& lineLayer = ui.setLayerInstance(
    Containers::pointer<Ui::LineLayerGL>(ui.createLayer(), lineLayerShared));
/* [LineLayer-setup] */
static_cast<void>(lineLayer);
}

{
Ui::AbstractUserInterface ui{{100, 100}};
/* [LineLayer-style-cap-join] */
Ui::LineLayerGL::Shared lineLayerSharedRound{
    Ui::LineLayer::Shared::Configuration{DOXYGEN_ELLIPSIS(1)}
        .setCapStyle(Ui::LineCapStyle::Round)
};
Ui::LineLayerGL::Shared lineLayerSharedSquare{
    Ui::LineLayer::Shared::Configuration{DOXYGEN_ELLIPSIS(1)}
        .setCapStyle(Ui::LineCapStyle::Square)
};

Ui::LineLayer& lineLayerRound = ui.setLayerInstance(
    Containers::pointer<Ui::LineLayerGL>(ui.createLayer(), lineLayerSharedRound));
Ui::LineLayer& lineLayerSquare = ui.setLayerInstance(
    Containers::pointer<Ui::LineLayerGL>(ui.createLayer(), lineLayerSharedSquare));
/* [LineLayer-style-cap-join] */
static_cast<void>(lineLayerRound);
static_cast<void>(lineLayerSquare);
}

{
/* [TextLayer-setup-glyph-cache] */
Text::GlyphCacheArrayGL glyphCache{PixelFormat::R8Unorm, {256, 256, 4}};
/* [TextLayer-setup-glyph-cache] */

/* [TextLayer-setup-shared] */
Ui::TextLayerGL::Shared textLayerShared{glyphCache,
    Ui::TextLayer::Shared::Configuration{3}
};
/* [TextLayer-setup-shared] */

Ui::AbstractUserInterface ui{{100, 100}};
/* [TextLayer-setup] */
Ui::TextLayer& textLayer = ui.setLayerInstance(
    Containers::pointer<Ui::TextLayerGL>(ui.createLayer(), textLayerShared));
/* [TextLayer-setup] */
static_cast<void>(textLayer);
}

{
Ui::UserInterfaceGL ui{NoCreate};
Text::GlyphCacheArrayGL glyphCache{PixelFormat::R8Unorm, {8, 8, 1}};
Ui::TextLayerGL::Shared textLayerShared{glyphCache, Ui::TextLayer::Shared::Configuration{1}};
/* [TextLayer-setup-implicit] */
ui.setTextLayerInstance(
    Containers::pointer<Ui::TextLayerGL>(ui.createLayer(), textLayerShared));
/* [TextLayer-setup-implicit] */
}

{
/* [TextLayer-distancefield-setup] */
Text::DistanceFieldGlyphCacheArrayGL glyphCache{{1024, 1024, 4}, {256, 256}, 20};

Ui::TextLayerGL::Shared textLayerShared{glyphCache,
    Ui::TextLayer::Shared::Configuration{3}
};
/* [TextLayer-distancefield-setup] */
}

{
Ui::UserInterfaceGL ui{NoCreate};
Text::DistanceFieldGlyphCacheArrayGL glyphCache{DOXYGEN_ELLIPSIS({}, {}, 0)};
Ui::TextLayerGL::Shared textLayerShared{glyphCache,
    Ui::TextLayer::Shared::Configuration{DOXYGEN_ELLIPSIS(0)}
};
/* [TextLayer-transformation-setup] */
Ui::TextLayerGL& textLayer = ui.setLayerInstance(
    Containers::pointer<Ui::TextLayerGL>(ui.createLayer(), textLayerShared,
                                         Ui::TextLayerFlag::Transformable));
/* [TextLayer-transformation-setup] */
static_cast<void>(textLayer);
}

{
Ui::UserInterfaceGL ui{NoCreate};
Text::GlyphCacheArrayGL glyphCache{PixelFormat::R8Unorm, {8, 8, 1}};
/* [TextLayer-dynamic-styles] */
Ui::TextLayerGL::Shared textLayerShared{glyphCache,
    Ui::TextLayerGL::Shared::Configuration{DOXYGEN_ELLIPSIS(1)}
        .setDynamicStyleCount(10)
};
Ui::TextLayerGL& textLayer = ui.setLayerInstance(
    Containers::pointer<Ui::TextLayerGL>(ui.createLayer(), textLayerShared));

DOXYGEN_ELLIPSIS()

UnsignedInt dynamicStyleId = DOXYGEN_ELLIPSIS(0); /* anything less than the dynamic style count */
textLayer.setDynamicStyle(dynamicStyleId, DOXYGEN_ELLIPSIS(Ui::TextLayerStyleUniform{}, {}, {}, {}, {}));

Ui::NodeHandle node = DOXYGEN_ELLIPSIS({});
textLayer.create(textLayer.shared().styleCount() + dynamicStyleId,
    DOXYGEN_ELLIPSIS("", {}), node);
/* [TextLayer-dynamic-styles] */
}

{
Text::GlyphCacheArrayGL glyphCache{PixelFormat::R8Unorm, {8, 8, 1}};
/* [TextLayer-editing-style-shared] */
Ui::TextLayerGL::Shared textLayerShared{glyphCache,
    Ui::TextLayer::Shared::Configuration{3}
        .setEditingStyleCount(2)
};
/* [TextLayer-editing-style-shared] */
}

{
Ui::AbstractUserInterface ui{{100, 100}};
Text::GlyphCacheArrayGL glyphCache{PixelFormat::R8Unorm, {8, 8, 1}};
/* [TextLayerStyleAnimator-setup1] */
Containers::Pointer<Ui::TextLayerStyleAnimator> animatorInstance{InPlaceInit,
    ui.createAnimator()};

Ui::TextLayerGL::Shared textLayerShared{glyphCache,
    Ui::TextLayer::Shared::Configuration{DOXYGEN_ELLIPSIS(0)}
        DOXYGEN_ELLIPSIS()
        .setDynamicStyleCount(10) /* adjust as needed */
};
Ui::TextLayer& textLayer = ui.setLayerInstance(
    Containers::pointer<Ui::TextLayerGL>(ui.createLayer(), textLayerShared));
/* [TextLayerStyleAnimator-setup1] */
static_cast<void>(textLayer);
}

{
Ui::UserInterfaceGL ui{NoCreate};
/* [EventLayer-setup-implicit] */
ui.setEventLayerInstance(Containers::pointer<Ui::EventLayer>(ui.createLayer()));
/* [EventLayer-setup-implicit] */
}

{
Ui::AbstractUserInterface ui{{100, 100}};
/* [RendererGL-setup] */
ui.setRendererInstance(Containers::pointer<Ui::RendererGL>());
/* [RendererGL-setup] */
}

}
