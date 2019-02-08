/*
 * SQLTransmissionHistory.h
 *
 * This file is part of the IHMC DisService Library/Component
 * Copyright (c) 2006-2016 IHMC.
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
 * Implements TransmissionHistoryInterface.h by
 * storing the transmitted messages, and their
 * targets in a SQL database.
 *
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on June 16, 2010, 6:59 PM
 */

#ifndef INCL_TRANSMISSION_HISTORY_H
#define	INCL_TRANSMISSION_HISTORY_H

#include "TransmissionHistoryInterface.h"

#include "Mutex.h"
#include "StrClass.h"
#include "PtrLList.h"

namespace NOMADSUtil
{
    class StringTokenizer;
}

namespace IHMC_MISC
{
    class PreparedStatement;
}

namespace IHMC_ACI
{
    class DisServiceMsg;

    class SQLTransmissionHistory : public TransmissionHistoryInterface
    {
        public:
            virtual ~SQLTransmissionHistory (void);

            int init (void);

            void messageSent (DisServiceMsg *pDisServiceMsg);
            int addMessageTarget (const char *pszKey, const char *pszTarget);

            /**
             * Returns all the node ID of all the nodes that have been sent
             * the message identified by pszKey.
             */
            const char ** getTargetList (const char *pszKey);
            const char ** getMessageList (const char *pszTarget);
            const char ** getMessageList (const char *pszTarget, const char *pszForwarder);
            const char ** getAllMessageIds (void);
            void releaseList (const char **ppszTargetList);

            bool hasMessage (const char *pszKey);

            bool hasTarget (const char *pszKey, const char *pszTarget);
            bool hasTargets (const char *pszKey, const char **ppszTargets);

            int reset (void);
            int deleteMessage (const char *pszKey);

        public:
            static const NOMADSUtil::String TABLE_MESSAGE_IDS;
            static const NOMADSUtil::String TABLE_TARGETS;
            static const NOMADSUtil::String TABLE_REL;
            static const NOMADSUtil::String MSG_ID;
            static const NOMADSUtil::String TARGET_ID;
            static const NOMADSUtil::String MSG_ROW_ID;
            static const NOMADSUtil::String TARGET_ROW_ID;

        protected:
            friend class TransmissionHistoryInterface;
            SQLTransmissionHistory (const char *pszStorageFile=NULL);

        private:
            const char ** getList (IHMC_MISC::PreparedStatement *pStmt, const char *pszID);

            struct Record {
                int rowidForeignKey;
            };

            int addMessageRecord (const char *pszKey);
            int addTargetRecord (const char *pszTarget);
            int addMessageTargetRelRecord (int messageRowid, int targetRowid);
            Record * getMessageRow (const char *pszKey);
            Record * getTargetRow (const char *pszTarget);

            void construct (void);

        private:
            NOMADSUtil::Mutex _m;

            IHMC_MISC::PreparedStatement *_psqlInsertMessage;
            IHMC_MISC::PreparedStatement *_psqlInsertTargets;
            IHMC_MISC::PreparedStatement *_psqlInsertMessageTargetsRel;
            IHMC_MISC::PreparedStatement *_psqlSelectMessageRow;
            IHMC_MISC::PreparedStatement *_psqlSelectTargetRow;
            IHMC_MISC::PreparedStatement *_psqlSelectMessageTargets;
            IHMC_MISC::PreparedStatement *_psqlSelectListMessageTargets;
            IHMC_MISC::PreparedStatement *_psqlSelectListMessageByTarget;
            IHMC_MISC::PreparedStatement *_psqlSelectCountMessage;
            IHMC_MISC::PreparedStatement *_psqlSelectCountTargets;
            IHMC_MISC::PreparedStatement *_psqlSelectAllMessages;

            IHMC_MISC::PreparedStatement *_psqlResetRelations;
            IHMC_MISC::PreparedStatement *_psqlResetTargets;
            IHMC_MISC::PreparedStatement *_psqlResetMessages;

            IHMC_MISC::PreparedStatement *_psqlDeleteExpRelations;
            IHMC_MISC::PreparedStatement *_psqlDeleteExpMessages;

            const char *_pszStorageFile;
    };
}

#endif	// INCL_TRANSMISSION_HISTORY_H
