#ifndef Magnum_Ui_TextProperties_h
#define Magnum_Ui_TextProperties_h
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
 * @brief Class @ref Magnum::Ui::TextFeatureValue, @ref Magnum::Ui::TextProperties
 * @m_since_latest
 */

#include <initializer_list>
#include <Corrade/Containers/Pointer.h>
#include <Magnum/Text/Text.h>

#include "Magnum/Ui/Ui.h"
#include "Magnum/Ui/visibility.h"

namespace Magnum { namespace Ui {

/**
@brief OpenType feature value
@m_since_latest

A subset of @ref Text::FeatureRange that always affects the whole text. Meant
to be used to supply default features for a style.
@see @ref TextLayer::Shared::setStyle()
*/
class MAGNUM_UI_EXPORT TextFeatureValue {
    public:
        /**
         * @brief Constructor
         * @param feature   Feature to control
         * @param value     Feature value to set
         */
        constexpr /*implicit*/ TextFeatureValue(Text::Feature feature, UnsignedInt value = true): _feature{feature}, _value{value} {}

        /** @brief Feature to control */
        constexpr Text::Feature feature() const { return _feature; }

        /**
         * @brief Whether to enable the feature
         *
         * Returns @cpp false @ce if @ref value() is @cpp 0 @ce, @cpp true @ce
         * otherwise.
         */
        constexpr bool isEnabled() const { return _value; }

        /** @brief Feature value to sest */
        constexpr UnsignedInt value() const { return _value; }

        /**
         * @brief Conversion to @ref Text::FeatureRange
         *
         * The range has @ref Text::FeatureRange::begin() set to @cpp 0 @ce and
         * @relativeref{Text::FeatureRange,end()} to @cpp 0xffffffffu @ce.
         */
        /*implicit*/ operator Text::FeatureRange() const;

    private:
        Text::Feature _feature;
        UnsignedInt _value;
};

/**
@brief Text properties
@m_since_latest

@see @ref TextLayer::create(), @ref TextLayer::setText()
*/
class MAGNUM_UI_EXPORT TextProperties {
    public:
        /** @brief Default constructor */
        /*implicit*/ TextProperties();

        /**
         * @brief Construct with an alignment
         *
         * Equivalent to constructing with @ref TextProperties() and then
         * calling @ref setAlignment(). See its documentation for value
         * restrictions.
         */
        /*implicit*/ TextProperties(Text::Alignment alignment);

        /**
         * @brief Construct with a font
         *
         * Equivalent to constructing with @ref TextProperties() and then
         * calling @ref setFont().
         */
        /*implicit*/ TextProperties(FontHandle font): TextProperties{} {
            setFont(font);
        }

        /**
         * @brief Construct with a font and an alignment
         *
         * Equivalent to constructing with @ref TextProperties() and then
         * calling @ref setFont() and @ref setAlignment(). See documentation of
         * the latter for value restrictions.
         */
        /*implicit*/ TextProperties(FontHandle font, Text::Alignment alignment);

        /** @brief Copying is not allowed */
        TextProperties(const TextProperties&) = delete;

        /** @brief Move constructor */
        TextProperties(TextProperties&&) noexcept;

        /** @brief Copying is not allowed */
        TextProperties& operator=(const TextProperties&) = delete;

        /** @brief Move assignment */
        TextProperties& operator=(TextProperties&&) noexcept;

        ~TextProperties();

        /** @brief Alignment */
        Containers::Optional<Text::Alignment> alignment() const;

        /**
         * @brief Set alignment
         * @return Reference to self (for method chaining)
         *
         * Default is @relativeref{Corrade,Containers::NullOpt}, i.e. an
         * alignment specified by the style is used. If not
         * @relativeref{Corrade,Containers::NullOpt}, expects that the
         * @p alignment isn't `*GlyphBounds` as the implementation can only
         * align based on font metrics and cursor position, not actual glyph
         * bounds.
         *
         * In addition to the behavior in particular @ref Text::Alignment
         * values, the aligned origin is then further offset respectively to
         * the node the text is attached to. In particular:
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
         */
        TextProperties& setAlignment(const Containers::Optional<Text::Alignment>& alignment) &;
        /** @overload */
        TextProperties&& setAlignment(const Containers::Optional<Text::Alignment>& alignment) &&;

        /** @brief Font for the whole text */
        FontHandle font() const { return _font; }

        /**
         * @brief Set font for the whole text
         * @return Reference to self (for method chaining)
         *
         * Default is @ref FontHandle::Null, i.e. the default font for given
         * style is used.
         */
        TextProperties& setFont(FontHandle font) & {
            _font = font;
            return *this;
        }
        /** @overload */
        TextProperties&& setFont(FontHandle font) && {
            return Utility::move(setFont(font));
        }

        /** @brief Script for the whole text */
        Text::Script script() const { return _script; }

        /**
         * @brief Set script for the whole text
         * @return Reference to self (for method chaining)
         *
         * Default is @ref Text::Script::Unspecified, i.e. the font shaper may
         * attempt to guess the script from the input text. See the
         * documentation of @ref Text::AbstractShaper for more information
         * about how script, language and direction setting affects the shaped
         * text.
         * @see @ref setLanguage(), @ref setShapeDirection()
         */
        TextProperties& setScript(Text::Script script) & {
            _script = script;
            return *this;
        }
        /** @overload */
        TextProperties&& setScript(Text::Script script) && {
            return Utility::move(setScript(script));
        }

