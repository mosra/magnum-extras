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

#include "DataLayer.h"

#include <Corrade/Containers/BitArrayView.h>
#include <Corrade/Containers/EnumSet.hpp>
#include <Corrade/Containers/GrowableArray.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Magnum/Math/Vector3.h>

#include "Magnum/Ui/Handle.h"

namespace Magnum { namespace Ui {

Debug& operator<<(Debug& debug, const DataLayerStorageHandle value) {
    const bool packed = debug.immediateFlags() >= Debug::Flag::Packed;

    if(value == DataLayerStorageHandle::Null)
        return debug << (packed ? "Null" : "Ui::DataLayerStorageHandle::Null");

    /* ID extraction is copy of dataLayerStorageHandleId() because it asserts
       if the generation is 0, and the assert calls into this debug printer,
       leading to infinite recursion */
    return debug << (packed ? "{" : "Ui::DataLayerStorageHandle(") << Debug::nospace << Debug::hex << (UnsignedInt(value) & ((1 << Implementation::DataLayerStorageHandleIdBits) - 1)) << Debug::nospace << "," << Debug::hex << dataLayerStorageHandleGeneration(value) << Debug::nospace << (packed ? "}" : ")");
}

Debug& operator<<(Debug& debug, const StorageHandle value) {
    const bool packed = debug.immediateFlags() >= Debug::Flag::Packed;

    if(value == StorageHandle::Null)
        return debug << (packed ? "Null" : "Ui::StorageHandle::Null");

    debug << (packed ? "{" : "Ui::StorageHandle(") << Debug::nospace;
    if(storageHandleLayer(value) == LayerHandle::Null)
        debug << "Null,";
    else
        /* ID extraction is copy of storageHandleLayerId() because it asserts
           if the generation is 0, and the assert calls into this debug
           printer, leading to infinite recursion */
        debug << "{" << Debug::nospace << Debug::hex << ((UnsignedLong(value) >> (Implementation::DataLayerStorageHandleIdBits + Implementation::DataLayerStorageHandleGenerationBits)) & ((1 << Implementation::LayerHandleIdBits) - 1)) << Debug::nospace << "," << Debug::hex << storageHandleLayerGeneration(value) << Debug::nospace << "},";

    if(storageHandleStorage(value) == DataLayerStorageHandle::Null)
        debug << (packed ? "Null}" : "Null)");
    else
        /* ID extraction is copy of storageHandleId() because it asserts if the
           generation is 0, and the assert calls into this debug printer,
           leading to infinite recursion */
        debug << "{" << Debug::nospace << Debug::hex << (UnsignedLong(value) & ((1 << Implementation::DataLayerStorageHandleIdBits) - 1)) << Debug::nospace << "," << Debug::hex << storageHandleGeneration(value) << Debug::nospace << (packed ? "}}" : "})");

    return debug;
}

Debug& operator<<(Debug& debug, const StorageFlag value) {
    const bool packed = debug.immediateFlags() >= Debug::Flag::Packed;

    if(!packed)
        debug << "Ui::StorageFlag" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case StorageFlag::value: return debug << (packed ? "" : "::") << Debug::nospace << #value;
        _c(ReferenceCounted)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << (packed ? "" : "(") << Debug::nospace << Debug::hex << UnsignedByte(value) << Debug::nospace << (packed ? "" : ")");
}

Debug& operator<<(Debug& debug, const StorageFlags value) {
    return Containers::enumSetDebugOutput(debug, value, debug.immediateFlags() >= Debug::Flag::Packed ? "{}" : "Ui::StorageFlags{}", {
        StorageFlag::ReferenceCounted,
    });
}

Debug& operator<<(Debug& debug, const StorageOperation value) {
    debug << "Ui::StorageOperation" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case StorageOperation::value: return debug << "::" #value;
        _c(Set)
        _c(Reset)
        _c(Toggle)
        _c(Increment)
        _c(Decrement)
        _c(Min)
        _c(Max)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "(" << Debug::nospace << Debug::hex << UnsignedShort(value) << Debug::nospace << ")";
}

Debug& operator<<(Debug& debug, const StorageOperations value) {
    return Containers::enumSetDebugOutput(debug, value, "Ui::StorageOperations{}", {
        StorageOperation::Set,
        StorageOperation::Reset,
        StorageOperation::Toggle,
        StorageOperation::Increment,
        StorageOperation::Decrement,
        StorageOperation::Min,
        StorageOperation::Max,
    });
}

Debug& operator<<(Debug& debug, const StorageUpdateState value) {
    debug << "Ui::StorageUpdateState" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case StorageUpdateState::value: return debug << "::" #value;
        _c(Success)
        _c(Approximated)
        _c(Clamped)
        _c(Failed)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "(" << Debug::nospace << Debug::hex << UnsignedByte(value) << Debug::nospace << ")";
}

namespace {

/* Abusing last two bits of the StorageFlag enum to prevent clashing with newly
   added flags */
constexpr StorageFlag StorageFlagAllocated = StorageFlag(1 << (sizeof(StorageFlag)*8 - 1));
constexpr StorageFlag StorageFlagDirty = StorageFlag(1 << (sizeof(StorageFlag)*8 - 2));
constexpr StorageFlags StorageFlagMask = ~(StorageFlagAllocated|StorageFlagDirty);

union StorageData {
    explicit StorageData() noexcept: used{} {}

    /* There is *deliberately* no move constructor and destructor for managing
       the deleter for allocated storage because:
        - the array only ever grows capacity at the end, using arrayAppend(),
          which never causes any existing allocated storage to be deleted
        - if it'd be a RAII type and arrayAppend() would cause a reallocation,
          then it'd be sequence of move constructions from the original
          elements (which would clear the original element Allocated flags)
          followed by deleter calls on all original elements (which would be a
          no-op as the Allocated flags are not there), so ultimately doing
          nothing useful over a memcpy(), just with more overhead
        - storage removal cannot just replace the instance with a fresh one
          (and thus call move assignment / destructor) because the generation
          counter needs to be preserved, so it can just call the deleter
          directly as well
        - given that the type is rather big on its own (and may grow further if
          it makes sense to store larger types in-place), being able to use
          realloc() for a trivial type instead of new + delete means fewer
          reallocations and memory copies as it can grow in-place */

    struct Used {
        /* Except for the generation these have to be re-filled every time a
           handle is recycled, so it doesn't make sense to initialize them to
           anything. */

