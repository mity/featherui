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

#ifndef FE_UITHEME_H
#define FE_UITHEME_H

#include "misc.h"
#include "gfx.h"


typedef enum UiThemeState UiThemeState;
enum UiThemeState {
    UI_THEME_STATE_NORMAL = 0,
    UI_THEME_STATE_HOT = 1,
    UI_THEME_STATE_PRESSED = 2,
    UI_THEME_STATE_DISABLED = 3
};
#define UI_THEME_STATE_COUNT        4


typedef enum UiThemeShade UiThemeShade;
enum UiThemeShade {
    UI_THEME_SHADE_STRONGER_3 = 0,
    UI_THEME_SHADE_STROMGER_2 = 1,
    UI_THEME_SHADE_STRONGER_1 = 2,
    UI_THEME_SHADE_NORMAL = 3,
    UI_THEME_SHADE_WEAKER_1 = 4,
    UI_THEME_SHADE_WEAKER_2 = 5,
    UI_THEME_SHADE_WEAKER_3 = 6
};
#define UI_THEME_SHADE_COUNT        7


/* Treat pointer to this structure always as transient/temporary one (i.e.
 * never "store it" for re-use.
 *
 * (Actually to mitigate likeliness of such anti-pattern and hard-to-debug
 * bugs, we don't really offer a direct way how to get it and instead we're
 * just propagating to a caller's provided callback.
 *
 * See the function uitheme_paint() below.
 */
typedef struct UiTheme UiTheme;
struct UiTheme {
    GfxColor clr_text[UI_THEME_SHADE_COUNT][UI_THEME_STATE_COUNT];
    GfxColor clr_stroke[UI_THEME_SHADE_COUNT][UI_THEME_STATE_COUNT];
    GfxColor clr_background[UI_THEME_SHADE_COUNT][UI_THEME_STATE_COUNT];
    GfxColor clr_accent[UI_THEME_SHADE_COUNT][UI_THEME_STATE_COUNT];

    // TODO: fonts

    // TODO: control/text metrics

    // TODO: animations on/off
    // TODO: animated ui transition duration
};


typedef uint8_t UiThemeId;

#define UITHEME_ID_DEFAULT      0
#define UITHEME_ID_SYSTEM       1
#define UITHEME_ID_LIGHT        2
#define UITHEME_ID_DARK         3


/* Calls the provided function to which "an object", "extra_args", and
 * the theme data propagate. Caller should never store the pointer to
 * the UiTheme structure as it's transient and can be anytime after return
 * from the callback re-allocated if needed.
 */
HRESULT uitheme_paint(const void* object, UiThemeId theme_id, const void* extra_arg,
        HRESULT (*callback_func)(const void* /*object*/, const UiTheme* /*theme*/, const void* /*extra_arg*/));


HRESULT uitheme_create(UiThemeId parent_id, UiThemeId* out_id);
void uitheme_destroy(UiThemeId id);


HRESULT uitheme_init(void);
void uitheme_fini(void);


#endif  /* FE_UITHEME_H */
