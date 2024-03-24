#ifndef Magnum_Whee_AbstractVisualLayer_h
#define Magnum_Whee_AbstractVisualLayer_h
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
 * @brief Class @ref Magnum::Whee::AbstractVisualLayer
 * @m_since_latest
 */

#include "Magnum/Whee/AbstractLayer.h"

namespace Magnum { namespace Whee {

/**
@brief Base for visual data layers
@m_since_latest

Provides style management and style changing depending on input events for
builtin visual layers like @ref BaseLayer or @ref TextLayer.
*/
class MAGNUM_WHEE_EXPORT AbstractVisualLayer: public AbstractLayer {
    public:
        class Shared;

        /** @brief Copying is not allowed */
        AbstractVisualLayer(const AbstractVisualLayer&) = delete;

        /**
         * @brief Move constructor
         *
         * Performs a destructive move, i.e. the original object isn't usable
         * afterwards anymore.
         */
        AbstractVisualLayer(AbstractVisualLayer&&) noexcept;

        ~AbstractVisualLayer();

        /** @brief Copying is not allowed */
        AbstractVisualLayer& operator=(const AbstractVisualLayer&) = delete;

        /** @brief Move assignment */
        AbstractVisualLayer& operator=(AbstractVisualLayer&&) noexcept;

        /**
         * @brief Type-erased data style index
         *
         * Expects that @p handle is valid. The index is guaranteed to be less
         * than @ref Shared::styleCount().
         * @see @ref isHandleValid(DataHandle) const
         */
        UnsignedInt style(DataHandle handle) const;

        /**
         * @brief Data style index in a concrete enum type
         *
         * Expects that @p handle is valid. The index is guaranteed to be less
         * than @ref Shared::styleCount().
         * @see @ref isHandleValid(DataHandle) const
         */
        template<class StyleIndex> StyleIndex style(DataHandle handle) const {
            static_assert(std::is_enum<StyleIndex>::value, "expected an enum type");
            return StyleIndex(style(handle));
        }

        /**
         * @brief Type-erased data style index assuming it belongs to this layer
         *
         * Like @ref style(DataHandle) const but without checking that
         * @p handle indeed belongs to this layer. See its documentation for
         * more information.
         * @see @ref isHandleValid(LayerDataHandle) const
         */
        UnsignedInt style(LayerDataHandle handle) const;

        /**
         * @brief Data style index in a concrete enum type assuming it belongs to this layer
         *
         * Like @ref style(DataHandle) const but without checking that
         * @p handle indeed belongs to this layer. See its documentation for
         * more information.
         * @see @ref isHandleValid(LayerDataHandle) const
         */
        template<class StyleIndex> StyleIndex style(LayerDataHandle handle) const {
            static_assert(std::is_enum<StyleIndex>::value, "expected an enum type");
            return StyleIndex(style(handle));
        }

        /**
         * @brief Set data style index
         *
         * Expects that @p handle is valid and @p style is less than
         * @ref Shared::styleCount().
         *
         * Calling this function causes @ref LayerState::NeedsUpdate to be set.
         * @see @ref isHandleValid(DataHandle) const
         */
        void setStyle(DataHandle handle, UnsignedInt style);

        /**
         * @brief Set data style index in a concrete enum type
         *
         * Casts @p style to @relativeref{Magnum,UnsignedInt} and delegates to
         * @ref setStyle(DataHandle, UnsignedInt).
         */
        template<class StyleIndex
            #ifndef DOXYGEN_GENERATING_OUTPUT
            , class = typename std::enable_if<std::is_enum<StyleIndex>::value>::type
            #endif
        > void setStyle(DataHandle handle, StyleIndex style) {
            setStyle(handle, UnsignedInt(style));
        }

        /**
         * @brief Set data style index assuming it belongs to this layer
         *
         * Like @ref setStyle(DataHandle, UnsignedInt) but without checking
         * that @p handle indeed belongs to this layer. See its documentation
         * for more information.
         */
        void setStyle(LayerDataHandle handle, UnsignedInt style);

