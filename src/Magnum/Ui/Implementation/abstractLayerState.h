#ifndef Magnum_Ui_Implementation_abstractLayerState_h
#define Magnum_Ui_Implementation_abstractLayerState_h
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

#include <Corrade/Containers/Array.h>

#include "Magnum/Ui/AbstractLayer.h"

namespace Magnum { namespace Ui {

namespace Implementation {

union AbstractLayerData {
    explicit AbstractLayerData() noexcept: used{} {}

    struct Used {
        /* Together with index of this item in `data` used for creating a
           LayerDataHandle. Increased every time a handle reaches remove(). Has
           to be initially non-zero to differentiate the first ever handle
           (with index 0) from LayerDataHandle::Null. Once becomes
           `1 << LayerDataHandleGenerationBits` the handle gets disabled. */
        UnsignedShort generation = 1;

        /* Two bytes free */

        /* Node the data is attached to. Becomes null again when the data is
           freed. Has to be re-filled every time a handle is recycled, so it
           doesn't make sense to initialize it to anything. */
        NodeHandle node;

        /* Four bytes free */
    } used;

    /* Used only if the Data is among free ones */
    struct Free {
        /* The generation value has to be preserved in order to increment it
           next time it gets used */
        UnsignedShort generation;

        /* The node field is needed to discard free items when directly
           iterating the list. */
        /** @todo any idea how to better pack this? this is a bit awful */
        NodeHandle node;

        /* See State::firstFree for more information */
        UnsignedInt next;
    } free;
};

#ifndef CORRADE_NO_STD_IS_TRIVIALLY_TRAITS
static_assert(std::is_trivially_copyable<AbstractLayerData>::value, "AbstractLayerData not trivially copyable");
#endif
static_assert(
    offsetof(AbstractLayerData::Used, generation) == offsetof(AbstractLayerData::Free, generation) &&
    offsetof(AbstractLayerData::Used, node) == offsetof(AbstractLayerData::Free, node),
    "AbstractLayerData::Used and Free layout not compatible");

}

struct AbstractLayer::State {
    LayerHandle handle;
    LayerStates state;

    #ifndef CORRADE_NO_ASSERT
    bool setSizeCalled = false;
    #endif
    /* 0/4 bytes free, 1/5 on a no-assert build */

    Containers::Array<Implementation::AbstractLayerData> data;
    /* Indices in the data array. The AbstractLayerData then has a nextFree
       member containing the next free index. New data get taken from the
       front, removed are put at the end. A value of ~UnsignedInt{} means
       there's no (first/next/last) free data. */
    UnsignedInt firstFree = ~UnsignedInt{};
    UnsignedInt lastFree = ~UnsignedInt{};
};

}}

#endif
