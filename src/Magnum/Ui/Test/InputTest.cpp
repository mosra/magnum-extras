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

#include <Corrade/Containers/String.h>
#include <Corrade/TestSuite/Compare/Numeric.h>
#include <Corrade/TestSuite/Compare/String.h>
#include <Corrade/Utility/Format.h> /** @todo drop once int-to-string APIs exist */

#include "Magnum/Ui/Anchor.h"
#include "Magnum/Ui/Event.h"
#include "Magnum/Ui/Formatter.h"
#include "Magnum/Ui/Input.h"
#include "Magnum/Ui/NodeFlags.h"
#include "Magnum/Ui/NumericStorage.h"
#include "Magnum/Ui/TextProperties.h"
#include "Magnum/Ui/Test/WidgetTester.hpp"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct InputTest: WidgetTester {
    explicit InputTest();

    void debugStyle();

    void construct();
    void constructTextProperties();
    /* So far, PasswordInput doesn't have a StorageQuery constructor, so these
       test only the Input */
    template<class T> void constructStorageQuery();
    template<class T> void constructStorageQueryNonOwned();
    template<class T> void constructStorageQueryStateless();
    template<class T, class Formatter> void constructStorageQueryFormatter();
    template<class T, class Formatter> void constructStorageQueryFormatterNonOwned();
    template<class T, class Formatter> void constructStorageQueryFormatterStateless();
    void constructStorageQueryInvalid();
    void constructNoCreate();

    void setStyle();
    void setStyleWhileActive();

    template<class T> void setText();
    template<class T> void setTextTextProperties();

    void setStyleTextInvalid();

    template<class T> void edit();
    void editStorageQuery();
    template<class T, class Formatter> void editStorageQueryCustomParser();
    void editStorageQueryIncrementDecrement();
};

using namespace Math::Literals;

const struct {
    const char* name;
    bool customDataLayer;
    StorageOperations operations;
} ConstructStorageQueryData[]{
    {"", false, {}},
    {"custom data layer", true, {}},
    {"increment/decrement support", false, StorageOperation::Increment|StorageOperation::Decrement},
};

const struct {
    const char* name;
    bool timeOverload;
} SetStyleData[]{
    {"", false},
    {"time overload", true}
};

const struct {
    const char* name;
    bool customDataLayer;
    InputStyle initialStyle;
    const char* text;
    InputStyle styleAfterInsert;
    /* 117 is an initial value */
    Int valueAfterInsert;
} EditStorageQueryData[]{
    {"trivial case", false,
        InputStyle::Default, "-123",
        InputStyle::Default, -123},
    {"custom data layer", true,
        InputStyle::Default, "-123",
        InputStyle::Default, -123},
    {"different initial style", false,
        InputStyle::Flat, "-123",
        InputStyle::Flat, -123},
    {"out of bounds", false,
        InputStyle::Default, "315",
        InputStyle::Warning, 200},
    {"out of bounds, different initial style", false,
        InputStyle::Flat, "315",
        InputStyle::Warning, 200},
    /* This doesn't fit into a signed 32-bit int, so the parser reports a
       Clamped state already, and it should be taken into account as well */
    {"out of bounds parser", false,
        InputStyle::Default, "-30000000000000",
        InputStyle::Warning, Math::TypeTraits<Int>::min()},
    {"invalid value", false,
        InputStyle::Default, "beef",
        InputStyle::Danger, 117}, /* value doesn't change */
    {"invalid value, different initial style", false,
        InputStyle::Flat, "beef",
        InputStyle::Danger, 117}, /* value doesn't change */
    {"redundant input", false,
        /* The text should become "155" only after blur */
        InputStyle::Default, " +00155  ",
        InputStyle::Default, 155},
};

const struct {
    const char* name;
    bool nonOwned;
    bool stateless;
} EditStorageQueryCustomParserData[]{
    {"", false, false},
    {"non-owned", true, false},
    {"stateless", false, true}
};

const struct {
    const char* name;
    bool customDataLayer;
    bool noIncrementDecrementOperation;
    InputStyle initialStyle;
    /* If not null, the input is currently being edited (and is thus
       focused) */
    const char* text;
    InputStyle styleAfterInsert;
    /* It should increment/decrement by 11 if Y is non-zero */
    Vector2 offset;
    bool accepted;
    InputStyle styleAfterScroll;
    /* 117 is an initial value */
    Int valueAfterScroll;
    const char* textAfterScroll;
} EditStorageQueryIncrementDecrementData[]{
    {"scroll up", false, false,
        InputStyle::Default, nullptr,
        InputStyle::Default, {0.0f, 1.0f}, true,
        InputStyle::Default, 128, "128"},
    {"scroll up, custom data layer", true, false,
        InputStyle::Default, nullptr,
        InputStyle::Default, {0.0f, 1.0f}, true,
        InputStyle::Default, 128, "128"},
    {"scroll up, different initial style", false, false,
        InputStyle::Flat, nullptr,
        InputStyle::Flat, {0.0f, 1.0f}, true,
        InputStyle::Flat, 128, "128"},
    {"scroll down", false, false,
        InputStyle::Default, nullptr,
        InputStyle::Default, {0.0f, -1.0f}, true,
        InputStyle::Default, 106, "106"},
    {"scroll down, custom data layer", true, false,
        InputStyle::Default, nullptr,
        InputStyle::Default, {0.0f, -1.0f}, true,
        InputStyle::Default, 106, "106"},
    {"scroll down, different initial style", false, false,
        InputStyle::Flat, nullptr,
        InputStyle::Flat, {0.0f, -1.0f}, true,
        InputStyle::Flat, 106, "106"},
    {"scroll up while edited", false, false,
        InputStyle::Default, "-123",
        InputStyle::Default, {0.0f, 1.0f}, true,
        InputStyle::Default, -112, "-112"},
    {"scroll down while edited", false, false,
        InputStyle::Default, "-123",
        InputStyle::Default, {0.0f, -1.0f}, true,
        InputStyle::Default, -134, "-134"},
    {"scroll down while out of bounds", false, false,
        InputStyle::Default, "315",
        InputStyle::Warning, {0.0f, -1.0f}, true,
        InputStyle::Default, 189, "189"},
    /* This doesn't actually change the storage value (it's 200 after the edit
       already) but nevertheless should result in the displayed text being
       changed */
    {"scroll up while out of bounds", false, false,
        InputStyle::Default, "315",
        InputStyle::Warning, {0.0f, 1.0f}, true,
        InputStyle::Default, 200, "200"},
    {"scroll dow while out of bounds, different initial style", false, false,
        InputStyle::Flat, "315",
        InputStyle::Warning, {0.0f, -1.0f}, true,
        InputStyle::Flat, 189, "189"},
    {"scroll up while an invalid value", false, false,
        InputStyle::Default, "beef",
        InputStyle::Danger, {0.0f, 1.0f}, true,
        InputStyle::Default, 128, "128"},
    {"scroll up while an invalid value, different initial style", false, false,
        InputStyle::Flat, "beef",
        InputStyle::Danger, {0.0f, 1.0f}, true,
        InputStyle::Flat, 128, "128"},
    /* These should cause no change, not even resetting the failure state */
    /** @todo make them also not accept the event so the scroll can propagate
        further, such as to a scroll area? */
    {"scroll nowhere", false, false,
        InputStyle::Default, nullptr,
        InputStyle::Default, {0.0f, 0.0f}, true,
        InputStyle::Default, 117, "117"}, /* value and text doesn't change */
    {"scroll left", false, false,
        InputStyle::Default, nullptr,
        InputStyle::Default, {-1.0f, 0.0f}, true,
        InputStyle::Default, 117, "117"}, /* value and text doesn't change */
    {"scroll right", false, false,
        InputStyle::Default, nullptr,
        InputStyle::Default, {1.0f, 0.0f}, true,
        InputStyle::Default, 117, "117"}, /* value and text doesn't change */
    {"scroll left while out of bounds", false, false,
        InputStyle::Default, "-2765",
        InputStyle::Warning, {-1.0f, 0.0f}, true,
        InputStyle::Warning, -200, "-2765"}, /* text doesn't change */
    {"scroll right while an invalid value", false, false,
        InputStyle::Default, "beef",
        InputStyle::Danger, {1.0f, 0.0f}, true,
        InputStyle::Danger, 117, "beef"}, /* text doesn't change */
    /* For these the event handlers shouldn't even be ther, thus not accepting
       the event */
    {"scroll up with a storage that doesn't support increment/decrement", false, true,
        InputStyle::Default, nullptr,
        InputStyle::Default, {0.0f, 1.0f}, false,
        InputStyle::Default, 117, "117"}, /* value and text doesn't change */
    {"scroll down with a storage that doesn't support increment/decrement", false, true,
        InputStyle::Default, nullptr,
        InputStyle::Default, {0.0f, -1.0f}, false,
        InputStyle::Default, 117, "117"}, /* value and text doesn't change */
};

