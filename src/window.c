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

#include "misc.h"
#include "window.h"


void
window_WM_WINDOWPOSCHANGED(Window* win, const WINDOWPOS* pos)
{
    HMONITOR mon_handle;
    UINT width = (UINT) MAX(0, pos->cx);
    UINT height = (UINT) MAX(0, pos->cy);

    /* Check whether we're migrating to a different monitor. */
    mon_handle = MonitorFromWindow(win->hwnd, MONITOR_DEFAULTTONEAREST);
    if(win->mon_info == NULL  ||  mon_handle != win->mon_info->handle) {
        monitor_detach(win->mon_info);
        win->mon_info = monitor_attach(mon_handle);
        gfx_set_text_rendering_params(win->canvas, win->mon_info->text_rendering_params);
    }

    if(!(pos->flags & SWP_NOSIZE)) {
        if(width != win->width  ||  height != win->height) {
            win->width = width;
            win->height = height;
            gfx_resize_canvas(win->canvas, width, height);
        }
    }
}

void
window_WM_DPICHANGED(Window* win, UINT dpi_x, UINT dpi_y, const RECT* rect_hint)
{
    UNUSED(rect_hint);

    if(win->dpi_info == NULL || dpi_x != win->dpi_info->dpi_x || dpi_y != win->dpi_info->dpi_y) {
        if(win->dpi_info != NULL)
            dpi_detach(win->dpi_info);
        win->dpi_info = dpi_attach(dpi_x, dpi_y);
    }
}

LRESULT
window_WM_NCCREATE(Window* win, HWND hwnd, const CREATESTRUCT* cs)
{
    uint16_t dpi_x, dpi_y;

    UNUSED(cs);

    memset(win, 0, sizeof(Window));

    win->hwnd = hwnd;
    win->width = (UINT) MAX(0, cs->cx);
    win->height = (UINT) MAX(0, cs->cy);
    dpi_x = GetDpiForWindow(hwnd);
    dpi_y = dpi_x;

    win->mon_info = monitor_attach(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST));
    win->dpi_info = dpi_attach(dpi_x, dpi_y);

    win->canvas = gfx_create_win_canvas(win);
    if(win->canvas == NULL) {
        XTRACE_E("gfx_create_hwnd_canvas() failed.");
        return FALSE;
    }

    return TRUE;
}

void
window_WM_NCDESTROY(Window* win)
{
    /* Windows send WM_NCDESTROY even when WM_NCCREATE fails. */
    if(win == NULL)
        return;

    if(win->dpi_info != NULL)
        dpi_detach(win->dpi_info);
    if(win->mon_info != NULL)
        monitor_detach(win->mon_info);
    if(win->canvas != NULL)
        gfx_destroy_canvas(win->canvas);
}
