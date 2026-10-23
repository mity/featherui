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

#ifndef FE_FONT_H
#define FE_FONT_H

#include "misc.h"


typedef enum FontWeight FontWeight;
enum FontWeight {
    FONT_WEIGHT_THIN        = 100,
    FONT_WEIGHT_EXTRA_LIGHT = 200, FONT_WEIGHT_ULTRA_LIGHT = 200,
    FONT_WEIGHT_LIGHT       = 300,
    FONT_WEIGHT_SEMI_LIGHT  = 350,
    FONT_WEIGHT_NORMAL      = 400, FONT_WEIGHT_REGULAR     = 400,
    FONT_WEIGHT_MEDIUM      = 500,
    FONT_WEIGHT_DEMI_BOLD   = 600, FONT_WEIGHT_SEMI_BOLD   = 600,
    FONT_WEIGHT_BOLD        = 700,
    FONT_WEIGHT_EXTRA_BOLD  = 800, FONT_WEIGHT_ULTRA_BOLD  = 800,
    FONT_WEIGHT_BLACK       = 900, FONT_WEIGHT_HEAVY       = 900,
    FONT_WEIGHT_EXTRA_BLACK = 950, FONT_WEIGHT_ULTRA_BLACK = 950
};

typedef enum FontStyle FontStyle;
enum FontStyle {
    FONT_STYLE_NORMAL   = 0,
    FONT_STYLE_OBLIQUE  = 1,
    FONT_STYLE_ITALIC   = 2
};

typedef enum FontStretch FontStretch;
enum FontStretch {
    FONT_STRETCH_UNDEFINED          = 0,    /* not all fonts support stretch. */
    FONT_STRETCH_ULTRA_CONDENSED    = 1,
    FONT_STRETCH_EXTRA_CONDENSED    = 2,
    FONT_STRETCH_CONDENSED          = 3,
    FONT_STRETCH_SEMI_CONDENSED     = 4,
    FONT_STRETCH_NORMAL             = 5, FONT_STRETCH_MEDIUM             = 5,
    FONT_STRETCH_SEMI_EXPANDED      = 6,
    FONT_STRETCH_EXPANDED           = 7,
    FONT_STRETCH_EXTRA_EXPANDED     = 8,
    FONT_STRETCH_ULTRA_EXPANDED     = 9
};

#define FONT_PARAMS_LOGICAL_SIZE    0x0001
#define FONT_PARAMS_WEIGHT          0x0002
#define FONT_PARAMS_STYLE           0x0004
#define FONT_PARAMS_STRETCH         0x0008

typedef struct FontParams FontParams;
struct FontParams {
    DWORD optional_mask;    /* Bitmask of FONT_PARAMS_xxx; describes what below is explicitly set. */

    const WCHAR* locale_name;
    const WCHAR* name;
    float size;             /* scaled by DPI and accessibility factor */

    /* These are optional; and used only if optional_mask enables them. */
    float logical_size;     /* "unscaled" font size */
    FontWeight weight;
    FontStyle style;
    FontStretch stretch;

    BOOL disable_optical_sizing;
    BOOL disable_font_fallback;
};


typedef struct Font Font;

Font* font_create(const FontParams* params);
void font_destroy(Font* font);


typedef struct FontMetrics FontMetrics;
struct FontMetrics {
    UINT16 design_units_per_em;
    UINT16 ascent;
    UINT16 descent;
    INT16 line_gap;
    UINT16 cap_height;
    UINT16 x_height;
};

HRESULT font_get_metrics(const FontParams* params, FontMetrics* metrics);


#endif  /* FE_FONT_H */
