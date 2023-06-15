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

#include "Handle.h"

#include <Corrade/Utility/Debug.h>

namespace Magnum { namespace Whee {

Debug& operator<<(Debug& debug, const LayerHandle value) {
    if(value == LayerHandle::Null)
        return debug << "Whee::LayerHandle::Null";
    return debug << "Whee::LayerHandle(" << Debug::nospace << Debug::hex << layerHandleId(value) << Debug::nospace << "," << Debug::hex << layerHandleGeneration(value) << Debug::nospace << ")";
}

Debug& operator<<(Debug& debug, const LayerDataHandle value) {
    if(value == LayerDataHandle::Null)
        return debug << "Whee::LayerDataHandle::Null";
    return debug << "Whee::LayerDataHandle(" << Debug::nospace << Debug::hex << layerDataHandleId(value) << Debug::nospace << "," << Debug::hex << layerDataHandleGeneration(value) << Debug::nospace << ")";
}

Debug& operator<<(Debug& debug, const DataHandle value) {
    if(value == DataHandle::Null)
        return debug << "Whee::DataHandle::Null";

    debug << "Whee::DataHandle(" << Debug::nospace;
    if(dataHandleLayer(value) == LayerHandle::Null)
        debug << "Null,";
    else
        debug << "{" << Debug::nospace << Debug::hex << dataHandleLayerId(value) << Debug::nospace << "," << Debug::hex << dataHandleLayerGeneration(value) << Debug::nospace << "},";

    if(dataHandleData(value) == LayerDataHandle::Null)
        debug << "Null)";
    else
        debug << "{" << Debug::nospace << Debug::hex << dataHandleId(value) << Debug::nospace << "," << Debug::hex << dataHandleGeneration(value) << Debug::nospace << "})";

    return debug;
}

Debug& operator<<(Debug& debug, const NodeHandle value) {
    if(value == NodeHandle::Null)
        return debug << "Whee::NodeHandle::Null";
    return debug << "Whee::NodeHandle(" << Debug::nospace << Debug::hex << nodeHandleId(value) << Debug::nospace << "," << Debug::hex << nodeHandleGeneration(value) << Debug::nospace << ")";
}

}}
