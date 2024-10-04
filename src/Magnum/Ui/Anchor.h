#ifndef Magnum_Ui_Anchor_h
#define Magnum_Ui_Anchor_h
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
 * @brief Class @ref Magnum::Ui::AbstractAnchor, @ref Magnum::Ui::BasicAnchor, typedef @ref Magnum::Ui::Anchor
 * @m_since_latest
 */

#include <Corrade/Containers/EnumSet.h>
#include <Corrade/Containers/Reference.h>
#include <Corrade/Utility/DebugAssert.h>

#include "Magnum/Ui/visibility.h"
#include "Magnum/Ui/Ui.h"

namespace Magnum { namespace Ui {

/**
@brief Base for widget positioning anchors
@m_since_latest

Wraps an @ref AbstractUserInterface reference, @ref NodeHandle and optional
@ref LayoutHandle. The @ref BasicAnchor template and the @ref Anchor typedef
then restrict the type to a concrete user interface instance.

Meant to be returned from layouter instances to construct widget instances
with.
*/
class MAGNUM_UI_EXPORT AbstractAnchor {
    public:
        /**
         * @brief Constructor
         * @param ui        User interface instance
         * @param node      Node handle
         * @param layout    Layout associated with given node or
         *      @ref LayoutHandle::Null
         *
         * The @p node is expected to be valid in @p ui. If @p layout is not
         * @ref LayoutHandle::Null, it's expected to be valid in @p ui and
         * associated with @p node.
         * @see @ref AbstractUserInterface::isHandleValid(NodeHandle) const,
         *      @ref AbstractUserInterface::isHandleValid(LayoutHandle) const,
         */
        explicit AbstractAnchor(AbstractUserInterface& ui, NodeHandle node, LayoutHandle layout);

        /**
         * @brief Create a custom-positioned anchor
         *
         * Calls @ref AbstractUserInterface::createNode() with @p parent,
         * @p offset, @p size and @p flags, and remembers the created
         * @ref NodeHandle. The @ref layout() is @ref LayoutHandle::Null.
         */
        /*implicit*/ AbstractAnchor(AbstractUserInterface& ui, NodeHandle parent, const Vector2& offset, const Vector2& size, NodeFlags flags = {});

        /**
         * @brief Create a custom-sized anchor
         *
         * Calls @ref AbstractUserInterface::createNode() with @p parent,
         * zero offset, @p size and @p flags, and remembers the created
         * @ref NodeHandle. The @ref layout() is @ref LayoutHandle::Null.
         */
        /*implicit*/ AbstractAnchor(AbstractUserInterface& ui, NodeHandle parent, const Vector2& size, NodeFlags flags = {});

        /**
         * @brief Create a custom-positioned top-level anchor
         *
         * Equivalent to calling @ref AbstractAnchor(AbstractUserInterface&, NodeHandle, const Vector2&, const Vector2&, NodeFlags)
         * with @ref NodeHandle::Null as the parent.
         */
        /*implicit*/ AbstractAnchor(AbstractUserInterface& ui, const Vector2& offset, const Vector2& size, NodeFlags flags = {});

        /**
         * @brief Create a custom-sized top-level anchor
         *
         * Equivalent to calling @ref AbstractAnchor(AbstractUserInterface&, NodeHandle, const Vector2&, NodeFlags)
         * with @ref NodeHandle::Null as the parent.
         */
        /*implicit*/ AbstractAnchor(AbstractUserInterface& ui, const Vector2& size, NodeFlags flags = {});

        /** @brief User interface instance */
        AbstractUserInterface& ui() const { return _ui; }

        /**
         * @brief Node handle
         *
         * Guaranteed to be never @ref NodeHandle::Null.
         */
        NodeHandle node() const { return _node; }

        /**
         * @brief Node handle
         *
         * Guaranteed to be never @ref NodeHandle::Null.
         */
        operator NodeHandle() const { return _node; }

