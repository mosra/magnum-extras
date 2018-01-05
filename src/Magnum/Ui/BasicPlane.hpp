#ifndef Magnum_Ui_BasicPlane_hpp
#define Magnum_Ui_BasicPlane_hpp
/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018
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
 * @brief @ref compilation-speedup-hpp "Template implementation" for @ref BasicPlane.h
 */

#include "BasicPlane.h"

#include "Magnum/Ui/AbstractUiShader.h"
#include "Magnum/Ui/Widget.h"

namespace Magnum { namespace Ui {

template<class ...Layers> BasicPlane<Layers...>::~BasicPlane() = default;

template<class ...Layers> BasicUserInterface<Layers...>& BasicPlane<Layers...>::ui() {
    return static_cast<BasicUserInterface<Layers...>&>(AbstractPlane::ui());
}

template<class ...Layers> const BasicUserInterface<Layers...>& BasicPlane<Layers...>::ui() const {
    return static_cast<const BasicUserInterface<Layers...>&>(AbstractPlane::ui());
}

template<class ...Layers> void BasicPlane<Layers...>::update() {
    updateInternal(std::integral_constant<std::size_t, 0>{});
}

template<class ...Layers> template<std::size_t i> void BasicPlane<Layers...>::updateInternal(std::integral_constant<std::size_t, i>) {
    std::get<i>(_layers).update();
    updateInternal(std::integral_constant<std::size_t, i + 1>{});
}

template<class ...Layers> void BasicPlane<Layers...>::draw(const Matrix3& projectionMatrix, const std::array<std::reference_wrapper<AbstractUiShader>, sizeof...(Layers)>& shaders) {
    drawInternal(projectionMatrix*Matrix3::translation(rect().min()), shaders, std::integral_constant<std::size_t, 0>{});
}

template<class ...Layers> template<std::size_t i> void BasicPlane<Layers...>::drawInternal(const Matrix3& transformationProjectionMatrix, const std::array<std::reference_wrapper<AbstractUiShader>, sizeof...(Layers)>& shaders, std::integral_constant<std::size_t, i>) {
    shaders[i].get().setTransformationProjectionMatrix(transformationProjectionMatrix);
    std::get<i>(_layers).draw(shaders[i]);
    drawInternal(transformationProjectionMatrix, shaders, std::integral_constant<std::size_t, i + 1>{});
}

}}

#endif
