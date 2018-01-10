/*
 * Serial.cpp
 *
 * Class to communicate with serial ports
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

#include "Serial.h"

#include "Logger.h"
#include "Serial2Net.h"

#if defined (WIN32)
	#define NOMINMAX
	#include <winsock2.h>
    #include <windows.h>
#elif defined (UNIX)
    #include <stdio.h>
    #include <errno.h>
    #include <fcntl.h>
    //#include <signal.h>
    #include <string.h>
    //#include <sys/time.h>
    #include <sys/ioctl.h>
    #include <termios.h>
    #include <unistd.h>
#endif

using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

Serial::Serial (void)
{
    _pS2N = NULL;
    #if defined (WIN32)
        _hComPort = NULL;
        _oRead.hEvent = NULL;
        _oWrite.hEvent = NULL;
    #else
        _fdComPort = -1;
    #endif
    _pDebugReadFile = NULL;
}

Serial::~Serial (void)
{
    if (_pS2N) {
        delete _pS2N;
        _pS2N = NULL;
    }
    else {
        #if defined (WIN32)
            CloseHandle (_hComPort);
            _hComPort = NULL;
            CloseHandle (_oRead.hEvent);
            _oRead.hEvent = NULL;
            CloseHandle (_oWrite.hEvent);
            _oWrite.hEvent = NULL;
        #else
            if (_fdComPort >= 0) {
                close (_fdComPort);
                _fdComPort = -1;
            }
        #endif
    }
}

int Serial::init (const char *pszPort, uint32 ui32DTESpeed, char chParity, uint8 ui8DataBits, uint8 ui8StopBits, int iIPControlPort)
{
    // If we find a colon in the middle of the port spec, assume terminal server tcpip
    const char *pszTemp;
    if ((pszTemp = strchr (pszPort, ':')) != NULL) {
        if (*(pszTemp+1) != NULL) { // Yes, assume this is a ser2net port
            if (_pS2N != 0) {
                // Just in case there is an old one hanging around
                delete _pS2N;
            }
            _pS2N = new Serial2Net();
            if (_pS2N->openPort (pszPort, iIPControlPort) < 0) {
                delete _pS2N;
                _pS2N = NULL;
                return -1;
            }
            if (_pS2N->setSpeed (ui32DTESpeed) < 0) {
                delete _pS2N;
                _pS2N = NULL;
                return -2;
            }
            return 0;
        }
    }
    #if defined (WIN32)
        char szFullyQualifiedPortName [_MAX_PATH];
        if (0 != strncmp ("\\\\.\\", pszPort, 4)) {
            strcpy (szFullyQualifiedPortName, "\\\\.\\");
            strcat (szFullyQualifiedPortName, pszPort);
        }
        else {
            strcpy (szFullyQualifiedPortName, pszPort);
        }
        _hComPort = CreateFile (szFullyQualifiedPortName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
        if (_hComPort == INVALID_HANDLE_VALUE) {
            checkAndLogMsg ("Serial::init", Logger::L_MildError,
                            "CreateFile failed with error %d while attempting to open <%s>\n", GetLastError(), szFullyQualifiedPortName);
            return -1;
        }
        memset (&_oRead, 0, sizeof (_oRead));
        memset (&_oWrite, 0, sizeof (_oWrite));
        _oRead.hEvent = CreateEvent (NULL, TRUE, FALSE, NULL);
        if (_oRead.hEvent == INVALID_HANDLE_VALUE) {
            checkAndLogMsg ("Serial::init", Logger::L_MildError,
                            "CreateEvent 1 failed with error %d\n", GetLastError());
            return -2;
        }
        _oWrite.hEvent = CreateEvent (NULL, TRUE, FALSE, NULL);
        if (_oWrite.hEvent == INVALID_HANDLE_VALUE) {
            checkAndLogMsg ("Serial::init", Logger::L_MildError,
                            "CreateEvent 2 failed with error %d\n", GetLastError());
            return -3;
        }

        DCB dcbComPort;
        memset (&dcbComPort, 0, sizeof (DCB));
        dcbComPort.DCBlength = sizeof (DCB);
        if (!strstr (pszPort, "pipe")) {    // VMWare kludge - Steve Choy (ARL)
            if (!GetCommState (_hComPort, &dcbComPort)) {
                checkAndLogMsg ("Serial::init", Logger::L_Warning,
                                "GetCommState failed with error %d\n", GetLastError());
                // CloseHandle (_hComPort);
                // _hComPort = NULL;
                return -4;
            }
        }

        dcbComPort.BaudRate = (DWORD) ui32DTESpeed;
        if ((chParity == 'N') || (chParity == 'n')) {
            dcbComPort.Parity = NOPARITY;
        }
        else if ((chParity == 'O') || (chParity == 'o')) {
            dcbComPort.Parity = ODDPARITY;
        }
        else if ((chParity == 'E') || (chParity == 'e')) {
            dcbComPort.Parity = EVENPARITY;
        }
        else {
            checkAndLogMsg ("Serial::init", Logger::L_MildError,
                            "parity must be N, n, O, o, E, or e\n");
            CloseHandle (_hComPort);
            _hComPort = NULL;
            return -5;
        }
        dcbComPort.ByteSize = ui8DataBits;
        if (ui8StopBits == 1) {
            dcbComPort.StopBits = ONESTOPBIT;
        }
        else if (ui8StopBits == 2) {
            dcbComPort.StopBits = TWOSTOPBITS;
        }
        else {
            checkAndLogMsg ("Serial::init", Logger::L_MildError,
                            "stop bits must be 1 or 2\n");
            CloseHandle (_hComPort);
            _hComPort = NULL;
            return -6;
        }

        if (!SetCommState (_hComPort, &dcbComPort)) {
            checkAndLogMsg ("Serial::init", Logger::L_Warning,
                            "SetCommState failed with error %d\n", GetLastError());
            // CloseHandle (_hComPort);
            // _hComPort = NULL;
            return -7;
        }
        return 0;
    #else
        if ((_fdComPort = open (pszPort, O_RDWR | O_NOCTTY)) < 0) {
            checkAndLogMsg ("Serial::init", Logger::L_MildError,
                            "open failed on port %s with error %d (%s)\n",
                            pszPort, errno, strerror (errno));
            return -8;
        }
        struct termios tios;
        memset (&tios, 0, sizeof(tios)); 
        cfmakeraw (&tios);
        speed_t speed = mapDTESpeed (ui32DTESpeed);
        if ((speed == B0) && (ui32DTESpeed != 0)) {
            // Error in the DTE speed parameter
            checkAndLogMsg ("Serial::init", Logger::L_MildError,
                            "invalid DTE speed %lu\n", ui32DTESpeed);
            close (_fdComPort);
            _fdComPort = -1;
            return -9;
        }
        if (cfsetispeed (&tios, speed)) {
            checkAndLogMsg ("Serial::init", Logger::L_MildError,
                            "ctsetispeed() failed; DTESpeed = %lu; errno = %d (%s)\n",
                            ui32DTESpeed, errno, strerror (errno));
            close (_fdComPort);
            _fdComPort = -1;
            return -10;
        }
        if (cfsetospeed (&tios, speed)) {
            checkAndLogMsg ("Serial::init", Logger::L_MildError,
                            "ctsetospeed() failed; DTESpeed = %lu; errno = %d (%s)\n",
                            ui32DTESpeed, errno, strerror (errno));
            close (_fdComPort);
            _fdComPort = -1;
            return -11;
        }
        if ((chParity == 'N') || (chParity == 'n')) {
            // Nothing to do by default
        }
        else if ((chParity == 'O') || (chParity == 'o')) {
            tios.c_cflag |= PARODD | PARENB;
        }
        else if ((chParity == 'E') || (chParity == 'e')) {
              tios.c_cflag |= PARENB;
        }
        else {
            checkAndLogMsg ("Serial::init", Logger::L_MildError,
                            "parity must be N, n, O, o, E, or e\n");
            close (_fdComPort);
            _fdComPort = -1;
            return -12;
        }
        if (ui8DataBits == 5) {
            tios.c_cflag |= CS5;
        }
        else if (ui8DataBits == 6) {
            tios.c_cflag |= CS6;
        }
        else if (ui8DataBits == 7) {
            tios.c_cflag |= CS7;
        }
        else if (ui8DataBits == 8) {
            tios.c_cflag |= CS8;
        }
        else {
            checkAndLogMsg ("Serial::init", Logger::L_MildError,
                            "data bits must be 5, 6, 7, or 8\n");
            close (_fdComPort);
            _fdComPort = -1;
            return -13;
        }
        if (ui8StopBits == 1) {
            // Nothing to do by default
        }
        else if (ui8StopBits == 2) {
              tios.c_cflag |= CSTOPB;
        }
        else {
            checkAndLogMsg ("Serial::init", Logger::L_MildError,
                            "stop bits must be 1 or 2\n");
            close (_fdComPort);
            _fdComPort = -1;
            return -14;
        }
        if (tcsetattr (_fdComPort, TCSANOW, &tios)) {
            checkAndLogMsg ("Serial::init", Logger::L_MildError,
                            "tcsetattr failed on %s; errno = %d (%s)\n",
                            pszPort, errno, strerror (errno));
            close (_fdComPort);
            return -15;
        }
        return 0;
    #endif
}

int Serial::setXONXOFFHandshaking (bool bEnable)
{
    if (_pS2N != NULL) {
        if (_pS2N->setXONXOFF (bEnable) < 0) {
            return -1;
        }
        return 0;
    }
    #if defined (WIN32)
        DCB dcbComPort;
        memset (&dcbComPort, 0, sizeof (DCB));
        dcbComPort.DCBlength = sizeof (DCB);
        if (!GetCommState (_hComPort, &dcbComPort)) {
            checkAndLogMsg ("Serial::setRTSCTSHandshaking", Logger::L_Warning,
                            "GetCommState failed with error %d\n", GetLastError());
            return -2;
        }

        if (bEnable) {
            dcbComPort.fOutX = dcbComPort.fInX = TRUE;
        }
        else {
            dcbComPort.fOutX = dcbComPort.fInX = FALSE;
        }

        if (!SetCommState (_hComPort, &dcbComPort)) {
            checkAndLogMsg ("Serial::init", Logger::L_Warning,
                            "SetCommState failed with error %d\n", GetLastError());
            return -3;
        }
        return 0;
    #else
        struct termios tios;
        if (tcgetattr (_fdComPort, &tios)) {
            checkAndLogMsg ("Serial::setXONXOFFHandshaking", Logger::L_MildError,
                            "tcgetattr failed; errno = %d (%s)\n",
                            errno, strerror (errno));
            return -4;
        }
        if (bEnable) {
            tios.c_iflag |= IXOFF;
            tios.c_iflag |= IXON;
        }
        else {
            tios.c_iflag &= ~IXOFF;
            tios.c_iflag &= ~IXON;
        }
        if (tcsetattr (_fdComPort, TCSANOW, &tios)) {
            checkAndLogMsg ("Serial::setXONXOFFHandshaking", Logger::L_MildError,
                            "tcsetattr failed; errno = %d (%s)\n",
                             errno, strerror (errno));
            return -5;
        }
        return 0;
    #endif
}

int Serial::getXONXOFFHandshaking (void)
{
    if (_pS2N != NULL) {
        return _pS2N->getXONXOFFState();
    }
    #if defined (WIN32)
        DCB dcbComPort;
        memset (&dcbComPort, 0, sizeof (DCB));
        dcbComPort.DCBlength = sizeof (DCB);
        if (!GetCommState (_hComPort, &dcbComPort)) {
            checkAndLogMsg ("Serial::setRTSCTSHandshaking", Logger::L_Warning,
                            "GetCommState failed with error %d\n", GetLastError());
            return -1;
        }
        if ((dcbComPort.fOutX == FALSE) && (dcbComPort.fInX == FALSE)) {
            return 0;
        }
        else if ((dcbComPort.fOutX == TRUE) && (dcbComPort.fInX == TRUE)) {
            return 1;
        }
        else {
            // There is an inconsistency between the XON setting and XOFF setting
            return -2;
        }
    #else
        struct termios tios;
        if (tcgetattr (_fdComPort, &tios)) {
            checkAndLogMsg ("Serial::getXONXOFFHandshaking", Logger::L_MildError,
                            "tcgetattr failed; errno = %d (%s)\n",
                            errno, strerror (errno));
            return -3;
        }
        if ((tios.c_iflag & IXON) && (tios.c_iflag & IXOFF)) {
            return 1;
        }
        else if ((tios.c_iflag & IXON) || (tios.c_iflag & IXOFF)) {
            // There is an inconsistency between the XON setting and XOFF setting
            return -4;
        }
        else {
            return 0;
        }
    #endif
}

int Serial::setRTSCTSHandshaking (bool bEnableRTS, bool bEnableCTS)
{
    if (_pS2N != NULL) {
        if (_pS2N->setRTSCTSHandshake (bEnableRTS & bEnableCTS) < 0) {
            return -1;
        }
        return 0;
    }
    #if defined (WIN32)
        DCB dcbComPort;
        memset (&dcbComPort, 0, sizeof (DCB));
        dcbComPort.DCBlength = sizeof (DCB);
        if (!GetCommState (_hComPort, &dcbComPort)) {
            checkAndLogMsg ("Serial::setRTSCTSHandshaking", Logger::L_Warning,
                            "GetCommState failed with error %d\n", GetLastError());
            return -2;
        }

        if (bEnableRTS) {
            dcbComPort.fRtsControl = RTS_CONTROL_HANDSHAKE;
        }
        else {
            dcbComPort.fRtsControl = RTS_CONTROL_DISABLE;
        }
        if (bEnableCTS) {
            dcbComPort.fOutxCtsFlow = TRUE;
        }
        else {
            dcbComPort.fOutxCtsFlow = FALSE;
        }

        if (!SetCommState (_hComPort, &dcbComPort)) {
            checkAndLogMsg ("Serial::setRTSCTSHandshaking", Logger::L_Warning,
                            "SetCommState failed with error %d\n", GetLastError());
            return -3;
        }
        return 0;
    #else
        struct termios tios;
        if (tcgetattr (_fdComPort, &tios)) {
            checkAndLogMsg ("Serial::setRTSCTSHandshaking", Logger::L_MildError,
                            "tcgetattr failed; errno = %d (%s)\n",
                            errno, strerror (errno));
            return -4;
        }
        if ((bEnableRTS) && (bEnableCTS)) {
            tios.c_cflag |= CRTSCTS;
        }
        else if ((!bEnableRTS) && (!bEnableCTS)) {
            tios.c_cflag &= ~CRTSCTS;
        }
        else {
            checkAndLogMsg ("Serial::setRTSCTSHandshaking", Logger::L_MildError,
                            "independent control over RTS and CTS is not supported; both must be enabled or disabled together\n");
            return -5;
        }
        if (tcsetattr (_fdComPort, TCSANOW, &tios)) {
            checkAndLogMsg ("Serial::setRTSCTSHandshaking", Logger::L_MildError,
                            "tcsetattr failed; errno = %d (%s)\n",
                             errno, strerror (errno));
            return -6;
        }
        return 0;
    #endif
}

int Serial::getRTSCTSHandshaking (void)
{
    if (_pS2N != NULL) {
        if (_pS2N->getRTSCTSHandshake()) {
            return 3;
        }
        return 0;
    }
    #if defined (WIN32)
        DCB dcbComPort;
        memset (&dcbComPort, 0, sizeof (DCB));
        dcbComPort.DCBlength = sizeof (DCB);
        if (!GetCommState (_hComPort, &dcbComPort)) {
            checkAndLogMsg ("Serial::setRTSCTSHandshaking", Logger::L_Warning,
                            "GetCommState failed with error %d\n", GetLastError());
            return -1;
        }
        int rc = 0;
        if (dcbComPort.fRtsControl == RTS_CONTROL_HANDSHAKE) {
            rc |= 0x01;
        }
        if (dcbComPort.fOutxCtsFlow == TRUE) {
            rc |= 0x02;
        }
        return rc;
    #else
        struct termios tios;
        if (tcgetattr (_fdComPort, &tios)) {
            checkAndLogMsg ("Serial::getRTSCTSHandshaking", Logger::L_MildError,
                            "tcgetattr failed; errno = %d (%s)\n",
                            errno, strerror (errno));
            return -3;
        }
        if (tios.c_cflag & CRTSCTS) {
            return 1;
        }
        else {
            return 0;
        }
    #endif
}

int Serial::setDTR (bool bValue)
{
    if (_pS2N != NULL) {
        return _pS2N->setDTR (bValue);
    }
    #if defined (WIN32)
        DCB dcbComPort;
        memset (&dcbComPort, 0, sizeof (DCB));
        dcbComPort.DCBlength = sizeof (DCB);
        if (!GetCommState (_hComPort, &dcbComPort)) {
            checkAndLogMsg ("Serial::setDTR", Logger::L_Warning,
                            "GetCommState failed with error %d\n", GetLastError());
            return -1;
        }

        if (bValue) {
            dcbComPort.fDtrControl = DTR_CONTROL_ENABLE;
        }
        else {
            dcbComPort.fDtrControl = DTR_CONTROL_DISABLE;
        }
        if (!SetCommState (_hComPort, &dcbComPort)) {
            checkAndLogMsg ("Serial::setDTR", Logger::L_Warning,
                            "SetCommState failed with error %d\n", GetLastError());
            return -2;
        }
        return 0;
    #else
        int iModemCtrlBits;
        if (ioctl (_fdComPort, TIOCMGET, &iModemCtrlBits)) {
            checkAndLogMsg ("Serial::setDTR", Logger::L_MildError,
                            "ioctl() failed when attempting to get modem control bits; errno = %d (%s)\n",
                            errno, strerror (errno));
            return -3;
        }
        if (bValue) {
            iModemCtrlBits |= TIOCM_DTR;
        }
        else {
            iModemCtrlBits &= ~TIOCM_DTR;
        }
        if (ioctl (_fdComPort, TIOCMSET, &iModemCtrlBits)) {
            checkAndLogMsg ("Serial::setDTR", Logger::L_MildError,
                            "ioctl() failed when attempting to set modem control bits; errno = %d (%s)\n",
                            errno, strerror (errno));
            return -4;
        }
        return 0;
    #endif
}

int Serial::getDTR (void)
{
    if (_pS2N != NULL) {
        return _pS2N->getDTRState();
    }
    #if defined (WIN32)
        DCB dcbComPort;
        memset (&dcbComPort, 0, sizeof (DCB));
        dcbComPort.DCBlength = sizeof (DCB);
        if (!GetCommState (_hComPort, &dcbComPort)) {
            checkAndLogMsg ("Serial::getDTR", Logger::L_Warning,
                            "GetCommState failed with error %d\n", GetLastError());
            return -1;
        }

        if (dcbComPort.fDtrControl == DTR_CONTROL_DISABLE) {
            return 0;
        }
        else if (dcbComPort.fDtrControl == DTR_CONTROL_ENABLE) {
            return 1;
        }
        else {
            // DTR is configured for handshaking - hence it cannot be set or get by the application
            return -2;
        }
    #else
        int iModemCtrlBits;
        if (ioctl (_fdComPort, TIOCMGET, &iModemCtrlBits)) {
            checkAndLogMsg ("Serial::getDTR", Logger::L_MildError,
                            "ioctl() failed when attempting to get modem control bits; errno = %d (%s)\n",
                            errno, strerror (errno));
            return -3;
        }
        if (iModemCtrlBits & TIOCM_DTR) {
            return 1;
        }
        else {
            return 0;
        }
    #endif
}

int Serial::setRTS (bool bValue)
{
    if (_pS2N != NULL) {
        if (_pS2N->setRTS (bValue) < 0) {
            return -1;
        }
    }
    #if defined (WIN32)
        if (bValue) {
            if (!EscapeCommFunction (_hComPort, SETRTS)) {
                checkAndLogMsg ("Serial::setRTS", Logger::L_MildError,
                                "EscapeCommFunction() failed when setting RTS with error %d\n", GetLastError());
                return -2;
            }
        }
        else {
            if (!EscapeCommFunction (_hComPort, CLRRTS)) {
                checkAndLogMsg ("Serial::setRTS", Logger::L_MildError,
                                "EscapeCommFunction() failed when clearing RTS with error %d\n", GetLastError());
                return -3;
            }
        }
        return 0;
    #else
        int iModemCtrlBits;
        if (ioctl (_fdComPort, TIOCMGET, &iModemCtrlBits)) {
            checkAndLogMsg ("Serial::setRTS", Logger::L_MildError,
                            "ioctl() failed when attempting to get modem control bits; errno = %d (%s)\n",
                            errno, strerror (errno));
            return -4;
        }
        if (bValue) {
            iModemCtrlBits |= TIOCM_RTS;
        }
        else {
            iModemCtrlBits &= ~TIOCM_RTS;
        }
        if (ioctl (_fdComPort, TIOCMSET, &iModemCtrlBits)) {
            checkAndLogMsg ("Serial::setRTS", Logger::L_MildError,
                            "ioctl() failed when attempting to set modem control bits; errno = %d (%s)\n",
                            errno, strerror (errno));
            return -5;
        }
        return 0;
    #endif
}

int Serial::setReadTimeout (uint32 ui32TimeoutInMS)
{
    if (_pS2N != NULL) {
        return -1; // Not implemented
    }
    #if defined (WIN32)
        COMMTIMEOUTS cto;
        if (!GetCommTimeouts (_hComPort, &cto)) {
            checkAndLogMsg ("Serial::setReadTimeout", Logger::L_MildError,
                            "GetCommTimeouts failed with error %d\n", GetLastError());
            return -2;
        }
        cto.ReadIntervalTimeout = 0;
        cto.ReadTotalTimeoutMultiplier = 0;
        cto.ReadTotalTimeoutConstant = ui32TimeoutInMS;
        if (!SetCommTimeouts (_hComPort, &cto)) {
            checkAndLogMsg ("Serial::setReadTimeout", Logger::L_MildError,
                            "SetCommTimeouts failed with error %d\n", GetLastError());
            return -3;
        }
        return 0;
    #else
        return -4;
    #endif
}

int Serial::setWriteTimeout (uint32 ui32TimeoutInMS)
{
    if (_pS2N != NULL) {
        return -1; // Not implemented
    }
    #if defined (WIN32)
        COMMTIMEOUTS cto;
        if (!GetCommTimeouts (_hComPort, &cto)) {
            checkAndLogMsg ("Serial::setWriteTimeout", Logger::L_MildError,
                            "GetCommTimeouts failed with error %d\n", GetLastError());
            return -2;
        }
        cto.WriteTotalTimeoutMultiplier = 0;
        cto.WriteTotalTimeoutConstant = ui32TimeoutInMS;
        if (!SetCommTimeouts (_hComPort, &cto)) {
            checkAndLogMsg ("Serial::setWriteTimeout", Logger::L_MildError,
                            "SetCommTimeouts failed with error %d\n", GetLastError());
            return -3;
        }
        return 0;
    #else
        return -4;
    #endif
}

int Serial::getReceiveBufferSize (void)
{
    if (_pS2N != NULL) {
        return -1; // Not implemented
    }
    #if defined (WIN32)
        COMMPROP cp;
        memset (&cp, 0, sizeof (COMMPROP));
        if (!GetCommProperties (_hComPort, &cp)) {
            checkAndLogMsg ("Serial::getReceiveBufferSize", Logger::L_MildError,
                            "GetCommProperties failed with error %d\n", GetLastError());
            return -2;
        }
        return (int) cp.dwCurrentRxQueue;
    #else
        return -3;
    #endif
}

int Serial::getBytesAvailable (void)
{
    if (_pS2N != NULL) {
        return -1; // Not implemented
    }
    #if defined (WIN32)
        COMSTAT cs;
        memset (&cs, 0, sizeof (COMSTAT));
        if (!ClearCommError (_hComPort, NULL, &cs)) {
            checkAndLogMsg ("Serial::getBytesAvailable", Logger::L_MildError,
                            "ClearCommError failed with error %d\n", GetLastError());
            return -2;
        }
        return (int) cs.cbInQue;
    #else
        int iBytesAvailable;
        if (ioctl (_fdComPort, FIONREAD, &iBytesAvailable)) {
            checkAndLogMsg ("Serial::getBytesAvailable", Logger::L_MildError,
                            "ioctl failed; errno = %d (%s)\n",
                            errno, strerror (errno));
            return -3;
        }
        return iBytesAvailable;
    #endif
}

int Serial::read (void *pBuf, uint32 ui32NumBytes)
{
    if (_pS2N != NULL) {
        return _pS2N->getline ((unsigned char *) pBuf, ui32NumBytes);
    }
    #if defined (WIN32)
        DWORD dwBytesRead;
        if (!ReadFile (_hComPort, pBuf, ui32NumBytes, &dwBytesRead, &_oRead)) {
            if (GetLastError() != ERROR_IO_PENDING) {
                checkAndLogMsg ("Serial::read", Logger::L_MildError,
                                "ReadFile failed with error %d\n", GetLastError());
                return -1;
            }
            else {
                if (!GetOverlappedResult (_hComPort, &_oRead, &dwBytesRead, TRUE)) {
                    checkAndLogMsg ("Serial::read", Logger::L_MildError,
                                    "GetOverlappedResult failed with error %d\n", GetLastError());
                    return -2;
                }
            }
        }
        if ((_pDebugReadFile) && (dwBytesRead > 0)) {
            fprintf (_pDebugReadFile, "Serial::read: <");
            for (DWORD dw = 0; dw < dwBytesRead; dw++) {
                fprintf (_pDebugReadFile, "%x ", (int) ((uint8*)pBuf)[dw]);
            }
            fprintf (_pDebugReadFile, ">\n");
        }
        return (int) dwBytesRead;
    #else
        int rc;
        if ((rc = ::read (_fdComPort, pBuf, ui32NumBytes)) < 0) {
            checkAndLogMsg ("Serial::read", Logger::L_MildError,
                            "read failed; errno = %d (%s)\n",
                            errno, strerror (errno));
            return -3;
        }
        if (_pDebugReadFile) {
            fprintf (_pDebugReadFile, "Serial::read: <");
            for (int i = 0; i < rc; i++) {
                fprintf (_pDebugReadFile, "%x ", (int) ((uint8*)pBuf)[i]);
            }
            fprintf (_pDebugReadFile, ">\n");
        }
        return rc;
    #endif
}

int Serial::write (const void *pBuf, uint32 ui32NumBytes)
{
    if (_pS2N != NULL) {
        return _pS2N->putline ((unsigned char *) pBuf, ui32NumBytes);
    }
    #if defined (WIN32)
        DWORD dwBytesWritten;
        if (!WriteFile (_hComPort, pBuf, ui32NumBytes, &dwBytesWritten, &_oWrite)) {
            if (GetLastError() !=  ERROR_IO_PENDING) {
                checkAndLogMsg ("Serial::write", Logger::L_MildError,
                                "WriteFile failed with error %d\n", GetLastError());
                return -1;
            }
            else {
                if (!GetOverlappedResult (_hComPort, &_oWrite, &dwBytesWritten, TRUE)) {
                    checkAndLogMsg ("Serial::write", Logger::L_MildError,
                                    "GetOverlappedResult failed with error %d\n", GetLastError());
                    return -2;
                }
            }
        }
        return (int) dwBytesWritten;
    #else
        int rc;
        if ((rc = ::write (_fdComPort, pBuf, ui32NumBytes)) < 0) {
            checkAndLogMsg ("Serial::write", Logger::L_MildError,
                            "write failed; errno = %d (%s)\n",
                            errno, strerror (errno));
            return -3;
        }
        return rc;
    #endif
}

int Serial::flushInput (void)
{
    if (_pS2N != NULL) {
        return -1; // Not implemented
    }
    #if defined (WIN32)
        return -2;
    #else
        if (tcflush (_fdComPort, TCIFLUSH)) {
            checkAndLogMsg ("Serial::flushInput", Logger::L_MildError,
                            "tcflush failed; errno = %d (%s)\n",
                            errno, strerror (errno));
            return -3;
        }
        return 0;
    #endif
}

int Serial::flushOutput (void)
{
    if (_pS2N != 0) {
        return -1; // Not implemented
    }
    #if defined (WIN32)
        return -2;
    #else
        if (tcflush (_fdComPort, TCOFLUSH)) {
            checkAndLogMsg ("Serial::flushOutput", Logger::L_MildError,
                            "tcflush failed; errno = %d (%s)\n",
                            errno, strerror (errno));
            return -3;
        }
        return 0;
    #endif
}

#if defined (UNIX)
    speed_t Serial::mapDTESpeed (uint32 ui32DTESpeed)
    {
        switch (ui32DTESpeed) {
            case 0:
                return B0;

            case 50:
                return B50;

            case 75:
                return B75;

            case 110:
                return B110;

            case 134:
                return B134;

            case 150:
                return B150;

            case 200:
                return B200;

            case 300:
                return B300;

            case 600:
                return B600;

            case 1200:
                return B1200;

            case 2400:
                return B2400;

            case 4800:
                return B4800;

            case 9600:
                return B9600;

            case 19200:
                return B19200;

            case 38400:
                return B38400;

            case 57600:
                return B57600;

            case 115200:
                return B115200;

            case 230400:
                return B230400;

            // In the GNU library, the speeds of serial lines is measured in
            // bits per second as input, and return speed values measured in
            // bits per second.
            // Other libraries require speeds to be indicated by special codes.
            // For POSIX.1 portability, you must use one of the following
            // symbols to represent the speed; their precise numeric values
            // are system-dependent, but each name has a fixed meaning: B110
            // stands for 110 bps, B300 for 300 bps, and so on.
            // There is no portable way to represent any speed, but these
            // are the only speeds that typical serial lines can support.
            // B0 B50 B75 B110 B134 B150 B200 B300 B600 B1200 B1800 B2400
            // B4800 B9600 B19200 B38400 B57600 B115200 B230400 B460800
			//
            // Apparently on MacOS the maximum speed allowed for serial lines
            // is B230400. (At least until GCC 4.0.1)
            #if !defined (OSX)
                case 460800:
                    return B460800;

                case 500000:
                    return B500000;

                case 576000:
                    return B576000;

                case 921600:
                    return B921600;

                case 1000000:
                    return B1000000;

                case 1152000:
                    return B1152000;

                case 1500000:
                    return B1500000;

                case 2000000:
                    return B2000000;

                case 2500000:
                    return B2500000;

                case 3000000:
                    return B3000000;

                case 3500000:
                    return B3500000;

                case 4000000:
                    return B4000000;
            #endif

            default:
                checkAndLogMsg ("Serial::mapDTESpeed", Logger::L_MildError,
                                "unknown DTE speed %lu\n", ui32DTESpeed);
                return B0;
        }
    }
#endif

