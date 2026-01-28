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

#include <Corrade/Containers/BitArrayView.h>
#include <Corrade/Containers/Function.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Containers/String.h>
#include <Corrade/Containers/Iterable.h> /** @todo remove once NeedsDataClean doesn't need to be handled */
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/Container.h>
#include <Corrade/TestSuite/Compare/Numeric.h>
#include <Corrade/TestSuite/Compare/String.h>
#include <Corrade/Utility/Algorithms.h>
#include <Magnum/Math/Vector2.h> /* for referenceCounted() */

#include "Magnum/Ui/AbstractAnimator.h" /** @todo remove once NeedsDataClean doesn't need to be handled */
#include "Magnum/Ui/AbstractUserInterface.h" /* for referenceCounted() */
#include "Magnum/Ui/DataLayer.h"
#include "Magnum/Ui/Handle.h" /* for LayerHandle extraction tests */

namespace Magnum { namespace Ui { namespace Test { namespace {

struct DataLayerTest: TestSuite::Tester {
    explicit DataLayerTest();

    void dataLayerStorageHandle();
    void dataLayerStorageHandleInvalid();
    void debugDataLayerStorageHandle();
    void debugDataLayerStorageHandlePacked();

    void storageHandle();
    void storageHandleInvalid();
    void debugStorageHandle();
    void debugStorageHandlePacked();

    void debugStorageFlag();
    void debugStorageFlagPacked();
    void debugStorageFlags();
    void debugStorageFlagsPacked();

    void construct();
    void constructCopy();
    void constructMove();

    void createRemoveStorage();
    void createRemoveStorageInPlace();
    void createRemoveStorageAllocated();
    void createRemoveStorageHandleRecycle();
    void createRemoveStorageHandleDisable();
    void createStorageNoHandlesLeft();
    void createStorageInvalid();
    #ifndef CORRADE_TARGET_32BIT
    void createStorageTooLarge();
    #endif
    void removeStorageInvalid();

    void setStorageDirty();

    void invalidStorageHandle();

    void createRemoveSetupTeardown();
    void createRemove();
    void createRemoveHandleRecycle();
    void createMemberFunction();
    void createOverloaded();
    void createOverloadedMemberFunction();
    void createInvalid();

    void setIndex();
    void setIndexInvalid();

    void invalidHandle();

    void indexLinearization();
    #ifndef CORRADE_TARGET_32BIT
    void indexLinearizationFullStorageCapacity();
    #endif

    void clean();
    /* No updateEmpty() like in other layers as doUpdate() currently isn't
       implemented at all */
    void update();