        /* Together with index of this item in `storage` used for creating a
           DataLayerStorageHandle. Increased every time a handle reaches
           removeStorage(). Has to be initially non-zero to differentiate the
           first ever handle (with index 0) from DataLayerStorageHandle::Null.
           Once becomes `1 << DataLayerStorageHandleGenerationBits` the handle
           gets disabled. */
        UnsignedShort generation = 1;

        /* If contains StorageFlagAllocated, the data.allocated references an
           external allocation, otherwise everything is stored inside
           data.inPlace. If contains StorageFlagDirty, the data got updated
           since last doUpdate() call. StorageFlagDirty gets cleared after
           doUpdate() is called. */
        StorageFlags flags{NoInit};

        /* 1 byte free */

        /* Reference count to ensure storages are not removed before data that
           use them, also used by ReferenceCounted to remove the storage once
           all users are gone */
        UnsignedInt referenceCount;

        /* If all 0s, the storage is in the free list. Code tests just the
           first member tho, as storages are guaranteed to have non-zero size
           in all dimensions. */
        Containers::Size3D size{NoInit};

        union Data {
            struct Allocated {
                void* data;
                std::size_t dataSize;
                void(*deleter)(void*, std::size_t);
            } allocated;

            /* This field should be 8-byte aligned if everything packs well,
               see the static_assert below. Making it four pointers large
               allows to store a pointer + Stride3D for generic strided views
               without an allocation on both 32-bit and 64-bit platforms.
               However, on 32-bit platforms four pointers makes the StorageData
               36 bytes large, which doesn't align, so make it five pointers
               there instead. */
            /** @todo this could be even 16-byte aligned now, do that once the
                Array allocators guarantee that */
            #ifndef CORRADE_TARGET_32BIT
            char inPlace[4*sizeof(std::size_t)];
            #else
            char inPlace[5*sizeof(std::size_t)];
            #endif
        } data;
    } used;

    struct Free {
        /* The generation value has to be preserved in order to increment it
           next time it gets used */
        UnsignedShort generation;

        /* The flags are preserved so ~State() can check only those to see if
           the deleter needs to be called, without having to test for zero size
           as well. This is also taken advantage of by usedAllocatedCount(). */
        StorageFlags flags;

        /* Just to not have `next` alias `referenceCount`, to prevent strange
           interactions when referenceCount is errorneously checked for freed
           items */
        UnsignedInt:32;

        /* Also has to be preserved in order to know whether the storage is
           used or not even if it's in the free list */
        Containers::Size3D size;

        /* See State::firstFreeStorage for more information */
        UnsignedInt next;
    } free;
};

static_assert(sizeof(StorageData::Used::Data::inPlace) == Implementation::DataLayerStorageMaxInPlaceSize,
    "outdated Implementation::DataLayerStorageMaxInPlaceSize constant");
static_assert(sizeof(StorageData) % 8 == 0 && offsetof(StorageData::Used::Data, inPlace) % 8 == 0,
    "StorageData in-place storage not 8-byte aligned");

#ifndef CORRADE_NO_STD_IS_TRIVIALLY_TRAITS
static_assert(std::is_trivially_copyable<StorageData>::value,
    "StorageData not trivially copyable");
#endif
static_assert(
    offsetof(StorageData::Used, generation) == offsetof(StorageData::Free, generation) &&
    offsetof(StorageData::Used, flags) == offsetof(StorageData::Free, flags) &&
    offsetof(StorageData::Used, size) == offsetof(StorageData::Free, size),
    "StorageData::Used and Free layout not compatible");

enum: UnsignedInt {
    /* For Data::storageIdDirty */
    DataIsDirty = 1 << Implementation::DataLayerStorageHandleIdBits,
};

struct Data {
    /* First Implementation::DataLayerStorageHandleIdBits (20 bits) is storage
       index. Since the storage is reference-counted and thus not allowed to be
       removed while data reference it, we don't need to store the
       DataLayerStorageHandle generation.

       The following bit denotes whether the data is dirty, to force an update
       even if the storage itself isn't. Is set upon data creation and after
       changing data properties, such as by DataLayer::setIndex(), and
       explicitly with DataLayer::setDirty(). */
    UnsignedInt storageIdDirty;
    /* Operations supported by the updater function */
    StorageOperations operations;
    /* 2 bytes free */
    /* Linearized storage index (basically a linear data position as if the 3D
       storage would be contiguous). The logic is that with a byte-sized
       storage, we wouldn't be able to address more than 32/64 bits anyway, so
       there's no point in storing 3*4/8 bytes. The value is accessed through
       linearizeIndex() and delinearizeIndex(). */
    std::size_t linearizedIndex;

    Containers::FunctionData function;
    void(*call)(DataLayer&, DataLayerStorageHandle, const Containers::Size3D&, DataHandle, Containers::FunctionData&);
    /* Nullptr if the data binding is immutable */
    /** @todo to save space the layer could maintain a separate array
        containing just the updater pointers and have them referenced from here
        via a 20-byte index; and have a possibility for onUpdate() to say
        "don't need the updater at all" for labels and other immutable
        bindings ... but so far it looks like a lot of extra complication for
        minimal gains */
    StorageUpdateState(*updater)(DataLayer&, DataLayerStorageHandle, const Containers::Size3D&, StorageOperation, const void*);
};

}

struct DataLayer::State {
    ~State();

    Containers::Array<StorageData> storages;
    /* Indices in the storage array. The StorageData then has a free.next
       member containing the next free index. New storage gets taken from the
       front, removed is put at the end. A value of ~UnsignedInt{} means
       there's no (first/next/last) free storage. */
    UnsignedInt firstFreeStorage = ~UnsignedInt{};
    UnsignedInt lastFreeStorage = ~UnsignedInt{};

