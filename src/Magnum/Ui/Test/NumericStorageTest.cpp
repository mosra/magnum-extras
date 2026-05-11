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

#include <limits> /* just for comparing min() / max() defaults */
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/Pair.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Containers/String.h>
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/Container.h>
#include <Corrade/TestSuite/Compare/String.h>
#include <Magnum/Math/Constants.h>
#include <Magnum/Math/Half.h>
#include <Magnum/Math/Time.h>
#include <Magnum/Math/Vector2.h> /* AbstractUserInterface constructor size */

#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/NumericStorage.h"
#include "Magnum/Ui/UserInterface.h"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct NumericStorageTest: TestSuite::Tester {
    explicit NumericStorageTest();

    template<class T> void constructValueInit();
    template<class T> void constructNoInit();
    template<class T> void constructDirectInit();
    template<class T> void constructNonOwned3D();
    template<class T> void constructNonOwned2D();
    template<class T> void constructNonOwned1D();
    template<class T> void constructNonOwned();

    void constructHandleRecycle();

    /* Verifies that both query and set accesses the right data index in all
       dimensions */
    void access3D();
    void access2D();
    void access1D();
    void access();

    /* Verifies that both query and set accesses the right data index in all
       dimensions and stride variants, as well as immutable behavior */
    void accessNonOwned3D();
    void accessNonOwned2D();
    void accessNonOwned1D();
    void accessNonOwned();

    void accessInvalid();

    /* Verifies that updates to a value at an arbitrary 3D index all do the
       right thing including setting a dirty bit, both in owned and non-owned
       storages */
    void update();

    /* Verifies behavior of setRange(), setStep() and their effect on value
       updates including clamping and other edge cases */
    void rangeStep();
    template<class T> void rangeStepTypeOverflowSigned();
    template<class T> void rangeStepTypeOverflowUnsigned();
    void rangeStepTypeOverflowFloat();

    void rangeStepInvalid();
};

using namespace Math::Literals;

const struct {
    const char* name;
    bool implicitLayer;
} ConstructData[]{
    {"", false},
    {"implicit layer", true}
};

const struct {
    const char* name;
    bool immutable;
    std::size_t valueIndex;
    std::size_t offset;
    Containers::Stride3D stride;
} AccessNonOwned3DData[]{
    {"", false, 11, 0, {24, 8, 4}},
    {"immutable", true, 11, 0, {24, 8, 4}},
    {"sparse", false, 29, 1, {64, 20, 8}},
    {"negative stride 0", false, 9, 10, {-32, 12, 4}},
    {"negative stride 1", false, 13, 10, {32, -12, 4}},
    {"negative stride 2", false, 25, 1, {64, 20, -8}},
    {"zero stride 0", false, 16, 5, {0, 20, 4}},
    {"zero stride 1", false, 14, 5, {32, 0, 4}},
    {"zero stride 2", false, 23, 5, {32, 20, 0}},
};

const struct {
    const char* name;
    bool immutable;
    std::size_t valueIndex;
    std::size_t offset;
    Containers::Stride2D stride;
} AccessNonOwned2DData[]{
    {"", false, 5, 0, {6, 2}},
    {"immutable", true, 5, 0, {6, 2}},
    {"sparse", false, 10, 1, {10, 4}},
    {"negative stride 0", false, 1, 3, {-12, 4}},
    {"negative stride 1", false, 1, 2, {6, -4}},
    {"zero stride 0", false, 7, 5, {0, 2}},
    {"zero stride 1", false, 10, 5, {10, 0}},
};

const struct {
    const char* name;
    bool immutable;
    std::size_t valueIndex;
    std::size_t offset;
    std::ptrdiff_t stride;
} AccessNonOwned1DData[]{
    {"", false, 4, 0, 1},
    {"immutable", true, 4, 0, 1},
    {"sparse", false, 13, 1, 3},
    {"negative stride", false, 1, 9, -2},
    {"zero stride", false, 5, 5, 0},
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
    Short value;
    Containers::Optional<Containers::Pair<Short, Short>> range;
    Containers::Optional<Short> step;
    StorageOperation operation;
    Containers::Optional<Int> set;
    StorageUpdateState expectedState;
    bool expectedDirty;
    Int expected;
} RangeStepData[]{
    {"set",
        1134, {}, {},
        StorageOperation::Set, 31655,
        StorageUpdateState::Success, true, 31655},
    /* Just to verify that there isn't some weird rogue assertion blowing up
       if encountering a zero (it used to be, which is why this is here) */
    {"set zero",
        1134, {}, {},
        StorageOperation::Set, 0,
        StorageUpdateState::Success, true, 0},
    {"set the same value",
        -1134, {}, {},
        StorageOperation::Set, -1134,
        StorageUpdateState::Success, false, -1134},
    {"set a value larger than representable",
        1134, {}, {},
        StorageOperation::Set, 100000,
        StorageUpdateState::Clamped, true, 32767},
    {"set a value smaller than representable",
        1134, {}, {},
        StorageOperation::Set, -100000,
        StorageUpdateState::Clamped, true, -32768},

    {"set a range that's the same as before and a value that's the same as before",
        1134, {{-32768, 32767}}, {},
        StorageOperation::Set, 1134,
        StorageUpdateState::Success, false, 1134},

    /* Setting the same value in both cases, it'll get clamped according to the
       new range and the storage gets marked as dirty even though the same
       value got set. The value *doesn't* get changed just by setting the
       range. */
    {"set the same value with a range that's before the value",
        1134, {{-1000, 1000}}, {},
        StorageOperation::Set, 1134,
        StorageUpdateState::Clamped, true, 1000},
    {"set the same value with a range that's after the value",
        1134, {{2000, 3000}}, {},
        StorageOperation::Set, 1134,
        StorageUpdateState::Clamped, true, 2000},

    {"set a value before a range",
        1134, {{-2000, 3000}}, {},
        StorageOperation::Set, -2152,
        StorageUpdateState::Clamped, true, -2000},
    {"set a value before a range, already at range begin",
        -2000, {{-2000, 3000}}, {},
        StorageOperation::Set, -2152,
        StorageUpdateState::Clamped, false, -2000},
    {"set a value after a range",
        1134, {{-2000, 3000}}, {},
        StorageOperation::Set, 3265,
        StorageUpdateState::Clamped, true, 3000},
    {"set a value after a range, already at range end",
        3000, {{-2000, 3000}}, {},
        StorageOperation::Set, 3265,
        StorageUpdateState::Clamped, false, 3000},
    {"set a value exactly at range begin",
        1134, {{-2000, 3000}}, {},
        StorageOperation::Set, -2000,
        StorageUpdateState::Success, true, -2000},
    {"set a value exactly at range begin",
        1134, {{-2000, 3000}}, {},
        StorageOperation::Set, -2000,
        StorageUpdateState::Success, true, -2000},
    {"set a value exactly at range begin, already there",
        -2000, {{-2000, 3000}}, {},
        StorageOperation::Set, -2000,
        StorageUpdateState::Success, false, -2000},
    {"set a value exactly at range end",
        1134, {{-2000, 3000}}, {},
        StorageOperation::Set, 3000,
        StorageUpdateState::Success, true, 3000},
    {"set a value exactly at range end, already there",
        3000, {{-2000, 3000}}, {},
        StorageOperation::Set, 3000,
        StorageUpdateState::Success, false, 3000},

    {"set a value with a range being empty",
        1134, {{4000, 4000}}, {},
        StorageOperation::Set, 3764,
        StorageUpdateState::Clamped, true, 4000},
    {"set a value with a range being empty, already there",
        4000, {{4000, 4000}}, {},
        StorageOperation::Set, 4000,
        StorageUpdateState::Success, false, 4000},

    {"increment",
        1134, {}, {},
        StorageOperation::Increment, {},
        StorageUpdateState{}, true, 1135},
    {"decrement",
        1134, {}, {},
        StorageOperation::Decrement, {},
        StorageUpdateState{}, true, 1133},
    {"increment exactly at range end",
        3000, {{-2000, 3000}}, {},
        StorageOperation::Increment, {},
        StorageUpdateState{}, false, 3000},
    {"decrement exactly at range begin",
        -2000, {{-2000, 3000}}, {},
        StorageOperation::Decrement, {},
        StorageUpdateState{}, false, -2000},
    {"increment, custom step",
        1134, {}, 20,
        StorageOperation::Increment, {},
        StorageUpdateState{}, true, 1154},
    {"decrement, custom step",
        1134, {}, 20,
        StorageOperation::Decrement, {},
        StorageUpdateState{}, true, 1114},
    {"increment, custom step, near range end",
        2996, {{-2000, 3000}}, 20,
        StorageOperation::Increment, {},
        StorageUpdateState{}, true, 3000},
    {"decrement, custom step, near range begin",
        -1996, {{-2000, 3000}}, 20,
        StorageOperation::Decrement, {},
        StorageUpdateState{}, true, -2000},
    {"increment, custom negative step",
        1134, {}, -20,
        StorageOperation::Increment, {},
        StorageUpdateState{}, true, 1114},
    {"decrement, custom negative step",
        1134, {}, -20,
        StorageOperation::Decrement, {},
        StorageUpdateState{}, true, 1154},
    /* With just a min() this would underflow the range */
    {"increment, custom negative step, near range begin",
        -1996, {{-2000, 3000}}, -20,
        StorageOperation::Increment, {},
        StorageUpdateState{}, true, -2000},
    /* With just a max() this would overflow the range */
    {"decrement, custom negative step, near range end",
        2996, {{-2000, 3000}}, -20,
        StorageOperation::Decrement, {},
        StorageUpdateState{}, true, 3000},

    {"min",
        1134, {}, {},
        StorageOperation::Min, {},
        StorageUpdateState{}, true, -32768},
    {"max",
        1134, {}, {},
        StorageOperation::Max, {},
        StorageUpdateState{}, true, 32767},
    {"min, custom range",
        1134, {{-2000, 3000}}, {},
        StorageOperation::Min, {},
        StorageUpdateState{}, true, -2000},
    {"max, custom range",
        1134, {{-2000, 3000}}, {},
        StorageOperation::Max, {},
        StorageUpdateState{}, true, 3000},
};

