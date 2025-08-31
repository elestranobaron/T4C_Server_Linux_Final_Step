#include "RegKeyHandler.h"
#include <string>
#include <fstream>
#include <iostream>
#include <cstring>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new new
#endif
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
RegKeyHandler::RegKeyHandler()
{ keyhandle = NULL;
}
RegKeyHandler::~RegKeyHandler(){
if(keyhandle != NULL) fclose((FILE*)keyhandle);
}
////////////////////////////////////////////////////////////////////////////
BOOL RegKeyHandler::Create(HKEY main_key, LPCTSTR sub_key){
        std::string ini_file = "/home/tll/T4C_Server_Linux_Final_Step/config.ini";
        FILE* fp = fopen(ini_file.c_str(), "a"); // Create or append
        if (!fp) return FALSE;
        keyhandle = fp;
        mainkey = main_key;
        subkey = sub_key;
        return TRUE;
}
///////////////////////////////////////////////////////////////////////////////
BOOL RegKeyHandler::Open(HKEY main_key, LPCTSTR sub_key){
        DWORD err;
        if( keyhandle != NULL ){
                fclose((FILE*)keyhandle);
        }
        std::string ini_file = "/home/tll/T4C_Server_Linux_Final_Step/config.ini";
        FILE* fp = fopen(ini_file.c_str(), "r+");
        if (!fp) {
                #ifdef _DEBUG
                std::cerr << "\r\nError 0x" << std::hex << err << "(" << std::dec << err << ") opening INI file " << sub_key << "." << std::endl;
                #endif
                return FALSE;
        }
        keyhandle = fp;
        mainkey = main_key;
        subkey = sub_key;
        return TRUE;
}
////////////////////////////////////////////////////////////////////////////
void RegKeyHandler::WriteProfileString(LPCTSTR item, LPCTSTR value){
        std::ofstream out("/home/tll/T4C_Server_Linux_Final_Step/config.ini", std::ios::app);
        out << item << "=" << value << "\n";
        out.close();
};
////////////////////////////////////////////////////////////////////////////
void RegKeyHandler::WriteProfileInt(LPCTSTR item, DWORD value){
        std::ofstream out("/home/tll/T4C_Server_Linux_Final_Step/config.ini", std::ios::app);
        out << item << "=" << value << "\n";
        out.close();
}
////////////////////////////////////////////////////////////////////////////
LPCTSTR RegKeyHandler::GetProfileString(LPCTSTR item, LPCTSTR default_arg){
        std::ifstream in("/home/tll/T4C_Server_Linux_Final_Step/config.ini");
        std::string line, key, val;
        returnstr[ 0 ] = 0;
        while (std::getline(in, line)) {
                size_t pos = line.find('=');
                if (pos != std::string::npos) {
                        key = line.substr(0, pos);
                        val = line.substr(pos + 1);
                        if (key == item) {
                                strcpy(returnstr, val.c_str());
                                in.close();
                                return returnstr;
                        }
                }
        }
        in.close();
        strcpy(returnstr, default_arg);
        return returnstr;
}
////////////////////////////////////////////////////////////////////////////////
DWORD RegKeyHandler::GetProfileInt(LPCTSTR item, DWORD default_arg)
{
        std::ifstream in("/home/tll/T4C_Server_Linux_Final_Step/config.ini");
        std::string line, key, val;
        while (std::getline(in, line)) {
                size_t pos = line.find('=');
                if (pos != std::string::npos) {
                        key = line.substr(0, pos);
                        val = line.substr(pos + 1);
                        if (key == item) {
                                in.close();
                                try { return std::stoul(val); } catch (...) { return default_arg; }
                        }
                }
        }
        in.close();
        return default_arg;
}
//////////////////////////////////////////////////////////////////////////////////////////
void RegKeyHandler::Close( void ){
        if(keyhandle != NULL) fclose((FILE*)keyhandle);
        keyhandle = NULL;
}
//////////////////////////////////////////////////////////////////////////////////////////
BOOL RegKeyHandler::DeleteValue( LPCTSTR lpszItem ){
        std::ifstream in("/home/tll/T4C_Server_Linux_Final_Step/config.ini");
        std::ofstream out("/home/tll/T4C_Server_Linux_Final_Step/config_temp.ini");
        std::string line, key;
        bool found = false;
        while (std::getline(in, line)) {
                size_t pos = line.find('=');
                if (pos != std::string::npos) {
                        key = line.substr(0, pos);
                        if (key != lpszItem) out << line << "\n";
                        else found = true;
                } else {
                        out << line << "\n";
                }
        }
        in.close();
        out.close();
        rename("/home/tll/T4C_Server_Linux_Final_Step/config_temp.ini", "/home/tll/T4C_Server_Linux_Final_Step/config.ini");
        return found;
}
//////////////////////////////////////////////////////////////////////////////////////////
BOOL RegKeyHandler::DeleteKey( LPCTSTR subkey ){
        if (remove("/home/tll/T4C_Server_Linux_Final_Step/config.ini") == 0) return TRUE;
        return FALSE;
}