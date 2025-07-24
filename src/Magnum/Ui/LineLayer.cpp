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

#include "LineLayer.h"

#include <Corrade/Containers/GrowableArray.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Utility/Algorithms.h>
#include <Magnum/Math/Swizzle.h>

#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/Implementation/lineLayerState.h"
#include "Magnum/Ui/Implementation/lineMiterLimit.h"

namespace Magnum { namespace Ui {

LineLayerStyleUniform& LineLayerStyleUniform::setMiterLengthLimit(const Float limit) {
    miterLimit = Implementation::lineMiterLengthLimit("Ui::LineLayerStyleUniform::setMiterLengthLimit():", limit);
    return *this;
}

LineLayerStyleUniform& LineLayerStyleUniform::setMiterAngleLimit(const Rad limit) {
    miterLimit = Implementation::lineMiterAngleLimit("Ui::LineLayerStyleUniform::setMiterAngleLimit():", limit);
    return *this;
}

Debug& operator<<(Debug& debug, const LineCapStyle value) {
    debug << "Ui::LineCapStyle" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(v) case LineCapStyle::v: return debug << "::" #v;
        _c(Butt)
        _c(Square)
        _c(Round)
        _c(Triangle)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "(" << Debug::nospace << Debug::hex << UnsignedByte(value) << Debug::nospace << ")";
}

Debug& operator<<(Debug& debug, const LineJoinStyle value) {
    debug << "Ui::LineJoinStyle" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(v) case LineJoinStyle::v: return debug << "::" #v;
        _c(Miter)
        _c(Bevel)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "(" << Debug::nospace << Debug::hex << UnsignedByte(value) << Debug::nospace << ")";
}

Debug& operator<<(Debug& debug, const LineAlignment value) {
    const bool packed = debug.immediateFlags() >= Debug::Flag::Packed;

    if(!packed)
        debug << "Ui::LineAlignment" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(v) case LineAlignment::v: return debug << (packed ? "" : "::") << Debug::nospace << #v;
        _c(TopLeft)
        _c(TopCenter)
        _c(TopRight)
        _c(MiddleLeft)
        _c(MiddleCenter)
        _c(MiddleRight)
        _c(BottomLeft)
        _c(BottomCenter)
        _c(BottomRight)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << (packed ? "" : "(") << Debug::nospace << Debug::hex << UnsignedByte(value) << Debug::nospace << (packed ? "" : ")");
}

LineLayer::Shared::State::State(Shared& self, const Configuration& configuration): AbstractVisualLayer::Shared::State{self, configuration.styleCount(), 0}, capStyle{configuration.capStyle()}, joinStyle{configuration.joinStyle()}, styleUniformCount{configuration.styleUniformCount()} {
    styleStorage = Containers::ArrayTuple{
        {NoInit, configuration.styleCount(), styles},
    };
}

LineLayer::Shared::Shared(Containers::Pointer<State>&& state): AbstractVisualLayer::Shared{Utility::move(state)} {}

LineLayer::Shared::Shared(const Configuration& configuration): Shared{Containers::pointer<State>(*this, configuration)} {}

LineLayer::Shared::Shared(NoCreateT) noexcept: AbstractVisualLayer::Shared{NoCreate} {}

UnsignedInt LineLayer::Shared::styleUniformCount() const {
    return static_cast<const State&>(*_state).styleUniformCount;
}

LineCapStyle LineLayer::Shared::capStyle() const {
    return static_cast<const State&>(*_state).capStyle;
}

LineJoinStyle LineLayer::Shared::joinStyle() const {
    return static_cast<const State&>(*_state).joinStyle;
}

void LineLayer::Shared::setStyleInternal(const LineLayerCommonStyleUniform& commonUniform, const Containers::ArrayView<const LineLayerStyleUniform> uniforms, const Containers::StridedArrayView1D<const LineAlignment>& styleAlignments, const Containers::StridedArrayView1D<const Vector4>& stylePaddings) {
    State& state = static_cast<State&>(*_state);
    CORRADE_ASSERT(uniforms.size() == state.styleUniformCount,
        "Ui::LineLayer::Shared::setStyle(): expected" << state.styleUniformCount << "uniforms, got" << uniforms.size(), );
    CORRADE_ASSERT(styleAlignments.size() == state.styleCount,
        "Ui::LineLayer::Shared::setStyle(): expected" << state.styleCount << "alignment values, got" << styleAlignments.size(), );
    CORRADE_ASSERT(stylePaddings.isEmpty() || stylePaddings.size() == state.styleCount,
        "Ui::LineLayer::Shared::setStyle(): expected either no or" << state.styleCount << "paddings, got" << stylePaddings.size(), );
    Utility::copy(styleAlignments, stridedArrayView(state.styles).slice(&Implementation::LineLayerStyle::alignment));
    if(stylePaddings.isEmpty()) {
        /** @todo some Utility::fill() for this */
        for(Implementation::LineLayerStyle& style: state.styles)
            style.padding = {};
    } else {
        Utility::copy(stylePaddings, stridedArrayView(state.styles).slice(&Implementation::LineLayerStyle::padding));
    }

    doSetStyle(commonUniform, uniforms);

    #ifndef CORRADE_NO_ASSERT
    /* Now it's safe to call update() */
    state.setStyleCalled = true;
    #endif

    /* Make doState() of all layers sharing this state return NeedsDataUpdate
       in order to update style-to-uniform mappings and paddings. Setting it
       only if those differ would trigger update only if actually needed, but
       it may be prohibitively expensive compared to updating always. */
    ++state.styleUpdateStamp;
}

LineLayer::Shared& LineLayer::Shared::setStyle(const LineLayerCommonStyleUniform& commonUniform, const Containers::ArrayView<const LineLayerStyleUniform> uniforms, const Containers::StridedArrayView1D<const UnsignedInt>& styleToUniform, const Containers::StridedArrayView1D<const LineAlignment>& styleAlignments, const Containers::StridedArrayView1D<const Vector4>& stylePaddings) {
    State& state = static_cast<State&>(*_state);
    CORRADE_ASSERT(styleToUniform.size() == state.styleCount,
        "Ui::LineLayer::Shared::setStyle(): expected" << state.styleCount << "style uniform indices, got" << styleToUniform.size(), *this);
    setStyleInternal(commonUniform, uniforms, styleAlignments, stylePaddings);
    #ifndef CORRADE_NO_ASSERT
    for(std::size_t i = 0; i != styleToUniform.size(); ++i)
        CORRADE_ASSERT(styleToUniform[i] < state.styleUniformCount,
            "Ui::LineLayer::Shared::setStyle(): uniform index" << styleToUniform[i] << "out of range for" << state.styleUniformCount << "uniforms" << "at index" << i, *this);
    #endif
    Utility::copy(styleToUniform, stridedArrayView(state.styles).slice(&Implementation::LineLayerStyle::uniform));
    return *this;
}

LineLayer::Shared& LineLayer::Shared::setStyle(const LineLayerCommonStyleUniform& commonUniform, const std::initializer_list<LineLayerStyleUniform> uniforms, const std::initializer_list<UnsignedInt> styleToUniform, const std::initializer_list<LineAlignment> styleAlignments, const std::initializer_list<Vector4> stylePaddings) {
    return setStyle(commonUniform, Containers::arrayView(uniforms), Containers::stridedArrayView(styleToUniform), Containers::stridedArrayView(styleAlignments), Containers::stridedArrayView(stylePaddings));
}

LineLayer::Shared& LineLayer::Shared::setStyle(const LineLayerCommonStyleUniform& commonUniform, const Containers::ArrayView<const LineLayerStyleUniform> uniforms, const Containers::StridedArrayView1D<const LineAlignment>& alignments, const Containers::StridedArrayView1D<const Vector4>& paddings) {
    State& state = static_cast<State&>(*_state);
    CORRADE_ASSERT(state.styleUniformCount == state.styleCount,
        "Ui::LineLayer::Shared::setStyle(): there's" << state.styleUniformCount << "uniforms for" << state.styleCount << "styles, provide an explicit mapping", *this);
    setStyleInternal(commonUniform, uniforms, alignments, paddings);
    for(UnsignedInt i = 0; i != state.styleCount; ++i)
        state.styles[i].uniform = i;
    return *this;
}

LineLayer::Shared& LineLayer::Shared::setStyle(const LineLayerCommonStyleUniform& commonUniform, const std::initializer_list<LineLayerStyleUniform> uniforms, const std::initializer_list<LineAlignment> alignments, const std::initializer_list<Vector4> paddings) {
    return setStyle(commonUniform, Containers::arrayView(uniforms), Containers::arrayView(alignments), Containers::arrayView(paddings));
}

LineLayer::Shared::Configuration::Configuration(const UnsignedInt styleUniformCount, const UnsignedInt styleCount): _styleUniformCount{styleUniformCount}, _styleCount{styleCount} {
    CORRADE_ASSERT(styleUniformCount, "Ui::LineLayer::Shared::Configuration: expected non-zero style uniform count", );
    CORRADE_ASSERT(styleCount, "Ui::LineLayer::Shared::Configuration: expected non-zero style count", );
}

LineLayer::State::State(Shared::State& shared): AbstractVisualLayer::State{shared}, styleUpdateStamp{shared.styleUpdateStamp} {}

LineLayer::LineLayer(const LayerHandle handle, Containers::Pointer<State>&& state): AbstractVisualLayer{handle, Utility::move(state)} {}

LineLayer::LineLayer(const LayerHandle handle, Shared& shared): LineLayer{handle, Containers::pointer<State>(static_cast<Shared::State&>(*shared._state))} {}

UnsignedInt LineLayer::createRun(const UnsignedInt dataId, const UnsignedInt indexCount, const UnsignedInt pointCount) {
    State& state = static_cast<State&>(*_state);
    const UnsignedInt run = state.runs.size();
    const UnsignedInt pointOffset = state.points.size();
    const UnsignedInt pointIndexOffset = state.pointIndices.size();
    arrayAppend(state.points, NoInit, pointCount);
    arrayAppend(state.pointIndices, NoInit, indexCount);
    arrayAppend(state.runs, InPlaceInit, pointOffset, pointCount, pointIndexOffset, indexCount, dataId, 0u);
    return run;
}

void LineLayer::fillIndices(const char*
    #ifndef CORRADE_NO_ASSERT
    const messagePrefix
    #endif
, const UnsignedInt dataId, const Containers::StridedArrayView1D<const UnsignedInt>& indices) {
    State& state = static_cast<State&>(*_state);
    Implementation::LineLayerRun& run = state.runs[state.data[dataId].run];

    /* fillStripIndices() and fillLoopIndices() can never trigger this, so the
       assert isn't in createRun() but here */
    CORRADE_ASSERT(indices.size() % 2 == 0,
        messagePrefix << "expected index count to be divisible by 2 but got" << indices.size(), );

    /* Assuming indices are filled before the points and their colors, we can
       abuse the point storage to count number of times each point is used, and
       the neighboring point index for the first two of them. If the point is
       used exactly twice, consider that a line join, if once or more than
       twice, consider that a cap. If the point isn't used at all, it'll
       ultimately stays unused when processing the index buffer in doUpdate()
       later. */
    struct PointUse {
        UnsignedInt count;
        UnsignedInt neighbors[2];
    };
    const Containers::StridedArrayView1D<PointUse> pointUses = Containers::arrayCast<PointUse>(stridedArrayView(state.points.sliceSize(run.pointOffset, run.pointCount)).slice(&Implementation::LineLayerPoint::position));
    for(PointUse& i: pointUses)
        i = {0, {~UnsignedInt{}, ~UnsignedInt{}}};
    for(std::size_t i = 0; i != indices.size(); ++i) {
        CORRADE_DEBUG_ASSERT(indices[i] < run.pointCount,
            messagePrefix << "index" << indices[i] << "out of range for" << run.pointCount << "points" << "at index" << i, );
        /* If this is the second from the index pair, neighbor is the first
           from the pair and vice versa. We're however not storing the point
           index, but the index index, as we ultimately need to reference a
           position in the output vertex stream, which is based on the index
           buffer and not on the original position list. */
        const UnsignedInt neighbor = i & 1 ? i - 1 : i + 1;
        /* It can also happen that the neighbor is the point itself, which is
           in case of singular points. Such a segment is formed by two caps and
           should't have any neighbors, thus skip. */
        if(indices[i] == indices[neighbor])
            continue;
        PointUse& pointUse = pointUses[indices[i]];
        /* If we have so far 1 neighbor and it's the same as this one, skip as
           well -- it'd result in a two-point loop, which isn't really possible
           to render anyway. */
        if(pointUse.count == 1 && indices[pointUse.neighbors[0]] == indices[neighbor])
            continue;
        /* If we have so far 0 or 1 neighbor, save the index of the next one */
        if(pointUse.count < 2)
            pointUse.neighbors[pointUse.count] = neighbor;
        ++pointUse.count;
    }

    /* Copy the indices over, save neighbors of those and collect total count
       of all joins for index buffer sizing in doUpdate(). */
    run.joinCount = 0;
    const Containers::ArrayView<Implementation::LineLayerPointIndex> pointIndices = state.pointIndices.sliceSize(run.indexOffset, run.indexCount);
    for(std::size_t i = 0; i != indices.size(); ++i) {
        pointIndices[i] = {indices[i], ~UnsignedInt{}};
        const PointUse& pointUse = pointUses[indices[i]];

        /* The stored neighbor is always the one that isn't already known from
           the other element of the pair */
        if(pointUse.count == 2) {
            /* Again the neighbor is not the point index but the index index */
            const UnsignedInt knownNeighbor = i & 1 ? i - 1 : i + 1;
            if(pointUse.neighbors[0] != knownNeighbor) {
                pointIndices[i].neighbor = pointUse.neighbors[0];
                ++run.joinCount;
            } else if(pointUse.neighbors[1] != knownNeighbor) {
                pointIndices[i].neighbor = pointUse.neighbors[1];
                ++run.joinCount;
            /* All other cases (e.g. with a singular point where the neighbor
               is the point itself) should have been filtered above. */
            } else CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
        }
    }
}

void LineLayer::fillStripIndices(const char*
    #ifndef CORRADE_NO_ASSERT
    const messagePrefix
    #endif
, const UnsignedInt dataId) {
    State& state = static_cast<State&>(*_state);
    Implementation::LineLayerRun& run = state.runs[state.data[dataId].run];

    /* A 0, 1, 1, 2, 2, 3, ... index sequence. If there are no points, the
       index buffer is empty and the loop will do nothing. If there's just a
       single point, the index buffer size would be calculated as empty, which
       isn't good. Fail in that case. */
    CORRADE_ASSERT(run.indexCount || !run.pointCount,
        messagePrefix << "expected either no or at least two points, got" << run.pointCount, );
    UnsignedInt i = 0;
    for(Implementation::LineLayerPointIndex& index: state.pointIndices.sliceSize(run.indexOffset, run.indexCount)) {
        index.index = (i >> 1) + (i & 1);
        /* Neigbor (which is not a point index but rather an index index, as
           explained in fillIndices() above) points either to the further point
           in the next segment (+2) or the further point in the previous
           segment (-2). Begin / end values will be wrong here, they get
           patched after the loop to avoid branching on every item. */
        index.neighbor = i & 1 ? i + 2 : i - 2;
        ++i;
    }

    /* If there are no points at all, there are no joins either. */
    if(!run.indexCount) {
        run.joinCount = 0;

    /* Otherwise fix up the first and last element of the strip to have no
       neighbor. */
    } else {
        state.pointIndices[run.indexOffset].neighbor =
            state.pointIndices[run.indexOffset + run.indexCount - 1].neighbor =
                ~UnsignedInt{};
        run.joinCount = (run.pointCount - 2)*2;
    }
}

void LineLayer::fillLoopIndices(const char*
    #ifndef CORRADE_NO_ASSERT
    const messagePrefix
    #endif
, const UnsignedInt dataId) {
    State& state = static_cast<State&>(*_state);
    Implementation::LineLayerRun& run = state.runs[state.data[dataId].run];

    /* If there are no points, the index buffer is empty, exit in that case to
       avoid an OOB access. */
    if(!run.indexCount)
        return;

    /* Single point creates a literal point. Two points are an error
       however. */
    CORRADE_ASSERT(run.pointCount != 2,
        messagePrefix << "expected either no, one or at least three points, got" << run.pointCount, );

    /* A 0, 1, 1, 2, 2, 3, ..., n - 1, 0 index sequence */
    UnsignedInt i = 0;
    for(Implementation::LineLayerPointIndex& index: state.pointIndices.sliceSize(run.indexOffset, run.indexCount - 1)) {
        index.index = (i >> 1) + (i & 1);
        /* Neigbor (which is not a point index but rather an index index, as
           explained in fillIndices() above) points either to the further point
           in the next segment (+2) or the further point in the previous
           segment (-2). Begin / end values will be wrong here, they get
           patched after the loop to avoid branching on every item. */
        index.neighbor = i & 1 ? i + 2 : i - 2;
        ++i;
    }
    state.pointIndices[run.indexOffset + run.indexCount - 1].index = 0;

    /* If we have just a single point, it won't have any neighbors */
    if(run.pointCount == 1) {
        state.pointIndices[run.indexOffset].neighbor = ~UnsignedInt{};
        state.pointIndices[run.indexOffset + 1].neighbor = ~UnsignedInt{};
        run.joinCount = 0;

    /* Otherwise make the first and last element neighbor loop together. Two
       points aren't allowed by the above assertion. */
    } else {
        state.pointIndices[run.indexOffset].neighbor = run.indexCount - 2;
        state.pointIndices[run.indexOffset + run.indexCount - 1].neighbor = 1;
        run.joinCount = 2*run.pointCount;
    }
}

void LineLayer::fillPoints(const char*
    #ifndef CORRADE_NO_ASSERT
    const messagePrefix
    #endif
, const UnsignedInt dataId, const Containers::StridedArrayView1D<const Vector2>& points, const Containers::StridedArrayView1D<const Vector4>& colors) {
    CORRADE_ASSERT(colors.isEmpty() || colors.size() == points.size(),
        messagePrefix << "expected either no or" << points.size() << "colors, got" << colors.size(), );

    State& state = static_cast<State&>(*_state);
    const Implementation::LineLayerRun& runData = state.runs[state.data[dataId].run];
    const Containers::StridedArrayView1D<Implementation::LineLayerPoint> pointData = state.points.sliceSize(runData.pointOffset, runData.pointCount);
    Utility::copy(points, pointData.slice(&Implementation::LineLayerPoint::position));
    if(colors.isEmpty())
        for(Implementation::LineLayerPoint& point: pointData)
            point.color = Color4{1.0f};
    else Utility::copy(colors, pointData.slice(&Implementation::LineLayerPoint::color));
}

DataHandle LineLayer::createInternal(const char*
    #ifndef CORRADE_NO_ASSERT
    const messagePrefix
    #endif
, const UnsignedInt style, const UnsignedInt indexCount, const UnsignedInt pointCount, const NodeHandle node) {
    State& state = static_cast<State&>(*_state);
    #ifndef CORRADE_NO_ASSERT
    auto& sharedState = static_cast<Shared::State&>(state.shared);
    #endif
    const DataHandle handle = AbstractLayer::create(node);
    const UnsignedInt id = dataHandleId(handle);
    if(id >= state.data.size()) {
        arrayAppend(state.data, NoInit, id - state.data.size() + 1);
        state.styles = stridedArrayView(state.data).slice(&Implementation::LineLayerData::style);
        state.calculatedStyles = stridedArrayView(state.data).slice(&Implementation::LineLayerData::calculatedStyle);
    }

    /* Add a new run and reference it from the newly created data */
    const UnsignedInt run = createRun(id, indexCount, pointCount);
    Implementation::LineLayerData& data = state.data[id];
    data.run = run;
    data.style = style;
    /* calculatedStyle is filled by AbstractVisualLayer::doUpdate() */
    data.alignment = LineAlignment(0xff);
    data.color = Color4{1.0f};
    data.padding = Vector4{};

    /* Asserting after populating the run and returning the data handle to not
       cause issues in the caller when testing graceful asserts */
    CORRADE_ASSERT(style < sharedState.styleCount + sharedState.dynamicStyleCount,
        messagePrefix << "style" << style << "out of range for" << sharedState.styleCount + sharedState.dynamicStyleCount << "styles", handle);

    return handle;
}

DataHandle LineLayer::create(const UnsignedInt style, const Containers::StridedArrayView1D<const UnsignedInt>& indices, const Containers::StridedArrayView1D<const Vector2>& points, const Containers::StridedArrayView1D<const Vector4>& colors, const NodeHandle node) {
    const DataHandle handle = createInternal("Ui::LineLayer::create():", style, indices.size(), points.size(), node);
    /* Indices abuse the point / color storage for finding neighbors, thus
       points and colors have to be filled only afterwards */
    fillIndices("Ui::LineLayer::create():", dataHandleId(handle), indices);
    fillPoints("Ui::LineLayer::create():", dataHandleId(handle), points, colors);

    return handle;
}

DataHandle LineLayer::create(const UnsignedInt style, const std::initializer_list<UnsignedInt> indices, const std::initializer_list<Vector2> points, std::initializer_list<Color4> colors, const NodeHandle node) {
    return create(style, Containers::stridedArrayView(indices), Containers::stridedArrayView(points), Containers::stridedArrayView(colors), node);
}

DataHandle LineLayer::createStrip(const UnsignedInt style, const Containers::StridedArrayView1D<const Vector2>& points, const Containers::StridedArrayView1D<const Vector4>& colors, const NodeHandle node) {
    const DataHandle handle = createInternal("Ui::LineLayer::createStrip():", style, points.isEmpty() ? 0 : 2*points.size() - 2, points.size(), node);
    fillStripIndices("Ui::LineLayer::createStrip():", dataHandleId(handle));
    fillPoints("Ui::LineLayer::createStrip():", dataHandleId(handle), points, colors);

    return handle;
}

DataHandle LineLayer::createStrip(const UnsignedInt style, const std::initializer_list<Vector2> points, std::initializer_list<Color4> colors, const NodeHandle node) {
    return createStrip(style, Containers::stridedArrayView(points), Containers::stridedArrayView(colors), node);
}

DataHandle LineLayer::createLoop(const UnsignedInt style, const Containers::StridedArrayView1D<const Vector2>& points, const Containers::StridedArrayView1D<const Vector4>& colors, const NodeHandle node) {
    const DataHandle handle = createInternal("Ui::LineLayer::createLoop():", style, 2*points.size(), points.size(), node);
    fillLoopIndices("Ui::LineLayer::createLoop():", dataHandleId(handle));
    fillPoints("Ui::LineLayer::createLoop():", dataHandleId(handle), points, colors);

    return handle;
}

DataHandle LineLayer::createLoop(const UnsignedInt style, const std::initializer_list<Vector2> points, std::initializer_list<Color4> colors, const NodeHandle node) {
    return createLoop(style, Containers::stridedArrayView(points), Containers::stridedArrayView(colors), node);
}

void LineLayer::remove(const DataHandle handle) {
    AbstractVisualLayer::remove(handle);
    removeInternal(dataHandleId(handle));
}

void LineLayer::remove(const LayerDataHandle handle) {
    AbstractVisualLayer::remove(handle);
    removeInternal(layerDataHandleId(handle));
}

void LineLayer::removeInternal(const UnsignedInt id) {
    State& state = static_cast<State&>(*_state);

    /* Mark the run as unused. It'll be removed during the next recompaction in
       doUpdate(). Both `indexOffset` and `pointOffset` are marked to avoid
       inconsistency. */
    state.runs[state.data[id].run].indexOffset = ~UnsignedInt{};
    state.runs[state.data[id].run].pointOffset = ~UnsignedInt{};

    /* Data removal doesn't need anything to be reuploaded to continue working
       correctly, thus setNeedsUpdate() isn't called.

       Which might mean that doing a lot of remove() and then a lot of create()
       with no update() automatically triggered in between can cause high peak
       memory use. However that would happen even if update() was automatically
       scheduled but not actually called between the remove() and create(),
       such as when both happen in the same frame. So calling setNeedsUpdate()
       wouldn't really fully solve that peak memory problem anyway, and on the
       other hand choosing to trigger update() manually after a lot of removals
       can achieve lower peak use than any automagic. */
}

UnsignedInt LineLayer::indexCount(const DataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::LineLayer::indexCount(): invalid handle" << handle, {});
    const State& state = static_cast<const State&>(*_state);
    return state.runs[state.data[dataHandleId(handle)].run].indexCount;
}

UnsignedInt LineLayer::indexCount(const LayerDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::LineLayer::indexCount(): invalid handle" << handle, {});
    const State& state = static_cast<const State&>(*_state);
    return state.runs[state.data[layerDataHandleId(handle)].run].indexCount;
}

UnsignedInt LineLayer::pointCount(const DataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::LineLayer::pointCount(): invalid handle" << handle, {});
    const State& state = static_cast<const State&>(*_state);
    return state.runs[state.data[dataHandleId(handle)].run].pointCount;
}

UnsignedInt LineLayer::pointCount(const LayerDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::LineLayer::pointCount(): invalid handle" << handle, {});
    const State& state = static_cast<const State&>(*_state);
    return state.runs[state.data[layerDataHandleId(handle)].run].pointCount;
}

void LineLayer::setLine(const DataHandle handle, const Containers::StridedArrayView1D<const UnsignedInt>& indices, const Containers::StridedArrayView1D<const Vector2>& points, const Containers::StridedArrayView1D<const Vector4>& colors) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::LineLayer::setLine(): invalid handle" << handle, );
    setLineInternal(dataHandleId(handle), indices, points, colors);
}

void LineLayer::setLine(const DataHandle handle, const std::initializer_list<UnsignedInt> indices, const std::initializer_list<Vector2> points, const std::initializer_list<Color4> colors) {
    setLine(handle, Containers::stridedArrayView(indices), Containers::stridedArrayView(points), Containers::stridedArrayView(colors));
}

void LineLayer::setLine(const LayerDataHandle handle, const Containers::StridedArrayView1D<const UnsignedInt>& indices, const Containers::StridedArrayView1D<const Vector2>& points, const Containers::StridedArrayView1D<const Vector4>& colors) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::LineLayer::setLine(): invalid handle" << handle, );
    setLineInternal(layerDataHandleId(handle), indices, points, colors);
}