    Containers::Array<Data> data;
};

DataLayer::State::~State() {
    /* Call deleters for remaining allocated storages. Allocated storages that
       are already removed have the Allocated flag cleared. See the StorageData
       struct definition for why it doesn't have a destructor on its own. */
    for(const StorageData& storage: storages)
        if(storage.used.flags >= StorageFlagAllocated)
            storage.used.data.allocated.deleter(storage.used.data.allocated.data, storage.used.data.allocated.dataSize);
}

DataLayer::DataLayer(const LayerHandle handle): AbstractLayer{handle}, _state{InPlaceInit} {}

DataLayer::DataLayer(DataLayer&&) noexcept = default;

DataLayer::~DataLayer() = default;

DataLayer& DataLayer::operator=(DataLayer&&) noexcept = default;

std::size_t DataLayer::storageCapacity() const {
    return _state->storages.size();
}

std::size_t DataLayer::storageUsedCount() const {
    /* In general we can assume that the amount of free storage is always
       either zero or significantly less than the capacity, and thus iterating
       the (presumably small) free list should be faster, even though it
       involves jumping around in memory. */
    const State& state = *_state;
    std::size_t free = 0;
    UnsignedInt index = state.firstFreeStorage;
    while(index != ~UnsignedInt{}) {
        index = state.storages[index].free.next;
        ++free;
    }
    return state.storages.size() - free;
}

std::size_t DataLayer::storageUsedAllocatedCount() const {
    std::size_t count = 0;
    for(const StorageData& storage: _state->storages)
        /* The Allocated flag gets reset on removal to make it easier for
           ~State() to know which deleters to call, so we don't need to do
           anything extra here to check only non-removed items */
        if(storage.used.flags >= StorageFlagAllocated)
            ++count;

    return count;
}

bool DataLayer::isHandleValid(const DataLayerStorageHandle handle) const {
    /* layerDataHandleId() below expects generation to be non-zero, check for
       that first. This is also a superset of a check for
       LayerDataHandle::Null. */
    const UnsignedInt generation = dataLayerStorageHandleGeneration(handle);
    if(!generation)
        return false;
    const State& state = *_state;
    const UnsignedInt index = dataLayerStorageHandleId(handle);
    if(index >= state.storages.size())
        return false;
    const StorageData& storage = state.storages[index];
    /* Zero generation handles (i.e., where it wrapped around from all bits
       set) are expected to be expired and thus with `size` being all zero.
       The size is asserted to be non-zero when creating the storage, so we
       only need to check the first element (4/8 bytes) here, not all three
       (12/24 bytes). Checking all three in the debug assert for sanity. */
    CORRADE_INTERNAL_DEBUG_ASSERT(storage.used.generation || storage.used.size == Containers::Size3D{});
    return storage.used.size[0] && generation == storage.used.generation;
}

bool DataLayer::isHandleValid(const StorageHandle handle) const {
    return storageHandleLayer(handle) == this->handle() && isHandleValid(storageHandleStorage(handle));
}

DataLayerStorageHandle DataLayer::createStorage(const Containers::Size3D& size, const StorageFlags flags) {
    State& state = *_state;
    /* This function can only publicly be called from an AbstractStorage
       constructor, point there from the assert message to avoid confusion */
    /** @todo remove this restriction once storages can be resized (i.e., then
        it's valid to create an empty storage and gradually fill it, for
        example) */
    CORRADE_ASSERT(size[0] && size[1] && size[2],
        "Ui::AbstractStorage: expected non-zero size but got" << size, {});

    /* Find the first free storage if there is, update the free index to point
       to the next one (or none) */
    StorageData* storage;
    if(state.firstFreeStorage != ~UnsignedInt{}) {
        storage = &state.storages[state.firstFreeStorage];

        if(state.firstFreeStorage == state.lastFreeStorage) {
            CORRADE_INTERNAL_ASSERT(storage->free.next == ~UnsignedInt{});
            state.firstFreeStorage = state.lastFreeStorage = ~UnsignedInt{};
        } else {
            state.firstFreeStorage = storage->free.next;
        }

    /* If there isn't, allocate a new one */
    } else {
        /* This function can only publicly be called from an AbstractStorage
           constructor, point there from the assert message to avoid
           confusion */
        CORRADE_ASSERT(state.storages.size() < 1 << Implementation::DataLayerStorageHandleIdBits,
            "Ui::AbstractStorage: can only have at most" << (1 << Implementation::DataLayerStorageHandleIdBits) << "storages", {});
        storage = &arrayAppend(state.storages, InPlaceInit);
    }

    storage->used.size = size;
    /* StorageFlagDirty isn't set upon creation */
    storage->used.flags = flags;
    storage->used.referenceCount = 0;

    /* Set NeedsCommonDataUpdate if the storage is reference-counted, to remove
       it immediately at the next update if it stays unused */
    if(flags >= StorageFlag::ReferenceCounted)
        setNeedsUpdate(LayerState::NeedsCommonDataUpdate);

    return dataLayerStorageHandle(storage - state.storages.data(), storage->used.generation);
}

void* DataLayer::createStorageAllocated(const DataLayerStorageHandle handle, void* const data, const std::size_t dataSize, void(*const deleter)(void*, std::size_t)) {
    /* The handle should be valid because this should only be called from
       AbstractStorage subclass constructors where the handle is created right
       before, but just to be sure */
    CORRADE_INTERNAL_DEBUG_ASSERT(isHandleValid(handle));
    /* This function can only publicly be called from AbstractStorage, point
       there from the assert message to avoid confusion */
    CORRADE_ASSERT(data,
        "Ui::AbstractStorage::createAllocated(): data is null", {});
    CORRADE_ASSERT(deleter,
        "Ui::AbstractStorage::createAllocated(): deleter is null", {});

    StorageData* const storage = &_state->storages[dataLayerStorageHandleId(handle)];
    storage->used.flags |= StorageFlagAllocated;
    storage->used.data.allocated.data = data;
    storage->used.data.allocated.dataSize = dataSize;
    storage->used.data.allocated.deleter = deleter;
    return data;
}

auto DataLayer::createStorageInPlace(const DataLayerStorageHandle handle) -> char(&)[Implementation::DataLayerStorageMaxInPlaceSize] {
    /* The handle should be valid because this should only be called from
       AbstractStorage subclass constructors where the handle is created right
       before, but just to be sure */
    CORRADE_INTERNAL_DEBUG_ASSERT(isHandleValid(handle));
    /* storage->used.flags shouldn't contain StorageFlagAllocated by default.
       In other words, this function does nothing at all besides returning the
       view, so if the storage doesn't need to initialize the memory, it
       doesn't need to call this function. */
    StorageData* const storage = &_state->storages[dataLayerStorageHandleId(handle)];
    CORRADE_INTERNAL_DEBUG_ASSERT(!(storage->used.flags & StorageFlagAllocated));
    return storage->used.data.inPlace;
}

void* DataLayer::storageData(const DataLayerStorageHandle handle) {
    /* This function is only callable from AbstractStorage, point there from
       the assert message to avoid confusion */
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::AbstractStorage::data(): invalid handle" << handle, {});
    StorageData& storage = _state->storages[dataLayerStorageHandleId(handle)];
    return storage.used.flags >= StorageFlagAllocated ?
        storage.used.data.allocated.data :
        storage.used.data.inPlace;
}

void DataLayer::removeStorage(const StorageHandle handle) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::DataLayer::removeStorage(): invalid handle" << handle, );
    const UnsignedInt id = storageHandleId(handle);
    CORRADE_ASSERT(_state->storages[id].used.referenceCount == 0,
        "Ui::DataLayer::removeStorage():" << handle << "referenced by" << _state->storages[id].used.referenceCount << "data", );

