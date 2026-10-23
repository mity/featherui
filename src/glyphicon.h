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

#ifndef FE_GLYPHICON_H
#define FE_GLYPHICON_H

#include "misc.h"


typedef UINT16 GlyphIconId;


GlyphIconId glyphicon_get(UINT32 codepoint);

float glyphicon_preferred_size(float size);

HRESULT glyphicon_init(void);
void glyphicon_fini(void);


#endif  /* FE_GLYPHICON_H */
