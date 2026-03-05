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

#include <Corrade/Containers/Function.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/PluginManager/Manager.h>
#include <Magnum/Image.h>
#include <Magnum/ImageView.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/TimeStl.h>
#include <Magnum/Platform/Sdl2Application.h>
#ifdef MAGNUM_TARGET_EGL
#include <Magnum/Platform/WindowlessEglApplication.h>
#elif defined(CORRADE_TARGET_IOS)
#include <Magnum/Platform/WindowlessIosApplication.h>
#elif defined(CORRADE_TARGET_APPLE)
#include <Magnum/Platform/WindowlessCglApplication.h>
#elif defined(CORRADE_TARGET_UNIX)
#include <Magnum/Platform/WindowlessGlxApplication.h>
#elif defined(CORRADE_TARGET_WINDOWS)
#include <Magnum/Platform/WindowlessWglApplication.h>
#else
#error no windowless application available on this platform
#endif
#include <Magnum/Trade/AbstractImageConverter.h>

#include "Magnum/Ui/Anchor.h"
#include "Magnum/Ui/Button.h"
#include "Magnum/Ui/Label.h"
#include "Magnum/Ui/Input.h"
#include "Magnum/Ui/Panel.h"
#include "Magnum/Ui/RendererGL.h"
#include "Magnum/Ui/SnapLayout.h"
#include "Magnum/Ui/Theme.h"
#include "Magnum/Ui/UserInterfaceGL.h"

#define DOXYGEN_ELLIPSIS(...) __VA_ARGS__
#define DOXYGEN_IGNORE(...) __VA_ARGS__

using namespace Magnum;
using namespace Math::Literals;

namespace {

/* The includes are already above so they should do nothing. This whole
   application also isn't used for rendering anything, it's just for tutorial
   code. */
namespace A {
/* [button] */
DOXYGEN_ELLIPSIS()
#include <Magnum/Ui/Anchor.h>
#include <Magnum/Ui/Button.h>

class MyApplication: public Platform::Application {
    DOXYGEN_ELLIPSIS(public:
        explicit MyApplication(const Arguments& arguments);)
    private:
        Ui::UserInterfaceGL _ui;
        Ui::Button _button;
};

DOXYGEN_IGNORE(CORRADE_UNUSED )MyApplication::MyApplication(const Arguments& arguments):
    DOXYGEN_ELLIPSIS(Platform::Application{arguments}, _ui{NoCreate},)
    _button{Ui::Anchor{_ui, (_ui.size() - Vector2{100, 40})*0.5f, {100, 40}},
        "Hello!", Ui::ButtonStyle::Success}
{
    DOXYGEN_ELLIPSIS()
/* [button] */

/* [button-trigger] */
#include <Corrade/Containers/Function.h>

DOXYGEN_ELLIPSIS()

    _button.onTrigger([this] {
        _button.setStyle(_button.style() == Ui::ButtonStyle::Success ?
            Ui::ButtonStyle::Danger : Ui::ButtonStyle::Success);
    });
}
/* [button-trigger] */
}

namespace B {

/* [button-nocreate] */
class MyApplication: public Platform::Application {
    DOXYGEN_ELLIPSIS(public:
        explicit MyApplication(const Arguments& arguments);)
    private:
        Ui::UserInterfaceGL _ui;
        Ui::Button _button{NoCreate};
};

DOXYGEN_IGNORE(CORRADE_UNUSED )MyApplication::MyApplication(const Arguments& arguments): DOXYGEN_ELLIPSIS(Platform::Application{arguments}, _ui{NoCreate}) {
    DOXYGEN_ELLIPSIS()

    _button = Ui::Button{
        Ui::Anchor{_ui, (_ui.size() - Vector2{100, 40})*0.5f, {100, 40}},
        "Hello!", Ui::ButtonStyle::Success};
}
/* [button-nocreate] */

}

namespace C {

class MyApplication: public Platform::Application {
    public:
        explicit MyApplication(const Arguments& arguments);
    private:
        Ui::UserInterfaceGL _ui;
        Ui::Button _button{NoCreate};
};

/* The include is already used above, so this does nothing */
/* [button-layout] */
#include <Magnum/Ui/SnapLayout.h>

DOXYGEN_ELLIPSIS()

DOXYGEN_IGNORE(CORRADE_UNUSED )MyApplication::MyApplication(const Arguments& arguments): DOXYGEN_ELLIPSIS(Platform::Application{arguments}, _ui{NoCreate}) {
    DOXYGEN_ELLIPSIS()

    _button = Ui::Button{
        Ui::SnapLayout::snapRoot(_ui, Ui::Snaps{}),
        "Hello!", Ui::ButtonStyle::Success};
}
/* [button-layout] */

}

constexpr Vector2i ImageSize{660, 600};

struct UiGettingStarted: Platform::WindowlessApplication {
    explicit UiGettingStarted(const Arguments& arguments): Platform::WindowlessApplication{arguments} {}

