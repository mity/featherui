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

#include "dpi.h"
#include "libex/list.h"


typedef struct DpiImpl DpiImpl;
struct DpiImpl {
    DpiInfo info;
    unsigned n_refs;
    DpiMetrics* metrics[FONTTHEME_MAX_COUNT];
    ListNode node;
};


static List dpi_list;


const DpiMetrics*
dpi_metrics(const DpiInfo* dpi, FontThemeId theme_id)
{
    DpiImpl* impl = CONTAINEROF(dpi, DpiImpl, info);
    DpiMetrics* dpi_metrics = impl->metrics[theme_id];
    const FontTheme* font_theme;
    unsigned i;

    if(dpi_metrics != NULL)
        return dpi_metrics;

    font_theme = fonttheme_get(theme_id);

    dpi_metrics = (DpiMetrics*) fe_malloc(sizeof(DpiMetrics));
    if(dpi_metrics == NULL) {
        XTRACE_E("fe_malloc() failed.");
        return NULL;
    }

    dpi_metrics->dpi_x = dpi->dpi_x;
    dpi_metrics->dpi_y = dpi->dpi_y;
    dpi_metrics->dpi_factor_x = (float)dpi->dpi_x / 96.0;
    dpi_metrics->dpi_factor_y = (float)dpi->dpi_y / 96.0;
    dpi_metrics->text_scale_factor = font_theme->text_scale_factor;

    for(i = 0; i < FONTTHEME_TEXT_TYPE_COUNT; i++) {
        dpi_metrics->text_font[i] = text_create_font_ex(
                font_theme->text_font_name,
                font_theme->font_size[i] * dpi_metrics->dpi_factor_y * dpi_metrics->text_scale_factor,
                font_theme->font_weight[i],
                TEXT_FONT_STYLE_NORMAL,
                TEXT_FONT_STRETCH_NORMAL
        );

        dpi_metrics->icon_font[i] = text_create_font_ex(
                font_theme->icon_font_name,
                font_theme->font_size[i] * dpi_metrics->dpi_factor_y * dpi_metrics->text_scale_factor,
                font_theme->font_weight[i],
                TEXT_FONT_STYLE_NORMAL,
                TEXT_FONT_STRETCH_NORMAL
        );

        // TODO: error handling?

        dpi_metrics->line_height[i] = dpi_metrics->dpi_factor_y * font_theme->line_height[i] * dpi_metrics->text_scale_factor;
    }

    impl->metrics[theme_id] = dpi_metrics;
    return dpi_metrics;
}

static void
dpi_destroy_metrics(DpiMetrics* metrics)
{
    unsigned i;

    for(i = 0; i < FONTTHEME_TEXT_TYPE_COUNT; i++) {
        if(metrics->text_font[i] != NULL)
            text_destroy_font(metrics->text_font[i]);
        if(metrics->icon_font[i] != NULL)
            text_destroy_font(metrics->icon_font[i]);
    }

    fe_free(metrics);
}

const DpiInfo*
dpi_attach(UINT16 dpi_x, UINT16 dpi_y)
{
    ListNode* node;
    DpiImpl* impl;

    LIST_FOR_EACH(&dpi_list, node) {
        impl = LIST_DATA(node, DpiImpl, node);
        if(impl->info.dpi_x == dpi_x  &&  impl->info.dpi_y == dpi_y) {
            impl->n_refs++;
            return &impl->info;
        }
    }

    impl = (DpiImpl*) fe_malloc(sizeof(DpiImpl));
    if(impl == NULL) {
        XTRACE_E("fe_malloc() failed.");
        return NULL;
    }

    impl->info.dpi_x = dpi_x;
    impl->info.dpi_y = dpi_y;
    impl->n_refs = 1;
    memset(impl->metrics, 0, sizeof(impl->metrics));
    list_append(&dpi_list, &impl->node);
    return &impl->info;
}

void
dpi_detach(const DpiInfo* dpi)
{
    DpiImpl* impl = CONTAINEROF(dpi, DpiImpl, info);
    FontThemeId theme_id;

    impl->n_refs--;
    if(impl->n_refs == 0) {
        list_remove(&dpi_list, &impl->node);
        for(theme_id = 0; theme_id < FONTTHEME_MAX_COUNT; theme_id++) {
            if(impl->metrics[theme_id] != NULL)
                dpi_destroy_metrics(impl->metrics[theme_id]);
        }
        fe_free(impl);
    }
}

void
dpi_destroy_all_theme_data(FontThemeId theme_id)
{
    ListNode* node;
    DpiImpl* impl;

    LIST_FOR_EACH(&dpi_list, node) {
        impl = LIST_DATA(node, DpiImpl, node);
        if(impl->metrics[theme_id] != NULL) {
            dpi_destroy_metrics(impl->metrics[theme_id]);
            impl->metrics[theme_id] = NULL;
        }
    }
}

HRESULT
dpi_init(void)
{
    list_init(&dpi_list);
    return S_OK;
}

void
dpi_fini(void)
{
    while(!list_is_empty(&dpi_list)) {
        ListNode* head = list_head(&dpi_list);
        DpiImpl* impl = LIST_DATA(head, DpiImpl, node);
        unsigned i;

        list_remove(&dpi_list, head);

        for(i = 0; i < FONTTHEME_MAX_COUNT; i++) {
            if(impl->metrics[i] != NULL)
                dpi_destroy_metrics(impl->metrics[i]);
        }
        fe_free(impl);
    }
}
