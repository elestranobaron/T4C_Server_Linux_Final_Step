#if !defined(AFX_MARSACCRED_H__C944C5B7_86C5_11D1_BDE7_00E029058623__INCLUDED_)
#define AFX_MARSACCRED_H__C944C5B7_86C5_11D1_BDE7_00E029058623__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "NPCstructure.h"

class MarsacCred : public NPCstructure  
{
public:
	MarsacCred();
	virtual ~MarsacCred();

	void Create( void );
	void OnTalk( UNIT_FUNC_PROTOTYPE );
    void OnNPCDataExchange( UNIT_FUNC_PROTOTYPE );
};

#endif // !defined(AFX_MARSACCRED_H__C944C5B7_86C5_11D1_BDE7_00E029058623__INCLUDED_)
