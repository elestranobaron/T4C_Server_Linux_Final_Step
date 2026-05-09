#ifdef _WIN32
#include "stdafx.h"
#endif
#include "ScriptFile.h"
#ifdef _WIN32
#ifdef _WIN32
#ifdef _WIN32
#include <process.h>
#endif
#endif
#endif
#include <chrono>
#include <thread>
#include <ctime>
#include <cstdio>
#include <cstring>
#ifdef _WIN32
#include "ThreadMonitor.h"
#include "DeadlockDetector.h"
#include "TFC Server.h"
#include "PlayerManager.h"
#include "Players.h"
#include "SysopCmd.h"
#endif
#include <stdarg.h>
#include <filesystem>
#include <fstream>

#if __cplusplus < 201703L
#error "std::filesystem requires C++17 or later"
#endif

namespace fs = std::filesystem;

#ifndef _WIN32
#include "TFC Server.h"
#include "ThreadMonitor.h"
#include "DeadlockDetector.h"
#include "TFCTimers.h"
extern CTFCServerApp theApp;
#endif

#ifdef _WIN32
extern CTFCServerApp theApp;
#endif

#define COMMENT				   if( strCmd[0] == '#' )
#define CHECK_COMMAND( __cmd ) else if( strCmd.find( __cmd ) != -1 )
#define BASIC_COMMAND		   else

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

ScriptFile::ScriptFile()
{
	boStopScriptExecution = false;
	boSafeMode        = false;
	boScriptIsRunning = false;
	boTraceExecution  = false;
	boKeepScript	  = false;

	strGodCharacter   = "";
	strScriptFilename = "";

	if( theApp.dwScriptLoopLength > 0 )
	{
		boScriptThreadDone	 = false;
#ifdef _WIN32
		hScriptThread = (HANDLE)_beginthreadex( NULL, 0, ScriptThread, 0, 0, &nScriptThreadId );
#else
		hScriptThread = std::thread( []() { ScriptFile::ScriptThread( nullptr ); } );
		nScriptThreadId = hScriptThread.get_id();
#endif
#ifdef _WIN32
		_LOG_DEBUG
			LOG_DEBUG_LVL1,
			"Script Thread ID=%u",
			GetThreadId()
		LOG_
#endif
	}
	else
		boScriptThreadDone	 = true;
}

////////////////////////////////////////////
// Return only instance of the class
ScriptFile& ScriptFile::GetInstance( void )
{
	static ScriptFile inst;
	return inst;
}

////////////////////////////////////////////
// Main Script thread
unsigned int CALLBACK ScriptFile::ScriptThread( void* pParam )
{
	CAutoThreadMonitor tmMonitor( "Script Thread" );
	GetInstance().ThreadFunc();
	return 0;
}

#ifndef _WIN32
std::uint64_t ScriptFile::GetThreadId() const
{
	// Hash thread::id to a stable numeric for logs.
	return static_cast<std::uint64_t>( std::hash<std::thread::id>{}( nScriptThreadId ) );
}
#endif

////////////////////////////////////////////
// Called by ScriptThread
void ScriptFile::ThreadFunc( void )
{
#ifdef _WIN32
	START_DEADLOCK_DETECTION( hScriptThread, "Script Thread" );
#else
	START_DEADLOCK_DETECTION( GetCurrentThread(), "Script Thread" );
#endif

	// Waiting the right time to start the thread
	int PriorityChange = 1;
	int lastsecond = 0;

	while( !boScriptThreadDone )
	{
		KEEP_ALIVE

		// Run script if possible
		if( theApp.dwScriptLoopLength > 0 )
			RunScript( fs::path("Script") / "script.txt" );

		KEEP_ALIVE

		// Verify global timers
		TFCTimerManager::VerifyTimers();

		std::this_thread::sleep_for( std::chrono::milliseconds( theApp.dwScriptLoopLength * 1000 ) );
	}

	STOP_DEADLOCK_DETECTION
}

ScriptFile::~ScriptFile()
{
#ifndef _WIN32
	if ( hScriptThread.joinable() )
	{
		hScriptThread.join();
	}
#endif
}

////////////////////////////////////////////
// Executes script content
int ScriptFile::RunScript( const fs::path &scriptPath )
{	
	if( boScriptIsRunning )
		return -1;

	boScriptIsRunning = true;

	int nResult = 0;

	try
	{
		// Open script
		std::string strScript =	OpenScript( scriptPath );

		// Default value is false
		boTraceExecution = false;
		boSafeMode = false;
		boKeepScript = false;
		boStopScriptExecution = false;
		strGodCharacter  = "";
		strScriptFilename = scriptPath.string();

		// Parse line after line
		while( !strScript.empty() && !boStopScriptExecution )
		{
			// Get end of line
			std::string::size_type pos_endl = strScript.find_first_of( "\n" )+1;

			// If no \n, this is the end of file
			if( pos_endl == 0 )
			{
				// Only line left is the last command
				pos_endl = strScript.size();
				TranslateScript( strScript );
			}
			else
			{
				// Translate command
				std::string strCmd = strScript.substr( 0, pos_endl-1 );
				TranslateScript( strCmd );
			}

			// Remove treated line
			strScript.erase( 0, pos_endl );
		}
	}
	catch( int err )
	{
		nResult = err;
	}

	if( !boKeepScript )
	{
		// Rename file to avoid to be executed another time
		char newFilename[128];
		sprintf( newFilename, "%d_%s", time( NULL ), strScriptFilename.c_str() );
		
		std::error_code ec;
		fs::rename( fs::path(strScriptFilename), fs::path(newFilename), ec );
	}

	strScriptFilename = "";
	boScriptIsRunning = false;

	return nResult;
}

