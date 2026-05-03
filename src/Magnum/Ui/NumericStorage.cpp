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

#include "NumericStorage.h"

#include <Corrade/Containers/Pair.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Magnum/Math/Half.h>
#include <Magnum/Math/Time.h>
#include <Magnum/Math/Vector.h> /* clamp() */

namespace Magnum { namespace Ui {

namespace {

enum class Flag : UnsignedByte {
    NonOwned = 1 << 0,
    NonOwnedImmutable = NonOwned|(1 << 1),
    /* 3D is NonOwned1D|NonOwned2D, single-item is neither */
    NonOwned1D = 1 << 2,
    NonOwned2D = 1 << 3,
};
typedef Containers::EnumSet<Flag> Flags;
#ifdef CORRADE_TARGET_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#endif
CORRADE_ENUMSET_OPERATORS(Flags)
#ifdef CORRADE_TARGET_CLANG
#pragma clang diagnostic pop
#endif

template<class T> struct Traits;

template<class T> struct IntegerTraits {
    constexpr static T min() { return Math::TypeTraits<T>::min(); }
    constexpr static T max() { return Math::TypeTraits<T>::max(); }
    constexpr static T step() { return 1; }
};
template<> struct Traits<UnsignedByte>: IntegerTraits<UnsignedByte> {
    typedef UnsignedInt ArithmeticType;
};
template<> struct Traits<Byte>: IntegerTraits<Byte>  {
    typedef Int ArithmeticType;
};
template<> struct Traits<UnsignedShort>: IntegerTraits<UnsignedShort>  {
    typedef UnsignedInt ArithmeticType;
};
template<> struct Traits<Short>: IntegerTraits<Short>  {
    typedef Int ArithmeticType;
};
template<> struct Traits<UnsignedInt>: IntegerTraits<UnsignedInt>  {
    typedef UnsignedInt ArithmeticType;
};
template<> struct Traits<Int>: IntegerTraits<Int>  {
    typedef Int ArithmeticType;
};
template<> struct Traits<UnsignedLong>: IntegerTraits<UnsignedLong>  {
    typedef UnsignedLong ArithmeticType;
};
template<> struct Traits<Long>: IntegerTraits<Long>  {
    typedef Long ArithmeticType;
};

template<> struct Traits<Float> {
    typedef Float ArithmeticType;
    constexpr static Float min() { return -Constants::inf(); }
    constexpr static Float max() { return +Constants::inf(); }
    constexpr static Float step() { return 1; }
};
template<> struct Traits<Double> {
    typedef Double ArithmeticType;
    constexpr static Double min() { return -Constantsd::inf(); }
    constexpr static Double max() { return +Constantsd::inf(); }
    constexpr static Double step() { return 1; }
};

template<class T, class Base> struct CastingTraits {
    constexpr static T min() { return T(Traits<Base>::min()); }
    constexpr static T max() { return T(Traits<Base>::max()); }
    constexpr static T step() { return T(Traits<Base>::step()); }
};
template<> struct Traits<Deg>: CastingTraits<Deg, Float> {
    typedef Float ArithmeticType;
};
template<> struct Traits<Rad>: CastingTraits<Rad, Float> {
    typedef Float ArithmeticType;
};
template<> struct Traits<Seconds>: CastingTraits<Seconds, Float> {
    typedef Float ArithmeticType;
};
template<> struct Traits<Nanoseconds>: CastingTraits<Nanoseconds, Long> {
    typedef Long ArithmeticType;
};
template<> struct Traits<Half>: CastingTraits<Half, Float> {
    typedef Float ArithmeticType;
};

template<class T> struct Data {
    Flags flags;
    /* 0/1/3/7 or 4/1/3/7 bytes free for a 1/2/4/8-byte type */
    T min = Traits<T>::min();
    T max = Traits<T>::max();
    T step = Traits<T>::step();
    /* For owned data, T[] is then right after, tightly packed, with array
       length matching the (3D) storage size. For non-owned data the derived
       DataNonOwned<dimensions> struct is used instead. */

    /* For non-owned data DataNonOwned::pointer should be used instead */
    T* pointer() {
        CORRADE_INTERNAL_DEBUG_ASSERT(!(flags >= Flag::NonOwned));
        return reinterpret_cast<T*>(this + 1);
    }

    /* The stride isn't depending on anything in the storage itself, apart from
       the assertion. For non-owned data DataNonOwned::stride() should be
       called instead. */
    Containers::Stride3D stride(const Containers::Size3D& size) const {
        CORRADE_INTERNAL_DEBUG_ASSERT(!(flags >= Flag::NonOwned));
        return {std::ptrdiff_t(sizeof(T)*size[2]*size[1]),
                std::ptrdiff_t(sizeof(T)*size[2]),
                std::ptrdiff_t(sizeof(T))};
    }

    /* For non-owned data DataNonOwned::data() should be called instead */
    T& data(const Containers::Size3D& size, const Containers::Size3D& index) {
        CORRADE_INTERNAL_DEBUG_ASSERT(!(flags >= Flag::NonOwned));
        T* pointer = reinterpret_cast<T*>(this + 1);
        std::size_t nextDimensionSize = 1;
        for(UnsignedInt i = 3; i != 0; --i) {
            /* Casting to std::ptrdiff_t to avoid cursed issues like in
               StridedArrayView itself, where it sometimes led to overflows due
               to a wrong result type picked. See StridedElement::get() there
               for details. */
            pointer += std::ptrdiff_t(index[i - 1])*nextDimensionSize;
            nextDimensionSize *= size[i - 1];
        }
        return *pointer;
    }
};

template<class T> struct DataNonOwned: Data<T> {
    void* pointer;
    /* The stride is stored only for the actual data dimensions (so 0, 1, 2 or
       3 components) in order to fit in-place in as many cases as possible.
       The count of stored components is denoted by presence of NonOwned1D and
       NonOwned2D in flags, neither set means 0, both set means 3. */

    Containers::Stride3D stride(const Containers::Size3D& size) const {
        /* Check how many stride dimensions we actually store */
        UnsignedInt strideDimensions = 0;
        if(Data<T>::flags >= Flag::NonOwned1D)
            strideDimensions += 1;
        if(Data<T>::flags >= Flag::NonOwned2D)
            strideDimensions += 2;

        /* Copy the stored stride suffix */
        Containers::Stride3D out;
        const std::ptrdiff_t* strideData = reinterpret_cast<const std::ptrdiff_t*>(this + 1);
        for(UnsignedInt i = 0; i != strideDimensions; ++i)
            out[3 - strideDimensions + i] = strideData[i];

        /* For the rest the stride is stride of the next dimension times size
           in that dimension, or size of the type, if there's no next
           dimension. Again casting the size to std::ptrdiff_t to avoid cursed
           issues like in StridedArrayView itself, where it sometimes led to
           overflows due to a wrong result type picked. See
           StridedElement::get() there for details. */
        const std::ptrdiff_t stride = strideDimensions ?
            std::ptrdiff_t(size[3 - strideDimensions])*strideData[0] : sizeof(T);
        for(UnsignedInt i = 0; i != 3 - strideDimensions; ++i) {
            /* Size in the extra dimensions should be 1 */
            CORRADE_INTERNAL_ASSERT(size[i] == 1);
            out[i] = stride;
        }

        return out;
    }

    T& data(const Containers::Size3D& size, const Containers::Size3D& index) {
        const Containers::Stride3D stride = this->stride(size);
        char* pointer = static_cast<char*>(this->pointer);
        for(UnsignedInt i = 0; i != 3; ++i)
            /* Casting to std::ptrdiff_t to avoid cursed issues like in
               StridedArrayView itself, where it sometimes led to overflows due
               to a wrong result type picked. See StridedElement::get() there
               for details. */
            pointer += std::ptrdiff_t(index[i])*stride[i];
        return *reinterpret_cast<T*>(pointer);
    }
};

}

template<class T> Containers::ArrayView<T> NumericStorage<T>::createNoInitInternal() {
    const Containers::Size3D& size = this->size();
    const std::size_t count = size[0]*size[1]*size[2];
    const std::size_t dataSize = sizeof(Data<T>) + sizeof(T)*count;

    /* Even with 8-byte types the Data<T> struct alone fits in-place, however
       the actual data storage after doesn't. So this branch is never entered
       in that case but as we don't do createInPlace<Data<T>>() we don't need
       to special-case that at compile time. */
    char* const storage = dataSize <= MaxInPlaceSize ?
        createInPlace<char>() :
        createAllocated(new char[dataSize], 1, [](void* data, std::size_t) {
            delete[] static_cast<char*>(data);
        });

    /* Construct the Data struct in-place to initialize its members */
    Data<T>* data = new(storage) Data<T>;
    return {data->pointer(), count};
}

template<class T> void NumericStorage<T>::create(NoInitT) {
    createNoInitInternal();
}

template<class T> void NumericStorage<T>::create(ValueInitT) {
    /** @todo Utility::fill() for this once it exists */
    for(T& i: createNoInitInternal())
        i = T{};
}

template<class T> void NumericStorage<T>::create(DirectInitT, const T& value) {
    /** @todo Utility::fill() for this once it exists */
    for(T& i: createNoInitInternal())
        i = value;
}

template<class T> void NumericStorage<T>::createNonOwnedInternal(const void* const pointer, const bool immutable, const std::ptrdiff_t* const stride, const UnsignedInt dimensions) {
    const std::size_t dataSize = sizeof(DataNonOwned<T>) + sizeof(std::ptrdiff_t)*dimensions;

    /* Unlike in createNoInitInternal() above, with 8-byte types not even the
       DataNonOwned<T> struct alone fits in-place, but again as we don't do
       createInPlace<DataNonOwned<T>>() we don't need to special-case that at
       compile time. */
    char* const storage = dataSize <= MaxInPlaceSize ?
        createInPlace<char>() :
        createAllocated(new char[dataSize], 1, [](void* data, std::size_t) {
            delete[] static_cast<char*>(data);
        });

    /* Construct the Data struct in-place to initialize its members */
    DataNonOwned<T>* data = new(storage) DataNonOwned<T>;
    data->flags |= Flag::NonOwned;
    if(dimensions >= 2)
        data->flags |= Flag::NonOwned2D;
    if(dimensions == 1 || dimensions == 3)
        data->flags |= Flag::NonOwned1D;
    if(immutable)
        data->flags |= Flag::NonOwnedImmutable;
    data->pointer = const_cast<T*>(static_cast<const T*>(pointer));

    /* Stride is only expected to be nullptr with a single-item storage */
    CORRADE_INTERNAL_ASSERT(!dimensions || stride);
    std::ptrdiff_t* strideStorage = reinterpret_cast<std::ptrdiff_t*>(data + 1);
    for(UnsignedInt i = 0; i != dimensions; ++i)
        strideStorage[i] = stride[i];
}

template<class T> template<UnsignedInt dimensions> void NumericStorage<T>::create(NonOwnedT, const Containers::StridedArrayView<dimensions, T>& values) {
    createNonOwnedInternal(values.data(), false, Containers::Stride<dimensions>{values.stride()}.begin(), dimensions);
}

template<class T> template<UnsignedInt dimensions> void NumericStorage<T>::create(NonOwnedT, const Containers::StridedArrayView<dimensions, const T>& values) {
    createNonOwnedInternal(values.data(), true, Containers::Stride<dimensions>{values.stride()}.begin(), dimensions);
}

template<class T> Containers::Stride3D NumericStorage<T>::stride() const {
    const Data<T>& data = *AbstractStorage::data<Data<T>>();
    if(data.flags >= Flag::NonOwned)
        return static_cast<const DataNonOwned<T>&>(data).stride(size());
    return data.stride(size());
}

template<class T> Containers::Pair<T, T> NumericStorage<T>::range() const {
    const Data<T>& data = *AbstractStorage::data<Data<T>>();
    return {data.min, data.max};
}

template<class T> const NumericStorage<T>& NumericStorage<T>::setRange(const T min, const T max) const {
    /* This expression also handles the case where one or both of the values
       are NaN -- it'll fail for them too. Having min the same as max is
       allowed. Explicit cast because Half types aren't comparable. */
    CORRADE_ASSERT(static_cast<typename Traits<T>::ArithmeticType>(min) <= static_cast<typename Traits<T>::ArithmeticType>(max),
        "Ui::NumericStorage::setRange(): expected min to not be larger than max but got" << min << "and" << max, *this);

    /* If the range is different from before, mark the storage as dirty */
    Data<T>& data = *AbstractStorage::data<Data<T>>();
    if(data.min != min || data.max != max) {
        data.min = min;
        data.max = max;
        setDirty();
    }

    return *this;
}

template<class T> T NumericStorage<T>::step() const {
    return AbstractStorage::data<Data<T>>()->step;
}

template<class T> const NumericStorage<T>& NumericStorage<T>::setStep(const T step) const {
    /* Explicit cast to an arithmetic type to catch also negative Half zero */
    CORRADE_ASSERT(static_cast<typename Traits<T>::ArithmeticType>(step) != typename Traits<T>::ArithmeticType{},
        "Ui::NumericStorage::setStep(): expected a non-zero step", *this);

    /* Not calling setDirty() in this case as the step doesn't affect the
       stored value or its range */
    AbstractStorage::data<Data<T>>()->step = step;

    return *this;
}

template<class T> typename NumericStorage<T>::Type NumericStorage<T>::query(const NumericStorage<T>& storage, const Containers::Size3D& index, const StorageOperation operation) {
    /* Almost a Rust-level code with the **.::<>() */
    Data<T>& data = *storage.AbstractStorage::data<Data<T>>();
    /* Explicit casting because Half isn't implicitly convertible to Float */
    if(operation == StorageOperation::Min)
        return static_cast<typename NumericStorage<T>::Type>(data.min);
    if(operation == StorageOperation::Max)
        return static_cast<typename NumericStorage<T>::Type>(data.max);
    CORRADE_INTERNAL_ASSERT(operation == StorageOperation{});

    if(data.flags >= Flag::NonOwned)
        return static_cast<typename NumericStorage<T>::Type>(static_cast<DataNonOwned<T>&>(data).data(storage.size(), index));
    return static_cast<typename NumericStorage<T>::Type>(data.data(storage.size(), index));
}

namespace {

/* This isn't inlined directly in the NumericStorage::update() partially
   because Half doesn't implement any arithmetic or comparison (and thus we'd
   have to add casts to every expression) and because restricting to just the
   builtin types will avoid having the same code compiled for e.g. Float, Deg,
   Rad, Seconds and Half when just a Float variant would be enough. Similarly
   with the static_assert() below we disallow any types that are less than 32
   bits, as there's no advantage using those. and it further reduces
   duplications in the binary. */
template<class T> T updaterImplementation(const T min, const T max, const T step, const T currentValue, const StorageOperation operation, const T value, StorageUpdateState& state) {
    static_assert(std::is_arithmetic<T>::value && sizeof(T) >= 4,
        "this helper should be called with builtin types only");

    /* Set can return a non-Success state */
    if(operation == StorageOperation::Set) {
        CORRADE_INTERNAL_ASSERT(value);
        if(value < min) {
            state = StorageUpdateState::Clamped;
            return min;
        }
        if(value > max) {
            state = StorageUpdateState::Clamped;
            return max;
        }
        state = StorageUpdateState::Success;
        return value;
    }

    /* All other operations are required to succeed, including cases where
       increment gets clamped and such */
    state = StorageUpdateState::Success;
    /* The following has to clamp() and not just min() / max() so they work
       properly also with negative step. See the "custom negative step" test
       cases. */
    if(operation == StorageOperation::Increment) {
        /* Handle potential integer overflow, basically so that incrementing
           a large value with a large step doesn't wrap around and result in
           something that the clamp() doesn't catch. While
            if(step > 0 && currentValue + step < currentValue)
           would work for unsigned types, for signed types it's UB, which means
           the compiler discards the condition as being always false. Instead
           we have to rewrite the conditions to have behavior well defined. See
           https://stackoverflow.com/a/1514309 for details. In case of floats
           there's no wraparound but rather implicit overflow to infinity, so
           both of these conditions always evaluate to false there. See the
           rangeStepTypeOverflow*() tests for thorough tests of all cases. */
        if(step > 0 && currentValue > Traits<T>::max() - step)
            return max;
        if(step < 0 && currentValue < Traits<T>::min() - step)
            return min;
        return Math::clamp(currentValue + step, min, max);
    }
    if(operation == StorageOperation::Decrement) {
        /* Same as above, just dealing with subtraction instead of addition */
        if(step > 0 && currentValue < Traits<T>::min() + step)
            return min;
        if(step < 0 && currentValue > Traits<T>::max() + step)
            return max;
        return Math::clamp(currentValue - step, min, max);
    }
    if(operation == StorageOperation::Min)
        return min;
    if(operation == StorageOperation::Max)
        return max;

    CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
}

}

template<class T> StorageUpdateState NumericStorage<T>::updater(const NumericStorage<T>& storage, const Containers::Size3D& index, const StorageOperation operation, const Type* const value) {
    Data<T>& data = *storage.AbstractStorage::data<Data<T>>();
    StorageUpdateState state;

    /* Get a reference to the current value */
    T& currentValueReference = data.flags >= Flag::NonOwned ?
        static_cast<DataNonOwned<T>&>(data).data(storage.size(), index) :
        data.data(storage.size(), index);

    /* Call the update implementation with all data cast to the corresponding
       arithmetic type */
    const typename Traits<T>::ArithmeticType currentValue = static_cast<typename Traits<T>::ArithmeticType>(currentValueReference);
    const typename Traits<T>::ArithmeticType valueToUpdate = updaterImplementation(
        static_cast<typename Traits<T>::ArithmeticType>(data.min),
        static_cast<typename Traits<T>::ArithmeticType>(data.max),
        static_cast<typename Traits<T>::ArithmeticType>(data.step),
        currentValue,
        operation,
        value ? static_cast<typename Traits<T>::ArithmeticType>(*value) : typename Traits<T>::ArithmeticType{},
        state);

    /* Update the value and mark the storage as dirty only if it actually
       changed */
    if(valueToUpdate != currentValue) {
        currentValueReference = T(valueToUpdate);
        storage.setDirty();
    }

    return state;
}

template<class T> StorageOperations NumericStorage<T>::operations() const {
    return AbstractStorage::data<Data<T>>()->flags >= Flag::NonOwnedImmutable ?
        StorageOperation::Min|StorageOperation::Max :
        StorageOperation::Set|StorageOperation::Increment|StorageOperation::Decrement|StorageOperation::Min|StorageOperation::Max;
}

template<class T> Containers::StridedArrayView3D<const T> NumericStorage<T>::data() const {
    Data<T>& data = *AbstractStorage::data<Data<T>>();

    /* We're sure the memory is correctly sized so fake the ArrayView size */
    if(data.flags >= Flag::NonOwned) {
        DataNonOwned<T>& dataNonOwned = static_cast<DataNonOwned<T>&>(data);
        return {{static_cast<T*>(dataNonOwned.pointer), ~std::size_t{}}, size(), dataNonOwned.stride(size())};
    }
    return {{data.pointer(), ~std::size_t{}}, size(), data.stride(size())};
}

/* Explicitly instantiating just the functions that actually have to be
   deinlined in this file to avoid inflating the binary with the full
   combinatorial explosion of constructor and operator[]() variants of which
   always only a small subset gets used by a cncrete application. In comparison
    template class MAGNUM_UI_EXPORT NumericStorage<type>;
   resulted in the NumericStorage.cpp.o file being 3.7 MB instead of 1.2 MB
   in a GCC Debug build. Making operator[]() and value() all delegate to the 3D
   variant (and then reimplementing the assertions that would otherwise be
   handled by StorageQuery) made the file "only" 2.5 MB, but that needlessly
   duplicates a lot of checks and is still significantly larger. */
#define _c(type)                                                            \
template MAGNUM_UI_EXPORT void NumericStorage<type>::create(NoInitT);       \
template MAGNUM_UI_EXPORT void NumericStorage<type>::create(ValueInitT);    \
template MAGNUM_UI_EXPORT void NumericStorage<type>::create(DirectInitT, const type&); \
template MAGNUM_UI_EXPORT void NumericStorage<type>::createNonOwnedInternal(const void*, bool, const std::ptrdiff_t*, UnsignedInt); \
template MAGNUM_UI_EXPORT void NumericStorage<type>::create<1>(NonOwnedT, const Containers::StridedArrayView1D<type>&); \
template MAGNUM_UI_EXPORT void NumericStorage<type>::create<1>(NonOwnedT, const Containers::StridedArrayView1D<const type>&); \
template MAGNUM_UI_EXPORT void NumericStorage<type>::create<2>(NonOwnedT, const Containers::StridedArrayView2D<type>&); \
template MAGNUM_UI_EXPORT void NumericStorage<type>::create<2>(NonOwnedT, const Containers::StridedArrayView2D<const type>&); \
template MAGNUM_UI_EXPORT void NumericStorage<type>::create<3>(NonOwnedT, const Containers::StridedArrayView3D<type>&); \
template MAGNUM_UI_EXPORT void NumericStorage<type>::create<3>(NonOwnedT, const Containers::StridedArrayView3D<const type>&); \
template MAGNUM_UI_EXPORT Containers::Stride3D NumericStorage<type>::stride() const; \
template MAGNUM_UI_EXPORT Containers::Pair<type, type> NumericStorage<type>::range() const; \
template MAGNUM_UI_EXPORT const NumericStorage<type>& NumericStorage<type>::setRange(type, type) const; \
template MAGNUM_UI_EXPORT type NumericStorage<type>::step() const;          \
template MAGNUM_UI_EXPORT const NumericStorage<type>& NumericStorage<type>::setStep(type) const; \
template MAGNUM_UI_EXPORT typename NumericStorage<type>::Type NumericStorage<type>::query(const NumericStorage<type>&, const Containers::Size3D&, StorageOperation); \
template MAGNUM_UI_EXPORT StorageUpdateState NumericStorage<type>::updater(const NumericStorage<type>&, const Containers::Size3D&, StorageOperation, const Type*); \
template MAGNUM_UI_EXPORT StorageOperations NumericStorage<type>::operations() const; \
template MAGNUM_UI_EXPORT Containers::StridedArrayView3D<const type> NumericStorage<type>::data() const;
_c(UnsignedByte)
_c(Byte)
_c(UnsignedShort)
_c(Short)
_c(UnsignedInt)
_c(Int)
_c(UnsignedLong)
_c(Long)
_c(Float)
_c(Double)
_c(Half)
_c(Deg)
_c(Rad)
_c(Seconds)
_c(Nanoseconds)
#undef _c

}}
