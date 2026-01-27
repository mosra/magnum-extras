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

#include "GenericLayouter.h"

#include <Corrade/Containers/BitArrayView.h>
#include <Corrade/Containers/Function.h>
#include <Corrade/Containers/GrowableArray.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Magnum/Math/Vector4.h>

#include "Magnum/Ui/AbstractUserInterface.h"
#include "Magnum/Ui/Handle.h"

namespace Magnum { namespace Ui {

namespace {

/* Done this way to prepare for each layout function potentially taking
   different arguments */
struct Layout {
    Containers::FunctionData layout;
    void(*call)(Layout&, const GenericLayouter&, Vector2&, Vector2&);
};

}

struct GenericLayouter::State {
    Containers::Array<Layout> layouts;

    Containers::StridedArrayView1D<Vector2> nodeOffsets;
    Containers::StridedArrayView1D<Vector2> nodeSizes;
    Containers::StridedArrayView1D<Vector2> nodeMinSizes;
    Containers::StridedArrayView1D<Vector2> nodeMaxSizes;
    Containers::StridedArrayView1D<Float> nodeAspectRatios;
    Containers::StridedArrayView1D<Vector4> nodePaddings;
    Containers::StridedArrayView1D<Vector4> nodeMargins;
};

GenericLayouter::GenericLayouter(const LayouterHandle handle): AbstractLayouter{handle}, _state{InPlaceInit} {}

GenericLayouter::GenericLayouter(GenericLayouter&&) noexcept = default;

GenericLayouter::~GenericLayouter() = default;

GenericLayouter& GenericLayouter::operator=(GenericLayouter&&) noexcept = default;

std::size_t GenericLayouter::usedAllocatedCount() const {
    std::size_t count = 0;
    for(const Layout& layout: _state->layouts)
        if(layout.layout.isAllocated())
            ++count;

    return count;
}

LayoutHandle GenericLayouter::add(const NodeHandle node, Containers::Function<void(const GenericLayouter&, Vector2&, Vector2&)>&& layout) {
    State& state = *_state;
    CORRADE_ASSERT(layout,
        "Ui::GenericLayouter::add(): layout is null", {});

    const LayoutHandle handle = AbstractLayouter::add(node);
    const UnsignedInt id = layoutHandleId(handle);
    if(id >= state.layouts.size())
        /* Can't use NoInit because of non-trivial types */
        arrayResize(state.layouts, ValueInit, id + 1);

    Layout& layoutData = state.layouts[id];
    layoutData.layout = Utility::move(layout);
    layoutData.call = [](Layout& layout, const GenericLayouter& layouter, Vector2& nodeOffset, Vector2& nodeSize) {
        static_cast<Containers::Function<void(const GenericLayouter&, Vector2&, Vector2&)>&>(layout.layout)(layouter, nodeOffset, nodeSize);
    };

    return handle;
}

void GenericLayouter::removeInternal(const UnsignedInt id) {
    /* Set the layout function to an empty instance to call any captured state
       destructors */
    _state->layouts[id].layout = {};
}

void GenericLayouter::remove(const LayoutHandle handle) {
    AbstractLayouter::remove(handle);
    removeInternal(layoutHandleId(handle));
}

void GenericLayouter::remove(const LayouterDataHandle handle) {
    AbstractLayouter::remove(handle);
    removeInternal(layouterDataHandleId(handle));
}

bool GenericLayouter::isAllocated(const LayoutHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::GenericLayouter::isAllocated(): invalid handle" << handle, {});
    return _state->layouts[layoutHandleId(handle)].layout.isAllocated();
}

bool GenericLayouter::isAllocated(const LayouterDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::GenericLayouter::isAllocated(): invalid handle" << handle, {});
    return _state->layouts[layouterDataHandleId(handle)].layout.isAllocated();
}

Vector2 GenericLayouter::nodeOffset(const NodeHandle node) const {
    const State& state = *_state;
    CORRADE_ASSERT(state.nodeOffsets,
        "Ui::GenericLayouter::nodeOffset(): can only be called from within layout functions", {});
    /* If the above assert doesn't fire, assume the layouter is part of the UI
       already without testing for hasUi() */
    CORRADE_ASSERT(ui().isHandleValid(node),
        "Ui::GenericLayouter::nodeOffset(): invalid handle" << node, {});
    return state.nodeOffsets[nodeHandleId(node)];
}

Vector2 GenericLayouter::nodeSize(const NodeHandle node) const {
    const State& state = *_state;
    CORRADE_ASSERT(state.nodeSizes,
        "Ui::GenericLayouter::nodeSize(): can only be called from within layout functions", {});
    /* If the above assert doesn't fire, assume the layouter is part of the UI
       already without testing for hasUi() */
    CORRADE_ASSERT(ui().isHandleValid(node),
        "Ui::GenericLayouter::nodeSize(): invalid handle" << node, {});
    return state.nodeSizes[nodeHandleId(node)];
}

Vector2 GenericLayouter::nodeMinSize(const NodeHandle node) const {
    const State& state = *_state;
    CORRADE_ASSERT(state.nodeMinSizes,
        "Ui::GenericLayouter::nodeMinSize(): can only be called from within layout functions", {});
    /* If the above assert doesn't fire, assume the layouter is part of the UI
       already without testing for hasUi() */
    CORRADE_ASSERT(ui().isHandleValid(node),
        "Ui::GenericLayouter::nodeMinSize(): invalid handle" << node, {});
    return state.nodeMinSizes[nodeHandleId(node)];
}

Vector2 GenericLayouter::nodeMaxSize(const NodeHandle node) const {
    const State& state = *_state;
    CORRADE_ASSERT(state.nodeMaxSizes,
        "Ui::GenericLayouter::nodeMaxSize(): can only be called from within layout functions", {});
    /* If the above assert doesn't fire, assume the layouter is part of the UI
       already without testing for hasUi() */
    CORRADE_ASSERT(ui().isHandleValid(node),
        "Ui::GenericLayouter::nodeMaxSize(): invalid handle" << node, {});
    return state.nodeMaxSizes[nodeHandleId(node)];
}

Float GenericLayouter::nodeAspectRatio(const NodeHandle node) const {
    const State& state = *_state;
    CORRADE_ASSERT(state.nodeAspectRatios,
        "Ui::GenericLayouter::nodeAspectRatio(): can only be called from within layout functions", {});
    /* If the above assert doesn't fire, assume the layouter is part of the UI
       already without testing for hasUi() */
    CORRADE_ASSERT(ui().isHandleValid(node),
        "Ui::GenericLayouter::nodeAspectRatio(): invalid handle" << node, {});
    return state.nodeAspectRatios[nodeHandleId(node)];
}

Vector4 GenericLayouter::nodePadding(const NodeHandle node) const {
    const State& state = *_state;
    CORRADE_ASSERT(state.nodePaddings,
        "Ui::GenericLayouter::nodePadding(): can only be called from within layout functions", {});
    /* If the above assert doesn't fire, assume the layouter is part of the UI
       already without testing for hasUi() */
    CORRADE_ASSERT(ui().isHandleValid(node),
        "Ui::GenericLayouter::nodePadding(): invalid handle" << node, {});
    return state.nodePaddings[nodeHandleId(node)];
}

Vector4 GenericLayouter::nodeMargin(const NodeHandle node) const {
    const State& state = *_state;
    CORRADE_ASSERT(state.nodeMargins,
        "Ui::GenericLayouter::nodeMargin(): can only be called from within layout functions", {});
    /* If the above assert doesn't fire, assume the layouter is part of the UI
       already without testing for hasUi() */
    CORRADE_ASSERT(ui().isHandleValid(node),
        "Ui::GenericLayouter::nodeMargin(): invalid handle" << node, {});
    return state.nodeMargins[nodeHandleId(node)];
}

LayouterFeatures GenericLayouter::doFeatures() const {
    /* This is deliberately *not* LayouterFeature::UniqueLayouts in order to
       allow multiple functions to be assigned for a single node, for example
       one affecting the node offset and size and another affecting padding of
       particular data */
    return {};
}

void GenericLayouter::doClean(const Containers::BitArrayView layoutIdsToRemove) {
    /** @todo some way to iterate bits */
    for(std::size_t i = 0; i != layoutIdsToRemove.size(); ++i) {
        if(!layoutIdsToRemove[i])
            continue;
        removeInternal(i);
    }
}

void GenericLayouter::doLayout(const Containers::BitArrayView layoutIdsToUpdate, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<Vector2>& nodeMinSizes, const Containers::StridedArrayView1D<Vector2>& nodeMaxSizes, const Containers::StridedArrayView1D<Float>& nodeAspectRatios, const Containers::StridedArrayView1D<Vector4>& nodePaddings, const Containers::StridedArrayView1D<Vector4>& nodeMargins, const Containers::StridedArrayView1D<Vector2>& nodeOffsets, const Containers::StridedArrayView1D<Vector2>& nodeSizes) {
    State& state = *_state;

    /* Save all views for use by the nodeMinSize(), nodeMaxSize(), ... getters
       called from within the layout functions */
    state.nodeOffsets = nodeOffsets;
    state.nodeSizes = nodeSizes;
    state.nodeMinSizes = nodeMinSizes;
    state.nodeMaxSizes = nodeMaxSizes;
    state.nodeAspectRatios = nodeAspectRatios;
    state.nodePaddings = nodePaddings;
    state.nodeMargins = nodeMargins;

    const Containers::StridedArrayView1D<const NodeHandle> nodes = this->nodes();

    /* The actual operation is then *really* simple */
    for(std::size_t i = 0; i != layoutIdsToUpdate.size(); ++i) {
        /** @todo some way to iterate set bits */
        if(!layoutIdsToUpdate[i])
            continue;

        Layout& layout = state.layouts[i];
        layout.call(layout, *this, nodeOffsets[nodeHandleId(nodes[i])], nodeSizes[nodeHandleId(nodes[i])]);
    }

    /* Reset the views back so the nodeMinSize(), ... getters cannot be called
       anymore */
    state.nodeOffsets = {};
    state.nodeSizes = {};
    state.nodeMinSizes = {};
    state.nodeMaxSizes = {};
    state.nodeAspectRatios = {};
    state.nodePaddings = {};
    state.nodeMargins = {};
}

}}
