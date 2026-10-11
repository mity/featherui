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

#ifndef FE_WINDOW_H
#define FE_WINDOW_H

#include "misc.h"
#include "gfx.h"
#include "monitor.h"
#include "dpi.h"


/* This serves as an abstraction of any top-level window (AppWindow, dialogs,
 * message boxes, popups).
 *
 * It provides them with support for Direct2D rendering, DirectWrite support,
 * DPI handling and other amenities.
 *
 * To implement a new window type, the implementation has to fulfill the
 * following:
 *
 * (1) In their WM_NCCREATE allocate struct Window and keep it around for the
 *     lifetime of the HWND handle.
 * (2) Call the functions below in their respective message handlers.
 * (3) Access members of the struct Window only for reading.
 */
typedef struct Window Window;
struct Window {
    HWND hwnd;
    UINT width;
    UINT height;
    const MonitorInfo* mon_info;
    const DpiInfo* dpi_info;
    GfxCanvas* canvas;
};


/* Other window implementations need to call the following functions from
 * their respective message handlers.
 */
void window_WM_WINDOWPOSCHANGED(Window* win, const WINDOWPOS* pos);
void window_WM_DPICHANGED(Window* win, UINT dpi_x, UINT dpi_y, const RECT* rect_hint);
LRESULT window_WM_NCCREATE(Window* win, HWND hwnd, const CREATESTRUCT* cs);
void window_WM_NCDESTROY(Window* win);


#endif  /* FE_WINDOW_H */
