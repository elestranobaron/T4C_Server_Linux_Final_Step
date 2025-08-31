#include "TFC Server.h"
#include "ODBCMage.h"
#include "TFCServerGP.h"
#include <thread>
#include <mysql.h>
#include <string>
#include <iostream>
#include "DeadlockDetector.h"
#include "RegKeyHandler.h"
#include "DebugLogger.h"
#include "format.h"
//#include "EasyMail.h"
#include "Shutdown.h"
#include "ThreadMonitor.h"

#define TRANSACT_SQL
#define CURSOR_SQL 0

extern CTFCServerApp theApp;

void cODBCMage::CheckDisconnectError( void ){
    BYTE lpbSQLState[ 6 ] = {0};
    BYTE lpbErrorMsg[ 200 ];
    strcpy((char*)lpbErrorMsg, mysql_error(mysql));
    long dwNativeError = mysql_errno(mysql);
    if( stricmp( (const char *)lpbSQLState, "08S01" ) == 0 ||
        stricmp( (const char *)lpbSQLState, "08001" ) == 0 ||
        stricmp( (const char *)lpbSQLState, "08003" ) == 0 ||
        stricmp( (const char *)lpbSQLState, "08004" ) == 0 ){
        static bool shutdown = false;
        if( !shutdown ){
            shutdown = true;
            // Add the shutdown key to make sure the process won't start
            // until 5 minutes.
            RegKeyHandler regKey;
            regKey.Open( 0, "SOFTWARE\\Vircom\\The 4th Coming Server" );
            regKey.WriteProfileInt( "SHUTDOWN", 1 );
            regKey.Close();
            _LOG_DEBUG
                LOG_CRIT_ERRORS,
                "T4C was shutdown because the MySQL connection is down."
            LOG_
            std::string body;
            body = fmt::format(
                "T4C was unable to establish a connection with the character database."
                "\r\nThe server cannot run without a character database and has therefore"
                "\r\nbeen shutdown for a minimum period of 5 minutes."
                "\r\nPlease verify your setup and correct the situation as soon as possible."
                "\r\nSQL Error:"
                "\r\nSQL State = %s"
                "\r\nNative error = %u"
                "\r\nError Message = %s",
                theApp.sContact.csCompanyName.c_str(),
                lpbSQLState,
                dwNativeError,
                lpbErrorMsg
            );
            _LOG_DEBUG
                LOG_CRIT_ERRORS,
                body.c_str()
            LOG_
            std::cerr << body << std::endl;
            CShutdown::CreateShutdown( SHUTDOWN_NOW, true, true );
            CShutdown::StartShutdown();
        }
    }
};

CLock cODBCMage::cStaticLock;
bool cODBCMage::boSerialize = true;
cODBCMage::StaticInit cODBCMage::m_StaticInit;
cODBCMage::StaticInit::StaticInit( void ){
    RegKeyHandler regKey;
    regKey.Open( 0, T4C_KEY GEN_CFG_KEY );
    if( regKey.GetProfileInt( "SerializeODBC", 1 ) != 0 ){
        boSerialize = true;
    }else{
        boSerialize = false;
    }
}

cODBCMage::cODBCMage()
{
    Create();
    ZeroMemory( lpszCurrentDSN, 20 );
    ZeroMemory( lpszCurrentUser, 20 );
    ZeroMemory( lpszCurrentPWD, 20 );
    boShutdown = FALSE;
    boStmtAlloc = false;
    mysql = mysql_init(NULL);
}

cODBCMage::~cODBCMage()
{
    Destroy();
}

void cODBCMage::Lock()
{
    /*TFormat format;
    DebugLogger::GetInstance().LogString(
        format(
            "Trying locking MySQL at %s(%u). (0x%x Thread %u)",
            lpszFileName,
            nLineNumber,
            &cStaticLock,
            std::this_thread::get_id()
        )
    );*/
    cStaticLock.Lock();
}

void cODBCMage::Unlock()
{
    /*TFormat format;
    DebugLogger::GetInstance().LogString(
        format(
            "Unlocked MySQL at %s(%u). (0x%x Thread %u)",
            lpszFileName,
            nLineNumber,
            &cStaticLock,
            std::this_thread::get_id()
        )
    );*/
    cStaticLock.Unlock();
}

void cODBCMage::Create()
{
    mysql = mysql_init(NULL);
}

void cODBCMage::Destroy()
{
    if (mysql) mysql_close(mysql);
}

