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

#include <Corrade/Containers/BitArrayView.h>
#include <Corrade/Containers/GrowableArray.h>
#include <Corrade/Containers/Reference.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Utility/Arguments.h>
#include <Magnum/Mesh.h>
#include <Magnum/VertexFormat.h>
#include <Magnum/DebugTools/ColorMap.h>
#include <Magnum/DebugTools/FrameProfiler.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/Math/Matrix3.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/ConfigurationValue.h>
#ifdef CORRADE_TARGET_EMSCRIPTEN
#include <Magnum/Platform/EmscriptenApplication.h>
#else
#include <Magnum/Platform/Sdl2Application.h>
#endif
#include <Magnum/Shaders/FlatGL.h>

#include "Magnum/Whee/AbstractLayer.h"
#include "Magnum/Whee/AbstractUserInterface.h"
#include "Magnum/Whee/Event.h"
#include "Magnum/Whee/Handle.h"
#include "Magnum/Whee/RendererGL.h"

namespace Magnum { namespace Whee { namespace Test { namespace {

class StressTest: public Platform::Application {
    public:
        explicit StressTest(const Arguments& arguments);

    private:
        void drawEvent() override;

        AbstractUserInterface _ui;
        LayerHandle _firstLayer;
        bool _triggerDataUpdate,
            _triggerNodeClipUpdate,
            _triggerNodeLayoutUpdate,
            _triggerNodeUpdate;

        DebugTools::FrameProfilerGL _profiler;
};

using namespace Containers::Literals;
using namespace Math::Literals;

class Layer: public AbstractLayer {
    public:
        explicit Layer(LayerHandle handle, bool skipVertexDataUpdate, bool skipIndexDataUpdate, bool events);

        DataHandle create(const Color4ub& color, NodeHandle node);

    private:
        LayerFeatures doFeatures() const override {
            return LayerFeature::Draw|(_advertiseEvents ? LayerFeature::Event : LayerFeatures{});
        }

        void doSetSize(const Vector2& size, const Vector2i&) override {
            const Matrix3 projection =
                Matrix3::scaling({1.0f, -1.0f})*
                Matrix3::translation({-1.0f, -1.0f})*
                Matrix3::projection(size);
            _projectionSize = size;
            _shader
                .setTransformationProjectionMatrix(projection);
        }

        void doUpdate(LayerStates states, const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, Containers::BitArrayView nodesEnabled, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes, const Containers::StridedArrayView1D<const Vector2>& compositeRectOffsets, const Containers::StridedArrayView1D<const Vector2>& compositeRectSizes) override;

        void doDraw(const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, std::size_t offset, std::size_t count, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, std::size_t clipRectOffset, std::size_t clipRectCount, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, Containers::BitArrayView nodesEnabled, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes) override;

        Containers::Array<Color4ub> _colors;
        Containers::Array<UnsignedInt> _indices;
        struct Vertex {
            Vector2us position;
            Color4ub color;
        };
        Containers::Array<Vertex> _vertices;
        Vector2 _projectionSize;

