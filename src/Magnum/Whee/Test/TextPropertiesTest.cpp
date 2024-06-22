/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023 Vladimír Vondruš <mosra@centrum.cz>

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

#include <sstream> /** @todo remove once Debug is stream-free */
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/String.h>
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/Utility/DebugStl.h> /** @todo remove once Debug is stream-free */
#include <Magnum/Text/Alignment.h>
#include <Magnum/Text/Direction.h>
#include <Magnum/Text/Feature.h>
#include <Magnum/Text/Script.h>

#include "Magnum/Whee/TextLayer.h" /* FontHandle */
#include "Magnum/Whee/TextProperties.h"

namespace Magnum { namespace Whee { namespace Test { namespace {

struct TextPropertiesTest: TestSuite::Tester {
    explicit TextPropertiesTest();

    void featureValue();
    void featureValueBoolean();
    void featureValueInteger();

    void construct();
    void constructAlignment();
    void constructFont();
    void constructFontAlignment();
    void constructCopy();
    void constructMove();

    void setters();
    void alignmentInvalid();
    void directionValueOverflow();
    void copyLanguage();
    void copyFeatures();
};

using namespace Containers::Literals;

const struct {
    const char* name;
    bool stateAlreadyAllocated;
} PropertiesCopyData[]{
    {"", false},
    {"state already allocated", true},
};

TextPropertiesTest::TextPropertiesTest() {
    addTests({&TextPropertiesTest::featureValue,
              &TextPropertiesTest::featureValueBoolean,
              &TextPropertiesTest::featureValueInteger,

              &TextPropertiesTest::construct,
              &TextPropertiesTest::constructAlignment,
              &TextPropertiesTest::constructFont,
              &TextPropertiesTest::constructFontAlignment,
              &TextPropertiesTest::constructCopy,
              &TextPropertiesTest::constructMove,

              &TextPropertiesTest::setters,
              &TextPropertiesTest::alignmentInvalid,
              &TextPropertiesTest::directionValueOverflow});

    addInstancedTests({&TextPropertiesTest::copyLanguage,
                       &TextPropertiesTest::copyFeatures},
        Containers::arraySize(PropertiesCopyData));
}

void TextPropertiesTest::featureValue() {
    TextFeatureValue a = Text::Feature::AboveBaseMarkPositioning;
    CORRADE_COMPARE(a.feature(), Text::Feature::AboveBaseMarkPositioning);
    CORRADE_COMPARE(a.value(), 1);
    CORRADE_VERIFY(a.isEnabled());

    Text::FeatureRange b = a;
    CORRADE_COMPARE(b.feature(), Text::Feature::AboveBaseMarkPositioning);
    CORRADE_COMPARE(b.value(), 1);
    CORRADE_VERIFY(b.isEnabled());
    CORRADE_COMPARE(b.begin(), 0);
    CORRADE_COMPARE(b.end(), 0xffffffffu);
}

void TextPropertiesTest::featureValueBoolean() {
    TextFeatureValue a{Text::Feature::Kerning, false};
    CORRADE_COMPARE(a.feature(), Text::Feature::Kerning);
    CORRADE_COMPARE(a.value(), 0);
    CORRADE_VERIFY(!a.isEnabled());

    Text::FeatureRange b = a;
    CORRADE_COMPARE(b.feature(), Text::Feature::Kerning);
    CORRADE_COMPARE(b.value(), 0);
    CORRADE_VERIFY(!b.isEnabled());
    CORRADE_COMPARE(b.begin(), 0);
    CORRADE_COMPARE(b.end(), 0xffffffffu);
}

void TextPropertiesTest::featureValueInteger() {
    TextFeatureValue a{Text::Feature::AccessAllAlternates, 134};
    CORRADE_COMPARE(a.feature(), Text::Feature::AccessAllAlternates);
    CORRADE_COMPARE(a.value(), 134);

    Text::FeatureRange b = a;
    CORRADE_COMPARE(b.feature(), Text::Feature::AccessAllAlternates);
    CORRADE_COMPARE(b.value(), 134);
    CORRADE_COMPARE(b.begin(), 0);
    CORRADE_COMPARE(b.end(), 0xffffffffu);
}

void TextPropertiesTest::construct() {
    TextProperties properties = {};

    CORRADE_COMPARE(properties.alignment(), Containers::NullOpt);
    CORRADE_COMPARE(properties.font(), FontHandle::Null);
    CORRADE_COMPARE(properties.script(), Text::Script::Unspecified);
    CORRADE_COMPARE(properties.language(), "");
    CORRADE_COMPARE(properties.shapeDirection(), Text::ShapeDirection::Unspecified);
    CORRADE_COMPARE(properties.layoutDirection(), Text::LayoutDirection::HorizontalTopToBottom);
    CORRADE_VERIFY(properties.features().isEmpty());
}

void TextPropertiesTest::constructAlignment() {
    TextProperties properties = Text::Alignment::LineCenterIntegral;

    /* The other properties should be the same as if default-constructed, i.e.
       it should delegate to the default constructor */
    CORRADE_COMPARE(properties.alignment(), Text::Alignment::LineCenterIntegral);
    CORRADE_COMPARE(properties.font(), FontHandle::Null);
    CORRADE_COMPARE(properties.script(), Text::Script::Unspecified);
    CORRADE_COMPARE(properties.language(), "");
    CORRADE_COMPARE(properties.shapeDirection(), Text::ShapeDirection::Unspecified);
    CORRADE_COMPARE(properties.layoutDirection(), Text::LayoutDirection::HorizontalTopToBottom);
    CORRADE_VERIFY(properties.features().isEmpty());

    /* It shouldn't be constructible from an optional or NullOpt, that makes no
       sense -- simply don't pass anything in that case */
    CORRADE_VERIFY(std::is_constructible<TextProperties, Text::Alignment>::value);
    CORRADE_VERIFY(!std::is_constructible<TextProperties, Containers::Optional<Text::Alignment>>::value);
    CORRADE_VERIFY(!std::is_constructible<TextProperties, Containers::NullOptT>::value);
}

void TextPropertiesTest::constructFont() {
    TextProperties properties = Whee::fontHandle(13, 1);

    /* The other properties should be the same as if default-constructed, i.e.
       it should delegate to the default constructor */
    CORRADE_COMPARE(properties.alignment(), Containers::NullOpt);
    CORRADE_COMPARE(properties.font(), Whee::fontHandle(13, 1));
    CORRADE_COMPARE(properties.script(), Text::Script::Unspecified);
    CORRADE_COMPARE(properties.language(), "");
    CORRADE_COMPARE(properties.shapeDirection(), Text::ShapeDirection::Unspecified);
    CORRADE_COMPARE(properties.layoutDirection(), Text::LayoutDirection::HorizontalTopToBottom);
    CORRADE_VERIFY(properties.features().isEmpty());
}

void TextPropertiesTest::constructFontAlignment() {
    TextProperties properties = {Whee::fontHandle(13, 1), Text::Alignment::LineCenterIntegral};

    /* The other properties should be the same as if default-constructed, i.e.
       it should delegate to the default constructor */
    CORRADE_COMPARE(properties.alignment(), Text::Alignment::LineCenterIntegral);
    CORRADE_COMPARE(properties.font(), Whee::fontHandle(13, 1));
    CORRADE_COMPARE(properties.script(), Text::Script::Unspecified);
    CORRADE_COMPARE(properties.language(), "");
    CORRADE_COMPARE(properties.shapeDirection(), Text::ShapeDirection::Unspecified);
    CORRADE_COMPARE(properties.layoutDirection(), Text::LayoutDirection::HorizontalTopToBottom);
    CORRADE_VERIFY(properties.features().isEmpty());
}

void TextPropertiesTest::constructCopy() {
    CORRADE_VERIFY(!std::is_copy_constructible<TextProperties>{});
    CORRADE_VERIFY(!std::is_copy_assignable<TextProperties>{});
}

void TextPropertiesTest::constructMove() {
    /* The move is defaulted, so just verify that ońe inline and one allocated
       property gets transferred correctly, the rest should behave the same */
    TextProperties a;
    a.setScript(Text::Script::HanifiRohingya);
    a.setFeatures({Text::Feature::Kerning});

    TextProperties b = Utility::move(a);
    CORRADE_COMPARE(b.script(), Text::Script::HanifiRohingya);
    CORRADE_COMPARE(b.features().size(), 1);
    CORRADE_COMPARE(b.features()[0].feature(), Text::Feature::Kerning);

    TextProperties c;
    c.setScript(Text::Script::Braille);
    c.setFeatures({Text::Feature::DiscretionaryLigatures, {Text::Feature::Kerning, false}});
    c = Utility::move(b);
    CORRADE_COMPARE(c.script(), Text::Script::HanifiRohingya);
    CORRADE_COMPARE(c.features().size(), 1);
    CORRADE_COMPARE(c.features()[0].feature(), Text::Feature::Kerning);

    CORRADE_VERIFY(std::is_nothrow_move_constructible<TextProperties>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<TextProperties>::value);
}

void TextPropertiesTest::setters() {
    Containers::StringView language = "eh-UH!"_s;

    TextProperties properties;
    properties
        .setAlignment(Text::Alignment::TopCenterIntegral)
        .setFont(Whee::fontHandle(13, 1))
        .setScript(Text::Script::HanifiRohingya)
        .setLanguage(language.exceptSuffix(1))
        .setShapeDirection(Text::ShapeDirection::BottomToTop)
        .setLayoutDirection(Text::LayoutDirection::VerticalRightToLeft)
        /* setFeatures() tested in propertiesCopyFeatures() instead */;
    CORRADE_COMPARE(properties.alignment(), Text::Alignment::TopCenterIntegral);
    CORRADE_COMPARE(properties.font(), Whee::fontHandle(13, 1));
    CORRADE_COMPARE(properties.script(), Text::Script::HanifiRohingya);

    /* The language should stay global and the exact same pointer, no copy even
       though it's not null-terminated. Copy is tested in
       propertiesCopyLanguage() instead. */
    CORRADE_COMPARE(properties.language(), "eh-UH");
    CORRADE_COMPARE(properties.language().flags(), Containers::StringViewFlag::Global);
    CORRADE_COMPARE(properties.language().data(), static_cast<const void*>(language.data()));

    CORRADE_COMPARE(properties.shapeDirection(), Text::ShapeDirection::BottomToTop);
    CORRADE_COMPARE(properties.layoutDirection(), Text::LayoutDirection::VerticalRightToLeft);

    /* Resetting alignment should again make it NullOpt */
    properties.setAlignment({});
    CORRADE_COMPARE(properties.alignment(), Containers::NullOpt);
}

void TextPropertiesTest::alignmentInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    TextProperties properties;

