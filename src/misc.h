/*
 * FeatherUI
 * <http://github.com/mity/featherui>
 *
 * Copyright (c) 2026 Martin Mitáš
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

#include "libex/defs.h"


/* Helper debug logging via DebugOutputString().
 * Noop in Release builds.
 */
#ifdef DEBUG
    void debug_trace__(const char* fname, const char* funcname, unsigned line, const char* fmt, ...);
    void debug_dump__(const char* fname, const char* funcname, unsigned line, const char* msg, const void* addr, size_t size);

    #define TRACE(...)              debug_trace__(__FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
    #define TRACE_ERR_(fnname, err) debug_trace__(__FILE__, __FUNCTION__, __LINE__, "%hs failed. [%lu]", (fnname), (err))
    #define TRACE_ERR(fnname)       debug_trace__(__FILE__, __FUNCTION__, __LINE__, "%hs failed. [%lu]", (fnname), GetLastError())
    #define TRACE_HR_(fnname, hr)   debug_trace__(__FILE__, __FUNCTION__, __LINE__, "%hs failed. [0x%lx]", (fnname), (hr))
    #define TRACE_HR(fnname)        debug_trace__(__FILE__, __FUNCTION__, __LINE__, "%hs failed. [0x%lx]", (fnname), hr)
    #define DUMP(msg, addr, size)   debug_dump__(__FILE__, __FUNCTION__, __LINE__, (msg), (addr), (size))
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

static inline LONG
rect_width(const RECT* rect)
{
    return rect->right - rect->left;
}

static inline LONG
rect_height(const RECT* rect)
{
    return rect->bottom - rect->top;
}


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
