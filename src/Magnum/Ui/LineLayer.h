#ifndef Magnum_Ui_LineLayer_h
#define Magnum_Ui_LineLayer_h
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
 * @brief Class @ref Magnum::Ui::LineLayer, struct @ref Magnum::Ui::LineLayerCommonStyleUniform, @ref Magnum::Ui::LineLayerStyleUniform, enum @ref Magnum::Ui::LineCapStyle, @ref Magnum::Ui::LineJoinStyle, @ref Magnum::Ui::LineAlignment
 * @m_since_latest
 */

#include <initializer_list>
#include <Magnum/Math/Color.h>

#include "Magnum/Ui/AbstractVisualLayer.h"

namespace Magnum { namespace Ui {

/**
@brief Properties common to all @ref LineLayer style uniforms
@m_since_latest

See the @ref LineLayer class documentation for information about setting up an
instance of the line layer and using it.

Together with one or more @ref LineLayerStyleUniform instances contains style
properties that are used by the @ref LineLayer shaders to draw the layer data,
packed in a form that allows direct usage in uniform buffers. Is uploaded
using @ref LineLayer::Shared::setStyle(), style data that aren't used by the
shader are passed to the function separately.
*/
struct LineLayerCommonStyleUniform {
    /** @brief Construct with default values */
    constexpr explicit LineLayerCommonStyleUniform(DefaultInitT) noexcept: smoothness{0.0f} {}

    /**
     * @brief Construct with default values
     *
     * Equivalent to @ref LineLayerCommonStyleUniform(DefaultInitT).
     */
    constexpr /*implicit*/ LineLayerCommonStyleUniform() noexcept: LineLayerCommonStyleUniform{DefaultInit} {}

    /** @brief Constructor */
    constexpr /*implicit*/ LineLayerCommonStyleUniform(Float smoothness): smoothness {smoothness} {}

    /** @brief Construct without initializing the contents */
    explicit LineLayerCommonStyleUniform(NoInitT) noexcept {}

    /** @{
     * @name Convenience setters
     *
     * Provided to allow the use of method chaining for populating a structure
     * in a single expression, otherwise equivalent to accessing the fields
     * directly. Also guaranteed to provide backwards compatibility when
     * packing of the actual fields changes.
     */

    /**
     * @brief Set the @ref smoothness field
     * @return Reference to self (for method chaining)
     */
    CORRADE_CONSTEXPR14 LineLayerCommonStyleUniform& setSmoothness(Float smoothness) {
        this->smoothness = smoothness;
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
     * the edge. Default value is @cpp 0.0f @ce. The bigger value between this
     * and @ref LineLayerStyleUniform::smoothness, converted to pixels, gets
     * used.
     * @see @ref Ui-AbstractUserInterface-dpi
     */
    Float smoothness;

    #ifndef DOXYGEN_GENERATING_OUTPUT
    Int:32;
    Int:32;
    Int:32;
    #endif
};

/**
@brief @ref LineLayer style uniform
@m_since_latest

See the @ref LineLayer class documentation for information about setting up an
instance of the line layer and using it.

Instances of this class together with @ref LineLayerCommonStyleUniform contain
style properties that are used by the @ref LineLayer shaders to draw the layer
data, packed in a form that allows direct usage in uniform buffers. Total count
of styles is specified with the
@ref LineLayer::Shared::Configuration::Configuration() constructor, uniforms
are then uploaded using @ref LineLayer::Shared::setStyle(), style data that
aren't used by the shader are passed to the function separately.
*/
struct MAGNUM_UI_EXPORT LineLayerStyleUniform {
    /** @brief Construct with default values */
    constexpr explicit LineLayerStyleUniform(DefaultInitT) noexcept: color{1.0f}, width{1.0f}, smoothness{0.0f}, miterLimit{0.875f} {}

    /**
     * @brief Construct with default values
     *
     * Equivalent to @ref LineLayerStyleUniform(DefaultInitT).
     */
    constexpr /*implicit*/ LineLayerStyleUniform() noexcept: LineLayerStyleUniform{DefaultInit} {}

    /** @brief Constructor */
    constexpr /*implicit*/ LineLayerStyleUniform(const Color4& color, Float width, Float smoothness, Float miterLimit) noexcept: color{color}, width{width}, smoothness{smoothness}, miterLimit{miterLimit} {}

    /** @brief Construct without initializing the contents */
    explicit LineLayerStyleUniform(NoInitT) noexcept: color{NoInit} {}

    /** @{
     * @name Convenience setters
     *
     * Provided to allow the use of method chaining for populating a structure
     * in a single expression, otherwise equivalent to accessing the fields
     * directly. Also guaranteed to provide backwards compatibility when
     * packing of the actual fields changes.
     */

    /**
     * @brief Set the @ref color field
     * @return Reference to self (for method chaining)
     */
    CORRADE_CONSTEXPR14 LineLayerStyleUniform& setColor(const Color4& color) {
        this->color = color;
        return *this;
    }

    /**
     * @brief Set the @ref width field
     * @return Reference to self (for method chaining)
     */
    CORRADE_CONSTEXPR14 LineLayerStyleUniform& setWidth(Float width) {
        this->width = width;
        return *this;
    }

    /**
     * @brief Set the @ref smoothness field
     * @return Reference to self (for method chaining)
     */
    CORRADE_CONSTEXPR14 LineLayerStyleUniform& setSmoothness(Float smoothness) {
        this->smoothness = smoothness;
        return *this;
    }

    /**
     * @brief Set the @ref miterLimit field
     * @return Reference to self (for method chaining)
     *
     * For convenience it's recommended to use the @ref setMiterLengthLimit()
     * and @ref setMiterAngleLimit() helpers instead of setting this value
     * directly.
     */
    CORRADE_CONSTEXPR14 LineLayerStyleUniform& setMiterLimit(Float limit) {
        this->miterLimit = limit;
        return *this;
    }

    /**
     * @brief Set the @ref miterLimit field to a length value
     * @return Reference to self (for method chaining)
     *
     * Expects that @p limit is greater than or equal to @cpp 1.0f @ce and
     * finite.
     */
    LineLayerStyleUniform& setMiterLengthLimit(Float limit);

    /**
     * @brief Set the @ref miterLimit field to an angle value
     * @return Reference to self (for method chaining)
     *
     * Expects that @p limit is greater than @cpp 0.0_radf @ce.
     */
    LineLayerStyleUniform& setMiterAngleLimit(Rad limit);

    /**
     * @}
     */

    /**
     * @brief Line color
     *
     * The color is expected to have premultiplied alpha. Default value is
     * @cpp 0xffffffff_srgbaf @ce. The color is further multiplied with
     * per-data value supplied with @ref LineLayer::setColor(), color specified
     * for individual points in @ref LineLayer::create(),
     * @relativeref{LineLayer,setLine()} and overloads, and with node opacity
     * coming from @ref AbstractUserInterface::setNodeOpacity().
     * @see @ref Color4::premultiplied()
     */
    Color4 color;

    /**
     * @brief Line width
     *
     * In UI units. Default value is @cpp 1.0f @ce.
     */
    Float width;

    /**
     * @brief Edge smoothness radius
     *
     * Compared to @ref LineLayerCommonStyleUniform::smoothness is in UI units
     * instead of pixels. Default is @cpp 0.0f @ce. Of the two, the larger
     * value in pixels gets used.
     */
    Float smoothness;

