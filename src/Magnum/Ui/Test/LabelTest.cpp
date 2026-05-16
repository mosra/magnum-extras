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
#include <Magnum/Math/Time.h>

#include "Magnum/Ui/Anchor.h"
#include "Magnum/Ui/Formatter.h"
#include "Magnum/Ui/Icon.h"
#include "Magnum/Ui/Label.h"
#include "Magnum/Ui/Storage.h"
#include "Magnum/Ui/TextProperties.h"
#include "Magnum/Ui/Test/WidgetTester.hpp"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct LabelTest: WidgetTester {
    explicit LabelTest();

    void debugStyle();

    void constructEmpty();
    /* No constructEmptyNonOwned() as it's just a special case of the others
       and the other *NonOwned() variants test it sufficiently */
    void constructEmptyStateless();
    void constructIcon();
    void constructIconNonOwned();
    void constructIconStateless();
    void constructText();
    void constructTextNonOwned();
    void constructTextStateless();
    void constructTextTextProperties();
    void constructTextTextPropertiesNonOwned();
    void constructTextTextPropertiesStateless();
    template<class T> void constructStorageQuery();
    template<class T> void constructStorageQueryNonOwned();
    template<class T> void constructStorageQueryStateless();
    template<class T, class Formatter> void constructStorageQueryFormatter();
    template<class T, class Formatter> void constructStorageQueryFormatterNonOwned();
    template<class T, class Formatter> void constructStorageQueryFormatterStateless();
    void constructNoCreate();

    void setStyle();

    void setIcon();
    void setIconFromText();
    void setIconFromEmpty();
    void setIconEmpty();
    void setIconEmptyFromText();

    void setText();
    void setTextTextProperties();
    void setTextFromIcon();
    void setTextFromEmpty();
    void setTextEmpty();
    void setTextEmptyFromIcon();

    void setIconTextInvalid();
};

using namespace Math::Literals;

const struct {
    const char* name;
    bool customDataLayer;
} ConstructStorageQueryData[]{
    {"", false},
    {"custom data layer", true}
};

const struct {
    const char* name;
    bool timeOverload;
    Icon icon;
    const char* text;
    LabelStyle originalStyle, changedStyle;
    bool expectLayoutStyleChange;
} SetStyleData[]{
    {"empty",
        false, Icon::None, nullptr,
        LabelStyle::Dim, LabelStyle::Success, false},
    {"icon",
        false, Icon::No, nullptr,
        LabelStyle::Dim, LabelStyle::Success, false},
    {"text",
        false, Icon::None, "hello",
        LabelStyle::Dim, LabelStyle::Success, false},
    {"text, from title style",
        false, Icon::None, "hello",
        LabelStyle::Title, LabelStyle::Success, true},
    {"text, to title style",
        false, Icon::None, "hello",
        LabelStyle::Dim, LabelStyle::Title, true},
    {"time overload, empty",
        true, Icon::None, nullptr,
        LabelStyle::Dim, LabelStyle::Success, false},
    {"time overload, icon",
        true, Icon::No, nullptr,
        LabelStyle::Dim, LabelStyle::Success, false},
    {"time overload, text",
        true, Icon::None, "hello",
        LabelStyle::Dim, LabelStyle::Success, false},
    {"time overload, text, from title style",
        true, Icon::None, "hello",
        LabelStyle::Title, LabelStyle::Success, true},
    {"time overload, text, to title style",
        true, Icon::None, "hello",
        LabelStyle::Dim, LabelStyle::Title, true},
};

