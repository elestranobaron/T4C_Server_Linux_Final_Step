#ifndef PORTABILITY_H
#define PORTABILITY_H

#ifdef _WIN32
#include <windows.h>
#include <afxstr.h>
typedef CString String;
#else
#include <string>
#include <algorithm> // for std::remove
class String : public std::string {
public:
    using std::string::string;
    using std::string::operator=;
    void Remove(char ch) {
        erase(std::remove(begin(), end(), ch), end());
    }
    bool IsEmpty() const {
        return empty();
    }
    void MakeUpper() {
        std::transform(begin(), end(), begin(), ::toupper);
    }
    operator const char*() const {
        return c_str();
    }
    // Add more methods if needed based on usage
};
#include <pthread.h>
typedef pthread_mutex_t CRITICAL_SECTION;
inline void InitializeCriticalSection(CRITICAL_SECTION* cs) { pthread_mutex_init(cs, NULL); }
inline void DeleteCriticalSection(CRITICAL_SECTION* cs) { pthread_mutex_destroy(cs); }
inline void EnterCriticalSection(CRITICAL_SECTION* cs) { pthread_mutex_lock(cs); }
inline void LeaveCriticalSection(CRITICAL_SECTION* cs) { pthread_mutex_unlock(cs); }

typedef long long __int64;
typedef unsigned long long uhyper;

// Dummy RGB for colors, can be adjusted if needed
#define RGB(r, g, b) ((unsigned int)(((unsigned char)(r) | ((unsigned short)((unsigned char)(g)) << 8)) | (((unsigned int)(unsigned char)(b)) << 16)))

typedef void* LPEXCEPTION_POINTERS;

#endif

#include <cstdarg> // for va_start, va_end
#include <cstring> // for strcpy, strlen

#endif // PORTABILITY_H
