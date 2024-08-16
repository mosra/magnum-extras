#ifndef Magnum_Whee_AbstractRenderer_h
#define Magnum_Whee_AbstractRenderer_h
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
 * @brief Class @ref Magnum::Whee::AbstractRenderer, enum @ref Magnum::Whee::RendererFeature, @ref Magnum::Whee::RendererTargetState, @ref Magnum::Whee::RendererDrawState, enum set @ref Magnum::Whee::RendererFeatures, @ref Magnum::Whee::RendererDrawStates
 * @m_since_latest
 */

#include <Corrade/Containers/EnumSet.h>
#include <Corrade/Containers/Pointer.h>

#include "Magnum/Whee/Whee.h"
#include "Magnum/Whee/visibility.h"

namespace Magnum { namespace Whee {

/**
@brief Renderer feature
@m_since_latest

@see @ref RendererFeatures, @ref AbstractRenderer::features()
*/
enum class RendererFeature {
    /**
     * Ability to composite from the default framebuffer. If supported, the
     * renderer is able to not only draw into the default framebuffer but also
     * read from it, making it possible for @ref AbstractLayer::doComposite()
     * to access framebuffer contents in a renderer-implementation-specific
     * way. If supported, it's possible to transition from and to
     * @ref RendererTargetState::Composite.
     */
    Composite = 1 << 0,
};

/**
@debugoperatorenum{RendererFeature}
@m_since_latest
*/
MAGNUM_WHEE_EXPORT Debug& operator<<(Debug& debug, RendererFeature value);

/**
@brief Renderer features
@m_since_latest

@see @ref AbstractRenderer::features()
*/
typedef Containers::EnumSet<RendererFeature> RendererFeatures;

/**
@debugoperatorenum{RendererFeatures}
@m_since_latest
*/
MAGNUM_WHEE_EXPORT Debug& operator<<(Debug& debug, RendererFeatures value);

CORRADE_ENUMSET_OPERATORS(RendererFeatures)

/**
@brief Renderer target state
@m_since_latest

@see @ref AbstractRenderer::currentTargetState(),
    @ref AbstractRenderer::transition(), @ref RendererDrawState
*/
enum class RendererTargetState {
    /**
     * Initial state. Can be transitioned from:
     *
     * -    @ref RendererTargetState::Initial, in which case the transition is
     *      a no-op;
     * -    @relativeref{RendererTargetState,Final}, in which case the previous
     *      contents of the default framebuffer can be forgotten.
     *
     * The corresponding @ref RendererDrawStates are expected to be empty when
     * transitioning to this state. Can be only transitioned to
     * @relativeref{RendererTargetState,Initial},
     * @relativeref{RendererTargetState,Draw} or
     * @relativeref{RendererTargetState,Final}.
     */
    Initial,

    /**
     * Drawing a layer. Can be transitioned from:
     *
     * -    @ref RendererTargetState::Initial, in which case the default
     *      framebuffer should be bound for drawing;
     * -    @relativeref{RendererTargetState,Draw}, in which case the default
     *      framebuffer should stay bound;
     * -    @relativeref{RendererTargetState,Composite}, in which case the
     *      default framebuffer should be bound for drawing, with the
     *      assumption that the compositing operation used some other
     *      framebuffer.
     *
     * Can be only transitioned to @relativeref{RendererTargetState,Draw},
     * @relativeref{RendererTargetState,Composite} and
     * @relativeref{RendererTargetState,Final}.
     */
    Draw,

    /**
     * Compositing a layer. Used only if @ref RendererFeature::Composite is
     * supported. Can be transitioned from:
     *
     * -    @ref RendererTargetState::Initial or
     *      @relativeref{RendererTargetState,Draw}, in which case the contents
     *      of the default framebuffer should be made available in a
     *      renderer-implementation-specific way for use by
     *      @ref AbstractLayer::doComposite(), with the assumption that the
     *      compositing operation uses some other framebuffer as a target.
     *
     * The corresponding @ref RendererDrawStates are expected to be empty when
     * transitioning to this state. Can be only transitioned to
     * @relativeref{RendererTargetState,Draw}.
     */
    Composite,

