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
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/Numeric.h>
#include <Corrade/Utility/DebugStl.h> /** @todo remove once Debug is stream-free */

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

    void callFreeFunction();
    void callFunctor();
    void callLargeFunctor();
    void callNonTrivialFunctor();
    void callNonCopyableFunctor();
    void callMemberFunction();
    void callMemberFunctionInBase();
    void callMemberFunctionMultipleInheritance();
    void callMemberFunctionMultipleVirtualInheritance();

    void connectFreeFunction();
    void connectFreeFunctionScoped();
    void connectFunctor();
    void connectFunctorScoped();
    void connectFunctorNonTrivial();
    void connectFunctorNonTrivialScoped();
    void connectFunctorNonCopyable();
    void connectFunctorNonCopyableScoped();
    void connectFunctorNonCopyableNonTrivial();
    void connectFunctorNonCopyableNonTrivialScoped();
    void connectMemberFunction();
    void connectMemberFunctionScoped();

    void tapOrClick();
    void middleClick();
    void rightClick();
    void drag();

    void remove();
    void removeScoped();
    void cleanNodes();
};

Int freeFunctionOutput = 1;

template<class ...Args> void connectFreeFunction(Args...) {
    freeFunctionOutput *= 2;
}
/* The functor is passed as a mutable reference and tests that it isn't
   accidentally moved */
template<class ...Args> struct ConnectFunctor {
    explicit ConnectFunctor(int& output): output{&output} {}
    ConnectFunctor(const ConnectFunctor&) = default;
    ConnectFunctor(ConnectFunctor&&) {
        CORRADE_FAIL("A move should not happen");
    }
    ConnectFunctor& operator=(const ConnectFunctor&) = default;
    ConnectFunctor& operator=(ConnectFunctor&&) {
        CORRADE_FAIL("A move should not happen");
        return *this;
    }
    void operator()(Args...) {
        *output *= 3;
    }

    Int* output;
};
/* Same as ConnectFunctor but also with a non-trivial destructor to test the
   "should not move" behavior in the non-trivial case */
template<class ...Args> struct ConnectFunctorNonTrivial {
    explicit ConnectFunctorNonTrivial(int& output): output{&output} {}
    ConnectFunctorNonTrivial(const ConnectFunctorNonTrivial&) = default;
    ConnectFunctorNonTrivial(ConnectFunctorNonTrivial&&) {
        CORRADE_FAIL("A move should not happen");
    }
    ~ConnectFunctorNonTrivial() {
        *output *= 5;
    }
    ConnectFunctorNonTrivial& operator=(const ConnectFunctorNonTrivial&) = default;
    ConnectFunctorNonTrivial& operator=(ConnectFunctorNonTrivial&&) {
        CORRADE_FAIL("A move should not happen");
        return *this;
    }
    void operator()(Args...) {
        *output *= 7;
    }

    Int* output;
};
/* It's non-copyable but is std::is_trivially_destructible. And, except for
   Clang, std::is_trivially_copyable it is also, as explained here -- it
   wouldn't be if any of the move constructors did something extra, like
   setting *output: https://www.foonathan.net/2021/03/trivially-copyable/
   Compared to ConnectFunctor it should get moved and not copied. */
template<class ...Args> struct ConnectFunctorNonCopyable {
    explicit ConnectFunctorNonCopyable(int& output): output{&output} {}
    ConnectFunctorNonCopyable(const ConnectFunctorNonCopyable&) = delete;
    ConnectFunctorNonCopyable(ConnectFunctorNonCopyable&&) = default;
    ConnectFunctorNonCopyable& operator=(const ConnectFunctorNonCopyable&) = delete;
    ConnectFunctorNonCopyable& operator=(ConnectFunctorNonCopyable&&) = default;
    void operator()(Args...) {
        *output *= 11;
    }

    Int* output;
};
template<class ...Args> struct ConnectFunctorNonCopyableNonTrivial {
    explicit ConnectFunctorNonCopyableNonTrivial(int& output): output{&output} {}
    ConnectFunctorNonCopyableNonTrivial(const ConnectFunctorNonCopyableNonTrivial&) = delete;
    ConnectFunctorNonCopyableNonTrivial(ConnectFunctorNonCopyableNonTrivial&&) = default;
    ~ConnectFunctorNonCopyableNonTrivial() {
        *output *= 13;
    }
    ConnectFunctorNonCopyableNonTrivial& operator=(const ConnectFunctorNonCopyableNonTrivial&) = delete;
    ConnectFunctorNonCopyableNonTrivial& operator=(ConnectFunctorNonCopyableNonTrivial&&) = default;
    void operator()(Args...) {
        *output *= 17;
    }

    Int* output;
};
struct ConnectMemberBase {
    /* Deliberately defined in a different type to test the worse scenario */
    template<class ...Args> void call(Args...) {
        output *= 19;
    }

    Int output = 1;
};
struct ConnectMember: ConnectMemberBase {};

