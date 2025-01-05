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

#include <sstream> /** @todo remove once Debug is stream-free */
#include <Corrade/Containers/Function.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/Numeric.h>
#include <Corrade/Utility/DebugStl.h> /** @todo remove once Debug is stream-free */
#include <Magnum/Math/Complex.h>

#include "Magnum/Ui/AbstractUserInterface.h"
#include "Magnum/Ui/EventLayer.h"
#include "Magnum/Ui/Event.h"
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/NodeFlags.h"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct EventLayerTest: TestSuite::Tester {
    explicit EventLayerTest();

    void eventConnectionConstruct();
    void eventConnectionConstructCopy();
    void eventConnectionConstructMove();
    void eventConnectionDestructMovedOut();
    void eventConnectionRelease();
    void eventConnectionReleaseMovedOut();

    void construct();
    void constructCopy();
    void constructMove();
    void constructMoveScopedConnectionsActive();
    void destructScopedConnectionsActive();

    void invalidSlot();

    void call();

    void connect();
    void connectScoped();
    void remove();
    void removeScoped();
    void connectRemoveHandleRecycle();
    void cleanNodes();

    void press();
    void release();
    void releasePress();
    void pressReleaseFromUserInterface();

    void tapOrClick();
    void tapOrClickPress();
    void tapOrClickFromUserInterface();

    void middleClick();
    void middleClickPress();
    void middleClickFromUserInterface();

    void rightClick();
    void rightClickPress();
    void rightClickFromUserInterface();

    void tapOrClickMiddleClickRightClickEdges();

    void drag();
    void dragPress();
    void dragFromUserInterface();
    void dragFromUserInterfaceFallthroughThreshold();
    void dragFromUserInterfaceFallthroughThresholdMultipleHandlers();

    void pinch();
    void pinchReset();
    void pinchPressMoveRelease();
    void pinchFromUserInterface();
    void pinchFromUserInterfaceMultipleHandlers();
    void pinchAndDragFromUserInterface();

    void enter();
    void enterMove();
    void leave();
    void leaveMove();
    void enterLeaveFromUserInterface();

    void focus();
    void blur();
    void focusBlurFromUserInterface();
};

using namespace Math::Literals;

/* Same as ConnectFunctor but also with a non-trivial destructor to test the
   "should not move" behavior in the non-trivial case */
template<class ...Args> struct ConnectFunctor {
    explicit ConnectFunctor(int& output): output{&output} {
        output *= 2;
    }
    ConnectFunctor(const ConnectFunctor& other): output{other.output} {
        *output *= 3;
    }
    ConnectFunctor(ConnectFunctor&&) {
        CORRADE_FAIL("A move should not happen");
    }
    ~ConnectFunctor() {
        *output *= 5;
    }
    ConnectFunctor& operator=(const ConnectFunctor&) {
        CORRADE_FAIL("A copy assignment should not happen");
        return *this;
    }
    ConnectFunctor& operator=(ConnectFunctor&&) {
        CORRADE_FAIL("A move should not happen");
        return *this;
    }
    void operator()(Args...) {
        *output *= 7;
    }

    Int* output;
};

const struct {
    TestSuite::TestCaseDescriptionSourceLocation name;
    DataHandle(*functor)(EventLayer&, NodeHandle, Int& output);
    EventConnection(*functorScoped)(EventLayer&, NodeHandle, Int& output);
    void(*call)(EventLayer& layer, UnsignedInt dataId);
} ConnectData[]{
    #define _cn(name, function, ...) name,                                  \
        [](EventLayer& layer, NodeHandle node, Int& output) {               \
            ConnectFunctor<__VA_ARGS__> functor{output};                    \
            return layer.function(node, functor);                           \
        },                                                                  \
        [](EventLayer& layer, NodeHandle node, Int& output) {               \
            ConnectFunctor<__VA_ARGS__> functor{output};                    \
            return layer.function ## Scoped(node, functor);                 \
        }
    #define _c(name, ...) _cn(#name, name, __VA_ARGS__)
    {_c(onPress, ),
        [](EventLayer& layer, UnsignedInt dataId) {
            PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
            layer.pointerPressEvent(dataId, event);
        }},
    {_cn("onPress with a position", onPress, const Vector2&),
        [](EventLayer& layer, UnsignedInt dataId) {
            PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
            layer.pointerPressEvent(dataId, event);
        }},
    {_c(onRelease, ),
        [](EventLayer& layer, UnsignedInt dataId) {
            PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
            layer.pointerReleaseEvent(dataId, event);
        }},
    {_cn("onRelease with a position", onRelease, const Vector2&),
        [](EventLayer& layer, UnsignedInt dataId) {
            PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
            layer.pointerReleaseEvent(dataId, event);
        }},
    {_c(onTapOrClick, ),
        [](EventLayer& layer, UnsignedInt dataId) {
            /* Yes, this uses the horrific testing-only constructor */
            PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0, {}, true, {1.0f, 1.0f}};
            layer.pointerReleaseEvent(dataId, event);
        }},
    {_cn("onTapOrClick with a position", onTapOrClick, const Vector2&),
        [](EventLayer& layer, UnsignedInt dataId) {
            /* Yes, this uses the horrific testing-only constructor */
            PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0, {}, true, {1.0f, 1.0f}};
            layer.pointerReleaseEvent(dataId, event);
        }},
    {_c(onMiddleClick, ),
        [](EventLayer& layer, UnsignedInt dataId) {
            /* Yes, this uses the horrific testing-only constructor */
            PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseMiddle, true, 0, {}, true, {1.0f, 1.0f}};
            layer.pointerReleaseEvent(dataId, event);
        }},
    {_cn("onMiddleClick with a position", onMiddleClick, const Vector2&),
        [](EventLayer& layer, UnsignedInt dataId) {
            /* Yes, this uses the horrific testing-only constructor */
            PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseMiddle, true, 0, {}, true, {1.0f, 1.0f}};
            layer.pointerReleaseEvent(dataId, event);
        }},
    {_c(onRightClick, ),
        [](EventLayer& layer, UnsignedInt dataId) {
            /* Yes, this uses the horrific testing-only constructor */
            PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseRight, true, 0, {}, true, {1.0f, 1.0f}};
            layer.pointerReleaseEvent(dataId, event);
        }},
    {_cn("onRightClick with a position", onRightClick, const Vector2&),
        [](EventLayer& layer, UnsignedInt dataId) {
            /* Yes, this uses the horrific testing-only constructor */
            PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseRight, true, 0, {}, true, {1.0f, 1.0f}};
            layer.pointerReleaseEvent(dataId, event);
        }},
    {_c(onDrag, const Vector2&),
        [](EventLayer& layer, UnsignedInt dataId) {
            PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, Pointer::MouseLeft, true, 0};
            event.setCaptured(true); /* only captured events are considered */
            layer.pointerMoveEvent(dataId, event);
        }},
    {_cn("onDrag with a position", onDrag, const Vector2&, const Vector2&),
        [](EventLayer& layer, UnsignedInt dataId) {
            PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, Pointer::MouseLeft, true, 0};
            event.setCaptured(true); /* only captured events are considered */
            layer.pointerMoveEvent(dataId, event);
        }},
    {_c(onPinch, const Vector2&, const Vector2&, const Complex&, Float),
        [](EventLayer& layer, UnsignedInt dataId) {
            /* Is triggered only if at least a primary + secondary finger is
               pressed and one of them is moved */
            PointerEvent primary{{}, PointerEventSource::Touch, Pointer::Finger, true, 12};
            PointerEvent secondary{{}, PointerEventSource::Touch, Pointer::Finger, false, 34};
            PointerMoveEvent move{{}, PointerEventSource::Touch, {}, Pointer::Finger, false, 34};
            layer.pointerPressEvent(dataId, primary);
            layer.pointerPressEvent(dataId, secondary);
            layer.pointerMoveEvent(dataId, move);
        }},
    {_c(onEnter, ),
        [](EventLayer& layer, UnsignedInt dataId) {
            PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, {}, true, 0};
            layer.pointerEnterEvent(dataId, event);
        }},
    {_c(onLeave, ),
        [](EventLayer& layer, UnsignedInt dataId) {
            PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, {}, true, 0};
            layer.pointerLeaveEvent(dataId, event);
        }},
    {_c(onFocus, ),
        [](EventLayer& layer, UnsignedInt dataId) {
            FocusEvent event{{}};
            layer.focusEvent(dataId, event);
        }},
    {_c(onBlur, ),
        [](EventLayer& layer, UnsignedInt dataId) {
            FocusEvent event{{}};
            layer.blurEvent(dataId, event);
        }},
    #undef _c
};

const struct {
    const char* name;
    NodeFlags flags;
    bool parent;
} FromUserInterfaceData[]{
    {"with a node below", {}, false},
    {"with a fallthrough parent node",
        NodeFlag::FallthroughPointerEvents, true}
};

const struct {
    const char* name;
    bool positionCallback;
} DragFromUserInterfaceFallthroughThresholdData[]{
    {"", false},
    {"with position callback", true}
};

const struct {
    const char* name;
    DataHandle(EventLayer::*call)(NodeHandle, Containers::Function<void()>&&);
    PointerEventSource source;
    Pointer pointer;
} TapOrClickMiddleClickRightClickEdgesData[]{
    {"tap or click, mouse left",
        &EventLayer::onTapOrClick, PointerEventSource::Mouse, Pointer::MouseLeft},
    {"tap or click, pen",
        &EventLayer::onTapOrClick, PointerEventSource::Pen, Pointer::Pen},
    {"tap or click, finger",
        &EventLayer::onTapOrClick, PointerEventSource::Touch, Pointer::Finger},
    {"middle click",
        &EventLayer::onMiddleClick, PointerEventSource::Mouse, Pointer::MouseMiddle},
    {"right click",
        &EventLayer::onRightClick, PointerEventSource::Mouse, Pointer::MouseRight},
};

EventLayerTest::EventLayerTest() {
    addTests({&EventLayerTest::eventConnectionConstruct,
              &EventLayerTest::eventConnectionConstructCopy,
              &EventLayerTest::eventConnectionConstructMove,
              &EventLayerTest::eventConnectionDestructMovedOut,
              &EventLayerTest::eventConnectionRelease,
              &EventLayerTest::eventConnectionReleaseMovedOut,

              &EventLayerTest::construct,
              &EventLayerTest::constructCopy,
              &EventLayerTest::constructMove,
              &EventLayerTest::constructMoveScopedConnectionsActive,
              &EventLayerTest::destructScopedConnectionsActive,

              &EventLayerTest::invalidSlot,

              &EventLayerTest::call});

    addInstancedTests({&EventLayerTest::connect,
                       &EventLayerTest::connectScoped},
        Containers::arraySize(ConnectData));

    addTests({&EventLayerTest::remove,
              &EventLayerTest::removeScoped,
              &EventLayerTest::connectRemoveHandleRecycle,
              &EventLayerTest::cleanNodes,

              &EventLayerTest::press,
              &EventLayerTest::release,
              &EventLayerTest::releasePress});

    addInstancedTests({&EventLayerTest::pressReleaseFromUserInterface},
        Containers::arraySize(FromUserInterfaceData));

    addTests({&EventLayerTest::tapOrClick,
              &EventLayerTest::tapOrClickPress});

    addInstancedTests({&EventLayerTest::tapOrClickFromUserInterface},
        Containers::arraySize(FromUserInterfaceData));

    addTests({&EventLayerTest::middleClick,
              &EventLayerTest::middleClickPress});

    addInstancedTests({&EventLayerTest::middleClickFromUserInterface},
        Containers::arraySize(FromUserInterfaceData));

    addTests({&EventLayerTest::rightClick,
              &EventLayerTest::rightClickPress});

    addInstancedTests({&EventLayerTest::rightClickFromUserInterface},
        Containers::arraySize(FromUserInterfaceData));

    addInstancedTests({&EventLayerTest::tapOrClickMiddleClickRightClickEdges},
        Containers::arraySize(TapOrClickMiddleClickRightClickEdgesData));

    addTests({&EventLayerTest::drag,
              &EventLayerTest::dragPress});

    addInstancedTests({&EventLayerTest::dragFromUserInterface},
        Containers::arraySize(FromUserInterfaceData));

    addInstancedTests({&EventLayerTest::dragFromUserInterfaceFallthroughThreshold},
        Containers::arraySize(DragFromUserInterfaceFallthroughThresholdData));

    addTests({&EventLayerTest::dragFromUserInterfaceFallthroughThresholdMultipleHandlers,

              &EventLayerTest::pinch,
              &EventLayerTest::pinchReset,
              &EventLayerTest::pinchPressMoveRelease});

    addInstancedTests({&EventLayerTest::pinchFromUserInterface},
        Containers::arraySize(FromUserInterfaceData));

    addTests({&EventLayerTest::pinchFromUserInterfaceMultipleHandlers,
              &EventLayerTest::pinchAndDragFromUserInterface,

              &EventLayerTest::enter,
              &EventLayerTest::enterMove,
              &EventLayerTest::leave,
              &EventLayerTest::leaveMove});

    addInstancedTests({&EventLayerTest::enterLeaveFromUserInterface},
        Containers::arraySize(FromUserInterfaceData));

    addTests({&EventLayerTest::focus,
              &EventLayerTest::blur});

    addInstancedTests({&EventLayerTest::focusBlurFromUserInterface},
        Containers::arraySize(FromUserInterfaceData));
}

void EventLayerTest::eventConnectionConstruct() {
    EventLayer layer{layerHandle(0, 2)};

    EventConnection a = layer.onTapOrClickScoped(NodeHandle::Null, []{});
    CORRADE_COMPARE(&a.layer(), &layer);
    CORRADE_COMPARE(&const_cast<const EventConnection&>(a).layer(), &layer);
    CORRADE_COMPARE(a.data(), dataHandle(layer.handle(), 0, 1));
    CORRADE_COMPARE(layer.usedCount(), 1);
    CORRADE_COMPARE(layer.usedScopedConnectionCount(), 1);
}

void EventLayerTest::eventConnectionConstructCopy() {
    CORRADE_VERIFY(!std::is_copy_constructible<EventConnection>{});
    CORRADE_VERIFY(!std::is_copy_assignable<EventConnection>{});
}

void EventLayerTest::eventConnectionConstructMove() {
    EventLayer layer{layerHandle(0, 2)};

    {
        EventConnection a = layer.onTapOrClickScoped(NodeHandle::Null, []{});
        CORRADE_COMPARE(layer.usedCount(), 1);
        CORRADE_COMPARE(layer.usedScopedConnectionCount(), 1);

        EventConnection b = Utility::move(a);
        CORRADE_COMPARE(&b.layer(), &layer);
        CORRADE_COMPARE(b.data(), dataHandle(layer.handle(), 0, 1));
        CORRADE_COMPARE(layer.usedCount(), 1);
        CORRADE_COMPARE(layer.usedScopedConnectionCount(), 1);

        EventConnection c = layer.onTapOrClickScoped(NodeHandle::Null, []{});
        CORRADE_COMPARE(layer.usedCount(), 2);
        CORRADE_COMPARE(layer.usedScopedConnectionCount(), 2);

        c = Utility::move(b);
        CORRADE_COMPARE(&c.layer(), &layer);
        CORRADE_COMPARE(c.data(), dataHandle(layer.handle(), 0, 1));
        CORRADE_COMPARE(layer.usedCount(), 2);
        CORRADE_COMPARE(layer.usedScopedConnectionCount(), 2);
    }

    /* The instances should still remove themselves after all those moves */
    CORRADE_COMPARE(layer.usedCount(), 0);
    CORRADE_COMPARE(layer.usedScopedConnectionCount(), 0);
}

