/*
 * CircularQueue.h
 *
 * This file is part of the IHMC Utility Library
 * Copyright (c) IHMC. All Rights Reserved.
 *
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

#ifndef INCL_CIRCULAR_QUEUE_H
#define INCL_CIRCULAR_QUEUE_H

#include <stdlib.h>

namespace NOMADSUtil
{

    template<class T> class CircularQueue
    {
        public:
            CircularQueue (int iSize);
            ~CircularQueue (void);

            class Iterator
            {
                public:
                    ~Iterator (void) {
                        delete _pQueueClone;
                    }
                    bool hasMoreElements (void) {
                        return !_pQueueClone->isEmpty();
                    }
                    T * nextElement (void) {
                        return _pQueueClone->dequeue();
                    }
                private:
                    friend class CircularQueue<T>;
                    Iterator (CircularQueue<T> *pQueueClone) {
                        _pQueueClone = pQueueClone;
                    }
                private:
                    CircularQueue<T> *_pQueueClone;
            };

            bool isEmpty (void);
            bool isFull (void);

            int setSize (int iNewSize);

            int enqueue (T *pElement);
            T * dequeue (void);
            T * peek (void);

            Iterator getAllElements (void);

        private:
            // This constructor makes a shallow copy and is only used for the Iterator
            CircularQueue (const CircularQueue &srcQueue);

        private:
            int _iHead;
            int _iTail;
            T **_apObjects;
            int _iArraySize;
            int _iQueueSize;
            int _iNumElements;
            bool _bShallowClone;
    };

    template<class T> CircularQueue<T>::CircularQueue (int iSize)
    {
        _iHead = 0;
        _iTail = 0;
        _apObjects = new T* [iSize];
        _iArraySize = iSize;
        _iQueueSize = iSize;
        _iNumElements = 0;
        _bShallowClone = false;
    }

    template<class T> CircularQueue<T>::CircularQueue (const CircularQueue<T> &srcQueue)
    {
        _iHead = srcQueue._iHead;
        _iTail = srcQueue._iTail;
        _apObjects = srcQueue._apObjects;
        _iArraySize = srcQueue._iArraySize;
        _iQueueSize = srcQueue._iQueueSize;
        _iNumElements = srcQueue._iNumElements;
        _bShallowClone = true;
    }

    template<class T> CircularQueue<T>::~CircularQueue (void)
    {
        if (!_bShallowClone) {
            delete[] _apObjects;
            _apObjects = NULL;
        }
    }

    template<class T> bool CircularQueue<T>::isEmpty (void)
    {
        return (_iNumElements == 0);
    }

    template<class T> bool CircularQueue<T>::isFull (void)
    {
        return (_iNumElements >= _iQueueSize);
    }

    template<class T> int CircularQueue<T>::setSize (int iNewSize)
    {
        // Will implement later
        return -1;
    }

    template<class T> int CircularQueue<T>::enqueue (T *pElement)
    {
        if (pElement == NULL) {
            return -1;
        }
        if (isFull()) {
            return -2;
        }
        _apObjects[_iTail] = pElement;
        _iTail = (_iTail+1) % _iArraySize;
        _iNumElements++;
        return 0;
    }

    template<class T> T * CircularQueue<T>::dequeue (void)
    {
        if (isEmpty()) {
            return NULL;
        }
        int iCurrent = _iHead;
        _iHead = (_iHead+1) % _iArraySize;
        _iNumElements--;
        return _apObjects[iCurrent];
    }

    template<class T> T * CircularQueue<T>::peek (void)
    {
        if (isEmpty()) {
            return NULL;
        }
        return _apObjects[_iHead];
    }

    template<class T> typename CircularQueue<T>::Iterator CircularQueue<T>::getAllElements (void)
    {
        return Iterator (new CircularQueue<T> (*this));
    }

}

#endif   // #ifndef INCL_CIRCULAR_QUEUE_H
