#ifndef Magnum_Ui_BasicUserInterface_h
#define Magnum_Ui_BasicUserInterface_h
/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017
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
 * @brief Class @ref Magnum::Ui::AbstractUserInterface, @ref Magnum::Ui::BasicUserInterface
 */

#include <array>
#include <functional>
#include <vector>
#include <Magnum/Magnum.h>
#include <Magnum/Math/Range.h>

#include "Magnum/Ui/Ui.h"
#include "Magnum/Ui/visibility.h"

namespace Magnum { namespace Ui {

/**
@brief Non-templated base for user interfaces

See @ref BasicUserInterface for more information.
@experimental
*/
class MAGNUM_UI_EXPORT AbstractUserInterface {
    friend AbstractPlane;
    template<class ...> friend class BasicUserInterface;

    public:
        /**
         * @brief Constructor
         *
         * See @ref BasicUserInterface::BasicUserInterface(const Vector2&, const Vector2i&)
         * for more information.
         */
        explicit AbstractUserInterface(const Vector2& size, const Vector2i& screenSize);

        /** @brief User interface size */
        Vector2 size() const { return _size; }

        /**
         * @brief Active plane
         *
         * @see @ref AbstractPlane::previousActivePlane()
         */
        AbstractPlane* activePlane() { return _activePlane; }
        const AbstractPlane* activePlane() const { return _activePlane; } /**< @overload */

        /** @brief Handle application mouse move event */
        bool handleMoveEvent(const Vector2i& screenPosition);

        /** @brief Handle application mouse press event */
        bool handlePressEvent(const Vector2i& screenPosition);

        /** @brief Handle application mouse release event */
        bool handleReleaseEvent(const Vector2i& screenPosition);

    private:
        /* MSVC 2015 doesn't like std::vector of undefined type so I have to
           put it here. It also doesn't like having non-assignable types in it,
           so I have to use std::reference_wrapper. */
        struct PlaneReference {
            explicit PlaneReference(const Range2D& rect, AbstractPlane& plane): rect{rect}, plane(plane) {}

            Range2D rect;
            std::reference_wrapper<AbstractPlane> plane;
        };

        ~AbstractUserInterface();

        AbstractPlane* addPlane(AbstractPlane& plane);

        std::pair<Vector2, AbstractPlane*> handleEvent(const Vector2i& screenPosition);

        std::vector<PlaneReference> _planes;
        AbstractPlane* _activePlane = nullptr;
        Vector2 _size;
        Vector2 _coordinateScaling;
};

/**
@brief Base for user interfaces

User interface fills up whole screen and consists of planes with specific
layers.
@experimental
*/
template<class ...Layers> class BasicUserInterface: public AbstractUserInterface {
    public:
        /**
         * @brief Constructor
         * @param size          User interface size
         * @param screenSize    Actual screen size
         *
         * All positioning and sizing inside the interface is done in regard to
         * @p size, without taking actual screen size into account. This allows
         * to have DPI-independent sizes.
         */
        explicit BasicUserInterface(const Vector2& size, const Vector2i& screenSize): AbstractUserInterface{size, screenSize} {}

        /**
         * @brief Update the interface
         *
         * Calls @ref BasicPlane::update() on all planes in the interface.
         * Called automatically at the beginning of @ref draw(), but scheduling
         * it explicitly in a different place might reduce the need for CPU/GPU
         * synchronization.
         */
        void update();

    protected:
        ~BasicUserInterface();

        /** @brief Draw the interface using designated shader for each layer */
        void draw(const std::array<std::reference_wrapper<AbstractUiShader>, sizeof...(Layers)>& shaders);
};

}}

#endif
