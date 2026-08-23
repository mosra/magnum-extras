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
#include <Corrade/Containers/String.h>
#include <Corrade/TestSuite/Compare/String.h>

#include "Magnum/Ui/Checkbox.h"
#include "Magnum/Ui/EnumStorage.h"
#include "Magnum/Ui/Event.h"
#include "Magnum/Ui/TextProperties.h"
#include "Magnum/Ui/Test/WidgetTester.hpp"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct CheckboxTest: WidgetTester {
    explicit CheckboxTest();

    void debugStyle();

    void construct();
    void constructEmptyLabel();
    void constructTextProperties();
    void constructStorageQuery();
    void constructStorageQueryNonOwned();
    void constructStorageQueryStateless();
    void constructStorageQueryTextProperties();
    void constructStorageQueryTextPropertiesNonOwned();
    void constructStorageQueryTextPropertiesStateless();
    void constructStorageQueryInvalid();
    void constructNoCreate();

    void setStyle();
    void setStyleWhileActive();

    void setText();
    void setTextTextProperties();

    void editSet();
    void editToggle();
    void editToggleEvent();
};

const struct {
    const char* name;
    bool customDataLayer;
    StorageOperations operations;
} ConstructStorageQueryData[]{
    {"", false, StorageOperation::Set|StorageOperation::Toggle},
    {"custom data layer", true, StorageOperation::Set|StorageOperation::Toggle},
    {"no edit support", false, {}},
};

const struct {
    const char* name;
    bool builtinStorage;
    bool customDataLayer;
    bool singleChoice;
    bool immutable;
} EditData[]{
    {"builtin storage",
        true, false, false, false},
    {"",
        false, false, false, false},
    {"single-choice storage",
        false, false, true, false},
    {"immutable storage",
        false, false, false, true},
    {"custom data layer",
        false, true, false, false}
};

CheckboxTest::CheckboxTest() {
    addTests({&CheckboxTest::debugStyle});

    addTests<CheckboxTest>({
        &CheckboxTest::construct,
        &CheckboxTest::constructEmptyLabel,
        &CheckboxTest::constructTextProperties
    }, &WidgetTester::setup,
       &WidgetTester::teardown);

    addInstancedTests<CheckboxTest>({
        &CheckboxTest::constructStorageQuery,
        &CheckboxTest::constructStorageQueryNonOwned,
        &CheckboxTest::constructStorageQueryStateless,
        &CheckboxTest::constructStorageQueryTextProperties,
        &CheckboxTest::constructStorageQueryTextPropertiesNonOwned,
        &CheckboxTest::constructStorageQueryTextPropertiesStateless,
    }, Containers::arraySize(ConstructStorageQueryData),
       &WidgetTester::setup,
       &WidgetTester::teardown);

    addTests<CheckboxTest>({&CheckboxTest::constructStorageQueryInvalid},
        &WidgetTester::setup,
        &WidgetTester::teardown);

    addTests<CheckboxTest>({&CheckboxTest::constructNoCreate},
        &WidgetTester::setupNoCreate,
        &WidgetTester::teardownNoCreate);

    addTests<CheckboxTest>({
        &CheckboxTest::setStyle,
        &CheckboxTest::setStyleWhileActive,
        &CheckboxTest::setText,
        &CheckboxTest::setTextTextProperties
    }, &WidgetTester::setup,
       &WidgetTester::teardown);

    addInstancedTests<CheckboxTest>({
        &CheckboxTest::editSet,
        &CheckboxTest::editToggle,
        &CheckboxTest::editToggleEvent
    }, Containers::arraySize(EditData),
       &WidgetTester::setup,
       &WidgetTester::teardown);
}

using Implementation::BaseStyle;
using Implementation::TextStyle;

void CheckboxTest::debugStyle() {
    Containers::String out;
    Debug{&out} << CheckboxStyle::RadioButton << CheckboxStyle(0xef);
    CORRADE_COMPARE(out, "Ui::CheckboxStyle::RadioButton Ui::CheckboxStyle(0xef)\n");
}