const struct {
    const char* name;
    DataHandle(*freeFunction)(EventLayer&, NodeHandle);
    EventConnection(*freeFunctionScoped)(EventLayer&, NodeHandle);
    DataHandle(*functor)(EventLayer&, NodeHandle, Int& output);
    EventConnection(*functorScoped)(EventLayer&, NodeHandle, Int& output);
    DataHandle(*functorNonTrivial)(EventLayer&, NodeHandle, Int& output);
    EventConnection(*functorNonTrivialScoped)(EventLayer&, NodeHandle, Int& output);
    DataHandle(*functorNonCopyable)(EventLayer&, NodeHandle, Int& output);
    EventConnection(*functorNonCopyableScoped)(EventLayer&, NodeHandle, Int& output);
    DataHandle(*functorNonCopyableNonTrivial)(EventLayer&, NodeHandle, Int& output);
    EventConnection(*functorNonCopyableNonTrivialScoped)(EventLayer&, NodeHandle, Int& output);
    DataHandle(*memberFunction)(EventLayer&, NodeHandle, ConnectMember& receiver);
    EventConnection(*memberFunctionScoped)(EventLayer&, NodeHandle, ConnectMember& receiver);
    void(*call)(EventLayer& layer, UnsignedInt dataId);
} ConnectData[]{
    #define _c(name, ...) #name,                                           \
        [](EventLayer& layer, NodeHandle node) {                            \
            return layer.name(node, connectFreeFunction<__VA_ARGS__>);      \
        },                                                                  \
        [](EventLayer& layer, NodeHandle node) {                            \
            return layer.name ## Scoped(node, connectFreeFunction<__VA_ARGS__>); \
        },                                                                  \
        [](EventLayer& layer, NodeHandle node, Int& output) {               \
            ConnectFunctor<__VA_ARGS__> functor{output};                    \
            return layer.name(node, functor);                               \
        },                                                                  \
        [](EventLayer& layer, NodeHandle node, Int& output) {               \
            ConnectFunctor<__VA_ARGS__> functor{output};                    \
            return layer.name ## Scoped(node, functor);                     \
        },                                                                  \
        [](EventLayer& layer, NodeHandle node, Int& output) {               \
            ConnectFunctorNonTrivial<__VA_ARGS__> functor{output};          \
            return layer.name(node, functor);                               \
        },                                                                  \
        [](EventLayer& layer, NodeHandle node, Int& output) {               \
            ConnectFunctorNonTrivial<__VA_ARGS__> functor{output};          \
            return layer.name ## Scoped(node, functor);                     \
        },                                                                  \
        [](EventLayer& layer, NodeHandle node, Int& output) {               \
            return layer.name(node, ConnectFunctorNonCopyable<__VA_ARGS__>{output}); \
        },                                                                  \
        [](EventLayer& layer, NodeHandle node, Int& output) {               \
            return layer.name ## Scoped(node, ConnectFunctorNonCopyable<__VA_ARGS__>{output}); \
        },                                                                  \
        [](EventLayer& layer, NodeHandle node, Int& output) {               \
            return layer.name(node, ConnectFunctorNonCopyableNonTrivial<__VA_ARGS__>{output}); \
        },                                                                  \
        [](EventLayer& layer, NodeHandle node, Int& output) {               \
            return layer.name ## Scoped(node, ConnectFunctorNonCopyableNonTrivial<__VA_ARGS__>{output}); \
        },                                                                  \
        [](EventLayer& layer, NodeHandle node, ConnectMember& receiver) {   \
            return layer.name(node, receiver, &ConnectMemberBase::call<__VA_ARGS__>); \
        },                                                                  \
        [](EventLayer& layer, NodeHandle node, ConnectMember& receiver) {   \
            return layer.name ## Scoped(node, receiver, &ConnectMemberBase::call<__VA_ARGS__>); \
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

              &EventLayerTest::callFreeFunction,
              &EventLayerTest::callFunctor,
              &EventLayerTest::callLargeFunctor,
              &EventLayerTest::callNonTrivialFunctor,
              &EventLayerTest::callNonCopyableFunctor,
              &EventLayerTest::callMemberFunction,
              &EventLayerTest::callMemberFunctionInBase,
              &EventLayerTest::callMemberFunctionMultipleInheritance,
              &EventLayerTest::callMemberFunctionMultipleVirtualInheritance});

    addInstancedTests({&EventLayerTest::connectFreeFunction,
                       &EventLayerTest::connectFreeFunctionScoped,
                       &EventLayerTest::connectFunctor,
                       &EventLayerTest::connectFunctorScoped,
                       &EventLayerTest::connectFunctorNonTrivial,
                       &EventLayerTest::connectFunctorNonTrivialScoped,
                       &EventLayerTest::connectFunctorNonCopyable,
                       &EventLayerTest::connectFunctorNonCopyableScoped,
                       &EventLayerTest::connectFunctorNonCopyableNonTrivial,
                       &EventLayerTest::connectFunctorNonCopyableNonTrivialScoped,
                       &EventLayerTest::connectMemberFunction,
                       &EventLayerTest::connectMemberFunctionScoped},
        Containers::arraySize(ConnectData));

    addTests({&EventLayerTest::tapOrClick,
              &EventLayerTest::middleClick,
              &EventLayerTest::rightClick,
              &EventLayerTest::drag,

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
    CORRADE_COMPARE(layer.usedNonTrivialConnectionCount(), 0);
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

Int freeFunctionCalledCount = 0;

void freeFunction() {
    ++freeFunctionCalledCount;
}

void EventLayerTest::callFreeFunction() {
    freeFunctionCalledCount = 0;

    EventLayer layer{layerHandle(0, 1)};
    DataHandle handle = layer.onTapOrClick(NodeHandle::Null, freeFunction);
    CORRADE_COMPARE(handle, dataHandle(layer.handle(), 0, 1));
    CORRADE_COMPARE(layer.usedCount(), 1);
    CORRADE_COMPARE(layer.usedScopedConnectionCount(), 0);
    CORRADE_COMPARE(layer.usedNonTrivialConnectionCount(), 0);

    PointerEvent event{Pointer::MouseLeft};
    layer.pointerTapOrClickEvent(0, event);
    CORRADE_COMPARE(freeFunctionCalledCount, 1);
}

void EventLayerTest::callFunctor() {
    Int functorCalledCount = 0;
    auto functor = [&functorCalledCount]{
        ++functorCalledCount;
    };
    CORRADE_VERIFY(std::is_trivially_destructible<decltype(functor)>::value);

    EventLayer layer{layerHandle(0, 1)};
    DataHandle handle = layer.onTapOrClick(NodeHandle::Null, functor);
    CORRADE_COMPARE(handle, dataHandle(layer.handle(), 0, 1));
    CORRADE_COMPARE(layer.usedCount(), 1);
    CORRADE_COMPARE(layer.usedScopedConnectionCount(), 0);
    CORRADE_COMPARE(layer.usedNonTrivialConnectionCount(), 0);

    PointerEvent event{Pointer::MouseLeft};
    layer.pointerTapOrClickEvent(0, event);
    CORRADE_COMPARE(functorCalledCount, 1);
}

void EventLayerTest::callLargeFunctor() {
    Int functorCalledCount = 0;
    /* MSVC 2015 dies with "array initialization requires a brace-enclosed
       initializer list" when attempting to initialize the array, wrapping it
       in a struct instead */
    #ifndef CORRADE_MSVC2015_COMPATIBILITY
    Int* functorCalledCountArray[]{
        nullptr, nullptr, nullptr, &functorCalledCount
            /* On 32bit Windows the storage is 16 bytes due to member function
               pointers with virtual bases taking 12 bytes, so we need one
               extra member to get over the limit. */
            #if defined(CORRADE_TARGET_WINDOWS) && !defined(CORRADE_TARGET_MINGW) && defined(CORRADE_TARGET_32BIT)
            , nullptr
            #endif
    };
    auto functor = [functorCalledCountArray]{
        ++*functorCalledCountArray[3];
    };
    #else
    struct FunctorCalledCountArray {
        Int* data[5];
    } functorCalledCountArray{{nullptr, nullptr, nullptr, &functorCalledCount
        /* On 32bit Windows the storage is 16 bytes due to member function
           pointers with virtual bases taking 12 bytes, so we need one extra
           member to get over the limit. */
        #if defined(CORRADE_TARGET_WINDOWS) && !defined(CORRADE_TARGET_MINGW) && defined(CORRADE_TARGET_32BIT)
        , nullptr
        #endif
    }};
    auto functor = [functorCalledCountArray]{
        ++*functorCalledCountArray.data[3];
    };
    #endif
    CORRADE_COMPARE_AS(sizeof(functor), sizeof(Implementation::EventConnectionData::Storage),
        TestSuite::Compare::Greater);

    EventLayer layer{layerHandle(0, 1)};
    DataHandle handle = layer.onTapOrClick(NodeHandle::Null, functor);
    CORRADE_COMPARE(handle, dataHandle(layer.handle(), 0, 1));
    CORRADE_COMPARE(layer.usedCount(), 1);
    CORRADE_COMPARE(layer.usedScopedConnectionCount(), 0);
    CORRADE_COMPARE(layer.usedNonTrivialConnectionCount(), 1);

    PointerEvent event{Pointer::MouseLeft};
    layer.pointerTapOrClickEvent(0, event);
    CORRADE_COMPARE(functorCalledCount, 1);
}

void EventLayerTest::callNonTrivialFunctor() {
    Int functorCalledConstructedDestructedCount = 0;
    struct NonTrivial {
        explicit NonTrivial(int& output): calledConstructedDestructedCount{&output} {
            *calledConstructedDestructedCount += 100;
        }
        explicit NonTrivial(const NonTrivial& other) noexcept: calledConstructedDestructedCount{other.calledConstructedDestructedCount} {
            *calledConstructedDestructedCount += 1000;
        }
        /* Move assignment shouldn't happen, so Clang rightfully complains that
           this function is unused. On the other hand not having it would mean
           we can't test that it doesn't get called. */
        explicit CORRADE_UNUSED NonTrivial(NonTrivial&& other) noexcept: calledConstructedDestructedCount{other.calledConstructedDestructedCount} {
            /* A (non-const) reference is passed, so a copy should be made.
               Never a move. */
            CORRADE_FAIL("A move shouldn't happen");
        }
        ~NonTrivial() {
            *calledConstructedDestructedCount += 10;
        }
        NonTrivial& operator=(const NonTrivial& other) noexcept {
            calledConstructedDestructedCount = other.calledConstructedDestructedCount;
            *calledConstructedDestructedCount += 10000;
            return *this;
        }
        /* Move assignment shouldn't happen, so Clang rightfully complains that
           this function is unused. On the other hand not having it would mean
           we can't test that it doesn't get called. */
        CORRADE_UNUSED NonTrivial& operator=(NonTrivial&& other) {
            calledConstructedDestructedCount = other.calledConstructedDestructedCount;
            CORRADE_FAIL("A move shouldn't happen");
            return *this;
        }
        void operator()() {
            *calledConstructedDestructedCount += 1;
        }

        Int* calledConstructedDestructedCount;
    };
    CORRADE_VERIFY(std::is_copy_constructible<NonTrivial>::value);
    CORRADE_VERIFY(std::is_copy_assignable<NonTrivial>::value);
    CORRADE_VERIFY(!std::is_trivially_destructible<NonTrivial>::value);

    {
        /* This is not const in order to test that a move isn't used by
           accident */
        NonTrivial nonTrivial{functorCalledConstructedDestructedCount};

        EventLayer layer{layerHandle(0, 1)};
        DataHandle handle = layer.onTapOrClick(NodeHandle::Null,  nonTrivial);
        /* Constructed a local instance (100) and copy-constructed it to the
           layer (1000) */
        CORRADE_COMPARE(functorCalledConstructedDestructedCount, 1100);
        CORRADE_COMPARE(handle, dataHandle(layer.handle(), 0, 1));
        CORRADE_COMPARE(layer.usedScopedConnectionCount(), 0);
        CORRADE_COMPARE(layer.usedNonTrivialConnectionCount(), 1);

        PointerEvent event{Pointer::MouseLeft};
        layer.pointerTapOrClickEvent(0, event);
        /* Called it (1) */
        CORRADE_COMPARE(functorCalledConstructedDestructedCount, 1101);
    }

    /* Destructed the original instance and the copy in the layer (20) */
    CORRADE_COMPARE(functorCalledConstructedDestructedCount, 1121);
}

void EventLayerTest::callNonCopyableFunctor() {
    Int functorCalledDestructedCount = 0;
    struct NonCopyable {
        explicit NonCopyable(int& output): calledConstructedDestructedCount{&output} {
            *calledConstructedDestructedCount += 100;
        }
        NonCopyable(const NonCopyable&) = delete;
        NonCopyable(NonCopyable&& other): calledConstructedDestructedCount{other.calledConstructedDestructedCount} {
            *calledConstructedDestructedCount += 1000;
        }
        ~NonCopyable() {
            *calledConstructedDestructedCount += 10;
        }
        NonCopyable& operator=(const NonCopyable&) = delete;
        /* This one doesn't end up being called but if it would it's not that
           bad */
        CORRADE_UNUSED NonCopyable& operator=(NonCopyable&& other) {
            calledConstructedDestructedCount = other.calledConstructedDestructedCount;
            *calledConstructedDestructedCount += 10000;
            return *this;
        }
        void operator()() {
            *calledConstructedDestructedCount += 1;
        }

        Int* calledConstructedDestructedCount;
    };
    CORRADE_VERIFY(!std::is_copy_constructible<NonCopyable>::value);
    CORRADE_VERIFY(!std::is_copy_assignable<NonCopyable>::value);

    {
        EventLayer layer{layerHandle(0, 1)};
        DataHandle handle = layer.onTapOrClick(NodeHandle::Null, NonCopyable{functorCalledDestructedCount});
        /* Constructed a temporary instance (100), move-constructed it to the
           layer (1000) and destructed the temporary (10) */
        CORRADE_COMPARE(functorCalledDestructedCount, 1110);
        CORRADE_COMPARE(handle, dataHandle(layer.handle(), 0, 1));
        CORRADE_COMPARE(layer.usedCount(), 1);
        CORRADE_COMPARE(layer.usedScopedConnectionCount(), 0);
        CORRADE_COMPARE(layer.usedNonTrivialConnectionCount(), 1);

        PointerEvent event{Pointer::MouseLeft};
        layer.pointerTapOrClickEvent(0, event);
        /* Called it (1) */
        CORRADE_COMPARE(functorCalledDestructedCount, 1111);
    }

    /* Destructed the moved instance in the layer (10) */
    CORRADE_COMPARE(functorCalledDestructedCount, 1121);
}

void EventLayerTest::callMemberFunction() {
    struct Thingy {
        Int calledCount = 0;

        void slot() {
            ++calledCount;
        }
    } thingy;

    EventLayer layer{layerHandle(0, 1)};
    DataHandle handle = layer.onTapOrClick(NodeHandle::Null, thingy, &Thingy::slot);
    CORRADE_COMPARE(handle, dataHandle(layer.handle(), 0, 1));
    CORRADE_COMPARE(layer.usedCount(), 1);
    CORRADE_COMPARE(layer.usedScopedConnectionCount(), 0);
    CORRADE_COMPARE(layer.usedNonTrivialConnectionCount(), 0);

    PointerEvent event{Pointer::MouseLeft};
    layer.pointerTapOrClickEvent(0, event);
    CORRADE_COMPARE(thingy.calledCount, 1);

    /* Member function pointers usually have a size of 2 pointers, except for
       non-MinGW Windows where they're just one pointer */
    #if !defined(CORRADE_TARGET_WINDOWS) || defined(CORRADE_TARGET_MINGW)
    CORRADE_COMPARE(sizeof(&Thingy::slot), 2*sizeof(std::size_t));
    #else
    CORRADE_COMPARE(sizeof(&Thingy::slot), sizeof(std::size_t));
    #endif
}

void EventLayerTest::callMemberFunctionInBase() {
    /* Like connectMemberFunction(), except that the actual instance is a
       derived class */

    struct Thingy {
        Int calledCount = 0;

        void slot() {
            ++calledCount;
        }
    };

    struct Derived: Thingy {
        Int a = 1;
    } derived;

    EventLayer layer{layerHandle(0, 1)};
    DataHandle handle = layer.onTapOrClick(NodeHandle::Null, derived, &Thingy::slot);
    CORRADE_COMPARE(handle, dataHandle(layer.handle(), 0, 1));
    CORRADE_COMPARE(layer.usedCount(), 1);
    CORRADE_COMPARE(layer.usedScopedConnectionCount(), 0);
    CORRADE_COMPARE(layer.usedNonTrivialConnectionCount(), 0);

    PointerEvent event{Pointer::MouseLeft};
    layer.pointerTapOrClickEvent(0, event);
    CORRADE_COMPARE(derived.calledCount, 1);

    /* Member function pointers usually have a size of 2 pointers, except for
       non-MinGW Windows where they're just one pointer */
    #if !defined(CORRADE_TARGET_WINDOWS) || defined(CORRADE_TARGET_MINGW)
    CORRADE_COMPARE(sizeof(&Thingy::slot), 2*sizeof(std::size_t));
    #else
    CORRADE_COMPARE(sizeof(&Thingy::slot), sizeof(std::size_t));
    #endif
}

void EventLayerTest::callMemberFunctionMultipleInheritance() {
    struct First {
        Int a = 1;
    };

    struct Second {
        Int calledCount = 0;
    };

    struct Derived: First, Second {
        void slot() {
            ++calledCount;
        }
    } derived;

    EventLayer layer{layerHandle(0, 1)};
    DataHandle handle = layer.onTapOrClick(NodeHandle::Null, derived, &Derived::slot);
    CORRADE_COMPARE(handle, dataHandle(layer.handle(), 0, 1));
    CORRADE_COMPARE(layer.usedCount(), 1);
    CORRADE_COMPARE(layer.usedScopedConnectionCount(), 0);
    CORRADE_COMPARE(layer.usedNonTrivialConnectionCount(), 0);

    PointerEvent event{Pointer::MouseLeft};
    layer.pointerTapOrClickEvent(0, event);
    CORRADE_COMPARE(derived.calledCount, 1);

    /* Member function pointers have a size of 2 pointers */
    CORRADE_COMPARE(sizeof(&Derived::slot), 2*sizeof(std::size_t));
}

void EventLayerTest::callMemberFunctionMultipleVirtualInheritance() {
    /* Same as connectMemberFunctionMultipleInheritance(), just that the
       base is inherited virtually */

    struct First {
        Int a = 1;
    };

    struct Second {
        Int calledCount = 0;
    };

    struct Derived: First, virtual Second {
        void slot() {
            ++calledCount;
        }
    } derived;

    EventLayer layer{layerHandle(0, 1)};
    DataHandle handle = layer.onTapOrClick(NodeHandle::Null, derived, &Derived::slot);
    CORRADE_COMPARE(handle, dataHandle(layer.handle(), 0, 1));
    CORRADE_COMPARE(layer.usedCount(), 1);
    CORRADE_COMPARE(layer.usedScopedConnectionCount(), 0);
    CORRADE_COMPARE(layer.usedNonTrivialConnectionCount(), 0);

    PointerEvent event{Pointer::MouseLeft};
    layer.pointerTapOrClickEvent(0, event);
    CORRADE_COMPARE(derived.calledCount, 1);

    /* Member function pointers have a size of 2 pointers, except for virtual
       inheritance on Windows where it is 12 / 16 bytes on both 32 / 64 bits.
       This is also the maximum to which the internal constant is scaled. */
    #ifndef CORRADE_TARGET_WINDOWS
    CORRADE_COMPARE(sizeof(&Derived::slot), 2*sizeof(std::size_t));
    #elif defined(CORRADE_TARGET_32BIT)
    CORRADE_COMPARE(sizeof(&Derived::slot), 12);
    #else
    CORRADE_COMPARE(sizeof(&Derived::slot), 16);
    #endif
    CORRADE_COMPARE(sizeof(&Derived::slot), Implementation::EventFunctionPointerSize*sizeof(std::size_t));
}

void EventLayerTest::connectFreeFunction() {
    auto&& data = ConnectData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    freeFunctionOutput = 1;

    EventLayer layer{layerHandle(0x96, 0xef)};

    /* Some initial data to have non-trivial IDs */
    layer.onTapOrClick(nodeHandle(0, 1), []{});
    layer.onTapOrClick(nodeHandle(2, 3), []{});
    layer.onTapOrClick(nodeHandle(4, 5), []{});

    NodeHandle node = nodeHandle(137, 0xded);

    DataHandle handle = data.freeFunction(layer, node);
    CORRADE_COMPARE(handle, dataHandle(layer.handle(), 3, 1));
    CORRADE_COMPARE(layer.node(handle), node);

    CORRADE_COMPARE(layer.usedCount(), 4);
    CORRADE_COMPARE(layer.usedScopedConnectionCount(), 0);
    CORRADE_COMPARE(layer.usedNonTrivialConnectionCount(), 0);

    data.call(layer, 3);
    CORRADE_COMPARE(freeFunctionOutput, 2);
}

void EventLayerTest::connectFreeFunctionScoped() {
    auto&& data = ConnectData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    freeFunctionOutput = 1;

    EventLayer layer{layerHandle(0x96, 0xef)};

    /* Some initial data to have non-trivial IDs */
    layer.onTapOrClick(nodeHandle(0, 1), []{});
    layer.onTapOrClick(nodeHandle(2, 3), []{});
    layer.onTapOrClick(nodeHandle(4, 5), []{});

    NodeHandle node = nodeHandle(137, 0xded);

    {
        EventConnection connection = data.freeFunctionScoped(layer, node);
        CORRADE_COMPARE(&connection.layer(), &layer);
        CORRADE_COMPARE(connection.data(), dataHandle(layer.handle(), 3, 1));
        CORRADE_COMPARE(layer.node(connection.data()), node);

        CORRADE_COMPARE(layer.usedCount(), 4);
        CORRADE_COMPARE(layer.usedScopedConnectionCount(), 1);
        CORRADE_COMPARE(layer.usedNonTrivialConnectionCount(), 0);

        data.call(layer, 3);
        CORRADE_COMPARE(freeFunctionOutput, 2);
    }

    CORRADE_COMPARE(layer.usedCount(), 3);
    CORRADE_COMPARE(layer.usedScopedConnectionCount(), 0);
    CORRADE_COMPARE(layer.usedNonTrivialConnectionCount(), 0);
}

void EventLayerTest::connectFunctor() {
    auto&& data = ConnectData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Verify basic constraints. Not doing that in static_assert()s as it's not
       a critical failure if some compiler disagrees here. */
    CORRADE_VERIFY(std::is_copy_constructible<ConnectFunctor<>>::value);
    CORRADE_VERIFY(std::is_copy_assignable<ConnectFunctor<>>::value);
    CORRADE_VERIFY(std::is_trivially_destructible<ConnectFunctor<>>::value);

    Int functorOutput = 1;

    EventLayer layer{layerHandle(0x96, 0xef)};

    /* Some initial data to have non-trivial IDs */
    layer.onTapOrClick(nodeHandle(0, 1), []{});
    layer.onTapOrClick(nodeHandle(2, 3), []{});
    layer.onTapOrClick(nodeHandle(4, 5), []{});

    NodeHandle node = nodeHandle(137, 0xded);

    DataHandle handle = data.functor(layer, node, functorOutput);
    CORRADE_COMPARE(handle, dataHandle(layer.handle(), 3, 1));
    CORRADE_COMPARE(layer.node(handle), node);

    CORRADE_COMPARE(layer.usedCount(), 4);
    CORRADE_COMPARE(layer.usedScopedConnectionCount(), 0);
    CORRADE_COMPARE(layer.usedNonTrivialConnectionCount(), 0);

    data.call(layer, 3);
    CORRADE_COMPARE(functorOutput, 3);
}

void EventLayerTest::connectFunctorScoped() {
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
        EventConnection connection = data.functorScoped(layer, node, functorOutput);
        CORRADE_COMPARE(&connection.layer(), &layer);
        CORRADE_COMPARE(connection.data(), dataHandle(layer.handle(), 3, 1));
        CORRADE_COMPARE(layer.node(connection.data()), node);

        CORRADE_COMPARE(layer.usedCount(), 4);
        CORRADE_COMPARE(layer.usedScopedConnectionCount(), 1);
        CORRADE_COMPARE(layer.usedNonTrivialConnectionCount(), 0);

        data.call(layer, 3);
        CORRADE_COMPARE(functorOutput, 3);
    }

    CORRADE_COMPARE(layer.usedCount(), 3);
    CORRADE_COMPARE(layer.usedScopedConnectionCount(), 0);
    CORRADE_COMPARE(layer.usedNonTrivialConnectionCount(), 0);
}

void EventLayerTest::connectFunctorNonTrivial() {
    auto&& data = ConnectData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Verify basic constraints. Not doing that in static_assert()s as it's not
       a critical failure if some compiler disagrees here. */
    CORRADE_VERIFY(std::is_copy_constructible<ConnectFunctorNonTrivial<>>::value);
    CORRADE_VERIFY(std::is_copy_assignable<ConnectFunctorNonTrivial<>>::value);
    CORRADE_VERIFY(!std::is_trivially_destructible<ConnectFunctorNonTrivial<>>::value);


    Int functorOutput = 1;

    {
        EventLayer layer{layerHandle(0x96, 0xef)};

        /* Some initial data to have non-trivial IDs */
        layer.onTapOrClick(nodeHandle(0, 1), []{});
        layer.onTapOrClick(nodeHandle(2, 3), []{});
        layer.onTapOrClick(nodeHandle(4, 5), []{});

        NodeHandle node = nodeHandle(137, 0xded);

        /* A functor temporary gets constructed inside, copied and destructed */
        DataHandle handle = data.functorNonTrivial(layer, node, functorOutput);
        CORRADE_COMPARE(functorOutput, 5);
        CORRADE_COMPARE(handle, dataHandle(layer.handle(), 3, 1));
        CORRADE_COMPARE(layer.node(handle), node);

        CORRADE_COMPARE(layer.usedCount(), 4);
        CORRADE_COMPARE(layer.usedScopedConnectionCount(), 0);
        CORRADE_COMPARE(layer.usedNonTrivialConnectionCount(), 1);

        /* THe functor gets called */
        data.call(layer, 3);
        CORRADE_COMPARE(functorOutput, 7*5);
    }

    /* The functor copy gets destructed after */
    CORRADE_COMPARE(functorOutput, 7*5*5);
}

void EventLayerTest::connectFunctorNonTrivialScoped() {
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
        EventConnection connection = data.functorNonTrivialScoped(layer, node, functorOutput);
        CORRADE_COMPARE(functorOutput, 5);
        CORRADE_COMPARE(&connection.layer(), &layer);
        CORRADE_COMPARE(connection.data(), dataHandle(layer.handle(), 3, 1));
        CORRADE_COMPARE(layer.node(connection.data()), node);

        CORRADE_COMPARE(layer.usedCount(), 4);
        CORRADE_COMPARE(layer.usedScopedConnectionCount(), 1);
        CORRADE_COMPARE(layer.usedNonTrivialConnectionCount(), 1);

        /* THe functor gets called */
        data.call(layer, 3);
        CORRADE_COMPARE(functorOutput, 7*5);
    }

    CORRADE_COMPARE(layer.usedCount(), 3);
    CORRADE_COMPARE(layer.usedScopedConnectionCount(), 0);
    CORRADE_COMPARE(layer.usedNonTrivialConnectionCount(), 0);

    /* The functor copy gets destructed after */
    CORRADE_COMPARE(functorOutput, 7*5*5);
}

