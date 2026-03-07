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

#include <algorithm> /* std::next_permutation() */
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/String.h>
#include <Corrade/TestSuite/Compare/Numeric.h>
#include <Corrade/Utility/Algorithms.h> /* Utility::copy() */

#include "Magnum/Ui/Anchor.h"
#include "Magnum/Ui/Event.h"
#include "Magnum/Ui/NodeFlags.h"
#include "Magnum/Ui/SnapLayout.h"
#include "Magnum/Ui/ScrollArea.h"
#include "Magnum/Ui/Theme.h"
#include "Magnum/Ui/Test/WidgetTester.hpp"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct ScrollAreaTest: WidgetTester {
    explicit ScrollAreaTest();

    void debugFlag();
    void debugFlags();

    void construct();
    void constructOnlyX();
    void constructOnlyY();
    void constructNonOwned();
    void constructNoCreate();
    void constructInvalid();

    template<ScrollAreaFlag flag = ScrollAreaFlag{}> void scrollX();
    template<ScrollAreaFlag flag = ScrollAreaFlag{}> void scrollY();

    void dragFallthrough();
    void widgetResized();

    template<UnsignedInt nodeCount, ScrollAreaFlag flag = ScrollAreaFlag{}> void scrollbarEventOrder();
};

const struct {
    const char* name;
    ScrollAreaFlags flags;
} ConstructData[]{
    {"", {}},
    {"no contents drag", ScrollAreaFlag::NoContentsDrag}
};

