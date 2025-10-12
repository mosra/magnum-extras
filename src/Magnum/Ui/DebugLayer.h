#ifndef Magnum_Ui_DebugLayer_h
#define Magnum_Ui_DebugLayer_h
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

/**
 * @brief Class @ref Magnum::Ui::DebugLayer, enum @ref Magnum::Ui::DebugLayerSource, @ref Magnum::Ui::DebugLayerFlag, enum set @ref Magnum::Ui::DebugLayerSources, @ref Magnum::Ui::DebugLayerFlags
 * @m_since_latest
 */

#include "Magnum/Ui/AbstractLayer.h"

namespace Magnum { namespace Ui {

/**
@brief Debug layer data source to track
@m_since_latest

@see @ref DebugLayerSources, @ref DebugLayer, @ref DebugLayer::sources()
*/
enum class DebugLayerSource: UnsignedShort {
    /**
     * Track created nodes and make it possible to name them using
     * @ref DebugLayer::setNodeName(). Subset of
     * @ref DebugLayerSource::NodeHierarchy and
     * @ref DebugLayerSource::NodeData.
     */
    Nodes = 1 << 0,

    /**
     * Track created layers and make it possible to name them using
     * @ref DebugLayer::setLayerName(). Subset of
     * @ref DebugLayerSource::NodeData.
     */
    Layers = 1 << 1,

    /**
     * Track created layouters and make it possible to name them using
     * @ref DebugLayer::setLayouterName(). Subset of
     * @ref DebugLayerSource::NodeLayouts.
     */
    Layouters = 1 << 2,

    /**
     * Track created animators and make it possible to name them using
     * @ref DebugLayer::setAnimatorName(). Subset of
     * @ref DebugLayerSource::NodeAnimations.
     */
    Animators = 1 << 3,

    /**
     * Track node offsets and sizes. Implies
     * @ref DebugLayerSource::Nodes.
     *
     * Note that this shows only the raw offset and size properties relative to
     * the parent node and before any layouters are run. Layout properties can
     * be shown using using @ref DebugLayerSource::NodeLayoutDetails.
     */
    NodeOffsetSize = Nodes|(1 << 4),

    /**
     * Track node parent and child relations. Implies
     * @ref DebugLayerSource::Nodes.
     */
    NodeHierarchy = Nodes|(1 << 5),

    /**
     * Track per-node layer data attachments. Implies
     * @ref DebugLayerSource::Nodes and @ref DebugLayerSource::Layers, subset
     * of @ref DebugLayerSource::NodeDataDetails.
     */
    NodeData = Nodes|Layers|(1 << 6),

    /**
     * Track per-node layer data attachments with per-data details provided by
     * layer-specific debug integrations as described in
     * @ref Ui-DebugLayer-node-inspect-node-details. Implies
     * @ref DebugLayerSource::NodeData.
     */
    NodeDataDetails = NodeData|(1 << 7),

    /**
     * Track per-node layout assignments. Implies @ref DebugLayerSource::Nodes
     * and @ref DebugLayerSource::Layouters, subset of
     * @ref DebugLayerSource::NodeLayoutDetails.
     */
    NodeLayouts = Nodes|Layouters|(1 << 8),

    /**
     * Track per-node layout assignments with per-layout details provided by
     * layouter-specific debug integrations as described in
     * @ref Ui-DebugLayer-node-inspect-node-details. Implies
     * @ref DebugLayerSource::NodeLayouts.
     */
    NodeLayoutDetails = NodeLayouts|(1 << 9),

    /**
     * Track per-node animation attachments. Implies
     * @ref DebugLayerSource::Nodes and @ref DebugLayerSource::Animators,
     * subset of @ref DebugLayerSource::NodeAnimationDetails.
     */
    NodeAnimations = Nodes|Animators|(1 << 10),

    /**
     * Track per-node layer data attachments with per-data details provided by
     * animator-specific debug integrations as described in
     * @ref Ui-DebugLayer-node-inspect-node-details. Implies
     * @ref DebugLayerSource::NodeAnimations.
     */
    NodeAnimationDetails = NodeAnimations|(1 << 11),
};

/**
@brief Debug layer data sources to track
@m_since_latest

@see @ref DebugLayer, @ref DebugLayer::sources()
*/
typedef Containers::EnumSet<DebugLayerSource> DebugLayerSources;

CORRADE_ENUMSET_OPERATORS(DebugLayerSources)

/**
@debugoperatorenum{DebugLayerSource}
@m_since_latest
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, DebugLayerSource value);

/**
@debugoperatorenum{DebugLayerSources}
@m_since_latest
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, DebugLayerSources value);

/**
@brief Debug layer flag
@m_since_latest

@see @ref DebugLayerFlags, @ref DebugLayer, @ref DebugLayer::flags()
*/
enum class DebugLayerFlag: UnsignedByte {
    /**
     * Highlight and show details of a node on pointer press. Expects that at
     * least @ref DebugLayerSource::Nodes is enabled.
     * @see @ref DebugLayer::setNodeInspectColor(),
     *      @ref DebugLayer::setNodeInspectGesture(),
     *      @ref DebugLayer::setNodeInspectCallback()
     */
    NodeInspect = 1 << 0,

    /**
     * Print all messages without color, even if
     * @relativeref{Corrade::Utility,Debug::isTty()} returns @cpp true @ce. By
     * default, messages are printed colored if the output is detected to be a
     * TTY, and strings passed to the callback specified in
     * @ref DebugLayer::setNodeInspectCallback() are without color. If
     * @ref DebugLayerFlag::ColorAlways is specified as well, this flag has a
     * priority.
     */
    ColorOff = 1 << 1,

