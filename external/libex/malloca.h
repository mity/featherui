/*
 * libex
 * <http://github.com/mity/libex>
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

#ifndef EX_MALLOCA_H
#define EX_MALLOCA_H

#include <stdlib.h>

#ifdef _WIN32
    /* Windows do not have <alloca.h>.
     * There is _alloca() in <malloc.h> instead. */
    #include <malloc.h>
    #define EX_func_alloca__ _alloca
#else
    #include <alloca.h>
    #define EX_func_alloca__ alloca
#endif


/* Rudimentary custom allocator support. */
#ifdef MALLOC_FUNC
    void* MALLOC_FUNC(size_t);
    #define EX_func_malloc__    MALLOC_FUNC
#else
    #define EX_func_malloc__    malloc
#endif
#ifdef FREE_FUNC
    void FREE_FUNC(void*);
    #define EX_func_free__      FREE_FUNC
#else
    #define EX_func_free__      free
#endif


#ifdef __cplusplus
extern "C" {
#endif


/* On resource-limited platforms with smaller stacks (e.g. on embedded systems)
 * you may want to lower this threshold. MALLOCA allocations smaller than this
 * are allocated on stack, larger on heap.
 */
#ifndef MALLOCA_THRESHOLD
    #define MALLOCA_THRESHOLD         (1024 - sizeof(void*))
#endif


static inline void*
malloca_set_mark__(void* ptr, int mark)
{
    if(ptr != NULL) {
        int* x = (int*)ptr;
        *x = mark;
        ptr = (void*)((char*)ptr + sizeof(void*));
    }
    return ptr;
}


/* Allocate block of memory via malloc() or alloca(), depending on the
 * requested amount of memory.
 *
 * MALLOCA uses the default threshold defined by MALLOCA_THRESHOLD,
 * while MALLOCA_ allows you to specify it with its second parameter explicitly
 * on each call site.
 *
 * Returns pointer to the memory block or NULL on failure. When not needed
 * anymore, release it with FREEA().
 */
#define MALLOCA_(size, threshold)                                             \
    ((size) < (threshold) - sizeof(void*)                                     \
        ? malloca_set_mark__(EX_func_alloca__(size + sizeof(void*)), 0xcccc)  \
        : malloca_set_mark__(EX_func_malloc__(size + sizeof(void*)), 0xdddd))

#define MALLOCA(size)       MALLOCA_((size), MALLOCA_THRESHOLD)

/* Release any memory allocated with MALLOCA() or MALLOCA_().
 */
/* Release any memory allocated with MALLOCA() or MALLOCA_().
 */
#define FREEA(ptr)                                                            \
    do {                                                                      \
        if((ptr) != NULL) {                                                   \
            int* x = (int*)((char*)(ptr) - sizeof(void*));                    \
            if(*x == 0xdddd)                                                  \
                EX_func_free__(x);                                            \
        }                                                                     \
    } while(0)


#ifdef __cplusplus
}  /* extern "C" { */
#endif

#endif  /* EX_MALLOCA_H */
