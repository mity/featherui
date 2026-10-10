/*
 * FeatherUI
 * <http://github.com/mity/featherui>
 *
 * Copyright (c) 2026 Martin Mitáš
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see
 * <https://www.gnu.org/licenses/>.
 */

#include <feather.h>

#include "misc.h"
#include "project.h"
/* The following ones are included just for their init/fini functions. */
#include "appwindow.h"
#include "gfx.h"


HMODULE dll_feather = NULL;
HMODULE dll_kernel32 = NULL;
HMODULE dll_ntdll = NULL;


/*********************************
 *** Detect System Information ***
 *********************************/

/* We detects _actual_ Windows version and avoids the lying GetVersionEx().
 *
 * Currently we just use it for debug logs, and preferably it should stay that
 * way. It's better to detect any availability of lack of Windows features by
 * other means.
 */

#ifdef DEBUG

static void
detect_preferred_languages(bool system)
{
    WCHAR* buffer;
    WCHAR* ptr;
    ULONG buffer_size;
    ULONG n_languages;
    ULONG i;

    if(system)
        GetSystemPreferredUILanguages(MUI_LANGUAGE_NAME, &n_languages, NULL, &buffer_size);
    else
        GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &n_languages, NULL, &buffer_size);

    buffer = (WCHAR*) MALLOCA(buffer_size * sizeof(WCHAR));

    if(system)
        GetSystemPreferredUILanguages(MUI_LANGUAGE_NAME, &n_languages, buffer, &buffer_size);
    else
        GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &n_languages, buffer, &buffer_size);

    ptr = buffer;
    for(i = 0; i < n_languages; i++) {
        XTRACE_ALWAYS("%s preferred language #%u: %ls", (system ? "System" : "User"), i+1, ptr);
        ptr += wcslen(ptr) + 1;
    }

    FREEA(buffer);
}

static void
detect_sys_info(void)
{
    static const struct {
        DWORD major_version;
        DWORD minor_version;
        DWORD min_build_number;
        const char* name;
    } os_ver_map[] = {
        { 10, 0, 22000, "Windows 11" },
        { 10, 0, 0,     "Windows 10" },
        {  6, 3, 0,     "Windows 8.1" },
        {  6, 2, 0,     "Windows 8" },
        {  6, 1, 0,     "Windows 7" },
        {  6, 0, 0,     "Windows Vista" },
        {  5, 2, 0,     "Windows XP 64" },
        {  5, 1, 0,     "Windows XP" },
        {  5, 0, 0,     "Windows 2000" },
        {  4, 0, 0,     "Windows NT" }
    };
    LONG (WINAPI* fn_RtlGetVersion)(RTL_OSVERSIONINFOW*);
    LONG err;
    RTL_OSVERSIONINFOW os_ver = { 0 };
    const char* os_ver_str = "<unknown os name>";

    static const struct {
        WORD arch_id;
        const char* name;
    } sys_info_map[] = {
        { PROCESSOR_ARCHITECTURE_INTEL, "x86 (32-bit Intel/AMD)" },
        { PROCESSOR_ARCHITECTURE_AMD64, "x86_64 (64-bit Intel/AMD)" },
        { PROCESSOR_ARCHITECTURE_IA64,  "IA64 (Intel Itanium)" },
        { PROCESSOR_ARCHITECTURE_ARM,   "ARM" },
        { PROCESSOR_ARCHITECTURE_ARM64, "ARM64" }
    };
    SYSTEM_INFO sys_info;
    const char* sys_arch_str = "<unknown arch name>";

    BOOL (WINAPI* fn_IsWow64Process)(HANDLE, BOOL*);
    BOOL is_wow64;

    char buffer[64 + SIZEOF_ARRAY(os_ver.szCSDVersion)];
    int i;


    /* (1) Detect _real_ Windows version. */
    fn_RtlGetVersion = (LONG (WINAPI *)(RTL_OSVERSIONINFOW*))
            GetProcAddress(dll_ntdll, "RtlGetVersion");
    if(fn_RtlGetVersion == NULL) {
        XTRACE_ERR("GetProcAddress(ntdll.dll, RtlGetVersion)");
        goto err_GetProcAddress_RtlGetVersion;
    }

    os_ver.dwOSVersionInfoSize = sizeof(os_ver);
    err = fn_RtlGetVersion(&os_ver);
    if(err != 0) {
        XTRACE_ERR_(err, "RtlGetVersion()");
        goto err_RtlGetVersion;
    }

    for(i = 0; i < (int) SIZEOF_ARRAY(os_ver_map); i++) {
        if(os_ver.dwMajorVersion == os_ver_map[i].major_version  &&
           os_ver.dwMinorVersion == os_ver_map[i].minor_version  &&
           os_ver.dwBuildNumber >= os_ver_map[i].min_build_number) {
            os_ver_str = os_ver_map[i].name;
            break;
        }
    }

err_RtlGetVersion:
err_GetProcAddress_RtlGetVersion:
    XTRACE_ALWAYS("Detected OS version: %s%s%S (%u.%u build %u)",
            os_ver_str, os_ver.szCSDVersion[0] ? " " : "", os_ver.szCSDVersion,
            os_ver.dwMajorVersion, os_ver.dwMinorVersion, os_ver.dwBuildNumber);

    /* (2) Detect native architecture. */
    GetNativeSystemInfo(&sys_info);
    sprintf(buffer, "%u <unknown arch id>", (unsigned) sys_info.wProcessorArchitecture);
    sys_arch_str = buffer;
    for(i = 0; i < (int) SIZEOF_ARRAY(sys_info_map); i++) {
        if(sys_info.wProcessorArchitecture == sys_info_map[i].arch_id) {
            sys_arch_str = sys_info_map[i].name;
            break;
        }
    }
    XTRACE_ALWAYS("Detected architecture: %s", sys_arch_str);

    /* (3) Determine whether we're WOW64 process via IsWow64Process2(). */
    fn_IsWow64Process = (BOOL (WINAPI*)(HANDLE, BOOL*))
            GetProcAddress(dll_kernel32, "IsWow64Process");
    if(!fn_IsWow64Process(GetCurrentProcess(), &is_wow64)) {
        XTRACE_ERR("IsWow64Process2()");
        is_wow64 = FALSE;
    }

    if(is_wow64)
        XTRACE_ALWAYS("Detected we're running as a Wow64 process.");

    /* (4) System and user locale information. */
    detect_preferred_languages(true);
    detect_preferred_languages(false);
}