void cODBCMage::Connect(LPCSTR szDataSource, LPCSTR szUsername, LPCSTR szPassword)
{
    std::cerr << "\r\nTrying to connect to DSN " << szDataSource << std::endl;
    std::cerr << "\r\nTrying to connect to DSN " << lpszCurrentDSN << std::endl;
    Lock();
    if( szDataSource != NULL ){
        strcpy( lpszCurrentDSN, szDataSource );
    }else{
        strcpy( lpszCurrentDSN, "DEFAULT" );
    }
    std::cerr << "\r\nTrying to connect to DSN " << lpszCurrentDSN << std::endl;
    std::cerr << "\r\nTrying to connect to DSN " << szDataSource << std::endl;
    if( szUsername != NULL ){
        strcpy( lpszCurrentUser, szUsername );
    }else{
        lpszCurrentUser[ 0 ] = 0;
    }
    if( szPassword != NULL ){
        strcpy( lpszCurrentPWD, szPassword );
    }else{
        lpszCurrentPWD[ 0 ] = 0;
    }
    if (!mysql_real_connect(mysql, "localhost", szUsername, szPassword, szDataSource, 3306, NULL, 0)) {
        std::cerr << "MySQL ERROR [SQLConnect]: " << mysql_error(mysql) << std::endl;
        CheckDisconnectError();
        _LOG_DEBUG
            LOG_DEBUG_LVL3,
            "SQL ERROR [SQLConnect]:"
        LOG_
        _LOG_DEBUG
            LOG_DEBUG_LVL3,
            "SQL State: %s",
            mysql_error(mysql)
        LOG_
        _LOG_DEBUG
            LOG_DEBUG_LVL3,
            "Native Error: %u",
            mysql_errno(mysql)
        LOG_
        _LOG_DEBUG
            LOG_DEBUG_LVL3,
            "Error message: %s",
            mysql_error(mysql)
        LOG_
    }
    Unlock();
}

void cODBCMage::AllocStmt()
{
    Lock();
    if( !boStmtAlloc ){
        boStmtAlloc = true;
    }
    Unlock();
}

void cODBCMage::Disconnect()
{
    Lock();
    if( boStmtAlloc ){
        boStmtAlloc = false;
    }
    mysql_close(mysql);
    mysql = mysql_init(NULL);
    Unlock();
}

BOOL cODBCMage::ExecuteRequest(LPCSTR szField, LPCSTR szTablename, LPCSTR szWhereStmt)
{
    CHAR szBuffer[1024];
    sprintf(szBuffer, "SELECT %s FROM %s %s", szField, szTablename, szWhereStmt);
    if (mysql_query(mysql, szBuffer)) {
        std::cerr << "MySQL ERROR: " << mysql_error(mysql) << std::endl;
        return FALSE;
    }
    MYSQL_RES* res = mysql_store_result(mysql);
    if (res) {
        mysql_free_result(res);
        return TRUE;
    }
    return FALSE;
}

void cODBCMage::ConnectOption(WORD wConnectOption, DWORD dwConnectValue)
{
    // MySQL does not use connect options like ODBC
}

int cODBCMage::Commit()
{
    if (!mysql_commit(mysql)) return TRUE;
    std::cerr << "MySQL COMMIT ERROR: " << mysql_error(mysql) << std::endl;
    return FALSE;
}

BOOL cODBCMage::Rollback()
{
    if (!mysql_rollback(mysql)) return TRUE;
    std::cerr << "MySQL ROLLBACK ERROR: " << mysql_error(mysql) << std::endl;
    return FALSE;
}

BOOL cODBCMage::Fetch()
{
    // Placeholder; MySQL fetching needs result set handling
    return FALSE;
}

BOOL cODBCMage::Cancel()
{
    // MySQL does not support SQLCancel; placeholder
    return FALSE;
}

BOOL cODBCMage::CloseCursor()
{
    // MySQL does not use cursors; placeholder
    return TRUE;
}

