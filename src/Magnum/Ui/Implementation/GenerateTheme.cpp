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

#include <Corrade/Containers/GrowableArray.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/Triple.h>
#include <Corrade/Utility/Arguments.h>
#include <Corrade/Utility/Format.h>
#include <Corrade/Utility/Path.h>
#include <Magnum/Math/Swizzle.h>
#include <Magnum/Text/Alignment.h>

/* All these are just header-only dependencies */
#include "Magnum/Ui/AbstractTheme.hpp"
#include "Magnum/Ui/BaseLayer.h"
#include "Magnum/Ui/TextLayer.h"
#include "Magnum/Ui/Implementation/Theme.h" /* TextFont enum */

using namespace Magnum;
using namespace Containers::Literals;
using namespace Math::Literals;

using Magnum::Ui::Implementation::BaseStyle;
using Magnum::Ui::Implementation::TextStyle;
using Magnum::Ui::Implementation::TextFont;
using Magnum::Ui::Implementation::LayoutStyle;

namespace {

enum class TextEditingStyle {
    None = -1,

    InputCursorNone = 0,

    InputCursorDefault,
    InputCursorFocusedDefault,
    InputSelectionDefault,

    InputCursorSuccess,
    InputCursorFocusedSuccess,
    InputSelectionSuccess,

    InputCursorWarning,
    InputCursorFocusedWarning,
    InputSelectionWarning,

    InputCursorDanger,
    InputCursorFocusedDanger,
    InputSelectionDanger,

    InputCursorFlat,
    InputCursorFocusedFlat,
    InputSelectionFlat,

