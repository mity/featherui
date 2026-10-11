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

#include "gfx.h"
#include "gfx_internal.h"
#include "window.h"

#include "libex/list.h"


/********************
 *** Misc helpers ***
 ********************/

typedef enum D2dBitmapPurpose D2dBitmapPurpose;
enum D2dBitmapPurpose {
    D2D_BITMAP_BACKBUFFER = 0,  /* a back buffer for window canvas */
    D2D_BITMAP_CANVAS = 1,      /* an in-memory canvas */
    D2D_BITMAP_RESOURCE = 2     /* an immutable image resource */
};

static void
d2d_setup_bitmap_properties(D2D1_BITMAP_PROPERTIES1* properties, D2dBitmapPurpose purpose)
{
    memset(properties, 0, sizeof(D2D1_BITMAP_PROPERTIES1));

    properties->pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    properties->pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    properties->dpiX = 96.0;
    properties->dpiY = 96.0;

    switch(purpose) {
        case D2D_BITMAP_BACKBUFFER:
            properties->bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET |
                                        D2D1_BITMAP_OPTIONS_CANNOT_DRAW;
            break;
        case D2D_BITMAP_CANVAS:
            properties->bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET;
            break;
        case D2D_BITMAP_RESOURCE:
            properties->bitmapOptions = D2D1_BITMAP_OPTIONS_NONE;
            break;
    }
}

#define D2D_COLOR_INIT(gfx_color)                                           \
        { gfx_color.r / 255.0, gfx_color.g / 255.0,                         \
          gfx_color.b / 255.0, gfx_color.a / 255.0 }


/********************
 *** Global Stuff ***
 ********************/

typedef struct GlobalCtx GlobalCtx;
struct GlobalCtx {
    ID3D11Device* d3d_device;
    ID3D11DeviceContext* d3d_device_ctx;
    IDXGIAdapter* dxgi_adapter;
    IDXGIFactory2* dxgi_factory;
    IDXGIDevice* dxgi_device;
    ID2D1Factory1* d2d_factory;
    ID2D1Device* d2d_device;
    ID2D1DeviceContext* d2d_device_ctx;
    ID2D1SolidColorBrush* d2d_solid_color_brush;

    List manager_list;
    BOOL is_connected;
    BOOL no_trim;
};

static GlobalCtx global_ctx = { 0 };


/* Every abstraction which holds some device-dependent resources has to be
 * a "manager". */
typedef enum ManagerType ManagerType;
enum ManagerType {
    TYPE_WIN_CANVAS,
    TYPE_MEM_CANVAS
};

typedef struct Manager Manager;
struct Manager {
    ManagerType type;
    ListNode node;
};

static void
manager_init(Manager* manager, ManagerType type)
{
    manager->type = type;
    list_append(&global_ctx.manager_list, &manager->node);
}

static void
manager_fini(Manager* manager)
{
    list_remove(&global_ctx.manager_list, &manager->node);
}

/* Forward declarations */
typedef struct MemCanvas MemCanvas;
typedef struct WinCanvas WinCanvas;
static void mem_canvas_disconnect(MemCanvas* canvas);
static void win_canvas_disconnect(WinCanvas* canvas);

static void
manager_reset(Manager* manager)
{
    switch(manager->type) {
        /* Note: We just shut disconnect. Individual managers should be
         * to re-connect on their own in a lazy fashion. */
        case TYPE_MEM_CANVAS:   mem_canvas_disconnect((MemCanvas*) manager); break;
        case TYPE_WIN_CANVAS:   win_canvas_disconnect((WinCanvas*) manager); break;
        default:                UNREACHABLE();
    }
}


/* Forward declaration */
static void global_ctx_disconnect(void);

