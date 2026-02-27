#ifndef Magnum_Ui_ScrollArea_h
#define Magnum_Ui_ScrollArea_h
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
 * @brief Class @ref Magnum::Ui::ScrollArea, enum @ref Magnum::Ui::ScrollAreaFlag, enum set @ref Magnum::Ui::ScrollAreaFlags
 * @m_since_latest_{extras}
 */

#include <Corrade/Containers/EnumSet.h>

#include "Magnum/Ui/Widget.h"

namespace Magnum { namespace Ui {

/**
@brief Scroll area flag
@m_since_latest_{extras}

@see @ref ScrollAreaFlags, @ref ScrollArea
*/
enum class ScrollAreaFlag: UnsignedByte {
    /**
     * Scroll only horizontally. If enabled, the scroll area will expand to
     * the width of the contents. Mutually exclusive with
     * @ref ScrollAreaFlag::OnlyY.
     */
    OnlyX = 1 << 0,

    /**
     * Scroll only vertically. If enabled, the scroll area will expand to the
     * height of the contents. Mutually exclusive with
     * @ref ScrollAreaFlag::OnlyX.
     */
    OnlyY = 1 << 1,

    /**
     * Don't scroll when dragging a pointer on the contents. By default the
     * scroll area reacts to pointer drag on the contents to support the common
     * drag-to-scroll pattern on touch screens, and it's interpreted as a
     * scroll only if the pointer is dragged beyond a threshold so taps and
     * clicks work on the contents as usual. Enable this flag if the
     * drag-to-scroll is undesired or conflicts with event handling in the
     * contents, such as if it's a canvas to be painted on.
     *
     * See @ref Ui-EventLayer-drag-to-scroll for a description of how the
     * drag-to-scroll is implemented internally.
     */
    NoContentsDrag = 1 << 2,
};

/**
@debugoperatorenum{ScrollAreaFlag}
@m_since_latest_{extras}
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, ScrollAreaFlag value);

/**
@brief Scroll area flags
@m_since_latest_{extras}

@see @ref ScrollArea
*/
typedef Containers::EnumSet<ScrollAreaFlag> ScrollAreaFlags;

/**
@debugoperatorenum{ScrollAreaFlags}
@m_since_latest_{extras}
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, ScrollAreaFlags value);

CORRADE_ENUMSET_OPERATORS(ScrollAreaFlags)

/**
@brief Scroll area widget
@m_since_latest_{extras}
*/
class MAGNUM_UI_EXPORT ScrollArea: public Widget {
    public:
        /**
         * @brief Constructor
         * @param anchor            Positioning anchor
         * @param flags             Scroll area flags
         */
        explicit ScrollArea(Anchor anchor, ScrollAreaFlags flags = {});

        /** @copydoc AbstractWidget::AbstractWidget(NoCreateT) */
        explicit ScrollArea(NoCreateT): Widget{NoCreate}, _viewNode{}, _contentsNode{}, _scrollbarXNode{}, _scrollbarThumbXNode{}, _scrollbarYNode{}, _scrollbarThumbYNode{}, _scrollXStorage{}, _scrollYStorage{} {}

        /**
         * @brief Flags
         *
         * Note that unlike with other widgets it's not possible to change the
         * flags after construction.
         */
        ScrollAreaFlags flags() const { return _flags; }

        /**
         * @brief Horizontal and vertical scroll percentage
         *
         * Guaranteed to be between @cpp 0.0f @ce and @cpp 100.0f @ce in both
         * directions. If either @ref ScrollAreaFlag::OnlyX or
         * @relativeref{ScrollAreaFlag,OnlyY} is enabled, given component is
         * @cpp 0.0f @ce.
         */
        Vector2 scrollPercentage() const;

        /**
         * @brief Scroll horizontally to given percentage
         * @return Reference to self (for method chaining)
         *
         * The @p percentage is clamped to a @f$ [0, 100] @f$ range. If
         * @ref ScrollAreaFlag::OnlyY is enabled or
         * @ref AbstractUserInterface::update() or
         * @relativeref{AbstractUserInterface,draw()} wasn't called yet to
         * layout the contents, the function is a no-op.
         */
        ScrollArea& scrollToPercentageX(Float percentage);

        /**
         * @brief Scroll vertically to given percentage
         * @return Reference to self (for method chaining)
         *
         * The @p percentage is clamped to a @f$ [0, 100] @f$ range. If
         * @ref ScrollAreaFlag::OnlyX is enabled or
         * @ref AbstractUserInterface::update() or
         * @relativeref{AbstractUserInterface,draw()} wasn't called yet to
         * layout the contents, the function is a no-op.
         */
        ScrollArea& scrollToPercentageY(Float percentage);

        /**
         * @brief Scroll to given percentage
         * @return Reference to self (for method chaining)
         *
         * Equivalent to calling @ref scrollToPercentageX() and
         * @ref scrollToPercentageY() with components of @p percentage.
         */
        ScrollArea& scrollToPercentage(const Vector2& percentage);

        /**
         * @brief Contents
         *
         * Use the returned anchor to put widgets inside the scroll area.
         */
        Anchor contents() const;

        /**
         * @brief View node
         *
         * Exposed mainly for testing purposes, not meant to be modified
         * directly. Use @ref contents() for placing widgets inside.
         */
        NodeHandle viewNode() const { return _viewNode; }

        /**
         * @brief Contents node
         *
         * Exposed mainly for testing purposes, not meant to be modified
         * directly. Use @ref contents() for placing widgets inside.
         */
        NodeHandle contentsNode() const { return _contentsNode; }

        /**
         * @brief Horizontal scrollbar node or @ref NodeHandle::Null
         *
         * Exposed mainly for testing purposes, not meant to be modified
         * directly.
         */
        NodeHandle scrollbarXNode() const { return _scrollbarXNode; }

        /**
         * @brief Horizontal scrollbar thumb node or @ref NodeHandle::Null
         *
         * Exposed mainly for testing purposes, not meant to be modified
         * directly.
         */
        NodeHandle scrollbarThumbXNode() const { return _scrollbarThumbXNode; }

        /**
         * @brief Vertical scrollbar node or @ref NodeHandle::Null
         *
         * Exposed mainly for testing purposes, not meant to be modified
         * directly.
         */
        NodeHandle scrollbarYNode() const { return _scrollbarYNode; }

        /**
         * @brief Vertical scrollbar thumb node or @ref NodeHandle::Null
         *
         * Exposed mainly for testing purposes, not meant to be modified
         * directly.
         */
        NodeHandle scrollbarThumbYNode() const { return _scrollbarThumbYNode; }

        /**
         * @brief Scroll X data storage or @ref StorageHandle::Null
         *
         * Exposed mainly for testing purposes, not meant to be modified
         * directly.
         */
        StorageHandle scrollXStorage() const;

        /**
         * @brief Scroll Y data storage or @ref StorageHandle::Null
         *
         * Exposed mainly for testing purposes, not meant to be modified
         * directly.
         */
        StorageHandle scrollYStorage() const;

        #ifndef DOXYGEN_GENERATING_OUTPUT
        _MAGNUM_UI_WIDGET_SUBCLASS_IMPLEMENTATION(ScrollArea) /* LCOV_EXCL_LINE */
        #endif

    private:
        NodeHandle _viewNode, _contentsNode,
            _scrollbarXNode, _scrollbarThumbXNode,
            _scrollbarYNode, _scrollbarThumbYNode;
        DataLayerStorageHandle _scrollXStorage, _scrollYStorage;
        ScrollAreaFlags _flags;
        /* 3 bytes free */
};

}}

#endif
