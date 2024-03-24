#ifndef Magnum_Whee_TextLayer_h
#define Magnum_Whee_TextLayer_h
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

/** @file
 * @brief Class @ref Magnum::Whee::TextLayer, struct @ref Magnum::Whee::TextLayerCommonStyleUniform, @ref Magnum::Whee::TextLayerStyleUniform, enum @ref Magnum::Whee::FontHandle, function @ref Magnum::Whee::fontHandle(), @ref Magnum::Whee::fontHandleId(), @ref Magnum::Whee::fontHandleGeneration()
 * @m_since_latest
 */

#include <Corrade/Containers/StringView.h> /* for templated create() overloads */
#include <Corrade/PluginManager/Manager.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Text/Text.h>

#include "Magnum/Whee/AbstractVisualLayer.h"

namespace Magnum { namespace Whee {

/**
@brief Properties common to all @ref TextLayer style uniforms
@m_since_latest

Together with one or more @ref TextLayerStyleUniform instances contains style
properties that are used by the @ref TextLayer shaders to draw the layer data,
packed in a form that allows direct usage in uniform buffers. Is uploaded
using @ref TextLayer::Shared::setStyle(), style data that aren't used by the
shader are passed to the function separately.

Currently this is just a placeholder with no properties.
*/
struct alignas(4) TextLayerCommonStyleUniform {
    /** @brief Construct with default values */
    constexpr explicit TextLayerCommonStyleUniform(DefaultInitT = DefaultInit) noexcept {}

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
     * @}
     */

    /** @todo remove the alignas once actual attributes are here */
    #ifndef DOXYGEN_GENERATING_OUTPUT
    Int:32;
    Int:32;
    Int:32;
    Int:32;
    #endif
};

/**
@brief @ref TextLayer style uniform
@m_since_latest

Instances of this class together with @ref TextLayerCommonStyleUniform contain
style properties that are used by the @ref TextLayer shaders to draw the layer
data, packed in a form that allows direct usage in uniform buffers. Total count
of styles is specified with the
@ref TextLayer::Shared::Configuration::Configuration() constructor, uniforms
are then uploaded using @ref TextLayer::Shared::setStyle(), style data that
aren't used by the shader are passed to the function separately.
*/
struct TextLayerStyleUniform {
    /** @brief Construct with default values */
    constexpr explicit TextLayerStyleUniform(DefaultInitT = DefaultInit) noexcept: color{1.0f} {}

    /** @brief Constructor */
    constexpr /*implicit*/ TextLayerStyleUniform(const Color4& color): color{color} {}

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
    TextLayerStyleUniform& setColor(const Color4& color) {
        this->color = color;
        return *this;
    }

    /**
     * @}
     */

    /**
     * @brief Color
     *
     * Default value is @cpp 0xffffffff_srgbf @ce.
     */
    Color4 color;
};

/* Unlike DataHandle, NodeHandle etc., which are global to the whole Whee
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
MAGNUM_WHEE_EXPORT Debug& operator<<(Debug& debug, FontHandle value);

/**
@brief Compose a font handle from an ID and a generation
@m_since_latest

Expects that the ID fits into 20 bits and the generation into 12 bits. Use
@ref fontHandleId() and @ref fontHandleGeneration() for an inverse operation.
*/
constexpr FontHandle fontHandle(UnsignedInt id, UnsignedInt generation) {
    return (CORRADE_CONSTEXPR_DEBUG_ASSERT(id < (1 << Implementation::FontHandleIdBits) && generation < (1 << Implementation::FontHandleGenerationBits),
        "Whee::fontHandle(): expected index to fit into" << Implementation::FontHandleIdBits << "bits and generation into" << Implementation::FontHandleGenerationBits << Debug::nospace << ", got" << Debug::hex << id << "and" << Debug::hex << generation), FontHandle(id|(generation << Implementation::FontHandleIdBits)));
}

/**
@brief Extract ID from a font handle
@m_since_latest

For @ref FontHandle::Null returns @cpp 0 @ce. Use @ref fontHandleGeneration()
for extracting the generation and @ref fontHandle() for an inverse operation.
*/
constexpr UnsignedInt fontHandleId(FontHandle handle) {
    return UnsignedInt(handle) & ((1 << Implementation::FontHandleIdBits) - 1);
}

/**
@brief Extract generation from a font handle
@m_since_latest

For @ref FontHandle::Null returns @cpp 0 @ce. Use @ref fontHandleId() for
extracting the ID and @ref fontHandle() for an inverse operation.
*/
constexpr UnsignedInt fontHandleGeneration(FontHandle handle) {
    return UnsignedInt(handle) >> Implementation::FontHandleIdBits;
}

/**
@brief Text layer
@m_since_latest

Draws text laid out using the @ref Text library. You'll most likely instantiate
the class through @ref TextLayerGL, which contains a concrete OpenGL
implementation.
@see @ref UserInterface::textLayer(),
    @ref UserInterface::setTextLayerInstance(), @ref StyleFeature::TextLayer
*/
class MAGNUM_WHEE_EXPORT TextLayer: public AbstractVisualLayer {
    public:
        class Shared;

