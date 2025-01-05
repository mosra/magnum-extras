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

#include <Corrade/Containers/BitArrayView.h>
#include <Corrade/Containers/Function.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Containers/String.h>
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/String.h>

#include "Magnum/Animation/Easing.h"
#include "Magnum/Math/Time.h"
#include "Magnum/Ui/AbstractLayer.h"
#include "Magnum/Ui/GenericAnimator.h"
#include "Magnum/Ui/Handle.h"

namespace Magnum { namespace Ui { namespace Test { namespace {

struct GenericAnimatorTest: TestSuite::Tester {
    explicit GenericAnimatorTest();

    void construct();
    void constructNode();
    void constructData();
    void constructCopy();
    void constructCopyNode();
    void constructCopyData();
    void constructMove();
    void constructMoveNode();
    void constructMoveData();

    void createRemove();
    void createRemoveNode();
    void createRemoveData();
    void createRemoveHandleRecycle();
    void createRemoveHandleRecycleNode();
    void createRemoveHandleRecycleData();
    void createInvalid();
    void createInvalidNode();
    void createInvalidData();
    /* There's no assert to trigger in remove() other than what's checked by
       AbstractAnimator::remove() already */
    void propertiesInvalid();
    void propertiesInvalidNode();
    void propertiesInvalidData();

    void clean();
    void cleanNode();
    void cleanData();

    void advance();
    void advanceNode();
    void advanceData();
    void advanceEmpty();
    void advanceEmptyNode();
    void advanceEmptyData();
};

using namespace Math::Literals;

GenericAnimatorTest::GenericAnimatorTest() {
    addTests({&GenericAnimatorTest::construct,
              &GenericAnimatorTest::constructNode,
              &GenericAnimatorTest::constructData,
              &GenericAnimatorTest::constructCopy,
              &GenericAnimatorTest::constructCopyNode,
              &GenericAnimatorTest::constructCopyData,
              &GenericAnimatorTest::constructMove,
              &GenericAnimatorTest::constructMoveNode,
              &GenericAnimatorTest::constructMoveData,

              &GenericAnimatorTest::createRemove,
              &GenericAnimatorTest::createRemoveNode,
              &GenericAnimatorTest::createRemoveData,
              &GenericAnimatorTest::createRemoveHandleRecycle,
              &GenericAnimatorTest::createRemoveHandleRecycleNode,
              &GenericAnimatorTest::createRemoveHandleRecycleData,
              &GenericAnimatorTest::createInvalid,
              &GenericAnimatorTest::createInvalidNode,
              &GenericAnimatorTest::createInvalidData,
              &GenericAnimatorTest::propertiesInvalid,
              &GenericAnimatorTest::propertiesInvalidNode,
              &GenericAnimatorTest::propertiesInvalidData,

              &GenericAnimatorTest::clean,
              &GenericAnimatorTest::cleanNode,
              &GenericAnimatorTest::cleanData,

              &GenericAnimatorTest::advance,
              &GenericAnimatorTest::advanceNode,
              &GenericAnimatorTest::advanceData,
              &GenericAnimatorTest::advanceEmpty,
              &GenericAnimatorTest::advanceEmptyNode,
              &GenericAnimatorTest::advanceEmptyData});
}

void GenericAnimatorTest::construct() {
    GenericAnimator animator{animatorHandle(0xab, 0x12)};

    CORRADE_COMPARE(animator.features(), AnimatorFeatures{});
    CORRADE_COMPARE(animator.handle(), animatorHandle(0xab, 0x12));
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 0);
    /* The rest is the same as in AbstractAnimatorTest::constructGeneric() */
}

void GenericAnimatorTest::constructNode() {
    GenericNodeAnimator animator{animatorHandle(0xab, 0x12)};

    CORRADE_COMPARE(animator.features(), AnimatorFeature::NodeAttachment);
    CORRADE_COMPARE(animator.handle(), animatorHandle(0xab, 0x12));
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 0);
    /* The rest is the same as in AbstractAnimatorTest::constructGeneric() */
}

void GenericAnimatorTest::constructData() {
    GenericDataAnimator animator{animatorHandle(0xab, 0x12)};

    CORRADE_COMPARE(animator.features(), AnimatorFeature::DataAttachment);
    CORRADE_COMPARE(animator.handle(), animatorHandle(0xab, 0x12));
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 0);
    CORRADE_COMPARE(animator.layer(), LayerHandle::Null);
    /* The rest is the same as in AbstractAnimatorTest::constructGeneric() */

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0xcd, 0x34)};
    animator.setLayer(layer);
    CORRADE_COMPARE(animator.layer(), layerHandle(0xcd, 0x34));
}

void GenericAnimatorTest::constructCopy() {
    CORRADE_VERIFY(!std::is_copy_constructible<GenericAnimator>{});
    CORRADE_VERIFY(!std::is_copy_assignable<GenericAnimator>{});
}

