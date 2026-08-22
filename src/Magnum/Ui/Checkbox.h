#ifndef Magnum_Ui_Checkbox_h
#define Magnum_Ui_Checkbox_h
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
 * @brief Class @ref Magnum::Ui::Checkbox, function @ref Magnum::Ui::checkbox(), @ref Magnum::Ui::radioButton(), enum @ref Magnum::Ui::CheckboxStyle
 * @m_since_latest_{extras}
 */

#include <Corrade/Utility/Macros.h> /* CORRADE_NODISCARD() */

#include "Magnum/Ui/Widget.h"

namespace Magnum { namespace Ui {

/**
@brief Checkbox style
@m_since_latest_{extras}

@see @ref Checkbox, @ref checkbox(), @ref radioButton()
*/
enum class CheckboxStyle: UnsignedByte {
    Checkbox,       /**< Checkbox */

    /**
     * Radio button. This is the implicit style when using the
     * @ref radioButton() function.
     */
    RadioButton
};

/**
@debugoperatorenum{CheckboxStyle}
@m_since_latest_{extras}
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, CheckboxStyle value);

/**
@brief Checkbox widget
@m_since_latest_{extras}
*/
class MAGNUM_UI_EXPORT Checkbox: public Widget {
    public:
        /**
         * @brief Constructor
         * @param anchor            Positioning anchor
         * @param text              Label text. Passing an empty string
         *      makes it just a checkbox with no label
         * @param textProperties    Text shaping and layouting properties
         * @param style             Checkbox style
         *
         * Creates an internal data storage instance to maintain the checked
         * state. Use @ref Checkbox(Anchor, const StorageQuery<bool>&, Containers::StringView, const TextProperties&, CheckboxStyle)
         * to supply an external data binding.
         */
        explicit Checkbox(Anchor anchor, Containers::StringView text, const TextProperties& textProperties, CheckboxStyle style = CheckboxStyle::Checkbox);
        /** @overload */
        explicit Checkbox(Anchor anchor, Containers::StringView text, CheckboxStyle style = CheckboxStyle::Checkbox);

        /**
         * @brief Construct a checkbox with a data binding for the checked state
         * @param anchor            Positioning anchor
         * @param query             @ref DataLayer storage query
         * @param text              Label text. Passing an empty string
         *      makes it just a checkbox with no label
         * @param textProperties    Text shaping and layouting properties
         * @param style             Checkbox style
         *
         * If the query is mutable, it's expected to support both
         * @ref StorageOperation::Set and @ref StorageOperation::Toggle. If
         * it's immutable, @ref setChecked(), @ref toggleChecked() and clicking
         * or tapping the widget does nothing.
         * @see @ref StorageQuery::isMutable(), @ref StorageQuery::operations()
         */
        explicit Checkbox(Anchor anchor, const StorageQuery<bool>& query, Containers::StringView text, const TextProperties& textProperties, CheckboxStyle style = CheckboxStyle::Checkbox);
        /** @overload */
        explicit Checkbox(Anchor anchor, const StorageQuery<bool>& query, Containers::StringView text, CheckboxStyle style = CheckboxStyle::Checkbox);

        /* No NonOwned variant without a StorageQuery, as it doesn't make sense
           to have a checkbox for which you cannot query the state later */

        /**
         * @brief Construct a non-owned checkbox with a data binding for the checked state
         * @param anchor            Positioning anchor
         * @param query             @ref DataLayer storage query
         * @param text              Label text. Passing an empty string
         *      makes it just a checkbox with no label
         * @param textProperties    Text shaping and layouting properties
         * @param style             Checkbox style
         *
         * Like @ref Checkbox(Anchor, const StorageQuery<bool>&, Containers::StringView, const TextProperties&, CheckboxStyle)
         * but the widget node doesn't get removed on destruction. Instead, it
         * gets removed either once any parent node is removed, or when
         * @ref AbstractUserInterface::removeNode() is explicitly called on
         * @ref node().
         * @see @ref isOwned(), @ref checkbox(Anchor, const StorageQuery<bool>&, Containers::StringView, const TextProperties&, CheckboxStyle)
         */
        explicit Checkbox(NonOwnedT, Anchor anchor, const StorageQuery<bool>& query, Containers::StringView text, const TextProperties& textProperties, CheckboxStyle style = CheckboxStyle::Checkbox);
        /** @overload */
        explicit Checkbox(NonOwnedT, Anchor anchor, const StorageQuery<bool>& query, Containers::StringView text, CheckboxStyle style = CheckboxStyle::Checkbox);

        /** @copydoc AbstractWidget::AbstractWidget(NoCreateT) */
        explicit Checkbox(NoCreateT): Widget{NoCreate}, _style{}, _backgroundData{}, _checkboxData{}, _textData{}, _dataBindingData{} {}

        /* No hasDataBinding() as that's true always */

        /** @brief Style */
        CheckboxStyle style() const { return _style; }

        /**
         * @brief Set style
         * @return Reference to self (for method chaining)
         *
         * @see @ref setText(), @ref setChecked(), @ref toggleChecked()
         */
        Checkbox& setStyle(CheckboxStyle style);

        /* No setStyle() taking a time there's no use case for animating a
           transition between a checkbox and a radio button (unlike e.g.
           transitioning a button from green to red) */

