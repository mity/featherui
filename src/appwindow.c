
#include <feather/appwindow.h>
#include "mainloop.h"
#include "misc.h"
#include "paint.h"


static const WCHAR appwindow_classname[] = L"feather::AppWindow";
static unsigned appwindow_count = 0;


typedef struct AppWindow AppWindow;
struct AppWindow {
    HWND hwnd;
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

    BeginPaint(app_win->hwnd, &ps);
    xxx(app_win->hwnd, &ps);
    EndPaint(app_win->hwnd, &ps);
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

    SetWindowLongPtr(hwnd, 0, (LONG_PTR) app_win);
    app_win->hwnd = hwnd;

    appwindow_count++;
    return TRUE;
}

static void
appwindow_WM_NCDESTROY(AppWindow* app_win)
{
    /* Windows send WM_NCDESTROY even when WM_NCCREATE fails. */
    if(app_win == NULL)
        return;

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
