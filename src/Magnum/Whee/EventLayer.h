#ifndef Magnum_Whee_EventLayer_h
#define Magnum_Whee_EventLayer_h
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

/** @file
 * @brief Class @ref Magnum::Whee::EventLayer, @ref Magnum::Whee::EventConnection
 * @m_since_latest
 */

#include <new>
#include <Corrade/Containers/Reference.h>

#include "Magnum/Whee/AbstractLayer.h"

namespace Magnum { namespace Whee {

namespace Implementation {
    struct EventConnectionData;

    enum class EventType: UnsignedByte {
        Press,
        TapOrClick,
        MiddleClick,
        RightClick,
        Drag
    };
}

/**
@brief Connection in the @ref EventLayer
@m_since_latest

Performs automatic removal of a connection on destruction. Each instance with
non-null @ref data() counts towards @ref EventLayer::usedScopedConnectionCount().
*/
class MAGNUM_WHEE_EXPORT EventConnection {
    public:
        /** @brief Copying is not allowed */
        EventConnection(const EventConnection&) = delete;

        /** @brief Move constructor */
        EventConnection(EventConnection&& other) noexcept: _layer{other._layer}, _data{other._data} {
            /* Not using LayerDataHandle::Null to avoid including Handle.h;
               OTOH keeping the constructor inlined for debug perf */
            other._data = LayerDataHandle{};
        }

        /**
         * @brief Destructor
         *
         * If @ref data() is not @ref DataHandle::Null and is valid, removes
         * the connection from associated @ref layer().
         */
        ~EventConnection();

        /** @brief Copying is not allowed */
        EventConnection& operator=(const EventConnection&) = delete;

        /** @brief Move assignment */
        EventConnection& operator=(EventConnection&& other) noexcept {
            Utility::swap(other._layer, _layer);
            Utility::swap(other._data, _data);
            return *this;
        }

        /**
         * @brief Event layer containing the connection
         *
         * If @ref data() is @ref DataHandle::Null, the reference may be
         * dangling.
         */
        EventLayer& layer() { return _layer; }
        const EventLayer& layer() const { return _layer; } /**< @overload */

        /**
         * @brief Connection data handle
         *
         * If @ref DataHandle::Null, the instance is moved out or released.
         */
        DataHandle data() const;

        /**
         * @brief Release the connection
         *
         * Returns @ref data() and sets it to @ref DataHandle::Null so it's no
         * longer removed at destruction. Removing the connection using
         * @ref EventLayer::remove() before the layer itself is destructed is
         * user responsibility.
         */
        DataHandle release();

    private:
        friend EventLayer;

        /* LayerDataHandle is the lower 32 bits of DataHandle. Not using
           dataHandleData() to avoid dependency on Handle.h, additionally
           checked with a static_assert() in the source file. */
        explicit EventConnection(EventLayer& layer, DataHandle data) noexcept;

