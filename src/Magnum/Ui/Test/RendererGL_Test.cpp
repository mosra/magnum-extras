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

#include <sstream> /** @todo remove once Debug is stream-free */
#include <Corrade/Containers/StringStl.h> /** @todo remove once Debug is stream-free */
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/String.h>
#include <Corrade/Utility/DebugStl.h> /** @todo remove once Debug is stream-free */

#include "Magnum/Ui/RendererGL.h"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct RendererGL_Test: TestSuite::Tester {
    explicit RendererGL_Test();

    void debugFlag();
    void debugFlags();

    void construct();

    void compositingFramebufferTextureNotEnabled();
};

RendererGL_Test::RendererGL_Test() {
    addTests({&RendererGL_Test::debugFlag,
              &RendererGL_Test::debugFlags,

              &RendererGL_Test::construct,

              &RendererGL_Test::compositingFramebufferTextureNotEnabled});
}

void RendererGL_Test::debugFlag() {
    std::ostringstream out;
    Debug{&out} << RendererGL::Flag::CompositingFramebuffer << RendererGL::Flag(0xbe);
    CORRADE_COMPARE(out.str(), "Ui::RendererGL::Flag::CompositingFramebuffer Ui::RendererGL::Flag(0xbe)\n");
}

void RendererGL_Test::debugFlags() {
    std::ostringstream out;
    Debug{&out} << (RendererGL::Flag::CompositingFramebuffer|RendererGL::Flag(0xb0)) << RendererGL::Flags{};
    CORRADE_COMPARE(out.str(), "Ui::RendererGL::Flag::CompositingFramebuffer|Ui::RendererGL::Flag(0xb0) Ui::RendererGL::Flags{}\n");
}

void RendererGL_Test::construct() {
    RendererGL renderer;

    /* It shouldn't require a GL context on construction or destruction */
    CORRADE_COMPARE(renderer.currentDrawStates(), RendererDrawStates{});
}

void RendererGL_Test::compositingFramebufferTextureNotEnabled() {
    CORRADE_SKIP_IF_NO_ASSERT();

    RendererGL renderer;
    const RendererGL& crenderer = renderer;

    std::ostringstream out;
    Error redirectError{&out};
    renderer.compositingFramebuffer();
    crenderer.compositingFramebuffer();
    renderer.compositingTexture();
    crenderer.compositingTexture();
    CORRADE_COMPARE_AS(out.str(),
        "Ui::RendererGL::compositingFramebuffer(): compositing framebuffer not enabled\n"
        "Ui::RendererGL::compositingFramebuffer(): compositing framebuffer not enabled\n"
        "Ui::RendererGL::compositingTexture(): compositing framebuffer not enabled\n"
        "Ui::RendererGL::compositingTexture(): compositing framebuffer not enabled\n",
        TestSuite::Compare::String);
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::RendererGL_Test)
