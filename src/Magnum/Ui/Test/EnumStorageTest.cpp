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

#include <Corrade/Containers/EnumSet.hpp>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/String.h>
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/Container.h>
#include <Corrade/TestSuite/Compare/String.h>
#include <Magnum/Math/Vector2.h> /* AbstractUserInterface constructor size */

#include "Magnum/Ui/EnumStorage.h"
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/UserInterface.h"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct EnumStorageTest: TestSuite::Tester {
    explicit EnumStorageTest();

    template<class T> void constructValueInit();
    template<class T> void constructNoInit();
    template<class T> void constructDirectInit();
    template<class T> void constructNonOwned1D();
    template<class T> void constructNonOwned();
    template<class T> void constructTypeProperties();
    void constructCopy();

    void constructHandleRecycle();

    /* Verifies that both query and set accesses the right data index in all
       dimensions */
    void access1D();
    void access1DSingleValue();
    void access();
    void accessSingleValue();

    /* Verifies that both query and updater accesses the right data index in
       all dimensions and stride variants, as well as immutable behavior */
    void accessNonOwned1D();
    void accessNonOwned1DSingleValue();
    void accessNonOwned();
    void accessNonOwnedSingleValue();

    void accessInvalid();
    void nonOwnedMutableDataInvalid();

    /* Verifies that updates to a value all do the right thing including
       setting a dirty bit, both in owned and non-owned storages */
    void update();
    void updateSingleValue();

    /* Verifies behavior of setEnumSet(), setDefault() and their effect on
       value updates at an arbitrary index */
    void enumSetDefault();
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

enum class Small: std::uint8_t {
    Value = 1 << 7
};
Debug& operator<<(Debug& debug, Small value) {
    return debug << std::uint8_t(value);
}

enum class Medium: std::uint32_t {
    Value = 1ull << 31
};
Debug& operator<<(Debug& debug, Medium value) {
    return debug << std::uint32_t(value);
}

enum class Large: std::uint64_t {
    Value = 1ull << 63
};
Debug& operator<<(Debug& debug, Large value) {
    return debug << std::uint64_t(value);
}

enum NonScopedEnum {};

enum class Enum: std::uint16_t {
    /* For use with sequential (enum) storage */
    Zero = 0,
    Two = 2,
    Three = 3,
    Four = 4,
    Five = 5,
    /* For use with mask (enum set) storage */
    ZerothBit = 1 << 0,
    SeventhBit = 1 << 7,
    EleventhBit = 1 << 11,
    SeventhEleventhBit = SeventhBit|EleventhBit,
};
Debug& operator<<(Debug& debug, Enum value) {
    switch(value) {
        #define _c(value) case Enum::value: return debug << #value;
        _c(Zero)
        _c(Two)
        _c(Three)
        _c(Four)
        _c(Five)
        _c(ZerothBit)
        _c(SeventhBit)
        _c(EleventhBit)
        _c(SeventhEleventhBit)
        #undef _c
    }
    return debug << Debug::hex << std::uint16_t(value);
}

typedef Containers::EnumSet<Enum> EnumSet;
#ifdef CORRADE_TARGET_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#endif
CORRADE_ENUMSET_OPERATORS(EnumSet)
#ifdef CORRADE_TARGET_CLANG
#pragma clang diagnostic pop
#endif
Debug& operator<<(Debug& debug, EnumSet value) {
    return Containers::enumSetDebugOutput(debug, value, "{}", {
        Enum::ZerothBit,
        Enum::SeventhBit,
        Enum::EleventhBit,
    });
}

const struct {
    TestSuite::TestCaseDescriptionSourceLocation name;

    EnumSet value;
    Containers::Optional<EnumSet> defaultValue;
    bool enumSet;
    Containers::Optional<Enum> enumSetValue;

    StorageOperation operation;
    Containers::Optional<EnumSet> set;
    Containers::Optional<bool> setEnumSetValue;

    StorageUpdateState expectedState;
    bool expectedDirty;
    EnumSet expected;
} EnumSetDefaultData[]{
    {"set",
        Enum::Three, {}, false, {},
        StorageOperation::Set, {Enum::Five}, {},
        StorageUpdateState::Success, true, Enum::Five},
    {"set to a zero value",
        Enum::Three, {}, false, {},
        StorageOperation::Set, {Enum::Zero}, {},
        StorageUpdateState::Success, true, Enum::Zero},
    {"set to the same value",
        Enum::Three, {}, false, {},
        StorageOperation::Set, {Enum::Three}, {},
        StorageUpdateState::Success, false, Enum::Three},

    {"set a single value to true",
        Enum::Three, {}, false, Enum::Five,
        StorageOperation::Set, {}, true,
        StorageUpdateState::Success, true, Enum::Five},
    /* Only supported for an enum set */
    {"set a single value to false",
        Enum::Three, {}, false, Enum::Five,
        StorageOperation::Set, {}, false,
        StorageUpdateState::Failed, false, Enum::Three},
    {"set a single value to true that's already true",
        Enum::Three, {}, false, Enum::Three,
        StorageOperation::Set, {}, true,
        StorageUpdateState::Success, false, Enum::Three},

    {"reset",
        Enum::Three, {}, false, {},
        StorageOperation::Reset, {}, {},
        StorageUpdateState::Success, true, Enum::Zero},
    {"reset when already zero",
        Enum::Zero, {}, false, {},
        StorageOperation::Reset, {}, {},
        StorageUpdateState::Success, false, Enum::Zero},
    {"reset with a custom default value",
        Enum::Three, {Enum::Five}, false, {},
        StorageOperation::Reset, {}, {},
        StorageUpdateState::Success, true, Enum::Five},
    {"reset with a custom default value when already that",
        Enum::Five, {Enum::Five}, false, {},
        StorageOperation::Reset, {}, {},
        StorageUpdateState::Success, false, Enum::Five},

    /* Only supported for enum set */
    {"reset a single value",
        Enum::Three, {}, false, Enum::Three,
        StorageOperation::Reset, {}, {},
        StorageUpdateState::Failed, false, Enum::Three},

    /* Toggle is only for single values */
    {"toggle a single value that isn't currently set",
        Enum::Three, {}, false, Enum::Five,
        StorageOperation::Toggle, {}, {},
        StorageUpdateState::Success, true, Enum::Five},
    /* Only supported for enum set */
    {"toggle a single value that is currently set",
        Enum::Three, {}, false, Enum::Three,
        StorageOperation::Toggle, {}, {},
        StorageUpdateState::Failed, false, Enum::Three},

    {"enum set, set",
        Enum::ZerothBit, {}, true, {},
        StorageOperation::Set, Enum::SeventhBit|Enum::EleventhBit, {},
        StorageUpdateState::Success, true, Enum::SeventhBit|Enum::EleventhBit},
    {"enum set, set to a zero value",
        Enum::ZerothBit, {}, true, {},
        StorageOperation::Set, EnumSet{}, {},
        StorageUpdateState::Success, true, EnumSet{}},
    {"enum set, set to the same value",
        Enum::SeventhBit|Enum::EleventhBit, {}, true, {},
        StorageOperation::Set, Enum::SeventhBit|Enum::EleventhBit, {},
        StorageUpdateState::Success, false, Enum::SeventhBit|Enum::EleventhBit},

    /* The value can have multiple bits, verifying all possible cases */
    {"enum set, set a single value to true",
        Enum::ZerothBit, {}, true, Enum::SeventhEleventhBit,
        StorageOperation::Set, {}, true,
        StorageUpdateState::Success, true, Enum::ZerothBit|Enum::SeventhBit|Enum::EleventhBit},
    {"enum set, set a single value to true that's already there",
        Enum::ZerothBit|Enum::SeventhBit|Enum::EleventhBit, {}, true, Enum::SeventhEleventhBit,
        StorageOperation::Set, {}, true,
        StorageUpdateState::Success, false, Enum::ZerothBit|Enum::SeventhBit|Enum::EleventhBit},
    {"enum set, set a single value to true that's only partially there",
        Enum::ZerothBit|Enum::SeventhBit, {}, true, Enum::SeventhEleventhBit,
        StorageOperation::Set, {}, true,
        StorageUpdateState::Success, true, Enum::ZerothBit|Enum::SeventhBit|Enum::EleventhBit},
    {"enum set, set a single value to false",
        Enum::ZerothBit|Enum::SeventhBit|Enum::EleventhBit, {}, true, Enum::SeventhEleventhBit,
        StorageOperation::Set, {}, false,
        StorageUpdateState::Success, true, Enum::ZerothBit},
    {"enum set, set a single value to false that already isn't there",
        Enum::ZerothBit, {}, true, Enum::SeventhEleventhBit,
        StorageOperation::Set, {}, false,
        StorageUpdateState::Success, false, Enum::ZerothBit},
    {"enum set, set a single value to false that only partially isn't there",
        Enum::ZerothBit|Enum::EleventhBit, {}, true, Enum::SeventhEleventhBit,
        StorageOperation::Set, {}, false,
        StorageUpdateState::Success, true, Enum::ZerothBit},
    /* The value can also have no bits, which should be a no-op */
    {"enum set, set a single zero value to true",
        Enum::SeventhBit, {}, true, Enum::Zero,
        StorageOperation::Set, {}, true,
        StorageUpdateState::Success, false, Enum::SeventhBit},
    {"enum set, set a single zero value to false",
        Enum::SeventhBit, {}, true, Enum::Zero,
        StorageOperation::Set, {}, false,
        StorageUpdateState::Success, false, Enum::SeventhBit},

    {"enum set, reset",
        Enum::SeventhBit, {}, true, {},
        StorageOperation::Reset, {}, {},
        StorageUpdateState::Success, true, EnumSet{}},
    {"enum set, reset when already zero",
        EnumSet{}, {}, true, {},
        StorageOperation::Reset, {}, {},
        StorageUpdateState::Success, false, EnumSet{}},
    {"enum set, reset with a custom default value",
        Enum::SeventhBit, {Enum::EleventhBit}, true, {},
        StorageOperation::Reset, {}, {},
        StorageUpdateState::Success, true, Enum::EleventhBit},
    {"enum set, reset with a custom default value when already that",
        Enum::EleventhBit, {Enum::EleventhBit}, true, {},
        StorageOperation::Reset, {}, {},
        StorageUpdateState::Success, false, Enum::EleventhBit},

    /* The value can have multiple bits, verifying all possible cases */
    {"enum set, reset a single value",
        Enum::ZerothBit|Enum::SeventhBit|Enum::EleventhBit, {}, true, Enum::SeventhEleventhBit,
        StorageOperation::Reset, {}, {},
        StorageUpdateState::Success, true, Enum::ZerothBit},
    {"enum set, reset a single value that's not there",
        Enum::ZerothBit, {}, true, Enum::SeventhEleventhBit,
        StorageOperation::Reset, {}, {},
        StorageUpdateState::Success, false, Enum::ZerothBit},
    {"enum set, reset a single value that's only partially there",
        Enum::ZerothBit, {}, true, Enum::SeventhEleventhBit,
        StorageOperation::Reset, {}, {},
        StorageUpdateState::Success, false, Enum::ZerothBit},
    {"enum set, reset a single value with a custom default value",
        Enum::ZerothBit, {Enum::SeventhBit}, true, Enum::SeventhEleventhBit,
        StorageOperation::Reset, {}, {},
        StorageUpdateState::Success, true, Enum::ZerothBit|Enum::SeventhBit},
    {"enum set, reset a single value with a custom default value when already that",
        Enum::ZerothBit|Enum::SeventhBit, {Enum::SeventhBit}, true, Enum::SeventhEleventhBit,
        StorageOperation::Reset, {}, {},
        StorageUpdateState::Success, false, Enum::ZerothBit|Enum::SeventhBit},
    {"enum set, reset a single value with a custom default value when partially that",
        Enum::ZerothBit|Enum::SeventhBit|Enum::EleventhBit, {Enum::EleventhBit}, true, Enum::SeventhEleventhBit,
        StorageOperation::Reset, {}, {},
        StorageUpdateState::Success, true, Enum::ZerothBit|Enum::EleventhBit},
    /* The value can also have no bits, which should be a no-op */
    {"enum set, reset a single zero value",
        Enum::SeventhBit, {}, true, Enum::Zero,
        StorageOperation::Reset, {}, {},
        StorageUpdateState::Success, false, Enum::SeventhBit},
    {"enum set, reset a single zero value with a custom default value",
        Enum::SeventhBit, {Enum::EleventhBit}, true, Enum::Zero,
        StorageOperation::Reset, {}, {},
        StorageUpdateState::Success, false, Enum::SeventhBit},

    /* Toggle is only for single values. Again the value can have multiple
       bits, verifying all possible cases */
    {"enum set, toggle a single value that isn't currently set",
        Enum::ZerothBit, {}, true, Enum::SeventhEleventhBit,
        StorageOperation::Toggle, {}, {},
        StorageUpdateState::Success, true, Enum::ZerothBit|Enum::SeventhBit|Enum::EleventhBit},
    {"enum set, toggle a single value that is currently set",
        Enum::ZerothBit|Enum::SeventhBit|Enum::EleventhBit, {}, true, Enum::SeventhEleventhBit,
        StorageOperation::Toggle, {}, {},
        StorageUpdateState::Success, true, Enum::ZerothBit},
    /* Here it sets both bits instead of toggling them seventh on and eleventh
       off */
    {"enum set, toggle a single value that is currently only partially set",
        Enum::ZerothBit|Enum::EleventhBit, {}, true, Enum::SeventhEleventhBit,
        StorageOperation::Toggle, {}, {},
        StorageUpdateState::Success, true, Enum::ZerothBit|Enum::SeventhBit|Enum::EleventhBit},
    /* The value can also have no bits, which should be a no-op */
    {"enum set, toggle a single zero value",
        Enum::SeventhBit, {}, true, Enum::Zero,
        StorageOperation::Toggle, {}, {},
        StorageUpdateState::Success, false, Enum::SeventhBit},
};

