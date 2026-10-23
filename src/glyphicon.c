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

#include "glyphicon.h"
#include "gfx_internal.h"


/* Globals for gfx.c */
IDWriteFontFace3* glyphicon_font_face = NULL;

GlyphIconId
glyphicon_get(UINT32 codepoint)
{
    GlyphIconId id;
    HRESULT hr;

    if(glyphicon_font_face == NULL)
        return 0;

    hr = IDWriteFontFace3_GetGlyphIndices(glyphicon_font_face, &codepoint, 1, &id);
    if(FAILED(hr)) {
        XTRACE_HR("IDWriteFontFace3::GetGlyphIndices(%x)", codepoint);
        return 0;
    }

    return id;
}


float
glyphicon_preferred_size(float size)
{
    static const float preferred_sizes[] =
            { 8.0f, 12.0f, 16.0f, 20.0f, 24.0f, 32.0f, 40.0f, 48.0f, 64.0f, 96.0f, 128.0f, 192.0f, 256.0f };

    float preferred = 0.0f;
    unsigned i;

    for(i = 0; i < SIZEOF_ARRAY(preferred_sizes); i++) {
        if(preferred_sizes[i] < size)
            preferred = preferred_sizes[i];
        else
            break;
    }

    return preferred;
}


HRESULT
glyphicon_init(void)
{
    static const WCHAR* font_list[] = { L"Segoe Fluent Icons", L"Segoe MDL2 Assets" };

    IDWriteFontSet* system_set = NULL;
    IDWriteFontSet* filtered_set = NULL;
    IDWriteFontFaceReference* font_face_ref = NULL;
    unsigned i;
    HRESULT hr;

    /* Note: We will always succeed no matter what fails below.
     *       Better not paint some icons than abort everything. */

    hr = IDWriteFactory3_GetSystemFontSet(dwrite_factory3, &system_set);
    if(FAILED(hr)) {
        XTRACE_HR("IDWriteFactory3::GetSystemFontSet()");
        return S_OK;
    }

    for(i = 0; i < SIZEOF_ARRAY(font_list); i++) {
        hr = IDWriteFontSet_GetMatchingFonts(system_set, font_list[i],
                    DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
                    DWRITE_FONT_STYLE_NORMAL, &filtered_set);
        if(FAILED(hr)) {
            XTRACE_HR("IDWriteFontSet::GetMatchingFonts() failed.");
            continue;
        }

        if(IDWriteFontSet_GetFontCount(filtered_set) > 0) {
            XTRACE("Using %ls", font_list[i]);
            break;
        }

        IDWriteFontSet_Release(filtered_set);
        filtered_set = NULL;
    }

    if(filtered_set == NULL) {
        XTRACE_W("No icon font found.");
        /* We succceed anyway, likely better than just abort everything. */
        return S_OK;
    }

    hr = IDWriteFontSet_GetFontFaceReference(filtered_set, 0, &font_face_ref);
    if(FAILED(hr))
        XTRACE_HR("IDWriteFontSet::GetFontFaceReference(0)");

    if(font_face_ref != NULL) {
        hr = IDWriteFontFaceReference_CreateFontFace(font_face_ref, &glyphicon_font_face);
        if(FAILED(hr))
            XTRACE_HR("IDWriteFontFaceReference::CreateFontFace()");
    }

    if(font_face_ref != NULL)
        IDWriteFontFaceReference_Release(font_face_ref);
    if(filtered_set != NULL)
        IDWriteFontSet_Release(filtered_set);
    if(system_set != NULL)
        IDWriteFontSet_Release(system_set);
    return S_OK;
}

void
glyphicon_fini(void)
{
    if(glyphicon_font_face != NULL) {
        IDWriteFontFace3_Release(glyphicon_font_face);
        glyphicon_font_face = NULL;
    }
}
