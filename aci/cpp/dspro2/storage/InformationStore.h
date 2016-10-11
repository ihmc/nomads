/*
 * InformationStore.h
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
 */

#ifndef INCL_INFORMATION_STORE_H
#define INCL_INFORMATION_STORE_H

#include "MetadataInterface.h"

#include "LoggingMutex.h"

class TiXmlElement;

struct sqlite3;
struct sqlite3_stmt;

namespace IHMC_C45
{
    class C45AVList;
}

namespace NOMADSUtil
{
    class AVList;
    class String;
}

namespace IHMC_ACI
{
    class ComplexMatchmakingQualifier;
    class DataStore;
    class SQLAVList;
    class MatchmakingQualifier;
    class MatchmakingQualifiers;
    class MetadataInterface;
    class MetadataConfiguration;

    class InformationStore
    {
        public:
            /**
             * Setup a fixed MetaData configuration. The fields are:
             * "Message_ID", "Usage", "Receiver_Time_Stamp", "Source",
             * "Source_Time_Stamp", "Expiration_Time",
             * "Relevant_Mission", "Latitude", "Longitude", "Altitude",
             * "Location", "Data_Type", "Classification", "Relevance",
             * "Pedigree", "Description".
             */
            InformationStore (DataStore *pDataCache,
                              const char *pszDSProGroupName);
            virtual ~InformationStore (void);

            int init (MetadataConfiguration *pMetadataConf,
                      const char *pszMetadataTableName = NULL,
                      const char *pszMetadataDBName = NULL);

            // database methods

            int insertIntoDB (MetadataInterface *pMetadata);

            char ** getDSProIds (const char *pszObjectId, const char *pszInstanceId);
            char ** getDSProIds (const char *pszQuery);

            /**
             * Stores the result in "metadata". If metadata = NULL, a new
             * metadata is created, else the function will insert the values
             * in the already existing metadata. The return value is the error
             * code.
             */
            MetadataInterface * getMetadata (const char *pszKey);
            MetadataList * getMetadataForData (const char *pszReferring);

            /**
             * For each metadata, the fields used in the prediction, and _only_
             * these fields are extracted in stored in pFields.
             * The elements returned is stored in "noLists".
             *
             * - pszCondition: condition on
             *
             * NOTE: The function overwrites the value *pFileds and must be
             *       deallocated by the caller.
             */
            MetadataList * getAllMetadata (const char **ppszMessageIdFilters, bool bExclusiveFilter = true);
            MetadataList * getAllMetadata (NOMADSUtil::AVList *pAVQueryList, int64 i64BeginArrivalTimestamp,
                                           int64 i64EndArrivalTimestamp);
            MetadataList * getAllMetadataInArea (MatchmakingQualifiers *pMatchmakingQualifiers,
                                                 const char **ppszMessageIdFilters,
                                                 float fMaxLat, float fMinLat, float fMaxLong, float fMinLong);

            /**
             * Extract the ids of the messages that match the given query.
             *
             * NOTE: the sqlConstraints represent a incomplete SQL statement
             * that express only the "where" constraints (not including the
             * "WHERE" keyword).
             */
            NOMADSUtil::PtrLList<const char> * getMessageIDs (const char *pszGroupName, const char *pszSqlConstraints,
                                                              char **ppszFilters=NULL);

            /**
             * Usage possible values: -1 = unknown, 0 = not used, 1 = used
             * This method updates the usage value just just if the previous
             * value was "unknown".
             */
            int updateUsage (const char *pszKey, int usage);

            /**
             * Group the metadata Stored in the DB by values of the given field
             * name. Returns in "perc" the % (range [0 - 1]) of groups with one
             * metadata. Used to understand if a field should be used in the
             * learning or not.
             */
            int groupAndCount (const char *pszFieldName, float *perc);

            int deleteMetadataFromDB (const char *pszKey);

            char * toSqlConstraint (MatchmakingQualifier *pMatchmakingQualifier);
            char * toSqlConstraints (ComplexMatchmakingQualifier *pCMatchmakingQualifier);
            char * toSqlStatement (MatchmakingQualifiers *pMatchmakingQualifiers);

        private:
            friend class StorageController;

            MetadataList * getMetadataInternal (sqlite3_stmt *pStmt, bool bAllowMultipleMatches=false);

            /*
             * Extract all the fields of every metadata that match the given query.
             *
             * NOTE:
             * - it is not thread safe! The caller must ensure it!
             * - differently from the public functions, sqlQuery is a
             *   "complete" SQL query statement.
             */
            MetadataList * getAllMetadata (const char *pszSQL, unsigned int uiExpectedNumberOfColumns);

            /*
             * Extract the ID of every metadata that match the given query.
             * 
             * NOTE:
             * - it is not thread safe! The caller must ensure it!
             * - differently from the public functions, sqlQuery is a
             *   "complete" sql query statement.
             */
            NOMADSUtil::PtrLList<const char> * extractMessageIDsFromDBBase (const char *pszGroupName,
                                                                            const char *pszSQL);

            void createSpatialIndexes (void);
            int openDataBase (MetadataConfiguration *pMetadataConf);

        private:
            char *_pszMetadataDBName;
            char *_pszMetadataTableName;
            char *_pszMetadataPrimaryKey;
            char *_pszMetadataAll;                // all the fields in the table
            unsigned int _uiMetadataAll;
            sqlite3 *_pSQL3DB;
            sqlite3_stmt *_psqlInserted;
            sqlite3_stmt *_psqlGetMetadata;
            sqlite3_stmt *_psqlGetReferringMetadata;

            /*
             * 0 = no errors
             * 1 = database opening failed
             * 2 = cannot create metadata table
             * 3 = prepare statement to insert query failed
             * 4 = cannot bind a field
             * 5 = insert failed
             * 6 = extract failed with a DB error
             * 7 = extract failed because the key doesn't exist
             * 8 = one of the given parameter is NULL
             * 9 = update "usage" failed
             * 10 = delete MetaData failed
             * 11 = convert xml to metadata failed
             */
            int _errorCode;

            /*
             * Database
             */
            static const char * DEFAULT_DATABASE_NAME;
            static const char * DEFAULT_METADATA_TABLE_NAME;

            DataStore *_pDataStore;
            const char *_pszDSProGroupName;
            const char *_pszStartsWithDSProGroupNameTemplate;

            MetadataConfiguration *_pMetadataConf;

            /*
             * NOTE: to prevent simultaneous access by different threads.
             * The InformationStore is accessed by controllers,
             * DisServicePro and the application.
             */
            NOMADSUtil::LoggingMutex _m;
    };
}

#endif // INCL_INFORMATION_STORE_H

