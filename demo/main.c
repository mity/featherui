/*
 * This example is provided for unrestricted use.
 * You may use, copy, modify, and distribute it for any purpose,
 * including commercial purposes, without restriction.
 */

#include <windows.h>
#include <winuser.h>

#include <feather.h>

#include "libex/defs.h"


int
wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nShowCmd)
{
    int exit_code = EXIT_FAILURE;

    UNUSED(hInstance);
    UNUSED(hPrevInstance);
    UNUSED(lpCmdLine);
    UNUSED(nShowCmd);

    feInitialize(NULL, NULL, 0);

    feAppWindow_Create(NULL);
    feRun(&exit_code);

    feTerminate();
    return exit_code;
}
