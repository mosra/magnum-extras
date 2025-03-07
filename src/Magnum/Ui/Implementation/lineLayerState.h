#ifndef Magnum_Ui_Implementation_lineLayerState_h
#define Magnum_Ui_Implementation_lineLayerState_h
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

/* Definition of the LineLayer::State struct to be used by both LineLayer and
   LineLayerGL as well as LineLayer tests, and (if this header gets published)
   eventually possibly also 3rd party renderer implementations */

#include <Corrade/Containers/Array.h>

#include "Magnum/Ui/LineLayer.h"
#include "Magnum/Ui/Implementation/abstractVisualLayerState.h"

namespace Magnum { namespace Ui {

namespace Implementation {

struct LineLayerStyle {
    /* Uniform index corresponding to given style */
    UnsignedInt uniform;

    LineAlignment alignment;
    /* 3 bytes free */
    Vector4 padding;
};

}

struct LineLayer::Shared::State: AbstractVisualLayer::Shared::State {
    explicit State(Shared& self, const Configuration& configuration);

    /* First 2/6 bytes overlap with padding of the base struct */

    /* Incremented every time setStyle() is called. There's a corresponding
       styleUpdateStamp variable in LineLayer::State that doState() compares to
       this one, returning LayerState::NeedsDataUpdate if it differs. */
    UnsignedShort styleUpdateStamp = 0;

    #ifndef CORRADE_NO_ASSERT
    bool setStyleCalled = false;
    #endif
    /* 1 byte free, 2 bytes w/ CORRADE_NO_ASSERT */
    LineCapStyle capStyle;
    LineJoinStyle joinStyle;
    UnsignedInt styleUniformCount;
    /* 4 bytes free */

    Containers::ArrayTuple styleStorage;
    /* Uniform mapping, alignment and padding values assigned to each style */
    Containers::ArrayView<Implementation::LineLayerStyle> styles;
};

namespace Implementation {

struct LineLayerPoint {
    Vector2 position;
    Vector4 color;
};

struct LineLayerPointIndex {
    UnsignedInt index;
    /* Index of a neighboring point that isn't a part of the same line segment.
       If set to ~UnsignedInt{}, this is a cap. */
    UnsignedInt neighbor;
};

struct LineLayerRun {
    /* If set to ~UnsignedInt{}, given run is unused and gets removed during
       the next recompaction in doUpdate(). */
    UnsignedInt pointOffset;
    UnsignedInt pointCount;
    UnsignedInt indexOffset;
    UnsignedInt indexCount;
    /* Backreference to the `LineLayerData` so the `run` can be updated there
       when recompacting */
    UnsignedInt data;
    /* Number of indices that have LineLayerPointIndex::neighbor filled. Used
       for calculating index buffer size, each such join is two additional
       triangles. */
    UnsignedInt joinCount;
};

struct LineLayerData {
    UnsignedInt run;
    /* calculatedStyle is filled by AbstractVisualLayer::doUpdate() */
    UnsignedInt style, calculatedStyle;
    /* If 0xff, the alignment coming from style is used. Not using an Optional
       to allow for better padding when more fields are added. */
    LineAlignment alignment;
    /* 3 bytes free */
    Color4 color;
    Vector4 padding;
};

/* Corresponds to Shaders::LineVertexAnnotation, the same constants are then in
   the shader code as well */
enum: UnsignedInt {
    LineVertexAnnotationUp = 1 << 0,
    LineVertexAnnotationJoin = 1 << 1,
    LineVertexAnnotationBegin = 1 << 2
};

struct LineLayerVertex {
    Vector2 position;
    /** @todo use the overlapping layouts eventually */
    Vector2 previousPosition;
    Vector2 nextPosition;
    Color4 color;
    /* First 3 bits used for LineVertexAnnotation* bits from above, the rest is
       (shifted) style uniform index */
    UnsignedInt annotationStyleUniform;
};

}

struct LineLayer::State: AbstractVisualLayer::State {
    explicit State(Shared::State& shared);

    /* First 2/6 bytes overlap with padding of the base struct */

    /* Is compared to Shared::styleUpdateStamp in order to detect that
       doUpdate() needs to be called to update to potentially new mappings
       between styles and uniform IDs, paddings etc. When the two are the same,
       it's assumed all style-dependent data are up-to-date.

       Gets set to the shared value on construction to not implicitly mark a
       fresh layer with no data as immediately needing an update.

       See AbstractVisualLayer::State::styleTransitionToDisabledUpdateStamp for
       discussion about when an update may get skipped by accident. */
    UnsignedShort styleUpdateStamp;

    /* Point data. Only the items referenced from `runs` are valid, the rest is
       unused space that gets recompacted during each doUpdate(). */
    Containers::Array<Implementation::LineLayerPoint> points;
    /* Indices are run-relative, not absolute, so when the runs get recompacted
       they don't need to be updated */
    Containers::Array<Implementation::LineLayerPointIndex> pointIndices;

    /* Runs. Each run is a sequence of indexed points belogning to one line
       layer data. Ordered by the offset. Removed items get marked as unused,
       new items get put at the end, modifying an item if the point / index
       count isn't the same means a removal and an addition. Gets recompacted
       during each doUpdate(), this process results in the static lines being
       eventually pushed to the front of the buffer (which doesn't need to be
       updated as often). */
    Containers::Array<Implementation::LineLayerRun> runs;

    /* Data for each text. Index to `runs` above, a style index and other
       properties. */
    Containers::Array<Implementation::LineLayerData> data;

    /* Vertex data, ultimately built from `pointData` combined with color and
       style index from `data` */
    Containers::Array<Implementation::LineLayerVertex> vertices;

    /* Index data, used to draw from `vertices`. In draw order, the
       `indexDrawOffsets` then point into `indices` for each data in draw
       order. */
    Containers::Array<UnsignedInt> indices;
    Containers::Array<UnsignedInt> indexDrawOffsets;
};

}}

#endif
