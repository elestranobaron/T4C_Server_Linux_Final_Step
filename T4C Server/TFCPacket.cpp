//#include <windows.h>
//#include <stdio.h>
#include "TFCPacket.h"
#include <iterator>
#include <algorithm>
#include <functional>
#include <string>
#include <iostream>

/* Old-Crypt stuff. Turned off.
#ifdef _AFXDLL
#include "EncRandom.h"
#else
#include "Random.h"
#endif
*/
//#include "T4CLog.h" //BLBLBL
//#include <smrtheap.hpp>

#define HIBYTE(w) ((BYTE)(((WORD)(w) >> 8) & 0xFF))
#define LOBYTE(w) ((BYTE)(w))
#define HIWORD(l) ((WORD)(((DWORD)(l) >> 16) & 0xFFFF))
#define LOWORD(l) ((WORD)(l))

#ifdef _AFXDLL
#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new new
#endif
#endif

#define TRACE(...) std::cerr << __VA_ARGS__ << std::endl
#define GROW_BY 5
#define HEADER_SIZE ( KEY_SIZE + CHECKSUM_SIZE )

TFCPacketException::TFCPacketException(UINT cause){
    m_cause = cause;
}

TFCPacket::TFCPacket( void )
{
    // Reserve 64 bytes default size.
    vBuffer.reserve( 64 );
    // Create the space for the initial header.
    int i;
    for( i = 0; i < HEADER_SIZE; i++ ){
        vBuffer.push_back( 0 );
    }
    nPos = 0;
    packetSeedID = 0;
}

TFCPacket::~TFCPacket( void )
{
}

void TFCPacket::Destroy( void )
{
    BYTE bHeader[ HEADER_SIZE ] = { 0, 0, 0, 0 };
    // Destroy the vector, but not its header.
    vBuffer.erase( vBuffer.begin() + HEADER_SIZE, vBuffer.end() );
    nPos = 0;
}

void TFCPacket::Seek(signed long where, char how)
{
    switch(how)
    {
        case 0: nPos = where; break;
        case 1: nPos += where; break;
    }
}

void TFCPacket::EncryptPacket( void )
{
/* FUNCTION OFF. No longer using this encryption method.
#ifdef _AFXDLL
    EncRandom rndKey( 0, sizeof( KEY ) * 256 );
#else
    Random rndKey( 0, sizeof( KEY ) * 256 );
#endif
    // Fetch key value
    KEY kKey = rndKey;
    // Set new seed according to key.
#ifdef _AFXDLL
    EncRandom rndEncrypt( 0, 255, kKey, packetSeedID );
#else
    Random rndEncrypt( 0, 255, kKey );
#endif
    // Copy key into buffer.
    unsigned int i;
    for( i = 0; i < KEY_SIZE; i++ ){
        vBuffer[ i ] = *((LPBYTE)( &kKey ) + i );
    }
    // Calculate checksum.
    CHECKSUM kCheckSum = 0;
    for( i = 0; i < vBuffer.size() - HEADER_SIZE; i++ ){
        kCheckSum += vBuffer[ i + HEADER_SIZE ];
    }
    // Copy checksum into buffer
    for( i = 0; i < CHECKSUM_SIZE; i++ ){
        vBuffer[ i + 2 ] = *((LPBYTE)( &kCheckSum ) + i );
    }
    // Encrypt the data.
    for( i = 0; i < vBuffer.size() - HEADER_SIZE; i++ ){
        vBuffer[ i + HEADER_SIZE ] = vBuffer[ i + HEADER_SIZE ] ^ rndEncrypt;
    }
FUNCTION OFF. No longer using this encryption method. */
}

