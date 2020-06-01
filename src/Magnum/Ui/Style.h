#ifndef Magnum_Ui_Style_h
#define Magnum_Ui_Style_h
/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020 Vladimír Vondruš <mosra@centrum.cz>

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
 * @brief Class @ref Magnum::Ui::StyleConfiguration, enum @ref Magnum::Ui::Type, @ref Magnum::Ui::Style
 */

#include <Magnum/Math/Color.h>
#include <Magnum/Math/Range.h>

#include "Magnum/Ui/AbstractUiShader.h"
#include "Magnum/Ui/Ui.h"
#include "Magnum/Ui/visibility.h"

namespace Magnum { namespace Ui {

namespace Implementation {
    enum: std::size_t {
        BackgroundColorCount = 14,
        ForegroundColorCount = 64,
        TextColorCount = 87
    };

    UnsignedByte MAGNUM_UI_EXPORT backgroundColorIndex(Type type, Style style, State state);
    UnsignedByte MAGNUM_UI_EXPORT backgroundColorIndex(Type type, Style style, WidgetFlags flags);
    UnsignedByte MAGNUM_UI_EXPORT foregroundColorIndex(Type type, Style style, State state);
    UnsignedByte MAGNUM_UI_EXPORT foregroundColorIndex(Type type, Style style, WidgetFlags flags);
    UnsignedByte MAGNUM_UI_EXPORT textColorIndex(Type type, Style style, State state);
    UnsignedByte MAGNUM_UI_EXPORT textColorIndex(Type type, Style style, WidgetFlags flags);
}

/**
@brief Widget type

@experimental
*/
enum class Type: UnsignedInt {
    Button = 0,     /**< @ref Button */
    Label = 1,      /**< @ref Label */
    Input = 2,      /**< @ref Input */
    Modal = 3       /**< @ref Modal */
};

/** @debugoperatorenum{Magnum::Ui::Type}
 * @experimental
 */
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, Type value);

/**
@brief Widget state

Extracted from @ref WidgetFlags, see particular values for details.
@see @ref Widget::flags()
@experimental
*/
enum class State: UnsignedInt {
    /** Default state */
    Default = 0,

    /**
     * The widget was hovered. Note that to reduce combinatorial explosion of
     * colors, @ref State::Hover gets a preference over @ref State::Pressed,
     * but not before @ref State::Active (so if the widget is both pressed and
     * hovered, hover style gets a preference).
     */
    Hover = 1,

    /**
     * The widget was pressed. Note that to reduce combinatorial explosion of
     * colors, @ref State::Hover gets a preference over @ref State::Pressed, if
     * the widget is both pressed and hovered.
     */
    Pressed = 2,

    /**
     * The widget is active. Note that to reduce combinatorial explosion of
     * colors, @ref State::Active gets a preference over @ref State::Hover, if
     * the widget is both active and hovered.
     */
    Active = 3,

    /** The widget is disabled. */
    Disabled = 4,

    /** The widget is hidden. This is implicitly mapped to transparent colors. */
    Hidden = 5
};

/** @debugoperatorenum{Magnum::Ui::State}
 * @experimental
 */
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, State value);

/**
@brief Widget style

@experimental
*/
enum class Style: UnsignedInt {
    /** Default. Used for common widgets that don't stand out. */
    Default = 0,

    /** Primary. Denotes primary widget. */
    Primary = 1,

    /** Success. Colored green. */
    Success = 2,

    /** Info. Colored light blue. */
    Info = 3,

    /** Warning. Colored yellow. */
    Warning = 4,

    /** Danger. Colored red. */
    Danger = 5,

    /**
     * Dim. Toned-down version of @ref State::Default that tries hard to not
     * gain attention. Used also for dimming out the background.
     */
    Dim = 6,

    /** Flat version of @ref State::Default. */
    Flat = 7
};

/** @debugoperatorenum{Magnum::Ui::Style}
 * @experimental
 */
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, Style value);

/**
@brief Style configuration

@experimental
*/
class MAGNUM_UI_EXPORT StyleConfiguration {
    public:
        /**
         * @brief Default constructor
         *
         * Sets everything to zero or transparent.
         */
        explicit StyleConfiguration();

        /** @brief Font size */
        Float fontSize() const { return _fontSize; }

        /**
         * @brief Set font size
         * @return Reference to self (for method chaining)
         */
        StyleConfiguration& setFontSize(Float size) {
            _fontSize = size;
            return *this;
        }

