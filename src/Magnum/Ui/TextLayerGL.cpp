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

#include "TextLayerGL.h"

#include <Corrade/Containers/BitArrayView.h>
#include <Corrade/Containers/Iterable.h>
#include <Corrade/Containers/String.h>
#include <Corrade/Utility/Format.h>
#include <Corrade/Utility/Resource.h>
#include <Magnum/Math/Range.h>
#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/Context.h>
#ifndef MAGNUM_TARGET_GLES
#include <Magnum/GL/Extensions.h>
#endif
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/GL/TextureArray.h>
#include <Magnum/GL/Version.h>
#include <Magnum/Text/DistanceFieldGlyphCacheGL.h>

#include "Magnum/Ui/Implementation/textLayerState.h"

#ifdef MAGNUM_UI_BUILD_STATIC
static void importShaderResources() {
    CORRADE_RESOURCE_INITIALIZE(MagnumUi_RESOURCES)
}
#endif

namespace Magnum { namespace Ui {

using namespace Containers::Literals;

namespace {

class TextShaderGL: public GL::AbstractShaderProgram {
    private:
        enum: Int {
            GlyphTextureBinding = 0,
            StyleBufferBinding = 0
        };

    public:
        enum Flag: UnsignedByte {
            DistanceField = 1 << 0
        };

        typedef Containers::EnumSet<Flag> Flags;

        typedef GL::Attribute<0, Vector2> Position;
        typedef GL::Attribute<1, Vector3> TextureCoordinates;
        typedef GL::Attribute<2, Vector4> Color4;
        typedef GL::Attribute<3, UnsignedInt> Style;
        typedef GL::Attribute<4, Float> Scale;

        explicit TextShaderGL(Flags flags, UnsignedInt styleCount);

        TextShaderGL& setProjection(const Vector2& scaling, const Float pixelScaling, const Float distanceFieldScaling) {
            /* XY is Y-flipped scale from the UI size to the 2x2 unit square,
               the shader then translates by (-1, 1) on its own to put the
               origin at center. Z and W is the distance field value delta
               corresponding to, when multiplied with per-text-run scale, one
               framebuffer pixel and one UI unit. */
            setUniform(_projectionUniform, Vector4{2.0f/scaling.x(), -2.0f/scaling.y(), pixelScaling*distanceFieldScaling, distanceFieldScaling});
            return *this;
        }

        TextShaderGL& bindGlyphTexture(GL::Texture2DArray& texture) {
            texture.bind(GlyphTextureBinding);
            return *this;
        }

        TextShaderGL& bindStyleBuffer(GL::Buffer& buffer) {
            buffer.bind(GL::Buffer::Target::Uniform, StyleBufferBinding);
            return *this;
        }

    private:
        Int _projectionUniform = 0;
};

#ifdef CORRADE_TARGET_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#endif
CORRADE_ENUMSET_OPERATORS(TextShaderGL::Flags)
#ifdef CORRADE_TARGET_CLANG
#pragma clang diagnostic pop
#endif

TextShaderGL::TextShaderGL(const Flags flags, const UnsignedInt styleCount) {
    GL::Context& context = GL::Context::current();
    #ifndef MAGNUM_TARGET_GLES
    MAGNUM_ASSERT_GL_EXTENSION_SUPPORTED(GL::Extensions::ARB::explicit_attrib_location);
    #endif

    #ifdef MAGNUM_UI_BUILD_STATIC
    if(!Utility::Resource::hasGroup("MagnumUi"_s))
        importShaderResources();
    #endif

    Utility::Resource rs{"MagnumUi"_s};

    const GL::Version version = context.supportedVersion({
        #ifndef MAGNUM_TARGET_GLES
        GL::Version::GL330
        #else
        GL::Version::GLES300
            #ifndef MAGNUM_TARGET_WEBGL
            , GL::Version::GLES310
            #endif
        #endif
    });

    GL::Shader vert{version, GL::Shader::Type::Vertex};
    vert.addSource(Utility::format("#define STYLE_COUNT {}\n", styleCount))
        .addSource(flags >= Flag::DistanceField ? "#define DISTANCE_FIELD\n"_s : ""_s)
        .addSource(rs.getString("compatibility.glsl"_s))
        .addSource(rs.getString("TextShader.vert"_s));

    GL::Shader frag{version, GL::Shader::Type::Fragment};
    frag.addSource(Utility::format("#define STYLE_COUNT {}\n", styleCount))
        .addSource(flags >= Flag::DistanceField ? "#define DISTANCE_FIELD\n"_s : ""_s)
        .addSource(rs.getString("compatibility.glsl"_s))
        .addSource(rs.getString("TextShader.frag"_s));

    CORRADE_INTERNAL_ASSERT(vert.compile() && frag.compile());

    attachShaders({vert, frag});
    CORRADE_INTERNAL_ASSERT(link());

    #ifndef MAGNUM_TARGET_GLES
    if(!context.isExtensionSupported<GL::Extensions::ARB::explicit_uniform_location>())
    #elif !defined(MAGNUM_TARGET_GLES2) && !defined(MAGNUM_TARGET_WEBGL)
    if(version < GL::Version::GLES310)
    #endif
    {
        _projectionUniform = uniformLocation("projection"_s);
    }

    #ifndef MAGNUM_TARGET_GLES
    if(!context.isExtensionSupported<GL::Extensions::ARB::shading_language_420pack>())
    #elif !defined(MAGNUM_TARGET_GLES2) && !defined(MAGNUM_TARGET_WEBGL)
    if(version < GL::Version::GLES310)
    #endif
    {
        setUniform(uniformLocation("glyphTextureData"_s), GlyphTextureBinding);
        setUniformBlockBinding(uniformBlockIndex("Style"_s), StyleBufferBinding);
    }
}

class TextEditingShaderGL: public GL::AbstractShaderProgram {
    private:
        enum: Int {
            /* The base shader uses binding 0, make it possible to bind both at
               the same time */
            StyleBufferBinding = 1
        };