EnumStorageTest::EnumStorageTest() {
    addInstancedTests<EnumStorageTest>({
        &EnumStorageTest::constructValueInit<Small>,
        &EnumStorageTest::constructValueInit<Medium>,
        &EnumStorageTest::constructValueInit<Large>,
        &EnumStorageTest::constructNoInit<Small>,
        &EnumStorageTest::constructNoInit<Medium>,
        &EnumStorageTest::constructNoInit<Large>,
        &EnumStorageTest::constructDirectInit<Small>,
        &EnumStorageTest::constructDirectInit<Medium>,
        &EnumStorageTest::constructDirectInit<Large>,
        &EnumStorageTest::constructNonOwned1D<Small>,
        &EnumStorageTest::constructNonOwned1D<Medium>,
        &EnumStorageTest::constructNonOwned1D<Large>,
        &EnumStorageTest::constructNonOwned<Small>,
        &EnumStorageTest::constructNonOwned<Medium>,
        &EnumStorageTest::constructNonOwned<Large>},
        Containers::arraySize(ConstructData));

    addTests({&EnumStorageTest::constructTypeProperties<Long>,
              &EnumStorageTest::constructTypeProperties<NonScopedEnum>,
              &EnumStorageTest::constructTypeProperties<Enum>,
              &EnumStorageTest::constructTypeProperties<EnumSet>,
              &EnumStorageTest::constructCopy,

              &EnumStorageTest::constructHandleRecycle,

              &EnumStorageTest::access1D,
              &EnumStorageTest::access1DSingleValue,
              &EnumStorageTest::access,
              &EnumStorageTest::accessSingleValue});

    addInstancedTests({&EnumStorageTest::accessNonOwned1D,
                       &EnumStorageTest::accessNonOwned1DSingleValue},
        Containers::arraySize(AccessNonOwned1DData));

    addInstancedTests({&EnumStorageTest::accessNonOwned,
                       &EnumStorageTest::accessNonOwnedSingleValue},
        Containers::arraySize(AccessNonOwnedData));

    addTests({&EnumStorageTest::accessInvalid,
              &EnumStorageTest::nonOwnedMutableDataInvalid});

    addInstancedTests({&EnumStorageTest::update,
                       &EnumStorageTest::updateSingleValue},
        Containers::arraySize(UpdateData));

    addInstancedTests({&EnumStorageTest::enumSetDefault},
        Containers::arraySize(EnumSetDefaultData));
}

template<class> struct StorageTraits;
template<> struct StorageTraits<Small> {
    static const char* name() { return "Small"; }
};
template<> struct StorageTraits<Medium> {
    static const char* name() { return "Medium"; }
};
template<> struct StorageTraits<Large> {
    static const char* name() { return "Large"; }
};

template<class T> void EnumStorageTest::constructValueInit() {
    auto&& data = ConstructData[testCaseInstanceId()];
    setTestCaseDescription(data.name);
    setTestCaseTemplateName(StorageTraits<T>::name());

    /* Either this instance or the implicit one below gets used */
    DataLayer layer{layerHandle(0, 1)};

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    ui.setDataLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));

    /* On 64-bit up to 28 bytes can fit, on 32-bit up to 16 */
    #ifndef CORRADE_TARGET_32BIT
    constexpr std::size_t size = 28;
    #else
    constexpr std::size_t size = 16;
    #endif

    EnumStorage<T> first1 = data.implicitLayer ?
        EnumStorage<T>{ui, ValueInit, size, StorageFlags{0x18}} :
        EnumStorage<T>{layer, ValueInit, size, StorageFlags{0x18}};
    EnumStorage<T> first2 = data.implicitLayer ?
        EnumStorage<T>{ui, size, StorageFlags{0x18}} :
        EnumStorage<T>{layer, size, StorageFlags{0x18}};
    CORRADE_COMPARE(&first1.layer(), data.implicitLayer ? &ui.dataLayer() : &layer);
    CORRADE_COMPARE(&first2.layer(), data.implicitLayer ? &ui.dataLayer() : &layer);
    /* 1-byte storage fits exactly in-place, larger not anymore */
    CORRADE_COMPARE(first1.isAllocated(), sizeof(T) > 1);
    CORRADE_COMPARE(first2.isAllocated(), sizeof(T) > 1);
    CORRADE_VERIFY(!first1.isDirty());
    CORRADE_VERIFY(!first2.isDirty());
    CORRADE_COMPARE(first1.flags(), StorageFlags{0x18});
    CORRADE_COMPARE(first2.flags(), StorageFlags{0x18});
    CORRADE_COMPARE(first1.size(), size);
    CORRADE_COMPARE(first2.size(), size);
    CORRADE_VERIFY(first1.isMutable());
    CORRADE_VERIFY(first2.isMutable());
    CORRADE_COMPARE(first1.defaultValue(), T{});
    CORRADE_COMPARE(first2.defaultValue(), T{});
    /* Value access APIs tested in access1D() below */

    /* The data should have the expected shape and be a contiguous sequence of
       zeros */
    Containers::StridedArrayView1D<const T> viewFirst1 = first1.data();
    Containers::StridedArrayView1D<const T> viewFirst2 = first2.data();
    CORRADE_COMPARE(viewFirst1.size(), size);
    CORRADE_COMPARE(viewFirst2.size(), size);
    CORRADE_COMPARE(viewFirst1.stride(), sizeof(T));
    CORRADE_COMPARE(viewFirst2.stride(), sizeof(T));
    CORRADE_VERIFY(viewFirst1.isContiguous());
    CORRADE_VERIFY(viewFirst2.isContiguous());
    CORRADE_COMPARE_AS(viewFirst1.asContiguous(),
        Containers::stridedArrayView({T{}}).template broadcasted<0>(size),
        TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(viewFirst2.asContiguous(),
        Containers::stridedArrayView({T{}}).template broadcasted<0>(size),
        TestSuite::Compare::Container);
    /* Mutable data should be the same */
    CORRADE_COMPARE(first1.mutableData().data(), viewFirst1.data());
    CORRADE_COMPARE(first2.mutableData().data(), viewFirst2.data());
    CORRADE_COMPARE(first1.mutableData().size(), viewFirst1.size());
    CORRADE_COMPARE(first2.mutableData().size(), viewFirst2.size());
    CORRADE_COMPARE(first1.mutableData().stride(), viewFirst1.stride());
    CORRADE_COMPARE(first2.mutableData().stride(), viewFirst2.stride());

    EnumStorage<T> second1 = data.implicitLayer ?
        EnumStorage<T>{ui, ValueInit, StorageFlags{0x28}} :
        EnumStorage<T>{layer, ValueInit, StorageFlags{0x28}};
    EnumStorage<T> second2 = data.implicitLayer ?
        EnumStorage<T>{ui, StorageFlags{0x28}} :
        EnumStorage<T>{layer, StorageFlags{0x28}};
    CORRADE_COMPARE(&second1.layer(), data.implicitLayer ? &ui.dataLayer() : &layer);
    CORRADE_COMPARE(&second2.layer(), data.implicitLayer ? &ui.dataLayer() : &layer);
    /* Single-item storage should fit in-place always on 64-bit, on 32-bit only
       up to 4 bytes */
    #ifdef CORRADE_TARGET_32BIT
    CORRADE_COMPARE(second1.isAllocated(), sizeof(T) >= 8);
    CORRADE_COMPARE(second2.isAllocated(), sizeof(T) >= 8);
    #else
    CORRADE_VERIFY(!second1.isAllocated());
    CORRADE_VERIFY(!second2.isAllocated());
    #endif
    CORRADE_VERIFY(!second1.isDirty());
    CORRADE_VERIFY(!second2.isDirty());
    CORRADE_COMPARE(second1.flags(), StorageFlags{0x28});
    CORRADE_COMPARE(second2.flags(), StorageFlags{0x28});
    CORRADE_COMPARE(second1.size(), 1);
    CORRADE_COMPARE(second2.size(), 1);
    /* Value access APIs tested in access() below */
    Containers::StridedArrayView1D<const T> viewSecond1 = second1.data();
    Containers::StridedArrayView1D<const T> viewSecond2 = second2.data();
    CORRADE_COMPARE(viewSecond1.size(), 1);
    CORRADE_COMPARE(viewSecond2.size(), 1);
    CORRADE_COMPARE(viewSecond1.stride(), sizeof(T));
    CORRADE_COMPARE(viewSecond2.stride(), sizeof(T));
    CORRADE_VERIFY(viewSecond1.isContiguous());
    CORRADE_VERIFY(viewSecond2.isContiguous());
    CORRADE_COMPARE_AS(viewSecond1.asContiguous(),
        Containers::stridedArrayView({T{}}),
        TestSuite::Compare::Container);
    CORRADE_COMPARE_AS(viewSecond2.asContiguous(),
        Containers::stridedArrayView({T{}}),
        TestSuite::Compare::Container);
    /* Mutable data should be the same */
    CORRADE_COMPARE(second1.mutableData().data(), viewSecond1.data());
    CORRADE_COMPARE(second2.mutableData().data(), viewSecond2.data());
    CORRADE_COMPARE(second1.mutableData().size(), viewSecond1.size());
    CORRADE_COMPARE(second2.mutableData().size(), viewSecond2.size());
    CORRADE_COMPARE(second1.mutableData().stride(), viewSecond1.stride());
    CORRADE_COMPARE(second2.mutableData().stride(), viewSecond2.stride());
}

