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
 * @brief Class @ref Magnum::Whee::TextLayer, struct @ref Magnum::Whee::TextLayerStyleCommon, @ref Magnum::Whee::TextLayerStyleItem, enum @ref Magnum::Whee::FontHandle, function @ref Magnum::Whee::fontHandle(), @ref Magnum::Whee::fontHandleId(), @ref Magnum::Whee::fontHandleGeneration()
 * @m_since_latest
 */

#include <Corrade/Containers/StringView.h> /* for templated create() overloads */
#include <Corrade/PluginManager/Manager.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Text/Text.h>

#include "Magnum/Whee/AbstractVisualLayer.h"

namespace Magnum { namespace Whee {

/**
@brief Common style properties for all @ref TextLayer data
@m_since_latest

Currently this is just a placeholder with no properties.
@see @ref TextLayerStyleItem, @ref TextLayerGL::Shared::setStyle()
*/
struct alignas(4) TextLayerStyleCommon {
    /** @brief Construct with default values */
    constexpr explicit TextLayerStyleCommon(DefaultInitT = DefaultInit) noexcept {}

    /** @brief Construct without initializing the contents */
    explicit TextLayerStyleCommon(NoInitT) noexcept {}

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
@brief Varying style properties for @ref TextLayer data
@m_since_latest

@see @ref TextLayerStyleCommon, @ref TextLayerGL::Shared::setStyle()
*/
struct TextLayerStyleItem {
    /** @brief Construct with default values */
    constexpr explicit TextLayerStyleItem(DefaultInitT = DefaultInit) noexcept: color{1.0f} {}

    /** @brief Constructor */
    constexpr /*implicit*/ TextLayerStyleItem(const Color4& color): color{color} {}

    /** @brief Construct without initializing the contents */
    explicit TextLayerStyleItem(NoInitT) noexcept: color{NoInit} {}

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
    TextLayerStyleItem& setColor(const Color4& color) {
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
*/
class MAGNUM_WHEE_EXPORT TextLayer: public AbstractVisualLayer {
    public:
        class Shared;

        /**
         * @brief Create a text
         * @param style         Style index
         * @param text          Text to render
         * @param properties    Text properties
         * @param node          Node to attach to
         * @return New data handle
         *
         * Expects that @p style is less than @ref Shared::styleCount() and
         * @ref TextProperties::font() is either @ref FontHandle::Null or
         * valid. Styling is driven from the @ref TextLayerStyleItem at index
         * @p style but if @ref TextProperties::font() is not null it's used
         * instead of the default @ref FontHandle assigned to given style. Use
         * @ref create(UnsignedInt, Containers::StringView, const TextProperties&, const Color3&, NodeHandle)
         * for creating a text with a custom color. This function is equivalent
         * to calling it with @cpp 0xffffff_srgbf @ce.
         * @see @ref setText()
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
         * valid. Styling is driven from the @ref TextLayerStyleItem at index
         * @p style but if @ref TextProperties::font() is not null it's used
         * instead of the default @ref FontHandle assigned to given style. In
         * addition @ref TextLayerStyleItem::color is multiplied with @p color.
         * @see @ref create(UnsignedInt, Containers::StringView, const TextProperties&, NodeHandle),
         *      @ref setText()
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
         * @brief Set text
         *
         * Expects that @p handle is valid and @ref TextProperties::font() is
         * either @ref FontHandle::Null or valid. If not null, the
         * @ref TextProperties::font() is used instead of the default
         * @ref FontHandle assigned to @ref style(). Note that it's not
         * possible to change the font alone with @ref setStyle(), it only can
         * be done when setting the text.
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
         * Expects that @p handle is valid. @ref TextLayerStyleItem::color is
         * multiplied with @p color. By default, unless specified in
         * @ref create() already, the custom color is @cpp 0xffffff_srgbf @ce,
         * i.e. not affecting the style in any way.
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

    #ifdef DOXYGEN_GENERATING_OUTPUT
    private:
    #else
    protected:
    #endif
        struct State;

        MAGNUM_WHEE_LOCAL explicit TextLayer(LayerHandle handle, Containers::Pointer<State>&& state);
        /* Used by tests to avoid having to include / allocate the state */
        explicit TextLayer(LayerHandle handle, Shared& shared);

        /* Can't be MAGNUM_WHEE_LOCAL otherwise deriving from this class in
           tests causes linker errors */
        void doUpdate(const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes) override;

    private:
        MAGNUM_WHEE_LOCAL void shapeInternal(
            #ifndef CORRADE_NO_ASSERT
            const char* messagePrefix,
            #endif
            UnsignedInt id, UnsignedInt style, Containers::StringView text, const TextProperties& properties);
        MAGNUM_WHEE_LOCAL void removeInternal(UnsignedInt id);
        MAGNUM_WHEE_LOCAL void setTextInternal(UnsignedInt id, Containers::StringView text, const TextProperties& properties);
        MAGNUM_WHEE_LOCAL void setColorInternal(UnsignedInt id, const Color3& color);

        /* These can't be MAGNUM_WHEE_LOCAL otherwise deriving from this class
           in tests causes linker errors */

        /* Advertises LayerFeature::Draw but *does not* implement doDraw(),
           that's on the subclass */
        LayerFeatures doFeatures() const override;

        void doClean(Containers::BitArrayView dataIdsToRemove) override;
};

/**
@brief Shared state for the text layer

Contains a set of fonts and a glyph cache used by all of them. In order to use
the layer it's expected that @ref setGlyphCache() was called, at least one font
was added with @ref addFont() and fonts were assigned to corresponding styles
with @ref setStyleFonts().

Pre-filling the glyph cache with appropriate glyphs for a particular font is
the user responsibility, the implementation currently won't perform that on its
own, neither it does any on-demand cache filling.
*/
class MAGNUM_WHEE_EXPORT TextLayer::Shared: public AbstractVisualLayer::Shared {
    public:
        /**
         * @brief Glyph cache instance
         *
         * Expects that a glyph cache was set with @ref setGlyphCache().
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
         * particular style via @ref setStyleFonts(), or passed to
         * @ref TextProperties::setFont() to create a text with a font that's
         * different from the one assigned to a particular style.
         *
         * It's the caller responsibility to ensure @p font stays in scope for
         * as long as the shared state is used. Use the
         * @ref addFont(Containers::Pointer<Text::AbstractFont>&&, Float)
         * overload to make the shared state take over the font instance.
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
         * @brief Font instance
         *
         * Expects that @p handle is valid.
         * @see @ref isHandleValid(FontHandle) const
         */
        Text::AbstractFont& font(FontHandle handle);
        const Text::AbstractFont& font(FontHandle handle) const; /**< @overload */

        /**
         * @brief Assign fonts to styles
         * @return Reference to self (for method chaining)
         *
         * Meant to be called in tandem with @ref TextLayerGL::Shared::setStyle()
         * to match styles with corresponding font properties. The @p handles
         * view is expected to consist of @ref styleCount() valid handles.
         * @see @ref isHandleValid()
         */
        Shared& setStyleFonts(const Containers::StridedArrayView1D<const FontHandle>& handles);
        /** @overload */
        Shared& setStyleFonts(std::initializer_list<FontHandle> handles);

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
        explicit Shared(UnsignedInt styleCount);
        /* Can't be MAGNUM_WHEE_LOCAL, used by tests */
        explicit Shared(NoCreateT) noexcept;
};

}}

#endif
