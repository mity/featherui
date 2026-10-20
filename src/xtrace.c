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

#ifdef _WIN32
    /* On Windows we may need to define one of these to get C99-compatible
     * behavior of printf() format strings. */
    #ifndef _CRT_STDIO_ISO_WIDE_SPECIFIERS
        #define _CRT_STDIO_ISO_WIDE_SPECIFIERS      1
    #endif
    #ifndef __USE_MINGW_ANSI_STDIO
        #define __USE_MINGW_ANSI_STDIO              1
    #endif
    #include <windows.h>
#endif
#include <ctype.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "xtrace.h"


#if defined XTRACE_IMPL || defined XTRACE || defined DEBUG


#ifndef thread_local
    #if __STDC_VERSION__ >= 201112 && !defined __STDC_NO_THREADS__
        #define thread_local            _Thread_local
    #elif defined _WIN32 && (defined _MSC_VER || defined __ICL || defined __DMC__ || defined __BORLANDC__ )
        #define thread_local            __declspec(thread)
    #elif defined __GNUC__ || defined __clang__ ||  defined __SUNPRO_C  ||  defined __hpux || defined __xlC__
        #define thread_local            __thread
    #else
        #define thread_local
    #endif
#endif


/* Allow all of these to be overridden in compile time:
 */

#ifndef XTRACE_MAX_PROLOG_LEN
    #define XTRACE_MAX_PROLOG_LEN                   64
#endif

#ifndef XTRACE_MAX_MSG_LEN
    #define XTRACE_MAX_MSG_LEN                      256
#endif

#ifndef XTRACE_MAX_EPILOG_LEN
    #define XTRACE_MAX_EPILOG_LEN                    16
#endif

#ifndef XTRACE_MAX_DUMP_SIZE
    /* Longer dumps will be truncated. */
    #define XTRACE_MAX_DUMP_SIZE                    2048
#endif

#ifndef XTRACE_DEFAULT_LEVEL
    /* 0 = trace only XTRACE_ALWAYS()
     * 1 = trace XTRACE_ALWAYS() and errors
     * 2 = trace XTRACE_ALWAYS(), errors and warnings
     * 3 = trace everything */
    #define XTRACE_DEFAULT_LEVEL                    3
#endif



static unsigned xtrace_level = XTRACE_DEFAULT_LEVEL;

static const char*
file_basename(const char* path)
{
    const char* delim;
    const char* ptr = path;

    delim = (const char*) strrchr(path, '/');
    ptr = (delim != NULL ? delim + 1 : ptr);

#ifdef _WIN32
    delim = (const char*) strrchr(ptr, '\\');
    ptr = (delim != NULL ? delim + 1 : ptr);
#endif

    return ptr;
}

static void
xtrace_prolog(char buf[XTRACE_MAX_PROLOG_LEN+1], const char* fname, unsigned line, const char* func, unsigned level)
{
    const char* tag;

    fname = file_basename(fname);

    switch(level) {
        case 0:     tag = "[.]"; break;
        case 1:     tag = "[E]"; break;
        case 2:     tag = "[w]"; break;
        default:    tag = "[ ]"; break;
    }

    if(func != NULL)
        snprintf(buf, XTRACE_MAX_PROLOG_LEN, "%s %s:%u (%s)", tag, fname, line, func);
    else
        snprintf(buf, XTRACE_MAX_PROLOG_LEN, "%s %s:%u", tag, fname, line);
    buf[XTRACE_MAX_PROLOG_LEN] = '\0';
}

static void
xtrace_epilog(char buf[XTRACE_MAX_EPILOG_LEN+1], unsigned flags, int errcode)
{
    if(flags & 0x0100)
        sprintf(buf, " [%d]", errcode);
#ifdef _WIN32
    else if(flags & 0x1000)
        sprintf(buf, " [%u]", (uint32_t) errcode);
    else if(flags & 0x2000)
        sprintf(buf, " [0x%x]", (uint32_t) errcode);
#endif
    else
        buf[0] = '\0';
}

static thread_local const char* last_fname = 0;
static thread_local unsigned last_line = 0;
static thread_local unsigned last_level = 0;

