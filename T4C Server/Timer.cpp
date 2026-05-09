#ifdef _WIN32
#include "stdafx.h"
#endif
#include "TimeUtils.h"

DWORD __declspec( dllexport ) GetRunTime(){
    static DWORD initialTime = GetMonotonicTickCountMs();
    return static_cast<DWORD>(GetMonotonicTickCountMs() - initialTime);
}