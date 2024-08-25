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

#include "Magnum/Whee/AbstractUserInterface.h"
#include "Magnum/Whee/AbstractVisualLayer.h"
#include "Magnum/Whee/Event.h"
#include "Magnum/Whee/Handle.h"

#define DOXYGEN_ELLIPSIS(...) __VA_ARGS__
#define DOXYGEN_IGNORE(...) __VA_ARGS__

using namespace Magnum;

Whee::AbstractVisualLayer::Shared& abstractVisualLayerShared();
namespace {
/* [AbstractVisualLayer-Shared-setStyleTransition] */
enum StyleIndex {
    DOXYGEN_ELLIPSIS()
};

StyleIndex styleIndexTransitionToInactiveBlur(StyleIndex index) {
    DOXYGEN_ELLIPSIS(return index;)
}
StyleIndex styleIndexTransitionToInactiveHover(StyleIndex index) {
    DOXYGEN_ELLIPSIS(return index;)
}
StyleIndex styleIndexTransitionToFocusedBlur(StyleIndex index) {
    DOXYGEN_ELLIPSIS(return index;)
}
StyleIndex styleIndexTransitionToFocusedHover(StyleIndex index) {
    DOXYGEN_ELLIPSIS(return index;)
}
StyleIndex styleIndexTransitionToPressedBlur(StyleIndex index) {
    DOXYGEN_ELLIPSIS(return index;)
}
StyleIndex styleIndexTransitionToPressedHover(StyleIndex index) {
    DOXYGEN_ELLIPSIS(return index;)
}
StyleIndex styleIndexTransitionToDisabled(StyleIndex index) {
    DOXYGEN_ELLIPSIS(return index;)
}

DOXYGEN_ELLIPSIS(} int main() {)

Whee::AbstractVisualLayer::Shared& shared = DOXYGEN_ELLIPSIS(abstractVisualLayerShared());
shared.setStyleTransition<StyleIndex,
    styleIndexTransitionToInactiveBlur,
    styleIndexTransitionToInactiveHover,
    styleIndexTransitionToFocusedBlur,
    styleIndexTransitionToFocusedHover,
    styleIndexTransitionToPressedBlur,
    styleIndexTransitionToPressedHover,
    styleIndexTransitionToDisabled>();
/* [AbstractVisualLayer-Shared-setStyleTransition] */
}

/* Make sure the name doesn't conflict with any other snippets to avoid linker
   warnings, unlike with `int main()` there now has to be a declaration to
   avoid -Wmisssing-prototypes */
void mainWhee();
void mainWhee() {
{
Whee::AbstractUserInterface ui{{100, 100}};
Whee::NodeHandle node{};
Whee::FocusEvent event{{}};
/* [AbstractUserInterface-focusEvent-blur-if-not-focusable] */
if(!ui.focusEvent(node, event))
    ui.focusEvent(Whee::NodeHandle::Null, event);
/* [AbstractUserInterface-focusEvent-blur-if-not-focusable] */
}
}
