#ifndef Magnum_Ui_AbstractTheme_h
#define Magnum_Ui_AbstractTheme_h
/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023, 2024, 2025, 2026
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
 * @brief Class @ref Magnum::Ui::AbstractTheme, enum @ref Magnum::Ui::ThemeFeature, enum set @ref Magnum::Ui::ThemeFeatures
 * @m_since_latest_{extras}
 */

#include <Corrade/Containers/EnumSet.h>
#include <Corrade/PluginManager/PluginManager.h>
#include <Magnum/Math/Vector3.h>
#include <Magnum/Text/Text.h>
#include <Magnum/Trade/Trade.h>

#include "Magnum/Ui/Ui.h"
#include "Magnum/Ui/visibility.h"

namespace Magnum { namespace Ui {

/**
@brief Feature supplied by a theme
@m_since_latest_{extras}

@see @ref ThemeFeatures, @ref AbstractTheme::features(),
    @ref AbstractTheme::apply(), @ref UserInterfaceGL::setTheme(),
    @ref UserInterfaceGL::trySetTheme()
*/
enum class ThemeFeature: UnsignedShort {
    /**
     * Data layer. Ensures a @ref DataLayer instance is set up on the
     * @ref UserInterface.
     * @see @ref UserInterface::dataLayer(),
     *      @ref UserInterface::hasDataLayer()
     */
    DataLayer = 1 << 0,

    /**
     * Background layer style. Ensures a background @ref BaseLayer instance
     * with a compatible @ref BaseLayer::Shared state is set up on the
     * @ref UserInterface, the theme implementation then calls
     * @relativeref{BaseLayer::Shared,setStyle()} and
     * @relativeref{BaseLayer::Shared,setStyleTransition()} on it.
     *
     * If @ref ThemeFeature::BaseLayer is supplied but
     * @ref ThemeFeature::BackgroundLayer not, the base layer is used for
     * backgrounds as well.
     * @see @ref UserInterface::backgroundLayer(),
     *      @ref UserInterface::hasBackgroundLayer()
     */
    BackgroundLayer = 1 << 1,

    /**
     * Background layer animations using @ref BaseLayerStyleAnimator. Can only
     * be used with @ref AbstractTheme::apply() if a background
     * @ref BaseLayerStyleAnimator instance is already set up on the
     * @ref UserInterface; can only be used with @ref UserInterfaceGL::setTheme()
     * / @relativeref{UserInterfaceGL,trySetTheme()} if set together with
     * @ref ThemeFeature::BackgroundLayer or if a background
     * @ref BaseLayerStyleAnimator instance is already set up on the
     * @ref UserInterface.
     *
     * If @ref ThemeFeature::BaseLayer is supplied but
     * @ref ThemeFeature::BackgroundLayer not, the base layer, along with any
     * animations, is used for backgrounds as well.
     * @see @ref UserInterface::backgroundLayerStyleAnimator(),
     *      @ref UserInterface::hasBackgroundLayerStyleAnimator()
     */
    BackgroundLayerAnimations = 1 << 2,

    /**
     * Base layer style. Ensures a @ref BaseLayer instance with a compatible
     * @ref BaseLayer::Shared state is set up on the @ref UserInterface, the
     * theme implementation then calls @relativeref{BaseLayer::Shared,setStyle()}
     * and @relativeref{BaseLayer::Shared,setStyleTransition()} on it.
     * @see @ref UserInterface::baseLayer(), @ref UserInterface::hasBaseLayer()
     */
    BaseLayer = 1 << 3,

    /**
     * Base layer animations using @ref BaseLayerStyleAnimator. Can only be
     * used with @ref AbstractTheme::apply() if a @ref BaseLayerStyleAnimator
     * instance is already set up on the @ref UserInterface; can only be used
     * with @ref UserInterfaceGL::setTheme() /
     * @relativeref{UserInterfaceGL,trySetTheme()} if set together with
     * @ref ThemeFeature::BaseLayer or if a @ref BaseLayerStyleAnimator
     * instance is already set up on the @ref UserInterface.
     * @see @ref UserInterface::baseLayerStyleAnimator(),
     *      @ref UserInterface::hasBaseLayerStyleAnimator()
     */
    BaseLayerAnimations = 1 << 4,

    /**
     * Text layer style and fonts. Ensures a @ref TextLayer instance with a
     * compatible @ref TextLayer::Shared state is set up on the
     * @ref UserInterface, the theme implementation then calls
     * @relativeref{TextLayer::Shared,addFont()},
     * @relativeref{TextLayer::Shared,setStyle()} and
     * @relativeref{TextLayer::Shared,setStyleTransition()} on it.
     * @see @ref UserInterface::textLayer(), @ref UserInterface::hasTextLayer()
     */
    TextLayer = 1 << 5,

    /**
     * Additional images such as icons for use with
     * @ref TextLayer::createGlyph(). Can only be used with
     * @ref AbstractTheme::apply() if a @ref TextLayer instance is already set
     * up on the @ref UserInterface; can only be used with
     * @ref UserInterfaceGL::setTheme() /
     * @relativeref{UserInterfaceGL,trySetTheme()} if set together with
     * @ref ThemeFeature::TextLayer or if a @ref TextLayer instance is already
     * set up on the @ref UserInterface.
     */
    TextLayerImages = 1 << 6,

