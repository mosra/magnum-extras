#ifndef Magnum_Ui_NumericStorage_h
#define Magnum_Ui_NumericStorage_h
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
 * @brief Class @ref Magnum::Ui::NumericStorage
 * @m_since_latest_{extras}
 */

#include "Magnum/Ui/DataLayer.h"
#include "Magnum/Ui/Tags.h"

#ifndef DOXYGEN_GENERATING_OUTPUT
namespace Corrade { namespace Containers { namespace Implementation {
    /* Defined in Corrade/Containers/StridedArrayView.h, used in NonOwned
       constructors to avoid having to include the whole thing */
    template<unsigned dimensions, class T> Size<dimensions>& sizeRef(StridedArrayView<dimensions, T>&);
}}}
#endif

namespace Magnum { namespace Ui {

namespace Implementation {
    template<class T> struct NumericStorageQueryTraits  {
        typedef T Type;
    };
    template<> struct NumericStorageQueryTraits<UnsignedByte>: NumericStorageQueryTraits<UnsignedInt> {};
    template<> struct NumericStorageQueryTraits<Byte>: NumericStorageQueryTraits<Int> {};
    template<> struct NumericStorageQueryTraits<UnsignedShort>: NumericStorageQueryTraits<UnsignedInt> {};
    template<> struct NumericStorageQueryTraits<Short>: NumericStorageQueryTraits<Int> {};
    template<> struct NumericStorageQueryTraits<Half>: NumericStorageQueryTraits<Float> {};