        Containers::Reference<EventLayer> _layer;
        LayerDataHandle _data;
};

namespace Implementation {

/* Lower bound of the size is verified with a static_assert() in templated
   EventLayer::create() implementations below, upper bound in particular  EventLayerTest::call*() cases. */
enum: std::size_t { EventFunctionPointerSize =
    #if !defined(CORRADE_TARGET_WINDOWS) || defined(CORRADE_TARGET_MINGW)
    2*sizeof(void*)/sizeof(std::size_t)
    #else
    /* On MSVC, pointers to members with a virtual base class are the biggest
       and have 12 bytes on 32bit and 16 on 64bit. Pointers to incomplete class
       members are 16 bytes on both 32bit and 64bit, but such scenario is
       impossible to happen here as there's a std::is_base_of check in
       EventLayer::create() that requires the type to be complete. */
    #ifdef CORRADE_TARGET_32BIT
    12/sizeof(std::size_t)
    #else
    16/sizeof(std::size_t)
    #endif
    #endif
};

struct EventConnectionData {
    union alignas(sizeof(std::size_t)) Storage {
        char data[EventFunctionPointerSize*sizeof(std::size_t) + sizeof(void*)];
        void(*function)();
        struct {
            char data[EventFunctionPointerSize*sizeof(std::size_t)];
            void* receiver;
        } member;
        struct {
            void* data;
            void(*destruct)(Storage&);
        } functor;
    } storage;
    void(*call)();
};

}

/**
@brief Event handling layer
@m_since_latest

Provides signal/slot-like functionality, connecting events happening on nodes
with aribtrary functions handling them.
@see @ref UserInterface::eventLayer(),
    @ref UserInterface::setEventLayerInstance(), @ref StyleFeature::EventLayer
*/
class MAGNUM_WHEE_EXPORT EventLayer: public AbstractLayer {
    public:
        /**
         * @brief Constructor
         * @param handle    Layer handle returned from
         *      @ref AbstractUserInterface::createLayer()
         */
        explicit EventLayer(LayerHandle handle);

        /** @brief Copying is not allowed */
        EventLayer(const EventLayer&) = delete;

        /**
         * @brief Move constructor
         *
         * Performs a destructive move, i.e. the original object isn't usable
         * afterwards anymore.
         */
        EventLayer(EventLayer&&) noexcept;

        /**
         * @brief Destructor
         *
         * Expects that @ref usedScopedConnectionCount() is @cpp 0 @ce.
         */
        ~EventLayer() override;

        /** @brief Copying is not allowed */
        EventLayer& operator=(const EventLayer&) = delete;

        /** @brief Move assignment */
        EventLayer& operator=(EventLayer&&) noexcept;

        /**
         * @brief Count of currently active @ref EventConnection instances
         *
         * Always at most @ref usedCount(). The layer is only allowed to be
         * destroyed after all scoped connections are removed, as the
         * @ref EventConnection destructors would then access a dangling layer
         * pointer.
         */
        UnsignedInt usedScopedConnectionCount() const;

        /**
         * @brief Count of non-trivial connections
         *
         * Always at most @ref usedCount(). Counts all connections that capture
         * non-trivially-destructible state or state that's too large to be
         * stored in-place. The operation is done with a @f$ \mathcal{O}(n) @f$
         * complexity where @f$ n @f$ is @ref capacity().
         */
        UnsignedInt usedNonTrivialConnectionCount() const;

        /**
         * @brief Connect to a finger / pen tap or left mouse press
         *
         * The slot (of signature @cpp void(*)() @ce) is called when a
         * @ref Pointer::MouseLeft, @ref Pointer::Finger or @ref Pointer::Pen
         * press happens on the @p node.
         *
         * Use @ref onTapOrClick() a combined press and release. The returned
         * @ref DataHandle is automatically removed once @p node or any of its
         * parents is removed, it's the caller responsibility to ensure it
         * doesn't outlive the state captured in the @p slot. See
         * @ref onPressScoped() for a scoped alternative.
         */
        template<class Slot> DataHandle onPress(NodeHandle node, Slot&& slot) {
            return create<Slot>(node, Implementation::EventType::Press, Utility::forward<Slot>(slot));
        }
        /** @overload */
        template<class ReceiverObject, class Receiver> DataHandle onPress(NodeHandle node, ReceiverObject& receiver, void(Receiver::*slot)()) {
            return create<ReceiverObject, Receiver>(node, Implementation::EventType::Press, receiver, slot);
        }

        /**
         * @brief Scoped connection to a finger / pen tap or left mouse press
         *
         * Compared to @ref onPress() the connection is removed automatically
         * when the returned @ref EventConnection gets destroyed.
         */
        template<class Slot> EventConnection onPressScoped(NodeHandle node, Slot&& slot) {
            return createScoped<Slot>(node, Implementation::EventType::Press, Utility::forward<Slot>(slot));
        }
        /** @overload */
        template<class ReceiverObject, class Receiver> EventConnection onPressScoped(NodeHandle node, ReceiverObject& receiver, void(Receiver::*slot)()) {
            return createScoped<ReceiverObject, Receiver>(node, Implementation::EventType::Press, receiver, slot);
        }

