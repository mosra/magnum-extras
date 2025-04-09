#ifndef Magnum_Ui_TextLayerGL_h
#define Magnum_Ui_TextLayerGL_h
/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023, 2024, 2025
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

/** @file
 * @brief Class @ref Magnum::Ui::TextLayerGL
 * @m_since_latest
 */

#include "Magnum/configure.h"

#ifdef MAGNUM_TARGET_GL
#include "Magnum/Ui/TextLayer.h"

namespace Magnum { namespace Ui {

/**
@brief OpenGL implementation of the text layer
@m_since_latest

See the @ref TextLayer base class documentation for information about setting
up an instance of this layer and using it. The base class contains most of the
interface you'll be interacting with, this subclass exposes just the APIs tied
to OpenGL, such as glyph cache setup.

The layer assumes @ref RendererGL is set on the user interface (or
@ref UserInterfaceGL used, which does so automatically), see its documentation
for more information about GL state expectations. The layer produces geometry
in a counter-clockwise winding, so @ref GL::Renderer::Feature::FaceCulling can
stay enabled when drawing it.

@note This class is available only if Magnum is compiled with
    @ref MAGNUM_TARGET_GL enabled (done by default). See @ref building-features
    for more information.
*/
class MAGNUM_UI_EXPORT TextLayerGL: public TextLayer {
    public:
        class Shared;

        /**
         * @brief Constructor
         * @param handle    Layer handle returned from
         *      @ref AbstractUserInterface::createLayer()
         * @param shared    Shared state containing font and style data
         *
         * The @p shared state is expected to be kept in scope for the whole
         * class lifetime. In order to draw the layer it's expected that
         * @ref Shared::setStyle() was called.
         */
        explicit TextLayerGL(LayerHandle handle, Shared& shared);

        /**
         * @brief Shared state used by this layer
         *
         * Reference to the instance passed to @ref TextLayerGL::TextLayerGL(LayerHandle, Shared&).
         */
        inline Shared& shared();
        /** @overload */
        inline const Shared& shared() const;

    protected:
        /**
         * @copybrief AbstractLayer::doDraw()
         *
         * It's possible for a subclass to override this function to perform
         * extra GL state changes and then delegate to the parent
         * implementation. As the implementation doesn't track current GL state
         * in any way at the moment, the state should be reset back to the
         * previous afterwards. Note that blending and scissor *enabled* state
         * is already taken care of by @ref LayerFeature::DrawUsesBlending and
         * @ref LayerFeature::DrawUsesScissor. See @ref AbstractLayer::doDraw()
         * for more information about how this function is called.
         */
        void doDraw(const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, std::size_t offset, std::size_t count, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, std::size_t clipRectOffset, std::size_t clipRectCount, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, const Containers::StridedArrayView1D<const Float>& nodeOpacities, Containers::BitArrayView nodesEnabled, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes) override;

    private:
        struct State;

        /* These can't be MAGNUM_UI_LOCAL otherwise deriving from this class
           causes linker errors. See BaseLayerGLTest::constructDerived() for a
           repro case. */
        LayerFeatures doFeatures() const override;

        void doSetSize(const Vector2& size, const Vector2i& framebufferSize) override;

        void doUpdate(LayerStates states, const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, const Containers::StridedArrayView1D<const Float>& nodeOpacities, Containers::BitArrayView nodesEnabled, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes, const Containers::StridedArrayView1D<const Vector2>& compositeRectOffsets, const Containers::StridedArrayView1D<const Vector2>& compositeRectSizes) override;
};

/**
@brief Shared state for the OpenGL implementation of the text layer

Contains fonts, shader instances and style data. In order to use the layer it's
expected that at least one font was added with @ref addFont(). In order to
update or draw the layer it's expected that @ref setStyle() was called.
*/
class MAGNUM_UI_EXPORT TextLayerGL::Shared: public TextLayer::Shared {
    public:
        /**
         * @brief Construct with a reference to a glyph cache instance
         *
         * The @p glyphCache is expected to be in scope for the whole shared
         * instance lifetime. Use the @ref Shared(Text::GlyphCacheArrayGL&&, const Configuration&)
         * constructor to make the shared state take over the glyph cache
         * instance.
         */
        explicit Shared(Text::GlyphCacheArrayGL& glyphCache, const Configuration& configuration);

