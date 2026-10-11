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

#include <roapi.h>

#define WIDL_using_Windows_Foundation
#include <windows.foundation.h>

#define WIDL_using_Windows_UI_ViewManagement
#include <windows.ui.viewmanagement.h>

#include "system.h"
#include "hstring_ref.h"


/* mingw-w64 libs miss these GUIDs (even though the headers create references to globals IID_xxxx).
 * So lets use our own IID definitions: */
static const GUID x_IID_IUISettings  = {0x85361600,0x1c63,0x4627,{0xbc,0xb1,0x3a,0x89,0xe0,0xbc,0x9c,0x55}};
static const GUID x_IID_IUISettings2 = {0xbad82401,0x2721,0x44f9,{0xbb,0x91,0x2b,0xb2,0x28,0xbe,0x44,0x2f}};
static const GUID x_IID_IUISettings3 = {0x03021be4,0x5254,0x4781,{0x81,0x94,0x51,0x68,0xf7,0xd0,0x6d,0x7b}};
static const GUID x_IID_IUISettings4 = {0x52bb3002,0x919b,0x4d6b,{0x9b,0x78,0x8d,0xd6,0x6f,0xf4,0xb9,0x3b}};

static SystemConfig system_config = { 0 };


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
report_preferred_languages(bool system)
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


/*******************************
 ***  Light/Dark Theme Flag  ***
 *******************************/

