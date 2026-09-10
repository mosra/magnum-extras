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

#include "BitStorage.h"

#include <Corrade/Containers/StridedBitArrayView.h>

namespace Magnum { namespace Ui {

namespace {

/* Not used as actual flags, just helper constants */
enum {
    OwnedOffset = 2,
    NonOwnedOffsetShift = 5,
    NonOwnedOffsetMask = 7 << NonOwnedOffsetShift,
};

/* In case of owned data, only the first two bits are used, the rest is used
   for storage already. With NonOwned, the NonOwnedImmutable bit is used as
   well, and the remaining bits store the offset. */
enum: UnsignedByte {
    FlagNone = 0,
    FlagDefaultTrue = 1 << 0, /* If not set, default is false */
    FlagNonOwned = 1 << 1,
    FlagNonOwnedImmutable = FlagNonOwned|(1 << 2),
};

struct Data {
    explicit Data(UnsignedByte flags, UnsignedByte offset = 0): offsetFlags{UnsignedByte((offset << NonOwnedOffsetShift)|flags)} {}

    UnsignedByte offsetFlags;
    /* For owned data, the bit storage is then right after, tightly packed. For
       non-owned data the derived DataNonOwned struct is used instead. */

    /* For non-owned data DataNonOwned::view() should be used instead */
    Containers::MutableBitArrayView view(const std::size_t size) {
        CORRADE_INTERNAL_DEBUG_ASSERT(!(offsetFlags & FlagNonOwned));
        return {this, OwnedOffset, size};
    }
};

struct DataNonOwned: Data {
    explicit DataNonOwned(UnsignedByte flags, const void* pointer, UnsignedByte offset, std::ptrdiff_t stride): Data{flags, offset}, pointer{pointer}, stride{stride} {}

    /* 7 bytes free before */
    const void* pointer;
    std::ptrdiff_t stride;