BOOL cODBCMage::SendRequest(LPCTSTR lpszRequest, bool boKnownError)
{
    if (mysql_query(mysql, lpszRequest)) {
        std::cerr << "MySQL ERROR [SQLExecDirect]: " << mysql_error(mysql) << std::endl;
        _LOG_DEBUG
            LOG_DEBUG_LVL1,
            "SQL ERROR [SQLExecDirect]:"
        LOG_
        _LOG_DEBUG
            LOG_DEBUG_LVL2,
            "Original SQL query statement: [ %s ]",
            lpszRequest
        LOG_
        _LOG_DEBUG
            LOG_DEBUG_LVL2,
            "SQL State: %s",
            mysql_error(mysql)
        LOG_
        _LOG_DEBUG
            LOG_DEBUG_LVL2,
            "Native Error: %u",
            mysql_errno(mysql)
        LOG_
        _LOG_DEBUG
            LOG_DEBUG_LVL2,
            "Error message: %s",
            mysql_error(mysql)
        LOG_
        if (boKnownError) return FALSE;
        if (mysql_query(mysql, lpszRequest)) {
            std::cerr << "MySQL RETRY ERROR: " << mysql_error(mysql) << std::endl;
            _LOG_DEBUG
                LOG_WARNING,
                "SQLExecDirect failed, trying to re-send the request."
            LOG_
            return FALSE;
        }
        _LOG_DEBUG
            LOG_WARNING,
            "SQLExecDirect failed, re-sending succeed."
        LOG_
    }
    return TRUE;
}

RETCODE cODBCMage::GetDWORD(UWORD uwCol, LPDWORD lpdwFetch)
{
    // Placeholder; needs MySQL result set handling
    *lpdwFetch = 0;
    std::cerr << "MySQL ERROR [SQLGetData]: Not implemented" << std::endl;
    return SQL_ERROR;
}

RETCODE cODBCMage::GetSDWORD(UWORD uwCol, long *lpdwFetch)
{
    // Placeholder
    *lpdwFetch = 0;
    std::cerr << "MySQL ERROR [SQLGetData]: Not implemented" << std::endl;
    return SQL_ERROR;
}

RETCODE cODBCMage::GetWORD(UWORD uwCol, LPWORD lpwFetch)
{
    // Placeholder
    std::cerr << "MySQL ERROR [SQLGetData]: Not implemented" << std::endl;
    return SQL_ERROR;
}

RETCODE cODBCMage::GetSWORD(UWORD uwCol, short *lpwFetch)
{
    // Placeholder
    std::cerr << "MySQL ERROR [SQLGetData]: Not implemented" << std::endl;
    return SQL_ERROR;
}

RETCODE cODBCMage::GetBYTE(UWORD uwCol, LPBYTE lpbFetch)
{
    // Placeholder
    *lpbFetch = 0;
    std::cerr << "MySQL ERROR [SQLGetData]: Not implemented" << std::endl;
    return SQL_ERROR;
}

RETCODE cODBCMage::GetBLOB(UWORD uwCol, LPVOID *lppData, LPDWORD lpdwSize)
{
    // Placeholder
    *lppData = NULL;
    *lpdwSize = 0;
    std::cerr << "MySQL ERROR [SQLGetData]: Not implemented" << std::endl;
    return SQL_ERROR;
}

RETCODE cODBCMage::GetString(UWORD uwCol, LPTSTR lpszText, int nMaxSize)
{
    // Placeholder
    lpszText[0] = 0;
    std::cerr << "MySQL ERROR [SQLGetData]: Not implemented" << std::endl;
    return SQL_ERROR;
}

RETCODE cODBCMage::GetDouble(UWORD uwCol, double *lpdblFetch)
{
    // Placeholder
    std::cerr << "MySQL ERROR [SQLGetData]: Not implemented" << std::endl;
    return SQL_ERROR;
}

RETCODE cODBCMage::GetDate(UWORD uwCol, SQL_DATE_STRUCT *lpSqlDate)
{
    // Placeholder
    std::cerr << "MySQL ERROR [SQLGetData]: Not implemented" << std::endl;
    return SQL_ERROR;
}

void cODBCMage::InitializeWriteThread()
{
    if( !write_thread.joinable() ){
        write_thread = std::thread(&cODBCMage::ODBCWriteThread, this, this);
    }
}

void cODBCMage::SendBatchRequest(TemplateList<SQL_REQUEST> *lptlRequests, SQLTERMINATION lpTerminationCallback, LPVOID lpData, LPTSTR lpszText)
{
    // Placeholder; threading model needs rework
}

void cODBCMage::WaitForODBCShutdown()
{
    boShutdown = TRUE;
    if( write_thread.joinable() ) write_thread.join();
}

void cODBCMage::ODBCWriteThread(LPVOID lpODBCObject)
{
    // Placeholder; threading model needs rework
    CAutoThreadMonitor tmMonitor("ODBC Write Thread");
    _LOG_DEBUG
        LOG_DEBUG_LVL1,
        "ODBCWriteThread Id=%u",
        std::this_thread::get_id()
    LOG_
}