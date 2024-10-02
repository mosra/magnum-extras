#ifndef Magnum_Ui_UserInterfaceGL_h
#define Magnum_Ui_UserInterfaceGL_h
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

Provides an interface for setting up @ref RendererGL, @ref BaseLayerGL and
@ref TextLayerGL instances, either directly or via an @ref AbstractStyle
instance.

@note This class is available only if Magnum is compiled with
    @ref MAGNUM_TARGET_GL enabled (done by default). See @ref building-features
    for more information.
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
        explicit UserInterfaceGL(const Vector2& size, const Vector2& windowSize, const Vector2i& framebufferSize, const AbstractStyle& style, PluginManager::Manager<Trade::AbstractImporter>* importerManager = nullptr, PluginManager::Manager<Text::AbstractFont>* fontManager = nullptr);

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
        explicit UserInterfaceGL(const Vector2& size, const Vector2& windowSize, const Vector2i& framebufferSize, const AbstractStyle& style, StyleFeatures styleFeatures, PluginManager::Manager<Trade::AbstractImporter>* importerManager = nullptr, PluginManager::Manager<Text::AbstractFont>* fontManager = nullptr);

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
        UserInterfaceGL& create(const Vector2& size, const Vector2& windowSize, const Vector2i& framebufferSize, const AbstractStyle& style, PluginManager::Manager<Trade::AbstractImporter>* importerManager = nullptr, PluginManager::Manager<Text::AbstractFont>* fontManager = nullptr);

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
         * @ref setEventLayerInstance() or @ref setRendererInstance() was
         * called yet. Equivalent to calling
         * @ref setSize(const Vector2&, const Vector2&, const Vector2i&)
         * followed by @ref setStyle(const AbstractStyle&, StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*).
         * See documentation of these functions for more information and
         * alternative ways to create the user interface. If style application
         * fails during the creation process, the program exits. Use
         * @ref tryCreate() for a more graceful failure handling.
         */
        UserInterfaceGL& create(const Vector2& size, const Vector2& windowSize, const Vector2i& framebufferSize, const AbstractStyle& style, StyleFeatures styleFeatures, PluginManager::Manager<Trade::AbstractImporter>* importerManager = nullptr, PluginManager::Manager<Text::AbstractFont>* fontManager = nullptr);

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
        bool tryCreate(const Vector2& size, const Vector2& windowSize, const Vector2i& framebufferSize, const AbstractStyle& style, PluginManager::Manager<Trade::AbstractImporter>* importerManager = nullptr, PluginManager::Manager<Text::AbstractFont>* fontManager = nullptr);

        /**
         * @brief Try to create the user interface with a subset of the style
         *
         * Unlike @ref create(const Vector2&, const Vector2&, const Vector2i&, const AbstractStyle&, StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*)
         * returns @cpp false @ce if @ref AbstractStyle::apply() failed instead
         * of exiting, @cpp true @ce otherwise. Equivalent to calling
         * @ref setSize(const Vector2&, const Vector2&, const Vector2i&)
         * followed by @ref trySetStyle(const AbstractStyle&, StyleFeatures, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*).
         */
        bool tryCreate(const Vector2& size, const Vector2& windowSize, const Vector2i& framebufferSize, const AbstractStyle& style, StyleFeatures styleFeatures, PluginManager::Manager<Trade::AbstractImporter>* importerManager = nullptr, PluginManager::Manager<Text::AbstractFont>* fontManager = nullptr);

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
         * @see @ref hasRenderer()
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
         * layer instances corresponding to all @p features with style uniform
         * count, style count and other parameters coming from @p style. If
         * @p features contain @ref StyleFeature::TextLayer and @p fontManager
         * is @cpp nullptr @ce, an internal font plugin manager instance is
         * created. The function then calls @ref AbstractStyle::apply() to
         * apply the style to those layers. If it fails, the program exits, see
         * @ref trySetStyle() for an alternative.
         *
         * Expects that user interface size is already set, either using the
         * constructor or by calling @ref setSize(). Expects that @p features
         * are a subset of @ref AbstractStyle::features() of @p style, contain
         * at least one feature and that the user interface doesn't yet contain
         * any layers corresponding to @p features as documented in the
         * @ref StyleFeature enum values.
         *
         * @m_class{m-note m-warning}
         *
         * @par
         *      Currently, if @p features contain @ref StyleFeature::TextLayer,
         *      the @ref AbstractStyle::textLayerGlyphCacheSize() depth is
         *      expected to be @cpp 1 @ce, as @ref Text::GlyphCache doesn't
         *      support arrays yet.
         *
         * While it's not allowed to set style features more than once for one
         * particular layer, it's possible to call this function multiple times
         * with mutually disjoint @p features. To replace a layer style with
         * another compatible style, call @ref AbstractStyle::apply() directly.
         * See its documentation for more information about style compatibility
         * restrictions.
         * @see @ref setStyle(const AbstractStyle&, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*),
         *      @ref hasBaseLayer(), @ref hasTextLayer(), @ref hasEventLayer(),
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
};

}}
#else
#error this header is available only in the OpenGL build
#endif

#endif
