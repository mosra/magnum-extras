#ifndef Magnum_Ui_UserInterface_h
#define Magnum_Ui_UserInterface_h
/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019
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

#include <Corrade/Containers/Pointer.h>
#include <Corrade/Interconnect/Emitter.h>
#include <Corrade/PluginManager/PluginManager.h>
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/Text/Text.h>

#include "Magnum/Ui/AbstractUiShader.h"
#include "Magnum/Ui/BasicUserInterface.h"
#include "Magnum/Ui/Style.h"
#include "Magnum/Ui/visibility.h"

namespace Magnum { namespace Ui {

/**
@brief Default user interface

@section Ui-UserInterface-dpi DPI awareness

There are three separate concepts for DPI-aware UI rendering:

-   UI size --- size of the user interface to which all widgets are positioned
-   Window size --- size of the window to which all input events are related
-   Framebuffer size --- size of the framebuffer the UI is being rendered to

Depending on the platform and use case, each of these three values can be
different. For example, a game menu screen can have the UI size the same
regardless of window size. Or on Retina macOS you can have different window and
framebuffer size and the UI size might be related to window size but
independent on the framebuffer size.

When using for example @ref Platform::Sdl2Application or other `*Application`
implementations, you usually have three values at your disposal ---
@ref Platform::Sdl2Application::windowSize() "windowSize()",
@ref Platform::Sdl2Application::framebufferSize() "framebufferSize()" and
@ref Platform::Sdl2Application::dpiScaling() "dpiScaling()". If you want the UI
to have the same layout and just scale on bigger window sizes, pass a fixed
value to the UI size:

@snippet Ui-sdl2.cpp UserInterface-dpi-fixed

If you want the UI to get more room with larger window sizes and behave
properly with different DPI scaling values, pass a ratio of window size and DPI
scaling to the UI size:

@snippet Ui-sdl2.cpp UserInterface-dpi-ratio

Finally, to gracefully deal with extremely small or extremely large windows,
you can apply @ref Math::clamp() on top with some defined bounds. Windows
outside of the reasonable size range will then get a scaled version of the UI
at boundary size:

@snippet Ui-sdl2.cpp UserInterface-dpi-clamp

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
         * @param size                  Size of the user interface to which all
         *      widgets are positioned
         * @param windowSize            Size of the window to which all input
         *      events are related
         * @param framebufferSize       Size of the window framebuffer. On
         *      some platforms with HiDPI screens may be different from window
         *      size.
         * @param extraGlyphs           Extra characters to add to glyph cache
         *
         * Uses @ref defaultStyleConfiguration() and a builtin font with
         * pre-populated glyph cache.
         */
        explicit UserInterface(const Vector2& size, const Vector2i& windowSize, const Vector2i& framebufferSize, const std::string& extraGlyphs = {});

        /** @overload
         *
         * Equivalent to @ref UserInterface(const Vector2&, const Vector2i&, const Vector2i&, const std::string&)
         * with both @p windowSize and @p framebufferSize set to the same
         * value. In order to have crisp rendering on HiDPI screens, it's
         * advised to set those two values separately.
         */
        explicit UserInterface(const Vector2& size, const Vector2i& windowSize, const std::string& extraGlyphs = {}): UserInterface{size, windowSize, windowSize, extraGlyphs} {}

        /**
         * @brief Construct the user interface with a custom style
         * @param size                  Size of the user interface to which all
         *      widgets are positioned
         * @param windowSize            Size of the window to which all input
         *      events are related
         * @param framebufferSize       Size of the window framebuffer. On
         *      some platforms with HiDPI screens may be different from window
         *      size.
         * @param styleConfiguration    Style configuration to use
         * @param extraGlyphs           Extra characters to add to glyph cache
         *
         * Uses a builtin font with pre-populated glyph cache.
         */
        explicit UserInterface(const Vector2& size, const Vector2i& windowSize, const Vector2i& framebufferSize, const StyleConfiguration& styleConfiguration, const std::string& extraGlyphs = {});

