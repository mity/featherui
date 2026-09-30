/*
 * FeatherUI
 * <http://github.com/mity/featherui>
 *
 * Copyright (c) 2016-2026 Martin Mitáš
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef DEMO_MISC_H
#define DEMO_MISC_H

#include <windows.h>
#include <feather.h>


/****************************
 *** Miscellaneous Macros ***
 ****************************/

/* The following macros are so trivial and so widespread that we check whether
 * they are already defined. If so, we leave them untouched in the believe they
 * are morally equivalent to the ones already defined. */

#ifndef MIN
    #define MIN(a,b)                ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
    #define MAX(a,b)                ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN3
    #define MIN3(a,b,c)             MIN(MIN((a), (b)), (c))
#endif

#ifndef MAX3
    #define MAX3(a,b,c)             MAX(MAX((a), (b)), (c))
#endif

/* Clamp a value into a range.
 * Note the result is undefined if v_min > v_max. */
#ifndef CLAMP
    #define CLAMP(v,v_min,v_max)    MIN(MAX((v), (v_min)), (v_max))
#endif

#ifndef ABS
    #define ABS(a)                  ((a) >= 0 ? (a) : -(a))
#endif

#ifndef SIZEOF_ARRAY
    #define SIZEOF_ARRAY(array)     (sizeof((array)) / sizeof((array)[0]))
#endif


/*
 * For declaring static functions in a reasonably portable way.
 */
#if defined __cplusplus
    #define INLINE                  inline
#elif defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901L
    #define INLINE                  static inline
#elif defined __GNUC__
    #define INLINE                  static __inline__
#elif defined _MSC_VER
    #define INLINE                  static __inline
#else
    #define INLINE                  static
#endif


/* Example:
 *
 * typedef struct {
 *     int x;
 *     int y;
 * } MYSTRUCT;
 *
 * size_t
 * get_offset_of_y_in_MYSTRUCT(void)
 * {
 *     return OFFSETOF(MYSTRUCT, y);
 * }
 */
#ifndef OFFSETOF
    #if defined offsetof
        #define OFFSETOF(type, member)      offsetof(type, member)
    #elif defined __GNUC__ && __GNUC__ >= 4
        #define OFFSETOF(type, member)      __builtin_offsetof(type, member)
    #else
        #define OFFSETOF(type, member)      ((size_t) &((type*)0)->member)
    #endif
#endif

/* Example:
 *
 * typedef struct {
 *     int x;
 *     int y;
 * } MYSTRUCT;
 *
 * typedef struct {
 *     int some_member;
 *     MYSTRUCT s_member;
 * } MYCONTAINER;
 *
 * void
 * foo(MYSTRUCT* ptr_to_s)
 * {
 *     MYCONTAINER* container = CONTAINEROF(ptr_to_s, MYCONTAINER, s_member);
 *
 *     ...  // do something with the container.
 * }
 *
 * Of course the example is valid ONLY if we KNOW that PTR_TO_S indeed points
 * to MYSTRUCT inside some MYCONTAINER. The behavior is undefined if it does
 * not.
 */
#ifndef CONTAINEROF
    #define CONTAINEROF(ptr, type, member)  \
                ((type*)((char*)(1 ? (ptr) : &((type *)0)->member) - OFFSETOF(type, member)))
#endif


/* Preprocessor magic for making a string literal from unquoted argument.
 * Example:
 *
 * #define VERSION_MAJOR    5
 * #define VERSION_MINOR    3
 *
 * #define VERSION_STRING_A STRINGIZE_A(VERSION_MAJOR.VERSION_MINOR)  // "5.3"
 * #define VERSION_STRING_W STRINGIZE_A(VERSION_MAJOR.VERSION_MINOR)  // L"5.3"
 */
#ifndef STRINGIZE_A
    #define STRINGIZE_HELPER_A__(a) #a
    #define STRINGIZE_A(a)          STRINGIZE_HELPER_A__(a)
#endif

#ifndef STRINGIZE_W
    #define STRINGIZE_HELPER_W__(w) #w
    #define STRINGIZE_W(w)          L"" STRINGIZE_HELPER_W__(w)
#endif

#ifndef STRINGIZE
    #ifdef UNICODE
        #define STRINGIZE(s)        STRINGIZE_W(s)
    #else
        #define STRINGIZE(s)        STRINGIZE_A(s)
    #endif
#endif

/* Some compilers emit by default warnings when they encounter switch branch
 * which does not end with 'break'. This macro can be used to explicitly inform
 * the compiler that it's intentional.
 */
#ifndef FALLTHOUGH
    #if defined __STDC_VERSION__  &&  __STDC_VERSION__ >= 202311
        #define FALLTHROUGH()       [[fallthrough]]
    #elif defined __clang__ && __clang_major__ >= 12
        #define FALLTHROUGH()       __attribute__((fallthrough))
    #elif defined __GNUC__ && __GNUC__ >= 7
        #define FALLTHROUGH()       __attribute__((fallthrough))
    #else
        #define FALLTHROUGH()       do {} while(0)
    #endif
#endif

/* A macro to declare the given variable/function argument may not be used,
 * in order to suppress compiler warning.
 */
#ifndef UNUSED
    #define UNUSED(x)               ((void)(x))
#endif


#endif  /* DEMO_MISC_H */