        /**
         * @brief Language for the whole text
         *
         * The returned view is only guaranteed to be valid for as long as the
         * @ref TextProperties instance is alive, or until @ref setLanguage()
         * is called.
         */
        Containers::StringView language() const;

        /**
         * @brief Set language for the whole text
         * @return Reference to self (for method chaining)
         *
         * The language is expected to be a [BCP 47 language tag](https://en.wikipedia.org/wiki/IETF_language_tag),
         * either just the base tag such as @cpp "en" @ce or @cpp "cs" @ce
         * alone, or further differentiating with a region subtag like for
         * example @cpp "en-US" @ce vs @cpp "en-GB" @ce, at most 15 bytes long.
         *
         * Default is an empty string, i.e. the font shaper may attempt to
         * guess the language from the input text. See the documentation of
         * @ref Text::AbstractShaper for more information about how script,
         * language and direction setting affects the shaped text.
         * @see @ref setScript(), @ref setShapeDirection()
         */
        TextProperties& setLanguage(Containers::StringView language) &;
        /** @overload */
        TextProperties&& setLanguage(Containers::StringView language) &&;

        /** @brief Shaping direction for the whole text */
        Text::ShapeDirection shapeDirection() const {
            return Text::ShapeDirection(_direction & 0x0f);
        }

        /**
         * @brief Set shaping direction for the whole text
         * @return Reference to self (for method chaining)
         *
         * Default is @ref Text::ShapeDirection::Unspecified, i.e. the font
         * shaper may attempt to guess the direction from the input text. See
         * the documentation of @ref Text::AbstractShaper for more information
         * about how script, language and direction setting affects the shaped
         * text.
         * @see @ref setLayoutDirection(), @ref setScript(), @ref setLanguage()
         */
        TextProperties& setShapeDirection(Text::ShapeDirection direction) & {
            _direction = (_direction & 0xf0)|(UnsignedByte(direction) & 0x0f);
            return *this;
        }
        /** @overload */
        TextProperties&& setShapeDirection(Text::ShapeDirection direction) && {
            return Utility::move(setShapeDirection(direction));
        }

        /** @brief Layout direction */
        Text::LayoutDirection layoutDirection() const {
            return Text::LayoutDirection(_direction >> 4);
        }

        /**
         * @brief Set layout direction
         * @return Reference to self (for method chaining)
         *
         * Default is @ref Text::LayoutDirection::HorizontalTopToBottom. Unlike
         * shape direction, script, language and font properties, which may be
         * different for different parts of the text, the layout direction is
         * always for the whole text.
         * @see @ref setShapeDirection()
         */
        TextProperties& setLayoutDirection(Text::LayoutDirection direction) & {
            _direction = (_direction & 0x0f)|(UnsignedByte(direction) << 4);
            return *this;
        }
        /** @overload */
        TextProperties&& setLayoutDirection(Text::LayoutDirection direction) && {
            return Utility::move(setLayoutDirection(direction));
        }

        /**
         * @brief Typographic features
         *
         * The returned view is only guaranteed to be valid for as long as the
         * @ref TextProperties instance is alive.
         */
        Containers::ArrayView<const Text::FeatureRange> features() const;

        /**
         * @brief Set typographic features
         * @return Reference to self (for method chaining)
         *
         * By default no features are explicitly set, relying on features
         * supplied by the style, if any, and then default behavior of a
         * particular font file and a font plugin. A copy of @p features is
         * made internally, is *appended* to features coming from the style
         * and subsequently passed to @ref Text::AbstractShaper::shape(). See
         * its documentation for details and information about various
         * constraints.
         * @see @ref TextLayer::Shared::setStyle()
         */
        TextProperties& setFeatures(Containers::ArrayView<const Text::FeatureRange> features) &;
        /** @overload */
        TextProperties&& setFeatures(Containers::ArrayView<const Text::FeatureRange> features) &&;
        /** @overload */
        TextProperties& setFeatures(std::initializer_list<Text::FeatureRange> features) &;
        /** @overload */
        TextProperties&& setFeatures(std::initializer_list<Text::FeatureRange> features) && {
            return Utility::move(setFeatures(features));
        }

    private:
        struct State;
        /* Internals accessed by TextLayer directly to not have to pack and
           unpack from StringView, Optional etc */
        friend TextLayer;

        /* Used by TextLayer. Only the _state pointer is initialized in this
           case, everything else is left at random. */
        MAGNUM_UI_LOCAL explicit TextProperties(NoInitT);

        /* The _state is only allocated when passing a feature list. Eventually
           it'll contain also font/language/script/direction properties for
           sub-ranges of the text. */
        Containers::Pointer<State> _state;
        /* Language stored as a null-terminated string up to 15 characters. Has
           the same size as a StringView on 64bit, but actually owns the data,
           avoiding a need to allocate _state every time a non-global language
           string is used. 15 bytes should be plenty even for the longer
           examples at https://en.wikipedia.org/wiki/IETF_language_tag, worst
           case we can always switch to storing a String (which has 22 bytes
           for SSO on 64bit). */
        char _language[16];
        Text::Script _script;
        FontHandle _font;
        /* If 0xff, indicates that alignment is not set to avoid an Optional
           wrapper that'd double the field size */
        Text::Alignment _alignment;
        /* Packs both shape and layout direction to avoid a 3/7 byte padding at
           the end */
        UnsignedByte _direction;
};

}}

#endif