const struct {
    TestSuite::TestCaseDescriptionSourceLocation name;
    ScrollAreaFlags extraFlags;
    Float size, contentsSize;
    Containers::Optional<Float> initialPercentage;
    bool isInitialPercentageNoOp;
    /* Yeah. Exactly this syntax, of course. Yes. */
    NodeHandle(ScrollArea::*nodeX)() const, (ScrollArea::*nodeY)() const;
    bool scroll;
    /* For scroll is in wheel steps, which are multiplied by EventLayer
       scrollStepDistance(), where positive is up/right and negative
       down/left. For drag it's UI units. */
    Vector2 offset;
    bool isNoOp, isThumbHidden;
    Float expectedPercentage;
    Float expectedNodeOffset; /* Should be 0 or negative */
} ScrollData[]{
    {"scroll on the view to 100%",
        {}, 120.0f, 360.0f, {}, false,
        &ScrollArea::viewNode, &ScrollArea::viewNode,
        /* There's 240 units invisible, so scroll by 240 units to the right,
           which is 2.4 scroll steps. */
        true, {2.4f, 0.0f},
        false, false, 100.0f, -240.0f},
    /* These two should have no difference compared to above */
    {"scroll on the scrollbar to 100%",
        {}, 120.0f, 360.0f, {}, false,
        &ScrollArea::scrollbarXNode, &ScrollArea::scrollbarYNode,
        true, {2.4f, 0.0f},
        false, false, 100.0f, -240.0f},
    {"scroll on the thumb to 100%",
        {}, 120.0f, 360.0f, {}, false,
        &ScrollArea::scrollbarThumbXNode, &ScrollArea::scrollbarThumbYNode,
        true, {2.4f, 0.0f},
        false, false, 100.0f, -240.0f},
    {"drag on the view to 100%",
        {}, 120.0f, 360.0f, {}, false,
        &ScrollArea::viewNode, &ScrollArea::viewNode,
        /* Compared to a scroll (to the right) here we need to drag to the
           left, thus negative */
        false, {-240.0f, 0.0f},
        false, false, 100.0f, -240.0f},
    /* This one should have no difference compared to above */
    {"drag on the thumb to 100%",
        {}, 120.0f, 360.0f, {}, false,
        &ScrollArea::scrollbarThumbXNode, &ScrollArea::scrollbarThumbYNode,
        /* The visible portion is 1/3 of the size, so the thumb can freely move
           in 2/3 of the scrollbar size (which is 80 out of 120), thus moving
           by all that (to the right) makes it 100% */
        false, {80.0f, 0.0f},
        false, false, 100.0f, -240.0f},

    /* These are reverse of the above */
    {"at 100%, scroll on the view to 0%",
        {}, 120.0f, 360.0f, 100.0f, false,
        &ScrollArea::viewNode, &ScrollArea::viewNode,
        true, {-2.4f, 0.0f},
        false, false, 0.0f, 0.0f},
    {"at 100%, scroll on the scrollbar to 0%",
        {}, 120.0f, 360.0f, 100.0f, false,
        &ScrollArea::scrollbarXNode, &ScrollArea::scrollbarYNode,
        true, {-2.4f, 0.0f},
        false, false, 0.0f, 0.0f},
    {"at 100%, scroll on the thumb to 0%",
        {}, 120.0f, 360.0f, 100.0f, false,
        &ScrollArea::scrollbarThumbXNode, &ScrollArea::scrollbarThumbYNode,
        true, {-2.4f, 0.0f},
        false, false, 0.0f, 0.0f},
    {"at 100%, drag on the view to 0%",
        {}, 120.0f, 360.0f, 100.0f, false,
        &ScrollArea::viewNode, &ScrollArea::viewNode,
        false, {240.0f, 0.0f},
        false, false, 0.0f, 0.0f},
    {"at 100%, drag on the thumb to 0%",
        {}, 120.0f, 360.0f, 100.0f, false,
        &ScrollArea::scrollbarThumbXNode, &ScrollArea::scrollbarThumbYNode,
        false, {-80.0f, 0.0f},
        false, false, 0.0f, 0.0f},

    /* Like the 0% -> 100% case, just shifted by 20% */
    {"at 20%, scroll on the view to 120%",
        {}, 120.0f, 360.0f, 20.0f, false,
        &ScrollArea::viewNode, &ScrollArea::viewNode,
        true, {2.4f, 0.0f},
        false, false, 100.0f, -240.0f},
    {"at 20%, scroll on the scrollbar to 120%",
        {}, 120.0f, 360.0f, 20.0f, false,
        &ScrollArea::scrollbarXNode, &ScrollArea::scrollbarYNode,
        true, {2.4f, 0.0f},
        false, false, 100.0f, -240.0f},
    {"at 20%, scroll on the thumb to 120%",
        {}, 120.0f, 360.0f, 20.0f, false,
        &ScrollArea::scrollbarThumbXNode, &ScrollArea::scrollbarThumbYNode,
        true, {2.4f, 0.0f},
        false, false, 100.0f, -240.0f},
    {"at 20%, drag on the view to 120%",
        {}, 120.0f, 360.0f, 20.0f, false,
        &ScrollArea::viewNode, &ScrollArea::viewNode,
        false, {-240.0f, 0.0f},
        false, false, 100.0f, -240.0f},
    {"at 20%, drag on the thumb to 120%",
        {}, 120.0f, 360.0f, 20.0f, false,
        &ScrollArea::scrollbarThumbXNode, &ScrollArea::scrollbarThumbYNode,
        false, {80.0f, 0.0f},
        false, false, 100.0f, -240.0f},

    /* Reverse of the above */
    {"at 80%, scroll on the view to -20%",
        {}, 120.0f, 360.0f, 80.0f, false,
        &ScrollArea::viewNode, &ScrollArea::viewNode,
        true, {-2.4f, 0.0f},
        false, false, 0.0f, 0.0f},
    {"at 80%, scroll on the scrollbar to -20%",
        {}, 120.0f, 360.0f, 80.0f, false,
        &ScrollArea::scrollbarXNode, &ScrollArea::scrollbarYNode,
        true, {-2.4f, 0.0f},
        false, false, 0.0f, 0.0f},
    {"at 80%, scroll on the thumb to -20%",
        {}, 120.0f, 360.0f, 80.0f, false,
        &ScrollArea::scrollbarThumbXNode, &ScrollArea::scrollbarThumbYNode,
        true, {-2.4f, 0.0f},
        false, false, 0.0f, 0.0f},
    {"at 80%, drag on the view to -20%",
        {}, 120.0f, 360.0f, 80.0f, false,
        &ScrollArea::viewNode, &ScrollArea::viewNode,
        false, {240.0f, 0.0f},
        false, false, 0.0f, 0.0f},
    {"at 80%, drag on the thumb to -20%",
        {}, 120.0f, 360.0f, 80.0f, false,
        &ScrollArea::scrollbarThumbXNode, &ScrollArea::scrollbarThumbYNode,
        false, {-80.0f, 0.0f},
        false, false, 0.0f, 0.0f},

    /* Now just 40% of the way */
    {"scroll on the view from 30% to 70%",
        {}, 120.0f, 360.0f, 30.0f, false,
        &ScrollArea::viewNode, &ScrollArea::viewNode,
        true, {0.96f, 0.0f},
        false, false, 70.0f, -168.0f}, /* 70% of 240, yep, that's the value */
    {"scroll on the scrollbar from 30% to 70%",
        {}, 120.0f, 360.0f, 30.0f, false,
        &ScrollArea::scrollbarXNode, &ScrollArea::scrollbarYNode,
        true, {0.96f, 0.0f},
        false, false, 70.0f, -168.0f},
    {"scroll on the thumb from 30% to 70%",
        {}, 120.0f, 360.0f, 30.0f, false,
        &ScrollArea::scrollbarThumbXNode, &ScrollArea::scrollbarThumbYNode,
        true, {0.96f, 0.0f},
        false, false, 70.0f, -168.0f},
    {"drag on the view from 30% to 70%",
        {}, 120.0f, 360.0f, 30.0f, false,
        &ScrollArea::viewNode, &ScrollArea::viewNode,
        false, {-96.0f, 0.0f},
        false, false, 70.0f, -168.0f},
    {"drag on the thumb from 30% to 70%",
        {}, 120.0f, 360.0f, 30.0f, false,
        &ScrollArea::scrollbarThumbXNode, &ScrollArea::scrollbarThumbYNode,
        /* The visible portion is 1/3 of the size, so the thumb can freely move
           in 2/3 of the scrollbar size (which is 80 out of 120), and 40% of
           that is 32 */
        false, {32.0f, 0.0f},
        false, false, 70.0f, -168.0f},

    /* Reverse of the above */
    {"scroll on the view from 70% to 30%",
        {}, 120.0f, 360.0f, 70.0f, false,
        &ScrollArea::viewNode, &ScrollArea::viewNode,
        true, {-0.96f, 0.0f},
        false, false, 30.0f, -72.0f}, /* 30% of 240, yep, that's the value */
    {"scroll on the scrollbar from 70% to 30%",
        {}, 120.0f, 360.0f, 70.0f, false,
        &ScrollArea::scrollbarXNode, &ScrollArea::scrollbarYNode,
        true, {-0.96f, 0.0f},
        false, false, 30.0f, -72.0f},
    {"scroll on the thumb from 70% to 30%",
        {}, 120.0f, 360.0f, 70.0f, false,
        &ScrollArea::scrollbarThumbXNode, &ScrollArea::scrollbarThumbYNode,
        true, {-0.96f, 0.0f},
        false, false, 30.0f, -72.0f},
    {"drag on the view from 70% to 30%",
        {}, 120.0f, 360.0f, 70.0f, false,
        &ScrollArea::viewNode, &ScrollArea::viewNode,
        false, {96.0f, 0.0f},
        false, false, 30.0f, -72.0f},
    {"drag on the thumb from 70% to 30%",
        {}, 120.0f, 360.0f, 70.0f, false,
        &ScrollArea::scrollbarThumbXNode, &ScrollArea::scrollbarThumbYNode,
        false, {-32.0f, 0.0f},
        false, false, 30.0f, -72.0f},

    /* No-op initial percentage, followed by nothing */
    {"no-op initial percentage",
        {}, 120.0f, 360.0f, 0.0f, true,
        nullptr, nullptr,
        true, {},
        true, false, 0.0f, 0.0f},

    /* No-op scroll in one direction */
    {"scroll on the view from 0% left/top",
        {}, 120.0f, 360.0f, {}, false,
        &ScrollArea::viewNode, &ScrollArea::viewNode,
        true, {-1.0f, 0.0f},
        true, false, 0.0f, 0.0f},
    {"scroll on the scrollbar from 0% left/top",
        {}, 120.0f, 360.0f, {}, false,
        &ScrollArea::scrollbarXNode, &ScrollArea::scrollbarYNode,
        true, {-1.0f, 0.0f},
        true, false, 0.0f, 0.0f},
    {"scroll on the thumb from 0% left/top",
        {}, 120.0f, 360.0f, {}, false,
        &ScrollArea::scrollbarThumbXNode, &ScrollArea::scrollbarThumbYNode,
        true, {-1.0f, 0.0f},
        true, false, 0.0f, 0.0f},
    {"drag on the view from 0% left/top",
        {}, 120.0f, 360.0f, {}, false,
        &ScrollArea::viewNode, &ScrollArea::viewNode,
        false, {50.0f, 0.0f},
        true, false, 0.0f, 0.0f},
    {"drag on the thumb from 0% left/top",
        {}, 120.0f, 360.0f, {}, false,
        &ScrollArea::scrollbarThumbXNode, &ScrollArea::scrollbarThumbYNode,
        false, {-50.0f, 0.0f},
        true, false, 0.0f, 0.0f},

    /* No-op scroll in another direction */
    {"scroll on the view from 100% right/bottom",
        {}, 120.0f, 360.0f, 100.0f, false,
        &ScrollArea::viewNode, &ScrollArea::viewNode,
        true, {1.0f, 0.0f},
        true, false, 100.0f, -240.0f},
    {"scroll on the scrollbar from 100% right/bottom",
        {}, 120.0f, 360.0f, 100.0f, false,
        &ScrollArea::scrollbarXNode, &ScrollArea::scrollbarYNode,
        true, {1.0f, 0.0f},
        true, false, 100.0f, -240.0f},
    {"scroll on the thumb from 100% right/bottom",
        {}, 120.0f, 360.0f, 100.0f, false,
        &ScrollArea::scrollbarThumbXNode, &ScrollArea::scrollbarThumbYNode,
        true, {1.0f, 0.0f},
        true, false, 100.0f, -240.0f},
    {"drag on the view from 100% right/bottom",
        {}, 120.0f, 360.0f, 100.0f, false,
        &ScrollArea::viewNode, &ScrollArea::viewNode,
        false, {-50.0f, 0.0f},
        true, false, 100.0f, -240.0f},
    {"drag on the thumb from 100% right/bottom",
        {}, 120.0f, 360.0f, 100.0f, false,
        &ScrollArea::scrollbarThumbXNode, &ScrollArea::scrollbarThumbYNode,
        false, {50.0f, 0.0f},
        true, false, 100.0f, -240.0f},

    /* Contents same size than the view, should do nothing for all and have the
       thumb hidden */
    {"contents same as view, no-op initial percentage",
        {}, 120.0f, 120.0f, 56.0f, true,
        nullptr, nullptr,
        true, {},
        true, true, 0.0f, 0.0f},

    {"contents same as view, small contents, scroll on the view right/bottom",
        {}, 120.0f, 120.0f, {}, false,
        &ScrollArea::viewNode, &ScrollArea::viewNode,
        true, {1.0f, 0.0f},
        true, true, 0.0f, 0.0f},
    {"contents same as view, scroll on the scrollbar right/bottom",
        {}, 120.0f, 120.0f, {}, false,
        &ScrollArea::scrollbarXNode, &ScrollArea::scrollbarYNode,
        true, {1.0f, 0.0f},
        true, true, 0.0f, 0.0f},
    /* No scroll on the thumb as the thumb is invisible */
    {"contents same as view, drag on the view right/bottom",
        {}, 120.0f, 120.0f, {}, false,
        &ScrollArea::viewNode, &ScrollArea::viewNode,
        false, {-50.0f, 0.0f},
        true, true, 0.0f, 0.0f},
    /* No drag on the thumb as the thumb is invisible */

    {"contents same as view, small contents, scroll on the view left/top",
        {}, 120.0f, 120.0f, {}, false,
        &ScrollArea::viewNode, &ScrollArea::viewNode,
        true, {-1.0f, 0.0f},
        true, true, 0.0f, 0.0f},
    {"contents same as view, scroll on the scrollbar left/top",
        {}, 120.0f, 120.0f, {}, false,
        &ScrollArea::scrollbarXNode, &ScrollArea::scrollbarYNode,
        true, {-1.0f, 0.0f},
        true, true, 0.0f, 0.0f},
    /* No scroll on the thumb as the thumb is invisible */
    {"contents same as view, drag on the view left/top",
        {}, 120.0f, 120.0f, {}, false,
        &ScrollArea::viewNode, &ScrollArea::viewNode,
        false, {50.0f, 0.0f},
        true, true, 0.0f, 0.0f},
    /* No drag on the thumb as the thumb is invisible */

    /* Contents smaller than the view, should do nothing for all and have the
       thumb hidden */
    {"small contents, no-op initial percentage",
        {}, 120.0f, 60.0f, 56.0f, true,
        nullptr, nullptr,
        true, {},
        true, true, 0.0f, 0.0f},

    {"small contents, small contents, scroll on the view right/bottom",
        {}, 120.0f, 60.0f, {}, false,
        &ScrollArea::viewNode, &ScrollArea::viewNode,
        true, {1.0f, 0.0f},
        true, true, 0.0f, 0.0f},
    {"small contents, scroll on the scrollbar right/bottom",
        {}, 120.0f, 60.0f, {}, false,
        &ScrollArea::scrollbarXNode, &ScrollArea::scrollbarYNode,
        true, {1.0f, 0.0f},
        true, true, 0.0f, 0.0f},
    /* No scroll on the thumb as the thumb is invisible */
    {"small contents, drag on the view right/bottom",
        {}, 120.0f, 60.0f, {}, false,
        &ScrollArea::viewNode, &ScrollArea::viewNode,
        false, {-50.0f, 0.0f},
        true, true, 0.0f, 0.0f},
    /* No drag on the thumb as the thumb is invisible */

    {"small contents, small contents, scroll on the view left/top",
        {}, 120.0f, 60.0f, {}, false,
        &ScrollArea::viewNode, &ScrollArea::viewNode,
        true, {-1.0f, 0.0f},
        true, true, 0.0f, 0.0f},
    {"small contents, scroll on the scrollbar left/top",
        {}, 120.0f, 60.0f, {}, false,
        &ScrollArea::scrollbarXNode, &ScrollArea::scrollbarYNode,
        true, {-1.0f, 0.0f},
        true, true, 0.0f, 0.0f},
    /* No scroll on the thumb as the thumb is invisible */
    {"small contents, drag on the view left/top",
        {}, 120.0f, 60.0f, {}, false,
        &ScrollArea::viewNode, &ScrollArea::viewNode,
        false, {50.0f, 0.0f},
        true, true, 0.0f, 0.0f},
    /* No drag on the thumb as the thumb is invisible */

    /* While horizontal/vertical scrolling on the view is limited to the
       allowed direction, it should be possible to use a (vertical) wheel on a
       horizontal scrollbar and vice versa. For details see the ASCII art above
       the directionalScrollOffset() helper in ScrollArea.cpp. */
    {"scroll on the scrollbar, orthogonal, 30% to 70%",
        {}, 120.0f, 360.0f, 30.0f, false,
        &ScrollArea::scrollbarXNode, &ScrollArea::scrollbarYNode,
        true, {0.0f, -0.96f}, /* Negative Y scrolls down */
        false, false, 70.0f, -168.0f},
    {"scroll on the scrollbar, orthogonal,70% to 30%",
        {}, 120.0f, 360.0f, 70.0f, false,
        &ScrollArea::scrollbarXNode, &ScrollArea::scrollbarYNode,
        true, {0.0f, 0.96f}, /* Positive Y scrolls up */
        false, false, 30.0f, -72.0f},
    {"scroll on the scrollbar, up left, 70% to 30%",
        {}, 120.0f, 360.0f, 70.0f, false,
        &ScrollArea::scrollbarXNode, &ScrollArea::scrollbarYNode,
        /* Above the 135° angle in up left quadrant; negative X scrolls left */
        true, Vector2{-0.5f, 1.0f}.resized(0.96f),
        false, false, 30.0f, -72.0f},
    {"scroll on the scrollbar, up left, 70% to 30%, again",
        {}, 120.0f, 360.0f, 70.0f, false,
        &ScrollArea::scrollbarXNode, &ScrollArea::scrollbarYNode,
        /* Below the 135° angle in up left quadrant, still going up */
        true, Vector2{-1.0f, 0.5f}.resized(0.96f),
        false, false, 30.0f, -72.0f},
    {"scroll on the scrollbar, up right, 30% to 70%",
        {}, 120.0f, 360.0f, 30.0f, false,
        &ScrollArea::scrollbarXNode, &ScrollArea::scrollbarYNode,
        /* Below the 45° angle in up right quadrant */
        true, Vector2{1.0f, 0.5f}.resized(0.96f),
        false, false, 70.0f, -168.0f},
    {"scroll on the scrollbar up right, 70% to 30%",
        {}, 120.0f, 360.0f, 70.0f, false,
        &ScrollArea::scrollbarXNode, &ScrollArea::scrollbarYNode,
        /* Above the 45° angle in up right quadrant */
        true, Vector2{0.5f, 1.0f}.resized(0.96f),
        false, false, 30.0f, -72.0f},
    {"scroll on the scrollbar, bottom left, 30% to 70%",
        {}, 120.0f, 360.0f, 30.0f, false,
        &ScrollArea::scrollbarXNode, &ScrollArea::scrollbarYNode,
        /* Below the 225° angle in bottom left quadrant */
        true, Vector2{-0.5f, -1.0f}.resized(0.96f),
        false, false, 70.0f, -168.0f},
    {"scroll on the scrollbar, bottom left, 70% to 30%",
        {}, 120.0f, 360.0f, 70.0f, false,
        &ScrollArea::scrollbarXNode, &ScrollArea::scrollbarYNode,
        /* Above the 225° angle in bottom left quadrant */
        true, Vector2{-1.0f, -0.5f}.resized(0.96f),
        false, false, 30.0f, -72.0f},
    {"scroll on the scrollbar, bottom right, 30% to 70%",
        {}, 120.0f, 360.0f, 30.0f, false,
        &ScrollArea::scrollbarXNode, &ScrollArea::scrollbarYNode,
        /* Below the 135° angle in bottom right quadrant */
        true, Vector2{1.0f, -0.5f}.resized(0.96f),
        false, false, 70.0f, -168.0f},
    {"scroll on the scrollbar, bottom right, 30% to 70%, again",
        {}, 120.0f, 360.0f, 30.0f, false,
        &ScrollArea::scrollbarXNode, &ScrollArea::scrollbarYNode,
        /* Above the 315° angle in bottom right quadrant, still going down */
        true, Vector2{1.0f, -0.5f}.resized(0.96f),
        false, false, 70.0f, -168.0f},

    /* These should expand to a size that is still functional for all events.
       Testing only scrolling back to 0% as for the other direction I'd have to
       hardcode *some* style property into the test to figure out what the
       contents offset is at 100% */
    {"zero size, at 100%, scroll on the view to before 0%",
        {}, 0.0f, 360.0f, 100.0f, false,
        &ScrollArea::viewNode, &ScrollArea::viewNode,
        true, {-4.0f, 0.0f},
        false, false, 0.0f, 0.0f},
    {"zero size, at 100%, scroll on the scrollbar to before 0%",
        {}, 0.0f, 360.0f, 100.0f, false,
        &ScrollArea::scrollbarXNode, &ScrollArea::scrollbarYNode,
        true, {-4.0f, 0.0f},
        false, false, 0.0f, 0.0f},
    {"zero size, at 100%, scroll on the thumb to before 0%",
        {}, 0.0f, 360.0f, 100.0f, false,
        &ScrollArea::scrollbarThumbXNode, &ScrollArea::scrollbarThumbYNode,
        true, {-4.0f, 0.0f},
        false, false, 0.0f, 0.0f},
    {"zero size, at 100%, drag on the view to before 0%",
        {}, 0.0f, 360.0f, 100.0f, false,
        &ScrollArea::viewNode, &ScrollArea::viewNode,
        false, {400.0f, 0.0f},
        false, false, 0.0f, 0.0f},
    {"zero size, at 100%, drag on the thumb to before 0%",
        {}, 0.0f, 360.0f, 100.0f, false,
        &ScrollArea::scrollbarThumbXNode, &ScrollArea::scrollbarThumbYNode,
        false, {-40.0f, 0.0f},
        false, false, 0.0f, 0.0f},

    /* NoContentsDrag enabled doesn't affect scroll but makes drag a no-op */
    {"no contents drag, scroll on the view from 30% to 70%",
        ScrollAreaFlag::NoContentsDrag, 120.0f, 360.0f, 30.0f, false,
        &ScrollArea::viewNode, &ScrollArea::viewNode,
        true, {0.96f, 0.0f},
        false, false, 70.0f, -168.0f},
    {"no contents drag, scroll on the view from 70% to 30%",
        ScrollAreaFlag::NoContentsDrag, 120.0f, 360.0f, 70.0f, false,
        &ScrollArea::viewNode, &ScrollArea::viewNode,
        true, {-0.96f, 0.0f},
        false, false, 30.0f, -72.0f},
    {"no contents drag, drag on the view from 30% to 70%",
        ScrollAreaFlag::NoContentsDrag, 120.0f, 360.0f, 30.0f, false,
        &ScrollArea::viewNode, &ScrollArea::viewNode,
        false, {-96.0f, 0.0f},
        true, false, 30.0f, -72.0f},
    {"no contents drag, drag on the view from 70% to 30%",
        ScrollAreaFlag::NoContentsDrag, 120.0f, 360.0f, 70.0f, false,
        &ScrollArea::viewNode, &ScrollArea::viewNode,
        false, {96.0f, 0.0f},
        true, false, 70.0f, -168.0f},
};