    /**
     * Text layer animations using @ref TextLayerStyleAnimator. Can only be
     * used with @ref AbstractTheme::apply() if a @ref TextLayerStyleAnimator
     * instance is already set up on the @ref UserInterface; can only be used
     * with @ref UserInterfaceGL::setTheme() /
     * @relativeref{UserInterfaceGL,trySetTheme()} if set together with
     * @ref ThemeFeature::TextLayer or if a @ref TextLayerStyleAnimator
     * instance is already set up on the @ref UserInterface.
     * @see @ref UserInterface::textLayerStyleAnimator(),
     *      @ref UserInterface::hasTextLayerStyleAnimator()
     */
    TextLayerAnimations = 1 << 7,

    /**
     * Event layer. Ensures an @ref EventLayer instance is set up on the
     * @ref UserInterface.
     * @see @ref UserInterface::eventLayer(),
     *      @ref UserInterface::hasEventLayer()
     */
    EventLayer = 1 << 8,

    /**
     * Layout layer. Ensures a @ref LayoutLayer instance is set up on the
     * @ref UserInterface.
     * @see @ref UserInterface::layoutLayer(),
     *      @ref UserInterface::hasLayoutLayer()
     */
    LayoutLayer = 1 << 9,

    /**
     * Snap layouter. Ensures a @ref SnapLayouter instance is set up on the
     * @ref UserInterface.
     * @see @ref UserInterface::snapLayouter(),
     *      @ref UserInterface::hasSnapLayouter()
     */
    SnapLayouter = 1 << 10,

    /**
     * Generic layouter. Ensures a @ref GenericLayouter instance is set up on
     * the @ref UserInterface.
     * @see @ref UserInterface::genericLayouter(),
     *      @ref UserInterface::hasGenericLayouter()
     */
    GenericLayouter = 1 << 11,

    /**
     * Node animations. Ensures a @ref NodeAnimator instance is set up on the
     * @ref UserInterface.
     * @see @ref UserInterface::nodeAnimator(),
     *      @ref UserInterface::hasNodeAnimator()
     */
    NodeAnimations = 1 << 12,
};

/**
@debugoperatorenum{ThemeFeature}
@m_since_latest_{extras}
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, ThemeFeature value);

/**
@brief Features supplied by a theme
@m_since_latest_{extras}

@see @ref AbstractTheme::features(), @ref AbstractTheme::apply(),
    @ref UserInterfaceGL::setTheme(), @ref UserInterfaceGL::trySetTheme()
*/
typedef Containers::EnumSet<ThemeFeature
    #ifndef DOXYGEN_GENERATING_OUTPUT
    , UnsignedInt(ThemeFeature::DataLayer)|
      UnsignedInt(ThemeFeature::BackgroundLayer)|
      UnsignedInt(ThemeFeature::BackgroundLayerAnimations)|
      UnsignedInt(ThemeFeature::BaseLayer)|
      UnsignedInt(ThemeFeature::BaseLayerAnimations)|
      UnsignedInt(ThemeFeature::TextLayer)|
      UnsignedInt(ThemeFeature::TextLayerImages)|
      UnsignedInt(ThemeFeature::TextLayerAnimations)|
      UnsignedInt(ThemeFeature::EventLayer)|
      UnsignedInt(ThemeFeature::LayoutLayer)|
      UnsignedInt(ThemeFeature::SnapLayouter)|
      UnsignedInt(ThemeFeature::GenericLayouter)|
      UnsignedInt(ThemeFeature::NodeAnimations)
    #endif
    > ThemeFeatures;

CORRADE_ENUMSET_OPERATORS(ThemeFeatures)

/**
@debugoperatorenum{ThemeFeatures}
@m_since_latest_{extras}
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, ThemeFeatures value);

namespace Implementation {
    /* Used by various tests, less wasteful to have here than in the
       potentially huge AbstractTheme.hpp */
    enum: UnsignedInt {
        BaseStyleCount = 63,
        TextStyleCount = 91,
        LayoutStyleCount = 4,
        IconCount = 2
    };
}

/**
@brief Base for @ref UserInterface themes
@m_since_latest_{extras}

@section Ui-AbstractTheme-subclassing Subclassing

A subclass implements at least @ref doFeatures() and @ref doApply(), and then a
subset of the other virtual functions based on what features are actually
supplied by the theme. See their documentation for more information.

@see @ref UserInterfaceGL::setTheme(),
    @ref UserInterfaceGL::UserInterfaceGL(const Vector2&, const Vector2&, const Vector2i&, const AbstractTheme&, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*),
    @ref UserInterfaceGL::UserInterfaceGL(const Vector2i&, const AbstractTheme&, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*),
*/
class MAGNUM_UI_EXPORT AbstractTheme {
    public:
        explicit AbstractTheme();