    /**
     * Print all messages with color, even if
     * @relativeref{Corrade::Utility,Debug::isTty()} returns @cpp false @ce and
     * even in case the message is passed to the callback specified in
     * @ref DebugLayer::setNodeInspectCallback(). If
     * @ref DebugLayerFlag::ColorOff is specified as well, it has a priority.
     */
    ColorAlways = 1 << 2,
};

/**
@brief Debug layer flags
@m_since_latest

@see @ref DebugLayer, @ref DebugLayer::flags()
*/
typedef Containers::EnumSet<DebugLayerFlag> DebugLayerFlags;

CORRADE_ENUMSET_OPERATORS(DebugLayerFlags)

/**
@debugoperatorenum{DebugLayerFlag}
@m_since_latest
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, DebugLayerFlag value);

/**
@debugoperatorenum{DebugLayerFlags}
@m_since_latest
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, DebugLayerFlags value);

/**
@brief Debug layer
@m_since_latest

Provides a non-intrusive and extensible way to inspect node hierarchy and layer
data attachments in any existing UI for debugging purposes. You can use either
the @ref DebugLayer base for inspection alone, or the @ref DebugLayerGL
subclass that provides also a way to visualize highlighted nodes directly in
the inspected UI.

@m_class{m-note m-success}

@par
    For a live example, full functionality of this layer including inspection
    of all builtin layers and styles is exposed by the `--debug` option of the
    @ref magnum-ui-gallery "magnum-ui-gallery" utility.

@section Ui-DebugLayer-setup Setting up a debug layer instance

A debug layer, either @ref DebugLayer or @ref DebugLayerGL, is constructed
using a fresh @ref AbstractUserInterface::createLayer() handle, a combination
of @ref DebugLayerSource values that the debug layer should track and a
@ref DebugLayerFlag combination describing the actual behavior; and is
subsequently passed to @ref AbstractUserInterface::setLayerInstance(). For
correct behavior it should be added as the very last layer so it's drawn on top
of all other layers and reacts to events first.

@snippet Ui-gl.cpp DebugLayer-setup

With this, assuming @ref AbstractUserInterface::draw() is called in an
appropriate place, the layer is ready to use. You likely don't need to keep a
reference to it as it will track changes in enabled sources without further
involvement. In case of the base @ref DebugLayer that doesn't draw, or when
inspecting the UI programmatically, calling just
@ref AbstractUserInterface::update() (which is otherwise called from
@relativeref{AbstractUserInterface,draw()}) is enough to make it aware of
latest state changes.

@m_class{m-note m-warning}

@par Performance implications
    Depending on what all @ref DebugLayerSource bits are enabled, the layer may
    add extra node data attachments and perform various operations on every
    update. While it isn't expected to have significant effects on performance,
    the @ref DebugLayer is meant primarily for debugging purposes and enabling
    it unconditionally in applications deployed to end users isn't recommended.
    See the @ref Ui-DebugLayer-opt-in section for more information.

@section Ui-DebugLayer-node-inspect Inspecting nodes

The setup shown above, in particular with @ref DebugLayerFlag::NodeInspect
together with at least @ref DebugLayerSource::Nodes enabled, makes it possible
to highlight any node in the hierarchy and see its details.
@relativeref{DebugLayerSource,NodeOffsetSize} lists raw node offset and size
before any layouters are run, @relativeref{DebugLayerSource,NodeHierarchy}
additionally shows info about parent and child nodes and
@relativeref{DebugLayerSource,NodeData} also lists data attachments. Let's say
we have a @ref Ui::Button placed somewhere in the UI, reacting to a tap or
click:

@snippet ui-debuglayer.cpp button

@image html ui-debuglayer-node.png width=128px

With the @ref DebugLayer set up, clicking on this button with
@m_class{m-label m-warning} **Ctrl**
@m_class{m-label m-default} **right mouse button** (or
@m_class{m-label m-warning} **Ctrl** @m_class{m-label m-default} **pen eraser**
in case of a pen input) highlights the node, showing a magenta rectangle over,
and prints details about it to the console like shown below. Clicking on any
other node will inspect that one instead, clicking again on the highlighted
node will remove the highlight.

@image html ui-debuglayer-node-inspect.png width=128px

@include ui-debuglayer-node-inspect.ansi

With @ref DebugLayerSource::NodeLayouts and
@relativeref{DebugLayerSource,NodeAnimations} you can list also layouts and
animations attached to particular nodes. The @ref Ui::Button doesn't have any
by default.

@subsection Ui-DebugLayer-node-inspect-naming Naming nodes, layers, layouters and animators

In the details we can see that the node is placed somewhere and it has four
data attachments. Because the widget is simple we can assume it's the
background, the icon, the text and the event handler. It would be better if the
layer could tell us that, but because naming various resources isn't essential
to UI functionality, and because the @ref DebugLayer is designed to work with
any custom user interface containing any custom layers, not just the builtin
ones, it can't have any knowledge about layer names on its own.

We have to supply those with @ref setLayerName(). In this case we'll name all
layers exposed on the @ref UserInterface instance, you can do the same for any
custom layer as well. Similarly, @ref setNodeName() allows to assign names to
particular nodes.

@snippet ui-debuglayer.cpp button-names

Inspecting the same node then groups the data by layer, showing them in the
order they're drawn. Besides the names now being listed in the printed details,
you can query them back with @ref layerName() and @ref nodeName().

@include ui-debuglayer-node-inspect-names.ansi

Similarly to layers, @ref setLayouterName() and @ref setAnimatorName() can name
layouters and animators.

@subsection Ui-DebugLayer-node-inspect-node-details Showing details about node data, layouts and animations

Enabling @ref DebugLayerSource::NodeDataDetails in addition to
@relativeref{DebugLayerSource,NodeData} makes use of debug integration
implemented by a particular layer. In case of @ref BaseLayer, @ref TextLayer
and other layers derived from @ref AbstractVisualLayer this makes the output
show also style assignment as well as any style transitions, if present. In
case of @ref EventLayer it shows the event that's being handled:

@include ui-debuglayer-node-inspect-details.ansi

The way this works is that by passing a concrete layer type, the
@ref setLayerName(const T&, const Containers::StringView&) overload gets picked
if the type contains an @ref AbstractLayer::DebugIntegration inner class.
Instance of which then gets used to print additional details. The integration
can take various optional parameters, such as a
@ref Ui-AbstractVisualLayer-debug-integration "function to provide style names"
in case of visual layers. Documentation of all builtin layers has information
about what's provided in their debug integration. It's also possible to
implement this integration for custom layers, see @ref Ui-DebugLayer-integration
below for details.

With @ref DebugLayerSource::NodeLayoutDetails, a similar functionality is
available for layouters that implement a @ref AbstractLayouter::DebugIntegration
inner class. Then, @ref DebugLayerSource::NodeAnimationDetails enables this for
for animators that implement a @ref AbstractAnimator::DebugIntegration inner
class, which is the case for example with @ref NodeAnimator.

@subsection Ui-DebugLayer-node-inspect-options Node inspect options

Node inspect has defaults chosen in a way that makes the highlight clearly
visible on most backgrounds, and with the pointer gesture unique enough to not
conflict with usual event handlers. You can change both with
@ref setNodeInspectColor() and @ref setNodeInspectGesture().

The default set of accepted gestures *does not* include touch input however, as
on a touch device it usually isn't possible to press any modifier keys to
distinguish a "debug tap" from a regular tap. It's however possible to use
@ref addFlags() and @ref clearFlags() to enable
@ref DebugLayerFlag::NodeInspect only temporarily and during that time accept
touches without any modifiers, for example in a response to some action in the
UI that enables some sort of a debug mode:

@snippet Ui.cpp DebugLayer-node-inspect-touch

Besides visual inspection using a pointer, @ref inspectNode() allows to
inspect a node programmatically.

Finally, while the inspected node details are by default printed to
@relativeref{Magnum,Debug}, a console might not be always accessible. In that
case you can direct the message to a callback using
@ref setNodeInspectCallback(), and populate for example a @ref Label directly
in the UI with it instead. The callback also gets called with an empty string
when the highlight is removed, which can be used to hide the label again. For
example:

@snippet Ui.cpp DebugLayer-node-inspect-callback

@subsection Ui-DebugLayer-node-inspect-limitations Limitations

Currently, it's only possible to visually inspect nodes that are visible and
are neither @ref NodeFlag::Disabled nor @ref NodeFlag::NoEvents, To help with
their discovery a bit at least, clicking their (event-accepting) parent will
list how many children are hidden, disabled or not accepting events.
Inspecting such nodes is only possible by passing their handle to
@ref inspectNode().

Additionally, if a top-level node covers other nodes but is otherwise invisible
and doesn't react to events in any way, with @ref DebugLayerFlag::NodeInspect
it will become inspectable and it won't be possible to inspect any nodes
underneath. Presence of such a node in the UI is usually accidental, currently
the workaround is to either restrict its size to cover only the necessary area,
move it behind other nodes in the @ref Ui-AbstractUserInterface-nodes-order "top-level node hierarchy"
or mark it with @ref NodeFlag::NoEvents if it doesn't have children that need
to react to events.

@section Ui-DebugLayer-opt-in Making the DebugLayer opt-in

As mentioned above, having the layer always present may have unintended
performance implications, and a node highlight can be confusing if triggered
accidentally by an unsuspecting user. One possibility is to create it only with
a certain startup option, such as is the case with `--debug` in
@ref magnum-ui-gallery "magnum-ui-gallery".

Another way is to create it with no @ref DebugLayerFlag present, and enable
them temporarily only if some debug mode is activated, similarly to
@ref Ui-DebugLayer-node-inspect-options "what was shown for touch input above".
Such setup will however still make it track all enabled @ref DebugLayerSource
bits all the time. To avoid this, it's also possible to defer the layer
creation and setup to the point when it's actually needed, and then destroy it
again after. Note that, as certain options involve iterating the whole node
tree, with very complex UIs such on-the-fly setup might cause stalls.

@section Ui-DebugLayer-integration DebugLayer integration for custom layers, layouters and animators

To make a custom layer provide detailed info for
@ref DebugLayerSource::NodeDataDetails, implement an inner type named
@relativeref{AbstractLayer,DebugIntegration} containing at least a
@relativeref{DebugIntegration,print()} function. In the following snippet, a
layer that exposes per-data color has the color printed in the
@ref DebugLayerFlag::NodeInspect output. To make the output fit with the
other text, it's expected to be indented and end with a newline:

@snippet ui-debuglayer.cpp integration

Assuming the concrete layer type is passed to @ref setLayerName(), nothing
else needs to be done and the integration gets used automatically. If there's
multiple data attached to the same node, the @relativeref{DebugIntegration,print()}
gets called for each of them.

@snippet ui-debuglayer.cpp integration-setLayerName

@include ui-debuglayer-integration.ansi

If the integration needs additional state, a constructor can be implemented,
and the instance then passed to
@ref DebugLayer::setLayerName(const T&, const Containers::StringView&, const typename T::DebugIntegration&)
or @ref DebugLayer::setLayerName(const T&, const Containers::StringView&, typename T::DebugIntegration&&) "setLayerName(..., typename T::DebugIntegration&&)":

@snippet Ui.cpp DebugLayer-integration-constructor

If additional details can be provided even without supplying additional state,
it's recommended to have the class default-constructible as well so at least
some functionality can be used even if users aren't aware of the extra options.
Additionally, in the above code the constructor isn't made @cpp explicit @ce,
which allows the debug integration arguments to be passed directly to
@ref setLayerName():

@snippet Ui.cpp DebugLayer-integration-constructor-setLayerName

For layouters and @ref DebugLayerSource::NodeLayoutDetails the workflow is
similar, just with a
@ref AbstractLayouter::DebugIntegration::print() "slightly different signature for the print function". Same goes for
@ref DebugLayerSource::NodeAnimationDetails, which also has
@ref AbstractAnimator::DebugIntegration::print() "its own signature".

@subsection Ui-DebugLayer-integration-override Overriding the integration in subclasses

Any layer, layouter or animator derived from a base that already has an
integration, such as from @ref AbstractVisualLayer or @ref EventLayer, will
implicitly inherit the base implementation. If the subclass wants to provide
its own output, it can either provide a custom type and hide the original, or
derive from it by calling the base @relativeref{DebugIntegration,print()} in
its own implementation. Here the `ColorLayer` from above is made to derive from
@ref AbstractVisualLayer now instead, and its `DebugIntegration` derives from
@ref AbstractVisualLayer::DebugIntegration as well and inherits its output:

@snippet Ui.cpp DebugLayer-integration-subclass
*/
class MAGNUM_UI_EXPORT DebugLayer: public AbstractLayer {
    public:
        /**
         * @brief Constructor
         * @param handle    Layer handle returned from
         *      @ref AbstractUserInterface::createLayer()
         * @param sources   Data sources to track
         * @param flags     Behavior flags
         *
         * See particular @ref DebugLayerFlag values for information about
         * which @ref DebugLayerSource is expected to be enabled for a
         * particular feature. While @p sources have to specified upfront, the
         * @p flags can be subsequently modified using @ref setFlags(),
         * @ref addFlags() and @ref clearFlags().
         *
         * Note that you can also construct the @ref DebugLayerGL subclass
         * instead to have the layer with visual feedback.
         */
        explicit DebugLayer(LayerHandle handle, DebugLayerSources sources, DebugLayerFlags flags);