void GenericAnimatorTest::constructCopyNode() {
    CORRADE_VERIFY(!std::is_copy_constructible<GenericNodeAnimator>{});
    CORRADE_VERIFY(!std::is_copy_assignable<GenericNodeAnimator>{});
}

void GenericAnimatorTest::constructCopyData() {
    CORRADE_VERIFY(!std::is_copy_constructible<GenericDataAnimator>{});
    CORRADE_VERIFY(!std::is_copy_assignable<GenericDataAnimator>{});
}

void GenericAnimatorTest::constructMove() {
    /* Just verify that the subclass doesn't have the moves broken */

    GenericAnimator a{animatorHandle(0xab, 0x12)};

    GenericAnimator b{Utility::move(a)};
    CORRADE_COMPARE(b.handle(), animatorHandle(0xab, 0x12));

    GenericAnimator c{animatorHandle(0xcd, 0x34)};
    c = Utility::move(b);
    CORRADE_COMPARE(c.handle(), animatorHandle(0xab, 0x12));

    CORRADE_VERIFY(std::is_nothrow_move_constructible<GenericAnimator>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<GenericAnimator>::value);
}

void GenericAnimatorTest::constructMoveNode() {
    /* Just verify that the subclass doesn't have the moves broken */

    GenericNodeAnimator a{animatorHandle(0xab, 0x12)};

    GenericNodeAnimator b{Utility::move(a)};
    CORRADE_COMPARE(b.handle(), animatorHandle(0xab, 0x12));

    GenericNodeAnimator c{animatorHandle(0xcd, 0x34)};
    c = Utility::move(b);
    CORRADE_COMPARE(c.handle(), animatorHandle(0xab, 0x12));

    CORRADE_VERIFY(std::is_nothrow_move_constructible<GenericNodeAnimator>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<GenericNodeAnimator>::value);
}

void GenericAnimatorTest::constructMoveData() {
    /* Just verify that the subclass doesn't have the moves broken */

    GenericDataAnimator a{animatorHandle(0xab, 0x12)};

    GenericDataAnimator b{Utility::move(a)};
    CORRADE_COMPARE(b.handle(), animatorHandle(0xab, 0x12));

    GenericDataAnimator c{animatorHandle(0xcd, 0x34)};
    c = Utility::move(b);
    CORRADE_COMPARE(c.handle(), animatorHandle(0xab, 0x12));

    CORRADE_VERIFY(std::is_nothrow_move_constructible<GenericDataAnimator>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<GenericDataAnimator>::value);
}

void GenericAnimatorTest::createRemove() {
    Int destructedCount = 0;
    struct NonTrivial {
        explicit NonTrivial(int& output): destructedCount{&output} {}
        ~NonTrivial() {
            ++*destructedCount;
        }
        void operator()(Float) const {
            CORRADE_FAIL("This should never be called.");
        }

        Int* destructedCount;
    };

    GenericAnimator animator{animatorHandle(0, 1)};

    AnimationHandle trivial = animator.create([](Float) {
        CORRADE_FAIL("This should never be called.");
    }, Animation::Easing::bounceOut, 137_nsec, 277_nsec, 3, AnimationFlag::KeepOncePlayed);
    CORRADE_COMPARE(animator.usedCount(), 1);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 0);
    CORRADE_COMPARE(animator.played(trivial), 137_nsec);
    CORRADE_COMPARE(animator.duration(trivial), 277_nsec);
    CORRADE_COMPARE(animator.repeatCount(trivial), 3);
    CORRADE_COMPARE(animator.flags(trivial), AnimationFlag::KeepOncePlayed);
    CORRADE_COMPARE(animator.easing(trivial), Animation::Easing::bounceOut);

    /* The temporary gets destructed right away */
    AnimationHandle nonTrivial = animator.create(NonTrivial{destructedCount}, Animation::Easing::smootherstep, 226_nsec, 191_nsec, 0, AnimationFlags{0x80});
    CORRADE_COMPARE(animator.usedCount(), 2);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 1);
    CORRADE_COMPARE(animator.played(nonTrivial), 226_nsec);
    CORRADE_COMPARE(animator.duration(nonTrivial), 191_nsec);
    CORRADE_COMPARE(animator.repeatCount(nonTrivial), 0);
    CORRADE_COMPARE(animator.flags(nonTrivial), AnimationFlags{0x80});
    /* Testing also the other overload */
    CORRADE_COMPARE(animator.easing(animationHandleData(nonTrivial)), Animation::Easing::smootherstep);
    CORRADE_COMPARE(destructedCount, 1);

    animator.remove(trivial);
    CORRADE_COMPARE(animator.usedCount(), 1);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 1);
    CORRADE_COMPARE(destructedCount, 1);

    /* Verifying also the other handle overload. They should both delegate into
       the same internal implementation. */
    animator.remove(animationHandleData(nonTrivial));
    CORRADE_COMPARE(animator.usedCount(), 0);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 0);
    CORRADE_COMPARE(destructedCount, 2);
}