void EventLayerTest::eventConnectionDestructMovedOut() {
    Containers::Optional<EventConnection> connection;

    {
        EventLayer layer{layerHandle(137, 0xfe)};

        connection = layer.onTapOrClickScoped(NodeHandle::Null, []{});
        CORRADE_COMPARE(layer.usedCount(), 1);
        CORRADE_COMPARE(layer.usedScopedConnectionCount(), 1);

        EventConnection moved = Utility::move(*connection);
        CORRADE_COMPARE(&connection->layer(), &layer);
        CORRADE_COMPARE(connection->data(), DataHandle::Null);
    }

    /* The layer is still a dangling reference, but the data is null at this
       point so it shouldn't try to access the nonexistent layer during
       destruction */
    CORRADE_COMPARE(connection->data(), DataHandle::Null);
}

void EventLayerTest::eventConnectionRelease() {
    EventLayer layer{layerHandle(137, 0xfe)};

    EventConnection connection1 = layer.onTapOrClickScoped(NodeHandle::Null, []{});
    EventConnection connection2 = layer.onTapOrClickScoped(NodeHandle::Null, []{});
    CORRADE_COMPARE(layer.usedCount(), 2);
    CORRADE_COMPARE(layer.usedScopedConnectionCount(), 2);

    DataHandle handle = connection2.release();
    CORRADE_COMPARE(&connection2.layer(), &layer);
    CORRADE_COMPARE(connection2.data(), DataHandle::Null);
    CORRADE_VERIFY(layer.isHandleValid(handle));
    CORRADE_COMPARE(handle, dataHandle(layer.handle(), 1, 1));
    CORRADE_COMPARE(layer.usedCount(), 2);
    CORRADE_COMPARE(layer.usedScopedConnectionCount(), 1);
}

void EventLayerTest::eventConnectionReleaseMovedOut() {
    Containers::Optional<EventConnection> connection;

    {
        EventLayer layer{layerHandle(137, 0xfe)};

        connection = layer.onTapOrClickScoped(NodeHandle::Null, []{});
        CORRADE_COMPARE(layer.usedCount(), 1);
        CORRADE_COMPARE(layer.usedScopedConnectionCount(), 1);

        EventConnection moved = Utility::move(*connection);
        CORRADE_COMPARE(&connection->layer(), &layer);
        CORRADE_COMPARE(connection->data(), DataHandle::Null);
    }

    /* It doesn't need to decrement or update andthing in the layer so it
       should work also if the layer no longer exists */
    DataHandle handle = connection->release();
    CORRADE_COMPARE(connection->data(), DataHandle::Null);
    CORRADE_COMPARE(handle, DataHandle::Null);
}

void EventLayerTest::construct() {
    EventLayer layer{layerHandle(137, 0xfe)};
    CORRADE_COMPARE(layer.handle(), layerHandle(137, 0xfe));
    CORRADE_COMPARE(layer.usedScopedConnectionCount(), 0);
    CORRADE_COMPARE(layer.usedAllocatedConnectionCount(), 0);
    CORRADE_COMPARE(layer.dragThreshold(), 16.0f);
}

void EventLayerTest::constructCopy() {
    CORRADE_VERIFY(!std::is_copy_constructible<EventLayer>{});
    CORRADE_VERIFY(!std::is_copy_assignable<EventLayer>{});
}

void EventLayerTest::constructMove() {
    EventLayer a{layerHandle(137, 0xfe)};

    EventLayer b{Utility::move(a)};
    CORRADE_COMPARE(b.handle(), layerHandle(137, 0xfe));

    EventLayer c{layerHandle(0, 2)};
    c = Utility::move(b);
    CORRADE_COMPARE(c.handle(), layerHandle(137, 0xfe));

    /* Moving the (destructively) moved instance to another shouldn't crash due
       to _state access in asserts */
    EventLayer d{Utility::move(c)};
    /* In this case the moved-to _state is null */
    a = Utility::move(d);
    /* In this case the moved-from _state is null */
    a = Utility::move(d);

    CORRADE_VERIFY(std::is_nothrow_move_constructible<EventLayer>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<EventLayer>::value);
}

void EventLayerTest::constructMoveScopedConnectionsActive() {
    CORRADE_SKIP_IF_NO_ASSERT();

    {
        EventLayer a{layerHandle(137, 0xfe)};
        EventConnection connection1 = a.onTapOrClickScoped(NodeHandle::Null, []{});
        EventConnection connection2 = a.onTapOrClickScoped(NodeHandle::Null, []{});
        CORRADE_COMPARE(a.usedScopedConnectionCount(), 2);

        std::ostringstream out;
        Error redirectError{&out};
        EventLayer b{Utility::move(a)};
        CORRADE_COMPARE(out.str(), "Ui::EventLayer: 2 scoped connections already active, can't move\n");

        /* The connections would try to call isHandleValid() on a, which has an
           empty _state due to the destructive move and thus would crash. Put a
           new instance there to avoid that. */
        a = EventLayer{layerHandle(0, 2)};
    } {
        EventLayer a{layerHandle(137, 0xfe)};
        EventConnection connection1 = a.onTapOrClickScoped(NodeHandle::Null, []{});
        EventConnection connection2 = a.onTapOrClickScoped(NodeHandle::Null, []{});
        EventConnection connection3 = a.onTapOrClickScoped(NodeHandle::Null, []{});
        CORRADE_COMPARE(a.usedScopedConnectionCount(), 3);

        EventLayer b{layerHandle(0, 2)};
        std::ostringstream out;
        Error redirectError{&out};
        b = Utility::move(a);
        a = Utility::move(b);
        CORRADE_COMPARE(out.str(),
            "Ui::EventLayer: 3 scoped connections already active in the moved-from object, can't move\n"
            "Ui::EventLayer: 3 scoped connections already active in the moved-to object, can't move\n");

        /* As the move happened back and forth, there should be no null _state
           access here, unlike above */
    }
}

void EventLayerTest::destructScopedConnectionsActive() {
    CORRADE_SKIP_IF_NO_ASSERT();

    Containers::Optional<EventLayer> a{InPlaceInit, layerHandle(137, 0xfe)};
    EventConnection connection1 = a->onTapOrClickScoped(NodeHandle::Null, []{});
    EventConnection connection2 = a->onTapOrClickScoped(NodeHandle::Null, []{});
    CORRADE_COMPARE(a->usedScopedConnectionCount(), 2);

    std::ostringstream out;
    Error redirectError{&out};
    a = {};
    CORRADE_COMPARE(out.str(), "Ui::EventLayer: destructed with 2 scoped connections still active\n");

    /* The connections would try to call isHandleValid() on a, which is
        destructed and thus would crash. Put a new instance on the same
        address to avoid that. */
    a.emplace(layerHandle(0, 2));
}

void EventLayerTest::call() {
    Int functorCalledConstructedDestructedCount = 0;
    struct Functor {
        explicit Functor(int& output): calledConstructedDestructedCount{&output} {
            *calledConstructedDestructedCount += 100;
        }
        explicit Functor(const Functor& other) noexcept: calledConstructedDestructedCount{other.calledConstructedDestructedCount} {
            *calledConstructedDestructedCount += 1000;
        }
        /* Move assignment shouldn't happen, so Clang rightfully complains that
           this function is unused. On the other hand not having it would mean
           we can't test that it doesn't get called. */
        explicit CORRADE_UNUSED Functor(Functor&& other) noexcept: calledConstructedDestructedCount{other.calledConstructedDestructedCount} {
            /* A (non-const) reference is passed, so a copy should be made.
               Never a move. */
            CORRADE_FAIL("A move shouldn't happen");
        }
        ~Functor() {
            *calledConstructedDestructedCount += 10;
        }
        Functor& operator=(const Functor& other) noexcept {
            calledConstructedDestructedCount = other.calledConstructedDestructedCount;
            *calledConstructedDestructedCount += 10000;
            return *this;
        }
        /* Move assignment shouldn't happen, so Clang rightfully complains that
           this function is unused. On the other hand not having it would mean
           we can't test that it doesn't get called. */
        CORRADE_UNUSED Functor& operator=(Functor&& other) {
            calledConstructedDestructedCount = other.calledConstructedDestructedCount;
            CORRADE_FAIL("A move shouldn't happen");
            return *this;
        }
        void operator()() {
            *calledConstructedDestructedCount += 1;
        }

        Int* calledConstructedDestructedCount;
    };

    {
        /* This is not const in order to test that a move isn't used by
           accident */
        Functor functor{functorCalledConstructedDestructedCount};

        EventLayer layer{layerHandle(0, 1)};
        DataHandle handle = layer.onPress(NodeHandle::Null,  functor);
        /* Constructed a local instance (100) and copy-constructed it to the
           layer (1000) */
        CORRADE_COMPARE(functorCalledConstructedDestructedCount, 1100);
        CORRADE_COMPARE(handle, dataHandle(layer.handle(), 0, 1));
        CORRADE_COMPARE(layer.usedScopedConnectionCount(), 0);
        CORRADE_COMPARE(layer.usedAllocatedConnectionCount(), 1);

        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        layer.pointerPressEvent(0, event);
        /* Called it (1) */
        CORRADE_COMPARE(functorCalledConstructedDestructedCount, 1101);
    }

    /* Destructed the original instance and the copy in the layer (20) */
    CORRADE_COMPARE(functorCalledConstructedDestructedCount, 1121);
}

void EventLayerTest::invalidSlot() {
    CORRADE_SKIP_IF_NO_ASSERT();

    EventLayer layer{layerHandle(0, 1)};

    std::ostringstream out;
    Error redirectError{&out};
    layer.onBlur(nodeHandle(0, 1), nullptr);
    CORRADE_COMPARE(out.str(), "Ui::EventLayer: slot is null\n");
}

void EventLayerTest::connect() {
    auto&& data = ConnectData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    Int functorOutput = 1;

    {
        EventLayer layer{layerHandle(0x96, 0xef)};

        /* Some initial data to have non-trivial IDs */
        layer.onTapOrClick(nodeHandle(0, 1), []{});
        layer.onTapOrClick(nodeHandle(2, 3), []{});
        layer.onTapOrClick(nodeHandle(4, 5), []{});

        NodeHandle node = nodeHandle(137, 0xded);

        /* A functor temporary gets constructed inside, copied and destructed */
        DataHandle handle = data.functor(layer, node, functorOutput);
        CORRADE_COMPARE(functorOutput, 2*3*5);
        CORRADE_COMPARE(handle, dataHandle(layer.handle(), 3, 1));
        CORRADE_COMPARE(layer.node(handle), node);

        CORRADE_COMPARE(layer.usedCount(), 4);
        CORRADE_COMPARE(layer.usedScopedConnectionCount(), 0);
        CORRADE_COMPARE(layer.usedAllocatedConnectionCount(), 1);

        /* THe functor gets called */
        data.call(layer, 3);
        CORRADE_COMPARE(functorOutput, 2*3*5*7);
    }

    /* The functor copy gets destructed after */
    CORRADE_COMPARE(functorOutput, 2*3*5*7*5);
}

void EventLayerTest::connectScoped() {
    auto&& data = ConnectData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    Int functorOutput = 1;

    EventLayer layer{layerHandle(0x96, 0xef)};

    /* Some initial data to have non-trivial IDs */
    layer.onTapOrClick(nodeHandle(0, 1), []{});
    layer.onTapOrClick(nodeHandle(2, 3), []{});
    layer.onTapOrClick(nodeHandle(4, 5), []{});

    NodeHandle node = nodeHandle(137, 0xded);

    /* Capture correct function name, in case the functor itself fails */
    CORRADE_VERIFY(true);

    {
        /* A functor temporary gets constructed inside, copied and destructed */
        EventConnection connection = data.functorScoped(layer, node, functorOutput);
        CORRADE_COMPARE(functorOutput, 2*3*5);
        CORRADE_COMPARE(&connection.layer(), &layer);
        CORRADE_COMPARE(connection.data(), dataHandle(layer.handle(), 3, 1));
        CORRADE_COMPARE(layer.node(connection.data()), node);

        CORRADE_COMPARE(layer.usedCount(), 4);
        CORRADE_COMPARE(layer.usedScopedConnectionCount(), 1);
        CORRADE_COMPARE(layer.usedAllocatedConnectionCount(), 1);

        /* THe functor gets called */
        data.call(layer, 3);
        CORRADE_COMPARE(functorOutput, 2*3*5*7);
    }

    CORRADE_COMPARE(layer.usedCount(), 3);
    CORRADE_COMPARE(layer.usedScopedConnectionCount(), 0);
    CORRADE_COMPARE(layer.usedAllocatedConnectionCount(), 0);

    /* The functor copy gets destructed after */
    CORRADE_COMPARE(functorOutput, 2*3*5*7*5);
}

void EventLayerTest::remove() {
    Int destructedCount = 0;
    struct NonTrivial {
        explicit NonTrivial(int& output): destructedCount{&output} {}
        ~NonTrivial() {
            ++*destructedCount;
        }
        void operator()() const {}

        Int* destructedCount;
    };

    EventLayer layer{layerHandle(0, 1)};

    DataHandle trivial = layer.onTapOrClick(nodeHandle(0, 1), []{});
    CORRADE_COMPARE(layer.usedCount(), 1);
    CORRADE_COMPARE(layer.usedScopedConnectionCount(), 0);
    CORRADE_COMPARE(layer.usedAllocatedConnectionCount(), 0);

    /* The temporary gets destructed right away */
    DataHandle nonTrivial = layer.onTapOrClick(nodeHandle(1, 2), NonTrivial{destructedCount});
    CORRADE_COMPARE(layer.usedCount(), 2);
    CORRADE_COMPARE(layer.usedScopedConnectionCount(), 0);
    CORRADE_COMPARE(layer.usedAllocatedConnectionCount(), 1);
    CORRADE_COMPARE(destructedCount, 1);

    layer.remove(trivial);
    CORRADE_COMPARE(layer.usedCount(), 1);
    CORRADE_COMPARE(layer.usedScopedConnectionCount(), 0);
    CORRADE_COMPARE(layer.usedAllocatedConnectionCount(), 1);
    CORRADE_COMPARE(destructedCount, 1);

    /* Verifying also the other handle overload. They should both delegate into
       the same internal implementation. */
    layer.remove(dataHandleData(nonTrivial));
    CORRADE_COMPARE(layer.usedCount(), 0);
    CORRADE_COMPARE(layer.usedScopedConnectionCount(), 0);
    CORRADE_COMPARE(layer.usedAllocatedConnectionCount(), 0);
    CORRADE_COMPARE(destructedCount, 2);
}

