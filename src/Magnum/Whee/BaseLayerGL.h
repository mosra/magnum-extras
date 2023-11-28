#ifndef Magnum_Whee_BaseLayerGL_h
#define Magnum_Whee_BaseLayerGL_h
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
 * @brief Class @ref Magnum::Whee::BaseLayerGL
 * @m_since_latest
 */

#include "Magnum/configure.h"

#ifdef MAGNUM_TARGET_GL
#include "Magnum/Whee/BaseLayer.h"

namespace Magnum { namespace Whee {

/**
@brief OpenGL implementation of the base layer
@m_since_latest

The layer expects pre-multiplied blending set up and scissor enabled in order
to draw correctly, as shown below. It produces geometry in a counter-clockwise
winding, so @ref GL::Renderer::Feature::FaceCulling can stay enabled when
drawing it. The scissor rectangle is reset back to the whole framebuffer size
(as supplied to the user interface constructor or
@ref AbstractUserInterface::setSize()) after drawing.

@snippet Whee-gl.cpp BaseLayerGL-renderer

@note This class is available only if Magnum is compiled with
    @ref MAGNUM_TARGET_GL enabled (done by default). See @ref building-features
    for more information.
*/
class MAGNUM_WHEE_EXPORT BaseLayerGL: public BaseLayer {
    public:
        class Shared;

        /**
         * @brief Constructor
         * @param handle    Layer handle returned from
         *      @ref AbstractUserInterface::createLayer()
         * @param shared    Shared state containing style data
         *
         * The @p shared state is expected to be kept in scope for the whole
         * class lifetime. In order to draw the layer it's expected that
         * @ref Shared::setStyle() was called.
         */
        explicit BaseLayerGL(LayerHandle handle, Shared& shared);

        /**
         * @brief Shared state used by this layer
         *
         * Reference to the instance passed to @ref BaseLayerGL::BaseLayerGL(LayerHandle, Shared&).
         */
        inline Shared& shared();
        /** @overload */
        inline const Shared& shared() const;

    private:
        struct State;

        MAGNUM_WHEE_LOCAL void doSetSize(const Vector2& size, const Vector2i& framebufferSize) override;

        MAGNUM_WHEE_LOCAL void doUpdate(const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes) override;

        MAGNUM_WHEE_LOCAL void doDraw(const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, std::size_t offset, std::size_t count, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, std::size_t clipRectOffset, std::size_t clipRectCount, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes) override;
};

/**
@brief Shared state for the OpenGL implementation of the base layer

Contains shader instances. In order to update or draw the layer it's expected
that @ref setStyle() was called.
*/
class MAGNUM_WHEE_EXPORT BaseLayerGL::Shared: public BaseLayer::Shared {
    public:
        /**
         * @brief Constructor
         *
         * The @p styleUniformCount parameter specifies the size of the uniform
         * array, @p styleCount then the number of distinct styles to use for
         * drawing. The sizes are independent in order to allow styles with
         * different paddings share the same uniform data. Both
         * @p styleUniformCount and @p styleCount is expected to be non-zero.
         * Style data are then set with @ref setStyle().
         */
        explicit Shared(UnsignedInt styleUniformCount, UnsignedInt styleCount);

        /**
         * @brief Construct with style uniform count being the same as style count
         *
         * Equivalent to calling @ref Shared(UnsignedInt, UnsignedInt) with
         * both parameters set to @p styleCount.
         */
        explicit Shared(UnsignedInt styleCount): Shared{styleCount, styleCount} {}

        /**
         * @brief Construct without creating the contents
         *
         * Doesn't touch any GL state. Move over a created instance to make it
         * useful. Passing a non-created instance to the @ref BaseLayerGL
         * constructor has undefined behavior and will likely crash.
         */
        explicit Shared(NoCreateT) noexcept;

        /* Overloads to remove a WTF factor from method chaining order */
        #ifndef DOXYGEN_GENERATING_OUTPUT
        MAGNUMEXTRAS_WHEE_ABSTRACTVISUALLAYER_SHARED_SUBCLASS_IMPLEMENTATION()
        Shared& setStyle(const BaseLayerCommonStyleUniform& commonUniform, Containers::ArrayView<const BaseLayerStyleUniform> uniforms, const Containers::StridedArrayView1D<const Vector4>& paddins);
        Shared& setStyle(const BaseLayerCommonStyleUniform& commonUniform, std::initializer_list<BaseLayerStyleUniform> uniforms, std::initializer_list<Vector4> paddings);
        Shared& setStyle(const BaseLayerCommonStyleUniform& commonUniform, Containers::ArrayView<const BaseLayerStyleUniform> uniforms, const Containers::StridedArrayView1D<const UnsignedInt>& styleToUniform, const Containers::StridedArrayView1D<const Vector4>& stylePaddings);
        Shared& setStyle(const BaseLayerCommonStyleUniform& commonUniform, std::initializer_list<BaseLayerStyleUniform> uniforms, std::initializer_list<UnsignedInt> styleToUniform, std::initializer_list<Vector4> stylePaddings);
        #endif

    private:
        struct State;
        friend BaseLayerGL;

        void doSetStyle(const BaseLayerCommonStyleUniform& commonUniform, Containers::ArrayView<const BaseLayerStyleUniform> uniforms) override;
};

inline BaseLayerGL::Shared& BaseLayerGL::shared() {
    return static_cast<Shared&>(BaseLayer::shared());
}

inline const BaseLayerGL::Shared& BaseLayerGL::shared() const {
    return static_cast<const Shared&>(BaseLayer::shared());
}

}}
#else
#error this header is available only in the OpenGL build
#endif

#endif
