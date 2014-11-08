/*
 * Serial2Net.h
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2014 IHMC.
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

#include "Serial2Net.h"

#if defined (WIN32)
    #define usleep(x) Sleep((x)/1000)
#elif defined (UNIX)
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <netdb.h>
    #include <arpa/inet.h>
    #include <signal.h>
    #include <string.h>
    #define closesocket close
#endif

#define SocketError ((SOCKET)-1)

#include <stdio.h>

using namespace NOMADSUtil;

Serial2Net::Serial2Net()
{
    portEnabled = false;
}

Serial2Net::~Serial2Net()
{
    if (portEnabled) {
        closePort();
    }
}

int Serial2Net::openPort (const char *devstr, int iIPControlPort)
{
    int rc = -1;
    char *tsearch;
    int tserverln;
    char tmpAddress[500];

    tserverSocket = SocketError;
    // if we find a colon in the middle of the device spec, assume terminal server tcpip
    if ((tsearch = strchr ((char *)devstr, ':')) != 0 ) {
        if (*(tsearch+1) != 0) {
            int rettmp = sscanf (tsearch+1, "%d", &(tserverPort));
            #ifdef WIN32
                WORD wVersionRequested;
                WSADATA wsaData;
                wVersionRequested = MAKEWORD (1, 1);
                WSAStartup (wVersionRequested, &wsaData);
            #else
                signal (SIGPIPE, SIG_IGN);
            #endif
            tserverln = (int) (tsearch - devstr);
            strncpy (tmpAddress, devstr, tserverln);
            tmpAddress[tserverln] = 0;
            tserverAddress = tmpAddress;
            tserverSocket = tcpipCreateClient (tserverAddress, tserverPort);
            if (tserverSocket != SocketError) {
                rc = 0;
                portEnabled = true;
                // Default control port for ser2net is is rounded down to nearest 100
                if (iIPControlPort == 0) {
                    iIPControlPort = (tserverPort / 100) * 100;
                }
                else if (iIPControlPort < 0) {
                    tcontrolSocket = SocketError;  // Control port is disabled by caller
                }
                else {
				    tcontrolSocket = tcpipCreateClient (tserverAddress, iIPControlPort);
                }
            }
            else {
                tcontrolSocket = SocketError;   // If no serverSocket, then there is no control socket
            }
        }
    }
    return rc;
}

void Serial2Net::closePort()
{
    if (portEnabled) {
        closesocket (tserverSocket);
        portEnabled = false;
    }
}

int Serial2Net::setXONXOFF (bool enable)
{
    xonxoffState = enable;
    if (portEnabled) {
        char configTxt[200];
        sprintf (configTxt, "setportconfig %d %s\r", tserverPort, (enable ? "-XONXOFF" : "XONXOFF"));
        return ser2netConfig (configTxt);
    }
    return -1;
}

bool Serial2Net::getXONXOFFState (void)
{
    return xonxoffState;
}

/*
clear/set DTR - returns -1 if failure
if val true then set else clear

*/
int Serial2Net::setDTR (bool val)
{
    dtrState = val;
    if (portEnabled) {
        char cmdTxt[200];
        sprintf (cmdTxt, "setportcontrol %d %s\r", tserverPort, (val ? "DTRHI" : "DTRLO"));
        return ser2netControl (cmdTxt);
    }
    return -1;
}

bool Serial2Net::getDTRState (void)
{
    return dtrState;
}

/*
clear/set RTS - returns -1 if failure
if val is not zero then set else clear
*/
int Serial2Net::setRTS (int val)
{
    if (portEnabled) {
        char cmdTxt[200];
        sprintf (cmdTxt, "setportcontrol %d %s\r", tserverPort, (val == 0 ? "RTSLO" : "RTSHI"));
        return ser2netControl (cmdTxt);
    }
    return -1;
}

/*
enable/disable rtscts hardware flow control returns -1 if failure
currently does bot together
*/
int Serial2Net::setRTSCTSHandshake (bool rtsctsHandshakeEnable)
{
    rtsctsHandshakeState = rtsctsHandshakeEnable;
    if (portEnabled) {
        char configTxt[200];
        sprintf (configTxt, "setportconfig %d %s\r", tserverPort, (rtsctsHandshakeEnable ? "RTSCTS" : "-RTSCTS"));
        return ser2netConfig (configTxt);
    }
    return -1;
}

