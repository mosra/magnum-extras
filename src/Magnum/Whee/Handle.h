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
 * @brief Handle @ref Magnum::Whee::LayerHandle, @ref Magnum::Whee::LayerDataHandle, @ref Magnum::Whee::DataHandle, @ref Magnum::Whee::NodeHandle, function @ref Magnum::Whee::layerHandle(), @ref Magnum::Whee::layerHandleId(), @ref Magnum::Whee::layerHandleGeneration(), @ref Magnum::Whee::layerDataHandle(), @ref Magnum::Whee::layerDataHandleId(), @ref Magnum::Whee::layerDataHandleGeneration(), @ref Magnum::Whee::dataHandle(), @ref Magnum::Whee::dataHandleLayer(), @ref Magnum::Whee::dataHandleData(), @ref Magnum::Whee::dataHandleLayerId(), @ref Magnum::Whee::dataHandleLayerGeneration(), @ref Magnum::Whee::dataHandleId(), @ref Magnum::Whee::dataHandleGeneration(), @ref Magnum::Whee::nodeHandle(), @ref Magnum::Whee::nodeHandleId(), @ref Magnum::Whee::nodeHandleGeneration()
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

For @ref LayerHandle::Null returns @cpp 0 @ce. Use @ref layerHandleGeneration()
for extracting the generation and @ref layerHandle() for an inverse operation.
*/
constexpr UnsignedInt layerHandleId(LayerHandle handle) {
    return UnsignedInt(handle) & ((1 << Implementation::LayerHandleIdBits) - 1);
}

/**
@brief Extract generation from a layer handle
@m_since_latest

For @ref LayerHandle::Null returns @cpp 0 @ce. Use @ref layerHandleId() for
extracting the ID and @ref layerHandle() for an inverse operation.
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

For @ref LayerDataHandle::Null returns @cpp 0 @ce. Use
@ref layerDataHandleGeneration() for extracting the generation and
@ref layerDataHandle() for an inverse operation.
*/
constexpr UnsignedInt layerDataHandleId(LayerDataHandle handle) {
    return UnsignedInt(handle) & ((1 << Implementation::LayerDataHandleIdBits) - 1);
}

/**
@brief Extract generation from a layer data handle
@m_since_latest

For @ref LayerDataHandle::Null returns @cpp 0 @ce. Use @ref layerDataHandleId()
for extracting the ID and @ref layerDataHandle() for an inverse operation.
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

If the layer portion of the handle is @ref LayerHandle::Null, returns
@cpp 0 @ce. Use @ref dataHandleLayerGeneration() for extracting the layer
generation and @ref layerHandle() together with @ref dataHandle() for an
inverse operation.
@see @ref dataHandleLayer()
*/
constexpr UnsignedInt dataHandleLayerId(DataHandle handle) {
    return (UnsignedLong(handle) >> (Implementation::LayerDataHandleIdBits + Implementation::LayerDataHandleGenerationBits)) & ((1 << Implementation::LayerHandleIdBits) - 1);
}

/**
@brief Extract layer generation from a data handle
@m_since_latest

If the layer portion of the handle is @ref LayerHandle::Null, returns
@cpp 0 @ce. Use @ref dataHandleLayerId() for extracting the ID and
@ref layerHandle() together with @ref dataHandle() for an inverse operation.
@see @ref dataHandleLayer()
*/
constexpr UnsignedInt dataHandleLayerGeneration(DataHandle handle) {
    return (UnsignedLong(handle) >> (Implementation::LayerDataHandleIdBits + Implementation::LayerDataHandleGenerationBits + Implementation::LayerHandleIdBits)) & ((1 << Implementation::LayerHandleGenerationBits) - 1);
}

/**
@brief Extract ID from a data handle
@m_since_latest

If the data portion of the handle is @ref LayerDataHandle::Null, returns
@cpp 0 @ce. Use @ref dataHandleGeneration() for extracting the generation and
@ref dataHandle(LayerHandle, UnsignedInt, UnsignedInt) for an inverse
operation.
@see @ref dataHandleData()
*/
constexpr UnsignedInt dataHandleId(DataHandle handle) {
    return UnsignedLong(handle) & ((1 << Implementation::LayerDataHandleIdBits) - 1);
}

/**
@brief Extract generation from a data handle
@m_since_latest

If the data portion of the handle is @ref LayerDataHandle::Null, returns
@cpp 0 @ce. Use @ref dataHandleId() for extracting the ID and
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

For @ref NodeHandle::Null returns @cpp 0 @ce. Use @ref nodeHandleGeneration()
for extracting the generation and @ref nodeHandle() for an inverse operation.
*/
constexpr UnsignedInt nodeHandleId(NodeHandle handle) {
    return UnsignedInt(handle) & ((1 << Implementation::NodeHandleIdBits) - 1);
}

/**
@brief Extract generation from a node handle
@m_since_latest

For @ref NodeHandle::Null returns @cpp 0 @ce. Use @ref nodeHandleId() for
extracting the ID and @ref nodeHandle() for an inverse operation.
*/
constexpr UnsignedInt nodeHandleGeneration(NodeHandle handle) {
    return UnsignedInt(handle) >> Implementation::NodeHandleIdBits;
}

}}

#endif