LabelTest::LabelTest() {
    addTests({&LabelTest::debugStyle});

    addTests<LabelTest>({
        &LabelTest::constructEmpty,
        &LabelTest::constructEmptyStateless,
        &LabelTest::constructIcon,
        &LabelTest::constructIconNonOwned,
        &LabelTest::constructIconStateless,
        &LabelTest::constructText,
        &LabelTest::constructTextNonOwned,
        &LabelTest::constructTextStateless,
        &LabelTest::constructTextTextProperties,
        &LabelTest::constructTextTextPropertiesNonOwned,
        &LabelTest::constructTextTextPropertiesStateless
    }, &WidgetTester::setup,
       &WidgetTester::teardown);

    addInstancedTests<LabelTest>({
        &LabelTest::constructStorageQuery<Int>,
        &LabelTest::constructStorageQuery<UnsignedInt>,
        &LabelTest::constructStorageQuery<Long>,
        &LabelTest::constructStorageQuery<UnsignedLong>,
        &LabelTest::constructStorageQuery<Float>,
        &LabelTest::constructStorageQuery<Double>,
        &LabelTest::constructStorageQuery<Containers::StringView>,
        &LabelTest::constructStorageQuery<Icon>,
        &LabelTest::constructStorageQueryNonOwned<Int>,
        &LabelTest::constructStorageQueryNonOwned<UnsignedInt>,
        &LabelTest::constructStorageQueryNonOwned<Long>,
        &LabelTest::constructStorageQueryNonOwned<UnsignedLong>,
        &LabelTest::constructStorageQueryNonOwned<Float>,
        &LabelTest::constructStorageQueryNonOwned<Double>,
        &LabelTest::constructStorageQueryNonOwned<Containers::StringView>,
        &LabelTest::constructStorageQueryNonOwned<Icon>,
        &LabelTest::constructStorageQueryStateless<Int>,
        &LabelTest::constructStorageQueryStateless<UnsignedInt>,
        &LabelTest::constructStorageQueryStateless<Long>,
        &LabelTest::constructStorageQueryStateless<UnsignedLong>,
        &LabelTest::constructStorageQueryStateless<Float>,
        &LabelTest::constructStorageQueryStateless<Double>,
        &LabelTest::constructStorageQueryStateless<Containers::StringView>,
        &LabelTest::constructStorageQueryStateless<Icon>,
        &LabelTest::constructStorageQueryFormatter<Int, DecimalFormatter>,
        &LabelTest::constructStorageQueryFormatter<UnsignedInt, DecimalFormatter>,
        &LabelTest::constructStorageQueryFormatter<Long, DecimalFormatter>,
        &LabelTest::constructStorageQueryFormatter<UnsignedLong, DecimalFormatter>,
        &LabelTest::constructStorageQueryFormatter<Int, HexadecimalFormatter>,
        &LabelTest::constructStorageQueryFormatter<UnsignedInt, HexadecimalFormatter>,
        &LabelTest::constructStorageQueryFormatter<Long, HexadecimalFormatter>,
        &LabelTest::constructStorageQueryFormatter<UnsignedLong, HexadecimalFormatter>,
        &LabelTest::constructStorageQueryFormatter<Float, FloatFormatter>,
        &LabelTest::constructStorageQueryFormatter<Double, FloatFormatter>,
        &LabelTest::constructStorageQueryFormatterNonOwned<Int, DecimalFormatter>,
        &LabelTest::constructStorageQueryFormatterNonOwned<UnsignedInt, DecimalFormatter>,
        &LabelTest::constructStorageQueryFormatterNonOwned<Long, DecimalFormatter>,
        &LabelTest::constructStorageQueryFormatterNonOwned<UnsignedLong, DecimalFormatter>,
        &LabelTest::constructStorageQueryFormatterNonOwned<Int, HexadecimalFormatter>,
        &LabelTest::constructStorageQueryFormatterNonOwned<UnsignedInt, HexadecimalFormatter>,
        &LabelTest::constructStorageQueryFormatterNonOwned<Long, HexadecimalFormatter>,
        &LabelTest::constructStorageQueryFormatterNonOwned<UnsignedLong, HexadecimalFormatter>,
        &LabelTest::constructStorageQueryFormatterNonOwned<Float, FloatFormatter>,
        &LabelTest::constructStorageQueryFormatterNonOwned<Double, FloatFormatter>,
        &LabelTest::constructStorageQueryFormatterStateless<Int, DecimalFormatter>,
        &LabelTest::constructStorageQueryFormatterStateless<UnsignedInt, DecimalFormatter>,
        &LabelTest::constructStorageQueryFormatterStateless<Long, DecimalFormatter>,
        &LabelTest::constructStorageQueryFormatterStateless<UnsignedLong, DecimalFormatter>,
        &LabelTest::constructStorageQueryFormatterStateless<Int, HexadecimalFormatter>,
        &LabelTest::constructStorageQueryFormatterStateless<UnsignedInt, HexadecimalFormatter>,
        &LabelTest::constructStorageQueryFormatterStateless<Long, HexadecimalFormatter>,
        &LabelTest::constructStorageQueryFormatterStateless<UnsignedLong, HexadecimalFormatter>,
        &LabelTest::constructStorageQueryFormatterStateless<Float, FloatFormatter>,
        &LabelTest::constructStorageQueryFormatterStateless<Double, FloatFormatter>,
    }, Containers::arraySize(ConstructStorageQueryData),
       &WidgetTester::setup,
       &WidgetTester::teardown);

    addTests<LabelTest>({&LabelTest::constructNoCreate},
        &WidgetTester::setupNoCreate,
        &WidgetTester::teardownNoCreate);

    addInstancedTests<LabelTest>({&LabelTest::setStyle},
        Containers::arraySize(SetStyleData),
        &WidgetTester::setup,
        &WidgetTester::teardown);

    addTests<LabelTest>({
        &LabelTest::setIcon,
        &LabelTest::setIconFromText,
        &LabelTest::setIconFromEmpty,
        &LabelTest::setIconEmpty,
        &LabelTest::setIconEmptyFromText,

        &LabelTest::setText,
        &LabelTest::setTextTextProperties,
        &LabelTest::setTextFromIcon,
        &LabelTest::setTextFromEmpty,
        &LabelTest::setTextEmpty,
        &LabelTest::setTextEmptyFromIcon,

        &LabelTest::setIconTextInvalid
    }, &WidgetTester::setup,
       &WidgetTester::teardown);
}

void LabelTest::debugStyle() {
    Containers::String out;
    Debug{&out} << LabelStyle::Success << LabelStyle(0xef);
    CORRADE_COMPARE(out, "Ui::LabelStyle::Success Ui::LabelStyle(0xef)\n");
}

