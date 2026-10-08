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

#include <feather.h>

#include "misc.h"
#include "project.h"
/* The following ones are included just for their init/fini functions. */
#include "appwindow.h"
#include "gfx.h"


HMODULE dll_feather = NULL;
HMODULE dll_kernel32 = NULL;
HMODULE dll_ntdll = NULL;

void* (*fn_malloc_)(size_t) = NULL;
void* (*fn_realloc_)(void*, size_t) = NULL;
void (*fn_free_)(void*) = NULL;


/*****************************************
 *** Debug Output via OutDebugString() ***
 *****************************************/

#ifdef DEBUG

static const char*
debug_basename__(const char* fname)
{
    const char* tmp;

    tmp = strrchr(fname, '/');
    if(tmp != NULL)
        fname = tmp+1;

    tmp = strrchr(fname, '\\');
    if(tmp != NULL)
        fname = tmp+1;

    return fname;
}

void
debug_trace__(const char* fname, const char* funcname, unsigned line, const char* fmt, ...)
{
    DWORD last_error;
    va_list args;
    char buffer[512];
    char* ptr = buffer;

    last_error = GetLastError();

    fname = debug_basename__(fname);
    ptr += sprintf(ptr, "[%08x] %s:%u (%s): ",
            (unsigned) GetCurrentThreadId(), fname, line, funcname);
    va_start(args, fmt);
    ptr += vsprintf(ptr, fmt, args);
    va_end(args);
    *ptr++ = '\n';
    *ptr++ = '\0';
    OutputDebugStringA(buffer);

    SetLastError(last_error);
}

void
debug_dump__(const char* fname, const char* funcname, unsigned line, const char* msg, const void* addr, size_t size)
{
    static const size_t n_bytes_per_line = 16;
    static const size_t n_bytes_per_extra_space = 8;

    DWORD last_error;
    const BYTE* bytes = (const BYTE*) addr;
    size_t off = 0;

    last_error = GetLastError();

    debug_trace__(fname, funcname, line, "%s", msg);

    while(off < size) {
        char buffer[512];
        char* ptr = buffer;
        size_t line_off = off;

        ptr += sprintf(ptr, "    %04lx:  ", (ULONG) off);

        /* Hex output */
        while(off - line_off < n_bytes_per_line) {
            if(off < size)
                ptr += sprintf(ptr, " %02x", bytes[off]);
            else
                ptr += sprintf(ptr, "   ");
            if(off % n_bytes_per_extra_space == 0)
                ptr += sprintf(ptr, "  ");
            off++;
        }

        off = line_off;
        ptr += sprintf(ptr, "   | ");

        /* Char output */
        while(off - line_off < n_bytes_per_line) {
            if(off < size)
                ptr += sprintf(ptr, "%c",
                        (iscntrl(bytes[off]) || (uint8_t)bytes[off] >= 0x80 ? '.' : bytes[off]));
            else
                break;
            off++;
        }

        debug_trace__(fname, funcname, line, "%s", buffer);
    }

    debug_trace__(fname, funcname, line, "            (%lu bytes)", (ULONG)size);
    SetLastError(last_error);
}

#endif  /* DEBUG */


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
        TRACE("%s preferred language #%u: %S", (system ? "System" : "User"), i+1, ptr);
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
        TRACE_ERR("GetProcAddress(ntdll.dll, RtlGetVersion)");
        goto err_GetProcAddress_RtlGetVersion;
    }

    os_ver.dwOSVersionInfoSize = sizeof(os_ver);
    err = fn_RtlGetVersion(&os_ver);
    if(err != 0) {
        TRACE_ERR_("RtlGetVersion()", err);
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
    TRACE("Detected OS version: %s%s%S (%u.%u build %u)",
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
    TRACE("Detected architecture: %s", sys_arch_str);

    /* (3) Determine whether we're WOW64 process via IsWow64Process2(). */
    fn_IsWow64Process = (BOOL (WINAPI*)(HANDLE, BOOL*))
            GetProcAddress(dll_kernel32, "IsWow64Process");
    if(!fn_IsWow64Process(GetCurrentProcess(), &is_wow64)) {
        TRACE_ERR("IsWow64Process2()");
        is_wow64 = FALSE;
    }

    if(is_wow64)
        TRACE("Detected we're running as a Wow64 process.");

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
    static BOOL was_here = FALSE;

    if(was_here)
        return;

    TRACE("========================================================");
    was_here = TRUE;
}

BOOL CALLBACK
DllMain(HINSTANCE hInstance, DWORD reason, void* reserved)
{
    UNUSED(hInstance);

    switch(reason) {
        case DLL_PROCESS_ATTACH:
            trace_hr_once();
            if(reserved != NULL)
                TRACE("FeatherUI loading (process startup)");
            else
                TRACE("FeatherUI loading (LoadLibrary())");
            break;

        case DLL_PROCESS_DETACH:
            if(reserved != NULL)
                TRACE("FeatherUI unloading (process termination)");
            else
                TRACE("FeatherUI unloading (FreeLibrary())");
            break;
    }

    return TRUE;
}


/************************
 *** Public Interface ***
 ************************/

HRESULT FEATHER_API
feInitialize(const FeAllocator* allocator, DWORD dwFlags)
{
    HMODULE dll_self;
    HRESULT hr;
    int i;

    UNUSED(dwFlags);

    trace_hr_once();

    TRACE("FeatherUI version %u.%u.%u (%u-bit)",
          PROJECT_MAJOR_VERSION, PROJECT_MINOR_VERSION, PROJECT_PATCH_VERSION,
          CHAR_BIT * sizeof(void*)
    );

#ifdef FEATHER_BUILD_DLL
    TRACE("(built as DLL)");
#endif

#ifdef FEATHER_BUILD_STATIC
    TRACE("(built as a static lib)");
#endif

    if(allocator != NULL  &&  allocator->cbSize != sizeof(FeAllocator)) {
        TRACE("initialize: invalid FeAllocator::cbSize.");
        return E_INVALIDARG;
    }

    if(allocator != NULL) {
        fn_malloc_ = allocator->fnMalloc;
        fn_realloc_ = allocator->fnRealloc;
        fn_free_ = allocator->fnFree;
    } else {
        fn_malloc_ = NULL;
        fn_realloc_ = NULL;
        fn_free_ = NULL;
    }

    if(!GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                (const WCHAR*) feInitialize, &dll_self))
    {
        TRACE_ERR("GetModuleHandleEx()");
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
            TRACE_HR(module_map[i].func_init_str);
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
