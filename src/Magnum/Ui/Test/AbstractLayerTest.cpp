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

#include <sstream>
#include <Corrade/Containers/BitArrayView.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/Iterable.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Containers/StridedBitArrayView.h>
#include <Corrade/Containers/StringStl.h> /** @todo remove once Debug is stream-free */
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/Container.h>
#include <Corrade/TestSuite/Compare/String.h>
#include <Corrade/Utility/DebugStl.h>
#include <Magnum/Math/Time.h>
#include <Magnum/Math/Vector2.h>

#include "Magnum/Ui/AbstractAnimator.h"
#include "Magnum/Ui/AbstractLayer.h"
#include "Magnum/Ui/AbstractRenderer.h"
#include "Magnum/Ui/Event.h"
#include "Magnum/Ui/Handle.h"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct AbstractLayerTest: TestSuite::Tester {
    explicit AbstractLayerTest();

    void debugFeature();
    void debugFeatures();
    void debugFeaturesSupersets();
    void debugState();
    void debugStates();
    void debugStatesSupersets();

    void construct();
    void constructInvalidHandle();
    void constructCopy();
    void constructMove();

    void stateQuery();
    void stateQueryNotImplemented();
    void stateQueryInvalid();

    void setNeedsUpdate();
    void setNeedsUpdateInvalid();

    void createRemove();
    void createRemoveHandleRecycle();
    void createRemoveHandleDisable();
    void createNoHandlesLeft();
    void createAttached();
    void removeInvalid();
    void attach();
    void attachInvalid();

    void setSize();
    void setSizeZero();
    void setSizeNotSupported();
    void setSizeNotImplemented();

    void assignDataAnimator();
    void assignStyleAnimator();
    void assignDataAnimatorNotSupported();
    void assignStyleAnimatorNotSupported();
    void assignDataAnimatorInvalid();
    void assignStyleAnimatorInvalid();

    void cleanNodes();
    void cleanNodesEmpty();
    void cleanNodesNotImplemented();

    void cleanDataAnimators();
    void cleanDataAnimatorsEmpty();
    void cleanDataAnimatorsInvalidFeatures();
    void cleanDataAnimatorsLayerNotSet();
    void cleanDataAnimatorsInvalidLayer();

    void advanceDataAnimations();
    void advanceStyleAnimations();
    void advanceDataAnimationsEmpty();
    void advanceStyleAnimationsEmpty();
    void advanceDataAnimationsNotSupported();
    void advanceStyleAnimationsNotSupported();
    void advanceDataAnimationsNotImplemented();
    void advanceStyleAnimationsNotImplemented();
    void advanceDataAnimationsInvalidFeatures();
    void advanceStyleAnimationsInvalidFeatures();
    void advanceDataAnimationsLayerNotSet();
    void advanceStyleAnimationsLayerNotSet();
    void advanceDataAnimationsInvalidLayer();
    void advanceStyleAnimationsInvalidLayer();
    void advanceDataAnimationsInvalidSize();
    void advanceStyleAnimationsInvalidSize();

    void update();
    void updateComposite();
    void updateEmpty();
    void updateNotImplemented();
    void updateInvalidState();
    void updateInvalidStateComposite();
    void updateInvalidSizes();
    void updateNoSizeSet();

    void state();

    void composite();
    void compositeEmpty();
    void compositeNotSupported();
    void compositeNotImplemented();
    void compositeInvalidSizes();

    void draw();
    void drawEmpty();
    void drawNotSupported();
    void drawNotImplemented();
    void drawInvalidSizes();

    void pointerEvent();
    void pointerEventNotSupported();
    void pointerEventNotImplemented();
    void pointerEventOutOfRange();
    void pointerEventNotPrimary();
    void pointerEventAlreadyAccepted();

    void focusBlurEvent();
    void focusBlurEventNotSupported();
    void focusBlurEventNotImplemented();
    void focusBlurEventOutOfRange();
    void focusBlurEventAlreadyAccepted();

    void keyEvent();
    void keyEventNotSupported();
    void keyEventNotImplemented();
    void keyEventOutOfRange();
    void keyEventAlreadyAccepted();

    void textInputEvent();
    void textInputEventNotSupported();
    void textInputEventNotImplemented();
    void textInputEventOutOfRange();
    void textInputEventAlreadyAccepted();

    void visibilityLostEvent();
    void visibilityLostEventNotSupported();
    void visibilityLostEventNotImplemented();
    void visibilityLostEventOutOfRange();
};

using namespace Containers::Literals;
using namespace Math::Literals;

const struct {
    const char* name;
    LayerFeatures features;
    LayerStates extraState;
} StateQuerySetNeedsUpdateData[]{
    {"", {}, {}},
    {"composite layer", LayerFeature::Composite, LayerState::NeedsCompositeOffsetSizeUpdate},
};

const struct {
    const char* name;
    LayerFeatures features;
    LayerStates extraAttachState;
} StateData[]{
    {"", {}, {}},
    {"composite layer", LayerFeature::Composite, LayerState::NeedsCompositeOffsetSizeUpdate},
};

AbstractLayerTest::AbstractLayerTest() {
    addTests({&AbstractLayerTest::debugFeature,
              &AbstractLayerTest::debugFeatures,
              &AbstractLayerTest::debugFeaturesSupersets,
              &AbstractLayerTest::debugState,
              &AbstractLayerTest::debugStates,
              &AbstractLayerTest::debugStatesSupersets,

              &AbstractLayerTest::construct,
              &AbstractLayerTest::constructInvalidHandle,
              &AbstractLayerTest::constructCopy,
              &AbstractLayerTest::constructMove});

    addInstancedTests({&AbstractLayerTest::stateQuery},
        Containers::arraySize(StateQuerySetNeedsUpdateData));

    addTests({&AbstractLayerTest::stateQueryNotImplemented,
              &AbstractLayerTest::stateQueryInvalid});

    addInstancedTests({&AbstractLayerTest::setNeedsUpdate},
        Containers::arraySize(StateQuerySetNeedsUpdateData));

    addTests({&AbstractLayerTest::setNeedsUpdateInvalid,

              &AbstractLayerTest::createRemove,
              &AbstractLayerTest::createRemoveHandleRecycle,
              &AbstractLayerTest::createRemoveHandleDisable,
              &AbstractLayerTest::createNoHandlesLeft,
              &AbstractLayerTest::createAttached,
              &AbstractLayerTest::removeInvalid,
              &AbstractLayerTest::attach,
              &AbstractLayerTest::attachInvalid,

              &AbstractLayerTest::setSize,
              &AbstractLayerTest::setSizeZero,
              &AbstractLayerTest::setSizeNotSupported,
              &AbstractLayerTest::setSizeNotImplemented,

              &AbstractLayerTest::assignDataAnimator,
              &AbstractLayerTest::assignStyleAnimator,
              &AbstractLayerTest::assignDataAnimatorNotSupported,
              &AbstractLayerTest::assignStyleAnimatorNotSupported,
              &AbstractLayerTest::assignDataAnimatorInvalid,
              &AbstractLayerTest::assignStyleAnimatorInvalid,

              &AbstractLayerTest::cleanNodes,
              &AbstractLayerTest::cleanNodesEmpty,
              &AbstractLayerTest::cleanNodesNotImplemented,

              &AbstractLayerTest::cleanDataAnimators,
              &AbstractLayerTest::cleanDataAnimatorsEmpty,
              &AbstractLayerTest::cleanDataAnimatorsInvalidFeatures,
              &AbstractLayerTest::cleanDataAnimatorsLayerNotSet,
              &AbstractLayerTest::cleanDataAnimatorsInvalidLayer,

              &AbstractLayerTest::advanceDataAnimations,
              &AbstractLayerTest::advanceStyleAnimations,
              &AbstractLayerTest::advanceDataAnimationsEmpty,
              &AbstractLayerTest::advanceStyleAnimationsEmpty,
              &AbstractLayerTest::advanceDataAnimationsNotSupported,
              &AbstractLayerTest::advanceStyleAnimationsNotSupported,
              &AbstractLayerTest::advanceDataAnimationsNotImplemented,
              &AbstractLayerTest::advanceStyleAnimationsNotImplemented,
              &AbstractLayerTest::advanceDataAnimationsInvalidFeatures,
              &AbstractLayerTest::advanceStyleAnimationsInvalidFeatures,
              &AbstractLayerTest::advanceDataAnimationsLayerNotSet,
              &AbstractLayerTest::advanceStyleAnimationsLayerNotSet,
              &AbstractLayerTest::advanceDataAnimationsInvalidLayer,
              &AbstractLayerTest::advanceStyleAnimationsInvalidLayer,
              &AbstractLayerTest::advanceDataAnimationsInvalidSize,
              &AbstractLayerTest::advanceStyleAnimationsInvalidSize,

              &AbstractLayerTest::update,
              &AbstractLayerTest::updateComposite,
              &AbstractLayerTest::updateEmpty,
              &AbstractLayerTest::updateNotImplemented,
              &AbstractLayerTest::updateInvalidState,
              &AbstractLayerTest::updateInvalidStateComposite,
              &AbstractLayerTest::updateInvalidSizes,
              &AbstractLayerTest::updateNoSizeSet});

    addInstancedTests({&AbstractLayerTest::state},
        Containers::arraySize(StateData));

    addTests({&AbstractLayerTest::composite,
              &AbstractLayerTest::compositeEmpty,
              &AbstractLayerTest::compositeNotSupported,
              &AbstractLayerTest::compositeNotImplemented,
              &AbstractLayerTest::compositeInvalidSizes,

              &AbstractLayerTest::draw,
              &AbstractLayerTest::drawEmpty,
              &AbstractLayerTest::drawNotSupported,
              &AbstractLayerTest::drawNotImplemented,
              &AbstractLayerTest::drawInvalidSizes,

              &AbstractLayerTest::pointerEvent,
              &AbstractLayerTest::pointerEventNotSupported,
              &AbstractLayerTest::pointerEventNotImplemented,
              &AbstractLayerTest::pointerEventOutOfRange,
              &AbstractLayerTest::pointerEventNotPrimary,
              &AbstractLayerTest::pointerEventAlreadyAccepted,

              &AbstractLayerTest::focusBlurEvent,
              &AbstractLayerTest::focusBlurEventNotSupported,
              &AbstractLayerTest::focusBlurEventNotImplemented,
              &AbstractLayerTest::focusBlurEventOutOfRange,
              &AbstractLayerTest::focusBlurEventAlreadyAccepted,

              &AbstractLayerTest::keyEvent,
              &AbstractLayerTest::keyEventNotSupported,
              &AbstractLayerTest::keyEventNotImplemented,
              &AbstractLayerTest::keyEventOutOfRange,
              &AbstractLayerTest::keyEventAlreadyAccepted,

              &AbstractLayerTest::textInputEvent,
              &AbstractLayerTest::textInputEventNotSupported,
              &AbstractLayerTest::textInputEventNotImplemented,
              &AbstractLayerTest::textInputEventOutOfRange,
              &AbstractLayerTest::textInputEventAlreadyAccepted,

              &AbstractLayerTest::visibilityLostEvent,
              &AbstractLayerTest::visibilityLostEventNotSupported,
              &AbstractLayerTest::visibilityLostEventNotImplemented,
              &AbstractLayerTest::visibilityLostEventOutOfRange});
}

void AbstractLayerTest::debugFeature() {
    std::ostringstream out;
    Debug{&out} << LayerFeature::Draw << LayerFeature(0xbe);
    CORRADE_COMPARE(out.str(), "Ui::LayerFeature::Draw Ui::LayerFeature(0xbe)\n");
}

void AbstractLayerTest::debugFeatures() {
    std::ostringstream out;
    Debug{&out} << (LayerFeature::Draw|LayerFeature(0x80)) << LayerFeatures{};
    CORRADE_COMPARE(out.str(), "Ui::LayerFeature::Draw|Ui::LayerFeature(0x80) Ui::LayerFeatures{}\n");
}

void AbstractLayerTest::debugFeaturesSupersets() {
    /* DrawUsesBlending and DrawUsesScissor are both a superset of Draw, so
       only one should be printed, but if there are both then both should be */
    {
        std::ostringstream out;
        Debug{&out} << (LayerFeature::DrawUsesBlending|LayerFeature::Draw);
        CORRADE_COMPARE(out.str(), "Ui::LayerFeature::DrawUsesBlending\n");
    } {
        std::ostringstream out;
        Debug{&out} << (LayerFeature::DrawUsesScissor|LayerFeature::Draw);
        CORRADE_COMPARE(out.str(), "Ui::LayerFeature::DrawUsesScissor\n");
    } {
        std::ostringstream out;
        Debug{&out} << (LayerFeature::DrawUsesBlending|LayerFeature::DrawUsesScissor);
        CORRADE_COMPARE(out.str(), "Ui::LayerFeature::DrawUsesBlending|Ui::LayerFeature::DrawUsesScissor\n");

    /* Composite is a superset of Draw, so only one should be printed */
    } {
        std::ostringstream out;
        Debug{&out} << (LayerFeature::Composite|LayerFeature::Draw);
        CORRADE_COMPARE(out.str(), "Ui::LayerFeature::Composite\n");
    }
}

void AbstractLayerTest::debugState() {
    std::ostringstream out;
    Debug{&out} << LayerState::NeedsAttachmentUpdate << LayerState(0xbebe);
    CORRADE_COMPARE(out.str(), "Ui::LayerState::NeedsAttachmentUpdate Ui::LayerState(0xbebe)\n");
}

void AbstractLayerTest::debugStates() {
    std::ostringstream out;
    Debug{&out} << (LayerState::NeedsSharedDataUpdate|LayerState(0xbe00)) << LayerStates{};
    CORRADE_COMPARE(out.str(), "Ui::LayerState::NeedsSharedDataUpdate|Ui::LayerState(0xbe00) Ui::LayerStates{}\n");
}

void AbstractLayerTest::debugStatesSupersets() {
    /* NeedsAttachmentUpdate and NeedsNodeOffsetSizeUpdate are both supersets
       of NeedsNodeOrderUpdate, so only one should be printed, but if there are
       both then both should be */
    {
        std::ostringstream out;
        Debug{&out} << (LayerState::NeedsNodeOrderUpdate|LayerState::NeedsAttachmentUpdate);
        CORRADE_COMPARE(out.str(), "Ui::LayerState::NeedsAttachmentUpdate\n");
    } {
        std::ostringstream out;
        Debug{&out} << (LayerState::NeedsNodeOrderUpdate|LayerState::NeedsNodeOffsetSizeUpdate);
        CORRADE_COMPARE(out.str(), "Ui::LayerState::NeedsNodeOffsetSizeUpdate\n");
    } {
        std::ostringstream out;
        Debug{&out} << (LayerState::NeedsNodeOrderUpdate|LayerState::NeedsNodeOffsetSizeUpdate|LayerState::NeedsAttachmentUpdate);
        CORRADE_COMPARE(out.str(), "Ui::LayerState::NeedsNodeOffsetSizeUpdate|Ui::LayerState::NeedsAttachmentUpdate\n");
    } {

    /* NeedsNodeOrderUpdate is a superset of NeedsNodeEnabledUpdate, so only
       one should be printed */
    } {
        std::ostringstream out;
        Debug{&out} << (LayerState::NeedsNodeEnabledUpdate|LayerState::NeedsNodeOrderUpdate);
        CORRADE_COMPARE(out.str(), "Ui::LayerState::NeedsNodeOrderUpdate\n");
    }
}

void AbstractLayerTest::construct() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return LayerFeatures(0xe0); }
    } layer{layerHandle(0xab, 0x12)};

    CORRADE_COMPARE(layer.handle(), layerHandle(0xab, 0x12));
    CORRADE_COMPARE(layer.features(), LayerFeatures(0xe0));
    CORRADE_COMPARE(layer.state(), LayerStates{});
    CORRADE_COMPARE(layer.capacity(), 0);
    CORRADE_COMPARE(layer.usedCount(), 0);
    CORRADE_VERIFY(!layer.isHandleValid(LayerDataHandle::Null));
    CORRADE_VERIFY(!layer.isHandleValid(DataHandle::Null));
}