        /**
         * @brief Shared state used by this layer
         *
         * Reference to the instance passed to @ref TextLayerGL::TextLayerGL(LayerHandle, Shared&).
         */
        inline Shared& shared();
        inline const Shared& shared() const; /**< @overload */

        /**
         * @brief Create a text
         * @param style         Style index
         * @param text          Text to render
         * @param properties    Text properties
         * @param node          Node to attach to
         * @return New data handle
         *
         * Expects that @ref Shared::setGlyphCache() has been called, @p style
         * is less than @ref Shared::styleCount() and
         * @ref TextProperties::font() is either @ref FontHandle::Null or
         * valid. Styling is driven from the @ref TextLayerStyleUniform at
         * index @p style. If @ref TextProperties::font() is not null it's
         * used, otherwise the default @ref FontHandle assigned to given style
         * is used and is expected to not be null. The @ref FontHandle, whether
         * coming from the @p style or from @p properties, is expected to have
         * a font instance. Instance-less fonts can be only used to create
         * single glyphs (such as various icons or images) with
         * @ref createGlyph(). Use @ref create(UnsignedInt, Containers::StringView, const TextProperties&, const Color3&, NodeHandle)
         * for creating a text with a custom color. This function is equivalent
         * to calling it with @cpp 0xffffff_srgbf @ce.
         * @see @ref Shared::hasFontInstance(), @ref setText()
         */
        DataHandle create(UnsignedInt style, Containers::StringView text, const TextProperties& properties, NodeHandle node =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            NodeHandle::Null
            #else
            NodeHandle{} /* To not have to include Handle.h */
            #endif
        ) {
            return create(style, text, properties, Color3{1.0f}, node);
        }

        /**
         * @brief Create a text with a style index in a concrete enum type
         *
         * Casts @p style to @relativeref{Magnum,UnsignedInt} and delegates to
         * @ref create(UnsignedInt, Containers::StringView, const TextProperties&, NodeHandle).
         */
        template<class StyleIndex
            #ifndef DOXYGEN_GENERATING_OUTPUT
            , class = typename std::enable_if<std::is_enum<StyleIndex>::value>::type
            #endif
        > DataHandle create(StyleIndex style, Containers::StringView text, const TextProperties& properties, NodeHandle node =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            NodeHandle::Null
            #else
            NodeHandle{} /* To not have to include Handle.h */
            #endif
        ) {
            return create(UnsignedInt(style), text, properties, node);
        }

