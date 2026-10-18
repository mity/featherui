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

#include "colortheme.h"
#include "uisettings.h"


/* Few notes and invariants:
 *
 * - LIGHT and DARK themes are actual "roots" and never inherit from anyone,
 *   instead system settings populate those. We guarantee their contents is
 *   always reasonably and completely initialized.
 *
 * - SYSTEM theme "inherits" from one of those (and never resets its contents
 *   to anything diverting from it), depending on whether light or dark theme
 *   is set in the user's preferences of the system.
 *
 * - In all other respects LIGHT, DARK, and SYSTEM are read-only
 *   and guaranteed to always exist from the API's perspective.
 *
 * - Custom (i.e. in run-time created themes) set their parent during their
 *   creation, and they then can only be re-parented automatically to another
 *   ancestor in the inheritance chain if/when the parent theme is destroyed.
 *
 * - The contents of custom-made can changed via API as the application wishes.
 *
 * - If application (for a custom theme) explicitly initialized only some of
 *   its attributes, other ones are either inherited from parent (if no shade
 *   of the given color has been set and there is no "obvious nonsense" as
 *   as e.g. too low contrast between text and background), or (in other cases
 *   guessed in order to provide contrast between foreground and background
 *   and to interpolate/extrapolate over the shades.)
 */


ColorThemeId colortheme_default_id;


typedef struct ColorThemeImpl ColorThemeImpl;
struct ColorThemeImpl {
    ColorTheme theme;

    /* Shade ID determines bit.
     * Bit set: The color has been explicitly set.
     * Bit unset: The actual color has been either deduced from other colors
     * and/or shades; or, if we were unable to do that, inherited from base
     * theme .
     */
    uint8_t clr_background_is_explicit[COLORTHEME_SHADE_COUNT];
    uint8_t clr_text_is_explicit[COLORTHEME_SHADE_COUNT];
    uint8_t clr_disabled_text_is_explicit[COLORTHEME_SHADE_COUNT];
    uint8_t clr_stroke_is_explicit[COLORTHEME_SHADE_COUNT];
    uint8_t clr_accent_is_explicit[COLORTHEME_SHADE_COUNT];
};


static struct {
    ColorThemeImpl* impl;
    ColorThemeId base_id;
    BOOL is_readonly;
} colortheme_array[COLORTHEME_MAX_COUNT];


static ColorThemeImpl*
colortheme_alloc(ColorThemeImpl* base_impl)
{
    ColorThemeImpl* impl;

    impl = (ColorThemeImpl*) fe_malloc(sizeof(ColorThemeImpl));
    if(impl == NULL) {
        XTRACE_E("fe_malloc() failed.");
        return NULL;
    }

    if(base_impl != NULL)
        memcpy(impl, base_impl, sizeof(ColorThemeImpl));
    else
        memset(impl, 0, sizeof(ColorThemeImpl));

    return impl;
}

HRESULT
colortheme_create(ColorThemeId base_id, ColorThemeId* out_id)
{
    ColorThemeId id;

    for(id = 0; id < COLORTHEME_MAX_COUNT; id++) {
        if(colortheme_array[id].impl == NULL)
            break;
    }

    if(id >= COLORTHEME_MAX_COUNT) {
        XTRACE_E("Too many themes.");
        return E_FAIL;
    }

    colortheme_array[id].impl = colortheme_alloc(colortheme_array[base_id].impl);
    if(colortheme_array[id].impl == NULL) {
        XTRACE_E("colortheme_alloc() failed.");
        return HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
    }
    colortheme_array[id].base_id = base_id;

    *out_id = id;
    return S_OK;
}

void
colortheme_destroy(ColorThemeId id)
{
    if(colortheme_array[id].impl != NULL) {
        fe_free(colortheme_array[(unsigned) id].impl);
        colortheme_array[(unsigned) id].impl = NULL;
    }
}


void
colortheme_reload_uisettings(void)
{
    UiSettings ui_settings;
    HRESULT hr = E_FAIL;

    hr = uisettings_load(&ui_settings);
    if(FAILED(hr)) {
        XTRACE_HR("uisettings_load() failed. Will continue using the old settings.");
        return;
    }

    if(ui_settings.use_light_theme) {
        colortheme_array[COLORTHEME_ID_SYSTEM].impl = colortheme_array[COLORTHEME_ID_LIGHT].impl;
        colortheme_array[COLORTHEME_ID_SYSTEM].base_id = COLORTHEME_ID_LIGHT;
    } else {
        colortheme_array[COLORTHEME_ID_SYSTEM].impl = colortheme_array[COLORTHEME_ID_DARK].impl;
        colortheme_array[COLORTHEME_ID_SYSTEM].base_id = COLORTHEME_ID_DARK;
    }

    // TODO: apply other stuff too (mainly dark/light theme colors).

    // TODO: walk over themes and propagate the changes via the inheritance chains.

    // TODO: kick into all the stuff to repaint.
}

HRESULT
colortheme_init(void)
{
    ColorThemeId id;
    UiSettings ui_settings;
    HRESULT hr = E_FAIL;

    for(id = 0; id < COLORTHEME_MAX_COUNT; id++)
        colortheme_array[id].impl = NULL;

    hr = uisettings_load(&ui_settings);
    if(FAILED(hr)) {
        XTRACE_HR("uisettings_load() failed.");
        return hr;
    }

    colortheme_array[COLORTHEME_ID_LIGHT].impl = colortheme_alloc(NULL);
    colortheme_array[COLORTHEME_ID_LIGHT].base_id = COLORTHEME_ID_LIGHT;
    colortheme_array[COLORTHEME_ID_LIGHT].is_readonly = TRUE;
    colortheme_array[COLORTHEME_ID_DARK].impl = colortheme_alloc(NULL);
    colortheme_array[COLORTHEME_ID_DARK].base_id = COLORTHEME_ID_DARK;
    colortheme_array[COLORTHEME_ID_DARK].is_readonly = TRUE;

    if(colortheme_array[COLORTHEME_ID_LIGHT].impl == NULL  ||
       colortheme_array[COLORTHEME_ID_DARK].impl == NULL) {
        XTRACE_E("colortheme_alloc() failed.");
        colortheme_fini();
        return HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
    }

    // TODO: populate light and dark theme.

    // TODO: mark everything in them as "explicitly set".

    if(ui_settings.use_light_theme) {
        colortheme_array[COLORTHEME_ID_SYSTEM].impl = colortheme_array[COLORTHEME_ID_LIGHT].impl;
        colortheme_array[COLORTHEME_ID_SYSTEM].base_id = COLORTHEME_ID_LIGHT;
    } else {
        colortheme_array[COLORTHEME_ID_SYSTEM].impl = colortheme_array[COLORTHEME_ID_DARK].impl;
        colortheme_array[COLORTHEME_ID_SYSTEM].base_id = COLORTHEME_ID_DARK;
    }
    colortheme_array[COLORTHEME_ID_SYSTEM].is_readonly = TRUE;

    colortheme_default_id = COLORTHEME_ID_SYSTEM;

    return S_OK;
}

void
colortheme_fini(void)
{
    ColorThemeId id;

    colortheme_array[COLORTHEME_ID_SYSTEM].impl = NULL;
    colortheme_destroy(COLORTHEME_ID_LIGHT);
    colortheme_destroy(COLORTHEME_ID_DARK);

    for(id = 0; id < COLORTHEME_MAX_COUNT; id++) {
        if(colortheme_array[id].impl != NULL) {
            XTRACE_W("Detected leak of ColorTheme (%u).", id);
            colortheme_destroy(id);
        }
    }
}
