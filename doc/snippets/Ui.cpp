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
#include <Magnum/Animation/Easing.h>

#include "Magnum/Ui/AbstractUserInterface.h"
#include "Magnum/Ui/AbstractVisualLayer.h"
#include "Magnum/Ui/BaseLayer.h"
#include "Magnum/Ui/BaseLayerAnimator.h"
#include "Magnum/Ui/TextLayerAnimator.h"
#include "Magnum/Ui/Event.h"
#include "Magnum/Ui/GenericAnimator.h"
#include "Magnum/Ui/Handle.h"

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

/* Make sure the name doesn't conflict with any other snippets to avoid linker
   warnings, unlike with `int main()` there now has to be a declaration to
   avoid -Wmisssing-prototypes */
void mainUi();
void mainUi() {
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
