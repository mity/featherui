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

#ifndef FE_UISETTINGS_H
#define FE_UISETTINGS_H

#include "misc.h"


typedef struct UiSettings UiSettings;
struct UiSettings {
    unsigned use_light_theme                : 1;
    unsigned use_transparency_effects       : 1;
    unsigned optimize_for_left_handed_user  : 1;
    double text_scale_factor;   /* Accessibility text scaling (on top of DPI) */
};


HRESULT uisettings_load(UiSettings* settings);


#endif  /* FE_UISETTINGS_H */
