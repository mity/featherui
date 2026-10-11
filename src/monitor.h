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


const MonitorInfo* monitor_acquire(HMONITOR handle);
void monitor_release(const MonitorInfo* mon_info);


HRESULT monitor_init(void);
void monitor_fini(void);


#endif  /* FE_MONITOR_H */
