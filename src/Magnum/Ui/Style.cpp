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

#include "Style.h"

#include <Corrade/Utility/Resource.h>
#include <Magnum/Buffer.h>
#include <Magnum/Shader.h>
#include <Magnum/Texture.h>
#include <Magnum/Version.h>

#include "Magnum/Ui/Widget.h"

#ifdef CORRADE_TARGET_ANDROID
#include <sstream>
#endif

#ifdef MAGNUM_BUILD_STATIC
static void importShaderResources() {
    CORRADE_RESOURCE_INITIALIZE(MagnumUi_RESOURCES)
}
#endif

namespace Magnum { namespace Ui {

namespace Implementation {

UnsignedByte backgroundColorIndex(const Type type, const Style style, const State state) {
    enum: UnsignedByte {
        Hidden = 0,

        ModalDefaultDefault,
        ModalDefaultDisabled,
        ModalPrimaryDefault,
        ModalPrimaryDisabled,
        ModalSuccessDefault,
        ModalSuccessDisabled,
        ModalInfoDefault,
        ModalInfoDisabled,
        ModalWarningDefault,
        ModalWarningDisabled,
        ModalDangerDefault,
        ModalDangerDisabled,
        ModalDimDefault,

        BackgroundColorCount
    };

    static_assert(UnsignedByte(BackgroundColorCount) <= UnsignedByte(Implementation::BackgroundColorCount), "");

    if(state == State::Hidden)
        return Hidden;

    const UnsignedInt encoded = (UnsignedInt(type) << 16) |
                                (UnsignedInt(style) << 8) |
                                 UnsignedInt(state);

    switch(encoded) {
        #define _c(type, style, state)                                      \
            case (UnsignedInt(Type::type) << 16) |                          \
                 (UnsignedInt(Style::style) << 8) |                         \
                  UnsignedInt(State::state):                                \
                return type ## style ## state;
        _c(Modal, Default, Default)
        _c(Modal, Default, Disabled)
        _c(Modal, Primary, Default)
        _c(Modal, Primary, Disabled)
        _c(Modal, Success, Default)
        _c(Modal, Success, Disabled)
        _c(Modal, Info, Default)
        _c(Modal, Info, Disabled)
        _c(Modal, Warning, Default)
        _c(Modal, Warning, Disabled)
        _c(Modal, Danger, Default)
        _c(Modal, Danger, Disabled)
        _c(Modal, Dim, Default)
        #undef _c
    }

    /* LCOV_EXCL_START */
    CORRADE_ASSERT(false, "Ui::StyleConfiguration: unsupported background color index" << type << style << state, 0);
    CORRADE_ASSERT_UNREACHABLE();
    /* LCOV_EXCL_STOP */
}

UnsignedByte foregroundColorIndex(const Type type, const Style style, const State state) {
    enum: UnsignedByte {
        Hidden = 0,

        ButtonDefaultDefault,
        ButtonDefaultHover,
        ButtonDefaultPressed,
        ButtonDefaultDisabled,
        ButtonPrimaryDefault,
        ButtonPrimaryHover,
        ButtonPrimaryPressed,
        ButtonPrimaryDisabled,
        ButtonSuccessDefault,
        ButtonSuccessHover,
        ButtonSuccessPressed,
        ButtonSuccessDisabled,
        ButtonInfoDefault,
        ButtonInfoHover,
        ButtonInfoPressed,
        ButtonInfoDisabled,
        ButtonWarningDefault,
        ButtonWarningHover,
        ButtonWarningPressed,
        ButtonWarningDisabled,
        ButtonDangerDefault,
        ButtonDangerHover,
        ButtonDangerPressed,
        ButtonDangerDisabled,
        ButtonDimDefault,
        ButtonDimHover,
        ButtonDimPressed,
        ButtonDimDisabled,

        InputDefaultDefault,
        InputDefaultHover,
        InputDefaultPressed,
        InputDefaultActive,
        InputDefaultDisabled,
        InputPrimaryDefault,
        InputPrimaryHover,
        InputPrimaryPressed,
        InputPrimaryActive,
        InputPrimaryDisabled,
        InputSuccessDefault,
        InputSuccessHover,
        InputSuccessPressed,
        InputSuccessActive,
        InputSuccessDisabled,
        InputInfoDefault,
        InputInfoHover,
        InputInfoPressed,
        InputInfoActive,
        InputInfoDisabled,
        InputWarningDefault,
        InputWarningHover,
        InputWarningPressed,
        InputWarningActive,
        InputWarningDisabled,
        InputDangerDefault,
        InputDangerHover,
        InputDangerPressed,
        InputDangerActive,
        InputDangerDisabled,
        InputDimDefault,
        InputDimHover,
        InputDimPressed,
        InputDimActive,
        InputDimDisabled,

        ForegroundColorCount
    };

    static_assert(UnsignedByte(ForegroundColorCount) <= UnsignedByte(Implementation::ForegroundColorCount), "");

    if(state == State::Hidden)
        return Hidden;

    const UnsignedInt encoded = (UnsignedInt(type) << 16) |
                                (UnsignedInt(style) << 8) |
                                 UnsignedInt(state);

    switch(encoded) {
        #define _c(type, style, state)                                      \
            case (UnsignedInt(Type::type) << 16) |                          \
                 (UnsignedInt(Style::style) << 8) |                         \
                  UnsignedInt(State::state):                                \
                return type ## style ## state;
        _c(Button, Default, Default)
        _c(Button, Default, Hover)
        _c(Button, Default, Pressed)
        _c(Button, Default, Disabled)
        _c(Button, Primary, Default)
        _c(Button, Primary, Hover)
        _c(Button, Primary, Pressed)
        _c(Button, Primary, Disabled)
        _c(Button, Success, Default)
        _c(Button, Success, Hover)
        _c(Button, Success, Pressed)
        _c(Button, Success, Disabled)
        _c(Button, Info, Default)
        _c(Button, Info, Hover)
        _c(Button, Info, Pressed)
        _c(Button, Info, Disabled)
        _c(Button, Warning, Default)
        _c(Button, Warning, Hover)
        _c(Button, Warning, Pressed)
        _c(Button, Warning, Disabled)
        _c(Button, Danger, Default)
        _c(Button, Danger, Hover)
        _c(Button, Danger, Pressed)
        _c(Button, Danger, Disabled)
        _c(Button, Dim, Default)
        _c(Button, Dim, Hover)
        _c(Button, Dim, Pressed)
        _c(Button, Dim, Disabled)

        _c(Input, Default, Default)
        _c(Input, Default, Hover)
        _c(Input, Default, Pressed)
        _c(Input, Default, Active)
        _c(Input, Default, Disabled)
        _c(Input, Primary, Default)
        _c(Input, Primary, Hover)
        _c(Input, Primary, Pressed)
        _c(Input, Primary, Active)
        _c(Input, Primary, Disabled)
        _c(Input, Success, Default)
        _c(Input, Success, Hover)
        _c(Input, Success, Pressed)
        _c(Input, Success, Active)
        _c(Input, Success, Disabled)
        _c(Input, Info, Default)
        _c(Input, Info, Hover)
        _c(Input, Info, Pressed)
        _c(Input, Info, Active)
        _c(Input, Info, Disabled)
        _c(Input, Warning, Default)
        _c(Input, Warning, Hover)
        _c(Input, Warning, Pressed)
        _c(Input, Warning, Active)
        _c(Input, Warning, Disabled)
        _c(Input, Danger, Default)
        _c(Input, Danger, Hover)
        _c(Input, Danger, Pressed)
        _c(Input, Danger, Active)
        _c(Input, Danger, Disabled)
        _c(Input, Dim, Default)
        _c(Input, Dim, Hover)
        _c(Input, Dim, Pressed)
        _c(Input, Dim, Active)
        _c(Input, Dim, Disabled)
        #undef _c
    }

    /* LCOV_EXCL_START */
    CORRADE_ASSERT(false, "Ui::StyleConfiguration: unsupported foreground color index" << type << style << state, 0);
    CORRADE_ASSERT_UNREACHABLE();
    /* LCOV_EXCL_STOP */
}

UnsignedByte textColorIndex(const Type type, const Style style, const State state) {
    enum: UnsignedByte {
        Hidden = 0,

        ButtonDefaultDefault,
        ButtonDefaultHover,
        ButtonDefaultPressed,
        ButtonDefaultDisabled,
        ButtonPrimaryDefault,
        ButtonPrimaryHover,
        ButtonPrimaryPressed,
        ButtonPrimaryDisabled,
        ButtonSuccessDefault,
        ButtonSuccessHover,
        ButtonSuccessPressed,
        ButtonSuccessDisabled,
        ButtonInfoDefault,
        ButtonInfoHover,
        ButtonInfoPressed,
        ButtonInfoDisabled,
        ButtonWarningDefault,
        ButtonWarningHover,
        ButtonWarningPressed,
        ButtonWarningDisabled,
        ButtonDangerDefault,
        ButtonDangerHover,
        ButtonDangerPressed,
        ButtonDangerDisabled,
        ButtonDimDefault,
        ButtonDimHover,
        ButtonDimPressed,
        ButtonDimDisabled,
        ButtonFlatDefault,
        ButtonFlatHover,
        ButtonFlatPressed,
        ButtonFlatDisabled,

        LabelDefaultDefault,
        LabelDefaultDisabled,
        LabelPrimaryDefault,
        LabelPrimaryDisabled,
        LabelSuccessDefault,
        LabelSuccessDisabled,
        LabelInfoDefault,
        LabelInfoDisabled,
        LabelWarningDefault,
        LabelWarningDisabled,
        LabelDangerDefault,
        LabelDangerDisabled,
        LabelDimDefault,
        LabelDimDisabled,

        InputDefaultDefault,
        InputDefaultHover,
        InputDefaultPressed,
        InputDefaultActive,
        InputDefaultDisabled,
        InputPrimaryDefault,
        InputPrimaryHover,
        InputPrimaryPressed,
        InputPrimaryActive,
        InputPrimaryDisabled,
        InputSuccessDefault,
        InputSuccessHover,
        InputSuccessPressed,
        InputSuccessActive,
        InputSuccessDisabled,
        InputInfoDefault,
        InputInfoHover,
        InputInfoPressed,
        InputInfoActive,
        InputInfoDisabled,
        InputWarningDefault,
        InputWarningHover,
        InputWarningPressed,
        InputWarningActive,
        InputWarningDisabled,
        InputDangerDefault,
        InputDangerHover,
        InputDangerPressed,
        InputDangerActive,
        InputDangerDisabled,
        InputDimDefault,
        InputDimHover,
        InputDimPressed,
        InputDimActive,
        InputDimDisabled,
        InputFlatDefault,
        InputFlatHover,
        InputFlatPressed,
        InputFlatActive,
        InputFlatDisabled,

        TextColorCount
    };

    static_assert(UnsignedByte(TextColorCount) <= UnsignedByte(Implementation::TextColorCount), "");

    if(state == State::Hidden)
        return Hidden;

    const UnsignedInt encoded = (UnsignedInt(type) << 16) |
                                (UnsignedInt(style) << 8) |
                                 UnsignedInt(state);

    switch(encoded) {
        #define _c(type, style, state)                                      \
            case (UnsignedInt(Type::type) << 16) |                          \
                 (UnsignedInt(Style::style) << 8) |                         \
                  UnsignedInt(State::state):                                \
                return type ## style ## state;
        _c(Button, Default, Default)
        _c(Button, Default, Hover)
        _c(Button, Default, Pressed)
        _c(Button, Default, Disabled)
        _c(Button, Primary, Default)
        _c(Button, Primary, Hover)
        _c(Button, Primary, Pressed)
        _c(Button, Primary, Disabled)
        _c(Button, Success, Default)
        _c(Button, Success, Hover)
        _c(Button, Success, Pressed)
        _c(Button, Success, Disabled)
        _c(Button, Info, Default)
        _c(Button, Info, Hover)
        _c(Button, Info, Pressed)
        _c(Button, Info, Disabled)
        _c(Button, Warning, Default)
        _c(Button, Warning, Hover)
        _c(Button, Warning, Pressed)
        _c(Button, Warning, Disabled)
        _c(Button, Danger, Default)
        _c(Button, Danger, Hover)
        _c(Button, Danger, Pressed)
        _c(Button, Danger, Disabled)
        _c(Button, Dim, Default)
        _c(Button, Dim, Hover)
        _c(Button, Dim, Pressed)
        _c(Button, Dim, Disabled)
        _c(Button, Flat, Default)
        _c(Button, Flat, Hover)
        _c(Button, Flat, Pressed)
        _c(Button, Flat, Disabled)

        _c(Label, Default, Default)
        _c(Label, Default, Disabled)
        _c(Label, Primary, Default)
        _c(Label, Primary, Disabled)
        _c(Label, Success, Default)
        _c(Label, Success, Disabled)
        _c(Label, Info, Default)
        _c(Label, Info, Disabled)
        _c(Label, Warning, Default)
        _c(Label, Warning, Disabled)
        _c(Label, Danger, Default)
        _c(Label, Danger, Disabled)
        _c(Label, Dim, Default)
        _c(Label, Dim, Disabled)

        _c(Input, Default, Default)
        _c(Input, Default, Hover)
        _c(Input, Default, Pressed)
        _c(Input, Default, Active)
        _c(Input, Default, Disabled)
        _c(Input, Primary, Default)
        _c(Input, Primary, Hover)
        _c(Input, Primary, Pressed)
        _c(Input, Primary, Active)
        _c(Input, Primary, Disabled)
        _c(Input, Success, Default)
        _c(Input, Success, Hover)
        _c(Input, Success, Pressed)
        _c(Input, Success, Active)
        _c(Input, Success, Disabled)
        _c(Input, Info, Default)
        _c(Input, Info, Hover)
        _c(Input, Info, Pressed)
        _c(Input, Info, Active)
        _c(Input, Info, Disabled)
        _c(Input, Warning, Default)
        _c(Input, Warning, Hover)
        _c(Input, Warning, Pressed)
        _c(Input, Warning, Active)
        _c(Input, Warning, Disabled)
        _c(Input, Danger, Default)
        _c(Input, Danger, Hover)
        _c(Input, Danger, Pressed)
        _c(Input, Danger, Active)
        _c(Input, Danger, Disabled)
        _c(Input, Dim, Default)
        _c(Input, Dim, Hover)
        _c(Input, Dim, Pressed)
        _c(Input, Dim, Active)
        _c(Input, Dim, Disabled)
        _c(Input, Flat, Default)
        _c(Input, Flat, Hover)
        _c(Input, Flat, Pressed)
        _c(Input, Flat, Active)
        _c(Input, Flat, Disabled)
        #undef _c
    }

    /* LCOV_EXCL_START */
    CORRADE_ASSERT(false, "Ui::StyleConfiguration: unsupported text color index" << type << style << state, 0);
    CORRADE_ASSERT_UNREACHABLE();
    /* LCOV_EXCL_STOP */
}

namespace {

State stateForFlags(WidgetFlags flags) {
    if(flags & WidgetFlag::Hidden) return State::Hidden;
    if(flags & WidgetFlag::Disabled) return State::Disabled;
    if(flags & WidgetFlag::Pressed) return State::Pressed;
    if(flags & WidgetFlag::Active) return State::Active;
    if(flags & WidgetFlag::Hovered) return State::Hover;
    return State::Default;
}

}

UnsignedByte backgroundColorIndex(const Type type, const Style style, const WidgetFlags flags) {
    return backgroundColorIndex(type, style, stateForFlags(flags));
}

UnsignedByte foregroundColorIndex(const Type type, const Style style, const WidgetFlags flags) {
    return foregroundColorIndex(type, style, stateForFlags(flags));
}

UnsignedByte textColorIndex(const Type type, const Style style, const WidgetFlags flags) {
    return textColorIndex(type, style, stateForFlags(flags));
}

}

Debug& operator<<(Debug& debug, const Type value) {
    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case Type::value: return debug << "Ui::Type::" #value;
        _c(Button)
        _c(Input)
        _c(Label)
        _c(Modal)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "Ui::Type(" << Debug::nospace << reinterpret_cast<void*>(UnsignedInt(value)) << Debug::nospace << ")";
}

Debug& operator<<(Debug& debug, const State value) {
    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case State::value: return debug << "Ui::State::" #value;
        _c(Default)
        _c(Hover)
        _c(Pressed)
        _c(Active)
        _c(Disabled)
        _c(Hidden)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "Ui::State(" << Debug::nospace << reinterpret_cast<void*>(UnsignedInt(value)) << Debug::nospace << ")";
}

Debug& operator<<(Debug& debug, const Style value) {
    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case Style::value: return debug << "Ui::Style::" #value;
        _c(Default)
        _c(Primary)
        _c(Success)
        _c(Info)
        _c(Warning)
        _c(Danger)
        _c(Dim)
        _c(Flat)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "Ui::Style(" << Debug::nospace << reinterpret_cast<void*>(UnsignedInt(value)) << Debug::nospace << ")";
}

StyleConfiguration::StyleConfiguration() {
    /* Don't forget to update the shader code also */
    static_assert(sizeof(Background) == 4*4 + 4*4*Implementation::BackgroundColorCount, "Improper size of Background uniform structure");
    static_assert(sizeof(Foreground) == 4*4 + 4*4*Implementation::ForegroundColorCount*3, "Improper size of Foreground uniform structure");
    static_assert(sizeof(Text) == 4*4*Implementation::TextColorCount, "Improper size of Text uniform structure");
    static_assert(sizeof(Implementation::QuadVertex) == 6*4, "Improper size of QuadVertex vertex structure");
    static_assert(sizeof(Implementation::QuadInstance) == 4*4 + 4, "Improper size of QuadInstance vertex structure");
    static_assert(sizeof(Implementation::TextVertex) == 4*4 + 4, "Improper size of TextVertex vertex structure");
}

namespace {

inline Color4 premult(const Color4& color) {
    return {color.rgb()*color.a(), color.a()};
}

}

StyleConfiguration defaultStyleConfiguration() {
    using namespace Math::Literals;

    return Ui::StyleConfiguration{}
        .setFontSize(18.0f)
        .setMargin({10.0f, 7.0f})
        .setPadding({13.0f, 10.0f})
        .setCornerRadius(4.0f)
        .setCornerSmoothnessOut(0.125f)

        .setTopFillColor(   Ui::Type::Button, Ui::Style::Primary, Ui::State::Hover,    0x7891e6_rgbf)
        .setBottomFillColor(Ui::Type::Button, Ui::Style::Primary, Ui::State::Hover,    0x4165dc_rgbf)
        .setTextColor(      Ui::Type::Button, Ui::Style::Primary, Ui::State::Hover,    0xf0f3fc_rgbf)
        .setTopFillColor(   Ui::Type::Button, Ui::Style::Primary, Ui::State::Default,  0x284fd6_rgbf)
        .setBottomFillColor(Ui::Type::Button, Ui::Style::Primary, Ui::State::Default,  0x2040ac_rgbf)
        .setTextColor(      Ui::Type::Button, Ui::Style::Primary, Ui::State::Default,  0xd7dff8_rgbf)
        .setTopFillColor(   Ui::Type::Button, Ui::Style::Primary, Ui::State::Pressed,  0x132566_rgbf)
        .setBottomFillColor(Ui::Type::Button, Ui::Style::Primary, Ui::State::Pressed,  0x1a3085_rgbf)
        .setTextColor(      Ui::Type::Button, Ui::Style::Primary, Ui::State::Pressed,  0x4f72e0_rgbf)
        .setTopFillColor(   Ui::Type::Button, Ui::Style::Primary, Ui::State::Disabled, premult(0x7171714d_rgbaf))
        .setBottomFillColor(Ui::Type::Button, Ui::Style::Primary, Ui::State::Disabled, premult(0x7171714d_rgbaf))
        .setTextColor(      Ui::Type::Button, Ui::Style::Primary, Ui::State::Disabled, premult(0xf6f6f64d_rgbaf))

        .setTopFillColor(   Ui::Type::Button, Ui::Style::Danger, Ui::State::Hover,    0xe5685f_rgbf)
        .setBottomFillColor(Ui::Type::Button, Ui::Style::Danger, Ui::State::Hover,    0xdc3427_rgbf)
        .setTextColor(      Ui::Type::Button, Ui::Style::Danger, Ui::State::Hover,    0xfae3e1_rgbf)
        .setTopFillColor(   Ui::Type::Button, Ui::Style::Danger, Ui::State::Default,  0xbd2a1f_rgbf)
        .setBottomFillColor(Ui::Type::Button, Ui::Style::Danger, Ui::State::Default,  0x9a2219_rgbf)
        .setTextColor(      Ui::Type::Button, Ui::Style::Danger, Ui::State::Default,  0xf5c7c4_rgbf)
        .setTopFillColor(   Ui::Type::Button, Ui::Style::Danger, Ui::State::Pressed,  0x57130e_rgbf)
        .setBottomFillColor(Ui::Type::Button, Ui::Style::Danger, Ui::State::Pressed,  0x7e1d15_rgbf)
        .setTextColor(      Ui::Type::Button, Ui::Style::Danger, Ui::State::Pressed,  0xd93124_rgbf)
        .setTopFillColor(   Ui::Type::Button, Ui::Style::Danger, Ui::State::Disabled, premult(0x7171714d_rgbaf))
        .setBottomFillColor(Ui::Type::Button, Ui::Style::Danger, Ui::State::Disabled, premult(0x7171714d_rgbaf))
        .setTextColor(      Ui::Type::Button, Ui::Style::Danger, Ui::State::Disabled, premult(0xf6f6f64d_rgbaf))

        .setTopFillColor(   Ui::Type::Button, Ui::Style::Success, Ui::State::Hover,    0x36d134_rgbf)
        .setBottomFillColor(Ui::Type::Button, Ui::Style::Success, Ui::State::Hover,    0x27a425_rgbf)
        .setTextColor(      Ui::Type::Button, Ui::Style::Success, Ui::State::Hover,    0xd5f4d5_rgbf)
        .setTopFillColor(   Ui::Type::Button, Ui::Style::Success, Ui::State::Default,  0x1d791b_rgbf)
        .setBottomFillColor(Ui::Type::Button, Ui::Style::Success, Ui::State::Default,  0x186014_rgbf)
        .setTextColor(      Ui::Type::Button, Ui::Style::Success, Ui::State::Default,  0xc1eebf_rgbf)
        .setTopFillColor(   Ui::Type::Button, Ui::Style::Success, Ui::State::Pressed,  0x0f390b_rgbf)
        .setBottomFillColor(Ui::Type::Button, Ui::Style::Success, Ui::State::Pressed,  0x165914_rgbf)
        .setTextColor(      Ui::Type::Button, Ui::Style::Success, Ui::State::Pressed,  0x2a9327_rgbf)
        .setTopFillColor(   Ui::Type::Button, Ui::Style::Success, Ui::State::Disabled, premult(0x7171714d_rgbaf))
        .setBottomFillColor(Ui::Type::Button, Ui::Style::Success, Ui::State::Disabled, premult(0x7171714d_rgbaf))
        .setTextColor(      Ui::Type::Button, Ui::Style::Success, Ui::State::Disabled, premult(0xf6f6f64d_rgbaf))

        .setTopFillColor(   Ui::Type::Button, Ui::Style::Warning, Ui::State::Hover,    0xf3f098_rgbf)
        .setBottomFillColor(Ui::Type::Button, Ui::Style::Warning, Ui::State::Hover,    0xe7e243_rgbf)
        .setTextColor(      Ui::Type::Button, Ui::Style::Warning, Ui::State::Hover,    0xa1a117_rgbf)
        .setTopFillColor(   Ui::Type::Button, Ui::Style::Warning, Ui::State::Default,  0xdbd61b_rgbf)
        .setBottomFillColor(Ui::Type::Button, Ui::Style::Warning, Ui::State::Default,  0xb7b217_rgbf)
        .setTextColor(      Ui::Type::Button, Ui::Style::Warning, Ui::State::Default,  0x363608_rgbf)
        .setTopFillColor(   Ui::Type::Button, Ui::Style::Warning, Ui::State::Pressed,  0x56540b_rgbf)
        .setBottomFillColor(Ui::Type::Button, Ui::Style::Warning, Ui::State::Pressed,  0x7f7d0f_rgbf)
        .setTextColor(      Ui::Type::Button, Ui::Style::Warning, Ui::State::Pressed,  0x313107_rgbf)
        .setTopFillColor(   Ui::Type::Button, Ui::Style::Warning, Ui::State::Disabled, premult(0x7171714d_rgbaf))
        .setBottomFillColor(Ui::Type::Button, Ui::Style::Warning, Ui::State::Disabled, premult(0x7171714d_rgbaf))
        .setTextColor(      Ui::Type::Button, Ui::Style::Warning, Ui::State::Disabled, premult(0xf6f6f64d_rgbaf))

        .setTextColor(Ui::Type::Button, Ui::Style::Flat, Ui::State::Hover,    0xd0d9f7_rgbf)
        .setTextColor(Ui::Type::Button, Ui::Style::Flat, Ui::State::Default,  0x8fa5ec_rgbf)
        .setTextColor(Ui::Type::Button, Ui::Style::Flat, Ui::State::Pressed,  0x224ccd_rgbf)
        .setTextColor(Ui::Type::Button, Ui::Style::Flat, Ui::State::Disabled, premult(0xa6a6a64d_rgbaf))

        .setTopFillColor(   Ui::Type::Button, Ui::Style::Default, Ui::State::Hover,    0xf9f9f9_rgbf)
        .setBottomFillColor(Ui::Type::Button, Ui::Style::Default, Ui::State::Hover,    0xd9d9d9_rgbf)
        .setTextColor(      Ui::Type::Button, Ui::Style::Default, Ui::State::Hover,    0x9e9e9e_rgbf)
        .setTopFillColor(   Ui::Type::Button, Ui::Style::Default, Ui::State::Default,  0xe9e9e9_rgbf)
        .setBottomFillColor(Ui::Type::Button, Ui::Style::Default, Ui::State::Default,  0xbfbfbf_rgbf)
        .setTextColor(      Ui::Type::Button, Ui::Style::Default, Ui::State::Default,  0x1c1c1c_rgbf)
        .setTopFillColor(   Ui::Type::Button, Ui::Style::Default, Ui::State::Pressed,  0x717171_rgbf)
        .setBottomFillColor(Ui::Type::Button, Ui::Style::Default, Ui::State::Pressed,  0x8e8e8e_rgbf)
        .setTextColor(      Ui::Type::Button, Ui::Style::Default, Ui::State::Pressed,  0x313131_rgbf)
        .setTopFillColor(   Ui::Type::Button, Ui::Style::Default, Ui::State::Disabled, premult(0x4a4a4a4d_rgbaf))
        .setBottomFillColor(Ui::Type::Button, Ui::Style::Default, Ui::State::Disabled, premult(0x4a4a4a4d_rgbaf))
        .setTextColor(      Ui::Type::Button, Ui::Style::Default, Ui::State::Disabled, premult(0xafafaf4d_rgbaf))

        .setTextColor(Ui::Type::Label, Ui::Style::Primary, Ui::State::Default,  0x284fd6_rgbf)
        .setTextColor(Ui::Type::Label, Ui::Style::Primary, Ui::State::Disabled, premult(0xb0b0b04d_rgbaf))

        .setTextColor(Ui::Type::Label, Ui::Style::Danger,  Ui::State::Default,  0xbd2a1f_rgbf)
        .setTextColor(Ui::Type::Label, Ui::Style::Danger,  Ui::State::Disabled, premult(0xb0b0b04d_rgbaf))

        .setTextColor(Ui::Type::Label, Ui::Style::Success, Ui::State::Default,  0x1d791b_rgbf)
        .setTextColor(Ui::Type::Label, Ui::Style::Success, Ui::State::Disabled, premult(0xb0b0b04d_rgbaf))

        .setTextColor(Ui::Type::Label, Ui::Style::Warning, Ui::State::Default,  0xdbd61b_rgbf)
        .setTextColor(Ui::Type::Label, Ui::Style::Warning, Ui::State::Disabled, premult(0xb0b0b04d_rgbaf))

        .setTextColor(Ui::Type::Label, Ui::Style::Info,    Ui::State::Default,  0x7c94e7_rgbf)
        .setTextColor(Ui::Type::Label, Ui::Style::Info,    Ui::State::Disabled, premult(0xb0b0b04d_rgbaf))

        .setTextColor(Ui::Type::Label, Ui::Style::Default, Ui::State::Default,  0xe7e7e7_rgbf)
        .setTextColor(Ui::Type::Label, Ui::Style::Default, Ui::State::Disabled, premult(0x6363634d_rgbaf))

        .setTextColor(Ui::Type::Label, Ui::Style::Dim,     Ui::State::Default,  0x939393_rgbf)
        .setTextColor(Ui::Type::Label, Ui::Style::Dim,     Ui::State::Disabled, premult(0x4242424d_rgbaf))

        .setTopFillColor(   Ui::Type::Input, Ui::Style::Default, Ui::State::Hover,    0xd9d9d9_rgbf)
        .setBottomFillColor(Ui::Type::Input, Ui::Style::Default, Ui::State::Hover,    0xf9f9f9_rgbf)
        .setTextColor(      Ui::Type::Input, Ui::Style::Default, Ui::State::Hover,    0x777777_rgbf)
        .setTopFillColor(   Ui::Type::Input, Ui::Style::Default, Ui::State::Default,  0xbfbfbf_rgbf)
        .setBottomFillColor(Ui::Type::Input, Ui::Style::Default, Ui::State::Default,  0xe9e9e9_rgbf)
        .setTextColor(      Ui::Type::Input, Ui::Style::Default, Ui::State::Default,  0x1c1c1c_rgbf)
        .setTopFillColor(   Ui::Type::Input, Ui::Style::Default, Ui::State::Pressed,  0x717171_rgbf)
        .setBottomFillColor(Ui::Type::Input, Ui::Style::Default, Ui::State::Pressed,  0x8e8e8e_rgbf)
        .setTextColor(      Ui::Type::Input, Ui::Style::Default, Ui::State::Pressed,  0x313131_rgbf)
        .setTopFillColor(   Ui::Type::Input, Ui::Style::Default, Ui::State::Active,   0x909090_rgbf)
        .setBottomFillColor(Ui::Type::Input, Ui::Style::Default, Ui::State::Active,   0xbdbdbd_rgbf)
        .setTextColor(      Ui::Type::Input, Ui::Style::Default, Ui::State::Active,   0x242424_rgbf)
        .setTopFillColor(   Ui::Type::Input, Ui::Style::Default, Ui::State::Disabled, premult(0x4a4a4a4d_rgbaf))
        .setBottomFillColor(Ui::Type::Input, Ui::Style::Default, Ui::State::Disabled, premult(0x4a4a4a4d_rgbaf))
        .setTextColor(      Ui::Type::Input, Ui::Style::Default, Ui::State::Disabled, premult(0xafafaf4d_rgbaf))

        .setTopFillColor(   Ui::Type::Input, Ui::Style::Danger, Ui::State::Hover,    0xd9d9d9_rgbf)
        .setBottomFillColor(Ui::Type::Input, Ui::Style::Danger, Ui::State::Hover,    0xf9f9f9_rgbf)
        .setTextColor(      Ui::Type::Input, Ui::Style::Danger, Ui::State::Hover,    0xe87f75_rgbf)
        .setTopFillColor(   Ui::Type::Input, Ui::Style::Danger, Ui::State::Default,  0xbfbfbf_rgbf)
        .setBottomFillColor(Ui::Type::Input, Ui::Style::Danger, Ui::State::Default,  0xe9e9e9_rgbf)
        .setTextColor(      Ui::Type::Input, Ui::Style::Danger, Ui::State::Default,  0xa4261b_rgbf)
        .setTopFillColor(   Ui::Type::Input, Ui::Style::Danger, Ui::State::Pressed,  0x717171_rgbf)
        .setBottomFillColor(Ui::Type::Input, Ui::Style::Danger, Ui::State::Pressed,  0x8e8e8e_rgbf)
        .setTextColor(      Ui::Type::Input, Ui::Style::Danger, Ui::State::Pressed,  0xf4c2be_rgbf)
        .setTopFillColor(   Ui::Type::Input, Ui::Style::Danger, Ui::State::Active,   0xea8884_rgbf)
        .setBottomFillColor(Ui::Type::Input, Ui::Style::Danger, Ui::State::Active,   0xf2bab6_rgbf)
        .setTextColor(      Ui::Type::Input, Ui::Style::Danger, Ui::State::Active,   0x7e1d15_rgbf)
        .setTopFillColor(   Ui::Type::Input, Ui::Style::Danger, Ui::State::Disabled, premult(0x4a4a4a4d_rgbaf))
        .setBottomFillColor(Ui::Type::Input, Ui::Style::Danger, Ui::State::Disabled, premult(0x4a4a4a4d_rgbaf))
        .setTextColor(      Ui::Type::Input, Ui::Style::Danger, Ui::State::Disabled, premult(0xafafaf4d_rgbaf))

        .setTopFillColor(   Ui::Type::Input, Ui::Style::Success, Ui::State::Hover,    0xd9d9d9_rgbf)
        .setBottomFillColor(Ui::Type::Input, Ui::Style::Success, Ui::State::Hover,    0xf9f9f9_rgbf)
        .setTextColor(      Ui::Type::Input, Ui::Style::Success, Ui::State::Hover,    0x61db5d_rgbf)
        .setTopFillColor(   Ui::Type::Input, Ui::Style::Success, Ui::State::Default,  0xbfbfbf_rgbf)
        .setBottomFillColor(Ui::Type::Input, Ui::Style::Success, Ui::State::Default,  0xe9e9e9_rgbf)
        .setTextColor(      Ui::Type::Input, Ui::Style::Success, Ui::State::Default,  0x21871e_rgbf)
        .setTopFillColor(   Ui::Type::Input, Ui::Style::Success, Ui::State::Pressed,  0x717171_rgbf)
        .setBottomFillColor(Ui::Type::Input, Ui::Style::Success, Ui::State::Pressed,  0x8e8e8e_rgbf)
        .setTextColor(      Ui::Type::Input, Ui::Style::Success, Ui::State::Pressed,  0x77e074_rgbf)
        .setTopFillColor(   Ui::Type::Input, Ui::Style::Success, Ui::State::Active,   0x8ae781_rgbf)
        .setBottomFillColor(Ui::Type::Input, Ui::Style::Success, Ui::State::Active,   0xc7f2c6_rgbf)
        .setTextColor(      Ui::Type::Input, Ui::Style::Success, Ui::State::Active,   0x165914_rgbf)
        .setTopFillColor(   Ui::Type::Input, Ui::Style::Success, Ui::State::Disabled, premult(0x4a4a4a4d_rgbaf))
        .setBottomFillColor(Ui::Type::Input, Ui::Style::Success, Ui::State::Disabled, premult(0x4a4a4a4d_rgbaf))
        .setTextColor(      Ui::Type::Input, Ui::Style::Success, Ui::State::Disabled, premult(0xafafaf4d_rgbaf))

        .setTopFillColor(   Ui::Type::Input, Ui::Style::Warning, Ui::State::Hover,    0xd9d9d9_rgbf)
        .setBottomFillColor(Ui::Type::Input, Ui::Style::Warning, Ui::State::Hover,    0xf9f9f9_rgbf)
        .setTextColor(      Ui::Type::Input, Ui::Style::Warning, Ui::State::Hover,    0xcec818_rgbf)
        .setTopFillColor(   Ui::Type::Input, Ui::Style::Warning, Ui::State::Default,  0xbfbfbf_rgbf)
        .setBottomFillColor(Ui::Type::Input, Ui::Style::Warning, Ui::State::Default,  0xe9e9e9_rgbf)
        .setTextColor(      Ui::Type::Input, Ui::Style::Warning, Ui::State::Default,  0x827e0f_rgbf)
        .setTopFillColor(   Ui::Type::Input, Ui::Style::Warning, Ui::State::Pressed,  0x717171_rgbf)
        .setBottomFillColor(Ui::Type::Input, Ui::Style::Warning, Ui::State::Pressed,  0x8e8e8e_rgbf)
        .setTextColor(      Ui::Type::Input, Ui::Style::Warning, Ui::State::Pressed,  0xd1cd19_rgbf)
        .setTopFillColor(   Ui::Type::Input, Ui::Style::Warning, Ui::State::Active,   0xf0ec88_rgbf)
        .setBottomFillColor(Ui::Type::Input, Ui::Style::Warning, Ui::State::Active,   0xfaf9d2_rgbf)
        .setTextColor(      Ui::Type::Input, Ui::Style::Warning, Ui::State::Active,   0x7f7d0f_rgbf)
        .setTopFillColor(   Ui::Type::Input, Ui::Style::Warning, Ui::State::Disabled, premult(0x4a4a4a4d_rgbaf))
        .setBottomFillColor(Ui::Type::Input, Ui::Style::Warning, Ui::State::Disabled, premult(0x4a4a4a4d_rgbaf))
        .setTextColor(      Ui::Type::Input, Ui::Style::Warning, Ui::State::Disabled, premult(0xafafaf4d_rgbaf))

        .setTextColor(Ui::Type::Input, Ui::Style::Flat, Ui::State::Hover,    0xd0d9f7_rgbf)
        .setTextColor(Ui::Type::Input, Ui::Style::Flat, Ui::State::Default,  0x8fa5ec_rgbf)
        .setTextColor(Ui::Type::Input, Ui::Style::Flat, Ui::State::Pressed,  0x224ccd_rgbf)
        .setTextColor(Ui::Type::Input, Ui::Style::Flat, Ui::State::Active,   0x456ce0_rgbf)
        .setTextColor(Ui::Type::Input, Ui::Style::Flat, Ui::State::Disabled, premult(0xa6a6a64d_rgbaf))

        .setBackgroundColor(Ui::Type::Modal, Ui::Style::Dim, Ui::State::Default, premult(0x00000099_rgbaf))
        .setBackgroundColor(Ui::Type::Modal, Ui::Style::Default, Ui::State::Default, premult(0x1f1f1fe5_rgbaf))
        .setBackgroundColor(Ui::Type::Modal, Ui::Style::Danger, Ui::State::Default, premult(0x411713e5_rgbaf))
        .setBackgroundColor(Ui::Type::Modal, Ui::Style::Success, Ui::State::Default, premult(0x102c10e5_rgbaf))
        .setBackgroundColor(Ui::Type::Modal, Ui::Style::Warning, Ui::State::Default, premult(0x323112e5_rgbaf))
        .setBackgroundColor(Ui::Type::Modal, Ui::Style::Info, Ui::State::Default, premult(0x10172fe5_rgbaf));
}

void StyleConfiguration::pack(Buffer& backgroundUniforms, Buffer& foregroundUniforms, Buffer& textUniforms) const {
    backgroundUniforms.setData({&_background, 1}, BufferUsage::StaticDraw);
    foregroundUniforms.setData({&_foreground, 1}, BufferUsage::StaticDraw);
    textUniforms.setData({&_text, 1}, BufferUsage::StaticDraw);
}

namespace Implementation {

AbstractQuadShader& AbstractQuadShader::bindCornerTexture(Texture2D& texture) {
    texture.bind(0);
    return *this;
}

BackgroundShader::BackgroundShader() {
    #ifdef MAGNUM_BUILD_STATIC
    if(!Utility::Resource::hasGroup("MagnumUi"))
        importShaderResources();
    #endif

    Utility::Resource rs{"MagnumUi"};

    Shader vert{
        #ifndef MAGNUM_TARGET_GLES
        Version::GL330,
        #else
        Version::GLES300,
        #endif
        Shader::Type::Vertex};
    Shader frag{
        #ifndef MAGNUM_TARGET_GLES
        Version::GL330,
        #else
        Version::GLES300,
        #endif
        Shader::Type::Fragment};
    #ifndef CORRADE_TARGET_ANDROID
    vert.addSource("#define BACKGROUND_COLOR_COUNT " + std::to_string(Implementation::BackgroundColorCount) + "\n");
    frag.addSource("#define BACKGROUND_COLOR_COUNT " + std::to_string(Implementation::BackgroundColorCount) + "\n");
    #else
    std::ostringstream out;
    out << Implementation::BackgroundColorCount;
    vert.addSource("#define BACKGROUND_COLOR_COUNT " + out.str() + "\n");
    frag.addSource("#define BACKGROUND_COLOR_COUNT " + out.str() + "\n");
    #endif
    vert.addSource(rs.get("BackgroundShader.vert"));
    frag.addSource(rs.get("BackgroundShader.frag"));

    CORRADE_INTERNAL_ASSERT(Shader::compile({vert, frag}));
    attachShaders({vert, frag});

    CORRADE_INTERNAL_ASSERT(link());

    setUniform(uniformLocation("cornerTextureData"), 0);
    setUniformBlockBinding(uniformBlockIndex("Style"), 0);
    _transformationProjectionMatrixUniform = uniformLocation("transformationProjectionMatrix");
}

BackgroundShader& BackgroundShader::bindStyleBuffer(Buffer& buffer) {
    buffer.bind(Buffer::Target::Uniform, 0);
    return *this;
}

ForegroundShader::ForegroundShader() {
    #ifdef MAGNUM_BUILD_STATIC
    if(!Utility::Resource::hasGroup("MagnumUi"))
        importShaderResources();
    #endif

    Utility::Resource rs{"MagnumUi"};

    Shader vert{
        #ifndef MAGNUM_TARGET_GLES
        Version::GL330,
        #else
        Version::GLES300,
        #endif
        Shader::Type::Vertex};
    Shader frag{
        #ifndef MAGNUM_TARGET_GLES
        Version::GL330,
        #else
        Version::GLES300,
        #endif
        Shader::Type::Fragment};
    #ifndef CORRADE_TARGET_ANDROID
    vert.addSource("#define FOREGROUND_COLOR_COUNT " + std::to_string(Implementation::ForegroundColorCount) + "\n");
    frag.addSource("#define FOREGROUND_COLOR_COUNT " + std::to_string(Implementation::ForegroundColorCount) + "\n");
    #else
    std::ostringstream out;
    out << Implementation::ForegroundColorCount;
    vert.addSource("#define FOREGROUND_COLOR_COUNT " + out.str() + "\n");
    frag.addSource("#define FOREGROUND_COLOR_COUNT " + out.str() + "\n");
    #endif
    vert.addSource(rs.get("ForegroundShader.vert"));
    frag.addSource(rs.get("ForegroundShader.frag"));

    CORRADE_INTERNAL_ASSERT(Shader::compile({vert, frag}));
    attachShaders({vert, frag});

    CORRADE_INTERNAL_ASSERT(link());

    setUniform(uniformLocation("cornerTextureData"), 0);
    setUniformBlockBinding(uniformBlockIndex("Style"), 1);
    _transformationProjectionMatrixUniform = uniformLocation("transformationProjectionMatrix");
}

ForegroundShader& ForegroundShader::bindStyleBuffer(Buffer& buffer) {
    buffer.bind(Buffer::Target::Uniform, 1);
    return *this;
}

TextShader::TextShader() {
    #ifdef MAGNUM_BUILD_STATIC
    if(!Utility::Resource::hasGroup("MagnumUi"))
        importShaderResources();
    #endif

    Utility::Resource rs{"MagnumUi"};

    Shader vert{
        #ifndef MAGNUM_TARGET_GLES
        Version::GL330,
        #else
        Version::GLES300,
        #endif
        Shader::Type::Vertex};
    Shader frag{
        #ifndef MAGNUM_TARGET_GLES
        Version::GL330,
        #else
        Version::GLES300,
        #endif
        Shader::Type::Fragment};
    vert.addSource(rs.get("TextShader.vert"));
    #ifndef CORRADE_TARGET_ANDROID
    frag.addSource("#define TEXT_COLOR_COUNT " + std::to_string(Implementation::TextColorCount) + "\n");
    #else
    std::ostringstream out;
    out << Implementation::TextColorCount;
    frag.addSource("#define TEXT_COLOR_COUNT " + out.str() + "\n");
    #endif
    frag.addSource(rs.get("TextShader.frag"));

    CORRADE_INTERNAL_ASSERT(Shader::compile({vert, frag}));
    attachShaders({vert, frag});

    CORRADE_INTERNAL_ASSERT(link());

    setUniformBlockBinding(uniformBlockIndex("Style"), 2);
    _transformationProjectionMatrixUniform = uniformLocation("transformationProjectionMatrix");
    setUniform(uniformLocation("textureData"), 1);
}

TextShader& TextShader::bindGlyphCacheTexture(Texture2D& texture) {
    texture.bind(1);
    return *this;
}

TextShader& TextShader::bindStyleBuffer(Buffer& buffer) {
    buffer.bind(Buffer::Target::Uniform, 2);
    return *this;
}

}}}
