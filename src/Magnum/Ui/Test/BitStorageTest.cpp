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

#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Containers/StridedBitArrayView.h>
#include <Corrade/Containers/String.h>
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/Container.h>
#include <Corrade/TestSuite/Compare/String.h>
#include <Magnum/Math/Vector2.h> /* AbstractUserInterface constructor size */

#include "Magnum/Ui/BitStorage.h"
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/UserInterface.h"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct BitStorageTest: TestSuite::Tester {
    explicit BitStorageTest();

    void constructValueInit();
    void constructNoInit();
    void constructDirectInit();
    #ifdef CORRADE_TARGET_32BIT
    /* BitArrayView etc. also asserts for this only on 32-bit, on 64-bit it's
       significantly less likely to hit the limit */
    void constructSizeTooLarge();
    #endif
    void constructNonOwned1D();
    void constructNonOwned();
    void constructCopy();

    void constructHandleRecycle();

    /* Verifies that both query and set accesses the right address and bit
       offset in all dimensions */
    void access1D();
    void access();

    /* Verifies that both query and updater accesses the right address and bit
       offset in all dimensions and stride variants, as well as immutable
       behavior */
    void accessNonOwned1D();
    void accessNonOwned();

    void accessInvalid();
    void nonOwnedMutableDataInvalid();

    /* Verifies that updates to a value all do the right thing including
       setting a dirty bit, both in owned and non-owned storages */
    void update();

    void updateDefaultValue();
};

const struct {
    const char* name;
    bool implicitLayer;
    std::size_t size;
    bool expectAllocated;
} ConstructData[]{
    {"max in-place size", false,
        /* Two bits used to store the NonOwned and DefaultTrue flags */
        #ifndef CORRADE_TARGET_32BIT
        32*8 - 2,
        #else
        20*8 - 2,
        #endif
        false},
    {"min allocated size", false,
        #ifndef CORRADE_TARGET_32BIT
        32*8 - 1,
        #else
        20*8 - 1,
        #endif
        true},
    {"implicit layer", true, 100, false}
};

const struct {
    const char* name;
    bool implicitLayer;
    bool value;
    std::size_t size;
    bool expectAllocated;
} ConstructDirectInitData[]{
    {"max in-place size", false, true,
        /* Two bits used to store the NonOwned and DefaultTrue flags */
        #ifndef CORRADE_TARGET_32BIT
        32*8 - 2,
        #else
        20*8 - 2,
        #endif
        false},
    {"min allocated size", false, true,
        #ifndef CORRADE_TARGET_32BIT
        32*8 - 1,
        #else
        20*8 - 1,
        #endif
        true},
    {"direct init to zero", true, false, 100, false},
    {"implicit layer", true, true, 100, false}
};

const struct {
    const char* name;
    bool implicitLayer;
    std::size_t size, stride, offset;
} ConstructNonOwnedData[]{
    {"", false, 3, 4, 5},
    {"zero bit offset", false, 3, 4, 0},
    {"max bit offset", false, 3, 4, 7},
    /* 3 bits from the size are used to store bit offset, verify everything up
       to that can be created. Stride is zero in this case because otherwise
       the size would be impossible to allocate. */
    {"max possible size", false,
        (std::size_t{1} << (sizeof(std::size_t)*8 - 3)) - 1, 0, 0},
    {"max possible size, max bit offset", false,
        (std::size_t{1} << (sizeof(std::size_t)*8 - 3)) - 1, 0, 7},
    {"implicit layer", true, 3, 4, 5},
};

const struct {
    const char* name;
    bool immutable;
    std::size_t valueIndex;
    std::size_t offset;
    std::ptrdiff_t stride;
} AccessNonOwned1DData[]{
    {"", false, 2, 4, 1},
    {"immutable", true, 2, 4, 1},
    {"sparse", false, 9, 2, 5},
    {"negative stride", false, 1, 37, -2},
    {"zero stride", false, 5, 45, 0},
};

const struct {
    const char* name;
    bool immutable;
} AccessNonOwnedData[]{
    {"", false},
    {"immutable", true},
};

const struct {
    const char* name;
    bool nonOwned;
} UpdateData[]{
    {"", false},
    {"non-owned", true},
};

const struct {
    TestSuite::TestCaseDescriptionSourceLocation name;

    bool value;
    Containers::Optional<bool> defaultValue;
    StorageOperation operation;
    Containers::Optional<bool> set;

    /* StorageUpdateState::Success is expected always */
    bool expectedDirty;
    bool expected;
} UpdateDefaultValueData[]{
    {"set",
        false, {}, StorageOperation::Set, true,
        true, true},
    {"set to false",
        true, {}, StorageOperation::Set, false,
        true, false},
    {"set to the same value",
        true, {}, StorageOperation::Set, true,
        false, true},

    {"reset",
        true, {}, StorageOperation::Reset, {},
        true, false},
    {"reset when already false",
        false, {}, StorageOperation::Reset, {},
        false, false},
    {"reset with default explicitly being false",
        true, false, StorageOperation::Reset, {},
        true, false},
    {"reset with default explicitly being false when already that",
        false, false, StorageOperation::Reset, {},
        false, false},
    {"reset with default being true",
        false, true, StorageOperation::Reset, {},
        true, true},
    {"reset with default being true when already that",
        true, true, StorageOperation::Reset, {},
        false, true},

    {"toggle from false",
        false, {}, StorageOperation::Toggle, {},
        true, true},
    {"toggle from true",
        true, {}, StorageOperation::Toggle, {},
        true, false},
};

BitStorageTest::BitStorageTest() {
    addInstancedTests({&BitStorageTest::constructValueInit,
                       &BitStorageTest::constructNoInit},
        Containers::arraySize(ConstructData));

    addInstancedTests({&BitStorageTest::constructDirectInit},
        Containers::arraySize(ConstructDirectInitData));

    #ifdef CORRADE_TARGET_32BIT
    addTests({&BitStorageTest::constructSizeTooLarge});
    #endif

    addInstancedTests({&BitStorageTest::constructNonOwned1D,
                       &BitStorageTest::constructNonOwned},
        Containers::arraySize(ConstructNonOwnedData));

    addTests({&BitStorageTest::constructCopy,

              &BitStorageTest::constructHandleRecycle,

              &BitStorageTest::access1D,
              &BitStorageTest::access});

    addInstancedTests({&BitStorageTest::accessNonOwned1D},
        Containers::arraySize(AccessNonOwned1DData));

    addInstancedTests({&BitStorageTest::accessNonOwned},
        Containers::arraySize(AccessNonOwnedData));

    addTests({&BitStorageTest::accessInvalid,
              &BitStorageTest::nonOwnedMutableDataInvalid});

    addInstancedTests({&BitStorageTest::update},
        Containers::arraySize(UpdateData));

    addInstancedTests({&BitStorageTest::updateDefaultValue},
        Containers::arraySize(UpdateDefaultValueData));
}

