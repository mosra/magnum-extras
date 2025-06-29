#ifndef Magnum_Ui_Ui_h
#define Magnum_Ui_Ui_h
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

/** @file
 * @brief Forward declarations for the @ref Magnum::Ui namespace
 */

#include "Magnum/Magnum.h"

#ifndef DOXYGEN_GENERATING_OUTPUT
namespace Magnum { namespace Ui {

enum class DataHandle: UnsignedLong;
enum class LayerHandle: UnsignedShort;
enum class LayerDataHandle: UnsignedInt;
enum class NodeHandle: UnsignedInt;
enum class LayoutHandle: UnsignedLong;
enum class LayouterHandle: UnsignedShort;
enum class LayouterDataHandle: UnsignedInt;
enum class AnimationHandle: UnsignedLong;
enum class AnimatorHandle: UnsignedShort;
enum class AnimatorDataHandle: UnsignedInt;

enum class NodeFlag: UnsignedByte;
typedef Containers::EnumSet<NodeFlag> NodeFlags;

class AbstractAnimator;
class AbstractGenericAnimator;
class AbstractNodeAnimator;
class AbstractDataAnimator;
class AbstractStyleAnimator;
class AbstractLayer;
class AbstractLayouter;
class AbstractRenderer;
class AbstractUserInterface;

class AbstractVisualLayer;
class AbstractVisualLayerStyleAnimator;

class BaseLayer;
struct BaseLayerCommonStyleUniform;
struct BaseLayerStyleUniform;
class BaseLayerStyleAnimator;
#ifdef MAGNUM_TARGET_GL
class BaseLayerGL;
#endif
enum class BaseLayerSharedFlag: UnsignedByte;
typedef Containers::EnumSet<BaseLayerSharedFlag> BaseLayerSharedFlags;

class DebugLayer;
#ifdef MAGNUM_TARGET_GL
class DebugLayerGL;
#endif

class EventConnection;
class EventLayer;

class LineLayer;
struct LineLayerCommonStyleUniform;
struct LineLayerStyleUniform;
#ifdef MAGNUM_TARGET_GL
class LineLayerGL;
#endif
enum class LineCapStyle: UnsignedByte;
enum class LineJoinStyle: UnsignedByte;
enum class LineAlignment: UnsignedByte;

enum class FontHandle: UnsignedShort;
class TextLayer;
struct TextLayerCommonStyleUniform;
struct TextLayerStyleUniform;
struct TextLayerCommonEditingStyleUniform;
struct TextLayerEditingStyleUniform;
class TextLayerStyleAnimator;
#ifdef MAGNUM_TARGET_GL
class TextLayerGL;
#endif
class TextFeatureValue;
class TextProperties;

class GenericAnimator;
class GenericNodeAnimator;
class GenericDataAnimator;

class RendererGL;

enum class StyleFeature: UnsignedByte;
typedef Containers::EnumSet<StyleFeature, 31> StyleFeatures;
class AbstractStyle;

enum class Icon: UnsignedInt;
class UserInterface;
class UserInterfaceGL;

class AbstractAnchor;
template<class> class BasicAnchor;
typedef BasicAnchor<UserInterface> Anchor;

class AbstractWidget;
template<class> class BasicWidget;
typedef BasicWidget<UserInterface> Widget;

class SnapLayouter;
class AbstractSnapLayout;
template<class> class BasicSnapLayout;
typedef BasicSnapLayout<UserInterface> SnapLayout;

enum class Pointer: UnsignedByte;
typedef Containers::EnumSet<Pointer> Pointers;
class PointerEvent;
class PointerMoveEvent;
class PointerCancelEvent;

class ScrollEvent;

class FocusEvent;

enum class Key: UnsignedShort;
enum class Modifier: UnsignedByte;
typedef Containers::EnumSet<Modifier> Modifiers;
class KeyEvent;

class TextInputEvent;

class VisibilityLostEvent;

}}
#endif

#endif
