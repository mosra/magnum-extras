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

#include <Corrade/Containers/Function.h>
#include <Corrade/Containers/GrowableArray.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Containers/String.h>
#include <Magnum/Animation/Easing.h>

#include "Magnum/Ui/AbstractUserInterface.h"
#include "Magnum/Ui/AbstractVisualLayer.h"
#include "Magnum/Ui/BaseLayer.h"
#include "Magnum/Ui/BaseLayerAnimator.h"
#include "Magnum/Ui/LineLayer.h"
#include "Magnum/Ui/Event.h"
#include "Magnum/Ui/EventLayer.h"
#include "Magnum/Ui/GenericAnimator.h"
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/NodeFlags.h"
#include "Magnum/Ui/TextLayerAnimator.h"

#define DOXYGEN_ELLIPSIS(...) __VA_ARGS__
#define DOXYGEN_IGNORE(...) __VA_ARGS__

using namespace Magnum;
using namespace Magnum::Math::Literals;

Ui::AbstractVisualLayer::Shared& abstractVisualLayerShared();
namespace {
/* [AbstractVisualLayer-Shared-setStyleTransition] */
enum StyleIndex {
    DOXYGEN_ELLIPSIS()
};

StyleIndex styleIndexTransitionToInactiveBlur(StyleIndex index) {
    DOXYGEN_ELLIPSIS(return index;)
}
StyleIndex styleIndexTransitionToInactiveHover(StyleIndex index) {
    DOXYGEN_ELLIPSIS(return index;)
}
StyleIndex styleIndexTransitionToFocusedBlur(StyleIndex index) {
    DOXYGEN_ELLIPSIS(return index;)
}
StyleIndex styleIndexTransitionToFocusedHover(StyleIndex index) {
    DOXYGEN_ELLIPSIS(return index;)
}
StyleIndex styleIndexTransitionToPressedBlur(StyleIndex index) {
    DOXYGEN_ELLIPSIS(return index;)
}
StyleIndex styleIndexTransitionToPressedHover(StyleIndex index) {
    DOXYGEN_ELLIPSIS(return index;)
}
StyleIndex styleIndexTransitionToDisabled(StyleIndex index) {
    DOXYGEN_ELLIPSIS(return index;)
}

DOXYGEN_ELLIPSIS(} int main() {)

Ui::AbstractVisualLayer::Shared& shared = DOXYGEN_ELLIPSIS(abstractVisualLayerShared());
shared.setStyleTransition<StyleIndex,
    styleIndexTransitionToInactiveBlur,
    styleIndexTransitionToInactiveHover,
    styleIndexTransitionToFocusedBlur,
    styleIndexTransitionToFocusedHover,
    styleIndexTransitionToPressedBlur,
    styleIndexTransitionToPressedHover,
    styleIndexTransitionToDisabled>();
/* [AbstractVisualLayer-Shared-setStyleTransition] */
}

/* Anonymous namespace to avoid "no previous declaration" warnings, main() and
   main2() is then outside of it to avoid "unused function" warnings */
