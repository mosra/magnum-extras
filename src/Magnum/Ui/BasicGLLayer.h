#ifndef Magnum_Ui_BasicGLLayer_h
#define Magnum_Ui_BasicGLLayer_h
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

/** @file
 * @brief Class @ref Magnum::Ui::BasicGLLayer
 */

#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/Mesh.h>

#include "Magnum/Ui/BasicLayer.h"

namespace Magnum { namespace Ui {

/**
@brief Layer with OpenGL backend

Adds OpenGL mesh and buffer management on top of @ref BasicLayer.
@experimental
*/
template<class VertexData> class BasicGLLayer: public BasicLayer<VertexData> {
    public:
        explicit BasicGLLayer();

        ~BasicGLLayer();

        /** @brief Vertex data buffer */
        GL::Buffer& buffer() { return _buffer; }

        /** @brief Layer mesh */
        GL::Mesh& mesh() { return _mesh; }

        /**
         * @brief Reset the layer
         *
         * Allocates CPU and GPU memory to store given @p elementCapacity of
         * elements and @p dataCapacity of vertices, clearing everything that
         * has been set before. If current memory capacity is larger or equal
         * to @p elementCapacity/@p capacity, no reallocation is done.
         */
        void reset(std::size_t elementCapacity, std::size_t dataCapacity, GL::BufferUsage usage);

        /**
         * @brief Update a layer
         *
         * Copies all data modified using @ref modifyElement() to GPU memory.
         * Called automatically at the beginning of @ref BasicUserInterface::draw(),
         * but scheduling it explicitly in a different place might reduce the
         * need for CPU/GPU synchronization.
         */
        void update();

        /** @brief Draw the layer using provided shader */
        void draw(AbstractUiShader& shader);

    private:
        GL::Buffer _buffer;
        GL::Mesh _mesh;
};

}}

#endif