void CheckboxTest::construct() {
    Checkbox checkbox{Anchor{root, {}, {32, 16}}, "Slap me", CheckboxStyle::RadioButton};
    CORRADE_COMPARE(ui.nodeParent(checkbox), root);
    CORRADE_COMPARE(ui.nodeSize(checkbox), (Vector2{32, 16}));
    CORRADE_VERIFY(checkbox.isOwned());

    CORRADE_COMPARE(checkbox.style(), CheckboxStyle::RadioButton);
    CORRADE_VERIFY(!checkbox.isChecked());

    CORRADE_VERIFY(ui.isHandleValid(checkbox.backgroundData()));
    CORRADE_VERIFY(ui.isHandleValid(checkbox.checkboxData()));
    CORRADE_VERIFY(ui.isHandleValid(checkbox.textData()));
    CORRADE_VERIFY(ui.isHandleValid(checkbox.dataBindingData()));
    CORRADE_COMPARE(ui.textLayer().glyphCount(checkbox.checkboxData()), 1);
    CORRADE_COMPARE(ui.textLayer().glyphCount(checkbox.textData()), 7);
    CORRADE_VERIFY(!ui.dataLayer().isStorageDirty(ui.dataLayer().storage(checkbox.dataBindingData())));

    /* The icon gets set during the first update but it's undetectable which
       one it actually is */
    ui.update();
    CORRADE_VERIFY(!ui.dataLayer().isDirty(checkbox.dataBindingData()));

    /* Can only verify that the ither data were created, they're not saved */
    CORRADE_COMPARE(ui.layoutLayer().usedCount(), 1);
    CORRADE_COMPARE(ui.eventLayer().usedCount(), 1);

    /* Verify also that we're not allocating anything by accident */
    /** @todo once MSVC 2017 support is dropped, construct all callbacks with
        NoAllocatedInit to catch these at compile time */
    CORRADE_COMPARE(ui.dataLayer().storageUsedAllocatedCount(), 0);
    #ifndef CORRADE_MSVC2017_COMPATIBILITY
    CORRADE_COMPARE(ui.dataLayer().usedAllocatedCount(), 0);
    CORRADE_COMPARE(ui.eventLayer().usedAllocatedCount(), 0);
    #else
    CORRADE_COMPARE(ui.dataLayer().usedAllocatedCount(), 1);
    CORRADE_COMPARE(ui.eventLayer().usedAllocatedCount(), 1);
    #endif
}

void CheckboxTest::constructEmptyLabel() {
    /* Verifies just that the text label is created always, even for an empty
       string, to not need to handle special cases in setText() and elsewhere.
       Compared to e.g. an icon-only Button, creating a Checkbox without a
       label isn't considered a common use case, so it isn't optimized for. */

    Checkbox checkbox{Anchor{root, {}, {32, 16}}, "", CheckboxStyle::RadioButton};
    CORRADE_COMPARE(ui.nodeParent(checkbox), root);
    CORRADE_COMPARE(ui.nodeSize(checkbox), (Vector2{32, 16}));
    CORRADE_VERIFY(checkbox.isOwned());

    CORRADE_COMPARE(checkbox.style(), CheckboxStyle::RadioButton);
    CORRADE_VERIFY(!checkbox.isChecked());

    CORRADE_VERIFY(ui.isHandleValid(checkbox.backgroundData()));
    CORRADE_VERIFY(ui.isHandleValid(checkbox.checkboxData()));
    CORRADE_VERIFY(ui.isHandleValid(checkbox.textData()));
    CORRADE_VERIFY(ui.isHandleValid(checkbox.dataBindingData()));
    CORRADE_COMPARE(ui.textLayer().glyphCount(checkbox.checkboxData()), 1);
    CORRADE_COMPARE(ui.textLayer().glyphCount(checkbox.textData()), 0);
}