    /* Doesn't delegate to remove(DataLayerStorageHandle) to avoid a double
       check; doesn't check just the layer portion of the handle and delegate
       to avoid a confusing assertion message if the data portion would be
       invalid */
    removeStorageInternal(id);
}

void DataLayer::removeStorage(const DataLayerStorageHandle handle) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::DataLayer::removeStorage(): invalid handle" << handle, );
    const UnsignedInt id = dataLayerStorageHandleId(handle);
    CORRADE_ASSERT(_state->storages[id].used.referenceCount == 0,
        "Ui::DataLayer::removeStorage():" << handle << "referenced by" << _state->storages[id].used.referenceCount << "data", );

    removeStorageInternal(id);
}

void DataLayer::removeStorageInternal(const UnsignedInt id) {
    State& state = *_state;
    StorageData& storage = state.storages[id];
    /* This should have been ensured by removeStorage() already, which has the
       check there in order to print the full handles. Removal of
       no-longer-referenced reference-counted storages from doPreUpdate() also
       doesn't need this function to do the same redundant check. */
    CORRADE_INTERNAL_DEBUG_ASSERT(storage.used.referenceCount == 0);

    /* Increase the storage generation so existing handles pointing to this
       storage are invalidated. Wrap around to 0 if it goes over the generation
       bits. Also mark it as not used by clearing the size so isHandleValid()
       doesn't return true if the generation matches by accident. */
    ++storage.used.generation &= (1 << Implementation::DataLayerStorageHandleGenerationBits) - 1;
    storage.used.size = {};

    /* If the storage is allocated, call the deleter. See the StorageData
       struct definition for why it doesn't have a destructor on its own. */
    if(storage.used.flags >= StorageFlagAllocated)
        storage.used.data.allocated.deleter(storage.used.data.allocated.data, storage.used.data.allocated.dataSize);

    /* Clear the flags so ~State() doesn't attempt to call a deleter for
       StorageFlagAllocated, or doPreUpdate() doesn't attempt to remove a
       StorageFlag::ReferenceCounted storage with zero referemces, etc. */
    storage.used.flags = {};

    /* Put the storage at the end of the free list (while they're allocated
       from the front) to not exhaust the generation counter too fast. If the
       free list is empty however, update also the index of the first free
       layer.

       Don't do this if the generation wrapped around. That makes it disabled,
       i.e. impossible to be recycled later, to avoid aliasing old handles. */
    if(storage.used.generation != 0) {
        storage.free.next = ~UnsignedInt{};
        if(state.lastFreeStorage == ~UnsignedInt{}) {
            CORRADE_INTERNAL_ASSERT(
                state.firstFreeStorage == ~UnsignedInt{} &&
                state.lastFreeStorage == ~UnsignedInt{});
            state.firstFreeStorage = id;
        } else {
            state.storages[state.lastFreeStorage].free.next = id;
        }
        state.lastFreeStorage = id;
    }
}

bool DataLayer::isStorageAllocated(const StorageHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::DataLayer::isStorageAllocated(): invalid handle" << handle, {});
    return _state->storages[storageHandleId(handle)].used.flags >= StorageFlagAllocated;
}

bool DataLayer::isStorageAllocated(const DataLayerStorageHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::DataLayer::isStorageAllocated(): invalid handle" << handle, {});
    return _state->storages[dataLayerStorageHandleId(handle)].used.flags >= StorageFlagAllocated;
}

bool DataLayer::isStorageDirty(const StorageHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::DataLayer::isStorageDirty(): invalid handle" << handle, {});
    return _state->storages[storageHandleId(handle)].used.flags >= StorageFlagDirty;
}

bool DataLayer::isStorageDirty(const DataLayerStorageHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::DataLayer::isStorageDirty(): invalid handle" << handle, {});
    return _state->storages[dataLayerStorageHandleId(handle)].used.flags >= StorageFlagDirty;
}

void DataLayer::setStorageDirty(const StorageHandle handle) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::DataLayer::setStorageDirty(): invalid handle" << handle, );
    setStorageDirtyInternal(storageHandleId(handle));
}

void DataLayer::setStorageDirty(const DataLayerStorageHandle handle) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::DataLayer::setStorageDirty(): invalid handle" << handle, );
    setStorageDirtyInternal(dataLayerStorageHandleId(handle));
}

void DataLayer::setStorageDirtyInternal(const UnsignedInt id) {
    StorageData& storage = _state->storages[id];
    storage.used.flags |= StorageFlagDirty;

    /* Trigger layer update only if the storage is actually used */
    if(storage.used.referenceCount != 0)
        setNeedsUpdate(LayerState::NeedsCommonDataUpdate);
}

StorageFlags DataLayer::storageFlags(const StorageHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::DataLayer::storageFlags(): invalid handle" << handle, {});
    return _state->storages[storageHandleId(handle)].used.flags & StorageFlagMask;
}

StorageFlags DataLayer::storageFlags(const DataLayerStorageHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::DataLayer::storageFlags(): invalid handle" << handle, {});
    return _state->storages[dataLayerStorageHandleId(handle)].used.flags & StorageFlagMask;
}

Containers::Size3D DataLayer::storageSize(const StorageHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::DataLayer::storageSize(): invalid handle" << handle, {});
    return _state->storages[storageHandleId(handle)].used.size;
}

Containers::Size3D DataLayer::storageSize(const DataLayerStorageHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::DataLayer::storageSize(): invalid handle" << handle, {});
    return _state->storages[dataLayerStorageHandleId(handle)].used.size;
}

std::size_t DataLayer::storageReferenceCount(const StorageHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::DataLayer::storageReferenceCount(): invalid handle" << handle, {});
    return _state->storages[storageHandleId(handle)].used.referenceCount;
}

std::size_t DataLayer::storageReferenceCount(const DataLayerStorageHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::DataLayer::storageReferenceCount(): invalid handle" << handle, {});
    return _state->storages[dataLayerStorageHandleId(handle)].used.referenceCount;
}

AbstractStorage DataLayer::storageInternal(const StorageHandle handle) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::DataLayer::storage(): invalid handle" << handle,
        (AbstractStorage{*this, storageHandleStorage(handle)}));
    /* Yeah this is here just for the assert */
    return AbstractStorage{*this, storageHandleStorage(handle)};
}

