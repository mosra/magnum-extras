/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023 Vladimír Vondruš <mosra@centrum.cz>

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

#include "Plane.h"

#include <Magnum/Text/Renderer.h>
#include <Magnum/Text/GlyphCache.h>

#include "Magnum/Ui/BasicPlane.hpp"
#include "Magnum/Ui/UserInterface.h"

namespace Magnum { namespace Ui {

Plane::Plane(UserInterface& ui, const Anchor& anchor): BasicPlane<Implementation::QuadLayer, Implementation::QuadLayer, Implementation::TextLayer>{
    ui,
    anchor,
    {ui.styleConfiguration().padding(), -ui.styleConfiguration().padding()},
    ui.styleConfiguration().margin(),
    _backgroundLayer,
    _foregroundLayer,
    _textLayer}
{
    /** @todo ugh Containers::reference()? How about creference()? */
    for(Implementation::QuadLayer& quadLayer: {Containers::Reference<Implementation::QuadLayer>{_backgroundLayer},
                                               Containers::Reference<Implementation::QuadLayer>{_foregroundLayer}}) {
        quadLayer.mesh()
            .setPrimitive(GL::MeshPrimitive::TriangleStrip)
            .setCount(4)
            .addVertexBuffer(ui._quadVertices, 0,
                Implementation::AbstractQuadShader::Position{},
                Implementation::AbstractQuadShader::EdgeDistance{})
            .addVertexBufferInstanced(quadLayer.buffer(), 1, 0,
                Implementation::AbstractQuadShader::Rect{},
                Implementation::AbstractQuadShader::ColorIndex{Implementation::AbstractQuadShader::ColorIndex::DataType::Short},
                2);
    }

    _textLayer.mesh()
        .setIndexBuffer(ui._quadIndices, 0, GL::MeshIndexType::UnsignedShort)
        .addVertexBuffer(_textLayer.buffer(), 0,
            Implementation::TextShader::Position{},
            Implementation::TextShader::TextureCoordinates{},
            Implementation::TextShader::ColorIndex{Implementation::TextShader::ColorIndex::DataType::Short},
            2);
}

Plane::~Plane() = default;

UserInterface& Plane::ui() {
    return static_cast<UserInterface&>(BasicPlane::ui());
}

const UserInterface& Plane::ui() const {
    return static_cast<const UserInterface&>(BasicPlane::ui());
}

void Plane::reset(const std::size_t backgroundCapacity, const std::size_t foregroundCapacity, const std::size_t textCapacity) {
    _backgroundLayer.reset(4*backgroundCapacity, GL::BufferUsage::StaticDraw);
    _foregroundLayer.reset(4*foregroundCapacity, GL::BufferUsage::StaticDraw);
    _textLayer.reset(foregroundCapacity, 4*textCapacity, GL::BufferUsage::StaticDraw);
}

std::size_t Plane::addText(const UnsignedByte colorIndex, const Float size, const Containers::ArrayView<const char> text, const Vector2& cursor, const Text::Alignment alignment, const std::size_t capacity) {
    /* Render the text */
    /** @todo oh god so many allocations */
    std::vector<Vector2> positions;
    std::vector<Vector2> textureCoordinates;
    Range2D rect;
    std::tie(positions, textureCoordinates, std::ignore, rect) = Text::AbstractRenderer::render(
        *ui()._font, *ui()._glyphCache, size, std::string{text, text.size()}, alignment);
    for(Vector2& position: positions) position += cursor;

    CORRADE_ASSERT(!capacity || capacity*4 >= positions.size(),
        "Ui::Plane::addText(): capacity too small for provided string, got" << positions.size() << "but expected at most" << capacity*4, 0);

    /* Add the element vertex data */
    Containers::Array<Implementation::TextVertex> vertices{Math::max(capacity*4, positions.size())};
    for(std::size_t i = 0; i != positions.size(); ++i) vertices[i] = Implementation::TextVertex{
        positions[i],
        textureCoordinates[i],
        colorIndex};
    return _textLayer.addElement(vertices, vertices.size()*6/4);
}

void Plane::setText(const std::size_t id, const UnsignedByte colorIndex, const Float size, const Containers::ArrayView<const char> text, const Vector2& cursor, const Text::Alignment alignment) {
    /* Render the text */
    /** @todo oh god so many allocations */
    std::vector<Vector2> positions;
    std::vector<Vector2> textureCoordinates;
    Range2D rect;
    std::tie(positions, textureCoordinates, std::ignore, rect) = Text::AbstractRenderer::render(
        *ui()._font, *ui()._glyphCache, size, std::string{text, text.size()}, alignment);
    for(Vector2& position: positions) position += cursor;

    Containers::ArrayView<Implementation::TextVertex> vertices = _textLayer.modifyElement(id);

    CORRADE_ASSERT(vertices.size() >= positions.size(),
        "Ui::Plane::setText(): capacity too small for provided string, got" << positions.size()/4 << "but expected at most" << vertices.size()/4, );

    /* Update vertex data */
    for(std::size_t i = 0; i != positions.size(); ++i) vertices[i] = Implementation::TextVertex{
        positions[i],
        textureCoordinates[i],
        colorIndex};

    /* Clear the rest */
    for(std::size_t i = positions.size(); i != vertices.size(); ++i) vertices[i] = {};
}

}}
