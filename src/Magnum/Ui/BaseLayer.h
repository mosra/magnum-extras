#ifndef Magnum_Ui_BaseLayer_h
#define Magnum_Ui_BaseLayer_h
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
 * @brief Class @ref Magnum::Ui::BaseLayer, struct @ref Magnum::Ui::BaseLayerCommonStyleUniform, @ref Magnum::Ui::BaseLayerStyleUniform, enum @ref Magnum::Ui::BaseLayerSharedFlag, enum set @ref Magnum::Ui::BaseLayerSharedFlags
 * @m_since_latest
 */

#include <initializer_list>
#include <Magnum/Math/Color.h>

#include "Magnum/Ui/AbstractVisualLayer.h"

namespace Magnum { namespace Ui {

/**
@brief Properties common to all @ref BaseLayer style uniforms
@m_since_latest

See the @ref BaseLayer class documentation for information about setting up an
instance of the base layer and using it.

Together with one or more @ref BaseLayerStyleUniform instances contains style
properties that are used by the @ref BaseLayer shaders to draw the layer data,
packed in a form that allows direct usage in uniform buffers. Is uploaded
using @ref BaseLayer::Shared::setStyle(), style data that aren't used by the
shader are passed to the function separately.
*/
struct BaseLayerCommonStyleUniform {
    /** @brief Construct with default values */
    constexpr explicit BaseLayerCommonStyleUniform(DefaultInitT = DefaultInit) noexcept: smoothness{0.0f}, innerOutlineSmoothness{0.0f}, backgroundBlurAlpha{1.0f} {}

    /** @brief Constructor */
    constexpr /*implicit*/ BaseLayerCommonStyleUniform(Float smoothness, Float innerOutlineSmoothness, Float blurredBackgroundAlpha): smoothness{smoothness}, innerOutlineSmoothness{innerOutlineSmoothness}, backgroundBlurAlpha{blurredBackgroundAlpha} {}

    /** @brief Construct without blur parameters */
    constexpr /*implicit*/ BaseLayerCommonStyleUniform(Float smoothness, Float innerOutlineSmoothness): smoothness{smoothness}, innerOutlineSmoothness{innerOutlineSmoothness}, backgroundBlurAlpha{1.0f} {}

    /** @brief Construct without blur parameters with the @ref smoothness and @ref innerOutlineSmoothness fields set to the same value */
    constexpr /*implicit*/ BaseLayerCommonStyleUniform(Float smoothness): BaseLayerCommonStyleUniform{smoothness, smoothness} {}

    /** @todo once there's more than one parameters, there could be also a
        variant taking a single smoothness value and all blur parameters? */

    /** @brief Construct without initializing the contents */
    explicit BaseLayerCommonStyleUniform(NoInitT) noexcept {}

    /** @{
     * @name Convenience setters
     *
     * Provided to allow the use of method chaining for populating a structure
     * in a single expression, otherwise equivalent to accessing the fields
     * directly. Also guaranteed to provide backwards compatibility when
     * packing of the actual fields changes.
     */

    /**
     * @brief Set the @ref smoothness and @ref innerOutlineSmoothness fields
     * @return Reference to self (for method chaining)
     */
    CORRADE_CONSTEXPR14 BaseLayerCommonStyleUniform& setSmoothness(Float smoothness, Float innerOutlineSmoothness) {
        this->smoothness = smoothness;
        this->innerOutlineSmoothness = innerOutlineSmoothness;
        return *this;
    }

    /**
     * @brief Set the @ref smoothness and @ref innerOutlineSmoothness fields to the same value
     * @return Reference to self (for method chaining)
     */
    CORRADE_CONSTEXPR14 BaseLayerCommonStyleUniform& setSmoothness(Float smoothness) {
        this->smoothness = smoothness;
        this->innerOutlineSmoothness = smoothness;
        return *this;
    }

    /**
     * @brief Set the @ref backgroundBlurAlpha field
     * @return Reference to self (for method chaining)
     */
    CORRADE_CONSTEXPR14 BaseLayerCommonStyleUniform& setBackgroundBlurAlpha(Float alpha) {
        this->backgroundBlurAlpha = alpha;
        return *this;
    }

    /**
     * @}
     */

    /**
     * @brief Edge smoothness radius
     *
     * In framebuffer pixels (as opposed to UI units). E.g., setting the value
     * to @cpp 1.0f @ce will make the smoothing extend 1 pixel on each side of
     * the edge. Default value is @cpp 0.0f @ce.
     * @see @ref Ui-AbstractUserInterface-dpi
     */
    Float smoothness;

    /**
     * @brief Inner outline edge smoothness radius
     *
     * In framebuffer pixels (as opposed to UI units). E.g., setting the value
     * to @cpp 1.0f @ce will make the smoothing extend 1 pixel on each side of
     * the edge. Default value is @cpp 0.0f @ce. Not used if
     * @ref BaseLayerSharedFlag::NoOutline is enabled.
     * @see @ref Ui-AbstractUserInterface-dpi
     */
    Float innerOutlineSmoothness;

    /**
     * @brief Blurred background alpha
     *
     * If @ref BaseLayerSharedFlag::BackgroundBlur is enabled, the alpha value
     * of @ref BaseLayerStyleUniform::topColor,
     * @relativeref{BaseLayerStyleUniform,bottomColor} and
     * @relativeref{BaseLayerStyleUniform,outlineColor} is used to interpolate
     * between the color value and the blurred background. Making this value
     * less than @cpp 1.0f @ce makes the original unblurred framebuffer
     * contents show through as well, which can be used to achieve a glow-like
     * effect. Default value is @cpp 1.0f @ce. A similar effect can also be
     * achieved using @ref BaseLayerSharedFlag::TextureMask and pixel alpha
     * values between @cpp 0.0f @ce and @cpp 1.0f @ce.
     *
     * @m_class{m-row}
     *
     * @parblock
     *
     * @m_div{m-col-s-6 m-text-center m-nopadt m-nopadx}
     * @image html ui-baselayer-flag-blur.png width=256px
     * Default value of @cpp 1.0f @ce
     * @m_enddiv
     *
     * @m_div{m-col-s-6 m-text-center m-nopadt m-nopadx}
     * @image html ui-baselayer-flag-blur-alpha.png width=256px
     * Value of @cpp 0.75f @ce
     * @m_enddiv
     *
     * @endparblock
     */
    Float backgroundBlurAlpha;

    #ifndef DOXYGEN_GENERATING_OUTPUT
    Int:32;
    #endif
};

/**
@brief @ref BaseLayer style uniform
@m_since_latest

See the @ref BaseLayer class documentation for information about setting up an
instance of the base layer and using it.

Instances of this class together with @ref BaseLayerCommonStyleUniform contain
style properties that are used by the @ref BaseLayer shaders to draw the layer
data, packed in a form that allows direct usage in uniform buffers. Total count
of styles is specified with the
@ref BaseLayer::Shared::Configuration::Configuration() constructor, uniforms
are then uploaded using @ref BaseLayer::Shared::setStyle(), style data that
aren't used by the shader are passed to the function separately. If dynamic
styles are enabled with @ref BaseLayer::Shared::Configuration::setDynamicStyleCount(),
instances of this class are also passed to @ref BaseLayer::setDynamicStyle().
*/
struct BaseLayerStyleUniform {
    /** @brief Construct with default values */
    constexpr explicit BaseLayerStyleUniform(DefaultInitT = DefaultInit) noexcept: topColor{1.0f}, bottomColor{1.0f}, outlineColor{1.0f}, outlineWidth{0.0f}, cornerRadius{0.0f}, innerOutlineCornerRadius{0.0f} {}

    /** @brief Constructor */
    constexpr /*implicit*/ BaseLayerStyleUniform(const Color4& topColor, const Color4& bottomColor, const Color4& outlineColor, const Vector4& outlineWidth, const Vector4& cornerRadius, const Vector4& innerOutlineCornerRadius): topColor{topColor}, bottomColor{bottomColor}, outlineColor{outlineColor}, outlineWidth{outlineWidth}, cornerRadius{cornerRadius}, innerOutlineCornerRadius{innerOutlineCornerRadius} {}

    /**
     * @brief Construct with all corners having the same radius and all edges the same outline width
     *
     * The @ref outlineWidth, @ref cornerRadius and
     * @ref innerOutlineCornerRadius fields have all components set to the
     * values of @p outlineWidth, @p cornerRadius and
     * @p innerOutlineCornerRadius.
     */
    constexpr /*implicit*/ BaseLayerStyleUniform(const Color4& topColor, const Color4& bottomColor, const Color4& outlineColor, Float outlineWidth, Float cornerRadius, Float innerOutlineCornerRadius): BaseLayerStyleUniform{topColor, bottomColor, outlineColor, Vector4{outlineWidth}, Vector4{cornerRadius}, Vector4{innerOutlineCornerRadius}} {}

