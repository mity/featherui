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

#include "uimetrics.h"
#include "libex/list.h"


/* A dummy we use when creating a new one fails.
 * We won't block creation of windows because of that. */
static const UiMetrics uimetrics_dummy = {
    96,
    96
};


typedef struct UiMetricsImpl UiMetricsImpl;
struct UiMetricsImpl {
    UiMetrics metrics;

    unsigned n_refs;
    ListNode node;
};


static List impl_list;


static UiMetricsImpl*
uimetrics_create(uint16_t dpi_x, uint16_t dpi_y)
{
    UiMetricsImpl* impl;

    impl = (UiMetricsImpl*) fe_malloc(sizeof(UiMetricsImpl));
    if(impl == NULL) {
        XTRACE_E("fe_malloc() failed.");
        return NULL;
    }
    memset(impl, 0, sizeof(UiMetricsImpl));

    impl->metrics.dpi_x = dpi_x;
    impl->metrics.dpi_y = dpi_y;
    list_append(&impl_list, &impl->node);

    impl->n_refs = 1;
    return impl;
}

static void
uimetrics_destroy(UiMetricsImpl* impl)
{
    list_remove(&impl_list, &impl->node);
    fe_free(impl);
}

const UiMetrics*
uimetrics_acquire(uint16_t dpi_x, uint16_t dpi_y)
{
    ListNode* node;
    UiMetricsImpl* impl;

    LIST_FOR_EACH(&impl_list, node) {
        impl = LIST_DATA(impl, UiMetricsImpl, node);
        if(impl->metrics.dpi_x == dpi_x  &&  impl->metrics.dpi_y == dpi_y) {
            impl->n_refs++;
            return &impl->metrics;
        }
    }

    impl = uimetrics_create(dpi_x, dpi_y);
    return (impl != NULL) ? &impl->metrics : &uimetrics_dummy;
}

void
uimetrics_release(const UiMetrics* metrics)
{
    if(metrics == &uimetrics_dummy)
        return;

    UiMetricsImpl* impl = CONTAINEROF(metrics, UiMetricsImpl, metrics);
    if(--impl->n_refs == 0)
        uimetrics_destroy(impl);
}


HRESULT
uimetrics_init(void)
{
    list_init(&impl_list);
    return S_OK;
}

void
uimetrics_fini(void)
{
    if(!list_is_empty(&impl_list))
        XTRACE_W("Leak of UiMetrics data detected.");
}
