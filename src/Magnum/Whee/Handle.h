#ifndef Magnum_Whee_Handle_h
#define Magnum_Whee_Handle_h
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

/** @file
 * @brief Handle @ref Magnum::Whee::LayerHandle, @ref Magnum::Whee::LayerDataHandle, @ref Magnum::Whee::DataHandle, @ref Magnum::Whee::NodeHandle, @ref Magnum::Whee::LayouterHandle, @ref Magnum::Whee::LayouterDataHandle, @ref Magnum::Whee::LayoutHandle, @ref Magnum::Whee::AnimatorHandle, @ref Magnum::Whee::AnimatorDataHandle, @ref Magnum::Whee::AnimationHandle, function @ref Magnum::Whee::layerHandle(), @ref Magnum::Whee::layerHandleId(), @ref Magnum::Whee::layerHandleGeneration(), @ref Magnum::Whee::layerDataHandle(), @ref Magnum::Whee::layerDataHandleId(), @ref Magnum::Whee::layerDataHandleGeneration(), @ref Magnum::Whee::dataHandle(), @ref Magnum::Whee::dataHandleLayer(), @ref Magnum::Whee::dataHandleData(), @ref Magnum::Whee::dataHandleLayerId(), @ref Magnum::Whee::dataHandleLayerGeneration(), @ref Magnum::Whee::dataHandleId(), @ref Magnum::Whee::dataHandleGeneration(), @ref Magnum::Whee::nodeHandle(), @ref Magnum::Whee::nodeHandleId(), @ref Magnum::Whee::nodeHandleGeneration(), @ref Magnum::Whee::layouterHandle(), @ref Magnum::Whee::layouterHandleId(), @ref Magnum::Whee::layouterHandleGeneration(), @ref Magnum::Whee::layouterDataHandle(), @ref Magnum::Whee::layouterDataHandleId(), @ref Magnum::Whee::layouterDataHandleGeneration(), @ref Magnum::Whee::layoutHandle(), @ref Magnum::Whee::layoutHandleLayouter(), @ref Magnum::Whee::layoutHandleData(), @ref Magnum::Whee::layoutHandleLayouterId(), @ref Magnum::Whee::layoutHandleLayouterGeneration(), @ref Magnum::Whee::layoutHandleId(), @ref Magnum::Whee::layoutHandleGeneration(), @ref Magnum::Whee::animatorHandle(), @ref Magnum::Whee::animatorHandleId(), @ref Magnum::Whee::animatorHandleGeneration(), @ref Magnum::Whee::animatorDataHandle(), @ref Magnum::Whee::animatorDataHandleId(), @ref Magnum::Whee::animatorDataHandleGeneration(), @ref Magnum::Whee::animationHandle(), @ref Magnum::Whee::animationHandleAnimator(), @ref Magnum::Whee::animationHandleData(), @ref Magnum::Whee::animationHandleAnimatorId(), @ref Magnum::Whee::animationHandleAnimatorGeneration(), @ref Magnum::Whee::animationHandleId(), @ref Magnum::Whee::animationHandleGeneration()
 * @m_since_latest
 */

#include <Corrade/Utility/DebugAssert.h>
#include <Magnum/Magnum.h>

#include "Magnum/Whee/Whee.h"
#include "Magnum/Whee/visibility.h"