static HRESULT
global_ctx_connect(void)
{
    static const D3D_FEATURE_LEVEL d3d_feature_levels[] =
            { D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1 };
    static const D2D1_FACTORY_OPTIONS d2d_factory_options =
            { D2D1_DEBUG_LEVEL_NONE };
    static const D2D1_COLOR_F d2d_dummy_color = { 0.0, 0.0, 0.0, 0.0 };
    HRESULT hr;

    hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL,
                D3D11_CREATE_DEVICE_BGRA_SUPPORT, d3d_feature_levels,
                SIZEOF_ARRAY(d3d_feature_levels), D3D11_SDK_VERSION,
                &global_ctx.d3d_device, NULL, &global_ctx.d3d_device_ctx);
    if(FAILED(hr)) {
        XTRACE_HR("D3D11CreateDevice()");
        goto err;
    }
    hr = ID3D11Device_QueryInterface(global_ctx.d3d_device, &IID_IDXGIDevice,
                (void**) &global_ctx.dxgi_device);
    if(FAILED(hr)) {
        XTRACE_HR("ID3D11Device::QueryInterface()");
        goto err;
    }
    hr = IDXGIDevice_GetAdapter(global_ctx.dxgi_device, &global_ctx.dxgi_adapter);
    if(FAILED(hr)) {
        XTRACE_HR("IDXGIDevice::GetAdapter()");
        goto err;
    }
    hr = IDXGIAdapter_GetParent(global_ctx.dxgi_adapter, &IID_IDXGIFactory2, (void**) &global_ctx.dxgi_factory);
    if(FAILED(hr)) {
        XTRACE_HR("IDXGIAdapter::GetParent()");
        goto err;
    }
    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &IID_ID2D1Factory1, &d2d_factory_options,
                (void**) &global_ctx.d2d_factory);
    if(FAILED(hr)) {
        XTRACE_HR("D2D1CreateFactory()");
        goto err;
    }
    hr = ID2D1Factory1_CreateDevice(global_ctx.d2d_factory, global_ctx.dxgi_device, &global_ctx.d2d_device);
    if(FAILED(hr)) {
        XTRACE_HR("ID2D1Factory1::CreateDevice()");
        goto err;
    }
    hr = ID2D1Device_CreateDeviceContext(global_ctx.d2d_device, 0, &global_ctx.d2d_device_ctx);
    if(FAILED(hr)) {
        XTRACE_HR("ID2D1Device::CreateDeviceContext()");
        goto err;
    }
    hr = ID2D1DeviceContext_CreateSolidColorBrush(global_ctx.d2d_device_ctx, &d2d_dummy_color, NULL,
                &global_ctx.d2d_solid_color_brush);
    if(FAILED(hr)) {
        XTRACE_HR("ID2D1DeviceContext::CreateSolidColorBrush()");
        goto err;
    }

    /* Success */
    global_ctx.is_connected = TRUE;
    return S_OK;

err:
    global_ctx_disconnect();
    return hr;
}

static void
global_ctx_disconnect(void)
{
    global_ctx.is_connected = FALSE;

    if(global_ctx.d2d_solid_color_brush != NULL) {
        ID2D1SolidColorBrush_Release(global_ctx.d2d_solid_color_brush);
        global_ctx.d2d_solid_color_brush = NULL;
    }
    if(global_ctx.d2d_device_ctx != NULL) {
        ID2D1DeviceContext_Release(global_ctx.d2d_device_ctx);
        global_ctx.d2d_device_ctx = NULL;
    }
    if(global_ctx.d2d_device != NULL) {
        ID2D1Device_Release(global_ctx.d2d_device);
        global_ctx.d2d_device = NULL;
    }
    if(global_ctx.d2d_factory != NULL) {
        ID2D1Factory1_Release(global_ctx.d2d_factory);
        global_ctx.d2d_factory = NULL;
    }
    if(global_ctx.dxgi_device != NULL) {
        IDXGIDevice_Release(global_ctx.dxgi_device);
        global_ctx.dxgi_device = NULL;
    }
    if(global_ctx.dxgi_factory != NULL) {
        IDXGIFactory2_Release(global_ctx.dxgi_factory);
        global_ctx.dxgi_factory = NULL;
    }
    if(global_ctx.dxgi_adapter != NULL) {
        IDXGIAdapter_Release(global_ctx.dxgi_adapter);
        global_ctx.dxgi_adapter = NULL;
    }
    if(global_ctx.d3d_device_ctx != NULL) {
        ID3D11DeviceContext_Release(global_ctx.d3d_device_ctx);
        global_ctx.d3d_device_ctx = NULL;
    }
    if(global_ctx.d3d_device != NULL) {
        ID3D11Device_Release(global_ctx.d3d_device);
        global_ctx.d3d_device = NULL;
    }
}