void LineLayer::setLine(const LayerDataHandle handle, const std::initializer_list<UnsignedInt> indices, const std::initializer_list<Vector2> points, const std::initializer_list<Color4> colors) {
    setLine(handle, Containers::stridedArrayView(indices), Containers::stridedArrayView(points), Containers::stridedArrayView(colors));
}

void LineLayer::setLineInternal(const UnsignedInt id, const Containers::StridedArrayView1D<const UnsignedInt>& indices, const Containers::StridedArrayView1D<const Vector2>& points, const Containers::StridedArrayView1D<const Vector4>& colors) {
    State& state = static_cast<State&>(*_state);

    /* If the run has the same count of points and indices, reuse it, otherwise
       mark it as unused and create a new one. Compared to the TextLayer, where
       the assumption is that text updates are almost never the same length,
       here it's quite likely. */
    {
        Implementation::LineLayerData& data = state.data[id];;
        Implementation::LineLayerRun& run = state.runs[data.run];
        if(run.indexCount != indices.size() || run.pointCount != points.size()) {
            /* The run will be removed during the next recompaction in
               doUpdate(). Both `indexOffset` and `pointOffset` are marked to
               avoid inconsistency. */
            run.indexOffset = ~UnsignedInt{};
            run.pointOffset = ~UnsignedInt{};

            data.run = createRun(id, indices.size(), points.size());
        }
    }

    /* Fill the run with new indices, points and colors. Indices abuse the
       point / color storage for finding neighbors, thus points and colors have
       to be filled only afterwards. */
    fillIndices("Ui::LineLayer::setLine():", id, indices);
    fillPoints("Ui::LineLayer::setLine():", id, points, colors);

    setNeedsUpdate(LayerState::NeedsDataUpdate);
}

