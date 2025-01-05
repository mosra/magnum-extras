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

#include <sstream>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/StringStl.h> /** @todo drop once Debug is stream-free */
#include <Corrade/Utility/Format.h>
#include <Corrade/Utility/FormatStl.h> /** @todo drop once Debug is stream-free */
#include <Corrade/Utility/Path.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/Math/Matrix3.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/Primitives/Square.h>
#include <Magnum/Shaders/FlatGL.h>
#include <Magnum/Text/Alignment.h>
#include <Magnum/Trade/AbstractImporter.h>
#include <Magnum/Trade/ImageData.h>
#include <Magnum/Trade/MeshData.h>

#include "Magnum/Ui/Anchor.h"
#include "Magnum/Ui/Label.h"
#include "Magnum/Ui/SnapLayouter.h"
#include "Magnum/Ui/TextProperties.h"
#include "Magnum/Ui/UserInterface.h"

#include "AbstractPlayer.h"
#include "LoadImage.h"

namespace Magnum { namespace Player {

namespace {

constexpr const Float LabelHeight{36.0f};
constexpr const Vector2 LabelSize{72.0f, LabelHeight};

class ImagePlayer: public AbstractPlayer {
    public:
        explicit ImagePlayer(Platform::ScreenedApplication& application, Ui::UserInterface& ui, Ui::NodeHandle controls);

    private:
        void drawEvent() override;
        void viewportEvent(ViewportEvent& event) override;
        void keyPressEvent(KeyEvent& event) override;
        void pointerMoveEvent(PointerMoveEvent& event) override;
        void scrollEvent(ScrollEvent& event) override;

        void load(Containers::StringView filename, Trade::AbstractImporter& importer, Int id) override;

        Vector2 unproject(const Vector2& windowPosition) const;
        Vector2 unprojectRelative(const Vector2& relativeWindowPosition) const;

        Shaders::FlatGL2D _coloredShader;

        /* UI */
        /* Owning base node for all UI stuff here, child of the `controls` node
           from constructor. Gets removed when the screen is destructed, taking
           with itself everything parented to it, so when another screen is
           created again, there are no stale nodes left around. */
        /** @todo none of this actually needs to be recreated, and neither the
            shaders etc., rethink the whole thing */
        Ui::Widget _screen;
        Ui::Label _imageInfo;

        GL::Texture2D _texture{NoCreate};
        GL::Mesh _square;
        Shaders::FlatGL2D _shader{Shaders::FlatGL2D::Configuration{}
            .setFlags(Shaders::FlatGL2D::Flag::Textured)};
        Vector2i _imageSize;
        Matrix3 _transformation;
        Matrix3 _projection;
};

ImagePlayer::ImagePlayer(Platform::ScreenedApplication& application, Ui::UserInterface& ui, Ui::NodeHandle controls):
    AbstractPlayer{application, PropagatedEvent::Draw|PropagatedEvent::Input},
    _screen{Ui::snap(ui, Ui::Snap::Fill|Ui::Snap::NoPad, controls, {})},
    _imageInfo{Ui::snap(ui, Ui::Snap::TopLeft|Ui::Snap::Inside, _screen, LabelSize), {}, Ui::LabelStyle::Dim}
{
    /* Prepare the square mesh and initial projection equal to framebuffer size */
    _square = MeshTools::compile(Primitives::squareSolid(Primitives::SquareFlag::TextureCoordinates));
    _projection = Matrix3::projection(Vector2{application.framebufferSize()});
}

void ImagePlayer::drawEvent() {
    #ifdef MAGNUM_TARGET_WEBGL /* Another FB could be bound from the depth read */
    GL::defaultFramebuffer.bind();
    #endif
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color|GL::FramebufferClear::Depth);

    _shader.bindTexture(_texture)
        .setTransformationProjectionMatrix(_projection*_transformation);

    /* Enable blending, disable depth test */
    GL::Renderer::enable(GL::Renderer::Feature::Blending);
    GL::Renderer::disable(GL::Renderer::Feature::DepthTest);

