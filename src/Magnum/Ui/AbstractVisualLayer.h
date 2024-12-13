#ifndef Magnum_Ui_AbstractVisualLayer_h
#define Magnum_Ui_AbstractVisualLayer_h
/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023, 2024
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
 * @brief Class @ref Magnum::Ui::AbstractVisualLayer
 * @m_since_latest
 */

#include "Magnum/Ui/AbstractLayer.h"

namespace Magnum { namespace Ui {

/**
@brief Base for visual data layers
@m_since_latest

Provides style management and style changing depending on primary &
non-fallthrough input events for builtin visual layers like @ref BaseLayer or
@ref TextLayer.
*/
class MAGNUM_UI_EXPORT AbstractVisualLayer: public AbstractLayer {
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
         * @brief Shared state used by this layer
         *
         * Reference to the instance passed to @ref BaseLayerGL::BaseLayerGL(LayerHandle, Shared&)
         * or @ref TextLayerGL::TextLayerGL(LayerHandle, Shared&).
         */
        Shared& shared();
        const Shared& shared() const; /**< @overload */

        /**
         * @brief Type-erased data style index
         *
         * Expects that @p handle is valid. The index is guaranteed to be less
         * than @ref Shared::totalStyleCount().
         * @see @ref isHandleValid(DataHandle) const
         */
        UnsignedInt style(DataHandle handle) const;

        /**
         * @brief Data style index in a concrete enum type
         *
         * Expects that @p handle is valid. The index is guaranteed to be less
         * than @ref Shared::totalStyleCount().
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
         * @ref Shared::totalStyleCount().
         *
         * Calling this function causes @ref LayerState::NeedsDataUpdate to be
         * set.
         * @see @ref isHandleValid(DataHandle) const,
         *      @ref setTransitionedStyle()
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

        /**
         * @brief Set data style index, potentially transitioning it based on user interface state
         *
         * Like @ref setStyle(), but if @p handle is assigned to a node that's
         * referenced from @ref AbstractUserInterface::currentPressedNode(),
         * @relativeref{AbstractUserInterface,currentHoveredNode()} or
         * @relativeref{AbstractUserInterface,currentFocusedNode()}, applies
         * style transition functions set in @ref Shared::setStyleTransition()
         * to it first. Expects that @p handle is valid and @p style is less
         * than @ref Shared::styleCount(). Not @ref Shared::totalStyleCount()
         * --- the style transition functions are not allowed to use the
         * dynamic style indices.
         *
         * Calling this function causes @ref LayerState::NeedsDataUpdate to be
         * set.
         * @see @ref isHandleValid(DataHandle) const
         */
        void setTransitionedStyle(const AbstractUserInterface& ui, DataHandle handle, UnsignedInt style);

        /**
         * @brief Set data style index in a concrete enum type, potentially transitioning it based on user interface state
         *
         * Casts @p style to @relativeref{Magnum,UnsignedInt} and delegates to
         * @ref setTransitionedStyle(const AbstractUserInterface&, DataHandle, UnsignedInt).
         */
        template<class StyleIndex
            #ifndef DOXYGEN_GENERATING_OUTPUT
            , class = typename std::enable_if<std::is_enum<StyleIndex>::value>::type
            #endif
        > void setTransitionedStyle(const AbstractUserInterface& ui, DataHandle handle, StyleIndex style) {
            setTransitionedStyle(ui, handle, UnsignedInt(style));
        }

        /**
         * @brief Set data style index assuming it belongs to this layer, potentially transitioning it based on user interface state
         *
         * Like @ref setTransitionedStyle(const AbstractUserInterface&, DataHandle, UnsignedInt) but without
         * checking that @p handle indeed belongs to this layer. See its
         * documentation for more information.
         */
        void setTransitionedStyle(const AbstractUserInterface& ui, LayerDataHandle handle, UnsignedInt style);

        /**
         * @brief Set data style index in a concrete enum type assuming it belongs to this layer, potentially transitioning it based on user interface state
         *
         * Casts @p style to @relativeref{Magnum,UnsignedInt} and delegates to
         * @ref setTransitionedStyle(const AbstractUserInterface&, LayerDataHandle, UnsignedInt).
         */
        template<class StyleIndex
            #ifndef DOXYGEN_GENERATING_OUTPUT
            , class = typename std::enable_if<std::is_enum<StyleIndex>::value>::type
            #endif
        > void setTransitionedStyle(const AbstractUserInterface& ui, LayerDataHandle handle, StyleIndex style) {
            setTransitionedStyle(ui, handle, UnsignedInt(style));
        }