void CheckboxTest::constructTextProperties() {
    Checkbox checkbox{Anchor{root, {}, {32, 16}}, "Slap me",
        TextProperties{}.setScript(Text::Script::Braille),
        CheckboxStyle::RadioButton};
    CORRADE_COMPARE(ui.nodeParent(checkbox), root);
    CORRADE_COMPARE(ui.nodeSize(checkbox), (Vector2{32, 16}));
    CORRADE_VERIFY(checkbox.isOwned());

    CORRADE_COMPARE(checkbox.style(), CheckboxStyle::RadioButton);
    CORRADE_VERIFY(!checkbox.isChecked());

    CORRADE_VERIFY(ui.isHandleValid(checkbox.backgroundData()));
    CORRADE_VERIFY(ui.isHandleValid(checkbox.checkboxData()));
    CORRADE_VERIFY(ui.isHandleValid(checkbox.textData()));
    CORRADE_VERIFY(ui.isHandleValid(checkbox.dataBindingData()));
    CORRADE_COMPARE(ui.textLayer().glyphCount(checkbox.checkboxData()), 1);
    /* Multiplied by 6 because of the Braille script */
    CORRADE_COMPARE(ui.textLayer().glyphCount(checkbox.textData()), 6*7);
    CORRADE_VERIFY(!ui.dataLayer().isStorageDirty(ui.dataLayer().storage(checkbox.dataBindingData())));

    /* The icon gets set during the first update but it's undetectable which
       one it actually is so just verify it got updated */
    ui.update();
    CORRADE_VERIFY(!ui.dataLayer().isDirty(checkbox.dataBindingData()));

    /* Can only verify that the ither data were created, they're not saved */
    CORRADE_COMPARE(ui.layoutLayer().usedCount(), 1);
    CORRADE_COMPARE(ui.eventLayer().usedCount(), 1);

    /* Verify also that we're not allocating anything by accident */
    /** @todo once MSVC 2017 support is dropped, construct all callbacks with
        NoAllocatedInit to catch these at compile time */
    CORRADE_COMPARE(ui.dataLayer().storageUsedAllocatedCount(), 0);
    #ifndef CORRADE_MSVC2017_COMPATIBILITY
    CORRADE_COMPARE(ui.dataLayer().usedAllocatedCount(), 0);
    CORRADE_COMPARE(ui.eventLayer().usedAllocatedCount(), 0);
    #else
    CORRADE_COMPARE(ui.dataLayer().usedAllocatedCount(), 1);
    CORRADE_COMPARE(ui.eventLayer().usedAllocatedCount(), 1);
    #endif
}

/* Used by constructStorageQuery*() for explicit control over presence of
   StorageOperation::Toggle etc. Actual editing tests use the builtin
   EnumStorage. */
struct DummyStorage: AbstractStorage {
    DummyStorage(DataLayer& layer, StorageOperations operations, StorageFlags flags): AbstractStorage{layer, flags} {
        *createInPlace<StorageOperations>() = operations;
    }

    operator StorageQuery<bool>() const {
        return *data<StorageOperations>() ?
            StorageQuery<bool>{*this, *data<StorageOperations>(), [](const DummyStorage&, StorageOperation) -> bool {
                return true;
            }, [](const DummyStorage&, StorageOperation, const bool*) -> StorageUpdateState {
                CORRADE_INTERNAL_ASSERT_UNREACHABLE();
            }} :
            StorageQuery<bool>{*this, *data<StorageOperations>(), [](const DummyStorage&, StorageOperation) -> bool {
                /* Yep, this returns a different value than above. The actual
                   chosen icon is impossible to query without rendering it so
                   the value doesn't matter. */
                return false;
            }};
    }
};

void CheckboxTest::constructStorageQuery() {
    auto&& data = ConstructStorageQueryData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Once the checkbox is gone, the storage gets removed as well */
    DummyStorage storage = data.customDataLayer ?
        DummyStorage{*customDataLayer, data.operations, StorageFlag::ReferenceCounted} :
        DummyStorage{ui.dataLayer(), data.operations, StorageFlag::ReferenceCounted};

    Checkbox checkbox{Anchor{root, {}, {32, 16}}, storage, "Kick me", CheckboxStyle::RadioButton};
    CORRADE_COMPARE(ui.nodeParent(checkbox), root);
    CORRADE_COMPARE(ui.nodeOffset(checkbox), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(checkbox), (Vector2{32, 16}));
    CORRADE_VERIFY(checkbox.isOwned());

    CORRADE_COMPARE(checkbox.style(), CheckboxStyle::RadioButton);
    CORRADE_COMPARE(ui.textLayer().glyphCount(checkbox.textData()), 7);

    /* The icon gets set during the first update but it's undetectable which
       one it actually is so just verify it got updated */
    ui.update();
    CORRADE_VERIFY(!(data.customDataLayer ? *customDataLayer : ui.dataLayer()).isDirty(checkbox.dataBindingData()));

    /* Can only verify that the ither data were created, they're not saved. No
       event handlers are used if the query isn't toggleable. */
    CORRADE_COMPARE(ui.layoutLayer().usedCount(), 1);
    CORRADE_COMPARE(ui.eventLayer().usedCount(),
        data.operations >= StorageOperation::Toggle ? 1 : 0);

    /* Verify also that we're not allocating anything by accident */
    /** @todo once MSVC 2017 support is dropped, construct all callbacks with
        NoAllocatedInit to catch these at compile time */
    #ifndef CORRADE_MSVC2017_COMPATIBILITY
    CORRADE_COMPARE((data.customDataLayer ? *customDataLayer : ui.dataLayer()).usedAllocatedCount(), 0);
    CORRADE_COMPARE(ui.eventLayer().usedAllocatedCount(), 0);
    #else
    CORRADE_COMPARE((data.customDataLayer ? *customDataLayer : ui.dataLayer()).usedAllocatedCount(), 1);
    CORRADE_COMPARE(ui.eventLayer().usedAllocatedCount(), data.operations >= StorageOperation::Toggle ? 1 : 0);
    #endif
}

