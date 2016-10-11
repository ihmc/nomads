/* 
 * MetadataConfiguration.h
 *
 * This file is part of the IHMC DSPro Library/Component
 * Copyright (c) 2008-2016 IHMC.
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
 *
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on June 22, 2012, 1:34 PM
 */

#ifndef INCL_METADATA_CONFIGURATION_H
#define	INCL_METADATA_CONFIGURATION_H

#include "FTypes.h"
#include "StrClass.h"

class TiXmlElement;

namespace NOMADSUtil
{
    class AVList;
}

namespace IHMC_C45
{
    class C45AVList;
}

namespace IHMC_ACI
{
    class MetaData;
    class MetadataInterface;
    class SQLAVList;

    struct MetadataFieldInfo
    {
        MetadataFieldInfo (const char *pszFieldName, const char *pszFieldType);
        ~MetadataFieldInfo (void);

        bool _bVolatile;
        bool _bNotNull;
        bool _bUsedInPolicies;
        bool _bUsedInLearning;

        const NOMADSUtil::String _sFieldName;
        const NOMADSUtil::String _sFieldType;
    };

    class MetadataConfiguration
    {
        public:
            virtual ~MetadataConfiguration (void);

            /**
             * Initialize MetadataConfiguration.
             * 
             * Allows an application to choose customized metadata fields to
             * add to the fixed fields. The fixed fields are "Message_ID",
             * "Usage", "Receiver_Time_Stamp", "Source", "Source_Time_Stamp",
             * "Expiration_Time", "Relevant_Mission", "Latitude", "Longitude",
             * "Altitude", "Location".
             *
             * When a new MetaData is created, all the
             * fixed fields values are set automatically. The application just
             * need to set the customized fields values.
             */
            static MetadataConfiguration * getConfiguration (void);
            static MetadataConfiguration * getConfiguration (const char *pszXMLMetadataFields);

            /**
             * Return the existing configuration, if it does not exists, it
             * returns NULL.
             * If the configuration should be initialized, use the methods above. 
             */
            static MetadataConfiguration * getExistingConfiguration (void);

            const char * getFieldAtIndex (unsigned int uiIndex) const;
            const char * getFieldType (const char *pszFieldName) const;
            bool isStringFieldType (const char *pszFieldName) const;

            /**
             * Create a MetaData from a SQLAVList instance. In case of error a
             * NULL pointer is returned. Call the method getErrorCode() to
             * retrieve the error code.
             */
            MetaData * createNewMetadata (SQLAVList *pFieldsValues);

            /**
             * Create a MetaData from a buffer. In case of error a
             * NULL pointer is returned. Call the method getErrorCode() to
             * retrieve the error code.
             */
            MetaData * createNewMetadataFromBuffer (const void *pBuf, uint32 ui32Len);
            MetaData * createMetadataFromXML (const char *pszXmlDocument);

            /**
             * Returns a pointer to the xml document. After used, the pointer
             * should be freed.
             */
            char * convertMetadataToXML (MetadataInterface *pMetadata);
            char * convertMetadataAndApplicationMetadataToXML (MetadataInterface *pMetadata);

            const MetadataFieldInfo ** getMetadataFieldInfos (uint16 &metadataFieldsNumber) const;
            unsigned short getMessageIdIndex (void);
            unsigned short getReceiverTimeStampIndex (void);
            unsigned short getRefersToIndex (void);
            int16 getMetadataNotWritten (void);

            /**
             * Returns metadata fields used to configure the decision tree.
             * Delete the return value after use.
             */
            IHMC_C45::C45AVList * getMetadataAsStructure (void);

            /**
             * Returns metadata values used to insert data in a decision tree.
             * Delete the return value after use.
             */
            IHMC_C45::C45AVList * getMetadataAsDataset (MetadataInterface *pMetadata);

            /**
             * Returns metadata values used to make predictions.
             * Delete the return value after use.
             */
            IHMC_C45::C45AVList * getMetadataAsC45List (MetadataInterface *pMetadata);

            /**
             * Returns the current metadata configuration in XML format.
             */
            char * getMetaDataStructureAsXML (void);

            /**
             * Returns the current metadata configuration as SQLAVList instance.
             */
            SQLAVList * getMetaDataStructure (void);

            /**
             * Extracts all the fields used in the learning and returns them in
             * a record format.
             */
            IHMC_C45::C45AVList * getFieldsAsRecord (NOMADSUtil::AVList *pFields);

