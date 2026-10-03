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

#include <feather/appwindow.h>
#include "mainloop.h"
#include "misc.h"
#include "draw.h"


static const WCHAR appwindow_classname[] = L"feather::AppWindow";
static unsigned appwindow_count = 0;


typedef struct AppWindow AppWindow;
struct AppWindow {
    HWND hwnd;
    DrawCanvas* canvas;
};


unsigned
appwindow_get_count(void)
{
    return appwindow_count;
}


static void
appwindow_WM_PAINT(AppWindow* app_win)
{
    static const DrawColor light_blue = { 0.9, 0.9, 1.0, 1.0 };
    PAINTSTRUCT ps;

    if(!draw_begin(app_win->canvas, &ps))
        return;

    draw_clear(app_win->canvas, &light_blue);

    draw_end(app_win->canvas, &ps);
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

    UNUSED(cs);

    app_win = (AppWindow*) fe_malloc(sizeof(AppWindow));
    if(app_win == NULL) {
        SetLastError(ERROR_OUTOFMEMORY);
        TRACE_ERR("malloc()");
        return FALSE;
    }
    memset(app_win, 0, sizeof(AppWindow));
    SetWindowLongPtr(hwnd, 0, (LONG_PTR) app_win);

    app_win->hwnd = hwnd;
    app_win->canvas = draw_create_hwnd_canvas(hwnd);

    appwindow_count++;
    return TRUE;
}

static void
appwindow_WM_NCDESTROY(AppWindow* app_win)
{
    /* Windows send WM_NCDESTROY even when WM_NCCREATE fails. */
    if(app_win == NULL)
        return;

    if(app_win->canvas != NULL)
        draw_destroy_canvas(app_win->canvas);

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

        case WM_SIZE:
            draw_resize_hwnd_canvas(app_win->canvas, wp, lp);
            UpdateWindow(hwnd);
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
        TRACE_ERR("RegisterClassEx()");
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
                    CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL,
                    dll_feather, 0);
    if(hwnd == NULL) {
        TRACE_ERR("CreateWindowEx()");
        return HRESULT_FROM_WIN32(GetLastError());
    }

    ShowWindow(hwnd, SW_RESTORE);

    if(pHwnd != NULL)
        *pHwnd = hwnd;
    return S_OK;
}
