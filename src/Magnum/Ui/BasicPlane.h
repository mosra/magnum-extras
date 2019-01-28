#ifndef Magnum_Ui_BasicPlane_h
#define Magnum_Ui_BasicPlane_h
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
 * @brief Class @ref Magnum::Ui::AbstractPlane, @ref Magnum::Ui::BasicPlane
 */

#include <tuple>
#include <vector>
#include <Corrade/Containers/LinkedList.h>
#include <Magnum/Magnum.h>
#include <Magnum/Math/Range.h>

#include "Magnum/Ui/Ui.h"
#include "Magnum/Ui/visibility.h"

namespace Magnum { namespace Ui {

/**
@brief Plane flag

@see @ref PlaneFlags, @ref AbstractPlane::flags()
*/
enum class PlaneFlag: UnsignedInt {
    /**
     * The plane is hidden.
     * @see @ref AbstractPlane::hide(), @ref AbstractPlane::activate()
     */
    Hidden = 1 << 0
};

/** @debugoperatorenum{PlaneFlag}
 * @experimental
 */
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, PlaneFlag value);

/**
@brief Plane flags

@see @ref AbstractPlane::flags()
*/
typedef Containers::EnumSet<PlaneFlag> PlaneFlags;

CORRADE_ENUMSET_OPERATORS(PlaneFlags)

/** @debugoperatorenum{PlaneFlags}
 * @experimental
 */
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, PlaneFlags value);

/**
@brief Non-templated base for planes

See @ref BasicPlane for more information.
@experimental
*/
class MAGNUM_UI_EXPORT AbstractPlane: private Containers::LinkedListItem<AbstractPlane, AbstractUserInterface> {
    public:
        #ifdef MAGNUM_BUILD_DEPRECATED
        /** @brief @copybrief PlaneFlag
         * @deprecated Use @ref PlaneFlag instead.
         */
        CORRADE_DEPRECATED("use PlaneFlag instead") typedef PlaneFlag Flag;

        /** @brief @copybrief PlaneFlags
         * @deprecated Use @ref PlaneFlags instead.
         */
        CORRADE_DEPRECATED("use PlaneFlags instead") typedef PlaneFlags Flags;
        #endif

        /**
         * @brief Constructor
         * @param ui        User interface this plane is part of
         * @param anchor    Positioning anchor
         * @param padding   Padding for widgets inside
         * @param margin    Margin between the widgets inside
         */
        explicit AbstractPlane(AbstractUserInterface& ui, const Anchor& anchor, const Range2D& padding, const Vector2& margin);

        /** @brief User interface this plane is part of */
        AbstractUserInterface& ui() {
            return *Containers::LinkedListItem<AbstractPlane, AbstractUserInterface>::list();
        }
        const AbstractUserInterface& ui() const {
            return *Containers::LinkedListItem<AbstractPlane, AbstractUserInterface>::list();
        } /**< @overload */

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
         * @brief Flags
         *
         * @see @ref activate(), @ref hide()
         */
        PlaneFlags flags() const { return _flags; }

        /**
         * @brief Previous active plane
         *
         * Plane that was active before this plane was active. If this plane is
         * at the back of the active hierarchy or not part of the active
         * hierarchy at all, the function returns `nullptr`.
         */
        AbstractPlane* previousActivePlane();
        const AbstractPlane* previousActivePlane() const; /**< @overload */

        /**
         * @brief Next active plane
         *
         * Active plane immediately in front of current. If this plane is at
         * the front of the active hierarchy or not part of the active
         * hierarchy at all, the function returns `nullptr`.
         */
        AbstractPlane* nextActivePlane();
        const AbstractPlane* nextActivePlane() const; /**< @overload */

        /**
         * @brief Activate the plane
         *
         * Activates the plane so it is frontmost, receives input events and
         * visible. If the plane is already active, the function is a no-op.
         * @see @ref BasicUserInterface::activePlane(),
         *      @ref previousActivePlane(), @ref Flag::Hidden, @ref flags()
         */
        void activate();

        /**
         * @brief Hide the plane
         *
         * Hides the plane and transfers the focus to previously active plane.
         * If the plane is already hidden, the function is a no-op.
         * @see @ref previousActivePlane(), @ref Flag::Hidden, @ref flags()
         */
        void hide();

    protected:
        ~AbstractPlane();