        /**
         * @brief Set data style index in a concrete enum type assuming it belongs to this layer
         *
         * Casts @p style to @relativeref{Magnum,UnsignedInt} and delegates to
         * @ref setStyle(LayerDataHandle, UnsignedInt).
         */
        template<class StyleIndex
            #ifndef DOXYGEN_GENERATING_OUTPUT
            , class = typename std::enable_if<std::is_enum<StyleIndex>::value>::type
            #endif
        > void setStyle(LayerDataHandle handle, StyleIndex style) {
            setStyle(handle, UnsignedInt(style));
        }

    #ifdef DOXYGEN_GENERATING_OUTPUT
    private:
    #else
    protected:
    #endif
        struct State;

        MAGNUM_WHEE_LOCAL explicit AbstractVisualLayer(LayerHandle handle, Containers::Pointer<State>&& state);
        /* Used by tests to avoid having to include / allocate the state */
        explicit AbstractVisualLayer(LayerHandle handle, Shared& shared);

        /* Can't be MAGNUM_WHEE_LOCAL otherwise deriving from this class in
           tests causes linker errors */
        LayerFeatures doFeatures() const override;

        Containers::Pointer<State> _state;

    private:
        MAGNUM_WHEE_LOCAL void setStyleInternal(UnsignedInt id, UnsignedInt style);

        /* Can't be MAGNUM_WHEE_LOCAL otherwise deriving from this class in
           tests causes linker errors */
        void doPointerPressEvent(UnsignedInt dataId, PointerEvent& event) override;
        void doPointerReleaseEvent(UnsignedInt dataId, PointerEvent& event) override;
        void doPointerMoveEvent(UnsignedInt dataId, PointerMoveEvent& event) override;
        void doPointerEnterEvent(UnsignedInt dataId, PointerMoveEvent& event) override;
        void doPointerLeaveEvent(UnsignedInt dataId, PointerMoveEvent& event) override;
};

/**
@brief Base shared state for visual data layers

Stores style transition functions.
*/
class MAGNUM_WHEE_EXPORT AbstractVisualLayer::Shared {
    public:
        /** @brief Copying is not allowed */
        Shared(const Shared&) = delete;

        /**
         * @brief Move constructor
         *
         * Performs a destructive move, i.e. the original object isn't usable
         * afterwards anymore.
         */
        Shared(Shared&&) noexcept;

        ~Shared();

        /** @brief Copying is not allowed */
        Shared& operator=(const Shared&) = delete;

        /** @brief Move assignment */
        Shared& operator=(Shared&&) noexcept;

        /**
         * @brief Style count
         *
         * @see @ref BaseLayerGL::Shared::Shared(UnsignedInt, UnsignedInt),
         *      @ref TextLayerGL::Shared::Shared(UnsignedInt, UnsignedInt),
         *      @ref BaseLayer::Shared::setStyle(),
         *      @ref TextLayer::Shared::setStyle()
         */
        UnsignedInt styleCount() const;

