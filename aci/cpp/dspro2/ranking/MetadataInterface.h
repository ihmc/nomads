/* 
 * MetadataInterface.h
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
 * Created on July 5, 2013, 11:52 AM
 */

#ifndef INCL_METADATA_INTERFACE_H
#define	INCL_METADATA_INTERFACE_H

#include "Pedigree.h"
#include "PreviousMessageIds.h"

#include "FTypes.h"
#include "PtrLList.h"

namespace NOMADSUtil
{
    class Writer;
}

namespace IHMC_ACI
{
    /**
     * Prediction classes names used in the classifier
     */
    struct PredictionClass
    {
        static const NOMADSUtil::String USEFUL;
        static const NOMADSUtil::String NOT_USEFUL;
    };

    struct MetadataType
    {
        static const NOMADSUtil::String INTEGER8;  // int8 and uint8
        static const NOMADSUtil::String INTEGER16; // int16 and uint16
        static const NOMADSUtil::String INTEGER32; // int32 and uint32
        static const NOMADSUtil::String INTEGER64; // int64 and uint64
        static const NOMADSUtil::String FLOAT;     // float
        static const NOMADSUtil::String DOUBLE;    // double
        static const NOMADSUtil::String TEXT;      // const char *
    };

    struct MetadataValue
    {
        static const NOMADSUtil::String UNKNOWN;
    };

    class MetadataInterface
    {
        public:
            enum MetadataWrapperType
            {
                Meta,
                List
            };

            MetadataInterface (void);
            virtual ~MetadataInterface (void);

            /**
             * Return the name of the uiIndex-th field if it exists, NULL otherwise
             */
            virtual const char * getFieldName (unsigned int uiIndex) const = 0;
            virtual const char * getFieldType (unsigned int uiIndex) const = 0;
            virtual const char * getFieldValueByIndex (unsigned int uiIndex) const = 0;

            /**
             * Stores the field value in "pszValue". If the given field name
             * exists and the value is known the function returns 0.
             * If the given field name exists and the value is unknown the
             * function returns 1.
             * If the given field name doesn't exist, or if there was an error,
             * it returns a negative number.
             *
             * NOTE:  ppszValue points to a copy of the field that has to be
             *        deallocated by the caller.
             */
            int getFieldValue (const char *pszFieldName, char **ppszValue) const;
            int getFieldValue (const char *pszFieldName, NOMADSUtil::String &value) const;
            int getFieldValue (const char *pszFieldName, int8 *pi8Value) const;
            int getFieldValue (const char *pszFieldName, uint8 *pui8Value) const;
            int getFieldValue (const char *pszFieldName, int16 *pi16Value) const;
            int getFieldValue (const char *pszFieldName, uint16 *pui16Value) const;
            int getFieldValue (const char *pszFieldName, int32 *pi32Value) const;
            int getFieldValue (const char *pszFieldName, uint32 *pui32Value) const;
            int getFieldValue (const char *pszFieldName, int64 *pi64value) const;
            int getFieldValue (const char *pszFieldName, uint64 *pui64Value) const;
            int getFieldValue (const char *pszFieldName, float *pfValue) const;
            int getFieldValue (const char *pszFieldName, double *pdValue) const;

            int getFieldValue (Pedigree &pedigree) const;
            int getFieldValue (PreviousMessageIds &pPreviousMessageIds) const;

            virtual MetadataWrapperType getMetadataType (void) = 0;

            /**
             * Returns TRUE if the field named "fieldName"
             * has an unknown value in this metadata instance.
             */
            bool isFieldUnknown (const char *pszFieldName) const;
            virtual bool isFieldAtIndexUnknown (unsigned int uiIndex) const = 0;
            static bool isFieldValueUnknown (const char *pszValue);

            virtual int setFieldValue (const char *pszAttribute, const char *pszValue) = 0;

            /**
             * Set the given message id and set "usage" to unknown.
             */
            int setMessageIdAndUsage (const char *pszMessageId);

            /**
             * Set the given fields values just if they where unknown before.
             * Always set the pedigree.
             */
            int setFixedMetadataFields (int64 i64ExpirationTime, int64 i64ReceiverTimeStamp,
                                        const char *pszSource, int64 i64SourceTimeStamp,
                                        const char *pszNodeID);

