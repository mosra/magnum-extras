#ifndef Magnum_Ui_AbstractLayer_h
#define Magnum_Ui_AbstractLayer_h
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
 * @brief Class @ref Magnum::Ui::AbstractLayer, enum @ref Magnum::Ui::LayerFeature, @ref Magnum::Ui::LayerState, enum set @ref Magnum::Ui::LayerFeatures, @ref Magnum::Ui::LayerStates
 * @m_since_latest
 */

#include <Corrade/Containers/EnumSet.h>
#include <Corrade/Containers/Pointer.h>
#include <Magnum/Magnum.h>

#include "Magnum/Ui/Ui.h"
#include "Magnum/Ui/visibility.h"

namespace Magnum { namespace Ui {

/**
@brief Features supported by a layer
@m_since_latest

@see @ref LayerFeatures, @ref AbstractLayer::features()
*/
enum class LayerFeature: UnsignedByte {
    /** Drawing using @ref AbstractLayer::draw() */
    Draw = 1 << 0,

    /**
     * Drawing using @ref AbstractLayer::draw() uses blending. Causes
     * @ref RendererDrawState::Blending to be passed to
     * @ref AbstractRenderer::transition() before drawing the layer. Implies
     * @ref LayerFeature::Draw.
     */
    DrawUsesBlending = Draw|(1 << 1),

    /**
     * Drawing using @ref AbstractLayer::draw() uses blending. Causes
     * @ref RendererDrawState::Scissor to be passed to
     * @ref AbstractRenderer::transition() before drawing the layer. Implies
     * @ref LayerFeature::Draw.
     */
    DrawUsesScissor = Draw|(1 << 2),

    /**
     * Compositing contents drawn underneath this layer using
     * @ref AbstractLayer::composite(), such as for example background blur,
     * and then uses the result of the composition for actual drawing. It's
     * assumed that the composition operation copies contents of the
     * framebuffer or processes them in some way so that a subsequent
     * @ref AbstractLayer::draw() can be performed to the same framebuffer
     * without causing a cyclic dependency. Implies @ref LayerFeature::Draw.
     */
    Composite = (1 << 3)|Draw,

    /**
     * Event handling using @ref AbstractLayer::pointerPressEvent(),
     * @relativeref{AbstractLayer,pointerReleaseEvent()} and
     * @relativeref{AbstractLayer,pointerMoveEvent()}.
     */
    Event = 1 << 4,

    /**
     * Assigning data animators using
     * @ref AbstractLayer::assignAnimator(AbstractDataAnimator&) const and
     * animating data using @ref AbstractLayer::advanceAnimations(Nanoseconds, Containers::MutableBitArrayView, Containers::MutableBitArrayView, Containers::MutableBitArrayView, const Containers::StridedArrayView1D<Float>&, Containers::MutableBitArrayView, const Containers::Iterable<AbstractDataAnimator>&).
     */
    AnimateData = 1 << 5,

    /**
     * Assigning style animators using
     * @ref AbstractLayer::assignAnimator(AbstractStyleAnimator&) const and
     * animating styles using @ref AbstractLayer::advanceAnimations(Nanoseconds, Containers::MutableBitArrayView, Containers::MutableBitArrayView, Containers::MutableBitArrayView, const Containers::StridedArrayView1D<Float>&, Containers::MutableBitArrayView, const Containers::Iterable<AbstractStyleAnimator>&).
     */
    AnimateStyles = 1 << 6,
};

/**
@debugoperatorenum{LayerFeature}
@m_since_latest
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, LayerFeature value);

/**
@brief Set of features supported by a layer
@m_since_latest

@see @ref AbstractLayer::features()
*/
typedef Containers::EnumSet<LayerFeature> LayerFeatures;

/**
@debugoperatorenum{LayerFeatures}
@m_since_latest
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, LayerFeatures value);

CORRADE_ENUMSET_OPERATORS(LayerFeatures)

/**
@brief Layer state
@m_since_latest

Used to decide whether @ref AbstractLayer::cleanData() (called from
@ref AbstractUserInterface::clean()) or @ref AbstractLayer::update() (called
from @ref AbstractUserInterface::update()) need to be called to refresh the
internal state before the interface is drawn or an event is handled. See
@ref UserInterfaceState for interface-wide state.
@see @ref LayerStates, @ref AbstractLayer::state()
*/
enum class LayerState: UnsignedShort {
    /**
     * @ref AbstractLayer::update() (which is called from
     * @ref AbstractUserInterface::update()) needs to be called to recalculate
     * and reupload node-disabled-related state after the set of enabled nodes
     * changed. Transitively set after every @ref AbstractLayer::create() with
     * a non-null @ref NodeHandle and after every @ref AbstractLayer::attach()
     * call that attaches data to a different non-null @ref NodeHandle. Is
     * reset next time @ref AbstractLayer::update() is called with this flag
     * present. Implied by @ref LayerState::NeedsNodeOrderUpdate.
     *
     * Gets passed to @ref AbstractLayer::update() when
     * @ref UserInterfaceState::NeedsNodeEnabledUpdate or anything that implies
     * it is set on the user interface. Is never returned by
     * @ref AbstractLayer::state() alone.
     */
    NeedsNodeEnabledUpdate = 1 << 0,

    /**
     * @ref AbstractLayer::update() (which is called from
     * @ref AbstractUserInterface::update()) needs to be called to recalculate
     * and reupload node-opacity-related state after node opacities changed.
     * Transitively set after every @ref AbstractLayer::create() with
     * a non-null @ref NodeHandle and after every @ref AbstractLayer::attach()
     * call that attaches data to a different non-null @ref NodeHandle. Is
     * reset next time @ref AbstractLayer::update() is called with this flag
     * present. Implied by @ref LayerState::NeedsAttachmentUpdate.
     *
     * Gets passed to @ref AbstractLayer::update() when
     * @ref UserInterfaceState::NeedsNodeOpacityUpdate or anything that implies
     * it is set on the user interface. Is never returned by
     * @ref AbstractLayer::state() alone.
     */
    NeedsNodeOpacityUpdate = 1 << 1,

    /**
     * @ref AbstractLayer::update() (which is called from
     * @ref AbstractUserInterface::update()) needs to be called to recalculate
     * and reupload draw-order-related state such as index buffers after node
     * order changed. Transitively set after every @ref AbstractLayer::create()
     * with a non-null @ref NodeHandle and after every
     * @ref AbstractLayer::attach() call that attaches data to a different
     * non-null @ref NodeHandle. Is reset next time
     * @ref AbstractLayer::update() is called with with this flag present.
     * Implies @ref LayerState::NeedsNodeEnabledUpdate, as a change in the set
     * of visible nodes may cause the set of enabled nodes to change. Implied
     * by @relativeref{LayerState,NeedsNodeOffsetSizeUpdate} and
     * @relativeref{LayerState,NeedsAttachmentUpdate}.
     *
     * Gets passed to @ref AbstractLayer::update() when
     * @ref UserInterfaceState::NeedsNodeClipUpdate,
     * @ref UserInterfaceState::NeedsDataAttachmentUpdate or anything that
     * implies those is set on the user interface. Is only returned together
     * with @ref LayerState::NeedsAttachmentUpdate by
     * @ref AbstractLayer::state(), never alone.
     */
    NeedsNodeOrderUpdate = NeedsNodeEnabledUpdate|(1 << 2),

    /**
     * @ref AbstractLayer::update() (which is called from
     * @ref AbstractUserInterface::update()) needs to be called to recalculate
     * and reupload position-related state after offsets and sizes of nodes the
     * data are attached to changed, including the case when data get attached
     * to a different node without the node offsets or sizes itself changing.
     * Set implicitly after every @ref AbstractLayer::create() with a non-null
     * @ref NodeHandle and after every @ref AbstractLayer::attach() call that
     * attaches data to a different non-null @ref NodeHandle. Is reset next
     * time @ref AbstractLayer::update() is called with this flag present.
     * Implies @ref LayerState::NeedsNodeOrderUpdate, as nodes changing their
     * offsets or sizes may cause the set of visible nodes, and thus their
     * order, to change; often gets set together with
     * @ref LayerState::NeedsAttachmentUpdate by
     * @ref AbstractLayer::attach() and @ref AbstractLayer::create().
     *
     * Besides being present in @ref AbstractLayer::state() gets passed to
     * @ref AbstractLayer::update() when
     * @ref UserInterfaceState::NeedsLayoutUpdate or anything that implies it
     * is set on the user interface.
     */
    NeedsNodeOffsetSizeUpdate = NeedsNodeOrderUpdate|(1 << 3),

    /**
     * @ref AbstractLayer::update() (which is called from
     * @ref AbstractUserInterface::update()) needs to be called to refresh the
     * data attached to visible node hierarchy after the node attachments were
     * changed. Set implicitly after every @ref AbstractLayer::attach() call,
     * after @ref AbstractLayer::create() with a non-null @ref NodeHandle and
     * after @ref AbstractLayer::remove() for a data that's attached to a node,
     * is reset next time @ref AbstractLayer::update() is called with this flag
     * present. Implies @ref LayerState::NeedsNodeOpacityUpdate, as node
     * attachment change may cause the opacity used by a particular data to
     * change; and @ref LayerState::NeedsNodeOrderUpdate, as node attachment
     * change may cause the set of visible nodes, and thus their order, to
     * change; often gets set together with
     * @ref LayerState::NeedsNodeOffsetSizeUpdate by
     * @ref AbstractLayer::attach() and @ref AbstractLayer::create().
     *
     * If set on a layer, causes @ref UserInterfaceState::NeedsDataAttachmentUpdate
     * to be set on the user interface. Gets passed to
     * @ref AbstractLayer::update() only if the layer itself has it set,
     * independently of @ref UserInterfaceState::NeedsDataAttachmentUpdate
     * being present.
     */
    NeedsAttachmentUpdate = NeedsNodeOpacityUpdate|NeedsNodeOrderUpdate|(1 << 4),

    /**
     * @ref AbstractLayer::update() (which is called from
     * @ref AbstractUserInterface::update()) needs to be called to recalculate
     * and reupload data after they've been changed. Set implicitly after every
     * @ref AbstractLayer::create() independently of whether also attached to a
     * @ref NodeHandle or not and whether that node is currently visible. Can
     * also be returned by @ref AbstractLayer::doState() or be explicitly set
     * by the layer implementation using @ref AbstractLayer::setNeedsUpdate().
     * Is reset next time @ref AbstractLayer::update() is called with this flag
     * present.
     *
     * If set on a layer, causes @ref UserInterfaceState::NeedsDataUpdate to
     * be set on the user interface. Gets passed to @ref AbstractLayer::update()
     * only if the layer itself has it set, independently of
     * @ref UserInterfaceState::NeedsDataUpdate being present.
     */
    NeedsDataUpdate = 1 << 5,

    /**
     * @ref AbstractLayer::update() (which is called from
     * @ref AbstractUserInterface::update()) needs to be called to recalculate
     * or reupload common layer data such as dynamic style data after they've
     * been changed. Can be returned by @ref AbstractLayer::doState(), can also
     * be explicitly set by the layer implementation using
     * @ref AbstractLayer::setNeedsUpdate(), is reset next time
     * @ref AbstractLayer::update() is called with this flag present.
     *
     * If set on a layer, causes @ref UserInterfaceState::NeedsDataUpdate to
     * be set on the user interface. Gets passed to @ref AbstractLayer::update()
     * only if the layer itself has it set, independently of
     * @ref UserInterfaceState::NeedsDataUpdate being present.
     */
    NeedsCommonDataUpdate = 1 << 6,

    /**
     * @ref AbstractLayer::update() (which is called from
     * @ref AbstractUserInterface::update()) needs to be called to recalculate
     * or reupload shared layer data such as shared style data after they've
     * been changed. Can be returned by @ref AbstractLayer::doState(), can also
     * be explicitly set by the layer implementation using
     * @ref AbstractLayer::setNeedsUpdate(). Is reset next time
     * @ref AbstractLayer::update() is called with this flag present.
     *
     * If set on a layer, causes @ref UserInterfaceState::NeedsDataUpdate to
     * be set on the user interface. Gets passed to @ref AbstractLayer::update()
     * only if the layer itself has it set, independently of
     * @ref UserInterfaceState::NeedsDataUpdate being present.
     */
    NeedsSharedDataUpdate = 1 << 7,

    /**
     * @ref AbstractLayer::update() (which is called from
     * @ref AbstractUserInterface::update()) needs to be called to recalculate
     * and reupload compositing-related data after node sizes and offsets
     * changed. Set on layers that advertise @ref LayerFeature::Composite
     * implicitly after every @ref AbstractLayer::create() with a non-null
     * @ref NodeHandle and after every @ref AbstractLayer::attach() call that
     * attaches data to a different non-null @ref NodeHandle. Can also be
     * returned by @ref AbstractLayer::doState() or be explicitly set by the
     * layer implementation using @ref AbstractLayer::setNeedsUpdate() if the
     * layer advertises @ref LayerFeature::Composite. Is reset next time
     * @ref AbstractLayer::update() is called with this flag present.
     *
     * Besides being present in @ref AbstractLayer::state() gets passed to
     * @ref AbstractLayer::update() when
     * @ref UserInterfaceState::NeedsLayoutUpdate or anything that implies it
     * is set on the user interface and the layer advertises
     * @ref LayerFeature::Composite.
     */
    NeedsCompositeOffsetSizeUpdate = 1 << 8,

