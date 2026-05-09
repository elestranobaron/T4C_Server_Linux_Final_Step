#include "stdafx.h"
#include "Portability.h"
#include "TFCPacket.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iterator>

#ifdef HEADER_SIZE
#undef HEADER_SIZE
#endif
#define HEADER_SIZE (KEY_SIZE + CHECKSUM_SIZE)

namespace {
inline BYTE Byte3(std::uint32_t value) { return static_cast<BYTE>((value >> 24U) & 0xFFU); }
inline BYTE Byte2(std::uint32_t value) { return static_cast<BYTE>((value >> 16U) & 0xFFU); }
inline BYTE Byte1(std::uint32_t value) { return static_cast<BYTE>((value >> 8U) & 0xFFU); }
inline BYTE Byte0(std::uint32_t value) { return static_cast<BYTE>(value & 0xFFU); }
} // namespace

TFCPacketException::TFCPacketException(UINT cause) {
    m_cause = cause;
}

TFCPacket::TFCPacket() : nPos(0), packetSeedID(0) {
    BYTE bHeader[HEADER_SIZE] = {0, 0, 0, 0};
    copy(bHeader, bHeader + HEADER_SIZE, inserter(vBuffer, vBuffer.begin()));
}

TFCPacket::~TFCPacket() {
}

void TFCPacket::Create(unsigned int length) {
    BYTE bHeader[HEADER_SIZE] = {0, 0, 0, 0};
    vBuffer.clear();
    copy(bHeader, bHeader + HEADER_SIZE, inserter(vBuffer, vBuffer.begin()));
    vBuffer.resize(HEADER_SIZE + length, 0);
    nPos = 0;
}

void TFCPacket::Destroy() {
    vBuffer.erase(vBuffer.begin() + HEADER_SIZE, vBuffer.end());
    nPos = 0;
}

void TFCPacket::Seek(std::int32_t where, char how) {
    switch (how) {
        case 0:
            nPos = static_cast<unsigned int>(where < 0 ? 0 : where);
            break;
        case 1: {
            const std::int64_t next = static_cast<std::int64_t>(nPos) + where;
            nPos = static_cast<unsigned int>(next < 0 ? 0 : next);
            break;
        }
        default:
            break;
    }
}

void TFCPacket::EncryptPacket() {
    // Kept disabled to preserve existing behavior.
}

BOOL TFCPacket::DecryptPacket(unsigned int seedNumber) {
    packetSeedID = seedNumber;
    return TRUE;
}

TFCPacket &TFCPacket::operator<<(std::int32_t value) {
    const std::uint32_t uvalue = static_cast<std::uint32_t>(value);
    vBuffer.push_back(Byte3(uvalue));
    vBuffer.push_back(Byte2(uvalue));
    vBuffer.push_back(Byte1(uvalue));
    vBuffer.push_back(Byte0(uvalue));
    return *this;
}

TFCPacket &TFCPacket::operator<<(long value) {
    return operator<<(static_cast<std::int32_t>(value));
}

TFCPacket &TFCPacket::operator<<(short value) {
    const std::uint16_t uvalue = static_cast<std::uint16_t>(value);
    vBuffer.push_back(Byte1(uvalue));
    vBuffer.push_back(Byte0(uvalue));
    return *this;
}

TFCPacket &TFCPacket::operator<<(char value) {
    vBuffer.push_back(static_cast<BYTE>(value));
    return *this;
}

TFCPacket &TFCPacket::operator<<(const char *lpszString) {
    if (lpszString == NULL) {
        return *this;
    }
    const int nStrLen = static_cast<int>(std::strlen(lpszString));
    vBuffer.push_back(Byte1(static_cast<std::uint16_t>(nStrLen)));
    vBuffer.push_back(Byte0(static_cast<std::uint16_t>(nStrLen)));
    copy(lpszString, lpszString + nStrLen, back_inserter(vBuffer));
    return *this;
}

TFCPacket &TFCPacket::operator<<(const string &csString) {
    return operator<<(csString.c_str());
}

#if defined(_AFXDLL) || !defined(_WIN32)
TFCPacket &TFCPacket::operator<<(const CString &csString) {
    return operator<<(static_cast<const char *>(csString));
}

CString TFCPacket::GetDebugPacketString(void) {
    CString out;
    const size_t maxBytes = 512;
    char chunk[8];
    const size_t n = std::min(vBuffer.size(), maxBytes);
    for (size_t i = 0; i < n; ++i) {
        std::snprintf(chunk, sizeof(chunk), "%02X ", static_cast<unsigned>(vBuffer[i]));
        out.append(chunk);
    }
    if (vBuffer.size() > maxBytes) {
        out.append("...");
    }
    return out;
}
#endif