    void referenceCounted();
};

const struct {
    const char* name;
    Containers::Size3D size, index;
} IndexLinearizationData[]{
    {"general case",
        {7, 11, 5}, {2, 4, 3}},
    {"begin of the data",
        {7, 11, 5}, {0, 0, 0}},
    {"end of the data",
        {7, 11, 5}, {6, 10, 4}},
    {"end of the data, expands just in dimension 0",
        {29, 1, 1}, {28, 0, 0}},
    {"end of the data, expands just in dimension 1",
        {1, 29, 1}, {0, 28, 0}},
    {"end of the data, expands just in dimension 2",
        {1, 1, 29}, {0, 0, 28}},
    #ifndef CORRADE_TARGET_32BIT
    {"max size in dimension 0",
        {1ull << 43, 1, 1}, {27364652678ull, 0, 0}},
    {"max size in dimension 0, end of the data",
        {1ull << 43, 1, 1}, {(1ull << 43) - 1, 0, 0}},
    {"max size in dimension 1",
        {1, 1ull << 43, 1}, {0, 928079817290ull, 0}},
    {"max size in dimension 1, end of the data",
        {1, 1ull << 43, 1}, {0, (1ull << 43) - 1, 0}},
    {"max size in dimension 2",
        {1, 1, 1ull << 43}, {0, 0, 2751651672ull}},
    {"max size in dimension 2, end of the data",
        {1, 1, 1ull << 43}, {0, 0, (1ull << 43) - 1}},
    {"max size in dimension 0, 1",
        {1ull << 21, 1ull << 22, 1}, {1973982ull, 349ull, 0}},
    {"max size in dimension 0, 1; end of the data",
        {1ull << 21, 1ull << 22, 1}, {(1ull << 21) - 1, (1ull << 22) - 1, 0}},
    {"max size in dimension 0, 2",
        {1ull << 22, 1, 1ull << 21}, {1387ull, 0, 1678671ull}},
    {"max size in dimension 0, 2; end of the data",
        {1ull << 22, 1, 1ull << 21}, {(1ull << 22) - 1, 0, (1ull << 21) - 1}},
    {"max size in dimension 1, 2",
        {1, 1ull << 21, 1ull << 22}, {0, 127656ull, 286ull}},
    {"max size in dimension 1, 2; end of the data",
        {1, 1ull << 21, 1ull << 22}, {0, (1ull << 21) - 1, (1ull << 22) - 1}},
    {"max size in all dimensions",
        {1ull << 14, 1ull << 15, 1ull << 14},
        {3622ull, 3176ull, 131ull}},
    {"max size in all dimensions, end of the data",
        {1ull << 14, 1ull << 15, 1ull << 14},
        {(1ull << 14) - 1, (1ull << 15) - 1, (1ull << 14) - 1}},
    #endif
};

const struct {
    const char* name;
    bool removeNodes;
    LayerStates extraStatesCreate, extraStatesRemove;
} ReferenceCountedData[]{
    {"direct data removal",
        /** @todo make it set NeedsDataClean only if any animators are actually
            attached */
        false, {}, LayerState::NeedsDataClean},
    {"node removal",
        true, LayerState::NeedsNodeOffsetSizeUpdate|LayerState::NeedsAttachmentUpdate, {}},
};

DataLayerTest::DataLayerTest() {
    addTests({&DataLayerTest::dataLayerStorageHandle,
              &DataLayerTest::dataLayerStorageHandleInvalid,
              &DataLayerTest::debugDataLayerStorageHandle,
              &DataLayerTest::debugDataLayerStorageHandlePacked,

              &DataLayerTest::storageHandle,
              &DataLayerTest::storageHandleInvalid,
              &DataLayerTest::debugStorageHandle,
              &DataLayerTest::debugStorageHandlePacked,

              &DataLayerTest::debugStorageFlag,
              &DataLayerTest::debugStorageFlagPacked,
              &DataLayerTest::debugStorageFlags,
              &DataLayerTest::debugStorageFlagsPacked,

              &DataLayerTest::construct,
              &DataLayerTest::constructCopy,
              &DataLayerTest::constructMove,

              &DataLayerTest::createRemoveStorage,
              &DataLayerTest::createRemoveStorageInPlace,
              &DataLayerTest::createRemoveStorageAllocated,
              &DataLayerTest::createRemoveStorageHandleRecycle,
              &DataLayerTest::createRemoveStorageHandleDisable,
              &DataLayerTest::createStorageNoHandlesLeft,
              &DataLayerTest::createStorageInvalid,
              #ifndef CORRADE_TARGET_32BIT
              &DataLayerTest::createStorageTooLarge,
              #endif
              &DataLayerTest::removeStorageInvalid,

              &DataLayerTest::invalidStorageHandle,

              &DataLayerTest::setStorageDirty});

    addTests({&DataLayerTest::createRemove,
              &DataLayerTest::createRemoveHandleRecycle,
              &DataLayerTest::createMemberFunction,
              &DataLayerTest::createOverloaded,
              &DataLayerTest::createOverloadedMemberFunction},
        &DataLayerTest::createRemoveSetupTeardown,
        &DataLayerTest::createRemoveSetupTeardown);

    addTests({&DataLayerTest::createInvalid,

              &DataLayerTest::setIndex,
              &DataLayerTest::setIndexInvalid,

              &DataLayerTest::invalidHandle});

    addInstancedTests({&DataLayerTest::indexLinearization},
        Containers::arraySize(IndexLinearizationData));

    #ifndef CORRADE_TARGET_32BIT
    addTests({&DataLayerTest::indexLinearizationFullStorageCapacity});
    #endif

    addTests({&DataLayerTest::clean,
              &DataLayerTest::update});

    addInstancedTests({&DataLayerTest::referenceCounted},
        Containers::arraySize(ReferenceCountedData));
}

void DataLayerTest::dataLayerStorageHandle() {
    CORRADE_COMPARE(DataLayerStorageHandle::Null, DataLayerStorageHandle{});
    CORRADE_COMPARE(Ui::dataLayerStorageHandle(0, 0), DataLayerStorageHandle::Null);
    CORRADE_COMPARE(Ui::dataLayerStorageHandle(0xabcde, 0x123), DataLayerStorageHandle(0x123abcde));
    CORRADE_COMPARE(Ui::dataLayerStorageHandle(0xfffff, 0xfff), DataLayerStorageHandle(0xffffffff));
    CORRADE_COMPARE(dataLayerStorageHandleId(DataLayerStorageHandle(0x123abcde)), 0xabcde);
    CORRADE_COMPARE(dataLayerStorageHandleGeneration(DataLayerStorageHandle::Null), 0);
    CORRADE_COMPARE(dataLayerStorageHandleGeneration(DataLayerStorageHandle(0x123abcde)), 0x123);

    constexpr DataLayerStorageHandle handle = Ui::dataLayerStorageHandle(0xabcde, 0x123);
    constexpr UnsignedInt id = dataLayerStorageHandleId(handle);
    constexpr UnsignedInt generation = dataLayerStorageHandleGeneration(handle);
    CORRADE_COMPARE(handle, DataLayerStorageHandle(0x123abcde));
    CORRADE_COMPARE(id, 0xabcde);
    CORRADE_COMPARE(generation, 0x123);
}

void DataLayerTest::dataLayerStorageHandleInvalid() {
    CORRADE_SKIP_IF_NO_DEBUG_ASSERT();

    /* Verify the zero generation check isn't off by a bit */
    dataLayerStorageHandleId(Ui::dataLayerStorageHandle(0, 1));
    dataLayerStorageHandleId(Ui::dataLayerStorageHandle(0, 1 << (Implementation::DataLayerStorageHandleGenerationBits - 1)));

    Containers::String out;
    Error redirectError{&out};
    Ui::dataLayerStorageHandle(0x100000, 0x1);
    Ui::dataLayerStorageHandle(0x1, 0x1000);
    dataLayerStorageHandleId(DataLayerStorageHandle::Null);
    dataLayerStorageHandleId(Ui::dataLayerStorageHandle(0xabcde, 0));
    CORRADE_COMPARE_AS(out,
        "Ui::dataLayerStorageHandle(): expected index to fit into 20 bits and generation into 12, got 0x100000 and 0x1\n"
        "Ui::dataLayerStorageHandle(): expected index to fit into 20 bits and generation into 12, got 0x1 and 0x1000\n"
        "Ui::dataLayerStorageHandleId(): invalid handle Ui::DataLayerStorageHandle::Null\n"
        "Ui::dataLayerStorageHandleId(): invalid handle Ui::DataLayerStorageHandle(0xabcde, 0x0)\n",
        TestSuite::Compare::String);
}

void DataLayerTest::debugDataLayerStorageHandle() {
    Containers::String out;
    Debug{&out} << DataLayerStorageHandle::Null << Ui::dataLayerStorageHandle(0x12345, 0xabc);
    CORRADE_COMPARE(out, "Ui::DataLayerStorageHandle::Null Ui::DataLayerStorageHandle(0x12345, 0xabc)\n");
}

void DataLayerTest::debugDataLayerStorageHandlePacked() {
    Containers::String out;
    /* Last is not packed, ones before should not make any flags persistent */
    Debug{&out} << Debug::packed << DataLayerStorageHandle::Null << Debug::packed << Ui::dataLayerStorageHandle(0x12345, 0xabc) << Ui::dataLayerStorageHandle(0x67890, 0xdef);
    CORRADE_COMPARE(out, "Null {0x12345, 0xabc} Ui::DataLayerStorageHandle(0x67890, 0xdef)\n");
}

void DataLayerTest::storageHandle() {
    CORRADE_COMPARE(StorageHandle::Null, StorageHandle{});
    CORRADE_COMPARE(Ui::storageHandle(LayerHandle::Null, 0, 0), StorageHandle::Null);
    CORRADE_COMPARE(Ui::storageHandle(LayerHandle(0x12ab), 0x34567, 0xcde), StorageHandle(0x12abcde34567));
    CORRADE_COMPARE(Ui::storageHandle(LayerHandle(0xffff), 0xfffff, 0xfff), StorageHandle(0xffffffffffff));
    CORRADE_COMPARE(Ui::storageHandle(LayerHandle::Null, DataLayerStorageHandle::Null), StorageHandle::Null);
    CORRADE_COMPARE(Ui::storageHandle(LayerHandle(0x12ab), DataLayerStorageHandle(0xcde34567)), StorageHandle(0x12abcde34567));
    CORRADE_COMPARE(storageHandleLayer(StorageHandle::Null), LayerHandle::Null);
    CORRADE_COMPARE(storageHandleLayer(StorageHandle(0x12abcde34567)), LayerHandle(0x12ab));
    CORRADE_COMPARE(storageHandleStorage(StorageHandle::Null), DataLayerStorageHandle::Null);
    CORRADE_COMPARE(storageHandleStorage(StorageHandle(0x12abcde34567)), DataLayerStorageHandle(0xcde34567));
    CORRADE_COMPARE(storageHandleLayerId(StorageHandle(0x12abcde34567)), 0xab);
    CORRADE_COMPARE(storageHandleLayerGeneration(StorageHandle::Null), 0);
    CORRADE_COMPARE(storageHandleLayerGeneration(StorageHandle(0x12abcde34567)), 0x12);
    CORRADE_COMPARE(storageHandleId(StorageHandle(0x12abcde34567)), 0x34567);
    CORRADE_COMPARE(storageHandleGeneration(StorageHandle::Null), 0);
    CORRADE_COMPARE(storageHandleGeneration(StorageHandle(0x12abcde34567)), 0xcde);

    constexpr StorageHandle handle1 = Ui::storageHandle(LayerHandle(0x12ab), 0x34567, 0xcde);
    constexpr StorageHandle handle2 = Ui::storageHandle(LayerHandle(0x12ab), DataLayerStorageHandle(0xcde34567));
    constexpr LayerHandle layer = storageHandleLayer(handle1);
    constexpr DataLayerStorageHandle data = storageHandleStorage(handle1);
    constexpr UnsignedInt layerId = storageHandleLayerId(handle1);
    constexpr UnsignedInt layerGeneration = storageHandleLayerGeneration(handle1);
    constexpr UnsignedInt id = storageHandleId(handle1);
    constexpr UnsignedInt generation = storageHandleGeneration(handle1);
    CORRADE_COMPARE(handle1, StorageHandle(0x12abcde34567));
    CORRADE_COMPARE(handle2, StorageHandle(0x12abcde34567));
    CORRADE_COMPARE(layer, LayerHandle(0x12ab));
    CORRADE_COMPARE(data, DataLayerStorageHandle(0xcde34567));
    CORRADE_COMPARE(layerId, 0xab);
    CORRADE_COMPARE(layerGeneration, 0x12);
    CORRADE_COMPARE(id, 0x34567);
    CORRADE_COMPARE(generation, 0xcde);
}

void DataLayerTest::storageHandleInvalid() {
    CORRADE_SKIP_IF_NO_DEBUG_ASSERT();

    /* Verify the zero generation check isn't off by a bit. The other
       generation being zero shouldn't matter. */
    storageHandleLayerId(Ui::storageHandle(layerHandle(0, 1), DataLayerStorageHandle::Null));
    storageHandleLayerId(Ui::storageHandle(layerHandle(0, 1 << (Implementation::LayerHandleGenerationBits - 1)), DataLayerStorageHandle::Null));
    storageHandleId(Ui::storageHandle(LayerHandle::Null, 0, 1));
    storageHandleId(Ui::storageHandle(LayerHandle::Null, 0, 1 << (Implementation::DataLayerStorageHandleGenerationBits - 1)));

    Containers::String out;
    Error redirectError{&out};
    Ui::storageHandle(LayerHandle::Null, 0x100000, 0x1);
    Ui::storageHandle(LayerHandle::Null, 0x1, 0x1000);
    storageHandleLayerId(StorageHandle::Null);
    storageHandleLayerId(Ui::storageHandle(LayerHandle::Null, 0x1, 0x1));
    storageHandleLayerId(Ui::storageHandle(layerHandle(0xab, 0), 0x1, 0x1));
    storageHandleId(StorageHandle::Null);
    storageHandleId(Ui::storageHandle(layerHandle(0x1, 0x1), DataLayerStorageHandle::Null));
    storageHandleId(Ui::storageHandle(layerHandle(0x1, 0x1), Ui::dataLayerStorageHandle(0xabcde, 0)));
    CORRADE_COMPARE_AS(out,
        "Ui::storageHandle(): expected index to fit into 20 bits and generation into 12, got 0x100000 and 0x1\n"
        "Ui::storageHandle(): expected index to fit into 20 bits and generation into 12, got 0x1 and 0x1000\n"
        "Ui::storageHandleLayerId(): invalid layer portion of Ui::StorageHandle::Null\n"
        "Ui::storageHandleLayerId(): invalid layer portion of Ui::StorageHandle(Null, {0x1, 0x1})\n"
        "Ui::storageHandleLayerId(): invalid layer portion of Ui::StorageHandle({0xab, 0x0}, {0x1, 0x1})\n"
        "Ui::storageHandleId(): invalid storage portion of Ui::StorageHandle::Null\n"
        "Ui::storageHandleId(): invalid storage portion of Ui::StorageHandle({0x1, 0x1}, Null)\n"
        "Ui::storageHandleId(): invalid storage portion of Ui::StorageHandle({0x1, 0x1}, {0xabcde, 0x0})\n",
        TestSuite::Compare::String);
}

void DataLayerTest::debugStorageHandle() {
    Containers::String out;
    Debug{&out} << StorageHandle::Null << Ui::storageHandle(LayerHandle::Null, Ui::dataLayerStorageHandle(0xabcde, 0x12)) << Ui::storageHandle(layerHandle(0x34, 0x56), DataLayerStorageHandle::Null) << Ui::storageHandle(layerHandle(0x34, 0x56), 0xabcde, 0x12);
    CORRADE_COMPARE(out, "Ui::StorageHandle::Null Ui::StorageHandle(Null, {0xabcde, 0x12}) Ui::StorageHandle({0x34, 0x56}, Null) Ui::StorageHandle({0x34, 0x56}, {0xabcde, 0x12})\n");
}

void DataLayerTest::debugStorageHandlePacked() {
    Containers::String out;
    /* Last is not packed, ones before should not make any flags persistent */
    Debug{&out} << Debug::packed << StorageHandle::Null << Debug::packed << Ui::storageHandle(LayerHandle::Null, Ui::dataLayerStorageHandle(0xabcde, 0x12)) << Debug::packed << Ui::storageHandle(layerHandle(0x34, 0x56), DataLayerStorageHandle::Null) << Debug::packed << Ui::storageHandle(layerHandle(0x34, 0x56), 0xabcde, 0x12) << Ui::storageHandle(layerHandle(0x78, 0x90), 0xf0123, 0xab);
    CORRADE_COMPARE(out, "Null {Null, {0xabcde, 0x12}} {{0x34, 0x56}, Null} {{0x34, 0x56}, {0xabcde, 0x12}} Ui::StorageHandle({0x78, 0x90}, {0xf0123, 0xab})\n");
}

void DataLayerTest::debugStorageFlag() {
    Containers::String out;
    Debug{&out} << StorageFlag::ReferenceCounted << StorageFlag(0xbe);
    CORRADE_COMPARE(out, "Ui::StorageFlag::ReferenceCounted Ui::StorageFlag(0xbe)\n");
}

void DataLayerTest::debugStorageFlagPacked() {
    Containers::String out;
    /* Last is not packed, ones before should not make any flags persistent */
    Debug{&out} << Debug::packed << StorageFlag::ReferenceCounted << Debug::packed << StorageFlag(0xbe) << StorageFlag::ReferenceCounted;
    CORRADE_COMPARE(out, "ReferenceCounted 0xbe Ui::StorageFlag::ReferenceCounted\n");
}

void DataLayerTest::debugStorageFlags() {
    Containers::String out;
    Debug{&out} << (StorageFlag::ReferenceCounted|StorageFlag(0x80)) << StorageFlags{};
    CORRADE_COMPARE(out, "Ui::StorageFlag::ReferenceCounted|Ui::StorageFlag(0x80) Ui::StorageFlags{}\n");
}

void DataLayerTest::debugStorageFlagsPacked() {
    Containers::String out;
    /* Last is not packed, ones before should not make any flags persistent */
    Debug{&out} << Debug::packed << (StorageFlag::ReferenceCounted|StorageFlag(0x80)) << Debug::packed << StorageFlags{} << (StorageFlag::ReferenceCounted|StorageFlag(0x80));
    CORRADE_COMPARE(out, "ReferenceCounted|0x80 {} Ui::StorageFlag::ReferenceCounted|Ui::StorageFlag(0x80)\n");
}

void DataLayerTest::construct() {
    DataLayer layer{layerHandle(137, 0xde)};
    CORRADE_COMPARE(layer.handle(), layerHandle(137, 0xde));
    CORRADE_COMPARE(layer.storageCapacity(), 0);
    CORRADE_COMPARE(layer.storageUsedCount(), 0);
    CORRADE_VERIFY(!layer.isHandleValid(DataLayerStorageHandle::Null));
    CORRADE_VERIFY(!layer.isHandleValid(StorageHandle::Null));
    /* Verify that out-of-bounds ID and zero generation is handled correctly
       even for an empty layer */
    CORRADE_VERIFY(!layer.isHandleValid(Ui::dataLayerStorageHandle(0, 1)));
    CORRADE_VERIFY(!layer.isHandleValid(Ui::dataLayerStorageHandle(1, 0)));
    CORRADE_VERIFY(!layer.isHandleValid(Ui::storageHandle(layer.handle(), 0, 1)));
    CORRADE_VERIFY(!layer.isHandleValid(Ui::storageHandle(layer.handle(), 1, 0)));
}

void DataLayerTest::constructCopy() {
    CORRADE_VERIFY(!std::is_copy_constructible<DataLayer>{});
    CORRADE_VERIFY(!std::is_copy_assignable<DataLayer>{});
}

void DataLayerTest::constructMove() {
    struct AllocatedStorage: AbstractStorage {
        explicit AllocatedStorage(DataLayer& layer, Int& destructed): AbstractStorage{layer} {
            createAllocated(&destructed, 0, [](void* data, std::size_t) {
                ++*static_cast<Int*>(data);
            });
        }
    };
    struct InPlaceStorage: AbstractStorage {
        explicit InPlaceStorage(DataLayer& layer): AbstractStorage{layer} {
            /* Fill the in-place storage with garbage to verify it isn't
               accidentally called as a deleter */
            for(UnsignedInt& i: createInPlace<UnsignedInt>())
                i = 0xdeadbeef;
        }
    };

    /* Add an allocated storage to verify it gets properly deleted, and
       deleted exactly once, and a non-allocated storage to verify it doesn't
       get */
    Int destructed = 0;
    {
        DataLayer a{layerHandle(137, 0xde)};

        AllocatedStorage{a, destructed};
        InPlaceStorage{a};
        CORRADE_COMPARE(destructed, 0);
        CORRADE_COMPARE(a.storageUsedCount(), 2);
        CORRADE_COMPARE(a.storageUsedAllocatedCount(), 1);

        DataLayer b = Utility::move(a);
        CORRADE_COMPARE(destructed, 0);
        CORRADE_COMPARE(b.handle(), layerHandle(137, 0xde));
        CORRADE_COMPARE(b.storageUsedCount(), 2);
        CORRADE_COMPARE(b.storageUsedAllocatedCount(), 1);

        DataLayer c{layerHandle(34, 56)};
        c = Utility::move(b);
        CORRADE_COMPARE(destructed, 0);
        CORRADE_COMPARE(c.handle(), layerHandle(137, 0xde));
        CORRADE_COMPARE(c.storageUsedCount(), 2);
        CORRADE_COMPARE(c.storageUsedAllocatedCount(), 1);
    }
    CORRADE_COMPARE(destructed, 1);

    CORRADE_VERIFY(std::is_nothrow_move_constructible<DataLayer>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<DataLayer>::value);
}

void DataLayerTest::createRemoveStorage() {
    /* Verifies just storage allocation and removal inside DataLayer, not
       anything related to actually allocating or initializing its memory (so
       not isStorageAllocated() or storageUsedAllocatedCount()). The
       createInPlace() / createAllocated() functions and their effects are
       tested in createRemoveStorage*() below. */

    struct DummyStorage: AbstractStorage {
        explicit DummyStorage(DataLayer& layer, StorageFlags flags): AbstractStorage{layer, flags} {}
        explicit DummyStorage(DataLayer& layer, std::size_t size, StorageFlags flags): AbstractStorage{layer, size, flags} {}
        explicit DummyStorage(DataLayer& layer, const Containers::Size2D& size, StorageFlags flags): AbstractStorage{layer, size, flags} {}
        explicit DummyStorage(DataLayer& layer, const Containers::Size3D& size, StorageFlags flags): AbstractStorage{layer, size, flags} {}
    };

    DataLayer layer{layerHandle(0xab, 0x12)};

    /* Zero dimensions. Except for StorageFlag::ReferenceCounted, which is
       tested separately in referenceCounted() below, creating a storage
       doesn't set any layer state flags. */
    DummyStorage first{layer, StorageFlags{0x18}};
    CORRADE_COMPARE(&first.layer(), &layer);
    CORRADE_COMPARE(first.handle(), Ui::storageHandle(layer.handle(), 0, 1));
    CORRADE_COMPARE(first, first.handle());
    CORRADE_VERIFY(layer.isHandleValid(first));
    CORRADE_COMPARE(layer.state(), LayerStates{});
    CORRADE_COMPARE(layer.storageCapacity(), 1);
    CORRADE_COMPARE(layer.storageUsedCount(), 1);
    CORRADE_VERIFY(!layer.isStorageDirty(first));
    CORRADE_COMPARE(layer.storageFlags(first), StorageFlags{0x18});
    CORRADE_COMPARE(layer.storageSize(first), (Containers::Size3D{1, 1, 1}));
    CORRADE_COMPARE(layer.storageReferenceCount(first), 0);
    CORRADE_COMPARE(&layer.storage<DummyStorage>(first).layer(), &layer);
    CORRADE_COMPARE(layer.storage<DummyStorage>(first).handle(), first.handle());
    CORRADE_VERIFY(!first.isDirty());
    CORRADE_COMPARE(first.flags(), StorageFlags{0x18});
    CORRADE_COMPARE(first.size(), (Containers::Size3D{1, 1, 1}));
    CORRADE_COMPARE(first.referenceCount(), 0);

    /* One dimension */
    DummyStorage second{layer, 15, StorageFlags{0x20}};
    CORRADE_COMPARE(&second.layer(), &layer);
    CORRADE_COMPARE(second.handle(), Ui::storageHandle(layer.handle(), 1, 1));
    CORRADE_COMPARE(second, second.handle());
    CORRADE_VERIFY(layer.isHandleValid(second));
    CORRADE_COMPARE(layer.state(), LayerStates{});
    CORRADE_COMPARE(layer.storageCapacity(), 2);
    CORRADE_COMPARE(layer.storageUsedCount(), 2);
    CORRADE_VERIFY(!layer.isStorageDirty(second));
    CORRADE_COMPARE(layer.storageFlags(second), StorageFlags{0x20});
    CORRADE_COMPARE(layer.storageSize(second), (Containers::Size3D{1, 1, 15}));
    CORRADE_COMPARE(layer.storageReferenceCount(second), 0);
    CORRADE_COMPARE(&layer.storage<DummyStorage>(second).layer(), &layer);
    CORRADE_COMPARE(layer.storage<DummyStorage>(second).handle(), second.handle());
    CORRADE_VERIFY(!second.isDirty());
    CORRADE_COMPARE(second.flags(), StorageFlags{0x20});
    CORRADE_COMPARE(second.size(), (Containers::Size3D{1, 1, 15}));
    CORRADE_COMPARE(second.referenceCount(), 0);

    /* Two dimensions, DataLayerStorageHandle overloads */
    DummyStorage third{layer, {3, 2}, StorageFlags{0x08}};
    CORRADE_COMPARE(&third.layer(), &layer);
    CORRADE_COMPARE(third.handle(), Ui::storageHandle(layer.handle(), 2, 1));
    CORRADE_COMPARE(third, third.handle());
    CORRADE_VERIFY(layer.isHandleValid(third));
    CORRADE_COMPARE(layer.state(), LayerStates{});
    CORRADE_COMPARE(layer.storageCapacity(), 3);
    CORRADE_COMPARE(layer.storageUsedCount(), 3);
    CORRADE_VERIFY(!layer.isStorageDirty(storageHandleStorage(third)));
    CORRADE_COMPARE(layer.storageFlags(storageHandleStorage(third)), StorageFlags{0x08});
    CORRADE_COMPARE(layer.storageSize(storageHandleStorage(third)), (Containers::Size3D{1, 3, 2}));
    CORRADE_COMPARE(layer.storageReferenceCount(storageHandleStorage(third)), 0);
    CORRADE_COMPARE(&layer.storage<DummyStorage>(storageHandleStorage(third)).layer(), &layer);
    CORRADE_COMPARE(layer.storage<DummyStorage>(storageHandleStorage(third)).handle(), third.handle());
    CORRADE_VERIFY(!third.isDirty());
    CORRADE_COMPARE(third.flags(), StorageFlags{0x08});
    CORRADE_COMPARE(third.size(), (Containers::Size3D{1, 3, 2}));
    CORRADE_COMPARE(third.referenceCount(), 0);

    /* Three dimensions */
    DummyStorage fourth{layer, {15, 32, 11}, StorageFlags{0x10}};
    CORRADE_COMPARE(&fourth.layer(), &layer);
    CORRADE_COMPARE(fourth.handle(), Ui::storageHandle(layer.handle(), 3, 1));
    CORRADE_COMPARE(fourth, fourth.handle());
    CORRADE_VERIFY(layer.isHandleValid(fourth));
    CORRADE_COMPARE(layer.state(), LayerStates{});
    CORRADE_COMPARE(layer.storageCapacity(), 4);
    CORRADE_COMPARE(layer.storageUsedCount(), 4);
    CORRADE_VERIFY(!layer.isStorageDirty(fourth));
    CORRADE_COMPARE(layer.storageFlags(fourth), StorageFlags{0x10});
    CORRADE_COMPARE(layer.storageSize(fourth), (Containers::Size3D{15, 32, 11}));
    CORRADE_COMPARE(layer.storageReferenceCount(fourth), 0);
    CORRADE_COMPARE(&layer.storage<DummyStorage>(fourth).layer(), &layer);
    CORRADE_COMPARE(layer.storage<DummyStorage>(fourth).handle(), fourth.handle());
    CORRADE_VERIFY(!fourth.isDirty());
    CORRADE_COMPARE(fourth.flags(), StorageFlags{0x10});
    CORRADE_COMPARE(fourth.size(), (Containers::Size3D{15, 32, 11}));
    CORRADE_COMPARE(fourth.referenceCount(), 0);

    /* Removing a storage doesn't set any layer flags either */
    layer.removeStorage(first);
    CORRADE_VERIFY(!layer.isHandleValid(first));
    CORRADE_VERIFY(layer.isHandleValid(second));
    CORRADE_VERIFY(layer.isHandleValid(third));
    CORRADE_VERIFY(layer.isHandleValid(fourth));
    CORRADE_COMPARE(layer.state(), LayerStates{});
    CORRADE_COMPARE(layer.storageCapacity(), 4);
    CORRADE_COMPARE(layer.storageUsedCount(), 3);

    /* Using also the LayerDataHandle overload */
    layer.removeStorage(storageHandleStorage(third));
    CORRADE_VERIFY(!layer.isHandleValid(first));
    CORRADE_VERIFY(layer.isHandleValid(second));
    CORRADE_VERIFY(!layer.isHandleValid(third));
    CORRADE_VERIFY(layer.isHandleValid(fourth));
    CORRADE_COMPARE(layer.state(), LayerStates{});
    CORRADE_COMPARE(layer.storageCapacity(), 4);
    CORRADE_COMPARE(layer.storageUsedCount(), 2);
}

void DataLayerTest::createRemoveStorageInPlace() {
    /* Compared to createRemoveStorage() above, this tests only the DataLayer
       APIs that are affected by the in-place / allocated construction, not the
       rest or the helpers exposed on the AbstractStorage instance */

    struct InPlaceStorage: AbstractStorage {
        /* Fill the in-place storage with something to verify it won't
           accidentally be assumed to be zeroed out */
        explicit InPlaceStorage(DataLayer& layer, StorageFlags flags): AbstractStorage{layer, flags} {
            for(UnsignedInt& i: createInPlace<UnsignedInt>())
                i = 0xdeadbeef;
        }
        explicit InPlaceStorage(DataLayer& layer, std::size_t size, StorageFlags flags): AbstractStorage{layer, size, flags} {
            for(UnsignedInt& i: createInPlace<UnsignedInt>())
                i = 0xca7f00d5;
        }
        explicit InPlaceStorage(DataLayer& layer, const Containers::Size2D& size, StorageFlags flags): AbstractStorage{layer, size, flags} {
            for(UnsignedInt& i: createInPlace<UnsignedInt>())
                i = 0xcaffe7ea;
        }
        explicit InPlaceStorage(DataLayer& layer, const Containers::Size3D& size, StorageFlags flags): AbstractStorage{layer, size, flags} {
            for(UnsignedInt& i: createInPlace<UnsignedInt>())
                i = 0xfeedbeef;
        }

        /* Delibrately *not* using operator*() here, it should work even
           without it being present */
        UnsignedInt value() {
            return *data<UnsignedInt>();
        }
    };

    DataLayer layer{layerHandle(0, 1)};

    /* Zero dimensions. Verify that the internal allocated / in-place
       distinction doesn't leak to flags. Calling createInPlace() also
       shouldn't set any layer state flags. */
    InPlaceStorage first{layer, StorageFlags{0x18}};
    CORRADE_COMPARE(layer.state(), LayerStates{});
    CORRADE_COMPARE(layer.storageCapacity(), 1);
    CORRADE_COMPARE(layer.storageUsedCount(), 1);
    CORRADE_COMPARE(layer.storageUsedAllocatedCount(), 0);
    CORRADE_VERIFY(!layer.isStorageAllocated(first));
    CORRADE_VERIFY(!layer.isStorageDirty(first));
    CORRADE_COMPARE(layer.storageFlags(first), StorageFlags{0x18});
    CORRADE_COMPARE(layer.storageSize(first), (Containers::Size3D{1, 1, 1}));
    CORRADE_COMPARE(layer.storageReferenceCount(first), 0);
    CORRADE_VERIFY(!first.isAllocated());
    CORRADE_VERIFY(!first.isDirty());
    CORRADE_COMPARE(first.flags(), StorageFlags{0x18});
    CORRADE_COMPARE(first.size(), (Containers::Size3D{1, 1, 1}));
    CORRADE_COMPARE(first.referenceCount(), 0);
    CORRADE_COMPARE(first.value(), 0xdeadbeef);

    /* One dimension */
    InPlaceStorage second{layer, 15, StorageFlags{0x20}};
    CORRADE_COMPARE(layer.state(), LayerStates{});
    CORRADE_COMPARE(layer.storageCapacity(), 2);
    CORRADE_COMPARE(layer.storageUsedCount(), 2);
    CORRADE_COMPARE(layer.storageUsedAllocatedCount(), 0);
    CORRADE_VERIFY(!layer.isStorageAllocated(second));
    CORRADE_VERIFY(!layer.isStorageDirty(second));
    CORRADE_COMPARE(layer.storageFlags(second), StorageFlags{0x20});
    CORRADE_COMPARE(layer.storageSize(second), (Containers::Size3D{1, 1, 15}));
    CORRADE_COMPARE(layer.storageReferenceCount(second), 0);
    CORRADE_VERIFY(!second.isAllocated());
    CORRADE_VERIFY(!second.isDirty());
    CORRADE_COMPARE(second.flags(), StorageFlags{0x20});
    CORRADE_COMPARE(second.size(), (Containers::Size3D{1, 1, 15}));
    CORRADE_COMPARE(second.referenceCount(), 0);
    CORRADE_COMPARE(second.value(), 0xca7f00d5);

    /* Two dimensions, DataLayerStorageHandle overloads */
    InPlaceStorage third{layer, {3, 2}, StorageFlags{0x08}};
    CORRADE_COMPARE(layer.state(), LayerStates{});
    CORRADE_COMPARE(layer.storageCapacity(), 3);
    CORRADE_COMPARE(layer.storageUsedCount(), 3);
    CORRADE_COMPARE(layer.storageUsedAllocatedCount(), 0);
    CORRADE_VERIFY(!layer.isStorageAllocated(storageHandleStorage(third)));
    CORRADE_VERIFY(!layer.isStorageDirty(storageHandleStorage(third)));
    CORRADE_COMPARE(layer.storageFlags(storageHandleStorage(third)), StorageFlags{0x08});
    CORRADE_COMPARE(layer.storageSize(storageHandleStorage(third)), (Containers::Size3D{1, 3, 2}));
    CORRADE_COMPARE(layer.storageReferenceCount(storageHandleStorage(third)), 0);
    CORRADE_VERIFY(!third.isAllocated());
    CORRADE_VERIFY(!third.isDirty());
    CORRADE_COMPARE(third.flags(), StorageFlags{0x08});
    CORRADE_COMPARE(third.size(), (Containers::Size3D{1, 3, 2}));
    CORRADE_COMPARE(third.referenceCount(), 0);
    CORRADE_COMPARE(third.value(), 0xcaffe7ea);

    /* Three dimensions */
    InPlaceStorage fourth{layer, {15, 32, 11}, StorageFlags{0x10}};
    CORRADE_COMPARE(layer.state(), LayerStates{});
    CORRADE_COMPARE(layer.storageCapacity(), 4);
    CORRADE_COMPARE(layer.storageUsedCount(), 4);
    CORRADE_COMPARE(layer.storageUsedAllocatedCount(), 0);
    CORRADE_VERIFY(!layer.isStorageAllocated(fourth));
    CORRADE_VERIFY(!layer.isStorageDirty(fourth));
    CORRADE_COMPARE(layer.storageFlags(fourth), StorageFlags{0x10});
    CORRADE_COMPARE(layer.storageSize(fourth), (Containers::Size3D{15, 32, 11}));
    CORRADE_COMPARE(layer.storageReferenceCount(fourth), 0);
    CORRADE_VERIFY(!fourth.isAllocated());
    CORRADE_VERIFY(!fourth.isDirty());
    CORRADE_COMPARE(fourth.flags(), StorageFlags{0x10});
    CORRADE_COMPARE(fourth.size(), (Containers::Size3D{15, 32, 11}));
    CORRADE_COMPARE(fourth.referenceCount(), 0);
    CORRADE_COMPARE(fourth.value(), 0xfeedbeef);

    /* Removing a storage shouldn't attempt to call 0xdeadbeef as a deleter
       function pointer */
    layer.removeStorage(first);
    CORRADE_VERIFY(!layer.isHandleValid(first));
    CORRADE_VERIFY(layer.isHandleValid(second));
    CORRADE_VERIFY(layer.isHandleValid(third));
    CORRADE_VERIFY(layer.isHandleValid(fourth));
    CORRADE_COMPARE(layer.state(), LayerStates{});
    CORRADE_COMPARE(layer.storageCapacity(), 4);
    CORRADE_COMPARE(layer.storageUsedCount(), 3);
    CORRADE_COMPARE(layer.storageUsedAllocatedCount(), 0);

    /* Using also the LayerDataHandle overload */
    layer.removeStorage(storageHandleStorage(third));
    CORRADE_VERIFY(!layer.isHandleValid(first));
    CORRADE_VERIFY(layer.isHandleValid(second));
    CORRADE_VERIFY(!layer.isHandleValid(third));
    CORRADE_VERIFY(layer.isHandleValid(fourth));
    CORRADE_COMPARE(layer.state(), LayerStates{});
    CORRADE_COMPARE(layer.storageCapacity(), 4);
    CORRADE_COMPARE(layer.storageUsedCount(), 2);
    CORRADE_COMPARE(layer.storageUsedAllocatedCount(), 0);

    /* Destructing the whole layer instance shouldn't call a deleter on any
       in-place storages either */
    layer = DataLayer{layerHandle(0, 2)};
    CORRADE_COMPARE(layer.handle(), layerHandle(0, 2));
}

void DataLayerTest::createRemoveStorageAllocated() {
    /* Compared to createRemoveStorage() above, this tests only the DataLayer
       APIs that are affected by the in-place / allocated construction, not the
       convenience helpers exposed on the AbstractStorage instance */

    struct AllocatedStorage: AbstractStorage {
        explicit AllocatedStorage(DataLayer& layer, Int& destructed, StorageFlags flags): AbstractStorage{layer, flags} {
            createAllocated(&destructed, 1336, [](void* data, std::size_t size) {
                CORRADE_COMPARE(size, 1336);
                ++*static_cast<Int*>(data);
            });
        }
        explicit AllocatedStorage(DataLayer& layer, std::size_t size, Int& destructed, StorageFlags flags): AbstractStorage{layer, size, flags} {
            createAllocated(&destructed, 1337, [](void* data, std::size_t size) {
                CORRADE_COMPARE(size, 1337);
                ++*static_cast<Int*>(data);
            });
        }
        explicit AllocatedStorage(DataLayer& layer, const Containers::Size2D& size, Int& destructed, StorageFlags flags): AbstractStorage{layer, size, flags} {
            createAllocated(&destructed, 1338, [](void* data, std::size_t size) {
                CORRADE_COMPARE(size, 1338);
                ++*static_cast<Int*>(data);
            });
        }
        explicit AllocatedStorage(DataLayer& layer, const Containers::Size3D& size, Int& destructed, StorageFlags flags): AbstractStorage{layer, size, flags} {
            createAllocated(&destructed, 1339, [](void* data, std::size_t size) {
                CORRADE_COMPARE(size, 1339);
                ++*static_cast<Int*>(data);
            });
        }

        /* Delibrately *not* using operator*() here, it should work even
           without it being present. The value is actually a reference to the
           destructed variable, so we compare the address and not the value. */
        Int& value() {
            return *data<Int>();
        }
    };

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    DataLayer layer{layerHandle(0, 1)};

    /* Zero dimensions. Verify that the internal allocated / in-place
       distinction doesn't leak to flags. Calling createInPlace() also
       shouldn't set any layer state flags. */
    Int firstDestructed = 0;
    AllocatedStorage first{layer, firstDestructed, StorageFlags{0x18}};
    CORRADE_COMPARE(firstDestructed, 0);
    CORRADE_COMPARE(layer.state(), LayerStates{});
    CORRADE_COMPARE(layer.storageCapacity(), 1);
    CORRADE_COMPARE(layer.storageUsedCount(), 1);
    CORRADE_COMPARE(layer.storageUsedAllocatedCount(), 1);
    CORRADE_VERIFY(layer.isStorageAllocated(first));
    CORRADE_VERIFY(!layer.isStorageDirty(first));
    CORRADE_COMPARE(layer.storageFlags(first), StorageFlags{0x18});
    CORRADE_COMPARE(layer.storageSize(first), (Containers::Size3D{1, 1, 1}));
    CORRADE_COMPARE(layer.storageReferenceCount(first), 0);
    CORRADE_VERIFY(first.isAllocated());
    CORRADE_VERIFY(!first.isDirty());
    CORRADE_COMPARE(first.flags(), StorageFlags{0x18});
    CORRADE_COMPARE(first.size(), (Containers::Size3D{1, 1, 1}));
    CORRADE_COMPARE(first.referenceCount(), 0);
    CORRADE_COMPARE(&first.value(), &firstDestructed);

    /* One dimension. Growing the storage capacity shouldn't cause destructors
       to be called on existing storages. */
    Int secondDestructed = 0;
    AllocatedStorage second{layer, 15, secondDestructed, StorageFlags{0x20}};
    CORRADE_COMPARE(firstDestructed, 0);
    CORRADE_COMPARE(secondDestructed, 0);
    CORRADE_COMPARE(layer.state(), LayerStates{});
    CORRADE_COMPARE(layer.storageCapacity(), 2);
    CORRADE_COMPARE(layer.storageUsedCount(), 2);
    CORRADE_COMPARE(layer.storageUsedAllocatedCount(), 2);
    CORRADE_VERIFY(layer.isStorageAllocated(second));
    CORRADE_VERIFY(!layer.isStorageDirty(second));
    CORRADE_COMPARE(layer.storageFlags(second), StorageFlags{0x20});
    CORRADE_COMPARE(layer.storageSize(second), (Containers::Size3D{1, 1, 15}));
    CORRADE_COMPARE(layer.storageReferenceCount(second), 0);
    CORRADE_VERIFY(second.isAllocated());
    CORRADE_VERIFY(!second.isDirty());
    CORRADE_COMPARE(second.flags(), StorageFlags{0x20});
    CORRADE_COMPARE(second.size(), (Containers::Size3D{1, 1, 15}));
    CORRADE_COMPARE(second.referenceCount(), 0);
    CORRADE_COMPARE(&second.value(), &secondDestructed);

    /* Two dimensions, DataLayerStorageHandle overloads */
    Int thirdDestructed = 0;
    AllocatedStorage third{layer, {3, 2}, thirdDestructed, StorageFlags{0x08}};
    CORRADE_COMPARE(firstDestructed, 0);
    CORRADE_COMPARE(secondDestructed, 0);
    CORRADE_COMPARE(thirdDestructed, 0);
    CORRADE_COMPARE(layer.state(), LayerStates{});
    CORRADE_COMPARE(layer.storageCapacity(), 3);
    CORRADE_COMPARE(layer.storageUsedCount(), 3);
    CORRADE_COMPARE(layer.storageUsedAllocatedCount(), 3);
    CORRADE_VERIFY(layer.isStorageAllocated(storageHandleStorage(third)));
    CORRADE_VERIFY(!layer.isStorageDirty(storageHandleStorage(third)));
    CORRADE_COMPARE(layer.storageFlags(storageHandleStorage(third)), StorageFlags{0x08});
    CORRADE_COMPARE(layer.storageSize(storageHandleStorage(third)), (Containers::Size3D{1, 3, 2}));
    CORRADE_COMPARE(layer.storageReferenceCount(storageHandleStorage(third)), 0);
    CORRADE_VERIFY(third.isAllocated());
    CORRADE_VERIFY(!third.isDirty());
    CORRADE_COMPARE(third.flags(), StorageFlags{0x08});
    CORRADE_COMPARE(third.size(), (Containers::Size3D{1, 3, 2}));
    CORRADE_COMPARE(third.referenceCount(), 0);
    CORRADE_COMPARE(&third.value(), &thirdDestructed);

    /* Three dimensions */
    Int fourthDestructed = 0;
    AllocatedStorage fourth{layer, {15, 32, 11}, fourthDestructed, StorageFlags{0x10}};
    CORRADE_COMPARE(firstDestructed, 0);
    CORRADE_COMPARE(secondDestructed, 0);
    CORRADE_COMPARE(thirdDestructed, 0);
    CORRADE_COMPARE(fourthDestructed, 0);
    CORRADE_COMPARE(layer.state(), LayerStates{});
    CORRADE_COMPARE(layer.storageCapacity(), 4);
    CORRADE_COMPARE(layer.storageUsedCount(), 4);
    CORRADE_COMPARE(layer.storageUsedAllocatedCount(), 4);
    CORRADE_VERIFY(layer.isStorageAllocated(fourth));
    CORRADE_VERIFY(!layer.isStorageDirty(fourth));
    CORRADE_COMPARE(layer.storageFlags(fourth), StorageFlags{0x10});
    CORRADE_COMPARE(layer.storageSize(fourth), (Containers::Size3D{15, 32, 11}));
    CORRADE_COMPARE(layer.storageReferenceCount(fourth), 0);
    CORRADE_VERIFY(fourth.isAllocated());
    CORRADE_VERIFY(!fourth.isDirty());
    CORRADE_COMPARE(fourth.flags(), StorageFlags{0x10});
    CORRADE_COMPARE(fourth.size(), (Containers::Size3D{15, 32, 11}));
    CORRADE_COMPARE(fourth.referenceCount(), 0);
    CORRADE_COMPARE(&fourth.value(), &fourthDestructed);

    /* Removing a storage should call the deleter */
    layer.removeStorage(first);
    CORRADE_COMPARE(firstDestructed, 1);
    CORRADE_COMPARE(secondDestructed, 0);
    CORRADE_COMPARE(thirdDestructed, 0);
    CORRADE_COMPARE(fourthDestructed, 0);
    CORRADE_VERIFY(!layer.isHandleValid(first));
    CORRADE_VERIFY(layer.isHandleValid(second));
    CORRADE_VERIFY(layer.isHandleValid(third));
    CORRADE_VERIFY(layer.isHandleValid(fourth));
    CORRADE_COMPARE(layer.state(), LayerStates{});
    CORRADE_COMPARE(layer.storageCapacity(), 4);
    CORRADE_COMPARE(layer.storageUsedCount(), 3);
    CORRADE_COMPARE(layer.storageUsedAllocatedCount(), 3);

    /* Using also the LayerDataHandle overload */
    layer.removeStorage(storageHandleStorage(third));
    CORRADE_COMPARE(firstDestructed, 1);
    CORRADE_COMPARE(secondDestructed, 0);
    CORRADE_COMPARE(thirdDestructed, 1);
    CORRADE_COMPARE(fourthDestructed, 0);
    CORRADE_VERIFY(!layer.isHandleValid(first));
    CORRADE_VERIFY(layer.isHandleValid(second));
    CORRADE_VERIFY(!layer.isHandleValid(third));
    CORRADE_VERIFY(layer.isHandleValid(fourth));
    CORRADE_COMPARE(layer.state(), LayerStates{});
    CORRADE_COMPARE(layer.storageCapacity(), 4);
    CORRADE_COMPARE(layer.storageUsedCount(), 2);
    CORRADE_COMPARE(layer.storageUsedAllocatedCount(), 2);

    /* Destructing the whole layer instance should call a deleter on all
       remaining storages. And not on the ones already removed. */
    layer = DataLayer{layerHandle(0, 2)};
    CORRADE_COMPARE(layer.handle(), layerHandle(0, 2));
    CORRADE_COMPARE(firstDestructed, 1);
    CORRADE_COMPARE(secondDestructed, 1);
    CORRADE_COMPARE(thirdDestructed, 1);
    CORRADE_COMPARE(fourthDestructed, 1);
}

void DataLayerTest::createRemoveStorageHandleRecycle() {
    DataLayer layer{layerHandle(0xab, 0x12)};

    struct DummyStorage: AbstractStorage {
        explicit DummyStorage(DataLayer& layer, StorageFlags flags = {}): AbstractStorage{layer, flags} {}
        explicit DummyStorage(DataLayer& layer, const Containers::Size2D& size, StorageFlags flags = {}): AbstractStorage{layer, size, flags} {}
        explicit DummyStorage(DataLayer& layer, const Containers::Size3D& size, StorageFlags flags = {}): AbstractStorage{layer, size, flags} {}
    };

    struct DummyAllocatedStorage: AbstractStorage {
        explicit DummyAllocatedStorage(DataLayer& layer, std::size_t size, Int& destructed, StorageFlags flags = {}): AbstractStorage{layer, size, flags} {
            createAllocated(&destructed, 0xdead, [](void* data, std::size_t size) {
                CORRADE_COMPARE(size, 0xdead);
                ++*static_cast<Int*>(data);
            });
        }
        explicit DummyAllocatedStorage(DataLayer& layer, const Containers::Size2D& size, Int& destructed, StorageFlags flags = {}): AbstractStorage{layer, size, flags} {
            createAllocated(&destructed, 0xbeef, [](void* data, std::size_t size) {
                CORRADE_COMPARE(size, 0xbeef);
                ++*static_cast<Int*>(data);
            });
        }
        explicit DummyAllocatedStorage(DataLayer& layer, const Containers::Size3D& size, Int& destructed, StorageFlags flags = {}): AbstractStorage{layer, size, flags} {
            createAllocated(&destructed, 0xcafe, [](void* data, std::size_t size) {
                CORRADE_COMPARE(size, 0xcafe);
                ++*static_cast<Int*>(data);
            });
        }
    };

    Int secondDestructed = 0;
    Int thirdDestructed = 0;
    DummyStorage first{layer, StorageFlags{0x21}};
    DummyAllocatedStorage second{layer, {32, 15}, secondDestructed, StorageFlags{0x21}};
    DummyAllocatedStorage third{layer, 12, thirdDestructed, StorageFlags{0x21}};
    DummyStorage fourth{layer, {1, 4, 1}, StorageFlags{0x21}};
    CORRADE_COMPARE(first.handle(), Ui::storageHandle(layer.handle(), 0, 1));
    CORRADE_COMPARE(second.handle(), Ui::storageHandle(layer.handle(), 1, 1));
    CORRADE_COMPARE(third.handle(), Ui::storageHandle(layer.handle(), 2, 1));
    CORRADE_COMPARE(fourth.handle(), Ui::storageHandle(layer.handle(), 3, 1));
    CORRADE_COMPARE(secondDestructed, 0);
    CORRADE_COMPARE(thirdDestructed, 0);
    CORRADE_VERIFY(layer.isHandleValid(first));
    CORRADE_VERIFY(layer.isHandleValid(second));
    CORRADE_VERIFY(layer.isHandleValid(third));
    CORRADE_VERIFY(layer.isHandleValid(fourth));
    CORRADE_COMPARE(layer.storageCapacity(), 4);
    CORRADE_COMPARE(layer.storageUsedCount(), 4);
    CORRADE_COMPARE(layer.storageUsedAllocatedCount(), 2);
    CORRADE_VERIFY(!layer.isStorageAllocated(first));
    CORRADE_VERIFY(layer.isStorageAllocated(second));
    CORRADE_VERIFY(layer.isStorageAllocated(third));
    CORRADE_VERIFY(!layer.isStorageAllocated(fourth));
    CORRADE_VERIFY(!layer.isStorageDirty(first));
    CORRADE_VERIFY(!layer.isStorageDirty(second));
    CORRADE_VERIFY(!layer.isStorageDirty(third));
    CORRADE_VERIFY(!layer.isStorageDirty(fourth));
    CORRADE_COMPARE(layer.storageFlags(first), StorageFlags{0x21});
    CORRADE_COMPARE(layer.storageFlags(second), StorageFlags{0x21});
    CORRADE_COMPARE(layer.storageFlags(third), StorageFlags{0x21});
    CORRADE_COMPARE(layer.storageFlags(fourth), StorageFlags{0x21});
    CORRADE_COMPARE(layer.storageSize(first), (Containers::Size3D{1, 1, 1}));
    CORRADE_COMPARE(layer.storageSize(second), (Containers::Size3D{1, 32, 15}));
    CORRADE_COMPARE(layer.storageSize(third), (Containers::Size3D{1, 1, 12}));
    CORRADE_COMPARE(layer.storageSize(fourth), (Containers::Size3D{1, 4, 1}));

    /* Set some storages as dirty */
    layer.setStorageDirty(fourth);
    layer.setStorageDirty(first);
    CORRADE_VERIFY(layer.isStorageDirty(first));
    CORRADE_VERIFY(layer.isStorageDirty(fourth));

    /* Remove three out of the four in an arbitrary order */
    layer.removeStorage(fourth);
    layer.removeStorage(first);
    layer.removeStorage(third);
    CORRADE_COMPARE(secondDestructed, 0);
    CORRADE_COMPARE(thirdDestructed, 1);
    CORRADE_VERIFY(!layer.isHandleValid(first));
    CORRADE_VERIFY(layer.isHandleValid(second));
    CORRADE_VERIFY(!layer.isHandleValid(third));
    CORRADE_VERIFY(!layer.isHandleValid(fourth));
    CORRADE_COMPARE(layer.storageCapacity(), 4);
    CORRADE_COMPARE(layer.storageUsedCount(), 1);
    CORRADE_COMPARE(layer.storageUsedAllocatedCount(), 1);
    CORRADE_VERIFY(layer.isStorageAllocated(second));

    /* Handles crafted with a manually incremented generation (i.e., the
       generation that will be used next) shouldn't be reported as valid */
    StorageHandle firstNext = Ui::storageHandle(layer.handle(), storageHandleId(first), storageHandleGeneration(first) + 1);
    StorageHandle thirdNext = Ui::storageHandle(layer.handle(), storageHandleId(third), storageHandleGeneration(third) + 1);
    StorageHandle fourthNext = Ui::storageHandle(layer.handle(), storageHandleId(fourth), storageHandleGeneration(fourth) + 1);
    CORRADE_VERIFY(!layer.isHandleValid(firstNext));
    CORRADE_VERIFY(!layer.isHandleValid(thirdNext));
    CORRADE_VERIFY(!layer.isHandleValid(fourthNext));

    /* Allocating new handles should recycle the handles in the order they were
       removed (oldest first). They should be the same as the handles crafted
       above which should report as valid now. Their properties, including
       flags and dirty bits, should be cleared. */
    Int fourth2Destructed = 0;
    DummyAllocatedStorage fourth2{layer, 31, fourth2Destructed};
    DummyStorage first2{layer, {2, 2}};
    DummyStorage third2{layer};
    CORRADE_COMPARE(first2.handle(), Ui::storageHandle(layer.handle(), 0, 2));
    CORRADE_COMPARE(third2.handle(), Ui::storageHandle(layer.handle(), 2, 2));
    CORRADE_COMPARE(fourth2.handle(), Ui::storageHandle(layer.handle(), 3, 2));
    CORRADE_COMPARE(first2, firstNext);
    CORRADE_COMPARE(third2, thirdNext);
    CORRADE_COMPARE(fourth2, fourthNext);
    CORRADE_COMPARE(secondDestructed, 0);
    CORRADE_COMPARE(thirdDestructed, 1);
    CORRADE_COMPARE(fourth2Destructed, 0);
    CORRADE_VERIFY(layer.isHandleValid(firstNext));
    CORRADE_VERIFY(layer.isHandleValid(thirdNext));
    CORRADE_VERIFY(layer.isHandleValid(fourthNext));
    CORRADE_COMPARE(layer.storageCapacity(), 4);
    CORRADE_COMPARE(layer.storageUsedCount(), 4);
    CORRADE_COMPARE(layer.storageUsedAllocatedCount(), 2);
    /* The first wasn't allocated before and isn't now either */
    CORRADE_VERIFY(!layer.isStorageAllocated(first2));
    CORRADE_VERIFY(layer.isStorageAllocated(second));
    /* The third was allocated before and isn't now */
    CORRADE_VERIFY(!layer.isStorageAllocated(third2));
    /* The fourth wasn't allocated before and is now */
    CORRADE_VERIFY(layer.isStorageAllocated(fourth2));
    /* Dirty bits are all reset */
    CORRADE_VERIFY(!layer.isStorageDirty(first2));
    CORRADE_VERIFY(!layer.isStorageDirty(second));
    CORRADE_VERIFY(!layer.isStorageDirty(third2));
    CORRADE_VERIFY(!layer.isStorageDirty(fourth2));
    /* The first used to have implicit size, now it doesn't */
    CORRADE_COMPARE(layer.storageSize(first2), (Containers::Size3D{1, 2, 2}));
    CORRADE_COMPARE(layer.storageSize(second), (Containers::Size3D{1, 32, 15}));
    /* The third used to have explicit size, now it has implicit */
    CORRADE_COMPARE(layer.storageSize(third2), (Containers::Size3D{1, 1, 1}));
    CORRADE_COMPARE(layer.storageSize(fourth2), (Containers::Size3D{1, 1, 31}));

    /* Old handles shouldn't get valid again */
    CORRADE_VERIFY(!layer.isHandleValid(first));
    CORRADE_VERIFY(layer.isHandleValid(first2));
    CORRADE_VERIFY(!layer.isHandleValid(third));
    CORRADE_VERIFY(layer.isHandleValid(third2));
    CORRADE_VERIFY(!layer.isHandleValid(fourth));
    CORRADE_VERIFY(layer.isHandleValid(fourth2));

    /* Removing a single handle and creating a new one directly reuses it if
       there's just one in the free list */
    layer.removeStorage(third2);
    Int third3Destructed = 0;
    DummyAllocatedStorage third3{layer, {1, 2, 3}, third3Destructed};
    CORRADE_COMPARE(third3.handle(), Ui::storageHandle(layer.handle(), 2, 3));
    CORRADE_COMPARE(secondDestructed, 0);
    CORRADE_COMPARE(thirdDestructed, 1);
    CORRADE_COMPARE(third3Destructed, 0);
    CORRADE_COMPARE(fourth2Destructed, 0);
    CORRADE_VERIFY(!layer.isHandleValid(third));
    CORRADE_VERIFY(!layer.isHandleValid(third2));
    CORRADE_VERIFY(layer.isHandleValid(third3));
    CORRADE_COMPARE(layer.storageCapacity(), 4);
    CORRADE_COMPARE(layer.storageUsedCount(), 4);
    CORRADE_COMPARE(layer.storageUsedAllocatedCount(), 3);
    CORRADE_VERIFY(layer.isStorageAllocated(third3));
    CORRADE_COMPARE(layer.storageSize(third3), (Containers::Size3D{1, 2, 3}));

    /* Allocating a new handle with the free list empty will grow it. It
       shouldn't do anything to existing allocations. */
    DummyStorage fifth{layer};
    CORRADE_COMPARE(fifth.handle(), Ui::storageHandle(layer.handle(), 4, 1));
    CORRADE_COMPARE(secondDestructed, 0);
    CORRADE_COMPARE(thirdDestructed, 1);
    CORRADE_COMPARE(third3Destructed, 0);
    CORRADE_COMPARE(fourth2Destructed, 0);
    CORRADE_VERIFY(layer.isHandleValid(fifth));
    CORRADE_COMPARE(layer.storageCapacity(), 5);
    CORRADE_COMPARE(layer.storageUsedCount(), 5);
    CORRADE_COMPARE(layer.storageUsedAllocatedCount(), 3);
    CORRADE_VERIFY(!layer.isStorageAllocated(fifth));
    CORRADE_COMPARE(layer.storageSize(fifth), (Containers::Size3D{1, 1, 1}));

    /* The generation counter view should reflect the number of how much was
       given ID recycled */
    CORRADE_COMPARE_AS(layer.storageGenerations(), Containers::arrayView<UnsignedShort>({
        2,
        1,
        3,
        2,
        1
    }), TestSuite::Compare::Container);

    /* Destructing the layer should call deleters on all remaining allocated
       storages */
    layer = DataLayer{layerHandle(0xcd, 0x23)};
    CORRADE_COMPARE(layer.handle(), layerHandle(0xcd, 0x23));
    CORRADE_COMPARE(secondDestructed, 1);
    CORRADE_COMPARE(thirdDestructed, 1);
    CORRADE_COMPARE(third3Destructed, 1);
    CORRADE_COMPARE(fourth2Destructed, 1);
}

void DataLayerTest::createRemoveStorageHandleDisable() {
    struct DummyStorage: AbstractStorage {
        explicit DummyStorage(DataLayer& layer): AbstractStorage{layer} {}
    };

    DataLayer layer{layerHandle(0xab, 0x12)};

    DummyStorage first{layer};
    CORRADE_COMPARE(first.handle(), Ui::storageHandle(layer.handle(), 0, 1));

    for(std::size_t i = 0; i != (1 << Implementation::DataLayerStorageHandleGenerationBits) - 1; ++i) {
        DummyStorage second{layer};
        CORRADE_COMPARE(second.handle(), Ui::storageHandle(layer.handle(), 1, 1 + i));
        layer.removeStorage(second);
    }

    /* The generation for the second slot is exhausted so the handle is not
       recycled */
    CORRADE_COMPARE(layer.storageCapacity(), 2);
    CORRADE_COMPARE(layer.storageUsedCount(), 2);

    /* It shouldn't think a handle from the second slot with generation 0 is
       valid */
    CORRADE_VERIFY(!layer.isHandleValid(Ui::storageHandle(layer.handle(), 1, 0)));

    /* There's nowhere to create a new handle from so the capacity is grown */
    DummyStorage third{layer};
    CORRADE_COMPARE(third.handle(), Ui::storageHandle(layer.handle(), 2, 1));
    CORRADE_COMPARE(layer.storageCapacity(), 3);
    CORRADE_COMPARE(layer.storageUsedCount(), 3);

    /* The generation counter view should have 0 for the disabled slot */
    CORRADE_COMPARE_AS(layer.storageGenerations(), Containers::arrayView<UnsignedShort>({
        1,
        0,
        1
    }), TestSuite::Compare::Container);
}

void DataLayerTest::createStorageNoHandlesLeft() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct DummyStorage: AbstractStorage {
        explicit DummyStorage(DataLayer& layer): AbstractStorage{layer} {}
    };

