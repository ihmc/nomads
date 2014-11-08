/*
 * BufferWriter.h
 *
 * This file is part of the IHMC Utility Library
 * Copyright (c) IHMC. All Rights Reserved.
 *
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

#ifndef INCL_BUFFER_WRITER_H
#define INCL_BUFFER_WRITER_H

#include "Writer.h"

namespace NOMADSUtil
{

    class BufferWriter : public Writer
    {
        public:
            BufferWriter (void);
            BufferWriter (unsigned long ulInitialSize, unsigned long ulIncrement);
            ~BufferWriter (void);

            // Returns a pointer to the internal buffer
            // NOTE: The buffer returned by this method will be deallocated in the destructor
            const char * getBuffer (void);

            // Returns the length of the buffer (i.e., the number of bytes that have been
            // written into the buffer)
            unsigned long getBufferLength (void) const;

            // Stops managing the buffer and returns a pointer to it
            // NOTE: Once this method is called, the BufferWriter will no longer have access to
            //       the buffer and no further data can be added
            //       The caller is responsible to deallocate the buffer when done
            // NOTE: The actual size of the buffer (i.e., the memory allocated) may be larger
            //       than the value returned by getBufferLength - this should not matter as
            //       free() will take care of releasing all the memory when it is called
            char * relinquishBuffer (void);

            // Stops managing the old buffer (which is returned and all the note
            // of relinquishBuffer apply to the returned old buffer) and starts
            // managing the passed one
            char * reliquishAndSetBuffer (char *pNewBuf, unsigned long ulCount);

            virtual int writeBytes (const void *pBuf, unsigned long ulCount);

            // Resets the buffer, in effect erasing anything that has been previously written to it
            // The buffer is not deallocated
            void reset (void);

        private:
            static const unsigned long DEFAULT_INITIAL_SIZE = 1024;
            static const unsigned long DEFAULT_INCREMENT = 1024;
            char *_pBuf;
            unsigned long _ulBufSize;
            unsigned long _ulPos;
            unsigned long _ulIncrement;
    };

    inline const char * BufferWriter::getBuffer (void)
    {
        return _pBuf;
    }

    inline unsigned long BufferWriter::getBufferLength (void) const
    {
        return _ulPos;
    }

    inline char * BufferWriter::relinquishBuffer (void)
    {
        char *pBuf = _pBuf;
        _pBuf = NULL;
        _ulBufSize = 0;
        _ulPos = 0;
        return pBuf;
    }

    inline void BufferWriter::reset (void)
    {
        _ulPos = 0;
    }
}

#endif   // #ifndef INCL_BUFFER_WRITER_H