void
xtrace_msg__(const char* fname, unsigned line, const char* func,
             unsigned flags, int errcode, const char* fmt, ...)
{
    unsigned level = (flags & 0x3);
    va_list args;
    char prolog[XTRACE_MAX_PROLOG_LEN+1];
    char msg[XTRACE_MAX_MSG_LEN+1];
    char epilog[XTRACE_MAX_EPILOG_LEN+1];
    int n;

    last_fname = fname;
    last_line = line;
    last_level = level;

    if(level > xtrace_level)
        return;

    xtrace_prolog(prolog, fname, line, func, level);

    va_start(args, fmt);
    n = vsnprintf(msg, XTRACE_MAX_MSG_LEN, fmt, args);
    va_end(args);
    msg[XTRACE_MAX_MSG_LEN] = '\0';

    if(n > 0  &&  n <= XTRACE_MAX_MSG_LEN) {
        if(msg[n-1] == '\n')
            msg[n-1] = '\0';
    }

    xtrace_epilog(epilog, flags, errcode);

    {
#ifdef _WIN32
        static INIT_ONCE console_once = INIT_ONCE_STATIC_INIT;
        static BOOL has_console = FALSE;
        BOOL pending;

        if(InitOnceBeginInitialize(&console_once, 0, &pending, NULL)) {
            has_console = (GetConsoleWindow() != NULL);
            InitOnceComplete(&console_once, 0, NULL);
        }

        if(!has_console) {
            char buf[XTRACE_MAX_PROLOG_LEN + XTRACE_MAX_MSG_LEN + XTRACE_MAX_EPILOG_LEN + 16];
            buf[sizeof(buf)-1] = '\0';
            sprintf(buf, "%s: %s%s\n", prolog, msg, epilog);
            OutputDebugStringA(buf);
        } else
#endif
        {
            fprintf(stderr, "%s: %s%s\n", prolog, msg, epilog);
        }
    }
}

#define DUMP_N_BYTES_PER_LINE           16
#define DUMP_N_BYTES_PER_EXTRA_SPACE    8

static void
xtrace_dump_block(const char* fname, unsigned line, const char* func,
                  unsigned flags, const void* addr, size_t size)
{
    const uint8_t* bytes = (const uint8_t*) addr;
    size_t off = 0;

    while(off < size) {
        char buffer[4 * DUMP_N_BYTES_PER_LINE + 64];
        char* ptr = buffer;
        size_t line_off = off;

        ptr += sprintf(ptr, "   %08zx:  ", off);

        /* Hex output */
        while(off - line_off < DUMP_N_BYTES_PER_LINE) {
            if(off < size)
                ptr += sprintf(ptr, "%02x ", bytes[off]);
            else
                ptr += sprintf(ptr, "   ");
            off++;
            if(off % DUMP_N_BYTES_PER_EXTRA_SPACE == 0)
                ptr += sprintf(ptr, "  ");
        }

        off = line_off;
        ptr += sprintf(ptr, "| ");

        /* Char output */
        while(off - line_off < DUMP_N_BYTES_PER_LINE) {
            if(off < size)
                ptr += sprintf(ptr, "%c",
                        (iscntrl(bytes[off]) || (uint8_t)bytes[off] >= 0x80 ? '.' : bytes[off]));
            else
                break;
            off++;
        }

        xtrace_msg__(fname, line, func, flags, 0, "%s", buffer);
    }
}

void
xtrace_dump__(const char* fname, unsigned line, const char* func,
              unsigned flags, const char* desc, const void* addr, size_t size)
{
    unsigned level = (flags & 0x3);

    if(level == 0) {
        /* A kludge for XDUMP() inheriting the level of preceding XTRACE()
         * message. */
        if(fname == last_fname  &&  (line == last_line || line == last_line+1))
            level = last_level;
        else
            level = 3;

        flags &= 0x3;
        flags |= level;
    }

    if(level > xtrace_level)
        return;

    xtrace_msg__(fname, line, func, flags, 0, "Dump %s [%zu bytes]:", desc, size);

    /* We're actually little more generous as it makes no sense to
     * skip too few bytes and replace them with longer "Skipping..." message
     */
    if(size > XTRACE_MAX_DUMP_SIZE + DUMP_N_BYTES_PER_LINE * 4) {
        size_t head_size = XTRACE_MAX_DUMP_SIZE / 2;
        size_t tail_size = XTRACE_MAX_DUMP_SIZE / 2;

        /* Make sure head ends and tail begins at a reasonable offsets. */
        head_size += DUMP_N_BYTES_PER_LINE - 1;
        head_size -= head_size % DUMP_N_BYTES_PER_LINE;
        tail_size -= tail_size % DUMP_N_BYTES_PER_LINE;

        xtrace_dump_block(fname, line, func, flags, addr, head_size);

        xtrace_msg__(fname, line, func, flags, 0,
                    " ... (too long; skipping %zu bytes) ...",
                    size - head_size - tail_size);

        xtrace_dump_block(fname, line, func, flags,
                    (const uint8_t*)addr + size - tail_size, tail_size);
    } else {
        xtrace_dump_block(fname, line, func, flags, addr, size);
    }
}


#endif  /* #if defined XTRACE_IMPL || defined XTRACE || defined DEBUG */
