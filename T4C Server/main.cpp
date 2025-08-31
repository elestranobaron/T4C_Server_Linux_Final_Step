#include "TFC Server.h"
#include "TFCInit.h"
#include "TFC_MAIN.h"
#include "RegKeyHandler.h"
#include "TFCServerGP.h"
#include "expfltr.h"
#include "T4CLog.h"
#include "IntlText.h"
#include "AsyncFuncQueue.h"
#include "PacketManager.h"
#include "PlayerManager.h"
#include "TFCMessagesHandler.h"
#include "DeadlockDetector.h"
#include "AutoConfig.h"
#include "Game_Rules.h"
#include "Scheduler.h"
#include "Format.h"
#include "DynObjManager.h"
#include "Clans.h"
#include "SpellEffectManager.h"
#include "NPC Thread.h"
#include "MainConsole.h"
#include "SysopCmd.h"
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string>
#include <iostream>
#include "ODBCTrace.h"
#include "version.h"
#include "ThreadMonitor.h"
#include "System.h"
#include "ExitCode.h"
#include "random.h"
#include "TextFilter.h" //16/06/2009 special censoring filter for bad words

typedef struct _ASYNC_PACKET_FUNC_PARAMS{
    TFCPacket *msg;
    Players *user;
    RQ_SIZE rqRequestID;
} ASYNC_PACKET_FUNC_PARAMS, *LPASYNC_PACKET_FUNC_PARAMS;

typedef struct _RQSTRUCT_PUT_PLAYER_IN_GAME{
    ASYNC_PACKET_FUNC_PARAMS sParams;
    std::string csName;
} RQSTRUCT_PUT_PLAYER_IN_GAME, *LPRQSTRUCT_PUT_PLAYER_IN_GAME;

typedef struct _GPEpTn{
    LPEXCEPTION_POINTERS lpEP;
    std::string threadName;
} GPEpTn, *LPGPEpTn;

#pragma data_seg ("SHARED_INSTANCE")
BOOL boServerRunning = FALSE;
#pragma data_seg ()

#ifdef _DEBUG
#define DEADLOCK 5000000
#else
#define DEADLOCK 5000
#endif

//To choose between MYSQL and MSSQL for ODBC : (in order to optimize requests)
//#define MSSQLSERVER 0//BLBLBL 10/01/2011 (set to 1 if server is MSSQL, use TOP instead of LIMIT in SQL statments)
// ^^ d�fini dans les options de compilation ^^ 
#define VERSION_STRING "v%u"

#ifdef __ENABLE_LOG
DEBUG_LOG __LOG(200, 100); // Basic debug level of 100 entries and 25 entries
#endif

void ReportLastError(){
    std::cerr << "Error: " << strerror(errno) << std::endl;
}

void __cdecl EntryFunction(void *);
void __cdecl StopFunction(void *);
void TerminateServer( bool boExit );
void TFCInitMaps(void);
extern TFC_MAIN *TFCServer;
ODBCTrace *ODBCHarness;
extern Clans *CreatureClans;
static std::string ServerPath;

void MainPumpExceptionFunction(unsigned int u, EXCEPTION_POINTERS* pExp ){
    TFCException *excp = new TFCException;
    excp->SetException(pExp);
    throw excp;
}

LPTOP_LEVEL_EXCEPTION_FILTER lpDefaultExceptionFunc = NULL;
//unsigned MemDefaultPoolFlags = MEM_POOL_SERIALIZE;

void FindIPAddress(DWORD *ipaddr, short nArrayLen)
{
    char hostName[256];
    struct hostent *pHostEnt;
    if (gethostname(hostName, sizeof(hostName)) == -1 || !(pHostEnt = gethostbyname(hostName)))
    {
        ipaddr[0] = 0;
        return;
    }
    short i = 0;
    for (i=0 ; pHostEnt->h_addr_list[i] != '\0'; i++)
    {
        struct in_addr *ptr = (struct in_addr *)pHostEnt->h_addr_list[i];
        SOCKADDR_IN sa;
        sa.sin_addr = *ptr;
        if (i < nArrayLen) ipaddr[i] = sa.sin_addr.s_addr;
    }
    if (!i)
    {
        ipaddr[0] = 0;
        return;
    }
    struct in_addr *ptr = (struct in_addr *)pHostEnt->h_addr;
    SOCKADDR_IN sa;
    sa.sin_addr = *ptr;
    // overwrite position 0 address.. in case its different..
    ipaddr[0] = sa.sin_addr.s_addr;// leave it in network order..
    return;
}

inline void AddTrailingBackslash(std::string &csText)
{
    // Remove trailing white spaces
    csText.erase(0, csText.find_first_not_of(" \t"));
    // If string doesn't have a trailing backslash, add it.
    if( !csText.empty() && csText.back() != '/' ){
        csText += '/';
    }
}

