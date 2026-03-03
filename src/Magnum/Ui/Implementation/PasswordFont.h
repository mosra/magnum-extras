#ifndef Magnum_Ui_Implementation_PasswordFont_h
#define Magnum_Ui_Implementation_PasswordFont_h
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

#include <Corrade/Containers/GrowableArray.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Containers/Triple.h>
#include <Corrade/Utility/Algorithms.h>
#include <Corrade/Utility/Unicode.h>
#include <Magnum/Math/Range.h>
#include <Magnum/Text/AbstractFont.h>
#include <Magnum/Text/AbstractGlyphCache.h>
#include <Magnum/Text/AbstractShaper.h>

namespace Magnum { namespace Ui { namespace Implementation {

/* Not exposed as a public API for custom styles as I'm not fully sure about
   the behavior yet, especially given that the font doesn't implement
   fillGlyphCache(). For all UTF-8 input chars it renders chosen glyph from
   another font, which it aliases in the cache. Tested in ThemeTest.cpp. */
class PasswordFont: public Text::AbstractFont {
    public:
        /* The glyphId is expected to exist in the original font and be already
           rendered into the cache. The font registers itself in the cache as
           a single-glyph font, and aliases the location of given glyph from
           the original font. */
        /** @todo can't glyphAdvance() and such be const on AbstractFont? this
            is silly ... */
        explicit PasswordFont(Text::AbstractGlyphCache& cache, Text::AbstractFont& originalFont, UnsignedInt glyphId, Float advanceScale = 1.0f) {
            /* The original font should be registered in the cache and the
               cache should contain the requested glyph */
            const Containers::Optional<UnsignedInt> originalFontId = cache.findFont(originalFont);
            CORRADE_INTERNAL_ASSERT(originalFontId);
            const UnsignedInt originalGlyphId = cache.glyphId(*originalFontId, glyphId);
            CORRADE_INTERNAL_ASSERT(originalGlyphId);

            const Containers::Triple<Vector2i, Int, Range2Di> glyph = cache.glyph(originalGlyphId);
            /* Undo the padding that was applied by addGlyph() itself so it
               doesn't add it again */
            cache.addGlyph(cache.addFont(1, this), 0,
                glyph.first() + cache.padding(),
                glyph.second(),
                glyph.third().padded(-cache.padding()));

            _fontProperties = {originalFont.size(), originalFont.ascent(), originalFont.descent(), originalFont.lineHeight(), 1};
            Vector2 glyphAdvance = originalFont.glyphAdvance(glyphId);
            CORRADE_INTERNAL_ASSERT(glyphAdvance.y() == 0.0f);
            _advance = glyphAdvance.x()*advanceScale;
        }

    private:
        Text::FontFeatures doFeatures() const override {
            return Text::FontFeature::OpenData;
        }

        bool doIsOpened() const override { return _opened; }

        Properties doOpenData(Containers::ArrayView<const char>, Float size) override {
            CORRADE_INTERNAL_ASSERT(size == _fontProperties.size);
            _opened = true;
            return _fontProperties;
        }

        /* LCOV_EXCL_START. Nothing in the theme should use any of these. */
        void doClose() override {
            CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        }
        void doGlyphIdsInto(const Containers::StridedArrayView1D<const char32_t>&, const Containers::StridedArrayView1D<UnsignedInt>&) override {
            CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        }
        Vector2 doGlyphSize(UnsignedInt) override {
            CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        }
        Vector2 doGlyphAdvance(UnsignedInt) override {
            CORRADE_INTERNAL_ASSERT_UNREACHABLE();
        }
        /* LCOV_EXCL_STOP */

        Containers::Pointer<Text::AbstractShaper> doCreateShaper() override {
            struct Shaper: Text::AbstractShaper {
                using Text::AbstractShaper::AbstractShaper;

                private:
                    UnsignedInt doShape(Containers::StringView textFull, UnsignedInt begin, UnsignedInt end, Containers::ArrayView<const Text::FeatureRange>) override {
                        /** @todo fix the API to not send ~UnsignedInt{} here */
                        const Containers::StringView text = textFull.slice(begin,
                            end == ~UnsignedInt{} ? UnsignedInt(textFull.size()) : end);

                        /* Iterate the input to find out cluster IDs */
                        arrayClear(_clusters);
                        arrayReserve(_clusters, text.size());
                        for(std::size_t i = 0; i != text.size(); ) {
                            const Containers::Pair<char32_t, std::size_t> codepointNext = Utility::Unicode::nextChar(text, i);
                            arrayAppend(_clusters, begin + UnsignedInt(i));
                            i = codepointNext.second();
                        }

                        return _clusters.size();
                    }
                    void doGlyphIdsInto(const Containers::StridedArrayView1D<UnsignedInt>& ids) const override {
                        /* It's just the one single glyph */
                        for(UnsignedInt& i: ids)
                            i = 0;
                    }
                    void doGlyphOffsetsAdvancesInto(const Containers::StridedArrayView1D<Vector2>& offsets, const Containers::StridedArrayView1D<Vector2>& advances) const override {
                        const Float advance = static_cast<const PasswordFont&>(font())._advance;
                        for(Vector2& i: offsets)
                            i = {};
                        for(Vector2& i: advances)
                            i = {advance, 0.0f};
                    }
                    void doGlyphClustersInto(const Containers::StridedArrayView1D<UnsignedInt>& clusters) const override {
                        Utility::copy(_clusters, clusters);
                    }

                    Containers::Array<UnsignedInt> _clusters;
            };

            return Containers::pointer<Shaper>(*this);
        }

        /** @todo geez, can't I invent some better way how to implement this
            than with an _opened bool, and returning cryptic Properties from
            doOpen()? such as with openState() or whatever, could do font views
            that way also... */
        bool _opened = false;
        Properties _fontProperties;
        Float _advance;
};

}}}

#endif