////////////////////////////////////////////
// Open script (if any errors, thows an exception )
std::string ScriptFile::OpenScript( const fs::path &filePath )
{
	std::ifstream in( filePath, std::ios::binary );
	if( !in.is_open() )
		throw 1;

	std::string content(
		(std::istreambuf_iterator<char>( in )),
		std::istreambuf_iterator<char>()
	);
	if( content.empty() )
		throw 2;

	return content;
}

////////////////////////////////////////////
// Translate a command
void ScriptFile::TranslateScript( std::string strCmd )
{
#ifndef _WIN32
	(void)strCmd;
	// Linux migration TODO: port Players/CPlayerManager/SysopCmd dependencies.
	return;
#else
	// Comments
	COMMENT
	{
		if( boTraceExecution )
			TraceExecution( "Commented line." );
	}
	// Enable report
	CHECK_COMMAND( "REPORT ON" )
	{
		InitTrace();
	}
	// Disable report
	CHECK_COMMAND( "REPORT OFF" )
	{
		if( boTraceExecution )
			TraceExecution( "Report disabled." );

		boTraceExecution = false;
	}
	// Does it stop as soon as an error occurs?
	CHECK_COMMAND( "SAFE MODE" )
	{
		boSafeMode = true;

		if( boTraceExecution )
			TraceExecution( "Safe mode activated." );
	}
	// Normal execution
	CHECK_COMMAND( "NORMAL MODE" )
	{
		boSafeMode = false;

		if( boTraceExecution )
			TraceExecution( "Normal mode activated." );
	}
	// Defines which char must be used
	CHECK_COMMAND( "USE CHAR" )
	{
		// Get character name
		strGodCharacter = strCmd.erase( 0, strlen( "USE CHAR " ) );

		// Check if this character is online
		if( CPlayerManager::GetCharacter( strGodCharacter.c_str() ) != NULL )
		{
			if( boTraceExecution )
				TraceExecution( "Character %s will be considered as god.", strGodCharacter.c_str() );
		}
		else
		{
			if( boTraceExecution )
				TraceExecution( "Character %s isn't online.", strGodCharacter.c_str() );
			if( boSafeMode )
				boStopScriptExecution = true;

			strGodCharacter = "";
		}
	}
	// Wait X seconds before continue execution
	CHECK_COMMAND( "SLEEP" )
	{
		// Get the delay
		std::string strLength = strCmd.erase( 0, strlen( "SLEEP " ) );
		int delay = atoi( strLength.c_str() );

		// Only if the delay is valid...
		if( delay > 0 )
		{
			if( boTraceExecution )
				TraceExecution( "Waiting %d seconds...", delay );

			Sleep( delay*1000 );
		}
		else
		{
			if( boTraceExecution )
				TraceExecution( "Weird delay! (%d seconds)", delay );
			if( boSafeMode )
				boStopScriptExecution = true;
		}
	}
	// Change loop delay
	CHECK_COMMAND( "SET LOOP" )
	{
		// Get new time
		std::string strLength = strCmd.erase( 0, strlen( "SET LOOP " ) );
		int delay = atoi( strLength.c_str() );

		// if new time is valid
		if( delay > 0 )
		{
			if( boTraceExecution )
				TraceExecution( "Loop delay has been set to %d seconds.", delay );

			theApp.dwScriptLoopLength = delay;
		}
		else
		{
			if( boTraceExecution )
				TraceExecution( "Invalid loop delay (%d seconds).", delay );
			if( boSafeMode )
				boStopScriptExecution = true;
		}

	}
	// Rename script to avoid to be executed another time
	CHECK_COMMAND( "DO NOT RENAME ME" )
	{
		boKeepScript = true;

		if( boTraceExecution )
			TraceExecution( "Script will be kept in the script directory." );
	}
	// Include a sub script
	CHECK_COMMAND( "INCLUDE" )
	{
		// Get script filename
		std::string strFileName = strCmd.erase( 0, strlen( "INCLUDE " ) );
		
		if( boTraceExecution )
			TraceExecution( "Include script %s.\r\n*****************************************************", strFileName.c_str() );

		// Create a new instance of a ScriptFile class to execute the sub script
		ScriptFile includedScript;
		int res = includedScript.RunScript( (char*) strFileName.c_str() );

		if( boTraceExecution )
			TraceExecution( "Script returned code %d.\r\n******************************************************", res );

		if( boSafeMode && res != 0 )
			boStopScriptExecution = true;
	}
	BASIC_COMMAND
	{
		bool boTemporaryUser = false;
		Players* user;

		// If a character has been assigned, get it
		if( strGodCharacter.size() > 0 )
		{
			user = CPlayerManager::GetCharacter( strGodCharacter.c_str() );

			// If user cannot be fetched
			if( user == NULL )
			{
				if( boTraceExecution )
					TraceExecution( "Character %s cannot be found. Sysop command cannot be translated.", strGodCharacter.c_str() );

				if( boSafeMode )
					boStopScriptExecution = true;

				return;
			}
		}
		else
		{
			// Else build a temporary character flagged
			user = new Players;
			boTemporaryUser = true;
			
			user->self->SetName( "OfflineScript" );
			user->SetAccount( "OfflineScript" );
			user->SetGodMode( TRUE );
			user->SetGodFlags( GOD_CAN_ZAP | GOD_CAN_LOCKOUT_USER |
						   GOD_CAN_SQUELCH | GOD_CAN_REMOVE_SHOUTS |
						   GOD_CAN_SUMMON_MONSTERS | GOD_CAN_SUMMON_ITEMS |
						   GOD_CAN_SET_USER_FLAG | GOD_CAN_EDIT_USER_STAT | GOD_CAN_EDIT_USER_HP | GOD_CAN_EDIT_USER_MANA_FAITH | GOD_CAN_EDIT_USER_XP_LEVEL | GOD_CAN_EDIT_USER_NAME | GOD_CAN_EDIT_USER_APPEARANCE_CORPSE | GOD_CAN_EDIT_USER_SPELLS | GOD_CAN_EDIT_USER_SKILLS | GOD_CAN_EDIT_USER_BACKPACK |
						   GOD_CAN_VIEW_USER_STAT | GOD_CAN_VIEW_USER_BACKPACK | GOD_CAN_VIEW_USER_SPELLS | GOD_CAN_VIEW_USER_SKILLS | GOD_CAN_VIEW_USER_APPEARANCE_CORPSE |
						   GOD_CAN_SLAY_USER | GOD_CAN_TELEPORT_USER | GOD_CAN_TELEPORT |
						   GOD_CAN_COPY_USER | GOD_CAN_EMULATE_MONSTER | GOD_CAN_SHUTDOWN |
						   GOD_CAN_SEE_ACCOUNTS | GOD_CAN_GIVE_GOD_FLAGS | GOD_CAN_EMULATE_SYSTEM | GOD_CHAT_MASTER );
		}

		// Parse sysop command
		int boSuccess = SysopCmd::VerifySysopCommand( user, strCmd.c_str() );

		// If Sysop command failed
		if( !boSuccess )
		{
			if( boTraceExecution )
				TraceExecution( "Sysop command \"%s\" failed.", strCmd.c_str() );

			if( boSafeMode )
				boStopScriptExecution = true;
		}
		else
		{
			if( boTraceExecution )
				TraceExecution( "Sysop command \"%s\" succeed.", strCmd.c_str() );
		}

		// If this is a temporary user, free it
		if( boTemporaryUser )
			delete user;
	}
#endif
}