    /**
     * @brief Miter limit
     *
     * Limit at which a @ref LineJoinStyle::Miter join is converted to a
     * @ref LineJoinStyle::Bevel in order to avoid sharp corners extending too
     * much. If joint style is not @ref LineJoinStyle::Miter, this value is
     * unused.
     *
     * Represented as a cosine of the angle between two neighboring line
     * segments, with @ref LineJoinStyle::Bevel used for angles below the limit
     * (thus their cosine larger than this value). For length-based limits,
     * the relation between angle @f$ \theta @f$, miter length @f$ l @f$ and
     * line half-width @f$ w @f$ is as follows: @f[
     *      \frac{w}{l} = \sin(\frac{\theta}{2})
     * @f]
     *
     * For convenience it's recommended to use the @ref setMiterLengthLimit()
     * and @ref setMiterAngleLimit() helpers instead of setting this value
     * directly. Default value is @cpp 0.875f @ce, which corresponds to a
     * length of @cpp 4.0f @ce and angle of approximately @cpp 28.955_degf @ce.
     */
    Float miterLimit;

    #ifndef DOXYGEN_GENERATING_OUTPUT
    Int:32;
    #endif
};

/**
@brief Line cap style
@m_since_latest

@see @ref LineLayer::Shared::capStyle(),
    @ref LineLayer::Shared::Configuration::setCapStyle(), @ref LineJoinStyle
*/
enum class LineCapStyle: UnsignedByte {
    /* Keep these in sync with Shaders::LineCapStyle (except for the related
       links, of course). The images are used directly from there. */

    /**
     * [Butt cap](https://en.wikipedia.org/wiki/Butt_joint). The line is cut
     * off right at the endpoint. Lines of zero length will be invisible.
     *
     * @htmlinclude line-cap-butt.svg
     */
    Butt,

    /**
     * Square cap. The line is extended by half of its width past the endpoint.
     * Lines of zero length will be shown as squares.
     *
     * @htmlinclude line-cap-square.svg
     */
    Square,

    /**
     * Round cap. The line is extended by half of its width past the endpoint.
     * It's still rendered as a quad but pixels outside of the half-circle are
     * transparent. Lines of zero length will be shown as circles.
     *
     * @htmlinclude line-cap-round.svg
     */
    Round,

    /**
     * Triangle cap. The line is extended by half of its width past the
     * endpoint. It's still rendered as a quad but pixels outside of the
     * triangle are transparent. Lines of zero length will be shown as squares
     * rotated by 45°.
     *
     * @htmlinclude line-cap-triangle.svg
     */
    Triangle
};

/**
@debugoperatorenum{LineCapStyle}
@m_since_latest
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, LineCapStyle value);

/**
@brief Line join style
@m_since_latest

@see @ref LineLayer::Shared::joinStyle(),
    @ref LineLayer::Shared::Configuration::setJoinStyle(), @ref LineCapStyle
*/
enum class LineJoinStyle: UnsignedByte {
    /* Keep these in sync with Shaders::LineJoinStyle (except for the related
       links, of course). The images are used directly from there. */

    /**
     * [Miter join](https://en.wikipedia.org/wiki/Miter_joint). The outer edges
     * of both line segments extend until they intersect.
     *
     * @htmlinclude line-join-miter.svg
     *
     * In this style, the points `A`, `B` and `C` collapse to a zero-area
     * triangle. If the miter length `l` would be larger than the limit set via
     * @ref LineLayerStyleUniform::setMiterLengthLimit() or the angle between
     * the two segments `α` would be less than the limit set via
     * @ref LineLayerStyleUniform::setMiterAngleLimit(), it switches to
     * @ref LineJoinStyle::Bevel instead.
     */
    Miter,

    /**
     * [Bevel join](https://en.wikipedia.org/wiki/Bevel). Outer edges of both
     * line segments are cut off at a right angle at their endpoints.
     *
     * @htmlinclude line-join-bevel.svg
     *
     * The area between points `A`, `B` and `C` is filled with an extra
     * triangle.
     */
    Bevel
};

/**
@debugoperatorenum{LineJoinStyle}
@m_since_latest
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, LineJoinStyle value);

namespace Implementation {
    enum: UnsignedByte {
        /* Middle/Center, which places the origin to the node center, is
           deliberately 0 to signify a default */

        LineAlignmentLeft = 1 << 0,
        LineAlignmentCenter = 0 << 0,
        LineAlignmentRight = 2 << 0,
        LineAlignmentHorizontal = LineAlignmentLeft|LineAlignmentCenter|LineAlignmentRight,

        LineAlignmentTop = 1 << 2,
        LineAlignmentMiddle = 0 << 2,
        LineAlignmentBottom = 2 << 2,
        LineAlignmentVertical = LineAlignmentTop|LineAlignmentMiddle|LineAlignmentBottom,
    };
}

/**
@brief Line alignment
@m_since_latest

@see @ref LineLayer::setAlignment()
*/
enum class LineAlignment: UnsignedByte {
    /**
     * Origin is put at the top left corner of a node, offset by top left
     * padding.
     */
    TopLeft = Implementation::LineAlignmentTop|Implementation::LineAlignmentLeft,

    /**
     * Origin is put at the center of the top node edge, offset by top and left
     * / right padding.
     */
    TopCenter = Implementation::LineAlignmentTop|Implementation::LineAlignmentCenter,

    /**
     * Origin is put at the top right corner of a node, offset by top right
     * padding.
     */
    TopRight = Implementation::LineAlignmentTop|Implementation::LineAlignmentRight,

    /**
     * Origin is put at the center of the left node edge, offset by left and
     * top / bottom padding.
     */
    MiddleLeft = Implementation::LineAlignmentMiddle|Implementation::LineAlignmentLeft,

    /**
     * Origin is put at the center of the node rectangle, offset by left /
     * right and top / bottom padding.
     */
    MiddleCenter = Implementation::LineAlignmentMiddle|Implementation::LineAlignmentCenter,

    /**
     * Origin is put at the center of the right node edge, offset by right and
     * top / bottom padding.
     */
    MiddleRight = Implementation::LineAlignmentMiddle|Implementation::LineAlignmentRight,

    /**
     * Origin is put at the bottom left corner of a node, offset by bottom left
     * padding.
     */
    BottomLeft = Implementation::LineAlignmentBottom|Implementation::LineAlignmentLeft,

    /**
     * Origin is put at the center of the bottom node edge, offset by bottom
     * and left / right padding.
     */
    BottomCenter = Implementation::LineAlignmentBottom|Implementation::LineAlignmentCenter,

