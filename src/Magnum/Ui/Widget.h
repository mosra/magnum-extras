#ifndef Magnum_Ui_Widget_h
#define Magnum_Ui_Widget_h
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
 * @brief Class @ref Magnum::Ui::AbstractWidget, @ref Magnum::Ui::BasicWidget, typedef @ref Magnum::Ui::Widget, tag type @ref Magnum::Ui::NonOwnedT, tag @ref Magnum::Ui::NonOwned
 * @m_since_latest_{extras}
 */

#include <Corrade/Utility/Move.h>

#include "Magnum/Tags.h"
#include "Magnum/Ui/Ui.h"
#include "Magnum/Ui/visibility.h"

namespace Magnum { namespace Ui {

/**
@brief Non-owned construction tag type
@m_since_latest_{extras}

Used to distinguish widget construction that doesn't own the underlying
@ref NodeHandle.
@see @ref NonOwned,
    @ref AbstractWidget::AbstractWidget(NonOwnedT, AbstractUserInterface&, NodeHandle),
    @ref AbstractWidget::AbstractWidget(NonOwnedT, const AbstractAnchor&)
*/
struct NonOwnedT {
    #ifndef DOXYGEN_GENERATING_OUTPUT
    struct Init {};
    /* Explicit constructor to avoid ambiguous calls when using {} */
    constexpr explicit NonOwnedT(Init) {}
    #endif
};

/**
@brief No-owned construction tag
@m_since_latest_{extras}

Use for constructiing widgets without them owning the underlying
@ref NodeHandle, and thus not removing it on destruction.
@see @ref AbstractWidget::AbstractWidget(NonOwnedT, AbstractUserInterface&, NodeHandle),
    @ref AbstractWidget::AbstractWidget(NonOwnedT, const AbstractAnchor&)
*/
constexpr NonOwnedT NonOwned{NonOwnedT::Init{}};

/**
@brief Base for stateful widgets
@m_since_latest_{extras}

A move-only owning wrapper over a @ref NodeHandle along with an
@ref AbstractUserInterface reference. The @ref BasicWidget template and the
@ref Widget typedef then restrict the type to a concrete user interface
instance.

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
class MAGNUM_UI_EXPORT AbstractWidget {
    public:
        /**
         * @brief Constructor
         * @param ui        User interface instance
         * @param node      Node to create the widget on
         *
         * The @p node is expected to be valid in @p ui.
         * @see @ref AbstractUserInterface::isHandleValid(NodeHandle) const
         */
        explicit AbstractWidget(AbstractUserInterface& ui, NodeHandle node);

        /**
         * @brief Construct a non-owned widget
         * @param ui        User interface instance
         * @param node      Node to create the widget on
         *
         * The @p node is expected to be valid in @p ui. Compared to
         * @ref AbstractWidget(AbstractUserInterface&, NodeHandle) the @p node
         * doesn't get removed on destruction. Instead, it gets removed either
         * once any parent node is removed, or when
         * @ref AbstractUserInterface::removeNode() is explicitly called on
         * @ref node().
         * @see @ref AbstractUserInterface::isHandleValid(NodeHandle) const,
         *      @ref isOwned()
         */
        explicit AbstractWidget(NonOwnedT, AbstractUserInterface& ui, NodeHandle node): AbstractWidget{ui, node} {
            _owned = false;
        }

        /**
         * @brief Construct with no underlying node
         *
         * The instance is equivalent to a moved-out state, i.e. not usable
         * for anything. Move another instance over it to make it useful.
         */
        explicit AbstractWidget(NoCreateT): _ui{}, _node{}, _owned{} {}

        /**
         * @brief Construct from a positioning anchor
         *
         * The @ref ui() and @ref node() is set to @ref AbstractAnchor::ui()
         * and @ref AbstractAnchor::node().
         */
        /* Could be by-value but then it'd need the Anchor.h include for
           BasicWidget below */
        explicit AbstractWidget(const AbstractAnchor& anchor);