void LabelTest::constructEmpty() {
    Label label1{Anchor{root, {}, {32, 16}}, Icon::None, LabelStyle::Success};
    Label label2{Anchor{root, {}, {32, 16}}, "", LabelStyle::Success};
    CORRADE_COMPARE(ui.nodeParent(label1), root);
    CORRADE_COMPARE(ui.nodeParent(label2), root);
    CORRADE_COMPARE(ui.nodeSize(label1), (Vector2{32, 16}));
    CORRADE_COMPARE(ui.nodeSize(label2), (Vector2{32, 16}));
    CORRADE_VERIFY(label1.isOwned());
    CORRADE_VERIFY(label2.isOwned());

    CORRADE_VERIFY(!label1.hasDataBinding());
    CORRADE_VERIFY(!label2.hasDataBinding());
    CORRADE_COMPARE(label1.style(), LabelStyle::Success);
    CORRADE_COMPARE(label2.style(), LabelStyle::Success);
    CORRADE_COMPARE(label1.icon(), Icon::None);
    CORRADE_COMPARE(label2.icon(), Icon::None);
    CORRADE_COMPARE(label1.textData(), DataHandle::Null);
    CORRADE_COMPARE(label2.textData(), DataHandle::Null);
    CORRADE_VERIFY(ui.isHandleValid(label1.layoutData()));
    CORRADE_VERIFY(ui.isHandleValid(label2.layoutData()));
    CORRADE_COMPARE(label1.dataBindingData(), DataHandle::Null);
    CORRADE_COMPARE(label2.dataBindingData(), DataHandle::Null);
}

void LabelTest::constructEmptyStateless() {
    NodeHandle node1 = label(Anchor{root, {}, {32, 16}}, Icon::None, LabelStyle::Success);
    NodeHandle node2 = label(Anchor{root, {}, {32, 16}}, "", LabelStyle::Success);
    CORRADE_COMPARE(ui.nodeParent(node1), root);
    CORRADE_COMPARE(ui.nodeParent(node2), root);
    CORRADE_COMPARE(ui.nodeSize(node1), (Vector2{32, 16}));
    CORRADE_COMPARE(ui.nodeSize(node2), (Vector2{32, 16}));

    /* Internally it should create a non-owned label, if it doesn't then this
       would remove everything */
    ui.clean();

    /* Can only verify that the data were (not) created, nothing else. Visually
       tested in LabelGLTest. */
    CORRADE_COMPARE(ui.baseLayer().usedCount(), 0);
    CORRADE_COMPARE(ui.textLayer().usedCount(), 0);
    CORRADE_COMPARE(ui.layoutLayer().usedCount(), 2);
    CORRADE_COMPARE(ui.dataLayer().usedCount(), 0);
}

void LabelTest::constructIcon() {
    Label label{Anchor{root, {}, {32, 16}}, Icon::Yes, LabelStyle::Warning};
    CORRADE_COMPARE(ui.nodeParent(label), root);
    CORRADE_COMPARE(ui.nodeSize(label), (Vector2{32, 16}));
    CORRADE_VERIFY(label.isOwned());

    CORRADE_VERIFY(!label.hasDataBinding());
    CORRADE_COMPARE(label.style(), LabelStyle::Warning);
    CORRADE_COMPARE(label.icon(), Icon::Yes);

    CORRADE_VERIFY(ui.isHandleValid(label.textData()));
    CORRADE_VERIFY(ui.isHandleValid(label.layoutData()));
    CORRADE_COMPARE(label.dataBindingData(), DataHandle::Null);
    CORRADE_COMPARE(ui.textLayer().glyphCount(label.textData()), 1);
}

void LabelTest::constructIconNonOwned() {
    /* All the properties are verified in constructIcon() above, check just
       that it propagates all arguments properly */

    Label label{NonOwned, Anchor{root, {}, {32, 16}}, Icon::Yes, LabelStyle::Warning};
    CORRADE_COMPARE(ui.nodeParent(label), root);
    CORRADE_COMPARE(ui.nodeSize(label), (Vector2{32, 16}));
    CORRADE_VERIFY(!label.isOwned());

    CORRADE_COMPARE(label.style(), LabelStyle::Warning);
    CORRADE_COMPARE(label.icon(), Icon::Yes);
}

void LabelTest::constructIconStateless() {
    NodeHandle node = label(Anchor{root, {}, {32, 16}}, Icon::Yes, LabelStyle::Success);
    CORRADE_COMPARE(ui.nodeParent(node), root);
    CORRADE_COMPARE(ui.nodeSize(node), (Vector2{32, 16}));

    /* Internally it should create a non-owned label, if it doesn't then this
       would remove everything */
    ui.clean();

    /* Can only verify that the data were created, nothing else. Visually
       tested in LabelGLTest. */
    CORRADE_COMPARE(ui.baseLayer().usedCount(), 0);
    CORRADE_COMPARE(ui.textLayer().usedCount(), 1);
    CORRADE_COMPARE(ui.layoutLayer().usedCount(), 1);
    CORRADE_COMPARE(ui.dataLayer().usedCount(), 0);
}