void BitStorageTest::constructValueInit() {
    auto&& data = ConstructData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Either this instance or the implicit one below gets used */
    DataLayer layer{layerHandle(0, 1)};

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    ui.setDataLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));

    BitStorage first1 = data.implicitLayer ?
        BitStorage{ui, ValueInit, data.size, StorageFlags{0x18}} :
        BitStorage{layer, ValueInit, data.size, StorageFlags{0x18}};
    BitStorage first2 = data.implicitLayer ?
        BitStorage{ui, data.size, StorageFlags{0x18}} :
        BitStorage{layer, data.size, StorageFlags{0x18}};
    CORRADE_COMPARE(&first1.layer(), data.implicitLayer ? &ui.dataLayer() : &layer);
    CORRADE_COMPARE(&first2.layer(), data.implicitLayer ? &ui.dataLayer() : &layer);
    CORRADE_COMPARE(first1.isAllocated(), data.expectAllocated);
    CORRADE_COMPARE(first2.isAllocated(), data.expectAllocated);
    CORRADE_VERIFY(!first1.isDirty());
    CORRADE_VERIFY(!first2.isDirty());
    CORRADE_COMPARE(first1.flags(), StorageFlags{0x18});
    CORRADE_COMPARE(first2.flags(), StorageFlags{0x18});
    CORRADE_COMPARE(first1.size(), data.size);
    CORRADE_COMPARE(first2.size(), data.size);
    CORRADE_VERIFY(first1.isMutable());
    CORRADE_VERIFY(first2.isMutable());
    CORRADE_COMPARE(first1.defaultValue(), false);
    CORRADE_COMPARE(first2.defaultValue(), false);
    /* Value access APIs tested in access1D() below */

    /* The data should have the expected shape and be a contiguous sequence of
       zero bits */
    Containers::StridedBitArrayView1D viewFirst1 = first1.data();
    Containers::StridedBitArrayView1D viewFirst2 = first2.data();
    CORRADE_COMPARE(viewFirst1.offset(), 2);
    CORRADE_COMPARE(viewFirst2.offset(), 2);
    CORRADE_COMPARE(viewFirst1.size(), data.size);
    CORRADE_COMPARE(viewFirst2.size(), data.size);
    CORRADE_COMPARE(viewFirst1.stride(), 1);
    CORRADE_COMPARE(viewFirst2.stride(), 1);
    CORRADE_VERIFY(viewFirst1.isContiguous());
    CORRADE_VERIFY(viewFirst2.isContiguous());
    CORRADE_COMPARE_AS(viewFirst1.asContiguous(),
        Containers::stridedArrayView({false}).broadcasted<0>(data.size).sliceBit(0),
        TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(viewFirst2.asContiguous(),
        Containers::stridedArrayView({false}).broadcasted<0>(data.size).sliceBit(0),
        TestSuite::Compare::Container);
    /* Mutable data should be the same */
    CORRADE_COMPARE(first1.mutableData().data(), viewFirst1.data());
    CORRADE_COMPARE(first2.mutableData().data(), viewFirst2.data());
    CORRADE_COMPARE(first1.mutableData().size(), viewFirst1.size());
    CORRADE_COMPARE(first2.mutableData().size(), viewFirst2.size());
    CORRADE_COMPARE(first1.mutableData().offset(), viewFirst1.offset());
    CORRADE_COMPARE(first2.mutableData().offset(), viewFirst2.offset());
    CORRADE_COMPARE(first1.mutableData().stride(), viewFirst1.stride());
    CORRADE_COMPARE(first2.mutableData().stride(), viewFirst2.stride());

    BitStorage second1 = data.implicitLayer ?
        BitStorage{ui, ValueInit, StorageFlags{0x28}} :
        BitStorage{layer, ValueInit, StorageFlags{0x28}};
    BitStorage second2 = data.implicitLayer ?
        BitStorage{ui, StorageFlags{0x28}} :
        BitStorage{layer, StorageFlags{0x28}};
    CORRADE_COMPARE(&second1.layer(), data.implicitLayer ? &ui.dataLayer() : &layer);
    CORRADE_COMPARE(&second2.layer(), data.implicitLayer ? &ui.dataLayer() : &layer);
    /* Single-item storage should fit in-place always */
    CORRADE_VERIFY(!second1.isAllocated());
    CORRADE_VERIFY(!second2.isAllocated());
    CORRADE_VERIFY(!second1.isDirty());
    CORRADE_VERIFY(!second2.isDirty());
    CORRADE_COMPARE(second1.flags(), StorageFlags{0x28});
    CORRADE_COMPARE(second2.flags(), StorageFlags{0x28});
    CORRADE_COMPARE(second1.size(), 1);
    CORRADE_COMPARE(second2.size(), 1);
    /* Value access APIs tested in access() below */
    Containers::StridedBitArrayView1D viewSecond1 = second1.data();
    Containers::StridedBitArrayView1D viewSecond2 = second2.data();
    CORRADE_COMPARE(viewSecond1.size(), 1);
    CORRADE_COMPARE(viewSecond2.size(), 1);
    CORRADE_COMPARE(viewSecond1.offset(), 2);
    CORRADE_COMPARE(viewSecond2.offset(), 2);
    CORRADE_COMPARE(viewSecond1.stride(), 1);
    CORRADE_COMPARE(viewSecond2.stride(), 1);
    CORRADE_VERIFY(viewSecond1.isContiguous());
    CORRADE_VERIFY(viewSecond2.isContiguous());
    CORRADE_COMPARE_AS(viewSecond1.asContiguous(),
        Containers::stridedArrayView({false}).sliceBit(0),
        TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(viewSecond2.asContiguous(),
        Containers::stridedArrayView({false}).sliceBit(0),
        TestSuite::Compare::Container);
    /* Mutable data should be the same */
    CORRADE_COMPARE(second1.mutableData().data(), viewSecond1.data());
    CORRADE_COMPARE(second2.mutableData().data(), viewSecond2.data());
    CORRADE_COMPARE(second1.mutableData().offset(), viewSecond1.offset());
    CORRADE_COMPARE(second2.mutableData().offset(), viewSecond2.offset());
    CORRADE_COMPARE(second1.mutableData().size(), viewSecond1.size());
    CORRADE_COMPARE(second2.mutableData().size(), viewSecond2.size());
    CORRADE_COMPARE(second1.mutableData().stride(), viewSecond1.stride());
    CORRADE_COMPARE(second2.mutableData().stride(), viewSecond2.stride());
}

void BitStorageTest::constructNoInit() {
    auto&& data = ConstructData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Copy of constructValueInit() with NoInit-specific differences */

    /* Either this instance or the implicit one below gets used */
    DataLayer layer{layerHandle(0, 1)};

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    ui.setDataLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));

    BitStorage first = data.implicitLayer ?
        BitStorage{ui, NoInit, data.size, StorageFlags{0x18}} :
        BitStorage{layer, NoInit, data.size, StorageFlags{0x18}};
    CORRADE_COMPARE(&first.layer(), data.implicitLayer ? &ui.dataLayer() : &layer);
    CORRADE_COMPARE(first.isAllocated(), data.expectAllocated);
    CORRADE_VERIFY(!first.isDirty());
    CORRADE_COMPARE(first.flags(), StorageFlags{0x18});
    CORRADE_COMPARE(first.size(), data.size);
    CORRADE_VERIFY(first.isMutable());
    CORRADE_COMPARE(first.defaultValue(), false);

    /* The data should have the expected shape. Can't verify the contents as
       the allocation can have just anything. */
    Containers::StridedBitArrayView1D view = first.data();
    CORRADE_COMPARE(view.offset(), 2);
    CORRADE_COMPARE(view.size(), data.size);
    CORRADE_COMPARE(view.stride(), 1);
    CORRADE_VERIFY(view.isContiguous());
    /* Mutable data should be the same */
    CORRADE_COMPARE(first.mutableData().data(), view.data());
    CORRADE_COMPARE(first.mutableData().size(), view.size());
    CORRADE_COMPARE(first.mutableData().offset(), view.offset());
    CORRADE_COMPARE(first.mutableData().stride(), view.stride());

    BitStorage second = data.implicitLayer ?
        BitStorage{ui, NoInit, StorageFlags{0x28}} :
        BitStorage{layer, NoInit, StorageFlags{0x28}};
    CORRADE_COMPARE(&second.layer(), data.implicitLayer ? &ui.dataLayer() : &layer);
    /* Single-item storage should fit in-place always */
    CORRADE_VERIFY(!second.isAllocated());
    CORRADE_VERIFY(!second.isDirty());
    CORRADE_COMPARE(second.flags(), StorageFlags{0x28});
    CORRADE_COMPARE(second.size(), 1);
    CORRADE_COMPARE(second.data().size(), 1);
    CORRADE_COMPARE(second.data().offset(), 2);
    CORRADE_COMPARE(second.data().stride(), 1);
    CORRADE_COMPARE(second.mutableData().data(), second.data().data());
    CORRADE_COMPARE(second.mutableData().offset(), second.data().offset());
    CORRADE_COMPARE(second.mutableData().size(), second.data().size());
    CORRADE_COMPARE(second.mutableData().stride(), second.data().stride());

    /* If the storage fits in-place, we can fill the storage, remove it and
       then recycle the slot to check that the bits are indeed left over from
       before. Or at except the first up to four bytes, which get overwritten
       with the new flags. For sizes that are allocated anything can happen --
       the exact memory reused, stomped over by something else in the meantime
       or a different memory being allocated next time. */
    if(!data.expectAllocated) {
        /* All ones and zero every 17 bits */
        for(std::size_t i = 0; i != first.size(); ++i)
            first.mutableData().set(i, i % 17 == 0 ? false : true);
        /* The single-bit second storage gets fully overwritten, not testing
           it */

        (data.implicitLayer ? ui.dataLayer() : layer).removeStorage(first);
        (data.implicitLayer ? ui.dataLayer() : layer).removeStorage(second);

        BitStorage first2 = data.implicitLayer ?
            BitStorage{ui, NoInit, data.size} :
            BitStorage{layer, NoInit, data.size};
        CORRADE_COMPARE(storageHandleId(first2.handle()), storageHandleId(first.handle()));
        CORRADE_VERIFY(!first2.isAllocated());
        /* A zero every 17 bits, which means each next 16-bit number is rotated
           by a bit */
        UnsignedShort bits[]{
            UnsignedShort(~1), UnsignedShort(~2), UnsignedShort(~4), UnsignedShort(~8), UnsignedShort(~16), UnsignedShort(~32),
            UnsignedShort(~64), UnsignedShort(~128), UnsignedShort(~256),
            UnsignedShort(~512), UnsignedShort(~1024), UnsignedShort(~2048),
            UnsignedShort(~4096), UnsignedShort(~8192), UnsignedShort(~16384),
            UnsignedShort(~32768),
        };
        /* The first byte gets written to when creating the storage, but the
           compile may optimize that to a 4B write, so skip more */
        CORRADE_COMPARE_AS(first2.data().exceptPrefix(30),
            (Containers::BitArrayView{bits, 0, data.size}).exceptPrefix(30),
            TestSuite::Compare::Container);
    }
}

