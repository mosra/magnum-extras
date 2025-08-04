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
#include <Magnum/Math/Functions.h>

#include "Magnum/Ui/NodeAnimator.h"
#include "Magnum/Ui/NodeFlags.h"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct NodeAnimatorCpp14Test: TestSuite::Tester {
    explicit NodeAnimatorCpp14Test();

    void animationSettersConstexpr();
};

NodeAnimatorCpp14Test::NodeAnimatorCpp14Test() {
    addTests({&NodeAnimatorCpp14Test::animationSettersConstexpr});
}

void NodeAnimatorCpp14Test::animationSettersConstexpr() {
    /* Mostly just a copy of NodeAnimatorTest verifying that all setters can
       be used in a C++14 context. Getters are not constexpr to avoid a header
       dependency on Pair, just hasRemoveNodeAfter() is. */

    /* Keep some unset to verify that it can stay partially unset as well */
    constexpr NodeAnimation a = NodeAnimation{}
        .fromOffsetY(1.0f)
        .toOffsetX(2.0f)
        .fromSizeX(7.0f)
        .toSizeY(8.0f)
        .fromOpacity(0.25f)
        .clearFlagsBegin(NodeFlag::Disabled|NodeFlag::Focusable)
        .addFlagsEnd(NodeFlag::Hidden|NodeFlag::Disabled)
        .setRemoveNodeAfter(false);
    /* NaN comparison works only for scalars */
    CORRADE_COMPARE(a.offsets().first().x(), Constants::nan());
    CORRADE_COMPARE(a.offsets().first().y(), 1.0f);
    CORRADE_COMPARE(a.offsets().second().x(), 2.0f);
    CORRADE_COMPARE(a.offsets().second().y(), Constants::nan());
    CORRADE_COMPARE(a.sizes().first().x(), 7.0f);
    CORRADE_COMPARE(a.sizes().first().y(), Constants::nan());
    CORRADE_COMPARE(a.sizes().second().x(), Constants::nan());
    CORRADE_COMPARE(a.sizes().second().y(), 8.0f);
    CORRADE_COMPARE(a.opacities().first(), 0.25f);
    CORRADE_COMPARE(a.opacities().second(), Constants::nan());
    CORRADE_COMPARE(a.flagsAdd(), Containers::pair(NodeFlags{}, NodeFlag::Hidden|NodeFlag::Disabled));
    CORRADE_COMPARE(a.flagsClear(), Containers::pair(NodeFlag::Disabled|NodeFlag::Focusable, NodeFlags{}));
    constexpr bool aHasRemoveNodeAfter = a.hasRemoveNodeAfter();
    CORRADE_COMPARE(aHasRemoveNodeAfter, false);

    constexpr NodeAnimation b = NodeAnimation{}
        .fromOffsetX(3.0f)
        .toOffsetY(4.0f)
        .fromSizeY(5.0f)
        .toSizeX(6.0f)
        .toOpacity(0.75f)
        .addFlagsBegin(NodeFlag::Clip|NodeFlag::NoEvents)
        .clearFlagsEnd(NodeFlag::FallthroughPointerEvents|NodeFlag::NoBlur)
        .setRemoveNodeAfter(true);
    /* NaN comparison works only for scalars */
    CORRADE_COMPARE(b.offsets().first().x(), 3.0f);
    CORRADE_COMPARE(b.offsets().first().y(), Constants::nan());
    CORRADE_COMPARE(b.offsets().second().x(), Constants::nan());
    CORRADE_COMPARE(b.offsets().second().y(), 4.0f);
    CORRADE_COMPARE(b.sizes().first().x(), Constants::nan());
    CORRADE_COMPARE(b.sizes().first().y(), 5.0f);
    CORRADE_COMPARE(b.sizes().second().x(), 6.0f);
    CORRADE_COMPARE(b.sizes().second().y(), Constants::nan());
    CORRADE_COMPARE(b.opacities().first(), Constants::nan());
    CORRADE_COMPARE(b.opacities().second(), 0.75f);
    CORRADE_COMPARE(b.flagsAdd(), Containers::pair(NodeFlag::Clip|NodeFlag::NoEvents, NodeFlags{}));
    CORRADE_COMPARE(b.flagsClear(), Containers::pair(NodeFlags{}, NodeFlag::FallthroughPointerEvents|NodeFlag::NoBlur));
    constexpr bool bHasRemoveNodeAfter = b.hasRemoveNodeAfter();
    CORRADE_COMPARE(bHasRemoveNodeAfter, true);

    /* The X and Y setters shouldn't overwrite the other component, behaving
       the same as setting both at once */
    constexpr NodeAnimation c1 = NodeAnimation{}
        .fromOffset({1.0f, 2.0f})
        .toSizeX(7.0f)
        .toSizeY(8.0f);
    constexpr NodeAnimation c2 = NodeAnimation{}
        .fromOffsetX(1.0f)
        .fromOffsetY(2.0f)
        .toSize({7.0f, 8.0f});
    CORRADE_COMPARE(c1.offsets().first(), (Vector2{1.0f, 2.0f}));
    CORRADE_COMPARE(c2.offsets().first(), (Vector2{1.0f, 2.0f}));
    CORRADE_COMPARE(Math::isNan(c1.offsets().second()), BitVector2{3});
    CORRADE_COMPARE(Math::isNan(c2.offsets().second()), BitVector2{3});
    CORRADE_COMPARE(Math::isNan(c1.sizes().first()), BitVector2{3});
    CORRADE_COMPARE(Math::isNan(c2.sizes().first()), BitVector2{3});
    CORRADE_COMPARE(c1.sizes().second(), (Vector2{7.0f, 8.0f}));
    CORRADE_COMPARE(c2.sizes().second(), (Vector2{7.0f, 8.0f}));

    /* Same for the other two */
    constexpr NodeAnimation d1 = NodeAnimation{}
        .toOffsetX(3.0f)
        .toOffsetY(4.0f)
        .fromSize({5.0f, 6.0f});
    constexpr NodeAnimation d2 = NodeAnimation{}
        .toOffset({3.0f, 4.0f})
        .fromSizeX(5.0f)
        .fromSizeY(6.0f);
    CORRADE_COMPARE(Math::isNan(d1.offsets().first()), BitVector2{3});
    CORRADE_COMPARE(Math::isNan(d2.offsets().first()), BitVector2{3});
    CORRADE_COMPARE(d1.offsets().second(), (Vector2{3.0f, 4.0f}));
    CORRADE_COMPARE(d2.offsets().second(), (Vector2{3.0f, 4.0f}));
    CORRADE_COMPARE(d1.sizes().first(), (Vector2{5.0f, 6.0f}));
    CORRADE_COMPARE(d2.sizes().first(), (Vector2{5.0f, 6.0f}));
    CORRADE_COMPARE(Math::isNan(d1.sizes().second()), BitVector2{3});
    CORRADE_COMPARE(Math::isNan(d2.sizes().second()), BitVector2{3});
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::NodeAnimatorCpp14Test)