void CheckboxTest::constructStorageQueryNonOwned() {
    auto&& data = ConstructStorageQueryData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Like constructStorageQuery() above, just verifying that the arguments
       are all propagated */

    DummyStorage storage = data.customDataLayer ?
        DummyStorage{*customDataLayer, data.operations, StorageFlag::ReferenceCounted} :
        DummyStorage{ui.dataLayer(), data.operations, StorageFlag::ReferenceCounted};

    Checkbox checkbox{NonOwned, Anchor{root, {}, {32, 16}}, storage, "Kick me", CheckboxStyle::RadioButton};
    CORRADE_COMPARE(ui.nodeParent(checkbox), root);
    CORRADE_COMPARE(ui.nodeOffset(checkbox), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(checkbox), (Vector2{32, 16}));
    CORRADE_VERIFY(!checkbox.isOwned());

    CORRADE_COMPARE(checkbox.style(), CheckboxStyle::RadioButton);
    CORRADE_COMPARE(ui.textLayer().glyphCount(checkbox.textData()), 7);

    /* The icon gets set during the first update but it's undetectable which
       one it actually is so just verify it got updated */
    ui.update();
    CORRADE_VERIFY(!(data.customDataLayer ? *customDataLayer : ui.dataLayer()).isDirty(checkbox.dataBindingData()));
}

void CheckboxTest::constructStorageQueryStateless() {
    auto&& data = ConstructStorageQueryData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    DummyStorage storage = data.customDataLayer ?
        DummyStorage{*customDataLayer, data.operations, StorageFlag::ReferenceCounted} :
        DummyStorage{ui.dataLayer(), data.operations, StorageFlag::ReferenceCounted};

    NodeHandle node1 = checkbox(Anchor{root, {}, {32, 16}}, storage, "Toggle me", CheckboxStyle::RadioButton);
    NodeHandle node2 = radioButton(Anchor{root, {}, {16, 32}}, storage, "Punch me");
    CORRADE_COMPARE(ui.nodeParent(node1), root);
    CORRADE_COMPARE(ui.nodeParent(node2), root);
    CORRADE_COMPARE(ui.nodeOffset(node1), Vector2{});
    CORRADE_COMPARE(ui.nodeOffset(node2), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(node1), (Vector2{32, 16}));
    CORRADE_COMPARE(ui.nodeSize(node2), (Vector2{16, 32}));

    /* Internally it should create a non-owned checkbox, if it doesn't then
       this would remove everything */
    ui.clean();

    /* Can only verify that the data were created, nothing else. Count of
       allocations is tested sufficiently in constructStorageQuery() where this
       delegates to. */
    CORRADE_COMPARE(ui.baseLayer().usedCount(), 2*1);
    CORRADE_COMPARE(ui.textLayer().usedCount(), 2*2);
    CORRADE_COMPARE(ui.layoutLayer().usedCount(), 2*1);
    CORRADE_COMPARE(ui.eventLayer().usedCount(),
        data.operations >= StorageOperation::Toggle ? 2*1 : 2*0);
    CORRADE_COMPARE((data.customDataLayer ? *customDataLayer : ui.dataLayer()).usedCount(), 2*1);
}