    /**
     * @brief Construct with no outline
     *
     * The @ref outlineColor field is set to @cpp 0xffffff_srgbf @ce,
     * @ref outlineWidth to a zero vector and both @ref cornerRadius and
     * @ref innerOutlineCornerRadius get a value of @p cornerRadius.
     */
    constexpr /*implicit*/ BaseLayerStyleUniform(const Color4& topColor, const Color4& bottomColor, const Vector4& cornerRadius): BaseLayerStyleUniform{topColor, bottomColor, Color4{1.0f}, Vector4{0.0f}, cornerRadius, cornerRadius} {}

    /**
     * @brief Construct with no outline and all corners having the same radius
     *
     * Delegates to @ref BaseLayerStyleUniform(const Color4&, const Color4&, const Vector4&)
     * with @p cornerRadius having all components set to the same value.
     */
    constexpr /*implicit*/ BaseLayerStyleUniform(const Color4& topColor, const Color4& bottomColor, Float cornerRadius): BaseLayerStyleUniform{topColor, bottomColor, Vector4{cornerRadius}} {}

    /**
     * @brief Construct with no gradient
     *
     * The @ref topColor and @ref bottomColor fields are both set to the value
     * of @p color.
     */
    constexpr /*implicit*/ BaseLayerStyleUniform(const Color4& color, const Color4& outlineColor, const Vector4& outlineWidth, const Vector4& cornerRadius, const Vector4& innerOutlineCornerRadius): BaseLayerStyleUniform{color, color, outlineColor, outlineWidth, cornerRadius, innerOutlineCornerRadius} {}

    /**
     * @brief Construct with no gradient, all corners having the same radius and all edges the same outline width
     *
     * Delegates to @ref BaseLayerStyleUniform(const Color4&, const Color4&, const Color4&, Float, Float, Float)
     * with @p color used for both @p topColor and @p bottomColor.
     */
    constexpr /*implicit*/ BaseLayerStyleUniform(const Color4& color, const Color4& outlineColor, Float outlineWidth, Float cornerRadius, Float innerOutlineCornerRadius): BaseLayerStyleUniform{color, color, outlineColor, outlineWidth, cornerRadius, innerOutlineCornerRadius} {}

    /**
     * @brief Construct with no gradient and no outline
     *
     * Delegates to @ref BaseLayerStyleUniform(const Color4&, const Color4&, const Vector4&)
     * with @p color used for both @p topColor and @p bottomColor.
     */
    constexpr /*implicit*/ BaseLayerStyleUniform(const Color4& color, const Vector4& cornerRadius): BaseLayerStyleUniform{color, color, cornerRadius} {}

    /**
     * @brief Construct with no gradient, no outline and all corners having the same radius
     *
     * Delegates to @ref BaseLayerStyleUniform(const Color4&, const Color4&, Float)
     * with @p color used for both @p topColor and @p bottomColor.
     */
    constexpr /*implicit*/ BaseLayerStyleUniform(const Color4& color, Float cornerRadius): BaseLayerStyleUniform{color, color, cornerRadius} {}

    /** @brief Construct without initializing the contents */
    explicit BaseLayerStyleUniform(NoInitT) noexcept: topColor{NoInit}, bottomColor{NoInit}, outlineColor{NoInit}, outlineWidth{NoInit}, cornerRadius{NoInit}, innerOutlineCornerRadius{NoInit} {}

    /** @{
     * @name Convenience setters
     *
     * Provided to allow the use of method chaining for populating a structure
     * in a single expression, otherwise equivalent to accessing the fields
     * directly. Also guaranteed to provide backwards compatibility when
     * packing of the actual fields changes.
     */

    /**
     * @brief Set the @ref topColor and @ref bottomColor fields
     * @return Reference to self (for method chaining)
     */
    CORRADE_CONSTEXPR14 BaseLayerStyleUniform& setColor(const Color4& top, const Color4& bottom) {
        topColor = top;
        bottomColor = bottom;
        return *this;
    }

    /**
     * @brief Set the @ref topColor and @ref bottomColor fields to the same value
     * @return Reference to self (for method chaining)
     */
    CORRADE_CONSTEXPR14 BaseLayerStyleUniform& setColor(const Color4& color) {
        topColor = color;
        bottomColor = color;
        return *this;
    }

    /**
     * @brief Set the @ref outlineColor field
     * @return Reference to self (for method chaining)
     */
    CORRADE_CONSTEXPR14 BaseLayerStyleUniform& setOutlineColor(const Color4& color) {
        outlineColor = color;
        return *this;
    }

    /**
     * @brief Set the @ref outlineWidth field
     * @return Reference to self (for method chaining)
     */
    CORRADE_CONSTEXPR14 BaseLayerStyleUniform& setOutlineWidth(const Vector4& width) {
        outlineWidth = width;
        return *this;
    }

    /**
     * @brief Set the @ref outlineWidth field with all edges having the same value
     * @return Reference to self (for method chaining)
     */
    CORRADE_CONSTEXPR14 BaseLayerStyleUniform& setOutlineWidth(Float width) {
        outlineWidth = Vector4{width};
        return *this;
    }

    /**
     * @brief Set the @ref cornerRadius field
     * @return Reference to self (for method chaining)
     */
    CORRADE_CONSTEXPR14 BaseLayerStyleUniform& setCornerRadius(const Vector4& radius) {
        cornerRadius = radius;
        return *this;
    }

    /**
     * @brief Set the @ref cornerRadius field with all corners having the same value
     * @return Reference to self (for method chaining)
     */
    CORRADE_CONSTEXPR14 BaseLayerStyleUniform& setCornerRadius(Float radius) {
        cornerRadius = Vector4{radius};
        return *this;
    }

    /**
     * @brief Set the @ref innerOutlineCornerRadius field
     * @return Reference to self (for method chaining)
     */
    CORRADE_CONSTEXPR14 BaseLayerStyleUniform& setInnerOutlineCornerRadius(const Vector4& radius) {
        innerOutlineCornerRadius = radius;
        return *this;
    }

    /**
     * @brief Set the @ref innerOutlineCornerRadius field with all corners having the same value
     * @return Reference to self (for method chaining)
     */
    CORRADE_CONSTEXPR14 BaseLayerStyleUniform& setInnerOutlineCornerRadius(Float radius) {
        innerOutlineCornerRadius = Vector4{radius};
        return *this;
    }

    /**
     * @}
     */

    /**
     * @brief Top gradient color
     *
     * The color is expected to have premultiplied alpha. Default value is
     * @cpp 0xffffffff_srgbaf @ce. The color is further multiplied with
     * per-data value supplied with @ref BaseLayer::setColor(), potentially
     * with a texture if @ref BaseLayerSharedFlag::Textured is enabled, and
     * with node opacity coming from
     * @ref AbstractUserInterface::setNodeOpacity().
     * @see @ref Color4::premultiplied()
     */
    Color4 topColor;

    /**
     * @brief Bottom gradient color
     *
     * The color is expected to have premultiplied alpha. Default value is
     * @cpp 0xffffffff_srgbaf @ce. The color is further multiplied with
     * per-data value supplied with @ref BaseLayer::setColor(), potentially
     * with a texture if @ref BaseLayerSharedFlag::Textured is enabled, and
     * with node opacity coming from
     * @ref AbstractUserInterface::setNodeOpacity().
     * @see @ref Color4::premultiplied()
     */
    Color4 bottomColor;

    /**
     * @brief Outline color
     *
     * The color is expected to have premultiplied alpha. Default value is
     * @cpp 0xffffffff_srgbaf @ce. Visible only if @ref outlineWidth is
     * non-zero on at least one side or if the difference between
     * @ref cornerRadius and @ref innerOutlineCornerRadius makes it show. Not
     * used if @ref BaseLayerSharedFlag::NoOutline is enabled.
     *
     * Unlike @ref topColor and @ref bottomColor, the outline color isn't
     * multiplied with @ref BaseLayer::setColor(). Node opacity coming from
     * @ref AbstractUserInterface::setNodeOpacity() affects it however, and
     * texture alpha as well if @ref BaseLayerSharedFlag::TextureMask is
     * enabled.
     * @see @ref Color4::premultiplied()
     */
    Color4 outlineColor;

    /**
     * @brief Outline width
     *
     * In UI units, in order left, top, right, bottom. Default value is
     * @cpp 0.0f @ce for all sides. Not used if
     * @ref BaseLayerSharedFlag::NoOutline is enabled. The width is further
     * extended with the per-data value coming from
     * @ref BaseLayer::setOutlineWidth().
     */
    Vector4 outlineWidth;

