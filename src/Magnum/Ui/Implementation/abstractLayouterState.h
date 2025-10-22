#ifndef Magnum_Ui_Implementation_abstractLayouterState_h
#define Magnum_Ui_Implementation_abstractLayouterState_h
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

/* Definition of the AbstractLayouter::State struct that's also used by
   AbstractUserInterface internals to manage the UI reference stored in it */

#include <Corrade/Containers/Array.h>

#include "Magnum/Ui/AbstractLayouter.h"

namespace Magnum { namespace Ui {

namespace Implementation {

union Layout {
    explicit Layout() noexcept: used{} {}

    struct Used {
        /* Together with index of this item in `layouts` used for creating a
           LayouterDataHandle. Increased every time a handle reaches remove().
           Has to be initially non-zero to differentiate the first ever handle
           (with index 0) from LayouterDataHandle::Null. Once becomes
           `1 << LayouterDataHandleGenerationBits` the handle gets disabled. */
        UnsignedShort generation = 1;

        /* Two bytes free */

        /* Node the layout is assigned to. Is null only when the layout is
           freed. Has to be re-filled every time a handle is recycled, so it
           doesn't make sense to initialize it to anything. isHandleValid()
           checks this field to correctly mark invalid handles if the
           generation matches by accident. */
        NodeHandle node;

        /* Four bytes free */
    } used;

    /* Used only if the Layout is among free ones */
    struct Free {
        /* The generation value has to be preserved in order to increment it
           next time it gets used */
        UnsignedShort generation;

        /* Two bytes free */

        /* If this is null, the layout is freed */
        /** @todo any idea how to better pack this? this is a bit awful */
        NodeHandle node;

        /* See State::firstFree for more information */
        UnsignedInt next;
    } free;
};

#ifndef CORRADE_NO_STD_IS_TRIVIALLY_TRAITS
static_assert(std::is_trivially_copyable<Layout>::value, "Layout not trivially copyable");
#endif
static_assert(
    offsetof(Layout::Used, generation) == offsetof(Layout::Free, generation) &&
    offsetof(Layout::Used, node) == offsetof(Layout::Free, node),
    "Layout::Used and Free layout not compatible");

}

struct AbstractLayouter::State {
    LayouterHandle handle;
    LayouterStates state;

    #ifndef CORRADE_NO_ASSERT
    bool setSizeCalled = false;
    #endif
    /* 0/4 bytes free, 1/5 on a no-assert build */

    /* Gets set by AbstractUserInterface::setLayouterInstance() and further
       updated on every UI move */
    const AbstractUserInterface* ui = nullptr;

    Containers::Array<Implementation::Layout> layouts;
    /* Indices in the `layouts` array. The Layout then has a nextFree member
       containing the next free index. New layouts get taken from the front,
       removed are put at the end. A value of ~UnsignedInt{} means there's no
       (first/next/last) free layout. */
    UnsignedInt firstFree = ~UnsignedInt{};
    UnsignedInt lastFree = ~UnsignedInt{};
};

}}

#endif
