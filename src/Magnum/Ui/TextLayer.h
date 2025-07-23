#ifndef Magnum_Ui_TextLayer_h
#define Magnum_Ui_TextLayer_h
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

/** @file
 * @brief Class @ref Magnum::Ui::TextLayer, struct @ref Magnum::Ui::TextLayerCommonStyleUniform, @ref Magnum::Ui::TextLayerStyleUniform, enum @ref Magnum::Ui::FontHandle, @ref Magnum::Ui::TextDataFlag, @ref Magnum::Ui::TextEdit, enum set @ref Magnum::Ui::TextDataFlags, function @ref Magnum::Ui::fontHandle(), @ref Magnum::Ui::fontHandleId(), @ref Magnum::Ui::fontHandleGeneration()
 * @m_since_latest
 */

#include <initializer_list>
#include <Corrade/Containers/StringView.h> /* for templated create() overloads */
#include <Corrade/PluginManager/Manager.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Text/Text.h>

#include "Magnum/Ui/AbstractVisualLayer.h"

namespace Magnum { namespace Ui {

/**
@brief Properties common to all @ref TextLayer style uniforms
@m_since_latest

See the @ref TextLayer class documentation for information about setting up an
instance of the text layer and using it.

Together with one or more @ref TextLayerStyleUniform instances contains style
properties that are used by the @ref TextLayer shaders to draw the layer data,
packed in a form that allows direct usage in uniform buffers. Is uploaded
using @ref TextLayer::Shared::setStyle(), style data that aren't used by the
shader are passed to the function separately.

Currently this is just a placeholder with no properties.
*/
struct TextLayerCommonStyleUniform {
    /** @brief Construct with default values */
    constexpr explicit TextLayerCommonStyleUniform(DefaultInitT) noexcept: smoothness{0.0f} {}

    /**
     * @brief Construct with default values
     *
     * Equivalent to @ref TextLayerCommonStyleUniform(DefaultInitT).
     */
    constexpr /*implicit*/ TextLayerCommonStyleUniform() noexcept: TextLayerCommonStyleUniform{DefaultInit} {}

    /** @brief Constructor */
    constexpr /*implicit*/ TextLayerCommonStyleUniform(Float smoothness): smoothness {smoothness} {}

    /** @brief Construct without initializing the contents */
    explicit TextLayerCommonStyleUniform(NoInitT) noexcept {}

    /** @{
     * @name Convenience setters
     *
     * Provided to allow the use of method chaining for populating a structure
     * in a single expression, otherwise equivalent to accessing the fields
     * directly. Also guaranteed to provide backwards compatibility when
     * packing of the actual fields changes.
     */

    /**
     * @brief Set the @ref smoothness field
     * @return Reference to self (for method chaining)
     */
    CORRADE_CONSTEXPR14 TextLayerCommonStyleUniform& setSmoothness(Float smoothness) {
        this->smoothness = smoothness;
        return *this;
    }

    /**
     * @}
     */

    /**
     * @brief Edge smoothness radius
     *
     * Used only if the text layer is created with a distance field glyph
     * cache, ignored otherwise. In framebuffer pixels (as opposed to UI
     * units). E.g., setting the value to @cpp 1.0f @ce will make the smoothing
     * extend 1 pixel on each side of the edge. Default value is @cpp 0.0f @ce.
     * The bigger value between this and
     * @ref TextLayerStyleUniform::smoothness, converted to pixels, gets used.
     * @see @ref TextLayerGL::Shared::Shared(Text::DistanceFieldGlyphCacheArrayGL&, const Configuration&),
     *      @ref TextLayerSharedFlag::DistanceField,
     *      @ref Ui-AbstractUserInterface-dpi
     */
    Float smoothness;

    #ifndef DOXYGEN_GENERATING_OUTPUT
    Int:32;
    Int:32;
    Int:32;
    #endif
};

/**
@brief @ref TextLayer style uniform
@m_since_latest

See the @ref TextLayer class documentation for information about setting up an
instance of the text layer and using it.

Instances of this class together with @ref TextLayerCommonStyleUniform contain
style properties that are used by the @ref TextLayer shaders to draw the layer
data, packed in a form that allows direct usage in uniform buffers. Total count
of styles is specified with the
@ref TextLayer::Shared::Configuration::Configuration() constructor, uniforms
are then uploaded using @ref TextLayer::Shared::setStyle(), style data that
aren't used by the shader are passed to the function separately. If dynamic
styles are enabled with @ref TextLayer::Shared::Configuration::setDynamicStyleCount(),
instances of this class are also passed to @ref TextLayer::setDynamicStyle()
and variants.
*/
struct TextLayerStyleUniform {
    /** @brief Construct with default values */
    constexpr explicit TextLayerStyleUniform(DefaultInitT) noexcept: color{1.0f}, outlineColor{1.0f}, outlineWidth{0.0f}, edgeOffset{0.0f}, smoothness{0.0f} {}

    /**
     * @brief Construct with default values
     *
     * Equivalent to @ref TextLayerStyleUniform(DefaultInitT).
     */
    constexpr /*implicit*/ TextLayerStyleUniform() noexcept: TextLayerStyleUniform{DefaultInit} {}

    /**
     * @brief Construct for a regular rendering
     *
     * The @ref outlineColor, @ref outlineWidth, @ref edgeOffset and
     * @ref smoothness are left at their default values.
     */
    constexpr /*implicit*/ TextLayerStyleUniform(const Color4& color): color{color}, outlineColor{1.0f}, outlineWidth{0.0f}, edgeOffset{0.0f}, smoothness{0.0f} {}

    /** @brief Construct for a distance field rendering */
    constexpr /*implicit*/ TextLayerStyleUniform(const Color4& color, const Color4& outlineColor, Float outlineWidth, Float edgeOffset, Float smoothness): color{color}, outlineColor{outlineColor}, outlineWidth{outlineWidth}, edgeOffset{edgeOffset}, smoothness{smoothness} {}

    /** @brief Construct without initializing the contents */
    explicit TextLayerStyleUniform(NoInitT) noexcept: color{NoInit} {}

    /** @{
     * @name Convenience setters
     *
     * Provided to allow the use of method chaining for populating a structure
     * in a single expression, otherwise equivalent to accessing the fields
     * directly. Also guaranteed to provide backwards compatibility when
     * packing of the actual fields changes.
     */

    /**
     * @brief Set the @ref color field
     * @return Reference to self (for method chaining)
     */
    CORRADE_CONSTEXPR14 TextLayerStyleUniform& setColor(const Color4& color) {
        this->color = color;
        return *this;
    }

    /**
     * @brief Set the @ref outlineColor field
     * @return Reference to self (for method chaining)
     */
    CORRADE_CONSTEXPR14 TextLayerStyleUniform& setOutlineColor(const Color4& color) {
        outlineColor = color;
        return *this;
    }

    /**
     * @brief Set the @ref outlineWidth field
     * @return Reference to self (for method chaining)
     */
    CORRADE_CONSTEXPR14 TextLayerStyleUniform& setOutlineWidth(Float width) {
        outlineWidth = width;
        return *this;
    }

    /**
     * @brief Set the @ref edgeOffset field
     * @return Reference to self (for method chaining)
     */
    CORRADE_CONSTEXPR14 TextLayerStyleUniform& setEdgeOffset(Float offset) {
        edgeOffset = offset;
        return *this;
    }

    /**
     * @brief Set the @ref smoothness field
     * @return Reference to self (for method chaining)
     */
    CORRADE_CONSTEXPR14 TextLayerStyleUniform& setSmoothness(Float smoothness) {
        this->smoothness = smoothness;
        return *this;
    }

    /**
     * @}
     */

    /**
     * @brief Color
     *
     * The color is expected to have premultiplied alpha. Default value is
     * @cpp 0xffffffff_srgbaf @ce. The color multiplies the glyph texture and
     * is further multiplied with per-data value supplied with
     * @ref TextLayer::setColor() and node opacity coming from
     * @ref AbstractUserInterface::setNodeOpacity().
     * @see @ref Color4::premultiplied()
     */
    Color4 color;

    /**
     * @brief Outline color
     *
     * Used only if the text layer is created with a distance field glyph
     * cache, ignored otherwise. The color is expected to have premultiplied
     * alpha. Default value is @cpp 0xffffffff_srgbaf @ce. Visible only if
     * @ref outlineWidth is non-zero.
     *
     * Unlike @ref color, the outline color isn't multiplied with
     * @ref TextLayer::setColor(). Node opacity coming from
     * @ref AbstractUserInterface::setNodeOpacity() affects it however.
     * @see @ref TextLayerGL::Shared::Shared(Text::DistanceFieldGlyphCacheArrayGL&, const Configuration&),
     *      @ref Color4::premultiplied()
     */
    Color4 outlineColor;

    /**
     * @brief Outline width
     *
     * Used only if the text layer is created with a distance field glyph
     * cache, ignored otherwise. In UI units, the actual outline width is
     * clamped to what the distance field radius allows. Default value is
     * @cpp 0.0f @ce.
     * @see @ref TextLayerGL::Shared::Shared(Text::DistanceFieldGlyphCacheArrayGL&, const Configuration&)
     */
    Float outlineWidth;

    /**
     * @brief Edge offset
     *
     * Used only if the text layer is created with a distance field glyph
     * cache, ignored otherwise. In UI units, positive values dilate the edges
     * and negative values erode it, the actual offset is clamped to what the
     * distance field radius allows. Default value is @cpp 0.0f @ce.
     * @see @ref TextLayerGL::Shared::Shared(Text::DistanceFieldGlyphCacheArrayGL&, const Configuration&)
     */
    Float edgeOffset;

    /**
     * @brief Edge smoothness radius
     *
     * Used only if the text layer is created with a distance field glyph
     * cache, ignored otherwise. Compared to
     * @ref TextLayerCommonStyleUniform::smoothness is in UI units instead of
     * pixels. Default is @cpp 0.0f @ce. Of the two, the larger value in pixels
     * gets used.
     * @see @ref TextLayerGL::Shared::Shared(Text::DistanceFieldGlyphCacheArrayGL&, const Configuration&)
     */
    Float smoothness;

    #ifndef DOXYGEN_GENERATING_OUTPUT
    Int:32;
    #endif
};

/**
@brief Properties common to all @ref TextLayer editing style uniforms
@m_since_latest

See the @ref TextLayer class documentation for information about setting up an
instance of the text layer and using it, and @ref Ui-TextLayer-editing in
particular for setting up styles for editable text.

Together with one or more @ref TextLayerEditingStyleUniform instances contains
style properties that are used by the @ref TextLayer shaders to draw the layer
editing data such as cursors and selection rectangles, packed in a form that
allows direct usage in uniform buffers. Is uploaded using
@ref TextLayer::Shared::setEditingStyle(), style data that aren't used by the
shader are passed to the function separately.
*/
struct TextLayerCommonEditingStyleUniform {
    /** @brief Construct with default values */
    constexpr explicit TextLayerCommonEditingStyleUniform(DefaultInitT) noexcept: smoothness{0.0f} {}

    /**
     * @brief Construct with default values
     *
     * Equivalent to @ref TextLayerCommonEditingStyleUniform(DefaultInitT).
     */
    constexpr /*implicit*/ TextLayerCommonEditingStyleUniform() noexcept: TextLayerCommonEditingStyleUniform{DefaultInit} {}

    /** @brief Constructor */
    constexpr /*implicit*/ TextLayerCommonEditingStyleUniform(Float smoothness): smoothness{smoothness} {}

    /** @brief Construct without initializing the contents */
    explicit TextLayerCommonEditingStyleUniform(NoInitT) noexcept {}

    /** @{
     * @name Convenience setters
     *
     * Provided to allow the use of method chaining for populating a structure
     * in a single expression, otherwise equivalent to accessing the fields
     * directly. Also guaranteed to provide backwards compatibility when
     * packing of the actual fields changes.
     */
    /**
     * @brief Set the @ref smoothness field
     * @return Reference to self (for method chaining)
     */
    TextLayerCommonEditingStyleUniform& setSmoothness(Float smoothness) {
        this->smoothness = smoothness;
        return *this;
    }

    /**
     * @}
     */

    /**
     * @brief Edge smoothness radius
     *
     * In framebuffer pixels (as opposed to UI units). E.g., setting the value
     * to @cpp 1.0f @ce will make the smoothing extend 1 pixel on each side of
     * the edge. Default value is @cpp 0.0f @ce.
     * @see @ref Ui-AbstractUserInterface-dpi
     */
    Float smoothness;

    /** @todo remove the alignas once actual attributes are here */
    #ifndef DOXYGEN_GENERATING_OUTPUT
    Int:32;
    Int:32;
    Int:32;
    #endif
};

/**
@brief @ref TextLayer editing style uniform
@m_since_latest

See the @ref TextLayer class documentation for information about setting up an
instance of the text layer and using it, and @ref Ui-TextLayer-editing in
particular for setting up styles for editable text.

Instances of this class together with @ref TextLayerCommonEditingStyleUniform
contain style properties that are used by the @ref TextLayer shaders to draw
the layer editing data such as cursors and selection rectangles, packed in a
form that allows direct usage in uniform buffers. Total count of styles is
specified with @ref TextLayer::Shared::Configuration::setEditingStyleCount(),
uniforms are then uploaded using @ref TextLayer::Shared::setEditingStyle(),
style data that aren't used by the shader are passed to the function
separately.
*/
struct TextLayerEditingStyleUniform {
    /** @brief Construct with default values */
    constexpr explicit TextLayerEditingStyleUniform(DefaultInitT) noexcept: backgroundColor{1.0f}, cornerRadius{0.0f} {}

    /**
     * @brief Construct with default values
     *
     * Equivalent to @ref TextLayerEditingStyleUniform(DefaultInitT).
     */
    constexpr /*implicit*/ TextLayerEditingStyleUniform() noexcept: TextLayerEditingStyleUniform{DefaultInit} {}

    /** @brief Constructor */
    constexpr /*implicit*/ TextLayerEditingStyleUniform(const Color4& backgroundColor, Float cornerRadius): backgroundColor{backgroundColor}, cornerRadius{cornerRadius} {}

    /** @brief Construct without initializing the contents */
    explicit TextLayerEditingStyleUniform(NoInitT) noexcept: backgroundColor{NoInit} {}

    /** @{
     * @name Convenience setters
     *
     * Provided to allow the use of method chaining for populating a structure
     * in a single expression, otherwise equivalent to accessing the fields
     * directly. Also guaranteed to provide backwards compatibility when
     * packing of the actual fields changes.
     */

    /**
     * @brief Set the @ref backgroundColor field
     * @return Reference to self (for method chaining)
     */
    TextLayerEditingStyleUniform& setBackgroundColor(const Color4& color) {
        this->backgroundColor = color;
        return *this;
    }

    /**
     * @brief Set the @ref cornerRadius field
     * @return Reference to self (for method chaining)
     */
    TextLayerEditingStyleUniform& setCornerRadius(Float radius) {
        this->cornerRadius = radius;
        return *this;
    }

    /**
     * @}
     */

    /**
     * @brief Selection background color
     *
     * The color is expected to have premultiplied alpha. Default value is
     * @cpp 0xffffffff_srgbaf @ce. Is further multiplied with node opacity
     * coming from @ref AbstractUserInterface::setNodeOpacity(). Color and
     * other style properties for selection *text* are applied directly to
     * individual glyphs and are thus supplied separately through the
     * @p textUniforms argument to @ref TextLayer::Shared::setEditingStyle().
     * @see @ref Color4::premultiplied()
     */
    Color4 backgroundColor;

    /**
     * @brief Corner radius
     *
     * In UI units. Default value is @cpp 0.0f @ce. Note that unlike
     * @ref BaseLayerStyleUniform::cornerRadius this is just a single value for
     * all corners.
     */
    Float cornerRadius;

    #ifndef DOXYGEN_GENERATING_OUTPUT
    Int:32;
    Int:32;
    Int:32;
    #endif
};

/* Unlike DataHandle, NodeHandle etc., which are global to the whole Ui
   library, FontHandle is specific to the TextLayer and thus isn't defined in
   Handle.h but here */

namespace Implementation {
    enum: UnsignedInt {
        FontHandleIdBits = 15,
        FontHandleGenerationBits = 1
    };
}

/**
@brief Font handle
@m_since_latest

Used for identifying fonts in the @ref TextLayer. See its documentation for
information about setting up an instance of the text layer and using it.

Uses 15 bits for storing an ID and 1 bit for a generation.
@see @ref TextLayer::Shared::addFont(), @ref fontHandle(), @ref fontHandleId(),
    @ref fontHandleGeneration()
*/
enum class FontHandle: UnsignedShort {
    Null = 0    /**< Null handle */
};

/**
@debugoperatorenum{FontHandle}
@m_since_latest
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, FontHandle value);

/**
@brief Compose a font handle from an ID and a generation
@m_since_latest

Expects that the ID fits into 20 bits and the generation into 12 bits. Use
@ref fontHandleId() and @ref fontHandleGeneration() for an inverse operation.
*/
constexpr FontHandle fontHandle(UnsignedInt id, UnsignedInt generation) {
    return (CORRADE_CONSTEXPR_DEBUG_ASSERT(id < (1 << Implementation::FontHandleIdBits) && generation < (1 << Implementation::FontHandleGenerationBits),
        "Ui::fontHandle(): expected index to fit into" << Implementation::FontHandleIdBits << "bits and generation into" << Implementation::FontHandleGenerationBits << Debug::nospace << ", got" << Debug::hex << id << "and" << Debug::hex << generation), FontHandle(id|(generation << Implementation::FontHandleIdBits)));
}

/**
@brief Extract ID from a font handle
@m_since_latest

Expects that @p handle is not @ref FontHandle::Null. Use
@ref fontHandleGeneration() for extracting the generation and @ref fontHandle()
for an inverse operation.
*/
constexpr UnsignedInt fontHandleId(FontHandle handle) {
    return (CORRADE_CONSTEXPR_DEBUG_ASSERT(handle != FontHandle::Null,
        "Ui::fontHandleId(): the handle is null"),
        UnsignedInt(handle) & ((1 << Implementation::FontHandleIdBits) - 1));
}

/**
@brief Extract generation from a font handle
@m_since_latest

For @ref FontHandle::Null returns @cpp 0 @ce. A valid handle has always a
non-zero generation. Use @ref fontHandleId() for extracting the ID and
@ref fontHandle() for an inverse operation.
*/
constexpr UnsignedInt fontHandleGeneration(FontHandle handle) {
    return UnsignedInt(handle) >> Implementation::FontHandleIdBits;
}

/**
@brief Text layer flag
@m_since_latest

@see @ref TextLayerFlags, @ref TextLayer::flags()
*/
enum class TextLayerFlag: UnsignedByte {
    /**
     * Enable arbitrary text transformation using
     * @ref TextLayer::setTransformation(),
     * @relativeref{TextLayer,translate()}, @relativeref{TextLayer,rotate()}
     * and @relativeref{TextLayer,scale()}. This feature replaces custom text
     * padding, meaning that @ref TextLayer::setPadding() cannot be called with
     * this flag enabled. Only the paddings supplied via
     * @ref TextLayer::Shared::setStyle() are used. At the moment, text also
     * can't be marked as @ref TextDataFlag::Editable if this flag is enabled,
     * as cursor placement with arbitrarily transformed text isn't implemented
     * yet.
     *
     * Note that internally the glyph quad transformation is performed on the
     * CPU side, which may have performance implications if enabled globally
     * and not just on text that actually needs arbitrary transformation ---
     * for that reason this is a per-layer flag and not a
     * @ref TextLayerSharedFlag so you can have a dedicated layer instance with
     * all other state shared for just transformed texts alone.
     *
     * For crisp rendering with arbitrary rotation and scaling it's recommended
     * to use this feature together with
     * @ref Ui-TextLayer-distancefield "distance field rendering".
     */
    Transformable = 1 << 0
};

/**
@debugoperatorenum{TextLayerFlag}
@m_since_latest
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, TextLayerFlag value);

/**
@brief Text layer data flags
@m_since_latest

@see @ref TextLayer::flags()
*/
typedef Containers::EnumSet<TextLayerFlag> TextLayerFlags;

CORRADE_ENUMSET_OPERATORS(TextLayerFlags)

/**
@debugoperatorenum{TextLayerFlags}
@m_since_latest
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, TextLayerFlags value);

/**
@brief Text layer data flag
@m_since_latest

@see @ref Ui-TextLayer-editing, @ref TextDataFlags, @ref TextLayer::create(),
    @ref TextLayer::flags(DataHandle) const
*/
enum class TextDataFlag: UnsignedByte {
    /**
     * Editable text. If data that have it enabled are attached to a currently
     * focused node, the layer reacts to text input and key events, allowing to
     * edit the contents. Unlike non-editable text, the contents are also
     * accessible through @ref TextLayer::text().
     *
     * If editing styles are present and used by given style, the cursor
     * position and/or selection is drawn as well.
     * @see @ref TextLayer::Shared::Configuration::setEditingStyleCount(),
     *      @ref TextLayer::Shared::setEditingStyle()
     */
    Editable = 1 << 0,
};

/**
@debugoperatorenum{TextDataFlag}
@m_since_latest
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, TextDataFlag value);

/**
@brief Text layer data flags
@m_since_latest

@see @ref TextLayer::create(), @ref TextLayer::flags(DataHandle) const
*/
typedef Containers::EnumSet<TextDataFlag> TextDataFlags;

CORRADE_ENUMSET_OPERATORS(TextDataFlags)

/**
@debugoperatorenum{TextDataFlags}
@m_since_latest
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, TextDataFlags value);

/**
@brief Text edit operation
@m_since_latest

@see @ref Ui-TextLayer-editing, @ref TextLayer::editText()
*/
enum class TextEdit: UnsignedByte {
    /**
     * Move cursor one character to the left and discard any selection,
     * equivalent to the @m_class{m-label m-default} **Left** key. The
     * @ref TextLayer::editText() @p text is expected to be empty.
     *
     * If the character on the left to the @ref TextLayer::cursor() is a valid
     * UTF-8 byte sequence, it moves past the whole sequence, otherwise just by
     * one byte. Note that, currently, if a character is combined out of
     * multiple Unicode codepoints (such as combining diacritics), the cursor
     * will not jump over the whole combined sequence but move inside.
     *
     * By default, the cursor moves backward in the byte stream, which is
     * optically to the left for left-to-right alphabets. If
     * @ref TextProperties::shapeDirection() is
     * @ref Text::ShapeDirection::RightToLeft or if it's
     * @relativeref{Text::ShapeDirection,Unspecified} and was detected to be
     * @relativeref{Text::ShapeDirection,RightToLeft} after calling
     * @ref Text::AbstractShaper::shape() internally, the cursor still moves
     * *optically* to the left, although it's forward in the byte stream.
     * @see @ref TextEdit::ExtendSelectionLeft
     */
    MoveCursorLeft,

