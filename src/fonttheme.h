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

#ifndef FE_FONTTHEME_H
#define FE_FONTTHEME_H

#include "misc.h"


/* We don't want too many active themes at the same time. */
#define FONTTHEME_MAX_COUNT         64


/* Names correspond to WinUI 3 */
typedef enum FontThemeTextType FontThemeTextType;
enum FontThemeTextType {
    FONTTHEME_TEXT_CAPTION          = 0,
    FONTTHEME_TEXT_BODY             = 1,
    FONTTHEME_TEXT_BODYSTRONG       = 2,
    FONTTHEME_TEXT_BODYLARGE        = 3,
    FONTTHEME_TEXT_BODYLARGESTRONG  = 4,
    FONTTHEME_TEXT_SUBTITLE         = 5,
    FONTTHEME_TEXT_TITLE            = 6,
    FONTTHEME_TEXT_TITLELARGE       = 7,
    FONTTHEME_TEXT_DISPLAY          = 8,

    FONTTHEME_TEXT_TYPE_COUNT
};


typedef uint8_t FontThemeId;


/* Font theme id for new top-level windows.
 * (Controls inherit their parent id.) */
extern FontThemeId fonttheme_default_id;


typedef struct FontTheme FontTheme;
struct FontTheme {
    const WCHAR* locale_name;

    const WCHAR* text_font_name;
    const WCHAR* icon_font_name;

    float font_weight[FONTTHEME_TEXT_TYPE_COUNT];
    float font_size[FONTTHEME_TEXT_TYPE_COUNT];
    float line_height[FONTTHEME_TEXT_TYPE_COUNT];
};


const FontTheme* fonttheme_get(FontThemeId id);

HRESULT fonttheme_create(FontThemeId base_id, FontThemeId* out_id);
void fonttheme_destroy(FontThemeId id);

HRESULT fonttheme_init(void);
void fonttheme_fini(void);


#endif  /* FE_FONTTHEME_H */