void GenericAnimatorTest::createRemoveNode() {
    Int destructedCount = 0;
    struct NonTrivial {
        explicit NonTrivial(int& output): destructedCount{&output} {}
        ~NonTrivial() {
            ++*destructedCount;
        }
        void operator()(NodeHandle, Float) const {
            CORRADE_FAIL("This should never be called.");
        }

        Int* destructedCount;
    };

    GenericNodeAnimator animator{animatorHandle(0, 1)};

    AnimationHandle trivial = animator.create([](NodeHandle, Float) {
        CORRADE_FAIL("This should never be called.");
    }, Animation::Easing::bounceOut, 137_nsec, 277_nsec, nodeHandle(0x12345, 0xabc), 3, AnimationFlag::KeepOncePlayed);
    CORRADE_COMPARE(animator.usedCount(), 1);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 0);
    CORRADE_COMPARE(animator.played(trivial), 137_nsec);
    CORRADE_COMPARE(animator.duration(trivial), 277_nsec);
    CORRADE_COMPARE(animator.repeatCount(trivial), 3);
    CORRADE_COMPARE(animator.flags(trivial), AnimationFlag::KeepOncePlayed);
    CORRADE_COMPARE(animator.node(trivial), nodeHandle(0x12345, 0xabc));
    CORRADE_COMPARE(animator.easing(trivial), Animation::Easing::bounceOut);

    /* The temporary gets destructed right away */
    AnimationHandle nonTrivial = animator.create(NonTrivial{destructedCount}, Animation::Easing::smootherstep, 226_nsec, 191_nsec, nodeHandle(0x67890, 0xdef), 0, AnimationFlags{0x80});
    CORRADE_COMPARE(animator.usedCount(), 2);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 1);
    CORRADE_COMPARE(animator.played(nonTrivial), 226_nsec);
    CORRADE_COMPARE(animator.duration(nonTrivial), 191_nsec);
    CORRADE_COMPARE(animator.repeatCount(nonTrivial), 0);
    CORRADE_COMPARE(animator.flags(nonTrivial), AnimationFlags{0x80});
    CORRADE_COMPARE(animator.node(nonTrivial), nodeHandle(0x67890, 0xdef));
    /* Testing also the other overload */
    CORRADE_COMPARE(animator.easing(animationHandleData(nonTrivial)), Animation::Easing::smootherstep);
    CORRADE_COMPARE(destructedCount, 1);

    animator.remove(trivial);
    CORRADE_COMPARE(animator.usedCount(), 1);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 1);
    CORRADE_COMPARE(destructedCount, 1);

    /* Verifying also the other handle overload. They should both delegate into
       the same internal implementation. */
    animator.remove(animationHandleData(nonTrivial));
    CORRADE_COMPARE(animator.usedCount(), 0);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 0);
    CORRADE_COMPARE(destructedCount, 2);
}

void GenericAnimatorTest::createRemoveData() {
    Int destructedCount = 0;
    struct NonTrivial {
        explicit NonTrivial(int& output): destructedCount{&output} {}
        ~NonTrivial() {
            ++*destructedCount;
        }
        void operator()(DataHandle, Float) const {
            CORRADE_FAIL("This should never be called.");
        }

        Int* destructedCount;
    };

    GenericDataAnimator animator{animatorHandle(0, 1)};

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0xcd, 0x34)};
    animator.setLayer(layer);

    AnimationHandle trivial = animator.create([](DataHandle, Float) {
        CORRADE_FAIL("This should never be called.");
    }, Animation::Easing::bounceOut, 137_nsec, 277_nsec, dataHandle(layer.handle(), 0x12345, 0xabc), 3, AnimationFlag::KeepOncePlayed);
    CORRADE_COMPARE(animator.usedCount(), 1);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 0);
    CORRADE_COMPARE(animator.played(trivial), 137_nsec);
    CORRADE_COMPARE(animator.duration(trivial), 277_nsec);
    CORRADE_COMPARE(animator.repeatCount(trivial), 3);
    CORRADE_COMPARE(animator.flags(trivial), AnimationFlag::KeepOncePlayed);
    CORRADE_COMPARE(animator.data(trivial), dataHandle(layer.handle(), 0x12345, 0xabc));
    CORRADE_COMPARE(animator.easing(trivial), Animation::Easing::bounceOut);

    /* The temporary gets destructed right away. Testing also the
       LayerDataHandle overload. */
    AnimationHandle nonTrivial = animator.create(NonTrivial{destructedCount}, Animation::Easing::smootherstep, 226_nsec, 191_nsec, layerDataHandle(0x67890, 0xdef), 0, AnimationFlags{0x80});
    CORRADE_COMPARE(animator.usedCount(), 2);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 1);
    CORRADE_COMPARE(animator.played(nonTrivial), 226_nsec);
    CORRADE_COMPARE(animator.duration(nonTrivial), 191_nsec);
    CORRADE_COMPARE(animator.repeatCount(nonTrivial), 0);
    CORRADE_COMPARE(animator.flags(nonTrivial), AnimationFlags{0x80});
    CORRADE_COMPARE(animator.data(nonTrivial), dataHandle(layer.handle(), 0x67890, 0xdef));
    /* Testing also the other overload */
    CORRADE_COMPARE(animator.easing(animationHandleData(nonTrivial)), Animation::Easing::smootherstep);
    CORRADE_COMPARE(destructedCount, 1);

    animator.remove(trivial);
    CORRADE_COMPARE(animator.usedCount(), 1);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 1);
    CORRADE_COMPARE(destructedCount, 1);

    /* Verifying also the other handle overload. They should both delegate into
       the same internal implementation. */
    animator.remove(animationHandleData(nonTrivial));
    CORRADE_COMPARE(animator.usedCount(), 0);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 0);
    CORRADE_COMPARE(destructedCount, 2);
}