        /** @brief Copying is not allowed */
        DebugLayer(const DebugLayer&) = delete;

        /**
         * @brief Move constructor
         *
         * Performs a destructive move, i.e. the original object isn't usable
         * afterwards anymore.
         */
        DebugLayer(DebugLayer&&) noexcept;

        ~DebugLayer();

        /** @brief Copying is not allowed */
        DebugLayer& operator=(const DebugLayer&) = delete;

        /** @brief Move assignment */
        DebugLayer& operator=(DebugLayer&&) noexcept;

        /** @brief Tracked data sources */
        DebugLayerSources sources() const;

        /** @brief Behavior flags */
        DebugLayerFlags flags() const;

        /**
         * @brief Set behavior flags
         * @return Reference to self (for method chaining)
         *
         * See particular @ref DebugLayerFlag values for information about
         * which @ref DebugLayerSource is expected to be enabled for a
         * particular feature.
         *
         * If a node was inspect and @ref DebugLayerFlag::NodeInspect was
         * cleared by calling this function, the highlight gets removed.  The
         * function doesn't print anything, but if a callback is set, it's
         * called with an empty string. Additionally, if the layer is
         * instantiated as @ref DebugLayerGL, it causes
         * @ref LayerState::NeedsDataUpdate to be set.
         * @see @ref addFlags(), @ref clearFlags()
         */
        DebugLayer& setFlags(DebugLayerFlags flags);

