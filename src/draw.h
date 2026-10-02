
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
