/**
 * TrasmissionHistoryInterface.h
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
 * Classes implementing TrasmissionHistoryInterface
 * store information on the messages that have been
 * sent to a certain target.
 *
 * TransmissionHistoryInterface is a singleton, and
 * its public methods must be thread-safe.
 *
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on June 21, 2010, 1:33 PM
 */

#ifndef INCL_TRASMISSION_HISTORY_INTERFACE_H
#define	INCL_TRASMISSION_HISTORY_INTERFACE_H

#include <stddef.h>

namespace IHMC_ACI
{
    class DisServiceMsg;

    class TransmissionHistoryInterface
    {
        public:
            virtual ~TransmissionHistoryInterface (void);

            /**
             * Returns a statically allocated implementation of TransmissionHistoryInterface.
             */
            static TransmissionHistoryInterface * getTransmissionHistory (const char *pszStorageFile=NULL);

            virtual void messageSent (DisServiceMsg *pDisServiceMsg)=0;
            virtual int addMessageTarget (const char *pszKey, const char *pszTarget)=0;

            /**
             * Returns a NULL-terminated array that contains the node IDs of the
             * nodes that have been target of a replication of the message
             * identified by pszKey.
             * NOTE: The caller MUST deallocate the returned array. (The release
             * target list function can be used to this end).
             */
            virtual const char ** getTargetList (const char *pszKey)=0;
            virtual const char ** getMessageList (const char *pszTarget)=0;
            virtual const char ** getMessageList (const char *pszTarget, const char *pszForwarder) = 0;
            virtual const char ** getAllMessageIds (void) = 0;
            virtual void releaseList (const char **ppszTargetList)=0;

            /**
             * Returns true if there is an entry for the message identified by
             * the passed key, false otherwise (or in case of error).
             */
            virtual bool hasMessage (const char *pszKey)=0;

            /**
             * Returns true if pszTarget is in pszKey's target list, false otherwise.
             * NOTE: the method returns false also if an error occurred.
             */
            virtual bool hasTarget (const char *pszKey, const char *pszTarget)=0;

            /**
             * Returns true if all the ppszTargets are in pszKey's target list,
             * false otherwise.
             * NOTE: the method returns false also if an error occurred.
             */
            virtual bool hasTargets (const char *pszKey, const char **ppszTargets)=0;

            /**
             * Removes all elements (targets and sent messages)
             */
            virtual int reset (void)=0;

            virtual int deleteMessage (const char *pszKey)=0;

        protected:
            TransmissionHistoryInterface (void);

        private:
            static TransmissionHistoryInterface *_pInstance;
    };
}

#endif  // INCL_TRASMISSIONHISTORYINTERFACE_H