template<class T> void EnumStorageTest::constructNoInit() {
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

    /* On 64-bit up to 28 bytes can fit, on 32-bit up to 16 */
    #ifndef CORRADE_TARGET_32BIT
    constexpr std::size_t size = 28;
    #else
    constexpr std::size_t size = 16;
    #endif

    EnumStorage<T> first = data.implicitLayer ?
        EnumStorage<T>{ui, NoInit, size, StorageFlags{0x18}} :
        EnumStorage<T>{layer, NoInit, size, StorageFlags{0x18}};
    CORRADE_COMPARE(&first.layer(), data.implicitLayer ? &ui.dataLayer() : &layer);
    CORRADE_COMPARE(first.isAllocated(), sizeof(T) > 1);
    CORRADE_VERIFY(!first.isDirty());
    CORRADE_COMPARE(first.flags(), StorageFlags{0x18});
    CORRADE_COMPARE(first.size(), size);
    CORRADE_VERIFY(first.isMutable());
    CORRADE_COMPARE(first.defaultValue(), T{});

    /* The data should have the expected shape. Can't verify the contents as
       the allocation can have just anything. */
    Containers::StridedArrayView1D<const T> view = first.data();
    CORRADE_COMPARE(view.size(), size);
    CORRADE_COMPARE(view.stride(), sizeof(T));
    CORRADE_VERIFY(view.isContiguous());
    /* Mutable data should be the same */
    CORRADE_COMPARE(first.mutableData().data(), view.data());
    CORRADE_COMPARE(first.mutableData().size(), view.size());
    CORRADE_COMPARE(first.mutableData().stride(), view.stride());

    EnumStorage<T> second = data.implicitLayer ?
        EnumStorage<T>{ui, NoInit, StorageFlags{0x28}} :
        EnumStorage<T>{layer, NoInit, StorageFlags{0x28}};
    CORRADE_COMPARE(&second.layer(), data.implicitLayer ? &ui.dataLayer() : &layer);
    #ifdef CORRADE_TARGET_32BIT
    CORRADE_COMPARE(second.isAllocated(), sizeof(T) >= 8);
    #else
    CORRADE_VERIFY(!second.isAllocated());
    #endif
    CORRADE_VERIFY(!second.isDirty());
    CORRADE_COMPARE(second.flags(), StorageFlags{0x28});
    CORRADE_COMPARE(second.size(), 1);
    CORRADE_COMPARE(second.data().size(), 1);
    CORRADE_COMPARE(second.data().stride(), sizeof(T));
    /* Mutable data should be the same */
    CORRADE_COMPARE(second.mutableData().data(), second.data().data());
    CORRADE_COMPARE(second.mutableData().size(), second.data().size());
    CORRADE_COMPARE(second.mutableData().stride(), second.data().stride());
}

template<class T> void EnumStorageTest::constructDirectInit() {
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

    /* On 64-bit up to 28 bytes can fit, on 32-bit up to 16 */
    #ifndef CORRADE_TARGET_32BIT
    constexpr std::size_t size = 28;
    #else
    constexpr std::size_t size = 16;
    #endif

    EnumStorage<T> first = data.implicitLayer ?
        EnumStorage<T>{ui, DirectInit, size, T::Value, StorageFlags{0x18}} :
        EnumStorage<T>{layer, DirectInit, size, T::Value, StorageFlags{0x18}};
    CORRADE_COMPARE(&first.layer(), data.implicitLayer ? &ui.dataLayer() : &layer);
    CORRADE_COMPARE(first.isAllocated(), sizeof(T) > 1);
    CORRADE_VERIFY(!first.isDirty());
    CORRADE_COMPARE(first.flags(), StorageFlags{0x18});
    CORRADE_COMPARE(first.size(), size);
    CORRADE_VERIFY(first.isMutable());
    CORRADE_COMPARE(first.defaultValue(), T::Value);

    /* The data should have the expected shape and be a contiguous sequence of
       zeros */
    Containers::StridedArrayView1D<const T> viewFirst1 = first.data();
    CORRADE_COMPARE(viewFirst1.size(), size);
    CORRADE_COMPARE(viewFirst1.stride(), sizeof(T));
    CORRADE_VERIFY(viewFirst1.isContiguous());
    CORRADE_COMPARE_AS(viewFirst1.asContiguous(),
        Containers::stridedArrayView({T::Value}).template broadcasted<0>(size),
        TestSuite::Compare::Container);
    /* Mutable data should be the same */
    CORRADE_COMPARE(first.mutableData().data(), viewFirst1.data());
    CORRADE_COMPARE(first.mutableData().size(), viewFirst1.size());
    CORRADE_COMPARE(first.mutableData().stride(), viewFirst1.stride());

    EnumStorage<T> second = data.implicitLayer ?
        EnumStorage<T>{ui, DirectInit, T::Value, StorageFlags{0x28}} :
        EnumStorage<T>{layer, DirectInit, T::Value, StorageFlags{0x28}};
    CORRADE_COMPARE(&second.layer(), data.implicitLayer ? &ui.dataLayer() : &layer);
    #ifdef CORRADE_TARGET_32BIT
    CORRADE_COMPARE(second.isAllocated(), sizeof(T) >= 8);
    #else
    CORRADE_VERIFY(!second.isAllocated());
    #endif
    CORRADE_VERIFY(!second.isDirty());
    CORRADE_COMPARE(second.flags(), StorageFlags{0x28});
    CORRADE_COMPARE(second.size(), 1);
    Containers::StridedArrayView1D<const T> viewSecond1 = second.data();
    CORRADE_COMPARE(viewSecond1.size(), 1);
    CORRADE_COMPARE(viewSecond1.stride(), sizeof(T));
    CORRADE_VERIFY(viewSecond1.isContiguous());
    CORRADE_COMPARE_AS(viewSecond1.asContiguous(),
        Containers::stridedArrayView({T::Value}),
        TestSuite::Compare::Container);
    /* Mutable data should be the same */
    CORRADE_COMPARE(second.mutableData().data(), viewSecond1.data());
    CORRADE_COMPARE(second.mutableData().size(), viewSecond1.size());
    CORRADE_COMPARE(second.mutableData().stride(), viewSecond1.stride());
}