    /**
     * @ref AbstractLayer::cleanData() (which is called from
     * @ref AbstractUserInterface::clean()) needs to be called to prune
     * animations attached to removed data. Set implicitly after every
     * @ref AbstractLayer::remove() call, is reset next time
     * @ref AbstractLayer::cleanData() is called with this flag present.
     *
     * If set on a layer, causes @ref UserInterfaceState::NeedsDataClean
     * to be set on the user interface. Doesn't get passed to
     * @ref AbstractLayer::update().
     */
    NeedsDataClean = 1 << 9
};

/**
@debugoperatorenum{LayerState}
@m_since_latest
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, LayerState value);

/**
@brief Layer states
@m_since_latest

@see @ref AbstractLayer::state()
*/
typedef Containers::EnumSet<LayerState> LayerStates;

/**
@debugoperatorenum{LayerStates}
@m_since_latest
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, LayerStates value);

CORRADE_ENUMSET_OPERATORS(LayerStates)

/**
@brief Base for data layers
@m_since_latest

Attaches data to particular nodes in the UI hierarchy, providing rendering and
event handling functionality. See the
@ref Ui-AbstractUserInterface-layers "AbstractUserInterface class documentation"
for introduction and overview of builtin layers. The following sections
describe behavior common to all layers and provide a guide for implementing
custom layers from scratch.

@section Ui-AbstractLayer-handles Layer data creation and removal

Layer data get created using @ref create() with an optional @ref NodeHandle to
attach the data to, returning a @ref DataHandle. The @ref create() function is
@cpp protected @ce on the @ref AbstractLayer, as concrete implementations
require additional parameters. Such as @ref TextLayer::create() taking also the
actual text to render, or for example @ref EventLayer having several
differently named functions like @relativeref{EventLayer,onTapOrClick()} or
@relativeref{EventLayer,onDrag()} for reacting to different events. As a
special case the @ref DebugLayer then doesn't expose any data creation
functionality at all, as it manages its data implicitly internally based on
what sources it tracks.

The @ref DataHandle is a combination of a @ref LayerHandle, identifying a
particular layer the data is coming from, and a @ref LayerDataHandle
identifying data within given layer, extractible using @ref dataHandleLayer()
and @ref dataHandleData(), respectively. All builtin layer APIs taking a
@ref DataHandle have overloads taking the smaller @ref LayerDataHandle type as
well, which is useful to save space in case you're storing the handles and know
which layer they come from.

@snippet Ui.cpp AbstractLayer-handles

Data lifetime is implicitly tied to a @ref NodeHandle they're attached to, if
any, so if the node or any of its parents get removed, all data attached to it
from all layers get removed. It's also possible to remove the data directly
using @ref remove(), after which the @ref DataHandle (or @ref LayerDataHandle)
becomes invalid. The @ref remove() function is again @cpp protected @ce as
concrete layers may want to extend its behavior, such as in case of
@ref TextLayer::remove() or @ref EventLayer::remove().

@section Ui-AbstractLayer-attachments Node data attachments

Besides attaching directly in @ref create() or its derivatives in subclasses,
node attachment can be modified using @ref attach(). A data can be also
detached from a node by passing @ref NodeHandle::Null, after which it's
excluded from all updates and rendering until it's attached to a node again.
This is useful for example to transfer some persistent state from one node to
another, or when it's containing a heavy resource and it makes more sense to
reattach an existing instance rather than remove and recreate it for a
different node.

@snippet Ui.cpp AbstractLayer-attachments

@section Ui-AbstractLayer-custom Creating a custom drawing layer

If none of the builtin layers provide desired functionality, it's possible to
implement a custom layer that then gets added among others. At the very least,
the @ref doFeatures() function needs to be implemented. Based on which
@ref LayerFeature values it returns, other interfaces need to be implemented as
well.

As an example, let's assume we want to implement a simple layer that draws
colored quads with OpenGL. In other words, a very small subset of what
@ref BaseLayer provides. The initial setup could look like this, with
@ref doFeatures() returning @ref LayerFeature::Draw and @ref create() along
with @ref remove() made public, specifying quad `color` at creation time:

@snippet Ui-gl.cpp AbstractLayer-custom

Internally the layer contains a @cpp struct @ce definition describing vertex
layout, @ref GL::Buffer for storing indices and vertices, a @ref GL::Mesh, and
a @ref Shaders::FlatGL shader to draw the mesh with. The shader is set up with
vertex colors enabled, and the mesh configured with a matching vertex layout.

@snippet Ui-gl.cpp AbstractLayer-custom-constructor

<b></b>

@m_class{m-note m-info}

@par
    In comparison, @ref BaseLayer has OpenGL-specific code in a
    @ref BaseLayerGL subclass instead, but such split is there for easier
    testing and to allow implementation of Vulkan and other backends. Custom
    layers don't need to follow this separation.

A common pattern for storing data associated with UI handles, as shown in
@ref Ui-AbstractUserInterface-handles, is to have a contiguous array indexed by
the handle ID, which is the case with the `_colors` member above. Creating a
data delegates to the base @ref create(), extracts the handle ID using
@ref dataHandleId(), enlarges the array to fit it, and saves the color there:

@snippet Ui-gl.cpp AbstractLayer-custom-create

As the layer stores just plain data, there's nothing to be done when data get
removed --- the color just stays unused in the array until it's overwritten by
a different handle that reuses the same ID. Data removal thus simply delegates
to the base @ref remove(), providing both a @ref DataHandle and a
@ref LayerDataHandle overload for convenience. Dealing with
@ref Ui-AbstractLayer-custom-resource-cleanup "resources that need explicit destruction"
is described later.

@snippet Ui-gl.cpp AbstractLayer-custom-remove

@subsection Ui-AbstractLayer-custom-update Implementing an update function

Each time @ref AbstractUserInterface::draw() is called and something in the UI
changed since last draw, it delegates to @ref AbstractUserInterface::update()
which then results in @ref doUpdate() being called with appropriate inputs on
all layers that need updating. The @ref doUpdate() implementation prepares data
for drawing; afterwards, depending on how many top-level node hierarchies the
layer is used in, follow one or more @ref doDraw() calls where the layer draws
a slice of data prepared in @ref doUpdate().

The @ref doUpdate() function receives a broad set of inputs, initially we'll be
interested in just the essential parameters to generate the quad mesh with,
shown in the snippet below. The `dataIds` view is a list of data handle IDs
attached to currently visible nodes, ordered back-to-front, so when they're
drawn in this order, they overlap correctly. Each time @ref doUpdate() is
called, the `dataIds` may have different size and contain different entries in
different order, based on what's currently visible.

@snippet Ui-gl.cpp AbstractLayer-custom-update-signature

Next there are `nodeOffsets` and `nodeSizes`, containing final node offsets and
sizes with node hierarchy and all layouts applied. The views contain offsets
and sizes for all nodes in the UI and are indexed by node ID. In other words,
they're *not* matching the order in `dataIds` --- instead, to get a
@ref NodeHandle attachment for a particular data ID, the view returned from
@ref nodes() is used, with @ref nodeHandleId() extracting the ID out of the
handle. Nodes that are not visible have offsets and sizes left in an
unspecified state, but as `dataIds` only contain IDs attached to currently
visible nodes, it's guaranteed that the `nodes[dataId]` is never
@ref NodeHandle::Null and both `nodeOffsets[nodeId]` and `nodeSizes[nodeId]`
have a meaningful value.

With the above inputs, the `vertexData` (containing @cpp 4 @ce vertices for
each quad), and `indexData` (with @cpp 6 @ce indices corresponding to the two
triangles) are filled, and then those are uploaded to the GPU mesh for drawing:

@snippet Ui-gl.cpp AbstractLayer-custom-update

@subsection Ui-AbstractLayer-custom-draw Drawing the data

In the @ref doUpdate() implementation above, we filled the mesh with vertex
positions in UI units. To apply a correct projection in the shader, we need to
know how large the UI is. The UI size is passed to the @ref doSetSize()
interface, which is called at least once before the first draw, and then each
time the UI size changes. We'll use the `size` to form a projection matrix
passed to @ref Shaders::FlatGL::setTransformationProjectionMatrix(), the second
argument is framebuffer size in pixels that we don't need at the moment. The
@ref Matrix3::projection() scales the size to the @f$ [-1, +1] @f$ unit square,
additionally we convert from the UI library coordinate system with Y down and
origin top left to OpenGL's Y up with origin in the center:

@snippet Ui-gl.cpp AbstractLayer-custom-setsize

<b></b>

@m_class{m-note m-info}

@par
    Every @ref doSetSize() is followed by a @ref doUpdate(), so instead of
    generating vertex data in the UI coordinates and setting up shader
    projection, the implementation can also just save the projection properties
    in @ref doSetSize() and then in @ref doUpdate() apply the projection
    directly to the vertex data.

Finally, the @ref doDraw() interface is called with almost the same inputs as
@ref doUpdate(), but additionally an `offset` and `count` is supplied,
describing the range of `dataIds` that is meant to be drawn. Since all setup
and upload was done in @ref doUpdate() already, we don't need to use any other
arguments and just use the appropriate index range, i.e. multiplying the
`offset` and `count` by @cpp 6 @ce to get the quad index range corresponding to
the data:

@snippet Ui-gl.cpp AbstractLayer-custom-draw

<b></b>

@m_class{m-note m-success}

@par
    The @ref doDraw() interface gets the same arguments as @ref doUpdate() to
    allow for convenient drawing in an immediate mode fashion, if desired. In
    such case you'd perform all setup in @ref doDraw() and wouldn't even need
    to implement @ref doUpdate(). For efficiency and avoiding GPU stalls it's
    however better to perform data upload separately from actual drawing, and
    by being in @ref doUpdate() the upload is only done if something actually
    changes.

@subsection Ui-AbstractLayer-custom-blending Drawing with alpha blending

The above showed the simplest possible case of drawing a fully opaque mesh. In
many cases you'll however want to have some sort of transparency, even if just
for smooth edges. For that, the layer can advertise
@ref Ui::LayerFeature::DrawUsesBlending in @ref doFeatures(), which makes the
@ref Ui-AbstractUserInterface-renderer "user itnerface renderer instance"
enable the corresponding state (such as @ref GL::Renderer::Feature::Blending in
case of OpenGL) when needed. Together with using @ref Color4 and the matching
@ref Shaders::FlatGL2D::Color4 attribute the `QuadLayer` would look like this
instead:

@snippet Ui-gl.cpp AbstractLayer-custom-blending

Note that the rest of the @ref Ui library uses a [premultiplied alpha](https://developer.nvidia.com/content/alpha-blending-pre-or-not-pre)
workflow and the custom layer should match that. Thus for example making a
color with 50% transparency is @cpp rgba*0.5f @ce rather than
@cpp {rgb, 0.5f} @ce. You can also use the @ref Color4::premultiplied() helper
to convert non-premultiplied RGBA colors to premultiplied.

@subsection Ui-AbstractLayer-custom-node-opacity-enabled Dealing with node opacity and disabled state

Among the other inputs passed to @ref doUpdate() are `nodeOpacities` and
`nodesEnabled`. They reflect presence of @ref NodeFlag::Disabled and values
passed to @ref AbstractUserInterface::setNodeOpacity(), together with
@ref Ui-AbstractUserInterface-nodes-opacity "propagation to child nodes".
If we'd draw quads in disabled nodes grayscale and slightly darker, and apply
the opacity in a premultiplied fashion, the relevant parts of the function
would look like this:

@snippet Ui-gl.cpp AbstractLayer-custom-node-opacity-enabled

You're free to do anything else with these inputs --- for example have an
entirely different "disabled look", or even ignore them altogether if given
layer is not expected to be used on such nodes.

@subsection Ui-AbstractLayer-custom-clip Taking clip rectangles into account

If the layer may get used within nodes that have @ref NodeFlag::Clip enabled,
such as various scroll areas, it should respect clip rectangles when drawing.
The `clipRectOffsets` and `clipRectSizes` views passed to @ref doUpdate()
describe clip rectangle placement, `clipRectIds` and `clipRectDataCounts` then
specify which of the rectangles is used for which subrange of `dataIds`.
There's always at least one clip rectangle present and the sum of
`clipRectDataCounts` is equal to size of `dataIds`.

As with node opacity and disabled state above, the actual implementation is
entirely up to the layer itself. One option is to apply the clip rectangles
directly to the vertex data, in this case performing an intersection of the
quad with the clip rectangle using @ref Math::intersect():

@snippet Ui-gl.cpp AbstractLayer-custom-clip

@subsection Ui-AbstractLayer-custom-clip-scissor Clipping using GPU scissor rectangles

It's not always possible or efficient to clip the vertex data directly. In such
cases it's possible to make use of scissor rectangles instead. Similarly as
with blending, the layer advertises @ref Ui::LayerFeature::DrawUsesScissor in
@ref doFeatures() to make the @ref AbstractRenderer enable scissor state when
drawing the layer. Clip rectangles are specified in framebuffer coordinates,
thus @ref doSetSize() now needs to remember both the UI and the framebuffer
size:

@snippet Ui-gl.cpp AbstractLayer-custom-clip-scissor

And then, instead of clipping inside @ref doUpdate(), the @ref doDraw()
function performs not just a single draw, but one for each clip rectangle. To
match OpenGL framebuffer coordinates that have origin bottom left and Y up, the
clip rectangles get scaled, Y-flipped and converted to integers.

@snippet Ui-gl.cpp AbstractLayer-custom-clip-scissor-draw

You can assume that the list of clip rectangles is made in a way that minimizes
the amount of extra draw calls this approach needs compared to culling
CPU-side.

@subsection Ui-AbstractLayer-custom-setters Setters and triggering data updates

So far, all updates and drawing happened only in a response to the node
hierarchy changing in some way --- nodes changing place, visibility, or data
being attached / detached. Combined with the user interface
@ref Ui-AbstractUserInterface-redraw-on-demand "only redrawing when needed" it
means that any updates to the layer data, such as changing the color of a
particular quad, need to notify the UI that an update and redraw is needed.
This is done with @ref setNeedsUpdate(). A color setter would thus look like
this:

@snippet Ui-gl.cpp AbstractLayer-custom-setters

As the snippet shows, it's a good practice to check for handle validity, to
ensure stale handles don't accidentally change unrelated data. All builtin APIs
have those checks, but in this case we're directly accessing our own data array
and thus nothing else can check the validity for us. Additionally it makes
sense to provide also a @ref LayerDataHandle overload so the setter can be
called with the layer-specific handle type as well:

@snippet Ui-gl.cpp AbstractLayer-custom-setters-layerdatahandle

@subsection Ui-AbstractLayer-custom-update-in-data-order Populating the vertex data in data order instead of draw order

So far, the @ref doUpdate() function populated the mesh with vertex data in
order as drawn in that particular frame. While that makes the mesh always
contain only exactly what's needed, it means reuploading also data that didn't
change, such as the quad colors in our `QuadLayer`, just in a slightly
different order.

Putting aside node opacity and disabled state for now, an alternative approach
could be to fill the vertex colors directly in @cpp create() @ce and
@cpp setColor() @ce instead of managing a CPU-side `_colors` array and then
copying from it. The @ref doUpdate() would then map the vertex buffer, update
just the positions in it, and only the index buffer gets fully regenerated
every time. We can use @ref capacity() to size the buffer mapping, as it's an
upper bound for all data IDs:

@snippet Ui-gl.cpp AbstractLayer-custom-update-in-data-order

@subsection Ui-AbstractLayer-custom-update-states Partial updates

The first argument to @ref doUpdate() is a set of @ref LayerState bits, which
enumerates the reasons why an update needs to done. For example, in given frame
only @ref LayerState::NeedsNodeOrderUpdate could be set due to a popup being
brought to the front, but node positions and everything else would stay
unchanged. Or it could be just @ref LayerState::NeedsDataUpdate being triggered
through @ref setNeedsUpdate() from a setter, but the node hierarchy stays the
same.

The @ref doUpdate() implementation can make use of these states to perform just
partial updates. This makes sense especially in the above case where the draw
data are stored in a way that's independent from the actual draw order, because
otherwise even changes in node visibility would require rebuilding all data in
the new order. Note that there are various interactions between the states, see
particular @ref LayerState values for more information.

@snippet Ui-gl.cpp AbstractLayer-custom-update-states

@subsection Ui-AbstractLayer-custom-update-states-common Explicitly and implicitly triggered updates

In addition to @ref LayerState::NeedsDataUpdate, the @ref setNeedsUpdate()
function can be called also with @relativeref{LayerState,NeedsCommonDataUpdate}
and @relativeref{LayerState,NeedsSharedDataUpdate}. These two states are never
triggered by the UI library but are meant to be used by the layer itself, to
denote a need for updates that aren't tied to any concrete data ID.

For example, in the @ref Ui-AbstractLayer-custom-update "original case of data being stored in draw order"
the index buffer is always the same and only needs to be updated if it isn't
large enough. For that, the @cpp create() @ce implementation would call
@ref setNeedsUpdate() with @ref LayerState::NeedsCommonDataUpdate if the buffer needs to be enlarged, which then gets done in @ref doUpdate().

@snippet Ui-gl.cpp AbstractLayer-custom-update-states-common

It's of course possible to perform the index buffer upload directly in
@cpp create() @ce as well, but when creating a lot of data the buffer could get
reuploaded several times over. Deferring the update like this makes it updated
at most once per frame.

The @ref LayerState::NeedsSharedDataUpdate is then for differentiating updates
of data that may be shared among multiple layers. This is what for example
@ref BaseLayer uses to trigger updates of style data, which are stored in
@ref BaseLayer::Shared, and for which it's enough to be updated just once for
all layers that use the same shared instance.

Finally, it might not always be possible to have @ref setNeedsUpdate() called
in order to trigger an update. One such case is when the layer *polls* data
from an external source, and there's no way for the source to explicitly notify
the layer about updates. To solve this, the layer can implement the
@ref doState() interface and return @ref LayerState::NeedsDataUpdate or similar
in case an update was detected. A common pattern is for the external source to
have some sort of a last update timestamp, or even just a counter that gets
incremented on every update. The layer then compares its copy against it in
@ref doState(), and refreshes the saved timestamp in @ref doUpdate():

@snippet Ui-gl.cpp AbstractLayer-custom-update-states-timestamp

@subsection Ui-AbstractLayer-custom-resource-cleanup Resource cleanup on data removal

Besides plain data, it's possible for layers to store heavier resources. As an
example, let's assume the quads are textured, with each such quad using a
dedicated @ref GL::Texture2D. Putting aside the obvious downsides like each
quad needing a separate draw call, we should ensure that the textures don't
just stay in GPU memory after the data are removed. We hande that explicitly in
the @ref remove() overrides:

@snippet Ui-gl.cpp AbstractLayer-custom-resource-cleanup-remove

More commonly however, instead of users explicitly calling @ref remove(), the
data get removed as a consequence of node hierarchy removal. For that there's
the @ref doClean() interface, which gets called as part of the regular update
if any cascaded removes happened since last time. It gets a bitmask marking
which data IDs got removed, which we use to perform a cleanup:

@snippet Ui-gl.cpp AbstractLayer-custom-resource-cleanup-clean

The @ref doClean() interface is designed like this instead of something like
calling @ref remove() in a loop in order to allow implementations to batch the
operations. For example, if the textures would be instead in some sort of an
atlas that needs repacking afterwards, doing it just once for all removed
textures would be more efficient than repacking after each removal.

@section Ui-AbstractLayer-custom-event Custom event handling layers

While the builtin @ref EventLayer allows attaching callbacks to various
high-level events like taps, clicks or pinch gestures, you may need specialized
behavior that can only be implemented by directly accessing the low-level event
interfaces in a custom layer. Another use case is implementing event handling
in addition to drawing, for example to implicitly handle hover and pressed
state. While it could be done externally with @ref EventLayer::onEnter(),
@relativeref{EventLayer,onLeave()} and such, implementing it directly on the
custom drawing layer is often simpler and makes the layer more self-contained.

@m_class{m-note m-info}

@par
    The high-level event input, handling and propagation is described in
    @ref Ui-AbstractUserInterface-events "AbstractUserInterface event handling docs",
    be sure to read that part first if you haven't already.

A layer that wants to handle events advertises @ref LayerFeature::Event in
@ref doFeatures() and implements one or more of the `do*Event()` interfaces,
overview of which is below. As with drawing, an event handling layer doesn't
* *need* to implement @ref doUpdate() or @ref doClean() --- if it has nothing
to do on a per-data basis, it can contain only event handlers alone.

@subsection Ui-AbstractLayer-custom-event-hover Reacting to hover

For a simple introductory example, let's make the `QuadLayer` react to hover by
making given quad brighter. The @ref doPointerMoveEvent() is called in a
response to a pointer moving over area of a node that given `dataId` is
attached to. With @ref PointerMoveEvent::setAccepted() we let the UI know that
it's handling the event and the event shouldn't get propagated to further
nodes. Here we want to handle a hover on all quads, so we call it regardless of
the data ID the event happens on, but the layer can implement any behavior it
wants. We however restrict it to just primary events, i.e. we don't want
secondary fingers in a multi-touch event to highlight the node:

@snippet Ui.cpp AbstractLayer-custom-event-hover

If the move event gets accepted and the node wasn't hovered already,
@ref doPointerEnterEvent() gets called next. Once the pointer moves outside of
the node area, @ref doPointerLeaveEvent() gets called. We'll simply brighten
the color in one and darken again in the other, and call @ref setNeedsUpdate()
to let the UI know that we need an update and a redraw. Compared to
@ref doPointerMoveEvent(), calling @relativeref{PointerMoveEvent,setAccepted()}
isn't needed, as the enter and leave events don't propagate anywhere if not
handled.

@snippet Ui.cpp AbstractLayer-custom-event-hover-enter-leave

The pair of enter / leave events deals with the common case of a pointer moving
across the user interface, but it can also happen that the currently hovered
nodes stops being visible or for example gets disabled, and at that point it
should no longer show a hover state. We'll get notified about that and other
cases in @ref doVisibilityLostEvent(), @ref VisibilityLostEvent::isNodeHovered()
tells us if the node was hovered before and we should thus darken again:

@m_class{m-console-wrap}

@snippet Ui.cpp AbstractLayer-custom-event-hover-visibility-lost

@subsection Ui-AbstractLayer-custom-event-press-release Pointer press and release, pointer capture

The @ref doPointerPressEvent() and @ref doPointerReleaseEvent() interfaces are
called when a pointer is pressed and released on a node. Compared to the
@ref Ui-EventLayer-tap-click-press-release "EventLayer tap or click handler",
there isn't any high-level tap or click event on the @ref AbstractLayer itself,
that's up to the layer to implement if needed. The @ref PointerEvent exposes
various state for this, allowing you to decide what a click should actually be,
such as taking into distance between a press and a release, time between the
events, pointers being pressed etc.

Additionally, @ref Ui-AbstractUserInterface-events-capture "pointer capture" is
implicitly enabled, meaning that all events following a press get sent to the
originating node even if the pointer leaves its area. Furthermore, the event
handlers can toggle the capture at any time, which can be used to implement
advanced functionality.

For example, we can use a press and a drag of a (primary mouse, pen or touch)
pointer to change the color brightness, but dragging more than a certain
distance will cancel the whole action, reverting back to the original color:

@snippet Ui.cpp AbstractLayer-custom-event-drag-capture

Finally, @ref doScrollEvent() handles scroll wheel and trackpad input. It's
affected by pointer capture as well but there isn't anything specific to wheel
events that would need a dedicated example.

@m_class{m-note m-info}

@par
    See @ref AbstractUserInterface::pointerPressEvent(),
    @relativeref{AbstractUserInterface,pointerReleaseEvent()},
    @relativeref{AbstractUserInterface,pointerMoveEvent()} and
    @relativeref{AbstractUserInterface,scrollEvent()} for a detailed
    description of how pointer and scroll events are propagated to concrete
    nodes and how pointer capture is handled.

@subsection Ui-AbstractLayer-custom-event-keyboard Key events, node focus and text input

The @ref doKeyPressEvent() and @ref doKeyReleaseEvent() interfaces are called
in response to keyboard input. By default, if no node is focused, they're
delivered the same way as pointer events, i.e. to a node under pointer or to
the currently captured node. As an example, we could react to
@m_class{m-label m-default} **R** being pressed to reset a color to some
default. Important to note is that we check for @ref KeyEvent::modifiers() to
be empty to not also react to @m_class{m-label m-warning} **Ctrl**
@m_class{m-label m-default} **R** and such:

@snippet Ui.cpp AbstractLayer-custom-event-keyboard

Nodes that have @ref NodeFlag::Focusable enabled can be focused, after which
they receive all key input as well as text input in @ref doTextInputEvent()
regardless of pointer location or capture. Focusing is usually done in a
response to an accepted pointer press, i.e. @ref doPointerPressEvent() has to
accept a press first, and then a @ref doFocusEvent() allows the node to decide
about the focus. When the node loses focus again, @ref doBlurEvent() is called.
Besides a pointer press, @ref AbstractUserInterface::focusEvent() can be
used to programmatically focus a node, which results in @ref doFocusEvent() /
@ref doBlurEvent() being called directly.

For a practical example, let's say we want to be able to type out a hexadecimal
color to change a color of a focused quad --- assuming it was attached to a
@ref NodeFlag::Focusable node. For simplicity, as soon as all six letters are
typed, the color is changed and input starts over:

@snippet Ui.cpp AbstractLayer-custom-event-text

Note that @ref TextInputEvent::text() is allowed to contain more than one byte.
If text input is active, regular key events still get sent as well, to allow
the layer to perform other operations with them if needed, such as a backspace
here. The @ref TextLayer implements more advanced text editing capabilities on
top of these events, see the @ref TextEdit enum and @ref TextLayer::editText()
for an overview of common editing behavior mapped to keyboard shortcuts.

@m_class{m-note m-info}

@par
    See @ref AbstractUserInterface::keyPressEvent(),
    @relativeref{AbstractUserInterface,keyReleaseEvent()} and
    @relativeref{AbstractUserInterface,textInputEvent()} for a detailed
    description of how key and text input events get propagated. Focus behavior
    is described in @relativeref{AbstractUserInterface,pointerPressEvent()} and
    @relativeref{AbstractUserInterface,focusEvent()}.
*/
class MAGNUM_UI_EXPORT AbstractLayer {
    public:
        #ifdef DOXYGEN_GENERATING_OUTPUT
        class DebugIntegration; /* For documentation only */
        #endif