        /**
         * @brief Add behavior flags
         * @return Reference to self (for method chaining)
         *
         * Calls @ref setFlags() with the existing flags ORed with @p flags.
         * Useful for preserving previously set flags.
         * @see @ref clearFlags()
         */
        DebugLayer& addFlags(DebugLayerFlags flags);

        /**
         * @brief Clear flags
         * @return Reference to self (for method chaining)
         *
         * Calls @ref setFlags() with the existing flags ANDed with the inverse
         * of @p flags. Useful for removing a subset of previously set flags.
         * @see @ref addFlags()
         */
        DebugLayer& clearFlags(DebugLayerFlags flags);

        /**
         * @brief Node name
         *
         * Expects that the debug layer has been already passed to
         * @ref AbstractUserInterface::setLayerInstance() and that @p handle
         * isn't @ref NodeHandle::Null, @p handle doesn't have to be valid
         * however. If @ref DebugLayerSource::Nodes isn't enabled or @p handle
         * isn't known, returns an empty string.
         *
         * If not empty, the returned string is always
         * @relativeref{Corrade,Containers::StringViewFlag::NullTerminated}.
         * Unless @ref setLayerName() was called with a
         * @relativeref{Corrade,Containers::StringViewFlag::Global} string, the
         * returned view is only guaranteed to be valid until the next call to
         * @ref setLayerName() or until the set of layers in the user interface
         * changes.
         */
        Containers::StringView nodeName(NodeHandle handle) const;

        /**
         * @brief Set node name
         * @return Reference to self (for method chaining)
         *
         * Expects that the debug layer has been already passed to
         * @ref AbstractUserInterface::setLayerInstance() and that @p handle
         * isn't @ref NodeHandle::Null, @p handle doesn't have to be valid
         * however. If @ref DebugLayerSource::Nodes is enabled, the @p name
         * will be used to annotate given @p handle, otherwise the function
         * does nothing.
         *
         * If @p name is @relativeref{Corrade,Containers::StringViewFlag::Global}
         * and @relativeref{Corrade::Containers::StringViewFlag,NullTerminated},
         * no internal copy of the string is made. If
         * @ref DebugLayerSource::Nodes isn't enabled, the function does
         * nothing.
         */
        DebugLayer& setNodeName(NodeHandle handle, Containers::StringView name);

        /**
         * @brief Layer name
         *
         * Expects that the debug layer has been already passed to
         * @ref AbstractUserInterface::setLayerInstance() and that @p handle
         * isn't @ref LayerHandle::Null, @p handle doesn't have to be valid
         * however. If @ref DebugLayerSource::Layers isn't enabled or @p handle
         * isn't known, returns an empty string. For @ref handle() returns
         * @cpp "Debug" @ce if a different name wasn't set.
         *
         * If not empty, the returned string is always
         * @relativeref{Corrade,Containers::StringViewFlag::NullTerminated}.
         * Unless @ref setLayerName() was called with a
         * @relativeref{Corrade,Containers::StringViewFlag::Global} string, the
         * returned view is only guaranteed to be valid until the next call to
         * @ref setLayerName() or until the set of layers in the user interface
         * changes.
         */
        Containers::StringView layerName(LayerHandle handle) const;

