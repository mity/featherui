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

#include "draw.h"

#define HELLO_FROM_DRAW_C
#include "draw_internal.h"

#include "libex/list.h"


/********************
 *** Misc helpers ***
 ********************/

typedef enum D2dBitmapPurpose D2dBitmapPurpose;
enum D2dBitmapPurpose {
    D2D_BITMAP_BACKBUFFER = 0,  /* a back buffer for HWND canvas */
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

    List manager_list;
    BOOL is_connected;
    BOOL no_trim;
};

static GlobalCtx global_ctx = { 0 };


/* Every abstraction which holds some device-dependent resources has to be
 * a "manager". */
typedef enum ManagerType ManagerType;
enum ManagerType {
    TYPE_HWND_CANVAS,
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
typedef struct HwndCanvas HwndCanvas;
static void mem_canvas_disconnect(MemCanvas* canvas);
static void hwnd_canvas_disconnect(HwndCanvas* canvas);

static void
manager_reset(Manager* manager)
{
    switch(manager->type) {
        /* Note: We just shut disconnect. Individual managers should be
         * to re-connect on their own in a lazy fashion. */
        case TYPE_MEM_CANVAS:   mem_canvas_disconnect((MemCanvas*) manager); break;
        case TYPE_HWND_CANVAS:  hwnd_canvas_disconnect((HwndCanvas*) manager); break;
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
    HRESULT hr;

    hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL,
                D3D11_CREATE_DEVICE_BGRA_SUPPORT, d3d_feature_levels,
                SIZEOF_ARRAY(d3d_feature_levels), D3D11_SDK_VERSION,
                &global_ctx.d3d_device, NULL, &global_ctx.d3d_device_ctx);
    if(FAILED(hr)) {
        TRACE_HR("D3D11CreateDevice()");
        goto err;
    }
    hr = ID3D11Device_QueryInterface(global_ctx.d3d_device, &IID_IDXGIDevice,
                (void**) &global_ctx.dxgi_device);
    if(FAILED(hr)) {
        TRACE_HR("ID3D11Device::QueryInterface()");
        goto err;
    }
    hr = IDXGIDevice_GetAdapter(global_ctx.dxgi_device, &global_ctx.dxgi_adapter);
    if(FAILED(hr)) {
        TRACE_HR("IDXGIDevice::GetAdapter()");
        goto err;
    }
    hr = IDXGIAdapter_GetParent(global_ctx.dxgi_adapter, &IID_IDXGIFactory2, (void**) &global_ctx.dxgi_factory);
    if(FAILED(hr)) {
        TRACE_HR("IDXGIAdapter::GetParent()");
        goto err;
    }
    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &IID_ID2D1Factory1, &d2d_factory_options,
                (void**) &global_ctx.d2d_factory);
    if(FAILED(hr)) {
        TRACE_HR("D2D1CreateFactory()");
        goto err;
    }
    hr = ID2D1Factory1_CreateDevice(global_ctx.d2d_factory, global_ctx.dxgi_device, &global_ctx.d2d_device);
    if(FAILED(hr)) {
        TRACE_HR("ID2D1Factory1::CreateDevice()");
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
typedef struct HwndCanvas HwndCanvas;
static void hwnd_canvas_disconnect(HwndCanvas* manager);


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
            TRACE_HR("global_ctx_connect");
            goto err;
        }
    }

    if(canvas->d2d_device_ctx == NULL) {
        hr = ID2D1Device_CreateDeviceContext(global_ctx.d2d_device, 0, &canvas->d2d_device_ctx);
        if(FAILED(hr)) {
            TRACE_HR("ID2D1Device::CreateDeviceContext()");
            goto err;
        }
    }

    if(canvas->d2d_bitmap == NULL) {
        d2d_setup_bitmap_properties(&d2d_bmp_properties, D2D_BITMAP_CANVAS);
        hr = ID2D1DeviceContext_CreateBitmap1(canvas->d2d_device_ctx,
                canvas->size, NULL, 0, &d2d_bmp_properties, &d2d_bitmap);
        if(hr != S_OK) {
            TRACE_HR("ID2D1DeviceContext::CreateBitmap()");
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


/*******************
 *** HWND Canvas ***
 *******************/

struct HwndCanvas {
    Manager manager;                    /* Keep 1st */
    ID2D1DeviceContext* d2d_device_ctx; /* Keep 2nd */
    IDXGISwapChain1* dxgi_swapchain;
    ID2D1Bitmap1* d2d_bitmap;

    HWND hwnd;
    BOOL is_connected;
};

static HRESULT
hwnd_canvas_connect(HwndCanvas* canvas)
{
    HRESULT hr = E_FAIL;

    /* Check the global ctx is ready. It may not be, if there has been a reset
     * of the graphics card. */
    if(!global_ctx.is_connected) {
        hr = global_ctx_connect();
        if(hr != S_OK) {
            TRACE_HR("global_ctx_connect");
            goto err;
        }
    }

    /* We may be semi-initialized in response to WM_RESIZE, hence check which
     * members are fine. */
    if(canvas->dxgi_swapchain == NULL) {
        DXGI_SWAP_CHAIN_DESC1 desc = { 0 };
        RECT client;

        GetClientRect(canvas->hwnd, &client);

        desc.Width = rect_width(&client);
        desc.Height = rect_height(&client);
        desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.BufferCount = 2;
        desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        desc.Scaling = DXGI_SCALING_NONE;

        hr = IDXGIFactory2_CreateSwapChainForHwnd(global_ctx.dxgi_factory,
                    (IUnknown*) global_ctx.d3d_device, canvas->hwnd, &desc,
                    NULL, NULL, &canvas->dxgi_swapchain);
        if(hr != S_OK) {
            TRACE_HR("IDXGIFactory2::CreateSwapChainForHwnd()");
            goto err;
        }
    }

    if(canvas->d2d_device_ctx == NULL) {
        hr = ID2D1Device_CreateDeviceContext(global_ctx.d2d_device, 0, &canvas->d2d_device_ctx);
        if(hr != S_OK) {
            TRACE_HR("ID2D1Device::CreateDeviceContext()");
            goto err;
        }
    }

    if(canvas->d2d_bitmap == NULL) {
        IDXGISurface* dxgi_buffer;
        D2D1_BITMAP_PROPERTIES1 d2d_bitmap_properties;

        hr = IDXGISwapChain1_GetBuffer(canvas->dxgi_swapchain, 0,
                    &IID_IDXGISurface, (void**) &dxgi_buffer);
        if(hr != S_OK) {
            TRACE_HR("IDXGISwapChain1::GetBuffer()");
            goto err;
        }

        d2d_setup_bitmap_properties(&d2d_bitmap_properties, D2D_BITMAP_BACKBUFFER);
        hr = ID2D1DeviceContext_CreateBitmapFromDxgiSurface(canvas->d2d_device_ctx, dxgi_buffer,
                    &d2d_bitmap_properties, &canvas->d2d_bitmap);
        IDXGISurface_Release(dxgi_buffer);
        if(hr != S_OK) {
            TRACE_HR("ID2D1DeviceContext::CreateBitmapFromDxgiSurface()");
            goto err;
        }
    }

    ID2D1DeviceContext_SetTarget(canvas->d2d_device_ctx, (void*) canvas->d2d_bitmap);

    /* Success */
    canvas->is_connected = TRUE;
    return S_OK;

err:
    hwnd_canvas_disconnect(canvas);
    return hr;
}

static void
hwnd_canvas_disconnect(HwndCanvas* canvas)
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
hwnd_canvas_resize(HwndCanvas* canvas, UINT width, UINT height)
{
    HRESULT hr = E_FAIL;

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
            TRACE_HR("IDXGISwapChain1::ResizeBuffers()");
            hwnd_canvas_disconnect(canvas);
        }
    }

    InvalidateRect(canvas->hwnd, NULL, FALSE);
}


