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
        TRACE("feRun: No application window.");
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
