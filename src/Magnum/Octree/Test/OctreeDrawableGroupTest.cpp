/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018
              Vladimír Vondruš <mosra@centrum.cz>
    Copyright © 2015 Andrea Capobianco <andrea.c.2205@gmail.com>
    Copyright © 2016, 2018 Jonathan Hale <squareys@googlemail.com>

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

#include <Corrade/TestSuite/Tester.h>

#include <Magnum/Magnum.h>
#include <Magnum/Math/Angle.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/Object.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/SceneGraph/MatrixTransformation3D.h>

#include "Magnum/Octree/OctreeDrawableGroup.h"
#include "Magnum/Octree/OctreeDrawableGroup.hpp"


namespace Magnum { namespace SceneGraph { namespace Test {

using namespace Magnum::Math::Literals;

typedef Object<MatrixTransformation3D> Object3D;
typedef Scene<MatrixTransformation3D> Scene3D;

/* @brief Drawable3D implementation for testing */
class TestDrawable: public Object3D, public Drawable3D {
    public:
        explicit TestDrawable(Object3D* parent): Object3D(parent), Drawable3D(*this, nullptr) {}

        virtual void draw(const Matrix4&, Camera3D&) {}
};

/* @brief Test for OctreeDrawableGroup */
struct OctreeDrawableGroupTest: Corrade::TestSuite::Tester {
    explicit OctreeDrawableGroupTest();

    void cull();
    void cullContained();
};

OctreeDrawableGroupTest::OctreeDrawableGroupTest() {
    addTests({&OctreeDrawableGroupTest::cull,
              &OctreeDrawableGroupTest::cullContained});
}

void OctreeDrawableGroupTest::cull() {
    /* Setup scene */
    Scene3D scene;

    Camera3D camera{scene};
    camera.setProjectionMatrix(Matrix4::perspectiveProjection(90.0_degf, 1.0f, 0.01f, 100.0f));

    constexpr Vector3 size0{1.0f, 1.0f, 1.0f};
    constexpr Vector3 size1{0.5f, 0.5f, 1.5f};

    TestDrawable drawable0{&scene};
    TestDrawable drawable1{&scene};

    drawable0.translate({0.0f, 0.0f, -5.0f}); /* visible */
    drawable1.translate({0.0f, 0.0f, 5.0f}); /* hidden */

    /* Create the OctreeDrawableGroup */
    OctreeDrawableGroup<Float> culledGroup;
    culledGroup.add(drawable0, Range3D{drawable0.transformation().translation() - size0,
                                       drawable0.transformation().translation() + size0});
    culledGroup.add(drawable1, Range3D{drawable1.transformation().translation() - size1,
                                       drawable1.transformation().translation() + size1});
    culledGroup.buildOctree();
    culledGroup.cull(camera);

    /* There should only be one object visible */
    CORRADE_COMPARE(culledGroup.size(), 1);
    /* The visible object should be drawable0 */
    CORRADE_COMPARE(&culledGroup[0], &drawable0);
}

void OctreeDrawableGroupTest::cullContained() {
    /* Setup scene */
    Scene3D scene;

    Camera3D camera{scene};
    camera.setProjectionMatrix(Matrix4::perspectiveProjection(90.0_degf, 1.0f, 0.01f, 100.0f));

    constexpr Vector3 size0{150.0f};

    TestDrawable drawable0{&scene};

    drawable0.translate({0.0f, 0.0f, 9.0f}); /* visible */

    /* Create the OctreeDrawableGroup */
    OctreeDrawableGroup<Float> culledGroup;
    culledGroup.add(drawable0, Range3D{drawable0.transformation().translation() - size0,
                                       drawable0.transformation().translation() + size0});
    culledGroup.buildOctree();
    culledGroup.cull(camera);

    /* The object should be visible */
    CORRADE_COMPARE(culledGroup.size(), 1);
    CORRADE_COMPARE(&culledGroup[0], &drawable0);
}

}}}

CORRADE_TEST_MAIN(Magnum::SceneGraph::Test::OctreeDrawableGroupTest)