        /**
         * @brief Create a text with a custom color
         * @param style         Style index
         * @param text          Text to render
         * @param properties    Text properties
         * @param color         Custom color
         * @param node          Node to attach to
         * @return New data handle
         *
         * Expects that @ref Shared::setGlyphCache() has been called, @p style
         * is less than @ref Shared::styleCount() and
         * @ref TextProperties::font() is either @ref FontHandle::Null or
         * valid. Styling is driven from the @ref TextLayerStyleUniform at
         * index @p style. If @ref TextProperties::font() is not null it's
         * used, otherwise the default @ref FontHandle assigned to given style
         * is used and is expected to not be null. In addition
         * @ref TextLayerStyleUniform::color is multiplied with @p color. The
         * @ref FontHandle, whether coming from the @p style or from
         * @p properties, is expected to have a font instance. Instance-less
         * fonts can be only used to create single glyphs (such as various
         * icons or images) with @ref createGlyph().
         * @see @ref create(UnsignedInt, Containers::StringView, const TextProperties&, NodeHandle),
         *      @ref Shared::hasFontInstance(), @ref setText()
         */
        DataHandle create(UnsignedInt style, Containers::StringView text, const TextProperties& properties, const Color3& color, NodeHandle node =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            NodeHandle::Null
            #else
            NodeHandle{} /* To not have to include Handle.h */
            #endif
        );

        /**
         * @brief Create a text with a style index in a concrete enum type and a custom color
         *
         * Casts @p style to @relativeref{Magnum,UnsignedInt} and delegates to
         * @ref create(UnsignedInt, Containers::StringView, const TextProperties&, const Color3&, NodeHandle).
         */
        template<class StyleIndex
            #ifndef DOXYGEN_GENERATING_OUTPUT
            , class = typename std::enable_if<std::is_enum<StyleIndex>::value>::type
            #endif
        > DataHandle create(StyleIndex style, Containers::StringView text, const TextProperties& properties, const Color3& color, NodeHandle node =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            NodeHandle::Null
            #else
            NodeHandle{} /* To not have to include Handle.h */
            #endif
        ) {
            return create(UnsignedInt(style), text, properties, color, node);
        }

        /**
         * @brief Create a single glyph
         * @param style         Style index
         * @param glyph         Glyph ID to render
         * @param properties    Text properties
         * @param node          Node to attach to
         * @return New data handle
         *
         * Expects that @ref Shared::setGlyphCache() has been called, @p style
         * is less than @ref Shared::styleCount() and
         * @ref TextProperties::font() is either @ref FontHandle::Null or
         * valid. Styling is driven from the @ref TextLayerStyleUniform at
         * index @p style. If @ref TextProperties::font() is not null it's
         * used, otherwise the default @ref FontHandle assigned to given style
         * is used and is expected to not be null. The @p glyph is expected to
         * be less than @ref Text::AbstractGlyphCache::fontGlyphCount() for
         * given font. Use @ref createGlyph(UnsignedInt, UnsignedInt, const TextProperties&, const Color3&, NodeHandle)
         * for creating a glyph with a custom color. This function is
         * equivalent to calling it with @cpp 0xffffff_srgbf @ce.
         *
         * Compared to @ref create(), the glyph is aligned according to
         * @ref TextProperties::alignment() based on its bounding rectangle
         * coming from the glyph cache, not based on font metrics. The
         * @ref TextProperties::script(),
         * @relativeref{TextProperties,language()},
         * @relativeref{TextProperties,shapeDirection()},
         * @relativeref{TextProperties,layoutDirection()} and
         * @relativeref{TextProperties,features()} properties aren't used in
         * any way.
         * @see @ref Shared::glyphCacheFontId(), @ref setGlyph()
         */
        DataHandle createGlyph(UnsignedInt style, UnsignedInt glyph, const TextProperties& properties, NodeHandle node =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            NodeHandle::Null
            #else
            NodeHandle{} /* To not have to include Handle.h */
            #endif
        ) {
            return createGlyph(style, glyph, properties, Color3{1.0f}, node);
        }