        /**
         * @brief Construct a non-owned widget from a positioning anchor
         *
         * The @ref ui() and @ref node() is set to @ref AbstractAnchor::ui()
         * and @ref AbstractAnchor::node(). Compared to
         * @ref AbstractWidget(const AbstractAnchor&) the widget node doesn't
         * get removed on destruction. Instead, it gets removed either once any
         * parent node is removed, or when
         * @ref AbstractUserInterface::removeNode() is explicitly called on
         * @ref node().
         * @see @ref isOwned()
         */
        explicit AbstractWidget(NonOwnedT, const AbstractAnchor& anchor): AbstractWidget{anchor} {
            _owned = false;
        }

        /** @brief Copying is not allowed */
        AbstractWidget(const AbstractWidget&) = delete;

        /**
         * @brief Move constructor
         *
         * Performs a destructive move, i.e. the @p other @ref node() becomes
         * @ref NodeHandle::Null.
         */
        AbstractWidget(AbstractWidget&& other) noexcept;

        /**
         * @brief Destructor
         *
         * If @ref isOwned() is @cpp true @ce and @ref node() is not
         * @ref NodeHandle::Null, expects it to be valid and calls
         * @ref AbstractUserInterface::removeNode(). If @ref isOwned() is
         * @cpp false @ce or @ref node() is @ref NodeHandle::Null, the
         * destructor is guaranteed to not access @ref ui() in any way.
         * @see @ref release(),
         *      @ref AbstractUserInterface::isHandleValid(NodeHandle) const
         */
        ~AbstractWidget();

        /** @brief Copying is not allowed */
        AbstractWidget& operator=(const AbstractWidget&) = delete;

        /** @brief Move assignment */
        AbstractWidget& operator=(AbstractWidget&& other) noexcept;

        /**
         * @brief Whether the widget is owned
         *
         * Returns @cpp false @ce if the widget was constructed using either
         * @ref AbstractWidget(NonOwnedT, AbstractUserInterface&, NodeHandle)
         * or @ref AbstractWidget(NonOwnedT, const AbstractAnchor&),
         * @cpp true @ce otherwise. If the widget is owned, the underlying
         * @ref node() gets removed on destruction.
         * @see @link ~AbstractWidget() @endlink, @ref release()
         */
        bool isOwned() const { return _owned; }

        /** @brief User interface instance this widget is part of */
        AbstractUserInterface& ui() const { return *_ui; }

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

        /**
         * @brief Widget node
         *
         * Same as @ref node().
         */
        /*implicit*/ operator NodeHandle() const { return _node; }

        /**
         * @brief Whether the widget is hidden
         *
         * Equivalent to querying @ref NodeFlag::Hidden on @ref node() with
         * @ref AbstractUserInterface::nodeFlags(). Note that the query doesn't
         * include the parents, which means it can still return @cpp false @ce
         * if the widget is hidden transitively.
         */
        bool isHidden() const;

        /**
         * @brief Set the widget hidden
         * @return Reference to self (for method chaining)
         *
         * Equivalent to adding or clearing @ref NodeFlag::Hidden on
         * @ref node() with @ref AbstractUserInterface::addNodeFlags() or
         * @relativeref{AbstractUserInterface,clearNodeFlags()}.
         */
        AbstractWidget& setHidden(bool hidden);

        /**
         * @brief Whether the widget is disabled
         *
         * Equivalent to querying @ref NodeFlag::Disabled on @ref node() with
         * @ref AbstractUserInterface::nodeFlags(). Note that the query doesn't
         * include the parents, which means it can still return @cpp false @ce
         * if the widget is disabled transitively.
         */
        bool isDisabled() const;

        /**
         * @brief Set the widget disabled
         * @return Reference to self (for method chaining)
         *
         * Equivalent to adding or clearing @ref NodeFlag::Disabled on
         * @ref node() with @ref AbstractUserInterface::addNodeFlags() or
         * @relativeref{AbstractUserInterface,clearNodeFlags()}.
         */
        AbstractWidget& setDisabled(bool disabled);

        /**
         * @brief Release the widget node
         *
         * Returns the node handle and resets it to @ref NodeHandle::Null,
         * making the widget equivalent to a moved-out instance. Assuming the
         * handle was valid in the first place, the widget then becomes a
         * stateless one, and gets removed either when
         * @ref AbstractUserInterface::removeNode() is explicitly called on the
         * returned handle or if any parent node is removed.
         *
         * Instead of calling this function to prevent a widget destructor from
         * removing the node you can also construct the widget using the
         * @ref NonOwned tag, if the subclass exposes it, or call a stateless
         * widget function instead of creating an instance instead (such as
         * @ref button() instead of making a @ref Button instance).
         * @see @ref AbstractUserInterface::isHandleValid(NodeHandle) const,
         *      @ref isOwned()
         */
        NodeHandle release();

