#ifndef Magnum_Ui_UserInterface_h
#define Magnum_Ui_UserInterface_h
/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023, 2024, 2025
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
 * @brief Class @ref Magnum::Ui::UserInterface
 * @m_since_latest
 */

#include "Magnum/Ui/AbstractUserInterface.h"

namespace Magnum { namespace Ui {

/**
@brief Main user interface
@m_since_latest

Owns the whole user interface, providing everything from input event handling
to animation and drawing. Compared to @ref AbstractUserInterface provides
acccess to everything that's needed by builtin widgets, however a concrete
setup is handled by the @ref UserInterfaceGL subclass. See documentation of
either of the classes for more information.

Builtin widgets, deriving from the @ref Widget class, have access to this
instance through @ref BasicWidget::ui() and generally assume that
@ref baseLayer(), @ref textLayer(), @ref eventLayer() and @ref snapLayouter()
are available for use.
*/
class MAGNUM_UI_EXPORT UserInterface: public AbstractUserInterface {
    public:
        /** @brief Copying is not allowed */
        UserInterface(const UserInterface&) = delete;

        /**
         * @brief Move constructor
         *
         * Performs a destructive move, i.e. the original object isn't usable
         * afterwards anymore.
         */
        UserInterface(UserInterface&&) noexcept;

        /** @brief Copying is not allowed */
        UserInterface& operator=(const UserInterface&) = delete;

        /** @brief Move assignment */
        UserInterface& operator=(UserInterface&&) noexcept;

        ~UserInterface();

        /**
         * @brief Whether a base layer instance has been set
         *
         * @see @ref baseLayer(), @ref setBaseLayerInstance()
         */
        bool hasBaseLayer() const;

        /**
         * @brief Base layer instance
         *
         * Expects that an instance has been set, either by
         * @ref setBaseLayerInstance() or transitively by
         * @ref UserInterfaceGL::setStyle(), @relativeref{UserInterfaceGL,create()}
         * or a @ref UserInterfaceGL constructor taking a style instance.
         * @see @ref StyleFeature::BaseLayer
         */
        BaseLayer& baseLayer();
        const BaseLayer& baseLayer() const; /**< @overload */

        /**
         * @brief Set a base layer instance
         * @return Reference to self (for method chaining)
         *
         * Expects that the instance hasn't been set yet, either by this
         * function or transitively either by @ref UserInterfaceGL::setStyle(),
         * @relativeref{UserInterfaceGL,create()} or a @ref UserInterfaceGL
         * constructor taking a style instance. The instance is subsequently
         * available through @ref baseLayer().
         * @see @ref hasBaseLayer(), @ref StyleFeature::BaseLayer
         */
        UserInterface& setBaseLayerInstance(Containers::Pointer<BaseLayer>&& instance);

        /**
         * @brief Whether a base layer style animator instance has been set
         *
         * @see @ref baseLayerStyleAnimator(),
         *      @ref setBaseLayerStyleAnimatorInstance()
         */
        bool hasBaseLayerStyleAnimator() const;

        /**
         * @brief Base layer style animator instance
         *
         * Expects that an instance has been set, either by
         * @ref setBaseLayerStyleAnimatorInstance() or transitively by
         * @ref UserInterfaceGL::setStyle(),
         * @relativeref{UserInterfaceGL,create()} or a @ref UserInterfaceGL
         * constructor taking a style instance.
         * @see @ref StyleFeature::BaseLayerAnimations
         */
        BaseLayerStyleAnimator& baseLayerStyleAnimator();
        const BaseLayerStyleAnimator& baseLayerStyleAnimator() const; /**< @overload */

