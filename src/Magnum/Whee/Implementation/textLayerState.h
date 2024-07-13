#ifndef Magnum_Whee_Implementation_textLayerState_h
#define Magnum_Whee_Implementation_textLayerState_h
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

/* Definition of TextLayer::State and TextLayer::Shared::State structs to be
   used by both TextLayer and TextLayerGL as well as TextLayer tests, and (if
   this header gets published) eventually possibly also 3rd party renderer
   implementations */

#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/ArrayTuple.h>
#include <Corrade/Containers/Reference.h>
#include <Magnum/Math/Range.h>
#include <Magnum/Text/AbstractFont.h>
#include <Magnum/Text/AbstractShaper.h>
#include <Magnum/Text/Alignment.h>

#include "Magnum/Whee/TextLayer.h"
#include "Magnum/Whee/TextProperties.h"
#include "Magnum/Whee/Implementation/abstractVisualLayerState.h"

namespace Magnum { namespace Whee {

namespace Implementation {

struct TextLayerFont {
    Containers::Pointer<Text::AbstractFont> fontStorage;
    /* Is null for instance-less fonts */
    Text::AbstractFont* font;
    /* The instance is cached to use for subsequent shaping operations. To keep
       things simple, every Font item has its own even though they might come
       from the same AbstractFont originally. */
    Containers::Pointer<Text::AbstractShaper> shaper;
    /* Size at which to render divided by `font->size()` */
    Float scale;
    UnsignedInt glyphCacheFontId;
};

struct TextLayerStyle {
    /* Uniform index corresponding to given style */
    UnsignedInt uniform;

    FontHandle font;
    Text::Alignment alignment;
    /* 1 byte free */
    /* Points to styleFeatures */
    UnsignedInt featureOffset, featureCount;
    Vector4 padding;
};

}

struct TextLayer::Shared::State: AbstractVisualLayer::Shared::State {
    explicit State(Shared& self, const Configuration& configuration);

    /* First 2/6 bytes overlap with padding of the base struct */

    /* Incremented every time setStyle() is called. There's a corresponding
       styleUpdateStamp variable in TextLayer::State that doState() compares to
       this one, returning LayerState::NeedsDataUpdate if it differs. */
    UnsignedShort styleUpdateStamp = 0;

    /* Can't be inferred from styleUniforms.size() as those are non-empty only
       if dynamicStyleCount is non-zero */
    UnsignedInt styleUniformCount;

    #ifndef CORRADE_NO_ASSERT
    bool setStyleCalled = false;
    /* 3/7 bytes free */
    #endif

    /* Glyph cache used by all fonts. It's expected to know about each font
       that's added. */
    Text::AbstractGlyphCache* glyphCache{};

    /* Fonts. Because a glyph cache doesn't allow glyph removal, they can only
       be added, not removed, thus all extra logic for freed items and
       FontHandle generation counters doesn't need to exist here. */
    Containers::Array<Implementation::TextLayerFont> fonts;

    /* Font features used by all styles. Each style maps into this array using
       TextLayerStyle::featureOffset and featureCount. It's a separate
       allocation from styleStorage because each setStyle() call may be with a
       different total feature count. */
    Containers::Array<TextFeatureValue> styleFeatures;

    Containers::ArrayTuple styleStorage;
    /* Uniform mapping, fonts, alignments, font features and padding values
       assigned to each style */
    Containers::ArrayView<Implementation::TextLayerStyle> styles;
    /* Uniform values to be copied to layer-specific uniform buffers. Empty
       and unused if dynamicStyleCount is 0. */
    Containers::ArrayView<TextLayerStyleUniform> styleUniforms;
    TextLayerCommonStyleUniform commonStyleUniform{NoInit};
};

namespace Implementation {

struct TextLayerGlyphData {
    /* (Aligned) position relative to the node origin */
    Vector2 position;
    /* Cache-global glyph ID */
    UnsignedInt glyphId;
    /* Padding. Currently here only to make it possible to query glyph offset
       + advance data *somewhere* without having to abuse the vertex buffer in
       a nasty way or, worse, temporary allocating. Eventually it could contain
       cluster information for editing/cursor placement, safe-to-break /
       safe-for-ellipsis flags etc. */
    Int:32;
};

struct TextLayerGlyphRun {
    /* If set to ~UnsignedInt{}, given run is unused and gets removed during
       the next recompaction in doUpdate(). */
    UnsignedInt glyphOffset;
    UnsignedInt glyphCount;
    /* Backreference to the `TextLayerData` so the `glyphRun` can be updated
       there when recompacting */
    UnsignedInt data;
};

struct TextLayerTextRun {
    UnsignedInt textOffset;
    UnsignedInt textSize;
    /* Backreference to the `TextLayerData` so the `textRun` can be updated
       there when recompacting */
    UnsignedInt data;
    /* Current editing position */
    UnsignedInt cursor;
    /* The other end of a selection. If less than `cursor`, it's before the
       cursor, if greater it's after, if the same, there's no selection. */
    UnsignedInt selection;