    /**
     * Final state. Can be transitioned from:
     *
     * -    @ref RendererTargetState::Initial or
     *      @relativeref{RendererTargetState,Draw}, in which case the default
     *      framebuffer should stay bound.
     *
     * The corresponding @ref RendererDrawStates are expected be empty when
     * transitioning to this state. Can be only transitioned to
     * @relativeref{RendererTargetState,Initial}.
     */
    Final
};

/**
@debugoperatorenum{RendererTargetState}
@m_since_latest
*/
MAGNUM_WHEE_EXPORT Debug& operator<<(Debug& debug, RendererTargetState value);

/**
@brief Renderer draw state
@m_since_latest

@see @ref RendererDrawStates, @ref AbstractRenderer::currentDrawStates(),
    @ref AbstractRenderer::transition()
*/
enum class RendererDrawState {
    /**
     * Blending is active. Gets enabled when drawing a layer that advertises
     * @ref LayerFeature::DrawUsesBlending, and disabled again when drawing a
     * layer that doesn't advertise it, or after drawing everything.
     */
    Blending = 1 << 0,

    /**
     * Scissor is active. Gets enabled when drawing a layer that advertises
     * @ref LayerFeature::DrawUsesScissor, and disabled again when drawing a
     * layer that doesn't advertise it, or after drawing everything.
     */
    Scissor = 1 << 1
};

/**
@debugoperatorenum{RendererDrawState}
@m_since_latest
*/
MAGNUM_WHEE_EXPORT Debug& operator<<(Debug& debug, RendererDrawState value);

/**
@brief Renderer draw states
@m_since_latest

@see @ref AbstractRenderer::currentDrawStates(),
    @ref AbstractRenderer::transition()
*/
typedef Containers::EnumSet<RendererDrawState> RendererDrawStates;

/**
@debugoperatorenum{RendererDrawStates}
@m_since_latest
*/
MAGNUM_WHEE_EXPORT Debug& operator<<(Debug& debug, RendererDrawStates value);

CORRADE_ENUMSET_OPERATORS(RendererDrawStates)

/**
@brief Base for renderer implementations
@m_since_latest

A renderer implementation handles GPU-API-specific framebuffer switching,
clearing and draw state setup. You'll most likely instantiate the class through
@ref RendererGL, which contains a concrete OpenGL implementation.
@see @ref AbstractUserInterface::setRendererInstance()
*/
class MAGNUM_WHEE_EXPORT AbstractRenderer {
    public:
        /** @brief Constructor */
        explicit AbstractRenderer();

        /** @brief Copying is not allowed */
        AbstractRenderer(const AbstractRenderer&) = delete;

        /**
         * @brief Move constructor
         *
         * Performs a destructive move, i.e. the original object isn't usable
         * afterwards anymore.
         */
        AbstractRenderer(AbstractRenderer&&) noexcept;

        virtual ~AbstractRenderer();

        /** @brief Copying is not allowed */
        AbstractRenderer& operator=(const AbstractRenderer&) = delete;

        /** @brief Move assignment */
        AbstractRenderer& operator=(AbstractRenderer&&) noexcept;

        /** @brief Features supported by a renderer */
        RendererFeatures features() const { return doFeatures(); }

        /**
         * @brief Framebuffer size
         *
         * Initial state is a zero vector. Use @ref setupFramebuffers() to set
         * up framebuffer properties.
         */
        Vector2i framebufferSize() const;

        /**
         * @brief Current target state
         *
         * Initial state is @ref RendererTargetState::Initial. Gets
         * subsequently updated with the states passed to @ref transition().
         */
        RendererTargetState currentTargetState() const;

        /**
         * @brief Current draw states
         *
         * Initial state is an empty set. Gets subsequently updated with the
         * states passed to @ref transition().
         */
        RendererDrawStates currentDrawStates() const;

        /**
         * @brief Set up framebuffer properties
         *
         * Used internally from @ref AbstractUserInterface::setSize(). Exposed
         * just for testing purposes, there should be no need to call this
         * function directly and doing so may cause internal
         * @ref AbstractUserInterface state update to misbehave. Allowed to be
         * called only if @ref currentTargetState() is
         * @ref RendererTargetState::Initial or
         * @relativeref{RendererTargetState,Final}. Delegates to
         * @ref doSetupFramebuffers(), see its documentation for more
         * information about the arguments.
         */
        void setupFramebuffers(const Vector2i& size);