    Count
};

struct TextLayerStyle {
    TextFont font = TextFont::Main;
    Text::Alignment alignment = Text::Alignment::MiddleCenter;
    TextEditingStyle cursorStyle = TextEditingStyle::None,
        selectionStyle = TextEditingStyle::None;
    Vector4 padding;
};

struct LayoutLayerStyle {
    Vector2 minSize;
    Vector4 padding, margin;
};

void dark(Containers::ArrayView<Ui::BaseLayerStyleUniform> baseUniforms, Containers::ArrayView<Ui::TextLayerStyleUniform> textUniforms, Containers::ArrayView<TextLayerStyle> textStyles, Containers::ArrayView<Ui::TextLayerEditingStyleUniform> textEditingUniforms, Containers::ArrayView<Vector4> textEditingPaddings, Containers::ArrayView<Ui::TextLayerStyleUniform> textSelectionUniforms, Containers::ArrayView<LayoutLayerStyle> layoutStyles) {
    const Float baseOpacity = 0.8f;
    const Float disabledOpacity = 0.3f;

    /* Compared to m.css, which has corner radius 0.2 rem (of 16px, so 3.2px),
       the radius is 4 because 3.2 looks too rough. */
    const Float cornerRadius = 4.0f;

    /* Compared to m.css, which has both and margin and padding 1rem (= 16px,
       matching font size), the spacing is slightly reduced here. */
    const Vector4 contentWidgetPadding = {16.0f, 12.0f, 16.0f, 12.0f};
    const Vector4 leafWidgetMargin = {12.0f, 10.0f, 12.0f, 10.0f};

    /* Buttons ------------------------------------------------------------- */

    /* Using m.css' --*-color for default color, --*-button-active-color for
       hover/pressed color. Everything has 80% opacity, the disabled state is
       default but with 30% opacity. */
    for(auto&& i: {BaseStyle::ButtonDefault,
                   BaseStyle::ButtonDefaultPressed})
        baseUniforms[Int(i)].setColor({0xdcdcdc_rgbf, baseOpacity});
    for(auto&& i: {BaseStyle::ButtonDefaultHovered,
                   BaseStyle::ButtonDefaultPressedHovered})
        baseUniforms[Int(i)].setColor({0xa5c9ea_rgbf, baseOpacity});
    baseUniforms[Int(BaseStyle::ButtonDefaultDisabled)]
        .setColor({0xdcdcdc_rgbf, disabledOpacity});

    for(auto&& i: {BaseStyle::ButtonPrimary,
                   BaseStyle::ButtonPrimaryPressed})
        baseUniforms[Int(i)].setColor({0xa5c9ea_rgbf, baseOpacity});
    for(auto&& i: {BaseStyle::ButtonPrimaryHovered,
                   BaseStyle::ButtonPrimaryPressedHovered})
        baseUniforms[Int(i)].setColor({0xdcdcdc_rgbf, baseOpacity});
    baseUniforms[Int(BaseStyle::ButtonPrimaryDisabled)]
        .setColor({0xa5c9ea_rgbf, disabledOpacity});

    for(auto&& i: {BaseStyle::ButtonSuccess,
                   BaseStyle::ButtonSuccessPressed})
        baseUniforms[Int(i)].setColor({0x3bd267_rgbf, baseOpacity});
    for(auto&& i: {BaseStyle::ButtonSuccessHovered,
                   BaseStyle::ButtonSuccessPressedHovered})
        baseUniforms[Int(i)].setColor({0xacecbe_rgbf, baseOpacity});
    baseUniforms[Int(BaseStyle::ButtonSuccessDisabled)]
        .setColor({0x3bd267_rgbf, disabledOpacity});

    for(auto&& i: {BaseStyle::ButtonWarning,
                   BaseStyle::ButtonWarningPressed})
        baseUniforms[Int(i)].setColor({0xc7cf2f_rgbf, baseOpacity});
    for(auto&& i: {BaseStyle::ButtonWarningHovered,
                   BaseStyle::ButtonWarningPressedHovered})
        baseUniforms[Int(i)].setColor({0xe9ecae_rgbf, baseOpacity});
    baseUniforms[Int(BaseStyle::ButtonWarningDisabled)]
        .setColor({0xc7cf2f_rgbf, disabledOpacity});

    for(auto&& i: {BaseStyle::ButtonDanger,
                   BaseStyle::ButtonDangerPressed})
        baseUniforms[Int(i)].setColor({0xcd3431_rgbf, baseOpacity});
    for(auto&& i: {BaseStyle::ButtonDangerHovered,
                   BaseStyle::ButtonDangerPressedHovered})
        baseUniforms[Int(i)].setColor({0xff9391_rgbf, baseOpacity});
    baseUniforms[Int(BaseStyle::ButtonDangerDisabled)]
        .setColor({0xcd3431_rgbf, disabledOpacity});

    for(auto&& i: {BaseStyle::ButtonInfo,
                   BaseStyle::ButtonInfoPressed})
        baseUniforms[Int(i)].setColor({0x2f83cc_rgbf, baseOpacity});
    for(auto&& i: {BaseStyle::ButtonInfoHovered,
                   BaseStyle::ButtonInfoPressedHovered})
        baseUniforms[Int(i)].setColor({0xa5caeb_rgbf, baseOpacity});
    baseUniforms[Int(BaseStyle::ButtonInfoDisabled)]
        .setColor({0x2f83cc_rgbf, disabledOpacity});

    for(auto&& i: {BaseStyle::ButtonDim,
                   BaseStyle::ButtonDimPressed})
        baseUniforms[Int(i)].setColor({0x747474_rgbf, baseOpacity});
    for(auto&& i: {BaseStyle::ButtonDimHovered,
                   BaseStyle::ButtonDimPressedHovered})
        baseUniforms[Int(i)].setColor({0xacacac_rgbf, baseOpacity});
    baseUniforms[Int(BaseStyle::ButtonDimDisabled)]
        .setColor({0x747474_rgbf, disabledOpacity});

    /* All buttons have the same corner radius */
    for(auto&& i: {BaseStyle::ButtonDefault,
                   BaseStyle::ButtonDefaultHovered,
                   BaseStyle::ButtonDefaultPressed,
                   BaseStyle::ButtonDefaultPressedHovered,
                   BaseStyle::ButtonDefaultDisabled,
                   BaseStyle::ButtonPrimary,
                   BaseStyle::ButtonPrimaryHovered,
                   BaseStyle::ButtonPrimaryPressed,
                   BaseStyle::ButtonPrimaryPressedHovered,
                   BaseStyle::ButtonPrimaryDisabled,
                   BaseStyle::ButtonSuccess,
                   BaseStyle::ButtonSuccessHovered,
                   BaseStyle::ButtonSuccessPressed,
                   BaseStyle::ButtonSuccessPressedHovered,
                   BaseStyle::ButtonSuccessDisabled,
                   BaseStyle::ButtonWarning,
                   BaseStyle::ButtonWarningHovered,
                   BaseStyle::ButtonWarningPressed,
                   BaseStyle::ButtonWarningPressedHovered,
                   BaseStyle::ButtonWarningDisabled,
                   BaseStyle::ButtonDanger,
                   BaseStyle::ButtonDangerHovered,
                   BaseStyle::ButtonDangerPressed,
                   BaseStyle::ButtonDangerPressedHovered,
                   BaseStyle::ButtonDangerDisabled,
                   BaseStyle::ButtonInfo,
                   BaseStyle::ButtonInfoHovered,
                   BaseStyle::ButtonInfoPressed,
                   BaseStyle::ButtonInfoPressedHovered,
                   BaseStyle::ButtonInfoDisabled,
                   BaseStyle::ButtonDim,
                   BaseStyle::ButtonDimHovered,
                   BaseStyle::ButtonDimPressed,
                   BaseStyle::ButtonDimPressedHovered,
                   BaseStyle::ButtonDimDisabled})
        baseUniforms[Int(i)].setCornerRadius(cornerRadius);

    /* Flat button outline matches the text color, styled the same as links
       (--link-color, --link-active-color). Disabled state is --dim-link-color
       with 80% opacity. Make sure the inner outline radius is same as outer if
       the outline has zero width, to not have fade out animations look
       weird. */
    for(auto&& i: {BaseStyle::ButtonFlat,
                   BaseStyle::ButtonFlatDisabled})
        baseUniforms[Int(i)]
            .setColor(0x00000000_rgbaf)
            .setOutlineColor(0x00000000_rgbaf)
            .setOutlineWidth(0.0f)
            .setCornerRadius(cornerRadius)
            .setInnerOutlineCornerRadius(cornerRadius);
    for(auto&& i: {BaseStyle::ButtonFlatHovered,
                   BaseStyle::ButtonFlatPressedHovered})
        baseUniforms[Int(i)]
            .setColor(0x00000000_rgbaf)
            .setOutlineColor(0xa5c9ea_rgbf)
            .setOutlineWidth(1.5f)
            .setCornerRadius(cornerRadius)
            .setInnerOutlineCornerRadius(cornerRadius - 1.5f);
    baseUniforms[Int(BaseStyle::ButtonFlatPressed)]
        .setColor(0x00000000_rgbaf)
        .setOutlineColor(0x5b9dd9_rgbf)
        .setOutlineWidth(1.5f)
        .setCornerRadius(cornerRadius)
        .setInnerOutlineCornerRadius(cornerRadius - 1.5f);

    /* Button text color is always --button-background-color. Compared to the
       background the text is fully opaque. Disabled button text color is
       --background-color with 80% opacity. */
    for(auto&& i: {TextStyle::ButtonIconOnly,
                   TextStyle::ButtonTextOnly,
                   TextStyle::ButtonIcon,
                   TextStyle::ButtonText,
                   TextStyle::ButtonPressedIconOnly,
                   TextStyle::ButtonPressedTextOnly,
                   TextStyle::ButtonPressedIcon,
                   TextStyle::ButtonPressedText})
        textUniforms[Int(i)].setColor(0x22272e_rgbf);
    for(auto&& i: {TextStyle::ButtonDisabledIconOnly,
                   TextStyle::ButtonDisabledTextOnly,
                   TextStyle::ButtonDisabledIcon,
                   TextStyle::ButtonDisabledText})
        textUniforms[Int(i)].setColor({0x2f363f_rgbf, 0.8f});

    /* Flat button text is styled the same as links (--link-color,
       --link-active-color). Disabled state is 30% opacity same as with button background color. */
    for(auto&& i: {TextStyle::ButtonFlatIconOnly,
                   TextStyle::ButtonFlatTextOnly,
                   TextStyle::ButtonFlatIcon,
                   TextStyle::ButtonFlatText,
                   TextStyle::ButtonFlatPressedIconOnly,
                   TextStyle::ButtonFlatPressedTextOnly,
                   TextStyle::ButtonFlatPressedIcon,
                   TextStyle::ButtonFlatPressedText})
        textUniforms[Int(i)].setColor(0x5b9dd9_rgbf);
    for(auto&& i: {TextStyle::ButtonFlatHoveredIconOnly,
                   TextStyle::ButtonFlatHoveredTextOnly,
                   TextStyle::ButtonFlatHoveredIcon,
                   TextStyle::ButtonFlatHoveredText,
                   TextStyle::ButtonFlatPressedHoveredIconOnly,
                   TextStyle::ButtonFlatPressedHoveredTextOnly,
                   TextStyle::ButtonFlatPressedHoveredIcon,
                   TextStyle::ButtonFlatPressedHoveredText})
        textUniforms[Int(i)].setColor(0xa5c9ea_rgbf);
    for(auto&& i: {TextStyle::ButtonFlatDisabledIconOnly,
                   TextStyle::ButtonFlatDisabledTextOnly,
                   TextStyle::ButtonFlatDisabledIcon,
                   TextStyle::ButtonFlatDisabledText})
        textUniforms[Int(i)].setColor({0x5b9dd9_rgbf, disabledOpacity});

    /* Button text padding. Generally the horizontal padding is 6 and vertical
       4. The button itself has a min height specified, so the vertical padding
       is here only for cases with larger fonts. */
    for(auto&& i: {TextStyle::ButtonIconOnly,
                   TextStyle::ButtonTextOnly,
                   TextStyle::ButtonDisabledIconOnly,
                   TextStyle::ButtonDisabledTextOnly,
                   TextStyle::ButtonFlatIconOnly,
                   TextStyle::ButtonFlatTextOnly,
                   TextStyle::ButtonFlatHoveredIconOnly,
                   TextStyle::ButtonFlatHoveredTextOnly,
                   TextStyle::ButtonFlatDisabledIconOnly,
                   TextStyle::ButtonFlatDisabledTextOnly})
        textStyles[Int(i)].padding = {6.0f, 4.0f, 6.0f, 4.0f};
    /* Assuming a 24x24 icon and 6-unit gap between, for the icon the
       right-side padding has to include the extra gap as well. The button
       widget then further expands the padding by the actual text size. */
    for(auto&& i: {TextStyle::ButtonIcon,
                   TextStyle::ButtonDisabledIcon,
                   TextStyle::ButtonFlatIcon,
                   TextStyle::ButtonFlatHoveredIcon,
                   TextStyle::ButtonFlatDisabledIcon})
        textStyles[Int(i)].padding = {6.0f, 4.0f, 12.0f, 4.0f};
    /* Conversely, the text then has to include both the icon size and the gap
       in the left-side padding */
    for(auto&& i: {TextStyle::ButtonText,
                   TextStyle::ButtonDisabledText,
                   TextStyle::ButtonFlatText,
                   TextStyle::ButtonFlatHoveredText,
                   TextStyle::ButtonFlatDisabledText})
        textStyles[Int(i)].padding = {36.0f, 4.0f, 6.0f, 4.0f};
    /* Pressed buttons have their icons and text shifted down by 1 unit */
    for(auto&& i: {TextStyle::ButtonPressedIconOnly,
                   TextStyle::ButtonPressedTextOnly,
                   TextStyle::ButtonFlatPressedIconOnly,
                   TextStyle::ButtonFlatPressedTextOnly,
                   TextStyle::ButtonFlatPressedHoveredIconOnly,
                   TextStyle::ButtonFlatPressedHoveredTextOnly})
        textStyles[Int(i)].padding = {6.0f, 5.0f, 6.0f, 3.0f};
    for(auto&& i: {TextStyle::ButtonPressedIcon,
                   TextStyle::ButtonFlatPressedIcon,
                   TextStyle::ButtonFlatPressedHoveredIcon})
        textStyles[Int(i)].padding = {6.0f, 5.0f, 12.0f, 3.0f};
    for(auto&& i: {TextStyle::ButtonPressedText,
                   TextStyle::ButtonFlatPressedText,
                   TextStyle::ButtonFlatPressedHoveredText})
        textStyles[Int(i)].padding = {36.0f, 5.0f, 6.0f, 3.0f};

    /* Icon font assignment */
    for(auto&& i: {TextStyle::ButtonIconOnly,
                   TextStyle::ButtonIcon,
                   TextStyle::ButtonPressedIconOnly,
                   TextStyle::ButtonPressedIcon,
                   TextStyle::ButtonDisabledIconOnly,
                   TextStyle::ButtonDisabledIcon,
                   TextStyle::ButtonFlatIconOnly,
                   TextStyle::ButtonFlatIcon,
                   TextStyle::ButtonFlatPressedIconOnly,
                   TextStyle::ButtonFlatPressedIcon,
                   TextStyle::ButtonFlatHoveredIconOnly,
                   TextStyle::ButtonFlatHoveredIcon,
                   TextStyle::ButtonFlatPressedHoveredIconOnly,
                   TextStyle::ButtonFlatPressedHoveredIcon})
        textStyles[Int(i)].font = TextFont::Icon;

    layoutStyles[Int(LayoutStyle::Button)].minSize = {0.0f, 36.0f};
    layoutStyles[Int(LayoutStyle::Button)].margin = leafWidgetMargin;

    /* Labels -------------------------------------------------------------- */

    /* Label colors are the same as --*-color. Disabled state is 30% opacity,
       same as with button background color. */
    for(auto&& i: {TextStyle::LabelDefaultIcon,
                   TextStyle::LabelDefaultText})
        textUniforms[Int(i)].setColor(0xdcdcdc_rgbf);
    for(auto&& i: {TextStyle::LabelDefaultDisabledIcon,
                   TextStyle::LabelDefaultDisabledText})
        textUniforms[Int(i)].setColor({0xdcdcdc_rgbf, disabledOpacity});

    for(auto&& i: {TextStyle::LabelPrimaryIcon,
                   TextStyle::LabelPrimaryText})
        textUniforms[Int(i)].setColor(0xa5c9ea_rgbf);
    for(auto&& i: {TextStyle::LabelPrimaryDisabledIcon,
                   TextStyle::LabelPrimaryDisabledText})
        textUniforms[Int(i)].setColor({0xa5c9ea_rgbf, disabledOpacity});

    for(auto&& i: {TextStyle::LabelSuccessIcon,
                   TextStyle::LabelSuccessText})
        textUniforms[Int(i)].setColor(0x3bd267_rgbf);
    for(auto&& i: {TextStyle::LabelSuccessDisabledIcon,
                   TextStyle::LabelSuccessDisabledText})
        textUniforms[Int(i)].setColor({0x3bd267_rgbf, disabledOpacity});

    for(auto&& i: {TextStyle::LabelWarningIcon,
                   TextStyle::LabelWarningText})
        textUniforms[Int(i)].setColor(0xc7cf2f_rgbf);
    for(auto&& i: {TextStyle::LabelWarningDisabledIcon,
                   TextStyle::LabelWarningDisabledText})
        textUniforms[Int(i)].setColor({0xc7cf2f_rgbf, disabledOpacity});

    for(auto&& i: {TextStyle::LabelDangerIcon,
                   TextStyle::LabelDangerText})
        textUniforms[Int(i)].setColor(0xcd3431_rgbf);
    for(auto&& i: {TextStyle::LabelDangerDisabledIcon,
                   TextStyle::LabelDangerDisabledText})
        textUniforms[Int(i)].setColor({0xcd3431_rgbf, disabledOpacity});

    for(auto&& i: {TextStyle::LabelInfoIcon,
                   TextStyle::LabelInfoText})
        textUniforms[Int(i)].setColor(0x2f83cc_rgbf);
    for(auto&& i: {TextStyle::LabelInfoDisabledIcon,
                   TextStyle::LabelInfoDisabledText})
        textUniforms[Int(i)].setColor({0x2f83cc_rgbf, disabledOpacity});

    for(auto&& i: {TextStyle::LabelDimIcon,
                   TextStyle::LabelDimText})
        textUniforms[Int(i)].setColor(0x747474_rgbf);
    for(auto&& i: {TextStyle::LabelDimDisabledIcon,
                   TextStyle::LabelDimDisabledText})
        textUniforms[Int(i)].setColor({0x747474_rgbf, disabledOpacity});

    for(auto&& i: {TextStyle::LabelTitleIcon,
                   TextStyle::LabelTitleText})
        textUniforms[Int(i)].setColor(0xdcdcdc_rgbf);
    for(auto&& i: {TextStyle::LabelTitleDisabledIcon,
                   TextStyle::LabelTitleDisabledText})
        textUniforms[Int(i)].setColor({0xdcdcdc_rgbf, disabledOpacity});

    /* Label icon font */
    for(auto&& i: {TextStyle::LabelDefaultIcon,
                   TextStyle::LabelDefaultDisabledIcon,
                   TextStyle::LabelPrimaryIcon,
                   TextStyle::LabelPrimaryDisabledIcon,
                   TextStyle::LabelSuccessIcon,
                   TextStyle::LabelSuccessDisabledIcon,
                   TextStyle::LabelWarningIcon,
                   TextStyle::LabelWarningDisabledIcon,
                   TextStyle::LabelDangerIcon,
                   TextStyle::LabelDangerDisabledIcon,
                   TextStyle::LabelInfoIcon,
                   TextStyle::LabelInfoDisabledIcon,
                   TextStyle::LabelDimIcon,
                   TextStyle::LabelDimDisabledIcon})
        textStyles[Int(i)].font = TextFont::Icon;

    /* Large label font / icon font */
    for(auto&& i: {TextStyle::LabelTitleText,
                   TextStyle::LabelTitleDisabledText})
        textStyles[Int(i)].font = TextFont::Large;
    for(auto&& i: {TextStyle::LabelTitleIcon,
                   TextStyle::LabelTitleDisabledIcon})
        textStyles[Int(i)].font = TextFont::LargeIcon;

    layoutStyles[Int(LayoutStyle::Label)].minSize = {0.0f, 24.0f};
    /* Title label spans the same height as a button */
    layoutStyles[Int(LayoutStyle::LabelTitle)].minSize = {0.0f, 36.0f};
    layoutStyles[Int(LayoutStyle::Label)].margin = leafWidgetMargin;
    layoutStyles[Int(LayoutStyle::LabelTitle)].margin = leafWidgetMargin;

    /* Inputs -------------------------------------------------------------- */

    /* Input is --*-filled-background-color for background, --*-color for left
       edge if not hovered or focused, --*-link-active-color for left edge and
       outline if hovered or focused. */
    baseUniforms[Int(BaseStyle::InputDefault)]
        .setColor({0x34424d_rgbf, baseOpacity})
        .setOutlineColor({0xdcdcdc_rgbf, baseOpacity});
    for(auto&& i: {BaseStyle::InputDefaultHovered,
                   BaseStyle::InputDefaultFocused})
        baseUniforms[Int(i)]
            .setColor({0x34424d_rgbf, baseOpacity})
            .setOutlineColor({0xa5c9ea_rgbf, baseOpacity});
    baseUniforms[Int(BaseStyle::InputDefaultDisabled)]
        .setColor({0x34424d_rgbf, disabledOpacity})
        .setOutlineColor({0xdcdcdc_rgbf, disabledOpacity});

    baseUniforms[Int(BaseStyle::InputSuccess)]
        .setColor({0x2a703f_rgbf, baseOpacity})
        .setOutlineColor({0x3bd267_rgbf, baseOpacity});
    for(auto&& i: {BaseStyle::InputSuccessHovered,
                   BaseStyle::InputSuccessFocused})
        baseUniforms[Int(i)]
            .setColor({0x2a703f_rgbf, baseOpacity})
            .setOutlineColor({0xacecbe_rgbf, baseOpacity});
    baseUniforms[Int(BaseStyle::InputSuccessDisabled)]
        .setColor({0x2a703f_rgbf, disabledOpacity})
        .setOutlineColor({0x3bd267_rgbf, disabledOpacity});

    baseUniforms[Int(BaseStyle::InputWarning)]
        .setColor({0x6d702a_rgbf, baseOpacity})
        .setOutlineColor({0xc7cf2f_rgbf, baseOpacity});
    for(auto&& i: {BaseStyle::InputWarningHovered,
                   BaseStyle::InputWarningFocused})
        baseUniforms[Int(i)]
            .setColor({0x6d702a_rgbf, baseOpacity})
            .setOutlineColor({0xe9ecae_rgbf, baseOpacity});
    baseUniforms[Int(BaseStyle::InputWarningDisabled)]
        .setColor({0x6d702a_rgbf, disabledOpacity})
        .setOutlineColor({0xc7cf2f_rgbf, disabledOpacity});

    baseUniforms[Int(BaseStyle::InputDanger)]
        .setColor({0x702b2a_rgbf, baseOpacity})
        .setOutlineColor({0xcd3431_rgbf, baseOpacity});
    for(auto&& i: {BaseStyle::InputDangerHovered,
                   BaseStyle::InputDangerFocused})
        baseUniforms[Int(i)]
            .setColor({0x702b2a_rgbf, baseOpacity})
            .setOutlineColor({0xff9391_rgbf, baseOpacity});
    baseUniforms[Int(BaseStyle::InputDangerDisabled)]
        .setColor({0x702b2a_rgbf, disabledOpacity})
        .setOutlineColor({0xcd3431_rgbf, disabledOpacity});

    /* Flat input has --link-color outline when hovered, to distinguish from
       --link-active-color of a hovered button, and --link-active-color outline
       when pressed or focused. When focused, it matches the text color. */
    baseUniforms[Int(BaseStyle::InputFlat)]
        .setColor(0x00000000_rgbaf)
        .setOutlineColor(0x00000000_rgbaf);
    baseUniforms[Int(BaseStyle::InputFlatHovered)]
        .setColor(0x00000000_rgbaf)
        .setOutlineColor(0x5b9dd9_rgbf);
    baseUniforms[Int(BaseStyle::InputFlatFocused)]
        .setColor(0x00000000_rgbaf)
        .setOutlineColor(0xa5c9ea_rgbf);
    baseUniforms[Int(BaseStyle::InputFlatDisabled)]
        .setColor(0x00000000_rgbaf)
        .setOutlineColor(0x00000000_rgbaf);

    /* All inputs except for flat have the same left-side outline, which
       changes to all-side outline when focused. Flat input has outline visible
       only when hovered or focused. Make sure the inner outline radius is same
       as outer if the outline has zero width, to not have fade out animations
       look weird. */
    for(auto&& i: {BaseStyle::InputDefault,
                   BaseStyle::InputDefaultHovered,
                   BaseStyle::InputDefaultDisabled,
                   BaseStyle::InputSuccess,
                   BaseStyle::InputSuccessHovered,
                   BaseStyle::InputSuccessDisabled,
                   BaseStyle::InputWarning,
                   BaseStyle::InputWarningHovered,
                   BaseStyle::InputWarningDisabled,
                   BaseStyle::InputDanger,
                   BaseStyle::InputDangerHovered,
                   BaseStyle::InputDangerDisabled})
        baseUniforms[Int(i)]
            /* left, top, right, bottom */
            .setOutlineWidth({4.0f, 0.0f, 0.0f, 0.0f})
            .setCornerRadius(cornerRadius)
            .setInnerOutlineCornerRadius({0.0f, 0.0f, 4.0f, 4.0f});
    for(auto&& i: {BaseStyle::InputFlat,
                   BaseStyle::InputFlatDisabled})
        baseUniforms[Int(i)]
            /* left, top, right, bottom */
            .setOutlineWidth({0.0f, 0.0f, 0.0f, 0.0f})
            .setCornerRadius(cornerRadius)
            .setInnerOutlineCornerRadius({4.0f, 4.0f, 4.0f, 4.0f});
    for(auto&& i: {BaseStyle::InputDefaultFocused,
                   BaseStyle::InputSuccessFocused,
                   BaseStyle::InputWarningFocused,
                   BaseStyle::InputDangerFocused,
                   BaseStyle::InputFlatHovered,
                   BaseStyle::InputFlatFocused})
        baseUniforms[Int(i)]
            .setOutlineWidth({4.0f, 1.0f, 1.0f, 1.0f})
            .setCornerRadius(cornerRadius)
            .setInnerOutlineCornerRadius({1.0f, 1.0f, 3.0f, 3.0f});

    /* Input text is always the same regardless of hover, --*-link-active-
       color. Disabled again 30% opacity. */
    for(auto&& i: {TextStyle::InputDefault,
                   TextStyle::InputDefaultHovered,
                   TextStyle::InputDefaultFocused,
                   TextStyle::InputDefaultFocusedBlink,
                   TextStyle::InputDefaultPressed,
                   TextStyle::InputDefaultPassword,
                   TextStyle::InputDefaultPasswordHovered,
                   TextStyle::InputDefaultPasswordFocused,
                   TextStyle::InputDefaultPasswordFocusedBlink,
                   TextStyle::InputDefaultPasswordPressed})
        textUniforms[Int(i)].setColor(0xdcdcdc_rgbf);
    for(auto&& i: {TextStyle::InputDefaultDisabled,
                   TextStyle::InputDefaultPasswordDisabled})
        textUniforms[Int(i)].setColor({0xdcdcdc_rgbf, disabledOpacity});

    for(auto&& i: {TextStyle::InputSuccess,
                   TextStyle::InputSuccessHovered,
                   TextStyle::InputSuccessFocused,
                   TextStyle::InputSuccessFocusedBlink,
                   TextStyle::InputSuccessPressed,
                   TextStyle::InputSuccessPassword,
                   TextStyle::InputSuccessPasswordHovered,
                   TextStyle::InputSuccessPasswordFocused,
                   TextStyle::InputSuccessPasswordFocusedBlink,
                   TextStyle::InputSuccessPasswordPressed})
        textUniforms[Int(i)].setColor(0xacecbe_rgbf);
    for(auto&& i: {TextStyle::InputSuccessDisabled,
                   TextStyle::InputSuccessPasswordDisabled})
        textUniforms[Int(i)].setColor({0xacecbe_rgbf, disabledOpacity});

    for(auto&& i: {TextStyle::InputWarning,
                   TextStyle::InputWarningHovered,
                   TextStyle::InputWarningFocused,
                   TextStyle::InputWarningFocusedBlink,
                   TextStyle::InputWarningPressed,
                   TextStyle::InputWarningPassword,
                   TextStyle::InputWarningPasswordHovered,
                   TextStyle::InputWarningPasswordFocused,
                   TextStyle::InputWarningPasswordFocusedBlink,
                   TextStyle::InputWarningPasswordPressed})
        textUniforms[Int(i)].setColor(0xe9ecae_rgbf);
    for(auto&& i: {TextStyle::InputWarningDisabled,
                   TextStyle::InputWarningPasswordDisabled})
        textUniforms[Int(i)].setColor({0xe9ecae_rgbf, disabledOpacity});

    for(auto&& i: {TextStyle::InputDanger,
                   TextStyle::InputDangerHovered,
                   TextStyle::InputDangerFocused,
                   TextStyle::InputDangerFocusedBlink,
                   TextStyle::InputDangerPressed,
                   TextStyle::InputDangerPassword,
                   TextStyle::InputDangerPasswordHovered,
                   TextStyle::InputDangerPasswordFocused,
                   TextStyle::InputDangerPasswordFocusedBlink,
                   TextStyle::InputDangerPasswordPressed})
        textUniforms[Int(i)].setColor(0xff9391_rgbf);
    for(auto&& i: {TextStyle::InputDangerDisabled,
                   TextStyle::InputDangerPasswordDisabled})
        textUniforms[Int(i)].setColor({0xff9391_rgbf, disabledOpacity});

    /* Flat input text is --link-active-color */
    for(auto&& i: {TextStyle::InputFlat,
                   TextStyle::InputFlatHovered,
                   TextStyle::InputFlatFocused,
                   TextStyle::InputFlatFocusedBlink,
                   TextStyle::InputFlatPressed,
                   TextStyle::InputFlatPassword,
                   TextStyle::InputFlatPasswordHovered,
                   TextStyle::InputFlatPasswordFocused,
                   TextStyle::InputFlatPasswordFocusedBlink,
                   TextStyle::InputFlatPasswordPressed})
        textUniforms[Int(i)].setColor(0xa5c9ea_rgbf);
    for(auto&& i: {TextStyle::InputFlatDisabled,
                   TextStyle::InputFlatPasswordDisabled})
        textUniforms[Int(i)].setColor({0xa5c9ea_rgbf, disabledOpacity});

    /* Cursor and selection style assignment. Disabled styles have neither
       cursor nor selection visible. */
    /** @todo ideally just drop TextEditingStyle altogether and have the
        cursor / selection assigned per style directly, and let the tool
        deduplicate those? */
    for(auto&& i: {TextStyle::InputDefault,
                   TextStyle::InputDefaultHovered,
                   TextStyle::InputDefaultFocused,
                   TextStyle::InputDefaultFocusedBlink,
                   TextStyle::InputDefaultPressed,
                   TextStyle::InputDefaultPassword,
                   TextStyle::InputDefaultPasswordHovered,
                   TextStyle::InputDefaultPasswordFocused,
                   TextStyle::InputDefaultPasswordFocusedBlink,
                   TextStyle::InputDefaultPasswordPressed})
        textStyles[Int(i)].selectionStyle = TextEditingStyle::InputSelectionDefault;
    for(auto&& i: {TextStyle::InputDefault,
                   TextStyle::InputDefaultFocusedBlink,
                   TextStyle::InputDefaultPassword,
                   TextStyle::InputDefaultPasswordFocusedBlink})
        textStyles[Int(i)].cursorStyle = TextEditingStyle::InputCursorNone;
    for(auto&& i: {TextStyle::InputDefaultHovered,
                   TextStyle::InputDefaultPasswordHovered})
        textStyles[Int(i)].cursorStyle = TextEditingStyle::InputCursorDefault;
    for(auto&& i: {TextStyle::InputDefaultFocused,
                   TextStyle::InputDefaultPressed,
                   TextStyle::InputDefaultPasswordFocused,
                   TextStyle::InputDefaultPasswordPressed})
        textStyles[Int(i)].cursorStyle = TextEditingStyle::InputCursorFocusedDefault;

    for(auto&& i: {TextStyle::InputSuccess,
                   TextStyle::InputSuccessHovered,
                   TextStyle::InputSuccessFocused,
                   TextStyle::InputSuccessFocusedBlink,
                   TextStyle::InputSuccessPressed,
                   TextStyle::InputSuccessPassword,
                   TextStyle::InputSuccessPasswordHovered,
                   TextStyle::InputSuccessPasswordFocused,
                   TextStyle::InputSuccessPasswordFocusedBlink,
                   TextStyle::InputSuccessPasswordPressed})
        textStyles[Int(i)].selectionStyle = TextEditingStyle::InputSelectionSuccess;
    for(auto&& i: {TextStyle::InputSuccess,
                   TextStyle::InputSuccessFocusedBlink,
                   TextStyle::InputSuccessPassword,
                   TextStyle::InputSuccessPasswordFocusedBlink})
        textStyles[Int(i)].cursorStyle = TextEditingStyle::InputCursorNone;
    for(auto&& i: {TextStyle::InputSuccessHovered,
                   TextStyle::InputSuccessPasswordHovered})
        textStyles[Int(i)].cursorStyle = TextEditingStyle::InputCursorSuccess;
    for(auto&& i: {TextStyle::InputSuccessFocused,
                   TextStyle::InputSuccessPressed,
                   TextStyle::InputSuccessPasswordFocused,
                   TextStyle::InputSuccessPasswordPressed})
        textStyles[Int(i)].cursorStyle = TextEditingStyle::InputCursorFocusedSuccess;

    for(auto&& i: {TextStyle::InputWarning,
                   TextStyle::InputWarningHovered,
                   TextStyle::InputWarningFocused,
                   TextStyle::InputWarningFocusedBlink,
                   TextStyle::InputWarningPressed,
                   TextStyle::InputWarningPassword,
                   TextStyle::InputWarningPasswordHovered,
                   TextStyle::InputWarningPasswordFocused,
                   TextStyle::InputWarningPasswordFocusedBlink,
                   TextStyle::InputWarningPasswordPressed})
        textStyles[Int(i)].selectionStyle = TextEditingStyle::InputSelectionWarning;
    for(auto&& i: {TextStyle::InputWarning,
                   TextStyle::InputWarningFocusedBlink,
                   TextStyle::InputWarningPassword,
                   TextStyle::InputWarningPasswordFocusedBlink})
        textStyles[Int(i)].cursorStyle = TextEditingStyle::InputCursorNone;
    for(auto&& i: {TextStyle::InputWarningHovered,
                   TextStyle::InputWarningPasswordHovered})
        textStyles[Int(i)].cursorStyle = TextEditingStyle::InputCursorWarning;
    for(auto&& i: {TextStyle::InputWarningFocused,
                   TextStyle::InputWarningPressed,
                   TextStyle::InputWarningPasswordFocused,
                   TextStyle::InputWarningPasswordPressed})
        textStyles[Int(i)].cursorStyle = TextEditingStyle::InputCursorFocusedWarning;

    for(auto&& i: {TextStyle::InputDanger,
                   TextStyle::InputDangerHovered,
                   TextStyle::InputDangerFocused,
                   TextStyle::InputDangerFocusedBlink,
                   TextStyle::InputDangerPressed,
                   TextStyle::InputDangerPassword,
                   TextStyle::InputDangerPasswordHovered,
                   TextStyle::InputDangerPasswordFocused,
                   TextStyle::InputDangerPasswordFocusedBlink,
                   TextStyle::InputDangerPasswordPressed})
        textStyles[Int(i)].selectionStyle = TextEditingStyle::InputSelectionDanger;
    for(auto&& i: {TextStyle::InputDanger,
                   TextStyle::InputDangerFocusedBlink,
                   TextStyle::InputDangerPassword,
                   TextStyle::InputDangerPasswordFocusedBlink})
        textStyles[Int(i)].cursorStyle = TextEditingStyle::InputCursorNone;
    for(auto&& i: {TextStyle::InputDangerHovered,
                   TextStyle::InputDangerPasswordHovered})
        textStyles[Int(i)].cursorStyle = TextEditingStyle::InputCursorDanger;
    for(auto&& i: {TextStyle::InputDangerFocused,
                   TextStyle::InputDangerPressed,
                   TextStyle::InputDangerPasswordFocused,
                   TextStyle::InputDangerPasswordPressed})
        textStyles[Int(i)].cursorStyle = TextEditingStyle::InputCursorFocusedDanger;

    for(auto&& i: {TextStyle::InputFlat,
                   TextStyle::InputFlatHovered,
                   TextStyle::InputFlatFocused,
                   TextStyle::InputFlatFocusedBlink,
                   TextStyle::InputFlatPressed,
                   TextStyle::InputFlatPassword,
                   TextStyle::InputFlatPasswordHovered,
                   TextStyle::InputFlatPasswordFocused,
                   TextStyle::InputFlatPasswordFocusedBlink,
                   TextStyle::InputFlatPasswordPressed})
        textStyles[Int(i)].selectionStyle = TextEditingStyle::InputSelectionFlat;
    for(auto&& i: {TextStyle::InputFlat,
                   TextStyle::InputFlatFocusedBlink,
                   TextStyle::InputFlatPassword,
                   TextStyle::InputFlatPasswordFocusedBlink})
        textStyles[Int(i)].cursorStyle = TextEditingStyle::InputCursorNone;
    for(auto&& i: {TextStyle::InputFlatHovered,
                   TextStyle::InputFlatPasswordHovered})
        textStyles[Int(i)].cursorStyle = TextEditingStyle::InputCursorFlat;
    for(auto&& i: {TextStyle::InputFlatFocused,
                   TextStyle::InputFlatPressed,
                   TextStyle::InputFlatPasswordFocused,
                   TextStyle::InputFlatPasswordPressed})
        textStyles[Int(i)].cursorStyle = TextEditingStyle::InputCursorFocusedFlat;

    /* No cursor. Is Used to have all Input* styles with both selection and
       cursor style so the animations can transition between them without
       problems. */
    textEditingUniforms[Int(TextEditingStyle::InputCursorNone)]
        .setBackgroundColor(0x00000000_rgbaf)
        .setCornerRadius(0.0f);
    textEditingPaddings[Int(TextEditingStyle::InputCursorNone)] = {};

    /* Input cursor is always --*-link-active-color. 80% opacity when hovered,
       fully opaque when focused, apart from the 80% opacity matching the
       outline and left edge color. Selection is --*-color with 40% opacity. */
    textEditingUniforms[Int(TextEditingStyle::InputCursorDefault)]
        .setBackgroundColor({0xa5c9ea_rgbf, 0.8f});
    textEditingUniforms[Int(TextEditingStyle::InputCursorFocusedDefault)]
        .setBackgroundColor(0xa5c9ea_rgbf);
    textEditingUniforms[Int(TextEditingStyle::InputSelectionDefault)]
        .setBackgroundColor({0xdcdcdc_rgbf, 0.4f});
    textEditingUniforms[Int(TextEditingStyle::InputCursorSuccess)]
        .setBackgroundColor({0xacecbe_rgbf, 0.8f});
    textEditingUniforms[Int(TextEditingStyle::InputCursorFocusedSuccess)]
        .setBackgroundColor(0xacecbe_rgbf);
    textEditingUniforms[Int(TextEditingStyle::InputSelectionSuccess)]
        .setBackgroundColor({0x3bd267_rgbf, 0.4f});
    textEditingUniforms[Int(TextEditingStyle::InputCursorWarning)]
        .setBackgroundColor({0xe9ecae_rgbf, 0.8f});
    textEditingUniforms[Int(TextEditingStyle::InputCursorFocusedWarning)]
        .setBackgroundColor(0xe9ecae_rgbf);
    textEditingUniforms[Int(TextEditingStyle::InputSelectionWarning)]
        .setBackgroundColor({0xc7cf2f_rgbf, 0.4f});
    textEditingUniforms[Int(TextEditingStyle::InputCursorDanger)]
        .setBackgroundColor({0xff9391_rgbf, 0.8f});
    textEditingUniforms[Int(TextEditingStyle::InputCursorFocusedDanger)]
        .setBackgroundColor(0xff9391_rgbf);
    textEditingUniforms[Int(TextEditingStyle::InputSelectionDanger)]
        .setBackgroundColor({0xcd3431_rgbf, 0.4f});

    /* Flat input cursor is --link-color when hovered, and --link-active-color
       when focused, always opaque, matching the outline. Selection is
       --link-color with 40% opacity. */
    textEditingUniforms[Int(TextEditingStyle::InputCursorFlat)]
        .setBackgroundColor(0x5b9dd9_rgbf);
    textEditingUniforms[Int(TextEditingStyle::InputCursorFocusedFlat)]
        .setBackgroundColor(0xa5c9ea_rgbf);
    textEditingUniforms[Int(TextEditingStyle::InputSelectionFlat)]
        .setBackgroundColor({0x5b9dd9_rgbf, 0.4f});

    /* Cursor, focused cursor and selection padding as well as radius is the
       same for all styles */
    for(auto&& i: {TextEditingStyle::InputCursorDefault,
                   TextEditingStyle::InputCursorSuccess,
                   TextEditingStyle::InputCursorWarning,
                   TextEditingStyle::InputCursorDanger,
                   TextEditingStyle::InputCursorFlat}) {
        textEditingUniforms[Int(i)].setCornerRadius(1.5f);
        textEditingPaddings[Int(i)] = {0.5f, 2.5f, 1.5f, 2.5f};
    }
    for(auto&& i: {TextEditingStyle::InputCursorFocusedDefault,
                   TextEditingStyle::InputCursorFocusedSuccess,
                   TextEditingStyle::InputCursorFocusedWarning,
                   TextEditingStyle::InputCursorFocusedDanger,
                   TextEditingStyle::InputCursorFocusedFlat}) {
        textEditingUniforms[Int(i)].setCornerRadius(1.5f);
        textEditingPaddings[Int(i)] = {0.5f, 3.5f, 2.5f, 3.5f};
    }
    for(auto&& i: {TextEditingStyle::InputSelectionDefault,
                   TextEditingStyle::InputSelectionSuccess,
                   TextEditingStyle::InputSelectionWarning,
                   TextEditingStyle::InputSelectionDanger,
                   TextEditingStyle::InputSelectionFlat}) {
        textEditingUniforms[Int(i)].setCornerRadius(4.0f);
        textEditingPaddings[Int(i)] = {1.0f, 2.0f, 1.0f, 2.0f};
    }

    /* Selection text color is --button-background-color, same as with button
       text */
    for(auto&& i: {TextEditingStyle::InputSelectionDefault,
                   TextEditingStyle::InputSelectionSuccess,
                   TextEditingStyle::InputSelectionWarning,
                   TextEditingStyle::InputSelectionDanger,
                   TextEditingStyle::InputSelectionFlat})
        textSelectionUniforms[Int(i)].setColor({0x22272e_rgbf});

    /* Password font for password inputs */
    for(auto&& i: {TextStyle::InputDefaultPassword,
                   TextStyle::InputDefaultPasswordHovered,
                   TextStyle::InputDefaultPasswordFocused,
                   TextStyle::InputDefaultPasswordFocusedBlink,
                   TextStyle::InputDefaultPasswordPressed,
                   TextStyle::InputDefaultPasswordDisabled,
                   TextStyle::InputSuccessPassword,
                   TextStyle::InputSuccessPasswordHovered,
                   TextStyle::InputSuccessPasswordFocused,
                   TextStyle::InputSuccessPasswordFocusedBlink,
                   TextStyle::InputSuccessPasswordPressed,
                   TextStyle::InputSuccessPasswordDisabled,
                   TextStyle::InputWarningPassword,
                   TextStyle::InputWarningPasswordHovered,
                   TextStyle::InputWarningPasswordFocused,
                   TextStyle::InputWarningPasswordFocusedBlink,
                   TextStyle::InputWarningPasswordPressed,
                   TextStyle::InputWarningPasswordDisabled,
                   TextStyle::InputDangerPassword,
                   TextStyle::InputDangerPasswordHovered,
                   TextStyle::InputDangerPasswordFocused,
                   TextStyle::InputDangerPasswordFocusedBlink,
                   TextStyle::InputDangerPasswordPressed,
                   TextStyle::InputDangerPasswordDisabled,
                   TextStyle::InputFlatPassword,
                   TextStyle::InputFlatPasswordHovered,
                   TextStyle::InputFlatPasswordFocused,
                   TextStyle::InputFlatPasswordFocusedBlink,
                   TextStyle::InputFlatPasswordPressed,
                   TextStyle::InputFlatPasswordDisabled})
        textStyles[Int(i)].font = TextFont::Password;

    /* All inputs align to left */
    for(auto&& i: {TextStyle::InputDefault,
                   TextStyle::InputDefaultHovered,
                   TextStyle::InputDefaultFocused,
                   TextStyle::InputDefaultFocusedBlink,
                   TextStyle::InputDefaultPressed,
                   TextStyle::InputDefaultDisabled,
                   TextStyle::InputDefaultPassword,
                   TextStyle::InputDefaultPasswordHovered,
                   TextStyle::InputDefaultPasswordFocused,
                   TextStyle::InputDefaultPasswordFocusedBlink,
                   TextStyle::InputDefaultPasswordPressed,
                   TextStyle::InputDefaultPasswordDisabled,
                   TextStyle::InputSuccess,
                   TextStyle::InputSuccessHovered,
                   TextStyle::InputSuccessFocused,
                   TextStyle::InputSuccessFocusedBlink,
                   TextStyle::InputSuccessPressed,
                   TextStyle::InputSuccessDisabled,
                   TextStyle::InputSuccessPassword,
                   TextStyle::InputSuccessPasswordHovered,
                   TextStyle::InputSuccessPasswordFocused,
                   TextStyle::InputSuccessPasswordFocusedBlink,
                   TextStyle::InputSuccessPasswordPressed,
                   TextStyle::InputSuccessPasswordDisabled,
                   TextStyle::InputWarning,
                   TextStyle::InputWarningHovered,
                   TextStyle::InputWarningFocused,
                   TextStyle::InputWarningFocusedBlink,
                   TextStyle::InputWarningPressed,
                   TextStyle::InputWarningDisabled,
                   TextStyle::InputWarningPassword,
                   TextStyle::InputWarningPasswordHovered,
                   TextStyle::InputWarningPasswordFocused,
                   TextStyle::InputWarningPasswordFocusedBlink,
                   TextStyle::InputWarningPasswordPressed,
                   TextStyle::InputWarningPasswordDisabled,
                   TextStyle::InputDanger,
                   TextStyle::InputDangerHovered,
                   TextStyle::InputDangerFocused,
                   TextStyle::InputDangerFocusedBlink,
                   TextStyle::InputDangerPressed,
                   TextStyle::InputDangerDisabled,
                   TextStyle::InputDangerPassword,
                   TextStyle::InputDangerPasswordHovered,
                   TextStyle::InputDangerPasswordFocused,
                   TextStyle::InputDangerPasswordFocusedBlink,
                   TextStyle::InputDangerPasswordPressed,
                   TextStyle::InputDangerPasswordDisabled,
                   TextStyle::InputFlat,
                   TextStyle::InputFlatHovered,
                   TextStyle::InputFlatFocused,
                   TextStyle::InputFlatFocusedBlink,
                   TextStyle::InputFlatPressed,
                   TextStyle::InputFlatDisabled,
                   TextStyle::InputFlatPassword,
                   TextStyle::InputFlatPasswordHovered,
                   TextStyle::InputFlatPasswordFocused,
                   TextStyle::InputFlatPasswordFocusedBlink,
                   TextStyle::InputFlatPasswordPressed,
                   TextStyle::InputFlatPasswordDisabled})
        textStyles[Int(i)].alignment = Text::Alignment::MiddleLeft;

    /* Input padding. Pressed shift by one unit down, like buttons. */
    for(auto&& i: {TextStyle::InputDefault,
                   TextStyle::InputDefaultHovered,
                   TextStyle::InputDefaultFocused,
                   TextStyle::InputDefaultFocusedBlink,
                   TextStyle::InputDefaultDisabled,
                   TextStyle::InputDefaultPassword,
                   TextStyle::InputDefaultPasswordHovered,
                   TextStyle::InputDefaultPasswordFocused,
                   TextStyle::InputDefaultPasswordFocusedBlink,
                   TextStyle::InputDefaultPasswordDisabled,
                   TextStyle::InputSuccess,
                   TextStyle::InputSuccessHovered,
                   TextStyle::InputSuccessFocused,
                   TextStyle::InputSuccessFocusedBlink,
                   TextStyle::InputSuccessDisabled,
                   TextStyle::InputSuccessPassword,
                   TextStyle::InputSuccessPasswordHovered,
                   TextStyle::InputSuccessPasswordFocused,
                   TextStyle::InputSuccessPasswordFocusedBlink,
                   TextStyle::InputSuccessPasswordDisabled,
                   TextStyle::InputWarning,
                   TextStyle::InputWarningHovered,
                   TextStyle::InputWarningFocused,
                   TextStyle::InputWarningFocusedBlink,
                   TextStyle::InputWarningDisabled,
                   TextStyle::InputWarningPassword,
                   TextStyle::InputWarningPasswordHovered,
                   TextStyle::InputWarningPasswordFocused,
                   TextStyle::InputWarningPasswordFocusedBlink,
                   TextStyle::InputWarningPasswordDisabled,
                   TextStyle::InputDanger,
                   TextStyle::InputDangerHovered,
                   TextStyle::InputDangerFocused,
                   TextStyle::InputDangerFocusedBlink,
                   TextStyle::InputDangerDisabled,
                   TextStyle::InputDangerPassword,
                   TextStyle::InputDangerPasswordHovered,
                   TextStyle::InputDangerPasswordFocused,
                   TextStyle::InputDangerPasswordFocusedBlink,
                   TextStyle::InputDangerPasswordDisabled,
                   TextStyle::InputFlat,
                   TextStyle::InputFlatHovered,
                   TextStyle::InputFlatFocused,
                   TextStyle::InputFlatFocusedBlink,
                   TextStyle::InputFlatDisabled,
                   TextStyle::InputFlatPassword,
                   TextStyle::InputFlatPasswordHovered,
                   TextStyle::InputFlatPasswordFocused,
                   TextStyle::InputFlatPasswordFocusedBlink,
                   TextStyle::InputFlatPasswordDisabled})
        textStyles[Int(i)].padding = {10.0f, 4.0f,  6.0f, 4.0f};
    for(auto&& i: {TextStyle::InputDefaultPressed,
                   TextStyle::InputDefaultPasswordPressed,
                   TextStyle::InputSuccessPressed,
                   TextStyle::InputSuccessPasswordPressed,
                   TextStyle::InputWarningPressed,
                   TextStyle::InputWarningPasswordPressed,
                   TextStyle::InputDangerPressed,
                   TextStyle::InputDangerPasswordPressed,
                   TextStyle::InputFlatPressed,
                   TextStyle::InputFlatPasswordPressed})
        textStyles[Int(i)].padding = {10.0f, 5.0f,  6.0f, 3.0f};

    /* Input spans the same height as a button */
    layoutStyles[Int(LayoutStyle::Input)].minSize = {0.0f, 36.0f};
    layoutStyles[Int(LayoutStyle::Input)].margin = leafWidgetMargin;

    /* Scroll area --------------------------------------------------------- */

    /* Scrollbars are overlaid on top of the contents, narrow and mostly
       transparent when inactive and fatter and opaque when interacted with.
       Thumb colors are the same as default button's, scrollbar background is
       transparent in all cases. */
    /** @todo once edge/corner swizzle is a thing, there could be just one
        style for both X and Y */
    for(auto&& i: {BaseStyle::ScrollbarX,
                   BaseStyle::ScrollbarY,
                   BaseStyle::ScrollbarXHovered,
                   BaseStyle::ScrollbarYHovered,
                   BaseStyle::ScrollbarXPressed,
                   BaseStyle::ScrollbarYPressed,
                   BaseStyle::ScrollbarXDisabled,
                   BaseStyle::ScrollbarYDisabled})
        baseUniforms[Int(i)]
            .setColor(0x00000000_rgbaf)
            .setOutlineColor(0x00000000_rgbaf);
    for(auto&& i: {BaseStyle::ScrollbarThumbX,
                   BaseStyle::ScrollbarThumbY,
                   BaseStyle::ScrollbarThumbXHovered,
                   BaseStyle::ScrollbarThumbYHovered,
                   BaseStyle::ScrollbarThumbXPressed,
                   BaseStyle::ScrollbarThumbYPressed,
                   BaseStyle::ScrollbarThumbXDisabled,
                   BaseStyle::ScrollbarThumbYDisabled})
        baseUniforms[Int(i)]
            .setOutlineColor(0x00000000_rgbaf);
    for(auto&& i: {BaseStyle::ScrollbarThumbX,
                   BaseStyle::ScrollbarThumbY})
        baseUniforms[Int(i)]
            .setColor({0xdcdcdc_rgbf, 0.3f});
    for(auto&& i: {BaseStyle::ScrollbarThumbXDisabled,
                   BaseStyle::ScrollbarThumbYDisabled})
        baseUniforms[Int(i)]
            .setColor({0xdcdcdc_rgbf, 0.1f});
    for(auto&& i: {BaseStyle::ScrollbarThumbXHovered,
                   BaseStyle::ScrollbarThumbYHovered,
                   BaseStyle::ScrollbarThumbXPressed,
                   BaseStyle::ScrollbarThumbYPressed})
        baseUniforms[Int(i)]
            .setColor({0xa5c9ea_rgbf, 0.8f});
    for(auto&& i: {BaseStyle::ScrollbarThumbX,
                   BaseStyle::ScrollbarThumbXDisabled})
        baseUniforms[Int(i)]
            .setOutlineWidth({2.0f, 10.0f, 2.0f, 2.0f})
            .setInnerOutlineCornerRadius(2.0f);
    for(auto&& i: {BaseStyle::ScrollbarThumbY,
                   BaseStyle::ScrollbarThumbYDisabled})
        baseUniforms[Int(i)]
            .setOutlineWidth({10.0f, 2.0f, 2.0f, 2.0f})
            .setInnerOutlineCornerRadius(2.0f);
    baseUniforms[Int(BaseStyle::ScrollbarThumbXHovered)]
        .setOutlineWidth({2.0f, 6.0f, 2.0f, 2.0f})
        .setInnerOutlineCornerRadius(4.0f);
    baseUniforms[Int(BaseStyle::ScrollbarThumbYHovered)]
        .setOutlineWidth({6.0f, 2.0f, 2.0f, 2.0f})
        .setInnerOutlineCornerRadius(4.0f);
    baseUniforms[Int(BaseStyle::ScrollbarThumbXPressed)]
        .setOutlineWidth({1.0f, 5.0f, 1.0f, 1.0f})
        .setInnerOutlineCornerRadius(5.0f);
    baseUniforms[Int(BaseStyle::ScrollbarThumbYPressed)]
        .setOutlineWidth({5.0f, 1.0f, 1.0f, 1.0f})
        .setInnerOutlineCornerRadius(5.0f);

    /* The bottom-side margin for ScrollbarX and right-side margin for
       ScrollbarY is picked in a way that doesn't make the two thumbs
       (visually) overlap. It causes the active scrollbar area to overlap
       though, but as that's just a small square it shouldn't cause any
       practical issues. */
    /** @todo even just the 6 unit gap looks kinda ugly when sizes are tiny,
        any idea how to improve that? with less than 6 the event handling
        overlaps too much that even the test fails to pick the correct node to
        highlight */
    for(auto&& i: {LayoutStyle::ScrollbarX,
                   LayoutStyle::ScrollbarOnlyX})
        layoutStyles[Int(i)].minSize = {32.0f, 16.0f};
    layoutStyles[Int(LayoutStyle::ScrollbarX)].margin.z() = 6.0f;
    layoutStyles[Int(LayoutStyle::ScrollbarThumbX)].minSize = {21.0f, 16.0f};
    for(auto&& i: {LayoutStyle::ScrollbarY,
                   LayoutStyle::ScrollbarOnlyY})
        layoutStyles[Int(i)].minSize = {16.0f, 32.0f};
    layoutStyles[Int(LayoutStyle::ScrollbarY)].margin.w() = 6.0f;
    layoutStyles[Int(LayoutStyle::ScrollbarThumbY)].minSize = {16.0f, 21.0f};

    /* Panel --------------------------------------------------------------- */

    /* --header-background-color, with some transparency, disabled variant the
       same as elsewhere with 30% opacity */
    baseUniforms[Int(BaseStyle::PanelBackground)]
        .setColor({0x22272e_rgbf, 0.9f})
        .setCornerRadius(cornerRadius);
    baseUniforms[Int(BaseStyle::PanelBackgroundDisabled)]
        .setColor({0x22272e_rgbf, 0.3f})
        .setCornerRadius(cornerRadius);

    layoutStyles[Int(LayoutStyle::Panel)].padding = contentWidgetPadding;
    layoutStyles[Int(LayoutStyle::Panel)].margin = leafWidgetMargin;
}

/* The following is a very rudimentary enum-name-to-string conversion code.
   Based off https://blog.rink.nu/2023/02/12/behind-the-magic-of-magic_enum/ ,
   it only does the bare essentials to work with the contiguous 0-to-Count
   BaseStyle, TextStyle and other enums. The main downside is that using this
   code results in *massive* __PRETTY_FUNCTION__ strings being embedded in the
   binary, and because I want the code to work with plain C++11, there's no
   way around that. Thus this is unlikely to ever become a reusable Corrade
   utility or something. */
template<class E, E value> static const char* enumNameFor() {
    /* In GCC < 9 the __PRETTY_FUNCTION__ format is only
        static const char* nameFor() [with Name::Space::Enum value = (Name::Space::Enum)37]
       without ever listing the enum name. Discovered completely by accident,
       couldn't find anything in GCC 9 changelog about this. The only relevant issue I found is https://github.com/Neargye/magic_enum/issues/222 */
    #if defined(CORRADE_TARGET_GCC) && !defined(CORRADE_TARGET_CLANG) && __GNUC__ < 9
    #error __PRETTY_FUNCTION__ in GCC < 9 does not expose enum names, sorry
    #endif

    #if defined(CORRADE_TARGET_GCC) || defined(CORRADE_TARGET_CLANG)
    /* static const char* nameFor() [with Name::Space::Enum value = Name::Space::Enum::Value],
       clang has just static const char* nameFor() [value = Name::Space::Enum::Value] */
    return __PRETTY_FUNCTION__;
    #elif defined(CORRADE_TARGET_MSVC)
    /* const char *__cdecl EnumNames::nameFor<Name::Space::Enum::Value>(void) */
    return __FUNCSIG__;
    #else
    #error what compiler is this?
    #endif
}

template<std::size_t size, class E> class EnumNames {
    public:
        /* The expectedEnumPrefix is for a sanity check that the code actually
           found what's expected, as I definitely don't want to write tests
           for this code. */
        explicit EnumNames(Containers::StringView expectedEnumPrefix): EnumNames{typename Containers::Implementation::GenerateSequence<size>::Type{}} {
            for(Containers::StringView& i: _names) {
                /* Assume the actual enum name is after the last colon in the
                   string, which should be the case for GCC, Clang and MSVC */
                Containers::StringView found = i.findLastOr(':', i.end());
                /* There should be the expected enum name right before */
                CORRADE_ASSERT(i.prefix(found.end()).hasSuffix(expectedEnumPrefix),
                    i.prefix(found.end()) << "doesn't end with" << expectedEnumPrefix, );
                i = i.suffix(found.end());
                /* Slice to just the name, i.e. until the ] on GCC and Clang,
                   and until > on MSVC */
                #if defined(CORRADE_TARGET_GCC) || defined(CORRADE_TARGET_CLANG)
                i = i.prefix(i.find(']').begin());
                #elif defined(CORRADE_TARGET_MSVC)
                i = i.prefix(i.find('>').begin());
                #else
                #error what compiler is this?
                #endif
            }
        }

        Containers::StringView operator[](std::size_t value) const {
            CORRADE_ASSERT(value < size,
                value << "out of bounds for" << size << "names", {});
            return _names[value];
        }

    private:
        template<std::size_t ...sequence> explicit EnumNames(Containers::Implementation::Sequence<sequence...>): _names{enumNameFor<E, E(sequence)>()...} {}

    Containers::StringView _names[size];
};

}

