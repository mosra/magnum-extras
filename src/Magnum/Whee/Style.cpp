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

#include "Style.h"
#include "Style.hpp"

#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Utility/Algorithms.h>
#include <Corrade/Utility/Resource.h>
#include <Magnum/ImageView.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Range.h>
#include <Magnum/Text/AbstractFont.h>
#include <Magnum/Text/AbstractGlyphCache.h>
#include <Magnum/TextureTools/Atlas.h>
#include <Magnum/Trade/AbstractImporter.h>
#include <Magnum/Trade/ImageData.h>

#include "Magnum/Whee/BaseLayer.h"
#include "Magnum/Whee/EventLayer.h"
#include "Magnum/Whee/TextLayer.h"
#include "Magnum/Whee/UserInterface.h"

#ifdef MAGNUM_BUILD_STATIC
static void importShaderResources() {
    CORRADE_RESOURCE_INITIALIZE(MagnumWhee_RESOURCES)
}
#endif

namespace Magnum { namespace Whee {

Debug& operator<<(Debug& debug, const Icon value) {
    debug << "Whee::Icon" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case Icon::value: return debug << "::" #value;
        _c(None)
        _c(Yes)
        _c(No)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "(" << Debug::nospace << Debug::hex << UnsignedInt(value) << Debug::nospace << ")";
}

using namespace Containers::Literals;
using namespace Math::Literals;
using Implementation::BaseStyle;
using Implementation::TextStyle;
using Implementation::TextStyleUniform;

namespace {

BaseStyle styleTransitionToInactiveOut(const BaseStyle index) {
    switch(index) {
        case BaseStyle::ButtonDefaultInactiveOut:
        case BaseStyle::ButtonDefaultInactiveOver:
        case BaseStyle::ButtonDefaultPressedOut:
        case BaseStyle::ButtonDefaultPressedOver:
            return BaseStyle::ButtonDefaultInactiveOut;
        case BaseStyle::ButtonPrimaryInactiveOut:
        case BaseStyle::ButtonPrimaryInactiveOver:
        case BaseStyle::ButtonPrimaryPressedOut:
        case BaseStyle::ButtonPrimaryPressedOver:
            return BaseStyle::ButtonPrimaryInactiveOut;
        case BaseStyle::ButtonSuccessInactiveOut:
        case BaseStyle::ButtonSuccessInactiveOver:
        case BaseStyle::ButtonSuccessPressedOut:
        case BaseStyle::ButtonSuccessPressedOver:
            return BaseStyle::ButtonSuccessInactiveOut;
        case BaseStyle::ButtonWarningInactiveOut:
        case BaseStyle::ButtonWarningInactiveOver:
        case BaseStyle::ButtonWarningPressedOut:
        case BaseStyle::ButtonWarningPressedOver:
            return BaseStyle::ButtonWarningInactiveOut;
        case BaseStyle::ButtonDangerInactiveOut:
        case BaseStyle::ButtonDangerInactiveOver:
        case BaseStyle::ButtonDangerPressedOut:
        case BaseStyle::ButtonDangerPressedOver:
            return BaseStyle::ButtonDangerInactiveOut;
        case BaseStyle::ButtonInfoInactiveOut:
        case BaseStyle::ButtonInfoInactiveOver:
        case BaseStyle::ButtonInfoPressedOut:
        case BaseStyle::ButtonInfoPressedOver:
            return BaseStyle::ButtonInfoInactiveOut;
        case BaseStyle::ButtonDimInactiveOut:
        case BaseStyle::ButtonDimInactiveOver:
        case BaseStyle::ButtonDimPressedOut:
        case BaseStyle::ButtonDimPressedOver:
            return BaseStyle::ButtonDimInactiveOut;
        case BaseStyle::ButtonFlatInactiveOut:
        case BaseStyle::ButtonFlatInactiveOver:
        case BaseStyle::ButtonFlatPressedOut:
        case BaseStyle::ButtonFlatPressedOver:
            return BaseStyle::ButtonFlatInactiveOut;
        /* LCOV_EXCL_START */
        case BaseStyle::ButtonDefaultDisabled:
        case BaseStyle::ButtonPrimaryDisabled:
        case BaseStyle::ButtonSuccessDisabled:
        case BaseStyle::ButtonWarningDisabled:
        case BaseStyle::ButtonDangerDisabled:
        case BaseStyle::ButtonInfoDisabled:
        case BaseStyle::ButtonDimDisabled:
        case BaseStyle::ButtonFlatDisabled:
            CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        /* LCOV_EXCL_STOP */
    }

    CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
}

BaseStyle styleTransitionToInactiveOver(const BaseStyle index) {
    switch(index) {
        case BaseStyle::ButtonDefaultInactiveOut:
        case BaseStyle::ButtonDefaultInactiveOver:
        case BaseStyle::ButtonDefaultPressedOut:
        case BaseStyle::ButtonDefaultPressedOver:
            return BaseStyle::ButtonDefaultInactiveOver;
        case BaseStyle::ButtonPrimaryInactiveOut:
        case BaseStyle::ButtonPrimaryInactiveOver:
        case BaseStyle::ButtonPrimaryPressedOut:
        case BaseStyle::ButtonPrimaryPressedOver:
            return BaseStyle::ButtonPrimaryInactiveOver;
        case BaseStyle::ButtonSuccessInactiveOut:
        case BaseStyle::ButtonSuccessInactiveOver:
        case BaseStyle::ButtonSuccessPressedOut:
        case BaseStyle::ButtonSuccessPressedOver:
            return BaseStyle::ButtonSuccessInactiveOver;
        case BaseStyle::ButtonWarningInactiveOut:
        case BaseStyle::ButtonWarningInactiveOver:
        case BaseStyle::ButtonWarningPressedOut:
        case BaseStyle::ButtonWarningPressedOver:
            return BaseStyle::ButtonWarningInactiveOver;
        case BaseStyle::ButtonDangerInactiveOut:
        case BaseStyle::ButtonDangerInactiveOver:
        case BaseStyle::ButtonDangerPressedOut:
        case BaseStyle::ButtonDangerPressedOver:
            return BaseStyle::ButtonDangerInactiveOver;
        case BaseStyle::ButtonInfoInactiveOut:
        case BaseStyle::ButtonInfoInactiveOver:
        case BaseStyle::ButtonInfoPressedOut:
        case BaseStyle::ButtonInfoPressedOver:
            return BaseStyle::ButtonInfoInactiveOver;
        case BaseStyle::ButtonDimInactiveOut:
        case BaseStyle::ButtonDimInactiveOver:
        case BaseStyle::ButtonDimPressedOut:
        case BaseStyle::ButtonDimPressedOver:
            return BaseStyle::ButtonDimInactiveOver;
        case BaseStyle::ButtonFlatInactiveOut:
        case BaseStyle::ButtonFlatInactiveOver:
        case BaseStyle::ButtonFlatPressedOut:
        case BaseStyle::ButtonFlatPressedOver:
            return BaseStyle::ButtonFlatInactiveOver;
        /* LCOV_EXCL_START */
        case BaseStyle::ButtonDefaultDisabled:
        case BaseStyle::ButtonPrimaryDisabled:
        case BaseStyle::ButtonSuccessDisabled:
        case BaseStyle::ButtonWarningDisabled:
        case BaseStyle::ButtonDangerDisabled:
        case BaseStyle::ButtonInfoDisabled:
        case BaseStyle::ButtonDimDisabled:
        case BaseStyle::ButtonFlatDisabled:
            CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        /* LCOV_EXCL_STOP */
    }

    CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
}

BaseStyle styleTransitionToPressedOut(const BaseStyle index) {
    switch(index) {
        case BaseStyle::ButtonDefaultInactiveOut:
        case BaseStyle::ButtonDefaultInactiveOver:
        case BaseStyle::ButtonDefaultPressedOut:
        case BaseStyle::ButtonDefaultPressedOver:
            return BaseStyle::ButtonDefaultPressedOut;
        case BaseStyle::ButtonPrimaryInactiveOut:
        case BaseStyle::ButtonPrimaryInactiveOver:
        case BaseStyle::ButtonPrimaryPressedOut:
        case BaseStyle::ButtonPrimaryPressedOver:
            return BaseStyle::ButtonPrimaryPressedOut;
        case BaseStyle::ButtonSuccessInactiveOut:
        case BaseStyle::ButtonSuccessInactiveOver:
        case BaseStyle::ButtonSuccessPressedOut:
        case BaseStyle::ButtonSuccessPressedOver:
            return BaseStyle::ButtonSuccessPressedOut;
        case BaseStyle::ButtonWarningInactiveOut:
        case BaseStyle::ButtonWarningInactiveOver:
        case BaseStyle::ButtonWarningPressedOut:
        case BaseStyle::ButtonWarningPressedOver:
            return BaseStyle::ButtonWarningPressedOut;
        case BaseStyle::ButtonDangerInactiveOut:
        case BaseStyle::ButtonDangerInactiveOver:
        case BaseStyle::ButtonDangerPressedOut:
        case BaseStyle::ButtonDangerPressedOver:
            return BaseStyle::ButtonDangerPressedOut;
        case BaseStyle::ButtonInfoInactiveOut:
        case BaseStyle::ButtonInfoInactiveOver:
        case BaseStyle::ButtonInfoPressedOut:
        case BaseStyle::ButtonInfoPressedOver:
            return BaseStyle::ButtonInfoPressedOut;
        case BaseStyle::ButtonDimInactiveOut:
        case BaseStyle::ButtonDimInactiveOver:
        case BaseStyle::ButtonDimPressedOut:
        case BaseStyle::ButtonDimPressedOver:
            return BaseStyle::ButtonDimPressedOut;
        case BaseStyle::ButtonFlatInactiveOut:
        case BaseStyle::ButtonFlatInactiveOver:
        case BaseStyle::ButtonFlatPressedOut:
        case BaseStyle::ButtonFlatPressedOver:
            return BaseStyle::ButtonFlatPressedOut;
        /* LCOV_EXCL_START */
        case BaseStyle::ButtonDefaultDisabled:
        case BaseStyle::ButtonPrimaryDisabled:
        case BaseStyle::ButtonSuccessDisabled:
        case BaseStyle::ButtonWarningDisabled:
        case BaseStyle::ButtonDangerDisabled:
        case BaseStyle::ButtonInfoDisabled:
        case BaseStyle::ButtonDimDisabled:
        case BaseStyle::ButtonFlatDisabled:
            CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        /* LCOV_EXCL_STOP */
    }

    CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
}

BaseStyle styleTransitionToPressedOver(const BaseStyle index) {
    switch(index) {
        case BaseStyle::ButtonDefaultInactiveOut:
        case BaseStyle::ButtonDefaultInactiveOver:
        case BaseStyle::ButtonDefaultPressedOut:
        case BaseStyle::ButtonDefaultPressedOver:
            return BaseStyle::ButtonDefaultPressedOver;
        case BaseStyle::ButtonPrimaryInactiveOut:
        case BaseStyle::ButtonPrimaryInactiveOver:
        case BaseStyle::ButtonPrimaryPressedOut:
        case BaseStyle::ButtonPrimaryPressedOver:
            return BaseStyle::ButtonPrimaryPressedOver;
        case BaseStyle::ButtonSuccessInactiveOut:
        case BaseStyle::ButtonSuccessInactiveOver:
        case BaseStyle::ButtonSuccessPressedOut:
        case BaseStyle::ButtonSuccessPressedOver:
            return BaseStyle::ButtonSuccessPressedOver;
        case BaseStyle::ButtonWarningInactiveOut:
        case BaseStyle::ButtonWarningInactiveOver:
        case BaseStyle::ButtonWarningPressedOut:
        case BaseStyle::ButtonWarningPressedOver:
            return BaseStyle::ButtonWarningPressedOver;
        case BaseStyle::ButtonDangerInactiveOut:
        case BaseStyle::ButtonDangerInactiveOver:
        case BaseStyle::ButtonDangerPressedOut:
        case BaseStyle::ButtonDangerPressedOver:
            return BaseStyle::ButtonDangerPressedOver;
        case BaseStyle::ButtonInfoInactiveOut:
        case BaseStyle::ButtonInfoInactiveOver:
        case BaseStyle::ButtonInfoPressedOut:
        case BaseStyle::ButtonInfoPressedOver:
            return BaseStyle::ButtonInfoPressedOver;
        case BaseStyle::ButtonDimInactiveOut:
        case BaseStyle::ButtonDimInactiveOver:
        case BaseStyle::ButtonDimPressedOut:
        case BaseStyle::ButtonDimPressedOver:
            return BaseStyle::ButtonDimPressedOver;
        case BaseStyle::ButtonFlatInactiveOut:
        case BaseStyle::ButtonFlatInactiveOver:
        case BaseStyle::ButtonFlatPressedOut:
        case BaseStyle::ButtonFlatPressedOver:
            return BaseStyle::ButtonFlatPressedOver;
        /* LCOV_EXCL_START */
        case BaseStyle::ButtonDefaultDisabled:
        case BaseStyle::ButtonPrimaryDisabled:
        case BaseStyle::ButtonSuccessDisabled:
        case BaseStyle::ButtonWarningDisabled:
        case BaseStyle::ButtonDangerDisabled:
        case BaseStyle::ButtonInfoDisabled:
        case BaseStyle::ButtonDimDisabled:
        case BaseStyle::ButtonFlatDisabled:
            CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        /* LCOV_EXCL_STOP */
    }

    CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
}

BaseStyle styleTransitionToDisabled(const BaseStyle index) {
    switch(index) {
        case BaseStyle::ButtonDefaultInactiveOut:
        case BaseStyle::ButtonDefaultInactiveOver:
        case BaseStyle::ButtonDefaultPressedOut:
        case BaseStyle::ButtonDefaultPressedOver:
            return BaseStyle::ButtonDefaultDisabled;
        case BaseStyle::ButtonPrimaryInactiveOut:
        case BaseStyle::ButtonPrimaryInactiveOver:
        case BaseStyle::ButtonPrimaryPressedOut:
        case BaseStyle::ButtonPrimaryPressedOver:
            return BaseStyle::ButtonPrimaryDisabled;
        case BaseStyle::ButtonSuccessInactiveOut:
        case BaseStyle::ButtonSuccessInactiveOver:
        case BaseStyle::ButtonSuccessPressedOut:
        case BaseStyle::ButtonSuccessPressedOver:
            return BaseStyle::ButtonSuccessDisabled;
        case BaseStyle::ButtonWarningInactiveOut:
        case BaseStyle::ButtonWarningInactiveOver:
        case BaseStyle::ButtonWarningPressedOut:
        case BaseStyle::ButtonWarningPressedOver:
            return BaseStyle::ButtonWarningDisabled;
        case BaseStyle::ButtonDangerInactiveOut:
        case BaseStyle::ButtonDangerInactiveOver:
        case BaseStyle::ButtonDangerPressedOut:
        case BaseStyle::ButtonDangerPressedOver:
            return BaseStyle::ButtonDangerDisabled;
        case BaseStyle::ButtonInfoInactiveOut:
        case BaseStyle::ButtonInfoInactiveOver:
        case BaseStyle::ButtonInfoPressedOut:
        case BaseStyle::ButtonInfoPressedOver:
            return BaseStyle::ButtonInfoDisabled;
        case BaseStyle::ButtonDimInactiveOut:
        case BaseStyle::ButtonDimInactiveOver:
        case BaseStyle::ButtonDimPressedOut:
        case BaseStyle::ButtonDimPressedOver:
            return BaseStyle::ButtonDimDisabled;
        case BaseStyle::ButtonFlatInactiveOut:
        case BaseStyle::ButtonFlatInactiveOver:
        case BaseStyle::ButtonFlatPressedOut:
        case BaseStyle::ButtonFlatPressedOver:
            return BaseStyle::ButtonFlatDisabled;
        /* LCOV_EXCL_START */
        case BaseStyle::ButtonDefaultDisabled:
        case BaseStyle::ButtonPrimaryDisabled:
        case BaseStyle::ButtonSuccessDisabled:
        case BaseStyle::ButtonWarningDisabled:
        case BaseStyle::ButtonDangerDisabled:
        case BaseStyle::ButtonInfoDisabled:
        case BaseStyle::ButtonDimDisabled:
        case BaseStyle::ButtonFlatDisabled:
            CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        /* LCOV_EXCL_STOP */
    }

    CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
}

TextStyle styleTransitionToInactiveOut(const TextStyle index) {
    switch(index) {
        case TextStyle::ButtonIconOnly:
        case TextStyle::ButtonPressedIconOnly:
            return TextStyle::ButtonIconOnly;
        case TextStyle::ButtonTextOnly:
        case TextStyle::ButtonPressedTextOnly:
            return TextStyle::ButtonTextOnly;
        case TextStyle::ButtonIcon:
        case TextStyle::ButtonPressedIcon:
            return TextStyle::ButtonIcon;
        case TextStyle::ButtonText:
        case TextStyle::ButtonPressedText:
            return TextStyle::ButtonText;
        case TextStyle::ButtonFlatInactiveOutIconOnly:
        case TextStyle::ButtonFlatInactiveOverIconOnly:
        case TextStyle::ButtonFlatPressedOutIconOnly:
        case TextStyle::ButtonFlatPressedOverIconOnly:
            return TextStyle::ButtonFlatInactiveOutIconOnly;
        case TextStyle::ButtonFlatInactiveOutTextOnly:
        case TextStyle::ButtonFlatInactiveOverTextOnly:
        case TextStyle::ButtonFlatPressedOutTextOnly:
        case TextStyle::ButtonFlatPressedOverTextOnly:
            return TextStyle::ButtonFlatInactiveOutTextOnly;
        case TextStyle::ButtonFlatInactiveOutIcon:
        case TextStyle::ButtonFlatInactiveOverIcon:
        case TextStyle::ButtonFlatPressedOutIcon:
        case TextStyle::ButtonFlatPressedOverIcon:
            return TextStyle::ButtonFlatInactiveOutIcon;
        case TextStyle::ButtonFlatInactiveOutText:
        case TextStyle::ButtonFlatInactiveOverText:
        case TextStyle::ButtonFlatPressedOutText:
        case TextStyle::ButtonFlatPressedOverText:
            return TextStyle::ButtonFlatInactiveOutText;
        case TextStyle::LabelDefaultIconOnly:
        case TextStyle::LabelDefaultTextOnly:
        case TextStyle::LabelDefaultIcon:
        case TextStyle::LabelDefaultText:
        case TextStyle::LabelPrimaryIconOnly:
        case TextStyle::LabelPrimaryTextOnly:
        case TextStyle::LabelPrimaryIcon:
        case TextStyle::LabelPrimaryText:
        case TextStyle::LabelSuccessIconOnly:
        case TextStyle::LabelSuccessTextOnly:
        case TextStyle::LabelSuccessIcon:
        case TextStyle::LabelSuccessText:
        case TextStyle::LabelWarningIconOnly:
        case TextStyle::LabelWarningTextOnly:
        case TextStyle::LabelWarningIcon:
        case TextStyle::LabelWarningText:
        case TextStyle::LabelDangerIconOnly:
        case TextStyle::LabelDangerTextOnly:
        case TextStyle::LabelDangerIcon:
        case TextStyle::LabelDangerText:
        case TextStyle::LabelInfoIconOnly:
        case TextStyle::LabelInfoTextOnly:
        case TextStyle::LabelInfoIcon:
        case TextStyle::LabelInfoText:
        case TextStyle::LabelDimIconOnly:
        case TextStyle::LabelDimTextOnly:
        case TextStyle::LabelDimIcon:
        case TextStyle::LabelDimText:
            return index;
        /* LCOV_EXCL_START */
        case TextStyle::ButtonDisabledIconOnly:
        case TextStyle::ButtonDisabledTextOnly:
        case TextStyle::ButtonDisabledIcon:
        case TextStyle::ButtonDisabledText:
        case TextStyle::ButtonFlatDisabledIconOnly:
        case TextStyle::ButtonFlatDisabledTextOnly:
        case TextStyle::ButtonFlatDisabledIcon:
        case TextStyle::ButtonFlatDisabledText:
        case TextStyle::LabelDefaultDisabledIconOnly:
        case TextStyle::LabelDefaultDisabledTextOnly:
        case TextStyle::LabelDefaultDisabledIcon:
        case TextStyle::LabelDefaultDisabledText:
        case TextStyle::LabelPrimaryDisabledIconOnly:
        case TextStyle::LabelPrimaryDisabledTextOnly:
        case TextStyle::LabelPrimaryDisabledIcon:
        case TextStyle::LabelPrimaryDisabledText:
        case TextStyle::LabelSuccessDisabledIconOnly:
        case TextStyle::LabelSuccessDisabledTextOnly:
        case TextStyle::LabelSuccessDisabledIcon:
        case TextStyle::LabelSuccessDisabledText:
        case TextStyle::LabelWarningDisabledIconOnly:
        case TextStyle::LabelWarningDisabledTextOnly:
        case TextStyle::LabelWarningDisabledIcon:
        case TextStyle::LabelWarningDisabledText:
        case TextStyle::LabelDangerDisabledIconOnly:
        case TextStyle::LabelDangerDisabledTextOnly:
        case TextStyle::LabelDangerDisabledIcon:
        case TextStyle::LabelDangerDisabledText:
        case TextStyle::LabelInfoDisabledIconOnly:
        case TextStyle::LabelInfoDisabledTextOnly:
        case TextStyle::LabelInfoDisabledIcon:
        case TextStyle::LabelInfoDisabledText:
        case TextStyle::LabelDimDisabledIconOnly:
        case TextStyle::LabelDimDisabledTextOnly:
        case TextStyle::LabelDimDisabledIcon:
        case TextStyle::LabelDimDisabledText:
            CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        /* LCOV_EXCL_STOP */
    }

    CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
}

TextStyle styleTransitionToInactiveOver(const TextStyle index) {
    switch(index) {
        case TextStyle::ButtonIconOnly:
        case TextStyle::ButtonPressedIconOnly:
            return TextStyle::ButtonIconOnly;
        case TextStyle::ButtonTextOnly:
        case TextStyle::ButtonPressedTextOnly:
            return TextStyle::ButtonTextOnly;
        case TextStyle::ButtonIcon:
        case TextStyle::ButtonPressedIcon:
            return TextStyle::ButtonIcon;
        case TextStyle::ButtonText:
        case TextStyle::ButtonPressedText:
            return TextStyle::ButtonText;
        case TextStyle::ButtonFlatInactiveOutIconOnly:
        case TextStyle::ButtonFlatInactiveOverIconOnly:
        case TextStyle::ButtonFlatPressedOutIconOnly:
        case TextStyle::ButtonFlatPressedOverIconOnly:
            return TextStyle::ButtonFlatInactiveOverIconOnly;
        case TextStyle::ButtonFlatInactiveOutTextOnly:
        case TextStyle::ButtonFlatInactiveOverTextOnly:
        case TextStyle::ButtonFlatPressedOutTextOnly:
        case TextStyle::ButtonFlatPressedOverTextOnly:
            return TextStyle::ButtonFlatInactiveOverTextOnly;
        case TextStyle::ButtonFlatInactiveOutIcon:
        case TextStyle::ButtonFlatInactiveOverIcon:
        case TextStyle::ButtonFlatPressedOutIcon:
        case TextStyle::ButtonFlatPressedOverIcon:
            return TextStyle::ButtonFlatInactiveOverIcon;
        case TextStyle::ButtonFlatInactiveOutText:
        case TextStyle::ButtonFlatInactiveOverText:
        case TextStyle::ButtonFlatPressedOutText:
        case TextStyle::ButtonFlatPressedOverText:
            return TextStyle::ButtonFlatInactiveOverText;
        case TextStyle::LabelDefaultIconOnly:
        case TextStyle::LabelDefaultTextOnly:
        case TextStyle::LabelDefaultIcon:
        case TextStyle::LabelDefaultText:
        case TextStyle::LabelPrimaryIconOnly:
        case TextStyle::LabelPrimaryTextOnly:
        case TextStyle::LabelPrimaryIcon:
        case TextStyle::LabelPrimaryText:
        case TextStyle::LabelSuccessIconOnly:
        case TextStyle::LabelSuccessTextOnly:
        case TextStyle::LabelSuccessIcon:
        case TextStyle::LabelSuccessText:
        case TextStyle::LabelWarningIconOnly:
        case TextStyle::LabelWarningTextOnly:
        case TextStyle::LabelWarningIcon:
        case TextStyle::LabelWarningText:
        case TextStyle::LabelDangerIconOnly:
        case TextStyle::LabelDangerTextOnly:
        case TextStyle::LabelDangerIcon:
        case TextStyle::LabelDangerText:
        case TextStyle::LabelInfoIconOnly:
        case TextStyle::LabelInfoTextOnly:
        case TextStyle::LabelInfoIcon:
        case TextStyle::LabelInfoText:
        case TextStyle::LabelDimIconOnly:
        case TextStyle::LabelDimTextOnly:
        case TextStyle::LabelDimIcon:
        case TextStyle::LabelDimText:
            return index;
        /* LCOV_EXCL_START */
        case TextStyle::ButtonDisabledIconOnly:
        case TextStyle::ButtonDisabledTextOnly:
        case TextStyle::ButtonDisabledIcon:
        case TextStyle::ButtonDisabledText:
        case TextStyle::ButtonFlatDisabledIconOnly:
        case TextStyle::ButtonFlatDisabledTextOnly:
        case TextStyle::ButtonFlatDisabledIcon:
        case TextStyle::ButtonFlatDisabledText:
        case TextStyle::LabelDefaultDisabledIconOnly:
        case TextStyle::LabelDefaultDisabledTextOnly:
        case TextStyle::LabelDefaultDisabledIcon:
        case TextStyle::LabelDefaultDisabledText:
        case TextStyle::LabelPrimaryDisabledIconOnly:
        case TextStyle::LabelPrimaryDisabledTextOnly:
        case TextStyle::LabelPrimaryDisabledIcon:
        case TextStyle::LabelPrimaryDisabledText:
        case TextStyle::LabelSuccessDisabledIconOnly:
        case TextStyle::LabelSuccessDisabledTextOnly:
        case TextStyle::LabelSuccessDisabledIcon:
        case TextStyle::LabelSuccessDisabledText:
        case TextStyle::LabelWarningDisabledIconOnly:
        case TextStyle::LabelWarningDisabledTextOnly:
        case TextStyle::LabelWarningDisabledIcon:
        case TextStyle::LabelWarningDisabledText:
        case TextStyle::LabelDangerDisabledIconOnly:
        case TextStyle::LabelDangerDisabledTextOnly:
        case TextStyle::LabelDangerDisabledIcon:
        case TextStyle::LabelDangerDisabledText:
        case TextStyle::LabelInfoDisabledIconOnly:
        case TextStyle::LabelInfoDisabledTextOnly:
        case TextStyle::LabelInfoDisabledIcon:
        case TextStyle::LabelInfoDisabledText:
        case TextStyle::LabelDimDisabledIconOnly:
        case TextStyle::LabelDimDisabledTextOnly:
        case TextStyle::LabelDimDisabledIcon:
        case TextStyle::LabelDimDisabledText:
            CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        /* LCOV_EXCL_STOP */
    }

    CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
}

TextStyle styleTransitionToPressedOut(const TextStyle index) {
    switch(index) {
        case TextStyle::ButtonIconOnly:
        case TextStyle::ButtonPressedIconOnly:
            return TextStyle::ButtonPressedIconOnly;
        case TextStyle::ButtonTextOnly:
        case TextStyle::ButtonPressedTextOnly:
            return TextStyle::ButtonPressedTextOnly;
        case TextStyle::ButtonIcon:
        case TextStyle::ButtonPressedIcon:
            return TextStyle::ButtonPressedIcon;
        case TextStyle::ButtonText:
        case TextStyle::ButtonPressedText:
            return TextStyle::ButtonPressedText;
        case TextStyle::ButtonFlatInactiveOutIconOnly:
        case TextStyle::ButtonFlatInactiveOverIconOnly:
        case TextStyle::ButtonFlatPressedOutIconOnly:
        case TextStyle::ButtonFlatPressedOverIconOnly:
            return TextStyle::ButtonFlatPressedOutIconOnly;
        case TextStyle::ButtonFlatInactiveOutTextOnly:
        case TextStyle::ButtonFlatInactiveOverTextOnly:
        case TextStyle::ButtonFlatPressedOutTextOnly:
        case TextStyle::ButtonFlatPressedOverTextOnly:
            return TextStyle::ButtonFlatPressedOutTextOnly;
        case TextStyle::ButtonFlatInactiveOutIcon:
        case TextStyle::ButtonFlatInactiveOverIcon:
        case TextStyle::ButtonFlatPressedOutIcon:
        case TextStyle::ButtonFlatPressedOverIcon:
            return TextStyle::ButtonFlatPressedOutIcon;
        case TextStyle::ButtonFlatInactiveOutText:
        case TextStyle::ButtonFlatInactiveOverText:
        case TextStyle::ButtonFlatPressedOutText:
        case TextStyle::ButtonFlatPressedOverText:
            return TextStyle::ButtonFlatPressedOutText;
        case TextStyle::LabelDefaultIconOnly:
        case TextStyle::LabelDefaultTextOnly:
        case TextStyle::LabelDefaultIcon:
        case TextStyle::LabelDefaultText:
        case TextStyle::LabelPrimaryIconOnly:
        case TextStyle::LabelPrimaryTextOnly:
        case TextStyle::LabelPrimaryIcon:
        case TextStyle::LabelPrimaryText:
        case TextStyle::LabelSuccessIconOnly:
        case TextStyle::LabelSuccessTextOnly:
        case TextStyle::LabelSuccessIcon:
        case TextStyle::LabelSuccessText:
        case TextStyle::LabelWarningIconOnly:
        case TextStyle::LabelWarningTextOnly:
        case TextStyle::LabelWarningIcon:
        case TextStyle::LabelWarningText:
        case TextStyle::LabelDangerIconOnly:
        case TextStyle::LabelDangerTextOnly:
        case TextStyle::LabelDangerIcon:
        case TextStyle::LabelDangerText:
        case TextStyle::LabelInfoIconOnly:
        case TextStyle::LabelInfoTextOnly:
        case TextStyle::LabelInfoIcon:
        case TextStyle::LabelInfoText:
        case TextStyle::LabelDimIconOnly:
        case TextStyle::LabelDimTextOnly:
        case TextStyle::LabelDimIcon:
        case TextStyle::LabelDimText:
            return index;
        /* LCOV_EXCL_START */
        case TextStyle::ButtonDisabledIconOnly:
        case TextStyle::ButtonDisabledTextOnly:
        case TextStyle::ButtonDisabledIcon:
        case TextStyle::ButtonDisabledText:
        case TextStyle::ButtonFlatDisabledIconOnly:
        case TextStyle::ButtonFlatDisabledTextOnly:
        case TextStyle::ButtonFlatDisabledIcon:
        case TextStyle::ButtonFlatDisabledText:
        case TextStyle::LabelDefaultDisabledIconOnly:
        case TextStyle::LabelDefaultDisabledTextOnly:
        case TextStyle::LabelDefaultDisabledIcon:
        case TextStyle::LabelDefaultDisabledText:
        case TextStyle::LabelPrimaryDisabledIconOnly:
        case TextStyle::LabelPrimaryDisabledTextOnly:
        case TextStyle::LabelPrimaryDisabledIcon:
        case TextStyle::LabelPrimaryDisabledText:
        case TextStyle::LabelSuccessDisabledIconOnly:
        case TextStyle::LabelSuccessDisabledTextOnly:
        case TextStyle::LabelSuccessDisabledIcon:
        case TextStyle::LabelSuccessDisabledText:
        case TextStyle::LabelWarningDisabledIconOnly:
        case TextStyle::LabelWarningDisabledTextOnly:
        case TextStyle::LabelWarningDisabledIcon:
        case TextStyle::LabelWarningDisabledText:
        case TextStyle::LabelDangerDisabledIconOnly:
        case TextStyle::LabelDangerDisabledTextOnly:
        case TextStyle::LabelDangerDisabledIcon:
        case TextStyle::LabelDangerDisabledText:
        case TextStyle::LabelInfoDisabledIconOnly:
        case TextStyle::LabelInfoDisabledTextOnly:
        case TextStyle::LabelInfoDisabledIcon:
        case TextStyle::LabelInfoDisabledText:
        case TextStyle::LabelDimDisabledIconOnly:
        case TextStyle::LabelDimDisabledTextOnly:
        case TextStyle::LabelDimDisabledIcon:
        case TextStyle::LabelDimDisabledText:
            CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        /* LCOV_EXCL_STOP */
    }

    CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
}

TextStyle styleTransitionToPressedOver(const TextStyle index) {
    switch(index) {
        case TextStyle::ButtonIconOnly:
        case TextStyle::ButtonPressedIconOnly:
            return TextStyle::ButtonPressedIconOnly;
        case TextStyle::ButtonTextOnly:
        case TextStyle::ButtonPressedTextOnly:
            return TextStyle::ButtonPressedTextOnly;
        case TextStyle::ButtonIcon:
        case TextStyle::ButtonPressedIcon:
            return TextStyle::ButtonPressedIcon;
        case TextStyle::ButtonText:
        case TextStyle::ButtonPressedText:
            return TextStyle::ButtonPressedText;
        case TextStyle::ButtonFlatInactiveOutIconOnly:
        case TextStyle::ButtonFlatInactiveOverIconOnly:
        case TextStyle::ButtonFlatPressedOutIconOnly:
        case TextStyle::ButtonFlatPressedOverIconOnly:
            return TextStyle::ButtonFlatPressedOverIconOnly;
        case TextStyle::ButtonFlatInactiveOutTextOnly:
        case TextStyle::ButtonFlatInactiveOverTextOnly:
        case TextStyle::ButtonFlatPressedOutTextOnly:
        case TextStyle::ButtonFlatPressedOverTextOnly:
            return TextStyle::ButtonFlatPressedOverTextOnly;
        case TextStyle::ButtonFlatInactiveOutIcon:
        case TextStyle::ButtonFlatInactiveOverIcon:
        case TextStyle::ButtonFlatPressedOutIcon:
        case TextStyle::ButtonFlatPressedOverIcon:
            return TextStyle::ButtonFlatPressedOverIcon;
        case TextStyle::ButtonFlatInactiveOutText:
        case TextStyle::ButtonFlatInactiveOverText:
        case TextStyle::ButtonFlatPressedOutText:
        case TextStyle::ButtonFlatPressedOverText:
            return TextStyle::ButtonFlatPressedOverText;
        case TextStyle::LabelDefaultIconOnly:
        case TextStyle::LabelDefaultTextOnly:
        case TextStyle::LabelDefaultIcon:
        case TextStyle::LabelDefaultText:
        case TextStyle::LabelPrimaryIconOnly:
        case TextStyle::LabelPrimaryTextOnly:
        case TextStyle::LabelPrimaryIcon:
        case TextStyle::LabelPrimaryText:
        case TextStyle::LabelSuccessIconOnly:
        case TextStyle::LabelSuccessTextOnly:
        case TextStyle::LabelSuccessIcon:
        case TextStyle::LabelSuccessText:
        case TextStyle::LabelWarningIconOnly:
        case TextStyle::LabelWarningTextOnly:
        case TextStyle::LabelWarningIcon:
        case TextStyle::LabelWarningText:
        case TextStyle::LabelDangerIconOnly:
        case TextStyle::LabelDangerTextOnly:
        case TextStyle::LabelDangerIcon:
        case TextStyle::LabelDangerText:
        case TextStyle::LabelInfoIconOnly:
        case TextStyle::LabelInfoTextOnly:
        case TextStyle::LabelInfoIcon:
        case TextStyle::LabelInfoText:
        case TextStyle::LabelDimIconOnly:
        case TextStyle::LabelDimTextOnly:
        case TextStyle::LabelDimIcon:
        case TextStyle::LabelDimText:
            return index;
        /* LCOV_EXCL_START */
        case TextStyle::ButtonDisabledIconOnly:
        case TextStyle::ButtonDisabledTextOnly:
        case TextStyle::ButtonDisabledIcon:
        case TextStyle::ButtonDisabledText:
        case TextStyle::ButtonFlatDisabledIconOnly:
        case TextStyle::ButtonFlatDisabledTextOnly:
        case TextStyle::ButtonFlatDisabledIcon:
        case TextStyle::ButtonFlatDisabledText:
        case TextStyle::LabelDefaultDisabledIconOnly:
        case TextStyle::LabelDefaultDisabledTextOnly:
        case TextStyle::LabelDefaultDisabledIcon:
        case TextStyle::LabelDefaultDisabledText:
        case TextStyle::LabelPrimaryDisabledIconOnly:
        case TextStyle::LabelPrimaryDisabledTextOnly:
        case TextStyle::LabelPrimaryDisabledIcon:
        case TextStyle::LabelPrimaryDisabledText:
        case TextStyle::LabelSuccessDisabledIconOnly:
        case TextStyle::LabelSuccessDisabledTextOnly:
        case TextStyle::LabelSuccessDisabledIcon:
        case TextStyle::LabelSuccessDisabledText:
        case TextStyle::LabelWarningDisabledIconOnly:
        case TextStyle::LabelWarningDisabledTextOnly:
        case TextStyle::LabelWarningDisabledIcon:
        case TextStyle::LabelWarningDisabledText:
        case TextStyle::LabelDangerDisabledIconOnly:
        case TextStyle::LabelDangerDisabledTextOnly:
        case TextStyle::LabelDangerDisabledIcon:
        case TextStyle::LabelDangerDisabledText:
        case TextStyle::LabelInfoDisabledIconOnly:
        case TextStyle::LabelInfoDisabledTextOnly:
        case TextStyle::LabelInfoDisabledIcon:
        case TextStyle::LabelInfoDisabledText:
        case TextStyle::LabelDimDisabledIconOnly:
        case TextStyle::LabelDimDisabledTextOnly:
        case TextStyle::LabelDimDisabledIcon:
        case TextStyle::LabelDimDisabledText:
            CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        /* LCOV_EXCL_STOP */
    }

    CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
}

TextStyle styleTransitionToDisabled(const TextStyle index) {
    switch(index) {
        case TextStyle::ButtonIconOnly:
        case TextStyle::ButtonPressedIconOnly:
            return TextStyle::ButtonDisabledIconOnly;
        case TextStyle::ButtonTextOnly:
        case TextStyle::ButtonPressedTextOnly:
            return TextStyle::ButtonDisabledTextOnly;
        case TextStyle::ButtonIcon:
        case TextStyle::ButtonPressedIcon:
            return TextStyle::ButtonDisabledIcon;
        case TextStyle::ButtonText:
        case TextStyle::ButtonPressedText:
            return TextStyle::ButtonDisabledText;
        case TextStyle::ButtonFlatInactiveOutIconOnly:
        case TextStyle::ButtonFlatInactiveOverIconOnly:
        case TextStyle::ButtonFlatPressedOutIconOnly:
        case TextStyle::ButtonFlatPressedOverIconOnly:
            return TextStyle::ButtonFlatDisabledIconOnly;
        case TextStyle::ButtonFlatInactiveOutTextOnly:
        case TextStyle::ButtonFlatInactiveOverTextOnly:
        case TextStyle::ButtonFlatPressedOutTextOnly:
        case TextStyle::ButtonFlatPressedOverTextOnly:
            return TextStyle::ButtonFlatDisabledTextOnly;
        case TextStyle::ButtonFlatInactiveOutIcon:
        case TextStyle::ButtonFlatInactiveOverIcon:
        case TextStyle::ButtonFlatPressedOutIcon:
        case TextStyle::ButtonFlatPressedOverIcon:
            return TextStyle::ButtonFlatDisabledIcon;
        case TextStyle::ButtonFlatInactiveOutText:
        case TextStyle::ButtonFlatInactiveOverText:
        case TextStyle::ButtonFlatPressedOutText:
        case TextStyle::ButtonFlatPressedOverText:
            return TextStyle::ButtonFlatDisabledText;
        case TextStyle::LabelDefaultIconOnly:
            return TextStyle::LabelDefaultDisabledIconOnly;
        case TextStyle::LabelDefaultTextOnly:
            return TextStyle::LabelDefaultDisabledTextOnly;
        case TextStyle::LabelDefaultIcon:
            return TextStyle::LabelDefaultDisabledIcon;
        case TextStyle::LabelDefaultText:
            return TextStyle::LabelDefaultDisabledText;
        case TextStyle::LabelPrimaryIconOnly:
            return TextStyle::LabelPrimaryDisabledIconOnly;
        case TextStyle::LabelPrimaryTextOnly:
            return TextStyle::LabelPrimaryDisabledTextOnly;
        case TextStyle::LabelPrimaryIcon:
            return TextStyle::LabelPrimaryDisabledIcon;
        case TextStyle::LabelPrimaryText:
            return TextStyle::LabelPrimaryDisabledText;
        case TextStyle::LabelSuccessIconOnly:
            return TextStyle::LabelSuccessDisabledIconOnly;
        case TextStyle::LabelSuccessTextOnly:
            return TextStyle::LabelSuccessDisabledTextOnly;
        case TextStyle::LabelSuccessIcon:
            return TextStyle::LabelSuccessDisabledIcon;
        case TextStyle::LabelSuccessText:
            return TextStyle::LabelSuccessDisabledText;
        case TextStyle::LabelWarningIconOnly:
            return TextStyle::LabelWarningDisabledIconOnly;
        case TextStyle::LabelWarningTextOnly:
            return TextStyle::LabelWarningDisabledTextOnly;
        case TextStyle::LabelWarningIcon:
            return TextStyle::LabelWarningDisabledIcon;
        case TextStyle::LabelWarningText:
            return TextStyle::LabelWarningDisabledText;
        case TextStyle::LabelDangerIconOnly:
            return TextStyle::LabelDangerDisabledIconOnly;
        case TextStyle::LabelDangerTextOnly:
            return TextStyle::LabelDangerDisabledTextOnly;
        case TextStyle::LabelDangerIcon:
            return TextStyle::LabelDangerDisabledIcon;
        case TextStyle::LabelDangerText:
            return TextStyle::LabelDangerDisabledText;
        case TextStyle::LabelInfoIconOnly:
            return TextStyle::LabelInfoDisabledIconOnly;
        case TextStyle::LabelInfoTextOnly:
            return TextStyle::LabelInfoDisabledTextOnly;
        case TextStyle::LabelInfoIcon:
            return TextStyle::LabelInfoDisabledIcon;
        case TextStyle::LabelInfoText:
            return TextStyle::LabelInfoDisabledText;
        case TextStyle::LabelDimIconOnly:
            return TextStyle::LabelDimDisabledIconOnly;
        case TextStyle::LabelDimTextOnly:
            return TextStyle::LabelDimDisabledTextOnly;
        case TextStyle::LabelDimIcon:
            return TextStyle::LabelDimDisabledIcon;
        case TextStyle::LabelDimText:
            return TextStyle::LabelDimDisabledText;
        /* LCOV_EXCL_START */
        case TextStyle::ButtonDisabledIconOnly:
        case TextStyle::ButtonDisabledTextOnly:
        case TextStyle::ButtonDisabledIcon:
        case TextStyle::ButtonDisabledText:
        case TextStyle::ButtonFlatDisabledIconOnly:
        case TextStyle::ButtonFlatDisabledTextOnly:
        case TextStyle::ButtonFlatDisabledIcon:
        case TextStyle::ButtonFlatDisabledText:
        case TextStyle::LabelDefaultDisabledIconOnly:
        case TextStyle::LabelDefaultDisabledTextOnly:
        case TextStyle::LabelDefaultDisabledIcon:
        case TextStyle::LabelDefaultDisabledText:
        case TextStyle::LabelPrimaryDisabledIconOnly:
        case TextStyle::LabelPrimaryDisabledTextOnly:
        case TextStyle::LabelPrimaryDisabledIcon:
        case TextStyle::LabelPrimaryDisabledText:
        case TextStyle::LabelSuccessDisabledIconOnly:
        case TextStyle::LabelSuccessDisabledTextOnly:
        case TextStyle::LabelSuccessDisabledIcon:
        case TextStyle::LabelSuccessDisabledText:
        case TextStyle::LabelWarningDisabledIconOnly:
        case TextStyle::LabelWarningDisabledTextOnly:
        case TextStyle::LabelWarningDisabledIcon:
        case TextStyle::LabelWarningDisabledText:
        case TextStyle::LabelDangerDisabledIconOnly:
        case TextStyle::LabelDangerDisabledTextOnly:
        case TextStyle::LabelDangerDisabledIcon:
        case TextStyle::LabelDangerDisabledText:
        case TextStyle::LabelInfoDisabledIconOnly:
        case TextStyle::LabelInfoDisabledTextOnly:
        case TextStyle::LabelInfoDisabledIcon:
        case TextStyle::LabelInfoDisabledText:
        case TextStyle::LabelDimDisabledIconOnly:
        case TextStyle::LabelDimDisabledTextOnly:
        case TextStyle::LabelDimDisabledIcon:
        case TextStyle::LabelDimDisabledText:
            CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        /* LCOV_EXCL_STOP */
    }

    CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
}

// TODO account for DPI scaling here also!
constexpr BaseLayerCommonStyleUniform BaseLayerCommonStyleUniformMcssDark{0.5f};

#ifndef CORRADE_MSVC2015_COMPATIBILITY
constexpr /* Trust me, you don't want to be on this compiler */
#endif
const BaseLayerStyleUniform BaseLayerStyleUniformsMcssDark[]{
    #define _c(style, ...) {__VA_ARGS__},
    #include "Magnum/Whee/Implementation/baseStyleUniformsMcssDark.h"
    #undef _c
};

constexpr TextLayerCommonStyleUniform TextLayerCommonStyleUniformMcssDark{};

#ifndef CORRADE_MSVC2015_COMPATIBILITY
constexpr /* Trust me, you don't want to be on this compiler */
#endif
const TextLayerStyleUniform TextLayerStyleUniformsMcssDark[]{
    #define _c(style, ...) {__VA_ARGS__},
    #include "Magnum/Whee/Implementation/textStyleUniformsMcssDark.h"
    #undef _c
};

constexpr struct {
    UnsignedInt uniform;
    Vector4 padding;
} TextStyleData[]{
    #define _c(style, suffix, font, ...) {UnsignedInt(TextStyleUniform::style), __VA_ARGS__},
    #include "Magnum/Whee/Implementation/textStyleMcssDark.h"
    #undef _c
};

}

StyleFeatures McssDarkStyle::doFeatures() const {
    return StyleFeature::BaseLayer|
           StyleFeature::TextLayer|
           StyleFeature::TextLayerImages|
           StyleFeature::EventLayer;
}

UnsignedInt McssDarkStyle::doBaseLayerStyleUniformCount() const {
    return Containers::arraySize(BaseLayerStyleUniformsMcssDark);
}

UnsignedInt McssDarkStyle::doBaseLayerStyleCount() const {
    return Containers::arraySize(BaseLayerStyleUniformsMcssDark);
}

UnsignedInt McssDarkStyle::doTextLayerStyleUniformCount() const {
    return Containers::arraySize(TextLayerStyleUniformsMcssDark);
}

UnsignedInt McssDarkStyle::doTextLayerStyleCount() const {
    return Containers::arraySize(TextStyleData);
}

Vector3i McssDarkStyle::doTextLayerGlyphCacheSize(StyleFeatures) const {
    return {256, 256, 1};
}

bool McssDarkStyle::doApply(UserInterface& ui, const StyleFeatures features, PluginManager::Manager<Trade::AbstractImporter>* importerManager, PluginManager::Manager<Text::AbstractFont>* fontManager) const {
    /* Base layer style */
    if(features >= StyleFeature::BaseLayer) {
        ui.baseLayer().shared()
            .setStyle(
                BaseLayerCommonStyleUniformMcssDark,
                BaseLayerStyleUniformsMcssDark,
                {})
            .setStyleTransition<BaseStyle,
                styleTransitionToPressedOut,
                styleTransitionToPressedOver,
                styleTransitionToInactiveOut,
                styleTransitionToInactiveOver,
                styleTransitionToDisabled>();
    }

    /* Icon font. Add also if just the text layer style is applied (where it
       gets assigned to icon styles, but without any icons actually loaded). */
    Whee::FontHandle iconFont;
    if(features & (StyleFeature::TextLayer|StyleFeature::TextLayerImages)) {
        #ifdef MAGNUM_BUILD_STATIC
        if(!Utility::Resource::hasGroup("MagnumWhee"_s))
            importShaderResources();
        #endif

        TextLayer::Shared& shared = ui.textLayer().shared();
        Text::AbstractGlyphCache& glyphCache = shared.glyphCache();
        /* The Icon enum reserves 0 for an invalid glyph, so add 1 */
        const UnsignedInt iconFontId = glyphCache.addFont(Implementation::IconCount + 1);
        // TODO account for DPI scaling!
        iconFont = shared.addInstancelessFont(iconFontId, 24.0f/64.0f);
    }

    /* Text layer fonts and style */
    // TODO this means that a font will be added each time a style gets applied, not nice -- something should remove all of them and clear the cache before setting a style, i think?
    // TODO same for the images, prune from the cache first
    if(features >= StyleFeature::TextLayer) {
        TextLayer::Shared& shared = ui.textLayer().shared();
        Text::AbstractGlyphCache& glyphCache = shared.glyphCache();
        const Utility::Resource rs{"MagnumWhee"_s};

        Containers::Pointer<Text::AbstractFont> font = fontManager->loadAndInstantiate("TrueTypeFont");
        // TODO account for DPI scaling
        if(!font || !font->openData(rs.getRaw("SourceSansPro-Regular.ttf"_s), 16.0f*2)) {
            Error{} << "Whee::McssDarkStyle::apply(): cannot open a font";
            return {};
        }
        /** @todo fail if this fails, once the function doesn't return void */
        /** @todo configurable way to fill the cache, or switch to on-demand
            by default once AbstractFont can fill the cache with glyph IDs */
        font->fillGlyphCache(glyphCache,
            "abcdefghijklmnopqrstuvwxyz"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "0123456789 _.,-+=*:;?!@$&#/\\|`\"'<>()[]{}%…");

        /* Main font */
        const Whee::FontHandle mainFont = shared.addFont(Utility::move(font), 16.0f);

        /* Font handles matching all styles. References either the `mainFont`
           or the `iconFont` defined above. */
        const Whee::FontHandle fontHandles[]{
            #define _c(style, suffix, font, ...) font,
            #include "Magnum/Whee/Implementation/textStyleMcssDark.h"
            #undef _c
        };

        shared
            .setStyle(
                TextLayerCommonStyleUniformMcssDark,
                TextLayerStyleUniformsMcssDark,
                Containers::stridedArrayView(TextStyleData).slice(&std::remove_all_extents<decltype(TextStyleData)>::type::uniform),
                fontHandles,
                Containers::stridedArrayView(TextStyleData).slice(&std::remove_all_extents<decltype(TextStyleData)>::type::padding))
            .setStyleTransition<TextStyle,
                styleTransitionToPressedOut,
                styleTransitionToPressedOver,
                styleTransitionToInactiveOut,
                styleTransitionToInactiveOver,
                styleTransitionToDisabled>();
    }

    /* Text layer images */
    if(features >= StyleFeature::TextLayerImages) {
        TextLayer::Shared& shared = ui.textLayer().shared();
        Text::AbstractGlyphCache& glyphCache = shared.glyphCache();
        const Utility::Resource rs{"MagnumWhee"_s};

        Containers::Pointer<Trade::AbstractImporter> importer = importerManager->loadAndInstantiate("AnyImageImporter");
        Containers::Optional<Trade::ImageData2D> image;
        if(!importer || !importer->openMemory(rs.getRaw("icons.png")) || !(image = importer->image2D(0))) {
            Error{} << "Whee::McssDarkStyle::apply(): cannot open an icon atlas";
            return {};
        }

        /* The image is originally grayscale 8-bit, expect that it's still
           imported with 8-bit channels. The importer can be globally
           configured to import them with more channels (which is fine, for
           example in testing context, where we might always want to compare to
           a RGBA image even if the on-disk representation has the alpha
           dropped), in which case just the red channel is taken, but it's
           important that it isn't expanded to 16 bits or to floats, for
           example. */
        if(pixelFormatChannelFormat(image->format()) != PixelFormat::R8Unorm) {
            Error{} << "Whee::McssDarkStyle::apply(): expected" << PixelFormat::R8Unorm << "icons but got an image with" << image->format();
            return {};
        }
        const std::size_t channelSize = image->pixelSize()/pixelFormatChannelCount(image->format());

        /* At the moment it's a single row of square icons, with the image
           height denoting the square size, and the order matching the Icon
           enum. Reserve space for all of them in the glyph cache. */
        const Vector2i imageSize{image->size().y()};
        CORRADE_INTERNAL_ASSERT(image->size().x() % image->size().y() == 0);
        Vector3i offsets[Implementation::IconCount];
        if(!glyphCache.atlas().add(Containers::stridedArrayView(&imageSize, 1).broadcasted<0>(Implementation::IconCount), offsets)) {
            Error{} << "Whee::McssDarkStyle::apply(): cannot fit" << Implementation::IconCount << "icons into the glyph cache";
            return {};
        }

        /* The font was added above, query the glyph cache ID of it */
        const UnsignedInt iconFontId = shared.glyphCacheFontId(iconFont);

        /* Copy the image data */
        Containers::StridedArrayView3D<const char> src = image->pixels();
        Containers::StridedArrayView4D<char> dst = glyphCache.image().pixels();
        Range2Di updated;
        for(UnsignedInt i = 0; i != Implementation::IconCount; ++i) {
            Range2Di rectangle = Range2Di::fromSize(offsets[i].xy(),
                                                    imageSize);
            /* The Icon enum reserves 0 for an invalid glyph, so add 1 */
            glyphCache.addGlyph(iconFontId, i + 1, {}, rectangle);

            /* Copy assuming all input images have the same pixel format */
            const Containers::Size3D size{
                std::size_t(imageSize.y()),
                std::size_t(imageSize.x()),
                channelSize};
            Utility::copy(
                src.sliceSize({0, std::size_t(i*imageSize.x()), 0}, size),
                dst[offsets[i].z()].sliceSize({std::size_t(offsets[i].y()),
                                                std::size_t(offsets[i].x()),
                                                0}, size));

            /* Maintain a range that was updated in the glyph cache */
            updated = Math::join(updated, rectangle);
        }

        /* Reflect the image data update to the actual GPU-side texture */
        glyphCache.flushImage(updated);
    }

    /* Event layer */
    if(features >= StyleFeature::EventLayer) {
        /* Right now nothing to set here. It's present in features() mainly in
           order to make UserInterface implicitly add this layer for use by the
           application. */
    }

    return true;
}

}}