void AbstractLayerTest::constructInvalidHandle() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    };

    std::ostringstream out;
    Error redirectError{&out};
    Layer{LayerHandle::Null};
    CORRADE_COMPARE(out.str(),
        "Ui::AbstractLayer: handle is null\n");
}

void AbstractLayerTest::constructCopy() {
    CORRADE_VERIFY(!std::is_copy_constructible<AbstractLayer>{});
    CORRADE_VERIFY(!std::is_copy_assignable<AbstractLayer>{});
}

void AbstractLayerTest::constructMove() {
    struct Layer: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    };

    /* The class has an internal state struct containing everything, so it's
       not needed to test each and every property */
    Layer a{layerHandle(0xab, 0x12)};

    Layer b{Utility::move(a)};
    CORRADE_COMPARE(b.handle(), layerHandle(0xab, 0x12));

    Layer c{layerHandle(0xcd, 0x34)};
    c = Utility::move(b);
    CORRADE_COMPARE(c.handle(), layerHandle(0xab, 0x12));

    CORRADE_VERIFY(std::is_nothrow_move_constructible<Layer>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<Layer>::value);
}

void AbstractLayerTest::stateQuery() {
    auto&& data = StateQuerySetNeedsUpdateData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct Layer: AbstractLayer {
        explicit Layer(LayerHandle handle, LayerFeatures features, LayerStates extraState): AbstractLayer{handle}, _features{features}, _extraState{extraState} {}

        LayerFeatures doFeatures() const override { return _features; }
        LayerStates doState() const override {
            return LayerState::NeedsSharedDataUpdate|_extraState;
        }
        private:
            LayerFeatures _features;
            LayerStates _extraState;
    } layer{layerHandle(0, 1), data.features, data.extraState};
    CORRADE_COMPARE(layer.state(), LayerState::NeedsSharedDataUpdate|data.extraState);

    /* The output of doState() should be combined with flags set directly */
    layer.setNeedsUpdate(LayerState::NeedsDataUpdate);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsSharedDataUpdate|LayerState::NeedsDataUpdate|data.extraState);
}

void AbstractLayerTest::stateQueryNotImplemented() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0, 1)};
    CORRADE_COMPARE(layer.state(), LayerStates{});

    layer.setNeedsUpdate(LayerState::NeedsDataUpdate);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);
}

void AbstractLayerTest::stateQueryInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
        LayerStates doState() const override {
            return LayerState::NeedsSharedDataUpdate|LayerState::NeedsCompositeOffsetSizeUpdate;
        }
    } layer{layerHandle(0, 1)};
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Composite;
        }
        LayerStates doState() const override {
            return LayerState::NeedsNodeEnabledUpdate|LayerState::NeedsSharedDataUpdate;
        }
    } layerCompositing{layerHandle(0, 1)};

    std::ostringstream out;
    Error redirectError{&out};
    layer.state();
    layerCompositing.state();
    CORRADE_COMPARE_AS(out.str(),
        "Ui::AbstractLayer::state(): implementation expected to return a subset of Ui::LayerState::NeedsDataUpdate|Ui::LayerState::NeedsCommonDataUpdate|Ui::LayerState::NeedsSharedDataUpdate but got Ui::LayerState::NeedsSharedDataUpdate|Ui::LayerState::NeedsCompositeOffsetSizeUpdate\n"
        "Ui::AbstractLayer::state(): implementation expected to return a subset of Ui::LayerState::NeedsDataUpdate|Ui::LayerState::NeedsCommonDataUpdate|Ui::LayerState::NeedsSharedDataUpdate|Ui::LayerState::NeedsCompositeOffsetSizeUpdate but got Ui::LayerState::NeedsNodeEnabledUpdate|Ui::LayerState::NeedsSharedDataUpdate\n",
        TestSuite::Compare::String);
}

void AbstractLayerTest::setNeedsUpdate() {
    auto&& data = StateQuerySetNeedsUpdateData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct Layer: AbstractLayer {
        explicit Layer(LayerHandle handle, LayerFeatures features): AbstractLayer{handle}, _features{features} {}

        LayerFeatures doFeatures() const override { return _features; }

        private:
            LayerFeatures _features;
    } layer{layerHandle(0, 1), data.features};
    CORRADE_COMPARE(layer.state(), LayerStates{});

    layer.setNeedsUpdate(LayerState::NeedsSharedDataUpdate|LayerState::NeedsCommonDataUpdate);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsSharedDataUpdate|LayerState::NeedsCommonDataUpdate);

    /* Subsequent set doesn't overwrite, but ORs with existing */
    layer.setNeedsUpdate(LayerState::NeedsDataUpdate|data.extraState);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate|LayerState::NeedsSharedDataUpdate|LayerState::NeedsCommonDataUpdate|data.extraState);
}

void AbstractLayerTest::setNeedsUpdateInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0, 1)};
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Composite;
        }
    } layerCompositing{layerHandle(0, 1)};
    CORRADE_COMPARE(layer.state(), LayerStates{});
    CORRADE_COMPARE(layerCompositing.state(), LayerStates{});

    std::ostringstream out;
    Error redirectError{&out};
    layer.setNeedsUpdate({});
    layer.setNeedsUpdate(LayerState::NeedsCompositeOffsetSizeUpdate);
    layerCompositing.setNeedsUpdate(LayerState::NeedsNodeOffsetSizeUpdate);
    CORRADE_COMPARE_AS(out.str(),
        "Ui::AbstractLayer::setNeedsUpdate(): expected a non-empty subset of Ui::LayerState::NeedsDataUpdate|Ui::LayerState::NeedsCommonDataUpdate|Ui::LayerState::NeedsSharedDataUpdate but got Ui::LayerStates{}\n"
        "Ui::AbstractLayer::setNeedsUpdate(): expected a non-empty subset of Ui::LayerState::NeedsDataUpdate|Ui::LayerState::NeedsCommonDataUpdate|Ui::LayerState::NeedsSharedDataUpdate but got Ui::LayerState::NeedsCompositeOffsetSizeUpdate\n"
        "Ui::AbstractLayer::setNeedsUpdate(): expected a non-empty subset of Ui::LayerState::NeedsDataUpdate|Ui::LayerState::NeedsCommonDataUpdate|Ui::LayerState::NeedsSharedDataUpdate|Ui::LayerState::NeedsCompositeOffsetSizeUpdate but got Ui::LayerState::NeedsNodeOffsetSizeUpdate\n",
        TestSuite::Compare::String);
}

void AbstractLayerTest::createRemove() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;
        using AbstractLayer::remove;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0xab, 0x12)};

    /* The node argument is tested in createAttached() below */

    DataHandle first = layer.create();
    CORRADE_COMPARE(first, dataHandle(layer.handle(), 0, 1));
    CORRADE_VERIFY(layer.isHandleValid(first));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);
    CORRADE_COMPARE(layer.capacity(), 1);
    CORRADE_COMPARE(layer.usedCount(), 1);
    CORRADE_COMPARE(layer.node(first), NodeHandle::Null);

    DataHandle second = layer.create();
    CORRADE_COMPARE(second, dataHandle(layer.handle(), 1, 1));
    CORRADE_VERIFY(layer.isHandleValid(second));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);
    CORRADE_COMPARE(layer.capacity(), 2);
    CORRADE_COMPARE(layer.usedCount(), 2);
    CORRADE_COMPARE(layer.node(second), NodeHandle::Null);

    layer.remove(first);
    CORRADE_VERIFY(!layer.isHandleValid(first));
    CORRADE_VERIFY(layer.isHandleValid(second));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate|LayerState::NeedsDataClean);
    CORRADE_COMPARE(layer.capacity(), 2);
    CORRADE_COMPARE(layer.usedCount(), 1);

    /* Using also the LayouterDataHandle overload */
    layer.remove(dataHandleData(second));
    CORRADE_VERIFY(!layer.isHandleValid(first));
    CORRADE_VERIFY(!layer.isHandleValid(second));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate|LayerState::NeedsDataClean);
    CORRADE_COMPARE(layer.capacity(), 2);
    CORRADE_COMPARE(layer.usedCount(), 0);
}

void AbstractLayerTest::createRemoveHandleRecycle() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;
        using AbstractLayer::remove;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0xab, 0x12)};
    DataHandle first = layer.create();
    DataHandle second = layer.create();
    DataHandle third = layer.create();
    DataHandle fourth = layer.create();
    CORRADE_COMPARE(first, dataHandle(layer.handle(), 0, 1));
    CORRADE_COMPARE(second, dataHandle(layer.handle(), 1, 1));
    CORRADE_COMPARE(third, dataHandle(layer.handle(), 2, 1));
    CORRADE_COMPARE(fourth, dataHandle(layer.handle(), 3, 1));
    CORRADE_VERIFY(layer.isHandleValid(first));
    CORRADE_VERIFY(layer.isHandleValid(second));
    CORRADE_VERIFY(layer.isHandleValid(third));
    CORRADE_VERIFY(layer.isHandleValid(fourth));
    CORRADE_COMPARE(layer.capacity(), 4);
    CORRADE_COMPARE(layer.usedCount(), 4);
    CORRADE_COMPARE(layer.node(first), NodeHandle::Null);
    CORRADE_COMPARE(layer.node(second), NodeHandle::Null);
    CORRADE_COMPARE(layer.node(third), NodeHandle::Null);
    CORRADE_COMPARE(layer.node(fourth), NodeHandle::Null);

    /* Attach some handles to an arbitrary node to populate their internals */
    layer.attach(first, NodeHandle(0xabc12345));
    layer.attach(third, NodeHandle(0x123abcde));
    CORRADE_COMPARE(layer.node(first), NodeHandle(0xabc12345));
    CORRADE_COMPARE(layer.node(third), NodeHandle(0x123abcde));
    CORRADE_COMPARE_AS(layer.nodes(), Containers::arrayView({
        NodeHandle(0xabc12345),
        NodeHandle::Null,
        NodeHandle(0x123abcde),
        NodeHandle::Null
    }), TestSuite::Compare::Container);

    /* Remove three out of the four in an arbitrary order */
    layer.remove(fourth);
    layer.remove(first);
    layer.remove(third);
    CORRADE_VERIFY(!layer.isHandleValid(first));
    CORRADE_VERIFY(layer.isHandleValid(second));
    CORRADE_VERIFY(!layer.isHandleValid(third));
    CORRADE_VERIFY(!layer.isHandleValid(fourth));
    CORRADE_COMPARE(layer.capacity(), 4);
    CORRADE_COMPARE(layer.usedCount(), 1);
    CORRADE_COMPARE(layer.node(second), NodeHandle::Null);

    /* Internally all attachments should be set to a null handle after
       deletion */
    CORRADE_COMPARE_AS(layer.nodes(), Containers::arrayView({
        NodeHandle::Null,
        NodeHandle::Null,
        NodeHandle::Null,
        NodeHandle::Null
    }), TestSuite::Compare::Container);

    /* Allocating new handles should recycle the handles in the order they were
       removed (oldest first). Their properties should be cleared. */
    DataHandle fourth2 = layer.create();
    DataHandle first2 = layer.create();
    DataHandle third2 = layer.create();
    CORRADE_COMPARE(first2, dataHandle(layer.handle(), 0, 2));
    CORRADE_COMPARE(third2, dataHandle(layer.handle(), 2, 2));
    CORRADE_COMPARE(fourth2, dataHandle(layer.handle(), 3, 2));
    CORRADE_COMPARE(layer.capacity(), 4);
    CORRADE_COMPARE(layer.usedCount(), 4);
    CORRADE_COMPARE(layer.node(first2), NodeHandle::Null);
    CORRADE_COMPARE(layer.node(second), NodeHandle::Null);
    CORRADE_COMPARE(layer.node(third2), NodeHandle::Null);
    CORRADE_COMPARE(layer.node(fourth2), NodeHandle::Null);

    /* Old handles shouldn't get valid again */
    CORRADE_VERIFY(!layer.isHandleValid(first));
    CORRADE_VERIFY(layer.isHandleValid(first2));
    CORRADE_VERIFY(!layer.isHandleValid(third));
    CORRADE_VERIFY(layer.isHandleValid(third2));
    CORRADE_VERIFY(!layer.isHandleValid(fourth));
    CORRADE_VERIFY(layer.isHandleValid(fourth2));

    /* Removing a single handle and creating a new one directly reuses it if
       there's just one in the free list */
    layer.remove(third2);
    DataHandle third3 = layer.create();
    CORRADE_COMPARE(third3, dataHandle(layer.handle(), 2, 3));
    CORRADE_VERIFY(!layer.isHandleValid(third));
    CORRADE_VERIFY(!layer.isHandleValid(third2));
    CORRADE_VERIFY(layer.isHandleValid(third3));
    CORRADE_COMPARE(layer.capacity(), 4);
    CORRADE_COMPARE(layer.usedCount(), 4);
    CORRADE_COMPARE(layer.node(third3), NodeHandle::Null);

    /* Allocating a new handle with the free list empty will grow it */
    DataHandle fifth = layer.create();
    CORRADE_COMPARE(fifth, dataHandle(layer.handle(), 4, 1));
    CORRADE_VERIFY(layer.isHandleValid(fifth));
    CORRADE_COMPARE(layer.capacity(), 5);
    CORRADE_COMPARE(layer.usedCount(), 5);
    CORRADE_COMPARE(layer.node(fifth), NodeHandle::Null);

    /* The generation counter view should reflect the number of how much was
       given ID recycled */
    CORRADE_COMPARE_AS(layer.generations(), Containers::arrayView<UnsignedShort>({
        2,
        1,
        3,
        2,
        1
    }), TestSuite::Compare::Container);
}

void AbstractLayerTest::createRemoveHandleDisable() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;
        using AbstractLayer::remove;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0xab, 0x12)};

    DataHandle first = layer.create();
    CORRADE_COMPARE(first, dataHandle(layer.handle(), 0, 1));

    for(std::size_t i = 0; i != (1 << Implementation::LayerDataHandleGenerationBits) - 1; ++i) {
        DataHandle second = layer.create();
        CORRADE_COMPARE(second, dataHandle(layer.handle(), 1, 1 + i));
        layer.remove(second);
    }

    /* The generation for the second slot is exhausted so the handle is not
       recycled */
    CORRADE_COMPARE(layer.capacity(), 2);
    CORRADE_COMPARE(layer.usedCount(), 2);

    /* It shouldn't think a handle from the second slot with generation 0 is
       valid */
    CORRADE_VERIFY(!layer.isHandleValid(dataHandle(layer.handle(), 1, 0)));

    /* There's nowhere to create a new handle from so the capacity is grown */
    DataHandle third = layer.create();
    CORRADE_COMPARE(third, dataHandle(layer.handle(), 2, 1));
    CORRADE_COMPARE(layer.capacity(), 3);
    CORRADE_COMPARE(layer.usedCount(), 3);
}

void AbstractLayerTest::createNoHandlesLeft() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0, 1)};

    for(std::size_t i = 0; i != 1 << Implementation::LayerDataHandleIdBits; ++i)
        layer.create();

    CORRADE_COMPARE(layer.capacity(), 1 << Implementation::LayerDataHandleIdBits);
    CORRADE_COMPARE(layer.usedCount(), 1 << Implementation::LayerDataHandleIdBits);

    std::ostringstream out;
    Error redirectError{&out};
    layer.create();
    /* Number is hardcoded in the expected message but not elsewhere in order
       to give a heads-up when modifying the handle ID bit count */
    CORRADE_COMPARE(out.str(),
        "Ui::AbstractLayer::create(): can only have at most 1048576 data\n");
}