void EventLayerTest::removeScoped() {
    Int destructedCount = 0;
    struct NonTrivial {
        explicit NonTrivial(int& output): destructedCount{&output} {}
        ~NonTrivial() {
            ++*destructedCount;
        }
        void operator()() const {}

        Int* destructedCount;
    };

    EventLayer layer{layerHandle(0, 1)};
    {
        EventConnection trivial = layer.onTapOrClickScoped(nodeHandle(0, 1), []{});
        CORRADE_COMPARE(layer.usedCount(), 1);
        CORRADE_COMPARE(layer.usedScopedConnectionCount(), 1);
        CORRADE_COMPARE(layer.usedAllocatedConnectionCount(), 0);

        /* The temporary gets destructed right away */
        EventConnection nonTrivial = layer.onTapOrClickScoped(nodeHandle(1, 2), NonTrivial{destructedCount});
        CORRADE_COMPARE(layer.usedCount(), 2);
        CORRADE_COMPARE(layer.usedScopedConnectionCount(), 2);
        CORRADE_COMPARE(layer.usedAllocatedConnectionCount(), 1);
        CORRADE_COMPARE(destructedCount, 1);

        layer.remove(trivial.data());
        CORRADE_COMPARE(layer.usedCount(), 1);
        CORRADE_COMPARE(layer.usedScopedConnectionCount(), 1);
        CORRADE_COMPARE(layer.usedAllocatedConnectionCount(), 1);
        CORRADE_COMPARE(destructedCount, 1);

        layer.remove(nonTrivial.data());
        CORRADE_COMPARE(layer.usedCount(), 0);
        CORRADE_COMPARE(layer.usedScopedConnectionCount(), 0);
        CORRADE_COMPARE(layer.usedAllocatedConnectionCount(), 0);
        CORRADE_COMPARE(destructedCount, 2);

        /* The EventConnection instances should not attempt to delete the same
           data again */
    }
}

void EventLayerTest::connectRemoveHandleRecycle() {
    Int destructedCount1 = 0,
        destructedCount2 = 0;
    struct NonTrivial {
        explicit NonTrivial(int& output): destructedCount{&output} {}
        ~NonTrivial() {
            ++*destructedCount;
        }
        void operator()() const {
            CORRADE_FAIL("This should never be called.");
        }

        Int* destructedCount;
    };

    EventLayer layer{layerHandle(0, 1)};
    layer.onTapOrClick(nodeHandle(1, 2), []{});

    /* The temporary gets destructed right away */
    DataHandle second = layer.onTapOrClick(nodeHandle(1, 2), NonTrivial{destructedCount1});
    CORRADE_COMPARE(destructedCount1, 1);

    layer.remove(second);
    CORRADE_COMPARE(destructedCount1, 2);

    /* Data that reuses a previous slot should not call the destructor on the
       previous function again or some such crazy stuff */
    DataHandle second2 = layer.onTapOrClick(nodeHandle(3, 4), NonTrivial{destructedCount2});
    CORRADE_COMPARE(dataHandleId(second2), dataHandleId(second));
    CORRADE_COMPARE(destructedCount1, 2);
    CORRADE_COMPARE(destructedCount2, 1);
}

void EventLayerTest::cleanNodes() {
    Int destructedCount = 0;
    Int anotherDestructedCount = 0;
    struct NonTrivial {
        explicit NonTrivial(int& output): destructedCount{&output} {}
        ~NonTrivial() {
            ++*destructedCount;
        }
        void operator()() const {}

        Int* destructedCount;
    };

    EventLayer layer{layerHandle(0, 1)};

    DataHandle trivial = layer.onTapOrClick(nodeHandle(1, 2), []{});
    CORRADE_COMPARE(layer.usedCount(), 1);
    CORRADE_COMPARE(layer.usedAllocatedConnectionCount(), 0);

    /* The temporary gets destructed right away */
    DataHandle nonTrivial = layer.onTapOrClick(nodeHandle(3, 4), NonTrivial{destructedCount});
    CORRADE_COMPARE(layer.usedCount(), 2);
    CORRADE_COMPARE(layer.usedAllocatedConnectionCount(), 1);
    CORRADE_COMPARE(destructedCount, 1);

    DataHandle another = layer.onTapOrClick(nodeHandle(0, 5), []{});
    CORRADE_COMPARE(layer.usedCount(), 3);
    CORRADE_COMPARE(layer.usedAllocatedConnectionCount(), 1);

    /* The temporary gets destructed right away */
    DataHandle anotherNonTrivial = layer.onTapOrClick(nodeHandle(4, 1), NonTrivial{anotherDestructedCount});
    CORRADE_COMPARE(layer.usedCount(), 4);
    CORRADE_COMPARE(layer.usedAllocatedConnectionCount(), 2);
    CORRADE_COMPARE(anotherDestructedCount, 1);

    UnsignedShort nodeHandleGenerations[]{
        5,      /* node 0 with `another` stays */
        1,      /* node 1 has generation = 2, so it gets deleted */
        666,    /* node 2 isn't used */
        5,      /* node 3 has generation = 4, so it gets deleted too */
        1,      /* node 4 with `anotherNonTrivial` stays too */
    };
    layer.cleanNodes(nodeHandleGenerations);

    /* It should remove two but call just one destructor */
    CORRADE_COMPARE(layer.usedCount(), 2);
    CORRADE_COMPARE(layer.usedAllocatedConnectionCount(), 1);
    CORRADE_COMPARE(destructedCount, 2);
    CORRADE_COMPARE(anotherDestructedCount, 1);
    CORRADE_VERIFY(!layer.isHandleValid(trivial));
    CORRADE_VERIFY(!layer.isHandleValid(nonTrivial));
    CORRADE_VERIFY(layer.isHandleValid(another));
    CORRADE_VERIFY(layer.isHandleValid(anotherNonTrivial));
}

void EventLayerTest::press() {
    EventLayer layer{layerHandle(0, 1)};

    Int called = 0;
    DataHandle handle = layer.onPress(nodeHandle(0, 1), [&called]{
        ++called;
    });

    /* Should only get fired for mouse left, *primary* finger or pen */
    {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseMiddle, true, 0};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseRight, true, 0};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        PointerEvent event{{}, PointerEventSource::Touch, Pointer::Finger, true, 0};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 2);
    } {
        PointerEvent event{{}, PointerEventSource::Touch, Pointer::Finger, false, 0};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 2);
    } {
        PointerEvent event{{}, PointerEventSource::Pen, Pointer::Pen, true, 0};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 3);
    } {
        PointerEvent event{{}, PointerEventSource::Pen, Pointer::Eraser, true, 0};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 3);

    /* Shouldn't get fired for any other than press events */
    } {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 3);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, Pointer::MouseLeft, true, 0};
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 3);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, Pointer::MouseLeft, true, 0};
        layer.pointerEnterEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 3);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, Pointer::MouseLeft, true, 0};
        layer.pointerLeaveEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 3);
    } {
        FocusEvent event{{}};
        layer.focusEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 3);
    } {
        FocusEvent event{{}};
        layer.blurEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 3);
    }
}

void EventLayerTest::release() {
    EventLayer layer{layerHandle(0, 1)};

    Int called = 0;
    DataHandle handle = layer.onRelease(nodeHandle(0, 1), [&called]{
        ++called;
    });

    /* Should only get fired for mouse left, *primary* finger or pen */
    {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseMiddle, true, 0};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseRight, true, 0};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        PointerEvent event{{}, PointerEventSource::Touch, Pointer::Finger, true, 0};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 2);
    } {
        PointerEvent event{{}, PointerEventSource::Touch, Pointer::Finger, false, 0};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 2);
    } {
        PointerEvent event{{}, PointerEventSource::Pen, Pointer::Pen, true, 0};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 3);
    } {
        PointerEvent event{{}, PointerEventSource::Pen, Pointer::Eraser, true, 0};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 3);

    /* Shouldn't get fired for any other than release events */
    } {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 3);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, Pointer::MouseLeft, true, 0};
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 3);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, Pointer::MouseLeft, true, 0};
        layer.pointerEnterEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 3);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, Pointer::MouseLeft, true, 0};
        layer.pointerLeaveEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 3);
    } {
        FocusEvent event{{}};
        layer.focusEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 3);
    } {
        FocusEvent event{{}};
        layer.blurEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 3);
    }
}

void EventLayerTest::releasePress() {
    EventLayer layer{layerHandle(0, 1)};

    Int called = 0;
    DataHandle handle = layer.onRelease(nodeHandle(0, 1), [&called]{
        ++called;
    });

    /* Unlike onTapOrClick() etc, the press event shouldn't get implicitly
       accepted -- it's up to the node on which a press was called to decide
       whether it should capture the event (and thus make release happen there
       as well) or not (and make release happen at whatever node is under
       pointer at the time) */
    {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseMiddle, true, 0};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseRight, true, 0};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{{}, PointerEventSource::Touch, Pointer::Finger, true, 0};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{{}, PointerEventSource::Pen, Pointer::Pen, true, 0};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{{}, PointerEventSource::Pen, Pointer::Eraser, true, 0};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);

    /* Any other than release events shouldn't get accepted either */
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, Pointer::MouseLeft, true, 0};
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, Pointer::MouseLeft, true, 0};
        layer.pointerEnterEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, Pointer::MouseLeft, true, 0};
        layer.pointerLeaveEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        FocusEvent event{{}};
        layer.focusEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        FocusEvent event{{}};
        layer.blurEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);

    /* Verify that the callback is actually properly registered so this doesn't
       result in false positives */
    } {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(event.isAccepted());
        CORRADE_COMPARE(called, 1);
    }
}

void EventLayerTest::pressReleaseFromUserInterface() {
    auto&& data = FromUserInterfaceData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* "Integration" test to verify onPress() and onRelease() behavior with the
       whole event pipeline in AbstractUserInterface.

       There's no mutual interaction between the two as with onTapOrClick()
       such as onRelease() accepting presses as well, so they're both tested
       together. */

    AbstractUserInterface ui{{100, 100}};

    EventLayer& layer = ui.setLayerInstance(Containers::pointer<EventLayer>(ui.createLayer()));

    /* A node below the one that should react to the tap or click event,
       accepting presses. Shouldn't get considered at all. */
    Int belowCalled = 0;
    NodeHandle nodeBelow = ui.createNode({}, {100, 100}, data.flags);
    layer.onPress(nodeBelow, [&belowCalled]{
        ++belowCalled;
    });
    layer.onRelease(nodeBelow, [&belowCalled]{
        ++belowCalled;
    });

    Int pressCalled = 0, pressPositionCalled = 0,
        releaseCalled = 0, releasePositionCalled = 0;
    NodeHandle node = ui.createNode(
        data.parent ? nodeBelow : NodeHandle::Null,
        {25, 50}, {50, 25});
    layer.onPress(node, [&pressCalled]{
        ++pressCalled;
    });
    layer.onPress(node, [&pressPositionCalled](const Vector2& position) {
        CORRADE_COMPARE(position, (Vector2{25.0f, 20.0f}));
        ++pressPositionCalled;
    });
    layer.onRelease(node, [&releaseCalled]{
        ++releaseCalled;
    });
    layer.onRelease(node, [&releasePositionCalled](const Vector2& position) {
        CORRADE_COMPARE(position, (Vector2{25.0f, 15.0f}));
        ++releasePositionCalled;
    });

    /* A press should be accepted but not resulting in the handler being
       called */
    {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        CORRADE_VERIFY(ui.pointerPressEvent({50, 70}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), node);
        CORRADE_COMPARE(ui.currentCapturedNode(), node);
        CORRADE_COMPARE(pressCalled, 1);
        CORRADE_COMPARE(pressPositionCalled, 1);
        CORRADE_COMPARE(releaseCalled, 0);
        CORRADE_COMPARE(releasePositionCalled, 0);
        CORRADE_COMPARE(belowCalled, 0);

    /* A release should be accepted as well, resulting in the handler being
       called */
    } {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        CORRADE_VERIFY(ui.pointerReleaseEvent({50, 65}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(pressCalled, 1);
        CORRADE_COMPARE(pressPositionCalled, 1);
        CORRADE_COMPARE(releaseCalled, 1);
        CORRADE_COMPARE(releasePositionCalled, 1);
        CORRADE_COMPARE(belowCalled, 0);
    }
}

void EventLayerTest::tapOrClick() {
    EventLayer layer{layerHandle(0, 1)};

    Int called = 0;
    DataHandle handle = layer.onTapOrClick(nodeHandle(0, 1), [&called]{
        ++called;
    });

    /* Should only get fired for mouse left, (primary) finger or pen release
       that's inside of a pressed node (yes this uses the horrific testing-only
       constructor) */
    {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0, {0.5f, 0.5f}, true, {1.0f, 1.0f}};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        /* Not pressed */
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0, {0.5f, 0.5f}, false, {1.0f, 1.0f}};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        /* Wrong button */
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseMiddle, true, 0, {0.5f, 0.5f}, true, {1.0f, 1.0f}};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        /* Wrong button */
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseRight, true, 0, {0.5f, 0.5f}, true, {1.0f, 1.0f}};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        PointerEvent event{{}, PointerEventSource::Touch, Pointer::Finger, true, 37, {0.5f, 0.5f}, true, {1.0f, 1.0f}};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 2);
    } {
        /* Secondary finger */
        PointerEvent event{{}, PointerEventSource::Touch, Pointer::Finger, false, 37, {0.5f, 0.5f}, true, {1.0f, 1.0f}};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 2);
    } {
        PointerEvent event{{}, PointerEventSource::Pen, Pointer::Pen, true, 0, {0.5f, 0.5f}, true, {1.0f, 1.0f}};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 3);
    } {
        /* Wrong button */
        PointerEvent event{{}, PointerEventSource::Pen, Pointer::Eraser, true, 0, {0.5f, 0.5f}, true, {1.0f, 1.0f}};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 3);

    /* Shouldn't get fired for any other events than release */
    } {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 3);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, Pointer::MouseLeft, true, 0};
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 3);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, Pointer::MouseLeft, true, 0};
        layer.pointerEnterEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 3);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, Pointer::MouseLeft, true, 0};
        layer.pointerLeaveEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 3);
    } {
        FocusEvent event{{}};
        layer.focusEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 3);
    } {
        FocusEvent event{{}};
        layer.blurEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 3);
    }
}

void EventLayerTest::tapOrClickPress() {
    EventLayer layer{layerHandle(0, 1)};

    Int called = 0;
    DataHandle handle = layer.onTapOrClick(nodeHandle(0, 1), [&called]{
        ++called;
    });

    /* The press event should get accepted for mouse left, *primary* finger or
       pen to prevent it from being propagated further if no other data accepts
       it. The handler shouldn't get called though. */
    {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseMiddle, true, 0};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseRight, true, 0};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{{}, PointerEventSource::Touch, Pointer::Finger, true, 0};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{{}, PointerEventSource::Touch, Pointer::Finger, false, 0};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{{}, PointerEventSource::Pen, Pointer::Pen, true, 0};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{{}, PointerEventSource::Pen, Pointer::Eraser, true, 0};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);

    /* Any other than press, release or tapOrClick event shouldn't get
       accepted */
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, Pointer::MouseLeft, true, 0};
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, Pointer::MouseLeft, true, 0};
        layer.pointerEnterEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, Pointer::MouseLeft, true, 0};
        layer.pointerLeaveEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        FocusEvent event{{}};
        layer.focusEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        FocusEvent event{{}};
        layer.blurEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);

    /* Verify that the callback is actually properly registered so this doesn't
       result in false positives */
    } {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0, {}, true, {1.0f, 1.0f}};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(event.isAccepted());
        CORRADE_COMPARE(called, 1);
    }
}

