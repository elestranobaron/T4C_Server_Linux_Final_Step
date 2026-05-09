#ifndef TIMER_H_18092000
#define TIMER_H_18092000

#include "StandardTypes.h"
#ifndef _WIN32
#ifndef __declspec
#define __declspec(x)
#endif
#endif

DWORD __declspec( dllexport ) GetRunTime();

#endif // TIMER_H_18092000