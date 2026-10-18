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

#include "misc.h"
#include "mainloop.h"
#include "appwindow.h"


int
mainloop_run(void)
{
    MSG msg;

    while(GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int) msg.wParam;
}

void
mainloop_quit(int iExitCode)
{
    PostQuitMessage(iExitCode);
}


/************************
 *** Public Interface ***
 ************************/

HRESULT FEATHER_API
feRun(int* piExitCode)
{
    int exit_code = EXIT_FAILURE;

    if(appwindow_get_count() == 0) {
        XTRACE_E("feRun: No application window.");
        return E_FAIL;
    }

    exit_code = mainloop_run();

    if(piExitCode != NULL)
        *piExitCode = exit_code;
    return S_OK;
}

void FEATHER_API
feQuit(int iExitCode)
{
    mainloop_quit(iExitCode);
}
