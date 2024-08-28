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

#include <sstream> /** @todo remove once Debug is stream-free */
#include <Corrade/Containers/BitArrayView.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/StringStl.h> /** @todo remove once Debug is stream-free */
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/String.h>
#include <Corrade/Utility/DebugStl.h> /** @todo remove once Debug is stream-free */
#include <Magnum/Math/Time.h>

#include "Magnum/Whee/AbstractVisualLayer.h"
#include "Magnum/Whee/AbstractVisualLayerAnimator.h"
#include "Magnum/Whee/Handle.h"
#include "Magnum/Whee/Implementation/abstractVisualLayerAnimatorState.h"

namespace Magnum { namespace Whee { namespace Test { namespace {

struct AbstractVisualLayerStyleAnimatorTest: TestSuite::Tester {
    explicit AbstractVisualLayerStyleAnimatorTest();

    void construct();
    void constructCopy();
    void constructMove();

    void assignAnimator();
    void assignAnimatorInvalid();

    /* targetStyle() and dynamicStyle() properties tested in
       BaseLayerStyleAnimatorTest and TextLayerStyleAnimatorTest, as those
       depend on the subclass correctly updating the views */
    void propertiesInvalid();

    void clean();
    void cleanEmpty();
    /* There's no assert to trigger in clean() other than what's checked by
       AbstractAnimator::clean() already */
};

using namespace Math::Literals;

AbstractVisualLayerStyleAnimatorTest::AbstractVisualLayerStyleAnimatorTest() {
    addTests({&AbstractVisualLayerStyleAnimatorTest::construct,
              &AbstractVisualLayerStyleAnimatorTest::constructCopy,
              &AbstractVisualLayerStyleAnimatorTest::constructMove,

              &AbstractVisualLayerStyleAnimatorTest::assignAnimator,
              &AbstractVisualLayerStyleAnimatorTest::assignAnimatorInvalid,

              &AbstractVisualLayerStyleAnimatorTest::propertiesInvalid,

              &AbstractVisualLayerStyleAnimatorTest::clean,
              &AbstractVisualLayerStyleAnimatorTest::cleanEmpty});
}

void AbstractVisualLayerStyleAnimatorTest::construct() {
    struct Animator: AbstractVisualLayerStyleAnimator {
        explicit Animator(AnimatorHandle handle): AbstractVisualLayerStyleAnimator{handle} {}
    } animator{animatorHandle(167, 0xeb)};

    /* There isn't anything to query on the AbstractVisualLayerStyleAnimator
       itself */
    CORRADE_COMPARE(animator.handle(), animatorHandle(167, 0xeb));
}

void AbstractVisualLayerStyleAnimatorTest::constructCopy() {
    CORRADE_VERIFY(!std::is_copy_constructible<AbstractVisualLayerStyleAnimator>{});
    CORRADE_VERIFY(!std::is_copy_assignable<AbstractVisualLayerStyleAnimator>{});
}

void AbstractVisualLayerStyleAnimatorTest::constructMove() {
    struct Animator: AbstractVisualLayerStyleAnimator {
        explicit Animator(AnimatorHandle handle): AbstractVisualLayerStyleAnimator{handle} {}
    };

    Animator a{animatorHandle(167, 0xeb)};

    Animator b{Utility::move(a)};
    CORRADE_COMPARE(b.handle(), animatorHandle(167, 0xeb));

    Animator c{animatorHandle(0, 2)};
    c = Utility::move(b);
    CORRADE_COMPARE(c.handle(), animatorHandle(167, 0xeb));

    CORRADE_VERIFY(std::is_nothrow_move_constructible<AbstractVisualLayerStyleAnimator>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<AbstractVisualLayerStyleAnimator>::value);
}

void AbstractVisualLayerStyleAnimatorTest::assignAnimator() {
    struct LayerShared: AbstractVisualLayer::Shared {
        explicit LayerShared(UnsignedInt styleCount, UnsignedInt dynamicStyleCount): AbstractVisualLayer::Shared{styleCount, dynamicStyleCount} {}
    } shared{2, 1};

    struct Layer: AbstractVisualLayer {
        explicit Layer(LayerHandle handle, Shared& shared): AbstractVisualLayer{handle, shared} {}

        using AbstractVisualLayer::assignAnimator;

        LayerFeatures doFeatures() const override {
            return LayerFeature::AnimateStyles;
        }
    } layer{layerHandle(0, 1), shared};

    struct Animator: AbstractVisualLayerStyleAnimator {
        explicit Animator(AnimatorHandle handle): AbstractVisualLayerStyleAnimator{handle} {}
    } animator{animatorHandle(0, 1)};
    CORRADE_COMPARE(animator.layer(), LayerHandle::Null);

    layer.assignAnimator(animator);
    CORRADE_COMPARE(animator.layer(), layer.handle());
}

void AbstractVisualLayerStyleAnimatorTest::assignAnimatorInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct LayerShared: AbstractVisualLayer::Shared {
        explicit LayerShared(UnsignedInt styleCount, UnsignedInt dynamicStyleCount): AbstractVisualLayer::Shared{styleCount, dynamicStyleCount} {}
    } shared{2, 0};