        /** @brief Border width */
        Float borderWidth() const { return _foreground.borderWidth; }

        /**
         * @brief Set border width
         * @return Reference to self (for method chaining)
         */
        StyleConfiguration& setBorderWidth(Float width) {
            _foreground.borderWidth = width;
            return *this;
        }

        /** @brief Corner radius */
        Float cornerRadius() const { return _foreground.cornerRadius; }

        /**
         * @brief Set corner radius
         * @return Reference to self (for method chaining)
         */
        StyleConfiguration& setCornerRadius(Float radius) {
            _background.cornerRadius = _foreground.cornerRadius = radius;
            return *this;
        }

        /** @brief Corner smoothness inside */
        Float cornerSmoothnessIn() const { return _foreground.cornerSmoothnessIn; }

        /**
         * @brief Set corner smoothness inside
         * @return Reference to self (for method chaining)
         */
        StyleConfiguration& setCornerSmoothnessIn(Float smoothness) {
            _foreground.cornerSmoothnessIn = smoothness;
            return *this;
        }

        /** @brief Corner smoothness outside */
        Float cornerSmoothnessOut() const { return _foreground.cornerSmoothnessOut; }

        /**
         * @brief Set corner smoothness outside
         * @return Reference to self (for method chaining)
         */
        StyleConfiguration& setCornerSmoothnessOut(Float smoothness) {
            _background.cornerSmoothnessOut = _foreground.cornerSmoothnessOut = smoothness;
            return *this;
        }

        /** @brief Padding inside of a widget */
        Vector2 padding() const { return _padding; }

        /**
         * @brief Set padding inside of a widget
         * @return Reference to self (for method chaining)
         */
        StyleConfiguration& setPadding(const Vector2& padding) {
            _padding = padding;
            return *this;
        }

        /** @brief Margin between widgets */
        Vector2 margin() const { return _margin; }

        /**
         * @brief Set margin between widgets
         * @return Reference to self (for method chaining)
         */
        StyleConfiguration& setMargin(const Vector2& margin) {
            _margin = margin;
            return *this;
        }

        /**
         * @brief Background color
         *
         * Expects that given combination is supported.
         */
        Color4 backgroundColor(Type type, Style style, State state) const {
            return _background.colors[Implementation::backgroundColorIndex(type, style, state)];
        }

        /**
         * @brief Set background color
         * @return Reference to self (for method chaining)
         *
         * Expects that given combination is supported.
         */
        StyleConfiguration& setBackgroundColor(Type type, Style style, State state, const Color4& color) {
            _background.colors[Implementation::backgroundColorIndex(type, style, state)] = color;
            return *this;
        }

        /**
         * @brief Top fill color
         *
         * Expects that given combination is supported.
         */
        Color4 topFillColor(Type type, Style style, State state) const {
            return _foreground.colors[Implementation::foregroundColorIndex(type, style, state)*3 + 0];
        }

        /**
         * @brief Set top fill color
         * @return Reference to self (for method chaining)
         *
         * Top color of the fill gradient. Expects that given combination is
         * supported.
         */
        StyleConfiguration& setTopFillColor(Type type, Style style, State state, const Color4& color) {
            _foreground.colors[Implementation::foregroundColorIndex(type, style, state)*3 + 0] = color;
            return *this;
        }

        /**
         * @brief Bottom fill color
         *
         * Expects that given combination is supported.
         */
        Color4 bottomFillColor(Type type, Style style, State state) const {
            return _foreground.colors[Implementation::foregroundColorIndex(type, style, state)*3 + 1];
        }

        /**
         * @brief Set bottom fill color
         * @return Reference to self (for method chaining)
         *
         * Bottom color of the fill gradient. Expects that given combination is
         * supported.
         */
        StyleConfiguration& setBottomFillColor(Type type, Style style, State state, const Color4& color) {
            _foreground.colors[Implementation::foregroundColorIndex(type, style, state)*3 + 1] = color;
            return *this;
        }

        /**
         * @brief Border color
         *
         * Expects that given combination is supported.
         */
        Color4 borderColor(Type type, Style style, State state) const {
            return _foreground.colors[Implementation::foregroundColorIndex(type, style, state)*3 + 2];
        }