void AbstractLayerTest::createAttached() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0xab, 0x12)};

    NodeHandle node = nodeHandle(9872, 0xbeb);

    /* Explicitly passing a null handle should work too, and causes only
       NeedsDataUpdate */
    DataHandle notAttached = layer.create(NodeHandle::Null);
    CORRADE_COMPARE(layer.node(notAttached), NodeHandle::Null);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Passing a non-null handle causes NeedsAttachmentUpdate and everything
       related to updating node-related state as well */
    DataHandle attached = layer.create(node);
    CORRADE_COMPARE(layer.node(attached), node);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate|LayerState::NeedsAttachmentUpdate|LayerState::NeedsNodeOffsetSizeUpdate|LayerState::NeedsNodeEnabledUpdate);

    /* The attachment should be reflected in the view as well */
    CORRADE_COMPARE_AS(layer.nodes(), Containers::arrayView({
        NodeHandle::Null,
        node
    }), TestSuite::Compare::Container);
}

void AbstractLayerTest::removeInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;
        using AbstractLayer::remove;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0, 1)};

    DataHandle handle = layer.create();

    std::ostringstream out;
    Error redirectError{&out};
    layer.remove(DataHandle::Null);
    /* Valid layer, invalid data */
    layer.remove(dataHandle(layer.handle(), LayerDataHandle(0x123abcde)));
    /* Invalid layer, valid data */
    layer.remove(dataHandle(LayerHandle::Null, dataHandleData(handle)));
    /* LayerDataHandle directly */
    layer.remove(LayerDataHandle(0x123abcde));
    CORRADE_COMPARE_AS(out.str(),
        "Ui::AbstractLayer::remove(): invalid handle Ui::DataHandle::Null\n"
        "Ui::AbstractLayer::remove(): invalid handle Ui::DataHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Ui::AbstractLayer::remove(): invalid handle Ui::DataHandle(Null, {0x0, 0x1})\n"
        "Ui::AbstractLayer::remove(): invalid handle Ui::LayerDataHandle(0xabcde, 0x123)\n",
        TestSuite::Compare::String);
}

void AbstractLayerTest::attach() {
    /* This *does not* test the LayerState::NeedsCompositeOffsetSizeUpdate
       flag, that's verified in state() more than enough. This case verifies
       actual node attaching and attachment querying. */

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0xab, 0x12)};

    DataHandle first = layer.create();
    DataHandle second = layer.create();
    CORRADE_COMPARE(layer.node(first), NodeHandle::Null);
    CORRADE_COMPARE(layer.node(second), NodeHandle::Null);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

    /* Clear the state flags */
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    NodeHandle nodeFirst = nodeHandle(2865, 0xcec);
    NodeHandle nodeSecond = nodeHandle(9872, 0xbeb);
    NodeHandle nodeThird = nodeHandle(12, 0x888);

    /* Attaching to a non-null node sets all state related to nodes as well */
    layer.attach(first, nodeSecond);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsNodeOffsetSizeUpdate|LayerState::NeedsNodeEnabledUpdate|LayerState::NeedsAttachmentUpdate);
    CORRADE_COMPARE(layer.node(first), nodeSecond);

    /* The attachment should be reflected in the view as well */
    CORRADE_COMPARE_AS(layer.nodes(), Containers::arrayView({
        nodeSecond,
        NodeHandle::Null
    }), TestSuite::Compare::Container);

    /* Clear the state flags */
    layer.update(LayerState::NeedsNodeOffsetSizeUpdate|LayerState::NeedsNodeEnabledUpdate|LayerState::NeedsAttachmentUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Calling with the layer-specific handles should work too */
    layer.attach(dataHandleData(second), nodeFirst);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsNodeOffsetSizeUpdate|LayerState::NeedsNodeEnabledUpdate|LayerState::NeedsAttachmentUpdate);
    CORRADE_COMPARE(layer.node(dataHandleData(second)), nodeFirst);

    /* Clear the state flags */
    layer.update(LayerState::NeedsNodeOffsetSizeUpdate|LayerState::NeedsNodeEnabledUpdate|LayerState::NeedsAttachmentUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Attaching to a new node should overwrite the previous */
    layer.attach(first, nodeThird);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsNodeOffsetSizeUpdate|LayerState::NeedsNodeEnabledUpdate|LayerState::NeedsAttachmentUpdate);
    CORRADE_COMPARE(layer.node(first), nodeThird);

    /* Clear the state flags */
    layer.update(LayerState::NeedsNodeOffsetSizeUpdate|LayerState::NeedsNodeEnabledUpdate|LayerState::NeedsAttachmentUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Attaching two data to the same node should work too */
    layer.attach(second, nodeThird);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsNodeOffsetSizeUpdate|LayerState::NeedsNodeEnabledUpdate|LayerState::NeedsAttachmentUpdate);
    CORRADE_COMPARE(layer.node(first), nodeThird);
    CORRADE_COMPARE(layer.node(second), nodeThird);

    /* Clear the state flags */
    layer.update(LayerState::NeedsNodeOffsetSizeUpdate|LayerState::NeedsNodeEnabledUpdate|LayerState::NeedsAttachmentUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Attaching data to the same node is a no-op, not setting any flags */
    layer.attach(second, nodeThird);
    CORRADE_COMPARE(layer.state(), LayerStates{});
    CORRADE_COMPARE(layer.node(first), nodeThird);
    CORRADE_COMPARE(layer.node(second), nodeThird);

    /* Detaching sets only the attachment update, not
       NeedsNodeOffsetSizeUpdate */
    layer.attach(first, NodeHandle::Null);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsAttachmentUpdate);
    CORRADE_COMPARE(layer.node(first), NodeHandle::Null);
    CORRADE_COMPARE(layer.node(second), nodeThird);

    /* Clear the state flags */
    layer.update(LayerState::NeedsAttachmentUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Detaching an already-detached data is a no-op again */
    layer.attach(first, NodeHandle::Null);
    CORRADE_COMPARE(layer.state(), LayerStates{});
    CORRADE_COMPARE(layer.node(first), NodeHandle::Null);
    CORRADE_COMPARE(layer.node(second), nodeThird);

    /* The cleared attachment should be reflected in the view as well */
    CORRADE_COMPARE_AS(layer.nodes(), Containers::arrayView({
        NodeHandle::Null,
        nodeThird
    }), TestSuite::Compare::Container);
}

void AbstractLayerTest::attachInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0xab, 0x12)};

    DataHandle handle = layer.create();

    std::ostringstream out;
    Error redirectError{&out};
    layer.attach(DataHandle::Null, nodeHandle(2865, 0xcec));
    layer.node(DataHandle::Null);
    /* Valid layer, invalid data */
    layer.attach(dataHandle(layer.handle(), LayerDataHandle(0x123abcde)), nodeHandle(2865, 0xcec));
    layer.node(dataHandle(layer.handle(), LayerDataHandle(0x123abcde)));
    /* Invalid layer, valid data */
    layer.attach(dataHandle(LayerHandle::Null, dataHandleData(handle)), nodeHandle(2865, 0xcec));
    layer.node(dataHandle(LayerHandle::Null, dataHandleData(handle)));
    /* LayerDataHandle directly */
    layer.attach(LayerDataHandle(0x123abcde), nodeHandle(2865, 0xcec));
    layer.node(LayerDataHandle(0x123abcde));
    CORRADE_COMPARE_AS(out.str(),
        "Ui::AbstractLayer::attach(): invalid handle Ui::DataHandle::Null\n"
        "Ui::AbstractLayer::node(): invalid handle Ui::DataHandle::Null\n"
        "Ui::AbstractLayer::attach(): invalid handle Ui::DataHandle({0xab, 0x12}, {0xabcde, 0x123})\n"
        "Ui::AbstractLayer::node(): invalid handle Ui::DataHandle({0xab, 0x12}, {0xabcde, 0x123})\n"
        "Ui::AbstractLayer::attach(): invalid handle Ui::DataHandle(Null, {0x0, 0x1})\n"
        "Ui::AbstractLayer::node(): invalid handle Ui::DataHandle(Null, {0x0, 0x1})\n"
        "Ui::AbstractLayer::attach(): invalid handle Ui::LayerDataHandle(0xabcde, 0x123)\n"
        "Ui::AbstractLayer::node(): invalid handle Ui::LayerDataHandle(0xabcde, 0x123)\n",
        TestSuite::Compare::String);
}

void AbstractLayerTest::setSize() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Draw;
        }

        void doSetSize(const Vector2& size, const Vector2i& framebufferSize) override {
            ++called;
            CORRADE_COMPARE(size, (Vector2{1.0f, 2.0f}));
            CORRADE_COMPARE(framebufferSize, (Vector2i{3, 4}));
        }

        Int called = 0;
    } layer{layerHandle(0, 1)};

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    layer.setSize({1.0f, 2.0f}, {3, 4});
    CORRADE_COMPARE(layer.called, 1);
}

void AbstractLayerTest::setSizeZero() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Draw;
        }
    } layer{layerHandle(0, 1)};

    std::ostringstream out;
    Error redirectError{&out};
    layer.setSize({0.0f, 1.0f}, {2, 3});
    layer.setSize({1.0f, 0.0f}, {2, 3});
    layer.setSize({1.0f, 2.0f}, {0, 3});
    layer.setSize({1.0f, 2.0f}, {3, 0});
    CORRADE_COMPARE_AS(out.str(),
        "Ui::AbstractLayer::setSize(): expected non-zero sizes, got Vector(0, 1) and Vector(2, 3)\n"
        "Ui::AbstractLayer::setSize(): expected non-zero sizes, got Vector(1, 0) and Vector(2, 3)\n"
        "Ui::AbstractLayer::setSize(): expected non-zero sizes, got Vector(1, 2) and Vector(0, 3)\n"
        "Ui::AbstractLayer::setSize(): expected non-zero sizes, got Vector(1, 2) and Vector(3, 0)\n",
        TestSuite::Compare::String);
}

void AbstractLayerTest::setSizeNotSupported() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0, 1)};

    std::ostringstream out;
    Error redirectError{&out};
    layer.setSize({}, {});
    CORRADE_COMPARE(out.str(),
        "Ui::AbstractLayer::setSize(): Ui::LayerFeature::Draw not supported\n");
}

void AbstractLayerTest::setSizeNotImplemented() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Draw;
        }
    } layer{layerHandle(0, 1)};

    layer.setSize({1.0f, 2.0f}, {3, 4});

    /* Shouldn't crash or anything */
    CORRADE_VERIFY(true);
}

void AbstractLayerTest::assignDataAnimator() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::assignAnimator;

        LayerFeatures doFeatures() const override {
            return LayerFeature::AnimateData;
        }
    } layer{layerHandle(0xab, 0x12)};

    struct: AbstractDataAnimator {
        using AbstractDataAnimator::AbstractDataAnimator;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::DataAttachment;
        }
    } animator{animatorHandle(0xcd, 0x34)};
    CORRADE_COMPARE(animator.layer(), LayerHandle::Null);

    layer.assignAnimator(animator);
    CORRADE_COMPARE(animator.layer(), layer.handle());
}

void AbstractLayerTest::assignStyleAnimator() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::assignAnimator;

        LayerFeatures doFeatures() const override {
            return LayerFeature::AnimateStyles;
        }
    } layer{layerHandle(0xab, 0x12)};

    struct: AbstractStyleAnimator {
        using AbstractStyleAnimator::AbstractStyleAnimator;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::DataAttachment;
        }
    } animator{animatorHandle(0xcd, 0x34)};
    CORRADE_COMPARE(animator.layer(), LayerHandle::Null);

    layer.assignAnimator(animator);
    CORRADE_COMPARE(animator.layer(), layer.handle());
}

void AbstractLayerTest::assignDataAnimatorNotSupported() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::assignAnimator;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0, 1)};

    struct: AbstractDataAnimator {
        using AbstractDataAnimator::AbstractDataAnimator;

        AnimatorFeatures doFeatures() const override { return {}; }
    } animator{animatorHandle(0, 1)};

    std::ostringstream out;
    Error redirectError{&out};
    layer.assignAnimator(animator);
    CORRADE_COMPARE(out.str(), "Ui::AbstractLayer::assignAnimator(): data animation not supported\n");
}

void AbstractLayerTest::assignStyleAnimatorNotSupported() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::assignAnimator;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0, 1)};

    struct: AbstractStyleAnimator {
        using AbstractStyleAnimator::AbstractStyleAnimator;

        AnimatorFeatures doFeatures() const override { return {}; }
    } animator{animatorHandle(0, 1)};

    std::ostringstream out;
    Error redirectError{&out};
    layer.assignAnimator(animator);
    CORRADE_COMPARE(out.str(), "Ui::AbstractLayer::assignAnimator(): style animation not supported\n");
}

void AbstractLayerTest::assignDataAnimatorInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::assignAnimator;

        LayerFeatures doFeatures() const override {
            return LayerFeature::AnimateData;
        }
    } layer{layerHandle(0xab, 0x12)};

    struct Animator: AbstractDataAnimator {
        explicit Animator(AnimatorHandle handle, AnimatorFeatures features): AbstractDataAnimator{handle}, _features{features} {}

        AnimatorFeatures doFeatures() const override { return _features; }

        private:
            AnimatorFeatures _features;
    } animatorNoDataAttachment{animatorHandle(0, 1), {}},
      animatorAlreadyAssociated{animatorHandle(1, 2), AnimatorFeature::DataAttachment};

    layer.assignAnimator(animatorAlreadyAssociated);

    std::ostringstream out;
    Error redirectError{&out};
    layer.assignAnimator(animatorNoDataAttachment);
    layer.assignAnimator(animatorAlreadyAssociated);
    CORRADE_COMPARE_AS(out.str(),
        "Ui::AbstractLayer::assignAnimator(): data attachment not supported by the animator\n"
        "Ui::AbstractLayer::assignAnimator(): animator already assigned to Ui::LayerHandle(0xab, 0x12)\n",
        TestSuite::Compare::String);
}

void AbstractLayerTest::assignStyleAnimatorInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::assignAnimator;

        LayerFeatures doFeatures() const override {
            return LayerFeature::AnimateStyles;
        }
    } layer{layerHandle(0xab, 0x12)};

    struct Animator: AbstractStyleAnimator {
        explicit Animator(AnimatorHandle handle, AnimatorFeatures features): AbstractStyleAnimator{handle}, _features{features} {}

        AnimatorFeatures doFeatures() const override { return _features; }

        private:
            AnimatorFeatures _features;
    } animatorNoDataAttachment{animatorHandle(0, 1), {}},
      animatorAlreadyAssociated{animatorHandle(1, 2), AnimatorFeature::DataAttachment};

    layer.assignAnimator(animatorAlreadyAssociated);

    std::ostringstream out;
    Error redirectError{&out};
    layer.assignAnimator(animatorNoDataAttachment);
    layer.assignAnimator(animatorAlreadyAssociated);
    CORRADE_COMPARE_AS(out.str(),
        "Ui::AbstractLayer::assignAnimator(): data attachment not supported by the animator\n"
        "Ui::AbstractLayer::assignAnimator(): animator already assigned to Ui::LayerHandle(0xab, 0x12)\n",
        TestSuite::Compare::String);
}