            virtual int resetFieldValue (const char *pszFieldName) = 0;

            bool operator == (const MetadataInterface &rhsMetadataInterface);

        public:
            static const char * const NO_REFERRED_OBJECT;

            static const char * const XML_METADATA_MIME_TYPE; 

            // Fixed Attribute Names
            static const char * const MESSAGE_ID;  // DisService Message ID that identifies
                                                   // this metadata
            static const char * const REFERS_TO;   // DisService Message ID that identifies
                                                   // the object described by this metadata
            static const char * const REFERRED_DATA_OBJECT_ID;   // object ID that identifies
                                                                 // the object described by this metadata
            static const char * const REFERRED_DATA_INSTANCE_ID;   // version ID that identifies
                                                                   // the version of the object described
                                                                   // by this metadata
            static const char * const REFERRED_DATA_SIZE;   // the size of the referred object
            static const char * const PREV_MSG_ID; // the ID of the previous message that was sent
                                                   // i the prev_msg_id is not set, it mean that it
                                                   // is the first mesage sent to the peer
            static const char * const ANNOTATION_TARGET_OBJ_ID;
            static const char * const RANKS_BY_TARGET_NODE_ID;
            static const char * const CHECKSUM;
            static const char * const DATA_CONTENT;
            static const char * const DATA_FORMAT;
            static const char * const CLASSIFICATION;
            static const char * const DESCRIPTION;
            static const char * const LATITUDE;
            static const char * const LONGITUDE;
            static const char * const LEFT_UPPER_LATITUDE;
            static const char * const LEFT_UPPER_LONGITUDE;
            static const char * const RIGHT_LOWER_LATITUDE;
            static const char * const RIGHT_LOWER_LONGITUDE;
            static const char * const SOURCE;
            static const char * const SOURCE_TIME_STAMP;
            static const char * const RECEIVER_TIME_STAMP;
            static const char * const EXPIRATION_TIME;
            static const char * const RELEVANT_MISSION;
            static const char * const LOCATION;
            static const char * const PEDIGREE;
            static const char * const IMPORTANCE;
            static const char * const SOURCE_RELIABILITY;
            static const char * const INFORMATION_CONTENT;
            static const char * const VOI_LIST;

            static const char * const TARGET_ID;
            static const char * const TARGET_ROLE;
            static const char * const TARGET_TEAM;

            static const char * const USAGE;
            static const char * const APPLICATION_METADATA;

            // Fixed Attribute Default Values
            static const float LEFT_UPPER_LATITUDE_UNSET;
            static const float LEFT_UPPER_LONGITUDE_UNSET;
            static const float RIGHT_LOWER_LATITUDE_UNSET;
            static const float RIGHT_LOWER_LONGITUDE_UNSET;
            static const int64 SOURCE_TIME_STAMP_UNSET;
            static const int64 RECEIVER_TIME_STAMP_UNSET;
            static const int64 EXPIRATION_TIME_UNSET;
            static const double IMPORTANCE_UNSET;

            static const NOMADSUtil::String UNKNOWN;

        protected:
            /**
             * Returns the value of the element with field name pszFieldName.
             * NOTE: ppszValue will point to the element in the data structure,
             *       findFieldValue() does not make a copy.
             */
            virtual int findFieldValue (const char *pszFieldName, const char **ppszValue) const = 0;

            int write (uint16 i, NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize, uint32 &totLength);

        private:
            int writeText (uint16 i, NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize, uint32 &ui32TotLength);
            int writeInt8 (uint16 i, NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize, uint32 &ui32TotLength);
            int writeInt16 (uint16 i, NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize, uint32 &ui32TotLength);
            int writeInt32 (uint16 i, NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize, uint32 &ui32TotLength);
            int writeInt64 (uint16 i, NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize, uint32 &ui32TotLength);
            int writeFloat (uint16 i, NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize, uint32 &ui32TotLength);
            int writeDouble (uint16 i, NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize, uint32 &ui32TotLength);
    };

    typedef NOMADSUtil::PtrLList<MetadataInterface> MetadataList;
}

#endif	/* INCL_METADATA_INTERFACE_H */