        /**
         * @brief Create a single glyph with a style index in a concrete enum type
         *
         * Casts @p style to @relativeref{Magnum,UnsignedInt} and delegates to
         * @ref createGlyph(UnsignedInt, UnsignedInt, const TextProperties&, NodeHandle).
         */
        template<class StyleIndex
            #ifndef DOXYGEN_GENERATING_OUTPUT
            , class = typename std::enable_if<std::is_enum<StyleIndex>::value>::type
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
            , class = typename std::enable_if<std::is_enum<GlyphIndex>::value>::type
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
            , class = typename std::enable_if<std::is_enum<StyleIndex>::value && std::is_enum<GlyphIndex>::value>::type
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
         * @brief Create a single glyph with a custom color
         * @param style         Style index
         * @param glyph         Glyph to render
         * @param properties    Text properties
         * @param color         Custom color
         * @param node          Node to attach to
         * @return New data handle
         *
         * Expects that @ref Shared::setGlyphCache() has been called, @p style
         * is less than @ref Shared::styleCount() and
         * @ref TextProperties::font() is either @ref FontHandle::Null or
         * valid. Styling is driven from the @ref TextLayerStyleUniform at
         * index @p style. If @ref TextProperties::font() is not null it's
         * used, otherwise the default @ref FontHandle assigned to given style
         * is used and is expected to not be null. In addition
         * @ref TextLayerStyleUniform::color is multiplied with @p color. The
         * @p glyphId is expected to be less than
         * @ref Text::AbstractGlyphCache::fontGlyphCount() for given font.
         *
         * Compared to @ref create(), the glyph is aligned according to
         * @ref TextProperties::alignment() based on its bounding rectangle
         * coming from the glyph cache, not based on font metrics. The
         * @ref TextProperties::script(),
         * @relativeref{TextProperties,language()},
         * @relativeref{TextProperties,shapeDirection()},
         * @relativeref{TextProperties,layoutDirection()} and
         * @relativeref{TextProperties,features()} properties aren't used in
         * any way.
         * @see @ref Shared::glyphCacheFontId(), @ref setGlyph()
         */
        DataHandle createGlyph(UnsignedInt style, UnsignedInt glyph, const TextProperties& properties, const Color3& color, NodeHandle node =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            NodeHandle::Null
            #else
            NodeHandle{} /* To not have to include Handle.h */
            #endif
        );

        /**
         * @brief Create a single glyph with a style index in a concrete enum type and a custom color
         *
         * Casts @p style to @relativeref{Magnum,UnsignedInt} and delegates to
         * @ref createGlyph(UnsignedInt, UnsignedInt, const TextProperties&, const Color3&, NodeHandle).
         */
        template<class StyleIndex
            #ifndef DOXYGEN_GENERATING_OUTPUT
            , class = typename std::enable_if<std::is_enum<StyleIndex>::value>::type
            #endif
        > DataHandle createGlyph(StyleIndex style, UnsignedInt glyph, const TextProperties& properties, const Color3& color, NodeHandle node =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            NodeHandle::Null
            #else
            NodeHandle{} /* To not have to include Handle.h */
            #endif
        ) {
            return createGlyph(UnsignedInt(style), glyph, properties, color, node);
        }

        /**
         * @brief Create a single glyph with a glyph ID in a concrete enum type and a custom color
         *
         * Casts @p glyph to @relativeref{Magnum,UnsignedInt} and delegates to
         * @ref createGlyph(UnsignedInt, UnsignedInt, const TextProperties&, const Color3&, NodeHandle).
         */
        template<class GlyphIndex
            #ifndef DOXYGEN_GENERATING_OUTPUT
            , class = typename std::enable_if<std::is_enum<GlyphIndex>::value>::type
            #endif
        > DataHandle createGlyph(UnsignedInt style, GlyphIndex glyph, const TextProperties& properties, const Color3& color, NodeHandle node =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            NodeHandle::Null
            #else
            NodeHandle{} /* To not have to include Handle.h */
            #endif
        ) {
            return createGlyph(style, UnsignedInt(glyph), properties, color, node);
        }

