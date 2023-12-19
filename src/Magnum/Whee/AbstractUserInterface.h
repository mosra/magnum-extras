#ifndef Magnum_Whee_AbstractUserInterface_h
#define Magnum_Whee_AbstractUserInterface_h
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
 * @brief Class @ref Magnum::Whee::AbstractUserInterface, enum @ref Magnum::Whee::UserInterfaceState, @ref Magnum::Whee::NodeFlag, enum set @ref Magnum::Whee::UserInterfaceStates, @ref Magnum::Whee::NodeFlags
 * @m_since_latest
 */

#include <Corrade/Containers/EnumSet.h>
#include <Corrade/Containers/Pointer.h>
#include <Magnum/Magnum.h>

#include "Magnum/Whee/Whee.h"
#include "Magnum/Whee/visibility.h"

namespace Magnum { namespace Whee {

/**
@brief User interface state
@m_since_latest

Used to decide whether @ref AbstractUserInterface::clean() or
@ref AbstractUserInterface::update() need to be called to refresh the internal
state before the interface is drawn or an event is handled. See also
@ref LayerState for layer-specific state and @ref LayouterState for
layouter-specific state.
@see @ref UserInterfaceStates, @ref AbstractUserInterface::state()
*/
enum class UserInterfaceState: UnsignedShort {
    /**
     * @ref AbstractUserInterface::update() needs to be called to recalculate
     * or reupload data attached to visible node hierarchy after they've been
     * changed. Set implicitly if any of the layers have
     * @ref LayerState::NeedsUpdate set, is reset next time
     * @ref AbstractUserInterface::update() is called. Implied by
     * @ref UserInterfaceState::NeedsDataAttachmentUpdate,
     * @relativeref{UserInterfaceState,NeedsNodeEnabledUpdate},
     * @relativeref{UserInterfaceState,NeedsNodeClipUpdate},
     * @relativeref{UserInterfaceState,NeedsLayoutUpdate},
     * @relativeref{UserInterfaceState,NeedsLayoutAssignmentUpdate},
     * @relativeref{UserInterfaceState,NeedsNodeUpdate} and
     * @relativeref{UserInterfaceState,NeedsNodeClean}, so it's also set by
     * everything that sets those flags.
     */
    NeedsDataUpdate = 1 << 0,

    /**
     * @ref AbstractUserInterface::update() needs to be called to refresh the
     * data attached to visible node hierarchy after new data were attached or
     * after existing attachments were removed and
     * @ref AbstractUserInterface::clean() was called. Set implicitly if any of
     * the layers have @ref LayerState::NeedsAttachmentUpdate set, after every
     * @ref AbstractUserInterface::removeLayer() call and transitively after
     * every @ref AbstractUserInterface::attachData() call, is reset next time
     * @ref AbstractUserInterface::update() is called. Implies
     * @ref UserInterfaceState::NeedsDataUpdate. Implied by
     * @relativeref{UserInterfaceState,NeedsNodeEnabledUpdate},
     * @relativeref{UserInterfaceState,NeedsNodeClipUpdate},
     * @relativeref{UserInterfaceState,NeedsLayoutUpdate},
     * @relativeref{UserInterfaceState,NeedsLayoutAssignmentUpdate},
     * @relativeref{UserInterfaceState,NeedsNodeUpdate} and
     * @relativeref{UserInterfaceState,NeedsNodeClean}, so it's also set by
     * everything that sets those flags.
     */
    NeedsDataAttachmentUpdate = NeedsDataUpdate|(1 << 1),

    /**
     * @ref AbstractUserInterface::update() needs to be called to refresh the
     * enabled node set after node flags changed. Set implicitly after every
     * @relativeref{AbstractUserInterface,setNodeFlags()},
     * @relativeref{AbstractUserInterface,addNodeFlags()} and
     * @relativeref{AbstractUserInterface,clearNodeFlags()} that changes the
     * presence of the @ref NodeFlag::NoEvents or @ref NodeFlag::Disabled
     * flag; is reset next time @ref AbstractUserInterface::update() is called.
     * Implies @ref UserInterfaceState::NeedsDataAttachmentUpdate. Implied by
     * @relativeref{UserInterfaceState,NeedsNodeClipUpdate},
     * @relativeref{UserInterfaceState,NeedsLayoutUpdate},
     * @relativeref{UserInterfaceState,NeedsLayoutAssignmentUpdate},
     * @relativeref{UserInterfaceState,NeedsNodeUpdate} and
     * @relativeref{UserInterfaceState,NeedsNodeClean}, so it's also set by
     * everything that sets those flags.
     */
    NeedsNodeEnabledUpdate = NeedsDataAttachmentUpdate|(1 << 2),

    /**
     * @ref AbstractUserInterface::update() needs to be called to refresh the
     * visible node set after node flags changed. Set implicitly after every
     * @relativeref{AbstractUserInterface,setNodeFlags()},
     * @relativeref{AbstractUserInterface,addNodeFlags()} and
     * @relativeref{AbstractUserInterface,clearNodeFlags()} that changes the
     * presence of the @ref NodeFlag::Clip flag; is reset next time
     * @ref AbstractUserInterface::update() is called. Implies
     * @ref UserInterfaceState::NeedsNodeEnabledUpdate. Implied by
     * @relativeref{UserInterfaceState,NeedsLayoutUpdate},
     * @relativeref{UserInterfaceState,NeedsLayoutAssignmentUpdate},
     * @relativeref{UserInterfaceState,NeedsNodeUpdate} and
     * @relativeref{UserInterfaceState,NeedsNodeClean}, so it's also set by
     * everything that sets those flags.
     */
    NeedsNodeClipUpdate = NeedsNodeEnabledUpdate|(1 << 3),

    /**
     * @ref AbstractUserInterface::update() needs to be called to refresh the
     * visible node hierarchy layout after node sizes or offsets changed. Set
     * implicitly if any of the layouters have
     * @ref LayouterState::NeedsUpdate set and after every
     * @ref AbstractUserInterface::setNodeOffset() and
     * @ref AbstractUserInterface::setNodeSize(), is reset next time
     * @ref AbstractUserInterface::update() is called. Implies
     * @ref UserInterfaceState::NeedsNodeClipUpdate. Implied by
     * @relativeref{UserInterfaceState,NeedsLayoutAssignmentUpdate},
     * @relativeref{UserInterfaceState,NeedsNodeUpdate} and
     * @relativeref{UserInterfaceState,NeedsNodeClean}, so it's also set by
     * everything that sets those flags.
     */
    NeedsLayoutUpdate = NeedsNodeClipUpdate|(1 << 4),

    /**
     * @ref AbstractUserInterface::update() needs to be called to refresh the
     * layouts assigned to visible node hierarchy after new layouts were added
     * or after existing layouts were removed and
     * @ref AbstractUserInterface::clean() was called. Set implicitly if any of
     * the layouters have @ref LayouterState::NeedsAssignmentUpdate set and
     * after every @ref AbstractUserInterface::removeLayouter() call, is reset
     * next time @ref AbstractUserInterface::update() is called. Implies
     * @ref UserInterfaceState::NeedsLayoutUpdate. Implied by
     * @relativeref{UserInterfaceState,NeedsNodeUpdate} and
     * @relativeref{UserInterfaceState,NeedsNodeClean}, so it's also set by
     * everything that sets those flags.
     */
    NeedsLayoutAssignmentUpdate = NeedsLayoutUpdate|(1 << 5),

    /**
     * @ref AbstractUserInterface::update() needs to be called to refresh the
     * visible node hierarchy and data attached to it after nodes were added or
     * removed, made hidden or visible again or the top-level node order
     * changed. Set implicitly after every
     * @ref AbstractUserInterface::createNode(),
     * @relativeref{AbstractUserInterface,setNodeOrder()} and
     * @relativeref{AbstractUserInterface,clearNodeOrder()} call and after
     * every @relativeref{AbstractUserInterface,setNodeFlags()},
     * @relativeref{AbstractUserInterface,addNodeFlags()} and
     * @relativeref{AbstractUserInterface,clearNodeFlags()} call that changes
     * the presence of the @ref NodeFlag::Hidden flag; is reset next time
     * @ref AbstractUserInterface::update() is called. Implies
     * @ref UserInterfaceState::NeedsLayoutAssignmentUpdate. Implied by
     * @relativeref{UserInterfaceState,NeedsNodeClean}, so it's also set by
     * everything that sets that flag.
     */
    NeedsNodeUpdate = NeedsLayoutAssignmentUpdate|(1 << 6),

