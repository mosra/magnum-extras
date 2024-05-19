/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023 Vladimír Vondruš <mosra@centrum.cz>

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

#include "Magnum/Whee/AbstractUserInterface.h"
#include "Magnum/Whee/EventLayer.h"
#include "Magnum/Whee/Event.h"
#include "Magnum/Whee/Handle.h"

namespace Magnum { namespace Whee { namespace Test { namespace {

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

    void call();

    void connect();
    void connectScoped();

    void press();
    void tapOrClick();
    void tapOrClickPressRelease();
    void tapOrClickFromUserInterface();

    void middleClick();
    void middleClickPressRelease();
    void middleClickFromUserInterface();

    void rightClick();
    void rightClickPressRelease();
    void rightClickFromUserInterface();

    void drag();
    void dragPress();
    void dragFromUserInterface();

    void remove();
    void removeScoped();
    void cleanNodes();
};

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
    const char* name;
    DataHandle(*functor)(EventLayer&, NodeHandle, Int& output);
    EventConnection(*functorScoped)(EventLayer&, NodeHandle, Int& output);
    void(*call)(EventLayer& layer, UnsignedInt dataId);
} ConnectData[]{
    #define _c(name, ...) #name,                                            \
        [](EventLayer& layer, NodeHandle node, Int& output) {               \
            ConnectFunctor<__VA_ARGS__> functor{output};                    \
            return layer.name(node, functor);                               \
        },                                                                  \
        [](EventLayer& layer, NodeHandle node, Int& output) {               \
            ConnectFunctor<__VA_ARGS__> functor{output};                    \
            return layer.name ## Scoped(node, functor);                     \
        }
    {_c(onPress, ),
        [](EventLayer& layer, UnsignedInt dataId) {
            PointerEvent event{Pointer::MouseLeft};
            layer.pointerPressEvent(dataId, event);
        }},
    {_c(onTapOrClick, ),
        [](EventLayer& layer, UnsignedInt dataId) {
            PointerEvent event{Pointer::MouseLeft};
            layer.pointerTapOrClickEvent(dataId, event);
        }},
    {_c(onMiddleClick, ),
        [](EventLayer& layer, UnsignedInt dataId) {
            PointerEvent event{Pointer::MouseMiddle};
            layer.pointerTapOrClickEvent(dataId, event);
        }},
    {_c(onRightClick, ),
        [](EventLayer& layer, UnsignedInt dataId) {
            PointerEvent event{Pointer::MouseRight};
            layer.pointerTapOrClickEvent(dataId, event);
        }},
    {_c(onDrag, const Vector2&),
        [](EventLayer& layer, UnsignedInt dataId) {
            PointerMoveEvent event{{}, Pointer::MouseLeft};
            layer.pointerMoveEvent(dataId, event);
        }},
    #undef _c
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

              &EventLayerTest::call});

    addInstancedTests({&EventLayerTest::connect,
                       &EventLayerTest::connectScoped},
        Containers::arraySize(ConnectData));

    addTests({&EventLayerTest::press,

              &EventLayerTest::tapOrClick,
              &EventLayerTest::tapOrClickPressRelease,
              &EventLayerTest::tapOrClickFromUserInterface,

              &EventLayerTest::middleClick,
              &EventLayerTest::middleClickPressRelease,
              &EventLayerTest::middleClickFromUserInterface,

              &EventLayerTest::rightClick,
              &EventLayerTest::rightClickPressRelease,
              &EventLayerTest::rightClickFromUserInterface,

              &EventLayerTest::drag,
              &EventLayerTest::dragPress,
              &EventLayerTest::dragFromUserInterface,

              &EventLayerTest::remove,
              &EventLayerTest::removeScoped,
              &EventLayerTest::cleanNodes});
}

