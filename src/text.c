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

#include "text.h"

#define HELLO_FROM_TEXT_C
#include "gfx_internal.h"


static const GUID x_IID_IDWriteFactory = {0xb859ee5a,0xd838,0x4b5b,{0xa2,0xe8,0x1a,0xdc,0x7d,0x93,0xdb,0x48}};
static IDWriteFactory* dwrite_factory = NULL;


TextFont*
text_create_font_ex(const WCHAR* name, float size,
        TextFontWeight weight, TextFontStyle style, TextFontStretch stretch)
{
    WCHAR locale[LOCALE_NAME_MAX_LENGTH];
    IDWriteTextFormat* text_format;
    HRESULT hr = E_FAIL;

    GetUserDefaultLocaleName(locale, LOCALE_NAME_MAX_LENGTH);

    hr = IDWriteFactory_CreateTextFormat(dwrite_factory, name, NULL,
                (DWRITE_FONT_WEIGHT) weight, (DWRITE_FONT_STYLE) style,
                (DWRITE_FONT_STRETCH) stretch, size, locale, &text_format);
    if(FAILED(hr)) {
        XTRACE_HR("IDWriteFactory::CreateTextFormat()");
        return NULL;
    }

    return (TextFont*) text_format;
}

TextFont*
text_create_font(const WCHAR* name, float size)
{
    return text_create_font_ex(name, size, TEXT_FONT_WEIGHT_NORMAL,
                TEXT_FONT_STYLE_NORMAL, TEXT_FONT_STRETCH_NORMAL);
}

void
text_destroy_font(TextFont* font)
{
    IDWriteTextFormat_Release((IDWriteTextFormat*) font);
}


TextLayout*
text_create_layout(const WCHAR* str, UINT str_len, TextFont* font,
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

    hr = IDWriteFactory_CreateTextLayout(dwrite_factory, str, str_len,
                text_format, max_width, max_height, &layout);
    if(FAILED(hr)) {
        XTRACE_HR("IDWriteFactory::CreateTextLayout()");
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
        hr = IDWriteFactory_CreateEllipsisTrimmingSign(dwrite_factory, text_format, &trim_sign);
        if(FAILED(hr))
            XTRACE_HR("IDWriteFactory::CreateEllipsisTrimmingSign()");
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

    hr = IDWriteFactory_CreateMonitorRenderingParams(dwrite_factory, mon_handle, &params);
    if(FAILED(hr)) {
        XTRACE_HR("IDWriteFactory()CreateMonitorRenderingParams");
        return NULL;
    }

    return (TextRenderingParams*) params;
}

void
text_destroy_rendering_params(TextRenderingParams* params)
{
    if(params == NULL)
        return;

    IDWriteRenderingParams_Release((IDWriteRenderingParams*) params);
}

HRESULT
text_init(void)
{
    HRESULT hr;

    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, &x_IID_IDWriteFactory,
                (void**) &dwrite_factory);
    if(FAILED(hr)) {
        XTRACE_HR("DWriteCreateFactory()");
        return hr;
    }

    return S_OK;
}

void
text_fini(void)
{
    IDWriteFactory_Release(dwrite_factory);
}