    /**
     * Extend or shrink selection to the left of the cursor, equivalent to the
     * @m_class{m-label m-warning} **Shift** @m_class{m-label m-default} **Left**
     * key combination. The @ref TextLayer::editText() @p text is expected to
     * be empty.
     *
     * If the character on the left to the @ref TextLayer::cursor() is a valid
     * UTF-8 byte sequence, it extends past the whole sequence, otherwise just
     * by one byte. Note that, currently, if a character is combined out of
     * multiple Unicode codepoints (such as combining diacritics), the cursor
     * will not jump over the whole combined sequence but move inside.
     *
     * By default, the cursor moves backward in the byte stream, which is
     * optically to the left for left-to-right alphabets. If
     * @ref TextProperties::shapeDirection() is
     * @ref Text::ShapeDirection::RightToLeft or if it's
     * @relativeref{Text::ShapeDirection,Unspecified} and was detected to be
     * @relativeref{Text::ShapeDirection,RightToLeft} after calling
     * @ref Text::AbstractShaper::shape() internally, the cursor still moves
     * *optically* to the left, although it's forward in the byte stream.
     * @see @ref TextEdit::MoveCursorLeft
     */
    ExtendSelectionLeft,

    /**
     * Move cursor one character to the right and discard any selection,
     * equivalent to the @m_class{m-label m-default} **Right** key. The
     * @ref TextLayer::editText() @p text is expected to be empty.
     *
     * If the character on the right to the @ref TextLayer::cursor() is a valid
     * UTF-8 byte sequence, it moves past the whole sequence, otherwise just by
     * one byte. Note that, currently, if a character is combined out of
     * multiple Unicode codepoints (such as combining diacritics), the cursor
     * will not jump over the whole combined sequence but move inside.
     *
     * By default, the cursor moves forward in the byte stream, which is
     * optically to the right for left-to-right alphabets. If
     * @ref TextProperties::shapeDirection() is
     * @ref Text::ShapeDirection::RightToLeft or if it's
     * @relativeref{Text::ShapeDirection,Unspecified} and was detected to be
     * @relativeref{Text::ShapeDirection,RightToLeft} after calling
     * @ref Text::AbstractShaper::shape() internally, the cursor still moves
     * *optically* to the right, although it's backward in the byte stream.
     * @see @ref TextEdit::ExtendSelectionRight
     */
    MoveCursorRight,

    /**
     * Extend or shrink selection to the right of the cursor, equivalent to the
     * @m_class{m-label m-warning} **Shift** @m_class{m-label m-default} **Right**
     * key combination. The @ref TextLayer::editText() @p text is expected to
     * be empty.
     *
     * If the character on the right to the @ref TextLayer::cursor() is a valid
     * UTF-8 byte sequence, it moves past the whole sequence, otherwise just by
     * one byte. Note that, currently, if a character is combined out of
     * multiple Unicode codepoints (such as combining diacritics), the cursor
     * will not jump over the whole combined sequence but move inside.
     *
     * By default, the cursor moves forward in the byte stream, which is
     * optically to the right for left-to-right alphabets. If
     * @ref TextProperties::shapeDirection() is
     * @ref Text::ShapeDirection::RightToLeft or if it's
     * @relativeref{Text::ShapeDirection,Unspecified} and was detected to be
     * @relativeref{Text::ShapeDirection,RightToLeft} after calling
     * @ref Text::AbstractShaper::shape() internally, the cursor still moves
     * *optically* to the right, although it's backward in the byte stream.
     * @see @ref TextEdit::MoveCursorRight
     */
    ExtendSelectionRight,

    /**
     * Move cursor at the beginning of the line and discard any selection,
     * equivalent to the @m_class{m-label m-default} **Home** key. The
     * @ref TextLayer::editText() @p text is expected to be empty.
     *
     * The cursor moves to the begin of the byte stream, which is optically on
     * the left for left-to-right alphabets. If
     * @ref TextProperties::shapeDirection() is
     * @ref Text::ShapeDirection::RightToLeft or if it's
     * @relativeref{Text::ShapeDirection,Unspecified} and was detected to be
     * @relativeref{Text::ShapeDirection,RightToLeft} after calling
     * @ref Text::AbstractShaper::shape() internally, it's still the begin of
     * the byte stream, but *optically* after the rightmost character.
     * @see @ref TextEdit::ExtendSelectionLineBegin
     */
    MoveCursorLineBegin,

    /**
     * Extend selection to the beginning of the line, equivalent to the
     * @m_class{m-label m-warning} **Shift** @m_class{m-label m-default} **Home**
     * key combination. The @ref TextLayer::editText() @p text is expected to
     * be empty.
     *
     * The cursor moves to the begin of the byte stream, which is optically on
     * the left for left-to-right alphabets. If
     * @ref TextProperties::shapeDirection() is
     * @ref Text::ShapeDirection::RightToLeft or if it's
     * @relativeref{Text::ShapeDirection,Unspecified} and was detected to be
     * @relativeref{Text::ShapeDirection,RightToLeft} after calling
     * @ref Text::AbstractShaper::shape() internally, it's still the begin of
     * the byte stream, but *optically* after the rightmost character.
     * @see @ref TextEdit::MoveCursorLineBegin
     */
    ExtendSelectionLineBegin,

    /**
     * Move cursor at the end of the line and discard any selection, equivalent
     * to the @m_class{m-label m-default} **End** key. The
     * @ref TextLayer::editText() @p text is expected to be empty.
     *
     * The cursor moves to the end of in the byte stream, which is optically on
     * the right for left-to-right alphabets. If
     * @ref TextProperties::shapeDirection() is
     * @ref Text::ShapeDirection::RightToLeft or if it's
     * @relativeref{Text::ShapeDirection,Unspecified} and was detected to be
     * @relativeref{Text::ShapeDirection,RightToLeft} after calling
     * @ref Text::AbstractShaper::shape() internally, it's still the end ofthe
     * byte stream, but *optically* before the leftmost character.
     * @see @ref TextEdit::ExtendSelectionLineEnd
     */
    MoveCursorLineEnd,

    /**
     * Extend selection to the end of the line, equivalent to the
     * @m_class{m-label m-warning} **Shift** @m_class{m-label m-default} **End**
     * key combination. The @ref TextLayer::editText() @p text is expected to
     * be empty.
     *
     * The cursor moves to the end of in the byte stream, which is optically on
     * the right for left-to-right alphabets. If
     * @ref TextProperties::shapeDirection() is
     * @ref Text::ShapeDirection::RightToLeft or if it's
     * @relativeref{Text::ShapeDirection,Unspecified} and was detected to be
     * @relativeref{Text::ShapeDirection,RightToLeft} after calling
     * @ref Text::AbstractShaper::shape() internally, it's still the end ofthe
     * byte stream, but *optically* before the leftmost character.
     * @see @ref TextEdit::MoveCursorLineEnd
     */
    ExtendSelectionLineEnd,

    /**
     * Remove selection or character before cursor, equivalent to the
     * @m_class{m-label m-default} **Backspace** key. The
     * @ref TextLayer::editText() @p text is expected to be empty.
     *
     * If the character before @ref TextLayer::cursor() is a valid UTF-8 byte
     * sequence, it deletes the whole sequence, otherwise just one byte. Note
     * that, currently, if a character is combined out of multiple Unicode
     * codepoints (such as combining diacritics), the cursor will not delete
     * the whole combined sequence but only a part.
     *
     * The deletion goes backward in the byte stream, which is optically to the
     * left for left-to-right alphabets. If
     * @ref TextProperties::shapeDirection() is
     * @ref Text::ShapeDirection::RightToLeft or if it's
     * @relativeref{Text::ShapeDirection,Unspecified} and was detected to be
     * @relativeref{Text::ShapeDirection,RightToLeft} after calling
     * @ref Text::AbstractShaper::shape() internally, the deletion still goes
     * backward in the byte stream, but *optically* it's to the right.
     *
     * For a non-empty selection the behavior is the same as
     * @ref TextEdit::RemoveAfterCursor.
     */
    RemoveBeforeCursor,

    /**
     * Remove selection or character after cursor, equivalent to the
     * @m_class{m-label m-default} **Delete** key. The
     * @ref TextLayer::editText() @p text is expected to be empty.
     *
     * If the character after @ref TextLayer::cursor() is a valid UTF-8 byte
     * sequence, it deletes the whole sequence, otherwise just one byte. Note
     * that, currently, if a character is combined out of multiple Unicode
     * codepoints (such as combining diacritics), the cursor will not delete
     * the whole combined sequence but only a part.
     *
     * The deletion goes forward in the byte stream, which is optically to the
     * right for left-to-right alphabets. If
     * @ref TextProperties::shapeDirection() is
     * @ref Text::ShapeDirection::RightToLeft or if it's
     * @relativeref{Text::ShapeDirection,Unspecified} and was detected to be
     * @relativeref{Text::ShapeDirection,RightToLeft} after calling
     * @ref Text::AbstractShaper::shape() internally, the deletion still goes
     * forward in the byte stream, but *optically* it's to the left.
     *
     * For a non-empty selection the behavior is the same as
     * @ref TextEdit::RemoveBeforeCursor.
     */
    RemoveAfterCursor,

    /**
     * Insert text before the cursor, replacing the selection if any. This will
     * cause the cursor to advance *after* the inserted text, which is the
     * usual text editing behavior.
     *
     * No UTF-8 byte sequence adjustment is performed in this case, i.e. it's
     * assumed that the cursor is at a valid UTF-8 character boundary and the
     * inserted text ends with a valid UTF-8 character as well.
     *
     * The insertion goes forward in the byte stream, which is optically to the
     * right for left-to-right alphabets. If
     * @ref TextProperties::shapeDirection() is
     * @ref Text::ShapeDirection::RightToLeft or if it's
     * @relativeref{Text::ShapeDirection,Unspecified} and was detected to be
     * @relativeref{Text::ShapeDirection,RightToLeft} after calling
     * @ref Text::AbstractShaper::shape() internally, the insertion still goes
     * forward in the byte stream, but *optically* it's to the left.
     */
    InsertBeforeCursor,

