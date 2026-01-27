#ifndef Magnum_Ui_GenericLayouter_h
#define Magnum_Ui_GenericLayouter_h
/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023, 2024, 2025, 2026
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
 * @brief Class @ref Magnum::Ui::GenericLayouter
 * @m_since_latest_{extras}
 */

#include "Magnum/Ui/AbstractLayouter.h"

namespace Magnum { namespace Ui {

/**
@brief Generic layouter
@m_since_latest_{extras}

Each layout is a function that gets called on a particular node, allowing to
arbitrarily modify the node offset and size as well as any associated data.

@section Ui-GenericLayouter-setup Setting up a generic layouter instance

If you create a @ref UserInterfaceGL with a style and don't exclude
@ref StyleFeature::GenericLayouter, an implicit instance is already provided
and available through @ref UserInterface::genericLayouter(). Otherwise, the
layouter doesn't have any shared state or configuration, so it's just about
constructing it from a fresh @ref AbstractUserInterface::createLayouter()
handle and passing it to @ref UserInterface::setGenericLayouterInstance():

@snippet Ui.cpp GenericLayouter-setup-implicit

In comparison, if you want to set up a custom generic layouter that's
independent of the one exposed through @ref UserInterface::genericLayouter(),
pass the newly created instance to
@ref AbstractUserInterface::setLayouterInstance() instead:

@snippet Ui.cpp GenericLayouter-setup

Afterwards, with either of the above, assuming
@ref AbstractUserInterface::draw() is called in an appropriate place, the
layouter is ready to use.

@section Ui-GenericLayouter-add Adding layouts

A layout is created by calling @ref add() with a @ref NodeHandle to which the
layout is assigned, and a function taking a reference to the layouter instance
along with mutable references to the associated node offset and size. The node
offset and size can be arbitrarily modified, the layouter can be used to query
current layout properties of various nodes using @ref nodeOffset(),
@ref nodeSize(), @ref nodeMinSize(), @ref nodeMaxSize(), @ref nodeAspectRatio(),
@ref nodePadding() and @ref nodeMargin(). For example, the following would
scale the filled part of a progress bar to be given percentage of its actual
width:

@m_class{m-console-wrap}

@snippet Ui.cpp GenericLayouter-add

Instead of the percentage being accessed through captured state, you can encode
it to the initial node size, which the layouter then just updates. This way you
can also animate percentage changes using @ref NodeAnimator, for example.

@m_class{m-console-wrap}

@snippet Ui.cpp GenericLayouter-add-percentage-in-node-size

The layout isn't *required* to update the node offset and size at all --- in
the following snippet, the progress bar is made out of a single node with a
@ref BaseLayer data, and the progress is visualized by outline on the right
edge:

@m_class{m-console-wrap}

@snippet Ui.cpp GenericLayouter-add-no-node-modification

@section Ui-GenericLayouter-execution-order Layout execution order

Unlike e.g. @ref SnapLayouter, which performs the layout according to node
hierarchy, the @ref GenericLayouter gives no guarantee about the order in which
the functions are executed, not even in respect to the sequence in which they
were added.

If you require certain ordering, create additional layouter instances and put
layout functions that need to be executed after the previous ones into these.
*/
class MAGNUM_UI_EXPORT GenericLayouter: public AbstractLayouter {
    public:
        /**
         * @brief Constructor
         * @param handle    Layouter handle returned from
         *      @ref AbstractUserInterface::createLayouter()
         */
        explicit GenericLayouter(LayouterHandle handle);

        /** @brief Copying is not allowed */
        GenericLayouter(const GenericLayouter&) = delete;

        /** @copydoc AbstractLayouter::AbstractLayouter(AbstractLayouter&&) */
        GenericLayouter(GenericLayouter&&) noexcept;

        ~GenericLayouter();

        /** @brief Copying is not allowed */
        GenericLayouter& operator=(const GenericLayouter&) = delete;

        /** @brief Move assignment */
        GenericLayouter& operator=(GenericLayouter&&) noexcept;

        /**
         * @brief Count of allocated layout functions
         *
         * Always at most @ref usedCount(). Counts all layout functions that
         * capture non-trivially-copyable state or state that's too large to be
         * stored in-place. The operation is done with a @f$ \mathcal{O}(n) @f$
         * complexity where @f$ n @f$ is @ref capacity().
         * @todoc fix the isAllocated link once Doxygen stops being shit -- it
         *      works only from Containers themselves
         * @see @ref Corrade::Containers::Function "Containers::Function<R(Args...)>::isAllocated()"
         */
        std::size_t usedAllocatedCount() const;

        /**
         * @brief Add a layout assigned to given node
         * @param node      Node to assign the layout to
         * @param layout    Function to perform the layout operation
         * @return New layout handle
         *
         * Expects that the @p layout is not @cpp nullptr @ce. The function
         * gets called as part of layout update if @p node is visible. When
         * executed, the function can query @ref nodeOffset(), @ref nodeSize(),
         * @ref nodeMinSize(), @ref nodeMaxSize(), @ref nodeAspectRatio(),
         * @ref nodePadding() and @ref nodeMargin() functions on the passed
         * @p layouter instance and use that information to update the
         * @p nodeOffset and @p nodeSize arguments. Leaving either or both
         * untouched is allowed as well, for example if the layout affects
         * something else than node positioning.
         *
         * Delegates to @ref AbstractLayouter::add(), see its documentation for
         * detailed description of all constraints.
         */
        LayoutHandle add(NodeHandle node, Containers::Function<void(const GenericLayouter& layouter, Vector2& nodeOffset, Vector2& nodeSize)>&& layout);