void EventLayerTest::tapOrClickFromUserInterface() {
    auto&& data = FromUserInterfaceData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* "Integration" test to verify onTapOrClick() behavior with the whole
       event pipeline in AbstractUserInterface */

    AbstractUserInterface ui{{100, 100}};

    EventLayer& layer = ui.setLayerInstance(Containers::pointer<EventLayer>(ui.createLayer()));

    /* A node below the one that should react to the tap or click event,
       accepting presses. Shouldn't get considered at all. */
    Int belowCalled = 0;
    NodeHandle nodeBelow = ui.createNode({}, {100, 100}, data.flags);
    layer.onPress(nodeBelow, [&belowCalled]{
        ++belowCalled;
    });

    Int called = 0, positionCalled = 0;
    NodeHandle node = ui.createNode(
        data.parent ? nodeBelow : NodeHandle::Null,
        {25, 50}, {50, 25});
    layer.onTapOrClick(node, [&called]{
        ++called;
    });
    layer.onTapOrClick(node, [&positionCalled](const Vector2& position) {
        CORRADE_COMPARE(position, (Vector2{25.0f, 15.0f}));
        ++positionCalled;
    });

    /* A press should be accepted but not resulting in the handler being
       called */
    {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        CORRADE_VERIFY(ui.pointerPressEvent({50, 70}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), node);
        CORRADE_COMPARE(ui.currentCapturedNode(), node);
        CORRADE_COMPARE(called, 0);
        CORRADE_COMPARE(positionCalled, 0);
        CORRADE_COMPARE(belowCalled, 0);

    /* A release should be accepted as well, resulting in the handler being
       called */
    } {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        CORRADE_VERIFY(ui.pointerReleaseEvent({50, 65}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(called, 1);
        CORRADE_COMPARE(positionCalled, 1);
        CORRADE_COMPARE(belowCalled, 0);

    /* The press and release pointer type or source doesn't have to match,
       currently */
    } {
        PointerEvent press{{}, PointerEventSource::Pen, Pointer::Pen, true, 0};
        PointerEvent release{{}, PointerEventSource::Touch, Pointer::Finger, true, 0};
        CORRADE_VERIFY(ui.pointerPressEvent({50, 70}, press));
        CORRADE_VERIFY(ui.pointerReleaseEvent({50, 65}, release));
        CORRADE_COMPARE(called, 2);
        CORRADE_COMPARE(positionCalled, 2);
        CORRADE_COMPARE(belowCalled, 0);

    /* Press with release outside shouldn't cause a tap or click */
    } {
        PointerEvent press{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        PointerEvent release{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        CORRADE_VERIFY(ui.pointerPressEvent({50, 70}, press));
        CORRADE_COMPARE(ui.currentPressedNode(), node);
        CORRADE_COMPARE(ui.currentCapturedNode(), node);
        CORRADE_VERIFY(!ui.pointerReleaseEvent({100, 65}, release));
        CORRADE_COMPARE(called, 2);
        CORRADE_COMPARE(positionCalled, 2);
        CORRADE_COMPARE(belowCalled, 0);
    }
}

void EventLayerTest::middleClick() {
    EventLayer layer{layerHandle(0, 1)};

    Int called = 0;
    DataHandle handle = layer.onMiddleClick(nodeHandle(0, 1), [&called]{
        ++called;
    });

    /* Should only get fired for mouse middle release that's inside of a
       pressed node (yes this uses the horrific testing-only constructor) */
    {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseMiddle, true, 0, {0.5f, 0.5f}, true, {1.0f, 1.0f}};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        /* Not pressed */
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseMiddle, true, 0, {0.5f, 0.5f}, false, {1.0f, 1.0f}};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        /* Wrong button */
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0, {0.5f, 0.5f}, true, {1.0f, 1.0f}};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        /* Wrong button */
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseRight, true, 0, {0.5f, 0.5f}, true, {1.0f, 1.0f}};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        /* Wrong source */
        PointerEvent event{{}, PointerEventSource::Touch, Pointer::Finger, true, 37, {0.5f, 0.5f}, true, {1.0f, 1.0f}};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        /* Wrong source */
        PointerEvent event{{}, PointerEventSource::Pen, Pointer::Pen, true, 0, {0.5f, 0.5f}, true, {1.0f, 1.0f}};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        /* Wrong source */
        PointerEvent event{{}, PointerEventSource::Pen, Pointer::Eraser, true, 0, {0.5f, 0.5f}, true, {1.0f, 1.0f}};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);

    /* Shouldn't get fired for any other events than release */
    } {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseMiddle, true, 0};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, Pointer::MouseMiddle, Pointer::MouseMiddle, true, 0};
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, Pointer::MouseMiddle, Pointer::MouseMiddle, true, 0};
        layer.pointerEnterEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, Pointer::MouseMiddle, Pointer::MouseMiddle, true, 0};
        layer.pointerLeaveEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        FocusEvent event{{}};
        layer.focusEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        FocusEvent event{{}};
        layer.blurEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    }
}

void EventLayerTest::middleClickPress() {
    EventLayer layer{layerHandle(0, 1)};

    Int called = 0;
    DataHandle handle = layer.onMiddleClick(nodeHandle(0, 1), [&called]{
        ++called;
    });

    /* The press event should get accepted for mouse middle to prevent it from
       being propagated further if no other data accepts it. The handler
       shouldn't get called though. */
    {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseMiddle, true, 0};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseRight, true, 0};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{{}, PointerEventSource::Touch, Pointer::Finger, true, 0};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{{}, PointerEventSource::Pen, Pointer::Pen, true, 0};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{{}, PointerEventSource::Pen, Pointer::Eraser, true, 0};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);

    /* Any other than press, release or tapOrClick event shouldn't get
       accepted */
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, Pointer::MouseMiddle, Pointer::MouseMiddle, true, 0};
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, Pointer::MouseMiddle, Pointer::MouseMiddle, true, 0};
        layer.pointerEnterEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, Pointer::MouseMiddle, Pointer::MouseMiddle, true, 0};
        layer.pointerLeaveEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        FocusEvent event{{}};
        layer.focusEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        FocusEvent event{{}};
        layer.blurEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);

    /* Verify that the callback is actually properly registered so this doesn't
       result in false positives */
    } {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseMiddle, true, 0, {}, true, {1.0f, 1.0f}};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(event.isAccepted());
        CORRADE_COMPARE(called, 1);
    }
}

void EventLayerTest::middleClickFromUserInterface() {
    auto&& data = FromUserInterfaceData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* "Integration" test to verify onTapOrClick() behavior with the whole
       event pipeline in AbstractUserInterface */

    AbstractUserInterface ui{{100, 100}};

    EventLayer& layer = ui.setLayerInstance(Containers::pointer<EventLayer>(ui.createLayer()));

    /* A node below the one that should react to the middle click event,
       accepting presses. Shouldn't get considered at all. */
    Int belowCalled = 0;
    NodeHandle nodeBelow = ui.createNode({}, {100, 100}, data.flags);
    layer.onPress(nodeBelow, [&belowCalled]{
        ++belowCalled;
    });

    Int called = 0, positionCalled = 0;
    NodeHandle node = ui.createNode(
        data.parent ? nodeBelow : NodeHandle::Null,
        {25, 50}, {50, 25});
    layer.onMiddleClick(node, [&called]{
        ++called;
    });
    layer.onMiddleClick(node, [&positionCalled](const Vector2& position) {
        CORRADE_COMPARE(position, (Vector2{25.0f, 15.0f}));
        ++positionCalled;
    });

    /* A press should be accepted but not resulting in the handler being
       called */
    {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseMiddle, true, 0};
        CORRADE_VERIFY(ui.pointerPressEvent({50, 70}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), node);
        CORRADE_COMPARE(ui.currentCapturedNode(), node);
        CORRADE_COMPARE(called, 0);
        CORRADE_COMPARE(positionCalled, 0);
        CORRADE_COMPARE(belowCalled, 0);

    /* A release should be accepted as well, resulting in the handler being
       called */
    } {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseMiddle, true, 0};
        CORRADE_VERIFY(ui.pointerReleaseEvent({50, 65}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(called, 1);
        CORRADE_COMPARE(positionCalled, 1);
        CORRADE_COMPARE(belowCalled, 0);

    /* Press with release outside shouldn't cause a tap or click */
    } {
        PointerEvent press{{}, PointerEventSource::Mouse, Pointer::MouseMiddle, true, 0};
        PointerEvent release{{}, PointerEventSource::Mouse, Pointer::MouseMiddle, true, 0};
        CORRADE_VERIFY(ui.pointerPressEvent({50, 70}, press));
        CORRADE_COMPARE(ui.currentPressedNode(), node);
        CORRADE_COMPARE(ui.currentCapturedNode(), node);
        CORRADE_VERIFY(!ui.pointerReleaseEvent({100, 65}, release));
        CORRADE_COMPARE(called, 1);
        CORRADE_COMPARE(positionCalled, 1);
        CORRADE_COMPARE(belowCalled, 0);
    }
}

void EventLayerTest::rightClick() {
    EventLayer layer{layerHandle(0, 1)};

    Int called = 0;
    DataHandle handle = layer.onRightClick(nodeHandle(0, 1), [&called]{
        ++called;
    });

    /* Should only get fired for mouse middle release that's inside of a
       pressed node (yes this uses the horrific testing-only constructor) */
    {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseRight, true, 0, {0.5f, 0.5f}, true, {1.0f, 1.0f}};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        /* Not pressed */
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseRight, true, 0, {0.5f, 0.5f}, false, {1.0f, 1.0f}};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        /* Wrong button */
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0, {0.5f, 0.5f}, true, {1.0f, 1.0f}};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        /* Wrong button */
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseMiddle, true, 0, {0.5f, 0.5f}, true, {1.0f, 1.0f}};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        /* Wrong source */
        PointerEvent event{{}, PointerEventSource::Touch, Pointer::Finger, true, 37, {0.5f, 0.5f}, true, {1.0f, 1.0f}};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        /* Wrong source */
        PointerEvent event{{}, PointerEventSource::Pen, Pointer::Pen, true, 0, {0.5f, 0.5f}, true, {1.0f, 1.0f}};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        /* Wrong source */
        PointerEvent event{{}, PointerEventSource::Pen, Pointer::Eraser, true, 0, {0.5f, 0.5f}, true, {1.0f, 1.0f}};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);

    /* Shouldn't get fired for any other events than release */
    } {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseRight, true, 0};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseRight, true, 0};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, Pointer::MouseRight, Pointer::MouseRight, true, 0};
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, Pointer::MouseRight, Pointer::MouseRight, true, 0};
        layer.pointerEnterEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, Pointer::MouseRight, Pointer::MouseRight, true, 0};
        layer.pointerLeaveEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        FocusEvent event{{}};
        layer.focusEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        FocusEvent event{{}};
        layer.blurEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    }
}

void EventLayerTest::rightClickPress() {
    EventLayer layer{layerHandle(0, 1)};

    Int called = 0;
    DataHandle handle = layer.onRightClick(nodeHandle(0, 1), [&called]{
        ++called;
    });

    /* The press event should get accepted for mouse right to prevent it from
       being propagated further if no other data accepts it. The handler
       shouldn't get called though. */
    {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseMiddle, true, 0};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseRight, true, 0};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{{}, PointerEventSource::Touch, Pointer::Finger, true, 0};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{{}, PointerEventSource::Pen, Pointer::Pen, true, 0};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{{}, PointerEventSource::Pen, Pointer::Eraser, true, 0};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);

    /* Any other than press, release or tapOrClick event shouldn't get
       accepted */
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, Pointer::MouseRight, Pointer::MouseRight, true, 0};
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, Pointer::MouseRight, Pointer::MouseRight, true, 0};
        layer.pointerEnterEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, Pointer::MouseRight, Pointer::MouseRight, true, 0};
        layer.pointerLeaveEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        FocusEvent event{{}};
        layer.focusEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        FocusEvent event{{}};
        layer.blurEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);

    /* Verify that the callback is actually properly registered so this doesn't
       result in false positives */
    } {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseRight, true, 0, {}, true, {1.0f, 1.0f}};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(event.isAccepted());
        CORRADE_COMPARE(called, 1);
    }
}

void EventLayerTest::rightClickFromUserInterface() {
    auto&& data = FromUserInterfaceData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* "Integration" test to verify onTapOrClick() behavior with the whole
       event pipeline in AbstractUserInterface */

    AbstractUserInterface ui{{100, 100}};

    EventLayer& layer = ui.setLayerInstance(Containers::pointer<EventLayer>(ui.createLayer()));

    /* A node below the one that should react to the right click event,
       accepting presses. Shouldn't get considered at all. */
    Int belowCalled = 0;
    NodeHandle nodeBelow = ui.createNode({}, {100, 100}, data.flags);
    layer.onPress(nodeBelow, [&belowCalled]{
        ++belowCalled;
    });

    Int called = 0, positionCalled = 0;
    NodeHandle node = ui.createNode(
        data.parent ? nodeBelow : NodeHandle::Null,
        {25, 50}, {50, 25});
    layer.onRightClick(node, [&called]{
        ++called;
    });
    layer.onRightClick(node, [&positionCalled](const Vector2& position) {
        CORRADE_COMPARE(position, (Vector2{25.0f, 15.0f}));
        ++positionCalled;
    });

    /* A press should be accepted but not resulting in the handler being
       called */
    {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseRight, true, 0};
        CORRADE_VERIFY(ui.pointerPressEvent({50, 70}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), node);
        CORRADE_COMPARE(ui.currentCapturedNode(), node);
        CORRADE_COMPARE(called, 0);
        CORRADE_COMPARE(positionCalled, 0);
        CORRADE_COMPARE(belowCalled, 0);

    /* A release should be accepted as well, resulting in the handler being
       called */
    } {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseRight, true, 0};
        CORRADE_VERIFY(ui.pointerReleaseEvent({50, 65}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(called, 1);
        CORRADE_COMPARE(positionCalled, 1);
        CORRADE_COMPARE(belowCalled, 0);

    /* Press with release outside shouldn't cause a tap or click */
    } {
        PointerEvent press{{}, PointerEventSource::Mouse, Pointer::MouseRight, true, 0};
        PointerEvent release{{}, PointerEventSource::Mouse, Pointer::MouseRight, true, 0};
        CORRADE_VERIFY(ui.pointerPressEvent({50, 70}, press));
        CORRADE_COMPARE(ui.currentPressedNode(), node);
        CORRADE_COMPARE(ui.currentCapturedNode(), node);
        CORRADE_VERIFY(!ui.pointerReleaseEvent({100, 65}, release));
        CORRADE_COMPARE(called, 1);
        CORRADE_COMPARE(positionCalled, 1);
        CORRADE_COMPARE(belowCalled, 0);
    }
}