    /**
     * @ref AbstractUserInterface::clean() needs to be called to prune child
     * hierarchies of removed nodes, layouts assigned to those and data
     * attached to those. Set implicitly after every
     * @ref AbstractUserInterface::removeNode() call, is reset to
     * @ref UserInterfaceState::NeedsNodeUpdate next time
     * @ref AbstractUserInterface::clean() is called. Implies
     * @ref UserInterfaceState::NeedsNodeUpdate.
     */
    NeedsNodeClean = NeedsNodeUpdate|(1 << 7),

    /**
     * @ref AbstractUserInterface::updateRenderer() needs to be called to set
     * up renderer framebuffers after user interface size was changed. Set
     * after every @ref AbstractUserInterface::setSize() call if the
     * framebuffer size changes and a renderer instance is already set at that
     * point, is reset next time @ref AbstractUserInterface::updateRenderer()
     * is called.
     */
    NeedsRendererSizeSetup = 1 << 8,
};

/**
@debugoperatorenum{UserInterfaceState}
@m_since_latest
*/
MAGNUM_WHEE_EXPORT Debug& operator<<(Debug& debug, UserInterfaceState value);

/**
@brief User interface states
@m_since_latest

@see @ref AbstractUserInterface::state()
*/
typedef Containers::EnumSet<UserInterfaceState> UserInterfaceStates;

/**
@debugoperatorenum{UserInterfaceStates}
@m_since_latest
*/
MAGNUM_WHEE_EXPORT Debug& operator<<(Debug& debug, UserInterfaceStates value);

CORRADE_ENUMSET_OPERATORS(UserInterfaceStates)

/**
@brief Node behavior flag
@m_since_latest

@see @ref NodeFlags, @ref AbstractUserInterface::createNode(),
    @ref AbstractUserInterface::nodeFlags(),
    @ref AbstractUserInterface::setNodeFlags(),
    @ref AbstractUserInterface::addNodeFlags(),
    @ref AbstractUserInterface::clearNodeFlags()
*/
enum class NodeFlag: UnsignedByte {
    /**
     * The node, all nested nodes and all attached data are hidden, i.e. not
     * drawn and excluded from event processing.
     *
     * For top-level nodes (i.e., nodes for which
     * @ref AbstractUserInterface::nodeParent() is @ref NodeHandle::Null) a
     * draw order is managed in addition to the hidden state, using
     * @ref AbstractUserInterface::setNodeOrder() and
     * @relativeref{AbstractUserInterface,clearNodeOrder()}. A top-level node
     * that isn't in the draw order list behaves the same as if
     * @ref NodeFlag::Hidden was set for it. For performance reasons it's
     * however recommended to keep the draw list small rather than having it
     * full of mostly hidden nodes.
     *
     * Changing this flag causes @ref UserInterfaceState::NeedsNodeUpdate to be
     * set.
     * @see @ref NodeFlag::NoEvents, @ref NodeFlag::Disabled
     */
    Hidden = 1 << 0,

    /**
     * The node clips its contents. When enabled, child nodes that are
     * completely outside of the node rectangle are culled and not even drawn,
     * nodes that are partially outside are clipped.
     *
     * Changing this flag causes @ref UserInterfaceState::NeedsNodeClipUpdate
     * to be set.
     */
    Clip = 1 << 1,

    /**
     * The node, all nested nodes and all attached data don't get any events
     * even if a particular layer implements event handlers. Doesn't have any
     * visual effect, see @ref NodeFlag::Disabled or @ref NodeFlag::Hidden for
     * alternatives.
     *
     * Changing this flag causes
     * @ref UserInterfaceState::NeedsNodeEnabledUpdate to be set.
     */
    NoEvents = 1 << 2,

    /**
     * The node, all nested nodes and all attached data are disabled. Implies
     * @ref NodeFlag::NoEvents and additionally has a visual effect on layers
     * that implement a disabled state.
     *
     * Changing this flag causes
     * @ref UserInterfaceState::NeedsNodeEnabledUpdate to be set.
     * @see @ref NodeFlag::Hidden
     */
    Disabled = NoEvents|(1 << 3),
};

/**
@debugoperatorenum{NodeFlag}
@m_since_latest
*/
MAGNUM_WHEE_EXPORT Debug& operator<<(Debug& debug, NodeFlag value);

/**
@brief Node behavior flags
@m_since_latest

@see @ref AbstractUserInterface::createNode()
*/
typedef Containers::EnumSet<NodeFlag> NodeFlags;

/**
@debugoperatorenum{NodeFlags}
@m_since_latest
*/
MAGNUM_WHEE_EXPORT Debug& operator<<(Debug& debug, NodeFlags value);

CORRADE_ENUMSET_OPERATORS(NodeFlags)

/**
@brief Base for the main user interface
@m_since_latest

Doesn't contain any implicit layers or layouters. You'll most likely want to
instantiate the @ref UserInterface subclass instead.

@section Whee-AbstractUserInterface-dpi DPI awareness

There are three separate concepts for DPI-aware UI rendering:

-   UI size --- size of the user interface to which all widgets are positioned
-   Window size --- size of the window to which all input events are related
-   Framebuffer size --- size of the framebuffer the UI is being rendered to

Depending on the platform and use case, each of these three values can be
different. For example, a game menu screen can have the UI size the same
regardless of window size. Or on Retina macOS you can have different window and
framebuffer size and the UI size might be related to window size but
independent on the framebuffer size.

When using for example @ref Platform::Sdl2Application or other `*Application`
implementations, you usually have three values at your disposal ---
@ref Platform::Sdl2Application::windowSize() "windowSize()",
@ref Platform::Sdl2Application::framebufferSize() "framebufferSize()" and
@ref Platform::Sdl2Application::dpiScaling() "dpiScaling()". If you want the UI
to have the same layout and just scale on bigger window sizes, pass a fixed
value to the UI size:

@snippet Whee-sdl2.cpp AbstractUserInterface-dpi-fixed

If you want the UI to get more room with larger window sizes and behave
properly with different DPI scaling values, pass a ratio of window size and DPI
scaling to the UI size:

@snippet Whee-sdl2.cpp AbstractUserInterface-dpi-ratio

Finally, to gracefully deal with extremely small or extremely large windows,
you can apply @ref Math::clamp() on top with some defined bounds. Windows
outside of the reasonable size range will then get a scaled version of the UI
at boundary size:

@snippet Whee-sdl2.cpp AbstractUserInterface-dpi-clamp
*/
class MAGNUM_WHEE_EXPORT AbstractUserInterface {
    public:
        /**
         * @brief Construct without creating the user interface with concrete parameters
         *
         * You're expected to call @ref setSize() afterwards in order to define
         * scaling of event coordinates, node positions and projection matrices
         * for drawing.
         */
        explicit AbstractUserInterface(NoCreateT);

        /**
         * @brief Construct
         * @param size                  Size of the user interface to which
         *      everything is positioned
         * @param windowSize            Size of the window to which all input
         *      events are related
         * @param framebufferSize       Size of the window framebuffer. On
         *      some platforms with HiDPI screens may be different from window
         *      size.
         *
         * Equivalent to constructing with @ref AbstractUserInterface(NoCreateT)
         * and then calling @ref setSize(const Vector2&, const Vector2&, const Vector2i&).
         * See its documentation for more information.
         */
        explicit AbstractUserInterface(const Vector2& size, const Vector2& windowSize, const Vector2i& framebufferSize);

        /**
         * @brief Construct with an unscaled size
         *
         * Delegates to @ref AbstractUserInterface(const Vector2&, const Vector2&, const Vector2i&)
         * with all sizes set to @p size. Doing so assumes that the coordinate
         * system in which events are passed matches framebuffer size.
         */
        explicit AbstractUserInterface(const Vector2i& size);