void CheckboxTest::constructStorageQueryTextProperties() {
    auto&& data = ConstructStorageQueryData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Like constructStorageQuery() above, just verifying that the arguments
       are all propagated */

    DummyStorage storage = data.customDataLayer ?
        DummyStorage{*customDataLayer, data.operations, StorageFlag::ReferenceCounted} :
        DummyStorage{ui.dataLayer(), data.operations, StorageFlag::ReferenceCounted};

    Checkbox checkbox{Anchor{root, {}, {32, 16}}, storage, "Kick me",
        TextProperties{}.setScript(Text::Script::Braille), CheckboxStyle::RadioButton};
    CORRADE_COMPARE(ui.nodeParent(checkbox), root);
    CORRADE_COMPARE(ui.nodeOffset(checkbox), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(checkbox), (Vector2{32, 16}));
    CORRADE_VERIFY(checkbox.isOwned());

    CORRADE_COMPARE(checkbox.style(), CheckboxStyle::RadioButton);
    /* Multiplied by 6 because of the Braille script */
    CORRADE_COMPARE(ui.textLayer().glyphCount(checkbox.textData()), 6*7);

    /* Can only verify that the ither data were created, they're not saved. No
       event handlers are used if the query isn't toggleable. */
    CORRADE_COMPARE(ui.layoutLayer().usedCount(), 1);
    CORRADE_COMPARE(ui.eventLayer().usedCount(),
        data.operations >= StorageOperation::Toggle ? 1 : 0);
}

void CheckboxTest::constructStorageQueryTextPropertiesNonOwned() {
    auto&& data = ConstructStorageQueryData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Like constructStorageQueryTextProperties() above, just verifying that
       the arguments are all propagated */

    DummyStorage storage = data.customDataLayer ?
        DummyStorage{*customDataLayer, data.operations, StorageFlag::ReferenceCounted} :
        DummyStorage{ui.dataLayer(), data.operations, StorageFlag::ReferenceCounted};

    Checkbox checkbox{NonOwned, Anchor{root, {}, {32, 16}}, storage, "Kick me",
        TextProperties{}.setScript(Text::Script::Braille), CheckboxStyle::RadioButton};
    CORRADE_COMPARE(ui.nodeParent(checkbox), root);
    CORRADE_COMPARE(ui.nodeOffset(checkbox), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(checkbox), (Vector2{32, 16}));
    CORRADE_VERIFY(!checkbox.isOwned());

    CORRADE_COMPARE(checkbox.style(), CheckboxStyle::RadioButton);
    /* Multiplied by 6 because of the Braille script */
    CORRADE_COMPARE(ui.textLayer().glyphCount(checkbox.textData()), 6*7);

    /* Can only verify that the ither data were created, they're not saved. No
       event handlers are used if the query isn't toggleable. */
    CORRADE_COMPARE(ui.layoutLayer().usedCount(), 1);
    CORRADE_COMPARE(ui.eventLayer().usedCount(),
        data.operations >= StorageOperation::Toggle ? 1 : 0);
}

void CheckboxTest::constructStorageQueryTextPropertiesStateless() {
    auto&& data = ConstructStorageQueryData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Like constructStorageQueryStateless() above, just verifying that the
       arguments are all propagated */

    DummyStorage storage = data.customDataLayer ?
        DummyStorage{*customDataLayer, data.operations, StorageFlag::ReferenceCounted} :
        DummyStorage{ui.dataLayer(), data.operations, StorageFlag::ReferenceCounted};

    NodeHandle node1 = checkbox(Anchor{root, {}, {32, 16}}, storage, "Toggle me",
        TextProperties{}.setScript(Text::Script::Braille), CheckboxStyle::RadioButton);
    NodeHandle node2 = radioButton(Anchor{root, {}, {16, 32}}, storage, "Punch me",
        TextProperties{}.setScript(Text::Script::Braille));
    CORRADE_COMPARE(ui.nodeParent(node1), root);
    CORRADE_COMPARE(ui.nodeParent(node2), root);
    CORRADE_COMPARE(ui.nodeOffset(node1), Vector2{});
    CORRADE_COMPARE(ui.nodeOffset(node2), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(node1), (Vector2{32, 16}));
    CORRADE_COMPARE(ui.nodeSize(node2), (Vector2{16, 32}));

    /* Internally it should create a non-owned checkbox, if it doesn't then
       this would remove everything */
    ui.clean();

    /* Can only verify that the data were created, nothing else. Count of
       allocations is tested sufficiently in constructStorageQuery() where this
       delegates to. */
    /** @todo this doesn't verify that the properties were passed :/ */
    CORRADE_COMPARE(ui.baseLayer().usedCount(), 2*1);
    CORRADE_COMPARE(ui.textLayer().usedCount(), 2*2);
    CORRADE_COMPARE(ui.layoutLayer().usedCount(), 2*1);
    CORRADE_COMPARE(ui.eventLayer().usedCount(),
        data.operations >= StorageOperation::Toggle ? 2*1 : 2*0);
    CORRADE_COMPARE((data.customDataLayer ? *customDataLayer : ui.dataLayer()).usedCount(), 2*1);
}

