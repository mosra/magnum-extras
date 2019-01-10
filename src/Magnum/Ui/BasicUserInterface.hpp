#ifndef Magnum_Ui_BasicUserInterface_hpp
#define Magnum_Ui_BasicUserInterface_hpp
/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019
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

#include "BasicUserInterface.h"

#include <Magnum/Math/Matrix3.h>

#include "Magnum/Ui/BasicPlane.h"

namespace Magnum { namespace Ui {

template<class ...Layers> BasicUserInterface<Layers...>::~BasicUserInterface() {}

template<class ...Layers> void BasicUserInterface<Layers...>::update() {
    /** @todo Update only non-hidden to save cycles? */
    for(AbstractPlane& plane: *this) static_cast<BasicPlane<Layers...>&>(plane).update();
}

template<class ...Layers> void BasicUserInterface<Layers...>::draw(const std::array<std::reference_wrapper<AbstractUiShader>, sizeof...(Layers)>& shaders) {
    const Matrix3 projectionMatrix = Matrix3::scaling(2.0f/_size)*Matrix3::translation(-_size/2);
    for(AbstractPlane& plane: *this) {
        /* Ignore all hidden planes in the back, draw back-to-front */
        if(plane.flags() & PlaneFlag::Hidden) continue;
        static_cast<BasicPlane<Layers...>&>(plane).draw(projectionMatrix, shaders);
    }
}

}}

#endif