        /** @brief Copying is not allowed */
        AbstractUserInterface(const AbstractUserInterface&) = delete;

        /**
         * @brief Move constructor
         *
         * Performs a destructive move, i.e. the original object isn't usable
         * afterwards anymore.
         */
        AbstractUserInterface(AbstractUserInterface&&) noexcept;

        /** @brief Copying is not allowed */
        AbstractUserInterface& operator=(const AbstractUserInterface&) = delete;

        /** @brief Move assignment */
        AbstractUserInterface& operator=(AbstractUserInterface&&) noexcept;

        ~AbstractUserInterface();

        /**
         * @brief User interface size
         *
         * Node positioning is in respect to this size. If @ref setSize() or
         * @ref AbstractUserInterface(const Vector2&, const Vector2&, const Vector2i&)
         * wasn't called yet, initial value is a zero vector.
         */
        Vector2 size() const;

        /**
         * @brief Window size
         *
         * Global event position in @ref pointerPressEvent(),
         * @ref pointerReleaseEvent() and @ref pointerMoveEvent() is in respect
         * to this size. If @ref setSize() or @ref AbstractUserInterface(const Vector2&, const Vector2&, const Vector2i&)
         * wasn't called yet, initial value is a zero vector.
         */
        Vector2 windowSize() const;

        /**
         * @brief Framebuffer size
         *
         * Rendering performed by layers is in respect to this size. If
         * @ref setSize() or
         * @ref AbstractUserInterface(const Vector2&, const Vector2&, const Vector2i&)
         * wasn't called yet, initial value is a zero vector.
         */
        Vector2i framebufferSize() const;

        /**
         * @brief Set user interface size
         * @param size                  Size of the user interface to which
         *      everything is positioned
         * @param windowSize            Size of the window to which all input
         *      events are related
         * @param framebufferSize       Size of the window framebuffer. On
         *      some platforms with HiDPI screens may be different from window
         *      size.
         * @return Reference to self (for method chaining)
         *
         * All sizes are expected to be non-zero, origin is top left for all.
         *
         * After calling this function, the @ref pointerPressEvent(),
         * @ref pointerReleaseEvent() and @ref pointerMoveEvent() functions
         * take the global event position with respect to @p windowSize, which
         * is then rescaled to match @p size when exposed through
         * @ref PointerEvent. The @p size and @p framebufferSize is passed
         * through to @ref AbstractLayer::setSize() to all layers with
         * @ref LayerFeature::Draw so they can set appropriate projection and
         * other framebuffer-related properties.
         *
         * There's no default size and this function is expected to be called
         * before the first @ref update() happens, either directly or through
         * the @ref AbstractUserInterface(const Vector2&, const Vector2&, const Vector2i&)
         * constructor. It's allowed to call this function for the first time
         * even after node, layers or data were created.
         *
         * Calling this function with new values will update the event position
         * scaling accordingly. If @p size or @p framebufferSize changes,
         * @ref AbstractLayer::setSize() is called on all layers. If a renderer
         * instance is set and this function is called for the first time,
         * @ref AbstractRenderer::setupFramebuffers() is called to make the
         * renderer populate its internal state. On subsequent calls to this
         * function with a renderer instance present, only
         * @ref UserInterfaceState::NeedsRendererSizeSetup is set and the
         * framebuffer size setup is deferred to when @ref updateRenderer() is
         * called, either directly or through @ref update() or @ref draw(). If
         * a renderer instance isn't set yet when calling this function, the
         * framebuffer setup is performed in the next
         * @ref setRendererInstance() call instead.
         */
        AbstractUserInterface& setSize(const Vector2& size, const Vector2& windowSize, const Vector2i& framebufferSize);

        /**
         * @brief Set unscaled user interface size
         * @return Reference to self (for method chaining)
         *
         * Calls @ref setSize(const Vector2&, const Vector2&, const Vector2i&)
         * with all sizes set to @p size. Doing so assumes that the coordinate
         * system in which events are passed matches framebuffer size.
         */
        AbstractUserInterface& setSize(const Vector2i& size);

        /**
         * @brief User interface state
         *
         * See the @ref UserInterfaceState enum for more information. By
         * default no flags are set.
         */
        UserInterfaceStates state() const;

        /** @{
         * @name Renderer management
         */

        /**
         * @brief Set renderer instance
         *
         * Expects that the instance hasn't been set yet. A renderer instance
         * has to be set in order to draw anything, it's the user
         * responsibility to ensure that the GPU API used by the renderer
         * matches the GPU API used by all layer instances, such as
         * @ref RendererGL being used for @ref BaseLayerGL and
         * @ref TextLayerGL. The instance is subsequently available through
         * @ref renderer().
         *
         * If framebuffer size was set with @ref setSize() already, calling
         * this function causes @ref AbstractRenderer::setupFramebuffers() to
         * be called. Otherwise the setup gets performed in the next
         * @ref setSize() call. Subsequent calls to @ref setSize() only set
         * @ref UserInterfaceState::NeedsRendererSizeSetup and the framebuffer
         * size setup is deferred to when @ref updateRenderer() is called,
         * either directly or through @ref update() or @ref draw().
         * @see @ref hasRenderer()
         */
        AbstractRenderer& setRendererInstance(Containers::Pointer<AbstractRenderer>&& instance);
        /** @overload */
        template<class T> T& setRendererInstance(Containers::Pointer<T>&& instance) {
            return static_cast<T&>(setRendererInstance(Containers::Pointer<AbstractRenderer>{Utility::move(instance)}));
        }

        /**
         * @brief Whether a renderer instance has been set
         *
         * @see @ref renderer(), @ref setRendererInstance()
         */
        bool hasRenderer() const;

        /**
         * @brief Renderer instance
         *
         * Expects that @ref setRendererInstance() was called.
         * @see @ref UserInterfaceGL::renderer()
         */
        AbstractRenderer& renderer();
        const AbstractRenderer& renderer() const; /**< @overload */

        /**
         * @brief Renderer instance in a concrete type
         *
         * Expected that @ref setRendererInstance() was called. It's the user
         * responsibility to ensure that @p T matches the actual instance type.
         */
        template<class T> T& renderer() {
            return static_cast<T&>(renderer());
        }
        /** @overload */
        template<class T> const T& renderer() const {
            return static_cast<const T&>(renderer());
        }

        /**
         * @}
         */

        /** @{
         * @name Layer and data management
         */

        /**
         * @brief Capacity of the layer storage
         *
         * Can be at most 256. If @ref createLayer() is called and there's no
         * free slots left, the internal storage gets grown.
         * @see @ref layerUsedCount()
         */
        std::size_t layerCapacity() const;

        /**
         * @brief Count of used items in the layer storage
         *
         * Always at most @ref layerCapacity(). Expired handles are counted
         * among used as well. The operation is done with a
         * @f$ \mathcal{O}(n) @f$ complexity where @f$ n @f$ is
         * @ref layerCapacity().
         */
        std::size_t layerUsedCount() const;

        /**
         * @brief Whether a layer handle is valid
         *
         * A handle is valid if it has been returned from @ref createLayer()
         * before and @ref removeLayer() wasn't called on it yet. Note that a
         * handle is valid even if the layer instance wasn't set with
         * @ref setLayerInstance() yet. For @ref LayerHandle::Null always
         * returns @cpp false @ce.
         */
        bool isHandleValid(LayerHandle handle) const;

        /**
         * @brief Whether a data handle is valid
         *
         * A shorthand for extracting a @ref LayerHandle from @p handle using
         * @ref dataHandleLayer(), calling @ref isHandleValid(LayerHandle) const
         * on it, if it's valid and set then retrieving the particular layer
         * instance using @ref layer() and then calling
         * @ref AbstractLayer::isHandleValid(LayerDataHandle) const with a
         * @ref LayerDataHandle extracted from @p handle using
         * @ref dataHandleData(). See these functions for more information. For
         * @ref DataHandle::Null, @ref LayerHandle::Null or
         * @ref LayerDataHandle::Null always returns @cpp false @ce.
         */
        bool isHandleValid(DataHandle handle) const;

