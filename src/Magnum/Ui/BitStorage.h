#ifndef Magnum_Ui_BitStorage_h
#define Magnum_Ui_BitStorage_h
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
 * @brief Class @ref Magnum::Ui::BitStorage
 * @m_since_latest_{extras}
 */

#include "Magnum/Ui/DataLayer.h"
#include "Magnum/Ui/Tags.h"

namespace Magnum { namespace Ui {

namespace Implementation {
    /* Used in NonOwned constructors to avoid having to include the whole
       thing. Yeah, I know, maybe I'm overdoing the strict header dependencies
       here ... */
    MAGNUM_UI_EXPORT std::size_t bitStorageViewSize(const Containers::StridedBitArrayView1D& view);
    MAGNUM_UI_EXPORT std::size_t bitStorageViewSize(const Containers::MutableStridedBitArrayView1D& view);
}

/**
@brief Bit storage
@m_since_latest_{extras}

Owns or references a single bit or a bit sequence, allowing storage queries to
toggle individual bits.

@attention Consistently with @relativeref{Corrade,Containers::BitArrayView},
    because the size represents bits and because the class additionally has to
    store initial offset in the first byte, on 32-bit systems the size is
    limited to 512M bits --- i.e., 64 MB of memory.

If you have a concrete @cpp enum @ce or @relativeref{Corrade,Containers::EnumSet}
in which you want to toggle individual values, an @ref EnumStorage may be a
better fit.
*/
class MAGNUM_UI_EXPORT BitStorage: public AbstractStorage {
    public:
        /**
         * @brief Construct a value-initialized single-bit storage
         *
         * Equivalent to calling @ref BitStorage(Owner&, ValueInitT, std::size_t, StorageFlags)
         * with @p size being @cpp 1 @ce.
         * @see @ref ValueInit, @ref BitStorage(Owner&, NoInitT, StorageFlags),
         *      @ref BitStorage(Owner&, DirectInitT, bool, StorageFlags),
         *      @ref BitStorage(Owner&, NonOwnedT, bool&, StorageFlags)
         */
        template<class Owner> explicit BitStorage(Owner& owner, ValueInitT, StorageFlags flags = {}): BitStorage{owner, ValueInit, 1, flags} {}

        /**
         * @brief Construct a value-initialized 1D storage
         * @param owner     @ref DataLayer to create the storage in or a
         *      @ref UserInterface instance to take a
         *      @ref UserInterface::dataLayer() from
         * @param size      Storage size
         * @param flags     Storage flags
         *
         * Delegates to @ref BitStorage(Owner&, NoInitT, std::size_t, StorageFlags)
         * and then initializes the storage to zero values. See the
         * documentation of @ref BitStorage(Owner&, NoInitT, std::size_t, StorageFlags)
         * for more information.
         * @see @ref ValueInit, @ref BitStorage(Owner&, DirectInitT, std::size_t, bool, StorageFlags),
         *      @ref BitStorage(Owner&, NonOwnedT, const Containers::MutableStridedBitArrayView1D&, StorageFlags)
         */
        template<class Owner> explicit BitStorage(Owner& owner, ValueInitT, std::size_t size, StorageFlags flags = {}): AbstractStorage{owner, size, flags} {
            create(ValueInit);
        }

        /**
         * @brief Construct a single-bit storage without initializing its contents
         *
         * Equivalent to calling @ref BitStorage(Owner&, NoInitT, std::size_t, StorageFlags)
         * with @p size being @cpp 1 @ce.
         * @see @ref NoInit, @ref BitStorage(Owner&, ValueInitT, StorageFlags),
         *      @ref BitStorage(Owner&, DirectInitT, bool, StorageFlags),
         *      @ref BitStorage(Owner&, NonOwnedT, bool&, StorageFlags)
         */
        template<class Owner> explicit BitStorage(Owner& owner, NoInitT, StorageFlags flags = {}): BitStorage{owner, NoInit, 1, flags} {}

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
         * @see @ref NoInit, @ref BitStorage(Owner&, ValueInitT, std::size_t, StorageFlags),
         *      @ref BitStorage(Owner&, DirectInitT, std::size_t, bool, StorageFlags),
         *      @ref BitStorage(Owner&, NonOwnedT, const Containers::MutableStridedBitArrayView1D&, StorageFlags)
         */
        template<class Owner> explicit BitStorage(Owner& owner, NoInitT, std::size_t size, StorageFlags flags = {}): AbstractStorage{owner, size, flags} {
            create(NoInit);
        }

        /**
         * @brief Construct a direct-initialized single-bit storage
         *
         * Equivalent to calling @ref BitStorage(Owner&, DirectInitT, std::size_t, bool, StorageFlags)
         * with @p size being @cpp 1 @ce.
         * @see @ref DirectInit, @ref BitStorage(Owner&, ValueInitT, StorageFlags),
         *      @ref BitStorage(Owner&, NoInitT, StorageFlags),
         *      @ref BitStorage(Owner&, NonOwnedT, bool&, StorageFlags)
         */
        template<class Owner> explicit BitStorage(Owner& owner, DirectInitT, bool value, StorageFlags flags = {}): BitStorage{owner, DirectInit, 1, value, flags} {}