InputTest::InputTest() {
    addTests({&InputTest::debugStyle});

    addTests<InputTest>({
        &InputTest::construct,
        &InputTest::constructTextProperties
    }, &WidgetTester::setup,
       &WidgetTester::teardown);

    addInstancedTests<InputTest>({
        &InputTest::constructStorageQuery<Int>,
        &InputTest::constructStorageQuery<UnsignedInt>,
        &InputTest::constructStorageQuery<Long>,
        &InputTest::constructStorageQuery<UnsignedLong>,
        &InputTest::constructStorageQuery<Float>,
        &InputTest::constructStorageQuery<Double>,
        &InputTest::constructStorageQueryNonOwned<Int>,
        &InputTest::constructStorageQueryNonOwned<UnsignedInt>,
        &InputTest::constructStorageQueryNonOwned<Long>,
        &InputTest::constructStorageQueryNonOwned<UnsignedLong>,
        &InputTest::constructStorageQueryNonOwned<Float>,
        &InputTest::constructStorageQueryNonOwned<Double>,
        &InputTest::constructStorageQueryStateless<Int>,
        &InputTest::constructStorageQueryStateless<UnsignedInt>,
        &InputTest::constructStorageQueryStateless<Long>,
        &InputTest::constructStorageQueryStateless<UnsignedLong>,
        &InputTest::constructStorageQueryStateless<Float>,
        &InputTest::constructStorageQueryStateless<Double>,
        &InputTest::constructStorageQueryFormatter<Int, DecimalFormatter>,
        &InputTest::constructStorageQueryFormatter<UnsignedInt, DecimalFormatter>,
        &InputTest::constructStorageQueryFormatter<Long, DecimalFormatter>,
        &InputTest::constructStorageQueryFormatter<UnsignedLong, DecimalFormatter>,
        &InputTest::constructStorageQueryFormatter<Int, HexadecimalFormatter>,
        &InputTest::constructStorageQueryFormatter<UnsignedInt, HexadecimalFormatter>,
        &InputTest::constructStorageQueryFormatter<Long, HexadecimalFormatter>,
        &InputTest::constructStorageQueryFormatter<UnsignedLong, HexadecimalFormatter>,
        &InputTest::constructStorageQueryFormatter<Float, FloatFormatter>,
        &InputTest::constructStorageQueryFormatter<Double, FloatFormatter>,
        &InputTest::constructStorageQueryFormatterNonOwned<Int, DecimalFormatter>,
        &InputTest::constructStorageQueryFormatterNonOwned<UnsignedInt, DecimalFormatter>,
        &InputTest::constructStorageQueryFormatterNonOwned<Long, DecimalFormatter>,
        &InputTest::constructStorageQueryFormatterNonOwned<UnsignedLong, DecimalFormatter>,
        &InputTest::constructStorageQueryFormatterNonOwned<Int, HexadecimalFormatter>,
        &InputTest::constructStorageQueryFormatterNonOwned<UnsignedInt, HexadecimalFormatter>,
        &InputTest::constructStorageQueryFormatterNonOwned<Long, HexadecimalFormatter>,
        &InputTest::constructStorageQueryFormatterNonOwned<UnsignedLong, HexadecimalFormatter>,
        &InputTest::constructStorageQueryFormatterNonOwned<Float, FloatFormatter>,
        &InputTest::constructStorageQueryFormatterNonOwned<Double, FloatFormatter>,
        &InputTest::constructStorageQueryFormatterStateless<Int, DecimalFormatter>,
        &InputTest::constructStorageQueryFormatterStateless<UnsignedInt, DecimalFormatter>,
        &InputTest::constructStorageQueryFormatterStateless<Long, DecimalFormatter>,
        &InputTest::constructStorageQueryFormatterStateless<UnsignedLong, DecimalFormatter>,
        &InputTest::constructStorageQueryFormatterStateless<Int, HexadecimalFormatter>,
        &InputTest::constructStorageQueryFormatterStateless<UnsignedInt, HexadecimalFormatter>,
        &InputTest::constructStorageQueryFormatterStateless<Long, HexadecimalFormatter>,
        &InputTest::constructStorageQueryFormatterStateless<UnsignedLong, HexadecimalFormatter>,
        &InputTest::constructStorageQueryFormatterStateless<Float, FloatFormatter>,
        &InputTest::constructStorageQueryFormatterStateless<Double, FloatFormatter>
    }, Containers::arraySize(ConstructStorageQueryData),
       &WidgetTester::setup,
       &WidgetTester::teardown);

    addTests<InputTest>({&InputTest::constructStorageQueryInvalid},
        &WidgetTester::setup,
        &WidgetTester::teardown);

    addTests<InputTest>({&InputTest::constructNoCreate},
        &WidgetTester::setupNoCreate,
        &WidgetTester::teardownNoCreate);

    addInstancedTests<InputTest>({
        &InputTest::setStyle,
        &InputTest::setStyleWhileActive
    }, Containers::arraySize(SetStyleData),
       &WidgetTester::setup,
       &WidgetTester::teardown);

    addTests<InputTest>({
        &InputTest::setText<Input>,
        &InputTest::setText<PasswordInput>,
        &InputTest::setTextTextProperties<Input>,
        &InputTest::setTextTextProperties<PasswordInput>,

        &InputTest::setStyleTextInvalid,

        &InputTest::edit<Input>,
        &InputTest::edit<PasswordInput>
    }, &WidgetTester::setup,
       &WidgetTester::teardown);

    addInstancedTests<InputTest>({
        &InputTest::editStorageQuery
    }, Containers::arraySize(EditStorageQueryData),
       &WidgetTester::setup,
       &WidgetTester::teardown);

    addInstancedTests<InputTest>({
        &InputTest::editStorageQueryCustomParser<Int, DecimalFormatter>,
        &InputTest::editStorageQueryCustomParser<UnsignedInt, DecimalFormatter>,
        &InputTest::editStorageQueryCustomParser<Long, DecimalFormatter>,
        &InputTest::editStorageQueryCustomParser<UnsignedLong, DecimalFormatter>,
        &InputTest::editStorageQueryCustomParser<Int, HexadecimalFormatter>,
        &InputTest::editStorageQueryCustomParser<UnsignedInt, HexadecimalFormatter>,
        &InputTest::editStorageQueryCustomParser<Long, HexadecimalFormatter>,
        &InputTest::editStorageQueryCustomParser<UnsignedLong, HexadecimalFormatter>,
        &InputTest::editStorageQueryCustomParser<Float, FloatFormatter>,
        &InputTest::editStorageQueryCustomParser<Double, FloatFormatter>
    }, Containers::arraySize(EditStorageQueryCustomParserData),
       &WidgetTester::setup,
       &WidgetTester::teardown);

    addInstancedTests<InputTest>({
        &InputTest::editStorageQueryIncrementDecrement
    }, Containers::arraySize(EditStorageQueryIncrementDecrementData),
       &WidgetTester::setup,
       &WidgetTester::teardown);
}

