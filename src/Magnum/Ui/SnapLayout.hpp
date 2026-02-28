#ifndef Magnum_Ui_SnapLayout_hpp
#define Magnum_Ui_SnapLayout_hpp
/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023, 2024, 2025, 2026
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
 * @brief @ref compilation-speedup-hpp "Template implementation" for @ref SnapLayout.h
 * @m_since_latest_{extras}
 */

#include "SnapLayout.h"

#include <Corrade/Utility/Assert.h>

#include "Magnum/Ui/Anchor.h"

namespace Magnum { namespace Ui {

#ifndef DOXYGEN_GENERATING_OUTPUT
template<class UserInterface> BasicSnapLayout<UserInterface>::BasicSnapLayout(UserInterface& ui, const NodeHandle node): AbstractSnapLayout{NoInit, ui} {
    CORRADE_ASSERT(ui.hasSnapLayouter(),
        "Ui::BasicSnapLayout: SnapLayouter not present in the UI", );
    addLayout(ui.snapLayouter(), node);
}

template<class UserInterface> BasicSnapLayout<UserInterface>::BasicSnapLayout(UserInterface& ui, const LayoutHandle layout): AbstractSnapLayout{NoInit, ui} {
    CORRADE_ASSERT(ui.hasSnapLayouter(),
        "Ui::BasicSnapLayout: SnapLayouter not present in the UI", );
    useLayout(ui.snapLayouter(), layout);
}

template<class UserInterface> BasicSnapLayout<UserInterface>::BasicSnapLayout(UserInterface& ui, const LayouterDataHandle layout): AbstractSnapLayout{NoInit, ui} {
    CORRADE_ASSERT(ui.hasSnapLayouter(),
        "Ui::BasicSnapLayout: SnapLayouter not present in the UI", );
    useLayout(ui.snapLayouter(), layout);
}

template<class UserInterface> BasicSnapLayout<UserInterface>::BasicSnapLayout(const BasicAnchor<UserInterface>& anchor): BasicSnapLayout{anchor.ui(), anchor.node()} {}

template<class UserInterface> UserInterface& BasicSnapLayout<UserInterface>::ui() const {
    return static_cast<UserInterface&>(AbstractSnapLayout::ui());
}

template<class UserInterface> BasicSnapLayout<UserInterface>::operator BasicAnchor<UserInterface>() const {
    return BasicAnchor<UserInterface>{ui(), node()};
}

template<class UserInterface> BasicSnapLayout<UserInterface> BasicSnapLayout<UserInterface>::snapRoot(UserInterface& ui, const Snaps snap, const Vector2& nodeSize, const NodeFlags nodeFlags, const SnapLayoutFlags layoutFlags) {
    CORRADE_ASSERT(ui.hasSnapLayouter(),
        "Ui::BasicSnapLayout::snapRoot(): SnapLayouter not present in the UI",
        (BasicSnapLayout<UserInterface>{ui, {}, {}, {}}));
    return snapRoot(ui, ui.snapLayouter(), snap, nodeSize, nodeFlags, layoutFlags);
}
#endif

}}

#endif