        /**
         * @brief Count of used dynamic styles
         *
         * Always at most @ref Shared::dynamicStyleCount(). If equal to
         * @ref Shared::dynamicStyleCount(), a call to
         * @ref allocateDynamicStyle() will return
         * @relativeref{Corrade,Containers::NullOpt}.
         */
        UnsignedInt dynamicStyleUsedCount() const;

        /**
         * @brief Allocate a dynamic style index
         *
         * The returned index can be used to set properties of a dynamic style
         * using @ref BaseLayer::setDynamicStyle() /
         * @ref TextLayer::setDynamicStyle(). When added to
         * @ref Shared::styleCount(), it can be passed as a style index to
         * @ref setStyle() or @ref BaseLayer::create() /
         * @ref TextLayer::create() / @ref TextLayer::createGlyph().
         *
         * When not used anymore, the index should be passed to
         * @ref recycleDynamicStyle() to make it available for allocation
         * again. If there are no free dynamic styles left, returns
         * @relativeref{Corrade,Containers::NullOpt}.
         *
         * If the dynamic style is driven by an animation, its handle can be
         * passed to the @p animation argument to retrieve later with
         * @ref dynamicStyleAnimation(). No validation is performed on the
         * handle, it can be arbitrary. However, if the @p animation belongs to
         * an animator that's set with @ref BaseLayer::setDefaultStyleAnimator()
         * / @ref TextLayer::setDefaultStyleAnimator(), style transitions in
         * response to events will treat data using given dynamic style as if
         * they had the style set to
         * @ref AbstractVisualLayerStyleAnimator::targetStyle(), instead of
         * leaving them untouched. Which means that for example, if there's an
         * animation that has a hovered style as the target, and a press
         * happens, it'll trigger a transition the hovered style to a pressed
         * one, instead of leaving the dynamic style untouched.
         * @see @ref dynamicStyleUsedCount(), @ref Shared::dynamicStyleCount()
         */
        Containers::Optional<UnsignedInt> allocateDynamicStyle(AnimationHandle animation =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            AnimationHandle::Null
            #else
            AnimationHandle{} /* To not have to include Handle.h */
            #endif
        );

        /**
         * @brief Animation associated with a dynamic style
         *
         * Expects that @p id is less than @ref Shared::dynamicStyleCount(). If
         * @ref allocateDynamicStyle() was called with a null handle, wasn't
         * called with @p id yet or @ref recycleDynamicStyle() was called for
         * @p id since, returns @ref AnimationHandle::Null. The returned handle
         * isn't guaranteed to be valid if non-null.
         */
        AnimationHandle dynamicStyleAnimation(UnsignedInt id) const;

        /**
         * @brief Recycle a dynamic style index
         *
         * Expects that @p id is less than @ref Shared::dynamicStyleCount(),
         * that it was returned from @ref allocateDynamicStyle() earlier and
         * that @ref recycleDynamicStyle() hasn't been called on the allocated
         * @p id yet. Animation handle associated with @p id is reset to
         * @ref AnimationHandle::Null after calling this function.
         * @see @ref dynamicStyleAnimation()
         */
        void recycleDynamicStyle(UnsignedInt id);

    #ifdef DOXYGEN_GENERATING_OUTPUT
    private:
    #else
    protected:
    #endif
        struct State;

        MAGNUM_UI_LOCAL explicit AbstractVisualLayer(LayerHandle handle, Containers::Pointer<State>&& state);
        /* Used by tests to avoid having to include / allocate the state */
        explicit AbstractVisualLayer(LayerHandle handle, Shared& shared);

        /* Can't be MAGNUM_UI_LOCAL otherwise this can't be called from
           tests */
        AbstractVisualLayer& assignAnimator(AbstractVisualLayerStyleAnimator& animator);
        AbstractVisualLayerStyleAnimator* defaultStyleAnimator() const;
        AbstractVisualLayer& setDefaultStyleAnimator(AbstractVisualLayerStyleAnimator* animator);

        /* Can't be MAGNUM_UI_LOCAL otherwise deriving from this class in
           tests causes linker errors */
        LayerFeatures doFeatures() const override;
        LayerStates doState() const override;