    public:
        typedef GL::Attribute<0, Vector2> Position;
        typedef GL::Attribute<1, Vector2> CenterDistance;
        typedef GL::Attribute<2, Float> Opacity;
        typedef GL::Attribute<3, UnsignedInt> Style;

        explicit TextEditingShaderGL(NoCreateT): GL::AbstractShaderProgram{NoCreate} {}
        explicit TextEditingShaderGL(UnsignedInt styleCount);

        TextEditingShaderGL& setProjection(const Vector2& scaling, const Float pixelScaling) {
            /* XY is Y-flipped scale from the UI size to the 2x2 unit square,
               the shader then translates by (-1, 1) on its own to put the
               origin at center. Z is multiplied with the pixel smoothness
               value to get the smoothness in actual UI units. */
            setUniform(_projectionUniform, Vector3{Vector2{2.0f, -2.0f}/scaling, pixelScaling});
            return *this;
        }

        TextEditingShaderGL& bindStyleBuffer(GL::Buffer& buffer) {
            buffer.bind(GL::Buffer::Target::Uniform, StyleBufferBinding);
            return *this;
        }

    private:
        Int _projectionUniform = 0;
};

TextEditingShaderGL::TextEditingShaderGL(const UnsignedInt styleCount) {
    GL::Context& context = GL::Context::current();
    #ifndef MAGNUM_TARGET_GLES
    MAGNUM_ASSERT_GL_EXTENSION_SUPPORTED(GL::Extensions::ARB::explicit_attrib_location);
    #endif

    #ifdef MAGNUM_UI_BUILD_STATIC
    if(!Utility::Resource::hasGroup("MagnumUi"_s))
        importShaderResources();
    #endif

    Utility::Resource rs{"MagnumUi"_s};

    const GL::Version version = context.supportedVersion({
        #ifndef MAGNUM_TARGET_GLES
        GL::Version::GL330
        #else
        GL::Version::GLES300
            #ifndef MAGNUM_TARGET_WEBGL
            , GL::Version::GLES310
            #endif
        #endif
    });

    GL::Shader vert{version, GL::Shader::Type::Vertex};
    vert.addSource(Utility::format("#define STYLE_COUNT {}\n", styleCount))
        .addSource(rs.getString("compatibility.glsl"_s))
        .addSource(rs.getString("TextEditingShader.vert"_s));

    GL::Shader frag{version, GL::Shader::Type::Fragment};
    frag.addSource(Utility::format("#define STYLE_COUNT {}\n", styleCount))
        .addSource(rs.getString("compatibility.glsl"_s))
        .addSource(rs.getString("TextEditingShader.frag"_s));

    CORRADE_INTERNAL_ASSERT(vert.compile() && frag.compile());

    attachShaders({vert, frag});
    CORRADE_INTERNAL_ASSERT(link());

    #ifndef MAGNUM_TARGET_GLES
    if(!context.isExtensionSupported<GL::Extensions::ARB::explicit_uniform_location>())
    #elif !defined(MAGNUM_TARGET_GLES2) && !defined(MAGNUM_TARGET_WEBGL)
    if(version < GL::Version::GLES310)
    #endif
    {
        _projectionUniform = uniformLocation("projection"_s);
    }

    #ifndef MAGNUM_TARGET_GLES
    if(!context.isExtensionSupported<GL::Extensions::ARB::shading_language_420pack>())
    #elif !defined(MAGNUM_TARGET_GLES2) && !defined(MAGNUM_TARGET_WEBGL)
    if(version < GL::Version::GLES310)
    #endif
    {
        setUniformBlockBinding(uniformBlockIndex("Style"_s), StyleBufferBinding);
    }
}

}

struct TextLayerGL::Shared::State: TextLayer::Shared::State {
    /* Delegated to from the four variants below */
    explicit State(Shared& self, Text::AbstractGlyphCache& glyphCache, const Configuration& configuration);

