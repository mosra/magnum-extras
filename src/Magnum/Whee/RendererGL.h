#ifndef Magnum_Whee_RendererGL_h
#define Magnum_Whee_RendererGL_h
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
 * @brief Class @ref Magnum::Whee::RendererGL
 * @m_since_latest
 */

#include "Magnum/configure.h"

#ifdef MAGNUM_TARGET_GL
#include "Magnum/Whee/AbstractRenderer.h"

namespace Magnum { namespace Whee {

/**
@brief OpenGL renderer implementation
@m_since_latest

Meant to be supplied to @ref AbstractUserInterface::setRendererInstance(). If
you're using the @ref UserInterfaceGL class, it's done automatically.

The renderer expects pre-multiplied blending set up, as shown below. Internally
it enables @ref GL::Renderer::Feature::Blending and/or
@ref GL::Renderer::Feature::ScissorTest for layers that advertise
@ref LayerFeature::DrawUsesBlending and/or @ref LayerFeature::DrawUsesScissor,
the scissor rectangle is then reset back to the whole framebuffer size (as
supplied to the user interface constructor or
@ref AbstractUserInterface::setSize()) after drawing.

@snippet Whee-gl.cpp RendererGL

@note This class is available only if Magnum is compiled with
    @ref MAGNUM_TARGET_GL enabled (done by default). See @ref building-features
    for more information.
*/
class MAGNUM_WHEE_EXPORT RendererGL: public AbstractRenderer {
    public:
        /** @brief Constructor */
        explicit RendererGL();

        /** @brief Copying is not allowed */
        RendererGL(const RendererGL&) = delete;

        /**
         * @brief Move constructor
         *
         * Performs a destructive move, i.e. the original object isn't usable
         * afterwards anymore.
         */
        RendererGL(RendererGL&&) noexcept;

        ~RendererGL();

        /** @brief Copying is not allowed */
        RendererGL& operator=(const RendererGL&) = delete;

        /** @brief Move assignment */
        RendererGL& operator=(RendererGL&&) noexcept;

    private:
        MAGNUM_WHEE_LOCAL RendererFeatures doFeatures() const override;
        MAGNUM_WHEE_LOCAL void doSetupFramebuffers(const Vector2i& size) override;
        MAGNUM_WHEE_LOCAL void doTransition(RendererTargetState targetStateFrom, RendererTargetState targetStateTo, RendererDrawStates drawStatesFrom, RendererDrawStates drawStatesTo) override;

        struct State;
        Containers::Pointer<State> _state;
};

}}
#else
#error this header is available only in the OpenGL build
#endif

#endif