NumericStorageTest::NumericStorageTest() {
    addInstancedTests<NumericStorageTest>({
        &NumericStorageTest::constructValueInit<UnsignedByte>,
        &NumericStorageTest::constructValueInit<Byte>,
        &NumericStorageTest::constructValueInit<UnsignedShort>,
        &NumericStorageTest::constructValueInit<Short>,
        &NumericStorageTest::constructValueInit<UnsignedInt>,
        &NumericStorageTest::constructValueInit<Int>,
        &NumericStorageTest::constructValueInit<UnsignedLong>,
        &NumericStorageTest::constructValueInit<Long>,
        &NumericStorageTest::constructValueInit<Float>,
        &NumericStorageTest::constructValueInit<Double>,
        &NumericStorageTest::constructValueInit<Half>,
        &NumericStorageTest::constructValueInit<Deg>,
        &NumericStorageTest::constructValueInit<Rad>,
        &NumericStorageTest::constructValueInit<Seconds>,
        &NumericStorageTest::constructValueInit<Nanoseconds>,

        &NumericStorageTest::constructNoInit<UnsignedByte>,
        &NumericStorageTest::constructNoInit<Byte>,
        &NumericStorageTest::constructNoInit<UnsignedShort>,
        &NumericStorageTest::constructNoInit<Short>,
        &NumericStorageTest::constructNoInit<UnsignedInt>,
        &NumericStorageTest::constructNoInit<Int>,
        &NumericStorageTest::constructNoInit<UnsignedLong>,
        &NumericStorageTest::constructNoInit<Long>,
        &NumericStorageTest::constructNoInit<Float>,
        &NumericStorageTest::constructNoInit<Double>,
        &NumericStorageTest::constructNoInit<Half>,
        &NumericStorageTest::constructNoInit<Deg>,
        &NumericStorageTest::constructNoInit<Rad>,
        &NumericStorageTest::constructNoInit<Seconds>,
        &NumericStorageTest::constructNoInit<Nanoseconds>,

        &NumericStorageTest::constructDirectInit<UnsignedByte>,
        &NumericStorageTest::constructDirectInit<Byte>,
        &NumericStorageTest::constructDirectInit<UnsignedShort>,
        &NumericStorageTest::constructDirectInit<Short>,
        &NumericStorageTest::constructDirectInit<UnsignedInt>,
        &NumericStorageTest::constructDirectInit<Int>,
        &NumericStorageTest::constructDirectInit<UnsignedLong>,
        &NumericStorageTest::constructDirectInit<Long>,
        &NumericStorageTest::constructDirectInit<Float>,
        &NumericStorageTest::constructDirectInit<Double>,
        &NumericStorageTest::constructDirectInit<Half>,
        &NumericStorageTest::constructDirectInit<Deg>,
        &NumericStorageTest::constructDirectInit<Rad>,
        &NumericStorageTest::constructDirectInit<Seconds>,
        &NumericStorageTest::constructDirectInit<Nanoseconds>,

        &NumericStorageTest::constructNonOwned3D<UnsignedByte>,
        &NumericStorageTest::constructNonOwned3D<Byte>,
        &NumericStorageTest::constructNonOwned3D<UnsignedShort>,
        &NumericStorageTest::constructNonOwned3D<Short>,
        &NumericStorageTest::constructNonOwned3D<UnsignedInt>,
        &NumericStorageTest::constructNonOwned3D<Int>,
        &NumericStorageTest::constructNonOwned3D<UnsignedLong>,
        &NumericStorageTest::constructNonOwned3D<Long>,
        &NumericStorageTest::constructNonOwned3D<Float>,
        &NumericStorageTest::constructNonOwned3D<Double>,
        &NumericStorageTest::constructNonOwned3D<Half>,
        &NumericStorageTest::constructNonOwned3D<Deg>,
        &NumericStorageTest::constructNonOwned3D<Rad>,
        &NumericStorageTest::constructNonOwned3D<Seconds>,
        &NumericStorageTest::constructNonOwned3D<Nanoseconds>,

        &NumericStorageTest::constructNonOwned2D<UnsignedByte>,
        &NumericStorageTest::constructNonOwned2D<Byte>,
        &NumericStorageTest::constructNonOwned2D<UnsignedShort>,
        &NumericStorageTest::constructNonOwned2D<Short>,
        &NumericStorageTest::constructNonOwned2D<UnsignedInt>,
        &NumericStorageTest::constructNonOwned2D<Int>,
        &NumericStorageTest::constructNonOwned2D<UnsignedLong>,
        &NumericStorageTest::constructNonOwned2D<Long>,
        &NumericStorageTest::constructNonOwned2D<Float>,
        &NumericStorageTest::constructNonOwned2D<Double>,
        &NumericStorageTest::constructNonOwned2D<Half>,
        &NumericStorageTest::constructNonOwned2D<Deg>,
        &NumericStorageTest::constructNonOwned2D<Rad>,
        &NumericStorageTest::constructNonOwned2D<Seconds>,
        &NumericStorageTest::constructNonOwned2D<Nanoseconds>,

        &NumericStorageTest::constructNonOwned1D<UnsignedByte>,
        &NumericStorageTest::constructNonOwned1D<Byte>,
        &NumericStorageTest::constructNonOwned1D<UnsignedShort>,
        &NumericStorageTest::constructNonOwned1D<Short>,
        &NumericStorageTest::constructNonOwned1D<UnsignedInt>,
        &NumericStorageTest::constructNonOwned1D<Int>,
        &NumericStorageTest::constructNonOwned1D<UnsignedLong>,
        &NumericStorageTest::constructNonOwned1D<Long>,
        &NumericStorageTest::constructNonOwned1D<Float>,
        &NumericStorageTest::constructNonOwned1D<Double>,
        &NumericStorageTest::constructNonOwned1D<Half>,
        &NumericStorageTest::constructNonOwned1D<Deg>,
        &NumericStorageTest::constructNonOwned1D<Rad>,
        &NumericStorageTest::constructNonOwned1D<Seconds>,
        &NumericStorageTest::constructNonOwned1D<Nanoseconds>,

        &NumericStorageTest::constructNonOwned<UnsignedByte>,
        &NumericStorageTest::constructNonOwned<Byte>,
        &NumericStorageTest::constructNonOwned<UnsignedShort>,
        &NumericStorageTest::constructNonOwned<Short>,
        &NumericStorageTest::constructNonOwned<UnsignedInt>,
        &NumericStorageTest::constructNonOwned<Int>,
        &NumericStorageTest::constructNonOwned<UnsignedLong>,
        &NumericStorageTest::constructNonOwned<Long>,
        &NumericStorageTest::constructNonOwned<Float>,
        &NumericStorageTest::constructNonOwned<Double>,
        &NumericStorageTest::constructNonOwned<Half>,
        &NumericStorageTest::constructNonOwned<Deg>,
        &NumericStorageTest::constructNonOwned<Rad>,
        &NumericStorageTest::constructNonOwned<Seconds>,
        &NumericStorageTest::constructNonOwned<Nanoseconds>},
        Containers::arraySize(ConstructData));

    addTests({&NumericStorageTest::constructHandleRecycle,

              &NumericStorageTest::access3D,
              &NumericStorageTest::access2D,
              &NumericStorageTest::access1D,
              &NumericStorageTest::access});

    addInstancedTests({&NumericStorageTest::accessNonOwned3D},
        Containers::arraySize(AccessNonOwned3DData));

    addInstancedTests({&NumericStorageTest::accessNonOwned2D},
        Containers::arraySize(AccessNonOwned2DData));

    addInstancedTests({&NumericStorageTest::accessNonOwned1D},
        Containers::arraySize(AccessNonOwned1DData));

    addInstancedTests({&NumericStorageTest::accessNonOwned},
        Containers::arraySize(AccessNonOwnedData));

    addTests({&NumericStorageTest::accessInvalid});

    addInstancedTests({&NumericStorageTest::update},
        Containers::arraySize(UpdateData));

    addInstancedTests({&NumericStorageTest::rangeStep},
        Containers::arraySize(RangeStepData));

    addTests({&NumericStorageTest::rangeStepTypeOverflowSigned<Int>,
              &NumericStorageTest::rangeStepTypeOverflowSigned<Long>,
              &NumericStorageTest::rangeStepTypeOverflowUnsigned<UnsignedInt>,
              &NumericStorageTest::rangeStepTypeOverflowUnsigned<UnsignedLong>,
              &NumericStorageTest::rangeStepTypeOverflowFloat,
              &NumericStorageTest::rangeStepInvalid});
}

template<class> struct StorageTraits;
template<> struct StorageTraits<UnsignedByte> {
    static const char* name() { return "UnsignedByte"; }
    static UnsignedByte value() { return 167; }
};
template<> struct StorageTraits<Byte> {
    static const char* name() { return "Byte"; }
    static Byte value() { return -112; }
};
template<> struct StorageTraits<UnsignedShort> {
    static const char* name() { return "UnsignedShort"; }
    static UnsignedShort value() { return 56343; }
};
template<> struct StorageTraits<Short> {
    static const char* name() { return "Short"; }
    static Short value() { return -31765; }
};
template<> struct StorageTraits<UnsignedInt> {
    static const char* name() { return "UnsignedInt"; }
    static UnsignedInt value() { return 3267676658; }
};
template<> struct StorageTraits<Int> {
    static const char* name() { return "Int"; }
    static Int value() { return -1945467672; }
};
template<> struct StorageTraits<UnsignedLong> {
    static const char* name() { return "UnsignedLong"; }
    static UnsignedLong value() { return 1000111222333444ull; }
};
template<> struct StorageTraits<Long> {
    static const char* name() { return "Long"; }
    static Long value() { return -1000111222333444ll; }
};
template<> struct StorageTraits<Float> {
    static const char* name() { return "Float"; }
    static Float value() { return 3.14159264f; }
};
template<> struct StorageTraits<Double> {
    static const char* name() { return "Double"; }
    static Double value() { return 1e100; }
};
template<> struct StorageTraits<Half> {
    static const char* name() { return "Half"; }
    static Half value() { return 3.14159264_h; }
};
template<> struct StorageTraits<Deg> {
    static const char* name() { return "Deg"; }
    static Deg value() { return 180.0_degf; }
};
template<> struct StorageTraits<Rad> {
    static const char* name() { return "Rad"; }
    static Rad value() { return 3.14159264_radf; }
};
template<> struct StorageTraits<Seconds> {
    static const char* name() { return "Seconds"; }
    static Seconds value() { return 3.14159264_sec; }
};
template<> struct StorageTraits<Nanoseconds> {
    static const char* name() { return "Nanoseconds"; }
    static Nanoseconds value() { return -1000111222333444_nsec; }
};

/* Compare against the STL to verify our own limit defaults. For floats it
   returns a smallest representable value in min() and a largest non-infinite
   value in max() which is useless for our purpose, so supply ±inf in that
   case. Nanoseconds are then another special case, std::numeric_limits return
   0, 0 for them which is even more useless than not compiling at all. */
template<class T, typename std::enable_if<!Math::IsFloatingPoint<T>::value, int>::type = 0> Containers::Pair<T, T> defaultRangeFor() {
    return {std::numeric_limits<T>::min(), std::numeric_limits<T>::max()};
}
template<class T, typename std::enable_if<Math::IsFloatingPoint<T>::value, int>::type = 0> Containers::Pair<T, T> defaultRangeFor() {
    return {T(-Constants::inf()), T(+Constants::inf())};
}
template<> Containers::Pair<Nanoseconds, Nanoseconds> defaultRangeFor<Nanoseconds>() {
    return {Nanoseconds::min(), Nanoseconds::max()};
}