    /**
     * Origin is put at the bottom right corner of a node, offset by bottom
     * right padding.
     */
    BottomRight = Implementation::LineAlignmentBottom|Implementation::LineAlignmentRight,
};

/**
@debugoperatorenum{LineAlignment}
@m_since_latest
*/
MAGNUM_UI_EXPORT Debug& operator<<(Debug& debug, LineAlignment value);

/**
@brief Line layer
@m_since_latest

Draws smooth wide lines with configurable caps and joins and per-point colors.
Based on the same internal implementation as @ref Shaders::LineGL.

@section Ui-LineLayer-setup Setting up a line layer instance

First you need to instantiate @ref LineLayer::Shared, which contains GPU
shaders and style definitions. It takes a @ref LineLayer::Shared::Configuration,
where at the very least you have to specify how many distinct visual *styles*
you intend to use --- which is for example the number @cpp 3 @ce in the
following snippet:

@snippet Ui-gl.cpp LineLayer-setup-shared

The shared instance, in this case a concrete @ref LineLayerGL::Shared subclass
for the OpenGL implementation of this layer, is then passed to the layer
constructor alongside a fresh @ref AbstractUserInterface::createLayer() handle,
and is expected to stay alive for the whole layer lifetime. The shared instance
can be used by multiple layers, for example if the application wants to have a
dedicated layer for very dynamic UI content, or if it combines visual options
that have to be hardcoded in particular @ref LineLayer::Shared instances. Pass
the newly created layer to @ref AbstractUserInterface::setLayerInstance():

@snippet Ui-gl.cpp LineLayer-setup

Afterwards, in order to be able to draw the layer, a style has to be set with
@ref LineLayer::Shared::setStyle(). At the very least you're expected to pass a
@ref LineLayerCommonStyleUniform containing properties common to all styles, an
array of @ref LineLayerStyleUniform matching the style count set in the
@ref LineLayer::Shared::Configuration and a list of per-style alignment values
specifying how a particular line art should be placed inside each node.
Default-constructed instances will result in white single-unit-wide lines, you
can then use method chaining to update only the properties you're interested
in; zero-initialized @ref LineAlignment value is equivalent to
@ref LineAlignment::MiddleCenter. So, for example, with style @cpp 0 @ce being
the default, style @cpp 1 @ce being a centered blue wide line and style
@cpp 2 @ce an azure glow aligned to the bottom left corner of the node:

@snippet Ui.cpp LineLayer-setup-style

With this, assuming @ref AbstractUserInterface::draw() is called in an
appropriate place, the layer is ready to use. All style options are described
in detail further below.

@m_class{m-note m-info}

@par Using an enum for style indexing
    Like with @ref BaseLayer, it's possible to use an @cpp enum @ce for
    populating @ref LineLayer styles and referencing them when creating data.
    See the corresponding @ref Ui-BaseLayer-style-enums "BaseLayer documentation chapter"
    for a detailed example.

Unlike with @ref BaseLayer or @ref TextLayer, the default @ref UserInterface
implementation doesn't implicitly provide a @ref LineLayer instance.

@section Ui-LineLayer-create Creating lines

A line strip is created by calling @ref createStrip() with desired style index,
a list of points that are meant to be connected together and a @ref NodeHandle
the data should be attached to. In this case it picks the style @cpp 1 @ce from
above, which makes a wide blue line:

@snippet Ui.cpp LineLayer-create-strip

The @ref createLoop() function then creates a loop, connecting also the first
and last point back together. If you give it just a single point, it will
render a literal point. The visual output of these three calls is shown below.

@snippet Ui.cpp LineLayer-create-loop

@image html ui-linelayer-create.png width=256px

Both @ref createStrip() and @ref createLoop() are convenience alternatives to
@ref create(). This function takes an indexed list of points, where each pair
of indices describes a line segment. Points that are referenced exactly twice form a line join, if they're referenced twice from a single pair, they form a literal
point. The index buffer thus allows you to put multiple disjoint line strips
and loops together. An indexed equivalent to the three calls to
@ref createStrip() and @ref createLoop() from above would look like this:

@snippet Ui.cpp LineLayer-create-indexed

As with all other data, they're implicitly tied to lifetime of the node they're
attached to. You can remember the @ref DataHandle returned by @ref create(),
@ref createStrip() or @ref createLoop() to modify the data later, @ref attach()
to a different node or @ref remove() it.

@section Ui-LineLayer-style Style options

@subsection Ui-LineLayer-style-smoothness Edge smoothness

@image html ui-linelayer-style-smoothness.png width=256px

One of the most desirable aspects of line rendering is antialiasing. Like
@ref BaseLayer, @ref LineLayer performs it in the shader without needing to
rely on multisampling. With @ref LineLayerCommonStyleUniform::setSmoothness(),
like the corresponding style option in @ref BaseLayer, you set up a smoothness
radius in framebuffer pixels. It's not in UI units in order to ensure crisp
look everywhere without having to specifically deal with HiDPI displays. Additionally, for creating glow, shadows and other effects,
@ref LineLayerStyleUniform::setSmoothness() can override the smoothness per
style, and this value is in UI units instead of pixels. The desired usage is
that the common smoothness gets tuned for
@ref Ui-BaseLayer-style-rounded-corners-smoothness "BaseLayer", line layer and
any @ref Ui-TextLayer-distancefield-smoothness "distance-field-enabled TextLayer"
to a value that makes sense for the UI in general, and the per-style smoothness
is then used for line elements that need a fuzzy appearance. Of the two values
coming from the style, the larger one --- when both are converted to the same
units --- gets used.

@snippet Ui.cpp LineLayer-style-smoothness

@subsection Ui-LineLayer-style-cap-join Line cap and join

The layer allows choosing between various cap and join styles shown below. The
default is @ref LineCapStyle::Square and @ref LineJoinStyle::Miter, and
currently this option is statically compiled into the shader, which means you
have to choose it upfront via @ref Shared::Configuration::setCapStyle() and
@ref Shared::Configuration::setJoinStyle().

@m_class{m-row}

@parblock

@m_div{m-col-l-6  m-text-center}
@htmlinclude line-cap-butt.svg
@ref LineCapStyle::Butt
@m_enddiv

@m_div{m-col-l-6 m-text-center}
@htmlinclude line-cap-square.svg
@ref LineCapStyle::Square
@m_enddiv

@m_div{m-col-l-6 m-text-center}
@htmlinclude line-cap-round.svg
@ref LineCapStyle::Round
@m_enddiv

@m_div{m-col-l-6 m-text-center}
@htmlinclude line-cap-triangle.svg
@ref LineCapStyle::Triangle
@m_enddiv

@endparblock

@ref LineJoinStyle::Miter joins have a configurable limit to prevent them from
getting too long with sharp angles, after which they become
@ref LineJoinStyle::Bevel. The limit is configurable per style using
@ref LineLayerStyleUniform::setMiterAngleLimit() and
@ref LineLayerStyleUniform::setMiterLengthLimit().

@m_class{m-row}

@parblock

@m_div{m-col-l-6  m-text-center}
@htmlinclude line-join-miter.svg
@ref LineJoinStyle::Miter
@m_enddiv

@m_div{m-col-l-6 m-text-center}
@htmlinclude line-join-bevel.svg
@ref LineJoinStyle::Bevel
@m_enddiv

@endparblock

If you need to combine lines with different cap and join styles, you can create
multiple line layers, each with a differently-configured @ref LineLayer::Shared
instance:

@snippet Ui-gl.cpp LineLayer-style-cap-join

@subsection Ui-LineLayer-style-color Style, per-data and per-point color

@image html ui-linelayer-style-color.png width=256px

In addition to @ref LineLayerStyleUniform::setColor() shown above, the color is
multiplied by a data-specific color set with @ref setColor(). The main use
case, like with @ref BaseLayer, is to allow for example custom plot coloring
where it would be impractical to dynamically update style data based on what's
being shown. The color is further multiplied by a per-node opacity coming from
@ref AbstractUserInterface::setNodeOpacity(), which can be used for various
fade-in / fade-out effects.

@snippet Ui.cpp LineLayer-style-color1

Finally, as visualized with the fourth arc above, it's possible to color each
individual point by passing a list of colors to @ref create(),
@ref createStrip() or @ref createLoop(), one for each point. The per-point
colors then get multiplied with the other colors set for the style and the
data.

@snippet Ui.cpp LineLayer-style-color2

@subsection Ui-LineLayer-style-alignment-padding Alignment and padding inside the node

@ref LineAlignment::MiddleCenter --- or the default-constructed
@ref LineAlignment value --- passed to @ref LineLayer::Shared::setStyle()
aligns @cpp {0.0f, 0.0f} @ce to a vertical and horizontal center of given node,
thus as long as the points are aligned around that origin, the whole line art
will be centered. At the moment, it's not possible to align based on the actual
bounding rectangle of the point data.

@ref LineAlignment::TopLeft and other alignment values move the origin to the
edges and corners, which is useful for example with various data plots. With
those, and especially when combined with wider lines, it's useful to specify
also a padding inside the node. That's done with the last argument to
@ref LineLayer::Shared::setStyle(), and the result is that a line crossing
through @cpp {0.0f, 0.0f} @ce won't partially leak out of node edges. Similarly
as with @ref Ui-BaseLayer-style-padding "padding in BaseLayer", it's one value
for each of four node edges, but often a single value for all is enough:

@snippet Ui.cpp LineLayer-style-alignment-padding

Finally, both alignment and padding can be overriden on a per-data basis with
@ref setAlignment() and @ref setPadding(), which may be useful for example when
aligning line art next to variable-width text. Note that, however, the draw
order isn't guaranteed in case of multiple data attached to the same node, so
it only gives a reliable output as long as the shapes don't overlap.

@subsection Ui-LineLayer-style-width-outline Line width and outline

@image html ui-linelayer-style-outline.png width=128px

As shown above, line width is controllable with
@ref LineLayerStyleUniform::setWidth(). At the moment there's no possibility of
having an outline, however you can emulate this to a certain degree by putting
another, slightly wider, line consisting of the same points underneath. In
order to ensure the outline is drawn below, it has to be put into a parent
node. In the following snippet, a white dot with a dark slightly blurry outline
is put on top of a blue circle, thus needing three nested nodes:

@snippet Ui.cpp LineLayer-style-width

@section Ui-LineLayer-style-transitions Style transition based on input events

Like with @ref BaseLayer, it's possible to configure @ref LineLayer to perform
automatic style transitions based on input events, such as highlighting on
hover or press. See the @ref Ui-BaseLayer-style-transitions "BaseLayer documentation for style transition"
for a detailed example, the interfaces are the same between the two.

@m_class{m-note m-warning}

@par
    Note that, however, at the moment, the whole node area reacts to the event,
    not just when a particular line segment is under the pointer. Thus it's
    currently not possible to implement for example selection of particular
    lines in a plot.

@section Ui-LineLayer-data-update Updating line data

Any created line can be subsequently updated with @ref setLineStrip(),
@ref setLineLoop() and @ref setLine(). Those functions take the same arguments
and behave the same as as @ref createStrip(), @ref createLoop() and
@ref create(), and are a more efficient operation than data removal and
recreation if the point and index count doesn't change, which is a common case
for example when updating plotted values.

Changing the point / index count is supported however and internally there's
also no distinction between a strip, loop or an indexed line, so a strip can be
safely changed to a loop etc.

@section Ui-LineLayer-debug-integration Debug layer integration

When using @ref Ui-DebugLayer-node-highlight "DebugLayer node highlighting",
this layer inherits @ref Ui-AbstractVisualLayer-debug-integration "debug integration from the AbstractVisualLayer".
See its documentation for more information. If
@ref DebugLayerSource::NodeDataAttachmentDetails is enabled, for @ref LineLayer
the output additionally shows if given data has custom color, alignment or
padding, to clearly distinguish it from just a vanilla style. For example:

@include ui-debuglayer-textlayer.ansi
*/
class MAGNUM_UI_EXPORT LineLayer: public AbstractVisualLayer {
    public:
        class Shared;
        class DebugIntegration;