void BitStorageTest::constructDirectInit() {
    auto&& data = ConstructDirectInitData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Copy of constructValueInit() with DirectInit-specific differences */

    /* Either this instance or the implicit one below gets used */
    DataLayer layer{layerHandle(0, 1)};

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    ui.setDataLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));

    BitStorage first = data.implicitLayer ?
        BitStorage{ui, DirectInit, data.size, data.value, StorageFlags{0x18}} :
        BitStorage{layer, DirectInit, data.size, data.value, StorageFlags{0x18}};
    CORRADE_COMPARE(&first.layer(), data.implicitLayer ? &ui.dataLayer() : &layer);
    CORRADE_COMPARE(first.isAllocated(), data.expectAllocated);
    CORRADE_VERIFY(!first.isDirty());
    CORRADE_COMPARE(first.flags(), StorageFlags{0x18});
    CORRADE_COMPARE(first.size(), data.size);
    CORRADE_VERIFY(first.isMutable());
    CORRADE_COMPARE(first.defaultValue(), false);

    /* The data should have the expected shape and be a contiguous sequence
       repeating the value supplied in the constructor */
    Containers::StridedBitArrayView1D view = first.data();
    CORRADE_COMPARE(view.offset(), 2);
    CORRADE_COMPARE(view.size(), data.size);
    CORRADE_COMPARE(view.stride(), 1);
    CORRADE_VERIFY(view.isContiguous());
    CORRADE_COMPARE_AS(view.asContiguous(),
        Containers::stridedArrayView({data.value}).broadcasted<0>(data.size).sliceBit(0),
        TestSuite::Compare::Container);
    /* Mutable data should be the same */
    CORRADE_COMPARE(first.mutableData().data(), view.data());
    CORRADE_COMPARE(first.mutableData().size(), view.size());
    CORRADE_COMPARE(first.mutableData().offset(), view.offset());
    CORRADE_COMPARE(first.mutableData().stride(), view.stride());

    BitStorage second = data.implicitLayer ?
        BitStorage{ui, DirectInit, data.value, StorageFlags{0x28}} :
        BitStorage{layer, DirectInit, data.value, StorageFlags{0x28}};
    CORRADE_COMPARE(&second.layer(), data.implicitLayer ? &ui.dataLayer() : &layer);
    CORRADE_VERIFY(!second.isAllocated());
    CORRADE_VERIFY(!second.isDirty());
    CORRADE_COMPARE(second.flags(), StorageFlags{0x28});
    CORRADE_COMPARE(second.size(), 1);
    CORRADE_COMPARE(second.data().size(), 1);
    CORRADE_COMPARE(second.data().offset(), 2);
    CORRADE_COMPARE(second.data().stride(), 1);
    CORRADE_COMPARE_AS(second.data().asContiguous(),
        Containers::stridedArrayView({data.value}).sliceBit(0),
        TestSuite::Compare::Container);
    CORRADE_COMPARE(second.mutableData().data(), second.data().data());
    CORRADE_COMPARE(second.mutableData().offset(), second.data().offset());
    CORRADE_COMPARE(second.mutableData().size(), second.data().size());
    CORRADE_COMPARE(second.mutableData().stride(), second.data().stride());
}