/*BEGIN_MESSAGE_MAP(CTFCServerApp, CWinApp)
    //{{AFX_MSG_MAP(CTFCServerApp)
            // NOTE - the ClassWizard will add and remove mapping macros here.
            // DO NOT EDIT what you see in these blocks of generated code!
    //}}AFX_MSG
    ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()*/

CTFCServerApp::CTFCServerApp() : szServerAcctID( "T4CSVRID" )
{
    current_check = 1;
    last_PlayerMainDeadlockFlag = 0xFFFFFFFF;
    last_TFCMainDeadlockFlag = 0xFFFFFFFF;
    last_PlayerMainTime = 0;
    last_TFCMainTime = 0;
    hCrtReportFile = -1;
    serverStarted = false;
}

CTFCServerApp::~CTFCServerApp(){
    if( hCrtReportFile != -1 ){
        close(hCrtReportFile);
    }
}

#define _EXITLOG { std::ofstream f("/home/tll/T4C_Server_Linux_Final_Step/exit.log", std::ios::app); if(f) { f
#define EXITLOG_ }; f.close(); }

#define KEY2TXT( __str, __key, __default ) __str = regKey.GetProfileString( __key, __default );\
                                           __str.erase(__str.find_last_not_of(" \t") + 1);\
                                           __str.erase(0, __str.find_first_not_of(" \t"));

typedef struct _CONTEXT_FETCH{
    pthread_t hThread; // Thread
    LPCONTEXT lpContext; // Context holder
    BOOL boDone;
} CONTEXT_FETCH, *LPCONTEXT_FETCH;

UINT ContextFetch(LPVOID lpData)
{
    LPCONTEXT_FETCH lpFetch = (LPCONTEXT_FETCH)lpData;
    TRACE( "\r\nHandle=%u", lpFetch->hThread );
    TRACE( "\r\nHandle=%u", pthread_self() );
    // Sleep until asyn function fetched this thread's context.
    // Linux does not support direct thread suspension; placeholder
    lpFetch->boDone = TRUE;
    return 0;
}

unsigned int __stdcall ExceptionThread(void *exceptpandname)
{
    CAutoThreadMonitor tmMonitor("Exception Thread");
    LPGPEpTn crashInfo = static_cast<LPGPEpTn>(exceptpandname);
    _EXITLOG "\r\nException thread started." EXITLOG_
    try{
        _LOG_DEBUG
            LOG_CRIT_ERRORS,
            "Exception in thread %s",
            crashInfo->threadName.c_str()
        LOG_
        SUSPEND_DEADLOCK_DETECTION( 60000 * 3 );
        DebugLogger::GetInstance().Flush();
        END_LOG
    }catch(...){}
    TFCException *e = new TFCException;
    e->SetException(crashInfo->lpEP);
    _EXITLOG "\r\nWriting GP." EXITLOG_
    ReportGP(e, crashInfo->threadName.c_str());
    _EXITLOG "\r\nClosing exception thread.." EXITLOG_
    return EXCEPTION_CONTINUE_SEARCH;
}