        /**
         * @brief Constructor
         * @param handle    Handle returned by
         *      @ref AbstractUserInterface::createLayer()
         */
        explicit AbstractLayer(LayerHandle handle);

        /** @brief Copying is not allowed */
        AbstractLayer(const AbstractLayer&) = delete;

        /**
         * @brief Move constructor
         *
         * Performs a destructive move, i.e. the original object isn't usable
         * afterwards anymore.
         */
        AbstractLayer(AbstractLayer&&) noexcept;

        virtual ~AbstractLayer();

        /** @brief Copying is not allowed */
        AbstractLayer& operator=(const AbstractLayer&) = delete;

        /** @brief Move assignment */
        AbstractLayer& operator=(AbstractLayer&&) noexcept;

        /**
         * @brief Layer handle
         *
         * Returns the handle passed to the constructor.
         */
        LayerHandle handle() const;

        /** @brief Features exposed by a layer */
        LayerFeatures features() const { return doFeatures(); }

        /**
         * @brief Layer state
         *
         * See the @ref LayerState enum for more information. By default no
         * flags are set.
         */
        LayerStates state() const;

        /**
         * @brief Mark the layer as needing an update
         *
         * Meant to be called by layer implementations when the data get
         * modified. Expects that @p state is a non-empty subset of
         * @ref LayerState::NeedsDataUpdate,
         * @relativeref{LayerState,NeedsCommonDataUpdate},
         * @relativeref{LayerState,NeedsSharedDataUpdate}, and if the layer
         * advertises @ref LayerFeature::Composite, also
         * @ref LayerState::NeedsCompositeOffsetSizeUpdate. See the flags for
         * more information.
         * @see @ref state(), @ref update()
         */
        void setNeedsUpdate(LayerStates state);