////////////////////////////////////////////
// Write a debug file
void ScriptFile::TraceExecution( const char* msg, ... )
{	
	va_list	args;
	va_start( args, msg );

	char buf[512];
	char date[64];

	vsnprintf( buf, sizeof( buf ), msg, args );
	strcat( buf, "\r\n" );

	std::time_t t = std::time( nullptr );
	std::tm tmLocal{};
#ifdef _WIN32
	localtime_s( &tmLocal, &t );
#else
	localtime_r( &t, &tmLocal );
#endif

	// Format date/time
	sprintf( date, "%04d/%02d/%02d %02d:%02d:%02d,", 
		tmLocal.tm_year + 1900,
        tmLocal.tm_mon + 1,
        tmLocal.tm_mday,
        tmLocal.tm_hour,
        tmLocal.tm_min,
        tmLocal.tm_sec  );

	// Open report file
	const fs::path reportDir = fs::path("Script");
	const fs::path reportPath = reportDir / "report.txt";
	std::error_code ec;
	fs::create_directories( reportDir, ec );

	std::ofstream out( reportPath, std::ios::app | std::ios::binary );
	if( out.is_open() )
	{
		out.write( date, static_cast<std::streamsize>(strlen( date )) );
		out.write( buf, static_cast<std::streamsize>(strlen( buf )) );
		out.flush();
	}
	else
	{
		boTraceExecution = false;
	}

	va_end( args );
}

////////////////////////////////////////////
// Init debug file
void ScriptFile::InitTrace( void )
{
	boTraceExecution = true;

	TraceExecution( "----- Script-Running -----" );
	TraceExecution( "Report enabled." );
}