        /* Not strictly needed as the class is trivially destructible, but
           derived classes might not be trivially destructible anymore which
           would leak their state if deleted through the base pointer. And
           Clang warns even if the subclass is trivially destructible. */
        virtual ~AbstractTheme();

        /**
         * @brief Features supplied by a theme
         *
         * Guaranteed to return at least one @ref ThemeFeature.
         */
        ThemeFeatures features() const;

        /**
         * @brief Style uniform count for the background layer
         *
         * Expects that @ref ThemeFeature::BackgroundLayer is supported. The
         * returned value is passed to background layer's
         * @ref BaseLayer::Shared::Configuration::Configuration(UnsignedInt, UnsignedInt)
         * constructor by @ref UserInterfaceGL::setTheme().
         * @see @ref backgroundLayerStyleCount(),
         *      @ref backgroundLayerDynamicStyleCount(), @ref features()
         */
        UnsignedInt backgroundLayerStyleUniformCount() const;

        /**
         * @brief Style count for the background layer
         *
         * Expects that @ref ThemeFeature::BackgroundLayer is supported. The
         * returned value is passed to background layer's
         * @ref BaseLayer::Shared::Configuration::Configuration(UnsignedInt, UnsignedInt)
         * constructor by @ref UserInterfaceGL::setTheme().
         * @see @ref backgroundLayerStyleUniformCount(),
         *      @ref backgroundLayerDynamicStyleCount(), @ref features()
         */
        UnsignedInt backgroundLayerStyleCount() const;

        /**
         * @brief Dynamic style count for the background layer
         *
         * Expects that @ref ThemeFeature::BackgroundLayer is supported. The
         * returned value is passed to background layer's
         * @ref BaseLayer::Shared::Configuration::setDynamicStyleCount()
         * by @ref UserInterfaceGL::setTheme(). Call
         * @ref setBackgroundLayerDynamicStyleCount() to increase the count if
         * needed.
         * @see @ref backgroundLayerStyleUniformCount(),
         *      @ref backgroundLayerStyleCount(), @ref features()
         */
        UnsignedInt backgroundLayerDynamicStyleCount() const;

        /**
         * @brief Request more background layer dynamic styles
         * @return Reference to self (for method chaining)
         *
         * Expects that @ref ThemeFeature::BackgroundLayer is supported. Call
         * this function if the dynamic style count requested by the theme
         * isn't enough for extra animations and other dynamic style features
         * used by the application. The used count will be a maximum of what
         * the theme itself returned and what was requested with this function.
         * @see @ref backgroundLayerDynamicStyleCount(), @ref features()
         */
        AbstractTheme& setBackgroundLayerDynamicStyleCount(UnsignedInt count);

        /**
         * @brief Additional flags for the background layer
         *
         * Expects that @ref ThemeFeature::BackgroundLayer is supported. The
         * returned value is passed to background layer's
         * @ref BaseLayer::Shared::Configuration::setFlags() by
         * @ref UserInterfaceGL::setTheme(). Call @ref setBackgroundLayerFlags()
         * to supply additional flags to add or clear from the set if needed.
         */
        BaseLayerSharedFlags backgroundLayerFlags() const;

        /**
         * @brief Set additional background layer flags
         * @return Reference to self (for method chaining)
         *
         * Expects that @ref ThemeFeature::BackgroundLayer is supported, @p add
         * is a subset of @ref BaseLayerSharedFlag::SubdividedQuads, and
         * @p clear is a subset of @ref BaseLayerSharedFlag::BackgroundBlur,
         * @relativeref{BaseLayerSharedFlag,NoRoundedCorners} and
         * @relativeref{BaseLayerSharedFlag,NoOutline}. Flags used are a union
         * of what the theme itself returned and what was requested in @p add,
         * with everything in @p clear cleared from the set.
         * @see @ref backgroundLayerFlags(), @ref features()
         */
        AbstractTheme& setBackgroundLayerFlags(BaseLayerSharedFlags add, BaseLayerSharedFlags clear);

        /**
         * @brief Background blur radius
         *
         * Expects that @ref ThemeFeature::BackgroundLayer is supported. The
         * returned value is passed to background layer's
         * @ref BaseLayer::Shared::Configuration::setBackgroundBlurRadius() by
         * @ref UserInterfaceGL::setTheme() and has an effect only if
         * @ref BaseLayerSharedFlag::BackgroundBlur is present in
         * @ref backgroundLayerFlags().
         */
        UnsignedInt backgroundLayerBlurRadius() const;

        /**
         * @brief Background blur sampling cutoff
         *
         * Expects that @ref ThemeFeature::BackgroundLayer is supported. The
         * returned value is passed to background layer's
         * @ref BaseLayer::Shared::Configuration::setBackgroundBlurRadius() by
         * @ref UserInterfaceGL::setTheme() and has an effect only if
         * @ref BaseLayerSharedFlag::BackgroundBlur is present in
         * @ref backgroundLayerFlags().
         */
        Float backgroundLayerBlurCutoff() const;

