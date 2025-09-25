#ifndef Magnum_Ui_UserInterfaceGL_h
#define Magnum_Ui_UserInterfaceGL_h
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
 * @brief Class @ref Magnum::Ui::UserInterfaceGL
 * @m_since_latest
 */

#include "Magnum/configure.h"

#ifdef MAGNUM_TARGET_GL
#include <Corrade/PluginManager/PluginManager.h>
#include <Magnum/Text/Text.h>
#include <Magnum/Trade/Trade.h>

#include "Magnum/Ui/UserInterface.h"

namespace Magnum { namespace Ui {

/**
@brief OpenGL implementation of the main user interface
@m_since_latest

Owns the whole user interface, providing everything from input event handling
to animation and drawing. Compared to @ref AbstractUserInterface includes
everything that's needed by builtin widgets, while the @ref UserInterface base
class is a common interface not tied to OpenGL.

@note This class is available only if Magnum is compiled with
    @ref MAGNUM_TARGET_GL enabled (done by default). See @ref building-features
    for more information.

@section Ui-UserInterfaceGL-setup Setting up a user interface instance

The simplest variant of the constructor takes a UI size, in respect to which
all contents as well as input events get positioned, and a style instance
describing how the widgets all look like. At the moment, @ref McssDarkStyle is
the only style provided by the library itself.

@snippet Ui-gl.cpp UserInterfaceGL-setup

The rest of the setup --- drawing and event handling --- is the same for all
@ref AbstractUserInterface subclasses, see its documentation for details.

@subsection Ui-UserInterfaceGL-setup-options Additional setup options

The above by default populates the user interface with everything a style
provides for use by builtin widgets --- in particular, making @ref baseLayer(),
@ref textLayer(), @ref eventLayer() and @ref snapLayouter() all available. In
case you for example use just a subset of the builtin widgets that only need a
part of the above, you can specify a @ref StyleFeatures subset. This can be
further combined with @ref setStyle(), where, as long as you specify
non-overlapping sets of @ref StyleFeatures, you can combine multiple styles
together:

@snippet Ui-gl.cpp UserInterfaceGL-setup-features

The constructors also provide a way to supply external plugin managers for
fonts and images, for example if you want to configure the plugins before
they're used or if you're going to use the same plugin managers elsewhere and
want to reduce duplication. The passed instances are expected to stay alive for
the whole user interface lifetime.

@snippet Ui-gl.cpp UserInterfaceGL-setup-managers

@subsection Ui-UserInterfaceGL-setup-delayed Delayed user interface creation

By default, the class expects that a Magnum OpenGL context is available at the
point of construction. If you're using @ref platform-configuration-delayed "delayed Application context creation"
or if you just need additional logic before creating the UI, you can employ a
similar approach as with the application itself --- construct with
@ref UserInterfaceGL(NoCreateT) and then call @ref create() once you're ready:

@snippet Ui-sdl2.cpp UserInterfaceGL-setup-delayed

The @ref create() as well as the main constructor both exit the application if
something goes wrong such as if a font plugin cannot be loaded. If you want to
deal with potential errors more gracefully or try out several options,
@ref tryCreate() returns @cpp false @ce instead of exiting, and there's a
@ref trySetStyle() counterpart as well.

@subsection Ui-UserInterfaceGL-setup-renderer Supplying a custom renderer instance

Setting a style either in the constructor or in @ref create() / @ref tryCreate()
implicitly sets up a @ref RendererGL instance. If you want to supply a custom
one --- for example to set up a @ref Ui-RendererGL-compositing-framebuffer "compositing framebuffer"
for a custom layer --- pass it to @ref setRendererInstance() and then call
@ref setSize() and @ref setStyle() / @ref trySetStyle() instead of
@ref create() / @ref tryCreate():

@snippet Ui-gl.cpp UserInterfaceGL-setup-renderer

@subsection Ui-UserInterfaceGL-setup-layers Supplying custom layer and layouter instances

If a constructor or @ref create() taking a style isn't used at all, or if a
style is applied excluding a particular layer or layouter, you can supply a
custom instance using @ref setBaseLayerInstance(), @ref setTextLayerInstance(),
@ref setEventLayerInstance() or @ref setSnapLayouterInstance(). Note that
however, at this point, you're on your own when you attempt to use any builtin
widgets that rely on given instance being set up in a particular way.

@snippet Ui-gl.cpp UserInterfaceGL-setup-layer
*/
class MAGNUM_UI_EXPORT UserInterfaceGL: public UserInterface {
    public:
        /**
         * @brief Construct without creating the user interface
         *
         * You're expected to call @ref create() or @ref tryCreate() afterwards
         * in order to define the UI size and coordinate scaling and set up a
         * style.
         */
        explicit UserInterfaceGL(NoCreateT);