void EventLayerTest::connectFunctorNonCopyable() {
    auto&& data = ConnectData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Verify basic constraints. Not doing that in static_assert()s as it's not
       a critical failure if some compiler disagrees here. */
    CORRADE_VERIFY(!std::is_copy_constructible<ConnectFunctorNonCopyable<>>::value);
    CORRADE_VERIFY(std::is_move_constructible<ConnectFunctorNonCopyable<>>::value);
    CORRADE_VERIFY(!std::is_copy_assignable<ConnectFunctorNonCopyable<>>::value);
    CORRADE_VERIFY(std::is_move_assignable<ConnectFunctorNonCopyable<>>::value);
    CORRADE_VERIFY(std::is_trivially_destructible<ConnectFunctorNonCopyable<>>::value);

    Int functorOutput = 1;

    EventLayer layer{layerHandle(0x96, 0xef)};

    /* Some initial data to have non-trivial IDs */
    layer.onTapOrClick(nodeHandle(0, 1), []{});
    layer.onTapOrClick(nodeHandle(2, 3), []{});
    layer.onTapOrClick(nodeHandle(4, 5), []{});

    NodeHandle node = nodeHandle(137, 0xded);

    DataHandle handle = data.functorNonCopyable(layer, node, functorOutput);
    CORRADE_COMPARE(handle, dataHandle(layer.handle(), 3, 1));
    CORRADE_COMPARE(layer.node(handle), node);

    CORRADE_COMPARE(layer.usedCount(), 4);
    CORRADE_COMPARE(layer.usedScopedConnectionCount(), 0);
    CORRADE_COMPARE(layer.usedNonTrivialConnectionCount(), 0);

    data.call(layer, 3);
    CORRADE_COMPARE(functorOutput, 11);
}