void CheckboxTest::constructStorageQueryInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct DummyStorage: AbstractStorage {
        explicit DummyStorage(DataLayer& layer): AbstractStorage{layer, StorageFlag::ReferenceCounted} {}
    } storage{ui.dataLayer()};

    StorageQuery<bool> queryNoOperation{storage, StorageOperations{}, [](const DummyStorage&, StorageOperation) -> bool {
        CORRADE_INTERNAL_ASSERT_UNREACHABLE();
    }, [](const DummyStorage&, StorageOperation, const bool*) -> StorageUpdateState {
        CORRADE_INTERNAL_ASSERT_UNREACHABLE();
    }};
    StorageQuery<bool> queryNoSet{storage, ~StorageOperation::Set, [](const DummyStorage&, StorageOperation) -> bool {
        CORRADE_INTERNAL_ASSERT_UNREACHABLE();
    }, [](const DummyStorage&, StorageOperation, const bool*) -> StorageUpdateState {
        CORRADE_INTERNAL_ASSERT_UNREACHABLE();
    }};
    StorageQuery<bool> queryNoToggle{storage, ~StorageOperation::Toggle, [](const DummyStorage&, StorageOperation) -> bool {
        CORRADE_INTERNAL_ASSERT_UNREACHABLE();
    }, [](const DummyStorage&, StorageOperation, const bool*) -> StorageUpdateState {
        CORRADE_INTERNAL_ASSERT_UNREACHABLE();
    }};

    Containers::String out;
    Error redirectError{&out};
    Checkbox{Anchor{root, {}, {}}, queryNoOperation, ""};
    Checkbox{Anchor{root, {}, {}}, queryNoSet, ""};
    Checkbox{Anchor{root, {}, {}}, queryNoToggle, ""};
    CORRADE_COMPARE_AS(out,
        "Ui::Checkbox: Ui::StorageOperation::Set|Ui::StorageOperation::Toggle not supported for a mutable query\n"
        "Ui::Checkbox: Ui::StorageOperation::Set not supported for a mutable query\n"
        "Ui::Checkbox: Ui::StorageOperation::Toggle not supported for a mutable query\n",
        TestSuite::Compare::String);
}

void CheckboxTest::constructNoCreate() {
    Checkbox checkbox{NoCreate};
    CORRADE_COMPARE(checkbox.node(), NodeHandle::Null);
    CORRADE_COMPARE(checkbox.backgroundData(), DataHandle::Null);
    CORRADE_COMPARE(checkbox.checkboxData(), DataHandle::Null);
    CORRADE_COMPARE(checkbox.textData(), DataHandle::Null);
    CORRADE_COMPARE(checkbox.dataBindingData(), DataHandle::Null);
}

void CheckboxTest::setStyle() {
    /* It's a checkbox by default */
    Checkbox checkbox{Anchor{root, {}, {32, 16}}, "Slap me!"};
    CORRADE_COMPARE(checkbox.style(), CheckboxStyle::Checkbox);

    /* Only the background is dependent on the style, the text and icon not */
    CORRADE_COMPARE(ui.baseLayer().style(checkbox.backgroundData()), UnsignedInt(BaseStyle::Checkbox));

    /* The style change should result in different layer style being used. The
       icon glyph changes too, but we cannot verify that here, only in
       CheckboxGLTest. */
    checkbox.setStyle(CheckboxStyle::RadioButton);
    CORRADE_COMPARE(checkbox.style(), CheckboxStyle::RadioButton);
    CORRADE_COMPARE(ui.baseLayer().style(checkbox.backgroundData()), UnsignedInt(BaseStyle::RadioButton));
}