        /**
         * @brief Set layer name
         * @return Reference to self (for method chaining)
         *
         * Expects that the debug layer has been already passed to
         * @ref AbstractUserInterface::setLayerInstance() and that @p layer is
         * part of the same user interface. If @ref DebugLayerSource::Layers is
         * enabled, the @p name will be used to annotate attachments from given
         * @p layer, otherwise the function does nothing.
         *
         * If @p name is @relativeref{Corrade,Containers::StringViewFlag::Global}
         * and @relativeref{Corrade::Containers::StringViewFlag,NullTerminated},
         * no internal copy of the string is made. If
         * @ref DebugLayerSource::Layers isn't enabled, the function does
         * nothing.
         *
         * If a concrete layer type gets passed instead of just
         * @ref AbstractLayer, the @ref setLayerName(const T&, const Containers::StringView&)
         * overload may get picked if given layer implements debug integration,
         * allowing it to provide further details.
         */
        DebugLayer& setLayerName(const AbstractLayer& layer, Containers::StringView name);

        /**
         * @brief Set name for a layer with @ref AbstractLayer::DebugIntegration implemented
         * @return Reference to self (for method chaining)
         *
         * In addition to @ref setLayerName(const AbstractLayer&, Containers::StringView),
         * if @ref DebugLayerSource::NodeDataDetails is enabled, the layer's
         * @relativeref{AbstractLayer,DebugIntegration} implementation gets
         * called for each attachment, allowing it to provide further details.
         * See documentation of a particular layer for more information. Use
         * the @ref setLayerName(const T&, const Containers::StringView&, const typename T::DebugIntegration&)
         * overload to pass additional arguments to the debug integration.
         */
        template<class T
            #ifndef DOXYGEN_GENERATING_OUTPUT
            , typename std::enable_if<std::is_default_constructible<typename T::DebugIntegration>::value, int>::type = 0
            #endif
        > DebugLayer& setLayerName(const T& layer, const Containers::StringView& name) {
            /* Delegating to the r-value overload */
            return setLayerName(layer, name, typename T::DebugIntegration{});
        }

        /**
         * @brief Set name for a layer with @ref AbstractLayer::DebugIntegration implemented
         * @return Reference to self (for method chaining)
         *
         * In addition to @ref setLayerName(const AbstractLayer&, Containers::StringView)
         * the passed layer @relativeref{AbstractLayer,DebugIntegration}
         * instance gets called for each attachment, allowing it to provide
         * further details. See documentation of a particular layer for more
         * information.
         */
        template<class T> DebugLayer& setLayerName(const T& layer, const Containers::StringView& name, const typename T::DebugIntegration& integration);
        /** @overload */
        template<class T> DebugLayer& setLayerName(const T& layer, const Containers::StringView& name, typename T::DebugIntegration&& integration);

        /**
         * @brief Layouter name
         *
         * Expects that the debug layer has been already passed to
         * @ref AbstractUserInterface::setLayerInstance() and that @p handle
         * isn't @ref LayouterHandle::Null, @p handle doesn't have to be valid
         * however. If @ref DebugLayerSource::Layouters isn't enabled or
         * @p handle isn't known, returns an empty string.
         *
         * If not empty, the returned string is always
         * @relativeref{Corrade,Containers::StringViewFlag::NullTerminated}.
         * Unless @ref setLayouterName() was called with a
         * @relativeref{Corrade,Containers::StringViewFlag::Global} string, the
         * returned view is only guaranteed to be valid until the next call to
         * @ref setLayouterName() or until the set of layouters in the user
         * interface changes.
         */
        Containers::StringView layouterName(LayouterHandle handle) const;

        /**
         * @brief Set layouter name
         * @return Reference to self (for method chaining)
         *
         * Expects that the debug layer has been already passed to
         * @ref AbstractUserInterface::setLayerInstance() and that @p layouter
         * is part of the same user interface. If @ref DebugLayerSource::Layouters
         * is enabled, the @p name will be used to annotate assignments from
         * given @p layouter, otherwise the function does nothing.
         *
         * If @p name is @relativeref{Corrade,Containers::StringViewFlag::Global}
         * and @relativeref{Corrade::Containers::StringViewFlag,NullTerminated},
         * no internal copy of the string is made. If
         * @ref DebugLayerSource::Layouters isn't enabled, the function does
         * nothing.
         *
         * If a concrete layouter type gets passed instead of just
         * @ref AbstractLayouter, the @ref setLayouterName(const T&, const Containers::StringView&)
         * overload may get picked if given layouter implements debug
         * integration, allowing it to provide further details.
         */
        DebugLayer& setLayouterName(const AbstractLayouter& layouter, Containers::StringView name);

        /**
         * @brief Set name for a layouter with @ref AbstractLayouter::DebugIntegration implemented
         * @return Reference to self (for method chaining)
         *
         * In addition to @ref setLayouterName(const AbstractLayouter&, Containers::StringView),
         * if @ref DebugLayerSource::NodeLayoutDetails is enabled, the
         * layouters's @relativeref{AbstractLayouter,DebugIntegration}
         * implementation gets called for each attachment, allowing it to
         * provide further details. See documentation of a particular animator
         * for more information. Use the
         * @ref setLayouterName(const T&, const Containers::StringView&, const typename T::DebugIntegration&)
         * overload to pass additional arguments to the debug integration.
         */
        template<class T
            #ifndef DOXYGEN_GENERATING_OUTPUT
            , typename std::enable_if<std::is_default_constructible<typename T::DebugIntegration>::value, int>::type = 0
            #endif
        > DebugLayer& setLayouterName(const T& layouter, const Containers::StringView& name) {
            /* Delegating to the r-value overload */
            return setLayouterName(layouter, name, typename T::DebugIntegration{});
        }

        /**
         * @brief Set name for a layouter with @ref AbstractLayouter::DebugIntegration implemented
         * @return Reference to self (for method chaining)
         *
         * In addition to @ref setLayouterName(const AbstractLayouter&, Containers::StringView),
         * if @ref DebugLayerSource::NodeLayoutDetails is enabled, the passed
         * layouter @relativeref{AbstractLayouter,DebugIntegration} instance
         * gets called for each attachment, allowing it to provide further
         * details. See documentation of a particular layouter for more
         * information.
         */
        template<class T> DebugLayer& setLayouterName(const T& layouter, const Containers::StringView& name, const typename T::DebugIntegration& integration);
        /** @overload */
        template<class T> DebugLayer& setLayouterName(const T& layouter, const Containers::StringView& name, typename T::DebugIntegration&& integration);