    /**
     * @brief Corner radius
     *
     * In UI units, in order top left, bottom left, top right, bottom right.
     * Default value is @cpp 0.0f @ce for all sides. Not used if
     * @ref BaseLayerSharedFlag::NoRoundedCorners is enabled.
     */
    Vector4 cornerRadius;

    /**
     * @brief Inner outline corner radius
     *
     * In UI units, in order top left, bottom left, top right, bottom right.
     * Default value is @cpp 0.0f @ce for all sides. Not used if
     * @ref BaseLayerSharedFlag::NoOutline or
     * @relativeref{BaseLayerSharedFlag,NoRoundedCorners} is enabled.
     */
    Vector4 innerOutlineCornerRadius;
};

/**
@brief Base layer
@m_since_latest

Draws quads with a color gradient, variable rounded corners and outline,
optionally with texturing and background blur.

@section Ui-BaseLayer-setup Setting up a base layer instance

If you create a @ref UserInterfaceGL with a style and don't exclude
@ref StyleFeature::BaseLayer, an implicit instance of the @ref BaseLayerGL
subclass, configured for use with builtin widgets, is already provided and
available through @ref UserInterface::baseLayer().

For a custom layer, you first need to instantiate @ref BaseLayer::Shared, which
contains GPU shaders and style definitions. It takes a
@ref BaseLayer::Shared::Configuration, where at the very least you have to
specify how many distinct visual *styles* you intend to use --- which is for
example the number @cpp 3 @ce in the following snippet:

@snippet Ui-gl.cpp BaseLayer-setup-shared

The shared instance, in this case a concrete @ref BaseLayerGL::Shared subclass
for the OpenGL implementation of this layer, is then passed to the layer
constructor alongside a fresh @ref AbstractUserInterface::createLayer() handle,
and is expected to stay alive for the whole layer lifetime. The shared instance
can be used by multiple layers, for example if the application wants to have a
dedicated layer for very dynamic UI content, or if it combines visual options
that have to be hardcoded in particular @ref BaseLayer::Shared instances. To
make the layer available as the implicit @ref UserInterface::baseLayer(), pass
it to @ref UserInterfaceGL::setBaseLayerInstance():

@snippet Ui-gl.cpp BaseLayer-setup-implicit

Otherwise, if you want to set up a custom base layer that's independent of the
one exposed through @ref UserInterface::baseLayer(), possibly for completely
custom widgets, pass the newly created layer to
@ref AbstractUserInterface::setLayerInstance() instead:

@snippet Ui-gl.cpp BaseLayer-setup

Afterwards, in order to be able to draw the layer, a style has to be set with
@ref BaseLayer::Shared::setStyle(). At the very least you're expected to pass a
@ref BaseLayerCommonStyleUniform containing properties common to all styles,
and an array of @ref BaseLayerStyleUniform matching the style count set in the
@ref BaseLayer::Shared::Configuration. Default-constructed instances will
result in white quads with no outline or rounded corners, you can then use
method chaining to update only the properties you're interested in. So, for
example, with style @cpp 0 @ce being the default, style @cpp 1 @ce being a blue
quad and style @cpp 2 @ce transparent with a white outline:

@snippet Ui.cpp BaseLayer-setup-style

With this, assuming @ref AbstractUserInterface::draw() is called in an
appropriate place, the layer is ready to use. All style options are described
in detail further below.

@section Ui-BaseLayer-create Creating quads

A quad is created by calling @ref create() with desired style index and a
@ref NodeHandle the data should be attached to. In this case it picks the style
@cpp 1 @ce from above, which colors the quad blue:

@snippet Ui.cpp BaseLayer-create

As with all other data, they're implicitly tied to lifetime of the node they're
attached to. You can remember the @ref Ui::DataHandle returned by @ref create()
to modify the data later, @ref attach() to a different node or @ref remove()
it.

@section Ui-BaseLayer-style-enums Using an enum for style indexing

While the style indices are internally just a contiguous sequence of integers,
an @cpp enum @ce is a more convenient way to index them. The @ref create() as
well as @ref setStyle() can also accept an @cpp enum @ce for the style index,
and @ref style() has a templated overload that you can use to retrieve the
style index in the enum type again. The above style setup and use could then
look for example like this:

@snippet Ui.cpp BaseLayer-style-enums

@section Ui-BaseLayer-style Style options

@subsection Ui-BaseLayer-style-color Base color and gradient

@image html ui-baselayer-style-color.png width=256px

Apart from the single color shown above, there's a
@ref BaseLayerStyleUniform::setColor(const Color4&, const Color4&) overload
that allows you to supply two colors for a gradient. Right now, the gradient
can only be vertical.

@snippet Ui.cpp BaseLayer-style-color1

The style-supplied color is additionally multiplied by a data-specific color
set with @ref setColor(), which is shown in the third square above. Main use
case is various color swatches where it would be impractical to have to add
each custom color to the style data. Finally, the color is also multiplied by a
per-node opacity coming from @ref AbstractUserInterface::setNodeOpacity(), as
shown in the fourth square above, which can be used for various fade-in /
fade-out effects.

@snippet Ui.cpp BaseLayer-style-color2

@subsection Ui-BaseLayer-style-rounded-corners-smoothness Rounded corners and edge smoothness

@image html ui-baselayer-style-rounded-corners.png width=256px

With @ref BaseLayerStyleUniform::setCornerRadius() the quads can have rounded
corners, and the radius can be also different for each corner. The edges are
aliased by default, use @ref BaseLayerCommonStyleUniform::setSmoothness() to
smoothen them out. The smoothness radius is in actual framebuffer pixels and
not UI units in order to ensure crisp look everywhere without having to
specifically deal with HiDPI displays, and it's a common setting for all as
it's assumed that the styles will all want to use the same value.

@snippet Ui.cpp BaseLayer-style-rounded-corners

Finally, @ref BaseLayerCommonStyleUniform::setSmoothness() supports different
inner and outer smoothness to achieve certain kind of effects, but again it's a
common setting for all styles. Similar options for edge smoothness tuning are
in the @ref Ui-LineLayer-style-smoothness "LineLayer" and
@ref Ui-TextLayer-distancefield-smoothness "distance-field-enabled TextLayer".

@subsection Ui-BaseLayer-style-outline Outline width and color

@image html ui-baselayer-style-outline.png width=256px

@ref BaseLayerStyleUniform::setOutlineWidth() and
@relativeref{BaseLayerStyleUniform,setOutlineColor()} set properties of the
outline. Similarly to corner radius, the width can be different for each edge,
such as for the switch-like shape above. When outline is combined with rounded
corners, @ref BaseLayerStyleUniform::setInnerOutlineCornerRadius() specifies
the inner corner radius. In many cases it'll be set to outer radius with
outline width subtracted, but different values can be used to achieve various
effects.

@snippet Ui.cpp BaseLayer-style-outline

@image html ui-baselayer-style-outline-data-width.png width=256px

The style-supplied outline width is added together with per-data outline width
coming from @ref setOutlineWidth(). The intended usage scenario is implementing
simple progress bars and scrollbars. For the image above, the style has the
outline fully defined except for its width, which is then supplied dynamically
based on the actual percentage it should visualize.

@snippet Ui.cpp BaseLayer-style-outline-data-width

Unlike base color, the outline color isn't affected by @ref setColor().

@subsection Ui-BaseLayer-style-padding Padding inside the node

The last argument to @ref BaseLayer::Shared::setStyle() is an optional list of
per-style padding values, which specify a padding between the rendered quad and
edges of the node containing it. Visually, the same can be achieved by
attaching the data to an appropriately shrunk child node instead, but doing so
may interfere with event handling behavior if both nodes are meant to behave as
a single active element.

@image html ui-baselayer-style-padding.png width=256px

On the left above is a button-like shape with a detached outline, achieved by
placing two quads inside the same node, with one being just an outline and the
other having a padding. The draw order isn't guaranteed in case of multiple
data attached to the same node, but as the shapes don't overlap, it doesn't
matter.

@snippet Ui.cpp BaseLayer-style-padding

There's also @ref setPadding() for additional per-data padding, which is useful
for example when aligning icons next to variable-width text. Or, as shown with
the slider above on the right, for additional styling flexibility. In order to
ensure correct draw order, the green bar is put into a child node, but both
have the same size to have the whole area react the same way to taps or clicks.

@snippet Ui.cpp BaseLayer-style-padding-data

@subsection Ui-BaseLayer-style-textured Textured drawing

@image html ui-baselayer-style-textured.png width=256px

If @ref BaseLayerSharedFlag::Textured is enabled in
@ref BaseLayer::Shared::Configuration, each quad additionally multiplies its
color with a texture. Unlike most other interfaces which are accessible
directly through the generic @ref BaseLayer, textures are tied to GPU-specific
APIs, in this case a @ref GL::Texture2DArray, and you're meant to supply it via
@ref BaseLayerGL::setTexture() on the concrete @ref BaseLayerGL subclass. The
supplied texture is expected to be alive for the whole layer lifetime,
alternatively you can use @ref BaseLayerGL::setTexture(GL::Texture2DArray&&) to
move its ownership to the layer instance.

@snippet Ui-gl.cpp BaseLayer-style-textured1

By default the whole first slice of the texture array is used, however it's
assumed that a texture atlas is used from which particular data use
sub-rectangles defined with @ref setTextureCoordinates(). Texturing can be
combined with rounded corners and all other features. The texture coordinates
include the outline, if it's present, but the outline itself isn't textured.

@snippet Ui-gl.cpp BaseLayer-style-textured2

You can use @ref TextureTools::AtlasLandfill or
@ref TextureTools::atlasArrayPowerOfTwo() to pack multiple images into a single
atlas, the former is usable for incremental packing as well. Currently only a
single texture can be used with the layer, if you need to draw from multiple
textures, create additional layer instances.

@subsection Ui-BaseLayer-style-background-blur Background blur

@m_class{m-row m-container-inflate}

@parblock

@m_div{m-col-l-4 m-col-m-6 m-text-center m-nopadx}
@image html ui-baselayer-flag-default.png width=256px
Default look of semi-transparent quads @m_span{null} @m_endspan
@m_enddiv

@m_div{m-col-l-4 m-col-m-6 m-text-center m-nopadx}
@image html ui-baselayer-flag-blur.png width=256px
With background blur enabled @m_span{null} @m_endspan
@m_enddiv

@m_div{m-col-l-4 m-col-m-6 m-push-m-3 m-push-l-0 m-text-center m-nopadx}
@image html ui-baselayer-flag-blur-alpha.png width=256px
Background blur with alpha of @cpp 0.75f @ce
@m_enddiv

@endparblock

With @ref BaseLayerSharedFlag::BackgroundBlur, and
@ref RendererGL::Flag::CompositingFramebuffer enabled for the renderer,
semi-transparent quads will be drawn with their background blurred. Blur
strength can be controlled with @ref BaseLayer::Shared::Configuration::setBackgroundBlurRadius()
and @ref setBackgroundBlurPassCount(),
@ref BaseLayerCommonStyleUniform::setBackgroundBlurAlpha() can be used to
achieve a frosted-glass-like effect. This effect relies on the UI being able to
read back the framebuffer it draws to, see the @ref RendererGL documentation
for a detailed example of how to set up the compositing framebuffer in the
application.

@snippet Ui-gl.cpp BaseLayer-style-background-blur

As the effect is potentially expensive, only the framebuffer areas that
actually are covered by quads get blurred. The effect is stackable, meaning
that blurred quads in each top-level node hierarchy will blur contents of all
top-level hierarchies underneath. To avoid performance issues, it's thus
recommended to switch to non-blurred layers when stacking reaches a certain
level.

@m_class{m-row}

@parblock

@m_div{m-col-m-6 m-text-center m-nopadx}
@image html ui-baselayer-flag-blur-textured.png width=256px
Default blurred texturing behavior @m_span{null} @m_endspan
@m_enddiv

@m_div{m-col-m-6 m-text-center m-nopadx}
@image html ui-baselayer-flag-blur-textured-mask.png width=256px
With @ref BaseLayerSharedFlag::TextureMask
@m_enddiv

@endparblock

Finally, when texturing is enabled together with background blur, the
@ref BaseLayerSharedFlag::TextureMask flag may come handy. By default, without
blur, transparent texture areas are see-through so this flag doesn't make any
visual difference, but with blur enabled, the flag excludes the transparent
areas from being blurred, allowing you to apply any custom shape to the blurred
area. In this case, the masking --- but not the texture color --- is applied to
the outline as well.

@section Ui-BaseLayer-dynamic-styles Dynamic styles

The @ref BaseLayer::Shared::setStyle() API is meant to be used to supply style
data for all known styles at once, with the assumption that the styles are
likely supplied from some constant memory and will be updated only in rare
scenarios afterwards, such as when switching application themes.

If a particular style needs to be modified often and it's not achievable with
@ref setColor(), @ref setOutlineWidth() or @ref setPadding() on the data
itself or it'd have to be updated on many data at once, a dynamic style can be
used instead. Dynamic styles have to be explicitly requested with
@ref BaseLayer::Shared::Configuration::setDynamicStyleCount(), after which the
style uniform buffer isn't shared among all layers using given
@ref BaseLayer::Shared instance, but is local to each layer and each layer has
its own set of dynamic styles. Dynamic styles occupy indices from
@ref BaseLayer::Shared::styleCount() upwards and their properties are specified
via @ref setDynamicStyle().

@snippet Ui-gl.cpp BaseLayer-dynamic-styles

The main use case for dynamic styles is animations, for example various fade-in
and fade-out transitions based on input events and application state changes.
See the @ref BaseLayerStyleAnimator class for a high-level style animator
working with the base layer. Note that if you intend to directly use dynamic
styles along with the animator, you should use @ref allocateDynamicStyle() and
@ref recycleDynamicStyle() to prevent the animator from stealing dynamic styles
you use elsewhere:

@snippet Ui.cpp BaseLayer-dynamic-styles-allocate

@section Ui-BaseLayer-style-transitions Style transition based on input events

With interactive UI elements such as buttons or inputs you'll likely have
different styles for an inactive and active state, possibly handling hover and
focus as well. While the style can be switched using @ref setStyle() for
example in an @ref EventLayer::onPress() handler, this quickly becomes very
tedious and repetitive. Instead, style transition can be implemented using
@ref BaseLayer::Shared::setStyleTransition().

It accepts a set of functions that get called with a style index when a node is
hovered, pressed, released etc., and should return a style index that matches
the new state. Assuming there's a button with various states, a label that
doesn't visually react to events, and the style doesn't deal with focused or
disabled state, implementing automatic transitions could look like this:

@snippet Ui.cpp BaseLayer-style-transitions

As in other APIs that deal with styles, the functions can operate either on an
@cpp enum @ce, or on a plain @relativeref{Magnum,UnsignedInt}. If @cpp nullptr @ce
is passed for any function, given transition is assumed to an identity, i.e. as
if the input was returned unchanged. The focused transition can happen only on
nodes that are @ref NodeFlag::Focusable, disabled transition then on node
hierarchies that have @ref NodeFlag::Disabled set.

Looking at the above snippet, you'll likely notice that there's a lot of
repetition. The @ref BaseLayer::Shared::setStyleTransition() API is
deliberately minimalistic and you're encouraged to implement the transitions in
any way that makes sense for given use case. Instead of a @cpp switch @ce they
can be for example baked into a compile-time lookup table. Or there can be just
a single transition function for which only a part of the result gets used each
time:

@snippet Ui.cpp BaseLayer-style-transitions-deduplicated

<b></b>

@m_class{m-note m-info}

@par
    Note that, at the moment, the whole node area reacts to the event, thus
    even the padding and rounded corner area.

@section Ui-BaseLayer-performance Options affecting performance

@subsection Ui-BaseLayer-performance-shaders Configuring shader complexity

The @ref BaseLayerSharedFlag::NoOutline and
@relativeref{BaseLayerSharedFlag,NoRoundedCorners} options disable rendering of
outline and rounded corners, respectively, making the layer behave as if those
features were not specified in any style nor supplied via
@ref setOutlineWidth(). The likely use case for these flags is for example a
texture / icon layer which don't make use of any outlines or rounded corners,
and depending on the platform this can significantly reduce shader complexity.

@htmlinclude ui-baselayer-subdivided-quads.svg

By default, each quad is literally two triangles, and positioning of the
outline and rounded corners is done purely in shader code. While that's fine on
common hardware, certain low-power GPUs may struggle with fragment shader
complexity. By enabling @ref BaseLayerSharedFlag::SubdividedQuads the drawing
is split up into 9 quads as shown on the right, offloading a large part of the
work to the vertex shader instead. Apart from that, the visual output is
exactly the same with and without this flag enabled. A downside is however that
a lot more data needs to be uploaded to the GPU, so this flag is only useful
when the gains in fragment processing time outweigh the additional vertex data
overhead.

In case of background blur, smaller blur radii need less texture samples and
thus are faster. Besides that, the second argument passed to
@ref BaseLayer::Shared::Configuration::setBackgroundBlurRadius() is a cutoff
threshold, which excludes samples that would contribute to the final pixel
value less than given threshold. By default it's @cpp 0.5f/255.0f @ce, i.e.
what would be at best a rounding error when operating on a 8-bit-per-channel
framebuffer. With a higher threshold the processing will get faster in exchange
for decreased blur quality. Finally, @ref setBackgroundBlurPassCount() can be
used to perform a blur of smaller radius in multiple passes, in case a bigger
radius is hitting hardware or implementation limits.
*/
class MAGNUM_UI_EXPORT BaseLayer: public AbstractVisualLayer {
    public:
        class Shared;