    DataLayer layer{layerHandle(0, 1)};

    for(std::size_t i = 0; i != 1 << Implementation::DataLayerStorageHandleIdBits; ++i)
        DummyStorage{layer};

    CORRADE_COMPARE(layer.storageCapacity(), 1 << Implementation::DataLayerStorageHandleIdBits);
    CORRADE_COMPARE(layer.storageUsedCount(), 1 << Implementation::DataLayerStorageHandleIdBits);

    Containers::String out;
    Error redirectError{&out};
    DummyStorage{layer};
    /* Number is hardcoded in the expected message but not elsewhere in order
       to give a heads-up when modifying the handle ID bit count */
    CORRADE_COMPARE(out,
        "Ui::AbstractStorage: can only have at most 1048576 storages\n");
}

void DataLayerTest::createStorageInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    DataLayer layer{layerHandle(0, 1)};

    struct CustomStorage: AbstractStorage {
        explicit CustomStorage(DataLayer& layer, std::size_t size): AbstractStorage{layer, size} {}
        explicit CustomStorage(DataLayer& layer, const Containers::Size2D& size): AbstractStorage{layer, size} {}
        explicit CustomStorage(DataLayer& layer, const Containers::Size3D& size): AbstractStorage{layer, size} {}
        explicit CustomStorage(DataLayer& layer, void* data, std::size_t dataSize, void(*deleter)(void*, std::size_t)): AbstractStorage{layer} {
            createAllocated(data, dataSize, deleter);
        }

