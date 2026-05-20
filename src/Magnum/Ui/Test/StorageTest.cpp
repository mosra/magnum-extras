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

#include <Corrade/Containers/Function.h>
#include <Corrade/Containers/String.h>
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/String.h>
#include <Magnum/Math/Vector2.h> /* AbstractUserInterface constructor size */

#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/Storage.h"
#include "Magnum/Ui/UserInterface.h"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct StorageTest: TestSuite::Tester {
    explicit StorageTest();

    void construct();
    void constructCopy();

    void access3D();
    void access2D();
    void access1D();
    void access();

    void accessCast3D();
    void accessCast2D();
    void accessCast1D();
    void accessCast();

    void accessInvalid();
};

const struct {
    const char* name;
    bool implicitLayer;
} ConstructData[]{
    {"", false},
    {"implicit layer", true}
};

const struct {
    const char* name;
    std::size_t valueIndex;
    std::size_t offset;
    Containers::Stride3D stride;
} Access3DData[]{
    {"", 11, 0, {24, 8, 4}},
    {"sparse", 29, 1, {64, 20, 8}},
    {"negative stride 0", 9, 10, {-32, 12, 4}},
    {"negative stride 1", 13, 10, {32, -12, 4}},
    {"negative stride 2", 25, 1, {64, 20, -8}},
    {"zero stride 0", 16, 5, {0, 20, 4}},
    {"zero stride 1", 14, 5, {32, 0, 4}},
    {"zero stride 2", 23, 5, {32, 20, 0}},
};

const struct {
    const char* name;
    std::size_t valueIndex;
    std::size_t offset;
    Containers::Stride2D stride;
} Access2DData[]{
    {"", 5, 0, {6, 2}},
    {"sparse", 10, 1, {10, 4}},
    {"negative stride 0", 1, 3, {-12, 4}},
    {"negative stride 1", 1, 2, {6, -4}},
    {"zero stride 0", 7, 5, {0, 2}},
    {"zero stride 1", 10, 5, {10, 0}},
};

const struct {
    const char* name;
    std::size_t valueIndex;
    std::size_t offset;
    std::ptrdiff_t stride;
} Access1DData[]{
    {"", 4, 0, 1},
    {"sparse", 13, 1, 3},
    {"negative stride", 1, 9, -2},
    {"zero stride", 5, 5, 0},
};

StorageTest::StorageTest() {
    addInstancedTests({&StorageTest::construct},
        Containers::arraySize(ConstructData));

    addTests({&StorageTest::constructCopy});

    addInstancedTests({&StorageTest::access3D},
        Containers::arraySize(Access3DData));

    addInstancedTests({&StorageTest::access2D},
        Containers::arraySize(Access2DData));

    addInstancedTests({&StorageTest::access1D},
        Containers::arraySize(Access1DData));

    addTests({&StorageTest::access});

    addInstancedTests({&StorageTest::accessCast3D},
        Containers::arraySize(Access3DData));

    addInstancedTests({&StorageTest::accessCast2D},
        Containers::arraySize(Access2DData));

    addInstancedTests({&StorageTest::accessCast1D},
        Containers::arraySize(Access1DData));

    addTests({&StorageTest::accessCast,

              &StorageTest::accessInvalid});
}

