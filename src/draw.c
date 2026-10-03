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


typedef struct GlobalCtx GlobalCtx;
struct GlobalCtx {
    ID3D11Device* d3d_device;
    ID3D11DeviceContext* d3d_device_ctx;
    IDXGIAdapter* dxgi_adapter;
    IDXGIFactory2* dxgi_factory;
    IDXGIDevice* dxgi_device;
    ID2D1Factory1* d2d_factory;
    ID2D1Device* d2d_device;

    List win_ctx_list;  /* doubly-linked list of WindowCtx */
    BOOL is_ready;
    BOOL trim_supported;
};

typedef struct WindowCtx WindowCtx;
struct WindowCtx {
    ID2D1DeviceContext* d2d_device_ctx;   /* Keep first! */
    IDXGISwapChain1* dxgi_swapchain;
    IDXGIDevice* dxgi_device;
    ID2D1Bitmap1* d2d_bitmap;

    HWND hwnd;
    ListNode win_ctx_list_node;
    BOOL is_ready;
};


static GlobalCtx global_ctx = { 0 };

static const D2D1_BITMAP_PROPERTIES1 d2d_bmp_properties = {
        .pixelFormat = {
                .format = DXGI_FORMAT_B8G8R8A8_UNORM,
                .alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED
        },
        .dpiX = 96.0,
        .dpiY = 96.0,
        .bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW
};


/* Forward declarations. */
static void global_ctx_close(void);
static void window_ctx_close(WindowCtx* wc);

static HRESULT
global_ctx_open(void)
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
    if(hr != S_OK) {
        TRACE_HR("D3D11CreateDevice()");
        goto err;
    }
    hr = ID3D11Device_QueryInterface(global_ctx.d3d_device, &IID_IDXGIDevice, (void**) &global_ctx.dxgi_device);
    if(hr != S_OK) {
        TRACE_HR("ID3D11Device::QueryInterface()");
        goto err;
    }
    hr = IDXGIDevice_GetAdapter(global_ctx.dxgi_device, &global_ctx.dxgi_adapter);
    if(hr != S_OK) {
        TRACE_HR("IDXGIDevice::GetAdapter()");
        goto err;
    }
    hr = IDXGIAdapter_GetParent(global_ctx.dxgi_adapter, &IID_IDXGIFactory2, (void**) &global_ctx.dxgi_factory);
    if(hr != S_OK) {
        TRACE_HR("IDXGIAdapter::GetParent()");
        goto err;
    }
    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
                &IID_ID2D1Factory1, &d2d_factory_options, (void**) &global_ctx.d2d_factory);
    if(hr != S_OK) {
        TRACE_HR("D2D1CreateFactory()");
        goto err;
    }
    hr = ID2D1Factory1_CreateDevice(global_ctx.d2d_factory, global_ctx.dxgi_device, &global_ctx.d2d_device);
    if(hr != S_OK) {
        TRACE_HR("ID2D1Factory1::CreateDevice()");
        goto err;
    }

    /* Success */
    global_ctx.is_ready = TRUE;
    return S_OK;

err:
    global_ctx_close();
    return hr;
}

