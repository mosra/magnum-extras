/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021 Vladimír Vondruš <mosra@centrum.cz>

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

#include "ValidatedInput.h"

#include <Corrade/Containers/Reference.h>

#ifdef _MSC_VER
#include "Magnum/Ui/BasicUserInterface.h"  /* Why? */
#include "Magnum/Ui/Plane.h"
#endif

namespace Magnum { namespace Ui {

bool ValidatedInput::allValid(std::initializer_list<Containers::Reference<const ValidatedInput>> inputs) {
    for(const ValidatedInput& i: inputs) if(!i.isValid()) return false;
    return true;
}

ValidatedInput::ValidatedInput(Plane& plane, const Anchor& anchor, const std::regex& validator, std::string value, const std::size_t maxValueSize, const Style style): Input{plane, anchor, std::move(value), maxValueSize, style}, _validator{validator} {
    if(!isValid()) setStyle(Ui::Style::Warning);
    Interconnect::connect(*this, &Ui::Input::valueChanged, *this, &Ui::ValidatedInput::updateStyle);
}

ValidatedInput::ValidatedInput(Plane& plane, const Anchor& anchor, const std::regex& validator, const std::size_t maxValueSize, const Style style): ValidatedInput{plane, anchor, validator, {}, maxValueSize, style} {}

ValidatedInput::~ValidatedInput() {}

bool ValidatedInput::isValid() const { return std::regex_match(value(), _validator); }

void ValidatedInput::updateStyle(const std::string&) {
    setStyle(isValid() ? Ui::Style::Default : Ui::Style::Warning);
}

}}
