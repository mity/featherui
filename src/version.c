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

#include <windows.h>
#include <shlwapi.h>

#include "misc.h"
#include "project.h"
#include "feather/version.h"


void FEATHER_API
feVersion(FeVersion* lpVersion)
{
    /* +0 is a trick for case the macro(s) are defined but having an empty.
     * THis is currently happening for our CMake build as we actually use
     * just major, minor, patch numbers. */
    lpVersion->dwMajor = PROJECT_MAJOR_VERSION;
    lpVersion->dwMinor = PROJECT_MINOR_VERSION;
    lpVersion->dwPatch = PROJECT_PATCH_VERSION;
}

const WCHAR* FEATHER_API
feVersionStr(void)
{
    return  L""
            STRINGIZE(PROJECT_MAJOR_VERSION)
            "."
            STRINGIZE(PROJECT_MINOR_VERSION)
            "."
            STRINGIZE(PROJECT_PATCH_VERSION)
#ifdef DEBUG
            " (debug)"
#endif
            ;
}


#ifdef FEATHER_BUILD_DLL

/* DllGetVersion accepts pointer to structure determined in runtime
 * by first DWORD (cbSize). */
typedef union DllGetVersionInfo DllGetVersionInfo;
union DllGetVersionInfo {
    DWORD cbSize;
    DLLVERSIONINFO info1;
    DLLVERSIONINFO2 info2;
};


HRESULT FEATHER_API
DllGetVersion(DllGetVersionInfo* dvi)
{
    switch(dvi->cbSize) {
        case sizeof(DLLVERSIONINFO2):
            dvi->info2.dwFlags = 0;
            dvi->info2.ullVersion = MAKEDLLVERULL(PROJECT_MAJOR_VERSION,
                        PROJECT_MINOR_VERSION, PROJECT_PATCH_VERSION, 0);
            FALLTHROUGH();

        case sizeof(DLLVERSIONINFO):
            dvi->info1.dwMajorVersion = PROJECT_MAJOR_VERSION;
            dvi->info1.dwMinorVersion = PROJECT_MINOR_VERSION;
            dvi->info1.dwBuildNumber = PROJECT_PATCH_VERSION;
            dvi->info1.dwPlatformID = DLLVER_PLATFORM_NT;
            return S_OK;

        default:
            XTRACE_E("DllGetVersion: unsupported cbSize");
            return E_INVALIDARG;
    }
}

#endif  /* FEATHER_BUILD_DLL */