        /**
         * @brief Construct
         * @param size              Size of the user interface to which
         *      everything is positioned
         * @param windowSize        Size of the window to which all input
         *      events are related
         * @param framebufferSize   Size of the window framebuffer. On some
         *      platforms with HiDPI screens may be different from window size.
         * @param style             Style instance to use
         * @param importerManager   Optional plugin manager instance for image
         *      loading
         * @param fontManager       Optional plugin manager instance for font
         *      loading
         *
         * Equivalent to constructing with @ref UserInterfaceGL(NoCreateT)
         * and then calling @ref create(const Vector2&, const Vector2&, const Vector2i&, const AbstractStyle&, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*).
         * See documentation of these functions for more information. In
         * particular, if style application fails, the program exits. Use the
         * @ref UserInterfaceGL(NoCreateT) constructor in combination with
         * @ref tryCreate() for a more graceful failure handling.
         */
        explicit UserInterfaceGL(const Vector2& size, const Vector2& windowSize, const Vector2i& framebufferSize, const AbstractStyle& style, PluginManager::Manager<Trade::AbstractImporter>* importerManager = nullptr, PluginManager::Manager<Text::AbstractFont>* fontManager = nullptr): UserInterfaceGL{NoCreate} {
            create(size, windowSize, framebufferSize, style, importerManager, fontManager);
        }

        /**
         * @brief Construct with properties taken from an application instance
         * @param application       Application instance to query properties
         *      from
         * @param style             Style instance to use
         * @param importerManager   Optional plugin manager instance for image
         *      loading
         * @param fontManager       Optional plugin manager instance for font
         *      loading
         *
         * Equivalent to constructing with @ref UserInterfaceGL(NoCreateT)
         * and then calling @ref create(const Application&, const AbstractStyle&, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*).
         * See documentation of these functions for more information. In
         * particular, if style application fails, the program exits. Use the
         * @ref UserInterfaceGL(NoCreateT) constructor in combination with
         * @ref tryCreate() for a more graceful failure handling.
         */
        template<class Application, class = decltype(Implementation::ApplicationSizeConverter<Application>::set(std::declval<AbstractUserInterface&>(), std::declval<const Application&>()))> explicit UserInterfaceGL(const Application& application, const AbstractStyle& style, PluginManager::Manager<Trade::AbstractImporter>* importerManager = nullptr, PluginManager::Manager<Text::AbstractFont>* fontManager = nullptr): UserInterfaceGL{NoCreate} {
            create(application, style, importerManager, fontManager);
        }

        /**
         * @brief Construct with a subset of the style
         * @param size              Size of the user interface to which
         *      everything is positioned
         * @param windowSize        Size of the window to which all input
         *      events are related
         * @param framebufferSize   Size of the window framebuffer. On some
         *      platforms with HiDPI screens may be different from window size.
         * @param style             Style instance to use
         * @param styleFeatures     Style features to apply
         * @param importerManager   Optional plugin manager instance for image
         *      loading
         * @param fontManager       Optional plugin manager instance for font
         *      loading
         *
         * Equivalent to constructing with @ref UserInterfaceGL(NoCreateT)
         * and then calling @ref create(const Vector2&, const Vector2&, const Vector2i&, const AbstractStyle&, StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*).
         * See documentation of these functions for more information. In
         * particular, if style application fails, the program exits. Use the
         * @ref UserInterfaceGL(NoCreateT) constructor in combination with
         * @ref tryCreate() for a more graceful failure handling.
         */
        explicit UserInterfaceGL(const Vector2& size, const Vector2& windowSize, const Vector2i& framebufferSize, const AbstractStyle& style, StyleFeatures styleFeatures, PluginManager::Manager<Trade::AbstractImporter>* importerManager = nullptr, PluginManager::Manager<Text::AbstractFont>* fontManager = nullptr): UserInterfaceGL{NoCreate} {
            create(size, windowSize, framebufferSize, style, styleFeatures, importerManager, fontManager);
        }

