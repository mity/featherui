
#ifndef FEATHER_APPWINDOW_H
#define FEATHER_APPWINDOW_H

#include <feather/_internal.h>

#ifdef __cplusplus
extern "C" {
#endif


/* TODO: This is just temporary hack, it will need more flexible interface. */
HRESULT FEATHER_API feAppWindow_Create(HWND* pHwnd);


#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* FEATHER_APPWINDOW_H */