        /**
         * @brief Animator name
         *
         * Expects that the debug layer has been already passed to
         * @ref AbstractUserInterface::setLayerInstance() and that @p handle
         * isn't @ref AnimatorHandle::Null, @p handle doesn't have to be valid
         * however. If @ref DebugLayerSource::Animators isn't enabled or
         * @p handle isn't known, returns an empty string.
         *
         * If not empty, the returned string is always
         * @relativeref{Corrade,Containers::StringViewFlag::NullTerminated}.
         * Unless @ref setAnimatorName() was called with a
         * @relativeref{Corrade,Containers::StringViewFlag::Global} string, the
         * returned view is only guaranteed to be valid until the next call to
         * @ref setAnimatorName() or until the set of animators in the user
         * interface changes.
         */
        Containers::StringView animatorName(AnimatorHandle handle) const;

        /**
         * @brief Set animator name
         * @return Reference to self (for method chaining)
         *
         * Expects that the debug layer has been already passed to
         * @ref AbstractUserInterface::setLayerInstance() and that @p animator
         * is part of the same user interface. If @ref DebugLayerSource::Animators
         * is enabled, the @p name will be used to annotate attachments from
         * given @p animator, otherwise the function does nothing.
         *
         * If @p name is @relativeref{Corrade,Containers::StringViewFlag::Global}
         * and @relativeref{Corrade::Containers::StringViewFlag,NullTerminated},
         * no internal copy of the string is made. If
         * @ref DebugLayerSource::Animators isn't enabled, the function does
         * nothing.
         *
         * If a concrete animator type gets passed instead of just
         * @ref AbstractAnimator, the @ref setAnimatorName(const T&, const Containers::StringView&)
         * overload may get picked if given animator implements debug
         * integration, allowing it to provide further details.
         */
        DebugLayer& setAnimatorName(const AbstractAnimator& animator, Containers::StringView name);

        /**
         * @brief Set name for an animator with @ref AbstractAnimator::DebugIntegration implemented
         * @return Reference to self (for method chaining)
         *
         * In addition to @ref setAnimatorName(const AbstractAnimator&, Containers::StringView),
         * if @ref DebugLayerSource::NodeAnimationDetails is enabled, the
         * animator's @relativeref{AbstractAnimator,DebugIntegration}
         * implementation gets called for each attachment, allowing it to
         * provide further details. See documentation of a particular animator
         * for more information. Use the
         * @ref setAnimatorName(const T&, const Containers::StringView&, const typename T::DebugIntegration&)
         * overload to pass additional arguments to the debug integration.
         */
        template<class T
            #ifndef DOXYGEN_GENERATING_OUTPUT
            , typename std::enable_if<std::is_default_constructible<typename T::DebugIntegration>::value, int>::type = 0
            #endif
        > DebugLayer& setAnimatorName(const T& animator, const Containers::StringView& name) {
            /* Delegating to the r-value overload */
            return setAnimatorName(animator, name, typename T::DebugIntegration{});
        }

        /**
         * @brief Set name for an animator with @ref AbstractAnimator::DebugIntegration implemented
         * @return Reference to self (for method chaining)
         *
         * In addition to @ref setAnimatorName(const AbstractAnimator&, Containers::StringView),
         * if @ref DebugLayerSource::NodeAnimationDetails is enabled, the
         * passed animator @relativeref{AbstractAnimator,DebugIntegration}
         * instance gets called for each attachment, allowing it to provide
         * further details. See documentation of a particular animator for more
         * information.
         */
        template<class T> DebugLayer& setAnimatorName(const T& animator, const Containers::StringView& name, const typename T::DebugIntegration& integration);
        /** @overload */
        template<class T> DebugLayer& setAnimatorName(const T& animator, const Containers::StringView& name, typename T::DebugIntegration&& integration);

        /** @brief Node inspect color */
        Color4 nodeInspectColor() const;

        /**
         * @brief Set node inspect color
         * @return Reference to self (for method chaining)
         *
         * Used only if @ref DebugLayerFlag::NodeInspect is enabled and if the
         * layer is instantiated as @ref DebugLayerGL to be able to draw the
         * highlight rectangle, ignored otherwise. Default is
         * @cpp 0xff00ffff_rgbaf*0.5f @ce.
         *
         * If the layer is instantiated as @ref DebugLayerGL, calling this
         * function causes @ref LayerState::NeedsDataUpdate to be set.
         * @see @ref setNodeInspectGesture(), @ref setNodeInspectCallback()
         */
        DebugLayer& setNodeInspectColor(const Color4& color);

        /** @brief Node inspect gesture */
        Containers::Pair<Pointers, Modifiers> nodeInspectGesture() const;

        /**
         * @brief Set node inspect gesture
         * @return Reference to self (for method chaining)
         *
         * Used only if @ref DebugLayerFlag::NodeInspect is enabled, ignored
         * otherwise. An inspect happens on a press of a pointer that's among
         * @p pointers with modifiers being exactly @p modifiers. Pressing on a
         * different node moves the highlight to the other node, pressing on a
         * node that's currently being inspected removes the highlight. Expects
         * that @p pointers are non-empty. Default is a combination of
         * @ref Pointer::MouseRight and @ref Pointer::Eraser, and
         * @ref Modifier::Ctrl, i.e. pressing either
         * @m_class{m-label m-warning} **Ctrl**
         * @m_class{m-label m-default} **right mouse button** or
         * @m_class{m-label m-warning} **Ctrl**
         * @m_class{m-label m-default} **pen eraser** will inspect a node under
         * the pointer. The currently inspected node is available in
         * @ref currentInspectedNode(), you can also use @ref inspectNode() to
         * perform a node inspect programmatically.
         * @see @ref setNodeInspectColor(),
         *      @ref setNodeInspectCallback()
         */
        DebugLayer& setNodeInspectGesture(Pointers pointers, Modifiers modifiers);

        /** @brief Whether a node inspect callback is set */
        bool hasNodeInspectCallback() const;