        /**
         * @brief Set border color
         * @return Reference to self (for method chaining)
         *
         * Expects that given combination is supported.
         */
        StyleConfiguration& setBorderColor(Type type, Style style, State state, const Color4& color) {
            _foreground.colors[Implementation::foregroundColorIndex(type, style, state)*3 + 2] = color;
            return *this;
        }

        /**
         * @brief Text color
         *
         * Expects that given combination is supported.
         */
        Color4 textColor(Type type, Style style, State state) const {
            return _text.colors[Implementation::textColorIndex(type, style, state)];
        }

        /**
         * @brief Set text color
         * @return Reference to self (for method chaining)
         *
         * Expects that given combination is supported.
         */
        StyleConfiguration& setTextColor(Type type, Style style, State state, const Color4& color) {
            _text.colors[Implementation::textColorIndex(type, style, state)] = color;
            return *this;
        }

        /** @brief Pack style configuration into OpenGL uniform buffers */
        void pack(GL::Buffer& backgroundUniforms, GL::Buffer& foregroundUniforms, GL::Buffer& textUniforms) const;

    private:
        struct Background {
            Int:32;
            Float cornerRadius{};
            Int:32;
            Float cornerSmoothnessOut{};
            Color4 colors[Implementation::BackgroundColorCount];
        } _background;

        struct Foreground {
            Float borderWidth{};
            Float cornerRadius{};
            Float cornerSmoothnessIn{};
            Float cornerSmoothnessOut{};
            Color4 colors[Implementation::ForegroundColorCount*3];
        } _foreground;

        struct Text {
            Color4 colors[Implementation::TextColorCount];
        } _text;

        Float _fontSize{};
        Vector2 _padding;
        Vector2 _margin;
};

/**
@brief Default style configuration

@experimental
*/
MAGNUM_UI_EXPORT StyleConfiguration defaultStyleConfiguration();

/**
@brief m.css dark style configuration

The dark CSS theme from http://mcss.mosra.cz.
@experimental
*/
MAGNUM_UI_EXPORT StyleConfiguration mcssDarkStyleConfiguration();

namespace Implementation {

struct QuadVertex {
    /* 0---2 0---2 5
       |   | |  / /|
       |   | | / / |
       |   | |/ /  |
       1---3 1 3---4 */
    Vector2 position;

    /*  x = 0 at the left
        y = 0 at the right
        z = 0 at the bottom
        w = 0 at the top */
    Vector4 edgeDistance;
};

struct QuadInstance {
    Range2D rect;
    UnsignedShort colorIndex;
    UnsignedShort:16;
};

struct TextVertex {
    Vector2 position;
    Vector2 textureCoordinates;
    UnsignedShort colorIndex;
    UnsignedShort:16;
};

using QuadLayer = BasicInstancedGLLayer<QuadInstance>;
using TextLayer = BasicGLLayer<TextVertex>;

class AbstractQuadShader: public AbstractUiShader {
    public:
        typedef GL::Attribute<0, Vector2> Position;
        typedef GL::Attribute<1, Vector4> EdgeDistance;
        typedef GL::Attribute<2, Vector4> Rect;
        typedef GL::Attribute<3, Int> ColorIndex;

        AbstractQuadShader& bindCornerTexture(GL::Texture2D& texture);
};

class MAGNUM_UI_EXPORT BackgroundShader: public AbstractQuadShader {
    public:
        explicit BackgroundShader();

        BackgroundShader& bindCornerTexture(GL::Texture2D& texture) {
            AbstractQuadShader::bindCornerTexture(texture);
            return *this;
        }
        BackgroundShader& bindStyleBuffer(GL::Buffer& buffer);
};

class MAGNUM_UI_EXPORT ForegroundShader: public AbstractQuadShader {
    public:
        explicit ForegroundShader();

        ForegroundShader& bindCornerTexture(GL::Texture2D& texture) {
            AbstractQuadShader::bindCornerTexture(texture);
            return *this;
        }
        ForegroundShader& bindStyleBuffer(GL::Buffer& buffer);
};

class MAGNUM_UI_EXPORT TextShader: public AbstractUiShader {
    public:
        typedef GL::Attribute<0, Vector2> Position;
        typedef GL::Attribute<1, Vector2> TextureCoordinates;
        typedef GL::Attribute<2, Int> ColorIndex;

        explicit TextShader();

        TextShader& bindGlyphCacheTexture(GL::Texture2D& texture);
        TextShader& bindStyleBuffer(GL::Buffer& buffer);
};

}

}}

#endif