void LabelTest::constructText() {
    Label label{Anchor{root, {}, {32, 16}}, "hello!", LabelStyle::Danger};
    CORRADE_COMPARE(ui.nodeParent(label), root);
    CORRADE_COMPARE(ui.nodeOffset(label), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(label), (Vector2{32, 16}));
    CORRADE_VERIFY(label.isOwned());

    CORRADE_VERIFY(!label.hasDataBinding());
    CORRADE_COMPARE(label.style(), LabelStyle::Danger);
    CORRADE_COMPARE(label.icon(), Icon::None);

    CORRADE_VERIFY(ui.isHandleValid(label.textData()));
    CORRADE_VERIFY(ui.isHandleValid(label.layoutData()));
    CORRADE_COMPARE(label.dataBindingData(), DataHandle::Null);
    CORRADE_COMPARE(ui.textLayer().glyphCount(label.textData()), 6);
}

void LabelTest::constructTextNonOwned() {
    /* All the properties are verified in constructText() above, check just
       that it propagates all arguments properly */

    Label label{NonOwned, Anchor{root, {}, {32, 16}}, "hello!", LabelStyle::Danger};
    CORRADE_COMPARE(ui.nodeParent(label), root);
    CORRADE_COMPARE(ui.nodeOffset(label), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(label), (Vector2{32, 16}));
    CORRADE_VERIFY(!label.isOwned());

    CORRADE_COMPARE(label.style(), LabelStyle::Danger);
    CORRADE_COMPARE(ui.textLayer().glyphCount(label.textData()), 6);
}

void LabelTest::constructTextStateless() {
    NodeHandle node = label(Anchor{root, {}, {32, 16}}, "hello!", LabelStyle::Warning);
    CORRADE_COMPARE(ui.nodeParent(node), root);
    CORRADE_COMPARE(ui.nodeOffset(node), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(node), (Vector2{32, 16}));

    /* Internally it should create a non-owned label, if it doesn't then this
       would remove everything */
    ui.clean();

    /* Can only verify that the data were created, nothing else. Visually
       tested in LabelGLTest. */
    CORRADE_COMPARE(ui.baseLayer().usedCount(), 0);
    CORRADE_COMPARE(ui.textLayer().usedCount(), 1);
    CORRADE_COMPARE(ui.layoutLayer().usedCount(), 1);
    CORRADE_COMPARE(ui.dataLayer().usedCount(), 0);
}

void LabelTest::constructTextTextProperties() {
    Label label{Anchor{root, {}, {32, 16}}, "hello!",
        TextProperties{}.setScript(Text::Script::Braille),
        LabelStyle::Info};
    CORRADE_COMPARE(ui.nodeParent(label), root);
    CORRADE_COMPARE(ui.nodeOffset(label), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(label), (Vector2{32, 16}));
    CORRADE_VERIFY(label.isOwned());

    CORRADE_VERIFY(!label.hasDataBinding());
    CORRADE_COMPARE(label.style(), LabelStyle::Info);
    CORRADE_COMPARE(label.icon(), Icon::None);

    CORRADE_VERIFY(ui.isHandleValid(label.textData()));
    CORRADE_VERIFY(ui.isHandleValid(label.layoutData()));
    CORRADE_COMPARE(label.dataBindingData(), DataHandle::Null);
    /* Multiplied by 6 because of the Braille script */
    CORRADE_COMPARE(ui.textLayer().glyphCount(label.textData()), 6*6);
}

void LabelTest::constructTextTextPropertiesNonOwned() {
    /* All the properties are verified in constructTextTextProperties() above,
       check just that it propagates all arguments properly */

    Label label{NonOwned, Anchor{root, {}, {32, 16}}, "hello!",
        TextProperties{}.setScript(Text::Script::Braille),
        LabelStyle::Info};
    CORRADE_COMPARE(ui.nodeParent(label), root);
    CORRADE_COMPARE(ui.nodeOffset(label), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(label), (Vector2{32, 16}));
    CORRADE_VERIFY(!label.isOwned());

    CORRADE_COMPARE(label.style(), LabelStyle::Info);
    /* Multiplied by 6 because of the Braille script */
    CORRADE_COMPARE(ui.textLayer().glyphCount(label.textData()), 6*6);
}

void LabelTest::constructTextTextPropertiesStateless() {
    NodeHandle node = label(Anchor{root, {}, {32, 16}}, "hello!",
        TextProperties{}.setScript(Text::Script::Braille),
        LabelStyle::Primary);
    CORRADE_COMPARE(ui.nodeParent(node), root);
    CORRADE_COMPARE(ui.nodeOffset(node), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(node), (Vector2{32, 16}));

    /* Internally it should create a non-owned label, if it doesn't then this
       would remove everything */
    ui.clean();

    /* Can only verify that the data were created, nothing else. Visually
       tested in LabelGLTest. */
    /** @todo this doesn't verify that the properties were passed :/ */
    CORRADE_COMPARE(ui.baseLayer().usedCount(), 0);
    CORRADE_COMPARE(ui.textLayer().usedCount(), 1);
    CORRADE_COMPARE(ui.layoutLayer().usedCount(), 1);
    CORRADE_COMPARE(ui.dataLayer().usedCount(), 0);
}