void EventLayerTest::eventConnectionConstruct() {
    EventLayer layer{layerHandle(0, 2)};

    EventConnection a = layer.onTapOrClickScoped(NodeHandle::Null, []{});
    CORRADE_COMPARE(&a.layer(), &layer);
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
        CORRADE_COMPARE(out.str(), "Whee::EventLayer: 2 scoped connections already active, can't move\n");

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
            "Whee::EventLayer: 3 scoped connections already active in the moved-from object, can't move\n"
            "Whee::EventLayer: 3 scoped connections already active in the moved-to object, can't move\n");

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
    CORRADE_COMPARE(out.str(), "Whee::EventLayer: destructed with 2 scoped connections still active\n");

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
        DataHandle handle = layer.onTapOrClick(NodeHandle::Null,  functor);
        /* Constructed a local instance (100) and copy-constructed it to the
           layer (1000) */
        CORRADE_COMPARE(functorCalledConstructedDestructedCount, 1100);
        CORRADE_COMPARE(handle, dataHandle(layer.handle(), 0, 1));
        CORRADE_COMPARE(layer.usedScopedConnectionCount(), 0);
        CORRADE_COMPARE(layer.usedAllocatedConnectionCount(), 1);

        PointerEvent event{Pointer::MouseLeft};
        layer.pointerTapOrClickEvent(0, event);
        /* Called it (1) */
        CORRADE_COMPARE(functorCalledConstructedDestructedCount, 1101);
    }

    /* Destructed the original instance and the copy in the layer (20) */
    CORRADE_COMPARE(functorCalledConstructedDestructedCount, 1121);
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

void EventLayerTest::press() {
    EventLayer layer{layerHandle(0, 1)};

    Int called = 0;
    DataHandle handle = layer.onPress(nodeHandle(0, 1), [&called]{
        ++called;
    });

    /* Should only get fired for mouse left, finger or pen */
    {
        PointerEvent event{Pointer::MouseLeft};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        PointerEvent event{Pointer::MouseMiddle};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        PointerEvent event{Pointer::MouseRight};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        PointerEvent event{Pointer::Finger};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 2);
    } {
        PointerEvent event{Pointer::Pen};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 3);
    } {
        PointerEvent event{Pointer::Eraser};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 3);

    /* Shouldn't get fired for any other than press events */
    } {
        PointerEvent event{Pointer::MouseLeft};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 3);
    } {
        PointerEvent event{Pointer::MouseLeft};
        layer.pointerTapOrClickEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 3);
    } {
        PointerMoveEvent event{Pointer::MouseLeft, Pointer::MouseLeft};
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 3);
    } {
        PointerMoveEvent event{Pointer::MouseLeft, Pointer::MouseLeft};
        layer.pointerEnterEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 3);
    } {
        PointerMoveEvent event{Pointer::MouseLeft, Pointer::MouseLeft};
        layer.pointerLeaveEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 3);
    }
}

void EventLayerTest::tapOrClick() {
    EventLayer layer{layerHandle(0, 1)};

    Int called = 0;
    DataHandle handle = layer.onTapOrClick(nodeHandle(0, 1), [&called]{
        ++called;
    });

    /* Should only get fired for mouse left, finger or pen */
    {
        PointerEvent event{Pointer::MouseLeft};
        layer.pointerTapOrClickEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        PointerEvent event{Pointer::MouseMiddle};
        layer.pointerTapOrClickEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        PointerEvent event{Pointer::MouseRight};
        layer.pointerTapOrClickEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        PointerEvent event{Pointer::Finger};
        layer.pointerTapOrClickEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 2);
    } {
        PointerEvent event{Pointer::Pen};
        layer.pointerTapOrClickEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 3);
    } {
        PointerEvent event{Pointer::Eraser};
        layer.pointerTapOrClickEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 3);

    /* Shouldn't get fired for any other than tapOrClick events */
    } {
        PointerEvent event{Pointer::MouseLeft};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 3);
    } {
        PointerEvent event{Pointer::MouseLeft};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 3);
    } {
        PointerMoveEvent event{Pointer::MouseLeft, Pointer::MouseLeft};
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 3);
    } {
        PointerMoveEvent event{Pointer::MouseLeft, Pointer::MouseLeft};
        layer.pointerEnterEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 3);
    } {
        PointerMoveEvent event{Pointer::MouseLeft, Pointer::MouseLeft};
        layer.pointerLeaveEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 3);
    }
}

