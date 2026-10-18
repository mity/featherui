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

#ifndef FEATHER_VERSION_H
#define FEATHER_VERSION_H

#include <feather/_internal.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct FeVersion FeVersion;
struct FeVersion {
    DWORD dwMajor;
    DWORD dwMinor;
    DWORD dwPatch;
};


void FEATHER_API feVersion(FeVersion* lpVersion);
const WCHAR* FEATHER_API feVersionStr(void);


#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* FEATHER_VERSION_H */
