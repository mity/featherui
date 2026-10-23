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

#include "font.h"
#include "gfx_internal.h"


static const GUID x_IID_IDWriteTextFormat1 =
        {0x5f174b49,0x0d8b,0x4cfb,{0x8b,0xca,0xf1,0xcc,0xe9,0xd0,0x6c,0x67}};


HRESULT
font_get_metrics(const FontParams* params, FontMetrics* metrics)
{
    IDWriteFontSet* system_set = NULL;
    IDWriteFontSet* filtered_set = NULL;
    IDWriteFontFaceReference* font_face_ref = NULL;
    IDWriteFontFace3* font_face = NULL;
    DWRITE_FONT_METRICS fm;
    DWRITE_FONT_WEIGHT weight = DWRITE_FONT_WEIGHT_NORMAL;
    DWRITE_FONT_STRETCH stretch = DWRITE_FONT_STRETCH_NORMAL;
    DWRITE_FONT_STYLE style = DWRITE_FONT_STYLE_NORMAL;
    HRESULT hr = E_FAIL;

    hr = IDWriteFactory3_GetSystemFontSet(dwrite_factory3, &system_set);
    if(FAILED(hr)) {
        XTRACE_HR("IDWriteFactory3::GetSystemFontSet()");
        goto err;
    }

    if(params->optional_mask & FONT_PARAMS_WEIGHT)
        weight = (DWRITE_FONT_WEIGHT) params->weight;

    if((params->optional_mask & FONT_PARAMS_STRETCH) && params->stretch != FONT_STRETCH_UNDEFINED)
        stretch = (DWRITE_FONT_STRETCH) params->stretch;

    if(params->optional_mask & FONT_PARAMS_STYLE)
        style = (DWRITE_FONT_STYLE) params->style;

    hr = IDWriteFontSet_GetMatchingFonts(system_set, params->name, weight,
                stretch, style, &filtered_set);
    if(FAILED(hr)) {
        XTRACE_HR("IDWriteFontSet::GetMatchingFonts() failed.");
        goto err;
    }

    if(IDWriteFontSet_GetFontCount(filtered_set) == 0) {
        XTRACE_E("No such font found (%ls).", params->name);
        hr = S_FALSE;
        goto err;
    }

    hr = IDWriteFontSet_GetFontFaceReference(filtered_set, 0, &font_face_ref);
    if(FAILED(hr)) {
        XTRACE_HR("IDWriteFontSet::GetFontFaceReference(0)");
        goto err;
    }

    hr = IDWriteFontFaceReference_CreateFontFace(font_face_ref, &font_face);
    if(FAILED(hr)) {
        XTRACE_HR("IDWriteFontFaceReference::CreateFontFace()");
        goto err;
    }

    /* Success. */
    IDWriteFontFace3_GetMetrics(font_face, &fm);
    metrics->design_units_per_em = fm.designUnitsPerEm;
    metrics->ascent = fm.ascent;
    metrics->descent = fm.descent;
    metrics->line_gap = fm.lineGap;
    metrics->cap_height = fm.capHeight;
    metrics->x_height = fm.xHeight;

    hr = S_OK;

err:
    if(font_face != NULL)
        IDWriteFontFace3_Release(font_face);
    if(font_face_ref != NULL)
        IDWriteFontFaceReference_Release(font_face_ref);
    if(filtered_set != NULL)
        IDWriteFontSet_Release(filtered_set);
    if(system_set != NULL)
        IDWriteFontSet_Release(system_set);
    return hr;
}

