#ifndef Magnum_Ui_Plane_h
#define Magnum_Ui_Plane_h
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
 * @brief Class @ref Magnum::Ui::Plane
 */

#include <Magnum/Text/Text.h>

#include "Magnum/Ui/BasicPlane.h"
#include "Magnum/Ui/BasicGLLayer.h"
#include "Magnum/Ui/BasicInstancedGLLayer.h"
#include "Magnum/Ui/Style.h"

namespace Magnum { namespace Ui {

/**
@brief Default UI plane

@experimental
*/
class MAGNUM_UI_EXPORT Plane: public BasicPlane<Implementation::QuadLayer, Implementation::QuadLayer, Implementation::TextLayer> {
    friend Button;
    friend Input;
    friend Label;
    friend Modal;

    public:
        /**
         * @brief Constructor
         * @param ui                    User interface this plane is part of
         * @param anchor                Positioning anchor
         */
        explicit Plane(UserInterface& ui, const Anchor& anchor);

        /**
         * @brief Construct and reserve capacity
         * @param ui                    User interface this plane is part of
         * @param anchor                Positioning anchor
         * @param backgroundCapacity    Number of background elements to reserve
         * @param foregroundCapacity    Number of foreground elements to reserve
         * @param textCapacity          Number of text glyphs to reserve
         *
         * Calls @ref reset() as part of the construction.
         */
        explicit Plane(UserInterface& ui, const Anchor& anchor, std::size_t backgroundCapacity, std::size_t foregroundCapacity, std::size_t textCapacity): Plane{ui, anchor} {
            reset(backgroundCapacity, foregroundCapacity, textCapacity);
        }

        ~Plane();

        /** @brief User interface this plane is part of */
        UserInterface& ui();
        const UserInterface& ui() const; /**< @overload */

        /**
         * @brief Previous active plane
         *
         * See @ref AbstractPlane::previousActivePlane() for more information.
         */
        Plane* previousActivePlane() {
            return static_cast<Plane*>(BasicPlane::previousActivePlane());
        }
        const Plane* previousActivePlane() const {
            return static_cast<const Plane*>(BasicPlane::previousActivePlane());
        } /**< @overload */

        /**
         * @brief Reset plane contents
         * @param backgroundCapacity    Number of background elements to reserve
         * @param foregroundCapacity    Number of foreground elements to reserve
         * @param textCapacity          Number of text glyphs to reserve
         *
         * Clears contents of the plane and reserves memory. If the memory
         * capacity is enough, no reallocation is done.
         */
        void reset(std::size_t backgroundCapacity, std::size_t foregroundCapacity, std::size_t textCapacity);

    private:
        std::size_t addText(UnsignedByte colorIndex, Float size, Containers::ArrayView<const char> text, const Vector2& cursor, Text::Alignment alignment, std::size_t capacity = 0);

        void setText(std::size_t id, UnsignedByte colorIndex, Float size, Containers::ArrayView<const char> text, const Vector2& cursor, Text::Alignment alignment);

        Implementation::QuadLayer _backgroundLayer;
        Implementation::QuadLayer _foregroundLayer;
        Implementation::TextLayer _textLayer;
};

}}

#endif