    /* Subset of TextProperties to be used for reshaping the edited text,
       mirroring all packing as well */
    char language[16];
    Text::Script script;
    FontHandle font;
    /* If 0xff, indicates that alignment is not set to avoid an Optional
       wrapper that'd double the field size */
    Text::Alignment alignment;
    /* Packs both shape and layout direction. This is what gets passed to the
       shaper, TextLayerData::shapedDirection is what the shaper returns, which
       may be different after each edit */
    UnsignedByte direction;
};

struct TextLayerData {
    Vector4 padding;
    UnsignedInt glyphRun;
    /* Used only if flags contain TextDataFlag::Editable, otherwise set to
       ~UnsignedInt{} */
    UnsignedInt textRun;
    /* calculatedStyle is filled by AbstractVisualLayer::doUpdate() */
    UnsignedInt style, calculatedStyle;
    /* Ratio of the style size and font size, for appropriately scaling the
       rectangles coming out of the glyph cache */
    Float scale;
    /* Actual rectangle occupied by the text glyphs. Used for cursor /
       selection positioning by the layer itself, in particular to know where
       to position the cursor at the very end, as the glyph run contains only
       offsets of the glyphs, not size of the last glyph. Note that the
       rectangle is returned by Text APIs which have Y up, while the UI library
       uses Y down. The rectangle size is also for use by client code to do
       various sizing and alignment. */
    Range2D rectangle;
    /* Alignment is both to align the glyphs while shaping and to position the
       bounding box relative to the node. Again impossible to change without
       relayouting the text. */
    Text::Alignment alignment;
    /* Actual direction used by the shaper, for direction-aware cursor
       movement in editable text. Unused otherwise, put here instead of inside
       TextLayerTextRun because here was a free space and it's easier to have
       it saved directly after shaping. */
    Text::ShapeDirection usedDirection;
    TextDataFlags flags;
    /* 1 byte free */
    Color3 color;
};

struct TextLayerVertex {
    Vector2 position;
    Vector3 textureCoordinates;
    Color3 color;
    UnsignedInt styleUniform;
};

struct TextLayerDynamicStyle {
    FontHandle font = FontHandle::Null;
    Text::Alignment alignment = Text::Alignment::MiddleCenter;
    /* 1 byte free */
    /* Points to dynamicStyleFeatures */
    UnsignedInt featureOffset, featureCount;
    Vector4 padding;
};

}

struct TextLayer::State: AbstractVisualLayer::State {
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
    /* Used to distinguish between needing an update of the shared part of the
       style (which is triggered by differing styleUpdateStamp) and the dynamic
       part */
    bool dynamicStyleChanged = false;

    /* 3 bytes free */

    /* Glyph / text data. Only the items referenced from `glyphRuns` /
       `textRuns` are valid, the rest is unused space that gets recompacted
       during each doUpdate(). */
    Containers::Array<Implementation::TextLayerGlyphData> glyphData;
    Containers::Array<char> textData;

    /* Glyph / text runs. Each run is a complete text belogning to one text
       layer data. Ordered by the offset. Removed items get marked as unused,
       new items get put at the end, modifying an item means a removal and an
       addition. Gets recompacted during each doUpdate(), this process results
       in the static texts being eventually pushed to the front of the
       buffer (which doesn't need to be updated as often). */
    Containers::Array<Implementation::TextLayerGlyphRun> glyphRuns;
    Containers::Array<Implementation::TextLayerTextRun> textRuns;

    /* Data for each text. Index to `glyphRus` and optionally `textRuns` above,
       a style index and other properties. */
    Containers::Array<Implementation::TextLayerData> data;

    /* Vertex data, ultimately built from `glyphData` combined with color and
       style index from `data` */
    Containers::Array<Implementation::TextLayerVertex> vertices;

    /* Index data, used to draw from `vertices`. In draw order, the
       `indexDrawOffsets` then point into indices for each data in draw
       order. */
    /** @todo any way to make these 16-bit? not really possible in the general
        case given that vertex data get ultimately ordered by frequency of
        change and not by draw order */
    Containers::Array<UnsignedInt> indices;
    Containers::Array<UnsignedInt> indexDrawOffsets;

    /* All these are used only if shared.dynamicStyleCount is non-zero */

    /* Each dynamic style points here with TextLayerDynamicStyle::featureOffset
       and featureCount. It's a separate allocation from dynamicStyleStorage
       because each setDynamicStyle() call may be with a different feature
       count. */
    Containers::Array<TextFeatureValue> dynamicStyleFeatures;

    Containers::ArrayTuple dynamicStyleStorage;
    Containers::ArrayView<TextLayerStyleUniform> dynamicStyleUniforms;
    Containers::ArrayView<Implementation::TextLayerDynamicStyle> dynamicStyles;
};

}}

#endif