        /**
         * @brief Current capacity of the data storage
         *
         * Can be at most 1048576. If @ref create() is called and there's no
         * free slots left, the internal storage gets grown.
         * @see @ref usedCount()
         */
        std::size_t capacity() const;

        /**
         * @brief Count of used items in the data storage
         *
         * Always at most @ref capacity(). Expired handles are counted among
         * used as well. The operation is done with a @f$ \mathcal{O}(n) @f$
         * complexity where @f$ n @f$ is @ref capacity().
         */
        std::size_t usedCount() const;

        /**
         * @brief Whether a data handle is valid
         *
         * A handle is valid if it has been returned from @ref create() before
         * and @ref remove() wasn't called on it yet. For
         * @ref LayerDataHandle::Null always returns @cpp false @ce.
         */
        bool isHandleValid(LayerDataHandle handle) const;

        /**
         * @brief Whether a data handle is valid
         *
         * A shorthand for extracting a @ref LayerHandle from @p handle using
         * @ref dataHandleLayer(), comparing it to @ref handle() and if it's
         * the same, calling @ref isHandleValid(LayerDataHandle) const with a
         * @ref LayerDataHandle extracted from @p handle using
         * @ref dataHandleData(). See these functions for more information. For
         * @ref DataHandle::Null, @ref LayerHandle::Null or
         * @ref LayerDataHandle::Null always returns @cpp false @ce.
         */
        bool isHandleValid(DataHandle handle) const;

        /**
         * @brief Attach data to a node
         *
         * Makes the @p data handle tied to a particular @p node, meaning it
         * gets included in draw or event processing depending on node position
         * and visibility. Also, @ref AbstractUserInterface::removeNode()
         * called for @p node or any parent node will then mean that the
         * @p data gets scheduled for removal during the next @ref cleanNodes()
         * call.
         *
         * Expects that @p data is valid. The @p node can be anything including
         * @ref NodeHandle::Null, but if it's non-null and not valid the data
         * will be scheduled for deletion during the next @ref cleanNodes()
         * call. If the @p data is already attached to some node, this will
         * overwrite the previous attachment --- i.e., it's not possible to
         * have the same data attached to multiple nodes. The inverse,
         * attaching multiple different data handles to a single node, is
         * supported however.
         *
         * If @p data wasn't attached to @p node before, calling this function
         * causes @ref LayerState::NeedsAttachmentUpdate to be set.
         * Additionally, if @p node isn't @ref NodeHandle::Null,
         * @ref LayerState::NeedsNodeOffsetSizeUpdate is set as well.
         * @see @ref isHandleValid(DataHandle) const, @ref create(),
         *      @ref AbstractUserInterface::attachData()
         */
        void attach(DataHandle data, NodeHandle node);

        /**
         * @brief Attach data to a node assuming it belongs to this layer
         *
         * Like @ref attach(DataHandle, NodeHandle) but without checking that
         * @p data indeed belongs to this layer. See its documentation for more
         * information.
         */
        void attach(LayerDataHandle data, NodeHandle node);

        /**
         * @brief Node attachment for given data
         *
         * Expects that @p data is valid. If given data isn't attached to any
         * node, returns @ref NodeHandle::Null. See also
         * @ref node(LayerDataHandle) const which is a simpler operation if the
         * data is already known to belong to this layer.
         *
         * The returned handle may be invalid if either the data got attached
         * to an invalid node in the first place or the node or any of its
         * parents were removed and @ref AbstractUserInterface::clean() wasn't
         * called since.
         * @see @ref isHandleValid(DataHandle) const
         */
        NodeHandle node(DataHandle data) const;

        /**
         * @brief Node attachment for given data assuming it belongs to this layer
         *
         * Like @ref node(DataHandle) const but without checking that @p data
         * indeed belongs to this layer. See its documentation for more
         * information.
         * @see @ref isHandleValid(LayerDataHandle) const,
         *      @ref dataHandleData()
         */
        NodeHandle node(LayerDataHandle data) const;

        /**
         * @brief Node attachments for all data
         *
         * Meant to be used by layer implementations to query node attachments
         * based on data IDs or masks without knowing their full handles,
         * application code should use @ref node(DataHandle) const /
         * @ref node(LayerDataHandle) const instead. Size of the returned view
         * is the same as @ref capacity(). Items that are @ref NodeHandle::Null
         * are either data with no node attachments or corresponding to data
         * that are freed.
         */
        Containers::StridedArrayView1D<const NodeHandle> nodes() const;

        /**
         * @brief Generation counters for all data
         *
         * Meant to be used by code that only gets data IDs or masks but needs
         * the full handle, or for various diagnostic purposes such as tracking
         * handle recycling. Size of the returned view is the same as
         * @ref capacity(), individual items correspond to generations of
         * particular data IDs. All values fit into the @ref DataHandle /
         * @ref LayerDataHandle generation bits, @cpp 0 @ce denotes an expired
         * generation counter.
         *
         * Passing an ID along with the corresponding generation to
         * @ref layerDataHandle() produces a @ref LayerDataHandle, passing that
         * along with @ref handle() to @ref dataHandle() produces a
         * @ref DataHandle. Use @ref isHandleValid(LayerDataHandle) const /
         * @ref isHandleValid(DataHandle) const to determine whether given slot
         * is actually used.
         */
        Containers::StridedArrayView1D<const UnsignedShort> generations() const;

        /**
         * @brief Set user interface size
         *
         * Used internally from @ref AbstractUserInterface::setSize() and
         * @ref AbstractUserInterface::setLayerInstance(). Exposed just for
         * testing purposes, there should be no need to call this function
         * directly. Expects that the layer supports @ref LayerFeature::Draw
         * and that both sizes are non-zero. Delegates to @ref doSetSize(), see
         * its documentation for more information about the arguments.
         */
        void setSize(const Vector2& size, const Vector2i& framebufferSize);

        /**
         * @brief Clean data attached to no longer valid nodes
         *
         * Used internally from @ref AbstractUserInterface::clean(). Exposed
         * just for testing purposes, there should be no need to call this
         * function directly and doing so may cause internal
         * @ref AbstractUserInterface state update to misbehave. Assumes that
         * @p nodeHandleGenerations contains handle generation counters for all
         * nodes, where the index is implicitly the handle ID. They're used to
         * decide about node attachment validity, data with invalid node
         * attachments are then removed. Delegates to @ref doClean(), see its
         * documentation for more information about the arguments.
         */
        void cleanNodes(const Containers::StridedArrayView1D<const UnsignedShort>& nodeHandleGenerations);

        /**
         * @brief Clean animations attached to no longer valid data
         *
         * Used internally from @ref AbstractUserInterface::clean(). Exposed
         * just for testing purposes, there should be no need to call this
         * function directly and doing so may cause internal
         * @ref AbstractUserInterface state update to misbehave. Expects that
         * all @p animators expose @ref AnimatorFeature::DataAttachment and
         * their @ref AbstractAnimator::layer() matches @ref handle(), and
         * assumes that all such animators are passed together in a single
         * call. Delegates to @ref AbstractAnimator::cleanData() for every
         * animator, see its documentation for more information.
         *
         * Calling this function resets @ref LayerState::NeedsDataClean,
         * however note that behavior of this function is independent of
         * @ref state() --- it performs the clean always regardless of what
         * flags are set.
         */
        void cleanData(const Containers::Iterable<AbstractAnimator>& animators);

        /**
         * @brief Advance data animations in animators assigned to this layer
         *
         * Used internally from @ref AbstractUserInterface::advanceAnimations().
         * Exposed just for testing purposes, there should be no need to call
         * this function directly and doing so may cause internal
         * @ref AbstractUserInterface state update to misbehave.
         *
         * Expects that @p activeStorage, @p startedStorage, @p stoppedStorage,
         * @p factorStorage and @p removeStorage have the same size, which is
         * at least as large as the largest capacity of all @p animators, and
         * that all @p animators expose @ref AnimatorFeature::DataAttachment
         * and their @ref AbstractAnimator::layer() matches @ref handle(), in
         * other words that they were passed to
         * @ref assignAnimator(AbstractDataAnimator&) const earlier. Delegates
         * to @ref doAdvanceAnimations(Nanoseconds, Containers::MutableBitArrayView, Containers::MutableBitArrayView, Containers::MutableBitArrayView, const Containers::StridedArrayView1D<Float>&, Containers::MutableBitArrayView, const Containers::Iterable<AbstractDataAnimator>&),
         * see its documentation for more information.
         */
        void advanceAnimations(Nanoseconds time, Containers::MutableBitArrayView activeStorage, Containers::MutableBitArrayView startedStorage, Containers::MutableBitArrayView stoppedStorage, const Containers::StridedArrayView1D<Float>& factorStorage, Containers::MutableBitArrayView removeStorage, const Containers::Iterable<AbstractDataAnimator>& animators);

        /**
         * @brief Advance style animations in animators assigned to this layer
         *
         * Used internally from @ref AbstractUserInterface::advanceAnimations().
         * Exposed just for testing purposes, there should be no need to call
         * this function directly and doing so may cause internal
         * @ref AbstractUserInterface state update to misbehave.
         *
         * Expects that @p activeStorage, @p startedStorage, @p stoppedStorage,
         * @p factorStorage and @p removeStorage have the same size, which is
         * at least as large as the largest capacity of all @p animators, and
         * that all @p animators expose @ref AnimatorFeature::DataAttachment
         * and their @ref AbstractAnimator::layer() matches @ref handle(), in
         * other words that they were passed to
         * @ref assignAnimator(AbstractStyleAnimator&) const earlier. Delegates
         * to @ref doAdvanceAnimations(Nanoseconds, Containers::MutableBitArrayView, Containers::MutableBitArrayView, Containers::MutableBitArrayView, const Containers::StridedArrayView1D<Float>&, Containers::MutableBitArrayView, const Containers::Iterable<AbstractStyleAnimator>&),
         * see its documentation for more information.
         */
        void advanceAnimations(Nanoseconds time, Containers::MutableBitArrayView activeStorage, Containers::MutableBitArrayView startedStorage, Containers::MutableBitArrayView stoppedStorage, const Containers::StridedArrayView1D<Float>& factorStorage, Containers::MutableBitArrayView removeStorage, const Containers::Iterable<AbstractStyleAnimator>& animators);

        /**
         * @brief Pre-update common and shared layer data
         *
         * Used internally from @ref AbstractUserInterface::update(). Exposed
         * just for testing purposes, there should be no need to call this
         * function directly and doing so may cause internal
         * @ref AbstractUserInterface state update to misbehave.
         *
         * Expects that @p states isn't empty and is a subset of
         * @ref LayerState::NeedsCommonDataUpdate and
         * @relativeref{LayerState,NeedsSharedDataUpdate}.
         *
         * Note that, unlike @ref update(), calling this function *does not*
         * reset @ref LayerStates present in @p state, that's only done once
         * @ref update() is subsequently called.
         */
        void preUpdate(LayerStates state);

        /**
         * @brief Update visible layer data to given offsets and positions
         *
         * Used internally from @ref AbstractUserInterface::update(). Exposed
         * just for testing purposes, there should be no need to call this
         * function directly and doing so may cause internal
         * @ref AbstractUserInterface state update to misbehave.
         *
         * Expects that @p states isn't empty and is a subset of
         * @ref LayerState::NeedsNodeOffsetSizeUpdate,
         * @relativeref{LayerState,NeedsNodeOrderUpdate},
         * @relativeref{LayerState,NeedsNodeEnabledUpdate},
         * @relativeref{LayerState,NeedsNodeOpacityUpdate},
         * @relativeref{LayerState,NeedsDataUpdate},
         * @relativeref{LayerState,NeedsCommonDataUpdate},
         * @relativeref{LayerState,NeedsSharedDataUpdate} and
         * @relativeref{LayerState,NeedsAttachmentUpdate}, and
         * @relativeref{LayerState,NeedsCompositeOffsetSizeUpdate} if the layer
         * advertises @ref LayerFeature::Composite, that the @p clipRectIds and
         * @p clipRectDataCounts views have the same size, @p nodeOffsets,
         * @p nodeSizes, @p nodeOpacities and @p nodesEnabled have the same
         * size, @p clipRectOffsets and @p clipRectOffset have the same size
         * and @p compositeRectOffsets and @p compositeRectSizes have the same
         * size. If @ref LayerFeature::Composite isn't supported,
         * @p compositeRectOffsets and @p compositeRectSizes are expected to
         * be empty. The @p nodeOffsets, @p nodeSizes, @p nodeOpacities and
         * @p nodesEnabled views should be large enough to contain any valid
         * node ID. If the layer advertises @ref LayerFeature::Draw, expects
         * that @ref setSize() was called at least once before this function.
         * Delegates to @ref doUpdate(), see its documentation for more
         * information about the arguments.
         *
         * Calling this function resets @ref LayerStates present in @p state,
         * however note that behavior of this function is independent of
         * @ref state() --- it performs the update only based on what's passed
         * in @p state.
         */
        void update(LayerStates state, const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, const Containers::StridedArrayView1D<const Float>& nodeOpacities, Containers::BitArrayView nodesEnabled, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes, const Containers::StridedArrayView1D<const Vector2>& compositeRectOffsets, const Containers::StridedArrayView1D<const Vector2>& compositeRectSizes);

