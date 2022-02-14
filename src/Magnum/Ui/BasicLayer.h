#ifndef Magnum_Ui_BasicLayer_h
#define Magnum_Ui_BasicLayer_h
/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022 Vladimír Vondruš <mosra@centrum.cz>

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
 * @brief Class @ref Magnum::Ui::BasicLayer
 */

#include <Corrade/Containers/Array.h>
#include <Magnum/Math/Range.h>

#include "Magnum/Ui/Ui.h"

namespace Magnum { namespace Ui {

/**
@brief Basic layer

Elements in this layer can have variable size at a cost of duplicating
per-element data for each vertex. Updating an element later with shorter size
also means that the remaining vertices will still be drawn but need to be
hidden somehow.

To use this type in a @ref BasicPlane, you have to provide a
`void draw(AbstractUiShader&)` function in a subclass that draws the contents
using given shader.
@experimental
*/
template<class VertexData> class BasicLayer {
    public:
        /**
         * @brief Reserved vertex capacity
         *
         * @see @ref size(), @ref reset()
         */
        std::size_t capacity() const { return _data.size(); }

        /**
         * @brief Reserved element capacity
         *
         * @see @ref elementCount(), @ref reset()
         */
        std::size_t elementCapacity() const { return _elementOffset.size(); }

        /**
         * @brief Occupied vertex count
         *
         * @see @ref capacity(), @ref reset()
         */
        std::size_t size() const { return _size; }

        /**
         * @brief Occupied element count
         *
         * @see @ref capacity(), @ref reset()
         */
        std::size_t elementCount() const { return _elementCount; }

        /**
         * @brief Index count to draw
         *
         * Sum of all @p count parameters passed to @ref addElement(). To be
         * used as index count when drawing the mesh. If your layer doesn't use
         * indexed draw, you can ignore this field.
         */
        std::size_t indexCount() const { return _indexCount; }

        /**
         * @brief Data
         *
         * A view onto currently populated data (of size @ref size(), not
         * @ref capacity()).
         * @see @ref elementData(), @ref addElement(), @ref modifyElement()
         */
        Containers::ArrayView<const VertexData> data() const { return {_data, _size}; }

        /**
         * @brief Modified range
         *
         * Range that needs to be updated on the GPU before drawing next frame.
         @ref resetModified()
         */
        Math::Range1D<std::size_t> modified() const { return _modified; }

        /**
         * @brief Reset the modified range
         *
         * Call after uploading the modified data onto the GPU to clear the
         * modifier range for next frame.
         * @see @ref modified()
         */
        void resetModified() { _modified = {}; }

        /**
         * @brief Reset the layer
         *
         * Allocates memory to store given @p elementCapacity of instances and
         * @p dataCapacity of vertices, clearing everything that has been set
         * before. If current memory capacity is larger or equal to
         * @p elementCapacity / @p capacity, no reallocation is done.
         */
        void reset(std::size_t elementCapacity, std::size_t dataCapacity);

        /**
         * @brief Add element
         * @param data          Widget vertex data
         * @param indexCount    Actual index count for the vertex data. If your
         *      layer doesn't use  indexed draw, you can set any value here.
         *
         * Expects that the capacity is large enough to store the vertex data.
         * Returns ID of the element that can be used later to modify its
         * contents.
         * @see @ref capacity(), @ref elementCapacity(), @ref size(),
         *      @ref elementCount(), @ref modifyElement(), @ref modified()
         */
        std::size_t addElement(Containers::ArrayView<const VertexData> data, std::size_t indexCount);

        /**
         * @brief Modify element
         * @param id            Element ID
         *
         * Marks returned data range as modified. Expects that the ID is
         * returned from previous @ref addElement() call.
         * @see @ref elementData(), @ref modified()
         */
        Containers::ArrayView<VertexData> modifyElement(std::size_t id);

        /**
         * @brief Element size
         * @param id            Element ID
         *
         * Expects that the ID is returned from previous @ref addElement()
         * call.
         */
        std::size_t elementSize(std::size_t id) const;

        /**
         * @brief Element data
         * @param id            Element ID
         *
         * Returns constant view on the data. Expects that the ID is returned
         * from previous @ref addElement() call.
         * @see @ref data(), @ref modifyElement()
         */
        Containers::ArrayView<const VertexData> elementData(std::size_t id) const;

    protected:
        explicit BasicLayer();

        ~BasicLayer();

    private:
        Containers::Array<VertexData> _data;
        Containers::Array<std::size_t> _elementOffset;
        Math::Range1D<std::size_t> _modified;
        std::size_t _elementCount, _size, _indexCount;
};

}}

#endif