    private:
        #ifndef DOXYGEN_GENERATING_OUTPUT /* https://bugzilla.gnome.org/show_bug.cgi?id=776986 */
        friend Containers::LinkedList<AbstractPlane>;
        friend Containers::LinkedListItem<AbstractPlane, AbstractUserInterface>;
        friend AbstractUserInterface;
        friend Widget;
        #endif

        /* MSVC 2015 doesn't like std::vector of undefined type so I have to
           put it here */
        struct MAGNUM_UI_LOCAL WidgetReference {
            explicit WidgetReference(const Range2D& rect, Widget* widget): rect{rect}, widget{widget} {}

            Range2D rect;
            Widget* widget;
        };

        std::size_t addWidget(Widget& widget);
        void removeWidget(std::size_t index);

        Widget* handleEvent(const Vector2& position);
        bool handleMoveEvent(const Vector2& position);
        bool handlePressEvent(const Vector2& position);
        bool handleReleaseEvent(const Vector2& position);

        Range2D _rect, _padding;
        Vector2 _margin;
        std::vector<WidgetReference> _widgets;
        Vector2 _lastCursorPosition;
        Widget *_lastHoveredWidget = nullptr,
            *_lastActiveWidget = nullptr;
        PlaneFlags _flags;
};

/**
@brief Base for planes

Each plane instance contains widgets on the same Z index and consists of layers
from which the widgets are made of. The order of layers denotes the drawing
order, first layer is drawn first.

@section Ui-BasicPlane-hierarchy Plane hierarchy

The user interface has a concept of active planes. Only one plane can be active
at a time. If a plane is active, it's receiving user input and is displayed in
front of all other inactive planes. Initially the UI has no planes, meaning
that @ref BasicUserInterface::activePlane() is returning `nullptr`. Adding
first plane to an UI will make it active. If there already is an active plane,
adding more planes to the UI will not change the active plane and the newly
added planes are added as hidden. Calling @ref hide() on a currently active
plane will make the previous plane active (if there is any) and the plane is
hidden. Calling @ref activate() on an inactive plane will bring it to the
front, make it receive input events and show it in case it was hidden.

Destroying a plane that's currently currently active makes the previous plane
active, otherwise it's just removed from the plane hierarchy.

@see @ref Widget, @ref BasicLayer
@experimental
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
         *
         * If @p ui doesn't have any active plane yet, this plane is set as
         * active. Otherwise the active plane is unchanged and this plane is
         * added as hidden.
         * @see @ref BasicUserInterface::activePlane()
         */
        explicit BasicPlane(BasicUserInterface<Layers...>& ui, const Anchor& anchor, const Range2D& padding, const Vector2& margin, Layers&... layers);

        /** @brief User interface this plane is part of */
        BasicUserInterface<Layers...>& ui();
        const BasicUserInterface<Layers...>& ui() const; /**< @overload */

        /**
         * @brief Previous active plane
         *
         * See @ref AbstractPlane::previousActivePlane() for more information.
         */
        BasicPlane<Layers...>* previousActivePlane() {
            return static_cast<BasicPlane<Layers...>*>(AbstractPlane::previousActivePlane());
        }
        const BasicPlane<Layers...>* previousActivePlane() const {
            return static_cast<const BasicPlane<Layers...>*>(AbstractPlane::previousActivePlane());
        } /**< @overload */

        /**
         * @brief Next active plane
         *
         * See @ref AbstractPlane::nextActivePlane() for more information.
         */
        BasicPlane<Layers...>* nextActivePlane() {
            return static_cast<BasicPlane<Layers...>*>(AbstractPlane::nextActivePlane());
        }
        const BasicPlane<Layers...>* nextActivePlane() const {
            return static_cast<const BasicPlane<Layers...>*>(AbstractPlane::nextActivePlane());
        } /**< @overload */

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

        /* Using StaticArray instead of StaticArrayView so the function can
           be called easily with {}. Not using initializer_list as we need to
           match the size. */
        void draw(const Matrix3& projectionMatrix, const Containers::StaticArray<sizeof...(Layers), Containers::Reference<AbstractUiShader>>& shaders);
        template<std::size_t i> void drawInternal(const Matrix3& transformationProjectionMatrix, const Containers::StaticArray<sizeof...(Layers), Containers::Reference<AbstractUiShader>>& shaders, std::integral_constant<std::size_t, i>);
        void drawInternal(const Matrix3&, const Containers::StaticArray<sizeof...(Layers), Containers::Reference<AbstractUiShader>>&, std::integral_constant<std::size_t, sizeof...(Layers)>) {}

        std::tuple<Layers&...> _layers;
};

}}

#endif
