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
#include "colortheme.h"
#include "dpi.h"
#include "fonttheme.h"
#include "gfx.h"
#include "monitor.h"
#include "system.h"
#include "text.h"


HMODULE dll_feather = NULL;
HMODULE dll_kernel32 = NULL;
HMODULE dll_ntdll = NULL;

WCHAR fe_locale_name[LOCALE_NAME_MAX_LENGTH] = L"";


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

    /* Be careful here: The order matters, there are dependencies. */
    MODULE(system_init,     system_fini),       /* keep 1st */
    MODULE(gfx_init,        gfx_fini),
    MODULE(text_init,       text_fini),
    MODULE(colortheme_init, colortheme_fini),
    MODULE(fonttheme_init,  fonttheme_fini),
    MODULE(monitor_init,    monitor_fini),
    MODULE(dpi_init,        dpi_fini),
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
    static INIT_ONCE once = INIT_ONCE_STATIC_INIT;
    BOOL pending;

    if(!InitOnceBeginInitialize(&once, 0, &pending, NULL))
        return;
    XTRACE_ALWAYS("========================================================");
    InitOnceComplete(&once, 0, NULL);
}

BOOL CALLBACK
DllMain(HINSTANCE hInstance, DWORD reason, void* reserved)
{
    switch(reason) {
        case DLL_PROCESS_ATTACH:
            trace_hr_once();
            if(reserved != NULL)
                XTRACE_ALWAYS("FeatherUI loading (process startup)");
            else
                XTRACE_ALWAYS("FeatherUI loading (LoadLibrary())");
            dll_feather = (HMODULE) hInstance;
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
feInitialize(const WCHAR* pszLocaleName, const FeAllocator* allocator, DWORD dwFlags)
{
    HRESULT hr;
    int i;

    UNUSED(dwFlags);

    trace_hr_once();

    if(pszLocaleName != NULL  &&  pszLocaleName[0] != L'\0') {
        if(!IsValidLocaleName(pszLocaleName)  ||  wcslen(pszLocaleName) >= LOCALE_NAME_MAX_LENGTH) {
            XTRACE_E("Invalid locale name: %ls", pszLocaleName);
            return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        }

        wcscpy(fe_locale_name, pszLocaleName);
    } else {
        GetUserDefaultLocaleName(fe_locale_name, LOCALE_NAME_MAX_LENGTH);
    }

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

    if(dll_feather == NULL) {
        /* We should get here only in the static build; in DLL dll_feather
         * is set in DllMain(). */
        if(!GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                    GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                    (const WCHAR*) feInitialize, &dll_feather))
        {
            XTRACE_ERR("GetModuleHandleEx()");
            return HRESULT_FROM_WIN32(GetLastError());
        }
    }

    dll_kernel32 = GetModuleHandle(L"kernel32.dll");
    dll_ntdll = GetModuleHandle(L"ntdll.dll");
    ASSERT(dll_kernel32 != NULL);
    ASSERT(dll_ntdll != NULL);

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
