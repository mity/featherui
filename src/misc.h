/*
 * FeatherUI
 * <http://github.com/mity/featherui>
 *
 * Copyright (c) 2026 Martin Mitáš
 * SPDX-License-Identifier: GPL-2.0-or-later OR LicenseRef-Commercial
 *
 * This file is part of FeatherUI project.
 *
 * This software is available under either of the following licenses:
 * - GNU General Public License v2.0 or later; see LICENSE.gpl.md.
 * - Commercial license; see LICENSE.commercial.md.
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
#include "libex/malloca.h"
#include "xtrace.h"


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

void* fe_malloc(size_t size);
void* fe_realloc(void* ptr, size_t size);
void fe_free(void* ptr);

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

extern WCHAR fe_locale_name[LOCALE_NAME_MAX_LENGTH];

extern HMODULE dll_feather;
extern HMODULE dll_kernel32;
extern HMODULE dll_ntdll;


#endif  /* FE_MISC_H */
