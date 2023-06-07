#ifndef Magnum_Ui_BasicInstancedLayer_h
#define Magnum_Ui_BasicInstancedLayer_h
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
 * @brief Class @ref Magnum::Ui::BasicInstancedLayer
 */

#include <Corrade/Containers/Array.h>
#include <Magnum/Math/Range.h>

#include "Magnum/Ui/Ui.h"

namespace Magnum { namespace Ui {

/**
@brief Base for instanced layers

All elements in this layer have the same size and structure described by
@p InstanceData. See @ref BasicLayer for an alternative.

To use this type in a @ref BasicPlane, you have to provide a
`void draw(AbstractUiShader&)` function in a subclass that draws the contents
using given shader.
@experimental
*/
template<class InstanceData> class BasicInstancedLayer {
    public:
        /**
         * @brief Reserved instance capacity
         *
         * @see @ref size(), @ref reset()
         */
        std::size_t capacity() const { return _data.size(); }

        /**
         * @brief Occupied instance count
         *
         * @see @ref capacity(), @ref reset()
         */
        std::size_t size() const { return _size; }

        /**
         * @brief Data
         *
         * A view onto currently populated data (of size @ref size(), not
         * @ref capacity()).
         * @see @ref elementData(), @ref addElement(), @ref modifyElement()
         */
        Containers::ArrayView<const InstanceData> data() const { return {_data, _size}; }

        /**
         * @brief Modified range
         *
         * Range that needs to be updated on the GPU before drawing next frame.
         * @see @ref resetModified()
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
         * Allocates memory to store given @p capacity of instances, clearing
         * everything that has been set before. If current memory capacity is
         * larger or equal to @p capacity, no reallocation is done.
         */
        void reset(std::size_t capacity);

        /**
         * @brief Add element
         *
         * Expects that the capacity is large enough to store the instance
         * data. Returns ID of the element that can be used later to modify its
         * contents using @ref modifyElement().
         * @see @ref capacity(), @ref size(), @ref modified()
         */
        std::size_t addElement(const InstanceData& instanceData);

        /**
         * @brief Modify element
         * @param id        Element ID
         *
         * Returns data for user to modify and marks them as modified. Expects
         * that the ID is returned from previous @ref addElement() call.
         * @see @ref elementData(), @ref modified()
         */
        InstanceData& modifyElement(std::size_t id);

        /**
         * @brief Element data
         *
         * Returns constant view on the data. Expects that the ID is returned
         * from previous @ref addElement() call.
         * @see @ref data(), @ref modifyElement()
         */
        const InstanceData& elementData(std::size_t id) const;

    protected:
        explicit BasicInstancedLayer();

        ~BasicInstancedLayer();

    private:
        Containers::Array<InstanceData> _data;
        Math::Range1D<std::size_t> _modified;
        std::size_t _size;
};

}}

#endif
