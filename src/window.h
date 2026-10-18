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