void GenericAnimatorTest::createRemoveHandleRecycle() {
    Int destructedCount1 = 0,
        destructedCount2 = 0;
    struct NonTrivial {
        explicit NonTrivial(int& output): destructedCount{&output} {}
        ~NonTrivial() {
            ++*destructedCount;
        }
        void operator()(Float) const {
            CORRADE_FAIL("This should never be called.");
        }

        Int* destructedCount;
    };

    GenericAnimator animator{animatorHandle(0, 1)};
    animator.create([](Float){}, Animation::Easing::linear, 0_nsec, 1_nsec);

    /* The temporary gets destructed right away */
    AnimationHandle second = animator.create(NonTrivial{destructedCount1}, Animation::Easing::linear, 0_nsec, 1_nsec);
    CORRADE_COMPARE(destructedCount1, 1);

    animator.remove(second);
    CORRADE_COMPARE(destructedCount1, 2);

    /* Animation that reuses a previous slot should not call the destructor
       on the previous function again or some such crazy stuff */
    AnimationHandle second2 = animator.create(NonTrivial{destructedCount2}, Animation::Easing::step, 0_nsec, 1_nsec);
    CORRADE_COMPARE(animationHandleId(second2), animationHandleId(second));
    CORRADE_COMPARE(destructedCount1, 2);
    CORRADE_COMPARE(destructedCount2, 1);
}

void GenericAnimatorTest::createRemoveHandleRecycleNode() {
    Int destructedCount1 = 0,
        destructedCount2 = 0;
    struct NonTrivial {
        explicit NonTrivial(int& output): destructedCount{&output} {}
        ~NonTrivial() {
            ++*destructedCount;
        }
        void operator()(NodeHandle, Float) const {
            CORRADE_FAIL("This should never be called.");
        }

        Int* destructedCount;
    };

    GenericNodeAnimator animator{animatorHandle(0, 1)};
    animator.create([](NodeHandle, Float){}, Animation::Easing::linear, 0_nsec, 1_nsec, NodeHandle::Null);

    /* The temporary gets destructed right away */
    AnimationHandle second = animator.create(NonTrivial{destructedCount1}, Animation::Easing::linear, 0_nsec, 1_nsec, NodeHandle::Null);
    CORRADE_COMPARE(destructedCount1, 1);

    animator.remove(second);
    CORRADE_COMPARE(destructedCount1, 2);

    /* Animation that reuses a previous slot should not call the destructor
       on the previous function again or some such crazy stuff */
    AnimationHandle second2 = animator.create(NonTrivial{destructedCount2}, Animation::Easing::step, 0_nsec, 1_nsec, NodeHandle::Null);
    CORRADE_COMPARE(animationHandleId(second2), animationHandleId(second));
    CORRADE_COMPARE(destructedCount1, 2);
    CORRADE_COMPARE(destructedCount2, 1);
}

void GenericAnimatorTest::createRemoveHandleRecycleData() {
    Int destructedCount1 = 0,
        destructedCount2 = 0;
    struct NonTrivial {
        explicit NonTrivial(int& output): destructedCount{&output} {}
        ~NonTrivial() {
            ++*destructedCount;
        }
        void operator()(DataHandle, Float) const {
            CORRADE_FAIL("This should never be called.");
        }

        Int* destructedCount;
    };

    GenericDataAnimator animator{animatorHandle(0, 1)};

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0xcd, 0x34)};
    animator.setLayer(layer);

    animator.create([](DataHandle, Float){}, Animation::Easing::linear, 0_nsec, 1_nsec, DataHandle::Null);

    /* The temporary gets destructed right away */
    AnimationHandle second = animator.create(NonTrivial{destructedCount1}, Animation::Easing::linear, 0_nsec, 1_nsec, DataHandle::Null);
    CORRADE_COMPARE(destructedCount1, 1);

    animator.remove(second);
    CORRADE_COMPARE(destructedCount1, 2);

    /* Animation that reuses a previous slot should not call the destructor
       on the previous function again or some such crazy stuff */
    AnimationHandle second2 = animator.create(NonTrivial{destructedCount2}, Animation::Easing::step, 0_nsec, 1_nsec, DataHandle::Null);
    CORRADE_COMPARE(animationHandleId(second2), animationHandleId(second));
    CORRADE_COMPARE(destructedCount1, 2);
    CORRADE_COMPARE(destructedCount2, 1);
}