        /**
         * @brief Shared state used by this layer
         *
         * Reference to the instance passed to @ref LineLayerGL::LineLayerGL(LayerHandle, Shared&).
         */
        inline Shared& shared();
        inline const Shared& shared() const; /**< @overload */

        /**
         * @brief Create a line from an indexed list of points
         * @param style         Style index
         * @param indices       Indices pointing into the @p points view
         * @param points        Line points, in UI units, indexed by @p indices
         * @param colors        Optional per-point colors
         * @param node          Node to attach to
         * @return New data handle
         *
         * Expects that @p style is less than @ref Shared::totalStyleCount().
         * All styling is driven from the @ref LineLayerStyleUniform at index
         * @p style.
         *
         * The @p indices are expected to have an even size and their values
         * are all expected to be less than size of @p points. Every successive
         * pair of values describes one line segment. If the same index is used
         * exactly twice in two different segments, it's drawn as a line join,
         * otherwise it's drawn as a line cap. A pair of the same values draws
         * a point. For example, assuming the @p points array has at least 7
         * items, the following sequence of indices draws a closed line loop, a
         * single line segment and a point:
         *
         * @code{.cpp}
         * 0, 1, 1, 2, 2, 3, 3, 0,  // loop with four segments
         * 4, 5,                    // standalone line segment with two caps
         * 6, 6                     // a single point
         * @endcode
         *
         * Note that the only purpose of the index buffer is to describe
         * connections between line points and for rendering the lines get
         * converted to a different representation. It's not an error if the
         * index buffer doesn't reference all @p points, it's also not an error
         * if the same point is present more than once.
         *
         * The @p colors array is expected to be either empty or have the same
         * size as @p points. If non-empty, the colors are expected to have
         * premultiplied alpha and each point is drawn with a corresponding
         * color that's further multiplied by a color coming from the style,
         * from @ref setColor() and with node opacity coming from
         * @ref AbstractUserInterface::setNodeOpacity(). If empty, it's as if
         * an array of @cpp 0xffffffff_srgbaf @ce was supplied.
         * @see @ref createStrip(), @ref createLoop(), @ref setAlignment(),
         *      @ref Color4::premultiplied()
         */
        /* This one takes Vector4 instead of Color4 because color views are
           implicitly convertible to vectors but not the other way around */
        DataHandle create(UnsignedInt style, const Containers::StridedArrayView1D<const UnsignedInt>& indices, const Containers::StridedArrayView1D<const Vector2>& points, const Containers::StridedArrayView1D<const Vector4>& colors, NodeHandle node =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            NodeHandle::Null
            #else
            NodeHandle{} /* To not have to include Handle.h */
            #endif
        );
        /* There is no Vector3 / Color3 overload because, since the colors are
           optional, it'd be ambiguous to call the function with just {} or
           nullptr, making the most common use case too annoying */
        /** @overload */
        /* This one takes a Color4 instead of Vector4 in order to have e.g.
           0x993366_rgbf implicitly converted to 0x993366ff_rgbaf */
        DataHandle create(UnsignedInt style, std::initializer_list<UnsignedInt> indices, std::initializer_list<Vector2> points, std::initializer_list<Color4> colors, NodeHandle node =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            NodeHandle::Null
            #else
            NodeHandle{} /* To not have to include Handle.h */
            #endif
        );

        /**
         * @brief Create a line from an indexed list of points with a style index in a concrete enum type
         *
         * Casts @p style to @relativeref{Magnum,UnsignedInt} and delegates to
         * @ref create(UnsignedInt, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector4>&, NodeHandle).
         */
        template<class StyleIndex
            #ifndef DOXYGEN_GENERATING_OUTPUT
            /* Accept any enum except NodeHandle to prevent create(node, ...)
               from being called by mistake */
            , typename std::enable_if<std::is_enum<StyleIndex>::value && !std::is_same<StyleIndex, NodeHandle>::value, int>::type = 0
            #endif
        > DataHandle create(StyleIndex style, const Containers::StridedArrayView1D<const UnsignedInt>& indices, const Containers::StridedArrayView1D<const Vector2>& points, const Containers::StridedArrayView1D<const Vector4>& colors, NodeHandle node =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            NodeHandle::Null
            #else
            NodeHandle{} /* To not have to include Handle.h */
            #endif
        ) {
            return create(UnsignedInt(style), indices, points, colors, node);
        }
        /** @overload */
        template<class StyleIndex
            #ifndef DOXYGEN_GENERATING_OUTPUT
            /* Accept any enum except NodeHandle to prevent create(node, ...)
               from being called by mistake */
            , typename std::enable_if<std::is_enum<StyleIndex>::value && !std::is_same<StyleIndex, NodeHandle>::value, int>::type = 0
            #endif
        > DataHandle create(StyleIndex style, std::initializer_list<UnsignedInt> indices, std::initializer_list<Vector2> points, std::initializer_list<Color4> colors, NodeHandle node =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            NodeHandle::Null
            #else
            NodeHandle{} /* To not have to include Handle.h */
            #endif
        ) {
            return create(UnsignedInt(style), indices, points, colors, node);
        }