const struct {
    const char* name;
    ScrollAreaFlags flags;
    Vector2 offset;
    bool expectDrag;
} DragFallthroughData[]{
    {"",
        {}, {20.0f, 20.0f}, true},
    {"only X",
        ScrollAreaFlag::OnlyX, {30.0f, 0.0f}, true},
    {"only Y",
        ScrollAreaFlag::OnlyY, {0.0f, 30.0f}, true},
    {"too short",
        {}, {5.0f, 5.0f}, false},
    {"too short, only X",
        ScrollAreaFlag::OnlyX, {8.0f, 0.0f}, false},
    {"too short, only Y",
        ScrollAreaFlag::OnlyY, {0.0f, 8.0f}, false},
    {"long enough but view drag disabled",
        ScrollAreaFlag::NoContentsDrag, {20.0f, 20.0f}, false},
    {"long enough but view drag disabled, only X",
        ScrollAreaFlag::NoContentsDrag|ScrollAreaFlag::OnlyX, {30.0f, 0.0f}, false},
    {"long enough but view drag disabled, only Y",
        ScrollAreaFlag::NoContentsDrag|ScrollAreaFlag::OnlyY, {0.0f, 30.0f}, false},
};

const struct {
    const char* name;
    ScrollAreaFlags flags;
    /* Original size is {1000, 1000}, contents size {2000, 2000} so the
       original node offset is 10 times the percentage */
    Vector2 scrollPercentage, newSize, expectedScrollPercentage;
} WidgetResizedData[]{
    {"not scrolled",
        {}, {}, {1500.0f, 1500.0f}, {}},
    {"not scrolled, only X",
        ScrollAreaFlag::OnlyX, {}, {1500.0f, 0.0f}, {}},
    {"not scrolled, only Y",
        ScrollAreaFlag::OnlyY, {}, {0.0f, 1500.0f}, {}},
    {"not scrolled, not scrollable in new size",
        {}, {}, {2000.0f, 2000.0f}, {}},
    {"not scrolled, not scrollable in new size, only X",
        ScrollAreaFlag::OnlyX, {}, {2000.0f, 0.0f}, {}},
    {"not scrolled, not scrollable in new size, only Y",
        ScrollAreaFlag::OnlyY, {}, {0.0f, 2000.0f}, {}},
    {"not scrolled, new size smaller",
        {}, {}, {500.0f, 500.0f}, {}},
    {"not scrolled, new size smaller, only X",
        ScrollAreaFlag::OnlyX, {}, {500.0f, 0.0f}, {}},
    {"not scrolled, new size smaller, only Y",
        ScrollAreaFlag::OnlyY, {}, {0.0f, 500.0f}, {}},

    {"scrolled past end in new size",
        /* The original node offset is {600, 700}, after the resize it's
           clamped to {500, 500} which is 100% and 100% */
        {}, {60.0f, 70.0f}, {1500.0f, 1500.0f}, {100.0f, 100.0f}},
    {"scrolled past end in new size, only X",
        ScrollAreaFlag::OnlyX, {60.0f, 0.0f}, {1500.0f, 0.0f}, {100.0f, 0.0f}},
    {"scrolled past end in new size, only Y",
        ScrollAreaFlag::OnlyY, {0.0f, 70.0f}, {0.0f, 1500.0f}, {0.0f, 100.0f}},

    {"not scrollable in new size",
        {}, {30.0f, 20.0f}, {2000.0f, 2000.0f}, {}},
    {"not scrollable in new size, only X",
        ScrollAreaFlag::OnlyX, {30.0f, 0.0f}, {2000.0f, 0.0f}, {}},
    {"not scrollable in new size, only Y",
        ScrollAreaFlag::OnlyY, {0.0f, 20.0f}, {0.0f, 2000.0f}, {}},

    {"new size smaller",
        /* The original node offset is {600, 400}, after the resize it's 40%
           and 20% of the new scrollable distance which is {1500, 1500} */
        {}, {60.0f, 30.0f}, {500.0f, 500.0f}, {40.0f, 20.0f}},
    {"new size smaller, only X",
        ScrollAreaFlag::OnlyX, {60.0f, 0.0f}, {500.0f, 0.0f}, {40.0f, 0.0f}},
    {"new size smaller, only Y",
        ScrollAreaFlag::OnlyY, {0.0f, 30.0f}, {0.0f, 500.0f}, {0.0f, 20.0f}}
};

