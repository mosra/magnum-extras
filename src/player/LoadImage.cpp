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

#include "LoadImage.h"

#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Utility/Algorithms.h>
#include <Magnum/ImageView.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/Math/Functions.h>
#include <Magnum/GL/Context.h>
#include <Magnum/GL/Extensions.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/Trade/ImageData.h>

namespace Magnum { namespace Player {

void loadImage(GL::Texture2D& texture, const Trade::ImageData2D& image) {
    if(!image.isCompressed()) {
        /* Single-channel images are probably meant to represent grayscale,
           two-channel grayscale + alpha. Probably, there's no way to know, but
           given we're using them for *colors*, it makes more sense than
           displaying them just red or red+green. */
        Containers::Array<char> usedImageStorage;
        ImageView2D usedImage = image;
        const UnsignedInt channelCount = pixelFormatChannelCount(image.format());
        if(channelCount == 1 || channelCount == 2) {
            #ifndef MAGNUM_TARGET_WEBGL
            #ifndef MAGNUM_TARGET_GLES
            /* Available in GLES 3 always */
            if(GL::Context::current().isExtensionSupported<GL::Extensions::ARB::texture_swizzle>())
            #endif
            {
                if(channelCount == 1)
                    texture.setSwizzle<'r', 'r', 'r', '1'>();
                else if(channelCount == 2)
                    texture.setSwizzle<'r', 'r', 'r', 'g'>();
            }
            #ifndef MAGNUM_TARGET_GLES
            else
            #endif
            #endif
            {
                /* Without texture swizzle support, allocate a copy of the image
                   and expand the channels manually */
                /** @todo make this a utility in TextureTools, with the channel
                    expansion being an optimized routine in Math/PackingBatch.h */
                const PixelFormat imageFormat = pixelFormat(image.format(), channelCount == 2 ? 4 : 3, isPixelFormatSrgb(image.format()));
                Debug{} << "Texture swizzle not supported, expanding a" << image.format() << "image to" << imageFormat;

                /* Pad to four-byte rows to not have to use non-optimal
                   alignment */
                const std::size_t rowStride = 4*((pixelFormatSize(imageFormat)*image.size().x() + 3)/4);
                usedImageStorage = Containers::Array<char>{NoInit, std::size_t(rowStride*image.size().y())};
                const MutableImageView2D usedMutableImage{imageFormat, image.size(), usedImageStorage};
                usedImage = usedMutableImage;

                /* Create 4D pixel views (rows, pixels, channels, channel
                   bytes) */
                const std::size_t channelSize = pixelFormatSize(pixelFormatChannelFormat(imageFormat));
                const std::size_t dstChannelCount = pixelFormatChannelCount(imageFormat);
                const Containers::StridedArrayView4D<const char> src = image.pixels().expanded<2>(Containers::Size2D{channelCount, channelSize});
                const Containers::StridedArrayView4D<char> dst = usedMutableImage.pixels().expanded<2>(Containers::Size2D{dstChannelCount, channelSize});

                /* Broadcast the red channel of the input to RRR and copy to
                   the RGB channels of the output */
                Utility::copy(
                    src.exceptSuffix({0, 0, channelCount == 2 ? 1 : 0, 0}).broadcasted<2>(3),
                    dst.exceptSuffix({0, 0, channelCount == 2 ? 1 : 0, 0}));
                /* If there's an alpha channel, copy it over as well */
                if(channelCount == 2) Utility::copy(
                    src.exceptPrefix({0, 0, 1, 0}),
                    dst.exceptPrefix({0, 0, 3, 0}));
            }
        }

        /* Whitelist only things we *can* display */
        /** @todo signed formats, exposure knob for float formats */
        GL::TextureFormat format;
        switch(usedImage.format()) {
            case PixelFormat::R8Unorm:
            case PixelFormat::RG8Unorm:
            /* can't really do sRGB R/RG as there are no widely available
               desktop extensions :( */
            case PixelFormat::RGB8Unorm:
            case PixelFormat::RGB8Srgb:
            case PixelFormat::RGBA8Unorm:
            case PixelFormat::RGBA8Srgb:
            /* I guess we can try using 16-bit formats even though our displays
               won't be able to show all the detail */
            case PixelFormat::R16Unorm:
            case PixelFormat::RG16Unorm:
            case PixelFormat::RGB16Unorm:
            case PixelFormat::RGBA16Unorm:
            /* Floating point is fine too */
            case PixelFormat::R16F:
            case PixelFormat::RG16F:
            case PixelFormat::RGB16F:
            case PixelFormat::RGBA16F:
            case PixelFormat::R32F:
            case PixelFormat::RG32F:
            case PixelFormat::RGB32F:
            case PixelFormat::RGBA32F:
                format = GL::textureFormat(usedImage.format());
                break;
            default:
                Warning{} << "Cannot load an image of format" << usedImage.format();
                return;
        }

        texture
            .setStorage(Math::log2(usedImage.size().max()) + 1, format, usedImage.size())
            .setSubImage(0, {}, usedImage)
            .generateMipmap();

    } else {
        /* Blacklist things we *cannot* display */
        GL::TextureFormat format;
        switch(image.compressedFormat()) {
            /** @todo signed formats, float formats */
            case CompressedPixelFormat::Bc4RSnorm:
            case CompressedPixelFormat::Bc5RGSnorm:
            case CompressedPixelFormat::EacR11Snorm:
            case CompressedPixelFormat::EacRG11Snorm:
            case CompressedPixelFormat::Bc6hRGBUfloat:
            case CompressedPixelFormat::Bc6hRGBSfloat:
            case CompressedPixelFormat::Astc4x4RGBAF:
            case CompressedPixelFormat::Astc5x4RGBAF:
            case CompressedPixelFormat::Astc5x5RGBAF:
            case CompressedPixelFormat::Astc6x5RGBAF:
            case CompressedPixelFormat::Astc6x6RGBAF:
            case CompressedPixelFormat::Astc8x5RGBAF:
            case CompressedPixelFormat::Astc8x6RGBAF:
            case CompressedPixelFormat::Astc8x8RGBAF:
            case CompressedPixelFormat::Astc10x5RGBAF:
            case CompressedPixelFormat::Astc10x6RGBAF:
            case CompressedPixelFormat::Astc10x8RGBAF:
            case CompressedPixelFormat::Astc10x10RGBAF:
            case CompressedPixelFormat::Astc12x10RGBAF:
            case CompressedPixelFormat::Astc12x12RGBAF:
                Warning{} << "Cannot load an image of format" << image.compressedFormat();
                return;

            default: format = GL::textureFormat(image.compressedFormat());
        }

        texture
            .setStorage(1, format, image.size())
            .setCompressedSubImage(0, {}, image);
            /** @todo mip level loading */
    }
}

}}