        using AbstractStorage::data;
    };

    char data[1];

    /* Zero data size is okay, such as when using the default new/delete, where
       it's not needed for anything */
    CustomStorage{layer, data, 0, [](void*, std::size_t) {}};

    Containers::String out;
    Error redirectError{&out};
    CustomStorage{layer, 0};
    CustomStorage{layer, {1, 0}};
    CustomStorage{layer, {1, 1, 0}};
    CustomStorage{layer, {0, 1}};
    CustomStorage{layer, {1, 0, 1}};
    CustomStorage{layer, {0, 1, 1}};
    CustomStorage{layer, nullptr, 1, [](void*, std::size_t) {}};
    CustomStorage{layer, data, 1, nullptr};
    CORRADE_COMPARE_AS(out,
        "Ui::AbstractStorage: expected non-zero size but got {1, 1, 0}\n"
        "Ui::AbstractStorage: expected non-zero size but got {1, 1, 0}\n"
        "Ui::AbstractStorage: expected non-zero size but got {1, 1, 0}\n"
        "Ui::AbstractStorage: expected non-zero size but got {1, 0, 1}\n"
        "Ui::AbstractStorage: expected non-zero size but got {1, 0, 1}\n"
        "Ui::AbstractStorage: expected non-zero size but got {0, 1, 1}\n"
        "Ui::AbstractStorage::createAllocated(): data is null\n"
        "Ui::AbstractStorage::createAllocated(): deleter is null\n",
        TestSuite::Compare::String);
}

#ifndef CORRADE_TARGET_32BIT
void DataLayerTest::createStorageTooLarge() {
    CORRADE_SKIP_IF_NO_ASSERT();

    DataLayer layer{layerHandle(0, 1)};

    struct CustomStorage: AbstractStorage {
        explicit CustomStorage(DataLayer& layer, std::size_t size): AbstractStorage{layer, size} {}
        explicit CustomStorage(DataLayer& layer, const Containers::Size2D& size): AbstractStorage{layer, size} {}
        explicit CustomStorage(DataLayer& layer, const Containers::Size3D& size): AbstractStorage{layer, size} {}

        using AbstractStorage::data;
    };

    /* Size that fits exactly into 43 bits is okay (well a 1 bit and then 43
       zero bits but you know what I mean, right? 0x80000000000 in hex.) */
    CustomStorage{layer, std::size_t{1} << 43};

    Containers::String out;
    Error redirectError{&out};
    CustomStorage{layer, (std::size_t{1} << 43) + 1};
    CustomStorage{layer, {1, (std::size_t{1} << 43) + 1}};
    CustomStorage{layer, {(std::size_t{1} << 43) + 1, 1}};
    CustomStorage{layer, {1, 1, (std::size_t{1} << 43) + 1}};
    CustomStorage{layer, {1, (std::size_t{1} << 43) + 1, 1}};
    CustomStorage{layer, {(std::size_t{1} << 43) + 1, 1, 1}};
    /* These can't make it exactly "one more than 43 bits" but it verifies that
       it actually makes a multiplication of all dimensions rather than making
       a max() or whatever */
    CustomStorage{layer, {(std::size_t{1} << 21) + 1, std::size_t{1} << 22}};
    CustomStorage{layer, {std::size_t{1} << 14, (std::size_t{1} << 14) + 1, std::size_t{1} << 15}};
    CORRADE_COMPARE_AS(out,
        "Ui::AbstractStorage: expected size to fit into 43 bits but got 8796093022209\n"
        "Ui::AbstractStorage: expected size to fit into 43 bits but got 8796093022209\n"
        "Ui::AbstractStorage: expected size to fit into 43 bits but got 8796093022209\n"
        "Ui::AbstractStorage: expected size to fit into 43 bits but got 8796093022209\n"
        "Ui::AbstractStorage: expected size to fit into 43 bits but got 8796093022209\n"
        "Ui::AbstractStorage: expected size to fit into 43 bits but got 8796093022209\n"

        "Ui::AbstractStorage: expected size to fit into 43 bits but got 8796097216512\n"
        "Ui::AbstractStorage: expected size to fit into 43 bits but got 8796629893120\n",
        TestSuite::Compare::String);
}
#endif

void DataLayerTest::removeStorageInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct DummyStorage: AbstractStorage {
        explicit DummyStorage(DataLayer& layer, StorageFlags flags = {}): AbstractStorage{layer, flags} {}

        Int operator*() const { return 3; }
    };

    DataLayer layer{layerHandle(0xab, 0x12)};

    DummyStorage storage{layer};
    layer.create(storage, [](Int) {});
    layer.create(storage, [](Int) {});
    layer.create(storage, [](Int) {});
    CORRADE_COMPARE(storage.referenceCount(), 3);

    /* Reference-counted storage shouldn't behave any different, it should also
       disallow removal while used */
    DummyStorage storageReferenceCounted{layer, StorageFlag::ReferenceCounted};
    layer.create(storageReferenceCounted, [](Int) {});
    layer.create(storageReferenceCounted, [](Int) {});
    CORRADE_COMPARE(storageReferenceCounted.referenceCount(), 2);

    Containers::String out;
    Error redirectError{&out};
    layer.removeStorage(StorageHandle::Null);
    /* Valid layer, invalid storage */
    layer.removeStorage(Ui::storageHandle(layer.handle(), DataLayerStorageHandle(0x345cdef0)));
    /* Invalid layer, valid storage */
    layer.removeStorage(Ui::storageHandle(LayerHandle::Null, storageHandleStorage(storage.handle())));
    /* DataLayerStorageHandle directly */
    layer.removeStorage(DataLayerStorageHandle(0x123abcde));
    /* Removing while in use */
    layer.removeStorage(storage);
    layer.removeStorage(storageHandleStorage(storage));
    layer.removeStorage(storageReferenceCounted);
    layer.removeStorage(storageHandleStorage(storageReferenceCounted));
    CORRADE_COMPARE_AS(out,
        "Ui::DataLayer::removeStorage(): invalid handle Ui::StorageHandle::Null\n"
        "Ui::DataLayer::removeStorage(): invalid handle Ui::StorageHandle({0xab, 0x12}, {0xcdef0, 0x345})\n"
        "Ui::DataLayer::removeStorage(): invalid handle Ui::StorageHandle(Null, {0x0, 0x1})\n"
        "Ui::DataLayer::removeStorage(): invalid handle Ui::DataLayerStorageHandle(0xabcde, 0x123)\n"

        "Ui::DataLayer::removeStorage(): Ui::StorageHandle({0xab, 0x12}, {0x0, 0x1}) referenced by 3 data\n"
        "Ui::DataLayer::removeStorage(): Ui::DataLayerStorageHandle(0x0, 0x1) referenced by 3 data\n"
        "Ui::DataLayer::removeStorage(): Ui::StorageHandle({0xab, 0x12}, {0x1, 0x1}) referenced by 2 data\n"
        "Ui::DataLayer::removeStorage(): Ui::DataLayerStorageHandle(0x1, 0x1) referenced by 2 data\n",
        TestSuite::Compare::String);
}

void DataLayerTest::setStorageDirty() {
    struct DummyStorage: AbstractStorage {
        explicit DummyStorage(DataLayer& layer, StorageFlags flags): AbstractStorage{layer, flags} {}

        Int operator*() const { return {}; }
    };

    DataLayer layer{layerHandle(0, 1)};

    /* Create a dummy storage to verify it doesn't always update the first */
    DummyStorage{layer, {}};

    /* By default the storage shouldn't be dirty and no layer state is set
       either, regardless of whether is used or not */
    DummyStorage storageUnused{layer, StorageFlags{0x18}};
    DummyStorage storage{layer, StorageFlags{0x18}};
    CORRADE_VERIFY(!layer.isStorageDirty(storageUnused));
    CORRADE_VERIFY(!layer.isStorageDirty(storage));
    CORRADE_COMPARE(layer.storageFlags(storageUnused), StorageFlags{0x18});
    CORRADE_COMPARE(layer.storageFlags(storage), StorageFlags{0x18});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Create a data on one storage to make it actually used, which sets a
       layer state */
    layer.create(storage, [](Int) {});
    CORRADE_COMPARE(layer.storageReferenceCount(storageUnused), 0);
    CORRADE_COMPARE(layer.storageReferenceCount(storage), 1);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate);

    /* Call preUpdate() + update() to reset the LayerState. From the outside it
       doesn't really matter which of the two does it, so call both. This
       shouldn't change anything about the storages. */
    layer.preUpdate(LayerState::NeedsCommonDataUpdate);
    layer.update(LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_VERIFY(!layer.isStorageDirty(storageUnused));
    CORRADE_VERIFY(!layer.isStorageDirty(storage));
    CORRADE_COMPARE(layer.storageFlags(storageUnused), StorageFlags{0x18});
    CORRADE_COMPARE(layer.storageFlags(storage), StorageFlags{0x18});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Marking an unused storage as dirty causes its dirty bit to be set but
       no layer state as it doesn't affect any data. The dirty bit is stored
       inside the flags but shouldn't leak to them in any way. */
    layer.setStorageDirty(storageUnused);
    CORRADE_VERIFY(layer.isStorageDirty(storageUnused));
    CORRADE_COMPARE(layer.storageFlags(storageUnused), StorageFlags{0x18});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Doing it again results in the same outcome, no LayerState set either */
    layer.setStorageDirty(storageUnused);
    CORRADE_VERIFY(layer.isStorageDirty(storageUnused));
    CORRADE_COMPARE(layer.storageFlags(storageUnused), StorageFlags{0x18});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Marking it as dirty causes the bit to be set along with layer state.
       It's stored inside the flags but shouldn't leak to them in any way. */
    layer.setStorageDirty(storage);
    CORRADE_VERIFY(layer.isStorageDirty(storage));
    CORRADE_COMPARE(layer.storageFlags(storage), StorageFlags{0x18});
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);

    /* Marking it again does the same operation -- but since all bits and
       states are already set, it's effectively a no-op */
    layer.setStorageDirty(storage);
    CORRADE_VERIFY(layer.isStorageDirty(storage));
    CORRADE_COMPARE(layer.storageFlags(storage), StorageFlags{0x18});
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);

    /* Calling preUpdate() + update() resets the dirty bit on all storages,
       even the unused, and the state. From the outside it doesn't really
       matter which of the two does it, so call both. */
    layer.preUpdate(LayerState::NeedsCommonDataUpdate);
    layer.update(LayerState::NeedsCommonDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_VERIFY(!layer.isStorageDirty(storageUnused));
    CORRADE_VERIFY(!layer.isStorageDirty(storage));
    CORRADE_COMPARE(layer.storageFlags(storage), StorageFlags{0x18});
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Mark the storage as dirty again, DataLayerStorageHandle overload */
    layer.setStorageDirty(storageHandleStorage(storage));
    CORRADE_VERIFY(layer.isStorageDirty(storageHandleStorage(storage)));
    CORRADE_COMPARE(layer.storageFlags(storageHandleStorage(storage)), StorageFlags{0x18});
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);

    /* Effect of the storage being dirty on actual data update functions being
       called is tested in update() below */
}

void DataLayerTest::invalidStorageHandle() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct DummyStorage: AbstractStorage {
        explicit DummyStorage(DataLayer& layer): AbstractStorage{layer} {}

        using AbstractStorage::data;
    };

    DataLayer layer{layerHandle(0, 1)};

    StorageHandle handle = DummyStorage{layer};

    DummyStorage removed{layer};
    layer.removeStorage(removed);

    Containers::String out;
    Error redirectError{&out};
    layer.isStorageAllocated(StorageHandle::Null);
    layer.isStorageDirty(StorageHandle::Null);
    layer.setStorageDirty(StorageHandle::Null);
    layer.storageFlags(StorageHandle::Null);
    layer.storageSize(StorageHandle::Null);
    layer.storageReferenceCount(StorageHandle::Null);
    layer.storage<DummyStorage>(StorageHandle::Null);
    /* Valid layer, invalid storage */
    layer.isStorageAllocated(Ui::storageHandle(layer.handle(), DataLayerStorageHandle(0x123abcde)));
    layer.isStorageDirty(Ui::storageHandle(layer.handle(), DataLayerStorageHandle(0x123abcde)));
    layer.setStorageDirty(Ui::storageHandle(layer.handle(), DataLayerStorageHandle(0x123abcde)));
    layer.storageFlags(Ui::storageHandle(layer.handle(), DataLayerStorageHandle(0x123abcde)));
    layer.storageSize(Ui::storageHandle(layer.handle(), DataLayerStorageHandle(0x123abcde)));
    layer.storageReferenceCount(Ui::storageHandle(layer.handle(), DataLayerStorageHandle(0x123abcde)));
    layer.storage<DummyStorage>(Ui::storageHandle(layer.handle(), DataLayerStorageHandle(0x123abcde)));
    /* Invalid layer, valid storage */
    layer.isStorageAllocated(Ui::storageHandle(LayerHandle::Null, storageHandleStorage(handle)));
    layer.isStorageDirty(Ui::storageHandle(LayerHandle::Null, storageHandleStorage(handle)));
    layer.setStorageDirty(Ui::storageHandle(LayerHandle::Null, storageHandleStorage(handle)));
    layer.storageFlags(Ui::storageHandle(LayerHandle::Null, storageHandleStorage(handle)));
    layer.storageSize(Ui::storageHandle(LayerHandle::Null, storageHandleStorage(handle)));
    layer.storageReferenceCount(Ui::storageHandle(LayerHandle::Null, storageHandleStorage(handle)));
    layer.storage<DummyStorage>(Ui::storageHandle(LayerHandle::Null, storageHandleStorage(handle)));
    /* DataLayerStorageHandle directly */
    layer.isStorageAllocated(DataLayerStorageHandle(0x123abcde));
    layer.isStorageDirty(DataLayerStorageHandle(0x123abcde));
    layer.setStorageDirty(DataLayerStorageHandle(0x123abcde));
    layer.storageFlags(DataLayerStorageHandle(0x123abcde));
    layer.storageSize(DataLayerStorageHandle(0x123abcde));
    layer.storageReferenceCount(DataLayerStorageHandle(0x123abcde));
    layer.storage<DummyStorage>(DataLayerStorageHandle(0x123abcde));
    /* AbstractStorage APIs, called on an instance that's already removed.
       These just delegate to DataLayer APIs so they should reuse the same
       DataLayerStorageHandle assertions. */
    removed.isAllocated();
    removed.isDirty();
    removed.setDirty();
    removed.flags();
    removed.size();
    removed.referenceCount();
    /* This one doesn't have any equivalent DataLayer API so it prints the
       AbstractStorage API name */
    removed.data<Int>();
    CORRADE_COMPARE_AS(out,
        "Ui::DataLayer::isStorageAllocated(): invalid handle Ui::StorageHandle::Null\n"
        "Ui::DataLayer::isStorageDirty(): invalid handle Ui::StorageHandle::Null\n"
        "Ui::DataLayer::setStorageDirty(): invalid handle Ui::StorageHandle::Null\n"
        "Ui::DataLayer::storageFlags(): invalid handle Ui::StorageHandle::Null\n"
        "Ui::DataLayer::storageSize(): invalid handle Ui::StorageHandle::Null\n"
        "Ui::DataLayer::storageReferenceCount(): invalid handle Ui::StorageHandle::Null\n"
        "Ui::DataLayer::storage(): invalid handle Ui::StorageHandle::Null\n"

        "Ui::DataLayer::isStorageAllocated(): invalid handle Ui::StorageHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Ui::DataLayer::isStorageDirty(): invalid handle Ui::StorageHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Ui::DataLayer::setStorageDirty(): invalid handle Ui::StorageHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Ui::DataLayer::storageFlags(): invalid handle Ui::StorageHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Ui::DataLayer::storageSize(): invalid handle Ui::StorageHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Ui::DataLayer::storageReferenceCount(): invalid handle Ui::StorageHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Ui::DataLayer::storage(): invalid handle Ui::StorageHandle({0x0, 0x1}, {0xabcde, 0x123})\n"

        "Ui::DataLayer::isStorageAllocated(): invalid handle Ui::StorageHandle(Null, {0x0, 0x1})\n"
        "Ui::DataLayer::isStorageDirty(): invalid handle Ui::StorageHandle(Null, {0x0, 0x1})\n"
        "Ui::DataLayer::setStorageDirty(): invalid handle Ui::StorageHandle(Null, {0x0, 0x1})\n"
        "Ui::DataLayer::storageFlags(): invalid handle Ui::StorageHandle(Null, {0x0, 0x1})\n"
        "Ui::DataLayer::storageSize(): invalid handle Ui::StorageHandle(Null, {0x0, 0x1})\n"
        "Ui::DataLayer::storageReferenceCount(): invalid handle Ui::StorageHandle(Null, {0x0, 0x1})\n"
        "Ui::DataLayer::storage(): invalid handle Ui::StorageHandle(Null, {0x0, 0x1})\n"

        "Ui::DataLayer::isStorageAllocated(): invalid handle Ui::DataLayerStorageHandle(0xabcde, 0x123)\n"
        "Ui::DataLayer::isStorageDirty(): invalid handle Ui::DataLayerStorageHandle(0xabcde, 0x123)\n"
        "Ui::DataLayer::setStorageDirty(): invalid handle Ui::DataLayerStorageHandle(0xabcde, 0x123)\n"
        "Ui::DataLayer::storageFlags(): invalid handle Ui::DataLayerStorageHandle(0xabcde, 0x123)\n"
        "Ui::DataLayer::storageSize(): invalid handle Ui::DataLayerStorageHandle(0xabcde, 0x123)\n"
        "Ui::DataLayer::storageReferenceCount(): invalid handle Ui::DataLayerStorageHandle(0xabcde, 0x123)\n"
        "Ui::DataLayer::storage(): invalid handle Ui::DataLayerStorageHandle(0xabcde, 0x123)\n"

        "Ui::DataLayer::isStorageAllocated(): invalid handle Ui::DataLayerStorageHandle(0x1, 0x1)\n"
        "Ui::DataLayer::isStorageDirty(): invalid handle Ui::DataLayerStorageHandle(0x1, 0x1)\n"
        "Ui::DataLayer::setStorageDirty(): invalid handle Ui::DataLayerStorageHandle(0x1, 0x1)\n"
        "Ui::DataLayer::storageFlags(): invalid handle Ui::DataLayerStorageHandle(0x1, 0x1)\n"
        "Ui::DataLayer::storageSize(): invalid handle Ui::DataLayerStorageHandle(0x1, 0x1)\n"
        "Ui::DataLayer::storageReferenceCount(): invalid handle Ui::DataLayerStorageHandle(0x1, 0x1)\n"
        "Ui::AbstractStorage::data(): invalid handle Ui::DataLayerStorageHandle(0x1, 0x1)\n",
        TestSuite::Compare::String);
}