        /**
         * @brief First layer in draw and event processing order
         *
         * The first layer gets drawn first (thus is at the back) and reacts to
         * events after all others. Returns @ref LayerHandle::Null if there's
         * no layers yet. The returned handle is always either valid or null.
         * @see @ref layerNext(), @ref layerLast(), @ref layerPrevious()
         */
        LayerHandle layerFirst() const;

        /**
         * @brief Last layer in draw and event processing order
         *
         * The last layer gets drawn last (thus is at the front) and reacts to
         * event before all others. Returns @ref LayerHandle::Null if there's
         * no layers yet. The returned handle is always either valid or null.
         * @see @ref layerPrevious(), @ref layerFirst(), @ref layerNext()
         */
        LayerHandle layerLast() const;

        /**
         * @brief Previous layer in draw and event processing order
         *
         * The previous layer gets drawn earlier (thus is behind) and reacts to
         * events later. Expects that @p handle is valid. Returns
         * @ref LayerHandle::Null if the layer is first. The returned handle
         * is always either valid or null.
         * @see @ref isHandleValid(LayerHandle) const, @ref layerNext(),
         *      @ref layerFirst(), @ref layerLast()
         */
        LayerHandle layerPrevious(LayerHandle handle) const;

        /**
         * @brief Next layer in draw and event processing order
         *
         * The next layer gets drawn later (thus is in front) and reacts to
         * events earlier. Expects that @p handle is valid. Returns
         * @ref LayerHandle::Null if the layer is last. The returned handle is
         * always either valid or null.
         * @see @ref isHandleValid(LayerHandle) const, @ref layerPrevious(),
         *      @ref layerLast(), @ref layerFirst()
         */
        LayerHandle layerNext(LayerHandle handle) const;

        /**
         * @brief Create a layer
         * @param before    A layer to order before for draw and event
         *      processing or @ref LayerHandle::Null if ordered as last (i.e.,
         *      at the front, being drawn last and receiving events first).
         *      Expected to be valid if not null.
         * @return New layer handle
         *
         * Allocates a new handle in a free slot in the internal storage or
         * grows the storage if there's no free slots left. Expects that
         * there's at most 256 layers. The returned handle is meant to be used
         * to construct an @ref AbstractLayer subclass and the instance then
         * passed to @ref setLayerInstance(). A layer can be removed again with
         * @ref removeLayer().
         * @see @ref isHandleValid(LayerHandle) const,
         *      @ref layerCapacity(), @ref layerUsedCount()
         */
        LayerHandle createLayer(LayerHandle before =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            LayerHandle::Null
            #else
            LayerHandle{} /* To not have to include Handle.h */
            #endif
        );

        /**
         * @brief Set a layer instance
         * @return Reference to @p instance
         *
         * Expects that @p instance was created with a @ref LayerHandle
         * returned from @ref createLayer() earlier, the handle is valid and
         * @ref setLayerInstance() wasn't called for the same handle yet.
         *
         * Calls @ref AbstractLayer::setSize() on the layer, unless neither
         * @ref setSize() nor @ref AbstractUserInterface(const Vector2&, const Vector2&, const Vector2i&)
         * was called yet.
         * @see @ref AbstractLayer::handle(),
         *      @ref isHandleValid(LayerHandle) const
         */
        AbstractLayer& setLayerInstance(Containers::Pointer<AbstractLayer>&& instance);
        /** @overload */
        template<class T> T& setLayerInstance(Containers::Pointer<T>&& instance) {
            return static_cast<T&>(setLayerInstance(Containers::Pointer<AbstractLayer>{Utility::move(instance)}));
        }

        /**
         * @brief Layer instance
         *
         * Expects that @p handle is valid and that @ref setLayerInstance() was
         * called for it.
         * @see @ref isHandleValid(LayerHandle) const
         */
        AbstractLayer& layer(LayerHandle handle);
        const AbstractLayer& layer(LayerHandle handle) const; /**< @overload */

        /**
         * @brief Layer instance in a concrete type
         *
         * Expects that @p handle is valid and that @ref setLayerInstance() was
         * called for it. It's the user responsibility to ensure that @p T
         * matches the actual instance type.
         * @see @ref isHandleValid(LayerHandle) const
         */
        template<class T> T& layer(LayerHandle handle) {
            return static_cast<T&>(layer(handle));
        }
        /** @overload */
        template<class T> const T& layer(LayerHandle handle) const {
            return static_cast<const T&>(layer(handle));
        }

        /**
         * @brief Remove a layer
         *
         * Expects that @p handle is valid. After this call,
         * @ref isHandleValid(LayerHandle) const returns @cpp false @ce for
         * @p handle and @ref isHandleValid(DataHandle) const returns
         * @cpp false @ce for all data associated with @p handle.
         *
         * Calling this function causes
         * @ref UserInterfaceState::NeedsDataAttachmentUpdate to be set.
         * @see @ref clean()
         */
        void removeLayer(LayerHandle handle);

        /**
         * @brief Attach data to a node
         *
         * A shorthand for extracting a @ref LayerHandle from @p data using
         * @ref dataHandleLayer(), retrieving the particular layer instance
         * using @ref layer() and then calling
         * @ref AbstractLayer::attach(LayerDataHandle, NodeHandle) with a
         * @ref LayerDataHandle extracted with @ref dataHandleData(). See these
         * functions for more information. In addition to
         * @ref AbstractLayer::attach(LayerDataHandle, NodeHandle), this
         * function checks that @p node is either valid or
         * @ref NodeHandle::Null.
         *
         * Calling this function transitively causes
         * @ref UserInterfaceState::NeedsDataAttachmentUpdate to be set, which
         * is a consequence of @ref LayerState::NeedsAttachmentUpdate being set
         * by @ref AbstractLayer::attach().
         * @see @ref update()
         */
        void attachData(NodeHandle node, DataHandle data);

        /**
         * @}
         */

        /** @{
         * @name Node layouter management
         */

        /**
         * @brief Capacity of the layouter storage
         *
         * Can be at most 256. If @ref createLayouter() is called and there's
         * no free slots left, the internal storage gets grown.
         * @see @ref layouterUsedCount()
         */
        std::size_t layouterCapacity() const;

        /**
         * @brief Count of used items in the layouter storage
         *
         * Always at most @ref layouterCapacity(). Expired handles are counted
         * among used as well. The operation is done with a
         * @f$ \mathcal{O}(n) @f$ complexity where @f$ n @f$ is
         * @ref layouterCapacity().
         */
        std::size_t layouterUsedCount() const;

        /**
         * @brief Whether a layouter handle is valid
         *
         * A handle is valid if it has been returned from @ref createLayouter()
         * before and @ref removeLayouter() wasn't called on it yet. Note that
         * a handle is valid even if the layouter instance wasn't set with
         * @ref setLayouterInstance() yet. For @ref LayouterHandle::Null always
         * returns @cpp false @ce.
         */
        bool isHandleValid(LayouterHandle handle) const;

        /**
         * @brief Whether a layout handle is valid
         *
         * A shorthand for extracting a @ref LayouterHandle from @p handle
         * using @ref layoutHandleLayouter(), calling
         * @ref isHandleValid(LayouterHandle) const on it, if it's valid and
         * set then retrieving the particular layouter instance using
         * @ref layouter() and then calling
         * @ref AbstractLayouter::isHandleValid(LayouterDataHandle) const with
         * a @ref LayouterDataHandle extracted from @p handle using
         * @ref layoutHandleData(). See these functions for more information.
         * For @ref LayoutHandle::Null, @ref LayouterHandle::Null or
         * @ref LayouterDataHandle::Null always returns @cpp false @ce.
         */
        bool isHandleValid(LayoutHandle handle) const;

        /**
         * @brief First layouter in the layout calculation order
         *
         * This layouter gets executed before all others. Returns
         * @ref LayouterHandle::Null if there's no layouters. The returned
         * handle is always either valid or null.
         * @see @ref layouterNext(), @ref layouterLast(),
         *      @ref layouterPrevious()
         */
        LayouterHandle layouterFirst() const;

        /**
         * @brief Last layouter in the layout calculation order
         *
         * This layouter gets executed after all others. Returns
         * @ref LayouterHandle::Null if there's no layouters. The returned
         * handle is always either valid or null.
         * @see @ref layouterPrevious(), @ref layouterFirst(),
         *      @ref layouterNext()
         */
        LayouterHandle layouterLast() const;

