#ifndef Magnum_Whee_AbstractStyle_h
#define Magnum_Whee_AbstractStyle_h
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
 * @brief Class @ref Magnum::Whee::AbstractStyle, enum @ref Magnum::Whee::StyleFeature, enum set @ref Magnum::Whee::StyleFeatures
 * @m_since_latest
 */

#include <Corrade/Containers/EnumSet.h>
#include <Corrade/PluginManager/PluginManager.h>
#include <Magnum/Math/Vector3.h>
#include <Magnum/Text/Text.h>
#include <Magnum/Trade/Trade.h>

#include "Magnum/Whee/Whee.h"
#include "Magnum/Whee/visibility.h"

namespace Magnum { namespace Whee {

/**
@brief Feature supplied by a style
@m_since_latest

@see @ref StyleFeatures, @ref AbstractStyle::features(),
    @ref AbstractStyle::apply(), @ref UserInterfaceGL::setStyle(),
    @ref UserInterfaceGL::trySetStyle()
*/
enum class StyleFeature: UnsignedByte {
    /**
     * @ref BaseLayer style. Ensures a @ref BaseLayer instance with a
     * compatible @ref BaseLayer::Shared state is set up on the
     * @ref UserInterface, the style implementation then calls
     * @relativeref{BaseLayer::Shared,setStyle()} and
     * @relativeref{BaseLayer::Shared,setStyleTransition()} on it.
     * @see @ref UserInterface::baseLayer()
     */
    BaseLayer = 1 << 0,

    /**
     * @ref TextLayer style and fonts. Ensures a @ref TextLayer instance with a
     * compatible @ref TextLayer::Shared state is set up on the
     * @ref UserInterface, the style implementation then calls
     * @relativeref{TextLayer::Shared,addFont()},
     * @relativeref{TextLayer::Shared,setStyle()} and
     * @relativeref{TextLayer::Shared,setStyleTransition()} on it.
     * @see @ref UserInterface::textLayer()
     */
    TextLayer = 1 << 1,

    /**
     * Additional images such as icons for use with
     * @ref TextLayer::createGlyph(). Can only be used with
     * @ref AbstractStyle::apply() if a @ref TextLayer instance is already set
     * up on the @ref UserInterface; can only be used with
     * @ref UserInterfaceGL::setStyle() /
     * @relativeref{UserInterfaceGL,trySetStyle()} if set together with
     * @ref StyleFeature::TextLayer or if a @ref TextLayer instance is already
     * set up on the @ref UserInterface.
     */
    TextLayerImages = 1 << 2,

    /**
     * @ref EventLayer style. Ensures an @ref EventLayer instance is set up on
     * the @ref UserInterface.
     * @see @ref UserInterface::eventLayer()
     */
    EventLayer = 1 << 3,
};

/** @debugoperatorenum{StyleFeature} */
MAGNUM_WHEE_EXPORT Debug& operator<<(Debug& debug, StyleFeature value);

/**
@brief Features supplied by a style
@m_since_latest

@see @ref AbstractStyle::features(), @ref AbstractStyle::apply(),
    @ref UserInterfaceGL::setStyle(), @ref UserInterfaceGL::trySetStyle()
*/
typedef Containers::EnumSet<StyleFeature
    #ifndef DOXYGEN_GENERATING_OUTPUT
    , UnsignedInt(StyleFeature::BaseLayer)|
      UnsignedInt(StyleFeature::TextLayer)|
      UnsignedInt(StyleFeature::TextLayerImages)|
      UnsignedInt(StyleFeature::EventLayer)
    #endif
    > StyleFeatures;

CORRADE_ENUMSET_OPERATORS(StyleFeatures)

/** @debugoperatorenum{StyleFeatures} */
MAGNUM_WHEE_EXPORT Debug& operator<<(Debug& debug, StyleFeatures value);

/**
@brief Base for @ref UserInterface styles
@m_since_latest

@section Whee-AbstractStyle-subclassing Subclassing

A subclass implements at least @ref doFeatures() and @ref doApply(), and then a
subset of the other virtual functions based on what features are actually
supplied by the style. See their documentation for more information.

@see @ref UserInterfaceGL::setStyle(),
    @ref UserInterfaceGL::UserInterfaceGL(const Vector2&, const Vector2&, const Vector2i&, const AbstractStyle&, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*),
    @ref UserInterfaceGL::UserInterfaceGL(const Vector2i&, const AbstractStyle&, PluginManager::Manager<Trade::AbstractImporter>*, PluginManager::Manager<Text::AbstractFont>*),
*/
class MAGNUM_WHEE_EXPORT AbstractStyle {
    public:
        explicit AbstractStyle();

