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

#include "LayoutLayer.h"

#include <Corrade/Containers/BitArrayView.h>
#include <Corrade/Containers/GrowableArray.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Containers/StringView.h>
#include <Corrade/Utility/Algorithms.h>
#include <Magnum/Math/Functions.h>
#include <Magnum/Math/Vector4.h>
#include <Magnum/Ui/Handle.h>

namespace Magnum { namespace Ui {

namespace {

struct Style {
    Vector2 minSize, maxSize;
    Float aspectRatio;
    Vector4 margin, padding;
};

}

struct LayoutLayer::State {
    explicit State(const UnsignedInt styleCount): styles{nullptr, styleCount} {
        CORRADE_ASSERT(styleCount,
            "Ui::LayoutLayer: expected non-zero style count", );
    }

    /* If styles.data() is null, setStyle() wasn't called yet */
    Containers::Array<Style> styles;
    Containers::Array<UnsignedInt> data;
};

LayoutLayer::LayoutLayer(const LayerHandle handle, const UnsignedInt styleCount): AbstractLayer{handle}, _state{InPlaceInit, styleCount} {}

LayoutLayer::LayoutLayer(LayoutLayer&&) noexcept = default;

LayoutLayer& LayoutLayer::operator=(LayoutLayer&&) noexcept = default;

LayoutLayer::~LayoutLayer() = default;

UnsignedInt LayoutLayer::styleCount() const { return _state->styles.size(); }

Containers::StridedArrayView1D<const Vector2> LayoutLayer::styleMinSizes() const {
    const State& state = *_state;
    CORRADE_ASSERT(state.styles.data(),
        "Ui::LayoutLayer::styleMinSizes(): no style data was set", {});
    return stridedArrayView(state.styles).slice(&Style::minSize);
}

Containers::StridedArrayView1D<const Vector2> LayoutLayer::styleMaxSizes() const {
    const State& state = *_state;
    CORRADE_ASSERT(state.styles.data(),
        "Ui::LayoutLayer::styleMaxSizes(): no style data was set", {});
    return stridedArrayView(state.styles).slice(&Style::maxSize);
}

Containers::StridedArrayView1D<const Float> LayoutLayer::styleAspectRatios() const {
    const State& state = *_state;
    CORRADE_ASSERT(state.styles.data(),
        "Ui::LayoutLayer::styleAspectRatios(): no style data was set", {});
    return stridedArrayView(state.styles).slice(&Style::aspectRatio);
}

Containers::StridedArrayView1D<const Vector4> LayoutLayer::stylePaddings() const {
    const State& state = *_state;
    CORRADE_ASSERT(state.styles.data(),
        "Ui::LayoutLayer::stylePaddings(): no style data was set", {});
    return stridedArrayView(state.styles).slice(&Style::padding);
}

Containers::StridedArrayView1D<const Vector4> LayoutLayer::styleMargins() const {
    const State& state = *_state;
    CORRADE_ASSERT(state.styles.data(),
        "Ui::LayoutLayer::styleMargins(): no style data was set", {});
    return stridedArrayView(state.styles).slice(&Style::margin);
}

void LayoutLayer::setStyle(const Containers::StridedArrayView1D<const Vector2>& minSizes, const Containers::StridedArrayView1D<const Vector2>& maxSizes, const Containers::StridedArrayView1D<const Float>& aspectRatios, const Containers::StridedArrayView1D<const Vector4>& paddings, const Containers::StridedArrayView1D<const Vector4>& margins) {
    State& state = *_state;
    CORRADE_ASSERT(
        (minSizes.isEmpty() || minSizes.size() == state.styles.size()) &&
        (maxSizes.isEmpty() || maxSizes.size() == state.styles.size()) &&
        (aspectRatios.isEmpty() || aspectRatios.size() == state.styles.size()) &&
        (paddings.isEmpty() || paddings.size() == state.styles.size()) &&
        (margins.isEmpty() || margins.size() == state.styles.size()),
        "Ui::LayoutLayer::setStyle(): expected min size, max size, aspect ratio, padding and margin views to be either empty or have a size of" << state.styles.size() << "but got" << minSizes.size() << Debug::nospace << "," << maxSizes.size() << Debug::nospace << "," << aspectRatios.size() << Debug::nospace << "," << paddings.size() << "and" << margins.size(), );

    /* Style wasn't set yet, allocate the storage */
    if(!state.styles.data())
        state.styles = Containers::Array<Style>{NoInit, state.styles.size()};

    if(!minSizes.isEmpty())
        Utility::copy(minSizes, stridedArrayView(state.styles).slice(&Style::minSize));
    else for(Style& i: state.styles)
        i.minSize = {}; /** @todo Utility::fill() */

    if(!maxSizes.isEmpty())
        Utility::copy(maxSizes, stridedArrayView(state.styles).slice(&Style::maxSize));
    else for(Style& i: state.styles)
        i.maxSize = Vector2{Constants::inf()}; /** @todo Utility::fill() */

    if(!aspectRatios.isEmpty())
        Utility::copy(aspectRatios, stridedArrayView(state.styles).slice(&Style::aspectRatio));
    else for(Style& i: state.styles)
        i.aspectRatio = 0.0f; /** @todo Utility::fill() */

    if(!paddings.isEmpty())
        Utility::copy(paddings, stridedArrayView(state.styles).slice(&Style::padding));
    else for(Style& i: state.styles)
        i.padding = {}; /** @todo Utility::fill() */

    if(!margins.isEmpty())
        Utility::copy(margins, stridedArrayView(state.styles).slice(&Style::margin));
    else for(Style& i: state.styles)
        i.margin = {}; /** @todo Utility::fill() */

    setNeedsUpdate(LayerState::NeedsLayoutUpdate);
}

void LayoutLayer::setStyle(const std::initializer_list<Vector2> minSizes, const std::initializer_list<Vector2> maxSizes, const std::initializer_list<Float> aspectRatios, const std::initializer_list<Vector4> paddings, const std::initializer_list<Vector4> margins) {
    return setStyle(Containers::stridedArrayView(minSizes), Containers::stridedArrayView(maxSizes), Containers::stridedArrayView(aspectRatios), Containers::stridedArrayView(paddings), Containers::stridedArrayView(margins));
}

DataHandle LayoutLayer::create(const UnsignedInt style, const NodeHandle node) {
    State& state = *_state;
    CORRADE_ASSERT(style < state.styles.size(),
        "Ui::LayoutLayer::create(): style" << style << "out of range for" << state.styles.size() << "styles", {});

    const DataHandle handle = AbstractLayer::create(node);
    const UnsignedInt id = dataHandleId(handle);
    if(id >= state.data.size())
        arrayResize(state.data, NoInit, id + 1);

    /* The data is just the style index alone. Perfection. */
    state.data[id] = style;

    return handle;
}

UnsignedInt LayoutLayer::style(const DataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::LayoutLayer::style(): invalid handle" << handle, {});
    return _state->data[dataHandleId(handle)];
}

UnsignedInt LayoutLayer::style(const LayerDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::LayoutLayer::style(): invalid handle" << handle, {});
    return _state->data[layerDataHandleId(handle)];
}

void LayoutLayer::setStyle(const DataHandle handle, const UnsignedInt style) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::LayoutLayer::setStyle(): invalid handle" << handle, );
    setStyleInternal(dataHandleId(handle), style);
}

void LayoutLayer::setStyle(const LayerDataHandle handle, const UnsignedInt style) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::LayoutLayer::setStyle(): invalid handle" << handle, );
    setStyleInternal(layerDataHandleId(handle), style);
}