void GenericAnimatorTest::createInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    GenericAnimator animator{animatorHandle(0, 1)};

    Containers::String out;
    Error redirectError{&out};
    animator.create(nullptr, Animation::Easing::linear, 0_nsec, 1_nsec);
    animator.create([](Float) {}, nullptr, 0_nsec, 1_nsec);
    CORRADE_COMPARE_AS(out,
        "Ui::GenericAnimator::create(): animation is null\n"
        "Ui::GenericAnimator::create(): easing is null\n",
        TestSuite::Compare::String);
}

void GenericAnimatorTest::createInvalidNode() {
    CORRADE_SKIP_IF_NO_ASSERT();

    GenericNodeAnimator animator{animatorHandle(0, 1)};

    Containers::String out;
    Error redirectError{&out};
    animator.create(nullptr, Animation::Easing::linear, 0_nsec, 1_nsec, NodeHandle::Null);
    animator.create([](NodeHandle, Float) {}, nullptr, 0_nsec, 1_nsec, NodeHandle::Null);
    CORRADE_COMPARE_AS(out,
        "Ui::GenericNodeAnimator::create(): animation is null\n"
        "Ui::GenericNodeAnimator::create(): easing is null\n",
        TestSuite::Compare::String);
}

void GenericAnimatorTest::createInvalidData() {
    CORRADE_SKIP_IF_NO_ASSERT();

    GenericDataAnimator animator{animatorHandle(0, 1)};

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0, 1)};
    animator.setLayer(layer);

    Containers::String out;
    Error redirectError{&out};
    animator.create(nullptr, Animation::Easing::linear, 0_nsec, 1_nsec, DataHandle::Null);
    animator.create(nullptr, Animation::Easing::linear, 0_nsec, 1_nsec, LayerDataHandle::Null);
    animator.create([](DataHandle, Float) {}, nullptr, 0_nsec, 1_nsec, DataHandle::Null);
    animator.create([](DataHandle, Float) {}, nullptr, 0_nsec, 1_nsec, LayerDataHandle::Null);
    CORRADE_COMPARE_AS(out,
        "Ui::GenericDataAnimator::create(): animation is null\n"
        "Ui::GenericDataAnimator::create(): animation is null\n"
        "Ui::GenericDataAnimator::create(): easing is null\n"
        "Ui::GenericDataAnimator::create(): easing is null\n",
        TestSuite::Compare::String);
}

void GenericAnimatorTest::propertiesInvalid() {
    CORRADE_SKIP_IF_NO_ASSERT();

    GenericAnimator animator{animatorHandle(0, 1)};

    AnimationHandle handle = animator.create([](Float) {}, Animation::Easing::linear, 12_nsec, 13_nsec);

    Containers::String out;
    Error redirectError{&out};
    animator.easing(AnimationHandle::Null);
    /* Valid animator, invalid data */
    animator.easing(animationHandle(animator.handle(), AnimatorDataHandle(0x123abcde)));
    /* Invalid animator, valid data */
    animator.easing(animationHandle(AnimatorHandle::Null, animationHandleData(handle)));
    /* AnimatorDataHandle directly */
    animator.easing(AnimatorDataHandle(0x123abcde));
    CORRADE_COMPARE_AS(out,
        "Ui::GenericAnimator::easing(): invalid handle Ui::AnimationHandle::Null\n"
        "Ui::GenericAnimator::easing(): invalid handle Ui::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Ui::GenericAnimator::easing(): invalid handle Ui::AnimationHandle(Null, {0x0, 0x1})\n"
        "Ui::GenericAnimator::easing(): invalid handle Ui::AnimatorDataHandle(0xabcde, 0x123)\n",
        TestSuite::Compare::String);
}

void GenericAnimatorTest::propertiesInvalidNode() {
    CORRADE_SKIP_IF_NO_ASSERT();

    GenericNodeAnimator animator{animatorHandle(0, 1)};

    AnimationHandle handle = animator.create([](NodeHandle, Float) {}, Animation::Easing::linear, 12_nsec, 13_nsec, NodeHandle::Null);

    Containers::String out;
    Error redirectError{&out};
    animator.easing(AnimationHandle::Null);
    /* Valid animator, invalid data */
    animator.easing(animationHandle(animator.handle(), AnimatorDataHandle(0x123abcde)));
    /* Invalid animator, valid data */
    animator.easing(animationHandle(AnimatorHandle::Null, animationHandleData(handle)));
    /* AnimatorDataHandle directly */
    animator.easing(AnimatorDataHandle(0x123abcde));
    CORRADE_COMPARE_AS(out,
        "Ui::GenericNodeAnimator::easing(): invalid handle Ui::AnimationHandle::Null\n"
        "Ui::GenericNodeAnimator::easing(): invalid handle Ui::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Ui::GenericNodeAnimator::easing(): invalid handle Ui::AnimationHandle(Null, {0x0, 0x1})\n"
        "Ui::GenericNodeAnimator::easing(): invalid handle Ui::AnimatorDataHandle(0xabcde, 0x123)\n",
        TestSuite::Compare::String);
}