namespace A { namespace {
/* [BaseLayer-style-transitions] */
enum BaseLayerStyle {
    Button,
    ButtonHovered,
    ButtonPressed,
    ButtonPressedHovered,
    Label
};
BaseLayerStyle toInactiveOut(BaseLayerStyle style) {
    switch(style) {
        case BaseLayerStyle::ButtonHovered:
        case BaseLayerStyle::ButtonPressed:
        case BaseLayerStyle::ButtonPressedHovered:
            return BaseLayerStyle::Button;
        default:
            return style;
    }
}
BaseLayerStyle toInactiveOver(BaseLayerStyle style) {
    switch(style) {
        case BaseLayerStyle::Button:
        case BaseLayerStyle::ButtonPressed:
        case BaseLayerStyle::ButtonPressedHovered:
            return BaseLayerStyle::ButtonHovered;
        default:
            return style;
    }
}
BaseLayerStyle toPressedOut(BaseLayerStyle style) {
    switch(style) {
        case BaseLayerStyle::Button:
        case BaseLayerStyle::ButtonHovered:
        case BaseLayerStyle::ButtonPressedHovered:
            return BaseLayerStyle::ButtonPressed;
        default:
            return style;
    }
}
BaseLayerStyle toPressedOver(BaseLayerStyle style) {
    switch(style) {
        case BaseLayerStyle::Button:
        case BaseLayerStyle::ButtonHovered:
        case BaseLayerStyle::ButtonPressed:
            return BaseLayerStyle::ButtonPressedHovered;
        default:
            return style;
    }
}

DOXYGEN_ELLIPSIS(} void main(); void main() { struct Shared: Ui::BaseLayer::Shared {
    explicit Shared(): Ui::BaseLayer::Shared::Shared{Configuration{1}} {}
    void doSetStyle(const Ui::BaseLayerCommonStyleUniform&, Containers::ArrayView<const Ui::BaseLayerStyleUniform>) override {}
} baseLayerShared;)

baseLayerShared.setStyleTransition<BaseLayerStyle,
    toInactiveOut,
    toInactiveOver,
    nullptr,
    nullptr,
    toPressedOut,
    toPressedOver,
    nullptr>();
/* [BaseLayer-style-transitions] */
}

namespace {
/* [BaseLayer-style-transitions-deduplicated] */
struct Transition {
    BaseLayerStyle inactiveOut;
    BaseLayerStyle inactiveOver;
    BaseLayerStyle pressedOut;
    BaseLayerStyle pressedOver;
};
Transition transition(BaseLayerStyle style) {
    /* In C++20 you can further simplify with `using enum BaseLayerStyle` */
    switch(style) {
        case BaseLayerStyle::Button:
        case BaseLayerStyle::ButtonHovered:
        case BaseLayerStyle::ButtonPressed:
        case BaseLayerStyle::ButtonPressedHovered:
            return {BaseLayerStyle::Button,
                    BaseLayerStyle::ButtonHovered,
                    BaseLayerStyle::ButtonPressed,
                    BaseLayerStyle::ButtonPressedHovered};
        default:
            return {style, style, style, style};
    }
}
template<BaseLayerStyle Transition::*member> BaseLayerStyle to(BaseLayerStyle style) {
    return transition(style).*member;
}

DOXYGEN_ELLIPSIS(} void main2(); void main2() { struct Shared: Ui::BaseLayer::Shared {
    explicit Shared(): Ui::BaseLayer::Shared::Shared{Configuration{1}} {}
    void doSetStyle(const Ui::BaseLayerCommonStyleUniform&, Containers::ArrayView<const Ui::BaseLayerStyleUniform>) override {}
} baseLayerShared;)

baseLayerShared.setStyleTransition<BaseLayerStyle,
    to<&Transition::inactiveOut>,
    to<&Transition::inactiveOver>,
    nullptr,
    nullptr,
    to<&Transition::pressedOut>,
    to<&Transition::pressedOver>,
    nullptr>();
/* [BaseLayer-style-transitions-deduplicated] */
}
}

namespace B {

/* Declarations to avoid -Wmisssing-prototypes */
void setNodeName(Ui::NodeHandle node, Containers::StringView name);
Containers::StringView nodeName(Ui::NodeHandle node);
/* [AbstractUserInterface-handles-extract] */
struct Name {
    Containers::String name;
    UnsignedInt generation = 0;
};
Containers::Array<Name> names;

DOXYGEN_ELLIPSIS()

void setNodeName(Ui::NodeHandle node, Containers::StringView name) {
    UnsignedInt id = Ui::nodeHandleId(node);
    if(id >= names.size())
        arrayResize(names, id + 1);

    names[id].name = name;
    names[id].generation = Ui::nodeHandleGeneration(node);
}

Containers::StringView nodeName(Ui::NodeHandle node) {
    UnsignedInt id = Ui::nodeHandleId(node);
    if(id < names.size() && names[id].generation == Ui::nodeHandleGeneration(node))
        return names[id].name;
    return {};
}
/* [AbstractUserInterface-handles-extract] */

}

/* Make sure the name doesn't conflict with any other snippets to avoid linker
   warnings, unlike with `int main()` there now has to be a declaration to
   avoid -Wmisssing-prototypes */