        /**
         * @brief Construct a direct-initialized 1D storage
         * @param owner     @ref DataLayer to create the storage in or a
         *      @ref UserInterface instance to take a
         *      @ref UserInterface::dataLayer() from
         * @param size      Storage size
         * @param value     Value to initialize the storage with
         * @param flags     Storage flags
         *
         * Delegates to @ref BitStorage(Owner&, NoInitT, std::size_t, StorageFlags)
         * and then initializes the storage using @p value. The @p value is
         * subsequently also used as the default value instead of @cpp T{} @ce.
         * See the documentation of @ref BitStorage(Owner&, NoInitT, std::size_t, StorageFlags)
         * for more information.
         * @see @ref DirectInit, @ref BitStorage(Owner&, ValueInitT, std::size_t, StorageFlags),
         *      @ref BitStorage(Owner&, NonOwnedT, const Containers::MutableStridedBitArrayView1D&, StorageFlags)
         */
        template<class Owner> explicit BitStorage(Owner& owner, DirectInitT, std::size_t size, bool value, StorageFlags flags = {}): AbstractStorage{owner, size, flags} {
            create(DirectInit, value);
        }

        /**
         * @brief Construct a non-owned single-bit storage
         *
         * Equivalent to calling @ref BitStorage(Owner&, NonOwnedT, const Containers::MutableStridedBitArrayView1D&, StorageFlags)
         * with @p value turned into a view of size @cpp 1 @ce and bit offset
         * of @cpp 0 @ce. If you need to reference a non-@cpp bool @ce type or
         * a different bit, pass an appropriate
         * @ref Corrade::Containers::BasicBitArrayView "Containers::MutableBitArrayView"
         * instance instead.
         * @see @ref ValueInit, @ref BitStorage(Owner&, ValueInitT, StorageFlags),
         *      @ref BitStorage(Owner&, NoInitT, StorageFlags),
         *      @ref BitStorage(Owner&, DirectInitT, bool, StorageFlags)
         */
        template<class Owner> explicit BitStorage(Owner& owner, NonOwnedT, bool& value, StorageFlags flags = {}): AbstractStorage{owner, {1, 1, 1}, flags} {
            create(NonOwned, value);
        }
        /** @overload */
        template<class Owner> explicit BitStorage(Owner& owner, NonOwnedT, const bool& value, StorageFlags flags = {}): AbstractStorage{owner, {1, 1, 1}, flags} {
            create(NonOwned, value);
        }

        /**
         * @brief Constructing a non-owned storage from a r-value reference is not allowed
         *
         * This prevents the storage to be accidentally constructed from a
         * temporary value.
         */
        template<class Owner> explicit BitStorage(Owner& owner, NonOwnedT, bool&& value, StorageFlags flags = {}) = delete;
        /** @copydoc BitStorage(Owner&, NonOwnedT, bool&&, StorageFlags) */
        template<class Owner> explicit BitStorage(Owner& owner, NonOwnedT, const bool&& value, StorageFlags flags = {}) = delete;

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
         * @see @ref NonOwned, @ref BitStorage(Owner&, ValueInitT, std::size_t, StorageFlags),
         *      @ref BitStorage(Owner&, NoInitT, std::size_t, StorageFlags),
         *      @ref BitStorage(Owner&, DirectInitT, std::size_t, bool, StorageFlags)
         */
        template<class Owner> explicit BitStorage(Owner& owner, NonOwnedT, const Containers::MutableStridedBitArrayView1D& values, StorageFlags flags = {}): AbstractStorage{owner, Implementation::bitStorageViewSize(values), flags} {
            create(NonOwned, values);
        }
        /** @overload */
        template<class Owner> explicit BitStorage(Owner& owner, NonOwnedT, const Containers::StridedBitArrayView1D& values, StorageFlags flags = {}): AbstractStorage{owner, Implementation::bitStorageViewSize(values), flags} {
            create(NonOwned, values);
        }

        /**
         * @brief Construct a value-initialized single-bit storage
         *
         * Alias to @ref BitStorage(Owner&, ValueInitT, StorageFlags).
         * @see @ref BitStorage(Owner&, NoInitT, StorageFlags),
         *      @ref BitStorage(Owner&, DirectInitT, bool, StorageFlags),
         *      @ref BitStorage(Owner&, NonOwnedT, bool&, StorageFlags)
         */
        /* The enable_if is to make this constructor not picked over the
           implicitly-generated copy */
        template<class Owner, typename std::enable_if<std::is_convertible<Owner&, DataLayer&>::value || std::is_convertible<Owner&, AbstractUserInterface&>::value, int>::type = 0> explicit BitStorage(Owner& owner, StorageFlags flags = {}): BitStorage{owner, ValueInit, flags} {}