        /* Updates State::Shared::calculatedStyles based on which nodes are
           enabled. Should be called by subclasses. */
        void doUpdate(LayerStates states, const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, const Containers::StridedArrayView1D<const Float>& nodeOpacities, Containers::BitArrayView nodesEnabled, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes, const Containers::StridedArrayView1D<const Vector2>& compositeRectOffsets, const Containers::StridedArrayView1D<const Vector2>& compositeRectSizes) override;

        Containers::Pointer<State> _state;

    private:
        MAGNUM_UI_LOCAL void setStyleInternal(UnsignedInt id, UnsignedInt style);
        MAGNUM_UI_LOCAL void setTransitionedStyleInternal(const AbstractUserInterface& ui, LayerDataHandle handle, UnsignedInt style);
        MAGNUM_UI_LOCAL UnsignedInt styleOrAnimationTargetStyle(UnsignedInt style) const;

        /* Can't be MAGNUM_UI_LOCAL otherwise deriving from this class in
           tests causes linker errors */
        void doPointerPressEvent(UnsignedInt dataId, PointerEvent& event) override;
        void doPointerReleaseEvent(UnsignedInt dataId, PointerEvent& event) override;
        void doPointerMoveEvent(UnsignedInt dataId, PointerMoveEvent& event) override;
        void doPointerEnterEvent(UnsignedInt dataId, PointerMoveEvent& event) override;
        void doPointerLeaveEvent(UnsignedInt dataId, PointerMoveEvent& event) override;
        void doFocusEvent(UnsignedInt dataId, FocusEvent& event) override;
        void doBlurEvent(UnsignedInt dataId, FocusEvent& event) override;
        void doVisibilityLostEvent(UnsignedInt dataId, VisibilityLostEvent& event) override;
};

/**
@brief Base shared state for visual data layers

Stores style transition functions.
*/
class MAGNUM_UI_EXPORT AbstractVisualLayer::Shared {
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
         * Count of styles used by all layer instances referencing this
         * @ref Shared instance. IDs greater than @ref styleCount() are then
         * dynamic styles, with their count specified by
         * @ref dynamicStyleCount().
         * @see @ref totalStyleCount(),
         *      @ref BaseLayer::Shared::Configuration::Configuration(UnsignedInt, UnsignedInt),
         *      @ref TextLayer::Shared::Configuration::Configuration(UnsignedInt, UnsignedInt),
         *      @ref BaseLayer::Shared::setStyle(),
         *      @ref TextLayer::Shared::setStyle()
         */
        UnsignedInt styleCount() const;

        /**
         * @brief Dynamic style count
         *
         * Count of dynamic styles appearing after the initial @ref styleCount()
         * styles, i.e. having IDs greater or equal to @ref styleCount() and
         * less than @ref totalStyleCount(). The dynamic styles are local to
         * every layer instance and are meant to be used mainly for style
         * transition animations.
         * @see @ref BaseLayer::Shared::Configuration::setDynamicStyleCount(),
         *      @ref TextLayer::Shared::Configuration::setDynamicStyleCount(),
         *      @ref BaseLayer::setDynamicStyle(),
         *      @ref TextLayer::setDynamicStyle()
         */
        UnsignedInt dynamicStyleCount() const;

        /**
         * @brief Total style count
         *
         * A sum of @ref styleCount() and @ref dynamicStyleCount(). Styles with
         * IDs less than @ref styleCount() are the shared ones, styles with
         * IDs greater or equal to @ref styleCount() and less than
         * @ref totalStyleCount() are the dynamic ones.
         */
        UnsignedInt totalStyleCount() const;