        /** @overload
         *
         * Equivalent to @ref UserInterface(const Vector2&, const Vector2i&, const Vector2i&, const StyleConfiguration&, const std::string&)
         * with both @p windowSize and @p framebufferSize set to the same
         * value. In order to have crisp rendering on HiDPI screens, it's
         * advised to set those two values separately.
         */
        explicit UserInterface(const Vector2& size, const Vector2i& windowSize, const StyleConfiguration& styleConfiguration, const std::string& extraGlyphs = {}): UserInterface{size, windowSize, windowSize, styleConfiguration, extraGlyphs} {}

        /**
         * @brief Construct using a external font plugin manager
         * @param fontManager           Font plugin manager
         * @param size                  Size of the user interface to which all
         *      widgets are positioned
         * @param windowSize            Size of the window to which all input
         *      events are related
         * @param framebufferSize       Size of the window framebuffer. On
         *      some platforms with HiDPI screens may be different from window
         *      size.
         * @param extraGlyphs           Extra characters to add to glyph cache
         *
         * Uses @ref defaultStyleConfiguration() and a builtin font with
         * pre-populated glyph cache, loaded from @p fontManager.
         */
        explicit UserInterface(PluginManager::Manager<Text::AbstractFont>& fontManager, const Vector2& size, const Vector2i& windowSize, const Vector2i& framebufferSize, const std::string& extraGlyphs = {});

        /**
         * @brief Construct the user interface with a custom style
         * @param fontManager           Font plugin manager
         * @param size                  Size of the user interface to which all
         *      widgets are positioned
         * @param windowSize            Size of the window to which all input
         *      events are related
         * @param framebufferSize       Size of the window framebuffer. On
         *      some platforms with HiDPI screens may be different from window
         *      size.
         * @param styleConfiguration    Style configuration to use
         * @param extraGlyphs           Extra characters to add to glyph cache
         *
         * Uses a builtin font with pre-populated glyph cache, loaded from
         * @p fontManager.
         */
        explicit UserInterface(PluginManager::Manager<Text::AbstractFont>& fontManager, const Vector2& size, const Vector2i& windowSize, const Vector2i& framebufferSize, const StyleConfiguration& styleConfiguration, const std::string& extraGlyphs = {});

        /** @overload
         *
         * Equivalent to @ref UserInterface(PluginManager::Manager<Text::AbstractFont>&, const Vector2&, const Vector2i&, const Vector2i&, const StyleConfiguration&, const std::string&)
         * with both @p windowSize and @p framebufferSize set to the same
         * value. In order to have crisp rendering on HiDPI screens, it's
         * advised to set those two values separately.
         */
        explicit UserInterface(PluginManager::Manager<Text::AbstractFont>& fontManager, const Vector2& size, const Vector2i& windowSize, const StyleConfiguration& styleConfiguration, const std::string& extraGlyphs = {}): UserInterface{fontManager, size, windowSize, windowSize, styleConfiguration, extraGlyphs} {}

        /**
         * @brief Construct the user interface with custom style and font
         * @param size                  Size of the user interface to which all
         *      widgets are positioned
         * @param windowSize            Size of the window to which all input
         *      events are related
         * @param framebufferSize       Size of the window framebuffer. On
         *      some platforms with HiDPI screens may be different from window
         *      size.
         * @param font                  Font to use
         * @param glyphCache            Populated glyph cache to use
         * @param styleConfiguration    Style configuration to use
         *
         * The @p font and @p glyphCache is expected to be kept in scope for
         * the whole user interface lifetime.
         */
        explicit UserInterface(const Vector2& size, const Vector2i& windowSize, const Vector2i& framebufferSize, Text::AbstractFont& font, Text::GlyphCache& glyphCache, const StyleConfiguration& styleConfiguration);

        /** @overload
         *
         * Equivalent to @ref UserInterface(const Vector2&, const Vector2i&, const Vector2i&, Text::AbstractFont&, Text::GlyphCache&, const StyleConfiguration&)
         * with both @p windowSize and @p framebufferSize set to the same
         * value. In order to have crisp rendering on HiDPI screens, it's
         * advised to set those two values separately.
         */
        explicit UserInterface(const Vector2& size, const Vector2i& windowSize, Text::AbstractFont& font, Text::GlyphCache& glyphCache, const StyleConfiguration& styleConfiguration): UserInterface{size, windowSize, windowSize, font, glyphCache, styleConfiguration} {}

