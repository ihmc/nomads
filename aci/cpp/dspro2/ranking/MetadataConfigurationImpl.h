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

#ifndef INCL_METADATA_CONFIGURATION_IMPL_H
#define INCL_METADATA_CONFIGURATION_IMPL_H

#include "FTypes.h"

#include "MetadataConfiguration.h"
#include "DArray2.h"

class TiXmlElement;

namespace NOMADSUtil
{
    class AVList;
}

namespace IHMC_C45
{
    class C45AVList;
}

namespace IHMC_VOI
{
    class MetadataInterface;
}

namespace IHMC_ACI
{
    class MetaData;

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

    class MetadataConfigurationImpl : public IHMC_VOI::MetadataConfiguration
    {
        public:
            static const NOMADSUtil::String XML_HEADER;
            static const NOMADSUtil::String XML_METADATA_ELEMENT;
            static const NOMADSUtil::String XML_FIELD_ELEMENT;
            static const NOMADSUtil::String XML_FIELD_NAME_ELEMENT;
            static const NOMADSUtil::String XML_FIELD_TYPE_ELEMENT;
            static const NOMADSUtil::String XML_FIELD_VALUE_ELEMENT;

            virtual ~MetadataConfigurationImpl (void);

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
            static MetadataConfigurationImpl * getConfiguration (void);
            static MetadataConfigurationImpl * getConfiguration (const char *pszXMLMetadataFields);

            /**
             * Return the existing configuration, if it does not exists, it
             * returns nullptr.
             * If the configuration should be initialized, use the methods above.
             */
            static MetadataConfigurationImpl * getExistingConfiguration (void);

            unsigned int getNumberOfLearningFields (void) const;
            unsigned int getNumberOfFields (void) const;
            NOMADSUtil::String getFieldName (unsigned int uiIdx) const;
            bool isLearningField (unsigned int uiIdx) const;

            const char * getFieldType (const char *pszFieldName) const;
            bool isStringFieldType (const char *pszFieldName) const;

            const MetadataFieldInfo ** getMetadataFieldInfos (uint16 &metadataFieldsNumber) const;
            unsigned short getMessageIdIndex (void) const;
            unsigned short getReceiverTimeStampIndex (void) const;
            unsigned short getRefersToIndex (void) const;
            int16 getMetadataNotWritten (void) const;
            void getMetadataNotWritten (NOMADSUtil::DArray2<NOMADSUtil::String> &nonTransientAttributes) const;

            /**
             * Returns metadata fields used to configure the decision tree.
             * Delete the return value after use.
             */
            IHMC_C45::C45AVList * getMetadataAsStructure (void);

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

        private:
            MetadataConfigurationImpl (void);

            int addCustomMetadata (TiXmlElement *pRoot);
            int addCustomMetadata (NOMADSUtil::AVList *pMetadataFields);

            int setFixedFields (void);
            int setupClassifierConfiguration (NOMADSUtil::AVList *pValueList);

        private:
            static MetadataConfigurationImpl *_pINSTANCE;

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
            const unsigned short _usPrevMsgIdIndex;
            const unsigned short _usSourceIndex;
            const unsigned short _usSourceTimeStampIndex;
            const unsigned short _usExpirationTimeIndex;
            const unsigned short _usRelevantMissionIndex;
            const unsigned short _usLeftUpperLatitudeIndex;
            const unsigned short _usLeftUpperLongitudeIndex;
            const unsigned short _usRightLowerLatitudeIndex;
            const unsigned short _usRightLoweLongitudeIndex;
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

    inline const MetadataFieldInfo ** MetadataConfigurationImpl::getMetadataFieldInfos (uint16 &metadataFieldsNumber) const
    {
        metadataFieldsNumber = _ui16MetadataFieldsNumber;
        return const_cast<const MetadataFieldInfo **>(_pMetadataFieldInfos);
    }

    inline unsigned short MetadataConfigurationImpl::getMessageIdIndex (void) const
    {
        return _usMessageIDIndex;
    }

    inline unsigned short MetadataConfigurationImpl::getReceiverTimeStampIndex (void) const
    {
        return _usReceiverTimeStampIndex;
    }

    inline unsigned short MetadataConfigurationImpl::getRefersToIndex (void) const
    {
        return _usRefersToIndex;
    }

    inline int16 MetadataConfigurationImpl::getMetadataNotWritten (void) const
    {
        return _metadataNotWritten;
    }

    inline IHMC_C45::C45AVList * MetadataConfigurationImpl::getMetadataAsStructure (void)
    {
        return _pAttributesValues;
    }

}

#endif    /* INCL_METADATA_CONFIGURATION_IMPL_H */