namespace Magnum { namespace Whee {

namespace Implementation {
    enum: UnsignedInt {
        LayerHandleIdBits = 8,
        LayerHandleGenerationBits = 8
    };
}

/**
@brief Layer handle
@m_since_latest

Uses 8 bits for storing an ID and 8 bits for a generation.
@see @ref AbstractUserInterface::createLayer(),
    @ref AbstractUserInterface::removeLayer(), @ref AbstractLayer,
    @ref layerHandle(), @ref layerHandleId(), @ref layerHandleGeneration()
*/
enum class LayerHandle: UnsignedShort {
    Null = 0    /**< Null handle */
};

/**
@debugoperatorenum{LayerHandle}
@m_since_latest
*/
MAGNUM_WHEE_EXPORT Debug& operator<<(Debug& debug, LayerHandle value);

/**
@brief Compose a layer handle from an ID and a generation
@m_since_latest

Expects that the ID fits into 8 bits and the generation into 8 bits. Use
@ref layerHandleId() and @ref layerHandleGeneration() for an inverse operation.
*/
constexpr LayerHandle layerHandle(UnsignedInt id, UnsignedInt generation) {
    return (CORRADE_CONSTEXPR_DEBUG_ASSERT(id < (1 << Implementation::LayerHandleIdBits) && generation < (1 << Implementation::LayerHandleGenerationBits),
        "Whee::layerHandle(): expected index to fit into" << Implementation::LayerHandleIdBits << "bits and generation into" << Implementation::LayerHandleGenerationBits << Debug::nospace << ", got" << Debug::hex << id << "and" << Debug::hex << generation), LayerHandle(id|(generation << Implementation::LayerHandleIdBits)));
}

/**
@brief Extract ID from a layer handle
@m_since_latest

Expects that @p handle is not @ref LayerHandle::Null. Use
@ref layerHandleGeneration() for extracting the generation and
@ref layerHandle() for an inverse operation.
*/
constexpr UnsignedInt layerHandleId(LayerHandle handle) {
    return (CORRADE_CONSTEXPR_DEBUG_ASSERT(handle != LayerHandle::Null,
        "Whee::layerHandleId(): the handle is null"),
        UnsignedInt(handle) & ((1 << Implementation::LayerHandleIdBits) - 1));
}

/**
@brief Extract generation from a layer handle
@m_since_latest

For @ref LayerHandle::Null returns @cpp 0 @ce. A valid handle has always a
non-zero generation. Use @ref layerHandleId() for extracting the ID and
@ref layerHandle() for an inverse operation.
*/
constexpr UnsignedInt layerHandleGeneration(LayerHandle handle) {
    return UnsignedInt(handle) >> Implementation::LayerHandleIdBits;
}

namespace Implementation {
    enum: UnsignedInt {
        LayerDataHandleIdBits = 20,
        LayerDataHandleGenerationBits = 12
    };
}

/**
@brief Layer data handle
@m_since_latest

Uses 20 bits for storing an ID and 12 bits for a generation.
@see @ref AbstractLayer::create(), @ref AbstractLayer::remove(),
    @ref layerDataHandle(), @ref layerDataHandleId(),
    @ref layerDataHandleGeneration()
*/
enum class LayerDataHandle: UnsignedInt {
    Null = 0    /**< Null handle */
};

/**
@debugoperatorenum{LayerDataHandle}
@m_since_latest
*/
MAGNUM_WHEE_EXPORT Debug& operator<<(Debug& debug, LayerDataHandle value);

/**
@brief Compose a layer data handle from an ID and a generation
@m_since_latest

Expects that the ID fits into 20 bits and the generation into 12 bits. Use
@ref layerDataHandleId() and @ref layerDataHandleGeneration() for an inverse
operation.
*/
constexpr LayerDataHandle layerDataHandle(UnsignedInt id, UnsignedInt generation) {
    return (CORRADE_CONSTEXPR_DEBUG_ASSERT(id < (1 << Implementation::LayerDataHandleIdBits) && generation < (1 << Implementation::LayerDataHandleGenerationBits),
        "Whee::layerDataHandle(): expected index to fit into" << Implementation::LayerDataHandleIdBits << "bits and generation into" << Implementation::LayerDataHandleGenerationBits << Debug::nospace << ", got" << Debug::hex << id << "and" << Debug::hex << generation), LayerDataHandle(id|(generation << Implementation::LayerDataHandleIdBits)));
}

/**
@brief Extract ID from a layer data handle
@m_since_latest

Expects that @p handle is not @ref LayerDataHandle::Null. Use
@ref layerDataHandleGeneration() for extracting the generation and
@ref layerDataHandle() for an inverse operation.
*/
constexpr UnsignedInt layerDataHandleId(LayerDataHandle handle) {
    return (CORRADE_CONSTEXPR_DEBUG_ASSERT(handle != LayerDataHandle::Null,
        "Whee::layerDataHandleId(): the handle is null"),
        UnsignedInt(handle) & ((1 << Implementation::LayerDataHandleIdBits) - 1));
}

/**
@brief Extract generation from a layer data handle
@m_since_latest

For @ref LayerDataHandle::Null returns @cpp 0 @ce. A valid handle has always a
non-zero generation. Use @ref layerDataHandleId() for extracting the ID and
@ref layerDataHandle() for an inverse operation.
*/
constexpr UnsignedInt layerDataHandleGeneration(LayerDataHandle handle) {
    return UnsignedInt(handle) >> Implementation::LayerDataHandleIdBits;
}

/**
@brief Layer data handle
@m_since_latest

A combination of a @ref LayerHandle and a @ref LayerDataHandle. Uses 8 bits for
storing a layer ID, 8 bits for a layer generation, 20 bits for storing a data
ID and 12 bits for a data generation.
@see @ref AbstractLayer::create(), @ref AbstractLayer::remove(),
    @ref dataHandle(), @ref dataHandleId(),
    @ref dataHandleGeneration()
*/
enum class DataHandle: UnsignedLong {
    Null = 0    /**< Null handle */
};

/**
@debugoperatorenum{DataHandle}
@m_since_latest
*/
MAGNUM_WHEE_EXPORT Debug& operator<<(Debug& debug, DataHandle value);

/**
@brief Compose a data handle from a layer handle, a data ID and a data generation
@m_since_latest

Expects that the ID fits into 20 bits and the generation into 12 bits. Use
@ref dataHandleLayer(), @ref dataHandleId() and @ref dataHandleGeneration() for
an inverse operation.
@see @ref dataHandle(LayerHandle, LayerDataHandle), @ref dataHandleData(),
    @ref dataHandleLayerId(), @ref dataHandleLayerGeneration()
*/
constexpr DataHandle dataHandle(LayerHandle layerHandle, UnsignedInt id, UnsignedInt generation) {
    return (CORRADE_CONSTEXPR_DEBUG_ASSERT(id < (1 << Implementation::LayerDataHandleIdBits) && generation < (1 << Implementation::LayerDataHandleGenerationBits),
        "Whee::dataHandle(): expected index to fit into" << Implementation::LayerDataHandleIdBits << "bits and generation into" << Implementation::LayerDataHandleGenerationBits << Debug::nospace << ", got" << Debug::hex << id << "and" << Debug::hex << generation), DataHandle(id|(UnsignedLong(generation) << Implementation::LayerDataHandleIdBits)|(UnsignedLong(layerHandle) << (Implementation::LayerDataHandleIdBits + Implementation::LayerDataHandleGenerationBits))));
}

/**
@brief Compose a data handle from a layer handle and a layer data handle
@m_since_latest

Use @ref dataHandleLayer() and @ref dataHandleData() for the inverse operation.
@see @ref dataHandle(LayerHandle, LayerDataHandle), @ref dataHandleLayerId(),
    @ref dataHandleLayerGeneration(), @ref dataHandleId(),
    @ref dataHandleGeneration()
*/
constexpr DataHandle dataHandle(LayerHandle layerHandle, LayerDataHandle layerDataHandle) {
    return DataHandle((UnsignedLong(layerHandle) << (Implementation::LayerDataHandleIdBits + Implementation::LayerDataHandleGenerationBits))|UnsignedLong(layerDataHandle));
}

/**
@brief Extract layer handle from a data handle
@m_since_latest

Use @ref dataHandleData() for extracting the layer data handle and
@ref dataHandle(LayerHandle, LayerDataHandle) for an inverse operation.
*/
constexpr LayerHandle dataHandleLayer(DataHandle handle) {
    return LayerHandle(UnsignedLong(handle) >> (Implementation::LayerDataHandleIdBits + Implementation::LayerDataHandleGenerationBits));
}

/**
@brief Extract layer data handle from a data handle
@m_since_latest

Use @ref dataHandleLayer() for extracting the layer handle and
@ref dataHandle(LayerHandle, LayerDataHandle) for an inverse operation.
*/
constexpr LayerDataHandle dataHandleData(DataHandle handle) {
    return LayerDataHandle(UnsignedLong(handle));
}

/**
@brief Extract layer ID from a data handle
@m_since_latest

Expects that the layer portion of @p handle is not @ref LayerHandle::Null. Use
@ref dataHandleLayerGeneration() for extracting the layer generation and
@ref layerHandle() together with @ref dataHandle() for an inverse operation.
@see @ref dataHandleLayer()
*/
constexpr UnsignedInt dataHandleLayerId(DataHandle handle) {
    return (CORRADE_CONSTEXPR_DEBUG_ASSERT(LayerHandle(UnsignedLong(handle) >> (Implementation::LayerDataHandleIdBits + Implementation::LayerDataHandleGenerationBits)) != LayerHandle::Null,
        "Whee::dataHandleLayerId(): the layer portion of" << handle << "is null"),
        (UnsignedLong(handle) >> (Implementation::LayerDataHandleIdBits + Implementation::LayerDataHandleGenerationBits)) & ((1 << Implementation::LayerHandleIdBits) - 1));
}

/**
@brief Extract layer generation from a data handle
@m_since_latest

If the layer portion of the handle is @ref LayerHandle::Null, returns
@cpp 0 @ce. A valid handle has always a non-zero generation. Use
@ref dataHandleLayerId() for extracting the ID and @ref layerHandle() together
with @ref dataHandle() for an inverse operation.
@see @ref dataHandleLayer()
*/
constexpr UnsignedInt dataHandleLayerGeneration(DataHandle handle) {
    return (UnsignedLong(handle) >> (Implementation::LayerDataHandleIdBits + Implementation::LayerDataHandleGenerationBits + Implementation::LayerHandleIdBits)) & ((1 << Implementation::LayerHandleGenerationBits) - 1);
}

/**
@brief Extract ID from a data handle
@m_since_latest

Expects that the data portion of @p handle is not @ref LayerDataHandle::Null.
Use @ref dataHandleGeneration() for extracting the generation and
@ref dataHandle(LayerHandle, UnsignedInt, UnsignedInt) for an inverse
operation.
@see @ref dataHandleData()
*/
constexpr UnsignedInt dataHandleId(DataHandle handle) {
    return (CORRADE_CONSTEXPR_DEBUG_ASSERT(LayerDataHandle(UnsignedLong(handle)) != LayerDataHandle::Null,
        "Whee::dataHandleId(): the data portion of" << handle << "is null"),
        UnsignedLong(handle) & ((1 << Implementation::LayerDataHandleIdBits) - 1));
}

/**
@brief Extract generation from a data handle
@m_since_latest

If the data portion of @p handle is @ref LayerDataHandle::Null, returns
@cpp 0 @ce. A valid handle has always a non-zero generation. Use
@ref dataHandleId() for extracting the ID and
@ref dataHandle(LayerHandle, UnsignedInt, UnsignedInt) for an inverse
operation.
@see @ref dataHandleData()
*/
constexpr UnsignedInt dataHandleGeneration(DataHandle handle) {
    return (UnsignedLong(handle) >> Implementation::LayerDataHandleIdBits) & ((1 << Implementation::LayerDataHandleGenerationBits) - 1);
}

namespace Implementation {
    enum: UnsignedInt {
        NodeHandleIdBits = 20,
        NodeHandleGenerationBits = 12
    };
}

/**
@brief Node handle
@m_since_latest

Uses 20 bits for storing an ID and 12 bits for a generation.
@see @ref AbstractUserInterface::createNode(),
    @ref AbstractUserInterface::removeNode(), @ref nodeHandle(),
    @ref nodeHandleId(), @ref nodeHandleGeneration()
*/
enum class NodeHandle: UnsignedInt {
    Null = 0    /**< Null handle */
};

/**
@debugoperatorenum{NodeHandle}
@m_since_latest
*/
MAGNUM_WHEE_EXPORT Debug& operator<<(Debug& debug, NodeHandle value);

/**
@brief Compose a node handle from an ID and a generation
@m_since_latest

Expects that the ID fits into 20 bits and the generation into 12 bits. Use
@ref nodeHandleId() and @ref nodeHandleGeneration() for an inverse operation.
*/
constexpr NodeHandle nodeHandle(UnsignedInt id, UnsignedInt generation) {
    return (CORRADE_CONSTEXPR_DEBUG_ASSERT(id < (1 << Implementation::NodeHandleIdBits) && generation < (1 << Implementation::NodeHandleGenerationBits),
        "Whee::nodeHandle(): expected index to fit into" << Implementation::NodeHandleIdBits << "bits and generation into" << Implementation::NodeHandleGenerationBits << Debug::nospace << ", got" << Debug::hex << id << "and" << Debug::hex << generation), NodeHandle(id|(generation << Implementation::NodeHandleIdBits)));
}

/**
@brief Extract ID from a node handle
@m_since_latest

Expects that @p handle is not @ref NodeHandle::Null. Use
@ref nodeHandleGeneration() for extracting the generation and @ref nodeHandle()
for an inverse operation.
*/
constexpr UnsignedInt nodeHandleId(NodeHandle handle) {
    return (CORRADE_CONSTEXPR_DEBUG_ASSERT(handle != NodeHandle::Null,
        "Whee::nodeHandleId(): the handle is null"),
        UnsignedInt(handle) & ((1 << Implementation::NodeHandleIdBits) - 1));
}

/**
@brief Extract generation from a node handle
@m_since_latest

For @ref NodeHandle::Null returns @cpp 0 @ce. A valid handle has always a
non-zero generation. Use @ref nodeHandleId() for extracting the ID and
@ref nodeHandle() for an inverse operation.
*/
constexpr UnsignedInt nodeHandleGeneration(NodeHandle handle) {
    return UnsignedInt(handle) >> Implementation::NodeHandleIdBits;
}

namespace Implementation {
    enum: UnsignedInt {
        LayouterHandleIdBits = 8,
        LayouterHandleGenerationBits = 8
    };
}

/**
@brief Layouter handle
@m_since_latest

Uses 8 bits for storing an ID and 8 bits for a generation.
@see @ref AbstractUserInterface::createLayouter(),
    @ref AbstractUserInterface::removeLayouter(), @ref AbstractLayouter,
    @ref layouterHandle(), @ref layouterHandleId(),
    @ref layouterHandleGeneration()
*/
enum class LayouterHandle: UnsignedShort {
    Null = 0    /**< Null handle */
};

/**
@debugoperatorenum{LayouterHandle}
@m_since_latest
*/
MAGNUM_WHEE_EXPORT Debug& operator<<(Debug& debug, LayouterHandle value);

/**
@brief Compose a layouter handle from an ID and a generation
@m_since_latest

Expects that the ID fits into 8 bits and the generation into 8 bits. Use
@ref layouterHandleId() and @ref layouterHandleGeneration() for an inverse
operation.
*/
constexpr LayouterHandle layouterHandle(UnsignedInt id, UnsignedInt generation) {
    return (CORRADE_CONSTEXPR_DEBUG_ASSERT(id < (1 << Implementation::LayouterHandleIdBits) && generation < (1 << Implementation::LayouterHandleGenerationBits),
        "Whee::layouterHandle(): expected index to fit into" << Implementation::LayouterHandleIdBits << "bits and generation into" << Implementation::LayouterHandleGenerationBits << Debug::nospace << ", got" << Debug::hex << id << "and" << Debug::hex << generation), LayouterHandle(id|(generation << Implementation::LayouterHandleIdBits)));
}

/**
@brief Extract ID from a layouter handle
@m_since_latest

Expects that @p handle is not @ref LayouterHandle::Null. Use
@ref layouterHandleGeneration() for extracting the generation and
@ref layouterHandle() for an inverse operation.
*/
constexpr UnsignedInt layouterHandleId(LayouterHandle handle) {
    return (CORRADE_CONSTEXPR_DEBUG_ASSERT(handle != LayouterHandle::Null,
        "Whee::layouterHandleId(): the handle is null"),
        UnsignedInt(handle) & ((1 << Implementation::LayouterHandleIdBits) - 1));
}

/**
@brief Extract generation from a layouter handle
@m_since_latest

For @ref LayouterHandle::Null returns @cpp 0 @ce. A valid handle has always a
non-zero generation. Use @ref layouterHandleId() for extracting the ID and
@ref layouterHandle() for an inverse operation.
*/
constexpr UnsignedInt layouterHandleGeneration(LayouterHandle handle) {
    return UnsignedInt(handle) >> Implementation::LayouterHandleIdBits;
}

namespace Implementation {
    enum: UnsignedInt {
        LayouterDataHandleIdBits = 20,
        LayouterDataHandleGenerationBits = 12
    };
}

/**
@brief Layouter data handle
@m_since_latest

Uses 20 bits for storing an ID and 12 bits for a generation.
@see @ref AbstractLayouter::add(), @ref AbstractLayouter::remove(),
    @ref layouterDataHandle(), @ref layouterDataHandleId(),
    @ref layouterDataHandleGeneration()
*/
enum class LayouterDataHandle: UnsignedInt {
    Null = 0    /**< Null handle */
};

/**
@debugoperatorenum{LayouterDataHandle}
@m_since_latest
*/
MAGNUM_WHEE_EXPORT Debug& operator<<(Debug& debug, LayouterDataHandle value);

/**
@brief Compose a layouter data handle from an ID and a generation
@m_since_latest

Expects that the ID fits into 20 bits and the generation into 12 bits. Use
@ref layouterDataHandleId() and @ref layouterDataHandleGeneration() for an
inverse operation.
*/
constexpr LayouterDataHandle layouterDataHandle(UnsignedInt id, UnsignedInt generation) {
    return (CORRADE_CONSTEXPR_DEBUG_ASSERT(id < (1 << Implementation::LayouterDataHandleIdBits) && generation < (1 << Implementation::LayouterDataHandleGenerationBits),
        "Whee::layouterDataHandle(): expected index to fit into" << Implementation::LayouterDataHandleIdBits << "bits and generation into" << Implementation::LayouterDataHandleGenerationBits << Debug::nospace << ", got" << Debug::hex << id << "and" << Debug::hex << generation), LayouterDataHandle(id|(generation << Implementation::LayouterDataHandleIdBits)));
}

/**
@brief Extract ID from a layouter data handle
@m_since_latest

Expects that @p handle is not @ref LayouterDataHandle::Null. Use
@ref layouterDataHandleGeneration() for extracting the generation and
@ref layouterDataHandle() for an inverse operation.
*/
constexpr UnsignedInt layouterDataHandleId(LayouterDataHandle handle) {
    return (CORRADE_CONSTEXPR_DEBUG_ASSERT(handle != LayouterDataHandle::Null,
        "Whee::layouterDataHandleId(): the handle is null"),
        UnsignedInt(handle) & ((1 << Implementation::LayouterDataHandleIdBits) - 1));
}

/**
@brief Extract generation from a layouter data handle
@m_since_latest

For @ref LayouterDataHandle::Null returns @cpp 0 @ce. A valid handle has always
a non-zero generation. Use @ref layouterDataHandleId() for extracting the ID
and @ref layouterDataHandle() for an inverse operation.
*/
constexpr UnsignedInt layouterDataHandleGeneration(LayouterDataHandle handle) {
    return UnsignedInt(handle) >> Implementation::LayouterDataHandleIdBits;
}

/**
@brief Layout handle
@m_since_latest

A combination of a @ref LayouterHandle and a @ref LayouterDataHandle. Uses 8
bits for storing a layouter ID, 8 bits for a layouter generation, 20 bits for
storing a layouter data ID and 12 bits for a layouter data generation.
@see @ref AbstractLayouter::add(), @ref AbstractLayouter::remove(),
    @ref layoutHandle(), @ref layoutHandleId(),
    @ref layoutHandleGeneration()
*/
enum class LayoutHandle: UnsignedLong {
    Null = 0    /**< Null handle */
};

/**
@debugoperatorenum{LayoutHandle}
@m_since_latest
*/
MAGNUM_WHEE_EXPORT Debug& operator<<(Debug& debug, LayoutHandle value);

/**
@brief Compose a layout handle from a layouter handle, a layouter data ID and a layouter data generation
@m_since_latest

Expects that the ID fits into 20 bits and the generation into 12 bits. Use
@ref layoutHandleLayouter(), @ref layoutHandleId() and
@ref layoutHandleGeneration() for an inverse operation.
@see @ref layoutHandle(LayouterHandle, LayouterDataHandle),
    @ref layoutHandleData(), @ref layoutHandleLayouterId(),
    @ref layoutHandleLayouterGeneration()
*/
constexpr LayoutHandle layoutHandle(LayouterHandle layouterHandle, UnsignedInt id, UnsignedInt generation) {
    return (CORRADE_CONSTEXPR_DEBUG_ASSERT(id < (1 << Implementation::LayouterDataHandleIdBits) && generation < (1 << Implementation::LayouterDataHandleGenerationBits),
        "Whee::layoutHandle(): expected index to fit into" << Implementation::LayouterDataHandleIdBits << "bits and generation into" << Implementation::LayouterDataHandleGenerationBits << Debug::nospace << ", got" << Debug::hex << id << "and" << Debug::hex << generation), LayoutHandle(id|(UnsignedLong(generation) << Implementation::LayouterDataHandleIdBits)|(UnsignedLong(layouterHandle) << (Implementation::LayouterDataHandleIdBits + Implementation::LayouterDataHandleGenerationBits))));
}

/**
@brief Compose a layout handle from a layouter handle and a layouter data handle
@m_since_latest

Use @ref layoutHandleLayouter() and @ref layoutHandleData() for the inverse
operation.
@see @ref layoutHandle(LayouterHandle, LayouterDataHandle),
    @ref layoutHandleLayouterId(), @ref layoutHandleLayouterGeneration(),
    @ref layoutHandleId(), @ref layoutHandleGeneration()
*/
constexpr LayoutHandle layoutHandle(LayouterHandle layouterHandle, LayouterDataHandle layouterDataHandle) {
    return LayoutHandle((UnsignedLong(layouterHandle) << (Implementation::LayouterDataHandleIdBits + Implementation::LayouterDataHandleGenerationBits))|UnsignedLong(layouterDataHandle));
}

/**
@brief Extract layouter handle from a layout handle
@m_since_latest

Use @ref layoutHandleData() for extracting the layouter data handle and
@ref layoutHandle(LayouterHandle, LayouterDataHandle) for an inverse operation.
*/
constexpr LayouterHandle layoutHandleLayouter(LayoutHandle handle) {
    return LayouterHandle(UnsignedLong(handle) >> (Implementation::LayouterDataHandleIdBits + Implementation::LayouterDataHandleGenerationBits));
}

/**
@brief Extract layouter data handle from a layout handle
@m_since_latest

Use @ref layoutHandleLayouter() for extracting the layouter handle and
@ref layoutHandle(LayouterHandle, LayouterDataHandle) for an inverse operation.
*/
constexpr LayouterDataHandle layoutHandleData(LayoutHandle handle) {
    return LayouterDataHandle(UnsignedLong(handle));
}

/**
@brief Extract layouter ID from a layout handle
@m_since_latest

Expects that the layouter portion of @p handle is @ref LayouterHandle::Null.
Use @ref layoutHandleLayouterGeneration() for extracting the layouter
generation and @ref layouterHandle() together with @ref layoutHandle()
for an inverse operation.
@see @ref layoutHandleLayouter()
*/
constexpr UnsignedInt layoutHandleLayouterId(LayoutHandle handle) {
    return (CORRADE_CONSTEXPR_DEBUG_ASSERT(LayouterHandle(UnsignedLong(handle) >> (Implementation::LayouterDataHandleIdBits + Implementation::LayouterDataHandleGenerationBits)) != LayouterHandle::Null,
        "Whee::layoutHandleLayouterId(): the layouter portion of" << handle << "is null"),
        (UnsignedLong(handle) >> (Implementation::LayouterDataHandleIdBits + Implementation::LayouterDataHandleGenerationBits)) & ((1 << Implementation::LayouterHandleIdBits) - 1));
}

/**
@brief Extract layouter generation from a layout handle
@m_since_latest

If the layouter portion of the handle is @ref LayouterHandle::Null, returns
@cpp 0 @ce. A valid handle has always a non-zero generation. Use
@ref layoutHandleLayouterId() for extracting the ID and @ref layouterHandle()
together with @ref layoutHandle() for an inverse operation.
@see @ref layoutHandleLayouter()
*/
constexpr UnsignedInt layoutHandleLayouterGeneration(LayoutHandle handle) {
    return (UnsignedLong(handle) >> (Implementation::LayouterDataHandleIdBits + Implementation::LayouterDataHandleGenerationBits + Implementation::LayouterHandleIdBits)) & ((1 << Implementation::LayouterHandleGenerationBits) - 1);
}

/**
@brief Extract ID from a layout handle
@m_since_latest

Expects that the data portion of @p handle is not @ref LayouterDataHandle::Null.
Use @ref layoutHandleGeneration() for extracting the generation and
@ref layoutHandle(LayouterHandle, UnsignedInt, UnsignedInt) for an inverse
operation.
@see @ref layoutHandleData()
*/
constexpr UnsignedInt layoutHandleId(LayoutHandle handle) {
    return (CORRADE_CONSTEXPR_DEBUG_ASSERT(LayouterDataHandle(UnsignedLong(handle)) != LayouterDataHandle::Null,
        "Whee::layoutHandleId(): the data portion of" << handle << "is null"),
        UnsignedLong(handle) & ((1 << Implementation::LayouterDataHandleIdBits) - 1));
}

/**
@brief Extract generation from a layout handle
@m_since_latest

If the layouter data portion of the handle is @ref LayouterDataHandle::Null,
returns @cpp 0 @ce. A valid handle has always a non-zero generation. Use
@ref layoutHandleId() for extracting the ID and
@ref layoutHandle(LayouterHandle, UnsignedInt, UnsignedInt) for an inverse
operation.
@see @ref layoutHandleData()
*/
constexpr UnsignedInt layoutHandleGeneration(LayoutHandle handle) {
    return (UnsignedLong(handle) >> Implementation::LayouterDataHandleIdBits) & ((1 << Implementation::LayouterDataHandleGenerationBits) - 1);
}

namespace Implementation {
    enum: UnsignedInt {
        AnimatorHandleIdBits = 8,
        AnimatorHandleGenerationBits = 8
    };
}

/**
@brief Animator handle
@m_since_latest

Uses 8 bits for storing an ID and 8 bits for a generation.
@see @ref AbstractUserInterface::createAnimator(),
    @ref AbstractUserInterface::removeAnimator(), @ref AbstractAnimator,
    @ref animatorHandle(), @ref animatorHandleId(),
    @ref animatorHandleGeneration()
*/
enum class AnimatorHandle: UnsignedShort {
    Null = 0    /**< Null handle */
};

/**
@debugoperatorenum{AnimatorHandle}
@m_since_latest
*/
MAGNUM_WHEE_EXPORT Debug& operator<<(Debug& debug, AnimatorHandle value);

/**
@brief Compose an animator handle from an ID and a generation
@m_since_latest

Expects that the ID fits into 8 bits and the generation into 8 bits. Use
@ref animatorHandleId() and @ref animatorHandleGeneration() for an inverse
operation.
*/
constexpr AnimatorHandle animatorHandle(UnsignedInt id, UnsignedInt generation) {
    return (CORRADE_CONSTEXPR_DEBUG_ASSERT(id < (1 << Implementation::AnimatorHandleIdBits) && generation < (1 << Implementation::AnimatorHandleGenerationBits),
        "Whee::animatorHandle(): expected index to fit into" << Implementation::AnimatorHandleIdBits << "bits and generation into" << Implementation::AnimatorHandleGenerationBits << Debug::nospace << ", got" << Debug::hex << id << "and" << Debug::hex << generation), AnimatorHandle(id|(generation << Implementation::AnimatorHandleIdBits)));
}

/**
@brief Extract ID from an animator handle
@m_since_latest

Expects that @p handle is not @ref AnimatorHandle::Null. Use
@ref animatorHandleGeneration() for extracting the generation and
@ref animatorHandle() for an inverse operation.
*/
constexpr UnsignedInt animatorHandleId(AnimatorHandle handle) {
    return (CORRADE_CONSTEXPR_DEBUG_ASSERT(handle != AnimatorHandle::Null,
        "Whee::animatorHandleId(): the handle is null"),
        UnsignedInt(handle) & ((1 << Implementation::AnimatorHandleIdBits) - 1));
}

/**
@brief Extract generation from an animator handle
@m_since_latest

For @ref AnimatorHandle::Null returns @cpp 0 @ce. A valid handle has always a
non-zero generation. Use @ref animatorHandleId() for extracting the ID and
@ref animatorHandle() for an inverse operation.
*/
constexpr UnsignedInt animatorHandleGeneration(AnimatorHandle handle) {
    return UnsignedInt(handle) >> Implementation::AnimatorHandleIdBits;
}

namespace Implementation {
    enum: UnsignedInt {
        AnimatorDataHandleIdBits = 20,
        AnimatorDataHandleGenerationBits = 12
    };
}

/**
@brief Animator data handle
@m_since_latest

Uses 20 bits for storing an ID and 12 bits for a generation.
@see @ref AbstractAnimator::create(), @ref AbstractAnimator::remove(),
    @ref animatorDataHandle(), @ref animatorDataHandleId(),
    @ref animatorDataHandleGeneration()
*/
enum class AnimatorDataHandle: UnsignedInt {
    Null = 0    /**< Null handle */
};

/**
@debugoperatorenum{AnimatorDataHandle}
@m_since_latest
*/
MAGNUM_WHEE_EXPORT Debug& operator<<(Debug& debug, AnimatorDataHandle value);

/**
@brief Compose an animator data handle from an ID and a generation
@m_since_latest

Expects that the ID fits into 20 bits and the generation into 12 bits. Use
@ref animatorDataHandleId() and @ref animatorDataHandleGeneration() for an
inverse operation.
*/
constexpr AnimatorDataHandle animatorDataHandle(UnsignedInt id, UnsignedInt generation) {
    return (CORRADE_CONSTEXPR_DEBUG_ASSERT(id < (1 << Implementation::AnimatorDataHandleIdBits) && generation < (1 << Implementation::AnimatorDataHandleGenerationBits),
        "Whee::animatorDataHandle(): expected index to fit into" << Implementation::AnimatorDataHandleIdBits << "bits and generation into" << Implementation::AnimatorDataHandleGenerationBits << Debug::nospace << ", got" << Debug::hex << id << "and" << Debug::hex << generation), AnimatorDataHandle(id|(generation << Implementation::AnimatorDataHandleIdBits)));
}

/**
@brief Extract ID from an animator data handle
@m_since_latest

Expects that @p handle is not @ref AnimatorDataHandle::Null. Use
@ref animatorDataHandleGeneration() for extracting the generation and
@ref animatorDataHandle() for an inverse operation.
*/
constexpr UnsignedInt animatorDataHandleId(AnimatorDataHandle handle) {
    return (CORRADE_CONSTEXPR_DEBUG_ASSERT(handle != AnimatorDataHandle::Null,
        "Whee::animatorDataHandleId(): the handle is null"),
        UnsignedInt(handle) & ((1 << Implementation::AnimatorDataHandleIdBits) - 1));
}

/**
@brief Extract generation from an animator data handle
@m_since_latest

For @ref AnimatorDataHandle::Null returns @cpp 0 @ce. A valid handle has always
a non-zero generation. Use @ref animatorDataHandleId() for extracting the ID
and @ref animatorDataHandle() for an inverse operation.
*/
constexpr UnsignedInt animatorDataHandleGeneration(AnimatorDataHandle handle) {
    return UnsignedInt(handle) >> Implementation::AnimatorDataHandleIdBits;
}

/**
@brief Animation handle
@m_since_latest

A combination of an @ref AnimatorHandle and an @ref AnimatorDataHandle.
Uses 8 bits for storing an animator ID, 8 bits for an animator generation, 20
bits for storing an animator data ID and 12 bits for an animator data
generation.
@see @ref AbstractAnimator::create(), @ref AbstractAnimator::remove(),
    @ref animationHandle(), @ref animationHandleId(),
    @ref animationHandleGeneration()
*/
enum class AnimationHandle: UnsignedLong {
    Null = 0    /**< Null handle */
};

/**
@debugoperatorenum{AnimationHandle}
@m_since_latest
*/
MAGNUM_WHEE_EXPORT Debug& operator<<(Debug& debug, AnimationHandle value);

/**
@brief Compose an animation handle from an animator handle, an animator data ID and an animator data generation
@m_since_latest

Expects that the ID fits into 20 bits and the generation into 12 bits. Use
@ref animationHandleAnimator(), @ref animationHandleId() and
@ref animationHandleGeneration() for an inverse operation.
@see @ref animationHandle(AnimatorHandle, AnimatorDataHandle),
    @ref animationHandleData(), @ref animationHandleAnimatorId(),
    @ref animationHandleAnimatorGeneration()
*/
constexpr AnimationHandle animationHandle(AnimatorHandle animatorHandle, UnsignedInt id, UnsignedInt generation) {
    return (CORRADE_CONSTEXPR_DEBUG_ASSERT(id < (1 << Implementation::AnimatorDataHandleIdBits) && generation < (1 << Implementation::AnimatorDataHandleGenerationBits),
        "Whee::animationHandle(): expected index to fit into" << Implementation::AnimatorDataHandleIdBits << "bits and generation into" << Implementation::AnimatorDataHandleGenerationBits << Debug::nospace << ", got" << Debug::hex << id << "and" << Debug::hex << generation), AnimationHandle(id|(UnsignedLong(generation) << Implementation::AnimatorDataHandleIdBits)|(UnsignedLong(animatorHandle) << (Implementation::AnimatorDataHandleIdBits + Implementation::AnimatorDataHandleGenerationBits))));
}

/**
@brief Compose a animation handle from an animator handle and an animator data handle
@m_since_latest

Use @ref animationHandleAnimator() and @ref animationHandleData() for the
inverse operation.
@see @ref animationHandle(AnimatorHandle, AnimatorDataHandle),
    @ref animationHandleAnimatorId(), @ref animationHandleAnimatorGeneration(),
    @ref animationHandleId(), @ref animationHandleGeneration()
*/
constexpr AnimationHandle animationHandle(AnimatorHandle animatorHandle, AnimatorDataHandle animatorDataHandle) {
    return AnimationHandle((UnsignedLong(animatorHandle) << (Implementation::AnimatorDataHandleIdBits + Implementation::AnimatorDataHandleGenerationBits))|UnsignedLong(animatorDataHandle));
}

/**
@brief Extract animator handle from an animation handle
@m_since_latest

Use @ref animationHandleData() for extracting the animator data handle and
@ref animationHandle(AnimatorHandle, AnimatorDataHandle) for an inverse
operation.
*/
constexpr AnimatorHandle animationHandleAnimator(AnimationHandle handle) {
    return AnimatorHandle(UnsignedLong(handle) >> (Implementation::AnimatorDataHandleIdBits + Implementation::AnimatorDataHandleGenerationBits));
}

/**
@brief Extract animator data handle from an animation handle
@m_since_latest

Use @ref animationHandleAnimator() for extracting the animator handle and
@ref animationHandle(AnimatorHandle, AnimatorDataHandle) for an inverse
operation.
*/
constexpr AnimatorDataHandle animationHandleData(AnimationHandle handle) {
    return AnimatorDataHandle(UnsignedLong(handle));
}

/**
@brief Extract animator ID from an animation handle
@m_since_latest

Expects that the animator portion of @p handle is not @ref AnimatorHandle::Null.
Use @ref animationHandleAnimatorGeneration() for extracting the animator
generation and @ref animatorHandle() together with @ref animationHandle() for
an inverse operation.
@see @ref animationHandleAnimator()
*/
constexpr UnsignedInt animationHandleAnimatorId(AnimationHandle handle) {
    return (CORRADE_CONSTEXPR_DEBUG_ASSERT(AnimatorHandle(UnsignedLong(handle) >> (Implementation::AnimatorDataHandleIdBits + Implementation::AnimatorDataHandleGenerationBits)) != AnimatorHandle::Null,
        "Whee::animationHandleAnimatorId(): the animator portion of" << handle << "is null"),
        (UnsignedLong(handle) >> (Implementation::AnimatorDataHandleIdBits + Implementation::AnimatorDataHandleGenerationBits)) & ((1 << Implementation::AnimatorHandleIdBits) - 1));
}

/**
@brief Extract animator generation from an animation handle
@m_since_latest

If the animator portion of the handle is @ref AnimatorHandle::Null, returns
@cpp 0 @ce. A valid handle has always a non-zero generation. Use
@ref animationHandleAnimatorId() for extracting the ID and
@ref animatorHandle() together with @ref animationHandle() for an inverse
operation.
@see @ref animationHandleAnimator()
*/
constexpr UnsignedInt animationHandleAnimatorGeneration(AnimationHandle handle) {
    return (UnsignedLong(handle) >> (Implementation::AnimatorDataHandleIdBits + Implementation::AnimatorDataHandleGenerationBits + Implementation::AnimatorHandleIdBits)) & ((1 << Implementation::AnimatorHandleGenerationBits) - 1);
}

/**
@brief Extract ID from an animation handle
@m_since_latest

Expects that the data portion of @p handle is not @ref AnimatorDataHandle::Null.
Use @ref animationHandleGeneration() for extracting the generation and
@ref animationHandle(AnimatorHandle, UnsignedInt, UnsignedInt) for an inverse
operation.
@see @ref animationHandleData()
*/
constexpr UnsignedInt animationHandleId(AnimationHandle handle) {
    return (CORRADE_CONSTEXPR_DEBUG_ASSERT(AnimatorDataHandle(UnsignedLong(handle)) != AnimatorDataHandle::Null,
        "Whee::animationHandleId(): the data portion of" << handle << "is null"),
        UnsignedLong(handle) & ((1 << Implementation::AnimatorDataHandleIdBits) - 1));
}

/**
@brief Extract generation from an animation handle
@m_since_latest

If the animator data portion of the handle is @ref AnimatorDataHandle::Null,
returns @cpp 0 @ce. A valid handle has always a non-zero generation. Use
@ref animationHandleId() for extracting the ID and
@ref animationHandle(AnimatorHandle, UnsignedInt, UnsignedInt) for an inverse
operation.
@see @ref animationHandleData()
*/
constexpr UnsignedInt animationHandleGeneration(AnimationHandle handle) {
    return (UnsignedLong(handle) >> Implementation::AnimatorDataHandleIdBits) & ((1 << Implementation::AnimatorDataHandleGenerationBits) - 1);
}

}}

#endif