void LineLayer::setLineStrip(const DataHandle handle, const Containers::StridedArrayView1D<const Vector2>& points, const Containers::StridedArrayView1D<const Vector4>& colors) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::LineLayer::setLineStrip(): invalid handle" << handle, );
    setLineStripInternal(dataHandleId(handle), points, colors);
}

void LineLayer::setLineStrip(const DataHandle handle, const std::initializer_list<Vector2> points, const std::initializer_list<Color4> colors) {
    setLineStrip(handle, Containers::stridedArrayView(points), Containers::stridedArrayView(colors));
}

void LineLayer::setLineStrip(const LayerDataHandle handle, const Containers::StridedArrayView1D<const Vector2>& points, const Containers::StridedArrayView1D<const Vector4>& colors) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::LineLayer::setLineStrip(): invalid handle" << handle, );
    setLineStripInternal(layerDataHandleId(handle), points, colors);
}

void LineLayer::setLineStrip(const LayerDataHandle handle, const std::initializer_list<Vector2> points, const std::initializer_list<Color4> colors) {
    setLineStrip(handle, Containers::stridedArrayView(points), Containers::stridedArrayView(colors));
}

void LineLayer::setLineStripInternal(const UnsignedInt id, const Containers::StridedArrayView1D<const Vector2>& points, const Containers::StridedArrayView1D<const Vector4>& colors) {
    State& state = static_cast<State&>(*_state);

    /* If the run has the same count of points and indices, reuse it, otherwise
       mark it as unused and create a new one. Compared to the TextLayer, where
       the assumption is that text updates are almost never the same length,
       here it's quite likely. */
    {
        Implementation::LineLayerData& data = state.data[id];;
        Implementation::LineLayerRun& run = state.runs[data.run];
        const UnsignedInt indexCount = points.isEmpty() ? 0 : points.size()*2 - 2;
        if(run.indexCount != indexCount || run.pointCount != points.size()) {
            /* The run will be removed during the next recompaction in
               doUpdate(). Both `indexOffset` and `pointOffset` are marked to
               avoid inconsistency. */
            run.indexOffset = ~UnsignedInt{};
            run.pointOffset = ~UnsignedInt{};

            data.run = createRun(id, indexCount, points.size());
        }
    }

    /* Fill the run with new strip indices, points and colors. We *may* have a
       strip index buffer present already, in which case this could be a no-op,
       but tracking that would be extra complexity with questinable
       benefits. */
    fillPoints("Ui::LineLayer::setLineStrip():", id, points, colors);
    fillStripIndices("Ui::LineLayer::setLineStrip():", id);

    setNeedsUpdate(LayerState::NeedsDataUpdate);
}