        /**
         * @brief Previous layouter in the layout calculation order
         *
         * The previous layouter gets executed earlier. Expects that @p handle
         * is valid. Returns @ref LayouterHandle::Null if the layouter is
         * first. The returned handle is always either valid or null.
         * @see @ref isHandleValid(LayouterHandle) const,
         *      @ref layouterNext(), @ref layouterFirst(), @ref layouterLast()
         */
        LayouterHandle layouterPrevious(LayouterHandle handle) const;

        /**
         * @brief Next layouter in the layout calculation order
         *
         * The next layouter gets executed later. Expects that @p handle is
         * valid. Returns @ref LayouterHandle::Null if the layouter is last.
         * The returned handle is always either valid or null.
         * @see @ref isHandleValid(LayouterHandle) const,
         *      @ref layouterPrevious(), @ref layouterLast(),
         *      @ref layouterFirst()
         */
        LayouterHandle layouterNext(LayouterHandle handle) const;

        /**
         * @brief Create a layouter
         * @param before    A layouter to order before for layout calculation
         *      or @ref LayouterHandle::Null if ordered as last. Expected to be
         *      valid if not null.
         * @return New layouter handle
         *
         * Allocates a new handle in a free slot in the internal storage or
         * grows the storage if there's no free slots left. Expects that
         * there's at most 256 layouters. The returned handle is meant to be
         * used to construct an @ref AbstractLayouter subclass and the instance
         * then passed to @ref setLayouterInstance(). A layouter can be removed
         * again with @ref removeLayer().
         * @see @ref isHandleValid(LayouterHandle) const,
         *      @ref layouterCapacity(), @ref layouterUsedCount()
         */
        LayouterHandle createLayouter(LayouterHandle before =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            LayouterHandle::Null
            #else
            LayouterHandle{} /* To not have to include Handle.h */
            #endif
        );

        /**
         * @brief Set a layouter instance
         * @return Reference to @p instance
         *
         * Expects that @p instance was created with a @ref LayouterHandle
         * returned from @ref createLayouter() earlier, the handle is valid and
         * @ref setLayouterInstance() wasn't called for the same handle yet.
         * @see @ref AbstractLayouter::handle(),
         *      @ref isHandleValid(LayouterHandle) const
         */
        AbstractLayouter& setLayouterInstance(Containers::Pointer<AbstractLayouter>&& instance);
        /** @overload */
        template<class T> T& setLayouterInstance(Containers::Pointer<T>&& instance) {
            return static_cast<T&>(setLayouterInstance(Containers::Pointer<AbstractLayouter>{Utility::move(instance)}));
        }

        /**
         * @brief Layouter instance
         *
         * Expects that @p handle is valid and that @ref setLayouterInstance()
         * was called for it.
         * @see @ref isHandleValid(LayouterHandle) const
         */
        AbstractLayouter& layouter(LayouterHandle handle);
        const AbstractLayouter& layouter(LayouterHandle handle) const; /**< @overload */

        /**
         * @brief Layouter instance in a concrete type
         *
         * Expects that @p handle is valid and that @ref setLayouterInstance()
         * was called for it. It's the user responsibility to ensure that @p T
         * matches the actual instance type.
         * @see @ref isHandleValid(LayouterHandle) const
         */
        template<class T> T& layouter(LayouterHandle handle) {
            return static_cast<T&>(layouter(handle));
        }
        /** @overload */
        template<class T> const T& layouter(LayouterHandle handle) const {
            return static_cast<const T&>(layouter(handle));
        }

        /**
         * @brief Remove a layouter
         *
         * Expects that @p handle is valid. After this call,
         * @ref isHandleValid(LayouterHandle) const returns @cpp false @ce for
         * @p handle and @ref isHandleValid(LayoutHandle) const returns
         * @cpp false @ce for all layouts associated with @p handle.
         *
         * Calling this function causes
         * @ref UserInterfaceState::NeedsLayoutAssignmentUpdate to be set.
         * @see @ref clean()
         */
        void removeLayouter(LayouterHandle handle);

        /**
         * @}
         */

        /** @{
         * @name Node management
         */

        /**
         * @brief Current capacity of the node storage
         *
         * Can be at most 1048576. If @ref createNode() is called and there's
         * no free slots left, the internal storage gets grown.
         * @see @ref nodeUsedCount()
         */
        std::size_t nodeCapacity() const;

        /**
         * @brief Count of used items in the node storage
         *
         * Always at most @ref nodeCapacity(). Expired handles are counted
         * among used as well. The operation is done with a
         * @f$ \mathcal{O}(n) @f$ complexity where @f$ n @f$ is
         * @ref nodeCapacity().
         */
        std::size_t nodeUsedCount() const;

        /**
         * @brief Whether a node handle is valid
         *
         * A handle is valid if it has been returned from @ref createNode()
         * before, @ref removeNode() wasn't called on it yet and it wasn't
         * removed inside @ref update() due to a parent node being removed
         * earlier. For @ref NodeHandle::Null always returns @cpp false @ce.
         */
        bool isHandleValid(NodeHandle handle) const;

        /**
         * @brief Create a node
         * @param parent    Parent node to attach to or
         *      @ref NodeHandle::Null for a new root node. Expected to be valid
         *      if not null.
         * @param offset    Offset relative to the parent node
         * @param size      Size of the node contents. Used for layouting,
         *      clipping and event handling.
         * @param flags     Initial node flags
         * @return New node handle
         *
         * Allocates a new handle in a free slot in the internal storage or
         * grows the storage if there's no free slots left. Expects that
         * there's at most 1048576 nodes. The returned handle can be then
         * removed again with @ref removeNode().
         *
         * If @p parent is @ref NodeHandle::Null, the node is added at the
         * back of the draw and event processing list, i.e. drawn last (thus
         * at the front) and reacting to events before all others. Use
         * @ref setNodeOrder() and @ref clearNodeOrder() to adjust the draw
         * and event processing order or remove it from the list of visible
         * top-level nodes.
         *
         * Calling this function causes @ref UserInterfaceState::NeedsNodeUpdate
         * to be set.
         * @see @ref isHandleValid(NodeHandle) const, @ref nodeCapacity(),
         *      @ref nodeUsedCount(), @ref update()
         */
        NodeHandle createNode(NodeHandle parent, const Vector2& offset, const Vector2& size, NodeFlags flags = {});

        /**
         * @brief Create a root node
         *
         * Equivalent to calling @ref createNode(NodeHandle, const Vector2&, const Vector2&, NodeFlags)
         * with @ref NodeHandle::Null as the parent.
         */
        NodeHandle createNode(const Vector2& offset, const Vector2& size, NodeFlags flags = {});

        /**
         * @brief Node parent
         *
         * Expects that @p handle is valid. Returns @ref NodeHandle::Null if
         * it's a root node, in which case @ref nodeOrderPrevious(),
         * @ref nodeOrderNext(), @ref nodeOrderFirst() and @ref nodeOrderLast()
         * can be used to inspect the draw and envent processing order.
         *
         * Note that the returned handle may be invalid if @ref removeNode()
         * was called on one of the parent nodes and @ref update() hasn't
         * been called since.
         *
         * Unlike other node properties, the parent cannot be changed after
         * creation.
         * @see @ref isHandleValid(NodeHandle) const
         */
        NodeHandle nodeParent(NodeHandle handle) const;

        /**
         * @brief Node offset relative to its parent
         *
         * The returned value is before any layout calculation is done. Expects
         * that @p handle is valid.
         * @see @ref isHandleValid(NodeHandle) const, @ref nodeParent()
         */
        Vector2 nodeOffset(NodeHandle handle) const;

        /**
         * @brief Set node offset relative to its parent
         *
         * The value is subsequently used for layout calculation. Expects that
         * @p handle is valid.
         *
         * Calling this function causes
         * @ref UserInterfaceState::NeedsLayoutUpdate to be set.
         * @see @ref isHandleValid(NodeHandle) const, @ref nodeParent()
         */
        void setNodeOffset(NodeHandle handle, const Vector2& offset);

