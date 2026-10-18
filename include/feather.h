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
 * @param pszLocaleName Locale name or `NULL` to use default user's locale.
 * @param lpcAllocator Optional caller-provided custom allocator. If `NULL`,
 * the standard `malloc()`, `realloc()` and `free()` are used.
 * @param dwFlags Reserved, set to zero.
 *
 * @return If the function succeeds, `S_OK` (0) is returned.
 */
HRESULT FEATHER_API feInitialize(const WCHAR* pszLocaleName, const FeAllocator* lpcAllocator, DWORD dwFlags);

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