ScrollAreaTest::ScrollAreaTest() {
    addTests({&ScrollAreaTest::debugFlag,
              &ScrollAreaTest::debugFlags});

    addInstancedTests<ScrollAreaTest>({
        &ScrollAreaTest::construct,
        &ScrollAreaTest::constructOnlyX,
        &ScrollAreaTest::constructOnlyY
    }, Containers::arraySize(ConstructData),
       &WidgetTester::setup,
       &WidgetTester::teardown);

    addTests<ScrollAreaTest>({&ScrollAreaTest::constructNonOwned},
        &WidgetTester::setup,
        &WidgetTester::teardown);

    addTests<ScrollAreaTest>({&ScrollAreaTest::constructNoCreate},
        &WidgetTester::setupNoCreate,
        &WidgetTester::teardownNoCreate);

    addTests<ScrollAreaTest>({&ScrollAreaTest::constructInvalid},
        &WidgetTester::setup,
        &WidgetTester::teardown);

    addInstancedTests<ScrollAreaTest>({
        &ScrollAreaTest::scrollX<ScrollAreaFlag::OnlyX>,
        &ScrollAreaTest::scrollX,
        &ScrollAreaTest::scrollY<ScrollAreaFlag::OnlyY>,
        &ScrollAreaTest::scrollY
    }, Containers::arraySize(ScrollData),
       &WidgetTester::setup,
       &WidgetTester::teardown);

    addInstancedTests<ScrollAreaTest>({&ScrollAreaTest::dragFallthrough},
        Containers::arraySize(DragFallthroughData),
        &WidgetTester::setup,
        &WidgetTester::teardown);

    addInstancedTests<ScrollAreaTest>({&ScrollAreaTest::widgetResized},
        Containers::arraySize(WidgetResizedData),
        &WidgetTester::setup,
        &WidgetTester::teardown);

    /* Expects the scroll area has at most 7 nodes, which gives 7! possible
       orders. No setup/teardown as it uses its own UI every time. The variants
       with X / Y scroll disabled each have two nodes less, and thus 5!
       orders. */
    addRepeatedTests<ScrollAreaTest>({&ScrollAreaTest::scrollbarEventOrder<7>},
        7*6*5*4*3*2);
    addRepeatedTests<ScrollAreaTest>({&ScrollAreaTest::scrollbarEventOrder<5, ScrollAreaFlag::OnlyX>},
        5*4*3*2);
    addRepeatedTests<ScrollAreaTest>({&ScrollAreaTest::scrollbarEventOrder<5, ScrollAreaFlag::OnlyY>},
        5*4*3*2);

    /* Need the LayoutLayer populated with actual real paddings and min sizes
       as otherwise most inner nodes would have zero sizes, being impossible to
       fire events on */
    /** @todo move to WidgetTester once this is needed by more widgets */
    CORRADE_INTERNAL_ASSERT_OUTPUT(DarkTheme{}.apply(ui, ThemeFeature::LayoutLayer, {}, {}));
}

void ScrollAreaTest::debugFlag() {
    Containers::String out;
    Debug{&out} << ScrollAreaFlag::OnlyX << ScrollAreaFlag(0xef);
    CORRADE_COMPARE(out, "Ui::ScrollAreaFlag::OnlyX Ui::ScrollAreaFlag(0xef)\n");
}

void ScrollAreaTest::debugFlags() {
    Containers::String out;
    Debug{&out} << (ScrollAreaFlag::OnlyX|ScrollAreaFlag::OnlyY|ScrollAreaFlag(0x80)) << ScrollAreaFlags{};
    CORRADE_COMPARE(out, "Ui::ScrollAreaFlag::OnlyX|Ui::ScrollAreaFlag::OnlyY|Ui::ScrollAreaFlag(0x80) Ui::ScrollAreaFlags{}\n");
}

void ScrollAreaTest::construct() {
    auto&& data = ConstructData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* The flags may contain ScrollAreaFlag::NoContentsDrag, but it should
       result in no difference in registered events and such, as the contents
       still react to scroll in that case */
    ScrollArea scroll{Anchor{root, {}, {32, 16}}, data.flags};
    CORRADE_COMPARE(ui.nodeParent(scroll), root);
    CORRADE_COMPARE(ui.nodeSize(scroll), (Vector2{32, 16}));
    CORRADE_VERIFY(scroll.isOwned());

    CORRADE_COMPARE(scroll.flags(), data.flags);
    CORRADE_COMPARE(scroll.scrollPercentage(), Vector2{});
    CORRADE_COMPARE(&scroll.contents().ui(), &ui);
    CORRADE_COMPARE(scroll.contents().node(), scroll.contentsNode());

    /* By default both scrollbars are present */
    CORRADE_VERIFY(ui.isHandleValid(scroll.viewNode()));
    CORRADE_VERIFY(ui.isHandleValid(scroll.contentsNode()));
    CORRADE_VERIFY(ui.isHandleValid(scroll.scrollbarXNode()));
    CORRADE_VERIFY(ui.isHandleValid(scroll.scrollbarThumbXNode()));
    CORRADE_VERIFY(ui.isHandleValid(scroll.scrollbarYNode()));
    CORRADE_VERIFY(ui.isHandleValid(scroll.scrollbarThumbYNode()));
    CORRADE_VERIFY(ui.dataLayer().isHandleValid(scroll.scrollXStorage()));
    CORRADE_VERIFY(ui.dataLayer().isHandleValid(scroll.scrollYStorage()));

    /* Can only verify that the data & layouts were created, they're not
       saved */
    CORRADE_COMPARE(ui.dataLayer().usedCount(), 2);
    /* One for each scrollbar and thumb */
    CORRADE_COMPARE(ui.baseLayer().usedCount(), 2*2);
    /* One for the scroll area itself, one for the view, one for each scrollbar
       and thumb */
    CORRADE_COMPARE(ui.layoutLayer().usedCount(), 2 + 2*2);
    /* One for scroll on the view, one for scroll on each scrollbar and one for
       drag on each thumb */
    CORRADE_COMPARE(ui.eventLayer().usedCount(), 1 + 2*2);
    /* One for the scroll area, one for the view, one for contents, one for
       each scrollbar and thumb */
    CORRADE_COMPARE(ui.snapLayouter().usedCount(), 3 + 2*2);
    /* One for each thumb, one for the view */
    CORRADE_COMPARE(ui.genericLayouter().usedCount(), 2 + 1);

    /* Verify also that we're not allocating anything by accident */
    CORRADE_COMPARE(ui.dataLayer().storageUsedAllocatedCount(), 0);
    /** @todo once MSVC 2017 support is dropped, construct all callbacks with
        NoAllocatedInit to catch these at compile time */
    #ifndef CORRADE_MSVC2017_COMPATIBILITY
    CORRADE_COMPARE(ui.dataLayer().usedAllocatedCount(), 0);
    CORRADE_COMPARE(ui.eventLayer().usedAllocatedCount(), 0);
    #else
    CORRADE_COMPARE(ui.dataLayer().usedAllocatedCount(), 2);
    CORRADE_COMPARE(ui.eventLayer().usedAllocatedCount(), 1 + 2*2);
    #endif
    #if !defined(CORRADE_TARGET_32BIT) && !defined(CORRADE_MSVC2017_COMPATIBILITY)
    CORRADE_COMPARE(ui.genericLayouter().usedAllocatedCount(), 0);
    #elif defined(CORRADE_MSVC2017_COMPATIBILITY)
    CORRADE_COMPARE(ui.genericLayouter().usedAllocatedCount(), 3);
    #else
    CORRADE_COMPARE(ui.genericLayouter().usedAllocatedCount(), 2);
    #endif

    /* Attempting to scroll without update(), which populates the view size,
       being called yet should be a no-op */
    scroll.scrollToPercentage({56.0f, 34.0f});
    CORRADE_COMPARE(scroll.scrollPercentage(), Vector2{});
}

