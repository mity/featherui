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

#ifndef GFX_INTERNAL_H
#define GFX_INTERNAL_H


/* Hula loops for C-compatible Direct2D and DirectWrite interfaces.
 *
 * Support in mingw-w64 is problematic [^1].
 * And Microsoft SDK seem to have drop C support in these headers ages ago.
 *
 * [^1]: https://sourceforge.net/p/mingw-w64/bugs/1040/
 */

#ifndef HELLO_FROM_GFX_C
    #error Hey, this is _my_ stuff. Use "gfx.h" instead.
#endif

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