static void
retrieve_light_theme_flag(void)
{
    static const WCHAR light_theme_reg_path[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize";
    static const WCHAR light_theme_value_name[] = L"AppsUseLightTheme";
    HKEY reg_key;
    DWORD value_type;
    DWORD value_contents;
    DWORD value_size;
    LRESULT err;

    system_config.use_light_theme = FALSE;

    err = RegOpenKeyEx(HKEY_CURRENT_USER, light_theme_reg_path, 0, KEY_QUERY_VALUE, &reg_key);
    if(err != ERROR_SUCCESS) {
        XTRACE_ERR_(err, "RegOpenKeyEx()");
        goto err_RegOpenKeyEx;
    }

    value_size = sizeof(DWORD);
    err = RegQueryValueEx(reg_key, light_theme_value_name, NULL, &value_type,
                        (BYTE*) &value_contents, &value_size);
    if(err != ERROR_SUCCESS) {
        XTRACE_ERR_(err, "RegQueryValueEx()");
        goto err_RegQueryValueEx;
    }

    if(value_type == REG_DWORD  &&  value_size == sizeof(DWORD)  &&  value_contents != 0)
        system_config.use_light_theme = TRUE;

err_RegQueryValueEx:
    RegCloseKey(reg_key);
err_RegOpenKeyEx:
    XTRACE("Use light theme: %s", system_config.use_light_theme ? "yes": "no");
}


/*********************************
 ***  Light/Dark Theme Colors  ***
 *********************************/

static void
retrieve_ui_settings(void)
{
    static const WCHAR rt_class_id[] = L"Windows.UI.ViewManagement.UISettings";
    HMODULE dll_combase;
    HRESULT (WINAPI* fn_RoInitialize)(RO_INIT_TYPE);
    HRESULT (WINAPI* fn_RoActivateInstance)(HSTRING, IInspectable**);
    void (WINAPI* fn_RoUninitialize)(void);
    HstringRef hstr_ref;
    IInspectable* inspectable;
    IUISettings* ui_settings1;
    IUISettings2* ui_settings2;
    IUISettings3* ui_settings3;
    IUISettings4* ui_settings4;
    HandPreference hand_pref;
    boolean enabled;
    double scale;
    Size size;
    HRESULT hr;

    /* Set sane defaults. */
    system_config.optimize_ui_for_left_handed_user = FALSE;
    system_config.use_ui_transparency_effects = FALSE;
    system_config.text_scale_factor = 1.0;

    /* Try to get actual settings from the system and overwrite what we
     * successfully retrieve. */
    dll_combase = LoadLibraryEx(L"combase.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if(dll_combase == NULL) {
        XTRACE_ERR("LoadLibraryEx()");
        goto err_LoadLibraryEx;
    }

    fn_RoInitialize = (HRESULT (WINAPI*)(RO_INIT_TYPE))
            GetProcAddress(dll_combase, "RoInitialize");
    fn_RoActivateInstance = (HRESULT (WINAPI*)(HSTRING, IInspectable**))
            GetProcAddress(dll_combase, "RoActivateInstance");
    fn_RoUninitialize = (void (WINAPI*)(void))
            GetProcAddress(dll_combase, "RoUninitialize");
    if(fn_RoInitialize == NULL || fn_RoActivateInstance == NULL || fn_RoUninitialize == NULL) {
        XTRACE_ERR("GetProcAddress()");
        goto err_GetProcAddress;
    }

    hr = fn_RoInitialize(RO_INIT_MULTITHREADED);
    if(FAILED(hr)) {
        XTRACE_HR("RoInitialize()");
        goto err_RoInitialize;
    }

    hr = fn_RoActivateInstance(hstring_ref(&hstr_ref, rt_class_id), (IInspectable**) &inspectable);
    if(FAILED(hr)) {
        XTRACE_HR("RoActivateInstance()");
        goto err_RoActivateInstance;
    }

    hr = IInspectable_QueryInterface(inspectable, &x_IID_IUISettings, (void**) &ui_settings1);
    if(FAILED(hr)) {
        XTRACE_HR("IInspectable::QueryInterface(IUISettings)");
        goto err_QueryInterface1;
    }

    hr = IUISettings_get_HandPreference(ui_settings1, &hand_pref);
    if(SUCCEEDED(hr)) {
        XTRACE("IUISettings::get_HandPreference() -> %s",
            (hand_pref == HandPreference_RightHanded) ? "right-handed user" : "left-handed user");
        system_config.optimize_ui_for_left_handed_user = (hand_pref == HandPreference_LeftHanded);
    } else {
        XTRACE_HR("IUISettings::get_HandPreference()");
    }

    hr = IUISettings_get_CursorSize(ui_settings1, &size);
    if(SUCCEEDED(hr)) {
        XTRACE("IUISettings::get_CursorSize()");
        //system_config.optimize_ui_for_left_handed_user = (hand_pref == HandPreference_LeftHanded);
    } else {
        XTRACE_HR("IUISettings::get_CursorSize()");
    }

    // TODO: IUISettings_get_CursorSize()  // mouse
    // TODO: IUISettings_get_ScrollBarSize()
    // TODO: IUISettings_get_ScrollBarArrowSize()
    // TODO: IUISettings_get_ScrollBarThumbBoxSize()
    // TODO: IUISettings_get_MessageDuration()
    // TODO: IUISettings_get_AnimationsEnabled()
    // TODO: IUISettings_get_CaretBrowsingEnabled()
    // TODO: IUISettings_get_CaretBlinkRate()
    // TODO: IUISettings_get_CaretWidth()
    // TODO: IUISettings_get_DoubleClickTime
    // TODO: IUISettings_get_MouseHoverTime
    // TODO: IUISettings_UIElementColor     // use, but let IUISettings3_GetColorValue() override it, if it succeeds.

    hr = IInspectable_QueryInterface(inspectable, &x_IID_IUISettings2, (void**) &ui_settings2);
    if(FAILED(hr)) {
        XTRACE_HR("IInspectable::QueryInterface(IUISettings2)");
        goto err_QueryInterface2;
    }

    hr = IUISettings2_get_TextScaleFactor(ui_settings2, &scale);
    if(SUCCEEDED(hr)) {
        XTRACE("IUISettings2::get_TextScaleFactor() -> %f", scale);
        system_config.text_scale_factor = scale;
    } else {
        XTRACE_HR("IUISettings2::get_TextScaleFactor()");
    }

    hr = IInspectable_QueryInterface(inspectable, &x_IID_IUISettings3, (void**) &ui_settings3);
    if(FAILED(hr)) {
        XTRACE_HR("IInspectable::QueryInterface(IUISettings3)");
        goto err_QueryInterface3;
    }

    // TODO: IUISettings3_GetColorValue()

    hr = IInspectable_QueryInterface(inspectable, &x_IID_IUISettings4, (void**) &ui_settings4);
    if(FAILED(hr)) {
        XTRACE_HR("IInspectable::QueryInterface(IUISettings4)");
        goto err_QueryInterface4;
    }

    hr = IUISettings4_get_AdvancedEffectsEnabled(ui_settings4, &enabled);
    if(SUCCEEDED(hr)) {
        XTRACE("IUISettings4::get_AdvancedEffectsEnabled() -> %s", enabled ? "yes" : "no");
        system_config.use_ui_transparency_effects = enabled;
    } else {
        XTRACE_HR("IUISettings4::get_AdvancedEffectsEnabled()");
    }

    IUISettings4_Release(ui_settings4);
err_QueryInterface4:
    IUISettings3_Release(ui_settings3);
err_QueryInterface3:
    IUISettings2_Release(ui_settings2);
err_QueryInterface2:
    IUISettings_Release(ui_settings1);
err_QueryInterface1:
    IInspectable_Release(inspectable);
err_RoActivateInstance:
    fn_RoUninitialize();
err_RoInitialize:
err_GetProcAddress:
    FreeLibrary(dll_combase);
err_LoadLibraryEx:
    ;
}


/*********************************
 ***  System Config Umbrella   ***
 *********************************/

const SystemConfig*
system_get_config(void)
{
    return &system_config;
}


#define CONFIG_SYSCOLORCHANGE       0x0001
#define CONFIG_THEMECHANGED         0x0002
#define CONFIG_ImmersiveColorSet    0x0010
#define CONFIG_WindowMetrics        0x0020
#define CONFIG_WindowsThemeElement  0x0040

#define CONFIG_ALL                  0xffff

static void
config_retrieve_sys_colors(void)
{
    // FIXME: Do we actually need this?
    XTRACE_W("not yet implemented.");
}

static void
config_retrieve_theme(void)
{
    // FIXME: Do we actually need this?
    XTRACE_W("not yet implemented.");
}

static void
config_retrieve_immersive_color_set(void)
{
    retrieve_light_theme_flag();
    retrieve_ui_settings();
}

static void
config_retrieve_window_metrics(void)
{
    NONCLIENTMETRICS nc_metrics;

    nc_metrics.cbSize = sizeof(NONCLIENTMETRICS);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS,
            nc_metrics.cbSize, &nc_metrics, 0);

    XTRACE("lfCaptionFont: %ls; height %ld",
            nc_metrics.lfCaptionFont.lfFaceName, nc_metrics.lfCaptionFont.lfHeight);
    XTRACE("lfSmCaptionFont: %ls; height %ld",
            nc_metrics.lfSmCaptionFont.lfFaceName, nc_metrics.lfSmCaptionFont.lfHeight);
    XTRACE("lfMenuFont: %ls; height %ld",
            nc_metrics.lfMenuFont.lfFaceName, nc_metrics.lfMenuFont.lfHeight);
    XTRACE("lfStatusFont: %ls; height %ld",
            nc_metrics.lfStatusFont.lfFaceName, nc_metrics.lfStatusFont.lfHeight);
    XTRACE("lfMessageFont: %ls; height %ld",
            nc_metrics.lfMessageFont.lfFaceName, nc_metrics.lfMessageFont.lfHeight);

    memcpy(&system_config.ui_font, &nc_metrics.lfMessageFont, sizeof(LOGFONT));
}

