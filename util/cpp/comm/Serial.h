/*
 * Serial.h
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

#ifndef INCL_SERIAL_H
#define INCL_SERIAL_H

/*
 * Serial.h
 *
 * Class to communicate with serial ports
 *
 * This file is part of the IHMC Utility Library
 * Copyright (c) IHMC. All Rights Reserved.
 *
 * Written by Niranjan Suri (nsuri@ihmc.us)
 * Changes to support Serial2Net by Steve Choy - U.S. Army Research Laboratory
 *
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

#include "FTypes.h"

#if defined (WIN32)
    //typedef void * HANDLE;
	#define NOMINMAX
    #include <winsock2.h>    // Only to avoid conflicts that occur if some other header file includes winsock2.h after the include below of windows.h
    #include <windows.h>
#elif defined (UNIX)
    #include <termios.h>
#else
    #error Must Define WIN32 or UNIX
#endif

#include <stdio.h>

namespace NOMADSUtil
{
    class Serial2Net;

    class Serial
    {
        public:
            Serial (void);
            ~Serial (void);

            enum ReturnCodes {
                RC_Ok = 0,
                RC_Error = -200,
                RC_EOF = -201,
                RC_TimeOut = -202
            };

            // Initialize the communications port using the specified parameters
            // NOTE: Specifying an <IPAddr>:<Port> as the port will result in the
            // Serial2Net class being used to contact a remote tty server
			// The iIPControlPort parameter is used to optionally specify the ser2net control port
			// A value of 0 results in the default port being computed, a value < 0 implies that
			// there should be no control port, and a value > 0 specifies the control port to be used
            int init (const char *pszPort, uint32 ui32DTESpeed, char chParity = 'N',
                      uint8 ui8DataBits = 8, uint8 ui8StopBits = 1, int iIPControlPort = 0);

            // Enables debugging of data read in from the serial port
            // The data is written to pFile as it is read using the format
            //     Serial::read: < x y z ... >
            // Use setDebugReadFile (stdout) to send output to the console
            // Use setDebugReadFile (NULL) to turn off debug output
            int setDebugReadFile (FILE *pFile);

            // Enable XON/XOFF Handshaking (bi-directionally)
            // Returns 0 if successful, and a negative value in case of error
            int setXONXOFFHandshaking (bool bEnable);

            // Returns the current setting for XON/XOFF handshaking
            // The return value is > 0 if handshaking is enabled, 0 if handshaking is disabled,
            //     and a negative value in case of error
            int getXONXOFFHandshaking (void);

            // Enables the RTS/CTS handshaking for the port
            // Returns 0 if successful, and a negative value in case of error
            int setRTSCTSHandshaking (bool bEnableRTS, bool bEnableCTS);

            // Returns the current setting for RTS and CTS handshaking
            // Return values are as follows:
            //     0 - Neither RTS nor CTS is enabled
            //     1 - RTS is enabled
            //     2 - CTS is enabled
            //     3 - RTS and CTS is enabled
            //     < 0 - Error
            int getRTSCTSHandshaking (void);

            // Sets the DTR line to high or low
            // A true value enables the line or sets it high
            // A false value disables the line or sets it low
            int setDTR (bool bValue);

            // Returns the current setting for the DTR line
            //     0 - DTR is disabled (or low)
            //     1 - DTR is enabled (or high)
            //     < 0 - Error
            int getDTR (void);

            // Sets the RTS line to high or low
            // A true value enables the line or sets it high
            // A false value disables the line or sets it low
            // This call is invalid if RTSCTSHandshaking is enabled
            int setRTS (bool bValue);

            // NOTE: cannot read back RTS line so no getRTS

            // Sets the timeout value for read operations in milliseconds
            // A value of 0 indicates that the read should block indefinitely until
            //     the specified number of bytes are read
            int setReadTimeout (uint32 ui32TimeoutInMS);

            // Sets the timeout value for write operations in milliseconds
            // A value of 0 indicates that the write should block indefinitely until
            //     the specified number of bytes are written
            int setWriteTimeout (uint32 ui32TimeoutInMS);

            // Returns the size of the OS receive buffer for the serial port, if available
            int getReceiveBufferSize (void);

            // Returns the number of bytes available to be read
            int getBytesAvailable (void);

            // Reads upto the specified number of bytes
            // May read fewer than the requested number of bytes in case of a timeout
            // Returns the number of bytes read if successful, or a negative value in case of error
            int read (void *pBuf, uint32 ui32NumBytes);

            // Writes upto the specified number of bytes
            // May write fewer than the requested number of bytes in case of a timeout
            // Returns the number of bytes written if successful, or a negative value in case of error
            int write (const void *pBuf, uint32 ui32NumBytes);

            int flushInput (void);
            int flushOutput (void);

        protected:
            #if defined (UNIX)
                speed_t mapDTESpeed (uint32 ui32DTESpeed);
            #endif

        protected:
            Serial2Net *_pS2N;
            #if defined (WIN32)
                HANDLE _hComPort;
                OVERLAPPED _oRead, _oWrite;
            #else
                int _fdComPort;
            #endif
            FILE *_pDebugReadFile;
    };

    inline int Serial::setDebugReadFile (FILE *pFile)
    {
        _pDebugReadFile = pFile;
        return 0;
    }
}

#endif