void EventLayerTest::connectFunctorNonCopyableScoped() {
    auto&& data = ConnectData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    Int functorOutput = 1;

    EventLayer layer{layerHandle(0x96, 0xef)};

    /* Some initial data to have non-trivial IDs */
    layer.onTapOrClick(nodeHandle(0, 1), []{});
    layer.onTapOrClick(nodeHandle(2, 3), []{});
    layer.onTapOrClick(nodeHandle(4, 5), []{});

    NodeHandle node = nodeHandle(137, 0xded);

    {
        EventConnection connection = data.functorNonCopyableScoped(layer, node, functorOutput);
        CORRADE_COMPARE(&connection.layer(), &layer);
        CORRADE_COMPARE(connection.data(), dataHandle(layer.handle(), 3, 1));
        CORRADE_COMPARE(layer.node(connection.data()), node);

        CORRADE_COMPARE(layer.usedCount(), 4);
        CORRADE_COMPARE(layer.usedScopedConnectionCount(), 1);
        CORRADE_COMPARE(layer.usedNonTrivialConnectionCount(), 0);

        data.call(layer, 3);
        CORRADE_COMPARE(functorOutput, 11);
    }

    CORRADE_COMPARE(layer.usedCount(), 3);
    CORRADE_COMPARE(layer.usedScopedConnectionCount(), 0);
    CORRADE_COMPARE(layer.usedNonTrivialConnectionCount(), 0);
}

