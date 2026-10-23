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
#include "font.h"
#include "dpi.h"


FontThemeId fonttheme_default_id;


static float fontheme_accessibility_scale_factor = 1.0f;


static struct {
    FontTheme* theme;
} fonttheme_array[FONTTHEME_MAX_COUNT];

const FontTheme*
fonttheme_get(FontThemeId id)
{
    return fonttheme_array[id].theme;
}

HRESULT
fonttheme_create_for_locale(const WCHAR* locale_name, FontThemeId* out_id)
{
    static const struct {
        /* Both are zero delimited, double zero as terminator */
        const WCHAR* locale_prefix_list;
        const WCHAR* font_name_list;
    } locale_map[] = {
        /* Note: Add only locale/languages for which Segoe UI + DWrite font
         * fallback mechanism is not a good choice. */

        /* Japanese */
        { L"ja\0",                      L"Yu Gothic UI\0Meiryo UI\0Meiryo\0" },
        /* Korean */
        { L"ko\0",                      L"Malgun Gothic\0" },
        /* Simplified Chinese (China, Singapore, generic Hans) */
        { L"zh-CN\0zh-SG\0zh-Hans\0",   L"Microsoft YaHei UI\0Microsoft YaHei\0" },
        /* Traditional Chinese (all other) */
        { L"zh\0",                      L"Microsoft JhengHei UI\0Microsoft JhengHei\0" },
        /* Thai + Lao + Khmer */
        { L"th\0lo\0km\0",              L"Leelawadee UI\0" },
        /* Hindi + Bengali + Tamil + Telugu + Gujarati (India, Bangladesh, Sri Lanka) */
        { L"hi\0bn\0ta\0te\0gu\0",      L"Nirmala UI\0" },
        /* Everything else; keep last */
        { NULL,                         L"Segoe UI Variable\0Segoe UI\0" }
    };

    static const struct {
        float line_height;

        float text_logical_size;
        FontWeight text_weight;

        float icon_logical_size;
        FontWeight icon_weight;
    } font_map[FONTTHEME_TEXT_TYPE_COUNT] = {
        /* Text font and line height sizes are based on values used by
         * WinUI 3 with Segoe UI or Segoe UI Variable fonts. For other fonts
         * we scale the lineHeight below to provide similar "fluent" UI look.
         * (See WinUI 3 Gallery >> Typography)
         *
         * The icon font is based on values recommended for the
         * Segoe Fluent Icons and Segoe MDL2 Assets fonts.
         * (See https://learn.microsoft.com/en-us/windows/apps/design/iconography/segoe-fluent-icons-font)
         */
        { 16.0f, 12.0f, FONT_WEIGHT_REGULAR,   12.0f, FONT_WEIGHT_REGULAR },
        { 20.0f, 14.0f, FONT_WEIGHT_REGULAR,   16.0f, FONT_WEIGHT_REGULAR },
        { 20.0f, 14.0f, FONT_WEIGHT_SEMI_BOLD, 16.0f, FONT_WEIGHT_REGULAR },
        { 24.0f, 18.0f, FONT_WEIGHT_REGULAR,   20.0f, FONT_WEIGHT_REGULAR },
        { 24.0f, 18.0f, FONT_WEIGHT_SEMI_BOLD, 20.0f, FONT_WEIGHT_REGULAR },
        { 28.0f, 20.0f, FONT_WEIGHT_SEMI_BOLD, 24.0f, FONT_WEIGHT_REGULAR },
        { 36.0f, 28.0f, FONT_WEIGHT_SEMI_BOLD, 32.0f, FONT_WEIGHT_REGULAR },
        { 52.0f, 40.0f, FONT_WEIGHT_SEMI_BOLD, 48.0f, FONT_WEIGHT_REGULAR },
        { 92.0f, 68.0f, FONT_WEIGHT_SEMI_BOLD, 64.0f, FONT_WEIGHT_REGULAR }
    };

    FontThemeId id;
    const WCHAR* font_name_list;
    const WCHAR* font_name;
    const WCHAR* text_font_name = NULL;
    const WCHAR* icon_font_name = NULL;
    HRESULT hr;
    FontMetrics metrics_regular = { 0 };
    FontMetrics metrics_semibold = { 0 };
    FontMetrics metrics_icon = { 0 };
    FontTheme* theme;
    size_t strsize;
    WCHAR* str_ptr;
    unsigned i;
    FontMetrics metrics_segoe_regular = { 0 };
    FontMetrics metrics_segoe_semibold = { 0 };
    BOOL need_line_height_fixup;

    for(id = 0; id < FONTTHEME_MAX_COUNT; id++) {
        if(fonttheme_array[id].theme == NULL)
            break;
    }

    if(id >= FONTTHEME_MAX_COUNT) {
        XTRACE_E("Too many themes.");
        return E_FAIL;
    }

    /* Find right font name list for the given locale. */
    for(i = 0; i < SIZEOF_ARRAY(locale_map); i++) {
        const WCHAR* locale_prefix;

        font_name_list = locale_map[i].font_name_list;

        if(locale_map[i].locale_prefix_list == NULL)
            break;

        for(locale_prefix = locale_map[i].locale_prefix_list;
            locale_prefix[0] != L'\0';
            locale_prefix += wcslen(locale_prefix) + 1)
        {
            if(wcsncmp(locale_name, locale_prefix, wcslen(locale_prefix)) == 0)
                break;
        }

        if(locale_prefix[0] != L'\0')
            break;
    }

    /* Find first working text font from the list. */
    for(font_name = font_name_list;
        font_name[0] != L'\0';
        font_name += wcslen(font_name) + 1)
    {
        FontParams font_params = { 0 };

        font_params.locale_name = locale_name;
        font_params.name = font_name;
        font_params.weight = FONT_WEIGHT_REGULAR;
        font_params.optional_mask |= FONT_PARAMS_WEIGHT;
        hr = font_get_metrics(&font_params, &metrics_regular);
        if(hr == S_OK) {
            font_params.weight = FONT_WEIGHT_SEMI_BOLD;
            hr = font_get_metrics(&font_params, &metrics_semibold);
        }
        if(FAILED(hr))
            XTRACE_HR("font_get_metrics(%ls)", font_name);
        if(hr == S_OK)
            break;
    }
    if(font_name[0] == L'\0') {
        XTRACE_E("No font suitable for the locale found.");
        return E_FAIL;
    }
    text_font_name = font_name;

    /* Find first working icon font from the list.
     * Unlike above, we treat this as optional. */
    font_name_list = L"Segoe Fluent Icons\0Segoe MDL2 Assets\0";
    for(font_name = font_name_list;
        font_name[0] != L'\0';
        font_name += wcslen(font_name) + 1)
    {
        FontParams font_params = { 0 };

        font_params.name = font_name;
        font_params.weight = FONT_WEIGHT_REGULAR;
        font_params.optional_mask |= FONT_PARAMS_WEIGHT;
        hr = font_get_metrics(&font_params, &metrics_icon);
        if(FAILED(hr))
            XTRACE_HR("font_get_metrics(%ls)", font_name);
        if(hr == S_OK)
            break;
    }
    icon_font_name = font_name;

    /* font_map[i].line_height comes from WinUI 3 Gallery >> Typography
     * and is only for Segoe UI font family. For other font(s) we need to do
     * a fix-up to preserve the fluent UI experience. */
    need_line_height_fixup = (wcsncmp(text_font_name, L"Segoe UI", 8) != 0);
    if(need_line_height_fixup) {
        FontParams font_params = { 0 };

        font_params.name = L"Segoe UI";
        font_params.weight = FONT_WEIGHT_REGULAR;
        font_params.optional_mask |= FONT_PARAMS_WEIGHT;
        hr = font_get_metrics(&font_params, &metrics_segoe_regular);
        if(hr != S_OK) {
            XTRACE_HR("font_get_metrics(%ls regular)", font_params.name);
            metrics_segoe_semibold.design_units_per_em = 2048;
            metrics_segoe_semibold.ascent = 2210;
            metrics_segoe_semibold.descent = -514;
            metrics_segoe_semibold.cap_height = 1434;
        }

        font_params.weight = FONT_WEIGHT_SEMI_BOLD;
        hr = font_get_metrics(&font_params, &metrics_segoe_semibold);
        if(hr != S_OK) {
            XTRACE_HR("font_get_metrics(%ls semi-bold)", font_params.name);
            metrics_segoe_semibold.design_units_per_em = 2048;
            metrics_segoe_semibold.ascent = 2210;
            metrics_segoe_semibold.descent = -514;
            metrics_segoe_semibold.cap_height = 1434;
        }
    }

    /* Finally ready to allocate. */
    strsize = 0;
    strsize += (locale_name != NULL) ? wcslen(locale_name) + 1 : 0;
    strsize += (text_font_name != NULL) ? wcslen(text_font_name) + 1 : 0;
    strsize += (icon_font_name != NULL) ? wcslen(icon_font_name) + 1 : 0;
    theme = (FontTheme*) fe_malloc(sizeof(FontTheme) + 3 * sizeof(FontMetrics) + strsize * sizeof(WCHAR));
    if(theme == NULL) {
        XTRACE_E("malloc() failed.");
        return HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
    }

    memcpy(&theme->font_metrics[0], &metrics_regular, sizeof(FontMetrics));
    memcpy(&theme->font_metrics[1], &metrics_semibold, sizeof(FontMetrics));
    memcpy(&theme->font_metrics[2], &metrics_icon, sizeof(FontMetrics));

    str_ptr = (WCHAR*)(&theme->font_metrics[3]);
    if(locale_name != NULL) {
        wcscpy(str_ptr, locale_name);
        theme->locale_name = str_ptr;
        str_ptr += wcslen(str_ptr)+1;
    } else {
        theme->locale_name = NULL;
    }
    if(text_font_name != NULL) {
        wcscpy(str_ptr, text_font_name);
        theme->text_font_name = str_ptr;
        str_ptr += wcslen(str_ptr)+1;
    } else {
        theme->text_font_name = NULL;
    }
    if(icon_font_name != NULL) {
        wcscpy(str_ptr, icon_font_name);
        theme->icon_font_name = str_ptr;
        str_ptr += wcslen(str_ptr)+1;
    } else {
        theme->icon_font_name = NULL;
    }

    theme->accessibility_scale_factor = fontheme_accessibility_scale_factor;

    for(i = 0; i < FONTTHEME_TEXT_TYPE_COUNT; i++) {
        ASSERT(font_map[i].text_weight == FONT_WEIGHT_REGULAR ||
               font_map[i].text_weight == FONT_WEIGHT_SEMI_BOLD);
        ASSERT(font_map[i].icon_weight == FONT_WEIGHT_REGULAR);

        theme->line_height[i] = font_map[i].line_height;

        /* Try to preserve "optical density" of text and actual gaps between
         * the text lines as for the Segoe UI, to provide similar UI
         * experience.
         *
         * For alphabetic scripts (Latin, Cyrillic, Greek etc.) we use
         * the capHeight (as most letters don't reach above ascent/below
         * descent); for scripts where capHeight is strange (e.g. East Asian
         * scripts which typically fill (almost) complete glyph box, we use
         * ascent + descent.)
         */
        if(need_line_height_fixup) {
            FontMetrics* metrics_segoe = (font_map[i].text_weight == FONT_WEIGHT_REGULAR)
                        ? &metrics_segoe_regular : &metrics_segoe_semibold;
            FontMetrics* metrics = (font_map[i].text_weight == FONT_WEIGHT_REGULAR)
                        ? &metrics_regular : &metrics_semibold;
            float optical_height_segoe;
            float optical_height;

            if(0 < metrics_segoe->cap_height && metrics_segoe->cap_height < metrics_segoe->ascent)
                optical_height_segoe = (float) metrics_segoe->cap_height;
            else
                optical_height_segoe = (float) (metrics_segoe->ascent + metrics_segoe->descent);
            optical_height_segoe /= (float) metrics_segoe->design_units_per_em;

            if(0 < metrics->cap_height && metrics->cap_height < metrics->ascent)
                optical_height = (float) metrics->cap_height;
            else
                optical_height = (float) (metrics->ascent + metrics->descent);
            optical_height /= (float) metrics->design_units_per_em;

            theme->line_height[i] *= optical_height / optical_height_segoe;
        }

        theme->text_logical_size[i] = font_map[i].text_logical_size;
        theme->text_weight[i] = font_map[i].text_weight;
        theme->text_metrics_index[i] = (font_map[i].text_weight == FONT_WEIGHT_REGULAR) ? 0 : 1;

        theme->icon_logical_size[i] = font_map[i].icon_logical_size;
        theme->icon_weight[i] = FONT_WEIGHT_REGULAR;
        theme->icon_metrics_index[i] = 2;
    }

    /* Success */
    fonttheme_array[id].theme = theme;
    *out_id = id;
    return S_OK;
}

void
fonttheme_destroy(FontThemeId id)
{
    dpi_destroy_all_theme_data(id);

    if(fonttheme_array[id].theme != NULL) {
        fe_free(fonttheme_array[id].theme);
        fonttheme_array[id].theme = NULL;
    }
}

void
fonttheme_apply_uisettings(UiSettings* settings)
{
    fontheme_accessibility_scale_factor = settings->text_scale_factor;

    // TODO: update the themes accordingly, and propagate also to
    // DpiData + refresh affected windows.
}


HRESULT
fonttheme_init(void)
{
    FontThemeId theme_id;
    HRESULT hr;

    hr = fonttheme_create_for_locale(fe_locale_name, &theme_id);
    if(FAILED(hr)) {
        XTRACE_HR("fonttheme_create_for_locale()");
        return hr;
    }

    ASSERT(theme_id == 0);

    return S_OK;
}

void
fonttheme_fini(void)
{
    FontThemeId id;

    fonttheme_destroy(0);

    for(id = 0; id < FONTTHEME_MAX_COUNT; id++) {
        if(fonttheme_array[id].theme != NULL) {
            XTRACE_W("Detected leak of FontTheme (%u)", id);
            fonttheme_destroy(id);
        }
    }
}
