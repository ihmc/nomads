#ifndef INCL_STREAM_SERVER_MOCKET_H
#define INCL_STREAM_SERVER_MOCKET_H

/*
 * StreamServerMocket.h
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
 */

#include "FTypes.h"


class ServerMocket;
class StreamMocket;

class StreamServerMocket
{
    public:
        StreamServerMocket (void);
        ~StreamServerMocket (void);

        // Binds the socket to the specified port and prepares it to accept connections 
        // If port is 0, a random port is chosen
        // Returns the port number if successful and a negative value in case of error
        int listen (uint16 ui16Port);

        // Binds the socket to the specified port and address and prepares it to accept connections
        // If port is 0, a random port is chosen
        // The listen addr is use to bind to a specific interface
        // Returns the port number if successful and a negative value in case of error
        int listen (uint16 ui16Port, const char *pszListenAddr);

        // Accepts a new incoming connection
        // Returns a new instance of Mocket to represent the local endpoint or nullptr in case of an error
        // NOTE: Caller is responsible for deleting the Mocket instance after use
        StreamMocket * accept (void);

        // Closes the server mocket instance
        // Returns 0 if successul and a negative value in case of error
        int close (void);

    private:
        ServerMocket *_pMSMocket;
};

#endif   // #ifndef INCL_STREAM_SERVER_MOCKET_H
