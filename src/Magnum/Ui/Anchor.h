#ifndef Magnum_Ui_Anchor_h
#define Magnum_Ui_Anchor_h
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
 * @brief Class @ref Magnum::Ui::AbstractAnchor, @ref Magnum::Ui::BasicAnchor, typedef @ref Magnum::Ui::Anchor
 * @m_since_latest_{extras}
 */

#include <Corrade/Containers/EnumSet.h>

#include "Magnum/Ui/Ui.h"
#include "Magnum/Ui/visibility.h"

namespace Magnum { namespace Ui {

/**
@brief Base for widget positioning anchors
@m_since_latest_{extras}

A non-owning wrapper over a @ref NodeHandle along with an
@ref AbstractUserInterface reference and optional @ref LayoutHandle. The
@ref BasicAnchor template and the @ref Anchor typedef then restrict the type to
a concrete user interface instance.

Meant to be returned from layouter instances to construct widget instances
with.
*/
class MAGNUM_UI_EXPORT AbstractAnchor {
    public:
        /**
         * @brief Constructor
         * @param ui        User interface instance
         * @param node      Node handle
         *
         * The @p node is expected to be valid in @p ui.
         * @see @ref AbstractUserInterface::isHandleValid(NodeHandle) const
         */
        /*implicit*/ AbstractAnchor(AbstractUserInterface& ui, NodeHandle node);

        /**
         * @brief Construct from a widget instance
         *
         * Unlike @ref AbstractAnchor(AbstractUserInterface&, NodeHandle)
         * doesn't perform any checks and just assumes the widget has a live
         * @ref AbstractUserInterface reference and a valid @ref NodeHandle.
         */
        /*implicit*/ AbstractAnchor(const AbstractWidget& widget);

        /**
         * @brief Create a custom-positioned anchor
         *
         * Calls @ref AbstractUserInterface::createNode() with @p parent,
         * @p offset, @p size and @p flags, and remembers the created
         * @ref NodeHandle. See its documentation for detailed description of
         * all constraints.
         */
        /*implicit*/ AbstractAnchor(AbstractUserInterface& ui, NodeHandle parent, const Vector2& offset, const Vector2& size, NodeFlags flags = {});
        /** @overload */
        /*implicit*/ AbstractAnchor(AbstractAnchor parent, const Vector2& offset, const Vector2& size, NodeFlags flags = {}): AbstractAnchor{parent.ui(), parent.node(), offset, size, flags} {}

        /**
         * @brief Create a custom-positioned root anchor
         *
         * Equivalent to calling @ref AbstractAnchor(AbstractUserInterface&, NodeHandle, const Vector2&, const Vector2&, NodeFlags)
         * with @ref NodeHandle::Null as the parent.
         */
        /*implicit*/ AbstractAnchor(AbstractUserInterface& ui, const Vector2& offset, const Vector2& size, NodeFlags flags = {});

        /** @brief User interface instance */
        AbstractUserInterface& ui() const { return *_ui; }

        /**
         * @brief Node handle
         *
         * Guaranteed to be never @ref NodeHandle::Null.
         */
        NodeHandle node() const { return _node; }

        /**
         * @brief Node handle
         *
         * Same as @ref node().
         */
        /*implicit*/ operator NodeHandle() const { return _node; }

    private:
        AbstractUserInterface* _ui;
        /* 0/4 bytes free */
        NodeHandle _node;
};

/**
@brief Widget positioning anchor
@m_since_latest_{extras}

Restricts @ref AbstractAnchor to a concrete user interface instance. Meant to
be used through concrete a typedef such as @ref Anchor. See the base class
documentation for more information.
*/
template<class UserInterface> class BasicAnchor: public AbstractAnchor {
    public:
        /** @copydoc AbstractAnchor::AbstractAnchor(AbstractUserInterface&, NodeHandle) */
        /*implicit*/ BasicAnchor(UserInterface& ui, NodeHandle node): AbstractAnchor{ui, node} {}

        /** @copydoc AbstractAnchor::AbstractAnchor(const AbstractWidget&) */
        /*implicit*/ BasicAnchor(const BasicWidget<UserInterface>& widget):
            /* Yes, again a reinterpret_cast like in BasicWidget so we don't
               need to pull in Widget.h and UserInterface.h to know that
               BasicWidget<UserInterface> is derived from AbstractWidget. The
               subclass has no members and it's a trivial type so such a cast
               is fine. */
            AbstractAnchor{reinterpret_cast<const AbstractWidget&>(widget)} {}

        /** @copydoc AbstractAnchor::AbstractAnchor(AbstractUserInterface&, NodeHandle, const Vector2&, const Vector2&, NodeFlags) */
        /*implicit*/ BasicAnchor(UserInterface& ui, NodeHandle parent, const Vector2& offset, const Vector2& size, NodeFlags flags = {}): AbstractAnchor{ui, parent, offset, size, flags} {}
        /** @copydoc AbstractAnchor::AbstractAnchor(AbstractAnchor, const Vector2&, const Vector2&, NodeFlags) */
        /*implicit*/ BasicAnchor(BasicAnchor<UserInterface> parent, const Vector2& offset, const Vector2& size, NodeFlags flags = {}): AbstractAnchor{parent, offset, size, flags} {}

        /**
         * @brief Create a custom-positioned root anchor
         *
         * Equivalent to calling @ref BasicAnchor(UserInterface&, NodeHandle, const Vector2&, const Vector2&, NodeFlags)
         * with @ref NodeHandle::Null as the parent.
         */
        /*implicit*/ BasicAnchor(UserInterface& ui, const Vector2& offset, const Vector2& size, NodeFlags flags = {}): AbstractAnchor{ui, offset, size, flags} {}

        /** @brief User interface instance */
        UserInterface& ui() const {
            return static_cast<UserInterface&>(AbstractAnchor::ui());
        }
};

/**
@brief Widget positioning anchor for use with @ref UserInterface
@m_since_latest_{extras}

See the @ref AbstractAnchor class documentation for more information.
*/
typedef BasicAnchor<UserInterface> Anchor;

}}

#endif
