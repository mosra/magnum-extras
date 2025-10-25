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

#include "Handle.h"

#include <Corrade/Utility/Debug.h>

namespace Magnum { namespace Ui {

Debug& operator<<(Debug& debug, const LayerHandle value) {
    const bool packed = debug.immediateFlags() >= Debug::Flag::Packed;

    if(value == LayerHandle::Null)
        return debug << (packed ? "Null" : "Ui::LayerHandle::Null");

    /* ID extraction is copy of layerHandleId() because it asserts if the
       generation is 0, and the assert calls into this debug printer, leading
       to infinite recursion */
    return debug << (packed ? "{" : "Ui::LayerHandle(") << Debug::nospace << Debug::hex << (UnsignedInt(value) & ((1 << Implementation::LayerHandleIdBits) - 1)) << Debug::nospace << "," << Debug::hex << layerHandleGeneration(value) << Debug::nospace << (packed ? "}" : ")");
}

Debug& operator<<(Debug& debug, const LayerDataHandle value) {
    const bool packed = debug.immediateFlags() >= Debug::Flag::Packed;

    if(value == LayerDataHandle::Null)
        return debug << (packed ? "Null" : "Ui::LayerDataHandle::Null");

    /* ID extraction is copy of layerDataHandleId() because it asserts if the
       generation is 0, and the assert calls into this debug printer, leading
       to infinite recursion */
    return debug << (packed ? "{" : "Ui::LayerDataHandle(") << Debug::nospace << Debug::hex << (UnsignedInt(value) & ((1 << Implementation::LayerDataHandleIdBits) - 1)) << Debug::nospace << "," << Debug::hex << layerDataHandleGeneration(value) << Debug::nospace << (packed ? "}" : ")");
}

Debug& operator<<(Debug& debug, const DataHandle value) {
    const bool packed = debug.immediateFlags() >= Debug::Flag::Packed;

    if(value == DataHandle::Null)
        return debug << (packed ? "Null" : "Ui::DataHandle::Null");

    debug << (packed ? "{" : "Ui::DataHandle(") << Debug::nospace;
    if(dataHandleLayer(value) == LayerHandle::Null)
        debug << "Null,";
    else
        /* ID extraction is copy of dataHandleLayerId() because it asserts if
           the generation is 0, and the assert calls into this debug printer,
           leading to infinite recursion */
        debug << "{" << Debug::nospace << Debug::hex << ((UnsignedLong(value) >> (Implementation::LayerDataHandleIdBits + Implementation::LayerDataHandleGenerationBits)) & ((1 << Implementation::LayerHandleIdBits) - 1)) << Debug::nospace << "," << Debug::hex << dataHandleLayerGeneration(value) << Debug::nospace << "},";

    if(dataHandleData(value) == LayerDataHandle::Null)
        debug << (packed ? "Null}" : "Null)");
    else
        /* ID extraction is copy of dataHandleId() because it asserts if the
           generation is 0, and the assert calls into this debug printer,
           leading to infinite recursion */
        debug << "{" << Debug::nospace << Debug::hex << (UnsignedLong(value) & ((1 << Implementation::LayerDataHandleIdBits) - 1)) << Debug::nospace << "," << Debug::hex << dataHandleGeneration(value) << Debug::nospace << (packed ? "}}" : "})");

    return debug;
}

Debug& operator<<(Debug& debug, const NodeHandle value) {
    const bool packed = debug.immediateFlags() >= Debug::Flag::Packed;

    if(value == NodeHandle::Null)
        return debug << (packed ? "Null" : "Ui::NodeHandle::Null");

    /* ID extraction is copy of nodeHandleId() because it asserts if the
       generation is 0, and the assert calls into this debug printer, leading
       to infinite recursion */
    return debug << (packed ? "{" : "Ui::NodeHandle(") << Debug::nospace << Debug::hex << (UnsignedInt(value) & ((1 << Implementation::NodeHandleIdBits) - 1)) << Debug::nospace << "," << Debug::hex << nodeHandleGeneration(value) << Debug::nospace << (packed ? "}" : ")");
}

Debug& operator<<(Debug& debug, const LayouterHandle value) {
    const bool packed = debug.immediateFlags() >= Debug::Flag::Packed;

    if(value == LayouterHandle::Null)
        return debug << (packed ? "Null" : "Ui::LayouterHandle::Null");

    /* ID extraction is copy of layouterHandleId() because it asserts if the
       generation is 0, and the assert calls into this debug printer, leading
       to infinite recursion */
    return debug << (packed ? "{" : "Ui::LayouterHandle(") << Debug::nospace << Debug::hex << (UnsignedInt(value) & ((1 << Implementation::LayouterHandleIdBits) - 1)) << Debug::nospace << "," << Debug::hex << layouterHandleGeneration(value) << Debug::nospace << (packed ? "}" : ")");
}

Debug& operator<<(Debug& debug, const LayouterDataHandle value) {
    const bool packed = debug.immediateFlags() >= Debug::Flag::Packed;

    if(value == LayouterDataHandle::Null)
        return debug << (packed ? "Null" : "Ui::LayouterDataHandle::Null");

    /* ID extraction is copy of layouterDataHandleId() because it asserts if
       the generation is 0, and the assert calls into this debug printer,
       leading to infinite recursion */
    return debug << (packed ? "{" : "Ui::LayouterDataHandle(") << Debug::nospace << Debug::hex << (UnsignedInt(value) & ((1 << Implementation::LayouterDataHandleIdBits) - 1)) << Debug::nospace << "," << Debug::hex << layouterDataHandleGeneration(value) << Debug::nospace << (packed ? "}" : ")");
}

Debug& operator<<(Debug& debug, const LayoutHandle value) {
    const bool packed = debug.immediateFlags() >= Debug::Flag::Packed;

    if(value == LayoutHandle::Null)
        return debug << (packed ? "Null" : "Ui::LayoutHandle::Null");

    debug << (packed ? "{" : "Ui::LayoutHandle(") << Debug::nospace;
    if(layoutHandleLayouter(value) == LayouterHandle::Null)
        debug << "Null,";
    else
        /* ID extraction is copy of layoutHandleLayouterId() because it asserts
           if the generation is 0, and the assert calls into this debug
           printer, leading to infinite recursion */
        debug << "{" << Debug::nospace << Debug::hex << ((UnsignedLong(value) >> (Implementation::LayouterDataHandleIdBits + Implementation::LayouterDataHandleGenerationBits)) & ((1 << Implementation::LayouterHandleIdBits) - 1)) << Debug::nospace << "," << Debug::hex << layoutHandleLayouterGeneration(value) << Debug::nospace << "},";

    if(layoutHandleData(value) == LayouterDataHandle::Null)
        debug << (packed ? "Null}" : "Null)");
    else
        /* ID extraction is copy of layoutHandleId() because it asserts if the
           generation is 0, and the assert calls into this debug printer,
           leading to infinite recursion */
        debug << "{" << Debug::nospace << Debug::hex << (UnsignedLong(value) & ((1 << Implementation::LayouterDataHandleIdBits) - 1)) << Debug::nospace << "," << Debug::hex << layoutHandleGeneration(value) << Debug::nospace << (packed ? "}}" : "})");

    return debug;
}

Debug& operator<<(Debug& debug, const AnimatorHandle value) {
    const bool packed = debug.immediateFlags() >= Debug::Flag::Packed;

    if(value == AnimatorHandle::Null)
        return debug << (packed ? "Null" : "Ui::AnimatorHandle::Null");

    /* ID extraction is copy of animatorHandleId() because it asserts if the
       generation is 0, and the assert calls into this debug printer, leading
       to infinite recursion */
    return debug << (packed ? "{" : "Ui::AnimatorHandle(") << Debug::nospace << Debug::hex << (UnsignedInt(value) & ((1 << Implementation::AnimatorHandleIdBits) - 1)) << Debug::nospace << "," << Debug::hex << animatorHandleGeneration(value) << Debug::nospace << (packed ? "}" : ")");
}

Debug& operator<<(Debug& debug, const AnimatorDataHandle value) {
    const bool packed = debug.immediateFlags() >= Debug::Flag::Packed;

    if(value == AnimatorDataHandle::Null)
        return debug << (packed ? "Null" : "Ui::AnimatorDataHandle::Null");

    /* ID extraction is copy of animatorDataHandleId() because it asserts if
       the generation is 0, and the assert calls into this debug printer,
       leading to infinite recursion */
    return debug << (packed ? "{" : "Ui::AnimatorDataHandle(") << Debug::nospace << Debug::hex << (UnsignedInt(value) & ((1 << Implementation::AnimatorDataHandleIdBits) - 1)) << Debug::nospace << "," << Debug::hex << animatorDataHandleGeneration(value) << Debug::nospace << (packed ? "}" : ")");
}

Debug& operator<<(Debug& debug, const AnimationHandle value) {
    const bool packed = debug.immediateFlags() >= Debug::Flag::Packed;

    if(value == AnimationHandle::Null)
        return debug << (packed ? "Null" : "Ui::AnimationHandle::Null");

    debug << (packed ? "{" : "Ui::AnimationHandle(") << Debug::nospace;
    if(animationHandleAnimator(value) == AnimatorHandle::Null)
        debug << "Null,";
    else
        /* ID extraction is copy of animationHandleAnimatorId() because it
           asserts if the generation is 0, and the assert calls into this debug
           printer, leading to infinite recursion */
        debug << "{" << Debug::nospace << Debug::hex << ((UnsignedLong(value) >> (Implementation::AnimatorDataHandleIdBits + Implementation::AnimatorDataHandleGenerationBits)) & ((1 << Implementation::AnimatorHandleIdBits) - 1)) << Debug::nospace << "," << Debug::hex << animationHandleAnimatorGeneration(value) << Debug::nospace << "},";

    if(animationHandleData(value) == AnimatorDataHandle::Null)
        debug << (packed ? "Null}" : "Null)");
    else
        /* ID extraction is copy of animationHandleId() because it asserts if
           the generation is 0, and the assert calls into this debug printer,
           leading to infinite recursion */
        debug << "{" << Debug::nospace << Debug::hex << (UnsignedLong(value) & ((1 << Implementation::AnimatorDataHandleIdBits) - 1)) << Debug::nospace << "," << Debug::hex << animationHandleGeneration(value) << Debug::nospace << (packed ? "}}" : "})");

    return debug;
}

}}
