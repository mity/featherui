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

#include <stdarg.h>
#include "text.h"
#include "gfx_internal.h"


static const GUID x_IID_IDWriteFactory3 =
        {0x9a1b41c3,0xd3bb,0x466a,{0x87,0xfc,0xfe,0x67,0x55,0x6a,0x3b,0x65}};
static const GUID x_IID_IDWriteFactory6 =
        {0xf3744d80,0x21f7,0x42eb,{0xb3,0x5d,0x99,0x5b,0xc7,0x2f,0xc2,0x23}};

const WCHAR* text_default_text_font_name = NULL;
const WCHAR* text_default_icon_font_name = NULL;

IDWriteFactory3* dwrite_factory3 = NULL;     /* Required */
IDWriteFactory6* dwrite_factory6 = NULL;     /* Optional */
IDWriteFontFallback* dwrite_font_fallback = NULL;


TextLayout*
text_create_layout(const WCHAR* str, UINT str_len, Font* font,
            float max_width, float max_height, DWORD flags)
{
    static const DWRITE_TRIMMING trim_end  = { DWRITE_TRIMMING_GRANULARITY_CHARACTER, 0, 0 };
    static const DWRITE_TRIMMING trim_word = { DWRITE_TRIMMING_GRANULARITY_WORD, 0, 0 };
    static const DWRITE_TRIMMING trim_path = { DWRITE_TRIMMING_GRANULARITY_WORD, L'\\', 1 };

    IDWriteTextFormat* text_format = (IDWriteTextFormat*) font;
    IDWriteTextLayout* layout;
    IDWriteInlineObject* trim_sign;
    const DWRITE_TRIMMING* trim;
    DWRITE_TEXT_ALIGNMENT align;
    DWRITE_PARAGRAPH_ALIGNMENT vert_align;
    HRESULT hr = E_FAIL;

    hr = IDWriteFactory3_CreateTextLayout(dwrite_factory3, str, str_len,
                text_format, max_width, max_height, &layout);
    if(FAILED(hr)) {
        XTRACE_HR("IDWriteFactory3::CreateTextLayout()");
        return NULL;
    }

    switch(flags & TEXT_ALIGN_MASK) {
        default:
        case TEXT_ALIGN_LEAD:       align = DWRITE_TEXT_ALIGNMENT_LEADING; break;
        case TEXT_ALIGN_CENTER:     align = DWRITE_TEXT_ALIGNMENT_CENTER; break;
        case TEXT_ALIGN_TRAIL:      align = DWRITE_TEXT_ALIGNMENT_TRAILING; break;
        case TEXT_ALIGN_JUSTIFY:    align = DWRITE_TEXT_ALIGNMENT_JUSTIFIED; break;
    }
    if(align != DWRITE_TEXT_ALIGNMENT_LEADING)
        IDWriteTextLayout_SetTextAlignment(layout, align);

    switch(flags & TEXT_VALIGN_MASK) {
        default:
        case TEXT_VALIGN_TOP:       vert_align = DWRITE_PARAGRAPH_ALIGNMENT_NEAR; break;
        case TEXT_VALIGN_CENTER:    vert_align = DWRITE_PARAGRAPH_ALIGNMENT_CENTER; break;
        case TEXT_VALIGN_BOTTOM:    vert_align = DWRITE_PARAGRAPH_ALIGNMENT_FAR; break;
    }
    if(vert_align != DWRITE_PARAGRAPH_ALIGNMENT_NEAR)
        IDWriteTextLayout_SetParagraphAlignment(layout, vert_align);

    switch(flags & TEXT_TRIM_MASK) {
        default:
        case TEXT_TRIM_NONE:        trim = NULL; break;
        case TEXT_TRIM_END:         trim = &trim_end; break;
        case TEXT_TRIM_WORD:        trim = &trim_word; break;
        case TEXT_TRIM_PATH:        trim = &trim_path; break;
    }

    if(trim != NULL) {
        hr = IDWriteFactory3_CreateEllipsisTrimmingSign(dwrite_factory3, text_format, &trim_sign);
        if(FAILED(hr))
            XTRACE_HR("IDWriteFactory3::CreateEllipsisTrimmingSign()");
        if(trim_sign != NULL) {
            IDWriteTextLayout_SetTrimming(layout, trim, trim_sign);
            IDWriteInlineObject_Release(trim_sign);
        }
    }

    return (TextLayout*) layout;
}

void
text_destroy_layout(TextLayout* layout)
{
    IDWriteTextLayout_Release((IDWriteTextLayout*) layout);
}


TextRenderingParams*
text_create_rendering_params(HMONITOR mon_handle)
{
    IDWriteRenderingParams* params;
    HRESULT hr;

    if(mon_handle == NULL) {
        XTRACE_W("mon_handle == NULL");
        return NULL;
    }

    hr = IDWriteFactory3_CreateMonitorRenderingParams(dwrite_factory3, mon_handle, &params);
    if(FAILED(hr)) {
        XTRACE_HR("IDWriteFactory3::CreateMonitorRenderingParams()");
        return NULL;
    }

    return (TextRenderingParams*) params;
}

void
text_destroy_rendering_params(TextRenderingParams* params)
{
    if(params != NULL)
        IDWriteRenderingParams_Release((IDWriteRenderingParams*) params);
}


/*****************************
 *** Module initialization ***
 *****************************/

HRESULT
text_init(void)
{
    HRESULT hr = E_FAIL;

    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, &x_IID_IDWriteFactory3,
                (void**) &dwrite_factory3);
    if(FAILED(hr)) {
        XTRACE_HR("DWriteCreateFactory() failed.");
        text_fini();
        return hr;
    }

    hr = IDWriteFactory3_QueryInterface(dwrite_factory3, &x_IID_IDWriteFactory6, (void**) &dwrite_factory6);
    if(FAILED(hr)) {
        XTRACE_HR("IDWriteFactory3::QueryInterface(IID_IDWriteFactory6) failed. Optical sizing is not available.");
        /* No abort here. */
    }

    hr = IDWriteFactory3_GetSystemFontFallback(dwrite_factory3, &dwrite_font_fallback);
    if(FAILED(hr)) {
        XTRACE_HR("IDWriteFactory3::GetSystemFontFallback() failed.");
        text_fini();
        return hr;
    }

    return S_OK;
}

void
text_fini(void)
{
    if(dwrite_font_fallback != NULL) {
        IDWriteFontFallback_Release(dwrite_font_fallback);
        dwrite_font_fallback = NULL;
    }
    if(dwrite_factory6 != NULL) {
        IDWriteFactory6_Release(dwrite_factory6);
        dwrite_factory6 = NULL;
    }
    if(dwrite_factory3 != NULL) {
        IDWriteFactory3_Release(dwrite_factory3);
        dwrite_factory3 = NULL;
    }
}