    /**
     * Insert text after the cursor, replacing the selection if any. Compared
     * to @ref TextEdit::InsertBeforeCursor, the cursor stays at the original
     * position, *before* the inserted text, other than that the behavior is
     * the same. Useful for autocompletion hints, for example.
     *
     * Like with @ref TextEdit::InsertBeforeCursor, no UTF-8 byte sequence
     * adjustment is performed in this case, i.e. it's assumed that the cursor
     * is at a valid UTF-8 character boundary and the inserted text ends with a
     * valid UTF-8 character as well.
     */
    InsertAfterCursor,
};

/**
@debugoperatorenum{TextEdit}
@m_since_latest
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, TextEdit value);

/**
@brief Text layer
@m_since_latest

Draws text laid out using the @ref Text library, including editing
capabilities.

@section Ui-TextLayer-setup Setting up a text layer instance

If you create a @ref UserInterfaceGL with a style and don't exclude
@ref StyleFeature::TextLayer, an implicit instance of the @ref TextLayerGL
subclass, configured for use with builtin widgets, is already provided and
available through @ref UserInterface::textLayer().

For a custom layer, you first need to instantiate @ref TextLayer::Shared,
which contains a glyph cache, font instances, GPU shaders and style
definitions. It's constructed from a glyph cache reference and a configuration
instance. In case of the @ref TextLayerGL implementation the cache is
@ref Text::GlyphCacheArrayGL. Assuming monochrome fonts, construct it with a
single-channel pixel format and a size large enough to fit pre-rendered glyphs
of all fonts you'll need:

@snippet Ui-gl.cpp TextLayer-setup-glyph-cache

The glyph cache is expected to be alive for the whole shared instance lifetime,
alternatively you can use the
@ref TextLayerGL::Shared::Shared(Text::GlyphCacheArrayGL&&, const Configuration&)
constructor to move its ownership to the shared instance. For the
@ref TextLayer::Shared::Configuration, at the very least you have to specify
how many distinct visual *styles* you intend to use, which is for example
the number @cpp 3 @ce in the following snippet:

@snippet Ui-gl.cpp TextLayer-setup-shared

The shared instance, in this case a concrete @ref TextLayerGL::Shared subclass
for the OpenGL implementation of this layer, is then passed to the layer
constructor alongside a fresh @ref AbstractUserInterface::createLayer() handle,
and is expected to stay alive for the whole layer lifetime. The shared instance
can be used by multiple layers, for example if the application wants to have a
dedicated layer for very dynamic UI content, or if it combines visual options
that have to be hardcoded in particular @ref TextLayer::Shared instances. To
make the layer available as the implicit @ref UserInterface::textLayer(), pass
it to @ref UserInterfaceGL::setTextLayerInstance():

@snippet Ui-gl.cpp TextLayer-setup-implicit

Otherwise, if you want to set up a custom base layer that's independent of the
one exposed through @ref UserInterface::textLayer(), possibly for completely
custom widgets, pass the newly created layer to
@ref AbstractUserInterface::setLayerInstance() instead:

@snippet Ui-gl.cpp TextLayer-setup

Afterwards, in order to be able to draw the layer, at least one font has to be
added, and a style referencing the fonts has to be set. @ref Text::AbstractFont
instances, commonly loaded using a plugin manager, are added with
@ref TextLayerGL::Shared::addFont(), together with specifying size in UI
coordinates at which a text using them should be rendered. The function then
returns a @ref FontHandle, which is subsequently used to reference the font
from styles. As with the glyph cache, the font instance is expected to be alive
for the whole shared instance lifetime, or you can use
@ref TextLayerGL::Shared::addFont(Containers::Pointer<Text::AbstractFont>&&, Float)
to move the loaded plugin ownership to the shared instance. Note that you're
still responsible for keeping the plugin manager instance around even in that
case.

@snippet Ui.cpp TextLayer-setup-fonts

<b></b>

@m_class{m-note m-warning}

@par
    At the moment, there's no mechanism in place to automatically fill the
    glyph cache based on what glyphs are used, you have to do it explicitly
    using @ref Text::AbstractFont::fillGlyphCache().

Assuming the UI size matches the framebuffer size, a good default is to use the
same size in @ref Text::AbstractFont::openFile() /
@relativeref{Text::AbstractFont,openData()} and
@ref TextLayerGL::Shared::addFont(), like in the snippet above. See the
@ref Ui-TextLayer-dpi section below for additional considerations.

Finally, a style referencing the fonts has to be set with
@ref TextLayer::Shared::setStyle(). At the very least you're expected to pass a
@ref TextLayerCommonStyleUniform containing properties common to all styles, an
array of @ref TextLayerStyleUniform matching the style count in the
@ref TextLayer::Shared::Configuration, and a @ref FontHandle and
@ref Text::Alignment alignment that given style should use. Default-constructed
instances will result in white text, you can then use method chaining to update
only the properties you're interested in; zero-initialized @ref Text::Alignment
value is equivalent to @ref Text::Alignment::LineLeft. In the following
snippet, style @cpp 0 @ce uses the first, larger font and style defaults, style
@cpp 1 @ce is the first font again but with a blue color, and style @cpp 2 @ce
is the smaller font centered:

@snippet Ui.cpp TextLayer-setup-style

With this, assuming @ref AbstractUserInterface::draw() is called in an
appropriate place, the layer is ready to use. All style options are described
in detail further below.

@m_class{m-note m-info}

@par Using an enum for style indexing
    Like with @ref BaseLayer, it's possible to use an @cpp enum @ce for
    populating @ref TextLayer styles and referencing them when creating data.
    See the corresponding @ref Ui-BaseLayer-style-enums "BaseLayer documentation chapter"
    for a detailed example.

@section Ui-TextLayer-create Creating texts

A text is created by calling @ref create() with desired style index, the actual
UTF-8 string to render, a @ref TextProperties instance with optional
data-specific shaping and layout settings, and a @ref NodeHandle the data
should be attached to. In this case it picks the style @cpp 1 @ce from above,
which makes the text blue:

@snippet Ui.cpp TextLayer-create

As with all other data, they're implicitly tied to lifetime of the node they're
attached to. You can remember the @ref DataHandle returned by @ref create() to
modify the data later, @ref attach() to a different node or @ref remove() it.

@section Ui-TextLayer-style Style, shaping and layout options

@subsection Ui-TextLayer-style-color Text color

@image html ui-textlayer-style-color.png width=192px

The color supplied with @ref TextLayerStyleUniform::setColor() as shown above
is additionally multiplied by a data-specific color set with @ref setColor().
The main use case, like with @ref BaseLayer, is to allow for example custom
label coloring where it would be impractical to dynamically update style data
based on what's being shown. The color is further multiplied by a per-node
opacity coming from @ref AbstractUserInterface::setNodeOpacity(), which can be
used for various fade-in / fade-out effects.

@snippet Ui.cpp TextLayer-style-color

At the moment, there's no possibility to assign different color to individual
glyphs or text ranges.

@subsection Ui-TextLayer-style-alignment-padding Alignment and padding inside the node

@ref Text::Alignment::MiddleCenter passed to @ref TextLayer::Shared::setStyle()
aligns vertical and horizontal glyph bounds, defined by font ascent, descent,
line advance and glyph advance, to vertical and horizontal center of given
node. @ref Text::Alignment::TopLeft and other alignment values then align a
particular edge or baseline of the text to node edges and corners. With those,
it's often desirable to specify also a padding inside the node with the last
argument to @ref TextLayer::Shared::setStyle(), so e.g. a text inside a text
box has some spacing around without having to put it inside a slightly smaller
child node. Similarly as with @ref Ui-BaseLayer-style-padding "padding in BaseLayer",
it's one value for each of four node edges, but often a single value for all is
enough:

@snippet Ui.cpp TextLayer-style-alignment-padding

If needed, style-provided alignment can be overriden for particular data with
@ref TextProperties::setAlignment() --- or by passing a @ref Text::Alignment
directly, as it's implicitly convertible to @ref TextProperties --- when
creating or updating the text. Note that, unlike e.g.
@ref LineLayer::setAlignment(), the alignment cannot be changed dynamically for
an already shaped text. The reason is that the alignment may offset each line
differently and is dependent on shaping direction of the input text, but
neither the input text nor line break positions or other properties are
remembered by default.

@image html ui-textlayer-style-data-padding.png width=96px

Finally, the padding can be overriden on a per-data basis with @ref setPadding().
This is especially useful when aligning text of a variable width next to other
UI elements such as icons, or combining two differently styled pieces of text
together. The following snippet shows aligning a text next to a colored hash
character using @ref size() to query the actual rendered size, the other
element could also be anything from a @ref BaseLayer or a @ref LineLayer which
provide the same padding interfaces. Additionally, the space between the two is
defined by the style itself and thus doesn't have to be hardcoded in the offset
calculation:

@snippet Ui.cpp TextLayer-style-data-padding

@subsection Ui-TextLayer-style-shaping Font, shaping language, script and direction

Besides alignment, @ref TextProperties::setFont() --- or, again, passing a
@ref FontHandle directly, as it's implicitly convertible to @ref TextProperties
--- allows you to override the font for a particular data. This is useful for
example when a certain text is using a script not supported by the font coming
from the style:

@snippet Ui.cpp TextLayer-style-shaping-font

It's also possible to explicitly specify the script, language and direction of
given text, however note that support for these options is highly dependent on
the font plugin used. @relativeref{Text,HarfBuzzFont}, for example, attempts to
detect these properties by default, so not setting them to any fixed value will
likely provide better results when working with text coming from unknown
international sources. On the other hand, specifying these properties if you're
sure about the text origin avoids the autodetection, which can improve
performance with highly dynamic text. In contrast,
@relativeref{Text,StbTrueTypeFont} for example doesn't support any of these
properties and setting them will have no effect.

@snippet Ui.cpp TextLayer-style-shaping-script-language-direction

At the moment, only @ref Text::LayoutDirection::HorizontalTopToBottom is
supported, i.e. vertical text isn't possible yet. See also
@ref Text::Alignment::LineBegin, @ref Text::Alignment::LineEnd and other
alignment values that resolve to either left or right alignment based on
either the detected or supplied @ref Text::ShapeDirection.

@subsection Ui-TextLayer-style-features Font feature selection

@image html ui-textlayer-style-features.png width=256px

@ref TextProperties::setFeatures() allows you to configure OpenType typographic
features applied when shaping given text. Again, support for these depends on
the font plugin used as well as features exposed by a particular font file. At
the moment, @relativeref{Text,HarfBuzzFont} is the only plugin implementing
these, and most of the features are limited to OTF fonts, TTF supports only a
small subset such as toggling @ref Text::Feature::Kerning. In the following
snippet, the whole text gets @ref Text::Feature::OldstyleFigures enabled,
causing certain numbers to reach below the baseline, then an alternative glyph
of the *a* character in *Status* is picked, and @ref Text::Feature::SmallCapitals
is used for the part of the string containing the actual status description:

@snippet Ui.cpp TextLayer-style-features-data

Note that the features will likely need additional glyphs to be pre-filled in
the cache. The @ref Text::AbstractFont::fillGlyphCache(AbstractGlyphCache&, const Containers::StridedArrayView1D<const UnsignedInt>&)
overload allows you to specify individual glyph IDs, and
@ref Text::AbstractFont::glyphForName() allows you to query the IDs by names
that you can look up in some font inspection utility. In case of the
[Source Sans](https://github.com/adobe-fonts/source-sans) font used here, it
could look like this:

@snippet Ui.cpp TextLayer-style-features-fill-glyph-cache

Font features can be also specified as part of the style. For example you may
want to have all title labels in small caps, and all numeric fields with
@ref Text::Feature::TabularFigures instead of proportional and with
@ref Text::Feature::SlashedZero to distinguish a zero from an uppercase O. All
used features are supplied as a list in the fifth argument to
@ref TextLayer::Shared::setStyle(), and then each style specifies an offset and
count into this list in the next two arguments. The same features can be used
by multiple ranges, count being @cpp 0 @ce means no features are used for given
style:

@snippet Ui.cpp TextLayer-style-features

@section Ui-TextLayer-dpi Text crispness and DPI awareness

With @ref Text::GlyphCacheArrayGL, if the UI size differs from the actual
framebuffer size, which is often the case on HiDPI systems and other scenarios
described in @ref Ui-AbstractUserInterface-dpi "AbstractUserInterface DPI awareness docs",
using the same font size in both @ref Text::AbstractFont::openFile() /
@relativeref{Text::AbstractFont,openData()} and
@ref TextLayerGL::Shared::addFont() will result in blurry text. Additionally,
because glyphs are often placed at subpixel locations, it's often better to
supersample the font at twice the size it's used in for crisper edges even if
no HiDPI interface scaling is involved. Thus, the size at which the font is
opened and at which the glyph cache is filled should be multiplied by a factor
compared to the size the font is used in the UI:

@snippet Ui.cpp TextLayer-dpi

Fonts added with @ref TextLayerGL::Shared::addInstancelessFont() should then
use the `scale` directly.

<b></b>

@m_class{m-note m-warning}

@par
    Keep in mind that since glyphs may get rendered at a bigger size, the glyph
    cache may also need to be created with a bigger size in that case.

@section Ui-TextLayer-distancefield Distance field rendering

With @ref Text::GlyphCacheArrayGL that's used above, drawing a particularly
sized text involves first populating the cache with a font rasterized at a
size matching physical screen pixels. This gives the font plugin an opportunity
to align glyph outlines in a way that best suits given size. While it's
possible to pass the same @ref Text::AbstractFont instance to
@ref TextLayer::Shared::addFont() multiple times with different sizes, the
result will likely be either blurry or jaggy.

Having a dedicated font for every size becomes impractical if the application
needs to use many font sizes, if it needs to scale the text dynamically or if
advanced effects such as thinning, thickening or outline are needed. For such
cases, it's possible to use @ref Text::DistanceFieldGlyphCacheArrayGL instead.
Compared to the regular glyph cache it requires three input parameters, size of
the source image, size of the resulting glyph cache image and a radius for the
distance field creation. Their relation and effect on output quality and memory
use is described in detail in @ref TextureTools-DistanceFieldGL-parameters "TextureTools::DistanceFieldGL docs".
In short, the ratio between the input and output image size is usually four or
eight times, and the size of the font should match the larger size. So, for
example, if a @cpp {256, 256} @ce @ref Text::GlyphCacheGL would be filled with
a 16 pt font, a @cpp {1024, 1024} @ce source image for the distance field
should use a 64 pt font. The radius should then be chosen so it's at least one
or two pixels in the scaled-down result, so in this case at least @cpp 4 @ce:

@snippet Ui-gl.cpp TextLayer-distancefield-setup

The glyph cache is expected to be alive for the whole shared instance lifetime,
or you can again use the
@ref TextLayerGL::Shared::Shared(Text::DistanceFieldGlyphCacheArrayGL&&, const Configuration&)
constructor to move the glyph cache ownership to the shared instance.
Afterwards, you can add the same font instance more than once with different
sizes. The sizes no longer need to match the size at which the font was opened
and rasterized, but rather should be a concrete size at which it's meant to be
drawn in the UI:

@snippet Ui.cpp TextLayer-distancefield-setup-fonts

@subsection Ui-TextLayer-distancefield-smoothness Edge smoothness

@image html ui-textlayer-style-smoothness.png width=256px

With @ref Text::GlyphCacheArrayGL, the rendered text has edges antialiased
already by the rasterizer of a particular font plugin. With distance field
rendering you have full control over how the edges look. Similarly to
@ref BaseLayer and @ref LineLayer, the edges are aliased by default and with
@ref TextLayerCommonStyleUniform::setSmoothness() you set up a smoothness
radius in framebuffer pixels. It's not in UI units in order to ensure crisp
look everywhere without having to specifically deal with HiDPI displays.
Additionally, for effects like shadow or glow,
@ref TextLayerStyleUniform::setSmoothness() can override the smoothness per
style, and this value is in UI units instead of pixels. The desired usage is
that the common smoothness gets tuned for
@ref Ui-BaseLayer-style-rounded-corners-smoothness "BaseLayer",
@ref Ui-LineLayer-style-smoothness "LineLayer" and any distance-field-enabled
text layer to a value that makes sense for the UI in general, and the per-style
smoothness is then used for text elements that need a fuzzy appearance. Of the
two values coming from the style, the larger one --- when both are converted to
the same units --- gets used.

@snippet Ui.cpp TextLayer-distancefield-smoothness

Note that the smoothness is *not* depending on the size at which the text is
actually drawn, it's always in pixels or UI units.

@subsection Ui-TextLayer-distancefield-offset-outline Edge offset and outline

@image html ui-textlayer-style-offset-outline.png width=256px

@ref TextLayerStyleUniform::setOutlineWidth() and
@relativeref{TextLayerStyleUniform,setOutlineColor()} set properties of the
outline. For consistentcy with vector drawing elsewhere, the outline is
centered on the edge, so e.g. a two-pixel outline would extend one pixel inside
the glyph and one pixel outside. With @ref TextLayerStyleUniform::setEdgeOffset()
you can adjust the edge position to make the glyphs thicker or thinner, and a
combination of these two allows you to adjust whether the outline will grow
from the center, outside or inside:

@snippet Ui.cpp TextLayer-distancefield-offset-outline

The range how far an outline together with edge offset can go is clamped to the
radius used when constructing the @ref Text::DistanceFieldGlyphCacheArrayGL.
From the snippet above, a radius of @cpp 20 @ce, together with the font being
scaled down from @cpp 64.0f @ce, means the edge can extend at most five pixels
for `font16Handle` or two and half for `font8Handle`. If you don't need that
much, you can reduce the radius, which also means the glyphs will occupy less
space and the distance field processing takes less time.

Similarly as with the smoothness, the outline width and offset doesn't depend
on size of the font actually used in given style. Consistently with
@ref Ui-BaseLayer-style-outline "BaseLayer outline drawing" the outline color
isn't affected by @ref setColor().

@section Ui-TextLayer-single-glyphs Rendering single glyphs

@image html ui-textlayer-single-glyph.png width=32px

With @ref createGlyph() you can render a single glyph specified with its ID.
This is useful mainly with icon fonts, where you can for example use
@ref Text::AbstractFont::glyphForName() at runtime if the font has named
glyphs:

@snippet Ui.cpp TextLayer-single-glyph-runtime

Or you can maintain an @cpp enum @ce with name-to-glyph-ID mapping. Similarly
to style IDs, @ref createGlyph() accepts both plain integers and enums for a
glyph ID:

@snippet Ui.cpp TextLayer-single-glyph-enum

It's however also possible to insert your own image data into the glyph cache
and reference them with this function. Assuming a list of images imported for
example from a set of PNGs, call @ref Text::AbstractGlyphCache::addFont() to
add a new font ID, copy the image contents to places in the cache reserved
using @ref TextureTools::AtlasLandfill::add(), make the glyph cache aware of
the new icons with @ref Text::AbstractGlyphCache::addGlyph(), and at the end
upload the updated part of the glyph cache to the GPU texture using
@ref Text::AbstractGlyphCache::flushImage().

@snippet Ui.cpp TextLayer-single-glyph-instanceless1

Finally create a @ref FontHandle for the new set of glyphs using
@ref TextLayer::Shared::addInstancelessFont(). Compared to
@ref TextLayer::Shared::addFont() it takes a *scale* instead of size. Assuming
the UI size matches the framebuffer size, use @cpp 1.0f @ce if you want to draw
the icons in their original pixel size. the @ref Ui-TextLayer-dpi section below
for additional considerations. The font handle can then be referenced from the
style or passed directly to @ref createGlyph().

@snippet Ui.cpp TextLayer-single-glyph-instanceless2

See @ref Text::AbstractGlyphCache and @ref TextureTools::AtlasLandfill for
detailed information about how the cache and atlas packing works internally. If
you use @ref Text::DistanceFieldGlyphCacheArrayGL, you don't need to do
anything extra as the distance field conversion will be performed by the
@relativeref{Text::AbstractGlyphCache,flushImage()} call on the fly.

@section Ui-TextLayer-update Updating text data

Any created text or single glyph can be subsequently updated with
@ref setText() and @ref setGlyph(). These functions take the same arguments and
behave the same as @ref create() and @ref createGlyph(). Internally there's no
distinction between a text and a single glyph, so a text can be safely changed
to just a glyph and vice versa.

@section Ui-TextLayer-transformation Arbitrary text and glyph transformation

By constructing the layer with @ref TextLayerFlag::Transformable, the text data
can be arbitrarily translated, rotated and scaled, replacing the padding
functionality. While this feature works with the regular glyph cache as well,
it's mainly intended for use with @ref Ui-TextLayer-distancefield "distance field rendering"
which ensures the transformation doesn't cause any visual artifacts.

@snippet Ui-gl.cpp TextLayer-transformation-setup

For a more involved example, the following code will draw an analog clock with
hands drawn as rotated glyphs. First, the hand is imported from an inline SVG
with one of the SVG importer plugins such as the
@relativeref{Trade,LunaSvgImporter}. It'll result in a 32x256 image that's ---
as documented --- in @ref PixelFormat::RGBA8Unorm, but is just black and white:

@snippet Ui.cpp TextLayer-transformation-clock1

Afterwards, similarly to the @ref Ui-TextLayer-single-glyphs "glyph cache packing code above",
the image is added to the glyph cache. Just the red channel is copied because
the distance field glyph cache is single-channel, and the
@ref Text::AbstractGlyphCache::flushImage() call will process it to a distance
field. Then a @ref Ui::FontHandle with this single image as glyph @cpp 0 @ce is
added, which gets referenced later. The @cpp {-16, 16} @ce offset moves the
glyph origin to the desired rotation center. The scale passed to
@ref TextLayer::Shared::addInstancelessFont() is chosen so the 256-pixel-high
input image has the right size for the desired size of the clock in the UI.

@snippet Ui.cpp TextLayer-transformation-clock2

@image html ui-textlayer-transformation.png width=128px

Finally, assuming `style` has a regular font associated, 12-, 3-, 6- and 9-hour
marks are added using various @ref Text::Alignment values. Two other nodes,
added as children in order to make the hands appear above the numbers, with the
`needleFont` glyph @cpp 0 @ce from above, then show a time of 10:11. The
default hand rotation is at the 12-hour mark, first child is the scaled-down
hand showing hours, and second, again above hours, is then minutes:

@snippet Ui.cpp TextLayer-transformation-clock3

@section Ui-TextLayer-dynamic-styles Dynamic styles

Like with @ref Ui-BaseLayer-dynamic-styles "BaseLayer dynamic styles", the
@ref TextLayer::Shared::setStyle() API is meant to be used to supply data for
static styles, and dynamic styles, requested with
@ref TextLayer::Shared::Configuration::setDynamicStyleCount() and specified via
@ref setDynamicStyle(), used for style data that change very often:

@snippet Ui-gl.cpp TextLayer-dynamic-styles

The main use case for dynamic styles is animations, which is for the text layer
implemented in the @ref TextLayerStyleAnimator class. Again note that if you
intend to directly use dynamic styles along with the animator, you should use
@ref allocateDynamicStyle() and @ref recycleDynamicStyle() to prevent the
animator from stealing dynamic styles you use elsewhere:

@snippet Ui.cpp TextLayer-dynamic-styles-allocate

@section Ui-TextLayer-style-transitions Style transition based on input events

Like with @ref BaseLayer, it's possible to configure @ref TextLayer to perform
automatic style transitions based on input events, such as highlighting on
hover or press. See the @ref Ui-BaseLayer-style-transitions "BaseLayer documentation for style transition"
for a detailed example, the interfaces are the same between the two.

@m_class{m-note m-info}

@par
    Note that at the moment, the whole node area reacts to the event, not just
    when the text glyphs themselves are under the pointer.

@section Ui-TextLayer-editing Editable text

By default, the text layer shapes the input text into glyphs, throwing away the
input text and all its properties like language or direction, and remembering
just which glyphs to render at which positions. As majority of text in user
interfaces is non-editable and often only set, never queried back, there's no
need to implicitly store such info.

To make editable text, first a style describing how cursor and selection looks
like needs to be configured. As both the cursor and the selection are
rectangular shapes, there's a single *editing style* definition, and then it's
specified which of them are used for cursors and which for selection. By
default there are no editing styles and their desired count has to be specified
via @ref TextLayer::Shared::Configuration::setEditingStyleCount(), in this
example we'll have just two, used by three regular styles:

@snippet Ui-gl.cpp TextLayer-editing-style-shared

Style data is then supplied with @ref TextLayer::Shared::setEditingStyle().
Similarly to regular styles, there can be multiple editing styles, and they
consist of a @ref TextLayerCommonEditingStyleUniform, containing properties
common to all editing styles, an array of @ref TextLayerEditingStyleUniform,
matching the style count specified in the configuration. Default-constructed
instances will result in white rectangles, you can then use method chaining to
update only the properties you're interested in. Finally, there is a list of
padding values, which define how far from the actual glyph bounds the selection
or cursor expands. In case of cursors in particular, with no padding specified
they'd have a zero width and thus be invisible. The following snippet defines
an azure style @cpp 0 @ce for a cursor, expanding one UI unit on each side from
the cursor position, and a blue selection style @cpp 1 @ce, tightly wrapping
the glyph rectangle:

@snippet Ui.cpp TextLayer-editing-style-editing

After that, the editing styles need to be referenced from the regular styles.
This is done with the eighth and ninth argument to
@ref TextLayer::Shared::setStyle(), which are are cursor and selection style
indices, respectively, or @cpp -1 @ce if given style shouldn't have a cursor /
selection style assigned. If either of those arguments is left empty, it's the
same as if it'd have @cpp -1 @ce for all styles. Here style @cpp 0 @ce is a
regular non-editing style, @cpp 1 @ce is a style with both a cursor and
selection style, and @cpp 2 @ce has just a selection, for example if a certain
label is meant to be just selectable but not directly editable:

@snippet Ui.cpp TextLayer-editing-style

With the editing style set up, an editable text can be made by passing
@ref TextDataFlag::Editable to either @ref create() or @ref setText()
Initially it will have no selection and the cursor at the end, you can then use
@ref setCursor() to specify the byte position where the cursor and selection
should be:

@snippet Ui.cpp TextLayer-editing-create

Contents of an editable text are then subsequently queryable with @ref text().
The @ref updateText() function can perform incremental updates in addition to
cursor movement and the @ref editText() API is for higher-level operations with
UTF-8 and text directionality awareness. This function is also what is called
on actual key and text input events, described in
@ref Ui-TextLayer-editing-events below.

@subsection Ui-TextLayer-editing-color Cursor and selection color, selection text style

@image html ui-textlayer-editing-color.png width=128px

Apart from the selection / cursor rectangle color, set with
@ref TextLayerEditingStyleUniform::setBackgroundColor(), it's possible to
override style of the text under selection using the third argument to
@ref TextLayer::Shared::setEditingStyle(). Compared to above, the following
snippet adds a style number @cpp 3 @ce, which is then referenced from the
selection editing style, and which makes the selected text darker:

@snippet Ui.cpp TextLayer-editing-style-text-selection

Note that only the @ref TextLayerCommonStyleUniform is used from the style
override, not font, alignment or other properties.

@subsection Ui-TextLayer-editing-padding Cursor and selection rectangle padding

@image html ui-textlayer-editing-padding.png width=128px

Padding used in the snippets above was only horizontal, but vertical padding
can be used for example to achieve better optical alignment of both the text
itself and the selection rectangle inside an edit box, or to make a cursor
stand out more. Horizontal padding also doesn't necessarily have to be
symmetrical. For the cursor it's often desirable to have it shifted towards the
typing direction. Here it's still two units wide, but is shifted to the right
for @ref Text::ShapeDirection::LeftToRight text, and to the left for
@relativeref{Text::ShapeDirection,RightToLeft} text:

@snippet Ui.cpp TextLayer-editing-style-padding

@subsection Ui-TextLayer-editing-rounded-corners-smoothness Rounded corners and edge smoothness

@image html ui-textlayer-editing-rounded.png width=128px

@ref TextLayerEditingStyleUniform::setCornerRadius() allows you to make the
cursor and selection rectangle corners rounded. The edges are aliased by
default, use @ref TextLayerCommonEditingStyleUniform::setSmoothness() to
smoothen them out. The radius is in framebuffer pixels, same as with
@ref Ui-BaseLayer-style-rounded-corners-smoothness "BaseLayer edge smoothness".

@snippet Ui.cpp TextLayer-editing-style-rounded

@subsection Ui-TextLayer-editing-events Reacting to focus, pointer and keyboard input events

Assuming key and text input events are propagated to the UI from the
application as described in the @ref Ui-AbstractUserInterface-application "AbstractUserInterface documentation",
if a text with @ref TextDataFlag::Editable is attached to a
@ref NodeFlag::Focusable node, focusing it either programmatically or with a
pointer tap or click will make it receive all keyboard and text input events,
allowing the user to edit the text. Arrow and other editing keys map to
concrete actions described in the @ref TextEdit enum.

@snippet Ui.cpp TextLayer-editing-focusable

<b></b>

@m_class{m-note m-warning}

@par
    Note that currently only cursor movement and selection via a keyboard is
    implemented, not with a pointer yet.

For proper visual feedback on focus and blur of a particular widget containing
the editable text it's recommended to implement also the `toFocusedOut` and
`toFocusedOver` state transitions in @ref Shared::setStyleTransition() for all
visual layers used in given widget, in addition to the basic inactive, pressed
and disabled transitions shown in the
@ref Ui-BaseLayer-style-transitions "BaseLayer documentation for style transitions".

@subsection Ui-TextLayer-editing-virtual-keyboards Implementing virtual keyboards

Besides key and text input events coming from the OS, it's possible to
synthesize them to implement a virtual keyboard, for example by hooking to
pointer presses using @ref EventLayer::onPress(), as shown below. See the
@ref EventLayer documentation for information about setting up the event layer.

An important thing to consider is that, by default, pressing outside of a node
that's currently focused will blur it, which ultimately means that when using
the virtual keyboard, you'd lose focus of the input field after each key press.
This behavior can be disabled for a node and all its children with
@ref NodeFlag::NoBlur, and is recommended to be done for the whole virtual
keyboard, not just individual keys, so even accidental presses in between the
keys don't lose focus.

@snippet Ui.cpp TextLayer-editing-virtual-keyboard

Generally you'd synthesize a @ref TextInputEvent for any key that inserts text,
and @ref KeyEvent for keys that perform cursor movement or deletion. Uppercase
can be implemented for example by having the virtual
@m_class{m-label m-default} **Shift** key toggle a boolean, which the
@ref EventLayer::onPress() handlers then use to synthesize either @cpp "q" @ce
or @cpp "Q" @ce. Or pressing the @m_class{m-label m-default} **Shift** key
toggles visibility of an entirely different key set for uppercase letters,
completely with different labels for those.

@subsection Ui-TextLayer-editing-dynamic-styles Dynamic editing styles

Dynamic styles, @ref Ui-TextLayer-dynamic-styles "described above", have also
variants that include cursor and selection styles. Depending on the subset of
editing styles you want to have for a particular dynamic style, use
@ref setDynamicStyleWithCursor(), @ref setDynamicStyleWithSelection() or
@ref setDynamicStyleWithCursorSelection() instead of @ref setDynamicStyle().

@section Ui-TextLayer-debug-integration Debug layer integration

When using @ref Ui-DebugLayer-node-highlight "DebugLayer node highlighting",
this layer inherits @ref Ui-AbstractVisualLayer-debug-integration "debug integration from the AbstractVisualLayer".
See its documentation for more information.
*/
class MAGNUM_UI_EXPORT TextLayer: public AbstractVisualLayer {
    public:
        class Shared;