    Containers::MutableStridedBitArrayView1D view(const std::size_t size) {
        /* We're sure the view is correctly sized so fake the size; excluding
           the three bits which get used to store the offset */
        return {{const_cast<void*>(pointer), std::size_t(offsetFlags & NonOwnedOffsetMask) >> NonOwnedOffsetShift, ~std::size_t{} >> 3}, size, stride};
    }
};

}

namespace Implementation {

std::size_t bitStorageViewSize(const Containers::StridedBitArrayView1D& view) {
    return view.size();
}
std::size_t bitStorageViewSize(const Containers::MutableStridedBitArrayView1D& view) {
    return view.size();
}

}

Containers::MutableBitArrayView BitStorage::createNoInitInternal(bool defaultValue) {
    const std::size_t bitCount = size();
    #ifdef CORRADE_TARGET_32BIT
    /* The size can be at most what (Strided)BitArrayView can fit, as we use it
       to do the actual bit addressing etc. It makes little sense to check the
       size constraint on 64-bit, if 64-bit code happens to go over then it's
       got bigger problems than this assert. IOW, the assertion logic and
       message is the same as in Containers::BitArrayView. */
    CORRADE_ASSERT(bitCount < std::size_t{1} << (sizeof(std::size_t)*8 - 3),
        "Ui::BitStorage: size expected to be smaller than 2^" << Utility::Debug::nospace << (sizeof(std::size_t)*8 - 3) << "bits, got" << bitCount, {});
    #endif
    /* The Data stores all other state in additional OwnedOffset bits, add them
       to the total bit count and then round up to whole bytes */
    const std::size_t dataSize = (bitCount + OwnedOffset + 7)/8;

    char* const storage = dataSize <= MaxInPlaceSize ?
        createInPlace<char>() :
        createAllocated(new char[dataSize], 1, [](void* data, std::size_t) {
            delete[] static_cast<char*>(data);
        });

    /* Construct the Data struct in-place to initialize its members */
    Data* data = new(storage) Data{defaultValue ? FlagDefaultTrue : FlagNone};
    return {data, OwnedOffset, bitCount};
}

void BitStorage::create(NoInitT) {
    createNoInitInternal(false);
}

void BitStorage::create(ValueInitT) {
    createNoInitInternal(false).resetAll();
}

void BitStorage::create(DirectInitT, const bool value) {
    createNoInitInternal(false).setAll(value);
}

void BitStorage::createNonOwnedInternal(const void* const pointer, const bool immutable, const UnsignedByte offset, const std::ptrdiff_t stride) {
    new(createInPlace<DataNonOwned>()) DataNonOwned{immutable ? FlagNonOwnedImmutable : FlagNonOwned, pointer, offset, stride};
}

void BitStorage::create(NonOwnedT, const Containers::MutableStridedBitArrayView1D& values) {
    createNonOwnedInternal(values.data(), false, values.offset(), values.stride());
}

void BitStorage::create(NonOwnedT, const Containers::StridedBitArrayView1D& values) {
    createNonOwnedInternal(values.data(), true, values.offset(), values.stride());
}

bool BitStorage::isMutable() const {
    /* If FlagNonOwned isn't set, the FlagNonOwnedImmutable bit position is
       already used for storage, so have to check that both bits are set */
    return (AbstractStorage::data<Data>()->offsetFlags & FlagNonOwnedImmutable) != FlagNonOwnedImmutable;
}

bool BitStorage::defaultValue() const {
    return AbstractStorage::data<Data>()->offsetFlags & FlagDefaultTrue;
}

const BitStorage& BitStorage::setDefaultValue(const bool value) const {
    Data& data = *AbstractStorage::data<Data>();
    if(value)
        data.offsetFlags |= FlagDefaultTrue;
    else
        data.offsetFlags &= ~FlagDefaultTrue;
    return *this;
}

bool BitStorage::query(const std::size_t index, const StorageOperation operation) const {
    CORRADE_INTERNAL_DEBUG_ASSERT(operation == StorageOperation{});
    #ifdef CORRADE_NO_DEBUG_ASSERT
    static_cast<void>(operation);
    #endif
    Data& data = *AbstractStorage::data<Data>();

    if(data.offsetFlags & FlagNonOwned)
        return static_cast<DataNonOwned&>(data).view(size())[index];
    return data.view(size())[index];
}

StorageUpdateState BitStorage::updater(const std::size_t index, const StorageOperation operation, const bool* value) const {
    Data& data = *AbstractStorage::data<Data>();
    const Containers::MutableStridedBitArrayView1D view =
        data.offsetFlags & FlagNonOwned ?
            static_cast<DataNonOwned&>(data).view(size()) : data.view(size());
    const bool currentValue = view[index];

    bool valueToUpdate;
    if(operation == StorageOperation::Set)
        valueToUpdate = *value;
    else if(operation == StorageOperation::Reset)
        valueToUpdate = data.offsetFlags & FlagDefaultTrue;
    else if(operation == StorageOperation::Toggle)
        valueToUpdate = !currentValue;
    else CORRADE_INTERNAL_DEBUG_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */

    /* Update the value and mark the storage as dirty only if it actually
       changed */
    if(valueToUpdate != currentValue) {
        view.set(index, valueToUpdate);
        setDirty();
    }

    return StorageUpdateState::Success;
}

StorageQuery<bool> BitStorage::value() const {
    const auto query = [](const BitStorage& storage, const StorageOperation operation) {
        return storage.query(0, operation);
    };
    const auto updater = [](const BitStorage& storage, const StorageOperation operation, const bool* value) {
        return storage.updater(0, operation, value);
    };
    /* If the storage is immutable, the query has no updater. If FlagNonOwned
       isn't set, the FlagNonOwnedImmutable bit position is already used for
       storage, so have to check that both bits are set. */
    if((AbstractStorage::data<Data>()->offsetFlags & FlagNonOwnedImmutable) == FlagNonOwnedImmutable)
        return {*this, {}, query};
    return {*this, StorageOperation::Set|StorageOperation::Reset|StorageOperation::Toggle, query, updater};
}

StorageQuery<bool> BitStorage::operator[](const std::size_t index) const {
    const auto query = [](const BitStorage& storage, const std::size_t index, const StorageOperation operation) {
        return storage.query(index, operation);
    };
    const auto updater = [](const BitStorage& storage, const std::size_t index, const StorageOperation operation, const bool* value) {
        return storage.updater(index, operation, value);
    };
    /* If the storage is immutable, the query has no updater. If FlagNonOwned
       isn't set, the FlagNonOwnedImmutable bit position is already used for
       storage, so have to check both bits. */
    if((AbstractStorage::data<Data>()->offsetFlags & FlagNonOwnedImmutable) == FlagNonOwnedImmutable)
        return {*this, index, {}, query};
    return {*this, index, StorageOperation::Set|StorageOperation::Reset|StorageOperation::Toggle, query, updater};
}

Containers::StridedBitArrayView1D BitStorage::data() const {
    Data& data = *AbstractStorage::data<Data>();
    if(data.offsetFlags & FlagNonOwned)
        return static_cast<DataNonOwned&>(data).view(size());
    return data.view(size());
}

Containers::MutableStridedBitArrayView1D BitStorage::mutableData() const {
    Data& data = *AbstractStorage::data<Data>();
    /* If FlagNonOwned isn't set, the FlagNonOwnedImmutable bit position is
       already used for storage, so have to check both bits */
    CORRADE_ASSERT((data.offsetFlags & FlagNonOwnedImmutable) != FlagNonOwnedImmutable,
        "Ui::BitStorage::mutableData(): data not mutable", {});
    /* The internals return a mutable view for simplicity so this is just a
       copy of what data() does */
    if(data.offsetFlags & FlagNonOwned)
        return static_cast<DataNonOwned&>(data).view(size());
    return data.view(size());
}

}}