using Implementation::BaseStyle;
using Implementation::TextStyle;

void InputTest::debugStyle() {
    Containers::String out;
    Debug{&out} << InputStyle::Warning << InputStyle(0xef);
    CORRADE_COMPARE(out, "Ui::InputStyle::Warning Ui::InputStyle(0xef)\n");
}

void InputTest::construct() {
    Input input1{Anchor{root, {}, {32, 16}}, "hello", InputStyle::Warning};
    PasswordInput input1Password{Anchor{root, {}, {32, 16}}, "hello", InputStyle::Warning};
    Input input2{Anchor{root, {}, {16, 32}}, InputStyle::Flat};
    PasswordInput input2Password{Anchor{root, {}, {16, 32}}, InputStyle::Flat};
    CORRADE_COMPARE(ui.nodeParent(input1), root);
    CORRADE_COMPARE(ui.nodeParent(input1Password), root);
    CORRADE_COMPARE(ui.nodeParent(input2), root);
    CORRADE_COMPARE(ui.nodeParent(input2Password), root);
    CORRADE_COMPARE(ui.nodeSize(input1), (Vector2{32, 16}));
    CORRADE_COMPARE(ui.nodeSize(input1Password), (Vector2{32, 16}));
    CORRADE_COMPARE(ui.nodeSize(input2), (Vector2{16, 32}));
    CORRADE_COMPARE(ui.nodeSize(input2Password), (Vector2{16, 32}));
    CORRADE_COMPARE(ui.nodeFlags(input1), NodeFlag::Focusable);
    CORRADE_COMPARE(ui.nodeFlags(input1Password), NodeFlag::Focusable);
    CORRADE_COMPARE(ui.nodeFlags(input2), NodeFlag::Focusable);
    CORRADE_COMPARE(ui.nodeFlags(input2Password), NodeFlag::Focusable);
    CORRADE_VERIFY(input1.isOwned());
    CORRADE_VERIFY(input1Password.isOwned());
    CORRADE_VERIFY(input2.isOwned());
    CORRADE_VERIFY(input2Password.isOwned());

    CORRADE_VERIFY(!input1.hasDataBinding());
    CORRADE_VERIFY(!input1Password.hasDataBinding());
    CORRADE_VERIFY(!input2.hasDataBinding());
    CORRADE_VERIFY(!input2Password.hasDataBinding());
    CORRADE_COMPARE(input1.style(), InputStyle::Warning);
    CORRADE_COMPARE(input1Password.style(), InputStyle::Warning);
    CORRADE_COMPARE(input2.style(), InputStyle::Flat);
    CORRADE_COMPARE(input2Password.style(), InputStyle::Flat);
    /* The password input should store the text as well */
    CORRADE_COMPARE(input1.text(), "hello");
    CORRADE_COMPARE(input1Password.text(), "hello");
    CORRADE_COMPARE(input2.text(), "");
    CORRADE_COMPARE(input2Password.text(), "");

    CORRADE_VERIFY(ui.isHandleValid(input1.backgroundData()));
    CORRADE_VERIFY(ui.isHandleValid(input1Password.backgroundData()));
    CORRADE_VERIFY(ui.isHandleValid(input2.backgroundData()));
    CORRADE_VERIFY(ui.isHandleValid(input2Password.backgroundData()));
    CORRADE_VERIFY(ui.isHandleValid(input1.textData()));
    CORRADE_VERIFY(ui.isHandleValid(input1Password.textData()));
    CORRADE_VERIFY(ui.isHandleValid(input2.textData()));
    CORRADE_VERIFY(ui.isHandleValid(input2Password.textData()));
    CORRADE_COMPARE(input1.dataBindingData(), DataHandle::Null);
    CORRADE_COMPARE(input1Password.dataBindingData(), DataHandle::Null);
    CORRADE_COMPARE(input2.dataBindingData(), DataHandle::Null);
    CORRADE_COMPARE(input2Password.dataBindingData(), DataHandle::Null);
    CORRADE_COMPARE(ui.textLayer().glyphCount(input1.textData()), 5);
    CORRADE_COMPARE(ui.textLayer().glyphCount(input1Password.textData()), 5);
    CORRADE_COMPARE(ui.textLayer().glyphCount(input2.textData()), 0);
    CORRADE_COMPARE(ui.textLayer().glyphCount(input2Password.textData()), 0);

    /* The password inputs should have different text style to choose a
       different font */
    CORRADE_COMPARE(ui.textLayer().style(input1.textData()), UnsignedInt(TextStyle::InputWarning));
    CORRADE_COMPARE(ui.textLayer().style(input1Password.textData()), UnsignedInt(TextStyle::InputWarningPassword));
    CORRADE_COMPARE(ui.textLayer().style(input2.textData()), UnsignedInt(TextStyle::InputFlat));
    CORRADE_COMPARE(ui.textLayer().style(input2Password.textData()), UnsignedInt(TextStyle::InputFlatPassword));

    /* Can only verify that the layout data were created, they're not saved.
       Compared to StorageQuery constructors, here should be no event
       handlers. */
    CORRADE_COMPARE(ui.layoutLayer().usedCount(), 4);
    CORRADE_COMPARE(ui.eventLayer().usedCount(), 0);
    CORRADE_COMPARE(ui.textLayer().usedTextEditCallbackCount(), 0);
}

void InputTest::constructTextProperties() {
    Input input{Anchor{root, {}, {32, 16}}, "hello",
            TextProperties{}.setScript(Text::Script::Braille),
            InputStyle::Flat};
    PasswordInput inputPassword{Anchor{root, {}, {32, 16}}, "hello",
            TextProperties{}.setScript(Text::Script::Braille),
            InputStyle::Flat};
    CORRADE_COMPARE(ui.nodeParent(input), root);
    CORRADE_COMPARE(ui.nodeParent(inputPassword), root);
    CORRADE_COMPARE(ui.nodeSize(input), (Vector2{32, 16}));
    CORRADE_COMPARE(ui.nodeSize(inputPassword), (Vector2{32, 16}));
    CORRADE_COMPARE(ui.nodeFlags(input), NodeFlag::Focusable);
    CORRADE_COMPARE(ui.nodeFlags(inputPassword), NodeFlag::Focusable);
    CORRADE_VERIFY(input.isOwned());
    CORRADE_VERIFY(inputPassword.isOwned());

    CORRADE_VERIFY(!input.hasDataBinding());
    CORRADE_VERIFY(!inputPassword.hasDataBinding());
    CORRADE_COMPARE(input.style(), InputStyle::Flat);
    CORRADE_COMPARE(inputPassword.style(), InputStyle::Flat);
    /* The password input should store the text as well */
    CORRADE_COMPARE(input.text(), "hello");
    CORRADE_COMPARE(inputPassword.text(), "hello");

    CORRADE_VERIFY(ui.isHandleValid(input.backgroundData()));
    CORRADE_VERIFY(ui.isHandleValid(inputPassword.backgroundData()));
    CORRADE_VERIFY(ui.isHandleValid(input.textData()));
    CORRADE_VERIFY(ui.isHandleValid(inputPassword.textData()));
    CORRADE_COMPARE(input.dataBindingData(), DataHandle::Null);
    CORRADE_COMPARE(inputPassword.dataBindingData(), DataHandle::Null);
    /* Multiplied by 6 because of the Braille script */
    CORRADE_COMPARE(ui.textLayer().glyphCount(input.textData()), 5*6);
    CORRADE_COMPARE(ui.textLayer().glyphCount(inputPassword.textData()), 5*6);

    /* The password inputs should have different text style to choose a
       different font */
    CORRADE_COMPARE(ui.textLayer().style(input.textData()), UnsignedInt(TextStyle::InputFlat));
    CORRADE_COMPARE(ui.textLayer().style(inputPassword.textData()), UnsignedInt(TextStyle::InputFlatPassword));

    /* Can only verify that the layout data were created, they're not saved.
       Compared to StorageQuery constructors, here should be no event
       handlers. */
    CORRADE_COMPARE(ui.layoutLayer().usedCount(), 2);
    CORRADE_COMPARE(ui.eventLayer().usedCount(), 0);
    CORRADE_COMPARE(ui.textLayer().usedTextEditCallbackCount(), 0);
}