        /**
         * @brief Shared state used by this layer
         *
         * Reference to the instance passed to @ref TextLayerGL::TextLayerGL(LayerHandle, Shared&, TextLayerFlags).
         */
        inline Shared& shared();
        inline const Shared& shared() const; /**< @overload */

        /**
         * @brief Layer flags
         *
         * @see @ref TextLayer::Shared::flags(), @ref flags(DataHandle) const
         */
        TextLayerFlags flags() const;

        /**
         * @brief Assign a style animator to this layer
         * @return Reference to self (for method chaining)
         *
         * Expects that @ref Shared::dynamicStyleCount() is non-zero and that
         * given @p animator wasn't passed to @ref assignAnimator() on any
         * layer yet. On the other hand, it's possible to associate multiple
         * different animators with the same layer.
         * @see @ref setDefaultStyleAnimator()
         */
        TextLayer& assignAnimator(TextLayerStyleAnimator& animator);

        /**
         * @brief Default style animator for this layer
         *
         * If a style animator hasn't been set, returns @cpp nullptr @ce. If
         * not @cpp nullptr @ce, the returned animator is guaranteed to be
         * assigned to this layer, i.e. that
         * @ref BaseLayerStyleAnimator::layer() is equal to @ref handle().
         */
        TextLayerStyleAnimator* defaultStyleAnimator() const;

        /**
         * @brief Set a default style animator for this layer
         * @return Reference to self (for method chaining)
         *
         * Makes @p animator used in style transitions in response to events.
         * Expects that @p animator is either @cpp nullptr @ce or is already
         * assigned to this layer, i.e. that @ref assignAnimator() was called
         * on this layer with @p animator before. Calling this function again
         * with a different animator or with @cpp nullptr @ce replaces the
         * previous one.
         * @see @ref allocateDynamicStyle()
         */
        TextLayer& setDefaultStyleAnimator(TextLayerStyleAnimator* animator);

        /**
         * @brief Dynamic style uniforms
         *
         * Size of the returned view is at least
         * @ref Shared::dynamicStyleCount(). These uniforms are used by style
         * indices greater than or equal to @ref Shared::styleCount(). If
         * @ref Shared::hasEditingStyles() is @cpp true @ce, size of the
         * returned view is *three times* @ref Shared::dynamicStyleCount() and
         * the extra elements are referenced by
         * @ref dynamicStyleSelectionStyleTextUniform(UnsignedInt) const.
         * @see @ref dynamicStyleFonts(), @ref dynamicStyleAlignments(),
         *      @ref dynamicStyleFeatures(), @ref dynamicStyleCursorStyles(),
         *      @ref dynamicStyleCursorStyle(),
         *      @ref dynamicStyleSelectionStyles(),
         *      @ref dynamicStyleSelectionStyle(),
         *      @ref dynamicStylePaddings(),
         *      @ref dynamicEditingStyleUniforms(),
         *      @ref dynamicEditingStylePaddings(), @ref setDynamicStyle(),
         *      @ref setDynamicStyleWithCursorSelection(),
         *      @ref setDynamicStyleWithCursor(),
         *      @ref setDynamicStyleWithSelection()
         */
        Containers::ArrayView<const TextLayerStyleUniform> dynamicStyleUniforms() const;

        /**
         * @brief Dynamic style fonts
         *
         * Size of the returned view is @ref Shared::dynamicStyleCount(). These
         * fonts are used by style indices greater than or equal to
         * @ref Shared::styleCount().
         * @see @ref dynamicStyleUniforms(), @ref dynamicStyleAlignments(),
         *      @ref dynamicStyleFeatures(), @ref dynamicStyleCursorStyles(),
         *      @ref dynamicStyleCursorStyle(),
         *      @ref dynamicStyleSelectionStyles(),
         *      @ref dynamicStyleSelectionStyle(),
         *      @ref dynamicStyleSelectionStyleTextUniform(),
         *      @ref dynamicStylePaddings(),
         *      @ref dynamicEditingStyleUniforms(),
         *      @ref dynamicEditingStylePaddings(), @ref setDynamicStyle(),
         *      @ref setDynamicStyleWithCursorSelection(),
         *      @ref setDynamicStyleWithCursor(),
         *      @ref setDynamicStyleWithSelection()
         */
        Containers::StridedArrayView1D<const FontHandle> dynamicStyleFonts() const;

        /**
         * @brief Dynamic style alignments
         *
         * Size of the returned view is @ref Shared::dynamicStyleCount(). These
         * alignments are used by style indices greater than or equal to
         * @ref Shared::styleCount().
         * @see @ref dynamicStyleUniforms(), @ref dynamicStyleFonts(),
         *      @ref dynamicStyleFeatures(), @ref dynamicStyleCursorStyles(),
         *      @ref dynamicStyleCursorStyle(),
         *      @ref dynamicStyleSelectionStyles(),
         *      @ref dynamicStyleSelectionStyle(),
         *      @ref dynamicStyleSelectionStyleTextUniform(),
         *      @ref dynamicStylePaddings(),
         *      @ref dynamicEditingStyleUniforms(),
         *      @ref dynamicEditingStylePaddings(), @ref setDynamicStyle(),
         *      @ref setDynamicStyleWithCursorSelection(),
         *      @ref setDynamicStyleWithCursor(),
         *      @ref setDynamicStyleWithSelection()
         */
        Containers::StridedArrayView1D<const Text::Alignment> dynamicStyleAlignments() const;

        /**
         * @brief Dynamic style font features
         *
         * Expects that the @p id is less than @ref Shared::dynamicStyleCount().
         * @see @ref dynamicStyleUniforms(), @ref dynamicStyleFonts(),
         *      @ref dynamicStyleAlignments(), @ref dynamicStyleCursorStyles(),
         *      @ref dynamicStyleCursorStyle(),
         *      @ref dynamicStyleSelectionStyles(),
         *      @ref dynamicStyleSelectionStyle(),
         *      @ref dynamicStyleSelectionStyleTextUniform(),
         *      @ref dynamicStylePaddings(),
         *      @ref dynamicEditingStyleUniforms(),
         *      @ref dynamicEditingStylePaddings(), @ref setDynamicStyle(),
         *      @ref setDynamicStyleWithCursorSelection(),
         *      @ref setDynamicStyleWithCursor(),
         *      @ref setDynamicStyleWithSelection()
         */
        Containers::ArrayView<const TextFeatureValue> dynamicStyleFeatures(UnsignedInt id) const;

        /**
         * @brief Which dynamic style have associated cursor styles
         *
         * Size of the returned view is @ref Shared::dynamicStyleCount(). Use
         * @ref dynamicStyleCursorStyle(UnsignedInt) const to retrieve ID of
         * the corresponding editing style.
         * @see @ref dynamicStyleUniforms(), @ref dynamicStyleFonts(),
         *      @ref dynamicStyleAlignments(), @ref dynamicStyleFeatures(),
         *      @ref dynamicStyleSelectionStyles(),
         *      @ref dynamicStyleSelectionStyle(),
         *      @ref dynamicStyleSelectionStyleTextUniform(),
         *      @ref dynamicStylePaddings(),
         *      @ref dynamicEditingStyleUniforms(),
         *      @ref dynamicEditingStylePaddings(),
         *      @ref setDynamicStyleWithCursorSelection(),
         *      @ref setDynamicStyleWithCursor()
         */
        Containers::BitArrayView dynamicStyleCursorStyles() const;

        /**
         * @brief Dynamic style cursor style ID
         *
         * Expects that the @p id is less than @ref Shared::dynamicStyleCount().
         * If given dynamic style has no associated cursor style, returns
         * @cpp -1 @ce, otherwise the returned index is a constant calculated
         * from @p id and points to the @ref dynamicEditingStyleUniforms() and
         * @ref dynamicEditingStylePaddings() arrays.
         * @see @ref dynamicStyleUniforms(), @ref dynamicStyleFonts(),
         *      @ref dynamicStyleAlignments(), @ref dynamicStyleFeatures(),
         *      @ref dynamicStyleCursorStyles(),
         *      @ref dynamicStyleSelectionStyles(),
         *      @ref dynamicStyleSelectionStyle(),
         *      @ref dynamicStyleSelectionStyleTextUniform(),
         *      @ref dynamicStylePaddings(),
         *      @ref dynamicEditingStyleUniforms(),
         *      @ref dynamicEditingStylePaddings(),
         *      @ref setDynamicStyleWithCursorSelection(),
         *      @ref setDynamicStyleWithCursor()
         */
        Int dynamicStyleCursorStyle(UnsignedInt id) const;

        /**
         * @brief Which dynamic style have associated selection styles
         *
         * Size of the returned view is @ref Shared::dynamicStyleCount(). Use
         * @ref dynamicStyleSelectionStyle(UnsignedInt) const to retrieve ID of
         * the corresponding editing style.
         * @see @ref dynamicStyleUniforms(), @ref dynamicStyleFonts(),
         *      @ref dynamicStyleAlignments(), @ref dynamicStyleFeatures(),
         *      @ref dynamicStyleCursorStyles(),
         *      @ref dynamicStyleCursorStyle(),
         *      @ref dynamicStyleSelectionStyleTextUniform(),
         *      @ref dynamicStylePaddings(),
         *      @ref dynamicEditingStyleUniforms(),
         *      @ref dynamicEditingStylePaddings(),
         *      @ref setDynamicStyleWithCursorSelection(),
         *      @ref setDynamicStyleWithSelection()
         */
        Containers::BitArrayView dynamicStyleSelectionStyles() const;

        /**
         * @brief Dynamic style selection style ID
         *
         * Expects that the @p id is less than @ref Shared::dynamicStyleCount().
         * If given dynamic style has no associated selection style, returns
         * @cpp -1 @ce, otherwise the returned index is a constant calculated
         * from @p id and points to the @ref dynamicEditingStyleUniforms() and
         * @ref dynamicEditingStylePaddings() arrays. Use
         * @ref dynamicStyleSelectionStyleTextUniform(UnsignedInt) const to
         * retrieve ID of the uniform override applied to selected text.
         * @see @ref dynamicStyleUniforms(), @ref dynamicStyleFonts(),
         *      @ref dynamicStyleAlignments(), @ref dynamicStyleFeatures(),
         *      @ref dynamicStyleCursorStyles(),
         *      @ref dynamicStyleSelectionStyles(),
         *      @ref dynamicStyleSelectionStyleTextUniform(),
         *      @ref dynamicStylePaddings(),
         *      @ref dynamicEditingStyleUniforms(),
         *      @ref dynamicEditingStylePaddings(),
         *      @ref setDynamicStyleWithCursorSelection(),
         *      @ref setDynamicStyleWithSelection()
         */
        Int dynamicStyleSelectionStyle(UnsignedInt id) const;

        /**
         * @brief Dynamic style selection style text uniform IDs
         *
         * Expects that the @p id is less than @ref Shared::dynamicStyleCount().
         * If given dynamic style has no associated selection style, returns
         * @cpp -1 @ce, otherwise the returned index is a constant calculated
         * based on @p id and points to the @ref dynamicStyleUniforms() array.
         *
         * In particular, contrary to the styles specified with
         * @ref Shared::setEditingStyle(), even if a text uniform wasn't passed
         * in @ref setDynamicStyleWithCursorSelection() or
         * @ref setDynamicStyleWithSelection(), the ID still points to a valid
         * uniform index -- the uniform data are instead copied over from the
         * base style.
         * @see @ref dynamicStyleUniforms(), @ref dynamicStyleFonts(),
         *      @ref dynamicStyleAlignments(), @ref dynamicStyleFeatures(),
         *      @ref dynamicStyleCursorStyles(),
         *      @ref dynamicStyleCursorStyle(),
         *      @ref dynamicStyleSelectionStyles(),
         *      @ref dynamicStyleSelectionStyle(), @ref dynamicStylePaddings(),
         *      @ref dynamicEditingStyleUniforms(),
         *      @ref dynamicEditingStylePaddings(),
         *      @ref setDynamicStyleWithCursorSelection(),
         *      @ref setDynamicStyleWithSelection()
         */
        Int dynamicStyleSelectionStyleTextUniform(UnsignedInt) const;

        /**
         * @brief Dynamic style paddings
         *
         * Size of the returned view is @ref Shared::dynamicStyleCount(). These
         * paddings are used by style indices greater than or equal to
         * @ref Shared::styleCount().
         * @see @ref dynamicStyleUniforms(), @ref dynamicStyleFonts(),
         *      @ref dynamicStyleAlignments(), @ref dynamicStyleFeatures(),
         *      @ref dynamicStyleCursorStyles(),
         *      @ref dynamicStyleCursorStyle(),
         *      @ref dynamicStyleSelectionStyles(),
         *      @ref dynamicStyleSelectionStyle(),
         *      @ref dynamicStyleSelectionStyleTextUniform(),
         *      @ref dynamicEditingStyleUniforms(),
         *      @ref dynamicEditingStylePaddings(), @ref setDynamicStyle(),
         *      @ref setDynamicStyleWithCursorSelection(),
         *      @ref setDynamicStyleWithCursor(),
         *      @ref setDynamicStyleWithSelection()
         */
        Containers::StridedArrayView1D<const Vector4> dynamicStylePaddings() const;

        /**
         * @brief Dynamic editing style uniforms
         *
         * Size of the returned view is twice @ref Shared::dynamicStyleCount().
         * Used by dynamic styles that have associated cursor or selection
         * styles, pointing here with @ref dynamicStyleCursorStyle() and
         * @ref dynamicStyleSelectionStyle().
         * @see @ref dynamicStyleUniforms(), @ref dynamicStyleFonts(),
         *      @ref dynamicStyleAlignments(), @ref dynamicStyleFeatures(),
         *      @ref dynamicStyleCursorStyles(),
         *      @ref dynamicStyleSelectionStyles(),
         *      @ref dynamicStyleSelectionStyleTextUniform(),
         *      @ref dynamicEditingStylePaddings(),
         *      @ref setDynamicStyleWithCursorSelection(),
         *      @ref setDynamicStyleWithCursor(),
         *      @ref setDynamicStyleWithSelection()
         */
        Containers::ArrayView<const TextLayerEditingStyleUniform> dynamicEditingStyleUniforms() const;

        /**
         * @brief Dynamic editing style paddings
         *
         * Size of the returned view is twice @ref Shared::dynamicStyleCount().
         * Used by dynamic styles that have associated cursor or selection
         * styles, pointing here with @ref dynamicStyleCursorStyle() and
         * @ref dynamicStyleSelectionStyle().
         * @see @ref dynamicStyleUniforms(), @ref dynamicStyleFonts(),
         *      @ref dynamicStyleAlignments(), @ref dynamicStyleFeatures(),
         *      @ref dynamicStyleCursorStyles(),
         *      @ref dynamicStyleSelectionStyles(),
         *      @ref dynamicStyleSelectionStyleTextUniform(),
         *      @ref dynamicEditingStyleUniforms(),
         *      @ref setDynamicStyleWithCursorSelection(),
         *      @ref setDynamicStyleWithCursor(),
         *      @ref setDynamicStyleWithSelection()
         */
        Containers::StridedArrayView1D<const Vector4> dynamicEditingStylePaddings() const;

        /**
         * @brief Set a dynamic style for text only
         * @param id                Dynamic style ID
         * @param uniform           Style uniform
         * @param font              Font handle
         * @param alignment         Alignment
         * @param features          Font features to use
         * @param padding           Padding inside the node in order left, top,
         *      right, bottom
         *
         * Expects that the @p id is less than @ref Shared::dynamicStyleCount(),
         * @p font is either @ref FontHandle::Null or valid and @p alignment is
         * not `*GlyphBounds` as the implementation can only align based on
         * font metrics and cursor position, not actual glyph bounds.
         * @ref Shared::styleCount() plus @p id is then a style index that can
         * be passed to @ref create(), @ref createGlyph() or @ref setStyle() in
         * order to use this style. Compared to @ref Shared::setStyle() the
         * mapping between dynamic styles and uniforms is implicit. All dynamic
         * styles are initially default-constructed @ref TextLayerStyleUniform
         * instances, @ref FontHandle::Null, @ref Text::Alignment::MiddleCenter
         * with empty feature lists, zero padding vectors and no associated
         * cursor or selection styles.
         *
         * Calling this function causes @ref LayerState::NeedsCommonDataUpdate
         * to be set to trigger an upload of changed dynamic style uniform
         * data. If @p padding changed, @ref LayerState::NeedsDataUpdate gets
         * set as well. On the other hand, changing the @p font, @p alignment
         * or @p features doesn't cause @ref LayerState::NeedsDataUpdate to be
         * set --- the @p font, @p alignment and @p features get only used in
         * the following call to @ref create(), @ref createGlyph(),
         * @ref setText() or @ref setGlyph().
         *
         * This function doesn't set any cursor or selection style, meaning
         * they won't be shown at all if using this style for a
         * @ref TextDataFlag::Editable text. Use
         * @ref setDynamicStyleWithCursorSelection(),
         * @ref setDynamicStyleWithCursor() or
         * @ref setDynamicStyleWithSelection() to specify those as well.
         * @see @ref dynamicStyleUniforms(), @ref dynamicStyleFonts(),
         *      @ref dynamicStyleAlignments(), @ref dynamicStyleFeatures(),
         *      @ref dynamicStylePaddings(), @ref allocateDynamicStyle(),
         *      @ref recycleDynamicStyle()
         */
        void setDynamicStyle(UnsignedInt id, const TextLayerStyleUniform& uniform, FontHandle font, Text::Alignment alignment, Containers::ArrayView<const TextFeatureValue> features, const Vector4& padding);
        /** @overload */
        void setDynamicStyle(UnsignedInt id, const TextLayerStyleUniform& uniform, FontHandle font, Text::Alignment alignment, std::initializer_list<TextFeatureValue> features, const Vector4& padding);

