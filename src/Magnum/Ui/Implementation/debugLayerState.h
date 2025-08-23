#ifndef Magnum_Ui_Implementation_debugLayerState_h
#define Magnum_Ui_Implementation_debugLayerState_h
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

/* Definition of the DebugLayer::State struct to be used by both DebugLayer and
   DebugLayerGL as well as DebugLayer tests, and (if this header gets
   published) eventually possibly also 3rd party renderer implementations */

#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/Function.h>
#include <Corrade/Containers/String.h>
#include <Magnum/Math/Color.h>

#include "Magnum/Ui/DebugLayer.h"
#include "Magnum/Ui/Event.h"
#include "Magnum/Ui/Handle.h"

namespace Magnum { namespace Ui {

namespace Implementation {

struct DebugLayerNode {
    NodeHandle handle = NodeHandle::Null;

    /* Name that's associated with `handle`. If the handle changes, the whole
       instance is expected to be replaced. */
    Containers::String name;

    /* If null, the node doesn't have any data associated. If the node gets
       removed, this briefly becomes invalid after clean(), after that the
       whole instance gets replaced in doPreUpdate(). */
    LayerDataHandle highlightData = LayerDataHandle::Null;
};

struct DebugLayerLayer {
    explicit DebugLayerLayer() noexcept = default;

    /** @todo clean this up once Pointer with a custom deleter is a thing */
    DebugLayerLayer(DebugLayerLayer&& other) noexcept: handle{other.handle}, name{Utility::move(other.name)}, integration{other.integration}, deleter{other.deleter}, print{other.print} {
        other.integration = nullptr;
        other.deleter = nullptr;
        other.print = nullptr;
    }

    ~DebugLayerLayer() {
        if(deleter)
            deleter(integration);
    }

    DebugLayerLayer& operator=(DebugLayerLayer&& other) {
        Utility::swap(other.handle, handle);
        Utility::swap(other.name, name);
        Utility::swap(other.integration, integration);
        Utility::swap(other.deleter, deleter);
        Utility::swap(other.print, print);
        return *this;
    }

    LayerHandle handle = LayerHandle::Null;

    /* Name & debug layer integration that's associated with `handle`. If the
       handle changes, the whole instance is expected to be replaced in
       doPreUpdate(). */
    Containers::String name;
    void* integration = nullptr;
    void(*deleter)(void*) = nullptr;
    void(*print)(void*, Debug&, const AbstractLayer&, const Containers::StringView&, LayerDataHandle) = nullptr;
};

}

struct DebugLayer::State {
    explicit State(DebugLayerSources sources, DebugLayerFlags flags);
    /* Assumes that the derived state structs will have
       non-trivially-destructible members. Without a virtual destructor those
       wouldn't be destructed properly when deleting from the base pointer.
       This is also checked with a static_assert() in the Pointer class itself.

       Another possibility would be to let the derived classes have their own
       state allocation, but I feel that having a base with a virtual
       destructor is less nasty and nicer to caches than several loose
       allocations of non-virtual types. */
    /** @todo switch to a custom (casting) deleter instead once Pointer has
        that */
    virtual ~State() = default;

    DebugLayerSources sources;
    DebugLayerFlags flags;
    /* 1 byte free */

    NodeHandle currentHighlightedNode = NodeHandle::Null;
    Color4 nodeHighlightColor{0.5f, 0.0f, 0.5f, 0.5f};
    Pointers nodeHighlightPointers = Pointer::MouseRight|Pointer::Eraser;
    Modifiers nodeHighlightModifiers = Modifier::Ctrl;
    /* 6 bytes free */
    Containers::Function<void(Containers::StringView)> nodeHighlightCallback;

    /* Used only if the layer advertises LayerFeature::Draw (i.e.,
       DebugLayerGL). The offset is an index into the dataIds array passed to
       doUpdate() and doDraw(). */
    std::size_t highlightedNodeDrawOffset = ~std::size_t{};
    struct {
        Vector2 position;
        Color4 color;
    } highlightedNodeVertices[4];

    Containers::Array<Implementation::DebugLayerNode> nodes;
    Containers::Array<Implementation::DebugLayerLayer> layers;
};

}}

#endif
