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

/* Subset of Magnum/Shaders/compatibility.glsl with just things relevant for
   GLSL 3.30+ and GLSL ES 3.00+ */

#if !defined(GL_ES) && defined(GL_ARB_shading_language_420pack) && !defined(MAGNUM_DISABLE_GL_ARB_shading_language_420pack)
    #extension GL_ARB_shading_language_420pack: enable
    #define RUNTIME_CONST
    #define EXPLICIT_BINDING
#endif

#if !defined(GL_ES) && defined(GL_ARB_explicit_uniform_location) && !defined(MAGNUM_DISABLE_GL_ARB_explicit_uniform_location)
    #extension GL_ARB_explicit_uniform_location: enable
    #define EXPLICIT_UNIFORM_LOCATION
#endif

#ifdef GL_ES
    #if __VERSION__ >= 310 || (defined(MAGNUM_GLSL_VERSION) && MAGNUM_GLSL_VERSION >= 310)
        #define EXPLICIT_BINDING
        #define EXPLICIT_UNIFORM_LOCATION
    #endif
    /* RUNTIME_CONST is not available in OpenGL ES */
#endif

/* This is added compared to Magnum/Shaders/compatibility.glsl. Since UI is 2D,
   noperspective is useful a lot, and it helps quite significantly. */
#if defined(GL_ES) && defined(GL_NV_shader_noperspective_interpolation)
    #extension GL_NV_shader_noperspective_interpolation: require
#endif
#if !defined(GL_ES) || defined(GL_NV_shader_noperspective_interpolation)
    #define NOPERSPECTIVE noperspective
#else
    #define NOPERSPECTIVE
#endif
