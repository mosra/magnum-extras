#ifndef Magnum_Ui_BasicInstancedGLLayer_hpp
#define Magnum_Ui_BasicInstancedGLLayer_hpp
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
 * @brief @ref compilation-speedup-hpp "Template implementation" for @ref BasicInstancedGLLayer.h
 */

#include "BasicInstancedGLLayer.h"

#include "Magnum/Ui/AbstractUiShader.h"

namespace Magnum { namespace Ui {

template<class InstanceData> BasicInstancedGLLayer<InstanceData>::BasicInstancedGLLayer(): _buffer{Buffer::TargetHint::Array} {}

template<class InstanceData> BasicInstancedGLLayer<InstanceData>::~BasicInstancedGLLayer() = default;

template<class InstanceData> void BasicInstancedGLLayer<InstanceData>::reset(const std::size_t capacity, const BufferUsage usage) {
    /* Reallocate */
    if(capacity > this->capacity())
        _buffer.setData({nullptr, sizeof(InstanceData)*capacity}, usage);

    /* Reset state */
    _mesh.setInstanceCount(0);

    /* Reset the CPU side too (can't call this at the beginning because then
       the capacity would be always okay) */
    BasicInstancedLayer<InstanceData>::reset(capacity);
}

template<class InstanceData> void BasicInstancedGLLayer<InstanceData>::update() {
    if(this->modified().size().isZero()) return;

    /* Update modified instance data */
    const Math::Range1D<std::size_t> modifiedBytes = this->modified().scaled(sizeof(InstanceData));
    const auto bufferData = _buffer.map<InstanceData>(modifiedBytes.min()[0], modifiedBytes.size()[0], Buffer::MapFlag::Write|Buffer::MapFlag::InvalidateRange);
    std::uninitialized_copy(this->data() + std::size_t{this->modified().min()[0]},
                            this->data() + std::size_t{this->modified().max()[0]}, bufferData);
    _buffer.unmap();

    /* Reset modified range */
    this->resetModified();

    /* Update mesh instance count */
    _mesh.setInstanceCount(this->size());
}

template<class InstanceData> void BasicInstancedGLLayer<InstanceData>::draw(AbstractUiShader& shader) {
    _mesh.draw(shader);
}

}}

#endif
