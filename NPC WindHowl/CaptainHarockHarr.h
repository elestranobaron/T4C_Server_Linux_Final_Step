#if !defined(AFX_CAPTAINHAROCKHARR_H__C944C5B4_86C5_11D1_BDE7_00E029058623__INCLUDED_)
#define AFX_CAPTAINHAROCKHARR_H__C944C5B4_86C5_11D1_BDE7_00E029058623__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "NPCstructure.h"

class CaptainHarockHarr : public NPCstructure  
{
public:
	CaptainHarockHarr();
	virtual ~CaptainHarockHarr();

	void Create( void );
	void OnTalk( UNIT_FUNC_PROTOTYPE );
};

#endif // !defined(AFX_CAPTAINHAROCKHARR_H__C944C5B4_86C5_11D1_BDE7_00E029058623__INCLUDED_)
