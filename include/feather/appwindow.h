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

#ifndef FEATHER_APPWINDOW_H
#define FEATHER_APPWINDOW_H

#include <feather/_internal.h>

#ifdef __cplusplus
extern "C" {
#endif


/* TODO: This is just temporary hack, it will need more flexible interface. */
HRESULT FEATHER_API feAppWindow_Create(HWND* pHwnd);


#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* FEATHER_APPWINDOW_H */
