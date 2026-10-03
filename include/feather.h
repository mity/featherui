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

#ifndef FEATHER_H
#define FEATHER_H

#include <feather/_internal.h>
#include <feather/appwindow.h>
#include <feather/version.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct FeAllocator FeAllocator;
struct FeAllocator {
    DWORD cbSize;
    void* (*fnMalloc)(size_t);
    void* (*fnRealloc)(void*, size_t);
    void (*fnFree)(void*);
};


/**
 * Initialize FeatherUI library. Must be called before any other call to the
 * library is done.
 *
 * A typical FeatherUI-based application calls this function once during
 * its startup, before creating any application window.
 *
 * @param lpcAllocator Optional caller-provided custom allocator. If `NULL`,
 * the standard `malloc()`, `realloc()` and `free()` are used.
 * @param dwFlags Reserved, set to zero.
 *
 * @return If the function succeeds, `S_OK` (0) is returned.
 */
HRESULT FEATHER_API feInitialize(const FeAllocator* lpcAllocator, DWORD dwFlags);

/**
 * Release any resources the library may have allocated in previous
 * call to `feInitialize()`. This includes destruction of all windows
 * and controls, the library manages.
 *
 * A typical FeatherUI-based application calls this function once before its
 * termination.
 */
void FEATHER_API feTerminate(void);


/**
 * This function executes the main user-interface loop.
 *
 * Unless there is an unrecoverable issue, the function doesn't return until
 * `feQuit()` is called.
 *
 * @param piExitCode Optional. If not `NULL`, the function sets it suggested
 * exit code to return from `WinMain()`. If the function returns due a call
 * to `feQuit()`, the exit code from the call propagates here.
 * If `feRuns()` returns due a fatal internal error, it is set to
 * `EXIT_FAILURE`.
 *
 * @returns `S_OK` if the function executed normally due `feQuit()`.
 */
HRESULT FEATHER_API feRun(int* piExitCode);

void FEATHER_API feQuit(int iExitCode);


#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* FEATHER_H */
