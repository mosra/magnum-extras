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
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/Utility/DebugStl.h> /** @todo remove once Debug is stream-free */

#include "Magnum/Whee/TextLayerGL.h"

namespace Magnum { namespace Whee { namespace Test { namespace {

struct TextLayerGL_Test: TestSuite::Tester {
    explicit TextLayerGL_Test();

    void sharedConstructNoCreate();
    void sharedConstructZeroStyleCount();
};

using namespace Math::Literals;

TextLayerGL_Test::TextLayerGL_Test() {
    addTests({&TextLayerGL_Test::sharedConstructNoCreate,
              &TextLayerGL_Test::sharedConstructZeroStyleCount});
}

void TextLayerGL_Test::sharedConstructNoCreate() {
    TextLayerGL::Shared shared{NoCreate};

    /* Shouldn't crash or try to acces GL */
    CORRADE_VERIFY(true);

    /* Implicit construction is not allowed */
    CORRADE_VERIFY(!std::is_convertible<NoCreateT, TextLayerGL::Shared>::value);
}

void TextLayerGL_Test::sharedConstructZeroStyleCount() {
    CORRADE_SKIP_IF_NO_ASSERT();

    std::ostringstream out;
    Error redirectError{&out};
    TextLayerGL::Shared{0, 4};
    TextLayerGL::Shared{4, 0};
    CORRADE_COMPARE(out.str(),
        "Whee::TextLayerGL::Shared: expected non-zero style uniform count\n"
        "Whee::TextLayerGL::Shared: expected non-zero style count\n");
}

}}}}

CORRADE_TEST_MAIN(Magnum::Whee::Test::TextLayerGL_Test)