        /**
         * @brief Construct with a subset of the style with properties taken from an application instance
         * @param application       Application instance to query properties
         *      from
         * @param style             Style instance to use
         * @param styleFeatures     Style features to apply
         * @param importerManager   Optional plugin manager instance for image
         *      loading
         * @param fontManager       Optional plugin manager instance for font
         *      loading
         *
         * Equivalent to constructing with @ref UserInterfaceGL(NoCreateT)
         * and then calling @ref create(const Application&, const AbstractStyle&, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*).
         * See documentation of these functions for more information. In
         * particular, if style application fails, the program exits. Use the
         * @ref UserInterfaceGL(NoCreateT) constructor in combination with
         * @ref tryCreate() for a more graceful failure handling.
         */
        template<class Application, class = decltype(Implementation::ApplicationSizeConverter<Application>::set(std::declval<AbstractUserInterface&>(), std::declval<const Application&>()))> explicit UserInterfaceGL(const Application& application, const AbstractStyle& style, StyleFeatures styleFeatures, PluginManager::Manager<Trade::AbstractImporter>* importerManager = nullptr, PluginManager::Manager<Text::AbstractFont>* fontManager = nullptr): UserInterfaceGL{NoCreate} {
            create(application, style, styleFeatures, importerManager, fontManager);
        }

        /**
         * @brief Construct with an unscaled size
         *
         * Delegates to @ref UserInterfaceGL(const Vector2&, const Vector2&, const Vector2i&, const AbstractStyle&, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*)
         * with all sizes set to @p size. Doing so assumes that the coordinate
         * system in which events are passed matches framebuffer size.
         */
        explicit UserInterfaceGL(const Vector2i& size, const AbstractStyle& style, PluginManager::Manager<Trade::AbstractImporter>* importerManager = nullptr, PluginManager::Manager<Text::AbstractFont>* fontManager = nullptr);

        /**
         * @brief Construct with an unscaled size and a subset of the style
         *
         * Delegates to @ref UserInterfaceGL(const Vector2&, const Vector2&, const Vector2i&, const AbstractStyle&, StyleFeatures features, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*)
         * with all sizes set to @p size. Doing so assumes that the coordinate
         * system in which events are passed matches framebuffer size.
         */
        explicit UserInterfaceGL(const Vector2i& size, const AbstractStyle& style, StyleFeatures styleFeatures, PluginManager::Manager<Trade::AbstractImporter>* importerManager = nullptr, PluginManager::Manager<Text::AbstractFont>* fontManager = nullptr);

        /**
         * @brief Create the user interface
         * @param size              Size of the user interface to which
         *      everything is positioned
         * @param windowSize        Size of the window to which all input
         *      events are related
         * @param framebufferSize   Size of the window framebuffer. On some
         *      platforms with HiDPI screens may be different from window size.
         * @param style             Style instance to use
         * @param importerManager   Optional plugin manager instance for image
         *      loading
         * @param fontManager       Optional plugin manager instance for font
         *      loading
         * @return Reference to self (for method chaining)
         *
         * Expects that none of @ref create(), @ref tryCreate(),
         * @ref setBaseLayerInstance(), @ref setTextLayerInstance(),
         * @ref setEventLayerInstance() or @ref setRendererInstance() was
         * called yet. Equivalent to calling
         * @ref setSize(const Vector2&, const Vector2&, const Vector2i&)
         * followed by @ref setStyle(const AbstractStyle&, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*).
         * See documentation of these functions for more information and
         * alternative ways to create the user interface. If style application
         * fails during the creation process, the program exits. Use
         * @ref tryCreate() for a more graceful failure handling.
         */
        UserInterfaceGL& create(const Vector2& size, const Vector2& windowSize, const Vector2i& framebufferSize, const AbstractStyle& style, PluginManager::Manager<Trade::AbstractImporter>* importerManager = nullptr, PluginManager::Manager<Text::AbstractFont>* fontManager = nullptr) {
            setSize(size, windowSize, framebufferSize);
            return createInternal(style, importerManager, fontManager);
        }

