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

#ifndef FE_DPI_H
#define FE_DPI_H

#include "misc.h"
#include "font.h"
#include "fonttheme.h"


typedef struct DpiInfo DpiInfo;
struct DpiInfo {
    UINT16 dpi_x;
    UINT16 dpi_y;
};


typedef struct DpiMetrics DpiMetrics;
struct DpiMetrics {
    UINT16 dpi_x;
    UINT16 dpi_y;

    float dpi_factor_x;
    float dpi_factor_y;
    float accessibility_scale_factor;

    Font* text_font[FONTTHEME_TEXT_TYPE_COUNT];
    Font* icon_font[FONTTHEME_TEXT_TYPE_COUNT];
    float line_height[FONTTHEME_TEXT_TYPE_COUNT];
};



const DpiMetrics* dpi_metrics(const DpiInfo* dpi, FontThemeId theme_id);

const DpiInfo* dpi_attach(UINT16 dpi_x, UINT16 dpi_y);
void dpi_detach(const DpiInfo* dpi);

/* Called from fonttheme_destroy(). */
void dpi_destroy_all_theme_data(FontThemeId theme_id);

HRESULT dpi_init(void);
void dpi_fini(void);


#endif  /* FE_DPI_H */
