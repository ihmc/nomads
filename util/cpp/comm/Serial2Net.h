/*
 * Serial2Net.h
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2016 IHMC.
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

#ifndef INCL_SERIAL_2_NET_H
#define INCL_SERIAL_2_NET_H

/*
 * Serial2Net
 *
 * Class that allows serial port connections to be redirected over a network to
 * a serial device attached to a remote computer.
 *
 * Contributed by Steve Choy from the U.S. Army Research Laboratory, Adelphi, MD
 * July 2009
 *
 */

#if defined (WIN32)
    #include <winsock2.h>
#else
    typedef int SOCKET;
#endif

#include "StrClass.h"

namespace NOMADSUtil
{
    class Serial2Net
    {
        public:
            Serial2Net (void);
            ~Serial2Net (void);

            int openPort (const char *devstr, int iIPControlPort);
            void closePort (void);

            int setXONXOFF (bool enable);
            bool getXONXOFFState (void);

            int setDTR (bool val);
            bool getDTRState (void);

            int setRTS (int val);
            int setRTSCTSHandshake (bool rtsAndCtsEnable);
            bool getRTSCTSHandshake (void);

            int setTimeOut (int tenthOfSecs);
            void doBreak (int milsec);
            int setSpeed (int speed);

            int get (void);
            int put (unsigned char c);
            int getline (unsigned char *bf, int bfln);
            int putline (unsigned char *bf, int bfln);

        protected:
            SOCKET tcpipCreateClient (const char *pszNodeName, int portid);
            int ser2netConfig (const char *configText);
            int ser2netControl (const char *configText);

        private:
            bool portEnabled;
            SOCKET tserverSocket;
            SOCKET tcontrolSocket;
            NOMADSUtil::String tserverAddress;
            int tserverPort;
            bool xonxoffState;
            bool dtrState;
            bool rtsctsHandshakeState;
    };
}

#endif   // #ifndef INCL_SERIAL_2_NET_H
