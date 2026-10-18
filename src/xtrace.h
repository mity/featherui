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

#ifndef XTRACE_H
#define XTRACE_H

#include <stddef.h>

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif


#if defined XTRACE || defined DEBUG
    /* Implementation helpers; do not rely on these. */
    void xtrace_msg__(
                    const char* fname, unsigned line, const char* func,
                    unsigned flags, int errcode, const char* fmt, ...);
    void xtrace_dump__(
                    const char* fname, unsigned line, const char* func,
                    unsigned flags, const char* desc, const void* ptr, size_t size);

    #if defined __GNUC__ || defined __clang__
        #define XTRACE_LOCATION__       __FILE__, __LINE__, __PRETTY_FUNCTION__
    #else
        #define XTRACE_LOCATION__       __FILE__, __LINE__, __FUNCTION__
    #endif
    #define XTRACE_STRINGIZE___(x)      #x
    #define XTRACE_STRINGIZE__(x)       XTRACE_STRINGIZE___(x)


    /**************************
     ***  Public interface  ***
     **************************/

    #define XTRACE_ALWAYS(...)          xtrace_msg__(XTRACE_LOCATION__, 0, 0, __VA_ARGS__)
    #define XTRACE(...)                 xtrace_msg__(XTRACE_LOCATION__, 3, 0, __VA_ARGS__)
    #define XTRACE_W(...)               xtrace_msg__(XTRACE_LOCATION__, 2, 0, __VA_ARGS__)
    #define XTRACE_E(...)               xtrace_msg__(XTRACE_LOCATION__, 1, 0, __VA_ARGS__)

    #define XTRACE_ERRNO_(err, ...)     xtrace_msg__(XTRACE_LOCATION__, 1 | 0x0100, (err), __VA_ARGS__)
    #define XTRACE_ERRNO(...)           xtrace_msg__(XTRACE_LOCATION__, 1 | 0x0100, errno, __VA_ARGS__)

    #define XDUMP(addr, size)           xtrace_dump__(XTRACE_LOCATION__, 0, XTRACE_STRINGIZE__(addr), (const void*)(addr), (size))
    #define XDUMP_W(addr, size)         xtrace_dump__(XTRACE_LOCATION__, 2, XTRACE_STRINGIZE__(addr), (const void*)(addr), (size))
    #define XDUMP_E(addr, size)         xtrace_dump__(XTRACE_LOCATION__, 1, XTRACE_STRINGIZE__(addr), (const void*)(addr), (size))

    /* Platform-specific extensions. */
    #ifdef _WIN32
        #define XTRACE_ERR_(err, ...)   xtrace_msg__(XTRACE_LOCATION__, 1 | 0x1000, (err), __VA_ARGS__)
        #define XTRACE_ERR(...)         xtrace_msg__(XTRACE_LOCATION__, 1 | 0x1000, GetLastError(), __VA_ARGS__)
        #define XTRACE_HR_(hr, ...)     xtrace_msg__(XTRACE_LOCATION__, 1 | 0x2000, (hr), __VA_ARGS__)
        #define XTRACE_HR(...)          xtrace_msg__(XTRACE_LOCATION__, 1 | 0x2000, (hr), __VA_ARGS__)
    #endif
#endif


/* Make sure every single public MACRO supported in the target platform is
 * #defined. */
#ifndef XTRACE_ALWAYS
    #define XTRACE_ALWAYS(...)          do {} while(0)
#endif
#ifndef XTRACE
    #define XTRACE(...)                 do {} while(0)
#endif
#ifndef XTRACE_W
    #define XTRACE_W(...)               do {} while(0)
#endif
#ifndef XTRACE_E
    #define XTRACE_E(...)               do {} while(0)
#endif
#ifndef XTRACE_ERRNO_
    #define XTRACE_ERRNO_(err, ...)     do {} while(0)
#endif
#ifndef XTRACE_ERRNO
    #define XTRACE_ERRNO(...)           do {} while(0)
#endif
#ifdef _WIN32
    #ifndef XTRACE_ERR_
        #define XTRACE_ERR_(err, ...)   do {} while(0)
    #endif
    #ifndef XTRACE_ERR
        #define XTRACE_ERR(...)         do {} while(0)
    #endif
    #ifndef XTRACE_HR_
        #define XTRACE_HR_(hr, ...)     do {} while(0)
    #endif
    #ifndef XTRACE_HR
        #define XTRACE_HR(...)          do {} while(0)
    #endif
#endif
#ifndef XDUMP
    #define XDUMP(ptr, size)            do {} while(0)
#endif
#ifndef XDUMP_W
    #define XDUMP_W(ptr, size)          do {} while(0)
#endif
#ifndef XDUMP_E
    #define XDUMP_E(ptr, size)          do {} while(0)
#endif


#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* XTRACE_H */
