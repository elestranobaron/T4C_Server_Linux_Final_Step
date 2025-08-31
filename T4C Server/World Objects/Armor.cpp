#include "stdafx.h"
#include "Armor.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Armor::Armor()
{
}

Armor::~Armor()
{

}

void Armor::OnAttacked( UNIT_FUNC_PROTOTYPE ){
	ObjectStructure::OnAttacked( UNIT_FUNC_PARAM );

	LPATTACK_STRUCTURE Blow = (LPATTACK_STRUCTURE)valueIN;
		
	// Substracts the dynamic AC flag
	//if(Blow) Blow->Strike -= armor.AC;
}

//////////////////////////////////////////////////////////////////////////////////////////
ObjectStructure *Armor::CreateObject( void ){
	return new Armor;
}