        /**
         * @brief Shared state used by this layer
         *
         * Reference to the instance passed to @ref BaseLayerGL::BaseLayerGL(LayerHandle, Shared&).
         */
        inline Shared& shared();
        inline const Shared& shared() const; /**< @overload */

        /**
         * @brief Background blur pass count
         *
         * Expects that @ref BaseLayerSharedFlag::BackgroundBlur was enabled
         * for the shared state the layer was created with.
         */
        UnsignedInt backgroundBlurPassCount() const;

        /**
         * @brief Set background blur pass count
         * @return Reference to self (for method chaining)
         *
         * Expects that @ref BaseLayerSharedFlag::BackgroundBlur was enabled
         * for the shared state the layer was created with and that @p count is
         * at least @cpp 1 @ce. Higher values will perform the blurring process
         * several times, which has the same effect as applying a single,
         * larger, Gaussian blur. With @f$ r @f$ being the radius configured by
         * @ref Shared::Configuration::setBackgroundBlurRadius() and @f$ n @f$
         * being the @p count, the relation to the larger radius @f$ l @f$ is
         * as follows: @f[
         *      l = \sqrt{nr^2}
         * @f]
         *
         * Thus by combining the radius and pass count it's possible to achieve
         * blurring in radii larger than the limit of @cpp 31 @ce in
         * @ref Shared::Configuration::setBackgroundBlurRadius(), or
         * alternatively tune the operation based on whether the GPU is faster
         * with few passes and many texture samples each or many passes with
         * few (localized) texture samples each. For example, a radius of 36
         * can be achieved with 16 passes of radius 9, or with 4 passes of
         * radius 18, or other combinations.
         *
         * Default pass count is @cpp 1 @ce.
         *
         * Calling this function causes
         * @ref LayerState::NeedsCompositeOffsetSizeUpdate to be set.
         */
        BaseLayer& setBackgroundBlurPassCount(UnsignedInt count);

