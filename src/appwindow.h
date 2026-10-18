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

#ifndef FE_APPWINDOW_H
#define FE_APPWINDOW_H

#include "misc.h"


HRESULT appwindow_init(void);
void appwindow_fini(void);

unsigned appwindow_get_count(void);


#endif  /* FE_APPWINDOW_H */