        /**
         * @brief Node size
         *
         * The returned value is before any layout calculation is done. Expects
         * that @p handle is valid.
         * @see @ref isHandleValid(NodeHandle) const
         */
        Vector2 nodeSize(NodeHandle handle) const;

        /**
         * @brief Set node size
         *
         * The value is subsequently used for layout calculation. Expects that
         * @p handle is valid.
         *
         * Calling this function causes
         * @ref UserInterfaceState::NeedsLayoutUpdate to be set.
         * @see @ref isHandleValid(NodeHandle) const
         */
        void setNodeSize(NodeHandle handle, const Vector2& size);

        /**
         * @brief Node flags
         *
         * Expects that @p handle is valid.
         * @see @ref isHandleValid(NodeHandle) const, @ref setNodeFlags(),
         *      @ref addNodeFlags(), @ref clearNodeFlags()
         */
        NodeFlags nodeFlags(NodeHandle handle) const;

        /**
         * @brief Set node flags
         *
         * Expects that @p handle is valid.
         *
         * If @ref NodeFlag::Hidden was added or cleared by calling this
         * function, it causes @ref UserInterfaceState::NeedsNodeUpdate to be
         * set. If @ref NodeFlag::Clip was added or cleared by calling this
         * function, it causes @ref UserInterfaceState::NeedsNodeClipUpdate to
         * be set. If @ref NodeFlag::NoEvents or @ref NodeFlag::Disabled was
         * added or cleared by calling this function, it causes
         * @ref UserInterfaceState::NeedsNodeEnabledUpdate to be set.
         * @see @ref isHandleValid(NodeHandle) const, @ref addNodeFlags(),
         *      @ref clearNodeFlags()
         */
        void setNodeFlags(NodeHandle handle, NodeFlags flags);

        /**
         * @brief Add node flags
         *
         * Calls @ref setNodeFlags() with the existing flags ORed with
         * @p flags. Useful for preserving previously set flags.
         * @see @ref clearNodeFlags()
         */
        void addNodeFlags(NodeHandle handle, NodeFlags flags);

        /**
         * @brief Clear node flags
         *
         * Calls @ref setNodeFlags() with the existing flags ANDed with the
         * inverse of @p flags. Useful for removing a subset of previously set
         * flags.
         * @see @ref addNodeFlags()
         */
        void clearNodeFlags(NodeHandle handle, NodeFlags flags);

        /**
         * @brief Remove a node
         *
         * Expects that @p handle is valid. Nested nodes and data attached to
         * any of the nodes are then removed during the next call to
         * @ref update(). After this call, @ref isHandleValid(NodeHandle) const
         * returns @cpp false @ce for @p handle.
         *
         * Calling this function causes @ref UserInterfaceState::NeedsNodeClean
         * to be set.
         * @see @ref clean()
         */
        void removeNode(NodeHandle handle);

        /**
         * @}
         */

        /** @{
         * @name Top-level node draw and event processing order management
         */

        /**
         * @brief Capacity of the node order storage
         *
         * @ref nodeOrderUsedCount()
         */
        std::size_t nodeOrderCapacity() const;

        /**
         * @brief Count of used items in the node order storage
         *
         * Always at most @ref nodeOrderCapacity(). The operation is done
         * with a @f$ \mathcal{O}(n) @f$ complexity where @f$ n @f$ is
         * @ref nodeOrderCapacity().
         */
        std::size_t nodeOrderUsedCount() const;

        /**
         * @brief First node in draw and event processing order
         *
         * The first node gets drawn first (thus is at the back) and reacts
         * to events after all others. Returns @ref NodeHandle::Null if
         * there's no nodes included in the draw and event processing order.
         * The returned handle is always either valid or null.
         * @see @ref nodeOrderNext(), @ref nodeOrderLast(),
         *      @ref nodeOrderPrevious(), @ref isNodeOrdered()
         */
        NodeHandle nodeOrderFirst() const;

        /**
         * @brief Last node in draw and event processing order
         *
         * The last node gets drawn last (thus is at the front) and reacts to
         * events before all others. Returns @ref NodeHandle::Null if there's
         * no nodes included in the draw and event processing order. The
         * returned handle is always either valid or null.
         * @see @ref nodeOrderPrevious(), @ref nodeOrderFirst(),
         *      @ref nodeOrderNext(), @ref isNodeOrdered()
         */
        NodeHandle nodeOrderLast() const;

        /**
         * @brief Whether a node is included in a draw and event processing order
         *
         * If not included, the node and all its children are not drawn and
         * don't react to events. Expects that @p handle is valid and is a
         * root node.
         * @see @ref isHandleValid(NodeHandle) const, @ref nodeParent(),
         *      @ref nodeOrderNext(), @ref setNodeOrder(),
         *      @ref clearNodeOrder()
         */
        bool isNodeOrdered(NodeHandle handle) const;

        /**
         * @brief Previous node in draw and event processing order
         *
         * The previous node gets drawn earlier (thus is behind) and reacts
         * to events later. Expects that @p handle is valid and is a root node.
         * Returns @ref NodeHandle::Null if the node is either first or not
         * included in the draw and event processing order. The returned handle
         * is always either valid or null.
         * @see @ref isHandleValid(NodeHandle) const, @ref nodeParent(),
         *      @ref nodeOrderNext(), @ref nodeOrderFirst(),
         *      @ref nodeOrderLast(), @ref isNodeOrdered()
         */
        NodeHandle nodeOrderPrevious(NodeHandle handle) const;

        /**
         * @brief Next node in draw and event processing order
         *
         * The next node gets drawn later (thus is in front) and reacts to
         * events earlier. Expects that @p handle is valid and is a root node.
         * Returns @ref NodeHandle::Null if the node is either last or not
         * included in the draw and event processing order. The returned handle
         * is always either valid or null.
         * @see @ref isHandleValid(NodeHandle) const, @ref nodeParent(),
         *      @ref nodeOrderPrevious(), @ref nodeOrderLast(),
         *      @ref nodeOrderFirst(), @ref isNodeOrdered()
         */
        NodeHandle nodeOrderNext(NodeHandle handle) const;

        /**
         * @brief Order a node for draw and event processing
         *
         * The @p handle gets ordered to be drawn earlier (thus behind) and
         * react to event later than @p before. Expects that @p handle is valid
         * and is a root node, and that @p before is either
         * @ref NodeHandle::Null or valid root node that's included in the draw
         * and event processing order. If the node was previously in a
         * different position in the draw and event processing order, it's
         * moved, if it wasn't previously in the draw and event processing
         * order, it's inserted.
         *
         * Calling this function causes @ref UserInterfaceState::NeedsNodeUpdate
         * to be set.
         * @see @ref isHandleValid(NodeHandle) const, @ref nodeParent(),
         *      @ref clearNodeOrder(), @ref update()
         */
        void setNodeOrder(NodeHandle handle, NodeHandle before);

        /**
         * @brief Clear a node from the draw and event processing order
         *
         * Expects that @p handle is valid and is a root node. If the node
         * wasn't previously in the draw and processing order, the function is
         * a no-op.
         *
         * If not a no-op, calling this function causes
         * @ref UserInterfaceState::NeedsNodeUpdate to be set.
         * @see @ref isHandleValid(NodeHandle) const, @ref nodeParent(),
         *      @ref setNodeOrder(), @ref update()
         */
        void clearNodeOrder(NodeHandle handle);

        /**
         * @}
         */

        /**
         * @brief Clean orphaned nodes, data and no longer valid data attachments
         * @return Reference to self (for method chaining)
         *
         * Called implicitly from @ref update() and subsequently also from
         * @ref draw() and all event processing functions. If @ref state()
         * doesn't contain @ref UserInterfaceState::NeedsNodeClean, this
         * function is a no-op, otherwise it performs a subset of the following
         * depending on the state:
         *
         * -    Removes nodes with an invalid (removed) parent node
         * -    Calls @ref AbstractLayer::cleanNodes() with updated node
         *      generations, causing removal of data attached to invalid nodes
         * -    Calls @ref AbstractLayouter::cleanNodes() with updated node
         *      generations, causing removal of layouts assigned to invalid
         *      nodes
         *
         * After calling this function, @ref state() doesn't contain
         * @ref UserInterfaceState::NeedsNodeClean anymore;
         * @ref nodeUsedCount() and @ref AbstractLayer::usedCount() may get
         * smaller.
         */
        AbstractUserInterface& clean();

