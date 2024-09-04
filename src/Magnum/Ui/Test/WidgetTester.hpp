#ifndef Magnum_Ui_Test_WidgetTest_hpp
#define Magnum_Ui_Test_WidgetTest_hpp
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

#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/TestSuite/Tester.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/Text/AbstractFont.h>
#include <Magnum/Text/AbstractGlyphCache.h>
#include <Magnum/Text/AbstractShaper.h>
#include <Magnum/Text/Alignment.h>
#include <Magnum/Text/Script.h>

#include "Magnum/Ui/BaseLayer.h"
#include "Magnum/Ui/Handle.h"
#include "Magnum/Ui/Style.h"
#include "Magnum/Ui/Style.hpp"
#include "Magnum/Ui/TextLayer.h"
#include "Magnum/Ui/UserInterface.h"

namespace Magnum { namespace Ui { namespace Test { namespace {

/* Base WidgetTester class to be used by actual widget tests. Desired use case:

    - Derive the test from the `WidgetTester` class
    - Use the `ui` member and parent everything to `rootNode`
    - Add tests with the `setup()` and `teardown()` routines, which will ensure
      that everything is correctly cleaned up for the next test case
*/

struct TestUserInterface: UserInterface {
    explicit TestUserInterface(NoCreateT): UserInterface{NoCreate} {}
};

struct TestBaseLayerShared: BaseLayer::Shared {
    explicit TestBaseLayerShared(): BaseLayer::Shared{Configuration{Implementation::BaseStyleCount}} {
        BaseLayerStyleUniform uniforms[Implementation::BaseStyleCount];
        setStyle(BaseLayerCommonStyleUniform{}, uniforms, {});

        setStyleTransition<Implementation::BaseStyle,
            Implementation::styleTransitionToInactiveOut,
            Implementation::styleTransitionToInactiveOver,
            Implementation::styleTransitionToFocusedOut,
            Implementation::styleTransitionToFocusedOver,
            Implementation::styleTransitionToPressedOut,
            Implementation::styleTransitionToPressedOver,
            Implementation::styleTransitionToDisabled>();
    }

    void doSetStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>) override {}
};

struct TestBaseLayer: BaseLayer {
    explicit TestBaseLayer(LayerHandle handle, Shared& shared): BaseLayer{handle, shared} {}
};

struct TestTextLayerShared: TextLayer::Shared {
    explicit TestTextLayerShared(): TextLayer::Shared{Configuration{Implementation::TextStyleUniformCount, Implementation::TextStyleCount}} {
        setGlyphCache(glyphCache);

        font.openFile("", 16.0f);
        glyphCache.addFont(Implementation::IconCount + 1, &font);

        FontHandle fontHandle[]{addFont(font, 16.0f)};
        Text::Alignment alignment[]{Text::Alignment::MiddleCenter};
        TextLayerStyleUniform uniforms[Implementation::TextStyleUniformCount];
        UnsignedInt styles[]{0};
        setStyle(TextLayerCommonStyleUniform{}, uniforms,
            Containers::stridedArrayView(styles).broadcasted<0>(Implementation::TextStyleCount),
            Containers::stridedArrayView(fontHandle).broadcasted<0>(Implementation::TextStyleCount),
            Containers::stridedArrayView(alignment).broadcasted<0>(Implementation::TextStyleCount),
            {}, {}, {}, {}, {}, {});

        setStyleTransition<Implementation::TextStyle,
            Implementation::styleTransitionToInactiveOut,
            Implementation::styleTransitionToInactiveOver,
            Implementation::styleTransitionToFocusedOut,
            Implementation::styleTransitionToFocusedOver,
            Implementation::styleTransitionToPressedOut,
            Implementation::styleTransitionToPressedOver,
            Implementation::styleTransitionToDisabled>();
    }

    void doSetStyle(const TextLayerCommonStyleUniform&, Containers::ArrayView<const TextLayerStyleUniform>) override {}
    void doSetEditingStyle(const TextLayerCommonEditingStyleUniform&, Containers::ArrayView<const TextLayerEditingStyleUniform>) override {}