        /**
         * @brief Assign a style animator to this layer
         * @return Reference to self (for method chaining)
         *
         * Expects that @ref Shared::dynamicStyleCount() is non-zero and that
         * given @p animator wasn't passed to @ref assignAnimator() on any
         * layer yet. On the other hand, it's possible to associate multiple
         * different animators with the same layer.
         * @see @ref setDefaultStyleAnimator()
         */
        BaseLayer& assignAnimator(BaseLayerStyleAnimator& animator);

        /**
         * @brief Default style animator for this layer
         *
         * If a style animator hasn't been set, returns @cpp nullptr @ce. If
         * not @cpp nullptr @ce, the returned animator is guaranteed to be
         * assigned to this layer, i.e. that
         * @ref BaseLayerStyleAnimator::layer() is equal to @ref handle().
         */
        BaseLayerStyleAnimator* defaultStyleAnimator() const;

        /**
         * @brief Set a default style animator for this layer
         * @return Reference to self (for method chaining)
         *
         * Makes @p animator used in style transitions in response to events.
         * Expects that @p animator is either @cpp nullptr @ce or is already
         * assigned to this layer, i.e. that @ref assignAnimator() was called
         * on this layer with @p animator before. Calling this function again
         * with a different animator or with @cpp nullptr @ce replaces the
         * previous one.
         * @see @ref allocateDynamicStyle()
         */
        BaseLayer& setDefaultStyleAnimator(BaseLayerStyleAnimator* animator);

        /**
         * @brief Dynamic style uniforms
         *
         * Size of the returned view is @ref Shared::dynamicStyleCount(). These
         * uniforms are used by style indices greater than or equal to
         * @ref Shared::styleCount().
         * @see @ref dynamicStylePaddings(), @ref setDynamicStyle()
         */
        Containers::ArrayView<const BaseLayerStyleUniform> dynamicStyleUniforms() const;

        /**
         * @brief Dynamic style paddings
         *
         * Size of the returned view is @ref Shared::dynamicStyleCount(). These
         * paddings are used by style indices greater than or equal to
         * @ref Shared::styleCount().
         * @see @ref dynamicStyleUniforms(), @ref setDynamicStyle()
         */
        Containers::StridedArrayView1D<const Vector4> dynamicStylePaddings() const;

        /**
         * @brief Set a dynamic style
         * @param id                Dynamic style ID
         * @param uniform           Style uniform
         * @param padding           Padding inside the node in order left, top,
         *      right, bottom
         *
         * Expects that the @p id is less than @ref Shared::dynamicStyleCount().
         * @ref Shared::styleCount() plus @p id is then a style index that can
         * be passed to @ref create() or @ref setStyle() in order to use this
         * style. Compared to @ref Shared::setStyle() the mapping between
         * dynamic styles and uniforms is implicit. All dynamic styles are
         * initially default-constructed @ref BaseLayerStyleUniform instances
         * and zero padding vectors.
         *
         * Calling this function causes @ref LayerState::NeedsCommonDataUpdate
         * to be set to trigger an upload of changed dynamic style uniform
         * data. If @p padding changed, @ref LayerState::NeedsDataUpdate gets
         * set as well.
         * @see @ref dynamicStyleUniforms(), @ref dynamicStylePaddings(),
         *      @ref allocateDynamicStyle(), @ref recycleDynamicStyle()
         */
        void setDynamicStyle(UnsignedInt id, const BaseLayerStyleUniform& uniform, const Vector4& padding);

        /**
         * @brief Create a quad
         * @param style         Style index
         * @param node          Node to attach to
         * @return New data handle
         *
         * Expects that @p style is less than @ref Shared::totalStyleCount().
         * All styling is driven from the @ref BaseLayerStyleUniform at index
         * @p style.
         * @see @ref setColor(), @ref setOutlineWidth(), @ref setPadding(),
         *      @ref setTextureCoordinates()
         */
        DataHandle create(UnsignedInt style, NodeHandle node =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            NodeHandle::Null
            #else
            NodeHandle{} /* To not have to include Handle.h */
            #endif
        );

        /**
         * @brief Create a quad with a style index in a concrete enum type
         *
         * Casts @p style to @relativeref{Magnum,UnsignedInt} and delegates to
         * @ref create(UnsignedInt, NodeHandle).
         */
        template<class StyleIndex
            #ifndef DOXYGEN_GENERATING_OUTPUT
            /* Accept any enum except NodeHandle to prevent create(node) from
               being called by mistake */
            , typename std::enable_if<std::is_enum<StyleIndex>::value && !std::is_same<StyleIndex, NodeHandle>::value, int>::type = 0
            #endif
        > DataHandle create(StyleIndex style, NodeHandle node =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            NodeHandle::Null
            #else
            NodeHandle{} /* To not have to include Handle.h */
            #endif
        ) {
            return create(UnsignedInt(style), node);
        }

        /**
         * @brief Remove a quad
         *
         * Delegates to @ref AbstractLayer::remove(DataHandle).
         */
        void remove(DataHandle handle) {
            AbstractVisualLayer::remove(handle);
        }

        /**
         * @brief Remove a quad assuming it belongs to this layer
         *
         * Delegates to @ref AbstractLayer::remove(LayerDataHandle).
         */
        void remove(LayerDataHandle handle) {
            AbstractVisualLayer::remove(handle);
        }

        /**
         * @brief Custom quad base color
         *
         * Expects that @p handle is valid.
         * @see @ref isHandleValid(DataHandle) const
         */
        Color4 color(DataHandle handle) const;

        /**
         * @brief Custom quad base color assuming it belongs to this layer
         *
         * Expects that @p handle is valid.
         * @see @ref isHandleValid(LayerDataHandle) const
         */
        Color4 color(LayerDataHandle handle) const;

        /**
         * @brief Set custom quad base color
         *
         * Expects that @p handle is valid. The @p color is expected to have
         * premultiplied alpha. It is multiplied with
         * @ref BaseLayerStyleUniform::topColor and
         * @relativeref{BaseLayerStyleUniform,bottomColor}, with a texture if
         * @ref BaseLayerSharedFlag::Textured is enabled, and with node opacity
         * coming from @ref AbstractUserInterface::setNodeOpacity(). By
         * default, the custom color is @cpp 0xffffffff_srgbaf @ce, i.e. not
         * affecting the color coming from the style or texture in any way.
         *
         * Calling this function causes @ref LayerState::NeedsDataUpdate to be
         * set.
         * @see @ref isHandleValid(DataHandle) const,
         *      @ref Color4::premultiplied()
         */
        void setColor(DataHandle handle, const Color4& color);

        /**
         * @brief Set custom quad base color assuming it belongs to this layer
         *
         * Like @ref setColor(DataHandle, const Color4&) but without checking
         * that @p handle indeed belongs to this layer. See its documentation
         * for more information.
         */
        void setColor(LayerDataHandle handle, const Color4& color);

