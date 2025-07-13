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

#include <new>
/* for keyTextEventSynthesizedFromPointerPress(), an "integration test" */
#include <Corrade/Containers/Function.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Containers/StridedBitArrayView.h>
#include <Corrade/Containers/String.h>
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/Container.h>
#include <Corrade/TestSuite/Compare/Numeric.h>
#include <Corrade/TestSuite/Compare/String.h>
#include <Corrade/Utility/Algorithms.h>
#include <Corrade/Utility/Format.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/Math/Matrix3.h>
#include <Magnum/Math/Range.h>
#include <Magnum/Text/AbstractGlyphCache.h>
#include <Magnum/Text/Alignment.h>
#include <Magnum/Text/Direction.h>
#include <Magnum/Text/Feature.h>
#include <Magnum/Text/Script.h>

#include "Magnum/Ui/AbstractUserInterface.h"
#include "Magnum/Ui/Event.h"
/* for keyTextEventSynthesizedFromPointerPress(), an "integration test" */
#include "Magnum/Ui/EventLayer.h"
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/NodeFlags.h"
#include "Magnum/Ui/TextLayer.h"
#include "Magnum/Ui/TextProperties.h"
/* for dynamicStyle(), createRemoveSetText(), updateCleanDataOrder(),
   updateAlignment(), updateAlignmentGlyph(), updatePadding() and
   updatePaddingGlyph() */
#include "Magnum/Ui/Implementation/textLayerState.h"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct TextLayerTest: TestSuite::Tester {
    explicit TextLayerTest();

    template<class T> void styleUniformSizeAlignment();

    void styleUniformCommonConstructDefault();
    void styleUniformCommonConstruct();
    void styleUniformCommonConstructNoInit();
    void styleUniformCommonSetters();

    void styleUniformConstructDefault();
    void styleUniformConstruct();
    void styleUniformConstructDistanceField();
    void styleUniformConstructNoInit();
    void styleUniformSetters();

    void editingStyleUniformCommonConstructDefault();
    void editingStyleUniformCommonConstruct();
    void editingStyleUniformCommonConstructNoInit();
    void editingStyleUniformCommonSetters();

    void editingStyleUniformConstructDefault();
    void editingStyleUniformConstruct();
    void editingStyleUniformConstructNoInit();
    void editingStyleUniformSetters();

    void fontHandle();
    void fontHandleInvalid();
    void debugFontHandle();
    void debugFontHandlePacked();

    void debugLayerFlag();
    void debugLayerFlags();
    void debugDataFlag();
    void debugDataFlagPacked();
    void debugDataFlags();
    void debugDataFlagsPacked();
    void debugEdit();

    void sharedDebugFlag();
    void sharedDebugFlags();

    void sharedConfigurationConstruct();
    void sharedConfigurationConstructSameStyleUniformCount();
    void sharedConfigurationConstructZeroStyleOrUniformCount();
    void sharedConfigurationConstructCopy();
    void sharedConfigurationSetters();
    void sharedConfigurationSettersSameEditingStyleUniformCount();
    void sharedConfigurationSettersInvalidEditingStyleOrUniformCount();

    void sharedConstruct();
    void sharedConstructNoCreate();
    void sharedConstructCopy();
    void sharedConstructMove();
    void sharedConstructZeroStyleCount();

    void sharedAddFont();
    void sharedAddFontTakeOwnership();
    void sharedAddFontTakeOwnershipNull();
    void sharedAddFontNotFoundInCache();
    void sharedAddFontNoHandlesLeft();
    void sharedAddInstancelessFontHasInstance();
    void sharedFontInvalidHandle();
    void sharedFontNoInstance();

    void sharedSetStyle();
    void sharedSetStyleImplicitFeatures();
    void sharedSetStyleImplicitEditingStyles();
    void sharedSetStyleImplicitPadding();
    void sharedSetStyleInvalidSize();
    void sharedSetStyleInvalidMapping();
    void sharedSetStyleImplicitMapping();
    void sharedSetStyleImplicitMappingImplicitFeatures();
    void sharedSetStyleImplicitMappingImplicitEditingStyles();
    void sharedSetStyleImplicitMappingImplicitPadding();
    void sharedSetStyleImplicitMappingInvalidSize();
    void sharedSetStyleInvalidFontHandle();
    void sharedSetStyleInvalidAlignment();
    void sharedSetStyleInvalidFeatures();
    void sharedSetStyleInvalidEditingStyles();

    void sharedSetEditingStyle();
    void sharedSetEditingStyleImplicitTextUniforms();
    void sharedSetEditingStyleInvalidSize();
    void sharedSetEditingStyleInvalidMapping();
    void sharedSetEditingStyleImplicitMapping();
    void sharedSetEditingStyleImplicitMappingImplicitTextUniforms();
    void sharedSetEditingStyleImplicitMappingInvalidSize();

    void construct();
    void constructCopy();
    void constructMove();

    void dynamicStyle();
    void dynamicStyleFeatureAllocation();
    void dynamicStyleEditingStyles();
    void dynamicStyleNoDynamicStyles();
    void dynamicStyleInvalid();

    /* remove() and setText() tested here as well */
    template<class StyleIndex, class GlyphIndex> void createRemoveSet();
    void createRemoveHandleRecycle();
    void createStyleOutOfRange();
    void createNoStyleSet();

    void setCursor();
    void setCursorInvalid();
    void updateText();
    void updateTextInvalid();
    void editText();
    void editTextInvalid();

    void cycleGlyphEditableNonEditableText();

    void createSetTextTextProperties();
    void createSetTextTextPropertiesEditable();
    void createSetTextTextPropertiesEditableInvalid();

    void createSetUpdateTextFromLayerItself();

    void setColor();
    void setPadding();
    void setPaddingInvalid();
    void setTransformation();
    void setTransformationInvalid();

    void invalidHandle();
    void invalidHandleTransformation();
    void invalidFontHandle();
    void nonEditableText();
    void nonEditableTextTransformation();
    void noSharedStyleFonts();
    void noFontInstance();
    void glyphOutOfRange();

    /* assignAnimator(), setDefaultStyleAnimator() and advanceAnimations()
       tested in TextLayerStyleAnimatorTest */

    void updateEmpty();
    void updateCleanDataOrder();
    void updateAlignment();
    void updateAlignmentGlyph();
    void updatePadding();
    void updatePaddingGlyph();
    void updateTransformation();
    void updateNoStyleSet();
    void updateNoEditingStyleSet();

    void sharedNeedsUpdateStatePropagatedToLayers();

    void keyTextEvent();
    void keyTextEventSynthesizedFromPointerPress();
};

using namespace Math::Literals;

const struct {
    const char* name;
    UnsignedInt dynamicStyleCount;
} SharedSetStyleData[]{
    {"", 0},
    {"dynamic styles", 17}
};

const struct {
    const char* name;
    TextLayerFlags layerFlags;
} ConstructData[]{
    {"", {}},
    {"transformable", TextLayerFlag::Transformable}
};

const struct {
    const char* name;
    bool changeFont;
    Text::Alignment alignment1, alignment2;
    Containers::Array<TextFeatureValue> features1, features2;
    Vector4 padding1, padding2;
    LayerStates expectedStates;
} DynamicStyleData[]{
    {"default font, alignment, features and padding",
        false,
        Text::Alignment::MiddleCenter, Text::Alignment::MiddleCenter,
        {}, {},
        {}, {},
        LayerState::NeedsCommonDataUpdate},
    {"different font, default alignment, features and padding",
        /* Doesn't cause NeedsUpdate as it's impossible to change a font of an
           already laid out text, have to set it again in that case */
        true,
        Text::Alignment::MiddleCenter, Text::Alignment::MiddleCenter,
        {}, {},
        {}, {},
        LayerState::NeedsCommonDataUpdate},
    {"different alignment, default font, features and padding",
        /* Similarly, doesn't cause NeedsUpdate as it's impossible to change
           alignment of an already laid out text (it'd have to remember line
           breaks to align each line differently, etc.) */
        false,
        Text::Alignment::LineLeft, Text::Alignment::TopRight,
        {}, {},
        {}, {},
        LayerState::NeedsCommonDataUpdate},
    {"different features, default font, alignment and padding",
        false,
        Text::Alignment::MiddleCenter, Text::Alignment::MiddleCenter,
        {InPlaceInit, {
            Text::Feature::SlashedZero,
            Text::Feature::TabularFigures,
        }}, {InPlaceInit, {
            {Text::Feature::Kerning, false}
        }},
        {}, {},
        LayerState::NeedsCommonDataUpdate},
    {"different font, alignment and features, default padding",
        /* The three above combined */
        true,
        Text::Alignment::TopRight, Text::Alignment::LineLeft,
        {InPlaceInit, {
            Text::Feature::SlashedZero,
            Text::Feature::TabularFigures,
        }}, {InPlaceInit, {
            {Text::Feature::Kerning, false}
        }},
        {}, {},
        LayerState::NeedsCommonDataUpdate},
    {"default font, alignment and features, non-zero padding",
        false,
        Text::Alignment::MiddleCenter, Text::Alignment::MiddleCenter,
        {}, {},
        {3.5f, 0.5f, 1.5f, 2.5f}, Vector4{2.0f},
        LayerState::NeedsCommonDataUpdate|LayerState::NeedsDataUpdate},
    {"different font, default alignment and features, non-zero padding",
        true,
        Text::Alignment::MiddleCenter, Text::Alignment::MiddleCenter,
        {}, {},
        {3.5f, 0.5f, 1.5f, 2.5f}, Vector4{2.0f},
        LayerState::NeedsCommonDataUpdate|LayerState::NeedsDataUpdate},
    {"different alignment, default font and features, non-zero padding",
        false,
        Text::Alignment::MiddleCenterIntegral, Text::Alignment::TopLeft,
        {}, {},
        {3.5f, 0.5f, 1.5f, 2.5f}, Vector4{2.0f},
        LayerState::NeedsCommonDataUpdate|LayerState::NeedsDataUpdate},
    {"different features, default font and alignment, non-zero padding",
        false,
        Text::Alignment::MiddleCenterIntegral, Text::Alignment::TopLeft,
        {InPlaceInit, {
            {Text::Feature::Kerning, false}
        }}, {InPlaceInit, {
            Text::Feature::SlashedZero,
            Text::Feature::TabularFigures,
        }},
        {3.5f, 0.5f, 1.5f, 2.5f}, Vector4{2.0f},
        LayerState::NeedsCommonDataUpdate|LayerState::NeedsDataUpdate},
    {"different font, alignment and features, non-zero padding",
        true,
        Text::Alignment::TopLeft, Text::Alignment::MiddleCenterIntegral,
        {InPlaceInit, {
            {Text::Feature::Kerning, false}
        }}, {InPlaceInit, {
            Text::Feature::SlashedZero,
            Text::Feature::TabularFigures,
        }},
        {3.5f, 0.5f, 1.5f, 2.5f}, Vector4{2.0f},
        LayerState::NeedsCommonDataUpdate|LayerState::NeedsDataUpdate},
};

const struct {
    TestSuite::TestCaseDescriptionSourceLocation name;
    Vector4 padding1, padding2;
    Containers::Optional<Vector4> cursorPadding1, cursorPadding2;
    Containers::Optional<Vector4> selectionPadding1, selectionPadding2;
    bool textUniform1, textUniform2;
    LayerStates expectedStates;
} DynamicStyleEditingStylesData[]{
    {"cursor style, everything stays the same",
        {}, {},
        /* The values are default-constructed but that doesn't matter, it
           should still cause an update for the first time at least, due to the
           style not being with associated cursor style before */
        Vector4{}, Vector4{},
        {}, {},
        false, false,
        LayerState::NeedsCommonDataUpdate},
    {"cursor style, base padding different",
        {}, {0.0f, 0.0f, 1.0f, 0.0f},
        Vector4{1.0f, 2.0f, 3.0f, 4.0f}, Vector4{1.0f, 2.0f, 3.0f, 4.0f},
        {}, {},
        false, false,
        LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate},
    {"cursor style, cursor padding different",
        {}, {},
        Vector4{1.0f, 2.0f, 3.0f, 4.0f}, Vector4{1.0f, 2.0f, 4.0f, 4.0f},
        {}, {},
        false, false,
        LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate},
    {"selection style, everything stays the same",
        {}, {},
        {}, {},
        /* The values are default-constructed but that doesn't matter, it
           should still cause an update for the first time at least, due to the
           style not being with associated selection style before */
        Vector4{}, Vector4{},
        false, false,
        LayerState::NeedsCommonDataUpdate},
    {"selection style, base padding different",
        {}, {0.0f, 0.0f, 1.0f, 0.0f},
        {}, {},
        Vector4{1.0f, 2.0f, 3.0f, 4.0f}, Vector4{1.0f, 2.0f, 3.0f, 4.0f},
        true, true,
        LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate},
    {"selection style, selection padding different",
        {}, {},
        {}, {},
        Vector4{1.0f, 2.0f, 3.0f, 4.0f}, Vector4{1.0f, 2.0f, 4.0f, 4.0f},
        true, true,
        LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate},
    {"selection style, text uniform becomes set",
        {}, {},
        {}, {},
        Vector4{1.0f, 2.0f, 3.0f, 4.0f}, Vector4{1.0f, 2.0f, 3.0f, 4.0f},
        false, true,
        /* This shouldn't cause a data update, the uniform ID is the same, it's
           just the contents getting different */
        LayerState::NeedsCommonDataUpdate},
    {"selection style, text uniform becomes unset",
        {}, {},
        {}, {},
        Vector4{1.0f, 2.0f, 3.0f, 4.0f}, Vector4{1.0f, 2.0f, 3.0f, 4.0f},
        true, false,
        /* This shouldn't cause a data update, the uniform ID is the same, it's
           just the contents getting different */
        LayerState::NeedsCommonDataUpdate},
    {"cursor + selection style, everything stays the same",
        {}, {},
        /* The values are default-constructed but that doesn't matter, it
           should still cause an update for the first time at least, due to the
           style not being with associated cursor + selection style before */
        Vector4{}, Vector4{},
        Vector4{}, Vector4{},
        false, false,
        LayerState::NeedsCommonDataUpdate},
    {"cursor + selection style, base padding different",
        {}, {0.0f, 0.0f, 1.0f, 0.0f},
        Vector4{1.0f, 2.0f, 3.0f, 4.0f}, Vector4{1.0f, 2.0f, 3.0f, 4.0f},
        Vector4{5.0f, 6.0f, 7.0f, 8.0f}, Vector4{5.0f, 6.0f, 7.0f, 8.0f},
        true, true,
        LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate},
    {"cursor + selection style, cursor padding different",
        {}, {},
        Vector4{1.0f, 2.0f, 3.0f, 4.0f}, Vector4{1.0f, 2.0f, 3.0f, 4.0f},
        Vector4{5.0f, 6.0f, 7.0f, 8.0f}, Vector4{5.0f, 6.0f, 7.0f, 7.0f},
        true, true,
        LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate},
    {"cursor + selection style, selection padding different",
        {}, {},
        Vector4{1.0f, 2.0f, 3.0f, 4.0f}, Vector4{1.0f, 2.0f, 4.0f, 4.0f},
        Vector4{5.0f, 6.0f, 7.0f, 8.0f}, Vector4{5.0f, 6.0f, 7.0f, 8.0f},
        true, true,
        LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate},
    {"cursor + selection style, text uniform becomes set",
        {}, {},
        Vector4{1.0f, 2.0f, 3.0f, 4.0f}, Vector4{1.0f, 2.0f, 3.0f, 4.0f},
        Vector4{5.0f, 6.0f, 7.0f, 8.0f}, Vector4{5.0f, 6.0f, 7.0f, 8.0f},
        false, true,
        /* This shouldn't cause a data update, the uniform ID is the same, it's
           just the contents getting different */
        LayerState::NeedsCommonDataUpdate},
    {"cursor + selection style, text uniform becomes unset",
        {}, {},
        Vector4{1.0f, 2.0f, 3.0f, 4.0f}, Vector4{1.0f, 2.0f, 3.0f, 4.0f},
        Vector4{5.0f, 6.0f, 7.0f, 8.0f}, Vector4{5.0f, 6.0f, 7.0f, 8.0f},
        true, false,
        /* This shouldn't cause a data update, the uniform ID is the same, it's
           just the contents getting different */
        LayerState::NeedsCommonDataUpdate},
    {"cursor style becomes cursor + selection style",
        {}, {},
        /* The values are default-constructed but that doesn't matter, it
           should still cause an update */
        Vector4{}, Vector4{},
        {}, Vector4{},
        false, false,
        LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate},
    {"cursor style becomes selection style",
        {}, {},
        /* The values are default-constructed but that doesn't matter, it
           should still cause an update */
        Vector4{}, {},
        {}, Vector4{},
        false, false,
        LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate},
    {"cursor style becomes no editing style",
        {}, {},
        /* The value is default-constructed but that doesn't matter, it should
           still cause an update */
        Vector4{}, {},
        {}, {},
        false, false,
        LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate},
    {"selection style becomes cursor + selection style",
        {}, {},
        /* The values are default-constructed but that doesn't matter, it
           should still cause an update */
        {}, Vector4{},
        Vector4{}, Vector4{},
        false, false,
        LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate},
    {"selection style becomes cursor style",
        {}, {},
        /* The values are default-constructed but that doesn't matter, it
           should still cause an update */
        {}, Vector4{},
        Vector4{}, {},
        false, false,
        LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate},
    {"selection style becomes no editing style",
        {}, {},
        {}, {},
        /* The value is default-constructed but that doesn't matter, it should
           still cause an update */
        Vector4{}, {},
        false, false,
        LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate},
    {"cursor + selection style becomes cursor style",
        {}, {},
        /* The values are default-constructed but that doesn't matter, it
           should still cause an update */
        Vector4{}, Vector4{},
        Vector4{}, {},
        false, false,
        LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate},
    {"cursor + selection style becomes cursor style",
        {}, {},
        /* The values are default-constructed but that doesn't matter, it
           should still cause an update */
        Vector4{}, {},
        Vector4{}, Vector4{},
        false, false,
        LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate},
    {"cursor + selection style becomes no editing style",
        {}, {},
        /* The values are default-constructed but that doesn't matter, it
           should still cause an update */
        Vector4{}, {},
        Vector4{}, {},
        false, false,
        LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate},
};

enum class Enum: UnsignedShort {};

Debug& operator<<(Debug& debug, Enum value) {
    return debug << UnsignedInt(value);
}

const struct {
    const char* name;
    NodeHandle node;
    LayerStates state;
    bool layerDataHandleOverloads, customFont, customAlignment, nullStyleFonts;
    UnsignedInt styleCount, dynamicStyleCount;
    TextLayerFlags layerFlags;
    Containers::Optional<TextDataFlags> flags;
} CreateRemoveSetData[]{
    {"create",
        NodeHandle::Null, LayerState::NeedsDataUpdate,
        false, false, false, false, 3, 0, {}, {}},
    {"create and attach",
        nodeHandle(9872, 0xbeb), LayerState::NeedsNodeOffsetSizeUpdate|LayerState::NeedsAttachmentUpdate|LayerState::NeedsDataUpdate,
        false, false, false, false, 3, 0, {}, {}},
    {"LayerDataHandle overloads",
        NodeHandle::Null, LayerState::NeedsDataUpdate,
        true, false, false, false, 3, 0, {}, {}},
    {"custom fonts",
        NodeHandle::Null, LayerState::NeedsDataUpdate,
        false, true, false, false, 3, 0, {}, {}},
    {"custom fonts, null style fonts",
        NodeHandle::Null, LayerState::NeedsDataUpdate,
        false, true, false, true, 3, 0, {}, {}},
    {"custom fonts, LayerDataHandle overloads",
        NodeHandle::Null, LayerState::NeedsDataUpdate,
        true, true, false, false, 3, 0, {}, {}},
    {"custom alignment",
        NodeHandle::Null, LayerState::NeedsDataUpdate,
        false, false, true, false, 3, 0, {}, {}},
    {"dynamic styles",
        NodeHandle::Null, LayerState::NeedsCommonDataUpdate|LayerState::NeedsDataUpdate,
        false, false, false, false, 1, 2, {}, {}},
    {"dynamic styles, custom alignment",
        NodeHandle::Null, LayerState::NeedsCommonDataUpdate|LayerState::NeedsDataUpdate,
        false, false, true, false, 1, 2, {}, {}},
    {"transformable",
        NodeHandle::Null, LayerState::NeedsDataUpdate,
        false, false, false, false, 3, 0, TextLayerFlag::Transformable, {}},
    {"editable",
        NodeHandle::Null, LayerState::NeedsDataUpdate,
        false, false, false, false, 3, 0, {}, ~~TextDataFlag::Editable},
    {"editable, create and attach",
        nodeHandle(9872, 0xbeb), LayerState::NeedsNodeOffsetSizeUpdate|LayerState::NeedsNodeEnabledUpdate|LayerState::NeedsAttachmentUpdate|LayerState::NeedsDataUpdate,
        false, false, false, false, 3, 0, {}, ~~TextDataFlag::Editable},
    {"editable, LayerDataHandle overloads",
        NodeHandle::Null, LayerState::NeedsDataUpdate,
        true, false, false, false, 3, 0, {}, ~~TextDataFlag::Editable},
};

const struct {
    const char* name;
    TextLayerFlags layerFlags;
    TextDataFlags flags;
} CreateRemoveHandleRecycleData[]{
    {"", {}, {}},
    {"transformable", TextLayerFlag::Transformable, {}},
    {"editable", {}, TextDataFlag::Editable},
};

const struct {
    const char* name;
    UnsignedInt styleCount, dynamicStyleCount;
} CreateStyleOutOfRangeData[]{
    {"", 3, 0},
    {"dynamic styles", 1, 2},
};

const struct {
    const char* name;
    UnsignedInt styleCount, dynamicStyleCount;
} CreateUpdateNoStyleSetData[]{
    {"", 1, 0},
    {"dynamic styles", 1, 5},
    {"dynamic styles only", 0, 5}
};

const struct {
    const char* name;
    UnsignedInt styleCount, dynamicStyleCount;
} CreateSetTextTextPropertiesData[]{
    {"", 3, 0},
    {"dynamic styles", 1, 2},
};

const struct {
    const char* name;
    TextProperties properties;
    const char* expected;
} CreateSetTextTextPropertiesEditableInvalidData[]{
    {"features",
        TextProperties{}
            .setFeatures({Text::Feature::SmallCapitals}),
        "passing font features for an editable text is not implemented yet, sorry"},
    {"top to bottom",
        TextProperties{}
            .setShapeDirection(Text::ShapeDirection::TopToBottom),
        "vertical shape direction for an editable text is not implemented yet, sorry"},
    {"bottom to top",
        TextProperties{}
            .setShapeDirection(Text::ShapeDirection::BottomToTop),
        "vertical shape direction for an editable text is not implemented yet, sorry"},
};

const struct {
    TestSuite::TestCaseDescriptionSourceLocation name;
    Text::ShapeDirection shapeDirection;
    const char* text;
    UnsignedInt cursor;
    Containers::Optional<UnsignedInt> selection;
    TextEdit edit;
    const char* insert;
    const char* expected;
    Containers::Pair<UnsignedInt, UnsignedInt> expectedCursor;
    LayerStates expectedState;
} EditData[]{
    {"move cursor left, direction unspecified",
        Text::ShapeDirection::Unspecified,
        "hello", 3, {}, TextEdit::MoveCursorLeft, "",
        "hello", {2, 2}, LayerState::NeedsDataUpdate},
    {"move cursor left, RTL",
        Text::ShapeDirection::RightToLeft,
        "hello", 3, {}, TextEdit::MoveCursorLeft, "",
        "hello", {4, 4}, LayerState::NeedsDataUpdate},
    {"move cursor left, UTF-8, LTR",
        Text::ShapeDirection::LeftToRight,
        "běhnu", 3, {}, TextEdit::MoveCursorLeft, "",
        "běhnu", {1, 1}, LayerState::NeedsDataUpdate},
    {"move cursor left, invalid UTF-8",
        Text::ShapeDirection::Unspecified,
        "b\xff\xfehnu", 3, {}, TextEdit::MoveCursorLeft, "",
        "b\xff\xfehnu", {2, 2}, LayerState::NeedsDataUpdate},
    {"move cursor left, selection active",
        Text::ShapeDirection::Unspecified,
        "hello", 3, 5, TextEdit::MoveCursorLeft, "",
        "hello", {2, 2}, LayerState::NeedsDataUpdate},
    {"extend selection left, no selection yet",
        Text::ShapeDirection::Unspecified,
        "hello", 3, 3, TextEdit::ExtendSelectionLeft, "",
        "hello", {2, 3}, LayerState::NeedsDataUpdate},
    {"extend selection left, selection active",
        Text::ShapeDirection::Unspecified,
        "hello", 3, 5, TextEdit::ExtendSelectionLeft, "",
        "hello", {2, 5}, LayerState::NeedsDataUpdate},
    {"extend selection left, selection active, different selection direction",
        Text::ShapeDirection::Unspecified,
        "hello", 5, 3, TextEdit::ExtendSelectionLeft, "",
        "hello", {4, 3}, LayerState::NeedsDataUpdate},
    {"move cursor left, at the boundary, direction unspecified",
        Text::ShapeDirection::Unspecified,
        "hello", 0, {}, TextEdit::MoveCursorLeft, "",
        "hello", {0, 0}, LayerStates{}},
    {"move cursor left, at the boundary, LTR",
        Text::ShapeDirection::LeftToRight,
        "hello", 0, {}, TextEdit::MoveCursorLeft, "",
        "hello", {0, 0}, LayerStates{}},
    {"move cursor left, at the boundary, LTR, selection active",
        Text::ShapeDirection::LeftToRight,
        "hello", 0, 5, TextEdit::MoveCursorLeft, "",
        "hello", {0, 0}, LayerState::NeedsDataUpdate},
    {"extend selection left, at the boundary, LTR, no selection yet",
        Text::ShapeDirection::LeftToRight,
        "hello", 0, 0, TextEdit::ExtendSelectionLeft, "",
        "hello", {0, 0}, LayerStates{}},
    {"extend selection left, at the boundary, LTR, selection active",
        Text::ShapeDirection::LeftToRight,
        "hello", 0, 2, TextEdit::ExtendSelectionLeft, "",
        "hello", {0, 2}, LayerStates{}},
    {"move cursor left, at the boundary, RTL",
        Text::ShapeDirection::RightToLeft,
        "hello", 5, {}, TextEdit::MoveCursorLeft, "",
        "hello", {5, 5}, LayerStates{}},
    {"move cursor left, at the boundary, RTL, selection active",
        Text::ShapeDirection::RightToLeft,
        "hello", 5, 2, TextEdit::MoveCursorLeft, "",
        "hello", {5, 5}, LayerState::NeedsDataUpdate},
    {"extend selection left, at the boundary, RTL, no selection yet",
        Text::ShapeDirection::RightToLeft,
        "hello", 5, 5, TextEdit::ExtendSelectionLeft, "",
        "hello", {5, 5}, LayerStates{}},
    {"extend selection left, at the boundary, RTL, selection active",
        Text::ShapeDirection::RightToLeft,
        "hello", 5, 3, TextEdit::ExtendSelectionLeft, "",
        "hello", {5, 3}, LayerStates{}},
    {"move cursor right, LTR",
        Text::ShapeDirection::LeftToRight,
        "hello", 3, {}, TextEdit::MoveCursorRight, "",
        "hello", {4, 4}, LayerState::NeedsDataUpdate},
    {"move cursor right, RTL",
        Text::ShapeDirection::RightToLeft,
        "hello", 3, {}, TextEdit::MoveCursorRight, "",
        "hello", {2, 2}, LayerState::NeedsDataUpdate},
    {"move cursor right, UTF-8, direction unspecified",
        Text::ShapeDirection::Unspecified,
        "sněhy", 2, {}, TextEdit::MoveCursorRight, "",
        "sněhy", {4, 4}, LayerState::NeedsDataUpdate},
    {"move cursor right, invalid UTF-8",
        Text::ShapeDirection::Unspecified,
        "sn\xfe\xffhy", 2, {}, TextEdit::MoveCursorRight, "",
        "sn\xfe\xffhy", {3, 3}, LayerState::NeedsDataUpdate},
    {"move cursor right, selection active",
        Text::ShapeDirection::Unspecified,
        "hello", 3, 5, TextEdit::MoveCursorRight, "",
        "hello", {4, 4}, LayerState::NeedsDataUpdate},
    {"extend selection right, no selection yet",
        Text::ShapeDirection::Unspecified,
        "hello", 3, 3, TextEdit::ExtendSelectionRight, "",
        "hello", {4, 3}, LayerState::NeedsDataUpdate},
    {"extend selection right, selection active",
        Text::ShapeDirection::Unspecified,
        "hello", 3, 1, TextEdit::ExtendSelectionRight, "",
        "hello", {4, 1}, LayerState::NeedsDataUpdate},
    {"extend selection right, selection active, different selection direction",
        Text::ShapeDirection::Unspecified,
        "hello", 1, 3, TextEdit::ExtendSelectionRight, "",
        "hello", {2, 3}, LayerState::NeedsDataUpdate},
    {"move cursor right, at the boundary, direction unspecified",
        Text::ShapeDirection::Unspecified,
        "hello", 5, {}, TextEdit::MoveCursorRight, "",
        "hello", {5, 5}, LayerStates{}},
    {"move cursor right, at the boundary, LTR",
        Text::ShapeDirection::LeftToRight,
        "hello", 5, {}, TextEdit::MoveCursorRight, "",
        "hello", {5, 5}, LayerStates{}},
    {"move cursor right, at the boundary, LTR, selection active",
        Text::ShapeDirection::LeftToRight,
        "hello", 5, 4, TextEdit::MoveCursorRight, "",
        "hello", {5, 5}, LayerState::NeedsDataUpdate},
    {"extend selection right, at the boundary, LTR, no selection yet",
        Text::ShapeDirection::LeftToRight,
        "hello", 5, 5, TextEdit::ExtendSelectionRight, "",
        "hello", {5, 5}, LayerStates{}},
    {"extend selection right, at the boundary, LTR, selection active",
        Text::ShapeDirection::LeftToRight,
        "hello", 5, 3, TextEdit::ExtendSelectionRight, "",
        "hello", {5, 3}, LayerStates{}},
    {"move cursor right, at the boundary, RTL",
        Text::ShapeDirection::RightToLeft,
        "hello", 0, {}, TextEdit::MoveCursorRight, "",
        "hello", {0, 0}, LayerStates{}},
    {"move cursor right, at the boundary, RTL, selection active",
        Text::ShapeDirection::RightToLeft,
        "hello", 0, 3, TextEdit::MoveCursorRight, "",
        "hello", {0, 0}, LayerState::NeedsDataUpdate},
    {"extend selection right, at the boundary, RTL, no selection yet",
        Text::ShapeDirection::RightToLeft,
        "hello", 0, 0, TextEdit::ExtendSelectionRight, "",
        "hello", {0, 0}, LayerStates{}},
    {"extend selection right, at the boundary, RTL, selection active",
        Text::ShapeDirection::RightToLeft,
        "hello", 0, 2, TextEdit::ExtendSelectionRight, "",
        "hello", {0, 2}, LayerStates{}},
    {"move cursor at line begin",
        Text::ShapeDirection::Unspecified,
        "hello", 3, {}, TextEdit::MoveCursorLineBegin, "",
        "hello", {0, 0}, LayerState::NeedsDataUpdate},
    /* It's still the first byte even with a RTL direction, it's different only
       optically */
    {"move cursor at line begin, RTL",
        Text::ShapeDirection::RightToLeft,
        "hello", 3, {}, TextEdit::MoveCursorLineBegin, "",
        "hello", {0, 0}, LayerState::NeedsDataUpdate},
    /* Invalid UTF-8 should do nothing to anything here */
    {"move cursor at line begin, invalid UTF-8",
        Text::ShapeDirection::Unspecified,
        "\xff\xfe\xfd\xfco", 3, {}, TextEdit::MoveCursorLineBegin, "",
        "\xff\xfe\xfd\xfco", {0, 0}, LayerState::NeedsDataUpdate},
    {"move cursor at line begin, selection active",
        Text::ShapeDirection::Unspecified,
        "hello", 3, 2, TextEdit::MoveCursorLineBegin, "",
        "hello", {0, 0}, LayerState::NeedsDataUpdate},
    {"extend selection to line begin, no selection yet",
        Text::ShapeDirection::Unspecified,
        "hello", 3, 3, TextEdit::ExtendSelectionLineBegin, "",
        "hello", {0, 3}, LayerState::NeedsDataUpdate},
    {"extend selection to line begin, selection active",
        Text::ShapeDirection::Unspecified,
        "hello", 3, 4, TextEdit::ExtendSelectionLineBegin, "",
        "hello", {0, 4}, LayerState::NeedsDataUpdate},
    {"extend selection to line begin, selection active, different selection direction",
        Text::ShapeDirection::Unspecified,
        "hello", 4, 3, TextEdit::ExtendSelectionLineBegin, "",
        "hello", {0, 3}, LayerState::NeedsDataUpdate},
    {"move cursor at line end",
        Text::ShapeDirection::Unspecified,
        "hello", 3, {}, TextEdit::MoveCursorLineEnd, "",
        "hello", {5, 5}, LayerState::NeedsDataUpdate},
    /* It's still (one byte after) the last byte even with a RTL direction,
       it's different only optically */
    {"move cursor at line end, RTL",
        Text::ShapeDirection::RightToLeft,
        "hello", 3, {}, TextEdit::MoveCursorLineEnd, "",
        "hello", {5, 5}, LayerState::NeedsDataUpdate},
    /* Invalid UTF-8 should do nothing to anything here */
    {"move cursor at line end, invalid UTF-8",
        Text::ShapeDirection::Unspecified,
        "h\xff\xfe\xfd\xfc", 3, {}, TextEdit::MoveCursorLineEnd, "",
        "h\xff\xfe\xfd\xfc", {5, 5}, LayerState::NeedsDataUpdate},
    {"move cursor at line end, selection active",
        Text::ShapeDirection::Unspecified,
        "hello", 3, 2, TextEdit::MoveCursorLineEnd, "",
        "hello", {5, 5}, LayerState::NeedsDataUpdate},
    {"extend selection to line end, no selection yet",
        Text::ShapeDirection::Unspecified,
        "hello", 3, 3, TextEdit::ExtendSelectionLineEnd, "",
        "hello", {5, 3}, LayerState::NeedsDataUpdate},
    {"extend selection to line end, selection active",
        Text::ShapeDirection::Unspecified,
        "hello", 3, 2, TextEdit::ExtendSelectionLineEnd, "",
        "hello", {5, 2}, LayerState::NeedsDataUpdate},
    {"extend selection to line end, selection active, different selection direction",
        Text::ShapeDirection::Unspecified,
        "hello", 2, 3, TextEdit::ExtendSelectionLineEnd, "",
        "hello", {5, 3}, LayerState::NeedsDataUpdate},
    {"remove character before cursor",
        Text::ShapeDirection::Unspecified,
        "hello", 3, {}, TextEdit::RemoveBeforeCursor, "",
        "helo", {2, 2}, LayerState::NeedsDataUpdate},
    /* There's no difference in behavior for RTL here */
    {"remove character before cursor, RTL",
        Text::ShapeDirection::RightToLeft,
        "hello", 3, {}, TextEdit::RemoveBeforeCursor, "",
        "helo", {2, 2}, LayerState::NeedsDataUpdate},
    {"remove character before cursor, UTF-8",
        Text::ShapeDirection::Unspecified,
        "běhnu", 3, {}, TextEdit::RemoveBeforeCursor, "",
        "bhnu", {1, 1}, LayerState::NeedsDataUpdate},
    {"remove character before cursor, invalid UTF-8",
        Text::ShapeDirection::Unspecified,
        "b\xfe\xffhnu", 3, {}, TextEdit::RemoveBeforeCursor, "",
        "b\xfehnu", {2, 2}, LayerState::NeedsDataUpdate},
    {"remove character before cursor, at the boundary",
        Text::ShapeDirection::Unspecified,
        "hello", 0, {}, TextEdit::RemoveBeforeCursor, "",
        "hello", {0, 0}, LayerStates{}},
    {"remove selection before cursor",
        Text::ShapeDirection::Unspecified,
        "hello", 3, 1, TextEdit::RemoveBeforeCursor, "",
        "hlo", {1, 1}, LayerState::NeedsDataUpdate},
    /* Should have the same behavior no matter what the direction */
    {"remove selection before cursor, different selection direction",
        Text::ShapeDirection::Unspecified,
        "hello", 1, 3, TextEdit::RemoveBeforeCursor, "",
        "hlo", {1, 1}, LayerState::NeedsDataUpdate},
    /* Cursor being 0 shouldn't stop this from removing */
    {"remove selection before cursor, at the boundary",
        Text::ShapeDirection::Unspecified,
        "hello", 0, 3, TextEdit::RemoveBeforeCursor, "",
        "lo", {0, 0}, LayerState::NeedsDataUpdate},
    {"remove character after cursor",
        Text::ShapeDirection::Unspecified,
        "hello", 3, {}, TextEdit::RemoveAfterCursor, "",
        "helo", {3, 3}, LayerState::NeedsDataUpdate},
    /* There's no difference in behavior for RTL here */
    {"remove character after cursor, RTL",
        Text::ShapeDirection::RightToLeft,
        "hello", 3, {}, TextEdit::RemoveAfterCursor, "",
        "helo", {3, 3}, LayerState::NeedsDataUpdate},
    {"remove character after cursor, UTF-8",
        Text::ShapeDirection::Unspecified,
        "sněhy", 2, {}, TextEdit::RemoveAfterCursor, "",
        "snhy", {2, 2}, LayerState::NeedsDataUpdate},
    {"remove character after cursor, invalid UTF-8",
        Text::ShapeDirection::Unspecified,
        "sn\xff\xfehy", 2, {}, TextEdit::RemoveAfterCursor, "",
        "sn\xfehy", {2, 2}, LayerState::NeedsDataUpdate},
    {"remove character after cursor, at the boundary",
        Text::ShapeDirection::Unspecified,
        "hello", 5, {}, TextEdit::RemoveAfterCursor, "",
        "hello", {5, 5}, LayerStates{}},
    {"remove selection after cursor",
        Text::ShapeDirection::Unspecified,
        "hello", 1, 3, TextEdit::RemoveAfterCursor, "",
        "hlo", {1, 1}, LayerState::NeedsDataUpdate},
    /* Should have the same behavior no matter what the direction */
    {"remove selection after cursor, different selection direction",
        Text::ShapeDirection::Unspecified,
        "hello", 3, 1, TextEdit::RemoveAfterCursor, "",
        "hlo", {1, 1}, LayerState::NeedsDataUpdate},
    /* Cursor being at the end shouldn't stop this from removing */
    {"remove selection after cursor, at the boundary",
        Text::ShapeDirection::Unspecified,
        "hello", 5, 3, TextEdit::RemoveAfterCursor, "",
        "hel", {3, 3}, LayerState::NeedsDataUpdate},
    {"insert before cursor",
        Text::ShapeDirection::Unspecified,
        "sume", 2, {}, TextEdit::InsertBeforeCursor, "mmerti",
        "summertime", {8, 8}, LayerState::NeedsDataUpdate},
    /* There's no difference in behavior for RTL here */
    {"insert before cursor, RTL",
        Text::ShapeDirection::RightToLeft,
        "sume", 2, {}, TextEdit::InsertBeforeCursor, "mmerti",
        "summertime", {8, 8}, LayerState::NeedsDataUpdate},
    {"insert before cursor, selection active",
        Text::ShapeDirection::Unspecified,
        "summertime", 4, 9, TextEdit::InsertBeforeCursor, "ariz",
        "summarize", {8, 8}, LayerState::NeedsDataUpdate},
    {"insert before cursor, selection active, different selection direction",
        Text::ShapeDirection::Unspecified,
        "summertime", 9, 4, TextEdit::InsertBeforeCursor, "ariz",
        "summarize", {8, 8}, LayerState::NeedsDataUpdate},
    {"insert after cursor",
        Text::ShapeDirection::Unspecified,
        "sume", 2, {}, TextEdit::InsertAfterCursor, "mmerti",
        "summertime", {2, 2}, LayerState::NeedsDataUpdate},
    /* There's no difference in behavior for RTL here */
    {"insert after cursor, RTL",
        Text::ShapeDirection::RightToLeft,
        "sume", 2, {}, TextEdit::InsertAfterCursor, "mmerti",
        "summertime", {2, 2}, LayerState::NeedsDataUpdate},
    {"insert after cursor, selection active",
        Text::ShapeDirection::Unspecified,
        "summertime", 4, 9, TextEdit::InsertAfterCursor, "ariz",
        "summarize", {4, 4}, LayerState::NeedsDataUpdate},
    {"insert after cursor, selection active, different selection direction",
        Text::ShapeDirection::Unspecified,
        "summertime", 9, 4, TextEdit::InsertAfterCursor, "ariz",
        "summarize", {4, 4}, LayerState::NeedsDataUpdate},
    /* Invalid UTF-8 should do nothing to anything here */
    {"insert before cursor, invalid original UTF-8",
        Text::ShapeDirection::Unspecified,
        "s\xff\xff" "e", 2, {}, TextEdit::InsertBeforeCursor, "ěží",
        "s\xffěží\xff" "e", {8, 8}, LayerState::NeedsDataUpdate},
    {"insert after cursor, invalid original UTF-8",
        Text::ShapeDirection::Unspecified,
        "s\xff\xff" "e", 2, {}, TextEdit::InsertAfterCursor, "ěží",
        "s\xffěží\xff" "e", {2, 2}, LayerState::NeedsDataUpdate},
    {"insert before cursor, invalid inserted UTF-8",
        Text::ShapeDirection::Unspecified,
        "snme", 2, {}, TextEdit::InsertBeforeCursor, "\xff\xfež\xfd\xfc",
        "sn\xff\xfež\xfd\xfcme", {8, 8}, LayerState::NeedsDataUpdate},
    {"insert after cursor, invalid inserted UTF-8",
        Text::ShapeDirection::Unspecified,
        "snme", 2, {}, TextEdit::InsertAfterCursor, "\xff\xfež\xfd\xfc",
        "sn\xff\xfež\xfd\xfcme", {2, 2}, LayerState::NeedsDataUpdate},
};

const struct {
    TestSuite::TestCaseDescriptionSourceLocation name;
    bool emptyUpdate;
    UnsignedInt styleCount, editingStyleCount, dynamicStyleCount;
    bool hasEditingStyles;
    TextLayerSharedFlags sharedLayerFlags;
    TextLayerFlags layerFlags;
    Vector2 node6Offset, node6Size;
    Vector4 paddingFromStyle;
    Vector4 paddingOrTranslationFromData;
    TextDataFlags dataFlags;
    Containers::Pair<UnsignedInt, UnsignedInt> data3Cursor, data9Cursor;
    Containers::Pair<Int, Int> editingStyle1, editingStyle2, editingStyle3;
    LayerStates states;
    bool expectIndexDataUpdated, expectVertexDataUpdated, expectEditingDataPresent;
} UpdateCleanDataOrderData[]{
    {"empty update", true, 6, 0, 0, false, {}, {},
        {}, {}, {}, {}, {},
        {}, {}, {-1, -1}, {-1, -1}, {-1, -1},
        LayerState::NeedsDataUpdate, true, true, false},
    {"", false, 6, 0, 0, false, {}, {},
        {1.0f, 2.0f}, {10.0f, 15.0f}, {}, {}, {},
        {}, {}, {-1, -1}, {-1, -1}, {-1, -1},
        LayerState::NeedsDataUpdate, true, true, false},
    {"distance field", false, 6, 0, 0, false,
        TextLayerSharedFlag::DistanceField, {},
        {1.0f, 2.0f}, {10.0f, 15.0f}, {}, {}, {},
        {}, {}, {-1, -1}, {-1, -1}, {-1, -1},
        LayerState::NeedsDataUpdate, true, true, false},
    {"node offset/size update only", false, 6, 0, 0, false, {}, {},
        {1.0f, 2.0f}, {10.0f, 15.0f}, {}, {}, {},
        {}, {}, {-1, -1}, {-1, -1}, {-1, -1},
        LayerState::NeedsNodeOffsetSizeUpdate, false, true, false},
    {"node order update only", false, 6, 0, 0, false, {}, {},
        {1.0f, 2.0f}, {10.0f, 15.0f}, {}, {}, {},
        {}, {}, {-1, -1}, {-1, -1}, {-1, -1},
        LayerState::NeedsNodeOrderUpdate, true, false, false},
    {"node enabled update only", false, 6, 0, 0, false, {}, {},
        {1.0f, 2.0f}, {10.0f, 15.0f}, {}, {}, {},
        {}, {}, {-1, -1}, {-1, -1}, {-1, -1},
        LayerState::NeedsNodeEnabledUpdate, false, true, false},
    /* Cannot use NeedsNodeOpacityUpdate alone because then AbstractVisualLayer
       doUpdate() doesn't fill in calculated styles, leading to OOB errors. */
    /** @todo Which ultimately means this doesn't correctly test that the
        implementation correctly handles the NeedsNodeOpacityUpdate flag alone
        -- what can I do differently to test that? */
    {"node enabled + opacity update only", false, 6, 0, 0, false, {}, {},
        {1.0f, 2.0f}, {10.0f, 15.0f}, {}, {}, {},
        {}, {}, {-1, -1}, {-1, -1}, {-1, -1},
        LayerState::NeedsNodeEnabledUpdate|LayerState::NeedsNodeOpacityUpdate, false, true, false},
    /* These two shouldn't cause anything to be done in update(), and also no
       crashes */
    {"shared data update only", false, 6, 0, 0, false, {}, {},
        {1.0f, 2.0f}, {10.0f, 15.0f}, {}, {}, {},
        {}, {}, {-1, -1}, {-1, -1}, {-1, -1},
        LayerState::NeedsSharedDataUpdate, false, false, false},
    {"common data update only", false, 6, 0, 0, false, {}, {},
        {1.0f, 2.0f}, {10.0f, 15.0f}, {}, {}, {},
        {}, {}, {-1, -1}, {-1, -1}, {-1, -1},
        LayerState::NeedsCommonDataUpdate, false, false, false},
    /* This would cause an update of the dynamic style data in derived classes
       if appropriate internal flags would be set internally, but in the base
       class it's nothing */
    {"common data update only, dynamic styles", false, 4, 0, 2, false, {}, {},
        {1.0f, 2.0f}, {10.0f, 15.0f}, {}, {}, {},
        {}, {}, {-1, -1}, {-1, -1}, {-1, -1},
        LayerState::NeedsCommonDataUpdate, false, false, false},
    {"padding from style", false, 6, 0, 0, false, {}, {},
        {-1.0f, 1.5f}, {13.0f, 17.0f},
        {2.0f, 0.5f, 1.0f, 1.5f}, {}, {},
        {}, {}, {-1, -1}, {-1, -1}, {-1, -1},
        LayerState::NeedsDataUpdate, true, true, false},
    {"padding from data", false, 6, 0, 0, false, {}, {},
        {-1.0f, 1.5f}, {13.0f, 17.0f},
        {}, {2.0f, 0.5f, 1.0f, 1.5f}, {},
        {}, {}, {-1, -1}, {-1, -1}, {-1, -1},
        LayerState::NeedsDataUpdate, true, true, false},
    {"padding from both style and data", false, 6, 0, 0, false, {}, {},
        {-1.0f, 1.5f}, {13.0f, 17.0f},
        {0.5f, 0.0f, 1.0f, 0.75f}, {1.5f, 0.5f, 0.0f, 0.75f}, {},
        {}, {}, {-1, -1}, {-1, -1}, {-1, -1},
        LayerState::NeedsDataUpdate, true, true, false},
    {"unused dynamic styles", false, 6, 0, 17, false, {}, {},
        {1.0f, 2.0f}, {10.0f, 15.0f}, {}, {}, {},
        {}, {}, {-1, -1}, {-1, -1}, {-1, -1},
        LayerState::NeedsDataUpdate, true, true, false},
    {"dynamic styles", false, 4, 0, 2, false, {}, {},
        {1.0f, 2.0f}, {10.0f, 15.0f}, {}, {}, {},
        {}, {}, {-1, -1}, {-1, -1}, {-1, -1},
        LayerState::NeedsDataUpdate, true, true, false},
    {"dynamic styles, padding from dynamic style", false, 4, 0, 2, false, {}, {},
        {-1.0f, 1.5f}, {13.0f, 17.0f},
        {2.0f, 0.5f, 1.0f, 1.5f}, {}, {},
        {}, {}, {-1, -1}, {-1, -1}, {-1, -1},
        LayerState::NeedsDataUpdate, true, true, false},
    {"dynamic styles, padding from both dynamic style and data", false, 4, 0, 2, false, {}, {},
        {-1.0f, 1.5f}, {13.0f, 17.0f},
        {0.5f, 0.0f, 1.0f, 0.75f}, {1.5f, 0.5f, 0.0f, 0.75f}, {},
        {}, {}, {-1, -1}, {-1, -1}, {-1, -1},
        LayerState::NeedsDataUpdate, true, true, false},
    {"transformable", false, 6, 0, 0, false, {}, TextLayerFlag::Transformable,
        {1.0f, 2.0f}, {10.0f, 15.0f}, {}, {}, {},
        {}, {}, {-1, -1}, {-1, -1}, {-1, -1},
        LayerState::NeedsDataUpdate, true, true, false},
    {"transformable, translation", false, 6, 0, 0, false, {}, TextLayerFlag::Transformable,
        {-1.0f, 1.5f}, {10.0f, 15.0f}, {},
        /* First two components interpreted as translation, which is then
           subtracted from node offset */
        {2.0f, 0.5f, 0.0f, 0.0f}, {},
        {}, {}, {-1, -1}, {-1, -1}, {-1, -1},
        LayerState::NeedsDataUpdate, true, true, false},
    {"transformable, translation + padding from style", false, 6, 0, 0, false, {}, TextLayerFlag::Transformable,
        {-1.0f, 1.5f}, {11.5f, 16.75f},
        {0.5f, 0.25f, 1.0f, 1.5f},
        /* First two components interpreted as translation, which is then
           together with style padding subtracted from node offset */
        {1.5f, 0.25f, 0.0f, 0.0f}, {},
        {}, {}, {-1, -1}, {-1, -1}, {-1, -1},
        LayerState::NeedsDataUpdate, true, true, false},
    {"editable, no editing styles", false, 6, 0, 0, false, {}, {},
        {1.0f, 2.0f}, {10.0f, 15.0f}, {}, {}, TextDataFlag::Editable,
        {}, {},
        {-1, -1}, {-1, -1}, {-1, -1},
        LayerState::NeedsDataUpdate, true, true, false},
    /* The editing styles are there but nothing references them, which means
       the corresponding draw data aren't filled either */
    {"editable, editing styles but not used", false, 6, 3, 0, false, {}, {},
        {1.0f, 2.0f}, {10.0f, 15.0f}, {}, {}, TextDataFlag::Editable,
        {}, {},
        {-1, -1}, {-1, -1}, {-1, -1},
        LayerState::NeedsDataUpdate, true, true, false},
    {"editable", false, 6, 3, 0, false, {}, {},
        {1.0f, 2.0f}, {10.0f, 15.0f}, {}, {}, TextDataFlag::Editable,
        {2, 5}, {1, 1},
        {-1, 1}, {1, 0}, {2, 0},
        LayerState::NeedsDataUpdate, true, true, true},
    {"editable, different selection direction", false, 6, 3, 0, false, {}, {},
        {1.0f, 2.0f}, {10.0f, 15.0f}, {}, {}, TextDataFlag::Editable,
        {5, 2}, {1, 1},
        {-1, 1}, {1, 0}, {2, 0},
        LayerState::NeedsDataUpdate, true, true, true},
    {"editable, non-empty selection but no selection style", false, 6, 3, 0, false, {}, {},
        {1.0f, 2.0f}, {10.0f, 15.0f}, {}, {}, TextDataFlag::Editable,
        {2, 5}, {1, 2},
        {-1, 1}, {1, 0}, {2, -1},
        LayerState::NeedsDataUpdate, true, true, true},
    /* Shouldn't cause anything to be done in update(), and also no crashes */
    {"editable, shared data update only", false, 6, 3, 0, false, {}, {},
        {1.0f, 2.0f}, {10.0f, 15.0f}, {}, {}, TextDataFlag::Editable,
        {2, 5}, {1, 1},
        {-1, 1}, {1, 0}, {2, 0},
        /* expectEditingDataPresent is set to true because subsequent updates
           are done with full NeedsDataUpdate and they'd check for the data
           being empty instead */
        LayerState::NeedsSharedDataUpdate, false, false, true},
    {"editable, dynamic, no editing styles", false, 4, 0, 2, false, {}, {},
        {1.0f, 2.0f}, {10.0f, 15.0f}, {}, {}, TextDataFlag::Editable,
        {}, {},
        {-1, -1}, {-1, -1}, {-1, -1},
        LayerState::NeedsDataUpdate, true, true, false},
    {"editable, dynamic, editing styles but not used", false, 4, 0, 2, true, {}, {},
        {1.0f, 2.0f}, {10.0f, 15.0f}, {}, {}, TextDataFlag::Editable,
        {}, {},
        {-1, -1}, {-1, -1}, {-1, -1},
        LayerState::NeedsDataUpdate, true, true, false},
    {"editable, dynamic", false, 4, 2, 2, false, {}, {},
        {1.0f, 2.0f}, {10.0f, 15.0f}, {}, {}, TextDataFlag::Editable,
        {2, 5}, {1, 1},
        /* editingStyle2 is used as dynamic */
        {-1, 0}, {-1, -1}, {1, -1},
        LayerState::NeedsDataUpdate, true, true, true},
    /* Shouldn't cause anything to be done in update(), and also no crashes */
    {"editable, dynamic, shared data update only", false, 4, 2, 2, false, {}, {},
        {1.0f, 2.0f}, {10.0f, 15.0f}, {}, {}, TextDataFlag::Editable,
        {2, 5}, {1, 1},
        /* editingStyle2 is used as dynamic */
        {-1, 0}, {-1, -1}, {1, -1},
        /* expectEditingDataPresent is set to true because subsequent updates
           are done with full NeedsDataUpdate and they'd check for the data
           being empty instead */
        LayerState::NeedsSharedDataUpdate, false, false, true},
};

const struct {
    const char* name;
    Text::Alignment alignment;
    Text::ShapeDirection shapeDirection;
    /* Node offset is {50.5, 20.5}, size {200.8, 100.4}; bounding box {9, 11},
       ascent 7, descent -4 */
    Vector2 offset;
    /* Glyph ounding box is {6, 8}, offset {-4, -6} */
    Vector2 offsetGlyph;
    Float editingPaddingL, editingPaddingR;
} UpdateAlignmentPaddingData[]{
    {"line left",
        Text::Alignment::LineLeft, Text::ShapeDirection::Unspecified,
        /* 20.5 + 100.4/2 = 70.7 */
        {50.5f, 70.7f},
        {50.5f, 76.7f},
        0.1f, 0.3f},
    {"line right",
        Text::Alignment::LineRight, Text::ShapeDirection::Unspecified,
        {50.5f + 200.8f - 9.0f, 70.7f},
        {50.5f + 200.8f - 6.0f, 76.7f},
        0.1f, 0.3f},
    {"top center",
        Text::Alignment::TopCenter, Text::ShapeDirection::Unspecified,
        {50.5f + 100.4f - 4.5f, 20.5f + 7.0f},
        {50.5f + 100.4f - 3.0f, 20.5f + 8.0f},
        0.1f, 0.3f},
    {"top center, interal",
        Text::Alignment::TopCenterIntegral, Text::ShapeDirection::Unspecified,
        /* Only the offset inside the node and the bounding box is rounded,
           not the node offset itself; not the Y coordinate either */
        {50.5f + 100.0f - 5.0f, 20.5f + 7.0f},
        /* No change for the glyph as the glyph cache has integer sizes */
        {50.5f + 100.0f - 3.0f, 20.5f + 8.0f},
        0.1f, 0.3f},
    {"bottom left",
        Text::Alignment::BottomLeft, Text::ShapeDirection::Unspecified,
        {50.5f, 120.9f - 4.0f},
        {50.5f, 120.9f},
        0.1f, 0.3f},
    {"middle right",
        Text::Alignment::MiddleRight, Text::ShapeDirection::Unspecified,
        {50.5f + 200.8f - 9.0f, 20.5f + 50.2f - 5.5f + 7.0f},
        {50.5f + 200.8f - 6.0f, 20.5f + 50.2f - 4.0f + 8.0f},
        0.1f, 0.3f},
    {"middle right, integral",
        Text::Alignment::MiddleRightIntegral, Text::ShapeDirection::Unspecified,
        /* Only the offset inside the node and the bounding box is rounded,
           not the node offset itself; not the X coordinate either. Note that
           the Y rounding is in the other direction compared to X because of Y
           flip. */
        {50.5f + 200.8f - 9.0f, 20.5f + 50.0f - 5.0f + 7.0f},
        {50.5f + 200.8f - 6.0f, 20.5f + 50.0f - 4.0f + 8.0f},
        0.1f, 0.3f},
    {"middle center",
        Text::Alignment::MiddleCenter, Text::ShapeDirection::Unspecified,
        {50.5f + 100.4f - 4.5f, 20.5f + 50.2f - 5.5f + 7.0f},
        {50.5f + 100.4f - 3.0f, 20.5f + 50.2f - 4.0f + 8.0f},
        0.1f, 0.3f},
    {"middle center, integral",
        Text::Alignment::MiddleCenterIntegral, Text::ShapeDirection::Unspecified,
        /* Only the offset inside the node and the bounding box is rounded,
           not the node offset itself. Note that the Y rounding is in the other
           direction compared to X because of Y flip. */
        {50.5f + 100.0f - 5.0f, 20.5f + 50.0f - 5.0f + 7.0f},
        {50.5f + 100.0f - 3.0f, 20.5f + 50.0f - 4.0f + 8.0f},
        0.1f, 0.3f},
    {"line end, RTL",
        Text::Alignment::LineEnd, Text::ShapeDirection::RightToLeft,
        /* Same as line left */
        {50.5f, 70.7f},
        {50.5f, 76.7f},
        0.3f, 0.1f}, /* Swapped compared to LTR */
    {"bottom begin, unspecified direction",
        Text::Alignment::BottomBegin, Text::ShapeDirection::Unspecified,
        /* Same as bottom start */
        {50.5f, 120.9f - 4.0f},
        {50.5f, 120.9f},
        0.1f, 0.3f},
    {"middle begin, RTL",
        Text::Alignment::MiddleBegin, Text::ShapeDirection::RightToLeft,
        /* Same as middle right */
        {50.5f + 200.8f - 9.0f, 20.5f + 50.2f - 5.5f + 7.0f},
        {50.5f + 200.8f - 6.0f, 20.5f + 50.2f - 4.0f + 8.0f},
        0.3f, 0.1f}, /* Swapped compared to LTR */
    {"middle center, RTL",
        Text::Alignment::MiddleCenter, Text::ShapeDirection::RightToLeft,
        /* No change compared to middle center above */
        {50.5f + 100.4f - 4.5f, 20.5f + 50.2f - 5.5f + 7.0f},
        {50.5f + 100.4f - 3.0f, 20.5f + 50.2f - 4.0f + 8.0f},
        0.3f, 0.1f}, /* Swapped compared to LTR */
};

const struct {
    const char* name;
    TextLayerSharedFlags sharedLayerFlags;
    TextLayerFlags layerFlags;
    Vector2 translation;
    Rad rotation;
    Float scaling;
    Matrix3 expected;
} UpdateTransformationData[]{
    {"",
        {}, {},
        {}, {}, 1.0f, Matrix3{Math::IdentityInit}},
    {"distance field",
        TextLayerSharedFlag::DistanceField, {},
        {}, {}, 1.0f, Matrix3{Math::IdentityInit}},
    {"transformable",
        {}, TextLayerFlag::Transformable,
        {}, {}, 1.0f, Matrix3{Math::IdentityInit}},
    {"transformable, translation",
        {}, TextLayerFlag::Transformable,
        {2.5f, -15.0f}, {}, 1.0f, Matrix3::translation({2.5f, -15.0f})},
    /* The first is just to make it easier to see what's wrong if everything is
       broken for some reason. */
    {"transformable, rotation 90°",
        {}, TextLayerFlag::Transformable,
        {}, 90.0_degf, 1.0f, Matrix3::rotation(90.0_degf)},
    /* Positive rotation should be clockwise, as explicitly verified in the
       test case */
    {"transformable, rotation 35°",
        {}, TextLayerFlag::Transformable,
        {}, 35.0_degf, 1.0f, Matrix3::rotation(35.0_degf)},
    /* Negative rotation should result in rotation counterclockwise instead */
    {"transformable, rotation -35°",
        {}, TextLayerFlag::Transformable,
        {}, -35.0_degf, 1.0f, Matrix3::rotation(-35.0_degf)},
    {"transformable, scaling",
        {}, TextLayerFlag::Transformable,
        {}, {}, 2.5f, Matrix3::scaling(Vector2{2.5f})},
    {"transformable, translation + rotation + scaling",
        {}, TextLayerFlag::Transformable,
        {2.5f, -15.0f}, 35.0_degf, 2.5f,
        Matrix3::translation({2.5f, -15.0f})*
        Matrix3::rotation(35.0_degf)*
        Matrix3::scaling(Vector2{2.5f})},
    {"transformable + distance field",
        TextLayerSharedFlag::DistanceField, TextLayerFlag::Transformable,
        {}, {}, 1.0f, Matrix3{Math::IdentityInit}},
    {"transformable + distance field, translation + rotation + scaling",
        TextLayerSharedFlag::DistanceField, TextLayerFlag::Transformable,
        {2.5f, -15.0f}, 35.0_degf, 2.5f,
        Matrix3::translation({2.5f, -15.0f})*
        Matrix3::rotation(35.0_degf)*
        Matrix3::scaling(Vector2{2.5f})},
};

const struct {
    const char* name;
    UnsignedInt editingStyleCount, dynamicStyleCount;
    bool setStyle, setEditingStyle;
    LayerStates extraState;
} SharedNeedsUpdateStatePropagatedToLayersData[]{
    {"",
        0, 0, true, false, {}},
    {"dynamic styles",
        0, 5, true, false, LayerState::NeedsCommonDataUpdate},
    {"editing styles, set base style only",
        1, 0, true, false, {}},
    {"editing styles + dynamic styles, set base style only",
        1, 5, true, false, LayerState::NeedsCommonDataUpdate},
    {"editing styles, set editing style only",
        1, 0, false, true, {}},
    {"editing styles + dynamic styles, set editing style only",
        1, 5, false, true, LayerState::NeedsCommonDataUpdate},
    {"editing styles, set both",
        1, 0, true, true, {}},
    {"editing styles + dynamic styles, set both",
        1, 5, true, true, LayerState::NeedsCommonDataUpdate},
};

const struct {
    const char* name;
    bool update;
} KeyTextEventSynthesizedFromPointerPressData[]{
    {"", false},
    {"with explicit update", true},
};

TextLayerTest::TextLayerTest() {
    addTests({&TextLayerTest::styleUniformSizeAlignment<TextLayerCommonStyleUniform>,
              &TextLayerTest::styleUniformSizeAlignment<TextLayerStyleUniform>,
              &TextLayerTest::styleUniformSizeAlignment<TextLayerCommonEditingStyleUniform>,
              &TextLayerTest::styleUniformSizeAlignment<TextLayerEditingStyleUniform>,

              &TextLayerTest::styleUniformCommonConstructDefault,
              &TextLayerTest::styleUniformCommonConstruct,
              &TextLayerTest::styleUniformCommonConstructNoInit,
              &TextLayerTest::styleUniformCommonSetters,

              &TextLayerTest::styleUniformConstructDefault,
              &TextLayerTest::styleUniformConstruct,
              &TextLayerTest::styleUniformConstructDistanceField,
              &TextLayerTest::styleUniformConstructNoInit,
              &TextLayerTest::styleUniformSetters,

              &TextLayerTest::editingStyleUniformCommonConstructDefault,
              &TextLayerTest::editingStyleUniformCommonConstruct,
              &TextLayerTest::editingStyleUniformCommonConstructNoInit,
              &TextLayerTest::editingStyleUniformCommonSetters,

              &TextLayerTest::editingStyleUniformConstructDefault,
              &TextLayerTest::editingStyleUniformConstruct,
              &TextLayerTest::editingStyleUniformConstructNoInit,
              &TextLayerTest::editingStyleUniformSetters,

              &TextLayerTest::fontHandle,
              &TextLayerTest::fontHandleInvalid,
              &TextLayerTest::debugFontHandle,
              &TextLayerTest::debugFontHandlePacked,

              &TextLayerTest::debugLayerFlag,
              &TextLayerTest::debugLayerFlags,
              &TextLayerTest::debugDataFlag,
              &TextLayerTest::debugDataFlagPacked,
              &TextLayerTest::debugDataFlags,
              &TextLayerTest::debugDataFlagsPacked,
              &TextLayerTest::debugEdit,

              &TextLayerTest::sharedDebugFlag,
              &TextLayerTest::sharedDebugFlags,

              &TextLayerTest::sharedConfigurationConstruct,
              &TextLayerTest::sharedConfigurationConstructSameStyleUniformCount,
              &TextLayerTest::sharedConfigurationConstructZeroStyleOrUniformCount,
              &TextLayerTest::sharedConfigurationConstructCopy,
              &TextLayerTest::sharedConfigurationSetters,
              &TextLayerTest::sharedConfigurationSettersSameEditingStyleUniformCount,
              &TextLayerTest::sharedConfigurationSettersInvalidEditingStyleOrUniformCount,

              &TextLayerTest::sharedConstruct,
              &TextLayerTest::sharedConstructNoCreate,
              &TextLayerTest::sharedConstructCopy,
              &TextLayerTest::sharedConstructMove,
              &TextLayerTest::sharedConstructZeroStyleCount,

              &TextLayerTest::sharedAddFont,
              &TextLayerTest::sharedAddFontTakeOwnership,
              &TextLayerTest::sharedAddFontTakeOwnershipNull,
              &TextLayerTest::sharedAddFontNotFoundInCache,
              &TextLayerTest::sharedAddFontNoHandlesLeft,
              &TextLayerTest::sharedAddInstancelessFontHasInstance,
              &TextLayerTest::sharedFontInvalidHandle,
              &TextLayerTest::sharedFontNoInstance});

    addInstancedTests({&TextLayerTest::sharedSetStyle,
                       &TextLayerTest::sharedSetStyleImplicitFeatures,
                       &TextLayerTest::sharedSetStyleImplicitEditingStyles,
                       &TextLayerTest::sharedSetStyleImplicitPadding,
                       &TextLayerTest::sharedSetStyleInvalidSize},
        Containers::arraySize(SharedSetStyleData));

    addTests({&TextLayerTest::sharedSetStyleInvalidMapping});

    addInstancedTests({&TextLayerTest::sharedSetStyleImplicitMapping,
                       &TextLayerTest::sharedSetStyleImplicitMappingImplicitFeatures,
                       &TextLayerTest::sharedSetStyleImplicitMappingImplicitEditingStyles,
                       &TextLayerTest::sharedSetStyleImplicitMappingImplicitPadding,
                       &TextLayerTest::sharedSetStyleImplicitMappingInvalidSize},
        Containers::arraySize(SharedSetStyleData));

    addTests({&TextLayerTest::sharedSetStyleInvalidFontHandle,
              &TextLayerTest::sharedSetStyleInvalidAlignment,
              &TextLayerTest::sharedSetStyleInvalidFeatures,
              &TextLayerTest::sharedSetStyleInvalidEditingStyles});

    addInstancedTests({&TextLayerTest::sharedSetEditingStyle,
                       &TextLayerTest::sharedSetEditingStyleImplicitTextUniforms,
                       &TextLayerTest::sharedSetEditingStyleInvalidSize,
                       &TextLayerTest::sharedSetEditingStyleInvalidMapping,
                       &TextLayerTest::sharedSetEditingStyleImplicitMapping,
                       &TextLayerTest::sharedSetEditingStyleImplicitMappingImplicitTextUniforms,
                       &TextLayerTest::sharedSetEditingStyleImplicitMappingInvalidSize},
        Containers::arraySize(SharedSetStyleData));

    addInstancedTests({&TextLayerTest::construct},
        Containers::arraySize(ConstructData));

    addTests({&TextLayerTest::constructCopy,
              &TextLayerTest::constructMove});

    addInstancedTests({&TextLayerTest::dynamicStyle},
        Containers::arraySize(DynamicStyleData));

    addTests({&TextLayerTest::dynamicStyleFeatureAllocation});

    addInstancedTests({&TextLayerTest::dynamicStyleEditingStyles},
        Containers::arraySize(DynamicStyleEditingStylesData));

    addTests({&TextLayerTest::dynamicStyleNoDynamicStyles,
              &TextLayerTest::dynamicStyleInvalid});

    addInstancedTests<TextLayerTest>({
        &TextLayerTest::createRemoveSet<UnsignedInt, UnsignedInt>,
        &TextLayerTest::createRemoveSet<UnsignedInt, Enum>,
        &TextLayerTest::createRemoveSet<Enum, UnsignedInt>,
        &TextLayerTest::createRemoveSet<Enum, Enum>},
        Containers::arraySize(CreateRemoveSetData));

    addInstancedTests({&TextLayerTest::createRemoveHandleRecycle},
        Containers::arraySize(CreateRemoveHandleRecycleData));

    addInstancedTests({&TextLayerTest::createStyleOutOfRange},
        Containers::arraySize(CreateStyleOutOfRangeData));

    addInstancedTests({&TextLayerTest::createNoStyleSet},
        Containers::arraySize(CreateUpdateNoStyleSetData));

    addTests({&TextLayerTest::setCursor,
              &TextLayerTest::setCursorInvalid,
              &TextLayerTest::updateText,
              &TextLayerTest::updateTextInvalid});

    addInstancedTests({&TextLayerTest::editText},
        Containers::arraySize(EditData));

    addTests({&TextLayerTest::editTextInvalid,

              &TextLayerTest::cycleGlyphEditableNonEditableText});

    addInstancedTests({&TextLayerTest::createSetTextTextProperties},
        Containers::arraySize(CreateSetTextTextPropertiesData));

    addInstancedTests({&TextLayerTest::createSetTextTextPropertiesEditable},
        Containers::arraySize(CreateSetTextTextPropertiesData));

    addInstancedTests({&TextLayerTest::createSetTextTextPropertiesEditableInvalid},
        Containers::arraySize(CreateSetTextTextPropertiesEditableInvalidData));

    addRepeatedTests({&TextLayerTest::createSetUpdateTextFromLayerItself}, 10);

    addTests({&TextLayerTest::setColor,
              &TextLayerTest::setPadding,
              &TextLayerTest::setPaddingInvalid,
              &TextLayerTest::setTransformation,
              &TextLayerTest::setTransformationInvalid,

              &TextLayerTest::invalidHandle,
              &TextLayerTest::invalidHandleTransformation,
              &TextLayerTest::invalidFontHandle,
              &TextLayerTest::nonEditableText,
              &TextLayerTest::nonEditableTextTransformation,
              &TextLayerTest::noSharedStyleFonts,
              &TextLayerTest::noFontInstance,
              &TextLayerTest::glyphOutOfRange,

              &TextLayerTest::updateEmpty});

    addInstancedTests({&TextLayerTest::updateCleanDataOrder},
        Containers::arraySize(UpdateCleanDataOrderData));

    addInstancedTests({&TextLayerTest::updateAlignment,
                       &TextLayerTest::updateAlignmentGlyph,
                       &TextLayerTest::updatePadding,
                       &TextLayerTest::updatePaddingGlyph},
        Containers::arraySize(UpdateAlignmentPaddingData));

    addInstancedTests({&TextLayerTest::updateTransformation},
        Containers::arraySize(UpdateTransformationData));

    addInstancedTests({&TextLayerTest::updateNoStyleSet,
                       &TextLayerTest::updateNoEditingStyleSet},
        Containers::arraySize(CreateUpdateNoStyleSetData));

    addInstancedTests({&TextLayerTest::sharedNeedsUpdateStatePropagatedToLayers},
        Containers::arraySize(SharedNeedsUpdateStatePropagatedToLayersData));

    addTests({&TextLayerTest::keyTextEvent});

    addInstancedTests({&TextLayerTest::keyTextEventSynthesizedFromPointerPress},
        Containers::arraySize(KeyTextEventSynthesizedFromPointerPressData));
}

using namespace Containers::Literals;

template<class> struct StyleTraits;
template<> struct StyleTraits<TextLayerCommonStyleUniform> {
    static const char* name() { return "TextLayerCommonStyleUniform"; }
};
template<> struct StyleTraits<TextLayerStyleUniform> {
    static const char* name() { return "TextLayerStyleUniform"; }
};
template<> struct StyleTraits<TextLayerCommonEditingStyleUniform> {
    static const char* name() { return "TextLayerCommonEditingStyleUniform"; }
};
template<> struct StyleTraits<TextLayerEditingStyleUniform> {
    static const char* name() { return "TextLayerEditingStyleUniform"; }
};

template<class T> void TextLayerTest::styleUniformSizeAlignment() {
    setTestCaseTemplateName(StyleTraits<T>::name());

    CORRADE_FAIL_IF(sizeof(T) % sizeof(Vector4) != 0, sizeof(T) << "is not a multiple of vec4 for UBO alignment.");

    /* 48-byte structures are fine, we'll align them to 768 bytes and not
       256, but warn about that */
    CORRADE_FAIL_IF(768 % sizeof(T) != 0, sizeof(T) << "can't fit exactly into 768-byte UBO alignment.");
    if(256 % sizeof(T) != 0)
        CORRADE_WARN(sizeof(T) << "can't fit exactly into 256-byte UBO alignment, only 768.");

    CORRADE_COMPARE(alignof(T), 4);
}

void TextLayerTest::styleUniformCommonConstructDefault() {
    TextLayerCommonStyleUniform a;
    TextLayerCommonStyleUniform b{DefaultInit};
    CORRADE_COMPARE(a.smoothness, 0.0f);
    CORRADE_COMPARE(b.smoothness, 0.0f);

    constexpr TextLayerCommonStyleUniform ca;
    constexpr TextLayerCommonStyleUniform cb{DefaultInit};
    CORRADE_COMPARE(ca.smoothness, 0.0f);
    CORRADE_COMPARE(cb.smoothness, 0.0f);

    CORRADE_VERIFY(std::is_nothrow_default_constructible<TextLayerCommonStyleUniform>::value);
    CORRADE_VERIFY(std::is_nothrow_constructible<TextLayerCommonStyleUniform, DefaultInitT>::value);

    /* Implicit construction is not allowed */
    CORRADE_VERIFY(!std::is_convertible<DefaultInitT, TextLayerCommonStyleUniform>::value);
}

void TextLayerTest::styleUniformCommonConstruct() {
    TextLayerCommonStyleUniform a{3.0f};
    CORRADE_COMPARE(a.smoothness, 3.0f);

    constexpr TextLayerCommonStyleUniform ca{3.0f};
    CORRADE_COMPARE(ca.smoothness, 3.0f);
}

void TextLayerTest::styleUniformCommonConstructNoInit() {
    /* Testing only some fields, should be enough */
    TextLayerCommonStyleUniform a;
    a.smoothness = 3.0f;

    new(&a) TextLayerCommonStyleUniform{NoInit};
    {
        /* Explicitly check we're not on Clang because certain Clang-based IDEs
           inherit __GNUC__ if GCC is used instead of leaving it at 4 like
           Clang itself does */
        #if defined(CORRADE_TARGET_GCC) && !defined(CORRADE_TARGET_CLANG) && __GNUC__*100 + __GNUC_MINOR__ >= 601 && __OPTIMIZE__
        CORRADE_EXPECT_FAIL("GCC 6.1+ misoptimizes and overwrites the value.");
        #endif
        CORRADE_COMPARE(a.smoothness, 3.0f);
    }

    /* Implicit construction is not allowed */
    CORRADE_VERIFY(!std::is_convertible<NoInitT, TextLayerCommonStyleUniform>::value);
}

void TextLayerTest::styleUniformCommonSetters() {
    TextLayerCommonStyleUniform a;
    a.setSmoothness(34.0f);
    CORRADE_COMPARE(a.smoothness, 34.0f);
}

void TextLayerTest::styleUniformConstructDefault() {
    TextLayerStyleUniform a;
    TextLayerStyleUniform b{DefaultInit};
    CORRADE_COMPARE(a.color, 0xffffffff_srgbaf);
    CORRADE_COMPARE(b.color, 0xffffffff_srgbaf);
    CORRADE_COMPARE(a.outlineColor, 0xffffffff_srgbaf);
    CORRADE_COMPARE(b.outlineColor, 0xffffffff_srgbaf);
    CORRADE_COMPARE(a.outlineWidth, 0.0f);
    CORRADE_COMPARE(b.outlineWidth, 0.0f);
    CORRADE_COMPARE(a.edgeOffset, 0.0f);
    CORRADE_COMPARE(b.edgeOffset, 0.0f);
    CORRADE_COMPARE(a.smoothness, 0.0f);
    CORRADE_COMPARE(b.smoothness, 0.0f);

    constexpr TextLayerStyleUniform ca;
    constexpr TextLayerStyleUniform cb{DefaultInit};
    CORRADE_COMPARE(ca.color, 0xffffffff_srgbaf);
    CORRADE_COMPARE(cb.color, 0xffffffff_srgbaf);
    CORRADE_COMPARE(ca.outlineColor, 0xffffffff_srgbaf);
    CORRADE_COMPARE(cb.outlineColor, 0xffffffff_srgbaf);
    CORRADE_COMPARE(ca.outlineWidth, 0.0f);
    CORRADE_COMPARE(cb.outlineWidth, 0.0f);
    CORRADE_COMPARE(ca.edgeOffset, 0.0f);
    CORRADE_COMPARE(cb.edgeOffset, 0.0f);
    CORRADE_COMPARE(ca.smoothness, 0.0f);
    CORRADE_COMPARE(cb.smoothness, 0.0f);

    CORRADE_VERIFY(std::is_nothrow_default_constructible<TextLayerStyleUniform>::value);
    CORRADE_VERIFY(std::is_nothrow_constructible<TextLayerStyleUniform, DefaultInitT>::value);

    /* Implicit construction is not allowed */
    CORRADE_VERIFY(!std::is_convertible<DefaultInitT, TextLayerStyleUniform>::value);
}

void TextLayerTest::styleUniformConstruct() {
    TextLayerStyleUniform a{0xff336699_rgbaf};
    CORRADE_COMPARE(a.color, 0xff336699_rgbaf);
    CORRADE_COMPARE(a.outlineColor, 0xffffffff_srgbaf);
    CORRADE_COMPARE(a.outlineWidth, 0.0f);
    CORRADE_COMPARE(a.edgeOffset, 0.0f);
    CORRADE_COMPARE(a.smoothness, 0.0f);

    constexpr TextLayerStyleUniform ca{0xff336699_rgbaf};
    CORRADE_COMPARE(ca.color, 0xff336699_rgbaf);
    CORRADE_COMPARE(ca.outlineColor, 0xffffffff_srgbaf);
    CORRADE_COMPARE(ca.outlineWidth, 0.0f);
    CORRADE_COMPARE(ca.edgeOffset, 0.0f);
    CORRADE_COMPARE(ca.smoothness, 0.0f);
}

void TextLayerTest::styleUniformConstructDistanceField() {
    TextLayerStyleUniform a{0xff336699_rgbaf, 0xaabbccdd_rgbaf, 2.0f, 3.0f, 4.0f};
    CORRADE_COMPARE(a.color, 0xff336699_rgbaf);
    CORRADE_COMPARE(a.outlineColor, 0xaabbccdd_rgbaf);
    CORRADE_COMPARE(a.outlineWidth, 2.0f);
    CORRADE_COMPARE(a.edgeOffset, 3.0f);
    CORRADE_COMPARE(a.smoothness, 4.0f);

    constexpr TextLayerStyleUniform ca{0xff336699_rgbaf, 0xaabbccdd_rgbaf, 2.0f, 3.0f, 4.0f};
    CORRADE_COMPARE(ca.color, 0xff336699_rgbaf);
    CORRADE_COMPARE(ca.outlineColor, 0xaabbccdd_rgbaf);
    CORRADE_COMPARE(ca.outlineWidth, 2.0f);
    CORRADE_COMPARE(ca.edgeOffset, 3.0f);
    CORRADE_COMPARE(ca.smoothness, 4.0f);
}

void TextLayerTest::styleUniformConstructNoInit() {
    /* Testing only some fields, should be enough */
    TextLayerStyleUniform a;
    a.color = 0xff3366_rgbf;
    a.outlineWidth = 3.5f;

    new(&a) TextLayerStyleUniform{NoInit};
    {
        /* Explicitly check we're not on Clang because certain Clang-based IDEs
           inherit __GNUC__ if GCC is used instead of leaving it at 4 like
           Clang itself does */
        #if defined(CORRADE_TARGET_GCC) && !defined(CORRADE_TARGET_CLANG) && __GNUC__*100 + __GNUC_MINOR__ >= 601 && __OPTIMIZE__
        CORRADE_EXPECT_FAIL("GCC 6.1+ misoptimizes and overwrites the value.");
        #endif
        CORRADE_COMPARE(a.color, 0xff3366_rgbf);
        CORRADE_COMPARE(a.outlineWidth, 3.5f);
    }

    /* Implicit construction is not allowed */
    CORRADE_VERIFY(!std::is_convertible<NoInitT, TextLayerStyleUniform>::value);
}

void TextLayerTest::styleUniformSetters() {
    TextLayerStyleUniform a;
    a.setColor(0xff336699_rgbaf)
     .setOutlineColor(0x663399ff_rgbaf)
     .setOutlineWidth(3.0f)
     .setEdgeOffset(-4.0f)
     .setSmoothness(0.3f);
    CORRADE_COMPARE(a.color, 0xff336699_rgbaf);
    CORRADE_COMPARE(a.outlineColor, 0x663399ff_rgbaf);
    CORRADE_COMPARE(a.outlineWidth, 3.0f);
    CORRADE_COMPARE(a.edgeOffset, -4.0f);
    CORRADE_COMPARE(a.smoothness, 0.3f);
}

void TextLayerTest::editingStyleUniformCommonConstructDefault() {
    TextLayerCommonEditingStyleUniform a;
    TextLayerCommonEditingStyleUniform b{DefaultInit};
    CORRADE_COMPARE(a.smoothness, 0.0f);
    CORRADE_COMPARE(b.smoothness, 0.0f);

    constexpr TextLayerCommonEditingStyleUniform ca;
    constexpr TextLayerCommonEditingStyleUniform cb{DefaultInit};
    CORRADE_COMPARE(ca.smoothness, 0.0f);
    CORRADE_COMPARE(cb.smoothness, 0.0f);

    CORRADE_VERIFY(std::is_nothrow_default_constructible<TextLayerCommonEditingStyleUniform>::value);
    CORRADE_VERIFY(std::is_nothrow_constructible<TextLayerCommonEditingStyleUniform, DefaultInitT>::value);

    /* Implicit construction is not allowed */
    CORRADE_VERIFY(!std::is_convertible<DefaultInitT, TextLayerCommonEditingStyleUniform>::value);
}

void TextLayerTest::editingStyleUniformCommonConstruct() {
    TextLayerCommonEditingStyleUniform a{3.0f};
    CORRADE_COMPARE(a.smoothness, 3.0f);

    constexpr TextLayerCommonEditingStyleUniform ca{3.0f};
    CORRADE_COMPARE(ca.smoothness, 3.0f);
}

void TextLayerTest::editingStyleUniformCommonConstructNoInit() {
    /* Testing only some fields, should be enough */
    TextLayerCommonEditingStyleUniform a;
    a.smoothness = 3.0f;

    new(&a) TextLayerCommonEditingStyleUniform{NoInit};
    {
        /* Explicitly check we're not on Clang because certain Clang-based IDEs
           inherit __GNUC__ if GCC is used instead of leaving it at 4 like
           Clang itself does */
        #if defined(CORRADE_TARGET_GCC) && !defined(CORRADE_TARGET_CLANG) && __GNUC__*100 + __GNUC_MINOR__ >= 601 && __OPTIMIZE__
        CORRADE_EXPECT_FAIL("GCC 6.1+ misoptimizes and overwrites the value.");
        #endif
        CORRADE_COMPARE(a.smoothness, 3.0f);
    }

    /* Implicit construction is not allowed */
    CORRADE_VERIFY(!std::is_convertible<NoInitT, TextLayerCommonEditingStyleUniform>::value);
}

void TextLayerTest::editingStyleUniformCommonSetters() {
    TextLayerCommonEditingStyleUniform a;
    a.setSmoothness(34.0f);
    CORRADE_COMPARE(a.smoothness, 34.0f);
}

void TextLayerTest::editingStyleUniformConstructDefault() {
    TextLayerEditingStyleUniform a;
    TextLayerEditingStyleUniform b{DefaultInit};
    CORRADE_COMPARE(a.backgroundColor, 0xffffffff_srgbaf);
    CORRADE_COMPARE(b.backgroundColor, 0xffffffff_srgbaf);
    CORRADE_COMPARE(a.cornerRadius, 0.0f);
    CORRADE_COMPARE(b.cornerRadius, 0.0f);

    constexpr TextLayerEditingStyleUniform ca;
    constexpr TextLayerEditingStyleUniform cb{DefaultInit};
    CORRADE_COMPARE(ca.backgroundColor, 0xffffffff_srgbaf);
    CORRADE_COMPARE(cb.backgroundColor, 0xffffffff_srgbaf);
    CORRADE_COMPARE(ca.cornerRadius, 0.0f);
    CORRADE_COMPARE(cb.cornerRadius, 0.0f);

    CORRADE_VERIFY(std::is_nothrow_default_constructible<TextLayerEditingStyleUniform>::value);
    CORRADE_VERIFY(std::is_nothrow_constructible<TextLayerEditingStyleUniform, DefaultInitT>::value);

    /* Implicit construction is not allowed */
    CORRADE_VERIFY(!std::is_convertible<DefaultInitT, TextLayerEditingStyleUniform>::value);
}

void TextLayerTest::editingStyleUniformConstruct() {
    TextLayerEditingStyleUniform a{0xff336699_rgbaf, 4.0f};
    CORRADE_COMPARE(a.backgroundColor, 0xff336699_rgbaf);
    CORRADE_COMPARE(a.cornerRadius, 4.0f);

    constexpr TextLayerEditingStyleUniform ca{0xff336699_rgbaf, 4.0f};
    CORRADE_COMPARE(ca.backgroundColor, 0xff336699_rgbaf);
    CORRADE_COMPARE(ca.cornerRadius, 4.0f);
}

void TextLayerTest::editingStyleUniformConstructNoInit() {
    /* Testing only some fields, should be enough */
    TextLayerEditingStyleUniform a;
    a.backgroundColor = 0xff3366_rgbf;
    a.cornerRadius = 34.0f;

    new(&a) TextLayerEditingStyleUniform{NoInit};
    {
        /* Explicitly check we're not on Clang because certain Clang-based IDEs
           inherit __GNUC__ if GCC is used instead of leaving it at 4 like
           Clang itself does */
        #if defined(CORRADE_TARGET_GCC) && !defined(CORRADE_TARGET_CLANG) && __GNUC__*100 + __GNUC_MINOR__ >= 601 && __OPTIMIZE__
        CORRADE_EXPECT_FAIL("GCC 6.1+ misoptimizes and overwrites the value.");
        #endif
        CORRADE_COMPARE(a.backgroundColor, 0xff3366_rgbf);
        CORRADE_COMPARE(a.cornerRadius, 34.0f);
    }

    /* Implicit construction is not allowed */
    CORRADE_VERIFY(!std::is_convertible<NoInitT, TextLayerEditingStyleUniform>::value);
}

void TextLayerTest::editingStyleUniformSetters() {
    TextLayerEditingStyleUniform a;
    a.setBackgroundColor(0xff336699_rgbaf)
     .setCornerRadius(34.0f);
    CORRADE_COMPARE(a.backgroundColor, 0xff336699_rgbaf);
    CORRADE_COMPARE(a.cornerRadius, 34.0f);
}

void TextLayerTest::fontHandle() {
    CORRADE_COMPARE(FontHandle::Null, FontHandle{});
    CORRADE_COMPARE(Ui::fontHandle(0, 0), FontHandle::Null);
    CORRADE_COMPARE(Ui::fontHandle(0x2bcd, 0x1), FontHandle(0xabcd));
    CORRADE_COMPARE(Ui::fontHandle(0x7fff, 0x1), FontHandle(0xffff));
    CORRADE_COMPARE(fontHandleId(FontHandle(0xabcd)), 0x2bcd);
    CORRADE_COMPARE(fontHandleGeneration(FontHandle::Null), 0);
    CORRADE_COMPARE(fontHandleGeneration(FontHandle(0xabcd)), 0x1);

    constexpr FontHandle handle = Ui::fontHandle(0x2bcd, 0x1);
    constexpr UnsignedInt id = fontHandleId(handle);
    constexpr UnsignedInt generation = fontHandleGeneration(handle);
    CORRADE_COMPARE(handle, FontHandle(0xabcd));
    CORRADE_COMPARE(id, 0x2bcd);
    CORRADE_COMPARE(generation, 0x1);
}

void TextLayerTest::fontHandleInvalid() {
    CORRADE_SKIP_IF_NO_DEBUG_ASSERT();

    Containers::String out;
    Error redirectError{&out};
    Ui::fontHandle(0x8000, 0x1);
    Ui::fontHandle(0x1, 0x2);
    Ui::fontHandleId(FontHandle::Null);
    CORRADE_COMPARE_AS(out,
        "Ui::fontHandle(): expected index to fit into 15 bits and generation into 1, got 0x8000 and 0x1\n"
        "Ui::fontHandle(): expected index to fit into 15 bits and generation into 1, got 0x1 and 0x2\n"
        "Ui::fontHandleId(): the handle is null\n",
        TestSuite::Compare::String);
}

void TextLayerTest::debugFontHandle() {
    Containers::String out;
    Debug{&out} << FontHandle::Null << Ui::fontHandle(0x2bcd, 0x1);
    CORRADE_COMPARE(out, "Ui::FontHandle::Null Ui::FontHandle(0x2bcd, 0x1)\n");
}

void TextLayerTest::debugFontHandlePacked() {
    Containers::String out;
    /* Last is not packed, ones before should not make any flags persistent */
    Debug{&out} << Debug::packed << FontHandle::Null << Debug::packed << Ui::fontHandle(0x2bcd, 0x1) << Ui::fontHandle(0x3abc, 0x1);
    CORRADE_COMPARE(out, "Null {0x2bcd, 0x1} Ui::FontHandle(0x3abc, 0x1)\n");
}

void TextLayerTest::debugLayerFlag() {
    Containers::String out;
    Debug{&out} << TextLayerFlag::Transformable << TextLayerFlag(0xbe);
    CORRADE_COMPARE(out, "Ui::TextLayerFlag::Transformable Ui::TextLayerFlag(0xbe)\n");
}

void TextLayerTest::debugLayerFlags() {
    Containers::String out;
    Debug{&out} << (TextLayerFlag::Transformable|TextLayerFlag(0xa0)) << TextLayerFlags{};
    CORRADE_COMPARE(out, "Ui::TextLayerFlag::Transformable|Ui::TextLayerFlag(0xa0) Ui::TextLayerFlags{}\n");
}

void TextLayerTest::debugDataFlag() {
    Containers::String out;
    Debug{&out} << TextDataFlag::Editable << TextDataFlag(0xbe);
    CORRADE_COMPARE(out, "Ui::TextDataFlag::Editable Ui::TextDataFlag(0xbe)\n");
}

void TextLayerTest::debugDataFlagPacked() {
    Containers::String out;
    /* Last is not packed, ones before should not make any flags persistent */
    Debug{&out} << Debug::packed << TextDataFlag::Editable << Debug::packed << TextDataFlag(0xbe) << TextDataFlag::Editable;
    CORRADE_COMPARE(out, "Editable 0xbe Ui::TextDataFlag::Editable\n");
}

void TextLayerTest::debugDataFlags() {
    Containers::String out;
    Debug{&out} << (TextDataFlag::Editable|TextDataFlag(0xa0)) << TextDataFlags{};
    CORRADE_COMPARE(out, "Ui::TextDataFlag::Editable|Ui::TextDataFlag(0xa0) Ui::TextDataFlags{}\n");
}

void TextLayerTest::debugDataFlagsPacked() {
    Containers::String out;
    /* Last is not packed, ones before should not make any flags persistent */
    Debug{&out} << Debug::packed << (TextDataFlag::Editable|TextDataFlag(0xa0)) << Debug::packed << TextDataFlags{} << (TextDataFlag::Editable|TextDataFlag(0xa0));
    CORRADE_COMPARE(out, "Editable|0xa0 {} Ui::TextDataFlag::Editable|Ui::TextDataFlag(0xa0)\n");
}

void TextLayerTest::debugEdit() {
    Containers::String out;
    Debug{&out} << TextEdit::MoveCursorLineBegin << TextEdit(0xbe);
    CORRADE_COMPARE(out, "Ui::TextEdit::MoveCursorLineBegin Ui::TextEdit(0xbe)\n");
}

void TextLayerTest::sharedDebugFlag() {
    Containers::String out;
    Debug{&out} << TextLayerSharedFlag::DistanceField << TextLayerSharedFlag(0xbe);
    CORRADE_COMPARE(out, "Ui::TextLayerSharedFlag::DistanceField Ui::TextLayerSharedFlag(0xbe)\n");
}

void TextLayerTest::sharedDebugFlags() {
    Containers::String out;
    Debug{&out} << (TextLayerSharedFlag::DistanceField|TextLayerSharedFlag(0x80)) << TextLayerSharedFlags{};
    CORRADE_COMPARE(out, "Ui::TextLayerSharedFlag::DistanceField|Ui::TextLayerSharedFlag(0x80) Ui::TextLayerSharedFlags{}\n");
}

void TextLayerTest::sharedConfigurationConstruct() {
    TextLayer::Shared::Configuration configuration{3, 5};
    CORRADE_COMPARE(configuration.styleUniformCount(), 3);
    CORRADE_COMPARE(configuration.styleCount(), 5);
}

void TextLayerTest::sharedConfigurationConstructSameStyleUniformCount() {
    TextLayer::Shared::Configuration configuration{3};
    CORRADE_COMPARE(configuration.styleUniformCount(), 3);
    CORRADE_COMPARE(configuration.styleCount(), 3);
}

void TextLayerTest::sharedConfigurationConstructZeroStyleOrUniformCount() {
    CORRADE_SKIP_IF_NO_ASSERT();

    /* Both being zero is fine */
    TextLayer::Shared::Configuration{0, 0};
    TextLayer::Shared::Configuration{0};

    Containers::String out;
    Error redirectError{&out};
    TextLayer::Shared::Configuration{0, 4};
    TextLayer::Shared::Configuration{4, 0};
    CORRADE_COMPARE_AS(out,
        "Ui::TextLayer::Shared::Configuration: expected style uniform count and style count to be either both zero or both non-zero, got 0 and 4\n"
        "Ui::TextLayer::Shared::Configuration: expected style uniform count and style count to be either both zero or both non-zero, got 4 and 0\n",
        TestSuite::Compare::String);
}

void TextLayerTest::sharedConfigurationConstructCopy() {
    /* Testing just some properties, it's an implicitly generated copy */
    TextLayer::Shared::Configuration a{3, 5};

    TextLayer::Shared::Configuration b = a;
    CORRADE_COMPARE(b.styleUniformCount(), 3);
    CORRADE_COMPARE(b.styleCount(), 5);

    TextLayer::Shared::Configuration c{7, 9};
    c = b;
    CORRADE_COMPARE(c.styleUniformCount(), 3);
    CORRADE_COMPARE(c.styleCount(), 5);

    #ifndef CORRADE_NO_STD_IS_TRIVIALLY_TRAITS
    CORRADE_VERIFY(std::is_trivially_copy_constructible<TextLayer::Shared::Configuration>::value);
    CORRADE_VERIFY(std::is_trivially_copy_assignable<TextLayer::Shared::Configuration>::value);
    #endif
}

void TextLayerTest::sharedConfigurationSetters() {
    TextLayer::Shared::Configuration configuration{3, 5};
    CORRADE_COMPARE(configuration.editingStyleUniformCount(), 0);
    CORRADE_COMPARE(configuration.editingStyleCount(), 0);
    CORRADE_COMPARE(configuration.dynamicStyleCount(), 0);
    CORRADE_COMPARE(configuration.hasEditingStyles(), false);
    CORRADE_COMPARE(configuration.flags(), TextLayerSharedFlags{});

    configuration
        .setEditingStyleCount(2, 7)
        .setDynamicStyleCount(9)
        .setFlags(TextLayerSharedFlag::DistanceField)
        .addFlags(TextLayerSharedFlag(0xe0))
        .clearFlags(TextLayerSharedFlag(0x70));
    CORRADE_COMPARE(configuration.editingStyleUniformCount(), 2);
    CORRADE_COMPARE(configuration.editingStyleCount(), 7);
    CORRADE_COMPARE(configuration.dynamicStyleCount(), 9);
    CORRADE_COMPARE(configuration.hasEditingStyles(), true);
    CORRADE_COMPARE(configuration.flags(), TextLayerSharedFlag::DistanceField|TextLayerSharedFlag(0x80));

    /* Disabling dynamic editing styles if there's non-zero editing style count
       is a no-op */
    configuration.setDynamicStyleCount(9, false);
    CORRADE_COMPARE(configuration.editingStyleUniformCount(), 2);
    CORRADE_COMPARE(configuration.editingStyleCount(), 7);
    CORRADE_COMPARE(configuration.dynamicStyleCount(), 9);
    CORRADE_COMPARE(configuration.hasEditingStyles(), true);

    /* Dynamic editing styles are by default not enabled if there's no static
       editing styles */
    configuration.setEditingStyleCount(0, 0);
    CORRADE_COMPARE(configuration.editingStyleUniformCount(), 0);
    CORRADE_COMPARE(configuration.editingStyleCount(), 0);
    CORRADE_COMPARE(configuration.dynamicStyleCount(), 9);
    CORRADE_COMPARE(configuration.hasEditingStyles(), false);

    /* But one can opt in */
    configuration.setDynamicStyleCount(8, true);
    CORRADE_COMPARE(configuration.editingStyleUniformCount(), 0);
    CORRADE_COMPARE(configuration.editingStyleCount(), 0);
    CORRADE_COMPARE(configuration.dynamicStyleCount(), 8);
    CORRADE_COMPARE(configuration.hasEditingStyles(), true);

    /* Similarly in case there's no static styles at all */
    TextLayer::Shared::Configuration zeroStyles{0};
    CORRADE_COMPARE(zeroStyles.editingStyleCount(), 0);
    CORRADE_COMPARE(zeroStyles.dynamicStyleCount(), 0);
    CORRADE_COMPARE(zeroStyles.hasEditingStyles(), false);

    zeroStyles.setDynamicStyleCount(11, false);
    CORRADE_COMPARE(zeroStyles.editingStyleCount(), 0);
    CORRADE_COMPARE(zeroStyles.dynamicStyleCount(), 11);
    CORRADE_COMPARE(zeroStyles.hasEditingStyles(), false);

    zeroStyles.setDynamicStyleCount(11, true);
    CORRADE_COMPARE(zeroStyles.editingStyleCount(), 0);
    CORRADE_COMPARE(zeroStyles.dynamicStyleCount(), 11);
    CORRADE_COMPARE(zeroStyles.hasEditingStyles(), true);

    /* With zero dynamic styles enabling editing styles should be a no-op */
    zeroStyles.setDynamicStyleCount(0, true);
    CORRADE_COMPARE(zeroStyles.editingStyleCount(), 0);
    CORRADE_COMPARE(zeroStyles.dynamicStyleCount(), 0);
    CORRADE_COMPARE(zeroStyles.hasEditingStyles(), false);
}

void TextLayerTest::sharedConfigurationSettersSameEditingStyleUniformCount() {
    TextLayer::Shared::Configuration configuration{3, 5};
    CORRADE_COMPARE(configuration.editingStyleUniformCount(), 0);
    CORRADE_COMPARE(configuration.editingStyleCount(), 0);

    configuration.setEditingStyleCount(2);
    CORRADE_COMPARE(configuration.editingStyleUniformCount(), 2);
    CORRADE_COMPARE(configuration.editingStyleCount(), 2);
}

void TextLayerTest::sharedConfigurationSettersInvalidEditingStyleOrUniformCount() {
    CORRADE_SKIP_IF_NO_ASSERT();

    /* Both being zero is fine */
    TextLayer::Shared::Configuration configuration{2, 3};
    configuration
        .setEditingStyleCount(0, 0)
        .setEditingStyleCount(0);

    TextLayer::Shared::Configuration zeroStyles{0};

    Containers::String out;
    Error redirectError{&out};
    configuration.setEditingStyleCount(0, 4);
    configuration.setEditingStyleCount(4, 0);
    zeroStyles.setEditingStyleCount(3, 2);
    CORRADE_COMPARE_AS(out,
        "Ui::TextLayer::Shared::Configuration::setEditingStyleCount(): expected uniform count and count to be either both zero or both non-zero, got 0 and 4\n"
        "Ui::TextLayer::Shared::Configuration::setEditingStyleCount(): expected uniform count and count to be either both zero or both non-zero, got 4 and 0\n"
        "Ui::TextLayer::Shared::Configuration::setEditingStyleCount(): editing style count has to be zero if style count is zero, got 2\n",
        TestSuite::Compare::String);
}

void TextLayerTest::sharedConstruct() {
    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32}};

    struct Shared: TextLayer::Shared {
        explicit Shared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{3, 5}
        .setEditingStyleCount(2, 7)
        .setDynamicStyleCount(4)
        .setFlags(TextLayerSharedFlag::DistanceField)
    };
    CORRADE_COMPARE(shared.styleUniformCount(), 3);
    CORRADE_COMPARE(shared.styleCount(), 5);
    CORRADE_COMPARE(shared.editingStyleUniformCount(), 2);
    CORRADE_COMPARE(shared.editingStyleCount(), 7);
    CORRADE_COMPARE(shared.dynamicStyleCount(), 4);
    CORRADE_VERIFY(shared.hasEditingStyles());
    CORRADE_COMPARE(shared.flags(), TextLayerSharedFlag::DistanceField);

    CORRADE_COMPARE(&shared.glyphCache(), &cache);
    CORRADE_COMPARE(&static_cast<const Shared&>(shared).glyphCache(), &cache);

    CORRADE_COMPARE(shared.fontCount(), 0);
    CORRADE_VERIFY(!shared.isHandleValid(FontHandle::Null));
}

void TextLayerTest::sharedConstructNoCreate() {
    struct Shared: TextLayer::Shared {
        explicit Shared(NoCreateT): TextLayer::Shared{NoCreate} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{NoCreate};

    /* Shouldn't crash */
    CORRADE_VERIFY(true);

    /* Implicit construction is not allowed */
    CORRADE_VERIFY(!std::is_convertible<NoCreateT, TextLayer::Shared>::value);
}

void TextLayerTest::sharedConstructCopy() {
    struct Shared: TextLayer::Shared {
        /* Clang says the constructor is unused otherwise. However I fear that
           without the constructor the type would be impossible to construct
           (and thus also to copy), leading to false positives in the trait
           check below */
        explicit CORRADE_UNUSED Shared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    };

    CORRADE_VERIFY(!std::is_copy_constructible<Shared>{});
    CORRADE_VERIFY(!std::is_copy_assignable<Shared>{});
}

void TextLayerTest::sharedConstructMove() {
    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32}},
      cache2{PixelFormat::RGBA8Unorm, {8, 8}};

    struct Shared: TextLayer::Shared {
        explicit Shared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    };

    Shared a{cache, TextLayer::Shared::Configuration{3, 5}
        .setDynamicStyleCount(4)
    };

    Shared b{Utility::move(a)};
    CORRADE_COMPARE(b.styleUniformCount(), 3);
    CORRADE_COMPARE(b.styleCount(), 5);
    CORRADE_COMPARE(b.dynamicStyleCount(), 4);
    CORRADE_COMPARE(&b.glyphCache(), &cache);

    Shared c{cache, TextLayer::Shared::Configuration{5, 7}};
    c = Utility::move(b);
    CORRADE_COMPARE(c.styleUniformCount(), 3);
    CORRADE_COMPARE(c.styleCount(), 5);
    CORRADE_COMPARE(c.dynamicStyleCount(), 4);
    CORRADE_COMPARE(&c.glyphCache(), &cache);

    CORRADE_VERIFY(std::is_nothrow_move_constructible<Shared>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<Shared>::value);
}

void TextLayerTest::sharedConstructZeroStyleCount() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32}};

    struct Shared: TextLayer::Shared {
        explicit Shared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    };

    /* Zero style count or dynamic style count is fine on its own */
    Shared{cache, Shared::Configuration{0}.setDynamicStyleCount(1)};
    Shared{cache, Shared::Configuration{1}.setDynamicStyleCount(0)};

    Containers::String out;
    Error redirectError{&out};
    Shared{cache, Shared::Configuration{0}.setDynamicStyleCount(0)};
    CORRADE_COMPARE(out, "Ui::TextLayer::Shared: expected non-zero total style count\n");
}

void TextLayerTest::sharedAddFont() {
    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};

    struct Shared: TextLayer::Shared {
        explicit Shared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{3, 5}};
    CORRADE_COMPARE(shared.fontCount(), 0);

    struct
        /* MSVC 2017 (_MSC_VER == 191x) crashes at runtime accessing instance2
           in the following case. Not a problem in MSVC 2015 or 2019+.
            struct: SomeBase {
            } instance1, instance2;
           Simply naming the derived struct is enough to fix the crash, FFS. */
        #if defined(CORRADE_TARGET_MSVC) && _MSC_VER >= 1910 && _MSC_VER < 1920
        ThisNameAlonePreventsMSVC2017FromBlowingUp
        #endif
        : Text::AbstractFont
    {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return {}; }
    } font1, font2;

    /* First font */
    UnsignedInt firstFontId = cache.addFont(13, &font1);
    FontHandle first = shared.addFont(font1, 13.0f);
    CORRADE_COMPARE(first, Ui::fontHandle(0, 1));
    CORRADE_COMPARE(shared.fontCount(), 1);
    CORRADE_VERIFY(shared.isHandleValid(first));
    CORRADE_COMPARE(shared.glyphCacheFontId(first), firstFontId);
    CORRADE_VERIFY(shared.hasFontInstance(first));
    CORRADE_COMPARE(&shared.font(first), &font1);
    /* Const overload */
    CORRADE_COMPARE(&const_cast<const Shared&>(shared).font(first), &font1);

    /* Second font, instanceless */
    UnsignedInt secondFontId = cache.addFont(223);
    FontHandle second = shared.addInstancelessFont(secondFontId, 0.5f);
    CORRADE_COMPARE(second, Ui::fontHandle(1, 1));
    CORRADE_COMPARE(shared.fontCount(), 2);
    CORRADE_VERIFY(shared.isHandleValid(second));
    CORRADE_COMPARE(shared.glyphCacheFontId(second), secondFontId);
    CORRADE_VERIFY(!shared.hasFontInstance(second));

    /* Third font */
    UnsignedInt thirdFontId = cache.addFont(56, &font2);
    FontHandle third = shared.addFont(font2, 6.0f);
    CORRADE_COMPARE(third, Ui::fontHandle(2, 1));
    CORRADE_COMPARE(shared.fontCount(), 3);
    CORRADE_VERIFY(shared.isHandleValid(third));
    CORRADE_COMPARE(shared.glyphCacheFontId(third), thirdFontId);
    CORRADE_VERIFY(shared.hasFontInstance(third));
    CORRADE_COMPARE(&shared.font(third), &font2);
    /* Const overload */
    CORRADE_COMPARE(&const_cast<const Shared&>(shared).font(third), &font2);

    /* Fourth font, instanceless */
    UnsignedInt fourthFontId = cache.addFont(117);
    FontHandle fourth = shared.addInstancelessFont(fourthFontId, 2.0f);
    CORRADE_COMPARE(fourth, Ui::fontHandle(3, 1));
    CORRADE_COMPARE(shared.fontCount(), 4);
    CORRADE_VERIFY(shared.isHandleValid(fourth));
    CORRADE_COMPARE(shared.glyphCacheFontId(second), secondFontId);
    CORRADE_VERIFY(!shared.hasFontInstance(second));
}

void TextLayerTest::sharedAddFontTakeOwnership() {
    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};

    struct Font: Text::AbstractFont {
        explicit Font(Int& destructed): _destructed(destructed) {}

        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        ~Font() {
            ++_destructed;
        }

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return {}; }

        private:
            Int& _destructed;
    };

    Int destructed = 0;

    {
        struct Shared: TextLayer::Shared {
            explicit Shared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

            void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
            void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
        } shared{cache, TextLayer::Shared::Configuration{3, 5}};
        CORRADE_COMPARE(shared.fontCount(), 0);

        Containers::Pointer<Font> font1{InPlaceInit, destructed};
        UnsignedInt firstFontId = cache.addFont(13, font1.get());
        Font* pointer1 = font1.get();
        FontHandle first = shared.addFont(Utility::move(font1), 13.0f);
        CORRADE_COMPARE(first, Ui::fontHandle(0, 1));
        CORRADE_COMPARE(shared.fontCount(), 1);
        CORRADE_VERIFY(shared.isHandleValid(first));
        CORRADE_COMPARE(shared.glyphCacheFontId(first), firstFontId);
        CORRADE_COMPARE(&shared.font(first), pointer1);

        /* It should be possible to add a second font using the same pointer
           but different options */
        FontHandle second = shared.addFont(*pointer1, 6.0f);
        CORRADE_COMPARE(second, Ui::fontHandle(1, 1));
        CORRADE_COMPARE(shared.fontCount(), 2);
        CORRADE_VERIFY(shared.isHandleValid(second));
        CORRADE_COMPARE(shared.glyphCacheFontId(second), firstFontId);
        CORRADE_COMPARE(&shared.font(second), pointer1);

        /* Add a second font, to verify both get deleted appropriately */
        Containers::Pointer<Font> font2{InPlaceInit, destructed};
        UnsignedInt thirdFontId = cache.addFont(13, font2.get());
        Font* pointer2 = font2.get();
        FontHandle third = shared.addFont(Utility::move(font2), 22.0f);
        CORRADE_COMPARE(third, Ui::fontHandle(2, 1));
        CORRADE_COMPARE(shared.fontCount(), 3);
        CORRADE_VERIFY(shared.isHandleValid(third));
        CORRADE_COMPARE(shared.glyphCacheFontId(third), thirdFontId);
        CORRADE_COMPARE(&shared.font(third), pointer2);
    }

    /* The owned instances should be destructed exactly once */
    CORRADE_COMPARE(destructed, 2);
}

void TextLayerTest::sharedAddFontTakeOwnershipNull() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32}};

    struct Shared: TextLayer::Shared {
        explicit Shared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{3, 5}};
    CORRADE_COMPARE(shared.fontCount(), 0);

    Containers::String out;
    Error redirectError{&out};
    shared.addFont(nullptr, 13.0f);
    CORRADE_COMPARE(out, "Ui::TextLayer::Shared::addFont(): font is null\n");
}

void TextLayerTest::sharedAddFontNotFoundInCache() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};

    /* Add some other fonts to the cache to verify it's not just checking for
       the cache being non-empty */
    cache.addFont(67);
    cache.addFont(36);

    struct Shared: TextLayer::Shared {
        explicit Shared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{3, 5}};

    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return false; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return {}; }
    } font;

    Containers::String out;
    Error redirectError{&out};
    shared.addFont(font, 1.0f);
    shared.addInstancelessFont(2, 1.0f);
    CORRADE_COMPARE(out,
        "Ui::TextLayer::Shared::addFont(): font not found among 2 fonts in associated glyph cache\n"
        "Ui::TextLayer::Shared::addInstancelessFont(): index 2 out of range for 2 fonts in associated glyph cache\n");
}

void TextLayerTest::sharedAddFontNoHandlesLeft() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return {}; }
    } font;

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};
    cache.addFont(67, &font);

    UnsignedInt glyphCacheInstanceLessFontId = cache.addFont(223);

    struct Shared: TextLayer::Shared {
        explicit Shared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{3, 5}};

    FontHandle handle;
    for(std::size_t i = 0; i != 1 << Implementation::FontHandleIdBits; ++i)
        handle = shared.addFont(font, 1.0f);
    CORRADE_COMPARE(handle, Ui::fontHandle((1 << Implementation::FontHandleIdBits) - 1, 1));

    CORRADE_COMPARE(shared.fontCount(), 1 << Implementation::FontHandleIdBits);

    Containers::String out;
    Error redirectError{&out};
    shared.addFont(font, 1.0f);
    shared.addInstancelessFont(glyphCacheInstanceLessFontId, 1.0f);
    /* Number is hardcoded in the expected message but not elsewhere in order
       to give a heads-up when modifying the handle ID bit count */
    CORRADE_COMPARE(out,
        "Ui::TextLayer::Shared::addFont(): can only have at most 32768 fonts\n"
        "Ui::TextLayer::Shared::addInstancelessFont(): can only have at most 32768 fonts\n");
}

void TextLayerTest::sharedAddInstancelessFontHasInstance() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return false; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return {}; }
    } font;

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};

    /* Add a font without an instance to check it's looking at the correct
       one */
    cache.addFont(223);
    UnsignedInt glyphCacheFontId = cache.addFont(67, &font);

    struct Shared: TextLayer::Shared {
        explicit Shared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{3, 5}};

    Containers::String out;
    Error redirectError{&out};
    shared.addInstancelessFont(glyphCacheFontId, 1.0f);
    CORRADE_COMPARE(out,
        "Ui::TextLayer::Shared::addInstancelessFont(): glyph cache font 1 has an instance set\n");
}

void TextLayerTest::sharedFontInvalidHandle() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return {}; }
    } font;

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};
    cache.addFont(67, &font);

    struct Shared: TextLayer::Shared {
        explicit Shared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{3, 5}};

    /* Need to add at least one font because the assertion returns the first
       font as a fallback */
    shared.addFont(font, 13.0f);

    Containers::String out;
    Error redirectError{&out};
    shared.glyphCacheFontId(FontHandle(0x12ab));
    shared.glyphCacheFontId(FontHandle::Null);
    shared.hasFontInstance(FontHandle(0x12ab));
    shared.hasFontInstance(FontHandle::Null);
    shared.font(FontHandle(0x12ab));
    shared.font(FontHandle::Null);
    /* Const overload */
    const_cast<const Shared&>(shared).font(FontHandle(0x12ab));
    const_cast<const Shared&>(shared).font(FontHandle::Null);
    CORRADE_COMPARE(out,
        "Ui::TextLayer::Shared::glyphCacheFontId(): invalid handle Ui::FontHandle(0x12ab, 0x0)\n"
        "Ui::TextLayer::Shared::glyphCacheFontId(): invalid handle Ui::FontHandle::Null\n"
        "Ui::TextLayer::Shared::hasFontInstance(): invalid handle Ui::FontHandle(0x12ab, 0x0)\n"
        "Ui::TextLayer::Shared::hasFontInstance(): invalid handle Ui::FontHandle::Null\n"
        "Ui::TextLayer::Shared::font(): invalid handle Ui::FontHandle(0x12ab, 0x0)\n"
        "Ui::TextLayer::Shared::font(): invalid handle Ui::FontHandle::Null\n"
        "Ui::TextLayer::Shared::font(): invalid handle Ui::FontHandle(0x12ab, 0x0)\n"
        "Ui::TextLayer::Shared::font(): invalid handle Ui::FontHandle::Null\n");
}

void TextLayerTest::sharedFontNoInstance() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return {}; }
    } font;

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};
    cache.addFont(67, &font);

    UnsignedInt glyphCacheInstanceLessFontId = cache.addFont(233);

    struct Shared: TextLayer::Shared {
        explicit Shared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{3, 5}};

    /* Need to add at least one font with an instance because the assertion
       returns the first font as a fallback */
    shared.addFont(font, 13.0f);

    FontHandle instanceless = shared.addInstancelessFont(glyphCacheInstanceLessFontId, 0.3f);
    CORRADE_VERIFY(!shared.hasFontInstance(instanceless));

    Containers::String out;
    Error redirectError{&out};
    shared.font(instanceless);
    CORRADE_COMPARE(out, "Ui::TextLayer::Shared::font(): Ui::FontHandle(0x1, 0x1) is an instance-less font\n");
}

void TextLayerTest::sharedSetStyle() {
    auto&& data = SharedSetStyleData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};

    struct Font: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return {}; }
    };

    struct Shared: TextLayer::Shared {
        explicit Shared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        State& state() { return static_cast<State&>(*_state); }

        void doSetStyle(const TextLayerCommonStyleUniform& commonUniform, Containers::ArrayView<const TextLayerStyleUniform> uniforms) override {
            CORRADE_COMPARE(commonUniform.smoothness, 3.14f);
            CORRADE_COMPARE(uniforms.size(), 3);
            CORRADE_COMPARE(uniforms[1].color, 0xc0ffee_rgbf);
            ++setStyleCalled;
        }
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {
            CORRADE_FAIL("This shouldn't be called.");
        }

        Int setStyleCalled = 0;
    } shared{cache, TextLayer::Shared::Configuration{3, 5}
        .setEditingStyleCount(1, 2)
        .setDynamicStyleCount(data.dynamicStyleCount)
    };

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Font font1, font2;
    cache.addFont(67, &font1);
    cache.addFont(23, &font2);
    FontHandle first = shared.addFont(font1, 13.0f);
    FontHandle second = shared.addFont(font2, 6.0f);
    shared.setStyle(
        TextLayerCommonStyleUniform{}
            .setSmoothness(3.14f),
        {TextLayerStyleUniform{},
         TextLayerStyleUniform{}
            .setColor(0xc0ffee_rgbf),
         TextLayerStyleUniform{}},
        {2, 1, 0, 0, 1},
        {first, second, first, second, second},
        {Text::Alignment::MiddleLeft,
         Text::Alignment::TopRight,
         Text::Alignment::BottomRight,
         Text::Alignment::LineLeft,
         Text::Alignment::LineCenterIntegral},
        {Text::Feature::SmallCapitals,
         {Text::Feature::Kerning, false},
         Text::Feature::SlashedZero},
        /* The last one is an empty range as well, just with a non-zero
           offset */
        {0, 1, 2, 0, 3},
        {1, 2, 1, 0, 0},
        {0, -1, 1, 0, 1},
        {-1, -1, -1, 0, 1},
        {{1.0f, 2.0f, 3.0f, 4.0f},
         {4.0f, 3.0f, 2.0f, 1.0f},
         {2.0f, 1.0f, 4.0f, 3.0f},
         {1.0f, 3.0f, 2.0f, 4.0f},
         {4.0f, 1.0f, 3.0f, 2.0f}});
    if(data.dynamicStyleCount == 0) {
        CORRADE_COMPARE(shared.setStyleCalled, 1);
    } else {
        CORRADE_COMPARE(shared.setStyleCalled, 0);
        /* If there are dynamic styles, it's copied into an internal array
           instead of calling doSetStyle(). The following is thus checking the
           same as doSetStyle() but on the internal array. */
        CORRADE_COMPARE(shared.state().commonStyleUniform.smoothness, 3.14f);
        CORRADE_COMPARE(shared.state().styleUniforms.size(), 3);
        CORRADE_COMPARE(shared.state().styleUniforms[1].color, 0xc0ffee_rgbf);
    }
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::uniform), Containers::stridedArrayView({
        2u, 1u, 0u, 0u, 1u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::font), Containers::stridedArrayView({
        first,
        second,
        first,
        second,
        second
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::alignment), Containers::stridedArrayView({
        Text::Alignment::MiddleLeft,
        Text::Alignment::TopRight,
        Text::Alignment::BottomRight,
        Text::Alignment::LineLeft,
        Text::Alignment::LineCenterIntegral
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS((Containers::arrayCast<const Containers::Pair<Text::Feature, UnsignedInt>>(shared.state().styleFeatures)), (Containers::stridedArrayView<Containers::Pair<Text::Feature, UnsignedInt>>({
        {Text::Feature::SmallCapitals, true},
        {Text::Feature::Kerning, false},
        {Text::Feature::SlashedZero, true},
    })), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::featureOffset), Containers::stridedArrayView({
        0u, 1u, 2u, 0u, 3u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::featureCount), Containers::stridedArrayView({
        1u, 2u, 1u, 0u, 0u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::cursorStyle), Containers::stridedArrayView({
        0, -1, 1, 0, 1
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::selectionStyle), Containers::stridedArrayView({
        -1, -1, -1, 0, 1
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::padding), Containers::stridedArrayView({
        Vector4{1.0f, 2.0f, 3.0f, 4.0f},
        Vector4{4.0f, 3.0f, 2.0f, 1.0f},
        Vector4{2.0f, 1.0f, 4.0f, 3.0f},
        Vector4{1.0f, 3.0f, 2.0f, 4.0f},
        Vector4{4.0f, 1.0f, 3.0f, 2.0f}
    }), TestSuite::Compare::Container);
}

void TextLayerTest::sharedSetStyleImplicitFeatures() {
    auto&& data = SharedSetStyleData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};

    struct Font: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return {}; }
    };

    struct Shared: TextLayer::Shared {
        explicit Shared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        State& state() { return static_cast<State&>(*_state); }

        void doSetStyle(const TextLayerCommonStyleUniform& commonUniform, Containers::ArrayView<const TextLayerStyleUniform> uniforms) override {
            CORRADE_COMPARE(commonUniform.smoothness, 3.14f);
            CORRADE_COMPARE(uniforms.size(), 3);
            CORRADE_COMPARE(uniforms[1].color, 0xc0ffee_rgbf);
            ++setStyleCalled;
        }
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {
            CORRADE_FAIL("This shouldn't be called.");
        }

        Int setStyleCalled = 0;
    } shared{cache, TextLayer::Shared::Configuration{3, 5}
        .setEditingStyleCount(1, 2)
        .setDynamicStyleCount(data.dynamicStyleCount)
    };

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Font font1, font2;
    cache.addFont(67, &font1);
    cache.addFont(23, &font2);
    FontHandle first = shared.addFont(font1, 13.0f);
    FontHandle second = shared.addFont(font2, 6.0f);
    shared.setStyle(
        TextLayerCommonStyleUniform{}
            .setSmoothness(3.14f),
        {TextLayerStyleUniform{},
         TextLayerStyleUniform{}
            .setColor(0xc0ffee_rgbf),
         TextLayerStyleUniform{}},
        {2, 1, 0, 0, 1},
        {first, second, first, second, second},
        {Text::Alignment::MiddleLeft,
         Text::Alignment::TopRight,
         Text::Alignment::BottomRight,
         Text::Alignment::LineLeft,
         Text::Alignment::LineCenterIntegral},
        {}, {}, {},
        {0, -1, 1, 0, 1},
        {-1, -1, -1, 0, 1},
        {{1.0f, 2.0f, 3.0f, 4.0f},
         {4.0f, 3.0f, 2.0f, 1.0f},
         {2.0f, 1.0f, 4.0f, 3.0f},
         {1.0f, 3.0f, 2.0f, 4.0f},
         {4.0f, 1.0f, 3.0f, 2.0f}});
    if(data.dynamicStyleCount == 0) {
        CORRADE_COMPARE(shared.setStyleCalled, 1);
    } else {
        CORRADE_COMPARE(shared.setStyleCalled, 0);
        /* If there are dynamic styles, it's copied into an internal array
           instead of calling doSetStyle(). The following is thus checking the
           same as doSetStyle() but on the internal array. */
        CORRADE_COMPARE(shared.state().commonStyleUniform.smoothness, 3.14f);
        CORRADE_COMPARE(shared.state().styleUniforms.size(), 3);
        CORRADE_COMPARE(shared.state().styleUniforms[1].color, 0xc0ffee_rgbf);
    }
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::uniform), Containers::stridedArrayView({
        2u, 1u, 0u, 0u, 1u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::font), Containers::stridedArrayView({
        first,
        second,
        first,
        second,
        second
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::alignment), Containers::stridedArrayView({
        Text::Alignment::MiddleLeft,
        Text::Alignment::TopRight,
        Text::Alignment::BottomRight,
        Text::Alignment::LineLeft,
        Text::Alignment::LineCenterIntegral
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS((Containers::arrayCast<const Containers::Pair<Text::Feature, UnsignedInt>>(shared.state().styleFeatures)), (Containers::arrayView<Containers::Pair<Text::Feature, UnsignedInt>>({
        /* Nothing */
    })), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::featureOffset), Containers::stridedArrayView({
        0u, 0u, 0u, 0u, 0u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::featureCount), Containers::stridedArrayView({
        0u, 0u, 0u, 0u, 0u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::cursorStyle), Containers::stridedArrayView({
        0, -1, 1, 0, 1
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::selectionStyle), Containers::stridedArrayView({
        -1, -1, -1, 0, 1
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::padding), Containers::stridedArrayView({
        Vector4{1.0f, 2.0f, 3.0f, 4.0f},
        Vector4{4.0f, 3.0f, 2.0f, 1.0f},
        Vector4{2.0f, 1.0f, 4.0f, 3.0f},
        Vector4{1.0f, 3.0f, 2.0f, 4.0f},
        Vector4{4.0f, 1.0f, 3.0f, 2.0f}
    }), TestSuite::Compare::Container);

    /* Setting a style with implicit features after non-implicit features were
       set should reset them back to nothing */
    shared.setStyle(
        TextLayerCommonStyleUniform{}
            .setSmoothness(3.14f),
        {TextLayerStyleUniform{},
         TextLayerStyleUniform{}
            .setColor(0xc0ffee_rgbf),
         TextLayerStyleUniform{}},
        {2, 1, 0, 0, 1},
        {first, second, first, second, second},
        {Text::Alignment::MiddleLeft,
         Text::Alignment::TopRight,
         Text::Alignment::BottomRight,
         Text::Alignment::LineLeft,
         Text::Alignment::LineCenterIntegral},
        {Text::Feature::SmallCapitals,
         {Text::Feature::Kerning, false},
         Text::Feature::SlashedZero},
        {0, 1, 2, 0, 3},
        {1, 2, 1, 0, 0},
        {0, -1, 1, 0, 1},
        {-1, -1, -1, 0, 1},
        {{1.0f, 2.0f, 3.0f, 4.0f},
         {4.0f, 3.0f, 2.0f, 1.0f},
         {2.0f, 1.0f, 4.0f, 3.0f},
         {1.0f, 3.0f, 2.0f, 4.0f},
         {4.0f, 1.0f, 3.0f, 2.0f}});
    shared.setStyle(
        TextLayerCommonStyleUniform{}
            .setSmoothness(3.14f),
        {TextLayerStyleUniform{},
         TextLayerStyleUniform{}
            .setColor(0xc0ffee_rgbf),
         TextLayerStyleUniform{}},
        {2, 1, 0, 0, 1},
        {first, second, first, second, second},
        {Text::Alignment::MiddleLeft,
         Text::Alignment::TopRight,
         Text::Alignment::BottomRight,
         Text::Alignment::LineLeft,
         Text::Alignment::LineCenterIntegral},
        {}, {}, {},
        {0, -1, 1, 0, 1},
        {-1, -1, -1, 0, 1},
        {{1.0f, 2.0f, 3.0f, 4.0f},
         {4.0f, 3.0f, 2.0f, 1.0f},
         {2.0f, 1.0f, 4.0f, 3.0f},
         {1.0f, 3.0f, 2.0f, 4.0f},
         {4.0f, 1.0f, 3.0f, 2.0f}});
    CORRADE_COMPARE_AS((Containers::arrayCast<const Containers::Pair<Text::Feature, UnsignedInt>>(shared.state().styleFeatures)), (Containers::arrayView<Containers::Pair<Text::Feature, UnsignedInt>>({
        /* Nothing */
    })), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::featureOffset), Containers::stridedArrayView({
        0u, 0u, 0u, 0u, 0u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::featureCount), Containers::stridedArrayView({
        0u, 0u, 0u, 0u, 0u
    }), TestSuite::Compare::Container);
}

void TextLayerTest::sharedSetStyleImplicitEditingStyles() {
    auto&& data = SharedSetStyleData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};

    struct Font: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return {}; }
    };

    struct Shared: TextLayer::Shared {
        explicit Shared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        State& state() { return static_cast<State&>(*_state); }

        void doSetStyle(const TextLayerCommonStyleUniform& commonUniform, Containers::ArrayView<const TextLayerStyleUniform> uniforms) override {
            CORRADE_COMPARE(commonUniform.smoothness, 3.14f);
            CORRADE_COMPARE(uniforms.size(), 3);
            CORRADE_COMPARE(uniforms[1].color, 0xc0ffee_rgbf);
            ++setStyleCalled;
        }
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {
            CORRADE_FAIL("This shouldn't be called.");
        }

        Int setStyleCalled = 0;
    } shared{cache, TextLayer::Shared::Configuration{3, 5}
        .setEditingStyleCount(1, 2)
        .setDynamicStyleCount(data.dynamicStyleCount)
    };

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Font font1, font2;
    cache.addFont(67, &font1);
    cache.addFont(23, &font2);
    FontHandle first = shared.addFont(font1, 13.0f);
    FontHandle second = shared.addFont(font2, 6.0f);
    shared.setStyle(
        TextLayerCommonStyleUniform{}
            .setSmoothness(3.14f),
        {TextLayerStyleUniform{},
         TextLayerStyleUniform{}
            .setColor(0xc0ffee_rgbf),
         TextLayerStyleUniform{}},
        {2, 1, 0, 0, 1},
        {first, second, first, second, second},
        {Text::Alignment::MiddleLeft,
         Text::Alignment::TopRight,
         Text::Alignment::BottomRight,
         Text::Alignment::LineLeft,
         Text::Alignment::LineCenterIntegral},
        {Text::Feature::SmallCapitals,
         {Text::Feature::Kerning, false},
         Text::Feature::SlashedZero},
        {0, 1, 2, 0, 3},
        {1, 2, 1, 0, 0},
        {},
        {},
        {{1.0f, 2.0f, 3.0f, 4.0f},
         {4.0f, 3.0f, 2.0f, 1.0f},
         {2.0f, 1.0f, 4.0f, 3.0f},
         {1.0f, 3.0f, 2.0f, 4.0f},
         {4.0f, 1.0f, 3.0f, 2.0f}});
    if(data.dynamicStyleCount == 0) {
        CORRADE_COMPARE(shared.setStyleCalled, 1);
    } else {
        CORRADE_COMPARE(shared.setStyleCalled, 0);
        /* If there are dynamic styles, it's copied into an internal array
           instead of calling doSetStyle(). The following is thus checking the
           same as doSetStyle() but on the internal array. */
        CORRADE_COMPARE(shared.state().commonStyleUniform.smoothness, 3.14f);
        CORRADE_COMPARE(shared.state().styleUniforms.size(), 3);
        CORRADE_COMPARE(shared.state().styleUniforms[1].color, 0xc0ffee_rgbf);
    }
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::uniform), Containers::stridedArrayView({
        2u, 1u, 0u, 0u, 1u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::font), Containers::stridedArrayView({
        first,
        second,
        first,
        second,
        second
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::alignment), Containers::stridedArrayView({
        Text::Alignment::MiddleLeft,
        Text::Alignment::TopRight,
        Text::Alignment::BottomRight,
        Text::Alignment::LineLeft,
        Text::Alignment::LineCenterIntegral
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS((Containers::arrayCast<const Containers::Pair<Text::Feature, UnsignedInt>>(shared.state().styleFeatures)), (Containers::stridedArrayView<Containers::Pair<Text::Feature, UnsignedInt>>({
        {Text::Feature::SmallCapitals, true},
        {Text::Feature::Kerning, false},
        {Text::Feature::SlashedZero, true},
    })), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::featureOffset), Containers::stridedArrayView({
        0u, 1u, 2u, 0u, 3u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::featureCount), Containers::stridedArrayView({
        1u, 2u, 1u, 0u, 0u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::cursorStyle), Containers::stridedArrayView({
        -1, -1, -1, -1, -1
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::selectionStyle), Containers::stridedArrayView({
        -1, -1, -1, -1, -1
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::padding), Containers::stridedArrayView({
        Vector4{1.0f, 2.0f, 3.0f, 4.0f},
        Vector4{4.0f, 3.0f, 2.0f, 1.0f},
        Vector4{2.0f, 1.0f, 4.0f, 3.0f},
        Vector4{1.0f, 3.0f, 2.0f, 4.0f},
        Vector4{4.0f, 1.0f, 3.0f, 2.0f}
    }), TestSuite::Compare::Container);

    /* Setting a style with implicit editing styles after non-implicit editing
       styles were set should reset them back to nothing ... */
    shared.setStyle(
        TextLayerCommonStyleUniform{}
            .setSmoothness(3.14f),
        {TextLayerStyleUniform{},
         TextLayerStyleUniform{}
            .setColor(0xc0ffee_rgbf),
         TextLayerStyleUniform{}},
        {2, 1, 0, 0, 1},
        {first, second, first, second, second},
        {Text::Alignment::MiddleLeft,
         Text::Alignment::TopRight,
         Text::Alignment::BottomRight,
         Text::Alignment::LineLeft,
         Text::Alignment::LineCenterIntegral},
        {Text::Feature::SmallCapitals,
         {Text::Feature::Kerning, false},
         Text::Feature::SlashedZero},
        /* The last one is an empty range as well, just with a non-zero
           offset */
        {0, 1, 2, 0, 3},
        {1, 2, 1, 0, 0},
        {0, -1, 1, 0, 1},
        {},
        {{1.0f, 2.0f, 3.0f, 4.0f},
         {4.0f, 3.0f, 2.0f, 1.0f},
         {2.0f, 1.0f, 4.0f, 3.0f},
         {1.0f, 3.0f, 2.0f, 4.0f},
         {4.0f, 1.0f, 3.0f, 2.0f}});
    shared.setStyle(
        TextLayerCommonStyleUniform{}
            .setSmoothness(3.14f),
        {TextLayerStyleUniform{},
         TextLayerStyleUniform{}
            .setColor(0xc0ffee_rgbf),
         TextLayerStyleUniform{}},
        {2, 1, 0, 0, 1},
        {first, second, first, second, second},
        {Text::Alignment::MiddleLeft,
         Text::Alignment::TopRight,
         Text::Alignment::BottomRight,
         Text::Alignment::LineLeft,
         Text::Alignment::LineCenterIntegral},
        {Text::Feature::SmallCapitals,
         {Text::Feature::Kerning, false},
         Text::Feature::SlashedZero},
        {0, 1, 2, 0, 3},
        {1, 2, 1, 0, 0},
        {},
        {-1, -1, -1, 0, 1},
        {{1.0f, 2.0f, 3.0f, 4.0f},
         {4.0f, 3.0f, 2.0f, 1.0f},
         {2.0f, 1.0f, 4.0f, 3.0f},
         {1.0f, 3.0f, 2.0f, 4.0f},
         {4.0f, 1.0f, 3.0f, 2.0f}});
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::cursorStyle), Containers::stridedArrayView({
        -1, -1, -1, -1, -1
    }), TestSuite::Compare::Container);

    /* ... independently for cursor and selection styles */
    shared.setStyle(
        TextLayerCommonStyleUniform{}
            .setSmoothness(3.14f),
        {TextLayerStyleUniform{},
         TextLayerStyleUniform{}
            .setColor(0xc0ffee_rgbf),
         TextLayerStyleUniform{}},
        {2, 1, 0, 0, 1},
        {first, second, first, second, second},
        {Text::Alignment::MiddleLeft,
         Text::Alignment::TopRight,
         Text::Alignment::BottomRight,
         Text::Alignment::LineLeft,
         Text::Alignment::LineCenterIntegral},
        {Text::Feature::SmallCapitals,
         {Text::Feature::Kerning, false},
         Text::Feature::SlashedZero},
        /* The last one is an empty range as well, just with a non-zero
           offset */
        {0, 1, 2, 0, 3},
        {1, 2, 1, 0, 0},
        {0, -1, 1, 0, 1},
        {},
        {{1.0f, 2.0f, 3.0f, 4.0f},
         {4.0f, 3.0f, 2.0f, 1.0f},
         {2.0f, 1.0f, 4.0f, 3.0f},
         {1.0f, 3.0f, 2.0f, 4.0f},
         {4.0f, 1.0f, 3.0f, 2.0f}});
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::selectionStyle), Containers::stridedArrayView({
        -1, -1, -1, -1, -1
    }), TestSuite::Compare::Container);
}

void TextLayerTest::sharedSetStyleImplicitPadding() {
    auto&& data = SharedSetStyleData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};

    struct Font: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return {}; }
    };

    struct Shared: TextLayer::Shared {
        explicit Shared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        State& state() { return static_cast<State&>(*_state); }

        void doSetStyle(const TextLayerCommonStyleUniform& commonUniform, Containers::ArrayView<const TextLayerStyleUniform> uniforms) override {
            CORRADE_COMPARE(commonUniform.smoothness, 3.14f);
            CORRADE_COMPARE(uniforms.size(), 3);
            CORRADE_COMPARE(uniforms[1].color, 0xc0ffee_rgbf);
            ++setStyleCalled;
        }
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {
            CORRADE_FAIL("This shouldn't be called.");
        }

        Int setStyleCalled = 0;
    } shared{cache, TextLayer::Shared::Configuration{3, 5}
        .setEditingStyleCount(1, 2)
        .setDynamicStyleCount(data.dynamicStyleCount)
    };

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Font font1, font2;
    cache.addFont(67, &font1);
    cache.addFont(23, &font2);
    FontHandle first = shared.addFont(font1, 13.0f);
    FontHandle second = shared.addFont(font2, 6.0f);
    shared.setStyle(
        TextLayerCommonStyleUniform{}
            .setSmoothness(3.14f),
        {TextLayerStyleUniform{},
         TextLayerStyleUniform{}
            .setColor(0xc0ffee_rgbf),
         TextLayerStyleUniform{}},
        {2, 1, 0, 0, 1},
        {first, second, first, second, second},
        {Text::Alignment::MiddleLeft,
         Text::Alignment::TopRight,
         Text::Alignment::BottomRight,
         Text::Alignment::LineLeft,
         Text::Alignment::LineCenterIntegral},
        {Text::Feature::SmallCapitals,
         {Text::Feature::Kerning, false},
         Text::Feature::SlashedZero},
        {0, 1, 2, 0, 3},
        {1, 2, 1, 0, 0},
        {0, -1, 1, 0, 1},
        {-1, -1, -1, 0, 1},
        {});
    if(data.dynamicStyleCount == 0) {
        CORRADE_COMPARE(shared.setStyleCalled, 1);
    } else {
        CORRADE_COMPARE(shared.setStyleCalled, 0);
        /* If there are dynamic styles, it's copied into an internal array
           instead of calling doSetStyle(). The following is thus checking the
           same as doSetStyle() but on the internal array. */
        CORRADE_COMPARE(shared.state().commonStyleUniform.smoothness, 3.14f);
        CORRADE_COMPARE(shared.state().styleUniforms.size(), 3);
        CORRADE_COMPARE(shared.state().styleUniforms[1].color, 0xc0ffee_rgbf);
    }
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::uniform), Containers::stridedArrayView({
        2u, 1u, 0u, 0u, 1u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::font), Containers::stridedArrayView({
        first,
        second,
        first,
        second,
        second
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::alignment), Containers::stridedArrayView({
        Text::Alignment::MiddleLeft,
        Text::Alignment::TopRight,
        Text::Alignment::BottomRight,
        Text::Alignment::LineLeft,
        Text::Alignment::LineCenterIntegral
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS((Containers::arrayCast<const Containers::Pair<Text::Feature, UnsignedInt>>(shared.state().styleFeatures)), (Containers::stridedArrayView<Containers::Pair<Text::Feature, UnsignedInt>>({
        {Text::Feature::SmallCapitals, true},
        {Text::Feature::Kerning, false},
        {Text::Feature::SlashedZero, true},
    })), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::featureOffset), Containers::stridedArrayView({
        0u, 1u, 2u, 0u, 3u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::featureCount), Containers::stridedArrayView({
        1u, 2u, 1u, 0u, 0u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::cursorStyle), Containers::stridedArrayView({
        0, -1, 1, 0, 1
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::selectionStyle), Containers::stridedArrayView({
        -1, -1, -1, 0, 1
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::padding), Containers::stridedArrayView({
        Vector4{},
        Vector4{},
        Vector4{},
        Vector4{},
        Vector4{}
    }), TestSuite::Compare::Container);

    /* Setting a style with implicit padding after a non-implicit padding was
       set should reset it back to zeros */
    shared.setStyle(
        TextLayerCommonStyleUniform{}
            .setSmoothness(3.14f),
        {TextLayerStyleUniform{},
         TextLayerStyleUniform{}
            .setColor(0xc0ffee_rgbf),
         TextLayerStyleUniform{}},
        {2, 1, 0, 0, 1},
        {first, second, first, second, second},
        {Text::Alignment::MiddleLeft,
         Text::Alignment::TopRight,
         Text::Alignment::BottomRight,
         Text::Alignment::LineLeft,
         Text::Alignment::LineCenterIntegral},
        {Text::Feature::SmallCapitals,
         {Text::Feature::Kerning, false},
         Text::Feature::SlashedZero},
        {0, 1, 2, 0, 3},
        {1, 2, 1, 0, 0},
        {0, -1, 1, 0, 1},
        {-1, -1, -1, 0, 1},
        {{1.0f, 2.0f, 3.0f, 4.0f},
         {4.0f, 3.0f, 2.0f, 1.0f},
         {2.0f, 1.0f, 4.0f, 3.0f},
         {1.0f, 3.0f, 2.0f, 4.0f},
         {4.0f, 1.0f, 3.0f, 2.0f}});
    shared.setStyle(
        TextLayerCommonStyleUniform{}
            .setSmoothness(3.14f),
        {TextLayerStyleUniform{},
         TextLayerStyleUniform{}
            .setColor(0xc0ffee_rgbf),
         TextLayerStyleUniform{}},
        {2, 1, 0, 0, 1},
        {first, second, first, second, second},
        {Text::Alignment::MiddleLeft,
         Text::Alignment::TopRight,
         Text::Alignment::BottomRight,
         Text::Alignment::LineLeft,
         Text::Alignment::LineCenterIntegral},
        {Text::Feature::SmallCapitals,
         {Text::Feature::Kerning, false},
         Text::Feature::SlashedZero},
        {0, 1, 2, 0, 3},
        {1, 2, 1, 0, 0},
        {0, -1, 1, 0, 1},
        {-1, -1, -1, 0, 1},
        {});
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::padding), Containers::stridedArrayView({
        Vector4{},
        Vector4{},
        Vector4{},
        Vector4{},
        Vector4{}
    }), TestSuite::Compare::Container);
}

void TextLayerTest::sharedSetStyleInvalidSize() {
    auto&& data = SharedSetStyleData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32}};

    struct Shared: TextLayer::Shared {
        explicit Shared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{3, 5}
        .setEditingStyleCount(1, 2)
        /* The checks should all deal with just the shared style count, not be
           dependent on this */
        .setDynamicStyleCount(data.dynamicStyleCount)
    };

    Containers::String out;
    Error redirectError{&out};
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}, TextLayerStyleUniform{}},
        {0, 1, 2, 1, 0},
        {FontHandle::Null, FontHandle::Null, FontHandle::Null, FontHandle::Null, FontHandle::Null},
        {Text::Alignment{}, Text::Alignment{}, Text::Alignment{}, Text::Alignment{}, Text::Alignment{}},
        {},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {-1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1},
        {{}, {}, {}, {}, {}});
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}, TextLayerStyleUniform{}, TextLayerStyleUniform{}},
        {0, 1, 2},
        {FontHandle::Null, FontHandle::Null, FontHandle::Null, FontHandle::Null, FontHandle::Null},
        {Text::Alignment{}, Text::Alignment{}, Text::Alignment{}, Text::Alignment{}, Text::Alignment{}},
        {},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {-1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1},
        {{}, {}, {}, {}, {}});
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}, TextLayerStyleUniform{}, TextLayerStyleUniform{}},
        {0, 1, 2, 1, 0},
        {FontHandle::Null, FontHandle::Null, FontHandle::Null},
        {Text::Alignment{}, Text::Alignment{}, Text::Alignment{}, Text::Alignment{}, Text::Alignment{}},
        {},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {-1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1},
        {{}, {}, {}, {}, {}});
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}, TextLayerStyleUniform{}, TextLayerStyleUniform{}},
        {0, 1, 2, 1, 0},
        {FontHandle::Null, FontHandle::Null, FontHandle::Null, FontHandle::Null, FontHandle::Null},
        {Text::Alignment{}, Text::Alignment{}, Text::Alignment{}},
        {},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {-1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1},
        {{}, {}, {}, {}, {}});
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}, TextLayerStyleUniform{}, TextLayerStyleUniform{}},
        {0, 1, 2, 1, 0},
        {FontHandle::Null, FontHandle::Null, FontHandle::Null, FontHandle::Null, FontHandle::Null},
        {Text::Alignment{}, Text::Alignment{}, Text::Alignment{}, Text::Alignment{}, Text::Alignment{}},
        {},
        {0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {-1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1},
        {{}, {}, {}, {}, {}});
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}, TextLayerStyleUniform{}, TextLayerStyleUniform{}},
        {0, 1, 2, 1, 0},
        {FontHandle::Null, FontHandle::Null, FontHandle::Null, FontHandle::Null, FontHandle::Null},
        {Text::Alignment{}, Text::Alignment{}, Text::Alignment{}, Text::Alignment{}, Text::Alignment{}},
        {},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0},
        {-1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1},
        {{}, {}, {}, {}, {}});
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}, TextLayerStyleUniform{}, TextLayerStyleUniform{}},
        {0, 1, 2, 1, 0},
        {FontHandle::Null, FontHandle::Null, FontHandle::Null, FontHandle::Null, FontHandle::Null},
        {Text::Alignment{}, Text::Alignment{}, Text::Alignment{}, Text::Alignment{}, Text::Alignment{}},
        {Text::Feature::SlashedZero},
        {},
        {},
        {-1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1},
        {{}, {}, {}, {}, {}});
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}, TextLayerStyleUniform{}, TextLayerStyleUniform{}},
        {0, 1, 2, 1, 0},
        {FontHandle::Null, FontHandle::Null, FontHandle::Null, FontHandle::Null, FontHandle::Null},
        {Text::Alignment{}, Text::Alignment{}, Text::Alignment{}, Text::Alignment{}, Text::Alignment{}},
        {},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {-1, -1, -1, -1},
        {-1, -1, -1, -1, -1},
        {{}, {}, {}, {}, {}});
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}, TextLayerStyleUniform{}, TextLayerStyleUniform{}},
        {0, 1, 2, 1, 0},
        {FontHandle::Null, FontHandle::Null, FontHandle::Null, FontHandle::Null, FontHandle::Null},
        {Text::Alignment{}, Text::Alignment{}, Text::Alignment{}, Text::Alignment{}, Text::Alignment{}},
        {},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {-1, -1, -1, -1, -1},
        {-1, -1, -1, -1},
        {{}, {}, {}, {}, {}});
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}, TextLayerStyleUniform{}, TextLayerStyleUniform{}},
        {0, 1, 2, 1, 0},
        {FontHandle::Null, FontHandle::Null, FontHandle::Null, FontHandle::Null, FontHandle::Null},
        {Text::Alignment{}, Text::Alignment{}, Text::Alignment{}, Text::Alignment{}, Text::Alignment{}},
        {},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {-1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1},
        {{}, {}, {}});
    CORRADE_COMPARE_AS(out,
        "Ui::TextLayer::Shared::setStyle(): expected 3 uniforms, got 2\n"
        "Ui::TextLayer::Shared::setStyle(): expected 5 style uniform indices, got 3\n"
        "Ui::TextLayer::Shared::setStyle(): expected 5 font handles, got 3\n"
        "Ui::TextLayer::Shared::setStyle(): expected 5 alignment values, got 3\n"
        "Ui::TextLayer::Shared::setStyle(): expected 5 feature offsets, got 4\n"
        "Ui::TextLayer::Shared::setStyle(): expected 5 feature counts, got 4\n"
        /* If the feature list is non-empty the offsets & counts are expected
           to be non-empty as well */
        "Ui::TextLayer::Shared::setStyle(): expected 5 feature offsets, got 0\n"
        "Ui::TextLayer::Shared::setStyle(): expected either no or 5 cursor styles, got 4\n"
        "Ui::TextLayer::Shared::setStyle(): expected either no or 5 selection styles, got 4\n"
        "Ui::TextLayer::Shared::setStyle(): expected either no or 5 paddings, got 3\n",
        TestSuite::Compare::String);
}

void TextLayerTest::sharedSetStyleInvalidMapping() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32}};

    struct Shared: TextLayer::Shared {
        explicit Shared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{3, 6}};

    Containers::String out;
    Error redirectError{&out};
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}, TextLayerStyleUniform{}, TextLayerStyleUniform{}},
        {0, 1, 2, 1, 3, 2},
        {FontHandle::Null, FontHandle::Null, FontHandle::Null,
         FontHandle::Null, FontHandle::Null, FontHandle::Null},
        {Text::Alignment{}, Text::Alignment{}, Text::Alignment{},
         Text::Alignment{}, Text::Alignment{}, Text::Alignment{}},
        {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE_AS(out,
        "Ui::TextLayer::Shared::setStyle(): uniform index 3 out of range for 3 uniforms at index 4\n",
        TestSuite::Compare::String);
}

void TextLayerTest::sharedSetStyleImplicitMapping() {
    auto&& data = SharedSetStyleData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};

    struct Font: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return {}; }
    };

    struct Shared: TextLayer::Shared {
        explicit Shared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        State& state() { return static_cast<State&>(*_state); }

        void doSetStyle(const TextLayerCommonStyleUniform& commonUniform, Containers::ArrayView<const TextLayerStyleUniform> uniforms) override {
            CORRADE_COMPARE(commonUniform.smoothness, 3.14f);
            CORRADE_COMPARE(uniforms.size(), 3);
            CORRADE_COMPARE(uniforms[1].color, 0xc0ffee_rgbf);
            ++setStyleCalled;
        }
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {
            CORRADE_FAIL("This shouldn't be called.");
        }

        Int setStyleCalled = 0;
    } shared{cache, TextLayer::Shared::Configuration{3}
        .setEditingStyleCount(1, 2)
        .setDynamicStyleCount(data.dynamicStyleCount)
    };

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Font font1, font2;
    cache.addFont(67, &font1);
    cache.addFont(23, &font2);
    FontHandle first = shared.addFont(font1, 13.0f);
    FontHandle second = shared.addFont(font2, 6.0f);
    shared.setStyle(
        TextLayerCommonStyleUniform{}
            .setSmoothness(3.14f),
        {TextLayerStyleUniform{},
         TextLayerStyleUniform{}
            .setColor(0xc0ffee_rgbf),
         TextLayerStyleUniform{}},
        {first, second, first},
        {Text::Alignment::MiddleLeft,
         Text::Alignment::TopRight,
         Text::Alignment::LineCenterIntegral},
        {Text::Feature::SmallCapitals,
         {Text::Feature::Kerning, false},
         Text::Feature::SlashedZero},
        {0, 1, 2},
        {1, 2, 1},
        {1, -1, 0},
        {-1, 0, 1},
        {{1.0f, 2.0f, 3.0f, 4.0f},
         {4.0f, 3.0f, 2.0f, 1.0f},
         {2.0f, 1.0f, 4.0f, 3.0f}});
    if(data.dynamicStyleCount == 0) {
        CORRADE_COMPARE(shared.setStyleCalled, 1);
    } else {
        CORRADE_COMPARE(shared.setStyleCalled, 0);
        /* If there are dynamic styles, it's copied into an internal array
           instead of calling doSetStyle(). The following is thus checking the
           same as doSetStyle() but on the internal array. */
        CORRADE_COMPARE(shared.state().commonStyleUniform.smoothness, 3.14f);
        CORRADE_COMPARE(shared.state().styleUniforms.size(), 3);
        CORRADE_COMPARE(shared.state().styleUniforms[1].color, 0xc0ffee_rgbf);
    }
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::uniform), Containers::stridedArrayView({
        0u, 1u, 2u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::font), Containers::stridedArrayView({
        first,
        second,
        first
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::alignment), Containers::stridedArrayView({
        Text::Alignment::MiddleLeft,
        Text::Alignment::TopRight,
        Text::Alignment::LineCenterIntegral
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS((Containers::arrayCast<const Containers::Pair<Text::Feature, UnsignedInt>>(shared.state().styleFeatures)), (Containers::stridedArrayView<Containers::Pair<Text::Feature, UnsignedInt>>({
        {Text::Feature::SmallCapitals, true},
        {Text::Feature::Kerning, false},
        {Text::Feature::SlashedZero, true},
    })), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::featureOffset), Containers::stridedArrayView({
        0u, 1u, 2u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::featureCount), Containers::stridedArrayView({
        1u, 2u, 1u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::cursorStyle), Containers::stridedArrayView({
        1, -1, 0
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::selectionStyle), Containers::stridedArrayView({
        -1, 0, 1
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::padding), Containers::stridedArrayView({
        Vector4{1.0f, 2.0f, 3.0f, 4.0f},
        Vector4{4.0f, 3.0f, 2.0f, 1.0f},
        Vector4{2.0f, 1.0f, 4.0f, 3.0f}
    }), TestSuite::Compare::Container);
}

void TextLayerTest::sharedSetStyleImplicitMappingImplicitFeatures() {
    auto&& data = SharedSetStyleData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};

    struct Font: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return {}; }
    };

    struct Shared: TextLayer::Shared {
        explicit Shared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        State& state() { return static_cast<State&>(*_state); }

        void doSetStyle(const TextLayerCommonStyleUniform& commonUniform, Containers::ArrayView<const TextLayerStyleUniform> uniforms) override {
            CORRADE_COMPARE(commonUniform.smoothness, 3.14f);
            CORRADE_COMPARE(uniforms.size(), 3);
            CORRADE_COMPARE(uniforms[1].color, 0xc0ffee_rgbf);
            ++setStyleCalled;
        }
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {
            CORRADE_FAIL("This shouldn't be called.");
        }

        Int setStyleCalled = 0;
    } shared{cache, TextLayer::Shared::Configuration{3}
        .setEditingStyleCount(1, 2)
        .setDynamicStyleCount(data.dynamicStyleCount)
    };

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Font font1, font2;
    cache.addFont(67, &font1);
    cache.addFont(23, &font2);
    FontHandle first = shared.addFont(font1, 13.0f);
    FontHandle second = shared.addFont(font2, 6.0f);
    shared.setStyle(
        TextLayerCommonStyleUniform{}
            .setSmoothness(3.14f),
        {TextLayerStyleUniform{},
         TextLayerStyleUniform{}
            .setColor(0xc0ffee_rgbf),
         TextLayerStyleUniform{}},
        {first, second, first},
        {Text::Alignment::MiddleLeft,
         Text::Alignment::TopRight,
         Text::Alignment::LineCenterIntegral},
        {}, {}, {},
        {1, -1, 0},
        {-1, 0, 1},
        {{1.0f, 2.0f, 3.0f, 4.0f},
         {4.0f, 3.0f, 2.0f, 1.0f},
         {2.0f, 1.0f, 4.0f, 3.0f}});
    if(data.dynamicStyleCount == 0) {
        CORRADE_COMPARE(shared.setStyleCalled, 1);
    } else {
        CORRADE_COMPARE(shared.setStyleCalled, 0);
        /* If there are dynamic styles, it's copied into an internal array
           instead of calling doSetStyle(). The following is thus checking the
           same as doSetStyle() but on the internal array. */
        CORRADE_COMPARE(shared.state().commonStyleUniform.smoothness, 3.14f);
        CORRADE_COMPARE(shared.state().styleUniforms.size(), 3);
        CORRADE_COMPARE(shared.state().styleUniforms[1].color, 0xc0ffee_rgbf);
    }
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::uniform), Containers::stridedArrayView({
        0u, 1u, 2u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::font), Containers::stridedArrayView({
        first,
        second,
        first
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::alignment), Containers::stridedArrayView({
        Text::Alignment::MiddleLeft,
        Text::Alignment::TopRight,
        Text::Alignment::LineCenterIntegral
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS((Containers::arrayCast<const Containers::Pair<Text::Feature, UnsignedInt>>(shared.state().styleFeatures)), (Containers::arrayView<Containers::Pair<Text::Feature, UnsignedInt>>({
        /* Nothing */
    })), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::featureOffset), Containers::stridedArrayView({
        0u, 0u, 0u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::featureCount), Containers::stridedArrayView({
        0u, 0u, 0u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::cursorStyle), Containers::stridedArrayView({
        1, -1, 0
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::selectionStyle), Containers::stridedArrayView({
        -1, 0, 1
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::padding), Containers::stridedArrayView({
        Vector4{1.0f, 2.0f, 3.0f, 4.0f},
        Vector4{4.0f, 3.0f, 2.0f, 1.0f},
        Vector4{2.0f, 1.0f, 4.0f, 3.0f}
    }), TestSuite::Compare::Container);

    /* Setting a style with implicit features after non-implicit features were
       set should reset them back to nothing */
    shared.setStyle(
        TextLayerCommonStyleUniform{}
            .setSmoothness(3.14f),
        {TextLayerStyleUniform{},
         TextLayerStyleUniform{}
            .setColor(0xc0ffee_rgbf),
         TextLayerStyleUniform{}},
        {first, second, first},
        {Text::Alignment::MiddleLeft,
         Text::Alignment::TopRight,
         Text::Alignment::LineCenterIntegral},
        {Text::Feature::SmallCapitals,
         {Text::Feature::Kerning, false},
         Text::Feature::SlashedZero},
        {0, 1, 2},
        {1, 2, 1},
        {1, -1, 0},
        {-1, 0, 1},
        {{1.0f, 2.0f, 3.0f, 4.0f},
         {4.0f, 3.0f, 2.0f, 1.0f},
         {2.0f, 1.0f, 4.0f, 3.0f}});
    shared.setStyle(
        TextLayerCommonStyleUniform{}
            .setSmoothness(3.14f),
        {TextLayerStyleUniform{},
         TextLayerStyleUniform{}
            .setColor(0xc0ffee_rgbf),
         TextLayerStyleUniform{}},
        {first, second, first},
        {Text::Alignment::MiddleLeft,
         Text::Alignment::TopRight,
         Text::Alignment::LineCenterIntegral},
        {}, {}, {},
        {1, -1, 0},
        {-1, 0, 1},
        {{1.0f, 2.0f, 3.0f, 4.0f},
         {4.0f, 3.0f, 2.0f, 1.0f},
         {2.0f, 1.0f, 4.0f, 3.0f}});
    CORRADE_COMPARE_AS((Containers::arrayCast<const Containers::Pair<Text::Feature, UnsignedInt>>(shared.state().styleFeatures)), (Containers::arrayView<Containers::Pair<Text::Feature, UnsignedInt>>({
        /* Nothing */
    })), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::featureOffset), Containers::stridedArrayView({
        0u, 0u, 0u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::featureCount), Containers::stridedArrayView({
        0u, 0u, 0u
    }), TestSuite::Compare::Container);
}

void TextLayerTest::sharedSetStyleImplicitMappingImplicitEditingStyles() {
    auto&& data = SharedSetStyleData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};

    struct Font: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return {}; }
    };

    struct Shared: TextLayer::Shared {
        explicit Shared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        State& state() { return static_cast<State&>(*_state); }

        void doSetStyle(const TextLayerCommonStyleUniform& commonUniform, Containers::ArrayView<const TextLayerStyleUniform> uniforms) override {
            CORRADE_COMPARE(commonUniform.smoothness, 3.14f);
            CORRADE_COMPARE(uniforms.size(), 3);
            CORRADE_COMPARE(uniforms[1].color, 0xc0ffee_rgbf);
            ++setStyleCalled;
        }
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {
            CORRADE_FAIL("This shouldn't be called.");
        }

        Int setStyleCalled = 0;
    } shared{cache, TextLayer::Shared::Configuration{3}
        .setEditingStyleCount(1, 2)
        .setDynamicStyleCount(data.dynamicStyleCount)
    };

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Font font1, font2;
    cache.addFont(67, &font1);
    cache.addFont(23, &font2);
    FontHandle first = shared.addFont(font1, 13.0f);
    FontHandle second = shared.addFont(font2, 6.0f);
    shared.setStyle(
        TextLayerCommonStyleUniform{}
            .setSmoothness(3.14f),
        {TextLayerStyleUniform{},
         TextLayerStyleUniform{}
            .setColor(0xc0ffee_rgbf),
         TextLayerStyleUniform{}},
        {first, second, first},
        {Text::Alignment::MiddleLeft,
         Text::Alignment::TopRight,
         Text::Alignment::LineCenterIntegral},
        {Text::Feature::SmallCapitals,
         {Text::Feature::Kerning, false},
         Text::Feature::SlashedZero},
        {0, 1, 2},
        {1, 2, 1},
        {},
        {},
        {{1.0f, 2.0f, 3.0f, 4.0f},
         {4.0f, 3.0f, 2.0f, 1.0f},
         {2.0f, 1.0f, 4.0f, 3.0f}});
    if(data.dynamicStyleCount == 0) {
        CORRADE_COMPARE(shared.setStyleCalled, 1);
    } else {
        CORRADE_COMPARE(shared.setStyleCalled, 0);
        /* If there are dynamic styles, it's copied into an internal array
           instead of calling doSetStyle(). The following is thus checking the
           same as doSetStyle() but on the internal array. */
        CORRADE_COMPARE(shared.state().commonStyleUniform.smoothness, 3.14f);
        CORRADE_COMPARE(shared.state().styleUniforms.size(), 3);
        CORRADE_COMPARE(shared.state().styleUniforms[1].color, 0xc0ffee_rgbf);
    }
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::uniform), Containers::stridedArrayView({
        0u, 1u, 2u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::font), Containers::stridedArrayView({
        first,
        second,
        first
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::alignment), Containers::stridedArrayView({
        Text::Alignment::MiddleLeft,
        Text::Alignment::TopRight,
        Text::Alignment::LineCenterIntegral
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS((Containers::arrayCast<const Containers::Pair<Text::Feature, UnsignedInt>>(shared.state().styleFeatures)), (Containers::stridedArrayView<Containers::Pair<Text::Feature, UnsignedInt>>({
        {Text::Feature::SmallCapitals, true},
        {Text::Feature::Kerning, false},
        {Text::Feature::SlashedZero, true},
    })), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::featureOffset), Containers::stridedArrayView({
        0u, 1u, 2u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::featureCount), Containers::stridedArrayView({
        1u, 2u, 1u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::cursorStyle), Containers::stridedArrayView({
        -1, -1, -1
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::selectionStyle), Containers::stridedArrayView({
        -1, -1, -1
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::padding), Containers::stridedArrayView({
        Vector4{1.0f, 2.0f, 3.0f, 4.0f},
        Vector4{4.0f, 3.0f, 2.0f, 1.0f},
        Vector4{2.0f, 1.0f, 4.0f, 3.0f}
    }), TestSuite::Compare::Container);

    /* Setting a style with implicit editing styles after non-implicit editing
       styles were set should reset them back to nothing ... */
    shared.setStyle(
        TextLayerCommonStyleUniform{}
            .setSmoothness(3.14f),
        {TextLayerStyleUniform{},
         TextLayerStyleUniform{}
            .setColor(0xc0ffee_rgbf),
         TextLayerStyleUniform{}},
        {first, second, first},
        {Text::Alignment::MiddleLeft,
         Text::Alignment::TopRight,
         Text::Alignment::LineCenterIntegral},
        {Text::Feature::SmallCapitals,
         {Text::Feature::Kerning, false},
         Text::Feature::SlashedZero},
        {0, 1, 2},
        {1, 2, 1},
        {1, -1, 0},
        {},
        {{1.0f, 2.0f, 3.0f, 4.0f},
         {4.0f, 3.0f, 2.0f, 1.0f},
         {2.0f, 1.0f, 4.0f, 3.0f}});
    shared.setStyle(
        TextLayerCommonStyleUniform{}
            .setSmoothness(3.14f),
        {TextLayerStyleUniform{},
         TextLayerStyleUniform{}
            .setColor(0xc0ffee_rgbf),
         TextLayerStyleUniform{}},
        {first, second, first},
        {Text::Alignment::MiddleLeft,
         Text::Alignment::TopRight,
         Text::Alignment::LineCenterIntegral},
        {Text::Feature::SmallCapitals,
         {Text::Feature::Kerning, false},
         Text::Feature::SlashedZero},
        {0, 1, 2},
        {1, 2, 1},
        {},
        {-1, 0, 1},
        {{1.0f, 2.0f, 3.0f, 4.0f},
         {4.0f, 3.0f, 2.0f, 1.0f},
         {2.0f, 1.0f, 4.0f, 3.0f}});
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::cursorStyle), Containers::stridedArrayView({
        -1, -1, -1
    }), TestSuite::Compare::Container);

    /* ... independently for cursor and selection styles */
    shared.setStyle(
        TextLayerCommonStyleUniform{}
            .setSmoothness(3.14f),
        {TextLayerStyleUniform{},
         TextLayerStyleUniform{}
            .setColor(0xc0ffee_rgbf),
         TextLayerStyleUniform{}},
        {first, second, first},
        {Text::Alignment::MiddleLeft,
         Text::Alignment::TopRight,
         Text::Alignment::LineCenterIntegral},
        {Text::Feature::SmallCapitals,
         {Text::Feature::Kerning, false},
         Text::Feature::SlashedZero},
        {0, 1, 2},
        {1, 2, 1},
        {1, -1, 0},
        {},
        {{1.0f, 2.0f, 3.0f, 4.0f},
         {4.0f, 3.0f, 2.0f, 1.0f},
         {2.0f, 1.0f, 4.0f, 3.0f}});
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::selectionStyle), Containers::stridedArrayView({
        -1, -1, -1
    }), TestSuite::Compare::Container);
}

void TextLayerTest::sharedSetStyleImplicitMappingImplicitPadding() {
    auto&& data = SharedSetStyleData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};

    struct Font: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return {}; }
    };

    struct Shared: TextLayer::Shared {
        explicit Shared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        State& state() { return static_cast<State&>(*_state); }

        void doSetStyle(const TextLayerCommonStyleUniform& commonUniform, Containers::ArrayView<const TextLayerStyleUniform> uniforms) override {
            CORRADE_COMPARE(commonUniform.smoothness, 3.14f);
            CORRADE_COMPARE(uniforms.size(), 3);
            CORRADE_COMPARE(uniforms[1].color, 0xc0ffee_rgbf);
            ++setStyleCalled;
        }
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {
            CORRADE_FAIL("This shouldn't be called.");
        }

        Int setStyleCalled = 0;
    } shared{cache, TextLayer::Shared::Configuration{3}
        .setEditingStyleCount(1, 2)
        .setDynamicStyleCount(data.dynamicStyleCount)
    };

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    Font font1, font2;
    cache.addFont(67, &font1);
    cache.addFont(23, &font2);
    FontHandle first = shared.addFont(font1, 13.0f);
    FontHandle second = shared.addFont(font2, 6.0f);
    shared.setStyle(
        TextLayerCommonStyleUniform{}
            .setSmoothness(3.14f),
        {TextLayerStyleUniform{},
         TextLayerStyleUniform{}
            .setColor(0xc0ffee_rgbf),
         TextLayerStyleUniform{}},
        {first, second, first},
        {Text::Alignment::MiddleLeft,
         Text::Alignment::TopRight,
         Text::Alignment::LineCenterIntegral},
        {Text::Feature::SmallCapitals,
         {Text::Feature::Kerning, false},
         Text::Feature::SlashedZero},
        {0, 1, 2},
        {1, 2, 1},
        {1, -1, 0},
        {-1, 0, 1},
        {});
    if(data.dynamicStyleCount == 0) {
        CORRADE_COMPARE(shared.setStyleCalled, 1);
    } else {
        CORRADE_COMPARE(shared.setStyleCalled, 0);
        /* If there are dynamic styles, it's copied into an internal array
           instead of calling doSetStyle(). The following is thus checking the
           same as doSetStyle() but on the internal array. */
        CORRADE_COMPARE(shared.state().commonStyleUniform.smoothness, 3.14f);
        CORRADE_COMPARE(shared.state().styleUniforms.size(), 3);
        CORRADE_COMPARE(shared.state().styleUniforms[1].color, 0xc0ffee_rgbf);
    }
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::uniform), Containers::stridedArrayView({
        0u, 1u, 2u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::font), Containers::stridedArrayView({
        first,
        second,
        first
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::alignment), Containers::stridedArrayView({
        Text::Alignment::MiddleLeft,
        Text::Alignment::TopRight,
        Text::Alignment::LineCenterIntegral
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS((Containers::arrayCast<const Containers::Pair<Text::Feature, UnsignedInt>>(shared.state().styleFeatures)), (Containers::stridedArrayView<Containers::Pair<Text::Feature, UnsignedInt>>({
        {Text::Feature::SmallCapitals, true},
        {Text::Feature::Kerning, false},
        {Text::Feature::SlashedZero, true},
    })), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::featureOffset), Containers::stridedArrayView({
        0u, 1u, 2u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::featureCount), Containers::stridedArrayView({
        1u, 2u, 1u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::cursorStyle), Containers::stridedArrayView({
        1, -1, 0
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::selectionStyle), Containers::stridedArrayView({
        -1, 0, 1
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::padding), Containers::stridedArrayView({
        Vector4{},
        Vector4{},
        Vector4{}
    }), TestSuite::Compare::Container);

    /* Setting a style with implicit padding after a non-implicit padding was
       set should reset it back to zeros */
    shared.setStyle(
        TextLayerCommonStyleUniform{}
            .setSmoothness(3.14f),
        {TextLayerStyleUniform{},
         TextLayerStyleUniform{}
            .setColor(0xc0ffee_rgbf),
         TextLayerStyleUniform{}},
        {first, second, first},
        {Text::Alignment::MiddleLeft,
         Text::Alignment::TopRight,
         Text::Alignment::LineCenterIntegral},
        {Text::Feature::SmallCapitals,
         {Text::Feature::Kerning, false},
         Text::Feature::SlashedZero},
        {0, 1, 2},
        {1, 2, 1},
        {1, -1, 0},
        {-1, 0, 1},
        {{1.0f, 2.0f, 3.0f, 4.0f},
         {4.0f, 3.0f, 2.0f, 1.0f},
         {2.0f, 1.0f, 4.0f, 3.0f}});
    shared.setStyle(
        TextLayerCommonStyleUniform{}
            .setSmoothness(3.14f),
        {TextLayerStyleUniform{},
         TextLayerStyleUniform{}
            .setColor(0xc0ffee_rgbf),
         TextLayerStyleUniform{}},
        {first, second, first},
        {Text::Alignment::MiddleLeft,
         Text::Alignment::TopRight,
         Text::Alignment::LineCenterIntegral},
        {Text::Feature::SmallCapitals,
         {Text::Feature::Kerning, false},
         Text::Feature::SlashedZero},
        {0, 1, 2},
        {1, 2, 1},
        {1, -1, 0},
        {-1, 0, 1},
        {});
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().styles).slice(&Implementation::TextLayerStyle::padding), Containers::stridedArrayView({
        Vector4{},
        Vector4{},
        Vector4{}
    }), TestSuite::Compare::Container);
}

void TextLayerTest::sharedSetStyleImplicitMappingInvalidSize() {
    auto&& data = SharedSetStyleData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32}};

    struct Shared: TextLayer::Shared {
        explicit Shared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{3, 5}
        /* The checks should all deal with just the shared style count, not be
           dependent on this */
        .setDynamicStyleCount(data.dynamicStyleCount)
    };

    Containers::String out;
    Error redirectError{&out};
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}, TextLayerStyleUniform{}, TextLayerStyleUniform{}},
        {FontHandle::Null, FontHandle::Null, FontHandle::Null, FontHandle::Null, FontHandle::Null},
        {Text::Alignment{}, Text::Alignment{}, Text::Alignment{}, Text::Alignment{}},
        {}, {}, {},
        {-1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1},
        {{}, {}, {}, {}, {}});
    CORRADE_COMPARE(out,
        "Ui::TextLayer::Shared::setStyle(): there's 3 uniforms for 5 styles, provide an explicit mapping\n");
}

void TextLayerTest::sharedSetStyleInvalidFontHandle() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return {}; }
    } font;

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};
    cache.addFont(67, &font);

    struct Shared: TextLayer::Shared {
        explicit Shared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{4}};

    FontHandle handle = shared.addFont(font, 13.0f);

    /* Setting a null handle is okay, but create() etc that uses given style
       then has to explicitly pass a font handle */
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}, TextLayerStyleUniform{}, TextLayerStyleUniform{}, TextLayerStyleUniform{}},
        {handle, handle, FontHandle::Null, handle},
        {Text::Alignment::MiddleLeft, Text::Alignment::TopRight, Text::Alignment::BottomRight, Text::Alignment::LineLeft},
        {}, {}, {}, {}, {}, {});

    Containers::String out;
    Error redirectError{&out};
    /* Testing just the implicit mapping variant, as both variants delegate to
       the same internal helper */
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}, TextLayerStyleUniform{}, TextLayerStyleUniform{}, TextLayerStyleUniform{}},
        {handle, FontHandle(0x12ab), handle, handle},
        {Text::Alignment::MiddleLeft, Text::Alignment::TopRight, Text::Alignment::BottomRight, Text::Alignment::LineLeft},
        {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(out,
        "Ui::TextLayer::Shared::setStyle(): invalid handle Ui::FontHandle(0x12ab, 0x0) at index 1\n");
}

void TextLayerTest::sharedSetStyleInvalidAlignment() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32}};

    struct Shared: TextLayer::Shared {
        explicit Shared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{2}};

    Containers::String out;
    Error redirectError{&out};
    /* Testing just the implicit mapping variant, as both variants delegate to
       the same internal helper */
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}, TextLayerStyleUniform{}},
        {FontHandle::Null, FontHandle::Null},
        {Text::Alignment::MiddleLeft, Text::Alignment::LineCenterGlyphBounds},
        {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(out,
        "Ui::TextLayer::Shared::setStyle(): unsupported Text::Alignment::LineCenterGlyphBounds at index 1\n");
}

void TextLayerTest::sharedSetStyleInvalidFeatures() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32}};

    struct Shared: TextLayer::Shared {
        explicit Shared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{2}};

    Containers::String out;
    Error redirectError{&out};
    /* Testing just the implicit mapping variant, as both variants delegate to
       the same internal helper */
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}, TextLayerStyleUniform{}},
        {FontHandle::Null, FontHandle::Null},
        {Text::Alignment{}, Text::Alignment{}},
        {Text::Feature::Kerning,
         Text::Feature::SmallCapitals,
         Text::Feature::HistoricalLigatures,
         Text::Feature::SlashedZero},
        {0, 3}, {0, 2}, {}, {}, {});
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}, TextLayerStyleUniform{}},
        {FontHandle::Null, FontHandle::Null},
        {Text::Alignment{}, Text::Alignment{}},
        {Text::Feature::Kerning,
         Text::Feature::SmallCapitals,
         Text::Feature::HistoricalLigatures,
         Text::Feature::SlashedZero},
        {5, 3}, {0, 1}, {}, {}, {});
    CORRADE_COMPARE_AS(out,
        "Ui::TextLayer::Shared::setStyle(): feature offset 3 and count 2 out of range for 4 features at index 1\n"
        "Ui::TextLayer::Shared::setStyle(): feature offset 5 and count 0 out of range for 4 features at index 0\n",
        TestSuite::Compare::String);
}

void TextLayerTest::sharedSetStyleInvalidEditingStyles() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32}};

    struct Shared: TextLayer::Shared {
        explicit Shared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{2}
        .setEditingStyleCount(2, 3)
    };

    Containers::String out;
    Error redirectError{&out};
    /* Testing just the implicit mapping variant, as both variants delegate to
       the same internal helper */
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}, TextLayerStyleUniform{}},
        {FontHandle::Null, FontHandle::Null},
        {Text::Alignment{}, Text::Alignment{}},
        {}, {}, {},
        {-1, 3},
        {0, 2},
        {});
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}, TextLayerStyleUniform{}},
        {FontHandle::Null, FontHandle::Null},
        {Text::Alignment{}, Text::Alignment{}},
        {}, {}, {},
        {0, 2},
        {3, -1},
        {});
    CORRADE_COMPARE_AS(out,
        "Ui::TextLayer::Shared::setStyle(): cursor style 3 out of range for 3 editing styles at index 1\n"
        "Ui::TextLayer::Shared::setStyle(): selection style 3 out of range for 3 editing styles at index 0\n",
        TestSuite::Compare::String);
}

void TextLayerTest::sharedSetEditingStyle() {
    auto&& data = SharedSetStyleData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32}};

    struct Shared: TextLayer::Shared {
        explicit Shared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        State& state() { return static_cast<State&>(*_state); }

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {
            CORRADE_FAIL("This shouldn't be called.");
        }
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform& commonUniform, Containers::ArrayView<const TextLayerEditingStyleUniform> uniforms) override {
            CORRADE_COMPARE(commonUniform.smoothness, 3.14f);
            CORRADE_COMPARE(uniforms.size(), 3);
            CORRADE_COMPARE(uniforms[1].backgroundColor, 0xc0ffee_rgbf);
            ++setEditingStyleCalled;
        }

        Int setEditingStyleCalled = 0;
    } shared{cache, TextLayer::Shared::Configuration{17, 52}
        .setEditingStyleCount(3, 5)
        .setDynamicStyleCount(data.dynamicStyleCount)
    };

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    shared.setEditingStyle(
        TextLayerCommonEditingStyleUniform{}
            .setSmoothness(3.14f),
        {TextLayerEditingStyleUniform{},
         TextLayerEditingStyleUniform{}
            .setBackgroundColor(0xc0ffee_rgbf),
         TextLayerEditingStyleUniform{}},
        {2, 1, 0, 0, 1},
        {-1, 12, 6, -1, 15},
        {{1.0f, 2.0f, 3.0f, 4.0f},
         {4.0f, 3.0f, 2.0f, 1.0f},
         {2.0f, 1.0f, 4.0f, 3.0f},
         {1.0f, 3.0f, 2.0f, 4.0f},
         {4.0f, 1.0f, 3.0f, 2.0f}});
    if(data.dynamicStyleCount == 0) {
        CORRADE_COMPARE(shared.setEditingStyleCalled, 1);
    } else {
        CORRADE_COMPARE(shared.setEditingStyleCalled, 0);
        /* If there are dynamic styles, it's copied into an internal array
           instead of calling doSetEditingStyle(). The following is thus
           checking the same as doSetEditingStyle() but on the internal
           array. */
        CORRADE_COMPARE(shared.state().commonEditingStyleUniform.smoothness, 3.14f);
        CORRADE_COMPARE(shared.state().editingStyleUniforms.size(), 3);
        CORRADE_COMPARE(shared.state().editingStyleUniforms[1].backgroundColor, 0xc0ffee_rgbf);
    }
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().editingStyles).slice(&Implementation::TextLayerEditingStyle::uniform), Containers::stridedArrayView({
        2u, 1u, 0u, 0u, 1u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().editingStyles).slice(&Implementation::TextLayerEditingStyle::textUniform), Containers::stridedArrayView({
        -1, 12, 6, -1, 15
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().editingStyles).slice(&Implementation::TextLayerEditingStyle::padding), Containers::stridedArrayView({
        Vector4{1.0f, 2.0f, 3.0f, 4.0f},
        Vector4{4.0f, 3.0f, 2.0f, 1.0f},
        Vector4{2.0f, 1.0f, 4.0f, 3.0f},
        Vector4{1.0f, 3.0f, 2.0f, 4.0f},
        Vector4{4.0f, 1.0f, 3.0f, 2.0f}
    }), TestSuite::Compare::Container);
}

void TextLayerTest::sharedSetEditingStyleImplicitTextUniforms() {
    auto&& data = SharedSetStyleData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32}};

    struct Shared: TextLayer::Shared {
        explicit Shared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        State& state() { return static_cast<State&>(*_state); }

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {
            CORRADE_FAIL("This shouldn't be called.");
        }
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform& commonUniform, Containers::ArrayView<const TextLayerEditingStyleUniform> uniforms) override {
            CORRADE_COMPARE(commonUniform.smoothness, 3.14f);
            CORRADE_COMPARE(uniforms.size(), 3);
            CORRADE_COMPARE(uniforms[1].backgroundColor, 0xc0ffee_rgbf);
            ++setEditingStyleCalled;
        }

        Int setEditingStyleCalled = 0;
    } shared{cache, TextLayer::Shared::Configuration{17, 52}
        .setEditingStyleCount(3, 5)
        .setDynamicStyleCount(data.dynamicStyleCount)
    };

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    shared.setEditingStyle(
        TextLayerCommonEditingStyleUniform{}
            .setSmoothness(3.14f),
        {TextLayerEditingStyleUniform{},
         TextLayerEditingStyleUniform{}
            .setBackgroundColor(0xc0ffee_rgbf),
         TextLayerEditingStyleUniform{}},
        {2, 1, 0, 0, 1},
        {},
        {{1.0f, 2.0f, 3.0f, 4.0f},
         {4.0f, 3.0f, 2.0f, 1.0f},
         {2.0f, 1.0f, 4.0f, 3.0f},
         {1.0f, 3.0f, 2.0f, 4.0f},
         {4.0f, 1.0f, 3.0f, 2.0f}});
    if(data.dynamicStyleCount == 0) {
        CORRADE_COMPARE(shared.setEditingStyleCalled, 1);
    } else {
        CORRADE_COMPARE(shared.setEditingStyleCalled, 0);
        /* If there are dynamic styles, it's copied into an internal array
           instead of calling doSetEditingStyle(). The following is thus
           checking the same as doSetEditingStyle() but on the internal
           array. */
        CORRADE_COMPARE(shared.state().commonEditingStyleUniform.smoothness, 3.14f);
        CORRADE_COMPARE(shared.state().editingStyleUniforms.size(), 3);
        CORRADE_COMPARE(shared.state().editingStyleUniforms[1].backgroundColor, 0xc0ffee_rgbf);
    }
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().editingStyles).slice(&Implementation::TextLayerEditingStyle::uniform), Containers::stridedArrayView({
        2u, 1u, 0u, 0u, 1u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().editingStyles).slice(&Implementation::TextLayerEditingStyle::textUniform), Containers::stridedArrayView({
        -1, -1, -1, -1, -1
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().editingStyles).slice(&Implementation::TextLayerEditingStyle::padding), Containers::stridedArrayView({
        Vector4{1.0f, 2.0f, 3.0f, 4.0f},
        Vector4{4.0f, 3.0f, 2.0f, 1.0f},
        Vector4{2.0f, 1.0f, 4.0f, 3.0f},
        Vector4{1.0f, 3.0f, 2.0f, 4.0f},
        Vector4{4.0f, 1.0f, 3.0f, 2.0f}
    }), TestSuite::Compare::Container);

    /* Setting a style with implicit text uniforms after non-implicit uniforms
       were set should reset them back to -1 */
    shared.setEditingStyle(
        TextLayerCommonEditingStyleUniform{}
            .setSmoothness(3.14f),
        {TextLayerEditingStyleUniform{},
         TextLayerEditingStyleUniform{}
            .setBackgroundColor(0xc0ffee_rgbf),
         TextLayerEditingStyleUniform{}},
        {2, 1, 0, 0, 1},
        {-1, 12, 6, -1, 15},
        {{1.0f, 2.0f, 3.0f, 4.0f},
         {4.0f, 3.0f, 2.0f, 1.0f},
         {2.0f, 1.0f, 4.0f, 3.0f},
         {1.0f, 3.0f, 2.0f, 4.0f},
         {4.0f, 1.0f, 3.0f, 2.0f}});
    shared.setEditingStyle(
        TextLayerCommonEditingStyleUniform{}
            .setSmoothness(3.14f),
        {TextLayerEditingStyleUniform{},
         TextLayerEditingStyleUniform{}
            .setBackgroundColor(0xc0ffee_rgbf),
         TextLayerEditingStyleUniform{}},
        {2, 1, 0, 0, 1},
        {},
        {{1.0f, 2.0f, 3.0f, 4.0f},
         {4.0f, 3.0f, 2.0f, 1.0f},
         {2.0f, 1.0f, 4.0f, 3.0f},
         {1.0f, 3.0f, 2.0f, 4.0f},
         {4.0f, 1.0f, 3.0f, 2.0f}});
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().editingStyles).slice(&Implementation::TextLayerEditingStyle::textUniform), Containers::stridedArrayView({
        -1, -1, -1, -1, -1
    }), TestSuite::Compare::Container);
}

void TextLayerTest::sharedSetEditingStyleInvalidSize() {
    auto&& data = SharedSetStyleData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32}};

    struct Shared: TextLayer::Shared {
        explicit Shared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{17, 52}
        .setEditingStyleCount(3, 5)
        /* The checks should all deal with just the shared style count, not be
           dependent on this */
        .setDynamicStyleCount(data.dynamicStyleCount)
    };

    Containers::String out;
    Error redirectError{&out};
    shared.setEditingStyle(TextLayerCommonEditingStyleUniform{},
        {TextLayerEditingStyleUniform{},
         TextLayerEditingStyleUniform{}},
        {2, 1, 0, 0, 1},
        {{}, {}, {}, {}, {}},
        {{}, {}, {}, {}, {}});
    shared.setEditingStyle(TextLayerCommonEditingStyleUniform{},
        {TextLayerEditingStyleUniform{},
         TextLayerEditingStyleUniform{},
         TextLayerEditingStyleUniform{}},
        {2, 1, 0},
        {{}, {}, {}, {}, {}},
        {{}, {}, {}, {}, {}});
    shared.setEditingStyle(TextLayerCommonEditingStyleUniform{},
        {TextLayerEditingStyleUniform{},
         TextLayerEditingStyleUniform{},
         TextLayerEditingStyleUniform{}},
        {2, 1, 0, 0, 1},
        {{}, {}, {}, {}},
        {{}, {}, {}, {}, {}});
    shared.setEditingStyle(TextLayerCommonEditingStyleUniform{},
        {TextLayerEditingStyleUniform{},
         TextLayerEditingStyleUniform{},
         TextLayerEditingStyleUniform{}},
        {2, 1, 0, 0, 1},
        {{}, {}, {}, {}, {}},
        {{}, {}, {}, {}});
    /* Compared to setStyle(), empty paddings shouldn't be allowed here */
    shared.setEditingStyle(TextLayerCommonEditingStyleUniform{},
        {TextLayerEditingStyleUniform{},
         TextLayerEditingStyleUniform{},
         TextLayerEditingStyleUniform{}},
        {2, 1, 0, 0, 1},
        {{}, {}, {}, {}, {}},
        {});
    CORRADE_COMPARE_AS(out,
        "Ui::TextLayer::Shared::setEditingStyle(): expected 3 uniforms, got 2\n"
        "Ui::TextLayer::Shared::setEditingStyle(): expected 5 style uniform indices, got 3\n"
        "Ui::TextLayer::Shared::setEditingStyle(): expected either no or 5 text uniform indices, got 4\n"
        "Ui::TextLayer::Shared::setEditingStyle(): expected 5 paddings, got 4\n"
        "Ui::TextLayer::Shared::setEditingStyle(): expected 5 paddings, got 0\n",
        TestSuite::Compare::String);
}

void TextLayerTest::sharedSetEditingStyleInvalidMapping() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32}};

    struct Shared: TextLayer::Shared {
        explicit Shared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{2, 1}
        .setEditingStyleCount(3, 6)
    }, sharedMatchingUniformCount{cache, TextLayer::Shared::Configuration{2, 1}
        .setEditingStyleCount(3)
    };

    Containers::String out;
    Error redirectError{&out};
    shared.setEditingStyle(TextLayerCommonEditingStyleUniform{},
        {TextLayerEditingStyleUniform{},
         TextLayerEditingStyleUniform{},
         TextLayerEditingStyleUniform{}},
        {0, 1, 2, 1, 3, 2},
        {},
        {{}, {}, {}, {}, {}, {}});
    shared.setEditingStyle(TextLayerCommonEditingStyleUniform{},
        {TextLayerEditingStyleUniform{},
         TextLayerEditingStyleUniform{},
         TextLayerEditingStyleUniform{}},
        {0, 1, 2, 1, 2, 0},
        {-1, -1, 0, 1, 2, -1},
        {{}, {}, {}, {}, {}, {}});
    /* It should fire in this overload as well, i.e. be done in the common
       code for both, not the leaf function */
    sharedMatchingUniformCount.setEditingStyle(TextLayerCommonEditingStyleUniform{},
        {TextLayerEditingStyleUniform{},
         TextLayerEditingStyleUniform{},
         TextLayerEditingStyleUniform{}},
        {-1, 1, 2},
        {{}, {}, {}});
    CORRADE_COMPARE_AS(out,
        "Ui::TextLayer::Shared::setEditingStyle(): uniform index 3 out of range for 3 uniforms at index 4\n"
        "Ui::TextLayer::Shared::setEditingStyle(): text uniform index 2 out of range for 2 uniforms at index 4\n"
        "Ui::TextLayer::Shared::setEditingStyle(): text uniform index 2 out of range for 2 uniforms at index 2\n",
        TestSuite::Compare::String);
}

void TextLayerTest::sharedSetEditingStyleImplicitMapping() {
    auto&& data = SharedSetStyleData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32}};

    struct Shared: TextLayer::Shared {
        explicit Shared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        State& state() { return static_cast<State&>(*_state); }

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {
            CORRADE_FAIL("This shouldn't be called.");
        }
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform& commonUniform, Containers::ArrayView<const TextLayerEditingStyleUniform> uniforms) override {
            CORRADE_COMPARE(commonUniform.smoothness, 3.14f);
            CORRADE_COMPARE(uniforms.size(), 3);
            CORRADE_COMPARE(uniforms[1].backgroundColor, 0xc0ffee_rgbf);
            ++setEditingStyleCalled;
        }

        Int setEditingStyleCalled = 0;
    } shared{cache, TextLayer::Shared::Configuration{17, 52}
        .setEditingStyleCount(3)
        .setDynamicStyleCount(data.dynamicStyleCount)
    };

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    shared.setEditingStyle(
        TextLayerCommonEditingStyleUniform{}
            .setSmoothness(3.14f),
        {TextLayerEditingStyleUniform{},
         TextLayerEditingStyleUniform{}
            .setBackgroundColor(0xc0ffee_rgbf),
         TextLayerEditingStyleUniform{}},
        {13, -1, 6},
        {{1.0f, 2.0f, 3.0f, 4.0f},
         {4.0f, 3.0f, 2.0f, 1.0f},
         {2.0f, 1.0f, 4.0f, 3.0f}});
    if(data.dynamicStyleCount == 0) {
        CORRADE_COMPARE(shared.setEditingStyleCalled, 1);
    } else {
        CORRADE_COMPARE(shared.setEditingStyleCalled, 0);
        /* If there are dynamic styles, it's copied into an internal array
           instead of calling doSetEditingStyle(). The following is thus
           checking the same as doSetEditingStyle() but on the internal
           array. */
        CORRADE_COMPARE(shared.state().commonEditingStyleUniform.smoothness, 3.14f);
        CORRADE_COMPARE(shared.state().editingStyleUniforms.size(), 3);
        CORRADE_COMPARE(shared.state().editingStyleUniforms[1].backgroundColor, 0xc0ffee_rgbf);
    }
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().editingStyles).slice(&Implementation::TextLayerEditingStyle::uniform), Containers::stridedArrayView({
        0u, 1u, 2u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().editingStyles).slice(&Implementation::TextLayerEditingStyle::textUniform), Containers::stridedArrayView({
        13, -1, 6
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().editingStyles).slice(&Implementation::TextLayerEditingStyle::padding), Containers::stridedArrayView({
        Vector4{1.0f, 2.0f, 3.0f, 4.0f},
        Vector4{4.0f, 3.0f, 2.0f, 1.0f},
        Vector4{2.0f, 1.0f, 4.0f, 3.0f}
    }), TestSuite::Compare::Container);
}

void TextLayerTest::sharedSetEditingStyleImplicitMappingImplicitTextUniforms() {
    auto&& data = SharedSetStyleData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32}};

    struct Shared: TextLayer::Shared {
        explicit Shared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        State& state() { return static_cast<State&>(*_state); }

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {
            CORRADE_FAIL("This shouldn't be called.");
        }
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform& commonUniform, Containers::ArrayView<const TextLayerEditingStyleUniform> uniforms) override {
            CORRADE_COMPARE(commonUniform.smoothness, 3.14f);
            CORRADE_COMPARE(uniforms.size(), 3);
            CORRADE_COMPARE(uniforms[1].backgroundColor, 0xc0ffee_rgbf);
            ++setEditingStyleCalled;
        }

        Int setEditingStyleCalled = 0;
    } shared{cache, TextLayer::Shared::Configuration{17, 52}
        .setEditingStyleCount(3)
        .setDynamicStyleCount(data.dynamicStyleCount)
    };

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    shared.setEditingStyle(
        TextLayerCommonEditingStyleUniform{}
            .setSmoothness(3.14f),
        {TextLayerEditingStyleUniform{},
         TextLayerEditingStyleUniform{}
            .setBackgroundColor(0xc0ffee_rgbf),
         TextLayerEditingStyleUniform{}},
        {},
        {{1.0f, 2.0f, 3.0f, 4.0f},
         {4.0f, 3.0f, 2.0f, 1.0f},
         {2.0f, 1.0f, 4.0f, 3.0f}});
    if(data.dynamicStyleCount == 0) {
        CORRADE_COMPARE(shared.setEditingStyleCalled, 1);
    } else {
        CORRADE_COMPARE(shared.setEditingStyleCalled, 0);
        /* If there are dynamic styles, it's copied into an internal array
           instead of calling doSetEditingStyle(). The following is thus
           checking the same as doSetEditingStyle() but on the internal
           array. */
        CORRADE_COMPARE(shared.state().commonEditingStyleUniform.smoothness, 3.14f);
        CORRADE_COMPARE(shared.state().editingStyleUniforms.size(), 3);
        CORRADE_COMPARE(shared.state().editingStyleUniforms[1].backgroundColor, 0xc0ffee_rgbf);
    }
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().editingStyles).slice(&Implementation::TextLayerEditingStyle::uniform), Containers::stridedArrayView({
        0u, 1u, 2u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().editingStyles).slice(&Implementation::TextLayerEditingStyle::textUniform), Containers::stridedArrayView({
        -1, -1, -1
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().editingStyles).slice(&Implementation::TextLayerEditingStyle::padding), Containers::stridedArrayView({
        Vector4{1.0f, 2.0f, 3.0f, 4.0f},
        Vector4{4.0f, 3.0f, 2.0f, 1.0f},
        Vector4{2.0f, 1.0f, 4.0f, 3.0f}
    }), TestSuite::Compare::Container);

    /* Setting a style with implicit text uniforms after non-implicit uniforms
       were set should reset them back to -1 */
    shared.setEditingStyle(
        TextLayerCommonEditingStyleUniform{}
            .setSmoothness(3.14f),
        {TextLayerEditingStyleUniform{},
         TextLayerEditingStyleUniform{}
            .setBackgroundColor(0xc0ffee_rgbf),
         TextLayerEditingStyleUniform{}},
        {13, -1, 6},
        {{1.0f, 2.0f, 3.0f, 4.0f},
         {4.0f, 3.0f, 2.0f, 1.0f},
         {2.0f, 1.0f, 4.0f, 3.0f}});
    shared.setEditingStyle(
        TextLayerCommonEditingStyleUniform{}
            .setSmoothness(3.14f),
        {TextLayerEditingStyleUniform{},
         TextLayerEditingStyleUniform{}
            .setBackgroundColor(0xc0ffee_rgbf),
         TextLayerEditingStyleUniform{}},
        {},
        {{1.0f, 2.0f, 3.0f, 4.0f},
         {4.0f, 3.0f, 2.0f, 1.0f},
         {2.0f, 1.0f, 4.0f, 3.0f}});
    CORRADE_COMPARE_AS(stridedArrayView(shared.state().editingStyles).slice(&Implementation::TextLayerEditingStyle::textUniform), Containers::stridedArrayView({
        -1, -1, -1
    }), TestSuite::Compare::Container);
}

void TextLayerTest::sharedSetEditingStyleImplicitMappingInvalidSize() {
    auto&& data = SharedSetStyleData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32}};

    struct Shared: TextLayer::Shared {
        explicit Shared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{17, 52}
        .setEditingStyleCount(3, 5)
        /* The checks should all deal with just the shared style count, not be
           dependent on this */
        .setDynamicStyleCount(data.dynamicStyleCount)
    };

    Containers::String out;
    Error redirectError{&out};
    shared.setEditingStyle(TextLayerCommonEditingStyleUniform{},
        {TextLayerEditingStyleUniform{}, TextLayerEditingStyleUniform{}, TextLayerEditingStyleUniform{}},
        {{}, {}, {}, {}, {}},
        {{}, {}, {}, {}, {}});
    CORRADE_COMPARE(out,
        "Ui::TextLayer::Shared::setEditingStyle(): there's 3 uniforms for 5 styles, provide an explicit mapping\n");
}

void TextLayerTest::construct() {
    auto&& data = ConstructData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32}};

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{3, 5}};

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared, TextLayerFlags flags): TextLayer{handle, shared, flags} {}
    } layer{layerHandle(137, 0xfe), shared, data.layerFlags};

    CORRADE_COMPARE(layer.handle(), layerHandle(137, 0xfe));
    CORRADE_COMPARE(&layer.shared(), &shared);
    /* Const overload */
    CORRADE_COMPARE(&static_cast<const Layer&>(layer).shared(), &shared);
    CORRADE_COMPARE(layer.flags(), data.layerFlags);
}

void TextLayerTest::constructCopy() {
    CORRADE_VERIFY(!std::is_copy_constructible<TextLayer>{});
    CORRADE_VERIFY(!std::is_copy_assignable<TextLayer>{});
}

void TextLayerTest::constructMove() {
    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32}};

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    };

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, LayerShared& shared): TextLayer{handle, shared} {}
    };

    LayerShared shared{cache, TextLayer::Shared::Configuration{1, 3}};
    LayerShared shared2{cache, TextLayer::Shared::Configuration{5, 7}};

    Layer a{layerHandle(137, 0xfe), shared};

    Layer b{Utility::move(a)};
    CORRADE_COMPARE(b.handle(), layerHandle(137, 0xfe));
    CORRADE_COMPARE(&b.shared(), &shared);

    Layer c{layerHandle(0, 2), shared2};
    c = Utility::move(b);
    CORRADE_COMPARE(c.handle(), layerHandle(137, 0xfe));
    CORRADE_COMPARE(&c.shared(), &shared);

    CORRADE_VERIFY(std::is_nothrow_move_constructible<TextLayer>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<TextLayer>::value);
}

void TextLayerTest::dynamicStyle() {
    auto&& data = DynamicStyleData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return {}; }
    } font;

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};
    cache.addFont(67, &font);

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{3, 5}
        .setDynamicStyleCount(3)
    };

    FontHandle fontHandle = shared.addFont(font, 1.0f);

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}

        TextLayer::State& stateData() {
            return static_cast<TextLayer::State&>(*_state);
        }
    } layer{layerHandle(0, 1), shared};

    /* All styles should be set to their defaults initially. Checking just a
       subset of the uniform properties, should be enough. */
    CORRADE_COMPARE_AS(stridedArrayView(layer.dynamicStyleUniforms()).slice(&TextLayerStyleUniform::color), Containers::arrayView({
        0xffffffff_rgbaf,
        0xffffffff_rgbaf,
        0xffffffff_rgbaf
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(layer.dynamicStyleFonts(), Containers::arrayView({
        FontHandle::Null,
        FontHandle::Null,
        FontHandle::Null
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(layer.dynamicStyleAlignments(), Containers::arrayView({
        Text::Alignment::MiddleCenter,
        Text::Alignment::MiddleCenter,
        Text::Alignment::MiddleCenter,
    }), TestSuite::Compare::Container);
    CORRADE_VERIFY(layer.dynamicStyleFeatures(0).isEmpty());
    CORRADE_VERIFY(layer.dynamicStyleFeatures(1).isEmpty());
    CORRADE_VERIFY(layer.dynamicStyleFeatures(2).isEmpty());
    CORRADE_COMPARE_AS(layer.dynamicStylePaddings(), Containers::arrayView({
        Vector4{0.0f},
        Vector4{0.0f},
        Vector4{0.0f}
    }), TestSuite::Compare::Container);
    /* No cursor and selection styles assigned by default */
    CORRADE_COMPARE_AS(layer.dynamicStyleCursorStyles(), Containers::stridedArrayView({
        false, false, false
    }).sliceBit(0), TestSuite::Compare::Container);
    CORRADE_COMPARE(layer.dynamicStyleCursorStyle(0), -1);
    CORRADE_COMPARE(layer.dynamicStyleCursorStyle(1), -1);
    CORRADE_COMPARE(layer.dynamicStyleCursorStyle(2), -1);
    CORRADE_COMPARE_AS(layer.dynamicStyleSelectionStyles(), Containers::stridedArrayView({
        false, false, false
    }).sliceBit(0), TestSuite::Compare::Container);
    CORRADE_COMPARE(layer.dynamicStyleSelectionStyle(0), -1);
    CORRADE_COMPARE(layer.dynamicStyleSelectionStyle(1), -1);
    CORRADE_COMPARE(layer.dynamicStyleSelectionStyle(2), -1);
    CORRADE_COMPARE(layer.dynamicStyleSelectionStyleTextUniform(0), -1);
    CORRADE_COMPARE(layer.dynamicStyleSelectionStyleTextUniform(1), -1);
    CORRADE_COMPARE(layer.dynamicStyleSelectionStyleTextUniform(2), -1);
    /* Neither LayerState nor the state bit is set initially, the initial
       upload is done implicitly on the first update */
    CORRADE_COMPARE(layer.state(), LayerStates{});
    CORRADE_VERIFY(!layer.stateData().dynamicStyleChanged);

    /* Setting a style should change these and flip the state bit on again */
    layer.stateData().dynamicStyleChanged = false;
    layer.setDynamicStyle(1,
        TextLayerStyleUniform{}
            .setColor(0x11223344_rgbaf),
        data.changeFont ? fontHandle : FontHandle::Null,
        data.alignment1,
        data.features1,
        data.padding1);
    layer.setDynamicStyle(2,
        TextLayerStyleUniform{}
            .setColor(0xff3366_rgbf),
        FontHandle::Null, /* Null is allowed */
        data.alignment2,
        data.features2,
        data.padding2);
    CORRADE_COMPARE_AS(stridedArrayView(layer.dynamicStyleUniforms()).slice(&TextLayerStyleUniform::color), Containers::arrayView({
        0xffffffff_rgbaf,
        0x11223344_rgbaf,
        0xff3366ff_rgbaf
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(layer.dynamicStyleFonts(), Containers::arrayView({
        FontHandle::Null,
        data.changeFont ? fontHandle : FontHandle::Null,
        FontHandle::Null
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(layer.dynamicStyleAlignments(), Containers::arrayView({
        Text::Alignment::MiddleCenter,
        data.alignment1,
        data.alignment2
    }), TestSuite::Compare::Container);
    CORRADE_VERIFY(layer.dynamicStyleFeatures(0).isEmpty());
    CORRADE_COMPARE_AS(
        (Containers::arrayCast<const Containers::Pair<Text::Feature, UnsignedInt>>(layer.dynamicStyleFeatures(1))),
        (Containers::arrayCast<const Containers::Pair<Text::Feature, UnsignedInt>>(data.features1)),
        TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(
        (Containers::arrayCast<const Containers::Pair<Text::Feature, UnsignedInt>>(layer.dynamicStyleFeatures(2))),
        (Containers::arrayCast<const Containers::Pair<Text::Feature, UnsignedInt>>(data.features2)),
        TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(layer.dynamicStylePaddings(), Containers::arrayView({
        Vector4{0.0f},
        data.padding1,
        data.padding2
    }), TestSuite::Compare::Container);
    /* No cursor and selection styles assigned now either */
    CORRADE_COMPARE_AS(layer.dynamicStyleCursorStyles(), Containers::stridedArrayView({
        false, false, false
    }).sliceBit(0), TestSuite::Compare::Container);
    CORRADE_COMPARE(layer.dynamicStyleCursorStyle(0), -1);
    CORRADE_COMPARE(layer.dynamicStyleCursorStyle(1), -1);
    CORRADE_COMPARE(layer.dynamicStyleCursorStyle(2), -1);
    CORRADE_COMPARE_AS(layer.dynamicStyleSelectionStyles(), Containers::stridedArrayView({
        false, false, false
    }).sliceBit(0), TestSuite::Compare::Container);
    CORRADE_COMPARE(layer.dynamicStyleSelectionStyle(0), -1);
    CORRADE_COMPARE(layer.dynamicStyleSelectionStyle(1), -1);
    CORRADE_COMPARE(layer.dynamicStyleSelectionStyle(2), -1);
    CORRADE_COMPARE(layer.dynamicStyleSelectionStyleTextUniform(0), -1);
    CORRADE_COMPARE(layer.dynamicStyleSelectionStyleTextUniform(1), -1);
    CORRADE_COMPARE(layer.dynamicStyleSelectionStyleTextUniform(2), -1);
    CORRADE_COMPARE(layer.state(), data.expectedStates);
    CORRADE_VERIFY(layer.stateData().dynamicStyleChanged);
}

void TextLayerTest::dynamicStyleFeatureAllocation() {
    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32}};

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{12, 2}
        .setDynamicStyleCount(3)
    };

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}

        const State& stateData() const {
            return static_cast<const State&>(*_state);
        }
    } layer{layerHandle(0, 1), shared};

    /* Filling up the feature lists for the first time will add them in the
       call order */
    layer.setDynamicStyle(2, TextLayerStyleUniform{}, FontHandle::Null, Text::Alignment{}, {
        {Text::Feature::Kerning, false},
        {Text::Feature::HistoricalLigatures, true},
    }, {});
    layer.setDynamicStyle(0, TextLayerStyleUniform{}, FontHandle::Null, Text::Alignment{}, {
        {Text::Feature::AccessAllAlternates, 57},
        {Text::Feature::SlashedZero, false},
        {Text::Feature::CharacterVariants47, true},
    }, {});
    layer.setDynamicStyle(1, TextLayerStyleUniform{}, FontHandle::Null, Text::Alignment{}, {
        {Text::Feature::TabularFigures, true},
    }, {});
    CORRADE_COMPARE_AS((Containers::arrayCast<const Containers::Pair<Text::Feature, UnsignedInt>>(layer.stateData().dynamicStyleFeatures)), (Containers::arrayView<Containers::Pair<Text::Feature, UnsignedInt>>({
        /* Style 2 */
        {Text::Feature::Kerning, false},
        {Text::Feature::HistoricalLigatures, true},
        /* Style 0 */
        {Text::Feature::AccessAllAlternates, 57},
        {Text::Feature::SlashedZero, false},
        {Text::Feature::CharacterVariants47, true},
        /* Style 1 */
        {Text::Feature::TabularFigures, true},
    })), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().dynamicStyles).slice(&Implementation::TextLayerDynamicStyle::featureOffset), Containers::stridedArrayView({
        2u, 5u, 0u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().dynamicStyles).slice(&Implementation::TextLayerDynamicStyle::featureCount), Containers::stridedArrayView({
        3u, 1u, 2u
    }), TestSuite::Compare::Container);

    /* Replacing a style with one that has the same feature count will keep it
       where it was */
    layer.setDynamicStyle(2, TextLayerStyleUniform{}, FontHandle::Null, Text::Alignment{}, {
        {Text::Feature::StylisticSet15, true},
        {Text::Feature::StandardLigatures, false},
    }, {});
    CORRADE_COMPARE_AS((Containers::arrayCast<const Containers::Pair<Text::Feature, UnsignedInt>>(layer.stateData().dynamicStyleFeatures)), (Containers::arrayView<Containers::Pair<Text::Feature, UnsignedInt>>({
        /* Style 2, changed */
        {Text::Feature::StylisticSet15, true},
        {Text::Feature::StandardLigatures, false},
        /* Style 0 */
        {Text::Feature::AccessAllAlternates, 57},
        {Text::Feature::SlashedZero, false},
        {Text::Feature::CharacterVariants47, true},
        /* Style 1 */
        {Text::Feature::TabularFigures, true},
    })), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().dynamicStyles).slice(&Implementation::TextLayerDynamicStyle::featureOffset), Containers::stridedArrayView({
        2u, 5u, 0u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().dynamicStyles).slice(&Implementation::TextLayerDynamicStyle::featureCount), Containers::stridedArrayView({
        3u, 1u, 2u
    }), TestSuite::Compare::Container);

    /* Replacing a style with one having less features will move it to the
       end, adjusting everything that changed */
    layer.setDynamicStyle(0, TextLayerStyleUniform{}, FontHandle::Null, Text::Alignment{}, {
        {Text::Feature::Kerning, false},
        {Text::Feature::ContextualLigatures, true},
    }, {});
    CORRADE_COMPARE_AS((Containers::arrayCast<const Containers::Pair<Text::Feature, UnsignedInt>>(layer.stateData().dynamicStyleFeatures)), (Containers::arrayView<Containers::Pair<Text::Feature, UnsignedInt>>({
        /* Style 2 */
        {Text::Feature::StylisticSet15, true},
        {Text::Feature::StandardLigatures, false},
        /* Style 1 */
        {Text::Feature::TabularFigures, true},
        /* Style 0, changed */
        {Text::Feature::Kerning, false},
        {Text::Feature::ContextualLigatures, true},
    })), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().dynamicStyles).slice(&Implementation::TextLayerDynamicStyle::featureOffset), Containers::stridedArrayView({
        3u, 2u, 0u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().dynamicStyles).slice(&Implementation::TextLayerDynamicStyle::featureCount), Containers::stridedArrayView({
        2u, 1u, 2u
    }), TestSuite::Compare::Container);

    /* Replacing a style with more features will also move it */
    layer.setDynamicStyle(2, TextLayerStyleUniform{}, FontHandle::Null, Text::Alignment{}, {
        {Text::Feature::Kerning, true},
        {Text::Feature::SlashedZero, true},
        {Text::Feature::SmallCapitals, true},
    }, {});
    CORRADE_COMPARE_AS((Containers::arrayCast<const Containers::Pair<Text::Feature, UnsignedInt>>(layer.stateData().dynamicStyleFeatures)), (Containers::arrayView<Containers::Pair<Text::Feature, UnsignedInt>>({
        /* Style 1 */
        {Text::Feature::TabularFigures, true},
        /* Style 0 */
        {Text::Feature::Kerning, false},
        {Text::Feature::ContextualLigatures, true},
        /* Style 2, changed */
        {Text::Feature::Kerning, true},
        {Text::Feature::SlashedZero, true},
        {Text::Feature::SmallCapitals, true},
    })), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().dynamicStyles).slice(&Implementation::TextLayerDynamicStyle::featureOffset), Containers::stridedArrayView({
        1u, 0u, 3u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().dynamicStyles).slice(&Implementation::TextLayerDynamicStyle::featureCount), Containers::stridedArrayView({
        2u, 1u, 3u
    }), TestSuite::Compare::Container);

    /* Making a style empty works too */
    layer.setDynamicStyle(0, TextLayerStyleUniform{}, FontHandle::Null, Text::Alignment{}, {}, {});
    CORRADE_COMPARE_AS((Containers::arrayCast<const Containers::Pair<Text::Feature, UnsignedInt>>(layer.stateData().dynamicStyleFeatures)), (Containers::arrayView<Containers::Pair<Text::Feature, UnsignedInt>>({
        /* Style 1 */
        {Text::Feature::TabularFigures, true},
        /* Style 2 */
        {Text::Feature::Kerning, true},
        {Text::Feature::SlashedZero, true},
        {Text::Feature::SmallCapitals, true},
        /* Style 0, changed */
    })), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().dynamicStyles).slice(&Implementation::TextLayerDynamicStyle::featureOffset), Containers::stridedArrayView({
        4u, 0u, 1u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().dynamicStyles).slice(&Implementation::TextLayerDynamicStyle::featureCount), Containers::stridedArrayView({
        0u, 1u, 3u
    }), TestSuite::Compare::Container);
}

void TextLayerTest::dynamicStyleEditingStyles() {
    auto&& data = DynamicStyleEditingStylesData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32}};

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{3, 1}
        /* There's zero editing styles, so we explicitly opt-in for them for
           dynamic styles */
        .setDynamicStyleCount(2, true)
    };

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}

        TextLayer::State& stateData() {
            return static_cast<TextLayer::State&>(*_state);
        }
    } layer{layerHandle(0, 1), shared};

    /* Required to be called before update() (because AbstractUserInterface
       guarantees the same on a higher level), not needed for anything here */
    layer.setSize({1, 1}, {1, 1});

    /* All styles should be set to their defaults initially. Checking just a
       subset of the non-editing properties that might actually change, should
       be enough. */
    CORRADE_COMPARE_AS(stridedArrayView(layer.dynamicStyleUniforms()).slice(&TextLayerStyleUniform::color), Containers::arrayView({
        0xffffffff_rgbaf,
        0xffffffff_rgbaf,
        /* Each editing style adds two more uniforms, one for selected text and
           one reserved for an overwriting cursor */
        0xffffffff_rgbaf,
        0xffffffff_rgbaf,
        0xffffffff_rgbaf,
        0xffffffff_rgbaf,
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(layer.dynamicStylePaddings(), Containers::arrayView({
        Vector4{0.0f},
        Vector4{0.0f}
    }), TestSuite::Compare::Container);
    /* No cursor and selection styles assigned by default */
    CORRADE_COMPARE_AS(layer.dynamicStyleCursorStyles(), Containers::stridedArrayView({
        false, false
    }).sliceBit(0), TestSuite::Compare::Container);
    CORRADE_COMPARE(layer.dynamicStyleCursorStyle(0), -1);
    CORRADE_COMPARE(layer.dynamicStyleCursorStyle(1), -1);
    CORRADE_COMPARE_AS(layer.dynamicStyleSelectionStyles(), Containers::stridedArrayView({
        false, false
    }).sliceBit(0), TestSuite::Compare::Container);
    CORRADE_COMPARE(layer.dynamicStyleSelectionStyle(0), -1);
    CORRADE_COMPARE(layer.dynamicStyleSelectionStyle(1), -1);
    CORRADE_COMPARE(layer.dynamicStyleSelectionStyleTextUniform(0), -1);
    CORRADE_COMPARE(layer.dynamicStyleSelectionStyleTextUniform(1), -1);
    /* Editing styles set their defaults initially as well; again checking just
       a subset of the uniform properties */
    CORRADE_COMPARE_AS(stridedArrayView(layer.dynamicEditingStyleUniforms()).slice(&TextLayerEditingStyleUniform::backgroundColor), Containers::arrayView({
        0xffffffff_rgbaf,
        0xffffffff_rgbaf,
        0xffffffff_rgbaf,
        0xffffffff_rgbaf
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(layer.dynamicEditingStylePaddings(), Containers::arrayView({
        Vector4{0.0f},
        Vector4{0.0f},
        Vector4{0.0f},
        Vector4{0.0f}
    }), TestSuite::Compare::Container);
    /* Neither LayerState nor the state bit is set initially, the initial
       upload is done implicitly on the first update */
    CORRADE_COMPARE(layer.state(), LayerStates{});
    CORRADE_VERIFY(!layer.stateData().dynamicStyleChanged);
    CORRADE_VERIFY(!layer.stateData().dynamicEditingStyleChanged);

    /* Setting an editing style for the first time always sets the LayerState
       no matter what gets changed */
    layer.stateData().dynamicStyleChanged = false;
    layer.stateData().dynamicEditingStyleChanged = false;
    if(data.cursorPadding1 && data.selectionPadding1)
        layer.setDynamicStyleWithCursorSelection(1,
            TextLayerStyleUniform{}
                .setColor(0xaabbcc_rgbf),
            FontHandle::Null,
            Text::Alignment::MiddleCenter, {}, data.padding1,
            TextLayerEditingStyleUniform{}
                .setBackgroundColor(0xaabbccdd_rgbaf)
                .setCornerRadius(4.0f),
            *data.cursorPadding1,
            TextLayerEditingStyleUniform{}
                .setBackgroundColor(0xeeff9966_rgbaf)
                .setCornerRadius(1.5f),
            data.textUniform1 ?
                TextLayerStyleUniform{}
                    .setColor(0x11223344_rgbaf) :
                Containers::Optional<TextLayerStyleUniform>{},
            *data.selectionPadding1);
    else if(data.cursorPadding1)
        layer.setDynamicStyleWithCursor(1,
            TextLayerStyleUniform{}
                .setColor(0xaabbcc_rgbf),
            FontHandle::Null,
            Text::Alignment::MiddleCenter, {}, data.padding1,
            TextLayerEditingStyleUniform{}
                .setBackgroundColor(0xaabbccdd_rgbaf)
                .setCornerRadius(4.0f),
            *data.cursorPadding1);
    else if(data.selectionPadding1)
        layer.setDynamicStyleWithSelection(1,
            TextLayerStyleUniform{}
                .setColor(0xaabbcc_rgbf),
            FontHandle::Null,
            Text::Alignment::MiddleCenter, {}, data.padding1,
            TextLayerEditingStyleUniform{}
                .setBackgroundColor(0xeeff9966_rgbaf)
                .setCornerRadius(1.5f),
            data.textUniform1 ?
                TextLayerStyleUniform{}
                    .setColor(0x11223344_rgbaf) :
                Containers::Optional<TextLayerStyleUniform>{},
            *data.selectionPadding1);
    else CORRADE_INTERNAL_ASSERT_UNREACHABLE();
    CORRADE_COMPARE_AS(stridedArrayView(layer.dynamicStyleUniforms()).slice(&TextLayerStyleUniform::color), Containers::arrayView({
        0xffffffff_rgbaf,
        0xaabbccff_rgbaf,
        /* Each editing style adds two more uniforms, one for selected text and
           one reserved for an overwriting cursor */
        0xffffffff_rgbaf,
        0xffffffff_rgbaf,
        /* If text uniform isn't set, the base uniform value is copied there,
           unless it doesn't include a selection style */
        data.textUniform1 ? 0x11223344_rgbaf :
            data.selectionPadding1 ? 0xaabbcc_rgbf : 0xffffffff_rgbaf,
        0xffffffff_rgbaf,
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(layer.dynamicStylePaddings(), Containers::arrayView({
        Vector4{0.0f},
        data.padding1
    }), TestSuite::Compare::Container);
    /* The editing styles have a fixed ID corresponding to the dynamic style
       ID, for each dynamic style it's a pair with the selection first and
       cursor second (matching draw order) */
    CORRADE_COMPARE_AS(layer.dynamicStyleCursorStyles(), Containers::stridedArrayView({
        false, !!data.cursorPadding1
    }).sliceBit(0), TestSuite::Compare::Container);
    CORRADE_COMPARE(layer.dynamicStyleCursorStyle(0), -1);
    CORRADE_COMPARE(layer.dynamicStyleCursorStyle(1), data.cursorPadding1 ? 3 : -1);
    CORRADE_COMPARE_AS(layer.dynamicStyleSelectionStyles(), Containers::stridedArrayView({
        false, !!data.selectionPadding1
    }).sliceBit(0), TestSuite::Compare::Container);
    CORRADE_COMPARE(layer.dynamicStyleSelectionStyle(0), -1);
    CORRADE_COMPARE(layer.dynamicStyleSelectionStyle(1), data.selectionPadding1 ? 2 : -1);
    CORRADE_COMPARE(layer.dynamicStyleSelectionStyleTextUniform(0), -1);
    CORRADE_COMPARE(layer.dynamicStyleSelectionStyleTextUniform(1), data.selectionPadding1 ? 4 : -1);
    CORRADE_COMPARE_AS(stridedArrayView(layer.dynamicEditingStyleUniforms()).slice(&TextLayerEditingStyleUniform::backgroundColor), Containers::arrayView({
        0xffffffff_rgbaf,
        0xffffffff_rgbaf,
        data.selectionPadding1 ? 0xeeff9966_rgbaf : 0xffffffff_rgbaf,
        data.cursorPadding1 ? 0xaabbccdd_rgbaf : 0xffffffff_rgbaf
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(layer.dynamicEditingStylePaddings(), Containers::arrayView({
        Vector4{0.0f},
        Vector4{0.0f},
        data.selectionPadding1 ? *data.selectionPadding1 : Vector4{0.0f},
        data.cursorPadding1 ? *data.cursorPadding1 : Vector4{0.0f}
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate);
    CORRADE_VERIFY(layer.stateData().dynamicStyleChanged);
    CORRADE_VERIFY(layer.stateData().dynamicEditingStyleChanged);

    /* Clear the state flags. In order to call update() we need to call
       setStyle() and setEditingStyle() tho, to not trip on asserts */
    /** @todo relax the requirements if no data created? or it's such a corner
        case that it doesn't matter? */
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}, TextLayerStyleUniform{}, TextLayerStyleUniform{}},
        {0},
        {FontHandle::Null},
        {Text::Alignment{}},
        {}, {}, {}, {}, {}, {});
    shared.setEditingStyle(TextLayerCommonEditingStyleUniform{},
        {},
        {},
        {});
    layer.update(LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Setting a style again sets LayerState only if actually needed */
    layer.stateData().dynamicStyleChanged = false;
    layer.stateData().dynamicEditingStyleChanged = false;
    if(data.cursorPadding2 && data.selectionPadding2)
        layer.setDynamicStyleWithCursorSelection(1,
            TextLayerStyleUniform{}
                .setColor(0x112233_rgbf),
            FontHandle::Null,
            Text::Alignment::MiddleCenter, {}, data.padding2,
            TextLayerEditingStyleUniform{}
                .setBackgroundColor(0xaabbccdd_rgbaf)
                .setCornerRadius(4.0f),
            *data.cursorPadding2,
            TextLayerEditingStyleUniform{}
                .setBackgroundColor(0xeeff9966_rgbaf)
                .setCornerRadius(1.5f),
            data.textUniform2 ?
                TextLayerStyleUniform{}
                    .setColor(0x663399_rgbf) :
                Containers::Optional<TextLayerStyleUniform>{},
            *data.selectionPadding2);
    else if(data.cursorPadding2)
        layer.setDynamicStyleWithCursor(1,
            TextLayerStyleUniform{}
                .setColor(0x112233_rgbf),
            FontHandle::Null,
            Text::Alignment::MiddleCenter, {}, data.padding2,
            TextLayerEditingStyleUniform{}
                .setBackgroundColor(0xaabbccdd_rgbaf)
                .setCornerRadius(4.0f),
            *data.cursorPadding2);
    else if(data.selectionPadding2)
        layer.setDynamicStyleWithSelection(1,
            TextLayerStyleUniform{}
                .setColor(0x112233_rgbf),
            FontHandle::Null,
            Text::Alignment::MiddleCenter, {}, data.padding2,
            TextLayerEditingStyleUniform{}
                .setBackgroundColor(0xeeff9966_rgbaf)
                .setCornerRadius(1.5f),
            data.textUniform2 ?
                TextLayerStyleUniform{}
                    .setColor(0x663399_rgbf) :
                Containers::Optional<TextLayerStyleUniform>{},
            *data.selectionPadding2);
    else
        layer.setDynamicStyle(1,
            TextLayerStyleUniform{}
                .setColor(0x112233_rgbf),
            FontHandle::Null,
            Text::Alignment::MiddleCenter, {}, data.padding2);
    CORRADE_COMPARE_AS(stridedArrayView(layer.dynamicStyleUniforms()).slice(&TextLayerStyleUniform::color), Containers::arrayView({
        0xffffffff_rgbaf,
        0x112233ff_rgbaf,
        /* Each editing style adds two more uniforms, one for selected text and
           one reserved for an overwriting cursor */
        0xffffffff_rgbaf,
        0xffffffff_rgbaf,
        /* If text uniform isn't set, the base uniform value is copied there,
           unless it doesn't include a selection style. Then whatever value was
           there before is left. */
        /** @todo a bit crazy cascade here, eh */
        data.textUniform2 ? 0x663399ff_rgbaf :
            data.selectionPadding2 ? 0x112233ff_rgbaf :
                data.textUniform1 ? 0x11223344_rgbaf :
                    data.selectionPadding1 ? 0xaabbcc_rgbf : 0xffffffff_rgbaf,
        0xffffffff_rgbaf,
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(layer.dynamicStyleCursorStyles(), Containers::stridedArrayView({
        false, !!data.cursorPadding2
    }).sliceBit(0), TestSuite::Compare::Container);
    CORRADE_COMPARE(layer.dynamicStyleCursorStyle(0), -1);
    CORRADE_COMPARE(layer.dynamicStyleCursorStyle(1), data.cursorPadding2 ? 3 : -1);
    CORRADE_COMPARE_AS(layer.dynamicStyleSelectionStyles(), Containers::stridedArrayView({
        false, !!data.selectionPadding2
    }).sliceBit(0), TestSuite::Compare::Container);
    CORRADE_COMPARE(layer.dynamicStyleSelectionStyle(0), -1);
    CORRADE_COMPARE(layer.dynamicStyleSelectionStyle(1), data.selectionPadding2 ? 2 : -1);
    CORRADE_COMPARE(layer.dynamicStyleSelectionStyleTextUniform(0), -1);
    CORRADE_COMPARE(layer.dynamicStyleSelectionStyleTextUniform(1), data.selectionPadding2 ? 4 : -1);
    CORRADE_COMPARE_AS(stridedArrayView(layer.dynamicEditingStyleUniforms()).slice(&TextLayerEditingStyleUniform::backgroundColor), Containers::arrayView({
        0xffffffff_rgbaf,
        0xffffffff_rgbaf,
        data.selectionPadding2 ? 0xeeff9966_rgbaf : 0xffffffff_rgbaf,
        data.cursorPadding2 ? 0xaabbccdd_rgbaf : 0xffffffff_rgbaf
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(layer.dynamicEditingStylePaddings(), Containers::arrayView({
        Vector4{0.0f},
        Vector4{0.0f},
        data.selectionPadding2 ? *data.selectionPadding2 : Vector4{0.0f},
        data.cursorPadding2 ? *data.cursorPadding2 : Vector4{0.0f}
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE(layer.state(), data.expectedStates);
    CORRADE_VERIFY(layer.stateData().dynamicStyleChanged);
    /* Editing style changes only if the second style is with the cursor or
       selection part */
    CORRADE_COMPARE(layer.stateData().dynamicEditingStyleChanged, data.cursorPadding2 || data.selectionPadding2);
}

void TextLayerTest::dynamicStyleNoDynamicStyles() {
    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32}};

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{12, 2}};

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    CORRADE_COMPARE(layer.dynamicStyleUniforms().size(), 0);
    CORRADE_COMPARE(layer.dynamicStyleFonts().size(), 0);
    CORRADE_COMPARE(layer.dynamicStyleAlignments().size(), 0);
    CORRADE_COMPARE(layer.dynamicStylePaddings().size(), 0);
}

void TextLayerTest::dynamicStyleInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32}};

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{12, 7}
        /* Making sure it's less than both style count and uniform count to
           verify it's not checked against those */
        .setDynamicStyleCount(3, true)
    }, sharedNoEditingStyles{cache, TextLayer::Shared::Configuration{12, 7}
        .setDynamicStyleCount(1, false)
    };

    CORRADE_VERIFY(shared.hasEditingStyles());
    CORRADE_VERIFY(!sharedNoEditingStyles.hasEditingStyles());

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared},
        layerNoEditingStyles{layerHandle(0, 1), sharedNoEditingStyles};

    /* Using a null font handle is fine */
    layer.setDynamicStyle(2, TextLayerStyleUniform{}, FontHandle::Null, Text::Alignment::MiddleCenter, {}, {});
    /* Calling setDynamicStyle() on a layer w/o editing styles is fine */
    layerNoEditingStyles.setDynamicStyle(0, TextLayerStyleUniform{}, FontHandle::Null, Text::Alignment::MiddleCenter, {}, {});

    Containers::String out;
    Error redirectError{&out};
    layer.dynamicStyleFeatures(3);
    layer.dynamicStyleCursorStyle(3);
    layer.dynamicStyleSelectionStyle(3);
    layer.dynamicStyleSelectionStyleTextUniform(3);
    layer.setDynamicStyle(3, TextLayerStyleUniform{}, FontHandle::Null, Text::Alignment::MiddleCenter, {}, {});
    layer.setDynamicStyleWithCursorSelection(3, TextLayerStyleUniform{}, FontHandle::Null, Text::Alignment::MiddleCenter, {}, {}, TextLayerEditingStyleUniform{}, {}, TextLayerEditingStyleUniform{}, {}, {});
    layer.setDynamicStyleWithCursor(3, TextLayerStyleUniform{}, FontHandle::Null, Text::Alignment::MiddleCenter, {}, {}, TextLayerEditingStyleUniform{}, {});
    layer.setDynamicStyleWithSelection(3, TextLayerStyleUniform{}, FontHandle::Null, Text::Alignment::MiddleCenter, {}, {}, TextLayerEditingStyleUniform{}, {}, {});
    layer.setDynamicStyle(2, TextLayerStyleUniform{}, FontHandle(0x12ab), Text::Alignment::MiddleCenter, {}, {});
    layer.setDynamicStyleWithCursorSelection(2, TextLayerStyleUniform{}, FontHandle(0x12ab), Text::Alignment::MiddleCenter, {}, {}, TextLayerEditingStyleUniform{}, {}, TextLayerEditingStyleUniform{}, {}, {});
    layer.setDynamicStyleWithCursor(2, TextLayerStyleUniform{}, FontHandle(0x12ab), Text::Alignment::MiddleCenter, {}, {}, TextLayerEditingStyleUniform{}, {});
    layer.setDynamicStyleWithSelection(2, TextLayerStyleUniform{}, FontHandle(0x12ab), Text::Alignment::MiddleCenter, {}, {}, TextLayerEditingStyleUniform{}, {}, {});
    layer.setDynamicStyle(2, TextLayerStyleUniform{}, FontHandle::Null, Text::Alignment::BottomCenterGlyphBounds, {}, {});
    layer.setDynamicStyleWithCursorSelection(2, TextLayerStyleUniform{}, FontHandle::Null, Text::Alignment::BottomCenterGlyphBounds, {}, {}, TextLayerEditingStyleUniform{}, {}, TextLayerEditingStyleUniform{}, {}, {});
    layer.setDynamicStyleWithCursor(2, TextLayerStyleUniform{}, FontHandle::Null, Text::Alignment::BottomCenterGlyphBounds, {}, {}, TextLayerEditingStyleUniform{}, {});
    layer.setDynamicStyleWithSelection(2, TextLayerStyleUniform{}, FontHandle::Null, Text::Alignment::BottomCenterGlyphBounds, {}, {}, TextLayerEditingStyleUniform{}, {}, {});
    layerNoEditingStyles.setDynamicStyleWithCursorSelection(0, TextLayerStyleUniform{}, FontHandle::Null, Text::Alignment::MiddleCenter, {}, {}, TextLayerEditingStyleUniform{}, {}, TextLayerEditingStyleUniform{}, {}, {});
    layerNoEditingStyles.setDynamicStyleWithCursor(0, TextLayerStyleUniform{}, FontHandle::Null, Text::Alignment::MiddleCenter, {}, {}, TextLayerEditingStyleUniform{}, {});
    layerNoEditingStyles.setDynamicStyleWithSelection(0, TextLayerStyleUniform{}, FontHandle::Null, Text::Alignment::MiddleCenter, {}, {}, TextLayerEditingStyleUniform{}, {}, {});
    CORRADE_COMPARE_AS(out,
        "Ui::TextLayer::dynamicStyleFeatures(): index 3 out of range for 3 dynamic styles\n"
        "Ui::TextLayer::dynamicStyleCursorStyle(): index 3 out of range for 3 dynamic styles\n"
        "Ui::TextLayer::dynamicStyleSelectionStyle(): index 3 out of range for 3 dynamic styles\n"
        "Ui::TextLayer::dynamicStyleSelectionStyleTextUniform(): index 3 out of range for 3 dynamic styles\n"
        "Ui::TextLayer::setDynamicStyle(): index 3 out of range for 3 dynamic styles\n"
        "Ui::TextLayer::setDynamicStyleWithCursorSelection(): index 3 out of range for 3 dynamic styles\n"
        "Ui::TextLayer::setDynamicStyleWithCursor(): index 3 out of range for 3 dynamic styles\n"
        "Ui::TextLayer::setDynamicStyleWithSelection(): index 3 out of range for 3 dynamic styles\n"
        "Ui::TextLayer::setDynamicStyle(): invalid handle Ui::FontHandle(0x12ab, 0x0)\n"
        "Ui::TextLayer::setDynamicStyleWithCursorSelection(): invalid handle Ui::FontHandle(0x12ab, 0x0)\n"
        "Ui::TextLayer::setDynamicStyleWithCursor(): invalid handle Ui::FontHandle(0x12ab, 0x0)\n"
        "Ui::TextLayer::setDynamicStyleWithSelection(): invalid handle Ui::FontHandle(0x12ab, 0x0)\n"
        "Ui::TextLayer::setDynamicStyle(): Text::Alignment::BottomCenterGlyphBounds is not supported\n"
        "Ui::TextLayer::setDynamicStyleWithCursorSelection(): Text::Alignment::BottomCenterGlyphBounds is not supported\n"
        "Ui::TextLayer::setDynamicStyleWithCursor(): Text::Alignment::BottomCenterGlyphBounds is not supported\n"
        "Ui::TextLayer::setDynamicStyleWithSelection(): Text::Alignment::BottomCenterGlyphBounds is not supported\n"
        /* setDynamicStyleWithCursorSelection() calls two helpers, of which
           each prints the message without aborting, so it's twice */
        "Ui::TextLayer::setDynamicStyleWithCursorSelection(): editing styles are not enabled\n"
        "Ui::TextLayer::setDynamicStyleWithCursorSelection(): editing styles are not enabled\n"
        "Ui::TextLayer::setDynamicStyleWithCursor(): editing styles are not enabled\n"
        "Ui::TextLayer::setDynamicStyleWithSelection(): editing styles are not enabled\n",
        TestSuite::Compare::String);
}

struct ThreeGlyphShaper: Text::AbstractShaper {
    explicit ThreeGlyphShaper(Text::AbstractFont& font, Text::ShapeDirection direction = Text::ShapeDirection::Unspecified): Text::AbstractShaper{font}, _direction{direction}, _constructedDirection{direction} {}

    UnsignedInt doShape(Containers::StringView, UnsignedInt begin, UnsignedInt end, Containers::ArrayView<const Text::FeatureRange>) override {
        _begin = begin;
        return end - begin;
    }
    bool doSetDirection(Text::ShapeDirection direction) override {
        /* The direction should be either specified in the constructor (in the
           editText() test case) or from TextProperties (in
           updateCleanDataOrder()), never both */
        if(_constructedDirection == Text::ShapeDirection::Unspecified) {
            _direction = direction;
            return true;
        } else {
            CORRADE_INTERNAL_ASSERT(direction == Text::ShapeDirection::Unspecified);
            return false;
        }
    }
    Text::ShapeDirection doDirection() const override {
        return _direction;
    }
    void doGlyphIdsInto(const Containers::StridedArrayView1D<UnsignedInt>& ids) const override {
        /* Just cycling through three glyphs */
        for(std::size_t i = 0; i != ids.size(); ++i) {
            if((_begin + i) % 3 == 0)
                ids[i] = 22;
            else if((_begin + i) % 3 == 1)
                ids[i] = 13;
            else if((_begin + i) % 3 == 2)
                ids[i] = 97;
            else CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        }
    }
    void doGlyphOffsetsAdvancesInto(const Containers::StridedArrayView1D<Vector2>& offsets, const Containers::StridedArrayView1D<Vector2>& advances) const override {
        /* Each next glyph has the advance and offset higher */
        for(std::size_t i = 0; i != offsets.size(); ++i) {
            offsets[i] = {Float(_begin + i), 1.0f + Float(_begin + i)};
            advances[i] = {2.0f + Float(_begin + i), 0.0f};
        }
    }
    void doGlyphClustersInto(const Containers::StridedArrayView1D<UnsignedInt>& clusters) const override {
        /* Just a trivial 1:1 mapping */
        for(std::size_t i = 0; i != clusters.size(); ++i)
            clusters[i] = _begin + i;
    }

    private:
        UnsignedInt _begin;
        Text::ShapeDirection _direction, _constructedDirection;
};

struct OneGlyphShaper: Text::AbstractShaper {
    using Text::AbstractShaper::AbstractShaper;

    UnsignedInt doShape(Containers::StringView text, UnsignedInt, UnsignedInt, Containers::ArrayView<const Text::FeatureRange>) override {
        _textSize = text.size();
        return 1;
    }
    void doGlyphIdsInto(const Containers::StridedArrayView1D<UnsignedInt>& ids) const override {
        ids[0] = 66;
    }
    void doGlyphOffsetsAdvancesInto(const Containers::StridedArrayView1D<Vector2>& offsets, const Containers::StridedArrayView1D<Vector2>& advances) const override {
        offsets[0] = {1.5f, -0.5f};
        advances[0] = {2.5f, 0.0f};
    }
    void doGlyphClustersInto(const Containers::StridedArrayView1D<UnsignedInt>& clusters) const override {
        /* In this case all clusters are pointing to the last byte of text */
        for(std::size_t i = 0; i != clusters.size(); ++i)
            clusters[i] = Math::max(_textSize - 1, 0);
    }

    private:
        Int _textSize = 0;
};

template<class StyleIndex, class GlyphIndex> void TextLayerTest::createRemoveSet() {
    auto&& data = CreateRemoveSetData[testCaseInstanceId()];
    setTestCaseDescription(data.name);
    setTestCaseTemplateName({
        std::is_same<StyleIndex, Enum>::value ? "Enum" : "UnsignedInt",
        std::is_same<GlyphIndex, Enum>::value ? "Enum" : "UnsignedInt"
    });

    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return _opened; }
        Properties doOpenFile(Containers::StringView, Float size) override {
            _opened = true;
            return {size, 8.0f, -4.0f, 16.0f, 98};
        }
        void doClose() override { _opened = false; }

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return Containers::pointer<ThreeGlyphShaper>(*this); }

        bool _opened = false;
    } threeGlyphFont;
    threeGlyphFont.openFile({}, 16.0f);

    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return _opened; }
        Properties doOpenFile(Containers::StringView, Float size) override {
            _opened = true;
            return {size, 1.0f, -0.5f, 2.0f, 67};
        }
        void doClose() override { _opened = false; }

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return Containers::pointer<OneGlyphShaper>(*this); }

        bool _opened = false;
    } oneGlyphFont;
    oneGlyphFont.openFile({}, 2.0f);

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    /* Default padding is 1, resetting to 0 for simplicity */
    } cache{PixelFormat::R8Unorm, {32, 32, 15}, {}};

    /* Glyph rectangle sizes in the glyph cache are only used for single-glyph
       data, text uses just the glyph ID mapping. Sizes, layers and offsets are
       only used in doUpdate() so they can be arbitrary. */
    cache.setInvalidGlyph({4, -2}, 7, {{16, 8}, {32, 32}});
    {
        UnsignedInt fontId = cache.addFont(threeGlyphFont.glyphCount(), &threeGlyphFont);
        cache.addGlyph(fontId, 97, {3000, 1000}, 13, {{7, 23}, {18, 30}});
        /* Glyph 22 deliberately omitted */
        cache.addGlyph(fontId, 13, {2, -4}, 6, {{8, 16}, {32, 32}});
    } {
        UnsignedInt fontId = cache.addFont(oneGlyphFont.glyphCount(), &oneGlyphFont);
        cache.addGlyph(fontId, 66, {2, -1}, 9, {{7, 8}, {15, 20}});
    }

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{data.styleCount}
        .setDynamicStyleCount(data.dynamicStyleCount)
    };

    /* The three-glyph font is scaled to 0.5, the one-glyph to 2.0 */
    FontHandle threeGlyphFontHandle = shared.addFont(threeGlyphFont, 8.0f);
    FontHandle oneGlyphFontHandle = shared.addFont(oneGlyphFont, 4.0f);

    /* If using custom fonts, set the style to either something completely
       different or not set them at all -- they shouldn't get used for
       anything. Text features tested in createSetTextTextProperties() instead,
       padding from the style in setPadding() instead, effect of the
       style->uniform mapping in updateCleanDataOrder() instead, here they're
       all implicit.

       Similarly, if using custom alignment, set the style to something
       completely different to verify the custom values get picked instead. */
    TextLayerStyleUniform uniforms[3];
    FontHandle fonts[3];
    if(!data.customFont)
        Utility::copy({threeGlyphFontHandle, threeGlyphFontHandle, oneGlyphFontHandle}, fonts);
    else if(data.nullStyleFonts)
        Utility::copy({FontHandle::Null, FontHandle::Null, FontHandle::Null}, fonts);
    else
        Utility::copy({oneGlyphFontHandle, oneGlyphFontHandle, threeGlyphFontHandle}, fonts);
    Text::Alignment alignment[3];
    if(!data.customAlignment)
        Utility::copy({Text::Alignment::LineLeft, Text::Alignment::MiddleCenter, Text::Alignment::BottomRight}, alignment);
    else
        Utility::copy({Text::Alignment::TopRight, Text::Alignment::BottomLeft, Text::Alignment::MiddleCenter}, alignment);
    shared.setStyle(TextLayerCommonStyleUniform{},
        Containers::arrayView(uniforms).prefix(data.styleCount),
        Containers::arrayView(fonts).prefix(data.styleCount),
        Containers::arrayView(alignment).prefix(data.styleCount),
        {}, {}, {},
        /* Right now, create(), setText() etc doesn't do anything with editing
           styles, only update() does, so they don't need to be present at
           all */
        {}, {},
        {});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared, TextLayerFlags flags): TextLayer{handle, shared, flags} {}

        const State& stateData() const {
            return static_cast<const State&>(*_state);
        }
    } layer{layerHandle(0, 1), shared, data.layerFlags};

    if(data.dynamicStyleCount == 2) {
        /* If custom alignment specified in TextProperties is used, these have
           it both set to a bogus value */
        layer.setDynamicStyle(0, TextLayerStyleUniform{}, threeGlyphFontHandle,
            data.customAlignment ? Text::Alignment::LineLeft : Text::Alignment::MiddleCenter, {}, {});
        layer.setDynamicStyle(1, TextLayerStyleUniform{}, oneGlyphFontHandle,
            data.customAlignment ? Text::Alignment::MiddleCenter : Text::Alignment::BottomRight, {}, {});
    } else CORRADE_INTERNAL_ASSERT(data.dynamicStyleCount == 0);

    DataHandle first = data.flags ?
        layer.create(
            StyleIndex(1),
            "hello",
            TextProperties{}
                .setFont(data.customFont ? threeGlyphFontHandle : FontHandle::Null)
                .setAlignment(data.customAlignment ? Containers::optional(Text::Alignment::MiddleCenter) : Containers::NullOpt),
            *data.flags,
            data.node) :
        layer.create(
            StyleIndex(1),
            "hello",
            TextProperties{}
                .setFont(data.customFont ? threeGlyphFontHandle : FontHandle::Null)
                .setAlignment(data.customAlignment ? Containers::optional(Text::Alignment::MiddleCenter) : Containers::NullOpt),
            data.node);
    CORRADE_COMPARE(layer.node(first), data.node);
    CORRADE_COMPARE(layer.style(first), 1);
    CORRADE_COMPARE(layer.flags(first), data.flags ? *data.flags : TextDataFlags{});
    CORRADE_COMPARE(layer.glyphCount(first), 5);
    CORRADE_COMPARE(layer.size(first), (Vector2{10.0f, 6.0f}));
    if(data.flags && *data.flags >= TextDataFlag::Editable) {
        CORRADE_COMPARE(layer.cursor(first), Containers::pair(5u, 5u));
        /* textProperties() tested in createSetTextTextPropertiesEditable() */
        CORRADE_COMPARE(layer.text(first), "hello");
    }
    CORRADE_COMPARE(layer.color(first), 0xffffff_rgbf);
    if(data.layerFlags >= TextLayerFlag::Transformable)
        CORRADE_COMPARE(layer.transformation(first), Containers::pair(Vector2{}, Complex{Math::IdentityInit}));
    else
        CORRADE_COMPARE(layer.padding(first), Vector4{0.0f});
    CORRADE_COMPARE(layer.state(), data.state);

    /* Single (invalid) glyph; createGlyph() takes no TextDataFlags */
    DataHandle firstGlyph = layer.createGlyph(
        StyleIndex(1),
        GlyphIndex(22),
        TextProperties{}
            .setFont(data.customFont ? threeGlyphFontHandle : FontHandle::Null)
            .setAlignment(data.customAlignment ? Containers::optional(Text::Alignment::MiddleCenter) : Containers::NullOpt),
        data.node);
    CORRADE_COMPARE(layer.node(firstGlyph), data.node);
    CORRADE_COMPARE(layer.style(firstGlyph), 1);
    CORRADE_COMPARE(layer.flags(firstGlyph), TextDataFlags{});
    CORRADE_COMPARE(layer.glyphCount(firstGlyph), 1);
    CORRADE_COMPARE(layer.size(firstGlyph), (Vector2{8.0f, 12.0f}));
    CORRADE_COMPARE(layer.color(firstGlyph), 0xffffff_rgbf);
    if(data.layerFlags >= TextLayerFlag::Transformable)
        CORRADE_COMPARE(layer.transformation(firstGlyph), Containers::pair(Vector2{}, Complex{Math::IdentityInit}));
    else
        CORRADE_COMPARE(layer.padding(firstGlyph), Vector4{0.0f});
    CORRADE_COMPARE(layer.state(), data.state);

    /* Default null node, testing also the getter overloads and templates;
       having a non-default alignment */
    DataHandle second = data.flags ?
        layer.create(
            StyleIndex(2),
            "ahoy",
            TextProperties{}
                .setFont(data.customFont ? oneGlyphFontHandle : FontHandle::Null)
                .setAlignment(data.customAlignment ? Containers::optional(Text::Alignment::BottomRight) : Containers::NullOpt),
            *data.flags) :
        layer.create(
            StyleIndex(2),
            "ahoy",
            TextProperties{}
                .setFont(data.customFont ? oneGlyphFontHandle : FontHandle::Null)
                .setAlignment(data.customAlignment ? Containers::optional(Text::Alignment::BottomRight) : Containers::NullOpt));
    CORRADE_COMPARE(layer.node(second), NodeHandle::Null);
    if(data.layerDataHandleOverloads) {
        CORRADE_COMPARE(layer.style(dataHandleData(second)), 2);
        /* Can't use StyleIndex, as the function restricts to enum types which
           would fail for StyleIndex == UnsignedInt */
        CORRADE_COMPARE(layer.template style<Enum>(dataHandleData(second)), Enum(2));
        CORRADE_COMPARE(layer.flags(dataHandleData(second)), data.flags ? *data.flags : TextDataFlags{});
        CORRADE_COMPARE(layer.glyphCount(dataHandleData(second)), 1);
        CORRADE_COMPARE(layer.size(dataHandleData(second)), (Vector2{5.0f, 3.0f}));
        if(data.flags && *data.flags >= TextDataFlag::Editable) {
            CORRADE_COMPARE(layer.cursor(dataHandleData(second)), Containers::pair(4u, 4u));
            /* textProperties() tested in createSetTextTextPropertiesEditable() */
            CORRADE_COMPARE(layer.text(dataHandleData(second)), "ahoy");
        }
        CORRADE_COMPARE(layer.color(dataHandleData(second)), 0xffffff_rgbf);
        if(data.layerFlags >= TextLayerFlag::Transformable)
            CORRADE_COMPARE(layer.transformation(dataHandleData(second)), Containers::pair(Vector2{}, Complex{Math::IdentityInit}));
        else
            CORRADE_COMPARE(layer.padding(dataHandleData(second)), Vector4{0.0f});
    } else {
        CORRADE_COMPARE(layer.style(second), 2);
        /* Can't use StyleIndex, as the function restricts to enum types which
           would fail for StyleIndex == UnsignedInt */
        CORRADE_COMPARE(layer.template style<Enum>(second), Enum(2));
        CORRADE_COMPARE(layer.flags(second), data.flags ? *data.flags : TextDataFlags{});
        CORRADE_COMPARE(layer.glyphCount(second), 1);
        CORRADE_COMPARE(layer.size(second), (Vector2{5.0f, 3.0f}));
        if(data.flags && *data.flags >= TextDataFlag::Editable) {
            CORRADE_COMPARE(layer.cursor(second), Containers::pair(4u, 4u));
            /* textProperties() tested in createSetTextTextPropertiesEditable() */
            CORRADE_COMPARE(layer.text(second), "ahoy");
        }
        CORRADE_COMPARE(layer.color(second), 0xffffff_rgbf);
        if(data.layerFlags >= TextLayerFlag::Transformable)
            CORRADE_COMPARE(layer.transformation(second), Containers::pair(Vector2{}, Complex{Math::IdentityInit}));
        else
            CORRADE_COMPARE(layer.padding(second), Vector4{0.0f});
    }
    CORRADE_COMPARE(layer.state(), data.state);

    /* Single glyph with default node; again createGlyph() takes no
       TextDataFlags */
    DataHandle secondGlyph = layer.createGlyph(
        StyleIndex(2),
        GlyphIndex(66),
        TextProperties{}
            .setFont(data.customFont ? oneGlyphFontHandle : FontHandle::Null)
            .setAlignment(data.customAlignment ? Containers::optional(Text::Alignment::BottomRight) : Containers::NullOpt),
        data.node);
    CORRADE_COMPARE(layer.node(secondGlyph), data.node);
    CORRADE_COMPARE(layer.style(secondGlyph), 2);
    CORRADE_COMPARE(layer.flags(secondGlyph), TextDataFlags{});
    CORRADE_COMPARE(layer.glyphCount(secondGlyph), 1);
    CORRADE_COMPARE(layer.size(secondGlyph), (Vector2{16.0f, 24.0f}));
    CORRADE_COMPARE(layer.color(secondGlyph), 0xffffff_rgbf);
    if(data.layerFlags >= TextLayerFlag::Transformable)
        CORRADE_COMPARE(layer.transformation(secondGlyph), Containers::pair(Vector2{}, Complex{Math::IdentityInit}));
    else
        CORRADE_COMPARE(layer.padding(secondGlyph), Vector4{0.0f});
    CORRADE_COMPARE(layer.state(), data.state);

    /* Empty text */
    DataHandle third = data.flags ?
        layer.create(
            StyleIndex(1),
            "",
            TextProperties{}
                .setFont(data.customFont ? threeGlyphFontHandle : FontHandle::Null)
                .setAlignment(data.customAlignment ? Containers::optional(Text::Alignment::MiddleCenter) : Containers::NullOpt),
            *data.flags,
            data.node) :
        layer.create(
            StyleIndex(1),
            "",
            TextProperties{}
                .setFont(data.customFont ? threeGlyphFontHandle : FontHandle::Null)
                .setAlignment(data.customAlignment ? Containers::optional(Text::Alignment::MiddleCenter) : Containers::NullOpt),
            data.node);
    CORRADE_COMPARE(layer.node(third), data.node);
    CORRADE_COMPARE(layer.style(third), 1);
    CORRADE_COMPARE(layer.flags(third), data.flags ? *data.flags : TextDataFlags{});
    CORRADE_COMPARE(layer.glyphCount(third), 0);
    /* glyphCount() is special-cased for data that have no glyphs, verify that
       the LayerDataHandle overload works properly too */
    CORRADE_COMPARE(layer.glyphCount(dataHandleData(third)), 0);
    CORRADE_COMPARE(layer.size(third), (Vector2{0.0f, 6.0f}));
    if(data.flags && *data.flags >= TextDataFlag::Editable) {
        CORRADE_COMPARE(layer.cursor(third), Containers::pair(0u, 0u));
        /* textProperties() tested in createSetTextTextPropertiesEditable() */
        CORRADE_COMPARE(layer.text(third), "");
    }
    CORRADE_COMPARE(layer.color(third), 0xffffff_rgbf);
    if(data.layerFlags >= TextLayerFlag::Transformable)
        CORRADE_COMPARE(layer.transformation(third), Containers::pair(Vector2{}, Complex{Math::IdentityInit}));
    else
        CORRADE_COMPARE(layer.padding(third), Vector4{0.0f});
    CORRADE_COMPARE(layer.state(), data.state);

    DataHandle fourth = data.flags ?
        layer.create(
            StyleIndex(0),
            "hi",
            TextProperties{}
                .setFont(data.customFont ? threeGlyphFontHandle : FontHandle::Null)
                .setAlignment(data.customAlignment ? Containers::optional(Text::Alignment::LineLeft) : Containers::NullOpt),
            *data.flags,
            data.node) :
        layer.create(
            StyleIndex(0),
            "hi",
            TextProperties{}
                .setFont(data.customFont ? threeGlyphFontHandle : FontHandle::Null)
                .setAlignment(data.customAlignment ? Containers::optional(Text::Alignment::LineLeft) : Containers::NullOpt),
            data.node);
    CORRADE_COMPARE(layer.node(fourth), data.node);
    CORRADE_COMPARE(layer.style(fourth), 0);
    CORRADE_COMPARE(layer.flags(fourth), data.flags ? *data.flags : TextDataFlags{});
    CORRADE_COMPARE(layer.glyphCount(fourth), 2);
    CORRADE_COMPARE(layer.size(fourth), (Vector2{2.5f, 6.0f}));
    if(data.flags && *data.flags >= TextDataFlag::Editable) {
        CORRADE_COMPARE(layer.cursor(fourth), Containers::pair(2u, 2u));
        /* textProperties() tested in createSetTextTextPropertiesEditable() */
        CORRADE_COMPARE(layer.text(fourth), "hi");
    }
    CORRADE_COMPARE(layer.color(fourth), 0xffffff_rgbf);
    if(data.layerFlags >= TextLayerFlag::Transformable)
        CORRADE_COMPARE(layer.transformation(fourth), Containers::pair(Vector2{}, Complex{Math::IdentityInit}));
    else
        CORRADE_COMPARE(layer.padding(fourth), Vector4{0.0f});
    CORRADE_COMPARE(layer.state(), data.state);

    /* Empty text that is never editable and thus has neither a glyph nor a
       text run */
    DataHandle fifth = layer.create(StyleIndex(1), "",
        TextProperties{}
            .setFont(data.customFont ? threeGlyphFontHandle : FontHandle::Null)
            .setAlignment(data.customAlignment ? Containers::optional(Text::Alignment::MiddleCenter) : Containers::NullOpt),
        data.node);
    CORRADE_COMPARE(layer.node(fifth), data.node);
    CORRADE_COMPARE(layer.style(fifth), 1);
    CORRADE_COMPARE(layer.flags(fifth), TextDataFlags{});
    CORRADE_COMPARE(layer.glyphCount(fifth), 0);
    CORRADE_COMPARE(layer.size(fifth), (Vector2{0.0f, 6.0f}));
    CORRADE_COMPARE(layer.color(fifth), 0xffffff_rgbf);
    if(data.layerFlags >= TextLayerFlag::Transformable)
        CORRADE_COMPARE(layer.transformation(fifth), Containers::pair(Vector2{}, Complex{Math::IdentityInit}));
    else
        CORRADE_COMPARE(layer.padding(fifth), Vector4{0.0f});
    CORRADE_COMPARE(layer.state(), data.state);

    /* Another empty text, again also editable */
    DataHandle sixth = data.flags ?
        layer.create(
            StyleIndex(1),
            "",
            TextProperties{}
                .setFont(data.customFont ? threeGlyphFontHandle : FontHandle::Null)
                .setAlignment(data.customAlignment ? Containers::optional(Text::Alignment::MiddleCenter) : Containers::NullOpt),
            *data.flags,
            data.node) :
        layer.create(
            StyleIndex(1),
            "",
            TextProperties{}
                .setFont(data.customFont ? threeGlyphFontHandle : FontHandle::Null)
                .setAlignment(data.customAlignment ? Containers::optional(Text::Alignment::MiddleCenter) : Containers::NullOpt),
            data.node);
    CORRADE_COMPARE(layer.node(sixth), data.node);
    CORRADE_COMPARE(layer.style(sixth), 1);
    CORRADE_COMPARE(layer.flags(sixth), data.flags ? *data.flags : TextDataFlags{});
    CORRADE_COMPARE(layer.glyphCount(sixth), 0);
    CORRADE_COMPARE(layer.size(sixth), (Vector2{0.0f, 6.0f}));
    if(data.flags && *data.flags >= TextDataFlag::Editable) {
        CORRADE_COMPARE(layer.cursor(sixth), Containers::pair(0u, 0u));
        CORRADE_COMPARE(layer.text(sixth), "");
    }
    CORRADE_COMPARE(layer.color(sixth), 0xffffff_rgbf);
    if(data.layerFlags >= TextLayerFlag::Transformable)
        CORRADE_COMPARE(layer.transformation(sixth), Containers::pair(Vector2{}, Complex{Math::IdentityInit}));
    else
        CORRADE_COMPARE(layer.padding(sixth), Vector4{0.0f});
    CORRADE_COMPARE(layer.state(), data.state);

    /* There should be five glyph runs, assigned to data that resulted in
       non-zero glyphs */
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().data).slice(&Implementation::TextLayerData::glyphRun), Containers::arrayView({
        0u, 1u, 2u, 3u, 0xffffffffu, 4u, 0xffffffffu, 0xffffffffu
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::glyphOffset), Containers::arrayView({
        /* The second and third data is a single glyph, `second` text is using
           the OneGlyphShaper, so it's just one glyph; `third`, `fifth`,
           `sixth` are empty so they have no run */
        0u, 5u, 6u, 7u, 8u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::glyphCount), Containers::arrayView({
        /* The second and third data is a single glyph, `second` text is using
           the OneGlyphShaper, so it's just one glyph; `third`, `fifth`,
           `sixth` are empty so they have no run */
        5u, 1u, 1u, 1u, 2u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::data), Containers::arrayView({
        0u, 1u, 2u, 3u, 5u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphData).slice(&Implementation::TextLayerGlyphData::glyphId), Containers::arrayView({
        /* Glyphs 22, 13, 97, 22, 13; glyph 22 isn't in the cache */
        0u, 2u, 1u, 0u, 2u,
        /* Single (invalid) glyph 22 */
        0u,
        /* Glyph 66 */
        3u,
        /* Single glyph 66 */
        3u,
        /* Glyphs 22, 13 */
        0u, 2u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphData).slice(&Implementation::TextLayerGlyphData::position), Containers::arrayView<Vector2>({
        /* "hello", aligned to MiddleCenter */
        {-5.0f, -0.5f},
        {-3.5f,  0.0f},
        {-1.5f,  0.5f},
        { 1.0f,  1.0f},
        { 4.0f,  1.5f},
        /* Single (invalid) glyph 22. Its size is {16, 24} and offset {4, -2},
           scaled to 0.5, aligned to MiddleCenter */
        {-6.0f, -5.0f},
        /* "ahoy", single glyph, aligned to BottomRight */
        {-2.0f, 0.0f},
        /* Single glyph 66. Its size is {8, 12} and offset {2, -1},
           scaled to 2.0, aligned to BottomRight */
        {-20.0f, 2.0f},
        /* "hi", aligned to LineLeft */
        {0.0f, 0.5f},
        {1.5f, 1.0f}
    }), TestSuite::Compare::Container);

    /* For editable text, there should be also cluster IDs and text runs,
       except for single glyphs that never have it */
    if(data.flags && *data.flags >= TextDataFlag::Editable) {
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphData).slice(&Implementation::TextLayerGlyphData::glyphCluster), Containers::arrayView({
            /* Five glyphs corresponding to five characters */
            0u, 1u, 2u, 3u, 4u,
            /* Single glyph */
            0u,
            /* Single glyph corresponding to four characters */
            3u,
            /* Single glyph */
            0u,
            /* Two glyphs corresponding to two characters */
            0u, 1u
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().data).slice(&Implementation::TextLayerData::textRun), Containers::arrayView({
            /* Glyphs and texts that are never editable have no runs */
            0u, 0xffffffffu, 1u, 0xffffffffu, 2u, 3u, 0xffffffffu, 4u
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().textRuns).slice(&Implementation::TextLayerTextRun::textOffset), Containers::arrayView({
            0u, 5u, 9u, 9u, 11u,
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().textRuns).slice(&Implementation::TextLayerTextRun::textSize), Containers::arrayView({
            5u, 4u, 0u, 2u, 0u
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().textRuns).slice(&Implementation::TextLayerTextRun::data), Containers::arrayView({
            0u, 2u, 4u, 5u, 7u
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(layer.stateData().textData,
            "hello"
            "ahoy"
            ""
            "hi"
            "",
            TestSuite::Compare::String);

    /* And no runs if no text is editable */
    } else {
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().data).slice(&Implementation::TextLayerData::textRun), Containers::arrayView<UnsignedInt>({
            0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu,
            0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().textRuns).slice(&Implementation::TextLayerTextRun::textOffset),
            Containers::ArrayView<UnsignedInt>{},
            TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().textRuns).slice(&Implementation::TextLayerTextRun::textSize),
            Containers::ArrayView<UnsignedInt>{},
            TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().textRuns).slice(&Implementation::TextLayerTextRun::data),
            Containers::ArrayView<UnsignedInt>{},
            TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(layer.stateData().textData,
            "",
            TestSuite::Compare::String);
    }

    /* Removing a text marks the original glyph run (if any) as unused, and if
       it's not attached to any node, doesn't set any state flag. The remaining
       data don't need any refresh, they still draw correctly. */
    data.layerDataHandleOverloads ?
        layer.remove(dataHandleData(fourth)) :
        layer.remove(fourth);
    /* Sixth has no a glyph run but has a text run */
    data.layerDataHandleOverloads ?
        layer.remove(dataHandleData(sixth)) :
        layer.remove(sixth);
    CORRADE_COMPARE(layer.state(), data.state|LayerState::NeedsDataClean);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().data).slice(&Implementation::TextLayerData::glyphRun), Containers::arrayView({
        0u, 1u, 2u, 3u, 0xffffffffu, 4u, 0xffffffffu, 0xffffffffu
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::glyphOffset), Containers::arrayView({
        0u, 5u, 6u, 7u, 0xffffffffu
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::glyphCount), Containers::arrayView({
        5u, 1u, 1u, 1u, 2u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::data), Containers::arrayView({
        0u, 1u, 2u, 3u, 5u
    }), TestSuite::Compare::Container);

    /* Similarly for text runs for editable text, if any */
    if(data.flags && *data.flags >= TextDataFlag::Editable) {
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().data).slice(&Implementation::TextLayerData::textRun), Containers::arrayView({
            0u, 0xffffffffu, 1u, 0xffffffffu, 2u, 3u, 0xffffffffu, 4u
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().textRuns).slice(&Implementation::TextLayerTextRun::textOffset), Containers::arrayView({
            0u, 5u, 9u, 0xffffffffu, 0xffffffffu
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().textRuns).slice(&Implementation::TextLayerTextRun::textSize), Containers::arrayView({
            5u, 4u, 0u, 2u, 0u
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().textRuns).slice(&Implementation::TextLayerTextRun::data), Containers::arrayView({
            0u, 2u, 4u, 5u, 7u
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(layer.stateData().textData,
            "hello"
            "ahoy"
            ""
            "hi" /* now unused */
            "", /* now unused */
            TestSuite::Compare::String);

    /* Nothing changes if no text is editable */
    } else {
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().data).slice(&Implementation::TextLayerData::textRun), Containers::arrayView<UnsignedInt>({
            0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu,
            0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().textRuns).slice(&Implementation::TextLayerTextRun::textOffset),
            Containers::ArrayView<UnsignedInt>{},
            TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().textRuns).slice(&Implementation::TextLayerTextRun::textSize),
            Containers::ArrayView<UnsignedInt>{},
            TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().textRuns).slice(&Implementation::TextLayerTextRun::data),
            Containers::ArrayView<UnsignedInt>{},
            TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(layer.stateData().textData,
            "",
            TestSuite::Compare::String);
    }

    /* Modifying a text creates a new run at the end, marks the original run
       (if any) as unused and marks the layer as needing an update. It's
       possible to switch to a different font, and between a single-glyph and
       text data as well. In this case the `second` text from the one-glyph
       font becomes a single glyph, and `secondGlyph` glyph from the one-glyph
       font becomes a text, and they optionally switch to the three-glyph font
       as well. */
    TextProperties textProperties;
    textProperties
        .setFont(data.customFont ? threeGlyphFontHandle : FontHandle::Null)
        .setAlignment(Text::Alignment::BottomRight);
    CORRADE_COMPARE(layer.flags(second), data.flags ? *data.flags : TextDataFlags{});
    CORRADE_COMPARE(layer.flags(secondGlyph), TextDataFlags{});
    if(data.layerDataHandleOverloads) {
        data.flags ?
            /* Have to pass the flags explicitly because otherwise it'll retain
               the previous (empty) flags */
            layer.setText(dataHandleData(secondGlyph), "hey", textProperties, *data.flags) :
            layer.setText(dataHandleData(secondGlyph), "hey", textProperties);
        layer.setGlyph(dataHandleData(second),
            data.customFont ? GlyphIndex(13) : GlyphIndex(66),
            textProperties);
        /* This changes empty text to a non-empty, i.e. there's no previous
           glyph run to remove. OTOH it makes the text non-editable, so the
           text run is removed without a replacement. Plus it uses a newline
           to verify it's propagated correctly all the way to
           Text::RendererCore and back. */
        layer.setText(dataHandleData(fifth), "a\nh", textProperties, TextDataFlags{});
        /* This changes empty text to a glyph, so again no previous glyph run
           to remove. But it again causes the text run to be removed. */
        layer.setGlyph(dataHandleData(third), GlyphIndex(33), textProperties);
    } else {
        data.flags ?
            layer.setText(secondGlyph, "hey", textProperties, *data.flags) :
            layer.setText(secondGlyph, "hey", textProperties);
        layer.setGlyph(second,
            data.customFont ? GlyphIndex(13) : GlyphIndex(66),
            textProperties);
        layer.setText(fifth, "a\nh", textProperties, TextDataFlags{});
        layer.setGlyph(third, GlyphIndex(33), textProperties);
    }
    CORRADE_COMPARE(layer.flags(second), TextDataFlags{});
    CORRADE_COMPARE(layer.flags(secondGlyph), data.flags ? *data.flags : TextDataFlags{});
    CORRADE_COMPARE(layer.flags(fifth), TextDataFlags{});
    CORRADE_COMPARE(layer.flags(third), TextDataFlags{});

    CORRADE_COMPARE(layer.state(), data.state|LayerState::NeedsDataClean|LayerState::NeedsDataUpdate);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().data).slice(&Implementation::TextLayerData::glyphRun), Containers::arrayView({
        0u, 1u, 6u, 5u, 8u, 4u, 7u, 0xffffffffu
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::glyphCount), Containers::arrayView({
        5u, 1u, 1u, 1u, 2u, data.customFont ? 3u : 1u, 1u, 2u, 1u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::data), Containers::arrayView({
        0u, 1u, 2u, 3u, 5u, 3u, 2u, 6u, 4u
    }), TestSuite::Compare::Container);
    if(data.customFont) {
        CORRADE_COMPARE(layer.glyphCount(secondGlyph), 3);
        CORRADE_COMPARE(layer.glyphCount(second), 1);
        CORRADE_COMPARE(layer.size(secondGlyph), (Vector2{4.5f, 6.0f}));
        CORRADE_COMPARE(layer.size(second), (Vector2{12.0f, 8.0f}));
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::glyphOffset), Containers::arrayView({
            0u, 5u, 0xffffffffu, 0xffffffffu, 0xffffffffu, 10u, 13u, 14u, 16u
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphData).slice(&Implementation::TextLayerGlyphData::glyphId), Containers::arrayView({
            /* Glyphs 22, 13, 97, 22, 13; glyph 22 isn't in the cache */
            0u, 2u, 1u, 0u, 2u,
            /* Single (invalid) glyph 22 */
            0u,
            /* Now-unused "ahoy" text */
            3u,
            /* Now-unused single glyph 66 */
            3u,
            /* Glyphs 22, 13 */
            0u, 2u,
            /* Glyphs 22, 13, 97; glyph 22 isn't in the cache */
            0u, 2u, 1u,
            /* Glyph 13 */
            2u,
            /* Glyphs 22, 97, char in the middle that mapped to 13 is \n */
            0u, 1u,
            /* Single (invalid) glyph 33 */
            0u
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphData).slice(&Implementation::TextLayerGlyphData::position), Containers::arrayView<Vector2>({
            /* "hello", aligned to MiddleCenter */
            {-5.0f, -0.5f},
            {-3.5f,  0.0f},
            {-1.5f,  0.5f},
            { 1.0f,  1.0f},
            { 4.0f,  1.5f},
            /* Single (invalid) glyph 22 */
            {-6.0f, -5.0f},
            /* Now-unused "ahoy" text, aligned to BottomRight */
            {-2.0f, 0.0f},
            /* Now-unused single glyph 66, aligned to BottomRight */
            {-20.0f, 2.0f},
            /* "hi", aligned to LineLeft */
            {0.0f, 0.5f},
            {1.5f, 1.0f},
            /* "hey", aligned to BottomRight */
            {-4.5f, 2.5f},
            {-3.0f, 3.0f},
            {-1.0f, 3.5f},
            /* Single glyph 13. Its size is {24, 16} and offset {2, -4},
               scaled to 0.5, aligned to BottomRight */
            {-13.0f, 2.0f},
            /* "ah", aligned to MiddleCenter */
            {-1.0f, 10.5f},
            {-1.0f, 3.5f},
            /* Single (invalid) glyph 33, MiddleCenter */
            {-10.0f, 1.0f},
        }), TestSuite::Compare::Container);
    } else {
        CORRADE_COMPARE(layer.glyphCount(secondGlyph), 1);
        CORRADE_COMPARE(layer.glyphCount(second), 1);
        CORRADE_COMPARE(layer.size(secondGlyph), (Vector2{5.0f, 3.0f}));
        CORRADE_COMPARE(layer.size(second), (Vector2{16.0f, 24.0f}));
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::glyphOffset), Containers::arrayView({
            0u, 5u, 0xffffffffu, 0xffffffffu, 0xffffffffu, 10u, 11u, 12u, 14u
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphData).slice(&Implementation::TextLayerGlyphData::glyphId), Containers::arrayView({
            /* Glyphs 22, 13, 97, 22, 13; glyph 22 isn't in the cache */
            0u, 2u, 1u, 0u, 2u,
            /* Single (invalid) glyph 22 */
            0u,
            /* Now-unused "ahoy" text */
            3u,
            /* Now-unused single glyph 66 */
            3u,
            /* Glyphs 22, 13 */
            0u, 2u,
            /* Glyph 66 */
            3u,
            /* Single glyph 66 */
            3u,
            /* Glyphs 22, 97, char in the middle that mapped to 13 is \n */
            0u, 1u,
            /* Single (invalid) glyph 33 */
            0u
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphData).slice(&Implementation::TextLayerGlyphData::position), Containers::arrayView<Vector2>({
            /* "hello", aligned to MiddleCenter */
            {-5.0f, -0.5f},
            {-3.5f,  0.0f},
            {-1.5f,  0.5f},
            { 1.0f,  1.0f},
            { 4.0f,  1.5f},
            /* Single (invalid) glyph 22 */
            {-6.0f, -5.0f},
            /* Now-unused "ahoy" text, aligned to BottomRight */
            {-2.0f, 0.0f},
            /* Now-unused single glyph 66, aligned to BottomRight */
            {-20.0f, 2.0f},
            /* "hi", aligned to LineLeft */
            {0.0f, 0.5f},
            {1.5f, 1.0f},
            /* "hey", aligned to BottomRight */
            {-2.0f, 0.0f},
            /* Single glyph 66 again, aligned to BottomRight */
            {-20.0f, 2.0f},
            /* "ah", aligned to MiddleCenter */
            {-1.0f, 10.5f},
            {-1.0f, 3.5f},
            /* Single (invalid) glyph 33, MiddleCenter */
            {-10.0f, 1.0f},
        }), TestSuite::Compare::Container);
    }

    /* Similarly for text runs for editable text. What was a glyph before is
       now text and what was a text is now glyph. */
    if(data.flags && *data.flags >= TextDataFlag::Editable) {
        if(data.customFont)
            CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphData).slice(&Implementation::TextLayerGlyphData::glyphCluster), Containers::arrayView({
                /* Five glyphs corresponding to five characters */
                0u, 1u, 2u, 3u, 4u,
                /* Single glyph */
                0u,
                /* (Now-unused) single glyph corresponding to four characters */
                3u,
                /* (Now-unused) single glyph */
                0u,
                /* Two glyphs corresponding to two characters */
                0u, 1u,
                /* Three glyphs corresponding to three characters */
                0u, 1u, 2u,
                /* Single glyph again */
                0u
            }), TestSuite::Compare::Container);
        else
            CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphData).slice(&Implementation::TextLayerGlyphData::glyphCluster), Containers::arrayView({
                /* Five glyphs corresponding to five characters */
                0u, 1u, 2u, 3u, 4u,
                /* Single non-editable glyph */
                0u,
                /* (Now-unused) single glyph corresponding to four characters */
                3u,
                /* (Now-unused) single glyph */
                0u,
                /* Two glyphs corresponding to two characters */
                0u, 1u,
                /* Single glyph corresponding to three characters */
                2u,
                /* Single non-editable glyph */
                0u,
                /* Two non-editable glyphs */
                0u, 0u,
                /* Single non-editable glyph */
                0u
            }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().data).slice(&Implementation::TextLayerData::textRun), Containers::arrayView({
            /* Changes to the `fifth` and `sixth` text result in no new text
               runs, only the change to the `secondGlyph` is a new run */
            0u, 0xffffffffu, 0xffffffffu, 5u, 0xffffffffu, 3u, 0xffffffffu, 4u
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().textRuns).slice(&Implementation::TextLayerTextRun::textOffset), Containers::arrayView({
            0u, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 11u
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().textRuns).slice(&Implementation::TextLayerTextRun::textSize), Containers::arrayView({
            5u, 4u, 0u, 2u, 0u, 3u
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().textRuns).slice(&Implementation::TextLayerTextRun::data), Containers::arrayView({
            0u, 2u, 4u, 5u, 7u, 3u
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(layer.stateData().textData,
            "hello"
            "ahoy" /* now unused */
            ""     /* now unused */
            "hi"   /* now unused */
            ""     /* now unused */
            "hey",
            TestSuite::Compare::String);

    /* Nothing changes if no text is editable */
    } else {
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().data).slice(&Implementation::TextLayerData::textRun), Containers::arrayView<UnsignedInt>({
            0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu,
            0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().textRuns).slice(&Implementation::TextLayerTextRun::textOffset),
            Containers::ArrayView<UnsignedInt>{},
            TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().textRuns).slice(&Implementation::TextLayerTextRun::textSize),
            Containers::ArrayView<UnsignedInt>{},
            TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().textRuns).slice(&Implementation::TextLayerTextRun::data),
            Containers::ArrayView<UnsignedInt>{},
            TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(layer.stateData().textData,
            "",
            TestSuite::Compare::String);
    }

    /* Finally, modify the text and glyph without switching to each other. In
       case of editable text, it should properly mark the existing text run as
       unused, OTOH in case of the glyph it shouldn't as there's no existing
       text run. */
    if(data.flags && *data.flags >= TextDataFlag::Editable) {
        /* Not passing any flags will preserve the previous flags */
        data.layerDataHandleOverloads ?
            layer.setText(dataHandleData(secondGlyph), "ahoy", textProperties) :
            layer.setText(secondGlyph, "ahoy", textProperties);
        layer.setGlyph(second,
            data.customFont ? GlyphIndex(66) : GlyphIndex(13),
            textProperties);
        CORRADE_COMPARE(layer.flags(second), TextDataFlags{});
        CORRADE_COMPARE(layer.flags(secondGlyph), data.flags ? *data.flags : TextDataFlags{});
        /* Not checking the glyph cluster runs, there's no new variant to
           catch */
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().data).slice(&Implementation::TextLayerData::textRun), Containers::arrayView({
            0u, 0xffffffffu, 0xffffffffu, 6u, 0xffffffffu, 3u, 0xffffffffu, 4u
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().textRuns).slice(&Implementation::TextLayerTextRun::textOffset), Containers::arrayView({
            0u, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu, 14u
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().textRuns).slice(&Implementation::TextLayerTextRun::textSize), Containers::arrayView({
            5u, 4u, 0u, 2u, 0u, 3u, 4u
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().textRuns).slice(&Implementation::TextLayerTextRun::data), Containers::arrayView({
            0u, 2u, 4u, 5u, 7u, 3u, 3u
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(layer.stateData().textData,
            "hello"
            "ahoy" /* now unused */
            ""
            "hi"   /* now unused */
            ""     /* now unused */
            "hey"  /* now unused */
            "ahoy",
            TestSuite::Compare::String);
    }
}

void TextLayerTest::createRemoveHandleRecycle() {
    auto&& data = CreateRemoveHandleRecycleData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return Containers::pointer<OneGlyphShaper>(*this); }
    } font;

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};
    cache.addFont(67, &font);

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{1}};
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}},
        {shared.addFont(font, 1.0f)},
        {Text::Alignment::MiddleCenter},
        {}, {}, {}, {}, {}, {});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared, TextLayerFlags flags): TextLayer{handle, shared, flags} {}

        const State& stateData() const {
            return static_cast<const State&>(*_state);
        }
    } layer{layerHandle(0, 1), shared, data.layerFlags};

    DataHandle first = layer.create(0, "hello", {}, data.flags);
    DataHandle second = layer.create(0, "again", {}, data.flags);
    layer.setColor(first, 0x663399_rgbf);
    layer.setColor(second, 0xff3366_rgbf);
    if(data.layerFlags >= TextLayerFlag::Transformable) {
        layer.setTransformation(first, {3.5f, -7.0f}, Complex{}, 2.0f);
        layer.setTransformation(second, {-2.3f, 12.5f}, 35.0_degf, 1.0f);
    } else {
        layer.setPadding(first, Vector4{15.0f});
        layer.setPadding(second, Vector4{5.0f});
    }
    CORRADE_COMPARE(layer.color(first), 0x663399_rgbf);
    CORRADE_COMPARE(layer.color(second), 0xff3366_rgbf);
    if(data.layerFlags >= TextLayerFlag::Transformable) {
        CORRADE_COMPARE(layer.transformation(first), Containers::pair(Vector2{3.5f, -7.0f}, Complex{2.0f, 0.0f}));
        CORRADE_COMPARE(layer.transformation(second), Containers::pair(Vector2{-2.3f, 12.5f}, Complex::rotation(35.0_degf)));
    } else {
        CORRADE_COMPARE(layer.padding(first), Vector4{15.0f});
        CORRADE_COMPARE(layer.padding(second), Vector4{5.0f});
    }
    CORRADE_COMPARE(layer.flags(first), data.flags);
    CORRADE_COMPARE(layer.flags(second), data.flags);
    CORRADE_COMPARE(layer.stateData().data[dataHandleId(first)].textRun, data.flags >= TextDataFlag::Editable ? 0 : 0xffffffffu);
    CORRADE_COMPARE(layer.stateData().data[dataHandleId(second)].textRun, data.flags >= TextDataFlag::Editable ? 1 : 0xffffffffu);

    /* Data that reuses a previous slot should have all properties cleared, as
       well as the flags and text run if the previous one was editable */
    layer.remove(second);
    DataHandle second2 = layer.create(0, "yes", {});
    CORRADE_COMPARE(dataHandleId(second2), dataHandleId(second));
    CORRADE_COMPARE(layer.color(second2), 0xffffff_rgbf);
    if(data.layerFlags >= TextLayerFlag::Transformable)
        CORRADE_COMPARE(layer.transformation(second2), Containers::pair(Vector2{}, Complex{Math::IdentityInit}));
    else
        CORRADE_COMPARE(layer.padding(second2), Vector4{0.0f});
    CORRADE_COMPARE(layer.flags(second2), TextDataFlags{});
    CORRADE_COMPARE(layer.stateData().data[dataHandleId(second2)].textRun, 0xffffffffu);

    /* Same for a glyph */
    layer.remove(first);
    DataHandle first2 = layer.createGlyph(0, 0, {});
    CORRADE_COMPARE(dataHandleId(first2), dataHandleId(first));
    CORRADE_COMPARE(layer.color(first2), 0xffffff_rgbf);
    if(data.layerFlags >= TextLayerFlag::Transformable)
        CORRADE_COMPARE(layer.transformation(first2), Containers::pair(Vector2{}, Complex{Math::IdentityInit}));
    else
        CORRADE_COMPARE(layer.padding(first2), Vector4{0.0f});
    CORRADE_COMPARE(layer.flags(first2), TextDataFlags{});
    CORRADE_COMPARE(layer.stateData().data[dataHandleId(first2)].textRun, 0xffffffffu);
}

void TextLayerTest::createStyleOutOfRange() {
    auto&& data = CreateStyleOutOfRangeData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return {}; }
    } font;

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};
    cache.addFont(56, &font);

    /* In this case the uniform count is higher than the style count, which is
       unlikely to happen in practice. It's to verify the check happens against
       the style count, not uniform count. */
    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{6, data.styleCount}
        .setDynamicStyleCount(data.dynamicStyleCount)
    };

    FontHandle fontHandle = shared.addFont(font, 1.0f);

    shared.setStyle(
        TextLayerCommonStyleUniform{},
        Containers::arrayView({TextLayerStyleUniform{}, TextLayerStyleUniform{}, TextLayerStyleUniform{}, TextLayerStyleUniform{}, TextLayerStyleUniform{}, TextLayerStyleUniform{}}),
        Containers::arrayView({0u, 1u, 2u}).prefix(data.styleCount),
        Containers::arrayView({fontHandle, fontHandle, fontHandle}).prefix(data.styleCount),
        Containers::arrayView({Text::Alignment{}, Text::Alignment{}, Text::Alignment{}}).prefix(data.styleCount),
        {}, {}, {}, {}, {}, {});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    Containers::String out;
    Error redirectError{&out};
    layer.create(3, "", {});
    layer.createGlyph(3, 0, {});
    CORRADE_COMPARE(out,
        "Ui::TextLayer::create(): style 3 out of range for 3 styles\n"
        "Ui::TextLayer::createGlyph(): style 3 out of range for 3 styles\n");
}

void TextLayerTest::createNoStyleSet() {
    auto&& data = CreateUpdateNoStyleSetData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32}};

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    /* It should complain regardless of dynamic style count and even if the
       style count is 0 as the common uniform is still used in that case */
    } shared{cache, TextLayer::Shared::Configuration{data.styleCount}
        .setDynamicStyleCount(data.dynamicStyleCount)
    };

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    Containers::String out;
    Error redirectError{&out};
    layer.create(2, "", {});
    layer.createGlyph(1, 0, {});
    CORRADE_COMPARE(out,
        "Ui::TextLayer::create(): no style data was set\n"
        "Ui::TextLayer::createGlyph(): no style data was set\n");
}

void TextLayerTest::setCursor() {
    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return Containers::pointer<OneGlyphShaper>(*this); }
    } font;

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};
    cache.addFont(67, &font);

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{1}};
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}},
        {shared.addFont(font, 1.0f)},
        {Text::Alignment::MiddleCenter},
        {}, {}, {}, {}, {}, {});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    /* Required to be called before update() (because AbstractUserInterface
       guarantees the same on a higher level), not needed for anything here */
    layer.setSize({1, 1}, {1, 1});

    /* Just to be sure the setters aren't picking up the first ever data
       always */
    layer.create(0, "", {});
    DataHandle data = layer.create(0, "hello!!", {}, TextDataFlag::Editable);
    layer.create(0, "", {});
    CORRADE_COMPARE(layer.cursor(data), Containers::pair(7u, 7u));

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Set to a different position updates the state flag */
    layer.setCursor(data, 5);
    CORRADE_COMPARE(layer.cursor(data), Containers::pair(5u, 5u));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* LayerDataHandle overload */
    layer.setCursor(dataHandleData(data), 3);
    CORRADE_COMPARE(layer.cursor(dataHandleData(data)), Containers::pair(3u, 3u));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Setting a non-empty selection range updates the state flag */
    layer.setCursor(data, 3, 5);
    CORRADE_COMPARE(layer.cursor(data), Containers::pair(3u, 5u));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Setting the range from the other direction updates the state flag;
       LayerDataHandle overload */
    layer.setCursor(dataHandleData(data), 5, 3);
    CORRADE_COMPARE(layer.cursor(data), Containers::pair(5u, 3u));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Setting to the same position is a no-op */
    layer.setCursor(data, 5, 3);
    CORRADE_COMPARE(layer.cursor(data), Containers::pair(5u, 3u));
    CORRADE_COMPARE(layer.state(), LayerState{});

    /* Setting it after all text should work */
    layer.setCursor(data, 7);
    CORRADE_COMPARE(layer.cursor(data), Containers::pair(7u, 7u));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Setting it before all text as well */
    layer.setCursor(data, 0);
    CORRADE_COMPARE(layer.cursor(data), Containers::pair(0u, 0u));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

    /** @todo expand this test once there is internal state being set other
        than what's publicly queryable, such as actual visible cursor position
        caching */
}

void TextLayerTest::setCursorInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return Containers::pointer<OneGlyphShaper>(*this); }
    } font;

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};
    cache.addFont(67, &font);

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{1}};
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}},
        {shared.addFont(font, 1.0f)},
        {Text::Alignment::MiddleCenter},
        {}, {}, {}, {}, {}, {});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    DataHandle data = layer.create(0, "hello!!", {}, TextDataFlag::Editable);
    CORRADE_COMPARE(layer.cursor(data), Containers::pair(7u, 7u));

    Containers::String out;
    Error redirectError{&out};
    layer.setCursor(data, 8);
    layer.setCursor(data, 7, 8);
    CORRADE_COMPARE_AS(out,
        "Ui::TextLayer::setCursor(): position 8 out of range for a text of 7 bytes\n"
        "Ui::TextLayer::setCursor(): selection 8 out of range for a text of 7 bytes\n",
        TestSuite::Compare::String);
}

void TextLayerTest::updateText() {
    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return Containers::pointer<ThreeGlyphShaper>(*this); }
    } font;

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};
    cache.addFont(98, &font);

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{3}};
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}, TextLayerStyleUniform{}, TextLayerStyleUniform{}},
        {FontHandle::Null, FontHandle::Null, shared.addFont(font, 1.0f)},
        {Text::Alignment{}, Text::Alignment{}, Text::Alignment::MiddleCenter},
        {}, {}, {}, {}, {}, {});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    /* Required to be called before update() (because AbstractUserInterface
       guarantees the same on a higher level), not needed for anything here */
    layer.setSize({1, 1}, {1, 1});

    /* Use style 2 to verify it actually uses that one and not some other
       in subsequent updateText(); have also some text before and after to
       catch weird overlaps and OOB issues */
    layer.create(2, "aaaa", {}, TextDataFlag::Editable);
    DataHandle text = layer.create(2, "hello", {}, TextDataFlag::Editable);
    layer.create(2, "bb", {}, TextDataFlag::Editable);
    CORRADE_COMPARE(layer.text(text), "hello");
    CORRADE_COMPARE(layer.cursor(text), Containers::pair(5u, 5u));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);
    CORRADE_COMPARE(layer.glyphCount(text), 5);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Various variants of a no-op operation, neither sets any flags */
    layer.updateText(text, 0, 0, 0, "", 5);
    layer.updateText(text, 0, 0, 0, "", 5, 5);
    layer.updateText(text, 5, 0, 5, "", 5);
    layer.updateText(text, 5, 0, 5, "", 5, 5);
    CORRADE_COMPARE(layer.text(text), "hello");
    CORRADE_COMPARE(layer.cursor(text), Containers::pair(5u, 5u));
    CORRADE_COMPARE(layer.state(), LayerStates{});
    /* No reshaping should be done in this case */
    CORRADE_COMPARE(layer.glyphCount(text), 5);

    /* Updating cursor location alone sets an update flag */
    layer.updateText(text, 0, 0, 0, "", 3);
    CORRADE_COMPARE(layer.text(text), "hello");
    CORRADE_COMPARE(layer.cursor(text), Containers::pair(3u, 3u));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);
    /* No reshaping should be done in this case however */
    CORRADE_COMPARE(layer.glyphCount(text), 5);

    /* Updating just the selection sets an update flag as well */
    layer.updateText(text, 0, 0, 0, "", 3, 4);
    CORRADE_COMPARE(layer.text(text), "hello");
    CORRADE_COMPARE(layer.cursor(text), Containers::pair(3u, 4u));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);
    /* No reshaping should be done in this case however */
    CORRADE_COMPARE(layer.glyphCount(text), 5);

    /* Insertion at the very end, putting cursor right after */
    layer.updateText(text, 0, 0, 5, "oo?!", 9);
    CORRADE_COMPARE(layer.text(text), "hellooo?!");
    CORRADE_COMPARE(layer.cursor(text), Containers::pair(9u, 9u));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);
    /* Lazy verification that the text gets implicitly reshaped */
    CORRADE_COMPARE(layer.glyphCount(text), 9);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Removal at the very end, putting cursor back at the end; LayerDataHandle
       overload with implicit selection */
    layer.updateText(dataHandleData(text), 6, 3, 0, "", 4);
    CORRADE_COMPARE(layer.text(text), "helloo");
    CORRADE_COMPARE(layer.cursor(text), Containers::pair(4u, 4u));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);
    /* Lazy verification that the text gets implicitly reshaped */
    CORRADE_COMPARE(layer.glyphCount(text), 6);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Insertion at the end after a removed portion, cursor & selection inside
       it */
    layer.updateText(text, 1, 4, 2, "vercrafts", 5, 3);
    CORRADE_COMPARE(layer.text(text), "hovercrafts");
    CORRADE_COMPARE(layer.cursor(text), Containers::pair(5u, 3u));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);
    /* Lazy verification that the text gets implicitly reshaped */
    CORRADE_COMPARE(layer.glyphCount(text), 11);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Insertion before a removed portion, cursor inside it; LayerDataHandle
       overload with explicit selection */
    layer.updateText(dataHandleData(text), 5, 5, 2, "ldo", 4, 3);
    CORRADE_COMPARE(layer.text(text), "holdovers");
    CORRADE_COMPARE(layer.cursor(text), Containers::pair(4u, 3u));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);
    /* Lazy verification that the text gets implicitly reshaped */
    CORRADE_COMPARE(layer.glyphCount(text), 9);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Removing everything */
    layer.updateText(text, 0, 9, 0, "", 0);
    CORRADE_COMPARE(layer.text(text), "");
    CORRADE_COMPARE(layer.cursor(text), Containers::pair(0u, 0u));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);
    /* Lazy verification that the text gets implicitly reshaped */
    CORRADE_COMPARE(layer.glyphCount(text), 0);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* This is a no-op again */
    layer.updateText(text, 0, 0, 0, "", 0);
    CORRADE_COMPARE(layer.text(text), "");
    CORRADE_COMPARE(layer.cursor(text), Containers::pair(0u, 0u));
    CORRADE_COMPARE(layer.state(), LayerStates{});
    /* Lazy verification that the text gets implicitly reshaped */
    CORRADE_COMPARE(layer.glyphCount(text), 0);

    /** @todo expand this test once there is internal state being set other
        than what's publicly queryable, such as actual visible cursor position
        caching */
}

void TextLayerTest::updateTextInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return Containers::pointer<OneGlyphShaper>(*this); }
    } font;

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};
    cache.addFont(67, &font);

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{1}};
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}},
        {shared.addFont(font, 1.0f)},
        {Text::Alignment::MiddleCenter},
        {}, {}, {}, {}, {}, {});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    DataHandle data = layer.create(0, "hello!!", {}, TextDataFlag::Editable);
    CORRADE_COMPARE(layer.cursor(data), Containers::pair(7u, 7u));

    Containers::String out;
    Error redirectError{&out};
    layer.updateText(data, 8, 0, 0, "", 0);
    layer.updateText(data, 5, 3, 0, "", 0);
    /* These overflow if checked incorrectly, causing the assert to pass */
    layer.updateText(data, 0xffffffffu, 1, 0, "", 0);
    layer.updateText(data, 1, 0xffffffffu, 0, "", 0);
    layer.updateText(data, 0, 0, 8, "", 0);
    layer.updateText(data, 0, 0, 0, "", 8);
    /* Text size got smaller by 2 in these two */
    layer.updateText(data, 3, 2, 6, "", 0);
    layer.updateText(data, 3, 2, 0, "", 6);
    /* Text size got smaller by 2 here but larger by 3, yet still not enouh */
    layer.updateText(data, 3, 2, 0, "hey", 9);
    layer.updateText(data, 3, 2, 0, "hey", 8, 9);
    CORRADE_COMPARE_AS(out,
        "Ui::TextLayer::updateText(): remove offset 8 and size 0 out of range for a text of 7 bytes\n"
        "Ui::TextLayer::updateText(): remove offset 5 and size 3 out of range for a text of 7 bytes\n"
        "Ui::TextLayer::updateText(): remove offset 4294967295 and size 1 out of range for a text of 7 bytes\n"
        "Ui::TextLayer::updateText(): remove offset 1 and size 4294967295 out of range for a text of 7 bytes\n"
        "Ui::TextLayer::updateText(): insert offset 8 out of range for a text of 7 bytes\n"
        "Ui::TextLayer::updateText(): cursor position 8 out of range for a text of 7 bytes\n"
        "Ui::TextLayer::updateText(): insert offset 6 out of range for a text of 5 bytes\n"
        "Ui::TextLayer::updateText(): cursor position 6 out of range for a text of 5 bytes\n"
        "Ui::TextLayer::updateText(): cursor position 9 out of range for a text of 8 bytes\n"
        "Ui::TextLayer::updateText(): selection position 9 out of range for a text of 8 bytes\n",
        TestSuite::Compare::String);
}

void TextLayerTest::editText() {
    auto&& data = EditData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct Font: Text::AbstractFont {
        explicit Font(Text::ShapeDirection shapeDirection): _shapeDirection{shapeDirection} {}

        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return Containers::pointer<ThreeGlyphShaper>(*this, _shapeDirection); }

        private:
            Text::ShapeDirection _shapeDirection;
    /* The actual used direction is returned by the shaper, not passed in via
       TextProperties */
    } font{data.shapeDirection};

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};
    cache.addFont(98, &font);

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{3}};
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}, TextLayerStyleUniform{}, TextLayerStyleUniform{}},
        {FontHandle::Null, FontHandle::Null, shared.addFont(font, 1.0f)},
        {Text::Alignment{}, Text::Alignment{}, Text::Alignment::MiddleCenter},
        {}, {}, {}, {}, {}, {});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    /* Required to be called before update() (because AbstractUserInterface
       guarantees the same on a higher level), not needed for anything here */
    layer.setSize({1, 1}, {1, 1});

    /* Use style 2 to verify it actually uses that one and not some other
       in subsequent editText(); have also some text before and after to
       catch weird overlaps and OOB issues */
    layer.create(2, "aaaa", {}, TextDataFlag::Editable);
    DataHandle text = layer.create(2, data.text, {}, TextDataFlag::Editable);
    layer.create(2, "bb", {}, TextDataFlag::Editable);

    /* The cursor should always be at the end of the input text even in
       presence of weird data. Update it to what's desired. */
    CORRADE_COMPARE(layer.cursor(text).first(), Containers::StringView{data.text}.size());
    CORRADE_COMPARE(layer.cursor(text).second(), Containers::StringView{data.text}.size());
    if(data.selection)
        layer.setCursor(text, data.cursor, *data.selection);
    else
        layer.setCursor(text, data.cursor);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    layer.editText(text, data.edit, data.insert);
    CORRADE_COMPARE(layer.text(text), data.expected);
    CORRADE_COMPARE(layer.cursor(text), data.expectedCursor);
    CORRADE_COMPARE(layer.state(), data.expectedState);
}

void TextLayerTest::editTextInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return Containers::pointer<OneGlyphShaper>(*this); }
    } font;

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};
    cache.addFont(67, &font);

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{1}};
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}},
        {shared.addFont(font, 1.0f)},
        {Text::Alignment::MiddleCenter},
        {}, {}, {}, {}, {}, {});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    DataHandle data = layer.create(0, "hello!!", {}, TextDataFlag::Editable);

    Containers::String out;
    Error redirectError{&out};
    layer.editText(data, TextEdit::RemoveAfterCursor, "ah");
    /* Test one more enum value, and the LayerDataHandle overload */
    layer.editText(dataHandleData(data), TextEdit::MoveCursorLeft, "ah");
    CORRADE_COMPARE_AS(out,
        "Ui::TextLayer::editText(): Ui::TextEdit::RemoveAfterCursor requires no text to insert\n"
        "Ui::TextLayer::editText(): Ui::TextEdit::MoveCursorLeft requires no text to insert\n",
        TestSuite::Compare::String);
}

void TextLayerTest::cycleGlyphEditableNonEditableText() {
    /* Internally two instances of Text::RendererCore are used, one that
       populates glyph clusters, and one that not. One or the other gets used
       based on whether given text is editable, and each time reset() is called
       to sync the allocators to point to the same data between the two.

       Verify that cycling the two renderers when doing text updates, as well
       as direct glyph setting, doesn't trigger any suspicious behavior.
       There's no update() call between so the internal arrays are just growing
       indefinitely but each time the last run should contain the same
       data. */

    struct Shaper: Text::AbstractShaper {
        using Text::AbstractShaper::AbstractShaper;

        UnsignedInt doShape(Containers::StringView, UnsignedInt begin, UnsignedInt end, Containers::ArrayView<const Text::FeatureRange>) override {
            return end - begin;
        }
        void doGlyphIdsInto(const Containers::StridedArrayView1D<UnsignedInt>& ids) const override {
            for(UnsignedInt& i: ids)
                i = 0;
        }
        void doGlyphOffsetsAdvancesInto(const Containers::StridedArrayView1D<Vector2>& offsets, const Containers::StridedArrayView1D<Vector2>& advances) const override {
            offsets[0] = {};
            advances[0] = {5.0f, 0.0f};
        }
        void doGlyphClustersInto(const Containers::StridedArrayView1D<UnsignedInt>& clusters) const override {
            for(UnsignedInt& i: clusters)
                i = 0;
        }
    };

    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return _opened; }
        Properties doOpenFile(Containers::StringView, Float) override {
            _opened = true;
            return {12.0f, 4.0f, -4.0f, 8.0f, 1};
        }
        void doClose() override { _opened = false; }

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return Containers::pointer<Shaper>(*this); }

        bool _opened = false;
    } font;
    font.openFile({}, 0.0f);

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    /* Default padding is 1, resetting to 0 for simplicity */
    } cache{PixelFormat::R8Unorm, {32, 32}, {}};
    cache.addFont(1, &font);

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{1}};
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}},
        {shared.addFont(font, 12.0f)},
        {Text::Alignment::TopLeft},
        {}, {}, {}, {}, {}, {});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}

        const State& stateData() const {
            return static_cast<const State&>(*_state);
        }
    } layer{layerHandle(0, 1), shared};

    DataHandle text = layer.create(0, "hello", {});

    UnsignedInt reallocatedGlyphs = 0, reallocatedRuns = 0;
    const void* previousGlyphs = layer.stateData().glyphData.data();
    const void* previousRuns = layer.stateData().glyphRuns.data();
    for(std::size_t i = 0; i != 1000; ++i) {
        CORRADE_ITERATION(i);

        if(i % 3 == 0) {
            layer.setText(text, "hey", {}, TextDataFlag::Editable);
            CORRADE_COMPARE(layer.stateData().glyphData.size(), 5 + (i/3)*9 + 3);

        } else if(i % 3 == 1) {
            layer.setText(text, "hello", {});
            CORRADE_COMPARE(layer.stateData().glyphData.size(), 5 + (i/3)*9 + 8);

        } else {
            layer.setGlyph(text, 0, {});
            CORRADE_COMPARE(layer.stateData().glyphData.size(), 5 + (i/3)*9 + 9);
        }

        CORRADE_COMPARE(layer.stateData().glyphRuns.size(), 2 + i);

        if(previousGlyphs != layer.stateData().glyphData.data())
            ++reallocatedGlyphs;
        if(previousRuns != layer.stateData().glyphRuns.data())
            ++reallocatedRuns;
        previousGlyphs = layer.stateData().glyphData.data();
        previousRuns = layer.stateData().glyphRuns.data();
    }

    /* To actually verify the allocators handle this correctly, the underlying
       storage should be reallocated a few times */
    CORRADE_VERIFY(reallocatedGlyphs);
    CORRADE_VERIFY(reallocatedRuns);
    CORRADE_INFO("Reallocated glyphs" << reallocatedGlyphs << "times, runs" << reallocatedRuns << "times");
}

void TextLayerTest::createSetTextTextProperties() {
    auto&& data = CreateSetTextTextPropertiesData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* A font that just checks what has been sent to the shaper */
    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return _opened; }
        Properties doOpenFile(Containers::StringView, Float) override {
            _opened = true;
            return {1.0f, 1.0f, 1.0f, 2.0f, 1};
        }
        void doClose() override { _opened = false; }

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override {
            struct Shaper: Text::AbstractShaper {
                explicit Shaper(Text::AbstractFont& font, int& setScriptCalled, int& setLanguageCalled, int& setDirectionCalled, int& shapeCalled): Text::AbstractShaper{font}, setScriptCalled(setScriptCalled), setLanguageCalled(setLanguageCalled), setDirectionCalled(setDirectionCalled), shapeCalled(shapeCalled) {}

                bool doSetScript(Text::Script script) override {
                    CORRADE_COMPARE(script, Text::Script::HanifiRohingya);
                    ++setScriptCalled;
                    return true;
                }
                bool doSetLanguage(Containers::StringView language) override {
                    CORRADE_COMPARE(language, "eh-UH");
                    ++setLanguageCalled;
                    return true;
                }
                bool doSetDirection(Text::ShapeDirection direction) override {
                    CORRADE_COMPARE(direction, Text::ShapeDirection::BottomToTop);
                    ++setDirectionCalled;
                    return true;
                }
                UnsignedInt doShape(Containers::StringView, UnsignedInt, UnsignedInt, Containers::ArrayView<const Text::FeatureRange> features) override {
                    /* Features from the (dynamic) style are first, features
                       passed to create() second */
                    CORRADE_COMPARE(features.size(), 4);
                    CORRADE_COMPARE(features[0].feature(), Text::Feature::AccessAllAlternates);
                    CORRADE_COMPARE(features[0].value(), 57);
                    CORRADE_COMPARE(features[1].feature(), Text::Feature::TabularFigures);
                    CORRADE_COMPARE(features[2].feature(), Text::Feature::DiscretionaryLigatures);
                    CORRADE_COMPARE(features[2].begin(), 3);
                    CORRADE_COMPARE(features[2].end(), 5);
                    CORRADE_COMPARE(features[3].feature(), Text::Feature::Kerning);
                    CORRADE_VERIFY(!features[3].isEnabled());
                    ++shapeCalled;
                    return 0;
                }
                void doGlyphIdsInto(const Containers::StridedArrayView1D<UnsignedInt>&) const override {}
                void doGlyphOffsetsAdvancesInto(const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&) const override {}
                void doGlyphClustersInto(const Containers::StridedArrayView1D<UnsignedInt>&) const override {}

                int& setScriptCalled;
                int& setLanguageCalled;
                int& setDirectionCalled;
                int& shapeCalled;
            };

            return Containers::pointer<Shaper>(*this, setScriptCalled, setLanguageCalled, setDirectionCalled, shapeCalled);
        }

        int setScriptCalled = 0;
        int setLanguageCalled = 0;
        int setDirectionCalled = 0;
        int shapeCalled = 0;

        bool _opened = false;
    } font;
    font.openFile({}, 16.0f);

    /* A trivial glyph cache */
    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    /* Default padding is 1, resetting to 0 for simplicity */
    } cache{PixelFormat::R8Unorm, {32, 32}, {}};
    cache.addGlyph(cache.addFont(1, &font), 0, {}, {});

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{data.styleCount}
        .setDynamicStyleCount(data.dynamicStyleCount)
    };

    FontHandle fontHandle = shared.addFont(font, 16.0f);
    if(!data.dynamicStyleCount)
        shared.setStyle(TextLayerCommonStyleUniform{},
            {TextLayerStyleUniform{},
             TextLayerStyleUniform{},
             TextLayerStyleUniform{}},
            {fontHandle, fontHandle, fontHandle},
            {Text::Alignment::MiddleCenter,
             Text::Alignment::MiddleCenter,
             Text::Alignment::MiddleCenter},
            {Text::Feature::SlashedZero,
             {Text::Feature::AccessAllAlternates, 57},
             Text::Feature::TabularFigures},
            {2, 3, 1},
            {1, 0, 2},
            {}, {}, {});
    else
        shared.setStyle(TextLayerCommonStyleUniform{},
            {TextLayerStyleUniform{}},
            {fontHandle},
            {Text::Alignment::MiddleCenter},
            {Text::Feature::TabularFigures},
            {0},
            {1},
            {}, {}, {});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    if(data.dynamicStyleCount)
        layer.setDynamicStyle(1,
            TextLayerStyleUniform{},
            fontHandle,
            Text::Alignment::MiddleCenter,
            {{Text::Feature::AccessAllAlternates, 57},
             Text::Feature::TabularFigures},
            {});

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    DataHandle text = layer.create(2, "hello", TextProperties{}
        .setScript(Text::Script::HanifiRohingya)
        .setLanguage("eh-UH")
        .setShapeDirection(Text::ShapeDirection::BottomToTop)
        .setFeatures({
            {Text::Feature::DiscretionaryLigatures, 3, 5},
            {Text::Feature::Kerning, false}
        }));
    CORRADE_COMPARE(font.setScriptCalled, 1);
    CORRADE_COMPARE(font.setLanguageCalled, 1);
    CORRADE_COMPARE(font.setDirectionCalled, 1);
    CORRADE_COMPARE(font.shapeCalled, 1);

    /* setText() should do the same */
    layer.setText(text, "hello", TextProperties{}
        .setScript(Text::Script::HanifiRohingya)
        .setLanguage("eh-UH")
        .setShapeDirection(Text::ShapeDirection::BottomToTop)
        .setFeatures({
            {Text::Feature::DiscretionaryLigatures, 3, 5},
            {Text::Feature::Kerning, false}
        }));
    CORRADE_COMPARE(font.setScriptCalled, 2);
    CORRADE_COMPARE(font.setLanguageCalled, 2);
    CORRADE_COMPARE(font.setDirectionCalled, 2);
    CORRADE_COMPARE(font.shapeCalled, 2);

    /* createGlyph() doesn't call shape() at all */
    DataHandle glyph = layer.createGlyph(0, 0, {});
    layer.setGlyph(glyph, 0, {});
    CORRADE_COMPARE(font.setScriptCalled, 2);
    CORRADE_COMPARE(font.setLanguageCalled, 2);
    CORRADE_COMPARE(font.setDirectionCalled, 2);
    CORRADE_COMPARE(font.shapeCalled, 2);
}

void TextLayerTest::createSetTextTextPropertiesEditable() {
    auto&& data = CreateSetTextTextPropertiesData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Variant of createSetTextTextProperties() without features passed through
       TextProperties and checking behavior of updateText() and edit() as well.
       The actual behavior and corner cases of updateText() and edit() is
       tested in dedicated test cases below. */

    /* A font that just checks what has been sent to the shaper */
    struct Font: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return _opened; }
        Properties doOpenFile(Containers::StringView, Float) override {
            _opened = true;
            return {1.0f, 1.0f, 1.0f, 2.0f, 1};
        }
        void doClose() override { _opened = false; }

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override {
            struct Shaper: Text::AbstractShaper {
                explicit Shaper(Text::AbstractFont& font, int& setScriptCalled, int& setLanguageCalled, int& setDirectionCalled, int& shapeCalled): Text::AbstractShaper{font}, setScriptCalled(setScriptCalled), setLanguageCalled(setLanguageCalled), setDirectionCalled(setDirectionCalled), shapeCalled(shapeCalled) {}

                bool doSetScript(Text::Script script) override {
                    CORRADE_COMPARE(script, static_cast<Font&>(font()).expectedScript);
                    ++setScriptCalled;
                    return true;
                }
                bool doSetLanguage(Containers::StringView language) override {
                    CORRADE_COMPARE(language, static_cast<Font&>(font()).expectedLanguage);
                    ++setLanguageCalled;
                    return true;
                }
                bool doSetDirection(Text::ShapeDirection direction) override {
                    CORRADE_COMPARE(direction, static_cast<Font&>(font()).expectedDirection);
                    ++setDirectionCalled;
                    return true;
                }
                UnsignedInt doShape(Containers::StringView, UnsignedInt, UnsignedInt, Containers::ArrayView<const Text::FeatureRange> features) override {
                    /* Only features from the (dynamic) style, create() doesn't
                       allow them for editable texts */
                    CORRADE_COMPARE(features.size(), 2);
                    CORRADE_COMPARE(features[0].feature(), Text::Feature::AccessAllAlternates);
                    CORRADE_COMPARE(features[0].value(), 57);
                    CORRADE_COMPARE(features[1].feature(), Text::Feature::TabularFigures);
                    ++shapeCalled;
                    return 0;
                }
                void doGlyphIdsInto(const Containers::StridedArrayView1D<UnsignedInt>&) const override {}
                void doGlyphOffsetsAdvancesInto(const Containers::StridedArrayView1D<Vector2>&, const Containers::StridedArrayView1D<Vector2>&) const override {}
                void doGlyphClustersInto(const Containers::StridedArrayView1D<UnsignedInt>&) const override {}

                int& setScriptCalled;
                int& setLanguageCalled;
                int& setDirectionCalled;
                int& shapeCalled;
            };

            return Containers::pointer<Shaper>(*this, setScriptCalled, setLanguageCalled, setDirectionCalled, shapeCalled);
        }

        Text::Script expectedScript;
        Containers::StringView expectedLanguage;
        Text::ShapeDirection expectedDirection;

        int setScriptCalled = 0;
        int setLanguageCalled = 0;
        int setDirectionCalled = 0;
        int shapeCalled = 0;

        bool _opened = false;
    } font;
    font.openFile({}, 16.0f);

    /* A trivial glyph cache */
    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    /* Default padding is 1, resetting to 0 for simplicity */
    } cache{PixelFormat::R8Unorm, {32, 32}, {}};
    cache.addGlyph(cache.addFont(1, &font), 0, {}, {});

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{data.styleCount}
        .setDynamicStyleCount(data.dynamicStyleCount)
    };

    FontHandle fontHandle = shared.addFont(font, 16.0f);
    FontHandle fontHandle2 = shared.addFont(font, 12.0f);
    if(!data.dynamicStyleCount)
        shared.setStyle(TextLayerCommonStyleUniform{},
            {TextLayerStyleUniform{},
             TextLayerStyleUniform{},
             TextLayerStyleUniform{}},
            {fontHandle, fontHandle, fontHandle},
            {Text::Alignment::MiddleCenter,
             Text::Alignment::MiddleCenter,
             Text::Alignment::MiddleCenter},
            {Text::Feature::SlashedZero,
             {Text::Feature::AccessAllAlternates, 57},
             Text::Feature::TabularFigures},
            {2, 3, 1},
            {1, 0, 2},
            {}, {}, {});
    else
        shared.setStyle(TextLayerCommonStyleUniform{},
            {TextLayerStyleUniform{}},
            {fontHandle},
            {Text::Alignment::MiddleCenter},
            {Text::Feature::TabularFigures},
            {0},
            {1},
            {}, {}, {});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    if(data.dynamicStyleCount)
        layer.setDynamicStyle(1,
            TextLayerStyleUniform{},
            fontHandle,
            Text::Alignment::MiddleCenter,
            {{Text::Feature::AccessAllAlternates, 57},
             Text::Feature::TabularFigures},
            {});

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    /* create() should save the properties */
    font.expectedScript = Text::Script::HanifiRohingya;
    font.expectedLanguage = "eh-UH";
    font.expectedDirection = Text::ShapeDirection::RightToLeft;
    DataHandle text = layer.create(2, "hello",
        TextProperties{}
            .setScript(Text::Script::HanifiRohingya)
            .setLanguage("eh-UH")
            .setShapeDirection(Text::ShapeDirection::RightToLeft),
        TextDataFlag::Editable);
    CORRADE_COMPARE(layer.text(text), "hello");
    CORRADE_COMPARE(layer.cursor(text), Containers::pair(5u, 5u));
    /* Alignment wasn't set */
    CORRADE_COMPARE(layer.textProperties(text).alignment(), Containers::NullOpt);
    /* Font wasn't set in the properties but the actual used one got saved to
       not need to go through the same font selection logic on every edit */
    CORRADE_COMPARE(layer.textProperties(text).font(), fontHandle);
    CORRADE_COMPARE(layer.textProperties(text).script(), Text::Script::HanifiRohingya);
    CORRADE_COMPARE(layer.textProperties(text).language(), "eh-UH");
    CORRADE_COMPARE(layer.textProperties(text).shapeDirection(), Text::ShapeDirection::RightToLeft);
    CORRADE_COMPARE(layer.textProperties(text).layoutDirection(), Text::LayoutDirection::HorizontalTopToBottom);
    CORRADE_VERIFY(layer.textProperties(text).features().isEmpty());
    CORRADE_COMPARE(font.setScriptCalled, 1);
    CORRADE_COMPARE(font.setLanguageCalled, 1);
    CORRADE_COMPARE(font.setDirectionCalled, 1);
    CORRADE_COMPARE(font.shapeCalled, 1);

    /* updateText() should pass the same */
    layer.updateText(text, 0, 0, 5, "!", 6);
    CORRADE_COMPARE(layer.text(text), "hello!");
    CORRADE_COMPARE(layer.cursor(text), Containers::pair(6u, 6u));
    CORRADE_COMPARE(font.setScriptCalled, 2);
    CORRADE_COMPARE(font.setLanguageCalled, 2);
    CORRADE_COMPARE(font.setDirectionCalled, 2);
    CORRADE_COMPARE(font.shapeCalled, 2);

    /* setText() with different properties should overwrite the previous */
    font.expectedScript = Text::Script::EgyptianHieroglyphs;
    font.expectedLanguage = "eg-HE";
    font.expectedDirection = Text::ShapeDirection::Unspecified;
    layer.setText(text, "hello?",
        TextProperties{}
            .setScript(Text::Script::EgyptianHieroglyphs)
            .setLanguage("eg-HE")
            .setShapeDirection(Text::ShapeDirection::Unspecified)
            /* Testing that these are now set. The font is the same but under a
               different handle. */
            .setFont(fontHandle2)
            .setAlignment(Text::Alignment::BottomCenter));
    /* Testing the LayerDataHandle overload */
    CORRADE_COMPARE(layer.textProperties(dataHandleData(text)).alignment(), Text::Alignment::BottomCenter);
    CORRADE_COMPARE(layer.textProperties(dataHandleData(text)).font(), fontHandle2);
    CORRADE_COMPARE(layer.textProperties(dataHandleData(text)).script(), Text::Script::EgyptianHieroglyphs);
    CORRADE_COMPARE(layer.textProperties(dataHandleData(text)).language(), "eg-HE");
    CORRADE_COMPARE(layer.textProperties(dataHandleData(text)).shapeDirection(), Text::ShapeDirection::Unspecified);
    CORRADE_COMPARE(layer.textProperties(dataHandleData(text)).layoutDirection(), Text::LayoutDirection::HorizontalTopToBottom);
    CORRADE_VERIFY(layer.textProperties(dataHandleData(text)).features().isEmpty());
    CORRADE_COMPARE(font.setScriptCalled, 3);
    CORRADE_COMPARE(font.setLanguageCalled, 3);
    CORRADE_COMPARE(font.setDirectionCalled, 3);
    CORRADE_COMPARE(font.shapeCalled, 3);

    /* editText() should behave same as updateText(), i.e. pass what was saved
       above */
    layer.editText(text, TextEdit::InsertBeforeCursor, "!");
    CORRADE_COMPARE(layer.text(text), "hello?!");
    CORRADE_COMPARE(layer.cursor(text), Containers::pair(7u, 7u));
    CORRADE_COMPARE(font.setScriptCalled, 4);
    CORRADE_COMPARE(font.setLanguageCalled, 4);
    CORRADE_COMPARE(font.setDirectionCalled, 4);
    CORRADE_COMPARE(font.shapeCalled, 4);
}

void TextLayerTest::createSetTextTextPropertiesEditableInvalid() {
    auto&& data = CreateSetTextTextPropertiesEditableInvalidData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return Containers::pointer<OneGlyphShaper>(*this); }
    } font;

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};
    cache.addFont(67, &font);

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{1}};
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}},
        {shared.addFont(font, 1.0f)},
        {Text::Alignment::MiddleCenter},
        /* Using some features in the style itself should be okay */
        {{Text::Feature::Kerning, false}}, {0}, {1},
        {}, {}, {});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    DataHandle nonEditable = layer.create(0, "hello", {});
    DataHandle editable = layer.create(0, "hello", {}, TextDataFlag::Editable);
    DataHandle editable2 = layer.create(0, "hello", {}, TextDataFlag::Editable);

    /* Passing features to a text that was formerly editable but isn't anymore
       is okay */
    layer.setText(editable2, "hey", data.properties, TextDataFlags{});

    Containers::String out;
    Error redirectError{&out};
    layer.create(0, "hello", data.properties, TextDataFlag::Editable);
    /* Should assert also if trying to pass features to a text that already has
       the flag set */
    layer.setText(editable, "hey", data.properties);
    /* Or if passing features that didn't have the flag but now does */
    layer.setText(nonEditable, "hey", data.properties, TextDataFlag::Editable);
    CORRADE_COMPARE_AS(out, Utility::format(
        "Ui::TextLayer::create(): {0}\n"
        "Ui::TextLayer::setText(): {0}\n"
        "Ui::TextLayer::setText(): {0}\n",
    data.expected), TestSuite::Compare::String);
}

void TextLayerTest::createSetUpdateTextFromLayerItself() {
    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return Containers::pointer<ThreeGlyphShaper>(*this); }
    } font;

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};
    cache.addFont(98, &font);

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{1}};
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}},
        {shared.addFont(font, 1.0f)},
        {Text::Alignment::MiddleCenter},
        {}, {}, {}, {}, {}, {});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    /* Assuming the implementation stores all texts in one large array, this
       working correctly relies on arrayAppend() detecting if the view being
       copied is a slice of the array itself. If it wouldn't, inevitably it'd
       happen that garbage gets copied, or it blows up due to invalid memory
       access.

       To make sure this indeed gets hit, the test is repeated a few times with
       varying string sizes to minimize the chance of realloc() growing
       in-place every time. */
    DataHandle first = layer.create(0, "hello there" + " how is everyone"_s*testCaseRepeatId(), {}, TextDataFlag::Editable);
    DataHandle firstCopy = layer.create(0, layer.text(first), {}, TextDataFlag::Editable);
    DataHandle second = layer.create(0, "hiya", {}, TextDataFlag::Editable);
    DataHandle third = layer.create(0, "hey hey", {}, TextDataFlag::Editable);
    layer.setText(second, layer.text(third), {});
    layer.updateText(third, 0, 0, 7, layer.text(first).exceptPrefix(5), 0);

    CORRADE_COMPARE(layer.text(first), "hello there" + " how is everyone"_s*testCaseRepeatId());
    CORRADE_COMPARE(layer.text(firstCopy), "hello there" + " how is everyone"_s*testCaseRepeatId());
    CORRADE_COMPARE(layer.text(second), "hey hey");
    CORRADE_COMPARE(layer.text(third), "hey hey there" + " how is everyone"_s*testCaseRepeatId());
}

void TextLayerTest::setColor() {
    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return Containers::pointer<OneGlyphShaper>(*this); }
    } font;

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};
    cache.addFont(67, &font);

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{1}};
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}},
        {shared.addFont(font, 1.0f)},
        {Text::Alignment::MiddleCenter},
        {}, {}, {}, {}, {}, {});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    /* Required to be called before update() (because AbstractUserInterface
       guarantees the same on a higher level), not needed for anything here */
    layer.setSize({1, 1}, {1, 1});

    /* Just to be sure the setters aren't picking up the first ever data
       always */
    layer.create(0, "", {});

    /* There's nothing that would work differently for createGlyph() */
    DataHandle data = layer.create(0, "", {});
    CORRADE_COMPARE(layer.color(data), 0xffffffff_rgbaf);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Setting a color marks the layer as dirty */
    layer.setColor(data, 0xaabbccdd_rgbaf);
    CORRADE_COMPARE(layer.color(data), 0xaabbccdd_rgbaf);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Testing also the other overload */
    layer.setColor(dataHandleData(data), 0x11223344_rgbaf);
    CORRADE_COMPARE(layer.color(dataHandleData(data)), 0x11223344_rgbaf);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);
}

void TextLayerTest::setPadding() {
    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return Containers::pointer<OneGlyphShaper>(*this); }
    } font;

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};
    cache.addFont(67, &font);

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{1}};
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}},
        {shared.addFont(font, 1.0f)},
        {Text::Alignment::MiddleCenter},
        {}, {}, {}, {}, {}, {});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    /* Required to be called before update() (because AbstractUserInterface
       guarantees the same on a higher level), not needed for anything here */
    layer.setSize({1, 1}, {1, 1});

    /* Just to be sure the setters aren't picking up the first ever data
       always */
    layer.create(0, "", {});

    /* There's nothing that would work differently for createGlyph() */
    DataHandle data = layer.create(0, "", {});
    CORRADE_COMPARE(layer.padding(data), Vector4{0.0f});
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Setting a padding marks the layer as dirty */
    layer.setPadding(data, {2.0f, 4.0f, 3.0f, 1.0f});
    CORRADE_COMPARE(layer.padding(data), (Vector4{2.0f, 4.0f, 3.0f, 1.0f}));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Testing also the other overload */
    layer.setPadding(dataHandleData(data), {1.0f, 2.0f, 3.0f, 4.0f});
    CORRADE_COMPARE(layer.padding(dataHandleData(data)), (Vector4{1.0f, 2.0f, 3.0f, 4.0f}));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Single-value padding */
    layer.setPadding(data, 4.0f);
    CORRADE_COMPARE(layer.padding(data), Vector4{4.0f});
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Testing also the other overload */
    layer.setPadding(dataHandleData(data), 3.0f);
    CORRADE_COMPARE(layer.padding(dataHandleData(data)), Vector4{3.0f});
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);
}

void TextLayerTest::setPaddingInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return Containers::pointer<OneGlyphShaper>(*this); }
    } font;

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};
    cache.addFont(67, &font);

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{1}};
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}},
        {shared.addFont(font, 1.0f)},
        {Text::Alignment{}},
        {}, {}, {}, {}, {}, {});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared, TextLayerFlag::Transformable} {}
    } layer{layerHandle(0, 1), shared};

    DataHandle data = layer.create(0, "", {});

    Containers::String out;
    Error redirectError{&out};
    layer.padding(data);
    layer.padding(dataHandleData(data));
    layer.setPadding(data, Vector4{});
    layer.setPadding(dataHandleData(data), Vector4{});
    layer.setPadding(data, 0.0f);
    layer.setPadding(dataHandleData(data), 0.0f);
    CORRADE_COMPARE_AS(out,
        "Ui::TextLayer::padding(): per-data padding not available on a Ui::TextLayerFlag::Transformable layer\n"
        "Ui::TextLayer::padding(): per-data padding not available on a Ui::TextLayerFlag::Transformable layer\n"
        "Ui::TextLayer::setPadding(): per-data padding not available on a Ui::TextLayerFlag::Transformable layer\n"
        "Ui::TextLayer::setPadding(): per-data padding not available on a Ui::TextLayerFlag::Transformable layer\n"
        "Ui::TextLayer::setPadding(): per-data padding not available on a Ui::TextLayerFlag::Transformable layer\n"
        "Ui::TextLayer::setPadding(): per-data padding not available on a Ui::TextLayerFlag::Transformable layer\n",
        TestSuite::Compare::String);
}

void TextLayerTest::setTransformation() {
    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return Containers::pointer<OneGlyphShaper>(*this); }
    } font;

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};
    cache.addFont(67, &font);

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{1}};
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}},
        {shared.addFont(font, 1.0f)},
        {Text::Alignment::MiddleCenter},
        {}, {}, {}, {}, {}, {});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared, TextLayerFlag::Transformable} {}
    } layer{layerHandle(0, 1), shared};

    /* Required to be called before update() (because AbstractUserInterface
       guarantees the same on a higher level), not needed for anything here */
    layer.setSize({1, 1}, {1, 1});

    /* Just to be sure the setters aren't picking up the first ever data
       always */
    layer.create(0, "", {});

    /* There's nothing that would work differently for createGlyph() */
    DataHandle data = layer.create(0, "", {});
    CORRADE_COMPARE(layer.transformation(data), Containers::pair(Vector2{}, Complex{Math::IdentityInit}));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Setting a transformation marks the layer as dirty */
    layer.setTransformation(data, {2.0f, 4.0f}, Complex::rotation(35.0_degf), 3.0f);
    CORRADE_COMPARE(layer.transformation(data), Containers::pair(Vector2{2.0f, 4.0f}, Complex::rotation(35.0_degf)*3.0f));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Testing also the LayerDataHandle overload */
    layer.setTransformation(dataHandleData(data), {1.0f, 3.0f}, Complex::rotation(-35.0_degf), 2.0f);
    CORRADE_COMPARE(layer.transformation(dataHandleData(data)), Containers::pair(Vector2{1.0f, 3.0f}, Complex::rotation(-35.0_degf)*2.0f));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Overload taking an angle directly */
    layer.setTransformation(data, {2.0f, 4.0f}, 35.0_degf, 3.0f);
    CORRADE_COMPARE(layer.transformation(data), Containers::pair(Vector2{2.0f, 4.0f}, Complex::rotation(35.0_degf)*3.0f));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Testing also the LayerDataHandle overload */
    layer.setTransformation(dataHandleData(data), {1.0f, 3.0f}, -35.0_degf, 2.0f);
    CORRADE_COMPARE(layer.transformation(dataHandleData(data)), Containers::pair(Vector2{1.0f, 3.0f}, Complex::rotation(-35.0_degf)*2.0f));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Translating adds to existing transformation */
    layer.translate(data, {0.5f, -0.25f});
    CORRADE_COMPARE(layer.transformation(data), Containers::pair(Vector2{1.5f, 2.75f}, Complex::rotation(-35.0_degf)*2.0f));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Testing also the LayerDataHandle overload */
    layer.translate(dataHandleData(data), {-0.5f, 0.25f});
    CORRADE_COMPARE(layer.transformation(dataHandleData(data)), Containers::pair(Vector2{1.0f, 3.0f}, Complex::rotation(-35.0_degf)*2.0f));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Rotating adds to existing transformation */
    layer.rotate(data, Complex::rotation(15.0_degf)*1.5f);
    CORRADE_COMPARE(layer.transformation(data), Containers::pair(Vector2{1.0f, 3.0f}, Complex::rotation(-20.0_degf)*3.0f));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Testing also the LayerDataHandle overload */
    layer.rotate(dataHandleData(data), Complex::rotation(-15.0_degf)/1.5f);
    CORRADE_COMPARE(layer.transformation(dataHandleData(data)), Containers::pair(Vector2{1.0f, 3.0f}, Complex::rotation(-35.0_degf)*2.0f));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Overload taking an angle directly */
    layer.rotate(data, 15.0_degf);
    CORRADE_COMPARE(layer.transformation(data), Containers::pair(Vector2{1.0f, 3.0f}, Complex::rotation(-20.0_degf)*2.0f));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Testing also the LayerDataHandle overload */
    layer.rotate(dataHandleData(data), -15.0_degf);
    CORRADE_COMPARE(layer.transformation(dataHandleData(data)), Containers::pair(Vector2{1.0f, 3.0f}, Complex::rotation(-35.0_degf)*2.0f));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Scaling adds to existing transformation */
    layer.scale(data, 4.0f);
    CORRADE_COMPARE(layer.transformation(data), Containers::pair(Vector2{1.0f, 3.0f}, Complex::rotation(-35.0_degf)*8.0f));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Testing also the LayerDataHandle overload */
    layer.scale(dataHandleData(data), 0.25f);
    CORRADE_COMPARE(layer.transformation(dataHandleData(data)), Containers::pair(Vector2{1.0f, 3.0f}, Complex::rotation(-35.0_degf)*2.0f));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);
}

void TextLayerTest::setTransformationInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return Containers::pointer<OneGlyphShaper>(*this); }
    } font;

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};
    cache.addFont(67, &font);

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{1}};
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}},
        {shared.addFont(font, 1.0f)},
        {Text::Alignment{}},
        {}, {}, {}, {}, {}, {});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    DataHandle data = layer.create(0, "", {});

    Containers::String out;
    Error redirectError{&out};
    layer.transformation(data);
    layer.transformation(dataHandleData(data));
    layer.setTransformation(data, {}, Complex{}, {});
    layer.setTransformation(data, {}, Rad{}, {});
    layer.setTransformation(dataHandleData(data), {}, Complex{}, {});
    layer.setTransformation(dataHandleData(data), {}, Rad{}, {});
    layer.translate(data, {});
    layer.translate(dataHandleData(data), {});
    layer.rotate(data, Complex{});
    layer.rotate(data, Rad{});
    layer.rotate(dataHandleData(data), Complex{});
    layer.rotate(dataHandleData(data), Rad{});
    layer.scale(data, {});
    layer.scale(dataHandleData(data), {});
    CORRADE_COMPARE_AS(out,
        "Ui::TextLayer::transformation(): layer isn't Ui::TextLayerFlag::Transformable\n"
        "Ui::TextLayer::transformation(): layer isn't Ui::TextLayerFlag::Transformable\n"
        "Ui::TextLayer::setTransformation(): layer isn't Ui::TextLayerFlag::Transformable\n"
        "Ui::TextLayer::setTransformation(): layer isn't Ui::TextLayerFlag::Transformable\n"
        "Ui::TextLayer::setTransformation(): layer isn't Ui::TextLayerFlag::Transformable\n"
        "Ui::TextLayer::setTransformation(): layer isn't Ui::TextLayerFlag::Transformable\n"
        "Ui::TextLayer::translate(): layer isn't Ui::TextLayerFlag::Transformable\n"
        "Ui::TextLayer::translate(): layer isn't Ui::TextLayerFlag::Transformable\n"
        "Ui::TextLayer::rotate(): layer isn't Ui::TextLayerFlag::Transformable\n"
        "Ui::TextLayer::rotate(): layer isn't Ui::TextLayerFlag::Transformable\n"
        "Ui::TextLayer::rotate(): layer isn't Ui::TextLayerFlag::Transformable\n"
        "Ui::TextLayer::rotate(): layer isn't Ui::TextLayerFlag::Transformable\n"
        "Ui::TextLayer::scale(): layer isn't Ui::TextLayerFlag::Transformable\n"
        "Ui::TextLayer::scale(): layer isn't Ui::TextLayerFlag::Transformable\n",
        TestSuite::Compare::String);
}

void TextLayerTest::invalidHandle() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32}};

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{1}};

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    Containers::String out;
    Error redirectError{&out};
    layer.flags(DataHandle::Null);
    layer.flags(LayerDataHandle::Null);
    layer.glyphCount(DataHandle::Null);
    layer.glyphCount(LayerDataHandle::Null);
    layer.size(DataHandle::Null);
    layer.size(LayerDataHandle::Null);
    layer.cursor(DataHandle::Null);
    layer.cursor(LayerDataHandle::Null);
    layer.setCursor(DataHandle::Null, 0);
    layer.setCursor(LayerDataHandle::Null, 0);
    layer.textProperties(DataHandle::Null);
    layer.textProperties(LayerDataHandle::Null);
    layer.text(DataHandle::Null);
    layer.text(LayerDataHandle::Null);
    layer.setText(DataHandle::Null, "", {});
    layer.setText(LayerDataHandle::Null, "", {});
    layer.updateText(DataHandle::Null, 0, 0, 0, {}, 0);
    layer.updateText(LayerDataHandle::Null, 0, 0, 0, {}, 0);
    layer.editText(DataHandle::Null, TextEdit::MoveCursorLeft, {});
    layer.editText(LayerDataHandle::Null, TextEdit::MoveCursorLeft, {});
    layer.setGlyph(DataHandle::Null, 0, {});
    layer.setGlyph(LayerDataHandle::Null, 0, {});
    layer.color(DataHandle::Null);
    layer.color(LayerDataHandle::Null);
    layer.setColor(DataHandle::Null, {});
    layer.setColor(LayerDataHandle::Null, {});
    layer.padding(DataHandle::Null);
    layer.padding(LayerDataHandle::Null);
    layer.setPadding(DataHandle::Null, {});
    layer.setPadding(LayerDataHandle::Null, {});
    CORRADE_COMPARE_AS(out,
        "Ui::TextLayer::flags(): invalid handle Ui::DataHandle::Null\n"
        "Ui::TextLayer::flags(): invalid handle Ui::LayerDataHandle::Null\n"
        "Ui::TextLayer::glyphCount(): invalid handle Ui::DataHandle::Null\n"
        "Ui::TextLayer::glyphCount(): invalid handle Ui::LayerDataHandle::Null\n"
        "Ui::TextLayer::size(): invalid handle Ui::DataHandle::Null\n"
        "Ui::TextLayer::size(): invalid handle Ui::LayerDataHandle::Null\n"
        "Ui::TextLayer::cursor(): invalid handle Ui::DataHandle::Null\n"
        "Ui::TextLayer::cursor(): invalid handle Ui::LayerDataHandle::Null\n"
        "Ui::TextLayer::setCursor(): invalid handle Ui::DataHandle::Null\n"
        "Ui::TextLayer::setCursor(): invalid handle Ui::LayerDataHandle::Null\n"
        "Ui::TextLayer::textProperties(): invalid handle Ui::DataHandle::Null\n"
        "Ui::TextLayer::textProperties(): invalid handle Ui::LayerDataHandle::Null\n"
        "Ui::TextLayer::text(): invalid handle Ui::DataHandle::Null\n"
        "Ui::TextLayer::text(): invalid handle Ui::LayerDataHandle::Null\n"
        "Ui::TextLayer::setText(): invalid handle Ui::DataHandle::Null\n"
        "Ui::TextLayer::setText(): invalid handle Ui::LayerDataHandle::Null\n"
        "Ui::TextLayer::updateText(): invalid handle Ui::DataHandle::Null\n"
        "Ui::TextLayer::updateText(): invalid handle Ui::LayerDataHandle::Null\n"
        "Ui::TextLayer::editText(): invalid handle Ui::DataHandle::Null\n"
        "Ui::TextLayer::editText(): invalid handle Ui::LayerDataHandle::Null\n"
        "Ui::TextLayer::setGlyph(): invalid handle Ui::DataHandle::Null\n"
        "Ui::TextLayer::setGlyph(): invalid handle Ui::LayerDataHandle::Null\n"
        "Ui::TextLayer::color(): invalid handle Ui::DataHandle::Null\n"
        "Ui::TextLayer::color(): invalid handle Ui::LayerDataHandle::Null\n"
        "Ui::TextLayer::setColor(): invalid handle Ui::DataHandle::Null\n"
        "Ui::TextLayer::setColor(): invalid handle Ui::LayerDataHandle::Null\n"
        "Ui::TextLayer::padding(): invalid handle Ui::DataHandle::Null\n"
        "Ui::TextLayer::padding(): invalid handle Ui::LayerDataHandle::Null\n"
        "Ui::TextLayer::setPadding(): invalid handle Ui::DataHandle::Null\n"
        "Ui::TextLayer::setPadding(): invalid handle Ui::LayerDataHandle::Null\n",
        TestSuite::Compare::String);
}

void TextLayerTest::invalidHandleTransformation() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32}};

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{1}};

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    Containers::String out;
    Error redirectError{&out};
    layer.transformation(DataHandle::Null);
    layer.transformation(LayerDataHandle::Null);
    layer.setTransformation(DataHandle::Null, {}, Complex{}, {});
    layer.setTransformation(LayerDataHandle::Null, {}, Complex{}, {});
    layer.setTransformation(DataHandle::Null, {}, Rad{}, {});
    layer.setTransformation(LayerDataHandle::Null, {}, Rad{}, {});
    layer.translate(DataHandle::Null, {});
    layer.translate(LayerDataHandle::Null, {});
    layer.rotate(DataHandle::Null, Complex{});
    layer.rotate(LayerDataHandle::Null, Complex{});
    layer.rotate(DataHandle::Null, Rad{});
    layer.rotate(LayerDataHandle::Null, Rad{});
    layer.scale(DataHandle::Null, {});
    layer.scale(LayerDataHandle::Null, {});
    CORRADE_COMPARE_AS(out,
        "Ui::TextLayer::transformation(): invalid handle Ui::DataHandle::Null\n"
        "Ui::TextLayer::transformation(): invalid handle Ui::LayerDataHandle::Null\n"
        "Ui::TextLayer::setTransformation(): invalid handle Ui::DataHandle::Null\n"
        "Ui::TextLayer::setTransformation(): invalid handle Ui::LayerDataHandle::Null\n"
        "Ui::TextLayer::setTransformation(): invalid handle Ui::DataHandle::Null\n"
        "Ui::TextLayer::setTransformation(): invalid handle Ui::LayerDataHandle::Null\n"
        "Ui::TextLayer::translate(): invalid handle Ui::DataHandle::Null\n"
        "Ui::TextLayer::translate(): invalid handle Ui::LayerDataHandle::Null\n"
        "Ui::TextLayer::rotate(): invalid handle Ui::DataHandle::Null\n"
        "Ui::TextLayer::rotate(): invalid handle Ui::LayerDataHandle::Null\n"
        "Ui::TextLayer::rotate(): invalid handle Ui::DataHandle::Null\n"
        "Ui::TextLayer::rotate(): invalid handle Ui::LayerDataHandle::Null\n"
        "Ui::TextLayer::scale(): invalid handle Ui::DataHandle::Null\n"
        "Ui::TextLayer::scale(): invalid handle Ui::LayerDataHandle::Null\n",
        TestSuite::Compare::String);
}

void TextLayerTest::invalidFontHandle() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return Containers::pointer<OneGlyphShaper>(*this); }
    } font;

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};
    cache.addFont(67, &font);

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{1}};
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}},
        {shared.addFont(font, 1.0f)},
        {Text::Alignment{}},
        {}, {}, {}, {}, {}, {});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    DataHandle data = layer.create(0, "", {});

    Containers::String out;
    Error redirectError{&out};
    layer.create(0, "", FontHandle(0x12ab));
    layer.createGlyph(0, 0, FontHandle(0x12ab));
    layer.setText(data, "", FontHandle(0x12ab));
    layer.setGlyph(data, 0, FontHandle(0x12ab));
    CORRADE_COMPARE(out,
        "Ui::TextLayer::create(): invalid handle Ui::FontHandle(0x12ab, 0x0)\n"
        "Ui::TextLayer::createGlyph(): invalid handle Ui::FontHandle(0x12ab, 0x0)\n"
        "Ui::TextLayer::setText(): invalid handle Ui::FontHandle(0x12ab, 0x0)\n"
        "Ui::TextLayer::setGlyph(): invalid handle Ui::FontHandle(0x12ab, 0x0)\n");
}

void TextLayerTest::nonEditableText() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return Containers::pointer<OneGlyphShaper>(*this); }
    } font;

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};
    cache.addFont(67, &font);

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{1}};
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}},
        {shared.addFont(font, 1.0f)},
        {Text::Alignment{}},
        {}, {}, {}, {}, {}, {});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    DataHandle text = layer.create(0, "", {});
    DataHandle glyph = layer.createGlyph(0, 0, {});

    Containers::String out;
    Error redirectError{&out};
    layer.cursor(text);
    layer.cursor(glyph);
    layer.setCursor(text, 0);
    layer.setCursor(glyph, 0);
    layer.textProperties(text);
    layer.textProperties(glyph);
    layer.text(text);
    layer.text(glyph);
    layer.updateText(text, 0, 0, 0, {}, 0);
    layer.updateText(glyph, 0, 0, 0, {}, 0);
    layer.editText(text, TextEdit::MoveCursorLeft, {});
    layer.editText(glyph, TextEdit::MoveCursorLeft, {});
    CORRADE_COMPARE_AS(out,
        "Ui::TextLayer::cursor(): text doesn't have Ui::TextDataFlag::Editable set\n"
        "Ui::TextLayer::cursor(): text doesn't have Ui::TextDataFlag::Editable set\n"
        "Ui::TextLayer::setCursor(): text doesn't have Ui::TextDataFlag::Editable set\n"
        "Ui::TextLayer::setCursor(): text doesn't have Ui::TextDataFlag::Editable set\n"
        "Ui::TextLayer::textProperties(): text doesn't have Ui::TextDataFlag::Editable set\n"
        "Ui::TextLayer::textProperties(): text doesn't have Ui::TextDataFlag::Editable set\n"
        "Ui::TextLayer::text(): text doesn't have Ui::TextDataFlag::Editable set\n"
        "Ui::TextLayer::text(): text doesn't have Ui::TextDataFlag::Editable set\n"
        "Ui::TextLayer::updateText(): text doesn't have Ui::TextDataFlag::Editable set\n"
        "Ui::TextLayer::updateText(): text doesn't have Ui::TextDataFlag::Editable set\n"
        "Ui::TextLayer::editText(): text doesn't have Ui::TextDataFlag::Editable set\n"
        "Ui::TextLayer::editText(): text doesn't have Ui::TextDataFlag::Editable set\n",
        TestSuite::Compare::String);
}

void TextLayerTest::nonEditableTextTransformation() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return Containers::pointer<OneGlyphShaper>(*this); }
    } font;

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};
    cache.addFont(67, &font);

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{1}};
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}},
        {shared.addFont(font, 1.0f)},
        {Text::Alignment{}},
        {}, {}, {}, {}, {}, {});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared, TextLayerFlag::Transformable} {}
    } layer{layerHandle(0, 1), shared};

    DataHandle data = layer.create(0, "", {});

    Containers::String out;
    Error redirectError{&out};
    layer.create(0, "", {}, TextDataFlag::Editable);
    layer.setText(data, "", {}, TextDataFlag::Editable);
    layer.setText(dataHandleData(data), "", {}, TextDataFlag::Editable);
    CORRADE_COMPARE_AS(out,
        "Ui::TextLayer::create(): cannot use Ui::TextDataFlag::Editable on a Ui::TextLayerFlag::Transformable layer\n"
        "Ui::TextLayer::setText(): cannot use Ui::TextDataFlag::Editable on a Ui::TextLayerFlag::Transformable layer\n"
        "Ui::TextLayer::setText(): cannot use Ui::TextDataFlag::Editable on a Ui::TextLayerFlag::Transformable layer\n",
        TestSuite::Compare::String);
}

void TextLayerTest::noSharedStyleFonts() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return Containers::pointer<OneGlyphShaper>(*this); }
    } font;

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};
    cache.addFont(67, &font);

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{4}
        .setDynamicStyleCount(2)
    };

    FontHandle fontHandle = shared.addFont(font, 1.0f);

    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}, TextLayerStyleUniform{}, TextLayerStyleUniform{}, TextLayerStyleUniform{}},
        {fontHandle, FontHandle::Null, fontHandle, FontHandle::Null},
        {Text::Alignment{}, Text::Alignment{}, Text::Alignment{}, Text::Alignment{}},
        {}, {}, {}, {}, {}, {});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    DataHandle layerData = layer.create(1, "", fontHandle);
    DataHandle layerDataDynamic = layer.create(5, "", fontHandle);

    Containers::String out;
    Error redirectError{&out};
    layer.create(1, "", {});
    layer.create(4, "", {});
    layer.createGlyph(3, 0, {});
    layer.createGlyph(5, 0, {});
    layer.setText(layerData, "", {});
    layer.setText(layerDataDynamic, "", {});
    layer.setGlyph(layerData, 1, {});
    layer.setGlyph(layerDataDynamic, 1, {});
    CORRADE_COMPARE_AS(out,
        "Ui::TextLayer::create(): style 1 has no font set and no custom font was supplied\n"
        "Ui::TextLayer::create(): dynamic style 0 has no font set and no custom font was supplied\n"
        "Ui::TextLayer::createGlyph(): style 3 has no font set and no custom font was supplied\n"
        "Ui::TextLayer::createGlyph(): dynamic style 1 has no font set and no custom font was supplied\n"
        "Ui::TextLayer::setText(): style 1 has no font set and no custom font was supplied\n"
        "Ui::TextLayer::setText(): dynamic style 1 has no font set and no custom font was supplied\n"
        "Ui::TextLayer::setGlyph(): style 1 has no font set and no custom font was supplied\n"
        "Ui::TextLayer::setGlyph(): dynamic style 1 has no font set and no custom font was supplied\n",
        TestSuite::Compare::String);
}

void TextLayerTest::noFontInstance() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};
    UnsignedInt glyphCacheInstanceLessFontId = cache.addFont(233);

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{1}};

    FontHandle fontHandle1 = shared.addInstancelessFont(glyphCacheInstanceLessFontId, 0.1f);
    FontHandle fontHandle2 = shared.addInstancelessFont(glyphCacheInstanceLessFontId, 0.1f);
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}},
        {fontHandle1},
        {Text::Alignment{}},
        {}, {}, {}, {}, {}, {});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    DataHandle data = layer.createGlyph(0, 0, {});

    Containers::String out;
    Error redirectError{&out};
    layer.create(0, "", {});
    layer.create(0, "", fontHandle2);
    layer.setText(data, "", {});
    layer.setText(data, "", fontHandle2);
    CORRADE_COMPARE(out,
        "Ui::TextLayer::create(): Ui::FontHandle(0x0, 0x1) is an instance-less font\n"
        "Ui::TextLayer::create(): Ui::FontHandle(0x1, 0x1) is an instance-less font\n"
        "Ui::TextLayer::setText(): Ui::FontHandle(0x0, 0x1) is an instance-less font\n"
        "Ui::TextLayer::setText(): Ui::FontHandle(0x1, 0x1) is an instance-less font\n");
}

void TextLayerTest::glyphOutOfRange() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};

    /* Add one more font to verify it's checking the right one */
    cache.addFont(57);
    UnsignedInt glyphCacheFontId = cache.addFont(56);

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{2, 3}};

    FontHandle fontHandle = shared.addInstancelessFont(glyphCacheFontId, 1.0f);
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}, TextLayerStyleUniform{}},
        {0, 1, 0},
        {fontHandle, fontHandle, fontHandle},
        {Text::Alignment{}, Text::Alignment{}, Text::Alignment{}},
        {}, {}, {}, {}, {}, {});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    DataHandle data = layer.createGlyph(2, 55, {});

    Containers::String out;
    Error redirectError{&out};
    layer.createGlyph(2, 56, {});
    layer.setGlyph(data, 56, {});
    CORRADE_COMPARE(out,
        "Ui::TextLayer::createGlyph(): glyph 56 out of range for 56 glyphs in glyph cache font 1\n"
        "Ui::TextLayer::setGlyph(): glyph 56 out of range for 56 glyphs in glyph cache font 1\n");
}

void TextLayerTest::updateEmpty() {
    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return {}; }
    } font;

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};
    cache.addFont(56, &font);

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{1}};

    FontHandle fontHandle = shared.addFont(font, 1.0f);
    shared.setStyle(
        TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}},
        {fontHandle},
        {Text::Alignment::MiddleCenter},
        {}, {}, {}, {}, {}, {});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    /* Required to be called before update() (because AbstractUserInterface
       guarantees the same on a higher level), not needed for anything here */
    layer.setSize({1, 1}, {1, 1});

    /* Shouldn't crash or do anything weird */
    layer.update(LayerState::NeedsNodeEnabledUpdate|LayerState::NeedsNodeOpacityUpdate|LayerState::NeedsNodeOrderUpdate|LayerState::NeedsNodeOffsetSizeUpdate|LayerState::NeedsAttachmentUpdate|LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate|LayerState::NeedsSharedDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_VERIFY(true);
}

void TextLayerTest::updateCleanDataOrder() {
    auto&& data = UpdateCleanDataOrderData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Does just extremely basic verification that the vertex and index data
       get filled with correct contents and in correct order. The actual visual
       output is checked in TextLayerGLTest. */

    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return _opened; }
        Properties doOpenFile(Containers::StringView, Float size) override {
            _opened = true;
            return {size, 8.0f, -4.0f, 16.0f, 98};
        }
        void doClose() override { _opened = false; }

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return Containers::pointer<ThreeGlyphShaper>(*this); }

        bool _opened = false;
    } threeGlyphFont;
    threeGlyphFont.openFile({}, 16.0f);

    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return _opened; }
        Properties doOpenFile(Containers::StringView, Float size) override {
            _opened = true;
            return {size, 1.0f, -0.5f, 2.0f, 67};
        }
        void doClose() override { _opened = false; }

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return Containers::pointer<OneGlyphShaper>(*this); }

        bool _opened = false;
    } oneGlyphFont;
    oneGlyphFont.openFile({}, 2.0f);

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    /* Default padding is 1, resetting to 0 for simplicity */
    } cache{PixelFormat::R8Unorm, {32, 32, 3}, {}};

    /* +--+--+
       |66|13|
       +-----+
       | 97  |
       +-----+ */
    {
        UnsignedInt fontId = cache.addFont(threeGlyphFont.glyphCount(), &threeGlyphFont);
        cache.addGlyph(fontId, 97, {8, 4}, 2, {{}, {32, 16}});
        /* Glyph 22 deliberately omitted */
        cache.addGlyph(fontId, 13, {4, -8}, 0, {{16, 16}, {32, 32}});
    } {
        UnsignedInt fontId = cache.addFont(oneGlyphFont.glyphCount(), &oneGlyphFont);
        cache.addGlyph(fontId, 66, {}, 1, {{0, 16}, {16, 32}});
    }

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{6, data.styleCount}
        .setEditingStyleCount(data.editingStyleCount ? 4 : 0, data.editingStyleCount)
        .setDynamicStyleCount(data.dynamicStyleCount, data.hasEditingStyles)
        .setFlags(data.sharedLayerFlags)
    };

    /* The three-glyph font is scaled to 0.5, the one-glyph to 2.0, this scale
       is baked into the quad size and also saved into the scale field if
       TextLayerSharedFlag::DistanceField is enabled */
    FontHandle threeGlyphFontHandle = shared.addFont(threeGlyphFont, 8.0f);
    FontHandle oneGlyphFontHandle = shared.addFont(oneGlyphFont, 4.0f);

    if(data.styleCount == 6) {
        shared.setStyle(TextLayerCommonStyleUniform{},
            /* Last two uniforms only get referenced by editing styles */
            {TextLayerStyleUniform{}, TextLayerStyleUniform{},
             TextLayerStyleUniform{}, TextLayerStyleUniform{},
             TextLayerStyleUniform{}, TextLayerStyleUniform{}},
            /* Style 5 doesn't get used (gets transitioned to 2), use an
               otherwise unused uniform index and weird padding to verify it
               doesn't get picked. The font handle should however match style 2
               as it can't be transitioned. */
            {1, 2, 0, 1, 1, 3},
            {oneGlyphFontHandle, oneGlyphFontHandle, threeGlyphFontHandle, threeGlyphFontHandle, threeGlyphFontHandle, threeGlyphFontHandle},
            {Text::Alignment::MiddleCenter, Text::Alignment::BottomRight, Text::Alignment::MiddleCenter, Text::Alignment::LineLeft, Text::Alignment::TopCenter, Text::Alignment::MiddleCenter},
            /* Features affect only create() / createGlyph() / setText() /
               setGlyph(), not update(); tested in
               createSetTextTextProperties() */
            {}, {}, {},
            {-1,
             data.editingStyle1.first(), /* used by data7 */
             data.editingStyle2.first(), /* used by data3 if not dynamic */
             data.editingStyle3.first(), /* used by data9 */
             -1,
             -1},
            {-1,
             data.editingStyle1.second(),
             data.editingStyle2.second(),
             data.editingStyle3.second(),
             -1,
             -1},
            {{}, {}, data.paddingFromStyle, {}, data.paddingFromStyle, Vector4{666}});
    } else if(data.styleCount == 4) {
        shared.setStyle(TextLayerCommonStyleUniform{},
            /* Last two uniforms only get referenced by editing styles */
            {TextLayerStyleUniform{}, TextLayerStyleUniform{},
             TextLayerStyleUniform{}, TextLayerStyleUniform{},
             TextLayerStyleUniform{}, TextLayerStyleUniform{}},
            {1, 2, 0, 1},
            {oneGlyphFontHandle, oneGlyphFontHandle, threeGlyphFontHandle, threeGlyphFontHandle},
            {Text::Alignment::MiddleCenter, Text::Alignment::BottomRight, Text::Alignment::MiddleCenter, Text::Alignment::LineLeft},
            /* Features affect only create() / createGlyph() / setText() /
               setGlyph(), not update(); tested in
               createSetTextTextProperties() */
            {}, {}, {},
            {-1,
             data.editingStyle1.first(),  /* used by data7 */
             data.editingStyle2.first(),  /* used by data3 if not dynamic */
             data.editingStyle3.first()}, /* used by data9 */
            {-1,
             data.editingStyle1.second(),
             data.editingStyle2.second(),
             data.editingStyle3.second()},
            {{}, {}, data.paddingFromStyle, {}});
    } else CORRADE_INTERNAL_ASSERT_UNREACHABLE();

    if(data.editingStyleCount == 3)
        shared.setEditingStyle(TextLayerCommonEditingStyleUniform{},
            {TextLayerEditingStyleUniform{}, TextLayerEditingStyleUniform{}, TextLayerEditingStyleUniform{}, TextLayerEditingStyleUniform{}},
            {2, 0, 3},
            {5,    /* Style 0 overrides the selection text uniform */
             -1,   /* Style 1 is only used for cursor, so this is unused */
             3},   /* Style 2 is used for selection but doesn't override,
                      redirect to an otherwise-unused slot as well */
            {{0.03f, 0.04f, 0.05f, 0.06f},
             {0.06f, 0.07f, 0.08f, 0.09f},
             {0.01f, 0.02f, 0.03f, 0.04f}});
    else if(data.editingStyleCount == 2)
        shared.setEditingStyle(TextLayerCommonEditingStyleUniform{},
            {TextLayerEditingStyleUniform{}, TextLayerEditingStyleUniform{}, TextLayerEditingStyleUniform{}, TextLayerEditingStyleUniform{}},
            {0, 3},
            {-1,   /* Style 0 is only used for cursor, so this is unused */
             3},   /* Style 1 is used for selection but doesn't override,
                      redirect to an otherwise-unused slot as well */
            {{0.06f, 0.07f, 0.08f, 0.09f},
             {0.01f, 0.02f, 0.03f, 0.04f}});
    else if(data.editingStyleCount == 0) {
        if(data.hasEditingStyles)
            shared.setEditingStyle(TextLayerCommonEditingStyleUniform{},
                {},
                {},
                {});
    } else CORRADE_INTERNAL_ASSERT_UNREACHABLE();

    /* Transition for disabled node6, which is done below through the
       nodesEnabled (that has only node15 enabled) view passed to update() */
    shared.setStyleTransition(
        nullptr,
        nullptr,
        nullptr,
        [](UnsignedInt style) {
            return style == 5 ? 2u : style;
        }
    );

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared, TextLayerFlags flags): TextLayer{handle, shared, flags} {}

        const State& stateData() const {
            return static_cast<const State&>(*_state);
        }
    } layer{layerHandle(0, 1), shared, data.layerFlags};

    /* Required to be called before update() (because AbstractUserInterface
       guarantees the same on a higher level), not needed for anything here */
    layer.setSize({1, 1}, {1, 1});

    if(data.styleCount < 6 && data.dynamicStyleCount) {
        /* Dynamic style 0 and 1 is style 4 and 5, which is used by data3 and
           data5 (so the same case as with padding from non-dynamic style or
           from data) */
        CORRADE_COMPARE(data.styleCount + 0, 4);
        /* Again, features affect only create() / createGlyph() / setText() /
           setGlyph(), not update(); tested in createSetTextTextProperties() */
        layer.setDynamicStyle(0, TextLayerStyleUniform{}, threeGlyphFontHandle, Text::Alignment::TopCenter, {}, data.paddingFromStyle);
        /* setDynamicStyleWithCursor() / setDynamicStyleWithSelection() doesn't
           affect update() any differently than the combined version, these
           were tested well enough in dynamicStyleEditingStyles() above so it's
           just the full-blown function used.

           The data.expectEditingDataPresent is checked to handle the case
           where dynamic editing styles are present but they're not meant to
           be used. */
        if((data.editingStyleCount || data.hasEditingStyles) && data.expectEditingDataPresent)
            layer.setDynamicStyleWithCursorSelection(1,
                TextLayerStyleUniform{},
                threeGlyphFontHandle,
                Text::Alignment::MiddleCenter,
                {},
                data.paddingFromStyle,
                TextLayerEditingStyleUniform{},
                    /* Padding like original dynamic editing style 1 */
                    {0.06f, 0.07f, 0.08f, 0.09f},
                TextLayerEditingStyleUniform{},
                    /* Not bothering with a non-trivial selection style -- the
                       actual dynamic uniform ID used is the same in both cases
                       so it doesn't matter */
                    {},
                    /* Padding like original dynamic editing style 0 */
                    {0.03f, 0.04f, 0.05f, 0.06f});
        else
            layer.setDynamicStyle(1, TextLayerStyleUniform{}, threeGlyphFontHandle, Text::Alignment::MiddleCenter, {}, data.paddingFromStyle);
    }

    /* Two node handles to attach the data to */
    NodeHandle node6 = nodeHandle(6, 0);
    NodeHandle node15 = nodeHandle(15, 0);

    /* Create 11 data handles. Only four get actually used, the rest results in
       a single or no quad */
    layer.create(0, "a", {});                            /* 0, quad 0 */
    layer.create(3, "", {});                             /* 1, no quad */
    layer.create(0, "a", {});                            /* 2, quad 1 */
    /* Node 6 is disabled, so style 5 should get transitioned to 2 if not
       dynamic */
    DataHandle data3 = layer.create(5, "hello",         /* 3, quad 2 to 6 */
        {}, data.dataFlags, node6);
    layer.create(0, "a", {});                           /* 4, quad 7 */
    /* Node 6 is disabled, but style 4 has no disabled transition so this stays
       the same */
    DataHandle data5 = layer.createGlyph(4, 13,         /* 5, quad 8 */
        Text::Alignment::TopCenter, node6);
    layer.create(3, "", {});                            /* 6, no quad */
    DataHandle data7 = layer.create(1, "ahoy",          /* 7, quad 9 */
        Text::Alignment::BottomRight, data.dataFlags, node15);
    layer.create(0, "a", {});                           /* 8, quad 10 */
    DataHandle data9 = layer.create(3, "hi",            /* 9, quad 11 to 12 */
        TextProperties{}
            /* Gets resolved to LineLeft */
            .setAlignment(Text::Alignment::LineEnd)
            /* Swaps horizontal padding of the cursor */
            .setShapeDirection(Text::ShapeDirection::RightToLeft),
        data.dataFlags, node15);
    /* Node 6 is disabled, but style 3 has no disabled transition so this stays
       the same */
    layer.create(3, "",                                 /* 10, no quad */
        TextProperties{}
            .setAlignment(Text::Alignment::MiddleCenter)
            /* Should behave the same as Unspecified */
            .setShapeDirection(Text::ShapeDirection::LeftToRight),
        data.dataFlags, node6);

    /* These are further multiplied by the node opacities */
    layer.setColor(data3, 0xff336699_rgbaf);
    layer.setColor(data5, 0xcceeff00_rgbaf);
    layer.setColor(data7, 0x11223344_rgbaf);
    layer.setColor(data9, 0x663399_rgbf);

    if(!data.paddingOrTranslationFromData.isZero()) {
        if(data.layerFlags >= TextLayerFlag::Transformable) {
            layer.setTransformation(data3, data.paddingOrTranslationFromData.xy(), Complex{}, 1.0f);
            layer.setTransformation(data5, data.paddingOrTranslationFromData.xy(), Complex{}, 1.0f);
        } else {
            layer.setPadding(data3, data.paddingOrTranslationFromData);
            layer.setPadding(data5, data.paddingOrTranslationFromData);
        }
    }

    if(data.dataFlags >= TextDataFlag::Editable) {
        /* Data 3 has both cursor and selection always, the selection may
           change direction though */
        layer.setCursor(data3, data.data3Cursor.first(), data.data3Cursor.second());
        /* Data 7 has only selection, due to no cursor style assigned */
        layer.setCursor(data7, 0, 4);
        /* Data 9 has only a cursor, either due to the selection being empty or
           due to no selection style assigned */
        layer.setCursor(data9, data.data9Cursor.first(), data.data9Cursor.second());
        /* Data 10 has a cursor, no selection because it's empty */
    }

    /* There should be 10 glyph runs, assigned to data that resulted in
       non-zero glyphs */
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().data).slice(&Implementation::TextLayerData::glyphRun), Containers::arrayView({
        0u, 0xffffffffu, 1u, 2u, 3u, 4u, 0xffffffffu, 5u, 6u, 7u, 0xffffffffu
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::glyphOffset), Containers::arrayView({
        0u, 1u, 2u, 7u, 8u, 9u, 10u, 11u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::glyphCount), Containers::arrayView({
        1u, 1u, 5u, 1u, 1u, 1u, 1u, 2u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::data), Containers::arrayView({
        0u, 2u, 3u, 4u, 5u, 7u, 8u, 9u
    }), TestSuite::Compare::Container);

    if(data.dataFlags >= TextDataFlag::Editable) {
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().data).slice(&Implementation::TextLayerData::textRun), Containers::arrayView({
            0xffffffffu, 0xffffffffu, 0xffffffffu, 0u, 0xffffffffu,
            0xffffffffu, 0xffffffffu, 1u, 0xffffffffu, 2u, 3u
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().textRuns).slice(&Implementation::TextLayerTextRun::textOffset), Containers::arrayView({
            0u, 5u, 9u, 11u
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().textRuns).slice(&Implementation::TextLayerTextRun::textSize), Containers::arrayView({
            5u, 4u, 2u, 0u
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().textRuns).slice(&Implementation::TextLayerTextRun::data), Containers::arrayView({
            3u, 7u, 9u, 10u
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(layer.stateData().textData,
            "hello"
            "ahoy"
            "hi"
            "",
            TestSuite::Compare::String);
    }

    Vector2 nodeOffsets[16];
    Vector2 nodeSizes[16];
    Float nodeOpacities[16];
    UnsignedByte nodesEnabledData[2]{};
    Containers::MutableBitArrayView nodesEnabled{nodesEnabledData, 0, 16};
    nodeOffsets[6] = data.node6Offset;
    nodeSizes[6] = data.node6Size;
    nodeOpacities[6] = 0.4f;
    nodeOffsets[15] = {3.0f, 4.0f};
    nodeSizes[15] = {20.0f, 5.0f};
    nodeOpacities[15] = 0.9f;
    nodesEnabled.set(15);

    /* An empty update should generate an empty draw list */
    if(data.emptyUpdate) {
        layer.update(data.states, {}, {}, {}, nodeOffsets, nodeSizes, nodeOpacities, nodesEnabled, {}, {}, {}, {});
        CORRADE_VERIFY(data.expectIndexDataUpdated);
        CORRADE_COMPARE_AS(layer.stateData().indices,
            Containers::ArrayView<const UnsignedInt>{},
            TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(layer.stateData().indexDrawOffsets, (Containers::arrayView<Containers::Pair<UnsignedInt, UnsignedInt>>({
            {0, 0},
        })), TestSuite::Compare::Container);
        return;
    }

    /* Just the filled subset is getting updated, and just what was selected in
       states */
    UnsignedInt dataIds[]{9, 5, 7, 3, 10};
    layer.update(data.states, dataIds, {}, {}, nodeOffsets, nodeSizes, nodeOpacities, nodesEnabled, {}, {}, {}, {});

    if(data.expectIndexDataUpdated) {
        /* The indices should be filled just for the four items */
        CORRADE_COMPARE_AS(layer.stateData().indices, Containers::arrayView<UnsignedInt>({
            /* (Possibly editable) text 9, "hi", quads 11 to 13 */
            11*4 + 0, 11*4 + 1, 11*4 + 2, 11*4 + 2, 11*4 + 1, 11*4 + 3,
            12*4 + 0, 12*4 + 1, 12*4 + 2, 12*4 + 2, 12*4 + 1, 12*4 + 3,
            /* Glyph 5, quad 9 */
             8*4 + 0,  8*4 + 1,  8*4 + 2,  8*4 + 2,  8*4 + 1,  8*4 + 3,
            /* (Possibly editable) text 7, "ahoy", quad 9 */
             9*4 + 0,  9*4 + 1,  9*4 + 2,  9*4 + 2,  9*4 + 1,  9*4 + 3,
            /* (Possibly editable) text 3, "hello", quads 2 to 6 */
             2*4 + 0,  2*4 + 1,  2*4 + 2,  2*4 + 2,  2*4 + 1,  2*4 + 3,
             3*4 + 0,  3*4 + 1,  3*4 + 2,  3*4 + 2,  3*4 + 1,  3*4 + 3,
             4*4 + 0,  4*4 + 1,  4*4 + 2,  4*4 + 2,  4*4 + 1,  4*4 + 3,
             5*4 + 0,  5*4 + 1,  5*4 + 2,  5*4 + 2,  5*4 + 1,  5*4 + 3,
             6*4 + 0,  6*4 + 1,  6*4 + 2,  6*4 + 2,  6*4 + 1,  6*4 + 3,
             /* No glyphs to render for (possibly editable) empty text 10 */
        }), TestSuite::Compare::Container);
        if(data.expectEditingDataPresent)
            /* Yes, the editing quad index order is different from above,
               because the above additionally has to flip from Y-up */
            CORRADE_COMPARE_AS(layer.stateData().editingIndices, Containers::arrayView<UnsignedInt>({
                /* Cursor for text 9, "hi", quad 5 */
                5*4 + 0,  5*4 + 2,  5*4 + 1,  5*4 + 2,  5*4 + 3,  5*4 + 1,
                /* Selection for text 7, quad 2 */
                2*4 + 0,  2*4 + 2,  2*4 + 1,  2*4 + 2,  2*4 + 3,  2*4 + 1,
                /* Selection + cursor for text 3, quads 0 to 1, included only
                   if the style is correctly transitioned for a disabled
                   node */
                0*4 + 0,  0*4 + 2,  0*4 + 1,  0*4 + 2,  0*4 + 3,  0*4 + 1,
                1*4 + 0,  1*4 + 2,  1*4 + 1,  1*4 + 2,  1*4 + 3,  1*4 + 1,
                /* Cursor for text 10, quad 7 */
                7*4 + 0,  7*4 + 2,  7*4 + 1,  7*4 + 2,  7*4 + 3,  7*4 + 1,
            }), TestSuite::Compare::Container);
        else
            CORRADE_COMPARE_AS(layer.stateData().editingIndices, Containers::arrayView<UnsignedInt>({
            }), TestSuite::Compare::Container);

        /* For drawing data 9, 5, 7, 3, 10 it needs to draw the first 2 quads
           in the index buffer, then next 1 quad, then next 1, then next 5,
           then none for the empty text.

           For editing there's a cursor quad for data 9, nothing for data 5,
           selection quad for data 7, cursor + selection quads for data 3 and
           cursor for data 10. */
        if(data.expectEditingDataPresent)
            CORRADE_COMPARE_AS(layer.stateData().indexDrawOffsets, (Containers::arrayView<Containers::Pair<UnsignedInt, UnsignedInt>>({
                {0, 0}, {2*6, 6}, {3*6, 6}, {4*6, 12}, {9*6, 24}, {9*6, 30},
            })), TestSuite::Compare::Container);
        else
            CORRADE_COMPARE_AS(layer.stateData().indexDrawOffsets, (Containers::arrayView<Containers::Pair<UnsignedInt, UnsignedInt>>({
                {0, 0}, {2*6, 0}, {3*6, 0}, {4*6, 0}, {9*6, 0}, {9*6, 0}
            })), TestSuite::Compare::Container);
    }

    if(data.expectVertexDataUpdated) {
        /* Depending on whether TextLayerSharedFlag::DistanceField is enabled
           the vertex data contain a different type. Make a view on the common
           prefix. */
        std::size_t vertexTypeSize = data.sharedLayerFlags >= TextLayerSharedFlag::DistanceField ?
            sizeof(Implementation::TextLayerDistanceFieldVertex) :
            sizeof(Implementation::TextLayerVertex);
        Containers::StridedArrayView1D<const Implementation::TextLayerVertex> vertices{
            layer.stateData().vertices,
            reinterpret_cast<const Implementation::TextLayerVertex*>(layer.stateData().vertices.data()),
            layer.stateData().vertices.size()/vertexTypeSize,
            std::ptrdiff_t(vertexTypeSize)};
        Containers::StridedArrayView1D<const Vector2> positions = vertices.slice(&Implementation::TextLayerVertex::position);
        Containers::StridedArrayView1D<const Vector3> textureCoordinates = vertices.slice(&Implementation::TextLayerVertex::textureCoordinates);
        Containers::StridedArrayView1D<const Float> invertedRunScales =
            data.sharedLayerFlags >= TextLayerSharedFlag::DistanceField ?
                stridedArrayView(Containers::arrayCast<Implementation::TextLayerDistanceFieldVertex>(layer.stateData().vertices)).slice(&Implementation::TextLayerDistanceFieldVertex::invertedRunScale) : nullptr;

        /* The vertices are there for all data that have glyphs, but only the
           actually used are filled */
        CORRADE_COMPARE(layer.stateData().vertices.size(), 13*4* vertexTypeSize);
        /* (Possibly editable) text 3, quads 2 to 6 */
        for(std::size_t i = 0; i != 5*4; ++i) {
            CORRADE_ITERATION(i);
            CORRADE_COMPARE(vertices[2*4 + i].color, 0xff336699_rgbaf*0.4f);
            if(data.sharedLayerFlags >= TextLayerSharedFlag::DistanceField)
                CORRADE_COMPARE(invertedRunScales[2*4 + i], 1.0f/0.5f); /* threeGlyphFont */
        }
        /* Created with style 5, which if not dynamic is transitioned to 2 as
           the node is disabled, which is mapped to uniform 0. If dynamic, it's
           implicitly `uniformCount + (id - styleCount)`, thus 7. If editable,
           quad 2 to 5 is switched to uniform 5, or if dynamic,
           `dynamicUniformCount + (id - styleCount)*2`, thus 10. */
        for(std::size_t j = 0; j != 4; ++j) {
            CORRADE_ITERATION(j);
            for(std::size_t i: {0, 1}) {
                CORRADE_ITERATION(i);
                if(data.styleCount == 6)
                    CORRADE_COMPARE(vertices[2*4 + i*4 + j].styleUniform, 0);
                else if(data.styleCount == 4)
                    CORRADE_COMPARE(vertices[2*4 + i*4 + j].styleUniform, 7);
                else CORRADE_INTERNAL_ASSERT_UNREACHABLE();
            }
            for(std::size_t i: {2, 3, 4}) {
                CORRADE_ITERATION(i);
                if(data.styleCount == 6)
                    CORRADE_COMPARE(vertices[2*4 + i*4 + j].styleUniform, data.expectEditingDataPresent ? 5 : 0);
                else if(data.styleCount == 4)
                    CORRADE_COMPARE(vertices[2*4 + i*4 + j].styleUniform, data.expectEditingDataPresent ? 10 : 7);
                else CORRADE_INTERNAL_ASSERT_UNREACHABLE();
            }
        }
        /* Glyph 5, quad 8 */
        for(std::size_t i = 0; i != 1*4; ++i) {
            CORRADE_ITERATION(i);
            CORRADE_COMPARE(vertices[8*4 + i].color, 0xcceeff00_rgbaf*0.4f);
            if(data.sharedLayerFlags >= TextLayerSharedFlag::DistanceField)
                CORRADE_COMPARE(invertedRunScales[8*4 + i], 1.0f/0.5f); /* threeGlyphFont */
            /* Created with style 4, which if not dynamic is mapped to uniform
               1. If dynamic, it's implicitly `uniformCount + (id - styleCount)`,
               thus 6. */
            if(data.styleCount == 6)
                CORRADE_COMPARE(vertices[8*4 + i].styleUniform, 1);
            else if(data.styleCount == 4)
                CORRADE_COMPARE(vertices[8*4 + i].styleUniform, 6);
            else CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        }
        /* (Possibly editable) text 7, quad 9 */
        for(std::size_t i = 0; i != 1*4; ++i) {
            CORRADE_ITERATION(i);
            CORRADE_COMPARE(vertices[9*4 + i].color, 0x11223344_rgbaf*0.9f);
            if(data.sharedLayerFlags >= TextLayerSharedFlag::DistanceField)
                CORRADE_COMPARE(invertedRunScales[9*4 + i], 1.0f/2.0f); /* oneGlyphFont */
            /* Created with style 1, which is mapped to uniform 2. The
               selection doesn't override the text uniform, so it's always the
               same. */
            CORRADE_COMPARE(vertices[9*4 + i].styleUniform, 2);
        }
        /* (Possibly editable) text 9, quads 11 to 12 */
        for(std::size_t i = 0; i != 2*4; ++i) {
            CORRADE_ITERATION(i);
            CORRADE_COMPARE(vertices[11*4 + i].color, 0x663399ff_rgbaf*0.9f);
            if(data.sharedLayerFlags >= TextLayerSharedFlag::DistanceField)
                CORRADE_COMPARE(invertedRunScales[11*4 + i], 1.0f/0.5f); /* threeGlyphFont */
            /* Created with style 3, which is mapped to uniform 1. There's only
               a cursor, which doesn't override the text uniform, so it's
               always the same. */
            CORRADE_COMPARE(vertices[11*4 + i].styleUniform, 1);
        }
        /* (Possibly editable) text 10 is empty */

        /* Text 3 and glyph 5 are attached to node 6, which has offset
           {1.0, 2.0} and a center of {6.0, 9.5}. Shaped positions should match
           what's in create() however as the coordinate system has Y up, the
           glyph positions have Y flipped compared in comparison to create():

            2--3
            |  |
            0--1 */
        CORRADE_COMPARE_AS(positions.sliceSize(2*4, 5*4), Containers::arrayView<Vector2>({
            /* Glyph 22, not in cache */
            {6.0f - 5.0f,               9.5f + 0.5f},
            {6.0f - 5.0f,               9.5f + 0.5f},
            {6.0f - 5.0f,               9.5f + 0.5f},
            {6.0f - 5.0f,               9.5f + 0.5f},

            /* Glyph 13. Offset {4, -8}, size {16, 16}, scaled to 0.5. */
            {6.0f - 3.5f + 2.0f + 0.0f, 9.5f - 0.0f + 4.0f - 0.0f},
            {6.0f - 3.5f + 2.0f + 8.0f, 9.5f - 0.0f + 4.0f - 0.0f},
            {6.0f - 3.5f + 2.0f + 0.0f, 9.5f - 0.0f + 4.0f - 8.0f},
            {6.0f - 3.5f + 2.0f + 8.0f, 9.5f - 0.0f + 4.0f - 8.0f},

            /* Glyph 97. Offset {8, 4}, size {32, 16}, scaled to 0.5. */
            {6.0f - 1.5f + 4.0f + 0.0f, 9.5f - 0.5f - 2.0f - 0.0f},
            {6.0f - 1.5f + 4.0f + 16.f, 9.5f - 0.5f - 2.0f - 0.0f},
            {6.0f - 1.5f + 4.0f + 0.0f, 9.5f - 0.5f - 2.0f - 8.0f},
            {6.0f - 1.5f + 4.0f + 16.f, 9.5f - 0.5f - 2.0f - 8.0f},

            /* Glyph 22, not in cache */
            {6.0f + 1.0f,               9.5f - 1.0f},
            {6.0f + 1.0f,               9.5f - 1.0f},
            {6.0f + 1.0f,               9.5f - 1.0f},
            {6.0f + 1.0f,               9.5f - 1.0f},

            /* Glyph 13 again */
            {6.0f + 4.0f + 2.0f + 0.0f, 9.5f - 1.5f + 4.0f - 0.0f},
            {6.0f + 4.0f + 2.0f + 8.0f, 9.5f - 1.5f + 4.0f - 0.0f},
            {6.0f + 4.0f + 2.0f + 0.0f, 9.5f - 1.5f + 4.0f - 8.0f},
            {6.0f + 4.0f + 2.0f + 8.0f, 9.5f - 1.5f + 4.0f - 8.0f},
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(positions.sliceSize(8*4, 1*4), Containers::arrayView<Vector2>({
            /* Glyph 13 again, but TopCenter */
            {6.0f - 4.0f        + 0.0f, 10.0f             - 0.0f},
            {6.0f - 4.0f        + 8.0f, 10.0f             - 0.0f},
            {6.0f - 4.0f        + 0.0f, 10.0f             - 8.0f},
            {6.0f - 4.0f        + 8.0f, 10.0f             - 8.0f},
        }), TestSuite::Compare::Container);

        /* Text 7 and 9 are both attached to node 15, which has offset {3, 4}
           and size {20, 5}. They're aligned to BottomRight and LineLeft
           respectively, instead of MiddleCenter. */
        CORRADE_COMPARE_AS(positions.sliceSize(9*4, 1*4), Containers::arrayView<Vector2>({
            /* Glyph 66. No offset, size {16, 16}, scaled to 2.0. */
            {23.f - 2.0f       + 0.0f, 9.0f + 0.0f        - 0.0f},
            {23.f - 2.0f       + 32.f, 9.0f + 0.0f        - 0.0f},
            {23.f - 2.0f       + 0.0f, 9.0f + 0.0f        - 32.f},
            {23.f - 2.0f       + 32.f, 9.0f + 0.0f        - 32.f},
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(positions.sliceSize(11*4, 2*4), Containers::arrayView<Vector2>({
            /* Glyph 22, not in cache */
            {3.0f + 0.0f,              6.5f - 0.5f},
            {3.0f + 0.0f,              6.5f - 0.5f},
            {3.0f + 0.0f,              6.5f - 0.5f},
            {3.0f + 0.0f,              6.5f - 0.5f},

            /* Glyph 13. Offset {4, -8}, size {16, 16}, scaled to 0.5. */
            {3.0f + 1.5f + 2.f + 0.0f, 6.5f - 1.0f + 4.0f - 0.0f},
            {3.0f + 1.5f + 2.f + 8.0f, 6.5f - 1.0f + 4.0f - 0.0f},
            {3.0f + 1.5f + 2.f + 0.0f, 6.5f - 1.0f + 4.0f - 8.0f},
            {3.0f + 1.5f + 2.f + 8.0f, 6.5f - 1.0f + 4.0f - 8.0f},
        }), TestSuite::Compare::Container);

        /* Text 10 is empty */

        /* Texture coordinates however stay the same, with Y up:

            +--+--+
            |66|13|
            3-----2
            | 97  |
            0-----1 */

        /* Glyph 22, at quad 2, 5, 11, isn't in cache */
        for(std::size_t i: {2, 5, 11}) {
            CORRADE_COMPARE_AS(textureCoordinates.sliceSize(i*4, 4), Containers::arrayView<Vector3>({
                {},
                {},
                {},
                {},
            }), TestSuite::Compare::Container);
        }

        /* Glyph 13, at quad 3, 6, 8, 12 */
        for(std::size_t i: {3, 6, 8, 12}) {
            CORRADE_COMPARE_AS(textureCoordinates.sliceSize(i*4, 4), Containers::arrayView<Vector3>({
                {0.5f, 0.5f, 0.0f},
                {1.0f, 0.5f, 0.0f},
                {0.5f, 1.0f, 0.0f},
                {1.0f, 1.0f, 0.0f},
            }), TestSuite::Compare::Container);
        }

        /* Glyph 66, at quad 9 */
        CORRADE_COMPARE_AS(textureCoordinates.sliceSize(9*4, 4), Containers::arrayView<Vector3>({
            {0.0f, 0.5f, 1.0f},
            {0.5f, 0.5f, 1.0f},
            {0.0f, 1.0f, 1.0f},
            {0.5f, 1.0f, 1.0f},
        }), TestSuite::Compare::Container);

        /* Glyph 97, at quad 4 */
        CORRADE_COMPARE_AS(textureCoordinates.sliceSize(4*4, 4), Containers::arrayView<Vector3>({
            {0.0f, 0.0f, 2.0f},
            {1.0f, 0.0f, 2.0f},
            {0.0f, 0.5f, 2.0f},
            {1.0f, 0.5f, 2.0f},
        }), TestSuite::Compare::Container);
    }

    if(data.expectVertexDataUpdated && data.expectEditingDataPresent) {
        /* Uniform IDs for all data */
        for(std::size_t i = 0; i != 4; ++i) {
            CORRADE_ITERATION(i);

            /* Text 3 selection (quad 0) has editing style 0, which maps
               to uniform 2, cursor (quad 1) then style 1 which maps to uniform
               0. If dynamic, then cursor maps to
               `editingStyleCount + (dynamicStyleId - styleCount)*2 + 0`,
               selection to
               `editingStyleCount + (dynamicStyleId - styleCount)*2 + 1`. */
            CORRADE_COMPARE(layer.stateData().editingVertices[0*4 + i].opacity, 0.4f);
            CORRADE_COMPARE(layer.stateData().editingVertices[0*4 + i].styleUniform, data.dynamicStyleCount ? 6 : 2);
            CORRADE_COMPARE(layer.stateData().editingVertices[1*4 + i].opacity, 0.4f);
            CORRADE_COMPARE(layer.stateData().editingVertices[1*4 + i].styleUniform, data.dynamicStyleCount ? 7 : 0);

            /* Text 7 selection (quad 2) has style 1 which maps to uniform 0 */
            CORRADE_COMPARE(layer.stateData().editingVertices[2*4 + i].opacity, 0.9f);
            CORRADE_COMPARE(layer.stateData().editingVertices[2*4 + i].styleUniform, 0);

            /* Text 9 cursor (quad 5) has style 2 which maps to uniform 3 */
            CORRADE_COMPARE(layer.stateData().editingVertices[5*4 + i].opacity, 0.9f);
            CORRADE_COMPARE(layer.stateData().editingVertices[5*4 + i].styleUniform, 3);

            /* Text 10 cursor (quad 7) has style 2 which maps to uniform 3 */
            CORRADE_COMPARE(layer.stateData().editingVertices[7*4 + i].opacity, 0.4f);
            CORRADE_COMPARE(layer.stateData().editingVertices[7*4 + i].styleUniform, 3);
        }

        Containers::StridedArrayView1D<const Vector2> editingPositions = stridedArrayView(layer.stateData().editingVertices).slice(&Implementation::TextLayerEditingVertex::position);
        Containers::StridedArrayView1D<const Vector2> editingCenterDistances = stridedArrayView(layer.stateData().editingVertices).slice(&Implementation::TextLayerEditingVertex::centerDistance);

        /* Text 3 selection starts at glyph 2 and ends at glyph 5, cursor is at
           either end. Quads 0 and 1. Again, like the indices, the vertices are
           in a different order compared to text quads as there's no Y-flip
           happening.

            0--1
            |  |
            2--3 */
        CORRADE_COMPARE_AS(editingPositions.sliceSize(0*4, 2*4), Containers::arrayView<Vector2>({
            /* Matches glyph 2 position and glyph 5 position + advance of 6*0.5
               on X without the additional offset; on Y it matches the
               threeGlyphFont ascent 8*0.5 and descent -4*0.5, which aligned
               to MiddleCenter is shifted +1.0 on Y. Then (left, top, right,
               bottom) paddings from editing style 0, which *expand* the quad
               contrary to what the usual style paddings do here and in
               BaseLayer. */
            {6.0f - 1.5f        - 0.03f, 9.5f - 4.0f + 1.0f - 0.04f},
            {6.0f + 2.0f + 3.0f + 0.05f, 9.5f - 4.0f + 1.0f - 0.04f},
            {6.0f - 1.5f        - 0.03f, 9.5f + 2.0f + 1.0f + 0.06f},
            {6.0f + 2.0f + 3.0f + 0.05f, 9.5f + 2.0f + 1.0f + 0.06f},

            /* Left or right edge of above, plus padding from style 1 */
            {6.0f + (data.data3Cursor.first() < data.data3Cursor.second() ? -1.5f : + 2.0f + 3.0f)
                                - 0.06f, 9.5f - 4.0f + 1.0f - 0.07f},
            {6.0f + (data.data3Cursor.first() < data.data3Cursor.second() ? -1.5f : + 2.0f + 3.0f)
                                + 0.08f, 9.5f - 4.0f + 1.0f - 0.07f},
            {6.0f + (data.data3Cursor.first() < data.data3Cursor.second() ? -1.5f : + 2.0f + 3.0f)
                                - 0.06f, 9.5f + 2.0f + 1.0f + 0.09f},
            {6.0f + (data.data3Cursor.first() < data.data3Cursor.second() ? -1.5f : + 2.0f + 3.0f)
                                + 0.08f, 9.5f + 2.0f + 1.0f + 0.09f},
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(editingCenterDistances.sliceSize(0*4, 2*4), Containers::arrayView<Vector2>({
            /* The selection quad size is {6.5, 6}, padding size
               {0.08, 0.10} */
            {-3.25f             - 0.04f, -3.0f              - 0.05f},
            {+3.25f             + 0.04f, -3.0f              - 0.05f},
            {-3.25f             - 0.04f, +3.0f              + 0.05f},
            {+3.25f             + 0.04f, +3.0f              + 0.05f},

            /* Cursor quad size on X is 0, on Y same as selection, padding
               size is (0.14, 0.16) */
            { 0.0f              - 0.07f, -3.0f              - 0.08f},
            { 0.0f              + 0.07f, -3.0f              - 0.08f},
            { 0.0f              - 0.07f, +3.0f              + 0.08f},
            { 0.0f              + 0.07f, +3.0f              + 0.08f},
        }), TestSuite::Compare::Container);

        /* Text 7 selection (quad 2) is the whole text, so again matching the X
           position w/o offset, and advance of 2.5*2.0; on Y the oneGlyphFont
           ascent 1*2.0 and descent -0.5*2.0. Then paddings from style 1. */
        CORRADE_COMPARE_AS(editingPositions.sliceSize(2*4, 1*4), Containers::arrayView<Vector2>({
            /** @todo here it incorrectly marks the begin of the glyph as 21,
                while that's including the base X offset, and should be 18
                instead -- would need to store the X offset separately
                internally. Y offset is already ignored because only X glyph
                position is taken into account */
            {23.f - 2.0f        - 0.06f, 9.0f - 3.0f        - 0.07f},
            {23.f               + 0.08f, 9.0f - 3.0f        - 0.07f},
            {23.f - 2.0f        - 0.06f, 9.0f               + 0.09f},
            {23.f               + 0.08f, 9.0f               + 0.09f},
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(editingCenterDistances.sliceSize(2*4, 1*4), Containers::arrayView<Vector2>({
            /* The selection quad size is {2, 3}, padding size (0.14, 0.16) */
            {-1.0f              - 0.07f, -1.5f              - 0.08f},
            {+1.0f              + 0.07f, -1.5f              - 0.08f},
            {-1.0f              - 0.07f, +1.5f              + 0.08f},
            {+1.0f              + 0.07f, +1.5f              + 0.08f},
        }), TestSuite::Compare::Container);

        /* Text 9 cursor (quad 5) is at glyph 1 position on X w/o offset, on Y
           it's again the threeGlyphFont ascent 8*0.5 and descent -4*0.5. Here
           the alignment is LineLeft, so no additional Y shift. Then paddings
           from style 2, but ordered ZYXW instead of XYZW because the text is
           RTL. */
        CORRADE_COMPARE_AS(editingPositions.sliceSize(5*4, 1*4), Containers::arrayView<Vector2>({
            {3.0f + 1.5f        - 0.03f, 6.5f - 4.0f        - 0.02f},
            {3.0f + 1.5f        + 0.01f, 6.5f - 4.0f        - 0.02f},
            {3.0f + 1.5f        - 0.03f, 6.5f + 2.0f        + 0.04f},
            {3.0f + 1.5f        + 0.01f, 6.5f + 2.0f        + 0.04f},
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(editingCenterDistances.sliceSize(5*4, 1*4), Containers::arrayView<Vector2>({
            /* Cursor quad size on X is 0, on Y 6, padding size (0.04, 0.06) */
            { 0.0f              - 0.02f, -3.0f              - 0.03f},
            { 0.0f              + 0.02f, -3.0f              - 0.03f},
            { 0.0f              - 0.02f, +3.0f              + 0.03f},
            { 0.0f              + 0.02f, +3.0f              + 0.03f},
        }), TestSuite::Compare::Container);

        /* Text 10 cursor (quad 7) is at the center of node 6, thus {6.0, 9.5}.
           On Y it's the threeGlyphFont ascent 8*0.5 and descent -4*0.5, which aligned to MiddleCenter is shifted +1.0 on Y. Then paddings from
           style 2. */
        CORRADE_COMPARE_AS(editingPositions.sliceSize(7*4, 1*4), Containers::arrayView<Vector2>({
            {6.0f               - 0.01f, 9.5f - 4.0f + 1.0f - 0.02f},
            {6.0f               + 0.03f, 9.5f - 4.0f + 1.0f - 0.02f},
            {6.0f               - 0.01f, 9.5f + 2.0f + 1.0f + 0.04f},
            {6.0f               + 0.03f, 9.5f + 2.0f + 1.0f + 0.04f},
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(editingCenterDistances.sliceSize(7*4, 1*4), Containers::arrayView<Vector2>({
            { 0.0f              - 0.02f, -3.0f              - 0.03f},
            { 0.0f              + 0.02f, -3.0f              - 0.03f},
            { 0.0f              - 0.02f, +3.0f              + 0.03f},
            { 0.0f              + 0.02f, +3.0f              + 0.03f},
        }), TestSuite::Compare::Container);
    }

    /* Removing a node with cleanNodes() marks the corresponding run as unused,
       and update() recompacts again */
    {
        UnsignedShort nodeGenerations[16];
        nodeGenerations[6] = nodeHandleGeneration(node6) + 1;
        nodeGenerations[15] = nodeHandleGeneration(node15);
        layer.cleanNodes(nodeGenerations);
        /* Node 6 was disabled before already, so the nodesEnabled mask doesn't
           need to be updated */
    }

    /* The run corresponding to the removed data should be marked as unused,
       the rest stays the same */
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().data).slice(&Implementation::TextLayerData::glyphRun), Containers::arrayView({
        0u, 0xffffffffu, 1u, 2u, 3u, 4u, 0xffffffffu, 5u, 6u, 7u, 0xffffffffu
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::glyphOffset), Containers::arrayView({
        0u, 1u, 0xffffffffu, 7u, 0xffffffffu, 9u, 10u, 11u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::glyphCount), Containers::arrayView({
        1u, 1u, 5u, 1u, 1u, 1u, 1u, 2u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::data), Containers::arrayView({
        0u, 2u, 3u, 4u, 5u, 7u, 8u, 9u
    }), TestSuite::Compare::Container);

    if(data.dataFlags >= TextDataFlag::Editable) {
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().data).slice(&Implementation::TextLayerData::textRun), Containers::arrayView({
            0xffffffffu, 0xffffffffu, 0xffffffffu, 0u, 0xffffffffu,
            0xffffffffu, 0xffffffffu, 1u, 0xffffffffu, 2u, 3u
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().textRuns).slice(&Implementation::TextLayerTextRun::textOffset), Containers::arrayView({
            0xffffffffu, 5u, 9u, 0xffffffffu
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().textRuns).slice(&Implementation::TextLayerTextRun::textSize), Containers::arrayView({
            5u, 4u, 2u, 0u
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().textRuns).slice(&Implementation::TextLayerTextRun::data), Containers::arrayView({
            3u, 7u, 9u, 10u
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(layer.stateData().textData,
            "hello" /* now unused */
            "ahoy"
            "hi"
            "", /* now unused */
            TestSuite::Compare::String);
    }

    /* Note that this adds LayerState::NeedsDataUpdate in order to force the
       glyph run recompaction, thus we also don't branch on
       data.expectIndexDataUpdated / data.expectVertexDataUpdated anymore */
    UnsignedInt dataIdsPostClean[]{9, 7};
    layer.update(data.states|LayerState::NeedsDataUpdate, dataIdsPostClean, {}, {}, nodeOffsets, nodeSizes, nodeOpacities, nodesEnabled, {}, {}, {}, {});

    /* Like in the expectVertexDataUpdated branch above, but making the view
       again because it could get reallocated here if it was empty before. It
       however won't get reallocated after this point, since it only shrinks
       now. */
    std::size_t vertexTypeSize = data.sharedLayerFlags >= TextLayerSharedFlag::DistanceField ?
        sizeof(Implementation::TextLayerDistanceFieldVertex) :
        sizeof(Implementation::TextLayerVertex);
    Containers::StridedArrayView1D<const Implementation::TextLayerVertex> vertices{
        layer.stateData().vertices,
        reinterpret_cast<const Implementation::TextLayerVertex*>(layer.stateData().vertices.data()),
        layer.stateData().vertices.size()/vertexTypeSize,
        std::ptrdiff_t(vertexTypeSize)};
    Containers::StridedArrayView1D<const Vector2> positions = vertices.slice(&Implementation::TextLayerVertex::position);
    Containers::StridedArrayView1D<const Vector3> textureCoordinates = vertices.slice(&Implementation::TextLayerVertex::textureCoordinates);
    Containers::StridedArrayView1D<const Float> invertedRunScales =
        data.sharedLayerFlags >= TextLayerSharedFlag::DistanceField ?
            stridedArrayView(Containers::arrayCast<Implementation::TextLayerDistanceFieldVertex>(layer.stateData().vertices)).slice(&Implementation::TextLayerDistanceFieldVertex::invertedRunScale) : nullptr;

    /* There should be just 6 glyph runs, assigned to the remaining 9 data */
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().data).slice(&Implementation::TextLayerData::glyphRun), Containers::arrayView({
        0u, 0xffffffffu /* no run */, 1u, 2u /* free data */, 2u,
        4u /* free data */, 0xffffffffu /* no run */, 3u, 4u, 5u,
        0xffffffffu /* free data */
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::glyphOffset), Containers::arrayView({
        0u, 1u, 2u, 3u, 4u, 5u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::glyphCount), Containers::arrayView({
        1u, 1u, 1u, 1u, 1u, 2u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::data), Containers::arrayView({
        0u, 2u, 4u, 7u, 8u, 9u
    }), TestSuite::Compare::Container);

    /* The glyph count queries should still match */
    CORRADE_COMPARE(layer.glyphCount(data7), 1u);
    CORRADE_COMPARE(layer.glyphCount(data9), 2u);

    /* And 2 text runs, assigned to the remining 2 texts */
    if(data.dataFlags >= TextDataFlag::Editable) {
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().data).slice(&Implementation::TextLayerData::textRun), Containers::arrayView({
            0xffffffffu, 0xffffffffu, 0xffffffffu, 0u /* free data */,
            0xffffffffu, 0xffffffffu, 0xffffffffu, 0u, 0xffffffffu, 1u,
            3u /* free data */
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().textRuns).slice(&Implementation::TextLayerTextRun::textOffset), Containers::arrayView({
            0u, 4u
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().textRuns).slice(&Implementation::TextLayerTextRun::textSize), Containers::arrayView({
            4u, 2u
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().textRuns).slice(&Implementation::TextLayerTextRun::data), Containers::arrayView({
            7u, 9u
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(layer.stateData().textData,
            "ahoy"
            "hi",
            TestSuite::Compare::String);

        /* The text queries should still match also */
        CORRADE_COMPARE(layer.text(data7), "ahoy");
        CORRADE_COMPARE(layer.text(data9), "hi");
    }

    /* Indices for remaining 3 visible glyphs */
    CORRADE_COMPARE_AS(layer.stateData().indices, Containers::arrayView<UnsignedInt>({
        /* (Possibly editable) text 9, "hi", quads 5 to 6 */
        5*4 + 0, 5*4 + 1, 5*4 + 2, 5*4 + 2, 5*4 + 1, 5*4 + 3,
        6*4 + 0, 6*4 + 1, 6*4 + 2, 6*4 + 2, 6*4 + 1, 6*4 + 3,
        /* (Possibly editable) text 7, "ahoy", quad 3 */
        3*4 + 0, 3*4 + 1, 3*4 + 2, 3*4 + 2, 3*4 + 1, 3*4 + 3,
        /* Text 3, "hello" is removed now */
        /* Glyph 5 is removed now */
    }), TestSuite::Compare::Container);
    if(data.expectEditingDataPresent)
        CORRADE_COMPARE_AS(layer.stateData().editingIndices, Containers::arrayView<UnsignedInt>({
            /* Cursor for text 9, "hi", quad 3 */
            3*4 + 0,  3*4 + 2,  3*4 + 1,  3*4 + 2,  3*4 + 3,  3*4 + 1,
            /* Selection for text 7, quad 0 */
            0*4 + 0,  0*4 + 2,  0*4 + 1,  0*4 + 2,  0*4 + 3,  0*4 + 1,
        }), TestSuite::Compare::Container);
    else
        CORRADE_COMPARE_AS(layer.stateData().editingIndices, Containers::arrayView<UnsignedInt>({
        }), TestSuite::Compare::Container);

    /* For drawing data 9 and 7 it needs to draw the first 2 quads in the
       index buffer, then next 1 quad. For editing there's a cursor quad for
       data 9 and a selection quad for data 7. */
    if(data.expectEditingDataPresent)
        CORRADE_COMPARE_AS(layer.stateData().indexDrawOffsets, (Containers::arrayView<Containers::Pair<UnsignedInt, UnsignedInt>>({
            {0, 0}, {2*6, 6}, {3*6, 12}
        })), TestSuite::Compare::Container);
    else
        CORRADE_COMPARE_AS(layer.stateData().indexDrawOffsets, (Containers::arrayView<Containers::Pair<UnsignedInt, UnsignedInt>>({
            {0, 0}, {2*6, 0}, {3*6, 0}
        })), TestSuite::Compare::Container);

    /* Vertices for all remaining 7 glyphs */
    CORRADE_COMPARE(layer.stateData().vertices.size(), 7*4*vertexTypeSize);
    /* (Possibly editable) text 7, quad 3 */
    for(std::size_t i = 0; i != 1*4; ++i) {
        CORRADE_ITERATION(i);
        CORRADE_COMPARE(vertices[3*4 + i].color, 0x11223344_rgbaf*0.9f);
        if(data.sharedLayerFlags >= TextLayerSharedFlag::DistanceField)
            CORRADE_COMPARE(invertedRunScales[3*4 + i], 1.0f/2.0f); /* oneGlyphFont */
        /* Created with style 1, which is mapped to uniform 2. The selection
           doesn't override the text uniform, so it's always the same. */
        CORRADE_COMPARE(vertices[3*4 + i].styleUniform, 2);
    }
    /* (Possibly editable) text 9, quads 5 to 6 */
    for(std::size_t i = 0; i != 2*4; ++i) {
        CORRADE_ITERATION(i);
        CORRADE_COMPARE(vertices[5*4 + i].color, 0x663399ff_rgbaf*0.9f);
        if(data.sharedLayerFlags >= TextLayerSharedFlag::DistanceField)
            CORRADE_COMPARE(invertedRunScales[5*4 + i], 1.0f/0.5f); /* threeGlyphFont */
        /* Created with style 3, which is mapped to uniform 1. There's only a
           cursor, which doesn't override the text uniform, so it's always the
           same. */
        CORRADE_COMPARE(vertices[5*4 + i].styleUniform, 1);
    }

    /* Text 7 and 9, now quads 3 and 5 to 6 */
    CORRADE_COMPARE_AS(positions.sliceSize(3*4, 1*4), Containers::arrayView<Vector2>({
        {23.f - 2.0f       + 0.0f, 9.0f + 0.0f        - 0.0f},
        {23.f - 2.0f       + 32.f, 9.0f + 0.0f        - 0.0f},
        {23.f - 2.0f       + 0.0f, 9.0f + 0.0f        - 32.f},
        {23.f - 2.0f       + 32.f, 9.0f + 0.0f        - 32.f},
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(positions.sliceSize(5*4, 2*4), Containers::arrayView<Vector2>({
        {3.0f + 0.0f,              6.5f - 0.5f},
        {3.0f + 0.0f,              6.5f - 0.5f},
        {3.0f + 0.0f,              6.5f - 0.5f},
        {3.0f + 0.0f,              6.5f - 0.5f},

        {3.0f + 1.5f + 2.f + 0.0f, 6.5f - 1.0f + 4.0f - 0.0f},
        {3.0f + 1.5f + 2.f + 8.0f, 6.5f - 1.0f + 4.0f - 0.0f},
        {3.0f + 1.5f + 2.f + 0.0f, 6.5f - 1.0f + 4.0f - 8.0f},
        {3.0f + 1.5f + 2.f + 8.0f, 6.5f - 1.0f + 4.0f - 8.0f},
    }), TestSuite::Compare::Container);

    /* Glyph 22, now only at quad 5 */
    CORRADE_COMPARE_AS(textureCoordinates.sliceSize(5*4, 4), Containers::arrayView<Vector3>({
        {},
        {},
        {},
        {},
    }), TestSuite::Compare::Container);

    /* Glyph 13, now only at quad 6 */
    CORRADE_COMPARE_AS(textureCoordinates.sliceSize(6*4, 4), Containers::arrayView<Vector3>({
        {0.5f, 0.5f, 0.0f},
        {1.0f, 0.5f, 0.0f},
        {0.5f, 1.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
    }), TestSuite::Compare::Container);

    /* Glyph 66, now at quad 3 */
    CORRADE_COMPARE_AS(textureCoordinates.sliceSize(3*4, 4), Containers::arrayView<Vector3>({
        {0.0f, 0.5f, 1.0f},
        {0.5f, 0.5f, 1.0f},
        {0.0f, 1.0f, 1.0f},
        {0.5f, 1.0f, 1.0f},
    }), TestSuite::Compare::Container);

    if(data.expectEditingDataPresent) {
        for(std::size_t i = 0; i != 4; ++i) {
            CORRADE_ITERATION(i);

            /* Text 7 selection, now quad 0 */
            CORRADE_COMPARE(layer.stateData().editingVertices[0*4 + i].opacity, 0.9f);
            CORRADE_COMPARE(layer.stateData().editingVertices[0*4 + i].styleUniform, 0);

            /* Text 9 cursor, now quad 3 */
            CORRADE_COMPARE(layer.stateData().editingVertices[3*4 + i].opacity, 0.9f);
            CORRADE_COMPARE(layer.stateData().editingVertices[3*4 + i].styleUniform, 3);
        }

        Containers::StridedArrayView1D<const Vector2> editingPositions = stridedArrayView(layer.stateData().editingVertices).slice(&Implementation::TextLayerEditingVertex::position);
        Containers::StridedArrayView1D<const Vector2> editingCenterDistances = stridedArrayView(layer.stateData().editingVertices).slice(&Implementation::TextLayerEditingVertex::centerDistance);

        /* Text 7 selection, now quad 0 */
        CORRADE_COMPARE_AS(editingPositions.sliceSize(0*4, 1*4), Containers::arrayView<Vector2>({
            /** @todo here it again incorrectly marks the begin including the
                base X offset */
            {23.f - 2.0f        - 0.06f, 9.0f - 3.0f        - 0.07f},
            {23.f               + 0.08f, 9.0f - 3.0f        - 0.07f},
            {23.f - 2.0f        - 0.06f, 9.0f               + 0.09f},
            {23.f               + 0.08f, 9.0f               + 0.09f},
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(editingCenterDistances.sliceSize(0*4, 1*4), Containers::arrayView<Vector2>({
            {-1.0f              - 0.07f, -1.5f              - 0.08f},
            {+1.0f              + 0.07f, -1.5f              - 0.08f},
            {-1.0f              - 0.07f, +1.5f              + 0.08f},
            {+1.0f              + 0.07f, +1.5f              + 0.08f},
        }), TestSuite::Compare::Container);

        /* Text 9 cursor, now quad 3, Again ordered ZYXW instead of XYZW
           because the text is RTL. */
        CORRADE_COMPARE_AS(editingPositions.sliceSize(3*4, 1*4), Containers::arrayView<Vector2>({
            {3.0f + 1.5f        - 0.03f, 6.5f - 4.0f        - 0.02f},
            {3.0f + 1.5f        + 0.01f, 6.5f - 4.0f        - 0.02f},
            {3.0f + 1.5f        - 0.03f, 6.5f + 2.0f        + 0.04f},
            {3.0f + 1.5f        + 0.01f, 6.5f + 2.0f        + 0.04f},
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(editingCenterDistances.sliceSize(3*4, 1*4), Containers::arrayView<Vector2>({
            { 0.0f              - 0.02f, -3.0f              - 0.03f},
            { 0.0f              + 0.02f, -3.0f              - 0.03f},
            { 0.0f              - 0.02f, +3.0f              + 0.03f},
            { 0.0f              + 0.02f, +3.0f              + 0.03f},
        }), TestSuite::Compare::Container);
    }

    /* Removing a text marks the corresponding run as unused, the next update()
       then recompacts it */
    layer.remove(data7);
    /* state() can additionally contain LayerState::NeedsNodeOffsetSizeUpdate
       if we didn't pass it to the update() above, so test just that it
       contains at least these flags */
    CORRADE_COMPARE_AS(layer.state(),
        LayerState::NeedsNodeEnabledUpdate|LayerState::NeedsAttachmentUpdate|LayerState::NeedsDataClean,
        TestSuite::Compare::GreaterOrEqual);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::glyphOffset), Containers::arrayView({
        0u, 1u, 2u, 0xffffffffu, 4u, 5u
    }), TestSuite::Compare::Container);
    if(data.dataFlags >= TextDataFlag::Editable)
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().textRuns).slice(&Implementation::TextLayerTextRun::textOffset), Containers::arrayView({
            0xffffffffu, 4u
        }), TestSuite::Compare::Container);

    /* Again this explicitly adds NeedsDataUpdate to force recompaction */
    UnsignedInt dataIdsPostRemoval[]{9};
    layer.update(data.states|LayerState::NeedsDataUpdate, dataIdsPostRemoval, {}, {}, nodeOffsets, nodeSizes, nodeOpacities, nodesEnabled, {}, {}, {}, {});

    /* There should be just 7 glyph runs, assigned to the remaining 7 data */
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().data).slice(&Implementation::TextLayerData::glyphRun), Containers::arrayView({
        0u, 0xffffffffu /* no run */, 1u, 2u /* free data */, 2u,
        4u /* free data */, 0xffffffffu /* no run */, 3u /* free data */, 3u,
        4u, 0xffffffffu /* free data */
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::glyphOffset), Containers::arrayView({
        0u, 1u, 2u, 3u, 4u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::glyphCount), Containers::arrayView({
        1u, 1u, 1u, 1u, 2u
    }), TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().glyphRuns).slice(&Implementation::TextLayerGlyphRun::data), Containers::arrayView({
        0u, 2u, 4u, 8u, 9u
    }), TestSuite::Compare::Container);

    /* The glyph count queries should still match */
    CORRADE_COMPARE(layer.glyphCount(data9), 2u);

    /* And 1 text run, assigned to the remining text */
    if(data.dataFlags >= TextDataFlag::Editable) {
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().data).slice(&Implementation::TextLayerData::textRun), Containers::arrayView({
            0xffffffffu, 0xffffffffu, 0xffffffffu, 0u /* free data */,
            0xffffffffu, 0xffffffffu, 0xffffffffu, 0u /* free data */,
            0xffffffffu, 0u, 3u /* free data */
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().textRuns).slice(&Implementation::TextLayerTextRun::textOffset), Containers::arrayView({
            0u
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().textRuns).slice(&Implementation::TextLayerTextRun::textSize), Containers::arrayView({
            2u
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().textRuns).slice(&Implementation::TextLayerTextRun::data), Containers::arrayView({
            9u
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(layer.stateData().textData,
            "hi",
            TestSuite::Compare::String);

        /* The text queries should still match also */
        CORRADE_COMPARE(layer.text(data9), "hi");
    }

    /* Indices for remaining 2 visible glyphs */
    CORRADE_COMPARE_AS(layer.stateData().indices, Containers::arrayView<UnsignedInt>({
        /* Text 9, "hi", quads 4 to 5 */
        4*4 + 0, 4*4 + 1, 4*4 + 2, 4*4 + 2, 4*4 + 1, 4*4 + 3,
        5*4 + 0, 5*4 + 1, 5*4 + 2, 5*4 + 2, 5*4 + 1, 5*4 + 3,
        /* Text 7, "ahoy", is removed now */
        /* Text 3, "hello", is removed now */
        /* Glyph 5 is removed now */
    }), TestSuite::Compare::Container);
    if(data.expectEditingDataPresent)
        CORRADE_COMPARE_AS(layer.stateData().editingIndices, Containers::arrayView<UnsignedInt>({
            /* Cursor for text 9, "hi", quad 1 (selection would be quad 0) */
            1*4 + 0,  1*4 + 2,  1*4 + 1,  1*4 + 2,  1*4 + 3,  1*4 + 1,
        }), TestSuite::Compare::Container);
    else
        CORRADE_COMPARE_AS(layer.stateData().editingIndices, Containers::arrayView<UnsignedInt>({
        }), TestSuite::Compare::Container);

    /* For drawing data 9 it needs to draw the first 2 quads in the index
       buffer. For editing just the data 9 cursor quad. */
    if(data.expectEditingDataPresent)
        CORRADE_COMPARE_AS(layer.stateData().indexDrawOffsets, (Containers::arrayView<Containers::Pair<UnsignedInt, UnsignedInt>>({
            {0, 0}, {2*6, 6}
        })), TestSuite::Compare::Container);
    else
        CORRADE_COMPARE_AS(layer.stateData().indexDrawOffsets, (Containers::arrayView<Containers::Pair<UnsignedInt, UnsignedInt>>({
            {0, 0}, {2*6, 0}
        })), TestSuite::Compare::Container);

    /* Vertices for all remaining 6 glyphs */
    CORRADE_COMPARE(layer.stateData().vertices.size(), 6*4*vertexTypeSize);
    /* (Possibly editable) text 9, quads 4 to 5 */
    for(std::size_t i = 0; i != 2*4; ++i) {
        CORRADE_ITERATION(i);
        CORRADE_COMPARE(vertices[4*4 + i].color, 0x663399ff_rgbaf*0.9f);
        if(data.sharedLayerFlags >= TextLayerSharedFlag::DistanceField)
            CORRADE_COMPARE(invertedRunScales[4*4 + i], 1.0f/0.5f); /* threeGlyphFont */
        /* Created with style 3, which is mapped to uniform 1. There's only a
           cursor, which doesn't override the text uniform, so it's always the
           same. */
        CORRADE_COMPARE(vertices[4*4 + i].styleUniform, 1);
    }

    /* Text 9, now quad 4 to 5 */
    CORRADE_COMPARE_AS(positions.sliceSize(4*4, 2*4), Containers::arrayView<Vector2>({
        {3.0f + 0.0f,              6.5f - 0.5f},
        {3.0f + 0.0f,              6.5f - 0.5f},
        {3.0f + 0.0f,              6.5f - 0.5f},
        {3.0f + 0.0f,              6.5f - 0.5f},

        {3.0f + 1.5f + 2.f + 0.0f, 6.5f - 1.0f + 4.0f - 0.0f},
        {3.0f + 1.5f + 2.f + 8.0f, 6.5f - 1.0f + 4.0f - 0.0f},
        {3.0f + 1.5f + 2.f + 0.0f, 6.5f - 1.0f + 4.0f - 8.0f},
        {3.0f + 1.5f + 2.f + 8.0f, 6.5f - 1.0f + 4.0f - 8.0f},
    }), TestSuite::Compare::Container);

    /* Glyph 22, now only at quad 4 */
    CORRADE_COMPARE_AS(textureCoordinates.sliceSize(4*4, 4), Containers::arrayView<Vector3>({
        {},
        {},
        {},
        {},
    }), TestSuite::Compare::Container);

    /* Glyph 13, now only at quad 5 */
    CORRADE_COMPARE_AS(textureCoordinates.sliceSize(5*4, 4), Containers::arrayView<Vector3>({
        {0.5f, 0.5f, 0.0f},
        {1.0f, 0.5f, 0.0f},
        {0.5f, 1.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
    }), TestSuite::Compare::Container);

    if(data.expectEditingDataPresent) {
        for(std::size_t i = 0; i != 4; ++i) {
            CORRADE_ITERATION(i);

            /* Text 9 cursor, now quad 1 */
            CORRADE_COMPARE(layer.stateData().editingVertices[1*4 + i].opacity, 0.9f);
            CORRADE_COMPARE(layer.stateData().editingVertices[1*4 + i].styleUniform, 3);
        }

        Containers::StridedArrayView1D<const Vector2> editingPositions = stridedArrayView(layer.stateData().editingVertices).slice(&Implementation::TextLayerEditingVertex::position);
        Containers::StridedArrayView1D<const Vector2> editingCenterDistances = stridedArrayView(layer.stateData().editingVertices).slice(&Implementation::TextLayerEditingVertex::centerDistance);

        /* Text 9 cursor, now quad 1. Again ordered ZYXW instead of XYZW
           because the text is RTL. */
        CORRADE_COMPARE_AS(editingPositions.sliceSize(1*4, 1*4), Containers::arrayView<Vector2>({
            {3.0f + 1.5f        - 0.03f, 6.5f - 4.0f        - 0.02f},
            {3.0f + 1.5f        + 0.01f, 6.5f - 4.0f        - 0.02f},
            {3.0f + 1.5f        - 0.03f, 6.5f + 2.0f        + 0.04f},
            {3.0f + 1.5f        + 0.01f, 6.5f + 2.0f        + 0.04f},
        }), TestSuite::Compare::Container);
        CORRADE_COMPARE_AS(editingCenterDistances.sliceSize(1*4, 1*4), Containers::arrayView<Vector2>({
            { 0.0f              - 0.02f, -3.0f              - 0.03f},
            { 0.0f              + 0.02f, -3.0f              - 0.03f},
            { 0.0f              - 0.02f, +3.0f              + 0.03f},
            { 0.0f              + 0.02f, +3.0f              + 0.03f},
        }), TestSuite::Compare::Container);
    }
}

void TextLayerTest::updateAlignment() {
    auto&& data = UpdateAlignmentPaddingData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return _opened; }
        Properties doOpenFile(Containers::StringView, Float) override {
            _opened = true;
            /* Font size and line height shouldn't be used for any alignment,
               ascent / descent should */
            return {100.0f, 3.5f, -2.0f, 200.0f, 1};
        }
        void doClose() override { _opened = false; }

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override {
            struct Shaper: Text::AbstractShaper {
                using Text::AbstractShaper::AbstractShaper;

                UnsignedInt doShape(Containers::StringView text, UnsignedInt, UnsignedInt, Containers::ArrayView<const Text::FeatureRange>) override {
                    return text.size();
                }
                bool doSetDirection(Text::ShapeDirection direction) override {
                    _direction = direction;
                    return true;
                }
                Text::ShapeDirection doDirection() const override {
                    return _direction;
                }
                void doGlyphIdsInto(const Containers::StridedArrayView1D<UnsignedInt>& ids) const override {
                    for(std::size_t i = 0; i != ids.size(); ++i)
                        ids[i] = 0;
                }
                void doGlyphOffsetsAdvancesInto(const Containers::StridedArrayView1D<Vector2>& offsets, const Containers::StridedArrayView1D<Vector2>& advances) const override {
                    for(std::size_t i = 0; i != offsets.size(); ++i) {
                        offsets[i] = {};
                        advances[i] = {1.5f, 0.0f};
                    }
                }
                void doGlyphClustersInto(const Containers::StridedArrayView1D<UnsignedInt>& clusters) const override {
                    /* Just a trivial 1:1 mapping */
                    for(std::size_t i = 0; i != clusters.size(); ++i)
                        clusters[i] = i;
                }

                private:
                    Text::ShapeDirection _direction = Text::ShapeDirection::Unspecified;
            };
            return Containers::pointer<Shaper>(*this);
        }

        bool _opened = false;
    } font;
    font.openFile({}, 100.0f);

    /* A trivial glyph cache. While font's ascent/descent goes both above and
       below the line, this is just above. */
    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    /* Default padding is 1, resetting to 0 for simplicity */
    } cache{PixelFormat::R8Unorm, {32, 32}, {}};
    cache.addGlyph(cache.addFont(1, &font), 0, {}, {{}, {1, 2}});

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{1}
        .setEditingStyleCount(1)
    };

    /* Font scaled 2x, so all metrics coming from the font or the cache should
       be scaled 2x */
    FontHandle fontHandle = shared.addFont(font, 200.0f);
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}},
        {fontHandle},
        /* Alignment supplied in the style; alignment in TextProperties tested
           in updatePadding() below, combination of both with a subset of the
           alignment values in updateCleanDataOrder() above */
        {data.alignment},
        /* Features affect only create() / createGlyph() / setText() /
           setGlyph(), not update(); tested in createSetTextTextProperties() */
        {}, {}, {},
        {0}, {0},
        {});
    shared.setEditingStyle(TextLayerCommonEditingStyleUniform{},
        {TextLayerEditingStyleUniform{}},
        {},
        /* The padding should be added to the selection / cursor rectangle but
           without shifting the whole text in any way */
        {{0.1f, 0.2f, 0.3f, 0.4f}});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}

        const State& stateData() const {
            return static_cast<const State&>(*_state);
        }
    } layer{layerHandle(0, 1), shared};

    /* Required to be called before update() (because AbstractUserInterface
       guarantees the same on a higher level), not needed for anything here */
    layer.setSize({1, 1}, {1, 1});

    NodeHandle node3 = nodeHandle(3, 0);

    /* 3 chars, size x2, so the bounding box is 9x11 */
    DataHandle node3Data = layer.create(0,
        "hey",
        /* Direction passed from TextProperties, direction returned from the
           shaper tested in updatePadding() below */
        TextProperties{}.setShapeDirection(data.shapeDirection),
        TextDataFlag::Editable,
        node3);
    layer.setCursor(node3Data, 1, 3);

    Vector2 nodeOffsets[4];
    Vector2 nodeSizes[4];
    Float nodeOpacities[4]{};
    UnsignedByte nodesEnabledData[1]{};
    Containers::BitArrayView nodesEnabled{nodesEnabledData, 0, 4};
    nodeOffsets[3] = {50.5f, 20.5f};
    nodeSizes[3] = {200.8f, 100.4f};
    UnsignedInt dataIds[]{0};
    layer.update(LayerState::NeedsDataUpdate, dataIds, {}, {}, nodeOffsets, nodeSizes, nodeOpacities, nodesEnabled, {}, {}, {}, {});

    /* 2--3
       |  |
       0--1 */
    CORRADE_COMPARE_AS(stridedArrayView(Containers::arrayCast<Implementation::TextLayerVertex>(layer.stateData().vertices)).slice(&Implementation::TextLayerVertex::position), Containers::arrayView<Vector2>({
        Vector2{0.0f, 0.0f} + data.offset,
        Vector2{2.0f, 0.0f} + data.offset,
        Vector2{0.0f, -4.0f} + data.offset,
        Vector2{2.0f, -4.0f} + data.offset,

        Vector2{3.0f, 0.0f} + data.offset,
        Vector2{5.0f, 0.0f} + data.offset,
        Vector2{3.0f, -4.0f} + data.offset,
        Vector2{5.0f, -4.0f} + data.offset,

        Vector2{6.0f, 0.0f} + data.offset,
        Vector2{8.0f, 0.0f} + data.offset,
        Vector2{6.0f, -4.0f} + data.offset,
        Vector2{8.0f, -4.0f} + data.offset,
    }), TestSuite::Compare::Container);

    /* 0--1
       |  |
       2--3 */
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().editingVertices).slice(&Implementation::TextLayerEditingVertex::position), Containers::arrayView<Vector2>({
        /* Selection containing glyph 1 and 2 (advance of 1.5*2), plus
           padding; ascent 3.5*2, descent 2.0*2 */
        Vector2{3.0f        - data.editingPaddingL, -7.0f - 0.2f} + data.offset,
        Vector2{6.0f + 3.0f + data.editingPaddingR, -7.0f - 0.2f} + data.offset,
        Vector2{3.0f        - data.editingPaddingL,  4.0f + 0.4f} + data.offset,
        Vector2{6.0f + 3.0f + data.editingPaddingR,  4.0f + 0.4f} + data.offset,

        /* Cursor before glyph 1, plus padding */
        Vector2{3.0f        - data.editingPaddingL, -7.0f - 0.2f} + data.offset,
        Vector2{3.0f        + data.editingPaddingR, -7.0f - 0.2f} + data.offset,
        Vector2{3.0f        - data.editingPaddingL,  4.0f + 0.4f} + data.offset,
        Vector2{3.0f        + data.editingPaddingR,  4.0f + 0.4f} + data.offset,
    }), TestSuite::Compare::Container);
}

void TextLayerTest::updateAlignmentGlyph() {
    auto&& data = UpdateAlignmentPaddingData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* A trivial glyph cache. Goes both above and below the line to verify
       vertical alignment. */
    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    /* Default padding is 1, resetting to 0 for simplicity */
    } cache{PixelFormat::R8Unorm, {32, 32}, {}};

    UnsignedInt glyphCacheFontId = cache.addFont(18);
    cache.addGlyph(glyphCacheFontId, 17, {-2, -3}, {{}, {3, 4}});

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{1}};

    /* Font scaled 2x, so all metrics coming from the the cache should be
       scaled 2x */
    FontHandle fontHandle = shared.addInstancelessFont(glyphCacheFontId, 2.0f);
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}},
        {fontHandle},
        /* Alignment supplied in the style; alignment in TextProperties tested
           in updatePaddingGlyph() below, combination of both with a subset of
           the alignment values in updateCleanDataOrder() above */
        {data.alignment},
        /* Features affect only create() / createGlyph() / setText() /
           setGlyph(), not update(); tested in createSetTextTextProperties() */
        {}, {}, {},
        /* Editing styles aren't used for single glyphs */
        {}, {},
        {});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}

        const State& stateData() const {
            return static_cast<const State&>(*_state);
        }
    } layer{layerHandle(0, 1), shared};

    /* Required to be called before update() (because AbstractUserInterface
       guarantees the same on a higher level), not needed for anything here */
    layer.setSize({1, 1}, {1, 1});

    NodeHandle node3 = nodeHandle(3, 0);

    /* Size x2, so the bounding box is 6x8 */
    layer.createGlyph(
        0,
        17,
        /* There's no way to detect the direction as no shaper is used, so
           compared to updateAlignment() / updatePadding() this is always
           passed in via TextProperties */
        TextProperties{}.setShapeDirection(data.shapeDirection),
        node3);

    Vector2 nodeOffsets[4];
    Vector2 nodeSizes[4];
    Float nodeOpacities[4]{};
    UnsignedByte nodesEnabledData[1]{};
    Containers::BitArrayView nodesEnabled{nodesEnabledData, 0, 4};
    nodeOffsets[3] = {50.5f, 20.5f};
    nodeSizes[3] = {200.8f, 100.4f};
    UnsignedInt dataIds[]{0};
    layer.update(LayerState::NeedsDataUpdate, dataIds, {}, {}, nodeOffsets, nodeSizes, nodeOpacities, nodesEnabled, {}, {}, {}, {});

    /* 2--3
       |  |
       0--1 */
    CORRADE_COMPARE_AS(stridedArrayView(Containers::arrayCast<Implementation::TextLayerVertex>(layer.stateData().vertices)).slice(&Implementation::TextLayerVertex::position), Containers::arrayView<Vector2>({
        Vector2{0.0f, 0.0f} + data.offsetGlyph,
        Vector2{6.0f, 0.0f} + data.offsetGlyph,
        Vector2{0.0f, -8.0f} + data.offsetGlyph,
        Vector2{6.0f, -8.0f} + data.offsetGlyph
    }), TestSuite::Compare::Container);
}

void TextLayerTest::updatePadding() {
    auto&& data = UpdateAlignmentPaddingData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Same as updateAlignment(), except that the node offset & size is
       different and only matches the original if padding is applied
       correctly from both the data and the style. Additionally, in comparison
       to updateAlignment(), shape direction is returned from the shaper
       directly instead of being passed from TextProperties, and the
       style-supplied alignment is a bogus value and alignment from the
       TextProperties is used instead. */

    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return _opened; }
        Properties doOpenFile(Containers::StringView, Float) override {
            _opened = true;
            /* Font size and line height shouldn't be used for any alignment,
               ascent / descent should */
            return {100.0f, 3.5f, -2.0f, 200.0f, 1};
        }
        void doClose() override { _opened = false; }

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override {
            struct Shaper: Text::AbstractShaper {
                explicit Shaper(Text::AbstractFont& font, Text::ShapeDirection direction): Text::AbstractShaper{font}, _direction{direction} {}

                UnsignedInt doShape(Containers::StringView text, UnsignedInt, UnsignedInt, Containers::ArrayView<const Text::FeatureRange>) override {
                    return text.size();
                }
                Text::ShapeDirection doDirection() const override {
                    return _direction;
                }
                void doGlyphIdsInto(const Containers::StridedArrayView1D<UnsignedInt>& ids) const override {
                    for(std::size_t i = 0; i != ids.size(); ++i)
                        ids[i] = 0;
                }
                void doGlyphOffsetsAdvancesInto(const Containers::StridedArrayView1D<Vector2>& offsets, const Containers::StridedArrayView1D<Vector2>& advances) const override {
                    for(std::size_t i = 0; i != offsets.size(); ++i) {
                        offsets[i] = {};
                        advances[i] = {1.5f, 0.0f};
                    }
                }
                void doGlyphClustersInto(const Containers::StridedArrayView1D<UnsignedInt>& clusters) const override {
                    /* Just a trivial 1:1 mapping */
                    for(std::size_t i = 0; i != clusters.size(); ++i)
                        clusters[i] = i;
                }

                Text::ShapeDirection _direction;
            };
            return Containers::pointer<Shaper>(*this, direction);
        }

        Text::ShapeDirection direction;
        bool _opened = false;
    } font;
    /* Direction returned from the shaper, direction passed through from
       TextProperties tested in updateAlignment() above */
    font.direction = data.shapeDirection;
    font.openFile({}, 100.0f);

    /* A trivial glyph cache. While font's ascent/descent goes both above and
       below the line, this is just above. */
    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    /* Default padding is 1, resetting to 0 for simplicity */
    } cache{PixelFormat::R8Unorm, {32, 32}, {}};
    cache.addGlyph(cache.addFont(1, &font), 0, {}, {{}, {1, 2}});

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{1}
        .setEditingStyleCount(1)
    };

    /* Font scaled 2x, so all metrics coming from the font or the cache should
       be scaled 2x */
    FontHandle fontHandle = shared.addFont(font, 200.0f);
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}},
        {fontHandle},
        /* Alignment set to an arbitrary value, the one from TextProperties
           should get used instead */
        {Text::Alignment::BottomRight},
        /* Features affect only create() / createGlyph() / setText() /
           setGlyph(), not update(); tested in createSetTextTextProperties() */
        {}, {}, {},
        {0}, {0},
        {{10.0f, 5.0f, 20.0f, 10.0f}});
    shared.setEditingStyle(TextLayerCommonEditingStyleUniform{},
        {TextLayerEditingStyleUniform{}},
        {},
        /* The padding should be added to the selection / cursor rectangle but
           without shifting the whole text in any way */
        {{0.1f, 0.2f, 0.3f, 0.4f}});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}

        const State& stateData() const {
            return static_cast<const State&>(*_state);
        }
    } layer{layerHandle(0, 1), shared};

    /* Required to be called before update() (because AbstractUserInterface
       guarantees the same on a higher level), not needed for anything here */
    layer.setSize({1, 1}, {1, 1});

    NodeHandle node3 = nodeHandle(3, 0);

    /* 3 chars, size x2, so the bounding box is 9x11 */
    DataHandle node3Data = layer.create(0, "hey", data.alignment, TextDataFlag::Editable, node3);
    layer.setPadding(node3Data, {20.0f, 5.0f, 50.0f, 30.0f});
    layer.setCursor(node3Data, 1, 3);

    Vector2 nodeOffsets[4];
    Vector2 nodeSizes[4];
    Float nodeOpacities[4]{};
    UnsignedByte nodesEnabledData[1]{};
    Containers::BitArrayView nodesEnabled{nodesEnabledData, 0, 4};
    nodeOffsets[3] = {20.5f, 10.5f};
    nodeSizes[3] = {300.8f, 150.4f};
    UnsignedInt dataIds[]{0};
    layer.update(LayerState::NeedsDataUpdate, dataIds, {}, {}, nodeOffsets, nodeSizes, nodeOpacities, nodesEnabled, {}, {}, {}, {});

    /* 2--3
       |  |
       0--1 */
    CORRADE_COMPARE_AS(stridedArrayView(Containers::arrayCast<Implementation::TextLayerVertex>(layer.stateData().vertices)).slice(&Implementation::TextLayerVertex::position), Containers::arrayView<Vector2>({
        Vector2{0.0f, 0.0f} + data.offset,
        Vector2{2.0f, 0.0f} + data.offset,
        Vector2{0.0f, -4.0f} + data.offset,
        Vector2{2.0f, -4.0f} + data.offset,

        Vector2{3.0f, 0.0f} + data.offset,
        Vector2{5.0f, 0.0f} + data.offset,
        Vector2{3.0f, -4.0f} + data.offset,
        Vector2{5.0f, -4.0f} + data.offset,

        Vector2{6.0f, 0.0f} + data.offset,
        Vector2{8.0f, 0.0f} + data.offset,
        Vector2{6.0f, -4.0f} + data.offset,
        Vector2{8.0f, -4.0f} + data.offset,
    }), TestSuite::Compare::Container);

    /* 0--1
       |  |
       2--3 */
    CORRADE_COMPARE_AS(stridedArrayView(layer.stateData().editingVertices).slice(&Implementation::TextLayerEditingVertex::position), Containers::arrayView<Vector2>({
        /* Selection containing glyph 1 and 2 (advance of 1.5*2), plus
           padding; ascent 3.5*2, descent 2.0*2 */
        Vector2{3.0f        - data.editingPaddingL, -7.0f - 0.2f} + data.offset,
        Vector2{6.0f + 3.0f + data.editingPaddingR, -7.0f - 0.2f} + data.offset,
        Vector2{3.0f        - data.editingPaddingL,  4.0f + 0.4f} + data.offset,
        Vector2{6.0f + 3.0f + data.editingPaddingR,  4.0f + 0.4f} + data.offset,

        /* Cursor before glyph 1, plus padding */
        Vector2{3.0f        - data.editingPaddingL, -7.0f - 0.2f} + data.offset,
        Vector2{3.0f        + data.editingPaddingR, -7.0f - 0.2f} + data.offset,
        Vector2{3.0f        - data.editingPaddingL,  4.0f + 0.4f} + data.offset,
        Vector2{3.0f        + data.editingPaddingR,  4.0f + 0.4f} + data.offset,
    }), TestSuite::Compare::Container);
}

void TextLayerTest::updatePaddingGlyph() {
    auto&& data = UpdateAlignmentPaddingData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Same as updateAlignmentGlyph(), except that the node offset & size is
       different and only matches the original if padding is applied
       correctly from both the data and the style. Additionally, in comparison
       to updateAlignmentGlyph(), the style-supplied alignment is a bogus value
       and padding from the TextProperties is used instead. */

    /* A trivial glyph cache. Goes both above and below the line to verify
       vertical alignment. */
    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    /* Default padding is 1, resetting to 0 for simplicity */
    } cache{PixelFormat::R8Unorm, {32, 32}, {}};

    UnsignedInt glyphCacheFontId = cache.addFont(18);
    cache.addGlyph(glyphCacheFontId, 17, {-2, -3}, {{}, {3, 4}});

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{1}};

    /* Font scaled 2x, so all metrics coming from the the cache should be
       scaled 2x */
    FontHandle fontHandle = shared.addInstancelessFont(glyphCacheFontId, 2.0f);
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}},
        {fontHandle},
        /* Alignment set to an arbitrary value, the one from TextProperties
           should get used instead */
        {Text::Alignment::BottomRight},
        /* Features affect only create() / createGlyph() / setText() /
           setGlyph(), not update(); tested in createSetTextTextProperties() */
        {}, {}, {},
        /* Editing styles aren't used for single glyphs */
        {}, {},
        {{10.0f, 5.0f, 20.0f, 10.0f}});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}

        const State& stateData() const {
            return static_cast<const State&>(*_state);
        }
    } layer{layerHandle(0, 1), shared};

    /* Required to be called before update() (because AbstractUserInterface
       guarantees the same on a higher level), not needed for anything here */
    layer.setSize({1, 1}, {1, 1});

    NodeHandle node3 = nodeHandle(3, 0);

    /* Size x2, so the bounding box is 6x8 */
    DataHandle node3Data = layer.createGlyph(0, 17,
        TextProperties{}
            .setAlignment(data.alignment)
            /* There's no way to detect the direction as no shaper is used, so
               compared to updateAlignment() / updatePadding() this is always
               passed in via TextProperties */
            .setShapeDirection(data.shapeDirection),
        node3);
    layer.setPadding(node3Data, {20.0f, 5.0f, 50.0f, 30.0f});

    Vector2 nodeOffsets[4];
    Vector2 nodeSizes[4];
    Float nodeOpacities[4]{};
    UnsignedByte nodesEnabledData[1]{};
    Containers::BitArrayView nodesEnabled{nodesEnabledData, 0, 4};
    nodeOffsets[3] = {20.5f, 10.5f};
    nodeSizes[3] = {300.8f, 150.4f};
    UnsignedInt dataIds[]{0};
    layer.update(LayerState::NeedsDataUpdate, dataIds, {}, {}, nodeOffsets, nodeSizes, nodeOpacities, nodesEnabled, {}, {}, {}, {});

    /* 2--3
       |  |
       0--1 */
    CORRADE_COMPARE_AS(stridedArrayView(Containers::arrayCast<Implementation::TextLayerVertex>(layer.stateData().vertices)).slice(&Implementation::TextLayerVertex::position), Containers::arrayView<Vector2>({
        Vector2{0.0f, 0.0f} + data.offsetGlyph,
        Vector2{6.0f, 0.0f} + data.offsetGlyph,
        Vector2{0.0f, -8.0f} + data.offsetGlyph,
        Vector2{6.0f, -8.0f} + data.offsetGlyph
    }), TestSuite::Compare::Container);
}

void TextLayerTest::updateTransformation() {
    auto&& data = UpdateTransformationData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Trimmed-down variant of updatePadding() that checks transformations get
       applied in correct order respective to node offset and padding coming
       from style. Testing with multiple glyphs to verify they're all
       transformed while preserving their relative position. Single-glyph
       rendering has no specific behavior so it isn't tested. */

    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return _opened; }
        Properties doOpenFile(Containers::StringView, Float size) override {
            _opened = true;
            /* Line height shouldn't be used for anything */
            return {size, 7.0f*size/100.0f, -4.0f*size/100.0f, 10000.0f, 1};
        }
        void doClose() override { _opened = false; }

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override {
            struct Shaper: Text::AbstractShaper {
                explicit Shaper(Text::AbstractFont& font): Text::AbstractShaper{font} {}

                UnsignedInt doShape(Containers::StringView text, UnsignedInt, UnsignedInt, Containers::ArrayView<const Text::FeatureRange>) override {
                    return text.size();
                }
                void doGlyphIdsInto(const Containers::StridedArrayView1D<UnsignedInt>& ids) const override {
                    for(std::size_t i = 0; i != ids.size(); ++i)
                        ids[i] = 0;
                }
                void doGlyphOffsetsAdvancesInto(const Containers::StridedArrayView1D<Vector2>& offsets, const Containers::StridedArrayView1D<Vector2>& advances) const override {
                    for(std::size_t i = 0; i != offsets.size(); ++i) {
                        offsets[i] = {};
                        advances[i] = {3.0f*font().size()/100.0f, 0.0f};
                    }
                }
                void doGlyphClustersInto(const Containers::StridedArrayView1D<UnsignedInt>&) const override {
                    CORRADE_FAIL("This shouldn't be called.");
                }
            };
            return Containers::pointer<Shaper>(*this);
        }

        bool _opened = false;
    } font;
    /* Open the font at twice the size to then add it back at the original size
       below, to verify this 0.5x scale gets correctly propagated to the
       distance field attribute */
    font.openFile({}, 200.0f);

    /* A trivial glyph cache. While font's ascent/descent goes both above and
       below the line, this is just above. */
    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    /* Default padding is 1, resetting to 0 for simplicity */
    } cache{PixelFormat::R8Unorm, {32, 32}, {}};
    /* Y glyph offset to verify the transformation is relative to cursor
       placement also on Y, not to bounding box edge. The offset and size gets
       divided by 2. */
    cache.addGlyph(cache.addFont(1, &font), 0, {0, -2}, {{}, {4, 8}});

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{1}
        .setFlags(data.sharedLayerFlags)
    };

    FontHandle fontHandle = shared.addFont(font, 100.0f);
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}},
        {fontHandle},
        {Text::Alignment::BottomRight},
        {}, {}, {}, {}, {},
        {{50.0f, 100.0f, 5.0f, 10.0f}});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared, TextLayerFlags flags): TextLayer{handle, shared, flags} {}

        const State& stateData() const {
            return static_cast<const State&>(*_state);
        }
    } layer{layerHandle(0, 1), shared, data.layerFlags};

    /* Required to be called before update() (because AbstractUserInterface
       guarantees the same on a higher level), not needed for anything here */
    layer.setSize({1, 1}, {1, 1});

    NodeHandle node3 = nodeHandle(3, 0);

    /* 3 chars, so the bounding box is 9x11. It's always that regardless of
       transformation. */
    DataHandle node3Data = layer.create(0, "hey", {}, node3);
    if(data.layerFlags >= TextLayerFlag::Transformable)
        layer.setTransformation(node3Data, data.translation, data.rotation, data.scaling);
    CORRADE_COMPARE(layer.size(node3Data), (Vector2{9.0f, 11.0f}));

    Vector2 nodeOffsets[4];
    Vector2 nodeSizes[4];
    Float nodeOpacities[4]{};
    UnsignedByte nodesEnabledData[1]{};
    Containers::BitArrayView nodesEnabled{nodesEnabledData, 0, 4};
    nodeOffsets[3] = {20.0f, 10.0f};
    nodeSizes[3] = {300.0f, 150.0f};
    UnsignedInt dataIds[]{0};
    layer.update(LayerState::NeedsDataUpdate, dataIds, {}, {}, nodeOffsets, nodeSizes, nodeOpacities, nodesEnabled, {}, {}, {}, {});

    /* As the alignment is bottom right, the text rendering cursor is at bottom
       right of the 9x11 box, and it's placed to the bottom right corner of the
       node. Then, while each glyph bounding box is 3x11, the actual glyph is
       2x4, so it's 3 units from the bottom and after each there's 1 unit space
       on the right.
                       node    node    padding right
                       offset  size    padding bottom */
    Vector2 baseOffset{20.0f + 300.0f - 5.0f,
                       10.0f + 150.0f - 10.0f};

    /* 2--3
       |  |
       0--1 */
    Containers::StridedArrayView1D<const Vector2> positions =
        data.sharedLayerFlags >= TextLayerSharedFlag::DistanceField ?
            stridedArrayView(Containers::arrayCast<Implementation::TextLayerDistanceFieldVertex>(layer.stateData().vertices)).slice(&Implementation::TextLayerDistanceFieldVertex::vertex).slice(&Implementation::TextLayerVertex::position) :
            stridedArrayView(Containers::arrayCast<Implementation::TextLayerVertex>(layer.stateData().vertices)).slice(&Implementation::TextLayerVertex::position);
    CORRADE_COMPARE_AS(positions, Containers::arrayView<Vector2>({
        baseOffset + data.expected.transformPoint({-9.0f, -3.0f}),
        baseOffset + data.expected.transformPoint({-7.0f, -3.0f}),
        baseOffset + data.expected.transformPoint({-9.0f, -7.0f}),
        baseOffset + data.expected.transformPoint({-7.0f, -7.0f}),

        baseOffset + data.expected.transformPoint({-6.0f, -3.0f}),
        baseOffset + data.expected.transformPoint({-4.0f, -3.0f}),
        baseOffset + data.expected.transformPoint({-6.0f, -7.0f}),
        baseOffset + data.expected.transformPoint({-4.0f, -7.0f}),

        baseOffset + data.expected.transformPoint({-3.0f, -3.0f}),
        baseOffset + data.expected.transformPoint({-1.0f, -3.0f}),
        baseOffset + data.expected.transformPoint({-3.0f, -7.0f}),
        baseOffset + data.expected.transformPoint({-1.0f, -7.0f}),
    }), TestSuite::Compare::Container);

    /* If there's a positive rotation, it should be clockwise, i.e. the next
       quads being lower (i.e., Y larger with Y down) than the first, and vice
       versa */
    if(data.rotation > 0.0_radf) {
        CORRADE_COMPARE_AS(positions[4].y(),
                           positions[0].y(),
                           TestSuite::Compare::Greater);
        CORRADE_COMPARE_AS(positions[8].y(),
                           positions[4].y(),
                           TestSuite::Compare::Greater);
    } else if(data.rotation < 0.0_radf) {
        CORRADE_COMPARE_AS(positions[4].y(),
                           positions[0].y(),
                           TestSuite::Compare::Less);
        CORRADE_COMPARE_AS(positions[8].y(),
                           positions[4].y(),
                           TestSuite::Compare::Less);
    }

    /* A transform scale should get reflected in the attribute for distance
       field radius scaling, in addition to the font scale */
    if(data.sharedLayerFlags >= TextLayerSharedFlag::DistanceField) {
        CORRADE_COMPARE_AS(stridedArrayView(Containers::arrayCast<Implementation::TextLayerDistanceFieldVertex>(layer.stateData().vertices)).slice(&Implementation::TextLayerDistanceFieldVertex::invertedRunScale), Containers::arrayView({
            1.0f/(0.5f*data.scaling),
            1.0f/(0.5f*data.scaling),
            1.0f/(0.5f*data.scaling),
            1.0f/(0.5f*data.scaling),

            1.0f/(0.5f*data.scaling),
            1.0f/(0.5f*data.scaling),
            1.0f/(0.5f*data.scaling),
            1.0f/(0.5f*data.scaling),

            1.0f/(0.5f*data.scaling),
            1.0f/(0.5f*data.scaling),
            1.0f/(0.5f*data.scaling),
            1.0f/(0.5f*data.scaling),
        }), TestSuite::Compare::Container);
    }
}

void TextLayerTest::updateNoStyleSet() {
    auto&& data = CreateUpdateNoStyleSetData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32}};

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    /* It should complain regardless of dynamic style count and even if the
       style count is 0 as the common uniform is still used in that case */
    } shared{cache, TextLayer::Shared::Configuration{data.styleCount}
        .setDynamicStyleCount(data.dynamicStyleCount)
    };

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    /* Required to be called before update() (because AbstractUserInterface
       guarantees the same on a higher level), not needed for anything here */
    layer.setSize({1, 1}, {1, 1});

    Containers::String out;
    Error redirectError{&out};
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(out, "Ui::TextLayer::update(): no style data was set\n");
}

void TextLayerTest::updateNoEditingStyleSet() {
    auto&& data = CreateUpdateNoStyleSetData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    CORRADE_SKIP_IF_NO_ASSERT();

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32}};

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    /* It should complain regardless of dynamic style count and even if the
       style count is 0 as the common uniform is still used in that case */
    } shared{cache, TextLayer::Shared::Configuration{data.styleCount}
        .setEditingStyleCount(data.styleCount ? 1 : 0)
        /* If the editing style count is 0, we have to explicitly opt in for
           dynamic editing styles */
        .setDynamicStyleCount(data.dynamicStyleCount, true)
    };

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    } layer{layerHandle(0, 1), shared};

    /* Required to be called before update() (because AbstractUserInterface
       guarantees the same on a higher level), not needed for anything here */
    layer.setSize({1, 1}, {1, 1});

    /* Set the base style but not the editing one */
    if(data.styleCount == 1)
        shared.setStyle(TextLayerCommonStyleUniform{},
            {TextLayerStyleUniform{}},
            {FontHandle::Null},
            {Text::Alignment{}},
            {}, {}, {}, {}, {}, {});
    else if(data.styleCount == 0)
        shared.setStyle(TextLayerCommonStyleUniform{},
            {},
            {},
            {},
            {}, {}, {}, {}, {}, {});
    else CORRADE_INTERNAL_ASSERT_UNREACHABLE();

    Containers::String out;
    Error redirectError{&out};
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(out, "Ui::TextLayer::update(): no editing style data was set\n");
}

void TextLayerTest::sharedNeedsUpdateStatePropagatedToLayers() {
    auto&& data = SharedNeedsUpdateStatePropagatedToLayersData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32}};

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{1}
        .setEditingStyleCount(data.editingStyleCount)
        .setDynamicStyleCount(data.dynamicStyleCount)
    };

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    };

    /* Initially no state is set */
    Layer layer1{layerHandle(0, 1), shared};
    Layer layer2{layerHandle(0, 1), shared};
    Layer layer3{layerHandle(0, 1), shared};
    CORRADE_COMPARE(layer1.state(), LayerStates{});
    CORRADE_COMPARE(layer2.state(), LayerStates{});
    CORRADE_COMPARE(layer3.state(), LayerStates{});

    /* Required to be called before update() (because AbstractUserInterface
       guarantees the same on a higher level), not needed for anything here */
    layer1.setSize({1, 1}, {1, 1});
    layer2.setSize({1, 1}, {1, 1});
    layer3.setSize({1, 1}, {1, 1});

    /* Explicitly set a non-trivial state on some of the layers */
    layer1.setNeedsUpdate(LayerState::NeedsCommonDataUpdate);
    layer3.setNeedsUpdate(LayerState::NeedsSharedDataUpdate);

    /* Calling setStyle() / setEditingStyle() sets LayerState::Needs*DataUpdate
       on all layers. If editing styles are enabled, need to call both in order
       to be able to update() later. */
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}},
        {FontHandle::Null},
        {Text::Alignment{}},
        {}, {}, {}, {}, {}, {});
    if(data.editingStyleCount)
        shared.setEditingStyle(TextLayerCommonEditingStyleUniform{},
            {TextLayerEditingStyleUniform{}},
            {},
            {{}});
    CORRADE_COMPARE(layer1.state(), LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate|data.extraState);
    CORRADE_COMPARE(layer2.state(), LayerState::NeedsDataUpdate|data.extraState);
    CORRADE_COMPARE(layer3.state(), LayerState::NeedsDataUpdate|LayerState::NeedsSharedDataUpdate|data.extraState);

    /* Updating one doesn't cause the flag to be reset on others */
    layer2.update(LayerState::NeedsDataUpdate|data.extraState, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer1.state(), LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate|data.extraState);
    CORRADE_COMPARE(layer2.state(), LayerStates{});
    CORRADE_COMPARE(layer3.state(), LayerState::NeedsDataUpdate|LayerState::NeedsSharedDataUpdate|data.extraState);

    /* Updating another still doesn't */
    layer1.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer1.state(), LayerState::NeedsCommonDataUpdate);
    CORRADE_COMPARE(layer2.state(), LayerStates{});
    CORRADE_COMPARE(layer3.state(), LayerState::NeedsDataUpdate|LayerState::NeedsSharedDataUpdate|data.extraState);

    /* Calling setStyle() / setEditingStyle() again sets
       LayerState::Needs*DataUpdate again, even if the data may be the same, as
       checking differences would be unnecessarily expensive compared to just
       doing the update always */
    if(data.setStyle)
        shared.setStyle(TextLayerCommonStyleUniform{},
            {TextLayerStyleUniform{}},
            {FontHandle::Null},
            {Text::Alignment{}},
            {}, {}, {}, {}, {}, {});
    if(data.setEditingStyle)
        shared.setEditingStyle(TextLayerCommonEditingStyleUniform{},
            {TextLayerEditingStyleUniform{}},
            {},
            {{}});
    CORRADE_COMPARE(layer1.state(), LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate|data.extraState);
    CORRADE_COMPARE(layer2.state(), LayerState::NeedsDataUpdate|data.extraState);
    CORRADE_COMPARE(layer3.state(), LayerState::NeedsDataUpdate|LayerState::NeedsSharedDataUpdate|data.extraState);

    /* Creating a new layer with the shared state that had setStyle() called a
       few times doesn't mark it as needing an update because there's no data
       that would need it yet and the layer should do all other
       shared-state-dependent setup during construction already. For dynamic
       styles it'll perform the upload on the first update() regardless on the
       LayerState. */
    Layer layer4{layerHandle(0, 1), shared};
    CORRADE_COMPARE(layer4.state(), LayerStates{});

    /* But calling setStyle() / setEditingStyle() next time will */
    if(data.setStyle)
        shared.setStyle(TextLayerCommonStyleUniform{},
            {TextLayerStyleUniform{}},
            {FontHandle::Null},
            {Text::Alignment{}},
            {}, {}, {}, {}, {}, {});
    if(data.setEditingStyle)
        shared.setEditingStyle(TextLayerCommonEditingStyleUniform{},
            {TextLayerEditingStyleUniform{}},
            {},
            {{}});
    CORRADE_COMPARE(layer1.state(), LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate|data.extraState);
    CORRADE_COMPARE(layer2.state(), LayerState::NeedsDataUpdate|data.extraState);
    CORRADE_COMPARE(layer3.state(), LayerState::NeedsDataUpdate|LayerState::NeedsSharedDataUpdate|data.extraState);
    CORRADE_COMPARE(layer4.state(), LayerState::NeedsDataUpdate|data.extraState);

    /* Updating again resets just one */
    layer3.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer1.state(), LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate|data.extraState);
    CORRADE_COMPARE(layer2.state(), LayerState::NeedsDataUpdate|data.extraState);
    CORRADE_COMPARE(layer3.state(), LayerState::NeedsSharedDataUpdate);
    CORRADE_COMPARE(layer4.state(), LayerState::NeedsDataUpdate|data.extraState);

    /* Calling the AbstractVisualLayer setStyleTransition() should still cause
       LayerState to be updated as well, i.e. the class should correctly
       propagate to the parent doState() as well */
    shared.setStyleTransition(
        nullptr,
        nullptr,
        nullptr,
        [](UnsignedInt a) { return a + 1; });
    CORRADE_COMPARE(layer1.state(), LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate|data.extraState);
    CORRADE_COMPARE(layer2.state(), LayerState::NeedsDataUpdate|data.extraState);
    /* This one has NeedsDataUpdate set again, not the extraState though as
       that comes only from setStyle() depending on dynamic styles being
       present */
    CORRADE_COMPARE(layer3.state(), LayerState::NeedsDataUpdate|LayerState::NeedsSharedDataUpdate);
    CORRADE_COMPARE(layer4.state(), LayerState::NeedsDataUpdate|data.extraState);
}

void TextLayerTest::keyTextEvent() {
    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return Containers::pointer<ThreeGlyphShaper>(*this); }
    } font;

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};
    cache.addFont(98, &font);

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{1}};
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}},
        {shared.addFont(font, 1.0f)},
        {Text::Alignment::MiddleCenter},
        {}, {}, {}, {}, {}, {});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    };

    AbstractUserInterface ui{{100, 100}};

    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), shared));
    NodeHandle node = ui.createNode({}, {100, 100}, NodeFlag::Focusable);

    /* Have also some text before and after to catch weird overlaps and OOB
       issues */
    layer.create(0, "aaaa", {}, TextDataFlag::Editable);
    DataHandle text = layer.create(0, "hello", {}, TextDataFlag::Editable, node);
    layer.setCursor(text, 3);
    layer.create(0, "bb", {}, TextDataFlag::Editable);
    CORRADE_COMPARE(layer.text(text), "hello");
    CORRADE_COMPARE(layer.cursor(text), Containers::pair(3u, 3u));

    /* Create also a non-editable text attached to the same node, it shouldn't
       get modified in any way */
    layer.create(0, "hey", {}, node);

    /* Reset state flags */
    ui.update();
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Hover the node */
    {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, {}, true, 0, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent({50, 50}, event));
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), node);

    /* Nothing should happen if not focused even if the node is under cursor */
    } {
        KeyEvent right{{}, Key::Right, {}};
        KeyEvent left{{}, Key::Left, {}};
        KeyEvent backspace{{}, Key::Backspace, {}};
        KeyEvent delete_{{}, Key::Delete, {}};
        KeyEvent home{{}, Key::Home, {}};
        KeyEvent end{{}, Key::End, {}};
        TextInputEvent input{{}, "hello"};
        CORRADE_VERIFY(!ui.keyPressEvent(right));
        CORRADE_VERIFY(!ui.keyPressEvent(left));
        CORRADE_VERIFY(!ui.keyPressEvent(backspace));
        CORRADE_VERIFY(!ui.keyPressEvent(delete_));
        CORRADE_VERIFY(!ui.keyPressEvent(home));
        CORRADE_VERIFY(!ui.keyPressEvent(end));
        CORRADE_VERIFY(!ui.textInputEvent(input));
        CORRADE_COMPARE(layer.text(text), "hello");
        CORRADE_COMPARE(layer.cursor(text), Containers::pair(3u, 3u));
        CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Move pointer away, focus the node instead */
    } {
        PointerMoveEvent move{{}, PointerEventSource::Mouse, {}, {}, true, 0, {}};
        FocusEvent focus{{}};
        CORRADE_VERIFY(!ui.pointerMoveEvent({1000, 1000}, move));
        CORRADE_VERIFY(ui.focusEvent(node, focus));
        CORRADE_COMPARE(ui.currentFocusedNode(), node);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

        /* Reset state flags */
        ui.update();
        CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Left / right arrow */
    } {
        KeyEvent event{{}, Key::Left, {}};
        CORRADE_VERIFY(ui.keyPressEvent(event));
        CORRADE_COMPARE(layer.text(text), "hello");
        CORRADE_COMPARE(layer.cursor(text), Containers::pair(2u, 2u));
        CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

        /* Reset state flags */
        ui.update();
        CORRADE_COMPARE(layer.state(), LayerStates{});
    } {
        KeyEvent event{{}, Key::Right, {}};
        CORRADE_VERIFY(ui.keyPressEvent(event));
        CORRADE_COMPARE(layer.text(text), "hello");
        CORRADE_COMPARE(layer.cursor(text), Containers::pair(3u, 3u));
        CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

        /* Reset state flags */
        ui.update();
        CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Shift home / right / end / left, ordered like this so I don't need to
       specially craft cursor positions for each */
    } {
        KeyEvent event{{}, Key::Home, Modifier::Shift};
        CORRADE_VERIFY(ui.keyPressEvent(event));
        CORRADE_COMPARE(layer.text(text), "hello");
        CORRADE_COMPARE(layer.cursor(text), Containers::pair(0u, 3u));
        CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

        /* Reset state flags */
        ui.update();
        CORRADE_COMPARE(layer.state(), LayerStates{});
    } {
        KeyEvent event{{}, Key::Right, Modifier::Shift};
        CORRADE_VERIFY(ui.keyPressEvent(event));
        CORRADE_COMPARE(layer.text(text), "hello");
        CORRADE_COMPARE(layer.cursor(text), Containers::pair(1u, 3u));
        CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

        /* Reset state flags */
        ui.update();
        CORRADE_COMPARE(layer.state(), LayerStates{});
    } {
        KeyEvent event{{}, Key::End, Modifier::Shift};
        CORRADE_VERIFY(ui.keyPressEvent(event));
        CORRADE_COMPARE(layer.text(text), "hello");
        CORRADE_COMPARE(layer.cursor(text), Containers::pair(5u, 3u));
        CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

        /* Reset state flags */
        ui.update();
        CORRADE_COMPARE(layer.state(), LayerStates{});
    } {
        KeyEvent event{{}, Key::Left, Modifier::Shift};
        CORRADE_VERIFY(ui.keyPressEvent(event));
        CORRADE_COMPARE(layer.text(text), "hello");
        CORRADE_COMPARE(layer.cursor(text), Containers::pair(4u, 3u));
        CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

        /* Reset state flags */
        ui.update();
        CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Home / end, resetting the selection again */
    } {
        KeyEvent event{{}, Key::Home, {}};
        CORRADE_VERIFY(ui.keyPressEvent(event));
        CORRADE_COMPARE(layer.text(text), "hello");
        CORRADE_COMPARE(layer.cursor(text), Containers::pair(0u, 0u));
        CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

        /* Reset state flags */
        ui.update();
        CORRADE_COMPARE(layer.state(), LayerStates{});
    } {
        KeyEvent event{{}, Key::End, {}};
        CORRADE_VERIFY(ui.keyPressEvent(event));
        CORRADE_COMPARE(layer.text(text), "hello");
        CORRADE_COMPARE(layer.cursor(text), Containers::pair(5u, 5u));
        CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

        /* Reset state flags */
        ui.update();
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }

    /* Put cursor back into a non-trivial position for editing operations */
    layer.setCursor(text, 3);

    /* Backspace / delete */
    {
        KeyEvent event{{}, Key::Backspace, {}};
        CORRADE_VERIFY(ui.keyPressEvent(event));
        CORRADE_COMPARE(layer.text(text), "helo");
        CORRADE_COMPARE(layer.cursor(text), Containers::pair(2u, 2u));
        CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

        /* Reset state flags */
        ui.update();
        CORRADE_COMPARE(layer.state(), LayerStates{});
    } {
        KeyEvent event{{}, Key::Delete, {}};
        CORRADE_VERIFY(ui.keyPressEvent(event));
        CORRADE_COMPARE(layer.text(text), "heo");
        CORRADE_COMPARE(layer.cursor(text), Containers::pair(2u, 2u));
        CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

        /* Reset state flags */
        ui.update();
        CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Text input */
    } {
        TextInputEvent event{{}, "avenly may"};
        CORRADE_VERIFY(ui.textInputEvent(event));
        CORRADE_COMPARE(layer.text(text), "heavenly mayo");
        CORRADE_COMPARE(layer.cursor(text), Containers::pair(12u, 12u));
        CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

        /* Reset state flags */
        ui.update();
        CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Nothing should happen with other modifiers set */
    } {
        KeyEvent right{{}, Key::Right, Modifier::Ctrl};
        KeyEvent left{{}, Key::Left, Modifier::Ctrl};
        KeyEvent backspace{{}, Key::Backspace, Modifier::Ctrl};
        KeyEvent delete_{{}, Key::Delete, Modifier::Ctrl};
        KeyEvent home{{}, Key::Home, Modifier::Ctrl};
        KeyEvent end{{}, Key::End, Modifier::Ctrl};
        CORRADE_VERIFY(!ui.keyPressEvent(right));
        CORRADE_VERIFY(!ui.keyPressEvent(left));
        CORRADE_VERIFY(!ui.keyPressEvent(backspace));
        CORRADE_VERIFY(!ui.keyPressEvent(delete_));
        CORRADE_VERIFY(!ui.keyPressEvent(home));
        CORRADE_VERIFY(!ui.keyPressEvent(end));
        CORRADE_COMPARE(layer.text(text), "heavenly mayo");
        CORRADE_COMPARE(layer.cursor(text), Containers::pair(12u, 12u));
        CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Nothing should happen for other keys */
    } {
        KeyEvent a{{}, Key::A, {}};
        KeyEvent aShift{{}, Key::A, Modifier::Shift};
        CORRADE_VERIFY(!ui.keyPressEvent(a));
        CORRADE_VERIFY(!ui.keyPressEvent(aShift));
        CORRADE_COMPARE(layer.text(text), "heavenly mayo");
        CORRADE_COMPARE(layer.cursor(text), Containers::pair(12u, 12u));
        CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Nothing should happen for a key release */
    } {
        KeyEvent right{{}, Key::Right, Modifier::Ctrl};
        KeyEvent left{{}, Key::Left, Modifier::Ctrl};
        KeyEvent backspace{{}, Key::Backspace, Modifier::Ctrl};
        KeyEvent delete_{{}, Key::Delete, Modifier::Ctrl};
        KeyEvent home{{}, Key::Home, Modifier::Ctrl};
        KeyEvent end{{}, Key::End, Modifier::Ctrl};
        CORRADE_VERIFY(!ui.keyReleaseEvent(right));
        CORRADE_VERIFY(!ui.keyReleaseEvent(left));
        CORRADE_VERIFY(!ui.keyReleaseEvent(backspace));
        CORRADE_VERIFY(!ui.keyReleaseEvent(delete_));
        CORRADE_VERIFY(!ui.keyReleaseEvent(home));
        CORRADE_VERIFY(!ui.keyReleaseEvent(end));
        CORRADE_COMPARE(layer.text(text), "heavenly mayo");
        CORRADE_COMPARE(layer.cursor(text), Containers::pair(12u, 12u));
        CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Nothing happens with a focus lost again */
    } {
        FocusEvent blur{{}};
        CORRADE_VERIFY(!ui.focusEvent(NodeHandle::Null, blur));
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);

        /* Reset state flags */
        ui.update();
        CORRADE_COMPARE(layer.state(), LayerStates{});

        KeyEvent event{{}, Key::Left, {}};
        CORRADE_VERIFY(!ui.keyPressEvent(event));
        CORRADE_COMPARE(layer.text(text), "heavenly mayo");
        CORRADE_COMPARE(layer.cursor(text), Containers::pair(12u, 12u));
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }
}

void TextLayerTest::keyTextEventSynthesizedFromPointerPress() {
    auto&& data = KeyTextEventSynthesizedFromPointerPressData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override { return Containers::pointer<ThreeGlyphShaper>(*this); }
    } font;

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } cache{PixelFormat::R8Unorm, {32, 32, 2}};
    cache.addFont(98, &font);

    struct LayerShared: TextLayer::Shared {
        explicit LayerShared(Text::AbstractGlyphCache& glyphCache, const Configuration& configuration): TextLayer::Shared{glyphCache, configuration} {}

        void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}
    } shared{cache, TextLayer::Shared::Configuration{1}};
    shared.setStyle(TextLayerCommonStyleUniform{},
        {TextLayerStyleUniform{}},
        {shared.addFont(font, 1.0f)},
        {Text::Alignment::MiddleCenter},
        {}, {}, {}, {}, {}, {});

    struct Layer: TextLayer {
        explicit Layer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
    };

    AbstractUserInterface ui{{100, 100}};

    Layer& layer = ui.setLayerInstance(Containers::pointer<Layer>(ui.createLayer(), shared));
    EventLayer& eventLayer = ui.setLayerInstance(Containers::pointer<EventLayer>(ui.createLayer()));

    NodeHandle node = ui.createNode({}, {10, 10}, NodeFlag::Focusable);
    DataHandle text = layer.create(0, "hello", {}, TextDataFlag::Editable, node);
    CORRADE_COMPARE(layer.text(text), "hello");
    CORRADE_COMPARE(layer.cursor(text), Containers::pair(5u, 5u));

    /* Virtual keyboard */
    NodeHandle keyboard = ui.createNode({50, 0}, {50, 50}, NodeFlag::NoBlur);

    NodeHandle exclamation = ui.createNode(keyboard, {0, 0}, {10, 10});
    eventLayer.onPress(exclamation, [&]{
        TextInputEvent event{{}, "!"};
        ui.textInputEvent(event);
    });

    NodeHandle backspace = ui.createNode(keyboard, {10, 0}, {10, 10});
    eventLayer.onPress(backspace, [&]{
        KeyEvent event{{}, Key::Backspace, {}};
        ui.keyPressEvent(event);
    });

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }

    /* Focus the node */
    {
        FocusEvent event{{}};
        ui.focusEvent(node, event);
        CORRADE_COMPARE(ui.currentFocusedNode(), node);
    }

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }

    /* Delete last char. The text node stays focused and is updated. */
    {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0, {}};
        CORRADE_VERIFY(ui.pointerPressEvent({65, 5}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), backspace);
        CORRADE_COMPARE(ui.currentFocusedNode(), node);
        CORRADE_COMPARE(layer.text(text), "hell");
        CORRADE_COMPARE(layer.cursor(text), Containers::pair(4u, 4u));
    }

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }

    /* Add an exclamation mark. Again it stays focused and is updated. */
    {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0, {}};
        CORRADE_VERIFY(ui.pointerPressEvent({55, 5}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), exclamation);
        CORRADE_COMPARE(ui.currentFocusedNode(), node);
        CORRADE_COMPARE(layer.text(text), "hell!");
        CORRADE_COMPARE(layer.cursor(text), Containers::pair(5u, 5u));
    }

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }

    /* More!! */
    {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0, {}};
        CORRADE_VERIFY(ui.pointerPressEvent({55, 5}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), exclamation);
        CORRADE_COMPARE(ui.currentFocusedNode(), node);
        CORRADE_COMPARE(layer.text(text), "hell!!");
        CORRADE_COMPARE(layer.cursor(text), Containers::pair(6u, 6u));
    }

    if(data.update) {
        ui.update();
        CORRADE_COMPARE(layer.state(), LayerStates{});
    }
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::TextLayerTest)