void EventLayerTest::tapOrClickPressRelease() {
    EventLayer layer{layerHandle(0, 1)};

    Int called = 0;
    DataHandle handle = layer.onTapOrClick(nodeHandle(0, 1), [&called]{
        ++called;
    });

    /* The press event should get accepted for mouse left, finger or pen to
       prevent it from being propagated further if no other data accepts it.
       The handler shouldn't get called though. */
    {
        PointerEvent event{Pointer::MouseLeft};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{Pointer::MouseMiddle};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{Pointer::MouseRight};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{Pointer::Finger};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{Pointer::Pen};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{Pointer::Eraser};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);

    /* Similarly for release */
    } {
        PointerEvent event{Pointer::MouseLeft};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{Pointer::MouseMiddle};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{Pointer::MouseRight};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{Pointer::Finger};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{Pointer::Pen};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{Pointer::Eraser};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);

    /* Any other than press, release or tapOrClick event shouldn't get
       accepted */
    } {
        PointerMoveEvent event{Pointer::MouseLeft, Pointer::MouseLeft};
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerMoveEvent event{Pointer::MouseLeft, Pointer::MouseLeft};
        layer.pointerEnterEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerMoveEvent event{Pointer::MouseLeft, Pointer::MouseLeft};
        layer.pointerLeaveEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);

    /* Verify that the callback is actually properly registered so this doesn't
       result in false positives */
    } {
        PointerEvent event{Pointer::MouseLeft};
        layer.pointerTapOrClickEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(event.isAccepted());
        CORRADE_COMPARE(called, 1);
    }
}

void EventLayerTest::tapOrClickFromUserInterface() {
    /* "Integration" test to verify onTapOrClick() behavior with the whole
       event pipeline in AbstractUserInterface */

    AbstractUserInterface ui{{100, 100}};

    EventLayer& layer = ui.setLayerInstance(Containers::pointer<EventLayer>(ui.createLayer()));

    /* A node below the one that should react to the tap or click event,
       accepting presses. Shouldn't get considered at all. */
    Int belowCalled = 0;
    NodeHandle nodeBelow = ui.createNode({}, {100, 100});
    layer.onPress(nodeBelow, [&belowCalled]{
        ++belowCalled;
    });

    Int called = 0;
    NodeHandle node = ui.createNode({25, 50}, {50, 25});
    layer.onTapOrClick(node, [&called]{
        ++called;
    });

    /* A press should be accepted but not resulting in the handler being
       called */
    {
        PointerEvent event{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerPressEvent({50, 70}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), node);
        CORRADE_COMPARE(ui.currentCapturedNode(), node);
        CORRADE_COMPARE(called, 0);
        CORRADE_COMPARE(belowCalled, 0);

    /* A release should be accepted as well, resulting in the handler being
       called */
    } {
        PointerEvent event{Pointer::MouseLeft};
        CORRADE_VERIFY(ui.pointerReleaseEvent({50, 65}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(called, 1);
        CORRADE_COMPARE(belowCalled, 0);
    }
}

void EventLayerTest::middleClick() {
    EventLayer layer{layerHandle(0, 1)};

    Int called = 0;
    DataHandle handle = layer.onMiddleClick(nodeHandle(0, 1), [&called]{
        ++called;
    });

    /* Shouldn't get fired for anything else than mouse middle */
    {
        PointerEvent event{Pointer::MouseLeft};
        layer.pointerTapOrClickEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{Pointer::MouseMiddle};
        layer.pointerTapOrClickEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        PointerEvent event{Pointer::MouseRight};
        layer.pointerTapOrClickEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        PointerEvent event{Pointer::Finger};
        layer.pointerTapOrClickEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        PointerEvent event{Pointer::Pen};
        layer.pointerTapOrClickEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        PointerEvent event{Pointer::Eraser};
        layer.pointerTapOrClickEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);

    /* Shouldn't get fired for any other than tapOrClick events */
    } {
        PointerEvent event{Pointer::MouseMiddle};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        PointerEvent event{Pointer::MouseMiddle};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        PointerMoveEvent event{Pointer::MouseMiddle, Pointer::MouseMiddle};
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        PointerMoveEvent event{Pointer::MouseMiddle, Pointer::MouseMiddle};
        layer.pointerEnterEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        PointerMoveEvent event{Pointer::MouseMiddle, Pointer::MouseMiddle};
        layer.pointerLeaveEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    }
}

void EventLayerTest::middleClickPressRelease() {
    EventLayer layer{layerHandle(0, 1)};

    Int called = 0;
    DataHandle handle = layer.onMiddleClick(nodeHandle(0, 1), [&called]{
        ++called;
    });

    /* The press event should get accepted for mouse middle to prevent it from
       being propagated further if no other data accepts it. The handler
       shouldn't get called though. */
    {
        PointerEvent event{Pointer::MouseLeft};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{Pointer::MouseMiddle};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{Pointer::MouseRight};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{Pointer::Finger};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{Pointer::Pen};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{Pointer::Eraser};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);

    /* Similarly for release */
    } {
        PointerEvent event{Pointer::MouseLeft};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{Pointer::MouseMiddle};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{Pointer::MouseRight};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{Pointer::Finger};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{Pointer::Pen};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{Pointer::Eraser};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);

    /* Any other than press, release or tapOrClick event shouldn't get
       accepted */
    } {
        PointerMoveEvent event{Pointer::MouseMiddle, Pointer::MouseMiddle};
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerMoveEvent event{Pointer::MouseMiddle, Pointer::MouseMiddle};
        layer.pointerEnterEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerMoveEvent event{Pointer::MouseMiddle, Pointer::MouseMiddle};
        layer.pointerLeaveEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);

    /* Verify that the callback is actually properly registered so this doesn't
       result in false positives */
    } {
        PointerEvent event{Pointer::MouseMiddle};
        layer.pointerTapOrClickEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(event.isAccepted());
        CORRADE_COMPARE(called, 1);
    }
}