void GenericAnimatorTest::propertiesInvalidData() {
    CORRADE_SKIP_IF_NO_ASSERT();

    GenericDataAnimator animator{animatorHandle(0, 1)};

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0, 1)};
    animator.setLayer(layer);

    AnimationHandle handle = animator.create([](DataHandle, Float) {}, Animation::Easing::linear, 12_nsec, 13_nsec, DataHandle::Null);

    Containers::String out;
    Error redirectError{&out};
    animator.easing(AnimationHandle::Null);
    /* Valid animator, invalid data */
    animator.easing(animationHandle(animator.handle(), AnimatorDataHandle(0x123abcde)));
    /* Invalid animator, valid data */
    animator.easing(animationHandle(AnimatorHandle::Null, animationHandleData(handle)));
    /* AnimatorDataHandle directly */
    animator.easing(AnimatorDataHandle(0x123abcde));
    CORRADE_COMPARE_AS(out,
        "Ui::GenericDataAnimator::easing(): invalid handle Ui::AnimationHandle::Null\n"
        "Ui::GenericDataAnimator::easing(): invalid handle Ui::AnimationHandle({0x0, 0x1}, {0xabcde, 0x123})\n"
        "Ui::GenericDataAnimator::easing(): invalid handle Ui::AnimationHandle(Null, {0x0, 0x1})\n"
        "Ui::GenericDataAnimator::easing(): invalid handle Ui::AnimatorDataHandle(0xabcde, 0x123)\n",
        TestSuite::Compare::String);
}

void GenericAnimatorTest::clean() {
    Int destructedCount = 0,
        anotherDestructedCount = 0;
    struct NonTrivial {
        explicit NonTrivial(int& output): destructedCount{&output} {}
        ~NonTrivial() {
            ++*destructedCount;
        }
        void operator()(Float) const {
            CORRADE_FAIL("This should never be called.");
        }

        Int* destructedCount;
    };

    GenericAnimator animator{animatorHandle(0, 1)};

    AnimationHandle trivial = animator.create([](Float) {}, Animation::Easing::bounceOut, 137_nsec, 277_nsec, 3, AnimationFlag::KeepOncePlayed);
    CORRADE_COMPARE(animator.usedCount(), 1);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 0);

    /* The temporary gets destructed right away */
    AnimationHandle nonTrivial = animator.create(NonTrivial{destructedCount}, Animation::Easing::smootherstep, 226_nsec, 191_nsec, 0, AnimationFlags{0x80});
    CORRADE_COMPARE(animator.usedCount(), 2);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 1);
    CORRADE_COMPARE(destructedCount, 1);

    AnimationHandle another = animator.create([](Float) {}, Animation::Easing::bounceOut, 137_nsec, 277_nsec, 3, AnimationFlag::KeepOncePlayed);
    CORRADE_COMPARE(animator.usedCount(), 3);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 1);
    CORRADE_COMPARE(destructedCount, 1);

    /* The temporary gets destructed right away */
    AnimationHandle anotherNonTrivial = animator.create(NonTrivial{anotherDestructedCount}, Animation::Easing::smootherstep, 226_nsec, 191_nsec, 0, AnimationFlags{0x80});
    CORRADE_COMPARE(animator.usedCount(), 4);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 2);
    CORRADE_COMPARE(anotherDestructedCount, 1);

    /* It should remove two but call just one destructor */
    UnsignedByte data[]{(1 << 0)|(1 << 3)};
    animator.clean(Containers::BitArrayView{data, 0, 4});
    CORRADE_COMPARE(animator.usedCount(), 2);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 1);
    CORRADE_COMPARE(destructedCount, 1);
    CORRADE_COMPARE(anotherDestructedCount, 2);
    CORRADE_VERIFY(!animator.isHandleValid(trivial));
    CORRADE_VERIFY(animator.isHandleValid(nonTrivial));
    CORRADE_VERIFY(animator.isHandleValid(another));
    CORRADE_VERIFY(!animator.isHandleValid(anotherNonTrivial));
}

