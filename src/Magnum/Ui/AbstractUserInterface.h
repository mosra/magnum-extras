#ifndef Magnum_Ui_AbstractUserInterface_h
#define Magnum_Ui_AbstractUserInterface_h
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
 * @brief Class @ref Magnum::Ui::AbstractUserInterface, enum @ref Magnum::Ui::UserInterfaceState, enum set @ref Magnum::Ui::UserInterfaceStates
 * @m_since_latest_{extras}
 */

#include <Corrade/Containers/EnumSet.h>
#include <Corrade/Containers/Pointer.h>
#include <Magnum/Magnum.h>

#include "Magnum/Ui/Ui.h"
#include "Magnum/Ui/visibility.h"

namespace Magnum { namespace Ui {

/**
@brief User interface state
@m_since_latest_{extras}

Used to decide whether @ref AbstractUserInterface::clean() or
@ref AbstractUserInterface::update() need to be called to refresh the internal
state before the interface is drawn or an event is handled. See also
@ref LayerState for layer-specific state, @ref LayouterState for
layouter-specific state and @ref AnimatorState for animator-specific state.
@see @ref UserInterfaceStates, @ref AbstractUserInterface::state()
*/
enum class UserInterfaceState: UnsignedShort {
    /**
     * @ref AbstractUserInterface::update() needs to be called to recalculate
     * or reupload data attached to visible node hierarchy after they've been
     * changed. Set implicitly if any of the layers have
     * @ref LayerState::NeedsDataUpdate,
     * @relativeref{LayerState,NeedsCommonDataUpdate} or
     * @relativeref{LayerState,NeedsSharedDataUpdate} set, is reset next time
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
     * set of nodes that can be affected by certain events after node flags
     * changed. Set implicitly after every
     * @relativeref{AbstractUserInterface,setNodeFlags()},
     * @relativeref{AbstractUserInterface,addNodeFlags()} and
     * @relativeref{AbstractUserInterface,clearNodeFlags()} that changes the
     * presence of the @ref NodeFlag::NoBlur flag; is reset next time
     * @ref AbstractUserInterface::update() is called. Implied by
     * @relativeref{UserInterfaceState,NeedsNodeEnabledUpdate},
     * @relativeref{UserInterfaceState,NeedsNodeClipUpdate},
     * @relativeref{UserInterfaceState,NeedsLayoutUpdate},
     * @relativeref{UserInterfaceState,NeedsLayoutAssignmentUpdate},
     * @relativeref{UserInterfaceState,NeedsNodeUpdate} and
     * @relativeref{UserInterfaceState,NeedsNodeClean}, so it's also set by
     * everything that sets those flags.
     */
    NeedsNodeEventMaskUpdate = 1 << 2,

    /**
     * @ref AbstractUserInterface::update() needs to be called to refresh the
     * enabled node set after node flags changed. Set implicitly after every
     * @relativeref{AbstractUserInterface,setNodeFlags()},
     * @relativeref{AbstractUserInterface,addNodeFlags()} and
     * @relativeref{AbstractUserInterface,clearNodeFlags()} that changes the
     * presence of the @ref NodeFlag::NoEvents, @ref NodeFlag::Disabled or
     * @ref NodeFlag::Focusable flag; is reset next time
     * @ref AbstractUserInterface::update() is called. Implies
     * @ref UserInterfaceState::NeedsNodeEventMaskUpdate and
     * @relativeref{UserInterfaceState,NeedsDataAttachmentUpdate}. Implied by
     * @relativeref{UserInterfaceState,NeedsNodeClipUpdate},
     * @relativeref{UserInterfaceState,NeedsLayoutUpdate},
     * @relativeref{UserInterfaceState,NeedsLayoutAssignmentUpdate},
     * @relativeref{UserInterfaceState,NeedsNodeUpdate} and
     * @relativeref{UserInterfaceState,NeedsNodeClean}, so it's also set by
     * everything that sets those flags.
     */
    NeedsNodeEnabledUpdate = NeedsNodeEventMaskUpdate|NeedsDataAttachmentUpdate|(1 << 3),

    /**
     * @ref AbstractUserInterface::update() needs to be called to refresh the
     * visible node set after the user interface size or node flags changed.
     * Set implicitly after every @relativeref{AbstractUserInterface,setSize()}
     * call that changes the user interface size if any nodes were already
     * created and after every
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
    NeedsNodeClipUpdate = NeedsNodeEnabledUpdate|(1 << 4),

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
    NeedsLayoutUpdate = NeedsNodeClipUpdate|(1 << 5),

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
    NeedsLayoutAssignmentUpdate = NeedsLayoutUpdate|(1 << 6),

    /**
     * @ref AbstractUserInterface::update() needs to be called to refresh the
     * the hierarchical opacity information after node opacity was changed. Set
     * implicitly after every @ref AbstractUserInterface::setNodeOpacity()
     * call, is reset next time @ref AbstractUserInterface::update() is
     * called. Implies @ref UserInterfaceState::NeedsDataUpdate. Implied by
     * @relativeref{UserInterfaceState,NeedsNodeUpdate} and
     * @relativeref{UserInterfaceState,NeedsNodeClean}, so it's also set by
     * everything that sets those flags.
     */
    NeedsNodeOpacityUpdate = NeedsDataUpdate|(1 << 7),

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
     * @ref UserInterfaceState::NeedsLayoutAssignmentUpdate and
     * @ref UserInterfaceState::NeedsNodeOpacityUpdate. Implied by
     * @relativeref{UserInterfaceState,NeedsNodeClean}, so it's also set by
     * everything that sets that flag.
     */
    NeedsNodeUpdate = NeedsLayoutAssignmentUpdate|NeedsNodeOpacityUpdate|(1 << 8),

    /**
     * @ref AbstractUserInterface::clean() needs to be called to prune
     * animations attached to removed data. Set implicitly if any of the layers
     * have @ref LayerState::NeedsDataClean set and transitively after every
     * @ref AbstractUserInterface::removeNode() call, is reset next time
     * @ref AbstractUserInterface::clean() is called. Implied by
     * @ref UserInterfaceState::NeedsNodeClean.
     */
    NeedsDataClean = 1 << 9,

    /**
     * @ref AbstractUserInterface::clean() needs to be called to prune child
     * hierarchies of removed nodes and data, layouts and animation assigned to
     * those. Set implicitly after every
     * @ref AbstractUserInterface::removeNode() call, is reset to
     * @ref UserInterfaceState::NeedsNodeUpdate next time
     * @ref AbstractUserInterface::clean() is called. Implies
     * @ref UserInterfaceState::NeedsNodeUpdate and
     * @relativeref{UserInterfaceState,NeedsDataClean}.
     */
    NeedsNodeClean = NeedsNodeUpdate|NeedsDataClean|(1 << 10),