static void
global_ctx_close(void)
{
    global_ctx.is_ready = FALSE;

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

static void
global_reset(void)
{
    ListNode* node;

    LIST_FOR_EACH(&global_ctx.win_ctx_list, node) {
        WindowCtx* win_ctx = LIST_DATA(node, WindowCtx, win_ctx_list_node);
        window_ctx_close(win_ctx);
    }
    global_ctx_close();

    /* In theory we might be using a different graphical card which now may
     * support it, so reset the flag to try if needed.*/
    global_ctx.trim_supported = TRUE;

    /* Note all the discarded stuff should be restored lazily when needed. */
}


static HRESULT
window_ctx_open(WindowCtx* wc, HWND hwnd)
{
    HRESULT hr = E_FAIL;

    ASSERT(wc->hwnd == NULL  ||  wc->hwnd == hwnd);

    /* Check the global ctx is ready. It may not be, if there has been a reset
     * of the graphical card. */
    if(!global_ctx.is_ready) {
        hr = global_ctx_open();
        if(hr != S_OK) {
            TRACE_HR("global_ctx_open");
            goto err;
        }
    }

    /* We may be semi-initialized in response to WM_RESIZE, hence check which
     * members are fine. */
    if(wc->dxgi_swapchain == NULL) {
        DXGI_SWAP_CHAIN_DESC1 desc = { 0 };
        RECT client;

        GetClientRect(hwnd, &client);

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
                    (IUnknown*) global_ctx.d3d_device, hwnd, &desc,
                    NULL, NULL, &wc->dxgi_swapchain);
        if(hr != S_OK) {
            TRACE_HR("IDXGIFactory2::CreateSwapChainForHwnd()");
            goto err;
        }
    }

    if(wc->d2d_device_ctx == NULL) {
        hr = ID2D1Device_CreateDeviceContext(global_ctx.d2d_device, 0, &wc->d2d_device_ctx);
        if(hr != S_OK) {
            TRACE_HR("ID2D1Device::CreateDeviceContext()");
            goto err;
        }
    }

    if(wc->d2d_bitmap == NULL) {
        IDXGISurface* dxgi_buffer;

        hr = IDXGISwapChain1_GetBuffer(wc->dxgi_swapchain, 0, &IID_IDXGISurface, (void**) &dxgi_buffer);
        if(hr != S_OK) {
            TRACE_HR("IDXGISwapChain1::GetBuffer()");
            goto err;
        }

        hr = ID2D1DeviceContext_CreateBitmapFromDxgiSurface(wc->d2d_device_ctx,
                    dxgi_buffer, &d2d_bmp_properties, &wc->d2d_bitmap);
        IDXGISurface_Release(dxgi_buffer);
        if(hr != S_OK) {
            TRACE_HR("ID2D1DeviceContext::CreateBitmapFromDxgiSurface()");
            goto err;
        }
    }

    ID2D1DeviceContext_SetTarget(wc->d2d_device_ctx, (void*) wc->d2d_bitmap);

    /* Success */
    wc->hwnd = hwnd;
    wc->is_ready = TRUE;
    return S_OK;

err:
    window_ctx_close(wc);
    return hr;
}

static void
window_ctx_close(WindowCtx* wc)
{
    wc->is_ready = FALSE;

    if(wc->d2d_bitmap != NULL) {
        ID2D1Bitmap1_Release(wc->d2d_bitmap);
        wc->d2d_bitmap = NULL;
    }
    if(wc->d2d_device_ctx != NULL) {
        ID2D1DeviceContext_Release(wc->d2d_device_ctx);
        wc->d2d_device_ctx = NULL;
    }
    if(wc->dxgi_device != NULL) {
        IDXGIDevice_Release(wc->dxgi_device);
        wc->dxgi_device = NULL;
    }
    if(wc->dxgi_swapchain != NULL) {
        IDXGISwapChain1_Release(wc->dxgi_swapchain);
        wc->dxgi_swapchain = NULL;
    }
}

/* This MUST be called in response to a resize of the window which owns
 * this context. */
static HRESULT
window_ctx_resize(WindowCtx* wc, UINT width, UINT height)
{
    HRESULT hr = E_FAIL;

    wc->is_ready = FALSE;

    if(wc->d2d_bitmap != NULL) {
        ID2D1Bitmap1_Release(wc->d2d_bitmap);
        wc->d2d_bitmap = NULL;
    }

    if(wc->d2d_device_ctx != NULL)
        ID2D1DeviceContext_SetTarget(wc->d2d_device_ctx, NULL);

    if(wc->dxgi_swapchain != NULL) {
        hr = IDXGISwapChain1_ResizeBuffers(wc->dxgi_swapchain, 0, width, height,
                    DXGI_FORMAT_B8G8R8A8_UNORM, 0);
        if(hr != S_OK) {
            /* Ouch, lets get to a well-defined state at least. */
            TRACE_HR("IDXGISwapChain1::ResizeBuffers()");
            window_ctx_close(wc);
            return hr;
        }
    }

    /* Try to re-create the buffers immediately and not wait for lazy
     * construction in WM_PAINT. */
    window_ctx_open(wc, wc->hwnd);

    /* Success */
    return S_OK;
}


// TODO: make this more universal with some union so that we could use it for
//          paining to an in-memory bitmap.
struct DrawCanvas {
    WindowCtx win_ctx;
};