#ifdef CORRADE_TARGET_32BIT
void BitStorageTest::constructSizeTooLarge() {
    CORRADE_SKIP_IF_NO_ASSERT();

    DataLayer layer{layerHandle(0, 1)};

    Containers::String out;
    Error redirectError{&out};
    BitStorage{layer, std::size_t{1} << (sizeof(std::size_t)*8 - 3)};
    BitStorage{layer, ValueInit, std::size_t{1} << (sizeof(std::size_t)*8 - 3)};
    BitStorage{layer, NoInit, std::size_t{1} << (sizeof(std::size_t)*8 - 3)};
    BitStorage{layer, DirectInit, std::size_t{1} << (sizeof(std::size_t)*8 - 3), true};
    CORRADE_COMPARE_AS(out,
        "Ui::BitStorage: size expected to be smaller than 2^29 bits, got 536870912\n"
        "Ui::BitStorage: size expected to be smaller than 2^29 bits, got 536870912\n"
        "Ui::BitStorage: size expected to be smaller than 2^29 bits, got 536870912\n"
        "Ui::BitStorage: size expected to be smaller than 2^29 bits, got 536870912\n",
        TestSuite::Compare::String);
}
#endif

void BitStorageTest::constructNonOwned1D() {
    auto&& data = ConstructNonOwnedData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Variant of constructValueInit() testing specifics of 1D NonOwned
       construction. Single-item variant tested in constructNonOwned()
       below. */

    /* Either this instance or the implicit one below gets used */
    DataLayer layer{layerHandle(0, 1)};

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    ui.setDataLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));

    UnsignedInt storageData[3];

    /* Flipped so we don't have a trivial stride */
    Containers::MutableStridedBitArrayView1D storageView =
        Containers::stridedArrayView(Containers::arrayView(storageData), data.size, data.stride).sliceBit(data.offset).template flipped<0>();
    Containers::StridedBitArrayView1D constStorageView = storageView;
    /* The view is flipped so the stride (in bits) should be too */
    CORRADE_COMPARE(storageView.stride(), -std::ptrdiff_t(data.stride*8));

    BitStorage storage1 = data.implicitLayer ?
        BitStorage{ui, NonOwned, storageView, StorageFlags{0x18}} :
        BitStorage{layer, NonOwned, storageView, StorageFlags{0x18}};
    BitStorage storage2 = data.implicitLayer ?
        BitStorage{ui, NonOwned, constStorageView, StorageFlags{0x18}} :
        BitStorage{layer, NonOwned, constStorageView, StorageFlags{0x18}};
    CORRADE_COMPARE(&storage1.layer(), data.implicitLayer ? &ui.dataLayer() : &layer);
    CORRADE_COMPARE(&storage2.layer(), data.implicitLayer ? &ui.dataLayer() : &layer);
    /* The storage fits in-place always */
    CORRADE_VERIFY(!storage1.isAllocated());
    CORRADE_VERIFY(!storage2.isAllocated());
    CORRADE_VERIFY(!storage1.isDirty());
    CORRADE_VERIFY(!storage2.isDirty());
    CORRADE_COMPARE(storage1.flags(), StorageFlags{0x18});
    CORRADE_COMPARE(storage2.flags(), StorageFlags{0x18});
    CORRADE_COMPARE(storage1.size(), data.size);
    CORRADE_COMPARE(storage2.size(), data.size);
    CORRADE_VERIFY(storage1.isMutable());
    CORRADE_VERIFY(!storage2.isMutable());
    /* The bit offset or mutable flags shouldn't alias the default value */
    CORRADE_COMPARE(storage1.defaultValue(), false);
    CORRADE_COMPARE(storage2.defaultValue(), false);

    /* Check just that the const variants don't have an updater set */
    StorageQuery<bool> query1 = storage1[0];
    StorageQuery<bool> query2 = storage2[0];
    CORRADE_VERIFY(query1.isMutable());
    CORRADE_VERIFY(!query2.isMutable());
    CORRADE_COMPARE(query1.operations(), StorageOperation::Set|StorageOperation::Reset|StorageOperation::Toggle);
    CORRADE_COMPARE(query2.operations(), StorageOperations{});

    /* The data should be the same views as passed to the constructor */
    Containers::StridedBitArrayView1D viewFirst1 = storage1.data();
    Containers::StridedBitArrayView1D viewFirst2 = storage2.data();
    CORRADE_COMPARE(viewFirst1.data(), storageView.data());
    CORRADE_COMPARE(viewFirst2.data(), constStorageView.data());
    CORRADE_COMPARE(viewFirst1.offset(), storageView.offset());
    CORRADE_COMPARE(viewFirst2.offset(), constStorageView.offset());
    CORRADE_COMPARE(viewFirst1.size(), storageView.size());
    CORRADE_COMPARE(viewFirst2.size(), constStorageView.size());
    CORRADE_COMPARE(viewFirst1.stride(), storageView.stride());
    CORRADE_COMPARE(viewFirst2.stride(), constStorageView.stride());
    /* Mutable data should be the same. Assertion for the other tested in
       nonOwnedMutableDataInvalid(). */
    CORRADE_COMPARE(storage1.mutableData().data(), viewFirst1.data());
    CORRADE_COMPARE(storage1.mutableData().offset(), viewFirst1.offset());
    CORRADE_COMPARE(storage1.mutableData().size(), viewFirst1.size());
    CORRADE_COMPARE(storage1.mutableData().stride(), viewFirst1.stride());
}

