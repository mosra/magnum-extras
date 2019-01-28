#ifndef Magnum_Ui_BasicUserInterface_h
#define Magnum_Ui_BasicUserInterface_h
/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019
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

#include <Corrade/Containers/LinkedList.h>
#include <Magnum/Magnum.h>
#include <Magnum/Math/Vector2.h>

#include "Magnum/Ui/Ui.h"
#include "Magnum/Ui/visibility.h"

namespace Magnum { namespace Ui {

/**
@brief Non-templated base for user interfaces

See @ref BasicUserInterface for more information.
@experimental
*/
class MAGNUM_UI_EXPORT AbstractUserInterface: private Containers::LinkedList<AbstractPlane> {
    public:
        /**
         * @brief Constructor
         *
         * See @ref BasicUserInterface::BasicUserInterface(const Vector2&, const Vector2i&)
         * for more information.
         */
        explicit AbstractUserInterface(const Vector2& size, const Vector2i& windowSize);

        /** @brief User interface size */
        Vector2 size() const { return _size; }

        /** @brief Coordinate scaling */
        Vector2 coordinateScaling() const { return _coordinateScaling; }

        /**
         * @brief Active plane
         *
         * If there is no active plane, returns @cpp nullptr @ce.
         * @see @ref AbstractPlane::previousActivePlane(),
         *      @ref AbstractPlane::nextActivePlane()
         */
        AbstractPlane* activePlane();
        const AbstractPlane* activePlane() const; /**< @overload */

        /** @brief Handle application mouse move event */
        bool handleMoveEvent(const Vector2i& screenPosition);

        /** @brief Handle application mouse press event */
        bool handlePressEvent(const Vector2i& screenPosition);

        /** @brief Handle application mouse release event */
        bool handleReleaseEvent(const Vector2i& screenPosition);

        /**
         * @brief Relayout the UI
         *
         * Adapts event handlers for a new size / window size. Overriden in
         * subclasses to do an actual work, see @ref BasicUserInterface::relayout()
         * for more information.
         *
         * @attention Currently, due to implementation limitations, the
         *      function expects the UI to be empty --- i.e., all planes
         *      attached to it need to be destroyed before and recreated again
         *      after. This will improve in the future.
         */
        void relayout(const Vector2& size, const Vector2i& windowSize);

    private:
        #ifndef DOXYGEN_GENERATING_OUTPUT /* https://bugzilla.gnome.org/show_bug.cgi?id=776986 */
        friend Containers::LinkedList<AbstractPlane>;
        friend Containers::LinkedListItem<AbstractPlane, AbstractUserInterface>;
        friend AbstractPlane;
        template<class ...> friend class BasicUserInterface;
        #endif

        ~AbstractUserInterface();

        std::pair<Vector2, AbstractPlane*> handleEvent(const Vector2i& screenPosition);

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
         * @param windowSize    Size of the window to which all inputs
         *      events are related
         *
         * All positioning and sizing inside the interface is done in regard to
         * @p size, without taking actual screen size into account. This allows
         * to have DPI-independent sizes.
         */
        explicit BasicUserInterface(const Vector2& size, const Vector2i& windowSize): AbstractUserInterface{size, windowSize} {}

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
        /* Using StaticArray instead of StaticArrayView so the function can
           be called easily with {}. Not using initializer_list as we need to
           match the size. */
        void draw(const Containers::StaticArray<sizeof...(Layers), Containers::Reference<AbstractUiShader>>& shaders);
};

}}

#endif
