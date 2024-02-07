#ifndef Magnum_Whee_UserInterface_h
#define Magnum_Whee_UserInterface_h
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
 * @brief Class @ref Magnum::Whee::UserInterface
 * @m_since_latest
 */

#include "Magnum/Whee/AbstractUserInterface.h"

namespace Magnum { namespace Whee {

/**
@brief Main user interface
@m_since_latest

Provides an interface for setting up and querying @ref BaseLayer and
@ref TextLayer instances for use by builtin widgets. You'll most likely
instantiate the class through @ref UserInterfaceGL, which populates the
instance with concrete OpenGL implementations of the renderer and builtin
layers.
*/
class MAGNUM_WHEE_EXPORT UserInterface: public AbstractUserInterface {
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
         * @ref UserInterfaceGL::setStyle() or a @ref UserInterfaceGL
         * constructor taking a style instance.
         * @see @ref StyleFeature::BaseLayer
         */
        BaseLayer& baseLayer();
        const BaseLayer& baseLayer() const; /**< @overload */

        /**
         * @brief Set a base layer instance
         * @return Reference to self (for method chaining)
         *
         * Expects that the instance hasn't been set yet, either by this
         * function or transitively either by @ref UserInterfaceGL::setStyle()
         * or a @ref UserInterfaceGL constructor taking a style instance. The
         * instance is subsequently available through @ref baseLayer().
         * @see @ref hasBaseLayer(), @ref StyleFeature::BaseLayer
         */
        UserInterface& setBaseLayerInstance(Containers::Pointer<BaseLayer>&& instance);

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
         * @ref UserInterfaceGL::setStyle() or a @ref UserInterfaceGL
         * constructor taking a style instance.
         * @see @ref StyleFeature::TextLayer
         */
        TextLayer& textLayer();
        const TextLayer& textLayer() const; /**< @overload */

        /**
         * @brief Set a text layer instance
         * @return Reference to self (for method chaining)
         *
         * Expects that the instance hasn't been set yet, either by this
         * function or transitively either by @ref UserInterfaceGL::setStyle()
         * or a @ref UserInterfaceGL constructor taking a style instance. The
         * instance is subsequently available through @ref textLayer().
         * @see @ref hasTextLayer(), @ref StyleFeature::TextLayer
         */
        UserInterface& setTextLayerInstance(Containers::Pointer<TextLayer>&& instance);

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
         * @ref UserInterfaceGL::setStyle() or a @ref UserInterfaceGL
         * constructor taking a style instance.
         * @see @ref StyleFeature::EventLayer
         */
        EventLayer& eventLayer();
        const EventLayer& eventLayer() const; /**< @overload */

        /**
         * @brief Set an event layer instance
         * @return Reference to self (for method chaining)
         *
         * Expects that the instance hasn't been set yet, either by this
         * function or transitively either by @ref UserInterfaceGL::setStyle()
         * or a @ref UserInterfaceGL constructor taking a style instance. The
         * instance is subsequently available through @ref eventLayer().
         * @see @ref hasEventLayer(), @ref StyleFeature::EventLayer
         */
        UserInterface& setEventLayerInstance(Containers::Pointer<EventLayer>&& instance);

        /* Overloads to remove a WTF factor from method chaining order */
        #ifndef DOXYGEN_GENERATING_OUTPUT
        UserInterface& setSize(const Vector2& size, const Vector2& windowSize, const Vector2i& framebufferSize) {
            return static_cast<UserInterface&>(AbstractUserInterface::setSize(size, windowSize, framebufferSize));
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

        MAGNUM_WHEE_LOCAL explicit UserInterface(NoCreateT, Containers::Pointer<State>&& state);
        /* Used by tests to avoid having to include / allocate the state */
        explicit UserInterface(NoCreateT);
};

}}

#endif
