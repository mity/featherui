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

#ifndef FE_HSTRING_REF_H
#define FE_HSTRING_REF_H

#include <winstring.h>

#include "misc.h"


/* This provides a moral replacement for WindowsCreateStringReference() so
 * that we don't have to introduce a dependency on the WinRT runtime just for
 * that function.
 *
 * Note that the internal layout isn't officially documented by Microsoft,
 * but I think it's one of those things that have become a de facto standard,
 * having spread across language implementations with different release
 * cycles.
 *
 * Use:
 *
 * ```
 * {
 *     HstringRef my_ref;  // lives on the stack
 *     HSTRING hstr = hstring_ref(&my_ref, L"Hello");
 *
 *     ... // use hstr
 * }
 * ```
 *
 * The HSTRING reference's lifetime is limited by the scope of the block
 * and caller is responsible for keeping the pointed str buffer for the time.
 *
 * Note: Although `str` can legally contain zero characters in its contents,
 * the caller is still responsible for making sure there is a zero terminator
 * at `str[len]`.
 */


typedef struct HstringRef HstringRef;
struct HstringRef {
    uint32_t flags;
    uint32_t len;
    uint32_t padding1;
    uint32_t padding2;
    const WCHAR* str;
};

static inline HSTRING
hstring_ref_len(HstringRef* ref, const WCHAR* str, UINT len)
{
    ref->flags = 0x1;   /* Mark us as a transient reference */
    ref->len = len;
    ref->padding1 = 0;
    ref->padding2 = 0;
    ref->str = str;
    return (HSTRING) ref;
}

static inline HSTRING
hstring_ref(HstringRef* ref, const WCHAR* str)
{
    return hstring_ref_len(ref, str, wcslen(str));
}


#endif  /* FE_HSTRING_REF_H */
