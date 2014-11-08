#ifndef INCL_SERVER_MOCKET_H
#define INCL_SERVER_MOCKET_H

/*
 * ServerMocket.h
 * 
 * This file is part of the IHMC Mockets Library/Component
 * Copyright (c) 2002-2014 IHMC.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 3 (GPLv3) as published by the Free Software Foundation.
 * 
 * U.S. Government agencies and organizations may redistribute
 * and/or modify this program under terms equivalent to
 * "Government Purpose Rights" as defined by DFARS
 * 252.227-7014(a)(12) (February 2014).
 * 
 * Alternative licenses that allow for use within commercial products may be
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details.
 *
 * ServerMocket
 *
 * The main class for a server application to use the mockets communication library
 * Similar in functionality to a server socket - used by a server to accept connections
 * from client applications.
 */

#include "ConditionVariable.h"
#include "DArray.h"
#include "FTypes.h"
#include "InetAddr.h"
#include "Mutex.h"
#include "StrClass.h"


namespace NOMADSUtil
{
    class UDPDatagramSocket;
}

class CommInterface;
class Mocket;
class Packet;

class ServerMocket
{
    public:
        ServerMocket (const char *pszConfigFile = NULL, CommInterface *pCI = NULL, bool bDeleteCIWhenDone = false);
        ~ServerMocket (void);

        // Initialize the server mocket to accept incoming connections
        // Specifying a 0 for the port causes a random port to be allocated
        // Returns the port number that was assigned, or a negative value in case of error
        int listen (uint16 ui16Port);

        int listen (uint16 ui16Port, const char *pszListenAddr);

        Mocket * accept (uint16 ui16PortForNewConnection = 0);

        int close (void);

        // See Mocket.h
        void setIdentifier (const char *pszIdentifier);

        // See Mocket.h
        const char * getIdentifier (void);

    private:
        static const uint16 MAX_RESUME_TOKEN_LEN = 1024;
        struct CookieRec {
            CookieRec (void);
            bool bInUse;
            NOMADSUtil::InetAddr remoteAddr;
            int64 i64CookieExpirationTime;
            uint16 ui16Count;
            uint16 ui16LocalPort;
            uint8 aui8ResumeToken[MAX_RESUME_TOKEN_LEN];   // Stores the Encrypted Resume Token
            uint16 ui16ResumeTokenLen;                     // Length of the Encrypted Resume Token
        };

    private:
        Mocket * processIncomingPacket (Packet *pPacket, NOMADSUtil::InetAddr *pRemoteAddr, uint16 ui16PortForNewConnection);
        int clearCookieRec (NOMADSUtil::InetAddr remoteAddr);
        CookieRec * getCookieRec (NOMADSUtil::InetAddr remoteAddr);

    private:
		NOMADSUtil::String _configFile;
        NOMADSUtil::String _identifier;
        uint16 _ui16Port;
        uint32 _ui32ListenAddr;
        CommInterface *_pCommInterface;
        bool _bLocallyCreatedCI;
        bool _bDeleteCIWhenDone;
        bool _bAccepting;
        NOMADSUtil::Mutex _mInAccept;
        NOMADSUtil::ConditionVariable _cvInAccept;
        bool _bInAccept;
        NOMADSUtil::DArray<CookieRec> _cookieHistory;
};

inline void ServerMocket::setIdentifier (const char *pszIdentifier)
{
    _identifier = pszIdentifier;
}

inline const char * ServerMocket::getIdentifier (void)
{
    return _identifier;
}

inline ServerMocket::CookieRec::CookieRec (void)
{
    bInUse = false;
    i64CookieExpirationTime = 0;
    ui16Count = 0;
    ui16LocalPort = 0;
    ui16ResumeTokenLen = 0;
}

#endif   // #ifndef INCL_SERVER_MOCKET_H