void AbstractLayerTest::cleanNodes() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;
        using AbstractLayer::remove;

        LayerFeatures doFeatures() const override { return {}; }

        void doClean(Containers::BitArrayView dataIdsToRemove) override {
            ++called;
            CORRADE_COMPARE_AS(dataIdsToRemove, Containers::stridedArrayView({
                true, false, false, true, false, true, false
            }).sliceBit(0), TestSuite::Compare::Container);
        }

        Int called = 0;
    } layer{layerHandle(0, 1)};

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    NodeHandle nodeFirst = nodeHandle(0, 0xcec);
    NodeHandle nodeSecond = nodeHandle(1, 0xded);
    NodeHandle nodeFourth = nodeHandle(3, 0xaba);
    NodeHandle nodeEighth = nodeHandle(7, 0xfef);

    /* Create seven data to match the seven bits. Attach them to random
       handles, leave one unassigned, attach two data to one node. */
    DataHandle first = layer.create(nodeEighth);
    DataHandle second = layer.create();
    DataHandle third = layer.create(nodeSecond);
    DataHandle fourth = layer.create(nodeFirst);
    DataHandle fifth = layer.create(nodeFourth);
    DataHandle sixth = layer.create(nodeFirst);
    DataHandle seventh = layer.create(nodeFourth);

    /* Remove two of them */
    layer.remove(third);
    layer.remove(seventh);

    /* Call cleanNodes() with updated generation counters */
    layer.cleanNodes(Containers::arrayView({
        /* First node generation gets different, affecting fourth and sixth
           data */
        UnsignedShort(nodeHandleGeneration(nodeFirst) + 1),
        /* Second node generation gets different but since the third data is
           already removed it doesn't affect anything */
        UnsignedShort(nodeHandleGeneration(nodeSecond) - 1),
        /* Third node has no attachments so it can be arbitrary */
        UnsignedShort{0xbeb},
        /* Fourth node stays the same generation so the fifth data stay.
           Seventh data are already removed so they aren't set for deletion
           either. */
        UnsignedShort(nodeHandleGeneration(nodeFourth)),
        /* Fifth, sixth, seventh nodes have no attachments so they can be
           arbitrary again */
        UnsignedShort{0xaca},
        UnsignedShort{0x808},
        UnsignedShort{0xefe},
        /* Eighth node is now a zero generation, i.e. disabled, which should
           trigger removal of first data */
        UnsignedShort{},
    }));
    CORRADE_COMPARE(layer.called, 1);

    /* Only the second and fifth data should stay afterwards */
    CORRADE_VERIFY(!layer.isHandleValid(first));
    CORRADE_VERIFY(layer.isHandleValid(second));
    CORRADE_VERIFY(!layer.isHandleValid(third));
    CORRADE_VERIFY(!layer.isHandleValid(fourth));
    CORRADE_VERIFY(layer.isHandleValid(fifth));
    CORRADE_VERIFY(!layer.isHandleValid(sixth));
    CORRADE_VERIFY(!layer.isHandleValid(seventh));
}

void AbstractLayerTest::cleanNodesEmpty() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }

        void doClean(Containers::BitArrayView) override {
            ++called;
        }

        Int called = 0;
    } layer{layerHandle(0, 1)};

    /* It should call the implementation even with empty contents */
    layer.cleanNodes({});
    CORRADE_COMPARE(layer.called, 1);
}

void AbstractLayerTest::cleanNodesNotImplemented() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0, 1)};

    layer.cleanNodes({});

    /* Shouldn't crash or anything */
    CORRADE_VERIFY(true);
}

void AbstractLayerTest::cleanDataAnimators() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;
        using AbstractLayer::remove;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0, 1)};

    struct: AbstractGenericAnimator {
        using AbstractGenericAnimator::AbstractGenericAnimator;
        using AbstractGenericAnimator::setLayer;
        using AbstractGenericAnimator::create;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::DataAttachment;
        }
        void doClean(Containers::BitArrayView animationIdsToRemove) override {
            ++called;
            CORRADE_COMPARE_AS(animationIdsToRemove, Containers::stridedArrayView({
                /* First and third is attached to removed data, fourth is not
                   attached to anything. fifth was attached to an invalid
                   handle in the first place */
                true, false, true, false, true
            }).sliceBit(0), TestSuite::Compare::Container);
        }
        void doAdvance(Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}

        Int called = 0;
    } animator1{animatorHandle(1, 1)};
    animator1.setLayer(layer);

    struct: AbstractGenericAnimator {
        using AbstractGenericAnimator::AbstractGenericAnimator;
        using AbstractGenericAnimator::setLayer;
        using AbstractGenericAnimator::create;
        using AbstractGenericAnimator::remove;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::DataAttachment;
        }
        void doClean(Containers::BitArrayView animationIdsToRemove) override {
            ++called;
            CORRADE_COMPARE_AS(animationIdsToRemove, Containers::stridedArrayView({
                /* Second is attached to removed data, the third is already
                   removed at this point; fourth is recreated with a different
                   handle generation */
                false, true, false, true
            }).sliceBit(0), TestSuite::Compare::Container);
        }
        void doAdvance(Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}

        Int called = 0;
    } animator2{animatorHandle(1, 1)};
    animator2.setLayer(layer);

    /* Seventh data unused, sixth unused and removed. Fifth data attached to by
       both animators and removed. Fourth data is removed and then recreated
       with the same handle ID, which should cause the now-stale assignment to
       get removed as well. */
    DataHandle first = layer.create();
    DataHandle second = layer.create();
    DataHandle third = layer.create();
    DataHandle fourth = layer.create();
    DataHandle fifth = layer.create();
    DataHandle sixth = layer.create();
    /*DataHandle seventh =*/ layer.create();
    layer.remove(fourth);
    layer.remove(fifth);
    layer.remove(sixth);
    DataHandle fourth2 = layer.create();
    CORRADE_COMPARE(dataHandleId(fourth2), dataHandleId(fourth));

    /* Two animations attached to the same data (which get removed), one
       animation not attached to anything, one animation attached to an already
       invalid handle */
    AnimationHandle animation11 = animator1.create(0_nsec, 1_nsec, fifth);
    AnimationHandle animation12 = animator1.create(0_nsec, 1_nsec, third);
    AnimationHandle animation13 = animator1.create(0_nsec, 1_nsec, fifth);
    AnimationHandle animation14 = animator1.create(0_nsec, 1_nsec);
    /* The ID however has to be in range, otherwise it'll assert on an OOB
       access */
    /** @todo this might be possible to hit in practice (accidentally attaching
        to a LayerDataHandle from a different layer that has more items), have
        some graceful handling? */
    AnimationHandle animation15 = animator1.create(0_nsec, 1_nsec, layerDataHandle(5, 0x12));

    /* One animation attached to the same data as in the first animator, one
       animation attached but then removed */
    AnimationHandle animation21 = animator2.create(0_nsec, 1_nsec, second);
    AnimationHandle animation22 = animator2.create(0_nsec, 1_nsec, fifth);
    AnimationHandle animation23 = animator2.create(0_nsec, 1_nsec, first);
    AnimationHandle animation24 = animator2.create(0_nsec, 1_nsec, fourth);
    animator2.remove(animation23);

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    layer.cleanData({animator1, animator2});
    CORRADE_VERIFY(!animator1.isHandleValid(animation11));
    CORRADE_VERIFY(animator1.isHandleValid(animation12));
    CORRADE_VERIFY(!animator1.isHandleValid(animation13));
    CORRADE_VERIFY(animator1.isHandleValid(animation14));
    CORRADE_VERIFY(!animator1.isHandleValid(animation15));
    CORRADE_VERIFY(animator2.isHandleValid(animation21));
    CORRADE_VERIFY(!animator2.isHandleValid(animation22));
    CORRADE_VERIFY(!animator2.isHandleValid(animation23));
    CORRADE_VERIFY(!animator2.isHandleValid(animation24));
    CORRADE_COMPARE(animator1.called, 1);
    CORRADE_COMPARE(animator2.called, 1);
}

void AbstractLayerTest::cleanDataAnimatorsEmpty() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0, 1)};

    /* It shouldn't crash or anything */
    layer.cleanData({});
    CORRADE_VERIFY(true);
}

void AbstractLayerTest::cleanDataAnimatorsInvalidFeatures() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0, 1)};

    struct: AbstractGenericAnimator {
        using AbstractGenericAnimator::AbstractGenericAnimator;
        using AbstractGenericAnimator::setLayer;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::DataAttachment;
        }
        void doAdvance(Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}
    } animator1{animatorHandle(0, 1)};
    animator1.setLayer(layer);

    struct: AbstractAnimator {
        using AbstractAnimator::AbstractAnimator;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::NodeAttachment;
        }
    } animator2{animatorHandle(1, 3)};

    std::ostringstream out;
    Error redirectError{&out};
    layer.cleanData({animator1, animator2});
    CORRADE_COMPARE(out.str(), "Ui::AbstractLayer::cleanData(): data attachment not supported by an animator\n");
}

void AbstractLayerTest::cleanDataAnimatorsLayerNotSet() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0, 1)};

    struct: AbstractGenericAnimator {
        using AbstractGenericAnimator::AbstractGenericAnimator;
        using AbstractGenericAnimator::setLayer;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::DataAttachment;
        }
        void doAdvance(Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}
    } animator1{animatorHandle(0, 1)};
    animator1.setLayer(layer);

    struct: AbstractAnimator {
        using AbstractAnimator::AbstractAnimator;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::DataAttachment;
        }
    } animator2{animatorHandle(1, 3)};
    /* Second animator layer not set */

    std::ostringstream out;
    Error redirectError{&out};
    layer.cleanData({animator1, animator2});
    CORRADE_COMPARE(out.str(), "Ui::AbstractLayer::cleanData(): animator has no layer set for data attachment\n");
}

void AbstractLayerTest::cleanDataAnimatorsInvalidLayer() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    } layer1{layerHandle(0xab, 0x12)}, layer2{layerHandle(0xcd, 0x34)};

    struct: AbstractGenericAnimator {
        using AbstractGenericAnimator::AbstractGenericAnimator;
        using AbstractGenericAnimator::setLayer;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::DataAttachment;
        }
        void doAdvance(Containers::BitArrayView, const Containers::StridedArrayView1D<const Float>&) override {}
    } animator1{animatorHandle(0, 1)}, animator2{animatorHandle(1, 3)};
    animator1.setLayer(layer1);
    animator2.setLayer(layer2);

    std::ostringstream out;
    Error redirectError{&out};
    layer1.cleanData({animator1, animator2});
    CORRADE_COMPARE(out.str(), "Ui::AbstractLayer::cleanData(): expected an animator assigned to Ui::LayerHandle(0xab, 0x12) but got Ui::LayerHandle(0xcd, 0x34)\n");
}

void AbstractLayerTest::advanceDataAnimations() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::assignAnimator;

        LayerFeatures doFeatures() const override {
            return LayerFeature::AnimateData;
        }

        void doAdvanceAnimations(Nanoseconds time, Containers::MutableBitArrayView activeStorage, const Containers::StridedArrayView1D<Float>& factorStorage, Containers::MutableBitArrayView removeStorage, const Containers::Iterable<AbstractDataAnimator>& animators) override {
            CORRADE_COMPARE(time, 476_nsec);
            CORRADE_COMPARE(activeStorage.size(), 17);
            CORRADE_COMPARE(factorStorage.size(), 17);
            CORRADE_COMPARE(removeStorage.size(), 17);
            CORRADE_COMPARE(animators.size(), 2);
            CORRADE_COMPARE(animators[0].handle(), animatorHandle(0xab, 0x12));
            CORRADE_COMPARE(animators[1].handle(), animatorHandle(0xcd, 0x34));
            ++advanceCalled;
        }

        Int advanceCalled = 0;
    } layer{layerHandle(0, 1)};

    struct: AbstractDataAnimator {
        using AbstractDataAnimator::AbstractDataAnimator;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::DataAttachment;
        }
    } animator1{animatorHandle(0xab, 0x12)},
      animator2{animatorHandle(0xcd, 0x34)};
    layer.assignAnimator(animator1);
    layer.assignAnimator(animator2);

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    UnsignedInt maskData[1];
    Containers::MutableBitArrayView maskStorage{maskData, 0, 17};
    Float factorStorage[17];
    layer.advanceAnimations(476_nsec, maskStorage, factorStorage, maskStorage, {animator1, animator2});
    CORRADE_COMPARE(layer.advanceCalled, 1);
}

void AbstractLayerTest::advanceStyleAnimations() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::assignAnimator;

        LayerFeatures doFeatures() const override {
            return LayerFeature::AnimateStyles;
        }

        void doAdvanceAnimations(Nanoseconds time, Containers::MutableBitArrayView activeStorage, const Containers::StridedArrayView1D<Float>& factorStorage, Containers::MutableBitArrayView removeStorage, const Containers::Iterable<AbstractStyleAnimator>& animators) override {
            CORRADE_COMPARE(time, 476_nsec);
            CORRADE_COMPARE(activeStorage.size(), 17);
            CORRADE_COMPARE(factorStorage.size(), 17);
            CORRADE_COMPARE(removeStorage.size(), 17);
            CORRADE_COMPARE(animators.size(), 2);
            CORRADE_COMPARE(animators[0].handle(), animatorHandle(0xab, 0x12));
            CORRADE_COMPARE(animators[1].handle(), animatorHandle(0xcd, 0x34));
            ++advanceCalled;
        }

        Int advanceCalled = 0;
    } layer{layerHandle(0, 1)};

    struct: AbstractStyleAnimator {
        using AbstractStyleAnimator::AbstractStyleAnimator;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::DataAttachment;
        }
    } animator1{animatorHandle(0xab, 0x12)},
      animator2{animatorHandle(0xcd, 0x34)};
    layer.assignAnimator(animator1);
    layer.assignAnimator(animator2);

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    UnsignedInt maskData[1];
    Containers::MutableBitArrayView maskStorage{maskData, 0, 17};
    Float factorStorage[17];
    layer.advanceAnimations(476_nsec, maskStorage, factorStorage, maskStorage, {animator1, animator2});
    CORRADE_COMPARE(layer.advanceCalled, 1);
}

void AbstractLayerTest::advanceDataAnimationsEmpty() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override {
            return LayerFeature::AnimateData;
        }
        void doAdvanceAnimations(Nanoseconds, Containers::MutableBitArrayView, const Containers::StridedArrayView1D<Float>&, Containers::MutableBitArrayView, const Containers::Iterable<AbstractDataAnimator>&) override {}
    } layer{layerHandle(0, 1)};

    /* It shouldn't crash or anything */
    layer.advanceAnimations(0_nsec, {}, {}, {}, Containers::Iterable<AbstractDataAnimator>{});
    CORRADE_VERIFY(true);
}

void AbstractLayerTest::advanceStyleAnimationsEmpty() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override {
            return LayerFeature::AnimateStyles;
        }
        void doAdvanceAnimations(Nanoseconds, Containers::MutableBitArrayView, const Containers::StridedArrayView1D<Float>&, Containers::MutableBitArrayView, const Containers::Iterable<AbstractStyleAnimator>&) override {}
    } layer{layerHandle(0, 1)};

    /* It shouldn't crash or anything */
    layer.advanceAnimations(0_nsec, {}, {}, {}, Containers::Iterable<AbstractStyleAnimator>{});
    CORRADE_VERIFY(true);
}

void AbstractLayerTest::advanceDataAnimationsNotSupported() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override {
            /* Not AnimateData */
            return LayerFeature::AnimateStyles;
        }
    } layer{layerHandle(0, 1)};

    std::ostringstream out;
    Error redirectError{&out};
    layer.advanceAnimations(0_nsec, {}, {}, {}, Containers::Iterable<AbstractDataAnimator>{});
    CORRADE_COMPARE(out.str(), "Ui::AbstractLayer::advanceAnimations(): data animation not supported\n");
}

void AbstractLayerTest::advanceStyleAnimationsNotSupported() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override {
            /* Not AnimateStyles */
            return LayerFeature::AnimateData;
        }
    } layer{layerHandle(0, 1)};

    std::ostringstream out;
    Error redirectError{&out};
    layer.advanceAnimations(0_nsec, {}, {}, {}, Containers::Iterable<AbstractStyleAnimator>{});
    CORRADE_COMPARE(out.str(), "Ui::AbstractLayer::advanceAnimations(): style animation not supported\n");
}