        /**
         * @brief Set a dynamic style for text, cursor and selection
         * @param id                Dynamic style ID
         * @param uniform           Style uniform
         * @param font              Font handle
         * @param alignment         Alignment
         * @param features          Font features to use
         * @param padding           Padding inside the node in order left, top,
         *      right, bottom
         * @param cursorUniform     Style uniform for the editing cursor
         * @param cursorPadding     Padding around the editing cursor in order
         *      left, top, right, bottom
         * @param selectionUniform  Style uniform for the editing selection
         * @param selectionTextUniform  Style uniform applied to selected
         *      portions of the text or @relativeref{Corrade,Containers::NullOpt}
         *      if @p uniform should be used for the selection as well
         * @param selectionPadding  Padding around the editing selection in
         *      order left, top, right, bottom
         *
         * Expects that the @p id is less than @ref Shared::dynamicStyleCount(),
         * @ref Shared::hasEditingStyles() is @cpp true @ce, @p font is either
         * @ref FontHandle::Null or valid and @p alignment is not
         * `*GlyphBounds` as the implementation can only align based on font
         * metrics and cursor position, not actual glyph bounds.
         * @ref Shared::styleCount() plus @p id is then a style index that can
         * be passed to @ref create(), @ref createGlyph() or @ref setStyle() in
         * order to use this style. Compared to @ref Shared::setStyle() the
         * mapping between dynamic styles and uniforms is implicit. All dynamic
         * styles are initially default-constructed @ref TextLayerStyleUniform
         * instances, @ref FontHandle::Null, @ref Text::Alignment::MiddleCenter
         * with empty feature lists, zero padding vectors and no associated
         * cursor or selection styles.
         *
         * Calling this function causes @ref LayerState::NeedsCommonDataUpdate
         * to be set to trigger an upload of changed dynamic style uniform
         * data. If @p padding, @p cursorPadding or @p selectionPadding
         * changed, @ref LayerState::NeedsDataUpdate gets set as well. On the
         * other hand, changing the @p font, @p alignment or @p features
         * doesn't cause any state flag to be set --- the @p font, @p alignment
         * and @p features get only used in the following call to
         * @ref create(), @ref createGlyph(), @ref setText() or
         * @ref setGlyph().
         *
         * Note that while @p selectionPadding is merely a way to visually
         * fine-tune the appearance, @p cursorPadding has to be non-zero
         * horizontally to actually be visible at all.
         *
         * Use @ref setDynamicStyleWithCursor() if showing selection is not
         * desirable for given style, @ref setDynamicStyleWithSelection() if
         * showing cursor is not desirable and @ref setDynamicStyle() for
         * non-editable styles or editable styles where showing neither cursor
         * nor selection is desirable, for example in widgets that aren't
         * focused.
         * @see @ref dynamicStyleUniforms(), @ref dynamicStyleFonts(),
         *      @ref dynamicStyleAlignments(), @ref dynamicStyleFeatures(),
         *      @ref dynamicStyleCursorStyles(),
         *      @ref dynamicStyleCursorStyle(),
         *      @ref dynamicStyleSelectionStyles(),
         *      @ref dynamicStyleSelectionStyle(),
         *      @ref dynamicStyleSelectionStyleTextUniform(),
         *      @ref dynamicStylePaddings(),
         *      @ref dynamicEditingStyleUniforms(),
         *      @ref dynamicEditingStylePaddings(),
         *      @ref allocateDynamicStyle(), @ref recycleDynamicStyle()
         */
        void setDynamicStyleWithCursorSelection(UnsignedInt id, const TextLayerStyleUniform& uniform, FontHandle font, Text::Alignment alignment, Containers::ArrayView<const TextFeatureValue> features, const Vector4& padding, const TextLayerEditingStyleUniform& cursorUniform, const Vector4& cursorPadding, const TextLayerEditingStyleUniform& selectionUniform, const Containers::Optional<TextLayerStyleUniform>& selectionTextUniform, const Vector4& selectionPadding);
        /** @overload */
        void setDynamicStyleWithCursorSelection(UnsignedInt id, const TextLayerStyleUniform& uniform, FontHandle font, Text::Alignment alignment, std::initializer_list<TextFeatureValue> features, const Vector4& padding, const TextLayerEditingStyleUniform& cursorUniform, const Vector4& cursorPadding, const TextLayerEditingStyleUniform& selectionUniform, const Containers::Optional<TextLayerStyleUniform>& selectionTextUniform, const Vector4& selectionPadding);

        /**
         * @brief Set a dynamic style for text and cursor only
         * @param id                Dynamic style ID
         * @param uniform           Style uniform
         * @param font              Font handle
         * @param alignment         Alignment
         * @param features          Font features to use
         * @param padding           Padding inside the node in order left, top,
         *      right, bottom
         * @param cursorUniform     Style uniform for the editing cursor
         * @param cursorPadding     Padding around the editing cursor in order
         *      left, top, right, bottom
         *
         * Compared to @ref setDynamicStyleWithCursorSelection(UnsignedInt, const TextLayerStyleUniform&, FontHandle, Text::Alignment, Containers::ArrayView<const TextFeatureValue>, const Vector4&, const TextLayerEditingStyleUniform&, const Vector4&, const TextLayerEditingStyleUniform&, const Containers::Optional<TextLayerStyleUniform>&, const Vector4&)
         * this doesn't set any selection style, meaning a
         * @ref TextDataFlag::Editable text will be rendered without the
         * selection visible. Useful for example for dynamic styles of input
         * widgets which show the cursor always but selection only when
         * focused.
         * @see @ref setDynamicStyleWithSelection(), @ref setDynamicStyle()
         */
        void setDynamicStyleWithCursor(UnsignedInt id, const TextLayerStyleUniform& uniform, FontHandle font, Text::Alignment alignment, Containers::ArrayView<const TextFeatureValue> features, const Vector4& padding, const TextLayerEditingStyleUniform& cursorUniform, const Vector4& cursorPadding);
        /** @overload */
        void setDynamicStyleWithCursor(UnsignedInt id, const TextLayerStyleUniform& uniform, FontHandle font, Text::Alignment alignment, std::initializer_list<TextFeatureValue> features, const Vector4& padding, const TextLayerEditingStyleUniform& cursorUniform, const Vector4& cursorPadding);

        /**
         * @brief Set a dynamic style for text and selection only
         * @param id                Dynamic style ID
         * @param uniform           Style uniform
         * @param font              Font handle
         * @param alignment         Alignment
         * @param features          Font features to use
         * @param padding           Padding inside the node in order left, top,
         *      right, bottom
         * @param selectionUniform  Style uniform for the editing selection
         * @param selectionTextUniform  Style uniform applied to selected
         *      portions of the text or @relativeref{Corrade,Containers::NullOpt}
         *      if @p uniform should be used for the selection as well
         * @param selectionPadding  Padding around the editing selection in
         *      order left, top, right, bottom
         *
         * Compared to @ref setDynamicStyleWithCursorSelection(UnsignedInt, const TextLayerStyleUniform&, FontHandle, Text::Alignment, Containers::ArrayView<const TextFeatureValue>, const Vector4&, const TextLayerEditingStyleUniform&, const Vector4&, const TextLayerEditingStyleUniform&, const Containers::Optional<TextLayerStyleUniform>&, const Vector4&)
         * this doesn't set any cursor style, meaning a
         * @ref TextDataFlag::Editable text will be rendered without the
         * cursor visible. Useful for example for dynamic styles of input
         * widgets which show the selection always but cursor only when
         * focused.
         * @see @ref setDynamicStyleWithCursor(), @ref setDynamicStyle()
         */
        void setDynamicStyleWithSelection(UnsignedInt id, const TextLayerStyleUniform& uniform, FontHandle font, Text::Alignment alignment, Containers::ArrayView<const TextFeatureValue> features, const Vector4& padding, const TextLayerEditingStyleUniform& selectionUniform, const Containers::Optional<TextLayerStyleUniform>& selectionTextUniform, const Vector4& selectionPadding);
        /** @overload */
        void setDynamicStyleWithSelection(UnsignedInt id, const TextLayerStyleUniform& uniform, FontHandle font, Text::Alignment alignment, std::initializer_list<TextFeatureValue> features, const Vector4& padding, const TextLayerEditingStyleUniform& selectionUniform, const Containers::Optional<TextLayerStyleUniform>& selectionTextUniform, const Vector4& selectionPadding);

        /**
         * @brief Create a text
         * @param style         Style index
         * @param text          Text to render
         * @param properties    Text properties
         * @param flags         Flags
         * @param node          Node to attach to
         * @return New data handle
         *
         * Expects that @ref Shared::setStyle() has been called, @p style is
         * less than @ref Shared::totalStyleCount(),
         * @ref TextProperties::font() is either @ref FontHandle::Null or
         * valid and @p flags don't contain @ref TextDataFlag::Editable if the
         * layer was created with @ref TextLayerFlag::Transformable enabled.
         * Styling is driven from the @ref TextLayerStyleUniform at index
         * @p style. If @ref TextProperties::font() is not null it's used,
         * otherwise the default @ref FontHandle assigned to given style is
         * used and is expected to not be null. The @ref FontHandle, whether
         * coming from the @p style or from @p properties, is expected to have
         * a font instance. Instance-less fonts can be only used to create
         * single glyphs (such as various icons or images) with
         * @ref createGlyph().
         *
         * If @p flags contain @ref TextDataFlag::Editable, the @p text and
         * @p properties are remembered and subsequently accessible through
         * @ref text() and @ref textProperties(), @ref cursor() position and
         * selection are both set to @p text size. Currently, for editable
         * text, the @p properties are expected to have empty
         * @ref TextProperties::features() --- only the features supplied by
         * the style are used for editable text.
         * @see @ref Shared::hasFontInstance(), @ref setText(),
         *      @ref setColor(), @ref setPadding(), @ref setCursor(),
         *      @ref updateText(), @ref editText()
         */
        DataHandle create(UnsignedInt style, Containers::StringView text, const TextProperties& properties, TextDataFlags flags = {}, NodeHandle node =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            NodeHandle::Null
            #else
            NodeHandle{} /* To not have to include Handle.h */
            #endif
        );
        /** @overload */
        DataHandle create(UnsignedInt style, Containers::StringView text, const TextProperties& properties, NodeHandle node) {
            return create(style, text, properties, TextDataFlags{}, node);
        }

        /**
         * @brief Create a text with a style index in a concrete enum type
         *
         * Casts @p style to @relativeref{Magnum,UnsignedInt} and delegates to
         * @ref create(UnsignedInt, Containers::StringView, const TextProperties&, TextDataFlags, NodeHandle).
         */
        template<class StyleIndex
            #ifndef DOXYGEN_GENERATING_OUTPUT
            /* Accept any enum except NodeHandle to prevent create(node, text)
               from being called by mistake */
            , typename std::enable_if<std::is_enum<StyleIndex>::value && !std::is_same<StyleIndex, NodeHandle>::value, int>::type = 0
            #endif
        > DataHandle create(StyleIndex style, Containers::StringView text, const TextProperties& properties, TextDataFlags flags = {}, NodeHandle node =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            NodeHandle::Null
            #else
            NodeHandle{} /* To not have to include Handle.h */
            #endif
        ) {
            return create(UnsignedInt(style), text, properties, flags, node);
        }

        /** @overload */
        template<class StyleIndex
            #ifndef DOXYGEN_GENERATING_OUTPUT
            /* Accept any enum except NodeHandle to prevent create(node, text)
               from being called by mistake */
            , typename std::enable_if<std::is_enum<StyleIndex>::value && !std::is_same<StyleIndex, NodeHandle>::value, int>::type = 0
            #endif
        > DataHandle create(StyleIndex style, Containers::StringView text, const TextProperties& properties, NodeHandle node) {
            return create(style, text, properties, TextDataFlags{}, node);
        }

        /**
         * @brief Create a single glyph
         * @param style         Style index
         * @param glyph         Glyph ID to render
         * @param properties    Text properties
         * @param node          Node to attach to
         * @return New data handle
         *
         * Expects that @ref Shared::setStyle() has been called, @p style is
         * less than @ref Shared::totalStyleCount() and
         * @ref TextProperties::font() is either @ref FontHandle::Null or
         * valid. Styling is driven from the @ref TextLayerStyleUniform at
         * index @p style. If @ref TextProperties::font() is not null it's
         * used, otherwise the default @ref FontHandle assigned to given style
         * is used and is expected to not be null. The @p glyph is expected to
         * be less than @ref Text::AbstractGlyphCache::fontGlyphCount() for
         * given font.
         *
         * Compared to @ref create(), the glyph is aligned according to
         * @ref TextProperties::alignment() based on its bounding rectangle
         * coming from the glyph cache, not based on font metrics. If the
         * @ref Text::Alignment is `*Start` or `*End`, it's converted to an
         * appropriate `*Left` or `*Right` value based on
         * @ref TextProperties::layoutDirection() and
         * @relativeref{TextProperties,shapeDirection()}. The
         * @ref TextProperties::script(),
         * @relativeref{TextProperties,language()} and
         * @relativeref{TextProperties,features()} properties aren't used in
         * any way.
         * @see @ref Shared::glyphCacheFontId(), @ref setGlyph(),
         *      @ref setColor(), @ref setPadding()
         */
        DataHandle createGlyph(UnsignedInt style, UnsignedInt glyph, const TextProperties& properties, NodeHandle node =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            NodeHandle::Null
            #else
            NodeHandle{} /* To not have to include Handle.h */
            #endif
        );

        /**
         * @brief Create a single glyph with a style index in a concrete enum type
         *
         * Casts @p style to @relativeref{Magnum,UnsignedInt} and delegates to
         * @ref createGlyph(UnsignedInt, UnsignedInt, const TextProperties&, NodeHandle).
         */
        template<class StyleIndex
            #ifndef DOXYGEN_GENERATING_OUTPUT
            /* Accept any enum except NodeHandle to prevent
               createGlyph(node, glyph) from being called by mistake */
            , typename std::enable_if<std::is_enum<StyleIndex>::value && !std::is_same<StyleIndex, NodeHandle>::value, int>::type = 0
            #endif
        > DataHandle createGlyph(StyleIndex style, UnsignedInt glyph, const TextProperties& properties, NodeHandle node =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            NodeHandle::Null
            #else
            NodeHandle{} /* To not have to include Handle.h */
            #endif
        ) {
            return createGlyph(UnsignedInt(style), glyph, properties, node);
        }

        /**
         * @brief Create a single glyph with a glyph ID in a concrete enum type
         *
         * Casts @p glyph to @relativeref{Magnum,UnsignedInt} and delegates to
         * @ref createGlyph(UnsignedInt, UnsignedInt, const TextProperties&, NodeHandle).
         */
        template<class GlyphIndex
            #ifndef DOXYGEN_GENERATING_OUTPUT
            /* Accept any enum except NodeHandle to prevent
               createGlyph(node, glyph) from being called by mistake */
            , typename std::enable_if<std::is_enum<GlyphIndex>::value && !std::is_same<GlyphIndex, NodeHandle>::value, int>::type = 0
            #endif
        > DataHandle createGlyph(UnsignedInt style, GlyphIndex glyph, const TextProperties& properties, NodeHandle node =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            NodeHandle::Null
            #else
            NodeHandle{} /* To not have to include Handle.h */
            #endif
        ) {
            return createGlyph(style, UnsignedInt(glyph), properties, node);
        }

        /**
         * @brief Create a single glyph with a style index and glyph ID in a concrete enum type
         *
         * Casts @p style and @p glyph to @relativeref{Magnum,UnsignedInt} and
         * delegates to @ref createGlyph(UnsignedInt, UnsignedInt, const TextProperties&, NodeHandle).
         */
        template<class StyleIndex, class GlyphIndex
            #ifndef DOXYGEN_GENERATING_OUTPUT
            /* Accept any enum except NodeHandle to prevent
               createGlyph(node, glyph), createGlyph(style, node) etc.
               from being called by mistake */
            , typename std::enable_if<std::is_enum<StyleIndex>::value && std::is_enum<GlyphIndex>::value && !std::is_same<StyleIndex, NodeHandle>::value && !std::is_same<GlyphIndex, NodeHandle>::value, int>::type = 0
            #endif
        > DataHandle createGlyph(StyleIndex style, GlyphIndex glyph, const TextProperties& properties, NodeHandle node =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            NodeHandle::Null
            #else
            NodeHandle{} /* To not have to include Handle.h */
            #endif
        ) {
            return createGlyph(UnsignedInt(style), UnsignedInt(glyph), properties, node);
        }

        /**
         * @brief Remove a text
         *
         * Delegates to @ref AbstractLayer::remove(DataHandle) and additionally
         * marks the now-unused glyph run for removal in the next @ref update().
         */
        void remove(DataHandle handle);

        /**
         * @brief Remove a text assuming it belongs to this layer
         *
         * Delegates to @ref AbstractLayer::remove(LayerDataHandle) and
         * additionally marks the now-unused glyph run for removal in the next
         * @ref update().
         */
        void remove(LayerDataHandle handle);

        /**
         * @brief Text flags
         *
         * Expects that @p handle is valid. Note that, to avoid implementation
         * complexity, the flags can be only specified in @ref create() or
         * @ref setText(DataHandle, Containers::StringView, const TextProperties&, TextDataFlags)
         * and cannot be modified independently. Data created with
         * @ref createGlyph() or updated with @ref setGlyph() have the flags
         * always empty.
         * @see @ref isHandleValid(DataHandle) const,
         *      @ref flags(), @ref TextLayer::Shared::flags()
         */
        TextDataFlags flags(DataHandle handle) const;

        /**
         * @brief Text flags assuming it belongs to this layer
         *
         * Like @ref flags(DataHandle) const but without checking that
         * @p handle indeed belongs to this layer. See its documentation for
         * more information.
         */
        TextDataFlags flags(LayerDataHandle handle) const;

        /**
         * @brief Text glyph count
         *
         * Expects that @p handle is valid.
         * @see @ref isHandleValid(DataHandle) const
         */
        UnsignedInt glyphCount(DataHandle handle) const;

        /**
         * @brief Text glyph count assuming it belongs to this layer
         *
         * Expects that @p handle is valid.
         * @see @ref isHandleValid(LayerDataHandle) const
         */
        UnsignedInt glyphCount(LayerDataHandle handle) const;

        /**
         * @brief Size of the laid out text
         *
         * Expects that @p handle is valid. For text laid out with
         * @ref create() or @ref setText() the size is derived from ascent,
         * descent and advances of individual glyphs, not from actual area of
         * the glyphs being drawn, as that may not be known at that time. For
         * @ref createGlyph() or @ref setGlyph() the size is based on the
         * actual glyph size coming out of the glyph cache.
         *
         * Text transformation, if @ref TextLayerFlag::Transformable is
         * enabled, is not taken into account in any way.
         * @see @ref isHandleValid(DataHandle) const
         */
        Vector2 size(DataHandle handle) const;

        /**
         * @brief Size of the laid out text assuming it belongs to this layer
         *
         * Like @ref size(DataHandle) const but without checking that @p handle
         * indeed belongs to this layer. See its documentation for more
         * information.
         */
        Vector2 size(LayerDataHandle handle) const;

        /**
         * @brief Cursor and selection position in an editable text
         *
         * Expects that @p handle is valid and the text was created or set with
         * @ref TextDataFlag::Editable present. Both values are guaranteed to
         * be always within bounds of a corresponding
         * @ref text(DataHandle) const. The first value is cursor position, the
         * second value denotes the other end of the selection. If both are the
         * same, there's no selection.
         * @see @ref isHandleValid(DataHandle) const,
         *      @ref flags(DataHandle) const
         */
        Containers::Pair<UnsignedInt, UnsignedInt> cursor(DataHandle handle) const;

        /**
         * @brief Cursor and selection position in an editable text assuming it belongs to this layer
         *
         * Like @ref cursor(DataHandle) const but without checking that
         * @p handle indeed belongs to this layer. See its documentation for
         * more information.
         */
        Containers::Pair<UnsignedInt, UnsignedInt> cursor(LayerDataHandle handle) const;

        /**
         * @brief Set cursor position and selection in an editable text
         *
         * Low-level interface for cursor positioning. See @ref updateText()
         * for a low-level interface to perform text modifications together
         * with cursor positioning, use @ref editText() to perform higher-level
         * operations with UTF-8 and text directionality awareness.
         *
         * Expects that @p handle is valid and the text was created or set with
         * @ref TextDataFlag::Editable enabled. Both the @p position and
         * @p selection is expected to be less or equal to @ref text() size.
         * The distance between the two is what's selected --- if @p selection
         * is less than @p position, the selection is before the cursor, if
         * @p position is less than @p selection, the selection is after the
         * cursor. If they're the same, there's no selection. No UTF-8 sequence
         * boundary adjustment is done for either of the two, i.e. it's
         * possible to move the cursor or selection inside a multi-byte UTF-8
         * sequence.
         *
         * Calling this function causes @ref LayerState::NeedsDataUpdate to be
         * set, unless the operation performed is a no-op, which is when both
         * @p removeSize and @p insertText size are both @cpp 0 @ce and
         * @p cursor is equal to @ref cursor().
         * @see @ref isHandleValid(DataHandle) const,
         *      @ref flags(DataHandle) const
         */
        void setCursor(DataHandle handle, UnsignedInt position, UnsignedInt selection);

        /**
         * @brief Set cursor position in an editable text
         *
         * Same as calling @ref setCursor(DataHandle, UnsignedInt, UnsignedInt)
         * with @p position passed to both @p position and @p selection, i.e.
         * with nothing selected.
         */
        void setCursor(DataHandle handle, UnsignedInt position) {
            setCursor(handle, position, position);
        }

        /**
         * @brief Set cursor position and selection in an editable text assuming it belongs to this layer
         *
         * Like @ref setCursor(DataHandle, UnsignedInt, UnsignedInt) but
         * without checking that @p handle indeed belongs to this layer. See
         * its documentation for more information.
         */
        void setCursor(LayerDataHandle handle, UnsignedInt position, UnsignedInt selection);

        /**
         * @brief Set cursor position in an editable text assuming it belongs to this layer
         *
         * Same as calling @ref setCursor(LayerDataHandle, UnsignedInt, UnsignedInt)
         * with @p position passed to both @p position and @p selection, i.e.
         * with nothing selected.
         */
        void setCursor(LayerDataHandle handle, UnsignedInt position) {
            setCursor(handle, position, position);
        }

        /**
         * @brief Properties used for shaping an editable text
         *
         * Expects that @p handle is valid and the text was created with
         * @ref TextDataFlag::Editable set. Returns content of
         * @ref TextProperties passed in previous @ref create() or
         * @ref setText() call for @p handle, except for
         * @ref TextProperties::font() that is picked from the style if it was
         * passed as @ref FontHandle::Null.
         * @see @ref isHandleValid(DataHandle) const,
         *      @ref flags(DataHandle) const, @ref style(DataHandle) const
         */
        TextProperties textProperties(DataHandle handle) const;