static void
config_retrieve_windows_theme_element(void)
{
    // FIXME: Do we actually need this?
    XTRACE_W("not yet implemented.");
}

static void
config_retrieve(DWORD flags, bool notify)
{
    if(flags & CONFIG_SYSCOLORCHANGE)
        config_retrieve_sys_colors();

    if(flags & CONFIG_THEMECHANGED)
        config_retrieve_theme();

    if(flags & CONFIG_ImmersiveColorSet)
        config_retrieve_immersive_color_set();

    if(flags & CONFIG_WindowMetrics)
        config_retrieve_window_metrics();

    if(flags & CONFIG_WindowsThemeElement)
        config_retrieve_windows_theme_element();

    if(notify) {
        // TODO: inform uitheme.c about the change(s)
    }
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
 * just once, if possible. */
static void
msgwindow_update_config(DWORD flags)
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
        { L"ImmersiveColorSet",     CONFIG_ImmersiveColorSet },
        { L"WindowMetrics",         CONFIG_WindowMetrics },
        { L"WindowsThemeElement",   CONFIG_WindowsThemeElement }
#if 0
        /* We might need (some of) these in the future: */
        { L"Environment",           CONFIG_Environment },
        { L"Policy",                CONFIG_Policy },
        { L"intl",                  CONFIG_intl },      /* locale/internationalization */
        { L"Control Panel Color",   CONFIG_ControlPanelColor }
#endif
    };
    unsigned i;
    DWORD flags = 0;

    UNUSED(ui_action);

    /* Note: param_name may be NULL; we then have to re-read everything
     * relevant to us. */

    XTRACE("ui_action: %u; param_name: %S",
            ui_action, param_name ? param_name : L"NULL");

    for(i = 0; i < SIZEOF_ARRAY(param_map); i++) {
        if(param_name == NULL  ||  wcscmp(param_name, param_map[i].name) == 0)
            flags |= param_map[i].flag;
    }

    if(flags != 0)
        msgwindow_update_config(flags);
}

