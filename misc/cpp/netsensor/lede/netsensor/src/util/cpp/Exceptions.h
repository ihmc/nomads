/*
 * Exceptions.h
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

#ifndef INCL_EXCEPTIONS_H
#define INCL_EXCEPTIONS_H

#include <stddef.h>

namespace NOMADSUtil
{

    // Generic Exception class
    class Exception
    {
        public:
            Exception (const char *pszMsg = NULL);
            const char * getMsg (void);
        protected:
            const char *_pszMsg;
    };

    // Exception about parameter
    class ParamException : public Exception
    {
        public:
            ParamException (const char *pszMsg = NULL);
    };

    // FormatException is used when something unexpected occurs
    //     while handling data (such as reading and parsing a file)
    class FormatException : public Exception
    {
        public:
            FormatException (const char *pszMsg = NULL);
    };

    // InvalidStateException is used when an object is in an invalid
    //     state and hence cannot execute the requested method
    class InvalidStateException : public Exception
    {
        public:
            InvalidStateException (const char *pszMsg = NULL);
    };

    // An IO Exception of some kind
    class IOException : public Exception
    {
        public:
            IOException (const char *pszMsg = NULL);
    };

    // An EOF Exception
    class EOFException : public IOException
    {
        public:
            EOFException (const char *pszMsg = NULL);
    };

    // CommException is used in case of communication problems
    class CommException : public IOException
    {
        public:
            CommException (const char *pszMsg = NULL);
    };

    // ProtocolException is used when something unexpected occurs while
    //     while handling or carrying out a protocol (such as a
    //     communications protocol)
    class ProtocolException : public Exception
    {
        public:
            ProtocolException (const char *pszMsg = NULL);
    };

    /**
    * The GroupManagerException is used an error occurs while handling the
    * GroupManager protocol.
    **/
    class GroupManagerException: public Exception
    {
        public :
            GroupManagerException (const char *pszMsg = NULL);
    };


    inline Exception::Exception (const char *pszMsg)
    {
        _pszMsg = pszMsg;
    }

    inline const char * Exception::getMsg (void)
    {
        return _pszMsg;
    }

    inline ParamException::ParamException (const char *pszMsg)
        : Exception (pszMsg)
    {
    }

    inline FormatException::FormatException (const char *pszMsg)
        : Exception (pszMsg)
    {
    }

    inline InvalidStateException::InvalidStateException (const char *pszMsg)
        : Exception (pszMsg)
    {
    }

    inline IOException::IOException (const char *pszMsg)
        : Exception (pszMsg)
    {
    }

    inline EOFException::EOFException (const char *pszMsg)
        : IOException (pszMsg)
    {
    }

    inline CommException::CommException (const char *pszMsg)
        : IOException (pszMsg)
    {
    }

    inline ProtocolException::ProtocolException (const char *pszMsg)
        : Exception (pszMsg)
    {
    }

    inline GroupManagerException::GroupManagerException (const char *pszMsg)
        : Exception (pszMsg)
    {
    }

}

#endif   // #ifndef INCL_EXCEPTIONS_H