    #ifdef DOXYGEN_GENERATING_OUTPUT
    private:
    #else
    protected:
    #endif
        /* To avoid subclass NonOwned constructor variants duplicating all the
           logic they should delegate to the non-NonOwned variant and then call
           this function */
        void makeNonOwned() { _owned = false; }

    private:
        AbstractUserInterface* _ui;
        NodeHandle _node;
        bool _owned;
        /* 3 bytes free */
};

#ifndef DOXYGEN_GENERATING_OUTPUT
#define _MAGNUM_UI_WIDGET_SUBCLASS_IMPLEMENTATION(T)                        \
    T& setHidden(bool hidden) {                                             \
        return static_cast<T&>(AbstractWidget::setHidden(hidden));          \
    }                                                                       \
    T& setDisabled(bool disabled) {                                         \
        return static_cast<T&>(AbstractWidget::setDisabled(disabled));      \
    }
#endif

/**
@brief Templated abstract base for stateful widgets
@m_since_latest_{extras}

Restricts @ref AbstractWidget to a concrete user interface instance. See the
base class documentation for more information.
@see @ref Widget
*/
template<class UserInterface> class BasicWidget: public AbstractWidget {
    public:
        /** @copydoc AbstractWidget::AbstractWidget(AbstractUserInterface&, NodeHandle) */
        explicit BasicWidget(UserInterface& ui, NodeHandle node): AbstractWidget{ui, node} {}

        /** @copydoc AbstractWidget::AbstractWidget(NonOwnedT, AbstractUserInterface&, NodeHandle) */
        explicit BasicWidget(NonOwnedT, UserInterface& ui, NodeHandle node): AbstractWidget{NonOwned, ui, node} {}

        /** @copydoc AbstractWidget::AbstractWidget(const AbstractAnchor&) */
        /* Could be by-value but then it'd need the Anchor.h include */
        explicit BasicWidget(const BasicAnchor<UserInterface>& anchor):
            /* Yes, a reinterpret_cast so we don't need to pull in Anchor.h to
               know that BasicAnchor is derived from AbstractAnchor in the
               usual cases where a widget gets created from a const& anchor
               (i.e. without the widget even needing to have the definition).
               Back in 2010 doing this caused me to fail a uni assignment, I
               still stand behind doing it to untangle complex dependencies. */
            AbstractWidget{reinterpret_cast<const AbstractAnchor&>(anchor)} {}

        /** @copydoc AbstractWidget::AbstractWidget(NonOwnedT, const AbstractAnchor&) */
        explicit BasicWidget(NonOwnedT, const BasicAnchor<UserInterface>& anchor):
            AbstractWidget{NonOwned, reinterpret_cast<const AbstractAnchor&>(anchor)} {}

        /** @copydoc AbstractWidget::AbstractWidget(NoCreateT) */
        explicit BasicWidget(NoCreateT): AbstractWidget{NoCreate} {}

        /** @brief Copying is not allowed */
        BasicWidget(const BasicWidget<UserInterface>&) = delete;

        /** @copydoc AbstractWidget::AbstractWidget(AbstractWidget&&) */
        BasicWidget(BasicWidget<UserInterface>&& other) noexcept = default;

        /** @brief Copying is not allowed */
        BasicWidget& operator=(const BasicWidget&) = delete;

        /** @brief Move assignment */
        BasicWidget& operator=(BasicWidget&& other) noexcept = default;

        /** @brief User interface instance this widget is part of */
        UserInterface& ui() const {
            return static_cast<UserInterface&>(AbstractWidget::ui());
        }

        #ifndef DOXYGEN_GENERATING_OUTPUT
        _MAGNUM_UI_WIDGET_SUBCLASS_IMPLEMENTATION(BasicWidget<UserInterface>)
        #endif
};

/**
@brief Base for stateful widgets for use with @ref UserInterface
@m_since_latest_{extras}

See the @ref AbstractWidget class documentation for more information.
*/
typedef BasicWidget<UserInterface> Widget;

}}

#endif