        /**
         * @brief Update renderer framebuffer sizes
         * @return Reference to self (for method chaining)
         *
         * Called implicitly from @ref update() and subsequently also from
         * @ref draw() and all event processing functions, but is a dedicated
         * API to allow scheduling it separately from regular updates as it's
         * potentially expensive due to framebuffer attachment reallocation.
         *
         * If @ref state() doesn't contain
         * @ref UserInterfaceState::NeedsRendererSizeSetup, this function a
         * no-op, otherwise it calls @ref AbstractRenderer::setupFramebuffers()
         * with current framebuffer size.
         *
         * After calling this function, @ref state() doesn't contain
         * @ref UserInterfaceState::NeedsRendererSizeSetup anymore.
         */
        AbstractUserInterface& updateRenderer();

        /**
         * @brief Update node hierarchy, data order and data contents for drawing and event processing
         * @return Reference to self (for method chaining)
         *
         * Expects that either @ref setSize() was called or the
         * @ref AbstractUserInterface(const Vector2&, const Vector2&, const Vector2i&)
         * constructor was used.
         *
         * Implicitly calls @ref clean() and @ref updateRenderer(); called
         * implicitly from @ref draw() and all event processing functions. If
         * @ref state() contains none of
         * @ref UserInterfaceState::NeedsDataUpdate,
         * @ref UserInterfaceState::NeedsDataAttachmentUpdate,
         * @ref UserInterfaceState::NeedsNodeEnabledUpdate,
         * @ref UserInterfaceState::NeedsNodeClipUpdate,
         * @ref UserInterfaceState::NeedsLayoutUpdate,
         * @ref UserInterfaceState::NeedsLayoutAssignmentUpdate or
         * @ref UserInterfaceState::NeedsNodeUpdate, this function is a no-op,
         * otherwise it performs a subset of the following depending on the
         * state:
         *
         * -    Orders visible nodes back-to-front for drawing and
         *      front-to-back for event processing
         * -    Orders layouts assigned to nodes by their dependency
         * -    Performs layout calculation
         * -    Calculates absolute offsets for visible nodes
         * -    Culls invisible nodes, calculates clip rectangles
         * -    Propagates @ref NodeFlag::Disabled and @ref NodeFlag::NoEvents
         *      to child nodes
         * -    Orders data attachments in each layer by draw order
         * -    Calls @ref AbstractLayer::update() with the ordered data
         * -    Resets @ref pointerEventPressedNode(),
         *      @ref pointerEventCapturedNode() or
         *      @ref pointerEventHoveredNode() if the nodes no longer exist,
         *      are not visible or have @ref NodeFlag::NoEvents or
         *      @ref NodeFlag::Disabled set on them or their parents
         *
         * After calling this function, @ref state() is empty.
         */
        AbstractUserInterface& update();

        /**
         * @brief Draw the user interface
         * @return Reference to self (for method chaining)
         *
         * Implicitly calls @ref update(), which in turn implicitly calls
         * @ref clean() and @ref updateRenderer(). Performs the following:
         *
         * -    Calls @ref AbstractRenderer::transition() with
         *      @ref RendererTargetState::Initial
         * -    Peforms draw calls by going through draws collected by
         *      @ref update() for each top level node and all its visible
         *      children in a back to front order, and then for each layer that
         *      supports @ref LayerFeature::Draw in a back to front order:
         *      -   Calls @ref AbstractRenderer::transition() with
         *          @ref RendererTargetState::Draw and appropriate
         *          @ref RendererDrawStates based on whether given layer
         *          advertises @ref LayerFeature::DrawUsesBlending or
         *          @relativeref{LayerFeature,DrawUsesScissor}
         *      -   Calls @ref AbstractLayer::draw()
         * -    Calls @ref AbstractRenderer::transition() with
         *      @ref RendererTargetState::Final
         */
        AbstractUserInterface& draw();

        /**
         * @brief Handle a pointer press event
         *
         * Implicitly calls @ref update(), which in turn implicitly calls
         * @ref clean() and @ref updateRenderer(). The @p globalPosition is
         * assumed to be in respect to @ref windowSize(), and is internally
         * scaled to match @ref size() before being set to @ref PointerEvent.
         *
         * Finds the front-most node under (scaled) @p globalPosition and
         * calls @ref AbstractLayer::pointerPressEvent() on all data attached
         * to it belonging to layers that support @ref LayerFeature::Event. If
         * no data accept the event, continues to other nodes under the
         * position in a front-to-back order and then to parent nodes. For each
         * such node, the event is always called on all attached data,
         * regardless of the accept status. For each call the event position is
         * made relative to the node to which given data is attached. Returns
         * @cpp true @ce if the event was accepted by at least one data,
         * @cpp false @ce if it wasn't or there wasn't any visible event
         * handling node at given position and thus the event should be
         * propagated further.
         *
         * The node that accepted the event is remembered and is subsequently
         * used by @ref pointerReleaseEvent() to emit a
         * @ref AbstractLayer::pointerTapOrClickEvent(), if the release happens
         * on it as well. The node that accepted the event also implicitly
         * captures all further pointer events until and including a
         * @ref pointerReleaseEvent() even if they happen outside of its area,
         * unless @ref PointerEvent::setCaptured() is called by the
         * implementation to disable this behavior. The capture can be also
         * removed from a later @ref pointerMoveEvent(). Any node that was
         * already captured when calling this function is ignored.
         *
         * Expects that the event is not accepted yet.
         * @see @ref PointerEvent::isAccepted(),
         *      @ref PointerEvent::setAccepted(),
         *      @ref pointerEventPressedNode(), @ref pointerEventCapturedNode()
         */
        bool pointerPressEvent(const Vector2& globalPosition, PointerEvent& event);

        /**
         * @brief Handle a pointer release event
         *
         * Implicitly calls @ref update(), which in turn implicitly calls
         * @ref clean() and @ref updateRenderer(). The @p globalPosition is
         * assumed to be in respect to @ref windowSize(), and is internally
         * scaled to match @ref size() before being set to @ref PointerEvent.
         *
         * If a node was captured by a previous @ref pointerPressEvent() or
         * @ref pointerMoveEvent(), @ref pointerReleaseEvent() wasn't called
         * yet and the node wasn't removed since, calls
         * @ref AbstractLayer::pointerReleaseEvent() on all data attached to
         * that node even if it happens outside of its area, with the event
         * position made relative to the node. Returns @cpp true @ce if the
         * event was accepted by at least one data attached to the captured
         * node, @cpp false @ce if it wasn't and thus the event should be
         * propagated further. The capture is implicitly released after
         * calling this function independently of whether
         * @ref PointerEvent::setCaptured() was set by the implementation.
         *
         * Otherwise, if a node wasn't captured, finds the front-most node
         * under (scaled) @p globalPosition and calls
         * @ref AbstractLayer::pointerReleaseEvent() on all data attached to it
         * belonging to layers that support @ref LayerFeature::Event. If no
         * data accept the event, continues to other nodes under the position
         * in a front-to-back order and then to parent nodes. For each such
         * node, the event is always called on all attached data, regardless of
         * the accept status. For each call the event position is made relative
         * to the node to which given data is attached. Returns @cpp true @ce
         * if the event was accepted by at least one data, @cpp false @ce if it
         * wasn't or there wasn't any visible event handling node at given
         * position and thus the event should be propagated further.
         *
         * If the node that accepted the event is the same as the node on which
         * a previous @ref pointerPressEvent() happened and the pointer was
         * either captured or didn't leave the node area since, calls also
         * @ref AbstractLayer::pointerTapOrClickEvent() on all data attached to
         * it.
         *
         * Expects that the event is not accepted yet.
         * @see @ref PointerEvent::isAccepted(),
         *      @ref PointerEvent::setAccepted(),
         *      @ref pointerEventPressedNode(), @ref pointerEventCapturedNode()
         */
        bool pointerReleaseEvent(const Vector2& globalPosition, PointerEvent& event);

