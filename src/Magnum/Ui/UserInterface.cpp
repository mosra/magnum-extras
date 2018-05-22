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

#include "UserInterface.h"

#include <Corrade/Containers/StaticArray.h>
#include <Corrade/PluginManager/Manager.h>
#include <Magnum/ImageView.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/Text/AbstractFont.h>
#include <Magnum/Text/GlyphCache.h>

#include "Magnum/Ui/Plane.h"
#include "Magnum/Ui/visibility.h"

namespace Magnum { namespace Ui {

namespace {
    enum: std::size_t { IndexCount = 16384 };
}

UserInterface::UserInterface(NoCreateT, const Vector2& size, const Vector2i& screenSize):
    BasicUserInterface<Implementation::QuadLayer, Implementation::QuadLayer, Implementation::TextLayer>(size, screenSize),
    _backgroundUniforms{GL::Buffer::TargetHint::Uniform},
    _foregroundUniforms{GL::Buffer::TargetHint::Uniform},
    _textUniforms{GL::Buffer::TargetHint::Uniform},
    _quadVertices{GL::Buffer::TargetHint::Array},
    _quadIndices{GL::Buffer::TargetHint::ElementArray}
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
    _quadVertices.setData(vertexData, GL::BufferUsage::StaticDraw);

    /* Prepare quad indices */
    Containers::Array<UnsignedShort> data{Containers::NoInit, IndexCount*6};
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
    _quadIndices.setData(data, GL::BufferUsage::StaticDraw);

    /* Prepare corner texture */
    Containers::StaticArray<32*32, UnsignedByte> corner;
    for(std::size_t y = 0; y != 32; ++y)
        for(std::size_t x = 0; x != 32; ++x)
            corner[y*32 + x] = 255*Math::max(0.0f, 1.0f - Vector2(x, y).length()/31.0f);
    _corner
        .setMinificationFilter(GL::SamplerFilter::Linear)
        .setMagnificationFilter(GL::SamplerFilter::Linear)
        .setWrapping(GL::SamplerWrapping::ClampToEdge)
        .setStorage(1, GL::TextureFormat::R8, {32, 32})
        .setSubImage(0, {}, ImageView2D{PixelFormat::R8Unorm, {32, 32}, Containers::ArrayView<const void>(corner)});
}

struct UserInterface::FontState {
    explicit FontState(const Vector2i& glyphCacheSize): glyphCache{glyphCacheSize} {}

    PluginManager::Manager<Text::AbstractFont> manager;
    std::unique_ptr<Text::AbstractFont> font;
    Text::GlyphCache glyphCache;
};

UserInterface::UserInterface(const Vector2& size, const Vector2i& screenSize, const StyleConfiguration& styleConfiguration, const std::string& extraGlyphs): UserInterface{NoCreate, size, screenSize} {
    /* Load TTF font plugin */
    _fontState.reset(new UserInterface::FontState{Vector2i{1024}});
    if(!(_fontState->font = _fontState->manager.loadAndInstantiate("TrueTypeFont")))
        std::exit(1);

    /* Make the font and glyph cache pointers public */
    _font = _fontState->font.get();
    _glyphCache = &_fontState->glyphCache;

    /* Open the font */
    if(!_font->openSingleData(Utility::Resource{"MagnumUi"}.getRaw("SourceSansPro-Regular.ttf"),
        32.0f*size.x()/screenSize.x()))
        std::exit(1);

    /* Prepare glyph cache */
    _font->fillGlyphCache(*_glyphCache,
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789 _.,-+=*:;?!@$&#/\\|`\"'<>()[]{}%…" + extraGlyphs);

    /* Set default style */
    setStyleConfiguration(styleConfiguration);
}

UserInterface::UserInterface(const Vector2& size, const Vector2i& screenSize, const std::string& extraGlyphs): UserInterface{size, screenSize, defaultStyleConfiguration(), extraGlyphs} {}

UserInterface::UserInterface(const Vector2& size, const Vector2i& screenSize, Text::AbstractFont& font, Text::GlyphCache& glyphCache, const StyleConfiguration& styleConfiguration): UserInterface{NoCreate, size, screenSize} {
    _font = &font;
    _glyphCache = &glyphCache;
    setStyleConfiguration(styleConfiguration);
}

#ifdef MAGNUM_BUILD_DEPRECATED
UserInterface::UserInterface(const Vector2& size, const Vector2i& screenSize, Text::AbstractFont& font, const StyleConfiguration& styleConfiguration): UserInterface{NoCreate, size, screenSize} {
    /* Populate the state only for the glyph cache */
    _fontState.reset(new UserInterface::FontState{Vector2i{1024}});
    _font = &font;
    _glyphCache = &_fontState->glyphCache;

    /* Prepare glyph cache */
    _font->fillGlyphCache(*_glyphCache,
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789 _.,-+=*:;?!@$&#/\\|`\"'<>()[]{}%…");

    /* Set desired style */
    setStyleConfiguration(styleConfiguration);
}

CORRADE_IGNORE_DEPRECATED_PUSH
UserInterface::UserInterface(const Vector2& size, const Vector2i& screenSize, Text::AbstractFont& font): UserInterface{size, screenSize, font, defaultStyleConfiguration()} {}
CORRADE_IGNORE_DEPRECATED_POP
#endif

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
        .bindGlyphCacheTexture(_glyphCache->texture())
        .bindStyleBuffer(_textUniforms);

    BasicUserInterface::draw({{_backgroundShader, _foregroundShader, _textShader}});
}

}}
