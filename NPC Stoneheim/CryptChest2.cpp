#pragma hdrstop
#include "CryptChest2.h"

CryptChest2::CryptChest2()
{}

CryptChest2::~CryptChest2()
{}

extern NPCstructure::NPC OracleChestNPC;

void CryptChest2::OnAttacked( UNIT_FUNC_PROTOTYPE ){
}

void CryptChest2::OnHit( UNIT_FUNC_PROTOTYPE ){
}

void CryptChest2::Create( void )
{
      npc = ( OracleChestNPC );
      SET_NPC_NAME( "[10840]wooden chest" );  
      npc.InitialPos.X = 558; 
      npc.InitialPos.Y = 1068;
      npc.InitialPos.world = 1;
}

void CryptChest2::OnInitialise( UNIT_FUNC_PROTOTYPE ){
	NPCstructure::OnInitialise( UNIT_FUNC_PARAM );
	WorldPos wlPos = { 0,0,0 };
	self->SetDestination( wlPos );
	self->Do( nothing );
	self->SetCanMove( FALSE );
}

void CryptChest2::OnTalk( UNIT_FUNC_PROTOTYPE )
///////////////////////////////////////////////////////////////////////////

{

InitTalk

Begin
""
IF(IsInRange(4))
	IF(CheckItem(__OBJ_DEW_COVERED_METAL_KEY) >= 1)
		IF(CheckFlag(__FLAG_CRYPT_RANDOM_CHEST) == 0)
			GiveFlag(__FLAG_CRYPT_RANDOM_CHEST, rnd.roll(dice(1, 5)))
		ENDIF
		IF(CheckFlag(__FLAG_CRYPT_RANDOM_CHEST) == 2)
			IF(CheckItem(__OBJ_CRUMBLING_BONE_KEY) >= 1)
				TakeItem(__OBJ_DEW_COVERED_METAL_KEY)
				PRIVATE_SYSTEM_MESSAGE(INTL( 9350, "Using the dew-covered metal key, you unlock the chest."))
				PRIVATE_SYSTEM_MESSAGE(INTL( 9351, "The key vanishes inside the lock."))
				PRIVATE_SYSTEM_MESSAGE(INTL( 8700, "The chest is empty."))
			ELSE
				TakeItem(__OBJ_DEW_COVERED_METAL_KEY)
				GiveItem(__OBJ_CRUMBLING_BONE_KEY)
				GiveFlag(__FLAG_CRYPT_RANDOM_CHEST, 0)
				PRIVATE_SYSTEM_MESSAGE(INTL( 9350, "Using the dew-covered metal key, you unlock the chest."))
				PRIVATE_SYSTEM_MESSAGE(INTL( 9351, "The key vanishes inside the lock."))
				PRIVATE_SYSTEM_MESSAGE(INTL( 9352, "Inside the chest you find a crumbling bone key."))
			ENDIF
		ELSE
			TakeItem(__OBJ_DEW_COVERED_METAL_KEY)
			PRIVATE_SYSTEM_MESSAGE(INTL( 9350, "Using the dew-covered metal key, you unlock the chest."))
			PRIVATE_SYSTEM_MESSAGE(INTL( 9351, "The key vanishes inside the lock."))
			PRIVATE_SYSTEM_MESSAGE(INTL( 8700, "The chest is empty."))
		ENDIF
	ELSE
		PRIVATE_SYSTEM_MESSAGE(INTL( 9355, "The chest is locked."))
	ENDIF
ENDIF
BREAK

Default
""
BREAK

EndTalk
}