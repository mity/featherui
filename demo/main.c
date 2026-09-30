
#include <windows.h>
#include <winuser.h>

#include <feather.h>

#include "misc.h"


int
wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nShowCmd)
{
    int exit_code = EXIT_FAILURE;

    UNUSED(hInstance);
    UNUSED(hPrevInstance);
    UNUSED(lpCmdLine);
    UNUSED(nShowCmd);

    feInitialize(NULL, 0);

    feAppWindow_Create(NULL);
    feRun(&exit_code);

    feTerminate();
    return exit_code;
}
