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

#include <shellscalingapi.h>

#include "monitor.h"
#include "libex/list.h"


/* A dummy we use when creating a new one fails.
 * We won't block creation of windows because of that. */
static MonitorInfo monitor_dummy = {
    NULL,
    NULL
};


typedef struct MonitorImpl MonitorImpl;
struct MonitorImpl {
    MonitorInfo info;

    unsigned n_refs;
    ListNode node;
};


static List impl_list;


static MonitorImpl*
monitor_create(HMONITOR handle)
{
    MonitorImpl* impl;

    impl = (MonitorImpl*) fe_malloc(sizeof(MonitorImpl));
    if(impl == NULL) {
        XTRACE_E("fe_malloc() failed.");
        return NULL;
    }
    memset(impl, 0, sizeof(MonitorImpl));

    impl->info.handle = handle;
    impl->info.text_rendering_params = text_create_rendering_params(handle);

    list_append(&impl_list, &impl->node);
    impl->n_refs = 1;
    return impl;
}

static void
monitor_destroy(MonitorImpl* impl)
{
    text_destroy_rendering_params(impl->info.text_rendering_params);

    list_remove(&impl_list, &impl->node);
    fe_free(impl);
}

const MonitorInfo*
monitor_acquire(HMONITOR handle)
{
    ListNode* node;
    MonitorImpl* impl;

    if(handle == NULL)
        return NULL;

    LIST_FOR_EACH(&impl_list, node) {
        impl = LIST_DATA(node, MonitorImpl, node);
        if(impl->info.handle == handle) {
            impl->n_refs++;
            return &impl->info;
        }
    }

    impl = monitor_create(handle);
    return (impl != NULL) ? &impl->info : &monitor_dummy;
}

void
monitor_release(const MonitorInfo* mon_info)
{
    MonitorImpl* impl;

    if(mon_info == &monitor_dummy)
        return;

    impl = CONTAINEROF(mon_info, MonitorImpl, info);

    if(--impl->n_refs == 0)
        monitor_destroy(impl);
}


HRESULT
monitor_init(void)
{
    list_init(&impl_list);
    return S_OK;
}

void
monitor_fini(void)
{
    if(!list_is_empty(&impl_list))
        XTRACE_W("Leak of MonitorImpl data detected.");
}