void BitStorageTest::constructNonOwned() {
    auto&& data = ConstructNonOwnedData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Variant of constructNonOwned1D() testing the single-item case. The
       tested internals are the same in both cases but it's more readable this
       way. */

    /* Either this instance or the implicit one below gets used */
    DataLayer layer{layerHandle(0, 1)};

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    ui.setDataLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));

    bool storageData;
    const bool& constStorageData = storageData;

    BitStorage storage1 = data.implicitLayer ?
        BitStorage{ui, NonOwned, storageData, StorageFlags{0x18}} :
        BitStorage{layer, NonOwned, storageData, StorageFlags{0x18}};
    BitStorage storage2 = data.implicitLayer ?
        BitStorage{ui, NonOwned, constStorageData, StorageFlags{0x18}} :
        BitStorage{layer, NonOwned, constStorageData, StorageFlags{0x18}};
    CORRADE_COMPARE(&storage1.layer(), data.implicitLayer ? &ui.dataLayer() : &layer);
    CORRADE_COMPARE(&storage2.layer(), data.implicitLayer ? &ui.dataLayer() : &layer);
    /* The storage fits in-place always */
    CORRADE_VERIFY(!storage1.isAllocated());
    CORRADE_VERIFY(!storage2.isAllocated());
    CORRADE_VERIFY(!storage1.isDirty());
    CORRADE_VERIFY(!storage2.isDirty());
    CORRADE_COMPARE(storage1.flags(), StorageFlags{0x18});
    CORRADE_COMPARE(storage2.flags(), StorageFlags{0x18});
    CORRADE_COMPARE(storage1.size(), 1);
    CORRADE_COMPARE(storage2.size(), 1);
    CORRADE_COMPARE(storage1.defaultValue(), false);
    CORRADE_COMPARE(storage2.defaultValue(), false);

    /* Check just that the const variants don't have an updater set */
    StorageQuery<bool> query1 = storage1;
    StorageQuery<bool> query2 = storage2;
    CORRADE_VERIFY(query1.isMutable());
    CORRADE_VERIFY(!query2.isMutable());
    CORRADE_COMPARE(query1.operations(), StorageOperation::Set|StorageOperation::Reset|StorageOperation::Toggle);
    CORRADE_COMPARE(query2.operations(), StorageOperations{});

    /* The data should be the same views as passed to the constructor */
    Containers::StridedBitArrayView1D viewFirst1 = storage1.data();
    Containers::StridedBitArrayView1D viewFirst2 = storage2.data();
    CORRADE_COMPARE(viewFirst1.data(), &storageData);
    CORRADE_COMPARE(viewFirst2.data(), &constStorageData);
    CORRADE_COMPARE(viewFirst1.offset(), 0);
    CORRADE_COMPARE(viewFirst2.offset(), 0);
    CORRADE_COMPARE(viewFirst1.size(), 1);
    CORRADE_COMPARE(viewFirst2.size(), 1);
    CORRADE_COMPARE(viewFirst1.stride(), 1);
    CORRADE_COMPARE(viewFirst2.stride(), 1);
    /* Mutable data should be the same. Assertion for the other tested in
       nonOwnedMutableDataInvalid(). */
    CORRADE_COMPARE(storage1.mutableData().data(), viewFirst1.data());
    CORRADE_COMPARE(storage1.mutableData().offset(), viewFirst1.offset());
    CORRADE_COMPARE(storage1.mutableData().size(), viewFirst1.size());
    CORRADE_COMPARE(storage1.mutableData().stride(), viewFirst1.stride());
}

void BitStorageTest::constructCopy() {
    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    ui.setDataLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));

    /* The copy constructor is implicitly generated, so just verify that
       something is done at all, and that the templated UI constructor doesn't
       break it */
    BitStorage a1{ui.dataLayer()};
    BitStorage a2{ui};
    /* This works even without restricting the templated UI constructor */
    BitStorage b1 = a1;
    /* This only if the UI constructor isn't all-catching */
    BitStorage b2{a2};
    CORRADE_COMPARE(&b1.layer(), &ui.dataLayer());
    CORRADE_COMPARE(&b2.layer(), &ui.dataLayer());
    CORRADE_COMPARE(b1.handle(), a1.handle());
    CORRADE_COMPARE(b2.handle(), a2.handle());

    #ifndef CORRADE_NO_STD_IS_TRIVIALLY_TRAITS
    /* This is verified by StorageQuery constructors and other APIs already,
       but doesn't hurt to have it here as well */
    CORRADE_VERIFY(std::is_trivially_copy_constructible<BitStorage>::value);
    CORRADE_VERIFY(std::is_trivially_copy_assignable<BitStorage>::value);
    #endif
}

void BitStorageTest::constructHandleRecycle() {
    DataLayer layer{layerHandle(0, 1)};

    /* Create one more storage to verify the recycling isn't handling just the
       first item correctly */
    BitStorage{layer};

    /* Create a storage and update its properties, non-owned with an offset so
       also the internal bits are set. It should be stored in-place so we can
       verify it gets properly reinitialized next time. */
    const UnsignedInt data[1]{};
    BitStorage first{layer, NonOwned, Containers::BitArrayView{data, 7, 2}};
    first
        .setDefaultValue(true);
    CORRADE_VERIFY(!first.isAllocated());
    CORRADE_COMPARE(first.defaultValue(), true);
    /* The storage should be immutable also, with full offset bits stored */
    CORRADE_VERIFY(!first.isMutable());
    CORRADE_COMPARE(first.data().offset(), 7);

    /* Remove and create a new storage in the same slot. All properties should
       be reset back to defaults. */
    layer.removeStorage(first);
    BitStorage second{layer, 8};
    CORRADE_COMPARE(storageHandleId(second.handle()), storageHandleId(first.handle()));
    CORRADE_VERIFY(!second.isAllocated());
    CORRADE_COMPARE(second.defaultValue(), false);
    /* The internal immutable flag should also be reset, and the offset bits
       should be cleared */
    CORRADE_VERIFY(second.isMutable());
    CORRADE_COMPARE_AS(second.data(), Containers::stridedArrayView({
        0, 0, 0, 0, 0, 0, 0, 0
    }).sliceBit(0), TestSuite::Compare::Container);
}