        /**
         * @brief Create the user interface with properties taken from an application instance
         * @param application       Application instance to query properties
         *      from
         * @param style             Style instance to use
         * @param importerManager   Optional plugin manager instance for image
         *      loading
         * @param fontManager       Optional plugin manager instance for font
         *      loading
         * @return Reference to self (for method chaining)
         *
         * Expects that none of @ref create(), @ref tryCreate(),
         * @ref setBaseLayerInstance(), @ref setTextLayerInstance(),
         * @ref setEventLayerInstance() or @ref setRendererInstance() was
         * called yet. Equivalent to calling
         * @ref setSize(const ApplicationOrViewportEvent&) followed by
         * @ref setStyle(const AbstractStyle&, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*).
         * See documentation of these functions for more information and
         * alternative ways to create the user interface. If style application
         * fails during the creation process, the program exits. Use
         * @ref tryCreate() for a more graceful failure handling.
         */
        template<class Application, class = decltype(Implementation::ApplicationSizeConverter<Application>::set(std::declval<AbstractUserInterface&>(), std::declval<const Application&>()))> UserInterfaceGL& create(const Application& application, const AbstractStyle& style, PluginManager::Manager<Trade::AbstractImporter>* importerManager = nullptr, PluginManager::Manager<Text::AbstractFont>* fontManager = nullptr) {
            setSize(application);
            return createInternal(style, importerManager, fontManager);
        }

        /**
         * @brief Create the user interface with a subset of the style
         * @param size              Size of the user interface to which
         *      everything is positioned
         * @param windowSize        Size of the window to which all input
         *      events are related
         * @param framebufferSize   Size of the window framebuffer. On some
         *      platforms with HiDPI screens may be different from window size.
         * @param style             Style instance to use
         * @param styleFeatures     Style features to apply
         * @param importerManager   Optional plugin manager instance for image
         *      loading
         * @param fontManager       Optional plugin manager instance for font
         *      loading
         * @return Reference to self (for method chaining)
         *
         * Expects that none of @ref create(), @ref tryCreate(),
         * @ref setBaseLayerInstance(), @ref setTextLayerInstance(),
         * @ref setEventLayerInstance(), @ref setSnapLayouterInstance() or
         * @ref setRendererInstance() was called yet. Equivalent to calling
         * @ref setSize(const Vector2&, const Vector2&, const Vector2i&)
         * followed by @ref setStyle(const AbstractStyle&, StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*).
         * See documentation of these functions for more information and
         * alternative ways to create the user interface. If style application
         * fails during the creation process, the program exits. Use
         * @ref tryCreate() for a more graceful failure handling.
         */
        UserInterfaceGL& create(const Vector2& size, const Vector2& windowSize, const Vector2i& framebufferSize, const AbstractStyle& style, StyleFeatures styleFeatures, PluginManager::Manager<Trade::AbstractImporter>* importerManager = nullptr, PluginManager::Manager<Text::AbstractFont>* fontManager = nullptr) {
            setSize(size, windowSize, framebufferSize);
            return createInternal(style, styleFeatures, importerManager, fontManager);
        }

