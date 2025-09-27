#ifndef Magnum_Ui_Style_h
#define Magnum_Ui_Style_h
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
 * @brief Class @ref Magnum::Ui::McssDarkStyle, enum @ref Magnum::Ui::Icon
 * @m_since_latest
 */

#include "Magnum/Ui/AbstractStyle.h"

namespace Magnum { namespace Ui {

/**
@brief Icon
@m_since_latest

For use in @ref Button and other widgets.
*/
enum class Icon: UnsignedInt {
    /**
     * No icon. When returned, means given widget has no icon. When passed as
     * an argument, causes a widget icon to be unset.
     */
    None = 0,

    Yes,        /**< Yes */
    No          /**< No */
};

/**
@debugoperatorenum{Icon}
@m_since_latest
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, Icon value);

namespace Implementation {
    /* Used by various tests, less wasteful to have here than in the
       potentially huge Style.hpp */
    enum: UnsignedInt {
        BaseStyleCount = 61,
        BaseStyleUniformCount = BaseStyleCount,
        TextStyleCount = 91,
        TextStyleUniformCount = 33,
        TextEditingStyleCount = 16,
        TextEditingStyleUniformCount = TextEditingStyleCount,
        IconCount = 2
    };
}

/**
@brief Style for builtin widgets based on the [m.css](https://mcss.mosra.cz) dark theme
@m_since_latest

An instance of this class is meant to be passed to the @ref UserInterfaceGL
constructor, to @ref UserInterfaceGL::create(), @relativeref{UserInterfaceGL,tryCreate()}
or @relativeref{UserInterfaceGL,setStyle()}. See the
@ref AbstractUserInterface "AbstractUserInterface introduction docs" for a
step-by-step guide for setting up an user interface.

@section Ui-McssDarkStyle-animations Style animations

By default the style is created without any animations to make the initial UI
setup easier. Once the application is @ref Ui-AbstractUserInterface-animations "set up for animations",
you can pass @ref Feature::Animations to the constructor to enable fade out
animations in certain style transitions and an animated cursor in text editing
fields. In comparison, for a distraction-less behavior,
@ref Feature::EssentialAnimations is just the text editing cursor alone,
blinking without any smooth transition.
*/
class MAGNUM_UI_EXPORT McssDarkStyle: public AbstractStyle {
    public:
        /**
         * @brief Style feature
         *
         * @see @ref Features, @ref McssDarkStyle()
         */
        enum class Feature: UnsignedByte {
            /**
             * Enable just essential animations, which is currently a blinking
             * cursor for text editing. Subset of @ref Feature::Animations.
             */
            EssentialAnimations = 1 << 0,

            /**
             * Enable all animations, which include fade out on pointer out,
             * release and blur, as well as an animated text editing cursor.
             * Superset of @ref Feature::EssentialAnimations.
             */
            Animations = EssentialAnimations|(1 << 1)
        };

        /**
         * @brief Style features
         *
         * @see @ref McssDarkStyle()
         */
        typedef Containers::EnumSet<Feature> Features;

        /**
         * @brief Constructor
         * @param features  Style features to enable
         */
        explicit McssDarkStyle(Features features = {});

    private:
        MAGNUM_UI_LOCAL StyleFeatures doFeatures() const override;
        MAGNUM_UI_LOCAL UnsignedInt doBaseLayerStyleUniformCount() const override;
        MAGNUM_UI_LOCAL UnsignedInt doBaseLayerStyleCount() const override;
        MAGNUM_UI_LOCAL UnsignedInt doBaseLayerDynamicStyleCount() const override;
        MAGNUM_UI_LOCAL UnsignedInt doTextLayerStyleUniformCount() const override;
        MAGNUM_UI_LOCAL UnsignedInt doTextLayerStyleCount() const override;
        MAGNUM_UI_LOCAL UnsignedInt doTextLayerDynamicStyleCount() const override;
        MAGNUM_UI_LOCAL UnsignedInt doTextLayerEditingStyleUniformCount() const override;
        MAGNUM_UI_LOCAL UnsignedInt doTextLayerEditingStyleCount() const override;
        MAGNUM_UI_LOCAL Vector3i doTextLayerGlyphCacheSize(StyleFeatures features) const override;
        MAGNUM_UI_LOCAL bool doApply(UserInterface& ui, StyleFeatures features, PluginManager::Manager<Trade::AbstractImporter>* importerManager, PluginManager::Manager<Text::AbstractFont>* fontManager) const override;

        Features _features;
};

/**
@debugoperatorclassenum{McssDarkStyle,McssDarkStyle::Feature}
@m_since_latest
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, McssDarkStyle::Feature value);

/**
@debugoperatorclassenum{McssDarkStyle,McssDarkStyle::Features}
@m_since_latest
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, McssDarkStyle::Features value);

CORRADE_ENUMSET_OPERATORS(McssDarkStyle::Features)

}}

#endif