typedef struct MemCanvas MemCanvas;
static void mem_canvas_disconnect(MemCanvas* manager);
typedef struct WinCanvas WinCanvas;
static void win_canvas_disconnect(WinCanvas* manager);


static void
global_reset(void)
{
    ListNode* node;

    LIST_FOR_EACH(&global_ctx.manager_list, node)
        manager_reset(LIST_DATA(node, Manager, node));

    global_ctx_disconnect();

    /* We might later reconnect to a different graphics card which might
     * support Trim() even if the previous didn't, so lets reset the flag
     * to (re)try. */
    global_ctx.no_trim = FALSE;
}


/*********************
 *** Memory Canvas ***
 *********************/

struct MemCanvas {
    Manager manager;                    /* Keep 1st */
    ID2D1DeviceContext* d2d_device_ctx; /* Keep 2nd */
    ID2D1Bitmap1* d2d_bitmap;

    D2D1_SIZE_U size;
    BOOL is_connected;
};

static HRESULT
mem_canvas_connect(MemCanvas* canvas)
{
    D2D1_BITMAP_PROPERTIES1 d2d_bmp_properties;
    ID2D1Bitmap1* d2d_bitmap;
    HRESULT hr = E_FAIL;

    if(!global_ctx.is_connected) {
        hr = global_ctx_connect();
        if(hr != S_OK) {
            XTRACE_HR("global_ctx_connect");
            goto err;
        }
    }

    if(canvas->d2d_device_ctx == NULL) {
        hr = ID2D1Device_CreateDeviceContext(global_ctx.d2d_device, 0, &canvas->d2d_device_ctx);
        if(FAILED(hr)) {
            XTRACE_HR("ID2D1Device::CreateDeviceContext()");
            goto err;
        }
    }

    if(canvas->d2d_bitmap == NULL) {
        d2d_setup_bitmap_properties(&d2d_bmp_properties, D2D_BITMAP_CANVAS);
        hr = ID2D1DeviceContext_CreateBitmap1(canvas->d2d_device_ctx,
                canvas->size, NULL, 0, &d2d_bmp_properties, &d2d_bitmap);
        if(hr != S_OK) {
            XTRACE_HR("ID2D1DeviceContext::CreateBitmap()");
            goto err;
        }
    }

    return S_OK;

err:
    mem_canvas_disconnect(canvas);
    return hr;
}

static void
mem_canvas_disconnect(MemCanvas* canvas)
{
    canvas->is_connected = FALSE;

    if(canvas->d2d_device_ctx != NULL) {
        ID2D1DeviceContext_Release(canvas->d2d_device_ctx);
        canvas->d2d_device_ctx = NULL;
    }
    if(canvas->d2d_bitmap != NULL) {
        ID2D1Bitmap1_Release(canvas->d2d_bitmap);
        canvas->d2d_bitmap = NULL;
    }
}


/*********************
 *** Window Canvas ***
 *********************/

struct WinCanvas {
    Manager manager;                    /* Keep 1st */
    ID2D1DeviceContext* d2d_device_ctx; /* Keep 2nd */
    IDXGISwapChain1* dxgi_swapchain;
    ID2D1Bitmap1* d2d_bitmap;

    Window* win;
    BOOL is_connected;
};

