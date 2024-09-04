#ifndef Magnum_Ui_Input_h
#define Magnum_Ui_Input_h
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
 * @brief Class @ref Magnum::Ui::Input, enum @ref Magnum::Ui::InputStyle
 * @m_since_latest
 */

#include "Magnum/Ui/Widget.h"

namespace Magnum { namespace Ui {

/**
@brief Input style
@m_since_latest

@see @ref Input
*/
enum class InputStyle: UnsignedByte {
    Default,        /**< Default */
    Success,        /**< Success */
    Warning,        /**< Warning */
    Danger,         /**< Danger */
    Flat,           /**< Flat */
};

/**
@debugoperatorenum{InputStyle}
@m_since_latest
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, InputStyle value);

/**
@brief Input widget
@m_since_latest
*/
class MAGNUM_UI_EXPORT Input: public Widget {
    public:
        /**
         * @brief Constructor
         * @param anchor            Positioning anchor
         * @param text              Pre-filled input text
         * @param textProperties    Text shaping and layouting properties
         * @param style             Input style
         */
        explicit Input(const Anchor& anchor, Containers::StringView text, const TextProperties& textProperties, InputStyle style = InputStyle::Default);
        /** @overload */
        explicit Input(const Anchor& anchor, Containers::StringView text, InputStyle style = InputStyle::Default);

        /**
         * @brief Construct with no underlying node
         *
         * The instance is equivalent to a moved-out state, i.e. not usable
         * for anything. Move another instance over it to make it useful.
         */
        explicit Input(NoCreateT, UserInterface& ui): Widget{NoCreate, ui}, _style{}, _backgroundData{}, _textData{} {}

        /** @brief Style */
        InputStyle style() const { return _style; }

        /**
         * @brief Set style
         *
         * Note that calling this function doesn't change the font if the new
         * style uses a different one, you have to call @ref setText()
         * afterwards to make it pick it up.
         * @see @ref setText()
         */
        void setStyle(InputStyle style);

        /**
         * @brief Text
         *
         * The returned view is valid only until any text is created or updated
         * on the user interface text layer.
         */
        Containers::StringView text() const;

        /**
         * @brief Set text
         *
         * @see @ref setStyle()
         */
        #ifdef DOXYGEN_GENERATING_OUTPUT
        void setText(Containers::StringView text, const TextProperties& textProperties = {});
        #else
        /* To avoid having to include TextProperties.h */
        void setText(Containers::StringView text, const TextProperties& textProperties);
        void setText(Containers::StringView text);
        #endif

        /**
         * @brief Background data
         *
         * Exposed mainly for testing purposes, not meant to be modified
         * directly.
         */
        DataHandle backgroundData() const;

        /**
         * @brief Text data or @ref DataHandle::Null
         *
         * Exposed mainly for testing purposes, not meant to be modified
         * directly.
         */
        DataHandle textData() const;

    private:
        InputStyle _style;
        /* 3 bytes free */
        LayerDataHandle _backgroundData, _textData;
};

}}

#endif