        /**
         * @brief Style uniform count for the base layer
         *
         * Expects that @ref ThemeFeature::BaseLayer is supported. The returned
         * value is passed to base layer's
         * @ref BaseLayer::Shared::Configuration::Configuration(UnsignedInt, UnsignedInt)
         * constructor by @ref UserInterfaceGL::setTheme().
         * @see @ref baseLayerStyleCount(), @ref baseLayerDynamicStyleCount(),
         *      @ref features()
         */
        UnsignedInt baseLayerStyleUniformCount() const;

        /**
         * @brief Style count for the base layer
         *
         * Expects that @ref ThemeFeature::BaseLayer is supported. The returned
         * value is passed to base layer's
         * @ref BaseLayer::Shared::Configuration::Configuration(UnsignedInt, UnsignedInt)
         * constructor by @ref UserInterfaceGL::setTheme().
         * @see @ref baseLayerStyleUniformCount(),
         *      @ref baseLayerDynamicStyleCount(), @ref features()
         */
        UnsignedInt baseLayerStyleCount() const;

        /**
         * @brief Dynamic style count for the base layer
         *
         * Expects that @ref ThemeFeature::BaseLayer is supported. The returned
         * value is passed to base layer's
         * @ref BaseLayer::Shared::Configuration::setDynamicStyleCount() by
         * @ref UserInterfaceGL::setTheme(). Call
         * @ref setBaseLayerDynamicStyleCount() to increase the count if
         * needed.
         * @see @ref baseLayerStyleUniformCount(), @ref baseLayerStyleCount(),
         *      @ref features()
         */
        UnsignedInt baseLayerDynamicStyleCount() const;

        /**
         * @brief Request more base layer dynamic styles
         * @return Reference to self (for method chaining)
         *
         * Expects that @ref ThemeFeature::BaseLayer is supported. Call this
         * function if the dynamic style count requested by the theme isn't
         * enough for extra animations and other dynamic style features used by
         * the application. The used count will be a maximum of what the theme
         * itself returned and what was requested with this function.
         * @see @ref baseLayerDynamicStyleCount(), @ref features()
         */
        AbstractTheme& setBaseLayerDynamicStyleCount(UnsignedInt count);

        /**
         * @brief Additional flags for the base layer
         *
         * Expects that @ref ThemeFeature::BaseLayer is supported. The returned
         * value is passed to base layer's
         * @ref BaseLayer::Shared::Configuration::setFlags() by
         * @ref UserInterfaceGL::setTheme(). Call @ref setBaseLayerFlags() to
         * supply additional flags to add or clear from the set if needed.
         */
        BaseLayerSharedFlags baseLayerFlags() const;

        /**
         * @brief Set additional base layer flags
         * @return Reference to self (for method chaining)
         *
         * Expects that @ref ThemeFeature::BaseLayer is supported, @p add is a
         * subset of @ref BaseLayerSharedFlag::SubdividedQuads, and @p clear is
         * a subset of @ref BaseLayerSharedFlag::NoRoundedCorners and
         * @relativeref{BaseLayerSharedFlag,NoOutline}. Flags used are a union
         * of what the theme itself returned and what was requested in @p add,
         * with everything in @p clear cleared from the set.
         * @see @ref baseLayerFlags(), @ref features()
         */
        AbstractTheme& setBaseLayerFlags(BaseLayerSharedFlags add, BaseLayerSharedFlags clear);

        /**
         * @brief Style uniform count for the text layer
         *
         * Expects that @ref ThemeFeature::TextLayer is supported. The returned
         * value is passed to text layer's
         * @ref TextLayer::Shared::Configuration::Configuration(UnsignedInt, UnsignedInt)
         * constructor by @ref UserInterfaceGL::setTheme().
         * @see @ref textLayerStyleCount(),
         *      @ref textLayerEditingStyleUniformCount(),
         *      @ref textLayerEditingStyleCount(),
         *      @ref textLayerDynamicStyleCount(),
         *      @ref textLayerGlyphCacheFormat(),
         *      @ref textLayerGlyphCacheSize(),
         *      @ref textLayerGlyphCachePadding(), @ref features()
         */
        UnsignedInt textLayerStyleUniformCount() const;

        /**
         * @brief Style count for the text layer
         *
         * Expects that @ref ThemeFeature::TextLayer is supported. The returned
         * value is passed to text layer's
         * @ref TextLayer::Shared::Configuration::Configuration(UnsignedInt, UnsignedInt)
         * constructor by @ref UserInterfaceGL::setTheme().
         * @see @ref textLayerStyleUniformCount(),
         *      @ref textLayerEditingStyleUniformCount(),
         *      @ref textLayerEditingStyleCount(),
         *      @ref textLayerDynamicStyleCount(),
         *      @ref textLayerGlyphCacheFormat(),
         *      @ref textLayerGlyphCacheSize(),
         *      @ref textLayerGlyphCachePadding(), @ref features()
         */
        UnsignedInt textLayerStyleCount() const;