Int storageCalled = 0;
Int storage1DCalled = 0;
Int storage2DCalled = 0;
Int storage3DCalled = 0;

void DataLayerTest::createRemoveSetupTeardown() {
    storageCalled =
        storage1DCalled =
            storage2DCalled =
                storage3DCalled = 0;
}

void DataLayerTest::createRemove() {
    /* Tests mainly data creation & removal along with correct destruction for
       non-trivial functions, with the 0D/1D/2D/3D overloads. Variants taking a
       member (function) pointer are tested in createMember*() below. */

    /* Dedicated storage type per dimension count, overloads tested in
       createOverloads() below. The getters get called below to verify the
       member function is correctly picked and called with the right
       arguments */
    struct DummyStorage: AbstractStorage {
        explicit DummyStorage(DataLayer& layer): AbstractStorage{layer} {}

        Int operator*() const {
            ++storageCalled;
            return 0x333;
        }
    };
    struct DummyStorage1D: AbstractStorage {
        explicit DummyStorage1D(DataLayer& layer): AbstractStorage{layer, 15} {}

        Int operator[](std::size_t index) const {
            CORRADE_COMPARE(index, 13);
            ++storage1DCalled;
            return 0x4444;
        }
    };
    struct DummyStorage2D: AbstractStorage {
        explicit DummyStorage2D(DataLayer& layer): AbstractStorage{layer, {3, 7}} {}

        Int operator[](const Containers::Size2D& index) const {
            CORRADE_COMPARE(index, (Containers::Size2D{2, 5}));
            ++storage2DCalled;
            return 0x55555;
        }
    };
    struct DummyStorage3D: AbstractStorage {
        explicit DummyStorage3D(DataLayer& layer): AbstractStorage{layer, {2, 5, 4}} {}

        Int operator[](const Containers::Size3D& index) const {
            CORRADE_COMPARE(index, (Containers::Size3D{1, 3, 2}));
            ++storage3DCalled;
            return 0x666666;
        }
    };

    struct NonTrivial {
        explicit NonTrivial(Int expected, Int& called, Int& destructed): expected{expected}, called{&called}, destructed{&destructed} {}
        ~NonTrivial() {
            ++*destructed;
        }
        void operator()(Int value) const {
            CORRADE_COMPARE(value, expected);
            ++*called;
        }

        Int expected;
        Int* called;
        Int* destructed;
    };

    DataLayer layer{layerHandle(0, 1)};

    /* Create some extra storages to verify it's correctly rebuilding even
       non-trivial storage handles from the ID */
    DummyStorage{layer};
    DummyStorage{layer};
    layer.removeStorage(DummyStorage{layer});
    layer.removeStorage(DummyStorage{layer});
    layer.removeStorage(DummyStorage{layer});
    layer.removeStorage(DummyStorage{layer});
    DummyStorage storage{layer};
    layer.removeStorage(DummyStorage{layer});
    DummyStorage1D storage1D{layer};
    layer.removeStorage(DummyStorage{layer});
    layer.removeStorage(DummyStorage{layer});
    DummyStorage2D storage2D{layer};
    layer.removeStorage(DummyStorage{layer});
    layer.removeStorage(DummyStorage{layer});
    layer.removeStorage(DummyStorage{layer});
    DummyStorage3D storage3D{layer};
    CORRADE_COMPARE(layer.storageCapacity(), 6);
    CORRADE_COMPARE(layer.storageUsedCount(), 6);
    CORRADE_COMPARE(layer.storageUsedAllocatedCount(), 0);
    CORRADE_COMPARE(layer.capacity(), 0);
    CORRADE_COMPARE(layer.usedCount(), 0);
    CORRADE_COMPARE(layer.usedAllocatedCount(), 0);
    CORRADE_COMPARE(layer.storageReferenceCount(storage), 0);
    CORRADE_COMPARE(layer.storageReferenceCount(storage1D), 0);
    CORRADE_COMPARE(layer.storageReferenceCount(storage2D), 0);
    CORRADE_COMPARE(layer.storageReferenceCount(storage3D), 0);
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Trivial function, single item */
    Int trivialCalled = 0;
    DataHandle trivial = layer.create(storage, [&trivialCalled](Int value) {
        CORRADE_COMPARE(value, 0x333);
        ++trivialCalled;
    });
    CORRADE_COMPARE(layer.storageReferenceCount(storage), 1);
    CORRADE_COMPARE(layer.storageReferenceCount(storage1D), 0);
    CORRADE_COMPARE(layer.storageReferenceCount(storage2D), 0);
    CORRADE_COMPARE(layer.storageReferenceCount(storage3D), 0);
    CORRADE_COMPARE(layer.capacity(), 1);
    CORRADE_COMPARE(layer.usedCount(), 1);
    CORRADE_COMPARE(layer.usedAllocatedCount(), 0);
    CORRADE_COMPARE(layer.node(trivial), NodeHandle::Null);
    CORRADE_VERIFY(!layer.isAllocated(trivial));
    CORRADE_VERIFY(layer.isDirty(trivial));
    CORRADE_COMPARE(layer.storage(trivial), storage);
    CORRADE_COMPARE(layer.index(trivial), (Containers::Size3D{0, 0, 0}));
    /* Sets LayerState::NeedsCommonDataUpdate in order to make sure the update
       function is called on the next update even though the data isn't
       attached to any node or the node is not visible. */
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate);

    /* Non-trivial function, single item, attached. The temporary gets
       destructed right away. */
    Int nonTrivialCalled = 0;
    Int destructed = 0;
    DataHandle nonTrivial = layer.create(storage, NonTrivial{0x333, nonTrivialCalled, destructed}, nodeHandle(0x12345, 0xabc));
    CORRADE_COMPARE(destructed, 1);
    CORRADE_COMPARE(layer.storageReferenceCount(storage), 2);
    CORRADE_COMPARE(layer.storageReferenceCount(storage1D), 0);
    CORRADE_COMPARE(layer.storageReferenceCount(storage2D), 0);
    CORRADE_COMPARE(layer.storageReferenceCount(storage3D), 0);
    CORRADE_COMPARE(layer.capacity(), 2);
    CORRADE_COMPARE(layer.usedCount(), 2);
    CORRADE_COMPARE(layer.usedAllocatedCount(), 1);
    CORRADE_COMPARE(layer.node(nonTrivial), nodeHandle(0x12345, 0xabc));
    CORRADE_VERIFY(layer.isAllocated(nonTrivial));
    CORRADE_VERIFY(layer.isDirty(nonTrivial));
    CORRADE_COMPARE(layer.storage(nonTrivial), storage);
    CORRADE_COMPARE(layer.index(nonTrivial), (Containers::Size3D{0, 0, 0}));
    /* In this and others additional states get set for node attachment */
    CORRADE_COMPARE_AS(layer.state(), LayerState::NeedsCommonDataUpdate,
        TestSuite::Compare::GreaterOrEqual);

    /* Trivial function, 1D index, not attached, LayerDataHandle overloads */
    Int trivial1DCalled = 0;
    DataHandle trivial1D = layer.create(storage1D, 13, [&trivial1DCalled](Int value) {
        CORRADE_COMPARE(value, 0x4444);
        ++trivial1DCalled;
    });
    CORRADE_COMPARE(layer.storageReferenceCount(storage), 2);
    CORRADE_COMPARE(layer.storageReferenceCount(storage1D), 1);
    CORRADE_COMPARE(layer.storageReferenceCount(storage2D), 0);
    CORRADE_COMPARE(layer.storageReferenceCount(storage3D), 0);
    CORRADE_COMPARE(layer.capacity(), 3);
    CORRADE_COMPARE(layer.usedCount(), 3);
    CORRADE_COMPARE(layer.usedAllocatedCount(), 1);
    CORRADE_COMPARE(layer.node(trivial1D), NodeHandle::Null);
    CORRADE_VERIFY(!layer.isAllocated(dataHandleData(trivial1D)));
    CORRADE_VERIFY(layer.isDirty(dataHandleData(trivial1D)));
    CORRADE_COMPARE(layer.storage(dataHandleData(trivial1D)), storage1D);
    CORRADE_COMPARE(layer.index(dataHandleData(trivial1D)), (Containers::Size3D{0, 0, 13}));
    CORRADE_COMPARE_AS(layer.state(), LayerState::NeedsCommonDataUpdate,
        TestSuite::Compare::GreaterOrEqual);

    /* Non-trivial function, 1D index. If the array grows, it shouldn't cause
       destructors to be called on anything existing. Again the temporary gets
       destructed right away. */
    Int nonTrivial1DCalled = 0;
    Int destructed1D = 0;
    DataHandle nonTrivial1D = layer.create(storage1D, 13, NonTrivial{0x4444, nonTrivial1DCalled, destructed1D}, nodeHandle(0xabcde, 0x123));
    CORRADE_COMPARE(destructed, 1);
    CORRADE_COMPARE(destructed1D, 1);
    CORRADE_COMPARE(layer.storageReferenceCount(storage), 2);
    CORRADE_COMPARE(layer.storageReferenceCount(storage1D), 2);
    CORRADE_COMPARE(layer.storageReferenceCount(storage2D), 0);
    CORRADE_COMPARE(layer.storageReferenceCount(storage3D), 0);
    CORRADE_COMPARE(layer.capacity(), 4);
    CORRADE_COMPARE(layer.usedCount(), 4);
    CORRADE_COMPARE(layer.usedAllocatedCount(), 2);
    CORRADE_COMPARE(layer.node(nonTrivial1D), nodeHandle(0xabcde, 0x123));
    CORRADE_VERIFY(layer.isAllocated(nonTrivial1D));
    CORRADE_VERIFY(layer.isDirty(nonTrivial1D));
    CORRADE_COMPARE(layer.storage(nonTrivial1D), storage1D);
    CORRADE_COMPARE(layer.index(nonTrivial1D), (Containers::Size3D{0, 0, 13}));
    CORRADE_COMPARE_AS(layer.state(), LayerState::NeedsCommonDataUpdate,
        TestSuite::Compare::GreaterOrEqual);

    /* Trivial function, 2D index */
    Int trivial2DCalled = 0;
    DataHandle trivial2D = layer.create(storage2D, {2, 5}, [&trivial2DCalled](Int value) {
        CORRADE_COMPARE(value, 0x55555);
        ++trivial2DCalled;
    }, nodeHandle(0x54321, 0xedc));
    CORRADE_COMPARE(layer.storageReferenceCount(storage), 2);
    CORRADE_COMPARE(layer.storageReferenceCount(storage1D), 2);
    CORRADE_COMPARE(layer.storageReferenceCount(storage2D), 1);
    CORRADE_COMPARE(layer.storageReferenceCount(storage3D), 0);
    CORRADE_COMPARE(layer.capacity(), 5);
    CORRADE_COMPARE(layer.usedCount(), 5);
    CORRADE_COMPARE(layer.usedAllocatedCount(), 2);
    CORRADE_COMPARE(layer.node(trivial2D), nodeHandle(0x54321, 0xedc));
    CORRADE_VERIFY(!layer.isAllocated(trivial2D));
    CORRADE_VERIFY(layer.isDirty(trivial2D));
    CORRADE_COMPARE(layer.storage(trivial2D), storage2D);
    CORRADE_COMPARE(layer.index(trivial2D), (Containers::Size3D{0, 2, 5}));
    CORRADE_COMPARE_AS(layer.state(), LayerState::NeedsCommonDataUpdate,
        TestSuite::Compare::GreaterOrEqual);

    /* Non-trivial function, 2D index. Again the temporary gets destructed
       right away. */
    Int nonTrivial2DCalled = 0;
    Int destructed2D = 0;
    DataHandle nonTrivial2D = layer.create(storage2D, {2, 5}, NonTrivial{0x55555, nonTrivial2DCalled, destructed2D}, nodeHandle(0xfedcb, 0x321));
    CORRADE_COMPARE(destructed, 1);
    CORRADE_COMPARE(destructed1D, 1);
    CORRADE_COMPARE(destructed2D, 1);
    CORRADE_COMPARE(layer.storageReferenceCount(storage), 2);
    CORRADE_COMPARE(layer.storageReferenceCount(storage1D), 2);
    CORRADE_COMPARE(layer.storageReferenceCount(storage2D), 2);
    CORRADE_COMPARE(layer.storageReferenceCount(storage3D), 0);
    CORRADE_COMPARE(layer.capacity(), 6);
    CORRADE_COMPARE(layer.usedCount(), 6);
    CORRADE_COMPARE(layer.usedAllocatedCount(), 3);
    CORRADE_COMPARE(layer.node(nonTrivial2D), nodeHandle(0xfedcb, 0x321));
    CORRADE_VERIFY(layer.isAllocated(nonTrivial2D));
    CORRADE_VERIFY(layer.isDirty(nonTrivial2D));
    CORRADE_COMPARE(layer.storage(nonTrivial2D), storage2D);
    CORRADE_COMPARE(layer.index(nonTrivial2D), (Containers::Size3D{0, 2, 5}));
    CORRADE_COMPARE_AS(layer.state(), LayerState::NeedsCommonDataUpdate,
        TestSuite::Compare::GreaterOrEqual);

    /* Trivial function, 3D index */
    Int trivial3DCalled = 0;
    DataHandle trivial3D = layer.create(storage3D, {1, 3, 2}, [&trivial3DCalled](Int value) {
        CORRADE_COMPARE(value, 0x666666);
        ++trivial3DCalled;
    }, nodeHandle(0x67890, 0xaba));
    CORRADE_COMPARE(layer.storageReferenceCount(storage), 2);
    CORRADE_COMPARE(layer.storageReferenceCount(storage1D), 2);
    CORRADE_COMPARE(layer.storageReferenceCount(storage2D), 2);
    CORRADE_COMPARE(layer.storageReferenceCount(storage3D), 1);
    CORRADE_COMPARE(layer.capacity(), 7);
    CORRADE_COMPARE(layer.usedCount(), 7);
    CORRADE_COMPARE(layer.usedAllocatedCount(), 3);
    CORRADE_COMPARE(layer.node(trivial3D), nodeHandle(0x67890, 0xaba));
    CORRADE_VERIFY(!layer.isAllocated(trivial3D));
    CORRADE_VERIFY(layer.isDirty(trivial3D));
    CORRADE_COMPARE(layer.storage(trivial3D), storage3D);
    CORRADE_COMPARE(layer.index(trivial3D), (Containers::Size3D{1, 3, 2}));
    CORRADE_COMPARE_AS(layer.state(), LayerState::NeedsCommonDataUpdate,
        TestSuite::Compare::GreaterOrEqual);

    /* Non-trivial function, 2D index. Again the temporary gets destructed
       right away. */
    Int nonTrivial3DCalled = 0;
    Int destructed3D = 0;
    DataHandle nonTrivial3D = layer.create(storage3D, {1, 3, 2}, NonTrivial{0x666666, nonTrivial3DCalled, destructed3D}, nodeHandle(0x98765, 0xbeb));
    CORRADE_COMPARE(destructed, 1);
    CORRADE_COMPARE(destructed1D, 1);
    CORRADE_COMPARE(destructed2D, 1);
    CORRADE_COMPARE(destructed3D, 1);
    CORRADE_COMPARE(layer.storageReferenceCount(storage), 2);
    CORRADE_COMPARE(layer.storageReferenceCount(storage1D), 2);
    CORRADE_COMPARE(layer.storageReferenceCount(storage2D), 2);
    CORRADE_COMPARE(layer.storageReferenceCount(storage3D), 2);
    CORRADE_COMPARE(layer.capacity(), 8);
    CORRADE_COMPARE(layer.usedCount(), 8);
    CORRADE_COMPARE(layer.usedAllocatedCount(), 4);
    CORRADE_COMPARE(layer.node(nonTrivial3D), nodeHandle(0x98765, 0xbeb));
    CORRADE_VERIFY(layer.isAllocated(nonTrivial3D));
    CORRADE_VERIFY(layer.isDirty(nonTrivial3D));
    CORRADE_COMPARE(layer.storage(nonTrivial3D), storage3D);
    CORRADE_COMPARE(layer.index(nonTrivial3D), (Containers::Size3D{1, 3, 2}));
    CORRADE_COMPARE_AS(layer.state(), LayerState::NeedsCommonDataUpdate,
        TestSuite::Compare::GreaterOrEqual);

    /* None of these -- neither the storage getters nor the update functions --
       should be called until now */
    CORRADE_COMPARE(storageCalled, 0);
    CORRADE_COMPARE(trivialCalled, 0);
    CORRADE_COMPARE(nonTrivialCalled, 0);
    CORRADE_COMPARE(storage1DCalled, 0);
    CORRADE_COMPARE(trivial1DCalled, 0);
    CORRADE_COMPARE(nonTrivial1DCalled, 0);
    CORRADE_COMPARE(storage2DCalled, 0);
    CORRADE_COMPARE(trivial2DCalled, 0);
    CORRADE_COMPARE(nonTrivial2DCalled, 0);
    CORRADE_COMPARE(storage3DCalled, 0);
    CORRADE_COMPARE(trivial3DCalled, 0);
    CORRADE_COMPARE(nonTrivial3DCalled, 0);

    /* They should be all called on the very first update however. The called
       functions then verify the corrrect arguments were passed every time.
       Each storage is used by two data so it gets called twice. */
    layer.preUpdate(LayerState::NeedsCommonDataUpdate);
    CORRADE_COMPARE(storageCalled, 2);
    CORRADE_COMPARE(trivialCalled, 1);
    CORRADE_COMPARE(nonTrivialCalled, 1);
    CORRADE_COMPARE(storage1DCalled, 2);
    CORRADE_COMPARE(trivial1DCalled, 1);
    CORRADE_COMPARE(nonTrivial1DCalled, 1);
    CORRADE_COMPARE(storage2DCalled, 2);
    CORRADE_COMPARE(trivial2DCalled, 1);
    CORRADE_COMPARE(nonTrivial2DCalled, 1);
    CORRADE_COMPARE(storage3DCalled, 2);
    CORRADE_COMPARE(trivial3DCalled, 1);
    CORRADE_COMPARE(nonTrivial3DCalled, 1);

    /* Removing data with a trivial function doesn't do much apart from
       decreasing reference count */
    layer.remove(trivial2D);
    CORRADE_COMPARE(destructed, 1);
    CORRADE_COMPARE(destructed1D, 1);
    CORRADE_COMPARE(destructed2D, 1);
    CORRADE_COMPARE(destructed3D, 1);
    CORRADE_COMPARE(layer.storageReferenceCount(storage), 2);
    CORRADE_COMPARE(layer.storageReferenceCount(storage1D), 2);
    CORRADE_COMPARE(layer.storageReferenceCount(storage2D), 1);
    CORRADE_COMPARE(layer.storageReferenceCount(storage3D), 2);
    CORRADE_COMPARE(layer.capacity(), 8);
    CORRADE_COMPARE(layer.usedCount(), 7);
    CORRADE_COMPARE(layer.usedAllocatedCount(), 4);

    /* Removing data with a non-trivial function calls its destructor, this
       time on the internal instance */
    layer.remove(nonTrivial2D);
    CORRADE_COMPARE(destructed, 1);
    CORRADE_COMPARE(destructed1D, 1);
    CORRADE_COMPARE(destructed2D, 2);
    CORRADE_COMPARE(destructed3D, 1);
    CORRADE_COMPARE(layer.storageReferenceCount(storage), 2);
    CORRADE_COMPARE(layer.storageReferenceCount(storage1D), 2);
    CORRADE_COMPARE(layer.storageReferenceCount(storage2D), 0);
    CORRADE_COMPARE(layer.storageReferenceCount(storage3D), 2);
    CORRADE_COMPARE(layer.capacity(), 8);
    CORRADE_COMPARE(layer.usedCount(), 6);
    CORRADE_COMPARE(layer.usedAllocatedCount(), 3);

    /* Another non-trivial function, LayerDataHandle overload */
    layer.remove(dataHandleData(nonTrivial));
    CORRADE_COMPARE(destructed, 2);
    CORRADE_COMPARE(destructed1D, 1);
    CORRADE_COMPARE(destructed2D, 2);
    CORRADE_COMPARE(destructed3D, 1);
    CORRADE_COMPARE(layer.storageReferenceCount(storage), 1);
    CORRADE_COMPARE(layer.storageReferenceCount(storage1D), 2);
    CORRADE_COMPARE(layer.storageReferenceCount(storage2D), 0);
    CORRADE_COMPARE(layer.storageReferenceCount(storage3D), 2);
    CORRADE_COMPARE(layer.capacity(), 8);
    CORRADE_COMPARE(layer.usedCount(), 5);
    CORRADE_COMPARE(layer.usedAllocatedCount(), 2);

    /* Can remove the no-longer referenced storage now as well */
    layer.removeStorage(storage2D);
    CORRADE_COMPARE(layer.storageCapacity(), 6);
    CORRADE_COMPARE(layer.storageUsedCount(), 5);
    CORRADE_COMPARE(layer.storageUsedAllocatedCount(), 0);

    /* Destructing the whole layer instance should destruct all remaining
       non-trivial functions. And not the ones already removed. */
    layer = DataLayer{layerHandle(0, 2)};
    CORRADE_COMPARE(layer.handle(), layerHandle(0, 2));
    CORRADE_COMPARE(destructed, 2);
    CORRADE_COMPARE(destructed1D, 2);
    CORRADE_COMPARE(destructed2D, 2);
    CORRADE_COMPARE(destructed3D, 2);
}

