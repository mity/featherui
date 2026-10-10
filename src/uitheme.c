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

#include "uitheme.h"
#include "system.h"


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
 * - DEFAULT is another pseudo-theme which initially (after feInitialize())
 *   inherits from SYSTEM. It can (and actually it is its purpose) can be
 *   re-parented. Unlike SYSTEM, the re-parenting has no limitations and is
 *   always only initiated by a call from application.
 *
 * - In all other respects LIGHT, DARK, SYSTEM and DEFAULT are read-only
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

typedef struct UiThemeData UiThemeData;
struct UiThemeData {
    UiTheme theme;
    unsigned n_refs;

    /* Shade ID determines bit.
     * Bit set: The actual attribute has been "guessed" from other values
     * and will need to be guessed again when the set of explicitly known
     * changes. */
    uint8_t clr_text_guessed[UI_THEME_STATE_COUNT];
    uint8_t clr_stroke_guessed[UI_THEME_STATE_COUNT];
    uint8_t clr_background_guessed[UI_THEME_STATE_COUNT];
    uint8_t clr_accent_guessed[UI_THEME_STATE_COUNT];

    /* Shade ID determines bit.
     * Bit set: The actual attribute needs to be "guessed" before we let anyone
     * see the UiTheme. (This is to prevent multiple cascaded guesses when
     * somebody is setting multiple attributes.) */
    uint8_t clr_text_need_guess[UI_THEME_STATE_COUNT];
    uint8_t clr_stroke_need_guess[UI_THEME_STATE_COUNT];
    uint8_t clr_background_need_guess[UI_THEME_STATE_COUNT];
    uint8_t clr_accent_need_guess[UI_THEME_STATE_COUNT];

    /* Invariant: This count set bits in the xxx_need_guess, so we can check
     * whether we need to populate something by guessing or whether we're ready
     * to go. */
    unsigned n_need_guess;
};

typedef struct UiThemeImpl UiThemeImpl;
struct UiThemeImpl {
    /* Create lazily when it diverges from inherits_from
     * Also used for "aliasing" of UITHEME_ID_DEFAULT and UITHEME_ID_SYSTEM. */
    UiThemeData* theme_data;

    union {
        UiThemeId inherits_from;
        UiThemeId next_free;
    };

    /* Shade ID determines bit.
     * Bit set: the actual attribute is explicitly set and won't be guessed
     * or inherited. */
    uint8_t clr_text_is_explicit[UI_THEME_STATE_COUNT];
    uint8_t clr_stroke_is_explicit[UI_THEME_STATE_COUNT];
    uint8_t clr_background_is_explicit[UI_THEME_STATE_COUNT];
    uint8_t clr_accent_is_explicit[UI_THEME_STATE_COUNT];
};


/* We don't want to be too many themes active at the same time.
 * Lets limit the count to 100, even that is more than generous. */

#define MAX_THEMES      100

static UiThemeImpl* impl_array = NULL;
static UiThemeId impl_alloc = 0;
static UiThemeId impl_count = 0;
static UiThemeId impl_free_list = 0; /* [0] is never free so we can reuse as invalid here. */


HRESULT
uitheme_paint(const void* object, UiThemeId theme_id, const void* extra_arg,
                HRESULT (*callback_func)(const void* /*object*/,
                            const UiTheme* /*theme*/, const void* /*extra_arg*/))
{
    if(impl_array[theme_id].theme_data->n_need_guess != 0) {
        // TODO: do all the required guessing.

        impl_array[theme_id].theme_data->n_need_guess = 0;
    }

    return callback_func(object, &impl_array[theme_id].theme_data->theme, extra_arg);
}