template<class T> void EnumStorageTest::constructNonOwned1D() {
    auto&& data = ConstructData[testCaseInstanceId()];
    setTestCaseDescription(data.name);
    setTestCaseTemplateName(StorageTraits<T>::name());

    /* Variant of constructValueInit() testing specifics of 1D NonOwned
       construction. Single-item variant tested in constructNonOwned()
       below. */

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
    /* The view is flipped so the stride should be too */
    CORRADE_COMPARE(storageView.stride(), -std::ptrdiff_t{sizeof(T)});

    EnumStorage<T> storage1 = data.implicitLayer ?
        EnumStorage<T>{ui, NonOwned, storageView, StorageFlags{0x18}} :
        EnumStorage<T>{layer, NonOwned, storageView, StorageFlags{0x18}};
    EnumStorage<T> storage2 = data.implicitLayer ?
        EnumStorage<T>{ui, NonOwned, constStorageView, StorageFlags{0x18}} :
        EnumStorage<T>{layer, NonOwned, constStorageView, StorageFlags{0x18}};
    CORRADE_COMPARE(&storage1.layer(), data.implicitLayer ? &ui.dataLayer() : &layer);
    CORRADE_COMPARE(&storage2.layer(), data.implicitLayer ? &ui.dataLayer() : &layer);
    /* Non-owned storage fits always on 64-bit, on 32-bit only up to 4 bytes */
    #ifdef CORRADE_TARGET_32BIT
    CORRADE_COMPARE(storage1.isAllocated(), sizeof(T) >= 8);
    CORRADE_COMPARE(storage2.isAllocated(), sizeof(T) >= 8);
    #else
    CORRADE_VERIFY(!storage1.isAllocated());
    CORRADE_VERIFY(!storage2.isAllocated());
    #endif
    CORRADE_VERIFY(!storage1.isDirty());
    CORRADE_VERIFY(!storage2.isDirty());
    CORRADE_COMPARE(storage1.flags(), StorageFlags{0x18});
    CORRADE_COMPARE(storage2.flags(), StorageFlags{0x18});
    CORRADE_COMPARE(storage1.size(), 3);
    CORRADE_COMPARE(storage2.size(), 3);
    CORRADE_VERIFY(storage1.isMutable());
    CORRADE_VERIFY(!storage2.isMutable());
    CORRADE_COMPARE(storage1.defaultValue(), T{});
    CORRADE_COMPARE(storage2.defaultValue(), T{});

    /* Check just that the const variants don't have an updater set */
    StorageQuery<T> query1 = storage1[0];
    StorageQuery<bool> query1SingleValue = storage1.template value<T::Value>(0);
    StorageQuery<T> query2 = storage2[0];
    StorageQuery<bool> query2SingleValue = storage2.template value<T::Value>(0);
    CORRADE_VERIFY(query1.isMutable());
    CORRADE_VERIFY(query1SingleValue.isMutable());
    CORRADE_VERIFY(!query2.isMutable());
    CORRADE_VERIFY(!query2SingleValue.isMutable());
    CORRADE_COMPARE(query1.operations(), StorageOperation::Set|StorageOperation::Reset);
    CORRADE_COMPARE(query1SingleValue.operations(), StorageOperation::Set|StorageOperation::Reset|StorageOperation::Toggle);
    CORRADE_COMPARE(query2.operations(), StorageOperations{});
    CORRADE_COMPARE(query2SingleValue.operations(), StorageOperations{});

    /* The data should be the same views as passed to the constructor */
    Containers::StridedArrayView1D<const T> viewFirst1 = storage1.data();
    Containers::StridedArrayView1D<const T> viewFirst2 = storage2.data();
    CORRADE_COMPARE(viewFirst1.data(), storageView.data());
    CORRADE_COMPARE(viewFirst2.data(), constStorageView.data());
    CORRADE_COMPARE(viewFirst1.size(), storageView.size());
    CORRADE_COMPARE(viewFirst2.size(), constStorageView.size());
    CORRADE_COMPARE(viewFirst1.stride(), storageView.stride());
    CORRADE_COMPARE(viewFirst2.stride(), constStorageView.stride());
    /* Mutable data should be the same. Assertion for the other tested in
       nonOwnedMutableDataInvalid(). */
    CORRADE_COMPARE(storage1.mutableData().data(), viewFirst1.data());
    CORRADE_COMPARE(storage1.mutableData().size(), viewFirst1.size());
    CORRADE_COMPARE(storage1.mutableData().stride(), viewFirst1.stride());
}

template<class T> void EnumStorageTest::constructNonOwned() {
    auto&& data = ConstructData[testCaseInstanceId()];
    setTestCaseDescription(data.name);
    setTestCaseTemplateName(StorageTraits<T>::name());

    /* Variant of constructNonOwned1D() testing the single-item case. The
       tested internals are the same in both cases but it's more readable this
       way. */

    /* Either this instance or the implicit one below gets used */
    DataLayer layer{layerHandle(0, 1)};

    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    ui.setDataLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));

    T storageData;
    const T& constStorageData = storageData;

    EnumStorage<T> storage1 = data.implicitLayer ?
        EnumStorage<T>{ui, NonOwned, storageData, StorageFlags{0x18}} :
        EnumStorage<T>{layer, NonOwned, storageData, StorageFlags{0x18}};
    EnumStorage<T> storage2 = data.implicitLayer ?
        EnumStorage<T>{ui, NonOwned, constStorageData, StorageFlags{0x18}} :
        EnumStorage<T>{layer, NonOwned, constStorageData, StorageFlags{0x18}};
    CORRADE_COMPARE(&storage1.layer(), data.implicitLayer ? &ui.dataLayer() : &layer);
    CORRADE_COMPARE(&storage2.layer(), data.implicitLayer ? &ui.dataLayer() : &layer);
    /* Non-owned storage fits always on 64-bit, on 32-bit only up to 4 bytes */
    #ifdef CORRADE_TARGET_32BIT
    CORRADE_COMPARE(storage1.isAllocated(), sizeof(T) >= 8);
    CORRADE_COMPARE(storage2.isAllocated(), sizeof(T) >= 8);
    #else
    CORRADE_VERIFY(!storage1.isAllocated());
    CORRADE_VERIFY(!storage2.isAllocated());
    #endif
    CORRADE_VERIFY(!storage1.isDirty());
    CORRADE_VERIFY(!storage2.isDirty());
    CORRADE_COMPARE(storage1.flags(), StorageFlags{0x18});
    CORRADE_COMPARE(storage2.flags(), StorageFlags{0x18});
    CORRADE_COMPARE(storage1.size(), 1);
    CORRADE_COMPARE(storage2.size(), 1);
    CORRADE_COMPARE(storage1.defaultValue(), T{});
    CORRADE_COMPARE(storage2.defaultValue(), T{});

    /* Check just that the const variants don't have an updater set */
    StorageQuery<T> query1 = storage1;
    StorageQuery<bool> query1SingleValue = storage1.template value<T::Value>();
    StorageQuery<T> query2 = storage2;
    StorageQuery<bool> query2SingleValue = storage2.template value<T::Value>();
    CORRADE_VERIFY(query1.isMutable());
    CORRADE_VERIFY(query1SingleValue.isMutable());
    CORRADE_VERIFY(!query2.isMutable());
    CORRADE_VERIFY(!query2SingleValue.isMutable());
    CORRADE_COMPARE(query1.operations(), StorageOperation::Set|StorageOperation::Reset);
    CORRADE_COMPARE(query1SingleValue.operations(), StorageOperation::Set|StorageOperation::Reset|StorageOperation::Toggle);
    CORRADE_COMPARE(query2.operations(), StorageOperations{});
    CORRADE_COMPARE(query2SingleValue.operations(), StorageOperations{});

    /* The data should be the same views as passed to the constructor */
    Containers::StridedArrayView1D<const T> viewFirst1 = storage1.data();
    Containers::StridedArrayView1D<const T> viewFirst2 = storage2.data();
    CORRADE_COMPARE(viewFirst1.data(), &storageData);
    CORRADE_COMPARE(viewFirst2.data(), &constStorageData);
    CORRADE_COMPARE(viewFirst1.size(), 1);
    CORRADE_COMPARE(viewFirst2.size(), 1);
    CORRADE_COMPARE(viewFirst1.stride(), sizeof(T));
    CORRADE_COMPARE(viewFirst2.stride(), sizeof(T));
    /* Mutable data should be the same. Assertion for the other tested in
       nonOwnedMutableDataInvalid(). */
    CORRADE_COMPARE(storage1.mutableData().data(), viewFirst1.data());
    CORRADE_COMPARE(storage1.mutableData().size(), viewFirst1.size());
    CORRADE_COMPARE(storage1.mutableData().stride(), viewFirst1.stride());
}

template<> struct StorageTraits<Long> {
    static const char* name() { return "Long"; }
    static bool isEnumSetByDefault() { return false; }
};
template<> struct StorageTraits<NonScopedEnum> {
    static const char* name() { return "NonScopedEnum"; }
    static bool isEnumSetByDefault() { return false; }
};
template<> struct StorageTraits<Enum> {
    static const char* name() { return "Enum"; }
    static bool isEnumSetByDefault() { return false; }
};
template<> struct StorageTraits<EnumSet> {
    static const char* name() { return "EnumSet"; }
    static bool isEnumSetByDefault() { return true; }
};

template<class T> void EnumStorageTest::constructTypeProperties() {
    setTestCaseTemplateName(StorageTraits<T>::name());

    DataLayer layer{layerHandle(0, 1)};

    /* Only the enum set toggle should depend on the type, the rest is
       type-independent */
    EnumStorage<T> storage{layer};
    CORRADE_COMPARE(storage.size(), 1);
    CORRADE_COMPARE(storage.defaultValue(), T{});
    CORRADE_COMPARE(storage.isEnumSet(), StorageTraits<T>::isEnumSetByDefault());
}

void EnumStorageTest::constructCopy() {
    struct Interface: UserInterface {
        explicit Interface(NoCreateT): UserInterface{NoCreate} {}
    } ui{NoCreate};
    ui.setDataLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));

    /* The copy constructor is implicitly generated, so just verify that
       something is done at all, and that the templated UI constructor doesn't
       break it */
    EnumStorage<Int> a1{ui.dataLayer()};
    EnumStorage<Int> a2{ui};
    /* This works even without restricting the templated UI constructor */
    EnumStorage<Int> b1 = a1;
    /* This only if the UI constructor isn't all-catching */
    EnumStorage<Int> b2{a2};
    CORRADE_COMPARE(&b1.layer(), &ui.dataLayer());
    CORRADE_COMPARE(&b2.layer(), &ui.dataLayer());
    CORRADE_COMPARE(b1.handle(), a1.handle());
    CORRADE_COMPARE(b2.handle(), a2.handle());

    #ifndef CORRADE_NO_STD_IS_TRIVIALLY_TRAITS
    /* This is verified by StorageQuery constructors and other APIs already,
       but doesn't hurt to have it here as well */
    CORRADE_VERIFY(std::is_trivially_copy_constructible<EnumStorage<Int>>::value);
    CORRADE_VERIFY(std::is_trivially_copy_assignable<EnumStorage<Int>>::value);
    #endif
}

