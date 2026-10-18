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