        /**
         * @brief Set type-erased style transition functions
         * @return Reference to self (for method chaining)
         *
         * The @p toPressedBlur and @p toPressedHover change a style index to a
         * pressed blurred or hovered one, for example after a pointer was
         * pressed on a hovered button, after an activated but blur button was
         * pressed via a keyboard, but also after a pointer leaves a pressed
         * button, making it blur or re-enters it, making it hovered again.
         *
         * The @p toInactiveBlur and @p toInactiveHover change a style index to
         * an inactive blurred or hovered one, for example when a mouse enters
         * or leaves an area of otherwise inactive and not pressed button, but
         * also when a button is released again or an input is no longer
         * active.
         *
         * If any of the functions is @cpp nullptr @ce, given transition is
         * a no-op, keeping the same index.
         *
         * For correct behavior, all functions should be mutually invertible,
         * e.g. @cpp toPressedHover(toInactiveBlur(style)) == style @ce if the
         * `style` was a pressed hovered style to begin with (and both
         * transition functions were defined). If the style doesn't handle
         * hover in any way, for example for touch-only interfaces, you can
         * use @ref setStyleTransition() "setStyleTransition(UnsignedInt(*)(UnsignedInt), UnsignedInt(*)(UnsignedInt))"
         * instead, which doesn't make any distinction between the hover and
         * blur states and uses the same transition function for both.
         */
        Shared& setStyleTransition(UnsignedInt(*toPressedBlur)(UnsignedInt), UnsignedInt(*toPressedHover)(UnsignedInt), UnsignedInt(*toInactiveBlur)(UnsignedInt), UnsignedInt(*toInactiveHover)(UnsignedInt));

        /**
         * @brief Set style transition functions
         * @return Reference to self (for method chaining)
         *
         * Like @ref setStyleTransition() "setStyleTransition(UnsignedInt(*)(UnsignedInt), UnsignedInt(*)(UnsignedInt), UnsignedInt(*)(UnsignedInt), UnsignedInt(*)(UnsignedInt))",
         * but allows to use a concrete enum type instead of a typeless index.
         * Same as with the type-erased variant, if any of the function
         * template parameters is @cpp nullptr @ce, given transition is a
         * no-op, keeping the same index. Example usage:
         *
         * @snippet Whee.cpp AbstractVisualLayer-Shared-setStyleTransition
         */
        template<class StyleIndex, StyleIndex(*toPressedBlur)(StyleIndex), StyleIndex(*toPressedHover)(StyleIndex), StyleIndex(*toInactiveBlur)(StyleIndex), StyleIndex(*toInactiveHover)(StyleIndex)> Shared& setStyleTransition();

        /**
         * @brief Set style transition functions without hover state
         * @return Reference to self (for method chaining)
         *
         * Same as calling @ref setStyleTransition() "setStyleTransition(UnsignedInt(*)(UnsignedInt), UnsignedInt(*)(UnsignedInt), UnsignedInt(*)(UnsignedInt), UnsignedInt(*)(UnsignedInt))"
         * with @p toPressed used for both @p toPressedBlur and
         * @p toPressedHover and @p toInactive used for both @p toInactiveBlur
         * and @p toInactiveHover. Useful in case the style doesn't handle
         * hover in any way, for example for touch-only interfaces.
         */
        Shared& setStyleTransition(UnsignedInt(*toPressed)(UnsignedInt), UnsignedInt(*toInactive)(UnsignedInt)) {
            return setStyleTransition(toPressed, toPressed, toInactive, toInactive);
        }

        /**
         * @brief Set style transition functions
         * @return Reference to self (for method chaining)
         *
         * Same as calling @ref setStyleTransition() with @p toPressed used for
         * both @p toPressedBlur and @p toPressedHover and @p toInactive used
         * for both @p toInactiveBlur and @p toInactiveHover. Useful in case
         * the style doesn't handle hover in any way, for example for
         * touch-only interfaces.
         */
        template<class StyleIndex, StyleIndex(*toPressed)(StyleIndex), StyleIndex(*toInactive)(StyleIndex)> Shared& setStyleTransition() {
            return setStyleTransition<StyleIndex, toPressed, toPressed, toInactive, toInactive>();
        }

    #ifdef DOXYGEN_GENERATING_OUTPUT
    private:
    #else
    protected:
    #endif
        struct State;
        friend AbstractVisualLayer;

        MAGNUM_WHEE_LOCAL explicit Shared(Containers::Pointer<State>&& state);
        /* Used by tests to avoid having to include / allocate the state */
        explicit Shared(UnsignedInt styleCount);
        /* Can't be MAGNUM_WHEE_LOCAL, used by tests */
        explicit Shared(NoCreateT) noexcept;

