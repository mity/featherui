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

#ifndef FE_MONITOR_H
#define FE_MONITOR_H

#include "misc.h"
#include "text.h"


/* This describes a monitor and holds some per-monitor data.
 */

typedef struct MonitorInfo MonitorInfo;
struct MonitorInfo {
    HMONITOR handle;
    TextRenderingParams* text_rendering_params;
};


const MonitorInfo* monitor_attach(HMONITOR handle);
void monitor_detach(const MonitorInfo* mon_info);


HRESULT monitor_init(void);
void monitor_fini(void);


#endif  /* FE_MONITOR_H */
