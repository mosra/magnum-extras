#ifndef Magnum_Ui_BasicPlane_h
#define Magnum_Ui_BasicPlane_h
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
 * @brief Class @ref Magnum::Ui::AbstractPlane, @ref Magnum::Ui::BasicPlane
 */

#include <tuple>
#include <vector>
#include <Magnum/Magnum.h>
#include <Magnum/Math/Range.h>

#include "Magnum/Ui/Ui.h"
#include "Magnum/Ui/visibility.h"

namespace Magnum { namespace Ui {

/**
@brief Non-templated base for planes

See @ref BasicPlane for more information.
*/
class MAGNUM_UI_EXPORT AbstractPlane {
    friend AbstractUserInterface;
    friend Widget;

    public:
        /**
         * @brief State flag
         *
         * @see @ref Flags, @ref flags()
         */
        enum class Flag: UnsignedInt {
            /**
             * The plane is hidden.
             * @see @ref hide(), @ref activate()
             */
            Hidden = 1 << 0
        };

        /**
         * @brief State flags
         *
         * @see @ref flags()
         */
        typedef Containers::EnumSet<Flag> Flags;

        /**
         * @brief Constructor
         * @param ui        User interface this plane is part of
         * @param anchor    Positioning anchor
         * @param padding   Padding for widgets inside
         * @param margin    Margin between the widgets inside
         */
        explicit AbstractPlane(AbstractUserInterface& ui, const Anchor& anchor, const Range2D& padding, const Vector2& margin);

        /** @brief User interface this plane is part of */
        AbstractUserInterface& ui() { return _ui; }
        const AbstractUserInterface& ui() const { return _ui; } /**< @overload */

        /** @brief Plane rectangle */
        Range2D rect() const { return _rect; }

        /** @brief Padding for widgets inside */
        Range2D padding() const { return _padding; }

        /**
         * @brief Margin between the widgets inside
         *
         * Used also as margin around the plane.
         */
        Vector2 margin() const { return _margin; }

        /**
         * @brief State flags
         *
         * @see @ref activate(), @ref hide()
         */
        Flags flags() const { return _flags; }

        /**
         * @brief Activate the plane
         *
         * Activates the plane so it is frontmost, receives input events and
         * visible.
         * @see @ref Flag::Hidden, @ref flags()
         */
        void activate();

        /**
         * @brief Hide the plane
         *
         * Hides the plane and transfers the focus to previously active plane.
         * Expects that the plane is currently active. If the plane is already
         * hidden, the function is a no-op.
         * @see @ref Flag::Hidden, @ref flags()
         */
        void hide();

    protected:
        ~AbstractPlane();

    private:
        struct WidgetReference;

        std::size_t addWidget(Widget& widget);
        void removeWidget(std::size_t index);

        Widget* handleEvent(const Vector2& position);
        bool handleMoveEvent(const Vector2& position);
        bool handlePressEvent(const Vector2& position);
        bool handleReleaseEvent(const Vector2& position);

        AbstractUserInterface& _ui;
        Range2D _rect, _padding;
        Vector2 _margin;
        std::vector<WidgetReference> _widgets;
        Vector2 _lastCursorPosition;
        Widget *_lastHoveredWidget = nullptr,
            *_lastActiveWidget = nullptr;
        AbstractPlane* _lastActivePlane;
        Flags _flags;
};

CORRADE_ENUMSET_OPERATORS(AbstractPlane::Flags)

/**
@brief Base for planes

Each plane instance contains widgets on the same Z index and consists of layers
from which the widgets are made of. The order of layers denotes the drawing
order, first layer is drawn first.
@see @ref Widget, @ref BasicLayer
*/
template<class ...Layers> class BasicPlane: public AbstractPlane {
    friend BasicUserInterface<Layers...>;

    public:
        /**
         * @brief Constructor
         * @param ui        User interface this plane is part of
         * @param anchor    Positioning anchor
         * @param padding   Padding for widgets inside
         * @param margin    Margin between the widgets inside
         * @param layers    Layers the widgets on this plane are made of
         */
        explicit BasicPlane(BasicUserInterface<Layers...>& ui, const Anchor& anchor, const Range2D& padding, const Vector2& margin, Layers&... layers): AbstractPlane{ui, anchor, padding, margin}, _layers{layers...} {}

        /** @brief User interface this plane is part of */
        BasicUserInterface<Layers...>& ui();
        const BasicUserInterface<Layers...>& ui() const; /**< @overload */

        /**
         * @brief Update a plane
         *
         * Calls @ref BasicGLLayer::update() "Basic*Layer::update()" on all
         * layers in the plane. Called automatically at the beginning of
         * @ref BasicUserInterface::draw(), but scheduling it explicitly in a
         * different place might reduce the need for CPU/GPU synchronization.
         */
        void update();

    protected:
        ~BasicPlane();

    private:
        template<std::size_t i> void updateInternal(std::integral_constant<std::size_t, i>);
        void updateInternal(std::integral_constant<std::size_t, sizeof...(Layers)>) {}

        void draw(const Matrix3& projectionMatrix, const std::array<std::reference_wrapper<AbstractUiShader>, sizeof...(Layers)>& shaders);
        template<std::size_t i> void drawInternal(const Matrix3& transformationProjectionMatrix, const std::array<std::reference_wrapper<AbstractUiShader>, sizeof...(Layers)>& shaders, std::integral_constant<std::size_t, i>);
        void drawInternal(const Matrix3&, const std::array<std::reference_wrapper<AbstractUiShader>, sizeof...(Layers)>&, std::integral_constant<std::size_t, sizeof...(Layers)>) {}

        std::tuple<Layers&...> _layers;
};

}}

#endif