template<class> struct FormatterTraits;
template<> struct FormatterTraits<DecimalFormatter> {
    static const char* name() { return "DecimalFormatter"; }
    static const char* expected() { return "+133742"; }
};
template<> struct FormatterTraits<HexadecimalFormatter> {
    static const char* name() { return "HexadecimalFormatter"; }
    static const char* expected() { return "+20a6e"; }
};
template<> struct FormatterTraits<FloatFormatter> {
    static const char* name() { return "FloatFormatter"; }
    static const char* expected() { return "+133742"; }
};

/* Used by constructStorageQuery*() for explicit control over presence of
   StorageOperation::Increment etc. Actual editing tests use the builtin
   NumericStorage. */
template<class T> struct DummyStorage: AbstractStorage {
    typedef T Type;

    DummyStorage(DataLayer& layer, StorageOperations operations, StorageFlags flags): AbstractStorage{layer, flags} {
        *createInPlace<StorageOperations>() = operations;
    }

    operator StorageQuery<T>() const {
        return {*this, *data<StorageOperations>()|StorageOperation::Set, [](const DummyStorage&, StorageOperation) -> T {
            return 133742;
        }, [](const DummyStorage&, StorageOperation, const T*) -> StorageUpdateState {
            CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        }};
    }
};

template<class T> void InputTest::constructStorageQuery() {
    auto&& data = ConstructStorageQueryData[testCaseInstanceId()];
    setTestCaseDescription(data.name);
    setTestCaseTemplateName(Math::TypeTraits<T>::name());

    /* Once the input is gone, the storage gets removed as well */
    DummyStorage<T> storage = data.customDataLayer ?
        DummyStorage<T>{*customDataLayer, data.operations, StorageFlag::ReferenceCounted} :
        DummyStorage<T>{ui.dataLayer(), data.operations, StorageFlag::ReferenceCounted};

    Input input{Anchor{root, {}, {32, 16}}, storage, InputStyle::Danger};
    CORRADE_COMPARE(ui.nodeParent(input), root);
    CORRADE_COMPARE(ui.nodeOffset(input), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(input), (Vector2{32, 16}));
    CORRADE_VERIFY(input.isOwned());

    CORRADE_VERIFY(input.hasDataBinding());
    CORRADE_COMPARE(input.style(), InputStyle::Danger);

    CORRADE_VERIFY(ui.isHandleValid(input.backgroundData()));
    CORRADE_VERIFY(ui.isHandleValid(input.textData()));
    CORRADE_VERIFY(ui.isHandleValid(input.dataBindingData()));
    /* Initially there's nothing set */
    CORRADE_COMPARE(input.text(), "");
    /* The data binding should be attached to the input so it cleans up
       properly */
    CORRADE_COMPARE((data.customDataLayer ? *customDataLayer : ui.dataLayer()).node(input.dataBindingData()), input.node());

    /* The contents are set during the first update */
    ui.update();
    CORRADE_COMPARE(input.text(), "133742");

    /* Can only verify that the data were created, they're not saved */
    CORRADE_COMPARE(ui.layoutLayer().usedCount(), 1);
    /* One for blur, one for scroll if increment/decrement is supported */
    CORRADE_COMPARE(ui.eventLayer().usedCount(),
        data.operations >= StorageOperation::Increment ? 2 : 1);
    /* One for updating the storage upon edit */
    CORRADE_COMPARE(ui.textLayer().usedTextEditCallbackCount(), 1);

    /* Verify also that we're not allocating anything by accident */
    /** @todo once MSVC 2017 support is dropped, construct all callbacks with
        NoAllocatedInit to catch these at compile time */
    #ifndef CORRADE_MSVC2017_COMPATIBILITY
    CORRADE_COMPARE((data.customDataLayer ? *customDataLayer : ui.dataLayer()).usedAllocatedCount(), 0);
    #else
    CORRADE_COMPARE((data.customDataLayer ? *customDataLayer : ui.dataLayer()).usedAllocatedCount(), 1);
    #endif
    {
        CORRADE_EXPECT_FAIL("Event handlers currently capture a lot more than can be stored in-place.");
        CORRADE_COMPARE(ui.eventLayer().usedAllocatedCount(), 0);
        CORRADE_COMPARE(ui.textLayer().usedAllocatedTextEditCallbackCount(), 0);
    }
}

template<class T> void InputTest::constructStorageQueryNonOwned() {
    auto&& data = ConstructStorageQueryData[testCaseInstanceId()];
    setTestCaseDescription(data.name);
    setTestCaseTemplateName(Math::TypeTraits<T>::name());

    /* Like *NonOwned() tests above, just verifying that the arguments are all
       propagated */

    DummyStorage<T> storage = data.customDataLayer ?
        DummyStorage<T>{*customDataLayer, data.operations, StorageFlag::ReferenceCounted} :
        DummyStorage<T>{ui.dataLayer(), data.operations, StorageFlag::ReferenceCounted};

    Input input{NonOwned, Anchor{root, {}, {32, 16}}, storage, InputStyle::Danger};
    CORRADE_COMPARE(ui.nodeParent(input), root);
    CORRADE_COMPARE(ui.nodeOffset(input), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(input), (Vector2{32, 16}));
    CORRADE_VERIFY(!input.isOwned());

    CORRADE_COMPARE(input.style(), InputStyle::Danger);

    /* The contents are set during the first update */
    ui.update();
    CORRADE_COMPARE(input.text(), "133742");
}

template<class T> void InputTest::constructStorageQueryStateless() {
    auto&& data = ConstructStorageQueryData[testCaseInstanceId()];
    setTestCaseDescription(data.name);
    setTestCaseTemplateName(Math::TypeTraits<T>::name());

    DummyStorage<T> storage = data.customDataLayer ?
        DummyStorage<T>{*customDataLayer, data.operations, StorageFlag::ReferenceCounted} :
        DummyStorage<T>{ui.dataLayer(), data.operations, StorageFlag::ReferenceCounted};

    NodeHandle node = input(Anchor{root, {}, {32, 16}}, storage, InputStyle::Success);
    CORRADE_COMPARE(ui.nodeParent(node), root);
    CORRADE_COMPARE(ui.nodeOffset(node), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(node), (Vector2{32, 16}));

    /* Internally it should create a non-owned input, if it doesn't then this
       would remove everything */
    ui.clean();

    /* Can only verify that the data were created, nothing else. Count of
       allocations is tested sufficiently in constructStorageQuery() where this
       delegates to. */
    /** @todo test in InputGLTest once we have string query support also */
    CORRADE_COMPARE(ui.baseLayer().usedCount(), 1);
    CORRADE_COMPARE(ui.textLayer().usedCount(), 1);
    CORRADE_COMPARE(ui.textLayer().usedTextEditCallbackCount(), 1);
    CORRADE_COMPARE(ui.layoutLayer().usedCount(), 1);
    CORRADE_COMPARE(ui.eventLayer().usedCount(),
        data.operations >= StorageOperation::Increment ? 2 : 1);
    CORRADE_COMPARE((data.customDataLayer ? *customDataLayer : ui.dataLayer()).usedCount(), 1);
}