Font*
font_create(const FontParams* params)
{
    IDWriteTextFormat* text_format;
    HRESULT hr = E_FAIL;

    if(dwrite_factory6 != NULL  &&  !params->disable_optical_sizing  &&
       (params->optional_mask & FONT_PARAMS_LOGICAL_SIZE))
    {
        DWRITE_FONT_AXIS_VALUE axis_values[4];
        unsigned n_axes = 0;

        /* Note the optical sizing is in typographic points, not pixels. */
        axis_values[n_axes].axisTag = DWRITE_FONT_AXIS_TAG_OPTICAL_SIZE;
        axis_values[n_axes].value = params->logical_size * 0.75;
        n_axes++;

        if(params->optional_mask & FONT_PARAMS_WEIGHT) {
            axis_values[n_axes].axisTag = DWRITE_FONT_AXIS_TAG_WEIGHT;
            axis_values[n_axes].value = (float) params->weight;
            n_axes++;
        }

        if(params->optional_mask & FONT_PARAMS_STYLE) {
            float slant = 0.0f;
            float italic = 0.0f;

            if(params->style == FONT_STYLE_OBLIQUE)
                slant = -20.0f;
            else if(params->style == FONT_STYLE_ITALIC)
                italic = 1.0f;

            axis_values[n_axes].axisTag = DWRITE_FONT_AXIS_TAG_SLANT;
            axis_values[n_axes].value = slant;
            n_axes++;
            axis_values[n_axes].axisTag = DWRITE_FONT_AXIS_TAG_ITALIC;
            axis_values[n_axes].value = italic;
            n_axes++;
        }

        if((params->optional_mask & FONT_PARAMS_STRETCH)  &&  params->stretch != FONT_STRETCH_UNDEFINED) {
            axis_values[n_axes].axisTag = DWRITE_FONT_AXIS_TAG_WIDTH;
            switch(params->stretch) {
                case FONT_STRETCH_ULTRA_CONDENSED:  axis_values[n_axes].value = 50.0f; break;
                case FONT_STRETCH_EXTRA_CONDENSED:  axis_values[n_axes].value = 62.5f; break;
                case FONT_STRETCH_CONDENSED:        axis_values[n_axes].value = 75.0f; break;
                case FONT_STRETCH_SEMI_CONDENSED:   axis_values[n_axes].value = 87.5f; break;
                case FONT_STRETCH_NORMAL:           axis_values[n_axes].value = 100.0f; break;
                case FONT_STRETCH_SEMI_EXPANDED:    axis_values[n_axes].value = 112.5f; break;
                case FONT_STRETCH_EXPANDED:         axis_values[n_axes].value = 125.0f; break;
                case FONT_STRETCH_EXTRA_EXPANDED:   axis_values[n_axes].value = 150.0f; break;
                case FONT_STRETCH_ULTRA_EXPANDED:   axis_values[n_axes].value = 200.0f; break;
                default:                            UNREACHABLE(); break;
            }
            n_axes++;
        }

        hr = IDWriteFactory6_CreateTextFormat1(dwrite_factory6,
                    params->name, NULL, axis_values, n_axes, params->size,
                    params->locale_name, (IDWriteTextFormat3**) &text_format);
        if(FAILED(hr)) {
            XTRACE_HR("IDWriteFactory6::CreateTextFormat()");
            return NULL;
        }
    } else {
        DWRITE_FONT_WEIGHT weight = (params->optional_mask & FONT_PARAMS_WEIGHT)
                ? (DWRITE_FONT_WEIGHT)params->weight : DWRITE_FONT_WEIGHT_REGULAR ;
        DWRITE_FONT_STYLE style = (params->optional_mask & FONT_PARAMS_STYLE)
                ? (DWRITE_FONT_STYLE)params->style : DWRITE_FONT_STYLE_NORMAL;
        DWRITE_FONT_STRETCH stretch = (params->optional_mask & FONT_PARAMS_STRETCH)
                ? (DWRITE_FONT_STRETCH)params->stretch : DWRITE_FONT_STRETCH_NORMAL;

        hr = IDWriteFactory3_CreateTextFormat(dwrite_factory3,
                    params->name, NULL, weight, style, stretch, params->size,
                    params->locale_name, &text_format);
        if(FAILED(hr)) {
            XTRACE_HR("IDWriteFactory3::CreateTextFormat()");
            return NULL;
        }
    }

    if(!params->disable_font_fallback  &&  dwrite_font_fallback != NULL) {
        IDWriteTextFormat1* text_format1;

        hr = IDWriteTextFormat_QueryInterface(text_format, &x_IID_IDWriteTextFormat1, (void**) &text_format1);
        if(SUCCEEDED(hr)) {
            IDWriteTextFormat1_SetFontFallback(text_format1, dwrite_font_fallback);
            IDWriteTextFormat1_Release(text_format1);
        }
    }

    return (Font*) text_format;
}

void
font_destroy(Font* font)
{
    if(font != NULL)
        IDWriteTextFormat_Release((IDWriteTextFormat*) font);
}