        /**
         * @brief Composite previously rendered contents
         *
         * Used internally from @ref AbstractUserInterface::draw(). Exposed
         * just for testing purposes, there should be no need to call this
         * function directly. Expects that the layer supports
         * @ref LayerFeature::Composite, that the @p rectOffsets and
         * @p rectSizes views have the same size and that @p offset and
         * @p count fits into their size. Delegates to @ref doComposite(), see
         * its documentation for more information about the arguments.
         * @see @ref features()
         */
        void composite(AbstractRenderer& renderer, const Containers::StridedArrayView1D<const Vector2>& rectOffsets, const Containers::StridedArrayView1D<const Vector2>& rectSizes, std::size_t offset, std::size_t count);

        /**
         * @brief Draw a sub-range of visible layer data
         *
         * Used internally from @ref AbstractUserInterface::draw(). Exposed
         * just for testing purposes, there should be no need to call this
         * function directly. Expects that the layer supports
         * @ref LayerFeature::Draw, @p offset and @p count fits into @p dataIds
         * size, that the @p clipRectIds and @p clipRectDataCounts views have
         * the same size, @p nodeOffsets, @p nodeSizes, @p nodeOpacities and
         * @p nodesEnabled have the same size and @p clipRectOffsets and
         * @p clipRectOffset have the same size. The @p nodeOffsets,
         * @p nodeSizes, @p nodeOpacities and @p nodesEnabled views should be
         * large enough to contain any valid node ID. Delegates to
         * @ref doDraw(), see its documentation for more information about the
         * arguments.
         * @see @ref features()
         */
        void draw(const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, std::size_t offset, std::size_t count, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, std::size_t clipRectOffset, std::size_t clipRectCount, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, const Containers::StridedArrayView1D<const Float>& nodeOpacities, Containers::BitArrayView nodesEnabled, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes);

        /**
         * @brief Handle a pointer press event
         *
         * Used internally from @ref AbstractUserInterface::pointerPressEvent().
         * Exposed just for testing purposes, there should be no need to call
         * this function directly. Expects that the layer supports
         * @ref LayerFeature::Event and @p dataId is less than @ref capacity(),
         * with the assumption that the ID points to a valid data and
         * @ref PointerEvent::position() is relative to the node to which the
         * data is attached. The event is expected to not be accepted yet.
         * Delegates to @ref doPointerPressEvent(), see its documentation for
         * more information.
         * @see @ref PointerEvent::isAccepted(),
         *      @ref PointerEvent::setAccepted()
         */
        void pointerPressEvent(UnsignedInt dataId, PointerEvent& event);

        /**
         * @brief Handle a pointer release event
         *
         * Used internally from @ref AbstractUserInterface::pointerReleaseEvent().
         * Exposed just for testing purposes, there should be no need to call
         * this function directly. Expects that the layer supports
         * @ref LayerFeature::Event and @p dataId is less than @ref capacity(),
         * with the assumption that the ID points to a valid data and
         * @ref PointerEvent::position() is relative to the node to which the
         * data is attached. The event is expected to not be accepted yet.
         * Delegates to @ref doPointerReleaseEvent(), see its documentation for
         * more information.
         * @see @ref PointerEvent::isAccepted(),
         *      @ref PointerEvent::setAccepted()
         */
        void pointerReleaseEvent(UnsignedInt dataId, PointerEvent& event);

        /**
         * @brief Handle a pointer move event
         *
         * Used internally from @ref AbstractUserInterface::pointerMoveEvent().
         * Exposed just for testing purposes, there should be no need to call
         * this function directly. Expects that the layer supports
         * @ref LayerFeature::Event and @p dataId is less than @ref capacity(),
         * with the assumption that the ID points to a valid data and
         * @ref PointerMoveEvent::position() is relative to the node to which
         * the data is attached. The event is expected to not be accepted yet.
         * Delegates to @ref doPointerMoveEvent(), see its documentation for
         * more information.
         * @see @ref PointerMoveEvent::isAccepted(),
         *      @ref PointerMoveEvent::setAccepted()
         */
        void pointerMoveEvent(UnsignedInt dataId, PointerMoveEvent& event);

        /**
         * @brief Handle a pointer enter event
         *
         * Used internally from @ref AbstractUserInterface::pointerMoveEvent().
         * Exposed just for testing purposes, there should be no need to call
         * this function directly. Expects that the layer supports
         * @ref LayerFeature::Event and @p dataId is less than @ref capacity(),
         * with the assumption that the ID points to a valid data and
         * @ref PointerMoveEvent::position() is relative to the node to which
         * the data is attached, and @ref PointerMoveEvent::relativePosition()
         * is a zero vector, given that the event is meant to happen right
         * after another event. The event is expected to be primary and to not
         * be accepted yet. Delegates to @ref doPointerEnterEvent(), see its
         * documentation for more information.
         * @see @ref PointerMoveEvent::isPrimary(),
         *      @ref PointerMoveEvent::isAccepted(),
         *      @ref PointerMoveEvent::setAccepted()
         */
        void pointerEnterEvent(UnsignedInt dataId, PointerMoveEvent& event);

        /**
         * @brief Handle a pointer leave event
         *
         * Used internally from @ref AbstractUserInterface::pointerMoveEvent().
         * Exposed just for testing purposes, there should be no need to call
         * this function directly. Expects that the layer supports
         * @ref LayerFeature::Event and @p dataId is less than @ref capacity(),
         * with the assumption that the ID points to a valid data and
         * @ref PointerMoveEvent::position() is relative to the node to which
         * the data is attached, and @ref PointerMoveEvent::relativePosition()
         * is a zero vector, given that the event is meant to happen right
         * after another event. The event is expected to be primary and to not
         * be accepted yet. Delegates to @ref doPointerLeaveEvent(), see its
         * documentation for more information.
         * @see @ref PointerMoveEvent::isPrimary(),
         *      @ref PointerMoveEvent::isAccepted(),
         *      @ref PointerMoveEvent::setAccepted()
         */
        void pointerLeaveEvent(UnsignedInt dataId, PointerMoveEvent& event);

        /**
         * @brief Handle a pointer cancel event
         *
         * Used internally from @ref AbstractUserInterface::pointerPressEvent(),
         * @relativeref{AbstractUserInterface,pointerReleaseEvent()} and
         * @relativeref{AbstractUserInterface,pointerMoveEvent()}. Exposed just
         * for testing purposes, there should be no need to call this function
         * directly. Expects that the layer supports @ref LayerFeature::Event
         * and @p dataId is less than @ref capacity(), with the assumption that
         * the ID points to a valid data. Delegates to
         * @ref doPointerCancelEvent(), see its documentation for more
         * information.
         */
        void pointerCancelEvent(UnsignedInt dataId, PointerCancelEvent& event);

        /**
         * @brief Handle a scroll event
         *
         * Used internally from @ref AbstractUserInterface::scrollEvent().
         * Exposed just for testing purposes, there should be no need to call
         * this function directly. Expects that the layer supports
         * @ref LayerFeature::Event and @p dataId is less than @ref capacity(),
         * with the assumption that the ID points to a valid data and
         * @ref ScrollEvent::position() is relative to the node to which the
         * data is attached. The event is expected to not be accepted yet.
         * Delegates to @ref doScrollEvent(), see its documentation for more
         * information.
         * @see @ref ScrollEvent::isAccepted(),
         *      @ref ScrollEvent::setAccepted()
         */
        void scrollEvent(UnsignedInt dataId, ScrollEvent& event);

        /**
         * @brief Handle a focus event
         *
         * Used internally from @ref AbstractUserInterface::focusEvent() and
         * @ref AbstractUserInterface::pointerPressEvent(). Exposed just for
         * testing purposes, there should be no need to call this function
         * directly. Expects that the layer supports @ref LayerFeature::Event
         * and @p dataId is less than @ref capacity(), with the assumption that
         * the ID points to a valid data. The event is expected to not be
         * accepted yet. Delegates to @ref doFocusEvent(), see its
         * documentation for more information.
         * @see @ref FocusEvent::isAccepted(), @ref FocusEvent::setAccepted()
         */
        void focusEvent(UnsignedInt dataId, FocusEvent& event);

        /**
         * @brief Handle a blur event
         *
         * Used internally from @ref AbstractUserInterface::focusEvent(),
         * @ref AbstractUserInterface::pointerPressEvent() and
         * @ref AbstractUserInterface::update(). Exposed just for testing
         * purposes, there should be no need to call this function directly.
         * Expects that the layer supports @ref LayerFeature::Event and
         * @p dataId is less than @ref capacity(), with the assumption that the
         * ID points to a valid data. The event is expected to not be accepted
         * yet. Delegates to @ref doBlurEvent(), see its documentation for more
         * information.
         * @see @ref FocusEvent::isAccepted(), @ref FocusEvent::setAccepted()
         */
        void blurEvent(UnsignedInt dataId, FocusEvent& event);

        /**
         * @brief Handle a key press event
         *
         * Used internally from @ref AbstractUserInterface::keyPressEvent().
         * Exposed just for testing purposes, there should be no need to call
         * this function directly. Expects that the layer supports
         * @ref LayerFeature::Event and @p dataId is less than @ref capacity(),
         * with the assumption that the ID points to a valid data. The event is
         * expected to not be accepted yet. Delegates to
         * @ref doKeyPressEvent(), see its documentation for more information.
         * @see @ref KeyEvent::isAccepted(), @ref KeyEvent::setAccepted()
         */
        void keyPressEvent(UnsignedInt dataId, KeyEvent& event);

        /**
         * @brief Handle a key release event
         *
         * Used internally from @ref AbstractUserInterface::keyReleaseEvent().
         * Exposed just for testing purposes, there should be no need to call
         * this function directly. Expects that the layer supports
         * @ref LayerFeature::Event and @p dataId is less than @ref capacity(),
         * with the assumption that the ID points to a valid data. The event is
         * expected to not be accepted yet. Delegates to
         * @ref doKeyReleaseEvent(), see its documentation for more
         * information.
         * @see @ref KeyEvent::isAccepted(), @ref KeyEvent::setAccepted()
         */
        void keyReleaseEvent(UnsignedInt dataId, KeyEvent& event);

        /**
         * @brief Handle a text input event
         *
         * Used internally from @ref AbstractUserInterface::textInputEvent().
         * Exposed just for testing purposes, there should be no need to call
         * this function directly. Expects that the layer supports
         * @ref LayerFeature::Event and @p dataId is less than @ref capacity(),
         * with the assumption that the ID points to a valid data. The event is
         * expected to not be accepted yet. Delegates to
         * @ref doTextInputEvent(), see its documentation for more information.
         * @see @ref TextInputEvent::isAccepted(),
         *      @ref TextInputEvent::setAccepted()
         */
        void textInputEvent(UnsignedInt dataId, TextInputEvent& event);

        /**
         * @brief Handle a visibility lost event
         *
         * Used internally from @ref AbstractUserInterface::update(). Exposed
         * just for testing purposes, there should be no need to call this
         * function directly. Expects that the layer supports
         * @ref LayerFeature::Event and @p dataId is less than @ref capacity(),
         * with the assumption that the ID points to a valid data. Delegates to
         * @ref doVisibilityLostEvent(), see its documentation for more
         * information.
         */
        void visibilityLostEvent(UnsignedInt dataId, VisibilityLostEvent& event);

    protected:
        /**
         * @brief Whether the layer is a part of an user interface instance
         *
         * Returns @cpp true @ce if the layer has been already passed to
         * @ref AbstractUserInterface::setLayerInstance(), @cpp false @ce
         * otherwise. The function isn't public as it's intended to be used
         * only by the layer implementation itself, not user code.
         * @see @ref ui()
         */
        bool hasUi() const;

        /**
         * @brief User interface instance the layer is part of
         *
         * Expects that the layer has been already passed to
         * @ref AbstractUserInterface::setLayerInstance(). The function isn't
         * public as it's intended to be used only by the layer implementation
         * itself, not user code.
         * @see @ref hasUi()
         */
        AbstractUserInterface& ui();
        const AbstractUserInterface& ui() const; /**< @overload */

        /**
         * @brief Create a data
         * @param node      Node to attach to
         * @return New data handle
         *
         * Allocates a new handle in a free slot in the internal storage or
         * grows the storage if there's no free slots left. Expects that
         * there's at most 1048576 data. The returned handle can be removed
         * again with @ref remove(). If @p node is not @ref NodeHandle::Null,
         * directly attaches the created data to given node, equivalent to
         * calling @ref attach().
         *
         * Calling this function causes @ref LayerState::NeedsDataUpdate to be
         * set. If @p node is not @ref NodeHandle::Null, causes also
         * @ref LayerState::NeedsAttachmentUpdate and
         * @relativeref{LayerState,NeedsNodeOffsetSizeUpdate} to be set. The
         * subclass is meant to wrap this function in a public API and perform
         * appropriate additional initialization work there.
         */
        DataHandle create(NodeHandle node =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            NodeHandle::Null
            #else
            NodeHandle{} /* To not have to include Handle.h */
            #endif
        );

        /**
         * @brief Remove a data
         *
         * Expects that @p handle is valid. After this call,
         * @ref isHandleValid(DataHandle) const returns @cpp false @ce for
         * @p handle. See also @ref remove(LayerDataHandle) which is a simpler
         * operation if the data is already known to belong to this layer.
         *
         * Calling this function causes @ref LayerState::NeedsDataClean to be
         * set. If @p handle is attached to a node, calling this function also
         * causes @ref LayerState::NeedsAttachmentUpdate to be set. Other than
         * that, no flag is set to trigger a subsequent @ref cleanNodes() or
         * @ref update() --- instead the subclass is meant to wrap this
         * function in a public API and perform appropriate cleanup work
         * directly there.
         * @see @ref node()
         */
        void remove(DataHandle handle);

