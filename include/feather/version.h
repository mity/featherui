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