    int exec() override;
};

/** @todo ffs, duplicated seven times, make a batch utility in Magnum */
Image2D unpremultiply(Image2D image) {
    for(Containers::StridedArrayView1D<Color4ub> row: image.pixels<Color4ub>())
        for(Color4ub& pixel: row)
            pixel = pixel.unpremultiplied();

    return image;
}

int UiGettingStarted::exec() {
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::One, GL::Renderer::BlendFunction::OneMinusSourceAlpha);

    PluginManager::Manager<Trade::AbstractImageConverter> converterManager;
    Containers::Pointer<Trade::AbstractImageConverter> converter = converterManager.loadAndInstantiate("AnyImageConverter");
    if(!converter)
        return 1;

    /* It's named `_ui` to match the private member name from earlier
       snippets */
    Ui::UserInterfaceGL _ui{NoCreate};
    /* Using a compositing framebuffer because it's easier than setting up a
       custom framebuffer here */
    _ui
        /** @todo uh, can't setting renderer flags be doable in some more
            intuitive way? such as flags on the style? */
        .setRendererInstance(Containers::pointer<Ui::RendererGL>(Ui::RendererGL::Flag::CompositingFramebuffer))
        /* The actual framebuffer size is 2x the UI size */
        .setSize(Vector2{ImageSize}*0.5f, Vector2{ImageSize}, ImageSize)
        .setTheme(Ui::DarkTheme{});

    /* The same button as in the B::MyApplication above, keep in sync */
    {
        Ui::Button button{
            Ui::Anchor{_ui, (_ui.size() - Vector2{100, 40})*0.5f, {100, 40}},
            "Hello!", Ui::ButtonStyle::Success};

        _ui.renderer().compositingFramebuffer().clearColor(0, 0x00000000_rgbaf);
        _ui.draw();
        converter->convertToFile(unpremultiply(_ui.renderer().compositingFramebuffer().read(Range2Di::fromSize((ImageSize - Vector2i{220, 100})/2, {220, 100}), {PixelFormat::RGBA8Unorm})), "ui-getting-started-button.png");

    /* The same button as in the C::MyApplication above, keep in sync */
    } {
        Ui::Button button{
            Ui::SnapLayout::snapRoot(_ui, Ui::Snaps{}),
            "Hello!", Ui::ButtonStyle::Success};

        _ui.renderer().compositingFramebuffer().clearColor(0, 0x00000000_rgbaf);
        _ui.draw();
        converter->convertToFile(unpremultiply(_ui.renderer().compositingFramebuffer().read(Range2Di::fromSize((ImageSize - Vector2i{200, 100})/2, {200, 100}), {PixelFormat::RGBA8Unorm})), "ui-getting-started-button-layout.png");
    }

    {
        /* The includes are already used above, so this does nothing. Brace
           yourself for the cryptic macro-ridden code however. */
/* [login-members] */
#include <Magnum/Ui/Input.h>
#include <Magnum/Ui/Label.h>

class MyApplication: public Platform::Application {
    DOXYGEN_ELLIPSIS(}; struct Stuff {)
    private:
        DOXYGEN_ELLIPSIS()
        Ui::Label _titleLabel{NoCreate},
            _usernameLabel{NoCreate},
            _passwordLabel{NoCreate};
        Ui::Input _username{NoCreate};
        Ui::PasswordInput _password{NoCreate};
        Ui::Button _signIn{NoCreate};
DOXYGEN_IGNORE(struct Foo {)};
/* [login-members] */
        public:
            Ui::NodeHandle root;
            Stuff(Ui::UserInterface& _ui) {
/* [login] */
Ui::SnapLayoutColumn layout = Ui::SnapLayout::snapRoot(_ui, Ui::Snaps{});

_titleLabel = Ui::Label{layout.child(), "Login required", Ui::LabelStyle::Title};

_usernameLabel = Ui::Label{layout.child(), "Username:"};
_username = Ui::Input{layout.child({200, 0})};

_passwordLabel = Ui::Label{layout.child(), "Password:"};
_password = Ui::PasswordInput{layout.child({200, 0})};

_signIn = Ui::Button{layout.child(), "Sign in", Ui::ButtonStyle::Primary};
_signIn.onTrigger([this] {
    /* For a lack of a better idea let's just leak the password to stdout :) */
    Debug{} << "Signing in with" << _username.text() << "and" << _password.text();
});
/* [login] */
                root = layout;
            }
        } stuff{_ui};

        _ui.renderer().compositingFramebuffer().clearColor(0, 0x00000000_rgbaf);
        _ui.draw();
        converter->convertToFile(unpremultiply(_ui.renderer().compositingFramebuffer().read(Range2Di::fromSize((ImageSize - Vector2i{420, 520})/2, {420, 520}), {PixelFormat::RGBA8Unorm})), "ui-getting-started-login.png");

        /* To not have the whole thing leak to following screenshots */
        _ui.removeNode(stuff.root);
    }

