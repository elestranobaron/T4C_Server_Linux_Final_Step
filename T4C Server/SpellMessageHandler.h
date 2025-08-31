#if !defined(AFX_SPELLMESSAGEHANDLER_H__184E5E03_90FC_11D1_AD0B_00E029058623__INCLUDED_)
#define AFX_SPELLMESSAGEHANDLER_H__184E5E03_90FC_11D1_AD0B_00E029058623__INCLUDED_

#include "SpellEffect.h"
#include <string>

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define MAX_SPELLS			13000
#define SPELL_ID_OFFSET		10000

#define SPELL_PROC_PROTOTYPE	WORD wSpellID, Unit *self, Unit *medium, Unit *target, WorldPos wlPos
#define SPELL_PROC_PARAM		wSpellID, wMana, self, medium, target, wlPos

DWORD __declspec( dllexport ) GetNextGlobalEffectID();

class SpellMessageHandler;

//////////////////////////////////////////////////////////////////////////////////////////
// Provide message handling and dispatching for spells.
class __declspec( dllexport ) SpellMessageHandler
{
public:
	static void Create( void );
    static void Destroy( void );
	
    static void RegisterSpell( WORD wSpellID, LPSPELL_STRUCT lpSpellStruct );

	static BOOL ActivateSpell( SPELL_PROC_PROTOTYPE );

	static LPSPELL_STRUCT GetSpell( WORD wSpellID );
    static LPSPELL_STRUCT GetSpellByName( std::string spellName, WORD wLang );

	static BOOL IsSpellLearnable( WORD wSpellID, Unit *lpuLearner, CString &reqText );



private:
	static LPSPELL_STRUCT	lpSpellTable[ MAX_SPELLS ];

};

#endif // !defined(AFX_SPELLMESSAGEHANDLER_H__184E5E03_90FC_11D1_AD0B_00E029058623__INCLUDED_)