AbstractStorage DataLayer::storageInternal(const DataLayerStorageHandle handle) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::DataLayer::storage(): invalid handle" << handle,
        (AbstractStorage{*this, handle}));
    /* Yeah this is here just for the assert */
    return AbstractStorage{*this, handle};
}

Containers::StridedArrayView1D<const UnsignedShort> DataLayer::storageGenerations() const {
    return stridedArrayView(_state->storages)
        .slice(&StorageData::used)
        .slice(&StorageData::Used::generation);
}

std::size_t DataLayer::usedAllocatedCount() const {
    std::size_t count = 0;
    for(const Data& data: _state->data)
        if(data.function.isAllocated())
            ++count;

    return count;
}

namespace {

inline std::size_t linearizeIndex(const Containers::Size3D& size, const Containers::Size3D& index) {
    const std::size_t rowSize = size[2];
    const std::size_t sliceSize = size[1]*rowSize;
    return index[0]*sliceSize + /* slices */
           index[1]*rowSize +   /* rows */
           index[2];            /* elements */
}

inline Containers::Size3D delinearizeIndex(const Containers::Size3D& size, std::size_t index) {
    const std::size_t rowSize = size[2];
    const std::size_t sliceSize = size[1]*rowSize;
    const std::size_t sliceRemainder = index%sliceSize;
    const Containers::Size3D out{index/sliceSize,           /* slices */
                                 sliceRemainder/rowSize,    /* rows */
                                 sliceRemainder%rowSize};   /* elements */
    /* Just a sanity check, index fitting into the size should be guaranteed by
       create() / setIndex() already. If this fails, the algorithm is incorrect
       or the index components get wrongly mixed up somewhere. */
    CORRADE_INTERNAL_DEBUG_ASSERT(out[0] < size[0] && out[1] < size[1] && out[2] < size[2]);
    return out;
}

inline UnsignedInt extractStorageId(const UnsignedInt storageIdDirty) {
    return storageIdDirty & ((1 << Implementation::DataLayerStorageHandleIdBits) - 1);
}

}

DataHandle DataLayer::create(const AbstractStorageQuery& query, const Implementation::StorageCallOoverload overload, Containers::FunctionData&& function, const NodeHandle node) {
    State& state = *_state;
    CORRADE_INTERNAL_ASSERT(query._layer == this);
    /* This function is called from StorageQuery::onUpdate(), so show that in
       the asserts. Also be helpful and print the whole handle including the
       layer, as that's what one gets from AbstractStorage::handle() as well */
    CORRADE_ASSERT(isHandleValid(query._storage),
        "Ui::StorageQuery::onUpdate(): invalid handle" << storageHandle(handle(), query._storage), {});
    /* The query should support Min/Max if the value, min, max overload is
       picked */
    CORRADE_ASSERT(overload != Implementation::StorageCallOoverload::ByValueMinMax || query.operations() >= (StorageOperation::Min|StorageOperation::Max),
        "Ui::StorageQuery::onUpdate(): query doesn't support" << (StorageOperation::Min|StorageOperation::Max) << "for this overload", {});
    CORRADE_ASSERT(function,
        "Ui::StorageQuery::onUpdate(): function is null", {});
    /** @todo the index being within storage size is tested by the StorageQuery
        constructor already so it currently doesn't make sense to check it
        again here, however it'll become important if/once storages can change
        their size, as the StorageQuery instance could be stale at the point it
        reaches DataLayer::create() */

    const UnsignedInt storageId = dataLayerStorageHandleId(query._storage);
    StorageData& storageData = state.storages[storageId];

    /* Increase storage reference count */
    ++storageData.used.referenceCount;

    const DataHandle handle = AbstractLayer::create(node);
    const UnsignedInt id = dataHandleId(handle);
    if(id >= state.data.size())
        /* Can't use NoInit because of non-trivial types */
        arrayResize(state.data, ValueInit, id + 1);
    Data& data = state.data[id];

    /* The data binding is implicitly dirty upon creation */
    data.storageIdDirty = storageId|DataIsDirty;
    data.operations = query._operations;
    /* While the 3D size is 3*4/8 bytes, in practice addressing anything with a
       >4/8 byte address is impossible, thus the 3D index gets linearized into
       a single size_t value */
    data.linearizedIndex = linearizeIndex(storageData.used.size, query._index);

    data.function = Utility::move(function);
    data.call = query._call(overload);
    data.updater = query._updater;

    /* Make sure the update function is called for the new data */
    setNeedsUpdate(LayerState::NeedsCommonDataUpdate);

    return handle;
}

void DataLayer::remove(const DataHandle handle) {
    AbstractLayer::remove(handle);
    removeInternal(dataHandleId(handle));
}

void DataLayer::remove(const LayerDataHandle handle) {
    AbstractLayer::remove(handle);
    removeInternal(layerDataHandleId(handle));
}

void DataLayer::removeInternal(const UnsignedInt id) {
    State& state = *_state;
    Data& data = state.data[id];

    /* Set the function to an empty instance to call any captured state
       destructors */
    data.function = {};

    /* Decrement storage reference count */
    const UnsignedInt storageId = extractStorageId(data.storageIdDirty);
    StorageData& storage = state.storages[storageId];
    CORRADE_INTERNAL_DEBUG_ASSERT(storage.used.referenceCount > 0);
    --storage.used.referenceCount;

    /* If the storage is reference-counted and has zero references, mark the
       layer for update so it removes the storage on the next update */
    if(storage.used.flags >= StorageFlag::ReferenceCounted && storage.used.referenceCount == 0)
        setNeedsUpdate(LayerState::NeedsCommonDataUpdate);
}

bool DataLayer::isAllocated(const DataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::DataLayer::isAllocated(): invalid handle" << handle, {});
    return _state->data[dataHandleId(handle)].function.isAllocated();
}

bool DataLayer::isAllocated(const LayerDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::DataLayer::isAllocated(): invalid handle" << handle, {});
    return _state->data[layerDataHandleId(handle)].function.isAllocated();
}

bool DataLayer::isDirty(const DataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::DataLayer::isDirty(): invalid handle" << handle, {});
    return _state->data[dataHandleId(handle)].storageIdDirty & DataIsDirty;
}

bool DataLayer::isDirty(const LayerDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::DataLayer::isDirty(): invalid handle" << handle, {});
    return _state->data[layerDataHandleId(handle)].storageIdDirty & DataIsDirty;
}

void DataLayer::setDirty(const DataHandle handle) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::DataLayer::setDirty(): invalid handle" << handle, );
    setDirtyInternal(dataHandleId(handle));
}

