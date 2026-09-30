
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
    return  STRINGIZE(PROJECT_MAJOR_VERSION)
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
            TRACE("DllGetVersion: unsupported cbSize");
            return E_INVALIDARG;
    }
}

#endif  /* FEATHER_BUILD_DLL */