/*************************************
 *** Public DrawCanvas abstraction ***
 *************************************/

struct DrawCanvas {
    union {
        /* Every abstraction which wants to be compatible with our canvas
         * drawing routines must begin with these members. */
        struct {
            Manager manager;
            ID2D1DeviceContext* d2d_device_ctx;
            BOOL is_connected;
        };

        HwndCanvas hwnd_canvas;
    };
};

DrawCanvas*
draw_create_mem_canvas(UINT width, UINT height)
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
        TRACE_HR("mem_canvas_connect()");
        /* Note we do not fail here, we just remain "disconnected" until
         * draw_begin(). */
    }

    return (DrawCanvas*) canvas;
}

DrawCanvas*
draw_create_hwnd_canvas(HWND hwnd)
{
    HwndCanvas* canvas;
    HRESULT hr;

    canvas = (HwndCanvas*) fe_malloc(sizeof(HwndCanvas));
    if(canvas == NULL)
        return NULL;
    memset(canvas, 0, sizeof(HwndCanvas));

    manager_init(&canvas->manager, TYPE_HWND_CANVAS);
    canvas->hwnd = hwnd;

    hr = hwnd_canvas_connect(canvas);
    if(hr != S_OK) {
        TRACE_HR("hwnd_canvas_connect()");
        /* Note we do not fail here, we just remain "disconnected" until
         * draw_begin(). */
    }

    return (DrawCanvas*) canvas;
}