void DataLayerTest::createRemoveHandleRecycle() {
    struct DummyStorage: AbstractStorage {
        explicit DummyStorage(DataLayer& layer): AbstractStorage{layer, {13, 5, 22}} {}

        Int operator[](const Containers::Size3D&) const {
            CORRADE_FAIL("This should never be called.");
            return {};
        }
    };

    /* Note that the referenced variables have to be *before* the layer in
       order for them to be still in scope when the layer destructor destructs
       remaining functions. ASan complains otherwise. */
    Int destructed1 = 0,
        destructed2 = 0;
    struct NonTrivial {
        explicit NonTrivial(Int& destructed):  destructed{&destructed} {}
        ~NonTrivial() {
            ++*destructed;
        }
        void operator()(Int) const {
            CORRADE_FAIL("This should never be called.");
        }

        Int* destructed;
    };

    DataLayer layer{layerHandle(0, 1)};
    DummyStorage storage{layer};
    layer.create(storage, {0, 0, 0}, [](Int) {});

    /* The temporary gets destructed right away. Fill it with a 3D index to
       verify it gets cleared on recycle. */
    DataHandle second = layer.create(storage, {8, 3, 16}, NonTrivial{destructed1});
    CORRADE_COMPARE(destructed1, 1);

    layer.remove(second);
    CORRADE_COMPARE(destructed1, 2);

    /* Data that reuses a previous slot should not call the destructor on the
       previous function again or some such crazy stuff. It should also reset
       the size and flags. */
    DataHandle second2 = layer.create(storage, {0, 0, 0}, NonTrivial{destructed2});
    CORRADE_COMPARE(dataHandleId(second2), dataHandleId(second));
    CORRADE_COMPARE(layer.index(second2), Containers::Size3D{});
    CORRADE_COMPARE(destructed1, 2);
    CORRADE_COMPARE(destructed2, 1);
}

void DataLayerTest::createMemberFunction() {
    /* Subset of createRemove() with explicit member function pointers instead
       of the implicitly picked member to verify these are correctly called. No
       reference count, allocated function, layer state or removal verification
       as that shouldn't behave any different in this case. */

    struct DummyStorage: AbstractStorage {
        explicit DummyStorage(DataLayer& layer): AbstractStorage{layer} {}

        Int singleValue() const {
            ++storageCalled;
            return 0x333;
        }
    };
    struct DummyStorage1D: AbstractStorage {
        explicit DummyStorage1D(DataLayer& layer): AbstractStorage{layer, 15} {}

        Int listItem(std::size_t index) const {
            CORRADE_COMPARE(index, 13);
            ++storage1DCalled;
            return 0x4444;
        }
    };
    struct DummyStorage2D: AbstractStorage {
        explicit DummyStorage2D(DataLayer& layer): AbstractStorage{layer, {3, 7}} {}

        Int imageValue(const Containers::Size2D& index) const {
            CORRADE_COMPARE(index, (Containers::Size2D{2, 5}));
            ++storage2DCalled;
            return 0x55555;
        }
    };
    struct DummyStorage3D: AbstractStorage {
        explicit DummyStorage3D(DataLayer& layer): AbstractStorage{layer, {2, 5, 4}} {}

        Int volumeCell(const Containers::Size3D& index) const {
            CORRADE_COMPARE(index, (Containers::Size3D{1, 3, 2}));
            ++storage3DCalled;
            return 0x666666;
        }
    };

    DataLayer layer{layerHandle(0, 1)};

    DummyStorage storage{layer};
    DummyStorage1D storage1D{layer};
    DummyStorage2D storage2D{layer};
    DummyStorage3D storage3D{layer};

    /* Single item */
    Int firstCalled = 0;
    DataHandle first = layer.create(storage, &DummyStorage::singleValue, [&firstCalled](Int value) {
        CORRADE_COMPARE(value, 0x333);
        ++firstCalled;
    }, nodeHandle(0x12345, 0xabc));
    CORRADE_COMPARE(layer.node(first), nodeHandle(0x12345, 0xabc));
    CORRADE_VERIFY(layer.isDirty(first));
    CORRADE_COMPARE(layer.storage(first), storage);
    CORRADE_COMPARE(layer.index(first), (Containers::Size3D{0, 0, 0}));

    /* 1D index */
    Int secondCalled = 0;
    DataHandle second = layer.create(storage1D, &DummyStorage1D::listItem, 13, [&secondCalled](Int value) {
        CORRADE_COMPARE(value, 0x4444);
        ++secondCalled;
    }, nodeHandle(0xabcde, 0xabc));
    CORRADE_COMPARE(layer.node(second), nodeHandle(0xabcde, 0xabc));
    CORRADE_VERIFY(layer.isDirty(second));
    CORRADE_COMPARE(layer.storage(second), storage1D);
    CORRADE_COMPARE(layer.index(second), (Containers::Size3D{0, 0, 13}));

    /* 2D index */
    Int thirdCalled = 0;
    DataHandle third = layer.create(storage2D, &DummyStorage2D::imageValue, {2, 5}, [&thirdCalled](Int value) {
        CORRADE_COMPARE(value, 0x55555);
        ++thirdCalled;
    }, nodeHandle(0x54321, 0xedc));
    CORRADE_COMPARE(layer.node(third), nodeHandle(0x54321, 0xedc));
    CORRADE_VERIFY(layer.isDirty(third));
    CORRADE_COMPARE(layer.storage(third), storage2D);
    CORRADE_COMPARE(layer.index(third), (Containers::Size3D{0, 2, 5}));

    /* 3D index */
    Int fourthCalled = 0;
    DataHandle fourth = layer.create(storage3D, &DummyStorage3D::volumeCell, {1, 3, 2}, [&fourthCalled](Int value) {
        CORRADE_COMPARE(value, 0x666666);
        ++fourthCalled;
    }, nodeHandle(0x67890, 0xaba));
    CORRADE_COMPARE(layer.node(fourth), nodeHandle(0x67890, 0xaba));
    CORRADE_VERIFY(layer.isDirty(fourth));
    CORRADE_COMPARE(layer.storage(fourth), storage3D);
    CORRADE_COMPARE(layer.index(fourth), (Containers::Size3D{1, 3, 2}));

    /* Verify that proper storage functions get called, the functions
       themselves then again verify argument correctness. */
    layer.preUpdate(LayerState::NeedsCommonDataUpdate);
    CORRADE_COMPARE(storageCalled, 1);
    CORRADE_COMPARE(firstCalled, 1);
    CORRADE_COMPARE(storage1DCalled, 1);
    CORRADE_COMPARE(secondCalled, 1);
    CORRADE_COMPARE(storage2DCalled, 1);
    CORRADE_COMPARE(thirdCalled, 1);
    CORRADE_COMPARE(storage3DCalled, 1);
    CORRADE_COMPARE(fourthCalled, 1);

    /* Just a sanity check that these variants don't cause allocated functions
       to be used or some such */
    CORRADE_COMPARE(layer.capacity(), 4);
    CORRADE_COMPARE(layer.usedCount(), 4);
    CORRADE_COMPARE(layer.usedAllocatedCount(), 0);
}

void DataLayerTest::createOverloaded() {
    /* Subset of createRemove() with a single storage that has overloaded
       access functions, additionally with each returning a different type. As
       with createMemberFunction() no reference count, allocated function,
       layer state or removal verification. */

    struct DummyStorage: AbstractStorage {
        explicit DummyStorage(DataLayer& layer, const Containers::Size3D& size): AbstractStorage{layer, size} {}

        /* These get called below to verify the correct member function is
           picked and called with the right arguments */
        Int operator*() const {
            ++storageCalled;
            return 0x333;
        }
        Short operator[](std::size_t index) const {
            CORRADE_COMPARE(index, 13);
            ++storage1DCalled;
            return 0x4444;
        }
        Byte operator[](const Containers::Size2D& index) const {
            CORRADE_COMPARE(index, (Containers::Size2D{2, 5}));
            ++storage2DCalled;
            return 0x55;
        }
        Long operator[](const Containers::Size3D& index) const {
            CORRADE_COMPARE(index, (Containers::Size3D{1, 3, 2}));
            ++storage3DCalled;
            return 0x666666;
        }
    };

    DataLayer layer{layerHandle(0, 1)};
    DummyStorage storage{layer, {1, 1, 1}};
    DummyStorage storage1D{layer, {1, 1, 15}};
    DummyStorage storage2D{layer, {1, 3, 7}};
    DummyStorage storage3D{layer, {2, 5, 4}};

    /* Single item. Shouldn't cause any issues, but still verify that it's
       correctly picked even in presence of operator[]. */
    Int firstCalled = 0;
    DataHandle first = layer.create(storage, [&firstCalled](Int value) {
        CORRADE_COMPARE(value, 0x333);
        ++firstCalled;
    }, nodeHandle(0x12345, 0xabc));
    CORRADE_COMPARE(layer.node(first), nodeHandle(0x12345, 0xabc));
    CORRADE_VERIFY(layer.isDirty(first));
    CORRADE_COMPARE(layer.storage(first), storage);
    CORRADE_COMPARE(layer.index(first), (Containers::Size3D{0, 0, 0}));

    /* 1D index */
    Int secondCalled = 0;
    DataHandle second = layer.create(storage1D, 13, [&secondCalled](Short value) {
        CORRADE_COMPARE(value, 0x4444);
        ++secondCalled;
    }, nodeHandle(0xabcde, 0xabc));
    CORRADE_COMPARE(layer.node(second), nodeHandle(0xabcde, 0xabc));
    CORRADE_VERIFY(layer.isDirty(second));
    CORRADE_COMPARE(layer.storage(second), storage1D);
    CORRADE_COMPARE(layer.index(second), (Containers::Size3D{0, 0, 13}));

    /* 2D index */
    Int thirdCalled = 0;
    DataHandle third = layer.create(storage2D, {2, 5}, [&thirdCalled](Byte value) {
        CORRADE_COMPARE(value, 0x55);
        ++thirdCalled;
    }, nodeHandle(0x54321, 0xedc));
    CORRADE_COMPARE(layer.node(third), nodeHandle(0x54321, 0xedc));
    CORRADE_VERIFY(layer.isDirty(third));
    CORRADE_COMPARE(layer.storage(third), storage2D);
    CORRADE_COMPARE(layer.index(third), (Containers::Size3D{0, 2, 5}));

    /* 3D index */
    Int fourthCalled = 0;
    DataHandle fourth = layer.create(storage3D, {1, 3, 2}, [&fourthCalled](Long value) {
        CORRADE_COMPARE(value, 0x666666);
        ++fourthCalled;
    }, nodeHandle(0x67890, 0xaba));
    CORRADE_COMPARE(layer.node(fourth), nodeHandle(0x67890, 0xaba));
    CORRADE_VERIFY(layer.isDirty(fourth));
    CORRADE_COMPARE(layer.storage(fourth), storage3D);
    CORRADE_COMPARE(layer.index(fourth), (Containers::Size3D{1, 3, 2}));

    /* Verify that proper storage functions get called, the functions
       themselves then again verify argument correctness. */
    layer.preUpdate(LayerState::NeedsCommonDataUpdate);
    CORRADE_COMPARE(storageCalled, 1);
    CORRADE_COMPARE(firstCalled, 1);
    CORRADE_COMPARE(storage1DCalled, 1);
    CORRADE_COMPARE(secondCalled, 1);
    CORRADE_COMPARE(storage2DCalled, 1);
    CORRADE_COMPARE(thirdCalled, 1);
    CORRADE_COMPARE(storage3DCalled, 1);
    CORRADE_COMPARE(fourthCalled, 1);

    /* Just a sanity check that these variants don't cause allocated functions
       to be used by accident or some such */
    CORRADE_COMPARE(layer.capacity(), 4);
    CORRADE_COMPARE(layer.usedCount(), 4);
    CORRADE_COMPARE(layer.usedAllocatedCount(), 0);
}

void DataLayerTest::createOverloadedMemberFunction() {
    /* Like createOverloaded(), just with an explicit member function used */

    struct DummyStorage: AbstractStorage {
        explicit DummyStorage(DataLayer& layer, const Containers::Size3D& size): AbstractStorage{layer, size} {}

        /* These get called below to verify the correct member function is
           picked and called with the right arguments */
        Int thingy() const {
            ++storageCalled;
            return 0x333;
        }
        Short thingy(std::size_t index) const {
            CORRADE_COMPARE(index, 13);
            ++storage1DCalled;
            return 0x4444;
        }
        Byte thingy(const Containers::Size2D& index) const {
            CORRADE_COMPARE(index, (Containers::Size2D{2, 5}));
            ++storage2DCalled;
            return 0x55;
        }
        Long thingy(const Containers::Size3D& index) const {
            CORRADE_COMPARE(index, (Containers::Size3D{1, 3, 2}));
            ++storage3DCalled;
            return 0x666666;
        }
    };

    DataLayer layer{layerHandle(0, 1)};
    DummyStorage storage{layer, {1, 1, 1}};
    DummyStorage storage1D{layer, {1, 1, 15}};
    DummyStorage storage2D{layer, {1, 3, 7}};
    DummyStorage storage3D{layer, {2, 5, 4}};

    /* Single item. Shouldn't cause any issues, but still verify that it's
       correctly picked even in presence of other overloads of thingy(). */
    Int firstCalled = 0;
    DataHandle first = layer.create(storage, &DummyStorage::thingy, [&firstCalled](Int value) {
        CORRADE_COMPARE(value, 0x333);
        ++firstCalled;
    }, nodeHandle(0x12345, 0xabc));
    CORRADE_COMPARE(layer.node(first), nodeHandle(0x12345, 0xabc));
    CORRADE_VERIFY(layer.isDirty(first));
    CORRADE_COMPARE(layer.storage(first), storage);
    CORRADE_COMPARE(layer.index(first), (Containers::Size3D{0, 0, 0}));

    /* 1D index */
    Int secondCalled = 0;
    DataHandle second = layer.create(storage1D, &DummyStorage::thingy, 13, [&secondCalled](Short value) {
        CORRADE_COMPARE(value, 0x4444);
        ++secondCalled;
    }, nodeHandle(0xabcde, 0xabc));
    CORRADE_COMPARE(layer.node(second), nodeHandle(0xabcde, 0xabc));
    CORRADE_VERIFY(layer.isDirty(second));
    CORRADE_COMPARE(layer.storage(second), storage1D);
    CORRADE_COMPARE(layer.index(second), (Containers::Size3D{0, 0, 13}));

    /* 2D index */
    Int thirdCalled = 0;
    DataHandle third = layer.create(storage2D, &DummyStorage::thingy, {2, 5}, [&thirdCalled](Byte value) {
        CORRADE_COMPARE(value, 0x55);
        ++thirdCalled;
    }, nodeHandle(0x54321, 0xedc));
    CORRADE_COMPARE(layer.node(third), nodeHandle(0x54321, 0xedc));
    CORRADE_VERIFY(layer.isDirty(third));
    CORRADE_COMPARE(layer.storage(third), storage2D);
    CORRADE_COMPARE(layer.index(third), (Containers::Size3D{0, 2, 5}));

    /* 3D index */
    Int fourthCalled = 0;
    DataHandle fourth = layer.create(storage3D, &DummyStorage::thingy, {1, 3, 2}, [&fourthCalled](Long value) {
        CORRADE_COMPARE(value, 0x666666);
        ++fourthCalled;
    }, nodeHandle(0x67890, 0xaba));
    CORRADE_COMPARE(layer.node(fourth), nodeHandle(0x67890, 0xaba));
    CORRADE_VERIFY(layer.isDirty(fourth));
    CORRADE_COMPARE(layer.storage(fourth), storage3D);
    CORRADE_COMPARE(layer.index(fourth), (Containers::Size3D{1, 3, 2}));

    /* Verify that proper storage functions get called, the functions
       themselves then again verify argument correctness. */
    layer.preUpdate(LayerState::NeedsCommonDataUpdate);
    CORRADE_COMPARE(storageCalled, 1);
    CORRADE_COMPARE(firstCalled, 1);
    CORRADE_COMPARE(storage1DCalled, 1);
    CORRADE_COMPARE(secondCalled, 1);
    CORRADE_COMPARE(storage2DCalled, 1);
    CORRADE_COMPARE(thirdCalled, 1);
    CORRADE_COMPARE(storage3DCalled, 1);
    CORRADE_COMPARE(fourthCalled, 1);

    /* Just a sanity check that these variants don't cause allocated functions
       to be used by accident or some such */
    CORRADE_COMPARE(layer.capacity(), 4);
    CORRADE_COMPARE(layer.usedCount(), 4);
    CORRADE_COMPARE(layer.usedAllocatedCount(), 0);
}

void DataLayerTest::createInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct DummyStorage: AbstractStorage {
        explicit DummyStorage(DataLayer& layer, const Containers::Size3D& size): AbstractStorage{layer, size} {}

        Int operator*() const {
            CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        }
        Int operator[](std::size_t) const {
            CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        }
        Int operator[](const Containers::Size2D&) const {
            CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        }
        Int operator[](const Containers::Size3D&) const {
            CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        }
    };

    DataLayer layer{layerHandle(0xab, 0x12)};
    DummyStorage storage{layer, {1, 1, 1}};
    DummyStorage storage1D{layer, {1, 1, 15}};
    DummyStorage storage2D{layer, {1, 3, 7}};
    DummyStorage storage3D{layer, {4, 2, 5}};

    DummyStorage removedStorage{layer, {1, 1, 1}};
    layer.removeStorage(removedStorage);

    /* Storage from another layer has the same handle, but should still fail
       because the instance is different */
    DataLayer anotherLayer{layerHandle(0xab, 0x12)};
    DummyStorage anotherLayerStorage{anotherLayer, {1, 1, 1}};
    CORRADE_COMPARE(anotherLayer.handle(), layer.handle());
    CORRADE_COMPARE(anotherLayerStorage.handle(), storage.handle());

    /* Indices exactly at the end shouldn't trigger an assert */
    layer.create(storage, [](Int) {});
    layer.create(storage1D, 14, [](Int) {});
    layer.create(storage2D, {2, 6}, [](Int) {});
    layer.create(storage3D, {3, 1, 4}, [](Int) {});

    Containers::String out;
    Error redirectError{&out};
    /* Storage from a different layer or with an invalid handle */
    layer.create(anotherLayerStorage, [](Int) {});
    layer.create(removedStorage, [](Int) {});
    /* Member being null. all dimension overloads */
    layer.create(storage, static_cast<Int(DummyStorage::*)() const>(nullptr), [](Int) {});
    layer.create(storage1D, static_cast<Int(DummyStorage::*)(std::size_t) const>(nullptr), 0, [](Int) {});
    layer.create(storage2D, static_cast<Int(DummyStorage::*)(const Containers::Size2D&) const>(nullptr), {0, 0}, [](Int) {});
    layer.create(storage3D, static_cast<Int(DummyStorage::*)(const Containers::Size3D&) const>(nullptr), {0, 0, 0}, [](Int) {});
    /* Update being null, all dimension overloads */
    layer.create(storage, nullptr);
    layer.create(storage1D, 0, nullptr);
    layer.create(storage2D, {0, 0}, nullptr);
    layer.create(storage3D, {0, 0, 0}, nullptr);
    /* Index out of bounds in 1D, 2D and 3D */
    layer.create(storage1D, 15, [](Int) {});
    layer.create(storage2D, {2, 7}, [](Int) {});
    layer.create(storage2D, {3, 6}, [](Int) {});
    layer.create(storage3D, {3, 1, 5}, [](Int) {});
    layer.create(storage3D, {3, 2, 4}, [](Int) {});
    layer.create(storage3D, {4, 1, 4}, [](Int) {});
    /* No index specified for a 1D/2D/3D storage even though it's in bounds */
    layer.create(storage1D, [](Int) {});
    layer.create(storage2D, [](Int) {});
    layer.create(storage3D, [](Int) {});
    /* 1D index specified for a 2D/3D storage even though it's in bounds */
    layer.create(storage2D, 0, [](Int) {});
    layer.create(storage3D, 0, [](Int) {});
    /* 2D index specified for a 3D storage even though it's in bounds */
    layer.create(storage3D, {0, 0}, [](Int) {});
    CORRADE_COMPARE_AS(out,
        "Ui::DataLayer::create(): storage doesn't belong to this layer\n"
        "Ui::DataLayer::create(): invalid handle Ui::StorageHandle({0xab, 0x12}, {0x4, 0x1})\n"

        "Ui::DataLayer::create(): member is null\n"
        "Ui::DataLayer::create(): member is null\n"
        "Ui::DataLayer::create(): member is null\n"
        "Ui::DataLayer::create(): member is null\n"

        "Ui::DataLayer::create(): update is null\n"
        "Ui::DataLayer::create(): update is null\n"
        "Ui::DataLayer::create(): update is null\n"
        "Ui::DataLayer::create(): update is null\n"

        "Ui::DataLayer::create(): index {0, 0, 15} out of range for storage of size {1, 1, 15}\n"
        "Ui::DataLayer::create(): index {0, 2, 7} out of range for storage of size {1, 3, 7}\n"
        "Ui::DataLayer::create(): index {0, 3, 6} out of range for storage of size {1, 3, 7}\n"
        "Ui::DataLayer::create(): index {3, 1, 5} out of range for storage of size {4, 2, 5}\n"
        "Ui::DataLayer::create(): index {3, 2, 4} out of range for storage of size {4, 2, 5}\n"
        "Ui::DataLayer::create(): index {4, 1, 4} out of range for storage of size {4, 2, 5}\n"

        "Ui::DataLayer::create(): expected a single-item storage but got a size of {1, 1, 15}\n"
        "Ui::DataLayer::create(): expected a single-item storage but got a size of {1, 3, 7}\n"
        "Ui::DataLayer::create(): expected a single-item storage but got a size of {4, 2, 5}\n"
        "Ui::DataLayer::create(): expected a 1D storage but got a size of {1, 3, 7}\n"
        "Ui::DataLayer::create(): expected a 1D storage but got a size of {4, 2, 5}\n"
        "Ui::DataLayer::create(): expected a 2D storage but got a size of {4, 2, 5}\n",
        TestSuite::Compare::String);
}