        /**
         * @brief Connect to a finger / pen tap or left mouse click
         *
         * The slot (of signature @cpp void(*)() @ce) is called when a
         * @ref Pointer::MouseLeft, @ref Pointer::Finger or @ref Pointer::Pen
         * release happens on the @p node after a previous pointer press. If
         * event capture is disabled by any event handler on given node, the
         * slot is called only if the pointer didn't leave the node area
         * between a press and a release.
         *
         * Use @ref onRightClick() and @ref onMiddleClick() to handle
         * @ref Pointer::MouseRight and @ref Pointer::MouseMiddle clicks. The
         * returned @ref DataHandle is automatically removed once @p node or
         * any of its parents is removed, it's the caller responsibility to
         * ensure it doesn't outlive the state captured in the @p slot. See
         * @ref onTapOrClickScoped() for a scoped alternative.
         */
        template<class Slot> DataHandle onTapOrClick(NodeHandle node, Slot&& slot) {
            return create<Slot>(node, Implementation::EventType::TapOrClick, Utility::forward<Slot>(slot));
        }
        /** @overload */
        template<class ReceiverObject, class Receiver> DataHandle onTapOrClick(NodeHandle node, ReceiverObject& receiver, void(Receiver::*slot)()) {
            return create<ReceiverObject, Receiver>(node, Implementation::EventType::TapOrClick, receiver, slot);
        }

        /**
         * @brief Scoped connection to a finger / pen tap or left mouse click
         *
         * Compared to @ref onTapOrClick() the connection is removed
         * automatically when the returned @ref EventConnection gets destroyed.
         */
        template<class Slot> EventConnection onTapOrClickScoped(NodeHandle node, Slot&& slot) {
            return createScoped<Slot>(node, Implementation::EventType::TapOrClick, Utility::forward<Slot>(slot));
        }
        /** @overload */
        template<class ReceiverObject, class Receiver> EventConnection onTapOrClickScoped(NodeHandle node, ReceiverObject& receiver, void(Receiver::*slot)()) {
            return createScoped<ReceiverObject, Receiver>(node, Implementation::EventType::TapOrClick, receiver, slot);
        }

        /**
         * @brief Connect to a middle mouse click
         *
         * The slot (of signature @cpp void(*)() @ce) is called when a
         * @ref Pointer::MouseMiddle release happens on the @p node after a
         * previous pointer press. If event capture is disabled by any event
         * handler on given node, the slot is called only if the pointer didn't
         * leave the node area between a press and a release.
         *
         * Use @ref onTapOrClick() and @ref onRightClick() to handle
         * @ref Pointer::MouseLeft / @ref Pointer::Finger / @ref Pointer::Pen
         * and @ref Pointer::MouseRight clicks. The returned @ref DataHandle is
         * automatically removed once @p node or any of its parents is removed,
         * it's the caller responsibility to ensure it doesn't outlive the
         * state captured in the @p slot. See @ref onMiddleClickScoped() for a
         * scoped alternative.
         */
        template<class Slot> DataHandle onMiddleClick(NodeHandle node, Slot&& slot) {
            return create<Slot>(node, Implementation::EventType::MiddleClick, Utility::forward<Slot>(slot));
        }
        /** @overload */
        template<class ReceiverObject, class Receiver> DataHandle onMiddleClick(NodeHandle node, ReceiverObject& receiver, void(Receiver::*slot)()) {
            return create<ReceiverObject, Receiver>(node, Implementation::EventType::MiddleClick, receiver, slot);
        }