template<class T> void NumericStorageTest::constructValueInit() {
    auto&& data = ConstructData[testCaseInstanceId()];
    setTestCaseDescription(data.name);
    setTestCaseTemplateName(StorageTraits<T>::name());

    /* Either this instance or the implicit one below gets used */
    DataLayer layer{layerHandle(0, 1)};

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    ui.setDataLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));

    /* On 64-bit up to 32 bytes fits in place, on 32-bit up to 20, adjust one
       coordinate in each size to fit exactly on both bitness variants */
    #ifndef CORRADE_TARGET_32BIT
    constexpr std::size_t sizeFirstY = 7;
    #else
    constexpr std::size_t sizeFirstY = 4;
    #endif

    NumericStorage<T> first1 = data.implicitLayer ?
        NumericStorage<T>{ui, ValueInit, {2, sizeFirstY, 2}, StorageFlags{0x18}} :
        NumericStorage<T>{layer, ValueInit, {2, sizeFirstY, 2}, StorageFlags{0x18}};
    NumericStorage<T> first2 = data.implicitLayer ?
        NumericStorage<T>{ui, {2, sizeFirstY, 2}, StorageFlags{0x18}} :
        NumericStorage<T>{layer, {2, sizeFirstY, 2}, StorageFlags{0x18}};
    CORRADE_COMPARE(&first1.layer(), data.implicitLayer ? &ui.dataLayer() : &layer);
    CORRADE_COMPARE(&first2.layer(), data.implicitLayer ? &ui.dataLayer() : &layer);
    /* 1-byte storage fits in-place (28 / 16 + 4*1 bytes), larger not
       anymore */
    CORRADE_COMPARE(first1.isAllocated(), sizeof(T) > 1);
    CORRADE_COMPARE(first1.isAllocated(), sizeof(T) > 1);
    CORRADE_VERIFY(!first1.isDirty());
    CORRADE_VERIFY(!first2.isDirty());
    CORRADE_COMPARE(first1.flags(), StorageFlags{0x18});
    CORRADE_COMPARE(first2.flags(), StorageFlags{0x18});
    CORRADE_COMPARE(first1.size(), (Containers::Size3D{2, sizeFirstY, 2}));
    CORRADE_COMPARE(first2.size(), (Containers::Size3D{2, sizeFirstY, 2}));
    CORRADE_COMPARE(first1.stride(), (Containers::Stride3D{sizeFirstY *2*sizeof(T), 2*sizeof(T), sizeof(T)}));
    CORRADE_COMPARE(first2.stride(), (Containers::Stride3D{sizeFirstY *2*sizeof(T), 2*sizeof(T), sizeof(T)}));

    /* See comment at defaultRangeFor() for details */
    CORRADE_COMPARE(first1.range(), defaultRangeFor<T>());
    /* Using 1.0 because unlike 1, which is ambiguous conversion to Half, this
       isn't ambigous for any type. Lol. */
    CORRADE_COMPARE(first1.step(), T(1.0));
    CORRADE_COMPARE(first2.step(), T(1.0));

    /* Verify at least one element to ensure the operator[]() (and value(), and
       operator StorageQuery<>()) is implemented for all types. Thorough
       indexing tests are in access1D() etc. below. For Half the storage gets
       expanded to Float and the two types aren't implicitly convertible to
       each other so I have to cast. */
    CORRADE_COMPARE((first1[{1, 3, 0}]),
        static_cast<typename decltype(first1)::Type>(T{}));
    CORRADE_COMPARE((first2[{1, 3, 0}]),
        static_cast<typename decltype(first2)::Type>(T{}));

    /* The data should have the same shape as reported above and be a
       contiguous sequence of zeros */
    Containers::StridedArrayView3D<const T> view1 = first1.data();
    Containers::StridedArrayView3D<const T> view2 = first2.data();
    CORRADE_COMPARE(view1.size(), first1.size());
    CORRADE_COMPARE(view2.size(), first2.size());
    CORRADE_COMPARE(view1.stride(), first1.stride());
    CORRADE_COMPARE(view2.stride(), first2.stride());
    CORRADE_VERIFY(view1.isContiguous());
    CORRADE_VERIFY(view2.isContiguous());
    CORRADE_COMPARE_AS(view1.asContiguous(),
        Containers::stridedArrayView({T{}}).template broadcasted<0>(2* sizeFirstY *2),
        TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(view2.asContiguous(),
        Containers::stridedArrayView({T{}}).template broadcasted<0>(2* sizeFirstY *2),
        TestSuite::Compare::Container);

    /* 2D, 1D and single-item variants delegate to the 3D constructor, so
       verify just that they propagate all arguments correctly */
    #ifndef CORRADE_TARGET_32BIT
    constexpr std::size_t sizeSecondX = 4;
    #else
    constexpr std::size_t sizeSecondX = 2;
    #endif
    NumericStorage<T> second1 = data.implicitLayer ?
        NumericStorage<T>{ui, ValueInit, {sizeSecondX, 3}, StorageFlags{0x20}} :
        NumericStorage<T>{layer, ValueInit, {sizeSecondX, 3}, StorageFlags{0x20}};
    NumericStorage<T> second2 = data.implicitLayer ?
        NumericStorage<T>{ui, {sizeSecondX, 3}, StorageFlags{0x20}} :
        NumericStorage<T>{layer, {sizeSecondX, 3}, StorageFlags{0x20}};
    CORRADE_COMPARE(&second1.layer(), data.implicitLayer ? &ui.dataLayer() : &layer);
    CORRADE_COMPARE(&second2.layer(), data.implicitLayer ? &ui.dataLayer() : &layer);
    /* 2-byte storage fits in-place (24 / 12 + 4*2 bytes), larger not
       anymore */
    CORRADE_COMPARE(second1.isAllocated(), sizeof(T) > 2);
    CORRADE_COMPARE(second2.isAllocated(), sizeof(T) > 2);
    CORRADE_VERIFY(!second1.isDirty());
    CORRADE_VERIFY(!second2.isDirty());
    CORRADE_COMPARE(second1.flags(), StorageFlags{0x20});
    CORRADE_COMPARE(second2.flags(), StorageFlags{0x20});
    CORRADE_COMPARE(second1.size(), (Containers::Size3D{1, sizeSecondX, 3}));
    CORRADE_COMPARE(second2.size(), (Containers::Size3D{1, sizeSecondX, 3}));
    CORRADE_COMPARE(second1.stride(), (Containers::Stride3D{sizeSecondX*3*sizeof(T), 3*sizeof(T), sizeof(T)}));
    CORRADE_COMPARE(second2.stride(), (Containers::Stride3D{sizeSecondX*3*sizeof(T), 3*sizeof(T), sizeof(T)}));
    CORRADE_COMPARE((second1[{1, 2}]), static_cast<typename decltype(second1)::Type>(T{}));
    CORRADE_COMPARE((second2[{1, 2}]), static_cast<typename decltype(second2)::Type>(T{}));
    CORRADE_COMPARE_AS(second1.data().asContiguous(),
        Containers::stridedArrayView({T{}}).template broadcasted<0>(sizeSecondX*3),
        TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(second2.data().asContiguous(),
        Containers::stridedArrayView({T{}}).template broadcasted<0>(sizeSecondX*3),
        TestSuite::Compare::Container);

    #ifndef CORRADE_TARGET_32BIT
    constexpr std::size_t sizeThird = 4;
    #else
    constexpr std::size_t sizeThird = 3;
    #endif
    NumericStorage<T> third1 = data.implicitLayer ?
        NumericStorage<T>{ui, ValueInit, sizeThird, StorageFlags{0x10}} :
        NumericStorage<T>{layer, ValueInit, sizeThird, StorageFlags{0x10}};
    NumericStorage<T> third2 = data.implicitLayer ?
        NumericStorage<T>{ui, sizeThird, StorageFlags{0x10}} :
        NumericStorage<T>{layer, sizeThird, StorageFlags{0x10}};
    CORRADE_COMPARE(&third1.layer(), data.implicitLayer ? &ui.dataLayer() : &layer);
    CORRADE_COMPARE(&third2.layer(), data.implicitLayer ? &ui.dataLayer() : &layer);
    /* 4-byte storage fits in-place on 64-bit (16 + 4*4 bytes), larger not
       anymore. On 32-bit only up to 2-byte (12 + 2*4 bytes), with a 4-byte
       storage only a single item would fit, which is tested in the single-item
       case below already. */
    #ifndef CORRADE_TARGET_32BIT
    CORRADE_COMPARE(third1.isAllocated(), sizeof(T) > 4);
    CORRADE_COMPARE(third2.isAllocated(), sizeof(T) > 4);
    #else
    CORRADE_COMPARE(third1.isAllocated(), sizeof(T) > 2);
    CORRADE_COMPARE(third2.isAllocated(), sizeof(T) > 2);
    #endif
    CORRADE_VERIFY(!third1.isDirty());
    CORRADE_VERIFY(!third2.isDirty());
    CORRADE_COMPARE(third1.flags(), StorageFlags{0x10});
    CORRADE_COMPARE(third2.flags(), StorageFlags{0x10});
    CORRADE_COMPARE(third1.size(), (Containers::Size3D{1, 1, sizeThird}));
    CORRADE_COMPARE(third2.size(), (Containers::Size3D{1, 1, sizeThird}));
    CORRADE_COMPARE(third1.stride(), (Containers::Stride3D{sizeThird*sizeof(T), sizeThird*sizeof(T), sizeof(T)}));
    CORRADE_COMPARE(third2.stride(), (Containers::Stride3D{sizeThird*sizeof(T), sizeThird*sizeof(T), sizeof(T)}));
    CORRADE_COMPARE(third1[1], static_cast<typename decltype(third1)::Type>(T{}));
    CORRADE_COMPARE(third2[1], static_cast<typename decltype(third2)::Type>(T{}));
    CORRADE_COMPARE_AS(third1.data().asContiguous(),
        Containers::stridedArrayView({T{}}).template broadcasted<0>(sizeThird),
        TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(third2.data().asContiguous(),
        Containers::stridedArrayView({T{}}).template broadcasted<0>(sizeThird),
        TestSuite::Compare::Container);

    NumericStorage<T> fourth1 = data.implicitLayer ?
        NumericStorage<T>{ui, ValueInit, StorageFlags{0x08}} :
        NumericStorage<T>{layer, ValueInit, StorageFlags{0x08}};
    NumericStorage<T> fourth2 = data.implicitLayer ?
        NumericStorage<T>{ui, StorageFlags{0x08}} :
        NumericStorage<T>{layer, StorageFlags{0x08}};
    CORRADE_COMPARE(&fourth1.layer(), data.implicitLayer ? &ui.dataLayer() : &layer);
    CORRADE_COMPARE(&fourth2.layer(), data.implicitLayer ? &ui.dataLayer() : &layer);
    /* 8-byte storage doesn't fit in-place anymore (8 + 4*8 bytes). On 32-bit
       it's the same case although the threshold is right at four bytes
       (4 + 4*4 bytes fits exactly). */
    CORRADE_COMPARE(fourth1.isAllocated(), sizeof(T) > 4);
    CORRADE_COMPARE(fourth2.isAllocated(), sizeof(T) > 4);
    CORRADE_VERIFY(!fourth1.isDirty());
    CORRADE_VERIFY(!fourth2.isDirty());
    CORRADE_COMPARE(fourth1.flags(), StorageFlags{0x08});
    CORRADE_COMPARE(fourth2.flags(), StorageFlags{0x08});
    CORRADE_COMPARE(fourth1.size(), (Containers::Size3D{1, 1, 1}));
    CORRADE_COMPARE(fourth2.size(), (Containers::Size3D{1, 1, 1}));
    CORRADE_COMPARE(fourth1.stride(), (Containers::Stride3D{sizeof(T), sizeof(T), sizeof(T)}));
    CORRADE_COMPARE(fourth2.stride(), (Containers::Stride3D{sizeof(T), sizeof(T), sizeof(T)}));
    CORRADE_COMPARE(fourth1.value(), static_cast<typename decltype(fourth1)::Type>(T{}));
    CORRADE_COMPARE(fourth2.value(), static_cast<typename decltype(fourth2)::Type>(T{}));
    CORRADE_COMPARE(StorageQuery<typename decltype(fourth1)::Type>{fourth1}, static_cast<typename decltype(fourth1)::Type>(T{}));
    CORRADE_COMPARE(StorageQuery<typename decltype(fourth1)::Type>{fourth2}, static_cast<typename decltype(first2)::Type>(T{}));
    CORRADE_COMPARE_AS(fourth1.data().asContiguous(),
        Containers::arrayView({T{}}),
        TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(fourth2.data().asContiguous(),
        Containers::arrayView({T{}}),
        TestSuite::Compare::Container);
}

template<class T> void NumericStorageTest::constructNoInit() {
    auto&& data = ConstructData[testCaseInstanceId()];
    setTestCaseDescription(data.name);
    setTestCaseTemplateName(StorageTraits<T>::name());

    /* Copy of constructValueInit() with NoInit-specific differences */

    /* Either this instance or the implicit one below gets used */
    DataLayer layer{layerHandle(0, 1)};

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    ui.setDataLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));

    #ifndef CORRADE_TARGET_32BIT
    constexpr std::size_t sizeFirstY = 7;
    #else
    constexpr std::size_t sizeFirstY = 4;
    #endif
    NumericStorage<T> first = data.implicitLayer ?
        NumericStorage<T>{ui, NoInit, {2, sizeFirstY, 2}, StorageFlags{0x18}} :
        NumericStorage<T>{layer, NoInit, {2, sizeFirstY, 2}, StorageFlags{0x18}};
    CORRADE_COMPARE(&first.layer(), data.implicitLayer ? &ui.dataLayer() : &layer);
    CORRADE_COMPARE(first.isAllocated(), sizeof(T) > 1);
    CORRADE_VERIFY(!first.isDirty());
    CORRADE_COMPARE(first.flags(), StorageFlags{0x18});
    CORRADE_COMPARE(first.size(), (Containers::Size3D{2, sizeFirstY, 2}));
    CORRADE_COMPARE(first.stride(), (Containers::Stride3D{sizeFirstY*2*sizeof(T), 2*sizeof(T), sizeof(T)}));
    CORRADE_COMPARE(first.range(), defaultRangeFor<T>());
    CORRADE_COMPARE(first.step(), T(1.0));

    /* Cannot test the operator[](), value() and operator StorageQuery() here
       as they return an uninitalized value. Thorough indexing tests are in
       access1D() etc. */

    /* The data should have the same shape as reported above. Can't verify the
       contents as the allocation can have just anything. */
    Containers::StridedArrayView3D<const T> view = first.data();
    CORRADE_COMPARE(view.size(), first.size());
    CORRADE_COMPARE(view.stride(), first.stride());
    CORRADE_VERIFY(view.isContiguous());

    /* 2D and 1D variants delegate to the 3D constructor, so verify just that
       they propagate all arguments correctly */
    #ifndef CORRADE_TARGET_32BIT
    constexpr std::size_t sizeSecondX = 4;
    #else
    constexpr std::size_t sizeSecondX = 2;
    #endif
    NumericStorage<T> second = data.implicitLayer ?
        NumericStorage<T>{ui, NoInit, {sizeSecondX, 3}, StorageFlags{0x20}} :
        NumericStorage<T>{layer, NoInit, {sizeSecondX, 3}, StorageFlags{0x20}};
    CORRADE_COMPARE(&second.layer(), data.implicitLayer ? &ui.dataLayer() : &layer);
    CORRADE_COMPARE(second.isAllocated(), sizeof(T) > 2);
    CORRADE_VERIFY(!second.isDirty());
    CORRADE_COMPARE(second.flags(), StorageFlags{0x20});
    CORRADE_COMPARE(second.size(), (Containers::Size3D{1, sizeSecondX, 3}));
    CORRADE_COMPARE(second.stride(), (Containers::Stride3D{sizeSecondX*3*sizeof(T), 3*sizeof(T), sizeof(T)}));

    #ifndef CORRADE_TARGET_32BIT
    constexpr std::size_t sizeThird = 4;
    #else
    constexpr std::size_t sizeThird = 3;
    #endif
    NumericStorage<T> third = data.implicitLayer ?
        NumericStorage<T>{ui, NoInit, sizeThird, StorageFlags{0x10}} :
        NumericStorage<T>{layer, NoInit, sizeThird, StorageFlags{0x10}};
    CORRADE_COMPARE(&third.layer(), data.implicitLayer ? &ui.dataLayer() : &layer);
    #ifndef CORRADE_TARGET_32BIT
    CORRADE_COMPARE(third.isAllocated(), sizeof(T) > 4);
    #else
    CORRADE_COMPARE(third.isAllocated(), sizeof(T) > 2);
    #endif
    CORRADE_VERIFY(!third.isDirty());
    CORRADE_COMPARE(third.flags(), StorageFlags{0x10});
    CORRADE_COMPARE(third.size(), (Containers::Size3D{1, 1, sizeThird}));
    CORRADE_COMPARE(third.stride(), (Containers::Stride3D{sizeThird*sizeof(T), sizeThird*sizeof(T), sizeof(T)}));

    /* For single-item storages, types below 8 bytes fit in-place, in which
       case we can create, fill & remove a storage and then recycling the slot
       gives us an ability to check that the value was indeed left over from
       before. 8-byte types are allocated and anything can happen there -- the
       exact memory reused, stomped over by something else in the meantime or a
       different memory being allocated next time. */
    StorageHandle previousHandle{};
    if(sizeof(T) < 8) {
        NumericStorage<T> previous{data.implicitLayer ? ui.dataLayer() : layer};
        previousHandle = previous.handle();
        /* For Half the storage gets expanded to Float and the two types aren't
           implicitly convertible to each other so I have to cast. */
        CORRADE_VERIFY(!previous.isAllocated());
        previous.value().set(static_cast<typename decltype(previous)::Type>(StorageTraits<T>::value()));
        (data.implicitLayer ? ui.dataLayer() : layer).removeStorage(previous);
    }

    NumericStorage<T> fourth = data.implicitLayer ?
        NumericStorage<T>{ui, NoInit, StorageFlags{0x08}} :
        NumericStorage<T>{layer, NoInit, StorageFlags{0x08}};
    if(sizeof(T) < 8)
        CORRADE_COMPARE(storageHandleId(fourth.handle()), storageHandleId(previousHandle));
    CORRADE_COMPARE(&fourth.layer(), data.implicitLayer ? &ui.dataLayer() : &layer);
    CORRADE_COMPARE(fourth.isAllocated(), sizeof(T) > 4);
    CORRADE_VERIFY(!fourth.isDirty());
    CORRADE_COMPARE(fourth.flags(), StorageFlags{0x08});
    CORRADE_COMPARE(fourth.size(), (Containers::Size3D{1, 1, 1}));
    CORRADE_COMPARE(fourth.stride(), (Containers::Stride3D{sizeof(T), sizeof(T), sizeof(T)}));
    if(sizeof(T) < 8) {
        CORRADE_COMPARE(fourth.value(), static_cast<typename decltype(fourth)::Type>(StorageTraits<T>::value()));
        CORRADE_COMPARE(StorageQuery<typename decltype(fourth)::Type>{fourth}, static_cast<typename decltype(fourth)::Type>(StorageTraits<T>::value()));
        CORRADE_COMPARE_AS(fourth.data().asContiguous(),
            Containers::arrayView({StorageTraits<T>::value()}),
            TestSuite::Compare::Container);
    }
}

template<class T> void NumericStorageTest::constructDirectInit() {
    auto&& data = ConstructData[testCaseInstanceId()];
    setTestCaseDescription(data.name);
    setTestCaseTemplateName(StorageTraits<T>::name());

    /* Copy of constructValueInit() with DirectInit-specific differences */

    /* Either this instance or the implicit one below gets used */
    DataLayer layer{layerHandle(0, 1)};

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    ui.setDataLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));

    #ifndef CORRADE_TARGET_32BIT
    constexpr std::size_t sizeFirstY = 7;
    #else
    constexpr std::size_t sizeFirstY = 4;
    #endif
    NumericStorage<T> first = data.implicitLayer ?
        NumericStorage<T>{ui, DirectInit, {2, sizeFirstY, 2}, StorageTraits<T>::value(), StorageFlags{0x18}} :
        NumericStorage<T>{layer, DirectInit, {2, sizeFirstY, 2}, StorageTraits<T>::value(), StorageFlags{0x18}};
    CORRADE_COMPARE(&first.layer(), data.implicitLayer ? &ui.dataLayer() : &layer);
    CORRADE_COMPARE(first.isAllocated(), sizeof(T) > 1);
    CORRADE_VERIFY(!first.isDirty());
    CORRADE_COMPARE(first.flags(), StorageFlags{0x18});
    CORRADE_COMPARE(first.size(), (Containers::Size3D{2, sizeFirstY, 2}));
    CORRADE_COMPARE(first.stride(), (Containers::Stride3D{sizeFirstY*2*sizeof(T), 2*sizeof(T), sizeof(T)}));
    CORRADE_COMPARE(first.range(), defaultRangeFor<T>());
    CORRADE_COMPARE(first.step(), T(1.0));

    /* The operator[](), value() and operator StorageQuery() is tested in
       constructValueInit() already, no need to do that again here, verify only
       the raw data views. Thorough indexing tests are in access1D() etc. */

    /* The data should have the same shape as reported above and be a
       contiguous sequence repeating the value supplied in the constructor */
    Containers::StridedArrayView3D<const T> view = first.data();
    CORRADE_COMPARE(view.size(), first.size());
    CORRADE_COMPARE(view.stride(), first.stride());
    CORRADE_VERIFY(view.isContiguous());
    CORRADE_COMPARE_AS(view.asContiguous(),
        Containers::stridedArrayView({StorageTraits<T>::value()}).template broadcasted<0>(2*sizeFirstY*2),
        TestSuite::Compare::Container);

    /* 2D, 1D and single-item variants delegate to the 3D constructor, so
       verify just that they propagate all arguments correctly */
    #ifndef CORRADE_TARGET_32BIT
    constexpr std::size_t sizeSecondX = 4;
    #else
    constexpr std::size_t sizeSecondX = 2;
    #endif
    NumericStorage<T> second = data.implicitLayer ?
        NumericStorage<T>{ui, DirectInit, {sizeSecondX, 3}, StorageTraits<T>::value(), StorageFlags{0x20}} :
        NumericStorage<T>{layer, DirectInit, {sizeSecondX, 3}, StorageTraits<T>::value(), StorageFlags{0x20}};
    CORRADE_COMPARE(&second.layer(), data.implicitLayer ? &ui.dataLayer() : &layer);
    CORRADE_COMPARE(second.isAllocated(), sizeof(T) > 2);
    CORRADE_VERIFY(!second.isDirty());
    CORRADE_COMPARE(second.flags(), StorageFlags{0x20});
    CORRADE_COMPARE(second.size(), (Containers::Size3D{1, sizeSecondX, 3}));
    CORRADE_COMPARE(second.stride(), (Containers::Stride3D{sizeSecondX*3*sizeof(T), 3*sizeof(T), sizeof(T)}));
    CORRADE_COMPARE_AS(second.data().asContiguous(),
        Containers::stridedArrayView({StorageTraits<T>::value()}).template broadcasted<0>(sizeSecondX*3),
        TestSuite::Compare::Container);

    #ifndef CORRADE_TARGET_32BIT
    constexpr std::size_t sizeThird = 4;
    #else
    constexpr std::size_t sizeThird = 3;
    #endif
    NumericStorage<T> third = data.implicitLayer ?
        NumericStorage<T>{ui, DirectInit, sizeThird, StorageTraits<T>::value(), StorageFlags{0x10}} :
        NumericStorage<T>{layer, DirectInit, sizeThird, StorageTraits<T>::value(), StorageFlags{0x10}};
    CORRADE_COMPARE(&third.layer(), data.implicitLayer ? &ui.dataLayer() : &layer);
    #ifndef CORRADE_TARGET_32BIT
    CORRADE_COMPARE(third.isAllocated(), sizeof(T) > 4);
    #else
    CORRADE_COMPARE(third.isAllocated(), sizeof(T) > 2);
    #endif
    CORRADE_VERIFY(!third.isDirty());
    CORRADE_COMPARE(third.flags(), StorageFlags{0x10});
    CORRADE_COMPARE(third.size(), (Containers::Size3D{1, 1, sizeThird}));
    CORRADE_COMPARE(third.stride(), (Containers::Stride3D{sizeThird*sizeof(T), sizeThird*sizeof(T), sizeof(T)}));
    CORRADE_COMPARE_AS(third.data().asContiguous(),
        Containers::stridedArrayView({StorageTraits<T>::value()}).template broadcasted<0>(sizeThird),
        TestSuite::Compare::Container);

    NumericStorage<T> fourth = data.implicitLayer ?
        NumericStorage<T>{ui, DirectInit, StorageTraits<T>::value(), StorageFlags{0x08}} :
        NumericStorage<T>{layer, DirectInit, StorageTraits<T>::value(), StorageFlags{0x08}};
    CORRADE_COMPARE(&fourth.layer(), data.implicitLayer ? &ui.dataLayer() : &layer);
    CORRADE_COMPARE(fourth.isAllocated(), sizeof(T) > 4);
    CORRADE_VERIFY(!fourth.isDirty());
    CORRADE_COMPARE(fourth.flags(), StorageFlags{0x08});
    CORRADE_COMPARE(fourth.size(), (Containers::Size3D{1, 1, 1}));
    CORRADE_COMPARE(fourth.stride(), (Containers::Stride3D{sizeof(T), sizeof(T), sizeof(T)}));
    CORRADE_COMPARE_AS(fourth.data().asContiguous(),
        Containers::arrayView({StorageTraits<T>::value()}),
        TestSuite::Compare::Container);
}