void DataLayerTest::setIndex() {
    struct DummyStorage: AbstractStorage {
        explicit DummyStorage(DataLayer& layer, const Containers::Size3D& size): AbstractStorage{layer, size} {}

        Int operator[](std::size_t) const { return {}; }
        Int operator[](const Containers::Size2D&) const { return {}; }
        Int operator[](const Containers::Size3D&) const { return {}; }
    };

    DataLayer layer{layerHandle(0, 1)};
    DummyStorage storage1D{layer, {1, 1, 15}};
    DummyStorage storage2D{layer, {1, 5, 3}};
    DummyStorage storage3D{layer, {4, 7, 5}};

    /* Create a dummy data to verify it doesn't always update the first */
    layer.create(storage3D, {1, 2, 3}, [](Int) {});

    /* By default the data is marked as dirty, and NeedsCommonDataUpdate is
       set. NeedsDataUpdate is set implicitly when creating new data. */
    DataHandle data1D = layer.create(storage1D, 3, [](Int) {});
    DataHandle data2D = layer.create(storage2D, {3, 2}, [](Int) {});
    DataHandle data3D = layer.create(storage3D, {3, 2, 1}, [](Int) {});
    CORRADE_COMPARE(layer.index(data1D), (Containers::Size3D{0, 0, 3}));
    CORRADE_COMPARE(layer.index(data2D), (Containers::Size3D{0, 3, 2}));
    CORRADE_COMPARE(layer.index(data3D), (Containers::Size3D{3, 2, 1}));
    CORRADE_VERIFY(layer.isDirty(data1D));
    CORRADE_VERIFY(layer.isDirty(data2D));
    CORRADE_VERIFY(layer.isDirty(data3D));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate);

    /* Calling preUpdate() + update() resets the dirty bit and the state. From
       the outside it doesn't really matter which of the two does it, so call
       both. */
    layer.preUpdate(LayerState::NeedsCommonDataUpdate);
    layer.update(LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_VERIFY(!layer.isDirty(data1D));
    CORRADE_VERIFY(!layer.isDirty(data2D));
    CORRADE_VERIFY(!layer.isDirty(data3D));
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Seting a 1D index. Marks the data as dirty and sets
       NeedsCommonDataUpdate. */
    layer.setIndex(data1D, 11);
    CORRADE_COMPARE(layer.index(data1D), (Containers::Size3D{0, 0, 11}));
    CORRADE_VERIFY(layer.isDirty(data1D));
    CORRADE_VERIFY(!layer.isDirty(data2D));
    CORRADE_VERIFY(!layer.isDirty(data3D));

    /* Calling preUpdate() + update() resets the dirty bit and the state */
    layer.preUpdate(LayerState::NeedsCommonDataUpdate);
    layer.update(LayerState::NeedsCommonDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_VERIFY(!layer.isDirty(data1D));
    CORRADE_VERIFY(!layer.isDirty(data2D));
    CORRADE_VERIFY(!layer.isDirty(data3D));
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Seting a 1D index to the same value does nothing */
    layer.setIndex(data1D, 11);
    CORRADE_COMPARE(layer.index(data1D), (Containers::Size3D{0, 0, 11}));
    CORRADE_VERIFY(!layer.isDirty(data1D));
    CORRADE_VERIFY(!layer.isDirty(data2D));
    CORRADE_VERIFY(!layer.isDirty(data3D));
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Seting a 1D index, LayerDataHandle overload */
    layer.setIndex(dataHandleData(data1D), 9);
    CORRADE_COMPARE(layer.index(data1D), (Containers::Size3D{0, 0, 9}));
    CORRADE_VERIFY(layer.isDirty(data1D));
    CORRADE_VERIFY(!layer.isDirty(data2D));
    CORRADE_VERIFY(!layer.isDirty(data3D));

    /* Calling preUpdate() + update() resets the dirty bit and the state */
    layer.preUpdate(LayerState::NeedsCommonDataUpdate);
    layer.update(LayerState::NeedsCommonDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_VERIFY(!layer.isDirty(data1D));
    CORRADE_VERIFY(!layer.isDirty(data2D));
    CORRADE_VERIFY(!layer.isDirty(data3D));
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Seting a 2D index */
    layer.setIndex(data2D, {4, 2});
    CORRADE_COMPARE(layer.index(data2D), (Containers::Size3D{0, 4, 2}));
    CORRADE_VERIFY(!layer.isDirty(data1D));
    CORRADE_VERIFY(layer.isDirty(data2D));
    CORRADE_VERIFY(!layer.isDirty(data3D));

    /* Calling preUpdate() + update() resets the dirty bit and the state */
    layer.preUpdate(LayerState::NeedsCommonDataUpdate);
    layer.update(LayerState::NeedsCommonDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_VERIFY(!layer.isDirty(data1D));
    CORRADE_VERIFY(!layer.isDirty(data2D));
    CORRADE_VERIFY(!layer.isDirty(data3D));
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Seting a 2D index to the same value does nothing */
    layer.setIndex(data2D, {4, 2});
    CORRADE_COMPARE(layer.index(data2D), (Containers::Size3D{0, 4, 2}));
    CORRADE_VERIFY(!layer.isDirty(data1D));
    CORRADE_VERIFY(!layer.isDirty(data2D));
    CORRADE_VERIFY(!layer.isDirty(data3D));
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Seting a 2D index, LayerDataHandle overload */
    layer.setIndex(dataHandleData(data2D), {3, 2});
    CORRADE_COMPARE(layer.index(data2D), (Containers::Size3D{0, 3, 2}));
    CORRADE_VERIFY(!layer.isDirty(data1D));
    CORRADE_VERIFY(layer.isDirty(data2D));
    CORRADE_VERIFY(!layer.isDirty(data3D));

    /* Calling preUpdate() + update() resets the dirty bit and the state */
    layer.preUpdate(LayerState::NeedsCommonDataUpdate);
    layer.update(LayerState::NeedsCommonDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_VERIFY(!layer.isDirty(data1D));
    CORRADE_VERIFY(!layer.isDirty(data2D));
    CORRADE_VERIFY(!layer.isDirty(data3D));
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Seting a 3D index */
    layer.setIndex(data3D, {3, 6, 2});
    CORRADE_COMPARE(layer.index(data3D), (Containers::Size3D{3, 6, 2}));
    CORRADE_VERIFY(!layer.isDirty(data1D));
    CORRADE_VERIFY(!layer.isDirty(data2D));
    CORRADE_VERIFY(layer.isDirty(data3D));

    /* Calling preUpdate() + update() resets the dirty bit and the state */
    layer.preUpdate(LayerState::NeedsCommonDataUpdate);
    layer.update(LayerState::NeedsCommonDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_VERIFY(!layer.isDirty(data1D));
    CORRADE_VERIFY(!layer.isDirty(data2D));
    CORRADE_VERIFY(!layer.isDirty(data3D));
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Seting a 3D index to the same value does nothing */
    layer.setIndex(data3D, {3, 6, 2});
    CORRADE_COMPARE(layer.index(data3D), (Containers::Size3D{3, 6, 2}));
    CORRADE_VERIFY(!layer.isDirty(data1D));
    CORRADE_VERIFY(!layer.isDirty(data2D));
    CORRADE_VERIFY(!layer.isDirty(data3D));
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Seting a 3D index, LayerDataHandle overload */
    layer.setIndex(dataHandleData(data3D), {1, 4, 3});
    CORRADE_COMPARE(layer.index(data3D), (Containers::Size3D{1, 4, 3}));
    CORRADE_VERIFY(!layer.isDirty(data1D));
    CORRADE_VERIFY(!layer.isDirty(data2D));
    CORRADE_VERIFY(layer.isDirty(data3D));
}

void DataLayerTest::setIndexInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    /* Invalid handles passed to setIndex() tested in invalidHandle() below */

    struct DummyStorage: AbstractStorage {
        explicit DummyStorage(DataLayer& layer, const Containers::Size3D& size): AbstractStorage{layer, size} {}

        Int operator[](std::size_t) const {
            CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        }
        Int operator[](const Containers::Size2D&) const {
            CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        }
        Int operator[](const Containers::Size3D&) const {
            CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        }
    };

    DataLayer layer{layerHandle(0, 1)};
    DummyStorage storage1D{layer, {1, 1, 15}};
    DummyStorage storage2D{layer, {1, 3, 7}};
    DummyStorage storage3D{layer, {4, 2, 5}};

    DataHandle data1D = layer.create(storage1D, 0, [](Int) {});
    DataHandle data2D = layer.create(storage2D, {0, 0}, [](Int) {});
    DataHandle data3D = layer.create(storage3D, {0, 0, 0}, [](Int) {});

    Containers::String out;
    Error redirectError{&out};
    /* Index out of bounds in 1D */
    layer.setIndex(data1D, 15);
    layer.setIndex(dataHandleData(data1D), 15);
    /* In 2D */
    layer.setIndex(data2D, {2, 7});
    layer.setIndex(dataHandleData(data2D), {2, 7});
    layer.setIndex(data2D, {3, 6});
    layer.setIndex(dataHandleData(data2D), {3, 6});
    /* In 3D */
    layer.setIndex(data3D, {3, 1, 5});
    layer.setIndex(dataHandleData(data3D), {3, 1, 5});
    layer.setIndex(data3D, {3, 2, 4});
    layer.setIndex(dataHandleData(data3D), {3, 2, 4});
    layer.setIndex(data3D, {4, 1, 4});
    layer.setIndex(dataHandleData(data3D), {4, 1, 4});
    /* 1D index specified for a 2D/3D storage even though it's in bounds */
    layer.setIndex(data2D, 0);
    layer.setIndex(dataHandleData(data2D), 0);
    layer.setIndex(data3D, 0);
    layer.setIndex(dataHandleData(data3D), 0);
    /* 2D index specified for a 3D storage even though it's in bounds */
    layer.setIndex(data3D, {0, 0});
    layer.setIndex(dataHandleData(data3D), {0, 0});
    CORRADE_COMPARE_AS(out,
        "Ui::DataLayer::setIndex(): index {0, 0, 15} out of range for storage of size {1, 1, 15}\n"
        "Ui::DataLayer::setIndex(): index {0, 0, 15} out of range for storage of size {1, 1, 15}\n"

        "Ui::DataLayer::setIndex(): index {0, 2, 7} out of range for storage of size {1, 3, 7}\n"
        "Ui::DataLayer::setIndex(): index {0, 2, 7} out of range for storage of size {1, 3, 7}\n"
        "Ui::DataLayer::setIndex(): index {0, 3, 6} out of range for storage of size {1, 3, 7}\n"
        "Ui::DataLayer::setIndex(): index {0, 3, 6} out of range for storage of size {1, 3, 7}\n"

        "Ui::DataLayer::setIndex(): index {3, 1, 5} out of range for storage of size {4, 2, 5}\n"
        "Ui::DataLayer::setIndex(): index {3, 1, 5} out of range for storage of size {4, 2, 5}\n"
        "Ui::DataLayer::setIndex(): index {3, 2, 4} out of range for storage of size {4, 2, 5}\n"
        "Ui::DataLayer::setIndex(): index {3, 2, 4} out of range for storage of size {4, 2, 5}\n"
        "Ui::DataLayer::setIndex(): index {4, 1, 4} out of range for storage of size {4, 2, 5}\n"
        "Ui::DataLayer::setIndex(): index {4, 1, 4} out of range for storage of size {4, 2, 5}\n"

        "Ui::DataLayer::setIndex(): expected a 1D storage but got a size of {1, 3, 7}\n"
        "Ui::DataLayer::setIndex(): expected a 1D storage but got a size of {1, 3, 7}\n"
        "Ui::DataLayer::setIndex(): expected a 1D storage but got a size of {4, 2, 5}\n"
        "Ui::DataLayer::setIndex(): expected a 1D storage but got a size of {4, 2, 5}\n"
        "Ui::DataLayer::setIndex(): expected a 2D storage but got a size of {4, 2, 5}\n"
        "Ui::DataLayer::setIndex(): expected a 2D storage but got a size of {4, 2, 5}\n",
        TestSuite::Compare::String);
}

void DataLayerTest::invalidHandle() {
    CORRADE_SKIP_IF_NO_ASSERT();

    DataLayer layer{layerHandle(0, 1)};

    Containers::String out;
    Error redirectError{&out};
    layer.isAllocated(DataHandle::Null);
    layer.isAllocated(LayerDataHandle::Null);
    layer.isDirty(DataHandle::Null);
    layer.isDirty(LayerDataHandle::Null);
    layer.storage(DataHandle::Null);
    layer.storage(LayerDataHandle::Null);
    layer.index(DataHandle::Null);
    layer.index(LayerDataHandle::Null);
    layer.setIndex(DataHandle::Null, 0);
    layer.setIndex(LayerDataHandle::Null, 0);
    layer.setIndex(DataHandle::Null, {0, 0});
    layer.setIndex(LayerDataHandle::Null, {0, 0});
    layer.setIndex(DataHandle::Null, {0, 0, 0});
    layer.setIndex(LayerDataHandle::Null, {0, 0, 0});
    CORRADE_COMPARE_AS(out,
        "Ui::DataLayer::isAllocated(): invalid handle Ui::DataHandle::Null\n"
        "Ui::DataLayer::isAllocated(): invalid handle Ui::LayerDataHandle::Null\n"
        "Ui::DataLayer::isDirty(): invalid handle Ui::DataHandle::Null\n"
        "Ui::DataLayer::isDirty(): invalid handle Ui::LayerDataHandle::Null\n"
        "Ui::DataLayer::storage(): invalid handle Ui::DataHandle::Null\n"
        "Ui::DataLayer::storage(): invalid handle Ui::LayerDataHandle::Null\n"
        "Ui::DataLayer::index(): invalid handle Ui::DataHandle::Null\n"
        "Ui::DataLayer::index(): invalid handle Ui::LayerDataHandle::Null\n"
        "Ui::DataLayer::setIndex(): invalid handle Ui::DataHandle::Null\n"
        "Ui::DataLayer::setIndex(): invalid handle Ui::LayerDataHandle::Null\n"
        "Ui::DataLayer::setIndex(): invalid handle Ui::DataHandle::Null\n"
        "Ui::DataLayer::setIndex(): invalid handle Ui::LayerDataHandle::Null\n"
        "Ui::DataLayer::setIndex(): invalid handle Ui::DataHandle::Null\n"
        "Ui::DataLayer::setIndex(): invalid handle Ui::LayerDataHandle::Null\n",
        TestSuite::Compare::String);
}

void DataLayerTest::indexLinearization() {
    auto&& data = IndexLinearizationData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct DummyStorage: AbstractStorage {
        explicit DummyStorage(DataLayer& layer, const Containers::Size3D& size): AbstractStorage{layer, size} {}

        Int operator[](const Containers::Size3D&) const { return {}; }
    };

    DataLayer layer{layerHandle(0, 1)};

    /* Create some extra storages to verify it's always correctly rebuilding
       even non-trivial storage handles from the ID */
    DummyStorage{layer, {1, 1, 1}};
    layer.removeStorage(DummyStorage{layer, {1, 1, 1}});
    layer.removeStorage(DummyStorage{layer, {1, 1, 1}});
    layer.removeStorage(DummyStorage{layer, {1, 1, 1}});
    layer.removeStorage(DummyStorage{layer, {1, 1, 1}});
    DummyStorage storage{layer, data.size};

    /* Index set initially, verify it won't stomp over the other properties */
    DataHandle indexInitial = layer.create(storage, data.index, [](Int) {});
    CORRADE_COMPARE(layer.index(indexInitial), data.index);
    CORRADE_VERIFY(layer.isDirty(indexInitial));
    CORRADE_COMPARE(layer.storage(indexInitial), storage);

    /* Index set only subsequently through setIndex() */
    DataHandle indexLater = layer.create(storage, Containers::Size3D{}, [](Int) {});
    layer.setIndex(indexLater, data.index);
    CORRADE_COMPARE(layer.index(indexLater), data.index);
    CORRADE_VERIFY(layer.isDirty(indexLater));
    CORRADE_COMPARE(layer.storage(indexLater), storage);

    /* The index and storage reference should stay intact even after the data
       are no longer dirty. From the outside it doesn't really matter whether
       preUpdate() or update() resets the dirty bits, so call both. */
    layer.preUpdate(LayerState::NeedsCommonDataUpdate);
    layer.update(LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(layer.index(indexInitial), data.index);
    CORRADE_COMPARE(layer.index(indexLater), data.index);
    CORRADE_VERIFY(!layer.isDirty(indexInitial));
    CORRADE_VERIFY(!layer.isDirty(indexLater));
    CORRADE_COMPARE(layer.storage(indexInitial), storage);
    CORRADE_COMPARE(layer.storage(indexLater), storage);
}

#ifndef CORRADE_TARGET_32BIT
void DataLayerTest::indexLinearizationFullStorageCapacity() {
    /* Variant of indexLinearization() with all bits of the linearized index
       used, where all bits of the storage ID are used as well */

    struct DummyStorage: AbstractStorage {
        explicit DummyStorage(DataLayer& layer, std::size_t size): AbstractStorage{layer, size} {}

        Int operator[](std::size_t) const { return {}; }
    };

    DataLayer layer{layerHandle(0, 1)};

    for(std::size_t i = 0; i != (1 << Implementation::DataLayerStorageHandleIdBits) - 1; ++i)
        DummyStorage{layer, 1};

    /* It should be enough to verify just with a 1D size */
    DummyStorage storage{layer, 1ull << (64 - Implementation::DataLayerStorageHandleIdBits - 1)};
    CORRADE_COMPARE(storage.handle(), Ui::storageHandle(layer.handle(), (1ull << 20) - 1, 1));
    CORRADE_COMPARE(storage.size(), (Containers::Size3D{1, 1, 1ull << 43}));

    /* Index set initially, verify it won't stomp over the other properties */
    DataHandle indexInitial = layer.create(storage, (1ull << (64 - Implementation::DataLayerStorageHandleIdBits - 1)) - 1, [](Int) {});
    CORRADE_COMPARE(layer.index(indexInitial), (Containers::Size3D{0, 0, (1ull << 43) - 1}));
    CORRADE_VERIFY(layer.isDirty(indexInitial));
    CORRADE_COMPARE(layer.storage(indexInitial), storage);

    /* Index set only subsequently through setIndex() */
    DataHandle indexLater = layer.create(storage, 0, [](Int) {});
    layer.setIndex(indexLater, (1ull << (64 - Implementation::DataLayerStorageHandleIdBits - 1)) - 1);
    CORRADE_COMPARE(layer.index(indexLater), (Containers::Size3D{0, 0, (1ull << 43) - 1}));
    CORRADE_VERIFY(layer.isDirty(indexLater));
    CORRADE_COMPARE(layer.storage(indexLater), storage);
}
#endif

void DataLayerTest::clean() {
    struct DummyStorage: AbstractStorage {
        explicit DummyStorage(DataLayer& layer): AbstractStorage{layer} {}

        Int operator*() const {
            ++storageCalled;
            return 0x333;
        }
    };

    Int destructed = 0,
        anotherDestructed = 0;
    struct NonTrivial {
        explicit NonTrivial(int& output): destructedCount{&output} {}
        ~NonTrivial() {
            ++*destructedCount;
        }
        void operator()(Int) const {
            CORRADE_FAIL("This should never be called.");
        }

        Int* destructedCount;
    };

    DataLayer layer{layerHandle(0, 1)};
    DummyStorage storage{layer};

    DataHandle trivial = layer.create(storage, [](Int) {}, nodeHandle(0, 7));
    CORRADE_COMPARE(layer.usedCount(), 1);
    CORRADE_COMPARE(layer.usedAllocatedCount(), 0);
    CORRADE_VERIFY(!layer.isAllocated(trivial));

    /* The temporary gets destructed right away */
    DataHandle nonTrivial = layer.create(storage, NonTrivial{destructed}, nodeHandle(1, 11));
    CORRADE_COMPARE(destructed, 1);
    CORRADE_COMPARE(layer.usedCount(), 2);
    CORRADE_COMPARE(layer.usedAllocatedCount(), 1);
    CORRADE_VERIFY(layer.isAllocated(nonTrivial));

    DataHandle another = layer.create(storage, [](Int) {}, nodeHandle(2, 23));
    CORRADE_COMPARE(layer.usedCount(), 3);
    CORRADE_COMPARE(layer.usedAllocatedCount(), 1);
    CORRADE_VERIFY(!layer.isAllocated(another));

    /* The temporary gets destructed right away */
    DataHandle anotherNonTrivial = layer.create(storage, NonTrivial{anotherDestructed}, nodeHandle(3, 17));
    CORRADE_COMPARE(anotherDestructed, 1);
    CORRADE_COMPARE(layer.usedCount(), 4);
    CORRADE_COMPARE(layer.usedAllocatedCount(), 2);
    CORRADE_VERIFY(layer.isAllocated(anotherNonTrivial));

    /* It should remove two but call just one destructor */
    UnsignedShort nodeHandleGenerations[]{
        7 + 1,
        11,
        23,
        17 + 1
    };
    layer.cleanNodes(nodeHandleGenerations);
    CORRADE_COMPARE(layer.usedCount(), 2);
    CORRADE_COMPARE(layer.usedAllocatedCount(), 1);
    CORRADE_COMPARE(destructed, 1);
    CORRADE_COMPARE(anotherDestructed, 2);
    CORRADE_VERIFY(!layer.isHandleValid(trivial));
    CORRADE_VERIFY(layer.isHandleValid(nonTrivial));
    CORRADE_VERIFY(layer.isHandleValid(another));
    CORRADE_VERIFY(!layer.isHandleValid(anotherNonTrivial));
}

void DataLayerTest::update() {
    /* "Integration" test that verifies the whole setup with dirty flags
       causing functions to be called */
    /** @todo Since currently all operation is done in doPreUpdate() regardless
        of what nodes are visible / culled / ..., and whether the data are
        attached to any node at all, it just calls preUpdate() directly. Once
        it's visibility-aware, this test should do the whole setup with a UI
        instance as well. */

    DataLayer layer{layerHandle(0, 1)};

    struct StorageUnused: AbstractStorage {
        explicit StorageUnused(DataLayer& layer): AbstractStorage{layer, 22} {}
    } storageUnused{layer};

    struct Storage: AbstractStorage {
        explicit Storage(DataLayer& layer): AbstractStorage{layer} {}

        Float leet() const { return 1.337f; }
    } storage{layer},
      storageRemoved{layer};

    struct Storage2D: AbstractStorage {
        explicit Storage2D(DataLayer& layer): AbstractStorage{layer, {2, 3}} {
            Utility::copy(
                {37, 38, 39, 56, 55, 54},
                Containers::arrayView(createInPlace<Short>()).prefix(6));
        }

        Short operator[](const Containers::Size2D& index) const {
            return data<Short>()[index[0]*3 + index[1]];
        }

        void set(const Containers::Size2D& index, Short value) const {
            data<Short>()[index[0]*3 + index[1]] = value;
            setDirty();
        }
    } storage2D{layer};

    Int firstCalled = 0;
    Int firstExpected = 56;
    DataHandle first = layer.create(storage2D, {1, 0}, [&firstCalled, &firstExpected](Short value) {
        CORRADE_COMPARE(value, firstExpected);
        ++firstCalled;
    });

    DataHandle removed = layer.create(storage, &Storage::leet, [](Float) {
        CORRADE_FAIL("This function shouldn't be called.");
    });

    Int secondCalled = 0;
    DataHandle second = layer.create(storage, &Storage::leet, [&secondCalled](Float value) {
        CORRADE_COMPARE(value, 1.337f);
        ++secondCalled;
    });

    /* Initially no storages are dirty, and all data are, and the layer
       advertises needing an update */
    CORRADE_VERIFY(!storageUnused.isDirty());
    CORRADE_VERIFY(!storage.isDirty());
    CORRADE_VERIFY(!storageRemoved.isDirty());
    CORRADE_VERIFY(!storage2D.isDirty());
    CORRADE_VERIFY(layer.isDirty(first));
    CORRADE_VERIFY(layer.isDirty(removed));
    CORRADE_VERIFY(layer.isDirty(second));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate);

    /* Remove the to-be-removed things, which then requires cleanData() to be
       called */
    /** @todo make it set NeedsDataClean only if any animators are actually
        attached */
    layer.removeStorage(storageRemoved);
    layer.remove(removed);
    CORRADE_COMPARE_AS(layer.state(), LayerState::NeedsDataClean,
        TestSuite::Compare::GreaterOrEqual);
    layer.cleanData({});
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate);

    /* The first update thus calls all updates except for the removed one, and
       clears all dirty bits + states. From the outside it doesn't really
       matter whether preUpdate() or update() does it, so call both. */
    layer.preUpdate(LayerState::NeedsCommonDataUpdate);
    layer.update(LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(firstCalled, 1);
    CORRADE_COMPARE(secondCalled, 1);
    CORRADE_VERIFY(!storageUnused.isDirty());
    CORRADE_VERIFY(!storage.isDirty());
    CORRADE_VERIFY(!storage2D.isDirty());
    CORRADE_VERIFY(!layer.isDirty(first));
    CORRADE_VERIFY(!layer.isDirty(second));
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Updating again does nothing */
    layer.preUpdate(LayerState::NeedsCommonDataUpdate);
    layer.update(LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(firstCalled, 1);
    CORRADE_COMPARE(secondCalled, 1);
    CORRADE_VERIFY(!storageUnused.isDirty());
    CORRADE_VERIFY(!storage.isDirty());
    CORRADE_VERIFY(!storage2D.isDirty());
    CORRADE_VERIFY(!layer.isDirty(first));
    CORRADE_VERIFY(!layer.isDirty(second));
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Adding new data makes the layer dirty */
    Int thirdCalled = 0;
    DataHandle third = layer.create(storage2D, {0, 2}, [&thirdCalled](Short value) {
        CORRADE_COMPARE(value, 39);
        ++thirdCalled;
    });
    CORRADE_VERIFY(layer.isDirty(third));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate);

    /* Updating again calls just the new data update, nothing else */
    layer.preUpdate(LayerState::NeedsCommonDataUpdate);
    layer.update(LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(firstCalled, 1);
    CORRADE_COMPARE(secondCalled, 1);
    CORRADE_COMPARE(thirdCalled, 1);
    CORRADE_VERIFY(!storageUnused.isDirty());
    CORRADE_VERIFY(!storage.isDirty());
    CORRADE_VERIFY(!storage2D.isDirty());
    CORRADE_VERIFY(!layer.isDirty(first));
    CORRADE_VERIFY(!layer.isDirty(second));
    CORRADE_VERIFY(!layer.isDirty(third));
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Marking a storage dirty causes the layer state to be set. Per-data dirty
       flags aren't set by this. */
    storage.setDirty();
    CORRADE_VERIFY(!storageUnused.isDirty());
    CORRADE_VERIFY(storage.isDirty());
    CORRADE_VERIFY(!storage2D.isDirty());
    CORRADE_VERIFY(!layer.isDirty(first));
    CORRADE_VERIFY(!layer.isDirty(second));
    CORRADE_VERIFY(!layer.isDirty(third));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);

    /* Updating again calls just the affected data update, and resets the
       storage dirty bit */
    layer.preUpdate(LayerState::NeedsCommonDataUpdate);
    layer.update(LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(firstCalled, 1);
    CORRADE_COMPARE(secondCalled, 2);
    CORRADE_COMPARE(thirdCalled, 1);
    CORRADE_VERIFY(!storageUnused.isDirty());
    CORRADE_VERIFY(!storage.isDirty());
    CORRADE_VERIFY(!storage2D.isDirty());
    CORRADE_VERIFY(!layer.isDirty(first));
    CORRADE_VERIFY(!layer.isDirty(second));
    CORRADE_VERIFY(!layer.isDirty(third));
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Marking an unused storage dirty doesn't set the layer state because it
       won't need to do anything */
    storageUnused.setDirty();
    CORRADE_VERIFY(storageUnused.isDirty());
    CORRADE_VERIFY(!storage.isDirty());
    CORRADE_VERIFY(!storage2D.isDirty());
    CORRADE_VERIFY(!layer.isDirty(first));
    CORRADE_VERIFY(!layer.isDirty(second));
    CORRADE_VERIFY(!layer.isDirty(third));
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* But updating clears it as well */
    layer.preUpdate(LayerState::NeedsCommonDataUpdate);
    layer.update(LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(firstCalled, 1);
    CORRADE_COMPARE(secondCalled, 2);
    CORRADE_COMPARE(thirdCalled, 1);
    CORRADE_VERIFY(!storageUnused.isDirty());
    CORRADE_VERIFY(!storage.isDirty());
    CORRADE_VERIFY(!storage2D.isDirty());
    CORRADE_VERIFY(!layer.isDirty(first));
    CORRADE_VERIFY(!layer.isDirty(second));
    CORRADE_VERIFY(!layer.isDirty(third));
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Modifying storage (which implicitly marks it as dirty) that's used by
       more than one data */
    storage2D.set({1, 0}, -57);
    CORRADE_VERIFY(!storageUnused.isDirty());
    CORRADE_VERIFY(!storage.isDirty());
    CORRADE_VERIFY(storage2D.isDirty());
    CORRADE_VERIFY(!layer.isDirty(first));
    CORRADE_VERIFY(!layer.isDirty(second));
    CORRADE_VERIFY(!layer.isDirty(third));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);

    /* Updating calls the update function on all data associated with the
       storage */
    firstExpected = -57;
    layer.preUpdate(LayerState::NeedsCommonDataUpdate);
    layer.update(LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(firstCalled, 2);
    CORRADE_COMPARE(secondCalled, 2);
    CORRADE_COMPARE(thirdCalled, 2);
    CORRADE_VERIFY(!storageUnused.isDirty());
    CORRADE_VERIFY(!storage.isDirty());
    CORRADE_VERIFY(!storage2D.isDirty());
    CORRADE_VERIFY(!layer.isDirty(first));
    CORRADE_VERIFY(!layer.isDirty(second));
    CORRADE_VERIFY(!layer.isDirty(third));
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Setting an index marks the data as dirty */
    layer.setIndex(first, {1, 2});
    CORRADE_VERIFY(!storageUnused.isDirty());
    CORRADE_VERIFY(!storage.isDirty());
    CORRADE_VERIFY(!storage2D.isDirty());
    CORRADE_VERIFY(layer.isDirty(first));
    CORRADE_VERIFY(!layer.isDirty(second));
    CORRADE_VERIFY(!layer.isDirty(third));
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);

    /* Updating calls only given data update, and resets both the dirty bit and
       layer state again */
    firstExpected = 54;
    layer.preUpdate(LayerState::NeedsCommonDataUpdate);
    layer.update(LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});
    CORRADE_COMPARE(firstCalled, 3);
    CORRADE_COMPARE(secondCalled, 2);
    CORRADE_COMPARE(thirdCalled, 2);
    CORRADE_VERIFY(!storageUnused.isDirty());
    CORRADE_VERIFY(!storage.isDirty());
    CORRADE_VERIFY(!storage2D.isDirty());
    CORRADE_VERIFY(!layer.isDirty(first));
    CORRADE_VERIFY(!layer.isDirty(second));
    CORRADE_VERIFY(!layer.isDirty(third));
    CORRADE_COMPARE(layer.state(), LayerStates{});
}

void DataLayerTest::referenceCounted() {
    auto&& data = ReferenceCountedData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Verifies behavior with storages marked StorageFlag::ReferenceCounted.
       Using a real UI instance to ensure there aren't no weird interactions
       caused by NeedsCommonDataUpdate being called from doClean(). */

    /* Note that the referenced variables have to be *before* the UI in
       order for them to be still in scope when the UI (and thus the layer)
       destructor destructs remaining functions. ASan complains otherwise. */
    Int storageUnusedDestructed = 0,
        storageReferenceCountedDestructed = 0,
        storageReferenceCountedUnusedDestructed = 0,
        storageReferenceCountedRemovedDestructed = 0,
        storageReferenceCountedUnused2Destructed = 0;
    struct NonTrivialStorage: AbstractStorage {
        explicit NonTrivialStorage(DataLayer& layer, Int& destructed, StorageFlags flags): AbstractStorage{layer, flags} {
            createAllocated(&destructed, 0, [](void* data, std::size_t) {
                ++*static_cast<Int*>(data);
            });
        }

        Int operator*() const { return {}; }
    };

    AbstractUserInterface ui{{100, 100}};

    NodeHandle node1 = data.removeNodes ? ui.createNode({}, {}) : NodeHandle::Null;
    NodeHandle node2 = data.removeNodes ? ui.createNode({}, {}) : NodeHandle::Null;

    DataLayer& layer = ui.setLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));

    /* Creating a regular storage doesn't set any LayerState */
    NonTrivialStorage storageUnused{layer, storageUnusedDestructed, {}};
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Creating reference-counted storage does in order to trigger an update
       and remove the storage if it ends up being unreferenced */
    NonTrivialStorage storageReferenceCounted{layer, storageReferenceCountedDestructed, StorageFlag::ReferenceCounted};
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);

    /* Reference-counted storages that are unused should stay alive until the
       first update() */
    NonTrivialStorage storageReferenceCountedUnused{layer, storageReferenceCountedUnusedDestructed, StorageFlag::ReferenceCounted};
    NonTrivialStorage storageReferenceCountedRemoved{layer, storageReferenceCountedRemovedDestructed, StorageFlag::ReferenceCounted};
    DataHandle data1 = layer.create(storageReferenceCounted, [](Int) {}, node1);
    DataHandle data2 = layer.create(storageReferenceCounted, [](Int) {}, node2);
    layer.removeStorage(storageReferenceCountedRemoved);
    CORRADE_COMPARE(storageUnusedDestructed, 0);
    CORRADE_COMPARE(storageReferenceCountedDestructed, 0);
    CORRADE_COMPARE(storageReferenceCountedUnusedDestructed, 0);
    CORRADE_COMPARE(storageReferenceCountedRemovedDestructed, 1);
    CORRADE_COMPARE(layer.storageCapacity(), 4);
    CORRADE_COMPARE(layer.storageUsedCount(), 3);
    CORRADE_VERIFY(layer.isHandleValid(storageUnused));
    CORRADE_VERIFY(layer.isHandleValid(storageReferenceCounted));
    CORRADE_VERIFY(layer.isHandleValid(storageReferenceCountedUnused));
    CORRADE_COMPARE(layer.storageReferenceCount(storageUnused), 0);
    CORRADE_COMPARE(layer.storageReferenceCount(storageReferenceCounted), 2);
    CORRADE_COMPARE(layer.storageReferenceCount(storageReferenceCountedUnused), 0);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsDataUpdate|LayerState::NeedsCommonDataUpdate|data.extraStatesCreate);

    /* The first update then removes the unused reference counted storage, but
       not the unused non-reference-counted storage */
    ui.update();
    CORRADE_COMPARE(storageUnusedDestructed, 0);
    CORRADE_COMPARE(storageReferenceCountedDestructed, 0);
    CORRADE_COMPARE(storageReferenceCountedUnusedDestructed, 1);
    CORRADE_COMPARE(storageReferenceCountedRemovedDestructed, 1);
    CORRADE_VERIFY(layer.isHandleValid(storageUnused));
    CORRADE_VERIFY(layer.isHandleValid(storageReferenceCounted));
    CORRADE_VERIFY(!layer.isHandleValid(storageReferenceCountedUnused));
    CORRADE_COMPARE(layer.storageReferenceCount(storageUnused), 0);
    CORRADE_COMPARE(layer.storageReferenceCount(storageReferenceCounted), 2);
    CORRADE_COMPARE(layer.storageCapacity(), 4);
    CORRADE_COMPARE(layer.storageUsedCount(), 2);
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Removing a data causes the refcount to be decremented. If not zero yet,
       LayerState::NeedsCommonDataUpdate is not set. */
    if(!data.removeNodes) {
        layer.remove(data2);
    } else {
        ui.removeNode(node2);
        ui.clean();
    }
    CORRADE_COMPARE(layer.storageReferenceCount(storageReferenceCounted), 1);
    CORRADE_COMPARE(layer.state(), data.extraStatesRemove);

    /* Removing the other makes the refcount zero, and causes LayerState to be
       set */
    if(!data.removeNodes) {
        layer.remove(data1);
    } else {
        ui.removeNode(node1);
        ui.clean();
    }
    CORRADE_COMPARE(layer.storageReferenceCount(storageReferenceCounted), 0);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate|data.extraStatesRemove);

    /* The following update then removes the storage. Again the other unused
       but not reference-counted storage is left untouched. */
    ui.update();
    CORRADE_COMPARE(storageUnusedDestructed, 0);
    CORRADE_COMPARE(storageReferenceCountedDestructed, 1);
    CORRADE_COMPARE(storageReferenceCountedUnusedDestructed, 1);
    CORRADE_COMPARE(storageReferenceCountedRemovedDestructed, 1);
    CORRADE_VERIFY(layer.isHandleValid(storageUnused));
    CORRADE_VERIFY(!layer.isHandleValid(storageReferenceCounted));
    CORRADE_VERIFY(!layer.isHandleValid(storageReferenceCountedUnused));
    CORRADE_COMPARE(layer.storageReferenceCount(storageUnused), 0);
    CORRADE_COMPARE(layer.storageCapacity(), 4);
    CORRADE_COMPARE(layer.storageUsedCount(), 1);
    CORRADE_COMPARE(layer.state(), LayerStates{});

    /* Create another unused reference-counted storage */
    NonTrivialStorage storageReferenceCountedUnused2{layer, storageReferenceCountedUnused2Destructed, StorageFlag::ReferenceCounted};
    CORRADE_COMPARE(storageUnusedDestructed, 0);
    CORRADE_COMPARE(storageReferenceCountedDestructed, 1);
    CORRADE_COMPARE(storageReferenceCountedUnusedDestructed, 1);
    CORRADE_COMPARE(storageReferenceCountedUnused2Destructed, 0);
    CORRADE_COMPARE(storageReferenceCountedRemovedDestructed, 1);
    CORRADE_VERIFY(layer.isHandleValid(storageUnused));
    CORRADE_VERIFY(!layer.isHandleValid(storageReferenceCounted));
    CORRADE_VERIFY(!layer.isHandleValid(storageReferenceCountedUnused));
    CORRADE_VERIFY(layer.isHandleValid(storageReferenceCountedUnused2));
    CORRADE_COMPARE(layer.storageReferenceCount(storageUnused), 0);
    CORRADE_COMPARE(layer.storageCapacity(), 4);
    CORRADE_COMPARE(layer.storageUsedCount(), 2);
    CORRADE_COMPARE(layer.state(), LayerState::NeedsCommonDataUpdate);

    /* ... and then directly update to verify this works also with no other
       change being done */
    ui.update();
    CORRADE_COMPARE(storageUnusedDestructed, 0);
    CORRADE_COMPARE(storageReferenceCountedDestructed, 1);
    CORRADE_COMPARE(storageReferenceCountedUnusedDestructed, 1);
    CORRADE_COMPARE(storageReferenceCountedUnused2Destructed, 1);
    CORRADE_COMPARE(storageReferenceCountedRemovedDestructed, 1);
    CORRADE_VERIFY(layer.isHandleValid(storageUnused));
    CORRADE_VERIFY(!layer.isHandleValid(storageReferenceCounted));
    CORRADE_VERIFY(!layer.isHandleValid(storageReferenceCountedUnused));
    CORRADE_VERIFY(!layer.isHandleValid(storageReferenceCountedUnused2));
    CORRADE_COMPARE(layer.storageReferenceCount(storageUnused), 0);
    CORRADE_COMPARE(layer.storageCapacity(), 4);
    CORRADE_COMPARE(layer.storageUsedCount(), 1);
    CORRADE_COMPARE(layer.state(), LayerStates{});
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::DataLayerTest)