        /**
         * @brief Set node inspect callback
         * @return Reference to self (for method chaining)
         *
         * Used only if @ref DebugLayerFlag::NodeInspect is enabled, ignored
         * otherwise. The @p callback receives a UTF-8 @p message with details
         * when a highlight happens on a pointer press, and an empty string if
         * a highlight is removed again. If not empty, the @p message is
         * guaranteed to be @relativeref{Corrade,Containers::StringViewFlag::NullTerminated}.
         *
         * If the callback is not set or if set to @cpp nullptr @ce, details
         * about the inspected node are printed to @relativeref{Magnum,Debug}
         * instead.
         * @see @ref setNodeInspectColor(),
         *      @ref setNodeInspectGesture()
         */
        DebugLayer& setNodeInspectCallback(Containers::Function<void(Containers::StringView message)>&& callback);

        /**
         * @brief Node inspected by last pointer press
         *
         * Expects that @ref DebugLayerFlag::NodeInspect is enabled. If no node
         * is currently being inspected, returns @ref NodeHandle::Null.
         *
         * The returned handle may be invalid if the node or any of its parents
         * were removed and @ref AbstractUserInterface::clean() wasn't called
         * since.
         */
        NodeHandle currentInspectedNode() const;

        /**
         * @brief Inspect a node
         *
         * Expects that @ref DebugLayerFlag::NodeInspect is enabled and the
         * layer has been already passed to
         * @ref AbstractUserInterface::setLayerInstance().
         *
         * If @p node is a known handle, the function performs similarly to the
         * node inspect gesture using a pointer press ---
         * @ref currentInspectedNode() is set to @p node, details about the
         * node are printed to @relativeref{Magnum,Debug} or passed to a
         * callback if set, the node is visually highlighted if this is a
         * @ref DebugLayerGL instance, and the function returns @cpp true @ce.
         *
         * If @p node is @ref NodeHandle::Null or it's not a known handle (for
         * example an invalid handle of a now-removed node, or a handle of a
         * newly created node but @ref AbstractUserInterface::update() wasn't
         * called since) and there's a currently inspected node, its highlight
         * is removed. The function doesn't print anything, but if a callback
         * is set, it's called with an empty string. If there's no currently
         * inspected node, the  callback isn't called. The functions returns
         * @cpp true @ce if @p node is @ref NodeHandle::Null and @cpp false @ce
         * if the handle is unknown.
         *
         * Note that, compared to the node inspect gesture, where the node
         * details are always extracted from an up-to-date UI state, this
         * function only operates with the state known at the last call to
         * @ref AbstractUserInterface::update(). As such, for example nodes or
         * layers added since the last update won't be included in the output.
         *
         * If the layer is instantiated as @ref DebugLayerGL and @p node is
         * different from the previously inspected node, calling this function
         * causes @ref LayerState::NeedsDataUpdate to be set.
         * @see @ref setNodeInspectColor()
         *      @ref setNodeInspectGesture(),
         *      @ref setNodeInspectCallback()
         */
        bool inspectNode(NodeHandle node);

    #ifdef DOXYGEN_GENERATING_OUTPUT
    private:
    #else
    protected:
    #endif
        struct State;

        MAGNUM_UI_LOCAL explicit DebugLayer(LayerHandle handle, Containers::Pointer<State>&& state);

        /* These can't be MAGNUM_UI_LOCAL otherwise deriving from this class
           in tests causes linker errors */
        LayerFeatures doFeatures() const override;
        void doUpdate(LayerStates state, const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, const Containers::StridedArrayView1D<const Float>& nodeOpacities, Containers::BitArrayView nodesEnabled, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes, const Containers::StridedArrayView1D<const Vector2>& compositeRectOffsets, const Containers::StridedArrayView1D<const Vector2>& compositeRectSizes) override;

        Containers::Pointer<State> _state;

    private:
        /* Returned value is a pointer where to save the allocated
           DebugIntegration instance */
        void** setLayerNameDebugIntegration(const AbstractLayer& instance, const Containers::StringView& name, void(*deleter)(void*), void(*print)(void*, Debug&, const AbstractLayer&, const Containers::StringView&, LayerDataHandle));
        void** setLayouterNameDebugIntegration(const AbstractLayouter& instance, const Containers::StringView& name, void(*deleter)(void*), void(*print)(void*, Debug&, const AbstractLayouter&, const Containers::StringView&, LayouterDataHandle));
        void** setAnimatorNameDebugIntegration(const AbstractAnimator& instance, const Containers::StringView& name, void(*deleter)(void*), void(*print)(void*, Debug&, const AbstractAnimator&, const Containers::StringView&, AnimatorDataHandle));