    std::ostringstream out;
    Error redirectError{&out};
    properties.setAlignment(Text::Alignment::LineCenterGlyphBounds);
    CORRADE_COMPARE(out.str(), "Whee::TextProperties::setAlignment(): Text::Alignment::LineCenterGlyphBounds is not supported\n");
}

void TextPropertiesTest::directionValueOverflow() {
    /* Setting an invalid (too large) direction value shouldn't overwrite the
       other direction property, it should get cut instead */

    {
        TextProperties properties;
        properties
            .setLayoutDirection(Text::LayoutDirection::VerticalRightToLeft)
            .setShapeDirection(Text::ShapeDirection(0xff));
        CORRADE_COMPARE(properties.layoutDirection(), Text::LayoutDirection::VerticalRightToLeft);
        CORRADE_COMPARE(properties.shapeDirection(), Text::ShapeDirection(0x0f));
    } {
        TextProperties properties;
        properties
            .setShapeDirection(Text::ShapeDirection::BottomToTop)
            .setLayoutDirection(Text::LayoutDirection(0xff));
        CORRADE_COMPARE(properties.shapeDirection(), Text::ShapeDirection::BottomToTop);
        CORRADE_COMPARE(properties.layoutDirection(), Text::LayoutDirection(0x0f));
    }
}

void TextPropertiesTest::copyLanguage() {
    auto&& data = PropertiesCopyData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    TextProperties properties;

    if(data.stateAlreadyAllocated)
        properties.setFeatures({Text::Feature::Kerning});

    Containers::StringView language = "eh-UH!";

    /* Internal state is allocated if not already and a copy is made, which
       makes it become null-terminated but not global. */
    properties.setLanguage(language.exceptSuffix(1));
    CORRADE_COMPARE(properties.language(), "eh-UH");
    CORRADE_COMPARE(properties.language().flags(), Containers::StringViewFlag::NullTerminated);
    CORRADE_VERIFY(properties.language().data() != language.data());

    /* It shouldn't unconditionally overwrite existing state */
    if(data.stateAlreadyAllocated) {
        CORRADE_COMPARE(properties.features().size(), 1);
        CORRADE_COMPARE(properties.features()[0].feature(), Text::Feature::Kerning);
    }
}

void TextPropertiesTest::copyFeatures() {
    auto&& data = PropertiesCopyData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    TextProperties properties;

    if(data.stateAlreadyAllocated) {
        /* Deliberately a String to be sure it doesn't get treated as global
           with no state allocation */
        properties.setLanguage(Containers::String{"eh-UH"});
    }

    /* Internal state is allocated if not already and a copy is made */
    Text::FeatureRange features[]{
        {Text::Feature::DiscretionaryLigatures, 3, 5},
        {Text::Feature::Kerning, false}
    };
    properties.setFeatures(features);
    CORRADE_COMPARE(properties.features().size(), 2);
    CORRADE_COMPARE(properties.features()[0].feature(), Text::Feature::DiscretionaryLigatures);
    CORRADE_COMPARE(properties.features()[0].begin(), 3);
    CORRADE_COMPARE(properties.features()[0].end(), 5);
    CORRADE_COMPARE(properties.features()[1].feature(), Text::Feature::Kerning);
    CORRADE_VERIFY(!properties.features()[1].isEnabled());

    /* It shouldn't unconditionally overwrite existing state */
    if(data.stateAlreadyAllocated)
        CORRADE_COMPARE(properties.language(), "eh-UH");
}

}}}}

CORRADE_TEST_MAIN(Magnum::Whee::Test::TextPropertiesTest)