void EventLayerTest::tapOrClickMiddleClickRightClickEdges() {
    auto&& data = TapOrClickMiddleClickRightClickEdgesData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    EventLayer layer{layerHandle(0, 1)};

    Int called = 0;
    DataHandle handle = (layer.*data.call)(nodeHandle(0, 1), [&called]{
        ++called;
    });

    {
        /* Top left */
        PointerEvent event{{}, data.source, data.pointer, true, 0, {0.0f, 0.0f}, true, {1.0f, 1.0f}};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        /* Outside on the top left */
        PointerEvent event{{}, data.source, data.pointer, true, 0, {-0.1f, -0.1f}, true, {1.0f, 1.0f}};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        /* Top right */
        PointerEvent event{{}, data.source, data.pointer, true, 0, {0.0f, 0.9f}, true, {1.0f, 1.0f}};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 2);
    } {
        /* Outside on the top right */
        PointerEvent event{{}, data.source, data.pointer, true, 0, {0.0f, 1.0f}, true, {1.0f, 1.0f}};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 2);
    } {
        /* Bottom left */
        PointerEvent event{{}, data.source, data.pointer, true, 0, {0.9f, 0.0f}, true, {1.0f, 1.0f}};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 3);
    } {
        /* Outside on the bottom left */
        PointerEvent event{{}, data.source, data.pointer, true, 0, {1.0f, 0.0f}, true, {1.0f, 1.0f}};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 3);
    } {
        /* Bottom right */
        PointerEvent event{{}, data.source, data.pointer, true, 0, {0.9f, 0.9f}, true, {1.0f, 1.0f}};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 4);
    } {
        /* Outside on the Bottom right */
        PointerEvent event{{}, data.source, data.pointer, true, 0, {1.0f, 1.0f}, true, {1.0f, 1.0f}};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 4);
    } {
        /* Outside on the left */
        PointerEvent event{{}, data.source, data.pointer, true, 0, {-0.1f, 0.5f}, true, {1.0f, 1.0f}};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 4);
    } {
        /* Outside on the top */
        PointerEvent event{{}, data.source, data.pointer, true, 0, {0.5f, -0.1f}, true, {1.0f, 1.0f}};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 4);
    } {
        /* Outside on the right */
        PointerEvent event{{}, data.source, data.pointer, true, 0, {0.5f, 0.5f}, true, {0.5f, 1.0f}};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 4);
    } {
        /* Outside on the bottom */
        PointerEvent event{{}, data.source, data.pointer, true, 0, {0.5f, 0.5f}, true, {1.0f, 0.5f}};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 4);
    }
}

void EventLayerTest::drag() {
    EventLayer layer{layerHandle(0, 1)};

    Int called = 0;
    Vector2 calledOffset;
    DataHandle handle = layer.onDrag(nodeHandle(0, 1), [&called, &calledOffset](const Vector2& offset) {
        ++called;
        calledOffset += offset;
    });

    /* Should only get fired for a move with mouse left, *primary* finger or
       pen present among pointers() and only if the event is captured (i.e.,
       the drag not coming from outside of the UI). The pointer() isn't
       considered in any way, as it could signalize a newly pressed pointer but
       also a no longer pressed one; extra pressed pointers are ignored as
       well. */
    {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        event.setCaptured(true);
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 0); /* no button pressed */
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, Pointer::MouseLeft, true, 0};
        event.setCaptured(false);
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 0); /* not captured */
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, Pointer::MouseLeft|Pointer::MouseRight, true, 0, {-1.0f, 2.4f}};
        event.setCaptured(true);
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
        CORRADE_COMPARE(calledOffset, (Vector2{-1.0f, 2.4f}));
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, Pointer::MouseMiddle, true, 0};
        event.setCaptured(true);
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1); /* not a valid pointer pressed */
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, Pointer::MouseRight, true, 0};
        event.setCaptured(true);
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1); /* not a valid pointer pressed */
    } {
        PointerMoveEvent event{{}, PointerEventSource::Touch, {}, Pointer::Finger|Pointer::Eraser, true, 0, {0.5f, -1.0f}};
        event.setCaptured(true);
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 2);
        CORRADE_COMPARE(calledOffset, (Vector2{-0.5f, 1.4f}));
    } {
        PointerMoveEvent event{{}, PointerEventSource::Touch, {}, Pointer::Finger|Pointer::Eraser, false, 0, {0.5f, -1.0f}};
        event.setCaptured(true);
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 2); /* seconary finger ignored */
    } {
        PointerMoveEvent event{{}, PointerEventSource::Pen, {}, Pointer::Pen|Pointer::Eraser, true, 0, {1.0f, -0.5f}};
        event.setCaptured(true);
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 3);
        CORRADE_COMPARE(calledOffset, (Vector2{0.5f, 0.9f}));
    } {
        PointerMoveEvent event{{}, PointerEventSource::Pen, {}, Pointer::Eraser, true, 0};
        event.setCaptured(true);
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 3); /* not a valid pointer pressed */

    /* Shouldn't get fired for any other than move events */
    } {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 3);
    } {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 3);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, Pointer::MouseLeft, true, 0};
        layer.pointerEnterEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 3);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, Pointer::MouseLeft, true, 0};
        layer.pointerLeaveEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 3);
    } {
        FocusEvent event{{}};
        layer.focusEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 3);
    } {
        FocusEvent event{{}};
        layer.blurEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 3);
    }
}

void EventLayerTest::dragPress() {
    EventLayer layer{layerHandle(0, 1)};

    Int called = 0;
    DataHandle handle = layer.onDrag(nodeHandle(0, 1), [&called](const Vector2&) {
        ++called;
    });

    /* The press event should get accepted for *captured* mouse left, *primary*
       finger or pen to prevent it from being propagated further if no other
       data accepts it. The handler shouldn't get called though. */
    {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        event.setCaptured(true);
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        event.setCaptured(false);
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseMiddle, true, 0};
        event.setCaptured(true);
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseRight, true, 0};
        event.setCaptured(true);
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{{}, PointerEventSource::Touch, Pointer::Finger, true, 0};
        event.setCaptured(true);
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{{}, PointerEventSource::Touch, Pointer::Finger, false, 0};
        event.setCaptured(true);
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{{}, PointerEventSource::Pen, Pointer::Pen, true, 0};
        event.setCaptured(true);
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{{}, PointerEventSource::Pen, Pointer::Eraser, true, 0};
        event.setCaptured(true);
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);

    /* Any other than press or move event shouldn't get accepted */
    } {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        event.setCaptured(true);
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, Pointer::MouseLeft, true, 0};
        event.setCaptured(true);
        layer.pointerEnterEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, Pointer::MouseLeft, true, 0};
        event.setCaptured(true);
        layer.pointerLeaveEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        FocusEvent event{{}};
        layer.focusEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        FocusEvent event{{}};
        layer.blurEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);

    /* Verify that the callback is actually properly registered so this doesn't
       result in false positives */
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, Pointer::MouseLeft, true, 0};
        event.setCaptured(true);
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(event.isAccepted());
        CORRADE_COMPARE(called, 1);
    }
}

void EventLayerTest::dragFromUserInterface() {
    auto&& data = FromUserInterfaceData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* "Integration" test to verify onDrag() behavior with the whole event
       pipeline in AbstractUserInterface */

    AbstractUserInterface ui{{100, 100}};

    EventLayer& layer = ui.setLayerInstance(Containers::pointer<EventLayer>(ui.createLayer()));

    /* A node below the one that should react to the drag event, accepting
       presses. Shouldn't get considered at all. */
    Int belowCalled = 0;
    NodeHandle nodeBelow = ui.createNode({}, {100, 100}, data.flags);
    layer.onPress(nodeBelow, [&belowCalled]{
        ++belowCalled;
    });

    NodeHandle node = ui.createNode(
        data.parent ? nodeBelow : NodeHandle::Null,
        {25, 50}, {50, 25});

    Int called = 0, positionCalled = 0;
    layer.onDrag(node, [&called](const Vector2& relativePosition) {
        CORRADE_COMPARE(relativePosition, (Vector2{-5.0f, -10.0f}));
        ++called;
    });
    layer.onDrag(node, [&positionCalled](const Vector2& position, const Vector2& relativePosition) {
        CORRADE_COMPARE(position, (Vector2{20.0f, 5.0f}));
        CORRADE_COMPARE(relativePosition, (Vector2{-5.0f, -10.0f}));
        ++positionCalled;
    });

    /* A move alone with a button pressed but no captured node shouldn't be
       accepted because it means it originates outside of the UI, and such
       events shouldn't lead to things being dragged. */
    {
        PointerMoveEvent event{{}, PointerEventSource::Touch, {}, Pointer::Finger, true, 0};
        CORRADE_VERIFY(!ui.pointerMoveEvent({50, 70}, event));
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(called, 0);
        CORRADE_COMPARE(positionCalled, 0);
        CORRADE_COMPARE(belowCalled, 0);

    /* A press should be accepted but not resulting in the handler being
       called */
    } {
        PointerEvent event{{}, PointerEventSource::Pen, Pointer::Pen, true, 0};
        CORRADE_VERIFY(ui.pointerPressEvent({50, 70}, event));
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentPressedNode(), node);
        CORRADE_COMPARE(ui.currentCapturedNode(), node);
        CORRADE_COMPARE(called, 0);
        CORRADE_COMPARE(positionCalled, 0);
        CORRADE_COMPARE(belowCalled, 0);

    /* A move with a node captured but without any pointer pressed should be
       ignored. This isn't likely to happen unless the application drops the
       release events somehow. */
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        CORRADE_VERIFY(!ui.pointerMoveEvent({50, 65}, event));
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentPressedNode(), node);
        CORRADE_COMPARE(ui.currentCapturedNode(), node);
        CORRADE_COMPARE(called, 0);
        CORRADE_COMPARE(positionCalled, 0);
        CORRADE_COMPARE(belowCalled, 0);

    /* A move with a pointer pressed with a node captured should be treated as
       a drag */
    } {
        PointerMoveEvent event{{}, PointerEventSource::Pen, {}, Pointer::Pen, true, 0};
        CORRADE_VERIFY(ui.pointerMoveEvent({45, 55}, event));
        CORRADE_COMPARE(ui.currentHoveredNode(), node);
        CORRADE_COMPARE(ui.currentPressedNode(), node);
        CORRADE_COMPARE(ui.currentCapturedNode(), node);
        CORRADE_COMPARE(called, 1);
        CORRADE_COMPARE(positionCalled, 1);
        CORRADE_COMPARE(belowCalled, 0);
    }
}

void EventLayerTest::dragFromUserInterfaceFallthroughThreshold() {
    auto&& data = DragFromUserInterfaceFallthroughThresholdData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Compared to dragFromUserInterface(), which tests that it doesn't
       unconditionally fall through to other nodes, this verifies that the
       threshold is in effect */

    AbstractUserInterface ui{{100, 100}};

    EventLayer& layer = ui.setLayerInstance(Containers::pointer<EventLayer>(ui.createLayer()));
    CORRADE_COMPARE(layer.dragThreshold(), 16.0f);

    Vector2 belowCalled;
    NodeHandle nodeBelow = ui.createNode({}, {100, 100}, NodeFlag::FallthroughPointerEvents);
    /* Verify that both variants of the callback get the same data for the
       initial jump */
    /** @todo once it's possible to have multiple fallback onDrag handlers for
        the same node, add them both instead of having an instanced test
        case */
    if(data.positionCallback)
        layer.onDrag(nodeBelow, [&belowCalled](const Vector2&, const Vector2& relativePosition) {
            belowCalled += relativePosition;
        });
    else
        layer.onDrag(nodeBelow, [&belowCalled](const Vector2& relativePosition) {
            belowCalled += relativePosition;
        });

    Int betweenCalled = 0;
    NodeHandle nodeBetween = ui.createNode(nodeBelow, {}, {100, 100});
    layer.onDrag(nodeBetween, [&betweenCalled](const Vector2&) {
        ++betweenCalled;
    });

    Vector2 aboveCalled;
    NodeHandle nodeAbove = ui.createNode(nodeBetween, {}, {100, 100});
    layer.onDrag(nodeAbove, [&aboveCalled](const Vector2& relativePosition) {
        aboveCalled += relativePosition;
    });

    /* Set the threshold lower, sqrt(3*3 + 4*4) = 5 */
    layer.setDragThreshold(5.0f);
    CORRADE_COMPARE(layer.dragThreshold(), 5.0f);

    /* Press to capture the node */
    {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        CORRADE_VERIFY(ui.pointerPressEvent({50, 70}, event));
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentPressedNode(), nodeAbove);
        CORRADE_COMPARE(ui.currentCapturedNode(), nodeAbove);
        CORRADE_COMPARE(belowCalled, Vector2{});
        CORRADE_COMPARE(betweenCalled, 0);
        CORRADE_COMPARE(aboveCalled, Vector2{});

    /* Move by 3 units horizontally directs to the above */
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, Pointer::MouseLeft, true, 0};
        CORRADE_VERIFY(ui.pointerMoveEvent({53, 70}, event));
        CORRADE_COMPARE(ui.currentHoveredNode(), nodeAbove);
        CORRADE_COMPARE(ui.currentPressedNode(), nodeAbove);
        CORRADE_COMPARE(ui.currentCapturedNode(), nodeAbove);
        CORRADE_COMPARE(belowCalled, Vector2{});
        CORRADE_COMPARE(betweenCalled, 0);
        CORRADE_COMPARE(aboveCalled, (Vector2{3.0f, 0.0f}));

    /* Move by 2 units vertically still also. The sum is 5 units but not the
       length. */
    } {
        aboveCalled = {};

        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, Pointer::MouseLeft, true, 0};
        CORRADE_VERIFY(ui.pointerMoveEvent({53, 72}, event));
        CORRADE_COMPARE(ui.currentHoveredNode(), nodeAbove);
        CORRADE_COMPARE(ui.currentPressedNode(), nodeAbove);
        CORRADE_COMPARE(ui.currentCapturedNode(), nodeAbove);
        CORRADE_COMPARE(belowCalled, Vector2{});
        CORRADE_COMPARE(betweenCalled, 0);
        CORRADE_COMPARE(aboveCalled, (Vector2{0.0f, 2.0f}));

    /* Moving by 2 more transfers the capture to the fallthrough node, dragging
       by the whole amount. Is still called on the above as well, the node in
       between that isn't fallthrough gets nothing. */
    } {
        aboveCalled = {};

        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, Pointer::MouseLeft, true, 0};
        CORRADE_VERIFY(ui.pointerMoveEvent({53, 74}, event));
        CORRADE_COMPARE(ui.currentHoveredNode(), nodeBelow);
        CORRADE_COMPARE(ui.currentPressedNode(), nodeBelow);
        CORRADE_COMPARE(ui.currentCapturedNode(), nodeBelow);
        CORRADE_COMPARE(belowCalled, (Vector2{3.0f, 4.0f}));
        CORRADE_COMPARE(betweenCalled, 0);
        CORRADE_COMPARE(aboveCalled, (Vector2{0.0f, 2.0f}));

    /* The next move is directed to just the node below. The distance from the
       initial press is now less than the threshold again but that's not
       considered anymore. */
    } {
        belowCalled = {};
        aboveCalled = {};

        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, Pointer::MouseLeft, true, 0};
        CORRADE_VERIFY(ui.pointerMoveEvent({53, 73}, event));
        CORRADE_COMPARE(ui.currentHoveredNode(), nodeBelow);
        CORRADE_COMPARE(ui.currentPressedNode(), nodeBelow);
        CORRADE_COMPARE(ui.currentCapturedNode(), nodeBelow);
        CORRADE_COMPARE(belowCalled, (Vector2{0.0f, -1.0f}));
        CORRADE_COMPARE(betweenCalled, 0);
        CORRADE_COMPARE(aboveCalled, Vector2{});

    /* Another press makes it start over again, i.e. directed to the top
       node */
    } {
        belowCalled = {};

        PointerEvent press{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        PointerMoveEvent move{{}, PointerEventSource::Mouse, {}, Pointer::MouseLeft, true, 0};
        CORRADE_VERIFY(ui.pointerPressEvent({30, 20}, press));
        CORRADE_VERIFY(ui.pointerMoveEvent({30, 18}, move));
        CORRADE_COMPARE(ui.currentHoveredNode(), nodeAbove);
        CORRADE_COMPARE(ui.currentPressedNode(), nodeAbove);
        CORRADE_COMPARE(ui.currentCapturedNode(), nodeAbove);
        CORRADE_COMPARE(belowCalled, Vector2{});
        CORRADE_COMPARE(betweenCalled, 0);
        CORRADE_COMPARE(aboveCalled, (Vector2{0.0f, -2.0f}));

    /* And again only after reaching the threshold it's transferred below */
    } {
        aboveCalled = {};

        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, Pointer::MouseLeft, true, 0};
        CORRADE_VERIFY(ui.pointerMoveEvent({30, 25}, event));
        CORRADE_COMPARE(ui.currentHoveredNode(), nodeBelow);
        CORRADE_COMPARE(ui.currentPressedNode(), nodeBelow);
        CORRADE_COMPARE(ui.currentCapturedNode(), nodeBelow);
        CORRADE_COMPARE(belowCalled, (Vector2{0.0f, 5.0f}));
        CORRADE_COMPARE(betweenCalled, 0);
        CORRADE_COMPARE(aboveCalled, (Vector2{0.0f, 7.0f}));
    }
}