void EventLayerTest::connectFunctorNonCopyableNonTrivial() {
    auto&& data = ConnectData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Verify basic constraints. Not doing that in static_assert()s as it's not
      a critical failure if some compiler disagrees here. */
    CORRADE_VERIFY(!std::is_copy_constructible<ConnectFunctorNonCopyableNonTrivial<>>::value);
    CORRADE_VERIFY(std::is_move_constructible<ConnectFunctorNonCopyableNonTrivial<>>::value);
    CORRADE_VERIFY(!std::is_copy_assignable<ConnectFunctorNonCopyableNonTrivial<>>::value);
    CORRADE_VERIFY(std::is_move_assignable<ConnectFunctorNonCopyableNonTrivial<>>::value);
    CORRADE_VERIFY(!std::is_trivially_destructible<ConnectFunctorNonCopyableNonTrivial<>>::value);

    Int functorOutput = 1;

    {
        EventLayer layer{layerHandle(0x96, 0xef)};

        /* Some initial data to have non-trivial IDs */
        layer.onTapOrClick(nodeHandle(0, 1), []{});
        layer.onTapOrClick(nodeHandle(2, 3), []{});
        layer.onTapOrClick(nodeHandle(4, 5), []{});

        NodeHandle node = nodeHandle(137, 0xded);

        /* A functor temporary gets constructed inside, copied and destructed */
        DataHandle handle = data.functorNonCopyableNonTrivial(layer, node, functorOutput);
        CORRADE_COMPARE(functorOutput, 13);
        CORRADE_COMPARE(handle, dataHandle(layer.handle(), 3, 1));
        CORRADE_COMPARE(layer.node(handle), node);

        CORRADE_COMPARE(layer.usedCount(), 4);
        CORRADE_COMPARE(layer.usedScopedConnectionCount(), 0);
        CORRADE_COMPARE(layer.usedNonTrivialConnectionCount(), 1);

        /* THe functor gets called */
        data.call(layer, 3);
        CORRADE_COMPARE(functorOutput, 17*13);
    }

    /* The functor moved copy gets destructed after */
    CORRADE_COMPARE(functorOutput, 17*13*13);
}