        Containers::Pointer<State> _state;
};

#ifndef DOXYGEN_GENERATING_OUTPUT
#define MAGNUMEXTRAS_WHEE_ABSTRACTVISUALLAYER_SHARED_SUBCLASS_IMPLEMENTATION() \
    Shared& setStyleTransition(UnsignedInt(*toPressedBlur)(UnsignedInt), UnsignedInt(*toPressedHover)(UnsignedInt), UnsignedInt(*toInactiveBlur)(UnsignedInt), UnsignedInt(*toInactiveHover)(UnsignedInt)) { \
        return static_cast<Shared&>(AbstractVisualLayer::Shared::setStyleTransition(toPressedBlur, toPressedHover, toInactiveBlur, toInactiveHover)); \
    }                                                                       \
    template<class StyleIndex, StyleIndex(*toPressedBlur)(StyleIndex), StyleIndex(*toPressedHover)(StyleIndex), StyleIndex(*toInactiveBlur)(StyleIndex), StyleIndex(*toInactiveHover)(StyleIndex)> Shared& setStyleTransition() { \
        return static_cast<Shared&>(AbstractVisualLayer::Shared::setStyleTransition<StyleIndex, toPressedBlur, toPressedHover, toInactiveBlur, toInactiveHover>()); \
    }                                                                       \
    Shared& setStyleTransition(UnsignedInt(*toPressed)(UnsignedInt), UnsignedInt(*toInactive)(UnsignedInt)) { \
        return static_cast<Shared&>(AbstractVisualLayer::Shared::setStyleTransition(toPressed, toInactive)); \
    }                                                                       \
    template<class StyleIndex, StyleIndex(*toPressed)(StyleIndex), StyleIndex(*toInactive)(StyleIndex)> Shared& setStyleTransition() { \
        return static_cast<Shared&>(AbstractVisualLayer::Shared::setStyleTransition<StyleIndex, toPressed, toInactive>()); \
    }
#endif

/* The damn thing fails to match these to the declarations above */
#ifndef DOXYGEN_GENERATING_OUTPUT
template<class StyleIndex, StyleIndex(*toPressedBlur)(StyleIndex), StyleIndex(*toPressedHover)(StyleIndex), StyleIndex(*toInactiveBlur)(StyleIndex), StyleIndex(*toInactiveHover)(StyleIndex)> AbstractVisualLayer::Shared& AbstractVisualLayer::Shared::setStyleTransition() {
    /* No matter what simplification I do, GCC warns about "implicit conversion
       to bool", so it's this obvious ugly == here. There could be + for the
       lambdas to turn them into function pointers to avoid ?: getting
       confused, but that doesn't work on MSVC 2015 so instead it's the nullptr
       being cast. */
    /** @todo revert back to the + once CORRADE_MSVC2015_COMPATIBILITY is
        dropped */
    return setStyleTransition(
        toPressedBlur == nullptr ? static_cast<UnsignedInt(*)(UnsignedInt)>(nullptr) : [](UnsignedInt index) {
            return UnsignedInt(toPressedBlur(StyleIndex(index)));
        },
        toPressedHover == nullptr ? static_cast<UnsignedInt(*)(UnsignedInt)>(nullptr) : [](UnsignedInt index) {
            return UnsignedInt(toPressedHover(StyleIndex(index)));
        },
        toInactiveBlur == nullptr ? static_cast<UnsignedInt(*)(UnsignedInt)>(nullptr) : [](UnsignedInt index) {
            return UnsignedInt(toInactiveBlur(StyleIndex(index)));
        },
        toInactiveHover == nullptr ? static_cast<UnsignedInt(*)(UnsignedInt)>(nullptr) : [](UnsignedInt index) {
            return UnsignedInt(toInactiveHover(StyleIndex(index)));
        });
}
#endif

}}

#endif