static HRESULT
win_canvas_connect(WinCanvas* canvas)
{
    HRESULT hr = E_FAIL;

    /* Check the global ctx is ready. It may not be, if there has been a reset
     * of the graphics card. */
    if(!global_ctx.is_connected) {
        hr = global_ctx_connect();
        if(hr != S_OK) {
            XTRACE_HR("global_ctx_connect");
            goto err;
        }
    }

    /* We may be semi-initialized in response to WM_RESIZE, hence check which
     * members are fine. */
    if(canvas->dxgi_swapchain == NULL) {
        DXGI_SWAP_CHAIN_DESC1 desc = { 0 };

        desc.Width = canvas->win->width;
        desc.Height = canvas->win->height;
        desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.BufferCount = 2;
        desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        desc.Scaling = DXGI_SCALING_NONE;

        hr = IDXGIFactory2_CreateSwapChainForHwnd(global_ctx.dxgi_factory,
                    (IUnknown*) global_ctx.d3d_device, canvas->win->hwnd, &desc,
                    NULL, NULL, &canvas->dxgi_swapchain);
        if(hr != S_OK) {
            XTRACE_HR("IDXGIFactory2::CreateSwapChainForHwnd()");
            goto err;
        }
    }

    if(canvas->d2d_device_ctx == NULL) {
        hr = ID2D1Device_CreateDeviceContext(global_ctx.d2d_device, 0, &canvas->d2d_device_ctx);
        if(hr != S_OK) {
            XTRACE_HR("ID2D1Device::CreateDeviceContext()");
            goto err;
        }
    }

    if(canvas->d2d_bitmap == NULL) {
        IDXGISurface* dxgi_buffer;
        D2D1_BITMAP_PROPERTIES1 d2d_bitmap_properties;

        hr = IDXGISwapChain1_GetBuffer(canvas->dxgi_swapchain, 0,
                    &IID_IDXGISurface, (void**) &dxgi_buffer);
        if(hr != S_OK) {
            XTRACE_HR("IDXGISwapChain1::GetBuffer()");
            goto err;
        }

        d2d_setup_bitmap_properties(&d2d_bitmap_properties, D2D_BITMAP_BACKBUFFER);
        hr = ID2D1DeviceContext_CreateBitmapFromDxgiSurface(canvas->d2d_device_ctx, dxgi_buffer,
                    &d2d_bitmap_properties, &canvas->d2d_bitmap);
        IDXGISurface_Release(dxgi_buffer);
        if(hr != S_OK) {
            XTRACE_HR("ID2D1DeviceContext::CreateBitmapFromDxgiSurface()");
            goto err;
        }
    }

    ID2D1DeviceContext_SetTarget(canvas->d2d_device_ctx, (void*) canvas->d2d_bitmap);
    ID2D1DeviceContext_SetTextRenderingParams(canvas->d2d_device_ctx,
            (IDWriteRenderingParams*) canvas->win->mon_info->text_rendering_params);

    /* Success */
    canvas->is_connected = TRUE;
    return S_OK;

err:
    win_canvas_disconnect(canvas);
    return hr;
}

static void
win_canvas_disconnect(WinCanvas* canvas)
{
    canvas->is_connected = FALSE;

    if(canvas->d2d_bitmap != NULL) {
        ID2D1Bitmap1_Release(canvas->d2d_bitmap);
        canvas->d2d_bitmap = NULL;
    }
    if(canvas->d2d_device_ctx != NULL) {
        ID2D1DeviceContext_Release(canvas->d2d_device_ctx);
        canvas->d2d_device_ctx = NULL;
    }
    if(canvas->dxgi_swapchain != NULL) {
        IDXGISwapChain1_Release(canvas->dxgi_swapchain);
        canvas->dxgi_swapchain = NULL;
    }
}

/* This must be called in response to a resize of the window which owns
 * this context. */