        /**
         * @brief Create a line strip
         * @param style         Style index
         * @param points        Line strip points, in UI units
         * @param colors        Optional per-point colors
         * @param node          Node to attach to
         * @return New data handle
         *
         * Creates a single connected line strip. The @p points are expected to
         * be either empty or at least two. Convenience equivalent to calling
         * @ref create(UnsignedInt, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector4>&, NodeHandle)
         * with @p indices being a @cpp {0, 1, 1, 2, 2, 3, ..., points.size() - 2, points.size() - 1} @ce
         * range. See its documentation for more information about other
         * arguments.
         * @see @ref createLoop(), @ref setAlignment()
         */
        /* These take Vector4 / Vector3 instead of Color4 / Color3 because
           color views are implicitly convertible to vector views but not the
           other way around */
        DataHandle createStrip(UnsignedInt style, const Containers::StridedArrayView1D<const Vector2>& points, const Containers::StridedArrayView1D<const Vector4>& colors, NodeHandle node =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            NodeHandle::Null
            #else
            NodeHandle{} /* To not have to include Handle.h */
            #endif
        );
        /* There is no Vector3 / Color3 overload because, since the colors are
           optional, it'd be ambiguous to call the function with just {} or
           nullptr, making the most common use case too annoying */
        /** @overload */
        /* This one takes a Color4 instead of Vector4 in order to have e.g.
           0x993366_rgbf implicitly converted to 0x993366ff_rgbaf */
        DataHandle createStrip(UnsignedInt style, std::initializer_list<Vector2> points, std::initializer_list<Color4> colors, NodeHandle node =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            NodeHandle::Null
            #else
            NodeHandle{} /* To not have to include Handle.h */
            #endif
        );

        /**
         * @brief Create a line strip with a style index in a concrete enum type
         *
         * Casts @p style to @relativeref{Magnum,UnsignedInt} and delegates to
         * @ref createStrip(UnsignedInt, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector4>&, NodeHandle).
         */
        template<class StyleIndex
            #ifndef DOXYGEN_GENERATING_OUTPUT
            /* Accept any enum except NodeHandle to prevent create(node, ...)
               from being called by mistake */
            , typename std::enable_if<std::is_enum<StyleIndex>::value && !std::is_same<StyleIndex, NodeHandle>::value, int>::type = 0
            #endif
        > DataHandle createStrip(StyleIndex style, const Containers::StridedArrayView1D<const Vector2>& points, const Containers::StridedArrayView1D<const Vector4>& colors, NodeHandle node =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            NodeHandle::Null
            #else
            NodeHandle{} /* To not have to include Handle.h */
            #endif
        ) {
            return createStrip(UnsignedInt(style), points, colors, node);
        }
        /** @overload */
        template<class StyleIndex
            #ifndef DOXYGEN_GENERATING_OUTPUT
            /* Accept any enum except NodeHandle to prevent create(node, ...)
               from being called by mistake */
            , typename std::enable_if<std::is_enum<StyleIndex>::value && !std::is_same<StyleIndex, NodeHandle>::value, int>::type = 0
            #endif
        > DataHandle createStrip(StyleIndex style, std::initializer_list<Vector2> points, std::initializer_list<Color4> colors, NodeHandle node =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            NodeHandle::Null
            #else
            NodeHandle{} /* To not have to include Handle.h */
            #endif
        ) {
            return createStrip(UnsignedInt(style), points, colors, node);
        }

        /**
         * @brief Create a line loop
         * @param style         Style index
         * @param points        Line loop points, in UI units
         * @param colors        Optional per-point colors
         * @param node          Node to attach to
         * @return New data handle
         *
         * Creates a single line loop with the last point connected to the
         * first. The @p points are expected to be either empty, a single point
         * (which will create a literal point) or at least three. Convenience
         * equivalent to calling @ref create(UnsignedInt, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector4>&, NodeHandle)
         * with @p indices being a @cpp {0, 1, 1, 2, 2, 3, ..., points.size() - 1, 0, @ce
         * range. See its documentation for more information about other
         * arguments.
         * @see @ref createStrip(), @ref setAlignment()
         */
        /* This one takes Vector4 instead of Color4 because color views are
           implicitly convertible to vectors but not the other way around */
        DataHandle createLoop(UnsignedInt style, const Containers::StridedArrayView1D<const Vector2>& points, const Containers::StridedArrayView1D<const Vector4>& colors, NodeHandle node =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            NodeHandle::Null
            #else
            NodeHandle{} /* To not have to include Handle.h */
            #endif
        );
        /* There is no Vector3 / Color3 overload because, since the colors are
           optional, it'd be ambiguous to call the function with just {} or
           nullptr, making the most common use case too annoying */
        /** @overload */
        /* This one takes a Color4 instead of Vector4 in order to have e.g.
           0x993366_rgbf implicitly converted to 0x993366ff_rgbaf */
        DataHandle createLoop(UnsignedInt style, std::initializer_list<Vector2> points, std::initializer_list<Color4> colors, NodeHandle node =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            NodeHandle::Null
            #else
            NodeHandle{} /* To not have to include Handle.h */
            #endif
        );

        /**
         * @brief Create a line loop with a style index in a concrete enum type
         *
         * Casts @p style to @relativeref{Magnum,UnsignedInt} and delegates to
         * @ref createLoop(UnsignedInt, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector4>&, NodeHandle).
         */
        template<class StyleIndex
            #ifndef DOXYGEN_GENERATING_OUTPUT
            /* Accept any enum except NodeHandle to prevent create(node, ...)
               from being called by mistake */
            , typename std::enable_if<std::is_enum<StyleIndex>::value && !std::is_same<StyleIndex, NodeHandle>::value, int>::type = 0
            #endif
        > DataHandle createLoop(StyleIndex style, const Containers::StridedArrayView1D<const Vector2>& points, const Containers::StridedArrayView1D<const Vector4>& colors, NodeHandle node =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            NodeHandle::Null
            #else
            NodeHandle{} /* To not have to include Handle.h */
            #endif
        ) {
            return createLoop(UnsignedInt(style), points, colors, node);
        }
        /** @overload */
        template<class StyleIndex
            #ifndef DOXYGEN_GENERATING_OUTPUT
            /* Accept any enum except NodeHandle to prevent create(node, ...)
               from being called by mistake */
            , typename std::enable_if<std::is_enum<StyleIndex>::value && !std::is_same<StyleIndex, NodeHandle>::value, int>::type = 0
            #endif
        > DataHandle createLoop(StyleIndex style, std::initializer_list<Vector2> points, std::initializer_list<Color4> colors, NodeHandle node =
            #ifdef DOXYGEN_GENERATING_OUTPUT
            NodeHandle::Null
            #else
            NodeHandle{} /* To not have to include Handle.h */
            #endif
        ) {
            return createLoop(UnsignedInt(style), points, colors, node);
        }

        /**
         * @brief Remove a line
         *
         * Delegates to @ref AbstractLayer::remove(DataHandle).
         */
        void remove(DataHandle handle);

        /**
         * @brief Remove a line assuming it belongs to this layer
         *
         * Delegates to @ref AbstractLayer::remove(LayerDataHandle).
         */
        void remove(LayerDataHandle handle);

        /**
         * @brief Line index count
         *
         * Count of indices passed to @ref create() or @ref setLine(). In case
         * of @ref createStrip() / @ref setLineStrip() the count is
         * @cpp 2*pointCount - 2 @ce, in case of @ref createLoop() /
         * @ref setLineLoop() the count is @cpp 2*pointCount @ce. Expects that
         * @p handle is valid.
         * @see @ref isHandleValid(DataHandle) const, @ref pointCount()
         */
        UnsignedInt indexCount(DataHandle handle) const;

        /**
         * @brief Line index count assuming it belongs to this layer
         *
         * Like @ref indexCount(DataHandle) const but without checking that
         * @p handle indeed belongs to this layer. See its documentation for
         * more information.
         */
        UnsignedInt indexCount(LayerDataHandle handle) const;

        /**
         * @brief Line point count
         *
         * Count of points passed to @ref create(), @ref createStrip(),
         * @ref createLoop(), @ref setLine(), @ref setLineStrip() or
         * @ref setLineLoop(). Expects that @p handle is valid.
         * @see @ref isHandleValid(DataHandle) const, @ref indexCount()
         */
        UnsignedInt pointCount(DataHandle handle) const;

