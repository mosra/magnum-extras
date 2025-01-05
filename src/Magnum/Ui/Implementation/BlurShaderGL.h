#ifndef Magnum_Ui_Implementation_BlurShaderGL_h
#define Magnum_Ui_Implementation_BlurShaderGL_h
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

#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/Math/Vector2.h>

#include "Magnum/Ui/visibility.h"

/* Extracted out of BaseLayerGL for easier testing and benchmarking. The
   BlurShaderGL constructor is implemented in BaseLayerGL.cpp, so it's not in
   an anonymous namespace. */

namespace Magnum { namespace Ui {

class
/* Exported only for tests (which have graceful assert enabled) */
#ifdef CORRADE_GRACEFUL_ASSERT
MAGNUM_UI_EXPORT
#endif
BlurShaderGL: public GL::AbstractShaderProgram {
    private:
        enum: Int {
            /* Using a texture binding hopefully different from all others to
               not stomp over bindings used by other shaders (0 for the text
               layer glyph texture, 7 for distance field processing). Certain
               devices may have just 8 texture binding slots in total, so avoid
               anything after. */
            TextureBinding = 6
        };

    public:
        typedef GL::Attribute<0, Vector2> Position;

        explicit BlurShaderGL(NoCreateT): GL::AbstractShaderProgram{NoCreate} {}
        explicit BlurShaderGL(UnsignedInt radius, Float limit);

        BlurShaderGL& setProjection(const Vector2& scaling) {
            /* Y-flipped scale from the UI size to the 2x2 unit square, the
               shader then translates by (-1, 1) on its own to put the
               origin at center */
            setUniform(_projectionUniform, Vector2{2.0f, -2.0f}/scaling);
            return *this;
        }

        BlurShaderGL& setDirection(const Vector2& direction) {
            /* If we check just the center pixel, the direction isn't used by
               the shader at all */
            if(_sampleCount != 1)
                setUniform(_directionUniform, direction);
            return *this;
        }

        BlurShaderGL& bindTexture(GL::Texture2D& texture) {
            texture.bind(TextureBinding);
            return *this;
        }

    private:
        UnsignedInt _sampleCount;
        Int _projectionUniform = 0,
            _directionUniform = 1;
};

}}

#endif