        /**
         * @brief Scoped connection to a middle mouse click
         *
         * Compared to @ref onMiddleClick() the connection is removed
         * automatically when the returned @ref EventConnection gets destroyed.
         */
        template<class Slot> EventConnection onMiddleClickScoped(NodeHandle node, Slot&& slot) {
            return createScoped<Slot>(node, Implementation::EventType::MiddleClick, Utility::forward<Slot>(slot));
        }
        /** @overload */
        template<class ReceiverObject, class Receiver> EventConnection onMiddleClickScoped(NodeHandle node, ReceiverObject& receiver, void(Receiver::*slot)()) {
            return createScoped<ReceiverObject, Receiver>(node, Implementation::EventType::MiddleClick, receiver, slot);
        }

        /**
         * @brief Connect to a right mouse click
         *
         * The slot (of signature @cpp void(*)() @ce) is called when a
         * @ref Pointer::MouseRight release happens on the @p node after a
         * previous pointer press. If event capture is disabled by any event
         * handler on given node, the slot is called only if the pointer didn't
         * leave the node area between a press and a release.
         *
         * Use @ref onTapOrClick() and @ref onRightClick() to handle
         * @ref Pointer::MouseLeft / @ref Pointer::Finger / @ref Pointer::Pen
         * and @ref Pointer::MouseRight clicks. The returned @ref DataHandle is
         * automatically removed once @p node or any of its parents is removed,
         * it's the caller responsibility to ensure it doesn't outlive the
         * state captured in the @p slot. See @ref onMiddleClickScoped() for a
         * scoped alternative.
         */
        template<class Slot> DataHandle onRightClick(NodeHandle node, Slot&& slot) {
            return create<Slot>(node, Implementation::EventType::RightClick, Utility::forward<Slot>(slot));
        }
        /** @overload */
        template<class ReceiverObject, class Receiver> DataHandle onRightClick(NodeHandle node, ReceiverObject& receiver, void(Receiver::*slot)()) {
            return create<ReceiverObject, Receiver>(node, Implementation::EventType::RightClick, receiver, slot);
        }

        /**
         * @brief Scoped connection to a right mouse click
         *
         * Compared to @ref onRightClick() the connection is removed
         * automatically when the returned @ref EventConnection gets destroyed.
         */
        template<class Slot> EventConnection onRightClickScoped(NodeHandle node, Slot&& slot) {
            return createScoped<Slot>(node, Implementation::EventType::RightClick, Utility::forward<Slot>(slot));
        }
        /** @overload */
        template<class ReceiverObject, class Receiver> EventConnection onRightClickScoped(NodeHandle node, ReceiverObject& receiver, void(Receiver::*slot)()) {
            return createScoped<ReceiverObject, Receiver>(node, Implementation::EventType::RightClick, receiver, slot);
        }

        /**
         * @brief Connect to a drag
         *
         * The slot (of signature @cpp void(*)(const Vector2&) @ce, receiving
         * the movement delta) is called when a @ref Pointer::MouseLeft,
         * @ref Pointer::Finger or @ref Pointer::Pen move happens on the
         * @p node. If event capture is disabled by any event handler on given
         * node, the slot is called only as long as the pointer doesn't leave
         * the node area.
         *
         * The returned @ref DataHandle is automatically removed once @p node
         * or any of its parents is removed, it's the caller responsibility to
         * ensure it doesn't outlive the state captured in the @p slot. See
         * @ref onDragScoped() for a scoped alternative.
         */
        template<class Slot> DataHandle onDrag(NodeHandle node, Slot&& slot) {
            return create<Slot, const Vector2&>(node, Implementation::EventType::Drag, Utility::forward<Slot>(slot));
        }
        /** @overload */
        template<class ReceiverObject, class Receiver> DataHandle onDrag(NodeHandle node, ReceiverObject& receiver, void(Receiver::*slot)(const Vector2&)) {
            return create<ReceiverObject, Receiver, const Vector2&>(node, Implementation::EventType::Drag, receiver, slot);
        }