        /**
         * @brief Set type-erased style transition functions
         * @return Reference to self (for method chaining)
         *
         * The @p toInactiveOut and @p toInactiveOver change a non-disabled
         * style index to an inactive one with the pointer outside or over the
         * node, for example when a mouse enters or leaves an area of otherwise
         * inactive (neither focused nor pressed) button, but also when a
         * button is released again or an input is no longer focused.
         *
         * The @p toFocusedOut and @p toFocusedOver change a non-disabled style
         * index to a focused one with the pointer outside or over the node.
         * Note that, to reduce the amount of combinations, a pressed state has
         * a priority over focused, so these two transitions are picked only
         * when hovering a focused node or when the pointer is released after a
         * node was focused by a pointer press. These transitions only ever
         * happen for data attached to @ref NodeFlag::Focusable nodes.
         *
         * The @p toPressedOut and @p toPressedOver change a non-disabled style
         * index to a pressed one with the pointer outside or over the node,
         * for example after a pointer was pressed on a hovered button, after
         * an activated but non-hovered button was pressed via a keyboard, but
         * also after a pointer leaves a pressed button, making it no longer
         * hovered or re-enters it, making it hovered again.
         *
         * The @p toDisabled changes a style index to a disabled one, which
         * happens when a @ref NodeFlag::Disabled is set on a node. Such a node
         * then doesn't receive any events until enabled again, meaning the
         * disabled style index cannot transition into any other.
         *
         * If any of the functions is @cpp nullptr @ce, given transition is
         * a no-op, keeping the same index. All transition functions are
         * @cpp nullptr @ce initially.
         *
         * For correct behavior, the @p toInactiveOut, @p toInactiveOver,
         * @p toFocusedOut, @p toFocusedOver, @p toPressedOut and
         * @p toPressedOver functions should be mutually invertible, e.g.
         * @cpp toPressedOver(toInactiveOut(style)) == style @ce if the `style`
         * was a pressed over style to begin with (and both transition
         * functions were defined). The @p toDisabled function doesn't have to
         * be, i.e. it can conflate multiple styles into one, as a disabled
         * style is internally never transitioned back to a non-disabled one.
         * If the style doesn't handle hover in any way, for example for
         * touch-only interfaces, you can use
         * @ref setStyleTransition() "setStyleTransition(UnsignedInt(*)(UnsignedInt), UnsignedInt(*)(UnsignedInt), UnsignedInt(*)(UnsignedInt), UnsignedInt(*)(UnsignedInt))"
         * instead, which doesn't make any distinction between the over and
         * out states and uses the same transition function for both.
         *
         * All functions are passed an index that's less than @ref styleCount()
         * and are expected to return an index that's less than
         * @ref styleCount() as well. Not @ref totalStyleCount() --- the style
         * transition functions are not allowed to use the dynamic style
         * indices. Data with a dynamic style index are not transitioned in any
         * way.
         *
         * Setting (and subsequently changing) the @p toDisabled function
         * causes @ref LayerState::NeedsDataUpdate to be set on all layers that
         * are constructed using this shared instance. The other transition
         * functions don't cause any @ref LayerState to be set, as they're only
         * used directly in event handlers.
         */
        Shared& setStyleTransition(UnsignedInt(*toInactiveOut)(UnsignedInt), UnsignedInt(*toInactiveOver)(UnsignedInt), UnsignedInt(*toFocusedOut)(UnsignedInt), UnsignedInt(*toFocusedOver)(UnsignedInt), UnsignedInt(*toPressedOut)(UnsignedInt), UnsignedInt(*toPressedOver)(UnsignedInt), UnsignedInt(*toDisabled)(UnsignedInt));

        /**
         * @brief Set style transition functions
         * @return Reference to self (for method chaining)
         *
         * Like @ref setStyleTransition() "setStyleTransition(UnsignedInt(*)(UnsignedInt), UnsignedInt(*)(UnsignedInt), UnsignedInt(*)(UnsignedInt), UnsignedInt(*)(UnsignedInt), UnsignedInt(*)(UnsignedInt), UnsignedInt(*)(UnsignedInt), UnsignedInt(*)(UnsignedInt))",
         * but allows to use a concrete enum type instead of a typeless index.
         * Same as with the type-erased variant, if any of the function
         * template parameters is @cpp nullptr @ce, given transition is a
         * no-op, keeping the same index. Example usage:
         *
         * @snippet Ui.cpp AbstractVisualLayer-Shared-setStyleTransition
         */
        template<class StyleIndex, StyleIndex(*toInactiveOut)(StyleIndex), StyleIndex(*toInactiveOver)(StyleIndex), StyleIndex(*toFocusedOut)(StyleIndex), StyleIndex(*toFocusedOver)(StyleIndex), StyleIndex(*toPressedOut)(StyleIndex), StyleIndex(*toPressedOver)(StyleIndex), StyleIndex(*toDisabled)(StyleIndex)> Shared& setStyleTransition();