        /**
         * @brief Line point count assuming it belongs to this layer
         *
         * Like @ref pointCount(DataHandle) const but without checking that
         * @p handle indeed belongs to this layer. See its documentation for
         * more information.
         */
        UnsignedInt pointCount(LayerDataHandle handle) const;

        /**
         * @brief Set line data
         *
         * Expects that @p handle is valid. The @p indices, @p points and
         * @p colors are interpreted the same way with the same restrictions as
         * in @ref create(UnsignedInt, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector4>&, NodeHandle),
         * see its documentation for more information.
         *
         * Calling this function causes @ref LayerState::NeedsDataUpdate to be
         * set.
         * @see @ref isHandleValid(DataHandle) const, @see @ref setLineStrip(),
         *      @ref setLineLoop()
         */
        /* This one takes Vector4 instead of Color4 because color views are
           implicitly convertible to vectors but not the other way around */
        void setLine(DataHandle handle, const Containers::StridedArrayView1D<const UnsignedInt>& indices, const Containers::StridedArrayView1D<const Vector2>& points, const Containers::StridedArrayView1D<const Vector4>& colors);
        /* There is no Vector3 / Color3 overload because, since the colors are
           optional, it'd be ambiguous to call the function with just {} or
           nullptr, making the most common use case too annoying */
        /** @overload */
        /* This one takes a Color4 instead of Vector4 in order to have e.g.
           0x993366_rgbf implicitly converted to 0x993366ff_rgbaf */
        void setLine(DataHandle handle, std::initializer_list<UnsignedInt> indices, std::initializer_list<Vector2> points, std::initializer_list<Color4> colors);

        /**
         * @brief Set line data assuming it belongs to this layer
         *
         * Like @ref setLine(DataHandle, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector4>&)
         * but without checking that @p handle indeed belongs to this layer.
         * See its documentation for more information.
         */
        void setLine(LayerDataHandle handle, const Containers::StridedArrayView1D<const UnsignedInt>& indices, const Containers::StridedArrayView1D<const Vector2>& points, const Containers::StridedArrayView1D<const Vector4>& colors);
        /** @overload */
        void setLine(LayerDataHandle handle, std::initializer_list<UnsignedInt> indices, std::initializer_list<Vector2> points, std::initializer_list<Color4> colors);

        /**
         * @brief Set line strip data
         *
         * Expects that @p handle is valid. The @p points and @p colors are
         * interpreted the same way with the same restrictions as
         * in @ref createStrip(UnsignedInt, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector4>&, NodeHandle),
         * see its documentation for more information.
         *
         * Calling this function causes @ref LayerState::NeedsDataUpdate to be
         * set.
         * @see @ref isHandleValid(DataHandle) const, @see @ref setLine(),
         *      @ref setLineLoop()
         */
        /* This one takes Vector4 instead of Color4 because color views are
           implicitly convertible to vectors but not the other way around */
        void setLineStrip(DataHandle handle, const Containers::StridedArrayView1D<const Vector2>& points, const Containers::StridedArrayView1D<const Vector4>& colors);
        /* There is no Vector3 / Color3 overload because, since the colors are
           optional, it'd be ambiguous to call the function with just {} or
           nullptr, making the most common use case too annoying */
        /** @overload */
        /* This one takes a Color4 instead of Vector4 in order to have e.g.
           0x993366_rgbf implicitly converted to 0x993366ff_rgbaf */
        void setLineStrip(DataHandle handle, std::initializer_list<Vector2> points, std::initializer_list<Color4> colors);

        /**
         * @brief Set line strip data assuming it belongs to this layer
         *
         * Like @ref setLineStrip(DataHandle, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector4>&)
         * but without checking that @p handle indeed belongs to this layer.
         * See its documentation for more information.
         */
        void setLineStrip(LayerDataHandle handle, const Containers::StridedArrayView1D<const Vector2>& points, const Containers::StridedArrayView1D<const Vector4>& colors);
        /** @overload */
        void setLineStrip(LayerDataHandle handle, std::initializer_list<Vector2> points, std::initializer_list<Color4> colors);

        /**
         * @brief Set line loop data
         *
         * Expects that @p handle is valid. The @p points and @p colors are
         * interpreted the same way with the same restrictions as
         * in @ref createLoop(UnsignedInt, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector4>&, NodeHandle),
         * see its documentation for more information.
         *
         * Calling this function causes @ref LayerState::NeedsDataUpdate to be
         * set.
         * @see @ref isHandleValid(DataHandle) const, @see @ref setLine(),
         *      @ref setLineStrip()
         */
        /* This one takes Vector4 instead of Color4 because color views are
           implicitly convertible to vectors but not the other way around */
        void setLineLoop(DataHandle handle, const Containers::StridedArrayView1D<const Vector2>& points, const Containers::StridedArrayView1D<const Vector4>& colors);
        /* There is no Vector3 / Color3 overload because, since the colors are
           optional, it'd be ambiguous to call the function with just {} or
           nullptr, making the most common use case too annoying */
        /** @overload */
        /* This one takes a Color4 instead of Vector4 in order to have e.g.
           0x993366_rgbf implicitly converted to 0x993366ff_rgbaf */
        void setLineLoop(DataHandle handle, std::initializer_list<Vector2> points, std::initializer_list<Color4> colors);

        /**
         * @brief Set line loop data assuming it belongs to this layer
         *
         * Like @ref setLineLoop(DataHandle, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector4>&)
         * but without checking that @p handle indeed belongs to this layer.
         * See its documentation for more information.
         */
        void setLineLoop(LayerDataHandle handle, const Containers::StridedArrayView1D<const Vector2>& points, const Containers::StridedArrayView1D<const Vector4>& colors);
        /** @overload */
        void setLineLoop(LayerDataHandle handle, std::initializer_list<Vector2> points, std::initializer_list<Color4> colors);

        /**
         * @brief Custom line color
         *
         * Expects that @p handle is valid.
         * @see @ref isHandleValid(DataHandle) const
         */
        Color4 color(DataHandle handle) const;

        /**
         * @brief Custom line color assuming it belongs to this layer
         *
         * Expects that @p handle is valid.
         * @see @ref isHandleValid(LayerDataHandle) const
         */
        Color4 color(LayerDataHandle handle) const;

        /**
         * @brief Set custom line color
         *
         * Expects that @p handle is valid. The @p color is expected to have
         * premultiplied alpha. It is multiplied with
         * @ref LineLayerStyleUniform::color, with per-point colors, if
         * specified in @ref create(), @ref setLine() or overloads, and with
         * node opacity coming from @ref AbstractUserInterface::setNodeOpacity().
         * By default, the custom color is @cpp 0xffffffff_srgbaf @ce, i.e. not
         * affecting the style or per-point colors in any way.
         *
         * Calling this function causes @ref LayerState::NeedsDataUpdate to be
         * set.
         * @see @ref isHandleValid(DataHandle) const,
         *      @ref Color4::premultiplied()
         */
        void setColor(DataHandle handle, const Color4& color);

        /**
         * @brief Set custom line color assuming it belongs to this layer
         *
         * Like @ref setColor(DataHandle, const Color4&) but without checking
         * that @p handle indeed belongs to this layer. See its documentation
         * for more information.
         */
        void setColor(LayerDataHandle handle, const Color4& color);

        /**
         * @brief Custom line alignment
         *
         * Expects that @p handle is valid. If
         * @relativeref{Corrade,Containers::NullOpt}, alignment coming from the
         * style is used.
         * @see @ref isHandleValid(DataHandle) const
         */
        Containers::Optional<LineAlignment> alignment(DataHandle handle) const;

        /**
         * @brief Custom line alignment assuming it belongs to this layer
         *
         * Expects that @p handle is valid. If
         * @relativeref{Corrade,Containers::NullOpt}, alignment coming from the
         * style is used.
         * @see @ref isHandleValid(LayerDataHandle) const
         */
        Containers::Optional<LineAlignment> alignment(LayerDataHandle handle) const;

