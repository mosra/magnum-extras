#ifndef Magnum_Ui_BasicInstancedLayer_hpp
#define Magnum_Ui_BasicInstancedLayer_hpp
/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018
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

/** @file
 * @brief @ref compilation-speedup-hpp "Template implementation" for @ref BasicInstancedLayer.h
 */

#include "BasicInstancedLayer.h"

#include <type_traits>

namespace Magnum { namespace Ui {

template<class InstanceData> BasicInstancedLayer<InstanceData>::BasicInstancedLayer(): _size{} {
    #ifndef CORRADE_GCC47_COMPATIBILITY /* Not available on that thing */
    static_assert(std::is_trivially_destructible<InstanceData>::value, "");
    #endif
}

template<class InstanceData> BasicInstancedLayer<InstanceData>::~BasicInstancedLayer() = default;

template<class InstanceData> void BasicInstancedLayer<InstanceData>::reset(const std::size_t capacity) {
    /* Reallocate */
    if(capacity > _data.size())
        _data = Containers::Array<InstanceData>{Containers::NoInit, capacity};

    /* Reset state */
    _modified = {};
    _size = {};
}

template<class InstanceData> std::size_t BasicInstancedLayer<InstanceData>::addElement(const InstanceData& instanceData) {
    CORRADE_ASSERT(_size < _data.size(), "Ui::BasicInstancedLayer::addElement(): not enough capacity, got" << _size << "but wanted" << _size + 1, _size);

    /* Copy data to uninitalized memory */
    new(&_data[_size]) InstanceData(instanceData);

    /* Update state */
    _modified = Math::join(_modified, {_size, _size+1});

    return _size++;
}

template<class InstanceData> InstanceData& BasicInstancedLayer<InstanceData>::modifyElement(const std::size_t id) {
    CORRADE_ASSERT(id < _size, "Ui::BasicInstancedLayer::modifyElement(): ID out of range", _data[id]);

    _modified = Math::join(_modified, {id, id+1});
    return _data[id];
}

template<class InstanceData> const InstanceData& BasicInstancedLayer<InstanceData>::elementData(const std::size_t id) const {
    CORRADE_ASSERT(id < _size, "Ui::BasicInstancedLayer::elementData(): ID out of range", _data[id]);

    return _data[id];
}

}}

#endif
