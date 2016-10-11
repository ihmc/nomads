/* 
 * MetaData.h
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
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details..
 */

#ifndef INCL_METADATA_H
#define INCL_METADATA_H

#include "MetadataInterface.h"

namespace NOMADSUtil
{
    class AVList;
    class Reader;
    class Writer;
}

namespace IHMC_ACI
{
    class MetaData;
    struct MetadataFieldInfo;
    class Pedigree;
    class SQLAVList;

    class MetaData : public MetadataInterface
    {
        public:
            MetaData (const MetadataFieldInfo ** const pMetadataFieldInfos,
                      uint16 ui16MetadataFieldsNumber, uint16 i16MetadataNotWritten);
            virtual ~MetaData (void);

            MetaData * clone (void);

            /**
             * Returns a human-readable representation of the content of the
             * metadata.
             */
            const char * toString (void);

            const char * getFieldName (unsigned int uiIndex) const;
            const char * getFieldType (unsigned int uiIndex) const;
            const char * getFieldValueByIndex (unsigned int uiIndex) const;

            MetadataInterface::MetadataWrapperType getMetadataType (void);

            /**
             * Returns the number of bytes that will be write when
             * write() is called. Returns -1 if the MetaData instance is empty.
             */
            int64 getWriteLength (void) const;

            bool isFieldAtIndexUnknown (unsigned int uiIndex) const;

            /**
             * Set one or more fields values
             * Returns 0 in case of success, a negative number otherwise
             *
             * NOTE: it makes a copy of pszValue
             */
            int setFieldValue (const char *pszAttribute, const char *pszValue);
            int setFieldsValues (SQLAVList *pFieldsValues);

            int resetFieldValue (const char *pszFieldName);

            /**
             * Reads metadata fields with the given Reader.
             * Returns a number > 0 if there are no errors; the returned value
             * is the number of bytes that were read.
             * Returns a negative number otherwise.
             *
             * NOTE: read note in the write() comment.
             */
            int64 read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize);

            /**
             * Writes metadata fields with the given Writer.
             * Returns a number > 0 if there are no errors. The
             * number specify the number of bytes written.
             *
             * Returns a negative number otherwise.
             *
             * NOTE: only the values are serialized. It is assumed that the order
             *       of the attribute is the same on every machine
             */
            int64 write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize);

        protected:
            /**
             * Stores the field value in "pszValue". If the given field name
             * exists and the value is known the function returns 0.
             * If the given field name exists and the value is unknown the
             * function returns 1.
             * If the given field name doesn't exist the function returns a
             * negative number.
             */
            int findFieldValue (const char *pszFieldName, const char **ppszValue) const;

        private:
            int setFieldValueInternal (char *&ppszMetaDataFieldOldValue,
                                       const char *pszMetaDataFieldNewValue);

        private:
            const MetadataFieldInfo ** const _pMetadataFieldInfos;
            const uint16 _ui16MetadataFieldsNumber;
            const uint16 _ui16MetadataNotWritten; // the first "metadataNotWritten" fields
                                             // are not written/read by write()/read()
            char **_ppszMetaDataFieldsValues;
    };

    class MetadataUtils
    {
        public:
            /**
             * Returns true if the REFERS_TO property is set and the object
             * pointed by this was published by pszSource.
             *
             * Return false if the REFERS_TO property is null or pszSource is
             * null, or pszSource is not the publisher.
             */
            static bool refersToDataFromSource (MetaData *pMetadata, const char *pszSource);
    };

    inline MetadataInterface::MetadataWrapperType MetaData::getMetadataType()
    {
        return MetadataInterface::Meta;
    }
}

#endif   // INCL_METADATA_H