        /**
         * @brief Remove a data assuming it belongs to this layer
         *
         * Expects that @p handle is valid. After this call,
         * @ref isHandleValid(LayerDataHandle) const returns @cpp false @ce for
         * @p handle. See also @ref remove(DataHandle) which additionally
         * checks that the data belongs to this layer.
         *
         * Calling this function causes @ref LayerState::NeedsDataClean to be
         * set. If @p handle is attached to a node, calling this function also
         * causes @ref LayerState::NeedsAttachmentUpdate to be set. Other than
         * that, no flag is set to trigger a subsequent @ref cleanNodes() or
         * @ref update() --- instead the subclass is meant to wrap this
         * function in a public API and perform appropriate cleanup work
         * directly there.
         * @see @ref dataHandleData(), @ref node()
         */
        void remove(LayerDataHandle handle);

        /**
         * @brief Assign a data animator to this layer
         *
         * Expects that the layer supports @ref LayerFeature::AnimateData, the
         * animator supports @ref AnimatorFeature::DataAttachment and that
         * given @p animator wasn't passed to
         * @ref assignAnimator(AbstractDataAnimator&) const on any layer yet.
         * On the other hand, it's possible to assign multiple different
         * animators to the same layer. Saves @ref handle() into
         * @ref AbstractAnimator::layer(), making it possible to call
         * @ref AbstractAnimator::create(Nanoseconds, Nanoseconds, DataHandle, UnsignedInt, AnimationFlags),
         * @ref AbstractAnimator::create(Nanoseconds, Nanoseconds, LayerDataHandle, UnsignedInt, AnimationFlags),
         * @ref AbstractAnimator::attach(AnimationHandle, DataHandle),
         * @ref AbstractAnimator::attach(AnimationHandle, LayerDataHandle),
         * @ref AbstractAnimator::attach(AnimatorDataHandle, DataHandle) and
         * @ref AbstractAnimator::attach(AnimatorDataHandle, LayerDataHandle)
         * and pass the @p animator to @ref advanceAnimations(Nanoseconds, Containers::MutableBitArrayView, Containers::MutableBitArrayView, Containers::MutableBitArrayView, const Containers::StridedArrayView1D<Float>&, Containers::MutableBitArrayView, const Containers::Iterable<AbstractDataAnimator>&).
         *
         * A concrete layer implementation is meant to wrap this function in a
         * public API, restricting to a more concrete animator type, in order
         * to be able to safely cast back to that type in
         * @ref doAdvanceAnimations(Nanoseconds, Containers::MutableBitArrayView, Containers::MutableBitArrayView, Containers::MutableBitArrayView, const Containers::StridedArrayView1D<Float>&, Containers::MutableBitArrayView, const Containers::Iterable<AbstractDataAnimator>&).
         *
         * See @ref assignAnimator(AbstractStyleAnimator&) const for style
         * animators, a corresponding API for an @ref AbstractGenericAnimator
         * is @ref AbstractGenericAnimator::setLayer(), where the animator has
         * the control over a concrete layer type instead.
         */
        void assignAnimator(AbstractDataAnimator& animator) const;

        /**
         * @brief Assign a style animator to this layer
         *
         * Expects that the layer supports @ref LayerFeature::AnimateStyles,
         * the animator supports @ref AnimatorFeature::DataAttachment and that
         * given @p animator wasn't passed to
         * @ref assignAnimator(AbstractStyleAnimator&) const on any layer yet.
         * On the other hand, it's possible to assign multiple different
         * animators to the same layer. Saves @ref handle() into
         * @ref AbstractAnimator::layer(), making it possible to call
         * @ref AbstractAnimator::create(Nanoseconds, Nanoseconds, DataHandle, UnsignedInt, AnimationFlags),
         * @ref AbstractAnimator::create(Nanoseconds, Nanoseconds, LayerDataHandle, UnsignedInt, AnimationFlags),
         * @ref AbstractAnimator::attach(AnimationHandle, DataHandle),
         * @ref AbstractAnimator::attach(AnimationHandle, LayerDataHandle),
         * @ref AbstractAnimator::attach(AnimatorDataHandle, DataHandle) and
         * @ref AbstractAnimator::attach(AnimatorDataHandle, LayerDataHandle)
         * and pass the @p animator to @ref advanceAnimations(Nanoseconds, Containers::MutableBitArrayView, Containers::MutableBitArrayView, Containers::MutableBitArrayView, const Containers::StridedArrayView1D<Float>&, Containers::MutableBitArrayView, const Containers::Iterable<AbstractStyleAnimator>&).
         *
         * A concrete layer implementation is meant to wrap this function in a
         * public API, restricting to a more concrete animator type, in order
         * to be able to safely cast back to that type in
         * @ref doAdvanceAnimations(Nanoseconds, Containers::MutableBitArrayView, Containers::MutableBitArrayView, Containers::MutableBitArrayView, const Containers::StridedArrayView1D<Float>&, Containers::MutableBitArrayView, const Containers::Iterable<AbstractStyleAnimator>&).
         **
         * See @ref assignAnimator(AbstractDataAnimator&) const for data
         * animators, a corresponding API for an @ref AbstractGenericAnimator
         * is @ref AbstractGenericAnimator::setLayer(), where the animator has
         * the control over a concrete layer type instead.
         */
        void assignAnimator(AbstractStyleAnimator& animator) const;

    private:
        friend AbstractUserInterface; /* for the ui() reference */

        /**
         * @brief Implementation for @ref features()
         *
         * Note that the value returned by this function is assumed to stay
         * constant during the whole layer lifetime.
         */
        virtual LayerFeatures doFeatures() const = 0;

        /**
         * @brief Query layer state
         *
         * Called by @ref state() to retrieve additional state bits that might
         * have changed without layer's direct involvement, such as data shared
         * between multiple layers getting modified by another layer. The
         * implementation is expected to return a subset of
         * @ref LayerState::NeedsDataUpdate,
         * @relativeref{LayerState,NeedsCommonDataUpdate} and
         * @relativeref{LayerState,NeedsSharedDataUpdate}, and if the layer
         * advertises @ref LayerFeature::Composite, also
         * @ref LayerState::NeedsCompositeOffsetSizeUpdate.
         *
         * Default implementation returns an empty set.
         */
        virtual LayerStates doState() const;

        /**
         * @brief Set user interface size
         * @param size              Size of the user interface to which
         *      everything including events is positioned
         * @param framebufferSize   Size of the window framebuffer
         *
         * Implementation for @ref setSize(), which is called from
         * @ref AbstractUserInterface::setSize() whenever the UI size or the
         * framebuffer size changes, and from
         * @ref AbstractUserInterface::setLayerInstance() if the UI size is
         * already set at the time the function is called. Called only if
         * @ref LayerFeature::Draw is supported. The implementation is expected
         * to update its internal rendering state such as projection matrices
         * and other framebuffer-related state. If any nodes are already
         * created, the implementation can expect a followup @ref doUpdate()
         * call with up-to-date node positions and sizes.
         *
         * Default implementation does nothing.
         */
        virtual void doSetSize(const Vector2& size, const Vector2i& framebufferSize);

        /**
         * @brief Clean no longer valid layer data
         * @param dataIdsToRemove   Data IDs to remove
         *
         * Implementation for @ref cleanNodes(), which is called from
         * @ref AbstractUserInterface::clean() (and transitively from
         * @ref AbstractUserInterface::update()) whenever
         * @ref UserInterfaceState::NeedsNodeClean or any of the states that
         * imply it are present in @ref AbstractUserInterface::state().
         *
         * The @p dataIdsToRemove view has the same size as @ref capacity() and
         * is guaranteed to have bits set only for valid data IDs, i.e. data
         * IDs that are already removed are not set.
         *
         * This function may get also called with @p dataIdsToRemove having all
         * bits zero.
         *
         * Default implementation does nothing.
         */
        virtual void doClean(Containers::BitArrayView dataIdsToRemove);

        /**
         * @brief Advance data animations in animators assigned to this layer
         * @param[in] time                  Time to which to advance
         * @param[in,out] activeStorage     Storage for animators to put a mask
         *      of active animations into
         * @param[in,out] startedStorage    Storage for animators to put a mask
         *      of started animations into
         * @param[in,out] stoppedStorage    Storage for animators to put a mask
         *      of stopped animations into
         * @param[in,out] factorStorage     Storage for animators to put
         *      animation interpolation factors into
         * @param[in,out] removeStorage     Storage for animators to put a mask
         *      of animations to remove into
         * @param[in] animators             Animators to advance
         *
         * Implementation for @ref advanceAnimations(), which is called from
         * @ref AbstractUserInterface::advanceAnimations() whenever
         * @ref UserInterfaceState::NeedsAnimationAdvance is present in
         * @ref AbstractUserInterface::state() and there are animators
         * assigned to given layer.
         *
         * The @p activeStorage, @p startedStorage, @p stoppedStorage,
         * @p factorStorage and @p removeStorage views are guaranteed to be at
         * least as large as the largest capacity of all @p animators. The
         * @p animators are all guaranteed to support
         * @ref AnimatorFeature::DataAttachment, with their
         * @ref AbstractAnimator::layer() matching @ref handle(), in other
         * words that they were passed to
         * @ref assignAnimator(AbstractDataAnimator&) const earlier.
         *
         * For each animator in @p animators the implementation is expected to
         * call @ref AbstractAnimator::update() to fill a correctly-sized slice
         * of @p activeStorage, @p factorStorage and @p removeStorage; then, if
         * the returned value says advance is needed, pass the slices of
         * @p activeStorage, @p factorStorage and @p removeStorage to the
         * animator-specific advance function; and then, if the returned value
         * says clean is needed, pass the slice of @p removeStorage to
         * @ref AbstractAnimator::clean().
         *
         * Assuming the layer implementation publicizes
         * @ref assignAnimator(AbstractDataAnimator&) const with a restricted
         * type, the animators can then be safely cast back to that type in
         * order to call a concrete layer-specific advance function.
         */
        virtual void doAdvanceAnimations(Nanoseconds time, Containers::MutableBitArrayView activeStorage, Containers::MutableBitArrayView startedStorage, Containers::MutableBitArrayView stoppedStorage, const Containers::StridedArrayView1D<Float>& factorStorage, Containers::MutableBitArrayView removeStorage, const Containers::Iterable<AbstractDataAnimator>& animators);

        /**
         * @brief Advance style animations in animators assigned to this layer
         * @param[in] time                  Time to which to advance
         * @param[in,out] activeStorage     Storage for animators to put a mask
         *      of active animations into
         * @param[in,out] startedStorage    Storage for animators to put a mask
         *      of started animations into
         * @param[in,out] stoppedStorage    Storage for animators to put a mask
         *      of stopped animations into
         * @param[in,out] factorStorage     Storage for animators to put
         *      animation interpolation factors into
         * @param[in,out] removeStorage     Storage for animators to put a mask
         *      of animations to remove into
         * @param[in] animators             Animators to advance
         *
         * Implementation for @ref advanceAnimations(), which is called from
         * @ref AbstractUserInterface::advanceAnimations() whenever
         * @ref UserInterfaceState::NeedsAnimationAdvance is present in
         * @ref AbstractUserInterface::state() and there are animators
         * assigned to given layer.
         *
         * The @p activeStorage, @p startedStorage, @p stoppedStorage,
         * @p factorStorage and @p removeStorage views are guaranteed to be at
         * least as large as the largest capacity of all @p animators. The
         * @p animators are all guaranteed to support
         * @ref AnimatorFeature::DataAttachment, with their
         * @ref AbstractAnimator::layer() matching @ref handle(), in other
         * words that they were passed to
         * @ref assignAnimator(AbstractStyleAnimator&) const earlier.
         *
         * For each animator in @p animators the implementation is expected to
         * call @ref AbstractAnimator::update() to fill a correctly-sized slice
         * of @p activeStorage, @p factorStorage and @p removeStorage; then, if
         * the returned value says advance is needed, pass the slices of
         * @p activeStorage, @p factorStorage and @p removeStorage to the
         * animator-specific advance function; and then, if the returned value
         * says clean is needed, pass the slice of @p removeStorage to
         * @ref AbstractAnimator::clean().
         *
         * Assuming the layer implementation publicizes
         * @ref assignAnimator(AbstractStyleAnimator&) const with a restricted
         * type, the animators can then be safely cast back to that type in
         * order to call a concrete layer-specific advance function.
         */
        virtual void doAdvanceAnimations(Nanoseconds time, Containers::MutableBitArrayView activeStorage, Containers::MutableBitArrayView startedStorage, Containers::MutableBitArrayView stoppedStorage, const Containers::StridedArrayView1D<Float>& factorStorage, Containers::MutableBitArrayView removeStorage, const Containers::Iterable<AbstractStyleAnimator>& animators);

        /**
         * @brief Pre-update common and shared layer data
         * @param state             State that's needed to be updated
         *
         * Implementation for @ref preUpdate(), which is called from
         * @ref AbstractUserInterface whenever
         * @ref UserInterfaceState::NeedsDataUpdate or any of the global or
         * layer-specific states that imply it are present in
         * @ref AbstractUserInterface::state(). Is always called after
         * @ref doClean() and before @ref doUpdate(), @ref doComposite() and
         * @ref doDraw(), with at least one @ref doSetSize() call happening at
         * some point before. In case the function performs operations that
         * trigger @ref UserInterfaceState::NeedsDataClean, another
         * @ref doClean() is executed right after calling this function. This
         * function is always called before any internal operations that
         * calculate the set of visible nodes and data, meaning this function
         * can be used not just for updating common and shared layer data, but
         * also for example for data addition or removal.
         *
         * The @p state is guaranteed to be a subset of
         * @ref LayerState::NeedsCommonDataUpdate and
         * @relativeref{LayerState,NeedsSharedDataUpdate} and the
         * implementation can make use of this information to skip some of its
         * internal update logic. It can however also update everything always.
         * Note that the @ref doUpdate() function gets called every time this
         * function is called, with a superset of @p state, so the layer can
         * choose to not implement this function at all if relevant updates can
         * be done during the regular @ref doUpdate() as well.
         *
         * Default implementation does nothing.
         */
        virtual void doPreUpdate(LayerStates state);