void EnumStorageTest::constructHandleRecycle() {
    DataLayer layer{layerHandle(0, 1)};

    /* Create one more storage to verify the recycling isn't handling just the
       first item correctly */
    EnumStorage<Long>{layer};

    /* Create a storage and update its properties, non-owned so also the
       internal bits are set. It should be stored in-place so we can verify
       it gets properly reinitialized next time. */
    const UnsignedShort data[3]{};
    EnumStorage<UnsignedShort> first{layer, NonOwned, data};
    first
        .setDefaultValue(0x4000)
        .setEnumSet(true);
    CORRADE_VERIFY(!first.isAllocated());
    CORRADE_COMPARE(first.defaultValue(), 0x4000);
    CORRADE_VERIFY(first.isEnumSet());
    /* The query should be immutable also */
    CORRADE_VERIFY(!first[0].isMutable());

    /* Remove and create a new storage in the same slot. All properties should
       be reset back to defaults. */
    layer.removeStorage(first);
    EnumStorage<UnsignedShort> second{layer};
    CORRADE_COMPARE(storageHandleId(second.handle()), storageHandleId(first.handle()));
    CORRADE_VERIFY(!second.isAllocated());
    CORRADE_COMPARE(second.defaultValue(), 0);
    CORRADE_VERIFY(!second.isEnumSet());
    /* The internal immutable flag should also be reset */
    CORRADE_VERIFY(second.value().isMutable());
}

void EnumStorageTest::access1D() {
    AbstractUserInterface ui{{100, 100}};
    DataLayer& layer = ui.setLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));

    EnumStorage<UnsignedShort> storage{layer, NoInit, 7};
    CORRADE_VERIFY(!storage.isEnumSet());
    CORRADE_VERIFY(!storage.isDirty());

    Int value = 10000;
    for(std::size_t i = 0; i != storage.size(); ++i) {
        storage[i].set(++value);
        CORRADE_VERIFY(storage.isDirty());
    }

    Int expected = 10000;
    for(std::size_t i = 0; i != storage.size(); ++i) {
        CORRADE_ITERATION(i);
        CORRADE_COMPARE(storage[i], ++expected);
    }

    CORRADE_COMPARE_AS(storage.data().asContiguous(), Containers::arrayView<UnsignedShort>({
        10001,  10002,  10003,  10004,  10005,  10006,  10007
    }), TestSuite::Compare::Container);

    StorageQuery<UnsignedShort> query = storage[4];
    CORRADE_COMPARE(&query.layer(), &layer);
    CORRADE_COMPARE(query.storage(), storage.handle());
    CORRADE_COMPARE(query.index(), (Containers::Size3D{0, 0, 4}));
    CORRADE_VERIFY(query.isMutable());
    CORRADE_COMPARE(query.operations(), StorageOperation::Set|StorageOperation::Reset);
    CORRADE_COMPARE(query, 10005);

    Int called = 0;
    query.onUpdate([&called](UnsignedShort value) {
        CORRADE_COMPARE(value, 10005);
        ++called;
    });
    CORRADE_COMPARE(called, 0);

    ui.update();
    CORRADE_VERIFY(!storage.isDirty());
    CORRADE_COMPARE(called, 1);
}

void EnumStorageTest::access1DSingleValue() {
    AbstractUserInterface ui{{100, 100}};
    DataLayer& layer = ui.setLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));

    EnumStorage<EnumSet> storage{layer, NoInit, 7};
    CORRADE_VERIFY(storage.isEnumSet());
    CORRADE_VERIFY(!storage.isDirty());

    Int value = 0;
    for(std::size_t i = 0; i != storage.size(); ++i) {
        storage.value<Enum::ZerothBit>(i).toggle();
        storage.value<Enum::SeventhBit>(i).set(value % 2 == 0 ? true : false);
        storage.value<Enum::EleventhBit>(i).set(value % 3 == 0 ? true : false);
        ++value;
        CORRADE_VERIFY(storage.isDirty());
    }

    for(std::size_t i = 0; i != storage.size(); ++i) {
        CORRADE_ITERATION(i);
        CORRADE_VERIFY(storage.value<Enum::ZerothBit>(i));
        CORRADE_COMPARE(storage.value<Enum::SeventhBit>(i), i % 2 == 0);
        CORRADE_COMPARE(storage.value<Enum::EleventhBit>(i), i % 3 == 0);

        /* Compare also the full value in addition to bits */
        CORRADE_COMPARE(storage[i],
            Enum::ZerothBit|
            (i % 2 == 0 ? Enum::SeventhBit : EnumSet{})|
            (i % 3 == 0 ? Enum::EleventhBit : EnumSet{}));
    }

    CORRADE_COMPARE_AS(storage.data().asContiguous(), Containers::arrayView<EnumSet>({
        Enum::ZerothBit|Enum::SeventhBit|Enum::EleventhBit,
        Enum::ZerothBit,
        Enum::ZerothBit|Enum::SeventhBit,
        Enum::ZerothBit|Enum::EleventhBit,
        Enum::ZerothBit|Enum::SeventhBit,
        Enum::ZerothBit,
        Enum::ZerothBit|Enum::SeventhBit|Enum::EleventhBit
    }), TestSuite::Compare::Container);

    /* Query also the full value in addition to bits */
    StorageQuery<bool> querySeventh = storage.value<Enum::SeventhBit>(4);
    StorageQuery<bool> queryEleventh = storage.value<Enum::EleventhBit>(4);
    StorageQuery<EnumSet> query = storage[4];
    CORRADE_COMPARE(&querySeventh.layer(), &layer);
    CORRADE_COMPARE(&queryEleventh.layer(), &layer);
    CORRADE_COMPARE(&query.layer(), &layer);
    CORRADE_COMPARE(querySeventh.storage(), storage.handle());
    CORRADE_COMPARE(queryEleventh.storage(), storage.handle());
    CORRADE_COMPARE(query.storage(), storage.handle());
    CORRADE_COMPARE(querySeventh.index(), (Containers::Size3D{0, 0, 4}));
    CORRADE_COMPARE(queryEleventh.index(), (Containers::Size3D{0, 0, 4}));
    CORRADE_COMPARE(query.index(), (Containers::Size3D{0, 0, 4}));
    CORRADE_VERIFY(querySeventh.isMutable());
    CORRADE_VERIFY(queryEleventh.isMutable());
    CORRADE_VERIFY(query.isMutable());
    CORRADE_COMPARE(querySeventh.operations(), StorageOperation::Set|StorageOperation::Reset|StorageOperation::Toggle);
    CORRADE_COMPARE(queryEleventh.operations(), StorageOperation::Set|StorageOperation::Reset|StorageOperation::Toggle);
    CORRADE_COMPARE(query.operations(), StorageOperation::Set|StorageOperation::Reset);
    CORRADE_VERIFY(querySeventh);
    CORRADE_VERIFY(!queryEleventh);
    CORRADE_COMPARE(query, Enum::ZerothBit|Enum::SeventhBit);

    Int calledSeventh = 0, calledEleventh = 0, called = 0;
    querySeventh.onUpdate([&calledSeventh](bool value) {
        CORRADE_VERIFY(value);
        ++calledSeventh;
    });
    queryEleventh.onUpdate([&calledEleventh](bool value) {
        CORRADE_VERIFY(!value);
        ++calledEleventh;
    });
    query.onUpdate([&called](EnumSet value) {
        CORRADE_COMPARE(value, Enum::ZerothBit|Enum::SeventhBit);
        ++called;
    });
    CORRADE_COMPARE(calledSeventh, 0);
    CORRADE_COMPARE(calledEleventh, 0);
    CORRADE_COMPARE(called, 0);

    ui.update();
    CORRADE_VERIFY(!storage.isDirty());
    CORRADE_COMPARE(calledSeventh, 1);
    CORRADE_COMPARE(calledEleventh, 1);
    CORRADE_COMPARE(called, 1);
}

void EnumStorageTest::access() {
    /* A single-item subset of access1D(), basically */

    AbstractUserInterface ui{{100, 100}};
    DataLayer& layer = ui.setLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));

    EnumStorage<Short> storage{layer, NoInit};
    CORRADE_VERIFY(!storage.isEnumSet());
    CORRADE_VERIFY(!storage.isDirty());

    storage.value().set(-24069);
    CORRADE_VERIFY(storage.isDirty());
    CORRADE_COMPARE(storage.value(), -24069);

    CORRADE_COMPARE_AS(storage.data().asContiguous(), Containers::arrayView<Short>({
        -24069
    }), TestSuite::Compare::Container);

    /* Verifying both the explicit and implicit query */
    StorageQuery<Short> query1 = storage.value();
    StorageQuery<Short> query2 = storage;
    CORRADE_COMPARE(&query1.layer(), &layer);
    CORRADE_COMPARE(&query2.layer(), &layer);
    CORRADE_COMPARE(query1.storage(), storage.handle());
    CORRADE_COMPARE(query2.storage(), storage.handle());
    CORRADE_COMPARE(query1.index(), (Containers::Size3D{0, 0, 0}));
    CORRADE_COMPARE(query2.index(), (Containers::Size3D{0, 0, 0}));
    CORRADE_VERIFY(query1.isMutable());
    CORRADE_VERIFY(query2.isMutable());
    CORRADE_COMPARE(query1.operations(), StorageOperation::Set|StorageOperation::Reset);
    CORRADE_COMPARE(query2.operations(), StorageOperation::Set|StorageOperation::Reset);
    CORRADE_COMPARE(query1, -24069);
    CORRADE_COMPARE(query2, -24069);

    /* Verifying onUpdate() with both the explicit and implicit query */
    Int called1 = 0;
    Int called2 = 0;
    query1.onUpdate([&called1](Short value) {
        CORRADE_COMPARE(value, -24069);
        ++called1;
    });
    storage->onUpdate([&called2](Short value) {
        CORRADE_COMPARE(value, -24069);
        ++called2;
    });
    CORRADE_COMPARE(called1, 0);
    CORRADE_COMPARE(called2, 0);

    ui.update();
    CORRADE_VERIFY(!storage.isDirty());
    CORRADE_COMPARE(called1, 1);
    CORRADE_COMPARE(called2, 1);
}