        /**
         * @brief Scoped connection to a drag
         *
         * Compared to @ref onDrag() the connection is removed automatically
         * when the returned @ref EventConnection gets destroyed.
         */
        template<class Slot> EventConnection onDragScoped(NodeHandle node, Slot&& slot) {
            return createScoped<Slot, const Vector2&>(node, Implementation::EventType::Drag, Utility::forward<Slot>(slot));
        }
        /** @overload */
        template<class ReceiverObject, class Receiver> EventConnection onDragScoped(NodeHandle node, ReceiverObject& receiver, void(Receiver::*slot)(const Vector2&)) {
            return createScoped<ReceiverObject, Receiver, const Vector2&>(node, Implementation::EventType::Drag, receiver, slot);
        }

        /**
         * @brief Remove a connection
         *
         * Delegates to @ref AbstractLayer::remove(DataHandle) and additionally
         * calls a destructor on the captured state, if it's not trivially
         * destructible. The @p handle becomes invalid, which means that any
         * existing @ref EventConnection instance for it will not attempt to
         * remove it again.
         * @see @ref usedNonTrivialConnectionCount()
         */
        void remove(DataHandle handle);

        /**
         * @brief Remove a connection assuming it belongs to this layer
         *
         * Delegates to @ref AbstractLayer::remove(LayerDataHandle) and
         * additionally calls a destructor on the captured state, if it's not
         * trivially destructible. The @p handle becomes invalid, which means
         * that any existing @ref EventConnection instance for it will not
         * attempt to remove it again.
         * @see @ref usedNonTrivialConnectionCount()
         */
        void remove(LayerDataHandle handle);

    private:
        /* Updates usedScopedConnectionCount */
        friend EventConnection;

        /* Used internally from all templated create() overloads below */
        DataHandle create(NodeHandle node, Implementation::EventType eventType, bool hasDestructor, Implementation::EventConnectionData*& connectionData);

        /* Free function connection implementation for on*() */
        template<class Slot, class ...Args, typename std::enable_if<std::is_convertible<typename std::decay<Slot>::type, void(*)(Args...)>::value, int>::type = 0> DataHandle create(NodeHandle node, Implementation::EventType eventType, Slot slot);

        /* Simple, small enough and trivial functor connection implementation
           for on*() */
        template<class Slot, class ...Args, typename std::enable_if<!std::is_convertible<typename std::decay<Slot>::type, void(*)(Args...)>::value && sizeof(typename std::decay<Slot>::type) <= sizeof(Implementation::EventConnectionData::Storage) && std::is_trivially_destructible<typename std::decay<Slot>::type>::value, int>::type = 0> DataHandle create(NodeHandle node, Implementation::EventType eventType, Slot&& slot);

        /* Non-trivially-destructible or too large functor connection
           implementation for on*() */
        template<class Slot, class ...Args, typename std::enable_if<(sizeof(typename std::decay<Slot>::type) > sizeof(Implementation::EventConnectionData::Storage)) || !std::is_trivially_destructible<typename std::decay<Slot>::type>::value, int>::type = 0> DataHandle create(NodeHandle node, Implementation::EventType eventType, Slot&& slot);

        /* Member function connection implementation for on*() */
        template<class ReceiverObject, class Receiver, class ...Args> DataHandle create(NodeHandle node, Implementation::EventType eventType, ReceiverObject& receiver, void(Receiver::*slot)(Args...));

        /* Implementations for on*Scoped(), delegating to create() and forming
           an EventConnection from the LayerDataHandle */
        template<class Slot, class ...Args> EventConnection createScoped(NodeHandle node, Implementation::EventType eventType, Slot&& slot);
        template<class ReceiverObject, class Receiver, class ...Args> EventConnection createScoped(NodeHandle node, Implementation::EventType eventType, ReceiverObject& receiver, void(Receiver::*slot)(Args...));

