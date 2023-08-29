#ifndef Magnum_Whee_Implementation_baseLayerState_h
#define Magnum_Whee_Implementation_baseLayerState_h
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

/* Definition of BaseLayer::State and BaseLayer::Shared::State structs to be
   used by both BaseLayer and BaseLayerGL as well as BaseLayer tests, and (if
   this header gets published) eventually possibly also 3rd party renderer
   implementations */

#include <Corrade/Containers/Array.h>

#include "Magnum/Whee/BaseLayer.h"

namespace Magnum { namespace Whee {

struct BaseLayer::Shared::State {
    explicit State(const UnsignedInt styleCount) noexcept: styleCount{styleCount} {}
    /* Assumes that the derived state struct either in BaseLayerGL::Shared or
       other GPU-API-specific implementations has non-trivially-destructible
       members. Without a virtual destructor those wouldn't be destructed
       properly when deleting from the base pointer. This is also checked with
       a static_assert() in the Pointer class itself.

       Another possibility would be to let the derived class have its own state
       allocation, but I feel that having a base with a virtual destructor is
       less nasty and nicer to caches than two loose allocations of non-virtual
       types. */
    virtual ~State() = default;

    UnsignedInt styleCount;
};

namespace Implementation {

struct BaseLayerData {
    Vector4 outlineWidth;
    Color3 color;
    UnsignedInt style;
};

struct BaseLayerVertex {
    Vector2 position;
    Vector2 centerDistance;
    Vector4 outlineWidth;
    Color3 color;
    UnsignedInt style;
};

}

struct BaseLayer::State {
    explicit State(Shared::State& shared): shared(shared) {}
    /* Assumes that the derived state struct either in BaseLayerGL or other
       GPU-API-specific implementations also has non-trivially-destructible
       members. Without a virtual destructor those wouldn't be destructed
       properly when deleting from the base pointer. This is also checked with
       a static_assert() in the Pointer class itself.

       Another possibility would be to let the derived class have its own state
       allocation, but I feel that having a base with a virtual destructor is
       less nasty and nicer to caches than two loose allocations of non-virtual
       types. */
    virtual ~State() = default;

    Containers::Array<Implementation::BaseLayerData> data;
    Containers::Array<Implementation::BaseLayerVertex> vertices;
    Containers::Array<UnsignedInt> indices;
    Shared::State& shared;
};

}}

#endif
