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

#include <feather.h>

#include "misc.h"


/* Keep this separated as we #undefine the malloc/realloc/free guards here. */
#undef malloc
#undef realloc
#undef free

static void* (*fn_malloc_)(size_t);
static void* (*fn_realloc_)(void*, size_t);
static void (*fn_free_)(void*);

void*
fe_malloc(size_t size)
{
    void* mem;

    /* In the (most common) case with the defaults, do a direct call as it's
     * on modern CPUs cheaper. */
    mem = (fn_malloc_ == NULL) ? malloc(size) : fn_malloc_(size);
    if(mem == NULL)
        XTRACE_E("malloc() failed.");
    return mem;
}

void*
fe_realloc(void* mem, size_t size)
{
    /* Ditto. */
    mem = (fn_realloc_ == NULL) ? realloc(mem, size) : fn_realloc_(mem, size);
    if(mem == NULL)
        XTRACE_E("realloc() failed.");
    return mem;
}

void
fe_free(void* ptr)
{
    /* Ditto. */
    (fn_free_ == NULL) ? free(ptr) : fn_free_(ptr);
}


/* Called from misc.c */
void
fe_set_allocator(const FeAllocator* allocator)
{
    if(allocator != NULL) {
        fn_malloc_ = allocator->fnMalloc;
        fn_realloc_ = allocator->fnRealloc;
        fn_free_ = allocator->fnFree;

        XTRACE_ALWAYS("Using custom allocator.");
    } else {
        fn_malloc_ = NULL;
        fn_realloc_ = NULL;
        fn_free_ = NULL;
    }
}