        /**
         * @brief Properties used for shaping an editable text assuming it belongs to this layer
         *
         * Like @ref textProperties(DataHandle) const but without checking that
         * @p handle indeed belongs to this layer. See its documentation for
         * more information.
         */
        TextProperties textProperties(LayerDataHandle handle) const;

        /**
         * @brief Contents of an editable text
         *
         * Expects that @p handle is valid and the text was created or set with
         * @ref TextDataFlag::Editable present. The returned view is only valid
         * until the next @ref create(), @ref setText() or @ref update() call
         * and is never
         * @relativeref{Corrade,Containers::StringViewFlag::NullTerminated}.
         * @see @ref isHandleValid(DataHandle) const
         */
        Containers::StringView text(DataHandle handle) const;

        /**
         * @brief Contents of an editable text assuming it belongs to this layer
         *
         * Like @ref text(DataHandle) const but without checking that @p handle
         * indeed belongs to this layer. See its documentation for more
         * information.
         */
        Containers::StringView text(LayerDataHandle handle) const;

        /**
         * @brief Set text
         *
         * Expects that @p handle is valid and @ref TextProperties::font() is
         * either @ref FontHandle::Null or valid. If not null, the
         * @ref TextProperties::font() is used, otherwise the default
         * @ref FontHandle assigned to @ref style() is used and expected to not
         * be null. Note that it's not possible to change the font alone with
         * @ref setStyle(), it only can be done when setting the text. The
         * @ref FontHandle, whether coming from the style or from
         * @p properties, is expected to have a font instance. Instance-less
         * fonts can be only used to set single glyphs (such as various icons
         * or images) with @ref setGlyph().
         *
         * This function preserves existing @ref flags(DataHandle) const for
         * given @p handle, use @ref setText(DataHandle, Containers::StringView, const TextProperties&, TextDataFlags)
         * to supply different @ref TextDataFlags for the new text.
         *
         * If @ref flags(DataHandle) const contain @ref TextDataFlag::Editable,
         * the @p text and @p properties are remembered and subsequently
         * accessible through @ref text() and @ref textProperties(),
         * @ref cursor() position and selection are both set to @p text size.
         * Currently, for editable text, the @p properties are expected to have
         * empty @ref TextProperties::features() --- only the features supplied
         * by the style are used for editable text.
         *
         * Calling this function causes @ref LayerState::NeedsDataUpdate to be
         * set.
         * @see @ref isHandleValid(DataHandle) const,
         *      @ref Shared::isHandleValid(FontHandle) const, @ref setCursor(),
         *      @ref updateText(), @ref editText()
         */
        void setText(DataHandle handle, Containers::StringView text, const TextProperties& properties);

        /**
         * @brief Set text with different flags
         *
         * Like @ref setText(DataHandle, Containers::StringView, const TextProperties&)
         * but supplying different @ref TextDataFlags for the new text instead
         * of preserving existing @ref flags(DataHandle) const. Expects that
         * @p flags don't contain @ref TextDataFlag::Editable if the layer was
         * created with @ref TextLayerFlag::Transformable enabled.
         */
        void setText(DataHandle handle, Containers::StringView text, const TextProperties& properties, TextDataFlags flags);

        /**
         * @brief Set text assuming it belongs to this layer
         *
         * Like @ref setText(DataHandle, Containers::StringView, const TextProperties&)
         * but without checking that @p handle indeed belongs to this layer.
         * See its documentation for more information.
         */
        void setText(LayerDataHandle handle, Containers::StringView text, const TextProperties& properties);

        /**
         * @brief Set text with different flags assuming it belongs to this layer
         *
         * Like @ref setText(DataHandle, Containers::StringView, const TextProperties&, TextDataFlags)
         * but without checking that @p handle indeed belongs to this layer.
         * See its documentation for more information.
         */
        void setText(LayerDataHandle handle, Containers::StringView text, const TextProperties& properties, TextDataFlags flags);

        /**
         * @brief Update text, cursor position and selection in an editable text
         * @param handle        Handle which to update
         * @param removeOffset  Offset at which to remove
         * @param removeSize    Count of bytes to remove
         * @param insertOffset  Offset at which to insert after the removal
         * @param insertText    Text to insert after the removal
         * @param cursor        Cursor position to set after the removal and
         *      subsequent insert
         * @param selection     Selection to set after the removal and
         *      subsequent insert
         *
         * Low-level interface for text removal and insertion together with
         * cursor positioning. See @ref setCursor() for just cursor positioning
         * alone, use @ref editText() to perform higher-level operations with
         * UTF-8 and text directionality awareness.
         *
         * Expects that @p handle is valid and the text was created or set with
         * @ref TextDataFlag::Editable enabled. The @p removeOffset together
         * with @p removeSize is expected to be less or equal to @ref text()
         * size; @p insertOffset then equal to the text size without
         * @p removeSize; @p cursor and @p selection then to text size without
         * @p removeSize but with @p insertText size. The distance between
         * @p cursor and @p selection is what's selected --- if @p selection
         * is less than @p cursor, the selection is before the cursor, if
         * @p cursor is less than @p selection, the selection is after the
         * cursor. If they're the same, there's no selection. No UTF-8
         * sequence boundary adjustment is done for any of these, i.e. it's
         * possible to remove or insert partial multi-byte UTF-8 sequences and
         * position the cursor inside them as well.
         *
         * Calling this function causes @ref LayerState::NeedsDataUpdate to be
         * set, unless the operation performed is a no-op, which is when both
         * @p removeSize and @p insertText size are both @cpp 0 @ce and
         * @p cursor is equal to @ref cursor().
         * @see @ref isHandleValid(DataHandle) const,
         *      @ref flags(DataHandle) const, @ref setText()
         */
        void updateText(DataHandle handle, UnsignedInt removeOffset, UnsignedInt removeSize, UnsignedInt insertOffset, Containers::StringView insertText, UnsignedInt cursor, UnsignedInt selection);

        /**
         * @brief Update text and cursor position in an editable text
         *
         * Same as calling @ref updateText(DataHandle, UnsignedInt, UnsignedInt, UnsignedInt, Containers::StringView, UnsignedInt, UnsignedInt)
         * with @p position passed to both @p position and @p selection, i.e.
         * with nothing selected.
         */
        void updateText(DataHandle handle, UnsignedInt removeOffset, UnsignedInt removeSize, UnsignedInt insertOffset, Containers::StringView insertText, UnsignedInt cursor) {
            updateText(handle, removeOffset, removeSize, insertOffset, insertText, cursor, cursor);
        }

        /**
         * @brief Update text, cursor position and selection in an editable text assuming it belongs to this layer
         *
         * Like @ref updateText(DataHandle, UnsignedInt, UnsignedInt, UnsignedInt, Containers::StringView, UnsignedInt, UnsignedInt)
         * but without checking that @p handle indeed belongs to this layer.
         * See its documentation for more information.
         */
        void updateText(LayerDataHandle handle, UnsignedInt removeOffset, UnsignedInt removeSize, UnsignedInt insertOffset, Containers::StringView insertText, UnsignedInt cursor, UnsignedInt selection);

        /**
         * @brief Update text and cursor position in an editable text
         *
         * Same as calling @ref updateText(DataHandle, UnsignedInt, UnsignedInt, UnsignedInt, Containers::StringView, UnsignedInt, UnsignedInt)
         * with @p position passed to both @p position and @p selection, i.e.
         * with nothing selected.
         */
        void updateText(LayerDataHandle handle, UnsignedInt removeOffset, UnsignedInt removeSize, UnsignedInt insertOffset, Containers::StringView insertText, UnsignedInt cursor) {
            updateText(handle, removeOffset, removeSize, insertOffset, insertText, cursor, cursor);
        }

        /**
         * @brief Edit text at current cursor position
         *
         * High-level interface for text editing, mapping to usual user
         * interface operations. Delegates to @ref setCursor() or
         * @ref updateText() internally.
         *
         * Expects that @p handle is valid, the text was created or set with
         * @ref TextDataFlag::Editable enabled and @p insert is non-empty only
         * if appropriate @p edit operation is used. See documentation of the
         * @ref TextEdit enum for detailed behavior of each operation.
         *
         * Calling this function causes @ref LayerState::NeedsDataUpdate to be
         * set, unless the operation performed is a no-op such as inserting
         * empty text or moving a cursor / deleting a character with there
         * being no character to move over or delete.
         * @see @ref isHandleValid(DataHandle) const,
         *      @ref flags(DataHandle) const, @ref setText()
         */
        void editText(DataHandle handle, TextEdit edit, Containers::StringView insert);

        /**
         * @brief Edit text at current cursor position assuming it belongs to this layer
         *
         * Like @ref editText(DataHandle, TextEdit, Containers::StringView) but
         * without checking that @p handle indeed belongs to this layer. See
         * its documentation for more information.
         */
        void editText(LayerDataHandle handle, TextEdit edit, Containers::StringView insert);

        /**
         * @brief Set a single glyph
         *
         * Expects that @p handle is valid and @ref TextProperties::font() is
         * either @ref FontHandle::Null or valid. If not null, the
         * @ref TextProperties::font() is used, otherwise the default
         * @ref FontHandle assigned to @ref style() is used and expected to not
         * be null. Note that it's not possible to change the font alone with
         * @ref setStyle(), it only can be done when setting the glyph. The
         * @p glyph is expected to be less than
         * @ref Text::AbstractGlyphCache::fontGlyphCount() for given font.
         *
         * Compared to @ref setText(), the glyph is aligned according to
         * @ref TextProperties::alignment() based on its bounding rectangle
         * coming from the glyph cache, not based on font metrics. The
         * @ref TextProperties::script(),
         * @relativeref{TextProperties,language()},
         * @relativeref{TextProperties,shapeDirection()},
         * @relativeref{TextProperties,layoutDirection()} and
         * @relativeref{TextProperties,features()} properties aren't used in
         * any way; @ref flags(DataHandle) const get reset to empty. Note that
         * it's also possible to change a handle that previously contained a
         * text to a single glyph and vice versa --- the internal
         * representation of both is the same.
         *
         * Calling this function causes @ref LayerState::NeedsDataUpdate to be
         * set.
         * @see @ref isHandleValid(DataHandle) const,
         *      @ref Shared::isHandleValid(FontHandle) const,
         *      @ref Shared::glyphCacheFontId()
         */
        void setGlyph(DataHandle handle, UnsignedInt glyph, const TextProperties& properties);

        /**
         * @brief Set a single glyph with a glyph ID in a concrete enum type
         *
         * Casts @p glyph to @relativeref{Magnum,UnsignedInt} and delegates to
         * @ref setGlyph(DataHandle, UnsignedInt, const TextProperties&).
         */
        template<class GlyphIndex
            #ifndef DOXYGEN_GENERATING_OUTPUT
            , typename std::enable_if<std::is_enum<GlyphIndex>::value, int>::type = 0
            #endif
        > void setGlyph(DataHandle handle, GlyphIndex glyph, const TextProperties& properties) {
            return setGlyph(handle, UnsignedInt(glyph), properties);
        }

        /**
         * @brief Set a single glyph assuming it belongs to this layer
         *
         * Like @ref setGlyph(DataHandle, UnsignedInt, const TextProperties&)
         * but without checking that @p handle indeed belongs to this layer.
         * See its documentation for more information.
         */
        void setGlyph(LayerDataHandle handle, UnsignedInt glyph, const TextProperties& properties);

        /**
         * @brief Set a single glyph with a glyph ID in a concrete enum type assuming it belongs to this layer
         *
         * Casts @p glyph to @relativeref{Magnum,UnsignedInt} and delegates to
         * @ref setGlyph(LayerDataHandle, UnsignedInt, const TextProperties&).
         */
        template<class GlyphIndex
            #ifndef DOXYGEN_GENERATING_OUTPUT
            , typename std::enable_if<std::is_enum<GlyphIndex>::value, int>::type = 0
            #endif
        > void setGlyph(LayerDataHandle handle, GlyphIndex glyph, const TextProperties& properties) {
            return setGlyph(handle, UnsignedInt(glyph), properties);
        }

        /**
         * @brief Custom text base color
         *
         * Expects that @p handle is valid.
         * @see @ref isHandleValid(DataHandle) const
         */
        Color4 color(DataHandle handle) const;

        /**
         * @brief Custom text base color assuming it belongs to this layer
         *
         * Expects that @p handle is valid.
         * @see @ref isHandleValid(LayerDataHandle) const
         */
        Color4 color(LayerDataHandle handle) const;

        /**
         * @brief Set custom text base color
         *
         * Expects that @p handle is valid. The @p color is expected to have
         * premultiplied alpha. It is multiplied with
         * @ref TextLayerStyleUniform::color, and with node opacity coming from
         * @ref AbstractUserInterface::setNodeOpacity(). Applies to style
         * override for selected text as well, but not to
         * @ref TextLayerEditingStyleUniform::backgroundColor for cursor and
         * selection rectangles. By default, the custom color is
         * @cpp 0xffffffff_srgbaf @ce, i.e. not affecting the color coming from
         * the style in any way.
         *
         * Calling this function causes @ref LayerState::NeedsDataUpdate to be
         * set.
         * @see @ref isHandleValid(DataHandle) const,
         *      @ref Color4::premultiplied()
         */
        void setColor(DataHandle handle, const Color4& color);

        /**
         * @brief Set custom text base color assuming it belongs to this layer
         *
         * Like @ref setColor(DataHandle, const Color4&) but without checking
         * that @p handle indeed belongs to this layer. See its documentation
         * for more information.
         */
        void setColor(LayerDataHandle handle, const Color4& color);

        /**
         * @brief Custom text padding
         *
         * In UI units, in order left, top, right, bottom. Expects that
         * that the layer was *not* created with
         * @ref TextLayerFlag::Transformable enabled and that @p handle is
         * valid.
         * @see @ref isHandleValid(DataHandle) const
         */
        Vector4 padding(DataHandle handle) const;

        /**
         * @brief Custom text padding assuming it belongs to this layer
         *
         * Like @ref padding(DataHandle) const but without checking that
         * @p handle indeed belongs to this layer. See its documentation for
         * more information.
         */
        Vector4 padding(LayerDataHandle handle) const;

        /**
         * @brief Set custom text padding
         *
         * Expects that that the layer was *not* created with
         * @ref TextLayerFlag::Transformable enabled and that @p handle is
         * valid. The @p padding is in UI units, in order left, top, right,
         * bottom, and is added to the per-style padding values specified in
         * @ref Shared::setStyle(). By default, the custom padding is a zero
         * vector, i.e. not affecting the padding coming from the style in any
         * way.
         *
         * Calling this function causes @ref LayerState::NeedsDataUpdate to be
         * set.
         * @see @ref isHandleValid(DataHandle) const
         */
        void setPadding(DataHandle handle, const Vector4& padding);

        /**
         * @brief Set custom text padding assuming it belongs to this layer
         *
         * Like @ref setPadding(DataHandle, const Vector4&) but without
         * checking that @p handle indeed belongs to this layer. See its
         * documentation for more information.
         */
        void setPadding(LayerDataHandle handle, const Vector4& padding);

        /**
         * @brief Set custom text padding with all edges having the same value
         *
         * Equivalent to calling @ref setPadding(DataHandle, const Vector4&)
         * with all four components set to @p padding. See its documentation
         * for more information.
         */
        void setPadding(DataHandle handle, Float padding) {
            setPadding(handle, Vector4{padding});
        }

        /**
         * @brief Set custom text padding with all edges having the same value assuming it belongs to this layer
         *
         * Like @ref setPadding(DataHandle, Float) but without checking that
         * @p handle indeed belongs to this layer. See its documentation for
         * more information.
         */
        void setPadding(LayerDataHandle handle, Float padding) {
            setPadding(handle, Vector4{padding});
        }

        /**
         * @brief Custom text transformation
         *
         * Translation and clockwise rotation multiplied by uniform scaling.
         * Use @ref Complex::normalized() to extract a pure rotation and
         * @ref Complex::length() to extract uniform scaling. Expects that the
         * layer was created with @ref TextLayerFlag::Transformable enabled and
         * that @p handle is valid.
         * @see @ref isHandleValid(DataHandle) const
         */
        Containers::Pair<Vector2, Complex> transformation(DataHandle handle) const;

        /**
         * @brief Custom text transformation
         *
         * Like @ref transformation(DataHandle) const but without checking that
         * @p handle indeed belongs to this layer. See its documentation for
         * more information.
         */
        Containers::Pair<Vector2, Complex> transformation(LayerDataHandle handle) const;

        /**
         * @brief Set custom text transformation
         *
         * Expects that the layer was created with
         * @ref TextLayerFlag::Transformable enabled and that @p handle is
         * valid. The @p translation is applied on top of how a particular
         * @ref Text::Alignment causes the text to be aligned within a node, as
         * specified in @ref TextLayer::Shared::setStyle(). The @p rotation is
         * interpreted with positive angle clockwise for consistency with the
         * UI coordinate system being Y down. If @p rotation is not normalized,
         * its length multiplies @p scaling. Scaling is applied first, rotation
         * second and translation last. By default, the custom transformation
         * is a zero translation vector, identity rotation and scaling of
         * @cpp 1.0f @ce.
         *
         * Rotation passed as a @relativeref{Magnum,Complex} is preferrable for
         * cases of supplying a concrete rotation several times to minimize
         * trigonometry calculations. Use the @ref setTransformation(DataHandle, const Vector2&, Rad, Float)
         * convenience overload to supply a concrete angle. Use
         * @ref translate(), @ref rotate() and @ref scale() to incrementally
         * change individual properties.
         *
         * Calling this function causes @ref LayerState::NeedsDataUpdate to be
         * set.
         */
        void setTransformation(DataHandle handle, const Vector2& translation, const Complex& rotation, Float scaling);

        /**
         * @brief Set custom text transformation
         *
         * Like @ref setTransformation(DataHandle, const Vector2&, const Complex&, Float)
         * but without checking that @p handle indeed belongs to this layer.
         * See its documentation for more information.
         */
        void setTransformation(LayerDataHandle handle, const Vector2& translation, const Complex& rotation, Float scaling);

        /**
         * @brief Set custom text transformation
         *
         * Equivalent to calling @ref setTransformation(DataHandle, const Vector2&, const Complex&, Float)
         * with @p rotation passed as @ref Complex::rotation(), positive angle
         * interpreted as clockwise for consistency with the UI coordinate
         * system being Y down. See its documentation for more information.
         *
         * If supplying a concrete rotation many times, passing it as a
         * @relativeref{Magnum,Complex} to @ref setTransformation(DataHandle, const Vector2&, const Complex&, Float)
         * instead of using this function is preferrable to minimize
         * trigonometry calculations.
         */
        void setTransformation(DataHandle handle, const Vector2& translation, Rad rotation, Float scaling);

        /**
         * @brief Set custom text transformation
         *
         * Like @ref setTransformation(DataHandle, const Vector2&, Rad, Float)
         * but without checking that @p handle indeed belongs to this layer.
         * See its documentation for more information.
         */
        void setTransformation(LayerDataHandle handle, const Vector2& translation, Rad rotation, Float scaling);

        /**
         * @brief Add to custom text translation
         *
         * Expects that the layer was created with
         * @ref TextLayerFlag::Transformable enabled and that @p handle is
         * valid. The @p translation is added to existing custom translation,
         * on top of how a particular @ref Text::Alignment causes the text to
         * be aligned within a node, as specified in
         * @ref TextLayer::Shared::setStyle(). Use @ref setTransformation() to
         * reset the translation to a concrete value. The rotation and scaling,
         * which are both applied before translation, stay unchanged. By
         * default, the custom translation is a zero vector.
         *
         * Calling this function causes @ref LayerState::NeedsDataUpdate to be
         * set.
         * @see @ref rotate(), @ref scale()
         */
        void translate(DataHandle handle, const Vector2& translation);

        /**
         * @brief Add to custom text translation
         *
         * Like @ref translate(DataHandle, const Vector2&) but without checking
         * that @p handle indeed belongs to this layer. See its documentation
         * for more information.
         */
        void translate(LayerDataHandle handle, const Vector2& translation);

        /**
         * @brief Add to custom text rotation
         *
         * Expects that the layer was created with
         * @ref TextLayerFlag::Transformable enabled and that @p handle is
         * valid. The @p rotation is interpreted with positive angle clockwise
         * for consistency with the UI coordinate system being Y down, and is
         * added to existing custom rotation. Use @ref setTransformation() to
         * reset the rotation to a concrete value. If @p rotation is not
         * normalized, its length multiplies existing custom scaling. The
         * translation, which is applied after rotation and scaling, stays
         * unchanged.
         *
         * Rotation passed as a @relativeref{Magnum,Complex} is preferrable for
         * cases of supplying a concrete rotation several times to minimize
         * trigonometry calculations. Use the @ref rotate(DataHandle, Rad)
         * convenience overload to supply a concrete angle.
         *
         * Calling this function causes @ref LayerState::NeedsDataUpdate to be
         * set.
         * @see @ref translate(), @ref scale()
         */
        void rotate(DataHandle handle, const Complex& rotation);