DrawCanvas*
draw_create_hwnd_canvas(HWND hwnd)
{
    DrawCanvas* canvas;
    HRESULT hr;

    canvas = fe_malloc(sizeof(DrawCanvas));
    if(canvas == NULL)
        return NULL;
    memset(canvas, 0, sizeof(DrawCanvas));

    hr = window_ctx_open(&canvas->win_ctx, hwnd);
    if(hr != S_OK) {
        TRACE_HR("window_ctx_open()");
        fe_free(canvas);
        return NULL;
    }

    list_append(&global_ctx.win_ctx_list, &canvas->win_ctx.win_ctx_list_node);
    return canvas;
}

void
draw_destroy_canvas(DrawCanvas* canvas)
{
    list_remove(&global_ctx.win_ctx_list, &canvas->win_ctx.win_ctx_list_node);
    window_ctx_close(&canvas->win_ctx);
    fe_free(canvas);
}

void
draw_resize_hwnd_canvas(DrawCanvas* canvas, WPARAM wp, LPARAM lp)
{
    UINT width, height;

    if(wp == SIZE_MAXHIDE  ||  wp == SIZE_MAXSHOW) {
        /* Don't bother, it's not actually about _our_ size. */
        return;
    }

    width = LOWORD(lp);
    height = HIWORD(lp);

    window_ctx_resize(&canvas->win_ctx, width, height);
}

BOOL
draw_begin(DrawCanvas* canvas, PAINTSTRUCT* ps)
{
    HRESULT hr;
    // TODO: optimization: skip painting if the windows minimized or occluded

    if(!canvas->win_ctx.is_ready) {
        hr = window_ctx_open(&canvas->win_ctx, canvas->win_ctx.hwnd);
        if(hr != S_OK) {
            TRACE_HR("window_ctx_open()");
            return FALSE;
        }
    }

    BeginPaint(canvas->win_ctx.hwnd, ps);

    ID2D1DeviceContext_BeginDraw(canvas->win_ctx.d2d_device_ctx);
    return TRUE;
}

void
draw_end(DrawCanvas* canvas, PAINTSTRUCT* ps)
{
    HRESULT hr;
    D2D1_TAG ignored;

    hr = ID2D1DeviceContext_EndDraw(canvas->win_ctx.d2d_device_ctx, &ignored, &ignored);
    if(hr != S_OK) {
        TRACE_HR(ID2D1DeviceContext_EndDraw);
        if( hr == D2DERR_RECREATE_TARGET ||
            hr == DXGI_ERROR_DEVICE_RESET ||
            hr == DXGI_ERROR_DEVICE_REMOVED )
        {
            TRACE("Reseting complete D2D machinery.");
            global_reset();
            EndPaint(canvas->win_ctx.hwnd, ps);

            /* Due the reset graphical hard reset, the contents is still invalid.
             * FIXME: But perhaps we should do this with some short delay to
             *        have a better chance and to avoid a busy loop?? */
            InvalidateRect(canvas->win_ctx.hwnd, &ps->rcPaint, FALSE);
            return;
        }
    }

    hr = IDXGISwapChain1_Present(canvas->win_ctx.dxgi_swapchain, 1, 0);
    if(hr != S_OK)
        TRACE_HR("IDXGISwapChain1::Present()");

    EndPaint(canvas->win_ctx.hwnd, ps);
}

void
draw_clear(DrawCanvas* canvas, const DrawColor* c)
{
    ID2D1DeviceContext_Clear(canvas->win_ctx.d2d_device_ctx, (const D2D_COLOR_F*) c);
}

void
draw_trim_gfx_mem(void)
{
    IDXGIDevice3* dxgi_device3;
    HRESULT hr;

    if(!global_ctx.is_ready)
        return;

    if(!global_ctx.trim_supported)
        return;

    hr = IDXGIDevice_QueryInterface(global_ctx.dxgi_device,
                &IID_IDXGIDevice3, (void**) &dxgi_device3);
    if(FAILED(hr)) {
        TRACE_HR("IDXGIDevice::QueryInterface(IID_IDXGIDevice3)");
        return;
    }

    IDXGIDevice3_Trim(dxgi_device3);
    IDXGIDevice3_Release(dxgi_device3);
}


HRESULT
draw_init(void)
{
    HRESULT hr;

    list_init(&global_ctx.win_ctx_list);

    /* By default assume it's supported. We reset to FALSE if-when we find
     * otherwise. */
    global_ctx.trim_supported = TRUE;

    hr = global_ctx_open();
    if(hr != S_OK)
        TRACE_HR("global_ctx_open()");

    return hr;
}

void
draw_fini(void)
{
    global_reset();
}