LONG __stdcall DefaultExcpFilter(LPEXCEPTION_POINTERS lp)
{
    unsigned int threadId = 0;
    pthread_t threadHandle;
    static int exceptionCount = 0;
    struct tm sysTime;
    time_t now = time(NULL);
    localtime_r(&now, &sysTime);
    std::string threadName;
    if ( CThreadMonitor::GetInstance().GetThreadName(pthread_self(), threadName) == FALSE ) {
        threadName = "ID ";
        threadName += std::to_string(pthread_self());
        threadName += " [ Unnamed thread :( ]";
    }
    _EXITLOG "\r\n+++[EXCEPTION at 0x%x in thread %s] %u/%u/%u %02u:%02u:%02u +++",
        lp->ContextRecord->Eip,
        threadName.c_str(),
        sysTime.tm_mon + 1,
        sysTime.tm_mday,
        sysTime.tm_year + 1900,
        sysTime.tm_hour,
        sysTime.tm_min,
        sysTime.tm_sec
    EXITLOG_
    exceptionCount++;
    if( exceptionCount > 20 ){
        _EXITLOG "\r\nException count too high, killing process." EXITLOG_
        exit(134);
    }
    if( !TRYLOCK_GP ){
        sleep(2);
        _EXITLOG "\r\nException already being analyzed." EXITLOG_
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    GPEpTn crashInfo;
    crashInfo.lpEP = lp;
    crashInfo.threadName = threadName;
    threadHandle = pthread_self();
    std::thread excp_thread(ExceptionThread, &crashInfo);
    DWORD exitCode = EXCEPTION_CONTINUE_SEARCH;
    _EXITLOG "\r\nWaiting for exception thread to finish." EXITLOG_
    excp_thread.join();
    _EXITLOG "\r\nException handler returning." EXITLOG_
    return exitCode;
}

void unexpectedfunc( void ){
    _EXITLOG "\r\nGame terminated abruptly (unexcepted exception was thrown)." EXITLOG_
}

void exitfunc( void )
{
    struct tm sysTime;
    time_t now = time(NULL);
    localtime_r(&now, &sysTime);
    _EXITLOG "\r\n+++[EXIT] %u/%u/%u %02u:%02u:%02u +++",
        sysTime.tm_mon + 1,
        sysTime.tm_mday,
        sysTime.tm_year + 1900,
        sysTime.tm_hour,
        sysTime.tm_min,
        sysTime.tm_sec
    EXITLOG_
}

void display_usage(void)
{
    std::cerr << "You must start T4C as a service or specifically add\r\n"
                 "the -m command parameter if you wish to quickly test\r\n"
                 "the server's settings in a standard application window." << std::endl;
    exit(INVALID_PROGRAM_ARGUMENT);
}

void sigfunc(int s){
    _exit(SERVER_RECEIVED_SIGNAL);
}

int main(int argc, char **argv)
{
    bool startAsService = false;
    // Setup the signal handler(s).
    signal( SIGABRT, sigfunc );
    signal( SIGTERM, sigfunc );
    if( boServerRunning ){
        return ANOTHER_SERVER_ALREADY_RUNNING;
    }
    boServerRunning = TRUE;
    if (argc == 2) {
        if( 0 == strcmp(argv[1], "--spawn-service") ){
            startAsService = true;
            theApp.InService = true;
        } else if (0 == strcmp(argv[1], "-m")) {
            theApp.InService = false;
        } else {
            display_usage();
        }
    } else {
        theApp.InService = false;
    }
    if( startAsService ){
        pid_t pid = fork();
        if( pid < 0 ) exit(COULDNT_START_AS_A_SERVICE);
        if( pid > 0 ) exit(0);
        setsid();
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
        open("/dev/null", O_RDWR);
        open("/home/tll/T4C_Server_Linux_Final_Step/daemon.log", O_WRONLY | O_CREAT | O_APPEND, 0644);
        open("/home/tll/T4C_Server_Linux_Final_Step/daemon.err", O_WRONLY | O_CREAT | O_APPEND, 0644);
    }
    // Install the cleanup function.
    atexit( exitfunc );
    try{
        #ifdef __ENABLE_LOG
        if(__LOG > 0) __LOG("Entering InitInstance");
        #endif
        // Getting the machine name.
        theApp.csMachineName = System::GetMachineName().c_str();
        //CUService service;
        int startAsService = FALSE;
        int loop = 0;
        TCHAR param = 0;
        TCHAR path[PATH_MAX];
        char stop_char = ' ';
        // Removing an unused key for the registry ... why setting it ?
        {
            RegKeyHandler regKey;
            regKey.Open( 0, "SOFTWARE\\Vircom\\The 4th Coming Server" );
            regKey.DeleteValue( "SHUTDOWN" );
            regKey.Close();
        }
        char resolved_path[PATH_MAX];
        ssize_t len = readlink("/proc/self/exe", path, PATH_MAX - 1);
        if (len != -1) {
            path[len] = '\0';
            realpath(path, resolved_path);
        } else {
            strcpy(path, "/home/tll/T4C_Server_Linux_Final_Step/bin/");
        }
        loop = strlen(path);
        do{
            loop--;
        }while( path[ loop ] != '/' && loop >= 0 );
        path[ loop + 1 ] = 0;
        // Load the registry!
        RegKeyHandler regKey;
        // Paths.
        if( regKey.Open( 0, T4C_KEY PATHS_KEY ) ){
            theApp.sPaths.csLogPath = regKey.GetProfileString( "LOG_PATH", path );
            theApp.sPaths.csBinaryPath = regKey.GetProfileString( "BINARY_PATH", path );
            AddTrailingBackslash( theApp.sPaths.csLogPath );
            AddTrailingBackslash( theApp.sPaths.csBinaryPath );
            // Make certain that the log path exists.
            mkdir( theApp.sPaths.csLogPath.c_str(), 0755 );
            regKey.Close();
        }else{
            theApp.sPaths.csLogPath = path;
            theApp.sPaths.csBinaryPath = path;
            AddTrailingBackslash( theApp.sPaths.csLogPath );
            AddTrailingBackslash( theApp.sPaths.csBinaryPath );
        }
        ServerPath = theApp.sPaths.csBinaryPath;
        {
            RegKeyHandler regKey;
            regKey.Open( 0, T4C_KEY CONTACT_KEY );
            theApp.sContact.csCompanyName = regKey.GetProfileString( "CompanyName", "<unspecified>" );
            theApp.sContact.csAdminEmail = regKey.GetProfileString( "AdminEmail", "unknown@unspecified.net" );
        }
        {
            RegKeyHandler regKey;
            if( regKey.Open( 0, T4C_KEY ORACLE_HEART_BEAT ) ){
                theApp.sOracleHB.csDelay = static_cast< DWORD >( regKey.GetProfileInt( "Delay_Minutes", 2 ) );
                theApp.sOracleHB.csState = static_cast< DWORD >( regKey.GetProfileInt( "State", 0 ) );
            }else{
                theApp.sOracleHB.csDelay = 0;
                theApp.sOracleHB.csState = 0;
            }
        }
        {
            RegKeyHandler regKey;
            if( regKey.Open( 0, T4C_KEY GEN_CFG_KEY ) )
            {
                theApp.sColosseum = static_cast< DWORD >( regKey.GetProfileInt( "ACK_COLOSSEUM", 0 ) );
            }
            else
            {
                theApp.sColosseum = 0;
            }
            if( regKey.Open( 0, T4C_KEY GEN_CFG_KEY ) )
            {
                theApp.sDoppelganger = static_cast< DWORD >( regKey.GetProfileInt( "ACK_DOPPELGANGER", 0 ) );
            }
            else
            {
                theApp.sDoppelganger = 0;
            }
            if( regKey.Open( 0, T4C_KEY GEN_CFG_KEY ) )
            {
                theApp.sMaxRemorts = static_cast< DWORD >( regKey.GetProfileInt( "ACK_MAXREMORTS", 3 ) );
            }
            else
            {
                theApp.sMaxRemorts = 3;
            }
            if( regKey.Open( 0, T4C_KEY GEN_CFG_KEY ) )
            {
                theApp.sMaxCharactersPerAccount = static_cast< DWORD >( regKey.GetProfileInt( "MAX_CHARACTERS_PER_ACCOUNT", 3 ) );
                theApp.dwDebugSkillParryDisabled = regKey.GetProfileInt( "DEBUG_SKILLPARRY_DISABLED", 0 );
                theApp.dwLogXPGains = regKey.GetProfileInt( "ENABLE_XP_GAIN_LOGGING", 0 );
                theApp.dwHideUncoverEffectDisabled = regKey.GetProfileInt( "DisableHideUncoverEffect", 0 );
                theApp.dwRobWhileWalkingEnabled = regKey.GetProfileInt( "EnableRobWhileWalking", 0 );
                theApp.dwRobWhileBeingAttackedEnabled = regKey.GetProfileInt( "EnableRobWhileBeingAttacked", 0 );
                theApp.dwDisableIndirectDelete = regKey.GetProfileInt( "DisableIndirectDelete", 0 );
                theApp.dwDisableItemInfo = regKey.GetProfileInt( "DisableItemInfo", 0 );
                theApp.dwScriptLoopLength = regKey.GetProfileInt( "SCRIPT_LOOP", 0 );
            }
            else
            {
                theApp.sMaxCharactersPerAccount = 3;
                theApp.dwDebugSkillParryDisabled = 0;
                theApp.dwLogXPGains = 0;
                theApp.dwHideUncoverEffectDisabled = 0;
                theApp.dwRobWhileWalkingEnabled = 0;
                theApp.dwRobWhileBeingAttackedEnabled = 0;
                theApp.dwDisableIndirectDelete = 0;
                theApp.dwDisableItemInfo = 0;
                theApp.dwScriptLoopLength = 0;
            }
            if( regKey.Open( 0, T4C_KEY GEN_CFG_KEY ) )
            {
                theApp.sBattleMode = static_cast< DWORD >( regKey.GetProfileInt( "BATTLE_MODE", 0 ) );
            }
            else
            {
                theApp.sBattleMode = 0;
            }
        }
        //BATTLE MODE KEY BEGIN//
        {
            RegKeyHandler regKey;
            if( regKey.Open( 0, T4C_KEY CFG_BATTLE_MODE ) )
            {
                theApp.sxptm1killtm2 = static_cast< DWORD >( regKey.GetProfileInt( "XP_TM1KILLTM2", 0 ) );
                theApp.sgoldtm1killtm2 = static_cast< DWORD >( regKey.GetProfileInt( "GOLD_TM1KILLTM2", 0 ) );
                theApp.skarmatm1killtm2 = static_cast< DWORD >( regKey.GetProfileInt( "KARMA_TM1KILLTM2", 0 ) );
                theApp.sxptm2killtm1 = static_cast< DWORD >( regKey.GetProfileInt( "XP_TM2KILLTM1", 0 ) );
                theApp.sgoldtm2killtm1 = static_cast< DWORD >( regKey.GetProfileInt( "GOLD_TM2KILLTM1", 0 ) );
                theApp.skarmatm2killtm1 = static_cast< DWORD >( regKey.GetProfileInt( "KARMA_TM2KILLTM1", 0 ) );
                theApp.sxplostsameteam = static_cast< DWORD >( regKey.GetProfileInt( "XP_LOST_SAMETEAM", 0 ) );
                theApp.sgoldlostsameteam = static_cast< DWORD >( regKey.GetProfileInt( "GOLD_LOST_SAMETEAM", 0 ) );
                theApp.skarmagaintk = static_cast< DWORD >( regKey.GetProfileInt( "KARMA_GAIN_SAMETEAM", 0 ) );
                theApp.skarmalosttk = static_cast< DWORD >( regKey.GetProfileInt( "KARMA_LOST_SAMETEAM", 0 ) );
                theApp.sbonuskill1 = static_cast< DWORD >( regKey.GetProfileInt( "BONUS_KILL_1", 0 ) );
                theApp.sbonuskill2 = static_cast< DWORD >( regKey.GetProfileInt( "BONUS_KILL_2", 0 ) );
                theApp.sbonuskill3 = static_cast< DWORD >( regKey.GetProfileInt( "BONUS_KILL_3", 0 ) );
                theApp.sxpbonus1 = static_cast< DWORD >( regKey.GetProfileInt( "XP_BONUS1", 0 ) );
                theApp.sxpbonus2 = static_cast< DWORD >( regKey.GetProfileInt( "XP_BONUS2", 0 ) );
                theApp.sxpbonus3 = static_cast< DWORD >( regKey.GetProfileInt( "XP_BONUS3", 0 ) );
                theApp.sgoldbonus1 = static_cast< DWORD >( regKey.GetProfileInt( "GOLD_BONUS1", 0 ) );
                theApp.sgoldbonus2 = static_cast< DWORD >( regKey.GetProfileInt( "GOLD_BONUS2", 0 ) );
                theApp.sgoldbonus3 = static_cast< DWORD >( regKey.GetProfileInt( "GOLD_BONUS3", 0 ) );
            }
            else
            {
                theApp.sxptm1killtm2 = 0;
                theApp.sgoldtm1killtm2 = 0;
                theApp.skarmatm1killtm2 = 0;
                theApp.sxptm2killtm1 = 0;
                theApp.sgoldtm2killtm1 = 0;
                theApp.skarmatm2killtm1 = 0;
                theApp.sxplostsameteam = 0;
                theApp.sgoldlostsameteam = 0;
                theApp.skarmagaintk = 0;
                theApp.skarmalosttk = 0;
                theApp.sbonuskill1 = 0;
                theApp.sbonuskill2 = 0;
                theApp.sbonuskill3 = 0;
                theApp.sxpbonus1 = 0;
                theApp.sxpbonus2 = 0;
                theApp.sxpbonus3 = 0;
                theApp.sgoldbonus1 = 0;
                theApp.sgoldbonus2 = 0;
                theApp.sgoldbonus3 = 0;
            }
        }
        {
            std::string strFile = fmt::format("{}/KeyWorld.dat", ServerPath);
            FILE *pf = fopen(strFile.c_str(), "rt");
            if(pf)
            {
                char *pRet;
                char strLigne[2048];
                do
                {
                    pRet = fgets(strLigne, 2048, pf);
                    if(pRet)
                    {
                        sMagicWorldSpell newWord;
                        char strTmp[2048];
                        int dwNbr = sscanf(strLigne, "%010d,%010d,%s", &newWord.uiFlagID, &newWord.uiSpellID, strTmp);
                        if(dwNbr == 3)
                        {
                            newWord.strText = fmt::format("%s", strTmp);
                            theApp.m_aSpellWorld.Add(newWord);
                        }
                    }
                }while(pRet);
                fclose(pf);
            }
            //sMagicWorldSpell nWorld;
            //nWorld.strText.Format("batard");
            //nWorld.uiSpellID = 10115;
            //nWorld.uiFlagID = 0;
            //theApp.m_aSpellWorld.Add(nWorld);
        }
        //BATTLE MODE KEY END//
        regKey.Open(0, T4C_KEY "Authentication\\");
        theApp.csDBDns = regKey.GetProfileString("ODBC_DSN", "");
        regKey.Open( 0, T4C_KEY CHARACTER_KEY );
        theApp.csDBUser = regKey.GetProfileString( "DB_USER", "" );
        theApp.csDBPwd = regKey.GetProfileString( "DB_PWD", "" );
        theApp.dwDeadSpellID = regKey.GetProfileInt( "DeadSpellID", 0x00 ); //DEATH_EFFECT_ID
        if ( theApp.dwCustomStartupPositionOnOff = regKey.GetProfileInt( "StartupPosOnOff", FALSE ) ) {
            theApp.dwCustomStartupPositionX = regKey.GetProfileInt( "StartupPosX", NULL );
            theApp.dwCustomStartupPositionY = regKey.GetProfileInt( "StartupPosY", NULL );
            theApp.dwCustomStartupPositionW = regKey.GetProfileInt( "StartupPosW", NULL );
        }
        if ( theApp.dwCustomStartupSanctuaryOnOff = regKey.GetProfileInt( "StartupSanctuaryOnOff", FALSE ) ) {
            theApp.dwCustomStartupSanctuaryX = regKey.GetProfileInt( "StartupSanctuaryX", NULL );
            theApp.dwCustomStartupSanctuaryY = regKey.GetProfileInt( "StartupSanctuaryY", NULL );
            theApp.dwCustomStartupSanctuaryW = regKey.GetProfileInt( "StartupSanctuaryW", NULL );
        }
        theApp.dwChestEncumbranceUpdatedLive = regKey.GetProfileInt( "ChestEncumbranceUpdatedLive", 0);
        theApp.csChestEncumbranceBoostFormula = regKey.GetProfileString( "ChestEncumbranceBoostFormula", "" );
        //NMNMNM Still Item ID
        TFormat format;
        int i = 1;
        DWORD dwID = static_cast< DWORD >( regKey.GetProfileInt( "StillItem1", 0 ) );
        while(dwID != 0 )
        {
            theApp.m_aStillItems.Add(dwID);
            i++;
            dwID = static_cast< DWORD >( regKey.GetProfileInt( format( "StillItem%u", i ), 0 ) );
        }
        regKey.Close();
        // ArenaLocations
        if( regKey.Open( 0, T4C_KEY "PVPDeath" ) )
        {
            theApp.arenaLocationList.clear();
            TFormat format;
            int i = 1;
            std::string locationId = regKey.GetProfileString( "ArenaLocation1", "$NULL$" );
            while( locationId != "$NULL$" )
            {
                sArenaLocation loc;
                loc.wlTopLeft.X = regKey.GetProfileInt( format( "ArenaLocation%uX1", i ), 0 );
                loc.wlTopLeft.Y = regKey.GetProfileInt( format( "ArenaLocation%uY1", i ), 0 );
                loc.wlBottomRight.X = regKey.GetProfileInt( format( "ArenaLocation%uX2", i ), 0 );
                loc.wlBottomRight.Y = regKey.GetProfileInt( format( "ArenaLocation%uY2", i ), 0 );
                loc.wlBottomRight.world = loc.wlTopLeft.world = regKey.GetProfileInt( format( "ArenaLocation%uWorld", i ), 0 );
                loc.wlRecallAttacker.X = regKey.GetProfileInt( format( "ArenaLocation%uRecallKillX", i ), -1 );
                loc.wlRecallAttacker.Y = regKey.GetProfileInt( format( "ArenaLocation%uRecallKillY", i ), -1 );
                loc.wlRecallAttacker.world = regKey.GetProfileInt( format( "ArenaLocation%uRecallKillW", i ), 0 );
                loc.wlRecallTarget.X = regKey.GetProfileInt( format( "ArenaLocation%uRecallDieX", i ), 0 );
                loc.wlRecallTarget.Y = regKey.GetProfileInt( format( "ArenaLocation%uRecallDieY", i ), 0 );
                loc.wlRecallTarget.world = regKey.GetProfileInt( format( "ArenaLocation%uRecallDieW", i ), 0 );
                theApp.arenaLocationList.push_back( loc );
                ++i;
                locationId = regKey.GetProfileString( format( "ArenaLocation%u", i ), "$NULL$" );
            }
            regKey.Close();
        }
        START_LOG;
        // Those lines got moved from below so that it starts the ODBCTrace
        // Before the first call to the logging functions.
        // This way we can make logging functions log to SQL as well as TXT files.
        try{
            ODBCHarness = new ODBCTrace;
            _LOG_DEBUG
                LOG_CRIT_ERRORS,
                "Creating ODBCTrace object."
            LOG_
        }catch(...){
            _LOG_DEBUG
                LOG_CRIT_ERRORS,
                "Crashed when creating ODBCTrace object."
            LOG_
            throw;
        }
        // End of moved lines
        _LOG_DEBUG LOG_ALWAYS, "-----" LOG_
        _LOG_DEBUG LOG_ALWAYS, "Starting T4C server." LOG_
        _LOG_DEBUG LOG_ALWAYS, Version::sBuildStamp.c_str() LOG_
        if ( theApp.sOracleHB.csState ){
            _LOG_DEBUG LOG_DEBUG_LVL4, "Oracle Heart Beat Delay: %u",theApp.sOracleHB.csDelay LOG_
            _LOG_DEBUG LOG_DEBUG_LVL4, "Oracle Heart Beat State: %u",theApp.sOracleHB.csState LOG_
        }
        _LOG_DEATH LOG_ALWAYS, "-----" LOG_
        _LOG_DEATH LOG_ALWAYS, "Starting T4C server." LOG_
        _LOG_GAMEOP LOG_ALWAYS, "-----" LOG_
        _LOG_GAMEOP LOG_ALWAYS, "Starting T4C server." LOG_
        _LOG_PC LOG_ALWAYS, "-----" LOG_
        _LOG_PC LOG_ALWAYS, "Starting T4C server." LOG_
        _LOG_ITEMS LOG_ALWAYS, "-----" LOG_
        _LOG_ITEMS LOG_ALWAYS, "Starting T4C server." LOG_
        _LOG_NPCS LOG_ALWAYS, "-----" LOG_
        _LOG_NPCS LOG_ALWAYS, "Starting T4C server." LOG_
        _LOG_WORLD LOG_ALWAYS, "-----" LOG_
        _LOG_WORLD LOG_ALWAYS, "Starting T4C server." LOG_
        std::string csSource;
        // Network ///////////////////////////////////////////////////////////////////////////////
        if( regKey.Open( 0, T4C_KEY NETWORK_KEY ) ){
            theApp.sNetwork.csRecvIP = regKey.GetProfileString( "RECV_IP", "" );
            theApp.sNetwork.csSendIP = regKey.GetProfileString( "SEND_IP", theApp.sNetwork.csRecvIP.c_str() );
            theApp.sNetwork.csRecvIP.erase(theApp.sNetwork.csRecvIP.find_last_not_of(" \t") + 1);
            theApp.sNetwork.csRecvIP.erase(0, theApp.sNetwork.csRecvIP.find_first_not_of(" \t"));
            theApp.sNetwork.csSendIP.erase(theApp.sNetwork.csSendIP.find_last_not_of(" \t") + 1);
            theApp.sNetwork.csSendIP.erase(0, theApp.sNetwork.csSendIP.find_first_not_of(" \t"));
            theApp.sNetwork.wRecvPort = static_cast< WORD >( regKey.GetProfileInt( "RECV_PORT", 11677 ) );
            theApp.sNetwork.wSendPort = static_cast< WORD >( regKey.GetProfileInt( "SEND_PORT", 11677 ) );
            regKey.Close();
        }else{
            theApp.sNetwork.wRecvPort = 11677;
            theApp.sNetwork.wSendPort = 11678;
            _LOG_DEBUG
                LOG_CRIT_ERRORS,
                "\n Could not open INI key " T4C_KEY NETWORK_KEY " required by T4C Server."
                "\n Run T4C Server Setup to setup the INI keys."
                "\n Using default values."
            LOG_
        }
        // General Configuration /////////////////////////////////////////////////////////////////
        if( regKey.Open( 0, T4C_KEY GEN_CFG_KEY ) ){
            theApp.sGeneral.wNbWarnings = (WORD)regKey.GetProfileInt( "NB_WARNINGS", 5 );
            theApp.sGeneral.dwTimeBeforeWarning = regKey.GetProfileInt( "TIME_BEFORE_WARNING", 2500 );
            TFormat format;
            int nCount = 1;
            std::string csLangDB = regKey.GetProfileString( "LangDB1", "$NULL$" );
            while( csLangDB != "$NULL$" ){
                IntlText::LoadLngDB( csLangDB.c_str() );
                theApp.sGeneral.csLang = csLangDB;
                csLangDB = regKey.GetProfileString( format( "LangDB%u", ++nCount ), "$NULL$" );
            }
            // fill in the theApp.sGeneral.csLang field //DC
            // use: if ( theApp.sGeneral.csLang == "t4c_kor.elng" ) for korean specific actions
            // If the language configuration isn't valid.
            if( !IntlText::IsLngOK() ){
                _LOG_DEBUG
                    LOG_CRIT_ERRORS,
                    "Could not find any valid language database. Server cannot be started."
                LOG_
                END_LOG
                std::cerr << "Could not find any valid language database. Server cannot be started." << std::endl;
                return false;
            }
            /*if( regKey.GetProfileInt( "T4C_CRASH", 0 ) != 0 ){
                // Delete the key.
                regKey.DeleteValue( "T4C_CRASH" );
                // If the privacy key is set, do not send email.
                if( regKey.GetProfileInt( "PRIVACY", 0 ) == 0 ){
                    // Get the system time
                    struct tm sysTime;
                    time_t now = time(NULL);
                    localtime_r(&now, &sysTime);
                    try{
                        std::string csGPfile = theApp.sPaths.csLogPath + "T4CServerGP.out";
                        std::string domain;
                        std::string mainText;
                        {
                            char hostName[ 256 ];
                            gethostname( hostName, 256 );
                            domain = hostName;
                            in_addr mainIP;
                            DWORD dwVersion = SERVER_CONNECTION_HI_VERSION;
                            {
                                RegKeyHandler regKey;
                                regKey.Open( 0, "Software\\Vircom\\The 4th Coming Server\\GeneralConfig" );
                                // Fetch version from registry, otherwise default to the executable's version.
                                dwVersion = regKey.GetProfileInt( "Version", SERVER_CONNECTION_HI_VERSION );
                                regKey.Close();
                            }
                            FindIPAddress( &mainIP.s_addr, 1 );
                            RegKeyHandler key;
                            key.Open( 0, "System\\CurrentControlSet\\Services\\Tcpip\\Parameters" );
                            domain += ".";
                            domain += key.GetProfileString( "domain", "" );
                            mainText = "GP fault.";
                            mainText += "\r\n";
                            mainText += fmt::format( "\r\nServer version: v%u.", dwVersion );
                            mainText += fmt::format( "\r\nCompany name: %s",
                                            theApp.sContact.csCompanyName.c_str()
                                        );
                            mainText += fmt::format( "\r\nAdministrator Email: %s",
                                            theApp.sContact.csAdminEmail.c_str()
                                        );
                            mainText += fmt::format(
                                            "\r\nComing from address %s, host %s",
                                            inet_ntoa( mainIP ),
                                            domain.c_str()
                                        );
                            key.Close();
                            std::string license;
                            key.Open( 0, "Software\\Vircom\\The 4th Coming Server" );
                            LPCTSTR szLicense = key.GetProfileString( "License", "" );
                            if( strlen( szLicense ) >= 22 ){
                                license = &szLicense[ 22 ];
                            }else{
                                license = "<invalid>";
                            }
                            mainText += fmt::format(
                                            "\r\nClient ID: %s",
                                            license.c_str()
                                        );
                        }
                        std::string deadlockFile( theApp.sPaths.csLogPath );
                        deadlockFile += "deadlock.txt";
                        std::string heavyDbg = theApp.sPaths.csLogPath;
                        heavyDbg += "heavydbg.txt";
                        std::string exitLog = theApp.sPaths.csLogPath;
                        exitLog += "exit.txt";
                        std::list< const char * > extraFileList;
                        extraFileList.push_back( deadlockFile.c_str() );
                        extraFileList.push_back( heavyDbg.c_str() );
                        extraFileList.push_back( exitLog.c_str() );
                        /*EasyMail()(
                            "smtp.vircom.com",
                            theApp.sContact.csAdminEmail.c_str(),
                            "t4ccrash@the4thcoming.com",
                            fmt::format(
                                "T4C crash on %s (%s) at %02u/%02u/%04u %02u:%02u:%02u",
                                theApp.sContact.csCompanyName.c_str(),
                                domain.c_str(),
                                sysTime.tm_mon + 1,
                                sysTime.tm_mday,
                                sysTime.tm_year + 1900,
                                sysTime.tm_hour,
                                sysTime.tm_min,
                                sysTime.tm_sec
                            ),
                            mainText.c_str(),
                            csGPfile.c_str(),
                            "crashreports@the4thcoming.com",
                            "p0rt1cule",
                            &extraFileList
                        );*/
                        std::string newGPfile( csGPfile );
                        // Append date/time to GP.
                        newGPfile += fmt::format(
                                ".%u%u%u.%02u%02u%04u.bck",
                                sysTime.tm_sec,
                                sysTime.tm_min,
                                sysTime.tm_hour,
                                sysTime.tm_mday,
                                sysTime.tm_mon + 1,
                                sysTime.tm_year + 1900
                         );
                        unlink( deadlockFile.c_str() );
                        unlink( heavyDbg.c_str() );
                        unlink( exitLog.c_str() );
                        // Rename GP.
                        rename( csGPfile.c_str(), newGPfile.c_str() );
                    }catch(...){ 
                        _LOG_DEBUG
                            LOG_DEBUG_HIGH,
                            "Crashed initializing emails."
                        LOG_
                    }
                }
            }*/
            regKey.Close();
        }else{
            theApp.sGeneral.wNbWarnings = 5;
            theApp.sGeneral.dwTimeBeforeWarning = 2500;
            _LOG_DEBUG
                LOG_CRIT_ERRORS,
                "\n Could not open INI key " T4C_KEY GEN_CFG_KEY " required by T4C Server."
                "\n Run T4C Server Setup to setup the INI keys."
                "\n Using default values."
            LOG_
        }
        {
            struct tm sysTime;
            time_t now = time(NULL);
            localtime_r(&now, &sysTime);
            _EXITLOG "\r\n+++[STARTUP] %u/%u/%u %02u:%02u:%02u +++",
                sysTime.tm_mon + 1,
                sysTime.tm_mday,
                sysTime.tm_year + 1900,
                sysTime.tm_hour,
                sysTime.tm_min,
                sysTime.tm_sec
            EXITLOG_
        }
        // Authentification //////////////////////////////////////////////////////////////////////
        if( regKey.Open( 0, T4C_KEY AUTH_KEY ) )
        {
            theApp.sAuth.bAuthentificationMethod = (BYTE)regKey.GetProfileInt( "AuthenticationMethod", ODBC_AUTH );
            /* if( theApp.sAuth.bAuthentificationMethod == GOA_AUTH )
                        {
                if( !InitGoaAuth() )
                                {
                    _LOG_DEBUG
                        LOG_CRIT_ERRORS,
                        "GOA authentication specified but could not be initialized."
                    LOG_
                    return false;
                }
            }*/
        }
        return true;
    }catch(...){ return false; }
}