        /**
         * @brief Update visible layer data to given offsets and positions
         * @param state             State that's needed to be updated
         * @param dataIds           Data IDs to update, in order that matches
         *      the draw order
         * @param clipRectIds       IDs of clip rects to use for @p dataIds
         * @param clipRectDataCounts  Counts of @p dataIds to use for each
         *      clip rect from @p clipRectIds
         * @param nodeOffsets       Absolute node offsets indexed by node ID
         * @param nodeSizes         Node sizes indexed by node ID
         * @param nodeOpacities     Absolute node opacities  (i.e., inheriting
         *      parent node opacities as well) indexed by node ID
         * @param nodesEnabled      Which visible nodes are enabled, i.e. which
         *      don't have @ref NodeFlag::Disabled set on themselves or any
         *      parent node
         * @param clipRectOffsets   Absolute clip rect offsets referenced by
         *      @p clipRectIds
         * @param clipRectSizes     Clip rect sizes referenced by
         *      @p clipRectIds
         * @param compositeRectOffsets  Offsets of framebuffer rectangles to
         *      composite
         * @param compositeRectSizes  Sizes of framebuffer rectangles to
         *      composite
         *
         * Implementation for @ref update(), which is called from
         * @ref AbstractUserInterface::update() whenever
         * @ref UserInterfaceState::NeedsDataUpdate or any of the global or
         * layer-specific states that imply it are present in
         * @ref AbstractUserInterface::state(). Is always called after
         * @ref doPreUpdate() and @ref doClean(), and before @ref doComposite()
         * and @ref doDraw(), with at least one @ref doSetSize() call happening
         * at some point before.
         *
         * The @p state is guaranteed to be a subset of
         * @ref LayerState::NeedsNodeOffsetSizeUpdate,
         * @relativeref{LayerState,NeedsNodeOrderUpdate},
         * @relativeref{LayerState,NeedsNodeEnabledUpdate},
         * @relativeref{LayerState,NeedsDataUpdate},
         * @relativeref{LayerState,NeedsCommonDataUpdate} and
         * @relativeref{LayerState,NeedsSharedDataUpdate} and the
         * implementation can make use of this information to skip some of its
         * internal update logic. It can however also update everything always.
         * The @ref LayerState::NeedsAttachmentUpdate flag isn't passed through
         * from @ref update() as it's only meant to be used by the layer to
         * signalize a state update to @ref AbstractUserInterface, not the
         * other way around.
         *
         * Node handles corresponding to @p dataIds are available in
         * @ref nodes(), node IDs can be then extracted from the handles
         * using @ref nodeHandleId(). The node IDs then index into the
         * @p nodeOffsets, @p nodeSizes, @p nodeOpacities and @p nodesEnabled
         * views. The @p nodeOffsets, @p nodeSizes, @p nodeOpacities and
         * @p nodesEnabled have the same size and are guaranteed to be large
         * enough to contain any valid node ID.
         *
         * All @ref nodes() at indices corresponding to @p dataIds are
         * guaranteed to not be @ref NodeHandle::Null at the time this function
         * is called. The @p nodeOffsets, @p nodeSizes, @p nodeOpacities and
         * @p nodesDisabled arrays may contain random or uninitialized values
         * for nodes different than those referenced from @p dataIds, such as
         * for nodes that are not currently visible or freed node handles.
         *
         * The node data are meant to be clipped by rects defined in
         * @p clipRectOffsets and @p clipRectSizes. The @p clipRectIds and
         * @p clipRectDataCounts have the same size and specify which of these
         * rects is used for which data. For example, a sequence of
         * @cpp {3, 2}, {0, 4}, {1, 7} @ce means clipping the first two data
         * with clip rect 3, then the next four data with clip rect 0 and then
         * the next seven data with clip rect 1. The sum of all
         * @p clipRectDataCounts is equal to the size of the @p dataIds array.
         * The @p clipRectOffsets and @p clipRectSizes have the same size and
         * are guaranteed to be large enough to contain any ID from
         * @p clipRectIds. They're in the same coordinate system as
         * @p nodeOffsets and @p nodeSizes, a zero offset and a zero size
         * denotes that no clipping is needed. Tt's up to the implementation
         * whether it clips the actual data directly or whether it performs
         * clipping at draw time.
         *
         * The @p compositeRectOffsets and @p compositeRectOffsets have the
         * same size and define rectangles to be used by compositing
         * operations, i.e. intersections of node rectangles with corresponding
         * clip rectangles. If the layer doesn't advertise
         * @ref LayerFeature::Composite, the views are empty.
         *
         * This function may get also called with @p dataIds being empty, for
         * example when @ref setNeedsUpdate() was called but the layer doesn't
         * have any data currently visible.
         *
         * Default implementation does nothing. Data passed to this function
         * are subsequently passed to @ref doComposite() / @ref doDraw() calls
         * as well, the only difference is that @ref doUpdate() gets called
         * just once with all data to update, while @ref doComposite() /
         * @ref doDraw() is called several times with different sub-ranges of
         * the data based on desired draw order.
         */
        virtual void doUpdate(LayerStates state, const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, const Containers::StridedArrayView1D<const Float>& nodeOpacities, Containers::BitArrayView nodesEnabled, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes, const Containers::StridedArrayView1D<const Vector2>& compositeRectOffsets, const Containers::StridedArrayView1D<const Vector2>& compositeRectSizes);

        /**
         * @brief Composite previously rendered contents
         * @param renderer          Renderer instance containing the previously
         *      rendered contents
         * @param compositeRectOffsets  Offsets of framebuffer rectangles to
         *      composite. Same as the view passed to @ref doUpdate() earlier.
         * @param compositeRectSizes  Sizes of framebuffer rectangles to
         *      composite. Same as the view passed to @ref doUpdate() earlier.
         * @param offset            Offset into @p compositeRectOffsets and
         *      @p compositeRectSizes
         * @param count             Count of @p compositeRectOffsets and
         *      @p compositeRectSizes to composite
         *
         * Implementation for @ref composite(), which is called from
         * @ref AbstractUserInterface::draw(). Called only if
         * @ref LayerFeature::Composite is supported, it's guaranteed that
         * @ref doUpdate() was called at some point before this function with
         * the exact same views passed to @p compositeRectOffsets and
         * @p compositeRectSizes, see its documentation for their relations and
         * constraints. This function is called after drawing contents of all
         * layers earlier in the top-level node and layer draw order. It's
         * guaranteed that @ref doDraw() will get called after this function.
         *
         * This function usually gets called several times with the same views
         * but different @p offset and @p count. The range of
         * @p compositeRectOffsets and @p compositeRectSizes views defined by
         * @p offset and @p count contains rectangles that correspond to the
         * layer data and can be used to restrict the compositing operation to
         * only the area that's actually subsequently drawn.)
         */
        virtual void doComposite(AbstractRenderer& renderer, const Containers::StridedArrayView1D<const Vector2>& compositeRectOffsets, const Containers::StridedArrayView1D<const Vector2>& compositeRectSizes, std::size_t offset, std::size_t count);

        /**
         * @brief Draw a sub-range of visible layer data
         * @param dataIds           Data IDs to update, in order that matches
         *      the draw order. Same as the view passed to @ref doUpdate()
         *      earlier.
         * @param offset            Offset into @p dataIds
         * @param count             Count of @p dataIds to draw
         * @param clipRectIds       IDs of clip rects to use for @p dataIds.
         *      Same as the view passed to @ref doUpdate() earlier.
         * @param clipRectDataCounts  Counts of @p dataIds to use for each
         *      clip rect from @p clipRectIds. Same as the view passed to
         *      @ref doUpdate() earlier.
         * @param clipRectOffset    Offset into @p clipRectIds and
         *      @p clipRectDataCounts
         * @param clipRectCount     Count of @p clipRectIds and
         *      @p clipRectDataCounts to use
         * @param nodeOffsets       Absolute node offsets. Same as the view
         *      passed to @ref doUpdate() earlier.
         * @param nodeSizes         Node sizes. Same as the view passed to
         *      @ref doUpdate() earlier.
         * @param nodeOpacities     Absolute node opacities. Same as the view
         *      passed to @ref doUpdate() earlier.
         * @param nodesEnabled      Which visible nodes are enabled, i.e. which
         *      don't have @ref NodeFlag::Disabled set on themselves or any
         *      parent node. Same as the view passed to @ref doUpdate()
         *      earlier.
         * @param clipRectOffsets   Absolute clip rect offsets. Same as the
         *      view passed to @ref doUpdate() earlier.
         * @param clipRectSizes     Clip rect sizes. Same as the view passed to
         *      @ref doUpdate() earlier.
         *
         * Implementation for @ref draw(), which is called from
         * @ref AbstractUserInterface::draw(). Called only if
         * @ref LayerFeature::Draw is supported, it's guaranteed that
         * @ref doUpdate() was called at some point before this function with
         * the exact same views passed to @p dataIds, @p clipRectIds,
         * @p clipRectDataCounts, @p nodeOffsets, @p nodeSizes,
         * @p nodeOpacities, @p nodesEnabled, @p clipRectOffsets and
         * @p clipRectSizes, see its documentation for their relations and
         * constraints. If @ref LayerFeature::Composite is supported as well,
         * it's guaranteed that @ref doComposite() was called before this
         * function and at some point after @ref doUpdate(), and after drawing
         * contents of all layers earlier in the top-level node and layer draw
         * order.
         *
         * Like with @ref doUpdate(), the @p clipRectOffsets and
         * @p clipRectSizes are in the same coordinate system as @p nodeOffsets
         * and @p nodeSizes. If performing the clipping at draw time (instead
         * of clipping the actual data directly in @p doDraw()), the
         * implementation may need to scale these to match actual framebuffer
         * pixels, i.e. by multiplying them with
         * @cpp Vector2{framebufferSize}/size @ce inside @ref doSetSize().
         *
         * This function usually gets called several times with the same views
         * but different @p offset, @p count, @p clipRectOffset and
         * @p clipRectCount values in order to interleave the draws for a
         * correct back-to-front order. In each call, the sum of all
         * @p clipRectDataCounts in the range given by @p clipRectOffset and
         * @p clipRectCount is equal to @p count. Unlike @ref doUpdate() or
         * @ref doClean(), this function is never called with an empty
         * @p count.
         */
        virtual void doDraw(const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, std::size_t offset, std::size_t count, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, std::size_t clipRectOffset, std::size_t clipRectCount, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, const Containers::StridedArrayView1D<const Float>& nodeOpacities, Containers::BitArrayView nodesEnabled, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes);

        /**
         * @brief Handle a pointer press event
         * @param dataId            Data ID the event happens on. Guaranteed to
         *      be less than @ref capacity() and point to a valid data.
         * @param event             Event data, with
         *      @ref PointerEvent::position() relative to the node to which the
         *      data is attached. If pointer event capture is active and the
         *      event is not primary, the position can be outside of the area
         *      of the node.
         *
         * Implementation for @ref pointerPressEvent(), which is called from
         * @ref AbstractUserInterface::pointerPressEvent(). See its
         * documentation for more information about pointer event behavior,
         * especially event capture. It's guaranteed that @ref doUpdate() was
         * called before this function with up-to-date data for @p dataId.
         *
         * If the implementation handles the event, it's expected to call
         * @ref PointerEvent::setAccepted() on it to prevent it from being
         * propagated further. If the event is primary, pointer capture is
         * implicitly performed on a node that accepts this event. Call
         * @ref PointerEvent::setCaptured() to disable it or to adjust the
         * behavior in case of a non-primary event.
         *
         * Default implementation does nothing, i.e. the @p event gets
         * implicitly propagated further.
         * @see @ref PointerEvent::isPrimary()
         */
        virtual void doPointerPressEvent(UnsignedInt dataId, PointerEvent& event);

        /**
         * @brief Handle a pointer release event
         * @param dataId            Data ID the event happens on. Guaranteed to
         *      be less than @ref capacity() and point to a valid data.
         * @param event             Event data, with
         *      @ref PointerEvent::position() relative to the node to which the
         *      data is attached. If pointer event capture is active, the
         *      position can be outside of the area of the node.
         *
         * Implementation for @ref pointerReleaseEvent(), which is called from
         * @ref AbstractUserInterface::pointerReleaseEvent(). See its
         * documentation for more information about pointer event behavior,
         * especially event capture. It's guaranteed that @ref doUpdate() was
         * called before this function with up-to-date data for @p dataId.
         *
         * If the implementation handles the event, it's expected to call
         * @ref PointerEvent::setAccepted() on it to prevent
         * it from being propagated further. If the event is primary, pointer
         * capture is implicitly released after, thus calling
         * @ref PointerEvent::setCaptured() only has an effect for non-primary
         * events.
         *
         * @m_class{m-note m-info}
         *
         * @par
         *      Note that this function is *not* called in case the currently
         *      pressed node becomes invisible or no longer accepts events. In
         *      that case @ref doVisibilityLostEvent() is called instead, see
         *      its documentation for details about differences in semantics.
         *
         * Default implementation does nothing, i.e. the @p event gets
         * implicitly propagated further.
         * @see @ref PointerEvent::isPrimary()
         */
        virtual void doPointerReleaseEvent(UnsignedInt dataId, PointerEvent& event);