        /**
         * @brief Set a base layer style animator instance
         * @return Reference to self (for method chaining)
         *
         * Expects that a @ref BaseLayer instance is present but the animator
         * instance hasn't been set yet, either by this function or
         * transitively either by @ref UserInterfaceGL::setStyle(),
         * @relativeref{UserInterfaceGL,create()} or a @ref UserInterfaceGL
         * constructor taking a style instance.
         *
         * The instance is internally passed to @ref BaseLayer::assignAnimator(BaseLayerStyleAnimator&)
         * and made default with @ref BaseLayer::setDefaultStyleAnimator().
         * It's subsequently available through @ref baseLayerStyleAnimator(),
         * and also @ref BaseLayer::defaultStyleAnimator() unless some other
         * animator becomes set as default.
         * @see @ref hasBaseLayerStyleAnimator(),
         *      @ref StyleFeature::BaseLayerAnimations
         */
        UserInterface& setBaseLayerStyleAnimatorInstance(Containers::Pointer<BaseLayerStyleAnimator>&& instance);

        /**
         * @brief Whether a text layer instance has been set
         *
         * @see @ref textLayer(), @ref setTextLayerInstance()
         */
        bool hasTextLayer() const;

        /**
         * @brief Text layer instance
         *
         * Expects that an instance has been set, either by
         * @ref setTextLayerInstance() or transitively by
         * @ref UserInterfaceGL::setStyle(), @relativeref{UserInterfaceGL,create()}
         * or a @ref UserInterfaceGL constructor taking a style instance.
         * @see @ref StyleFeature::TextLayer
         */
        TextLayer& textLayer();
        const TextLayer& textLayer() const; /**< @overload */

        /**
         * @brief Set a text layer instance
         * @return Reference to self (for method chaining)
         *
         * Expects that the instance hasn't been set yet, either by this
         * function or transitively either by @ref UserInterfaceGL::setStyle(),
         * @relativeref{UserInterfaceGL,create()} or a @ref UserInterfaceGL
         * constructor taking a style instance. The instance is subsequently
         * available through @ref textLayer().
         * @see @ref hasTextLayer(), @ref StyleFeature::TextLayer
         */
        UserInterface& setTextLayerInstance(Containers::Pointer<TextLayer>&& instance);

        /**
         * @brief Whether a text layer style animator instance has been set
         *
         * @see @ref textLayerStyleAnimator(),
         *      @ref setTextLayerStyleAnimatorInstance()
         */
        bool hasTextLayerStyleAnimator() const;

        /**
         * @brief Text layer style animator instance
         *
         * Expects that an instance has been set, either by
         * @ref setTextLayerStyleAnimatorInstance() or transitively by
         * @ref UserInterfaceGL::setStyle(),
         * @relativeref{UserInterfaceGL,create()} or a @ref UserInterfaceGL
         * constructor taking a style instance.
         * @see @ref StyleFeature::TextLayerAnimations
         */
        TextLayerStyleAnimator& textLayerStyleAnimator();
        const TextLayerStyleAnimator& textLayerStyleAnimator() const; /**< @overload */

        /**
         * @brief Set a text layer style animator instance
         * @return Reference to self (for method chaining)
         *
         * Expects that a @ref TextLayer instance is present but the animator
         * instance hasn't been set yet, either by this function or
         * transitively either by @ref UserInterfaceGL::setStyle(),
         * @relativeref{UserInterfaceGL,create()} or a @ref UserInterfaceGL
         * constructor taking a style instance.
         *
         * The instance is internally passed to @ref TextLayer::assignAnimator(TextLayerStyleAnimator&)
         * and made default with @ref BaseLayer::setDefaultStyleAnimator().
         * It's subsequently available through @ref textLayerStyleAnimator(),
         * and also @ref TextLayer::defaultStyleAnimator() unless some other
         * animator becomes set as default.
         * @see @ref hasTextLayerStyleAnimator(),
         *      @ref StyleFeature::TextLayerAnimations
         */
        UserInterface& setTextLayerStyleAnimatorInstance(Containers::Pointer<TextLayerStyleAnimator>&& instance);

        /**
         * @brief Whether an event layer instance has been set
         *
         * @see @ref eventLayer(), @ref setEventLayerInstance()
         */
        bool hasEventLayer() const;