void EventLayerTest::middleClickFromUserInterface() {
    /* "Integration" test to verify onTapOrClick() behavior with the whole
       event pipeline in AbstractUserInterface */

    AbstractUserInterface ui{{100, 100}};

    EventLayer& layer = ui.setLayerInstance(Containers::pointer<EventLayer>(ui.createLayer()));

    /* A node below the one that should react to the middle click event,
       accepting presses. Shouldn't get considered at all. */
    Int belowCalled = 0;
    NodeHandle nodeBelow = ui.createNode({}, {100, 100});
    layer.onPress(nodeBelow, [&belowCalled]{
        ++belowCalled;
    });

    Int called = 0;
    NodeHandle node = ui.createNode({25, 50}, {50, 25});
    layer.onMiddleClick(node, [&called]{
        ++called;
    });

    /* A press should be accepted but not resulting in the handler being
       called */
    {
        PointerEvent event{Pointer::MouseMiddle};
        CORRADE_VERIFY(ui.pointerPressEvent({50, 70}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), node);
        CORRADE_COMPARE(ui.currentCapturedNode(), node);
        CORRADE_COMPARE(called, 0);
        CORRADE_COMPARE(belowCalled, 0);

    /* A release should be accepted as well, resulting in the handler being
       called */
    } {
        PointerEvent event{Pointer::MouseMiddle};
        CORRADE_VERIFY(ui.pointerReleaseEvent({50, 65}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(called, 1);
        CORRADE_COMPARE(belowCalled, 0);
    }
}

void EventLayerTest::rightClick() {
    EventLayer layer{layerHandle(0, 1)};

    Int called = 0;
    DataHandle handle = layer.onRightClick(nodeHandle(0, 1), [&called]{
        ++called;
    });

    /* Shouldn't get fired for anything else than mouse right */
    {
        PointerEvent event{Pointer::MouseLeft};
        layer.pointerTapOrClickEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{Pointer::MouseMiddle};
        layer.pointerTapOrClickEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{Pointer::MouseRight};
        layer.pointerTapOrClickEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        PointerEvent event{Pointer::Finger};
        layer.pointerTapOrClickEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        PointerEvent event{Pointer::Pen};
        layer.pointerTapOrClickEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        PointerEvent event{Pointer::Eraser};
        layer.pointerTapOrClickEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);

    /* Shouldn't get fired for any other than tapOrClick events */
    } {
        PointerEvent event{Pointer::MouseMiddle};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        PointerEvent event{Pointer::MouseMiddle};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        PointerMoveEvent event{Pointer::MouseMiddle, Pointer::MouseMiddle};
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        PointerMoveEvent event{Pointer::MouseMiddle, Pointer::MouseMiddle};
        layer.pointerEnterEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        PointerMoveEvent event{Pointer::MouseMiddle, Pointer::MouseMiddle};
        layer.pointerLeaveEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    }
}

void EventLayerTest::rightClickPressRelease() {
    EventLayer layer{layerHandle(0, 1)};

    Int called = 0;
    DataHandle handle = layer.onRightClick(nodeHandle(0, 1), [&called]{
        ++called;
    });

    /* The press event should get accepted for mouse right to prevent it from
       being propagated further if no other data accepts it. The handler
       shouldn't get called though. */
    {
        PointerEvent event{Pointer::MouseLeft};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{Pointer::MouseMiddle};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{Pointer::MouseRight};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{Pointer::Finger};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{Pointer::Pen};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{Pointer::Eraser};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);

    /* Similarly for release */
    } {
        PointerEvent event{Pointer::MouseLeft};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{Pointer::MouseMiddle};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{Pointer::MouseRight};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{Pointer::Finger};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{Pointer::Pen};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{Pointer::Eraser};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);

    /* Any other than press, release or tapOrClick event shouldn't get
       accepted */
    } {
        PointerMoveEvent event{Pointer::MouseRight, Pointer::MouseRight};
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerMoveEvent event{Pointer::MouseRight, Pointer::MouseRight};
        layer.pointerEnterEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerMoveEvent event{Pointer::MouseRight, Pointer::MouseRight};
        layer.pointerLeaveEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);

    /* Verify that the callback is actually properly registered so this doesn't
       result in false positives */
    } {
        PointerEvent event{Pointer::MouseRight};
        layer.pointerTapOrClickEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(event.isAccepted());
        CORRADE_COMPARE(called, 1);
    }
}