void StorageTest::construct() {
    auto&& data = ConstructData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Either this instance or the implicit one below gets used */
    DataLayer layer{layerHandle(0, 1)};

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    ui.setDataLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));

    Float storageData[30];

    /* Transposed so we don't have a trivial stride */
    Containers::StridedArrayView3D<const Float> view3D =
        Containers::stridedArrayView(storageData)
            .template expanded<0, 3>({2, 3, 5})
            .template transposed<1, 2>();
    Storage<Float> first = data.implicitLayer ?
        Storage<Float>{ui, view3D, StorageFlags{0x10}} :
        Storage<Float>{layer, view3D, StorageFlags{0x10}};
    CORRADE_COMPARE(&first.layer(), data.implicitLayer ? &ui.dataLayer() : &layer);
    CORRADE_VERIFY(!first.isAllocated());
    CORRADE_VERIFY(!first.isDirty());
    CORRADE_COMPARE(first.flags(), StorageFlags{0x10});
    CORRADE_COMPARE(first.size(),  (Containers::Size3D{2, 5, 3}));
    /* The view is transposed so the stride should be too */
    CORRADE_COMPARE(first.stride(),(Containers::Stride3D{3*5*4, 4, 5*4}));

    /* The operator[]() etc is tested in access3D() below, no need to repeat
       that here */

    /* The data should be the same view as passed to the constructor */
    CORRADE_COMPARE(first.data().data(), view3D.data());
    CORRADE_COMPARE(first.data().size(), view3D.size());
    CORRADE_COMPARE(first.data().stride(), view3D.stride());

    /* 2D, 1D and single-item variants delegate to the 3D constructor, so
       verify just that they propagate all arguments correctly */
    Containers::StridedArrayView2D<const Float> view2D =
        Containers::stridedArrayView(storageData)
            .template expanded<0, 2>({6, 5})
            .template transposed<0, 1>();
    Storage<Float> second = data.implicitLayer ?
        Storage<Float>{ui, view2D, StorageFlags{0x20}} :
        Storage<Float>{layer, view2D, StorageFlags{0x20}};
    CORRADE_COMPARE(&second.layer(), data.implicitLayer ? &ui.dataLayer() : &layer);
    CORRADE_VERIFY(!second.isAllocated());
    CORRADE_VERIFY(!second.isDirty());
    CORRADE_COMPARE(second.flags(), StorageFlags{0x20});
    CORRADE_COMPARE(second.size(),  (Containers::Size3D{1, 5, 6}));
    /* The view is transposed so the stride should be too */
    CORRADE_COMPARE(second.stride(),(Containers::Stride3D{5*4, 4, 5*4}));
    /* The data should be the same views as passed to the constructor, expanded
       to 3D */
    CORRADE_COMPARE(second.data().data(), view2D.data());
    CORRADE_COMPARE(second.data().size(), Containers::StridedArrayView3D<const Float>{view2D}.size());
    CORRADE_COMPARE(second.data().stride(), Containers::StridedArrayView3D<const Float>{view2D}.stride());

    Containers::StridedArrayView1D<const Float> view1D =
        Containers::stridedArrayView(storageData)
            .template flipped<0>();
    Storage<Float> third = data.implicitLayer ?
        Storage<Float>{ui, view1D, StorageFlags{0x18}} :
        Storage<Float>{layer, view1D, StorageFlags{0x18}};
    CORRADE_COMPARE(&third.layer(), data.implicitLayer ? &ui.dataLayer() : &layer);
    CORRADE_VERIFY(!third.isAllocated());
    CORRADE_VERIFY(!third.isDirty());
    CORRADE_COMPARE(third.flags(), StorageFlags{0x18});
    CORRADE_COMPARE(third.size(),  (Containers::Size3D{1, 1, 30}));
    /* The view is flipped so the first and second dimension becomes flipped
       too, times the original view size */
    CORRADE_COMPARE(third.stride(),(Containers::Stride3D{-30*4, -30*4, -4}));
    /* The data should be the same views as passed to the constructor, expanded
       to 3D */
    CORRADE_COMPARE(third.data().data(), view1D.data());
    CORRADE_COMPARE(third.data().size(), Containers::StridedArrayView3D<const Float>{view1D}.size());
    CORRADE_COMPARE(third.data().stride(), Containers::StridedArrayView3D<const Float>{view1D}.stride());

    Storage<Float> fourth = data.implicitLayer ?
        Storage<Float>{ui, storageData[3], StorageFlags{0x18}} :
        Storage<Float>{layer, storageData[3], StorageFlags{0x18}};
    CORRADE_COMPARE(&fourth.layer(), data.implicitLayer ? &ui.dataLayer() : &layer);
    CORRADE_VERIFY(!fourth.isAllocated());
    CORRADE_VERIFY(!fourth.isDirty());
    CORRADE_COMPARE(fourth.flags(), StorageFlags{0x18});
    CORRADE_COMPARE(fourth.size(),  (Containers::Size3D{1, 1, 1}));
    CORRADE_COMPARE(fourth.stride(),(Containers::Stride3D{4, 4, 4}));
    /* The data should point to the value passed to the constructor, with
       trivial size and stride */
    CORRADE_COMPARE(fourth.data().data(), storageData + 3);
    CORRADE_COMPARE(fourth.data().size(), (Containers::Size3D{1, 1, 1}));
    CORRADE_COMPARE(fourth.data().stride(), (Containers::Stride3D{4, 4, 4}));
}