void LayoutLayer::setStyleInternal(const UnsignedInt id, const UnsignedInt style) {
    State& state = *_state;
    CORRADE_ASSERT(style < state.styles.size(),
        "Ui::LayoutLayer::setStyle(): style" << style << "out of range for" << state.styles.size() << "styles", );

    state.data[id] = style;

    /* If the data is attached, trigger a layout update. Otherwise the style
       change affects nothing. */
    if(nodes()[id] != NodeHandle::Null)
        setNeedsUpdate(LayerState::NeedsLayoutUpdate);
}

LayerFeatures LayoutLayer::doFeatures() const {
    return LayerFeature::Layout;
}

void LayoutLayer::doLayout(const Containers::BitArrayView dataIdsToLayout, const Containers::StridedArrayView1D<Vector2>& nodeMinSizes, const Containers::StridedArrayView1D<Vector2>& nodeMaxSizes, const Containers::StridedArrayView1D<Float>& nodeAspectRatios, const Containers::StridedArrayView1D<Vector4>& nodePaddings, const Containers::StridedArrayView1D<Vector4>& nodeMargins) {
    const State& state = *_state;
    CORRADE_ASSERT(state.styles.data(),
        "Ui::LayoutLayer::layout(): no style data was set", );

    const Containers::StridedArrayView1D<const NodeHandle> nodes = this->nodes();
    for(std::size_t i = 0; i != dataIdsToLayout.size(); ++i) {
        /** @todo some way to iterate set bits */
        if(!dataIdsToLayout[i])
            continue;

        const Style& style = state.styles[state.data[i]];
        const UnsignedInt nodeId = nodeHandleId(nodes[i]);
        nodeMinSizes[nodeId] = Math::max(nodeMinSizes[nodeId], style.minSize);
        nodeMaxSizes[nodeId] = Math::min(nodeMaxSizes[nodeId], style.maxSize);
        if(style.aspectRatio && !nodeAspectRatios[nodeId])
            nodeAspectRatios[nodeId] = style.aspectRatio;
        nodePaddings[nodeId] = Math::max(nodePaddings[nodeId], style.padding);
        nodeMargins[nodeId] = Math::max(nodeMargins[nodeId], style.margin);
    }
}

void LayoutLayer::DebugIntegration::print(Debug& debug, const LayoutLayer& layer, const Containers::StringView& layerName, LayerDataHandle data) {
    debug << "  Data" << Debug::packed << data << "from layer" << Debug::packed << layer.handle();
    if(layerName)
        debug << Debug::color(Debug::Color::Yellow) << layerName << Debug::resetColor;
    debug << "with style";

    const UnsignedInt style = layer.style(data);
    Containers::StringView name;
    if(_styleName)
        name = _styleName(style);
    if(name)
        debug << Debug::color(Debug::Color::Yellow) << name << Debug::resetColor << "(" << Debug::nospace << style << Debug::nospace << ")";
    else
        debug << style;
    debug << Debug::newline;
}

}}