template<class T> void NumericStorageTest::constructNonOwned3D() {
    auto&& data = ConstructData[testCaseInstanceId()];
    setTestCaseDescription(data.name);
    setTestCaseTemplateName(StorageTraits<T>::name());

    /* Variant of constructValueInit() testing specifics of 3D NonOwned
       construction. 2D, 1D and single-item variants tested in
       constructNonOwned2D() etc. below. */

    /* Either this instance or the implicit one below gets used */
    DataLayer layer{layerHandle(0, 1)};

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    ui.setDataLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));

    T storageData[30];

    /* The random `template` keywords are getting out of hand ffs. Transposed
       so we don't have a trivial stride. */
    Containers::StridedArrayView3D<T> storageView =
        Containers::stridedArrayView(storageData)
            .template expanded<0, 3>({2, 3, 5})
            .template transposed<1, 2>();
    Containers::StridedArrayView3D<const T> constStorageView = storageView;

    NumericStorage<T> storage1 = data.implicitLayer ?
        NumericStorage<T>{ui, NonOwned, storageView, StorageFlags{0x18}} :
        NumericStorage<T>{layer, NonOwned, storageView, StorageFlags{0x18}};
    NumericStorage<T> storage2 = data.implicitLayer ?
        NumericStorage<T>{ui, NonOwned, constStorageView, StorageFlags{0x18}} :
        NumericStorage<T>{layer, NonOwned, constStorageView, StorageFlags{0x18}};
    CORRADE_COMPARE(&storage1.layer(), data.implicitLayer ? &ui.dataLayer() : &layer);
    CORRADE_COMPARE(&storage2.layer(), data.implicitLayer ? &ui.dataLayer() : &layer);
    /* 3D storages don't fit in-place on 64 bits because even pointer + 3D
       stride is 32 bytes already. On 32-bit it can fit only for single-byte
       types. */
    #ifndef CORRADE_TARGET_32BIT
    CORRADE_VERIFY(storage1.isAllocated());
    CORRADE_VERIFY(storage2.isAllocated());
    #else
    CORRADE_COMPARE(storage1.isAllocated(), sizeof(T) > 1);
    CORRADE_COMPARE(storage2.isAllocated(), sizeof(T) > 1);
    #endif
    CORRADE_VERIFY(!storage1.isDirty());
    CORRADE_VERIFY(!storage2.isDirty());
    CORRADE_COMPARE(storage1.flags(), StorageFlags{0x18});
    CORRADE_COMPARE(storage2.flags(), StorageFlags{0x18});
    CORRADE_COMPARE(storage1.size(), (Containers::Size3D{2, 5, 3}));
    CORRADE_COMPARE(storage2.size(), (Containers::Size3D{2, 5, 3}));
    /* The view is transposed so the stride should be too */
    CORRADE_COMPARE(storage1.stride(), (Containers::Stride3D{5*3*sizeof(T), sizeof(T), 5*sizeof(T)}));
    CORRADE_COMPARE(storage2.stride(), (Containers::Stride3D{5*3*sizeof(T), sizeof(T), 5*sizeof(T)}));
    CORRADE_COMPARE(storage1.range(), defaultRangeFor<T>());
    CORRADE_COMPARE(storage2.range(), defaultRangeFor<T>());
    CORRADE_COMPARE(storage1.step(), T(1.0));
    CORRADE_COMPARE(storage2.step(), T(1.0));

    /* The operator[](), value() and operator StorageQuery() is tested for all
       types in constructValueInit() already, and verifying just non-owned
       getters here would be insufficient anyway. Thorough indexing tests for
       non-owned data are in access1D() etc. Here check just that the const
       variant doesn't have an updater set. */
    StorageQuery<typename decltype(storage1)::Type> query1 = storage1[{0, 0, 0}];
    StorageQuery<typename decltype(storage2)::Type> query2 = storage2[{0, 0, 0}];
    CORRADE_COMPARE(query1.operations(), StorageOperation::Set|StorageOperation::Min|StorageOperation::Max|StorageOperation::Increment|StorageOperation::Decrement);
    CORRADE_COMPARE(query2.operations(), StorageOperation::Min|StorageOperation::Max);
    CORRADE_VERIFY(query1.updater());
    CORRADE_VERIFY(!query2.updater());

    /* The data should be the same views as passed to the constructor */
    Containers::StridedArrayView3D<const T> view1 = storage1.data();
    Containers::StridedArrayView3D<const T> view2 = storage2.data();
    CORRADE_COMPARE(view1.data(), storageView.data());
    CORRADE_COMPARE(view2.data(), constStorageView.data());
    CORRADE_COMPARE(view1.size(), storageView.size());
    CORRADE_COMPARE(view2.size(), constStorageView.size());
    CORRADE_COMPARE(view1.stride(), storageView.stride());
    CORRADE_COMPARE(view2.stride(), constStorageView.stride());
}