        /* Not strictly needed as the class is trivially destructible, but
           derived classes might not be trivially destructible anymore which
           would leak their state if deleted through the base pointer. And
           Clang warns even if the subclass is trivially destructible. */
        virtual ~AbstractStyle();

        /**
         * @brief Features supplied by a style
         *
         * Guaranteed to return at least one @ref StyleFeature.
         */
        StyleFeatures features() const;

        /**
         * @brief Style uniform count for the base layer
         *
         * Expects that @ref StyleFeature::BaseLayer is supported. The returned
         * value is passed to the @ref BaseLayer::Shared::Configuration::Configuration(UnsignedInt, UnsignedInt)
         * constructor by @ref UserInterfaceGL::setStyle().
         * @see @ref baseLayerStyleCount(), @ref features()
         */
        UnsignedInt baseLayerStyleUniformCount() const;

        /**
         * @brief Style count for the base layer
         *
         * Expects that @ref StyleFeature::BaseLayer is supported. The returned
         * value is passed to the @ref BaseLayer::Shared::Configuration::Configuration(UnsignedInt, UnsignedInt)
         * constructor by @ref UserInterfaceGL::setStyle().
         * @see @ref baseLayerStyleUniformCount(), @ref features()
         */
        UnsignedInt baseLayerStyleCount() const;

        /**
         * @brief Style uniform count for the text layer
         *
         * Expects that @ref StyleFeature::TextLayer is supported. The returned
         * value is passed to the @ref TextLayer::Shared::Configuration::Configuration(UnsignedInt, UnsignedInt)
         * constructor by @ref UserInterfaceGL::setStyle().
         * @see @ref textLayerStyleCount(), @ref textLayerGlyphCacheFormat(),
         *      @ref textLayerGlyphCacheSize(),
         *      @ref textLayerGlyphCachePadding(), @ref features()
         */
        UnsignedInt textLayerStyleUniformCount() const;

        /**
         * @brief Style count for the text layer
         *
         * Expects that @ref StyleFeature::TextLayer is supported. The returned
         * value is passed to the @ref TextLayer::Shared::Configuration::Configuration(UnsignedInt, UnsignedInt)
         * constructor by @ref UserInterfaceGL::setStyle().
         * @see @ref textLayerStyleUniformCount(),
         *      @ref textLayerGlyphCacheFormat(),
         *      @ref textLayerGlyphCacheSize(),
         *      @ref textLayerGlyphCachePadding(), @ref features()
         */
        UnsignedInt textLayerStyleCount() const;

        /**
         * @brief Text layer glyph cache format
         *
         * Expects that @ref StyleFeature::TextLayer is supported. The returned
         * value is passed to the @ref Text::GlyphCache constructor by
         * @ref UserInterfaceGL::setStyle().
         * @see @ref textLayerStyleUniformCount(), @ref textLayerStyleCount(),
         *      @ref textLayerGlyphCacheSize(),
         *      @ref textLayerGlyphCachePadding(), @ref features()
         */
        PixelFormat textLayerGlyphCacheFormat() const;

        /**
         * @brief Text layer glyph cache size
         *
         * Expects that @ref StyleFeature::TextLayer is supported and that
         * @p features are a subset of @ref features() and contain at least
         * @ref StyleFeature::TextLayer. The implementation may choose to
         * return a different value based on whether
         * @ref StyleFeature::TextLayerImages is present in @p features or not.
         * The returned value is passed to the @ref Text::GlyphCache
         * constructor by @ref UserInterfaceGL::setStyle(). Call
         * @ref setTextLayerGlyphCacheSize() to enlarge the glyph cache if the
         * application needs to store more glyphs.
         * @see @ref textLayerStyleUniformCount(), @ref textLayerStyleCount(),
         *      @ref textLayerGlyphCacheFormat(),
         *      @ref textLayerGlyphCachePadding(), @ref features()
         */
        Vector3i textLayerGlyphCacheSize(StyleFeatures features) const;