void LineLayer::setLineLoop(const DataHandle handle, const Containers::StridedArrayView1D<const Vector2>& points, const Containers::StridedArrayView1D<const Vector4>& colors) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::LineLayer::setLineLoop(): invalid handle" << handle, );
    setLineLoopInternal(dataHandleId(handle), points, colors);
}

void LineLayer::setLineLoop(const DataHandle handle, const std::initializer_list<Vector2> points, const std::initializer_list<Color4> colors) {
    setLineLoop(handle, Containers::stridedArrayView(points), Containers::stridedArrayView(colors));
}

void LineLayer::setLineLoop(const LayerDataHandle handle, const Containers::StridedArrayView1D<const Vector2>& points, const Containers::StridedArrayView1D<const Vector4>& colors) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::LineLayer::setLineLoop(): invalid handle" << handle, );
    setLineLoopInternal(layerDataHandleId(handle), points, colors);
}

void LineLayer::setLineLoop(const LayerDataHandle handle, const std::initializer_list<Vector2> points, const std::initializer_list<Color4> colors) {
    setLineLoop(handle, Containers::stridedArrayView(points), Containers::stridedArrayView(colors));
}

void LineLayer::setLineLoopInternal(const UnsignedInt id, const Containers::StridedArrayView1D<const Vector2>& points, const Containers::StridedArrayView1D<const Vector4>& colors) {
    State& state = static_cast<State&>(*_state);

    /* If the run has the same count of points and indices, reuse it, otherwise
       mark it as unused and create a new one. Compared to the TextLayer, where
       the assumption is that text updates are almost never the same length,
       here it's quite likely. */
    {
        Implementation::LineLayerData& data = state.data[id];;
        Implementation::LineLayerRun& run = state.runs[data.run];
        const UnsignedInt indexCount = points.size()*2;
        if(run.indexCount != indexCount || run.pointCount != points.size()) {
            /* The run will be removed during the next recompaction in
               doUpdate(). Both `indexOffset` and `pointOffset` are marked to
               avoid inconsistency. */
            run.indexOffset = ~UnsignedInt{};
            run.pointOffset = ~UnsignedInt{};

            data.run = createRun(id, indexCount, points.size());
        }
    }

    /* Fill the run with new loop indices, points and colors. We *may* have a
       loop index buffer present already, in which case this could be a no-op,
       but tracking that would be extra complexity with questinable
       benefits. */
    fillLoopIndices("Ui::LineLayer::setLineLoop():", id);
    fillPoints("Ui::LineLayer::setLineLoop():", id, points, colors);

    setNeedsUpdate(LayerState::NeedsDataUpdate);
}

