#ifndef Magnum_Whee_Label_h
#define Magnum_Whee_Label_h
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
 * @brief Class @ref Magnum::Whee::Label, function @ref Magnum::Whee::label(), enum @ref Magnum::Whee::LabelStyle
 * @m_since_latest
 */

#include "Magnum/Whee/Widget.h"

namespace Magnum { namespace Whee {

/**
@brief Label style
@m_since_latest

@see @ref Label, @ref label()
*/
enum class LabelStyle: UnsignedByte {
    Default,        /**< Default */
    Primary,        /**< Primary */
    Success,        /**< Success */
    Warning,        /**< Warning */
    Danger,         /**< Danger */
    Info,           /**< Info */
    Dim,            /**< Dim */
};

/**
@debugoperatorenum{LabelStyle}
@m_since_latest
*/
MAGNUM_WHEE_EXPORT Debug& operator<<(Debug& debug, LabelStyle value);

/**
@brief Label widget
@m_since_latest
*/
class MAGNUM_WHEE_EXPORT Label: public Widget {
    public:
        /**
         * @brief Construct an icon label
         * @param anchor            Positioning anchor
         * @param style             Label style
         * @param icon              Label icon. Passing @ref Icon::None makes
         *      the label empty.
         *
         * The label can be subsequently converted to a text label using
         * @ref setText().
         * @see @ref label(const Anchor&, LabelStyle, Icon)
         */
        explicit Label(const Anchor& anchor, LabelStyle style, Icon icon);

        /**
         * @brief Construct a text label
         * @param anchor            Positioning anchor
         * @param style             Label style
         * @param text              Label text. Passing an empty string makes
         *      the label empty.
         * @param textProperties    Text shaping and layouting properties
         *
         * The label can be subsequently converted to an icon label using
         * @ref setIcon().
         * @see @ref label(const Anchor&, LabelStyle, Containers::StringView, const TextProperties&)
         */
        #ifdef DOXYGEN_GENERATING_OUTPUT
        explicit Label(const Anchor& anchor, LabelStyle style, Containers::StringView text, const TextProperties& textProperties = {});
        #else
        /* To avoid having to include TextProperties.h */
        explicit Label(const Anchor& anchor, LabelStyle style, Containers::StringView text, const TextProperties& textProperties);
        explicit Label(const Anchor& anchor, LabelStyle style, Containers::StringView text);
        #endif

        /** @brief Style */
        LabelStyle style() const { return _style; }

        /**
         * @brief Set style
         *
         * Note that calling this function doesn't change the font if the new
         * style uses a different one, you have to call @ref setText()
         * afterwards to make it pick it up.
         * @see @ref setIcon(), @ref setText()
         */
        void setStyle(LabelStyle style);

        /**
         * @brief Icon
         *
         * If the label is text-only or has neither an icon nor a text, returns
         * @ref Icon::None.
         */
        Icon icon() const { return _icon; }

        /**
         * @brief Set icon
         *
         * If the label had a text before, it's replaced with the icon. Passing
         * @ref Icon::None makes the label empty.
         * @see @ref setText(), @ref setStyle()
         */
        void setIcon(Icon icon);

        /**
         * @brief Set text
         *
         * If the label had an icon before, it's replaced with a text. Passing
         * an empty @p text makes the label empty.
         * @see @ref setIcon(), @ref setStyle()
         */
        #ifdef DOXYGEN_GENERATING_OUTPUT
        void setText(Containers::StringView text, const TextProperties& textProperties = {});
        #else
        /* To avoid having to include TextProperties.h */
        void setText(Containers::StringView text, const TextProperties& textProperties);
        void setText(Containers::StringView text);
        #endif

        /**
         * @brief Icon / text data or @ref DataHandle::Null
         *
         * Exposed mainly for testing purposes, not meant to be modified
         * directly.
         */
        DataHandle data() const;

    private:
        LabelStyle _style;
        /* 3 bytes free */
        Icon _icon;
        LayerDataHandle _data;
};

/**
@brief Stateless icon label widget
@param anchor           Positioning anchor
@param style            Label style
@param icon             Label icon. Passing @ref Icon::None makes the label
    empty.
@return The @p anchor verbatim
@m_since_latest

Compared to @ref Label::Label(const Anchor&, LabelStyle, Icon)
this creates a stateless label that doesn't have any class instance that would
need to be kept in scope and eventually destructed, making it more lightweight.
As a consequence it can't have its style, icon or text subsequently changed and
is removed only when the node or its parent get removed.
*/
MAGNUM_WHEE_EXPORT Anchor label(const Anchor& anchor, LabelStyle style, Icon icon);

/**
@brief Stateless text label widget
@param anchor           Positioning anchor
@param style            Label style
@param text             Label text. Passing an empty string makes the label
    empty.
@param textProperties   Text shaping and layouting properties
@return The @p anchor verbatim
@m_since_latest

Compared to @ref Label::Label(const Anchor&, LabelStyle, Containers::StringView, const TextProperties&)
this creates a stateless label that doesn't have any class instance that would
need to be kept in scope and eventually destructed, making it more lightweight.
As a consequence it can't have its style, icon or text subsequently changed and
is removed only when the node or its parent get removed.
*/
#ifdef DOXYGEN_GENERATING_OUTPUT
MAGNUM_WHEE_EXPORT Anchor label(const Anchor& anchor, LabelStyle style, Containers::StringView text, const TextProperties& textProperties = {});
#else
/* To avoid having to include TextProperties.h */
MAGNUM_WHEE_EXPORT Anchor label(const Anchor& anchor, LabelStyle style, Containers::StringView text, const TextProperties& textProperties);
MAGNUM_WHEE_EXPORT Anchor label(const Anchor& anchor, LabelStyle style, Containers::StringView text);
#endif

}}

#endif