        /**
         * @brief Text layer glyph cache padding
         *
         * Expects that @ref StyleFeature::TextLayer is supported. The returned
         * value is passed to the @ref Text::GlyphCache constructor by
         * @ref UserInterfaceGL::setStyle(). Call
         * @ref setTextLayerGlyphCacheSize() to enlarge the glyph cache size or
         * padding if needed.
         * @see @ref textLayerStyleUniformCount(), @ref textLayerStyleCount(),
         *      @ref textLayerGlyphCacheFormat(),
         *      @ref textLayerGlyphCacheSize(), @ref features()
         */
        Vector2i textLayerGlyphCachePadding() const;

        /**
         * @brief Request a larger glyph cache size
         * @return Reference to self (for method chaining)
         *
         * Expects that @ref StyleFeature::TextLayer is supported. Call this
         * function if the glyph cache size requested by the style isn't enough
         * for extra glyphs used by the application, if the application adds
         * additional fonts or other image data to it or if additional padding
         * is needed. The used size and padding will be a maximum of what the
         * style itself returned and what was requested with this function.
         * @see @ref textLayerGlyphCacheSize(),
         *      @ref textLayerGlyphCachePadding(), @ref features()
         */
        AbstractStyle& setTextLayerGlyphCacheSize(const Vector3i& size, const Vector2i& padding = {});

        /**
         * @brief Apply the style
         *
         * Used internally from @ref UserInterfaceGL::setStyle() /
         * @relativeref{UserInterfaceGL,trySetStyle()}, you'll likely want to
         * call that function instead if setting up a style on a new
         * @ref UserInterface instance. Direct use of this function is mainly
         * for applying another compatible style to a user interface that
         * already has a style set.
         *
         * Expects that @p features are a subset of @ref features() and contain
         * at least one feature, that @p ui already contains all layers
         * corresponding to @p features and that their shared state style
         * uniform and style count matches the subset of
         * @ref baseLayerStyleUniformCount(), @ref baseLayerStyleCount(),
         * @ref textLayerStyleUniformCount(), @ref textLayerStyleCount()
         * matching @p features. Additionally, if @ref StyleFeature::TextLayer
         * is present in @p features, expects that the @ref TextLayer::Shared
         * instance has a glyph cache set that matches
         * @ref textLayerGlyphCacheFormat(), @ref textLayerGlyphCacheSize() for
         * @p features and @ref textLayerGlyphCachePadding(), and that
         * @p fontManager is not @cpp nullptr @ce; and if
         * @ref StyleFeature::TextLayerImages is present in @p features,
         * expects that either the @ref TextLayer is already present in the
         * user interface or that @ref StyleFeature::TextLayer is included in
         * @p features as well, and that @p importerManager is not
         * @cpp nullptr @ce. Returns @cpp true @ce on success, prints a message
         * to @relativeref{Magnum,Error} and returns @cpp false @ce if some
         * run-time error happened during style preparation, such as a plugin
         * not being found or external data failing to load.
         */
        bool apply(UserInterface& ui, StyleFeatures features, PluginManager::Manager<Trade::AbstractImporter>* importerManager, PluginManager::Manager<Text::AbstractFont>* fontManager) const;

    private:
        /**
         * @brief Implementation for @ref features()
         *
         * Is expected to return at least one @ref StyleFeature.
         */
        virtual StyleFeatures doFeatures() const = 0;

        /**
         * @brief Implementation for @ref baseLayerStyleUniformCount()
         *
         * Guaranteed to be called only if @ref doFeatures() contains
         * @ref StyleFeature::BaseLayer. Default implementation delegates to
         * @ref doBaseLayerStyleCount().
         */
        virtual UnsignedInt doBaseLayerStyleUniformCount() const;

        /**
         * @brief Implementation for @ref baseLayerStyleCount()
         *
         * Guaranteed to be called only if @ref doFeatures() contains
         * @ref StyleFeature::BaseLayer. Has to be implemented if
         * @ref doFeatures() contains @ref StyleFeature::BaseLayer.
         */
        virtual UnsignedInt doBaseLayerStyleCount() const;