        /**
         * @brief Custom quad outline width
         *
         * In UI units, in order left, top, right, bottom. Expects that
         * @p handle is valid.
         * @see @ref isHandleValid(DataHandle) const
         */
        Vector4 outlineWidth(DataHandle handle) const;

        /**
         * @brief Custom quad outline width assuming it belongs to this layer
         *
         * Like @ref outlineWidth(DataHandle) const but without checking that
         * @p handle indeed belongs to this layer. See its documentation for
         * more information.
         */
        Vector4 outlineWidth(LayerDataHandle handle) const;

        /**
         * @brief Set custom quad outline width
         *
         * Expects that @p handle is valid. The @p width is in UI units, in
         * order left, top, right, bottom and is added to
         * @ref BaseLayerStyleUniform::outlineWidth. By default the custom
         * outline width is a zero vector, i.e. not affecting the style in any
         * way. Has no visual effect if @ref BaseLayerSharedFlag::NoOutline is
         * enabled.
         *
         * Calling this function causes @ref LayerState::NeedsDataUpdate to be
         * set.
         * @see @ref isHandleValid(DataHandle) const
         */
        void setOutlineWidth(DataHandle handle, const Vector4& width);

        /**
         * @brief Set custom quad outline width with all edges having the same value
         *
         * Equivalent to calling @ref setOutlineWidth(DataHandle, const Vector4&)
         * with all four components set to @p width. See its documentation for
         * more information.
         */
        void setOutlineWidth(DataHandle handle, Float width) {
            setOutlineWidth(handle, Vector4{width});
        }

        /**
         * @brief Set custom quad outline width assuming it belongs to this layer
         *
         * Like @ref setOutlineWidth(DataHandle, const Vector4&) but without
         * checking that @p handle indeed belongs to this layer. See its
         * documentation for more information.
         */
        void setOutlineWidth(LayerDataHandle handle, const Vector4& width);

        /**
         * @brief Set custom quad outline width with all edges having the same value assuming it belongs to this layer
         *
         * Like @ref setOutlineWidth(DataHandle, Float) but without checking
         * that @p handle indeed belongs to this layer. See its documentation
         * for more information.
         */
        void setOutlineWidth(LayerDataHandle handle, Float width) {
            setOutlineWidth(handle, Vector4{width});
        }

        /**
         * @brief Custom quad padding
         *
         * In UI units, in order left, top, right, bottom. Expects that
         * @p handle is valid.
         * @see @ref isHandleValid(DataHandle) const
         */
        Vector4 padding(DataHandle handle) const;

        /**
         * @brief Custom quad padding assuming it belongs to this layer
         *
         * Like @ref padding(DataHandle) const but without checking that
         * @p handle indeed belongs to this layer. See its documentation for
         * more information.
         */
        Vector4 padding(LayerDataHandle handle) const;

        /**
         * @brief Set custom quad padding
         *
         * Expects that @p handle is valid. The @p padding is in UI units, in
         * order left, top, right, bottom and is added to the per-style padding
         * values specified in @ref Shared::setStyle(). By default, the custom
         * padding is a zero vector, i.e. not affecting the padding coming from
         * the style in any way.
         *
         * Calling this function causes @ref LayerState::NeedsDataUpdate to be
         * set.
         * @see @ref isHandleValid(DataHandle) const
         */
        void setPadding(DataHandle handle, const Vector4& padding);

        /**
         * @brief Set custom quad padding assuming it belongs to this layer
         *
         * Like @ref setPadding(DataHandle, const Vector4&) but without
         * checking that @p handle indeed belongs to this layer. See its
         * documentation for more information.
         */
        void setPadding(LayerDataHandle handle, const Vector4& padding);

        /**
         * @brief Set custom quad padding with all edges having the same value
         *
         * Equivalent to calling @ref setPadding(DataHandle, const Vector4&)
         * with all four components set to @p padding. See its documentation
         * for more information.
         */
        void setPadding(DataHandle handle, Float padding) {
            setPadding(handle, Vector4{padding});
        }

        /**
         * @brief Set custom quad padding with all edges having the same value assuming it belongs to this layer
         *
         * Like @ref setPadding(DataHandle, Float) but without checking that
         * @p handle indeed belongs to this layer. See its documentation for
         * more information.
         */
        void setPadding(LayerDataHandle handle, Float padding) {
            setPadding(handle, Vector4{padding});
        }

        /**
         * @brief Quad texture coordinate offset and size
         *
         * The third coordinate is array layer. Expects that @p handle is
         * valid and that @ref BaseLayerSharedFlag::Textured was enabled for
         * the shared state the layer was created with.
         * @see @ref isHandleValid(DataHandle) const
         */
        Containers::Pair<Vector3, Vector2> textureCoordinates(DataHandle handle) const;

        /**
         * @brief Quad texture coordinate offset and size assuming it belongs to this layer
         *
         * The third coordinate is array layer. Expects that @p handle is
         * valid and that @ref BaseLayerSharedFlag::Textured was enabled for
         * the shared state the layer was created with.
         * @see @ref isHandleValid(LayerDataHandle) const
         */
        Containers::Pair<Vector3, Vector2> textureCoordinates(LayerDataHandle handle) const;

        /**
         * @brief Set quad texture coordinates
         *
         * The third coordinate of @p offset is array layer. Expects that
         * @p handle is valid and that @ref BaseLayerSharedFlag::Textured was
         * enabled for the shared state the layer was created with. By default
         * the offset is @cpp {0.0f, 0.0f, 0.0f} @ce and size is
         * @cpp {1.0f, 1.0f} @ce, i.e. covering the whole first slice of the
         * texture.
         *
         * Calling this function causes @ref LayerState::NeedsDataUpdate to be
         * set.
         * @see @ref BaseLayerGL::setTexture()
         */
        void setTextureCoordinates(DataHandle handle, const Vector3& offset, const Vector2& size);

        /**
         * @brief Set quad texture coordinates assuming it belongs to this layer
         *
         * Like @ref setTextureCoordinates(DataHandle, const Vector3&, const Vector2&)
         * but without checking that @p handle indeed belongs to this layer.
         * See its documentation for more information.
         */
        void setTextureCoordinates(LayerDataHandle handle, const Vector3& offset, const Vector2& size);

    #ifdef DOXYGEN_GENERATING_OUTPUT
    private:
    #else
    protected:
    #endif
        struct State;

        MAGNUM_UI_LOCAL explicit BaseLayer(LayerHandle handle, Containers::Pointer<State>&& state);
        /* Used by tests to avoid having to include / allocate the state */
        explicit BaseLayer(LayerHandle handle, Shared& shared);

        /* These can't be MAGNUM_UI_LOCAL otherwise deriving from this class
           in tests causes linker errors */

        /* Advertises LayerFeature::Draw (and Composite if BackgroundBlur is
           enabled) but *does not* implement doDraw() or doComposite(), that's
           on the subclass */
        LayerFeatures doFeatures() const override;

        void doSetSize(const Vector2& size, const Vector2i& framebufferSize) override;
        void doUpdate(LayerStates states, const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, const Containers::StridedArrayView1D<const Float>& nodeOpacities, Containers::BitArrayView nodesEnabled, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes, const Containers::StridedArrayView1D<const Vector2>& compositeRectOffsets, const Containers::StridedArrayView1D<const Vector2>& compositeRectSizes) override;

    private:
        MAGNUM_UI_LOCAL void setColorInternal(UnsignedInt id, const Color4& color);
        MAGNUM_UI_LOCAL void setOutlineWidthInternal(UnsignedInt id, const Vector4& width);
        MAGNUM_UI_LOCAL void setPaddingInternal(UnsignedInt id, const Vector4& padding);
        MAGNUM_UI_LOCAL Containers::Pair<Vector3, Vector2> textureCoordinatesInternal(UnsignedInt id) const;
        MAGNUM_UI_LOCAL void setTextureCoordinatesInternal(UnsignedInt id, const Vector3& offset, const Vector2& size);

        /* These can't be MAGNUM_UI_LOCAL otherwise deriving from this class
           in tests causes linker errors */
        LayerStates doState() const override;
        void doAdvanceAnimations(Nanoseconds time, Containers::MutableBitArrayView activeStorage, const Containers::StridedArrayView1D<Float>& factorStorage, Containers::MutableBitArrayView removeStorage, const Containers::Iterable<AbstractStyleAnimator>& animators) override;
};

/**
@brief Base layer shared state flag
@m_since_latest

@see @ref BaseLayerSharedFlags,
    @ref BaseLayer::Shared::Configuration::setFlags(),
    @ref BaseLayer::Shared::flags()
*/
enum class BaseLayerSharedFlag: UnsignedByte {
    /**
     * Textured drawing. If enabled, the @ref BaseLayerStyleUniform::topColor
     * and @relativeref{BaseLayerStyleUniform,bottomColor} is multiplied with a
     * color coming from a texture set in @ref BaseLayerGL::setTexture() and
     * texture coordinates specified with @ref BaseLayer::setTextureCoordinates().
     * @see @ref BaseLayerSharedFlag::TextureMask
     */
    Textured = 1 << 0,