        /**
         * @brief Construct with taking over ownership of a glyph cache instance
         *
         * Like @ref Shared(Text::GlyphCacheArrayGL&, const Configuration&),
         * but the shared state takes over the glyph cache ownership. You can
         * access the instance using @ref glyphCache() later.
         */
        explicit Shared(Text::GlyphCacheArrayGL&& glyphCache, const Configuration& configuration);

        /**
         * @brief Construct without creating the contents
         *
         * Doesn't touch any GL state. Move over a created instance to make it
         * useful. Passing a non-created instance to the @ref TextLayerGL
         * constructor has undefined behavior and will likely crash.
         */
        explicit Shared(NoCreateT) noexcept;

        /* Overloads to remove a WTF factor from method chaining order */
        #ifndef DOXYGEN_GENERATING_OUTPUT
        MAGNUMEXTRAS_UI_ABSTRACTVISUALLAYER_SHARED_SUBCLASS_IMPLEMENTATION()
        Shared& setStyle(const TextLayerCommonStyleUniform& commonUniform, Containers::ArrayView<const TextLayerStyleUniform> uniforms, const Containers::StridedArrayView1D<const FontHandle>& fonts, const Containers::StridedArrayView1D<const Text::Alignment>& alignments, Containers::ArrayView<const TextFeatureValue> features, const Containers::StridedArrayView1D<const UnsignedInt>& featureOffsets, const Containers::StridedArrayView1D<const UnsignedInt>& featureCounts, const Containers::StridedArrayView1D<const Int>& cursorStyles, const Containers::StridedArrayView1D<const Int>& selectionStyles, const Containers::StridedArrayView1D<const Vector4>& paddings);
        Shared& setStyle(const TextLayerCommonStyleUniform& commonUniform, std::initializer_list<TextLayerStyleUniform> uniforms, std::initializer_list<FontHandle> fonts, std::initializer_list<Text::Alignment> alignments, std::initializer_list<TextFeatureValue> features, std::initializer_list<UnsignedInt> featureOffsets, std::initializer_list<UnsignedInt> featureCounts, std::initializer_list<Int> cursorStyles, std::initializer_list<Int> selectionStyles, std::initializer_list<Vector4> paddings);
        Shared& setStyle(const TextLayerCommonStyleUniform& commonUniform, Containers::ArrayView<const TextLayerStyleUniform> uniforms, const Containers::StridedArrayView1D<const UnsignedInt>& styleToUniform, const Containers::StridedArrayView1D<const FontHandle>& styleFonts, const Containers::StridedArrayView1D<const Text::Alignment>& styleAlignments, Containers::ArrayView<const TextFeatureValue> styleFeatures, const Containers::StridedArrayView1D<const UnsignedInt>& styleFeatureOffsets, const Containers::StridedArrayView1D<const UnsignedInt>& styleFeatureCounts, const Containers::StridedArrayView1D<const Int>& styleCursorStyles, const Containers::StridedArrayView1D<const Int>& styleSelectionStyles, const Containers::StridedArrayView1D<const Vector4>& stylePaddings);
        Shared& setStyle(const TextLayerCommonStyleUniform& commonUniform, std::initializer_list<TextLayerStyleUniform> uniforms, std::initializer_list<UnsignedInt> styleToUniform, std::initializer_list<FontHandle> styleFonts, std::initializer_list<Text::Alignment> styleAlignments, std::initializer_list<TextFeatureValue> styleFeatures, std::initializer_list<UnsignedInt> styleFeatureOffsets, std::initializer_list<UnsignedInt> styleFeatureCounts, std::initializer_list<Int> styleCursorStyles, std::initializer_list<Int> styleSelectionStyles, std::initializer_list<Vector4> stylePaddings);
        #endif

    private:
        struct State;
        friend TextLayerGL;

        void doSetStyle(const TextLayerCommonStyleUniform& commonUniform, Containers::ArrayView<const TextLayerStyleUniform> uniforms) override;
        void doSetEditingStyle(const TextLayerCommonEditingStyleUniform& commonUniform, Containers::ArrayView<const TextLayerEditingStyleUniform> uniforms) override;
};

inline TextLayerGL::Shared& TextLayerGL::shared() {
    return static_cast<Shared&>(TextLayer::shared());
}

inline const TextLayerGL::Shared& TextLayerGL::shared() const {
    return static_cast<const Shared&>(TextLayer::shared());
}

}}
#else
#error this header is available only in the OpenGL build
#endif

#endif