        /**
         * @brief Create the user interface with a subset of the style with properties taken from an application instance
         * @param application       Application instance to query properties
         *      from
         * @param style             Style instance to use
         * @param styleFeatures     Style features to apply
         * @param importerManager   Optional plugin manager instance for image
         *      loading
         * @param fontManager       Optional plugin manager instance for font
         *      loading
         * @return Reference to self (for method chaining)
         *
         * Expects that none of @ref create(), @ref tryCreate(),
         * @ref setBaseLayerInstance(), @ref setTextLayerInstance(),
         * @ref setEventLayerInstance() or @ref setRendererInstance() was
         * called yet. Equivalent to calling
         * @ref setSize(const ApplicationOrViewportEvent&) followed by
         * @ref setStyle(const AbstractStyle&, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*).
         * See documentation of these functions for more information and
         * alternative ways to create the user interface. If style application
         * fails during the creation process, the program exits. Use
         * @ref tryCreate() for a more graceful failure handling.
         */
        template<class Application, class = decltype(Implementation::ApplicationSizeConverter<Application>::set(std::declval<AbstractUserInterface&>(), std::declval<const Application&>()))> UserInterfaceGL& create(const Application& application, const AbstractStyle& style, StyleFeatures styleFeatures, PluginManager::Manager<Trade::AbstractImporter>* importerManager = nullptr, PluginManager::Manager<Text::AbstractFont>* fontManager = nullptr) {
            setSize(application);
            return createInternal(style, styleFeatures, importerManager, fontManager);
        }

        /**
         * @brief Create the user interface with an unscaled size
         *
         * Delegates to @ref create(const Vector2&, const Vector2&, const Vector2i&, const AbstractStyle&, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*)
         * with all sizes set to @p size. Doing so assumes that the coordinate
         * system in which events are passed matches framebuffer size.
         */
        UserInterfaceGL& create(const Vector2i& size, const AbstractStyle& style, PluginManager::Manager<Trade::AbstractImporter>* importerManager = nullptr, PluginManager::Manager<Text::AbstractFont>* fontManager = nullptr);

        /**
         * @brief Create the user interface with an unscaled size and a subset of the style
         *
         * Delegates to @ref create(const Vector2&, const Vector2&, const Vector2i&, const AbstractStyle&, StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*)
         * with all sizes set to @p size. Doing so assumes that the coordinate
         * system in which events are passed matches framebuffer size.
         */
        UserInterfaceGL& create(const Vector2i& size, const AbstractStyle& style, StyleFeatures styleFeatures, PluginManager::Manager<Trade::AbstractImporter>* importerManager = nullptr, PluginManager::Manager<Text::AbstractFont>* fontManager = nullptr);

        /**
         * @brief Try to create the user interface
         *
         * Unlike @ref create(const Vector2&, const Vector2&, const Vector2i&, const AbstractStyle&, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*)
         * returns @cpp false @ce if @ref AbstractStyle::apply() failed instead
         * of exiting, @cpp true @ce otherwise. Equivalent to calling
         * @ref setSize(const Vector2&, const Vector2&, const Vector2i&)
         * followed by @ref trySetStyle(const AbstractStyle&, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*).
         */
        bool tryCreate(const Vector2& size, const Vector2& windowSize, const Vector2i& framebufferSize, const AbstractStyle& style, PluginManager::Manager<Trade::AbstractImporter>* importerManager = nullptr, PluginManager::Manager<Text::AbstractFont>* fontManager = nullptr) {
            setSize(size, windowSize, framebufferSize);
            return tryCreateInternal(style, importerManager, fontManager);
        }

        /**
         * @brief Try to create the user interface with properties taken from an application instance
         *
         * Unlike @ref create(const Application&, const AbstractStyle&, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*)
         * returns @cpp false @ce if @ref AbstractStyle::apply() failed instead
         * of exiting, @cpp true @ce otherwise. Equivalent to calling
         * @ref setSize(const ApplicationOrViewportEvent&) followed by
         * @ref trySetStyle(const AbstractStyle&, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*).
         */
        template<class Application, class = decltype(Implementation::ApplicationSizeConverter<Application>::set(std::declval<AbstractUserInterface&>(), std::declval<const Application&>()))> bool tryCreate(const Application& application, const AbstractStyle& style, PluginManager::Manager<Trade::AbstractImporter>* importerManager = nullptr, PluginManager::Manager<Text::AbstractFont>* fontManager = nullptr) {
            setSize(application);
            return tryCreateInternal(style, importerManager, fontManager);
        }