template<class T> struct StorageQueryTraits {
    static const char* name() { return Math::TypeTraits<T>::name(); }
    static T value() { return 133742; }
    static const char* expected() { return "133742"; }
};
template<> struct StorageQueryTraits<Containers::StringView> {
    static const char* name() { return "Containers::StringView"; }
    static Containers::StringView value() { return "hello?"; }
    static const char* expected() { return "hello?"; }
};
template<> struct StorageQueryTraits<Icon> {
    static const char* name() { return "Icon"; }
    static Icon value() { return Icon::No; }
    static const char* expected() { return nullptr; }
};
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

template<class T> void LabelTest::constructStorageQuery() {
    auto&& data = ConstructStorageQueryData[testCaseInstanceId()];
    setTestCaseDescription(data.name);
    setTestCaseTemplateName(StorageQueryTraits<T>::name());

    /* Once the label is gone, the storage gets removed as well */
    T value = StorageQueryTraits<T>::value();
    Storage<T> storage = data.customDataLayer ?
        Storage<T>{*customDataLayer, value, StorageFlag::ReferenceCounted} :
        Storage<T>{ui, value, StorageFlag::ReferenceCounted};

    Label label{Anchor{root, {}, {32, 16}}, storage, LabelStyle::Danger};
    CORRADE_COMPARE(ui.nodeParent(label), root);
    CORRADE_COMPARE(ui.nodeOffset(label), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(label), (Vector2{32, 16}));
    CORRADE_VERIFY(label.isOwned());

    CORRADE_VERIFY(label.hasDataBinding());
    CORRADE_COMPARE(label.style(), LabelStyle::Danger);
    CORRADE_COMPARE(label.icon(), Icon::None);

    CORRADE_VERIFY(ui.isHandleValid(label.textData()));
    CORRADE_VERIFY(ui.isHandleValid(label.layoutData()));
    CORRADE_VERIFY(ui.isHandleValid(label.dataBindingData()));
    /* Initially there's nothing set. In case of an icon, there's a single
       empty glyph. */
    CORRADE_COMPARE(ui.textLayer().glyphCount(label.textData()),
        StorageQueryTraits<T>::expected() ? 0 : 1);
    /* The data binding should be attached to the label so it cleans up
       properly */
    CORRADE_COMPARE((data.customDataLayer ? *customDataLayer : ui.dataLayer()).node(label.dataBindingData()), label.node());

    /* Mark the text data as editable so we can subsequently query the contents
       set by the DataLayer */
    ui.textLayer().setText(label.textData(), "nothing here!", {}, TextDataFlag::Editable);
    CORRADE_COMPARE(ui.textLayer().text(label.textData()), "nothing here!");

    /* The contents are set during the first update. In case of an icon,
       there's no text, just a single glyph set instead. */
    ui.update();
    if(const char* expected = StorageQueryTraits<T>::expected())
        CORRADE_COMPARE(ui.textLayer().text(label.textData()), expected);
    else
        CORRADE_COMPARE(ui.textLayer().glyphCount(label.textData()), 1);
    /* The icon getter on the instance isn't / cannot be updated by the data
       binding */
    CORRADE_COMPARE(label.icon(), Icon::None);
}

template<class T> void LabelTest::constructStorageQueryNonOwned() {
    auto&& data = ConstructStorageQueryData[testCaseInstanceId()];
    setTestCaseDescription(data.name);
    setTestCaseTemplateName(StorageQueryTraits<T>::name());

    /* Like *NonOwned() tests above, just verifying that the arguments are all
       propagated */

    T value = StorageQueryTraits<T>::value();
    Storage<T> storage = data.customDataLayer ?
        Storage<T>{*customDataLayer, value, StorageFlag::ReferenceCounted} :
        Storage<T>{ui, value, StorageFlag::ReferenceCounted};

    Label label{NonOwned, Anchor{root, {}, {32, 16}}, storage, LabelStyle::Danger};
    CORRADE_COMPARE(ui.nodeParent(label), root);
    CORRADE_COMPARE(ui.nodeOffset(label), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(label), (Vector2{32, 16}));
    CORRADE_VERIFY(!label.isOwned());

    CORRADE_COMPARE(label.style(), LabelStyle::Danger);

    /* The contents are set during the first update */
    ui.update();
    CORRADE_COMPARE(ui.textLayer().glyphCount(label.textData()), StorageQueryTraits<T>::expected() ?
        Containers::StringView{StorageQueryTraits<T>::expected()}.size() : 1);
}

template<class T> void LabelTest::constructStorageQueryStateless() {
    auto&& data = ConstructStorageQueryData[testCaseInstanceId()];
    setTestCaseDescription(data.name);
    setTestCaseTemplateName(StorageQueryTraits<T>::name());

    T value = StorageQueryTraits<T>::value();
    Storage<T> storage = data.customDataLayer ?
        Storage<T>{*customDataLayer, value, StorageFlag::ReferenceCounted} :
        Storage<T>{ui, value, StorageFlag::ReferenceCounted};

    NodeHandle node = label(Anchor{root, {}, {32, 16}}, storage, LabelStyle::Success);
    CORRADE_COMPARE(ui.nodeParent(node), root);
    CORRADE_COMPARE(ui.nodeOffset(node), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(node), (Vector2{32, 16}));

    /* Internally it should create a non-owned label, if it doesn't then this
       would remove everything */
    ui.clean();

    /* Can only verify that the data were created, nothing else. Visually
       tested in LabelGLTest. */
    CORRADE_COMPARE(ui.baseLayer().usedCount(), 0);
    CORRADE_COMPARE(ui.textLayer().usedCount(), 1);
    CORRADE_COMPARE(ui.layoutLayer().usedCount(), 1);
    CORRADE_COMPARE((data.customDataLayer ? *customDataLayer : ui.dataLayer()).usedCount(), 1);
}