        /**
         * @brief Transition to the next renderer state
         *
         * Used internally from @ref AbstractUserInterface::draw(). Exposed
         * just for testing purposes, there should be no need to call this
         * function directly and doing so may cause internal
         * @ref AbstractUserInterface state update to misbehave. The
         * @p targetState is expected to be an allowed transition from
         * @ref currentTargetState() and @p drawStates is expected to match
         * restrictions of given @p targetState. If the set of states is
         * different from @ref currentTargetState() and
         * @ref currentDrawStates(), delegates to @ref doTransition(), see its
         * documentation for more information about the arguments. If the set
         * is the same, the function is a no-op.
         */
        void transition(RendererTargetState targetState, RendererDrawStates drawStates);

    private:
        /** @brief Implementation for @ref features() */
        virtual RendererFeatures doFeatures() const = 0;

        /**
         * @brief Set up framebuffer properties
         * @param size                      Framebuffer size in pixels for
         *      allocating custom framebuffer memory, resetting the scissor
         *      rectangle and other framebuffer-related operations.
         *
         * Implementation for @ref setupFramebuffers(), which is called from
         * @ref AbstractUserInterface::setSize(). Is guaranteed to be called
         * only if @ref currentTargetState() is either
         * @ref RendererTargetState::Initial or
         * @ref RendererTargetState::Final, i.e. before any @ref doTransition()
         * call that transitions to other states.
         */
        virtual void doSetupFramebuffers(const Vector2i& size) = 0;

        /**
         * @brief Transition to the next renderer state
         * @param targetStateFrom   Target state to transition from. Equal to
         *      @ref currentTargetState().
         * @param targetStateTo     Target state to transition to
         * @param drawStatesFrom    Draw states to transition from. Equal to
         *      @ref currentDrawStates()
         * @param drawStatesTo      Draw states to transition to
         *
         * Implementation for @ref transition(), which is called from
         * @ref AbstractUserInterface::draw() before, between and after calls
         * to @ref AbstractLayer::draw() based on whether they advertise
         * @ref LayerFeature::DrawUsesBlending or
         * @relativeref{LayerFeature,DrawUsesScissor}. The @p targetStateFrom
         * and @p targetStateTo values are guaranteed to be one of the
         * following combinations:
         *
         * -    @ref RendererTargetState::Initial to
         *      @relativeref{RendererTargetState,Draw}.
         * -    @relativeref{RendererTargetState,Initial} to
         *      @relativeref{RendererTargetState,Final}. The @p drawStatesTo is
         *      guaranteed to be empty in this case.
         * -    @relativeref{RendererTargetState,Initial} to
         *      @relativeref{RendererTargetState,Composite}. Is guaranteed to
         *      be called only if @ref RendererFeature::Composite is supported.
         * -    @relativeref{RendererTargetState,Draw} to
         *      @relativeref{RendererTargetState,Draw}. Called only if the
         *      @p drawStatesFrom and @p drawStatesTo differ.
         * -    @relativeref{RendererTargetState,Draw} to
         *      @relativeref{RendererTargetState,Composite}. Is guaranteed to
         *      be called only if @ref RendererFeature::Composite is supported.
         * -    @relativeref{RendererTargetState,Draw} to
         *      @relativeref{RendererTargetState,Final}
         * -    @relativeref{RendererTargetState,Composite} to
         *      @relativeref{RendererTargetState,Draw}. Is guaranteed to
         *      be called only if @ref RendererFeature::Composite is supported.
         * -    @relativeref{RendererTargetState,Final} to
         *      @relativeref{RendererTargetState,Initial}. Both
         *      @p drawStatesFrom and @p drawStatesTo are guaranteed to be
         *      empty in this case.
         *
         * The @relativeref{RendererTargetState,Initial} to
         * @relativeref{RendererTargetState,Initial} transition, while allowed
         * in @ref transition(), is effectively a no-op so it doesn't propagate
         * here.
         *
         * In each @ref AbstractUserInterface::draw() invocation, the renderer
         * is guaranteed to start at @ref RendererTargetState::Initial, go
         * through zero or more @relativeref{RendererTargetState,Draw} states,
         * optionally preceded by @relativeref{RendererTargetState,Composite},
         * and end up at the @relativeref{RendererTargetState,Final} state.
         */
        virtual void doTransition(RendererTargetState targetStateFrom, RendererTargetState targetStateTo, RendererDrawStates drawStatesFrom, RendererDrawStates drawStatesTo) = 0;

        struct State;
        Containers::Pointer<State> _state;
};

}}

#endif
