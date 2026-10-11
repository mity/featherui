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

#ifndef FE_TEXT_H
#define FE_TEXT_H

#include "misc.h"


typedef struct TextFont TextFont;
typedef struct TextLayout TextLayout;
typedef struct TextRenderingParams TextRenderingParams;


typedef enum TextFontWeight TextFontWeight;
enum TextFontWeight {
    TEXT_FONT_WEIGHT_THIN = 100,
    TEXT_FONT_WEIGHT_EXTRA_LIGHT = 200,
    TEXT_FONT_WEIGHT_ULTRA_LIGHT = 200,
    TEXT_FONT_WEIGHT_LIGHT = 300,
    TEXT_FONT_WEIGHT_SEMI_LIGHT = 350,
    TEXT_FONT_WEIGHT_NORMAL = 400,
    TEXT_FONT_WEIGHT_REGULAR = 400,
    TEXT_FONT_WEIGHT_MEDIUM = 500,
    TEXT_FONT_WEIGHT_DEMI_BOLD = 600,
    TEXT_FONT_WEIGHT_SEMI_BOLD = 600,
    TEXT_FONT_WEIGHT_BOLD = 700,
    TEXT_FONT_WEIGHT_EXTRA_BOLD = 800,
    TEXT_FONT_WEIGHT_ULTRA_BOLD = 800,
    TEXT_FONT_WEIGHT_BLACK = 900,
    TEXT_FONT_WEIGHT_HEAVY = 900,
    TEXT_FONT_WEIGHT_EXTRA_BLACK = 950,
    TEXT_FONT_WEIGHT_ULTRA_BLACK = 950
};

typedef enum TextFontStyle TextFontStyle;
enum TextFontStyle {
    TEXT_FONT_STYLE_NORMAL = 0,
    TEXT_FONT_STYLE_OBLIQUE = 1,
    TEXT_FONT_STYLE_ITALIC = 2
};

typedef enum TextFontStretch TextFontStretch;
enum TextFontStretch {
    TEXT_FONT_STRETCH_UNDEFINED = 0,
    TEXT_FONT_STRETCH_ULTRA_CONDENSED = 1,
    TEXT_FONT_STRETCH_EXTRA_CONDENSED = 2,
    TEXT_FONT_STRETCH_CONDENSED = 3,
    TEXT_FONT_STRETCH_SEMI_CONDENSED = 4,
    TEXT_FONT_STRETCH_NORMAL = 5,
    TEXT_FONT_STRETCH_MEDIUM = 5,
    TEXT_FONT_STRETCH_SEMI_EXPANDED = 6,
    TEXT_FONT_STRETCH_EXPANDED = 7,
    TEXT_FONT_STRETCH_EXTRA_EXPANDED = 8,
    TEXT_FONT_STRETCH_ULTRA_EXPANDED = 9
};


TextFont* text_create_font(const WCHAR* name, float size);
TextFont* text_create_font_ex(const WCHAR* name, float size,
            TextFontWeight weight, TextFontStyle style, TextFontStretch stretch);
void text_destroy_font(TextFont* font);


#define TEXT_ALIGN_LEAD         0x0000
#define TEXT_ALIGN_CENTER       0x0001
#define TEXT_ALIGN_TRAIL        0x0002
#define TEXT_ALIGN_JUSTIFY      0x0003
#define TEXT_VALIGN_TOP         0x0000
#define TEXT_VALIGN_CENTER      0x0004
#define TEXT_VALIGN_BOTTOM      0x0008
#define TEXT_TRIM_NONE          0x0000
#define TEXT_TRIM_END           0x0010
#define TEXT_TRIM_WORD          0x0020
#define TEXT_TRIM_PATH          0x0040
#define TEXT_NOWRAP             0x0100

#define TEXT_ALIGN_MASK         (TEXT_ALIGN_LEAD | TEXT_ALIGN_CENTER | TEXT_ALIGN_TRAIL | TEXT_ALIGN_JUSTIFY)
#define TEXT_VALIGN_MASK        (TEXT_VALIGN_TOP | TEXT_VALIGN_CENTER | TEXT_VALIGN_BOTTOM)
#define TEXT_TRIM_MASK          (TEXT_TRIM_NONE | TEXT_TRIM_END | TEXT_TRIM_WORD | TEXT_TRIM_PATH)

TextLayout* text_create_layout(const WCHAR* str, UINT str_len, TextFont* font,
            float max_width, float max_height, DWORD flags);
void text_destroy_layout(TextLayout* layout);


TextRenderingParams* text_create_rendering_params(HMONITOR mon_handle);
void text_destroy_rendering_params(TextRenderingParams* params);


HRESULT text_init(void);
void text_fini(void);


#endif  /* FE_TEXT_H */
