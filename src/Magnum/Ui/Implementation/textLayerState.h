#ifndef Magnum_Ui_Implementation_textLayerState_h
#define Magnum_Ui_Implementation_textLayerState_h
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

/* Definition of TextLayer::State and TextLayer::Shared::State structs to be
   used by both TextLayer and TextLayerGL as well as TextLayer tests, and (if
   this header gets published) eventually possibly also 3rd party renderer
   implementations */

#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/Reference.h>
#include <Magnum/Math/Range.h>
#include <Magnum/Text/AbstractFont.h>
#include <Magnum/Text/AbstractShaper.h>
#include <Magnum/Text/Alignment.h>
#include <Magnum/Text/Renderer.h>

#include "Magnum/Ui/TextLayer.h"
#include "Magnum/Ui/TextProperties.h"
#include "Magnum/Ui/Implementation/abstractVisualLayerState.h"

namespace Magnum { namespace Ui {

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
    /* Both point to editingStyles or are -1 */
    Int cursorStyle, selectionStyle;
    Vector4 padding;
};

struct TextLayerEditingStyle {
    /* Uniform index corresponding to given style */
    UnsignedInt uniform;
    /* Uniform index to use for the selected text or -1 */
    Int textUniform;
    Vector4 padding;
};

}

struct TextLayer::Shared::State: AbstractVisualLayer::Shared::State {
    explicit State(Shared& self, Text::AbstractGlyphCache& glyphCache, const Configuration& configuration);

    /* First 2/6 bytes overlap with padding of the base struct */

    /* Incremented every time setStyle() / setEditingStyle() is called. There's
       a corresponding styleUpdateStamp / editingStyleUpdateStamp variable in
       TextLayer::State that doState() compares to this one, returning
       LayerState::NeedsDataUpdate if it differs. */
    UnsignedShort styleUpdateStamp = 0;
    UnsignedShort editingStyleUpdateStamp = 0;

    #ifndef CORRADE_NO_ASSERT
    bool setStyleCalled = false;
    bool setEditingStyleCalled = false;
    #endif

    /* Set to true if there are either static editing styles or, if not,
       dynamic styles with editing styles included */
    bool hasEditingStyles;

    TextLayerSharedFlags flags;

    /* 2 bytes free, 0 bytes free on a no-assert build */

    /* Can't be inferred from {styleUniforms,editingStyleUniforms}.size() as
       those are non-empty only if dynamicStyleCount is non-zero */
    UnsignedInt styleUniformCount, editingStyleUniformCount;

    /* 0/4 bytes free */

    /* Glyph cache used by all fonts. It's expected to know about each font
       that's added. */
    Text::AbstractGlyphCache& glyphCache;

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

    /* Uniform mapping, selection colors and margin values assigned to each
       editing style */
    Containers::ArrayView<Implementation::TextLayerEditingStyle> editingStyles;
    /* Uniform values to be copied to layer-specific uniform buffers. Empty and
       unused if dynamicStyleCount is 0. */
    Containers::ArrayView<TextLayerEditingStyleUniform> editingStyleUniforms;
    TextLayerCommonEditingStyleUniform commonEditingStyleUniform{NoInit};
};

namespace Implementation {

struct TextLayerGlyphData {
    /* (Aligned) position relative to the node origin */
    Vector2 position;
    /* Cache-global glyph ID */
    UnsignedInt glyphId;
    /* Cluster ID for cursor positioning in editable text. Initially abused for
       saving glyph offset + advance (i.e., two Vector2) *somewehere* without
       having to make a temp allocation. If the text is not editable, this
       retains unspecified advance values. */
    UnsignedInt glyphCluster;
};

struct TextLayerGlyphRun {
    /* If set to ~UnsignedInt{}, given run is unused and gets removed during
       the next recompaction in doUpdate(). */
    UnsignedInt glyphOffset;
    UnsignedInt glyphCount;
    /* Backreference to the `TextLayerData` so the `glyphRun` can be updated
       there when recompacting */
    UnsignedInt data;
    /* Ratio of the style size and font size, for appropriately scaling the
       rectangles coming out of the glyph cache */
    Float scale;
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
    /* Set to ~UnsignedInt{} if there are no glyphs */
    UnsignedInt glyphRun;
    /* Used only if flags contain TextDataFlag::Editable, otherwise set to
       ~UnsignedInt{} */
    UnsignedInt textRun;
    /* calculatedStyle is filled by AbstractVisualLayer::doUpdate() */
    UnsignedInt style, calculatedStyle;
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
       movement and cursor styling in editable text. Unused otherwise, put here
       instead of inside TextLayerTextRun because here was a free space and
       it's easier to have it saved directly after shaping. */
    Text::ShapeDirection usedDirection;
    TextDataFlags flags;
    /* 1 byte free */
    Color4 color;
};

struct TextLayerVertex {
    Vector2 position;
    Vector3 textureCoordinates;
    Color4 color;
    UnsignedInt styleUniform;
};

struct TextLayerDistanceFieldVertex {
    /* Has to be a member and not a base class because then is_standard_layout
       complains, making arrayCast() impossible. Sigh. */
    TextLayerVertex vertex;
    /* Scale from the TextLayerGlyphRun but inverted, so the shader knows how
       the distance field value delta maps to actual UI / framebuffer pixels */
    Float invertedRunScale;
};

struct TextLayerEditingVertex {
    Vector2 position;
    Vector2 centerDistance;
    Float opacity;
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

/* Deliberately named differently from TextLayer::dynamicStyleCursorStyle() etc
   to avoid those being called instead by accident */
inline UnsignedInt cursorStyleForDynamicStyle(UnsignedInt id) {
    return 2*id + 1;
}
inline UnsignedInt selectionStyleForDynamicStyle(UnsignedInt id) {
    return 2*id + 0;
}

inline UnsignedInt textUniformForEditingStyle(UnsignedInt dynamicStyleCount, UnsignedInt id) {
    return dynamicStyleCount + id;
}
inline UnsignedInt selectionStyleTextUniformForDynamicStyle(UnsignedInt dynamicStyleCount, UnsignedInt id) {
    return textUniformForEditingStyle(dynamicStyleCount, 2*id + 0);
}
/* textStyleForDynamicCursorStyle would be 2*id + 1 */

}

struct TextLayer::State: AbstractVisualLayer::State {
    explicit State(Shared::State& shared);