void CheckboxTest::setStyleWhileActive() {
    Checkbox checkbox{Anchor{root, {}, {32, 16}}, "Punch me", CheckboxStyle::RadioButton};
    CORRADE_COMPARE(checkbox.style(), CheckboxStyle::RadioButton);

    CORRADE_COMPARE(ui.baseLayer().style(checkbox.backgroundData()), UnsignedInt(BaseStyle::RadioButton));
    CORRADE_COMPARE(ui.textLayer().style(checkbox.checkboxData()), UnsignedInt(TextStyle::Checkbox));
    CORRADE_COMPARE(ui.textLayer().style(checkbox.textData()), UnsignedInt(TextStyle::CheckboxLabel));

    PointerEvent pressEvent{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0, {}};
    CORRADE_VERIFY(ui.pointerPressEvent({16, 8}, pressEvent));
    CORRADE_COMPARE(ui.currentPressedNode(), checkbox);

    /* Verify that style transition works */
    CORRADE_COMPARE(ui.baseLayer().style(checkbox.backgroundData()), UnsignedInt(BaseStyle::RadioButtonPressed));
    CORRADE_COMPARE(ui.textLayer().style(checkbox.checkboxData()), UnsignedInt(TextStyle::CheckboxPressed));
    CORRADE_COMPARE(ui.textLayer().style(checkbox.textData()), UnsignedInt(TextStyle::CheckboxLabelPressed));

    checkbox.setStyle(CheckboxStyle::Checkbox);
    CORRADE_COMPARE(checkbox.style(), CheckboxStyle::Checkbox);

    /* Style should now be changed in a way that preserves the current pressed
       state */
    CORRADE_COMPARE(ui.baseLayer().style(checkbox.backgroundData()), UnsignedInt(BaseStyle::CheckboxPressed));
    /* The checkbox and label style is the same for both Checkbox and
       RadioButton */
    CORRADE_COMPARE(ui.textLayer().style(checkbox.checkboxData()), UnsignedInt(TextStyle::CheckboxPressed));
    CORRADE_COMPARE(ui.textLayer().style(checkbox.textData()), UnsignedInt(TextStyle::CheckboxLabelPressed));
}

void CheckboxTest::setText() {
    Checkbox checkbox{Anchor{root, {}, {32, 16}}, "Toggle me"};
    CORRADE_COMPARE(ui.textLayer().glyphCount(checkbox.textData()), 9);

    checkbox.setText("Slap me");
    CORRADE_COMPARE(ui.textLayer().glyphCount(checkbox.textData()), 7);
}

void CheckboxTest::setTextTextProperties() {
    Checkbox checkbox{Anchor{root, {}, {32, 16}}, "Toggle me"};
    CORRADE_COMPARE(ui.textLayer().glyphCount(checkbox.textData()), 9);

    checkbox.setText("Slap me",
        TextProperties{}.setScript(Text::Script::Braille));
    /* Multiplied by 6 because of the Braille script */
    CORRADE_COMPARE(ui.textLayer().glyphCount(checkbox.textData()), 7*6);
}

void CheckboxTest::editSet() {
    auto&& data = EditData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    const Int immutableValue = 1;
    EnumStorage<Int> storage = data.customDataLayer ?
        EnumStorage<Int>{*customDataLayer, StorageFlag::ReferenceCounted} :
        data.immutable ?
            EnumStorage<Int>{ui.dataLayer(), NonOwned, immutableValue, StorageFlag::ReferenceCounted} :
            EnumStorage<Int>{ui.dataLayer(), StorageFlag::ReferenceCounted};
    storage.setEnumSet(!data.singleChoice);

    Checkbox checkbox = data.builtinStorage ?
        Checkbox{Anchor{root, {}, {32, 16}}, {}} :
        Checkbox{Anchor{root, {}, {32, 16}}, storage.value<1>(), {}};
    /* Immutable storage is checked by default */
    CORRADE_COMPARE(checkbox.isChecked(), data.immutable ? true : false);

    /* The setter does nothing for an immutable storage, i.e. it stays
       checked as before */
    checkbox.setChecked(true);
    CORRADE_VERIFY(checkbox.isChecked());
    if(!data.builtinStorage)
        CORRADE_COMPARE(storage.value(), 1);

    /* Single-choice (radio button) cannot be made unchecked, on the immutable
       it also has no effect  */
    checkbox.setChecked(false);
    CORRADE_COMPARE(checkbox.isChecked(), data.singleChoice || data.immutable ? true : false);
    if(!data.builtinStorage)
        CORRADE_COMPARE(storage.value(), data.singleChoice || data.immutable ? 1 : 0);
}