static void
win_canvas_resize(WinCanvas* canvas, UINT width, UINT height)
{
    HRESULT hr = E_FAIL;

    XTRACE("win_canvas_resize(%u, %u)", width, height);

    canvas->is_connected = FALSE;

    if(canvas->d2d_device_ctx != NULL)
        ID2D1DeviceContext_SetTarget(canvas->d2d_device_ctx, NULL);

    if(canvas->d2d_bitmap != NULL) {
        ID2D1Bitmap1_Release(canvas->d2d_bitmap);
        canvas->d2d_bitmap = NULL;
    }

    if(canvas->dxgi_swapchain != NULL) {
        hr = IDXGISwapChain1_ResizeBuffers(canvas->dxgi_swapchain, 0, width, height,
                    DXGI_FORMAT_B8G8R8A8_UNORM, 0);
        if(hr != S_OK) {
            /* Ouch, lets get to a well-defined state at least, and have it
             * sorted in the next WM_PAINT. */
            XTRACE_HR("IDXGISwapChain1::ResizeBuffers()");
            win_canvas_disconnect(canvas);
        }
    }

    InvalidateRect(canvas->win->hwnd, NULL, FALSE);
}


/*************************************
 *** Public GfxCanvas abstraction ***
 *************************************/

struct GfxCanvas {
    union {
        /* Every abstraction which wants to be compatible with our canvas
         * drawing routines must begin with these members. */
        struct {
            Manager manager;
            ID2D1DeviceContext* d2d_device_ctx;
            BOOL is_connected;
        };

        WinCanvas win_canvas;
    };
};

GfxCanvas*
gfx_create_mem_canvas(UINT width, UINT height)
{
    MemCanvas* canvas;
    HRESULT hr;

    canvas = (MemCanvas*) fe_malloc(sizeof(MemCanvas));
    if(canvas == NULL)
        return NULL;
    memset(canvas, 0, sizeof(MemCanvas));

    manager_init(&canvas->manager, TYPE_MEM_CANVAS);
    canvas->size.width = width;
    canvas->size.height = height;

    hr = mem_canvas_connect(canvas);
    if(hr != S_OK) {
        XTRACE_HR("mem_canvas_connect()");
        /* Note we do not fail here, we just remain "disconnected" until
         * gfx_begin(). */
    }

    return (GfxCanvas*) canvas;
}

GfxCanvas*
gfx_create_win_canvas(Window* win)
{
    WinCanvas* canvas;
    HRESULT hr;

    canvas = (WinCanvas*) fe_malloc(sizeof(WinCanvas));
    if(canvas == NULL)
        return NULL;
    memset(canvas, 0, sizeof(WinCanvas));

    manager_init(&canvas->manager, TYPE_WIN_CANVAS);
    canvas->win = win;

    hr = win_canvas_connect(canvas);
    if(hr != S_OK) {
        XTRACE_HR("win_canvas_connect()");
        /* Note we do not fail here, we just remain "disconnected" until
         * gfx_begin(). */
    }

    return (GfxCanvas*) canvas;
}

HRESULT
gfx_resize_canvas(GfxCanvas* canvas, UINT width, UINT height)
{
    if(canvas->manager.type != TYPE_WIN_CANVAS) {
        XTRACE_W("gfx_resize_canvas: Unsupported canvas type.");
        return E_NOTIMPL;
    }

    win_canvas_resize((WinCanvas*) canvas, width, height);
    return S_OK;
}

void
gfx_set_text_rendering_params(GfxCanvas* canvas, TextRenderingParams* params)
{
    ID2D1DeviceContext_SetTextRenderingParams(canvas->d2d_device_ctx,
                (IDWriteRenderingParams*) params);
}

void
gfx_destroy_canvas(GfxCanvas* canvas)
{
    manager_fini(&canvas->manager);

    switch(canvas->manager.type) {
        case TYPE_WIN_CANVAS:   win_canvas_disconnect((WinCanvas*) canvas); break;
        case TYPE_MEM_CANVAS:   mem_canvas_disconnect((MemCanvas*) canvas); break;
        default:                UNREACHABLE();
    }

    fe_free(canvas);
}

