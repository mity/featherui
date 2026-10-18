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

#ifndef GFX_INTERNAL_H
#define GFX_INTERNAL_H

#include <dxgi.h>
#include <dxgi1_2.h>
#include <dxgi1_3.h>
#include <d3d11.h>

#if 0
    /* In a sane world, this would work and I wound't loose half of the day
     * by tearing my hairs out. */
    #include <d2d1.h>
    #include <d2d1_1.h>
    #include <dwrite.h>
#else
    #ifdef __GNUC__
        #pragma GCC diagnostic ignored "-Wunknown-pragmas"
        #define WIN_NOEXCEPT
    #endif

    #include "c_d2d_dwrite/cdwrite.h"
    #include "c_d2d_dwrite/cd2d.h"

    #ifdef __GNUC__
        #pragma GCC diagnostic pop
    #endif

    /* This seems missing in these headers. */
    typedef UINT64 D2D1_TAG;
#endif


#endif  /* GFX_INTERNAL_H */