template<class T, class Formatter> void LabelTest::constructStorageQueryFormatter() {
    auto&& data = ConstructStorageQueryData[testCaseInstanceId()];
    setTestCaseDescription(data.name);
    setTestCaseTemplateName({StorageQueryTraits<T>::name(), FormatterTraits<Formatter>::name()});

    /* Like constructStorageQuery(), just additionally passing an explicit
       formatter argument */

    /* Once the label is gone, the storage gets removed as well */
    T value = StorageQueryTraits<T>::value();
    Storage<T> storage = data.customDataLayer ?
        Storage<T>{*customDataLayer, value, StorageFlag::ReferenceCounted} :
        Storage<T>{ui, value, StorageFlag::ReferenceCounted};

    Label label{Anchor{root, {}, {32, 16}}, storage,
        Formatter{Formatter::Flag::ExplicitPlus},
        LabelStyle::Warning};
    CORRADE_COMPARE(ui.nodeParent(label), root);
    CORRADE_COMPARE(ui.nodeOffset(label), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(label), (Vector2{32, 16}));
    CORRADE_VERIFY(label.isOwned());

    CORRADE_VERIFY(label.hasDataBinding());
    CORRADE_COMPARE(label.style(), LabelStyle::Warning);
    CORRADE_COMPARE(label.icon(), Icon::None);

    CORRADE_VERIFY(ui.isHandleValid(label.textData()));
    CORRADE_VERIFY(ui.isHandleValid(label.layoutData()));
    CORRADE_VERIFY(ui.isHandleValid(label.dataBindingData()));
    /* Initially there's nothing set */
    CORRADE_COMPARE(ui.textLayer().glyphCount(label.textData()), 0);
    /* The data binding should be attached to the label so it cleans up
       properly */
    CORRADE_COMPARE((data.customDataLayer ? *customDataLayer : ui.dataLayer()).node(label.dataBindingData()), label.node());

    /* Mark the text data as editable so we can subsequently query the contents
       set by the DataLayer */
    ui.textLayer().setText(label.textData(), "nothing here!", {}, TextDataFlag::Editable);
    CORRADE_COMPARE(ui.textLayer().text(label.textData()), "nothing here!");

    /* The contents are set during the first update. In case of an icon,
       there's no text, just a single glyph set instead. */
    ui.update();
    CORRADE_COMPARE(ui.textLayer().text(label.textData()), FormatterTraits<Formatter>::expected());
}

template<class T, class Formatter> void LabelTest::constructStorageQueryFormatterNonOwned() {
    auto&& data = ConstructStorageQueryData[testCaseInstanceId()];
    setTestCaseDescription(data.name);
    setTestCaseTemplateName({StorageQueryTraits<T>::name(), FormatterTraits<Formatter>::name()});

    /* Like *NonOwned() tests above, just verifying that the arguments are all
       propagated */

    T value = StorageQueryTraits<T>::value();
    Storage<T> storage = data.customDataLayer ?
        Storage<T>{*customDataLayer, value, StorageFlag::ReferenceCounted} :
        Storage<T>{ui, value, StorageFlag::ReferenceCounted};

    Label label{NonOwned, Anchor{root, {}, {32, 16}}, storage,
        Formatter{Formatter::Flag::ExplicitPlus},
        LabelStyle::Warning};
    CORRADE_COMPARE(ui.nodeParent(label), root);
    CORRADE_COMPARE(ui.nodeOffset(label), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(label), (Vector2{32, 16}));
    CORRADE_VERIFY(!label.isOwned());

    CORRADE_COMPARE(label.style(), LabelStyle::Warning);

    /* The contents are set during the first update */
    ui.update();
    CORRADE_COMPARE(ui.textLayer().glyphCount(label.textData()), Containers::StringView{FormatterTraits<Formatter>::expected()}.size());
}

