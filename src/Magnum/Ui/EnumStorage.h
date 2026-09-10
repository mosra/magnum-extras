#ifndef Magnum_Ui_EnumStorage_h
#define Magnum_Ui_EnumStorage_h
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

/** @file
 * @brief Class @ref Magnum::Ui::EnumStorage
 * @m_since_latest_{extras}
 */

#include <Corrade/Containers/StridedArrayView.h>

#include "Magnum/Ui/DataLayer.h"
#include "Magnum/Ui/Tags.h"

namespace Magnum { namespace Ui {

namespace Implementation {

/* If the base implementation is all-catching, MSVC 2015 directs EnumSet there
   as well, instead of picking the EnumSet specialization. Maybe it's because
   EnumSet is usually typedef'd, maybe because it cannot figure out the nested
   T. What works instead is not having any all-catching base but instead define
   both the integer and the EnumSet variant with a SFINAE. */
#ifdef CORRADE_MSVC2015_COMPATIBILITY
template<class T, class = void> struct EnumStorageTraits;
template<class T> struct EnumStorageTraits<T, typename std::enable_if<std::is_integral<T>::value>::type> {
    static_assert(!std::is_same<T, bool>::value, "only integral and enum types accepted for EnumStorage");
    typedef T UnderlyingType;
    typedef T ValueType;
    enum: bool { EnumSet = false };
};
template<class T> struct EnumStorageTraits<T, typename std::enable_if<sizeof(typename T::UnderlyingType)>::type> {
    typedef typename T::UnderlyingType UnderlyingType;
    typedef typename T::Type ValueType;
    enum: bool { EnumSet = true };
};
#else
template<class T, class = void> struct EnumStorageTraits {
    static_assert(std::is_integral<T>::value && !std::is_same<T, bool>::value, "only integral and enum types accepted for EnumStorage");
    typedef T UnderlyingType;
    typedef T ValueType;
    enum: bool { EnumSet = false };
};
template<class T> struct EnumStorageTraits<Containers::EnumSet<T>> {
    typedef typename Containers::EnumSet<T>::UnderlyingType UnderlyingType;
    typedef T ValueType;
    enum: bool { EnumSet = true };
};
#endif
template<class T> struct EnumStorageTraits<T, typename std::enable_if<std::is_enum<T>::value>::type> {
    typedef typename std::underlying_type<T>::type UnderlyingType;
    typedef T ValueType;
    enum: bool { EnumSet = false };
};

}

/**
@brief Enum value storage
@m_since_latest_{extras}

Owns or references a single enum value or their list, allowing storage queries
to either access and update the enum value directly, or toggle individual
choices in the enum.

The template can be used for any @cpp enum @ce, @cpp enum class @ce as well as
builtin integer types, and a @relativeref{Corrade,Containers::EnumSet}. It's
not allowed to use this storage with a @cpp bool @ce type as the semantics
would be unclear.
*/
template<class T> class EnumStorage: public AbstractStorage {
    public:
        /** @brief Storage and query type */
        typedef T Type;

        /**
         * @brief Enum value type
         *
         * Equal to @p Type for @cpp enum @ce and integer types, and to
         * @relativeref{Corrade,Containers::EnumSet::Type} for an
         * @relativeref{Corrade,Containers::EnumSet}.
         */
        typedef typename Implementation::EnumStorageTraits<T>::ValueType ValueType;

        /**
         * @brief Construct a value-initialized single-item storage
         *
         * Equivalent to calling @ref EnumStorage(Owner&, ValueInitT, std::size_t, StorageFlags)
         * with @p size being @cpp 1 @ce.
         * @see @ref ValueInit, @ref EnumStorage(Owner&, NoInitT, StorageFlags),
         *      @ref EnumStorage(Owner&, DirectInitT, T, StorageFlags),
         *      @ref EnumStorage(Owner&, NonOwnedT, T&, StorageFlags)
         */
        template<class Owner> explicit EnumStorage(Owner& owner, ValueInitT, StorageFlags flags = {}): EnumStorage{owner, ValueInit, 1, flags} {}

        /**
         * @brief Construct a value-initialized 1D storage
         * @param owner     @ref DataLayer to create the storage in or a
         *      @ref UserInterface instance to take a
         *      @ref UserInterface::dataLayer() from
         * @param size      Storage size
         * @param flags     Storage flags
         *
         * Delegates to @ref EnumStorage(Owner&, NoInitT, std::size_t, StorageFlags)
         * and then initializes the storage to zero values. See the
         * documentation of @ref EnumStorage(Owner&, NoInitT, std::size_t, StorageFlags)
         * for more information.
         * @see @ref ValueInit, @ref EnumStorage(Owner&, DirectInitT, std::size_t, T, StorageFlags),
         *      @ref EnumStorage(Owner&, NonOwnedT, const Containers::StridedArrayView1D<T>&, StorageFlags)
         */
        template<class Owner> explicit EnumStorage(Owner& owner, ValueInitT, std::size_t size, StorageFlags flags = {});

        /**
         * @brief Construct a single-item storage without initializing its contents
         *
         * Equivalent to calling @ref EnumStorage(Owner&, NoInitT, std::size_t, StorageFlags)
         * with @p size being @cpp 1 @ce.
         * @see @ref NoInit, @ref EnumStorage(Owner&, ValueInitT, StorageFlags),
         *      @ref EnumStorage(Owner&, DirectInitT, T, StorageFlags),
         *      @ref EnumStorage(Owner&, NonOwnedT, T&, StorageFlags)
         */
        template<class Owner> explicit EnumStorage(Owner& owner, NoInitT, StorageFlags flags = {}): EnumStorage{owner, NoInit, 1, flags} {}

        /**
         * @brief Construct a 1D storage without initializing its contents
         * @param owner     @ref DataLayer to create the storage in or a
         *      @ref UserInterface instance to take a
         *      @ref UserInterface::dataLayer() from
         * @param size      Storage size
         * @param flags     Storage flags
         *
         * Expects that the @p size is non-zero. If @p owner is a user
         * interface reference, expects that it contains a @ref DataLayer
         * instance. While the stored values are *not* initialized in any way,
         * the @ref defaultValue() is set to @cpp T{} @ce. You can use
         * @ref mutableData() to fill the storage upon creation.
         *
         * Delegates to either @ref AbstractStorage::AbstractStorage(DataLayer&, std::size_t, StorageFlags)
         * or @ref AbstractStorage::AbstractStorage(UserInterface&, std::size_t, StorageFlags),
         * see their documentation for detailed description of all constraints.
         * @see @ref NoInit, @ref EnumStorage(Owner&, ValueInitT, std::size_t, StorageFlags),
         *      @ref EnumStorage(Owner&, DirectInitT, std::size_t, T, StorageFlags),
         *      @ref EnumStorage(Owner&, NonOwnedT, const Containers::StridedArrayView1D<T>&, StorageFlags)
         */
        template<class Owner> explicit EnumStorage(Owner& owner, NoInitT, std::size_t size, StorageFlags flags = {}): AbstractStorage{owner, size, flags} {
            createNoInitInternal(T{});
        }

        /**
         * @brief Construct a direct-initialized single-item storage
         *
         * Equivalent to calling @ref EnumStorage(Owner&, DirectInitT, std::size_t, T, StorageFlags)
         * with @p size being @cpp 1 @ce.
         * @see @ref DirectInit, @ref EnumStorage(Owner&, ValueInitT, StorageFlags),
         *      @ref EnumStorage(Owner&, NoInitT, StorageFlags),
         *      @ref EnumStorage(Owner&, NonOwnedT, T&, StorageFlags)
         */
        template<class Owner> explicit EnumStorage(Owner& owner, DirectInitT, T value, StorageFlags flags = {}): EnumStorage{owner, DirectInit, 1, value, flags} {}

        /**
         * @brief Construct a direct-initialized 1D storage
         * @param owner     @ref DataLayer to create the storage in or a
         *      @ref UserInterface instance to take a
         *      @ref UserInterface::dataLayer() from
         * @param size      Storage size
         * @param value     Value to initialize the storage with
         * @param flags     Storage flags
         *
         * Delegates to @ref EnumStorage(Owner&, NoInitT, std::size_t, StorageFlags)
         * and then initializes the storage using @p value. The @p value is
         * subsequently also used as the default value instead of @cpp T{} @ce.
         * See the documentation of @ref EnumStorage(Owner&, NoInitT, std::size_t, StorageFlags)
         * for more information.
         * @see @ref DirectInit, @ref EnumStorage(Owner&, ValueInitT, std::size_t, StorageFlags),
         *      @ref EnumStorage(Owner&, NonOwnedT, const Containers::StridedArrayView1D<T>&, StorageFlags)
         */
        template<class Owner> explicit EnumStorage(Owner& owner, DirectInitT, std::size_t size, T value, StorageFlags flags = {});

        /**
         * @brief Construct a non-owned single-item storage
         *
         * Equivalent to calling @ref EnumStorage(Owner&, NonOwnedT, const Containers::StridedArrayView1D<T>&, StorageFlags)
         * with @p value turned into a view of size @cpp 1 @ce.
         * @see @ref ValueInit, @ref EnumStorage(Owner&, ValueInitT, StorageFlags),
         *      @ref EnumStorage(Owner&, NoInitT, StorageFlags),
         *      @ref EnumStorage(Owner&, DirectInitT, T, StorageFlags)
         */
        template<class Owner> explicit EnumStorage(Owner& owner, NonOwnedT, T& value, StorageFlags flags = {}): EnumStorage{owner, NonOwned, Containers::StridedArrayView1D<T>{&value, 1}, flags} {}
        /** @overload */
        template<class Owner> explicit EnumStorage(Owner& owner, NonOwnedT, const T& value, StorageFlags flags = {}): EnumStorage{owner, NonOwned, {&value, 1}, flags} {}

        /**
         * @brief Constructing a non-owned storage from a r-value reference is not allowed
         *
         * This prevents the storage to be accidentally constructed from a
         * temporary value.
         */
        template<class Owner> explicit EnumStorage(Owner& owner, NonOwnedT, T&& value, StorageFlags flags = {}) = delete;
        /** @copydoc EnumStorage(Owner&, NonOwnedT, T&&, StorageFlags) */
        template<class Owner> explicit EnumStorage(Owner& owner, NonOwnedT, const T&& value, StorageFlags flags = {}) = delete;

        /**
         * @brief Construct a non-owned 1D storage
         * @param owner     @ref DataLayer to create the storage in or a
         *      @ref UserInterface instance to take a
         *      @ref UserInterface::dataLayer() from
         * @param values    Value view
         * @param flags     Storage flags
         *
         * Uses the @p values view as the storage instead of allocating it. If
         * @p owner is a user interface reference, expects that it contains a
         * @ref DataLayer instance. The @p values view is expected to be
         * non-empty with its contents staying in scope for the whole storage
         * lifetime. If the @p values are a @cpp const @ce view, data updates
         * through the storage are not possible.
         *
         * It's expected that @ref setDirty() is called whenever the values are
         * modified externally.
         *
         * Delegates to either @ref AbstractStorage::AbstractStorage(DataLayer&, std::size_t, StorageFlags)
         * or @ref AbstractStorage::AbstractStorage(UserInterface&, std::size_t, StorageFlags),
         * see their documentation for detailed description of all constraints.
         * @see @ref NonOwned, @ref EnumStorage(Owner&, ValueInitT, std::size_t, StorageFlags),
         *      @ref EnumStorage(Owner&, NoInitT, std::size_t, StorageFlags),
         *      @ref EnumStorage(Owner&, DirectInitT, std::size_t, T, StorageFlags)
         */
        template<class Owner> explicit EnumStorage(Owner& owner, NonOwnedT, const Containers::StridedArrayView1D<T>& values, StorageFlags flags = {}): EnumStorage{owner, NonOwned, Containers::StridedArrayView1D<const T>{values}, flags} {
            AbstractStorage::data<DataNonOwned>()->nonOwnedImmutable = false;
        }
        /** @overload */
        /* On 32-bit platforms, if T is aligned to 64 bits, the alignment
           padding alone makes DataNonOwned not fit in-place, have to allocate
           instead */
        template<class Owner
            #ifdef CORRADE_TARGET_32BIT
            , class U = T, typename std::enable_if<(sizeof(U) < 8), int>::type = 0
            #endif
        > explicit EnumStorage(Owner& owner, NonOwnedT, const Containers::StridedArrayView1D<const T>& values, StorageFlags flags = {});
        #ifdef CORRADE_TARGET_32BIT
        template<class Owner, class U = T, typename std::enable_if<(sizeof(U) >= 8), int>::type = 0> explicit EnumStorage(Owner& owner, NonOwnedT, const Containers::StridedArrayView1D<const T>& values, StorageFlags flags = {});
        #endif

        /**
         * @brief Construct a value-initialized single-item storage
         *
         * Alias to @ref EnumStorage(Owner&, ValueInitT, StorageFlags).
         * @see @ref EnumStorage(Owner&, NoInitT, StorageFlags),
         *      @ref EnumStorage(Owner&, DirectInitT, T, StorageFlags),
         *      @ref EnumStorage(Owner&, NonOwnedT, T&, StorageFlags)
         */
        /* The enable_if is to make this constructor not picked over the
           implicitly-generated copy */
        template<class Owner, typename std::enable_if<std::is_convertible<Owner&, DataLayer&>::value || std::is_convertible<Owner&, AbstractUserInterface&>::value, int>::type = 0> explicit EnumStorage(Owner& owner, StorageFlags flags = {}): EnumStorage{owner, ValueInit, flags} {}

        /**
         * @brief Construct a value-initialized 1D storage
         *
         * Alias to @ref EnumStorage(Owner&, ValueInitT, std::size_t, StorageFlags).
         * @see @ref EnumStorage(Owner&, NoInitT, std::size_t, StorageFlags),
         *      @ref EnumStorage(Owner&, DirectInitT, std::size_t, T, StorageFlags),
         *      @ref EnumStorage(Owner&, NonOwnedT, const Containers::StridedArrayView1D<T>&, StorageFlags)
         */
        template<class Owner> explicit EnumStorage(Owner& owner, std::size_t size, StorageFlags flags = {}): EnumStorage{owner, ValueInit, size, flags} {}

        /**
         * @brief Storage size
         *
         * Returns just the last component of @ref AbstractStorage::size() as
         * the storage is at most 1D.
         */
        std::size_t size() const { return AbstractStorage::size()[2]; }

        /**
         * @brief Whether the storage is mutable
         *
         * Returns @cpp true @ce if the storage is owned (i.e., created using
         * the @ref ValueInit, @ref NoInit or @ref DirectInit constructor) or
         * if it's created using the @ref NonOwned constructor from a mutable
         * view or value reference, @cpp false @ce otherwise.
         *
         * Immutable storages return immutable queries from @ref value() and
         * @ref operator[]() and don't allow accessing @ref mutableData().
         * @see @ref StorageQuery::isMutable()
         */
        bool isMutable() const {
            return !AbstractStorage::data<Data>()->nonOwnedImmutable;
        }

        /** @brief Default value */
        T defaultValue() const {
            return AbstractStorage::data<Data>()->defaultValue;
        }

        /**
         * @brief Set default value
         * @return Reference to self (for method chaining)
         *
         * The default value is used when calling @ref StorageQuery::reset().
         * Initially is set to either @cpp T{} @ce or the value that was passed
         * to the @ref EnumStorage(Owner&, DirectInitT, std::size_t, T, StorageFlags)
         * constructor.
         *
         * Calling this function *does not* cause the storage to be marked as
         * dirty, as it doesn't have any effect on existing values.
         */
        const EnumStorage<T>& setDefaultValue(T value) const {
            /* Not calling setDirty() in this case as this doesn't affect the
               stored values in any way */
            AbstractStorage::data<Data>()->defaultValue = value;
            return *this;
        }

        /** @brief Whether the storage behaves like an enum set */
        bool isEnumSet() const {
            return AbstractStorage::data<Data>()->enumSet;
        }

        /**
         * @brief Set the storage to behave like an enum set
         *
         * By default, for @cpp enum @ce types and plain integers, the storage
         * behaves like an enum, allowing to pick *exactly one* of the possible
         * enumerated values via the @ref value() "value<expected>()" queries
         * --- the query being @cpp true @ce if `expected` is equal to the
         * stored value. In other words, behaving like radio buttons.
         *
         * For @relativeref{Corrade,Containers::EnumSet} types it's by default
         * behaving like an enum set, allowing to pick a *set of values*, with
         * @ref value() "value<expected>()" queries being @cpp true @ce if
         * `expected` is a bitwise subset of the stored value. In other words,
         * behaving like checkboxes.
         *
         * This function allows to switch the default, such as when a bitmask
         * is represented by a plain integer instead of a
         * @relativeref{Corrade,Containers::EnumSet}.
         */
        const EnumStorage<T>& setEnumSet(bool set) const {
            /* Not calling setDirty() in this case as the setting isn't meant
               to affect existing values */
            AbstractStorage::data<Data>()->enumSet = set;
            return *this;
        }

        /**
         * @brief Single-item storage value
         *
         * Expects that @ref size() is @cpp 1 @ce. If it's not, use
         * @ref operator[]().
         *
         * See documentation of @ref operator[]() for details about what is all
         * supported by the returned query.
         */
        StorageQuery<T> value() const;

        /**
         * @brief Single-item storage value
         *
         * Equivalent to @ref value(), see its documentation for more
         * information.
         */
        operator StorageQuery<T>() const { return value(); }

        /**
         * @brief Single-item storage value
         *
         * Equivalent to @ref value(), see its documentation for more
         * information.
         */
        StorageQuery<T> operator->() const { return value(); }

        /**
         * @brief 1D storage value at given index
         *
         * Expects that @p index is less than @ref size().
         *
         * Unless the storage is non-owned and immutable, the query implements
         * also an updater accepting @ref StorageOperation::Set and
         * @relativeref{StorageOperation,Reset}.
         */
        StorageQuery<T> operator[](std::size_t index) const;

        /**
         * @brief Single-item storage value presence
         *
         * Expects that @ref size() is @cpp 1 @ce. If it's not, use
         * @ref value(std::size_t) const.
         *
         * See documentation of @ref value(std::size_t) const for details about
         * what is all supported by the returned query.
         */
        template<ValueType expected> StorageQuery<bool> value() const;

        /**
         * @brief 1D storage value presence at given index
         *
         * Unless the storage is non-owned and immutable, the query implements
         * also an updater accepting @ref StorageOperation::Set,
         * @relativeref{StorageOperation,Reset} and
         * @relativeref{StorageOperation,Toggle}.
         *
         * For enum storages, the query is @cpp true @ce if @p expected is
         * equal to the stored value. If the storage is mutable,
         * @ref StorageOperation::Set to @cpp true @ce sets the stored value to
         * @p expected and @ref StorageOperation::Toggle sets the stored value
         * to @p expected if it's not already, both returning
         * @ref StorageUpdateState::Success. @ref StorageOperation::Set to
         * @cpp false @ce, @ref StorageOperation::Reset and
         * @ref StorageOperation::Toggle if the storage value is already
         * @p expected result in @ref StorageUpdateState::Failed, as there's no
         * generally unambiguous semantics for those operations.
         *
         * For enum set storages, the query is @cpp true @ce if @p expected
         * is a bitwise subset of @p expected. If the storage is mutable,
         * @ref StorageOperation::Set sets or resets bits of @p expected in the
         * stored value, @ref StorageOperation::Reset sets bits of @p expected
         * to their value in @ref defaultValue(), @ref StorageOperation::Toggle
         * sets bits of @p expected if the stored value doesn't have them all
         * set, and resets them if the stored value has them all set.
         *
         * @attention Note that for enum set storages, using @p expected with
         *      zero bits set is likely an error, as it causes the query to be
         *      @cpp true @ce always, and any @ref StorageOperation to be a
         *      no-op that always results in @ref StorageUpdateState::Success.
         *      This is however not checked by the implementation in any way as
         *      for non-enum-set storages being @cpp false @ce this is a valid
         *      use.
         *
         * @see @ref isEnumSet(), @ref isMutable()
         */
        template<ValueType expected> StorageQuery<bool> value(std::size_t index) const;

        /**
         * @brief Raw storage data
         *
         * For mutable access use either @ref mutableData() or the
         * @ref StorageQuery instances returned by @ref value() and
         * @ref operator[]().
         *
         * In case of an owned storage (created using the @ref ValueInit,
         * @ref NoInit or @ref DirectInit constructor variants) the returned
         * view is tightly packed with a size of @ref size(), and the view
         * is only guaranteed to be valid for as long as no new storages are
         * created in the underlying @ref DataLayer. In case of a non-owned
         * storage (created using the @ref NonOwned constructor) the returned
         * view matches the one passed to the constructor.
         * @see @ref layer()
         */
        Containers::StridedArrayView1D<const T> data() const;

        /**
         * @brief Raw mutable storage data
         *
         * Like @ref data() const, but returns a mutable view. Expects that the
         * storage is either owned or non-owned and mutable. Meant to be used
         * mainly for filling a storage created using the @ref NoInit
         * constructor variant, or for batch updates. Note that the user is
         * responsible for calling @ref setDirty() upon a modification in order
         * to correctly trigger updates on associated data bindings.
         * @see @ref isMutable()
         */
        Containers::StridedArrayView1D<T> mutableData() const;

    private:
        struct Data;
        struct DataNonOwned;

        /* Returns a view that ValueInit and DirectInit constructors use to
           fill the contents, thus cannot be inlined inside the NoInit
           constructor */
        Containers::ArrayView<T> createNoInitInternal(T defaultValue);

        /* Shared code used by operator[]() and value() */
        StorageOperations operations() const {
            return AbstractStorage::data<Data>()->nonOwnedImmutable ?
                StorageOperations{} : StorageOperation::Set|StorageOperation::Reset;
        }
        StorageOperations bitOperations() const {
            return AbstractStorage::data<Data>()->nonOwnedImmutable ?
                StorageOperations{} : StorageOperation::Set|StorageOperation::Reset|StorageOperation::Toggle;
        }
        /* Common internals used by operator[]() and value() */
        T query(std::size_t index, StorageOperation operation) const;
        template<ValueType expected> bool query(std::size_t index, StorageOperation operation) const;
        StorageUpdateState updater(std::size_t index, StorageOperation operation, const T* value) const;
        template<ValueType expected> StorageUpdateState updater(std::size_t index, StorageOperation operation, const bool* value) const;
};

#ifndef DOXYGEN_GENERATING_OUTPUT
template<class T> struct EnumStorage<T>::Data {
    explicit Data(T defaultValue, bool nonOwned = false, bool nonOwnedImmutable = false): defaultValue{defaultValue}, nonOwned{nonOwned}, nonOwnedImmutable{nonOwnedImmutable} {}

    T defaultValue;
    bool nonOwned;
    bool nonOwnedImmutable;
    bool enumSet = Implementation::EnumStorageTraits<T>::EnumSet;
    /* 5 bytes free for owned/non-owned 64-bit T on 64-bit and for owned 64-bit
       T on 32-bit, 1 byte free for a non-owned 64-bit T on 32-bit */
    /* For owned data, T[] is then right after, tightly packed, with array
       length matching the storage size. For non-owned data the derived
       DataNonOwned struct is used instead. */

    /* For non-owned data DataNonOwned::pointer should be used instead */
    T* pointer() {
        CORRADE_INTERNAL_DEBUG_ASSERT(!nonOwned);
        return reinterpret_cast<T*>(this + 1);
    }

    /* For non-owned data DataNonOwned::data() should be called instead */
    T& data(std::size_t index) {
        CORRADE_INTERNAL_DEBUG_ASSERT(!nonOwned);
        return reinterpret_cast<T*>(this + 1)[index];
    }
};

template<class T> struct EnumStorage<T>::DataNonOwned: Data {
    /* The default for non-owned data is always a default-constructed T. The
       immutable flag gets subsequently reset in the mutable constructor. */
    explicit DataNonOwned(const void* pointer, std::ptrdiff_t stride): Data{T{}, /*nonOwned*/ true, /*immutable*/ true}, pointer{pointer}, stride{stride} {}

    const void* pointer;
    std::ptrdiff_t stride;

    const T& data(std::size_t index) {
        /* Casting to std::ptrdiff_t to avoid cursed issues like in
           StridedArrayView itself, where it sometimes led to overflows due to
           a wrong result type picked. See StridedElement::get() there for
           details. */
        return *reinterpret_cast<const T*>(static_cast<const char*>(pointer) + std::ptrdiff_t(index)*stride);
    }
};
#endif

template<class T> Containers::ArrayView<T> EnumStorage<T>::createNoInitInternal(T defaultValue) {
    const std::size_t count = size();
    const std::size_t dataSize = sizeof(Data) + sizeof(T)*count;

    /* Single-item storage should fit even for an 8-byte type */
    char* const storage = dataSize <= MaxInPlaceSize ?
        createInPlace<char>() :
        createAllocated(new char[dataSize], 1, [](void* data, std::size_t) {
            delete[] static_cast<char*>(data);
        });

    /* Construct the Data struct in-place to initialize its members */
    Data* data = new(storage) Data{defaultValue};
    return {data->pointer(), count};
}

template<class T> template<class Owner> EnumStorage<T>::EnumStorage(Owner& owner, ValueInitT, const std::size_t size, const StorageFlags flags): AbstractStorage{owner, size, flags} {
    /** @todo Utility::fill() for this once it exists */
    for(T& i: createNoInitInternal(T{}))
        i = T{};
}

template<class T> template<class Owner> EnumStorage<T>::EnumStorage(Owner& owner, DirectInitT, const std::size_t size, T value, const StorageFlags flags): AbstractStorage{owner, size, flags} {
    /** @todo Utility::fill() for this once it exists */
    for(T& i: createNoInitInternal(value))
        i = value;
}

template<class T> template<class Owner
    #ifdef CORRADE_TARGET_32BIT
    , class U, typename std::enable_if<(sizeof(U) < 8), int>::type
    #endif
> EnumStorage<T>::EnumStorage(Owner& owner, NonOwnedT, const Containers::StridedArrayView1D<const T>& values, const StorageFlags flags): AbstractStorage{owner, values.size(), flags} {
    /* Construct the Data struct in-place to initialize its members. On 64-bit
       platforms the size should always fit in-place, on 32-bit only up to
       four bytes, which is guarded by the SFINAE above. */
    new(createInPlace<DataNonOwned>()) DataNonOwned{values.data(), values.stride()};
}
#ifdef CORRADE_TARGET_32BIT
template<class T> template<class Owner, class U, typename std::enable_if<(sizeof(U) >= 8), int>::type> EnumStorage<T>::EnumStorage(Owner& owner, NonOwnedT, const Containers::StridedArrayView1D<const T>& values, const StorageFlags flags): AbstractStorage{owner, values.size(), flags} {
    createAllocated(new DataNonOwned{values.data(), values.stride()}, 1, [](void* data, std::size_t) {
        delete static_cast<DataNonOwned*>(data);
    });
}
#endif

template<class T> T EnumStorage<T>::query(const std::size_t index, const StorageOperation operation) const {
    Data& data = *AbstractStorage::data<Data>();
    CORRADE_INTERNAL_DEBUG_ASSERT(operation == StorageOperation{});
    #ifdef CORRADE_NO_DEBUG_ASSERT
    static_cast<void>(operation);
    #endif

    if(data.nonOwned)
        return static_cast<DataNonOwned&>(data).data(index);
    return data.data(index);
}

template<class T> template<typename EnumStorage<T>::ValueType expected> bool EnumStorage<T>::query(const std::size_t index, const StorageOperation operation) const {
    Data& data = *AbstractStorage::data<Data>();
    CORRADE_INTERNAL_DEBUG_ASSERT(operation == StorageOperation{});
    #ifdef CORRADE_NO_DEBUG_ASSERT
    static_cast<void>(operation);
    #endif

    T value;
    if(data.nonOwned)
        value = static_cast<DataNonOwned&>(data).data(index);
    else
        value = data.data(index);
    return data.enumSet ?
        (static_cast<typename Implementation::EnumStorageTraits<T>::UnderlyingType>(value) & static_cast<typename Implementation::EnumStorageTraits<T>::UnderlyingType>(expected)) == static_cast<typename Implementation::EnumStorageTraits<T>::UnderlyingType>(expected) :
        value == expected;
}

template<class T> StorageUpdateState EnumStorage<T>::updater(const std::size_t index, const StorageOperation operation, const T* const value) const {
    Data& data = *AbstractStorage::data<Data>();

    /* Perform desired operation */
    StorageUpdateState state;
    T valueToUpdate;
    if(operation == StorageOperation::Set) {
        valueToUpdate = *value;
        state = StorageUpdateState::Success;
    } else if(operation == StorageOperation::Reset) {
        valueToUpdate = data.defaultValue;
        state = StorageUpdateState::Success;
    } else CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */

    /* Update the value and mark the storage as dirty only if it actually
       changed */
    T& currentValue = data.nonOwned ?
        const_cast<T&>(static_cast<DataNonOwned&>(data).data(index)) :
        data.data(index);
    if(valueToUpdate != currentValue) {
        currentValue = valueToUpdate;
        setDirty();
    }

    return state;
}

template<class T> template<typename EnumStorage<T>::ValueType expected> StorageUpdateState EnumStorage<T>::updater(const std::size_t index, const StorageOperation operation, const bool* const value) const {
    Data& data = *AbstractStorage::data<Data>();

    T& currentValue = data.nonOwned ?
        const_cast<T&>(static_cast<DataNonOwned&>(data).data(index)) :
        data.data(index);

    /* Perform desired operation */
    StorageUpdateState state;
    auto valueToUpdate = static_cast<typename Implementation::EnumStorageTraits<T>::UnderlyingType>(currentValue);
    const auto expectedUnderlyingType = static_cast<typename Implementation::EnumStorageTraits<T>::UnderlyingType>(expected);

    /* Set a value, or enum set bits */
    if(operation == StorageOperation::Set) {
        if(data.enumSet) {
            *value ?
                valueToUpdate |= expectedUnderlyingType :
                valueToUpdate &= ~expectedUnderlyingType;
            state = StorageUpdateState::Success;
        } else if(*value) {
            valueToUpdate = expectedUnderlyingType;
            state = StorageUpdateState::Success;
        /* Setting a single non-enum-set value to false doesn't have a defined
           semantics right now. Resetting the value to zero feels weird (why
           zero? might not be a valid value at all), resetting to the default
           is equally weird for opposite reasons, as set is usually independent
           from the default value setting. */
        } else state = StorageUpdateState::Failed;

    /* Reset a value, or enum set bits */
    } else if(operation == StorageOperation::Reset) {
        const auto defaultValueUnderlyingType = static_cast<typename Implementation::EnumStorageTraits<T>::UnderlyingType>(data.defaultValue);
        /* Here we replace bits from `expected` with corresponding bits from
          `defaultValue`, i.e. first clear all bits set in `expected` and then
          set bits of `defaultValue` present in `expected` */
        if(data.enumSet) {
            valueToUpdate = (valueToUpdate & ~expectedUnderlyingType)|(expectedUnderlyingType & defaultValueUnderlyingType);
            state = StorageUpdateState::Success;
        /* Otherwise a reset doesn't have a defined semantics right now */
        } else state = StorageUpdateState::Failed;

    /* Toggle a value, or enum set bits */
    } else if(operation == StorageOperation::Toggle) {
        /* If there's multiple enum set bits to toggle and not all of them are
           set, interpret that the same as none of them being set and set them
           all (instead of toggling each bit individually, resulting in again
           the multiple bits being neither all set or not set). An alternative
           decision would be treat a partial selection as all being set, and
           unset them in that case, but I think that's less common behavior. */
        if(data.enumSet) {
            (valueToUpdate & expectedUnderlyingType) == expectedUnderlyingType ?
                valueToUpdate &= ~expectedUnderlyingType :
                valueToUpdate |= expectedUnderlyingType;
            state = StorageUpdateState::Success;
        /* Otherwise, for a single-choice storage (a radio button), set the
           value to expected if it's not, and fail if it already is. Because
           there's no concept of "toggling the value away" in that case. This
           also means a checkbox and a radio button can be both implemented
           with a toggle operation -- for a checkbox it'll set or reset given
           bit, for a radio button it'll switch to it if not set already, and
           do nothing if it's already set. */
        } else if(valueToUpdate != expectedUnderlyingType) {
            valueToUpdate = expectedUnderlyingType;
            state = StorageUpdateState::Success;
        } else state = StorageUpdateState::Failed;
    } else CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */

    /* Update the value and mark the storage as dirty only if it actually
       changed */
    if(T(valueToUpdate) != currentValue) {
        currentValue = T(valueToUpdate);
        setDirty();
    }

    return state;
}

template<class T> StorageQuery<T> EnumStorage<T>::value() const {
    const auto query = [](const EnumStorage<T>& storage, const StorageOperation operation) {
        return storage.query(0, operation);
    };
    const auto updater = [](const EnumStorage<T>& storage, const StorageOperation operation, const Type* value) {
        return storage.updater(0, operation, value);
    };
    /* If the storage is immutable, the query has no updater */
    const StorageOperations operations = this->operations();
    if(operations >= StorageOperation::Set)
        return {*this, operations, query, updater};
    return {*this, operations, query};
}

template<class T> StorageQuery<T> EnumStorage<T>::operator[](const std::size_t index) const {
    const auto query = [](const EnumStorage<T>& storage, const std::size_t index, const StorageOperation operation) {
        return storage.query(index, operation);
    };
    const auto updater = [](const EnumStorage<T>& storage, const std::size_t index, const StorageOperation operation, const Type* value) {
        return storage.updater(index, operation, value);
    };
    /* If the storage is immutable, the query has no updater */
    const StorageOperations operations = this->operations();
    if(operations >= StorageOperation::Set)
        return {*this, index, operations, query, updater};
    return {*this, index, operations, query};
}

template<class T> template<typename EnumStorage<T>::ValueType expected> StorageQuery<bool> EnumStorage<T>::value() const {
    const auto query = [](const EnumStorage<T>& storage, const StorageOperation operation) {
        return storage.template query<expected>(0, operation);
    };
    const auto updater = [](const EnumStorage<T>& storage, const StorageOperation operation, const bool* value) {
        return storage.template updater<expected>(0, operation, value);
    };
    /* If the storage is immutable, the query has no updater */
    const StorageOperations operations = bitOperations();
    if(operations >= StorageOperation::Set)
        return {*this, operations, query, updater};
    return {*this, operations, query};
}

template<class T> template<typename EnumStorage<T>::ValueType expected> StorageQuery<bool> EnumStorage<T>::value(const std::size_t index) const {
    const auto query = [](const EnumStorage<T>& storage, const std::size_t index, const StorageOperation operation) {
        return storage.template query<expected>(index, operation);
    };
    const auto updater = [](const EnumStorage<T>& storage, const std::size_t index, const StorageOperation operation, const bool* value) {
        return storage.template updater<expected>(index, operation, value);
    };
    /* If the storage is immutable, the query has no updater */
    const StorageOperations operations = bitOperations();
    if(operations >= StorageOperation::Set)
        return {*this, index, operations, query, updater};
    return {*this, index, operations, query};
}

template<class T> Containers::StridedArrayView1D<const T> EnumStorage<T>::data() const {
    Data& data = *AbstractStorage::data<Data>();

    /* We're sure the memory is correctly sized so fake the ArrayView size */
    const std::size_t size = AbstractStorage::size()[2];
    if(data.nonOwned) {
        DataNonOwned& dataNonOwned = static_cast<DataNonOwned&>(data);
        return {{static_cast<const T*>(dataNonOwned.pointer), ~std::size_t{}}, size, dataNonOwned.stride};
    }
    return {{data.pointer(), ~std::size_t{}}, size, sizeof(T)};
}

template<class T> Containers::StridedArrayView1D<T> EnumStorage<T>::mutableData() const {
    CORRADE_ASSERT(!AbstractStorage::data<Data>()->nonOwnedImmutable,
        "Ui::EnumStorage::mutableData(): data not mutable", {});
    Containers::StridedArrayView1D<const T> out = data();
    /* We're sure the memory is correctly sized so fake the ArrayView size */
    return {{const_cast<T*>(static_cast<const T*>(out.data())), ~std::size_t{}}, out.size(), out.stride()};
}

}}

#endif
