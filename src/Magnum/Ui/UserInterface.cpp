/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017
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

#include "UserInterface.h"

#include <Corrade/Containers/StaticArray.h>
#include <Magnum/ImageView.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/TextureFormat.h>
#include <Magnum/Text/AbstractFont.h>

#include "Magnum/Ui/Plane.h"
#include "Magnum/Ui/visibility.h"

namespace Magnum { namespace Ui {

namespace {
    enum: std::size_t { IndexCount = 16384 };
}

UserInterface::UserInterface(const Vector2& size, const Vector2i& screenSize, Text::AbstractFont& font, const StyleConfiguration& styleConfiguration):
    BasicUserInterface<Implementation::QuadLayer, Implementation::QuadLayer, Implementation::TextLayer>(size, screenSize),
    _backgroundUniforms{Buffer::TargetHint::Uniform},
    _foregroundUniforms{Buffer::TargetHint::Uniform},
    _textUniforms{Buffer::TargetHint::Uniform},
    _quadVertices{Buffer::TargetHint::Array},
    _quadIndices{Buffer::TargetHint::ElementArray},
    _font(font), _glyphCache{Vector2i{1024}}
{
    /* Prepare quad vertices */
    /** @todo make this a shader constant and use gl_VertexId */
    constexpr const Implementation::QuadVertex vertexData[4]{
        /* 0---2
           |   |
           |   |
           |   |
           1---3 */
        {{0.0f, 1.0f}, {0.0f, 1.0f, 1.0f, 0.0f}},
        {{0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
        {{1.0f, 1.0f}, {1.0f, 0.0f, 1.0f, 0.0f}},
        {{1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}}};
    _quadVertices.setData(vertexData, BufferUsage::StaticDraw);

    /* Prepare quad indices */
    _quadIndices.setData({nullptr, IndexCount*6*sizeof(UnsignedShort)}, BufferUsage::StaticDraw);
    Containers::ArrayView<UnsignedShort> data{_quadIndices.map<UnsignedShort>(0, IndexCount*6*sizeof(UnsignedShort), Buffer::MapFlag::Write|Buffer::MapFlag::InvalidateBuffer), IndexCount*6};

    for(std::size_t i = 0; i != IndexCount; ++i) {
        /* 0---2 0---2 5
           |   | |  / /|
           |   | | / / |
           |   | |/ /  |
           1---3 1 3---4 */
        data[i*6 + 0] = i*4 + 0;
        data[i*6 + 1] = i*4 + 1;
        data[i*6 + 2] = i*4 + 2;
        data[i*6 + 3] = i*4 + 1;
        data[i*6 + 4] = i*4 + 3;
        data[i*6 + 5] = i*4 + 2;
    }

    _quadIndices.unmap();

    /* Prepare corner texture */
    Containers::StaticArray<32*32, UnsignedByte> corner;
    for(std::size_t y = 0; y != 32; ++y)
        for(std::size_t x = 0; x != 32; ++x)
            corner[y*32 + x] = 255*Math::max(0.0f, 1.0f - Vector2(x, y).length()/31.0f);
    _corner
        .setMinificationFilter(Sampler::Filter::Linear)
        .setMagnificationFilter(Sampler::Filter::Linear)
        .setWrapping(Sampler::Wrapping::ClampToEdge)
        .setStorage(1, TextureFormat::R8, {32, 32})
        .setSubImage(0, {}, ImageView2D{PixelFormat::Red, PixelType::UnsignedByte, {32, 32}, Containers::ArrayView<const void>(corner)});

    /* Set default style */
    setStyleConfiguration(styleConfiguration);

    /* Prepare glyph cache */
    _font.fillGlyphCache(_glyphCache, "abcdefghijklmnopqrstuvwxyz"
                                      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                      "0123456789 _.,-+=*:;?!@$&#/\\|`\"'<>()[]{}%…");
}

UserInterface::UserInterface(const Vector2& size, const Vector2i& screenSize, Text::AbstractFont& font): UserInterface{size, screenSize, font, defaultStyleConfiguration()} {}

UserInterface::~UserInterface() = default;

const Plane* UserInterface::activePlane() const {
    return static_cast<const Plane*>(BasicUserInterface::activePlane());
}

Plane* UserInterface::activePlane() {
    return static_cast<Plane*>(BasicUserInterface::activePlane());
}

void UserInterface::setStyleConfiguration(const StyleConfiguration& configuration) {
    _styleConfiguration = configuration;
    configuration.pack(_backgroundUniforms, _foregroundUniforms, _textUniforms);
}

void UserInterface::draw() {
    update();

    _backgroundShader
        .bindCornerTexture(_corner)
        .bindStyleBuffer(_backgroundUniforms);
    _foregroundShader
        .bindCornerTexture(_corner)
        .bindStyleBuffer(_foregroundUniforms);
    _textShader
        .bindGlyphCacheTexture(_glyphCache.texture())
        .bindStyleBuffer(_textUniforms);

    BasicUserInterface::draw({{_backgroundShader, _foregroundShader, _textShader}});
}

}}