BOOL TFCPacket::DecryptPacket( unsigned int seedNumber )
{
return TRUE;
/* FUNCTION OFF. No longer using this encryption method.
    // Minimal packet length
    unsigned int i;
    //Lets backup the buffer
    vector<BYTE> vBackupBuffer(vBuffer);
    // First fetch the key.
    KEY kKey;
    for( i = 0; i < KEY_SIZE; i++ ){
        *((LPBYTE)( &kKey ) + i ) = vBuffer[ i ];
    }
    CHECKSUM kCheckSum;
    for( i = 0; i < CHECKSUM_SIZE; i++ ){
        *((LPBYTE)( &kCheckSum ) + i ) = vBuffer[ i + 2 ];
    }
#ifdef _AFXDLL
    EncRandom rndEncrypt( 0, 255, kKey, seedNumber );
#else
    Random rndEncrypt( 0, 255, kKey );
#endif
    // Decrypt data.
    for( i = 0; i < vBuffer.size() - HEADER_SIZE; i++ ){
        vBuffer[ i + HEADER_SIZE ] = vBuffer[ i + HEADER_SIZE ] ^ rndEncrypt;
    }
    CHECKSUM kCheck = 0;
    // Calculate checksum.
    for( i = 0; i < vBuffer.size() - HEADER_SIZE; i++ ){
        kCheck += vBuffer[ i + HEADER_SIZE ];
    }
    // If the two values are the same.
    if( kCheck == kCheckSum ){
        packetSeedID = seedNumber;
        return TRUE;
    }
    //Restores the buffer before returning false
    //so it can be decrypted again if needed
    vBuffer = vBackupBuffer;
    return FALSE;
FUNCTION OFF. No longer using this encryption method. */
}

TFCPacket &TFCPacket::operator << (short value)
{
    vBuffer.push_back( HIBYTE(value) );
    vBuffer.push_back( LOBYTE(value) );
    return *this;
}

TFCPacket &TFCPacket::operator << (char value)
{
    vBuffer.push_back( (BYTE)value );
    return(*this);
}

TFCPacket &TFCPacket::operator << (long value)
{
    vBuffer.push_back( HIBYTE(HIWORD(value)) );
    vBuffer.push_back( LOBYTE(HIWORD(value)) );
    vBuffer.push_back( HIBYTE(LOWORD(value)) );
    vBuffer.push_back( LOBYTE(LOWORD(value)) );
    return(*this);
}

TFCPacket & TFCPacket::operator << (const char * lpszString){
    int nStrLen = strlen(lpszString);
    // Stored string length.
    vBuffer.push_back( HIBYTE( nStrLen ) );
    vBuffer.push_back( LOBYTE( nStrLen ) );
    // Copy string into vector.
    copy( lpszString, lpszString + nStrLen, back_inserter( vBuffer ) );
    return *this;
}

TFCPacket &TFCPacket::operator << ( const string &csString ){
    return operator<< ( csString.c_str() );
}

#ifdef _AFXDLL
TFCPacket & TFCPacket:: operator << (std::string &csString){
    int nStrLen = csString.length();
    // Stored string length.
    vBuffer.push_back( HIBYTE( nStrLen ) );
    vBuffer.push_back( LOBYTE( nStrLen ) );
    // Copy string buffer into vector
    copy( csString.begin(), csString.end(), back_inserter( vBuffer ) );
    return *this;
}
#endif

void TFCPacket::Get(long *i)
{
    *i = 0;
    if( HEADER_SIZE + nPos + sizeof( long ) <= vBuffer.size() ){
        *i = vBuffer[ HEADER_SIZE + nPos++ ] << 24;
        *i += vBuffer[ HEADER_SIZE + nPos++ ] << 16;
        *i += vBuffer[ HEADER_SIZE + nPos++ ] << 8;
        *i += vBuffer[ HEADER_SIZE + nPos++ ];
    }else{
        TFCPacketException *e = new TFCPacketException(1);
        throw(e);
    }
}

void TFCPacket::Get(short *i)
{
    *i = 0;
    if( HEADER_SIZE + nPos + sizeof( short ) <= vBuffer.size() ){
        *i = vBuffer[ HEADER_SIZE + nPos++ ] << 8;
        *i += vBuffer[ HEADER_SIZE + nPos++ ];
    }else{
        TFCPacketException *e = new TFCPacketException(1);
        throw(e);
    }
}

void TFCPacket::Get(char *i)
{
    *i = 0;
    if( HEADER_SIZE + nPos + sizeof( char ) <= vBuffer.size() ){
        *i = vBuffer[ HEADER_SIZE + nPos++ ];
    }/*else{//BLBLBL test sans l'exception
        TFCPacketException *e = new TFCPacketException(1);
        throw(e);
    }*/
}

// Mestoph : V�rification de la taille des strings avant de lire le contenu du data
bool TFCPacket::CheckLen(WORD usLen)
{
    if ( HEADER_SIZE + nPos + usLen <= vBuffer.size())
    {
        return true;
    }
    return false;
}

