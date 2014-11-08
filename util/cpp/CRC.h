/*
 * CRC.h
 *
 *  Author: Matteo Interlandi
 *  Created on: Nov 24, 2009
 *
 *  This code is derived from Michael Barr implementation of CRC
 *
 *  Copyright (c) 2000 by Michael Barr.  This software is placed into
 *  the public domain and may be used for any purpose.  However, this
 *  notice must not be changed or removed and no warranty is either
 *  expressed or implied by its publication or distribution.
 */

#ifndef CRC_H_
#define CRC_H_

#define CRC16

#ifdef CRC16
typedef unsigned short crc;
#define POLYNOMIAL          0x8005
#define INITIAL_REMAINDER   0x0000
#define FINAL_XOR_VALUE     0x0000
#define CHECK_VALUE         0xBB3D
#elif CRC32
typedef unsigned int crc;
#define POLYNOMIAL          0x04C11DB7
#define INITIAL_REMAINDER   0xFFFFFFFF
#define FINAL_XOR_VALUE     0xFFFFFFFF
#define CHECK_VALUE         0xCBF43926
#endif

namespace NOMADSUtil
{
    class CRC
    {
        public:
            CRC (void);
            virtual ~CRC (void);

            // Method to compute the reusable table
            // This function must be rerun any time the CRC standard is changed or to reset the internal buffer
            int init (void);

            // Append to the internal buffer an 8 byte value
            int update8 (const void *pBuf);

            // Append to the internal buffer a 16 bit (2 byte) value
            // NOTE: The data is converted to big-endian format if this is a little-endian machine
            int update16 (void *pBuf);

            // Append to the internal buffer a 32 bit (4 byte) value
            // NOTE: The data is converted to big-endian format if this is a little-endian machine
            int update32 (void *pBuf);

            // Append to the internal buffer a 64 bit (8 byte) value
            // NOTE: The data is converted to big-endian format if this is a little-endian machine
            int update64 (void *pBuf);

            // Append a NULL-terminated string to the internal buffer
            int update (const char *pszString);

            // Append to the internal buffer the new data
            int update (const void *pBuf, unsigned long ulBufSize);

            // Reset the internal buffer
            int reset (void);

            // Calculate the checksum on the internal buffer
            crc getChecksum (void);

            // Calculate the checksum on buffer passed as a parameter
            crc getChecksum (const void *pBuf, unsigned long ulBufSize);

        private:
            unsigned int reflect (unsigned int data, unsigned char nBits);

            void *_pInternalBuf;
            unsigned long _ulInternalBufSize;
            crc *_pCrcTable;
    };
}

#endif /* CRC_H_ */