void EnumStorageTest::accessSingleValue() {
    /* A single-item subset of access1DSingleValue(), basically */

    AbstractUserInterface ui{{100, 100}};
    DataLayer& layer = ui.setLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));

    EnumStorage<EnumSet> storage{layer, NoInit};
    CORRADE_VERIFY(storage.isEnumSet());
    CORRADE_VERIFY(!storage.isDirty());

    storage.value<Enum::ZerothBit>().toggle();
    storage.value<Enum::EleventhBit>().toggle();
    CORRADE_VERIFY(storage.isDirty());
    CORRADE_VERIFY(storage.value<Enum::ZerothBit>());
    CORRADE_VERIFY(!storage.value<Enum::SeventhBit>());
    CORRADE_VERIFY(storage.value<Enum::EleventhBit>());
    /* Compare also the full value in addition to bits */
    CORRADE_COMPARE(storage.value(), Enum::ZerothBit|Enum::EleventhBit);

    CORRADE_COMPARE_AS(storage.data().asContiguous(), Containers::arrayView<EnumSet>({
        Enum::ZerothBit|Enum::EleventhBit
    }), TestSuite::Compare::Container);

    /* Query also the full value in addition to bits. Explicit and implicit
       query was tested in access() above already. */
    StorageQuery<bool> querySeventh = storage.value<Enum::SeventhBit>();
    StorageQuery<bool> queryEleventh = storage.value<Enum::EleventhBit>();
    StorageQuery<EnumSet> query = storage.value();
    CORRADE_COMPARE(&querySeventh.layer(), &layer);
    CORRADE_COMPARE(&queryEleventh.layer(), &layer);
    CORRADE_COMPARE(&query.layer(), &layer);
    CORRADE_COMPARE(querySeventh.storage(), storage.handle());
    CORRADE_COMPARE(queryEleventh.storage(), storage.handle());
    CORRADE_COMPARE(query.storage(), storage.handle());
    CORRADE_COMPARE(querySeventh.index(), (Containers::Size3D{0, 0, 0}));
    CORRADE_COMPARE(queryEleventh.index(), (Containers::Size3D{0, 0, 0}));
    CORRADE_COMPARE(query.index(), (Containers::Size3D{0, 0, 0}));
    CORRADE_VERIFY(querySeventh.isMutable());
    CORRADE_VERIFY(queryEleventh.isMutable());
    CORRADE_VERIFY(query.isMutable());
    CORRADE_COMPARE(querySeventh.operations(), StorageOperation::Set|StorageOperation::Reset|StorageOperation::Toggle);
    CORRADE_COMPARE(queryEleventh.operations(), StorageOperation::Set|StorageOperation::Reset|StorageOperation::Toggle);
    CORRADE_COMPARE(query.operations(), StorageOperation::Set|StorageOperation::Reset);
    CORRADE_VERIFY(!querySeventh);
    CORRADE_VERIFY(queryEleventh);
    CORRADE_COMPARE(query, Enum::ZerothBit|Enum::EleventhBit);

    /* Again, explicit and implicit query was tested in access() above
       already */
    Int calledSeventh = 0, calledEleventh = 0, called = 0;
    querySeventh.onUpdate([&calledSeventh](bool value) {
        CORRADE_VERIFY(!value);
        ++calledSeventh;
    });
    queryEleventh.onUpdate([&calledEleventh](bool value) {
        CORRADE_VERIFY(value);
        ++calledEleventh;
    });
    query.onUpdate([&called](EnumSet value) {
        CORRADE_COMPARE(value, Enum::ZerothBit|Enum::EleventhBit);
        ++called;
    });
    CORRADE_COMPARE(calledSeventh, 0);
    CORRADE_COMPARE(calledEleventh, 0);
    CORRADE_COMPARE(called, 0);

    ui.update();
    CORRADE_VERIFY(!storage.isDirty());
    CORRADE_COMPARE(calledSeventh, 1);
    CORRADE_COMPARE(calledEleventh, 1);
    CORRADE_COMPARE(called, 1);
}

void EnumStorageTest::accessNonOwned1D() {
    auto&& data = AccessNonOwned1DData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    AbstractUserInterface ui{{100, 100}};
    DataLayer& layer = ui.setLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));

    Byte storageData[]{
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14
    };
    storageData[data.valueIndex] = -114;
    Containers::StridedArrayView1D<Byte> view{storageData, storageData + data.offset, 5, data.stride};
    CORRADE_COMPARE(view[4], -114);

    EnumStorage<Byte> storage = data.immutable ?
        EnumStorage<Byte>{layer, NonOwned, Containers::StridedArrayView1D<const Byte>{view}} :
        EnumStorage<Byte>{layer, NonOwned, view};
    CORRADE_VERIFY(!storage.isEnumSet());
    CORRADE_VERIFY(!storage.isDirty());

    StorageQuery<Byte> query = storage[4];
    CORRADE_COMPARE(&query.layer(), &layer);
    CORRADE_COMPARE(query.storage(), storage.handle());
    CORRADE_COMPARE(query.index(), (Containers::Size3D{0, 0, 4}));
    CORRADE_COMPARE(query.isMutable(), !data.immutable);
    CORRADE_COMPARE(query.operations(), data.immutable ?
        StorageOperations{} :
        StorageOperation::Set|StorageOperation::Reset);
    CORRADE_COMPARE(query, -114);

    Int called = 0;
    query.onUpdate([&called](Byte value) {
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

void EnumStorageTest::accessNonOwned1DSingleValue() {
    auto&& data = AccessNonOwned1DData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    AbstractUserInterface ui{{100, 100}};
    DataLayer& layer = ui.setLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));

    EnumSet storageData[]{
        Enum(0), Enum(1), Enum(2), Enum(3), Enum(4), Enum(5), Enum(6), Enum(7),
        Enum(8), Enum(9), Enum(10), Enum(11), Enum(12), Enum(13), Enum(14)
    };
    storageData[data.valueIndex] = Enum::ZerothBit|Enum::SeventhBit;
    Containers::StridedArrayView1D<EnumSet> view{storageData, storageData + data.offset, 5, data.stride*std::ptrdiff_t(sizeof(EnumSet))};
    CORRADE_COMPARE(view[4], Enum::ZerothBit|Enum::SeventhBit);

    EnumStorage<EnumSet> storage = data.immutable ?
        EnumStorage<EnumSet>{layer, NonOwned, Containers::StridedArrayView1D<const EnumSet>{view}} :
        EnumStorage<EnumSet>{layer, NonOwned, view};
    CORRADE_VERIFY(storage.isEnumSet());
    CORRADE_VERIFY(!storage.isDirty());

    /* Query also the full value in addition to bits */
    StorageQuery<bool> querySeventh = storage.value<Enum::SeventhBit>(4);
    StorageQuery<bool> queryEleventh = storage.value<Enum::EleventhBit>(4);
    StorageQuery<EnumSet> query = storage[4];
    CORRADE_COMPARE(&querySeventh.layer(), &layer);
    CORRADE_COMPARE(&queryEleventh.layer(), &layer);
    CORRADE_COMPARE(&query.layer(), &layer);
    CORRADE_COMPARE(querySeventh.storage(), storage.handle());
    CORRADE_COMPARE(queryEleventh.storage(), storage.handle());
    CORRADE_COMPARE(query.storage(), storage.handle());
    CORRADE_COMPARE(querySeventh.index(), (Containers::Size3D{0, 0, 4}));
    CORRADE_COMPARE(queryEleventh.index(), (Containers::Size3D{0, 0, 4}));
    CORRADE_COMPARE(query.index(), (Containers::Size3D{0, 0, 4}));
    CORRADE_COMPARE(querySeventh.isMutable(), !data.immutable);
    CORRADE_COMPARE(queryEleventh.isMutable(), !data.immutable);
    CORRADE_COMPARE(query.isMutable(), !data.immutable);
    CORRADE_COMPARE(querySeventh.operations(), data.immutable ?
        StorageOperations{} :
        StorageOperation::Set|StorageOperation::Reset|StorageOperation::Toggle);
    CORRADE_COMPARE(queryEleventh.operations(), data.immutable ?
        StorageOperations{} :
        StorageOperation::Set|StorageOperation::Reset|StorageOperation::Toggle);
    CORRADE_COMPARE(query.operations(), data.immutable ?
        StorageOperations{} :
        StorageOperation::Set|StorageOperation::Reset);
    CORRADE_VERIFY(querySeventh);
    CORRADE_VERIFY(!queryEleventh);
    CORRADE_COMPARE(query, Enum::ZerothBit|Enum::SeventhBit);

    Int calledSeventh = 0, calledEleventh = 0, called = 0;
    querySeventh.onUpdate([&calledSeventh](bool value) {
        CORRADE_VERIFY(value);
        ++calledSeventh;
    });
    queryEleventh.onUpdate([&calledEleventh](bool value) {
        CORRADE_VERIFY(!value);
        ++calledEleventh;
    });
    query.onUpdate([&called](EnumSet value) {
        CORRADE_COMPARE(value, Enum::ZerothBit|Enum::SeventhBit);
        ++called;
    });
    CORRADE_COMPARE(calledSeventh, 0);
    CORRADE_COMPARE(calledEleventh, 0);
    CORRADE_COMPARE(called, 0);

    ui.update();
    CORRADE_VERIFY(!storage.isDirty());
    CORRADE_COMPARE(calledSeventh, 1);
    CORRADE_COMPARE(calledEleventh, 1);
    CORRADE_COMPARE(called, 1);

    if(!data.immutable) {
        query.set(EnumSet{});
        queryEleventh.set(true);
        querySeventh.reset();
        CORRADE_VERIFY(storage.isDirty());
        CORRADE_VERIFY(!querySeventh);
        CORRADE_VERIFY(queryEleventh);
        CORRADE_COMPARE(query, EnumSet{Enum::EleventhBit});
        CORRADE_COMPARE(storageData[data.valueIndex], Enum::EleventhBit);
    }
}