        /**
         * @brief Set style transition functions without hover state
         * @return Reference to self (for method chaining)
         *
         * Same as calling @ref setStyleTransition() "setStyleTransition(UnsignedInt(*)(UnsignedInt), UnsignedInt(*)(UnsignedInt), UnsignedInt(*)(UnsignedInt), UnsignedInt(*)(UnsignedInt), UnsignedInt(*)(UnsignedInt), UnsignedInt(*)(UnsignedInt), UnsignedInt(*)(UnsignedInt))"
         * with @p toInactive used for both @p toInactiveOut and
         * @p toInactiveOver, @p toFocused used for both @p toFocusedOut and
         * @p toFocusedOver and @p toPressed used for both @p toPressedOut and
         * @p toPressedOver. Useful in case the style doesn't handle hover in
         * any way, for example for touch-only interfaces.
         */
        Shared& setStyleTransition(UnsignedInt(*toInactive)(UnsignedInt), UnsignedInt(*toFocused)(UnsignedInt), UnsignedInt(*toPressed)(UnsignedInt), UnsignedInt(*toDisabled)(UnsignedInt)) {
            return setStyleTransition(toInactive, toInactive, toFocused, toFocused, toPressed, toPressed, toDisabled);
        }

        /**
         * @brief Set style transition functions
         * @return Reference to self (for method chaining)
         *
         * Same as calling @ref setStyleTransition() with @p toInactive used
         * for both @p toInactiveOut and @p toInactiveOver, @p toFocused used
         * for both @p toFocusedOut and @p toFocusedOver and @p toPressed used
         * for both @p toPressedOut and @p toPressedOver. Useful in case the
         * style doesn't handle hover in any way, for example for touch-only
         * interfaces.
         */
        template<class StyleIndex, StyleIndex(*toInactive)(StyleIndex), StyleIndex(*toFocused)(StyleIndex), StyleIndex(*toPressed)(StyleIndex), StyleIndex(*toDisabled)(StyleIndex)> Shared& setStyleTransition() {
            return setStyleTransition<StyleIndex, toInactive, toInactive, toFocused, toFocused, toPressed, toPressed, toDisabled>();
        }

    #ifdef DOXYGEN_GENERATING_OUTPUT
    private:
    #else
    protected:
    #endif
        struct State;
        friend AbstractVisualLayer;
        friend AbstractVisualLayerStyleAnimator;

        MAGNUM_UI_LOCAL explicit Shared(Containers::Pointer<State>&& state);
        /* Used by tests to avoid having to include / allocate the state */
        explicit Shared(UnsignedInt styleCount, UnsignedInt dynamicStyleCount);
        /* Can't be MAGNUM_UI_LOCAL, used by tests */
        explicit Shared(NoCreateT) noexcept;

