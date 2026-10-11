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

#ifndef FE_DPI_H
#define FE_DPI_H

#include "misc.h"
#include "text.h"
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

    TextFont* text_font[FONTTHEME_TEXT_TYPE_COUNT];
    TextFont* icon_font[FONTTHEME_TEXT_TYPE_COUNT];
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