    /**
     * Blur the background of semi-transparent quads. If enabled, the
     * alpha value of @ref BaseLayerStyleUniform::topColor,
     * @relativeref{BaseLayerStyleUniform,bottomColor} and
     * @relativeref{BaseLayerStyleUniform,outlineColor} is used to interpolate
     * between the color and the blurred background, instead of performing a
     * classical blending of the color and the framebuffer contents underneath.
     *
     * @m_class{m-row}
     *
     * @parblock
     *
     * @m_div{m-col-m-6 m-text-center m-nopadt m-nopadx}
     * @image html ui-baselayer-flag-default.png width=256px
     * Default look of semi-transparent quads @m_span{null} @m_endspan
     * @m_enddiv
     *
     * @m_div{m-col-m-6 m-text-center m-nopadt m-nopadx}
     * @image html ui-baselayer-flag-blur.png width=256px
     * With @ref BaseLayerSharedFlag::BackgroundBlur
     * @m_enddiv
     *
     * @endparblock
     *
     * Use @ref BaseLayer::Shared::Configuration::setBackgroundBlurRadius() and
     * @ref BaseLayer::setBackgroundBlurPassCount() to control the blur radius
     * and @ref BaseLayerCommonStyleUniform::backgroundBlurAlpha to achieve
     * additional effects.
     *
     * @see @ref BaseLayerSharedFlag::TextureMask
     */
    BackgroundBlur = 1 << 1,

    /**
     * Disable support for rounded corners. If set, the
     * @ref BaseLayerStyleUniform::cornerRadius and
     * @relativeref{BaseLayerStyleUniform,innerOutlineCornerRadius} fields are
     * not used and the behavior is the same as if they were both set to
     * @cpp 0.0f @ce. Can result in rendering performance improvement, useful
     * for example when @ref BaseLayerSharedFlag::Textured is enabled and the
     * layer is used just to draw (alpha blended) images, or if a particular
     * widget style doesn't use rounded corners at all.
     *
     * Mutually exclusive with the @ref BaseLayerSharedFlag::SubdividedQuads
     * optimization.
     * @see @ref BaseLayerSharedFlag::NoOutline
     */
    NoRoundedCorners = 1 << 2,

    /**
     * Disable support for rounded corners. If set, the
     * @ref BaseLayerCommonStyleUniform::innerOutlineSmoothness,
     * @ref BaseLayerStyleUniform::outlineColor,
     * @relativeref{BaseLayerStyleUniform,outlineWidth} and
     * @relativeref{BaseLayerStyleUniform,innerOutlineCornerRadius} fields are
     * not used and @ref BaseLayer::setOutlineWidth() has no effect. The
     * behavior is then the same as if the outline width was set to
     * @cpp 0.0f @ce on all sides both in the style and for all data. Can
     * result in rendering performance improvement, useful for example when
     * @ref BaseLayerSharedFlag::Textured is enabled and the layer is used just
     * to draw (alpha blended) images, or if a particular widget style doesn't
     * use outlines at all.
     *
     * Mutually exclusive with the @ref BaseLayerSharedFlag::SubdividedQuads
     * optimization.
     * @see @ref BaseLayerSharedFlag::NoRoundedCorners
     */
    NoOutline = 1 << 3,

    /**
     * Use alpha channel of the texture to mask out the outline and background
     * blur. By default the outline is drawn over the texture without taking
     * the texture color or alpha into account; and the background is blurred
     * for the whole area of the quad, with transparent areas of the texture
     * causing just the blurred background to be shown. Enabling this flag
     * causes the transparent areas to make holes in both the outline and the
     * blurred background. Implies @ref BaseLayerSharedFlag::Textured.
     *
     * @m_class{m-row}
     *
     * @parblock
     *
     * @m_div{m-col-m-6 m-text-center m-nopadt m-nopadx}
     * @image html ui-baselayer-flag-blur-textured.png width=256px
     * Default blurred texturing behavior @m_span{null} @m_endspan
     * @m_enddiv
     *
     * @m_div{m-col-m-6 m-text-center m-nopadt m-nopadx}
     * @image html ui-baselayer-flag-blur-textured-mask.png width=256px
     * With @ref BaseLayerSharedFlag::TextureMask
     * @m_enddiv
     *
     * @endparblock
     *
     * @see @ref BaseLayerSharedFlag::BackgroundBlur,
     *      @ref BaseLayerCommonStyleUniform::backgroundBlurAlpha
     */
    TextureMask = Textured|(1 << 4),

    /**
     * Render the quads subdivided into 9 quads, where each contains either a
     * corner, an edge with an outline or the inside. This significantly
     * simplifies the fragment shader at the cost of uploading much more data
     * to the GPU. May improve rendering speed on low performance GPUs but the
     * extra bandwidth may have a negative effect on GPUs where the rendering
     * wasn't bottlenecked by fragment shading.
     *
     * Mutually exclusive with the @ref BaseLayerSharedFlag::NoRoundedCorners
     * and @relativeref{BaseLayerSharedFlag,NoOutline} optimizations.
     */
    SubdividedQuads = 1 << 5,
};

/**
@brief Base layer shared state flag
@m_since_latest

@see @ref BaseLayer::Shared::Configuration::setFlags(),
    @ref BaseLayer::Shared::flags()
*/
typedef Containers::EnumSet<BaseLayerSharedFlag> BaseLayerSharedFlags;

CORRADE_ENUMSET_OPERATORS(BaseLayerSharedFlags)

/**
@debugoperatorenum{BaseLayerSharedFlag}
@m_since_latest
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, BaseLayerSharedFlag value);

/**
@debugoperatorenum{BaseLayerSharedFlags}
@m_since_latest
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, BaseLayerSharedFlags value);

/**
@brief Shared state for the base layer

Contains style definitions. See the @ref BaseLayer class documentation for
information about setting up an instance of this layer and using it.

You'll most likely instantiate the class through @ref BaseLayerGL::Shared. In
order to update or draw the layer it's expected that @ref setStyle() was
called.
*/
class MAGNUM_UI_EXPORT BaseLayer::Shared: public AbstractVisualLayer::Shared {
    public:
        class Configuration;

        /**
         * @brief Style uniform count
         *
         * Size of the style uniform buffer excluding dynamic styles. May or
         * may not be the same as @ref styleCount(). For dynamic styles, the
         * style uniform count is the same as @ref dynamicStyleCount().
         * @see @ref Configuration::Configuration(UnsignedInt, UnsignedInt),
         *      @ref setStyle()
         */
        UnsignedInt styleUniformCount() const;

        /** @brief Flags */
        BaseLayerSharedFlags flags() const;

        /**
         * @brief Set style data with implicit mapping between styles and uniforms
         * @param commonUniform Common style uniform data
         * @param uniforms      Style uniforms
         * @param paddings      Padding inside the node in UI units, in order
         *      left, top, right, bottom, corresponding to style uniforms
         * @return Reference to self (for method chaining)
         *
         * The @p uniforms view is expected to have the same size as
         * @ref styleUniformCount(). The @p paddings view is expected to either
         * have the same size as @ref styleCount() or be empty, in which case
         * all paddings are implicitly zero.
         *
         * Can only be called if @ref styleUniformCount() and @ref styleCount()
         * were set to the same value in @ref Configuration passed to the
         * constructor, otherwise you have to additionally provide a mapping
         * from styles to uniforms using
         * @ref setStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const Vector4>&)
         * instead.
         *
         * Calling this function causes @ref LayerState::NeedsDataUpdate to be
         * set on all layers that are constructed using this shared instance.
         * If @ref dynamicStyleCount() is non-zero,
         * @ref LayerState::NeedsCommonDataUpdate is set as well to trigger an
         * upload of changed dynamic style uniform data.
         */
        Shared& setStyle(const BaseLayerCommonStyleUniform& commonUniform, Containers::ArrayView<const BaseLayerStyleUniform> uniforms, const Containers::StridedArrayView1D<const Vector4>& paddings);
        /** @overload */
        Shared& setStyle(const BaseLayerCommonStyleUniform& commonUniform, std::initializer_list<BaseLayerStyleUniform> uniforms, std::initializer_list<Vector4> paddings);

