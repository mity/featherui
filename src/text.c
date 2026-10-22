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
static const GUID x_IID_IDWriteTextFormat1 =
        {0x5f174b49,0x0d8b,0x4cfb,{0x8b,0xca,0xf1,0xcc,0xe9,0xd0,0x6c,0x67}};

const WCHAR* text_default_text_font_name = NULL;
const WCHAR* text_default_icon_font_name = NULL;

static IDWriteFactory3* dwrite_factory3 = NULL;     /* Required */
static IDWriteFactory6* dwrite_factory6 = NULL;     /* Optional */
static IDWriteFontFallback* dwrite_font_fallback = NULL;


TextFont*
text_create_font_ex(const WCHAR* name, float logical_size, float scaled_size,
        TextFontWeight weight, TextFontStyle style, TextFontStretch stretch,
        const WCHAR* locale_name, BOOL allow_optical_sizing)
{
    IDWriteTextFormat* text_format;
    HRESULT hr = E_FAIL;

    if(allow_optical_sizing  &&  dwrite_factory6 != NULL) {
        DWRITE_FONT_AXIS_VALUE axis_values[2] = {
            { DWRITE_FONT_AXIS_TAG_WEIGHT, (float) weight },
            { DWRITE_FONT_AXIS_TAG_OPTICAL_SIZE, logical_size * 0.75 }
        };

        hr = IDWriteFactory6_CreateTextFormat1(dwrite_factory6, name, NULL,
                    axis_values, SIZEOF_ARRAY(axis_values), scaled_size,
                    locale_name, (IDWriteTextFormat3**) &text_format);
        if(FAILED(hr)) {
            XTRACE_HR("IDWriteFactory6::CreateTextFormat()");
            return NULL;
        }
    } else {
        hr = IDWriteFactory3_CreateTextFormat(dwrite_factory3, name, NULL,
                    (DWRITE_FONT_WEIGHT) weight, (DWRITE_FONT_STYLE) style,
                    (DWRITE_FONT_STRETCH) stretch, scaled_size, locale_name,
                    &text_format);
        if(FAILED(hr)) {
            XTRACE_HR("IDWriteFactory3::CreateTextFormat()");
            return NULL;
        }
    }

    if(dwrite_font_fallback != NULL) {
        IDWriteTextFormat1* text_format1;

        hr = IDWriteTextFormat_QueryInterface(text_format, &x_IID_IDWriteTextFormat1, (void**) &text_format1);
        if(SUCCEEDED(hr)) {
            IDWriteTextFormat1_SetFontFallback(text_format1, dwrite_font_fallback);
            IDWriteTextFormat1_Release(text_format1);
        }
    }

    return (TextFont*) text_format;
}

TextFont*
text_create_font(const WCHAR* name, float logical_size, float scaled_size, const WCHAR* locale_name)
{
    return text_create_font_ex(name, logical_size, scaled_size,
                TEXT_FONT_WEIGHT_NORMAL, TEXT_FONT_STYLE_NORMAL,
                TEXT_FONT_STRETCH_NORMAL, locale_name, TRUE);
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
    if(params == NULL)
        return;

    IDWriteRenderingParams_Release((IDWriteRenderingParams*) params);
}


/*****************************
 *** Module initialization ***
 *****************************/

static const WCHAR*
text_pick_1st_font_name(IDWriteFontSet* font_set, ...)
{
    va_list args;
    DWRITE_FONT_PROPERTY font_name_property;
    IDWriteFontSet* filtered_set;
    HRESULT hr;
    const WCHAR* font_name = NULL;
    BOOL font_exists = FALSE;

    va_start(args, font_set);

    while(TRUE) {
        font_name_property.propertyId = DWRITE_FONT_PROPERTY_ID_TYPOGRAPHIC_FAMILY_NAME;
        font_name_property.propertyValue = va_arg(args, WCHAR*);
        font_name_property.localeName = NULL;
        if(font_name_property.propertyValue == NULL)
            break;
        hr = IDWriteFontSet_GetMatchingFonts1(font_set, &font_name_property, 1, &filtered_set);
        if(FAILED(hr)) {
            XTRACE_HR("IDWriteFontSet::FindFamilyName() failed.");
            /* Go on anyway. */
            continue;
        }

        font_name = font_name_property.propertyValue;
        font_exists = (IDWriteFontSet_GetFontCount(filtered_set) > 0);
        IDWriteFontSet_Release(filtered_set);

        if(font_exists)
            break;
    }

    va_end(args);

    return font_name;
}

HRESULT
text_init(void)
{
    IDWriteFontSet* font_set;
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

    hr = IDWriteFactory3_GetSystemFontSet(dwrite_factory3, &font_set);
    if(FAILED(hr)) {
        XTRACE_HR("IDWriteFactory3::GetSystemFontSet() failed.");
        text_fini();
        return hr;
    }

    /* Heuristics for default font selection. */
    if(wcsncmp(fe_locale_name, L"ja", 2) == 0)
        text_default_text_font_name = text_pick_1st_font_name(font_set,
                L"Yu Gothic UI", L"Segoe UI Variable", L"Segoe UI", NULL);
    else if(wcsncmp(fe_locale_name, L"ko", 2) == 0)
        text_default_text_font_name = text_pick_1st_font_name(font_set,
                L"Malgun Gothic", L"Segoe UI Variable", L"Segoe UI", NULL);
    else if(wcscmp(fe_locale_name, L"zh-CN") == 0  ||  wcscmp(fe_locale_name,L"zh-SG") == 0)
        text_default_text_font_name = text_pick_1st_font_name(font_set,
                L"Microsoft YaHei UI", L"Segoe UI Variable", L"Segoe UI", NULL);
    else if(wcsncmp(fe_locale_name, L"zh", 2) == 0)
        text_default_text_font_name = text_pick_1st_font_name(font_set,
                L"Microsoft JhengHei UI", L"Segoe UI Variable", L"Segoe UI", NULL);
    else if(wcsncmp(fe_locale_name, L"th", 2) == 0  ||  wcsncmp(fe_locale_name, L"lo", 2) == 0)
        text_default_text_font_name = text_pick_1st_font_name(font_set,
                L"Leelawadee UI", L"Segoe UI Variable", L"Segoe UI", NULL);
    else if(wcsncmp(fe_locale_name, L"hi", 2) == 0)
        text_default_text_font_name = text_pick_1st_font_name(font_set,
                L"Nirmala UI", L"Segoe UI Variable", L"Segoe UI", NULL);
    else
        text_default_text_font_name = text_pick_1st_font_name(font_set,
                L"Segoe UI Variable", L"Segoe UI", NULL);

    if(text_default_text_font_name == NULL) {
        /* The ultimate fallback: let the DWrite engine use whatever it wants. */
        text_default_text_font_name = L"";
    }

    text_default_icon_font_name = text_pick_1st_font_name(font_set,
                L"Segoe Fluent Icons", L"Segoe MDL2 Assets", NULL);

    XTRACE("text_default_text_font_name: '%ls'", text_default_text_font_name);
    XTRACE("text_default_icon_font_name: '%ls'", text_default_icon_font_name);

    IDWriteFontSet_Release(font_set);

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
