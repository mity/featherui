
#ifndef FEATHER_VERSION_H
#define FEATHER_VERSION_H

#include <feather/_internal.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct FeVersion FeVersion;
struct FeVersion {
    DWORD dwMajor;
    DWORD dwMinor;
    DWORD dwPatch;
};


void FEATHER_API feVersion(FeVersion* lpVersion);
const WCHAR* FEATHER_API feVersionStr(void);


#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* FEATHER_VERSION_H */
