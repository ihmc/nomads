/*
 * DisServiceScheduler.h
 *
 * This file is part of the IHMC DisService Library/Component
 * Copyright (c) 2006-2014 IHMC.
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

#ifndef INCL_DISSERVICE_SCHEDULER_H
#define INCL_DISSERVICE_SCHEDULER_H

#if defined (USE_SCHEDULER)

#include "DisServiceMsg.h"

#include "NLFLib.h"
#include "PtrLList.h"

namespace NOMADSUtil
{
    class Mutex;
}

namespace IHMC_ACI
{
    class DisseminationService;
    class DisServiceMsg;

    class DisServiceScheduler
    {
        public:
            static const int _DATA_MESSAGES_TO_SEND;
            static const uint8 _DEFAULT_PRIORITY;

            DisServiceScheduler(DisseminationService * pDisService);
            virtual ~DisServiceScheduler();

            int enqueueMessage(DisServiceMsg * pMessage, const char * pszPurpose);
            int sendMessages(void);

        private:
            struct MsgWrap
            {
                public:
                    MsgWrap(DisServiceMsg * pMessage, uint8 priority, const char * pszPurpose, int64 timeStamp);
                    virtual ~MsgWrap();

                    void setMessage(DisServiceMsg * pMessage);
                    void setPriority(uint8 priority);
                    void setTimeStamp(int64 timeStamp);
                    void setPurpose(const char * pszPurpose);

                    DisServiceMsg * getMessage(void);
                    uint8 getPriority(void);
                    int64 getTimeStamp(void);
                    const char * getPurpose(void);

                    bool operator > (MsgWrap &msgToMatch);
                    bool operator < (MsgWrap &msgToMatch);
                    bool operator == (MsgWrap &msgToMatch);

                private:
                    int64 _timeStamp;
                    uint8 _priority;
                    char * _pszPurpose;
                    DisServiceMsg * _pMessage;
            };

            DisseminationService * _pDisService;
            NOMADSUtil::PtrLList<MsgWrap> * _pOutgoingDataQueue;
            NOMADSUtil::PtrLList<MsgWrap> * _pOutgoingCtrlQueue;
            int16 _dataQueueCount;
            int16 _ctrlQueueCount;
            NOMADSUtil::Mutex _m;
    };

    inline DisServiceScheduler::MsgWrap::MsgWrap(DisServiceMsg * pMessage, uint8 priority, const char * pszPurpose, int64 timeStamp)
    {
        _timeStamp = timeStamp;
        _priority = priority;
        _pMessage = pMessage;
        if(pszPurpose == NULL) {
            _pszPurpose = NULL;
            return;
        }
        _pszPurpose = NOMADSUtil::strDup (pszPurpose);
    }

    inline DisServiceScheduler::MsgWrap::~MsgWrap()
    {
        // do not delete the message
        _pMessage = NULL;
        free(_pszPurpose);
    }

    inline void DisServiceScheduler::MsgWrap::setMessage(DisServiceMsg * pMessage)
    {
        _pMessage = pMessage;
    }

    inline void DisServiceScheduler::MsgWrap::setPriority(uint8 priority)
    {
        _priority = priority;
    }

    inline void DisServiceScheduler::MsgWrap::setTimeStamp(int64 timeStamp)
    {
        _timeStamp = timeStamp;
    }

    inline void DisServiceScheduler::MsgWrap::setPurpose(const char * pszPurpose)
    {
        if(pszPurpose == NULL) {
            _pszPurpose = NULL;
            return;
        }
        if(_pszPurpose != NULL) {
            free(_pszPurpose);
        }
        _pszPurpose = NOMADSUtil::strDup (pszPurpose);
    }

    inline DisServiceMsg * DisServiceScheduler::MsgWrap::getMessage(void)
    {
        return _pMessage;
    }

    inline uint8 DisServiceScheduler::MsgWrap::getPriority(void)
    {
        return _priority;
    }

    inline int64 DisServiceScheduler::MsgWrap::getTimeStamp(void)
    {
        return _timeStamp;
    }

    inline const char * DisServiceScheduler::MsgWrap::getPurpose(void)
    {
        return _pszPurpose;
    }

    inline bool DisServiceScheduler::MsgWrap::operator > (MsgWrap &msgToMatch)
    {
        /*if(_priority > msgToMatch._priority) {
            return true;
        }
        if(_priority < msgToMatch._priority) {
            return false;
        }
        if(_timeStamp >= msgToMatch._timeStamp) {
            return true;
        }*/
        return !(operator <(msgToMatch));
    }

    inline bool DisServiceScheduler::MsgWrap::operator < (MsgWrap &msgToMatch)
    {
        if (_priority != msgToMatch._priority) {
            return (_priority < msgToMatch._priority);
        }
        return (_timeStamp < msgToMatch._timeStamp);
        /*if(_priority > msgToMatch._priority) return false;
        if(_priority < msgToMatch._priority) return true;
        if(_timeStamp >= msgToMatch._timeStamp) return false;*/
    }

    inline bool DisServiceScheduler::MsgWrap::operator == (MsgWrap &msgToMatch)
    {
        if((_priority == msgToMatch._priority) && (_timeStamp == msgToMatch._timeStamp)) {
            return true;
        }
        return false;
    }
}

#endif

#endif // INCL_DISSERVICE_SCHEDULER_H