        Containers::Pointer<State> _state;
};

#ifndef DOXYGEN_GENERATING_OUTPUT
#define MAGNUMEXTRAS_UI_ABSTRACTVISUALLAYER_SHARED_SUBCLASS_IMPLEMENTATION() \
    Shared& setStyleTransition(UnsignedInt(*toInactiveOut)(UnsignedInt), UnsignedInt(*toInactiveOver)(UnsignedInt), UnsignedInt(*toFocusedOut)(UnsignedInt), UnsignedInt(*toFocusedOver)(UnsignedInt), UnsignedInt(*toPressedOut)(UnsignedInt), UnsignedInt(*toPressedOver)(UnsignedInt), UnsignedInt(*toDisabled)(UnsignedInt)) { \
        return static_cast<Shared&>(AbstractVisualLayer::Shared::setStyleTransition(toInactiveOut, toInactiveOver, toFocusedOut, toFocusedOver, toPressedOut, toPressedOver, toDisabled)); \
    }                                                                       \
    template<class StyleIndex, StyleIndex(*toInactiveOut)(StyleIndex), StyleIndex(*toInactiveOver)(StyleIndex), StyleIndex(*toFocusedOut)(StyleIndex), StyleIndex(*toFocusedOver)(StyleIndex), StyleIndex(*toPressedOut)(StyleIndex), StyleIndex(*toPressedOver)(StyleIndex), StyleIndex(*toDisabled)(StyleIndex)> Shared& setStyleTransition() { \
        return static_cast<Shared&>(AbstractVisualLayer::Shared::setStyleTransition<StyleIndex, toInactiveOut, toInactiveOver, toFocusedOut, toFocusedOver, toPressedOut, toPressedOver, toDisabled>()); \
    }                                                                       \
    Shared& setStyleTransition(UnsignedInt(*toInactive)(UnsignedInt), UnsignedInt(*toFocused)(UnsignedInt), UnsignedInt(*toPressed)(UnsignedInt), UnsignedInt(*toDisabled)(UnsignedInt)) { \
        return static_cast<Shared&>(AbstractVisualLayer::Shared::setStyleTransition(toInactive, toFocused, toPressed, toDisabled)); \
    }                                                                       \
    template<class StyleIndex, StyleIndex(*toInactive)(StyleIndex), StyleIndex(*toFocused)(StyleIndex), StyleIndex(*toPressed)(StyleIndex), StyleIndex(*toDisabled)(StyleIndex)> Shared& setStyleTransition() { \
        return static_cast<Shared&>(AbstractVisualLayer::Shared::setStyleTransition<StyleIndex, toInactive, toFocused, toPressed, toDisabled>()); \
    }
#endif

/* The damn thing fails to match these to the declarations above */
#ifndef DOXYGEN_GENERATING_OUTPUT
template<class StyleIndex, StyleIndex(*toInactiveOut)(StyleIndex), StyleIndex(*toInactiveOver)(StyleIndex), StyleIndex(*toFocusedOut)(StyleIndex), StyleIndex(*toFocusedOver)(StyleIndex), StyleIndex(*toPressedOut)(StyleIndex), StyleIndex(*toPressedOver)(StyleIndex), StyleIndex(*toDisabled)(StyleIndex)> AbstractVisualLayer::Shared& AbstractVisualLayer::Shared::setStyleTransition() {
    /* No matter what simplification I do, GCC warns about "implicit conversion
       to bool", so it's this obvious ugly == here. There could be + for the
       lambdas to turn them into function pointers to avoid ?: getting
       confused, but that doesn't work on MSVC 2015 so instead it's the nullptr
       being cast. */
    /** @todo revert back to the + once CORRADE_MSVC2015_COMPATIBILITY is
        dropped */
    #if defined(CORRADE_TARGET_GCC) && __GNUC__ < 13
    #pragma GCC diagnostic push
    /* GCC since at least version 8 warns that function pointers passed here,
       if not null, will never be null. Well, yes. Fixed in GCC 13.
        https://gcc.gnu.org/bugzilla/show_bug.cgi?id=94554 */
    #pragma GCC diagnostic ignored "-Waddress"
    #endif
    return setStyleTransition(
        toInactiveOut == nullptr ? static_cast<UnsignedInt(*)(UnsignedInt)>(nullptr) : [](UnsignedInt index) {
            return UnsignedInt(toInactiveOut(StyleIndex(index)));
        },
        toInactiveOver == nullptr ? static_cast<UnsignedInt(*)(UnsignedInt)>(nullptr) : [](UnsignedInt index) {
            return UnsignedInt(toInactiveOver(StyleIndex(index)));
        },
        toFocusedOut == nullptr ? static_cast<UnsignedInt(*)(UnsignedInt)>(nullptr) : [](UnsignedInt index) {
            return UnsignedInt(toFocusedOut(StyleIndex(index)));
        },
        toFocusedOver == nullptr ? static_cast<UnsignedInt(*)(UnsignedInt)>(nullptr) : [](UnsignedInt index) {
            return UnsignedInt(toFocusedOver(StyleIndex(index)));
        },
        toPressedOut == nullptr ? static_cast<UnsignedInt(*)(UnsignedInt)>(nullptr) : [](UnsignedInt index) {
            return UnsignedInt(toPressedOut(StyleIndex(index)));
        },
        toPressedOver == nullptr ? static_cast<UnsignedInt(*)(UnsignedInt)>(nullptr) : [](UnsignedInt index) {
            return UnsignedInt(toPressedOver(StyleIndex(index)));
        },
        toDisabled == nullptr ? static_cast<UnsignedInt(*)(UnsignedInt)>(nullptr) : [](UnsignedInt index) {
            return UnsignedInt(toDisabled(StyleIndex(index)));
        });
    #if defined(CORRADE_TARGET_GCC) && __GNUC__ < 13
    #pragma GCC diagnostic pop
    #endif
}
#endif

}}

#endif