void DataLayer::setDirty(const LayerDataHandle handle) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::DataLayer::setDirty(): invalid handle" << handle, );
    setDirtyInternal(layerDataHandleId(handle));
}

void DataLayer::setDirtyInternal(const UnsignedInt id) {
    Data& data = _state->data[id];

    /* If the data is already dirty, the corresponding state should be set as
       well. In other words, we don't need to branch and set the state only if
       not dirty already. */
    CORRADE_INTERNAL_DEBUG_ASSERT(!(data.storageIdDirty & DataIsDirty) || state() >= LayerState::NeedsCommonDataUpdate);

    data.storageIdDirty |= DataIsDirty;
    setNeedsUpdate(LayerState::NeedsCommonDataUpdate);
}

StorageHandle DataLayer::storage(const DataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::DataLayer::storage(): invalid handle" << handle, {});
    return storageInternal(dataHandleId(handle));
}

StorageHandle DataLayer::storage(const LayerDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::DataLayer::storage(): invalid handle" << handle, {});
    return storageInternal(layerDataHandleId(handle));
}

inline StorageHandle DataLayer::storageInternal(const UnsignedInt id) const {
    const State& state = *_state;
    const UnsignedInt storageId = extractStorageId(state.data[id].storageIdDirty);
    return storageHandle(handle(), storageId, state.storages[storageId].used.generation);
}

Containers::Size3D DataLayer::index(const DataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::DataLayer::index(): invalid handle" << handle, {});
    return indexInternal(dataHandleId(handle));
}

Containers::Size3D DataLayer::index(const LayerDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::DataLayer::index(): invalid handle" << handle, {});
    return indexInternal(layerDataHandleId(handle));
}

inline Containers::Size3D DataLayer::indexInternal(const UnsignedInt id) const {
    const State& state = *_state;
    const Data& data = state.data[id];
    return delinearizeIndex(state.storages[extractStorageId(data.storageIdDirty)].used.size, data.linearizedIndex);
}

void DataLayer::setIndex(const DataHandle handle, const std::size_t index) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::DataLayer::setIndex(): invalid handle" << handle, );
    setIndexInternal(dataHandleId(handle), index);
}

void DataLayer::setIndex(const LayerDataHandle handle, const std::size_t index) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::DataLayer::setIndex(): invalid handle" << handle, );
    setIndexInternal(layerDataHandleId(handle), index);
}

void DataLayer::setIndexInternal(const UnsignedInt id, const std::size_t index) {
    #ifndef CORRADE_NO_ASSERT
    State& state = *_state;
    const Containers::Size3D& size = state.storages[extractStorageId(state.data[id].storageIdDirty)].used.size;
    #endif
    CORRADE_ASSERT(size[0] == 1 && size[1] == 1,
        "Ui::DataLayer::setIndex(): expected a 1D storage but got a size of" << size, );
    setIndexInternal(id, {0, 0, index});
}

void DataLayer::setIndex(const DataHandle handle, const Containers::Size2D& index) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::DataLayer::setIndex(): invalid handle" << handle, );
    setIndexInternal(dataHandleId(handle), index);
}

void DataLayer::setIndex(const LayerDataHandle handle, const Containers::Size2D& index) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::DataLayer::setIndex(): invalid handle" << handle, );
    setIndexInternal(layerDataHandleId(handle), index);
}

void DataLayer::setIndexInternal(const UnsignedInt id, const Containers::Size2D& index) {
    #ifndef CORRADE_NO_ASSERT
    State& state = *_state;
    const Containers::Size3D& size = state.storages[extractStorageId(state.data[id].storageIdDirty)].used.size;
    #endif
    CORRADE_ASSERT(size[0] == 1,
        "Ui::DataLayer::setIndex(): expected a 2D storage but got a size of" << size, );
    setIndexInternal(id, {0, index[0], index[1]});
}

void DataLayer::setIndex(const DataHandle handle, const Containers::Size3D& index) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::DataLayer::setIndex(): invalid handle" << handle, );
    setIndexInternal(dataHandleId(handle), index);
}

void DataLayer::setIndex(const LayerDataHandle handle, const Containers::Size3D& index) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::DataLayer::setIndex(): invalid handle" << handle, );
    setIndexInternal(layerDataHandleId(handle), index);
}

void DataLayer::setIndexInternal(const UnsignedInt id, const Containers::Size3D& index) {
    State& state = *_state;
    Data& data = state.data[id];
    const Containers::Size3D& size = state.storages[extractStorageId(data.storageIdDirty)].used.size;
    CORRADE_ASSERT(index[0] < size[0] && index[1] < size[1] && index[2] < size[2],
        "Ui::DataLayer::setIndex(): index" << index << "out of range for" << size << "elements", );
    const std::size_t linearizedIndex = linearizeIndex(size, index);

    /* If the index is different, update it and mark the data as dirty along
       with NeedsCommonDataUpdate */
    if(data.linearizedIndex != linearizedIndex) {
        data.linearizedIndex = linearizedIndex;
        data.storageIdDirty |= DataIsDirty;
        setNeedsUpdate(LayerState::NeedsCommonDataUpdate);
    }
}

bool DataLayer::isMutable(const DataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::DataLayer::isMutable(): invalid handle" << handle, {});
    return _state->data[dataHandleId(handle)].updater;
}

bool DataLayer::isMutable(const LayerDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::DataLayer::isMutable(): invalid handle" << handle, {});
    return _state->data[layerDataHandleId(handle)].updater;
}

StorageOperations DataLayer::operations(const DataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::DataLayer::operations(): invalid handle" << handle, {});
    return _state->data[dataHandleId(handle)].operations;
}

StorageOperations DataLayer::operations(const LayerDataHandle handle) const {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::DataLayer::operations(): invalid handle" << handle, {});
    return _state->data[layerDataHandleId(handle)].operations;
}

StorageUpdateState DataLayer::setInternal(const DataHandle handle, const void* const value) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::DataLayer::set(): invalid handle" << handle, {});
    return updateInternal(
        #ifndef CORRADE_NO_ASSERT
        "Ui::DataLayer::set():",
        #endif
        dataHandleData(handle), StorageOperation::Set, value);
}

StorageUpdateState DataLayer::setInternal(const LayerDataHandle handle, const void* const value) {
    CORRADE_ASSERT(isHandleValid(handle),
        "Ui::DataLayer::set(): invalid handle" << handle, {});
    return updateInternal(
        #ifndef CORRADE_NO_ASSERT
        "Ui::DataLayer::set():",
        #endif
        handle, StorageOperation::Set, value);
}

