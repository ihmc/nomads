/*
 * SoapMessage.h
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

#ifndef INCL_SOAPMESSAGE_H
#define INCL_SOAPMESSAGE_H

#include "StrClass.h"

namespace NOMADSUtil
{
    class DOMDocument;
    /*
    class SoapMessage
    {
        public:
            SoapMessage (void);
            ~SoapMessage (void);

            bool init (const char *pszBuf);
            bool init (DOMDocument *pDOMDocument);

            static bool validateSoapMessage (DOMDocument *pDOMDocument);
            
            // Note that the SOAP header is optional
            bool setHeader (const char *pszBuf);
            bool setHeader (DOMDocument *pDOMDocument);
            
            bool setBody (const char *pszBuf);
            bool setBody (DOMDocument *pDOMDocument);

            DOMDocument * getHeader (void);
            DOMDocument * getBody (void);
            DOMDocument * getMessage (void);
            
            String getHeaderAsString (void);
            String getBodyAsString (void);
            String getMessageAsString (void);

            bool setVersion (const char *pszVersion);
            String getVersion (void);

        private:
            static bool createDOMDocument (const char *pszBuf, DOMDocument *pDOMDocument);
            static String serializeDOMDocument (DOMDocument *pDOMDocument); 
            DOMDocument * buildSoapMessage (void);

        private:
            DOMDocument *_pSoapHeader;
            DOMDocument *_pSoapBody;
            char *pszVersion;
    };

    inline bool SoapMessage::setHeader (const char *pszBuf) 
    {
        if (!_pSoapHeader) {
            _pSoapHeader = new DOMDocument();
        }
        
        if (createDOMDocument (pszBuf, _pSoapHeader)) {
            return true;
        }

        delete _pSoapHeader;
        return false;
    }

    inline bool SoapMessage::setHeader (DOMDocument *pDOMDocument)
    {
        _pSoapHeader = pDOMDocument;
    }

    inline bool SoapMessage::setBody (const char *pszBuf)
    {
        if (!_pSoapBody) {
            _pSoapBody = new DOMDocument();
        }

        if (createDOMDocument (pszBuf, _pSoapBody)) {
            return true;
        }

        delete _pSoapBody;
        return false;
    }

    inline bool SoapMessage::setBody (DOMDocument *pDOMDocument)
    {
            _pSoapBody = pDOMDocument;
    }

    inline DOMDocument * SoapMessage::getHeader (void)
    {
        return _pSoapHeader;
    }

    inline DOMDocument * SoapMessage::getBody (void)
    {
            return _pSoapBody;
    }

    inline DOMDocument * SoapMessage::getMessage (void)
    {
            return createSoapMessage();
    }

    inline String SoapMessage::getHeaderAsString (void)
    {
        return serializeDOMDocument (_pSoapHeader);
    }

    inline String SoapMessage::getBodyAsString (void)
    {
        return serializeDOMDocument (_pSoapBody);
    }

    inline String SoapMessage::getMessageAsString (void)
    {
        return serializeDOMDocument (buildSoapMessage());
    }

    inline bool SoapMessage::setVersion (const char *pszVersion)
    {
        _pszVersion = pszVersion;
    }

    inline String SoapMessage::getVersion (void)
    {
        return _pszVersion;
    }
    */

}

#endif   // #ifndef INCL_SOAPMESSAGE_H