void ScrollAreaTest::constructOnlyX() {
    auto&& data = ConstructData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Subset of construct(), verifying that the correct members get
       populated for given dimension */

    ScrollArea scroll{Anchor{root, {}, {32, 16}}, ScrollAreaFlag::OnlyX|data.flags};
    CORRADE_COMPARE(ui.nodeParent(scroll), root);
    CORRADE_COMPARE(ui.nodeSize(scroll), (Vector2{32, 16}));
    CORRADE_VERIFY(scroll.isOwned());

    CORRADE_COMPARE(scroll.flags(), ScrollAreaFlag::OnlyX|data.flags);
    CORRADE_COMPARE(scroll.scrollPercentage(), Vector2{});
    CORRADE_COMPARE(&scroll.contents().ui(), &ui);
    CORRADE_COMPARE(scroll.contents().node(), scroll.contentsNode());

    CORRADE_VERIFY(ui.isHandleValid(scroll.viewNode()));
    CORRADE_VERIFY(ui.isHandleValid(scroll.contentsNode()));
    CORRADE_VERIFY(ui.isHandleValid(scroll.scrollbarXNode()));
    CORRADE_VERIFY(ui.isHandleValid(scroll.scrollbarThumbXNode()));
    CORRADE_COMPARE(scroll.scrollbarYNode(), NodeHandle::Null);
    CORRADE_COMPARE(scroll.scrollbarThumbYNode(), NodeHandle::Null);
    CORRADE_VERIFY(ui.dataLayer().isHandleValid(scroll.scrollXStorage()));
    CORRADE_COMPARE(scroll.scrollYStorage(), StorageHandle::Null);

    CORRADE_COMPARE(ui.dataLayer().usedCount(), 1);
    CORRADE_COMPARE(ui.baseLayer().usedCount(), 1*2);
    CORRADE_COMPARE(ui.layoutLayer().usedCount(), 2 + 1*2);
    CORRADE_COMPARE(ui.eventLayer().usedCount(), 1 + 1*2);
    CORRADE_COMPARE(ui.snapLayouter().usedCount(), 3 + 1*2);
    CORRADE_COMPARE(ui.genericLayouter().usedCount(), 1 + 1);

    CORRADE_COMPARE(ui.dataLayer().storageUsedAllocatedCount(), 0);
    /** @todo once MSVC 2017 support is dropped, construct all callbacks with
        NoAllocatedInit to catch these at compile time */
    #ifndef CORRADE_MSVC2017_COMPATIBILITY
    CORRADE_COMPARE(ui.dataLayer().usedAllocatedCount(), 0);
    CORRADE_COMPARE(ui.eventLayer().usedAllocatedCount(), 0);
    #else
    CORRADE_COMPARE(ui.dataLayer().usedAllocatedCount(), 1);
    CORRADE_COMPARE(ui.eventLayer().usedAllocatedCount(), 1 + 1*2);
    #endif
    #if !defined(CORRADE_TARGET_32BIT) && !defined(CORRADE_MSVC2017_COMPATIBILITY)
    CORRADE_COMPARE(ui.genericLayouter().usedAllocatedCount(), 0);
    #elif defined(CORRADE_MSVC2017_COMPATIBILITY)
    CORRADE_COMPARE(ui.genericLayouter().usedAllocatedCount(), 2);
    #else
    CORRADE_COMPARE(ui.genericLayouter().usedAllocatedCount(), 1);
    #endif

    /* Attempting to scroll without update(), which populates the view size,
       being called yet should be a no-op */
    scroll.scrollToPercentageX(56.0f);
    CORRADE_COMPARE(scroll.scrollPercentage(), Vector2{});
}

void ScrollAreaTest::constructOnlyY() {
    auto&& data = ConstructData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Subset of construct(), verifying that the correct members get
       populated for given dimension */

    ScrollArea scroll{Anchor{root, {}, {32, 16}}, ScrollAreaFlag::OnlyY|data.flags};
    CORRADE_COMPARE(ui.nodeParent(scroll), root);
    CORRADE_COMPARE(ui.nodeSize(scroll), (Vector2{32, 16}));
    CORRADE_VERIFY(scroll.isOwned());

    CORRADE_COMPARE(scroll.flags(), ScrollAreaFlag::OnlyY|data.flags);
    CORRADE_COMPARE(scroll.scrollPercentage(), Vector2{});
    CORRADE_COMPARE(&scroll.contents().ui(), &ui);
    CORRADE_COMPARE(scroll.contents().node(), scroll.contentsNode());

    CORRADE_VERIFY(ui.isHandleValid(scroll.viewNode()));
    CORRADE_VERIFY(ui.isHandleValid(scroll.contentsNode()));
    CORRADE_COMPARE(scroll.scrollbarXNode(), NodeHandle::Null);
    CORRADE_COMPARE(scroll.scrollbarThumbXNode(), NodeHandle::Null);
    CORRADE_VERIFY(ui.isHandleValid(scroll.scrollbarYNode()));
    CORRADE_VERIFY(ui.isHandleValid(scroll.scrollbarThumbYNode()));
    CORRADE_COMPARE(scroll.scrollXStorage(), StorageHandle::Null);
    CORRADE_VERIFY(ui.dataLayer().isHandleValid(scroll.scrollYStorage()));

    CORRADE_COMPARE(ui.dataLayer().usedCount(), 1);
    CORRADE_COMPARE(ui.baseLayer().usedCount(), 1*2);
    CORRADE_COMPARE(ui.layoutLayer().usedCount(), 2 + 1*2);
    CORRADE_COMPARE(ui.eventLayer().usedCount(), 1 + 1*2);
    CORRADE_COMPARE(ui.snapLayouter().usedCount(), 3 + 1*2);
    CORRADE_COMPARE(ui.genericLayouter().usedCount(), 1 + 1);

    CORRADE_COMPARE(ui.dataLayer().storageUsedAllocatedCount(), 0);
    /** @todo once MSVC 2017 support is dropped, construct all callbacks with
        NoAllocatedInit to catch these at compile time */
    #ifndef CORRADE_MSVC2017_COMPATIBILITY
    CORRADE_COMPARE(ui.dataLayer().usedAllocatedCount(), 0);
    CORRADE_COMPARE(ui.eventLayer().usedAllocatedCount(), 0);
    #else
    CORRADE_COMPARE(ui.dataLayer().usedAllocatedCount(), 1);
    CORRADE_COMPARE(ui.eventLayer().usedAllocatedCount(), 1 + 1*2);
    #endif
    #if !defined(CORRADE_TARGET_32BIT) && !defined(CORRADE_MSVC2017_COMPATIBILITY)
    CORRADE_COMPARE(ui.genericLayouter().usedAllocatedCount(), 0);
    #elif defined(CORRADE_MSVC2017_COMPATIBILITY)
    CORRADE_COMPARE(ui.genericLayouter().usedAllocatedCount(), 2);
    #else
    CORRADE_COMPARE(ui.genericLayouter().usedAllocatedCount(), 1);
    #endif

    /* Attempting to scroll without update(), which populates the view size,
       being called yet should be a no-op */
    scroll.scrollToPercentageY(34.0f);
    CORRADE_COMPARE(scroll.scrollPercentage(), Vector2{});
}

void ScrollAreaTest::constructNonOwned() {
    /* All the properties are verified in construct*() above, check just that
       it propagates all arguments properly */

    ScrollArea scroll{NonOwned, Anchor{root, {}, {32, 16}}, ScrollAreaFlags{0x80}};
    CORRADE_COMPARE(ui.nodeParent(scroll), root);
    CORRADE_COMPARE(ui.nodeSize(scroll), (Vector2{32, 16}));
    CORRADE_VERIFY(!scroll.isOwned());

    CORRADE_COMPARE(scroll.flags(), ScrollAreaFlags{0x80});
}

void ScrollAreaTest::constructNoCreate() {
    ScrollArea scroll{NoCreate};
    CORRADE_COMPARE(scroll.node(), NodeHandle::Null);
    CORRADE_COMPARE(scroll.viewNode(), NodeHandle::Null);
    CORRADE_COMPARE(scroll.contentsNode(), NodeHandle::Null);
    CORRADE_COMPARE(scroll.scrollbarXNode(), NodeHandle::Null);
    CORRADE_COMPARE(scroll.scrollbarThumbXNode(), NodeHandle::Null);
    CORRADE_COMPARE(scroll.scrollbarYNode(), NodeHandle::Null);
    CORRADE_COMPARE(scroll.scrollbarThumbYNode(), NodeHandle::Null);
    CORRADE_COMPARE(scroll.scrollXStorage(), StorageHandle::Null);
    CORRADE_COMPARE(scroll.scrollYStorage(), StorageHandle::Null);
}

void ScrollAreaTest::constructInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    Containers::String out;
    Error redirectError{&out};
    ScrollArea{Anchor{root, {}, {32, 16}}, ScrollAreaFlag::OnlyX|ScrollAreaFlag::OnlyY};
    CORRADE_COMPARE(out, "Ui::ScrollArea: Ui::ScrollAreaFlag::OnlyX and Ui::ScrollAreaFlag::OnlyY are mutually exclusive\n");
}

template<ScrollAreaFlag> struct ScrollAreaFlagTraits;
template<> struct ScrollAreaFlagTraits<ScrollAreaFlag{}> {
    static const char* name() { return ""; }
};
template<> struct ScrollAreaFlagTraits<ScrollAreaFlag::OnlyX> {
    static const char* name() { return "OnlyX"; }
};
template<> struct ScrollAreaFlagTraits<ScrollAreaFlag::OnlyY> {
    static const char* name() { return "OnlyY"; }
};