        /**
         * @brief Set style data
         * @param commonUniform     Common style uniform data
         * @param uniforms          Style uniforms
         * @param styleToUniform    Style to style uniform mapping
         * @param stylePaddings     Per-style padding inside the node in UI
         *      units, in order left, top, right, bottom
         * @return Reference to self (for method chaining)
         *
         * The @p uniforms view is expected to have the same size as
         * @ref styleUniformCount(), the @p styleToUniform view the same size
         * as @ref styleCount(). All uniform indices are expected to be less
         * than @ref styleUniformCount().
         *
         * The @p stylePaddings view is expected to either have the same size
         * as @ref styleCount() or be empty, in which case all paddings are
         * implicitly zero.
         *
         * Value of @cpp styleToUniform[i] @ce should give back an index into
         * the @p uniforms array for style @cpp i @ce. If
         * @ref styleUniformCount() and @ref styleCount() is the same and the
         * mapping is implicit, you can use the
         * @ref setStyle(const BaseLayerCommonStyleUniform&, Containers::ArrayView<const BaseLayerStyleUniform>, const Containers::StridedArrayView1D<const Vector4>&)
         * convenience overload instead.
         *
         * Calling this function causes @ref LayerState::NeedsDataUpdate to be
         * set on all layers that are constructed using this shared instance.
         * If @ref dynamicStyleCount() is non-zero,
         * @ref LayerState::NeedsCommonDataUpdate is set as well to trigger an
         * upload of changed dynamic style uniform data.
         */
        Shared& setStyle(const BaseLayerCommonStyleUniform& commonUniform, Containers::ArrayView<const BaseLayerStyleUniform> uniforms, const Containers::StridedArrayView1D<const UnsignedInt>& styleToUniform, const Containers::StridedArrayView1D<const Vector4>& stylePaddings);
        /** @overload */
        Shared& setStyle(const BaseLayerCommonStyleUniform& commonUniform, std::initializer_list<BaseLayerStyleUniform> uniforms, std::initializer_list<UnsignedInt> styleToUniform, std::initializer_list<Vector4> stylePaddings);

        /* Overloads to remove a WTF factor from method chaining order */
        #ifndef DOXYGEN_GENERATING_OUTPUT
        MAGNUMEXTRAS_UI_ABSTRACTVISUALLAYER_SHARED_SUBCLASS_IMPLEMENTATION()
        #endif

    #ifdef DOXYGEN_GENERATING_OUTPUT
    private:
    #else
    protected:
    #endif
        struct State;
        friend BaseLayer;
        friend BaseLayerStyleAnimator;

        MAGNUM_UI_LOCAL explicit Shared(Containers::Pointer<State>&& state);
        /* Used by tests to avoid having to include / allocate the state */
        explicit Shared(const Configuration& configuration);
        /* Can't be MAGNUM_UI_LOCAL, used by tests */
        explicit Shared(NoCreateT) noexcept;

    private:
        MAGNUM_UI_LOCAL void setStyleInternal(const BaseLayerCommonStyleUniform& commonUniform, Containers::ArrayView<const BaseLayerStyleUniform> styleUniforms, const Containers::StridedArrayView1D<const Vector4>& stylePaddings);

        /* The items are guaranteed to have the same size as
           styleUniformCount(). Called only if there are no dynamic styles,
           otherwise the data are copied to internal arrays to be subsequently
           combined with dynamic uniforms and uploaded together in doDraw(). */
        virtual void doSetStyle(const BaseLayerCommonStyleUniform& commonUniform, Containers::ArrayView<const BaseLayerStyleUniform> uniforms) = 0;
};

/**
@brief Configuration of a base layer shared state

See the @ref BaseLayer class documentation for information about setting up an
instance of this layer and using it.
@see @ref BaseLayerGL::Shared::Shared(const Configuration&)
*/
class MAGNUM_UI_EXPORT BaseLayer::Shared::Configuration {
    public:
        /**
         * @brief Constructor
         *
         * The @p styleUniformCount parameter specifies the size of the uniform
         * array, @p styleCount then the number of distinct styles to use for
         * drawing. The sizes are independent in order to allow styles with
         * different paddings share the same uniform data. Either both
         * @p styleUniformCount and @p styleCount is expected to be non-zero,
         * or both zero with a non-zero dynamic style count specified with
         * @ref setDynamicStyleCount(). Style data are then set with
         * @ref setStyle(), dynamic style data then with
         * @ref BaseLayer::setDynamicStyle().
         */
        explicit Configuration(UnsignedInt styleUniformCount, UnsignedInt styleCount);

        /**
         * @brief Construct with style uniform count being the same as style count
         *
         * Equivalent to calling @ref Configuration(UnsignedInt, UnsignedInt)
         * with both parameters set to @p styleCount.
         */
        explicit Configuration(UnsignedInt styleCount): Configuration{styleCount, styleCount} {}

        /** @brief Style uniform count */
        UnsignedInt styleUniformCount() const { return _styleUniformCount; }

        /** @brief Style count */
        UnsignedInt styleCount() const { return _styleCount; }

        /** @brief Dynamic style count */
        UnsignedInt dynamicStyleCount() const { return _dynamicStyleCount; }

        /**
         * @brief Set dynamic style count
         * @return Reference to self (for method chaining)
         *
         * Initial count is @cpp 0 @ce.
         * @see @ref Configuration(UnsignedInt, UnsignedInt),
         *      @ref AbstractVisualLayer::allocateDynamicStyle(),
         *      @ref AbstractVisualLayer::recycleDynamicStyle(),
         *      @ref AbstractVisualLayer::dynamicStyleUsedCount()
         */
        Configuration& setDynamicStyleCount(UnsignedInt count) {
            _dynamicStyleCount = count;
            return *this;
        }

        /** @brief Flags */
        BaseLayerSharedFlags flags() const { return _flags; }

        /**
         * @brief Set flags
         * @return Reference to self (for method chaining)
         *
         * By default no flags are set.
         * @see @ref addFlags(), @ref clearFlags()
         */
        Configuration& setFlags(BaseLayerSharedFlags flags) {
            _flags = flags;
            return *this;
        }

        /**
         * @brief Add flags
         * @return Reference to self (for method chaining)
         *
         * Calls @ref setFlags() with the existing flags ORed with @p flags.
         * Useful for preserving previously set flags.
         * @see @ref clearFlags()
         */
        Configuration& addFlags(BaseLayerSharedFlags flags) {
            return setFlags(_flags|flags);
        }

        /**
         * @brief Clear flags
         * @return Reference to self (for method chaining)
         *
         * Calls @ref setFlags() with the existing flags ANDed with the inverse
         * of @p flags. Useful for removing a subset of previously set flags.
         * @see @ref addFlags()
         */
        Configuration& clearFlags(BaseLayerSharedFlags flags) {
            return setFlags(_flags & ~flags);
        }

        /** @brief Background blur radius */
        UnsignedInt backgroundBlurRadius() const {
            return _backgroundBlurRadius;
        }

        /** @brief Background blur sampling cutoff */
        Float backgroundBlurCutoff() const { return _backgroundBlurCutoff; }

        /**
         * @brief Set background blur radius and sampling cutoff
         * @param radius    Gaussian blur radius
         * @param cutoff    Cutoff for weight values after which no sampling is
         *      done
         * @return Reference to self (for method chaining)
         *
         * Expects that the @p radius is less than @cpp 31 @ce, value of
         * @cpp 0 @ce makes the blur sample only the center pixel, effectively
         * not blurring anything. The @p cutoff value controls a balance
         * between speed and precision, @cpp 0.0f @ce samples all pixels in
         * given radius even though they may have a weight that doesn't result
         * in any contribution to the quantized output, higher values gradually
         * reduce the number of sampled pixels down to just one.
         *
         * While the radius is a static setting that has to be set upfront,
         * there's also pass count that can be changed dynamically at runtime.
         * See @ref BaseLayer::setBackgroundBlurPassCount() for more
         * information and details about how it interacts with blur radius.
         *
         * Initial @p radius is @cpp 4 @ce and @p cutoff is
         * @cpp 0.5f/255.0f @ce, i.e. weights that don't contribute any value
         * even when combined from both sides of the blur circle for a 8bpp
         * render target are ignored.
         */
        Configuration& setBackgroundBlurRadius(UnsignedInt radius, Float cutoff = 0.5f/255.0f);

    private:
        UnsignedInt _styleUniformCount, _styleCount;
        UnsignedInt _dynamicStyleCount = 0;
        BaseLayerSharedFlags _flags;
        UnsignedInt _backgroundBlurRadius = 4;
        Float _backgroundBlurCutoff = 0.5f/255.0f;
};

inline BaseLayer::Shared& BaseLayer::shared() {
    return static_cast<Shared&>(AbstractVisualLayer::shared());
}

inline const BaseLayer::Shared& BaseLayer::shared() const {
    return static_cast<const Shared&>(AbstractVisualLayer::shared());
}

}}

#endif
