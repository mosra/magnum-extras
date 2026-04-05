#ifndef Magnum_Ui_Storage_h
#define Magnum_Ui_Storage_h
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
 * @brief Class @ref Magnum::Ui::Storage
 * @m_since_latest_{extras}
 */

#include <Corrade/Containers/StridedArrayView.h>

#include "Magnum/Ui/DataLayer.h"

namespace Magnum { namespace Ui {

/**
@brief Read-only non-owning multi-dimensional storage for arbitrary types
@m_since_latest_{extras}

Exposes a view on external values for use with @ref DataLayer. The values
cannot be modified through the storage. It's expected that @ref setDirty() is
called when the value is modified externally.
*/
template<class T> class Storage: public AbstractStorage {
    public:
        /** @brief Storage and query type */
        typedef T Type;

        /**
         * @brief Construct a single-item storage
         *
         * Equivalent to calling @ref Storage(DataLayer&, const Containers::StridedArrayView3D<const T>&, StorageFlags)
         * with @p value turned into a view of size @cpp {1, 1, 1} @ce.
         */
        explicit Storage(DataLayer& layer, const T& value, StorageFlags flags = {}): Storage{layer, Containers::stridedArrayView(&value, 1), flags} {}

        /**
         * @brief Construct a storage
         * @param layer     Data layer to create the storage in
         * @param values    Value view. Passing a 1D or 2D view will implicitly
         *      expand it to a 3D view, adding extra dimensions at the front.
         * @param flags     Storage flags
         *
         * The @p values view is expected to be non-empty with its contents
         * staying in scope for the whole storage lifetime. Delegates to
         * @ref AbstractStorage::AbstractStorage(DataLayer&, const Containers::Size3D&, StorageFlags),
         * see its documentation for detailed description of all constraints.
         */
        explicit Storage(DataLayer& layer, const Containers::StridedArrayView3D<const T>& values, StorageFlags flags = {}):  AbstractStorage{layer, values.size(), flags} {
            *createInPlace<Data>() = {
                values.data(),
                values.stride()
            };
        }

        /**
         * @brief Construct a single-item storage using the default @ref DataLayer in given user interface
         *
         * Equivalent to calling @ref Storage(DataLayer&, const Containers::StridedArrayView3D<const T>&, StorageFlags)
         * with @p value turned into a view of size @cpp {1, 1, 1} @ce.
         */
        template<class UserInterface> explicit Storage(UserInterface& ui, const T& value, StorageFlags flags = {}): Storage{ui, Containers::stridedArrayView(&value, 1), flags} {}

        /**
         * @brief Construct a storage using the default @ref DataLayer in given user interface
         *
         * Like @ref Storage(DataLayer&, const Containers::StridedArrayView3D<const T>&, StorageFlags)
         * but using the default @ref DataLayer available through
         * @ref UserInterface::dataLayer(). Expects that the @p ui contains a
         * @ref DataLayer instance.
         */
        template<class UserInterface> explicit Storage(UserInterface& ui, const Containers::StridedArrayView3D<const T>& values, StorageFlags flags = {}): AbstractStorage{ui, values.size(), flags} {
            *createInPlace<Data>() = {
                values.data(),
                values.stride()
            };
        }

        /** @brief Storage stride */
        Containers::Stride3D stride() const {
            return AbstractStorage::data<Data>()->stride;
        }

        /**
         * @brief Single-item storage value
         *
         * Expects that @ref size() is @cpp {1, 1, 1} @ce. If it's not, use one
         * of the @ref operator[]() overloads.
         */
        StorageQuery<T> value() const {
            return {*this, [](const Storage<T>& storage) {
                /* The StorageQuery requires lambdas so can't just pass the
                   query() static function by pointer */
                return query(storage, {});
            }};
        }

        /**
         * @brief Single-item storage value query
         *
         * Equivalent to @ref value(), see its documentation for more
         * information.
         */
        operator StorageQuery<T>() const { return value(); }

        /**
         * @brief 1D storage value at given index
         *
         * Expects that @ref size() is @cpp {1, 1, size} @ce and @p index is
         * less than `size`. Use @ref operator[](const Containers::Size2D&) const
         * for indexing a 2D storage and @ref operator[](const Containers::Size3D&) const
         * for indexing a 3D storage.
         */
        StorageQuery<T> operator[](std::size_t index) const {
            return {*this, index, [](const Storage<T>& storage, const std::size_t index) {
                /* The StorageQuery requires lambdas so can't just pass the
                   query() static function by pointer */
                return query(storage, {0, 0, index});
            }};
        }

        /**
         * @brief 2D storage value at given index
         *
         * Expects that @ref size() is @cpp {1, size[0], size[1]} @ce and
         * @p index is less than `size`. Use
         * @ref operator[](const Containers::Size3D&) const for indexing a 3D
         * storage.
         */
        StorageQuery<T> operator[](const Containers::Size2D& index) const {
            return {*this, index, [](const Storage<T>& storage, const Containers::Size2D& index) {
                /* The StorageQuery requires lambdas so can't just pass the
                   query() static function by pointer */
                return query(storage, {0, index[0], index[1]});
            }};
        }

        /**
         * @brief 3D storage value at given index
         *
         * Expects that @p index is less than @ref size().
         */
        StorageQuery<T> operator[](const Containers::Size3D& index) const {
            return {*this, index, [](const Storage<T>& storage, const Containers::Size3D& index) {
                /* The StorageQuery requires lambdas so can't just pass the
                   query() static function by pointer */
                return query(storage, index);
            }};
        }

        /**
         * @brief Single-item storage value at given index, cast to a different type
         *
         * Expects that @ref size() is @cpp {1, 1, 1} @ce. If it's not, use one
         * of the other overloads.
         */
        template<class U> StorageQuery<U> value() const {
            return {*this, [](const Storage<T>& storage) {
                /* The StorageQuery requires lambdas so can't just pass the
                   query() static function by pointer */
                return U(query(storage, {}));
            }};
        }

        /**
         * @brief 1D storage value at given index, cast to a different type
         *
         * Expects that @ref size() is @cpp {1, 1, size} @ce and @p index is
         * less than `size`. Use @ref value(const Containers::Size2D&) const
         * for indexing a 2D storage and @ref value(const Containers::Size3D&) const
         * for indexing a 3D storage.
         */
        template<class U> StorageQuery<U> value(std::size_t index) const {
            return {*this, index, [](const Storage<T>& storage, std::size_t index) {
                /* The StorageQuery requires lambdas so can't just pass the
                   query() static function by pointer */
                return U(query(storage, {0, 0, index}));
            }};
        }

        /**
         * @brief 2D storage value at given index, cast to a different type
         *
         * Expects that @ref size() is @cpp {1, size[0], size[1]} @ce and
         * @p index is less than `size`. Use
         * @ref operator[](const Containers::Size3D&) const for indexing a 3D
         * storage.
         */
        template<class U> StorageQuery<U> value(const Containers::Size2D& index) const {
            return {*this, index, [](const Storage<T>& storage, const Containers::Size2D& index) {
                /* The StorageQuery requires lambdas so can't just pass the
                   query() static function by pointer */
                return U(query(storage, {0, index[0], index[1]}));
            }};
        }

        /**
         * @brief 3D storage value query, cast to a different type
         *
         * Expects that @p index is less than @ref size().
         */
        template<class U> StorageQuery<U> value(const Containers::Size3D& index) const {
            return {*this, index, [](const Storage<T>& storage, const Containers::Size3D& index) {
                /* The StorageQuery requires lambdas so can't just pass the
                   query() static function by pointer */
                return U(query(storage, index));
            }};
        }

        /**
         * @brief Raw storage data
         *
         * Meant to be used mainly for diagnostic purposes, for value access
         * prefer to use the @ref StorageQuery instances returned by
         * @ref operator[]() and @ref value(). The returned view matches the
         * one passed to the constructor.
         */
        Containers::StridedArrayView3D<const T> data() const {
            const Data& data = *AbstractStorage::data<Data>();
            /* We're sure the view is correctly sized so fake the size */
            return {{static_cast<const T*>(data.pointer), ~std::size_t{}}, size(), data.stride};
        }

    private:
        struct Data {
            const void* pointer;
            Containers::Stride3D stride;
        };

        /* Common internals shared by operator[]() and value<U>() */
        static T query(const Storage<T>& storage, const Containers::Size3D& index) {
            /* Almost a Rust-level code with the **.::<>() */
            const Data& data = *storage.AbstractStorage::data<Data>();
            const char* pointer = static_cast<const char*>(data.pointer);
            for(UnsignedInt i = 0; i != 3; ++i)
                /* Casting to std::ptrdiff_t to avoid cursed issues like in
                   StridedArrayView itself, where it sometimes led to overflows
                   due to a wrong result type picked. See StridedElement::get()
                   there for details. */
                pointer += std::ptrdiff_t(index[i])*data.stride[i];
            return *static_cast<const T*>(static_cast<const void*>(pointer));
        };
};

}}

#endif
