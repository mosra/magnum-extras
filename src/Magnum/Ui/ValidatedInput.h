#ifndef Magnum_Ui_ValidatedInput_h
#define Magnum_Ui_ValidatedInput_h
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

/** @file
 * @brief Class @ref Magnum::Ui::ValidatedInput
 */

#include <regex>
#include <Corrade/Interconnect/Receiver.h>

#include "Magnum/Ui/Input.h"

namespace Magnum { namespace Ui {

/**
@brief Validated input

A version of @ref Input that validates input against passed @ref std::regex.
Connects to the @ref valueChanged() signal and sets style to either
@ref Style::Default or @ref Style::Warning based on whether the value matches
the regular expression or not.
@experimental
*/
class MAGNUM_UI_EXPORT ValidatedInput: public Input, public Interconnect::Receiver {
    public:
        /**
         * @brief If values of all inputs are valid
         *
         * Convenience alternative to calling @ref isValid() in a loop. Returns
         * `false` if one of the passed input values is not valid, `true`
         * otherwise. In particular, returns `true` also for an empty list.
         */
        static bool allValid(std::initializer_list<std::reference_wrapper<const ValidatedInput>> inputs);

        /**
         * @brief Constructor
         * @param plane         Plane this widget is a part of
         * @param anchor        Positioning anchor
         * @param validator     Validator regex
         * @param value         Initial input value
         * @param maxValueSize  Max input text size
         * @param style         Widget style
         *
         * The caller is expected to keep @p validator in scope for the whole
         * instance lifetime.
         */
        explicit ValidatedInput(Plane& plane, const Anchor& anchor, const std::regex& validator, std::string value, std::size_t maxValueSize, Style style = Style::Default);

        /** @overload */
        explicit ValidatedInput(Plane& plane, const Anchor& anchor, std::regex&& validator, std::string value, std::size_t maxValueSize, Style style = Style::Default) = delete;

        /** @overload */
        explicit ValidatedInput(Plane& plane, const Anchor& anchor, const std::regex& validator, std::size_t maxValueSize, Style style = Style::Default);

        /** @overload */
        explicit ValidatedInput(Plane& plane, const Anchor& anchor, std::regex&& validator, std::size_t maxValueSize, Style style = Style::Default) = delete;

        ~ValidatedInput();

        /**
         * @brief If the value is valid
         *
         * @see @ref allValid()
         */
        bool isValid() const;

    private:
        void MAGNUM_UI_LOCAL updateStyle(const std::string&);

        const std::regex& _validator;
};

}}

#endif