    explicit State(Shared& self, Text::GlyphCacheArrayGL& glyphCache, const Configuration& configuration);
    explicit State(Shared& self, Text::DistanceFieldGlyphCacheArrayGL& glyphCache, const Configuration& configuration);

    explicit State(Shared& self, Text::GlyphCacheArrayGL&& glyphCache, const Configuration& configuration);
    explicit State(Shared& self, Text::DistanceFieldGlyphCacheArrayGL&& glyphCache, const Configuration& configuration);

    /* Never used directly, only owns the instance passed to the
       Shared(*GlyphCacheGL&&) constructor if it got called instead of
       Shared(*GlyphCacheGL&). The actual used glyph cache reference is in the
       base state struct. */
    Containers::Optional<Text::GlyphCacheArrayGL> glyphCacheStorage;
    Containers::Optional<Text::DistanceFieldGlyphCacheArrayGL> distanceFieldGlyphCacheStorage;
    TextShaderGL shader;
    /* Used only if editingStyleCount is non-zero */
    TextEditingShaderGL editingShader{NoCreate};
    /* In case dynamic styles are present, these buffers are unused and each
       layer has its own copies instead */
    GL::Buffer styleBuffer{NoCreate};
    GL::Buffer editingStyleBuffer{NoCreate};
    /* If not zero, a distance field glyph cache is used and this describes a
       distance field value delta that corresponds to one UI unit if a glyph is
       rendered at exactly the size in UI units it has pixels in the cache.
       E.g., if a distance field radius is 16 pixels, the distance between
       distance field value 0 and 1 is 32 pixels, and this value is thus
       1/32. */
    Float distanceFieldScaling = 0.0f;
};

TextLayerGL::Shared::State::State(Shared& self, Text::AbstractGlyphCache& glyphCache, const Configuration& configuration):
    TextLayer::Shared::State{self, glyphCache, configuration},
    shader{
        configuration.flags() >= TextLayerSharedFlag::DistanceField ? TextShaderGL::Flag::DistanceField : TextShaderGL::Flags{},
        /* If dynamic editing styles are enabled, there's two extra styles for
           each dynamic style, one reserved for under-cursor text and one for
           selected text. If there are no dynamic styles, the editing styles
           pick those from the regular styleUniformCount range. */
        configuration.styleUniformCount() + configuration.dynamicStyleCount()*(configuration.hasEditingStyles() ? 3 : 1)}
{
    if(!dynamicStyleCount) {
        styleBuffer = GL::Buffer{GL::Buffer::TargetHint::Uniform, {nullptr, sizeof(TextLayerCommonStyleUniform) + sizeof(TextLayerStyleUniform)*styleUniformCount}};
        editingStyleBuffer = GL::Buffer{GL::Buffer::TargetHint::Uniform, {nullptr, sizeof(TextLayerCommonEditingStyleUniform) + sizeof(TextLayerEditingStyleUniform)*editingStyleUniformCount}};
    }
    if(hasEditingStyles)
        /* Each dynamic style has two associated editing styles, one for cursor
           and one for selection */
        editingShader = TextEditingShaderGL{configuration.editingStyleUniformCount() + 2*configuration.dynamicStyleCount()};
}

TextLayerGL::Shared::State::State(Shared& self, Text::GlyphCacheArrayGL& glyphCache, const Configuration& configuration): State{self, static_cast<Text::AbstractGlyphCache&>(glyphCache), configuration} {
    CORRADE_ASSERT(!(flags >= TextLayerSharedFlag::DistanceField),
        "Ui::TextLayerGL::Shared:" << TextLayerSharedFlag::DistanceField << "cannot be used with a non-distance-field glyph cache", );
}

/* The base State struct contains an AbstractGlyphCache reference that cannot
   be rebound later, which means that here, in order to make it point to the
   yet-to-be-constructed instance, we have to do a nasty cast of the address of
   the Optional member, to which the glyph cache will be subsequently
   move-constructed. It assumes that the Optional contains the data at offset 0
   in the class, which holds right now, and likely will going forward, since it
   isn't some 3rd party container like std::optional. But still, nasty. */
TextLayerGL::Shared::State::State(Shared& self, Text::GlyphCacheArrayGL&& glyphCache, const Configuration& configuration): State{self, *reinterpret_cast<Text::AbstractGlyphCache*>(&glyphCacheStorage), configuration} {
    glyphCacheStorage = Utility::move(glyphCache);
}

TextLayerGL::Shared::State::State(Shared& self, Text::DistanceFieldGlyphCacheArrayGL& glyphCache, const Configuration& configuration): State{self,
    static_cast<Text::AbstractGlyphCache&>(glyphCache),
    /* Implicitly add the DistanceField flag, the delegated-to constructor then
       uses it to correctly set up the shader and everything else. MSVC 2015
       and 2017 interpret Configuration{} as "cast configuration to a const&"
       or some such, have to use Configuration() to make an actual mutable
       copy. */
    Configuration(configuration)
        .addFlags(TextLayerSharedFlag::DistanceField)
} {
    /* Make sure this is in sync with the overload below */
    distanceFieldScaling = 0.5f/glyphCache.radius();
}

/* Same as the nasty reinterpret_cast code above, just with
   DistanceFieldGlyphCacheArrayGL instead of GlyphCacheArrayGL which is stored
   in the other Optional member. Cannot delegate to the
   Text::DistanceFieldGlyphCacheArrayGL& overload above because that one
   accesses the passed glyph cache instance, which isn't move-constructed yet,
   and thus have to replicate all the flag-adding and scaling calculation logic
   here. */
/** @todo ehh, if one more "make sure this is in sync" thing gets added to both
    of these, maybe reconsider this whole thing and do it differently */
TextLayerGL::Shared::State::State(Shared& self, Text::DistanceFieldGlyphCacheArrayGL&& glyphCache, const Configuration& configuration): State{
    self, *reinterpret_cast<Text::AbstractGlyphCache*>(&distanceFieldGlyphCacheStorage),
    /* Implicitly add the DistanceField flag, the delegated-to constructor then
       uses it to correctly set up the shader and everything else. MSVC 2015
       and 2017 interpret Configuration{} as "cast configuration to a const&"
       or some such, have to use Configuration() to make an actual mutable
       copy. */
    Configuration(configuration)
        .addFlags(TextLayerSharedFlag::DistanceField)
} {
    /* Make sure this is in sync with the overload above */
    distanceFieldScaling = 0.5f/glyphCache.radius();
    distanceFieldGlyphCacheStorage = Utility::move(glyphCache);
}

TextLayerGL::Shared::Shared(Text::GlyphCacheArrayGL& glyphCache, const Configuration& configuration): TextLayer::Shared{Containers::pointer<State>(*this, glyphCache, configuration)} {}

TextLayerGL::Shared::Shared(Text::GlyphCacheArrayGL&& glyphCache, const Configuration& configuration): TextLayer::Shared{Containers::pointer<State>(*this, Utility::move(glyphCache), configuration)} {}

TextLayerGL::Shared::Shared(Text::DistanceFieldGlyphCacheArrayGL& glyphCache, const Configuration& configuration): TextLayer::Shared{Containers::pointer<State>(*this, glyphCache, configuration)} {}

TextLayerGL::Shared::Shared(Text::DistanceFieldGlyphCacheArrayGL&& glyphCache, const Configuration& configuration): TextLayer::Shared{Containers::pointer<State>(*this, Utility::move(glyphCache), configuration)} {}

TextLayerGL::Shared::Shared(NoCreateT) noexcept: TextLayer::Shared{NoCreate} {}

TextLayerGL::Shared& TextLayerGL::Shared::setStyle(const TextLayerCommonStyleUniform& commonUniform, const Containers::ArrayView<const TextLayerStyleUniform> uniforms, const Containers::StridedArrayView1D<const FontHandle>& fonts, const Containers::StridedArrayView1D<const Text::Alignment>& alignments, const Containers::ArrayView<const TextFeatureValue> features, const Containers::StridedArrayView1D<const UnsignedInt>& featureOffsets, const Containers::StridedArrayView1D<const UnsignedInt>& featureCounts, const Containers::StridedArrayView1D<const Int>& cursorStyles, const Containers::StridedArrayView1D<const Int>& selectionStyles, const Containers::StridedArrayView1D<const Vector4>& paddings) {
    return static_cast<Shared&>(TextLayer::Shared::setStyle(commonUniform, uniforms, fonts, alignments, features, featureOffsets, featureCounts, cursorStyles, selectionStyles, paddings));
}

TextLayerGL::Shared& TextLayerGL::Shared::setStyle(const TextLayerCommonStyleUniform& commonUniform, const std::initializer_list<TextLayerStyleUniform> uniforms, const std::initializer_list<FontHandle> fonts, const std::initializer_list<Text::Alignment> alignments, const std::initializer_list<TextFeatureValue> features, const std::initializer_list<UnsignedInt> featureOffsets, const std::initializer_list<UnsignedInt> featureCounts, const std::initializer_list<Int> cursorStyles, const std::initializer_list<Int> selectionStyles, const std::initializer_list<Vector4> paddings) {
    return static_cast<Shared&>(TextLayer::Shared::setStyle(commonUniform, uniforms, fonts, alignments, features, featureOffsets, featureCounts, cursorStyles, selectionStyles, paddings));
}

TextLayerGL::Shared& TextLayerGL::Shared::setStyle(const TextLayerCommonStyleUniform& commonUniform, const Containers::ArrayView<const TextLayerStyleUniform> uniforms, const Containers::StridedArrayView1D<const UnsignedInt>& styleToUniform, const Containers::StridedArrayView1D<const FontHandle>& styleFonts, const Containers::StridedArrayView1D<const Text::Alignment>& styleAlignments, const Containers::ArrayView<const TextFeatureValue> styleFeatures, const Containers::StridedArrayView1D<const UnsignedInt>& styleFeatureOffsets, const Containers::StridedArrayView1D<const UnsignedInt>& styleFeatureCounts, const Containers::StridedArrayView1D<const Int>& styleCursorStyles, const Containers::StridedArrayView1D<const Int>& styleSelectionStyles, const Containers::StridedArrayView1D<const Vector4>& stylePaddings) {
    return static_cast<Shared&>(TextLayer::Shared::setStyle(commonUniform, uniforms, styleToUniform, styleFonts, styleAlignments, styleFeatures, styleFeatureOffsets, styleFeatureCounts, styleCursorStyles, styleSelectionStyles, stylePaddings));
}

TextLayerGL::Shared& TextLayerGL::Shared::setStyle(const TextLayerCommonStyleUniform& commonUniform, const std::initializer_list<TextLayerStyleUniform> uniforms, const std::initializer_list<UnsignedInt> styleToUniform, const std::initializer_list<FontHandle> styleFonts, const std::initializer_list<Text::Alignment> styleAlignments, const std::initializer_list<TextFeatureValue> styleFeatures, const std::initializer_list<UnsignedInt> styleFeatureOffsets, const std::initializer_list<UnsignedInt> styleFeatureCounts, const std::initializer_list<Int> styleCursorStyles, const std::initializer_list<Int> styleSelectionStyles, const std::initializer_list<Vector4> stylePaddings) {
    return static_cast<Shared&>(TextLayer::Shared::setStyle(commonUniform, uniforms, styleToUniform, styleFonts, styleAlignments, styleFeatures, styleFeatureOffsets, styleFeatureCounts, styleCursorStyles, styleSelectionStyles, stylePaddings));
}

void TextLayerGL::Shared::doSetStyle(const TextLayerCommonStyleUniform& commonUniform, const Containers::ArrayView<const TextLayerStyleUniform> uniforms) {
    /* This function should get called only if the dynamic style count is 0 */
    auto& state = static_cast<State&>(*_state);
    CORRADE_INTERNAL_ASSERT(state.dynamicStyleCount == 0);

    /** @todo the common stuff wouldn't even need to be uploaded, skipping it
        causes no breakage in the shader */
    state.styleBuffer.setSubData(0, {&commonUniform, 1});
    state.styleBuffer.setSubData(sizeof(TextLayerCommonStyleUniform), uniforms);
}

void TextLayerGL::Shared::doSetEditingStyle(const TextLayerCommonEditingStyleUniform& commonUniform, const Containers::ArrayView<const TextLayerEditingStyleUniform> uniforms) {
    /* This function should get called only if the dynamic style count is 0 */
    auto& state = static_cast<State&>(*_state);
    CORRADE_INTERNAL_ASSERT(state.dynamicStyleCount == 0);

    state.editingStyleBuffer.setSubData(0, {&commonUniform, 1});
    state.editingStyleBuffer.setSubData(sizeof(TextLayerCommonEditingStyleUniform), uniforms);
}

struct TextLayerGL::State: TextLayer::State {
    explicit State(Shared::State& shared, const TextLayerFlags flags): TextLayer::State{shared, flags} {}

