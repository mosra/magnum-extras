#ifndef Magnum_Ui_Implementation_abstractVisualLayerAnimationState_h
#define Magnum_Ui_Implementation_abstractVisualLayerAnimationState_h
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

#include "Magnum/Ui/AbstractVisualLayerAnimator.h"
#include "Magnum/Ui/Implementation/abstractVisualLayerState.h"

/* Definition of the AbstractVisualLayerStyleAnimator::State struct to be used
   by subclasses */

namespace Magnum { namespace Ui {

struct AbstractVisualLayerStyleAnimator::State {
    /* Assumes that the derived state structs have non-trivially-destructible
       members. Without a virtual destructor those wouldn't be destructed
       properly when deleting from the base pointer. This is also checked with
       a static_assert() in the Pointer class itself.

       Another possibility would be to let the derived class have its own state
       allocation, but I feel that having a base with a virtual destructor is
       less nasty and nicer to caches than two loose allocations of non-virtual
       types. */
    /** @todo switch to a custom (casting) deleter instead once Pointer has
        that */
    virtual ~State() = default;

    AbstractVisualLayer* layer = nullptr;
    const AbstractVisualLayer::Shared::State* layerSharedState = nullptr;
    /* These views are assumed to point to subclass own data and maintained to
       have its size always match layer capacity */
    Containers::StridedArrayView1D<UnsignedInt> expectedStyles;
    Containers::StridedArrayView1D<UnsignedInt> sourceStyles;
    Containers::StridedArrayView1D<UnsignedInt> targetStyles;
    Containers::StridedArrayView1D<UnsignedInt> dynamicStyles;
};

}}

#endif