        /**
         * @brief Editing style uniform count for the text layer
         *
         * Expects that @ref ThemeFeature::TextLayer is supported. The returned
         * value is passed to text layer's
         * @ref TextLayer::Shared::Configuration::setEditingStyleCount() by
         * @ref UserInterfaceGL::setTheme().
         * @see @ref textLayerStyleUniformCount(), @ref textLayerStyleCount(),
         *      @ref textLayerEditingStyleCount(),
         *      @ref textLayerDynamicStyleCount(),
         *      @ref textLayerGlyphCacheFormat(),
         *      @ref textLayerGlyphCacheSize(),
         *      @ref textLayerGlyphCachePadding(), @ref features()
         */
        UnsignedInt textLayerEditingStyleUniformCount() const;

        /**
         * @brief Editing style count for the text layer
         *
         * Expects that @ref ThemeFeature::TextLayer is supported. The returned
         * value is passed to text layer's
         * @ref TextLayer::Shared::Configuration::setEditingStyleCount() by
         * @ref UserInterfaceGL::setTheme().
         * @see @ref textLayerStyleUniformCount(), @ref textLayerStyleCount(),
         *      @ref textLayerEditingStyleUniformCount(),
         *      @ref textLayerDynamicStyleCount(),
         *      @ref textLayerGlyphCacheFormat(),
         *      @ref textLayerGlyphCacheSize(),
         *      @ref textLayerGlyphCachePadding(), @ref features()
         */
        UnsignedInt textLayerEditingStyleCount() const;

        /**
         * @brief Dynamic style count for the text layer
         *
         * Expects that @ref ThemeFeature::TextLayer is supported. The returned
         * value is passed to text layer's
         * @ref TextLayer::Shared::Configuration::setDynamicStyleCount() by
         * @ref UserInterfaceGL::setTheme(). Call
         * @ref setTextLayerDynamicStyleCount() to increase the count if
         * needed.
         * @see @ref textLayerStyleUniformCount(), @ref textLayerStyleCount(),
         *      @ref textLayerEditingStyleUniformCount(),
         *      @ref textLayerEditingStyleCount(),
         *      @ref textLayerGlyphCacheFormat(),
         *      @ref textLayerGlyphCacheSize(),
         *      @ref textLayerGlyphCachePadding(), @ref features()
         */
        UnsignedInt textLayerDynamicStyleCount() const;

        /**
         * @brief Request more text layer dynamic styles
         * @return Reference to self (for method chaining)
         *
         * Expects that @ref ThemeFeature::TextLayer is supported. Call this
         * function if the dynamic style count requested by the theme isn't
         * enough for extra animations and other dynamic style features used by
         * the application. The used count will be a maximum of what the theme
         * itself returned and what was requested with this function.
         * @see @ref textLayerDynamicStyleCount(), @ref features()
         */
        AbstractTheme& setTextLayerDynamicStyleCount(UnsignedInt count);

        /**
         * @brief Text layer glyph cache format
         *
         * Expects that @ref ThemeFeature::TextLayer is supported. The returned
         * value is passed to text layer's @ref Text::GlyphCacheGL constructor
         * by @ref UserInterfaceGL::setTheme().
         * @see @ref textLayerStyleUniformCount(), @ref textLayerStyleCount(),
         *      @ref textLayerEditingStyleUniformCount(),
         *      @ref textLayerEditingStyleCount(),
         *      @ref textLayerDynamicStyleCount(),
         *      @ref textLayerGlyphCacheSize(),
         *      @ref textLayerGlyphCachePadding(), @ref features()
         */
        PixelFormat textLayerGlyphCacheFormat() const;

        /**
         * @brief Text layer glyph cache size
         *
         * Expects that @ref ThemeFeature::TextLayer is supported and that
         * @p features are a subset of @ref features() and contain at least
         * @ref ThemeFeature::TextLayer. The implementation may choose to
         * return a different value based on whether
         * @ref ThemeFeature::TextLayerImages is present in @p features or not.
         * The returned value is passed to text layer's @ref Text::GlyphCacheGL
         * constructor by @ref UserInterfaceGL::setTheme(). Call
         * @ref setTextLayerGlyphCacheSize() to enlarge the glyph cache if the
         * application needs to store more glyphs.
         * @see @ref textLayerStyleUniformCount(), @ref textLayerStyleCount(),
         *      @ref textLayerEditingStyleUniformCount(),
         *      @ref textLayerEditingStyleCount(),
         *      @ref textLayerDynamicStyleCount(),
         *      @ref textLayerGlyphCacheFormat(),
         *      @ref textLayerGlyphCachePadding(), @ref features()
         */
        Vector3i textLayerGlyphCacheSize(ThemeFeatures features) const;

        /**
         * @brief Text layer glyph cache padding
         *
         * Expects that @ref ThemeFeature::TextLayer is supported. The returned
         * value is passed to text layer's @ref Text::GlyphCacheGL constructor
         * by @ref UserInterfaceGL::setTheme(). Call
         * @ref setTextLayerGlyphCacheSize() to enlarge the glyph cache size or
         * padding if needed.
         * @see @ref textLayerStyleUniformCount(), @ref textLayerStyleCount(),
         *      @ref textLayerEditingStyleUniformCount(),
         *      @ref textLayerEditingStyleCount(),
         *      @ref textLayerDynamicStyleCount(),
         *      @ref textLayerGlyphCacheFormat(),
         *      @ref textLayerGlyphCacheSize(), @ref features()
         */
        Vector2i textLayerGlyphCachePadding() const;

