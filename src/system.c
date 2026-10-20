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

#include "system.h"
#include "colortheme.h"


/************************************
 ***  Log Windows version et al.  ***
 ************************************/

/* We detects _actual_ Windows version and avoids the lying GetVersionEx().
 *
 * Currently we just use it for debug logs, and preferably it should stay that
 * way. It's better to detect any availability of lack of Windows features by
 * other means.
 */

#ifdef DEBUG

static void
report_sys_info(void)
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
        XTRACE_ALWAYS("IsWow64Process2()");
        is_wow64 = FALSE;
    }

    if(is_wow64)
        XTRACE_ALWAYS("Detected we're running as a Wow64 process.");
}

static void
report_preferred_languages(BOOL system)
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
        XTRACE_ALWAYS("%s preferred language #%u: %S", (system ? "System" : "User"), i+1, ptr);
        ptr += wcslen(ptr) + 1;
    }

    FREEA(buffer);
}

#endif  /* DEBUG */


/*********************************
 ***  System Config Umbrella   ***
 *********************************/

#define UPDATE_ImmersiveColorSet    0x0001

static void
update_config(DWORD flags)
{
    if(flags & UPDATE_ImmersiveColorSet)
        colortheme_reload_uisettings();
}


/************************************
 ***  Broadcast message receiver  ***
 ************************************/

/* A helper, always invisible, top-level window for handling system settings
 * changes just once instead of for every top-level HWND we manage. */

static const WCHAR msgwindow_classname[] = L"feather::MsgWindow";
static HWND msgwindow_hwnd = NULL;


static DWORD msgwindow_config_flags = 0;
#define MSGWINDOW_WM_UPDATECONFIG   (WM_USER + 100)


/* Windows tend to send multiple events for one user's interaction in
 * the settings dialogs. So lets coalesce them and then re-read the settings
 * just once, if possible.
 *
 * We do this by posting a message so all messages queued before-hand will be
 * already processed (and possibly still update the flags what to do).
 */
static void
coalesce_flags(DWORD flags)
{
    DWORD old_flags = msgwindow_config_flags;

    msgwindow_config_flags |= flags;
    if(old_flags == 0)
        PostMessage(msgwindow_hwnd, MSGWINDOW_WM_UPDATECONFIG, 0, 0);
}

static void
msgwindow_WM_SETTINGCHANGE(UINT ui_action, const WCHAR* param_name)
{
    static const struct {
        const WCHAR* name;
        DWORD flag;
    } param_map[] = {
        { L"ImmersiveColorSet",     UPDATE_ImmersiveColorSet },
#if 0
        /* FIXME: Do we need any of this? */
        { L"WindowMetrics",         CONFIG_WindowMetrics },
        { L"WindowsThemeElement",   CONFIG_WindowsThemeElement }
        { L"Environment",           CONFIG_Environment },
        { L"Policy",                CONFIG_Policy },
        { L"intl",                  CONFIG_intl },      /* locale/internationalization */
        { L"Control Panel Color",   CONFIG_ControlPanelColor }
#endif
    };
    unsigned i;
    DWORD flags = 0;

    UNUSED(ui_action);

    /* Note: param_name may be NULL; we then have to re-read _everything_
     * relevant to us. */

    XTRACE("ui_action: %u; param_name: %S",
            ui_action, param_name ? param_name : L"NULL");

    for(i = 0; i < SIZEOF_ARRAY(param_map); i++) {
        if(param_name == NULL  ||  wcscmp(param_name, param_map[i].name) == 0)
            flags |= param_map[i].flag;
    }

    if(flags != 0)
        coalesce_flags(flags);
}

static LRESULT CALLBACK
msgwindow_WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch(msg) {
        case MSGWINDOW_WM_UPDATECONFIG:
            if(msgwindow_config_flags != 0) {
                update_config(msgwindow_config_flags);
                msgwindow_config_flags = 0;
            }
            return 0;

        case WM_SETTINGCHANGE:
            msgwindow_WM_SETTINGCHANGE((UINT) wp, (const WCHAR*) lp);
            return 0;
    }

    return DefWindowProc(hwnd, msg, wp, lp);
}


/*******************************
 ***  Module initialization  ***
 *******************************/

HRESULT
system_init(void)
{
    WNDCLASSEX wc = { 0 };
    HRESULT hr = E_FAIL;

#ifdef DEBUG
    report_sys_info();
    report_preferred_languages(TRUE);
    report_preferred_languages(FALSE);
#endif

    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = msgwindow_WndProc;
    wc.hInstance = dll_feather;
    wc.lpszClassName = msgwindow_classname;

    if(!RegisterClassEx(&wc)) {
        XTRACE_ERR("RegisterClassEx()");
        return HRESULT_FROM_WIN32(GetLastError());
    }

    msgwindow_hwnd = CreateWindowEx(WS_EX_TOOLWINDOW, msgwindow_classname,
                L"", 0, 0, 0, 0, 0, NULL, NULL, dll_feather, NULL);
    if(msgwindow_hwnd == NULL) {
        XTRACE_ERR("CreateWindowEx()");
        hr = HRESULT_FROM_WIN32(GetLastError());
        UnregisterClass(msgwindow_classname, dll_feather);
        return hr;
    }

    return S_OK;
}

void
system_fini(void)
{
    DestroyWindow(msgwindow_hwnd);
    UnregisterClass(msgwindow_classname, dll_feather);
}
