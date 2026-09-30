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

#ifndef FE_MISC_H
#define FE_MISC_H

#include <limits.h>
#include <malloc.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <wchar.h>
#include <windows.h>

#include <feather/_internal.h>


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


/*******************************
 *** Helper Debugging Macros ***
 *******************************/

/* <assert.h> by default enables the assertions even for release or production
 * code. But especially for a lot of code it makes a good sense to have a macro
 * assertion which is by default enabled in an explicit debug build; and in
 * release builds a noop or even use the provided invariant as a potentially
 * valuable hint to the optimizer.
 *
 * Caution: If the invariant provided by the caller shows false in a non-debug
 * build, an undefined behavior follows.
 */
#ifdef DEBUG
    #define ASSERT(x)                                                     \
        do {                                                              \
            if(!(x)) {                                                    \
                const char msg[] =                                        \
                            __FILE__ ":" STRINGIZE_A(__LINE__) ": "       \
                            "Assertion '" #x "' failed.";                 \
                TRACE(msg);                                               \
                if(IsDebuggerPresent()) {                                 \
                    __debugbreak();                                       \
                } else {                                                  \
                    MessageBoxA(NULL, msg, "Assert", MB_OK);              \
                    abort();                                              \
                }                                                         \
            }                                                             \
        } while(0)
#elif defined __GNUC__
    #define ASSERT(x)               do { if(!(x)) __builtin_unreachable(); } while(0)
#elif defined __clang__
    #define ASSERT(x)               do { if(!(x)) __builtin_unreachable(); } while(0)
#elif defined _MSC_VER  &&  _MSC_VER > 120
    #define ASSERT(x)           __assume(x)
#else
    #define ASSERT(x)               do {} while(0)
#endif

/* Probably every compiler knows it nowadays, but lets have it in upper case
 * for the sake of consistency.
 */
#define STATIC_ASSERT(x)            static_assert(x)

/* A macro to declare that the given code branch is unreachable.
 *
 * In a debug build, reaching the point will be treated as a failed assertion
 * (i.e. the program aborted); in release builds the knowledge may be used by
 * the compiler as a hint for optimizer that the given is never executed
 *
 * Caution: If the program does take it in a release build, an undefined
 * behavior follows.
 */
#if defined DEBUG
    #define UNREACHABLE             ASSERT(0)
#elif defined __STDC_VERSION__  &&  __STDC_VERSION__ >= 202311
    #define UNREACHABLE             unreachable()
#else
    #define UNREACHABLE             ASSERT(0)
#endif


/* Helper debug logging via DebugOutputString().
 * Noop in Release builds.
 */
#ifdef DEBUG
    void debug_trace__(const char* fmt, ...);
    void debug_dump__(const char* msg, const void* addr, size_t size);

    #define TRACE(...)              debug_trace__(__VA_ARGS__)
    #define TRACE_ERR_(fnname, err) debug_trace__("%s failed. [%lu]", (fnname), (err))
    #define TRACE_ERR(fnname)       debug_trace__("%s failed. [%lu]", (fnname), GetLastError())
    #define TRACE_HR_(fnname, hr)   debug_trace__("%s failed. [%lu]", (fnname), (hr))
    #define TRACE_HR(fnname)        debug_trace__("%s failed. [%lu]", (fnname), hr)
    #define DUMP(msg, addr, size)   debug_dump__((msg), (addr), (size))
#else
    #define TRACE(...)              do {} while(0)
    #define TRACE_ERR_(fnname, err) do {} while(0)
    #define TRACE_ERR(fnname)       do {} while(0)
    #define TRACE_HR_(fnname, hr)   do {} while(0)
    #define TRACE_HR(fnname)        do {} while(0)
    #define DUMP(msg, addr, size)   do {} while(0)
#endif


/**************************************
 *** Win32 API Convenience Wrappers ***
 **************************************/

/* Avoid over-sensitive gcc warning
 * (cast between incompatible function types from 'FARPROC').
 */
#define GetProcAddress(dll,symbol_name)         ((void*)GetProcAddress((dll), (symbol_name)))

/* MS platform SDK #defines [GS]etWindowLongPtr as plain [GS]etWindowLong
 * for x86 builds without any casting, hence causing lots of compiler
 * warnings C4312. Lets workaround it.
 */
#ifndef _WIN64
    #ifdef GetWindowLongPtrA
        #undef GetWindowLongPtrA
        #define GetWindowLongPtrA(win,ix)      ((intptr_t)GetWindowLongA((win),(ix)))
    #endif
    #ifdef GetWindowLongPtrW
        #undef GetWindowLongPtrW
        #define GetWindowLongPtrW(win,ix)      ((intptr_t)GetWindowLongW((win),(ix)))
    #endif
    #ifdef SetWindowLongPtrA
        #undef SetWindowLongPtrA
        #define SetWindowLongPtrA(win,ix,val)  SetWindowLongA((win),(ix),(LONG)(val))
    #endif
    #ifdef SetWindowLongPtrW
        #undef SetWindowLongPtrW
        #define SetWindowLongPtrW(win,ix,val)  SetWindowLongW((win),(ix),(LONG)(val))
    #endif
#endif


/********************************
 *** Custom Allocator Support ***
 ********************************/

extern void* (*fn_malloc_)(size_t);
extern void* (*fn_realloc_)(void*, size_t);
extern void (*fn_free_)(void*);

INLINE void*
fe_malloc(size_t size)
{
    void* mem;

    /* In the (most common) case with the defaults, do a direct call as it's
     * on modern CPUs cheaper. */
    mem = (fn_malloc_ == NULL) ? malloc(size) : fn_malloc_(size);
    if(mem == NULL)
        TRACE("malloc() failed.");
    return mem;
}

INLINE void*
fe_realloc(void* mem, size_t size)
{
    /* Ditto. */
    mem = (fn_realloc_ == NULL) ? realloc(mem, size) : fn_realloc_(mem, size);
    if(mem == NULL)
        TRACE("realloc() failed.");
    return mem;
}

INLINE void
fe_free(void* ptr)
{
    /* Ditto. */
    (fn_free_ == NULL) ? free(ptr) : fn_free_(ptr);
}

/* Make sure we don't accidentally use the plain malloc/realloc/free, as
 * and mix-up of the standard libc allocator and app-provided allocator might
 * be hard to debug.
 *
 * We also don't "#define malloc fe_malloc", as (a) there's a little bit logic
 * and (b) caller should be aware of it.
 */
#define malloc      DONT_USE_standard_malloc___INSTEAD_USE_fe_malloc
#define realloc     DONT_USE_standard_realloc___INSTEAD_USE_fe_realloc
#define free        DONT_USE_standard_free___INSTEAD_USE_fe_free


/**************************
 *** Few Useful Globals ***
 **************************/

extern HMODULE dll_feather;
extern HMODULE dll_kernel32;
extern HMODULE dll_ntdll;


#endif  /* FE_MISC_H */