template<ScrollAreaFlag flag> void ScrollAreaTest::scrollX() {
    auto&& data = ScrollData[testCaseInstanceId()];
    setTestCaseDescription(data.name);
    setTestCaseTemplateName(ScrollAreaFlagTraits<flag>::name());

    ScrollArea scroll{Anchor{root, {150.0f, 350.0f}, {data.size, 300.0f}}, flag|data.extraFlags};
    /* It should be fine for the other direction to have a zero size, even if
       scrolling in both directions. What matters is the view size, which is
       guarded by layout min sizes. */
    ui.setNodeSize(scroll.contents(), {data.contentsSize, 0.0f});
    /* Everything should be clean at first, as the contents node is initially
       at 0% and that's all that matters */
    CORRADE_VERIFY(!ui.dataLayer().isStorageDirty(scroll.scrollXStorage()));
    if(scroll.scrollYStorage() != StorageHandle::Null)
        CORRADE_VERIFY(!ui.dataLayer().isStorageDirty(scroll.scrollYStorage()));

    /* Upon the first update everything should be clean, with the contents and
       the scrollbar thumb being parked at 0% regardless of contents size */
    ui.update();
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(scroll.scrollPercentage(), Vector2{});
    CORRADE_VERIFY(!ui.dataLayer().isStorageDirty(scroll.scrollXStorage()));
    if(scroll.scrollYStorage() != StorageHandle::Null)
        CORRADE_VERIFY(!ui.dataLayer().isStorageDirty(scroll.scrollYStorage()));
    CORRADE_COMPARE(ui.nodeOffset(scroll.contentsNode()), Vector2{});
    const Containers::Pair<Vector2, Vector2> scrollbarThumbOffsetSizeBefore = nodeOffsetSizeAfterLayout(scroll.scrollbarThumbXNode());
    CORRADE_COMPARE(scrollbarThumbOffsetSizeBefore.first().x(),
        nodeOffsetSizeAfterLayout(scroll.scrollbarXNode()).first().x());

    /* The thumb should have a zero size if it's meant to be hidden due to the
       contents not scrolling */
    if(data.isThumbHidden)
        CORRADE_COMPARE(scrollbarThumbOffsetSizeBefore.second().x(), 0.0f);

    /* Scroll to the initial percentage if requested, and update again */
    if(data.initialPercentage) {
        scroll.scrollToPercentageX(*data.initialPercentage);
        CORRADE_COMPARE(ui.dataLayer().isStorageDirty(scroll.scrollXStorage()), !data.isInitialPercentageNoOp);
        /* The other storage should stay clean */
        if(scroll.scrollYStorage() != StorageHandle::Null)
            CORRADE_VERIFY(!ui.dataLayer().isStorageDirty(scroll.scrollYStorage()));

        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
        CORRADE_COMPARE(scroll.scrollPercentage(), data.isInitialPercentageNoOp ?
            Vector2{} : Vector2::xAxis(*data.initialPercentage));
        CORRADE_VERIFY(!ui.dataLayer().isStorageDirty(scroll.scrollXStorage()));
        if(scroll.scrollYStorage() != StorageHandle::Null)
            CORRADE_VERIFY(!ui.dataLayer().isStorageDirty(scroll.scrollYStorage()));
    }

    /* Do a scroll or a drag from the center of given node, if desired */
    Vector2 nodeCenterBefore;
    if(data.nodeX) {
        nodeCenterBefore = nodeCenterAfterLayout((scroll.*data.nodeX)());
        if(data.scroll) {
            ScrollEvent event{{}, data.offset, {}};
            CORRADE_VERIFY(ui.scrollEvent(nodeCenterBefore, event));
            /* This is the prerequisite for correct scroll amount */
            CORRADE_COMPARE(ui.eventLayer().scrollStepDistance().x(), -100.0f);
        } else {
            /* Press to actually capture the event on the node. If
               NoContentsDrag is enabled, the event and all events after are
               ignored. */
            PointerEvent press{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0, {}};
            CORRADE_COMPARE(ui.pointerPressEvent(nodeCenterBefore, press), !(data.extraFlags >= ScrollAreaFlag::NoContentsDrag));
            CORRADE_COMPARE(ui.currentPressedNode(), data.extraFlags >= ScrollAreaFlag::NoContentsDrag ?
                NodeHandle::Null : (scroll.*data.nodeX)());

            /* Drag, is ignored if NoContentsDrag is enabled */
            PointerMoveEvent drag{{}, PointerEventSource::Mouse, {}, Pointer::MouseLeft, true, 0, {}};
            CORRADE_COMPARE(ui.pointerMoveEvent(nodeCenterBefore + data.offset, drag), !(data.extraFlags >= ScrollAreaFlag::NoContentsDrag));
        }
    }

    /* If we were dragging the thumb to a precise position (without any clamp
       occuring at either end), it's mainly important that it follows the
       cursor, not that the expected percentage is exact, which is checked
       below. */
    bool preciseThumbDrag = data.nodeX == &ScrollArea::scrollbarThumbXNode && !data.scroll && data.expectedPercentage != 0.0f && data.expectedPercentage != 100.0f;

    /* This should update the percentage and make the storage dirty if not
       no-op. The other storage should stay clean. */
    CORRADE_COMPARE(ui.dataLayer().isStorageDirty(scroll.scrollXStorage()), !data.isNoOp);
    if(scroll.scrollYStorage() != StorageHandle::Null)
        CORRADE_VERIFY(!ui.dataLayer().isStorageDirty(scroll.scrollYStorage()));
    /* If we were dragging the thumb and the scroll area has both directions
       enabled, the scrollbar may not have the same size as the scroll area due
       to styling and thus the percentage may not match exactly. */
    if(preciseThumbDrag && flag == ScrollAreaFlag{}) {
        CORRADE_COMPARE_WITH(scroll.scrollPercentage(),
            Vector2::xAxis(data.expectedPercentage),
            TestSuite::Compare::around(Vector2::xAxis(5.0f)));
    } else CORRADE_COMPARE(scroll.scrollPercentage(), Vector2::xAxis(data.expectedPercentage));

    /* If not no-op, the actual node offset changes after an update */
    if(!data.isNoOp)
        ui.update();
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    /* Again if we were dragging the thumb and the scroll area has both
       directions enabled, the offset may not match exactly */
    if(preciseThumbDrag && flag == ScrollAreaFlag{}) {
        CORRADE_COMPARE_WITH(ui.nodeOffset(scroll.contents()),
            Vector2::xAxis(data.expectedNodeOffset),
            TestSuite::Compare::around(Vector2::xAxis(12.0f)));
    } else CORRADE_COMPARE(ui.nodeOffset(scroll.contents()), Vector2::xAxis(data.expectedNodeOffset));

    /* The thumb placement should match the percentage exactly. It's only
       reflected in the layout, not in the base UI node offset. */
    Containers::Pair<Vector2, Vector2> scrollbarOffsetSize = nodeOffsetSizeAfterLayout(scroll.scrollbarXNode());
    Containers::Pair<Vector2, Vector2> scrollbarThumbOffsetSize = nodeOffsetSizeAfterLayout(scroll.scrollbarThumbXNode());
    CORRADE_COMPARE(100.0f*(scrollbarThumbOffsetSize.first().x() - scrollbarOffsetSize.first().x())/(scrollbarOffsetSize.second().x() - scrollbarThumbOffsetSize.second().x()), scroll.scrollPercentage().x());

    /* If we were dragging the thumb, it should have moved exactly the drag
       offset, following the cursor, even in case the scrollbar isn't actually
       the same size as the scroll area view. Check only if the final position
       is neither 0% nor 100%, as there a clamp might occur. */
    if(preciseThumbDrag)
        CORRADE_COMPARE(nodeCenterAfterLayout(scroll.scrollbarThumbXNode()).x() - nodeCenterBefore.x(), data.offset.x());
}