        /**
         * @brief Set label text
         * @return Reference to self (for method chaining)
         *
         * Passing an empty @p text removes the label.
         * @see @ref setStyle(), @ref setChecked(), @ref toggleChecked()
         */
        #ifdef DOXYGEN_GENERATING_OUTPUT
        Checkbox& setText(Containers::StringView text, const TextProperties& textProperties = {});
        #else
        /* To avoid having to include TextProperties.h */
        Checkbox& setText(Containers::StringView text, const TextProperties& textProperties);
        Checkbox& setText(Containers::StringView text);
        #endif

        /** @brief Whether the checkbox is checked */
        bool isChecked() const;

        /**
         * @brief Check or uncheck the checkbox
         * @return Reference to self (for method chaining)
         *
         * Note that whether the checkbox actually gets checked or unchecked
         * depends on the underlying storage implementation --- for example it
         * may not be possible to uncheck all from a set of radio buttons. Use
         * @ref isChecked() to verify the checked state afterwards, if needed.
         *
         * If the checkbox was created with an immutable data binding, this
         * function does nothing.
         * @see @ref toggleChecked(), @ref StorageQuery::isMutable()
         */
        Checkbox& setChecked(bool checked);

        /**
         * @brief Toggle a checkbox
         * @return Reference to self (for method chaining)
         *
         * Note that whether the checkbox actually gets checked or unchecked
         * depends on the underlying storage implementation --- for example it
         * may not be possible to uncheck all from a set of radio buttons. Use
         * @ref isChecked() to verify the checked state afterwards, if needed.
         *
         * If the checkbox was created with an immutable data binding, this
         * function does nothing.
         * @see @ref setChecked(), @ref StorageQuery::isMutable()
         */
        Checkbox& toggleChecked();

        /**
         * @brief Background data
         *
         * Exposed mainly for testing purposes, not meant to be modified
         * directly.
         */
        DataHandle backgroundData() const;

        /**
         * @brief Checkbox data
         *
         * Exposed mainly for testing purposes, not meant to be modified
         * directly.
         */
        DataHandle checkboxData() const;

        /**
         * @brief Text data
         *
         * Exposed mainly for testing purposes, not meant to be modified
         * directly.
         */
        DataHandle textData() const;

        /**
         * @brief Data binding data
         *
         * Exposed mainly for testing purposes, not meant to be modified
         * directly.
         */
        DataHandle dataBindingData() const { return _dataBindingData; }

        #ifndef DOXYGEN_GENERATING_OUTPUT
        _MAGNUM_UI_WIDGET_SUBCLASS_IMPLEMENTATION(Checkbox) /* LCOV_EXCL_LINE */
        #endif

    private:
        MAGNUM_UI_LOCAL void createInternal(const StorageQuery<bool>& query, Containers::StringView text, const TextProperties& textProperties);

        CheckboxStyle _style;
        /* 2 bytes free (_style fits into padding of Widget) */
        LayerDataHandle _backgroundData, _checkboxData, _textData;
        /* Unlike with other data, here the layer can be arbitrary, so storing
           a full handle */
        DataHandle _dataBindingData;
};

/**
@brief Stateless checkbox with a data binding for the checked state
@param anchor           Positioning anchor
@param query            @ref DataLayer storage query
@param text             Label text. Passing an empty string makes it just a
    checkbox with no label
@param textProperties    Text shaping and layouting properties
@param style             Checkbox style
@return The @p anchor verbatim
@m_since_latest_{extras}

Equivalent to constructing a non-owned @ref Checkbox using
@ref Checkbox::Checkbox(NonOwnedT, Anchor, const StorageQuery<bool>&, Containers::StringView, const TextProperties&, CheckboxStyle)
and discarding the stateful instance. See its documentation for more
information.
@see @ref radioButton()
*/
MAGNUM_UI_EXPORT Anchor checkbox(Anchor anchor, const StorageQuery<bool>& query, Containers::StringView text, const TextProperties& textProperties, CheckboxStyle style = CheckboxStyle::Checkbox);
/**
@overload
@m_since_latest_{extras}
*/
MAGNUM_UI_EXPORT Anchor checkbox(Anchor anchor, const StorageQuery<bool>& query, Containers::StringView text, CheckboxStyle style = CheckboxStyle::Checkbox);

/**
@brief Stateless radio button with a data binding for the checked state
@param anchor           Positioning anchor
@param query            @ref DataLayer storage query
@param text             Label text. Passing an empty string makes it just a
    checkbox with no label
@param textProperties    Text shaping and layouting properties
@return The @p anchor verbatim
@m_since_latest_{extras}

Equivalent to constructing a non-owned @ref Checkbox using
@ref Checkbox::Checkbox(NonOwnedT, Anchor, const StorageQuery<bool>&, Containers::StringView, const TextProperties&, CheckboxStyle)
with style set to @ref CheckboxStyle::RadioButton and discarding the stateful
instance. See its documentation for more information.
@see @ref checkbox()
*/
MAGNUM_UI_EXPORT Anchor radioButton(Anchor anchor, const StorageQuery<bool>& query, Containers::StringView text, const TextProperties& textProperties);
/**
@overload
@m_since_latest_{extras}
*/
MAGNUM_UI_EXPORT Anchor radioButton(Anchor anchor, const StorageQuery<bool>& query, Containers::StringView text);

}}

#endif