void EventLayerTest::rightClickFromUserInterface() {
    /* "Integration" test to verify onTapOrClick() behavior with the whole
       event pipeline in AbstractUserInterface */

    AbstractUserInterface ui{{100, 100}};

    EventLayer& layer = ui.setLayerInstance(Containers::pointer<EventLayer>(ui.createLayer()));

    /* A node below the one that should react to the right click event,
       accepting presses. Shouldn't get considered at all. */
    Int belowCalled = 0;
    NodeHandle nodeBelow = ui.createNode({}, {100, 100});
    layer.onPress(nodeBelow, [&belowCalled]{
        ++belowCalled;
    });

    Int called = 0;
    NodeHandle node = ui.createNode({25, 50}, {50, 25});
    layer.onRightClick(node, [&called]{
        ++called;
    });

    /* A press should be accepted but not resulting in the handler being
       called */
    {
        PointerEvent event{Pointer::MouseRight};
        CORRADE_VERIFY(ui.pointerPressEvent({50, 70}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), node);
        CORRADE_COMPARE(ui.currentCapturedNode(), node);
        CORRADE_COMPARE(called, 0);
        CORRADE_COMPARE(belowCalled, 0);

    /* A release should be accepted as well, resulting in the handler being
       called */
    } {
        PointerEvent event{Pointer::MouseRight};
        CORRADE_VERIFY(ui.pointerReleaseEvent({50, 65}, event));
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(called, 1);
        CORRADE_COMPARE(belowCalled, 0);
    }
}

void EventLayerTest::drag() {
    EventLayer layer{layerHandle(0, 1)};

    Int called = 0;
    Vector2 calledOffset;
    DataHandle handle = layer.onDrag(nodeHandle(0, 1), [&called, &calledOffset](const Vector2& offset){
        ++called;
        calledOffset += offset;
    });

    /* Should only get fired for a move with mouse left, finger or pen present
       among types(). The type() isn't considered in any way, as it could
       signalize a newly pressed pointer but also a no longer pressed one;
       extra pressed pointers are ignored as well. */
    {
        PointerMoveEvent event{{}, {}};
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 0);
    } {
        PointerMoveEvent event{{}, Pointer::MouseLeft|Pointer::MouseRight, {-1.0f, 2.4f}};
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
        CORRADE_COMPARE(calledOffset, (Vector2{-1.0f, 2.4f}));
    } {
        PointerMoveEvent event{{}, Pointer::MouseMiddle};
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        PointerMoveEvent event{{}, Pointer::MouseRight};
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 1);
    } {
        PointerMoveEvent event{{}, Pointer::Finger|Pointer::Eraser, {0.5f, -1.0f}};
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 2);
        CORRADE_COMPARE(calledOffset, (Vector2{-0.5f, 1.4f}));
    } {
        PointerMoveEvent event{{}, Pointer::Pen|Pointer::Eraser, {1.0f, -0.5f}};
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 3);
        CORRADE_COMPARE(calledOffset, (Vector2{0.5f, 0.9f}));
    } {
        PointerMoveEvent event{{}, Pointer::Eraser};
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 3);

    /* Shouldn't get fired for any other than move events */
    } {
        PointerEvent event{Pointer::MouseLeft};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 3);
    } {
        PointerEvent event{Pointer::MouseLeft};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 3);
    } {
        PointerEvent event{Pointer::MouseLeft};
        layer.pointerTapOrClickEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 3);
    } {
        PointerMoveEvent event{Pointer::MouseLeft, Pointer::MouseLeft};
        layer.pointerEnterEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 3);
    } {
        PointerMoveEvent event{Pointer::MouseLeft, Pointer::MouseLeft};
        layer.pointerLeaveEvent(dataHandleId(handle), event);
        CORRADE_COMPARE(called, 3);
    }
}

