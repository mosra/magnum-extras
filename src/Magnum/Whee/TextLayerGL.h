#ifndef Magnum_Whee_TextLayerGL_h
#define Magnum_Whee_TextLayerGL_h
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

/** @file
 * @brief Class @ref Magnum::Whee::TextLayerGL
 * @m_since_latest
 */

#include "Magnum/configure.h"

#ifdef MAGNUM_TARGET_GL
#include "Magnum/Whee/TextLayer.h"

namespace Magnum { namespace Whee {

/**
@brief OpenGL implementation of the text layer
@m_since_latest

The layer expects pre-multiplied blending set up in order to draw correctly, as
shown below. It produces geometry in a counter-clockwise winding, so
@ref GL::Renderer::Feature::FaceCulling can stay enabled when drawing it.

@snippet Whee-gl.cpp TextLayerGL-renderer

@note This class is available only if Magnum is compiled with
    @ref MAGNUM_TARGET_GL enabled (done by default). See @ref building-features
    for more information.
*/
class MAGNUM_WHEE_EXPORT TextLayerGL: public TextLayer {
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

    private:
        struct State;

        MAGNUM_WHEE_LOCAL void doSetSize(const Vector2& size, const Vector2i& framebufferSize) override;

        MAGNUM_WHEE_LOCAL void doUpdate(const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes) override;

        MAGNUM_WHEE_LOCAL void doDraw(const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, std::size_t offset, std::size_t count, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, std::size_t clipRectOffset, std::size_t clipRectCount, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes) override;
};

/**
@brief Shared state for the OpenGL implementation of the text layer

Contains fonts, shader instances and style data. In order to draw the layer
it's expected that @ref setStyle() was called.
*/
class MAGNUM_WHEE_EXPORT TextLayerGL::Shared: public TextLayer::Shared {
    public:
        /**
         * @brief Constructor
         *
         * The @p styleCount parameter specifies the number of distinct styles
         * to use for drawing and is expected to be non-zero. Style data are
         * then set with @ref setStyle().
         */
        explicit Shared(UnsignedInt styleCount);

        /**
         * @brief Construct without creating the contents
         *
         * Doesn't touch any GL state. Move over a created instance to make it
         * useful. Passing a non-created instance to the @ref TextLayerGL
         * constructor has undefined behavior and will likely crash.
         */
        explicit Shared(NoCreateT) noexcept;

        /**
         * @brief Set a glyph cache instance
         * @return Reference to self (for method chaining)
         *
         * Has to be called before any @ref addFont(), is expected to be called
         * exactly once. Use the @ref setGlyphCache(Text::GlyphCache&&)
         * overload to make the shared state take over the glyph cache
         * instance.
         */
        Shared& setGlyphCache(Text::GlyphCache& cache);

        /**
         * @brief Set a glyph cache instance and take over its ownership
         * @return Reference to self (for method chaining)
         *
         * Like @ref setGlyphCache(Text::GlyphCache&), but the shared state
         * takes over the glyph cache ownership. You can access the instance
         * using @ref glyphCache() later.
         */
        Shared& setGlyphCache(Text::GlyphCache&& cache);

        /**
         * @brief Set style data
         * @return Reference to self (for method chaining)
         *
         * The style is expected to be a tightly packed struct consisting of
         * one @ref TextLayerStyleCommon instance followed by
         * @ref styleCount() const instances of @ref TextLayerStyleItem. For
         * example, for three different styles the setup could look like this.
         * Or the @ref TextLayerStyleItem instances could be in a three-item
         * array.
         *
         * @snippet Whee-gl.cpp TextLayerGL-setStyle
         */
        template<class T> Shared& setStyle(const T& style) {
            #ifndef CORRADE_NO_STD_IS_TRIVIALLY_TRAITS
            static_assert(std::is_trivially_copyable<T>::value, "style data not trivially copyable");
            #endif
            return setStyleInternal(&style, sizeof(style));
        }

        /* Overloads to remove a WTF factor from method chaining order */
        #ifndef DOXYGEN_GENERATING_OUTPUT
        MAGNUMEXTRAS_WHEE_ABSTRACTVISUALLAYER_SHARED_SUBCLASS_IMPLEMENTATION()
        Shared& setStyleFonts(const Containers::StridedArrayView1D<const FontHandle>& handles) {
            return static_cast<Shared&>(TextLayer::Shared::setStyleFonts(handles));
        }
        Shared& setStyleFonts(std::initializer_list<FontHandle> handles) {
            return static_cast<Shared&>(TextLayer::Shared::setStyleFonts(handles));
        }
        #endif

    private:
        struct State;
        friend TextLayerGL;

        Shared& setStyleInternal(const void* style, std::size_t size);
};

}}
#else
#error this header is available only in the OpenGL build
#endif

#endif