void AbstractLayerTest::advanceDataAnimationsNotImplemented() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override {
            return LayerFeature::AnimateData;
        }
        void doAdvanceAnimations(Nanoseconds, Containers::MutableBitArrayView, const Containers::StridedArrayView1D<Float>&, Containers::MutableBitArrayView, const Containers::Iterable<AbstractStyleAnimator>&) override {
            CORRADE_FAIL("This shouldn't be called");
        }
    } layer{layerHandle(0, 1)};

    std::ostringstream out;
    Error redirectError{&out};
    layer.advanceAnimations(0_nsec, {}, {}, {}, Containers::Iterable<AbstractDataAnimator>{});
    CORRADE_COMPARE(out.str(), "Ui::AbstractLayer::advanceAnimations(): data animation advertised but not implemented\n");
}

void AbstractLayerTest::advanceStyleAnimationsNotImplemented() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override {
            return LayerFeature::AnimateStyles;
        }
        void doAdvanceAnimations(Nanoseconds, Containers::MutableBitArrayView, const Containers::StridedArrayView1D<Float>&, Containers::MutableBitArrayView, const Containers::Iterable<AbstractDataAnimator>&) override {
            CORRADE_FAIL("This shouldn't be called");
        }
    } layer{layerHandle(0, 1)};

    std::ostringstream out;
    Error redirectError{&out};
    layer.advanceAnimations(0_nsec, {}, {}, {}, Containers::Iterable<AbstractStyleAnimator>{});
    CORRADE_COMPARE(out.str(), "Ui::AbstractLayer::advanceAnimations(): style animation advertised but not implemented\n");
}

void AbstractLayerTest::advanceDataAnimationsInvalidFeatures() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::assignAnimator;

        LayerFeatures doFeatures() const override {
            return LayerFeature::AnimateData;
        }
    } layer{layerHandle(0, 1)};

    struct Animator: AbstractDataAnimator {
        explicit Animator(AnimatorHandle handle, AnimatorFeatures features): AbstractDataAnimator{handle}, _features{features} {}

        AnimatorFeatures doFeatures() const override { return _features; }

        private:
            AnimatorFeatures _features;
    } animator1{animatorHandle(0, 1), AnimatorFeature::DataAttachment},
      animator2{animatorHandle(1, 3), AnimatorFeature::NodeAttachment};
    layer.assignAnimator(animator1);

    std::ostringstream out;
    Error redirectError{&out};
    layer.advanceAnimations(0_nsec, {}, {}, {}, {animator1, animator2});
    CORRADE_COMPARE(out.str(), "Ui::AbstractLayer::advanceAnimations(): data attachment not supported by an animator\n");
}

void AbstractLayerTest::advanceStyleAnimationsInvalidFeatures() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::assignAnimator;

        LayerFeatures doFeatures() const override {
            return LayerFeature::AnimateStyles;
        }
    } layer{layerHandle(0, 1)};

    struct Animator: AbstractStyleAnimator {
        explicit Animator(AnimatorHandle handle, AnimatorFeatures features): AbstractStyleAnimator{handle}, _features{features} {}

        AnimatorFeatures doFeatures() const override { return _features; }

        private:
            AnimatorFeatures _features;
    } animator1{animatorHandle(0, 1), AnimatorFeature::DataAttachment},
      animator2{animatorHandle(1, 3), AnimatorFeature::NodeAttachment};
    layer.assignAnimator(animator1);

    std::ostringstream out;
    Error redirectError{&out};
    layer.advanceAnimations(0_nsec, {}, {}, {}, {animator1, animator2});
    CORRADE_COMPARE(out.str(), "Ui::AbstractLayer::advanceAnimations(): data attachment not supported by an animator\n");
}

void AbstractLayerTest::advanceDataAnimationsLayerNotSet() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::assignAnimator;

        LayerFeatures doFeatures() const override {
            return LayerFeature::AnimateData;
        }
    } layer{layerHandle(0, 1)};

    struct: AbstractDataAnimator {
        using AbstractDataAnimator::AbstractDataAnimator;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::DataAttachment;
        }
    } animator1{animatorHandle(0, 1)}, animator2{animatorHandle(1, 3)};
   layer.assignAnimator(animator1);
    /* Second animator layer not set */

    std::ostringstream out;
    Error redirectError{&out};
    layer.advanceAnimations(0_nsec, {}, {}, {}, {animator1, animator2});
    CORRADE_COMPARE(out.str(), "Ui::AbstractLayer::advanceAnimations(): animator has no layer set for data attachment\n");
}

void AbstractLayerTest::advanceStyleAnimationsLayerNotSet() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::assignAnimator;

        LayerFeatures doFeatures() const override {
            return LayerFeature::AnimateStyles;
        }
    } layer{layerHandle(0, 1)};

    struct: AbstractStyleAnimator {
        using AbstractStyleAnimator::AbstractStyleAnimator;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::DataAttachment;
        }
    } animator1{animatorHandle(0, 1)}, animator2{animatorHandle(1, 3)};
   layer.assignAnimator(animator1);
    /* Second animator layer not set */

    std::ostringstream out;
    Error redirectError{&out};
    layer.advanceAnimations(0_nsec, {}, {}, {}, {animator1, animator2});
    CORRADE_COMPARE(out.str(), "Ui::AbstractLayer::advanceAnimations(): animator has no layer set for data attachment\n");
}

void AbstractLayerTest::advanceDataAnimationsInvalidLayer() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::assignAnimator;

        LayerFeatures doFeatures() const override {
            return LayerFeature::AnimateData;
        }
    } layer1{layerHandle(0xab, 0x12)}, layer2{layerHandle(0xcd, 0x34)};

    struct: AbstractDataAnimator {
        using AbstractDataAnimator::AbstractDataAnimator;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::DataAttachment;
        }
    } animator1{animatorHandle(0, 1)}, animator2{animatorHandle(1, 3)};
    layer1.assignAnimator(animator1);
    layer2.assignAnimator(animator2);

    std::ostringstream out;
    Error redirectError{&out};
    layer1.advanceAnimations(0_nsec, {}, {}, {}, {animator1, animator2});
    CORRADE_COMPARE(out.str(), "Ui::AbstractLayer::advanceAnimations(): expected an animator assigned to Ui::LayerHandle(0xab, 0x12) but got Ui::LayerHandle(0xcd, 0x34)\n");
}

void AbstractLayerTest::advanceStyleAnimationsInvalidLayer() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::assignAnimator;

        LayerFeatures doFeatures() const override {
            return LayerFeature::AnimateStyles;
        }
    } layer1{layerHandle(0xab, 0x12)}, layer2{layerHandle(0xcd, 0x34)};

    struct: AbstractStyleAnimator {
        using AbstractStyleAnimator::AbstractStyleAnimator;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::DataAttachment;
        }
    } animator1{animatorHandle(0, 1)}, animator2{animatorHandle(1, 3)};
    layer1.assignAnimator(animator1);
    layer2.assignAnimator(animator2);

    std::ostringstream out;
    Error redirectError{&out};
    layer1.advanceAnimations(0_nsec, {}, {}, {}, {animator1, animator2});
    CORRADE_COMPARE(out.str(), "Ui::AbstractLayer::advanceAnimations(): expected an animator assigned to Ui::LayerHandle(0xab, 0x12) but got Ui::LayerHandle(0xcd, 0x34)\n");
}

void AbstractLayerTest::advanceDataAnimationsInvalidSize() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::assignAnimator;

        LayerFeatures doFeatures() const override {
            return LayerFeature::AnimateData;
        }
    } layer{layerHandle(0, 1)};

    struct: AbstractDataAnimator {
        using AbstractDataAnimator::AbstractDataAnimator;
        using AbstractDataAnimator::create;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::DataAttachment;
        }
    } animator1{animatorHandle(0, 1)}, animator2{animatorHandle(1, 3)}, animator3{animatorHandle(2, 2)};
    layer.assignAnimator(animator1);
    layer.assignAnimator(animator2);
    layer.assignAnimator(animator3);

    animator1.create(0_nsec, 1_nsec);
    animator1.create(0_nsec, 1_nsec);
    animator1.create(0_nsec, 1_nsec);

    animator2.create(0_nsec, 1_nsec);
    animator2.create(0_nsec, 1_nsec);
    animator2.create(0_nsec, 1_nsec);
    animator2.create(0_nsec, 1_nsec);
    animator2.create(0_nsec, 1_nsec);
    animator2.create(0_nsec, 1_nsec);

    animator3.create(0_nsec, 1_nsec);
    animator3.create(0_nsec, 1_nsec);

    UnsignedInt maskData[1];
    Containers::MutableBitArrayView maskStorageLow{maskData, 0, 5};
    Containers::MutableBitArrayView maskStorage{maskData, 0, 6};
    Containers::MutableBitArrayView maskStorageHigh{maskData, 0, 7};
    Float factorStorageLow[5];
    Float factorStorage[6];
    Float factorStorageHigh[7];

    std::ostringstream out;
    Error redirectError{&out};
    layer.advanceAnimations(0_nsec, maskStorageLow, factorStorageLow, maskStorageLow, {animator1, animator2, animator3});
    layer.advanceAnimations(0_nsec, maskStorage, factorStorage, maskStorageHigh, {animator1, animator2, animator3});
    layer.advanceAnimations(0_nsec, maskStorage, factorStorageHigh, maskStorage, {animator1, animator2, animator3});
    layer.advanceAnimations(0_nsec, maskStorageHigh, factorStorage, maskStorage, {animator1, animator2, animator3});
    CORRADE_COMPARE_AS(out.str(),
        "Ui::AbstractLayer::advanceAnimations(): expected activeStorage, factorStorage and removeStorage views to have the same size of at least 6 elements but got 5, 5 and 5\n"
        "Ui::AbstractLayer::advanceAnimations(): expected activeStorage, factorStorage and removeStorage views to have the same size of at least 6 elements but got 6, 6 and 7\n"
        "Ui::AbstractLayer::advanceAnimations(): expected activeStorage, factorStorage and removeStorage views to have the same size of at least 6 elements but got 6, 7 and 6\n"
        "Ui::AbstractLayer::advanceAnimations(): expected activeStorage, factorStorage and removeStorage views to have the same size of at least 6 elements but got 7, 6 and 6\n",
        TestSuite::Compare::String);
}

void AbstractLayerTest::advanceStyleAnimationsInvalidSize() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::assignAnimator;

        LayerFeatures doFeatures() const override {
            return LayerFeature::AnimateStyles;
        }
    } layer{layerHandle(0, 1)};

    struct: AbstractStyleAnimator {
        using AbstractStyleAnimator::AbstractStyleAnimator;
        using AbstractStyleAnimator::create;

        AnimatorFeatures doFeatures() const override {
            return AnimatorFeature::DataAttachment;
        }
    } animator1{animatorHandle(0, 1)}, animator2{animatorHandle(1, 3)}, animator3{animatorHandle(2, 2)};
    layer.assignAnimator(animator1);
    layer.assignAnimator(animator2);
    layer.assignAnimator(animator3);

    animator1.create(0_nsec, 1_nsec);
    animator1.create(0_nsec, 1_nsec);
    animator1.create(0_nsec, 1_nsec);

    animator2.create(0_nsec, 1_nsec);
    animator2.create(0_nsec, 1_nsec);
    animator2.create(0_nsec, 1_nsec);
    animator2.create(0_nsec, 1_nsec);
    animator2.create(0_nsec, 1_nsec);
    animator2.create(0_nsec, 1_nsec);

    animator3.create(0_nsec, 1_nsec);
    animator3.create(0_nsec, 1_nsec);

    UnsignedInt maskData[1];
    Containers::MutableBitArrayView maskStorageLow{maskData, 0, 5};
    Containers::MutableBitArrayView maskStorage{maskData, 0, 6};
    Containers::MutableBitArrayView maskStorageHigh{maskData, 0, 7};
    Float factorStorageLow[5];
    Float factorStorage[6];
    Float factorStorageHigh[7];

    std::ostringstream out;
    Error redirectError{&out};
    layer.advanceAnimations(0_nsec, maskStorageLow, factorStorageLow, maskStorageLow, {animator1, animator2, animator3});
    layer.advanceAnimations(0_nsec, maskStorage, factorStorage, maskStorageHigh, {animator1, animator2, animator3});
    layer.advanceAnimations(0_nsec, maskStorage, factorStorageHigh, maskStorage, {animator1, animator2, animator3});
    layer.advanceAnimations(0_nsec, maskStorageHigh, factorStorage, maskStorage, {animator1, animator2, animator3});
    CORRADE_COMPARE_AS(out.str(),
        "Ui::AbstractLayer::advanceAnimations(): expected activeStorage, factorStorage and removeStorage views to have the same size of at least 6 elements but got 5, 5 and 5\n"
        "Ui::AbstractLayer::advanceAnimations(): expected activeStorage, factorStorage and removeStorage views to have the same size of at least 6 elements but got 6, 6 and 7\n"
        "Ui::AbstractLayer::advanceAnimations(): expected activeStorage, factorStorage and removeStorage views to have the same size of at least 6 elements but got 6, 7 and 6\n"
        "Ui::AbstractLayer::advanceAnimations(): expected activeStorage, factorStorage and removeStorage views to have the same size of at least 6 elements but got 7, 6 and 6\n",
        TestSuite::Compare::String);
}

