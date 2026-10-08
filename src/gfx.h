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

#ifndef FE_GFX_H
#define FE_GFX_H

#include "misc.h"


/* DPI and Pixel Snapping
 * ======================
 *
 * Under the hood, we create all "canvases" and "bitmaps" by explicitly saying
 * the Direct2D subsystem that we use 96.0 DPI and all our drawing routines
 * paint with a stroke 1 pixel wide.
 *
 * This has two-fold consequence:
 *
 * (1) Caller has to do all scaling of coordinates and dimensions of painted
 *     elements for himself.
 *
 * (2) This allows allows the caller to round the scaled value so that
 *     horizontal and vertical lines can be snapped to the nearest pixel and
 *     avoid affecting neighborhood pixels via anti-aliasing.
 *
 * To do the snapping successfully, keep this in mind:
 *
 * -- For painting horizontal and vertical lines snapped into the pixel grid,
 *    the line axis needs to go through the middle of a pixel. I.e. for
 *    horizontal lines the Y-coordinates must have 0.5f fraction; for
 *    vertical lines the X-coordinates must have 0.5f fraction.
 *
 *    For example:
 *    ```
 *    gfx_draw_line(canvas, x0, 0.5f, x1, 0.5f, color);
 *    ```
 *
 * -- For filling operations, the coordinates define exactly the inner pixels
 *    of the shape to be filled, so to avoid any anti-aliasing artifacts along
 *    the horizontal and vertical edges, there must be no fraction.
 *
 *    For example:
 *    ```
 *    gfx_fill_rect(canvas, 10.0f, 10.0f, 50.0f, 20.0f, color);
 *    ```
 *
 * Put in other words: The origin of the coordinate system is not the pixel
 * at [0, 0] but left-top corner of that pixels and all the lines have 1 pixel
 * width when painting.
 *
 *
 * Error Handling
 * ==============
 *
 * As much as possible all the resources generally live on the graphics card.
 * And that can be lost when graphics card gets reset (e.g. when user changes
 * a resolution or there's a driver issue) and then everything gets lost.
 *
 * Our "canvas" will report it through the next call to gfx_end_draw() which
 * will fail. The first following gfx_begin_draw() will try to recreate all
 * resources like the buffers under the hood but the painted state (the
 * contents of those buffer) will be lost.
 *
 * For HWND canvas this is easy: When it fails as for it, gfx_end_draw()
 * automatically invalidates complete window when this happens, so that
 * the next call to WM_PAINT should fix it and the caller does not need
 * anything special.
 *
 * For memory canvas the caller needs essentially to retry and paint
 * everything anew, starting with gfx_begin_draw().
 *
 * TODO: Our "bitmap" abstraction is not yet implemented but it will have
 * to deal with it too.
 */


/* Be warned this is incompatible with COLORREF!
 *
 * COLORREF is encoded as 0x00bbggrr where rr, gg, bb represent the color
 * channels. However the most significant byte 0x00 is then udnerstood as
 * "fully transparent".
 */
typedef union GfxColor GfxColor;
union GfxColor {
    /* Note we assume Windows are little-endian here! */
    DWORD argb;
    struct {
        BYTE b;
        BYTE g;
        BYTE r;
        BYTE a; /* alpha channel: 0x00 = transparent; 0xff = opaque */
    };
};

#define GFX_OPAQUE          0xff
#define GFX_TRANSPARENT     0x00

static inline GfxColor
gfx_rgb(BYTE r, BYTE g, BYTE b)
{
    GfxColor color;
    color.r = r; color.g = g; color.b = b; color.a = GFX_OPAQUE;
    return color;
}

static inline GfxColor
gfx_rgba(BYTE r, BYTE g, BYTE b, BYTE a)
{
    GfxColor color;
    color.r = r; color.g = g; color.b = b; color.a = a;
    return color;
}


/* An opaque pointers representing graphics tools or resources. */
typedef struct GfxCanvas GfxCanvas;
typedef struct GfxBrush GfxBrush;


GfxCanvas* gfx_create_mem_canvas(UINT width, UINT height);
GfxCanvas* gfx_create_hwnd_canvas(HWND hwnd);
void gfx_destroy_canvas(GfxCanvas* canvas);

/* Resize canvas
 * (currently? only supported for HWND canvas)
 */
HRESULT gfx_resize_canvas(GfxCanvas* canvas, UINT width, UINT height);

/* These replace the WinAPI BeginPaint() and EndPaint(),
 * ps can be NULL. */
HRESULT gfx_begin_draw(GfxCanvas* canvas, PAINTSTRUCT* ps);
HRESULT gfx_end_draw(GfxCanvas* canvas, PAINTSTRUCT* ps);

/* Drawing operations */
void gfx_clear(GfxCanvas* canvas, GfxColor color);
void gfx_draw_point(GfxCanvas* canvas, float x, float y, GfxColor color);
void gfx_draw_line(GfxCanvas* canvas, float x0, float y0, float x1, float y1, GfxColor color);
void gfx_draw_rect(GfxCanvas* canvas, float x0, float y0, float x1, float y1, GfxColor color);
void gfx_draw_rounded_rect(GfxCanvas* canvas, float x0, float y0, float x1, float y1, float rx, float ry, GfxColor color);

/* Filling operations */
void gfx_fill_rect(GfxCanvas* canvas, float x0, float y0, float x1, float y1, GfxColor color);
void gfx_fill_rounded_rect(GfxCanvas* canvas, float x0, float y0, float x1, float y1, float rx, float ry, GfxColor color);

/* This may be called when we suspect that this process won't need to do
 * any Direct2D/3D in near future. It has no impact on the state of the
 * application, it's just courtesy to system/other processes as we release
 * currently held buffers allocated on the graphical card's memory. */
void gfx_trim_mem(void);

/* Direct2D module initialization. */
HRESULT gfx_init(void);
void gfx_fini(void);


#endif  /* FE_GFX_H */