void DataLayer::reset(const DataHandle handle) {
    updateInternal(
        #ifndef CORRADE_NO_ASSERT
        "Ui::DataLayer::reset():",
        #endif
        handle, StorageOperation::Reset);
}

void DataLayer::reset(const LayerDataHandle handle) {
    updateInternal(
        #ifndef CORRADE_NO_ASSERT
        "Ui::DataLayer::reset():",
        #endif
        handle, StorageOperation::Reset);
}

void DataLayer::toggle(const DataHandle handle) {
    updateInternal(
        #ifndef CORRADE_NO_ASSERT
        "Ui::DataLayer::toggle():",
        #endif
        handle, StorageOperation::Toggle);
}

void DataLayer::toggle(const LayerDataHandle handle) {
    updateInternal(
        #ifndef CORRADE_NO_ASSERT
        "Ui::DataLayer::toggle():",
        #endif
        handle, StorageOperation::Toggle);
}

void DataLayer::increment(const DataHandle handle) {
    updateInternal(
        #ifndef CORRADE_NO_ASSERT
        "Ui::DataLayer::increment():",
        #endif
        handle, StorageOperation::Increment);
}

void DataLayer::increment(const LayerDataHandle handle) {
    updateInternal(
        #ifndef CORRADE_NO_ASSERT
        "Ui::DataLayer::increment():",
        #endif
        handle, StorageOperation::Increment);
}

void DataLayer::decrement(const DataHandle handle) {
    updateInternal(
        #ifndef CORRADE_NO_ASSERT
        "Ui::DataLayer::decrement():",
        #endif
        handle, StorageOperation::Decrement);
}

void DataLayer::decrement(const LayerDataHandle handle) {
    updateInternal(
        #ifndef CORRADE_NO_ASSERT
        "Ui::DataLayer::decrement():",
        #endif
        handle, StorageOperation::Decrement);
}

void DataLayer::setToMin(const DataHandle handle) {
    updateInternal(
        #ifndef CORRADE_NO_ASSERT
        "Ui::DataLayer::setToMin():",
        #endif
        handle, StorageOperation::Min);
}

void DataLayer::setToMin(const LayerDataHandle handle) {
    updateInternal(
        #ifndef CORRADE_NO_ASSERT
        "Ui::DataLayer::setToMin():",
        #endif
        handle, StorageOperation::Min);
}

void DataLayer::setToMax(const DataHandle handle) {
    updateInternal(
        #ifndef CORRADE_NO_ASSERT
        "Ui::DataLayer::setToMax():",
        #endif
        handle, StorageOperation::Max);
}

void DataLayer::setToMax(const LayerDataHandle handle) {
    updateInternal(
        #ifndef CORRADE_NO_ASSERT
        "Ui::DataLayer::setToMax():",
        #endif
        handle, StorageOperation::Max);
}

void DataLayer::updateInternal(
    #ifndef CORRADE_NO_ASSERT
    const char* const messagePrefix,
    #endif
    const DataHandle handle, const StorageOperation operation)
{
    CORRADE_ASSERT(isHandleValid(handle),
        messagePrefix << "invalid handle" << handle, );
    updateInternal(
        #ifndef CORRADE_NO_ASSERT
        messagePrefix,
        #endif
        dataHandleData(handle), operation, nullptr);
}

void DataLayer::updateInternal(
    #ifndef CORRADE_NO_ASSERT
    const char* const messagePrefix,
    #endif
    const LayerDataHandle handle, const StorageOperation operation)
{
    CORRADE_ASSERT(isHandleValid(handle),
        messagePrefix << "invalid handle" << handle, );
    updateInternal(
        #ifndef CORRADE_NO_ASSERT
        messagePrefix,
        #endif
        handle, operation, nullptr);
}

StorageUpdateState DataLayer::updateInternal(
    #ifndef CORRADE_NO_ASSERT
    const char* const messagePrefix,
    #endif
    const LayerDataHandle handle, StorageOperation operation, const void* const value)
{
    const State& state = *_state;
    const Data& data = state.data[layerDataHandleId(handle)];
    /* In case of StorageOperation::Min / Max, only the queries could be
       supported, so check also for mutability. All other operations are not
       allowed to be specified if the StorageQuery is constructed without an
       updater, so for Set, Reset etc. this check wouldn't be needed. */
    CORRADE_ASSERT(data.updater,
        messagePrefix << "data binding is immutable", {});
    CORRADE_ASSERT(data.operations >= operation,
        messagePrefix << operation << "not supported", {});
    const UnsignedInt storageId = extractStorageId(data.storageIdDirty);
    const StorageData& storage = state.storages[storageId];
    const StorageUpdateState updateState = data.updater(*this, dataLayerStorageHandle(storageId, state.storages[storageId].used.generation), delinearizeIndex(storage.used.size, data.linearizedIndex), operation, value);
    /* All operations except Set are expected to return only Success */
    CORRADE_ASSERT(operation == StorageOperation::Set || updateState == StorageUpdateState::Success,
        messagePrefix << "updater implementation expected to return" << StorageUpdateState::Success << "for" << operation << "but got" << updateState, {});
    return updateState;
}

LayerFeatures DataLayer::doFeatures() const {
    return {};
}

void DataLayer::doClean(const Containers::BitArrayView dataIdsToRemove) {
    /** @todo some way to iterate bits */
    for(std::size_t i = 0; i != dataIdsToRemove.size(); ++i) {
        if(!dataIdsToRemove[i])
            continue;
        removeInternal(i);
    }
}