        /**
         * @brief Create a single glyph with a style index and glyph ID in a concrete enum type and a custom color
         *
         * Casts @p style and @p glyph to @relativeref{Magnum,UnsignedInt} and
         * delegates to @ref createGlyph(UnsignedInt, UnsignedInt, const TextProperties&, const Color3&, NodeHandle).
         */
        template<class StyleIndex, class GlyphIndex
            #ifndef DOXYGEN_GENERATING_OUTPUT
            , class = typename std::enable_if<std::is_enum<StyleIndex>::value && std::is_enum<GlyphIndex>::value>::type
            #endif
        > DataHandle createGlyph(StyleIndex style, GlyphIndex glyph, const TextProperties& properties, const Color3& color, NodeHandle node =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            NodeHandle::Null
            #else
            NodeHandle{} /* To not have to include Handle.h */
            #endif
        ) {
            return createGlyph(UnsignedInt(style), UnsignedInt(glyph), properties, color, node);
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
         * Calling this function causes @ref LayerState::NeedsUpdate to be set.
         * @see @ref isHandleValid(DataHandle) const,
         *      @ref Shared::isHandleValid(FontHandle) const
         */
        void setText(DataHandle handle, Containers::StringView text, const TextProperties& properties);

        /**
         * @brief Set text assuming it belongs to this layer
         *
         * Like @ref setText(DataHandle, Containers::StringView, const TextProperties&)
         * but without checking that @p handle indeed belongs to this layer.
         * See its documentation for more information.
         */
        void setText(LayerDataHandle handle, Containers::StringView text, const TextProperties& properties);

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
         * any way. Note that it's also possible to change a handle that
         * previously contained a text to a single glyph and vice versa --- the
         * internal representation of both is the same.
         *
         * Calling this function causes @ref LayerState::NeedsUpdate to be set.
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
            , class = typename std::enable_if<std::is_enum<GlyphIndex>::value>::type
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
            , class = typename std::enable_if<std::is_enum<GlyphIndex>::value>::type
            #endif
        > void setGlyph(LayerDataHandle handle, GlyphIndex glyph, const TextProperties& properties) {
            return setGlyph(handle, UnsignedInt(glyph), properties);
        }

        /**
         * @brief Text custom base color
         *
         * Expects that @p handle is valid.
         * @see @ref isHandleValid(DataHandle) const
         */
        Color3 color(DataHandle handle) const;

        /**
         * @brief Text custom base color assuming it belongs to this layer
         *
         * Expects that @p handle is valid.
         * @see @ref isHandleValid(LayerDataHandle) const
         */
        Color3 color(LayerDataHandle handle) const;

        /**
         * @brief Set text custom base color
         *
         * Expects that @p handle is valid. @ref TextLayerStyleUniform::color
         * is multiplied with @p color. By default, unless specified in
         * @ref create() / @ref createGlyph() already, the custom color is
         * @cpp 0xffffff_srgbf @ce, i.e. not affecting the style in any way.
         *
         * Calling this function causes @ref LayerState::NeedsUpdate to be set.
         * @see @ref isHandleValid(DataHandle) const
         */
        void setColor(DataHandle handle, const Color3& color);

        /**
         * @brief Set text custom base color assuming it belongs to this layer
         *
         * Like @ref setColor(DataHandle, const Color3&) but without checking
         * that @p handle indeed belongs to this layer. See its documentation
         * for more information.
         */
        void setColor(LayerDataHandle handle, const Color3& color);

        /**
         * @brief Text custom padding
         *
         * In order left, top. right, bottom. Expects that @p handle is valid.
         * @see @ref isHandleValid(DataHandle) const
         */
        Vector4 padding(DataHandle handle) const;

        /**
         * @brief Text custom padding assuming it belongs to this layer
         *
         * In order left, top. right, bottom. Expects that @p handle is valid.
         * @see @ref isHandleValid(LayerDataHandle) const
         */
        Vector4 padding(LayerDataHandle handle) const;

        /**
         * @brief Set text custom padding
         *
         * Expects that @p handle is valid. The @p padding is in order left,
         * top, right, bottom and is added to the per-style padding values
         * specified in @ref Shared::setStyle().
         *
         * Calling this function causes @ref LayerState::NeedsUpdate to be set.
         * @see @ref isHandleValid(DataHandle) const
         */
        void setPadding(DataHandle handle, const Vector4& padding);

        /**
         * @brief Set text custom padding assuming it belongs to this layer
         *
         * Like @ref setPadding(DataHandle, const Vector4&) but without
         * checking that @p handle indeed belongs to this layer. See its
         * documentation for more information.
         */
        void setPadding(LayerDataHandle handle, const Vector4& padding);

        /**
         * @brief Set text custom padding with all edges having the same value
         *
         * Expects that @p handle is valid. The @p padding is added to the
         * per-style padding values specified in @ref Shared::setStyle().
         *
         * Calling this function causes @ref LayerState::NeedsUpdate to be set.
         * @see @ref isHandleValid(DataHandle) const
         */
        void setPadding(DataHandle handle, Float padding) {
            setPadding(handle, Vector4{padding});
        }

        /**
         * @brief Set text custom padding with all edges having the same value assuming it belongs to this layer
         *
         * Like @ref setPadding(DataHandle, Float) but without checking that
         * @p handle indeed belongs to this layer. See its documentation for
         * more information.
         */
        void setPadding(LayerDataHandle handle, Float padding) {
            setPadding(handle, Vector4{padding});
        }

    #ifdef DOXYGEN_GENERATING_OUTPUT
    private:
    #else
    protected:
    #endif
        struct State;

        MAGNUM_WHEE_LOCAL explicit TextLayer(LayerHandle handle, Containers::Pointer<State>&& state);
        /* Used by tests to avoid having to include / allocate the state */
        explicit TextLayer(LayerHandle handle, Shared& shared);

        /* These can't be MAGNUM_WHEE_LOCAL otherwise deriving from this class
           in tests causes linker errors */

        /* Advertises LayerFeature::Draw but *does not* implement doDraw(),
           that's on the subclass */
        LayerFeatures doFeatures() const override;

        void doUpdate(const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, Containers::BitArrayView nodesEnabled, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes) override;

    private:
        MAGNUM_WHEE_LOCAL DataHandle createInternal(NodeHandle node);
        MAGNUM_WHEE_LOCAL void shapeTextInternal(
            #ifndef CORRADE_NO_ASSERT
            const char* messagePrefix,
            #endif
            UnsignedInt id, UnsignedInt style, Containers::StringView text, const TextProperties& properties);
        MAGNUM_WHEE_LOCAL void shapeGlyphInternal(
            #ifndef CORRADE_NO_ASSERT
            const char* messagePrefix,
            #endif
            UnsignedInt id, UnsignedInt style, UnsignedInt glyphId, const TextProperties& properties);
        MAGNUM_WHEE_LOCAL void removeInternal(UnsignedInt id);
        MAGNUM_WHEE_LOCAL void setTextInternal(UnsignedInt id, Containers::StringView text, const TextProperties& properties);
        MAGNUM_WHEE_LOCAL void setGlyphInternal(UnsignedInt id, UnsignedInt glyph, const TextProperties& properties);
        MAGNUM_WHEE_LOCAL void setColorInternal(UnsignedInt id, const Color3& color);
        MAGNUM_WHEE_LOCAL void setPaddingInternal(UnsignedInt id, const Vector4& padding);

        /* Can't be MAGNUM_WHEE_LOCAL otherwise deriving from this class in
           tests causes linker errors */
        void doClean(Containers::BitArrayView dataIdsToRemove) override;
};

/**
@brief Shared state for the text layer

Contains a set of fonts and a glyph cache used by all of them. In order to use
the layer it's expected that @ref setGlyphCache() was called and at least one
font was added with @ref addFont(). In order to update or draw the layer it's
expected that @ref setStyle() was called.

Pre-filling the glyph cache with appropriate glyphs for a particular font is
the user responsibility, the implementation currently won't perform that on its
own, neither it does any on-demand cache filling.
*/
class MAGNUM_WHEE_EXPORT TextLayer::Shared: public AbstractVisualLayer::Shared {
    public:
        class Configuration;