        /**
         * @brief Implementation for @ref textLayerStyleUniformCount()
         *
         * Guaranteed to be called only if @ref doFeatures() contains
         * @ref StyleFeature::TextLayer. Default implementation delegates to
         * @ref doTextLayerStyleCount().
         */
        virtual UnsignedInt doTextLayerStyleUniformCount() const;

        /**
         * @brief Implementation for @ref textLayerStyleCount()
         *
         * Guaranteed to be called only if @ref doFeatures() contains
         * @ref StyleFeature::TextLayer. Has to be implemented if
         * @ref doFeatures() contains @ref StyleFeature::TextLayer.
         */
        virtual UnsignedInt doTextLayerStyleCount() const;

        /**
         * @brief Implementation for @ref textLayerGlyphCacheFormat()
         *
         * Guaranteed to be called only if @ref doFeatures() contains
         * @ref StyleFeature::TextLayer. Default implementation returns
         * @ref PixelFormat::R8Unorm.
         */
        virtual PixelFormat doTextLayerGlyphCacheFormat() const;

        /**
         * @brief Implementation for @ref textLayerGlyphCacheSize()
         *
         * Guaranteed to be called only if @ref doFeatures() contains
         * @ref StyleFeature::TextLayer. The @p features are guaranteed to be a
         * subset of @ref features() and contain at least
         * @ref StyleFeature::TextLayer. Has to be implemented if
         * @ref doFeatures() contains @ref StyleFeature::TextLayer.
         */
        virtual Vector3i doTextLayerGlyphCacheSize(StyleFeatures features) const;

        /**
         * @brief Implementation for @ref textLayerGlyphCacheFormat()
         *
         * Guaranteed to be called only if @ref doFeatures() contains
         * @ref StyleFeature::TextLayer. Default implementation returns
         * @cpp {1, 1} @ce, consistently with default padding in
         * @ref Text::AbstractGlyphCache, to prevent artifacts. See
         * @ref Text-AbstractGlyphCache-padding for more information.
         */
        virtual Vector2i doTextLayerGlyphCachePadding() const;

        /**
         * @brief Implementation for @ref apply()
         *
         * Should call @ref BaseLayer::Shared::setStyle(),
         * @ref BaseLayer::Shared::setStyleTransition(),
         * @ref TextLayer::Shared::addFont(),
         * @ref TextLayer::Shared::setStyle() and
         * @ref TextLayer::Shared::setStyleTransition() with style contents
         * based on what @p features are passed and return @cpp true @ce. If
         * some runtime error happens, should print a message to
         * @relativeref{Magnum,Error} and return @cpp false @ce.
         *
         * The @p features are guaranteed to be a subset of @ref features() and
         * contain at least one feature, that @p ui already contains all layers
         * corresponding to @p features and that their shared state style
         * uniform and style count matches the subset of
         * @ref baseLayerStyleUniformCount(),
         * @ref baseLayerStyleCount(), @ref textLayerStyleUniformCount(),
         * @ref textLayerStyleCount() matching @p features. Additionally, if
         * @ref StyleFeature::TextLayer is present in @p features, the
         * @ref TextLayer::Shared instance is guaranteed to have a glyph cache
         * set that matches @ref textLayerGlyphCacheFormat(),
         * @ref textLayerGlyphCacheSize() for @p features and
         * @ref textLayerGlyphCachePadding(), and @p fontManager is guaranteed
         * to not be @cpp nullptr @ce; and if
         * @ref StyleFeature::TextLayerImages is present in @p features, the
         * @p importerManager is guaranteed to not be @cpp nullptr @ce.
         */
        virtual bool doApply(UserInterface& ui, StyleFeatures features, PluginManager::Manager<Trade::AbstractImporter>* importerManager, PluginManager::Manager<Text::AbstractFont>* fontManager) const = 0;

        /* When more user-overridable properties are present, might want to put
           them into a PIMPL instead, and remove the Vector3 include again */
        Vector3i _textLayerGlyphCacheSize;
        Vector2i _textLayerGlyphCachePadding;
};

}}

#endif