void AbstractLayerTest::update() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }

        void doUpdate(LayerStates state, const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, Containers::BitArrayView nodesEnabled, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes, const Containers::StridedArrayView1D<const Vector2>& compositeRectOffsets, const Containers::StridedArrayView1D<const Vector2>& compositeRectSizes) override {
            ++called;
            CORRADE_COMPARE(state, LayerState::NeedsNodeOffsetSizeUpdate|LayerState::NeedsCommonDataUpdate);
            CORRADE_COMPARE_AS(dataIds, Containers::arrayView({
                0xabcdeu,
                0x45678u
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(clipRectIds, Containers::arrayView({
                /* These should be small enough to index into clipRectOffsets
                   and clipRectSizes but nobody cares here */
                3u,
                16u,
                27u,
                2u
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(clipRectDataCounts, Containers::arrayView({
                /* The sum should be equal to dataIds.size(), yes, nobody cares
                   here */
                265u,
                1u,
                13u,
                7u
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(nodeOffsets, Containers::arrayView<Vector2>({
                {1.0f, 2.0f},
                {3.0f, 4.0f},
                {5.0f, 6.0f}
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(nodeSizes, Containers::arrayView<Vector2>({
                {0.1f, 0.2f},
                {0.3f, 0.4f},
                {0.5f, 0.6f}
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(nodesEnabled, Containers::stridedArrayView({
                true,
                false,
                true
            }).sliceBit(0), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(clipRectOffsets, Containers::arrayView<Vector2>({
                {6.5f, 7.5f},
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(clipRectSizes, Containers::arrayView<Vector2>({
                {8.5f, 9.5f},
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(compositeRectOffsets,
                Containers::arrayView<Vector2>({}),
                TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(compositeRectSizes,
                Containers::arrayView<Vector2>({}),
                TestSuite::Compare::Container);
        }

        Int called = 0;
    } layer{layerHandle(0, 1)};

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    UnsignedByte nodesEnabled[1]{0x5};

    layer.update(
        LayerState::NeedsNodeOffsetSizeUpdate|LayerState::NeedsCommonDataUpdate,
        Containers::arrayView({
            0xabcdeu,
            0x45678u,
        }),
        Containers::arrayView({
            3u,
            16u,
            27u,
            2u
        }),
        Containers::arrayView({
            265u,
            1u,
            13u,
            7u
        }),
        Containers::arrayView<Vector2>({
            {1.0f, 2.0f},
            {3.0f, 4.0f},
            {5.0f, 6.0f}
        }),
        Containers::arrayView<Vector2>({
            {0.1f, 0.2f},
            {0.3f, 0.4f},
            {0.5f, 0.6f}
        }),
        Containers::BitArrayView{nodesEnabled, 0, 3},
        Containers::arrayView<Vector2>({
            {6.5f, 7.5f},
        }),
        Containers::arrayView<Vector2>({
            {8.5f, 9.5f},
        }),
        {}, {}
    );
    CORRADE_COMPARE(layer.called, 1);
}

void AbstractLayerTest::updateComposite() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return LayerFeature::Composite; }

        void doUpdate(LayerStates state, const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, Containers::BitArrayView nodesEnabled, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes, const Containers::StridedArrayView1D<const Vector2>& compositeRectOffsets, const Containers::StridedArrayView1D<const Vector2>& compositeRectSizes) override {
            ++called;
            CORRADE_COMPARE(state, LayerState::NeedsNodeOffsetSizeUpdate|LayerState::NeedsCommonDataUpdate);
            CORRADE_COMPARE_AS(dataIds, Containers::arrayView({
                0xabcdeu,
                0x45678u
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(clipRectIds, Containers::arrayView({
                /* These should be small enough to index into clipRectOffsets
                   and clipRectSizes but nobody cares here */
                3u,
                16u,
                27u,
                2u
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(clipRectDataCounts, Containers::arrayView({
                /* The sum should be equal to dataIds.size(), yes, nobody cares
                   here */
                265u,
                1u,
                13u,
                7u
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(nodeOffsets, Containers::arrayView<Vector2>({
                {1.0f, 2.0f},
                {3.0f, 4.0f},
                {5.0f, 6.0f}
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(nodeSizes, Containers::arrayView<Vector2>({
                {0.1f, 0.2f},
                {0.3f, 0.4f},
                {0.5f, 0.6f}
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(nodesEnabled, Containers::stridedArrayView({
                true,
                false,
                true
            }).sliceBit(0), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(clipRectOffsets, Containers::arrayView<Vector2>({
                {6.5f, 7.5f},
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(clipRectSizes, Containers::arrayView<Vector2>({
                {8.5f, 9.5f},
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(compositeRectOffsets, Containers::arrayView<Vector2>({
                {3.0f, 4.0f},
                {5.0f, 6.0f}
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(compositeRectSizes, Containers::arrayView<Vector2>({
                {0.3f, 0.4f},
                {0.5f, 0.6f}
            }), TestSuite::Compare::Container);
        }

        Int called = 0;
    } layer{layerHandle(0, 1)};

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    UnsignedByte nodesEnabled[1]{0x5};

    /* Required to be called before update() (because AbstractUserInterface
       guarantees the same on a higher level), not needed for anything here */
    layer.setSize({1, 1}, {1, 1});

    layer.update(
        LayerState::NeedsNodeOffsetSizeUpdate|LayerState::NeedsCommonDataUpdate,
        Containers::arrayView({
            0xabcdeu,
            0x45678u,
        }),
        Containers::arrayView({
            3u,
            16u,
            27u,
            2u
        }),
        Containers::arrayView({
            265u,
            1u,
            13u,
            7u
        }),
        Containers::arrayView<Vector2>({
            {1.0f, 2.0f},
            {3.0f, 4.0f},
            {5.0f, 6.0f}
        }),
        Containers::arrayView<Vector2>({
            {0.1f, 0.2f},
            {0.3f, 0.4f},
            {0.5f, 0.6f}
        }),
        Containers::BitArrayView{nodesEnabled, 0, 3},
        Containers::arrayView<Vector2>({
            {6.5f, 7.5f},
        }),
        Containers::arrayView<Vector2>({
            {8.5f, 9.5f},
        }),
        Containers::arrayView<Vector2>({
            {3.0f, 4.0f},
            {5.0f, 6.0f}
        }),
        Containers::arrayView<Vector2>({
            {0.3f, 0.4f},
            {0.5f, 0.6f}
        }));
    CORRADE_COMPARE(layer.called, 1);
}

void AbstractLayerTest::updateEmpty() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }

        void doUpdate(LayerStates, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, Containers::BitArrayView, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&) override {
            ++called;
        }

        Int called = 0;
    } layer{layerHandle(0, 1)};

    /* It should call the implementation even with empty contents */
    layer.update(LayerState::NeedsSharedDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.called, 1);
}

void AbstractLayerTest::updateNotImplemented() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0, 1)};

    UnsignedByte nodesEnabled[1]{};

    layer.update(
        LayerState::NeedsSharedDataUpdate,
        Containers::arrayView({
            0u,
            0u
        }),
        Containers::arrayView({
            0u,
            0u,
            0u,
            0u
        }),
        Containers::arrayView({
            0u,
            0u,
            0u,
            0u
        }),
        Containers::arrayView<Vector2>({
            {},
            {},
            {}
        }),
        Containers::arrayView<Vector2>({
            {},
            {},
            {}
        }),
        Containers::BitArrayView{nodesEnabled, 0, 3},
        Containers::arrayView<Vector2>({
            {}
        }),
        Containers::arrayView<Vector2>({
            {}
        }),
        {}, {}
    );

    /* Shouldn't crash or anything */
    CORRADE_VERIFY(true);
}

void AbstractLayerTest::updateInvalidState() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0, 1)};

    std::ostringstream out;
    Error redirectError{&out};
    layer.update({}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    layer.update(LayerState::NeedsDataClean, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    layer.update(LayerState::NeedsCompositeOffsetSizeUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE_AS(out.str(),
        "Ui::AbstractLayer::update(): expected a non-empty subset of Ui::LayerState::NeedsNodeOffsetSizeUpdate|Ui::LayerState::NeedsAttachmentUpdate|Ui::LayerState::NeedsDataUpdate|Ui::LayerState::NeedsCommonDataUpdate|Ui::LayerState::NeedsSharedDataUpdate but got Ui::LayerStates{}\n"
        "Ui::AbstractLayer::update(): expected a non-empty subset of Ui::LayerState::NeedsNodeOffsetSizeUpdate|Ui::LayerState::NeedsAttachmentUpdate|Ui::LayerState::NeedsDataUpdate|Ui::LayerState::NeedsCommonDataUpdate|Ui::LayerState::NeedsSharedDataUpdate but got Ui::LayerState::NeedsDataClean\n"
        "Ui::AbstractLayer::update(): expected a non-empty subset of Ui::LayerState::NeedsNodeOffsetSizeUpdate|Ui::LayerState::NeedsAttachmentUpdate|Ui::LayerState::NeedsDataUpdate|Ui::LayerState::NeedsCommonDataUpdate|Ui::LayerState::NeedsSharedDataUpdate but got Ui::LayerState::NeedsCompositeOffsetSizeUpdate\n",
        TestSuite::Compare::String);
}

void AbstractLayerTest::updateInvalidStateComposite() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Composite;
        }
    } layer{layerHandle(0, 1)};

    std::ostringstream out;
    Error redirectError{&out};
    layer.update({}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    layer.update(LayerState::NeedsDataClean, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE_AS(out.str(),
        "Ui::AbstractLayer::update(): expected a non-empty subset of Ui::LayerState::NeedsNodeOffsetSizeUpdate|Ui::LayerState::NeedsAttachmentUpdate|Ui::LayerState::NeedsDataUpdate|Ui::LayerState::NeedsCommonDataUpdate|Ui::LayerState::NeedsSharedDataUpdate|Ui::LayerState::NeedsCompositeOffsetSizeUpdate but got Ui::LayerStates{}\n"
        "Ui::AbstractLayer::update(): expected a non-empty subset of Ui::LayerState::NeedsNodeOffsetSizeUpdate|Ui::LayerState::NeedsAttachmentUpdate|Ui::LayerState::NeedsDataUpdate|Ui::LayerState::NeedsCommonDataUpdate|Ui::LayerState::NeedsSharedDataUpdate|Ui::LayerState::NeedsCompositeOffsetSizeUpdate but got Ui::LayerState::NeedsDataClean\n",
        TestSuite::Compare::String);
}

void AbstractLayerTest::updateInvalidSizes() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0, 1)};

    UnsignedByte nodesEnabled[1]{};

    std::ostringstream out;
    Error redirectError{&out};
    layer.update(
        LayerState::NeedsDataUpdate,
        {},
        Containers::arrayView({
            0u,
            0u,
            0u
        }),
        Containers::arrayView({
            0u,
            0u
        }),
        {}, {}, {},
        {}, {},
        {}, {}
    );
    layer.update(
        LayerState::NeedsDataUpdate,
        {}, {}, {},
        Containers::arrayView<Vector2>({
            {},
            {}
        }),
        Containers::arrayView<Vector2>({
            {},
            {},
            {}
        }),
        Containers::BitArrayView{nodesEnabled, 0, 2},
        {}, {},
        {}, {}
    );
    layer.update(
        LayerState::NeedsDataUpdate,
        {}, {}, {},
        Containers::arrayView<Vector2>({
            {},
            {}
        }),
        Containers::arrayView<Vector2>({
            {},
            {}
        }),
        Containers::BitArrayView{nodesEnabled, 0, 3},
        {}, {},
        {}, {}
    );
    layer.update(
        LayerState::NeedsDataUpdate,
        {}, {}, {},
        {}, {}, {},
        Containers::arrayView<Vector2>({
            {},
            {},
            {}
        }),
        Containers::arrayView<Vector2>({
            {},
            {}
        }),
        {}, {}
    );
    layer.update(
        LayerState::NeedsDataUpdate,
        {}, {}, {},
        {}, {}, {},
        {}, {},
        Containers::arrayView<Vector2>({
            {},
            {},
            {}
        }),
        Containers::arrayView<Vector2>({
            {},
            {}
        })
    );
    layer.update(
        LayerState::NeedsDataUpdate,
        {}, {}, {},
        {}, {}, {},
        {}, {},
        Containers::arrayView<Vector2>({
            {},
            {}
        }),
        Containers::arrayView<Vector2>({
            {},
            {}
        })
    );
    CORRADE_COMPARE_AS(out.str(),
        "Ui::AbstractLayer::update(): expected clip rect ID and data count views to have the same size but got 3 and 2\n"
        "Ui::AbstractLayer::update(): expected node offset, size and enabled views to have the same size but got 2, 3 and 2\n"
        "Ui::AbstractLayer::update(): expected node offset, size and enabled views to have the same size but got 2, 2 and 3\n"
        "Ui::AbstractLayer::update(): expected clip rect offset and size views to have the same size but got 3 and 2\n"
        "Ui::AbstractLayer::update(): expected composite rect offset and size views to have the same size but got 3 and 2\n"
        "Ui::AbstractLayer::update(): compositing not supported but got 2 composite rects\n",
        TestSuite::Compare::String);
}

void AbstractLayerTest::updateNoSizeSet() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct Layer: AbstractLayer {
        Layer(LayerHandle handle, LayerFeatures features): AbstractLayer{handle}, _features{features} {}

        LayerFeatures doFeatures() const override { return _features; }

        private:
            LayerFeatures _features;
    } layerNoDraw{layerHandle(0, 1), {}},
      layer{layerHandle(0, 1), LayerFeature::Draw};

    /* It's fine if the layer doesn't support drawing */
    layerNoDraw.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});

    std::ostringstream out;
    Error redirectError{&out};
    layer.update(LayerState::NeedsDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(out.str(), "Ui::AbstractLayer::update(): user interface size wasn't set\n");
}

void AbstractLayerTest::state() {
    auto&& data = StateData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct Layer: AbstractLayer {
        explicit Layer(LayerHandle handle, LayerFeatures features): AbstractLayer{handle}, _features{features} {}

        using AbstractLayer::create;
        using AbstractLayer::remove;

        LayerFeatures doFeatures() const override { return _features; }

        void doUpdate(LayerStates state, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, Containers::BitArrayView, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&) override {
            /* The doUpdate() should never get the NeedsAttachmentUpdate, only
               the NeedsNodeOrderUpdate that's a subset of it */
            CORRADE_VERIFY(!(state >= LayerState::NeedsAttachmentUpdate));
        }

        private:
            LayerFeatures _features;
    } layer{layerHandle(0xab, 0x12), data.features};

    /* Required to be called before update() (because AbstractUserInterface
       guarantees the same on a higher level), not needed for anything here */
    if(data.features >= LayerFeature::Draw)
        layer.setSize({1, 1}, {1, 1});

    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Creating a data adds NeedsDataUpdate */
    DataHandle data1 = layer.create();
    DataHandle data2 = layer.create();
    DataHandle data3 = layer.create();
    DataHandle data4 = layer.create();
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate);

    /* update() then resets it, if passed the same flag */
    layer.update(LayerState::NeedsDataUpdate|LayerState::NeedsNodeOrderUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Creating an attached data sets more state flags */
    DataHandle data5 = layer.create(nodeHandle(0, 0x123));
    CORRADE_COMPARE(layer.state(), Ui::LayerState::NeedsNodeOffsetSizeUpdate|Ui::LayerState::NeedsNodeEnabledUpdate|LayerState::NeedsAttachmentUpdate|LayerState::NeedsDataUpdate|data.extraAttachState);

    /* update() then resets it, if passed the same flag */
    layer.update(Ui::LayerState::NeedsNodeOffsetSizeUpdate|Ui::LayerState::NeedsNodeEnabledUpdate|LayerState::NeedsAttachmentUpdate|LayerState::NeedsDataUpdate|data.extraAttachState, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* No other way to trigger any of these flags */
    layer.setNeedsUpdate(LayerState::NeedsSharedDataUpdate|LayerState::NeedsCommonDataUpdate);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsSharedDataUpdate|LayerState::NeedsCommonDataUpdate);

    /* update() then resets the subset that was passed */
    layer.update(LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerState::NeedsSharedDataUpdate);

    /* update() again for the remaining */
    layer.update(LayerState::NeedsSharedDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Attaching to a node sets state flags */
    layer.attach(data2, nodeHandle(0, 0x123));
    layer.attach(data3, nodeHandle(0, 0x123));
    layer.attach(data4, nodeHandle(0, 0x123));
    CORRADE_COMPARE(layer.state(), Ui::LayerState::NeedsNodeOffsetSizeUpdate|Ui::LayerState::NeedsNodeEnabledUpdate|LayerState::NeedsAttachmentUpdate|data.extraAttachState);

    /* update() then resets them */
    layer.update(Ui::LayerState::NeedsNodeOffsetSizeUpdate|Ui::LayerState::NeedsNodeEnabledUpdate|LayerState::NeedsAttachmentUpdate|data.extraAttachState, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Detaching sets a state flag as well. Also testing the other overload
       here. */
    layer.attach(dataHandleData(data5), NodeHandle::Null);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsAttachmentUpdate);

    /* update() then resets it */
    layer.update(LayerState::NeedsAttachmentUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Attaching/detaching an already-attached/detached data does nothing */
    layer.attach(data1, NodeHandle::Null);
    layer.attach(data4, nodeHandle(0, 0x123));
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* remove() adds NeedsDataClean */
    layer.remove(data1);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataClean);

    /* cleanNodes() is a no-op, doesn't affect this flag. Passing the matching
       generation to not make it remove any data. */
    layer.cleanNodes(Containers::arrayView({UnsignedShort{0x123}}));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataClean);

    /* cleanData() then resets NeedsDataClean. Passing no animators is a valid
       case as not every layer may have any attached. */
    layer.cleanData({});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* remove() adds NeedsAttachmentUpdate if the data were attached */
    layer.remove(data2);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataClean|LayerState::NeedsAttachmentUpdate);

    /* update() then resets one */
    layer.update(LayerState::NeedsAttachmentUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataClean);

    /* cleanData() the other */
    layer.cleanData({});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Testing the other overload */
    layer.remove(dataHandleData(data3));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataClean|LayerState::NeedsAttachmentUpdate);

    /* update() and cleanData() then resets it */
    layer.update(LayerState::NeedsAttachmentUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    layer.cleanData({});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* cleanNodes() that removes a data doesn't set any flags either */
    CORRADE_VERIFY(layer.isHandleValid(data4));
    layer.cleanNodes(Containers::arrayView({UnsignedShort{0xfef}}));
    CORRADE_COMPARE(layer.state(), LayerStates{});
    CORRADE_VERIFY(!layer.isHandleValid(data4));
}

void AbstractLayerTest::composite() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Composite;
        }

        void doComposite(AbstractRenderer& renderer, const Containers::StridedArrayView1D<const Vector2>& rectOffsets, const Containers::StridedArrayView1D<const Vector2>& rectSizes, std::size_t offset, std::size_t count) override {
            ++called;
            CORRADE_COMPARE(renderer.framebufferSize(), (Vector2i{12, 34}));
            CORRADE_COMPARE_AS(rectOffsets, Containers::arrayView<Vector2>({
                {1.0f, 2.0f},
                {3.0f, 4.0f},
                {5.0f, 6.0f},
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(rectSizes, Containers::arrayView<Vector2>({
                {0.1f, 0.2f},
                {0.3f, 0.4f},
                {0.5f, 0.6f}
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE(offset, 1);
            CORRADE_COMPARE(count, 2);
        }

        Int called = 0;
    } layer{layerHandle(0, 1)};

    struct: AbstractRenderer {
        RendererFeatures doFeatures() const override { return {}; }
        void doSetupFramebuffers(const Vector2i&) override {}
        void doTransition(RendererTargetState, RendererTargetState, RendererDrawStates, RendererDrawStates) override {}
    } renderer;
    renderer.setupFramebuffers({12, 34});

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    layer.composite(renderer,
        Containers::arrayView<Vector2>({
            {1.0f, 2.0f},
            {3.0f, 4.0f},
            {5.0f, 6.0f},
        }),
        Containers::arrayView<Vector2>({
            {0.1f, 0.2f},
            {0.3f, 0.4f},
            {0.5f, 0.6f}
        }),
        1, 2
    );
    CORRADE_COMPARE(layer.called, 1);
}

void AbstractLayerTest::compositeEmpty() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Composite;
        }

        void doComposite(AbstractRenderer&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, std::size_t, std::size_t) override {
            ++called;
        }

        Int called = 0;
    } layer{layerHandle(0, 1)};

    struct: AbstractRenderer {
        RendererFeatures doFeatures() const override { return {}; }
        void doSetupFramebuffers(const Vector2i&) override {}
        void doTransition(RendererTargetState, RendererTargetState, RendererDrawStates, RendererDrawStates) override {}
    } renderer;

    /* It should call the implementation even with empty contents */
    layer.composite(renderer, {}, {}, 0, 0);
    CORRADE_COMPARE(layer.called, 1);
}

void AbstractLayerTest::compositeNotSupported() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0, 1)};

    struct: AbstractRenderer {
        RendererFeatures doFeatures() const override { return {}; }
        void doSetupFramebuffers(const Vector2i&) override {}
        void doTransition(RendererTargetState, RendererTargetState, RendererDrawStates, RendererDrawStates) override {}
    } renderer;

    std::ostringstream out;
    Error redirectError{&out};
    layer.composite(renderer, {}, {}, 0, 0);
    CORRADE_COMPARE(out.str(), "Ui::AbstractLayer::composite(): feature not supported\n");
}

void AbstractLayerTest::compositeNotImplemented() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Composite;
        }
    } layer{layerHandle(0, 1)};

    struct: AbstractRenderer {
        RendererFeatures doFeatures() const override { return {}; }
        void doSetupFramebuffers(const Vector2i&) override {}
        void doTransition(RendererTargetState, RendererTargetState, RendererDrawStates, RendererDrawStates) override {}
    } renderer;

    std::ostringstream out;
    Error redirectError{&out};
    layer.composite(renderer, {}, {}, 0, 0);
    CORRADE_COMPARE(out.str(), "Ui::AbstractLayer::composite(): feature advertised but not implemented\n");
}

void AbstractLayerTest::compositeInvalidSizes() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Composite;
        }
    } layer{layerHandle(0, 1)};

    struct: AbstractRenderer {
        RendererFeatures doFeatures() const override { return {}; }
        void doSetupFramebuffers(const Vector2i&) override {}
        void doTransition(RendererTargetState, RendererTargetState, RendererDrawStates, RendererDrawStates) override {}
    } renderer;

    std::ostringstream out;
    Error redirectError{&out};
    layer.composite(renderer,
        Containers::arrayView<Vector2>({
            {},
            {}
        }),
        Containers::arrayView<Vector2>({
            {},
            {},
            {}
        }),
        0, 0
    );
    layer.composite(renderer,
        Containers::arrayView<Vector2>({
            {},
            {}
        }),
        Containers::arrayView<Vector2>({
            {},
            {}
        }),
        3, 0
    );
    layer.composite(renderer,
        Containers::arrayView<Vector2>({
            {},
            {}
        }),
        Containers::arrayView<Vector2>({
            {},
            {}
        }),
        2, 1
    );
    CORRADE_COMPARE_AS(out.str(),
        "Ui::AbstractLayer::composite(): expected rect offset and size views to have the same size but got 2 and 3\n"
        "Ui::AbstractLayer::composite(): offset 3 and count 0 out of range for 2 items\n"
        "Ui::AbstractLayer::composite(): offset 2 and count 1 out of range for 2 items\n",
        TestSuite::Compare::String);
}

void AbstractLayerTest::draw() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Draw;
        }

        void doDraw(const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, std::size_t offset, std::size_t count, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, std::size_t clipRectOffset, std::size_t clipRectCount, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, Containers::BitArrayView nodesEnabled, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes) override {
            ++called;
            CORRADE_COMPARE_AS(dataIds, Containers::arrayView({
                0xabcdeu,
                0u,
                0x45678u,
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE(offset, 1);
            CORRADE_COMPARE(count, 2);
            CORRADE_COMPARE_AS(clipRectIds, Containers::arrayView({
                /* These should be small enough to index into clipRectOffsets
                   and clipRectSizes but nobody cares here */
                3u,
                16u,
                0u,
                27u,
                2u
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(clipRectDataCounts, Containers::arrayView({
                /* The sum should be equal to dataIds.size(), yes, nobody cares
                   here */
                265u,
                1u,
                0u,
                13u,
                7u
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE(clipRectOffset, 2);
            CORRADE_COMPARE(clipRectCount, 3);
            CORRADE_COMPARE_AS(nodeOffsets, Containers::arrayView<Vector2>({
                {1.0f, 2.0f},
                {3.0f, 4.0f}
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(nodeSizes, Containers::arrayView<Vector2>({
                {0.1f, 0.2f},
                {0.3f, 0.4f}
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(nodesEnabled, Containers::stridedArrayView({
                false,
                true
            }).sliceBit(0), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(clipRectOffsets, Containers::arrayView<Vector2>({
                {6.5f, 7.5f},
            }), TestSuite::Compare::Container);
            CORRADE_COMPARE_AS(clipRectSizes, Containers::arrayView<Vector2>({
                {8.5f, 9.5f},
            }), TestSuite::Compare::Container);
        }

        Int called = 0;
    } layer{layerHandle(0, 1)};

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    UnsignedByte nodesEnabled[1]{0x2};

    layer.draw(
        Containers::arrayView({
            0xabcdeu,
            0u,
            0x45678u
        }),
        1, 2,
        Containers::arrayView({
            3u,
            16u,
            0u,
            27u,
            2u
        }),
        Containers::arrayView({
            265u,
            1u,
            0u,
            13u,
            7u
        }),
        2, 3,
        Containers::arrayView<Vector2>({
            {1.0f, 2.0f},
            {3.0f, 4.0f}
        }),
        Containers::arrayView<Vector2>({
            {0.1f, 0.2f},
            {0.3f, 0.4f}
        }),
        Containers::BitArrayView{nodesEnabled, 0, 2},
        Containers::arrayView<Vector2>({
            {6.5f, 7.5f},
        }),
        Containers::arrayView<Vector2>({
            {8.5f, 9.5f},
        })
    );
    CORRADE_COMPARE(layer.called, 1);
}

void AbstractLayerTest::drawEmpty() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Draw;
        }

        void doDraw(const Containers::StridedArrayView1D<const UnsignedInt>&, std::size_t, std::size_t, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const UnsignedInt>&, std::size_t, std::size_t, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, Containers::BitArrayView, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&) override {
            ++called;
        }

        Int called = 0;
    } layer{layerHandle(0, 1)};

    /* It should call the implementation even with empty contents */
    layer.draw({}, 0, 0, {}, {}, 0, 0, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.called, 1);
}

void AbstractLayerTest::drawNotSupported() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0, 1)};

    std::ostringstream out;
    Error redirectError{&out};
    layer.draw({}, 0, 0, {}, {}, 0, 0, {}, {}, {}, {}, {});
    CORRADE_COMPARE(out.str(), "Ui::AbstractLayer::draw(): feature not supported\n");
}

void AbstractLayerTest::drawNotImplemented() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Draw;
        }
    } layer{layerHandle(0, 1)};

    std::ostringstream out;
    Error redirectError{&out};
    layer.draw({}, 0, 0, {}, {}, 0, 0, {}, {}, {}, {}, {});
    CORRADE_COMPARE(out.str(), "Ui::AbstractLayer::draw(): feature advertised but not implemented\n");
}

void AbstractLayerTest::drawInvalidSizes() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Draw;
        }
    } layer{layerHandle(0, 1)};

    UnsignedByte nodesEnabled[1]{};

    std::ostringstream out;
    Error redirectError{&out};
    layer.draw(
        {},
        0, 0,
        Containers::arrayView({
            0u,
            0u,
            0u
        }),
        Containers::arrayView({
            0u,
            0u
        }),
        0, 0,
        {}, {}, {},
        {}, {}
    );
    layer.draw(
        {},
        0, 0,
        {}, {},
        0, 0,
        Containers::arrayView<Vector2>({
            {},
            {}
        }),
        Containers::arrayView<Vector2>({
            {},
            {},
            {}
        }),
        Containers::BitArrayView{nodesEnabled, 0, 2},
        {}, {}
    );
    layer.draw(
        {},
        0, 0,
        {}, {},
        0, 0,
        Containers::arrayView<Vector2>({
            {},
            {}
        }),
        Containers::arrayView<Vector2>({
            {},
            {}
        }),
        Containers::BitArrayView{nodesEnabled, 0, 3},
        {}, {}
    );
    layer.draw(
        {},
        0, 0,
        {}, {},
        0, 0,
        {}, {}, {},
        Containers::arrayView<Vector2>({
            {},
            {},
            {}
        }),
        Containers::arrayView<Vector2>({
            {},
            {}
        })
    );
    layer.draw(
        Containers::arrayView({
            0u,
            0u
        }),
        3, 0,
        {}, {},
        0, 0,
        {}, {}, {},
        {}, {}
    );
    layer.draw(
        Containers::arrayView({
            0u,
            0u
        }),
        2, 1,
        {}, {},
        0, 0,
        {}, {}, {},
        {}, {}
    );
    layer.draw(
        {},
        0, 0,
        Containers::arrayView({
            0u,
            0u,
            0u
        }),
        Containers::arrayView({
            0u,
            0u,
            0u
        }),
        4, 0,
        {}, {}, {},
        {}, {}
    );
    layer.draw(
        {},
        0, 0,
        Containers::arrayView({
            0u,
            0u,
            0u
        }),
        Containers::arrayView({
            0u,
            0u,
            0u
        }),
        1, 3,
        {}, {}, {},
        {}, {}
    );
    CORRADE_COMPARE_AS(out.str(),
        "Ui::AbstractLayer::draw(): expected clip rect ID and data count views to have the same size but got 3 and 2\n"
        "Ui::AbstractLayer::draw(): expected node offset, size and enabled views to have the same size but got 2, 3 and 2\n"
        "Ui::AbstractLayer::draw(): expected node offset, size and enabled views to have the same size but got 2, 2 and 3\n"
        "Ui::AbstractLayer::draw(): expected clip rect offset and size views to have the same size but got 3 and 2\n"
        "Ui::AbstractLayer::draw(): offset 3 and count 0 out of range for 2 items\n"
        "Ui::AbstractLayer::draw(): offset 2 and count 1 out of range for 2 items\n"
        "Ui::AbstractLayer::draw(): clip rect offset 4 and count 0 out of range for 3 items\n"
        "Ui::AbstractLayer::draw(): clip rect offset 1 and count 3 out of range for 3 items\n",
        TestSuite::Compare::String);
}

void AbstractLayerTest::pointerEvent() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Event;
        }

        void doPointerPressEvent(UnsignedInt dataId, PointerEvent& event) override {
            CORRADE_COMPARE(dataId, 1);
            CORRADE_COMPARE(event.time(), 123_nsec);
            CORRADE_COMPARE(event.pointer(), Pointer::MouseLeft);
            called *= 2;
        }
        void doPointerReleaseEvent(UnsignedInt dataId, PointerEvent& event) override {
            CORRADE_COMPARE(dataId, 2);
            CORRADE_COMPARE(event.time(), 1234_nsec);
            CORRADE_COMPARE(event.pointer(), Pointer::MouseRight);
            called *= 3;
        }
        void doPointerTapOrClickEvent(UnsignedInt dataId, PointerEvent& event) override {
            CORRADE_COMPARE(dataId, 3);
            CORRADE_COMPARE(event.time(), 12345_nsec);
            CORRADE_COMPARE(event.pointer(), Pointer::Pen);
            called *= 5;
        }
        void doPointerMoveEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            CORRADE_COMPARE(dataId, 4);
            CORRADE_COMPARE(event.time(), 123456_nsec);
            CORRADE_COMPARE(event.pointer(), Pointer::Finger);
            called *= 7;
        }
        void doPointerEnterEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            CORRADE_COMPARE(dataId, 5);
            CORRADE_COMPARE(event.time(), 1234567_nsec);
            CORRADE_COMPARE(event.pointer(), Pointer::Finger);
            called *= 11;
        }
        void doPointerLeaveEvent(UnsignedInt dataId, PointerMoveEvent& event) override {
            CORRADE_COMPARE(dataId, 6);
            CORRADE_COMPARE(event.time(), 12345678_nsec);
            CORRADE_COMPARE(event.pointer(), Pointer::Finger);
            called *= 13;
        }

        int called = 1;
    } layer{layerHandle(0, 1)};

    /* Capture correct test case name */
    CORRADE_VERIFY(true);

    layer.create();
    layer.create();
    layer.create();
    layer.create();
    layer.create();
    layer.create();
    layer.create();
    {
        PointerEvent event{123_nsec, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        layer.pointerPressEvent(1, event);
    } {
        PointerEvent event{1234_nsec, PointerEventSource::Mouse, Pointer::MouseRight, true, 0};
        layer.pointerReleaseEvent(2, event);
    } {
        PointerEvent event{12345_nsec, PointerEventSource::Pen, Pointer::Pen, true, 0};
        layer.pointerTapOrClickEvent(3, event);
    } {
        PointerMoveEvent event{123456_nsec, PointerEventSource::Touch, Pointer::Finger, {}, true, 0};
        layer.pointerMoveEvent(4, event);
    } {
        PointerMoveEvent event{1234567_nsec, PointerEventSource::Touch, Pointer::Finger, {}, true, 0};
        layer.pointerEnterEvent(5, event);
    } {
        PointerMoveEvent event{12345678_nsec, PointerEventSource::Touch, Pointer::Finger, {}, true, 0};
        layer.pointerLeaveEvent(6, event);
    }
    CORRADE_COMPARE(layer.called, 2*3*5*7*11*13);
}

void AbstractLayerTest::pointerEventNotSupported() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0, 1)};

    PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseMiddle, true, 0};
    PointerMoveEvent moveEvent{{}, PointerEventSource::Mouse, {}, {}, true, 0};

    std::ostringstream out;
    Error redirectError{&out};
    layer.pointerPressEvent(0, event);
    layer.pointerReleaseEvent(0, event);
    layer.pointerTapOrClickEvent(0, event);
    layer.pointerMoveEvent(0, moveEvent);
    layer.pointerEnterEvent(0, moveEvent);
    layer.pointerLeaveEvent(0, moveEvent);
    CORRADE_COMPARE_AS(out.str(),
        "Ui::AbstractLayer::pointerPressEvent(): feature not supported\n"
        "Ui::AbstractLayer::pointerReleaseEvent(): feature not supported\n"
        "Ui::AbstractLayer::pointerTapOrClickEvent(): feature not supported\n"
        "Ui::AbstractLayer::pointerMoveEvent(): feature not supported\n"
        "Ui::AbstractLayer::pointerEnterEvent(): feature not supported\n"
        "Ui::AbstractLayer::pointerLeaveEvent(): feature not supported\n",
        TestSuite::Compare::String);
}

void AbstractLayerTest::pointerEventNotImplemented() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Event;
        }
    } layer{layerHandle(0, 1)};

    layer.create();

    PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseMiddle, true, 0};
    PointerMoveEvent moveEvent{{}, PointerEventSource::Mouse, {}, {}, true, 0};
    layer.pointerPressEvent(0, event);
    layer.pointerReleaseEvent(0, event);
    layer.pointerTapOrClickEvent(0, event);
    layer.pointerMoveEvent(0, moveEvent);
    layer.pointerEnterEvent(0, moveEvent);
    layer.pointerLeaveEvent(0, moveEvent);

    /* Shouldn't crash or anything */
    CORRADE_VERIFY(true);
}

void AbstractLayerTest::pointerEventOutOfRange() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Event;
        }
    } layer{layerHandle(0, 1)};

    layer.create();
    layer.create();

    PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseMiddle, true, 0};
    PointerMoveEvent moveEvent{{}, PointerEventSource::Mouse, {}, {}, true, 0};

    std::ostringstream out;
    Error redirectError{&out};
    layer.pointerPressEvent(2, event);
    layer.pointerReleaseEvent(2, event);
    layer.pointerTapOrClickEvent(2, event);
    layer.pointerMoveEvent(2, moveEvent);
    layer.pointerEnterEvent(2, moveEvent);
    layer.pointerLeaveEvent(2, moveEvent);
    CORRADE_COMPARE_AS(out.str(),
        "Ui::AbstractLayer::pointerPressEvent(): index 2 out of range for 2 data\n"
        "Ui::AbstractLayer::pointerReleaseEvent(): index 2 out of range for 2 data\n"
        "Ui::AbstractLayer::pointerTapOrClickEvent(): index 2 out of range for 2 data\n"
        "Ui::AbstractLayer::pointerMoveEvent(): index 2 out of range for 2 data\n"
        "Ui::AbstractLayer::pointerEnterEvent(): index 2 out of range for 2 data\n"
        "Ui::AbstractLayer::pointerLeaveEvent(): index 2 out of range for 2 data\n",
        TestSuite::Compare::String);
}

void AbstractLayerTest::pointerEventNotPrimary() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Event;
        }
    } layer{layerHandle(0, 1)};

    layer.create();

    PointerEvent event{{}, PointerEventSource::Touch, Pointer::Finger, false, 0};
    PointerMoveEvent moveEvent{{}, PointerEventSource::Touch, {}, {}, false, 0};

    /* These can be called with non-primary events */
    layer.pointerPressEvent(0, event);
    layer.pointerReleaseEvent(0, event);
    layer.pointerMoveEvent(0, moveEvent);

    std::ostringstream out;
    Error redirectError{&out};
    layer.pointerTapOrClickEvent(0, event);
    layer.pointerEnterEvent(0, moveEvent);
    layer.pointerLeaveEvent(0, moveEvent);
    CORRADE_COMPARE(out.str(),
        "Ui::AbstractLayer::pointerTapOrClickEvent(): event not primary\n"
        "Ui::AbstractLayer::pointerEnterEvent(): event not primary\n"
        "Ui::AbstractLayer::pointerLeaveEvent(): event not primary\n");
}

void AbstractLayerTest::pointerEventAlreadyAccepted() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Event;
        }
    } layer{layerHandle(0, 1)};

    layer.create();

    PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseMiddle, true, 0};
    event.setAccepted();
    PointerMoveEvent moveEvent{{}, PointerEventSource::Mouse, {}, {}, true, 0};
    moveEvent.setAccepted();

    std::ostringstream out;
    Error redirectError{&out};
    layer.pointerPressEvent(0, event);
    layer.pointerReleaseEvent(0, event);
    layer.pointerTapOrClickEvent(0, event);
    layer.pointerMoveEvent(0, moveEvent);
    layer.pointerEnterEvent(0, moveEvent);
    layer.pointerLeaveEvent(0, moveEvent);
    CORRADE_COMPARE(out.str(),
        "Ui::AbstractLayer::pointerPressEvent(): event already accepted\n"
        "Ui::AbstractLayer::pointerReleaseEvent(): event already accepted\n"
        "Ui::AbstractLayer::pointerTapOrClickEvent(): event already accepted\n"
        "Ui::AbstractLayer::pointerMoveEvent(): event already accepted\n"
        "Ui::AbstractLayer::pointerEnterEvent(): event already accepted\n"
        "Ui::AbstractLayer::pointerLeaveEvent(): event already accepted\n");
}