template<class T, class Formatter> void InputTest::constructStorageQueryFormatter() {
    auto&& data = ConstructStorageQueryData[testCaseInstanceId()];
    setTestCaseDescription(data.name);
    setTestCaseTemplateName({Math::TypeTraits<T>::name(), FormatterTraits<Formatter>::name()});

    /* Like constructStorageQuery(), just additionally passing an explicit
       formatter argument */

    /* Once the input is gone, the storage gets removed as well */
    DummyStorage<T> storage = data.customDataLayer ?
        DummyStorage<T>{*customDataLayer, data.operations, StorageFlag::ReferenceCounted} :
        DummyStorage<T>{ui.dataLayer(), data.operations, StorageFlag::ReferenceCounted};

    Input input1{Anchor{root, {}, {32, 16}}, storage,
        Formatter{Formatter::Flag::ExplicitPlus}, Formatter::parse,
        InputStyle::Danger};
    Input input2{Anchor{root, {}, {32, 16}}, storage,
        Formatter{Formatter::Flag::ExplicitPlus},
        InputStyle::Danger};
    CORRADE_COMPARE(ui.nodeParent(input1), root);
    CORRADE_COMPARE(ui.nodeParent(input2), root);
    CORRADE_COMPARE(ui.nodeOffset(input1), Vector2{});
    CORRADE_COMPARE(ui.nodeOffset(input2), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(input1), (Vector2{32, 16}));
    CORRADE_COMPARE(ui.nodeSize(input2), (Vector2{32, 16}));
    CORRADE_VERIFY(input1.isOwned());
    CORRADE_VERIFY(input2.isOwned());

    CORRADE_VERIFY(input1.hasDataBinding());
    CORRADE_VERIFY(input2.hasDataBinding());
    CORRADE_COMPARE(input1.style(), InputStyle::Danger);
    CORRADE_COMPARE(input2.style(), InputStyle::Danger);

    CORRADE_VERIFY(ui.isHandleValid(input1.backgroundData()));
    CORRADE_VERIFY(ui.isHandleValid(input2.backgroundData()));
    CORRADE_VERIFY(ui.isHandleValid(input1.textData()));
    CORRADE_VERIFY(ui.isHandleValid(input2.textData()));
    CORRADE_VERIFY(ui.isHandleValid(input1.dataBindingData()));
    CORRADE_VERIFY(ui.isHandleValid(input2.dataBindingData()));
    /* Initially there's nothing set */
    CORRADE_COMPARE(input1.text(), "");
    CORRADE_COMPARE(input2.text(), "");
    /* The data binding should be attached to the input so it cleans up
       properly */
    CORRADE_COMPARE((data.customDataLayer ? *customDataLayer : ui.dataLayer()).node(input1.dataBindingData()), input1.node());
    CORRADE_COMPARE((data.customDataLayer ? *customDataLayer : ui.dataLayer()).node(input2.dataBindingData()), input2.node());

    /* The contents are set during the first update */
    ui.update();
    CORRADE_COMPARE(input1.text(), FormatterTraits<Formatter>::expected());
    CORRADE_COMPARE(input2.text(), FormatterTraits<Formatter>::expected());

    CORRADE_COMPARE(ui.layoutLayer().usedCount(), 2*1);
    CORRADE_COMPARE(ui.eventLayer().usedCount(),
        data.operations >= StorageOperation::Increment ? 2*2 : 2*1);
    CORRADE_COMPARE(ui.textLayer().usedTextEditCallbackCount(), 2*1);

    /* Verify also that we're not allocating anything by accident */
    /** @todo once MSVC 2017 support is dropped, construct all callbacks with
        NoAllocatedInit to catch these at compile time */
    #ifndef CORRADE_MSVC2017_COMPATIBILITY
    CORRADE_COMPARE((data.customDataLayer ? *customDataLayer : ui.dataLayer()).usedAllocatedCount(), 0);
    #else
    CORRADE_COMPARE((data.customDataLayer ? *customDataLayer : ui.dataLayer()).usedAllocatedCount(), 2*1);
    #endif
    {
        CORRADE_EXPECT_FAIL("Event handlers currently capture a lot more than can be stored in-place.");
        CORRADE_COMPARE(ui.eventLayer().usedAllocatedCount(), 0);
        CORRADE_COMPARE(ui.textLayer().usedAllocatedTextEditCallbackCount(), 0);
    }
}

template<class T, class Formatter> void InputTest::constructStorageQueryFormatterNonOwned() {
    auto&& data = ConstructStorageQueryData[testCaseInstanceId()];
    setTestCaseDescription(data.name);
    setTestCaseTemplateName({Math::TypeTraits<T>::name(), FormatterTraits<Formatter>::name()});

    /* Like *NonOwned() tests above, just verifying that the arguments are all
       propagated */

    DummyStorage<T> storage = data.customDataLayer ?
        DummyStorage<T>{*customDataLayer, data.operations, StorageFlag::ReferenceCounted} :
        DummyStorage<T>{ui.dataLayer(), data.operations, StorageFlag::ReferenceCounted};

    Input input1{NonOwned, Anchor{root, {}, {32, 16}}, storage,
        Formatter{Formatter::Flag::ExplicitPlus}, Formatter::parse,
        InputStyle::Warning};
    Input input2{NonOwned, Anchor{root, {}, {32, 16}}, storage,
        Formatter{Formatter::Flag::ExplicitPlus},
        InputStyle::Warning};
    CORRADE_COMPARE(ui.nodeParent(input1), root);
    CORRADE_COMPARE(ui.nodeParent(input2), root);
    CORRADE_COMPARE(ui.nodeOffset(input1), Vector2{});
    CORRADE_COMPARE(ui.nodeOffset(input2), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(input1), (Vector2{32, 16}));
    CORRADE_COMPARE(ui.nodeSize(input2), (Vector2{32, 16}));
    CORRADE_VERIFY(!input1.isOwned());
    CORRADE_VERIFY(!input2.isOwned());

    CORRADE_COMPARE(input1.style(), InputStyle::Warning);
    CORRADE_COMPARE(input2.style(), InputStyle::Warning);

    /* The contents are set during the first update */
    ui.update();
    CORRADE_COMPARE(input1.text(), FormatterTraits<Formatter>::expected());
    CORRADE_COMPARE(input2.text(), FormatterTraits<Formatter>::expected());
}

template<class T, class Formatter> void InputTest::constructStorageQueryFormatterStateless() {
    auto&& data = ConstructStorageQueryData[testCaseInstanceId()];
    setTestCaseDescription(data.name);
    setTestCaseTemplateName({Math::TypeTraits<T>::name(), FormatterTraits<Formatter>::name()});

    DummyStorage<T> storage = data.customDataLayer ?
        DummyStorage<T>{*customDataLayer, data.operations, StorageFlag::ReferenceCounted} :
        DummyStorage<T>{ui.dataLayer(), data.operations, StorageFlag::ReferenceCounted};

    NodeHandle node1 = input(Anchor{root, {}, {32, 16}}, storage,
        Formatter{Formatter::Flag::ExplicitPlus}, Formatter::parse,
        InputStyle::Flat);
    NodeHandle node2 = input(Anchor{root, {}, {32, 16}}, storage,
        Formatter{Formatter::Flag::ExplicitPlus},
        InputStyle::Flat);
    CORRADE_COMPARE(ui.nodeParent(node1), root);
    CORRADE_COMPARE(ui.nodeParent(node2), root);
    CORRADE_COMPARE(ui.nodeOffset(node1), Vector2{});
    CORRADE_COMPARE(ui.nodeOffset(node2), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(node1), (Vector2{32, 16}));
    CORRADE_COMPARE(ui.nodeSize(node2), (Vector2{32, 16}));

    /* Internally it should create a non-owned input, if it doesn't then this
       would remove everything */
    ui.clean();

    /* Can only verify that the data were created, nothing else. Count of
       allocations is tested sufficiently in constructStorageQuery() where this
       delegates to. */
    /** @todo test in InputGLTest once we have string query support also */
    CORRADE_COMPARE(ui.baseLayer().usedCount(), 2*1);
    CORRADE_COMPARE(ui.textLayer().usedCount(), 2*1);
    CORRADE_COMPARE(ui.textLayer().usedTextEditCallbackCount(), 2*1);
    CORRADE_COMPARE(ui.layoutLayer().usedCount(), 2*1);
    CORRADE_COMPARE(ui.eventLayer().usedCount(),
        data.operations >= StorageOperation::Increment ? 2*2 : 2*1);
    CORRADE_COMPARE((data.customDataLayer ? *customDataLayer : ui.dataLayer()).usedCount(), 2*1);
}

