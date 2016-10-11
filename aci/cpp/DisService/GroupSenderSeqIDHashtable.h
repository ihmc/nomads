/*
 * GroupSenderSeqIDHashtable.h
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
 * This class stores objects of a generic class
 * T in different sets, given a group name, a
 * sender node id and a seq id.
 *
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on December 7, 2009, 11:29 AM
 */

#ifndef INCL_GROUP_SENDER_SEQID_HASHTABLE_H
#define	INCL_GROUP_SENDER_SEQID_HASHTABLE_H

#include "Exceptions.h"
#include "StringHashtable.h"
#include "UInt32Hashtable.h"

namespace IHMC_ACI
{
    template <class T>
    class GroupSenderSeqIDHashtable
    {
        public:
            GroupSenderSeqIDHashtable (bool bCaseSensitiveGroupKeys = true,
                                       bool bCloneGroupKeys = true,
                                       bool bDeleteGroupKeys = true,
                                       bool bDeleteGroupValues = false,
                                       bool bCaseSensitiveSenderKeys = true,
                                       bool bCloneSenderKeys = true,
                                       bool bDeletSendereKeys = true,
                                       bool bDeletesenderValues = false,
                                       bool bDelSeqIDValues = false);
            virtual ~GroupSenderSeqIDHashtable();

            virtual void configure (bool bCaseSensitiveGroupKeys = true,
                                    bool bCloneGroupKeys = true,
                                    bool bDeleteGroupKeys = true,
                                    bool bDeleteGroupValues = false,
                                    bool bCaseSensitiveSenderKeys = true,
                                    bool bCloneSenderKeys = true,
                                    bool bDeletSendereKeys = true,
                                    bool bDeletesenderValues = false,
                                    bool bDelSeqIDValues = false);

            virtual T * put (const char *pszGroupName, const char *pszSenderNodeId, uint32 ui32SeqId, T *pValue);
            virtual T * get (const char *pszGroupName, const char *pszSenderNodeId, uint32 ui32SeqId);

            /*virtual NOMADSUtil::UInt32Hashtable<T>::Iterator getAllElements (const char *pszGroupName,
                                                                             const char *pszSenderNodeId,
                                                                             uint32 ui32SeqId)
                                                                            throw (InvalidStateException);*/

            /**
             * Removes the matching entry from the hashtable and returns the
             * value associated with the key
             * Returns NULL if the key does not exist
             * NOTE: The value is NOT DELETED even if bDeleteValues is set to
             * true, so the caller is responsible for deleting the removed value
             */
             virtual T * remove (const char *pszGroupName, const char *pszSenderNodeId, uint32 ui32SeqId);
             void removeAll (void);

        protected:
            struct MessagesBySender
            {
                MessagesBySender (bool bDelSeqIDValues);
                ~MessagesBySender ();

                NOMADSUtil::UInt32Hashtable<T> messageBySeqId;
            };

            struct MessagesByGroup
            {
                MessagesByGroup (bool bCaseSensitiveSenderKeys, bool bCloneSenderKeys,
                                 bool bDeletSendereKeys, bool bDeletesenderValues);
                ~MessagesByGroup ();

                NOMADSUtil::StringHashtable<MessagesBySender> messageBySender;
            };

            bool _bCaseSensitiveSenderKeys;
            bool _bCloneSenderKeys;
            bool _bDeletSendereKeys;
            bool _bDeletesenderValues;
            bool _bDelSeqIDValues;

            NOMADSUtil::StringHashtable<MessagesByGroup> _messageByGroup;
    };

    template <class T>
    GroupSenderSeqIDHashtable<T>::GroupSenderSeqIDHashtable(bool bCaseSensitiveGroupKeys,
                                                            bool bCloneGroupKeys,
                                                            bool bDeleteGroupKeys,
                                                            bool bDeleteGroupValues,
                                                            bool bCaseSensitiveSenderKeys,
                                                            bool bCloneSenderKeys,
                                                            bool bDeletSendereKeys,
                                                            bool bDeletesenderValues,
                                                            bool bDelSeqIDValues)
        : _messageByGroup (bCaseSensitiveGroupKeys, bCloneGroupKeys, bDeleteGroupKeys, bDeleteGroupValues)
    {
        _bCaseSensitiveSenderKeys = bCaseSensitiveSenderKeys;
        _bCloneSenderKeys = bCloneSenderKeys;
        _bDeletSendereKeys = bCloneSenderKeys;
        _bDeletesenderValues = bDeletesenderValues;
        _bDelSeqIDValues = bDelSeqIDValues;
    };

    template <class T>
    GroupSenderSeqIDHashtable<T>::~GroupSenderSeqIDHashtable()
    {
        removeAll();
    }

