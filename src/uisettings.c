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

#define WIDL_using_Windows_Foundation
#define WIDL_using_Windows_UI_ViewManagement
#include <roapi.h>
#include <windows.foundation.h>
#include <windows.ui.viewmanagement.h>

#include "uisettings.h"
#include "hstring_ref.h"


/* mingw-w64 libs miss these GUIDs (even though the headers create references to globals IID_xxxx).
 * So lets use our own IID definitions: */
static const GUID x_IID_IUISettings  = {0x85361600,0x1c63,0x4627,{0xbc,0xb1,0x3a,0x89,0xe0,0xbc,0x9c,0x55}};
static const GUID x_IID_IUISettings2 = {0xbad82401,0x2721,0x44f9,{0xbb,0x91,0x2b,0xb2,0x28,0xbe,0x44,0x2f}};
static const GUID x_IID_IUISettings3 = {0x03021be4,0x5254,0x4781,{0x81,0x94,0x51,0x68,0xf7,0xd0,0x6d,0x7b}};
static const GUID x_IID_IUISettings4 = {0x52bb3002,0x919b,0x4d6b,{0x9b,0x78,0x8d,0xd6,0x6f,0xf4,0xb9,0x3b}};


/* The setting which determines whether to use light or dark theme lives in
 * registry. */
static void
uisettings_load_light_theme_flag(UiSettings* settings)
{
    static const WCHAR light_theme_reg_path[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize";
    static const WCHAR light_theme_value_name[] = L"AppsUseLightTheme";
    HKEY reg_key;
    DWORD value_type;
    DWORD value_contents;
    DWORD value_size;
    LRESULT err;

    settings->use_light_theme = FALSE;

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
        settings->use_light_theme = TRUE;

err_RegQueryValueEx:
    RegCloseKey(reg_key);
err_RegOpenKeyEx:
    XTRACE("Use light theme: %s", settings->use_light_theme ? "yes": "no");
}


static void
uisettings_load_settings(UiSettings* settings)
{
    static const WCHAR rt_class_id[] = L"Windows.UI.ViewManagement.UISettings";
    HMODULE dll_combase;
    HRESULT (WINAPI* fn_RoInitialize)(RO_INIT_TYPE);
    HRESULT (WINAPI* fn_RoActivateInstance)(HSTRING, IInspectable**);
    void (WINAPI* fn_RoUninitialize)(void);
    HstringRef hstr_ref;
    IInspectable* inspectable;
    IUISettings* ui_settings;
    IUISettings2* ui_settings2;
    IUISettings3* ui_settings3;
    IUISettings4* ui_settings4;
    HandPreference hand_pref;
    boolean enabled;
    double scale;
    Size size;
    HRESULT hr;

    /* Set sane defaults. */
    settings->optimize_for_left_handed_user = FALSE;
    settings->use_transparency_effects = FALSE;
    settings->text_scale_factor = 1.0;

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

    hr = IInspectable_QueryInterface(inspectable, &x_IID_IUISettings, (void**) &ui_settings);
    if(FAILED(hr)) {
        XTRACE_HR("IInspectable::QueryInterface(IUISettings)");
        goto err_QueryInterface1;
    }

    hr = IUISettings_get_HandPreference(ui_settings, &hand_pref);
    if(SUCCEEDED(hr))
        settings->optimize_for_left_handed_user = (hand_pref == HandPreference_LeftHanded);
    else
        XTRACE_HR("IUISettings::get_HandPreference()");

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
    if(SUCCEEDED(hr))
        settings->text_scale_factor = scale;
    else
        XTRACE_HR("IUISettings2::get_TextScaleFactor()");

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
    if(SUCCEEDED(hr))
        settings->use_transparency_effects = enabled;
    else
        XTRACE_HR("IUISettings4::get_AdvancedEffectsEnabled()");

    IUISettings4_Release(ui_settings4);
err_QueryInterface4:
    IUISettings3_Release(ui_settings3);
err_QueryInterface3:
    IUISettings2_Release(ui_settings2);
err_QueryInterface2:
    IUISettings_Release(ui_settings);
err_QueryInterface1:
    IInspectable_Release(inspectable);
err_RoActivateInstance:
    fn_RoUninitialize();
err_RoInitialize:
err_GetProcAddress:
    FreeLibrary(dll_combase);
err_LoadLibraryEx:
    XTRACE("optimize_for_left_handed_user: %s",
            (settings->optimize_for_left_handed_user) ? "yes" : "no");
    XTRACE("use_transparency_effects: %s", settings->use_transparency_effects ? "yes" : "no");
    XTRACE("text_scale_factor: %f", settings->text_scale_factor);
}


HRESULT
uisettings_load(UiSettings* settings)
{
    uisettings_load_light_theme_flag(settings);
    uisettings_load_settings(settings);
    return S_OK;
}