void EventLayerTest::connectFunctorNonCopyableNonTrivialScoped() {
    auto&& data = ConnectData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    Int functorOutput = 1;

    EventLayer layer{layerHandle(0x96, 0xef)};

    /* Some initial data to have non-trivial IDs */
    layer.onTapOrClick(nodeHandle(0, 1), []{});
    layer.onTapOrClick(nodeHandle(2, 3), []{});
    layer.onTapOrClick(nodeHandle(4, 5), []{});

    NodeHandle node = nodeHandle(137, 0xded);

    {
        /* A functor temporary gets constructed inside, copied and destructed */
        EventConnection connection = data.functorNonCopyableNonTrivialScoped(layer, node, functorOutput);
        CORRADE_COMPARE(functorOutput, 13);
        CORRADE_COMPARE(&connection.layer(), &layer);
        CORRADE_COMPARE(connection.data(), dataHandle(layer.handle(), 3, 1));
        CORRADE_COMPARE(layer.node(connection.data()), node);

        CORRADE_COMPARE(layer.usedCount(), 4);
        CORRADE_COMPARE(layer.usedScopedConnectionCount(), 1);
        CORRADE_COMPARE(layer.usedNonTrivialConnectionCount(), 1);

        /* THe functor gets called */
        data.call(layer, 3);
        CORRADE_COMPARE(functorOutput, 17*13);
    }

    CORRADE_COMPARE(layer.usedCount(), 3);
    CORRADE_COMPARE(layer.usedScopedConnectionCount(), 0);
    CORRADE_COMPARE(layer.usedNonTrivialConnectionCount(), 0);

    /* The functor moved copy gets destructed after */
    CORRADE_COMPARE(functorOutput, 17*13*13);
}

