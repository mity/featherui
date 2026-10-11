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

#include <feather/appwindow.h>

#include "misc.h"
#include "appwindow.h"
#include "window.h"
#include "mainloop.h"
#include "gfx.h"
#include "text.h"


static const WCHAR appwindow_classname[] = L"feather::AppWindow";
static unsigned appwindow_count = 0;


typedef struct AppWindow AppWindow;
struct AppWindow {
    Window win;
};


unsigned
appwindow_get_count(void)
{
    return appwindow_count;
}


static void
appwindow_WM_PAINT(AppWindow* app_win)
{
    PAINTSTRUCT ps;
    HRESULT hr;

    hr = gfx_begin_draw(app_win->win.canvas, &ps);
    if(FAILED(hr))
        XTRACE_HR("gfx_begin()");
    if(hr != S_OK)  /* S_FALSE means no issue but no need to paint either. */
        return;

    gfx_clear(app_win->win.canvas, gfx_rgb(223, 223, 255));
    gfx_draw_line(app_win->win.canvas, 10.0f, 10.5f, 100.0f, 10.5f, gfx_rgb(0, 0, 0));
    gfx_draw_rect(app_win->win.canvas, 10.5f, 20.5f, 99.5f, 29.5f, gfx_rgb(0, 0, 0));
    gfx_fill_rect(app_win->win.canvas, 10.0f, 40.0f, 100.0f, 50.0f, gfx_rgb(0, 0, 0));
    gfx_fill_rounded_rect(app_win->win.canvas, 10.0f, 60.0f, 100.0f, 100.0f, 4.0f, 4.0f, gfx_rgb(0, 0, 0));
    gfx_fill_rounded_rect(app_win->win.canvas, 10.0f, 110.0f, 100.0f, 150.0f, 8.0f, 8.0f, gfx_rgb(0, 0, 0));

    {
        TextFont* font;
        TextLayout* layout;

        font = text_create_font(text_default_text_font_name, 14.0);

        layout = text_create_layout(L"Hello world", wcslen(L"Hello world"),
                        font, 100.0f, 20.0f,
                        TEXT_ALIGN_LEAD | TEXT_VALIGN_TOP | TEXT_TRIM_WORD);
        gfx_draw_text_layout(app_win->win.canvas, 150.0f, 50.0f, layout, gfx_rgb(0, 0, 0));
        text_destroy_layout(layout);
        text_destroy_font(font);
    }

    hr = gfx_end_draw(app_win->win.canvas, &ps);
    if(FAILED(hr))
        XTRACE_HR("gfx_end_draw()");
}

static void
appwindow_WM_CLOSE(AppWindow* app_win)
{
    UNUSED(app_win);

    if(appwindow_count <= 1)
        mainloop_quit(EXIT_SUCCESS);
}

static LRESULT
appwindow_WM_NCCREATE(HWND hwnd, const CREATESTRUCT* cs)
{
    AppWindow* app_win;

    app_win = (AppWindow*) fe_malloc(sizeof(AppWindow));
    if(app_win == NULL) {
        XTRACE_E("malloc()");
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }
    memset(app_win, 0, sizeof(AppWindow));
    SetWindowLongPtr(hwnd, 0, (LONG_PTR) app_win);

    if(window_WM_NCCREATE(&app_win->win, hwnd, cs) != TRUE)
        return FALSE;

    appwindow_count++;
    return TRUE;
}

static void
appwindow_WM_NCDESTROY(AppWindow* app_win)
{
    /* Windows send WM_NCDESTROY even when WM_NCCREATE fails. */
    if(app_win == NULL)
        return;

    window_WM_NCDESTROY(&app_win->win);

    appwindow_count--;
    fe_free(app_win);
}

static LRESULT CALLBACK
appwindow_WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    AppWindow* app_win = (AppWindow*) GetWindowLongPtr(hwnd, 0);

    switch(msg) {
        case WM_PAINT:
            appwindow_WM_PAINT(app_win);
            return 0;

        case WM_ERASEBKGND:
            return 0;

        case WM_WINDOWPOSCHANGED:
            window_WM_WINDOWPOSCHANGED(&app_win->win, (const WINDOWPOS*) lp);
            break;

        case WM_DPICHANGED:
            window_WM_DPICHANGED(&app_win->win, LOWORD(wp), HIWORD(wp), (const RECT*) lp);
            return 0;

        case WM_CLOSE:
            appwindow_WM_CLOSE(app_win);
            return 0;

        case WM_NCCREATE:
            if(!appwindow_WM_NCCREATE(hwnd, (const CREATESTRUCT*) lp))
                return FALSE;
            break;

        case WM_NCDESTROY:
            appwindow_WM_NCDESTROY(app_win);
            break;
    }

    return DefWindowProc(hwnd, msg, wp, lp);
}

HRESULT
appwindow_init(void)
{
    WNDCLASSEX wc = { 0 };

    wc.cbSize = sizeof(wc);
    wc.style = 0;
    wc.lpfnWndProc = appwindow_WndProc;
    wc.cbWndExtra = sizeof(AppWindow*);
    wc.hInstance = dll_feather;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszClassName = appwindow_classname;

    if(!RegisterClassEx(&wc)) {
        XTRACE_ERR("RegisterClassEx()");
        return HRESULT_FROM_WIN32(GetLastError());
    }

    return S_OK;
}

void
appwindow_fini(void)
{
    UnregisterClass(appwindow_classname, dll_feather);
}


/************************
 *** Public Interface ***
 ************************/

HRESULT FEATHER_API
feAppWindow_Create(HWND* pHwnd)
{
    HWND hwnd;

    hwnd = CreateWindowEx(WS_EX_APPWINDOW, appwindow_classname, L"App Name",
                    WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                    500, 300, NULL, NULL,
                    dll_feather, 0);
    if(hwnd == NULL) {
        XTRACE_ERR("CreateWindowEx()");
        return HRESULT_FROM_WIN32(GetLastError());
    }

    ShowWindow(hwnd, SW_RESTORE);

    if(pHwnd != NULL)
        *pHwnd = hwnd;
    return S_OK;
}