    /* First 2/6 bytes overlap with padding of the base struct */

    /* Is compared to Shared::styleUpdateStamp / editingStyleUpdateStamp in
       order to detect that doUpdate() needs to be called to update to
       potentially new mappings between styles and uniform IDs, paddings etc.
       When the two are the same, it's assumed all style-dependent data are
       up-to-date.

       Gets set to the shared value on construction to not implicitly mark a
       fresh layer with no data as immediately needing an update.

       See AbstractVisualLayer::State::styleTransitionToDisabledUpdateStamp for
       discussion about when an update may get skipped by accident. */
    UnsignedShort styleUpdateStamp;
    UnsignedShort editingStyleUpdateStamp;
    /* Used to distinguish between needing an update of the shared part of the
       style (which is triggered by differing styleUpdateStamp) and the dynamic
       (editing) part */
    bool dynamicStyleChanged = false;
    bool dynamicEditingStyleChanged = false;

    /* Glyph / text data. Only the items referenced from `glyphRuns` /
       `textRuns` are valid, the rest is unused space that gets recompacted
       during each doUpdate(). */
    Containers::Array<Implementation::TextLayerGlyphData> glyphData;
    Containers::Array<char> textData;

    /* Text renderer using the shader glyph cache, and with a custom allocator
       that puts data into an appropriate slice of `glyphData` above. The
       second instance is the same except for RendererCoreFlag::GlyphClusters
       enabled for editable text. */
    /** @todo hmm, maybe make that optional in there or some such to avoid
        having two instances? or maybe the edit-aware variant will grow further
        (more per-run info, etc.) that this is warranted? */
    Text::RendererCore renderer;
    Text::RendererCore rendererGlyphClusters;

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
       style index from `data`. Is either Implementation::TextLayerVertex or
       TextLayerDistanceFieldVertex based on whether Flag::DistanceField is
       enabled */
    Containers::Array<char> vertices;
    /* Vertex data for cursor and selection rectangles */
    Containers::Array<Implementation::TextLayerEditingVertex> editingVertices;

    /* Index data, used to draw from `vertices` and `editingVertices`. In draw
       order, the `indexDrawOffsets` then point into `indices` /
       `editingIndices` for each data in draw order. */
    /** @todo any way to make these 16-bit? not really possible in the general
        case given that vertex data get ultimately ordered by frequency of
        change and not by draw order; tho we could maybe assume that there will
        never be more than 8k editable texts with cursor and selection visible
        at the same time? */
    Containers::Array<UnsignedInt> indices;
    Containers::Array<UnsignedInt> editingIndices;
    Containers::Array<Containers::Pair<UnsignedInt, UnsignedInt>> indexDrawOffsets;

    /* All these are used only if shared.dynamicStyleCount is non-zero */

    /* Each dynamic style points here with TextLayerDynamicStyle::featureOffset
       and featureCount. It's a separate allocation from dynamicStyleStorage
       because each setDynamicStyle() call may be with a different feature
       count. */
    Containers::Array<TextFeatureValue> dynamicStyleFeatures;

    Containers::ArrayTuple dynamicStyleStorage;
    /* If dynamic styles include editing styles, the size is
       3*dynamicStyleCount to include uniform overrides for selected text,
       otherwise it's 1*dynamicStyleCount. */
    Containers::ArrayView<TextLayerStyleUniform> dynamicStyleUniforms;
    /* If dynamic styles include editing styles, the size is
       2*dynamicStyleCount, otherwise it's empty */
    Containers::ArrayView<TextLayerEditingStyleUniform> dynamicEditingStyleUniforms;
    Containers::ArrayView<Implementation::TextLayerDynamicStyle> dynamicStyles;
    Containers::MutableBitArrayView dynamicStyleCursorStyles;
    Containers::MutableBitArrayView dynamicStyleSelectionStyles;
    /* If dynamic styles include editing styles, the size is
       2*dynamicStyleCount, otherwise it's empty */
    Containers::ArrayView<Vector4> dynamicEditingStylePaddings;
};

}}

#endif