        /**
         * @brief Layout handle
         *
         * Can be @ref LayoutHandle::Null, in which case the anchor doesn't
         * have any associated layout.
         */
        LayoutHandle layout() const { return _layout; }

        /**
         * @brief Layout handle
         *
         * Unlike @ref layout() expects that the handle is not null.
         */
        operator LayoutHandle() const;

    private:
        Containers::Reference<AbstractUserInterface> _ui;
        /* 0/4 bytes free */
        NodeHandle _node;
        LayoutHandle _layout;
};

/**
@brief Templated base for widget positioning anchors
@m_since_latest

Restricts @ref AbstractAnchor to a concrete user interface instance. See the
base class documentation for more information.
@see @ref Anchor
*/
template<class UserInterface> class BasicAnchor: public AbstractAnchor {
    public:
        /** @copydoc AbstractAnchor::AbstractAnchor(AbstractUserInterface&, NodeHandle, LayoutHandle) */
        explicit BasicAnchor(UserInterface& ui, NodeHandle node, LayoutHandle layout): AbstractAnchor{ui, node, layout} {}

        /** @copydoc AbstractAnchor::AbstractAnchor(AbstractUserInterface&, NodeHandle, const Vector2&, const Vector2&, NodeFlags) */
        /*implicit*/ BasicAnchor(UserInterface& ui, NodeHandle parent, const Vector2& offset, const Vector2& size, NodeFlags flags = {}): AbstractAnchor{ui, parent, offset, size, flags} {}

        /** @copydoc AbstractAnchor::AbstractAnchor(AbstractUserInterface&, NodeHandle, const Vector2&, NodeFlags) */
        /*implicit*/ BasicAnchor(UserInterface& ui, NodeHandle parent, const Vector2& size, NodeFlags flags = {}): AbstractAnchor{ui, parent, size, flags} {}

        /**
         * @brief Create a custom-positioned top-level anchor
         *
         * Equivalent to calling @ref BasicAnchor(UserInterface&, NodeHandle, const Vector2&, const Vector2&, NodeFlags)
         * with @ref NodeHandle::Null as the parent.
         */
        /*implicit*/ BasicAnchor(UserInterface& ui, const Vector2& offset, const Vector2& size, NodeFlags flags = {}): AbstractAnchor{ui, offset, size, flags} {}

        /**
         * @brief Create a custom-sized top-level anchor
         *
         * Equivalent to calling @ref BasicAnchor(UserInterface&, NodeHandle, const Vector2&, NodeFlags)
         * with @ref NodeHandle::Null as the parent.
         */
        /*implicit*/ BasicAnchor(UserInterface& ui, const Vector2& size, NodeFlags flags = {}): AbstractAnchor{ui, size, flags} {}

        /**
         * @brief Construct from an abstract anchor and a concrete user interface reference
         *
         * Expects that @ref AbstractAnchor::ui() matches @p ui.
         */
        explicit BasicAnchor(UserInterface& ui, const AbstractAnchor& anchor): AbstractAnchor{anchor} {
            #ifdef CORRADE_NO_DEBUG_ASSERT
            static_cast<void>(ui);
            #endif
            /* Yes, again a reinterpret_cast like in BasicWidget so we don't
               need to pull in UserInterface.h to know that UserInterface is
               derived from AbstractUserInterface just for the assert alone. */
            CORRADE_DEBUG_ASSERT(reinterpret_cast<AbstractUserInterface*>(&ui) == &anchor.ui(),
                "Ui::BasicAnchor: expected the user interface reference to match" << &anchor.ui() << "but got" << &ui, );
        }

        /** @brief User interface instance */
        UserInterface& ui() const {
            return static_cast<UserInterface&>(AbstractAnchor::ui());
        }
};

/**
@brief Widget positioning anchor for use with @ref UserInterface
@m_since_latest

See the @ref AbstractAnchor class documentation for more information.
*/
typedef BasicAnchor<UserInterface> Anchor;

}}

#endif