void StorageTest::constructCopy() {
    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    ui.setDataLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));

    /* The copy constructor is implicitly generated, so just verify that
       something is done at all, and that the templated UI constructor doesn't
       break it. */
    Int data;
    Storage<Int> a1{ui.dataLayer(), data};
    Storage<Int> a2{ui, data};
    /* In this case the templated UI constructor takes two arguments always so
       both of these pick the implicitly-generated copy */
    Storage<Int> b1 = a1;
    Storage<Int> b2{a2};
    CORRADE_COMPARE(&b1.layer(), &ui.dataLayer());
    CORRADE_COMPARE(&b2.layer(), &ui.dataLayer());
    CORRADE_COMPARE(b1.handle(), a1.handle());
    CORRADE_COMPARE(b2.handle(), a2.handle());

    #ifndef CORRADE_NO_STD_IS_TRIVIALLY_TRAITS
    /* This is verified by StorageQuery constructors and other APIs already,
       but doesn't hurt to have it here as well */
    CORRADE_VERIFY(std::is_trivially_copy_constructible<Storage<Int>>::value);
    CORRADE_VERIFY(std::is_trivially_copy_assignable<Storage<Int>>::value);
    #endif
}

void StorageTest::access3D() {
    auto&& data = Access3DData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    AbstractUserInterface ui{{100, 100}};
    DataLayer& layer = ui.setLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));

    Int storageData[]{
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
        16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31
    };
    storageData[data.valueIndex] = -17654321;
    Containers::StridedArrayView3D<Int> view{storageData, storageData + data.offset, {2, 3, 2}, data.stride};
    CORRADE_COMPARE((view[{1, 2, 1}]), -17654321);

    Storage<Int> storage{layer, view};
    StorageQuery<Int> query = storage[{1, 2, 1}];
    CORRADE_COMPARE(&query.layer(), &layer);
    CORRADE_COMPARE(query.storage(), storage.handle());
    CORRADE_COMPARE(query.index(), (Containers::Size3D{1, 2, 1}));
    CORRADE_VERIFY(!query.isMutable());
    CORRADE_COMPARE(query.operations(), StorageOperations{});
    CORRADE_COMPARE(query, -17654321);

    Int called = 0;
    DataHandle update = query.onUpdate([&called](Int value) {
        CORRADE_COMPARE(value, -17654321);
        ++called;
    });
    CORRADE_VERIFY(!layer.isStorageDirty(storage));
    CORRADE_VERIFY(layer.isDirty(update));
    CORRADE_COMPARE(called, 0);

    /* The callback gets called on the first update */
    ui.update();
    CORRADE_VERIFY(!layer.isStorageDirty(storage));
    CORRADE_VERIFY(!layer.isDirty(update));
    CORRADE_COMPARE(called, 1);
}

void StorageTest::access2D() {
    auto&& data = Access2DData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* A 2D subset of access3D(), basically */

    AbstractUserInterface ui{{100, 100}};
    DataLayer& layer = ui.setLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));

    Short storageData[]{
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13
    };
    storageData[data.valueIndex] = -17654;
    Containers::StridedArrayView2D<Short> view{storageData, storageData + data.offset, {2, 3}, data.stride};
    CORRADE_COMPARE((view[{1, 2}]), -17654);

    Storage<Short> storage{layer, view};
    StorageQuery<Short> query = storage[{1, 2}];
    CORRADE_COMPARE(&query.layer(), &layer);
    CORRADE_COMPARE(query.storage(), storage.handle());
    CORRADE_COMPARE(query.index(), (Containers::Size3D{0, 1, 2}));
    CORRADE_VERIFY(!query.isMutable());
    CORRADE_COMPARE(query.operations(), StorageOperations{});
    CORRADE_COMPARE(query, -17654);

    Int called = 0;
    DataHandle update = query.onUpdate([&called](Short value) {
        CORRADE_COMPARE(value, -17654);
        ++called;
    });
    CORRADE_VERIFY(!layer.isStorageDirty(storage));
    CORRADE_VERIFY(layer.isDirty(update));
    CORRADE_COMPARE(called, 0);

    ui.update();
    CORRADE_VERIFY(!layer.isStorageDirty(storage));
    CORRADE_VERIFY(!layer.isDirty(update));
    CORRADE_COMPARE(called, 1);
}

