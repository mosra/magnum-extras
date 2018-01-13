#ifndef Magnum_Ui_UserInterface_h
#define Magnum_Ui_UserInterface_h
/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018
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
 * @brief Class @ref Magnum::Ui::UserInterface
 */

#include <memory>
#include <Corrade/Interconnect/Emitter.h>
#include <Magnum/Buffer.h>
#include <Magnum/Texture.h>
#include <Magnum/Text/Text.h>

#include "Magnum/Ui/AbstractUiShader.h"
#include "Magnum/Ui/BasicUserInterface.h"
#include "Magnum/Ui/Style.h"
#include "Magnum/Ui/visibility.h"

namespace Magnum { namespace Ui {

/**
@brief Default user interface

All positioning and sizing inside the interface is done in regard to the size
parameter passed to the constructor, without taking actual screen size into
account. This allows to have DPI-independent sizes.

@section Ui-UserInterface-fonts Font plugins

Unless you pass your own font instance via
@ref UserInterface(const Vector2&, const Vector2i&, Text::AbstractFont&, Text::GlyphCache&, const StyleConfiguration&),
the constructor expects that it can load some @cpp "TrueTypeFont" @ce plugin.
The plugin should be either in a system directory or be linked and correctly
imported statically. If the plugin cannot be loaded, the application exits. See
@ref plugins for more information.

@see @ref defaultStyleConfiguration(), @ref mcssDarkStyleConfiguration()
@experimental
*/
class MAGNUM_UI_EXPORT UserInterface: public BasicUserInterface<Implementation::QuadLayer, Implementation::QuadLayer, Implementation::TextLayer>, public Interconnect::Emitter {
    friend Input;
    friend Plane;

    public:
        /**
         * @brief Constructor
         * @param size                  User interface size
         * @param screenSize            Actual screen size
         * @param extraGlyphs           Extra characters to add to glyph cache
         *
         * Uses @ref defaultStyleConfiguration() and a builtin font with
         * pre-populated glyph cache.
         */
        explicit UserInterface(const Vector2& size, const Vector2i& screenSize, const std::string& extraGlyphs = {});

        /**
         * @brief Construct the user interface with a custom style
         * @param size                  User interface size
         * @param screenSize            Actual screen size
         * @param styleConfiguration    Style configuration to use
         * @param extraGlyphs           Extra characters to add to glyph cache
         *
         * Uses a builtin font with pre-populated glyph cache.
         */
        explicit UserInterface(const Vector2& size, const Vector2i& screenSize, const StyleConfiguration& styleConfiguration, const std::string& extraGlyphs = {});

        /**
         * @brief Construct the user interface with custom style and font
         * @param size                  User interface size
         * @param screenSize            Actual screen size
         * @param font                  Font to use
         * @param glyphCache            Populated glyph cache to use
         * @param styleConfiguration    Style configuration to use
         *
         * The @p font and @p glyphCache is expected to be kept in scope for
         * the whole user interface lifetime.
         */
        explicit UserInterface(const Vector2& size, const Vector2i& screenSize, Text::AbstractFont& font, Text::GlyphCache& glyphCache, const StyleConfiguration& styleConfiguration);

        #ifdef MAGNUM_BUILD_DEPRECATED
        /**
         * @brief Construct the user interface with custom style and font
         * @deprecated Use either @ref UserInterface(const Vector2&, const Vector2i&, const StyleConfiguration&)
         *      or @ref UserInterface(const Vector2&, const Vector2i&, Text::AbstractFont&, Text::GlyphCache&, const StyleConfiguration&)
         *      instead.
         */
        explicit CORRADE_DEPRECATED("use either UserInterface(const Vector2&, const Vector2i&) or UserInterface(const Vector2&, const Vector2i&, Text::AbstractFont&, Text::GlyphCache&, const StyleConfiguration&) instead") UserInterface(const Vector2& size, const Vector2i& screenSize, Text::AbstractFont& font, const StyleConfiguration& styleConfiguration);

        /**
         * @brief Construct the user interface with a custom font
         * @deprecated Use either @ref UserInterface(const Vector2&, const Vector2i&)
         *      or @ref UserInterface(const Vector2&, const Vector2i&, Text::AbstractFont&, Text::GlyphCache&, const StyleConfiguration&)
         *      instead.
         */
        explicit CORRADE_DEPRECATED("use either UserInterface(const Vector2&, const Vector2i&) or UserInterface(const Vector2&, const Vector2i&, Text::AbstractFont&, Text::GlyphCache&, const StyleConfiguration&) instead") UserInterface(const Vector2& size, const Vector2i& screenSize, Text::AbstractFont& font);
        #endif

        ~UserInterface();

        /** @brief Active plane */
        Plane* activePlane();
        const Plane* activePlane() const; /**< @overload */

        /** @brief Used style configuration */
        const StyleConfiguration& styleConfiguration() const { return _styleConfiguration; }

        /**
         * @brief Set style configuration
         *
         * Note that everything set prior to this call will have undefined
         * contents after calling this function. Reset the contents and fill
         * the interface again.
         */
        void setStyleConfiguration(const StyleConfiguration& configuration);

        /** @brief Font used for the interface */
        const Text::AbstractFont& font() const { return *_font; }

        /**
         * @brief Currently focused input widget
         *
         * Input widget that should receive text input from the application.
         * Returns `nullptr` if there is no active text input widget.
         * @see @ref inputWidgetFocused(), @ref inputWidgetBlurred(),
         *      @ref Input::handleKeyPress(), @ref Input::handleTextInput()
         */
        Input* focusedInputWidget() { return _focusedInputWidget; }

        /** @brief Draw the user interface */
        void draw();

        /**
         * @brief Input widget was focused
         *
         * Text input from the application should be started upon signaling
         * this. Currently active input widget is available in
         * @ref focusedInputWidget().
         * @see @ref Input::focused(),
         *      @ref Platform::Sdl2Application::startTextInput() "Platform::*Application::startTextInput()"
         */
        Signal inputWidgetFocused() {
            return emit(&UserInterface::inputWidgetFocused);
        }

        /**
         * @brief Input widget was blurred
         *
         * Text input from the application should be started upon signaling
         * this. Currently active input widget is available in
         * @ref focusedInputWidget().
         * @see @ref Input::focused(),
         *      @ref Platform::Sdl2Application::stopTextInput() "Platform::*Application::stopTextInput()"
         */
        Signal inputWidgetBlurred() {
            return emit(&UserInterface::inputWidgetBlurred);
        }

    private:
        struct MAGNUM_UI_LOCAL FontState;

        /* Internal constructor used by all the public ones */
        explicit UserInterface(NoCreateT, const Vector2& size, const Vector2i& screenSize);

        Buffer _backgroundUniforms,
            _foregroundUniforms,
            _textUniforms,
            _quadVertices,
            _quadIndices;

        StyleConfiguration _styleConfiguration;

        Implementation::BackgroundShader _backgroundShader;
        Implementation::ForegroundShader _foregroundShader;
        Implementation::TextShader _textShader;

        std::unique_ptr<FontState> _fontState;
        Text::AbstractFont* _font;
        Text::GlyphCache* _glyphCache;
        Texture2D _corner;

        Input* _focusedInputWidget = nullptr;
};

}}

#endif