        /**
         * @brief Handle a pointer move event
         *
         * Implicitly calls @ref update(), which in turn implicitly calls
         * @ref clean() and @ref updateRenderer(). The @p globalPosition is
         * assumed to be in respect to @ref windowSize(), and is internally
         * scaled to match @ref size() before being set to @ref PointerEvent.
         *
         * If a node was captured by a previous @ref pointerPressEvent() or
         * @ref pointerMoveEvent(), @ref pointerReleaseEvent() wasn't called
         * yet and the node wasn't removed since, calls
         * @ref AbstractLayer::pointerMoveEvent() on all data attached to that
         * node even if it happens outside of its area, with the event position
         * made relative to the node. If the move event happened inside the
         * node area and is accepted by at least one data, the node is treated
         * as hovered, otherwise as not hovered. An
         * @ref AbstractLayer::pointerEnterEvent() or
         * @relativeref{AbstractLayer,pointerLeaveEvent()} is then called for
         * all data attached to the captured node if the node hover status
         * changed with the same @p event except for
         * @ref PointerMoveEvent::relativePosition() which is reset to a zero
         * vector. No corresponding leave / enter event is called for any other
         * node in this case. Returns @cpp true @ce if the move event was
         * accepted by at least one data attached to the captured node,
         * @cpp false @ce if it wasn't and thus the event should be propagated
         * further; accept status of the enter and leave events is ignored. If
         * the move, enter or leave event implementations called
         * @ref PointerMoveEvent::setCaptured() resulting in it being
         * @cpp false @ce regardless of their accept status, the capture is
         * released after this function, otherwise it stays unchanged.
         *
         * Otherwise, if a node wasn't captured, finds the front-most node
         * under (scaled) @p globalPosition and calls
         * @ref AbstractLayer::pointerMoveEvent() on all data attached to it
         * belonging to layers that support @ref LayerFeature::Event. If
         * no data accept the event, continues to other nodes under the
         * position in a front-to-back order and then to parent nodes. For each
         * such node, the event is always called on all attached data,
         * regardless of the accept status. The node on which the data accepted
         * the event is then treated as hovered; if no data accepted the event,
         * there's no hovered node. For each call the event position is made
         * relative to the node to which given data is attached. If the
         * currently hovered node changed, an
         * @ref AbstractLayer::pointerLeaveEvent() is then called for all data
         * attached to a previously hovered node if it exists, and then a
         * corresponding @ref AbstractLayer::pointerEnterEvent() is called for
         * all data attached to the currently hovered node if it exists, in
         * both cases with the same @p event except for
         * @ref PointerMoveEvent::position() which is made relative to the
         * particular node it's called on and
         * @ref PointerMoveEvent::relativePosition() which is reset to a zero
         * vector. Returns @cpp true @ce if the event was accepted by at least
         * one data, @cpp false @ce if it wasn't or there wasn't any visible
         * event handling node at given position and thus the event should be
         * propagated further; accept status of the enter and leave events is
         * ignored. If any accepted move event or any enter event called
         * @ref PointerMoveEvent::setCaptured() resulting in it being
         * @cpp true @ce, the containing node implicitly captures all further
         * pointer events until and including a @ref pointerReleaseEvent() even
         * if they happen outside of its area, or until the capture is released
         * in a @ref pointerMoveEvent() again. Calling
         * @ref PointerMoveEvent::setCaptured() in the leave event has no
         * effect in this case.
         *
         * Expects that the event is not accepted yet.
         * @see @ref PointerEvent::isAccepted(),
         *      @ref PointerEvent::setAccepted(),
         *      @ref pointerEventCapturedNode(),
         *      @ref pointerEventHoveredNode()
         */
        bool pointerMoveEvent(const Vector2& globalPosition, PointerMoveEvent& event);

        /**
         * @brief Node pressed by last pointer event
         *
         * Returns handle of a node that was under the pointer for the last
         * @ref pointerPressEvent(), the pointer wasn't released since and the
         * pointer is either captured on that node or didn't leave its area
         * since. If a @ref pointerReleaseEvent() then happens on the node
         * area, all data attached to it will receive a
         * @ref AbstractLayer::pointerTapOrClickEvent().
         *
         * If no pointer press event was called yet, if the event wasn't
         * accepted by any data, if @ref pointerReleaseEvent() was called since
         * or the pointer was uncaptured and left the node area, returns
         * @ref NodeHandle::Null. It also becomes @ref NodeHandle::Null if the
         * node or any of its parents were removed, hidden or have
         * @ref NodeFlag::NoEvents or @ref NodeFlag::Disabled set and
         * @ref update() was called since.
         *
         * The returned handle may be invalid if the node or any of its parents
         * were removed and @ref clean() wasn't called since.
         */
        NodeHandle pointerEventPressedNode() const;

        /**
         * @brief Node captured by last pointer event
         *
         * Returns handle of a node that captured the last
         * @ref pointerPressEvent() or @ref pointerMoveEvent(). All data
         * attached to the captured node then receive all following pointer
         * events until and including a @ref pointerReleaseEvent() even if they
         * happen outside of its area, or until the capture is released in a
         * @ref pointerMoveEvent() again.
         *
         * If no pointer press event was called yet, if the event wasn't
         * accepted by any data, if the capture was disabled or subsequently
         * released with @ref PointerEvent::setCaptured() or if a
         * @ref pointerReleaseEvent() was called since, returns
         * @ref NodeHandle::Null. It also becomes @ref NodeHandle::Null if the
         * node or any of its parents were removed, hidden or have
         * @ref NodeFlag::NoEvents or @ref NodeFlag::Disabled set and
         * @ref update() was called since.
         *
         * The returned handle may be invalid if the node or any of its parents
         * were removed and @ref clean() wasn't called since.
         */
        NodeHandle pointerEventCapturedNode() const;

        /**
         * @brief Node hovered by last pointer event
         *
         * Returns handle of a node that was under the pointer for the last
         * @ref pointerMoveEvent(). All data attached to such node already
         * received a @ref AbstractLayer::pointerEnterEvent(). Once a
         * @ref pointerMoveEvent() leaves its area, all data attached to that
         * node will receive a @ref AbstractLayer::pointerLeaveEvent(), and
         * this either becomes @ref NodeHandle::Null, or another node becomes
         * hovered, receiving an enter event. It's also @ref NodeHandle::Null
         * if no pointer move event was called yet or if the node or any of its
         * parents were removed, hidden or have @ref NodeFlag::NoEvents or
         * @ref NodeFlag::Disabled set and @ref update() was called since.
         *
         * The returned handle may be invalid if the node or any of its parents
         * were removed and @ref clean() wasn't called since.
         */
        NodeHandle pointerEventHoveredNode() const;

    private:
        /* Used by removeNode() and clean() */
        MAGNUM_WHEE_LOCAL void removeNodeInternal(UnsignedInt id);
        /* Used by setNodeFlags(), addNodeFlags() and clearNodeFlags() */
        MAGNUM_WHEE_LOCAL void setNodeFlagsInternal(UnsignedInt id, NodeFlags flags);
        /* Used by setNodeOrder() and clearNodeOrder() */
        MAGNUM_WHEE_LOCAL void clearNodeOrderInternal(NodeHandle handle);
        /* Used by *Event() functions */
        template<class Event, void(AbstractLayer::*function)(UnsignedInt, Event&)> MAGNUM_WHEE_LOCAL bool callEventOnNode(const Vector2& globalPositionScaled, UnsignedInt nodeId, Event& event, bool rememberCaptureOnUnaccepted = false);
        template<class Event, void(AbstractLayer::*function)(UnsignedInt, Event&)> MAGNUM_WHEE_LOCAL NodeHandle callEvent(const Vector2& globalPositionScaled, UnsignedInt visibleNodeIndex, Event& event);
        template<class Event, void(AbstractLayer::*function)(UnsignedInt, Event&)> MAGNUM_WHEE_LOCAL NodeHandle callEvent(const Vector2& globalPositionScaled, Event& event);

        struct State;
        Containers::Pointer<State> _state;
};

}}

#endif
