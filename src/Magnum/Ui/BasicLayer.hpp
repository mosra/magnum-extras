#ifndef Magnum_Ui_BasicLayer_hpp
#define Magnum_Ui_BasicLayer_hpp
/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020 Vladimír Vondruš <mosra@centrum.cz>

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
 * @brief @ref compilation-speedup-hpp "Template implementation" for @ref BasicLayer.h
 */

#include "BasicLayer.h"

#include <memory>
#include <type_traits>

namespace Magnum { namespace Ui {

template<class VertexData> BasicLayer<VertexData>::BasicLayer(): _elementCount{}, _size{}, _indexCount{} {
    static_assert(std::is_trivially_destructible<VertexData>::value, "");
}

template<class VertexData> BasicLayer<VertexData>::~BasicLayer() = default;

template<class VertexData> void BasicLayer<VertexData>::reset(const std::size_t elementCapacity, const std::size_t dataCapacity) {
    /* Reallocate, if needed */
    if(elementCapacity > _elementOffset.size())
        _elementOffset = Containers::Array<std::size_t>{Containers::NoInit, elementCapacity};
    if(dataCapacity > capacity())
        _data = Containers::Array<VertexData>{Containers::NoInit, dataCapacity};

    /* Reset state */
    _modified = {};
    _elementCount = _size = _indexCount = {};
}

template<class VertexData> std::size_t BasicLayer<VertexData>::addElement(const Containers::ArrayView<const VertexData> vertexData, std::size_t indexCount) {
    CORRADE_ASSERT(_elementCount < _elementOffset.size(), "Ui::BasicLayer::addElement(): not enough element capacity, got" << _elementCount << "but wanted" << _elementCount + 1, _size);
    CORRADE_ASSERT(_size + vertexData.size() <= capacity(), "Ui::BasicLayer::addElement(): not enough data capacity, got" << capacity() << "but wanted" << _size + vertexData.size(), _size);

    /* Copy data */
    std::uninitialized_copy(vertexData.begin(), vertexData.end(), _data + _size);

    /* Update state */
    _modified = Math::join(_modified, {_size, _size + vertexData.size()});
    _elementOffset[_elementCount] = _size;
    _size += vertexData.size();
    _indexCount += indexCount;

    return _elementCount++;
}

template<class VertexData> Containers::ArrayView<VertexData> BasicLayer<VertexData>::modifyElement(const std::size_t id) {
    CORRADE_ASSERT(id < _size, "Ui::BasicLayer::modifyElement(): ID out of range", {});

    _modified = Math::join(_modified, Math::Range1D<std::size_t>::fromSize(_elementOffset[id], elementSize(id)));
    return {_data + _elementOffset[id], elementSize(id)};
}

template<class VertexData> std::size_t BasicLayer<VertexData>::elementSize(const std::size_t id) const {
    CORRADE_ASSERT(id < _size, "Ui::BasicLayer::elementSize(): ID out of range", {});

    return (id == _elementCount - 1 ? _size : _elementOffset[id + 1]) - _elementOffset[id];
}

template<class VertexData> Containers::ArrayView<const VertexData> BasicLayer<VertexData>::elementData(const std::size_t id) const {
    CORRADE_ASSERT(id < _size, "Ui::BasicLayer::elementData(): ID out of range", {});

    return _data.slice(_elementOffset[id], id == _elementCount - 1 ? _size : _elementOffset[id + 1]);
}

}}

#endif
