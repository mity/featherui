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

#ifndef FE_COLORTHEME_H
#define FE_COLORTHEME_H

#include "misc.h"
#include "gfx.h"


/* We don't want too many active themes at the same time. */
#define COLORTHEME_MAX_COUNT        64



typedef enum ColorThemeShade ColorThemeShade;
enum ColorThemeShade {
    COLOR_THEME_SHADE_STRONGER_3    = 0,
    COLOR_THEME_SHADE_STROMGER_2    = 1,
    COLOR_THEME_SHADE_STRONGER_1    = 2,
    COLOR_THEME_SHADE_NORMAL        = 3,
    COLOR_THEME_SHADE_WEAKER_1      = 4,
    COLOR_THEME_SHADE_WEAKER_2      = 5,
    COLOR_THEME_SHADE_WEAKER_3      = 6
};
#define COLORTHEME_SHADE_COUNT      7


/* Treat pointer to this structure always as transient/temporary one (i.e.
 * never "store it" for re-use.
 *
 * (Actually to mitigate likeliness of such anti-pattern and hard-to-debug
 * bugs, we don't really offer a direct way how to get it and instead we're
 * just propagating to a caller's provided callback.
 *
 * See the function uitheme_paint() below.
 */
typedef struct ColorTheme ColorTheme;
struct ColorTheme {
    GfxColor clr_background[COLORTHEME_SHADE_COUNT];
    GfxColor clr_text[COLORTHEME_SHADE_COUNT];
    GfxColor clr_disabled_text[COLORTHEME_SHADE_COUNT];
    GfxColor clr_stroke[COLORTHEME_SHADE_COUNT];
    GfxColor clr_accent[COLORTHEME_SHADE_COUNT];
};


typedef uint8_t ColorThemeId;

#define COLORTHEME_ID_SYSTEM       0
#define COLORTHEME_ID_LIGHT        1
#define COLORTHEME_ID_DARK         2


extern ColorThemeId colortheme_default_id;


HRESULT colortheme_create(ColorThemeId base_id, ColorThemeId* out_id);
void colortheme_destroy(ColorThemeId id);


/* Called from system.c when it gets notified about change of system theme colors. */
void colortheme_reload_uisettings(void);

HRESULT colortheme_init(void);
void colortheme_fini(void);


#endif  /* FE_UITHEME_H */