        bool _skipVertexDataUpdate, _skipIndexDataUpdate, _advertiseEvents;
        GL::Mesh _mesh;
        GL::Buffer _vertexBuffer{GL::Buffer::TargetHint::Array}, _indexBuffer{GL::Buffer::TargetHint::ElementArray};
        Shaders::FlatGL2D _shader{Shaders::FlatGL2D::Configuration{}
            .setFlags(Shaders::FlatGL2D::Flag::VertexColor)};
};

Layer::Layer(LayerHandle handle, bool skipVertexDataUpdate, bool skipIndexDataUpdate, bool advertiseEvents): AbstractLayer{handle}, _skipVertexDataUpdate{skipVertexDataUpdate}, _skipIndexDataUpdate{skipIndexDataUpdate}, _advertiseEvents{advertiseEvents} {
    _mesh.addVertexBuffer(_vertexBuffer, 0,
        Shaders::FlatGL2D::Position{
            Shaders::FlatGL2D::Position::DataType::UnsignedShort},
        Shaders::FlatGL2D::Color4{
            Shaders::FlatGL2D::Color4::DataType::UnsignedByte,
            Shaders::FlatGL2D::Color4::DataOption::Normalized
        });
    _mesh.setIndexBuffer(_indexBuffer, 0, MeshIndexType::UnsignedInt);
}

DataHandle Layer::create(const Color4ub& color, NodeHandle node) {
    const DataHandle handle = AbstractLayer::create(node);
    const UnsignedInt id = dataHandleId(handle);
    if(id >= _colors.size())
        arrayAppend(_colors, NoInit, id - _colors.size() + 1);

    _colors[id] = color;
    return handle;
}

void Layer::doUpdate(LayerStates, const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, Containers::BitArrayView, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&) {
    /* Fill in indices in desired order. Skip if already filled and
       index data update isn't desired, only index updates. */
    if(_indices.size() != dataIds.size()*6 || !_skipIndexDataUpdate) {
        arrayResize(_indices, NoInit, dataIds.size()*6);
        for(UnsignedInt i = 0; i != dataIds.size(); ++i) {
            const UnsignedInt v = dataIds[i];

            /* 0---1 0---2 5
               |   | |  / /|
               |   | | / / |
               |   | |/ /  |
               2---3 1 3---4 */
            _indices[i*6 + 0] = v*4 + 0;
            _indices[i*6 + 1] = v*4 + 2;
            _indices[i*6 + 2] = v*4 + 1;
            _indices[i*6 + 3] = v*4 + 2;
            _indices[i*6 + 4] = v*4 + 3;
            _indices[i*6 + 5] = v*4 + 1;
        }
        _indexBuffer.setData(_indices);
        _mesh.setCount(_indices.size());
    }

    const Containers::StridedArrayView1D<const NodeHandle> nodes = this->nodes();

    /* Fill in quad corner positions and colors. Skip if already filled and
       vertex data update isn't desired. */
    if(_vertices.size() != capacity()*4 || !_skipVertexDataUpdate) {
        arrayResize(_vertices, NoInit, capacity()*4);
        for(std::size_t i = 0; i != dataIds.size(); ++i) {
            const UnsignedInt dataId = dataIds[i];

            /* 0---1
               |   |
               |   |
               |   |
               2---3 */
            const Vector2us min = Vector2us{nodeOffsets[nodeHandleId(nodes[dataIds[i]])]};
            const Vector2us max = min + Vector2us{nodeSizes[nodeHandleId(nodes[dataIds[i]])]};

            for(UnsignedByte j = 0; j != 4; ++j) {
                /* ✨ */
                _vertices[dataId*4 + j].position = Math::lerp(min, max, BitVector2{j});
                _vertices[dataId*4 + j].color = _colors[dataId];
            }
        }

        _vertexBuffer.setData(_vertices);
    }
}

void Layer::doDraw(const Containers::StridedArrayView1D<const UnsignedInt>&, std::size_t offset, std::size_t count, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const UnsignedInt>&, std::size_t, std::size_t, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, Containers::BitArrayView, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&) {
    _mesh
        .setIndexOffset(offset*6)
        .setCount(count*6);
    _shader
        .draw(_mesh);
}

StressTest::StressTest(const Arguments& arguments): Platform::Application{arguments, NoCreate}, _ui{NoCreate} {
    Utility::Arguments args;
    args.addSkippedPrefix("magnum", "engine-specific options")
        .addBooleanOption("data-update").setHelp("data-update", "trigger NeedsDataUpdate every frame")
        /** @todo drop once there's a distinction between data, position and
            visible set change */
        .addBooleanOption("skip-vertex-data-update").setHelp("skip-vertex-data-update", "skip vertex data update")
        .addBooleanOption("skip-index-data-update").setHelp("skip-index-data-update", "skip index data update")
        .addBooleanOption("advertise-events").setHelp("advertise-events", "advertise (but don't handle) events on the main layer")
        .addBooleanOption("node-clip-update").setHelp("node-clip-update", "trigger NeedsNodeClipUpdate update every frame")
        .addBooleanOption("node-layout-update").setHelp("node-layout-update", "trigger NeedsNodeLayoutUpdate update every frame")
        .addBooleanOption("node-update").setHelp("node-update", "trigger NeedsNodeUpdate every frame")
        .addOption("clip", "1.0").setHelp("clip", "clip to only a part of the view", "RATIO")
        /** @todo other triggers */
        .addOption("size", "1000 1000").setHelp("size", "node grid size")
        .addOption("count", "1").setHelp("count", "count of data per node")
        .parse(arguments.argc, arguments.argv);

    _triggerDataUpdate = args.isSet("data-update");
    _triggerNodeClipUpdate = args.isSet("node-clip-update");
    _triggerNodeLayoutUpdate = args.isSet("node-layout-update");
    _triggerNodeUpdate = args.isSet("node-update");
    bool skipVertexDataUpdate = args.isSet("skip-vertex-data-update");
    bool skipIndexDataUpdate = args.isSet("skip-index-data-update");
    bool advertiseEvents = args.isSet("advertise-events");

    const Vector2ui size = args.value<Vector2ui>("size");
    if(size.product() > 1000000)
        Fatal{} << "At most a million nodes is allowed, got" << size;

    const UnsignedInt count = args.value<UnsignedInt>("count");
    if(count > 128)
        Fatal{} << "At most 128 layers is allowed, got" << count;

    create(Configuration{}
        .setTitle("Magnum::Whee Stress Test"_s));

    _profiler = DebugTools::FrameProfilerGL{
        DebugTools::FrameProfilerGL::Value::FrameTime|
        DebugTools::FrameProfilerGL::Value::GpuDuration|
        DebugTools::FrameProfilerGL::Value::CpuDuration, 50};

    _ui
        .setSize(Vector2{size}*args.value<Float>("clip"), Vector2{windowSize()}, framebufferSize())
        .setRendererInstance(Containers::pointer<Whee::RendererGL>());

    Containers::Array<Containers::Reference<Layer>> layers;
    for(UnsignedInt i = 0; i != count*2; ++i)
        arrayAppend(layers,  _ui.setLayerInstance(Containers::pointer<Layer>(_ui.createLayer(), skipVertexDataUpdate, skipIndexDataUpdate, advertiseEvents)));
    _firstLayer = layers[0]->handle();

    const Containers::StaticArrayView<256, const Vector3ub> colors = DebugTools::ColorMap::turbo();

    NodeHandle window = _ui.createNode({}, _ui.size());
    NodeHandle view = _ui.createNode(window, {}, _ui.size());

    UnsignedInt i = 0;
    for(UnsignedInt y = 0; y != size.y(); ++y) {
        for(UnsignedInt x = 0; x != size.x()/2; ++x) {
            NodeHandle node = _ui.createNode(view, {Float(x)*2, Float(y)}, {2.0f, 1.0f});
            NodeHandle nodeSub = _ui.createNode(node, {0.0f, 0.0f}, {1.0f, 1.0f});
            Color4ub color = colors[(i*117) % colors.size()];
            ColorHsv hsv = color.toHsv();
            for(UnsignedInt j = 0; j != count; ++j)
                layers[j]->create(color, node);
            for(UnsignedInt j = 0; j != count; ++j)
                layers[count + j]->create(Color4ub::fromHsv({hsv.hue, hsv.saturation*0.25f, hsv.value}), nodeSub);
            ++i;
        }
    }

    UnsignedInt capacity = 0;
    for(UnsignedInt j = 0; j != count*2; ++j)
        capacity += layers[j]->capacity();

    Debug{} << _ui.nodeCapacity() << "nodes total," << capacity << "data attachments";

    #ifndef CORRADE_TARGET_EMSCRIPTEN
    setSwapInterval(0);
    #endif
}

void StressTest::drawEvent() {
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color|GL::FramebufferClear::Depth);

    _profiler.beginFrame();

    NodeHandle node = nodeHandle(Math::min(std::size_t{56}, _ui.nodeCapacity() - 1), 1);
    if(_triggerNodeUpdate)
        _ui.setNodeFlags(node, ~_ui.nodeFlags(node));
    else if(_triggerNodeClipUpdate)
        _ui.setNodeSize(nodeHandle(0, 1), _ui.nodeSize(nodeHandle(0, 1)));
    else if(_triggerNodeLayoutUpdate)
        _ui.setNodeOffset(nodeHandle(0, 1), _ui.nodeOffset(nodeHandle(0, 1)));
    else if(_triggerDataUpdate)
        _ui.layer<Layer>(_firstLayer).setNeedsUpdate(LayerState::NeedsDataUpdate);

    _ui.draw();

    _profiler.endFrame();
    _profiler.printStatistics(50);

    swapBuffers();
    redraw();
}

}}}}

MAGNUM_APPLICATION_MAIN(Magnum::Whee::Test::StressTest)