void InputTest::constructStorageQueryInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct DummyStorage: AbstractStorage {
        explicit DummyStorage(DataLayer& layer): AbstractStorage{layer, StorageFlag::ReferenceCounted} {}
    } storage{ui.dataLayer()};

    StorageQuery<Int> queryNoSet{storage, ~StorageOperation::Set, [](const DummyStorage&, StorageOperation) -> Int {
        CORRADE_INTERNAL_ASSERT_UNREACHABLE();
    }, [](const DummyStorage&, StorageOperation, const Int*) -> StorageUpdateState {
        CORRADE_INTERNAL_ASSERT_UNREACHABLE();
    }};

    /* Immutable query cannot have StorageOperation::Set exposed anyway, so
       both cases get covered by the same check */
    StorageQuery<Int> queryImmutable{storage, {}, [](const DummyStorage&, StorageOperation) -> Int {
        CORRADE_INTERNAL_ASSERT_UNREACHABLE();
    }};

    Containers::String out;
    Error redirectError{&out};
    Input{Anchor{root, {}, {}}, queryNoSet};
    Input{Anchor{root, {}, {}}, queryImmutable};
    CORRADE_COMPARE_AS(out,
        "Ui::Input: Ui::StorageOperation::Set not supported\n"
        "Ui::Input: Ui::StorageOperation::Set not supported\n",
        TestSuite::Compare::String);
}

void InputTest::constructNoCreate() {
    Input input{NoCreate};
    PasswordInput inputPassword{NoCreate};
    CORRADE_COMPARE(input.node(), NodeHandle::Null);
    CORRADE_COMPARE(inputPassword.node(), NodeHandle::Null);
    CORRADE_COMPARE(input.backgroundData(), DataHandle::Null);
    CORRADE_COMPARE(inputPassword.backgroundData(), DataHandle::Null);
    CORRADE_COMPARE(input.textData(), DataHandle::Null);
    CORRADE_COMPARE(inputPassword.textData(), DataHandle::Null);
    CORRADE_COMPARE(input.dataBindingData(), DataHandle::Null);
    CORRADE_COMPARE(inputPassword.dataBindingData(), DataHandle::Null);
}

void InputTest::setStyle() {
    auto&& data = SetStyleData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    Input input{Anchor{root, {}, {32, 16}}, "hello", InputStyle::Danger};
    PasswordInput inputPassword{Anchor{root, {}, {32, 16}}, "hello", InputStyle::Danger};
    CORRADE_COMPARE(input.style(), InputStyle::Danger);
    CORRADE_COMPARE(inputPassword.style(), InputStyle::Danger);

    /* The password inputs should have different text style to choose a
       different font. The time overload should behave exactly the same
       assuming no animations are set up. Behavior with style-supplied
       animations tested in InputGLTest. */
    if(data.timeOverload) {
        input.setStyle(InputStyle::Success, 12345_nsec);
        inputPassword.setStyle(InputStyle::Success, 12345_nsec);
    } else {
        input.setStyle(InputStyle::Success);
        inputPassword.setStyle(InputStyle::Success);
    }
    CORRADE_COMPARE(input.style(), InputStyle::Success);
    CORRADE_COMPARE(inputPassword.style(), InputStyle::Success);
    CORRADE_COMPARE(ui.baseLayer().style(input.backgroundData()),
        UnsignedInt(BaseStyle::InputSuccess));
    CORRADE_COMPARE(ui.baseLayer().style(inputPassword.backgroundData()),
        UnsignedInt(BaseStyle::InputSuccess));
    CORRADE_COMPARE(ui.textLayer().style(input.textData()),
        UnsignedInt(TextStyle::InputSuccess));
    CORRADE_COMPARE(ui.textLayer().style(inputPassword.textData()),
        UnsignedInt(TextStyle::InputSuccessPassword));
}

void InputTest::setStyleWhileActive() {
    auto&& data = SetStyleData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    Input input{Anchor{root, {}, {32, 16}}, "hello", InputStyle::Success};
    PasswordInput inputPassword{Anchor{root, {}, {32, 16}}, "hello", InputStyle::Success};
    CORRADE_COMPARE(input.style(), InputStyle::Success);
    CORRADE_COMPARE(inputPassword.style(), InputStyle::Success);

    /* The password inputs should have different text style to choose a
       different font */
    CORRADE_COMPARE(ui.baseLayer().style(input.backgroundData()), UnsignedInt(BaseStyle::InputSuccess));
    CORRADE_COMPARE(ui.baseLayer().style(inputPassword.backgroundData()), UnsignedInt(BaseStyle::InputSuccess));
    CORRADE_COMPARE(ui.textLayer().style(input.textData()), UnsignedInt(TextStyle::InputSuccess));
    CORRADE_COMPARE(ui.textLayer().style(inputPassword.textData()), UnsignedInt(TextStyle::InputSuccessPassword));

    {
        FocusEvent focusEvent{{}};
        CORRADE_VERIFY(ui.focusEvent(input, focusEvent));
        CORRADE_COMPARE(ui.currentFocusedNode(), input);

        /* Verify that style transition works */
        CORRADE_COMPARE(ui.baseLayer().style(input.backgroundData()), UnsignedInt(BaseStyle::InputSuccessFocused));
        CORRADE_COMPARE(ui.textLayer().style(input.textData()), UnsignedInt(TextStyle::InputSuccessFocused));

        /* The time overload should behave exactly the same assuming no
           animations are set up. Behavior with style-supplied animations
           tested in InputGLTest. */
        if(data.timeOverload)
            input.setStyle(InputStyle::Default, 12345_nsec);
        else
            input.setStyle(InputStyle::Default);
        CORRADE_COMPARE(input.style(), InputStyle::Default);

        /* All styles should now be changed in a way that preserves the current
           focused state */
        CORRADE_COMPARE(ui.baseLayer().style(input.backgroundData()), UnsignedInt(BaseStyle::InputDefaultFocused));
        CORRADE_COMPARE(ui.textLayer().style(input.textData()), UnsignedInt(TextStyle::InputDefaultFocused));

    /* Same for the password input, which should use different text styles */
    } {
        FocusEvent focusEvent{{}};
        CORRADE_VERIFY(ui.focusEvent(inputPassword, focusEvent));
        CORRADE_COMPARE(ui.currentFocusedNode(), inputPassword);

        CORRADE_COMPARE(ui.baseLayer().style(inputPassword.backgroundData()), UnsignedInt(BaseStyle::InputSuccessFocused));
        CORRADE_COMPARE(ui.textLayer().style(inputPassword.textData()), UnsignedInt(TextStyle::InputSuccessPasswordFocused));

        if(data.timeOverload)
            inputPassword.setStyle(InputStyle::Default, 12345_nsec);
        else
            inputPassword.setStyle(InputStyle::Default);
        CORRADE_COMPARE(inputPassword.style(), InputStyle::Default);

        CORRADE_COMPARE(ui.baseLayer().style(inputPassword.backgroundData()), UnsignedInt(BaseStyle::InputDefaultFocused));
        CORRADE_COMPARE(ui.textLayer().style(inputPassword.textData()), UnsignedInt(TextStyle::InputDefaultPasswordFocused));
    }
}

template<class> struct InputTraits;
template<> struct InputTraits<Input> {
    static const char* name() { return "Input"; }
};
template<> struct InputTraits<PasswordInput> {
    static const char* name() { return "PasswordInput"; }
};