void mainUi();
void mainUi() {
{
Ui::AbstractUserInterface ui{{100, 100}};
/* [AbstractUserInterface-setup-events] */
Ui::PointerEvent event{{},
    Ui::PointerEventSource::Mouse,
    Ui::Pointer::MouseLeft, true, 0};
if(!ui.pointerPressEvent({123, 456}, event)) {
    // Not handled by the UI, pass further ...
}
/* [AbstractUserInterface-setup-events] */
}

{
Ui::AbstractUserInterface ui{{100, 100}};
/* [AbstractUserInterface-nodes] */
Ui::NodeHandle panel = ui.createNode({50, 50}, {200, 150});
Ui::NodeHandle title = ui.createNode(panel, {10, 10}, {180, 20});
Ui::NodeHandle content = ui.createNode(panel, {10, 40}, {180, 100});
/* [AbstractUserInterface-nodes] */
static_cast<void>(content);

/* [AbstractUserInterface-nodes-opacity] */
ui.setNodeOpacity(panel, 0.8f);
ui.setNodeOpacity(title, 0.75f);
/* [AbstractUserInterface-nodes-opacity] */

/* [AbstractUserInterface-nodes-order] */
Ui::NodeHandle anotherPanel = ui.createNode({200, 130}, {120, 80});

/* Put the new panel behind the first one, instead of being on top */
ui.setNodeOrder(anotherPanel, panel);
/* [AbstractUserInterface-nodes-order] */

/* [AbstractUserInterface-nodes-order-clear] */
ui.clearNodeOrder(panel);
DOXYGEN_ELLIPSIS()

/* Show the panel again, on top of everything else */
ui.setNodeOrder(panel, Ui::NodeHandle::Null);
/* [AbstractUserInterface-nodes-order-clear] */

/* [AbstractUserInterface-nodes-order-nested] */
/* Tooltip rectangle overlapping the title, shown on the top */
Ui::NodeHandle titleTooltip = ui.createNode(title, {105, 25}, {100, 20});
ui.setNodeOrder(titleTooltip, Ui::NodeHandle::Null);
DOXYGEN_ELLIPSIS()

/* Hide the tooltip when no longer meant to be visible */
ui.clearNodeOrder(titleTooltip);
/* [AbstractUserInterface-nodes-order-nested] */
}

{
Ui::AbstractUserInterface ui{{100, 100}};
Ui::NodeHandle node{};
Ui::FocusEvent event{{}};
/* [AbstractUserInterface-focusEvent-blur-if-not-focusable] */
if(!ui.focusEvent(node, event))
    ui.focusEvent(Ui::NodeHandle::Null, event);
/* [AbstractUserInterface-focusEvent-blur-if-not-focusable] */
}

{
struct Shared: Ui::BaseLayer::Shared {
    explicit Shared(): Ui::BaseLayer::Shared::Shared{Configuration{1}} {}

    void doSetStyle(const Ui::BaseLayerCommonStyleUniform&, Containers::ArrayView<const Ui::BaseLayerStyleUniform>) override {}
} baseLayerShared;
/* [BaseLayer-setup-style] */
baseLayerShared.setStyle(Ui::BaseLayerCommonStyleUniform{}, {
    Ui::BaseLayerStyleUniform{}, /* Style 0, default */
    Ui::BaseLayerStyleUniform{}  /* Style 1 */
        .setColor(0x2f83cc_rgbf),
    Ui::BaseLayerStyleUniform{}  /* Style 2 */
        .setColor(0x00000000_rgbaf)
        .setOutlineColor(0xdcdcdc_rgbf)
        .setOutlineWidth(2.0f)
}, {});
/* [BaseLayer-setup-style] */

struct BaseLayer: Ui::BaseLayer {
    explicit BaseLayer(Ui::LayerHandle layer, Ui::BaseLayer::Shared& shared): Ui::BaseLayer{layer, shared} {}
};
Ui::AbstractUserInterface ui{{100, 100}};
Ui::BaseLayer& baseLayer = ui.setLayerInstance(Containers::pointer<BaseLayer>(ui.createLayer(), baseLayerShared));
{
/* [BaseLayer-create] */
Ui::NodeHandle blueBox = ui.createNode(DOXYGEN_ELLIPSIS({}, {}));

baseLayer.create(1, blueBox);
/* [BaseLayer-create] */
}

{
/* [BaseLayer-style-enums] */
enum class BaseLayerStyle {
    Default,
    Blue,
    Outline,
    Count
};

Ui::BaseLayerStyleUniform uniforms[Int(BaseLayerStyle::Count)];
uniforms[Int(BaseLayerStyle::Blue)]
    .setColor(0x2f83cc_rgbf);
uniforms[Int(BaseLayerStyle::Outline)]
    .setColor(0x00000000_rgbaf)
    .setOutlineColor(0xdcdcdc_rgbf)
    .setOutlineWidth(2.0f);
baseLayerShared.setStyle(Ui::BaseLayerCommonStyleUniform{}, uniforms, {});

DOXYGEN_ELLIPSIS(Ui::NodeHandle blueBox{};)

baseLayer.create(BaseLayerStyle::Blue, blueBox);
/* [BaseLayer-style-enums] */
}

{
/* [BaseLayer-style-color1] */
baseLayerShared.setStyle(DOXYGEN_ELLIPSIS(Ui::BaseLayerCommonStyleUniform{}), {
    Ui::BaseLayerStyleUniform{} /* 0 */
        .setColor(0x2f83cc_rgbf),
    Ui::BaseLayerStyleUniform{} /* 1 */
        .setColor(0xdcdcdc_rgbf, 0xa5c9ea_rgbf)
}, {});

DOXYGEN_ELLIPSIS()

Ui::NodeHandle blueBox = DOXYGEN_ELLIPSIS({});
baseLayer.create(0, blueBox);

Ui::NodeHandle gradient = DOXYGEN_ELLIPSIS({});
baseLayer.create(1, gradient);
/* [BaseLayer-style-color1] */

/* [BaseLayer-style-color2] */
Ui::NodeHandle coloredGradient = DOXYGEN_ELLIPSIS({});
Ui::DataHandle coloredGradientData = baseLayer.create(1, coloredGradient);
baseLayer.setColor(coloredGradientData, 0x3bd267_rgbf);

Ui::NodeHandle fadedGradient = DOXYGEN_ELLIPSIS({});
ui.setNodeOpacity(fadedGradient, 0.25f);
baseLayer.create(1, fadedGradient);
/* [BaseLayer-style-color2] */
}

{
/* [BaseLayer-style-rounded-corners] */
baseLayerShared.setStyle(
    Ui::BaseLayerCommonStyleUniform{}
        .setSmoothness(1.0f),
    {
        Ui::BaseLayerStyleUniform{} /* 0 */
            .setCornerRadius(8.0f)
            .setColor(0xcd3431_rgbf),
        Ui::BaseLayerStyleUniform{} /* 1 */
            /* Top left, bottom left, top right, bottom right */
            .setCornerRadius({8.0f, 1.0f, 8.0f, 1.0f})
            .setColor(0xdcdcdc_rgbf)
    }, {});

DOXYGEN_ELLIPSIS()

Ui::NodeHandle heading = ui.createNode(DOXYGEN_ELLIPSIS({}, {}));
Ui::NodeHandle close = ui.createNode(heading, DOXYGEN_ELLIPSIS({}, {}));
baseLayer.create(1, heading);
baseLayer.create(0, close);
/* [BaseLayer-style-rounded-corners] */
}

{
/* [BaseLayer-style-outline] */
baseLayerShared.setStyle(DOXYGEN_ELLIPSIS(Ui::BaseLayerCommonStyleUniform{}), {
    Ui::BaseLayerStyleUniform{} /* 0 */
        .setColor(0x00000000_rgbaf)
        .setOutlineColor(0xdcdcdc_rgbf)
        .setOutlineWidth(1.0f),
    Ui::BaseLayerStyleUniform{} /* 1 */
        .setColor(0xa5c9ea_rgbf)
        .setOutlineColor(0x405363_rgbf)
        /* Left, top, right, bottom */
        .setOutlineWidth({1.0f, 1.0f, 16.0f, 1.0f})
        .setCornerRadius(12.0f)
        .setInnerOutlineCornerRadius(11.0f),
    Ui::BaseLayerStyleUniform{} /* 2 */
        .setColor(0x2a703f_rgbf)
        .setOutlineColor(0x3bd267_rgbf)
        .setOutlineWidth(2.0f)
        .setCornerRadius(2.0f)
        .setInnerOutlineCornerRadius(10.0f)
}, {});

DOXYGEN_ELLIPSIS()

Ui::NodeHandle frame = DOXYGEN_ELLIPSIS({});
Ui::NodeHandle toggle = DOXYGEN_ELLIPSIS({});
Ui::NodeHandle radio = DOXYGEN_ELLIPSIS({});
baseLayer.create(0, frame);
baseLayer.create(1, toggle);
baseLayer.create(2, radio);
/* [BaseLayer-style-outline] */
}

{
Float percentage{};
/* [BaseLayer-style-outline-data-width] */
baseLayerShared.setStyle(DOXYGEN_ELLIPSIS(Ui::BaseLayerCommonStyleUniform{}), {
    Ui::BaseLayerStyleUniform{} /* 0 */
        .setColor(0x3bd267_rgbf)
        .setOutlineColor(0x405363_rgbf)
        .setCornerRadius(6.0f)
        .setInnerOutlineCornerRadius(6.0f)
}, {});

DOXYGEN_ELLIPSIS()

Ui::NodeHandle progress = DOXYGEN_ELLIPSIS({});
Ui::DataHandle progressData = baseLayer.create(0, progress);
baseLayer.setOutlineWidth(progressData, /* Left, top, right, bottom */
    {0.0f, 0.0f, ui.nodeSize(progress).x()*(100.0f - percentage)/100.0f, 0.0f});
/* [BaseLayer-style-outline-data-width] */
}

{
/* [BaseLayer-style-padding] */
baseLayerShared.setStyle(DOXYGEN_ELLIPSIS(Ui::BaseLayerCommonStyleUniform{}), {
    Ui::BaseLayerStyleUniform{} /* 0 */
        .setColor(0x00000000_rgbaf)
        .setOutlineColor(0xa5c9ea_rgbf)
        .setOutlineWidth(1.0f)
        .setCornerRadius(5.0f)
        .setInnerOutlineCornerRadius(4.0f),
    Ui::BaseLayerStyleUniform{} /* 1 */
        .setColor(0xa5c9ea_rgbf)
        .setCornerRadius(2.0f)
}, {
    {},                         /* 0 */
    Vector4{3.0f}               /* 1 */
});

DOXYGEN_ELLIPSIS()

Ui::NodeHandle button = DOXYGEN_ELLIPSIS({});
baseLayer.create(0, button);
baseLayer.create(1, button);
/* [BaseLayer-style-padding] */
}

{
/* [BaseLayer-style-padding-data] */
baseLayerShared.setStyle(DOXYGEN_ELLIPSIS(Ui::BaseLayerCommonStyleUniform{}), {
    Ui::BaseLayerStyleUniform{} /* 0 */
        .setColor(0x405363_rgbf*0.9f, 0x405363_rgbf*1.1f)
        .setCornerRadius(3.0f),
    Ui::BaseLayerStyleUniform{} /* 1 */
        .setColor(0x3bd267_rgbf*1.1f, 0x3bd267_rgbf*0.9f)
        .setCornerRadius(6.0f)
}, {
    Vector4{3.0f},              /* 0 */
    {},                         /* 1 */
});

DOXYGEN_ELLIPSIS()

Ui::NodeHandle slider = ui.createNode(DOXYGEN_ELLIPSIS({}, {}));
Ui::NodeHandle bar = ui.createNode(slider, {}, ui.nodeSize(slider));
baseLayer.create(0, slider);
baseLayer.create(1, bar);
/* [BaseLayer-style-padding-data] */
}

{
/* [BaseLayer-dynamic-styles-allocate] */
/* Attempt to allocate a dynamic style ID, if available */
Containers::Optional<UnsignedInt> dynamicStyleId = baseLayer.allocateDynamicStyle();
if(!dynamicStyleId) {
    DOXYGEN_ELLIPSIS()
}

/* Populate it, use */
baseLayer.setDynamicStyle(*dynamicStyleId, DOXYGEN_ELLIPSIS(Ui::BaseLayerStyleUniform{}, {}));
DOXYGEN_ELLIPSIS()

/* Once not used anymore, recycle the ID again */
baseLayer.recycleDynamicStyle(*dynamicStyleId);
/* [BaseLayer-dynamic-styles-allocate] */
}
}

{
Ui::AbstractUserInterface ui{{100, 100}};
/* [EventLayer-setup] */
Ui::EventLayer& layer = ui.setLayerInstance(
    Containers::pointer<Ui::EventLayer>(ui.createLayer()));
/* [EventLayer-setup] */

/* [EventLayer-create] */
Ui::NodeHandle button = DOXYGEN_ELLIPSIS({});

layer.onTapOrClick(button, []{
    Debug{} << "Click!";
});
/* [EventLayer-create] */

/* [EventLayer-create-scoped] */
class Observer {
    public:
        explicit Observer(Ui::EventLayer& layer, Ui::NodeHandle button):
            _c{layer.onTapOrClickScoped(button, {*this, &Observer::call})} {}

        void call() {
            DOXYGEN_ELLIPSIS()
        }

    private:
        Ui::EventConnection _c;
};
/* [EventLayer-create-scoped] */
}

{
Ui::AbstractUserInterface ui{{100, 100}};
Ui::EventLayer layer{Ui::layerHandle(0, 1)};
/* [EventLayer-tap-position] */
Ui::NodeHandle picker = DOXYGEN_ELLIPSIS({});

layer.onPress(picker, [&ui, picker, DOXYGEN_ELLIPSIS(&layer)](const Vector2& position) {
    Vector2 normalized = position/ui.nodeSize(picker);
    Color3 color = Color3::fromHsv({DOXYGEN_ELLIPSIS(0.0_degf), normalized.x(), 1.0f - normalized.y()});
    DOXYGEN_ELLIPSIS(static_cast<void>(layer); static_cast<void>(color);)
});
/* [EventLayer-tap-position] */
}

{
Ui::AbstractUserInterface ui{{100, 100}};
Ui::EventLayer layer{Ui::layerHandle(0, 1)};
/* [EventLayer-drag] */
Ui::NodeHandle scrollbar = DOXYGEN_ELLIPSIS({});
Ui::NodeHandle scrollarea = ui.createNode(DOXYGEN_ELLIPSIS({}, {}), Ui::NodeFlag::Clip);
Ui::NodeHandle contents = ui.createNode(scrollarea, DOXYGEN_ELLIPSIS({}, {}));

layer.onDrag(scrollbar, [&ui, scrollarea, contents](const Vector2& relativePosition) {
    Vector2 offset = ui.nodeOffset(contents);
    offset.y() = Math::clamp(offset.y() - relativePosition.y(),
        ui.nodeSize(scrollarea).y() - ui.nodeSize(contents).y(), 0.0f);
    ui.setNodeOffset(contents, offset);
});
/* [EventLayer-drag] */

/* [EventLayer-drag-to-scroll] */
layer.onDrag(scrollarea, [&ui, scrollarea, contents](const Vector2& relativePosition) {
    ui.setNodeOffset(contents, Math::clamp(
        ui.nodeOffset(contents) + relativePosition,
        ui.nodeSize(scrollarea) - ui.nodeSize(contents), Vector2{0.0f}));
});
/* [EventLayer-drag-to-scroll] */

/* [EventLayer-drag-to-scroll-fallthrough] */
ui.addNodeFlags(scrollarea, Ui::NodeFlag::FallthroughPointerEvents);
/* [EventLayer-drag-to-scroll-fallthrough] */
}

{
Ui::AbstractUserInterface ui{{100, 100}};
Ui::EventLayer layer{Ui::layerHandle(0, 1)};
/* [EventLayer-pinch] */
Ui::NodeHandle canvas = DOXYGEN_ELLIPSIS({});

layer.onDrag(canvas, [canvas](const Vector2& position, const Vector2& relativePosition) {
    DOXYGEN_IGNORE(static_cast<void>(position);static_cast<void>(relativePosition);static_cast<void>(canvas);)// Draw ...
});
layer.onPinch(canvas, [&ui, canvas](const Vector2&, const Vector2& relativeTranslation, const Complex&, Float) {
    ui.setNodeOffset(canvas, ui.nodeOffset(canvas) + relativeTranslation);

    // Also discard any in-progress draw from onDrag() that may have been made
    // while just one finger of the two was down ...
});
/* [EventLayer-pinch] */
}

{
Ui::AbstractUserInterface ui{{100, 100}};
/* [BaseLayerStyleAnimator-setup1] */
Ui::BaseLayerStyleAnimator& animator = ui.setStyleAnimatorInstance(
    Containers::pointer<Ui::BaseLayerStyleAnimator>(ui.createAnimator()));
/* [BaseLayerStyleAnimator-setup1] */

Nanoseconds now;
/* [BaseLayerStyleAnimator-create] */
enum class BaseLayerStyle {
    DOXYGEN_ELLIPSIS()
    Button,
    ButtonHovered,
    DOXYGEN_ELLIPSIS()
};

Ui::DataHandle buttonBackground = DOXYGEN_ELLIPSIS({});

animator.create(BaseLayerStyle::ButtonHovered, BaseLayerStyle::Button,
    Animation::Easing::cubicOut, now, 0.5_sec, buttonBackground);
/* [BaseLayerStyleAnimator-create] */
}

{
struct Shared: Ui::LineLayer::Shared {
    explicit Shared(): Ui::LineLayer::Shared::Shared{Configuration{1}} {}

    void doSetStyle(const Ui::LineLayerCommonStyleUniform&, Containers::ArrayView<const Ui::LineLayerStyleUniform>) override {}
} lineLayerShared;
/* [LineLayer-setup-style] */
lineLayerShared.setStyle(Ui::LineLayerCommonStyleUniform{}, {
    Ui::LineLayerStyleUniform{}, /* Style 0, default */
    Ui::LineLayerStyleUniform{}  /* Style 1 */
        .setColor(0x2f83cc_rgbf)
        .setWidth(2.0f),
    Ui::LineLayerStyleUniform{}  /* Style 2 */
        .setColor(0xa5c9ea_rgbf)
        .setWidth(3.0f)
        .setSmoothness(8.0f)
}, {
    Ui::LineAlignment{},
    Ui::LineAlignment::MiddleCenter,
    Ui::LineAlignment::BottomLeft,
}, {});
/* [LineLayer-setup-style] */

struct LineLayer: Ui::LineLayer {
    explicit LineLayer(Ui::LayerHandle layer, Ui::LineLayer::Shared& shared): Ui::LineLayer{layer, shared} {}
};
Ui::AbstractUserInterface ui{{100, 100}};
Ui::LineLayer& lineLayer = ui.setLayerInstance(Containers::pointer<LineLayer>(ui.createLayer(), lineLayerShared));
{
/* [LineLayer-create-strip] */
Ui::NodeHandle node = ui.createNode(DOXYGEN_ELLIPSIS({}, {}));

lineLayer.createStrip(1, {
    {8.0f, -24.0f}, {8.0f, 24.0f}, {56.0f, 24.0f}
}, {}, node);
/* [LineLayer-create-strip] */

/* [LineLayer-create-loop] */
lineLayer.createLoop(1, {
    {-56.0f, -24.0f}, {-8.0f, -24.0f}, {-8.0f, 24.0f}, {-56.0f, 24.0f}
}, {}, node);

lineLayer.createLoop(1, {{56.0f, -24.0f}}, {}, node);
/* [LineLayer-create-loop] */

/* [LineLayer-create-indexed] */
lineLayer.create(1, {
    0, 1, 1, 2,
    3, 4, 4, 5, 5, 6, 6, 3,
    7, 7
}, {
    {8.0f, -24.0f}, {8.0f, 24.0f}, {56.0f, 24.0f},
    {-56.0f, -24.0f}, {-8.0f, -24.0f}, {-8.0f, 24.0f}, {-56.0f, 24.0f},
    {56.0f, -24.0f}
}, {}, node);
/* [LineLayer-create-indexed] */
}

{
/* [LineLayer-style-smoothness] */
lineLayerShared.setStyle(
    Ui::LineLayerCommonStyleUniform{}
        .setSmoothness(1.0f),
    {
        Ui::LineLayerStyleUniform{} /* 0 */
            .setColor(0x2f83cc_rgbf),
        Ui::LineLayerStyleUniform{} /* 1 */
            .setColor(0xa5c9ea_rgbf)
            .setSmoothness(6.0f)
    }, {DOXYGEN_ELLIPSIS(Ui::LineAlignment{})}, {});

DOXYGEN_ELLIPSIS()

Ui::NodeHandle circle = DOXYGEN_ELLIPSIS({});
lineLayer.createLoop(0, {DOXYGEN_ELLIPSIS({})}, {}, circle);

Ui::NodeHandle glow = DOXYGEN_ELLIPSIS({});
lineLayer.createLoop(1, {DOXYGEN_ELLIPSIS({})}, {}, glow);
/* [LineLayer-style-smoothness] */
}

{
/* [LineLayer-style-color1] */
lineLayerShared.setStyle(DOXYGEN_ELLIPSIS(Ui::LineLayerCommonStyleUniform{}), {
    Ui::LineLayerStyleUniform{}, /* 0 */
    Ui::LineLayerStyleUniform{}  /* 1 */
        .setColor(0x2f83cc_rgbf)
}, {DOXYGEN_ELLIPSIS(Ui::LineAlignment{})}, {});

DOXYGEN_ELLIPSIS()

Ui::NodeHandle blue = DOXYGEN_ELLIPSIS({});
lineLayer.createStrip(1, {DOXYGEN_ELLIPSIS({})}, {}, blue);

Ui::NodeHandle colored = DOXYGEN_ELLIPSIS({});
Ui::DataHandle coloredData = lineLayer.createStrip(0, {DOXYGEN_ELLIPSIS({})}, {}, colored);
lineLayer.setColor(coloredData, 0x3bd267_rgbf);

Ui::NodeHandle fadedBlue = DOXYGEN_ELLIPSIS({});
lineLayer.createStrip(1, {DOXYGEN_ELLIPSIS({})}, {}, fadedBlue);
ui.setNodeOpacity(fadedBlue, 0.25f);
/* [LineLayer-style-color1] */

/* [LineLayer-style-color2] */
Ui::NodeHandle gradient = DOXYGEN_ELLIPSIS({});
lineLayer.createStrip(0, {
    DOXYGEN_ELLIPSIS({})
}, {
    0x2f83cc_rgbf, 0x3bd267_rgbf, 0xc7cf2f_rgbf, DOXYGEN_ELLIPSIS()
}, gradient);
/* [LineLayer-style-color2] */

/* [LineLayer-style-alignment-padding] */
lineLayerShared.setStyle(DOXYGEN_ELLIPSIS(Ui::LineLayerCommonStyleUniform{}), {
    Ui::LineLayerStyleUniform{}
        .setWidth(5.0f),
    DOXYGEN_ELLIPSIS()
}, {
    Ui::LineAlignment::BottomLeft,
    DOXYGEN_ELLIPSIS()
}, {
    /* Padding is half of line width to make a point at origin touch the bottom
       left node corner, but not leak out of it */
    Vector4{2.5f},
    DOXYGEN_ELLIPSIS()
});
/* [LineLayer-style-alignment-padding] */

/* [LineLayer-style-width] */
lineLayerShared.setStyle(DOXYGEN_ELLIPSIS(Ui::LineLayerCommonStyleUniform{}), {
    Ui::LineLayerStyleUniform{} /* 0 */
        .setColor(0x2f83cc_rgbf)
        .setWidth(3.0f),
    Ui::LineLayerStyleUniform{} /* 1 */
        .setColor(0x292e32_rgbf)
        .setWidth(11.0f)
        .setSmoothness(1.5f),
    Ui::LineLayerStyleUniform{}  /* 2 */
        .setColor(0xdcdcdc_rgbf)
        .setWidth(10.0f)
}, {DOXYGEN_ELLIPSIS()}, {});

Ui::NodeHandle circle = ui.createNode(DOXYGEN_ELLIPSIS({}, {}));
Ui::NodeHandle pointOuter = ui.createNode(circle, DOXYGEN_ELLIPSIS({}, {}));
Ui::NodeHandle point = ui.createNode(pointOuter, DOXYGEN_ELLIPSIS({}, {}));

lineLayer.createLoop(0, {DOXYGEN_ELLIPSIS({})}, {}, circle);

Vector2 position[]{DOXYGEN_ELLIPSIS({})};
lineLayer.createStrip(1, position, {}, pointOuter);
lineLayer.createStrip(2, position, {}, point);
/* [LineLayer-style-width] */
}
}

{
Ui::AbstractUserInterface ui{{100, 100}};
/* [TextLayerStyleAnimator-setup1] */
Ui::TextLayerStyleAnimator& animator = ui.setStyleAnimatorInstance(
    Containers::pointer<Ui::TextLayerStyleAnimator>(ui.createAnimator()));
/* [TextLayerStyleAnimator-setup1] */

Nanoseconds now;
/* [TextLayerStyleAnimator-create] */
enum class TextLayerStyle {
    DOXYGEN_ELLIPSIS()
    Button,
    ButtonHovered,
    DOXYGEN_ELLIPSIS()
};

Ui::DataHandle buttonText = DOXYGEN_ELLIPSIS({});

animator.create(TextLayerStyle::ButtonHovered, TextLayerStyle::Button,
    Animation::Easing::cubicOut, now, 0.5_sec, buttonText);
/* [TextLayerStyleAnimator-create] */
}

{
Ui::AbstractUserInterface ui{{100, 100}};
/* [GenericAnimator-setup] */
Ui::GenericAnimator& animator = ui.setGenericAnimatorInstance(
    Containers::pointer<Ui::GenericAnimator>(ui.createAnimator()));
/* [GenericAnimator-setup] */

Nanoseconds now;
/* [GenericAnimator-create] */
animator.create([](Float factor) {
    DOXYGEN_ELLIPSIS(static_cast<void>(factor));
}, Animation::Easing::cubicIn, now, 1.5_sec);
/* [GenericAnimator-create] */
}

{
Ui::AbstractUserInterface ui{{100, 100}};
/* [GenericNodeAnimator-setup] */
Ui::GenericNodeAnimator& animator = ui.setGenericAnimatorInstance(
    Containers::pointer<Ui::GenericNodeAnimator>(ui.createAnimator()));
/* [GenericNodeAnimator-setup] */

Nanoseconds now;
/* [GenericNodeAnimator-create] */
Ui::NodeHandle dropdown = DOXYGEN_ELLIPSIS({});

animator.create([&ui](Ui::NodeHandle dropdown, Float factor) {
    ui.setNodeSize(dropdown, {ui.nodeSize(dropdown).x(), 150.0f*factor});
    ui.setNodeOpacity(dropdown, 1.0f*factor);
}, Animation::Easing::cubicIn, now, 0.5_sec, dropdown);
/* [GenericNodeAnimator-create] */
}

{
Ui::AbstractUserInterface ui{{100, 100}};
/* [GenericDataAnimator-setup] */
Ui::AbstractLayer& layer = DOXYGEN_ELLIPSIS(ui.layer({}));

Ui::GenericDataAnimator& animator = ui.setGenericAnimatorInstance(
    Containers::pointer<Ui::GenericDataAnimator>(ui.createAnimator()));
animator.setLayer(layer);
/* [GenericDataAnimator-setup] */

Nanoseconds now;
/* [GenericDataAnimator-create] */
Ui::BaseLayer& baseLayer = DOXYGEN_ELLIPSIS(ui.layer<Ui::BaseLayer>({}));
Ui::DataHandle progressbar = DOXYGEN_ELLIPSIS({});

Float from = DOXYGEN_ELLIPSIS(0.0f);
Float to = DOXYGEN_ELLIPSIS(0.0f);
animator.create([&baseLayer, from, to](Ui::DataHandle progressbar, Float factor) {
    baseLayer.setPadding(progressbar, {Math::lerp(from, to, factor), 0.0f, 0.0f, 0.0f});
}, Animation::Easing::cubicIn, now, 0.5_sec, progressbar);
/* [GenericDataAnimator-create] */
}
}
