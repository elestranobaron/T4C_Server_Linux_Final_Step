#if !defined(AFX_CRITICALSTRIKE_H__7ADCA875_570C_11D1_BD7D_00E029058623__INCLUDED_)
#define AFX_CRITICALSTRIKE_H__7ADCA875_570C_11D1_BD7D_00E029058623__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "Skills.h"

class CriticalStrike  
{
public:
	CriticalStrike();
	void Destroy( void );

	static LPSKILLPNTFUNC lpOnAddPnts;

	static int Func(DWORD dwReason, Unit *self, Unit *medium, Unit *target, 
					void *valueIN, void *valueOUT, LPUSER_SKILL lpusUserSkill);

	SKILL_ATTRIBUTES s_saAttrib;
};

#endif // !defined(AFX_CRITICALSTRIKE_H__7ADCA875_570C_11D1_BD7D_00E029058623__INCLUDED_)