void EventLayerTest::dragFromUserInterfaceFallthroughThresholdMultipleHandlers() {
    AbstractUserInterface ui{{100, 100}};

    EventLayer& layer = ui.setLayerInstance(Containers::pointer<EventLayer>(ui.createLayer()));

    NodeHandle nodeBelow = ui.createNode({}, {100, 100}, NodeFlag::FallthroughPointerEvents);

    Int aboveCalled = 0;
    NodeHandle nodeAbove = ui.createNode(nodeBelow, {}, {100, 100});
    layer.onDrag(nodeAbove, [&aboveCalled](const Vector2&) {
        ++aboveCalled;
    });

    /* With just one handler it gets called alright */
    Int belowCalled1 = 0;
    layer.onDrag(nodeBelow, [&belowCalled1](const Vector2&) {
        ++belowCalled1;
    });
    {
        PointerEvent press{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        PointerMoveEvent move{{}, PointerEventSource::Mouse, {}, Pointer::MouseLeft, true, 0};
        CORRADE_VERIFY(ui.pointerPressEvent({50, 70}, press));
        CORRADE_VERIFY(ui.pointerMoveEvent({70, 70}, move));
        CORRADE_COMPARE(aboveCalled, 1);
        CORRADE_COMPARE(belowCalled1, 1);
    }

    /* Second handler on the same node breaks it because it's currently tracked
       on a data ID and not a node ID. Doing a press + drag again so it starts
       from the nodeAbove -- it's only the fallthrough that breaks, not the
       direct call. */
    Int belowCalled2 = 0;
    layer.onDrag(nodeBelow, [&belowCalled2](const Vector2&) {
        ++belowCalled2;
    });
    {
        PointerEvent press{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        PointerMoveEvent move{{}, PointerEventSource::Mouse, {}, Pointer::MouseLeft, true, 0};
        CORRADE_VERIFY(ui.pointerPressEvent({50, 70}, press));
        CORRADE_VERIFY(ui.pointerMoveEvent({70, 70}, move));
        CORRADE_COMPARE(aboveCalled, 2);

        CORRADE_EXPECT_FAIL("Multiple onDrag() handlers on the same fallthrough node conflict with each other, causing nothing to be sent.");
        CORRADE_COMPARE(belowCalled1, 2);
        CORRADE_COMPARE(belowCalled2, 1);
    }
}

void EventLayerTest::pinch() {
    EventLayer layer{layerHandle(0, 1)};

    Int called = 0;
    DataHandle handle = layer.onPinch(nodeHandle(0, 1), [&called](const Vector2&, const Vector2&, const Complex&, Float) {
        ++called;
    });

    /* Make the gesture actually recognized first */
    {
        PointerEvent primary{{}, PointerEventSource::Touch, Pointer::Finger, true, 36};
        PointerEvent secondary{{}, PointerEventSource::Touch, Pointer::Finger, false, 17};
        layer.pointerPressEvent(dataHandleId(handle), primary);
        layer.pointerPressEvent(dataHandleId(handle), secondary);
    }

    /* Should only get fired for a move that originates from one of the
       registered fingers, not any other arbitrary move where it would
       repeatedly give back the same data */
    {
        /* Secondary with a matching ID */
        PointerMoveEvent event{{}, PointerEventSource::Touch, {}, Pointer::Finger, false, 17};
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        /* Primary with a matching ID */
        PointerMoveEvent event{{}, PointerEventSource::Touch, {}, Pointer::Finger, true, 36};
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 2);
    } {
        /* Primary, but different ID */
        PointerMoveEvent event{{}, PointerEventSource::Touch, {}, Pointer::Finger, true, 37};
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 2);
    } {
        /* Secondary, but different ID. Passing an event that doesn't actually
           have the finger pressed currently works as well, it checks just the
           source. */
        PointerMoveEvent event{{}, PointerEventSource::Touch, {}, {}, false, 16};
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 2);
    } {
        /* Events coming from a mouse are ignored by the gesture recognizer
           altogether, even if a finger is currently pressed */
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, Pointer::Finger, true, 0};
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 2);
    } {
        /* Pen also */
        PointerMoveEvent event{{}, PointerEventSource::Pen, {}, Pointer::Finger, true, 0};
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 2);

    /* Shouldn't get fired for any other than move events, even though the
       gesture recognizer is getting fed in press and release as well */
    } {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 2);
    } {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 2);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, Pointer::MouseLeft, true, 0};
        layer.pointerEnterEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 2);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, Pointer::MouseLeft, true, 0};
        layer.pointerLeaveEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 2);
    } {
        FocusEvent event{{}};
        layer.focusEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 2);
    } {
        FocusEvent event{{}};
        layer.blurEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 2);
    } {
        /* ID matches the tracked primary finger so this should feed the
           gesture recognizer, but it being fed shouldn't trigger a call. Doing
           as last because this resets the isGesture() bit. */
        PointerEvent event{{}, PointerEventSource::Touch, Pointer::Finger, true, 36};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 2);
    } {
        /* ID matches the tracked secondary finger so again but again. This
           should set isGesture() back. */
        PointerEvent event{{}, PointerEventSource::Touch, Pointer::Finger, false, 17};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 2);
    } {
        /* ID matches the tracked secondary finger, so again but again. This
           should reset isGesture() again. */
        PointerEvent event{{}, PointerEventSource::Touch, Pointer::Finger, false, 17};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 2);
    }
}

void EventLayerTest::pinchReset() {
    EventLayer layer{layerHandle(0, 1)};

    Int firstCalled = 0, secondCalled = 0;
    DataHandle first = layer.onPinch(nodeHandle(0, 1), [&firstCalled](const Vector2&, const Vector2&, const Complex&, Float) {
        ++firstCalled;
    });
    DataHandle second = layer.onPinch(nodeHandle(0, 1), [&secondCalled](const Vector2&, const Vector2&, const Complex&, Float) {
        ++secondCalled;
    });

    /* Make the gesture recognized on the first */
    {
        PointerEvent primary{{}, PointerEventSource::Touch, Pointer::Finger, true, 36};
        PointerEvent secondary{{}, PointerEventSource::Touch, Pointer::Finger, false, 17};
        PointerMoveEvent move{{}, PointerEventSource::Touch, {}, Pointer::Finger, true, 36};
        layer.pointerPressEvent(dataHandleId(first), primary);
        layer.pointerPressEvent(dataHandleId(first), secondary);
        layer.pointerMoveEvent(dataHandleId(first), move);
        CORRADE_COMPARE(firstCalled, 1);

    /* A mouse or pen press, move, release, cancel or visibility lost on the
       second should be independent and not result in the gesture being
       reset */
    } {
        /* Matching ID shouldn't cause any problem either */
        PointerEvent mousePress{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 36};
        PointerMoveEvent mouseMove{{}, PointerEventSource::Mouse, {}, {}, true, 36};
        PointerEvent mouseRelease{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 36};
        PointerEvent penPress{{}, PointerEventSource::Pen, Pointer::Pen, true, 36};
        PointerMoveEvent penMove{{}, PointerEventSource::Mouse, {}, {}, true, 36};
        PointerEvent penRelease{{}, PointerEventSource::Pen, Pointer::Pen, true, 36};
        PointerCancelEvent cancel{{}};
        VisibilityLostEvent visibilityLost;
        layer.pointerPressEvent(dataHandleId(second), mousePress);
        layer.pointerMoveEvent(dataHandleId(second), mouseMove);
        layer.pointerReleaseEvent(dataHandleId(second), mouseRelease);
        layer.pointerPressEvent(dataHandleId(second), penPress);
        layer.pointerMoveEvent(dataHandleId(second), penMove);
        layer.pointerReleaseEvent(dataHandleId(second), penRelease);
        layer.pointerCancelEvent(dataHandleId(second), cancel);
        layer.visibilityLostEvent(dataHandleId(second), visibilityLost);
        CORRADE_COMPARE(firstCalled, 1);

        /* Gets called on the next move on the first */
        PointerMoveEvent move{{}, PointerEventSource::Touch, {}, Pointer::Finger, true, 36};
        layer.pointerMoveEvent(dataHandleId(first), move);
        CORRADE_COMPARE(firstCalled, 2);

    /* A finger press on the second however resets it */
    } {
        /* Even a different ID should reset it */
        PointerEvent fingerPress{{}, PointerEventSource::Touch, Pointer::Finger, false, 22};
        layer.pointerPressEvent(dataHandleId(second), fingerPress);
        CORRADE_COMPARE(firstCalled, 2);

        /* Slot no longer triggered on the next move on the first */
        PointerMoveEvent move1{{}, PointerEventSource::Touch, {}, Pointer::Finger, true, 36};
        layer.pointerMoveEvent(dataHandleId(first), move1);
        CORRADE_COMPARE(firstCalled, 2);

        /* The gesture needs to be fully recognized on the first again */
        PointerEvent primary{{}, PointerEventSource::Touch, Pointer::Finger, true, 36};
        PointerEvent secondary{{}, PointerEventSource::Touch, Pointer::Finger, false, 17};
        PointerMoveEvent move2{{}, PointerEventSource::Touch, {}, Pointer::Finger, true, 36};
        layer.pointerPressEvent(dataHandleId(first), primary);
        layer.pointerPressEvent(dataHandleId(first), secondary);
        layer.pointerMoveEvent(dataHandleId(first), move2);
        CORRADE_COMPARE(firstCalled, 3);

    /* A finger move on the second resets it as well */
    } {
        /* Even a different ID should reset it */
        PointerMoveEvent fingerMove{{}, PointerEventSource::Touch, {}, {}, false, 22};
        layer.pointerMoveEvent(dataHandleId(second), fingerMove);
        CORRADE_COMPARE(firstCalled, 3);

        /* Slot no longer triggered on the next move on the first */
        PointerMoveEvent move1{{}, PointerEventSource::Touch, {}, Pointer::Finger, true, 36};
        layer.pointerMoveEvent(dataHandleId(first), move1);
        CORRADE_COMPARE(firstCalled, 3);

        /* The gesture needs to be fully recognized on the first again */
        PointerEvent primary{{}, PointerEventSource::Touch, Pointer::Finger, true, 36};
        PointerEvent secondary{{}, PointerEventSource::Touch, Pointer::Finger, false, 17};
        PointerMoveEvent move2{{}, PointerEventSource::Touch, {}, Pointer::Finger, true, 36};
        layer.pointerPressEvent(dataHandleId(first), primary);
        layer.pointerPressEvent(dataHandleId(first), secondary);
        layer.pointerMoveEvent(dataHandleId(first), move2);
        CORRADE_COMPARE(firstCalled, 4);

    /* And a finger release on the second also */
    } {
        /* Even a different ID should reset it */
        PointerEvent fingerRelease{{}, PointerEventSource::Touch, Pointer::Finger, false, 22};
        layer.pointerReleaseEvent(dataHandleId(second), fingerRelease);
        CORRADE_COMPARE(firstCalled, 4);

        /* Slot no longer triggered on the next move on the first */
        PointerMoveEvent move1{{}, PointerEventSource::Touch, {}, Pointer::Finger, true, 36};
        layer.pointerMoveEvent(dataHandleId(first), move1);
        CORRADE_COMPARE(firstCalled, 4);

        /* The gesture needs to be fully recognized on the first again */
        PointerEvent primary{{}, PointerEventSource::Touch, Pointer::Finger, true, 36};
        PointerEvent secondary{{}, PointerEventSource::Touch, Pointer::Finger, false, 17};
        PointerMoveEvent move2{{}, PointerEventSource::Touch, {}, Pointer::Finger, true, 36};
        layer.pointerPressEvent(dataHandleId(first), primary);
        layer.pointerPressEvent(dataHandleId(first), secondary);
        layer.pointerMoveEvent(dataHandleId(first), move2);
        CORRADE_COMPARE(firstCalled, 5);

    /* Cancel on the same data ID resets */
    } {
        PointerCancelEvent cancel{{}};
        layer.pointerCancelEvent(dataHandleId(first), cancel);
        CORRADE_COMPARE(firstCalled, 5);

        /* Slot no longer triggered on the next move on the first */
        PointerMoveEvent move1{{}, PointerEventSource::Touch, {}, Pointer::Finger, true, 36};
        layer.pointerMoveEvent(dataHandleId(first), move1);
        CORRADE_COMPARE(firstCalled, 5);

        /* The gesture needs to be fully recognized on the first again */
        PointerEvent primary{{}, PointerEventSource::Touch, Pointer::Finger, true, 36};
        PointerEvent secondary{{}, PointerEventSource::Touch, Pointer::Finger, false, 17};
        PointerMoveEvent move2{{}, PointerEventSource::Touch, {}, Pointer::Finger, true, 36};
        layer.pointerPressEvent(dataHandleId(first), primary);
        layer.pointerPressEvent(dataHandleId(first), secondary);
        layer.pointerMoveEvent(dataHandleId(first), move2);
        CORRADE_COMPARE(firstCalled, 6);

    /* Visibility lost event resets as well */
    } {
        VisibilityLostEvent lost;
        layer.visibilityLostEvent(dataHandleId(first), lost);
        CORRADE_COMPARE(firstCalled, 6);

        /* Slot no longer triggered on the next move on the first */
        PointerMoveEvent move1{{}, PointerEventSource::Touch, {}, Pointer::Finger, true, 36};
        layer.pointerMoveEvent(dataHandleId(first), move1);
        CORRADE_COMPARE(firstCalled, 6);

        /* The gesture needs to be fully recognized on the first again */
        PointerEvent primary{{}, PointerEventSource::Touch, Pointer::Finger, true, 36};
        PointerEvent secondary{{}, PointerEventSource::Touch, Pointer::Finger, false, 17};
        PointerMoveEvent move2{{}, PointerEventSource::Touch, {}, Pointer::Finger, true, 36};
        layer.pointerPressEvent(dataHandleId(first), primary);
        layer.pointerPressEvent(dataHandleId(first), secondary);
        layer.pointerMoveEvent(dataHandleId(first), move2);
        CORRADE_COMPARE(firstCalled, 7);

    /* If the data is removed and created again with the same ID, it gets reset
       also. Transitive data removal due to the node being removed is tested in
       pinchFromUserInterface() below. */
    /** @todo probably no longer necessary once we attach to a node instead
        (when events are called in a bulk for the whole node) -- then it gets
        reset only when the node disappears */
    } {
        layer.remove(first);

        DataHandle first2 = layer.onPinch(nodeHandle(0, 1), [&firstCalled](const Vector2&, const Vector2&, const Complex&, Float) {
            ++firstCalled;
        });
        CORRADE_COMPARE(dataHandleId(first2), dataHandleId(first));

        /* Slot no longer triggered on the next move on the first */
        PointerMoveEvent move1{{}, PointerEventSource::Touch, {}, Pointer::Finger, true, 36};
        layer.pointerMoveEvent(dataHandleId(first2), move1);
        CORRADE_COMPARE(firstCalled, 7);

        /* The gesture needs to be fully recognized on the first again */
        PointerEvent primary{{}, PointerEventSource::Touch, Pointer::Finger, true, 36};
        PointerEvent secondary{{}, PointerEventSource::Touch, Pointer::Finger, false, 17};
        PointerMoveEvent move2{{}, PointerEventSource::Touch, {}, Pointer::Finger, true, 36};
        layer.pointerPressEvent(dataHandleId(first2), primary);
        layer.pointerPressEvent(dataHandleId(first2), secondary);
        layer.pointerMoveEvent(dataHandleId(first2), move2);
        CORRADE_COMPARE(firstCalled, 8);
    }
}

