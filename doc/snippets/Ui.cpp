/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023, 2024
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
#include <Corrade/Containers/String.h>
#include <Magnum/Animation/Easing.h>

#include "Magnum/Ui/AbstractUserInterface.h"
#include "Magnum/Ui/AbstractVisualLayer.h"
#include "Magnum/Ui/BaseLayer.h"
#include "Magnum/Ui/BaseLayerAnimator.h"
#include "Magnum/Ui/TextLayerAnimator.h"
#include "Magnum/Ui/Event.h"
#include "Magnum/Ui/EventLayer.h"
#include "Magnum/Ui/GenericAnimator.h"
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/NodeFlags.h"

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

namespace A {

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
    ButtonHover,
    DOXYGEN_ELLIPSIS()
};

Ui::DataHandle buttonBackground = DOXYGEN_ELLIPSIS({});

animator.create(BaseLayerStyle::ButtonHover, BaseLayerStyle::Button,
    Animation::Easing::cubicOut, now, 0.5_sec, buttonBackground);
/* [BaseLayerStyleAnimator-create] */
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
    ButtonHover,
    DOXYGEN_ELLIPSIS()
};

Ui::DataHandle buttonText = DOXYGEN_ELLIPSIS({});

animator.create(TextLayerStyle::ButtonHover, TextLayerStyle::Button,
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