void StorageTest::access1D() {
    auto&& data = Access1DData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* A 1D subset of access3D(), basically */

    AbstractUserInterface ui{{100, 100}};
    DataLayer& layer = ui.setLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));

    Byte storageData[]{
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14
    };
    storageData[data.valueIndex] = -114;
    Containers::StridedArrayView1D<Byte> view{storageData, storageData + data.offset, 5, data.stride};
    CORRADE_COMPARE(view[4], -114);

    Storage<Byte> storage{layer, view};
    StorageQuery<Byte> query = storage[4];
    CORRADE_COMPARE(&query.layer(), &layer);
    CORRADE_COMPARE(query.storage(), storage.handle());
    CORRADE_COMPARE(query.index(), (Containers::Size3D{0, 0, 4}));
    CORRADE_VERIFY(!query.isMutable());
    CORRADE_COMPARE(query.operations(), StorageOperations{});
    CORRADE_COMPARE(query, -114);

    Int called = 0;
    DataHandle update = query.onUpdate([&called](Byte value) {
        CORRADE_COMPARE(value, -114);
        ++called;
    });
    CORRADE_VERIFY(!layer.isStorageDirty(storage));
    CORRADE_VERIFY(layer.isDirty(update));
    CORRADE_COMPARE(called, 0);

    ui.update();
    CORRADE_VERIFY(!layer.isStorageDirty(storage));
    CORRADE_VERIFY(!layer.isDirty(update));
    CORRADE_COMPARE(called, 1);
}

void StorageTest::access() {
    /* A single-item subset of access3D(), basically */

    AbstractUserInterface ui{{100, 100}};
    DataLayer& layer = ui.setLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));

    /* Verifying both the explicit and implicit query */
    Long data = -9876543210ll;
    Storage<Long> storage{layer, data};
    StorageQuery<Long> query1 = storage.value();
    StorageQuery<Long> query2 = storage;
    CORRADE_COMPARE(&query1.layer(), &layer);
    CORRADE_COMPARE(&query2.layer(), &layer);
    CORRADE_COMPARE(query1.storage(), storage.handle());
    CORRADE_COMPARE(query2.storage(), storage.handle());
    CORRADE_COMPARE(query1.index(), (Containers::Size3D{0, 0, 0}));
    CORRADE_COMPARE(query2.index(), (Containers::Size3D{0, 0, 0}));
    CORRADE_VERIFY(!query1.isMutable());
    CORRADE_VERIFY(!query2.isMutable());
    CORRADE_COMPARE(query1.operations(), StorageOperations{});
    CORRADE_COMPARE(query2.operations(), StorageOperations{});
    CORRADE_COMPARE(query1, -9876543210ll);
    CORRADE_COMPARE(query2, -9876543210ll);

    /* Verifying onUpdate() with both the explicit and implicit query */
    Int called1 = 0, called2 = 0;
    DataHandle update1 = query1.onUpdate([&called1](Long value) {
        CORRADE_COMPARE(value, -9876543210);
        ++called1;
    });
    DataHandle update2 = storage->onUpdate([&called2](Long value) {
        CORRADE_COMPARE(value, -9876543210);
        ++called2;
    });
    CORRADE_VERIFY(!layer.isStorageDirty(storage));
    CORRADE_VERIFY(layer.isDirty(update1));
    CORRADE_VERIFY(layer.isDirty(update2));
    CORRADE_COMPARE(called1, 0);
    CORRADE_COMPARE(called2, 0);

    ui.update();
    CORRADE_VERIFY(!layer.isStorageDirty(storage));
    CORRADE_VERIFY(!layer.isDirty(update1));
    CORRADE_VERIFY(!layer.isDirty(update2));
    CORRADE_COMPARE(called1, 1);
    CORRADE_COMPARE(called2, 1);
}