        /**
         * @brief Add to custom text rotation
         *
         * Like @ref rotate(DataHandle, const Complex&) but without checking
         * that @p handle indeed belongs to this layer. See its documentation
         * for more information.
         */
        void rotate(LayerDataHandle handle, const Complex& rotation);

        /**
         * @brief Add to custom text rotation
         *
         * Equivalent to calling @ref rotate(DataHandle, const Complex&) with
         * @p rotation passed as @ref Complex::rotation(), positive angle
         * interpreted as clockwise for consistency with the UI coordinate
         * system being Y down. See its documentation for more information.
         *
         * If supplying a concrete rotation many times, passing it as a
         * @relativeref{Magnum,Complex} to @ref rotate(DataHandle, const Complex&)
         * instead of using this function is preferrable to minimize
         * trigonometry calculations.
         */
        void rotate(DataHandle handle, Rad rotation);

        /**
         * @brief Add to custom text rotation
         *
         * Like @ref rotate(DataHandle, Rad) but without checking that
         * @p handle indeed belongs to this layer. See its documentation for
         * more information.
         */
        void rotate(LayerDataHandle handle, Rad rotation);

        /**
         * @brief Update custom text scaling
         *
         * Expects that the layer was created with
         * @ref TextLayerFlag::Transformable enabled and that @p handle is
         * valid. The @p scaling multiplies existing custom scaling. Use
         * @ref setTransformation() to reset the scaling to a concrete value.
         * The rotation and translation, which are both applied after scaling,
         * stay unchanged. By default, the custom scaling is @cpp 1.0f @ce.
         *
         * Calling this function causes @ref LayerState::NeedsDataUpdate to be
         * set.
         * @see @ref translate(), @ref rotate()
         */
        void scale(DataHandle handle, Float scaling);

        /**
         * @brief Update custom text scaling
         *
         * Like @ref scale(DataHandle, Float) but without checking that
         * @p handle indeed belongs to this layer. See its documentation for
         * more information.
         */
        void scale(LayerDataHandle handle, Float scaling);

    #ifdef DOXYGEN_GENERATING_OUTPUT
    private:
    #else
    protected:
    #endif
        struct State;

        MAGNUM_UI_LOCAL explicit TextLayer(LayerHandle handle, Containers::Pointer<State>&& state);
        /* Used by tests to avoid having to include / allocate the state */
        explicit TextLayer(LayerHandle handle, Shared& shared, TextLayerFlags flags = {});

        /* These can't be MAGNUM_UI_LOCAL otherwise deriving from this class
           in tests causes linker errors */

        /* Advertises LayerFeature::Draw but *does not* implement doDraw(),
           that's on the subclass */
        LayerFeatures doFeatures() const override;

        void doUpdate(LayerStates states, const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, const Containers::StridedArrayView1D<const Float>& nodeOpacities, Containers::BitArrayView nodesEnabled, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes, const Containers::StridedArrayView1D<const Vector2>& compositeRectOffsets, const Containers::StridedArrayView1D<const Vector2>& compositeRectSizes) override;

    private:
        MAGNUM_UI_LOCAL void setDynamicStyleInternal(
            #ifndef CORRADE_NO_ASSERT
            const char* messagePrefix,
            #endif
            UnsignedInt id, const TextLayerStyleUniform& uniform, FontHandle font, Text::Alignment alignment, Containers::ArrayView<const TextFeatureValue> features, const Vector4& padding);
        MAGNUM_UI_LOCAL void setDynamicCursorStyleInternal(
            #ifndef CORRADE_NO_ASSERT
            const char* messagePrefix,
            #endif
            UnsignedInt id, const TextLayerEditingStyleUniform& uniform, const Vector4& padding);
        MAGNUM_UI_LOCAL void setDynamicSelectionStyleInternal(
            #ifndef CORRADE_NO_ASSERT
            const char* messagePrefix,
            #endif
            UnsignedInt id, const TextLayerEditingStyleUniform& uniform, const Containers::Optional<TextLayerStyleUniform>& textUniform, const Vector4& padding);
        MAGNUM_UI_LOCAL DataHandle createInternal(NodeHandle node);
        MAGNUM_UI_LOCAL void shapeTextInternal(UnsignedInt id, UnsignedInt style, Containers::StringView text, const TextProperties& properties, FontHandle font, TextDataFlags flags);
        MAGNUM_UI_LOCAL void shapeRememberTextInternal(
            #ifndef CORRADE_NO_ASSERT
            const char* messagePrefix,
            #endif
            UnsignedInt id, UnsignedInt style, Containers::StringView text, const TextProperties& properties, TextDataFlags flags);
        MAGNUM_UI_LOCAL void shapeGlyphInternal(
            #ifndef CORRADE_NO_ASSERT
            const char* messagePrefix,
            #endif
            UnsignedInt id, UnsignedInt style, UnsignedInt glyphId, const TextProperties& properties);
        MAGNUM_UI_LOCAL void removeInternal(UnsignedInt id);
        MAGNUM_UI_LOCAL Containers::Pair<UnsignedInt, UnsignedInt> cursorInternal(UnsignedInt id) const;
        MAGNUM_UI_LOCAL void setCursorInternal(UnsignedInt id, UnsignedInt position, UnsignedInt selection);
        MAGNUM_UI_LOCAL TextProperties textPropertiesInternal(UnsignedInt id) const;
        MAGNUM_UI_LOCAL Containers::StringView textInternal(UnsignedInt id) const;
        MAGNUM_UI_LOCAL void setTextInternal(UnsignedInt id, Containers::StringView text, const TextProperties& properties, TextDataFlags flags);
        MAGNUM_UI_LOCAL void updateTextInternal(UnsignedInt id, UnsignedInt removeOffset, UnsignedInt removeSize, UnsignedInt insertOffset, Containers::StringView text, UnsignedInt cursor, UnsignedInt selection);
        MAGNUM_UI_LOCAL void editTextInternal(UnsignedInt id, TextEdit edit, Containers::StringView text);
        MAGNUM_UI_LOCAL void setGlyphInternal(UnsignedInt id, UnsignedInt glyph, const TextProperties& properties);
        MAGNUM_UI_LOCAL void setColorInternal(UnsignedInt id, const Color4& color);
        MAGNUM_UI_LOCAL Vector4 paddingInternal(UnsignedInt id) const;
        MAGNUM_UI_LOCAL void setPaddingInternal(UnsignedInt id, const Vector4& padding);
        MAGNUM_UI_LOCAL Containers::Pair<Vector2, Complex> transformationInternal(UnsignedInt id) const;
        MAGNUM_UI_LOCAL void setTransformationInternal(UnsignedInt id, const Vector2& translation, const Complex& rotation, Float scaling);
        MAGNUM_UI_LOCAL void translateInternal(UnsignedInt id, const Vector2& translation);
        MAGNUM_UI_LOCAL void rotateInternal(UnsignedInt id, const Complex& rotation);
        MAGNUM_UI_LOCAL void scaleInternal(UnsignedInt id, Float scaling);

        /* Can't be MAGNUM_UI_LOCAL otherwise deriving from this class in
           tests causes linker errors */
        LayerStates doState() const override;
        void doClean(Containers::BitArrayView dataIdsToRemove) override;
        void doAdvanceAnimations(Nanoseconds time, Containers::MutableBitArrayView activeStorage, const Containers::StridedArrayView1D<Float>& factorStorage, Containers::MutableBitArrayView removeStorage, const Containers::Iterable<AbstractStyleAnimator>& animators) override;
        void doKeyPressEvent(UnsignedInt dataId, KeyEvent& event) override;
        void doTextInputEvent(UnsignedInt dataId, TextInputEvent& event) override;
};

/**
@brief Text layer shared state flag
@m_since_latest

@see @ref TextLayerSharedFlags,
    @ref TextLayer::Shared::Configuration::setFlags(),
    @ref TextLayer::Shared::flags()
*/
enum class TextLayerSharedFlag: UnsignedByte {
    /**
     * Distance field rendering. If enabled, the
     * @ref TextLayerCommonStyleUniform::smoothness,
     * @ref TextLayerStyleUniform::outlineColor,
     * @ref TextLayerStyleUniform::outlineWidth,
     * @ref TextLayerStyleUniform::edgeOffset and
     * @ref TextLayerStyleUniform::smoothness get used as well. Enabled
     * implicitly when the shared state is constructed with
     * @ref TextLayerGL::Shared::Shared(Text::DistanceFieldGlyphCacheArrayGL&, const Configuration&)
     * or @ref TextLayerGL::Shared::Shared(Text::DistanceFieldGlyphCacheArrayGL&&, const Configuration&),
     * on the other hand is expected to not be set when using the
     * @ref TextLayerGL::Shared::Shared(Text::GlyphCacheArrayGL&, const Configuration&)
     * or @ref TextLayerGL::Shared::Shared(Text::GlyphCacheArrayGL&&, const Configuration&)
     * constructors.
     */
    DistanceField = 1 << 0
};

/**
@brief Text layer shared state flag
@m_since_latest

@see @ref TextLayer::Shared::Configuration::setFlags(),
    @ref TextLayer::Shared::flags()
*/
typedef Containers::EnumSet<TextLayerSharedFlag> TextLayerSharedFlags;

CORRADE_ENUMSET_OPERATORS(TextLayerSharedFlags)

/**
@debugoperatorenum{TextLayerSharedFlag}
@m_since_latest
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, TextLayerSharedFlag value);

/**
@debugoperatorenum{TextLayerSharedFlags}
@m_since_latest
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, TextLayerSharedFlags value);

/**
@brief Shared state for the text layer

Contains a set of fonts and a glyph cache used by all of them. In order to use
the layer it's expected that at least one font was added with @ref addFont().
In order to update or draw the layer it's expected that @ref setStyle() was
called.

Pre-filling the glyph cache with appropriate glyphs for a particular font is
the user responsibility, the implementation currently won't perform that on its
own, neither it does any on-demand cache filling.
*/
class MAGNUM_UI_EXPORT TextLayer::Shared: public AbstractVisualLayer::Shared {
    public:
        class Configuration;

        /**
         * @brief Style uniform count
         *
         * Size of the style uniform buffer excluding dynamic styles. May or
         * may not be the same as @ref styleCount(). For dynamic styles, the
         * style uniform count is the same as @ref dynamicStyleCount().
         * @see @ref Configuration::Configuration(UnsignedInt, UnsignedInt),
         *      @ref setStyle(), @ref editingStyleUniformCount()
         */
        UnsignedInt styleUniformCount() const;

        /**
         * @brief Editing style count
         *
         * Count of editing styles used by all layers referencing this
         * @ref Shared instance. If dynamic styles include editing styles, IDs
         * greater than @ref editingStyleCount() are then dynamic editing
         * styles, with their count being twice of @ref dynamicStyleCount().
         * @see @ref editingStyleUniformCount(), @ref hasEditingStyles(),
         *      @ref Configuration::setEditingStyleCount(),
         *      @ref setEditingStyle()
         */
        UnsignedInt editingStyleCount() const;

        /**
         * @brief Editing style uniform count
         *
         * Size of the editing style uniform buffer excluding dynamic styles.
         * May or may not be the same as @ref styleCount(). For dynamic styles,
         * if they include editing styles, the style uniform count is twice
         * @ref dynamicStyleCount(), as each dynamic style can have at most two
         * associated editing styles --- one for the cursor and one for the
         * selection.
         * @see @ref editingStyleCount(), @ref hasEditingStyles()
         *      @ref Configuration::setEditingStyleCount(),
         *      @ref setEditingStyle()
         */
        UnsignedInt editingStyleUniformCount() const;

        /**
         * @brief Whether the layer has any editing styles
         *
         * Returns @cpp true @ce if either @ref editingStyleCount() is non-zero
         * or @ref dynamicStyleCount() is non-zero and editing styles were
         * explicitly enabled with @ref Configuration::setDynamicStyleCount(UnsignedInt, bool).
         */
        bool hasEditingStyles() const;

        /**
         * @brief Shared layer flags
         *
         * @see @ref TextLayer::flags(), @ref TextLayer::flags(DataHandle) const
         */
        TextLayerSharedFlags flags() const;

        /**
         * @brief Glyph cache instance
         *
         * The actual type depends on the concrete layer implementation, in
         * case of @ref TextLayerGL it's either @ref Text::GlyphCacheArrayGL or
         * @ref Text::DistanceFieldGlyphCacheArrayGL based on whether
         * @ref flags() contain @ref TextLayerSharedFlag::DistanceField.
         */
        Text::AbstractGlyphCache& glyphCache();
        const Text::AbstractGlyphCache& glyphCache() const; /**< @overload */

        /**
         * @brief Count of added fonts
         *
         * Can be at most 32768.
         */
        std::size_t fontCount() const;

        /**
         * @brief Whether a font handle is valid
         *
         * A handle is valid if it has been returned from @ref addFont()
         * before. Note that the implementation has no way to distinguish
         * between handles returned from different @ref TextLayer::Shared
         * instances. A handle from another shared state instance thus may or
         * may not be treated as valid by another instance, and it's the user
         * responsibility to not mix them up. For @ref FontHandle::Null always
         * returns @cpp false @ce.
         */
        bool isHandleValid(FontHandle handle) const;

        /**
         * @brief Add a font
         * @param font      Font instance
         * @param size      Size in points at which to render the font
         * @return New font handle
         *
         * Expects that @ref glyphCache() is set and contains @p font. Doesn't
         * perform any operation with the glyph cache, pre-filling is left to
         * the caller. The returned handle can be subsequently assigned to a
         * particular style via @ref setStyle(), or passed to
         * @ref TextProperties::setFont() to create a text with a font that's
         * different from the one assigned to a particular style.
         *
         * It's the caller responsibility to ensure @p font stays in scope for
         * as long as the shared state is used. Use the
         * @ref addFont(Containers::Pointer<Text::AbstractFont>&&, Float)
         * overload to make the shared state take over the font instance.
         * @see @ref addInstancelessFont(), @ref Text-AbstractFont-font-size
         */
        FontHandle addFont(Text::AbstractFont& font, Float size);

        /**
         * @brief Add a font and take over its ownership
         * @param font      Font instance
         * @param size      Size in points at which to render the font
         * @return New font handle
         *
         * Like @ref addFont(Text::AbstractFont&, Float), but the shared state
         * takes over the font ownership. You can access the instance using
         * @ref font() later. It's the caller responsibility to ensure the
         * plugin manager the font is coming from stays in scope for as long as
         * the shared state is used.
         * @see @ref Text-AbstractFont-font-size
         */
        FontHandle addFont(Containers::Pointer<Text::AbstractFont>&& font, Float size);

        /**
         * @brief Add an instance-less font
         * @param glyphCacheFontId  ID of the font in the glyph cache
         * @param scale             Scale to apply to glyph rectangles coming
         *      from the glyph cache
         * @return New font handle
         *
         * Makes it possible to render arbitrary custom glyphs added to the
         * glyph cache, such as icons or other image data.
         *
         * Expects that @ref glyphCache() is set, @p glyphCacheFontId is less
         * than @ref Text::AbstractGlyphCache::fontCount() and
         * @ref Text::AbstractGlyphCache::fontPointer() is @cpp nullptr @ce for
         * @p glyphCacheFontId. Doesn't perform any operation with the glyph
         * cache, the caller is expected to populate the cache with desired
         * data. The resulting handle can be subsequently assigned to a
         * particular style via @ref setStyle(), or passed to
         * @ref TextProperties::setFont(). The instance-less font however can
         * be only used with @ref createGlyph() or @ref setGlyph() that takes a
         * glyph ID, not with @ref create() or @ref setText().
         * @see @ref hasFontInstance(), @ref addFont()
         */
        FontHandle addInstancelessFont(UnsignedInt glyphCacheFontId, Float scale);

        /**
         * @brief ID of a font in a glyph cache
         *
         * Returns ID under which given font glyphs are stored in the
         * @ref glyphCache(). For fonts with an instance is equivalent to the
         * ID returned from @ref Text::AbstractGlyphCache::findFont(). Expects
         * that @p handle is valid.
         * @see @ref isHandleValid(FontHandle) const, @ref hasFontInstance()
         */
        UnsignedInt glyphCacheFontId(FontHandle handle) const;

        /**
         * @brief Whether a font has an instance
         *
         * Returns @cpp true @ce if the font was created with @ref addFont(),
         * @cpp false @ce if with @ref addInstancelessFont(). Expects that
         * @p handle is valid.
         * @see @ref isHandleValid(FontHandle) const
         */
        bool hasFontInstance(FontHandle handle) const;

        /**
         * @brief Font instance
         *
         * Expects that @p handle is valid and has a font instance, i.e. was
         * created with @ref addFont() and not @ref addInstancelessFont().
         * @see @ref isHandleValid(FontHandle) const, @ref hasFontInstance()
         */
        Text::AbstractFont& font(FontHandle handle);
        const Text::AbstractFont& font(FontHandle handle) const; /**< @overload */

        /**
         * @brief Set style data with implicit mapping between styles and uniforms
         * @param commonUniform Common style uniform data
         * @param uniforms      Style uniforms
         * @param fonts         Font handles corresponding to style uniforms
         * @param alignments    Text alignment corresponding to style uniforms
         * @param features      Font feature data for all styles
         * @param featureOffsets  Offsets into @p features corresponding to
         *      style uniforms
         * @param featureCounts  Counts into @p features corresponding to style
         *      uniforms
         * @param cursorStyles  Cursor style IDs corresponding to style
         *      uniforms
         * @param selectionStyles  Selection style IDs corresponding to style
         *      uniforms
         * @param paddings      Padding inside the node in UI units, in order
         *      left, top, right, bottom, corresponding to style uniforms
         * @return Reference to self (for method chaining)
         *
         * The @p uniforms view is expected to have the same size as
         * @ref styleUniformCount(), the @p fonts and @p alignments views the
         * same size as @ref styleCount(). All font handles are expected to be
         * either @ref FontHandle::Null or valid.
         *
         * All @p alignments values are expected to not be `*GlyphBounds` as
         * the implementation can only align based on font metrics and cursor
         * position, not actual glyph bounds. In addition to the behavior
         * described in particular @ref Text::Alignment values, the aligned
         * origin is then further offset respectively to the node the text is
         * attached to. In particular:
         *
         *  -   `*Left` makes the horizontal origin aligned with node left side
         *  -   `*Right` makes the horizontal origin aligned with node right
         *      side
         *  -   `*Center` makes the horizontal origin aligned with horizontal
         *      node center, and additionally `*Integral` rounds the horizontal
         *      offset inside the node to whole units
         *  -   `*Top` makes the vertical origin aligned with node top side
         *  -   `*Bottom` makes the vertical origin aligned with node bottom
         *      side
         *  -   `*Line` and `*Middle` makes the vertical origin aligned with
         *      vertical node center, and additionally `*Integral` rounds the
         *      vertical offset inside the node to whole units. The difference
         *      between the two is that multiple texts with different font
         *      metrics get their line positions matched with `*Line`, while
         *      `*Middle` makes the midpoint between font ascent and descent
         *      matched.
         *
         * The @p featureOffsets and @p featureCounts views are expected to
         * both either have a size of @ref styleCount(), or both be empty. If
         * non-empty, they contain offsets and counts into the @p features view
         * for each style, with the ranges expected to fit into the view size.
         * The ranges are allowed to overlap, i.e. multiple styles can share
         * the same default feature set. If empty, the @p features view is
         * expected to be empty as well and no default font features are
         * specified for any styles. Features coming from @ref TextProperties
         * are *appended* after these, making it possible to override them.
         *
         * The @p cursorStyles and @p selectionStyles views are expected to
         * either have the same size as @ref styleCount() or be empty. If
         * non-empty, they're expected to either contain non-negative indices
         * less than @ref editingStyleCount() or @cpp -1 @ce denoting that
         * given style doesn't have the cursor or selection visible. If empty,
         * no styles have cursor or selection visible.
         *
         * The @p paddings view is expected to either have the same size
         * as @ref styleCount() or be empty, in which case all paddings are
         * implicitly zero.
         *
         * Can only be called if @ref styleUniformCount() and @ref styleCount()
         * were set to the same value in @ref Configuration passed to the
         * constructor, otherwise you have to additionally provide a mapping
         * from styles to uniforms using
         * @ref setStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const FontHandle>&, const Containers::StridedArrayView1D<const Text::Alignment>&, Containers::ArrayView<const TextFeatureValue>, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const Int>&, const Containers::StridedArrayView1D<const Int>&, const Containers::StridedArrayView1D<const Vector4>&)
         * instead.
         *
         * Calling this function causes @ref LayerState::NeedsDataUpdate to be
         * set on all layers that are constructed using this shared instance.
         * If @ref dynamicStyleCount() is non-zero,
         * @ref LayerState::NeedsCommonDataUpdate is set as well to trigger an
         * upload of changed dynamic style uniform data.
         * @see @ref isHandleValid(FontHandle) const
         */
        Shared& setStyle(const TextLayerCommonStyleUniform& commonUniform, Containers::ArrayView<const TextLayerStyleUniform> uniforms, const Containers::StridedArrayView1D<const FontHandle>& fonts, const Containers::StridedArrayView1D<const Text::Alignment>& alignments, Containers::ArrayView<const TextFeatureValue> features, const Containers::StridedArrayView1D<const UnsignedInt>& featureOffsets, const Containers::StridedArrayView1D<const UnsignedInt>& featureCounts, const Containers::StridedArrayView1D<const Int>& cursorStyles, const Containers::StridedArrayView1D<const Int>& selectionStyles, const Containers::StridedArrayView1D<const Vector4>& paddings);
        /** @overload */
        Shared& setStyle(const TextLayerCommonStyleUniform& commonUniform, std::initializer_list<TextLayerStyleUniform> uniforms, std::initializer_list<FontHandle> fonts, std::initializer_list<Text::Alignment> alignments, std::initializer_list<TextFeatureValue> features, std::initializer_list<UnsignedInt> featureOffsets, std::initializer_list<UnsignedInt> featureCounts, std::initializer_list<Int> cursorStyles, std::initializer_list<Int> selectionStyles, std::initializer_list<Vector4> paddings);

