#ifndef T4C_STANDARD_TYPES_H
#define T4C_STANDARD_TYPES_H

#include <cstdint>

// Fixed-width replacements for legacy Win32 integral aliases.
using BYTE = std::uint8_t;
using WORD = std::uint16_t;
using DWORD = std::uint32_t;
using INT = int;
using LPBOOL = int *;

#ifndef _WIN32
#ifndef BOOL
typedef int BOOL;
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
using LPBYTE = BYTE *;
using LPVOID = void *;
using COLORREF = std::uint32_t;
using USHORT = std::uint16_t;
using SOCKET = int;
using LPCSTR = const char *;
using LONG = long;
#ifndef __declspec
#define __declspec(x)
#endif

inline LONG InterlockedIncrement(LONG *value) {
    return ++(*value);
}
inline LONG InterlockedDecrement(LONG *value) {
    return --(*value);
}
#endif

#endif // T4C_STANDARD_TYPES_H