        /**
         * @brief Request a larger glyph cache size
         * @return Reference to self (for method chaining)
         *
         * Expects that @ref ThemeFeature::TextLayer is supported. Call this
         * function if the glyph cache size requested by the theme isn't enough
         * for extra glyphs used by the application, if the application adds
         * additional fonts or other image data to it or if additional padding
         * is needed. The used size and padding will be a maximum of what the
         * theme itself returned and what was requested with this function.
         * @see @ref textLayerGlyphCacheSize(),
         *      @ref textLayerGlyphCachePadding(), @ref features()
         */
        AbstractTheme& setTextLayerGlyphCacheSize(const Vector3i& size, const Vector2i& padding = {});

        /**
         * @brief Style count for the layout layer
         *
         * Expects that @ref ThemeFeature::LayoutLayer is supported. The
         * returned value is passed to layout layer's
         * @ref LayoutLayer::LayoutLayer(LayerHandle, UnsignedInt) constructor
         * by @ref UserInterfaceGL::setTheme().
         * @see @ref features()
         */
        UnsignedInt layoutLayerStyleCount() const;

        /**
         * @brief Apply the theme
         *
         * Used internally from @ref UserInterfaceGL::setTheme() /
         * @relativeref{UserInterfaceGL,trySetTheme()}, you'll likely want to
         * call that function instead if setting up a theme on a new
         * @ref UserInterface instance. Direct use of this function is mainly
         * for applying another compatible theme to a user interface that
         * already has a theme set.
         *
         * Expects that @p ui has user interface size already set, either using
         * the constructor or by calling @ref UserInterface::setSize(). Expects
         * that @p features are a subset of @ref features() and contain at
         * least one feature, that @p ui already contains all layers, layouters
         * and animators corresponding to @p features, that the layer shared
         * state style uniform and style count matches the subset of
         * @ref backgroundLayerStyleUniformCount(),
         * @ref backgroundLayerStyleCount(), @ref baseLayerStyleUniformCount(),
         * @ref baseLayerStyleCount(), @ref textLayerStyleUniformCount(),
         * @ref textLayerStyleCount(), @ref textLayerEditingStyleUniformCount(),
         * @ref textLayerEditingStyleCount(), @ref layoutLayerStyleCount()
         * matching @p features and the layer shared state dynamic style count
         * is at least the subset of @ref backgroundLayerDynamicStyleCount(),
         * @ref baseLayerDynamicStyleCount(), @ref textLayerDynamicStyleCount()
         * matching @p features. Additionally, if @ref ThemeFeature::TextLayer
         * is present in @p features, expects that the @ref TextLayer::Shared
         * instance has a glyph cache set that matches
         * @ref textLayerGlyphCacheFormat(), has size at least
         * @ref textLayerGlyphCacheSize() for @p features and padding at least
         * @ref textLayerGlyphCachePadding() and that @p fontManager is not
         * @cpp nullptr @ce; and if @ref ThemeFeature::TextLayerImages is
         * present in @p features, expects that either the @ref TextLayer is
         * already present in the user interface or that
         * @ref ThemeFeature::TextLayer is included in @p features as well, and
         * that @p importerManager is not @cpp nullptr @ce. Returns
         * @cpp true @ce on success, prints a message to
         * @relativeref{Magnum,Error} and returns @cpp false @ce if some
         * run-time error happened during theme preparation, such as a plugin
         * not being found or external data failing to load.
         */
        bool apply(UserInterface& ui, ThemeFeatures features, PluginManager::Manager<Trade::AbstractImporter>* importerManager, PluginManager::Manager<Text::AbstractFont>* fontManager) const;

    private:
        /**
         * @brief Implementation for @ref features()
         *
         * Is expected to return at least one @ref ThemeFeature.
         */
        virtual ThemeFeatures doFeatures() const = 0;

        /**
         * @brief Implementation for @ref backgroundLayerStyleUniformCount()
         *
         * Guaranteed to be called only if @ref doFeatures() contains
         * @ref ThemeFeature::BackgroundLayer. Default implementation delegates
         * to @ref doBackgroundLayerStyleCount().
         */
        virtual UnsignedInt doBackgroundLayerStyleUniformCount() const;

        /**
         * @brief Implementation for @ref backgroundLayerStyleCount()
         *
         * Guaranteed to be called only if @ref doFeatures() contains
         * @ref ThemeFeature::BackgroundLayer. Has to be implemented if
         * @ref doFeatures() contains @ref ThemeFeature::BackgroundLayer.
         */
        virtual UnsignedInt doBackgroundLayerStyleCount() const;

        /**
         * @brief Implementation for @ref backgroundLayerDynamicStyleCount()
         *
         * Guaranteed to be called only if @ref doFeatures() contains
         * @ref ThemeFeature::BackgroundLayer. Default implementation returns
         * @cpp 0 @ce. If @ref ThemeFeature::BackgroundLayerAnimations is
         * present in @ref doFeatures(), expects that the implementation
         * returns at least one dynamic style.
         */
        virtual UnsignedInt doBackgroundLayerDynamicStyleCount() const;