        /**
         * @brief Style uniform count
         *
         * Size of the style uniform buffer. May or may not be the same as
         * @ref styleCount().
         * @see @ref Configuration::Configuration(UnsignedInt, UnsignedInt),
         *      @ref setStyle()
         */
        UnsignedInt styleUniformCount() const;

        /**
         * @brief Whether a glyph cache has been set
         *
         * @see @ref glyphCache(), @ref setGlyphCache()
         */
        bool hasGlyphCache() const;

        /**
         * @brief Glyph cache instance
         *
         * Expects that a glyph cache was set with @ref setGlyphCache().
         * @see @ref hasGlyphCache()
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
         * @param size      Size at which to render the font
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
         * @see @ref addInstancelessFont()
         */
        FontHandle addFont(Text::AbstractFont& font, Float size);

        /**
         * @brief Add a font and take over its ownership
         * @param font      Font instance
         * @param size      Size at which to render the font
         * @return New font handle
         *
         * Like @ref addFont(Text::AbstractFont&, Float), but the shared state
         * takes over the font ownership. You can access the instance using
         * @ref font() later. It's the caller responsibility to ensure the
         * plugin manager the font is coming from stays in scope for as long as
         * the shared state is used.
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
         * @param paddings      Padding inside the node in order left, top,
         *      right, bottom corresponding to style uniforms
         * @return Reference to self (for method chaining)
         *
         * The @p uniforms view is expected to have the same size as
         * @ref styleUniformCount(), the @p fonts view the same size as
         * @ref styleCount(). All font handles are expected to be either
         * @ref FontHandle::Null or valid. The @p paddings view is expected to
         * either have the same size as @ref styleCount() or be empty, in which
         * case all paddings are implicitly zero.
         *
         * Can only be called if @ref styleUniformCount() and @ref styleCount()
         * were set to the same value in @ref Configuration passed to the
         * constructor, otherwise you have to additionally provide a mapping
         * from styles to uniforms using
         * @ref setStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const FontHandle>&, const Containers::StridedArrayView1D<const Vector4>&)
         * instead.
         * @see @ref isHandleValid(FontHandle) const
         */
        Shared& setStyle(const TextLayerCommonStyleUniform& commonUniform, Containers::ArrayView<const TextLayerStyleUniform> uniforms, const Containers::StridedArrayView1D<const FontHandle>& fonts, const Containers::StridedArrayView1D<const Vector4>& paddings);
        /** @overload */
        Shared& setStyle(const TextLayerCommonStyleUniform& commonUniform, std::initializer_list<TextLayerStyleUniform> uniforms, std::initializer_list<FontHandle> fonts, std::initializer_list<Vector4> paddings);