        /**
         * @brief Try to create the user interface with a subset of the style
         *
         * Unlike @ref create(const Vector2&, const Vector2&, const Vector2i&, const AbstractStyle&, StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*)
         * returns @cpp false @ce if @ref AbstractStyle::apply() failed instead
         * of exiting, @cpp true @ce otherwise. Equivalent to calling
         * @ref setSize(const Vector2&, const Vector2&, const Vector2i&)
         * followed by @ref trySetStyle(const AbstractStyle&, StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*).
         */
        bool tryCreate(const Vector2& size, const Vector2& windowSize, const Vector2i& framebufferSize, const AbstractStyle& style, StyleFeatures styleFeatures, PluginManager::Manager<Trade::AbstractImporter>* importerManager = nullptr, PluginManager::Manager<Text::AbstractFont>* fontManager = nullptr) {
            setSize(size, windowSize, framebufferSize);
            return tryCreateInternal(style, styleFeatures, importerManager, fontManager);
        }

        /**
         * @brief Try to create the user interface with a subset of the style with properties taken from an application instance
         *
         * Unlike @ref create(const Application&, const AbstractStyle&, StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*)
         * returns @cpp false @ce if @ref AbstractStyle::apply() failed instead
         * of exiting, @cpp true @ce otherwise. Equivalent to calling
         * @ref setSize(const ApplicationOrViewportEvent&) followed by
         * @ref trySetStyle(const AbstractStyle&, StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*).
         */
        template<class Application, class = decltype(Implementation::ApplicationSizeConverter<Application>::set(std::declval<AbstractUserInterface&>(), std::declval<const Application&>()))> bool tryCreate(const Application& application, const AbstractStyle& style, StyleFeatures styleFeatures, PluginManager::Manager<Trade::AbstractImporter>* importerManager = nullptr, PluginManager::Manager<Text::AbstractFont>* fontManager = nullptr) {
            setSize(application);
            return tryCreateInternal(style, styleFeatures, importerManager, fontManager);
        }

        /**
         * @brief Try to create the user interface with an unscaled size
         *
         * Unlike @ref create(const Vector2i&, const AbstractStyle&, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*)
         * returns @cpp false @ce if @ref AbstractStyle::apply() failed instead
         * of exiting, @cpp true @ce otherwise. Equivalent to calling
         * @ref setSize(const Vector2i&) followed by
         * @ref trySetStyle(const AbstractStyle&, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*).
         */
        bool tryCreate(const Vector2i& size, const AbstractStyle& style, PluginManager::Manager<Trade::AbstractImporter>* importerManager = nullptr, PluginManager::Manager<Text::AbstractFont>* fontManager = nullptr);

        /**
         * @brief Try to create the user interface with an unscaled size and a subset of the style
         *
         * Unlike @ref create(const Vector2i&, const AbstractStyle&, StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*)
         * returns @cpp false @ce if @ref AbstractStyle::apply() failed instead
         * of exiting, @cpp true @ce otherwise. Equivalent to calling
         * @ref setSize(const Vector2i&) followed by
         * @ref trySetStyle(const AbstractStyle&, StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*).
         */
        bool tryCreate(const Vector2i& size, const AbstractStyle& style, StyleFeatures styleFeatures, PluginManager::Manager<Trade::AbstractImporter>* importerManager = nullptr, PluginManager::Manager<Text::AbstractFont>* fontManager = nullptr);

        /**
         * @brief Set renderer instance
         * @return Reference to self (for method chaining)
         *
         * Expects that the instance hasn't been set yet, either by this
         * function or transitively either by @ref UserInterfaceGL::setStyle()
         * or a @ref UserInterfaceGL constructor taking a style instance. The
         * instance is subsequently available through @ref renderer().
         * @see @ref hasRendererInstance()
         */
        UserInterfaceGL& setRendererInstance(Containers::Pointer<RendererGL>&& instance);