    /* Draw the image with non-premultiplied alpha blending as that's the
       common format */
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::SourceAlpha, GL::Renderer::BlendFunction::OneMinusSourceAlpha);
    _shader.draw(_square);

    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::disable(GL::Renderer::Feature::Blending);
}

void ImagePlayer::viewportEvent(ViewportEvent& event) {
    _projection = Matrix3::projection(Vector2{event.framebufferSize()});
}

void ImagePlayer::keyPressEvent(KeyEvent& event) {
    if(event.key() == Key::NumZero) {
        if((_imageSize > application().framebufferSize()*0.5f).any())
            _transformation = Matrix3::scaling(Vector2{_imageSize}/2.0f);
        else
            _transformation = Matrix3::scaling(application().framebufferSize().min()*0.9f*Vector2{1.0f, 1.0f/Vector2{_imageSize}.aspectRatio()});
    } else return;

    event.setAccepted();
    redraw();
}

Vector2 ImagePlayer::unproject(const Vector2& windowPosition) const {
    /* Normalize from window-relative position with origin at top left and Y
       down to framebuffer-relative position with origin at center and Y going
       up */
    return (windowPosition/Vector2{application().windowSize()} - Vector2{0.5f})*Vector2{application().framebufferSize()}*Vector2::yScale(-1.0f);
}

Vector2 ImagePlayer::unprojectRelative(const Vector2& relativeWindowPosition) const {
    /* Only resizing for framebuffer-relative position and Y going up instead
       of down, no origin movements */
    return relativeWindowPosition*Vector2{application().framebufferSize()}*Vector2::yScale(-1.0f)/Vector2{application().windowSize()};
}

void ImagePlayer::pointerMoveEvent(PointerMoveEvent& event) {
    if(!event.isPrimary() ||
       !(event.pointers() & (Pointer::MouseLeft|Pointer::Finger)))
        return;

    const Vector2 delta = unprojectRelative(event.relativePosition());
    _transformation = Matrix3::translation(delta)*_transformation;
    event.setAccepted();
    redraw();
}

void ImagePlayer::scrollEvent(ScrollEvent& event) {
    if(!event.offset().y()) return;

    /* Zoom to selection point -- translate that point to origin, scale,
       translate back */
    const Vector2 projectedPosition = unproject(event.position());
    _transformation =
        Matrix3::translation(projectedPosition)*
        Matrix3::scaling(Vector2{1.0f + 0.1f*event.offset().y()})*
        Matrix3::translation(-projectedPosition)*
        _transformation;

    event.setAccepted();
    redraw();
}

void ImagePlayer::load(Containers::StringView filename, Trade::AbstractImporter& importer, Int id) {
    if(id < 0) id = 0;
    else if(UnsignedInt(id) >= importer.image2DCount()) {
        Fatal{} << "Cannot load an image with ID" << id << "as there's only" << importer.image2DCount() << "images";
    }

    Debug{} << "Loading image" << id << importer.image2DName(id);

    Containers::Optional<Trade::ImageData2D> image = importer.image2D(id);
    if(!image) return;

    _texture = GL::Texture2D{};
    _texture
        .setMagnificationFilter(GL::SamplerFilter::Nearest)
        .setMinificationFilter(GL::SamplerFilter::Linear)
        .setWrapping(GL::SamplerWrapping::ClampToEdge);

    loadImage(_texture, *image);

    /* Set up default transformation. Centered, 1:1 scale if more than 50% of
       the view, otherwise scaled up to 90% of the view. */
    _imageSize = image->size();
    if(_transformation == Matrix3{}) {
        if((_imageSize > application().framebufferSize()*0.5f).any())
            _transformation = Matrix3::scaling(Vector2{_imageSize}/2.0f);
        else
            _transformation = Matrix3::scaling(application().framebufferSize().min()*0.9f*Vector2{1.0f, 1.0f/Vector2{_imageSize}.aspectRatio()}/2.0f);
    }

    /* Populate the model info */
    /** @todo ugh debug->format converter?! */
    std::ostringstream out;
    if(!image->isCompressed())
        Debug{&out, Debug::Flag::NoNewlineAtTheEnd} << image->format();
    else
        Debug{&out, Debug::Flag::NoNewlineAtTheEnd} << image->compressedFormat();

    _imageInfo.setText(Utility::format(
        "{}: {}x{}, {}",
        Utility::Path::split(filename).second(),
        image->size().x(), image->size().y(),
        out.str()),
        /** @todo ugh, having to specify this every time is NASTY, what to do
            besides supplying extra style variants? */
        Text::Alignment::MiddleLeft);
}

}

Containers::Pointer<AbstractPlayer> createImagePlayer(Platform::ScreenedApplication& application, Ui::UserInterface& ui, Ui::NodeHandle controls) {
    return Containers::Pointer<ImagePlayer>{InPlaceInit, application, ui, controls};
}

}}