template<ScrollAreaFlag flag> void ScrollAreaTest::scrollY() {
    auto&& data = ScrollData[testCaseInstanceId()];
    setTestCaseDescription(data.name);
    setTestCaseTemplateName(ScrollAreaFlagTraits<flag>::name());

    /* Like scrollX(), just with dimensions flipped. Everything should behave
       exactly the same. */

    ScrollArea scroll{Anchor{root, {350.0f, 150.0f}, {300.0f, data.size}}, flag|data.extraFlags};
    /* It should be fine for the other direction to have a zero size, even if
       scrolling in both directions. What matters is the view size, which is
       guarded by layout min sizes. */
    ui.setNodeSize(scroll.contents(), {0.0f, data.contentsSize});

    /* Upon the first update everything should be clean, with the contents and
       the scrollbar thumb being parked at 0% regardless of contents size */
    ui.update();
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(scroll.scrollPercentage(), Vector2{});
    CORRADE_VERIFY(!ui.dataLayer().isStorageDirty(scroll.scrollYStorage()));
    if(scroll.scrollXStorage() != StorageHandle::Null)
        CORRADE_VERIFY(!ui.dataLayer().isStorageDirty(scroll.scrollXStorage()));
    CORRADE_COMPARE(ui.nodeOffset(scroll.contentsNode()), Vector2{});
    const Containers::Pair<Vector2, Vector2> scrollbarThumbOffsetSizeBefore = nodeOffsetSizeAfterLayout(scroll.scrollbarThumbYNode());
    CORRADE_COMPARE(scrollbarThumbOffsetSizeBefore.first().y(),
        nodeOffsetSizeAfterLayout(scroll.scrollbarYNode()).first().y());

    /* The thumb should have a zero size if it's meant to be hidden due to the
       contents not scrolling */
    if(data.isThumbHidden)
        CORRADE_COMPARE(scrollbarThumbOffsetSizeBefore.second().y(), 0.0f);

    /* Scroll to the initial percentage if requested, and update again */
    if(data.initialPercentage) {
        scroll.scrollToPercentageY(*data.initialPercentage);
        CORRADE_COMPARE(ui.dataLayer().isStorageDirty(scroll.scrollYStorage()), !data.isInitialPercentageNoOp);
        /* The other storage should stay clean */
        if(scroll.scrollXStorage() != StorageHandle::Null)
            CORRADE_VERIFY(!ui.dataLayer().isStorageDirty(scroll.scrollXStorage()));

        ui.update();
        CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
        CORRADE_COMPARE(scroll.scrollPercentage(), data.isInitialPercentageNoOp ?
            Vector2{} : Vector2::yAxis(*data.initialPercentage));
        CORRADE_VERIFY(!ui.dataLayer().isStorageDirty(scroll.scrollYStorage()));
        if(scroll.scrollXStorage() != StorageHandle::Null)
            CORRADE_VERIFY(!ui.dataLayer().isStorageDirty(scroll.scrollXStorage()));
    }

    /* Do a scroll or a drag from the center of given node, if desired */
    Vector2 nodeCenterBefore;
    if(data.nodeY) {
        nodeCenterBefore = nodeCenterAfterLayout((scroll.*data.nodeY)());
        if(data.scroll) {
            /* While horizontal scroll is positive to the right, vertical is
               positive to the top. Flip the signs in addition to flipping the
               dimensions, so a +X now becomes -Y but also -Y now becomes +X */
            ScrollEvent scroll{{}, -data.offset.flipped(), {}};
            CORRADE_VERIFY(ui.scrollEvent(nodeCenterBefore, scroll));
            /* This is the prerequisite for correct scroll amount */
            CORRADE_COMPARE(ui.eventLayer().scrollStepDistance().y(), 100.0f);
        } else {
            /* Press to actually capture the event on the node. If
               NoContentsDrag is enabled, the event and all events after are
               ignored. */
            PointerEvent press{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0, {}};
            CORRADE_COMPARE(ui.pointerPressEvent(nodeCenterBefore, press), !(data.extraFlags >= ScrollAreaFlag::NoContentsDrag));
            CORRADE_COMPARE(ui.currentPressedNode(), data.extraFlags >= ScrollAreaFlag::NoContentsDrag ?
                NodeHandle::Null : (scroll.*data.nodeY)());

            /* Drag, is ignored if NoContentsDrag is enabled */
            PointerMoveEvent drag{{}, PointerEventSource::Mouse, {}, Pointer::MouseLeft, true, 0, {}};
            CORRADE_COMPARE(ui.pointerMoveEvent(nodeCenterBefore + data.offset.flipped(), drag), !(data.extraFlags >= ScrollAreaFlag::NoContentsDrag));
        }
    }

    /* If we were dragging the thumb to a precise position (without any clamp
       occuring at either end), it's mainly important that it follows the
       cursor, not that the expected percentage is exact, which is checked
       below. */
    bool preciseThumbDrag = data.nodeY == &ScrollArea::scrollbarThumbYNode && !data.scroll && data.expectedPercentage != 0.0f && data.expectedPercentage != 100.0f;

    /* This should update the percentage and make the storage dirty if not
       no-op. The other storage should stay clean. */
    CORRADE_COMPARE(ui.dataLayer().isStorageDirty(scroll.scrollYStorage()), !data.isNoOp);
    if(scroll.scrollXStorage() != StorageHandle::Null)
        CORRADE_VERIFY(!ui.dataLayer().isStorageDirty(scroll.scrollXStorage()));
    /* If we were dragging the thumb and the scroll area has both directions
       enabled, the scrollbar may not have the same size as the scroll area due
       to styling and thus the percentage may not match exactly. */
    if(preciseThumbDrag && flag == ScrollAreaFlag{}) {
        CORRADE_COMPARE_WITH(scroll.scrollPercentage(),
            Vector2::yAxis(data.expectedPercentage),
            TestSuite::Compare::around(Vector2::yAxis(5.0f)));
    } else CORRADE_COMPARE(scroll.scrollPercentage(), Vector2::yAxis(data.expectedPercentage));

    /* If not no-op, the actual node offset changes after an update */
    if(!data.isNoOp)
        ui.update();
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    /* Again if we were dragging the thumb and the scroll area has both
       directions enabled, the offset may not match exactly */
    if(preciseThumbDrag && flag == ScrollAreaFlag{}) {
        CORRADE_COMPARE_WITH(ui.nodeOffset(scroll.contents()),
            Vector2::yAxis(data.expectedNodeOffset),
            TestSuite::Compare::around(Vector2::yAxis(12.0f)));
    } else CORRADE_COMPARE(ui.nodeOffset(scroll.contents()), Vector2::yAxis(data.expectedNodeOffset));

    /* The thumb placement should match the percentage exactly. It's only
       reflected in the layout, not in the base UI node offset. */
    Containers::Pair<Vector2, Vector2> scrollbarOffsetSize = nodeOffsetSizeAfterLayout(scroll.scrollbarYNode());
    Containers::Pair<Vector2, Vector2> scrollbarThumbOffsetSize = nodeOffsetSizeAfterLayout(scroll.scrollbarThumbYNode());
    CORRADE_COMPARE(100.0f*(scrollbarThumbOffsetSize.first().y() - scrollbarOffsetSize.first().y())/(scrollbarOffsetSize.second().y() - scrollbarThumbOffsetSize.second().y()), scroll.scrollPercentage().y());

    /* If we were dragging the thumb, it should have moved exactly the drag
       offset, following the cursor, even in case the scrollbar isn't actually
       the same size as the scroll area view. Check only if the final position
       is neither 0% nor 100%, as there a clamp might occur. */
    if(preciseThumbDrag)
        /* Using data.offset.x() because we're feeding it as Y offset here */
        CORRADE_COMPARE(nodeCenterAfterLayout(scroll.scrollbarThumbYNode()).y() - nodeCenterBefore.y(), data.offset.x());
}

void ScrollAreaTest::dragFallthrough() {
    auto&& data = DragFallthroughData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* First verify that the test data match the drag threshold expectation. If
       we have NoContentsDrag enabled, the drag offset should be also large
       enough to trigger the fallthrough. */
    if(data.expectDrag || data.flags >= ScrollAreaFlag::NoContentsDrag)
        CORRADE_COMPARE_AS(data.offset.length(),
            ui.eventLayer().dragThreshold(),
            TestSuite::Compare::Greater);
    else
        CORRADE_COMPARE_AS(data.offset.length(),
            ui.eventLayer().dragThreshold(),
            TestSuite::Compare::Less);

    ScrollArea scroll{Anchor{root, {200.0f, 300.0f}, {100.0f, 100.0f}}, data.flags};

    /* Make the contents large enough and reacting to a tap or click */
    ui.setNodeSize(scroll.contents(), {
        data.flags >= ScrollAreaFlag::OnlyY ? 100.0f : 200.0f,
        data.flags >= ScrollAreaFlag::OnlyX ? 100.0f : 200.0f
    });
    Int clicked = 0;
    ui.eventLayer().onTapOrClick(scroll.contents(), [&clicked] {
        ++clicked;
    });

    /* The view node shouldn't receive fallthrough events if pointer events are
       disabled. React to tap or click in that case to test that. */
    if(data.flags >= ScrollAreaFlag::NoContentsDrag)
        ui.eventLayer().onDrag(scroll.viewNode(), [](const Vector2&) {
            CORRADE_FAIL("Events incorrectly fall through to the view node even with" << ScrollAreaFlag::NoContentsDrag << "enabled");
        });

    /* Update to layout the contents and scroll to the center so we can drag
       either way without hitting an edge. The contents are 2x large as the
       view which means this scrolls them so the range [50, 150] is in the
       center, i.e. the percentage corresponds exactly to the units. */
    ui.update();
    Vector2 initialPercentage{
        data.flags >= ScrollAreaFlag::OnlyY ? 0.0f : 50.0f,
        data.flags >= ScrollAreaFlag::OnlyX ? 0.0f : 50.0f,
    };
    scroll.scrollToPercentage(initialPercentage);
    CORRADE_COMPARE(scroll.scrollPercentage(), initialPercentage);

    /* Press to actually capture the event on the scroll area, where it gets
       accepted by the contents already, and not the view */
    PointerEvent press{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0, {}};
    CORRADE_VERIFY(ui.pointerPressEvent({250.0f, 350.0f}, press));
    CORRADE_COMPARE(ui.currentPressedNode(), scroll.contentsNode());

    /* Drag. This should fall through to the view if we're expecting a drag and
       the drag is long enough, and result in the scroll percentage updated. */
    PointerMoveEvent drag{{}, PointerEventSource::Mouse, {}, Pointer::MouseLeft, true, 0, {}};
    CORRADE_COMPARE(ui.pointerMoveEvent(Vector2{250.0f, 350.0f} + data.offset, drag), data.expectDrag);
    CORRADE_COMPARE(ui.currentPressedNode(), data.expectDrag ?
        scroll.viewNode() : scroll.contentsNode());
    CORRADE_COMPARE(scroll.scrollPercentage(), data.expectDrag ?
        initialPercentage - data.offset : initialPercentage);

    /* On release the contents should receive (and accept) a tap or click only
       if not expecting a drag */
    PointerEvent release{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0, {}};
    CORRADE_COMPARE(ui.pointerReleaseEvent(Vector2{250.0f, 350.0f} + data.offset, release), !data.expectDrag);
    CORRADE_COMPARE(clicked, data.expectDrag ? 0 : 1);
}

