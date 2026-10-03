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

#ifndef FE_DRAW_H
#define FE_DRAW_H

#include "misc.h"


typedef struct DrawCanvas DrawCanvas;


 /* All values between 0.0 (black) and 1.0 (white) */
typedef struct DrawColor DrawColor;
struct DrawColor {
    float r;
    float g;
    float b;
    float a;    /* 0.0 = fully transparent; 1.0 = fully opaque */
};


DrawCanvas* draw_create_hwnd_canvas(HWND hwnd);

void draw_destroy_canvas(DrawCanvas* canvas);

/* For a HWND canvas, call this in response to WM_SIZE */
void draw_resize_hwnd_canvas(DrawCanvas* canvas, WPARAM wp, LPARAM lp);

/* These replace the WinAPI BeginPaint() and EndPaint() */
BOOL draw_begin(DrawCanvas* canvas, PAINTSTRUCT* ps);
void draw_end(DrawCanvas* canvas, PAINTSTRUCT* ps);

/* Drawing operations */
void draw_clear(DrawCanvas* canvas, const DrawColor* c);

/* This may be called when we suspect that this process won't need to do
 * any Direct2D/3D in near future. It has no impact on the state of the
 * application, it's just courtesy to system/other processes as we release
 * currently held buffers allocated on the graphical card's memory. */
void draw_trim_gfx_mem(void);


/* Direct2D module initialization. */
HRESULT draw_init(void);
void draw_fini(void);


#endif  /* FE_DRAW_H */