    /**
     * @ref AbstractUserInterface::advanceAnimations() needs to be called to
     * advance active animations. Set implicitly if any of the animators have
     * @ref AnimatorState::NeedsAdvance set, is reset next time
     * @ref AbstractUserInterface::advanceAnimations() is called if no
     * animations are @ref AnimationState::Scheduled,
     * @ref AnimationState::Playing or @ref AnimationState::Paused anymore.
     */
    NeedsAnimationAdvance = 1 << 11,
};

/**
@debugoperatorenum{UserInterfaceState}
@m_since_latest_{extras}
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, UserInterfaceState value);

/**
@brief User interface states
@m_since_latest_{extras}

@see @ref AbstractUserInterface::state()
*/
typedef Containers::EnumSet<UserInterfaceState> UserInterfaceStates;

/**
@debugoperatorenum{UserInterfaceStates}
@m_since_latest_{extras}
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, UserInterfaceStates value);

CORRADE_ENUMSET_OPERATORS(UserInterfaceStates)

namespace Implementation {
    template<class, class = void> struct ApplicationSizeConverter;
    template<class, class = void> struct PointerEventConverter;
    template<class, class = void> struct PointerMoveEventConverter;
    template<class, class = void> struct ScrollEventConverter;
    template<class, class = void> struct KeyEventConverter;
    template<class, class = void> struct TextInputEventConverter;
}

/**
@brief Base implementation of the main user interface
@m_since_latest_{extras}

Owns the whole user interface, providing everything from input event handling
to animation and drawing.

@section Ui-AbstractUserInterface-setup Setting up a user interface instance

Unless you're building a UI fully consisting of custom widgets, you'll want to
instantiate the user interface through the @ref UserInterfaceGL subclass, which
by default includes everything that's needed by builtin widgets. The
constructor takes a UI size, in respect to which all contents as well as input
events get positioned, and a style instance describing how the widgets will
look like. At the moment, @ref McssDarkStyle is the only style provided by the
library itself.

@snippet Ui-gl.cpp UserInterfaceGL-setup

<b></b>

@m_class{m-note m-info}

@par
    See the @ref UserInterfaceGL class documentation if you need delayed
    creation or more control over what's being set up.

Then, at the very least, the UI needs to be (re)drawn when needed. The renderer
uses a [premultiplied alpha](https://developer.nvidia.com/content/alpha-blending-pre-or-not-pre)
workflow, so it expects an appropriate @ref GL::Renderer::setBlendFunction()
setup, shown below. If all other rendering uses a premultiplied alpha as well,
it's enough to call it just once, otherwise you need to make sure it's set up
every time the UI is drawn.

@snippet Ui-gl.cpp AbstractUserInterface-setup-blend

The @ref draw() function draws to the currently bound framebuffer, clearing is
left upon the application so it can draw other contents underneath the UI. The
actual enablement of blending and other state during a @ref draw() is taken
care of by a @ref RendererGL instance internally.

@snippet Ui-gl.cpp AbstractUserInterface-setup-draw

@anchor Ui-AbstractUserInterface-redraw-on-demand

<b></b>

@m_class{m-block m-success}

@par Power-efficient on-demand redrawing
    The whole @ref Ui library, including animations, is designed to redraw only
    when needed. A desire to redraw is signalled by @ref state() returning a
    non-empty set of @ref UserInterfaceState values, so assuming there's
    nothing else besides the UI that needs to be redrawn, the above could be
    rewritten like this:
@par
    @snippet Ui-gl.cpp AbstractUserInterface-setup-draw-ondemand
@par

For actual interactivity, the user interface needs to receive input events via
@ref pointerPressEvent(), @ref pointerReleaseEvent(), @ref pointerMoveEvent(),
@ref scrollEvent(), @ref keyPressEvent(), @ref keyReleaseEvent() and
@ref textInputEvent(). They take a @ref PointerEvent, @ref PointerMoveEvent,
@ref ScrollEvent, @ref KeyEvent or @ref TextInputEvent instances containing the
actual event data such as the pointer or key being pressed, and in case of
pointer and scroll events also a position at which the event happens. For
example:

@snippet Ui.cpp AbstractUserInterface-setup-events

<b></b>

@m_class{m-note m-info}

@par
    @ref Ui-AbstractUserInterface-events "Event handling and propagation" is
    described in more detail later.

@section Ui-AbstractUserInterface-application Integration with application libraries

Unless you're using a custom windowing toolkit, you'll likely want to make the
UI drawing and event handling directly tied to a
@ref Platform::Sdl2Application "Platform::*Application" class. By including
@ref Magnum/Ui/Application.h it's possible to pass the application and event
instances directly to the user interface without needing to translate and pass
through individual data. The constructor can take an application instance to
figure out an appropriate DPI-aware size, and similarly you can pass the
viewport event to @ref setSize() to make the UI perform a relayout. Then,
assuming nothing else needs to be drawn besides the UI, a redraw is again
scheduled only when needed:

@snippet Ui-sdl2.cpp AbstractUserInterface-application-construct-viewport

<b></b>

@m_class{m-note m-info}

@par
    If you need more control over UI size, see below for detailed explanation
    of @ref Ui-AbstractUserInterface-dpi "DPI awareness".

With the code below, events coming from the application get internally
converted to a corresponding @ref PointerEvent, @ref PointerMoveEvent,
@ref ScrollEvent, @ref KeyEvent or @ref TextInputEvent instance with a subset
of information given application provides. Then, if the event is accepted by
the user interface, matching @ref Platform::Sdl2Application::PointerEvent::setAccepted() "Platform::*Application::PointerEvent::setAccepted()"
etc. get called as well, to prevent it from propagating further in certain
circumstances (such as to the browser window when compiling for the web). To
make sure the UI is appropriately redrawn after handling an event, each of
these again checks against @ref state():

@snippet Ui-sdl2.cpp AbstractUserInterface-application-events

Note that in general the UI needing a redraw is unrelated to whether an event
was accepted by it, so the checks are separate --- for example a
@m_class{m-label m-default} **Home** key press may be accepted by a text input,
but if the cursor is already at the begin of the text, it doesn't cause any
visual change and thus there's no need to redraw anything.

@section Ui-AbstractUserInterface-animations Setup for UI animations

In order to have animations working, @ref advanceAnimations() needs to be
called right before @ref draw(). The UI library itself doesn't have any notion
of current time and the time is meant to be passed externally instead, which
allows for more flexibility especially when testing and debugging animations.

The time value can be either absolute or relative to the application start, but
it should be coming from a monotonic clock as the internal animation logic
expects the time to never go backward, such as when system clock is adjusted or
if it's periodically synchronized over the network. Generally the
@ref Platform::Sdl2Application "Platform::*Application" classes don't expose
any timer provided by the toolkits, so the most portable choice is
@ref std::chrono::steady_clock (as opposed to @ref std::chrono::system_clock,
for example). With @ref Magnum/Math/TimeStl.h it can convert to a
@ref Nanoseconds value:

@snippet Ui-sdl2.cpp AbstractUserInterface-animations-advance

<b></b>

@m_class{m-block m-success}

@par Animations and on-demand redrawing
    Note that, as shown in the snippet, to have animations working you don't
    need to constantly redraw either. The library keeps track of whether any
    animations are currently playing or are scheduled to be played in the
    future, and returns @ref UserInterfaceState::NeedsAnimationAdvance in
    @ref state() in that case, which the snippet uses to trigger a redraw. Once
    all animations stop, @ref state() becomes empty, causing the application to
    again just wait for another input event.

Animations can happen also in response to input events. To equip them with a
time value that matches what's passed to @ref advanceAnimations(), pass it as a
second argument to the @ref pointerPressEvent(Event& event, Args&&... args)
etc. functions. If you don't pass a time value to an event, given animation
will behave as if it started at time @cpp 0 @ce, thus being already stopped at
the time next animation advance happens.

@snippet Ui-sdl2.cpp AbstractUserInterface-animations-events

With everything set up, you can enable @ref McssDarkStyle::Feature::Animations
in the builtin style, which will perform various fade out animations as well as
a blinking cursor in text input fields. The builtin style has more animation
options, see @ref Ui-McssDarkStyle-animations for details.

@snippet Ui-sdl2.cpp AbstractUserInterface-animations-style-features

@section Ui-AbstractUserInterface-handles Handles and resource ownership

Unlike traditional toolkits, which commonly use pointers to UI elements
individually allocated on heap, all resources in the @ref Ui library are
referenced by *handles*, such as a @ref NodeHandle or an @ref AnimatorHandle.
Much like a raw pointer, a handle is a trivially copyable type (an
@cpp enum @ce in this case), but compared to a pointer it's not usable on its
own. Getting the actual resource means passing the handle to a corresponding
interface that actually owns given resource.

The handle consists of an *index* and a *generation counter*. Internally, the
index points into a contiguous array (or several arrays) containing data for
resources of given type. When a new resource is created, the handle gets an
index of the next free item in that array; when it's removed, item at given
index gets put back into a list of free items. Additionally, the generation
counter gets incremented on every removal, which in turn causes existing
handles pointing to the same index but with old generation counter to be
treated as invalid.

@htmlinclude ui-handle.svg

Compared to pointers that can point basically anywhere, handles have several
advantages. The contiguous array simplifies memory management and makes batch
processing efficient, and the handle types can be only large enough to store
the typical amount of data (for example @ref NodeHandle is just 32 bits,
@ref AnimatorHandle just 16). The clear ownership model together with the
generation counter then solves the problem of dangling references and resource
leaks.

While in most cases you'll treat handles as opaque identifiers, it's possible
to make use of the contiguous nature of their indices when storing any extra
data associated with a particular handle type. For example, in case you'd want
to name node handles, instead of creating a (hash)map with @ref NodeHandle as a
key, you can extract the handle index with @ref nodeHandleId() and use a plain
array instead:

@snippet Ui.cpp AbstractUserInterface-handles-extract

The above snippet also makes use of the handle generation, extracted with
@ref nodeHandleGeneration(), to ensure stale names aren't returned for handles
that recycle a previously used index. The generation is remembered when setting
the name, and it's compared against the handle when retrieving. If it doesn't
match, it's the same as if the name wouldn't be present at all. The generation
counter is @cpp 0 @ce only for @ref NodeHandle::Null, so the
default-constructed `Name` entries above aren't referring to valid nodes
either.

<b></b>

@m_class{m-note m-info}

@par
    There are equivalently named helpers and constants for all other handle
    types, see @ref Magnum/Ui/Handle.h for the complete list.

@section Ui-AbstractUserInterface-nodes Node hierarchy

At the core of the user interface, defining both the visuals and event
propagation, is a hierarchy of *nodes*. A node is by itself just a rectangle
defined by a size, an offset relative to its parent, opacity and a few behavior
flags. Everything else --- visuals, interactivity, layouting behavior,
animation, persistent state --- is handled by *data*, *layouts* and *animations*
optionally attached to particular nodes. Those are explained further below.

A node is created using @ref createNode() by passing an offset and size. You
get a @ref NodeHandle back, which can be then used to query and modify the node
properties such as @ref setNodeOffset() or @ref setNodeSize().

@snippet Ui.cpp AbstractUserInterface-nodes

@htmlinclude ui-node-hierarchy.svg

The `panel` above is a *root node*, which is positioned relatively to the UI
itself. The other nodes then specify it as a parent, and are positioned
relatively to it, so e.g. `content` is at offset @cpp {60, 90} @ce. Besides
positioning hierarchy, the parent/child relationship also has effect on
lifetime. While a root node exists until @ref removeNode() is called on it or
until the end of the UI instance lifetime, child nodes additionally get removed
if their parent is removed. Once a node is removed, either directly or
indirectly, its @ref NodeHandle becomes invalid. Continuing from the above, if
you remove the `panel`, the `title` and `content` will get subsequently cleaned
up as well. Currently, node parent is specified during creation and cannot be
subsequently changed.

@subsection Ui-AbstractUserInterface-nodes-opacity Node opacity

Node opacity, controlled with @ref setNodeOpacity(), has similar nesting
behavior as node offsets. By default, all nodes are fully opaque, and the
opacity gets multiplied with the parent's. In the snippet below, the effective
opacity of `title` will be @cpp 0.6f @ce and `content` will inherit the
@cpp 0.8f @ce opacity from the `panel`. This feature is mainly for convenient
fade-in / fade-out of various parts of the UI without having to laboriously
update each and every individual UI element.

@snippet Ui.cpp AbstractUserInterface-nodes-opacity

@subsection Ui-AbstractUserInterface-nodes-order Top-level node hierarchies and visibility order

A root node along with all its children --- in this case `panel` along with
`title` and `content` --- is called a *top-level node hierarchy*. Each
top-level node hierarchy is ordered relatively to other top-level hierarchies,
allowing stacking of various UI elements such as dialogs, tooltips and menus.
When a root node is created, it's by default put in front of all other
top-level node hierarchies. Its order can be then adjusted using
@ref setNodeOrder() where the second argument is a top-level node *behind*
which the node should be placed. Specifying @ref NodeHandle::Null puts it in
front.

@snippet Ui.cpp AbstractUserInterface-nodes-order

@htmlinclude ui-node-hierarchy-another.svg

It's also possible to completely remove the hierarchy from the top-level node
order with @ref clearNodeOrder(). Doing so doesn't remove any nodes, just
excludes then from the visible set, and calling @ref setNodeOrder() with that
top-level node puts it back into the visibility order. This is a useful
alternative to recreating a short-lived popup many times over, for example.

@snippet Ui.cpp AbstractUserInterface-nodes-order-clear

In addition to the top-level node order, visibility is defined by the node
hierarchy. Parents are always behind their children (so, from the snippet
above, the `panel` background will be visible underneath the `title` and
`content`), however there's no order specified between siblings (i.e., one
can't rely on `title` being drawn before `content` or vice versa). The
assumption here is that majority of UIs are non-overlapping hierarchies for
which such an ordering would be a costly extra step without significant
benefits.

@subsubsection Ui-AbstractUserInterface-nodes-order-nested Nested top-level node hierarchies

Besides root nodes, nested nodes can be made top-level as well. They'll stay
positioned relative to the parent node offset, which makes it convenient for
placing various popups and tooltips next to UI elements they come from. As with
root nodes, a nested node can be made top-level by calling @ref setNodeOrder(),
@ref clearNodeOrder() then again excludes it from the visibility order.

@snippet Ui.cpp AbstractUserInterface-nodes-order-nested

@htmlinclude ui-node-hierarchy-tooltip.svg

Compared to root nodes, nested top-level nodes are tied to the node hierarchy
they come from --- in particular, they can be only ordered relative to other
nested top-level nodes under the same top-level node hierarchy. Changing order
of the enclosing top-level nodes then brings the nested top-level hierarchies
along. In the above example, assuming the `tooltip` wouldn't get hidden,
raising the `anotherPanel` in front of `panel` would make it ordered in front
of the `tooltip` as well.

Top-level hierarchies, nested or otherwise, can be also used to circumvent the
visibility order limitations in cases where UI elements actually end up
overlapping each other. One such case might be drag & drop, or various list
reordering animations. Affected node sub-hierarchy can be made top-level for
the time it's being moved and when it's back in place, @ref flattenNodeOrder()
puts it back to the visibility order defined by the hierarchy.

@subsection Ui-AbstractUserInterface-nodes-flags Node behavior flags

Finally, nodes can have various flags set that affect their visibility and
event handling behavior. By passing @ref NodeFlag::Clip to either
@ref createNode() or to @ref addNodeFlags(), contents of the node and all
nested nodes that overflow the node area will be clipped, @ref clearNodeFlags()
then does an inverse. The assumption is again that most UI elements don't
overflow and clipping every node would be relatively expensive, so by default
overflowing contents are visible.

Visibility of individual nodes can be toggled with @ref NodeFlag::Hidden. When
set, all nested nodes are hidden as well. This can also be used as an
alternative to @ref clearNodeOrder() for top-level nodes because hiding and
un-hiding preserves the previous top-level node order.

@ref NodeFlag::Disabled makes a node disabled, which effectively makes it not
respond to events anymore and *may* also affect the visual look, depending on
whether given UI element provides a disabled visual style.
@ref NodeFlag::NoEvents is a subset of that, affecting only the events but not
visuals, and can be used for example when animating a gradual transition from/to
a disabled state, to not have the elements react to input until the animation
finishes. Both of these again propagate to all nested nodes as well, so
disabling a part of the UI can be done only on the enclosing node and not on
each and every child.

@section Ui-AbstractUserInterface-layers Data layers

Next to the node hierarchy, the user interface contains a list of layers, which
attach *data* to particular nodes. Each layer instance, derived from
@ref AbstractLayer, defines what features its data have --- whether they draw,
process input events, or for example act as just state storage. While nodes
define placement and hierarchy, layers are what makes the node hierarchy
actually draw and do something.

While it's possible to implement all drawing and all event processing in a
single layer, in practice a UI consists of several specialized layers. The
following builtin layers are expected to cover most use cases, an application
can then choose to implement a custom layer if none of the builtin layers
satisfy its needs.

-   @ref BaseLayer for drawing rounded rectangles, outlines and textured quads,
-   @ref TextLayer for text rendering as well as editing,
-   @ref LineLayer for antialiased line drawing,
-   @ref EventLayer for attaching callbacks to input events like tap, click,
    drag or key press,
-   and @ref DebugLayer, a sort of a meta-layer for inspecting contents of the
    UI itself.

A particular node then gets data attached from zero or more layers. As an example, continuing with the node diagram from above and visualizing data
attachments with colored outlines, the `panel` node could have attached a
@m_class{m-label m-warning} **background** drawn by @ref BaseLayer, the `title`
node then have attached a nested @m_class{m-label m-warning} **background**, a
@m_class{m-label m-info} **text** from @ref TextLayer and an
@m_class{m-label m-danger} **event handler** to be able to drag the panel
around from @ref EventLayer, and the `content` node then being just a
@m_class{m-label m-info} **text** again:

@htmlinclude ui-node-data.svg

A layer is created with @ref createLayer(), the resulting @ref LayerHandle is
then passed to a constructor of a particular @ref AbstractLayer subclass, and
the constructed layer instance is then passed to @ref setLayerInstance(). For
example, as shown below with the @ref EventLayer. Because the layer instance is
a @relativeref{Corrade,Containers::Pointer}, it won't get moved anywhere
internally afterwards and it's safe to keep a reference to it for as long as it
exists:

@snippet Ui.cpp AbstractUserInterface-layers-create

The instance can be also queried with a @ref LayerHandle using @ref layer(),
but at that point it's your responsibility to ensure the type is correct:

@snippet Ui.cpp AbstractUserInterface-layers-query

Visual layers commonly require additional constructor arguments and have
subclasses with GPU-API-specific implementations, the full setup is described
in documentation of each class. Similarly, how actual layer data are created
and attached to a node is specific to given layer. As another example, here's
how a drag handler on the `title` node could be made with the above-created
@ref EventLayer, resulting in the whole `panel` being moved on pointer drag:

@snippet Ui.cpp AbstractUserInterface-layers-create-data

A layer along with its instance implicitly stays alive until the end of the
@ref AbstractUserInterface lifetime. It can be removed earlier with
@ref removeLayer() if needed. If an instance has been set for it, it gets
deleted, and all its data are thus removed as well. The @ref LayerHandle then
becomes invalid. If you saved the reference to the instance, like shown above,
it'll become dangling with no way to check if it's still valid, so if the layer
may get removed early, be sure to always query it via its handle instead of
keeping a reference.

@subsection Ui-AbstractUserInterface-layers-order Layer visibility order

Contents of a single layer are drawn in an order
@ref Ui-AbstractUserInterface-nodes-order "defined by the node hierarchy"
explained above --- first by top-level node order, then in each hierarchy each
parent gets drawn before its children. But as multiple data from multiple
layers can be added to the same node (such as is the case with
the @m_class{m-label m-warning} **background** and
@m_class{m-label m-info} **text** for the `title` above, where the background
coming from @ref BaseLayer should be drawn under the text coming from
@ref TextLayer), an order has to be specified for those as well.

For performance reasons, it's desirable to draw as much from a single layer as
possible at once --- ideally the whole layer --- and thus the order is defined
between the whole layers, not between individual data. For example:

1.  `baseLayer` with backgrounds from @ref BaseLayer
2.  `lineLayer` for line art using @ref LineLayer
3.  `textLayer` with @ref TextLayer

If a certain piece of the UI needs a different order and
@ref Ui-AbstractUserInterface-nodes-order-nested "adding a nested top-level node" doesn't solve the problem (for example if you need a highlight rectangle or a
line *over* the text), a solution is to add extra instances of the same layer:

1.  `baseLayer` with backgrounds from @ref BaseLayer
2.  `lineLayer` for line art using @ref LineLayer
3.  `textLayer` with @ref TextLayer
4.  `overlayLayer` for highlight rectangles from @ref BaseLayer
5.  `overlayLineLayer` for highlight lines from @ref LineLayer

By default, the layers are drawn in the order @ref createLayer() is called, but
you can pass an optional @ref LayerHandle saying *behind* which existing layer
to put the new one. Unlike with top-level nodes, the layer order has to be
specified upfront and cannot be changed afterwards. For example, inserting the
`overlayLayer` between the `textLayer` and `overlayLayerLayer`:

@snippet Ui.cpp AbstractUserInterface-layers-order

@section Ui-AbstractUserInterface-renderer Renderer instance and compositing layers

Layers that draw have an implementation in a concrete GPU API (in particular,
for the builtin layers there's @ref BaseLayerGL, @ref TextLayerGL,
@ref LineLayerGL and @ref DebugLayerGL that draw using OpenGL), and each of
them may require different GPU state to be set for drawing, such as blending or
scissor. Enabling such state at appropriate times is handled by a *renderer*,
derived from @ref AbstractRenderer (and, in particular, @ref RendererGL for
OpenGL). In order to draw anything, an instance is expected to be set. The
@ref UserInterfaceGL class @ref Ui-UserInterfaceGL-setup-renderer "sets it up implicitly",
otherwise you're expected to pass it to @ref setRendererInstance().

@snippet Ui-gl.cpp AbstractUserInterface-renderer

Besides state handling, the renderer is also responsible for compositing, for
example if @ref Ui-BaseLayer-style-background-blur "background blur is enabled in BaseLayer".

@m_class{m-note m-success}

@par
    The renderer is a separate instance instead of being handled directly in
    the user interface internals as the @ref AbstractUserInterface is itself
    GPU-API-agnostic. Only the choice of concrete layers and renderer
    implementation defines what GPU API is actually used to put pixels on the
    screen.

@section Ui-AbstractUserInterface-events Event handling

Commonly, the UI is visible on top of any other content in the application
(such as a game, or a model / image in case of an editor or viewer) and so the
expectations is that input events should go to the UI first, as shown in the
snippets above. The event handler then returns @cpp true @ce if the event was
consumed by the UI (such as a button being pressed by a pointer, or a key press
causing an action on a focused input), @cpp false @ce if not and it should fall
through to the rest of the application or to the OS, browser etc.

@snippet Ui-sdl2.cpp AbstractUserInterface-events-application-fallthrough

@subsection Ui-AbstractUserInterface-events-propagation Node hierarchy event propagation

Inside the UI, pointer and position-dependent key events are directed to
concrete nodes based on their position. Top-level nodes get iterated in a
front-to-back order, and if the (appropriately scaled) event position is within
the rectangle of any of those, it recurses into given hierarchy, checking the
position against a rectangle of each child. The search continues until a leaf
node under given position is found, to which the event is directed.

On that node the user interface then iterates all data belonging to event
handling layers, and passes the @ref PointerEvent etc. instances to
corresponding layer interfaces. The layers can then *accept* the event on given
node via @ref PointerEvent::setAccepted() etc. If the event is accepted by any
layer, the event propagation stops there. If not, the propagation continues
through sibling nodes that are under given position, then back up the hierarchy
and then to other top-level node hierarchies until it's accepted. Example
propagation of a pointer event marked @m_class{m-label m-info} **blue**:

@m_class{m-row}

@parblock

@m_div{m-col-m-5 m-nopadt}
@htmlinclude ui-node-hierarchy-event.svg
@m_enddiv

@m_class{m-col-m-7 m-nopadt m-nopadl}

@par
    1.  Check its position against `titleTooltip` rectangle. It's outside,
        continue to the next top-level hierarchy.
    2.  Check against `panel`. Inside, go through child nodes.
        1.  Check against `title`. Outside.
        2.  Against `content`. Inside, no children, send event.
        3.  If accepted, return @cpp true @ce. If not, there are no other
            children, continue to the next top-level hierarchy.
    3.  Against `anotherPanel`. Inside, no children, send event.
    4.  If accepted, return @cpp true @ce. If not, there are no other top-level
        hierarchies, return @cpp false @ce.

@endparblock

@m_class{m-block m-danger}

@par Event propagation with overflowing node contents
    The above implies that if a node has contents or children that overflow its
    rectangle and it doesn't have @ref NodeFlag::Clip set, the overflowing
    contents will be visible but won't be able to react to events because they
    won't even get considered when propagating the events. The assumption is
    again that that most UI elements don't overflow and the additional cost and
    complexity of propagation to each and every potentially visible node isn't
    worth it.
@par
    A solution in this case is expanding the node area so it fully contains
    everything that's inside, and adjusting positioning of the contents if
    necessary.

If the event is accepted on any node, that particular event function returns
@cpp true @ce. If it isn't accepted on any node, or if there isn't any node at
given position in the first place, @cpp false @ce is returned.

@subsection Ui-AbstractUserInterface-events-capture Pointer capture, pressed and hovered node tracking

Especially with touch input where the aiming precision is rather coarse, it
commonly happens that the finger moves between a tap press and a release,
sometimes even outside of the tapped element. Another similar is when a mouse
cursor gets dragged outside of the narrow area of a scrollbar. In such cases it
would be annoying if the UI would cancel the action or, worse, performed some
other action entirely, and that's what *pointer capture* is designed to solve.

@htmlinclude ui-event-capture.svg

When a node @m_class{m-label m-info} **press** happens as a result of
@ref pointerPressEvent(), the node gets remembered. By default that enables
pointer capture, so if a @ref pointerMoveEvent() then drags the pointer
outside, the events are still sent to the original node (marked as *A* in the
above diagram), and *A* is still considered to be pressed, but not hovered. In
comparison, with capture disabled, a drag outside would lose the pressed state,
and the events would be sent to whatever other node is underneath (marked as
* *B* above), and said to be hovered on that node instead. Note that *B* isn't
considered to be pressed in that case in order to distinguish drags that
originated on given node from drags that originated outside.

When the (still pressed) pointer returns to *A*, in the captured case it's the
same as if it would never leave. A @m_class{m-label m-danger} **release** from
a @ref pointerReleaseEvent() could then generate a tap or click in given event
handler, for example. With capture disabled, a tap or click would happen only
if the pointer would never leave. Ultimately, pointer release implicitly
removes the capture again.

All this state is exposed via @ref PointerEvent::isNodePressed(), @relativeref{PointerEvent,isNodeHovered()} and
@relativeref{PointerEvent,isCaptured()} and similar properties in other event
classes for use by event handler implementations. For diagnostic purposes it's
also exposed via @ref currentPressedNode(), @ref currentHoveredNode() and
@ref currentCapturedNode(). Additionally, position of the previous pointer
event is tracked in @ref currentGlobalPointerPosition() and is used to fill in
the value of @ref PointerMoveEvent::relativePosition().

While pointer capture is a good default, in certain use cases such as drag &
drop it's desirable to know the node the pointer is being dragged to instead of
the events being always sent to the originating node. For that, the press and
move event handlers can toggle the capture using @ref PointerEvent::setCaptured()
/ @ref PointerMoveEvent::setCaptured().

@subsection Ui-AbstractUserInterface-events-focus Key and text input and node focus

By default, @ref keyPressEvent() and @ref keyReleaseEvent() is directed to a
node under pointer, based on the location at which the last pointer event
happened. This is useful for implementing workflows common in 2D/3D editing
software, where simply hovering a particular UI element and pressing a key
performs an action, without having to activate the element first by a click. As
with pointer events, if @ref KeyEvent::setAccepted() isn't called by the
handler, the functions return @cpp false @ce, signalling that the event should
be propagated elsewhere.

Text editing however has usually a different workflow --- clicking an input
element makes it *focused*, after which it accepts keyboard input regardless of
pointer position. Only nodes that are marked with @ref NodeFlag::Focusable can
be focused. Focus is then activated either by a pointer press on given node, or
programmatically by passing given node handle to @ref focusEvent(). Then
@ref keyPressEvent(), @ref keyReleaseEvent() as well as @ref textInputEvent()
are all directed to that node with no propagation anywhere else.

The node is then *blurred* by a pointer press outside of its area, or
programmatically by passing @ref NodeHandle::Null to @ref focusEvent(), after
which key events again go only to the node under cursor and
@ref textInputEvent() is ignored completely, returning @cpp false @ce always.
This behavior can be further tuned using @ref NodeFlag::NoBlur, where press on
such a node or its children will *not* cause the current focus to get lost,
which is useful for example when implementing virtual keyboards.

Information about whether a node the event is called on is focused is available
via @ref KeyEvent::isNodeFocused() and similarly on other event classes. For
diagnostic purposes it's also available through @ref currentFocusedNode().

@subsection Ui-AbstractUserInterface-events-fallthrough Pointer event fallthrough

By default, a pointer event is directed only to a single node, either as a
result of the hierarchy propagation, or because given node captures the event,
and as soon as the event is accepted it's not sent to any other node.

Sometimes it's however desirable to have events fall through to outer nodes,
for example to allow scrolling an area that's otherwise full of active
elements. This can be achieved by marking given node with
@ref NodeFlag::FallthroughPointerEvents. When a pointer event happens on any
child of such a node, the event is then sent to it as well, with
@ref PointerEvent::isFallthrough() / @ref PointerMoveEvent::isFallthrough()
being @cpp true @ce.

The node can then either just observe the event without accepting, in which
case the event outcome isn't any different compared to when the node isn't
marked with @ref NodeFlag::FallthroughPointerEvents. For a fallthrough event
the @ref PointerEvent::isNodePressed(), @relativeref{PointerEvent,isNodeHovered()}
and @relativeref{PointerEvent,isCaptured()} properties are inherited from the
original node, with the logic that if a child node was pressed or hovered, its
parent (whose area includes the child node) is pressed or hovered as well.

Or, instead of observing, the fallthrough event can be accepted. Accepting the
fallthrough event makes the node inherit the pressed, hovered and captured
properties from the original one, and the original node gets a
@ref PointerCancelEvent in exchange. The fallthrough works across nested
top-level nodes as well. Continuing from the event propagation example above
and assuming the `panel` node is marked as fallthrough, such as to implement a
"pull down to refresh" behavior, the event handling could look like this:

@m_class{m-row}

@parblock

@m_div{m-col-m-5 m-nopadt}
@htmlinclude ui-node-hierarchy-event-fallthrough.svg
@m_enddiv

@m_class{m-col-m-7 m-nopadt m-nopadl}

@par
    1.  Call a @m_class{m-label m-info} **press** event on the `titleTooltip`.
        It's accepted, thus it's captured and continues to fallthrough parent
        nodes.
        -   A fallthrough press on `panel`. Not accepted but the handler
            remembers its position.
    2.  Call a drag event on the captured `titleTooltip`. Continue to
        fallthrough parents.
        -   A fallthrough drag on `panel`. The handler accepts it if it was
            dragged far enough. The `panel` is now captured instead of
            `titleTooltip`, which gets a cancel event.
    3.  All further drags go directly to `panel`. The
        @m_class{m-label m-danger} **release** as well, which then removes the
        `panel` capture again.

@endparblock

It's possible for multiple nodes in a hierarchy chain to be marked with
@ref NodeFlag::FallthroughPointerEvents and the fallthrough events get sent to
all of them in reverse hierarchy order. Accepting then always sends the cancel
event to the node that previously accepted the event, either the original one
or a previous fallthrough event, and the pressed / hovered / captured node ends
up being set to the last node in the chain that accepted the event.

@section Ui-AbstractUserInterface-dpi DPI awareness

If you use the application integration shown above, in particular the
constructor and @ref setSize() overloads taking an application instance, the UI
is already made DPI-aware and no extra steps need to be done. Nevertheless, it
may be useful to know what happens underneath. There are three separate
concepts for DPI-aware UI rendering:

-   UI size --- size of the user interface to which all widgets are positioned
-   Window size --- size of the window to which all input events are related
-   Framebuffer size --- size of the framebuffer the UI is being rendered to

Depending on the platform and use case, each of these three values can be
different. For example, a game menu screen can have the UI size the same
regardless of window size. Or on Retina macOS you can have different window and
framebuffer size and the UI size might be related to window size but
independent on the framebuffer size.

With the @ref Platform::Sdl2Application "Platform::*Application" classes, you
usually have three values at your disposal ---
@relativeref{Platform::Sdl2Application,windowSize()},
@relativeref{Platform::Sdl2Application,framebufferSize()} and
@relativeref{Platform::Sdl2Application,dpiScaling()}. What the application
integration does internally is equivalent to the following --- the UI size
scales with window size and it's scaled based on either the framebuffer /
window size ratio or the DPI scaling value:

@snippet Ui-sdl2.cpp AbstractUserInterface-dpi-ratio

Or, for example, to have the UI size derived from window size while gracefully
dealing with extremely small or extremely large windows, you can apply
@ref Math::clamp() on top with some defined bounds. Windows outside of the
reasonable size range will then get a scaled version of the UI instead:

@snippet Ui-sdl2.cpp AbstractUserInterface-dpi-clamp

Ultimately, if you want the UI to have the same size and just scale with bigger
window sizes, pass a fixed value to the UI size:

@snippet Ui-sdl2.cpp AbstractUserInterface-dpi-fixed

With all variants above, the input events will still be dealt with correctly,
being scaled from the window coordinates to the actual UI size.
*/
class MAGNUM_UI_EXPORT AbstractUserInterface {
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
         * @param size              Size of the user interface to which
         *      everything is positioned
         * @param windowSize        Size of the window to which all input
         *      events are related
         * @param framebufferSize   Size of the window framebuffer. On some
         *      platforms with HiDPI screens may be different from window size.
         *
         * Equivalent to constructing with @ref AbstractUserInterface(NoCreateT)
         * and then calling @ref setSize(const Vector2&, const Vector2&, const Vector2i&).
         * See its documentation for more information.
         */
        explicit AbstractUserInterface(const Vector2& size, const Vector2& windowSize, const Vector2i& framebufferSize): AbstractUserInterface{NoCreate} {
            setSize(size, windowSize, framebufferSize);
        }