void EnumStorageTest::accessNonOwned() {
    auto&& data = AccessNonOwnedData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* A single-item subset of accessNonOwned1D(), basically */

    AbstractUserInterface ui{{100, 100}};
    DataLayer& layer = ui.setLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));

    /* Verifying both the explicit and implicit query */
    Long storageData = -9876543210ll;
    EnumStorage<Long> storage = data.immutable ?
        EnumStorage<Long>{layer, NonOwned, static_cast<const Long&>(storageData)} :
        EnumStorage<Long>{layer, NonOwned, storageData};
    StorageQuery<Long> query1 = storage.value();
    StorageQuery<Long> query2 = storage;
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
        StorageOperation::Set|StorageOperation::Reset);
    CORRADE_COMPARE(query2.operations(), data.immutable ?
        StorageOperations{} :
        StorageOperation::Set|StorageOperation::Reset);
    CORRADE_COMPARE(query1, -9876543210ll);
    CORRADE_COMPARE(query2, -9876543210ll);

    /* Verifying onUpdate() with both the explicit and implicit query */
    Int called1 = 0, called2 = 0;
    query1.onUpdate([&called1](Long value) {
        CORRADE_COMPARE(value, -9876543210);
        ++called1;
    });
    storage->onUpdate([&called2](Long value) {
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

void EnumStorageTest::accessNonOwnedSingleValue() {
    auto&& data = AccessNonOwnedData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* A single-item subset of accessNonOwned1DSingleValue(), basically */

    AbstractUserInterface ui{{100, 100}};
    DataLayer& layer = ui.setLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));

    EnumSet storageData = Enum::ZerothBit|Enum::EleventhBit;
    EnumStorage<EnumSet> storage = data.immutable ?
        EnumStorage<EnumSet>{layer, NonOwned, static_cast<const EnumSet&>(storageData)} :
        EnumStorage<EnumSet>{layer, NonOwned, storageData};
    CORRADE_VERIFY(storage.isEnumSet());
    CORRADE_VERIFY(!storage.isDirty());

    /* Query also the full value in addition to bits. Explicit and implicit
       query was tested in accessNonOwned() above already. */
    StorageQuery<bool> querySeventh = storage.value<Enum::SeventhBit>();
    StorageQuery<bool> queryEleventh = storage.value<Enum::EleventhBit>();
    StorageQuery<EnumSet> query = storage.value();
    CORRADE_COMPARE(&querySeventh.layer(), &layer);
    CORRADE_COMPARE(&queryEleventh.layer(), &layer);
    CORRADE_COMPARE(&query.layer(), &layer);
    CORRADE_COMPARE(querySeventh.storage(), storage.handle());
    CORRADE_COMPARE(queryEleventh.storage(), storage.handle());
    CORRADE_COMPARE(query.storage(), storage.handle());
    CORRADE_COMPARE(querySeventh.index(), (Containers::Size3D{0, 0, 0}));
    CORRADE_COMPARE(queryEleventh.index(), (Containers::Size3D{0, 0, 0}));
    CORRADE_COMPARE(query.index(), (Containers::Size3D{0, 0, 0}));
    CORRADE_COMPARE(querySeventh.isMutable(), !data.immutable);
    CORRADE_COMPARE(queryEleventh.isMutable(), !data.immutable);
    CORRADE_COMPARE(query.isMutable(), !data.immutable);
    CORRADE_COMPARE(querySeventh.operations(), data.immutable ?
        StorageOperations{} :
        StorageOperation::Set|StorageOperation::Reset|StorageOperation::Toggle);
    CORRADE_COMPARE(queryEleventh.operations(), data.immutable ?
        StorageOperations{} :
        StorageOperation::Set|StorageOperation::Reset|StorageOperation::Toggle);
    CORRADE_COMPARE(query.operations(), data.immutable ?
        StorageOperations{} :
        StorageOperation::Set|StorageOperation::Reset);
    CORRADE_VERIFY(!querySeventh);
    CORRADE_VERIFY(queryEleventh);
    CORRADE_COMPARE(query, Enum::ZerothBit|Enum::EleventhBit);

    /* Again, explicit and implicit query was tested in accessNonOwned() above
       already */
    Int calledSeventh = 0, calledEleventh = 0, called = 0;
    querySeventh.onUpdate([&calledSeventh](bool value) {
        CORRADE_VERIFY(!value);
        ++calledSeventh;
    });
    queryEleventh.onUpdate([&calledEleventh](bool value) {
        CORRADE_VERIFY(value);
        ++calledEleventh;
    });
    query.onUpdate([&called](EnumSet value) {
        CORRADE_COMPARE(value, Enum::ZerothBit|Enum::EleventhBit);
        ++called;
    });
    CORRADE_COMPARE(calledSeventh, 0);
    CORRADE_COMPARE(calledEleventh, 0);
    CORRADE_COMPARE(called, 0);

    ui.update();
    CORRADE_VERIFY(!storage.isDirty());
    CORRADE_COMPARE(calledSeventh, 1);
    CORRADE_COMPARE(calledEleventh, 1);
    CORRADE_COMPARE(called, 1);

    if(!data.immutable) {
        query.set(EnumSet{});
        querySeventh.toggle();
        queryEleventh.reset();
        CORRADE_VERIFY(storage.isDirty());
        CORRADE_VERIFY(querySeventh);
        CORRADE_VERIFY(!queryEleventh);
        CORRADE_COMPARE(query, EnumSet{Enum::SeventhBit});
        CORRADE_COMPARE(storageData, Enum::SeventhBit);
    }
}

void EnumStorageTest::accessInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    /* All these assertions are fired by StorageQuery, this is just verifying
       that they actually get fired without something else happening first or
       them being sidestepped for example by delegating all APIs to the 3D
       queries. */

    DataLayer layer{layerHandle(0, 1)};
    EnumStorage<Enum> storage1D{layer, 15};

    Containers::String out;
    Error redirectError{&out};
    /* Index out of bounds. The assertion should be fired by StorageQuery, so
       just verify that we're not attempting to access something else first. */
    storage1D[15];
    storage1D.value<Enum::Five>(15);
    /* Single-item API for a 1D storage even though it's in bounds */
    storage1D.value();
    storage1D.value<Enum::Three>();
    StorageQuery<Enum>{storage1D};
    CORRADE_COMPARE_AS(out,
        "Ui::StorageQuery: index {0, 0, 15} out of range for {1, 1, 15} elements\n"
        "Ui::StorageQuery: index {0, 0, 15} out of range for {1, 1, 15} elements\n"

        "Ui::StorageQuery: expected a single-item storage but got a size of {1, 1, 15}\n"
        "Ui::StorageQuery: expected a single-item storage but got a size of {1, 1, 15}\n"
        "Ui::StorageQuery: expected a single-item storage but got a size of {1, 1, 15}\n",
        TestSuite::Compare::String);
}

void EnumStorageTest::nonOwnedMutableDataInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    DataLayer layer{layerHandle(0, 1)};

    const Enum storageData[7]{};
    EnumStorage<Enum> storage{layer, NonOwned, storageData};
    CORRADE_VERIFY(!storage.isMutable());

    Containers::String out;
    Error redirectError{&out};
    storage.mutableData();
    CORRADE_COMPARE(out, "Ui::EnumStorage::mutableData(): data not mutable\n");
}

