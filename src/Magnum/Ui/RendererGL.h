#ifndef Magnum_Ui_RendererGL_h
#define Magnum_Ui_RendererGL_h
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
 * @brief Class @ref Magnum::Ui::RendererGL
 * @m_since_latest
 */

#include "Magnum/configure.h"

#ifdef MAGNUM_TARGET_GL
#include <Magnum/GL/GL.h>

#include "Magnum/Ui/AbstractRenderer.h"

namespace Magnum { namespace Ui {

/**
@brief OpenGL renderer implementation
@m_since_latest

Performs renderer state management for OpenGL layer implementations such as
@ref BaseLayerGL or @ref TextLayerGL.

@section Ui-RendererGL-setup Setting up a renderer instance

If you use one of the @ref UserInterfaceGL constructors taking a style,
@ref UserInterfaceGL::create(), @relativeref{UserInterfaceGL,tryCreate()},
@relativeref{UserInterfaceGL,setStyle()} or
@relativeref{UserInterfaceGL,trySetStyle()}, an implicit renderer instance is
already set up by those. If you don't, or if you want to set up a
custom-configured renderer before specifying a style, pass its instance to
@ref AbstractUserInterface::setRendererInstance():

@snippet Ui-gl.cpp RendererGL-setup

When @ref AbstractUserInterface::draw() is executed, the renderer internally
enables @ref GL::Renderer::Feature::Blending and/or
@relativeref{GL::Renderer,Feature::ScissorTest} for layers that advertise
@ref LayerFeature::DrawUsesBlending and/or @ref LayerFeature::DrawUsesScissor,
the scissor rectangle is then reset back to the whole framebuffer size (as
supplied to the user interface constructor or
@ref AbstractUserInterface::setSize()) after drawing.

@section Ui-RendererGL-compositing-framebuffer Use with a compositing framebuffer

By default, the @ref RendererGL instance assumes *some* framebuffer is bound
for drawing and it doesn't touch the binding in any way. Layers that implement
compositing operations however need a framebuffer which can be both drawn into
and read from, which is achieved by constructing the renderer with
@link Flag::CompositingFramebuffer @endlink:

@snippet Ui-sdl2.cpp RendererGL-compositing-framebuffer

With the flag enabled, the application is then responsible for clearing the
@ref compositingFramebuffer() at frame start, drawing all content underneath
the UI to it, and ultimately blitting it back to the main / default application
framebuffer after the UI is drawn:

@snippet Ui-sdl2.cpp RendererGL-compositing-framebuffer-draw

@note This class is available only if Magnum is compiled with
    @ref MAGNUM_TARGET_GL enabled (done by default). See @ref building-features
    for more information.
*/
class MAGNUM_UI_EXPORT RendererGL: public AbstractRenderer {
    public:
        /**
         * @brief Renderer flag
         *
         * @see @ref Flags, @ref RendererGL(Flags), @ref flags()
         */
        enum class Flag: UnsignedByte {
            /**
             * Create a framebuffer to be used as a target for drawing all UI
             * contents and a source for compositing operations implemented by
             * various layers.
             *
             * The framebuffer, with a single @ref GL::TextureFormat::RGBA8
             * color attachment, is created on the first call to
             * @ref setupFramebuffers(), which is called as a
             * consequence of @ref AbstractUserInterface::setSize() or a
             * user interface constructor taking a size parameter, and is
             * recreated on all following @ref AbstractUserInterface::setSize()
             * calls.
             *
             * Then application is then responsible for clearing the
             * @ref compositingFramebuffer() at frame start, drawing all
             * content underneath the UI to it, and ultimately blitting it back
             * to the main / default application framebuffer after the UI is
             * drawn.
             */
            CompositingFramebuffer = 1 << 0,
        };

        /**
         * @brief Renderer flags
         *
         * @see @ref RendererGL(Flags), @ref flags()
         */
        typedef Containers::EnumSet<Flag> Flags;

        /** @brief Constructor */
        explicit RendererGL(Flags flags = {});

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

        /** @brief Renderer flags */
        Flags flags() const;

        /**
         * @brief Compositing framebuffer instance
         *
         * Available only if the renderer was constructed with
         * @ref Flag::CompositingFramebuffer and only after framebuffer sizes
         * were set up with @ref setupFramebuffers(), which is called as a
         * consequence of @ref AbstractUserInterface::setSize() or a
         * user interface constructor taking a size parameter. The viewport is
         * implicitly set to the whole @ref framebufferSize().
         *
         * With a compositing framebuffer enabled, the application is
         * responsible for clearing the framebuffer at frame start, drawing all
         * content underneath the UI to it, and ultimately blitting it back to
         * the main / default application framebuffer after the UI is drawn.
         * @see @ref flags()
         */
        GL::Framebuffer& compositingFramebuffer();
        const GL::Framebuffer& compositingFramebuffer() const; /**< @overload */

        /**
         * @brief Compositing framebuffer texture instance
         *
         * Available only if the renderer was constructed with
         * @ref Flag::CompositingFramebuffer and only after framebuffer sizes
         * were set up with @ref setupFramebuffers(), which is called as a
         * consequence of @ref AbstractUserInterface::setSize() or a
         * user interface constructor taking a size parameter. The texture is
         * implicitly set to a single @ref GL::TextureFormat::RGBA8 level of
         * @ref framebufferSize(), with both minification and magnification
         * filter being @ref GL::SamplerFilter::Linear and with
         * @ref GL::SamplerWrapping::ClampToEdge.
         *
         * The texture is meant to be accessed inside an
         * @ref AbstractLayer::doComposite() implementation. In other cases,
         * such as in an @ref AbstractLayer::doDraw(), using it may lead to a
         * framebuffer corruption as it would be used for both sampling and as
         * a framebuffer target.
         * @see @ref flags()
         */
        GL::Texture2D& compositingTexture();
        const GL::Texture2D& compositingTexture() const; /**< @overload */

    private:
        MAGNUM_UI_LOCAL RendererFeatures doFeatures() const override;
        MAGNUM_UI_LOCAL void doSetupFramebuffers(const Vector2i& size) override;
        MAGNUM_UI_LOCAL void doTransition(RendererTargetState targetStateFrom, RendererTargetState targetStateTo, RendererDrawStates drawStatesFrom, RendererDrawStates drawStatesTo) override;

        struct State;
        Containers::Pointer<State> _state;
};

CORRADE_ENUMSET_OPERATORS(RendererGL::Flags)

/**
@debugoperatorclassenum{RendererGL,RendererGL::Flag}
@m_since_latest
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, RendererGL::Flag value);

/**
@debugoperatorclassenum{RendererGL,RendererGL::Flags}
@m_since_latest
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, RendererGL::Flags value);

}}
#else
#error this header is available only in the OpenGL build
#endif

#endif
