/*
* TimeIntervalAverage.h
*
* This file is part of the IHMC Util Library
* Copyright (c) 1993-2016 IHMC.
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
* This class is the implementation of a double linked list that holds data within
* a moving window, the size of the window is not fixed, a time interval is specified
* upon creation and expired data is removed from the list before the result of
* getSum, getMin, getAverage or getNumValues is returned.
*
*/

#ifndef INCL_TIME_INTERVAL_AVERAGE_H
#define INCL_TIME_INTERVAL_AVERAGE_H

#include "NLFLib.h"

template <class T> class TimeIntervalAverage
{
public:
    explicit TimeIntervalAverage (uint32 ui32TimeIntervalInMS);
    ~TimeIntervalAverage (void);
    TimeIntervalAverage (const TimeIntervalAverage &obj);
    TimeIntervalAverage & operator = (const TimeIntervalAverage &obj);

    void add (const T & value);
    void add (int64 i64Timestamp, const T & value);
    void expireOldEntries (void);

    T getSum (void);
    T getMin (void);
    double getAverage (void);
    uint32 getNumValues (void);
    void setNewTimeInterval (uint32 ui32TimeIntervalInMS);

private:
    void copyTiaContent (const TimeIntervalAverage & obj);
    void cleanTIA (void);

    int insertEntry (int64 i64TimeStamp, const T & value);

private:
    struct Node
    {
        Node(void);
        int64 i64TimeStamp;
        T value;
        Node * pNext;
        Node * pPrev;
    };

private:
    uint32 _ui32TimeInterval;
    Node * _pHead;
    Node * _pTail;
    T      _sum;
    uint32 _ui32NumValues;
};


template <class T> TimeIntervalAverage<T>::TimeIntervalAverage (uint32 ui32TimeIntervalInMS)
{
    _ui32TimeInterval = ui32TimeIntervalInMS;
    _pHead = nullptr;
    _pTail = nullptr;
    _sum = T(0);
    _ui32NumValues = 0;
}

template <class T> TimeIntervalAverage<T>::~TimeIntervalAverage (void)
{
    cleanTIA();
}

template <class T> TimeIntervalAverage<T>::TimeIntervalAverage (const TimeIntervalAverage & obj)
{
    cleanTIA();

    _ui32TimeInterval = obj._ui32TimeInterval;
    if (obj._pHead == nullptr) {
        return;
    }

    copyTiaContent (obj);
}


template <class T> TimeIntervalAverage<T> & TimeIntervalAverage<T>::operator = (const TimeIntervalAverage & obj)
{
    if (this == &obj) {
        return *this;
    }
    cleanTIA();

    _ui32TimeInterval = obj._ui32TimeInterval;
    if (obj._pHead == nullptr) {
        return *this;
    }

    copyTiaContent (obj);
    return *this;
}

// Values are inserted in order, newer values are inserted at the end
template <class T> void TimeIntervalAverage<T>::add (const T & value)
{
    insertEntry (NOMADSUtil::getTimeInMilliseconds(), value);
}

// Values are inserted in order, newer values are inserted at the end
template <class T> void TimeIntervalAverage<T>::add (int64 i64Timestamp, const T & value)
{
    // Check if this new entry isn't already too old before inserting it
    if ((NOMADSUtil::getTimeInMilliseconds() - (int64)_ui32TimeInterval) < i64Timestamp) {
        insertEntry (i64Timestamp, value);
    }
}

template <class T> T TimeIntervalAverage<T>::getSum(void)
{
    expireOldEntries();
    return _sum;
}

template <class T> T TimeIntervalAverage<T>::getMin (void)
{
    expireOldEntries();
    T min;

    // Find the minimum rather than store it because we expire entries
    if (_pHead == nullptr)
        min = (T)-1; //TODO: temporary!
    else {
        min = _pHead->value;
        Node *pTempNode = _pHead;
        while (pTempNode != nullptr) {
            if (pTempNode->value < min) {
                min = pTempNode->value;
            }
            pTempNode = pTempNode->pNext;
        }
    }
    return min;
}