        MAGNUM_WHEE_LOCAL void removeInternal(UnsignedInt id);

        MAGNUM_WHEE_LOCAL LayerFeatures doFeatures() const override;
        MAGNUM_WHEE_LOCAL void doClean(Containers::BitArrayView dataIdsToRemove) override;

        MAGNUM_WHEE_LOCAL void doPointerPressEvent(UnsignedInt dataId, PointerEvent& event) override;
        MAGNUM_WHEE_LOCAL void doPointerTapOrClickEvent(UnsignedInt dataId, PointerEvent& event) override;
        MAGNUM_WHEE_LOCAL void doPointerMoveEvent(UnsignedInt dataId, PointerMoveEvent& event) override;

        struct State;
        Containers::Pointer<State> _state;
};

inline EventConnection::~EventConnection()  {
    /* Not using LayerDataHandle::Null to avoid including Handle.h; OTOH
       keeping the destructor inlined for debug perf */
    if(_data != LayerDataHandle{} && _layer->isHandleValid(_data))
        _layer->remove(_data);
}

/* It parses various stuff from below as VARIABLES IN THE MAGNUM NAMESPACE!
   What a dumpster fire of a broken tool. */
#ifndef DOXYGEN_GENERATING_OUTPUT
/* Free function connection */
template<class Slot, class ...Args, typename std::enable_if<std::is_convertible<typename std::decay<Slot>::type, void(*)(Args...)>::value, int>::type> DataHandle EventLayer::create(NodeHandle node, Implementation::EventType eventType, Slot slot) {
    Implementation::EventConnectionData* data;
    const DataHandle handle = create(node, eventType, false, data);

    data->storage.function = reinterpret_cast<void(*)()>(static_cast<void(*)(Args...)>(slot));
    /* The + is to decay the lambda to a function pointer so we can cast it.
       MSVC 2015 says it's "illegal on a class" so there it's an explicit cast
       to the function pointer type (and the parentheses are for both to have
       only one ifdef). */
    data->call = reinterpret_cast<void(*)()>(
        #ifndef CORRADE_MSVC2015_COMPATIBILITY
        +
        #else
        static_cast<void(*)(Implementation::EventConnectionData::Storage& storage, Args&&...)>
        #endif
    ([](Implementation::EventConnectionData::Storage& storage, Args&&... args) {
        reinterpret_cast<void(*)(Args...)>(storage.function)(Utility::forward<Args>(args)...);
    }));

    return handle;
}

/* Simple, small enough and trivially destructible functor (which is not
   convertible to a function pointer) connection */
template<class Slot, class ...Args, typename std::enable_if<!std::is_convertible<typename std::decay<Slot>::type, void(*)(Args...)>::value && sizeof(typename std::decay<Slot>::type) <= sizeof(Implementation::EventConnectionData::Storage) && std::is_trivially_destructible<typename std::decay<Slot>::type>::value, int>::type> DataHandle EventLayer::create(NodeHandle node, Implementation::EventType eventType, Slot&& slot) {
    Implementation::EventConnectionData* data;
    const DataHandle handle = create(node, eventType, false, data);

    new(&data->storage.data) typename std::decay<Slot>::type{Utility::forward<Slot>(slot)};
    /* The + is to decay the lambda to a function pointer so we can cast it.
       MSVC 2015 says it's "illegal on a class" so there it's an explicit cast
       to the function pointer type (and the parentheses are for both to have
       only one ifdef). */
    data->call = reinterpret_cast<void(*)()>(
        #ifndef CORRADE_MSVC2015_COMPATIBILITY
        +
        #else
        static_cast<void(*)(Implementation::EventConnectionData::Storage& storage, Args&&...)>
        #endif
    ([](Implementation::EventConnectionData::Storage& storage, Args&&... args) {
        reinterpret_cast<Slot&>(storage.data)(Utility::forward<Args>(args)...);
    }));

    return handle;
}

/* Non-trivially-destructible or too large functor connection, allocating on
   heap */
template<class Slot, class ...Args, typename std::enable_if<(sizeof(typename std::decay<Slot>::type) > sizeof(Implementation::EventConnectionData::Storage)) || !std::is_trivially_destructible<typename std::decay<Slot>::type>::value, int>::type> DataHandle EventLayer::create(NodeHandle node, Implementation::EventType eventType, Slot&& slot) {
    Implementation::EventConnectionData* data;
    const DataHandle handle = create(node, eventType, true, data);

    reinterpret_cast<typename std::decay<Slot>::type*&>(data->storage.functor.data) = new typename std::decay<Slot>::type{Utility::forward<Slot>(slot)};
    data->storage.functor.destruct = [](Implementation::EventConnectionData::Storage& storage) {
        delete reinterpret_cast<typename std::decay<Slot>::type*>(storage.functor.data);
    };
    /* The + is to decay the lambda to a function pointer so we can cast it.
       MSVC 2015 says it's "illegal on a class" so there it's an explicit cast
       to the function pointer type (and the parentheses are for both to have
       only one ifdef). */
    data->call = reinterpret_cast<void(*)()>(
        #ifndef CORRADE_MSVC2015_COMPATIBILITY
        +
        #else
        static_cast<void(*)(Implementation::EventConnectionData::Storage& storage, Args&&...)>
        #endif
    ([](Implementation::EventConnectionData::Storage& storage, Args&&... args) {
        (*reinterpret_cast<typename std::decay<Slot>::type*>(storage.functor.data))(Utility::forward<Args>(args)...);
    }));

    return handle;
}

/* Member function connection */
template<class ReceiverObject, class Receiver, class ...Args> DataHandle EventLayer::create(NodeHandle node, Implementation::EventType eventType, ReceiverObject& receiver, void(Receiver::*slot)(Args...)) {
    Implementation::EventConnectionData* data;
    const DataHandle handle = create(node, eventType, false, data);

    static_assert(sizeof(slot) <= sizeof(data->storage.member.data),
        "size of member function pointer is incorrectly assumed to be smaller");
    static_assert(std::is_base_of<Receiver, ReceiverObject>::value,
        "receiver object doesn't have given slot");
    reinterpret_cast<void(Receiver::*&)(Args...)>(data->storage.member.data) = slot;
    data->storage.member.receiver = &receiver;
    /* The + is to decay the lambda to a function pointer so we can cast it.
       MSVC 2015 says it's "illegal on a class" so there it's an explicit cast
       to the function pointer type (and the parentheses are for both to have
       only one ifdef). */
    data->call = reinterpret_cast<void(*)()>(
        #ifndef CORRADE_MSVC2015_COMPATIBILITY
        +
        #else
        static_cast<void(*)(Implementation::EventConnectionData::Storage& storage, Args&&...)>
        #endif
    ([](Implementation::EventConnectionData::Storage& storage, Args&&... args) {
        (static_cast<ReceiverObject*>(storage.member.receiver)->*reinterpret_cast<void(Receiver::*&)(Args...)>(storage.member.data))(Utility::forward<Args>(args)...);
    }));

    return handle;
}

template<class Slot, class ...Args> EventConnection EventLayer::createScoped(NodeHandle node, Implementation::EventType eventType, Slot&& slot) {
    return EventConnection{*this, create<Slot, Args...>(node, eventType, Utility::forward<Slot>(slot))};
}

template<class ReceiverObject, class Receiver, class ...Args> EventConnection EventLayer::createScoped(NodeHandle node, Implementation::EventType eventType, ReceiverObject& receiver, void(Receiver::*slot)(Args...)) {
    return EventConnection{*this, create<ReceiverObject, Receiver, Args...>(node, eventType, receiver, slot)};
}
#endif

}}

#endif