    /* Used in NonOwned constructors to avoid having to include the whole
       thing. Yeah, I know, it's horrible, but alternatives (like having a
       deinlined viewSize() helper instantiated for all possible types) are
       even worse... */
    template<UnsignedInt dimensions, class T> inline Containers::Size3D numericStorageViewSize(Containers::StridedArrayView<dimensions, T> view) {
        Containers::Size<dimensions> size = Containers::Implementation::sizeRef(view);
        Containers::Size3D out{1, 1, 1};
        for(UnsignedInt i = 0; i != dimensions; ++i)
            out[3 - dimensions + i] = size[i];
        return out;
    }
}

/**
@brief Multi-dimensional storage for numeric types
@m_since_latest_{extras}

Owns or references typed numeric data for use with @ref DataLayer, providing
also a way to update them along with customizable value range and steps for
incrementing and decrementing.

The template is explicitly instantiated for @relativeref{Magnum,UnsignedByte},
@relativeref{Magnum,Byte}, @relativeref{Magnum,UnsignedShort},
@relativeref{Magnum,Short}, @relativeref{Magnum,UnsignedInt},
@relativeref{Magnum,Int}, @relativeref{Magnum,UnsignedLong},
@relativeref{Magnum,Long}, @relativeref{Magnum,Float},
@relativeref{Magnum,Double}, @relativeref{Magnum,Half},
@relativeref{Magnum,Deg}, @relativeref{Magnum,Rad},
@relativeref{Magnum,Seconds} and @relativeref{Magnum,Nanoseconds}. To reduce
the amount of used @ref StorageQuery types, types smaller than 32 bits are
implicitly expanded to a corresponding 32-bit type in returned
@ref StorageQuery instances. Other types are currently not supported.
*/
template<class T> class NumericStorage: public AbstractStorage {
    public:
        /**
         * @brief Storage type
         *
         * Note that to reduce combinatorial explosion, the types retured by
         * queries don't always match the actual storage type. See @ref Type
         * for details.
         */
        typedef T StorageType;

        /**
         * @brief Query type
         *
         * To reduce the amount of used @ref StorageQuery types, types smaller
         * than 32 bits are implicitly expanded to a corresponding 32-bit type
         * in returned @ref StorageQuery instances, in particular for
         * @ref StorageType being @relativeref{Magnum,UnsignedByte} and
         * @relativeref{Magnum,UnsignedShort} the @ref StorageQuery type
         * returned by @ref value() expands to
         * @relativeref{Magnum,UnsignedInt}, @relativeref{Magnum,Byte} and
         * @relativeref{Magnum,Short} expands to @relativeref{Magnum,Int}, and
         * @relativeref{Magnum,Half} expands to @relativeref{Magnum,Float}.
         * Otherwise the storage and query type is the same.
         */
        typedef typename Implementation::NumericStorageQueryTraits<T>::Type Type;

        /**
         * @brief Construct a value-initialized single-item storage
         *
         * Equivalent to calling @ref NumericStorage(DataLayer&, ValueInitT, const Containers::Size3D&, StorageFlags)
         * with @p size being @cpp {1, 1, 1} @ce.
         * @see @ref ValueInit, @ref NumericStorage(DataLayer&, NoInitT, StorageFlags),
         *      @ref NumericStorage(DataLayer&, DirectInitT, const T&, StorageFlags),
         *      @ref NumericStorage(DataLayer&, NonOwnedT, T&, StorageFlags)
         */
        explicit NumericStorage(DataLayer& layer, ValueInitT, StorageFlags flags = {}): NumericStorage{layer, ValueInit, {1, 1, 1}, flags} {}

        /**
         * @brief Construct a value-initialized 1D storage
         *
         * Equivalent to calling @ref NumericStorage(DataLayer&, ValueInitT, const Containers::Size3D&, StorageFlags)
         * with @p size being @cpp {1, 1, size} @ce.
         * @see @ref ValueInit, @ref NumericStorage(DataLayer&, NoInitT, std::size_t, StorageFlags),
         *      @ref NumericStorage(DataLayer&, DirectInitT, std::size_t, const T&, StorageFlags),
         *      @ref NumericStorage(DataLayer&, NonOwnedT, const Containers::StridedArrayView1D<T>&, StorageFlags)
         */
        explicit NumericStorage(DataLayer& layer, ValueInitT, std::size_t size, StorageFlags flags = {}): NumericStorage{layer, ValueInit, {1, 1, size}, flags} {}

        /**
         * @brief Construct a value-initialized 2D storage
         *
         * Equivalent to calling @ref NumericStorage(DataLayer&, ValueInitT, const Containers::Size3D&, StorageFlags)
         * with @p size being @cpp {1, size[0], size[1]} @ce.
         * @see @ref ValueInit,
         *      @ref NumericStorage(DataLayer&, NoInitT, const Containers::Size2D&, StorageFlags),
         *      @ref NumericStorage(DataLayer&, DirectInitT, const Containers::Size2D&, const T&, StorageFlags),
         *      @ref NumericStorage(DataLayer&, NonOwnedT, const Containers::StridedArrayView2D<T>&, StorageFlags)
         */
        explicit NumericStorage(DataLayer& layer, ValueInitT, const Containers::Size2D& size, StorageFlags flags = {}): NumericStorage{layer, ValueInit, {1, size[0], size[1]}, flags} {}

        /**
         * @brief Construct a value-initialized 3D storage
         * @param layer     Data layer to create the storage in
         * @param size      Storage size
         * @param flags     Storage flags
         *
         * Delegates to @ref NumericStorage(DataLayer&, NoInitT, const Containers::Size3D&, StorageFlags)
         * and then initializes the storage, trivial types set to zero and
         * default constructor called otherwise. See the documentation of
         * @ref NumericStorage(DataLayer&, NoInitT, const Containers::Size3D&, StorageFlags)
         * for more information.
         * @see @ref ValueInit, @ref NumericStorage(DataLayer&, DirectInitT, const Containers::Size3D&, const T&, StorageFlags),
         *      @ref NumericStorage(DataLayer&, NonOwnedT, const Containers::StridedArrayView3D<T>&, StorageFlags)
         */
        explicit NumericStorage(DataLayer& layer, ValueInitT, const Containers::Size3D& size, StorageFlags flags = {}): AbstractStorage{layer, size, flags} {
            create(ValueInit);
        }

        /**
         * @brief Construct a value-initialized single-item storage using the default @ref DataLayer in given user interface
         *
         * Equivalent to calling @ref NumericStorage(UserInterface&, ValueInitT, const Containers::Size3D&, StorageFlags)
         * with @p size being @cpp {1, 1, 1} @ce.
         */
        template<class UserInterface> explicit NumericStorage(UserInterface& ui, ValueInitT, StorageFlags flags = {}): NumericStorage{ui, ValueInit, {1, 1, 1}, flags} {}

        /**
         * @brief Construct a value-initialized 1D storage using the default @ref DataLayer in given user interface
         *
         * Equivalent to calling @ref NumericStorage(UserInterface&, ValueInitT, const Containers::Size3D&, StorageFlags)
         * with @p size being @cpp {1, 1, size} @ce.
         */
        template<class UserInterface> explicit NumericStorage(UserInterface& ui, ValueInitT, std::size_t size, StorageFlags flags = {}): NumericStorage{ui, ValueInit, {1, 1, size}, flags} {}

        /**
         * @brief Construct a value-initialized 2D storage using the default @ref DataLayer in given user interface
         *
         * Equivalent to calling @ref NumericStorage(UserInterface&, ValueInitT, const Containers::Size3D&, StorageFlags)
         * with @p size being @cpp {1, size[0], size[1]} @ce.
         */
        template<class UserInterface> explicit NumericStorage(UserInterface& ui, ValueInitT, const Containers::Size2D& size, StorageFlags flags = {}): NumericStorage{ui, ValueInit, {1, size[0], size[1]}, flags} {}

        /**
         * @brief Construct a value-initialized 3D storage using the default @ref DataLayer in given user interface
         *
         * Like @ref NumericStorage(DataLayer&, ValueInitT, const Containers::Size3D&, StorageFlags)
         * but using the default @ref DataLayer available through
         * @ref UserInterface::dataLayer(). Expects that the @p ui contains a
         * @ref DataLayer instance.
         */
        template<class UserInterface> explicit NumericStorage(UserInterface& ui, ValueInitT, const Containers::Size3D& size, StorageFlags flags = {}): AbstractStorage{ui, size, flags} {
            create(ValueInit);
        }

        /**
         * @brief Construct a single-item storage without initializing its contents
         *
         * Equivalent to calling @ref NumericStorage(DataLayer&, NoInitT, const Containers::Size3D&, StorageFlags)
         * with @p size being @cpp {1, 1, 1} @ce.
         * @see @ref NoInit, @ref NumericStorage(DataLayer&, ValueInitT, StorageFlags),
         *      @ref NumericStorage(DataLayer&, DirectInitT, const T&, StorageFlags),
         *      @ref NumericStorage(DataLayer&, NonOwnedT, T&, StorageFlags)
         */
        explicit NumericStorage(DataLayer& layer, NoInitT, StorageFlags flags = {}): NumericStorage{layer, NoInit, {1, 1, 1}, flags} {}

        /**
         * @brief Construct a 1D storage without initializing its contents
         *
         * Equivalent to calling @ref NumericStorage(DataLayer&, ValueInitT, const Containers::Size3D&, StorageFlags)
         * with @p size being @cpp {1, 1, size} @ce.
         * @see @ref NoInit, @ref NumericStorage(DataLayer&, ValueInitT, std::size_t, StorageFlags),
         *      @ref NumericStorage(DataLayer&, DirectInitT, std::size_t, const T&, StorageFlags),
         *      @ref NumericStorage(DataLayer&, NonOwnedT, const Containers::StridedArrayView1D<T>&, StorageFlags)
         */
        explicit NumericStorage(DataLayer& layer, NoInitT, std::size_t size, StorageFlags flags = {}): NumericStorage{layer, NoInit, {1, 1, size}, flags} {}

        /**
         * @brief Construct a 2D storage without initializing its contents
         *
         * Equivalent to calling @ref NumericStorage(DataLayer&, ValueInitT, const Containers::Size3D&, StorageFlags)
         * with @p size being @cpp {1, size[0], size[1]} @ce.
         * @see @ref NoInit, @ref NumericStorage(DataLayer&, ValueInitT, const Containers::Size2D&, StorageFlags),
         *      @ref NumericStorage(DataLayer&, DirectInitT, const Containers::Size2D&, const T&, StorageFlags),
         *      @ref NumericStorage(DataLayer&, NonOwnedT, const Containers::StridedArrayView2D<T>&, StorageFlags)
         */
        explicit NumericStorage(DataLayer& layer, NoInitT, const Containers::Size2D& size, StorageFlags flags = {}): NumericStorage{layer, NoInit, {1, size[0], size[1]}, flags} {}

        /**
         * @brief Construct a 3D storage without initializing its contents
         * @param layer     Data layer to create the storage in
         * @param size      Storage size
         * @param flags     Storage flags
         *
         * Expects that the @p size is non-empty. While the stored value is
         * *not* initialized in any way, the @ref range() is set to min and max
         * representable values of given type, and @ref step() is set to
         * @cpp T(1) @ce.
         *
         * Delegates to @ref AbstractStorage::AbstractStorage(DataLayer&, const Containers::Size3D&, StorageFlags),
         * see its documentation for detailed description of all constraints.
         * @see @ref NoInit, @ref NumericStorage(DataLayer&, ValueInitT, const Containers::Size3D&, StorageFlags),
         *      @ref NumericStorage(DataLayer&, DirectInitT, const Containers::Size3D&, const T&, StorageFlags),
         *      @ref NumericStorage(DataLayer&, NonOwnedT, const Containers::StridedArrayView3D<T>&, StorageFlags)
         */
        explicit NumericStorage(DataLayer& layer, NoInitT, const Containers::Size3D& size, StorageFlags flags = {}): AbstractStorage{layer, size, flags} {
            create(NoInit);
        }

        /**
         * @brief Construct a single-item storage without initializing its contents using the default @ref DataLayer in given user interface
         *
         * Equivalent to calling @ref NumericStorage(UserInterface&, NoInitT, const Containers::Size3D&, StorageFlags)
         * with @p size being @cpp {1, 1, 1} @ce.
         */
        template<class UserInterface> explicit NumericStorage(UserInterface& ui, NoInitT, StorageFlags flags = {}): NumericStorage{ui, NoInit, {1, 1, 1}, flags} {}

        /**
         * @brief Construct a 1D storage without initializing its contents using the default @ref DataLayer in given user interface
         *
         * Equivalent to calling @ref NumericStorage(UserInterface&, NoInitT, const Containers::Size3D&, StorageFlags)
         * with @p size being @cpp {1, 1, size} @ce.
         */
        template<class UserInterface> explicit NumericStorage(UserInterface& ui, NoInitT, std::size_t size, StorageFlags flags = {}): NumericStorage{ui, NoInit, {1, 1, size}, flags} {}

        /**
         * @brief Construct a 2D storage without initializing its contents using the default @ref DataLayer in given user interface
         *
         * Equivalent to calling @ref NumericStorage(UserInterface&, NoInitT, const Containers::Size3D&, StorageFlags)
         * with @p size being @cpp {1, size[0], size[1]} @ce.
         */
        template<class UserInterface> explicit NumericStorage(UserInterface& ui, NoInitT, const Containers::Size2D& size, StorageFlags flags = {}): NumericStorage{ui, NoInit, {1, size[0], size[1]}, flags} {}

        /**
         * @brief Construct a 3D storage without initializing its contents using the default @ref DataLayer in given user interface
         *
         * Like @ref NumericStorage(DataLayer&, NoInitT, const Containers::Size3D&, StorageFlags)
         * but using the default @ref DataLayer available through
         * @ref UserInterface::dataLayer(). Expects that the @p ui contains a
         * @ref DataLayer instance.
         */
        template<class UserInterface> explicit NumericStorage(UserInterface& ui, NoInitT, const Containers::Size3D& size, StorageFlags flags = {}): AbstractStorage{ui, size, flags} {
            create(NoInit);
        }

        /**
         * @brief Construct a direct-initialized single-item storage
         *
         * Equivalent to calling @ref NumericStorage(DataLayer&, DirectInitT, const Containers::Size3D&, const T&, StorageFlags)
         * with @p size being @cpp {1, 1, 1} @ce.
         * @see @ref DirectInit, @ref NumericStorage(DataLayer&, ValueInitT, StorageFlags),
         *      @ref NumericStorage(DataLayer&, NoInitT, StorageFlags),
         *      @ref NumericStorage(DataLayer&, NonOwnedT, T&, StorageFlags)
         */
        explicit NumericStorage(DataLayer& layer, DirectInitT, const T& value, StorageFlags flags = {}): NumericStorage{layer, DirectInit, {1, 1, 1}, value, flags} {}

        /**
         * @brief Construct a direct-initialized 1D storage
         *
         * Equivalent to calling @ref NumericStorage(DataLayer&, DirectInitT, const Containers::Size3D&, const T&, StorageFlags)
         * with @p size being @cpp {1, 1, size} @ce.
         * @see @ref DirectInit, @ref NumericStorage(DataLayer&, ValueInitT, std::size_t, StorageFlags),
         *      @ref NumericStorage(DataLayer&, NoInitT, StorageFlags),
         *      @ref NumericStorage(DataLayer&, NonOwnedT, const Containers::StridedArrayView1D<T>&, StorageFlags)
         */
        explicit NumericStorage(DataLayer& layer, DirectInitT, std::size_t size, const T& value, StorageFlags flags = {}): NumericStorage{layer, DirectInit, {1, 1, size}, value, flags} {}

        /**
         * @brief Construct a direct-initialized 2D storage
         *
         * Equivalent to calling @ref NumericStorage(DataLayer&, DirectInitT, const Containers::Size3D&, const T&, StorageFlags)
         * with @p size being @cpp {1, size[0], size[1]} @ce.
         * @see @ref DirectInitT, @ref NumericStorage(DataLayer&, ValueInitT, const Containers::Size2D&, StorageFlags),
         *      @ref NumericStorage(DataLayer&, NoInitT, const Containers::Size2D&, StorageFlags),
         *      @ref NumericStorage(DataLayer&, NonOwnedT, const Containers::StridedArrayView2D<T>&, StorageFlags)
         */
        explicit NumericStorage(DataLayer& layer, DirectInitT, const Containers::Size2D& size, const T& value, StorageFlags flags = {}): NumericStorage{layer, DirectInit, {1, size[0], size[1]}, value, flags} {}

        /**
         * @brief Construct a direct-initialized 3D storage
         * @param layer     Data layer to create the storage in
         * @param size      Storage size
         * @param value     Value to initialize the storage with
         * @param flags     Storage flags
         *
         * Delegates to @ref NumericStorage(DataLayer&, NoInitT, const Containers::Size3D&, StorageFlags)
         * and then initializes the storage using @p value. See the
         * documentation of @ref NumericStorage(DataLayer&, NoInitT, const Containers::Size3D&, StorageFlags)
         * for more information.
         * @see @ref DirectInit, @ref NumericStorage(DataLayer&, ValueInitT, const Containers::Size3D&, StorageFlags),
         *      @ref NumericStorage(DataLayer&, NonOwnedT, const Containers::StridedArrayView3D<T>&, StorageFlags)
         */
        explicit NumericStorage(DataLayer& layer, DirectInitT, const Containers::Size3D& size, const T& value, StorageFlags flags = {}): AbstractStorage{layer, size, flags} {
            create(DirectInit, value);
        }

        /**
         * @brief Construct a direct-initialized single-item storage using the default @ref DataLayer in given user interface
         *
         * Equivalent to calling @ref NumericStorage(UserInterface&, DirectInitT, const Containers::Size3D&, const T&, StorageFlags)
         * with @p size being @cpp {1, 1, 1} @ce.
         */
        template<class UserInterface> explicit NumericStorage(UserInterface& ui, DirectInitT, const T& value, StorageFlags flags = {}): NumericStorage{ui, DirectInit, {1, 1, 1}, value, flags} {}

        /**
         * @brief Construct a direct-initialized 1D storage using the default @ref DataLayer in given user interface
         *
         * Equivalent to calling @ref NumericStorage(UserInterface&, DirectInitT, const Containers::Size3D&, const T&, StorageFlags)
         * with @p size being @cpp {1, 1, size} @ce.
         */
        template<class UserInterface> explicit NumericStorage(UserInterface& ui, DirectInitT, std::size_t size, const T& value, StorageFlags flags = {}): NumericStorage{ui, DirectInit, {1, 1, size}, value, flags} {}

        /**
         * @brief Construct a direct-initialized 2D storage using the default @ref DataLayer in given user interface
         *
         * Equivalent to calling @ref NumericStorage(UserInterface&, DirectInitT, const Containers::Size3D&, const T&, StorageFlags)
         * with @p size being @cpp {1, size[0], size[1]} @ce.
         */
        template<class UserInterface> explicit NumericStorage(UserInterface& ui, DirectInitT, const Containers::Size2D& size, const T& value, StorageFlags flags = {}): NumericStorage{ui, DirectInit, {1, size[0], size[1]}, value, flags} {}

        /**
         * @brief Construct a direct-initialized 3D storage using the default @ref DataLayer in given user interface
         *
         * Like @ref NumericStorage(DataLayer&, DirectInitT, const Containers::Size3D&, const T&, StorageFlags)
         * but using the default @ref DataLayer available through
         * @ref UserInterface::dataLayer(). Expects that the @p ui contains a
         * @ref DataLayer instance.
         */
        template<class UserInterface> explicit NumericStorage(UserInterface& ui, DirectInitT, const Containers::Size3D& size, const T& value, StorageFlags flags = {}): AbstractStorage{ui, size, flags} {
            create(DirectInit, value);
        }

        /**
         * @brief Construct a non-owned single-item storage
         *
         * Behaves like calling @ref NumericStorage(DataLayer&, NonOwnedT, const Containers::StridedArrayView3D<T>&, StorageFlags)
         * with @p value turned into a view of size @cpp {1, 1, 1} @ce but
         * results in a more compact internal representation.
         * @see @ref ValueInit, @ref NumericStorage(DataLayer&, ValueInitT, StorageFlags),
         *      @ref NumericStorage(DataLayer&, NoInitT, StorageFlags),
         *      @ref NumericStorage(DataLayer&, DirectInitT, const T&, StorageFlags)
         */
        explicit NumericStorage(DataLayer& layer, NonOwnedT, T& value, StorageFlags flags = {}): AbstractStorage{layer, {1, 1, 1}, flags} {
            create(NonOwned, value);
        }
        /** @overload */
        explicit NumericStorage(DataLayer& layer, NonOwnedT, const T& value, StorageFlags flags = {}): AbstractStorage{layer, {1, 1, 1}, flags} {
            create(NonOwned, value);
        }

        /**
         * @brief Constructing a non-owned storage from a r-value reference is not allowed
         *
         * This prevents the storage to be accidentally constructed from a
         * temporary value instead.
         */
        explicit NumericStorage(DataLayer& layer, NonOwnedT, T&& value, StorageFlags flags = {}) = delete;
        /** @copydoc NumericStorage(DataLayer&, NonOwnedT, T&&, StorageFlags) */
        explicit NumericStorage(DataLayer& layer, NonOwnedT, const T&& value, StorageFlags flags = {}) = delete;

        /**
         * @brief Construct a non-owned 1D storage
         *
         * Behaves like calling @ref NumericStorage(DataLayer&, NonOwnedT, const Containers::StridedArrayView3D<T>&, StorageFlags)
         * with @p values turned into a view of size
         * @cpp {1, 1, values.size()} @ce but results in a more compact
         * internal representation.
         * @see @ref NonOwned, @ref NumericStorage(DataLayer&, ValueInitT, std::size_t, StorageFlags),
         *      @ref NumericStorage(DataLayer&, NoInitT, std::size_t, StorageFlags),
         *      @ref NumericStorage(DataLayer&, DirectInitT, std::size_t, const T&, StorageFlags)
         */
        explicit NumericStorage(DataLayer& layer, NonOwnedT, const Containers::StridedArrayView1D<T>& values, StorageFlags flags = {}): AbstractStorage{layer, Implementation::numericStorageViewSize(values), flags} {
            /* Contrary to what the docs say, cannot just delegate to the 3D
               overload as that'd mean needing a StridedArrayView3D definition
               (and the internal data layout is different for 1D, 2D, 3D also).
               Instead we delegate to one of the createNonOwned() variants. */
            create(NonOwned, values);
        }
        /** @overload */
        explicit NumericStorage(DataLayer& layer, NonOwnedT, const Containers::StridedArrayView1D<const T>& values, StorageFlags flags = {}): AbstractStorage{layer, Implementation::numericStorageViewSize(values), flags} {
            /* See above for why we're not delegating to the 3D overload */
            create(NonOwned, values);
        }

        /**
         * @brief Construct a non-owned 2D storage
         *
         * Behaves like calling @ref NumericStorage(DataLayer&, NonOwnedT, const Containers::StridedArrayView2D<T>&, StorageFlags)
         * with @p values turned into a view of size
         * @cpp {1, values.size()[0], values.size()[1]} @ce but results in a
         * more compact internal representation.
         * @see @ref NonOwned, @ref NumericStorage(DataLayer&, ValueInitT, const Containers::Size2D&, StorageFlags),
         *      @ref NumericStorage(DataLayer&, NoInitT, const Containers::Size2D&, StorageFlags),
         *      @ref NumericStorage(DataLayer&, DirectInitT, const Containers::Size2D&, const T&, StorageFlags)
         */
        explicit NumericStorage(DataLayer& layer, NonOwnedT, const Containers::StridedArrayView2D<T>& values, StorageFlags flags = {}): AbstractStorage{layer, Implementation::numericStorageViewSize(values), flags} {
            /* See above for why we're not delegating to the 3D overload */
            create(NonOwned, values);
        }
        /** @overload */
        explicit NumericStorage(DataLayer& layer, NonOwnedT, const Containers::StridedArrayView2D<const T>& values, StorageFlags flags = {}): AbstractStorage{layer, Implementation::numericStorageViewSize(values), flags} {
            /* See above for why we're not delegating to the 3D overload */
            create(NonOwned, values);
        }

        /**
         * @brief Construct a non-owned 3D storage
         * @param layer     Data layer to create the storage in
         * @param values    Value view
         * @param flags     Storage flags
         *
         * Uses the @p values view as the storage instead of allocating it. The
         * @p values view is expected to be non-empty with its contents staying
         * in scope for the whole storage lifetime. If the @p values are a
         * @cpp const @ce view, data updates through the storage are not
         * possible. The @ref range() is set to min and max representable
         * values of given type, and @ref step() is set to @cpp T(1) @ce.
         *
         * It's expected that @ref setDirty() is called whenever the value is
         * modified externally. Also note that external modifications don't get
         * clamped against the @ref range() and as such the query values may
         * fall outside of it.
         *
         * Delegates to @ref AbstractStorage::AbstractStorage(DataLayer&, const Containers::Size3D&, StorageFlags),
         * see its documentation for detailed description of all constraints.
         *
         * Delegates to @ref NumericStorage(DataLayer&, NoInitT, StorageFlags)
         * and then uses the pointed-to @p value as the storage, along with
         * remembering whether it's mutable. See the documentation of
         * @ref NumericStorage(DataLayer&, NoInitT, StorageFlags) for more
         * information.
         *
         * The @p value is expected to stay in scope for the whole storage
         * lifetime. If the @p value is a @cpp const @ce reference, data
         * updates through the storage are not possible. It's expected that
         * @ref setDirty() is called whenever the value is modified externally.
         * Also note that there's no way to clamp external modifications
         * against the @ref range() and as such the query values may fall
         * outside of it.
         * @see @ref NonOwned, @ref NumericStorage(DataLayer&, ValueInitT, StorageFlags),
         *      @ref NumericStorage(DataLayer&, NoInitT, StorageFlags),
         *      @ref NumericStorage(DataLayer&, DirectInitT, const T&, StorageFlags)
         */
        explicit NumericStorage(DataLayer& layer, NonOwnedT, const Containers::StridedArrayView3D<T>& values, StorageFlags flags = {}): AbstractStorage{layer, Implementation::numericStorageViewSize(values), flags} {
            create(NonOwned, values);
        }
        /** @overload */
        explicit NumericStorage(DataLayer& layer, NonOwnedT, const Containers::StridedArrayView3D<const T>& values, StorageFlags flags = {}): AbstractStorage{layer, Implementation::numericStorageViewSize(values), flags} {
            create(NonOwned, values);
        }

        /**
         * @brief Construct a non-owned single-item storage using the default @ref DataLayer in given user interface
         *
         * Behaves like calling @ref NumericStorage(UserInterface&, NonOwnedT, const Containers::StridedArrayView3D<T>&, StorageFlags)
         * with @p values turned into a view of size @cpp {1, 1, 1} @ce but
         * results in a more compact internal representation.
         */
        template<class UserInterface> explicit NumericStorage(UserInterface& ui, NonOwnedT, T& value, StorageFlags flags = {}): AbstractStorage{ui, {1, 1, 1}, flags} {
            /* See above for why we're not delegating to the 3D overload */
            create(NonOwned, value);
        }
        /** @overload */
        template<class UserInterface> explicit NumericStorage(UserInterface& ui, NonOwnedT, const T& value, StorageFlags flags = {}): AbstractStorage{ui, {1, 1, 1}, flags} {
            /* See above for why we're not delegating to the 3D overload */
            create(NonOwned, value);
        }

        /** @copydoc NumericStorage(DataLayer&, NonOwnedT, T&&, StorageFlags) */
        template<class UserInterface> explicit NumericStorage(UserInterface& ui, NonOwnedT, T&& value, StorageFlags flags = {}) = delete;
        /** @copydoc NumericStorage(DataLayer&, NonOwnedT, T&&, StorageFlags) */
        template<class UserInterface> explicit NumericStorage(UserInterface& ui, NonOwnedT, const T&& value, StorageFlags flags = {}) = delete;

        /**
         * @brief Construct a non-owned 1D storage using the default @ref DataLayer in given user interface
         *
         * Behaves like calling @ref NumericStorage(UserInterface&, NonOwnedT, const Containers::StridedArrayView3D<T>&, StorageFlags)
         * with @p values turned into a view of size
         * @cpp {1, 1, values.size()} @ce but results in a more compact
         * internal representation.
         */
        template<class UserInterface> explicit NumericStorage(UserInterface& ui, NonOwnedT, const Containers::StridedArrayView1D<T>& values, StorageFlags flags = {}): AbstractStorage{ui, Implementation::numericStorageViewSize(values), flags} {
            /* See above for why we're not delegating to the 3D overload */
            create(NonOwned, values);
        }
        /** @overload */
        template<class UserInterface> explicit NumericStorage(UserInterface& ui, NonOwnedT, const Containers::StridedArrayView1D<const T>& values, StorageFlags flags = {}): AbstractStorage{ui, Implementation::numericStorageViewSize(values), flags} {
            /* See above for why we're not delegating to the 3D overload */
            create(NonOwned, values);
        }

        /**
         * @brief Construct a non-owned 2D storage using the default @ref DataLayer in given user interface
         *
         * Behaves like calling @ref NumericStorage(UserInterface&, NonOwnedT, const Containers::StridedArrayView3D<T>&, StorageFlags)
         * with @p values turned into a view of size
         * @cpp {1, values.size()[0], values.size()[1]} @ce but results in a
         * more compact internal representation.
         */
        template<class UserInterface> explicit NumericStorage(UserInterface& ui, NonOwnedT, const Containers::StridedArrayView2D<T>& values, StorageFlags flags = {}): AbstractStorage{ui, Implementation::numericStorageViewSize(values), flags} {
            /* See above for why we're not delegating to the 3D overload */
            create(NonOwned, values);
        }
        /** @overload */
        template<class UserInterface> explicit NumericStorage(UserInterface& ui, NonOwnedT, const Containers::StridedArrayView2D<const T>& values, StorageFlags flags = {}): AbstractStorage{ui, Implementation::numericStorageViewSize(values), flags} {
            /* See above for why we're not delegating to the 3D overload */
            create(NonOwned, values);
        }

        /**
         * @brief Construct a non-owned 3D storage using the default @ref DataLayer in given user interface
         *
         * Like @ref NumericStorage(DataLayer&, NonOwnedT, const Containers::StridedArrayView3D<T>&, StorageFlags)
         * but using the default @ref DataLayer available through @ref UserInterface::dataLayer().
         * Expects that the @p ui contains a @ref DataLayer instance.
         */
        template<class UserInterface> explicit NumericStorage(UserInterface& ui, NonOwnedT, const Containers::StridedArrayView3D<T>& values, StorageFlags flags = {}): AbstractStorage{ui, Implementation::numericStorageViewSize(values), flags} {
            create(NonOwned, values);
        }
        /** @overload */
        template<class UserInterface> explicit NumericStorage(UserInterface& ui, NonOwnedT, const Containers::StridedArrayView3D<const T>& values, StorageFlags flags = {}): AbstractStorage{ui, Implementation::numericStorageViewSize(values), flags} {
            create(NonOwned, values);
        }

        /**
         * @brief Construct a value-initialized single-item storage
         *
         * Alias to @ref NumericStorage(DataLayer&, ValueInitT, StorageFlags).
         * @see @ref NumericStorage(DataLayer&, NoInitT, StorageFlags),
         *      @ref NumericStorage(DataLayer&, DirectInitT, const T&, StorageFlags),
         *      @ref NumericStorage(DataLayer&, NonOwnedT, T&, StorageFlags)
         */
        explicit NumericStorage(DataLayer& layer, StorageFlags flags = {}): NumericStorage{layer, ValueInit, flags} {}

        /**
         * @brief Construct a value-initialized 1D storage
         *
         * Alias to @ref NumericStorage(DataLayer&, ValueInitT, std::size_t, StorageFlags).
         * @see @ref NumericStorage(DataLayer&, NoInitT, std::size_t, StorageFlags),
         *      @ref NumericStorage(DataLayer&, DirectInitT, std::size_t, const T&, StorageFlags),
         *      @ref NumericStorage(DataLayer&, NonOwnedT, const Containers::StridedArrayView1D<T>&, StorageFlags)
         */
        explicit NumericStorage(DataLayer& layer, std::size_t size, StorageFlags flags = {}): NumericStorage{layer, ValueInit, size, flags} {}

        /**
         * @brief Construct a value-initialized 2D storage
         *
         * Alias to @ref NumericStorage(DataLayer&, ValueInitT, const Containers::Size2D&, StorageFlags).
         * @see @ref NumericStorage(DataLayer&, NoInitT, const Containers::Size2D&, StorageFlags),
         *      @ref NumericStorage(DataLayer&, DirectInitT, const Containers::Size2D&, const T&, StorageFlags),
         *      @ref NumericStorage(DataLayer&, NonOwnedT, const Containers::StridedArrayView2D<T>&, StorageFlags)
         */
        explicit NumericStorage(DataLayer& layer, const Containers::Size2D& size, StorageFlags flags = {}): NumericStorage{layer, ValueInit, size, flags} {}

        /**
         * @brief Construct a value-initialized 3D storage
         *
         * Alias to @ref NumericStorage(DataLayer&, ValueInitT, const Containers::Size3D&, StorageFlags).
         * @see @ref NumericStorage(DataLayer&, NoInitT, const Containers::Size3D&, StorageFlags),
         *      @ref NumericStorage(DataLayer&, DirectInitT, const Containers::Size3D&, const T&, StorageFlags),
         *      @ref NumericStorage(DataLayer&, NonOwnedT, const Containers::StridedArrayView3D<T>&, StorageFlags)
         */
        explicit NumericStorage(DataLayer& layer, const Containers::Size3D& size, StorageFlags flags = {}): NumericStorage{layer, ValueInit, size, flags} {}

        /**
         * @brief Construct a value-initialized single-item storage using the default @ref DataLayer in given user interface
         *
         * Alias to @ref NumericStorage(UserInterface&, ValueInitT, StorageFlags).
         * @see @ref NumericStorage(UserInterface&, NoInitT, StorageFlags),
         *      @ref NumericStorage(UserInterface&, DirectInitT, const T&, StorageFlags),
         *      @ref NumericStorage(UserInterface&, NonOwnedT, T&, StorageFlags)
         */
        /* The enable_if is to make this constructor not picked over the
           implicitly-generated copy */
        template<class UserInterface, typename std::enable_if<std::is_convertible<UserInterface&, AbstractUserInterface&>::value, int>::type = 0> explicit NumericStorage(UserInterface& ui, StorageFlags flags = {}): NumericStorage{ui, ValueInit, flags} {}

        /**
         * @brief Construct a value-initialized 1D storage using the default @ref DataLayer in given user interface
         *
         * Alias to @ref NumericStorage(UserInterface&, ValueInitT, std::size_t, StorageFlags).
         * @see @ref NumericStorage(UserInterface&, NoInitT, std::size_t, StorageFlags),
         *      @ref NumericStorage(UserInterface&, DirectInitT, std::size_t, const T&, StorageFlags),
         *      @ref NumericStorage(UserInterface&, NonOwnedT, const Containers::StridedArrayView1D<T>&, StorageFlags)
         */
        template<class UserInterface> explicit NumericStorage(UserInterface& ui, std::size_t size, StorageFlags flags = {}): NumericStorage{ui, ValueInit, size, flags} {}

        /**
         * @brief Construct a value-initialized 2D storage using the default @ref DataLayer in given user interface
         *
         * Alias to @ref NumericStorage(UserInterface&, ValueInitT, const Containers::Size2D&, StorageFlags).
         * @see @ref NumericStorage(UserInterface&, NoInitT, const Containers::Size2D&, StorageFlags),
         *      @ref NumericStorage(UserInterface&, DirectInitT, const Containers::Size2D&, const T&, StorageFlags),
         *      @ref NumericStorage(UserInterface&, NonOwnedT, const Containers::StridedArrayView2D<T>&, StorageFlags)
         */
        template<class UserInterface> explicit NumericStorage(UserInterface& ui, const Containers::Size2D& size, StorageFlags flags = {}): NumericStorage{ui, ValueInit, size, flags} {}

        /**
         * @brief Construct a value-initialized 3D storage using the default @ref DataLayer in given user interface
         *
         * Alias to @ref NumericStorage(UserInterface&, ValueInitT, const Containers::Size3D&, StorageFlags).
         * @see @ref NumericStorage(UserInterface&, NoInitT, const Containers::Size3D&, StorageFlags),
         *      @ref NumericStorage(UserInterface&, DirectInitT, const Containers::Size3D&, const T&, StorageFlags),
         *      @ref NumericStorage(UserInterface&, NonOwnedT, const Containers::StridedArrayView3D<T>&, StorageFlags)
         */
        template<class UserInterface> explicit NumericStorage(UserInterface& ui, const Containers::Size3D& size, StorageFlags flags = {}): NumericStorage{ui, ValueInit, size, flags} {}

        /**
         * @brief Storage stride
         *
         * In case of an owned storage (created using the @ref ValueInit,
         * @ref NoInit or @ref DirectInit constructor variants) the elements
         * are stored tightly packed. In case of a non-owned storage (created
         * using the @ref NonOwned constructor) the stride can be arbitrary.
         */
        Containers::Stride3D stride() const;

        /** @brief Accepted value range */
        Containers::Pair<T, T> range() const;

        /**
         * @brief Set accepted value range
         * @return Reference to self (for method chaining)
         *
         * Expects that @p min isn't larger than @p max. The range is used to
         * clamp the value when setting, incrementing and decrementing it
         * through @ref StorageQuery APIs, as well as setting it to a min or
         * max. Note that setting the range *doesn't* clamp existing values in
         * the storage, it only affects future updates. Similarly it also
         * doesn't affect external updates in a non-owned storage.
         *
         * For integer types the default is the min and max representable
         * value, e.g. @cpp -32768 @ce and @cpp 32768 @ce for
         * @relativeref{Magnum,Short}, for floating-point types it's negative
         * and positive infinity.
         *
         * If the range is different from the previously used range, calling
         * this function causes the storage to be marked as dirty.
         * @see @ref StorageQuery::set(), @ref StorageQuery::increment(),
         *      @ref StorageQuery::decrement(), @ref StorageQuery::setToMin(),
         *      @ref StorageQuery::setToMax(), @ref isDirty()
         */
        const NumericStorage<T>& setRange(T min, T max) const;

        /** @brief Step for increment and decrement */
        T step() const;

        /**
         * @brief Set step for increment and decrement
         * @return Reference to self (for method chaining)
         *
         * Expects that @p step is not @cpp 0 @ce. The step is used just for
         * incrementing and decrementing the value (for as long as the
         * @ref range() allows), the values can still be set to any value
         * between individual steps without getting rounded.
         *
         * The default step is @cpp 1 @ce for both integer and floating-point
         * types. Setting a negative step value will cause the effect of
         * @ref StorageQuery::increment() and
         * @relativeref{StorageQuery,decrement()} to reverse.
         *
         * Unlike @ref setRange(), calling this function *does not* cause the
         * storage to be marked as dirty, as it doesn't have any effect on the
         * values
         * @see @ref StorageQuery::increment(), @ref StorageQuery::decrement()
         */
        const NumericStorage<T>& setStep(T step) const;

        /**
         * @brief Single-item storage value
         *
         * Expects that @ref size() is @cpp {1, 1, 1} @ce. If it's not, use one
         * of the @ref operator[]() overloads.
         *
         * See documentation of @ref operator[](const Containers::Size3D&) const
         * for details about what is all supported by the returned query.
         */
        StorageQuery<Type> value() const;

        /**
         * @brief Single-item storage value
         *
         * Equivalent to @ref value(), see its documentation for more
         * information.
         */
        operator StorageQuery<Type>() const { return value(); }

        /**
         * @brief Single-item storage value
         *
         * Equivalent to @ref value(), see its documentation for more
         * information.
         */
        StorageQuery<Type> operator->() const { return value(); }

        /**
         * @brief 1D storage value at given index
         *
         * Expects that @ref size() is @cpp {1, 1, size} @ce and @p index is
         * less than `size`. Use @ref operator[](const Containers::Size2D&) const
         * for indexing a 2D storage and @ref operator[](const Containers::Size3D&) const
         * for indexing a 3D storage.
         *
         * See documentation of @ref operator[](const Containers::Size3D&) const
         * for details about what is all supported by the returned query.
         */
        StorageQuery<Type> operator[](std::size_t index) const;

        /**
         * @brief 2D storage value at given index
         *
         * Expects that @ref size() is @cpp {1, size[0], size[1]} @ce and
         * @p index is less than `size`. Use
         * @ref operator[](const Containers::Size3D&) const for indexing a 3D
         * storage.
         *
         * See documentation of @ref operator[](const Containers::Size3D&) const
         * for details about what is all supported by the returned query.
         */
        StorageQuery<Type> operator[](const Containers::Size2D& index) const;

        /**
         * @brief 3D storage value at given index
         *
         * Expects that @p index is less than @ref size().
         *
         * The returned query supports @ref StorageOperation::Min and
         * @relativeref{StorageOperation,Max} for querying the value range.
         * Unless the storage is non-owned and immutable, the query implements
         * also an updater accepting @ref StorageOperation::Set,
         * @relativeref{StorageOperation,Increment} and
         * @relativeref{StorageOperation,Decrement}.
         *
         * Note that to reduce combinatorial type explosion, the query uses a
         * @ref Type which is always at least 32-bit. All value updates
         * ultimately use the original @ref StorageType however. Use
         * @ref value() "value<U>()" if you need a query in a different type.
         */
        StorageQuery<Type> operator[](const Containers::Size3D& index) const;

        /**
         * @brief Raw storage data
         *
         * Meant to be used mainly for diagnostic purposes. For mutable access
         * use the @ref StorageQuery instances returned by @ref value() and
         * @ref operator[]().
         *
         * In case of an owned storage (created using the @ref ValueInit,
         * @ref NoInit or @ref DirectInit constructor variants) the returned
         * view is tightly packed with a size of @ref size(). In case of a
         * non-owned storage (created using the @ref NonOwned constructor) the
         * returned view matches the one passed to the constructor, possibly
         * with extra dimensions added at the front.
         */
        Containers::StridedArrayView3D<const T> data() const;

    private:
        /* Common internals used by operator[]() */
        static Type query(const NumericStorage<T>& storage, const Containers::Size3D& index, const StorageOperation operation);
        static StorageUpdateState updater(const NumericStorage<T>& storage, const Containers::Size3D& index, const StorageOperation operation, const Type* value);

        /* All create() functions are called from either the DataLayer& or the
           UserInterface& constructors. The UserInterface& constructors
           delegate to the AbstractStorage UserInterface& constructor which
           performs various assertions, so these have to be member functions
           and not constructors. */
        void create(ValueInitT);
        void create(NoInitT);
        void create(DirectInitT, const T& value);
        /* Have to enumerate all variants in order to not need a definition of
           StridedArrayView in the header :( */
        void create(NonOwnedT, T& value) {
            createNonOwnedInternal(&value, false, nullptr, 0);
        }
        void create(NonOwnedT, const T& value) {
            createNonOwnedInternal(&value, true, nullptr, 0);
        }
        template<UnsignedInt dimensions> void create(NonOwnedT, const Containers::StridedArrayView<dimensions, T>& values);
        template<UnsignedInt dimensions> void create(NonOwnedT, const Containers::StridedArrayView<dimensions, const T>& values);
        /* Called from other create() functions, but calling it from the header
           would require including ArrayView so there's a void create(NoInitT)
           for use in header instead */
        MAGNUM_UI_LOCAL Containers::ArrayView<T> createNoInitInternal();
        /* Called from create(NonOwnedT) functions, not MAGNUM_UI_LOCAL so we
           can inline the trivial create(NonOwnedT) above */
        void createNonOwnedInternal(const void* pointer, bool immutable, const std::ptrdiff_t* stride, UnsignedInt dimensions);

        /* Called from operator[]() to decide on mutability */
        StorageOperations operations() const;
};

template<class T> StorageQuery<typename NumericStorage<T>::Type> NumericStorage<T>::value() const {
    /* The StorageQuery requires lambdas so can't just pass the query() /
       updater() static functions by pointer */
    const auto query = [](const NumericStorage<T>& storage, const StorageOperation operation) {
        return NumericStorage<T>::query(storage, {}, operation);
    };
    const auto updater = [](const NumericStorage<T>& storage, const StorageOperation operation, const Type* value) {
        return NumericStorage<T>::updater(storage, {}, operation, value);
    };
    /* If the storage is immutable, the query has no updater */
    const StorageOperations operations = this->operations();
    if(operations >= StorageOperation::Set)
        return {*this, operations, query, updater};
    return {*this, operations, query};
}

template<class T> StorageQuery<typename NumericStorage<T>::Type> NumericStorage<T>::operator[](const std::size_t index) const {
    /* The StorageQuery requires lambdas so can't just pass the query() /
       updater() static functions by pointer */
    const auto query = [](const NumericStorage<T>& storage, const std::size_t index, const StorageOperation operation) {
        return NumericStorage<T>::query(storage, {0, 0, index}, operation);
    };
    const auto updater = [](const NumericStorage<T>& storage, const std::size_t index, const StorageOperation operation, const Type* value) {
        return NumericStorage<T>::updater(storage, {0, 0, index}, operation, value);
    };
    /* If the storage is immutable, the query has no updater */
    const StorageOperations operations = this->operations();
    if(operations >= StorageOperation::Set)
        return {*this, index, operations, query, updater};
    return {*this, index, operations, query};
}

template<class T> StorageQuery<typename NumericStorage<T>::Type> NumericStorage<T>::operator[](const Containers::Size2D& index) const {
    /* The StorageQuery requires lambdas so can't just pass the query() /
       updater() static functions by pointer */
    const auto query = [](const NumericStorage<T>& storage, const Containers::Size2D& index, const StorageOperation operation) {
        return NumericStorage<T>::query(storage, {0, index[0], index[1]}, operation);
    };
    const auto updater = [](const NumericStorage<T>& storage, const Containers::Size2D& index, const StorageOperation operation, const Type* value) {
        return NumericStorage<T>::updater(storage, {0, index[0], index[1]}, operation, value);
    };
    /* If the storage is immutable, the query has no updater */
    const StorageOperations operations = this->operations();
    if(operations >= StorageOperation::Set)
        return {*this, index, operations, query, updater};
    return {*this, index, operations, query};
}

template<class T> StorageQuery<typename NumericStorage<T>::Type> NumericStorage<T>::operator[](const Containers::Size3D& index) const {
    /* The StorageQuery requires lambdas so can't just pass the query() /
       updater() static functions by pointer */
    const auto query = [](const NumericStorage<T>& storage, const Containers::Size3D& index, const StorageOperation operation) {
        return NumericStorage<T>::query(storage, index, operation);
    };
    const auto updater = [](const NumericStorage<T>& storage, const Containers::Size3D& index, const StorageOperation operation, const Type* value) {
        return NumericStorage<T>::updater(storage, index, operation, value);
    };
    /* If the storage is immutable, the query has no updater */
    const StorageOperations operations = this->operations();
    if(operations >= StorageOperation::Set)
        return {*this, index, operations, query, updater};
    return {*this, index, operations, query};
}

}}

#endif