void CheckboxTest::editToggle() {
    auto&& data = EditData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Same as editSet() above, just using toggleChecked() instead of
       setChecked() */

    const Int immutableValue = 1;
    EnumStorage<Int> storage = data.customDataLayer ?
        EnumStorage<Int>{*customDataLayer, StorageFlag::ReferenceCounted} :
        data.immutable ?
            EnumStorage<Int>{ui.dataLayer(), NonOwned, immutableValue, StorageFlag::ReferenceCounted} :
            EnumStorage<Int>{ui.dataLayer(), StorageFlag::ReferenceCounted};
    storage.setEnumSet(!data.singleChoice);

    Checkbox checkbox = data.builtinStorage ?
        Checkbox{Anchor{root, {}, {32, 16}}, {}} :
        Checkbox{Anchor{root, {}, {32, 16}}, storage.value<1>(), {}};
    CORRADE_COMPARE(checkbox.isChecked(), data.immutable ? true : false);

    checkbox.toggleChecked();
    CORRADE_VERIFY(checkbox.isChecked());
    if(!data.builtinStorage)
        CORRADE_COMPARE(storage.value(), 1);

    checkbox.toggleChecked();
    CORRADE_COMPARE(checkbox.isChecked(), data.singleChoice || data.immutable ? true : false);
    if(!data.builtinStorage)
        CORRADE_COMPARE(storage.value(), data.singleChoice || data.immutable ? 1 : 0);
}

void CheckboxTest::editToggleEvent() {
    auto&& data = EditData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    /* Like editToggle() above, just using an event instead of a direct API
       call */

    const Int immutableValue = 1;
    EnumStorage<Int> storage = data.customDataLayer ?
        EnumStorage<Int>{*customDataLayer, StorageFlag::ReferenceCounted} :
        data.immutable ?
            EnumStorage<Int>{ui.dataLayer(), NonOwned, immutableValue, StorageFlag::ReferenceCounted} :
            EnumStorage<Int>{ui.dataLayer(), StorageFlag::ReferenceCounted};
    storage.setEnumSet(!data.singleChoice);

    Checkbox checkbox = data.builtinStorage ?
        Checkbox{Anchor{root, {}, {32, 16}}, {}} :
        Checkbox{Anchor{root, {}, {32, 16}}, storage.value<1>(), {}};
    CORRADE_COMPARE(checkbox.isChecked(), data.immutable ? true : false);

    {
        PointerEvent press{{}, PointerEventSource::Pen, Pointer::Pen, true, 0, {}};
        PointerEvent release{{}, PointerEventSource::Pen, Pointer::Pen, true, 0, {}};
        CORRADE_VERIFY(ui.pointerPressEvent({16, 8}, press));
        CORRADE_VERIFY(ui.pointerReleaseEvent({16, 8}, release));
    }
    CORRADE_VERIFY(checkbox.isChecked());
    if(!data.builtinStorage)
        CORRADE_COMPARE(storage.value(), 1);

    {
        PointerEvent press{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0, {}};
        PointerEvent release{{}, PointerEventSource::Mouse, Pointer::MouseLeft, true, 0, {}};
        CORRADE_VERIFY(ui.pointerPressEvent({16, 8}, press));
        CORRADE_VERIFY(ui.pointerReleaseEvent({16, 8}, release));
    }
    CORRADE_COMPARE(checkbox.isChecked(), data.singleChoice || data.immutable ? true : false);
    if(!data.builtinStorage)
        CORRADE_COMPARE(storage.value(), data.singleChoice || data.immutable ? 1 : 0);
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::CheckboxTest)
