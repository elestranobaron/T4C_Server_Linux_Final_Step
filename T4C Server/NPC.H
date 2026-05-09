#include "stdafx.h"

#include <CreatureListing.h>
#include <AttackListing.h>
#include <MonsterStructure.h>
#include <NPCstructure.h>
#include <Unit.h>
#include "Random.h"

#ifndef T4C_CREATE_NPC_FUNC_NAME
#define T4C_CREATE_NPC_FUNC_NAME CreateNPC
#endif

void MonsterStatSetup();
void MonsterStatDestroy();
void RegisterNCP( void );

//////////////////////////////////////////////////////////////////////////////////////////
// Start a NPC interpretion block

//////////////////////////////////////////////////////////////////////////////////////////
// Inits NPCs (Windows: MFC extension DLL entry point; Linux: static link, no DllMain.)
#ifdef _WIN32
#define INIT_NPC_DATA_PART1 Random rnd; static AFX_EXTENSION_MODULE NPCsDLL;\
void RegisterNPC( void );\
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)\
{\
	UNREFERENCED_PARAMETER(lpReserved);\
	switch(dwReason) {\
    case DLL_PROCESS_ATTACH: \
			if (!AfxInitExtensionModule(NPCsDLL, hInstance))\
				return 0;

#define INIT_NPC_DATA_PART2 TRACE0("NPC DLL Initializing!\n");\
			TFCMAIN::RegisterUnitStartupFunction( MonsterStatSetup );\
			RegisterNPC();\
		break;\
    case DLL_THREAD_ATTACH: break;\
	case DLL_THREAD_DETACH: break;\
    case DLL_PROCESS_DETACH: \
			TRACE0("NPC DLL Terminating!\n");\
			MonsterStatDestroy();\
			AfxTermExtensionModule(NPCsDLL);\
		break;\
    }\
return 1;\
}\
void RegisterNPC( void ){

#define INIT_NPC_DATA INIT_NPC_DATA_PART1 INIT_NPC_DATA_PART2

#define	END_INIT_NPC_DATA	};
#else
/* Linux: one executable links many NPC*.CPP units; avoid duplicate global symbols
 * (Windows used one DLL per NPC module). rnd + registration live in an anonymous
 * namespace per translation unit; ctor runs before main. */
#define INIT_NPC_DATA \
	namespace { \
		Random rnd; \
		void RegisterNPC_Impl(void); \
		struct NPCAutoRegister { \
			NPCAutoRegister() { RegisterNPC_Impl(); } \
		}; \
		static NPCAutoRegister g_npc_auto_register; \
		void RegisterNPC_Impl(void) {

#define END_INIT_NPC_DATA \
		} \
	}
#endif

#define REGISTER_NPC( ObjectName )	Unit::RegisterUnitMessageHandler( 30000, &ObjectName, #ObjectName, U_NPC, TRUE );

#define INIT_NPC_CREATION	void T4C_CREATE_NPC_FUNC_NAME(){

#define CREATE_NPC( ObjectName )	ObjectName.Create();

#define END_INIT_NPC_CREATION }