void BitStorageTest::access1D() {
    AbstractUserInterface ui{{100, 100}};
    DataLayer& layer = ui.setLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));

    BitStorage storage{layer, NoInit, 9*9};
    CORRADE_VERIFY(!storage.isDirty());

    /* Set all ones and zero every 17 bits */
    for(std::size_t i = 0; i != storage.size(); ++i) {
        storage[i].set(i % 9 == 0 ? false : true);
        /* Ahem. The first bit is zero because the NoInit constructor clears
           the first byte to fill it with flags, so setting the first ever bit
           to false *may not* cause a dirty flag to be set. Thoroughly checked
           in update() below. */
        if(i != 0)
            CORRADE_VERIFY(storage.isDirty());
    }

    /* Check that the query returns the same */
    for(std::size_t i = 0; i != storage.size(); ++i) {
        CORRADE_ITERATION(i);
        CORRADE_COMPARE(storage[i], i % 9 == 0 ? false : true);
    }

    /* A zero every 9 bits, which means each next 8-bit number is rotated by a
       bit */
    UnsignedByte bits[]{
        UnsignedByte(~1), UnsignedByte(~2), UnsignedByte(~4),
        UnsignedByte(~8), UnsignedByte(~16), UnsignedByte(~32),
        UnsignedByte(~64), UnsignedByte(~128),
        /* All bits set in this one, as the previous bit is the last in the
           byte, and the next is the first in the byte */
        UnsignedByte(~0), UnsignedByte(~1), UnsignedByte(~4)
    };
    CORRADE_COMPARE_AS(storage.data(),
        (Containers::BitArrayView{bits, 0, 9*9}),
        TestSuite::Compare::Container);

    StorageQuery<bool> query1 = storage[9];
    StorageQuery<bool> query2 = storage[10];
    CORRADE_COMPARE(&query1.layer(), &layer);
    CORRADE_COMPARE(&query2.layer(), &layer);
    CORRADE_COMPARE(query1.storage(), storage.handle());
    CORRADE_COMPARE(query2.storage(), storage.handle());
    CORRADE_COMPARE(query1.index(), (Containers::Size3D{0, 0, 9}));
    CORRADE_COMPARE(query2.index(), (Containers::Size3D{0, 0, 10}));
    CORRADE_VERIFY(query1.isMutable());
    CORRADE_VERIFY(query2.isMutable());
    CORRADE_COMPARE(query1.operations(), StorageOperation::Set|StorageOperation::Reset|StorageOperation::Toggle);
    CORRADE_COMPARE(query2.operations(), StorageOperation::Set|StorageOperation::Reset|StorageOperation::Toggle);
    CORRADE_COMPARE(query1, false);
    CORRADE_COMPARE(query2, true);

    Int called1 = 0, called2 = 0;
    query1.onUpdate([&called1](bool value) {
        CORRADE_COMPARE(value, false);
        ++called1;
    });
    query2.onUpdate([&called2](bool value) {
        CORRADE_COMPARE(value, true);
        ++called2;
    });
    CORRADE_COMPARE(called1, 0);
    CORRADE_COMPARE(called2, 0);

    ui.update();
    CORRADE_VERIFY(!storage.isDirty());
    CORRADE_COMPARE(called1, 1);
    CORRADE_COMPARE(called2, 1);
}

void BitStorageTest::access() {
    /* A single-item subset of access1D(), basically */

    AbstractUserInterface ui{{100, 100}};
    DataLayer& layer = ui.setLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));

    /* Explicitly zero-initializing to not run into the same
       implementation-defined isDirty() case as in access1D() above */
    BitStorage storage{layer, ValueInit};
    CORRADE_VERIFY(!storage.isDirty());

    storage.value().set(true);
    CORRADE_VERIFY(storage.isDirty());
    CORRADE_COMPARE(storage.value(), true);

    bool bits[]{true};
    CORRADE_COMPARE_AS(storage.data(),
        (Containers::BitArrayView{bits, 0, 1}),
        TestSuite::Compare::Container);

    /* Verifying both the explicit and implicit query */
    StorageQuery<bool> query1 = storage.value();
    StorageQuery<bool> query2 = storage;
    CORRADE_COMPARE(&query1.layer(), &layer);
    CORRADE_COMPARE(&query2.layer(), &layer);
    CORRADE_COMPARE(query1.storage(), storage.handle());
    CORRADE_COMPARE(query2.storage(), storage.handle());
    CORRADE_COMPARE(query1.index(), (Containers::Size3D{0, 0, 0}));
    CORRADE_COMPARE(query2.index(), (Containers::Size3D{0, 0, 0}));
    CORRADE_VERIFY(query1.isMutable());
    CORRADE_VERIFY(query2.isMutable());
    CORRADE_COMPARE(query1.operations(), StorageOperation::Set|StorageOperation::Reset|StorageOperation::Toggle);
    CORRADE_COMPARE(query2.operations(), StorageOperation::Set|StorageOperation::Reset|StorageOperation::Toggle);
    CORRADE_COMPARE(query1, true);
    CORRADE_COMPARE(query2, true);

    /* Verifying onUpdate() with both the explicit and implicit query */
    Int called1 = 0, called2 = 0;
    query1.onUpdate([&called1](bool value) {
        CORRADE_COMPARE(value, true);
        ++called1;
    });
    storage->onUpdate([&called2](bool value) {
        CORRADE_COMPARE(value, true);
        ++called2;
    });
    CORRADE_COMPARE(called1, 0);
    CORRADE_COMPARE(called2, 0);

    ui.update();
    CORRADE_VERIFY(!storage.isDirty());
    CORRADE_COMPARE(called1, 1);
    CORRADE_COMPARE(called2, 1);
}