template<class T> void InputTest::setText() {
    setTestCaseTemplateName(InputTraits<T>::name());

    T input{Anchor{root, {}, {32, 16}}, "hiya"};
    CORRADE_COMPARE(input.text(), "hiya");
    CORRADE_COMPARE(ui.textLayer().glyphCount(input.textData()), 4);

    /* Both the rendered and stored text should update */
    input.setText("buh bye");
    CORRADE_COMPARE(input.text(), "buh bye");
    CORRADE_COMPARE(ui.textLayer().glyphCount(input.textData()), 7);
}

template<class T> void InputTest::setTextTextProperties() {
    setTestCaseTemplateName(InputTraits<T>::name());

    T input{Anchor{root, {}, {32, 16}}, "hiya"};
    CORRADE_COMPARE(input.text(), "hiya");
    CORRADE_COMPARE(ui.textLayer().glyphCount(input.textData()), 4);

    /* Both the rendered and stored text should update */
    input.setText("buh bye",
        TextProperties{}.setScript(Text::Script::Braille));
    CORRADE_COMPARE(input.text(), "buh bye");
    /* Multiplied by 6 because of the Braille script */
    CORRADE_COMPARE(ui.textLayer().glyphCount(input.textData()), 7*6);
}

void InputTest::setStyleTextInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    Input input{Anchor{ui, {}, {}},
        NumericStorage<Double>{ui, StorageFlag::ReferenceCounted}};

    Containers::String out;
    Error redirectError{&out};
    input.setStyle(InputStyle::Danger);
    input.setStyle(InputStyle::Danger, Nanoseconds{});
    input.setText({});
    CORRADE_COMPARE_AS(out,
        "Ui::Input::setStyle(): can't be called with a data binding present\n"
        "Ui::Input::setStyle(): can't be called with a data binding present\n"
        "Ui::Input::setText(): can't be called with a data binding present\n",
        TestSuite::Compare::String);
}

template<class T> void InputTest::edit() {
    setTestCaseTemplateName(InputTraits<T>::name());

    T input{Anchor{root, {}, {32, 16}}, "set"};
    /** @todo use a cursor API once it exists */
    CORRADE_COMPARE(ui.textLayer().cursor(input.textData()), Containers::pair(3u, 3u));

    /* Focus first */
    FocusEvent focusEvent{{}};
    CORRADE_VERIFY(ui.focusEvent(input, focusEvent));
    CORRADE_COMPARE(ui.currentFocusedNode(), input);

    /* Move two chars to the left */
    KeyEvent keyEvent1{{}, Key::Left, {}};
    KeyEvent keyEvent2{{}, Key::Left, {}};
    CORRADE_VERIFY(ui.keyPressEvent(keyEvent1));
    CORRADE_VERIFY(ui.keyPressEvent(keyEvent2));
    /** @todo use a cursor API once it exists */
    CORRADE_COMPARE(ui.textLayer().cursor(input.textData()), Containers::pair(1u, 1u));

    /* Insert a text */
    TextInputEvent textInputEvent{{}, "uns"};
    CORRADE_VERIFY(ui.textInputEvent(textInputEvent));
    CORRADE_COMPARE(input.text(), "sunset");
    /** @todo use a cursor API once it exists */
    CORRADE_COMPARE(ui.textLayer().cursor(input.textData()), Containers::pair(4u, 4u));
    CORRADE_COMPARE(ui.textLayer().glyphCount(input.textData()), 6);
}

void InputTest::editStorageQuery() {
    auto&& data = EditStorageQueryData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Testing just a single type and default formatter, as the editing
       behavior shouldn't differ between various types */

    NumericStorage<Int> storage = data.customDataLayer ?
        NumericStorage<Int>{*customDataLayer, DirectInit, 117, StorageFlag::ReferenceCounted} :
        NumericStorage<Int>{ui, DirectInit, 117, StorageFlag::ReferenceCounted};
    /* Have just an upper bound, lower bound unchanged, to be able to verify
       the parser reporting a Clamped state as well */
    storage.setRange(Math::TypeTraits<Int>::min(), 200);

    Input input{Anchor{root, {}, {32, 16}}, storage, data.initialStyle};

    /* Update to have the storage fill the text and select all to have the next
       text input event replace it */
    /** @todo have a builtin flag to select all on focus */
    ui.update();
    ui.textLayer().setCursor(input.textData(), 0, 3);

    /* Focus the node */
    FocusEvent focusEvent{{}};
    CORRADE_VERIFY(ui.focusEvent(input, focusEvent));
    CORRADE_COMPARE(ui.currentFocusedNode(), input);

    /* Input characters, which immediately updates the storage */
    TextInputEvent textInputEvent{{}, data.text};
    CORRADE_VERIFY(ui.textInputEvent(textInputEvent));
    CORRADE_COMPARE(input.text(), data.text);
    CORRADE_COMPARE(storage.value(), data.valueAfterInsert);

    /* The style potentially updates as well */
    auto expectedFocusedBackgroundStyle = [](InputStyle style) {
        if(style == InputStyle::Default)
            return BaseStyle::InputDefaultFocused;
        if(style == InputStyle::Warning)
            return BaseStyle::InputWarningFocused;
        if(style == InputStyle::Danger)
            return BaseStyle::InputDangerFocused;
        if(style == InputStyle::Flat)
            return BaseStyle::InputFlatFocused;
        CORRADE_INTERNAL_ASSERT_UNREACHABLE();
    };
    auto expectedFocusedTextStyle = [](InputStyle style) {
        if(style == InputStyle::Default)
            return TextStyle::InputDefaultFocused;
        if(style == InputStyle::Warning)
            return TextStyle::InputWarningFocused;
        if(style == InputStyle::Danger)
            return TextStyle::InputDangerFocused;
        if(style == InputStyle::Flat)
            return TextStyle::InputFlatFocused;
        CORRADE_INTERNAL_ASSERT_UNREACHABLE();
    };
    CORRADE_COMPARE(ui.baseLayer().style(input.backgroundData()), Int(expectedFocusedBackgroundStyle(data.styleAfterInsert)));
    CORRADE_COMPARE(ui.textLayer().style(input.textData()), Int(expectedFocusedTextStyle(data.styleAfterInsert)));

    /* The input text or style shouldn't change after an update() */
    ui.update();
    CORRADE_COMPARE(input.text(), data.text);
    CORRADE_COMPARE(ui.baseLayer().style(input.backgroundData()), Int(expectedFocusedBackgroundStyle(data.styleAfterInsert)));
    CORRADE_COMPARE(ui.textLayer().style(input.textData()), Int(expectedFocusedTextStyle(data.styleAfterInsert)));

    /* Blur the node */
    FocusEvent blurEvent{{}};
    CORRADE_VERIFY(!ui.focusEvent(NodeHandle::Null, blurEvent));
    CORRADE_COMPARE(ui.currentFocusedNode(), NodeHandle::Null);

    /* Now, after an update that refreshes data bindings, the text changes to
       match the storage value exactly, throwing away any error state or
       redundancy, and resetting the style back to the initial one */
    ui.update();
    CORRADE_COMPARE(input.text(), Utility::format("{}", data.valueAfterInsert));
    auto expectedBackgroundStyle = [](InputStyle style) {
        if(style == InputStyle::Default)
            return BaseStyle::InputDefault;
        if(style == InputStyle::Warning)
            return BaseStyle::InputWarning;
        if(style == InputStyle::Danger)
            return BaseStyle::InputDanger;
        if(style == InputStyle::Flat)
            return BaseStyle::InputFlat;
        CORRADE_INTERNAL_ASSERT_UNREACHABLE();
    };
    auto expectedTextStyle = [](InputStyle style) {
        if(style == InputStyle::Default)
            return TextStyle::InputDefault;
        if(style == InputStyle::Warning)
            return TextStyle::InputWarning;
        if(style == InputStyle::Danger)
            return TextStyle::InputDanger;
        if(style == InputStyle::Flat)
            return TextStyle::InputFlat;
        CORRADE_INTERNAL_ASSERT_UNREACHABLE();
    };
    CORRADE_COMPARE(ui.baseLayer().style(input.backgroundData()), Int(expectedBackgroundStyle(data.initialStyle)));
    CORRADE_COMPARE(ui.textLayer().style(input.textData()), Int(expectedTextStyle(data.initialStyle)));
}