        /**
         * @brief Construct a value-initialized 1D storage
         *
         * Alias to @ref BitStorage(Owner&, ValueInitT, std::size_t, StorageFlags).
         * @see @ref BitStorage(Owner&, NoInitT, std::size_t, StorageFlags),
         *      @ref BitStorage(Owner&, DirectInitT, std::size_t, bool, StorageFlags),
         *      @ref BitStorage(Owner&, NonOwnedT, const Containers::MutableStridedBitArrayView1D&, StorageFlags)
         */
        template<class Owner> explicit BitStorage(Owner& owner, std::size_t size, StorageFlags flags = {}): BitStorage{owner, ValueInit, size, flags} {}

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
         * view or bit reference, @cpp false @ce otherwise.
         *
         * Immutable storages return immutable queries from @ref value() and
         * @ref operator[]() and don't allow accessing @ref mutableData().
         * @see @ref StorageQuery::isMutable()
         */
        bool isMutable() const;

        /** @brief Default value */
        bool defaultValue() const;

        /**
         * @brief Set default value
         * @return Reference to self (for method chaining)
         *
         * The default value is used when calling @ref StorageQuery::reset().
         * Initially is set to either @cpp false @ce or the value that was
         * passed to the @ref BitStorage(Owner&, DirectInitT, std::size_t, bool, StorageFlags)
         * constructor.
         *
         * Calling this function *does not* cause the storage to be marked as
         * dirty, as it doesn't have any effect on existing values.
         */
        const BitStorage& setDefaultValue(bool value) const;

        /**
         * @brief Single-item storage value
         *
         * Expects that @ref size() is @cpp 1 @ce. If it's not, use
         * @ref operator[]().
         *
         * See documentation of @ref operator[]() for details about what is all
         * supported by the returned query.
         */
        StorageQuery<bool> value() const;

        /**
         * @brief Single-bit storage value
         *
         * Equivalent to @ref value(), see its documentation for more
         * information.
         */
        operator StorageQuery<bool>() const { return value(); }

        /**
         * @brief Single-bit storage value
         *
         * Equivalent to @ref value(), see its documentation for more
         * information.
         */
        StorageQuery<bool> operator->() const { return value(); }

        /**
         * @brief 1D storage value at given index
         *
         * Expects that @p index is less than @ref size().
         *
         * Unless the storage is non-owned and immutable, the query implements
         * also an updater accepting @ref StorageOperation::Set,
         * @relativeref{StorageOperation,Reset} and
         * @relativeref{StorageOperation,Toggle}.
         */
        StorageQuery<bool> operator[](std::size_t index) const;

        /**
         * @brief Raw storage data
         *
         * For mutable access use either @ref mutableData() or the
         * @ref StorageQuery instances returned by @ref value() and
         * @ref operator[]().
         *
         * In case of an owned storage (created using the @ref ValueInit,
         * @ref NoInit or @ref DirectInit constructor variants) the returned
         * view is contiguous with a size of @ref size(), and the view is only
         * guaranteed to be valid for as long as no new storages are created in
         * the underlying @ref DataLayer. In case of a non-owned storage
         * (created using the @ref NonOwned constructor) the returned view
         * matches the one passed to the constructor.
         * @see @ref layer()
         */
        Containers::StridedBitArrayView1D data() const;

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
        Containers::MutableStridedBitArrayView1D mutableData() const;

    private:

        /* All create() functions are called from the constructors. If a
           UserInterface is passed they delegate to the (templated)
           AbstractStorage(UserInterface&) constructor which performs various
           assertions, so to not need to have these templated as well, they
           have to be member functions and not constructors. */
        void create(ValueInitT);
        void create(NoInitT);
        void create(DirectInitT, bool value);
        void create(NonOwnedT, bool& value) {
            createNonOwnedInternal(&value, false, 0, 1);
        }
        void create(NonOwnedT, const bool& value) {
            createNonOwnedInternal(&value, true, 0, 1);
        }
        void create(NonOwnedT, const Containers::MutableStridedBitArrayView1D& values);
        void create(NonOwnedT, const Containers::StridedBitArrayView1D& values);
        /* Called from other create() functions, but calling it from the header
           would require including BitArrayView so there's a void
           create(NoInitT) for use in header instead */
        MAGNUM_UI_LOCAL Containers::MutableBitArrayView createNoInitInternal(bool defaultValue);
        /* Called from create(NonOwnedT) functions, not MAGNUM_UI_LOCAL so we
           can inline the trivial create(NonOwnedT) above */
        void createNonOwnedInternal(const void* pointer, bool immutable, UnsignedByte offset, std::ptrdiff_t stride);
        /* Common internals used by value() and operator[]() */
        MAGNUM_UI_LOCAL bool query(std::size_t index, StorageOperation operation) const;
        MAGNUM_UI_LOCAL StorageUpdateState updater(std::size_t index, StorageOperation operation, const bool* value) const;
};

}}

#endif