void GenericAnimatorTest::cleanNode() {
    Int destructedCount = 0,
        anotherDestructedCount = 0;
    struct NonTrivial {
        explicit NonTrivial(int& output): destructedCount{&output} {}
        ~NonTrivial() {
            ++*destructedCount;
        }
        void operator()(NodeHandle, Float) const {
            CORRADE_FAIL("This should never be called.");
        }

        Int* destructedCount;
    };

    GenericNodeAnimator animator{animatorHandle(0, 1)};

    AnimationHandle trivial = animator.create([](NodeHandle, Float) {}, Animation::Easing::bounceOut, 137_nsec, 277_nsec, NodeHandle::Null, 3, AnimationFlag::KeepOncePlayed);
    CORRADE_COMPARE(animator.usedCount(), 1);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 0);

    /* The temporary gets destructed right away */
    AnimationHandle nonTrivial = animator.create(NonTrivial{destructedCount}, Animation::Easing::smootherstep, 226_nsec, 191_nsec, NodeHandle::Null, 0, AnimationFlags{0x80});
    CORRADE_COMPARE(animator.usedCount(), 2);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 1);
    CORRADE_COMPARE(destructedCount, 1);

    AnimationHandle another = animator.create([](NodeHandle, Float) {}, Animation::Easing::bounceOut, 137_nsec, 277_nsec, NodeHandle::Null, 3, AnimationFlag::KeepOncePlayed);
    CORRADE_COMPARE(animator.usedCount(), 3);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 1);
    CORRADE_COMPARE(destructedCount, 1);

    /* The temporary gets destructed right away */
    AnimationHandle anotherNonTrivial = animator.create(NonTrivial{anotherDestructedCount}, Animation::Easing::smootherstep, 226_nsec, 191_nsec, NodeHandle::Null, 0, AnimationFlags{0x80});
    CORRADE_COMPARE(animator.usedCount(), 4);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 2);
    CORRADE_COMPARE(anotherDestructedCount, 1);

    /* It should remove two but call just one destructor */
    UnsignedByte data[]{(1 << 0)|(1 << 3)};
    animator.clean(Containers::BitArrayView{data, 0, 4});
    CORRADE_COMPARE(animator.usedCount(), 2);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 1);
    CORRADE_COMPARE(destructedCount, 1);
    CORRADE_COMPARE(anotherDestructedCount, 2);
    CORRADE_VERIFY(!animator.isHandleValid(trivial));
    CORRADE_VERIFY(animator.isHandleValid(nonTrivial));
    CORRADE_VERIFY(animator.isHandleValid(another));
    CORRADE_VERIFY(!animator.isHandleValid(anotherNonTrivial));
}

void GenericAnimatorTest::cleanData() {
    Int destructedCount = 0,
        anotherDestructedCount = 0;
    struct NonTrivial {
        explicit NonTrivial(int& output): destructedCount{&output} {}
        ~NonTrivial() {
            ++*destructedCount;
        }
        void operator()(DataHandle, Float) const {
            CORRADE_FAIL("This should never be called.");
        }

        Int* destructedCount;
    };

    GenericDataAnimator animator{animatorHandle(0, 1)};

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0, 1)};
    animator.setLayer(layer);

    AnimationHandle trivial = animator.create([](DataHandle, Float) {}, Animation::Easing::bounceOut, 137_nsec, 277_nsec, DataHandle::Null, 3, AnimationFlag::KeepOncePlayed);
    CORRADE_COMPARE(animator.usedCount(), 1);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 0);

    /* The temporary gets destructed right away */
    AnimationHandle nonTrivial = animator.create(NonTrivial{destructedCount}, Animation::Easing::smootherstep, 226_nsec, 191_nsec, DataHandle::Null, 0, AnimationFlags{0x80});
    CORRADE_COMPARE(animator.usedCount(), 2);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 1);
    CORRADE_COMPARE(destructedCount, 1);

    AnimationHandle another = animator.create([](DataHandle, Float) {}, Animation::Easing::bounceOut, 137_nsec, 277_nsec, DataHandle::Null, 3, AnimationFlag::KeepOncePlayed);
    CORRADE_COMPARE(animator.usedCount(), 3);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 1);
    CORRADE_COMPARE(destructedCount, 1);

    /* The temporary gets destructed right away */
    AnimationHandle anotherNonTrivial = animator.create(NonTrivial{anotherDestructedCount}, Animation::Easing::smootherstep, 226_nsec, 191_nsec, DataHandle::Null, 0, AnimationFlags{0x80});
    CORRADE_COMPARE(animator.usedCount(), 4);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 2);
    CORRADE_COMPARE(anotherDestructedCount, 1);

    /* It should remove two but call just one destructor */
    UnsignedByte data[]{(1 << 0)|(1 << 3)};
    animator.clean(Containers::BitArrayView{data, 0, 4});
    CORRADE_COMPARE(animator.usedCount(), 2);
    CORRADE_COMPARE(animator.usedAllocatedAnimationCount(), 1);
    CORRADE_COMPARE(destructedCount, 1);
    CORRADE_COMPARE(anotherDestructedCount, 2);
    CORRADE_VERIFY(!animator.isHandleValid(trivial));
    CORRADE_VERIFY(animator.isHandleValid(nonTrivial));
    CORRADE_VERIFY(animator.isHandleValid(another));
    CORRADE_VERIFY(!animator.isHandleValid(anotherNonTrivial));
}