HRESULT
gfx_begin_draw(GfxCanvas* canvas, PAINTSTRUCT* ps)
{
    HRESULT hr = E_FAIL;

    switch(canvas->manager.type) {
        case TYPE_MEM_CANVAS:
        {
            MemCanvas* mem_canvas = (MemCanvas*) canvas;
            if(!mem_canvas->is_connected) {
                hr = mem_canvas_connect(mem_canvas);
                if(FAILED(hr)) {
                    XTRACE_HR("mem_canvas_connect()");
                    return hr;
                }
            }
            break;
        }

        case TYPE_WIN_CANVAS:
        {
            WinCanvas* win_canvas = (WinCanvas*) canvas;

            if(!win_canvas->is_connected) {
                hr = win_canvas_connect(win_canvas);
                if(FAILED(hr)) {
                    XTRACE_HR("win_canvas_connect()");
                    return hr;
                }
            }

            if(ps != NULL)
                BeginPaint(win_canvas->win->hwnd, ps);
            break;
        }

        default:
            UNREACHABLE();
    }

    ID2D1DeviceContext_BeginDraw(canvas->d2d_device_ctx);
    return S_OK;
}

HRESULT
gfx_end_draw(GfxCanvas* canvas, PAINTSTRUCT* ps)
{
    HRESULT hr = E_FAIL;
    D2D1_TAG ignored;

    hr = ID2D1DeviceContext_EndDraw(canvas->d2d_device_ctx, &ignored, &ignored);
    if(FAILED(hr))
        XTRACE_HR("ID2D1DeviceContext::EndDraw()");

    if(canvas->manager.type == TYPE_WIN_CANVAS) {
        WinCanvas* win_canvas = (WinCanvas*) canvas;
        if(SUCCEEDED(hr)) {
            hr = IDXGISwapChain1_Present(win_canvas->dxgi_swapchain, 1, 0);
            if(FAILED(hr))
                XTRACE_HR("IDXGISwapChain1::Present()");
        }

        if(ps != NULL)
            EndPaint(win_canvas->win->hwnd, ps);
    }

    switch(hr) {
        case DXGI_ERROR_DEVICE_RESET:
        case DXGI_ERROR_DEVICE_REMOVED:
        case DXGI_ERROR_DRIVER_INTERNAL_ERROR:
            XTRACE_W("Reseting complete D2D machinery.");
            global_reset();
            break;

        case D2DERR_RECREATE_TARGET:
            XTRACE_W("Reseting canvas.");
            manager_reset(&canvas->manager);
            break;
    }

    return hr;
}


/*************************
 *** Painting Routines ***
 *************************/

void
gfx_clear(GfxCanvas* canvas, GfxColor color)
{
    D2D_COLOR_F d2d_color = D2D_COLOR_INIT(color);

    ID2D1DeviceContext_Clear(canvas->d2d_device_ctx, &d2d_color);
}

void
gfx_draw_point(GfxCanvas* canvas, float x, float y, GfxColor color)
{
    D2D_RECT_F d2d_rect = { x - 0.5f, y - 0.5f, x + 0.5f, y + 0.5f };
    D2D_COLOR_F d2d_color = D2D_COLOR_INIT(color);

    ID2D1SolidColorBrush_SetColor(global_ctx.d2d_solid_color_brush, &d2d_color);
    ID2D1DeviceContext_FillRectangle(canvas->d2d_device_ctx, &d2d_rect,
                (ID2D1Brush*) global_ctx.d2d_solid_color_brush);
}

void
gfx_draw_line(GfxCanvas* canvas, float x0, float y0, float x1, float y1, GfxColor color)
{
    D2D_POINT_2F pt[2] = { { x0, y0 }, { x1, y1 } };
    D2D_COLOR_F d2d_color = D2D_COLOR_INIT(color);

    ID2D1SolidColorBrush_SetColor(global_ctx.d2d_solid_color_brush, &d2d_color);
    ID2D1DeviceContext_DrawLine(canvas->d2d_device_ctx, pt[0], pt[1],
                (ID2D1Brush*) global_ctx.d2d_solid_color_brush, 1.0f, NULL);
}

void
gfx_draw_rect(GfxCanvas* canvas, float x0, float y0, float x1, float y1, GfxColor color)
{
    D2D_RECT_F d2d_rect = { x0, y0, x1, y1 };
    D2D_COLOR_F d2d_color = D2D_COLOR_INIT(color);

    ID2D1SolidColorBrush_SetColor(global_ctx.d2d_solid_color_brush, &d2d_color);
    ID2D1DeviceContext_DrawRectangle(canvas->d2d_device_ctx, &d2d_rect,
                (ID2D1Brush*) global_ctx.d2d_solid_color_brush, 1.0f, NULL);
}