static void
msgwindow_WM_SYSCOLORCHANGE(void)
{
    XTRACE("got WM_SYSCOLORCHANGE");
    msgwindow_update_config(CONFIG_SYSCOLORCHANGE);
}

static void
msgwindow_WM_THEMECHANGED(void)
{
    XTRACE("got WM_THEMECHANGED");
    msgwindow_update_config(CONFIG_THEMECHANGED);
}

static LRESULT CALLBACK
msgwindow_WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch(msg) {
        case MSGWINDOW_WM_UPDATECONFIG:
            if(msgwindow_config_flags != 0) {
                config_retrieve(msgwindow_config_flags, true);
                msgwindow_config_flags = 0;
            }
            return 0;

        case WM_SETTINGCHANGE:
            msgwindow_WM_SETTINGCHANGE((UINT) wp, (const WCHAR*) lp);
            return 0;

        case WM_SYSCOLORCHANGE:
            msgwindow_WM_SYSCOLORCHANGE();
            return 0;

        case WM_THEMECHANGED:
            msgwindow_WM_THEMECHANGED();
            return 0;
    }

    return DefWindowProc(hwnd, msg, wp, lp);
}

static HRESULT
msgwindow_init(void)
{
    WNDCLASSEX wc = { 0 };
    HRESULT hr;

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

static void
msgwindow_fini(void)
{
    DestroyWindow(msgwindow_hwnd);
    UnregisterClass(msgwindow_classname, dll_feather);
}


/*******************************
 ***  Module initialization  ***
 *******************************/

HRESULT
system_init(void)
{
    HRESULT hr = E_FAIL;
#ifdef DEBUG
    report_sys_info();
    report_preferred_languages(true);
    report_preferred_languages(false);
#endif

    hr = msgwindow_init();
    if(hr != S_OK) {
        XTRACE_HR("msgwindow_init()");
        return hr;
    }

    config_retrieve(CONFIG_ALL, false);

    return S_OK;
}

void
system_fini(void)
{
    msgwindow_fini();
}