#endif  /* DEBUG */


/**********************************
 *** Initialization/Termination ***
 **********************************/

static const struct {
#ifdef DEBUG
    const char* func_init_str;
#endif
    HRESULT (*func_init)(void);
    void (*func_fini)(void);
} module_map[] = {
#ifdef DEBUG
    #define MODULE(func_init, func_fini)      { STRINGIZE(func_init) "()", (func_init), (func_fini) }
#else
    #define MODULE(func_init, func_fini)      { (func_init), (func_fini) }
#endif

    /* Carefull here: The order matters, there may be module dependencies. */
    MODULE(gfx_init,        gfx_fini),
    MODULE(appwindow_init,  appwindow_fini)

#undef MODULE
};

static void
feTerminate_impl(int count)
{
    /* Clean up the modules in reversed order. */
    while(count > 0) {
        count--;
        module_map[count].func_fini();
    }
}

/* Called from feInitialize() as well as from DllMain(). */
static void
trace_hr_once(void)
{
#ifdef DEBUG
    static INIT_ONCE once = INIT_ONCE_STATIC_INIT;
    BOOL pending;

    if(!InitOnceBeginInitialize(&once, 0, &pending, NULL))
        return;
    XTRACE_ALWAYS("========================================================");
    InitOnceComplete(&once, 0, NULL);
#endif
}

BOOL CALLBACK
DllMain(HINSTANCE hInstance, DWORD reason, void* reserved)
{
    UNUSED(hInstance);

    switch(reason) {
        case DLL_PROCESS_ATTACH:
            trace_hr_once();
            if(reserved != NULL)
                XTRACE_ALWAYS("FeatherUI loading (process startup)");
            else
                XTRACE_ALWAYS("FeatherUI loading (LoadLibrary())");
            break;

        case DLL_PROCESS_DETACH:
            if(reserved != NULL)
                XTRACE_ALWAYS("FeatherUI unloading (process termination)");
            else
                XTRACE_ALWAYS("FeatherUI unloading (FreeLibrary())");
            break;
    }

    return TRUE;
}


/************************
 *** Public Interface ***
 ************************/

/* Forward declaration.
 * This lives in misc_alloc.c but it's just for our use here. */
void fe_set_allocator(const FeAllocator* allocator);

HRESULT FEATHER_API
feInitialize(const FeAllocator* allocator, DWORD dwFlags)
{
    HMODULE dll_self;
    HRESULT hr;
    int i;

    UNUSED(dwFlags);

    trace_hr_once();

    if(allocator != NULL) {
        if(allocator->fnMalloc == NULL  ||
           allocator->fnRealloc == NULL  ||
           allocator->fnFree == NULL)
        {
            XTRACE_E("Invalid custom allocator.");
            return E_INVALIDARG;
        }
    }

    fe_set_allocator(allocator);

    XTRACE_ALWAYS("FeatherUI version %u.%u.%u (%u-bit)",
            PROJECT_MAJOR_VERSION, PROJECT_MINOR_VERSION, PROJECT_PATCH_VERSION,
            CHAR_BIT * sizeof(void*)
    );

#ifdef FEATHER_BUILD_DLL
    XTRACE_ALWAYS("(built as DLL)");
#endif

#ifdef FEATHER_BUILD_STATIC
    XTRACE_ALWAYS("(built as a static lib)");
#endif

    if(allocator != NULL  &&  allocator->cbSize != sizeof(FeAllocator)) {
        XTRACE_E("initialize: invalid FeAllocator::cbSize.");
        return E_INVALIDARG;
    }

    if(!GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                (const WCHAR*) feInitialize, &dll_self))
    {
        XTRACE_ERR("GetModuleHandleEx()");
        return HRESULT_FROM_WIN32(GetLastError());
    }

    dll_feather = dll_self;
    dll_kernel32 = GetModuleHandle(L"kernel32.dll");
    dll_ntdll = GetModuleHandle(L"ntdll.dll");
    ASSERT(dll_kernel32 != NULL);
    ASSERT(dll_ntdll != NULL);

#ifdef DEBUG
    detect_sys_info();
#endif

    for(i = 0; i < (int) SIZEOF_ARRAY(module_map); i++) {
        hr = module_map[i].func_init();
        if(hr != S_OK) {
            XTRACE_HR(module_map[i].func_init_str);
            /* Undo what we've already done. */
            feTerminate_impl(i);
            return hr;
        }
    }

    return S_OK;
}

void FEATHER_API
feTerminate(void)
{
    feTerminate_impl(SIZEOF_ARRAY(module_map));
}
