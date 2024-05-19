#ifndef Magnum_Whee_Widget_h
#define Magnum_Whee_Widget_h
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
 * @brief Class @ref Magnum::Whee::Widget
 * @m_since_latest
 */

#include <Corrade/Containers/Reference.h>
#include <Corrade/Utility/Move.h>

#include "Magnum/Whee/Whee.h"
#include "Magnum/Whee/visibility.h"

namespace Magnum { namespace Whee {

/**
@brief Base for stateful widgets
@m_since_latest

A move-only owning wrapper over a @ref NodeHandle.

Stateful widgets (such as a @ref Button) are meant to be used when their visual
state will need be modified during their lifetime (for example, a button
changing its color or text), or when the widget implementation maintains a
state that is regularly queried by the application (for example, an input text
label, or a list selection).

In comparison, stateless widgets (such as a @ref button()) are only set up once
and then live until removed, either explicitly through their @ref NodeHandle,
or implicitly when the parent nodes are removed. Stateless widgets are more
lightweight as no individual destructors need to be called for them and should
be preferred where possible.
*/
class MAGNUM_WHEE_EXPORT Widget {
    public:
        /**
         * @brief Constructor
         * @param ui        User interface instance
         * @param node      Node to create the widget on
         *
         * Note that @p node *isn't* required to be a valid handle in @p ui.
         * @see @ref UserInterface::isHandleValid(NodeHandle) const
         */
        explicit Widget(UserInterface& ui, NodeHandle node): _ui{ui}, _node{node} {}

        /**
         * @brief Construct from a positioning anchor
         *
         * The @ref ui() and @ref node() is set to @ref Anchor::ui() and
         * @ref Anchor::node().
         */
        explicit Widget(const Anchor& anchor);

        /** @brief Copying is not allowed */
        Widget(const Widget&) = delete;

        /**
         * @brief Move constructor
         *
         * Performs a destructive move, i.e. the @p other @ref node() becomes
         * @ref NodeHandle::Null.
         */
        Widget(Widget&& other) noexcept;

        /**
         * @brief Destructor
         *
         * If @ref node() is not @ref NodeHandle::Null and is valid in
         * @ref ui(), calls @ref AbstractUserInterface::removeNode(). If
         * @ref node() is @ref NodeHandle::Null, the destructor is guaranteed
         * to not access @ref ui() in any way.
         * @see @ref release(),
         *      @ref AbstractUserInterface::isHandleValid(NodeHandle) const
         */
        ~Widget();

        /** @brief Copying is not allowed */
        Widget& operator=(const Widget&) = delete;

        /** @brief Move assignment */
        Widget& operator=(Widget&& other) noexcept;

        /** @brief User interface instance this widget is part of */
        UserInterface& ui() const { return _ui; }

        /**
         * @brief Widget node
         *
         * Returns @ref NodeHandle::Null for a moved-out or released widget.
         * The returned handle may be also invalid if
         * @ref AbstractUserInterface::removeNode() was explicitly called on it
         * or if any parent node was removed.
         * @see @ref release(),
         *      @ref AbstractUserInterface::isHandleValid(NodeHandle) const
         */
        NodeHandle node() const { return _node; }
        operator NodeHandle() const { return _node; } /**< @overload */

        /**
         * @brief Whether the widget is hidden
         *
         * Equivalent to querying @ref NodeFlag::Hidden on @ref node() with
         * @ref AbstractUserInterface::nodeFlags().
         */
        bool isHidden() const;

        /**
         * @brief Set the widget hidden
         *
         * Equivalent to adding or clearing @ref NodeFlag::Hidden on
         * @ref node() with @ref AbstractUserInterface::addNodeFlags() or
         * @relativeref{AbstractUserInterface,clearNodeFlags()}.
         */
        void setHidden(bool hidden = true);

        /**
         * @brief Release the widget node
         *
         * Returns the node handle and resets it to @ref NodeHandle::Null,
         * making the widget equivalent to a moved-out instance. Assuming the
         * handle was valid in the first place, the widget then becomes a
         * stateless one, and gets removed either when
         * @ref AbstractUserInterface::removeNode() is explicitly called on the
         * returned handle or if any parent node is removed.
         * @see @ref AbstractUserInterface::isHandleValid(NodeHandle) const
         */
        NodeHandle release();

    private:
        Containers::Reference<UserInterface> _ui;
        NodeHandle _node;
};

}}

#endif