void BitStorageTest::accessNonOwned1D() {
    auto&& data = AccessNonOwned1DData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    AbstractUserInterface ui{{100, 100}};
    DataLayer& layer = ui.setLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));

    /* A zero every 9 bits, which means each next 8-bit number is rotated by a
       bit */
    UnsignedByte storageData[]{
        UnsignedByte(~1), UnsignedByte(~2), UnsignedByte(~4),
        UnsignedByte(~8), UnsignedByte(~16), UnsignedByte(~32),
        UnsignedByte(~64), UnsignedByte(~128),
        /* All bits set in this one, as the previous bit is the last in the
           byte, and the next is the first in the byte */
        UnsignedByte(~0), UnsignedByte(~1)
    };
    Containers::MutableStridedBitArrayView1D view{storageData, storageData + data.offset/8, data.offset % 8, 16, data.stride};
    CORRADE_COMPARE(view[14], false);
    /* With zero stride, both bits are obviously the same */
    CORRADE_COMPARE(view[15], data.stride ? true : false);

    BitStorage storage = data.immutable ?
        BitStorage{layer, NonOwned, Containers::StridedBitArrayView1D{view}} :
        BitStorage{layer, NonOwned, view};
    CORRADE_VERIFY(!storage.isDirty());

    StorageQuery<bool> query1 = storage[14];
    StorageQuery<bool> query2 = storage[15];
    CORRADE_COMPARE(&query1.layer(), &layer);
    CORRADE_COMPARE(&query2.layer(), &layer);
    CORRADE_COMPARE(query1.storage(), storage.handle());
    CORRADE_COMPARE(query2.storage(), storage.handle());
    CORRADE_COMPARE(query1.index(), (Containers::Size3D{0, 0, 14}));
    CORRADE_COMPARE(query2.index(), (Containers::Size3D{0, 0, 15}));
    CORRADE_COMPARE(query1.isMutable(), !data.immutable);
    CORRADE_COMPARE(query2.isMutable(), !data.immutable);
    CORRADE_COMPARE(query1.operations(), data.immutable ?
        StorageOperations{} :
        StorageOperation::Set|StorageOperation::Reset|StorageOperation::Toggle);
    CORRADE_COMPARE(query2.operations(), data.immutable ?
        StorageOperations{} :
        StorageOperation::Set|StorageOperation::Reset|StorageOperation::Toggle);
    CORRADE_COMPARE(query1, false);
    /* With zero stride, both bits are obviously the same */
    CORRADE_COMPARE(query2, data.stride ? true : false);

    Int called1 = 0, called2 = 0;
    query1.onUpdate([&called1](bool value) {
        CORRADE_COMPARE(value, false);
        ++called1;
    });
    query2.onUpdate([&called2, &data](bool value) {
        /* With zero stride, both bits are obviously the same */
        CORRADE_COMPARE(value, data.stride ? true : false);
        ++called2;
    });
    CORRADE_COMPARE(called1, 0);
    CORRADE_COMPARE(called2, 0);

    ui.update();
    CORRADE_VERIFY(!storage.isDirty());
    CORRADE_COMPARE(called1, 1);
    CORRADE_COMPARE(called2, 1);

    if(!data.immutable) {
        query1.set(true);
        CORRADE_VERIFY(storage.isDirty());
        CORRADE_COMPARE(query1, true);
        /* There's always at most one bit set in each 8-bit element, so this
           always results in all bits set */
        CORRADE_COMPARE(storageData[data.valueIndex], UnsignedByte(~0));
    }
}

void BitStorageTest::accessNonOwned() {
    auto&& data = AccessNonOwnedData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* A single-item subset of accessNonOwned1D(), basically */

    AbstractUserInterface ui{{100, 100}};
    DataLayer& layer = ui.setLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));

    /* Verifying both the explicit and implicit query */
    bool storageData = true;
    BitStorage storage = data.immutable ?
        BitStorage{layer, NonOwned, static_cast<const bool&>(storageData)} :
        BitStorage{layer, NonOwned, storageData};
    StorageQuery<bool> query1 = storage.value();
    StorageQuery<bool> query2 = storage;
    CORRADE_COMPARE(&query1.layer(), &layer);
    CORRADE_COMPARE(&query2.layer(), &layer);
    CORRADE_COMPARE(query1.storage(), storage.handle());
    CORRADE_COMPARE(query2.storage(), storage.handle());
    CORRADE_COMPARE(query1.index(), (Containers::Size3D{0, 0, 0}));
    CORRADE_COMPARE(query2.index(), (Containers::Size3D{0, 0, 0}));
    CORRADE_COMPARE(query1.isMutable(), !data.immutable);
    CORRADE_COMPARE(query2.isMutable(), !data.immutable);
    CORRADE_COMPARE(query1.operations(), data.immutable ?
        StorageOperations{} :
        StorageOperation::Set|StorageOperation::Reset|StorageOperation::Toggle);
    CORRADE_COMPARE(query2.operations(), data.immutable ?
        StorageOperations{} :
        StorageOperation::Set|StorageOperation::Reset|StorageOperation::Toggle);
    CORRADE_COMPARE(query1, true);
    CORRADE_COMPARE(query2, true);

    /* Verifying onUpdate() with both the explicit and implicit query */
    Int called1 = 0, called2 = 0;
    query1.onUpdate([&called1](bool value) {
        CORRADE_COMPARE(value, true);
        ++called1;
    });
    storage->onUpdate([&called2](bool value) {
        CORRADE_COMPARE(value, true);
        ++called2;
    });
    CORRADE_COMPARE(called1, 0);
    CORRADE_COMPARE(called2, 0);

    ui.update();
    CORRADE_VERIFY(!storage.isDirty());
    CORRADE_COMPARE(called1, 1);
    CORRADE_COMPARE(called2, 1);

    if(!data.immutable) {
        query1.set(false);
        CORRADE_VERIFY(storage.isDirty());
        CORRADE_COMPARE(query1, false);
        CORRADE_COMPARE(storageData, false);

        query2.set(true);
        CORRADE_VERIFY(storage.isDirty());
        CORRADE_COMPARE(query2, true);
        CORRADE_COMPARE(storageData, true);
    }
}

void BitStorageTest::accessInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    /* All these assertions are fired by StorageQuery, this is just verifying
       that they actually get fired without something else happening first or
       them being sidestepped for example by delegating all APIs to the 3D
       queries. */

    DataLayer layer{layerHandle(0, 1)};
    BitStorage storage1D{layer, 15};

    Containers::String out;
    Error redirectError{&out};
    /* Index out of bounds. The assertion should be fired by StorageQuery, so
       just verify that we're not attempting to access something else first. */
    storage1D[15];
    /* Single-item API for a 1D storage even though it's in bounds */
    storage1D.value();
    StorageQuery<bool>{storage1D};
    CORRADE_COMPARE_AS(out,
        "Ui::StorageQuery: index {0, 0, 15} out of range for {1, 1, 15} elements\n"

        "Ui::StorageQuery: expected a single-item storage but got a size of {1, 1, 15}\n"
        "Ui::StorageQuery: expected a single-item storage but got a size of {1, 1, 15}\n",
        TestSuite::Compare::String);
}