        /**
         * @brief Set style data
         * @param commonUniform     Common style uniform data
         * @param uniforms          Style uniforms
         * @param styleToUniform    Style to style uniform mapping
         * @param styleFonts        Per-style font handles
         * @param stylePaddings     Per-style padding inside the node in order
         *      left, top, right, bottom
         * @return Reference to self (for method chaining)
         *
         * The @p uniforms view is expected to have the same size as
         * @ref styleUniformCount(), the @p styleToUniform and @p styleFonts
         * views the same size as @ref styleCount(). All font handles are
         * expected to be either @ref FontHandle::Null or valid. The
         * @p stylePaddings view is expected to either have the same size as
         * @ref styleCount() or be empty, in which case all paddings are
         * implicitly zero.
         *
         * Value of @cpp styleToUniform[i] @ce should give back an index into
         * the @p uniforms array for style @cpp i @ce. If
         * @ref styleUniformCount() and @ref styleCount() is the same and the
         * mapping is implicit, you can use the
         * @ref setStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>, const Containers::StridedArrayView1D<const FontHandle>&, const Containers::StridedArrayView1D<const Vector4>&)
         * convenience overload instead.
         * @see @ref isHandleValid(FontHandle) const
         */
        Shared& setStyle(const TextLayerCommonStyleUniform& commonUniform, Containers::ArrayView<const TextLayerStyleUniform> uniforms, const Containers::StridedArrayView1D<const UnsignedInt>& styleToUniform, const Containers::StridedArrayView1D<const FontHandle>& styleFonts, const Containers::StridedArrayView1D<const Vector4>& stylePaddings);
        /** @overload */
        Shared& setStyle(const TextLayerCommonStyleUniform& commonUniform, std::initializer_list<TextLayerStyleUniform> uniforms, std::initializer_list<UnsignedInt> styleToUniform, std::initializer_list<FontHandle> styleFonts, std::initializer_list<Vector4> stylePaddings);