void EventLayerTest::dragPress() {
    EventLayer layer{layerHandle(0, 1)};

    Int called = 0;
    DataHandle handle = layer.onDrag(nodeHandle(0, 1), [&called](const Vector2&){
        ++called;
    });

    /* The press event should get accepted for mouse left, finger or pen to
       prevent it from being propagated further if no other data accepts it.
       The handler shouldn't get called though. */
    {
        PointerEvent event{Pointer::MouseLeft};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{Pointer::MouseMiddle};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{Pointer::MouseRight};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{Pointer::Finger};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{Pointer::Pen};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{Pointer::Eraser};
        layer.pointerPressEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);

    /* Any other than press or move event shouldn't get accepted */
    } {
        PointerEvent event{Pointer::MouseLeft};
        layer.pointerReleaseEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerEvent event{Pointer::MouseLeft};
        layer.pointerTapOrClickEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerMoveEvent event{Pointer::MouseLeft, Pointer::MouseLeft};
        layer.pointerEnterEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);
    } {
        PointerMoveEvent event{Pointer::MouseLeft, Pointer::MouseLeft};
        layer.pointerLeaveEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(!event.isAccepted());
        CORRADE_COMPARE(called, 0);

    /* Verify that the callback is actually properly registered so this doesn't
       result in false positives */
    } {
        PointerMoveEvent event{Pointer::MouseLeft, Pointer::MouseLeft};
        layer.pointerMoveEvent(dataHandleId(handle), event);
        CORRADE_VERIFY(event.isAccepted());
        CORRADE_COMPARE(called, 1);
    }
}

void EventLayerTest::dragFromUserInterface() {
    /* "Integration" test to verify onDrag() behavior with the whole event
       pipeline in AbstractUserInterface */

    AbstractUserInterface ui{{100, 100}};

    EventLayer& layer = ui.setLayerInstance(Containers::pointer<EventLayer>(ui.createLayer()));

    /* A node below the one that should react to the drag event, accepting
       presses. Shouldn't get considered at all. */
    Int belowCalled = 0;
    NodeHandle nodeBelow = ui.createNode({}, {100, 100});
    layer.onPress(nodeBelow, [&belowCalled]{
        ++belowCalled;
    });

    NodeHandle node = ui.createNode({25, 50}, {50, 25});

    Int called = 0;
    layer.onDrag(node, [&called](const Vector2&){
        ++called;
    });

    /* A move alone with a button pressed should be accepted even though it
       doesn't cause any node to get registered as pressed or captured */
    {
        PointerMoveEvent event{{}, Pointer::Finger};
        CORRADE_VERIFY(ui.pointerMoveEvent({50, 70}, event));
        CORRADE_COMPARE(ui.currentHoveredNode(), node);
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(called, 1);
        CORRADE_COMPARE(belowCalled, 0);

    /* Another move without a button pressed should be ignored */
    } {
        PointerMoveEvent event{{}, {}};
        CORRADE_VERIFY(!ui.pointerMoveEvent({50, 65}, event));
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentPressedNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentCapturedNode(), NodeHandle::Null);
        CORRADE_COMPARE(called, 1);
        CORRADE_COMPARE(belowCalled, 0);

    /* A press should be accepted but not resulting in the handler being
       called */
    } {
        PointerEvent event{Pointer::Pen};
        CORRADE_VERIFY(ui.pointerPressEvent({50, 70}, event));
        CORRADE_COMPARE(ui.currentHoveredNode(), NodeHandle::Null);
        CORRADE_COMPARE(ui.currentPressedNode(), node);
        CORRADE_COMPARE(ui.currentCapturedNode(), node);
        CORRADE_COMPARE(called, 1);
        CORRADE_COMPARE(belowCalled, 0);

    /* A move after a press should then be treated as a drag */
    } {
        PointerMoveEvent event{{}, Pointer::Pen};
        CORRADE_VERIFY(ui.pointerMoveEvent({45, 60}, event));
        CORRADE_COMPARE(ui.currentHoveredNode(), node);
        CORRADE_COMPARE(ui.currentPressedNode(), node);
        CORRADE_COMPARE(ui.currentCapturedNode(), node);
        CORRADE_COMPARE(called, 2);
        CORRADE_COMPARE(belowCalled, 0);
    }
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

}}}}

CORRADE_TEST_MAIN(Magnum::Whee::Test::EventLayerTest)
