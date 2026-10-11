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

#ifndef FE_SYSTEM_H
#define FE_SYSTEM_H

#include "misc.h"


typedef struct SystemConfig SystemConfig;
struct SystemConfig {
    NONCLIENTMETRICS nc_metrics;

    /* Light versus dark theme preferences */
    BOOL use_light_theme;

    BOOL use_ui_transparency_effects;

    BOOL optimize_ui_for_left_handed_user;

    /* Text scaling (on top of DPI scaling). Between 1.0 to 2.25 (inclusive). */
    double text_scale_factor;

    /* Default UI font. */
    LOGFONT ui_font;
};

const SystemConfig* system_get_config(void);


HRESULT system_init(void);
void system_fini(void);


#endif  /* FE_SYSTEM_H */