        /**
         * @brief Handle a pointer move event
         * @param dataId            Data ID the event happens on. Guaranteed to
         *      be less than @ref capacity() and point to a valid data.
         * @param event             Event data, with
         *      @ref PointerMoveEvent::position() relative to the node to which
         *      the data is attached. If pointer event capture is active, the
         *      position can be outside of the area of the node.
         *
         * Implementation for @ref pointerMoveEvent(), which is called from
         * @ref AbstractUserInterface::pointerMoveEvent(). See its
         * documentation for more information about pointer event behavior,
         * especially event capture, hover and relation to
         * @ref doPointerEnterEvent() and @ref doPointerLeaveEvent(). It's
         * guaranteed that @ref doUpdate() was called before this function with
         * up-to-date data for @p dataId.
         *
         * If the implementation handles the event, it's expected to call
         * @ref PointerEvent::setAccepted() on it to prevent it from being
         * propagated further. Pointer capture behavior for remaining pointer
         * events can be changed using @ref PointerMoveEvent::setCaptured().
         *
         * Default implementation does nothing, i.e. the @p event gets
         * implicitly propagated further. That also implies the node is never
         * marked as hovered for primary events and enter / leave events are
         * not emitted for it.
         * @see @ref PointerMoveEvent::isPrimary()
         */
        virtual void doPointerMoveEvent(UnsignedInt dataId, PointerMoveEvent& event);

        /**
         * @brief Handle a pointer enter event
         * @param dataId            Data ID the event happens on. Guaranteed to
         *      be less than @ref capacity() and point to a valid data.
         * @param event             Event data, with
         *      @ref PointerMoveEvent::position() relative to the node to which
         *      the data is attached. If pointer event capture is active, the
         *      position can be outside of the area of the node.
         *
         * Implementation for @ref pointerEnterEvent(), which is called from
         * @ref AbstractUserInterface::pointerMoveEvent() if the currently
         * hovered node changed to one containing @p dataId. See its
         * documentation for more information about relation of pointer
         * enter/leave events to @ref doPointerMoveEvent(). It's guaranteed
         * that @ref doUpdate() was called before this function with up-to-date
         * data for @p dataId, the @p event is guaranteed to be always primary.
         *
         * Unlike @ref doPointerMoveEvent(), the accept status is ignored for
         * enter and leave events, as the event isn't propagated anywhere if
         * it's not handled. Thus calling @ref PointerEvent::setAccepted() has
         * no effect here. On the other hand, pointer capture behavior for
         * remaining pointer events can be changed using
         * @ref PointerMoveEvent::setCaptured() here as well.
         *
         * Default implementation does nothing.
         * @see @ref PointerMoveEvent::isPrimary()
         */
        virtual void doPointerEnterEvent(UnsignedInt dataId, PointerMoveEvent& event);

        /**
         * @brief Handle a pointer leave event
         * @param dataId            Data ID the event happens on. Guaranteed to
         *      be less than @ref capacity() and point to a valid data.
         * @param event             Event data, with
         *      @ref PointerMoveEvent::position() relative to the node to which
         *      the data is attached. If pointer event capture is active, the
         *      position can be outside of the area of the node.
         *
         * Implementation for @ref pointerEnterEvent(), which is called from
         * @ref AbstractUserInterface::pointerMoveEvent() if the currently
         * hovered node changed away from one containing @p dataId. See its
         * documentation for more information about relation of pointer
         * enter/leave events to @ref doPointerMoveEvent(). It's guaranteed
         * that @ref doUpdate() was called before this function with up-to-date
         * data for @p dataId, the @p event is guaranteed to be always primary.
         *
         * Unlike @ref doPointerMoveEvent(), the accept status is ignored for
         * enter and leave events, as the event isn't propagated anywhere if
         * it's not handled. Thus calling @ref PointerEvent::setAccepted() has
         * no effect here. On the other hand, pointer capture behavior for
         * remaining pointer events can be changed using
         * @ref PointerMoveEvent::setCaptured() here as well.
         *
         * @m_class{m-note m-info}
         *
         * @par
         *      Note that this function is *not* called in case the currently
         *      hovered node becomes invisible or no longer accepts events. In
         *      that case @ref doVisibilityLostEvent() is called instead, see
         *      its documentation for details about differences in semantics.
         *
         * Default implementation does nothing.
         * @see @ref PointerMoveEvent::isPrimary()
         */
        virtual void doPointerLeaveEvent(UnsignedInt dataId, PointerMoveEvent& event);

        /**
         * @brief Handle a pointer cancel event
         * @param dataId            Data ID the event happens on. Guaranteed to
         *      be less than @ref capacity() and point to a valid data.
         * @param event             Event data
         *
         * Implementation for @ref pointerCancelEvent(), which is called from
         * @ref AbstractUserInterface::pointerPressEvent(),
         * @relativeref{AbstractUserInterface,pointerReleaseEvent()} or
         * @relativeref{AbstractUserInterface,pointerMoveEvent()} if the event
         * falls through and gets accepted by a node different than the one it
         * originally happened on. It's guaranteed that @ref doUpdate() was
         * called before this function with up-to-date data for @p dataId.
         *
         * Default implementation does nothing.
         */
        virtual void doPointerCancelEvent(UnsignedInt dataId, PointerCancelEvent& event);

        /**
         * @brief Handle a scroll event
         * @param dataId            Data ID the event happens on. Guaranteed to
         *      be less than @ref capacity() and point to a valid data.
         * @param event             Event data, with
         *      @ref ScrollEvent::position() relative to the node to which the
         *      data is attached. If pointer event capture is active, the
         *      position can be outside of the area of the node.
         *
         * Implementation for @ref scrollEvent(), which is called from
         * @ref AbstractUserInterface::scrollEvent(). See its documentation for
         * more information about pointer event behavior, especially event
         * capture. It's guaranteed that @ref doUpdate() was called before this
         * function with up-to-date data for @p dataId.
         *
         * If the implementation handles the event, it's expected to call
         * @ref ScrollEvent::setAccepted() on it to prevent it from being
         * propagated further.
         *
         * Default implementation does nothing, i.e. the @p event gets
         * implicitly propagated further.
         */
        virtual void doScrollEvent(UnsignedInt dataId, ScrollEvent& event);

        /**
         * @brief Handle a focus event
         * @param dataId            Data ID the event happens on. Guaranteed to
         *      be less than @ref capacity() and point to a valid data.
         * @param event             Event data
         *
         * Implementation for @ref focusEvent(), which is called from
         * @ref AbstractUserInterface::focusEvent() and
         * @ref AbstractUserInterface::pointerPressEvent(). See their
         * documentation for more information about focus and blur event
         * behavior. It's guaranteed that @ref doUpdate() was called before
         * this function with up-to-date data for @p dataId.
         *
         * If the implementation handles the event, it's expected to call
         * @ref FocusEvent::setAccepted() on it to cause the node to be marked
         * as focused. If it doesn't, the node doesn't get marked as focused
         * and depending on what @ref AbstractUserInterface API the event
         * originated in, the previously focused node stays or gets blurred.
         *
         * Default implementation does nothing.
         */
        virtual void doFocusEvent(UnsignedInt dataId, FocusEvent& event);

        /**
         * @brief Handle a blur event
         * @param dataId            Data ID the event happens on. Guaranteed to
         *      be less than @ref capacity() and point to a valid data.
         * @param event             Event data
         *
         * Implementation for @ref blurEvent(), which is called from
         * @ref AbstractUserInterface::focusEvent() and
         * @ref AbstractUserInterface::pointerPressEvent(). See their
         * documentation for more information about focus and blur event
         * behavior. It's guaranteed that @ref doUpdate() was called before
         * this function with up-to-date data for @p dataId.
         *
         * Unlike @ref doFocusEvent(), the accept status is ignored for blur
         * events, as the node is still unmarked as focused if the event is not
         * handled. Thus calling @ref FocusEvent::setAccepted() has no effect
         * here.
         *
         * @m_class{m-note m-info}
         *
         * @par
         *      Note that this function is *not* called in case the currently
         *      focused node becomes invisible, no longer accepts events or is
         *      no longer focusable. In that case @ref doVisibilityLostEvent()
         *      is called instead, see its documentation for details about
         *      differences in semantics.
         *
         * Default implementation does nothing.
         */
        virtual void doBlurEvent(UnsignedInt dataId, FocusEvent& event);

        /**
         * @brief Handle a key press event
         * @param dataId            Data ID the event happens on. Guaranteed to
         *      be less than @ref capacity() and point to a valid data.
         * @param event             Event data
         *
         * Implementation for @ref keyPressEvent(), which is called from
         * @ref AbstractUserInterface::keyPressEvent(). See its documentation
         * for more information about key event behavior. It's guaranteed that
         * @ref doUpdate() was called before this function with up-to-date data
         * for @p dataId.
         *
         * If the implementation handles the event, it's expected to call
         * @ref KeyEvent::setAccepted() on it to prevent it from being
         * propagated further.
         *
         * Default implementation does nothing, i.e. the @p event gets
         * implicitly propagated further.
         */
        virtual void doKeyPressEvent(UnsignedInt dataId, KeyEvent& event);

        /**
         * @brief Handle a pointer release event
         * @param dataId            Data ID the event happens on. Guaranteed to
         *      be less than @ref capacity() and point to a valid data.
         * @param event             Event data
         *
         * Implementation for @ref keyReleaseEvent(), which is called from
         * @ref AbstractUserInterface::keyReleaseEvent(). See its documentation
         * for more information. It's guaranteed that @ref doUpdate() was
         * called before this function with up-to-date data for @p dataId.
         *
         * If the implementation handles the event, it's expected to call
         * @ref KeyEvent::setAccepted() on it to prevent it from being
         * propagated further.
         *
         * Default implementation does nothing, i.e. the @p event gets
         * implicitly propagated further.
         */
        virtual void doKeyReleaseEvent(UnsignedInt dataId, KeyEvent& event);

        /**
         * @brief Handle a text input event
         * @param dataId            Data ID the event happens on. Guaranteed to
         *      be less than @ref capacity() and point to a valid data.
         * @param event             Event data
         *
         * Implementation for @ref textInputEvent(), which is called from
         * @ref AbstractUserInterface::textInputEvent(). See its documentation
         * for more information about text input event behavior. It's
         * guaranteed that @ref doUpdate() was called before this function with
         * up-to-date data for @p dataId.
         *
         * If the implementation handles the event, it's expected to call
         * @ref TextInputEvent::setAccepted() on it to prevent it from being
         * propagated further.
         *
         * Default implementation does nothing, i.e. the @p event gets
         * implicitly propagated further.
         */
        virtual void doTextInputEvent(UnsignedInt dataId, TextInputEvent& event);

        /**
         * @brief Handle a visibility lost event
         * @param dataId            Data ID the event happens on. Guaranteed to
         *      be less than @ref capacity() and point to a valid data.
         * @param event             Event data
         *
         * Implementation for @ref visibilityLostEvent(), which is called from
         * @ref AbstractUserInterface::update() if the currently hovered,
         * pressed, captured or focused node containing @p dataId can no longer
         * receive events due to @ref NodeFlag::Hidden, @ref NodeFlag::NoEvents
         * or @ref NodeFlag::Disabled being set on the node or any of its
         * parents, or if a currently focused node is no longer
         * @ref NodeFlag::Focusable.
         *
         * @m_class{m-note m-warning}
         *
         * @par
         *      Unlike other events, for which it's guaranteed that they're
         *      called *after* a @ref doUpdate(), this event is always called
         *      *before* the corresponding @ref doUpdate() in order to make the
         *      changes it performs immediately applied. Which also means the
         *      implementation should only perform operations that don't
         *      require a global UI state rebuild, such as node or data
         *      removal.
         *
         * Default implementation does nothing.
         */
        virtual void doVisibilityLostEvent(UnsignedInt dataId, VisibilityLostEvent& event);

        /* Common implementations for foo(DataHandle, ...) and
           foo(LayerDataHandle, ...) */
        MAGNUM_UI_LOCAL void attachInternal(UnsignedInt id, NodeHandle node);
        MAGNUM_UI_LOCAL void removeInternal(UnsignedInt id);

        struct State;
        Containers::Pointer<State> _state;
};

#ifdef DOXYGEN_GENERATING_OUTPUT
/**
@brief Debug layer integration

If an inner type with this name is implemented on a layer that's passed to
@ref DebugLayer::setLayerName(const T&, const Containers::StringView&), the
@ref print() function is used by the @ref DebugLayerFlag::NodeHighlight
functionality to provide additional details about all data attachments coming
from given layer. See @ref Ui-DebugLayer-integration for more information.
*/
/* While it'd be significantly simpler both for the library and for layers to
   have this as a virtual base class that then gets subclassed with interfaces
   implemented, it's deliberately not done to avoid header dependencies as well
   as make it possible to DCE all debug-layer-related code if it isn't used. */
class AbstractLayer::DebugIntegration {
    public:
        /**
         * @brief Print details about a particular data
         * @param debug     Debug output where to print
         * @param layer     Layer associated with given @p data. The
         *      implementation can use either the layer type this class is part
         *      of or any base type.
         * @param layerName Layer name that was passed to
         *      @ref DebugLayer::setLayerName(const T&, const Containers::StringView&)
         * @param data      Data to print info about. Guaranteed to be valid.
         *
         * Used internally by @ref DebugLayer. To fit among other info provided
         * by @ref DebugLayer itself, the implementation is expected to indent
         * the output by at least two spaces and end with a newline (i.e.,
         * @relativeref{Magnum,Debug::newline}).
         */
        void print(Debug& debug, const Layer& layer, Containers::StringView layerName, LayerDataHandle data);
};
#endif

}}

#endif
