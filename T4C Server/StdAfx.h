#if !defined(AFX_STDAFX_H__BC8F306A_A74F_11D0_9B9E_444553540000__INCLUDED_)
#define AFX_STDAFX_H__BC8F306A_A74F_11D0_9B9E_444553540000__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifdef _WIN32

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers
#ifndef _WIN32_WINNT
	#define _WIN32_WINNT 0x0600
#endif 
#define DEBUG			#ifdef _DEBUG

//#define _AFX_NO_OLE_SUPPORT
//#define _AFX_NO_DB_SUPPORT
//#define _AFX_NO_DAO_SUPPORT
#pragma warning( disable : 4786 )   // Debug string too long.
#pragma warning( disable : 4284 )   // Return type of operator ->

// To disable when debugging app.
#pragma warning( disable : 4244 )   // Conversion from 'type1' to 'type2'
#pragma warning( disable : 4018 )   // Signed/unsigned mismatched.


//#define __ENABLE_LOG
#include <afxdao.h>

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxmt.h>

#include <afxsock.h>		// MFC socket extensions

#include <imagehlp.h>

#else /* !_WIN32 — Linux / POSIX server build */

#include <cstddef>
#include <cstring>
#include "Win32Compat.h"
#include "Portability.h"

#ifndef TRACE
#define TRACE(...) ((void)0)
#endif

#endif /* _WIN32 */

#if defined(MEM_DEBUG) && defined(_WIN32)
    #include <smrtheap.hpp>
#endif

#include "ExitCode.h"
#include "Timer.h"

#ifdef __ENABLE_LOG
	extern DEBUG_LOG __LOG;
#endif

#ifndef ATLASSERT
#ifdef _WIN32
#define ATLASSERT ASSERT
#else
#include <cassert>
#define ATLASSERT(expr) assert(expr)
#ifndef ASSERT
#define ASSERT(expr) assert(expr)
#endif
#endif
#endif
namespace vir{};
using namespace vir;
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__BC8F306A_A74F_11D0_9B9E_444553540000__INCLUDED_)