template<class T> void NumericStorageTest::constructNonOwned2D() {
    auto&& data = ConstructData[testCaseInstanceId()];
    setTestCaseDescription(data.name);
    setTestCaseTemplateName(StorageTraits<T>::name());

    /* Variant of constructNonOwned3D() testing the 2D case. Compared to owned
       constructors the non-owned don't all delegate to 3D, and result in
       different internal layout, so each case needs to be tested
       separately. */

    /* Either this instance or the implicit one below gets used */
    DataLayer layer{layerHandle(0, 1)};

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    ui.setDataLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));

    T storageData[6];

    /* Transposed so we don't have a trivial stride */
    Containers::StridedArrayView2D<T> storageView =
        Containers::stridedArrayView(storageData)
            .template expanded<0, 2>({3, 2})
            .template transposed<0, 1>();
    Containers::StridedArrayView2D<const T> constStorageView = storageView;

    NumericStorage<T> storage1 = data.implicitLayer ?
        NumericStorage<T>{ui, NonOwned, storageView, StorageFlags{0x20}} :
        NumericStorage<T>{layer, NonOwned, storageView, StorageFlags{0x20}};
    NumericStorage<T> storage2 = data.implicitLayer ?
        NumericStorage<T>{ui, NonOwned, constStorageView, StorageFlags{0x20}} :
        NumericStorage<T>{layer, NonOwned, constStorageView, StorageFlags{0x20}};
    CORRADE_COMPARE(&storage1.layer(), data.implicitLayer ? &ui.dataLayer() : &layer);
    CORRADE_COMPARE(&storage2.layer(), data.implicitLayer ? &ui.dataLayer() : &layer);
    /* On 64-bit pointer + 2D stride is 24 bytes, so the storage fits in-place
       only if the remaining four members are at most 8 bytes. On 32-bit
       pointer + 2D stride is 12 so it's again 8 bytes left for the rest. */
    CORRADE_COMPARE(storage1.isAllocated(), sizeof(T) > 2);
    CORRADE_COMPARE(storage2.isAllocated(), sizeof(T) > 2);
    CORRADE_VERIFY(!storage1.isDirty());
    CORRADE_VERIFY(!storage2.isDirty());
    CORRADE_COMPARE(storage1.flags(), StorageFlags{0x20});
    CORRADE_COMPARE(storage2.flags(), StorageFlags{0x20});
    CORRADE_COMPARE(storage1.size(), (Containers::Size3D{1, 2, 3}));
    CORRADE_COMPARE(storage2.size(), (Containers::Size3D{1, 2, 3}));
    /* The view is transposed so the stride should be too */
    CORRADE_COMPARE(storage1.stride(), (Containers::Stride3D{2*sizeof(T), sizeof(T), 2*sizeof(T)}));
    CORRADE_COMPARE(storage2.stride(), (Containers::Stride3D{2*sizeof(T), sizeof(T), 2*sizeof(T)}));
    CORRADE_COMPARE(storage1.range(), defaultRangeFor<T>());
    CORRADE_COMPARE(storage2.range(), defaultRangeFor<T>());
    CORRADE_COMPARE(storage1.step(), T(1.0));
    CORRADE_COMPARE(storage2.step(), T(1.0));

    /* Check just that the const variant doesn't have an updater set */
    StorageQuery<typename decltype(storage1)::Type> query1 = storage1[{0, 0}];
    StorageQuery<typename decltype(storage2)::Type> query2 = storage2[{0, 0}];
    CORRADE_COMPARE(query1.operations(), StorageOperation::Set|StorageOperation::Min|StorageOperation::Max|StorageOperation::Increment|StorageOperation::Decrement);
    CORRADE_COMPARE(query2.operations(), StorageOperation::Min|StorageOperation::Max);
    CORRADE_VERIFY(query1.updater());
    CORRADE_VERIFY(!query2.updater());

    /* The data should be the same views as passed to the constructor, expanded
       to 3D */
    Containers::StridedArrayView3D<const T> view1 = storage1.data();
    Containers::StridedArrayView3D<const T> view2 = storage2.data();
    CORRADE_COMPARE(view1.data(), storageView.data());
    CORRADE_COMPARE(view2.data(), constStorageView.data());
    CORRADE_COMPARE(view1.size(), Containers::StridedArrayView3D<T>{storageView}.size());
    CORRADE_COMPARE(view2.size(), Containers::StridedArrayView3D<const T>{constStorageView}.size());
    CORRADE_COMPARE(view1.stride(), Containers::StridedArrayView3D<T>{storageView}.stride());
    CORRADE_COMPARE(view2.stride(), Containers::StridedArrayView3D<const T>{constStorageView}.stride());
}

template<class T> void NumericStorageTest::constructNonOwned1D() {
    auto&& data = ConstructData[testCaseInstanceId()];
    setTestCaseDescription(data.name);
    setTestCaseTemplateName(StorageTraits<T>::name());

    /* Either this instance or the implicit one below gets used */
    DataLayer layer{layerHandle(0, 1)};

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    ui.setDataLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));

    T storageData[3];

    /* Flipped so we don't have a trivial stride */
    Containers::StridedArrayView1D<T> storageView =
        Containers::stridedArrayView(storageData).template flipped<0>();
    Containers::StridedArrayView1D<const T> constStorageView = storageView;

    NumericStorage<T> storage1 = data.implicitLayer ?
        NumericStorage<T>{ui, NonOwned, storageView, StorageFlags{0x10}} :
        NumericStorage<T>{layer, NonOwned, storageView, StorageFlags{0x10}};
    NumericStorage<T> storage2 = data.implicitLayer ?
        NumericStorage<T>{ui, NonOwned, constStorageView, StorageFlags{0x10}} :
        NumericStorage<T>{layer, NonOwned, constStorageView, StorageFlags{0x10}};
    CORRADE_COMPARE(&storage1.layer(), data.implicitLayer ? &ui.dataLayer() : &layer);
    CORRADE_COMPARE(&storage2.layer(), data.implicitLayer ? &ui.dataLayer() : &layer);
    /* Pointer + 1D stride is 16 bytes on 64-bit, so the storage fits in-place
       only if the remaining four members are at most 16 bytes. On 32-bit it's
       8 bytes, so the rest can occupy at most 12 bytes, which makes it 2-byte
       types only. */
    #ifndef CORRADE_TARGET_32BIT
    CORRADE_COMPARE(storage1.isAllocated(), sizeof(T) > 4);
    CORRADE_COMPARE(storage2.isAllocated(), sizeof(T) > 4);
    #else
    CORRADE_COMPARE(storage1.isAllocated(), sizeof(T) > 2);
    CORRADE_COMPARE(storage2.isAllocated(), sizeof(T) > 2);
    #endif
    CORRADE_VERIFY(!storage1.isDirty());
    CORRADE_VERIFY(!storage2.isDirty());
    CORRADE_COMPARE(storage1.flags(), StorageFlags{0x10});
    CORRADE_COMPARE(storage2.flags(), StorageFlags{0x10});
    CORRADE_COMPARE(storage1.size(), (Containers::Size3D{1, 1, 3}));
    CORRADE_COMPARE(storage2.size(), (Containers::Size3D{1, 1, 3}));
    /* The view is flipped so the first and second dimension becomes flipped
       too, times the original view size */
    CORRADE_COMPARE(storage1.stride(), (Containers::Stride3D{-3*std::ptrdiff_t{sizeof(T)}, -3*std::ptrdiff_t(sizeof(T)), -std::ptrdiff_t{sizeof(T)}}));
    CORRADE_COMPARE(storage2.stride(), (Containers::Stride3D{-3*std::ptrdiff_t{sizeof(T)}, -3*std::ptrdiff_t(sizeof(T)), -std::ptrdiff_t{sizeof(T)}}));
    CORRADE_COMPARE(storage1.range(), defaultRangeFor<T>());
    CORRADE_COMPARE(storage2.range(), defaultRangeFor<T>());
    CORRADE_COMPARE(storage1.step(), T(1.0));
    CORRADE_COMPARE(storage2.step(), T(1.0));

    /* Check just that the const variant doesn't have an updater set */
    StorageQuery<typename decltype(storage1)::Type> query1 = storage1[0];
    StorageQuery<typename decltype(storage2)::Type> query2 = storage2[0];
    CORRADE_COMPARE(query1.operations(), StorageOperation::Set|StorageOperation::Min|StorageOperation::Max|StorageOperation::Increment|StorageOperation::Decrement);
    CORRADE_COMPARE(query2.operations(), StorageOperation::Min|StorageOperation::Max);
    CORRADE_VERIFY(query1.updater());
    CORRADE_VERIFY(!query2.updater());

    /* The data should be the same views as passed to the constructor, expanded
       to 3D */
    Containers::StridedArrayView3D<const T> view1 = storage1.data();
    Containers::StridedArrayView3D<const T> view2 = storage2.data();
    CORRADE_COMPARE(view1.data(), storageView.data());
    CORRADE_COMPARE(view2.data(), constStorageView.data());
    CORRADE_COMPARE(view1.size(), Containers::StridedArrayView3D<T>{storageView}.size());
    CORRADE_COMPARE(view2.size(), Containers::StridedArrayView3D<const T>{constStorageView}.size());
    CORRADE_COMPARE(view1.stride(), Containers::StridedArrayView3D<T>{storageView}.stride());
    CORRADE_COMPARE(view2.stride(), Containers::StridedArrayView3D<const T>{constStorageView}.stride());
}

template<class T> void NumericStorageTest::constructNonOwned() {
    auto&& data = ConstructData[testCaseInstanceId()];
    setTestCaseDescription(data.name);
    setTestCaseTemplateName(StorageTraits<T>::name());

    /* Either this instance or the implicit one below gets used */
    DataLayer layer{layerHandle(0, 1)};

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    ui.setDataLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));

    T storageData;
    const T& constStorageData = storageData;

    NumericStorage<T> storage1 = data.implicitLayer ?
        NumericStorage<T>{ui, NonOwned, storageData, StorageFlags{0x08}} :
        NumericStorage<T>{layer, NonOwned, storageData, StorageFlags{0x08}};
    NumericStorage<T> storage2 = data.implicitLayer ?
        NumericStorage<T>{ui, NonOwned, constStorageData, StorageFlags{0x08}} :
        NumericStorage<T>{layer, NonOwned, constStorageData, StorageFlags{0x08}};
    CORRADE_COMPARE(&storage1.layer(), data.implicitLayer ? &ui.dataLayer() : &layer);
    CORRADE_COMPARE(&storage2.layer(), data.implicitLayer ? &ui.dataLayer() : &layer);
    /* Pointer + no stride is 8 bytes on 64-bit, so the storage fits in-place
       only if the remaining four members are at most 24 bytes -- so not 8-byte
       types anymore. On 32-bit the rest has to fit into 16 bytes, so also at
       most four-byte types. */
    CORRADE_COMPARE(storage1.isAllocated(), sizeof(T) > 4);
    CORRADE_COMPARE(storage2.isAllocated(), sizeof(T) > 4);
    CORRADE_VERIFY(!storage1.isDirty());
    CORRADE_VERIFY(!storage2.isDirty());
    CORRADE_COMPARE(storage1.flags(), StorageFlags{0x08});
    CORRADE_COMPARE(storage2.flags(), StorageFlags{0x08});
    CORRADE_COMPARE(storage1.size(), (Containers::Size3D{1, 1, 1}));
    CORRADE_COMPARE(storage2.size(), (Containers::Size3D{1, 1, 1}));
    CORRADE_COMPARE(storage1.stride(), (Containers::Stride3D{sizeof(T), sizeof(T), sizeof(T)}));
    CORRADE_COMPARE(storage2.stride(), (Containers::Stride3D{sizeof(T), sizeof(T), sizeof(T)}));
    CORRADE_COMPARE(storage1.range(), defaultRangeFor<T>());
    CORRADE_COMPARE(storage2.range(), defaultRangeFor<T>());
    CORRADE_COMPARE(storage1.step(), T(1.0));
    CORRADE_COMPARE(storage2.step(), T(1.0));

    /* Check just that the const variant doesn't have an updater set */
    StorageQuery<typename decltype(storage1)::Type> query1 = storage1.value();
    StorageQuery<typename decltype(storage2)::Type> query2 = storage2.value();
    CORRADE_COMPARE(query1.operations(), StorageOperation::Set|StorageOperation::Min|StorageOperation::Max|StorageOperation::Increment|StorageOperation::Decrement);
    CORRADE_COMPARE(query2.operations(), StorageOperation::Min|StorageOperation::Max);
    CORRADE_VERIFY(query1.updater());
    CORRADE_VERIFY(!query2.updater());

    /* The data should point to the value passed to the constructor, with
       trivial size and stride */
    Containers::StridedArrayView3D<const T> view1 = storage1.data();
    Containers::StridedArrayView3D<const T> view2 = storage2.data();
    CORRADE_COMPARE(view1.data(), &storageData);
    CORRADE_COMPARE(view2.data(), &constStorageData);
    CORRADE_COMPARE(view1.size(), (Containers::Size3D{1, 1, 1}));
    CORRADE_COMPARE(view2.size(), (Containers::Size3D{1, 1, 1}));
    CORRADE_COMPARE(view1.stride(), (Containers::Stride3D{sizeof(T), sizeof(T), sizeof(T)}));
    CORRADE_COMPARE(view2.stride(), (Containers::Stride3D{sizeof(T), sizeof(T), sizeof(T)}));
}