void GenericAnimatorTest::advance() {
    GenericAnimator animator{animatorHandle(0, 1)};

    Float first = 0.0f;
    animator.create([&first](Float factor) {
        first += factor;
    }, Animation::Easing::linear, 0_nsec, 10_nsec);

    animator.create([](Float) {
        CORRADE_FAIL("This shouldn't be called");
    }, Animation::Easing::linear, 5_nsec, 15_nsec);

    Float third = 0.0f;
    animator.create([&third](Float factor) {
        third += factor;
    }, Animation::Easing::linear, 10_nsec, 5_nsec);

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    /* Should call just the first and third with given factors */
    UnsignedByte data[]{(1 << 0)|(1 << 2)};
    Float factors[]{0.75f, 0.42f, 0.25f};
    animator.advance(Containers::BitArrayView{data, 0, 3}, factors);
    CORRADE_COMPARE(first, 0.75f);
    CORRADE_COMPARE(third, 0.25f);
}

void GenericAnimatorTest::advanceNode() {
    GenericNodeAnimator animator{animatorHandle(0, 1)};

    Float first = 0.0f;
    animator.create([&first](NodeHandle node, Float factor) {
        CORRADE_COMPARE(node, nodeHandle(0xabcde, 0x123));
        first += factor;
    }, Animation::Easing::linear, 0_nsec, 10_nsec, nodeHandle(0xabcde, 0x123));

    animator.create([](NodeHandle, Float) {
        CORRADE_FAIL("This shouldn't be called");
    }, Animation::Easing::linear, 5_nsec, 15_nsec, nodeHandle(0xedcba, 0x321));

    Float third = 0.0f;
    animator.create([&third](NodeHandle node, Float factor) {
        CORRADE_COMPARE(node, NodeHandle::Null);
        third += factor;
    }, Animation::Easing::linear, 10_nsec, 5_nsec, NodeHandle::Null);

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    /* Should call just the first and third with given factors */
    UnsignedByte data[]{(1 << 0)|(1 << 2)};
    Float factors[]{0.75f, 0.42f, 0.25f};
    animator.advance(Containers::BitArrayView{data, 0, 3}, factors);
    CORRADE_COMPARE(first, 0.75f);
    CORRADE_COMPARE(third, 0.25f);
}

void GenericAnimatorTest::advanceData() {
    GenericDataAnimator animator{animatorHandle(0, 1)};

    struct: AbstractLayer {
        using AbstractLayer::AbstractLayer;

        LayerFeatures doFeatures() const override { return {}; }
    } layer{layerHandle(0xab, 0xcd)};
    animator.setLayer(layer);

    Float first = 0.0f;
    animator.create([&first](DataHandle data, Float factor) {
        CORRADE_COMPARE(data, dataHandle(layerHandle(0xab, 0xcd), 0xabcde, 0x123));
        first += factor;
    }, Animation::Easing::linear, 0_nsec, 10_nsec, dataHandle(layer.handle(), 0xabcde, 0x123));

    animator.create([](DataHandle, Float) {
        CORRADE_FAIL("This shouldn't be called");
    }, Animation::Easing::linear, 5_nsec, 15_nsec, dataHandle(layer.handle(), 0xedcba, 0x321));

    Float third = 0.0f;
    animator.create([&third](DataHandle data, Float factor) {
        /* If there's no associated data, the layer handle shouldn't be added
           to the null LayerDataHandle */
        CORRADE_COMPARE(data, DataHandle::Null);
        third += factor;
    }, Animation::Easing::linear, 10_nsec, 5_nsec, LayerDataHandle::Null);

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    /* Should call just the first and third with given factors */
    UnsignedByte data[]{(1 << 0)|(1 << 2)};
    Float factors[]{0.75f, 0.42f, 0.25f};
    animator.advance(Containers::BitArrayView{data, 0, 3}, factors);
    CORRADE_COMPARE(first, 0.75f);
    CORRADE_COMPARE(third, 0.25f);
}

void GenericAnimatorTest::advanceEmpty() {
    GenericAnimator animator{animatorHandle(0, 1)};
    animator.advance({}, {});

    /* Shouldn't crash or anything */
    CORRADE_VERIFY(true);
}

void GenericAnimatorTest::advanceEmptyNode() {
    GenericNodeAnimator animator{animatorHandle(0, 1)};
    animator.advance({}, {});

    /* Shouldn't crash or anything */
    CORRADE_VERIFY(true);
}

void GenericAnimatorTest::advanceEmptyData() {
    /* This should work even with no layer being set */
    GenericDataAnimator animator{animatorHandle(0, 1)};
    animator.advance({}, {});

    /* Shouldn't crash or anything */
    CORRADE_VERIFY(true);
}

}}}}

CORRADE_TEST_MAIN(Magnum::Ui::Test::GenericAnimatorTest)