void EnumStorageTest::update() {
    auto&& data = UpdateData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* This verifies that all update operations properly touch the stored or
       referenced value along with setting dirty bits. Behavior with update
       operations on a bit is tested in updateSingleValue() below, behavior
       with enum set storage, custom default etc. is tested in enumSetDefault()
       below. */

    AbstractUserInterface ui{{100, 100}};
    DataLayer& layer = ui.setLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));

    UnsignedByte storageData = 176;
    EnumStorage<UnsignedByte> storage = data.nonOwned ?
        EnumStorage<UnsignedByte>{layer, NonOwned, storageData} :
        EnumStorage<UnsignedByte>{layer, DirectInit, 176};
    StorageQuery<UnsignedByte> query = storage.value();

    storage.setDefaultValue(37);

    /* Attach an update to min and max as well to verify it's being passed
       correctly on updates */
    struct {
        Int called = 0;
        UnsignedByte expected;
    } state;
    DataHandle update = query.onUpdate([&state](UnsignedByte value) {
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

    /* Reset marks the storage as dirty */
    query.reset();
    CORRADE_VERIFY(layer.isStorageDirty(storage));
    CORRADE_VERIFY(!layer.isDirty(update));

    /* Update calls and resets */
    state.expected = 37;
    {
        CORRADE_ITERATION(__FILE__ ":" CORRADE_LINE_STRING);
        ui.update();
    }
    CORRADE_VERIFY(!layer.isStorageDirty(storage));
    CORRADE_VERIFY(!layer.isDirty(update));
    CORRADE_COMPARE(state.called, 3);
}

void EnumStorageTest::updateSingleValue() {
    auto&& data = UpdateData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Like update() but with operations on a single value query. Behavior with
       enum set storage, custom default etc. is tested in enumSetDefault()
       below. */

    AbstractUserInterface ui{{100, 100}};
    DataLayer& layer = ui.setLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));

    EnumSet storageData = Enum::ZerothBit|Enum::EleventhBit;
    EnumStorage<EnumSet> storage = data.nonOwned ?
        EnumStorage<EnumSet>{layer, NonOwned, storageData} :
        EnumStorage<EnumSet>{layer, DirectInit, Enum::ZerothBit|Enum::EleventhBit};
    CORRADE_VERIFY(storage.isEnumSet());

    StorageQuery<bool> querySeventh = storage.value<Enum::SeventhBit>();
    StorageQuery<bool> queryEleventh = storage.value<Enum::EleventhBit>();
    /* Query also the full value in addition to bits */
    StorageQuery<EnumSet> query = storage.value();

    /* The DirectInit constructor uses the passed value as a default, reset to
       have the behavior consistent with the NonOwned variant */
    if(!data.nonOwned)
        storage.setDefaultValue({});

    /* Attach an update to min and max as well to verify it's being passed
       correctly on updates */
    struct {
        Int calledSeventh = 0, calledEleventh = 0, called = 0;
        bool expectedSeventh, expectedEleventh;
        EnumSet expected;
    } state;
    DataHandle updateSeventh = querySeventh.onUpdate([&state](bool value) {
        CORRADE_COMPARE(value, state.expectedSeventh);
        ++state.calledSeventh;
    });
    DataHandle updateEleventh = queryEleventh.onUpdate([&state](bool value) {
        CORRADE_COMPARE(value, state.expectedEleventh);
        ++state.calledEleventh;
    });
    DataHandle update = query.onUpdate([&state](EnumSet value) {
        CORRADE_COMPARE(value, state.expected);
        ++state.called;
    });
    CORRADE_VERIFY(!layer.isStorageDirty(storage));
    CORRADE_VERIFY(layer.isDirty(updateSeventh));
    CORRADE_VERIFY(layer.isDirty(updateEleventh));
    CORRADE_VERIFY(layer.isDirty(update));

    /* The callback gets called on the first update */
    state.expectedSeventh = false;
    state.expectedEleventh = true;
    state.expected = Enum::ZerothBit|Enum::EleventhBit;
    {
        CORRADE_ITERATION(__FILE__ ":" CORRADE_LINE_STRING);
        ui.update();
    }
    CORRADE_VERIFY(!layer.isStorageDirty(storage));
    CORRADE_VERIFY(!layer.isDirty(updateSeventh));
    CORRADE_VERIFY(!layer.isDirty(updateEleventh));
    CORRADE_VERIFY(!layer.isDirty(update));
    CORRADE_COMPARE(state.calledSeventh, 1);
    CORRADE_COMPARE(state.calledEleventh, 1);
    CORRADE_COMPARE(state.called, 1);

    /* Changing a bit marks the storage as dirty */
    querySeventh.set(true);
    CORRADE_VERIFY(layer.isStorageDirty(storage));
    CORRADE_VERIFY(!layer.isDirty(update));

    /* Update calls and resets */
    state.expectedSeventh = true;
    state.expectedEleventh = true;
    state.expected = Enum::ZerothBit|Enum::SeventhBit|Enum::EleventhBit;
    {
        CORRADE_ITERATION(__FILE__ ":" CORRADE_LINE_STRING);
        ui.update();
    }
    CORRADE_VERIFY(!layer.isStorageDirty(storage));
    CORRADE_VERIFY(!layer.isDirty(updateSeventh));
    CORRADE_VERIFY(!layer.isDirty(updateEleventh));
    CORRADE_VERIFY(!layer.isDirty(update));
    CORRADE_COMPARE(state.calledSeventh, 2);
    CORRADE_COMPARE(state.calledEleventh, 2);
    CORRADE_COMPARE(state.called, 2);

    /* Reset marks the storage as dirty */
    queryEleventh.reset();
    CORRADE_VERIFY(layer.isStorageDirty(storage));
    CORRADE_VERIFY(!layer.isDirty(updateSeventh));
    CORRADE_VERIFY(!layer.isDirty(updateEleventh));
    CORRADE_VERIFY(!layer.isDirty(update));

    /* Update calls and resets */
    state.expectedSeventh = true;
    state.expectedEleventh = false;
    state.expected = Enum::ZerothBit|Enum::SeventhBit;
    {
        CORRADE_ITERATION(__FILE__ ":" CORRADE_LINE_STRING);
        ui.update();
    }
    CORRADE_VERIFY(!layer.isStorageDirty(storage));
    CORRADE_VERIFY(!layer.isDirty(updateSeventh));
    CORRADE_VERIFY(!layer.isDirty(updateEleventh));
    CORRADE_VERIFY(!layer.isDirty(update));
    CORRADE_COMPARE(state.calledSeventh, 3);
    CORRADE_COMPARE(state.calledEleventh, 3);
    CORRADE_COMPARE(state.called, 3);

    /* Toggle to true marks the storage as dirty */
    queryEleventh.toggle();
    CORRADE_VERIFY(layer.isStorageDirty(storage));
    CORRADE_VERIFY(!layer.isDirty(updateSeventh));
    CORRADE_VERIFY(!layer.isDirty(updateEleventh));
    CORRADE_VERIFY(!layer.isDirty(update));

    /* Update calls and resets */
    state.expectedSeventh = true;
    state.expectedEleventh = true;
    state.expected = Enum::ZerothBit|Enum::SeventhBit|Enum::EleventhBit;
    {
        CORRADE_ITERATION(__FILE__ ":" CORRADE_LINE_STRING);
        ui.update();
    }
    CORRADE_VERIFY(!layer.isStorageDirty(storage));
    CORRADE_VERIFY(!layer.isDirty(updateSeventh));
    CORRADE_VERIFY(!layer.isDirty(updateEleventh));
    CORRADE_VERIFY(!layer.isDirty(update));
    CORRADE_COMPARE(state.calledSeventh, 4);
    CORRADE_COMPARE(state.calledEleventh, 4);
    CORRADE_COMPARE(state.called, 4);

    /* Toggle to false marks the storage as dirty */
    querySeventh.toggle();
    CORRADE_VERIFY(layer.isStorageDirty(storage));
    CORRADE_VERIFY(!layer.isDirty(updateSeventh));
    CORRADE_VERIFY(!layer.isDirty(updateEleventh));
    CORRADE_VERIFY(!layer.isDirty(update));

    /* Update calls and resets */
    state.expectedSeventh = false;
    state.expectedEleventh = true;
    state.expected = Enum::ZerothBit|Enum::EleventhBit;
    {
        CORRADE_ITERATION(__FILE__ ":" CORRADE_LINE_STRING);
        ui.update();
    }
    CORRADE_VERIFY(!layer.isStorageDirty(storage));
    CORRADE_VERIFY(!layer.isDirty(updateSeventh));
    CORRADE_VERIFY(!layer.isDirty(updateEleventh));
    CORRADE_VERIFY(!layer.isDirty(update));
    CORRADE_COMPARE(state.calledSeventh, 5);
    CORRADE_COMPARE(state.calledEleventh, 5);
    CORRADE_COMPARE(state.called, 5);
}

void EnumStorageTest::enumSetDefault() {
    auto&& data = EnumSetDefaultData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Compared to update() / updateSingleValue(), which verified that the
       right value was updated depending on whether the storage is owned or
       not, and that both whole enum and single value queries were updated,
       this verifies that all operations correctly affect given memory location
       with various custom behavior, assuming the implementation isn't
       differing in the non-owned variant and with callbacks attached to single
       value queries. */

    AbstractUserInterface ui{{100, 100}};
    DataLayer& layer = ui.setLayerInstance(Containers::pointer<DataLayer>(ui.createLayer()));

    EnumStorage<EnumSet> storage{layer, NoInit, 17};
    StorageQuery<EnumSet> query = storage[14];
    query.set(data.value);

    struct {
        Int called = 0;
        EnumSet expected;
    } state;
    DataHandle update = query.onUpdate([&state](EnumSet value) {
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
       as dirty, and also shouldn't change the value in any way. */
    if(data.defaultValue) {
        storage.setDefaultValue(*data.defaultValue);
        CORRADE_COMPARE(storage.defaultValue(), *data.defaultValue);
        CORRADE_COMPARE(query, data.value);
        CORRADE_VERIFY(!layer.isStorageDirty(storage));
        CORRADE_VERIFY(!layer.isDirty(update));
    }

    /* Set an enum set storage as desired, default is true for an EnumSet. It
       should *not* mark the storage as dirty, and also shouldn't change the
       value in any way. Also doing this *after* setting the default value so
       it doesn't assert. */
    CORRADE_VERIFY(storage.isEnumSet());
    storage.setEnumSet(data.enumSet);
    CORRADE_COMPARE(storage.isEnumSet(), data.enumSet);
    CORRADE_COMPARE(query, data.value);
    CORRADE_VERIFY(!layer.isStorageDirty(storage));
    CORRADE_VERIFY(!layer.isDirty(update));

    /* Perform the desired update operation on either the whole value or a
       particular bit. This should mark the storage as dirty only if the value
       actually changes. */
    if(data.enumSetValue) {
        CORRADE_INTERNAL_ASSERT(!data.set);
        /** @todo yeah I know this isn't great */
        Containers::Optional<StorageQuery<bool>> bitQuery;
        switch(*data.enumSetValue) {
            #define _c(value_) case Enum::value_:                           \
                bitQuery = storage.value<Enum::value_>(14);                 \
                break;
            _c(Zero)
            _c(Three)
            _c(Five)
            _c(SeventhBit)
            _c(EleventhBit)
            _c(SeventhEleventhBit)
            #undef _c
            default:
                CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        }

        /* The query should be true if the original value is already containing
           the queried single value, and false if not. If enumSetValue is zero,
           this is true always. */
        CORRADE_COMPARE(*bitQuery, data.value >= *data.enumSetValue);

        if(data.operation == StorageOperation::Set) {
            CORRADE_COMPARE(bitQuery->set(*data.setEnumSetValue), data.expectedState);
        } else {
            CORRADE_INTERNAL_ASSERT(!data.setEnumSetValue);
            if(data.operation == StorageOperation::Reset)
                bitQuery->reset();
            else if(data.operation == StorageOperation::Toggle)
                bitQuery->toggle();
            else CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        }
    } else {
        CORRADE_INTERNAL_ASSERT(!data.setEnumSetValue);
        if(data.operation == StorageOperation::Set) {
            CORRADE_COMPARE(query.set(*data.set), data.expectedState);
        } else {
            CORRADE_INTERNAL_ASSERT(!data.set);
            if(data.operation == StorageOperation::Reset)
                query.reset();
            else CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        }
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

CORRADE_TEST_MAIN(Magnum::Ui::Test::EnumStorageTest)