template<class T, class Formatter> void LabelTest::constructStorageQueryFormatterStateless() {
    auto&& data = ConstructStorageQueryData[testCaseInstanceId()];
    setTestCaseDescription(data.name);
    setTestCaseTemplateName({StorageQueryTraits<T>::name(), FormatterTraits<Formatter>::name()});

    T value = StorageQueryTraits<T>::value();
    Storage<T> storage = data.customDataLayer ?
        Storage<T>{*customDataLayer, value, StorageFlag::ReferenceCounted} :
        Storage<T>{ui, value, StorageFlag::ReferenceCounted};

    NodeHandle node = label(Anchor{root, {}, {32, 16}}, storage,
        Formatter{Formatter::Flag::ExplicitPlus},
        LabelStyle::Dim);
    CORRADE_COMPARE(ui.nodeParent(node), root);
    CORRADE_COMPARE(ui.nodeOffset(node), Vector2{});
    CORRADE_COMPARE(ui.nodeSize(node), (Vector2{32, 16}));

    /* Internally it should create a non-owned label, if it doesn't then this
       would remove everything */
    ui.clean();

    /* Can only verify that the data were created, nothing else. Visually
       tested in LabelGLTest. */
    CORRADE_COMPARE(ui.baseLayer().usedCount(), 0);
    CORRADE_COMPARE(ui.textLayer().usedCount(), 1);
    CORRADE_COMPARE(ui.layoutLayer().usedCount(), 1);
    CORRADE_COMPARE((data.customDataLayer ? *customDataLayer : ui.dataLayer()).usedCount(), 1);
}

void LabelTest::constructNoCreate() {
    Label label{NoCreate};
    CORRADE_VERIFY(!label.hasDataBinding());
    CORRADE_COMPARE(label.node(), NodeHandle::Null);
    CORRADE_COMPARE(label.textData(), DataHandle::Null);
    CORRADE_COMPARE(label.layoutData(), DataHandle::Null);
    CORRADE_COMPARE(label.dataBindingData(), DataHandle::Null);
}

void LabelTest::setStyle() {
    auto&& data = SetStyleData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    Label label = data.text ?
        Label{Anchor{root, {}, {32, 16}}, data.text, data.originalStyle} :
        Label{Anchor{root, {}, {32, 16}}, data.icon, data.originalStyle};
    CORRADE_COMPARE(label.style(), data.originalStyle);

    UnsignedInt previousLayoutStyle = ui.layoutLayer().style(label.layoutData());
    UnsignedInt previousStyle;
    if(data.text || data.icon != Icon::None)
        previousStyle = ui.textLayer().style(label.textData());
    else CORRADE_COMPARE(label.textData(), DataHandle::Null);

    /* The style change should result in different text layer style being
       used. The time overload should behave exactly the same assuming no
       animations are set up. Behavior with style-supplied animations tested in
       InputGLTest. */
    if(data.timeOverload)
        label.setStyle(data.changedStyle, 123456_nsec);
    else
        label.setStyle(data.changedStyle);
    CORRADE_COMPARE(label.style(), data.changedStyle);
    if(data.text || data.icon != Icon::None)
        CORRADE_COMPARE_AS(ui.textLayer().style(label.textData()),
            previousStyle,
            TestSuite::Compare::NotEqual);
    else CORRADE_COMPARE(label.textData(), DataHandle::Null);

    /* ... and optionally layout layer style as well */
    if(data.expectLayoutStyleChange)
        CORRADE_COMPARE_AS(ui.layoutLayer().style(label.layoutData()),
            previousLayoutStyle,
            TestSuite::Compare::NotEqual);
}

void LabelTest::setIcon() {
    Label label{Anchor{root, {}, {16, 32}}, Icon::No, LabelStyle::Default};
    CORRADE_COMPARE(label.icon(), Icon::No);
    CORRADE_COMPARE(ui.textLayer().glyphCount(label.textData()), 1);

    /* Clear the icon data to be able to verify that it gets updated */
    ui.textLayer().setText(label.textData(), "", {});
    CORRADE_COMPARE(ui.textLayer().glyphCount(label.textData()), 0);

    label.setIcon(Icon::Yes);
    CORRADE_COMPARE(label.icon(), Icon::Yes);
    CORRADE_COMPARE(ui.textLayer().glyphCount(label.textData()), 1);
}

void LabelTest::setIconFromText() {
    Label label{Anchor{root, {}, {16, 32}}, "hello", LabelStyle::Default};
    CORRADE_COMPARE(label.icon(), Icon::None);
    CORRADE_VERIFY(ui.isHandleValid(label.textData()));

    /* It should reuse the same data instead of recreating */
    DataHandle previousData = label.textData();
    label.setIcon(Icon::Yes);
    CORRADE_COMPARE(label.icon(), Icon::Yes);
    CORRADE_COMPARE(label.textData(), previousData);
    CORRADE_VERIFY(ui.isHandleValid(label.textData()));
    CORRADE_COMPARE(ui.textLayer().glyphCount(label.textData()), 1);
}

void LabelTest::setIconFromEmpty() {
    Label label{Anchor{root, {}, {16, 32}}, Icon::None, LabelStyle::Danger};
    CORRADE_COMPARE(label.icon(), Icon::None);
    CORRADE_COMPARE(label.textData(), DataHandle::Null);
    CORRADE_COMPARE(ui.textLayer().usedCount(), 0);

    label.setIcon(Icon::Yes);
    CORRADE_COMPARE(label.icon(), Icon::Yes);
    CORRADE_VERIFY(ui.isHandleValid(label.textData()));
    CORRADE_COMPARE(ui.textLayer().glyphCount(label.textData()), 1);
}