    struct: Text::AbstractGlyphCache {
        using Text::AbstractGlyphCache::AbstractGlyphCache;

        Text::GlyphCacheFeatures doFeatures() const override { return {}; }
        void doSetImage(const Vector2i&, const ImageView2D&) override {}
    } glyphCache{PixelFormat::R8Unorm, {32, 32}};

    struct TestFont: Text::AbstractFont {
        Text::FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return _opened; }
        Properties doOpenFile(Containers::StringView, Float) override {
            _opened = true;
            return {16.0f, 8.0f, -4.0f, 16.0f, 1};
        }
        void doClose() override { _opened = false; }

        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {}
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override {
            struct Shaper: Text::AbstractShaper {
                using Text::AbstractShaper::AbstractShaper;

                bool doSetScript(Text::Script script) override {
                    _multiply = script == Text::Script::Braille ? 6 : 1;
                    return true;
                }
                UnsignedInt doShape(Containers::StringView text, UnsignedInt, UnsignedInt, Containers::ArrayView<const Text::FeatureRange>) override {
                    return text.size()*_multiply;
                }
                void doGlyphIdsInto(const Containers::StridedArrayView1D<UnsignedInt>& ids) const override {
                    for(UnsignedInt& id: ids)
                        id = 0;
                }
                void doGlyphOffsetsAdvancesInto(const Containers::StridedArrayView1D<Vector2>& offsets, const Containers::StridedArrayView1D<Vector2>& advances) const override {
                    /* Each next glyph has the advance and offset higher */
                    for(std::size_t i = 0; i != offsets.size(); ++i) {
                        offsets[i] = {};
                        advances[i] = {12.0f, 0.0f};
                    }
                }
                void doGlyphClustersInto(const Containers::StridedArrayView1D<UnsignedInt>& clusters) const override {
                    for(std::size_t i = 0; i != clusters.size(); ++i) {
                        clusters[i] = i;
                    }
                }

                private:
                    UnsignedInt _multiply = 1;
            };

            return Containers::pointer<Shaper>(*this);
        }

        bool _opened = false;
    } font;
};

struct TestTextLayer: TextLayer {
    explicit TestTextLayer(LayerHandle handle, Shared& shared): TextLayer{handle, shared} {}
};

struct WidgetTester: TestSuite::Tester {
    explicit WidgetTester();

    void setup();
    void teardown();

    TestBaseLayerShared baseLayerShared;
    TestTextLayerShared textLayerShared;
    TestUserInterface ui{NoCreate};
    /* Deliberately an invalid non-null handle initially, to make sure nothing
       is parented to it before it's populated in setup() */
    NodeHandle rootNode = nodeHandle(0xfffff, 0xfff);
};

WidgetTester::WidgetTester() {
    ui.setBaseLayerInstance(Containers::pointer<TestBaseLayer>(ui.createLayer(), baseLayerShared))
      .setTextLayerInstance(Containers::pointer<TestTextLayer>(ui.createLayer(), textLayerShared))
      .setSize({100, 100});
}

void WidgetTester::setup() {
    CORRADE_INTERNAL_ASSERT(!ui.isHandleValid(rootNode));
    CORRADE_INTERNAL_ASSERT(ui.nodeUsedCount() == 0);
    CORRADE_INTERNAL_ASSERT(ui.baseLayer().usedCount() == 0);
    CORRADE_INTERNAL_ASSERT(ui.textLayer().usedCount() == 0);
    rootNode = ui.createNode({}, ui.size());
}

void WidgetTester::teardown() {
    ui.removeNode(rootNode);
    ui.clean();
    CORRADE_INTERNAL_ASSERT(!ui.isHandleValid(rootNode));
    CORRADE_INTERNAL_ASSERT(ui.nodeUsedCount() == 0);
    CORRADE_INTERNAL_ASSERT(ui.baseLayer().usedCount() == 0);
    CORRADE_INTERNAL_ASSERT(ui.textLayer().usedCount() == 0);
}


}}}}

#endif