        /**
         * @brief Set custom line alignment
         *
         * Expects that @p handle is valid. Setting the alignment to
         * @relativeref{Corrade,Containers::NullOpt} makes it use the alignment
         * coming from the style.
         *
         * Calling this function causes @ref LayerState::NeedsDataUpdate to be
         * set.
         * @see @ref isHandleValid(DataHandle) const
         */
        void setAlignment(DataHandle handle, Containers::Optional<LineAlignment> alignment);

        /**
         * @brief Set custom line alignment assuming it belongs to this layer
         *
         * Like @ref setAlignment(DataHandle, Containers::Optional<LineAlignment>)
         * but without checking that @p handle indeed belongs to this layer.
         * See its documentation for more information.
         */
        void setAlignment(LayerDataHandle handle, Containers::Optional<LineAlignment> alignment);

        /**
         * @brief Custom line padding
         *
         * In UI units, in order left, top, right, bottom. Expects that
         * @p handle is valid.
         * @see @ref isHandleValid(DataHandle) const
         */
        Vector4 padding(DataHandle handle) const;

        /**
         * @brief Custom line padding assuming it belongs to this layer
         *
         * Like @ref padding(DataHandle) const but without checking that
         * @p handle indeed belongs to this layer. See its documentation for
         * more information.
         */
        Vector4 padding(LayerDataHandle handle) const;

        /**
         * @brief Set custom line padding
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
         * @brief Set custom line padding assuming it belongs to this layer
         *
         * Like @ref setPadding(DataHandle, const Vector4&) but without
         * checking that @p handle indeed belongs to this layer. See its
         * documentation for more information.
         */
        void setPadding(LayerDataHandle handle, const Vector4& padding);

        /**
         * @brief Set custom line padding with all edges having the same value
         *
         * Equivalent to calling @ref setPadding(DataHandle, const Vector4&)
         * with all four components set to @p padding. See its documentation
         * for more information.
         */
        void setPadding(DataHandle handle, Float padding) {
            setPadding(handle, Vector4{padding});
        }

        /**
         * @brief Set custom line padding with all edges having the same value assuming it belongs to this layer
         *
         * Like @ref setPadding(DataHandle, Float) but without checking that
         * @p handle indeed belongs to this layer. See its documentation for
         * more information.
         */
        void setPadding(LayerDataHandle handle, Float padding) {
            setPadding(handle, Vector4{padding});
        }

    #ifdef DOXYGEN_GENERATING_OUTPUT
    private:
    #else
    protected:
    #endif
        struct State;

        MAGNUM_UI_LOCAL explicit LineLayer(LayerHandle handle, Containers::Pointer<State>&& state);
        /* Used by tests to avoid having to include / allocate the state */
        explicit LineLayer(LayerHandle handle, Shared& shared);

        /* These can't be MAGNUM_UI_LOCAL otherwise deriving from this class
           in tests causes linker errors */

        /* Advertises LayerFeature::Draw but *does not* implement doDraw(),
           that's on the subclass */
        LayerFeatures doFeatures() const override;

        void doUpdate(LayerStates states, const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, const Containers::StridedArrayView1D<const Float>& nodeOpacities, Containers::BitArrayView nodesEnabled, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes, const Containers::StridedArrayView1D<const Vector2>& compositeRectOffsets, const Containers::StridedArrayView1D<const Vector2>& compositeRectSizes) override;

    private:
        /* (Transitively) used by create(), createStrip(), createLoop(),
           setLine(), setLineStrip() and setLineLoop() */
        MAGNUM_UI_LOCAL UnsignedInt createRun(UnsignedInt dataId, UnsignedInt indexCount, UnsignedInt pointCount);
        MAGNUM_UI_LOCAL void fillIndices(const char* messagePrefix, UnsignedInt dataId, const Containers::StridedArrayView1D<const UnsignedInt>& indices);
        MAGNUM_UI_LOCAL void fillStripIndices(const char* messagePrefix, UnsignedInt dataId);
        MAGNUM_UI_LOCAL void fillLoopIndices(const char* messagePrefix, UnsignedInt dataId);
        MAGNUM_UI_LOCAL void fillPoints(const char* messagePrefix, UnsignedInt dataId, const Containers::StridedArrayView1D<const Vector2>& points, const Containers::StridedArrayView1D<const Vector4>& colors);

        /* Used by create(), createLoop() and createStrip() */
        MAGNUM_UI_LOCAL DataHandle createInternal(const char* messagePrefix, UnsignedInt style, UnsignedInt indexCount, UnsignedInt pointCount, NodeHandle node);
        /* Used by remove() */
        MAGNUM_UI_LOCAL void removeInternal(UnsignedInt id);

        MAGNUM_UI_LOCAL void setLineInternal(UnsignedInt id, const Containers::StridedArrayView1D<const UnsignedInt>& indices, const Containers::StridedArrayView1D<const Vector2>& points, const Containers::StridedArrayView1D<const Vector4>& colors);
        MAGNUM_UI_LOCAL void setLineStripInternal(UnsignedInt id, const Containers::StridedArrayView1D<const Vector2>& points, const Containers::StridedArrayView1D<const Vector4>& colors);
        MAGNUM_UI_LOCAL void setLineLoopInternal(UnsignedInt id, const Containers::StridedArrayView1D<const Vector2>& points, const Containers::StridedArrayView1D<const Vector4>& colors);

        MAGNUM_UI_LOCAL void setColorInternal(UnsignedInt id, const Color4& color);
        MAGNUM_UI_LOCAL Containers::Optional<LineAlignment> alignmentInternal(UnsignedInt id) const;
        MAGNUM_UI_LOCAL void setAlignmentInternal(UnsignedInt id, Containers::Optional<LineAlignment> alignment);
        MAGNUM_UI_LOCAL void setPaddingInternal(UnsignedInt id, const Vector4& padding);

        /* Can't be MAGNUM_UI_LOCAL otherwise deriving from this class in
           tests causes linker errors */
        LayerStates doState() const override;
        void doClean(Containers::BitArrayView dataIdsToRemove) override;
};

/**
@brief Shared state for the line layer

Contains style definitions. See the @ref LineLayer class documentation for
information about setting up an instance of this layer and using it.

You'll most likely instantiate the class through @ref LineLayerGL::Shared. In
order to update or draw the layer it's expected that @ref setStyle() was
called.
*/
class MAGNUM_UI_EXPORT LineLayer::Shared: public AbstractVisualLayer::Shared {
    public:
        class Configuration;

        /**
         * @brief Style uniform count
         *
         * Size of the style uniform buffer excluding dynamic styles. May or
         * may not be the same as @ref styleCount().
         * @see @ref Configuration::Configuration(UnsignedInt, UnsignedInt),
         *      @ref setStyle()
         */
        UnsignedInt styleUniformCount() const;

        /** @brief Cap style */
        LineCapStyle capStyle() const;

        /** @brief Join style */
        LineJoinStyle joinStyle() const;