        /**
         * @brief Remove a layout
         *
         * Expects that @p handle is valid.
         *
         * Delegates to @ref AbstractLayouter::remove(LayoutHandle), see its
         * documentation for detailed description of all constraints.
         * @see @ref isHandleValid(LayoutHandle) const
         */
        void remove(LayoutHandle handle);

        /**
         * @brief Remove a layout assuming it belongs to this layer
         *
         * Compared to @ref remove(LayoutHandle) delegates to
         * @ref AbstractLayouter::remove(LayouterDataHandle) instead.
         * @see @ref isHandleValid(LayouterDataHandle) const,
         *      @ref layoutHandleData()
         */
        void remove(LayouterDataHandle handle);

        /**
         * @brief Whether given layout function is allocated
         *
         * Returns @cpp true @ce if given layout function captures
         * non-trivially-copyable state or state that's too large to be stored
         * in-place, @cpp false @ce otherwise. Expects that @p handle is valid.
         * @see @ref isHandleValid(LayoutHandle) const,
         *      @ref Corrade::Containers::Function "Containers::Function<R(Args...)>::isAllocated()",
         *      @ref usedAllocatedCount()
         */
        bool isAllocated(LayoutHandle handle) const;

        /**
         * @brief Whether given layout function is allocated assuming it belongs to this layouter
         *
         * Like @ref isAllocated(LayoutHandle) const but without checking
         * that @p handle indeed belongs to this animator. See its
         * documentation for more information.
         * @see @ref isHandleValid(LayouterDataHandle) const,
         *      @ref layoutHandleData()
         */
        bool isAllocated(LayouterDataHandle handle) const;

        /**
         * @brief Node offset in current layout, relative to its parent
         *
         * Compared to @ref AbstractUserInterface::nodeOffset() contains the
         * value calculated by all previous layouters until this one. Expects
         * that @p node is valid, can only be called from within the layout
         * functions passed to @ref add().
         */
        Vector2 nodeOffset(NodeHandle node) const;

        /**
         * @brief Node size in current layout
         *
         * Compared to @ref AbstractUserInterface::nodeSize() contains the
         * value calculated by all previous layouters until this one. Expects
         * that @p node is valid, can only be called from within the layout
         * functions passed to @ref add().
         */
        Vector2 nodeSize(NodeHandle node) const;

        /**
         * @brief Minimal node size in current layout
         *
         * Contains the value calculated by all layers advertising
         * @ref LayerFeature::Layout and all previous layouters until this one.
         * If nothing specified any min size, the default is all components
         * being @cpp 0.0f @ce. Expects that @p node is valid, can only be
         * called from within the layout functions passed to @ref add().
         */
        Vector2 nodeMinSize(NodeHandle node) const;

        /**
         * @brief Maximal node size in current layout
         *
         * Contains the value calculated by all layers advertising
         * @ref LayerFeature::Layout and all previous layouters until this one.
         * If nothing specified any max size, the default is all components
         * being @ref Constants::inf(). Expects that @p node is valid, can only
         * be called from within the layout functions passed to @ref add().
         */
        Vector2 nodeMaxSize(NodeHandle node) const;

        /**
         * @brief Node aspect ratio in current layout
         *
         * Contains the value calculated by all layers advertising
         * @ref LayerFeature::Layout and all previous layouters until this one.
         * If nothing specified any aspect ratio, the default is @cpp 0.0f @ce.
         * Expects that @p node is valid, can only be called from within the
         * layout functions passed to @ref add().
         */
        Float nodeAspectRatio(NodeHandle node) const;

        /**
         * @brief Padding inside a node in current layout
         *
         * Contains the value calculated by all layers advertising
         * @ref LayerFeature::Layout and all previous layouters until this one.
         * If nothing specified any padding, the default is all components
         * being @cpp 0.0f @ce. Expects that @p node is valid, can only be
         * called from within the layout functions passed to @ref add().
         */
        Vector4 nodePadding(NodeHandle node) const;

        /**
         * @brief Margin inside a node in current layout
         *
         * Contains the value calculated by all layers advertising
         * @ref LayerFeature::Layout and all previous layouters until this one.
         * If nothing specified any margin, the default is all components being
         * @cpp 0.0f @ce. Expects that @p node is valid, can only be
         * called from within the layout functions passed to @ref add().
         */
        Vector4 nodeMargin(NodeHandle node) const;

    private:
        MAGNUM_UI_LOCAL void removeInternal(UnsignedInt id);

        MAGNUM_UI_LOCAL LayouterFeatures doFeatures() const override;
        MAGNUM_UI_LOCAL void doClean(Containers::BitArrayView layoutIdsToRemove) override;
        MAGNUM_UI_LOCAL void doLayout(Containers::BitArrayView layoutIdsToUpdate, const Containers::StridedArrayView1D<const UnsignedInt>& topLevelLayoutIds, const Containers::StridedArrayView1D<Vector2>& nodeMinSizes, const Containers::StridedArrayView1D<Vector2>& nodeMaxSizes, const Containers::StridedArrayView1D<Float>& nodeAspectRatios, const Containers::StridedArrayView1D<Vector4>& nodePaddings, const Containers::StridedArrayView1D<Vector4>& nodeMargins, const Containers::StridedArrayView1D<Vector2>& nodeOffsets, const Containers::StridedArrayView1D<Vector2>& nodeSizes) override;

        struct State;
        Containers::Pointer<State> _state;
};

}}

#endif