void EventLayerTest::connectMemberFunction() {
    auto&& data = ConnectData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    ConnectMember instance;

    EventLayer layer{layerHandle(0x96, 0xef)};

    /* Some initial data to have non-trivial IDs */
    layer.onTapOrClick(nodeHandle(0, 1), []{});
    layer.onTapOrClick(nodeHandle(2, 3), []{});
    layer.onTapOrClick(nodeHandle(4, 5), []{});

    NodeHandle node = nodeHandle(137, 0xded);

    DataHandle handle = data.memberFunction(layer, node, instance);
    CORRADE_COMPARE(handle, dataHandle(layer.handle(), 3, 1));
    CORRADE_COMPARE(layer.node(handle), node);

    CORRADE_COMPARE(layer.usedCount(), 4);
    CORRADE_COMPARE(layer.usedScopedConnectionCount(), 0);
    CORRADE_COMPARE(layer.usedNonTrivialConnectionCount(), 0);

    data.call(layer, 3);
    CORRADE_COMPARE(instance.output, 19);
}

void EventLayerTest::connectMemberFunctionScoped() {
    auto&& data = ConnectData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    ConnectMember instance;

    EventLayer layer{layerHandle(0x96, 0xef)};

    /* Some initial data to have non-trivial IDs */
    layer.onTapOrClick(nodeHandle(0, 1), []{});
    layer.onTapOrClick(nodeHandle(2, 3), []{});
    layer.onTapOrClick(nodeHandle(4, 5), []{});

    NodeHandle node = nodeHandle(137, 0xded);

    {
        EventConnection connection = data.memberFunctionScoped(layer, node, instance);
        CORRADE_COMPARE(&connection.layer(), &layer);
        CORRADE_COMPARE(connection.data(), dataHandle(layer.handle(), 3, 1));
        CORRADE_COMPARE(layer.node(connection.data()), node);

        CORRADE_COMPARE(layer.usedCount(), 4);
        CORRADE_COMPARE(layer.usedScopedConnectionCount(), 1);
        CORRADE_COMPARE(layer.usedNonTrivialConnectionCount(), 0);

        data.call(layer, 3);
        CORRADE_COMPARE(instance.output, 19);
    }

    CORRADE_COMPARE(layer.usedCount(), 3);
    CORRADE_COMPARE(layer.usedScopedConnectionCount(), 0);
    CORRADE_COMPARE(layer.usedNonTrivialConnectionCount(), 0);
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
    CORRADE_COMPARE(layer.usedNonTrivialConnectionCount(), 0);

    /* The temporary gets destructed right away */
    DataHandle nonTrivial = layer.onTapOrClick(nodeHandle(1, 2), NonTrivial{destructedCount});
    CORRADE_COMPARE(layer.usedCount(), 2);
    CORRADE_COMPARE(layer.usedScopedConnectionCount(), 0);
    CORRADE_COMPARE(layer.usedNonTrivialConnectionCount(), 1);
    CORRADE_COMPARE(destructedCount, 1);

    layer.remove(trivial);
    CORRADE_COMPARE(layer.usedCount(), 1);
    CORRADE_COMPARE(layer.usedScopedConnectionCount(), 0);
    CORRADE_COMPARE(layer.usedNonTrivialConnectionCount(), 1);
    CORRADE_COMPARE(destructedCount, 1);

    /* Verifying also the other handle overload. They should both delegate into
       the same internal implementation. */
    layer.remove(dataHandleData(nonTrivial));
    CORRADE_COMPARE(layer.usedCount(), 0);
    CORRADE_COMPARE(layer.usedScopedConnectionCount(), 0);
    CORRADE_COMPARE(layer.usedNonTrivialConnectionCount(), 0);
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
        CORRADE_COMPARE(layer.usedNonTrivialConnectionCount(), 0);

        /* The temporary gets destructed right away */
        EventConnection nonTrivial = layer.onTapOrClickScoped(nodeHandle(1, 2), NonTrivial{destructedCount});
        CORRADE_COMPARE(layer.usedCount(), 2);
        CORRADE_COMPARE(layer.usedScopedConnectionCount(), 2);
        CORRADE_COMPARE(layer.usedNonTrivialConnectionCount(), 1);
        CORRADE_COMPARE(destructedCount, 1);

        layer.remove(trivial.data());
        CORRADE_COMPARE(layer.usedCount(), 1);
        CORRADE_COMPARE(layer.usedScopedConnectionCount(), 1);
        CORRADE_COMPARE(layer.usedNonTrivialConnectionCount(), 1);
        CORRADE_COMPARE(destructedCount, 1);

        layer.remove(nonTrivial.data());
        CORRADE_COMPARE(layer.usedCount(), 0);
        CORRADE_COMPARE(layer.usedScopedConnectionCount(), 0);
        CORRADE_COMPARE(layer.usedNonTrivialConnectionCount(), 0);
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
    CORRADE_COMPARE(layer.usedNonTrivialConnectionCount(), 0);

    /* The temporary gets destructed right away */
    DataHandle nonTrivial = layer.onTapOrClick(nodeHandle(3, 4), NonTrivial{destructedCount});
    CORRADE_COMPARE(layer.usedCount(), 2);
    CORRADE_COMPARE(layer.usedNonTrivialConnectionCount(), 1);
    CORRADE_COMPARE(destructedCount, 1);

    DataHandle another = layer.onTapOrClick(nodeHandle(0, 5), []{});
    CORRADE_COMPARE(layer.usedCount(), 3);
    CORRADE_COMPARE(layer.usedNonTrivialConnectionCount(), 1);

    /* The temporary gets destructed right away */
    DataHandle anotherNonTrivial = layer.onTapOrClick(nodeHandle(4, 1), NonTrivial{anotherDestructedCount});
    CORRADE_COMPARE(layer.usedCount(), 4);
    CORRADE_COMPARE(layer.usedNonTrivialConnectionCount(), 2);
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
    CORRADE_COMPARE(layer.usedNonTrivialConnectionCount(), 1);
    CORRADE_COMPARE(destructedCount, 2);
    CORRADE_COMPARE(anotherDestructedCount, 1);
    CORRADE_VERIFY(!layer.isHandleValid(trivial));
    CORRADE_VERIFY(!layer.isHandleValid(nonTrivial));
    CORRADE_VERIFY(layer.isHandleValid(another));
    CORRADE_VERIFY(layer.isHandleValid(anotherNonTrivial));
}

}}}}

CORRADE_TEST_MAIN(Magnum::Whee::Test::EventLayerTest)