            /**
             * Returns true whether the metadata was configured to include a field
             * of name pszFieldName. Returns false otherwise.
             */
            bool hasField (const char *pszFieldName);

            /**
             * Set all the possible values for the discrete attributes that
             * will be used in the learning.
             */
            int setMetadataFields (const char *pszXMLMetaDataValues);
            int setMetadataFields (SQLAVList *pMetaDataValues);

        private:
            MetadataConfiguration (void);

            int addCustomMetadata (TiXmlElement *pRoot);
            int addCustomMetadata (SQLAVList *pMetadataFields);

            int setFixedFields (void);
            int setupClassifierConfiguration (SQLAVList *pValueList);

        private:
            static const NOMADSUtil::String XML_HEADER;
            static const NOMADSUtil::String XML_METADATA_ELEMENT;
            static const NOMADSUtil::String XML_FIELD_ELEMENT;
            static const NOMADSUtil::String XML_FIELD_NAME_ELEMENT;
            static const NOMADSUtil::String XML_FIELD_TYPE_ELEMENT;
            static const NOMADSUtil::String XML_FIELD_VALUE_ELEMENT;

            static MetadataConfiguration *_pINSTANCE;

            /*
             * fixed metadata parameters
             */

            //----------------------------------------------
            // NOT WRITTEN
            //----------------------------------------------
            const unsigned short _usMessageIDIndex;
            const unsigned short _usUsageIndex;
            const unsigned short _usReceiverTimeStampIndex;
            const unsigned short _usReferredDataObjectIdIndex;
            const unsigned short _usReferredDataInstanceIdIndex;

            //----------------------------------------------
            // WRITTEN
            //----------------------------------------------
            const unsigned short _usPedigreeIndex;
            const unsigned short _usRefersToIndex;
            const unsigned short _usAnnotationTargetMsgIdIndex;
            const unsigned short _usRanksByTargetIndex;
            const unsigned short _usPrevMsgIdIndex;
            const unsigned short _usSourceIndex;
            const unsigned short _usSourceTimeStampIndex;
            const unsigned short _usExpirationTimeIndex;
            const unsigned short _usRelevantMissionIndex;
            const unsigned short _usLeftUpperLatitudeIndex;
            const unsigned short _usLeftUpperLongitudeIndex;
            const unsigned short _usRightLowerLatitudeIndex;
            const unsigned short _usRightLoweLongitudeIndex;
            const unsigned short _usLocationIndex;
            const unsigned short _usImportanceIndex;
            const unsigned short _usSourceReliabilityIndex;
            const unsigned short _usInformationContentIndex;
            const unsigned short _usDataContentIndex;
            const unsigned short _usDataFomatIndex;
            const unsigned short _usTargetIdIndex;
            const unsigned short _usTargetRoleIndex;
            const unsigned short _usTargetTeamIndex;

            //----------------------------------------------
            // COUNTERS
            //----------------------------------------------

            /*
             * fields not written or read by MetaData::read()
             * and MetaData::write()
             */
            const uint16 _metadataNotWritten;
            const unsigned int _usMetadataFixedFieldsNumber;

            MetadataFieldInfo **_pMetadataFieldInfos;

            uint16 _ui16MetadataFieldsNumber;   // number of fields in the previous two arrays
            uint16 _ui16MetadataLearningNumber; // number of fields used in the learning
            uint16 _ui16VolatileMetadataNumber; // number of fields that are volatile

            IHMC_C45::C45AVList *_pAttributesValues; // used to configure the learning
    };

    inline const MetadataFieldInfo ** MetadataConfiguration::getMetadataFieldInfos (uint16 &metadataFieldsNumber) const
    {
        metadataFieldsNumber = _ui16MetadataFieldsNumber;
        return const_cast<const MetadataFieldInfo **>(_pMetadataFieldInfos);
    }

    inline unsigned short MetadataConfiguration::getMessageIdIndex (void)
    {
        return _usMessageIDIndex;
    }

    inline unsigned short MetadataConfiguration::getReceiverTimeStampIndex (void)
    {
        return _usReceiverTimeStampIndex;
    }

    inline unsigned short MetadataConfiguration::getRefersToIndex (void)
    {
        return _usRefersToIndex;
    }

    inline int16 MetadataConfiguration::getMetadataNotWritten (void)
    {
        return _metadataNotWritten;
    }

    inline IHMC_C45::C45AVList * MetadataConfiguration::getMetadataAsStructure (void)
    {
        return _pAttributesValues;
    }

}

#endif	/* INCL_METADATA_CONFIGURATION_H */