HRESULT
draw_resize_canvas(DrawCanvas* canvas, UINT width, UINT height)
{
    if(canvas->manager.type != TYPE_HWND_CANVAS) {
        TRACE("draw_resize_canvas: Unsupported canvas type.");
        return E_NOTIMPL;
    }

    hwnd_canvas_resize((HwndCanvas*) canvas, width, height);
    return S_OK;
}

void
draw_destroy_canvas(DrawCanvas* canvas)
{
    manager_fini(&canvas->manager);

    switch(canvas->manager.type) {
        case TYPE_HWND_CANVAS:  hwnd_canvas_disconnect((HwndCanvas*) canvas); break;
        case TYPE_MEM_CANVAS:   mem_canvas_disconnect((MemCanvas*) canvas); break;
        default:                UNREACHABLE();
    }

    fe_free(canvas);
}

HRESULT
draw_begin(DrawCanvas* canvas, PAINTSTRUCT* ps)
{
    HRESULT hr = E_FAIL;

    switch(canvas->manager.type) {
        case TYPE_MEM_CANVAS:
        {
            MemCanvas* mem_canvas = (MemCanvas*) canvas;
            if(!mem_canvas->is_connected) {
                hr = mem_canvas_connect(mem_canvas);
                if(FAILED(hr)) {
                    TRACE_HR("mem_canvas_connect()");
                    return hr;
                }
            }
            break;
        }

        case TYPE_HWND_CANVAS:
        {
            HwndCanvas* hwnd_canvas = (HwndCanvas*) canvas;

            if(!hwnd_canvas->is_connected) {
                hr = hwnd_canvas_connect(hwnd_canvas);
                if(FAILED(hr)) {
                    TRACE_HR("hwnd_canvas_connect()");
                    return hr;
                }
            }

            BeginPaint(hwnd_canvas->hwnd, ps);
            break;
        }

        default:
            UNREACHABLE();
    }

    ID2D1DeviceContext_BeginDraw(canvas->d2d_device_ctx);
    return S_OK;
}

HRESULT
draw_end(DrawCanvas* canvas, PAINTSTRUCT* ps)
{
    HRESULT hr = E_FAIL;
    D2D1_TAG ignored;

    hr = ID2D1DeviceContext_EndDraw(canvas->d2d_device_ctx, &ignored, &ignored);
    if(FAILED(hr))
        TRACE_HR("ID2D1DeviceContext::EndDraw()");

    if(canvas->manager.type == TYPE_HWND_CANVAS) {
        HwndCanvas* hwnd_canvas = (HwndCanvas*) canvas;
        if(SUCCEEDED(hr)) {
            hr = IDXGISwapChain1_Present(hwnd_canvas->dxgi_swapchain, 1, 0);
            if(FAILED(hr))
                TRACE_HR("IDXGISwapChain1::Present()");
        }

        EndPaint(hwnd_canvas->hwnd, ps);
    }

    switch(hr) {
        case DXGI_ERROR_DEVICE_RESET:
        case DXGI_ERROR_DEVICE_REMOVED:
        case DXGI_ERROR_DRIVER_INTERNAL_ERROR:
            TRACE("Reseting complete D2D machinery.");
            global_reset();
            break;

        case D2DERR_RECREATE_TARGET:
            TRACE("Reseting canvas.");
            manager_reset(&canvas->manager);
            break;
    }

    return hr;
}


/*************************
 *** Painting Routines ***
 *************************/

void
draw_clear(DrawCanvas* canvas, const DrawColor* c)
{
    ID2D1DeviceContext_Clear(canvas->d2d_device_ctx, (const D2D_COLOR_F*) c);
}


/*****************************
 *** Public Misc Functions ***
 *****************************/

void
draw_trim_gfx_mem(void)
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
        TRACE_HR("IDXGIDevice::QueryInterface(IID_IDXGIDevice3)");
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
draw_init(void)
{
    HRESULT hr;

    memset(&global_ctx, 0, sizeof(GlobalCtx));
    list_init(&global_ctx.manager_list);

    hr = global_ctx_connect();
    if(hr != S_OK)
        TRACE_HR("global_ctx_connect()");

    return hr;
}

void
draw_fini(void)
{
    global_reset();
}
