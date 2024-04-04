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
 * @brief Handle @ref Magnum::Whee::LayerHandle, @ref Magnum::Whee::LayerDataHandle, @ref Magnum::Whee::DataHandle, @ref Magnum::Whee::NodeHandle, @ref Magnum::Whee::LayouterHandle, @ref Magnum::Whee::LayouterDataHandle, @ref Magnum::Whee::LayoutHandle, function @ref Magnum::Whee::layerHandle(), @ref Magnum::Whee::layerHandleId(), @ref Magnum::Whee::layerHandleGeneration(), @ref Magnum::Whee::layerDataHandle(), @ref Magnum::Whee::layerDataHandleId(), @ref Magnum::Whee::layerDataHandleGeneration(), @ref Magnum::Whee::dataHandle(), @ref Magnum::Whee::dataHandleLayer(), @ref Magnum::Whee::dataHandleData(), @ref Magnum::Whee::dataHandleLayerId(), @ref Magnum::Whee::dataHandleLayerGeneration(), @ref Magnum::Whee::dataHandleId(), @ref Magnum::Whee::dataHandleGeneration(), @ref Magnum::Whee::nodeHandle(), @ref Magnum::Whee::nodeHandleId(), @ref Magnum::Whee::nodeHandleGeneration(), @ref Magnum::Whee::layouterHandle(), @ref Magnum::Whee::layouterHandleId(), @ref Magnum::Whee::layouterHandleGeneration(), @ref Magnum::Whee::layouterDataHandle(), @ref Magnum::Whee::layouterDataHandleId(), @ref Magnum::Whee::layouterDataHandleGeneration(), @ref Magnum::Whee::layoutHandle(), @ref Magnum::Whee::layoutHandleLayouter(), @ref Magnum::Whee::layoutHandleData(), @ref Magnum::Whee::layoutHandleLayouterId(), @ref Magnum::Whee::layoutHandleLayouterGeneration(), @ref Magnum::Whee::layoutHandleId(), @ref Magnum::Whee::layoutHandleGeneration()
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

For @ref LayouterHandle::Null returns @cpp 0 @ce. Use
@ref layouterHandleGeneration() for extracting the generation and
@ref layouterHandle() for an inverse operation.
*/
constexpr UnsignedInt layouterHandleId(LayouterHandle handle) {
    return UnsignedInt(handle) & ((1 << Implementation::LayouterHandleIdBits) - 1);
}

/**
@brief Extract generation from a layouter handle
@m_since_latest

For @ref LayouterHandle::Null returns @cpp 0 @ce. Use @ref layouterHandleId()
for extracting the ID and @ref layouterHandle() for an inverse operation.
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

For @ref LayouterDataHandle::Null returns @cpp 0 @ce. Use
@ref layouterDataHandleGeneration() for extracting the generation and
@ref layouterDataHandle() for an inverse operation.
*/
constexpr UnsignedInt layouterDataHandleId(LayouterDataHandle handle) {
    return UnsignedInt(handle) & ((1 << Implementation::LayouterDataHandleIdBits) - 1);
}

/**
@brief Extract generation from a layouter data handle
@m_since_latest

For @ref LayouterDataHandle::Null returns @cpp 0 @ce. Use
@ref layouterDataHandleId() for extracting the ID and @ref layouterDataHandle()
for an inverse operation.
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

If the layouter portion of the handle is @ref LayouterHandle::Null, returns
@cpp 0 @ce. Use @ref layoutHandleLayouterGeneration() for extracting the
layouter generation and @ref layouterHandle() together with @ref layoutHandle()
for an inverse operation.
@see @ref layoutHandleLayouter()
*/
constexpr UnsignedInt layoutHandleLayouterId(LayoutHandle handle) {
    return (UnsignedLong(handle) >> (Implementation::LayouterDataHandleIdBits + Implementation::LayouterDataHandleGenerationBits)) & ((1 << Implementation::LayouterHandleIdBits) - 1);
}

/**
@brief Extract layouter generation from a layout handle
@m_since_latest

If the layouter portion of the handle is @ref LayouterHandle::Null, returns
@cpp 0 @ce. Use @ref layoutHandleLayouterId() for extracting the ID and
@ref layouterHandle() together with @ref layoutHandle() for an inverse
operation.
@see @ref layoutHandleLayouter()
*/
constexpr UnsignedInt layoutHandleLayouterGeneration(LayoutHandle handle) {
    return (UnsignedLong(handle) >> (Implementation::LayouterDataHandleIdBits + Implementation::LayouterDataHandleGenerationBits + Implementation::LayouterHandleIdBits)) & ((1 << Implementation::LayouterHandleGenerationBits) - 1);
}

/**
@brief Extract ID from a layout handle
@m_since_latest

If the layouter data portion of the handle is @ref LayouterDataHandle::Null,
returns @cpp 0 @ce. Use @ref layoutHandleGeneration() for extracting the
generation and @ref layoutHandle(LayouterHandle, UnsignedInt, UnsignedInt) for
an inverse operation.
@see @ref layoutHandleData()
*/
constexpr UnsignedInt layoutHandleId(LayoutHandle handle) {
    return UnsignedLong(handle) & ((1 << Implementation::LayouterDataHandleIdBits) - 1);
}

/**
@brief Extract generation from a layout handle
@m_since_latest

If the layouter data portion of the handle is @ref LayouterDataHandle::Null,
returns @cpp 0 @ce. Use @ref layoutHandleId() for extracting the ID and
@ref layoutHandle(LayouterHandle, UnsignedInt, UnsignedInt) for an inverse
operation.
@see @ref layoutHandleData()
*/
constexpr UnsignedInt layoutHandleGeneration(LayoutHandle handle) {
    return (UnsignedLong(handle) >> Implementation::LayouterDataHandleIdBits) & ((1 << Implementation::LayouterDataHandleGenerationBits) - 1);
}

}}

#endif