Color4 LineLayer::color(const DataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::LineLayer::color(): invalid handle" << handle, {});
    return static_cast<const State&>(*_state).data[dataHandleId(handle)].color;
}

Color4 LineLayer::color(const LayerDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::LineLayer::color(): invalid handle" << handle, {});
    return static_cast<const State&>(*_state).data[layerDataHandleId(handle)].color;
}

void LineLayer::setColor(const DataHandle handle, const Color4& color) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::LineLayer::setColor(): invalid handle" << handle, );
    setColorInternal(dataHandleId(handle), color);
}

void LineLayer::setColor(const LayerDataHandle handle, const Color4& color) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::LineLayer::setColor(): invalid handle" << handle, );
    setColorInternal(layerDataHandleId(handle), color);
}

void LineLayer::setColorInternal(const UnsignedInt id, const Color4& color) {
    static_cast<State&>(*_state).data[id].color = color;
    setNeedsUpdate(LayerState::NeedsDataUpdate);
}

Containers::Optional<LineAlignment> LineLayer::alignment(const DataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::LineLayer::alignment(): invalid handle" << handle, {});
    return alignmentInternal(dataHandleId(handle));
}

Containers::Optional<LineAlignment> LineLayer::alignment(const LayerDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::LineLayer::alignment(): invalid handle" << handle, {});
    return alignmentInternal(layerDataHandleId(handle));
}