        /**
         * @brief Set style data
         * @param commonUniform     Common style uniform data
         * @param uniforms          Style uniforms
         * @param styleToUniform    Style to style uniform mapping
         * @param styleFonts        Per-style font handles
         * @param styleAlignments   Per-style text alignment
         * @param styleFeatures     Font feature data for all styles
         * @param styleFeatureOffsets  Per-style offsets into @p styleFeatures
         * @param styleFeatureCounts  Per-style counts into @p styleFeatures
         * @param styleCursorStyles  Per-style cursor style IDs
         * @param styleSelectionStyles  Per-style selection style IDs
         * @param stylePaddings     Per-style padding inside the node in UI
         *      units, in order left, top, right, bottom
         * @return Reference to self (for method chaining)
         *
         * The @p uniforms view is expected to have the same size as
         * @ref styleUniformCount(), the @p styleToUniform, @p styleFonts and
         * @p styleAlignments views the same size as @ref styleCount(). All
         * uniform indices are expected to be less than
         * @ref styleUniformCount(), all font handles are expected to be either
         * @ref FontHandle::Null or valid, all @p styleAlignments values are
         * expected to not be `*GlyphBounds` as the implementation can only
         * align based on font metrics and cursor position, not actual glyph
         * bounds. Detailed behavior of all alignment values is described in
         * the @ref setStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>, const Containers::StridedArrayView1D<const FontHandle>&, const Containers::StridedArrayView1D<const Text::Alignment>&, Containers::ArrayView<const TextFeatureValue>, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const Int>&, const Containers::StridedArrayView1D<const Int>&, const Containers::StridedArrayView1D<const Vector4>&) "above overload".
         *
         * The @p styleFeatureOffsets and @p styleFeatureCounts views are
         * expected to both either have a size of @ref styleCount(), or both be
         * empty. If non-empty, they contain offsets and counts into the
         * @p styleFeatures view for each style, with the ranges expected to
         * fit into the view size. The ranges are allowed to overlap, i.e.
         * multiple styles can share the same default feature set. If empty,
         * the @p styleFeatures view is expected to be empty as well and no
         * default font features are specified for any styles. Features coming
         * from @ref TextProperties are *appended* after these, making it
         * possible to override them.
         *
         * The @p styleCursorStyles and @p styleSelectionStyles views are
         * expected to either have the same size as @ref styleCount() or be
         * empty. If non-empty, they're expected to either contain non-negative
         * indices less than @ref editingStyleCount() or @cpp -1 @ce denoting
         * that given style doesn't have the cursor or selection visible. If
         * empty, no styles have cursor or selection visible.
         *
         * The @p stylePaddings view is expected to either have the same size
         * as @ref styleCount() or be empty, in which case all paddings are
         * implicitly zero.
         *
         * Value of @cpp styleToUniform[i] @ce should give back an index into
         * the @p uniforms array for style @cpp i @ce. If
         * @ref styleUniformCount() and @ref styleCount() is the same and the
         * mapping is implicit, you can use the
         * @ref setStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>, const Containers::StridedArrayView1D<const FontHandle>&, const Containers::StridedArrayView1D<const Text::Alignment>&, Containers::ArrayView<const TextFeatureValue>, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const Int>&, const Containers::StridedArrayView1D<const Int>&, const Containers::StridedArrayView1D<const Vector4>&)
         * convenience overload instead.
         *
         * Calling this function causes @ref LayerState::NeedsDataUpdate to be
         * set on all layers that are constructed using this shared instance.
         * If @ref dynamicStyleCount() is non-zero,
         * @ref LayerState::NeedsCommonDataUpdate is set as well to trigger an
         * upload of changed dynamic style uniform data.
         * @see @ref isHandleValid(FontHandle) const
         */
        Shared& setStyle(const TextLayerCommonStyleUniform& commonUniform, Containers::ArrayView<const TextLayerStyleUniform> uniforms, const Containers::StridedArrayView1D<const UnsignedInt>& styleToUniform, const Containers::StridedArrayView1D<const FontHandle>& styleFonts, const Containers::StridedArrayView1D<const Text::Alignment>& styleAlignments, Containers::ArrayView<const TextFeatureValue> styleFeatures, const Containers::StridedArrayView1D<const UnsignedInt>& styleFeatureOffsets, const Containers::StridedArrayView1D<const UnsignedInt>& styleFeatureCounts, const Containers::StridedArrayView1D<const Int>& styleCursorStyles, const Containers::StridedArrayView1D<const Int>& styleSelectionStyles, const Containers::StridedArrayView1D<const Vector4>& stylePaddings);
        /** @overload */
        Shared& setStyle(const TextLayerCommonStyleUniform& commonUniform, std::initializer_list<TextLayerStyleUniform> uniforms, std::initializer_list<UnsignedInt> styleToUniform, std::initializer_list<FontHandle> styleFonts, std::initializer_list<Text::Alignment> styleAlignments, std::initializer_list<TextFeatureValue> styleFeatures, std::initializer_list<UnsignedInt> styleFeatureOffsets, std::initializer_list<UnsignedInt> styleFeatureCounts, std::initializer_list<Int> styleCursorStyles, std::initializer_list<Int> styleSelectionStyles, std::initializer_list<Vector4> stylePaddings);

        /**
         * @brief Set editing style data with implicit mapping between styles and uniforms
         * @param commonUniform     Common style uniform data
         * @param uniforms          Style uniforms
         * @param textUniforms      Base style uniform IDs to use for selected
         *      portions of the text. Not used if given style is for a cursor.
         * @param paddings          Paddings outside the cursor or selection
         *      rectangle in UI units, in order begin, top, end, bottom
         *      corresponding to style uniforms
         * @return Reference to self (for method chaining)
         *
         * The @p uniforms view is expected to have the same size as
         * @ref editingStyleUniformCount(), the @p paddings views the same size
         * as @ref editingStyleCount().
         *
         * The @p textUniforms are expected to either have the same size as
         * @ref editingStyleCount() or be empty. The indices point into the
         * uniform array supplied by @ref setStyle(), in which case selected
         * portions of the text are switched to be drawn with given uniform ID
         * instead, or are @cpp -1 @ce, in which case the original style
         * uniform ID is used unchanged. If the view is empty, it's the same as
         * all values being @cpp -1 @ce.
         *
         * The order in which @p paddings get used depends on direction of
         * given text --- for @ref Text::ShapeDirection::LeftToRight, the order
         * is left, top, right, bottom; for
         * @relativeref{Text::ShapeDirection,RightToLeft} the order is right,
         * top, left, bottom. This makes it possible to for example position
         * the cursor to always overlap the *next* character depending on the
         * direction.
         *
         * Can only be called if @ref editingStyleUniformCount() and
         * @ref editingStyleCount() were set to the same value in
         * @ref Configuration::setEditingStyleCount(), otherwise you have
         * to additionally provide a mapping from styles to uniforms using
         * @ref setEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const Int>&, const Containers::StridedArrayView1D<const Vector4>&)
         * instead.
         *
         * Note that while for *selection* styles the paddings are merely a way
         * to visually fine-tune the appearance, for *cursor* styles the
         * padding has to be non-zero horizontally to actually be visible at
         * all. This is also why, unlike @ref setStyle(), the @p paddings can't
         * be left empty.
         */
        Shared& setEditingStyle(const TextLayerCommonEditingStyleUniform& commonUniform, Containers::ArrayView<const TextLayerEditingStyleUniform> uniforms, const Containers::StridedArrayView1D<const Int>& textUniforms, const Containers::StridedArrayView1D<const Vector4>& paddings);
        /** @overload */
        Shared& setEditingStyle(const TextLayerCommonEditingStyleUniform& commonUniform, std::initializer_list<TextLayerEditingStyleUniform> uniforms, std::initializer_list<Int> textUniforms, std::initializer_list<Vector4> paddings);

        /**
         * @brief Set editing style data
         * @param commonUniform     Common style uniform data
         * @param uniforms          Style uniforms
         * @param styleToUniform    Style to style uniform mapping
         * @param styleTextUniforms  Base style uniform IDs to use for selected
         *      portions of the text. Not used if given style is for a cursor.
         * @param stylePaddings     Per-style margins outside the cursor or
         *      selection rectangle in UI units, in order left, top, right,
         *      bottom
         * @return Reference to self (for method chaining)
         *
         * The @p uniforms view is expected to have the same size as
         * @ref editingStyleUniformCount(), the @p styleToUniform and
         * @p stylePaddings views the same size as @ref editingStyleCount().
         * All @p styleToUniform indices are expected to be less than
         * @ref editingStyleUniformCount().
         *
         * Value of @cpp styleToUniform[i] @ce should give back an index into
         * the @p uniforms array for style @cpp i @ce. If
         * @ref editingStyleUniformCount() and @ref editingStyleCount() is the
         * same and the mapping is implicit, you can use the
         * @ref setEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>, const Containers::StridedArrayView1D<const Int>&, const Containers::StridedArrayView1D<const Vector4>&)
         * convenience overload instead.
         *
         * The @p styleTextUniforms are expected to either have the same size
         * as @ref editingStyleCount() or be empty. If non-empty, they're
         * expected to either be non-negative indices less than
         * @ref styleUniformCount(), pointing into the uniform array supplied
         * by @ref setStyle() in which case selected portions of the text are
         * switched to be drawn with given uniform ID instead, or be
         * @cpp -1 @ce, in which case the original style uniform ID is used
         * unchanged. If the view is empty, it's the same as all values being
         * @cpp -1 @ce.
         *
         * Note that while for *selection* styles the paddings are merely a way
         * to visually fine-tune the appearance, for *cursor* styles the
         * paddings have to be non-zero horizontally to actually be visible at
         * all. This is also why, unlike @ref setStyle(), the @p stylePaddings
         * can't be left empty.
         */
        Shared& setEditingStyle(const TextLayerCommonEditingStyleUniform& commonUniform, Containers::ArrayView<const TextLayerEditingStyleUniform> uniforms, const Containers::StridedArrayView1D<const UnsignedInt>& styleToUniform, const Containers::StridedArrayView1D<const Int>& styleTextUniforms, const Containers::StridedArrayView1D<const Vector4>& stylePaddings);
        /** @overload */
        Shared& setEditingStyle(const TextLayerCommonEditingStyleUniform& commonUniform, std::initializer_list<TextLayerEditingStyleUniform> uniforms, std::initializer_list<UnsignedInt> styleToUniform, std::initializer_list<Int> styleTextUniforms, std::initializer_list<Vector4> stylePaddings);

        /* Overloads to remove a WTF factor from method chaining order */
        #ifndef DOXYGEN_GENERATING_OUTPUT
        MAGNUMEXTRAS_UI_ABSTRACTVISUALLAYER_SHARED_SUBCLASS_IMPLEMENTATION()
        #endif

    protected:
        struct State;

    #ifdef DOXYGEN_GENERATING_OUTPUT
    private:
    #else
    protected:
    #endif
        friend TextLayer;
        friend TextLayerStyleAnimator;

        MAGNUM_UI_LOCAL explicit Shared(Containers::Pointer<State>&& state);
        /* Used by tests to avoid having to include / allocate the state */
        explicit Shared(Text::AbstractGlyphCache& cache, const Configuration& configuration);
        /* Can't be MAGNUM_UI_LOCAL, used by tests */
        explicit Shared(NoCreateT) noexcept;

    private:
        MAGNUM_UI_LOCAL void setStyleInternal(const TextLayerCommonStyleUniform& commonUniform, Containers::ArrayView<const TextLayerStyleUniform> uniforms, const Containers::StridedArrayView1D<const FontHandle>& styleFonts, const Containers::StridedArrayView1D<const Text::Alignment>& styleAlignments, Containers::ArrayView<const TextFeatureValue> styleFeatures, const Containers::StridedArrayView1D<const UnsignedInt>& styleFeatureOffsets, const Containers::StridedArrayView1D<const UnsignedInt>& styleFeatureCounts, const Containers::StridedArrayView1D<const Int>& styleCursorStyles, const Containers::StridedArrayView1D<const Int>& styleSelectionStyles, const Containers::StridedArrayView1D<const Vector4>& stylePaddings);
        MAGNUM_UI_LOCAL void setEditingStyleInternal(const TextLayerCommonEditingStyleUniform& commonUniform, Containers::ArrayView<const TextLayerEditingStyleUniform> uniforms, const Containers::StridedArrayView1D<const Int>& styleTextUniforms, const Containers::StridedArrayView1D<const Vector4>& stylePaddings);

        /* The items are guaranteed to have the same size as
           styleUniformCount(). Called only if there are no dynamic styles,
           otherwise the data are copied to internal arrays to be subsequently
           combined with dynamic uniforms and uploaded together in doDraw(). */
        virtual void doSetStyle(const TextLayerCommonStyleUniform& commonUniform, Containers::ArrayView<const TextLayerStyleUniform> uniforms) = 0;
        /* The items are guaranteed to have the same size as
           editingStyleUniformCount(). Called only if there are no dynamic
           styles, otherwise the data are copied to internal arrays to be
           subsequently combined with dynamic uniforms and uploaded together in
           doDraw(). */
        virtual void doSetEditingStyle(const TextLayerCommonEditingStyleUniform& commonUniform, Containers::ArrayView<const TextLayerEditingStyleUniform> uniforms) = 0;
};

/**
@brief Configuration of a base layer shared state

@see @ref TextLayerGL::Shared::Shared(Text::GlyphCacheArrayGL&, const Configuration&)
*/
class MAGNUM_UI_EXPORT TextLayer::Shared::Configuration {
    public:
        /**
         * @brief Constructor
         *
         * The @p styleUniformCount parameter specifies the size of the uniform
         * array, @p styleCount then the number of distinct styles to use for
         * drawing. The sizes are independent in order to allow styles with
         * different fonts or paddings share the same uniform data. Either both
         * @p styleUniformCount and @p styleCount is expected to be non-zero,
         * or both zero with a non-zero dynamic style count specified with
         * @ref setDynamicStyleCount(). Style data are then set with
         * @ref setStyle(), dynamic style data then with
         * @ref TextLayer::setDynamicStyle().
         * @see @ref setEditingStyleCount(), @ref setEditingStyle()
         */
        explicit Configuration(UnsignedInt styleUniformCount, UnsignedInt styleCount);

        /**
         * @brief Construct with style uniform count being the same as style count
         *
         * Equivalent to calling @ref Configuration(UnsignedInt, UnsignedInt)
         * with both parameters set to @p styleCount.
         */
        explicit Configuration(UnsignedInt styleCount): Configuration{styleCount, styleCount} {}

        /** @brief Style uniform count */
        UnsignedInt styleUniformCount() const { return _styleUniformCount; }

        /** @brief Style count */
        UnsignedInt styleCount() const { return _styleCount; }

        /** @brief Editing style uniform count */
        UnsignedInt editingStyleUniformCount() const { return _editingStyleUniformCount; }

        /** @brief Editing style count */
        UnsignedInt editingStyleCount() const { return _editingStyleCount; }

        /**
         * @brief Set editing style count
         * @return Reference to self (for method chaining)
         *
         * The @p uniformCount parameter specifies the size of the uniform
         * array, @p count then the number of distinct styles to use for cursor
         * and selection drawing. The sizes are independent in order to allow
         * editing styles with different paddings share the same uniform data.
         * Both @p uniformCount and @p count is expected to either be zero or
         * both non-zero. Style data are then set with @ref setEditingStyle().
         * Initial count is @cpp 0 @ce, which means neither cursor nor
         * selection is drawn.
         *
         * If the style count passed to the constructor is @cpp 0 @ce, the
         * editing style count is expected to be @cpp 0 @ce as well.
         * @see @ref TextDataFlag::Editable
         */
        Configuration& setEditingStyleCount(UnsignedInt uniformCount, UnsignedInt count);

        /**
         * @brief Set editing style count with style uniform count being the same as style count
         *
         * Equivalent to calling @ref setEditingStyleCount(UnsignedInt, UnsignedInt)
         * with both parameters set to @p count.
         */
        Configuration& setEditingStyleCount(UnsignedInt count) {
            return setEditingStyleCount(count, count);
        }

        /** @brief Dynamic style count */
        UnsignedInt dynamicStyleCount() const { return _dynamicStyleCount; }

        /**
         * @brief Whether any editing styles are enabled
         *
         * Returns @cpp true @ce if either @ref setEditingStyleCount() was
         * called with a non-zero value or @ref setDynamicStyleCount(UnsignedInt, bool)
         * was called with a non-zero value and @p withEditingStyles explicitly
         * enabled, @cpp false @ce otherwise.
         */
        bool hasEditingStyles() const {
            return _dynamicEditingStyles || _editingStyleCount;
        }

        /**
         * @brief Set dynamic style count
         * @return Reference to self (for method chaining)
         *
         * Initial count is @cpp 0 @ce. Dynamic editing styles are implicitly
         * enabled only if editing style count is non-zero, use the
         * @ref setDynamicStyleCount(UnsignedInt, bool) overload to control
         * their presence in case editing style count is zero.
         * @see @ref Configuration(UnsignedInt, UnsignedInt),
         *      @ref AbstractVisualLayer::allocateDynamicStyle(),
         *      @ref AbstractVisualLayer::dynamicStyleUsedCount()
         */
        Configuration& setDynamicStyleCount(UnsignedInt count) {
            return setDynamicStyleCount(count, false);
        }

        /**
         * @brief Set dynamic style count and explicitly enable dynamic editing styles
         * @return Reference to self (for method chaining)
         *
         * If editing style count is zero, setting @p withEditingStyles to
         * @cpp true @ce will include them for dynamic styles. Otherwise, if
         * editing style count is zero, they're not included, meaning it's not
         * possible to call @ref TextLayer::setDynamicStyleWithCursor(),
         * @ref TextLayer::setDynamicStyleWithSelection() or
         * @ref TextLayer::setDynamicStyleWithCursorSelection(), only
         * @ref TextLayer::setDynamicStyle(). If editing style count is
         * non-zero, dynamic editing styles are included always and
         * @p withEditingStyles is ignored.
         * @see @ref Configuration(UnsignedInt, UnsignedInt),
         *      @ref AbstractVisualLayer::allocateDynamicStyle(),
         *      @ref AbstractVisualLayer::dynamicStyleUsedCount()
         */
        Configuration& setDynamicStyleCount(UnsignedInt count, bool withEditingStyles);

        /** @brief Shared layer flags */
        TextLayerSharedFlags flags() const { return _flags; }

        /**
         * @brief Set shared layer flags
         * @return Reference to self (for method chaining)
         *
         * By default no flags are set. The @ref TextLayerFlags get set during
         * layer construction instead, such as in
         * @ref TextLayerGL::TextLayerGL(LayerHandle, Shared&, TextLayerFlags).
         * @see @ref addFlags(), @ref clearFlags()
         */
        Configuration& setFlags(TextLayerSharedFlags flags) {
            _flags = flags;
            return *this;
        }

        /**
         * @brief Add flags
         * @return Reference to self (for method chaining)
         *
         * Calls @ref setFlags() with the existing flags ORed with @p flags.
         * Useful for preserving previously set flags.
         * @see @ref clearFlags()
         */
        Configuration& addFlags(TextLayerSharedFlags flags) {
            return setFlags(_flags|flags);
        }

        /**
         * @brief Clear flags
         * @return Reference to self (for method chaining)
         *
         * Calls @ref setFlags() with the existing flags ANDed with the inverse
         * of @p flags. Useful for removing a subset of previously set flags.
         * @see @ref addFlags()
         */
        Configuration& clearFlags(TextLayerSharedFlags flags) {
            return setFlags(_flags & ~flags);
        }

    private:
        UnsignedInt _styleUniformCount, _styleCount;
        UnsignedInt _editingStyleUniformCount = 0, _editingStyleCount = 0;
        UnsignedInt _dynamicStyleCount = 0;
        TextLayerSharedFlags _flags;
        bool _dynamicEditingStyles = false;
};

inline TextLayer::Shared& TextLayer::shared() {
    return static_cast<Shared&>(AbstractVisualLayer::shared());
}

inline const TextLayer::Shared& TextLayer::shared() const {
    return static_cast<const Shared&>(AbstractVisualLayer::shared());
}

}}

#endif