HRESULT
uitheme_create(UiThemeId parent_id, UiThemeId* out_id)
{
    UiThemeId id;
    HRESULT hr = E_FAIL;

    if(impl_free_list != 0  &&  impl_count >= MAX_THEMES) {
        hr = HRESULT_FROM_WIN32(ERROR_TOO_MANY_OPEN_FILES); // FIXME: not really good err code
        XTRACE_HR("Max. quota of active themes (" STRINGIZE(MAX_THEMES) ") has been reached.");
        goto err;
    }

    if(impl_free_list != 0) {
        id = impl_free_list;
        impl_free_list = impl_array[impl_free_list].next_free;
    } else {
        if(impl_count >= impl_alloc) {
            UiThemeImpl* new_array;
            UiThemeId new_alloc;

            /* We don't need exponential growth here. */
            new_alloc = impl_alloc + 5;
            new_array = (UiThemeImpl*) fe_realloc(impl_array, new_alloc * sizeof(UiThemeImpl));
            if(new_array == NULL) {
                XTRACE_E("fe_realloc()");
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto err;
            }

            impl_array = new_array;
            impl_alloc = new_alloc;
        }

        id = impl_count++;
    }

    if(id == UITHEME_ID_LIGHT || id == UITHEME_ID_DARK) {
        /* Part of bootstrapping. We need to mark everything for DARK and LIGHT
         * themes marked as set explicitly so that those as roots never try to
         * reach to some parent, as they don't have one really. */
        memset(&impl_array[id], 0xff, sizeof(UiThemeImpl));
        impl_array[id].inherits_from = id;
        impl_array[id].theme_data = NULL;
    } else {
        memset(&impl_array[id], 0, sizeof(UiThemeImpl));
    }

    if(!(id == UITHEME_ID_LIGHT || id == UITHEME_ID_DARK ||
            id == UITHEME_ID_SYSTEM || id == UITHEME_ID_DEFAULT))
    {
        impl_array[id].theme_data = impl_array[parent_id].theme_data;
        impl_array[id].theme_data->n_refs++;
    }

    *out_id = id;
    return S_OK;

err:
    return hr;
}

void
uitheme_destroy(UiThemeId id)
{
    /* Forbid destruction of our built-in themes. We destroy them in uitheme_fini(). */
    switch(id) {
        case UITHEME_ID_DEFAULT:
        case UITHEME_ID_SYSTEM:
        case UITHEME_ID_LIGHT:
        case UITHEME_ID_DARK:
            XTRACE_W("An attempt to destroy built-in theme.");
            return;
    }

    if(--impl_array[id].theme_data->n_refs == 0) {
        fe_free(impl_array[id].theme_data);
        impl_array[id].theme_data = NULL;
    }

    impl_array[id].next_free = impl_free_list;
    impl_free_list = id;
}


HRESULT
uitheme_init(void)
{
    const SystemConfig* sys_config;
    UiThemeId ids[4];
    UiThemeId i;
    UiThemeId system_ref;
    HRESULT hr = E_FAIL;

    sys_config = system_get_config();

    /* Bootstrapping is little bit tricky here: The built-in themes are
     * not ref. counted and handle inheritance specially. */
    for(i = 0; i < SIZEOF_ARRAY(ids); i++) {
        hr = uitheme_create(0, &ids[i]);
        if(FAILED(hr)) {
            XTRACE_HR("uitheme_create()");
            uitheme_fini();
            return hr;
        }
    }

    /* Populate UITHEME_ID_LIGHT & UITHEME_ID_DARK */
    impl_array[UITHEME_ID_LIGHT].theme_data = (UiThemeData*) fe_malloc(sizeof(UiThemeData));
    impl_array[UITHEME_ID_DARK].theme_data = (UiThemeData*) fe_malloc(sizeof(UiThemeData));
    if(impl_array[UITHEME_ID_LIGHT].theme_data == NULL  ||
       impl_array[UITHEME_ID_DARK].theme_data == NULL)
    {
        XTRACE_E("fe_malloc()");
        hr = HRESULT_FROM_WIN32(GetLastError());
        uitheme_fini();
        return hr;
    }
    // TODO: populate them from config (or on any failure with some sensible hard-coded values).

    /* Connect UITHEME_ID_SYSTEM and UITHEME_ID_DEFAULT as sensible aliases */
    system_ref = (sys_config->use_light_theme ? UITHEME_ID_LIGHT : UITHEME_ID_DARK);
    impl_array[UITHEME_ID_SYSTEM].inherits_from = system_ref;
    impl_array[UITHEME_ID_SYSTEM].theme_data = impl_array[system_ref].theme_data;
    impl_array[UITHEME_ID_DEFAULT].inherits_from = UITHEME_ID_SYSTEM;
    impl_array[UITHEME_ID_DEFAULT].theme_data = impl_array[UITHEME_ID_SYSTEM].theme_data;

    return S_OK;
}

void
uitheme_fini(void)
{
    if(impl_array != NULL) {
        if(impl_array[UITHEME_ID_LIGHT].theme_data != NULL)
            fe_free(impl_array[UITHEME_ID_LIGHT].theme_data);
        if(impl_array[UITHEME_ID_DARK].theme_data != NULL)
            fe_free(impl_array[UITHEME_ID_DARK].theme_data);

        fe_free(impl_array);
    }

    impl_array = NULL;
    impl_alloc = 0;
    impl_count = 0;
    impl_free_list = 0;
}