        #ifdef MAGNUM_BUILD_DEPRECATED
        /**
         * @brief Construct the user interface with custom style and font
         * @deprecated Use either @ref UserInterface(const Vector2&, const Vector2i&, const StyleConfiguration&, const std::string&)
         *      or @ref UserInterface(const Vector2&, const Vector2i&, Text::AbstractFont&, Text::GlyphCache&, const StyleConfiguration&)
         *      instead.
         */
        explicit CORRADE_DEPRECATED("use either UserInterface(const Vector2&, const Vector2i&, const StyleConfiguration&, const std::string&) or UserInterface(const Vector2&, const Vector2i&, Text::AbstractFont&, Text::GlyphCache&, const StyleConfiguration&) instead") UserInterface(const Vector2& size, const Vector2i& screenSize, Text::AbstractFont& font, const StyleConfiguration& styleConfiguration);

        /**
         * @brief Construct the user interface with a custom font
         * @deprecated Use either @ref UserInterface(const Vector2&, const Vector2i&, const std::string&)
         *      or @ref UserInterface(const Vector2&, const Vector2i&, Text::AbstractFont&, Text::GlyphCache&, const StyleConfiguration&)
         *      instead.
         */
        explicit CORRADE_DEPRECATED("use either UserInterface(const Vector2&, const Vector2i&, const std::string&) or UserInterface(const Vector2&, const Vector2i&, Text::AbstractFont&, Text::GlyphCache&, const StyleConfiguration&) instead") UserInterface(const Vector2& size, const Vector2i& screenSize, Text::AbstractFont& font);
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

        /**
         * @brief Relayout the user interface
         *
         * Adapts the internal state for a new size / window size.
         *
         * @attention Currently, due to implementation limitations, the
         *      function expects the UI to be empty --- i.e., all planes
         *      attached to it need to be destroyed before and recreated again
         *      after. This will improve in the future.
         * @attention Also, at the moment, the UI assumes the pixel density did
         *      not change. If it did change, you may experience blurry and/or
         *      aliased font look. To fix that, fully reconstruct the instance.
         */
        void relayout(const Vector2& size, const Vector2i& windowSize, const Vector2i& framebufferSize);

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
        Signal inputWidgetFocused();

        /**
         * @brief Input widget was blurred
         *
         * Text input from the application should be started upon signaling
         * this. Currently active input widget is available in
         * @ref focusedInputWidget().
         * @see @ref Input::focused(),
         *      @ref Platform::Sdl2Application::stopTextInput() "Platform::*Application::stopTextInput()"
         */
        Signal inputWidgetBlurred();

    private:
        struct MAGNUM_UI_LOCAL FontState;

        /* Internal constructor used by all the public ones */
        explicit MAGNUM_UI_LOCAL UserInterface(NoCreateT, const Vector2& size, const Vector2i& windowSize, const Vector2i& framebufferSize);

        /* Shared code between constructors taking or not taking an external
           plugin manager */
        void MAGNUM_UI_LOCAL initialize(const Vector2& size, const Vector2i& framebufferSize, const StyleConfiguration& styleConfiguration, const std::string& extraGlyphs);

        GL::Buffer _backgroundUniforms,
            _foregroundUniforms,
            _textUniforms,
            _quadVertices,
            _quadIndices;

        StyleConfiguration _styleConfiguration;

        Implementation::BackgroundShader _backgroundShader;
        Implementation::ForegroundShader _foregroundShader;
        Implementation::TextShader _textShader;

        Containers::Pointer<FontState> _fontState;
        PluginManager::Manager<Text::AbstractFont>* _fontManager;
        Text::AbstractFont* _font;
        Text::GlyphCache* _glyphCache;
        GL::Texture2D _corner;

        Input* _focusedInputWidget = nullptr;
};

}}

#endif