    struct Layer: AbstractVisualLayer {
        explicit Layer(LayerHandle handle, Shared& shared): AbstractVisualLayer{handle, shared} {}

        using AbstractVisualLayer::assignAnimator;

        LayerFeatures doFeatures() const override {
            return LayerFeature::AnimateStyles;
        }
    } layer{layerHandle(0, 1), shared};

    struct Animator: AbstractVisualLayerStyleAnimator {
        explicit Animator(AnimatorHandle handle): AbstractVisualLayerStyleAnimator{handle} {}
    } animator{animatorHandle(0, 1)};
    CORRADE_COMPARE(animator.layer(), LayerHandle::Null);

    std::ostringstream out;
    Error redirectError{&out};
    layer.assignAnimator(animator);
    CORRADE_COMPARE(out.str(), "Whee::AbstractVisualLayer::assignAnimator(): can't animate a layer with zero dynamic styles\n");
}

void AbstractVisualLayerStyleAnimatorTest::propertiesInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct LayerShared: AbstractVisualLayer::Shared {
        explicit LayerShared(UnsignedInt styleCount, UnsignedInt dynamicStyleCount): AbstractVisualLayer::Shared{styleCount, dynamicStyleCount} {}
    } shared{2, 1};

    struct Layer: AbstractVisualLayer {
        explicit Layer(LayerHandle handle, Shared& shared): AbstractVisualLayer{handle, shared} {}

        using AbstractVisualLayer::assignAnimator;

        LayerFeatures doFeatures() const override {
            return LayerFeature::AnimateStyles;
        }
    } layer{layerHandle(0, 1), shared};

    struct Animator: AbstractVisualLayerStyleAnimator {
        explicit Animator(AnimatorHandle handle): AbstractVisualLayerStyleAnimator{handle} {}

        using AbstractVisualLayerStyleAnimator::create;
    } animator{animatorHandle(0, 1)};
    layer.assignAnimator(animator);

    AnimationHandle handle = animator.create(12_nsec, 13_nsec, DataHandle::Null);