void StorageTest::accessCast3D() {
    auto&& data = Access3DData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Like access3D(), but using the value<T>() variant */

    AbstractUserInterface ui{{100, 100}};
    DataLayer& layer = ui.setLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));

    Int storageData[]{
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
        16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31
    };
    storageData[data.valueIndex] = -17654321;
    Containers::StridedArrayView3D<Int> view{storageData, storageData + data.offset, {2, 3, 2}, data.stride};
    CORRADE_COMPARE(UnsignedShort(view[{1, 2, 1}]), 40399);

    Storage<Int> storage{layer, view};
    StorageQuery<UnsignedShort> query = storage.value<UnsignedShort>({1, 2, 1});
    CORRADE_COMPARE(&query.layer(), &layer);
    CORRADE_COMPARE(query.storage(), storage.handle());
    CORRADE_COMPARE(query.index(), (Containers::Size3D{1, 2, 1}));
    CORRADE_VERIFY(!query.isMutable());
    CORRADE_COMPARE(query.operations(), StorageOperations{});
    CORRADE_COMPARE(query, 40399);

    Int called = 0;
    DataHandle update = query.onUpdate([&called](UnsignedShort value) {
        CORRADE_COMPARE(value, 40399);
        ++called;
    });
    CORRADE_VERIFY(!layer.isStorageDirty(storage));
    CORRADE_VERIFY(layer.isDirty(update));
    CORRADE_COMPARE(called, 0);

    ui.update();
    CORRADE_VERIFY(!layer.isStorageDirty(storage));
    CORRADE_VERIFY(!layer.isDirty(update));
    CORRADE_COMPARE(called, 1);
}

void StorageTest::accessCast2D() {
    auto&& data = Access2DData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Like access2D(), but using the value<T>() variant */

    AbstractUserInterface ui{{100, 100}};
    DataLayer& layer = ui.setLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));

    Short storageData[]{
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13
    };
    storageData[data.valueIndex] = -19831;
    Containers::StridedArrayView2D<Short> view{storageData, storageData + data.offset, {2, 3}, data.stride};
    CORRADE_COMPARE(Byte(view[{1, 2}]), -119);

    Storage<Short> storage{layer, view};
    StorageQuery<Byte> query = storage.value<Byte>({1, 2});
    CORRADE_COMPARE(&query.layer(), &layer);
    CORRADE_COMPARE(query.storage(), storage.handle());
    CORRADE_COMPARE(query.index(), (Containers::Size3D{0, 1, 2}));
    CORRADE_VERIFY(!query.isMutable());
    CORRADE_COMPARE(query.operations(), StorageOperations{});
    CORRADE_COMPARE(query, -119);

    Int called = 0;
    DataHandle update = query.onUpdate([&called](Byte value) {
        CORRADE_COMPARE(value, -119);
        ++called;
    });
    CORRADE_VERIFY(!layer.isStorageDirty(storage));
    CORRADE_VERIFY(layer.isDirty(update));
    CORRADE_COMPARE(called, 0);

    ui.update();
    CORRADE_VERIFY(!layer.isStorageDirty(storage));
    CORRADE_VERIFY(!layer.isDirty(update));
    CORRADE_COMPARE(called, 1);
}

void StorageTest::accessCast1D() {
    auto&& data = Access1DData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Like access1D(), but using the value<T>() variant */

    AbstractUserInterface ui{{100, 100}};
    DataLayer& layer = ui.setLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));

    Byte storageData[]{
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14
    };
    storageData[data.valueIndex] = -114;
    Containers::StridedArrayView1D<Byte> view{storageData, storageData + data.offset, 5, data.stride};
    CORRADE_COMPARE(UnsignedByte(view[4]), 142);

    Storage<Byte> storage{layer, view};
    StorageQuery<UnsignedByte> query = storage.value<UnsignedByte>(4);
    CORRADE_COMPARE(&query.layer(), &layer);
    CORRADE_COMPARE(query.storage(), storage.handle());
    CORRADE_COMPARE(query.index(), (Containers::Size3D{0, 0, 4}));
    CORRADE_VERIFY(!query.isMutable());
    CORRADE_COMPARE(query.operations(), StorageOperations{});
    CORRADE_COMPARE(query, 142);

    Int called = 0;
    DataHandle update = query.onUpdate([&called](UnsignedByte value) {
        CORRADE_COMPARE(value, 142);
        ++called;
    });
    CORRADE_VERIFY(!layer.isStorageDirty(storage));
    CORRADE_VERIFY(layer.isDirty(update));
    CORRADE_COMPARE(called, 0);

    ui.update();
    CORRADE_VERIFY(!layer.isStorageDirty(storage));
    CORRADE_VERIFY(!layer.isDirty(update));
    CORRADE_COMPARE(called, 1);
}

