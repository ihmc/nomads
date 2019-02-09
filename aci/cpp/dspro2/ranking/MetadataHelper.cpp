#include "MetadataHelper.h"

#include "Defs.h"
#include "DSSFLib.h"
#include "MetaData.h"
#include "PreviousMessageIds.h"

#include "AVList.h"
#include "BufferReader.h"
#include "Logger.h"
#include "tinyxml.h"
#include "MetadataConfigurationImpl.h"
#include "StringHashset.h"
#include "InstrumentedReader.h"

using namespace NOMADSUtil;
using namespace IHMC_VOI;
using namespace IHMC_ACI;

int IHMC_ACI::getFieldValueAsPrevMessageIds (MetadataInterface *pMetadata, PreviousMessageIds &previousMessageIds)
{
    String value;
    if (pMetadata->getFieldValue (MetadataInterface::PREV_MSG_ID, value) < 0 || value.length() <= 0) {
        return -1;
    }

    previousMessageIds = value.c_str(); // = operator makes a copy
    return 0;
}

bool IHMC_ACI::refersToDataFromSource (MetadataInterface *pMetadata, const char *pszSource)
{
    String referredDataId;
    if (0 != pMetadata->getReferredDataMsgId (referredDataId) || referredDataId.length() <= 0) {
        return false;
    }
    const String dataPublisherId (extractSenderNodeIdFromKey (referredDataId));
    return (dataPublisherId.length() > 0 ? (1 == (dataPublisherId == pszSource)) : false);
}

int IHMC_ACI::setFieldsValues (MetadataInterface *pMetadata, AVList *pFieldsValues)
{
    for (unsigned int i = 0; i < pFieldsValues->getLength(); i++) {
        const String value (pFieldsValues->getValueByIndex (i));
        if (value != MetadataInterface::UNKNOWN) {
            int rc = pMetadata->setFieldValue (pFieldsValues->getAttribute (i), value);
            if (rc < 0) {
                return rc;
            }
        }
    }
    return 0;
}

int IHMC_ACI::setFixedMetadataFields (MetadataInterface *pMetadata, int64 i64ExpirationTime, int64 i64ReceiverTimeStamp,
                                      const char *pszSource, int64 i64SourceTimeStamp, const char *pszNodeId)
{
    const char *pszMethodName = "IHMC_ACI::setFixedMetadataFields";
    int rc = 0;
    int64 timestampValues[3] = { i64ReceiverTimeStamp, i64SourceTimeStamp, i64ExpirationTime };
    const char * timestamps[3] = { MetadataInterface::RECEIVER_TIME_STAMP, MetadataInterface::SOURCE_TIME_STAMP, MetadataInterface::EXPIRATION_TIME };
    for (unsigned int i = 0; i < 3U; i++) {
        const char *pszFieldName = timestamps[i];
        if (pMetadata->isFieldUnknown (pszFieldName)) {
            rc = pMetadata->setFieldValue (pszFieldName, timestampValues[i]);
        }
    }

    pMetadata->getFieldValue (MetadataInterface::RECEIVER_TIME_STAMP, &i64ReceiverTimeStamp);
    pMetadata->getFieldValue (MetadataInterface::SOURCE_TIME_STAMP, &i64SourceTimeStamp);
    if (i64ReceiverTimeStamp < i64SourceTimeStamp) {
        // Time skew!  Fix it by simply setting the source time stamp to the receiver time stamp
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "detected time skew. Setting %s to current time.\n",
                        MetadataInterface::SOURCE_TIME_STAMP);
        pMetadata->setFieldValue (MetadataInterface::SOURCE_TIME_STAMP, i64ReceiverTimeStamp);
    }

    const String source (pszSource == nullptr ? pszNodeId : pszSource);
    if (pMetadata->isFieldUnknown (MetadataInterface::SOURCE) || (pszSource == nullptr)) {
        rc = pMetadata->setFieldValue (MetadataInterface::SOURCE, source);
    }

    if (source != pszNodeId) {
        // The message obviously comes from its source, therefore it does not
        // need to be inserted in the pedigree, therefore saving some space.
        Pedigree pedigree (pszSource);
        if (pMetadata->getFieldValue (pedigree) == 0) {
            pedigree += pszNodeId;
            rc = pMetadata->setFieldValue (MetadataInterface::PEDIGREE, pedigree.toString());
        }
        else {
            rc = pMetadata->setFieldValue (MetadataInterface::PEDIGREE, pszNodeId);
        }
    }

    return rc;
}

MetaData * IHMC_ACI::toMetadata (AVList *pAVList)
{
    if (pAVList == nullptr) {
        return nullptr;
    }
    MetaData *pMetadata = new MetaData();
    if (pMetadata != nullptr) {
        setFieldsValues (pMetadata, pAVList);
    }
    return pMetadata;
}

MetaData * IHMC_ACI::toMetadata (const void *pBuf, uint32 ui32Len)
{
    const char *pszMethodName = "MetadataHelper::toMetadata";
    MetaData *pMetadata = new MetaData();
    if (pMetadata == nullptr) {
        checkAndLogMsg (pszMethodName, memoryExhausted);
    }
    else {
        BufferReader br (pBuf, ui32Len);
        InstrumentedReader ir (&br);
        int rc = pMetadata->read (&ir, ui32Len);
        if (rc < 0) {
            checkAndLogMsg (pszMethodName, Logger::L_MildError, "could not read Metadata. Return code: %lld.\n", rc);
            delete pMetadata;
            pMetadata = nullptr;
        }
        else if (ir.getBytesRead() < ui32Len) {
            checkAndLogMsg (pszMethodName, Logger::L_Warning, "read %lld bytes of Metadata, but the buffer was %u "
                            "bytes. Some of the received fields may have not been read\n", ir.getBytesRead(), ui32Len);
        }
    }
    return pMetadata;
}

int IHMC_ACI::toBuffer (IHMC_VOI::MetadataInterface *pMetadata, Writer *pWriter, bool bOmitTransient)
{
    StringHashset filters;
    if (bOmitTransient) {
        MetadataConfigurationImpl *pMetadataConf = MetadataConfigurationImpl::getConfiguration ();
        const int16 iTransientMetadata = pMetadataConf->getMetadataNotWritten ();
        for (int16 i = 0; i < iTransientMetadata; i++) {
            String filedName (pMetadataConf->getFieldName (i));
            filters.put (filedName);
        }
    }
    return pMetadata->write (pWriter, 0, (filters.getCount() <= 0 ? nullptr : &filters));
}