        /**
         * @brief Renderer instance
         *
         * Expects that an instance has been set, either by
         * @ref setRendererInstance() or transitively by
         * @ref UserInterfaceGL::setStyle() or a @ref UserInterfaceGL
         * constructor taking a style instance.
         */
        RendererGL& renderer();
        const RendererGL& renderer() const; /**< @overload */

        /**
         * @brief Set features from a style
         * @param style             Style instance
         * @param features          Style features to apply
         * @param importerManager   Optional plugin manager instance for image
         *      loading
         * @param fontManager       Optional plugin manager instance for font
         *      loading
         * @return Reference to self (for method chaining)
         *
         * If a renderer isn't present yet, sets its instance. Then creates
         * layer and layouter instances corresponding to all @p features with
         * style uniform count, style count and other parameters coming from
         * @p style. If @p features contain @ref StyleFeature::TextLayer and
         * @p fontManager is @cpp nullptr @ce, an internal font plugin manager
         * instance is created; if @p features contain
         * @ref StyleFeature::TextLayerImages and @p importerManager is
         * @cpp nullptr @ce, an internal importer plugin manager instance is
         * created. The function then calls @ref AbstractStyle::apply() to
         * apply the style to those layers and layouters. If it fails, the
         * program exits, see @ref trySetStyle() for an alternative.
         *
         * Expects that user interface size is already set, either using the
         * constructor or by calling @ref setSize(). Expects that @p features
         * are a subset of @ref AbstractStyle::features() of @p style, contain
         * at least one feature and that the user interface doesn't yet contain
         * any layers or layouters corresponding to @p features as documented
         * in the @ref StyleFeature enum values.
         *
         * @m_class{m-note m-warning}
         *
         * @par
         *      Currently, if @p features contain @ref StyleFeature::TextLayer,
         *      the @ref AbstractStyle::textLayerGlyphCacheSize() depth is
         *      expected to be @cpp 1 @ce, as @ref Text::GlyphCacheGL doesn't
         *      support arrays yet.
         *
         * While it's not allowed to set style features more than once for one
         * particular layer, it's possible to call this function multiple times
         * with mutually disjoint @p features. To replace a layer style with
         * another compatible style, call @ref AbstractStyle::apply() directly.
         * See its documentation for more information about style compatibility
         * restrictions.
         * @see @ref setStyle(const AbstractStyle&, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*),
         *      @ref hasRendererInstance(), @ref hasBaseLayer(),
         *      @ref hasTextLayer(), @ref hasEventLayer(),
         *      @ref hasSnapLayouter(),
         *      @ref UserInterfaceGL(const Vector2&, const Vector2&, const Vector2i&, const AbstractStyle&, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*),
         *      @ref UserInterfaceGL(const Vector2i&, const AbstractStyle&, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*)
         */
        UserInterfaceGL& setStyle(const AbstractStyle& style, StyleFeatures features, PluginManager::Manager<Trade::AbstractImporter>* importerManager = nullptr, PluginManager::Manager<Text::AbstractFont>* fontManager = nullptr);

        /**
         * @brief Set all features from a style
         * @return Reference to self (for method chaining)
         *
         * Equivalent to calling @ref setStyle(const AbstractStyle&, StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*)
         * with @p features set to @ref AbstractStyle::features() of @p style.
         */
        UserInterfaceGL& setStyle(const AbstractStyle& style, PluginManager::Manager<Trade::AbstractImporter>* importerManager = nullptr, PluginManager::Manager<Text::AbstractFont>* fontManager = nullptr);

        /**
         * @brief Try to set features from a style
         *
         * Unlike @ref setStyle(const AbstractStyle&, StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*)
         * returns @cpp false @ce if @ref AbstractStyle::apply() failed instead
         * of exiting, @cpp true @ce otherwise.
         */
        bool trySetStyle(const AbstractStyle& style, StyleFeatures features, PluginManager::Manager<Trade::AbstractImporter>* importerManager = nullptr, PluginManager::Manager<Text::AbstractFont>* fontManager = nullptr);

