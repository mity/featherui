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

#ifndef FE_TEXT_H
#define FE_TEXT_H

#include "misc.h"
#include "font.h"


typedef struct TextLayout TextLayout;
typedef struct TextRenderingParams TextRenderingParams;


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

TextLayout* text_create_layout(const WCHAR* str, UINT str_len, Font* font,
            float max_width, float max_height, DWORD flags);
void text_destroy_layout(TextLayout* layout);

TextRenderingParams* text_create_rendering_params(HMONITOR mon_handle);
void text_destroy_rendering_params(TextRenderingParams* params);

HRESULT text_init(void);
void text_fini(void);


#endif  /* FE_TEXT_H */
