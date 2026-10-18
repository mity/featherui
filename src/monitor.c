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
monitor_attach(HMONITOR handle)
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
monitor_detach(const MonitorInfo* mon_info)
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