    {
        Ui::Input _username{NoCreate}, _password{NoCreate};
/* [login-stateless] */
Ui::SnapLayoutColumn layout = Ui::SnapLayout::snapRoot(_ui, Ui::Snaps{});
Ui::label(layout.child(), "Login required", Ui::LabelStyle::Title);

Ui::label(layout.child(), "Username:");
_username = Ui::Input{layout.child({200, 0})};

Ui::label(layout.child(), "Password:");
_password = Ui::PasswordInput{layout.child({200, 0})};

Ui::button(layout.child(), "Sign in", [this] {
    DOXYGEN_ELLIPSIS(exec(); /* just to avoid warning with unused capture */)
}, Ui::ButtonStyle::Primary);
/* [login-stateless] */

        /* To not have the whole thing leak to following screenshots */
        _ui.removeNode(layout);
    }

    {
        Ui::Input _username{NoCreate}, _password{NoCreate};
/* [login-container] */
Ui::Panel panel{Ui::NonOwned, Ui::SnapLayout::snapRoot(_ui, Ui::Snaps{}),
    Ui::PanelStyle::Filled};
Ui::SnapLayoutColumn layout = panel.contents();
Ui::label(layout.child(), "Login required", Ui::LabelStyle::Title);

Ui::label(layout.child(), "Username:");
_username = Ui::Input{layout.child({200, 0})};

Ui::label(layout.child(), "Password:");
_password = Ui::PasswordInput{layout.child({200, 0})};

Ui::button(layout.child(), "Sign in", [this] {
    DOXYGEN_ELLIPSIS(exec(); /* just to avoid warning with unused capture */)
}, Ui::ButtonStyle::Primary);
/* [login-container] */

        _ui.renderer().compositingFramebuffer().clearColor(0, 0x00000000_rgbaf);
        _ui.draw();
        converter->convertToFile(unpremultiply(_ui.renderer().compositingFramebuffer().read(Range2Di::fromSize((ImageSize - Vector2i{480, 550})/2, {480, 550}), {PixelFormat::RGBA8Unorm})), "ui-getting-started-login-container.png");

        /* To not have the whole thing leak to following screenshots */
        _ui.removeNode(panel);
    }

    {
        Ui::Input _username{NoCreate}, _password{NoCreate};
Ui::Panel panel{Ui::NonOwned, Ui::SnapLayout::snapRoot(_ui, Ui::Snaps{}),
    Ui::PanelStyle::Filled};
/* [login-compact] */
Ui::SnapLayoutColumnRight layout = panel.contents();
Ui::label(layout.child(), "Login required", Ui::LabelStyle::Title);

Ui::SnapLayoutRow usernameLayout = layout.child();
Ui::label(usernameLayout.child(), "Username:");
_username = Ui::Input{usernameLayout.child({200, 0})};

Ui::SnapLayoutRow passwordLayout = layout.child();
Ui::label(passwordLayout.child(), "Password:");
_password = Ui::PasswordInput{passwordLayout.child({200, 0})};

Ui::button(layout.child(), "Sign in", [this] {
    DOXYGEN_ELLIPSIS(exec(); /* just to avoid warning with unused capture */)
}, Ui::ButtonStyle::Primary);
/* [login-compact] */

        _ui.renderer().compositingFramebuffer().clearColor(0, 0x00000000_rgbaf);
        _ui.draw();
        converter->convertToFile(unpremultiply(_ui.renderer().compositingFramebuffer().read(Range2Di::fromSize((ImageSize - Vector2i{660, 420})/2, {660, 420}), {PixelFormat::RGBA8Unorm})), "ui-getting-started-login-compact.png");

        /* To not have the whole thing leak to following screenshots */
        _ui.removeNode(panel);
    }

    return 0;
}

}

MAGNUM_WINDOWLESSAPPLICATION_MAIN(UiGettingStarted)