        /**
         * @brief Construct with properties taken from an application instance
         * @param application       Application instance to query properties
         *      from
         *
         * Equivalent to constructing with @ref AbstractUserInterface(NoCreateT)
         * and then calling @ref setSize(const ApplicationOrViewportEvent&).
         * See its documentation for more information.
         */
        template<class Application, class = decltype(Implementation::ApplicationSizeConverter<Application>::set(std::declval<AbstractUserInterface&>(), std::declval<const Application&>()))> explicit AbstractUserInterface(const Application& application): AbstractUserInterface{NoCreate} {
            setSize(application);
        }

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
         * other framebuffer-related properties, similarly the @p size is
         * passed through to @ref AbstractLayouter::setSize() to all layouters.
         *
         * There's no default size and this function is expected to be called
         * before the first @ref update() happens, either directly or through
         * the @ref AbstractUserInterface(const Vector2&, const Vector2&, const Vector2i&)
         * constructor. It's allowed to call this function for the first time
         * even after node, layers or data were created.
         *
         * Calling this function with new values will update the event position
         * scaling accordingly. If @p size or @p framebufferSize changes,
         * @ref AbstractLayer::setSize() is called on all layers; if @p size
         * changes, @ref AbstractLayouter::setSize() is called on all
         * layouters. If @p size changes and any nodes were already created,
         * @ref UserInterfaceState::NeedsNodeClipUpdate is set. If a renderer
         * instance is set, @ref AbstractRenderer::setupFramebuffers() is
         * called to make the renderer populate or update its internal state.
         * If a renderer instance isn't set yet when calling this function, the
         * framebuffer setup is performed in the next
         * @ref setRendererInstance() call instead.
         */
        AbstractUserInterface& setSize(const Vector2& size, const Vector2& windowSize, const Vector2i& framebufferSize);

        /**
         * @brief Set user interface size from an application or a viewport event instance
         * @param applicationOrViewportEvent    Application or a viewport event
         *      instance to query properties from
         *
         * Delegates to @ref setSize(const Vector2&, const Vector2&, const Vector2i&)
         * using window size, framebuffer size and DPI scaling queried from the
         * @p applicationOrViewportEvent instance. See its documentation,
         * @ref Ui-AbstractUserInterface-application and
         * @ref Ui-AbstractUserInterface-dpi for more information.
         */
        template<class ApplicationOrViewportEvent, class = decltype(Implementation::ApplicationSizeConverter<ApplicationOrViewportEvent>::set(std::declval<AbstractUserInterface&>(), std::declval<const ApplicationOrViewportEvent&>()))> AbstractUserInterface& setSize(const ApplicationOrViewportEvent& applicationOrViewportEvent) {
            Implementation::ApplicationSizeConverter<ApplicationOrViewportEvent>::set(*this, applicationOrViewportEvent);
            return *this;
        }

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

        /**
         * @brief Animation time
         *
         * Time value last passed to @ref advanceAnimations(). Initial value is
         * @cpp 0_nsec @ce.
         * @see @ref AbstractAnimator::time()
         */
        Nanoseconds animationTime() const;

        /** @{
         * @name Renderer management
         */

        /**
         * @brief Whether a renderer instance has been set
         *
         * @see @ref renderer(), @ref setRendererInstance()
         */
        bool hasRendererInstance() const;

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
         * @ref setSize() call.
         * @see @ref hasRendererInstance()
         */
        AbstractRenderer& setRendererInstance(Containers::Pointer<AbstractRenderer>&& instance);
        /** @overload */
        template<class T> T& setRendererInstance(Containers::Pointer<T>&& instance) {
            return static_cast<T&>(setRendererInstance(Containers::Pointer<AbstractRenderer>{Utility::move(instance)}));
        }

        /**
         * @brief Renderer instance
         *
         * Expects that @ref setRendererInstance() has been called.
         * @see @ref hasRendererInstance(), @ref UserInterfaceGL::renderer()
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
         *
         * See the @ref Ui-AbstractUserInterface-layers "Data layers" section
         * of the class documentation for more information.
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
         * @brief Generation counters for all layers
         *
         * Meant to be used for various diagnostic purposes such as tracking
         * handle recycling. Size of the returned view is the same as
         * @ref layerCapacity(), individual items correspond to generations of
         * particular layer IDs. All values fit into the @ref LayerHandle
         * generation bits, @cpp 0 @ce denotes an expired generation counter.
         *
         * Passing an ID along with the corresponding generation to
         * @ref layerHandle() produces a @ref LayerHandle. Use
         * @ref isHandleValid(LayerHandle) const to determine whether given
         * slot is actually used.
         */
        Containers::StridedArrayView1D<const UnsignedByte> layerGenerations() const;

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
         * @see @ref layerNext(), @ref layerLast(), @ref layerPrevious(),
         *      @ref Ui-AbstractUserInterface-layers-order
         */
        LayerHandle layerFirst() const;

