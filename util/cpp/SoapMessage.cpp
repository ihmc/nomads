/*
 * SoapMessage.cpp
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

#include "SoapMessage.h"
/*
#include "xercesc/dom/DOM.hpp"
#include "xercesc/dom/DOMDocument.hpp"
#include "xercesc/util/PlatformUtils.hpp"
#include "xercesc/framework/MemBufInputSource.hpp"
#include "xercesc/parsers/XercesDOMParser.hpp"

#include "Logger.h"
#define checkAndLogMsg if (pLogger) pLogger->logMsg

XERCES_CPP_NAMESPACE_USE

SoapMessage::SoapMessage (void)
{
    const char *pszMethodName = "SoapMessage::SoapMessage";
    try {
        XMLPlatformUtils::Initialize();
    }
    catch (const XMLException &xcp) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError,
                        "failed to initialize Xerces\n");
        XMLPlatformUtils::Terminate();
    }
}

SoapMessage::~SoapMessage (void)
{
    const char *pszMethodName = "SoapMessage::~SoapMessage";

    try {
        XMLPlatformUtils::Terminate();  // Terminate Xerces
    }
    catch (xercesc::XMLException &e) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError,
                        "failed to terminate Xerces\n");
    }
}

bool SoapMessage::init (const char *pszBuf)
{
    const char *pszMethodName = "SoapMessage::init";

    DOMDocument pSoapMessage = new DOMDocument();
    if (createDOMDocument (pszBuf, _pSoapMessage) && validateSoapMessage (pSoapMessage)) {
        DOMDocument *pSoapHeader;
        DOMDocument *pSoapMessage;
        //TODO: parse the document and get the header and the body out of it
        setHeader (pSoapHeader);
        setBody (pSoapBody);
        return true;
    }

    delete pSoapMessage;
    return false;
}

bool SoapMessage::init (DOMDocument *pDOMDocument)
{
    const char *pszMethodName = "SoapMessage::init";

    if (validateSoapMessage (pDOMDocument)) {
        DOMDocument *pSoapHeader;
        DOMDocument *pSoapMessage;
        //TODO: parse the document and get the header and the body out of it
        setHeader (pSoapHeader);
        setBody (pSoapBody);
        return true;
    }
    return false;
}

bool SoapMessage::validateSoapMessage (DOMDocument *pDOMDocument)
{
    const char *pszMethodName = "SoapMessage::validateSoapMessage";
    //TODO: implement this
    return true;
}

bool SoapMessage::createDOMDocument (const char *pszBuf, DOMDocument *pDOMDocument)
{
    const char *pszMethodName = "SoapMessage::createDOMDocument";
    MemBufInputSource *pMemBufIS = new MemBufInputSource ((const XMLByte*) pszBuf,
                                                          strlen (pszBuf),
                                                          "",
                                                          false);
    XercesDOMParser *pParser = new XercesDOMParser();
    try {
        pParser->parse (*pMemBufIS);
    }
    catch (...) {
        pLogger->logMsg (pszMethodName, Logger::L_MildError,
                         "failed to parse; exception occurred\n");
        return false;
    }

    pDOMDocument = pParser->getDocument();
    return true;
}

String SoapMessage::serializeDOMDocument (DOMDocument *pDOMDocument)
{
    const char *pszMethodName = "SoapMessage::serializeDOMDocument";

    DOMImplementation *pImpl = pDOMDocument->getImplementation();
    DOMWriter *pWriter = ((DOMImplementationLS*)pImpl)->createDOMWriter();

    // set feature if the serializer supports the feature/mode
    if (pWriter->canSetFeature (XMLUni::fgDOMWRTSplitCdataSections, gSplitCdataSections))
        pWriter->setFeature (XMLUni::fgDOMWRTSplitCdataSections, gSplitCdataSections);

    if (pWriter->canSetFeature (XMLUni::fgDOMWRTDiscardDefaultContent, gDiscardDefaultContent))
        pWriter->setFeature (XMLUni::fgDOMWRTDiscardDefaultContent, gDiscardDefaultContent);

    if (pWriter->canSetFeature (XMLUni::fgDOMWRTFormatPrettyPrint, gFormatPrettyPrint))
        pWriter->setFeature (XMLUni::fgDOMWRTFormatPrettyPrint, gFormatPrettyPrint);

    if (pWriter->canSetFeature (XMLUni::fgDOMWRTBOM, gWriteBOM))
        pWriter->setFeature (XMLUni::fgDOMWRTBOM, gWriteBOM);

    return pWriter->writeToString (*pDOMDocument);
}

DOMDocument * SoapMessage::buildSoapMessage (void)
{
    DOMDocument *pSoapMessage = new DOMDocument();
    //TODO: create the SOAP envelope, and store in it the header and the
    //body
    return pSoapMessage;
}
*/
