#if !defined(AFX_SCRIPTFILE_H__1C5D1070_77F3_4C92_B3FE_143F922CF337__INCLUDED_)
#define AFX_SCRIPTFILE_H__1C5D1070_77F3_4C92_B3FE_143F922CF337__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "StandardTypes.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <thread>
#ifndef CALLBACK
#define CALLBACK
#endif
#endif

#include <string>
#include <filesystem>
//using namespace std;

class ScriptFile  
{
public:
	static ScriptFile &GetInstance();

#ifdef _WIN32
	unsigned int GetThreadId() { return nScriptThreadId; };
	HANDLE GetThreadHandle() { return hScriptThread; };
#else
	// GCC/Linux: expose a stable integer id for logging.
	std::uint64_t GetThreadId() const;
#endif

	int RunScript( const std::filesystem::path &scriptPath );

private:
	ScriptFile();
	~ScriptFile();

	void ThreadFunc( void );

	static unsigned int CALLBACK ScriptThread( void *pParam );

	std::string OpenScript( const std::filesystem::path &filePath );
	void TranslateScript( std::string strCmd );

	void TraceExecution( const char* msg, ... );
	void InitTrace( void );

#ifdef _WIN32
    HANDLE hScriptThread;
    unsigned int nScriptThreadId;
#else
	std::thread hScriptThread;
	std::thread::id nScriptThreadId;
#endif

    bool boScriptThreadDone;

	bool boTraceExecution;
	bool boSafeMode;
	bool boScriptIsRunning;
	bool boStopScriptExecution;
	bool boKeepScript;

	std::string strGodCharacter;
	
	std::string strScriptFilename;

};

#endif // !defined(AFX_SCRIPTFILE_H__1C5D1070_77F3_4C92_B3FE_143F922CF337__INCLUDED_)
