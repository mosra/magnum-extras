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

#include "UserInterface.h"

#include <Corrade/Utility/Assert.h>
#include <Magnum/Math/Time.h>

#include "Magnum/Ui/BaseLayer.h"
#include "Magnum/Ui/BaseLayerAnimator.h"
#include "Magnum/Ui/EventLayer.h"
#include "Magnum/Ui/GenericLayouter.h"
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/LayoutLayer.h"
#include "Magnum/Ui/NodeAnimator.h"
#include "Magnum/Ui/SnapLayouter.h"
#include "Magnum/Ui/TextLayer.h"
#include "Magnum/Ui/TextLayerAnimator.h"
#include "Magnum/Ui/Implementation/userInterfaceState.h"

namespace Magnum { namespace Ui {

UserInterface::UserInterface(NoCreateT, Containers::Pointer<State>&& state): AbstractUserInterface{NoCreate}, _state{Utility::move(state)} {}

UserInterface::UserInterface(NoCreateT): UserInterface{NoCreate, Containers::pointer<State>()} {}

UserInterface::UserInterface(UserInterface&&) noexcept = default;

UserInterface::~UserInterface() = default;

UserInterface& UserInterface::operator=(UserInterface&&) noexcept = default;

bool UserInterface::hasBackgroundLayer() const {
    /* The backgroundLayer pointer may alias the base layer if background layer
       hasn't been set. Return false in that case. Doing it this way rather
       than having a special case in the backgroundLayer() accessor as that one
       is likely to get accessed much more frequently and thus it's better to
       not do any branching there. */
    return _state->backgroundLayer && _state->backgroundLayer != _state->baseLayer;
}

const BaseLayer& UserInterface::backgroundLayer() const {
    CORRADE_ASSERT(_state->backgroundLayer,
        "Ui::UserInterface::backgroundLayer(): no instance set", *_state->baseLayer);
    return *_state->backgroundLayer;
}

BaseLayer& UserInterface::backgroundLayer() {
    return const_cast<BaseLayer&>(const_cast<const UserInterface&>(*this).backgroundLayer());
}

UserInterface& UserInterface::setBackgroundLayerInstance(Containers::Pointer<BaseLayer>&& instance) {
    CORRADE_ASSERT(instance,
        "Ui::UserInterface::setBackgroundLayerInstance(): instance is null", *this);
    /* Querying the pointer alone isn't sufficient due to the base layer
       fallback, see hasBackgroundLayer() for details */
    CORRADE_ASSERT(!hasBackgroundLayer(),
        "Ui::UserInterface::setBackgroundLayerInstance(): instance already set", *this);
    _state->backgroundLayer = instance.get();
    /* If a base layer animator was already set, the
       backgroundLayerStyleAnimator pointer is aliased to it. Clear it. */
    _state->backgroundLayerStyleAnimator = {};
    setLayerInstance(Utility::move(instance));
    return *this;
}

bool UserInterface::hasBackgroundLayerStyleAnimator() const {
    /* Similar case as with hasBackgroundLayerStyleAnimator() */
    return _state->backgroundLayerStyleAnimator && _state->backgroundLayerStyleAnimator != _state->baseLayerStyleAnimator;
}

const BaseLayerStyleAnimator& UserInterface::backgroundLayerStyleAnimator() const {
    CORRADE_ASSERT(_state->backgroundLayerStyleAnimator,
        "Ui::UserInterface::backgroundLayerStyleAnimator(): no instance set", *_state->backgroundLayerStyleAnimator);
    return *_state->backgroundLayerStyleAnimator;
}

BaseLayerStyleAnimator& UserInterface::backgroundLayerStyleAnimator() {
    return const_cast<BaseLayerStyleAnimator&>(const_cast<const UserInterface&>(*this).backgroundLayerStyleAnimator());
}

UserInterface& UserInterface::setBackgroundLayerStyleAnimatorInstance(Containers::Pointer<BaseLayerStyleAnimator>&& instance) {
    CORRADE_ASSERT(instance,
        "Ui::UserInterface::setBackgroundLayerStyleAnimatorInstance(): instance is null", *this);
    /* Unlike with setBackgroundLayerInstance(), which has to use
       hasBackgroundLayer() instead of querying the pointer, here the pointer
       gets cleared during the setBackgroundLayerInstance() call, and is not
       set by setBaseLayerStyleAnimatorInstance() if background layer is
       already present. */
    CORRADE_ASSERT(!_state->backgroundLayerStyleAnimator,
        "Ui::UserInterface::setBackgroundLayerStyleAnimatorInstance(): instance already set", *this);
    CORRADE_ASSERT(_state->backgroundLayer,
        "Ui::UserInterface::setBackgroundLayerStyleAnimatorInstance(): background layer instance not set", *this);
    CORRADE_ASSERT(_state->backgroundLayer->shared().dynamicStyleCount(),
        "Ui::UserInterface::setBackgroundLayerStyleAnimatorInstance(): can't animate a background layer with zero dynamic styles", *this);
    CORRADE_ASSERT(instance->layer() == LayerHandle::Null,
        "Ui::UserInterface::setBackgroundLayerStyleAnimatorInstance(): instance already assigned to" << instance->layer(), *this);
    _state->backgroundLayerStyleAnimator = instance.get();
    (*_state->backgroundLayer)
        .assignAnimator(*instance)
        .setDefaultStyleAnimator(instance.get());
    setAnimatorInstance(Utility::move(instance));
    return *this;
}

bool UserInterface::hasBaseLayer() const {
    return _state->baseLayer;
}

const BaseLayer& UserInterface::baseLayer() const {
    CORRADE_ASSERT(_state->baseLayer,
        "Ui::UserInterface::baseLayer(): no instance set", *_state->baseLayer);
    return *_state->baseLayer;
}

BaseLayer& UserInterface::baseLayer() {
    return const_cast<BaseLayer&>(const_cast<const UserInterface&>(*this).baseLayer());
}

UserInterface& UserInterface::setBaseLayerInstance(Containers::Pointer<BaseLayer>&& instance) {
    CORRADE_ASSERT(instance,
        "Ui::UserInterface::setBaseLayerInstance(): instance is null", *this);
    CORRADE_ASSERT(!_state->baseLayer,
        "Ui::UserInterface::setBaseLayerInstance(): instance already set", *this);
    _state->baseLayer = instance.get();
    /* If background layer instance isn't set, use the base layer for it. The
       setBackgroundLayerInstance(), if called, will overwrite this back. Using
       hasBackgroundLayer() instead of just querying the backgroundLayer
       pointer for consistency with setBackgroundLayerInstance(),
       setBaseLayerStyleAnimatorInstance() etc. */
    if(!hasBackgroundLayer())
        _state->backgroundLayer = _state->baseLayer;
    setLayerInstance(Utility::move(instance));
    return *this;
}

bool UserInterface::hasBaseLayerStyleAnimator() const {
    return _state->baseLayerStyleAnimator;
}

const BaseLayerStyleAnimator& UserInterface::baseLayerStyleAnimator() const {
    CORRADE_ASSERT(_state->baseLayerStyleAnimator,
        "Ui::UserInterface::baseLayerStyleAnimator(): no instance set", *_state->baseLayerStyleAnimator);
    return *_state->baseLayerStyleAnimator;
}

BaseLayerStyleAnimator& UserInterface::baseLayerStyleAnimator() {
    return const_cast<BaseLayerStyleAnimator&>(const_cast<const UserInterface&>(*this).baseLayerStyleAnimator());
}

UserInterface& UserInterface::setBaseLayerStyleAnimatorInstance(Containers::Pointer<BaseLayerStyleAnimator>&& instance) {
    CORRADE_ASSERT(instance,
        "Ui::UserInterface::setBaseLayerStyleAnimatorInstance(): instance is null", *this);
    CORRADE_ASSERT(!_state->baseLayerStyleAnimator,
        "Ui::UserInterface::setBaseLayerStyleAnimatorInstance(): instance already set", *this);
    CORRADE_ASSERT(_state->baseLayer,
        "Ui::UserInterface::setBaseLayerStyleAnimatorInstance(): base layer instance not set", *this);
    CORRADE_ASSERT(_state->baseLayer->shared().dynamicStyleCount(),
        "Ui::UserInterface::setBaseLayerStyleAnimatorInstance(): can't animate a base layer with zero dynamic styles", *this);
    CORRADE_ASSERT(instance->layer() == LayerHandle::Null,
        "Ui::UserInterface::setBaseLayerStyleAnimatorInstance(): instance already assigned to" << instance->layer(), *this);
    _state->baseLayerStyleAnimator = instance.get();
    /* If background layer instance isn't set, use the base layer for it. The
       setBackgroundLayerInstance(), if called, will overwrite this back. Can't
       query just the backgroundLayer pointer as it may be already aliased to
       baseLayer and thus not null, have to use hasBackgroundLayer(). Cannot
       also query the backgroundLayerStyleAnimator pointer alone as, if
       background layer is present but its animator isn't, it'd lead to
       associating base layer animator with the background layer */
    if(!hasBackgroundLayer())
        _state->backgroundLayerStyleAnimator = _state->baseLayerStyleAnimator;
    (*_state->baseLayer)
        .assignAnimator(*instance)
        .setDefaultStyleAnimator(instance.get());
    setAnimatorInstance(Utility::move(instance));
    return *this;
}

bool UserInterface::hasTextLayer() const {
    return _state->textLayer;
}

const TextLayer& UserInterface::textLayer() const {
    CORRADE_ASSERT(_state->textLayer,
        "Ui::UserInterface::textLayer(): no instance set", *_state->textLayer);
    return *_state->textLayer;
}

TextLayer& UserInterface::textLayer() {
    return const_cast<TextLayer&>(const_cast<const UserInterface&>(*this).textLayer());
}

UserInterface& UserInterface::setTextLayerInstance(Containers::Pointer<TextLayer>&& instance) {
    CORRADE_ASSERT(instance,
        "Ui::UserInterface::setTextLayerInstance(): instance is null", *this);
    CORRADE_ASSERT(!_state->textLayer,
        "Ui::UserInterface::setTextLayerInstance(): instance already set", *this);
    _state->textLayer = instance.get();
    setLayerInstance(Utility::move(instance));
    return *this;
}

bool UserInterface::hasTextLayerStyleAnimator() const {
    return _state->textLayerStyleAnimator;
}

const TextLayerStyleAnimator& UserInterface::textLayerStyleAnimator() const {
    CORRADE_ASSERT(_state->textLayerStyleAnimator,
        "Ui::UserInterface::textLayerStyleAnimator(): no instance set", *_state->textLayerStyleAnimator);
    return *_state->textLayerStyleAnimator;
}

TextLayerStyleAnimator& UserInterface::textLayerStyleAnimator() {
    return const_cast<TextLayerStyleAnimator&>(const_cast<const UserInterface&>(*this).textLayerStyleAnimator());
}

UserInterface& UserInterface::setTextLayerStyleAnimatorInstance(Containers::Pointer<TextLayerStyleAnimator>&& instance) {
    CORRADE_ASSERT(instance,
        "Ui::UserInterface::setTextLayerStyleAnimatorInstance(): instance is null", *this);
    CORRADE_ASSERT(!_state->textLayerStyleAnimator,
        "Ui::UserInterface::setTextLayerStyleAnimatorInstance(): instance already set", *this);
    CORRADE_ASSERT(_state->textLayer,
        "Ui::UserInterface::setTextLayerStyleAnimatorInstance(): text layer instance not set", *this);
    CORRADE_ASSERT(_state->textLayer->shared().dynamicStyleCount(),
        "Ui::UserInterface::setTextLayerStyleAnimatorInstance(): can't animate a text layer with zero dynamic styles", *this);
    CORRADE_ASSERT(instance->layer() == LayerHandle::Null,
        "Ui::UserInterface::setTextLayerStyleAnimatorInstance(): instance already assigned to" << instance->layer(), *this);
    _state->textLayerStyleAnimator = instance.get();
    (*_state->textLayer)
        .assignAnimator(*instance)
        .setDefaultStyleAnimator(instance.get());
    setAnimatorInstance(Utility::move(instance));
    return *this;
}

bool UserInterface::hasEventLayer() const {
    return _state->eventLayer;
}

const EventLayer& UserInterface::eventLayer() const {
    CORRADE_ASSERT(_state->eventLayer,
        "Ui::UserInterface::eventLayer(): no instance set", *_state->eventLayer);
    return *_state->eventLayer;
}

EventLayer& UserInterface::eventLayer() {
    return const_cast<EventLayer&>(const_cast<const UserInterface&>(*this).eventLayer());
}

UserInterface& UserInterface::setEventLayerInstance(Containers::Pointer<EventLayer>&& instance) {
    CORRADE_ASSERT(instance,
        "Ui::UserInterface::setEventLayerInstance(): instance is null", *this);
    CORRADE_ASSERT(!_state->eventLayer,
        "Ui::UserInterface::setEventLayerInstance(): instance already set", *this);
    _state->eventLayer = instance.get();
    setLayerInstance(Utility::move(instance));
    return *this;
}

bool UserInterface::hasLayoutLayer() const {
    return _state->layoutLayer;
}

const LayoutLayer& UserInterface::layoutLayer() const {
    CORRADE_ASSERT(_state->layoutLayer,
        "Ui::UserInterface::layoutLayer(): no instance set", *_state->layoutLayer);
    return *_state->layoutLayer;
}

LayoutLayer& UserInterface::layoutLayer() {
    return const_cast<LayoutLayer&>(const_cast<const UserInterface&>(*this).layoutLayer());
}

UserInterface& UserInterface::setLayoutLayerInstance(Containers::Pointer<LayoutLayer>&& instance) {
    CORRADE_ASSERT(instance,
        "Ui::UserInterface::setLayoutLayerInstance(): instance is null", *this);
    CORRADE_ASSERT(!_state->layoutLayer,
        "Ui::UserInterface::setLayoutLayerInstance(): instance already set", *this);
    _state->layoutLayer = instance.get();
    setLayerInstance(Utility::move(instance));
    return *this;
}

bool UserInterface::hasSnapLayouter() const {
    return _state->snapLayouter;
}

const SnapLayouter& UserInterface::snapLayouter() const {
    CORRADE_ASSERT(_state->snapLayouter,
        "Ui::UserInterface::snapLayouter(): no instance set", *_state->snapLayouter);
    return *_state->snapLayouter;
}

SnapLayouter& UserInterface::snapLayouter() {
    return const_cast<SnapLayouter&>(const_cast<const UserInterface&>(*this).snapLayouter());
}

UserInterface& UserInterface::setSnapLayouterInstance(Containers::Pointer<SnapLayouter>&& instance) {
    CORRADE_ASSERT(instance,
        "Ui::UserInterface::setSnapLayouterInstance(): instance is null", *this);
    CORRADE_ASSERT(!_state->snapLayouter,
        "Ui::UserInterface::setSnapLayouterInstance(): instance already set", *this);
    _state->snapLayouter = instance.get();
    setLayouterInstance(Utility::move(instance));
    return *this;
}

bool UserInterface::hasGenericLayouter() const {
    return _state->genericLayouter;
}

const GenericLayouter& UserInterface::genericLayouter() const {
    CORRADE_ASSERT(_state->genericLayouter,
        "Ui::UserInterface::genericLayouter(): no instance set", *_state->genericLayouter);
    return *_state->genericLayouter;
}

GenericLayouter& UserInterface::genericLayouter() {
    return const_cast<GenericLayouter&>(const_cast<const UserInterface&>(*this).genericLayouter());
}

UserInterface& UserInterface::setGenericLayouterInstance(Containers::Pointer<GenericLayouter>&& instance) {
    CORRADE_ASSERT(instance,
        "Ui::UserInterface::setGenericLayouterInstance(): instance is null", *this);
    CORRADE_ASSERT(!_state->genericLayouter,
        "Ui::UserInterface::setGenericLayouterInstance(): instance already set", *this);
    _state->genericLayouter = instance.get();
    setLayouterInstance(Utility::move(instance));
    return *this;
}

bool UserInterface::hasNodeAnimator() const {
    return _state->nodeAnimator;
}

const NodeAnimator& UserInterface::nodeAnimator() const {
    CORRADE_ASSERT(_state->nodeAnimator,
        "Ui::UserInterface::nodeAnimator(): no instance set", *_state->nodeAnimator);
    return *_state->nodeAnimator;
}

NodeAnimator& UserInterface::nodeAnimator() {
    return const_cast<NodeAnimator&>(const_cast<const UserInterface&>(*this).nodeAnimator());
}

UserInterface& UserInterface::setNodeAnimatorInstance(Containers::Pointer<NodeAnimator>&& instance) {
    CORRADE_ASSERT(instance,
        "Ui::UserInterface::setNodeAnimatorInstance(): instance is null", *this);
    CORRADE_ASSERT(!_state->nodeAnimator,
        "Ui::UserInterface::setNodeAnimatorInstance(): instance already set", *this);
    _state->nodeAnimator = instance.get();
    setAnimatorInstance(Utility::move(instance));
    return *this;
}

UserInterface& UserInterface::advanceAnimations(const Nanoseconds time) {
    return static_cast<UserInterface&>(AbstractUserInterface::advanceAnimations(time));
}

}}