        /**
         * @brief Implementation for @ref backgroundLayerFlags()
         *
         * Guaranteed to be called only if @ref doFeatures() contains
         * @ref ThemeFeature::BackgroundLayer. Expects that the implementation
         * returns a subset of @ref BaseLayerSharedFlag::BackgroundBlur,
         * @relativeref{BaseLayerSharedFlag,NoRoundedCorners} and
         * @relativeref{BaseLayerSharedFlag,NoOutline}. Default implementation
         * returns an empty set.
         */
        virtual BaseLayerSharedFlags doBackgroundLayerFlags() const;

        /**
         * @brief Implementation for @ref backgroundLayerBlurRadius()
         *
         * Guaranteed to be called only if @ref doFeatures() contains
         * @ref ThemeFeature::BackgroundLayer. Default implementation returns
         * @cpp 4 @ce, consistent with @ref BaseLayer::Shared::Configuration
         * defaults. The call to @ref BaseLayer::setBackgroundBlurPassCount(),
         * if needed, can be done from within the @ref doApply()
         * implementation.
         */
        virtual UnsignedInt doBackgroundLayerBlurRadius() const;

        /**
         * @brief Implementation for @ref backgroundLayerBlurCutoff()
         *
         * Guaranteed to be called only if @ref doFeatures() contains
         * @ref ThemeFeature::BackgroundLayer. Default implementation returns
         * @cpp 0.5f/255.0f @ce, consistent with
         * @ref BaseLayer::Shared::Configuration defaults.
         */
        virtual Float doBackgroundLayerBlurCutoff() const;

        /**
         * @brief Implementation for @ref baseLayerStyleUniformCount()
         *
         * Guaranteed to be called only if @ref doFeatures() contains
         * @ref ThemeFeature::BaseLayer. Default implementation delegates to
         * @ref doBaseLayerStyleCount().
         */
        virtual UnsignedInt doBaseLayerStyleUniformCount() const;

        /**
         * @brief Implementation for @ref baseLayerStyleCount()
         *
         * Guaranteed to be called only if @ref doFeatures() contains
         * @ref ThemeFeature::BaseLayer. Has to be implemented if
         * @ref doFeatures() contains @ref ThemeFeature::BaseLayer.
         */
        virtual UnsignedInt doBaseLayerStyleCount() const;

        /**
         * @brief Implementation for @ref baseLayerDynamicStyleCount()
         *
         * Guaranteed to be called only if @ref doFeatures() contains
         * @ref ThemeFeature::BaseLayer. Default implementation returns
         * @cpp 0 @ce. If @ref ThemeFeature::BaseLayerAnimations is present in
         * @ref doFeatures(), expects that the implementation returns at least
         * one dynamic style.
         */
        virtual UnsignedInt doBaseLayerDynamicStyleCount() const;

        /**
         * @brief Implementation for @ref baseLayerFlags()
         *
         * Guaranteed to be called only if @ref doFeatures() contains
         * @ref ThemeFeature::BaseLayer. Expects that the implementation
         * returns a subset of @ref BaseLayerSharedFlag::NoRoundedCorners and
         * @relativeref{BaseLayerSharedFlag,NoOutline}. Default implementation
         * returns an empty set.
         */
        virtual BaseLayerSharedFlags doBaseLayerFlags() const;

        /**
         * @brief Implementation for @ref textLayerStyleUniformCount()
         *
         * Guaranteed to be called only if @ref doFeatures() contains
         * @ref ThemeFeature::TextLayer. Default implementation delegates to
         * @ref doTextLayerStyleCount().
         */
        virtual UnsignedInt doTextLayerStyleUniformCount() const;

        /**
         * @brief Implementation for @ref textLayerStyleCount()
         *
         * Guaranteed to be called only if @ref doFeatures() contains
         * @ref ThemeFeature::TextLayer. Has to be implemented if
         * @ref doFeatures() contains @ref ThemeFeature::TextLayer.
         */
        virtual UnsignedInt doTextLayerStyleCount() const;

        /**
         * @brief Implementation for @ref textLayerEditingStyleUniformCount()
         *
         * Guaranteed to be called only if @ref doFeatures() contains
         * @ref ThemeFeature::TextLayer. Default implementation delegates to
         * @ref doTextLayerEditingStyleCount().
         */
        virtual UnsignedInt doTextLayerEditingStyleUniformCount() const;

        /**
         * @brief Implementation for @ref textLayerEditingStyleCount()
         *
         * Guaranteed to be called only if @ref doFeatures() contains
         * @ref ThemeFeature::TextLayer. Default implementation returns
         * @cpp 0 @ce.
         */
        virtual UnsignedInt doTextLayerEditingStyleCount() const;

