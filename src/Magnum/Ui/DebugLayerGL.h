#ifndef Magnum_Ui_DebugLayerGL_h
#define Magnum_Ui_DebugLayerGL_h
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

/**
 * @brief Class @ref Magnum::Ui::DebugLayerGL
 * @m_since_latest
 */

#include "Magnum/configure.h"

#ifdef MAGNUM_TARGET_GL
#include "Magnum/Ui/DebugLayer.h"

namespace Magnum { namespace Ui {

/**
@brief OpenGL implementation of the debug layer
@m_since_latest

Implements visual feedback for @ref DebugLayerFlag::NodeInspect. See the
@ref DebugLayer base class documentation for information about setting up an
instance of this layer and using it. The base class contains the whole
interface you'll be interacting with, thus you don't need to subsequently
access the derived type for anything. If you don't use
@ref DebugLayerFlag::NodeInspect or don't need the visual feedback, you can
instantiate just the @ref DebugLayer base class instead.

The layer assumes @ref RendererGL is set on the user interface (or
@ref UserInterfaceGL used, which does so automatically), see its documentation
for more information about GL state expectations. The layer produces geometry
in a counter-clockwise winding, so @ref GL::Renderer::Feature::FaceCulling can
stay enabled when drawing it.

@note This class is available only if Magnum is compiled with
    @ref MAGNUM_TARGET_GL enabled (done by default). See @ref building-features
    for more information.
*/
class MAGNUM_UI_EXPORT DebugLayerGL: public DebugLayer {
    public:
        /**
         * @brief Constructor
         * @param handle    Layer handle returned from
         *      @ref AbstractUserInterface::createLayer()
         * @param sources   Data sources to track
         * @param flags     Behavior flags
         */
        explicit DebugLayerGL(LayerHandle handle, DebugLayerSources sources, DebugLayerFlags flags = {});

    private:
        struct State;

        MAGNUM_UI_LOCAL LayerFeatures doFeatures() const override;
        MAGNUM_UI_LOCAL void doSetSize(const Vector2& size, const Vector2i& framebufferSize) override;
        MAGNUM_UI_LOCAL void doUpdate(LayerStates state, const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, const Containers::StridedArrayView1D<const Float>& nodeOpacities, Containers::BitArrayView nodesEnabled, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes, const Containers::StridedArrayView1D<const Vector2>& compositeRectOffsets, const Containers::StridedArrayView1D<const Vector2>& compositeRectSizes) override;
        MAGNUM_UI_LOCAL void doDraw(const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, std::size_t offset, std::size_t count, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, std::size_t clipRectOffset, std::size_t clipRectCount, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, const Containers::StridedArrayView1D<const Float>& nodeOpacities, Containers::BitArrayView nodesEnabled, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes) override;
};

}}
#else
#error this header is available only in the OpenGL build
#endif

#endif
