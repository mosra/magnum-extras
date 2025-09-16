#ifndef Magnum_Ui_AbstractVisualLayerAnimator_h
#define Magnum_Ui_AbstractVisualLayerAnimator_h
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
 * @brief Class @ref Magnum::Ui::AbstractVisualLayerStyleAnimator
 * @m_since_latest
 */

#include <Corrade/Containers/Pair.h> /* for styles<T>() */

#include "Magnum/Ui/AbstractAnimator.h"

namespace Magnum { namespace Ui {

/**
@brief Base for @ref AbstractVisualLayer style animators
@m_since_latest

Provides common code for visual layer style animators like
@ref BaseLayerStyleAnimator or @ref TextLayerStyleAnimator.
*/
class MAGNUM_UI_EXPORT AbstractVisualLayerStyleAnimator: public AbstractStyleAnimator {
    public:
        /** @brief Copying is not allowed */
        AbstractVisualLayerStyleAnimator(const AbstractVisualLayerStyleAnimator&) = delete;

        /** @copydoc AbstractStyleAnimator::AbstractStyleAnimator(AbstractStyleAnimator&&) */
        AbstractVisualLayerStyleAnimator(AbstractVisualLayerStyleAnimator&&) noexcept;

        ~AbstractVisualLayerStyleAnimator();

        /** @brief Copying is not allowed */
        AbstractVisualLayerStyleAnimator& operator=(const AbstractVisualLayerStyleAnimator&) = delete;

        /** @brief Move assignment */
        AbstractVisualLayerStyleAnimator& operator=(AbstractVisualLayerStyleAnimator&&) noexcept;

        /**
         * @brief Animation source and target style IDs
         *
         * Expects that @p handle is valid. The returned values are always less
         * than @ref AbstractVisualLayer::Shared::styleCount() of the layer
         * associated with this animator.
         * @see @ref isHandleValid(AnimationHandle) const
         */
        Containers::Pair<UnsignedInt, UnsignedInt> styles(AnimationHandle handle) const;

        /**
         * @brief Animation source and target style IDs in a concrete enum type
         *
         * Like @ref styles(AnimationHandle) const but returning a concrete
         * enum type. See its documentation for more information.
         */
        template<class StyleIndex> Containers::Pair<StyleIndex, StyleIndex> styles(AnimationHandle handle) const {
            static_assert(std::is_enum<StyleIndex>::value, "expected an enum type");
            Containers::Pair<UnsignedInt, UnsignedInt> out = styles(handle);
            return {StyleIndex(out.first()), StyleIndex(out.second())};
        }

        /**
         * @brief Animation source and target styles assuming it belongs to this animator
         *
         * Like @ref styles(AnimationHandle) const but without checking that
         * @p handle indeed belongs to this animator. See its documentation for
         * more information.
         * @see @ref isHandleValid(AnimatorDataHandle) const,
         *      @ref animationHandleData()
         */
        Containers::Pair<UnsignedInt, UnsignedInt> styles(AnimatorDataHandle handle) const;

        /**
         * @brief Animation source and target style IDs in a concrete enum type assuming it belongs to this animator
         *
         * Like @ref styles(AnimatorDataHandle) const but returning a concrete
         * enum type. See its documentation for more information.
         */
        template<class StyleIndex> Containers::Pair<StyleIndex, StyleIndex> styles(AnimatorDataHandle handle) const {
            static_assert(std::is_enum<StyleIndex>::value, "expected an enum type");
            Containers::Pair<UnsignedInt, UnsignedInt> out = styles(handle);
            return {StyleIndex(out.first()), StyleIndex(out.second())};
        }

        /**
         * @brief Animation dynamic style ID
         *
         * Expects that @p handle is valid. If a dynamic style is allocated,
         * the returned value is always less than
         * @ref AbstractVisualLayer::Shared::dynamicStyleCount() of the
         * associated layer. @ref AbstractVisualLayer::Shared::styleCount()
         * plus the returned value is then a style index that can be passed to
         * e.g. @ref AbstractVisualLayer::setStyle(). If the dynamic style
         * wasn't allocated yet, either due to the animation not being advanced
         * yet or due to no free dynamic styles being available, returns
         * @relativeref{Corrade,Containers::NullOpt}.
         * @see @ref styles()
         */
        Containers::Optional<UnsignedInt> dynamicStyle(AnimationHandle handle) const;

        /**
         * @brief Animation dynamic style IDs assuming it belongs to this animator
         *
         * Like @ref dynamicStyle(AnimationHandle) const but without checking
         * that @p handle indeed belongs to this animator. See its
         * documentation for more information.
         * @see @ref animationHandleData()
         */
        Containers::Optional<UnsignedInt> dynamicStyle(AnimatorDataHandle handle) const;

    #ifdef DOXYGEN_GENERATING_OUTPUT
    private:
    #else
    protected:
    #endif
        struct State;
        /* Only AbstractVisualLayer::assignAnimator(), but we don't want to
           include the whole thing */
        friend AbstractVisualLayer;

        MAGNUM_UI_LOCAL explicit AbstractVisualLayerStyleAnimator(AnimatorHandle handle, Containers::Pointer<State>&& state);
        /* Used by tests to avoid having to include / allocate the state */
        explicit AbstractVisualLayerStyleAnimator(AnimatorHandle handle);

        MAGNUM_UI_LOCAL void removeInternal(UnsignedInt id);
        /* Called from subclass advance() implementations, manages style
           switching and dynamic style allocation. First returned bool is
           whether style assignments were updated by this function, second is
           whether uniform data are meant to be updated by the subclass. */
        MAGNUM_UI_LOCAL Containers::Pair<bool, bool> advance(Containers::BitArrayView active, Containers::BitArrayView stopped, const Containers::StridedArrayView1D<UnsignedInt>& dataStyles);

        Containers::Pointer<State> _state;

    private:
        /* Called by AbstractVisualLayer::assignAnimator() to set the actual
           instance for dynamic style allocation and recycling, and for
           accessing the input style data (which is a typeless pointer to not
           need to include the whole AbstractVisualLayer header). Yeah, it's
           quite ew. */
        MAGNUM_UI_LOCAL void setLayerInstance(AbstractVisualLayer& instance, const void* sharedState);

        /* Can't be MAGNUM_UI_LOCAL otherwise deriving from this class in
           tests causes linker errors */
        void doClean(Containers::BitArrayView animationIdsToRemove) override;
};

}}

#endif