        /**
         * @brief Set style data with implicit mapping between styles and uniforms
         * @param commonUniform Common style uniform data
         * @param uniforms      Style uniforms
         * @param alignments    Line alignment corresponding to style uniforms
         * @param paddings      Padding inside the node in UI units, in order
         *      left, top, right, bottom, corresponding to style uniforms
         * @return Reference to self (for method chaining)
         *
         * The @p uniforms view is expected to have the same size as
         * @ref styleUniformCount(), the @p alignments view the same size as
         * @ref styleCount(). The @p paddings view is expected to either have
         * the same size as @ref styleCount() or be empty, in which case all
         * paddings are implicitly zero.
         *
         * Can only be called if @ref styleUniformCount() and @ref styleCount()
         * were set to the same value in @ref Configuration passed to the
         * constructor, otherwise you have to additionally provide a mapping
         * from styles to uniforms using
         * @ref setStyle(const LineLayerCommonStyleUniform&, Containers::ArrayView<const LineLayerStyleUniform>, const Containers::StridedArrayView1D<const UnsignedInt>&, const Containers::StridedArrayView1D<const LineAlignment>&, const Containers::StridedArrayView1D<const Vector4>&)
         * instead.
         *
         * Calling this function causes @ref LayerState::NeedsDataUpdate to be
         * set on all layers that are constructed using this shared instance.
         */
        Shared& setStyle(const LineLayerCommonStyleUniform& commonUniform, Containers::ArrayView<const LineLayerStyleUniform> uniforms, const Containers::StridedArrayView1D<const LineAlignment>& alignments, const Containers::StridedArrayView1D<const Vector4>& paddings);
        /** @overload */
        Shared& setStyle(const LineLayerCommonStyleUniform& commonUniform, std::initializer_list<LineLayerStyleUniform> uniforms, std::initializer_list<LineAlignment> alignments, std::initializer_list<Vector4> paddings);

        /**
         * @brief Set style data
         * @param commonUniform     Common style uniform data
         * @param uniforms          Style uniforms
         * @param styleToUniform    Style to style uniform mapping
         * @param styleAlignments   Per-style line alignment
         * @param stylePaddings     Per-style padding inside the node in UI
         *      units, in order left, top, right, bottom
         * @return Reference to self (for method chaining)
         *
         * The @p uniforms view is expected to have the same size as
         * @ref styleUniformCount(), the @p styleToUniform and
         * @p styleAlignments views the same size as @ref styleCount(). All
         * uniform indices are expected to be less than
         * @ref styleUniformCount().
         *
         * The @p stylePaddings view is expected to either have the same size
         * as @ref styleCount() or be empty, in which case all paddings are
         * implicitly zero.
         *
         * Value of @cpp styleToUniform[i] @ce should give back an index into
         * the @p uniforms array for style @cpp i @ce. If
         * @ref styleUniformCount() and @ref styleCount() is the same and the
         * mapping is implicit, you can use the
         * @ref setStyle(const LineLayerCommonStyleUniform&, Containers::ArrayView<const LineLayerStyleUniform>, const Containers::StridedArrayView1D<const LineAlignment>&, const Containers::StridedArrayView1D<const Vector4>&)
         * convenience overload instead.
         *
         * Calling this function causes @ref LayerState::NeedsDataUpdate to be
         * set on all layers that are constructed using this shared instance.
         */
        Shared& setStyle(const LineLayerCommonStyleUniform& commonUniform, Containers::ArrayView<const LineLayerStyleUniform> uniforms, const Containers::StridedArrayView1D<const UnsignedInt>& styleToUniform, const Containers::StridedArrayView1D<const LineAlignment>& styleAlignments, const Containers::StridedArrayView1D<const Vector4>& stylePaddings);
        /** @overload */
        Shared& setStyle(const LineLayerCommonStyleUniform& commonUniform, std::initializer_list<LineLayerStyleUniform> uniforms, std::initializer_list<UnsignedInt> styleToUniform, std::initializer_list<LineAlignment> styleAlignments, std::initializer_list<Vector4> stylePaddings);

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
        friend LineLayer;

        MAGNUM_UI_LOCAL explicit Shared(Containers::Pointer<State>&& state);
        /* Used by tests to avoid having to include / allocate the state */
        explicit Shared(const Configuration& configuration);
        /* Can't be MAGNUM_UI_LOCAL, used by tests */
        explicit Shared(NoCreateT) noexcept;

    private:
        /* Dynamic styles are so far not implemented, thus don't even expose
           this to avoid confusion */
        using AbstractVisualLayer::Shared::dynamicStyleCount;

        MAGNUM_UI_LOCAL void setStyleInternal(const LineLayerCommonStyleUniform& commonUniform, Containers::ArrayView<const LineLayerStyleUniform> styleUniforms, const Containers::StridedArrayView1D<const LineAlignment>& styleAlignments, const Containers::StridedArrayView1D<const Vector4>& stylePaddings);

        /* The items are guaranteed to have the same size as
           styleUniformCount(). Called only if there are no dynamic styles,
           otherwise the data are copied to internal arrays to be subsequently
           combined with dynamic uniforms and uploaded together in doDraw(). */
        virtual void doSetStyle(const LineLayerCommonStyleUniform& commonUniform, Containers::ArrayView<const LineLayerStyleUniform> uniforms) = 0;
};

/**
@brief Configuration of a line layer shared state

See the @ref LineLayer class documentation for information about setting up an
instance of this layer and using it.
@see @ref LineLayerGL::Shared::Shared(const Configuration&)
*/
class MAGNUM_UI_EXPORT LineLayer::Shared::Configuration {
    public:
        /**
         * @brief Constructor
         *
         * The @p styleUniformCount parameter specifies the size of the uniform
         * array, @p styleCount then the number of distinct styles to use for
         * drawing. The sizes are independent in order to allow styles with
         * different paddings share the same uniform data. Both
         * @p styleUniformCount and @p styleCount is expected to be non-zero.
         * Style data are then set with @ref setStyle().
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

        /** @brief Cap style */
        LineCapStyle capStyle() const { return _capStyle; }

        /**
         * @brief Set cap style
         *
         * Unlike for example the SVG specification that uses
         * @ref LineCapStyle::Butt by default, the default value is
         * @ref LineCapStyle::Square, in order to make zero-length lines
         * visible.
         * @see @ref LineLayer::Shared::capStyle()
         */
        Configuration& setCapStyle(LineCapStyle style) {
            _capStyle = style;
            return *this;
        }

        /** @brief Join style */
        LineJoinStyle joinStyle() const { return _joinStyle; }

        /**
         * @brief Set join style
         *
         * Default value is @ref LineJoinStyle::Miter, consistently with the
         * SVG specification.
         * @see @ref LineLayer::Shared::joinStyle()
         */
        Configuration& setJoinStyle(LineJoinStyle style) {
            _joinStyle = style;
            return *this;
        }

    private:
        UnsignedInt _styleUniformCount, _styleCount;
        LineCapStyle _capStyle = LineCapStyle::Square;
        LineJoinStyle _joinStyle = LineJoinStyle::Miter;
};

/**
@brief Debug layer integration

Integrates the layer with @ref DebugLayer. See
@ref Ui-LineLayer-debug-integration "LineLayer debug layer integration" for
more information and example usage.
*/
class MAGNUM_UI_EXPORT LineLayer::DebugIntegration: public AbstractVisualLayer::DebugIntegration {
    public:
        /**
         * @brief Constructor
         *
         * Same as the @ref AbstractVisualLayer::DebugIntegration constructor,
         * see its documentation for more information.
         */
        /*implicit*/ DebugIntegration(Containers::StringView(*styleName)(UnsignedInt style) = nullptr): AbstractVisualLayer::DebugIntegration{styleName} {}
        #ifndef DOXYGEN_GENERATING_OUTPUT
        /* Same as the workaround in AbstractVisualLayer::DebugIntegration */
        template<class T, typename std::enable_if<std::is_convertible<T, Containers::StringView(*)(UnsignedInt)>::value, int>::type = 0> /*implicit*/ DebugIntegration(T styleName): AbstractVisualLayer::DebugIntegration{styleName} {}
        #endif

        #ifndef DOXYGEN_GENERATING_OUTPUT
        /* Used internally by DebugLayer, no point in documenting it here */
        void print(Debug& out, const LineLayer& layer, const Containers::StringView& layerName, LayerDataHandle data);
        #endif
};

inline LineLayer::Shared& LineLayer::shared() {
    return static_cast<Shared&>(AbstractVisualLayer::shared());
}

inline const LineLayer::Shared& LineLayer::shared() const {
    return static_cast<const Shared&>(AbstractVisualLayer::shared());
}

}}

#endif