void TFCPacket::Get(unsigned long *i)
{
    *i = 0;
    if( HEADER_SIZE + nPos + sizeof( long ) <= vBuffer.size() ){
        *i = vBuffer[ HEADER_SIZE + nPos++ ] << 24;
        *i += vBuffer[ HEADER_SIZE + nPos++ ] << 16;
        *i += vBuffer[ HEADER_SIZE + nPos++ ] << 8;
        *i += vBuffer[ HEADER_SIZE + nPos++ ];
    }else{
        TFCPacketException *e = new TFCPacketException(1);
        throw(e);
    }
}

void TFCPacket::Get(unsigned short *i)
{
    *i = 0;
    if( HEADER_SIZE + nPos + sizeof( short ) <= vBuffer.size() ){
        *i = vBuffer[ HEADER_SIZE + nPos++ ] << 8;
        *i += vBuffer[ HEADER_SIZE + nPos++ ];
    }else{
        TFCPacketException *e = new TFCPacketException(1);
        throw(e);
    }
}

void TFCPacket::Get(unsigned char *i)
{
    *i = 0;
    if( HEADER_SIZE + nPos + sizeof( char ) <= vBuffer.size() ){
        *i = vBuffer[ HEADER_SIZE + nPos++ ];
    }else{
        TFCPacketException *e = new TFCPacketException(1);
        throw(e);
    }
}

void TFCPacket::Get( string &str ){
    char buf[ 1024 ];
    WORD strLen = 0;
    Get( (short *)&strLen );
    if (strLen>1024) strLen=1024; //BLBLBL Juste au cas o�, tronquage de toute chaine soit disant plus longue que le buffer.
    if( HEADER_SIZE + nPos + sizeof( char ) * strLen <= vBuffer.size() ){
        int i;
        for( i = 0; i < strLen; i++ ){
            Get( (char *)&buf[ i ] );
        }
        buf[ i ] = 0;
        str = buf;
    /*}else{ //BLBLBL test sans l'exception
        TFCPacketException *e = new TFCPacketException(1);
        throw(e);*/
    }
}

#ifdef _AFXDLL
CString TFCPacket::GetDebugPacketString( void )
{
    std::string csTemp;
    std::string csFinal;
    unsigned int i;
    // Scroll through packet
    for( i = 0; i < vBuffer.size() - HEADER_SIZE; i++ ){
        csTemp = fmt::format( "%u ", vBuffer[ HEADER_SIZE + i ] );
        csFinal += csTemp;
    }
    return csFinal;
}
#endif

BOOL TFCPacket::SetBuffer(LPBYTE lpNewBuffer, int nBufferSize)
{
    // If given packet buffer is big enough to hold wanted information.
    if( nBufferSize < HEADER_SIZE + sizeof( RQ_SIZE ) ){
        return FALSE;
    }
    // Destroy previous packet.
    vBuffer.erase( vBuffer.begin(), vBuffer.end() );
    copy( lpNewBuffer, lpNewBuffer + nBufferSize, back_inserter( vBuffer ) );
    nPos = 0;
    return TRUE;
}

void TFCPacket::GetBuffer(LPBYTE &lpNewBuffer, int &nBufferSize)
{
    lpNewBuffer = &vBuffer.front();
    nBufferSize = vBuffer.size();
}

unsigned int TFCPacket::GetPacketSeedID()
{
    return packetSeedID;
}

void TFCPacket::SetPacketSeedID(unsigned int newPacketSeedID)
{
    packetSeedID = newPacketSeedID;
}

RQ_SIZE TFCPacket::GetPacketID( void )
{
    // If there is at least a packetID.
    if( vBuffer.size() - HEADER_SIZE >= sizeof( RQ_SIZE ) ){
        int nOldPos = nPos;
        // Go to the beginning of the packet.
        nPos = 0;
        RQ_SIZE rqPacketID = 0;
        // Fetch the packetID.
        Get( (RQ_SIZE *)&rqPacketID );
        // Restore old position.
        nPos = nOldPos;
        return rqPacketID;
    }/*else{
        //BLBLBL on cherche l'origine des packets sans ID
        _LOG_DEBUG
            LOG_DEBUG_HIGH,
            "Packet have no ID, wrong size ? %u",
            vBuffer.size()
        LOG_
        return 0;
    }*/
    return 0;
}