template <class T> double TimeIntervalAverage<T>::getAverage (void)
{
    expireOldEntries();
    return (double)_sum / _ui32TimeInterval * 1000;
}

template <class T> uint32 TimeIntervalAverage<T>::getNumValues (void)
{
    expireOldEntries();
    return _ui32NumValues;
}

// Values are inserted in order, newer values are inserted at the end
template <class T> void TimeIntervalAverage<T>::setNewTimeInterval (uint32 ui32TimeIntervalInMS)
{
    _ui32TimeInterval = ui32TimeIntervalInMS;
}

template <class T> int TimeIntervalAverage<T>::insertEntry (int64 i64TimeStamp, const T & value)
{
    Node * pNewNode = new Node();
    pNewNode->i64TimeStamp = i64TimeStamp;
    pNewNode->value = value;

    if (_pTail == nullptr) {
        _pHead = _pTail = pNewNode;
        _sum += value;
        _ui32NumValues++;
        return 0;
    }

    // Insert at the tail of the queue if the node is the most recent
    if (_pTail->i64TimeStamp <= i64TimeStamp) {
        pNewNode->pPrev = _pTail;
        _pTail->pNext = pNewNode;
        _pTail = pNewNode;
        _sum += value;
        _ui32NumValues++;
        return 0;
    }

    // Insert at the correct point in the list
    Node *pTempNode = _pTail->pPrev;
    while (pTempNode != nullptr) {
        if (pTempNode->i64TimeStamp <= i64TimeStamp) {
            pNewNode->pPrev = pTempNode;
            pNewNode->pNext = pTempNode->pNext;
            pTempNode->pNext->pPrev = pNewNode;
            pTempNode->pNext = pNewNode;
            _sum += value;
            _ui32NumValues++;
            return 0;
        }
        pTempNode = pTempNode->pPrev;
    }

    // Reached the head
    pNewNode->pNext = _pHead;
    _pHead->pPrev = pNewNode;
    _pHead = pNewNode;
    _sum += value;
    _ui32NumValues++;
    return 0;
}

template <class T> void TimeIntervalAverage<T>::expireOldEntries (void)
{
    if (_pHead == nullptr) {
        // Empty structure
        return;
    }

    int64 i64ExpireUntil = NOMADSUtil::getTimeInMilliseconds() - _ui32TimeInterval;
    //printf("Expire Until = %I64d\n", i64ExpireUntil);
    Node *pExpiredNode;
    Node *pTempNode = _pHead;
    while (pTempNode != nullptr) {
        if (pTempNode->i64TimeStamp <= i64ExpireUntil) {
            _sum -= pTempNode->value;
            _ui32NumValues--;
            pExpiredNode = pTempNode;
            pTempNode = pTempNode->pNext;
            delete pExpiredNode;
            pExpiredNode = nullptr;
        }
        else {
            // The nodes are inserted in order therefore we are done expiring nodes
            _pHead = pTempNode;
            _pHead->pPrev = nullptr;
            return;
        }
    }

    // All the nodes are expired
    _pHead = _pTail = nullptr;
    return;
}

template <class T> void TimeIntervalAverage<T>::copyTiaContent (const TimeIntervalAverage & obj)
{
    auto pOtherNode = obj._pHead;
    while (pOtherNode != nullptr) {
        insertEntry (pOtherNode->i64TimeStamp, pOtherNode->value);
        auto pOtherNode = obj._pHead->pNext;
    }
}

template <class T> void TimeIntervalAverage<T>::cleanTIA (void)
{
    while (_pHead != nullptr) {
        Node *pTemp = _pHead;
        _pHead = _pHead->pNext;
        delete pTemp;
    }

    _pTail = nullptr;
    _sum = T(0);
    _ui32NumValues = 0;
}

template <class T> TimeIntervalAverage<T>::Node::Node (void)
{
    i64TimeStamp = 0;
    value = T(0);
    pNext = nullptr;
    pPrev = nullptr;
}

#endif   // #ifndef INCL_TIME_INTERVAL_AVERAGE_H