void LabelTest::setIconEmpty() {
    Label label{Anchor{root, {}, {16, 32}}, Icon::No, LabelStyle::Primary};
    CORRADE_COMPARE(label.icon(), Icon::No);
    CORRADE_VERIFY(ui.isHandleValid(label.textData()));
    CORRADE_COMPARE(ui.textLayer().usedCount(), 1);

    /* The original icon data should be removed */
    label.setIcon(Icon::None);
    CORRADE_COMPARE(label.icon(), Icon::None);
    CORRADE_COMPARE(label.textData(), DataHandle::Null);
    CORRADE_COMPARE(ui.textLayer().usedCount(), 0);
}

void LabelTest::setIconEmptyFromText() {
    Label label{Anchor{root, {}, {16, 32}}, "hello", LabelStyle::Default};
    CORRADE_COMPARE(label.icon(), Icon::None);
    CORRADE_VERIFY(ui.isHandleValid(label.textData()));
    CORRADE_COMPARE(ui.textLayer().usedCount(), 1);

    /* The original text data should be removed */
    label.setIcon(Icon::None);
    CORRADE_COMPARE(label.icon(), Icon::None);
    CORRADE_COMPARE(label.textData(), DataHandle::Null);
    CORRADE_COMPARE(ui.textLayer().usedCount(), 0);
}

void LabelTest::setText() {
    Label label{Anchor{root, {}, {16, 32}}, "hello", LabelStyle::Default};
    CORRADE_COMPARE(ui.textLayer().glyphCount(label.textData()), 5);

    label.setText("wonderful!!");
    CORRADE_COMPARE(ui.textLayer().glyphCount(label.textData()), 11);
}

void LabelTest::setTextTextProperties() {
    Label label{Anchor{root, {}, {16, 32}}, "hello", LabelStyle::Default};
    CORRADE_COMPARE(ui.textLayer().glyphCount(label.textData()), 5);

    label.setText("wonderful!!",
        TextProperties{}.setScript(Text::Script::Braille));
    /* Multiplied by 6 because of the Braille script */
    CORRADE_COMPARE(ui.textLayer().glyphCount(label.textData()), 11*6);
}

void LabelTest::setTextFromIcon() {
    Label label{Anchor{root, {}, {16, 32}}, Icon::No, LabelStyle::Default};
    CORRADE_VERIFY(ui.isHandleValid(label.textData()));
    CORRADE_COMPARE(ui.textLayer().usedCount(), 1);

    /* It should reuse the same data instead of recreating */
    DataHandle previousData = label.textData();
    label.setText("wonderful!!");
    CORRADE_COMPARE(label.icon(), Icon::None);
    CORRADE_COMPARE(label.textData(), previousData);
    CORRADE_VERIFY(ui.isHandleValid(label.textData()));
    CORRADE_COMPARE(ui.textLayer().glyphCount(label.textData()), 11);
}

void LabelTest::setTextFromEmpty() {
    Label label{Anchor{root, {}, {16, 32}}, ""};
    CORRADE_COMPARE(label.textData(), DataHandle::Null);
    CORRADE_COMPARE(ui.textLayer().usedCount(), 0);

    label.setText("wonderful!!");
    CORRADE_COMPARE(label.icon(), Icon::None);
    CORRADE_VERIFY(ui.isHandleValid(label.textData()));
    CORRADE_COMPARE(ui.textLayer().glyphCount(label.textData()), 11);
}

void LabelTest::setTextEmpty() {
    Label label{Anchor{root, {}, {16, 32}}, "hello", LabelStyle::Default};
    CORRADE_VERIFY(ui.isHandleValid(label.textData()));
    CORRADE_COMPARE(ui.textLayer().usedCount(), 1);

    /* The original text data should be removed */
    label.setText("");
    CORRADE_COMPARE(label.icon(), Icon::None);
    CORRADE_COMPARE(label.textData(), DataHandle::Null);
    CORRADE_COMPARE(ui.textLayer().usedCount(), 0);
}

void LabelTest::setTextEmptyFromIcon() {
    Label label{Anchor{root, {}, {16, 32}}, Icon::No, LabelStyle::Info};
    CORRADE_COMPARE(label.icon(), Icon::No);
    CORRADE_VERIFY(ui.isHandleValid(label.textData()));
    CORRADE_COMPARE(ui.textLayer().usedCount(), 1);

    /* The original icon data should be removed */
    label.setText("");
    CORRADE_COMPARE(label.icon(), Icon::None);
    CORRADE_COMPARE(label.textData(), DataHandle::Null);
    CORRADE_COMPARE(ui.textLayer().usedCount(), 0);
}

void LabelTest::setIconTextInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    Int value = 3711;
    Storage<Int> storage{ui, value, StorageFlag::ReferenceCounted};
    Label label{Anchor{root, {}, {}}, storage};
    CORRADE_VERIFY(label.hasDataBinding());

    /* Setting a style is allowed with a data binding present */
    label.setStyle(LabelStyle::Danger);
    CORRADE_COMPARE(label.style(), LabelStyle::Danger);

    Containers::String out;
    Error redirectError{&out};
    label.setIcon({});
    label.setText({});
    CORRADE_COMPARE_AS(out,
        "Ui::Label::setIcon(): can't be called with a data binding present\n"
        "Ui::Label::setText(): can't be called with a data binding present\n",
        TestSuite::Compare::String);
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::LabelTest)
