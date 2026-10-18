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

#include "fonttheme.h"
#include "text.h"
#include "dpi.h"


FontThemeId fonttheme_default_id;


typedef struct FontThemeImpl FontThemeImpl;
struct FontThemeImpl {
    FontTheme theme;
};


static struct {
    FontThemeImpl* impl;
    FontThemeId base_id;
    BOOL is_readonly;
} fonttheme_array[FONTTHEME_MAX_COUNT];


static FontThemeImpl*
fonttheme_alloc(FontThemeImpl* base_impl)
{
    FontThemeImpl* impl;

    impl = (FontThemeImpl*) fe_malloc(sizeof(FontThemeImpl));
    if(impl == NULL) {
        XTRACE_E("fe_malloc() failed.");
        return NULL;
    }

    if(base_impl != NULL)
        memcpy(impl, base_impl, sizeof(FontThemeImpl));
    else
        memset(impl, 0, sizeof(FontThemeImpl));

    return impl;
}

const FontTheme*
fonttheme_get(FontThemeId id)
{
    if(fonttheme_array[id].impl != NULL)
        return &fonttheme_array[id].impl->theme;
    else
        return NULL;
}

HRESULT
fonttheme_create(FontThemeId base_id, FontThemeId* out_id)
{
    FontThemeId id;

    for(id = 0; id < FONTTHEME_MAX_COUNT; id++) {
        if(fonttheme_array[id].impl == NULL)
            break;
    }

    if(id >= FONTTHEME_MAX_COUNT) {
        XTRACE_E("Too many themes.");
        return E_FAIL;
    }

    fonttheme_array[id].impl = fonttheme_alloc(fonttheme_array[base_id].impl);
    if(fonttheme_array[id].impl == NULL) {
        XTRACE_E("fonttheme_alloc() failed.");
        return HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
    }
    fonttheme_array[id].base_id = base_id;

    *out_id = id;
    return S_OK;
}

void
fonttheme_destroy(FontThemeId id)
{
    dpi_destroy_all_theme_data(id);

    if(fonttheme_array[id].impl != NULL) {
        fe_free(fonttheme_array[id].impl);
        fonttheme_array[id].impl = NULL;
    }
}


HRESULT
fonttheme_init(void)
{
    /* The values are coming from WinUI 3 Gallery -> Design -> Typography. */
    static const float default_font_weight[FONTTHEME_TEXT_TYPE_COUNT] = {
        TEXT_FONT_WEIGHT_REGULAR, TEXT_FONT_WEIGHT_REGULAR, TEXT_FONT_WEIGHT_SEMI_BOLD,
        TEXT_FONT_WEIGHT_REGULAR, TEXT_FONT_WEIGHT_SEMI_BOLD, TEXT_FONT_WEIGHT_SEMI_BOLD,
        TEXT_FONT_WEIGHT_SEMI_BOLD, TEXT_FONT_WEIGHT_SEMI_BOLD, TEXT_FONT_WEIGHT_SEMI_BOLD
    };
    static const float default_font_size[FONTTHEME_TEXT_TYPE_COUNT] = {
        12.0f, 14.0f, 14.0f, 18.0f, 18.0f, 20.0f, 28.0f, 40.0f, 68.0f
    };
    static const float default_line_height[FONTTHEME_TEXT_TYPE_COUNT] = {
        16.0f, 20.0f, 20.0f, 24.0f, 24.0f, 28.0f, 36.0f, 52.0f, 92.0f
    };
    FontThemeId id;

    for(id = 0; id < FONTTHEME_MAX_COUNT; id++)
        fonttheme_array[id].impl = NULL;

    fonttheme_array[0].impl = fonttheme_alloc(NULL);
    fonttheme_array[0].base_id = 0;
    fonttheme_array[0].is_readonly = TRUE;

    if(fonttheme_array[0].impl == NULL) {
        XTRACE_E("fonttheme_alloc() failed.");
        fonttheme_fini();
        return HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
    }

    fonttheme_array[0].impl->theme.locale_name = fe_locale_name;
    fonttheme_array[0].impl->theme.text_font_name = text_default_text_font_name;
    fonttheme_array[0].impl->theme.icon_font_name = text_default_icon_font_name;
    memcpy(fonttheme_array[0].impl->theme.font_weight, default_font_weight, sizeof(default_font_weight));
    memcpy(fonttheme_array[0].impl->theme.font_size, default_font_size, sizeof(default_font_size));
    memcpy(fonttheme_array[0].impl->theme.line_height, default_line_height, sizeof(default_line_height));

    fonttheme_default_id = 0;
    return S_OK;
}

void
fonttheme_fini(void)
{
    FontThemeId id;

    fonttheme_destroy(0);

    for(id = 0; id < FONTTHEME_MAX_COUNT; id++) {
        if(fonttheme_array[id].impl != NULL) {
            XTRACE_W("Detected leak of FontTheme (%u)", id);
            fonttheme_destroy(id);
        }
    }
}