void NumericStorageTest::constructHandleRecycle() {
    DataLayer layer{layerHandle(0, 1)};

    /* Create one more layer to verify the recycling isn't handling just the
       first item correctly */
    NumericStorage<Long>{layer};

    /* Create a storage and update its properties, non-owned so also the
       internal flags are set. It should be stored in-place so we can verify
       it gets properly reinitialized next time. */
    const UnsignedShort data[3]{};
    NumericStorage<UnsignedShort> first{layer, NonOwned, data};
    first
        .setRange(13457, 58776)
        .setStep(5556);
    CORRADE_VERIFY(!first.isAllocated());
    CORRADE_COMPARE(first.range(), (Containers::Pair<UnsignedShort, UnsignedShort>{13457, 58776}));
    CORRADE_COMPARE(first.step(), 5556);
    /* The query should be immutable also */
    CORRADE_VERIFY(!first[0].updater());

    /* Remove and create a new storage in the same slot. All properties should
       be reset back to defaults. */
    layer.removeStorage(first);
    NumericStorage<UnsignedShort> second{layer};
    CORRADE_COMPARE(storageHandleId(second.handle()), storageHandleId(first.handle()));
    CORRADE_VERIFY(!second.isAllocated());
    CORRADE_COMPARE(second.range(), (Containers::Pair<UnsignedShort, UnsignedShort>{0, 65535}));
    CORRADE_COMPARE(second.step(), 1);
    /* The internal immutable flag should also be reset */
    CORRADE_VERIFY(second.value().updater());
}

void NumericStorageTest::access3D() {
    AbstractUserInterface ui{{100, 100}};
    DataLayer& layer = ui.setLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));

    /* Deliberately using prime numbers to hopefully catch any weird
       calculations. The storage shouldn't be dirty initially. */
    NumericStorage<UnsignedLong> storage{layer, NoInit, {5, 7, 3}};
    CORRADE_VERIFY(!storage.isDirty());

    /* Set the values first, verifying the updater implementation. The storage
       becomes dirty after the first set. */
    Int value = 0;
    for(std::size_t i = 0; i != storage.size()[0]; ++i) {
        for(std::size_t j = 0; j != storage.size()[1]; ++j) {
            for(std::size_t k = 0; k != storage.size()[2]; ++k) {
                storage[{i, j, k}].set(++value);
                CORRADE_VERIFY(storage.isDirty());
            }
        }
    }

    /* Getting the values back should result in what's expected, verifying the
       query implementation. Checking in a separate loop to catch also issues
       where set() would be overwriting previous values. */
    Int expected = 0;
    for(std::size_t i = 0; i != storage.size()[0]; ++i) {
        CORRADE_ITERATION(i);
        for(std::size_t j = 0; j != storage.size()[1]; ++j) {
            CORRADE_ITERATION(j);
            for(std::size_t k = 0; k != storage.size()[2]; ++k) {
                CORRADE_ITERATION(k);
                CORRADE_COMPARE((storage[{i, j, k}]), ++expected);
            }
        }
    }

    /* Verify that the indexing follows memory order, i.e. the contiguous view
       giving back a monotonic sequence */
    CORRADE_COMPARE_AS(storage.data().asContiguous(), Containers::arrayView<UnsignedLong>({
         1,  2,  3,  4,  5,  6,  7,  8,  9, 10,
        11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
        21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
        31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
        51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
        61, 62, 63, 64, 65, 66, 67, 68, 69, 70,
        71, 72, 73, 74, 75, 76, 77, 78, 79, 80,
        81, 82, 83, 84, 85, 86, 87, 88, 89, 90,
        91, 92, 93, 94, 95, 96, 97, 98, 99, 100,
        101, 102, 103, 104, 105
    }), TestSuite::Compare::Container);

    /* Verifying also the query API and that it works when passed to
       DataLayer::onUpdate() */
    StorageQuery<UnsignedLong> query = storage[{3, 4, 1}];
    CORRADE_COMPARE(&query.layer(), &layer);
    CORRADE_COMPARE(query.storage(), storage.handle());
    CORRADE_COMPARE(query.index(), (Containers::Size3D{3, 4, 1}));
    CORRADE_COMPARE(query.operations(), StorageOperation::Set|StorageOperation::Min|StorageOperation::Max|StorageOperation::Increment|StorageOperation::Decrement);
    CORRADE_VERIFY(query.updater());
    CORRADE_COMPARE(query, 77);

    Int called = 0;
    layer.onUpdate(query, [&called](UnsignedLong value) {
        CORRADE_COMPARE(value, 77);
        ++called;
    });
    CORRADE_COMPARE(called, 0);

    ui.update();
    CORRADE_VERIFY(!storage.isDirty());
    CORRADE_COMPARE(called, 1);
}

void NumericStorageTest::access2D() {
    /* A 2D subset of access3D(), basically */

    AbstractUserInterface ui{{100, 100}};
    DataLayer& layer = ui.setLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));

    NumericStorage<UnsignedShort> storage{layer, NoInit, {5, 7}};
    CORRADE_VERIFY(!storage.isDirty());

    Short value = 0;
    for(std::size_t i = 0; i != storage.size()[1]; ++i) {
        for(std::size_t j = 0; j != storage.size()[2]; ++j) {
            storage[{i, j}].set(++value);
            CORRADE_VERIFY(storage.isDirty());
        }
    }

    Int expected = 0;
    for(std::size_t i = 0; i != storage.size()[1]; ++i) {
        CORRADE_ITERATION(i);
        for(std::size_t j = 0; j != storage.size()[2]; ++j) {
            CORRADE_ITERATION(j);
            CORRADE_COMPARE((storage[{i, j}]), ++expected);
        }
    }

    CORRADE_COMPARE_AS(storage.data().asContiguous(), Containers::arrayView<UnsignedShort>({
         1,  2,  3,  4,  5,  6,  7,  8,  9, 10,
        11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
        21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
        31, 32, 33, 34, 35
    }), TestSuite::Compare::Container);

    StorageQuery<UnsignedInt> query = storage[{3, 4}];
    CORRADE_COMPARE(&query.layer(), &layer);
    CORRADE_COMPARE(query.storage(), storage.handle());
    CORRADE_COMPARE(query.index(), (Containers::Size3D{0, 3, 4}));
    CORRADE_COMPARE(query.operations(), StorageOperation::Set|StorageOperation::Min|StorageOperation::Max|StorageOperation::Increment|StorageOperation::Decrement);
    CORRADE_VERIFY(query.updater());
    CORRADE_COMPARE(query, 26);

    Int called = 0;
    layer.onUpdate(query, [&called](UnsignedInt value) {
        CORRADE_COMPARE(value, 26);
        ++called;
    });
    CORRADE_COMPARE(called, 0);

    ui.update();
    CORRADE_VERIFY(!storage.isDirty());
    CORRADE_COMPARE(called, 1);
}

void NumericStorageTest::access1D() {
    /* A 1D subset of access3D(), basically */

    AbstractUserInterface ui{{100, 100}};
    DataLayer& layer = ui.setLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));

    NumericStorage<UnsignedByte> storage{layer, NoInit, 7};
    CORRADE_VERIFY(!storage.isDirty());

    Int value = 0;
    for(std::size_t i = 0; i != storage.size()[2]; ++i) {
        storage[i].set(++value);
        CORRADE_VERIFY(storage.isDirty());
    }

    Int expected = 0;
    for(std::size_t i = 0; i != storage.size()[2]; ++i) {
        CORRADE_ITERATION(i);
        CORRADE_COMPARE(storage[i], ++expected);
    }

    CORRADE_COMPARE_AS(storage.data().asContiguous(), Containers::arrayView<UnsignedByte>({
         1,  2,  3,  4,  5,  6,  7,
    }), TestSuite::Compare::Container);

    StorageQuery<UnsignedInt> query = storage[4];
    CORRADE_COMPARE(&query.layer(), &layer);
    CORRADE_COMPARE(query.storage(), storage.handle());
    CORRADE_COMPARE(query.index(), (Containers::Size3D{0, 0, 4}));
    CORRADE_COMPARE(query.operations(), StorageOperation::Set|StorageOperation::Min|StorageOperation::Max|StorageOperation::Increment|StorageOperation::Decrement);
    CORRADE_VERIFY(query.updater());
    CORRADE_COMPARE(query, 5);

    Int called = 0;
    layer.onUpdate(query, [&called](UnsignedInt value) {
        CORRADE_COMPARE(value, 5);
        ++called;
    });
    CORRADE_COMPARE(called, 0);

    ui.update();
    CORRADE_VERIFY(!storage.isDirty());
    CORRADE_COMPARE(called, 1);
}

void NumericStorageTest::access() {
    /* A single-item subset of access3D(), basically */

    AbstractUserInterface ui{{100, 100}};
    DataLayer& layer = ui.setLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));

    NumericStorage<UnsignedInt> storage{layer, NoInit};
    CORRADE_VERIFY(!storage.isDirty());

    storage.value().set(1337);
    CORRADE_VERIFY(storage.isDirty());
    CORRADE_COMPARE(storage.value(), 1337);

    CORRADE_COMPARE_AS(storage.data().asContiguous(), Containers::arrayView<UnsignedInt>({
        1337
    }), TestSuite::Compare::Container);

    /* Verifying both the explicit and implicit query */
    StorageQuery<UnsignedInt> query1 = storage.value();
    StorageQuery<UnsignedInt> query2 = storage;
    CORRADE_COMPARE(&query1.layer(), &layer);
    CORRADE_COMPARE(&query2.layer(), &layer);
    CORRADE_COMPARE(query1.storage(), storage.handle());
    CORRADE_COMPARE(query2.storage(), storage.handle());
    CORRADE_COMPARE(query1.index(), (Containers::Size3D{0, 0, 0}));
    CORRADE_COMPARE(query2.index(), (Containers::Size3D{0, 0, 0}));
    CORRADE_COMPARE(query1.operations(), StorageOperation::Set|StorageOperation::Min|StorageOperation::Max|StorageOperation::Increment|StorageOperation::Decrement);
    CORRADE_COMPARE(query2.operations(), StorageOperation::Set|StorageOperation::Min|StorageOperation::Max|StorageOperation::Increment|StorageOperation::Decrement);
    CORRADE_VERIFY(query1.updater());
    CORRADE_VERIFY(query2.updater());
    CORRADE_COMPARE(query1, 1337);
    CORRADE_COMPARE(query2, 1337);

    /* Verifying onUpdate() with both the explicit and implicit query */
    Int called1 = 0;
    Int called2 = 0;
    layer.onUpdate(query1, [&called1](UnsignedInt value) {
        CORRADE_COMPARE(value, 1337);
        ++called1;
    });
    layer.onUpdate(storage, [&called2](UnsignedInt value) {
        CORRADE_COMPARE(value, 1337);
        ++called2;
    });
    CORRADE_COMPARE(called1, 0);
    CORRADE_COMPARE(called2, 0);

    ui.update();
    CORRADE_VERIFY(!storage.isDirty());
    CORRADE_COMPARE(called1, 1);
    CORRADE_COMPARE(called2, 1);
}