        /**
         * @brief Implementation for @ref textLayerDynamicStyleCount()
         *
         * Guaranteed to be called only if @ref doFeatures() contains
         * @ref ThemeFeature::TextLayer. Default implementation returns
         * @cpp 0 @ce. If @ref ThemeFeature::TextLayerAnimations is present in
         * @ref doFeatures(), expects that the implementation returns at least
         * one dynamic style.
         */
        virtual UnsignedInt doTextLayerDynamicStyleCount() const;

        /**
         * @brief Implementation for @ref textLayerGlyphCacheFormat()
         *
         * Guaranteed to be called only if @ref doFeatures() contains
         * @ref ThemeFeature::TextLayer. Default implementation returns
         * @ref PixelFormat::R8Unorm.
         */
        virtual PixelFormat doTextLayerGlyphCacheFormat() const;

        /**
         * @brief Implementation for @ref textLayerGlyphCacheSize()
         *
         * Guaranteed to be called only if @ref doFeatures() contains
         * @ref ThemeFeature::TextLayer. The @p features are guaranteed to be a
         * subset of @ref features() and contain at least
         * @ref ThemeFeature::TextLayer. Has to be implemented if
         * @ref doFeatures() contains @ref ThemeFeature::TextLayer.
         */
        virtual Vector3i doTextLayerGlyphCacheSize(ThemeFeatures features) const;

        /**
         * @brief Implementation for @ref textLayerGlyphCacheFormat()
         *
         * Guaranteed to be called only if @ref doFeatures() contains
         * @ref ThemeFeature::TextLayer. Default implementation returns
         * @cpp {1, 1} @ce, consistently with default padding in
         * @ref Text::AbstractGlyphCache, to prevent artifacts. See
         * @ref Text-AbstractGlyphCache-padding for more information.
         */
        virtual Vector2i doTextLayerGlyphCachePadding() const;

        /**
         * @brief Implementation for @ref layoutLayerStyleCount()
         *
         * Guaranteed to be called only if @ref doFeatures() contains
         * @ref ThemeFeature::LayoutLayer. Has to be implemented if
         * @ref doFeatures() contains @ref ThemeFeature::LayoutLayer.
         */
        virtual UnsignedInt doLayoutLayerStyleCount() const;

        /**
         * @brief Implementation for @ref apply()
         *
         * Should call @ref BaseLayer::Shared::setStyle(),
         * @ref BaseLayer::Shared::setStyleTransition(),
         * @ref BaseLayer::Shared::setStyleAnimation(),
         * @ref TextLayer::Shared::addFont(),
         * @ref TextLayer::Shared::setStyle(),
         * @ref TextLayer::Shared::setStyleTransition(),
         * @ref TextLayer::Shared::setStyleAnimation() and
         * @ref LayoutLayer::setStyle() with style contents based on what
         * @p features are passed and return @cpp true @ce. If some runtime
         * error happens, should print a message to @relativeref{Magnum,Error}
         * and return @cpp false @ce.
         *
         * The @p ui is guaranteed to have user interface set for the theme to
         * use to calculate font rasterization and icon sizes, for example. The
         * @p features are guaranteed to be a subset of @ref features() and
         * contain at least one feature, that @p ui already contains all
         * layers, layouters and animators corresponding to @p features, that
         * the layer shared state style uniform and style count matches the
         * subset of @ref baseLayerStyleUniformCount(),
         * @ref baseLayerStyleCount(), @ref textLayerStyleUniformCount(),
         * @ref textLayerStyleCount(), @ref textLayerEditingStyleUniformCount(),
         * @ref textLayerEditingStyleCount(), @ref layoutLayerStyleCount()
         * matching @p features and the layer shared state dynamic style count
         * is at least the subset of @ref baseLayerDynamicStyleCount(),
         * @ref textLayerDynamicStyleCount() matching @p features.
         * Additionally, if @ref ThemeFeature::TextLayer is present in
         * @p features, the @ref TextLayer::Shared instance is guaranteed to
         * have a glyph cache set that matches
         * @ref textLayerGlyphCacheFormat(), with a size at least
         * @ref textLayerGlyphCacheSize() for @p features and padding at least
         * @ref textLayerGlyphCachePadding(), and @p fontManager is guaranteed
         * to not be @cpp nullptr @ce; and if
         * @ref ThemeFeature::TextLayerImages is present in @p features, the
         * @p importerManager is guaranteed to not be @cpp nullptr @ce.
         */
        virtual bool doApply(UserInterface& ui, ThemeFeatures features, PluginManager::Manager<Trade::AbstractImporter>* importerManager, PluginManager::Manager<Text::AbstractFont>* fontManager) const = 0;

        /* When more user-overridable properties are present, might want to put
           them into a PIMPL instead, and remove the Vector3 include again */
        UnsignedInt
            _backgroundLayerDynamicStyleCount = 0,
            _baseLayerDynamicStyleCount = 0,
            _textLayerDynamicStyleCount = 0;
        Vector3i _textLayerGlyphCacheSize;
        Vector2i _textLayerGlyphCachePadding;
        BaseLayerSharedFlags
            _backgroundLayerFlagsAdd, _backgroundLayerFlagsClear,
            _baseLayerFlagsAdd, _baseLayerFlagsClear;
};

}}

#endif