void AbstractLayerTest::focusBlurEvent() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Event;
        }

        void doFocusEvent(UnsignedInt dataId, FocusEvent& event) override {
            CORRADE_COMPARE(dataId, 1);
            CORRADE_COMPARE(event.time(), 123_nsec);
            called *= 2;
        }
        void doBlurEvent(UnsignedInt dataId, FocusEvent& event) override {
            CORRADE_COMPARE(dataId, 2);
            CORRADE_COMPARE(event.time(), 1234_nsec);
            called *= 3;
        }

        int called = 1;
    } layer{layerHandle(0, 1)};

    /* Capture correct test case name */
    CORRADE_VERIFY(true);

    layer.create();
    layer.create();
    layer.create();
    {
        FocusEvent event{123_nsec};
        layer.focusEvent(1, event);
    } {
        FocusEvent event{1234_nsec};
        layer.blurEvent(2, event);
    }
    CORRADE_COMPARE(layer.called, 2*3);
}

void AbstractLayerTest::focusBlurEventNotSupported() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0, 1)};

    FocusEvent event{{}};

    std::ostringstream out;
    Error redirectError{&out};
    layer.focusEvent(0, event);
    layer.blurEvent(0, event);
    CORRADE_COMPARE(out.str(),
        "Ui::AbstractLayer::focusEvent(): feature not supported\n"
        "Ui::AbstractLayer::blurEvent(): feature not supported\n");
}