        /**
         * @brief Event layer instance
         *
         * Expects that an instance has been set, either by
         * @ref setEventLayerInstance() or transitively by
         * @ref UserInterfaceGL::setStyle(), @relativeref{UserInterfaceGL,create()}
         * or a @ref UserInterfaceGL constructor taking a style instance.
         * @see @ref StyleFeature::EventLayer
         */
        EventLayer& eventLayer();
        const EventLayer& eventLayer() const; /**< @overload */

        /**
         * @brief Set an event layer instance
         * @return Reference to self (for method chaining)
         *
         * Expects that the instance hasn't been set yet, either by this
         * function or transitively either by @ref UserInterfaceGL::setStyle(),
         * @relativeref{UserInterfaceGL,create()} or a @ref UserInterfaceGL
         * constructor taking a style instance. The instance is subsequently
         * available through @ref eventLayer().
         * @see @ref hasEventLayer(), @ref StyleFeature::EventLayer
         */
        UserInterface& setEventLayerInstance(Containers::Pointer<EventLayer>&& instance);

        /**
         * @brief Whether a snap layouter instance has been set
         *
         * @see @ref snapLayouter(), @ref setSnapLayouterInstance()
         */
        bool hasSnapLayouter() const;

        /**
         * @brief Snap layouter instance
         *
         * Expects that an instance has been set, either by
         * @ref setSnapLayouterInstance() or transitively by
         * @ref UserInterfaceGL::setStyle(), @relativeref{UserInterfaceGL,create()}
         * or a @ref UserInterfaceGL constructor taking a style instance.
         * @see @ref StyleFeature::SnapLayouter
         */
        SnapLayouter& snapLayouter();
        const SnapLayouter& snapLayouter() const; /**< @overload */

        /**
         * @brief Set a snap layouter instance
         * @return Reference to self (for method chaining)
         *
         * Expects that the instance hasn't been set yet, either by this
         * function or transitively either by @ref UserInterfaceGL::setStyle(),
         * @relativeref{UserInterfaceGL,create()} or a @ref UserInterfaceGL
         * constructor taking a style instance. The instance is subsequently
         * available through @ref snapLayouter().
         * @see @ref hasSnapLayouter(), @ref StyleFeature::SnapLayouter
         */
        UserInterface& setSnapLayouterInstance(Containers::Pointer<SnapLayouter>&& instance);

        /* Overloads to remove a WTF factor from method chaining order */
        #ifndef DOXYGEN_GENERATING_OUTPUT
        UserInterface& setSize(const Vector2& size, const Vector2& windowSize, const Vector2i& framebufferSize) {
            return static_cast<UserInterface&>(AbstractUserInterface::setSize(size, windowSize, framebufferSize));
        }
        template<class Application, class = decltype(Implementation::ApplicationSizeConverter<Application>::set(std::declval<AbstractUserInterface&>(), std::declval<const Application&>()))> UserInterface& setSize(const Application& application) {
            return static_cast<UserInterface&>(AbstractUserInterface::setSize(application));
        }
        UserInterface& setSize(const Vector2i& size) {
            return static_cast<UserInterface&>(AbstractUserInterface::setSize(size));
        }
        UserInterface& clean() {
            return static_cast<UserInterface&>(AbstractUserInterface::clean());
        }
        UserInterface& advanceAnimations(Nanoseconds time);
        UserInterface& update() {
            return static_cast<UserInterface&>(AbstractUserInterface::update());
        }
        UserInterface& draw() {
            return static_cast<UserInterface&>(AbstractUserInterface::draw());
        }
        #endif

    #ifdef DOXYGEN_GENERATING_OUTPUT
    private:
    #else
    protected:
    #endif
        struct State;
        Containers::Pointer<State> _state;

        MAGNUM_UI_LOCAL explicit UserInterface(NoCreateT, Containers::Pointer<State>&& state);
        /* Used by tests to avoid having to include / allocate the state */
        explicit UserInterface(NoCreateT);
};

}}

#endif