void ScrollAreaTest::widgetResized() {
    auto&& data = WidgetResizedData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    ScrollArea scroll{Anchor{root, {}, {1000.0f, 1000.0f}}, data.flags};
    ui.setNodeSize(scroll.contentsNode(), {2000.0f, 2000.0f});

    /* Update to make the scroll area aware of its size and put it into the
       initial position */
    ui.update();
    scroll.scrollToPercentage(data.scrollPercentage);

    /* Updating again should make the node offset match the percentage, just
       10x larger and negative. There should be no leftover state set. */
    ui.update();
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(scroll.scrollPercentage(), data.scrollPercentage);
    CORRADE_COMPARE(nodeOffsetSizeAfterLayout(scroll.contentsNode()).first(), data.scrollPercentage*-10.0f);

    /* Resize and update again. The scroll position should adapt so it's still
       in [0%, 100%], and the actual layout node offset should match that. */
    ui.setNodeSize(scroll, data.newSize);
    ui.update();
    CORRADE_COMPARE(ui.state(), UserInterfaceStates{});
    CORRADE_COMPARE(scroll.scrollPercentage(), data.expectedScrollPercentage);
    CORRADE_COMPARE(nodeOffsetSizeAfterLayout(scroll.contentsNode()).first(),  data.expectedScrollPercentage*(ui.nodeSize(scroll.contentsNode()) - nodeOffsetSizeAfterLayout(scroll.viewNode()).second())/-100.0f);
}

UnsignedInt nodeIndices[7];

template<UnsignedInt nodeCount, ScrollAreaFlag flag> void ScrollAreaTest::scrollbarEventOrder() {
    setTestCaseTemplateName(ScrollAreaFlagTraits<flag>::name());

    /* This test is being run 7! times, to go through all possible node order
       combinations. That's 5040 combinations, which if the test reused the
       WidgetTester UI instance, would likely cause handle generation
       exhaustion (and thus assertions in WidgetTester), and we'd have hard
       time getting the nodes in the right order.

       Even though this is creating the whole UI so many times, it doesn't seem
       to make any significant dent in the total test run time. Interesting. */

    DarkTheme theme;

    TestUserInterface ui{NoCreate};
    ui.setDataLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()))
      .setBaseLayerInstance(Containers::pointer<TestBaseLayer>(ui.createLayer(), baseLayerShared))
      /* The text layer isn't used for anything by this widget */
      .setEventLayerInstance(Containers::pointer<EventLayer>(ui.createLayer()))
      .setLayoutLayerInstance(Containers::pointer<LayoutLayer>(ui.createLayer(), theme.layoutLayerStyleCount()))
      .setSnapLayouterInstance(Containers::pointer<SnapLayouter>(ui.createLayouter()))
      .setGenericLayouterInstance(Containers::pointer<GenericLayouter>(ui.createLayouter()))
      .setSize({100, 100});

    /* Need the LayoutLayer populated with actual real paddings and min sizes
       as otherwise most inner nodes would have zero sizes, being impossible to
       fire events on */
    CORRADE_VERIFY(theme.apply(ui, ThemeFeature::LayoutLayer, {}, {}));

    /* If this is the first repeat, fill the index array, otherwise turn it
       into the next permutation. Cannot just call std::next_permutation() in a
       loop because that apparently takes A LOT more than creating the whole UI
       instance. Heh. Don't tell me the STL Debug perf is this bad, I refuse to
       accept that sorting 7 items some millions of times is so slow. */
    if(testCaseRepeatId() == 0)
        Utility::copy({0u, 1u, 2u, 3u, 4u, 5u, 6u}, nodeIndices);
    else
        CORRADE_VERIFY(std::next_permutation(nodeIndices, nodeIndices + nodeCount));
    CORRADE_ITERATION(Debug::packed << Containers::arrayView(nodeIndices));

    /* Create at most 7 nodes and then recycle them in the shuffled order. That
       should result in them being picked from the free list in desired
       combination every time. */
    NodeHandle nodes[nodeCount];
    for(NodeHandle& node: nodes)
        node = ui.createNode({}, {});
    for(UnsignedInt i: Containers::arrayView(nodeIndices, nodeCount))
        ui.removeNode(nodes[i]);

    /* Create the scroll area as a root node and verify the above assumption
       that it consists of at most 7 nodes */
    ScrollArea scroll{Anchor{ui, {}, {100.0f, 100.0f}}, flag};
    CORRADE_COMPARE(ui.nodeUsedCount(), nodeCount);

    /* Size the contents and make them react to pointer press, drag and scroll.
       If everything goes well, it shouldn't get called earlier than the
       scrollbar event handlers. */
    ui.setNodeSize(scroll.contents(), {
        flag == ScrollAreaFlag::OnlyY ? 100.0f : 300.0f,
        flag == ScrollAreaFlag::OnlyX ? 100.0f : 300.0f
    });
    ui.eventLayer().onPress(scroll.contents(), []() {
        CORRADE_FAIL("Press got caught on the contents first.");
    });
    ui.eventLayer().onDrag(scroll.contents(), [](const Vector2&) {
        CORRADE_FAIL("Drag got caught on the contents first.");
    });
    ui.eventLayer().onScroll(scroll.contents(), [](const Vector2&) {
        CORRADE_FAIL("Scroll got caught on the contents first.");
    });

    /* Layer for node layout position queries */
    NodeOffsetSizeQueryLayer& offsetSizeQueryLayer = ui.setLayerInstance(Containers::pointer<NodeOffsetSizeQueryLayer>(ui.createLayer()));

    /* Horizontal scrollbar scroll, if there is */
    if(scroll.scrollbarXNode() != NodeHandle::Null) {
        CORRADE_ITERATION(__FILE__ ":" CORRADE_LINE_STRING);

        ScrollEvent event{{}, {1.0f, 0.0f}, {}};
        CORRADE_VERIFY(ui.scrollEvent(nodeCenterAfterLayout(ui, offsetSizeQueryLayer, scroll.scrollbarXNode()), event));
        CORRADE_COMPARE_WITH(scroll.scrollPercentage(),
            (Vector2{50.0f, 0.0f}),
            TestSuite::Compare::around(Vector2{5.0f}));
    }

    /* Vertical scrollbar scroll, if there is */
    if(scroll.scrollbarYNode() != NodeHandle::Null) {
        CORRADE_ITERATION(__FILE__ ":" CORRADE_LINE_STRING);

        ScrollEvent event{{}, {0.0f, -1.0f}, {}};
        CORRADE_VERIFY(ui.scrollEvent(nodeCenterAfterLayout(ui, offsetSizeQueryLayer, scroll.scrollbarYNode()), event));
        CORRADE_COMPARE_WITH(scroll.scrollPercentage(),
            (Vector2{flag == ScrollAreaFlag::OnlyY ? 0.0f : 50.0f, 50.0f}),
            TestSuite::Compare::around(Vector2{5.0f}));
    }

    /* Horizontal scrollbar thumb drag. If there is a scrollbar, the thumb
       should be as well, the contents are not smaller than the view. */
    if(scroll.scrollbarXNode() != NodeHandle::Null) {
        CORRADE_ITERATION(__FILE__ ":" CORRADE_LINE_STRING);

        Vector2 center = nodeCenterAfterLayout(ui, offsetSizeQueryLayer, scroll.scrollbarThumbXNode());

        /* Press to actually capture the event on the node */
        PointerEvent press{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0, {}};
        CORRADE_VERIFY(ui.pointerPressEvent(center, press));
        CORRADE_COMPARE(ui.currentPressedNode(), scroll.scrollbarThumbXNode());

        /* Drag */
        PointerMoveEvent drag{{}, PointerEventSource::Mouse, {}, Pointer::MouseLeft, true, 0, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent(center - Vector2{10.0f, 0.0f}, drag));

        CORRADE_COMPARE_WITH(scroll.scrollPercentage(),
            (Vector2{35.0f, flag == ScrollAreaFlag::OnlyX ? 0.0f : 50.0f}),
            TestSuite::Compare::around(Vector2{5.0f}));
    }

    /* Vertical scrollbar thumb drag. If there is a scrollbar, the thumb
       should be as well, the contents are not smaller than the view. */
    if(scroll.scrollbarYNode() != NodeHandle::Null) {
        CORRADE_ITERATION(__FILE__ ":" CORRADE_LINE_STRING);

        Vector2 center = nodeCenterAfterLayout(ui, offsetSizeQueryLayer, scroll.scrollbarThumbYNode());

        /* Press to actually capture the event on the node */
        PointerEvent press{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0, {}};
        CORRADE_VERIFY(ui.pointerPressEvent(center, press));
        CORRADE_COMPARE(ui.currentPressedNode(), scroll.scrollbarThumbYNode());

        /* Drag */
        PointerMoveEvent drag{{}, PointerEventSource::Mouse, {}, Pointer::MouseLeft, true, 0, {}};
        CORRADE_VERIFY(ui.pointerMoveEvent(center - Vector2{0.0f, 20.0f}, drag));

        CORRADE_COMPARE_WITH(scroll.scrollPercentage(),
            (Vector2{flag == ScrollAreaFlag::OnlyY ? 0.0f : 35.0f, 20.0f}),
            TestSuite::Compare::around(Vector2{5.0f}));
    }
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::ScrollAreaTest)