void NumericStorageTest::accessNonOwned3D() {
    auto&& data = AccessNonOwned3DData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Matches StorageTest::access3D() */

    AbstractUserInterface ui{{100, 100}};
    DataLayer& layer = ui.setLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));

    Int storageData[]{
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
        16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31
    };
    storageData[data.valueIndex] = -17654321;
    Containers::StridedArrayView3D<Int> view{storageData, storageData + data.offset, {2, 3, 2}, data.stride};
    CORRADE_COMPARE((view[{1, 2, 1}]), -17654321);

    /* Verifying also the query API and that it works when passed to
       DataLayer::onUpdate() */
    NumericStorage<Int> storage = data.immutable ?
        NumericStorage<Int>{layer, NonOwned, Containers::StridedArrayView3D<const Int>{view}} :
        NumericStorage<Int>{layer, NonOwned, view};
    StorageQuery<Int> query = storage[{1, 2, 1}];
    CORRADE_COMPARE(&query.layer(), &layer);
    CORRADE_COMPARE(query.storage(), storage.handle());
    CORRADE_COMPARE(query.index(), (Containers::Size3D{1, 2, 1}));
    CORRADE_COMPARE(query.operations(), data.immutable ?
        StorageOperation::Min|StorageOperation::Max :
        StorageOperation::Set|StorageOperation::Min|StorageOperation::Max|StorageOperation::Increment|StorageOperation::Decrement);
    CORRADE_COMPARE(query.updater(), !data.immutable);
    CORRADE_COMPARE(query, -17654321);

    Int called = 0;
    layer.onUpdate(query, [&called](Int value) {
        CORRADE_COMPARE(value, -17654321);
        ++called;
    });
    CORRADE_COMPARE(called, 0);

    /* The callback gets called on the first update */
    ui.update();
    CORRADE_VERIFY(!storage.isDirty());
    CORRADE_COMPARE(called, 1);

    /* Tf the storage is mutable test also that updates go back to the same
       index. All update operations are thoroughly verified in update() below
       with an assumption that they all share the same index calculation
       code as is tested here. */
    if(!data.immutable) {
        query.set(97253482);
        CORRADE_VERIFY(storage.isDirty());
        CORRADE_COMPARE(query, 97253482);
        CORRADE_COMPARE(storageData[data.valueIndex], 97253482);
    }
}

void NumericStorageTest::accessNonOwned2D() {
    auto&& data = AccessNonOwned2DData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* A 2D subset of accessNonOwned3D(), basically. Matches
       StorageTest::access2D(). */

    AbstractUserInterface ui{{100, 100}};
    DataLayer& layer = ui.setLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));

    Short storageData[]{
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13
    };
    storageData[data.valueIndex] = -17654;
    Containers::StridedArrayView2D<Short> view{storageData, storageData + data.offset, {2, 3}, data.stride};
    CORRADE_COMPARE((view[{1, 2}]), -17654);

    NumericStorage<Short> storage = data.immutable ?
        NumericStorage<Short>{layer, NonOwned, Containers::StridedArrayView3D<const Short>{view}} :
        NumericStorage<Short>{layer, NonOwned, view};
    StorageQuery<Int> query = storage[{1, 2}];
    CORRADE_COMPARE(&query.layer(), &layer);
    CORRADE_COMPARE(query.storage(), storage.handle());
    CORRADE_COMPARE(query.index(), (Containers::Size3D{0, 1, 2}));
    CORRADE_COMPARE(query.operations(), data.immutable ?
        StorageOperation::Min|StorageOperation::Max :
        StorageOperation::Set|StorageOperation::Min|StorageOperation::Max|StorageOperation::Increment|StorageOperation::Decrement);
    CORRADE_COMPARE(query.updater(), !data.immutable);
    CORRADE_COMPARE(query, -17654);

    Int called = 0;
    layer.onUpdate(query, [&called](Int value) {
        CORRADE_COMPARE(value, -17654);
        ++called;
    });
    CORRADE_COMPARE(called, 0);

    ui.update();
    CORRADE_VERIFY(!storage.isDirty());
    CORRADE_COMPARE(called, 1);

    if(!data.immutable) {
        query.set(9725);
        CORRADE_VERIFY(storage.isDirty());
        CORRADE_COMPARE(query, 9725);
        CORRADE_COMPARE(storageData[data.valueIndex], 9725);
    }
}

void NumericStorageTest::accessNonOwned1D() {
    auto&& data = AccessNonOwned1DData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* A 1D subset of accessNonOwned3D(), basically. Matches
       StorageTest::access1D(). */

    AbstractUserInterface ui{{100, 100}};
    DataLayer& layer = ui.setLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));

    Byte storageData[]{
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14
    };
    storageData[data.valueIndex] = -114;
    Containers::StridedArrayView1D<Byte> view{storageData, storageData + data.offset, 5, data.stride};
    CORRADE_COMPARE(view[4], -114);

    NumericStorage<Byte> storage = data.immutable ?
        NumericStorage<Byte>{layer, NonOwned, Containers::StridedArrayView3D<const Byte>{view}} :
        NumericStorage<Byte>{layer, NonOwned, view};
    StorageQuery<Int> query = storage[4];
    CORRADE_COMPARE(&query.layer(), &layer);
    CORRADE_COMPARE(query.storage(), storage.handle());
    CORRADE_COMPARE(query.index(), (Containers::Size3D{0, 0, 4}));
    CORRADE_COMPARE(query.operations(), data.immutable ?
        StorageOperation::Min|StorageOperation::Max :
        StorageOperation::Set|StorageOperation::Min|StorageOperation::Max|StorageOperation::Increment|StorageOperation::Decrement);
    CORRADE_COMPARE(query.updater(), !data.immutable);
    CORRADE_COMPARE(query, -114);

    Int called = 0;
    layer.onUpdate(query, [&called](Int value) {
        CORRADE_COMPARE(value, -114);
        ++called;
    });
    CORRADE_COMPARE(called, 0);

    ui.update();
    CORRADE_VERIFY(!storage.isDirty());
    CORRADE_COMPARE(called, 1);

    if(!data.immutable) {
        query.set(97);
        CORRADE_VERIFY(storage.isDirty());
        CORRADE_COMPARE(query, 97);
        CORRADE_COMPARE(storageData[data.valueIndex], 97);
    }
}

void NumericStorageTest::accessNonOwned() {
    auto&& data = AccessNonOwnedData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* A single-item subset of accessNonOwned3D(), basically. Matches
       StorageTest::access(). */

    AbstractUserInterface ui{{100, 100}};
    DataLayer& layer = ui.setLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));

    /* Verifying both the explicit and implicit query */
    Long storageData = -9876543210ll;
    NumericStorage<Long> storage = data.immutable ?
        NumericStorage<Long>{layer, NonOwned, static_cast<const Long&>(storageData)} :
        NumericStorage<Long>{layer, NonOwned, storageData};
    StorageQuery<Long> query1 = storage.value();
    StorageQuery<Long> query2 = storage;
    CORRADE_COMPARE(&query1.layer(), &layer);
    CORRADE_COMPARE(&query2.layer(), &layer);
    CORRADE_COMPARE(query1.storage(), storage.handle());
    CORRADE_COMPARE(query2.storage(), storage.handle());
    CORRADE_COMPARE(query1.index(), (Containers::Size3D{0, 0, 0}));
    CORRADE_COMPARE(query2.index(), (Containers::Size3D{0, 0, 0}));
    CORRADE_COMPARE(query1.operations(), data.immutable ?
        StorageOperation::Min|StorageOperation::Max :
        StorageOperation::Set|StorageOperation::Min|StorageOperation::Max|StorageOperation::Increment|StorageOperation::Decrement);
    CORRADE_COMPARE(query2.operations(), data.immutable ?
        StorageOperation::Min|StorageOperation::Max :
        StorageOperation::Set|StorageOperation::Min|StorageOperation::Max|StorageOperation::Increment|StorageOperation::Decrement);
    CORRADE_COMPARE(query1.updater(), !data.immutable);
    CORRADE_COMPARE(query2.updater(), !data.immutable);
    CORRADE_COMPARE(query1, -9876543210ll);
    CORRADE_COMPARE(query2, -9876543210ll);

    /* Verifying onUpdate() with both the explicit and implicit query */
    Int called1 = 0, called2 = 0;
    layer.onUpdate(query1, [&called1](Long value) {
        CORRADE_COMPARE(value, -9876543210);
        ++called1;
    });
    layer.onUpdate(storage, [&called2](Long value) {
        CORRADE_COMPARE(value, -9876543210);
        ++called2;
    });
    CORRADE_COMPARE(called1, 0);
    CORRADE_COMPARE(called2, 0);

    ui.update();
    CORRADE_VERIFY(!storage.isDirty());
    CORRADE_COMPARE(called1, 1);
    CORRADE_COMPARE(called2, 1);

    if(!data.immutable) {
        query1.set(42069);
        CORRADE_VERIFY(storage.isDirty());
        CORRADE_COMPARE(query1, 42069);
        CORRADE_COMPARE(storageData, 42069);

        query2.set(69420);
        CORRADE_VERIFY(storage.isDirty());
        CORRADE_COMPARE(query2, 69420);
        CORRADE_COMPARE(storageData, 69420);
    }
}

void NumericStorageTest::accessInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    /* All these assertions are fired by StorageQuery, this is just verifying
       that they actually get fired without something else happening first or
       them being sidestepped for example by delegating all APIs to the 3D
       queries. */

    DataLayer layer{layerHandle(0, 1)};
    NumericStorage<Int> storage1D{layer, 15};
    NumericStorage<Int> storage2D{layer, {3, 7}};
    NumericStorage<Int> storage3D{layer, {4, 2, 5}};

    Containers::String out;
    Error redirectError{&out};
    /* Index out of bounds. The assertion should be fired by StorageQuery, so
       just verify that we're not attempting to access something else first. */
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

void NumericStorageTest::update() {
    auto&& data = UpdateData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* This verifies that all update operations properly touch the stored or
       referenced value at given index along with setting dirty bits. Behavior
       with a custom range, step, clamping etc. is tested in rangeStep()
       below. */

    AbstractUserInterface ui{{100, 100}};
    DataLayer& layer = ui.setLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));

    UnsignedByte storageData = 176;
    NumericStorage<UnsignedByte> storage = data.nonOwned ?
        NumericStorage<UnsignedByte>{layer, NonOwned, storageData} :
        NumericStorage<UnsignedByte>{layer, DirectInit, 176};
    /* The query is always at least a 32-bit type */
    StorageQuery<UnsignedInt> query = storage.value();

    /* Attach an update to min and max as well to verify it's being passed
       correctly on updates */
    struct {
        Int called = 0;
        UnsignedInt expected;
    } state;
    DataHandle update = layer.onUpdate(query, [&state](UnsignedInt value) {
        CORRADE_COMPARE(value, state.expected);
        ++state.called;
    });
    CORRADE_VERIFY(!layer.isStorageDirty(storage));
    CORRADE_VERIFY(layer.isDirty(update));

    /* The callback gets called on the first update */
    state.expected = 176;
    {
        CORRADE_ITERATION(__FILE__ ":" CORRADE_LINE_STRING);
        ui.update();
    }
    CORRADE_VERIFY(!layer.isStorageDirty(storage));
    CORRADE_VERIFY(!layer.isDirty(update));
    CORRADE_COMPARE(state.called, 1);

    /* Changing the value marks the storage as dirty */
    query.set(97);
    CORRADE_VERIFY(layer.isStorageDirty(storage));
    CORRADE_VERIFY(!layer.isDirty(update));

    /* Update calls and resets */
    state.expected = 97;
    {
        CORRADE_ITERATION(__FILE__ ":" CORRADE_LINE_STRING);
        ui.update();
    }
    CORRADE_VERIFY(!layer.isStorageDirty(storage));
    CORRADE_VERIFY(!layer.isDirty(update));
    CORRADE_COMPARE(state.called, 2);

    /* Incrementing marks the storage as dirty */
    query.increment();
    CORRADE_VERIFY(layer.isStorageDirty(storage));
    CORRADE_VERIFY(!layer.isDirty(update));

    /* Update calls and resets */
    state.expected = 98;
    {
        CORRADE_ITERATION(__FILE__ ":" CORRADE_LINE_STRING);
        ui.update();
    }
    CORRADE_VERIFY(!layer.isStorageDirty(storage));
    CORRADE_VERIFY(!layer.isDirty(update));
    CORRADE_COMPARE(state.called, 3);

    /* Decrementing marks the storage as dirty */
    query.decrement();
    CORRADE_VERIFY(layer.isStorageDirty(storage));
    CORRADE_VERIFY(!layer.isDirty(update));

    /* Update calls and resets */
    state.expected = 97;
    {
        CORRADE_ITERATION(__FILE__ ":" CORRADE_LINE_STRING);
        ui.update();
    }
    CORRADE_VERIFY(!layer.isStorageDirty(storage));
    CORRADE_VERIFY(!layer.isDirty(update));
    CORRADE_COMPARE(state.called, 4);

    /* Setting to min marks the storage as dirty */
    query.setToMin();
    CORRADE_VERIFY(layer.isStorageDirty(storage));
    CORRADE_VERIFY(!layer.isDirty(update));

    /* Update calls and resets */
    state.expected = 0;
    {
        CORRADE_ITERATION(__FILE__ ":" CORRADE_LINE_STRING);
        ui.update();
    }
    CORRADE_VERIFY(!layer.isStorageDirty(storage));
    CORRADE_VERIFY(!layer.isDirty(update));
    CORRADE_COMPARE(state.called, 5);

    /* Setting to max marks the storage as dirty */
    query.setToMax();
    CORRADE_VERIFY(layer.isStorageDirty(storage));
    CORRADE_VERIFY(!layer.isDirty(update));

    /* Update calls and resets */
    state.expected = 255;
    {
        CORRADE_ITERATION(__FILE__ ":" CORRADE_LINE_STRING);
        ui.update();
    }
    CORRADE_VERIFY(!layer.isStorageDirty(storage));
    CORRADE_VERIFY(!layer.isDirty(update));
    CORRADE_COMPARE(state.called, 6);
}