void AbstractLayerTest::focusBlurEventNotImplemented() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Event;
        }
    } layer{layerHandle(0, 1)};

    layer.create();

    FocusEvent event{{}};
    layer.focusEvent(0, event);
    layer.blurEvent(0, event);

    /* Shouldn't crash or anything */
    CORRADE_VERIFY(true);
}

void AbstractLayerTest::focusBlurEventOutOfRange() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Event;
        }
    } layer{layerHandle(0, 1)};

    layer.create();
    layer.create();

    FocusEvent event{{}};

    std::ostringstream out;
    Error redirectError{&out};
    layer.focusEvent(2, event);
    layer.blurEvent(2, event);
    CORRADE_COMPARE(out.str(),
        "Ui::AbstractLayer::focusEvent(): index 2 out of range for 2 data\n"
        "Ui::AbstractLayer::blurEvent(): index 2 out of range for 2 data\n");
}

void AbstractLayerTest::focusBlurEventAlreadyAccepted() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Event;
        }
    } layer{layerHandle(0, 1)};

    layer.create();

    FocusEvent event{{}};
    event.setAccepted();

    std::ostringstream out;
    Error redirectError{&out};
    layer.focusEvent(0, event);
    layer.blurEvent(0, event);
    CORRADE_COMPARE(out.str(),
        "Ui::AbstractLayer::focusEvent(): event already accepted\n"
        "Ui::AbstractLayer::blurEvent(): event already accepted\n");
}

void AbstractLayerTest::keyEvent() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Event;
        }

        void doKeyPressEvent(UnsignedInt dataId, KeyEvent& event) override {
            CORRADE_COMPARE(dataId, 1);
            CORRADE_COMPARE(event.time(), 1234_nsec);
            CORRADE_COMPARE(event.key(), Key::Comma);
            called *= 2;
        }
        void doKeyReleaseEvent(UnsignedInt dataId, KeyEvent& event) override {
            CORRADE_COMPARE(dataId, 2);
            CORRADE_COMPARE(event.time(), 123_nsec);
            CORRADE_COMPARE(event.key(), Key::Delete);
            CORRADE_COMPARE(event.modifiers(), Modifier::Ctrl|Modifier::Alt);
            called *= 3;
        }

        int called = 1;
    } layer{layerHandle(0, 1)};

    /* Capture correct test case name */
    CORRADE_VERIFY(true);

    layer.create();
    layer.create();
    layer.create();
    {
        KeyEvent event{1234_nsec, Key::Comma, {}};
        layer.keyPressEvent(1, event);
    } {
        KeyEvent event{123_nsec, Key::Delete, Modifier::Ctrl|Modifier::Alt};
        layer.keyReleaseEvent(2, event);
    }
    CORRADE_COMPARE(layer.called, 2*3);
}

void AbstractLayerTest::keyEventNotSupported() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0, 1)};

    KeyEvent event{{}, Key::C, {}};

    std::ostringstream out;
    Error redirectError{&out};
    layer.keyPressEvent(0, event);
    layer.keyReleaseEvent(0, event);
    CORRADE_COMPARE(out.str(),
        "Ui::AbstractLayer::keyPressEvent(): feature not supported\n"
        "Ui::AbstractLayer::keyReleaseEvent(): feature not supported\n");
}

void AbstractLayerTest::keyEventNotImplemented() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Event;
        }
    } layer{layerHandle(0, 1)};

    layer.create();

    KeyEvent event{{}, Key::NumDivide, {}};
    layer.keyPressEvent(0, event);
    layer.keyReleaseEvent(0, event);

    /* Shouldn't crash or anything */
    CORRADE_VERIFY(true);
}

void AbstractLayerTest::keyEventOutOfRange() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Event;
        }
    } layer{layerHandle(0, 1)};

    layer.create();
    layer.create();

    KeyEvent event{{}, Key::Backquote, {}};

    std::ostringstream out;
    Error redirectError{&out};
    layer.keyPressEvent(2, event);
    layer.keyReleaseEvent(2, event);
    CORRADE_COMPARE(out.str(),
        "Ui::AbstractLayer::keyPressEvent(): index 2 out of range for 2 data\n"
        "Ui::AbstractLayer::keyReleaseEvent(): index 2 out of range for 2 data\n");
}

void AbstractLayerTest::keyEventAlreadyAccepted() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Event;
        }
    } layer{layerHandle(0, 1)};

    layer.create();

    KeyEvent event{{}, Key::Space, {}};
    event.setAccepted();

    std::ostringstream out;
    Error redirectError{&out};
    layer.keyPressEvent(0, event);
    layer.keyReleaseEvent(0, event);
    CORRADE_COMPARE(out.str(),
        "Ui::AbstractLayer::keyPressEvent(): event already accepted\n"
        "Ui::AbstractLayer::keyReleaseEvent(): event already accepted\n");
}

void AbstractLayerTest::textInputEvent() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Event;
        }

        void doTextInputEvent(UnsignedInt dataId, TextInputEvent& event) override {
            CORRADE_COMPARE(dataId, 1);
            CORRADE_COMPARE(event.time(), 123_nsec);
            CORRADE_COMPARE(event.text(), "hello");
            CORRADE_COMPARE(event.text().flags(), Containers::StringViewFlag::Global);
            called *= 2;
        }

        int called = 1;
    } layer{layerHandle(0, 1)};

    /* Capture correct test case name */
    CORRADE_VERIFY(true);

    layer.create();
    layer.create();
    {
        /* To verify the string view doesn't get copied anywhere on the way */
        TextInputEvent event{123_nsec, "hello!"_s.exceptSuffix(1)};
        layer.textInputEvent(1, event);
    }
    CORRADE_COMPARE(layer.called, 2);
}

void AbstractLayerTest::textInputEventNotSupported() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0, 1)};

    TextInputEvent event{{}, "oh"};

    std::ostringstream out;
    Error redirectError{&out};
    layer.textInputEvent(0, event);
    CORRADE_COMPARE(out.str(),
        "Ui::AbstractLayer::textInputEvent(): feature not supported\n");
}

void AbstractLayerTest::textInputEventNotImplemented() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Event;
        }
    } layer{layerHandle(0, 1)};

    layer.create();

    TextInputEvent event{{}, "hey"};
    layer.textInputEvent(0, event);

    /* Shouldn't crash or anything */
    CORRADE_VERIFY(true);
}

void AbstractLayerTest::textInputEventOutOfRange() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Event;
        }
    } layer{layerHandle(0, 1)};

    layer.create();
    layer.create();

    TextInputEvent event{{}, "ooh"};

    std::ostringstream out;
    Error redirectError{&out};
    layer.textInputEvent(2, event);
    CORRADE_COMPARE(out.str(),
        "Ui::AbstractLayer::textInputEvent(): index 2 out of range for 2 data\n");
}

void AbstractLayerTest::textInputEventAlreadyAccepted() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Event;
        }
    } layer{layerHandle(0, 1)};

    layer.create();

    TextInputEvent event{{}, "welp"};
    event.setAccepted();

    std::ostringstream out;
    Error redirectError{&out};
    layer.textInputEvent(0, event);
    CORRADE_COMPARE(out.str(),
        "Ui::AbstractLayer::textInputEvent(): event already accepted\n");
}

void AbstractLayerTest::visibilityLostEvent() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Event;
        }

        void doVisibilityLostEvent(UnsignedInt dataId, VisibilityLostEvent&) override {
            CORRADE_COMPARE(dataId, 1);
            called *= 2;
        }

        int called = 1;
    } layer{layerHandle(0, 1)};

    /* Capture correct test case name */
    CORRADE_VERIFY(true);

    layer.create();
    layer.create();
    {
        VisibilityLostEvent event;
        layer.visibilityLostEvent(1, event);
    }
    CORRADE_COMPARE(layer.called, 2);
}

void AbstractLayerTest::visibilityLostEventNotSupported() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0, 1)};

    VisibilityLostEvent event;

    std::ostringstream out;
    Error redirectError{&out};
    layer.visibilityLostEvent(0, event);
    CORRADE_COMPARE_AS(out.str(),
        "Ui::AbstractLayer::visibilityLostEvent(): feature not supported\n",
        TestSuite::Compare::String);
}

void AbstractLayerTest::visibilityLostEventNotImplemented() {
    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Event;
        }
    } layer{layerHandle(0, 1)};

    layer.create();

    VisibilityLostEvent event;
    layer.visibilityLostEvent(0, event);

    /* Shouldn't crash or anything */
    CORRADE_VERIFY(true);
}

void AbstractLayerTest::visibilityLostEventOutOfRange() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;
        using AbstractLayer::create;

        LayerFeatures doFeatures() const override {
            return LayerFeature::Event;
        }
    } layer{layerHandle(0, 1)};

    layer.create();
    layer.create();

    VisibilityLostEvent event;

    std::ostringstream out;
    Error redirectError{&out};
    layer.visibilityLostEvent(2, event);
    CORRADE_COMPARE_AS(out.str(),
        "Ui::AbstractLayer::visibilityLostEvent(): index 2 out of range for 2 data\n",
        TestSuite::Compare::String);
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::AbstractLayerTest)