Containers::Optional<LineAlignment> LineLayer::alignmentInternal(const UnsignedInt id) const {
    const LineAlignment alignment = static_cast<const State&>(*_state).data[id].alignment;
    return alignment == LineAlignment(0xff) ?
        Containers::NullOpt : Containers::optional(alignment);
}

void LineLayer::setAlignment(const DataHandle handle, const Containers::Optional<LineAlignment> alignment) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::LineLayer::setAlignment(): invalid handle" << handle, );
    setAlignmentInternal(dataHandleId(handle), alignment);
}

void LineLayer::setAlignment(const LayerDataHandle handle, const Containers::Optional<LineAlignment> alignment) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::LineLayer::setAlignment(): invalid handle" << handle, );
    setAlignmentInternal(layerDataHandleId(handle), alignment);
}

void LineLayer::setAlignmentInternal(const UnsignedInt id, const Containers::Optional<LineAlignment> alignment) {
    static_cast<State&>(*_state).data[id].alignment = alignment ? *alignment : LineAlignment(0xff);
    setNeedsUpdate(LayerState::NeedsDataUpdate);
}

Vector4 LineLayer::padding(const DataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::LineLayer::padding(): invalid handle" << handle, {});
    return static_cast<const State&>(*_state).data[dataHandleId(handle)].padding;
}

Vector4 LineLayer::padding(const LayerDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::LineLayer::padding(): invalid handle" << handle, {});
    return static_cast<const State&>(*_state).data[layerDataHandleId(handle)].padding;
}

void LineLayer::setPadding(const DataHandle handle, const Vector4& padding) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::LineLayer::setPadding(): invalid handle" << handle, );
    setPaddingInternal(dataHandleId(handle), padding);
}

void LineLayer::setPadding(const LayerDataHandle handle, const Vector4& padding) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::LineLayer::setPadding(): invalid handle" << handle, );
    setPaddingInternal(layerDataHandleId(handle), padding);
}

void LineLayer::setPaddingInternal(const UnsignedInt id, const Vector4& padding) {
    static_cast<State&>(*_state).data[id].padding = padding;
    setNeedsUpdate(LayerState::NeedsDataUpdate);
}

LayerFeatures LineLayer::doFeatures() const {
    return AbstractVisualLayer::doFeatures()|LayerFeature::Draw;
}

LayerStates LineLayer::doState() const {
    LayerStates states = AbstractVisualLayer::doState();

    auto& state = static_cast<const State&>(*_state);
    auto& sharedState = static_cast<const Shared::State&>(state.shared);
    if(state.styleUpdateStamp != sharedState.styleUpdateStamp) {
        /* Needed because uniform mapping, paddings and alignment can change */
        states |= LayerState::NeedsDataUpdate;
    }
    return states;
}

void LineLayer::doClean(const Containers::BitArrayView dataIdsToRemove) {
    /* Mark runs attached to removed data as unused, similarly as when calling
       remove(). They'll get actually removed during the next recompaction in
       doUpdate(). */
    /** @todo some way to iterate set bits */
    for(std::size_t i = 0; i != dataIdsToRemove.size(); ++i)
        if(dataIdsToRemove[i])
            removeInternal(i);

    /* Data removal doesn't need anything to be reuploaded to continue working
       correctly, thus setNeedsUpdate() isn't called, and neither is in
       remove(). See a comment there for more information. */
}