        /* These can't be MAGNUM_UI_LOCAL otherwise deriving from this class
           in tests causes linker errors */
        LayerStates doState() const override;
        void doClean(Containers::BitArrayView dataIdsToRemove) override;
        void doPreUpdate(LayerStates state) override;
        void doPointerPressEvent(UnsignedInt dataId, PointerEvent& event) override;
};

template<class T> DebugLayer& DebugLayer::setLayerName(const T& layer, const Containers::StringView& name, const typename T::DebugIntegration& integration) {
    /* Delegating to the r-value overload. On MSVC 2015 and 2017 passing a
       temporary value like this doesn't pick the && overload but const& again,
       leading to an infinite recursion. Passing `Utility::move(typename ...)`
       doesn't work either (?!), one has to make a named copy and then move
       that, sigh. */
    #ifdef CORRADE_MSVC2017_COMPATIBILITY
    typename T::DebugIntegration copy{integration};
    return setLayerName(layer, name, Utility::move(copy));
    #else
    return setLayerName(layer, name,
        #if defined(CORRADE_TARGET_GCC) && !defined(CORRADE_TARGET_CLANG) && __GNUC__ < 5
        /* Can't use {} because for plain structs it would attempt to
           initialize the first member with `integration` instead of calling
           the copy constructor. Similar case is in Containers::Array etc., see
           layerNameDebugIntegrationCopyConstructPlainStruct() for details. */
        typename T::DebugIntegration(integration)
        #else
        typename T::DebugIntegration{integration}
        #endif
    );
    #endif
}

template<class T> DebugLayer& DebugLayer::setLayerName(const T& layer, const Containers::StringView& name, typename T::DebugIntegration&& integration) {
    void** instance = setLayerNameDebugIntegration(
        /* This cast isn't strictly necessary but it makes the error clearer in
           case a completely unrelated type (such as a layouter) is passed */
        static_cast<const AbstractLayer&>(layer), name,
        [](void* integration) {
            delete static_cast<typename T::DebugIntegration*>(integration);
        },
        [](void* integration, Debug& debug, const AbstractLayer& layer, const Containers::StringView& layerName, LayerDataHandle data) {
            static_cast<typename T::DebugIntegration*>(integration)->print(debug, static_cast<const T&>(layer), layerName, data);
        });
    /* If the instance is null, either NodeDataDetails isn't set, in which case
       the instance wouldn't be used anyway, or a graceful assert happened.
       Don't allocate anything in that case. */
    if(instance) {
        #if defined(CORRADE_TARGET_GCC) && !defined(CORRADE_TARGET_CLANG) && __GNUC__ < 5
        /* Can't use {} because for plain structs it would attempt to
           initialize the first member with `integration` instead of calling
           the move constructor. Similar case is in Containers::Array etc., see
           layerNameDebugIntegrationMoveConstructPlainStruct() for details. */
        *instance = new typename T::DebugIntegration(Utility::move(integration));
        #else
        *instance = new typename T::DebugIntegration{Utility::move(integration)};
        #endif
    }
    return *this;
}

template<class T> DebugLayer& DebugLayer::setLayouterName(const T& layouter, const Containers::StringView& name, const typename T::DebugIntegration& integration) {
    /* Like setLayerName(..., const DebugIntegration&), just for layouters. See
       above for comments about all the workarounds, sigh. */
    #ifdef CORRADE_MSVC2017_COMPATIBILITY
    typename T::DebugIntegration copy{integration};
    return setLayouterName(layouter, name, Utility::move(copy));
    #else
    return setLayouterName(layouter, name,
        #if defined(CORRADE_TARGET_GCC) && !defined(CORRADE_TARGET_CLANG) && __GNUC__ < 5
        typename T::DebugIntegration(integration)
        #else
        typename T::DebugIntegration{integration}
        #endif
    );
    #endif
}

template<class T> DebugLayer& DebugLayer::setLayouterName(const T& layouter, const Containers::StringView& name, typename T::DebugIntegration&& integration) {
    /* Like setLayerName(..., DebugIntegration&&), just for layouters. See
       above for comments about all the workarounds, sigh. */
    void** instance = setLayouterNameDebugIntegration(
        /* This cast isn't strictly necessary but it makes the error clearer in
           case a completely unrelated type (such as a layer) is passed */
        static_cast<const AbstractLayouter&>(layouter), name,
        [](void* integration) {
            delete static_cast<typename T::DebugIntegration*>(integration);
        },
        [](void* integration, Debug& debug, const AbstractLayouter& layouter, const Containers::StringView& layouterName, LayouterDataHandle layout) {
            static_cast<typename T::DebugIntegration*>(integration)->print(debug, static_cast<const T&>(layouter), layouterName, layout);
        });
    /* If the instance is null, either NodeLayoutDetails isn't set, in which
       case the instance wouldn't be used anyway, or a graceful assert
       happened. Don't allocate anything in that case. */
    if(instance) {
        #if defined(CORRADE_TARGET_GCC) && !defined(CORRADE_TARGET_CLANG) && __GNUC__ < 5
        *instance = new typename T::DebugIntegration(Utility::move(integration));
        #else
        *instance = new typename T::DebugIntegration{Utility::move(integration)};
        #endif
    }
    return *this;
}

template<class T> DebugLayer& DebugLayer::setAnimatorName(const T& animator, const Containers::StringView& name, const typename T::DebugIntegration& integration) {
    /* Like setLayerName(..., const DebugIntegration&), just for animators. See
       above for comments about all the workarounds, sigh. */
    #ifdef CORRADE_MSVC2017_COMPATIBILITY
    typename T::DebugIntegration copy{integration};
    return setAnimatorName(animator, name, Utility::move(copy));
    #else
    return setAnimatorName(animator, name,
        #if defined(CORRADE_TARGET_GCC) && !defined(CORRADE_TARGET_CLANG) && __GNUC__ < 5
        typename T::DebugIntegration(integration)
        #else
        typename T::DebugIntegration{integration}
        #endif
    );
    #endif
}

template<class T> DebugLayer& DebugLayer::setAnimatorName(const T& animator, const Containers::StringView& name, typename T::DebugIntegration&& integration) {
    /* Like setLayerName(..., DebugIntegration&&), just for animators. See
       above for comments about all the workarounds, sigh. */
    void** instance = setAnimatorNameDebugIntegration(
        /* This cast isn't strictly necessary but it makes the error clearer in
           case a completely unrelated type (such as a layer) is passed */
        static_cast<const AbstractAnimator&>(animator), name,
        [](void* integration) {
            delete static_cast<typename T::DebugIntegration*>(integration);
        },
        [](void* integration, Debug& debug, const AbstractAnimator& animator, const Containers::StringView& animatorName, AnimatorDataHandle data) {
            static_cast<typename T::DebugIntegration*>(integration)->print(debug, static_cast<const T&>(animator), animatorName, data);
        });
    /* If the instance is null, either NodeAnimationDetails isn't set, in which
       case the instance wouldn't be used anyway, or a graceful assert
       happened. Don't allocate anything in that case. */
    if(instance) {
        #if defined(CORRADE_TARGET_GCC) && !defined(CORRADE_TARGET_CLANG) && __GNUC__ < 5
        *instance = new typename T::DebugIntegration(Utility::move(integration));
        #else
        *instance = new typename T::DebugIntegration{Utility::move(integration)};
        #endif
    }
    return *this;
}

}}

#endif