void
gfx_draw_rounded_rect(GfxCanvas* canvas, float x0, float y0, float x1, float y1, float rx, float ry, GfxColor color)
{
    D2D1_ROUNDED_RECT d2d_rounded_rect = { { x0, y0, x1, y1 }, rx, ry };
    D2D_COLOR_F d2d_color = D2D_COLOR_INIT(color);

    ID2D1SolidColorBrush_SetColor(global_ctx.d2d_solid_color_brush, &d2d_color);
    ID2D1DeviceContext_DrawRoundedRectangle(canvas->d2d_device_ctx, &d2d_rounded_rect,
                (ID2D1Brush*) global_ctx.d2d_solid_color_brush, 1.0f, NULL);
}

void
gfx_fill_rect(GfxCanvas* canvas, float x0, float y0, float x1, float y1, GfxColor color)
{
    D2D_RECT_F d2d_rect = { x0, y0, x1, y1 };
    D2D_COLOR_F d2d_color = D2D_COLOR_INIT(color);

    ID2D1SolidColorBrush_SetColor(global_ctx.d2d_solid_color_brush, &d2d_color);
    ID2D1DeviceContext_FillRectangle(canvas->d2d_device_ctx, &d2d_rect,
                (ID2D1Brush*) global_ctx.d2d_solid_color_brush);
}

void
gfx_fill_rounded_rect(GfxCanvas* canvas, float x0, float y0, float x1, float y1, float rx, float ry, GfxColor color)
{
    D2D1_ROUNDED_RECT d2d_rounded_rect = { { x0, y0, x1, y1 }, rx, ry };
    D2D_COLOR_F d2d_color = D2D_COLOR_INIT(color);

    ID2D1SolidColorBrush_SetColor(global_ctx.d2d_solid_color_brush, &d2d_color);
    ID2D1DeviceContext_FillRoundedRectangle(canvas->d2d_device_ctx, &d2d_rounded_rect,
                (ID2D1Brush*) global_ctx.d2d_solid_color_brush);
}

void
gfx_draw_text_layout(GfxCanvas* canvas, float x0, float y0, TextLayout* layout, GfxColor color)
{
    D2D_POINT_2F pt = { x0, y0 };
    D2D_COLOR_F d2d_color = D2D_COLOR_INIT(color);

    ID2D1SolidColorBrush_SetColor(global_ctx.d2d_solid_color_brush, &d2d_color);
    ID2D1DeviceContext_DrawTextLayout(canvas->d2d_device_ctx, pt, (IDWriteTextLayout*) layout,
            (ID2D1Brush*) global_ctx.d2d_solid_color_brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
}


/*****************************
 *** Public Misc Functions ***
 *****************************/

void
gfx_trim_mem(void)
{
    IDXGIDevice3* dxgi_device3;
    HRESULT hr;

    if(!global_ctx.is_connected)
        return;

    if(global_ctx.no_trim)
        return;

    hr = IDXGIDevice_QueryInterface(global_ctx.dxgi_device,
                &IID_IDXGIDevice3, (void**) &dxgi_device3);
    if(FAILED(hr)) {
        XTRACE_HR("IDXGIDevice::QueryInterface(IID_IDXGIDevice3)");
        global_ctx.no_trim = TRUE;
        return;
    }

    IDXGIDevice3_Trim(dxgi_device3);
    IDXGIDevice3_Release(dxgi_device3);
}


/*****************************
 *** Global Initialization ***
 *****************************/

HRESULT
gfx_init(void)
{
    HRESULT hr;

    memset(&global_ctx, 0, sizeof(GlobalCtx));
    list_init(&global_ctx.manager_list);

    hr = global_ctx_connect();
    if(hr != S_OK)
        XTRACE_HR("global_ctx_connect()");

    return hr;
}

void
gfx_fini(void)
{
    global_reset();
}