    std::ostringstream out;
    Error redirectError{&out};
    animator.targetStyle(AnimationHandle::Null);
    animator.dynamicStyle(AnimationHandle::Null);
    /* Valid animator, invalid data */
    animator.targetStyle(animationHandle(animator.handle(), AnimatorDataHandle(0x123abcde)));
    animator.dynamicStyle(animationHandle(animator.handle(), AnimatorDataHandle(0x123abcde)));
    /* Invalid animator, valid data */
    animator.targetStyle(animationHandle(AnimatorHandle::Null, animationHandleData(handle)));
    animator.dynamicStyle(animationHandle(AnimatorHandle::Null, animationHandleData(handle)));
    /* AnimatorDataHandle directly */
    animator.targetStyle(AnimatorDataHandle(0x123abcde));
    animator.dynamicStyle(AnimatorDataHandle(0x123abcde));
    CORRADE_COMPARE_AS(out.str(),
        "Whee::AbstractVisualLayerStyleAnimator::targetStyle(): invalid handle Whee::AnimationHandle::Null\n"
        "Whee::AbstractVisualLayerStyleAnimator::dynamicStyle(): invalid handle Whee::AnimationHandle::Null\n"

        "Whee::AbstractVisualLayerStyleAnimator::targetStyle(): invalid handle Whee::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Whee::AbstractVisualLayerStyleAnimator::dynamicStyle(): invalid handle Whee::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"

        "Whee::AbstractVisualLayerStyleAnimator::targetStyle(): invalid handle Whee::AnimationHandle(Null, {0x0, 0x1})\n"
        "Whee::AbstractVisualLayerStyleAnimator::dynamicStyle(): invalid handle Whee::AnimationHandle(Null, {0x0, 0x1})\n"

        "Whee::AbstractVisualLayerStyleAnimator::targetStyle(): invalid handle Whee::AnimatorDataHandle(0xabcde, 0x123)\n"
        "Whee::AbstractVisualLayerStyleAnimator::dynamicStyle(): invalid handle Whee::AnimatorDataHandle(0xabcde, 0x123)\n",
        TestSuite::Compare::String);
}

void AbstractVisualLayerStyleAnimatorTest::clean() {
    struct LayerShared: AbstractVisualLayer::Shared {
        explicit LayerShared(UnsignedInt styleCount, UnsignedInt dynamicStyleCount): AbstractVisualLayer::Shared{styleCount, dynamicStyleCount} {}
    } shared{2, 1};

    struct Layer: AbstractVisualLayer {
        explicit Layer(LayerHandle handle, Shared& shared): AbstractVisualLayer{handle, shared} {}

        using AbstractVisualLayer::assignAnimator;

        LayerFeatures doFeatures() const override {
            return LayerFeature::AnimateStyles;
        }
    } layer{layerHandle(0, 1), shared};

    struct Animator: AbstractVisualLayerStyleAnimator {
        explicit Animator(AnimatorHandle handle): AbstractVisualLayerStyleAnimator{handle} {}

        AnimationHandle create(Nanoseconds played, Nanoseconds duration, DataHandle data) {
            AnimationHandle handle = AbstractVisualLayerStyleAnimator::create(played, duration, data);
            /* Have to satisfy the requirement of the view having the same size
               as capacity */
            _state->dynamicStyles = Containers::stridedArrayView(_dynamicStyles).broadcasted<0>(capacity());
            return handle;
        }

        private:
            UnsignedInt _dynamicStyles[1]{~UnsignedInt{}};
    } animator{animatorHandle(0, 1)};
    layer.assignAnimator(animator);

    /* Creating animations doesn't allocate dynamic styles just yet, only
       advance() does */
    AnimationHandle first = animator.create(12_nsec, 13_nsec, DataHandle::Null);
    AnimationHandle second = animator.create(12_nsec, 13_nsec, DataHandle::Null);
    AnimationHandle third = animator.create(12_nsec, 13_nsec, DataHandle::Null);
    CORRADE_COMPARE(animator.usedCount(), 3);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 0);

    /* So cleaning them shouldn't try to recycle them either. Cleaning
       animations with allocated dynamic styles is tested in advance(). */
    UnsignedByte animationIdsToRemove[]{0x05}; /* 0b101 */
    animator.clean(Containers::BitArrayView{animationIdsToRemove, 0, 3});
    CORRADE_COMPARE(animator.usedCount(), 1);
    CORRADE_COMPARE(layer.dynamicStyleUsedCount(), 0);
    CORRADE_VERIFY(!animator.isHandleValid(first));
    CORRADE_VERIFY(animator.isHandleValid(second));
    CORRADE_VERIFY(!animator.isHandleValid(third));
}

void AbstractVisualLayerStyleAnimatorTest::cleanEmpty() {
    /* This should work even with no layer being set */
    struct Animator: AbstractVisualLayerStyleAnimator {
        explicit Animator(AnimatorHandle handle): AbstractVisualLayerStyleAnimator{handle} {}
    } animator{animatorHandle(0, 1)};
    animator.clean({});

    CORRADE_VERIFY(true);
}

}}}}

CORRADE_TEST_MAIN(Magnum::Whee::Test::AbstractVisualLayerStyleAnimatorTest)
