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

#include "DebugLayer.h"

#include <Corrade/Containers/BitArrayView.h>
#include <Corrade/Containers/EnumSet.hpp>
#include <Corrade/Containers/GrowableArray.h>
#include <Corrade/Containers/StridedArrayView.h>

#include "Magnum/Ui/AbstractAnimator.h"
#include "Magnum/Ui/AbstractLayouter.h"
#include "Magnum/Ui/Event.h"
#include "Magnum/Ui/NodeFlags.h"
#include "Magnum/Ui/UserInterface.h"
#include "Magnum/Ui/Implementation/debugLayerState.h"

namespace Magnum { namespace Ui {

using namespace Containers::Literals;
using namespace Math::Literals;

Debug& operator<<(Debug& debug, const DebugLayerSource value) {
    debug << "Ui::DebugLayerSource" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case DebugLayerSource::value: return debug << "::" #value;
        _c(Nodes)
        _c(Layers)
        _c(Layouters)
        _c(Animators)
        _c(NodeHierarchy)
        _c(NodeData)
        _c(NodeDataDetails)
        _c(NodeAnimations)
        _c(NodeAnimationDetails)
        _c(NodeLayouts)
        _c(NodeLayoutDetails)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "(" << Debug::nospace << Debug::hex << UnsignedShort(value) << Debug::nospace << ")";
}

Debug& operator<<(Debug& debug, const DebugLayerSources value) {
    /* All those are a superset of Nodes, so if at least two are present, print
       them separately as printing the regular way one would result in
       `Ui::DebugLayerSource::NodeHierarchy|Ui::DebugLayerSource::NodeData|Ui::DebugLayerSource::Animators|Ui::DebugLayerSource(0x40)`
       and similar in the output. If more than two are present, it'll remove
       them recursively one by one. */
    {
        DebugLayerSource exclude{};
        UnsignedInt count = 0;
        /* As the last matching one is taken, the list is in reverse order
           compared to the enum */
        for(DebugLayerSource i: {DebugLayerSource::NodeAnimations,
                                 DebugLayerSource::NodeAnimationDetails,
                                 DebugLayerSource::NodeLayouts,
                                 DebugLayerSource::NodeLayoutDetails,
                                 DebugLayerSource::NodeData,
                                 DebugLayerSource::NodeDataDetails,
                                 DebugLayerSource::NodeHierarchy}) {
            if(value >= i) {
                /* Only increase the count if the value is not a superset of
                   the previously remembered. Casting to DebugLayerSources as
                   otherwise >= isn't checking for a superset but for an
                   integer value. */
                if(exclude == DebugLayerSource{} || !(i >= DebugLayerSources{exclude}))
                    ++count;
                exclude = i;
            }
        }

        if(count >= 2) {
            return debug << exclude << Debug::nospace << "|" << Debug::nospace << ((value & ~exclude)|DebugLayerSource::Nodes);
        }
    }

    return Containers::enumSetDebugOutput(debug, value, "Ui::DebugLayerSources{}", {
        DebugLayerSource::NodeHierarchy,
        DebugLayerSource::NodeDataDetails,
        /* Implied by NodeDataDetails, has to be after */
        DebugLayerSource::NodeData,
        DebugLayerSource::NodeLayoutDetails,
        /* Implied by NodeLayoutDetails, has to be after */
        DebugLayerSource::NodeLayouts,
        DebugLayerSource::NodeAnimationDetails,
        /* Implied by NodeAnimationDetails, has to be after */
        DebugLayerSource::NodeAnimations,
        /* Implied by NodeHierarchy and NodeData, has to be after */
        DebugLayerSource::Nodes,
        /* Implied by NodeData, has to be after */
        DebugLayerSource::Layers,
        /* Implied by NodeLayouts, has to be after */
        DebugLayerSource::Layouters,
        /* Implied by NodeAnimations, has to be after */
        DebugLayerSource::Animators,
    });
}

Debug& operator<<(Debug& debug, const DebugLayerFlag value) {
    debug << "Ui::DebugLayerFlag" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case DebugLayerFlag::value: return debug << "::" #value;
        _c(NodeHighlight)
        _c(ColorOff)
        _c(ColorAlways)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "(" << Debug::nospace << Debug::hex << UnsignedByte(value) << Debug::nospace << ")";
}

Debug& operator<<(Debug& debug, const DebugLayerFlags value) {
    return Containers::enumSetDebugOutput(debug, value, "Ui::DebugLayerFlags{}", {
        DebugLayerFlag::NodeHighlight,
        DebugLayerFlag::ColorOff,
        DebugLayerFlag::ColorAlways,
    });
}

DebugLayer::State::State(const DebugLayerSources sources, const DebugLayerFlags flags): sources{sources}, flags{flags} {
    CORRADE_ASSERT(!(flags >= DebugLayerFlag::NodeHighlight) || sources >= DebugLayerSource::Nodes,
        "Ui::DebugLayer:" << DebugLayerSource::Nodes << "has to be enabled for" << DebugLayerFlag::NodeHighlight, );
}

/** @todo could also not allocate any state if no flags are set, to make it
    more efficient to have the debug layer created always and use it only if
    needed but the testing effort for all getters and setters is likely not
    worth it */
DebugLayer::DebugLayer(const LayerHandle handle, const DebugLayerSources sources, const DebugLayerFlags flags): DebugLayer{handle, Containers::pointer<State>(sources, flags)} {}

DebugLayer::DebugLayer(const LayerHandle handle, Containers::Pointer<State>&& state_): AbstractLayer{handle}, _state{Utility::move(state_)} {
    /* If we have node data attachments enabled, set the default name for this
       layer, and remember its handle so we know it's for this one. The
       allocation would get made in the next doPreUpdate() anyway so it's fine
       to do it here already, and assuming the debug layer is always added
       last it'll be already in its final size, without getting subsequently reallocated. */
    State& state = *_state;
    if(state.sources >= DebugLayerSource::Layers) {
        const UnsignedInt layerId = layerHandleId(handle);
        arrayResize(state.layers, layerId + 1);
        state.layers[layerId].handle = handle;
        state.layers[layerId].name = Containers::String::nullTerminatedGlobalView("Debug"_s);
    }
}

DebugLayer::DebugLayer(DebugLayer&&) noexcept = default;

DebugLayer::~DebugLayer() = default;

DebugLayer& DebugLayer::operator=(DebugLayer&&) noexcept = default;

DebugLayerSources DebugLayer::sources() const { return _state->sources; }

DebugLayerFlags DebugLayer::flags() const { return _state->flags; }

DebugLayer& DebugLayer::setFlags(const DebugLayerFlags flags) {
    State& state = *_state;
    CORRADE_ASSERT(!(flags >= DebugLayerFlag::NodeHighlight) || state.sources >= DebugLayerSource::Nodes,
        "Ui::DebugLayer::setFlags():" << DebugLayerSource::Nodes << "has to be enabled for" << DebugLayerFlag::NodeHighlight, *this);

    /* If a node is highlighted and NodeHighlight was cleared from flags,
       remove the highlight */
    if(state.currentHighlightedNode != NodeHandle::Null && (state.flags & DebugLayerFlag::NodeHighlight) && !(flags & DebugLayerFlag::NodeHighlight)) {
        state.currentHighlightedNode = NodeHandle::Null;

        /* If a highlight callback is set up, call it with an empty string to
           signal that the highlight is removed */
        if(state.nodeHighlightCallback)
            state.nodeHighlightCallback({});

        /* If we're drawing the highlight, trigger an update. No actual data
           upload needs to happen, it's just to schedule a redraw */
        /** @todo once NeedsDraw or some such exists, only the
            highlightedNodeDrawOffset needs to be cleared, update() doesn't
            need to be called */
        if(doFeatures() >= LayerFeature::Draw) {
            state.highlightedNodeDrawOffset = ~std::size_t{};
            setNeedsUpdate(LayerState::NeedsDataUpdate);
        }
    }

    state.flags = flags;

    /* If a node was highlighted and NodeHighlight isn't enabled anymore, call
       the callback */

    return *this;
}

DebugLayer& DebugLayer::addFlags(const DebugLayerFlags flags) {
    return setFlags(_state->flags|flags);
}

DebugLayer& DebugLayer::clearFlags(const DebugLayerFlags flags) {
    return setFlags(_state->flags & ~flags);
}

Containers::StringView DebugLayer::nodeName(const NodeHandle handle) const {
    /* If we're not part of a UI, there's no way to track nodes and so all
       nodes would be left at an empty name. Consider that an error. */
    CORRADE_ASSERT(hasUi(),
        "Ui::DebugLayer::nodeName(): layer not part of a user interface", {});
    CORRADE_ASSERT(handle != NodeHandle::Null,
        "Ui::DebugLayer::nodeName(): handle is null", {});
    const State& state = *_state;
    /* If the feature isn't enabled, do nothing */
    if(!(state.sources >= DebugLayerSource::Nodes))
        return {};

    const UnsignedInt nodeId = nodeHandleId(handle);
    if(nodeId < state.nodes.size() && state.nodes[nodeId].handle == handle)
        return state.nodes[nodeId].name;
    return {};
}

DebugLayer& DebugLayer::setNodeName(const NodeHandle handle, Containers::StringView name) {
    CORRADE_ASSERT(hasUi(),
        "Ui::DebugLayer::setNodeName(): layer not part of a user interface", *this);
    CORRADE_ASSERT(handle != NodeHandle::Null,
        "Ui::DebugLayer::setNodeName(): handle is null", *this);
    State& state = *_state;
    /* If the feature isn't enabled, do nothing */
    if(!(state.sources >= DebugLayerSource::Nodes))
        return *this;

    const UnsignedInt nodeId = nodeHandleId(handle);
    if(state.nodes.size() <= nodeId)
        arrayResize(state.nodes, ValueInit, nodeId + 1);

    Implementation::DebugLayerNode& node = state.nodes[nodeId];
    node.handle = handle;
    node.name = Containers::String::nullTerminatedGlobalView(name);

    return *this;
}

Containers::StringView DebugLayer::layerName(const LayerHandle handle) const {
    /* If we're not part of a UI, there's no way to track layers and so all
       layers would be left at an empty name. Consider that an error. */
    CORRADE_ASSERT(hasUi(),
        "Ui::DebugLayer::layerName(): debug layer not part of a user interface", {});
    CORRADE_ASSERT(handle != LayerHandle::Null,
        "Ui::DebugLayer::layerName(): handle is null", {});
    const State& state = *_state;
    /* If the feature isn't enabled, do nothing */
    if(!(state.sources >= DebugLayerSource::Layers))
        return {};

    const UnsignedInt layerId = layerHandleId(handle);
    if(layerId < state.layers.size() && state.layers[layerId].handle == handle)
        return state.layers[layerId].name;
    return {};
}

DebugLayer& DebugLayer::setLayerName(const AbstractLayer& instance, const Containers::StringView name) {
    CORRADE_ASSERT(hasUi(),
        "Ui::DebugLayer::setLayerName(): debug layer not part of a user interface", *this);
    CORRADE_ASSERT(ui().isHandleValid(instance.handle()) && &ui().layer(instance.handle()) == &instance,
        "Ui::DebugLayer::setLayerName(): layer not part of the same user interface", *this);
    State& state = *_state;
    /* If the feature isn't enabled, do nothing */
    if(!(state.sources >= DebugLayerSource::Layers))
        return *this;

    /* If there are not enough tracked layers, add. Otherwise replace the
       instance to correctly free any existing debug integration. */
    const UnsignedInt layerId = layerHandleId(instance.handle());
    if(state.layers.size() <= layerId)
        arrayResize(state.layers, ValueInit, layerId + 1);
    else
        state.layers[layerId] = Implementation::DebugLayerLayer{};

    Implementation::DebugLayerLayer& layer = state.layers[layerId];
    layer.handle = instance.handle();
    layer.name = Containers::String::nullTerminatedGlobalView(name);

    return *this;
}

void** DebugLayer::setLayerNameDebugIntegration(const AbstractLayer& instance, const Containers::StringView& name, void(*const deleter)(void*), void(*const print)(void*, Debug&, const AbstractLayer&, const Containers::StringView&, LayerDataHandle)) {
    /* This already enlarges _state->layers and frees previous integration
       instance if there's any, no need to do that here again. Well, unless it
       asserted, in which case bail. */
    setLayerName(instance, name);
    const UnsignedInt layerId = layerHandleId(instance.handle());
    State& state = *_state;
    #ifdef CORRADE_GRACEFUL_ASSERT
    if(layerId >= state.layers.size())
        return {};
    #endif

    Implementation::DebugLayerLayer& layer = state.layers[layerId];
    CORRADE_INTERNAL_DEBUG_ASSERT(!layer.integration && !layer.deleter && !layer.print);

    /* Save the integration only if node data details are wanted (as for
       example one might not want such amount of verbosity). If not, return
       null so the instance doesn't get allocated at all. */
    if(state.sources >= DebugLayerSource::NodeDataDetails) {
        layer.deleter = deleter;
        layer.print = print;
        return &layer.integration;
    } else return nullptr;
}

Containers::StringView DebugLayer::animatorName(const AnimatorHandle handle) const {
    /* If we're not part of a UI, there's no way to track animators and so all
       animators would be left at an empty name. Consider that an error. */
    CORRADE_ASSERT(hasUi(),
        "Ui::DebugLayer::animatorName(): debug layer not part of a user interface", {});
    CORRADE_ASSERT(handle != AnimatorHandle::Null,
        "Ui::DebugLayer::animatorName(): handle is null", {});
    const State& state = *_state;
    /* If the feature isn't enabled, do nothing */
    if(!(state.sources >= DebugLayerSource::Animators))
        return {};

    const UnsignedInt animatorId = animatorHandleId(handle);
    if(animatorId < state.animators.size() && state.animators[animatorId].handle == handle)
        return state.animators[animatorId].name;
    return {};
}

DebugLayer& DebugLayer::setAnimatorName(const AbstractAnimator& instance, const Containers::StringView name) {
    CORRADE_ASSERT(hasUi(),
        "Ui::DebugLayer::setAnimatorName(): debug layer not part of a user interface", *this);
    CORRADE_ASSERT(ui().isHandleValid(instance.handle()) && &ui().animator(instance.handle()) == &instance,
        "Ui::DebugLayer::setAnimatorName(): animator not part of the same user interface", *this);
    State& state = *_state;
    /* If the feature isn't enabled, do nothing */
    if(!(state.sources >= DebugLayerSource::Animators))
        return *this;

    /* If there are not enough tracked layers, add. Otherwise replace the
       instance to correctly free any existing debug integration. */
    const UnsignedInt animatorId = animatorHandleId(instance.handle());
    if(state.animators.size() <= animatorId)
        arrayResize(state.animators, ValueInit, animatorId + 1);
    else
        state.animators[animatorId] = Implementation::DebugLayerAnimator{};

    Implementation::DebugLayerAnimator& animator = state.animators[animatorId];
    animator.handle = instance.handle();
    animator.name = Containers::String::nullTerminatedGlobalView(name);

    return *this;
}

void** DebugLayer::setAnimatorNameDebugIntegration(const AbstractAnimator& instance, const Containers::StringView& name, void(*const deleter)(void*), void(*const print)(void*, Debug&, const AbstractAnimator&, const Containers::StringView&, AnimatorDataHandle)) {
    /* This already enlarges _state->animators and frees previous integration
       instance if there's any, no need to do that here again. Well, unless it
       asserted, in which case bail. */
    setAnimatorName(instance, name);
    const UnsignedInt animatorId = animatorHandleId(instance.handle());
    State& state = *_state;
    #ifdef CORRADE_GRACEFUL_ASSERT
    if(animatorId >= state.animators.size())
        return {};
    #endif

    Implementation::DebugLayerAnimator& animator = state.animators[animatorId];
    CORRADE_INTERNAL_DEBUG_ASSERT(!animator.integration && !animator.deleter && !animator.print);

    /* Save the integration only if node animation details are wanted (as for
       example one might not want such amount of verbosity). If not, return
       null so the instance doesn't get allocated at all. */
    if(state.sources >= DebugLayerSource::NodeAnimationDetails) {
        animator.deleter = deleter;
        animator.print = print;
        return &animator.integration;
    } else return nullptr;
}

Containers::StringView DebugLayer::layouterName(const LayouterHandle handle) const {
    /* If we're not part of a UI, there's no way to track layouters and so all
       layouters would be left at an empty name. Consider that an error. */
    CORRADE_ASSERT(hasUi(),
        "Ui::DebugLayer::layouterName(): debug layer not part of a user interface", {});
    CORRADE_ASSERT(handle != LayouterHandle::Null,
        "Ui::DebugLayer::layouterName(): handle is null", {});
    const State& state = *_state;
    /* If the feature isn't enabled, do nothing */
    if(!(state.sources >= DebugLayerSource::Layouters))
        return {};

    const UnsignedInt layouterId = layouterHandleId(handle);
    if(layouterId < state.layouters.size() && state.layouters[layouterId].handle == handle)
        return state.layouters[layouterId].name;
    return {};
}

DebugLayer& DebugLayer::setLayouterName(const AbstractLayouter& instance, const Containers::StringView name) {
    CORRADE_ASSERT(hasUi(),
        "Ui::DebugLayer::setLayouterName(): debug layer not part of a user interface", *this);
    CORRADE_ASSERT(ui().isHandleValid(instance.handle()) && &ui().layouter(instance.handle()) == &instance,
        "Ui::DebugLayer::setLayouterName(): layouter not part of the same user interface", *this);
    State& state = *_state;
    /* If the feature isn't enabled, do nothing */
    if(!(state.sources >= DebugLayerSource::Layouters))
        return *this;

    /* If there are not enough tracked layers, add. Otherwise replace the
       instance to correctly free any existing debug integration. */
    const UnsignedInt layouterId = layouterHandleId(instance.handle());
    if(state.layouters.size() <= layouterId)
        arrayResize(state.layouters, ValueInit, layouterId + 1);
    else
        state.layouters[layouterId] = Implementation::DebugLayerLayouter{};

    Implementation::DebugLayerLayouter& layouter = state.layouters[layouterId];
    layouter.handle = instance.handle();
    layouter.name = Containers::String::nullTerminatedGlobalView(name);

    return *this;
}

void** DebugLayer::setLayouterNameDebugIntegration(const AbstractLayouter& instance, const Containers::StringView& name, void(*const deleter)(void*), void(*const print)(void*, Debug&, const AbstractLayouter&, const Containers::StringView&, LayouterDataHandle)) {
    /* This already enlarges _state->layouters and frees previous integration
       instance if there's any, no need to do that here again. Well, unless it
       asserted, in which case bail. */
    setLayouterName(instance, name);
    const UnsignedInt layouterId = layouterHandleId(instance.handle());
    State& state = *_state;
    #ifdef CORRADE_GRACEFUL_ASSERT
    if(layouterId >= state.layouters.size())
        return {};
    #endif

    Implementation::DebugLayerLayouter& layouter = state.layouters[layouterId];
    CORRADE_INTERNAL_DEBUG_ASSERT(!layouter.integration && !layouter.deleter && !layouter.print);

    /* Save the integration only if node layout details are wanted (as for
       example one might not want such amount of verbosity). If not, return
       null so the instance doesn't get allocated at all. */
    if(state.sources >= DebugLayerSource::NodeLayoutDetails) {
        layouter.deleter = deleter;
        layouter.print = print;
        return &layouter.integration;
    } else return nullptr;
}

Color4 DebugLayer::nodeHighlightColor() const {
    return _state->nodeHighlightColor;
}

DebugLayer& DebugLayer::setNodeHighlightColor(const Color4& color) {
    _state->nodeHighlightColor = color;

    /* If this is a subclass that draws, trigger an update so the highlight
       rectangle is shown or hidden as appropriate */
    if(doFeatures() >= LayerFeature::Draw)
        setNeedsUpdate(LayerState::NeedsDataUpdate);

    return *this;
}

Containers::Pair<Pointers, Modifiers> DebugLayer::nodeHighlightGesture() const {
    return {_state->nodeHighlightPointers, _state->nodeHighlightModifiers};
}

DebugLayer& DebugLayer::setNodeHighlightGesture(const Pointers pointers, const Modifiers modifiers) {
    CORRADE_ASSERT(pointers,
        "Ui::DebugLayer::setNodeHighlightGesture(): expected at least one pointer", *this);
    _state->nodeHighlightPointers = pointers;
    _state->nodeHighlightModifiers = modifiers;
    return *this;
}

bool DebugLayer::hasNodeHighlightCallback() const {
    return !!_state->nodeHighlightCallback;
}

DebugLayer& DebugLayer::setNodeHighlightCallback(Containers::Function<void(Containers::StringView)>&& callback) {
    _state->nodeHighlightCallback = Utility::move(callback);
    return *this;
}

NodeHandle DebugLayer::currentHighlightedNode() const {
    const State& state = *_state;
    CORRADE_ASSERT(state.flags >= DebugLayerFlag::NodeHighlight,
        "Ui::DebugLayer::currentHighlightedNode():" << DebugLayerFlag::NodeHighlight << "not enabled", {});
    return state.currentHighlightedNode;
}

bool DebugLayer::highlightNode(const NodeHandle handle) {
    State& state = *_state;
    CORRADE_ASSERT(state.flags >= DebugLayerFlag::NodeHighlight,
        "Ui::DebugLayer::highlightNode():" << DebugLayerFlag::NodeHighlight << "not enabled", {});
    CORRADE_ASSERT(hasUi(),
        "Ui::DebugLayer::highlightNode(): layer not part of a user interface", {});
    const AbstractUserInterface& ui = this->ui();

    /* If this is a subclass that draws, trigger an update so the highlight
       rectangle is shown or hidden as appropriate */
    if(doFeatures() >= LayerFeature::Draw)
        setNeedsUpdate(LayerState::NeedsDataUpdate);

    /* If the handle is null or unknown, reset the currently highlighted node
       call the callback with an empty string. Return true only for null, false
       indicates unknown node. */
    if(handle == NodeHandle::Null || nodeHandleId(handle) >= state.nodes.size() || state.nodes[nodeHandleId(handle)].handle != handle) {
        if(state.currentHighlightedNode != NodeHandle::Null) {
            state.currentHighlightedNode = NodeHandle::Null;
            if(state.nodeHighlightCallback)
                state.nodeHighlightCallback({});
        }
        return handle == NodeHandle::Null;
    }

    /* Scope the (optional) output redirection to prevent it from being active
       even during the callback at the end, which could cause strange memory
       corruption issues if the callback attempts to print to Debug as well */
    Containers::String out;
    {
        /* Disable colors if:
            - ColorOff is set, which has the precedence over everything else
            - ColorAlways isn't set, and
                - Either we have a callback output (which obv. isn't a TTY)
                - Or we have an output that isn't a TTY */
        const Debug::Flags disableColors =
            state.flags >= DebugLayerFlag::ColorOff || (!(state.flags >= DebugLayerFlag::ColorAlways) && (state.nodeHighlightCallback || !Debug::isTty())) ?
                Debug::Flag::DisableColors : Debug::Flags{};
        Debug debug = state.nodeHighlightCallback ?
            Debug{&out, Debug::Flag::NoNewlineAtTheEnd|disableColors} :
            Debug{Debug::Flag::NoNewlineAtTheEnd|disableColors};
        const Implementation::DebugLayerNode& node = state.nodes[nodeHandleId(handle)];
        CORRADE_INTERNAL_ASSERT(node.handle == handle);
        debug << Debug::boldColor(Debug::Color::Default) << (ui.isNodeTopLevel(handle) ? "Top-level node" : "Node") << Debug::resetColor << Debug::packed << handle;
        /* `magnum-sceneconverter --info` etc. print a `:` after resource ID
           and before name, but here it'd be ideally without a `:` if neither
           node hierarchy nor any attachments exist, and the logic for that
           would be too complex and annoying to test. Moreover, the packed
           handle printing is already specific enough to not be mistakenly
           treated as part of the name, so not having a `:` should be fine. */
        if(node.name)
            debug << Debug::boldColor(Debug::Color::Yellow) << node.name << Debug::resetColor;
        debug << Debug::newline;
        if(const NodeFlags flags = ui.nodeFlags(handle))
            debug << "  Flags:" << Debug::color(Debug::Color::Cyan) << Debug::packed << flags << Debug::resetColor << Debug::newline;

        if(state.sources >= DebugLayerSource::NodeHierarchy) {
            /* Calculate hierarchy depth */
            UnsignedInt depth = 0;
            {
                NodeHandle parent = ui.nodeParent(handle);
                while(parent != NodeHandle::Null) {
                    parent = ui.nodeParent(parent);
                    ++depth;
                }
            }

            /* Calculate child count. Done by linearly going over all nodes and
               picking ones that have this node set as a parent. It's "fine" to
               do it like this for just one node total, but if children for
               more nodes will eventually get queried, then it's better to use
               the algorithms from AbstractUserInterface internals. */
            UnsignedInt childCount = 0;
            UnsignedInt hiddenChildCount = 0;
            UnsignedInt disabledChildCount = 0;
            UnsignedInt noEventChildCount = 0;
            for(const Implementation::DebugLayerNode& childNode: state.nodes) {
                /* Skip nodes that are null (i.e., free slots after removed
                   nodes), nodes that aren't valid (if highlightNode() is
                   called, there may be nodes that are already removed but
                   DebugLayer doesn't know about that yet), and nodes that
                   aren't children of this node */
                if(childNode.handle == NodeHandle::Null ||
                   !ui.isHandleValid(childNode.handle) ||
                   ui.nodeParent(childNode.handle) != handle)
                    continue;

                ++childCount;
                if(ui.nodeFlags(childNode.handle) >= Ui::NodeFlag::Hidden)
                    ++hiddenChildCount;
                else if(ui.nodeFlags(childNode.handle) >= Ui::NodeFlag::Disabled)
                    ++disabledChildCount;
                else if(ui.nodeFlags(childNode.handle) >= Ui::NodeFlag::NoEvents)
                    ++noEventChildCount;
            }

            if(depth == 0)
                debug << "  Root node";
            else
                debug << "  Nested at level" << depth;
            debug << "with" << childCount << "direct children" << Debug::newline;
            if(hiddenChildCount)
                debug << "    of which" << hiddenChildCount << Debug::color(Debug::Color::Cyan) << Debug::packed << NodeFlag::Hidden << Debug::resetColor << Debug::newline;
            if(disabledChildCount)
                debug << "    of which" << disabledChildCount << Debug::color(Debug::Color::Cyan) << Debug::packed << NodeFlag::Disabled << Debug::resetColor << Debug::newline;
            if(noEventChildCount)
                debug << "    of which" << noEventChildCount << Debug::color(Debug::Color::Cyan) << Debug::packed << NodeFlag::NoEvents << Debug::resetColor << Debug::newline;
        }

        if(state.sources >= DebugLayerSource::NodeData) {
            UnsignedInt otherLayerCount = 0;
            UnsignedInt otherDataCount = 0;
            bool hasNamedLayers = false;
            for(LayerHandle layerHandle = ui.layerFirst(); layerHandle != LayerHandle::Null; layerHandle = ui.layerNext(layerHandle)) {
                /* Skip the debug layer itself, layers that have no instance
                   and layers we don't know about yet (if highlightNode() is
                   called, there may be layers that are yet unknown to the
                   DebugLayer, either ones with IDs outside of the state.layers
                   bounds or ones that got removed and the slot reused for
                   others). Since we're iterating over UI's own layer order the
                   handles should be all valid. */
                CORRADE_INTERNAL_DEBUG_ASSERT(ui.isHandleValid(layerHandle));
                const UnsignedInt layerId = layerHandleId(layerHandle);
                if(layerHandle == this->handle() ||
                   !ui.hasLayerInstance(layerHandle) ||
                   layerId >= state.layers.size() ||
                   state.layers[layerId].handle != layerHandle)
                    continue;

                const Implementation::DebugLayerLayer& layer = state.layers[layerId];
                const AbstractLayer& layerInstance = ui.layer(layerHandle);
                bool hasOtherDataFromThisLayer = false;

                const UnsignedInt dataCapacity = layerInstance.capacity();
                const Containers::StridedArrayView1D<const UnsignedShort> dataGenerations = layerInstance.generations();
                UnsignedInt namedLayerDataCount = 0;
                for(UnsignedInt dataId = 0; dataId != dataCapacity; ++dataId) {
                    const LayerDataHandle data = layerDataHandle(dataId, dataGenerations[dataId]);
                    if(layerInstance.isHandleValid(data) && layerInstance.node(data) == handle) {
                        if(layer.print) {
                            hasNamedLayers = true;
                            layer.print(layer.integration, debug, layerInstance, layer.name, data);
                        } else if(layer.name) {
                            hasNamedLayers = true;
                            ++namedLayerDataCount;
                        } else {
                            hasOtherDataFromThisLayer = true;
                            ++otherDataCount;
                        }
                    }
                }

                if(namedLayerDataCount) {
                    debug << " " << namedLayerDataCount << "data from layer" << Debug::packed << layerHandle << Debug::color(Debug::Color::Yellow) << layer.name << Debug::resetColor << Debug::newline;
                }

                if(hasOtherDataFromThisLayer)
                    ++otherLayerCount;
            }

            if(otherDataCount)
                debug << " " << otherDataCount << "data from" << otherLayerCount << (hasNamedLayers ? "other layers" : "layers") << Debug::newline;
            else CORRADE_INTERNAL_ASSERT(otherLayerCount == 0);
        }

        if(state.sources >= DebugLayerSource::NodeLayouts) {
            UnsignedInt otherLayouterCount = 0;
            UnsignedInt otherLayoutCount = 0;
            bool hasNamedLayouters = false;
            for(LayouterHandle layouterHandle = ui.layouterFirst(); layouterHandle != LayouterHandle::Null; layouterHandle = ui.layouterNext(layouterHandle)) {
                /* Skip layouters that have no instance and layouters we don't
                   know about yet (if highlightNode() is called, there may be
                   layouters that are yet unknown to the DebugLayer, either
                   ones with IDs outside of the state.layouters bounds or ones
                   that got removed and the slot reused for others). Since
                   we're iterating over UI's own layouter order the handles
                   should be all valid. */
                CORRADE_INTERNAL_DEBUG_ASSERT(ui.isHandleValid(layouterHandle));
                const UnsignedInt layouterId = layouterHandleId(layouterHandle);
                if(!ui.hasLayouterInstance(layouterHandle) ||
                   layouterId >= state.layouters.size() ||
                   state.layouters[layouterId].handle != layouterHandle)
                    continue;

                const Implementation::DebugLayerLayouter& layouter = state.layouters[layouterId];
                const AbstractLayouter& layouterInstance = ui.layouter(layouterHandle);
                bool hasOtherLayoutsFromThisLayouter = false;

                const UnsignedInt dataCapacity = layouterInstance.capacity();
                const Containers::StridedArrayView1D<const UnsignedShort> dataGenerations = layouterInstance.generations();
                UnsignedInt namedLayouterDataCount = 0;
                for(UnsignedInt dataId = 0; dataId != dataCapacity; ++dataId) {
                    const LayouterDataHandle data = layouterDataHandle(dataId, dataGenerations[dataId]);
                    if(layouterInstance.isHandleValid(data) && layouterInstance.node(data) == handle) {
                        if(layouter.print) {
                            hasNamedLayouters = true;
                            layouter.print(layouter.integration, debug, layouterInstance, layouter.name, data);
                        } else if(layouter.name) {
                            hasNamedLayouters = true;
                            ++namedLayouterDataCount;
                        } else {
                            hasOtherLayoutsFromThisLayouter = true;
                            ++otherLayoutCount;
                        }
                    }
                }

                if(namedLayouterDataCount) {
                    debug << " " << namedLayouterDataCount << "layouts from layouter" << Debug::packed << layouterHandle << Debug::color(Debug::Color::Yellow) << layouter.name << Debug::resetColor << Debug::newline;
                }

                if(hasOtherLayoutsFromThisLayouter)
                    ++otherLayouterCount;
            }

            if(otherLayoutCount)
                debug << " " << otherLayoutCount << "layouts from" << otherLayouterCount << (hasNamedLayouters ? "other layouters" : "layouters") << Debug::newline;
            else CORRADE_INTERNAL_ASSERT(otherLayouterCount == 0);
        }

        if(state.sources >= DebugLayerSource::NodeAnimations) {
            /* Four entries, each for one AnimationState */
            UnsignedInt otherAnimatorCount[4]{};
            UnsignedInt otherAnimationCount[4]{};
            bool hasNamedAnimators = false;
            for(UnsignedInt animatorId = 0; animatorId != state.animators.size(); ++animatorId) {
                /* Skip animators that are freed or that we don't know about
                   yet (if highlightNode() is called, there may be animators
                   that are yet unknown to the DebugLayer, either ones with IDs
                   outside of the state.animators bounds or ones that got
                   removed and the slot reused for others), and animators that
                   have no instance */
                const Implementation::DebugLayerAnimator& animator = state.animators[animatorId];
                if(!ui.isHandleValid(animator.handle) ||
                   !ui.hasAnimatorInstance(animator.handle))
                    continue;

                /* Query only animators that support node attachment */
                /** @todo support also animations attached to data, do that in
                    the loop above somehow? basically for each layer that has
                    some data in given node it should go through animators
                    attached to that layer and check which animations are
                    attached to those */
                const AbstractAnimator& animatorInstance = ui.animator(animator.handle);
                if(!(animatorInstance.features() >= AnimatorFeature::NodeAttachment))
                    continue;

                const UnsignedInt dataCapacity = animatorInstance.capacity();
                const Containers::StridedArrayView1D<const UnsignedShort> dataGenerations = animatorInstance.generations();
                /* Four entries, each for one AnimationState */
                bool hasOtherAnimationsFromThisAnimator[4]{};
                UnsignedInt namedAnimatorDataCount[4]{};
                for(UnsignedInt dataId = 0; dataId != dataCapacity; ++dataId) {
                    const AnimatorDataHandle data = animatorDataHandle(dataId, dataGenerations[dataId]);
                    if(animatorInstance.isHandleValid(data) && animatorInstance.node(data) == handle) {
                        AnimationState state = animatorInstance.state(data);
                        CORRADE_INTERNAL_ASSERT(UnsignedInt(state) < 4);
                        if(animator.print) {
                            hasNamedAnimators = true;
                            animator.print(animator.integration, debug, animatorInstance, animator.name, data);
                        } else if(animator.name) {
                            hasNamedAnimators = true;
                            ++namedAnimatorDataCount[UnsignedInt(state)];
                        } else {
                            hasOtherAnimationsFromThisAnimator[UnsignedInt(state)] = true;
                            ++otherAnimationCount[UnsignedInt(state)];
                        }
                    }
                }

                for(UnsignedInt i = 0; i != Containers::arraySize(namedAnimatorDataCount); ++i) {
                    if(namedAnimatorDataCount[i]) {
                        debug << " " << namedAnimatorDataCount[i] << Debug::color(Debug::Color::Cyan) << Debug::packed << AnimationState(i) << Debug::resetColor << "animations from animator" << Debug::packed << animator.handle << Debug::color(Debug::Color::Yellow) << animator.name << Debug::resetColor << Debug::newline;
                    }

                    if(hasOtherAnimationsFromThisAnimator[i])
                        ++otherAnimatorCount[i];
                }
            }

            for(UnsignedInt i = 0; i != Containers::arraySize(otherAnimationCount); ++i) {
                if(otherAnimationCount[i])
                    debug << " " << otherAnimationCount[i] << Debug::color(Debug::Color::Cyan) << Debug::packed << AnimationState(i) << Debug::resetColor << "animations from" << otherAnimatorCount[i] << (hasNamedAnimators ? "other animators" : "animators") << Debug::newline;
                else CORRADE_INTERNAL_ASSERT(otherAnimatorCount[i] == 0);
            }
        }
    }

    state.currentHighlightedNode = handle;

    /* At this point the debug output redirection is no longer active so we can
       pass the result to the callback without the redirection being active
       even in the callback */
    if(state.nodeHighlightCallback) {
        /* Be nice and make the output null-terminated and without the trailing
           newline. Ideally we wouldn't print the newline at all but it's hard
           to achieve, especially with externally supplied DebugIntegration
           instances.

           The assert is worded with assumption that there's a missing newline
           in third-party integrations, if it fires for Ui itself, tell me I'm
           stupid. */
        CORRADE_ASSERT(out.hasSuffix("\n"),
            "Ui::DebugLayer: expected DebugIntegration::print() to end with a newline but got" << out.suffix(out.findLast('\n').end()).trimmedPrefix(), {});
        out.back() = '\0';
        state.nodeHighlightCallback({out.exceptSuffix("\0"_s), Containers::StringViewFlag::NullTerminated});
    }

    return true;
}

LayerFeatures DebugLayer::doFeatures() const {
    /* The events are used only if NodeHighlight is enabled, but while that can
       be toggled at runtime, the value returned from features() shouldn't
       change so they're reported always */
    return LayerFeature::Event;
}

LayerStates DebugLayer::doState() const {
    const State& state = *_state;
    return state.sources & (DebugLayerSource::Nodes|
                            DebugLayerSource::Layers|
                            DebugLayerSource::Layouters|
                            DebugLayerSource::Animators) ?
        LayerState::NeedsCommonDataUpdate : LayerStates{};
}

void DebugLayer::doClean(const Containers::BitArrayView dataIdsToRemove) {
    /* Clear the currently highlighted node if there is one and it's among the
       data IDs to remove */
    State& state = *_state;
    if(state.currentHighlightedNode != NodeHandle::Null) {
        /* At this point the `highlightData` is valid if and only if it's not
           present in `dataIdsToRemove`. The doClean() gets called only once
           AbstractLayer already removes the marked data, thus for all set bits
           the data are already invalid. On the other hand, in doPreUpdate()
           that's called right after we update the `highlightData` to account
           for node removal / creation, so it should never contain handles that
           became invalid earlier than in the immediately preceding clean
           call. */
        const LayerDataHandle data = state.nodes[nodeHandleId(state.currentHighlightedNode)].highlightData;
        if(data != LayerDataHandle::Null) {
            if(dataIdsToRemove[layerDataHandleId(data)]) {
                state.currentHighlightedNode = NodeHandle::Null;
            }
            CORRADE_INTERNAL_ASSERT(isHandleValid(data) == !dataIdsToRemove[layerDataHandleId(data)]);
        }
    }

    /* Not going over `nodes` and updating now-potentially-invalid
       `highlightData` handles, as doPreUpdate() has a similar loop anyway,
       where it also does the opposite, i.e. adding new handles for new nodes,
       so it'd be a wasteful work to do here as well */
}

void DebugLayer::doPreUpdate(LayerStates) {
    CORRADE_ASSERT(hasUi(),
        "Ui::DebugLayer::preUpdate(): layer not part of a user interface", );

    const AbstractUserInterface& ui = this->ui();
    State& state = *_state;

    if(state.sources >= DebugLayerSource::Nodes) {
        const UnsignedInt nodeCapacity = ui.nodeCapacity();
        const Containers::StridedArrayView1D<const UnsignedShort> nodeGenerations = ui.nodeGenerations();
        if(state.nodes.size() < nodeCapacity)
            arrayResize(state.nodes, ValueInit, nodeCapacity);

        for(std::size_t i = 0; i != state.nodes.size(); ++i) {
            const NodeHandle handle = nodeHandle(i, nodeGenerations[i]);

            /* If the node we remembered is different from the current one,
               reset its properties. If the current one is valid, remember its
               handle. */
            Implementation::DebugLayerNode& node = state.nodes[i];
            if(node.handle != handle) {
                /* Reset only if we actually remembered something before */
                if(node.handle != NodeHandle::Null)
                    node = {};
                if(ui.isHandleValid(handle))
                    node.handle = handle;
            }

            /* If node highlight is enabled, create new data if the node
               doesn't have it yet. Data for removed nodes are pruned
               automatically in clean() and we're replacing the whole
               DebugLayerNode contents above so the highlightData handle is
               always either valid or null. */
            if(state.flags >= DebugLayerFlag::NodeHighlight && node.handle != NodeHandle::Null && node.highlightData == LayerDataHandle::Null) {
                CORRADE_INTERNAL_DEBUG_ASSERT(ui.isHandleValid(handle));
                node.handle = handle;
                node.highlightData = dataHandleData(create(handle));
            } else CORRADE_INTERNAL_DEBUG_ASSERT(node.highlightData == LayerDataHandle::Null || isHandleValid(node.highlightData));
        }
    }

    if(state.sources >= DebugLayerSource::Layers) {
        const UnsignedInt layerCapacity = ui.layerCapacity();
        const Containers::StridedArrayView1D<const UnsignedByte> layerGenerations = ui.layerGenerations();
        if(state.layers.size() < layerCapacity)
            arrayResize(state.layers, ValueInit, layerCapacity);

        for(std::size_t i = 0; i != state.layers.size(); ++i) {
            const LayerHandle handle = layerHandle(i, layerGenerations[i]);

            /* If the layer we remembered is different from the current one,
               reset its properties. If the current one is valid, remember its
               handle. */
            Implementation::DebugLayerLayer& layer = state.layers[i];
            if(layer.handle != handle) {
                if(layer.handle != LayerHandle::Null)
                    layer = Implementation::DebugLayerLayer{};
                if(ui.isHandleValid(handle))
                    layer.handle = handle;
            }
        }
    }

    if(state.sources >= DebugLayerSource::Layouters) {
        const UnsignedInt layouterCapacity = ui.layouterCapacity();
        const Containers::StridedArrayView1D<const UnsignedByte> layouterGenerations = ui.layouterGenerations();
        if(state.layouters.size() < layouterCapacity)
            arrayResize(state.layouters, ValueInit, layouterCapacity);

        for(std::size_t i = 0; i != state.layouters.size(); ++i) {
            const LayouterHandle handle = layouterHandle(i, layouterGenerations[i]);

            /* If the layouter we remembered is different from the current one,
               reset its properties. If the current one is valid, remember its
               handle. */
            Implementation::DebugLayerLayouter& layouter = state.layouters[i];
            if(layouter.handle != handle) {
                if(layouter.handle != LayouterHandle::Null)
                    layouter = Implementation::DebugLayerLayouter{};
                if(ui.isHandleValid(handle))
                    layouter.handle = handle;
            }
        }
    }

    if(state.sources >= DebugLayerSource::Animators) {
        const UnsignedInt animatorCapacity = ui.animatorCapacity();
        const Containers::StridedArrayView1D<const UnsignedByte> animatorGenerations = ui.animatorGenerations();
        if(state.animators.size() < animatorCapacity)
            arrayResize(state.animators, ValueInit, animatorCapacity);

        for(std::size_t i = 0; i != state.animators.size(); ++i) {
            const AnimatorHandle handle = animatorHandle(i, animatorGenerations[i]);

            /* If the animator we remembered is different from the current one,
               reset its properties. If the current one is valid, remember its
               handle. */
            Implementation::DebugLayerAnimator& animator = state.animators[i];
            if(animator.handle != handle) {
                if(animator.handle != AnimatorHandle::Null)
                    animator = Implementation::DebugLayerAnimator{};
                if(ui.isHandleValid(handle))
                    animator.handle = handle;
            }
        }
    }
}

void DebugLayer::doUpdate(const LayerStates states, const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, const Containers::StridedArrayView1D<const Float>&, Containers::BitArrayView, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&) {
    /* NeedsCommonDataUpdate is handled in doPreUpdate() above */

    State& state = *_state;

    /* If we're meant to draw... */
    if(doFeatures() >= LayerFeature::Draw) {
        /* If there's no current highlighted node, there's nothing to draw */
        if(state.currentHighlightedNode == NodeHandle::Null) {
            state.highlightedNodeDrawOffset = ~std::size_t{};

        /* Otherwise, if anything that affects the current highlighted node
           needs to be updated, find the highlighted node among the data IDs
           (if there at all) and remember its index to know when to draw */
        } else if(states >= LayerState::NeedsDataUpdate ||
                  states >= LayerState::NeedsNodeOffsetSizeUpdate ||
                  states >= LayerState::NeedsNodeOrderUpdate)
        {
            const UnsignedInt highlightedDataId = layerDataHandleId(state.nodes[nodeHandleId(state.currentHighlightedNode)].highlightData);
            state.highlightedNodeDrawOffset = ~std::size_t{};
            for(std::size_t i = 0; i != dataIds.size(); ++i) if(dataIds[i] == highlightedDataId) {
                state.highlightedNodeDrawOffset = i;
                break;
            }
        }
    }

    /* If there's any highlighted node and anything changed for it, regenerate
       vertex data. In particular, if only NeedsNodeOrderUpdate is set, only
       the highlightedNodeDrawOffset changes, vertex data don't need any
       update. */
    if(state.highlightedNodeDrawOffset != ~std::size_t{} &&
       (states >= LayerState::NeedsDataUpdate ||
        states >= LayerState::NeedsNodeOffsetSizeUpdate))
    {
        /* We'll be drawing a triangle strip, which is ordered 012 123 and thus
           the usual lerp()'d winding would be clockwise. Flip the Y coordinate
           to make it CCW. */
        const UnsignedInt highlightedNodeId = nodeHandleId(state.currentHighlightedNode);
        Vector2 min = nodeOffsets[highlightedNodeId];
        Vector2 max = min + nodeSizes[highlightedNodeId];
        Utility::swap(min.y(), max.y());
        for(UnsignedByte i = 0; i != 4; ++i) {
            /* ✨ */
            state.highlightedNodeVertices[i].position = Math::lerp(min, max, BitVector2{i});
            state.highlightedNodeVertices[i].color = state.nodeHighlightColor;
        }
    }
}

void DebugLayer::doPointerPressEvent(const UnsignedInt dataId, PointerEvent& event) {
    /* Accept presses only if node highlight is enabled, the pointer is among
       one of the expected, is primary and the modifiers match exactly */
    State& state = *_state;
    if(!(state.flags >= DebugLayerFlag::NodeHighlight) ||
       !event.isPrimary() ||
       !(event.pointer() <= state.nodeHighlightPointers) ||
       event.modifiers() != state.nodeHighlightModifiers)
        return;

    /* If the node that's clicked on is currently highlighted, remove the
       highlight and exit. In case a callback is set, call it with an empty
       string to notify it that it's no longer desirable to show the
       details. */
    const NodeHandle nodeHandle = nodes()[dataId];
    if(state.currentHighlightedNode == nodeHandle)
        highlightNode(NodeHandle::Null);
    else
        highlightNode(nodeHandle);

    /* Accept the event to prevent it from propagating to other nodes, even in
       case we're clicking second time to remove the highlight */
    event.setAccepted();
}

}}