    GL::Buffer vertexBuffer{GL::Buffer::TargetHint::Array},
        indexBuffer{GL::Buffer::TargetHint::ElementArray};
    GL::Mesh mesh;
    Vector2 clipScale;
    Vector2i framebufferSize;

    /* Used only if shared.hasEditingStyles is set */
    GL::Buffer editingVertexBuffer{NoCreate}, editingIndexBuffer{NoCreate};
    GL::Mesh editingMesh{NoCreate};

    /* Used only if shared.dynamicStyleCount is non-zero (and then also
       shared.hasEditingStyles is set in case of editingStyleBuffer), in which
       case it's created during the first doUpdate(). Even though the size is
       known in advance, the NoCreate'd state is used to correctly perform the
       first ever style upload without having to implicitly set any
       LayerStates. */
    GL::Buffer styleBuffer{NoCreate};
    GL::Buffer editingStyleBuffer{NoCreate};
};

TextLayerGL::TextLayerGL(const LayerHandle handle, Shared& sharedState_, const TextLayerFlags flags): TextLayer{handle, Containers::pointer<State>(static_cast<Shared::State&>(*sharedState_._state), flags)} {
    auto& state = static_cast<State&>(*_state);
    auto& sharedState = static_cast<Shared::State&>(state.shared);
    if(sharedState.flags >= TextLayerSharedFlag::DistanceField)
        state.mesh.addVertexBuffer(state.vertexBuffer, 0,
            TextShaderGL::Position{},
            TextShaderGL::TextureCoordinates{},
            TextShaderGL::Color4{},
            TextShaderGL::Style{},
            TextShaderGL::Scale{});
    else
        state.mesh.addVertexBuffer(state.vertexBuffer, 0,
            TextShaderGL::Position{},
            TextShaderGL::TextureCoordinates{},
            TextShaderGL::Color4{},
            TextShaderGL::Style{});
    state.mesh.setIndexBuffer(state.indexBuffer, 0, GL::MeshIndexType::UnsignedInt);

    if(sharedState.hasEditingStyles) {
        state.editingVertexBuffer = GL::Buffer{GL::Buffer::TargetHint::Array};
        state.editingIndexBuffer = GL::Buffer{GL::Buffer::TargetHint::ElementArray};
        state.editingMesh = GL::Mesh{};
        state.editingMesh.addVertexBuffer(state.editingVertexBuffer, 0,
            TextEditingShaderGL::Position{},
            TextEditingShaderGL::CenterDistance{},
            TextEditingShaderGL::Opacity{},
            TextEditingShaderGL::Style{});
        state.editingMesh.setIndexBuffer(state.editingIndexBuffer, 0, GL::MeshIndexType::UnsignedInt);
    }
}

LayerFeatures TextLayerGL::doFeatures() const {
    return TextLayer::doFeatures()|LayerFeature::DrawUsesBlending|LayerFeature::DrawUsesScissor;
}

void TextLayerGL::doSetSize(const Vector2& size, const Vector2i& framebufferSize) {
    auto& state = static_cast<State&>(*_state);
    auto& sharedState = static_cast<Shared::State&>(state.shared);

    /** @todo Max or min? Should I even bother with non-square scaling? */
    sharedState.shader.setProjection(size, (size/Vector2{framebufferSize}).max(), sharedState.distanceFieldScaling);
    if(sharedState.hasEditingStyles)
        /** @todo Max or min? Should I even bother with non-square scaling? */
        sharedState.editingShader.setProjection(size, (size/Vector2{framebufferSize}).max());

    /* For scaling and Y-flipping the clip rects in doDraw() */
    state.clipScale = Vector2{framebufferSize}/size;
    state.framebufferSize = framebufferSize;
}

void TextLayerGL::doUpdate(const LayerStates states, const Containers::StridedArrayView1D<const UnsignedInt>& dataIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, const Containers::StridedArrayView1D<const Vector2>& nodeOffsets, const Containers::StridedArrayView1D<const Vector2>& nodeSizes, const Containers::StridedArrayView1D<const Float>& nodeOpacities, const Containers::BitArrayView nodesEnabled, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes, const Containers::StridedArrayView1D<const Vector2>& compositeRectOffsets, const Containers::StridedArrayView1D<const Vector2>& compositeRectSizes) {
    State& state = static_cast<State&>(*_state);
    Shared::State& sharedState = static_cast<Shared::State&>(state.shared);

    /* Check whether the shared styles changed before calling into the base
       doUpdate() that syncs the stamps. For dynamic styles, if the style
       changed, it should be accompanied by NeedsCommonDataUpdate being set in
       order to be correctly handled below. */
    const bool sharedStyleChanged = sharedState.styleUpdateStamp != state.styleUpdateStamp;
    const bool sharedEditingStyleChanged = sharedState.editingStyleUpdateStamp != state.editingStyleUpdateStamp;
    CORRADE_INTERNAL_ASSERT(!sharedState.dynamicStyleCount || (!sharedStyleChanged && !sharedEditingStyleChanged && !state.dynamicStyleChanged && !state.dynamicEditingStyleChanged) || states >= LayerState::NeedsCommonDataUpdate);

    TextLayer::doUpdate(states, dataIds, clipRectIds, clipRectDataCounts, nodeOffsets, nodeSizes, nodeOpacities, nodesEnabled, clipRectOffsets, clipRectSizes, compositeRectOffsets, compositeRectSizes);

    /* The branching here mirrors how TextLayer::doUpdate() restricts the
       updates. Keep in sync. */
    if(states >= LayerState::NeedsNodeOrderUpdate ||
       states >= LayerState::NeedsDataUpdate)
    {
        state.indexBuffer.setData(state.indices);
        state.mesh.setCount(state.indices.size());
        if(sharedState.hasEditingStyles) {
            state.editingIndexBuffer.setData(state.editingIndices);
            state.editingMesh.setCount(state.editingIndices.size());
        }
    }
    if(states >= LayerState::NeedsNodeOffsetSizeUpdate ||
       states >= LayerState::NeedsNodeEnabledUpdate ||
       states >= LayerState::NeedsNodeOpacityUpdate ||
       states >= LayerState::NeedsDataUpdate)
    {
        state.vertexBuffer.setData(state.vertices);
        if(sharedState.hasEditingStyles)
            state.editingVertexBuffer.setData(state.editingVertices);
    }

    /* If we have dynamic styles and either NeedsCommonDataUpdate is set
       (meaning either the static style or the dynamic style changed) or
       they haven't been uploaded yet at all, upload them. */
    if(sharedState.dynamicStyleCount && ((states >= LayerState::NeedsCommonDataUpdate) || !state.styleBuffer.id())) {
        const bool needsFirstUpload = !state.styleBuffer.id();
        if(needsFirstUpload) {
            /* If dynamic editing styles are enabled, there's two extra styles
               for each dynamic style, one for reserved for under-cursor text
               and one for selected text */
            /** @todo check if DynamicDraw has any effect on perf */
            state.styleBuffer = GL::Buffer{GL::Buffer::TargetHint::Uniform, {nullptr, sizeof(TextLayerCommonStyleUniform) + sizeof(TextLayerStyleUniform)*(sharedState.styleUniformCount + sharedState.dynamicStyleCount*(sharedState.hasEditingStyles ? 3 : 1))}, GL::BufferUsage::DynamicDraw};
        }
        if(needsFirstUpload || sharedStyleChanged) {
            state.styleBuffer.setSubData(0, {&sharedState.commonStyleUniform, 1});
            /* If dynamic styles include editing styles, styleUniforms contain
               also uniforms used for text selection. If there are no dynamic
               editing styles, the array may be empty if there are only dynamic
               styles -- then skip the empty upload. */
            if(!sharedState.styleUniforms.isEmpty())
                state.styleBuffer.setSubData(sizeof(TextLayerCommonStyleUniform), sharedState.styleUniforms);
        }
        if(needsFirstUpload || state.dynamicStyleChanged) {
            state.styleBuffer.setSubData(sizeof(TextLayerCommonStyleUniform) + sizeof(TextLayerStyleUniform)*sharedState.styleUniformCount, state.dynamicStyleUniforms);
            state.dynamicStyleChanged = false;
        }
    }

    /* If we have any dynamic editing styles and either NeedsCommonDataUpdate
       is set (meaning either the static style or the dynamic style changed) or
       they haven't been uploaded yet at all, upload them. */
    if(sharedState.hasEditingStyles && sharedState.dynamicStyleCount && ((states >= LayerState::NeedsCommonDataUpdate) || !state.editingStyleBuffer.id())) {
        const bool needsFirstUpload = !state.editingStyleBuffer.id();
        if(needsFirstUpload) {
            /* Each dynamic style has two associated editing styles, one for
               cursor and one for selection */
            /** @todo check if DynamicDraw has any effect on perf */
            state.editingStyleBuffer = GL::Buffer{GL::Buffer::TargetHint::Uniform, {nullptr, sizeof(TextLayerCommonEditingStyleUniform) + sizeof(TextLayerEditingStyleUniform)*(sharedState.editingStyleUniformCount + 2*sharedState.dynamicStyleCount)}, GL::BufferUsage::DynamicDraw};
        }
        if(needsFirstUpload || sharedEditingStyleChanged) {
            state.editingStyleBuffer.setSubData(0, {&sharedState.commonEditingStyleUniform, 1});
            /* Skip empty upload if there are just dynamic styles */
            if(!sharedState.editingStyleUniforms.isEmpty())
                state.editingStyleBuffer.setSubData(sizeof(TextLayerCommonEditingStyleUniform), sharedState.editingStyleUniforms);
        }
        if(needsFirstUpload || state.dynamicEditingStyleChanged) {
            state.editingStyleBuffer.setSubData(sizeof(TextLayerCommonEditingStyleUniform) + sizeof(TextLayerEditingStyleUniform)*sharedState.editingStyleUniformCount, state.dynamicEditingStyleUniforms);
            state.dynamicEditingStyleChanged = false;
        }
    }
}

void TextLayerGL::doDraw(const Containers::StridedArrayView1D<const UnsignedInt>&, const std::size_t offset, const std::size_t count, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectIds, const Containers::StridedArrayView1D<const UnsignedInt>& clipRectDataCounts, const std::size_t clipRectOffset, const std::size_t clipRectCount, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Vector2>&, const Containers::StridedArrayView1D<const Float>&, Containers::BitArrayView, const Containers::StridedArrayView1D<const Vector2>& clipRectOffsets, const Containers::StridedArrayView1D<const Vector2>& clipRectSizes) {
    auto& state = static_cast<State&>(*_state);
    CORRADE_ASSERT(!state.framebufferSize.isZero() && !state.clipScale.isZero(),
        "Ui::TextLayerGL::draw(): user interface size wasn't set", );

    auto& sharedState = static_cast<Shared::State&>(state.shared);
    CORRADE_ASSERT(sharedState.setStyleCalled,
        "Ui::TextLayerGL::draw(): no style data was set", );

    sharedState.shader.bindGlyphTexture(static_cast<Text::GlyphCacheArrayGL&>(sharedState.glyphCache).texture());

    /* If there are dynamic styles, bind the layer-specific buffer that
       contains them, otherwise bind the shared buffer */
    sharedState.shader.bindStyleBuffer(sharedState.dynamicStyleCount ?
        state.styleBuffer : sharedState.styleBuffer);
    /* Similarly for the editing shader, that one is created only if there are
       actually any editing styles. The two shaders have a non-conflicting
       binding point so they can be both bound upfront. */
    if(sharedState.hasEditingStyles)
        sharedState.editingShader.bindStyleBuffer(sharedState.dynamicStyleCount ?
            state.editingStyleBuffer : sharedState.editingStyleBuffer);

    std::size_t clipDataOffset = offset;
    for(std::size_t i = 0; i != clipRectCount; ++i) {
        const UnsignedInt clipRectId = clipRectIds[clipRectOffset + i];
        const UnsignedInt clipRectDataCount = clipRectDataCounts[clipRectOffset + i];
        const Vector2i clipRectOffset_ = Vector2i{clipRectOffsets[clipRectId]*state.clipScale};
        const Vector2i clipRectSize = clipRectSizes[clipRectId].isZero() ?
            state.framebufferSize : Vector2i{clipRectSizes[clipRectId]*state.clipScale};

        GL::Renderer::setScissor(Range2Di::fromSize(
            {clipRectOffset_.x(), state.framebufferSize.y() - clipRectOffset_.y() - clipRectSize.y()},
            clipRectSize));

        /* If there are any selection / cursor quads for texts in this clip
           rect, draw them before the actual text. The assumption is that
           editable texts aren't overlapping in a single top-level node, so it
           should be fine to render them all before the actual texts instead of
           right before every piece of editable text. */
        /** @todo Better would probably be outside of this loop to trade
            expensive shader switching for a less expensive duplicated scissor
            state update, but then would have to duplicate the scissor and all
            the other logic. Or maybe let's just implement the clipping in the
            shader instead and draw everything at once? */
        if(const UnsignedInt indexCount = state.indexDrawOffsets[clipDataOffset + clipRectDataCount].second() - state.indexDrawOffsets[clipDataOffset].second()) {
            state.editingMesh
                .setIndexOffset(state.indexDrawOffsets[clipDataOffset].second())
                .setCount(indexCount);
            sharedState.editingShader
                .draw(state.editingMesh);
        }

        state.mesh
            .setIndexOffset(state.indexDrawOffsets[clipDataOffset].first())
            .setCount(state.indexDrawOffsets[clipDataOffset + clipRectDataCount].first() - state.indexDrawOffsets[clipDataOffset].first());
        sharedState.shader
            .draw(state.mesh);

        clipDataOffset += clipRectDataCount;
    }

    CORRADE_INTERNAL_ASSERT(clipDataOffset == offset + count);
    #ifdef CORRADE_NO_ASSERT
    static_cast<void>(count);
    #endif
}

}}