void EventLayerTest::pinchPressMoveRelease() {
    EventLayer layer{layerHandle(0, 1)};

    Int called = 0;
    DataHandle handle = layer.onPinch(nodeHandle(0, 1), [&called](const Vector2&, const Vector2&, const Complex&, Float) {
        ++called;
    });

    /* The press event should get accepted for tracked fingers to prevent it
       from being propagated further if no other data accepts it. The handler
       shouldn't get called though. */
    {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{{}, PointerEventSource::Pen, Pointer::Pen, true, 0};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{{}, PointerEventSource::Touch, Pointer::Finger, true, 32};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{{}, PointerEventSource::Touch, Pointer::Finger, false, 16};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        /* Secondary finger press that doesn't match the above isn't used */
        PointerEvent event{{}, PointerEventSource::Touch, Pointer::Finger, false, 22};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);

    /* Similarly for move. Only in case the actual finger matches what's
       tracked it gets fired. */
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Pen, {}, {}, true, 0};
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Touch, {}, {}, true, 32};
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(event.isAccepted());
        CORRADE_COMPARE(called, 1);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Touch, {}, {}, false, 16};
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(event.isAccepted());
        CORRADE_COMPARE(called, 2);
    } {
        /* Primary finger move that doesn't match the ID above isn't used */
        PointerMoveEvent event{{}, PointerEventSource::Touch, {}, {}, true, 44};
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 2);

    /* And release */
    } {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 2);
    } {
        PointerEvent event{{}, PointerEventSource::Pen, Pointer::Pen, true, 0};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 2);
    } {
        /* Secondary finger release that doesn't match the above isn't used */
        PointerEvent event{{}, PointerEventSource::Touch, Pointer::Finger, false, 22};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 2);
    } {
        PointerEvent event{{}, PointerEventSource::Touch, Pointer::Finger, false, 16};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(event.isAccepted());
        CORRADE_COMPARE(called, 2);
    } {
        /* If the primary finger would be released first, the secondary release
           wouldn't be used */
        PointerEvent event{{}, PointerEventSource::Touch, Pointer::Finger, true, 32};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(event.isAccepted());
        CORRADE_COMPARE(called, 2);

    /* Any other than press, move or release event shouldn't get accepted */
    } {
        PointerMoveEvent event{{}, PointerEventSource::Touch, {}, {}, true, 32};
        layer.pointerEnterEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 2);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Touch, {}, {}, true, 32};
        layer.pointerLeaveEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 2);
    } {
        FocusEvent event{{}};
        layer.focusEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 2);
    } {
        FocusEvent event{{}};
        layer.blurEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 2);
    }
}

void EventLayerTest::pinchFromUserInterface() {
    auto&& data = FromUserInterfaceData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* "Integration" test to verify onDrag() behavior with the whole event
       pipeline in AbstractUserInterface */

    AbstractUserInterface ui{{100, 100}};

    EventLayer& layer = ui.setLayerInstance(Containers::pointer<EventLayer>(ui.createLayer()));

    /* A node below the one that should react to the drag event, accepting
       presses, moves and releases. Shouldn't get considered at all. */
    Int belowCalled = 0;
    NodeHandle nodeBelow = ui.createNode({}, {100, 100}, data.flags);
    layer.onPress(nodeBelow, [&belowCalled]{
        ++belowCalled;
    });
    layer.onDrag(nodeBelow, [&belowCalled](const Vector2&) {
        ++belowCalled;
    });
    layer.onRelease(nodeBelow, [&belowCalled]{
        ++belowCalled;
    });

    NodeHandle node = ui.createNode(
        data.parent ? nodeBelow : NodeHandle::Null,
        {25, 50}, {50, 25});

    Int called = 0;
    struct {
        Vector2 position;
        Vector2 relativeTranslation;
        Complex relativeRotation;
        Float relativeScaling;
    } expected;
    DataHandle nodeData = layer.onPinch(node, [&called, &expected](const Vector2& position, const Vector2& relativeTranslation, const Complex& relativeRotation, Float relativeScaling) {
        CORRADE_COMPARE(position, expected.position);
        CORRADE_COMPARE(relativeTranslation, expected.relativeTranslation);
        CORRADE_COMPARE(relativeRotation, expected.relativeRotation);
        CORRADE_COMPARE(relativeScaling, expected.relativeScaling);
        ++called;
    });

    /* Presses for the two tracked fingers should be accepted but not resulting
       in the handler being called */
    {
        PointerEvent event1{{}, PointerEventSource::Touch, Pointer::Finger, true, 633};
        PointerEvent event2{{}, PointerEventSource::Touch, Pointer::Finger, false, 3371};
        CORRADE_VERIFY(ui.pointerPressEvent({50, 70}, event1));
        CORRADE_VERIFY(ui.pointerPressEvent({50, 75}, event2));
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentPressedNode(), node);
        CORRADE_COMPARE(ui.currentCapturedNode(), node);
        CORRADE_COMPARE(called, 0);
        CORRADE_COMPARE(belowCalled, 0);

    /* A move of one of the two fingers makes the slot called, rotating 180° */
    } {
        expected.position = {25.0f, 17.5f};
        expected.relativeTranslation = {0.0f, -5.0f};
        expected.relativeRotation = Complex::rotation(180.0_degf);
        expected.relativeScaling = 1.0f;

        PointerMoveEvent event{{}, PointerEventSource::Touch, {}, {}, false, 3371};
        CORRADE_VERIFY(ui.pointerMoveEvent({50, 65}, event));
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentPressedNode(), node);
        CORRADE_COMPARE(ui.currentCapturedNode(), node);
        CORRADE_COMPARE(called, 1);
        CORRADE_COMPARE(belowCalled, 0);

    /* A press of another finger should be ignored, and since there's a
       capture, it shouldn't fall through either */
    } {
        PointerEvent event{{}, PointerEventSource::Touch, Pointer::Finger, false, 1226};
        CORRADE_VERIFY(!ui.pointerPressEvent({50, 75}, event));
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentPressedNode(), node);
        CORRADE_COMPARE(ui.currentCapturedNode(), node);
        CORRADE_COMPARE(called, 1);
        CORRADE_COMPARE(belowCalled, 0);

    /* A release of one of the fingers should be accepted. Move of that finger
       then doesn't get accepted. */
    } {
        PointerEvent release{{}, PointerEventSource::Touch, Pointer::Finger, false, 3371};
        CORRADE_VERIFY(ui.pointerReleaseEvent({50, 55}, release));
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentPressedNode(), node);
        CORRADE_COMPARE(ui.currentCapturedNode(), node);
        CORRADE_COMPARE(called, 1);
        CORRADE_COMPARE(belowCalled, 0);

        PointerMoveEvent move{{}, PointerEventSource::Touch, {}, {}, false, 3371};
        CORRADE_VERIFY(!ui.pointerMoveEvent({50, 65}, move));
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentPressedNode(), node);
        CORRADE_COMPARE(ui.currentCapturedNode(), node);
        CORRADE_COMPARE(called, 1);
        CORRADE_COMPARE(belowCalled, 0);

    /* A press of another finger from above is accepted now, and a subsequent
       move of even the primary finger generates another pinch. */
    } {
        PointerEvent press{{}, PointerEventSource::Touch, Pointer::Finger, false, 1226};
        CORRADE_VERIFY(ui.pointerPressEvent({50, 65}, press));
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentPressedNode(), node);
        CORRADE_COMPARE(ui.currentCapturedNode(), node);
        CORRADE_COMPARE(called, 1);
        CORRADE_COMPARE(belowCalled, 0);

        expected.position = {25.0f, 12.5f};
        expected.relativeTranslation = {0.0f, -5.0f};
        expected.relativeRotation = Complex::rotation(180.0_degf);
        expected.relativeScaling = 1.0f;

        PointerMoveEvent move{{}, PointerEventSource::Touch, {}, {}, true, 633};
        CORRADE_VERIFY(ui.pointerMoveEvent({50, 60}, move));
        CORRADE_COMPARE(ui.currentHoveredNode(), node);
        CORRADE_COMPARE(ui.currentPressedNode(), node);
        CORRADE_COMPARE(ui.currentCapturedNode(), node);
        CORRADE_COMPARE(called, 2);
        CORRADE_COMPARE(belowCalled, 0);

    /* Hiding the node and then showing it again makes the gesture reset. It
       has to be recognized from scratch to generate a pinch again. */
    } {
        ui.addNodeFlags(node, NodeFlag::Hidden);
        /* Update so it's actually cleared from currentCapturedNode() etc.
           Without this, it'd be as if the flag wasn't set at all. */
        ui.update();
        ui.clearNodeFlags(node, NodeFlag::Hidden);

        /* Move of the primary finger isn't even accepted now because the
           gesture recognizer doesn't track it as pressed */
        PointerMoveEvent move1{{}, PointerEventSource::Touch, {}, {}, true, 633};
        CORRADE_VERIFY(!ui.pointerMoveEvent({50, 65}, move1));
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(called, 2); /* not called */
        CORRADE_COMPARE(belowCalled, 0);

        /* Same as the initial state */
        expected.position = {25.0f, 17.5f};
        expected.relativeTranslation = {0.0f, -5.0f};
        expected.relativeRotation = Complex::rotation(180.0_degf);
        expected.relativeScaling = 1.0f;

        PointerEvent press1{{}, PointerEventSource::Touch, Pointer::Finger, true, 633};
        PointerEvent press2{{}, PointerEventSource::Touch, Pointer::Finger, false, 3371};
        PointerMoveEvent move2{{}, PointerEventSource::Touch, {}, {}, false, 3371};
        CORRADE_VERIFY(ui.pointerPressEvent({50, 70}, press1));
        CORRADE_VERIFY(ui.pointerPressEvent({50, 75}, press2));
        CORRADE_VERIFY(ui.pointerMoveEvent({50, 65}, move2));
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentPressedNode(), node);
        CORRADE_COMPARE(ui.currentCapturedNode(), node);
        CORRADE_COMPARE(called, 3);
        CORRADE_COMPARE(belowCalled, 0);

    /* Removing the node and recreating the data with the same ID makes the
       gesture reset, again it needs a full re-recognition */
    } {
        ui.removeNode(node);
        /* Update so layer data clean gets actually performed */
        ui.update();

        NodeHandle node2 = ui.createNode({25, 50}, {50, 25});
        DataHandle nodeData2 = layer.onPinch(node2, [&called, &expected](const Vector2& position, const Vector2& relativeTranslation, const Complex& relativeRotation, Float relativeScaling) {
            CORRADE_COMPARE(position, expected.position);
            CORRADE_COMPARE(relativeTranslation, expected.relativeTranslation);
            CORRADE_COMPARE(relativeRotation, expected.relativeRotation);
            CORRADE_COMPARE(relativeScaling, expected.relativeScaling);
            ++called;
        });
        CORRADE_COMPARE(dataHandleId(nodeData2), dataHandleId(nodeData));

        /* Move of the primary finger isn't even accepted now because the
           gesture recognizer doesn't track it as pressed */
        PointerMoveEvent move1{{}, PointerEventSource::Touch, {}, {}, true, 633};
        CORRADE_VERIFY(!ui.pointerMoveEvent({50, 65}, move1));
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(called, 3); /* not called */
        CORRADE_COMPARE(belowCalled, 0);

        /* Same as the initial state again */
        expected.position = {25.0f, 17.5f};
        expected.relativeTranslation = {0.0f, -5.0f};
        expected.relativeRotation = Complex::rotation(180.0_degf);
        expected.relativeScaling = 1.0f;

        PointerEvent press1{{}, PointerEventSource::Touch, Pointer::Finger, true, 633};
        PointerEvent press2{{}, PointerEventSource::Touch, Pointer::Finger, false, 3371};
        PointerMoveEvent move2{{}, PointerEventSource::Touch, {}, {}, false, 3371};
        CORRADE_VERIFY(ui.pointerPressEvent({50, 70}, press1));
        CORRADE_VERIFY(ui.pointerPressEvent({50, 75}, press2));
        CORRADE_VERIFY(ui.pointerMoveEvent({50, 65}, move2));
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentPressedNode(), node2);
        CORRADE_COMPARE(ui.currentCapturedNode(), node2);
        CORRADE_COMPARE(called, 4);
        CORRADE_COMPARE(belowCalled, 0);
    }
}

void EventLayerTest::pinchFromUserInterfaceMultipleHandlers() {
    AbstractUserInterface ui{{100, 100}};

    EventLayer& layer = ui.setLayerInstance(Containers::pointer<EventLayer>(ui.createLayer()));

    NodeHandle node = ui.createNode({25, 50}, {50, 25});

    /* With just one handler it gets called alright */
    Int called1 = 0;
    layer.onPinch(node, [&called1](const Vector2&, const Vector2&, const Complex&, Float) {
        ++called1;
    });
    {
        PointerEvent press1{{}, PointerEventSource::Touch, Pointer::Finger, true, 633};
        PointerEvent press2{{}, PointerEventSource::Touch, Pointer::Finger, false, 3371};
        PointerMoveEvent move{{}, PointerEventSource::Touch, {}, {}, false, 3371};
        CORRADE_VERIFY(ui.pointerPressEvent({50, 70}, press1));
        CORRADE_VERIFY(ui.pointerPressEvent({50, 75}, press2));
        CORRADE_VERIFY(ui.pointerMoveEvent({50, 65}, move));
        CORRADE_COMPARE(called1, 1);
    }

    /* Second handler on the same node breaks it because it's currently tracked
       on a data ID and not a node ID */
    Int called2 = 0;
    layer.onPinch(node, [&called2](const Vector2&, const Vector2&, const Complex&, Float) {
        ++called2;
    });
    {
        PointerMoveEvent move{{}, PointerEventSource::Touch, {}, {}, false, 3371};

        CORRADE_EXPECT_FAIL("Multiple onPinch() handlers on the same node conflict with each other, causing nothing to be sent.");
        CORRADE_VERIFY(ui.pointerMoveEvent({50, 75}, move));
        CORRADE_COMPARE(called1, 2);
        CORRADE_COMPARE(called2, 1);
    }
}