void NumericStorageTest::rangeStep() {
    auto&& data = RangeStepData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Compared to update(), which verified that the right value was updated
       depending on whether the storage is owned or not, this verifies various
       clamping behavior, assuming the implementation isn't differing in the
       non-owned variant. */

    AbstractUserInterface ui{{100, 100}};
    DataLayer& layer = ui.setLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));

    NumericStorage<Short> storage{layer, NoInit, {2, 5, 3}};
    StorageQuery<Int> query = storage[{1, 3, 2}];
    query.set(data.value);

    struct {
        Int called = 0;
        Int expected;
        Int expectedMin = -32768;
        Int expectedMax = 32767;
    } state;
    DataHandle update = layer.onUpdate(query, [&state](Int value, Int min, Int max) {
        CORRADE_COMPARE(value, state.expected);
        CORRADE_COMPARE(min, state.expectedMin);
        CORRADE_COMPARE(max, state.expectedMax);
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

    /* Set a custom range if desired. It should mark the storage as dirty but
       the actual value shouldn't change. */
    if(data.range) {
        storage.setRange(data.range->first(), data.range->second());
        CORRADE_COMPARE(storage.range(), *data.range);
        /* The actual value shouldn't change from before even though it may now
           be outside of the range */
        CORRADE_COMPARE(query, data.value);
        /* The storage should be marked as dirty only if the range actually
           changes */
        bool rangeChanged = data.range != Containers::Pair<Short, Short>{-32768, 32767};
        CORRADE_COMPARE(layer.isStorageDirty(storage), rangeChanged);
        CORRADE_VERIFY(!layer.isDirty(update));

        /* Reset the dirty state. The actual value shouldn't change or get
           clamped, only the min/max. */
        state.expectedMin = data.range->first();
        state.expectedMax = data.range->second();
        {
            CORRADE_ITERATION(__FILE__ ":" CORRADE_LINE_STRING);
            ui.update();
        }
        CORRADE_VERIFY(!layer.isStorageDirty(storage));
        CORRADE_VERIFY(!layer.isDirty(update));
        CORRADE_COMPARE(state.called, rangeChanged ? 2 : 1);
    }

    /* Set a custom step if desired. It should *not* mark the storage as
       dirty, and also shouldn't change the value in any way. */
    if(data.step) {
        storage.setStep(*data.step);
        CORRADE_COMPARE(storage.step(), *data.step);
        CORRADE_COMPARE(query, data.value);
        CORRADE_VERIFY(!layer.isStorageDirty(storage));
        CORRADE_VERIFY(!layer.isDirty(update));
    }

    /* Perform the desired update operation. This should mark the storage as
       dirty only if the value actually changes. */
    if(data.operation == StorageOperation::Set) {
        CORRADE_COMPARE(query.set(*data.set), data.expectedState);
    } else {
        CORRADE_INTERNAL_ASSERT(!data.set);
        if(data.operation == StorageOperation::Increment)
            query.increment();
        else if(data.operation == StorageOperation::Decrement)
            query.decrement();
        else if(data.operation == StorageOperation::Min)
            query.setToMin();
        else if(data.operation == StorageOperation::Max)
            query.setToMax();
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

template<class> struct RangeStepTypeOverflowTraits;
template<> struct RangeStepTypeOverflowTraits<Int> {
                               /* 12345678 */
    static Int value() { return 0x70000000; }
    static Int step() {  return 0x60000000; }
};
template<> struct RangeStepTypeOverflowTraits<Long> {
                                /* 1234567890abcdef */
    static Long value() { return 0x7000000000000000; }
    static Long step() {  return 0x6000000000000000; }
};

template<class T> void NumericStorageTest::rangeStepTypeOverflowSigned() {
    setTestCaseTemplateName(StorageTraits<T>::name());

    /* Tests the case where `currentValue + step` or `currentValue - step`
       over- or underflows the data type and as such should be properly clamped
       to the correct edge instead of treating the wrapped-around value as
       correct. As the queries don't operate on 8- and 16-bit types, only need
       to verify with 32 and 64-bit ints. */

    DataLayer layer{layerHandle(0, 1)};
    NumericStorage<T> storage{layer};
    StorageQuery<T> query = storage;

    /* Shortcuts so the repeated use below isn't too verbose */
    T value = RangeStepTypeOverflowTraits<T>::value();
    T step = RangeStepTypeOverflowTraits<T>::step();
    T back = std::numeric_limits<T>::max() - step;

    CORRADE_COMPARE(query.set(value), StorageUpdateState::Success);
    CORRADE_COMPARE(query, value);

    /* Increment should result in clamping to max */
    storage.setStep(step);
    query.increment();
    CORRADE_COMPARE(query, std::numeric_limits<T>::max());

    /* Decrementing the clamped value back should fit easily without clamping */
    query.decrement();
    CORRADE_COMPARE(query, back);

    /* Negative value */
    CORRADE_COMPARE(query.set(-value), StorageUpdateState::Success);
    CORRADE_COMPARE(query, -value);

    /* Decrement should result in clamping to min */
    query.decrement();
    CORRADE_COMPARE(query, std::numeric_limits<T>::min());

    /* Incrementing the clamped value back should fit easily without clamping */
    query.increment();
    CORRADE_COMPARE(query, -back - 1);

    /* Positive value again */
    CORRADE_COMPARE(query.set(value), StorageUpdateState::Success);
    CORRADE_COMPARE(query, value);

    /* Decrement with a negative step should result in clamping to max */
    storage.setStep(-step);
    query.decrement();
    CORRADE_COMPARE(query, std::numeric_limits<T>::max());

    /* Negatively incrementing the clamped value should fit again */
    query.increment();
    CORRADE_COMPARE(query, back);

    /* Negative value again */
    CORRADE_COMPARE(query.set(-value), StorageUpdateState::Success);
    CORRADE_COMPARE(query, -value);

    /* Increment with a negative step should result in clamping to min */
    query.increment();
    CORRADE_COMPARE(query, std::numeric_limits<T>::min());

    /* Negatively decrementing the clamped value should fit again */
    query.decrement();
    CORRADE_COMPARE(query, -back - 1);
}

template<> struct RangeStepTypeOverflowTraits<UnsignedInt> {
                                       /* 12345678 */
    static UnsignedInt high() { return 0xf0000000u; }
    static UnsignedInt low() {  return 0x10000000u; }
    static UnsignedInt step() { return 0x90000000u; }
};
template<> struct RangeStepTypeOverflowTraits<UnsignedLong> {
                                        /* 1234567890abcdef */
    static UnsignedLong high() { return 0xf000000000000000ull; }
    static UnsignedLong low() {  return 0x1000000000000000ull; }
    static UnsignedLong step() { return 0x9000000000000000ull; }
};

template<class T> void NumericStorageTest::rangeStepTypeOverflowUnsigned() {
    setTestCaseTemplateName(StorageTraits<T>::name());

    /* Like rangeStepTypeOverflowSigned() but testing unsigned types, so
       without going negative part */

    DataLayer layer{layerHandle(0, 1)};
    NumericStorage<T> storage{layer};
    StorageQuery<T> query = storage;

    /* Shortcuts so the repeated use below isn't too verbose */
    T high = RangeStepTypeOverflowTraits<T>::high();
    T low = RangeStepTypeOverflowTraits<T>::low();
    T step = RangeStepTypeOverflowTraits<T>::step();
    T back = std::numeric_limits<T>::max() - step;

    /* Set a high value */
    CORRADE_COMPARE(query.set(high), StorageUpdateState::Success);
    CORRADE_COMPARE(query, high);

    /* Increment with a large step should result in clamping to max */
    storage.setStep(step);
    query.increment();
    CORRADE_COMPARE(query, std::numeric_limits<T>::max());

    /* Decrementing the clamped value back should fit easily without clamping */
    query.decrement();
    CORRADE_COMPARE(query, back);

    /* Set a low value */
    CORRADE_COMPARE(query.set(low), StorageUpdateState::Success);
    CORRADE_COMPARE(query, low);

    /* Decrement should result in clamping to min */
    query.decrement();
    CORRADE_COMPARE(query, std::numeric_limits<T>::min());

    /* Incrementing the clamped value back should fit easily without clamping,
       resulting in the step value alone */
    query.increment();
    CORRADE_COMPARE(query, step);
}

void NumericStorageTest::rangeStepTypeOverflowFloat() {
    /* Verifies that the generic code for handling integer overflows doesn't
       break behavior for floats, where none of this is needed and the floating
       point type handles this directly */

    DataLayer layer{layerHandle(0, 1)};
    NumericStorage<Float> storage{layer};
    StorageQuery<Float> query = storage;

    /* To not need to update the constant several times */
    Float value = 2e38f;
    Float step = 3e38f;

    /* Set a large value */
    CORRADE_COMPARE(query.set(value), StorageUpdateState::Success);
    CORRADE_COMPARE(query, value);

    /* Increment with a large step should result in clamping to +inf */
    storage.setStep(step);
    query.increment();
    CORRADE_COMPARE(query, Constants::inf());

    /* Decrement stays there */
    query.decrement();
    CORRADE_COMPARE(query, Constants::inf());

    /* Set a large negative value */
    CORRADE_COMPARE(query.set(-value), StorageUpdateState::Success);
    CORRADE_COMPARE(query, -value);

    /* Decrement should result in -inf */
    query.decrement();
    CORRADE_COMPARE(query, -Constants::inf());

    /* Increment stays there */
    query.increment();
    CORRADE_COMPARE(query, -Constants::inf());

    /* Set a large negative value again */
    CORRADE_COMPARE(query.set(-value), StorageUpdateState::Success);
    CORRADE_COMPARE(query, -value);

    /* Increment with a large negative step should result in -inf */
    storage.setStep(-step);
    query.increment();
    CORRADE_COMPARE(query, -Constants::inf());

    /* Decrement stays there */
    query.decrement();
    CORRADE_COMPARE(query, -Constants::inf());

    /* Set a large positive value again */
    CORRADE_COMPARE(query.set(value), StorageUpdateState::Success);
    CORRADE_COMPARE(query, value);

    /* Decrement with a large negative step should result in +inf */
    query.decrement();
    CORRADE_COMPARE(query, Constants::inf());

    /* Increment stays there */
    query.increment();
    CORRADE_COMPARE(query, Constants::inf());
}

void NumericStorageTest::rangeStepInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    DataLayer layer{layerHandle(0, 1)};
    NumericStorage<Int> storage{layer};
    NumericStorage<Float> storageFloat{layer};
    NumericStorage<Half> storageHalf{layer};

    /* Setting an empty range is fine, even with infinities */
    storage.setRange(34, 34);
    storageFloat.setRange(Constants::inf(), Constants::inf());
    storageFloat.setRange(-Constants::inf(), -Constants::inf());

    /* Negative step is fine also */
    storage.setStep(-1);

    Containers::String out;
    Error redirectError{&out};
    storage.setRange(34, 33);
    storageFloat.setRange(Constants::nan(), 3.0f);
    storageFloat.setRange(3.0f, Constants::nan());
    storageFloat.setRange(Constants::nan(), Constants::nan());
    storage.setStep(0);
    storageFloat.setStep(0.0f);
    storageFloat.setStep(-0.0f); /* Negative zero should be invalid too */
    storageHalf.setStep(-0.0_h); /* It should blow up for half-floats too */
    const char* expected =
        "Ui::NumericStorage::setRange(): expected min to not be larger than max but got 34 and 33\n"
        /* MSVC before version 2019 16.10(11?) prints -nan(ind) instead of
           ±nan. Check matching DebugTools::CompareImage tests. */
        #if defined(CORRADE_TARGET_MSVC) && _MSC_VER < 1929 && !defined(CORRADE_TARGET_CLANG_CL)
        "Ui::NumericStorage::setRange(): expected min to not be larger than max but got -nan(ind) and 3\n"
        "Ui::NumericStorage::setRange(): expected min to not be larger than max but got 3 and -nan(ind)\n"
        "Ui::NumericStorage::setRange(): expected min to not be larger than max but got -nan(ind) and -nan(ind)\n"
        #else
        "Ui::NumericStorage::setRange(): expected min to not be larger than max but got nan and 3\n"
        "Ui::NumericStorage::setRange(): expected min to not be larger than max but got 3 and nan\n"
        "Ui::NumericStorage::setRange(): expected min to not be larger than max but got nan and nan\n"
        #endif
        "Ui::NumericStorage::setStep(): expected a non-zero step\n"
        "Ui::NumericStorage::setStep(): expected a non-zero step\n"
        "Ui::NumericStorage::setStep(): expected a non-zero step\n"
        "Ui::NumericStorage::setStep(): expected a non-zero step\n";
    CORRADE_COMPARE_AS(out, expected, TestSuite::Compare::String);
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::NumericStorageTest)
