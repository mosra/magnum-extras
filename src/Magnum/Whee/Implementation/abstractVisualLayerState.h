#ifndef Magnum_Whee_Implementation_abstractVisualLayerState_h
#define Magnum_Whee_Implementation_abstractVisualLayerState_h
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

/* Definition of AbstractVisualLayer::State and
   AbstractVisualLayer::Shared::State structs to be used by tests, and (if this
   header gets published) eventually possibly also 3rd party renderer
   implementations */

#include <Corrade/Containers/ArrayTuple.h>
#include <Corrade/Containers/BitArrayView.h>
#include <Corrade/Containers/Reference.h>
#include <Corrade/Containers/StridedArrayView.h>

#include "Magnum/Whee/AbstractVisualLayer.h"

namespace Magnum { namespace Whee {

namespace Implementation {
    constexpr UnsignedInt styleTransitionPassthrough(UnsignedInt index) { return index; }
}

struct AbstractVisualLayer::Shared::State {
    explicit State(Shared& self, const UnsignedInt styleCount, const UnsignedInt dynamicStyleCount) noexcept: self{self}, styleCount{styleCount}, dynamicStyleCount{dynamicStyleCount} {}
    /* Assumes that the derived state structs will have
       non-trivially-destructible members. Without a virtual destructor those
       wouldn't be destructed properly when deleting from the base pointer.
       This is also checked with a static_assert() in the Pointer class itself.

       Another possibility would be to let the derived classes have their own
       state allocation (and subsequently the GL/... derived classes another),
       but I feel that having a base with a virtual destructor is less nasty
       and nicer to caches than several loose allocations of non-virtual
       types. */
    /** @todo switch to a custom (casting) deleter instead once Pointer has
        that */
    virtual ~State() = default;

    /* References the public instance, for use by BaseLayer::shared() and
       similar APIs. Gets updated when the Shared instance itself is moved. */
    Containers::Reference<Shared> self;

    UnsignedInt styleCount, dynamicStyleCount;
    UnsignedInt(*styleTransitionToInactiveOut)(UnsignedInt) = Implementation::styleTransitionPassthrough;
    UnsignedInt(*styleTransitionToInactiveOver)(UnsignedInt) = Implementation::styleTransitionPassthrough;
    UnsignedInt(*styleTransitionToFocusedOut)(UnsignedInt) = Implementation::styleTransitionPassthrough;
    UnsignedInt(*styleTransitionToFocusedOver)(UnsignedInt) = Implementation::styleTransitionPassthrough;
    UnsignedInt(*styleTransitionToPressedOut)(UnsignedInt) = Implementation::styleTransitionPassthrough;
    UnsignedInt(*styleTransitionToPressedOver)(UnsignedInt) = Implementation::styleTransitionPassthrough;
    /* Unlike the others, this one can be nullptr, in which case the whole
       logic in doUpdate() gets skipped */
    UnsignedInt(*styleTransitionToDisabled)(UnsignedInt) = nullptr;

    /* Incremented every time the styleTransitionToDisabled pointer is changed.
       There's a corresponding styleTransitionToDisabledUpdateStamp variable in
       AbstractVisualLayer::State that doState() compares to this one,
       returning LayerState::NeedsDataUpdate if it differs. */
    UnsignedShort styleTransitionToDisabledUpdateStamp = 0;

    /* 2/6 bytes free to be used by the derived structs */
};

struct AbstractVisualLayer::State {
    explicit State(Shared::State& shared);
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

    /* Has bits set for dynamic styles that are used */
    /** @todo the allocation could be shared with subclass data */
    Containers::ArrayTuple dynamicStyleStorage;
    Containers::MutableBitArrayView dynamicStylesUsed;
    Containers::ArrayView<AnimationHandle> dynamicStyleAnimations;

    /* These views are assumed to point to subclass own data and maintained to
       have its size always match layer capacity. The `calculatedStyles` are a
       copy of `styles` with additional transitions applied for disabled
       nodes, which is performed in the layer doUpdate(). */
    Containers::StridedArrayView1D<UnsignedInt> styles, calculatedStyles;
    /* 99% of internal accesses to the Shared instance need the State struct,
       so saving it directly to avoid an extra indirection, In some cases the
       public API reference is needed (mainly for user-side access, such as
       BaseLayer::shared()), that one is referenced in State::self (and also
       correctly updated when the State instance gets moved). */
    Shared::State& shared;

    /* Set by setDefaultStyleAnimator(), gets passed to
       shared.styleTransitionAnimationOn* functions */
    AbstractVisualLayerStyleAnimator* styleAnimator = nullptr;

    /* Is compared to Shared::styleTransitionToDisabledUpdateStamp in order to
       detect that doUpdate() needs to be called to update to potentially new
       disabled styles. When the two are the same, it's assumed the
       calculatedStyles were calculated with an up-to-date transition function.

       Gets set to the shared value on construction to not implicitly mark a
       fresh layer with no data as immediately needing an update.

       The only case where an update may get skipped by accident is if the
       shared state gets 65536 style transition updates, wrapping back to 0,
       and a completely new layer gets created and updated right at that point.
       Which I think is rather unlikely, but if it wouldn't the stamps could be
       expanded to 32 bits. */
    UnsignedShort styleTransitionToDisabledUpdateStamp;

    /* 2/6 bytes free used by the derived structs */
};

}}

#endif