void EventLayerTest::pinchAndDragFromUserInterface() {
    /* Verifies that if a node has both onDrag() and onPinch(), it doesn't get
       both */

    AbstractUserInterface ui{{100, 100}};

    EventLayer& layer = ui.setLayerInstance(Containers::pointer<EventLayer>(ui.createLayer()));

    NodeHandle node = ui.createNode({25, 50}, {50, 25});

    Int pinchCalled = 0;
    Int dragCalled = 0;
    layer.onPinch(node, [&pinchCalled](const Vector2&, const Vector2&, const Complex&, Float) {
        ++pinchCalled;
    });
    layer.onDrag(node, [&dragCalled](const Vector2&) {
        ++dragCalled;
    });

    /* A press and a move of one finger calls the drag */
    {
        PointerEvent press1{{}, PointerEventSource::Touch, Pointer::Finger, true, 633};
        CORRADE_VERIFY(ui.pointerPressEvent({50, 70}, press1));
        CORRADE_COMPARE(pinchCalled, 0);
        CORRADE_COMPARE(dragCalled, 0);

        PointerMoveEvent move1{{}, PointerEventSource::Touch, {}, Pointer::Finger, true, 633};
        CORRADE_VERIFY(ui.pointerMoveEvent({50, 65}, move1));
        CORRADE_COMPARE(pinchCalled, 0);
        CORRADE_COMPARE(dragCalled, 1);

    /* A press and move of another finger calls the pinch. Since it's a
       secondary finger, it won't call the drag. */
    } {
        PointerEvent press2{{}, PointerEventSource::Touch, Pointer::Finger, false, 3371};
        CORRADE_VERIFY(ui.pointerPressEvent({50, 75}, press2));
        CORRADE_COMPARE(pinchCalled, 0);
        CORRADE_COMPARE(dragCalled, 1);

        PointerMoveEvent move2{{}, PointerEventSource::Touch, {}, Pointer::Finger, false, 3371};
        CORRADE_VERIFY(ui.pointerMoveEvent({50, 70}, move2));
        CORRADE_COMPARE(pinchCalled, 1);
        CORRADE_COMPARE(dragCalled, 1);

    /* A move of the first finger should calls the pinch again, but not drag */
    } {
        PointerMoveEvent move1{{}, PointerEventSource::Touch, {}, Pointer::Finger, true, 633};
        CORRADE_VERIFY(ui.pointerMoveEvent({50, 60}, move1));
        CORRADE_COMPARE(pinchCalled, 2);
        CORRADE_COMPARE(dragCalled, 1);

    /* A release of the second finger and a move of the first calls the drag */
    } {
        PointerEvent release2{{}, PointerEventSource::Touch, Pointer::Finger, false, 3371};
        CORRADE_VERIFY(ui.pointerReleaseEvent({50, 60}, release2));
        CORRADE_COMPARE(pinchCalled, 2);
        CORRADE_COMPARE(dragCalled, 1);

        PointerMoveEvent move1{{}, PointerEventSource::Touch, {}, Pointer::Finger, true, 633};
        CORRADE_VERIFY(ui.pointerMoveEvent({50, 60}, move1));
        CORRADE_COMPARE(pinchCalled, 2);
        CORRADE_COMPARE(dragCalled, 2);

    /* A press and move of yet another secondary finger again calls the
       pinch */
    } {
        PointerEvent press2{{}, PointerEventSource::Touch, Pointer::Finger, false, 1221};
        CORRADE_VERIFY(ui.pointerPressEvent({50, 75}, press2));
        CORRADE_COMPARE(pinchCalled, 2);
        CORRADE_COMPARE(dragCalled, 2);

        PointerMoveEvent move2{{}, PointerEventSource::Touch, {}, Pointer::Finger, false, 1221};
        CORRADE_VERIFY(ui.pointerMoveEvent({50, 70}, move2));
        CORRADE_COMPARE(pinchCalled, 3);
        CORRADE_COMPARE(dragCalled, 2);
    }
}

void EventLayerTest::enter() {
    EventLayer layer{layerHandle(0, 1)};

    Int called = 0;
    DataHandle handle = layer.onEnter(nodeHandle(0, 1), [&called]{
        ++called;
    });

    /* Should get fired for just any pointer combination being pressed. The
       AbstractLayer disallows pointerEnterEvent() for non-primary events on
       its own already. */
    {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        layer.pointerEnterEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, Pointer::MouseLeft|Pointer::MouseMiddle|Pointer::MouseRight, true, 0};
        layer.pointerEnterEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 2);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Touch, {}, Pointer::Eraser|Pointer::Pen|Pointer::Finger, true, 0};
        layer.pointerEnterEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 3);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, {}, true, 0};
        layer.pointerEnterEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 4);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Pen, Pointer::Eraser, Pointer::Finger, true, 0};
        layer.pointerEnterEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 5);

    /* Shouldn't get fired for any other than enter events */
    } {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 5);
    } {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 5);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 5);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        layer.pointerLeaveEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 5);
    } {
        FocusEvent event{{}};
        layer.focusEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 5);
    } {
        FocusEvent event{{}};
        layer.blurEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 5);
    }
}

void EventLayerTest::enterMove() {
    EventLayer layer{layerHandle(0, 1)};

    Int called = 0;
    DataHandle handle = layer.onEnter(nodeHandle(0, 1), [&called]{
        ++called;
    });

    /* The move event should get accepted for any pointer combination (but it
       has to be a primary event) in order to mark the node as hovered, and
       thus have the pointerEnterEvent() synthesized. The handler shouldn't get
       called though. */
    {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Touch, {}, Pointer::Eraser|Pointer::MouseLeft|Pointer::Finger, true, 0};
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Touch, {}, Pointer::Eraser|Pointer::MouseLeft|Pointer::Finger, false, 0};
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, Pointer::MouseRight|Pointer::MouseMiddle|Pointer::Pen, true, 0};
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, Pointer::MouseRight, {}, true, 0};
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Pen, Pointer::Pen, Pointer::MouseMiddle|Pointer::Finger, true, 0};
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(event.isAccepted());
        CORRADE_COMPARE(called, 0);

    /* Any other than move or enter events shouldn't get accepted */
    } {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        layer.pointerLeaveEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        FocusEvent event{{}};
        layer.focusEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        FocusEvent event{{}};
        layer.blurEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);

    /* Verify that the callback is actually properly registered so this doesn't
       result in false positives */
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        layer.pointerEnterEvent(dataHandleId(handle), event);
        /* The accept status is ignored for enter/leave events so the layer
           doesn't call setAccepted() */
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 1);
    }
}

void EventLayerTest::leave() {
    EventLayer layer{layerHandle(0, 1)};

    Int called = 0;
    DataHandle handle = layer.onLeave(nodeHandle(0, 1), [&called]{
        ++called;
    });

    /* Should get fired for just any pointer combination being pressed. The
       AbstractLayer disallows pointerLeaveEvent() for non-primary events on
       its own already. */
    {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        layer.pointerLeaveEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, Pointer::MouseLeft|Pointer::MouseMiddle|Pointer::MouseRight, true, 0};
        layer.pointerLeaveEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 2);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Touch, {}, Pointer::Eraser|Pointer::Pen|Pointer::Finger, true, 0};
        layer.pointerLeaveEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 3);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, {}, true, 0};
        layer.pointerLeaveEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 4);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Pen, Pointer::Eraser, Pointer::Finger, true, 0};
        layer.pointerLeaveEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 5);

    /* Shouldn't get fired for any other than leave events */
    } {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 5);
    } {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 5);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 5);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        layer.pointerEnterEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 5);
    } {
        FocusEvent event{{}};
        layer.focusEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 5);
    } {
        FocusEvent event{{}};
        layer.blurEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 5);
    }
}

void EventLayerTest::leaveMove() {
    EventLayer layer{layerHandle(0, 1)};

    Int called = 0;
    DataHandle handle = layer.onLeave(nodeHandle(0, 1), [&called]{
        ++called;
    });

    /* The move event should get accepted for any pointer combination (but it
       has to be a primary event) in order to mark the node as hovered, and
       thus have the pointerLeaveEvent() synthesized. The handler shouldn't get
       called though. */
    {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Touch, {}, Pointer::Eraser|Pointer::MouseLeft|Pointer::Finger, true, 0};
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Touch, {}, Pointer::Eraser|Pointer::MouseLeft|Pointer::Finger, false, 0};
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, Pointer::MouseRight|Pointer::MouseMiddle|Pointer::Pen, true, 0};
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, Pointer::MouseRight, {}, true, 0};
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Pen, Pointer::Pen, Pointer::MouseMiddle|Pointer::Finger, true, 0};
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(event.isAccepted());
        CORRADE_COMPARE(called, 0);

    /* Any other than move or leave events shouldn't get accepted */
    } {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        layer.pointerEnterEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        FocusEvent event{{}};
        layer.focusEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        FocusEvent event{{}};
        layer.blurEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);

    /* Verify that the callback is actually properly registered so this doesn't
       result in false positives */
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        layer.pointerLeaveEvent(dataHandleId(handle), event);
        /* The accept status is ignored for enter/leave events so the layer
           doesn't call setAccepted() */
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 1);
    }
}

void EventLayerTest::enterLeaveFromUserInterface() {
    auto&& data = FromUserInterfaceData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* "Integration" test to verify onEnter() and onLeave() behavior with the
       whole event pipeline in AbstractUserInterface.

       There's no mutual interaction between the two as with onTapOrClick()
       such as onRelease() accepting presses as well, so they're both tested
       together. */

    AbstractUserInterface ui{{100, 100}};

    EventLayer& layer = ui.setLayerInstance(Containers::pointer<EventLayer>(ui.createLayer()));

    /* A node below the one that should react to the enter/leave event,
       accepting the same. Shouldn't get considered at all. */
    Int belowCalled = 0;
    NodeHandle nodeBelow = ui.createNode({}, {100, 100}, data.flags);
    layer.onEnter(nodeBelow, [&belowCalled]{
        ++belowCalled;
    });
    layer.onLeave(nodeBelow, [&belowCalled]{
        ++belowCalled;
    });

    Int enterCalled = 0, leaveCalled = 0;
    NodeHandle node = ui.createNode(
        data.parent ? nodeBelow : NodeHandle::Null,
        {25, 50}, {50, 25});
    layer.onEnter(node, [&enterCalled]{
        ++enterCalled;
    });
    layer.onLeave(node, [&leaveCalled]{
        ++leaveCalled;
    });

    /* A move onto the node should result in the enter handler being called */
    {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        CORRADE_VERIFY(ui.pointerMoveEvent({50, 70}, event));
        CORRADE_COMPARE(ui.currentHoveredNode(), node);
        CORRADE_COMPARE(enterCalled, 1);
        CORRADE_COMPARE(leaveCalled, 0);
        CORRADE_COMPARE(belowCalled, 0);

    /* A move out (and out of the below node as well) should result in the
       leave handler being called */
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        /* There's no node underneath, so this didn't get accepted */
        CORRADE_VERIFY(!ui.pointerMoveEvent({150, 150}, event));
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(enterCalled, 1);
        CORRADE_COMPARE(leaveCalled, 1);
        CORRADE_COMPARE(belowCalled, 0);
    }
}

void EventLayerTest::focus() {
    EventLayer layer{layerHandle(0, 1)};

    Int called = 0;
    DataHandle handle = layer.onFocus(nodeHandle(0, 1), [&called]{
        ++called;
    });

    /* Should get fired for a focus event */
    {
        FocusEvent event{{}};
        layer.focusEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);

    /* Shouldn't get fired for any other than focus events */
    } {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        layer.pointerEnterEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        layer.pointerLeaveEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        FocusEvent event{{}};
        layer.blurEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    }
}

void EventLayerTest::blur() {
    EventLayer layer{layerHandle(0, 1)};

    Int called = 0;
    DataHandle handle = layer.onBlur(nodeHandle(0, 1), [&called]{
        ++called;
    });

    /* Should get fired for a blur event */
    {
        FocusEvent event{{}};
        layer.blurEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);

    /* Shouldn't get fired for any other than blur events */
    } {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        layer.pointerEnterEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        PointerMoveEvent event{{}, PointerEventSource::Mouse, {}, {}, true, 0};
        layer.pointerLeaveEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        FocusEvent event{{}};
        layer.focusEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    }
}

void EventLayerTest::focusBlurFromUserInterface() {
    auto&& data = FromUserInterfaceData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* "Integration" test to verify onFocus() and onBlur() behavior with the
       whole event pipeline in AbstractUserInterface.

       There's no mutual interaction between the two as with onTapOrClick()
       such as onRelease() accepting presses as well, so they're both tested
       together. */

    AbstractUserInterface ui{{100, 100}};

    EventLayer& layer = ui.setLayerInstance(Containers::pointer<EventLayer>(ui.createLayer()));

    /* A node below the one that should react to the focus/blur event,
       accepting the same. Shouldn't get considered at all. */
    Int belowCalled = 0;
    NodeHandle nodeBelow = ui.createNode({}, {100, 100}, data.flags|NodeFlag::Focusable);
    layer.onFocus(nodeBelow, [&belowCalled]{
        ++belowCalled;
    });
    layer.onBlur(nodeBelow, [&belowCalled]{
        ++belowCalled;
    });

    Int focusCalled = 0, blurCalled = 0;
    NodeHandle node = ui.createNode(
        data.parent ? nodeBelow : NodeHandle::Null,
        {25, 50}, {50, 25}, NodeFlag::Focusable);
    layer.onFocus(node, [&focusCalled]{
        ++focusCalled;
    });
    layer.onBlur(node, [&blurCalled]{
        ++blurCalled;
    });

    /* Focusing and blurring the node directly should work */
    {
        FocusEvent event{{}};
        CORRADE_VERIFY(ui.focusEvent(node, event));
        CORRADE_COMPARE(ui.currentFocusedNode(), node);
        CORRADE_COMPARE(focusCalled, 1);
        CORRADE_COMPARE(blurCalled, 0);
        CORRADE_COMPARE(belowCalled, 0);
    } {
        FocusEvent event{{}};
        CORRADE_VERIFY(!ui.focusEvent(NodeHandle::Null, event));
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE(focusCalled, 1);
        CORRADE_COMPARE(blurCalled, 1);
        CORRADE_COMPARE(belowCalled, 0);

    /* A press on the node result in the focus handler being called as well,
       i.e. it should accept the event here as well */
    } {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        CORRADE_VERIFY(ui.pointerPressEvent({50, 70}, event));
        CORRADE_COMPARE(ui.currentFocusedNode(), node);
        CORRADE_COMPARE(focusCalled, 2);
        CORRADE_COMPARE(blurCalled, 1);
        CORRADE_COMPARE(belowCalled, 0);

    /* A press outside (and out of the below node as well) should result in the
       blur handler being called */
    } {
        PointerEvent event{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0};
        /* There's no node underneath, so this didn't get accepted */
        CORRADE_VERIFY(!ui.pointerPressEvent({150, 150}, event));
        CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);
        CORRADE_COMPARE(focusCalled, 2);
        CORRADE_COMPARE(blurCalled, 2);
        CORRADE_COMPARE(belowCalled, 0);
    }
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::EventLayerTest)