void LineLayer::doUpdate(const LayerStates states, const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, const Containers::StridedArrayView1D<const Float>& nodeOpacities, const Containers::BitArrayView nodesEnabled, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes, const Containers::StridedArrayView1D<const Vector2>& compositeRectOffsets, const Containers::StridedArrayView1D<const Vector2>& compositeRectSizes) {
    /* The base implementation populates data.calculatedStyle */
    AbstractVisualLayer::doUpdate(states, dataIds, clipRectIds, clipRectDataCounts, nodeOffsets, nodeSizes, nodeOpacities, nodesEnabled, clipRectOffsets, clipRectSizes, compositeRectOffsets, compositeRectSizes);

    auto& state = static_cast<State&>(*_state);
    auto& sharedState = static_cast<Shared::State&>(state.shared);
    /* Technically needed only if there's any actual data to update, but
       require it always for consistency (and easier testing) */
    CORRADE_ASSERT(sharedState.setStyleCalled,
        "Ui::LineLayer::update(): no style data was set", );

    /* Recompact the line data by removing unused runs. Do this only if data
       actually change, this isn't affected by anything node-related */
    /** @todo further restrict this to just NeedsCommonDataUpdate which gets
        set by setLine*(), remove() etc that actually produces unused runs, but
        not setColor() and such? the recompaction however implies a need to
        update the actual index buffer etc anyway, so a dedicated state won't
        make that update any smaller, and we'd now trigger it from clean() and
        remove() as well, which we didn't need to before */
    if(states >= LayerState::NeedsDataUpdate) {
        std::size_t outputPointIndexOffset = 0;
        std::size_t outputPointOffset = 0;
        std::size_t outputRunOffset = 0;
        for(std::size_t i = 0; i != state.runs.size(); ++i) {
            Implementation::LineLayerRun& run = state.runs[i];
            /* If a run is marked as unused, but the index and point offset
               should have it for consistency */
            CORRADE_INTERNAL_DEBUG_ASSERT((run.indexOffset == ~UnsignedInt{}) == (run.pointOffset == ~UnsignedInt{}));
            if(run.indexOffset == ~UnsignedInt{})
                continue;

            /* Move the index data earlier if there were skipped runs before,
               update the reference to it in the run */
            if(run.indexOffset != outputPointIndexOffset) {
                CORRADE_INTERNAL_DEBUG_ASSERT(run.indexOffset > outputPointIndexOffset);
                CORRADE_INTERNAL_DEBUG_ASSERT(i != outputRunOffset);

                std::memmove(state.pointIndices.data() + outputPointIndexOffset,
                             state.pointIndices.data() + run.indexOffset,
                             run.indexCount*sizeof(Implementation::LineLayerPointIndex));
                run.indexOffset = outputPointIndexOffset;
            }
            outputPointIndexOffset += run.indexCount;

            /* Same for point data. Note that there may be runs with non-zero
               points but zero indices so this has to be checked independently
               of the indexOffset. */
            if(run.pointOffset != outputPointOffset) {
                CORRADE_INTERNAL_DEBUG_ASSERT(run.pointOffset > outputPointOffset);
                CORRADE_INTERNAL_DEBUG_ASSERT(i != outputRunOffset);

                std::memmove(state.points.data() + outputPointOffset,
                             state.points.data() + run.pointOffset,
                             run.pointCount*sizeof(Implementation::LineLayerPoint));
                run.pointOffset = outputPointOffset;
            }
            outputPointOffset += run.pointCount;

            /* Move the glyph run info earlier if there were skipped runs
               before, update the reference to it in the data */
            if(i != outputRunOffset) {
                CORRADE_INTERNAL_DEBUG_ASSERT(i > outputRunOffset);
                state.data[run.data].run = outputRunOffset;
                state.runs[outputRunOffset] = run;
            }
            ++outputRunOffset;
        }

        /* Remove the now-unused data from the end */
        CORRADE_INTERNAL_ASSERT(outputPointIndexOffset <= state.pointIndices.size());
        CORRADE_INTERNAL_ASSERT(outputPointOffset <= state.points.size());
        CORRADE_INTERNAL_ASSERT(outputRunOffset <= state.runs.size());
        arrayResize(state.pointIndices, outputPointIndexOffset);
        arrayResize(state.points, outputPointOffset);
        arrayResize(state.runs, outputRunOffset);
    }

    /* Fill in indices in desired order if either the data themselves or the
       node order changed. Keep the checks in sync with
       LineLayerGL::doUpdate(). */
    if(states >= LayerState::NeedsNodeOrderUpdate ||
       states >= LayerState::NeedsDataUpdate)
    {
        /* Index offsets for each run, plus one more for the last run */
        arrayResize(state.indexDrawOffsets, NoInit, dataIds.size() + 1);

        /* Calculate how many line segments and joins we'll draw */
        UnsignedInt drawSegmentCount = 0;
        UnsignedInt drawJoinCount = 0;
        for(const UnsignedInt id: dataIds) {
            const Implementation::LineLayerData& data = state.data[id];
            const Implementation::LineLayerRun& run = state.runs[data.run];
            /* Every two indices is one segment */
            CORRADE_INTERNAL_DEBUG_ASSERT(run.indexCount % 2 == 0);
            drawSegmentCount += run.indexCount/2;
            drawJoinCount += run.joinCount;
        }

        /* Generate index data */
        arrayResize(state.indices, NoInit, drawSegmentCount*6 + drawJoinCount*3);
        UnsignedInt indexOffset = 0;
        for(std::size_t i = 0; i != dataIds.size(); ++i) {
            const Implementation::LineLayerData& data = state.data[dataIds[i]];
            const Implementation::LineLayerRun& run = state.runs[data.run];

            /* Generate indices in draw order. Remeber the offset for each data
               to draw from later. */
            state.indexDrawOffsets[i] = indexOffset;
            const Containers::ArrayView<const Implementation::LineLayerPointIndex> pointIndices = state.pointIndices.sliceSize(run.indexOffset, run.indexCount);
            /* Every two input indices is one segment, every segment is six
               output indices, every pair of joins is two triangles */
            const Containers::ArrayView<UnsignedInt> indexData = state.indices.sliceSize(indexOffset, (run.indexCount/2)*6 + run.joinCount*3);

            /* The order is chosen in a way that makes it possible to interpret
               the 6 indices as 3 lines instead of 2 triangles, and
               additionally those forming only one line, with the other two
               degenerating to an invisible point to avoid overlaps that would
               break blending.

                0---2 2
                |  / /|       0---2
                | / / |
                |/ /  |      11   32
                1 1---3

               This matches what's done in MeshTools::generateLines(). */
            std::size_t runIndexOffset = 0;
            /* The output vertices are in the order defined by the input index
               buffer, and for every pair of input indices defining a line
               segment we have four output vertices */
            const UnsignedInt vertexOffset = (run.indexOffset/2)*4;
            for(std::size_t j = 0, jMax = run.indexCount/2; j != jMax; ++j) {
                const UnsignedInt segmentVertexOffset = vertexOffset + j*4;

                indexData[runIndexOffset++] = segmentVertexOffset + 2;
                indexData[runIndexOffset++] = segmentVertexOffset + 0;
                indexData[runIndexOffset++] = segmentVertexOffset + 1;

                indexData[runIndexOffset++] = segmentVertexOffset + 1;
                indexData[runIndexOffset++] = segmentVertexOffset + 3;
                indexData[runIndexOffset++] = segmentVertexOffset + 2;

                /* Add also indices for the bevel (one will always degenerate).
                   For the line fallback these will all degenerate.

                    2 2   2---n n    n--
                     /|   |  / /|    |        23    nn
                    / |   | / / |    | /
                      |   |/ /  |    |/          3n+1
                    --3   3 3--n+1   n+1

                   Again matches what's done in MeshTools::generateLines().
                   However, there are always two ends of two segments both
                   marked as two sides of the same join, and adding one
                   triangle for each could lead to cases where both get
                   rendered instead of one always degenerating. Thus pick only
                   one of them by choosing one where the neighbor index is
                   larger than the point index itself, and add two triangles
                   for it, while the other side will have no triangle. */
                for(std::size_t k: {0, 1}) {
                    if(pointIndices[j*2 + k].neighbor == ~UnsignedInt{} || pointIndices[j*2 + k].neighbor <= j*2 + k)
                        continue;

                    /* The neighbor index is pointing to the farther end of the
                       joined segment while we need the closer. If the neighbor
                       is the second index of the segment index pair, the
                       closer point is in the first, and vice versa. */
                    const UnsignedInt neighbor = pointIndices[j*2 + k].neighbor;
                    const UnsignedInt joinVertexOffset = vertexOffset +
                        (neighbor & 1 ? neighbor - 1 : neighbor + 1)*2;

                    indexData[runIndexOffset++] = segmentVertexOffset + 2;
                    indexData[runIndexOffset++] = segmentVertexOffset + 3;
                    indexData[runIndexOffset++] = joinVertexOffset + 0;

                    indexData[runIndexOffset++] = joinVertexOffset + 0;
                    indexData[runIndexOffset++] = segmentVertexOffset + 3;
                    indexData[runIndexOffset++] = joinVertexOffset + 1;
                }
            }

            CORRADE_INTERNAL_DEBUG_ASSERT(runIndexOffset == indexData.size());
            indexOffset += indexData.size();
        }

        CORRADE_INTERNAL_ASSERT(indexOffset == drawSegmentCount*6 + drawJoinCount*3);
        state.indexDrawOffsets[dataIds.size()] = indexOffset;
    }

    /* Fill in vertex data if the data themselves, the node offset/size or node
       enablement (and thus calculated styles) or opacities (and thus
       calculated colors) changed. Keep the checks in sync with
       LineLayerGL::doUpdate(). */
    /** @todo split this further to just position-related data update and other
        data if it shows to help with perf */
    if(states >= LayerState::NeedsNodeOffsetSizeUpdate ||
       states >= LayerState::NeedsNodeEnabledUpdate ||
       states >= LayerState::NeedsNodeOpacityUpdate ||
       states >= LayerState::NeedsDataUpdate)
    {
        /* Calculate how many points are there in total. For each segment
           defined by the input index buffer we'll have two points, so
           basically removing the indexing, and then further duplicating them
           to form quads. */
        UnsignedInt totalPointCount = 0;
        for(const Implementation::LineLayerRun& run: state.runs)
            totalPointCount += run.indexCount;

        const Containers::StridedArrayView1D<const Ui::NodeHandle> nodes = this->nodes();

        /* Generate vertex data */
        arrayResize(state.vertices, NoInit, totalPointCount*2);
        for(const UnsignedInt dataId: dataIds) {
            const UnsignedInt nodeId = nodeHandleId(nodes[dataId]);
            const Implementation::LineLayerData& data = state.data[dataId];
            const Implementation::LineLayerRun& run = state.runs[data.run];

            /* Fill in vertices in the same order as the original runs */
            /** @todo ideally this would only be done if some text actually
                changes, not on every visibility change */
            const Containers::ArrayView<const Implementation::LineLayerPointIndex> pointIndices = state.pointIndices.sliceSize(run.indexOffset, run.indexCount);
            const Containers::StridedArrayView1D<const Implementation::LineLayerPoint> points = state.points.sliceSize(run.pointOffset, run.pointCount);
            const Containers::StridedArrayView1D<Implementation::LineLayerVertex> vertexData = state.vertices.sliceSize(run.indexOffset*2, run.indexCount*2);
            for(std::size_t i = 0, iMax = run.indexCount; i != iMax; ++i) {
                /* Position and color is the same for both copies of the
                   segment endpoint */
                vertexData[i*2 + 0].position =
                    vertexData[i*2 + 1].position =
                        points[pointIndices[i].index].position;
                vertexData[i*2 + 0].color =
                    vertexData[i*2 + 1].color =
                        points[pointIndices[i].index].color;

                /* If this is the first index of the pair, it's marked as a
                   Begin. The previous position is from the potential connected
                   neighbor, if any, the next position is the second position
                   in the pair. */
                if((i & 1) == 0) {
                    vertexData[i*2 + 0].annotationStyleUniform =
                        vertexData[i*2 + 1].annotationStyleUniform =
                            Implementation::LineVertexAnnotationBegin;
                    vertexData[i*2 + 0].previousPosition =
                        vertexData[i*2 + 1].previousPosition =
                            pointIndices[i].neighbor != ~UnsignedInt{} ?
                                /* The neigbor is not the point index but the
                                   index index as we need to know its position
                                   in the output vertex stream in the index
                                   buffer population above, thus there's one
                                   extra indirection */
                                points[pointIndices[pointIndices[i].neighbor].index].position : Vector2{};
                    vertexData[i*2 + 0].nextPosition =
                        vertexData[i*2 + 1].nextPosition =
                            points[pointIndices[i + 1].index].position;

                /* If this is the second index of the pair, it's not marked as
                   Begin. The previous position is the first position in the
                   pair, the next position is from the potential connected
                   neigbor, if any. */
                } else {
                    vertexData[i*2 + 0].annotationStyleUniform =
                        vertexData[i*2 + 1].annotationStyleUniform =
                            0;
                    vertexData[i*2 + 0].previousPosition =
                        vertexData[i*2 + 1].previousPosition =
                            points[pointIndices[i - 1].index].position;
                    vertexData[i*2 + 0].nextPosition =
                        vertexData[i*2 + 1].nextPosition =
                            pointIndices[i].neighbor != ~UnsignedInt{} ?
                                /* The neigbor is not the point index but the
                                   index index as we need to know its position
                                   in the output vertex stream in the index
                                   buffer population above, thus there's one
                                   extra indirection */
                                points[pointIndices[pointIndices[i].neighbor].index].position : Vector2{};
                }

                /* First of the two copies of the segment endpoint gets marked
                   as Up. Additionally, if there's a connected neighbor, it's
                   marked as Join. */
                vertexData[i*2 + 0].annotationStyleUniform |=
                    Implementation::LineVertexAnnotationUp;
                if(pointIndices[i].neighbor != ~UnsignedInt{}) {
                    vertexData[i*2 + 0].annotationStyleUniform |=
                        Implementation::LineVertexAnnotationJoin;
                    vertexData[i*2 + 1].annotationStyleUniform |=
                        Implementation::LineVertexAnnotationJoin;
                }
            }

            /* Align the run relative to the node area */
            const Vector4 padding = data.padding + sharedState.styles[data.calculatedStyle].padding;
            Vector2 offset = nodeOffsets[nodeId] + padding.xy();
            const Vector2 size = nodeSizes[nodeId] - padding.xy() - Math::gather<'z', 'w'>(padding);
            /* If per-data alignment is set, use that, otherwise take one from
               the style */
            const LineAlignment alignment = data.alignment != LineAlignment(0xff) ?
                data.alignment : sharedState.styles[data.calculatedStyle].alignment;
            const UnsignedByte alignmentHorizontal = UnsignedByte(alignment) & Implementation::LineAlignmentHorizontal;
            if(alignmentHorizontal == Implementation::LineAlignmentLeft) {
                offset.x() += 0.0f;
            } else if(alignmentHorizontal == Implementation::LineAlignmentRight) {
                offset.x() += size.x();
            } else if(alignmentHorizontal == Implementation::LineAlignmentCenter) {
                offset.x() += size.x()*0.5f;
            } else CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
            const UnsignedByte alignmentVertical = UnsignedByte(alignment) & Implementation::LineAlignmentVertical;
            if(alignmentVertical == Implementation::LineAlignmentTop) {
                offset.y() += 0.0f;
            } else if(alignmentVertical == Implementation::LineAlignmentBottom) {
                offset.y() += size.y();
            } else if(alignmentVertical == Implementation::LineAlignmentMiddle) {
                offset.y() += size.y()*0.5f;
            } else CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */

            /* Translate the (aligned) run, fill color and style */
            const Float opacity = nodeOpacities[nodeId];
            for(Implementation::LineLayerVertex& vertex: vertexData) {
                vertex.position += offset;
                vertex.previousPosition += offset;
                vertex.nextPosition += offset;
                vertex.color *= data.color*opacity;
                /* Annotation is the lower 3 bits, style index is above that */
                vertex.annotationStyleUniform |= sharedState.styles[data.calculatedStyle].uniform << 3;
            }
        }
    }

    /* Sync the style update stamp to not have doState() return NeedsDataUpdate
       again next time it's asked */
    if(states >= LayerState::NeedsDataUpdate)
        state.styleUpdateStamp = sharedState.styleUpdateStamp;
}

}}
