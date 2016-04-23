#ifndef Magnum_Ui_BasicGLLayer_hpp
#define Magnum_Ui_BasicGLLayer_hpp
/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016
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
 * @brief @ref compilation-speedup-hpp "Template implementation" for @ref BasicGLLayer.h
 */

#include "BasicGLLayer.h"

#include "Magnum/Ui/AbstractUiShader.h"

namespace Magnum { namespace Ui {

template<class VertexData> BasicGLLayer<VertexData>::BasicGLLayer(): _buffer{Buffer::TargetHint::Array} {}

template<class VertexData> BasicGLLayer<VertexData>::~BasicGLLayer() = default;

template<class VertexData> void BasicGLLayer<VertexData>::reset(const std::size_t elementCapacity, const std::size_t dataCapacity, const BufferUsage usage) {
    /* Reallocate the buffer, if needed */
    if(dataCapacity > this->capacity())
        _buffer.setData({nullptr, sizeof(VertexData)*dataCapacity}, usage);

    /* Reset state */
    _mesh.setCount(0);

    /* Reset the CPU side too (can't call this at the beginning because then
       the capacity would be always okay) */
    BasicLayer<VertexData>::reset(elementCapacity, dataCapacity);
}

template<class VertexData> void BasicGLLayer<VertexData>::update() {
    if(this->modified().size().isZero()) return;

    /* Upload modified vertex data */
    const Math::Range1D<std::size_t> modifiedBytes = this->modified().scaled(sizeof(VertexData));
    const auto bufferData = _buffer.map<VertexData>(modifiedBytes.min()[0], modifiedBytes.size()[0], Buffer::MapFlag::Write|Buffer::MapFlag::InvalidateRange);
    std::uninitialized_copy(this->data() + std::size_t{this->modified().min()[0]},
                            this->data() + std::size_t{this->modified().max()[0]}, bufferData);
    _buffer.unmap();

    /* Reset modified range */
    this->resetModified();

    /* Update mesh index count */
    _mesh.setCount(this->indexCount());
}

template<class VertexData> void BasicGLLayer<VertexData>::draw(AbstractUiShader& shader) {
    _mesh.draw(shader);
}

}}

#endif