int main(int argc, char** argv) {
    Utility::Arguments args;
    args.addArgument("output")
            .setHelp("output", "Path where to write the output")
        .parse(argc, argv);

    /** @todo uniform deduplication, now that it's all automated */
    /** @todo ability to poison the colors etc. with NaNs to catch cases where
        they're accidentally not filled */
    Ui::BaseLayerStyleUniform baseUniforms[Int(BaseStyle::Count)];
    Ui::TextLayerStyleUniform textUniforms[Int(TextStyle::Count) + Int(TextEditingStyle::Count)];
    TextLayerStyle textStyles[Int(TextStyle::Count)];
    Ui::TextLayerEditingStyleUniform textEditingUniforms[Int(TextEditingStyle::Count)];
    Vector4 textEditingPaddings[Int(TextEditingStyle::Count)];
    LayoutLayerStyle layoutStyles[Int(LayoutStyle::Count)];

    dark(
        baseUniforms,
        Containers::arrayView(textUniforms).prefix(Int(TextStyle::Count)),
        textStyles,
        textEditingUniforms,
        textEditingPaddings,
        Containers::arrayView(textUniforms).exceptPrefix(Int(TextStyle::Count)),
        layoutStyles);
    constexpr Containers::StringView filePrefix = "themeDark"_s;

    /* Preamble and postamble common to all files */
    constexpr Containers::StringView preamble = R"(/*
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

/* This file is generated using GenerateTheme.cpp, don't edit directly */

)"_s;

    const auto printColor = [](const Color4& color) {
        if(color == Color4{})
            return Containers::String{"{}"};
        if(color.a() == 1.0f)
            return Utility::format("0x{:.6x}_rgbf",
                color.rgb().toLinearRgbInt());
        return Utility::format("0x{:.6x}ff_rgbaf*{:.1f}f",
            color.rgb().toLinearRgbInt(),
            color.a());
    };

    /* Base layer uniforms */
    {
        Containers::Array<char> output;
        arrayAppend(output, preamble);

        arrayAppend(output,
            "/* BaseStyle enum name, BaseLayerStyleUniform constructor arguments */\n"
            "#ifdef _c\n"_s);

        EnumNames<Int(BaseStyle::Count), BaseStyle> enumNames{"BaseStyle::"};

        for(std::size_t i = 0; i != Containers::arraySize(baseUniforms); ++i) {
            const Ui::BaseLayerStyleUniform& uniform = baseUniforms[i];
            const Containers::StringView name = enumNames[i];

            /* Assuming there are no gradients so far */
            CORRADE_INTERNAL_ASSERT(uniform.topColor == uniform.bottomColor);

            /* Default background color, default radius, no outline */
            /** @todo implement comparison operators on the uniforms already */
            if(uniform.topColor == Ui::BaseLayerStyleUniform{}.topColor &&
               uniform.cornerRadius == Ui::BaseLayerStyleUniform{}.cornerRadius &&
               uniform.outlineWidth == Ui::BaseLayerStyleUniform{}.outlineWidth) {
                arrayAppend(output, Utility::format(
                    "_c({}, )\n",
                    name));

            /* No outline */
            } else if(uniform.outlineWidth == Vector4{}) {
                /* All corners the same */
                if(uniform.cornerRadius == Vector4{uniform.cornerRadius.x()})
                    arrayAppend(output, Utility::format(
                        "_c({}, {}, {:.1f}f)\n",
                        name,
                        printColor(uniform.topColor),
                        uniform.cornerRadius.x()));
                /* No other variants right now */
                else CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */

            /* Outline */
            } else {
                /* All edges and corners the same */
                if(uniform.outlineWidth == Vector4{uniform.outlineWidth.x()} &&
                   uniform.cornerRadius == Vector4{uniform.cornerRadius.x()} &&
                   uniform.innerOutlineCornerRadius == Vector4{uniform.innerOutlineCornerRadius.x()})
                    arrayAppend(output, Utility::format(
                        "_c({}, {}, {},\n"
                        "    {:.1f}f, {:.1f}f, {:.1f}f)\n",
                        name,
                        printColor(uniform.topColor),
                        printColor(uniform.outlineColor),
                        uniform.outlineWidth.x(),
                        uniform.cornerRadius.x(),
                        uniform.innerOutlineCornerRadius.x()));
                /* Each edge / corner different */
                else
                    arrayAppend(output, Utility::format(
                        "_c({}, {}, {},\n"
                           "    {{{:.1f}f, {:.1f}f, {:.1f}f, {:.1f}f}}, "
                           "{{{:.1f}f, {:.1f}f, {:.1f}f, {:.1f}f}}, "
                           "{{{:.1f}f, {:.1f}f, {:.1f}f, {:.1f}f}})\n",
                        name,
                        printColor(uniform.topColor),
                        printColor(uniform.outlineColor),
                        uniform.outlineWidth.x(),
                        uniform.outlineWidth.y(),
                        uniform.outlineWidth.z(),
                        uniform.outlineWidth.w(),
                        uniform.cornerRadius.x(),
                        uniform.cornerRadius.y(),
                        uniform.cornerRadius.z(),
                        uniform.cornerRadius.w(),
                        uniform.innerOutlineCornerRadius.x(),
                        uniform.innerOutlineCornerRadius.y(),
                        uniform.innerOutlineCornerRadius.z(),
                        uniform.innerOutlineCornerRadius.w()));
            }
        }

        arrayAppend(output, "#endif\n"_s);

        const Containers::String filename = Utility::Path::join({*Utility::Path::currentDirectory(), args.value<Containers::StringView>("output"), filePrefix + "BaseStyleUniforms.h"});
        if(!Utility::Path::write(filename, output))
            return 1; /* LCOV_EXCL_LINE */
        Debug{} << "Wrote" << filename;

    /* Text layer uniforms */
    } {
        Containers::Array<char> output;
        arrayAppend(output, preamble);

        arrayAppend(output,
            "/* TextLayerStyleUniform constructor arguments */\n"
            "#ifdef _c\n"_s);

        EnumNames<Int(TextStyle::Count), TextStyle> enumNames{"TextStyle::"};
        EnumNames<Int(TextEditingStyle::Count), TextEditingStyle> editingEnumNames{"TextEditingStyle::"};

        for(std::size_t i = 0; i != Containers::arraySize(textUniforms); ++i) {
            if(i == std::size_t(TextStyle::Count))
                arrayAppend(output, "\n/* Editing styles */\n"_s);

            const Ui::TextLayerStyleUniform& uniform = textUniforms[i];
            const Containers::StringView name = i < std::size_t(TextStyle::Count) ?
                enumNames[i] : editingEnumNames[i - std::size_t(TextStyle::Count)];

            /* Assuming there are no distance field styles */
            CORRADE_INTERNAL_ASSERT(uniform.outlineWidth == 0.0f && uniform.edgeOffset == 0.0f && uniform.smoothness == 0.0f);

            /* Default color */
            /** @todo implement comparison operators on the uniforms already */
            if(uniform.color == Ui::TextLayerStyleUniform{}.color)
                arrayAppend(output, Utility::format(
                    "_c() /* {} */\n",
                    name));

            /* Color */
             else
                arrayAppend(output, Utility::format(
                    "_c({}) /* {} */\n",
                    printColor(uniform.color),
                    name));
        }

        arrayAppend(output, "#endif\n"_s);

        const Containers::String filename = Utility::Path::join({*Utility::Path::currentDirectory(), args.value<Containers::StringView>("output"), filePrefix + "TextStyleUniforms.h"});
        if(!Utility::Path::write(filename, output))
            return 1; /* LCOV_EXCL_LINE */
        Debug{} << "Wrote" << filename;

    /* Text layer styles */
    } {
        Containers::Array<char> output;
        arrayAppend(output, preamble);
        arrayAppend(output,
            "/* TextStyle enum name, TextFont enum name, alignment, padding */\n"
            "#ifdef _c\n"_s);

        EnumNames<Int(TextStyle::Count), TextStyle> enumNames{"TextStyle::"};
        EnumNames<Int(TextEditingStyle::Count), TextEditingStyle> editingEnumNames{"TextEditingStyle::"};
        EnumNames<Int(TextFont::Count), TextFont> fontNames{"TextFont::"};
        EnumNames<Int(TextFont::Count), TextFont> alignmentNames{"TextFont::"};

        for(std::size_t i = 0; i != Containers::arraySize(textStyles); ++i) {
            const TextLayerStyle& style = textStyles[i];
            const Containers::StringView name = enumNames[i];
            const Containers::StringView fontName = fontNames[Int(style.font)];

            Containers::String alignmentName;
            Debug{&alignmentName, Debug::Flag::NoNewlineAtTheEnd} << style.alignment;

            if(style.cursorStyle == TextEditingStyle::None &&
               style.selectionStyle == TextEditingStyle::None)
                arrayAppend(output, Utility::format(
                    style.padding == Vector4{} ?
                        "_c({}, {}, {}, {{}})\n" :
                        "_c({}, {}, {}, {{{:.1f}f, {:.1f}f, {:.1f}f, {:.1f}f}})\n",
                    name,
                    fontName,
                    alignmentName.exceptPrefix("Text::Alignment::"),
                    style.padding.x(),
                    style.padding.y(),
                    style.padding.z(),
                    style.padding.w()));
            else
                arrayAppend(output, Utility::format(
                    style.padding == Vector4{} ?
                        "_c({}, {}, {}, {{}}, {8}, {9})\n" :
                        "_c({}, {}, {}, {{{:.1f}f, {:.1f}f, {:.1f}f, {:.1f}f}},\n"
                        "    {}{}, {}{})\n",
                    name,
                    fontName,
                    alignmentName.exceptPrefix("Text::Alignment::"),
                    style.padding.x(),
                    style.padding.y(),
                    style.padding.z(),
                    style.padding.w(),
                    Int(style.cursorStyle),
                    style.cursorStyle == TextEditingStyle::None ?
                        "" : Utility::format(" /* {} */", editingEnumNames[Int(style.cursorStyle)]),
                    Int(style.selectionStyle),
                    style.selectionStyle == TextEditingStyle::None ?
                        "" : Utility::format(" /* {} */", editingEnumNames[Int(style.selectionStyle)])));
        }

        /* Uniform mapping, currently trivial. Separated from the above to not
           cause massive diffs every time a new style is added or the mapping
           changes. */
        arrayAppend(output,Utility::format(
            "#endif\n"
            "/* Style -> uniform ID in {}TextStyleUniforms.h */\n"
            "#ifdef _u\n_u(", filePrefix));
        std::size_t lineBegin = output.size() - 3;
        for(std::size_t i = 0; i != Containers::arraySize(textStyles); ++i) {
            /* If the next number (<999) together with a comma is beyond 79
               chars, wrap */
            if(output.size() + 4 - lineBegin > 79) {
                /* Remove the last space, replace with a newline & indent */
                arrayRemoveSuffix(output, 1);
                arrayAppend(output, "\n   "_s);
                lineBegin = output.size() - 3;
            }

            arrayAppend(output, Utility::format("{}, ", i));
        }
        /* Remove the last ", " */
        arrayRemoveSuffix(output, 2);
        arrayAppend(output, ")\n#endif\n"_s);

        const Containers::String filename = Utility::Path::join({*Utility::Path::currentDirectory(), args.value<Containers::StringView>("output"), filePrefix + "TextStyles.h"});
        if(!Utility::Path::write(filename, output))
            return 1; /* LCOV_EXCL_LINE */
        Debug{} << "Wrote" << filename;

    /* Text layer editing styles */
    } {
        Containers::Array<char> output;
        arrayAppend(output, preamble);
        arrayAppend(output,
            "/* Padding, TextLayerEditingStyleUniform constructor arguments */\n"
            "#ifdef _c\n"_s);

        EnumNames<Int(TextEditingStyle::Count), TextEditingStyle> enumNames{"TextEditingStyle::"};

        for(std::size_t i = 0; i != Containers::arraySize(textEditingUniforms); ++i) {
            const Ui::TextLayerEditingStyleUniform& uniform = textEditingUniforms[i];
            const Vector4& padding = textEditingPaddings[i];
            const Containers::StringView name = enumNames[i];

            arrayAppend(output, Utility::format(
                "_c({{{:.1f}f, {:.1f}f, {:.1f}f, {:.1f}f}}, {}, {:.1f}f) /* {} */\n",
                padding.x(),
                padding.y(),
                padding.z(),
                padding.w(),
                printColor(uniform.backgroundColor),
                uniform.cornerRadius,
                name));
        }

        /* Mapping to selection uniforms. Separated from the above to not cause
           massive diffs every time a new text style is added or the mapping
           changes. */
        arrayAppend(output, Utility::format(
            "#endif\n"
            "/* Style -> selection uniform ID in {}TextStyleUniforms.h */\n"
            "#ifdef _u\n_u(", filePrefix));
        std::size_t lineBegin = output.size() - 3;
        for(std::size_t i = 0; i != Containers::arraySize(textEditingUniforms); ++i) {
            /* If the next number (<999) together with a comma is beyond 79
               chars, wrap */
            if(output.size() + 4 - lineBegin > 79) {
                /* Remove the last space, replace with a newline & indent */
                arrayRemoveSuffix(output, 1);
                arrayAppend(output, "\n   "_s);
                lineBegin = output.size() - 3;
            }

            /* The selection uniforms are after all other text style
               uniforms */
            arrayAppend(output, Utility::format("{}, ", Int(TextStyle::Count) + i));
        }
        /* Remove the last ", " */
        arrayRemoveSuffix(output, 2);
        arrayAppend(output, ")\n#endif\n"_s);

        const Containers::String filename = Utility::Path::join({*Utility::Path::currentDirectory(), args.value<Containers::StringView>("output"), filePrefix + "TextEditingStyles.h"});
        if(!Utility::Path::write(filename, output))
            return 1; /* LCOV_EXCL_LINE */
        Debug{} << "Wrote" << filename;

    /* Layout layer styles */
    } {
        Containers::Array<char> output;
        arrayAppend(output, preamble);
        arrayAppend(output,
            "/* LayoutStyle enum name, min size[, padding, margin] */\n"
            "#ifdef _c\n"_s);

        EnumNames<Int(LayoutStyle::Count), LayoutStyle> enumNames{"LayoutStyle::"};

        for(std::size_t i = 0; i != Containers::arraySize(layoutStyles); ++i) {
            const LayoutLayerStyle& style = layoutStyles[i];
            const Containers::StringView name = enumNames[i];

            /* No padding or margin */
            if(style.padding == Vector4{} && style.margin == Vector4{}) {
                if(style.minSize == Vector2{})
                    arrayAppend(output, Utility::format(
                        "_c({}, {{}})\n",
                        name));
                else
                    arrayAppend(output, Utility::format(
                        "_c({}, {{{:.1f}f, {:.1f}f}})\n",
                        name,
                        style.minSize.x(),
                        style.minSize.y()));

            /* Horizontal and vertical padding & margin */
            } else if(Math::gather<0, 1>(style.padding) == Math::gather<2, 3>(style.padding) &&
                      Math::gather<0, 1>(style.margin) == Math::gather<2, 3>(style.margin)) {
                /* No min size, both padding and margin (i.e., min size
                   defined by the padding) */
                if(style.minSize == Vector2{} &&
                   style.padding != Vector4{} &&
                   style.margin != Vector4{})
                    arrayAppend(output, Utility::format(
                        "_c({}, {{}}, {{{:.1f}f, {:.1f}f}}, {{{:.1f}f, {:.1f}f}})\n",
                        name,
                        style.padding.x(),
                        style.padding.y(),
                        style.margin.x(),
                        style.margin.y()));
                /* No padding, just min size and margin (i.e., leaf widget) */
                else if(style.minSize != Vector2{} &&
                        style.padding == Vector4{} &&
                        style.margin != Vector4{})
                    arrayAppend(output, Utility::format(
                        "_c({}, {{{:.1f}f, {:.1f}f}}, {{}}, {{{:.1f}f, {:.1f}f}})\n",
                        name,
                        style.minSize.x(),
                        style.minSize.y(),
                        style.margin.x(),
                        style.margin.y()));
                /* No other variants right now */
                else CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */

            /* Padding / margin on all four sides */
            } else {
                /* No padding, just min size and margin (i.e., leaf widget) */
                if(style.minSize != Vector2{} &&
                   style.padding == Vector4{} &&
                   style.margin != Vector4{})
                    arrayAppend(output, Utility::format(
                        "_c({}, {{{:.1f}f, {:.1f}f}}, {{}}, {{{:.1f}f, {:.1f}f, {:.1f}f, {:.1f}f}})\n",
                        name,
                        style.minSize.x(),
                        style.minSize.y(),
                        style.margin.x(),
                        style.margin.y(),
                        style.margin.z(),
                        style.margin.w()));
                /* No other variants right now */
                else CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
            }
        }

        arrayAppend(output, "#endif\n"_s);

        const Containers::String filename = Utility::Path::join({*Utility::Path::currentDirectory(), args.value<Containers::StringView>("output"), filePrefix + "LayoutStyles.h"});
        if(!Utility::Path::write(filename, output))
            return 1; /* LCOV_EXCL_LINE */
        Debug{} << "Wrote" << filename;
    }
}