template<class T, class Formatter> void InputTest::editStorageQueryCustomParser() {
    auto&& data = EditStorageQueryCustomParserData[testCaseInstanceId()];
    setTestCaseDescription(data.name);
    setTestCaseTemplateName({Math::TypeTraits<T>::name(), FormatterTraits<Formatter>::name()});

    /* Verifies that the parser argument passed to the constructor gets
       actually used in all overloads. The parser returns a constant value
       regardless of the input. */

    NumericStorage<T> storage{ui, StorageFlag::ReferenceCounted};

    Input input{NoCreate};
    NodeHandle inputNode;
    if(data.stateless)
        inputNode = Ui::input(Anchor{root, {}, {32, 16}}, storage, Formatter{}, [](Containers::StringView, T& value) {
            value = 133742;
            return ParseState::Success;
        });
    else if(data.nonOwned)
        inputNode = input = Input{NonOwned, Anchor{root, {}, {32, 16}}, storage, Formatter{}, [](Containers::StringView, T& value) {
            value = 133742;
            return ParseState::Success;
        }};
    else
        inputNode = input = Input{Anchor{root, {}, {32, 16}}, storage, Formatter{}, [](Containers::StringView, T& value) {
            value = 133742;
            return ParseState::Success;
        }};

    /* Focus the node */
    FocusEvent focusEvent{{}};
    CORRADE_VERIFY(ui.focusEvent(inputNode, focusEvent));
    CORRADE_COMPARE(ui.currentFocusedNode(), inputNode);

    /* Insert characters, which should result in the text being updated as
       expected */
    TextInputEvent textInputEvent{{}, "6969"};
    CORRADE_VERIFY(ui.textInputEvent(textInputEvent));
    if(input.node() != NodeHandle::Null)
        CORRADE_COMPARE(input.text(), "06969");

    /* But the storage value gets set to what the parser produces */
    CORRADE_COMPARE(storage.value(), 133742);
}

void InputTest::editStorageQueryIncrementDecrement() {
    auto&& data = EditStorageQueryIncrementDecrementData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    struct NoIncrementDecrementStorage: AbstractStorage {
        NoIncrementDecrementStorage(DataLayer& layer): AbstractStorage{layer, StorageFlag::ReferenceCounted} {}

        StorageQuery<Int> value() const {
            return {*this, StorageOperation::Set, [](const NoIncrementDecrementStorage&, StorageOperation) {
                return 117;
            }, [](const NoIncrementDecrementStorage&, StorageOperation, const Int*) -> StorageUpdateState {
                CORRADE_INTERNAL_ASSERT_UNREACHABLE();
            },};
        }
    };

    /* Compared to editStorageQuery() here the storage type isn't the same for
       all cases so storing just the query instead */
    StorageQuery<Int> query = data.noIncrementDecrementOperation ?
        NoIncrementDecrementStorage{ui.dataLayer()}.value() :
        data.customDataLayer ?
            NumericStorage<Int>{*customDataLayer, DirectInit, 117, StorageFlag::ReferenceCounted}
                .setRange(-200, 200)
                .setStep(11) :
            NumericStorage<Int>{ui, DirectInit, 117, StorageFlag::ReferenceCounted}
                .setRange(-200, 200)
                .setStep(11);

    Input input{Anchor{root, {}, {32, 16}}, query, data.initialStyle};

    /* Update to have the storage fill the text */
    ui.update();

    auto expectedFocusedBackgroundStyle = [](InputStyle style) {
        if(style == InputStyle::Default)
            return BaseStyle::InputDefaultFocused;
        if(style == InputStyle::Warning)
            return BaseStyle::InputWarningFocused;
        if(style == InputStyle::Danger)
            return BaseStyle::InputDangerFocused;
        if(style == InputStyle::Flat)
            return BaseStyle::InputFlatFocused;
        CORRADE_INTERNAL_ASSERT_UNREACHABLE();
    };
    auto expectedFocusedTextStyle = [](InputStyle style) {
        if(style == InputStyle::Default)
            return TextStyle::InputDefaultFocused;
        if(style == InputStyle::Warning)
            return TextStyle::InputWarningFocused;
        if(style == InputStyle::Danger)
            return TextStyle::InputDangerFocused;
        if(style == InputStyle::Flat)
            return TextStyle::InputFlatFocused;
        CORRADE_INTERNAL_ASSERT_UNREACHABLE();
    };

    /* If testing increment/decrement while editing is desired, focus the node
       and input the characters. The style may change based on that. */
    if(data.text) {
        /* Select all to have the text input event replace it */
        /** @todo have a builtin flag to select all on focus */
        ui.textLayer().setCursor(input.textData(), 0, 3);

        FocusEvent focusEvent{{}};
        CORRADE_VERIFY(ui.focusEvent(input, focusEvent));
        CORRADE_COMPARE(ui.currentFocusedNode(), input);

        TextInputEvent textInputEvent{{}, data.text};
        CORRADE_VERIFY(ui.textInputEvent(textInputEvent));
        CORRADE_COMPARE(input.text(), data.text);

        CORRADE_COMPARE(ui.baseLayer().style(input.backgroundData()), Int(expectedFocusedBackgroundStyle(data.styleAfterInsert)));
        CORRADE_COMPARE(ui.textLayer().style(input.textData()), Int(expectedFocusedTextStyle(data.styleAfterInsert)));
    }

    /* Scroll event on the node, which should potentially result in an
       immediate change to the storage. If the storage doesn't support
       increment / decrement, the event shouldn't even get accepted.
       If no update is expected, the query should stay at the original
       value. */
    ScrollEvent scrollEvent{{}, data.offset, {}};
    CORRADE_COMPARE(ui.scrollEvent({20, 10}, scrollEvent), data.accepted);
    CORRADE_COMPARE(query, data.valueAfterScroll);

    /* Now, after an update that refreshes data bindings, if an update is
       expected the text changes to match the storage value exactly, throwing
       away any error state. Otherwise it stays as what was set before. */
    ui.update();
    CORRADE_COMPARE(input.text(), data.textAfterScroll);

    /* The style is reset only if the event actually does something. If we
       edited above (and potentially turned it into a warning or danger), it
       stays focused, otherwise it can become only one of these two. */
    auto expectedBackgroundStyle = [](InputStyle style) {
        if(style == InputStyle::Default)
            return BaseStyle::InputDefault;
        if(style == InputStyle::Flat)
            return BaseStyle::InputFlat;
        CORRADE_INTERNAL_ASSERT_UNREACHABLE();
    };
    auto expectedTextStyle = [](InputStyle style) {
        if(style == InputStyle::Default)
            return TextStyle::InputDefault;
        if(style == InputStyle::Flat)
            return TextStyle::InputFlat;
        CORRADE_INTERNAL_ASSERT_UNREACHABLE();
    };
    CORRADE_COMPARE(ui.baseLayer().style(input.backgroundData()), data.text ?
        Int(expectedFocusedBackgroundStyle(data.styleAfterScroll)) :
        Int(expectedBackgroundStyle(data.styleAfterScroll)));
    CORRADE_COMPARE(ui.textLayer().style(input.textData()), data.text ?
        Int(expectedFocusedTextStyle(data.styleAfterScroll)) :
        Int(expectedTextStyle(data.styleAfterScroll)));
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::InputTest)