        /* Overloads to remove a WTF factor from method chaining order */
        #ifndef DOXYGEN_GENERATING_OUTPUT
        MAGNUMEXTRAS_WHEE_ABSTRACTVISUALLAYER_SHARED_SUBCLASS_IMPLEMENTATION()
        #endif

    protected:
        struct State;

        /**
         * @brief Set a glyph cache instance
         * @return Reference to self (for method chaining)
         *
         * Has to be called before any @ref addFont(), is expected to be called
         * exactly once. You'll most probably want to set it through the public
         * @ref TextLayerGL::Shared::setGlyphCache(Text::GlyphCache&).
         */
        Shared& setGlyphCache(Text::AbstractGlyphCache& cache);

    #ifdef DOXYGEN_GENERATING_OUTPUT
    private:
    #else
    protected:
    #endif
        friend TextLayer;

        MAGNUM_WHEE_LOCAL explicit Shared(Containers::Pointer<State>&& state);
        /* Used by tests to avoid having to include / allocate the state */
        explicit Shared(const Configuration& configuration);
        /* Can't be MAGNUM_WHEE_LOCAL, used by tests */
        explicit Shared(NoCreateT) noexcept;

    private:
        MAGNUM_WHEE_LOCAL void setStyleInternal(const TextLayerCommonStyleUniform& commonUniform, Containers::ArrayView<const TextLayerStyleUniform> uniforms, const Containers::StridedArrayView1D<const FontHandle>& styleFonts, const Containers::StridedArrayView1D<const Vector4>& stylePaddings);

        /* The items are guaranteed to have the same size as
           styleUniformCount() */
        virtual void doSetStyle(const TextLayerCommonStyleUniform& commonUniform, Containers::ArrayView<const TextLayerStyleUniform> uniforms) = 0;
};

/**
@brief Configuration of a base layer shared state

@see @ref TextLayerGL::Shared::Shared(const Configuration&)
*/
class MAGNUM_WHEE_EXPORT TextLayer::Shared::Configuration {
    public:
        /**
         * @brief Constructor
         *
         * The @p styleUniformCount parameter specifies the size of the uniform
         * array, @p styleCount then the number of distinct styles to use for
         * drawing. The sizes are independent in order to allow styles with
         * different fonts or paddings share the same uniform data. Both
         * @p styleUniformCount and @p styleCount is expected to be non-zero.
         * Style data are then set with @ref setStyle().
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

    private:
        UnsignedInt _styleUniformCount, _styleCount;
};

inline TextLayer::Shared& TextLayer::shared() {
    return static_cast<Shared&>(AbstractVisualLayer::shared());
}

inline const TextLayer::Shared& TextLayer::shared() const {
    return static_cast<const Shared&>(AbstractVisualLayer::shared());
}

}}

#endif