        /**
         * @brief Last layer in draw and event processing order
         *
         * The last layer gets drawn last (thus is at the front) and reacts to
         * event before all others. Returns @ref LayerHandle::Null if there's
         * no layers yet. The returned handle is always either valid or null.
         * @see @ref layerPrevious(), @ref layerFirst(), @ref layerNext(),
         *      @ref Ui-AbstractUserInterface-layers-order
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
         *      @ref layerFirst(), @ref layerLast(),
         *      @ref Ui-AbstractUserInterface-layers-order
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
         *      @ref layerLast(), @ref layerFirst(),
         *      @ref Ui-AbstractUserInterface-layers-order
         */
        LayerHandle layerNext(LayerHandle handle) const;

        /**
         * @brief Create a layer
         * @param behind    A layer to order behind for draw and event
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
         *      @ref layerCapacity(), @ref layerUsedCount(),
         *      @ref Ui-AbstractUserInterface-layers
         */
        LayerHandle createLayer(LayerHandle behind =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            LayerHandle::Null
            #else
            LayerHandle{} /* To not have to include Handle.h */
            #endif
        );

        /**
         * @brief Whether an instance has been set for given layer
         *
         * Expects that @p handle is valid. Returns @cpp true @ce if
         * @ref setLayerInstance() has been already called for @p handle,
         * @cpp false @ce otherwise.
         * @see @ref layer()
         */
        bool hasLayerInstance(LayerHandle handle) const;

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
         *      @ref isHandleValid(LayerHandle) const,
         *      @ref hasLayerInstance(LayerHandle const,
         *      @ref Ui-AbstractUserInterface-layers
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
         * @see @ref isHandleValid(LayerHandle) const,
         *      @ref hasLayerInstance(LayerHandle) const,
         *      @ref Ui-AbstractUserInterface-layers
         */
        AbstractLayer& layer(LayerHandle handle);
        const AbstractLayer& layer(LayerHandle handle) const; /**< @overload */

        /**
         * @brief Layer instance in a concrete type
         *
         * Expects that @p handle is valid and that @ref setLayerInstance() was
         * called for it. It's the user responsibility to ensure that @p T
         * matches the actual instance type.
         * @see @ref isHandleValid(LayerHandle) const,
         *      @ref hasLayerInstance(LayerHandle) const,
         *      @ref Ui-AbstractUserInterface-layers
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
         * Animators with @ref AnimatorFeature::DataAttachment that were
         * associated with this layers are kept, but are excluded from any
         * further processing in @ref clean() or @ref advanceAnimations().
         * Calling @ref removeAnimator() on these is left to the user.
         *
         * Calling this function causes
         * @ref UserInterfaceState::NeedsDataAttachmentUpdate to be set.
         * @see @ref update(), @ref Ui-AbstractUserInterface-layers
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
         * @see @ref update(), @ref Ui-AbstractUserInterface-layers
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
         * @brief Generation counters for all layouters
         *
         * Meant to be used for various diagnostic purposes such as tracking
         * handle recycling. Size of the returned view is the same as
         * @ref layouterCapacity(), individual items correspond to generations
         * of particular layouter IDs. All values fit into the
         * @ref LayouterHandle generation bits, @cpp 0 @ce denotes an expired
         * generation counter.
         *
         * Passing an ID along with the corresponding generation to
         * @ref layerHandle() produces a @ref LayerHandle. Use
         * @ref isHandleValid(LayouterHandle) const to determine whether given
         * slot is actually used.
         */
        Containers::StridedArrayView1D<const UnsignedByte> layouterGenerations() const;

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
         * @brief Whether an instance has been set for given layouter
         *
         * Expects that @p handle is valid. Returns @cpp true @ce if
         * @ref setLayouterInstance() has been already called for @p handle,
         * @cpp false @ce otherwise.
         * @see @ref layouter()
         */
        bool hasLayouterInstance(LayouterHandle handle) const;

        /**
         * @brief Set a layouter instance
         * @return Reference to @p instance
         *
         * Expects that @p instance was created with a @ref LayouterHandle
         * returned from @ref createLayouter() earlier, the handle is valid and
         * @ref setLayouterInstance() wasn't called for the same handle yet.
         *
         * Calls @ref AbstractLayouter::setSize() on the layouter, unless
         * neither @ref setSize() nor @ref AbstractUserInterface(const Vector2&, const Vector2&, const Vector2i&)
         * was called yet.
         * @see @ref AbstractLayouter::handle(),
         *      @ref isHandleValid(LayouterHandle) const,
         *      @ref hasLayouterInstance(LayouterHandle) const
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
         * @see @ref isHandleValid(LayouterHandle) const,
         *      @ref hasLayouterInstance(LayouterHandle) const
         */
        AbstractLayouter& layouter(LayouterHandle handle);
        const AbstractLayouter& layouter(LayouterHandle handle) const; /**< @overload */

        /**
         * @brief Layouter instance in a concrete type
         *
         * Expects that @p handle is valid and that @ref setLayouterInstance()
         * was called for it. It's the user responsibility to ensure that @p T
         * matches the actual instance type.
         * @see @ref isHandleValid(LayouterHandle) const,
         *      @ref hasLayouterInstance(LayouterHandle) const
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
         * @name Animator management
         */

        /**
         * @brief Capacity of the animator storage
         *
         * Can be at most 256. If @ref createAnimator() is called and there's
         * no free slots left, the internal storage gets grown.
         * @see @ref animatorUsedCount()
         */
        std::size_t animatorCapacity() const;

        /**
         * @brief Count of used items in the animator storage
         *
         * Always at most @ref animatorCapacity(). Expired handles are counted
         * among used as well. The operation is done with a
         * @f$ \mathcal{O}(n) @f$ complexity where @f$ n @f$ is
         * @ref animatorCapacity().
         */
        std::size_t animatorUsedCount() const;

        /**
         * @brief Generation counters for all animators
         *
         * Meant to be used for various diagnostic purposes such as tracking
         * handle recycling. Size of the returned view is the same as
         * @ref animatorCapacity(), individual items correspond to generations
         * of particular animator IDs. All values fit into the
         * @ref AnimatorHandle generation bits, @cpp 0 @ce denotes an expired
         * generation counter.
         *
         * Passing an ID along with the corresponding generation to
         * @ref animatorHandle() produces an @ref AnimatorHandle. Use
         * @ref isHandleValid(AnimatorHandle) const to determine whether given
         * slot is actually used.
         */
        Containers::StridedArrayView1D<const UnsignedByte> animatorGenerations() const;

        /**
         * @brief Whether an animator handle is valid
         *
         * A handle is valid if it has been returned from @ref createAnimator()
         * before and @ref removeAnimator() wasn't called on it yet. Note that
         * a handle is valid even if the animator instance wasn't set with
         * @ref setGenericAnimatorInstance(), @ref setNodeAnimatorInstance(),
         * @ref setDataAnimatorInstance() or @ref setStyleAnimatorInstance()
         * yet. For @ref AnimatorHandle::Null always returns @cpp false @ce.
         */
        bool isHandleValid(AnimatorHandle handle) const;

        /**
         * @brief Whether an animation handle is valid
         *
         * A shorthand for extracting an @ref AnimatorHandle from @p handle
         * using @ref animationHandleAnimator(), calling
         * @ref isHandleValid(AnimatorHandle) const on it, if it's valid and
         * set then retrieving the particular animator instance using
         * @ref animator() and then calling
         * @ref AbstractAnimator::isHandleValid(AnimatorDataHandle) const with
         * an @ref AnimatorDataHandle extracted from @p handle using
         * @ref animationHandleData(). See these functions for more
         * information. For @ref AnimationHandle::Null,
         * @ref AnimatorHandle::Null or @ref AnimatorDataHandle::Null always
         * returns @cpp false @ce.
         */
        bool isHandleValid(AnimationHandle handle) const;

        /**
         * @brief Create an animator
         * @return New animator handle
         *
         * Allocates a new handle in a free slot in the internal storage or
         * grows the storage if there's no free slots left. Expects that
         * there's at most 256 animators. The returned handle is meant to be
         * used to construct an @ref AbstractAnimator subclass and the instance
         * then passed to @ref setGenericAnimatorInstance(),
         * @ref setNodeAnimatorInstance(), @ref setDataAnimatorInstance() or
         * @ref setStyleAnimatorInstance(). An animator can be removed again
         * with @ref removeAnimator().
         * @see @ref isHandleValid(AnimatorHandle) const,
         *      @ref animatorCapacity(), @ref animatorUsedCount()
         */
        AnimatorHandle createAnimator();

        /**
         * @brief Whether an instance has been set for given animator
         *
         * Expects that @p handle is valid. Returns @cpp true @ce if
         * @ref setGenericAnimatorInstance(), @ref setNodeAnimatorInstance(),
         * @ref setDataAnimatorInstance() or @ref setStyleAnimatorInstance()
         * has been already called for @p handle, @cpp false @ce otherwise.
         * @see @ref animator()
         */
        bool hasAnimatorInstance(AnimatorHandle handle) const;

        /**
         * @brief Set a generic animator instance
         * @return Reference to @p instance
         *
         * Expects that @p instance was created with an @ref AnimatorHandle
         * returned from @ref createAnimator() earlier, the handle is valid and
         * none of @ref setGenericAnimatorInstance(),
         * @ref setNodeAnimatorInstance(), @ref setDataAnimatorInstance() or
         * @ref setStyleAnimatorInstance() was called for the same handle yet.
         * Additionally, if @ref AnimatorFeature::DataAttachment is supported
         * by @p instance, expects that
         * @ref AbstractGenericAnimator::setLayer() has already been called on
         * it.
         *
         * Internally, the instance is inserted into a list partitioned by
         * animator type, which is done with a @f$ \mathcal{O}(n) @f$
         * complexity where @f$ n @f$ is @ref animatorCapacity().
         * @see @ref AbstractAnimator::handle(),
         *      @ref isHandleValid(AnimatorHandle) const,
         *      @ref hasAnimatorInstance(AnimatorHandle) const
         */
        AbstractGenericAnimator& setGenericAnimatorInstance(Containers::Pointer<AbstractGenericAnimator>&& instance);
        /** @overload */
        template<class T> T& setGenericAnimatorInstance(Containers::Pointer<T>&& instance) {
            return static_cast<T&>(setGenericAnimatorInstance(Containers::Pointer<AbstractGenericAnimator>{Utility::move(instance)}));
        }

        /**
         * @brief Set a node animator instance
         * @return Reference to @p instance
         *
         * Expects that @p instance was created with an @ref AnimatorHandle
         * returned from @ref createAnimator() earlier, the handle is valid and
         * none of @ref setGenericAnimatorInstance(),
         * @ref setNodeAnimatorInstance(), @ref setDataAnimatorInstance() or
         * @ref setStyleAnimatorInstance() was called for the same handle yet.
         * The @ref AbstractNodeAnimator is expected to advertise
         * @ref AnimatorFeature::NodeAttachment.
         *
         * Internally, the instance is inserted into a list partitioned by
         * animator type, which is done with a @f$ \mathcal{O}(n) @f$
         * complexity where @f$ n @f$ is @ref animatorCapacity().
         * @see @ref AbstractAnimator::handle(),
         *      @ref isHandleValid(AnimatorHandle) const,
         *      @ref hasAnimatorInstance(AnimatorHandle) const
         */
        AbstractNodeAnimator& setNodeAnimatorInstance(Containers::Pointer<AbstractNodeAnimator>&& instance);
        /** @overload */
        template<class T> T& setNodeAnimatorInstance(Containers::Pointer<T>&& instance) {
            return static_cast<T&>(setNodeAnimatorInstance(Containers::Pointer<AbstractNodeAnimator>{Utility::move(instance)}));
        }

        /**
         * @brief Set a data animator instance
         * @return Reference to @p instance
         *
         * Expects that @p instance was created with an @ref AnimatorHandle
         * returned from @ref createAnimator() earlier, the handle is valid and
         * none of @ref setGenericAnimatorInstance(),
         * @ref setNodeAnimatorInstance(), @ref setDataAnimatorInstance() or
         * @ref setStyleAnimatorInstance() was called for the same handle yet.
         * The @ref AbstractDataAnimator is expected to advertise
         * @ref AnimatorFeature::DataAttachment and it's expected that
         * @ref AbstractLayer::assignAnimator(AbstractDataAnimator&) const has
         * already been called for it.
         *
         * Internally, the instance is inserted into a list partitioned by
         * animator type, which is done with a @f$ \mathcal{O}(n) @f$
         * complexity where @f$ n @f$ is @ref animatorCapacity().
         * @see @ref AbstractAnimator::handle(),
         *      @ref isHandleValid(AnimatorHandle) const,
         *      @ref hasAnimatorInstance(AnimatorHandle) const
         */
        AbstractDataAnimator& setDataAnimatorInstance(Containers::Pointer<AbstractDataAnimator>&& instance);
        /** @overload */
        template<class T> T& setDataAnimatorInstance(Containers::Pointer<T>&& instance) {
            return static_cast<T&>(setDataAnimatorInstance(Containers::Pointer<AbstractDataAnimator>{Utility::move(instance)}));
        }

        /**
         * @brief Set a style animator instance
         * @return Reference to @p instance
         *
         * Expects that @p instance was created with an @ref AnimatorHandle
         * returned from @ref createAnimator() earlier, the handle is valid and
         * none of @ref setGenericAnimatorInstance(),
         * @ref setNodeAnimatorInstance(), @ref setDataAnimatorInstance() or
         * @ref setStyleAnimatorInstance() was called for the same handle yet.
         * The @ref AbstractStyleAnimator is expected to advertise
         * @ref AnimatorFeature::DataAttachment and it's expected that
         * @ref AbstractLayer::assignAnimator(AbstractStyleAnimator&) const has
         * already been called for it.
         *
         * Internally, the instance is inserted into a list partitioned by
         * animator type, which is done with a @f$ \mathcal{O}(n) @f$
         * complexity where @f$ n @f$ is @ref animatorCapacity().
         * @see @ref AbstractAnimator::handle(),
         *      @ref isHandleValid(AnimatorHandle) const,
         *      @ref hasAnimatorInstance(AnimatorHandle) const
         */
        AbstractStyleAnimator& setStyleAnimatorInstance(Containers::Pointer<AbstractStyleAnimator>&& instance);
        /** @overload */
        template<class T> T& setStyleAnimatorInstance(Containers::Pointer<T>&& instance) {
            return static_cast<T&>(setStyleAnimatorInstance(Containers::Pointer<AbstractStyleAnimator>{Utility::move(instance)}));
        }

        /**
         * @brief Animator instance
         *
         * Expects that @p handle is valid and that one of
         * @ref setGenericAnimatorInstance(), @ref setNodeAnimatorInstance(),
         * @ref setDataAnimatorInstance() or @ref setStyleAnimatorInstance()
         * was called for it.
         * @see @ref isHandleValid(AnimatorHandle) const,
         *      @ref hasAnimatorInstance(AnimatorHandle) const
         */
        AbstractAnimator& animator(AnimatorHandle handle);
        const AbstractAnimator& animator(AnimatorHandle handle) const; /**< @overload */

        /**
         * @brief Animator instance in a concrete type
         *
         * Expects that @p handle is valid and that one of
         * @ref setGenericAnimatorInstance(), @ref setNodeAnimatorInstance(),
         * @ref setDataAnimatorInstance() or @ref setStyleAnimatorInstance()
         * was called for it. It's the user responsibility to ensure that @p T
         * matches the actual instance type.
         * @see @ref isHandleValid(AnimatorHandle) const,
         *      @ref hasAnimatorInstance(AnimatorHandle) const
         */
        template<class T> T& animator(AnimatorHandle handle) {
            return static_cast<T&>(animator(handle));
        }
        /** @overload */
        template<class T> const T& animator(AnimatorHandle handle) const {
            return static_cast<const T&>(animator(handle));
        }

        /**
         * @brief Remove an animator
         *
         * Expects that @p handle is valid. After this call,
         * @ref isHandleValid(AnimatorHandle) const returns @cpp false @ce for
         * @p handle and @ref isHandleValid(AnimationHandle) const returns
         * @cpp false @ce for all animations associated with @p handle.
         *
         * Internally, if the removed animator had an instance set, the
         * instance is removed from a list partitioned by animator type, which
         * is done with a @f$ \mathcal{O}(n) @f$ complexity where @f$ n @f$ is
         * @ref animatorCapacity().
         * @see @ref clean()
         */
        void removeAnimator(AnimatorHandle handle);

        /**
         * @brief Attach an animation to a node
         *
         * A shorthand for extracting a @ref AnimatorHandle from @p animation
         * using @ref animationHandleAnimator(), retrieving the particular
         * animator instance using @ref animator() and then calling
         * @ref AbstractAnimator::attach(AnimatorDataHandle, NodeHandle) with a
         * @ref AnimatorDataHandle extracted with @ref animationHandleData().
         * See these functions for more information. In addition to
         * @ref AbstractAnimator::attach(AnimatorDataHandle, NodeHandle), this
         * function checks that @p node is either valid or
         * @ref NodeHandle::Null.
         */
        void attachAnimation(NodeHandle node, AnimationHandle animation);

        /**
         * @brief Attach an animation to a data
         *
         * A shorthand for extracting a @ref AnimatorHandle from @p animation
         * using @ref animationHandleAnimator(), retrieving the particular
         * animator instance using @ref animator() and then calling
         * @ref AbstractAnimator::attach(AnimatorDataHandle, DataHandle) with a
         * @ref AnimatorDataHandle extracted with @ref animationHandleData().
         * See these functions for more information. In addition to
         * @ref AbstractAnimator::attach(AnimatorDataHandle, DataHandle), this
         * function checks that @p data is either valid with the layer portion
         * matching @ref AbstractAnimator::layer() of given animator or
         * @ref DataHandle::Null.
         *
         * Note that unlike @ref AbstractAnimator::attach(AnimationHandle, LayerDataHandle),
         * here's no convenience function that would take a
         * @ref LayerDataHandle as it wouldn't be able to provide any extra
         * checks over calling the @ref AbstractAnimator API directly.
         */
        void attachAnimation(DataHandle data, AnimationHandle animation);

        /**
         * @}
         */

        /** @{
         * @name Node management
         *
         * See the @ref Ui-AbstractUserInterface-nodes "Node hierarchy" section
         * of the class documentation for more information.
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
         * @brief Generation counters for all nodes
         *
         * Meant to be used for various diagnostic purposes such as tracking
         * handle recycling. Size of the returned view is the same as
         * @ref nodeCapacity(), individual items correspond to generations of
         * particular node IDs. All values fit into the @ref NodeHandle
         * generation bits, @cpp 0 @ce denotes an expired generation counter.
         *
         * Passing an ID along with the corresponding generation to
         * @ref nodeHandle() produces a @ref NodeHandle. Use
         * @ref isHandleValid(NodeHandle) const to determine whether given slot
         * is actually used.
         */
        Containers::StridedArrayView1D<const UnsignedShort> nodeGenerations() const;

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
         *      @ref nodeUsedCount(), @ref update(),
         *      @ref Ui-AbstractUserInterface-nodes
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
         * it's a root node. Note that the returned handle may be invalid if
         * @ref removeNode() was called on one of the parent nodes and
         * @ref update() hasn't been called since.
         *
         * Unlike other node properties, the parent cannot be changed after
         * creation.
         * @see @ref isHandleValid(NodeHandle) const
         */
        NodeHandle nodeParent(NodeHandle handle) const;

        /**
         * @brief Node offset relative to its parent
         *
         * The returned value is before any layout calculation is done.
         * Absolute offset after all layout calculation is passed to
         * @ref AbstractLayer::doUpdate() and @ref AbstractLayer::doDraw().
         *
         * Expects that @p handle is valid.
         * @see @ref isHandleValid(NodeHandle) const, @ref nodeParent()
         */
        Vector2 nodeOffset(NodeHandle handle) const;

        /**
         * @brief Set node offset relative to its parent
         *
         * If the node has a layout assigned, the value is subsequently used
         * for layout calculation and it's up to the particular layouter
         * implementation how the initial offset value is used. If the node
         * doesn't have a layout assigned, the offset is used as-is. Expects
         * that @p handle is valid. Initially, a node has the offset that was
         * passed to @ref createNode().
         *
         * Calling this function causes
         * @ref UserInterfaceState::NeedsLayoutUpdate to be set.
         * @see @ref isHandleValid(NodeHandle) const, @ref nodeParent(),
         *      @ref Ui-AbstractUserInterface-nodes
         */
        void setNodeOffset(NodeHandle handle, const Vector2& offset);

        /**
         * @brief Node size
         *
         * The returned value is before any layout calculation is done. Size
         * after all layout calculations is available through
         * @ref PointerEvent::nodeSize(), @ref PointerMoveEvent::nodeSize(),
         * @ref KeyEvent::nodeSize() and is passed to
         * @ref AbstractLayer::doUpdate() and @ref AbstractLayer::doDraw().
         *
         * Expects that @p handle is valid.
         * @see @ref isHandleValid(NodeHandle) const
         */
        Vector2 nodeSize(NodeHandle handle) const;

        /**
         * @brief Set node size
         *
         * If the node has a layout assigned, the value is subsequently used
         * for layout calculation and it's up to the particular layouter
         * implementation how the initial size value is used. If the node
         * doesn't have a layout assigned, the size is used as-is. Expects that
         * @p handle is valid. Initially, a node has the size that was passed
         * to @ref createNode().
         *
         * Calling this function causes
         * @ref UserInterfaceState::NeedsLayoutUpdate to be set.
         * @see @ref isHandleValid(NodeHandle) const,
         *      @ref Ui-AbstractUserInterface-nodes
         */
        void setNodeSize(NodeHandle handle, const Vector2& size);

        /**
         * @brief Node opacity
         *
         * The returned value is only the opacity set on the node itself,
         * without considering opacity inherited from parents. Expects that
         * @p handle is valid.
         * @see @ref isHandleValid(NodeHandle) const
         */
        Float nodeOpacity(NodeHandle handle) const;

        /**
         * @brief Set node opacity
         *
         * The value is subsequently multiplied with opacity of all parents and
         * passed to layers to affect rendering. Opacity of @cpp 1.0f @ce makes
         * a node opaque and @cpp 0.0f @ce fully transparent, although values
         * outside of this range are allowed as well. Note that it's up to the
         * particular layer implementation how the opacity value is actually
         * used, and if at all --- the layer may for example have additional
         * options that affect the opacity either way. Expects that @p handle
         * is valid. Initially, a node has the opacity set to @cpp 1.0f @ce.
         *
         * Calling this function causes
         * @ref UserInterfaceState::NeedsNodeOpacityUpdate to be set.
         * @see @ref isHandleValid(NodeHandle) const,
         *      @ref Ui-AbstractUserInterface-nodes-opacity
         */
        void setNodeOpacity(NodeHandle handle, Float opacity);

        /**
         * @brief Node flags
         *
         * The returned value is only flags set directly on the node itself,
         * without considering flags such as @ref NodeFlag::Disabled that would
         * be inherited from parents. Expects that @p handle is valid.
         * @see @ref isHandleValid(NodeHandle) const, @ref setNodeFlags(),
         *      @ref addNodeFlags(), @ref clearNodeFlags()
         */
        NodeFlags nodeFlags(NodeHandle handle) const;

        /**
         * @brief Set node flags
         *
         * Expects that @p handle is valid. Initially, a node has the flags
         * that were passed to @ref createNode(), which are by default none.
         *
         * If @ref NodeFlag::Hidden was added or cleared by calling this
         * function, it causes @ref UserInterfaceState::NeedsNodeUpdate to be
         * set. If @ref NodeFlag::Clip was added or cleared by calling this
         * function, it causes @ref UserInterfaceState::NeedsNodeClipUpdate to
         * be set. If @ref NodeFlag::NoEvents, @ref NodeFlag::Disabled or
         * @ref NodeFlag::Focusable was added or cleared by calling this
         * function, it causes @ref UserInterfaceState::NeedsNodeEnabledUpdate
         * to be set. If @ref NodeFlag::NoBlur was added or cleared by calling
         * this function, it causes
         * @ref UserInterfaceState::NeedsNodeEventMaskUpdate to be set.
         * @see @ref isHandleValid(NodeHandle) const, @ref addNodeFlags(),
         *      @ref clearNodeFlags(),
         *      @ref Ui-AbstractUserInterface-nodes-flags
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
         * If @p handle is a top-level node, the operation is done with an
         * @f$ \mathcal{O}(n) @f$ complexity, where @f$ n @f$ is the count of
         * nested top-level hierarchies.
         *
         * Calling this function causes @ref UserInterfaceState::NeedsNodeClean
         * to be set.
         * @see @ref clean(), @ref nodeOrderLastNested(),
         *      @ref Ui-AbstractUserInterface-nodes
         */
        void removeNode(NodeHandle handle);

        /**
         * @}
         */

        /** @{
         * @name Top-level node draw and event processing order management
         *
         * See the @ref Ui-AbstractUserInterface-nodes-order "Top-level node hierarchies"
         * section of the class documentation for more information.
         */

        /**
         * @brief Capacity of the top-level node draw and event processing order storage
         *
         * @see @ref nodeOrderUsedCount()
         */
        std::size_t nodeOrderCapacity() const;

        /**
         * @brief Count of used items in the top-level node draw and event processing order storage
         *
         * Always at most @ref nodeOrderCapacity(). The operation is done
         * with a @f$ \mathcal{O}(n) @f$ complexity where @f$ n @f$ is
         * @ref nodeOrderCapacity(). When a root node is created or
         * @ref setNodeOrder() is called for the first time on a non-root node,
         * a slot in the node draw and event processing order storage is used
         * for it, and gets recycled only when the node is removed again or
         * when @ref flattenNodeOrder() is called on a non-root node.
         * @see @ref isNodeTopLevel()
         */
        std::size_t nodeOrderUsedCount() const;

        /**
         * @brief First top-level node in draw and event processing order
         *
         * The first node gets drawn first (thus is at the back) and reacts
         * to events after all others. Returns @ref NodeHandle::Null if
         * there's no nodes included in the draw and event processing order.
         * The returned handle is always either valid or null.
         * @see @ref nodeOrderNext(), @ref nodeOrderLast(),
         *      @ref nodeOrderPrevious(), @ref nodeOrderLastNested(),
         *      @ref isNodeTopLevel(), @ref isNodeOrdered(),
         *      @ref Ui-AbstractUserInterface-nodes-order
         */
        NodeHandle nodeOrderFirst() const;

        /**
         * @brief Last top-level node in draw and event processing order
         *
         * The last node gets drawn last (thus is at the front) and reacts to
         * events before all others. Returns @ref NodeHandle::Null if there's
         * no nodes included in the draw and event processing order. The
         * returned handle is always either valid or null.
         * @see @ref nodeOrderPrevious(), @ref nodeOrderFirst(),
         *      @ref nodeOrderNext(), @ref nodeOrderLastNested(),
         *      @ref isNodeOrdered(), @ref Ui-AbstractUserInterface-nodes-order
         */
        NodeHandle nodeOrderLast() const;

        /**
         * @brief Whether a node is top-level for draw and event processing
         *
         * If not top-level, it's in the usual draw and event processing order
         * defined by the node hierarchy. If top-level, it's ordered respective
         * to other top-level nodes. Expects that @p handle is valid.
         *
         * Always returns @cpp true @ce for root nodes. If @ref isNodeOrdered()
         * returns @cpp true @ce, it implies the node is top-level. If this
         * function returns @cpp true @ce, it doesn't necessarily mean the node
         * is visible --- the node or any of its parents could be excluded from
         * the draw and event processing order or it could be hidden.
         * @see @ref isHandleValid(NodeHandle) const, @ref nodeOrderNext(),
         *      @ref setNodeOrder(), @ref clearNodeOrder(),
         *      @ref flattenNodeOrder(),
         *      @ref Ui-AbstractUserInterface-nodes-order
         */
        bool isNodeTopLevel(NodeHandle handle) const;

        /**
         * @brief Whether a node is top-level and is included in a draw and event processing order
         *
         * If not included, the node and all its children are not drawn and
         * don't react to events. Expects that @p handle is valid.
         *
         * If this function returns @cpp true @ce, @ref isNodeTopLevel() is
         * @cpp true @ce as well. For non-root nodes the function returning
         * @cpp true @ce means the node is included in the draw and event
         * processing order respective to its parent top-level node. It doesn't
         * necessarily mean the node is visible --- the parents can themselves
         * be excluded from the draw and event processing order or they could
         * be hidden.
         * @see @ref isHandleValid(NodeHandle) const, @ref nodeOrderNext(),
         *      @ref setNodeOrder(), @ref clearNodeOrder(),
         *      @ref flattenNodeOrder(),
         *      @ref Ui-AbstractUserInterface-nodes-order
         */
        bool isNodeOrdered(NodeHandle handle) const;

        /**
         * @brief Previous node in draw and event processing order
         *
         * The previous node gets drawn earlier (thus is behind) and reacts
         * to events later. Expects that @p handle is valid. Returns
         * @ref NodeHandle::Null if the node is first in the draw and
         * processing order, is a root node that's not included in the draw and
         * event processing order, or is a non-root node that's drawn and
         * has events processed in a regular hierarchy-defined order. The
         * returned handle is always either valid or null.
         * @see @ref isHandleValid(NodeHandle) const, @ref nodeOrderNext(),
         *      @ref nodeOrderFirst(), @ref nodeOrderLast(),
         *      @ref isNodeOrdered(), @ref Ui-AbstractUserInterface-nodes-order
         */
        NodeHandle nodeOrderPrevious(NodeHandle handle) const;

        /**
         * @brief Next node in draw and event processing order
         *
         * The next node gets drawn later (thus is in front) and reacts to
         * events earlier. Expects that @p handle is valid. Returns
         * @ref NodeHandle::Null if the node is last in the draw and processing
         * order, is a root node that's not included in the draw and event
         * processing order, or is a non-root node that's drawn and has events
         * processed in a regular hierarchy-defined order. The returned handle
         * is always either valid or null.
         * @see @ref isHandleValid(NodeHandle) const, @ref nodeOrderPrevious(),
         *      @ref nodeOrderLast(), @ref nodeOrderLastNested(),
         *      @ref nodeOrderFirst(), @ref isNodeOrdered(),
         *      @ref Ui-AbstractUserInterface-nodes-order
         */
        NodeHandle nodeOrderNext(NodeHandle handle) const;

        /**
         * @brief Last node in draw and event processing order nested under this node
         *
         * Expects that @p handle is valid. If any children are top-level nodes
         * as well, points to the last of them, including any nested
         * top-level hierarchies. If there are no child top-level hierarchies,
         * returns @p handle itself.
         *
         * The draw and event processing order of child top-level hierarchies
         * relative to the @p handle gets preserved when modifying the
         * top-level draw and event processing order of @p handle using
         * @ref setNodeOrder() or @ref clearNodeOrder().
         * @see @ref isHandleValid(NodeHandle) const,
         *      @ref Ui-AbstractUserInterface-nodes-order
         */
        NodeHandle nodeOrderLastNested(NodeHandle handle) const;

        /**
         * @brief Order a top-level node for draw and event processing
         *
         * The @p handle gets ordered to be drawn earlier (thus behind) and
         * react to event later than @p behind. Expects that @p handle is valid
         * and @p behind is either @ref NodeHandle::Null or a valid node that's
         * included in the draw and event processing order.
         *
         * If @p handle is a root node, expects that @p behind is also a root
         * node, if not null. If null, the @p handle is ordered as drawn last.
         * The operation is done with an @f$ \mathcal{O}(1) @f$ complexity in
         * this case.
         *
         * If @p handle is not a root node, expects that both @p handle and
         * @p behind have a common parent that's the closest top-level or root
         * node for both, if @p behind is not null. If null, the @p handle is
         * ordered after all other top-level nodes under its closest top-level
         * or root parent. @ref NodeFlag::Hidden set on any parent node affects
         * the top-level node, @ref NodeFlag::Clip,
         * @relativeref{NodeFlag,NoEvents} or @relativeref{NodeFlag,Disabled}
         * doesn't. The operation is done with an @f$ \mathcal{O}(n) @f$
         * complexity in this case, where @f$ n @f$ is the depth at which
         * @p handle is in the node hierarchy.
         *
         * If the node was previously in a different position in the draw and
         * event processing order, it's moved, if it wasn't previously a
         * top-level node or if it wasn't included in the draw and event
         * processing order, it's inserted. Use @ref clearNodeOrder() to
         * exclude the node from the draw and event processing order afterwards
         * and @ref flattenNodeOrder() to integrate a non-root node back into
         * the usual draw and event processing order defined by the node
         * hierarchy.
         *
         * @m_class{m-note m-warning}
         *
         * @par
         *      At the moment, if a non-root node isn't top-level yet, it can
         *      be made top-level only if it isn't already included in a nested
         *      top-level draw and event processing order. If it is, call
         *      @ref clearNodeOrder() on the nested top-level nodes first (or
         *      @ref flattenNodeOrder() them), after that you can order them
         *      back.
         *
         * Calling this function causes @ref UserInterfaceState::NeedsNodeUpdate
         * to be set.
         * @see @ref isHandleValid(NodeHandle) const, @ref isNodeTopLevel(),
         *      @ref nodeOrderNext(), @ref nodeOrderLastNested(),
         *      @ref update(), @ref Ui-AbstractUserInterface-nodes-order
         */
        void setNodeOrder(NodeHandle handle, NodeHandle behind);

        /**
         * @brief Clear a node from the draw and event processing order
         *
         * Expects that @p handle is valid. If the node isn't top-level or
         * wasn't previously in the draw and processing order, the function is
         * a no-op. After calling this function, @ref isNodeOrdered() returns
         * @cpp false @ce for @p handle.
         *
         * If @p handle is a root node, the operation is done with an
         * @f$ \mathcal{O}(1) @f$ complexity. If @p handle is not a root node,
         * the operation is done with an @f$ \mathcal{O}(n) @f$ complexity,
         * where @f$ n @f$ is the depth at which @p handle is in the node
         * hierarchy.
         *
         * If the node contains any child top-level hierarchies, their draw and
         * event processing order relative to the @p handle gets preserved,
         * i.e. they get inserted back alongside it next time
         * @ref setNodeOrder() is called. Use @ref flattenNodeOrder() to
         * integrate a non-root node back into the usual draw and event
         * processing order defined by the node hierarchy.
         *
         * If not a no-op, calling this function causes
         * @ref UserInterfaceState::NeedsNodeUpdate to be set.
         * @see @ref isHandleValid(NodeHandle) const,
         *      @ref nodeOrderLastNested(), @ref update(),
         *      @ref Ui-AbstractUserInterface-nodes-order
         */
        void clearNodeOrder(NodeHandle handle);

        /**
         * @brief Flatten a non-root top-level node back to the usual draw and event processing order defined by the node hierarchy
         *
         * Expects that @p handle is valid and isn't a root node. Undoes the
         * operation done by calling @ref setNodeOrder() on a non-root node,
         * i.e. the node is again drawn alongside its neighbors. If the node
         * isn't top-level, the function is a no-op. After calling this
         * function, @ref isNodeTopLevel() returns @cpp false @ce for
         * @p handle.
         *
         * The operation is done with an @f$ \mathcal{O}(n) @f$ complexity,
         * where @f$ n @f$ is the depth at which @p handle is in the node
         * hierarchy.
         *
         * If the node contains nested top-level nodes, they stay top-level.
         * Use @ref clearNodeOrder() if you want to exclude a top-level node
         * including all its children from the draw and event processing order.
         *
         * If not a no-op, calling this function causes
         * @ref UserInterfaceState::NeedsNodeUpdate to be set.
         * @see @ref isHandleValid(NodeHandle) const, @ref update(),
         *      @ref Ui-AbstractUserInterface-nodes-order
         */
        void flattenNodeOrder(NodeHandle handle);

        /**
         * @}
         */

        /**
         * @brief Clean orphaned nodes, data and no longer valid data attachments
         * @return Reference to self (for method chaining)
         *
         * Called implicitly from @ref update() and subsequently also from
         * @ref draw() and all event processing functions. If @ref state()
         * contains neither @ref UserInterfaceState::NeedsNodeClean nor
         * @ref UserInterfaceState::NeedsDataClean, this function is a no-op,
         * otherwise it performs a subset of the following depending on the
         * state:
         *
         * -    Removes nodes with an invalid (removed) parent node
         * -    Calls @ref AbstractLayer::cleanNodes() with updated node
         *      generations, causing removal of data attached to invalid nodes
         * -    Calls @ref AbstractLayouter::cleanNodes() with updated node
         *      generations, causing removal of layouts assigned to invalid
         *      nodes
         * -    Calls @ref AbstractAnimator::cleanNodes() on all animators
         *      supporting @ref AnimatorFeature::NodeAttachment with updated
         *      node generations, causing removal of animations attached to
         *      invalid nodes
         * -    For all layers marked with @ref LayerState::NeedsDataClean
         *      calls @ref AbstractLayer::cleanData() with all animators
         *      supporting @ref AnimatorFeature::DataAttachment associated with
         *      given layer, causing removal of animations attached to invalid
         *      data
         *
         * After calling this function, @ref state() doesn't contain
         * @ref UserInterfaceState::NeedsNodeClean anymore;
         * @ref nodeUsedCount() and @ref AbstractLayer::usedCount() may get
         * smaller.
         */
        AbstractUserInterface& clean();

        /**
         * @brief Advance active animations
         * @return Reference to self (for method chaining)
         *
         * Implicitly calls @ref clean(), should be called before any
         * @ref update() or @ref draw() for given frame. Expects that @p time
         * is greater or equal to @ref animationTime(). If @ref state()
         * contains @ref UserInterfaceState::NeedsAnimationAdvance, this
         * function delegates to @ref AbstractAnimator::update() and then a
         * corresponding animator-specific advance function such as
         * @ref AbstractGenericAnimator::advance(),
         * @ref AbstractNodeAnimator::advance() or layer-specific
         * @ref AbstractLayer::advanceAnimations() on all animator instances
         * that have @ref AnimatorState::NeedsAdvance set.
         *
         * Calling this function updates @ref animationTime(). Afterwards,
         * @ref state() may still contain
         * @ref UserInterfaceState::NeedsAnimationAdvance, signalling that
         * animation advance is still needed the next fram. It may also have
         * other states added depending on what all the animators touched, and
         * a subsequent call to @ref draw() (or directly to @ref clean() and
         * @ref update() before that) for given frame will take care of
         * correctly updating the internal state.
         */
        AbstractUserInterface& advanceAnimations(Nanoseconds time);

        /**
         * @brief Update node hierarchy, data order and data contents for drawing and event processing
         * @return Reference to self (for method chaining)
         *
         * Expects that either @ref setSize() was called or the
         * @ref AbstractUserInterface(const Vector2&, const Vector2&, const Vector2i&)
         * constructor was used.
         *
         * Called implicitly from @ref draw() and all event processing
         * functions. If @ref state() contains none of
         * @ref UserInterfaceState::NeedsDataClean,
         * @relativeref{UserInterfaceState,NeedsNodeClean},
         * @relativeref{UserInterfaceState,NeedsDataUpdate},
         * @relativeref{UserInterfaceState,NeedsDataAttachmentUpdate},
         * @relativeref{UserInterfaceState,NeedsNodeEnabledUpdate},
         * @relativeref{UserInterfaceState,NeedsNodeClipUpdate},
         * @relativeref{UserInterfaceState,NeedsLayoutUpdate},
         * @relativeref{UserInterfaceState,NeedsLayoutAssignmentUpdate} or
         * @relativeref{UserInterfaceState,NeedsNodeUpdate}, this function is a
         * no-op, otherwise it performs a subset of the following depending on
         * the state, in order:
         *
         * -    In case @ref UserInterfaceState::NeedsDataClean or
         *      @ref UserInterfaceState::NeedsNodeClean is set, calls
         *      @ref clean()
         * -    Goes in a back to front order through layers that have
         *      instances set and calls @ref AbstractLayer::preUpdate() for
         *      layers that have @ref LayerState::NeedsCommonDataUpdate or
         *      @ref LayerState::NeedsSharedDataUpdate set to trigger updates
         *      that are not depending on a concrete set of visible nodes
         * -    In case the pre-update caused
         *      @ref UserInterfaceState::NeedsDataClean or
         *      @ref UserInterfaceState::NeedsNodeClean to be set again, calls
         *      @ref clean() for the second time
         * -    Orders visible nodes back-to-front for drawing and
         *      front-to-back for event processing
         * -    Orders layouts assigned to nodes by their dependency
         * -    Performs layout calculation
         * -    Calculates absolute offsets for visible nodes
         * -    Culls invisible nodes, calculates clip rectangles
         * -    Propagates @ref NodeFlag::Disabled and @ref NodeFlag::NoEvents
         *      to child nodes
         * -    Orders data attachments in each layer by draw order
         * -    Resets @ref currentPressedNode(), @ref currentCapturedNode(),
         *      @ref currentHoveredNode() or @ref currentFocusedNode() if they
         *      no longer exist
         * -    Calls @ref AbstractLayer::visibilityLostEvent() and resets
         *      @ref currentPressedNode(), @ref currentCapturedNode(),
         *      @ref currentHoveredNode() or @ref currentFocusedNode() if they
         *      are not visible or have @ref NodeFlag::NoEvents or
         *      @ref NodeFlag::Disabled set on them or their parents, or if the
         *      currently focused node is no longer @ref NodeFlag::Focusable.
         * -    Goes in a back to front order through layers that have
         *      instances set and calls @ref AbstractLayer::update() with the
         *      ordered data
         *
         * After calling this function, @ref state() is empty apart from
         * @ref UserInterfaceState::NeedsAnimationAdvance, which may be present
         * if there are any animators for which @ref advanceAnimations() should
         * be called.
         */
        AbstractUserInterface& update();

        /**
         * @brief Draw the user interface
         * @return Reference to self (for method chaining)
         *
         * Implicitly calls @ref update(), which in turn implicitly calls
         * @ref clean(). Performs the following:
         *
         * -    Calls @ref AbstractRenderer::transition() with
         *      @ref RendererTargetState::Initial
         * -    Peforms draw calls by going through draws collected by
         *      @ref update() for each top level node and all its visible
         *      children in a back to front order, and then for each layer that
         *      supports @ref LayerFeature::Draw in a back to front order:
         *      -   If a layer advertises @ref LayerFeature::Composite, calls
         *          @ref AbstractRenderer::transition() with
         *          @ref RendererTargetState::Composite, and then
         *          @ref AbstractLayer::composite()
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
         * @ref clean(). The @p globalPosition is assumed to be in respect to
         * @ref windowSize(), and is internally scaled to match @ref size()
         * before being set in the passed @ref PointerEvent instance.
         *
         * If the event is not primary and a node was captured by a previous
         * @ref pointerPressEvent(), @ref pointerMoveEvent() or a non-primary
         * @ref pointerReleaseEvent(), calls @ref AbstractLayer::pointerPressEvent()
         * on all data attached to that node belonging to layers that support
         * @ref LayerFeature::Event even if the event happens outside of its
         * area, with the event position made relative to the node.
         *
         * Otherwise, if either the event is primary or no node is captured,
         * propagates it to nodes under (scaled) @p globalPosition and
         * calls @ref AbstractLayer::pointerPressEvent() on all data attached
         * to it belonging to layers that support @ref LayerFeature::Event, in
         * a front-to-back order as described in
         * @ref Ui-AbstractUserInterface-events-propagation. For each such
         * node, the event is always called on all attached data, regardless of
         * the accept status. For each call the @ref PointerEvent::position()
         * is made relative to the node to which given data is attached.
         *
         * If the press happened with @ref Pointer::MouseLeft, primary
         * @ref Pointer::Finger or @ref Pointer::Pen, the currently focused
         * node is affected as well. If the node the event was accepted on is
         * different from currently focused node (if there's any),
         * @ref AbstractLayer::blurEvent() is called on all data attached to
         * that node. Then, if the node has @ref NodeFlag::Focusable set,
         * @ref AbstractLayer::focusEvent() is called on all data attached to
         * it. If any data accept the focus event, the node is treated as
         * focused and receives all following key events until a primary
         * @ref pointerPressEvent() outside of this node happens or until a
         * different node is made focused using @ref focusEvent(). If the node
         * is not @ref NodeFlag::Focusable or the focus event was not accepted
         * by any data, the currently focused node is reset. If a primary press
         * event happens on a @ref NodeFlag::Focusable node that's already
         * focused, @ref AbstractLayer::focusEvent() gets called on it again,
         * without any preceding @ref AbstractLayer::blurEvent(). If that focus
         * event is not accepted however, the node subsequently receives an
         * @ref AbstractLayer::blurEvent(). If the event isn't primary, the
         * current focused node isn't affected in any way.
         *
         * If the event is primary, the node that accepted the event is
         * remembered as pressed. The node that accepted the primary event also
         * implicitly captures all further pointer events until and including a
         * primary @ref pointerReleaseEvent() even if they happen outside of
         * its area, unless @ref PointerEvent::setCaptured() is called by the
         * implementation to disable this behavior. The capture
         * can be also removed from a later @ref pointerMoveEvent() or a
         * non-primary @ref pointerReleaseEvent(). Any node that was already
         * captured when calling this function with a primary event is ignored.
         * If the event isn't primary, the node is not rememebered as pressed
         * and neither it captures any further events.
         *
         * If no node accepted the event or there wasn't any visible event
         * handling node at given position, the previously remembered pressed
         * and captured nodes are reset if and only if the event is primary.
         *
         * After the above, if the event was accepted on any node or wasn't but
         * is still captured on any node, it falls through upwards the parent
         * hierarchy, calling @ref AbstractLayer::pointerPressEvent() on all
         * data attached to nodes that have
         * @ref NodeFlag::FallthroughPointerEvents set, with
         * @ref PointerEvent::isFallthrough() being @cpp true @ce and the
         * @relativeref{PointerEvent,isNodePressed()},
         * @relativeref{PointerEvent,isNodeHovered()},
         * @relativeref{PointerEvent,isNodeFocused()} and
         * @relativeref{PointerEvent,isCaptured()} properties inherited as
         * described in @ref Ui-AbstractUserInterface-events-fallthrough. If
         * any data accept the event, @ref AbstractLayer::pointerCancelEvent()
         * is called on all data attached to any previously captured node, and
         * if the event is primary, also any previously pressed, hovered or
         * focused nodes. The captured state is then transferred to the new
         * node, and for a primary event the pressed and hovered state is
         * transferred as well. However, currently, no
         * @ref AbstractLayer::pointerEnterEvent() is subsequently sent to the
         * new node. Focus is lost for a primary event unless the node that
         * accepted the fallthrough event is itself focused, in which case it
         * stays unchanged. This fallthrough then repeats until a root node is
         * reached, and each time an event is accepted on a new node it's
         * cancelled on the previous as appropriate.
         *
         * Returns @cpp true @ce if the press event was accepted by at least
         * one data, @cpp false @ce if it wasn't or there wasn't any visible
         * event handling node at given position and thus the event should be
         * propagated further. Accept status of the focus and blur events
         * doesn't have any effect on the return value.
         *
         * Expects that the event is not accepted yet. Presence of any
         * @ref Modifiers doesn't affect the behavior described above in any
         * way.
         * @see @ref PointerEvent::isPrimary(), @ref PointerEvent::isAccepted(),
         *      @ref PointerEvent::setAccepted(), @ref currentPressedNode(),
         *      @ref currentHoveredNode(), @ref currentCapturedNode(),
         *      @ref currentFocusedNode(), @ref currentGlobalPointerPosition()
         */
        bool pointerPressEvent(const Vector2& globalPosition, PointerEvent& event);

        /**
         * @brief Handle an external pointer press event
         *
         * Converts the @p event to a @ref PointerEvent and delegates to
         * @ref pointerPressEvent(const Vector2&, PointerEvent&), see its
         * documentation and @ref Ui-AbstractUserInterface-application for more
         * information. The @p args allow passing optional extra arguments to a
         * particular event converter.
         */
        template<class Event, class ...Args, class = decltype(Implementation::PointerEventConverter<Event>::press(std::declval<AbstractUserInterface&>(), std::declval<Event&>(), std::declval<Args>()...))> bool pointerPressEvent(Event& event, Args&&... args) {
            return Implementation::PointerEventConverter<Event>::press(*this, event, Utility::forward<Args>(args)...);
        }

        /**
         * @brief Handle a pointer release event
         *
         * Implicitly calls @ref update(), which in turn implicitly calls
         * @ref clean(). The @p globalPosition is assumed to be in respect to
         * @ref windowSize(), and is internally scaled to match @ref size()
         * before being set in the passed @ref PointerEvent instance.
         *
         * If a node was captured by a previous @ref pointerPressEvent(),
         * @ref pointerMoveEvent() or a non-primary @ref pointerReleaseEvent(),
         * calls @ref AbstractLayer::pointerReleaseEvent() on all data attached
         * to that node belonging to layers that support
         * @ref LayerFeature::Event even if the event happens outside of its
         * area, with the event position made relative to the node.
         *
         * Otherwise, if a node wasn't captured, propagates the event to nodes
         * under (scaled) @p globalPosition and calls
         * @ref AbstractLayer::pointerReleaseEvent() on all data attached to it
         * belonging to layers that support @ref LayerFeature::Event, in
         * a front-to-back order as described in
         * @ref Ui-AbstractUserInterface-events-propagation. For each such
         * node, the event is always called on all attached data, regardless of
         * the accept status. For each call the @ref PointerEvent::position()
         * is made relative to the node to which given data is attached.
         *
         * If the event is primary and a node is captured, the capture is
         * implicitly released after calling this function independently of
         * whether @ref PointerEvent::setCaptured() was set by the
         * implementation. If the event isn't primary, the current captured
         * node isn't affected by default, but the implementation can both
         * release the existing capture or make the containing node capture
         * future events by setting @ref PointerEvent::setCaptured().
         *
         * After the above, if the event was accepted on any node or wasn't but
         * is still captured on any node, it falls through upwards the parent
         * hierarchy, calling @ref AbstractLayer::pointerReleaseEvent() on all
         * data attached to nodes that have
         * @ref NodeFlag::FallthroughPointerEvents set, with
         * @ref PointerEvent::isFallthrough() being @cpp true @ce and the
         * @relativeref{PointerEvent,isNodePressed()},
         * @relativeref{PointerEvent,isNodeHovered()},
         * @relativeref{PointerEvent,isNodeFocused()} and
         * @relativeref{PointerEvent,isCaptured()} properties inherited as
         * described in @ref Ui-AbstractUserInterface-events-fallthrough. If
         * any data accept the event, @ref AbstractLayer::pointerCancelEvent()
         * is called on all data attached to any previously captured node, and
         * if the event is primary, also any previously pressed, hovered or
         * focused nodes. The captured state is then transferred to the new
         * node, and for a primary event the pressed and hovered state is
         * transferred as well. However, currently, no
         * @ref AbstractLayer::pointerEnterEvent() is subsequently sent to the
         * new node. Focus is lost for a primary event unless the node that
         * accepted the fallthrough event is itself focused, in which case it
         * stays unchanged. This fallthrough then repeats until a root node is
         * reached, and each time an event is accepted on a new node it's
         * cancelled on the previous as appropriate.
         *
         * Returns @cpp true @ce if the event was accepted by at least one
         * data, @cpp false @ce if it wasn't or there wasn't any visible event
         * handling node at given position and thus the event should be
         * propagated further.
         *
         * Expects that the event is not accepted yet. Presence of any
         * @ref Modifiers doesn't affect the behavior described above in any
         * way.
         * @see @ref PointerEvent::isPrimary(), @ref PointerEvent::isAccepted(),
         *      @ref PointerEvent::setAccepted(), @ref currentPressedNode(),
         *      @ref currentHoveredNode(), @ref currentCapturedNode(),
         *      @ref currentGlobalPointerPosition()
         */
        bool pointerReleaseEvent(const Vector2& globalPosition, PointerEvent& event);

        /**
         * @brief Handle an external pointer release event
         *
         * Converts the @p event to a @ref PointerEvent and delegates to
         * @ref pointerReleaseEvent(const Vector2&, PointerEvent&), see its
         * documentation and @ref Ui-AbstractUserInterface-application for more
         * information. The @p args allow passing optional extra arguments to a
         * particular event converter.
         */
        template<class Event, class ...Args, class = decltype(Implementation::PointerEventConverter<Event>::release(std::declval<AbstractUserInterface&>(), std::declval<Event&>(), std::declval<Args>()...))> bool pointerReleaseEvent(Event& event, Args&&... args) {
            return Implementation::PointerEventConverter<Event>::release(*this, event, Utility::forward<Args>(args)...);
        }

        /**
         * @brief Handle a pointer move event
         *
         * Implicitly calls @ref update(), which in turn implicitly calls
         * @ref clean(). The @p globalPosition is assumed to be in respect to
         * @ref windowSize(), and is internally scaled to match @ref size()
         * before being used to calculate relative position and before both get
         * set in the passed @ref PointerMoveEvent instance.
         *
         * If a node was captured by a previous @ref pointerPressEvent() or
         * @ref pointerMoveEvent(), @ref pointerReleaseEvent() wasn't called
         * yet and the node wasn't removed since, calls
         * @ref AbstractLayer::pointerMoveEvent() on all data attached to that
         * node belonging to layers that support @ref LayerFeature::Event even
         * if the event happens outside of its area, with the event position
         * made relative to the node. If the move event is primary, happened
         * inside the node area and is accepted by at least one data, the node
         * is treated as hovered, otherwise as not hovered. An
         * @ref AbstractLayer::pointerEnterEvent() or
         * @relativeref{AbstractLayer,pointerLeaveEvent()} is then called for
         * all data attached to the captured node if the node hover status
         * changed with the same @p event except for
         * @ref PointerMoveEvent::relativePosition() which is reset to a zero
         * vector. No corresponding leave / enter event is called for any other
         * node in this case. If the move event isn't primary, the current
         * hovered node isn't affected in any way. If the move, enter or leave
         * event implementations called @ref PointerMoveEvent::setCaptured()
         * resulting in it being @cpp false @ce, the capture is released after
         * this function, otherwise it stays unchanged. For primary events the
         * capture reset is performed regardless of the event accept status,
         * for non-primary events only if they're accepted.
         *
         * Otherwise, if a node wasn't captured, propagates the event to nodes
         * under (scaled) @p globalPosition and calls
         * @ref AbstractLayer::pointerMoveEvent() on all data attached to it
         * belonging to layers that support @ref LayerFeature::Event, in
         * a front-to-back order as described in
         * @ref Ui-AbstractUserInterface-events-propagation. For each such
         * node, the event is always called on all attached data, regardless of
         * the accept status. If the move event is primary, the node on which
         * the data accepted the event is then treated as hovered; if no data
         * accepted the event, there's no hovered node. For each call the
         * @ref PointerMoveEvent::position() is made relative to the node to
         * which given data is attached. If the currently hovered node changed,
         * an @ref AbstractLayer::pointerLeaveEvent() is then called for all
         * data attached to a previously hovered node if it exists, and then a
         * corresponding @ref AbstractLayer::pointerEnterEvent() is called for
         * all data attached to the currently hovered node if it exists, in
         * both cases with the same @p event except for
         * @ref PointerMoveEvent::position() which is made relative to the
         * particular node it's called on and
         * @ref PointerMoveEvent::relativePosition() which is reset to a zero
         * vector. If the move event isn't primary, the current hovered node
         * isn't affected in any way. If any accepted move event or any enter
         * event called @ref PointerMoveEvent::setCaptured() resulting in it
         * being @cpp true @ce, the containing node implicitly captures all
         * further pointer events until and including a
         * @ref pointerReleaseEvent() even if they happen outside of its area,
         * or until the capture is released in a @ref pointerMoveEvent() again.
         * Calling @ref PointerMoveEvent::setCaptured() in the leave event has
         * no effect in this case.
         *
         * After the above, if the event was accepted on any node or wasn't but
         * is still captured on any node, it falls through upwards the parent
         * hierarchy, calling @ref AbstractLayer::pointerMoveEvent() on all
         * data attached to nodes that have
         * @ref NodeFlag::FallthroughPointerEvents set, with
         * @ref PointerMoveEvent::isFallthrough() being @cpp true @ce and the
         * @relativeref{PointerMoveEvent,isNodePressed()},
         * @relativeref{PointerMoveEvent,isNodeHovered()},
         * @relativeref{PointerMoveEvent,isNodeFocused()} and
         * @relativeref{PointerMoveEvent,isCaptured()} properties inherited as
         * described in @ref Ui-AbstractUserInterface-events-fallthrough. If
         * any data accept the event, @ref AbstractLayer::pointerCancelEvent()
         * is called on all data attached to any previously captured node, and
         * if the event is primary, also any previously pressed, hovered or
         * focused nodes. The captured state is then transferred to the new
         * node, and for a primary event the pressed and hovered state is
         * transferred as well. However, currently, no
         * @ref AbstractLayer::pointerEnterEvent() is subsequently sent to the
         * new node. Focus is lost for a primary event unless the node that
         * accepted the fallthrough event is itself focused, in which case it
         * stays unchanged. This fallthrough then repeats until a root node is
         * reached, and each time an event is accepted on a new node it's
         * cancelled on the previous as appropriate.
         *
         * Returns @cpp true @ce if the event was accepted by at least one
         * data, @cpp false @ce if it wasn't or there wasn't any visible event
         * handling node at given position and thus the event should be
         * propagated further; accept status of the enter and leave events is
         * ignored.
         *
         * Expects that the event is not accepted yet. Presence of any
         * @ref Modifiers doesn't affect the behavior described above in any
         * way.
         * @see @ref PointerEvent::isAccepted(),
         *      @ref PointerEvent::setAccepted(), @ref currentPressedNode(),
         *      @ref currentHoveredNode(), @ref currentCapturedNode(),
         *      @ref currentGlobalPointerPosition()
         */
        bool pointerMoveEvent(const Vector2& globalPosition, PointerMoveEvent& event);

        /**
         * @brief Handle an external pointer move event
         *
         * Converts the @p event to a @ref PointerMoveEvent and delegates to
         * @ref pointerMoveEvent(const Vector2&, PointerMoveEvent&), see its
         * documentation and @ref Ui-AbstractUserInterface-application for more
         * information. The @p args allow passing optional extra arguments to a
         * particular event converter.
         */
        template<class Event, class ...Args, class = decltype(Implementation::PointerMoveEventConverter<Event>::move(std::declval<AbstractUserInterface&>(), std::declval<Event&>(), std::declval<Args>()...))> bool pointerMoveEvent(Event& event, Args&&... args) {
            return Implementation::PointerMoveEventConverter<Event>::move(*this, event, Utility::forward<Args>(args)...);
        }

        /**
         * @brief Handle a scroll event
         *
         * Implicitly calls @ref update(), which in turn implicitly calls
         * @ref clean(). The @p globalPosition is assumed to be in respect to
         * @ref windowSize(), and is internally scaled to match @ref size()
         * before being set in the passed @ref ScrollEvent instance.
         *
         * If a node was captured by a previous @ref pointerPressEvent(),
         * @ref pointerMoveEvent() or a non-primary @ref pointerReleaseEvent(),
         * calls @ref AbstractLayer::scrollEvent() on all data attached to that
         * node belonging to layers that support @ref LayerFeature::Event even
         * if the event happened outside of its area, with the event position
         * made relative to the node.
         *
         * Otherwise, if no node is captured, propagates it to nodes under
         * (scaled) @p globalPosition and calls @ref AbstractLayer::scrollEvent()
         * on all data attached to it belonging to layers that support
         * @ref LayerFeature::Event, in a front-to-back order as described in
         * @ref Ui-AbstractUserInterface-events-propagation. For each such
         * node, the event is always called on all attached data, regardless of
         * the accept status. For each call the @ref ScrollEvent::position() is
         * made relative to the node to which given data is attached.
         *
         * Unlike pointer events, scroll events don't have any effect on
         * @ref currentHoveredNode(), @ref currentPressedNode(),
         * @ref currentCapturedNode(), @ref currentFocusedNode() or
         * @ref currentGlobalPointerPosition().
         *
         * Returns @cpp true @ce if the event was accepted by at least one
         * data; @cpp false @ce if it wasn't or there wasn't any visible event
         * handling node at given position and thus the event should be
         * propagated further.
         *
         * Expects that the event is not accepted yet. Presence of any
         * @ref Modifiers doesn't affect the behavior described above in any
         * way.
         * @see @ref ScrollEvent::isAccepted(),
         *      @ref ScrollEvent::setAccepted()
         */
        bool scrollEvent(const Vector2& globalPosition, ScrollEvent& event);

        /**
         * @brief Handle an external scroll event
         *
         * Converts the @p event to a @ref ScrollEvent and delegates to
         * @ref scrollEvent(const Vector2&, ScrollEvent&), see its
         * documentation and @ref Ui-AbstractUserInterface-application for more
         * information. The @p args allow passing optional extra arguments to a
         * particular event converter.
         */
        template<class Event, class ...Args, class = decltype(Implementation::ScrollEventConverter<Event>::trigger(std::declval<AbstractUserInterface&>(), std::declval<Event&>(), std::declval<Args>()...))> bool scrollEvent(Event& event, Args&&... args) {
            return Implementation::ScrollEventConverter<Event>::trigger(*this, event, Utility::forward<Args>(args)...);
        }

        /**
         * @brief Handle a focus event
         *
         * Implicitly calls @ref update(), which in turn implicitly calls
         * @ref clean(). The @p node is expected to be either
         * @ref NodeHandle::Null or valid with @ref NodeFlag::Focusable set.
         *
         * If @p node is non-null and is or any of its parents are not visible
         * either due to @ref NodeFlag::Hidden set or due to the node hierarchy
         * not being in the top-level node order, or have
         * @ref NodeFlag::Disabled or @ref NodeFlag::NoEvents set, the function
         * is a no-op and returns @cpp false @ce. Note that this *does not*
         * apply to nodes that are clipped or otherwise out of view.
         *
         * If the @p node is visible and can be focused, calls
         * @ref AbstractLayer::focusEvent() on all attached data, regardless of
         * the accept status, and even if the current focused node is the same
         * as @p node. Then, if any of them accept the event, the node is set
         * as currently focused, and @ref AbstractLayer::blurEvent() is called
         * on the previously focused node if different from @p node. If none of
         * them accept the event and the previously focused node is different
         * from @p node, the previously focused node stays. If none of them
         * accept the event and the previously focused node is the same as
         * @p node, @ref AbstractLayer::blurEvent() is called on @p node and
         * the current focused node is set to @ref NodeHandle::Null.
         *
         * If @p node is @ref NodeHandle::Null and current focused node is
         * non-null, calls @ref AbstractLayer::blurEvent() on it and sets the
         * current focused node to @ref NodeHandle::Null.
         *
         * Returns @cpp true @ce if the @ref AbstractLayer::focusEvent() was
         * called at all and accepted by at least one data, @cpp false @ce
         * otherwise.
         *
         * Compared to @ref pointerPressEvent(), where pressing on a
         * non-focusable node blurs the previously focused node, this function
         * preserves the previously focused node if @p node is cannot be
         * focused for any of the above reasons. Given this function is meant
         * to be called by the application itself, this gives it more control
         * --- for example to try to focus the next active control, if there's
         * any, and stay on the previous one if not. To achieve the same
         * behavior as @ref pointerPressEvent(), call the function with
         * @ref NodeHandle::Null if a call with a non-null handle fails:
         *
         * @snippet Ui.cpp AbstractUserInterface-focusEvent-blur-if-not-focusable
         *
         * @see @ref currentFocusedNode()
         */
        bool focusEvent(NodeHandle node, FocusEvent& event);

        /**
         * @brief Handle a key press event
         *
         * Implicitly calls @ref update(), which in turn implicitly calls
         * @ref clean().
         *
         * If @ref currentFocusedNode() is not @ref NodeHandle::Null, calls
         * @ref AbstractLayer::keyPressEvent() on all data attached to it
         * belonging to layers that support @ref LayerFeature::Event.
         *
         * Otherwise, if a node was captured by a previous
         * @ref pointerPressEvent() or @ref pointerMoveEvent(),
         * @ref pointerReleaseEvent() wasn't called yet and the node wasn't
         * removed since, calls @ref AbstractLayer::keyPressEvent() on all data
         * attached to that node belonging to layers that support
         * @ref LayerFeature::Event.
         *
         * Otherwise, if no node is captured and
         * @ref currentGlobalPointerPosition() is not
         * @relativeref{Corrade,Containers::NullOpt}, propagates the event to
         * nodes under it and calls @ref AbstractLayer::keyPressEvent() on all
         * data attached to it belonging to layers that support
         * @ref LayerFeature::Event, in a front-to-back order as described in
         * @ref Ui-AbstractUserInterface-events-propagation. For each such
         * node, the event is always called on all attached data, regardless of
         * the accept status. For each call the event contains the
         * @ref currentGlobalPointerPosition() made relative to the node to
         * which given data is attached.
         *
         * Returns @cpp true @ce if the event was accepted by at least one
         * data; @cpp false @ce if it wasn't, if @ref currentFocusedNode() is
         * @ref NodeHandle::Null and there either wasn't any visible event
         * handling node at given position or @ref currentGlobalPointerPosition()
         * is @relativeref{Corrade,Containers::NullOpt}, and thus the event
         * should be propagated further.
         *
         * Expects that the event is not accepted yet.
         * @see @ref KeyEvent::isAccepted(), @ref KeyEvent::setAccepted()
         */
        bool keyPressEvent(KeyEvent& event);

        /**
         * @brief Handle an external key press event
         *
         * Converts the @p event to a @ref KeyEvent and delegates to
         * @ref keyPressEvent(KeyEvent&), see its documentation and
         * @ref Ui-AbstractUserInterface-application for more information.
         * The @p args allow passing optional extra arguments to a particular
         * event converter.
         */
        template<class Event, class ...Args, class = decltype(Implementation::KeyEventConverter<Event>::press(std::declval<AbstractUserInterface&>(), std::declval<Event&>(), std::declval<Args>()...))> bool keyPressEvent(Event& event, Args&&... args) {
            return Implementation::KeyEventConverter<Event>::press(*this, event, Utility::forward<Args>(args)...);
        }

        /**
         * @brief Handle a key release event
         *
         * Implicitly calls @ref update(), which in turn implicitly calls
         * @ref clean().
         *
         * If @ref currentFocusedNode() is not @ref NodeHandle::Null, calls
         * @ref AbstractLayer::keyReleaseEvent() on all data attached to it
         * belonging to layers that support @ref LayerFeature::Event.
         *
         * Otherwise, if @ref currentGlobalPointerPosition() is not
         * @relativeref{Corrade,Containers::NullOpt}, finds the front-most node
         * under it and calls @ref AbstractLayer::keyReleaseEvent() on all data
         * attached to it belonging to layers that support
         * @ref LayerFeature::Event. If no data accept the event, continues to
         * other nodes under the position in a front-to-back order and then to
         * parent nodes. For each such node, the event is always called on all
         * attached data, regardless of the accept status. For each call the
         * event contains the @ref currentGlobalPointerPosition() made relative
         * to the node to which given data is attached.
         *
         * Returns @cpp true @ce if the event was accepted by at least one
         * data; @cpp false @ce if it wasn't, if @ref currentFocusedNode() is
         * @ref NodeHandle::Null and there either wasn't any visible event
         * handling node at given position or @ref currentGlobalPointerPosition()
         * is @relativeref{Corrade,Containers::NullOpt}, and thus the event
         * should be propagated further.
         *
         * Expects that the event is not accepted yet.
         * @see @ref KeyEvent::isAccepted(), @ref KeyEvent::setAccepted()
         */
        bool keyReleaseEvent(KeyEvent& event);

        /**
         * @brief Handle an external key release event
         *
         * Converts the @p event to a @ref KeyEvent and delegates to
         * @ref keyReleaseEvent(KeyEvent&), see its documentation and
         * @ref Ui-AbstractUserInterface-application for more information.
         * The @p args allow passing optional extra arguments to a particular
         * event converter.
         */
        template<class Event, class ...Args, class = decltype(Implementation::KeyEventConverter<Event>::release(std::declval<AbstractUserInterface&>(), std::declval<Event&>(), std::declval<Args>()...))> bool keyReleaseEvent(Event& event, Args&&... args) {
            return Implementation::KeyEventConverter<Event>::release(*this, event, Utility::forward<Args>(args)...);
        }

        /**
         * @brief Handle a text input event
         *
         * Implicitly calls @ref update(), which in turn implicitly calls
         * @ref clean().
         *
         * If @ref currentFocusedNode() is not @ref NodeHandle::Null, calls
         * @ref AbstractLayer::textInputEvent() on all data attached to it
         * belonging to layers that support @ref LayerFeature::Event.
         *
         * Returns @cpp true @ce if the event was accepted by at least one
         * data; @cpp false @ce if it wasn't or if @ref currentFocusedNode() is
         * @ref NodeHandle::Null, and thus the event should be propagated
         * further.
         *
         * Expects that the event is not accepted yet.
         * @see @ref TextInputEvent::isAccepted(),
         *      @ref TextInputEvent::setAccepted()
         */
        bool textInputEvent(TextInputEvent& event);

        /**
         * @brief Handle an external text input event
         *
         * Converts the @p event to a @ref TextInputEvent and delegates to
         * @ref textInputEvent(TextInputEvent&), see its documentation and
         * @ref Ui-AbstractUserInterface-application for more information.
         * The @p args allow passing optional extra arguments to a particular
         * event converter.
         */
        template<class Event, class ...Args, class = decltype(Implementation::TextInputEventConverter<Event>::trigger(std::declval<AbstractUserInterface&>(), std::declval<Event&>(), std::declval<Args>()...))> bool textInputEvent(Event& event, Args&&... args) {
            return Implementation::TextInputEventConverter<Event>::trigger(*this, event, Utility::forward<Args>(args)...);
        }

        /**
         * @brief Node pressed by last pointer event
         *
         * Returns handle of a node that was under the pointer for the last
         * @ref pointerPressEvent(), the pointer wasn't released since and the
         * pointer is either captured on that node or didn't leave its area
         * since.
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
         * @see @ref currentCapturedNode(), @ref currentFocusedNode()
         */
        NodeHandle currentPressedNode() const;

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
        NodeHandle currentCapturedNode() const;

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
        NodeHandle currentHoveredNode() const;

        /**
         * @brief Node focused by last pointer or focus event
         *
         * Returns handle of a @ref NodeFlag::Focusable node that was under the
         * pointer for the last @ref pointerPressEvent() or for which
         * @ref focusEvent() was called and at least one data attached to such
         * node accepted the @ref AbstractLayer::focusEvent(). Once the node
         * becomes hidden (either due to @ref NodeFlag::Hidden set or due to
         * the node hierarchy not being in the top-level node order),
         * @ref NodeFlag::NoEvents or @ref NodeFlag::Disabled is set on it or
         * it loses @ref NodeFlag::Focusable, the data receive
         * @ref AbstractLayer::blurEvent() and this becomes
         * @ref NodeHandle::Null. It's also @ref NodeHandle::Null if no node
         * was focused yet or if the node or any of its
         * parents were removed or hidden and @ref update() was called since.
         *
         * The returned handle may be invalid if the node or any of its parents
         * were removed and @ref clean() wasn't called since.
         * @see @ref currentPressedNode()
         */
        NodeHandle currentFocusedNode() const;

        /**
         * @brief Position of last pointer event
         *
         * Returns a position passed to the last primary
         * @ref pointerPressEvent(), @ref pointerReleaseEvent() or
         * @ref pointerMoveEvent(), scaled to match @ref size() instead of
         * @ref windowSize(). If no primary pointer event happened yet, returns
         * @relativeref{Corrade,Containers::NullOpt}.
         * @see @ref PointerEvent::isPrimary(),
         *      @ref PointerMoveEvent::isPrimary()
         */
        Containers::Optional<Vector2> currentGlobalPointerPosition() const;

    private:
        /* Used by set*AnimatorInstance() */
        MAGNUM_UI_LOCAL AbstractAnimator& setAnimatorInstanceInternal(
            #ifndef CORRADE_NO_ASSERT
            const char* messagePrefix,
            #endif
            Containers::Pointer<AbstractAnimator>&& instance, Int type);
        /* Used by removeNode(), advanceAnimations() and clean() */
        MAGNUM_UI_LOCAL void removeNodeInternal(UnsignedInt id);
        /* Used by setNodeFlags(), addNodeFlags() and clearNodeFlags() */
        MAGNUM_UI_LOCAL void setNodeFlagsInternal(UnsignedInt id, NodeFlags flags);
        /* Used by removeNodeInternal(), setNodeOrder() and clearNodeOrder() */
        MAGNUM_UI_LOCAL bool clearNodeOrderInternal(NodeHandle handle);
        /* Used by *Event() functions */
        MAGNUM_UI_LOCAL void callVisibilityLostEventOnNode(NodeHandle node, VisibilityLostEvent& event, bool canBePressedOrHovering);
        template<void(AbstractLayer::*function)(UnsignedInt, FocusEvent&)> MAGNUM_UI_LOCAL bool callFocusEventOnNode(NodeHandle node, FocusEvent& event);
        template<void(AbstractLayer::*function)(UnsignedInt, KeyEvent&)> MAGNUM_UI_LOCAL bool callKeyEventOnNode(NodeHandle node, KeyEvent& event);
        MAGNUM_UI_LOCAL bool callTextInputEventOnNode(NodeHandle node, TextInputEvent& event);
        template<class Event, void(AbstractLayer::*function)(UnsignedInt, Event&)> MAGNUM_UI_LOCAL bool callEventOnNode(const Vector2& globalPositionScaled, NodeHandle node, NodeHandle targetNode, Event& event, bool rememberCaptureOnUnaccepted = false);
        template<class Event, void(AbstractLayer::*function)(UnsignedInt, Event&)> MAGNUM_UI_LOCAL bool callEventOnNode(const Vector2& globalPositionScaled, NodeHandle node, Event& event, bool rememberCaptureOnUnaccepted = false) {
            return callEventOnNode<Event, function>(globalPositionScaled, node, node, event, rememberCaptureOnUnaccepted);
        }
        template<class Event, void(AbstractLayer::*function)(UnsignedInt, Event&)> MAGNUM_UI_LOCAL NodeHandle callEvent(const Vector2& globalPositionScaled, UnsignedInt visibleNodeIndex, Event& event);
        template<class Event, void(AbstractLayer::*function)(UnsignedInt, Event&)> MAGNUM_UI_LOCAL NodeHandle callEvent(const Vector2& globalPositionScaled, Event& event);
        template<class Event, void(AbstractLayer::*function)(UnsignedInt, Event&)> MAGNUM_UI_LOCAL void callFallthroughPointerEvents(NodeHandle node, const Vector2& globalPositionScaled, Event& event, bool allowCapture);
        template<void(AbstractLayer::*function)(UnsignedInt, KeyEvent&)> MAGNUM_UI_LOCAL bool keyPressOrReleaseEvent(KeyEvent& event);

        struct State;
        Containers::Pointer<State> _state;
};

}}

#endif