void StorageTest::accessCast() {
    /* Like access(), but using the value<T>() variant */

    AbstractUserInterface ui{{100, 100}};
    DataLayer& layer = ui.setLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));

    Double data = -9876.543210;
    Storage<Double> storage{layer, data};
    StorageQuery<Int> query = storage.value<Int>();
    CORRADE_COMPARE(&query.layer(), &layer);
    CORRADE_COMPARE(query.storage(), storage.handle());
    CORRADE_COMPARE(query.index(), (Containers::Size3D{0, 0, 0}));
    CORRADE_VERIFY(!query.isMutable());
    CORRADE_COMPARE(query.operations(), StorageOperations{});
    CORRADE_COMPARE(query, -9876);

    Int called = 0;
    DataHandle update = query.onUpdate([&called](Int value) {
        CORRADE_COMPARE(value, -9876);
        ++called;
    });
    CORRADE_VERIFY(!layer.isStorageDirty(storage));
    CORRADE_VERIFY(layer.isDirty(update));
    CORRADE_COMPARE(called, 0);

    ui.update();
    CORRADE_VERIFY(!layer.isStorageDirty(storage));
    CORRADE_VERIFY(!layer.isDirty(update));
    CORRADE_COMPARE(called, 1);
}

void StorageTest::accessInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    /* All these assertions are fired by StorageQuery, this is just verifying
       that they actually get fired without something else happening first or
       them being sidestepped for example by delegating all APIs to the 3D
       queries. */

    Int data[40];
    Containers::StridedArrayView1D<Int> view = data;

    DataLayer layer{layerHandle(0, 1)};
    Storage<Int> storage1D{layer, view.prefix(15)};
    Storage<Int> storage2D{layer, view.prefix(21).expanded<0, 2>({3, 7})};
    Storage<Int> storage3D{layer, view.expanded<0, 3>({4, 2, 5})};

    Containers::String out;
    Error redirectError{&out};
    /* Index out of bounds */
    storage1D[15];
    storage2D[{2, 7}];
    storage3D[{3, 2, 4}];
    /* Single-item API for a 1D/3D/3D storage even though it's in bounds */
    storage1D.value();
    StorageQuery<Int>{storage1D};
    storage2D.value();
    StorageQuery<Int>{storage2D};
    storage3D.value();
    StorageQuery<Int>{storage3D};
    /* 1D index specified for a 2D/3D storage even though it's in bounds */
    storage2D[0];
    storage3D[0];
    /* 2D index specified for a 3D storage even though it's in bounds */
    storage3D[{0, 0}];
    CORRADE_COMPARE_AS(out,
        "Ui::StorageQuery: index {0, 0, 15} out of range for {1, 1, 15} elements\n"
        "Ui::StorageQuery: index {0, 2, 7} out of range for {1, 3, 7} elements\n"
        "Ui::StorageQuery: index {3, 2, 4} out of range for {4, 2, 5} elements\n"

        "Ui::StorageQuery: expected a single-item storage but got a size of {1, 1, 15}\n"
        "Ui::StorageQuery: expected a single-item storage but got a size of {1, 1, 15}\n"
        "Ui::StorageQuery: expected a single-item storage but got a size of {1, 3, 7}\n"
        "Ui::StorageQuery: expected a single-item storage but got a size of {1, 3, 7}\n"
        "Ui::StorageQuery: expected a single-item storage but got a size of {4, 2, 5}\n"
        "Ui::StorageQuery: expected a single-item storage but got a size of {4, 2, 5}\n"

        "Ui::StorageQuery: expected a 1D storage but got a size of {1, 3, 7}\n"
        "Ui::StorageQuery: expected a 1D storage but got a size of {4, 2, 5}\n"

        "Ui::StorageQuery: expected a 2D storage but got a size of {4, 2, 5}\n",
        TestSuite::Compare::String);
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::StorageTest)
