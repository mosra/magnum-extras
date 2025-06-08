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

#include <Corrade/TestSuite/Tester.h>
#include <Magnum/Math/Vector2.h>

#include "Magnum/Ui/UserInterfaceGL.h"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct UserInterfaceGL_Test: TestSuite::Tester {
    explicit UserInterfaceGL_Test();

    void constructNoCreate();
};

UserInterfaceGL_Test::UserInterfaceGL_Test() {
    addTests({&UserInterfaceGL_Test::constructNoCreate});
}

void UserInterfaceGL_Test::constructNoCreate() {
    UserInterfaceGL ui{NoCreate};
    CORRADE_COMPARE(ui.size(), Vector2{});
    CORRADE_COMPARE(ui.windowSize(), Vector2{});
    CORRADE_COMPARE(ui.framebufferSize(), Vector2i{});

    /* Doesn't add any renderer or layers at all, not even ones without GL */
    CORRADE_VERIFY(!ui.hasRendererInstance());
    CORRADE_COMPARE(ui.layerCapacity(), 0);
    CORRADE_COMPARE(ui.layerUsedCount(), 0);
    CORRADE_VERIFY(!ui.hasBaseLayer());
    CORRADE_VERIFY(!ui.hasTextLayer());
    CORRADE_VERIFY(!ui.hasEventLayer());
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::UserInterfaceGL_Test)