    template <class T>
    void GroupSenderSeqIDHashtable<T>::configure (bool bCaseSensitiveGroupKeys,
                                                  bool bCloneGroupKeys,
                                                  bool bDeleteGroupKeys,
                                                  bool bDeleteGroupValues,
                                                  bool bCaseSensitiveSenderKeys,
                                                  bool bCloneSenderKeys,
                                                  bool bDeletSendereKeys,
                                                  bool bDeletesenderValues,
                                                  bool bDelSeqIDValues)
    {
        _messageByGroup.configure (bCaseSensitiveGroupKeys, bCloneGroupKeys,
                                   bDeleteGroupKeys, bDeleteGroupValues);
        _bCaseSensitiveSenderKeys = bCaseSensitiveSenderKeys;
        _bCloneSenderKeys = bCloneSenderKeys;
        _bDeletSendereKeys = bCloneSenderKeys;
        _bDeletesenderValues = bDeletesenderValues;
        _bDelSeqIDValues = bDelSeqIDValues;
    }

    template <class T>
    T * GroupSenderSeqIDHashtable<T>::put (const char *pszGroupName, const char *pszSenderNodeId, uint32 ui32SeqId, T *pValue)
    {
        MessagesByGroup *pBG = _messageByGroup.get (pszGroupName);
        if (!pBG) {
            pBG = new MessagesByGroup(_bCaseSensitiveSenderKeys, _bCloneSenderKeys,
                                      _bDeletSendereKeys, _bDeletesenderValues);
            _messageByGroup.put (pszGroupName, pBG);
        }

        MessagesBySender *pMS = pBG->messageBySender.get (pszSenderNodeId);
        if (!pMS) {
            pMS = new MessagesBySender(_bDelSeqIDValues);
            pBG->messageBySender.put (pszSenderNodeId, pMS);
        }

        return pMS->messageBySeqId.put (ui32SeqId, pValue);
    }

    template <class T>
    T * GroupSenderSeqIDHashtable<T>::get (const char *pszGroupName, const char *pszSenderNodeId, uint32 ui32SeqId)
    {
        MessagesByGroup *pBG = _messageByGroup.get (pszGroupName);
        if (pBG) {
            MessagesBySender *pMS = pBG->messageBySender.get (pszSenderNodeId);
            if (pMS) {
                T *pT = pMS->messageBySeqId.get (ui32SeqId);
                if (pT) {
                    return pT;
                }
            }
        }
        return NULL;
    }

    /*template <class T>
    NOMADSUtil::UInt32Hashtable<T>::Iterator GroupSenderSeqIDHashtable<T>::getAllElements (const char *pszGroupName,
                                                                                        const char *pszSenderNodeId,
                                                                                        uint32 ui32SeqId) throw (InvalidStateException)
    {
        MessagesByGroup *pBG = _messageByGroup.get (pszGroupName);
        if (pBG) {
            MessagesBySender *pMS = pBG->messageBySender.get (pszSenderNodeId);
            if (pMS) {
                T *pT = pMS->messageBySeqId.remove (ui32SeqId);
                if (pT) {
                    return pT->getAllElements();
                }
            }
        }
        NOMADSUtil::String errmsg = (NOMADSUtil::)"GroupSenderSeqIDHashtable<T>::getAllElements: No T element to iterate was found for" +
                                    pszGroupName + " " + pszSenderNodeId + " " + ui32SeqId;
        throw InvalidStateException (errmsg);
    }*/

    template <class T>
    T * GroupSenderSeqIDHashtable<T>::remove (const char *pszGroupName, const char *pszSenderNodeId, uint32 ui32SeqId)
    {
        MessagesByGroup *pBG = _messageByGroup.get (pszGroupName);
        if (pBG) {
            MessagesBySender *pMS = pBG->messageBySender.get (pszSenderNodeId);
            if (pMS) {
                T *pT = pMS->messageBySeqId.remove (ui32SeqId);
                if (pT) {
                    return pT;
                }
            }
        }
        return NULL;
    }

    template <class T>
    void GroupSenderSeqIDHashtable<T>::removeAll()
    {
        _messageByGroup.removeAll();
    };

    template <class T>
    inline GroupSenderSeqIDHashtable<T>::MessagesBySender::MessagesBySender(bool bDelSeqIDValues)
        : messageBySeqId (US_INITSIZE, bDelSeqIDValues)
    {
    }

    template <class T>
    inline GroupSenderSeqIDHashtable<T>::MessagesBySender::~MessagesBySender()
    {
    }

    template <class T>
    inline GroupSenderSeqIDHashtable<T>::MessagesByGroup::MessagesByGroup(bool bCaseSensitiveSenderKeys,
                                                                          bool bCloneSenderKeys,
                                                                          bool bDeletSendereKeys,
                                                                          bool bDeletesenderValues)
        : messageBySender (bCaseSensitiveSenderKeys, bCloneSenderKeys,
                           bDeletSendereKeys, bDeletesenderValues)
    {
    }

    template <class T>
    inline GroupSenderSeqIDHashtable<T>::MessagesByGroup::~MessagesByGroup()
    {
    }
}

#endif  // INCL_GROUP_SENDER_SEQID_HASHTABLE_H
