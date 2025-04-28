#ifndef Magnum_Ui_BaseLayerGL_h
#define Magnum_Ui_BaseLayerGL_h
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
 * @brief Class @ref Magnum::Ui::BaseLayerGL
 * @m_since_latest
 */

#include "Magnum/configure.h"

#ifdef MAGNUM_TARGET_GL
#include <Magnum/GL/GL.h>

#include "Magnum/Ui/BaseLayer.h"

namespace Magnum { namespace Ui {

/**
@brief OpenGL implementation of the base layer
@m_since_latest

See the @ref BaseLayer base class documentation for information about setting
up an instance of this layer and using it. The base class contains most of the
interface you'll be interacting with, this subclass exposes just the APIs tied
to OpenGL, such as texture setup.

The layer assumes @ref RendererGL is set on the user interface (or
@ref UserInterfaceGL used, which does so automatically), see its documentation
for more information about GL state expectations. The layer produces geometry
in a counter-clockwise winding, so @ref GL::Renderer::Feature::FaceCulling can
stay enabled when drawing it.

@note This class is available only if Magnum is compiled with
    @ref MAGNUM_TARGET_GL enabled (done by default). See @ref building-features
    for more information.
*/
class MAGNUM_UI_EXPORT BaseLayerGL: public BaseLayer {
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
         * @ref Shared::setStyle() was called. In case
         * @ref BaseLayerSharedFlag::Textured was enabled, additionally it's
         * expected that @ref setTexture() was called as well.
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

        /**
         * @brief Set a texture to draw with
         * @return Reference to self (for method chaining)
         *
         * Expects that the layer was constructed with a shared state that has
         * @ref BaseLayerSharedFlag::Textured. The @p texture is expected to
         * stay alive for as long as the layer is drawn. Use
         * @ref setTexture(GL::Texture2DArray&&) to make the layer take
         * ownership of the texture instead.
         * @see @ref setTextureCoordinates()
         */
        BaseLayerGL& setTexture(GL::Texture2DArray& texture);

        /**
         * @brief Set a texture to draw with, taking over its ownership
         * @return Reference to self (for method chaining)
         *
         * Compared to @ref setTexture(GL::Texture2DArray&) takes over
         * ownership of the texture instance.
         */
        BaseLayerGL& setTexture(GL::Texture2DArray&& texture);

        /* Overloads to remove a WTF factor from method chaining order */
        #ifndef DOXYGEN_GENERATING_OUTPUT
        BaseLayerGL& setBackgroundBlurPassCount(UnsignedInt count) {
            return static_cast<BaseLayerGL&>(BaseLayer::setBackgroundBlurPassCount(count));
        }
        BaseLayerGL& assignAnimator(BaseLayerStyleAnimator& animator) {
            return static_cast<BaseLayerGL&>(BaseLayer::assignAnimator(animator));
        }
        #endif

    protected:
        /**
         * @copybrief AbstractLayer::doComposite()
         *
         * It's possible for a subclass to override this function to perform
         * extra GL state changes and then delegate to the parent
         * implementation. As the implementation doesn't track current GL state
         * in any way at the moment, the state should be reset back to the
         * previous afterwards. See @ref AbstractLayer::doComposite() for more
         * information about how this function is called.
         */
        void doComposite(AbstractRenderer& renderer, const Containers::StridedArrayView1D<const Vector2>& compositingRectOffsets, const Containers::StridedArrayView1D<const Vector2>& compositingRectSizes, std::size_t offset, std::size_t count) override;

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
@brief Shared state for the OpenGL implementation of the base layer

Contains GPU shaders and style definitions. See the @ref BaseLayer class
documentation for information about setting up an instance of this layer and
using it.

In order to update or draw the layer it's expected that @ref setStyle() was
called, in case @ref BaseLayerSharedFlag::Textured is enabled additionally it's
expected that @ref setTexture() was called on the layer as well.
*/
class MAGNUM_UI_EXPORT BaseLayerGL::Shared: public BaseLayer::Shared {
    public:
        /** @brief Constructor */
        explicit Shared(const Configuration& configuration);

        /**
         * @brief Construct without creating the contents
         *
         * Doesn't touch any GL state. Move over a created instance to make it
         * useful. Passing a non-created instance to the @ref BaseLayerGL
         * constructor has undefined behavior and will likely crash.
         */
        explicit Shared(NoCreateT) noexcept;

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