void BitStorageTest::nonOwnedMutableDataInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    DataLayer layer{layerHandle(0, 1)};

    const bool storageData{};
    BitStorage storage{layer, NonOwned, storageData};
    CORRADE_VERIFY(!storage.isMutable());

    Containers::String out;
    Error redirectError{&out};
    storage.mutableData();
    CORRADE_COMPARE(out, "Ui::BitStorage::mutableData(): data not mutable\n");
}

void BitStorageTest::update() {
    auto&& data = UpdateData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* This verifies that all update operations properly touch the stored or
       referenced value along with setting dirty bits. Behavior with custom
       default etc. and cases when dirty bit is *not* set is tested in defaultValue()
       below. */

    AbstractUserInterface ui{{100, 100}};
    DataLayer& layer = ui.setLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));

    bool storageData = true;
    BitStorage storage = data.nonOwned ?
        BitStorage{layer, NonOwned, storageData} :
        BitStorage{layer, DirectInit, true};
    StorageQuery<bool> query = storage.value();

    storage.setDefaultValue(true);

    struct {
        Int called = 0;
        bool expected;
    } state;
    DataHandle update = query.onUpdate([&state](bool value) {
        CORRADE_COMPARE(value, state.expected);
        ++state.called;
    });
    CORRADE_VERIFY(!layer.isStorageDirty(storage));
    CORRADE_VERIFY(layer.isDirty(update));

    /* The callback gets called on the first update */
    state.expected = true;
    {
        CORRADE_ITERATION(__FILE__ ":" CORRADE_LINE_STRING);
        ui.update();
    }
    CORRADE_VERIFY(!layer.isStorageDirty(storage));
    CORRADE_VERIFY(!layer.isDirty(update));
    CORRADE_COMPARE(state.called, 1);

    /* Changing the value marks the storage as dirty */
    query.set(false);
    CORRADE_VERIFY(layer.isStorageDirty(storage));
    CORRADE_VERIFY(!layer.isDirty(update));

    /* Update calls and resets */
    state.expected = false;
    {
        CORRADE_ITERATION(__FILE__ ":" CORRADE_LINE_STRING);
        ui.update();
    }
    CORRADE_VERIFY(!layer.isStorageDirty(storage));
    CORRADE_VERIFY(!layer.isDirty(update));
    CORRADE_COMPARE(state.called, 2);

    /* Reset marks the storage as dirty */
    query.reset();
    CORRADE_VERIFY(layer.isStorageDirty(storage));
    CORRADE_VERIFY(!layer.isDirty(update));

    /* Update calls and resets */
    state.expected = true;
    {
        CORRADE_ITERATION(__FILE__ ":" CORRADE_LINE_STRING);
        ui.update();
    }
    CORRADE_VERIFY(!layer.isStorageDirty(storage));
    CORRADE_VERIFY(!layer.isDirty(update));
    CORRADE_COMPARE(state.called, 3);

    /* Toggle marks the storage as dirty */
    query.toggle();
    CORRADE_VERIFY(layer.isStorageDirty(storage));
    CORRADE_VERIFY(!layer.isDirty(update));

    /* Update calls and resets */
    state.expected = false;
    {
        CORRADE_ITERATION(__FILE__ ":" CORRADE_LINE_STRING);
        ui.update();
    }
    CORRADE_VERIFY(!layer.isStorageDirty(storage));
    CORRADE_VERIFY(!layer.isDirty(update));
    CORRADE_COMPARE(state.called, 4);
}

void BitStorageTest::updateDefaultValue() {
    auto&& data = UpdateDefaultValueData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Compared to update(), which verified that the right value was updated
       depending on whether the storage is owned or not, this verifies that all
       operations correctly affect given memory location with various custom
       behavior and set (or not set) the dirty bit, assuming the implementation
       isn't differing in the non-owned variant. */

    AbstractUserInterface ui{{100, 100}};
    DataLayer& layer = ui.setLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));

    BitStorage storage{layer, DirectInit, 17, data.value};
    StorageQuery<bool> query = storage[14];

    struct {
        Int called = 0;
        bool expected;
    } state;
    DataHandle update = query.onUpdate([&state](bool value) {
        CORRADE_COMPARE(value, state.expected);
        ++state.called;
    });

    /* The callback gets called on the first update */
    state.expected = data.value;
    {
        CORRADE_ITERATION(__FILE__ ":" CORRADE_LINE_STRING);
        ui.update();
    }
    CORRADE_VERIFY(!layer.isStorageDirty(storage));
    CORRADE_VERIFY(!layer.isDirty(update));
    CORRADE_COMPARE(state.called, 1);

    /* Set a custom default value if desired. It should *not* mark the storage
       as dirty, and also shouldn't change the values in any way. */
    if(data.defaultValue) {
        storage.setDefaultValue(*data.defaultValue);
        CORRADE_COMPARE(storage.defaultValue(), *data.defaultValue);
        /* It should also not stomp on the bits in any way */
        CORRADE_COMPARE_AS(storage.data(),
            Containers::stridedArrayView({data.value}).sliceBit(0).broadcasted<0>(17),
            TestSuite::Compare::Container);
        CORRADE_COMPARE(query, data.value);
        CORRADE_VERIFY(!layer.isStorageDirty(storage));
        CORRADE_VERIFY(!layer.isDirty(update));
    }

    /* Perform the desired update operation. This should mark the storage as
       dirty only if the value actually changes. */
    if(data.operation == StorageOperation::Set) {
        CORRADE_COMPARE(query.set(*data.set), StorageUpdateState::Success);
    } else {
        CORRADE_INTERNAL_ASSERT(!data.set);
        if(data.operation == StorageOperation::Reset)
            CORRADE_COMPARE(query.reset(), StorageUpdateState::Success);
        else if(data.operation == StorageOperation::Toggle)
            CORRADE_COMPARE(query.toggle(), StorageUpdateState::Success);
        else CORRADE_INTERNAL_ASSERT_UNREACHABLE();
    }
    CORRADE_COMPARE(query, data.expected);
    CORRADE_COMPARE(layer.isStorageDirty(storage), data.expectedDirty);
    CORRADE_VERIFY(!layer.isDirty(update));

    /* The update function should get called only if something actually
       changed */
    state.called = 0;
    state.expected = data.expected;
    {
        CORRADE_ITERATION(__FILE__ ":" CORRADE_LINE_STRING);
        ui.update();
    }
    CORRADE_VERIFY(!layer.isStorageDirty(storage));
    CORRADE_VERIFY(!layer.isDirty(update));
    CORRADE_COMPARE(state.called, data.expectedDirty ? 1 : 0);
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::BitStorageTest)