void TFCPacket::Get(std::int32_t *i) {
    *i = 0;
    if (HEADER_SIZE + nPos + 4 <= vBuffer.size()) {
        const std::uint32_t value = (static_cast<std::uint32_t>(vBuffer[HEADER_SIZE + nPos++]) << 24U) |
                                    (static_cast<std::uint32_t>(vBuffer[HEADER_SIZE + nPos++]) << 16U) |
                                    (static_cast<std::uint32_t>(vBuffer[HEADER_SIZE + nPos++]) << 8U) |
                                    static_cast<std::uint32_t>(vBuffer[HEADER_SIZE + nPos++]);
        *i = static_cast<std::int32_t>(value);
    } else {
        TFCPacketException *e = new TFCPacketException(1);
        throw (e);
    }
}

void TFCPacket::Get(long *i) {
    std::int32_t v = 0;
    Get(&v);
    *i = static_cast<long>(v);
}

void TFCPacket::Get(short *i) {
    *i = 0;
    if (HEADER_SIZE + nPos + sizeof(short) <= vBuffer.size()) {
        *i = static_cast<short>((vBuffer[HEADER_SIZE + nPos++] << 8) + vBuffer[HEADER_SIZE + nPos++]);
    } else {
        TFCPacketException *e = new TFCPacketException(1);
        throw (e);
    }
}

void TFCPacket::Get(char *i) {
    *i = 0;
    if (HEADER_SIZE + nPos + sizeof(char) <= vBuffer.size()) {
        *i = static_cast<char>(vBuffer[HEADER_SIZE + nPos++]);
    } else {
        TFCPacketException *e = new TFCPacketException(1);
        throw (e);
    }
}

void TFCPacket::Get(std::uint32_t *i) {
    *i = 0;
    if (HEADER_SIZE + nPos + 4 <= vBuffer.size()) {
        *i = (static_cast<std::uint32_t>(vBuffer[HEADER_SIZE + nPos++]) << 24U) |
             (static_cast<std::uint32_t>(vBuffer[HEADER_SIZE + nPos++]) << 16U) |
             (static_cast<std::uint32_t>(vBuffer[HEADER_SIZE + nPos++]) << 8U) |
             static_cast<std::uint32_t>(vBuffer[HEADER_SIZE + nPos++]);
    } else {
        TFCPacketException *e = new TFCPacketException(1);
        throw (e);
    }
}

void TFCPacket::Get(unsigned long *i) {
    std::uint32_t v = 0;
    Get(&v);
    *i = static_cast<unsigned long>(v);
}

void TFCPacket::Get(unsigned short *i) {
    *i = 0;
    if (HEADER_SIZE + nPos + sizeof(short) <= vBuffer.size()) {
        *i = static_cast<unsigned short>((vBuffer[HEADER_SIZE + nPos++] << 8) + vBuffer[HEADER_SIZE + nPos++]);
    } else {
        TFCPacketException *e = new TFCPacketException(1);
        throw (e);
    }
}

void TFCPacket::Get(unsigned char *i) {
    *i = 0;
    if (HEADER_SIZE + nPos + sizeof(char) <= vBuffer.size()) {
        *i = vBuffer[HEADER_SIZE + nPos++];
    } else {
        TFCPacketException *e = new TFCPacketException(1);
        throw (e);
    }
}

bool TFCPacket::CheckLen(WORD usLen) {
    return (HEADER_SIZE + nPos + usLen <= vBuffer.size());
}

void TFCPacket::Get(string &str) {
    char buf[1024];
    WORD strLen = 0;
    Get(reinterpret_cast<short *>(&strLen));

    if (strLen > 1024) {
        strLen = 1024;
    }

    if (HEADER_SIZE + nPos + sizeof(char) * strLen <= vBuffer.size()) {
        int i = 0;
        for (; i < strLen; i++) {
            Get(reinterpret_cast<char *>(&buf[i]));
        }
        buf[i] = 0;
        str = buf;
    }
}

BOOL TFCPacket::SetBuffer(LPBYTE lpNewBuffer, int nBufferSize) {
    if (nBufferSize < HEADER_SIZE + static_cast<int>(sizeof(RQ_SIZE))) {
        return FALSE;
    }

    vBuffer.erase(vBuffer.begin(), vBuffer.end());
    copy(lpNewBuffer, lpNewBuffer + nBufferSize, back_inserter(vBuffer));
    nPos = 0;
    return TRUE;
}

void TFCPacket::GetBuffer(LPBYTE &lpNewBuffer, int &nBufferSize) {
    lpNewBuffer = &vBuffer.front();
    nBufferSize = static_cast<int>(vBuffer.size());
}

unsigned int TFCPacket::GetPacketSeedID() {
    return packetSeedID;
}

void TFCPacket::SetPacketSeedID(unsigned int newPacketSeedID) {
    packetSeedID = newPacketSeedID;
}

RQ_SIZE TFCPacket::GetPacketID() {
    if (vBuffer.size() - HEADER_SIZE >= sizeof(RQ_SIZE)) {
        const unsigned int nOldPos = nPos;
        nPos = 0;
        RQ_SIZE rqPacketID = 0;
        Get(reinterpret_cast<short *>(&rqPacketID));
        nPos = nOldPos;
        return rqPacketID;
    }
    return 0;
}