        /**
         * @brief Try to set all features from a style
         *
         * Unlike @ref setStyle(const AbstractStyle&, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*)
         * returns @cpp false @ce if @ref AbstractStyle::apply() failed instead
         * of exiting, @cpp true @ce otherwise.
         */
        bool trySetStyle(const AbstractStyle& style, PluginManager::Manager<Trade::AbstractImporter>* importerManager = nullptr, PluginManager::Manager<Text::AbstractFont>* fontManager = nullptr);

        /**
         * @brief Set a base layer instance
         * @return Reference to self (for method chaining)
         *
         * Expects that the instance hasn't been set yet, either by this
         * function or transitively either by @ref setStyle() or a constructor
         * taking a style instance. The instance is subsequently available
         * through @ref baseLayer().
         * @see @ref hasBaseLayer(), @ref StyleFeature::BaseLayer
         */
        UserInterfaceGL& setBaseLayerInstance(Containers::Pointer<BaseLayerGL>&& instance);

        /**
         * @brief Set a text layer instance
         * @return Reference to self (for method chaining)
         *
         * Expects that the instance hasn't been set yet, either by this
         * function or transitively either by @ref UserInterfaceGL::setStyle()
         * or a @ref UserInterfaceGL constructor taking a style instance. The
         * instance is subsequently available through @ref textLayer().
         * @see @ref hasTextLayer(), @ref StyleFeature::TextLayer
         */
        UserInterfaceGL& setTextLayerInstance(Containers::Pointer<TextLayerGL>&& instance);

        /* Overloads to remove a WTF factor from method chaining order */
        #ifndef DOXYGEN_GENERATING_OUTPUT
        UserInterfaceGL& setSize(const Vector2& size, const Vector2& windowSize, const Vector2i& framebufferSize) {
            return static_cast<UserInterfaceGL&>(UserInterface::setSize(size, windowSize, framebufferSize));
        }
        template<class Application, class = decltype(Implementation::ApplicationSizeConverter<Application>::set(std::declval<AbstractUserInterface&>(), std::declval<const Application&>()))> UserInterfaceGL& setSize(const Application& application) {
            return static_cast<UserInterfaceGL&>(UserInterface::setSize(application));
        }
        UserInterfaceGL& setSize(const Vector2i& size) {
            return static_cast<UserInterfaceGL&>(UserInterface::setSize(size));
        }
        UserInterfaceGL& setEventLayerInstance(Containers::Pointer<EventLayer>&& instance) {
            return static_cast<UserInterfaceGL&>(UserInterface::setEventLayerInstance(Utility::move(instance)));
        }
        UserInterfaceGL& setSnapLayouterInstance(Containers::Pointer<SnapLayouter>&& instance) {
            return static_cast<UserInterfaceGL&>(UserInterface::setSnapLayouterInstance(Utility::move(instance)));
        }
        UserInterfaceGL& clean() {
            return static_cast<UserInterfaceGL&>(UserInterface::clean());
        }
        UserInterfaceGL& advanceAnimations(Nanoseconds time);
        UserInterfaceGL& update() {
            return static_cast<UserInterfaceGL&>(UserInterface::update());
        }
        UserInterfaceGL& draw() {
            return static_cast<UserInterfaceGL&>(UserInterface::draw());
        }
        #endif

    private:
        struct State;

        UserInterfaceGL& createInternal(const AbstractStyle& style, StyleFeatures styleFeatures, PluginManager::Manager<Trade::AbstractImporter>* importerManager, PluginManager::Manager<Text::AbstractFont>* fontManager);
        UserInterfaceGL& createInternal(const AbstractStyle& style, PluginManager::Manager<Trade::AbstractImporter>* importerManager, PluginManager::Manager<Text::AbstractFont>* fontManager);
        bool tryCreateInternal(const AbstractStyle& style, StyleFeatures styleFeatures, PluginManager::Manager<Trade::AbstractImporter>* importerManager, PluginManager::Manager<Text::AbstractFont>* fontManager);
        bool tryCreateInternal(const AbstractStyle& style, PluginManager::Manager<Trade::AbstractImporter>* importerManager, PluginManager::Manager<Text::AbstractFont>* fontManager);
};

}}
#else
#error this header is available only in the OpenGL build
#endif

#endif