bool Serial2Net::getRTSCTSHandshake (void)
{
    return rtsctsHandshakeState;
}

/* if timeout is 0, wait forever, >0 timeout is in 1/10 seconds, -1 is noblock) */
int Serial2Net::setTimeOut (int tenthOfSecs)
{   
    return -1;
}

void Serial2Net::doBreak (int milsec)
{
}

int Serial2Net::setSpeed (int speed)
{
    if (portEnabled) {
        char configTxt[200];
        sprintf (configTxt, "setportconfig %d %d\r", tserverPort, speed);
        return ser2netConfig (configTxt);
    }
    return -1;
}

int Serial2Net::get()
{
    unsigned char c = 0;
    int readln;
    if (!portEnabled) {
        usleep (10000);
        return 0;
    }
    readln = getline (&c, 1);
    if (readln != 1) {
        return -1;
    }
    return (int)c;
}

int Serial2Net::put (unsigned char c)
{
    return putline (&c, 1);
}

//
//  ttyGetline will
//    return 0 if nothing read (i.e. timeout)
//          -1 is error on read
//          else number of bytes read 
int Serial2Net::getline (unsigned char *bf, int bfln)
{
    int rc;
    if (portEnabled) {
        rc = recv (tserverSocket, (char*)bf, bfln, 0);
        if (rc <= 0) {
            closesocket (tserverSocket);
            usleep (20000);
            tserverSocket = tcpipCreateClient (tserverAddress, tserverPort);
            if (tserverSocket == SocketError) {
                portEnabled = false;
            }
        }
        return rc;
    }
    return -1;
}

int Serial2Net::putline (unsigned char *bf, int bfln)
{
    if (portEnabled) {
        return send (tserverSocket, (char*)bf, bfln, 0);
    }
    return -1;
}

SOCKET Serial2Net::tcpipCreateClient (const char *pszNodeName, int portid)
{
    SOCKET s;
    if ((s = socket (AF_INET, SOCK_STREAM, 0)) == SocketError) {
        return SocketError;
    }
    struct hostent *hp;
    if (0 == (hp = gethostbyname (pszNodeName))) {
        return SocketError;
    }
    struct sockaddr serve;
    memset(&serve, 0, sizeof (struct sockaddr));
    struct sockaddr_in *sin = (struct sockaddr_in *) &serve;
    sin->sin_addr = *((struct in_addr*)(hp->h_addr));
    sin->sin_family = AF_INET;
    sin->sin_port = htons ((short) portid);
    if (connect (s, &serve, sizeof (serve)) < 0) {
        closesocket (s);
        return SocketError;
    }
    return s;
}

// don't forget terminating \r in configText
int Serial2Net::ser2netConfig (const char *configText)
{
    int status = -1;
    char tmp[2];
    if (portEnabled) {
        if (tcontrolSocket != SocketError) {
            closesocket (tserverSocket);  // close data port
            portEnabled = false;
            // send config change to control port
            send (tcontrolSocket, configText, (int) strlen (configText) + 1, 0);
            // wait for reply "->"
            status = recv (tcontrolSocket, tmp, 2, 0);
            // re-open dataport
            tserverSocket = tcpipCreateClient (tserverAddress, tserverPort);
            if (tserverSocket == SocketError) {
                tserverSocket = 0;
            }
            else {
                portEnabled = true;
            }
        }
        else {
            // no control port, so probably not ser2net protocol
            // need to wait a little bit or com2tcp loses data
            usleep (100000);
        }
    }
    //return status;
    return 0;
}

int Serial2Net::ser2netControl (const char *configText)
{
    int status = -1;
    // use to change dynamic controls like rts and dtr
    char tmp[2];
    if (portEnabled) {
        if (tcontrolSocket != SocketError) {
            // send dynamic command change to control port
            send (tcontrolSocket, configText, (int) strlen (configText) + 1, 0);
            // wait for reply "->"
            status = recv (tcontrolSocket, tmp, 2, 0);
        }
        else {
            // no control port, so probably not ser2net protocol
            // need to wait a little bit or com2tcp loses data
            usleep (100000);
        }
    }
    return status;
}