void DataLayer::doPreUpdate(const LayerStates state_) {
    /* There's no other case when this function should get called, besides
       directly through the (test-only) public AbstractLayer interface */
    #ifdef CORRADE_NO_ASSERT
    static_cast<void>(state_);
    #endif
    CORRADE_INTERNAL_ASSERT(state_ == LayerState::NeedsCommonDataUpdate);

    /** @todo once fast bit traversal is a thing, for faster updates might want
        to maintain per-data and per-storage dirty bits in dedicated BitArrays
        and data->storage mapping as well, which then gets used to map
        per-storage dirty bits to per-data and ultimately traverse only those
        that are set */

    /* Go through all data and fire updates on data that are dirty or data
       associated with storages that are dirty */
    State& state = *_state;
    const Containers::StridedArrayView1D<const UnsignedShort> generations = this->generations();
    const LayerHandle layerHandle = handle();
    for(std::size_t i = 0; i != state.data.size(); ++i) {
        Data& data = state.data[i];

        /* Data with null functions are removed, skip those */
        if(!data.function)
            continue;

        /* If neither the data nor the storage is dirty, skip */
        const UnsignedInt storageId = extractStorageId(data.storageIdDirty);
        const StorageData& storage = state.storages[storageId];
        if(!(data.storageIdDirty & DataIsDirty) &&
           !(storage.used.flags >= StorageFlagDirty))
            continue;

        /** @todo to avoid too many nested function calls, we could also pass
            the data pointer directly to the call function instead of it having
            to go through AbstractStorage::data() -> DataLayer::storageData()
            each time. In other words, AbstractStorage would have an ability to
            use a cached data member pointer, by default a storage instantiated
            through DataLayer::storage<T>() would query it from the layer every
            time (as in-place pointers can change on reallocation), but here
            it'd use the cached value. */
        data.call(
            *this, dataLayerStorageHandle(storageId, storage.used.generation),
            delinearizeIndex(storage.used.size, data.linearizedIndex),
            dataHandle(layerHandle, i, generations[i]),
            data.function);

        /* Update got called, reset the data dirty bit if it's set */
        data.storageIdDirty &= ~DataIsDirty;
    }

    /* Once all data are updated, reset the storage dirty bits as well */
    for(std::size_t i = 0; i != state.storages.size(); ++i) {
        StorageData& storage = state.storages[i];
        /* This can be safely done even on removed storages, as the flags field
           is preserved (but cleared, so this is a no-op). */
        storage.used.flags &= ~StorageFlagDirty;

        /* If the storage is reference-counted and has zero references, remove
           it. Storages have their flags cleared on removal, so this branch
           shouldn't be entered for removed storages (which have the size set
           to zero). */
        if(storage.used.flags >= StorageFlag::ReferenceCounted && storage.used.referenceCount == 0) {
            CORRADE_INTERNAL_DEBUG_ASSERT(storage.used.size[0]);
            removeStorageInternal(i);
        }
    }
}

AbstractStorageQuery::AbstractStorageQuery(const AbstractStorage& storage, const Containers::Size3D& index, const StorageOperations operations, void(*(*const call)(Implementation::StorageCallOoverload))(DataLayer&, DataLayerStorageHandle, const Containers::Size3D&, DataHandle, Containers::FunctionData&), StorageUpdateState(*const updater)(DataLayer&, DataLayerStorageHandle, const Containers::Size3D&, StorageOperation, const void*)): _layer{&storage.layer()}, _storage{storageHandleStorage(storage.handle())}, _operations{operations}, _index{index}, _call{call}, _updater{updater} {
    /* The class is always constructed through the StorageQuery subclass, so
       make the assertions mention that to reduce confusion */
    CORRADE_ASSERT(_layer->isHandleValid(_storage),
        "Ui::StorageQuery: invalid handle" << storageHandle(_layer->handle(), _storage), );
    #ifndef CORRADE_NO_ASSERT
    const Containers::Size3D& size = storage.size();
    #endif
    CORRADE_ASSERT(index[0] < size[0] && index[1] < size[1] && index[2] < size[2],
        "Ui::StorageQuery: index" << index << "out of range for" << size << "elements", );
    #ifndef CORRADE_NO_ASSERT
    for(const StorageOperations expected: {
        StorageOperation::Min|StorageOperation::Max,
        StorageOperation::Increment|StorageOperation::Decrement,
    })
        CORRADE_ASSERT(!(operations & expected) || operations >= expected,
            "Ui::StorageQuery: either both" << expected << "have to be set or neither", );
    #endif
    /* Min|Max can be used for the query as well */
    CORRADE_ASSERT(!(operations & ~(StorageOperation::Min|StorageOperation::Max)) || updater,
        "Ui::StorageQuery:" << (operations & ~(StorageOperation::Min|StorageOperation::Max)) << "requires a non-null updater", );
}

void AbstractStorageQuery::reset() const {
    updateInternal(
        #ifndef CORRADE_NO_ASSERT
        "Ui::StorageQuery::reset():",
        #endif
        StorageOperation::Reset, nullptr);
}

void AbstractStorageQuery::toggle() const {
    updateInternal(
        #ifndef CORRADE_NO_ASSERT
        "Ui::StorageQuery::toggle():",
        #endif
        StorageOperation::Toggle, nullptr);
}

void AbstractStorageQuery::increment() const {
    updateInternal(
        #ifndef CORRADE_NO_ASSERT
        "Ui::StorageQuery::increment():",
        #endif
        StorageOperation::Increment, nullptr);
}

void AbstractStorageQuery::decrement() const {
    updateInternal(
        #ifndef CORRADE_NO_ASSERT
        "Ui::StorageQuery::decrement():",
        #endif
        StorageOperation::Decrement, nullptr);
}

void AbstractStorageQuery::setToMin() const {
    updateInternal(
        #ifndef CORRADE_NO_ASSERT
        "Ui::StorageQuery::setToMin():",
        #endif
        StorageOperation::Min, nullptr);
}

void AbstractStorageQuery::setToMax() const {
    updateInternal(
        #ifndef CORRADE_NO_ASSERT
        "Ui::StorageQuery::setToMax():",
        #endif
        StorageOperation::Max, nullptr);
}

StorageUpdateState AbstractStorageQuery::updateInternal(
    #ifndef CORRADE_NO_ASSERT
    const char* const messagePrefix,
    #endif
    const StorageOperation operation, const void* const value) const
{
    CORRADE_ASSERT(_layer->isHandleValid(_storage),
        messagePrefix << "invalid handle" << storageHandle(_layer->handle(), _storage), {});
    /* In case of StorageOperation::Min / Max, only the queries could be
       supported, so check also for mutability. All other operations are not
       allowed to be specified if the StorageQuery is constructed without an
       updater, so for Set, Reset etc. this check wouldn't be needed. */
    CORRADE_ASSERT(_updater,
        messagePrefix << "query is immutable", {});
    CORRADE_ASSERT(_operations >= operation,
        messagePrefix << operation << "not supported", {});
    const StorageUpdateState state = _updater(*_layer, _storage, _index, operation, value);
    CORRADE_ASSERT(operation == StorageOperation::Set || state == StorageUpdateState::Success,
        messagePrefix << "updater implementation expected to return" << StorageUpdateState::Success << "for" << operation << "but got" << state, {});
    return state;
}

StorageUpdateState AbstractStorageQuery::setInternal(const void* const value) const {
    return updateInternal(
        #ifndef CORRADE_NO_ASSERT
        "Ui::StorageQuery::set():",
        #endif
        StorageOperation::Set, value);
}

}}
