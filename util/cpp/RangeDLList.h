/*
 * RangeDLList.h
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
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on March 21, 2011, 4:54 PM
 */

#ifndef INCL_RANGE_DLLIST_H
#define INCL_RANGE_DLLIST_H

#include "FTypes.h"
#include "SequentialArithmetic.h"

#include <stdlib.h>
#include <stdio.h>

namespace NOMADSUtil
{
    class BufferWriter;
    class Reader;

    template <class T>
    class RangeDLList
    {
        public:
            RangeDLList (bool bUseSequentialArithmetic, T lowerLimit, T upperLimit);
            virtual ~RangeDLList (void);

            // Returns true if there are any ranges, false if the list is empty
            bool haveInformation (void);
            bool hasTSN (T el) const;

            int getFirst (T &beginEl, T &endEl, bool bResetGet=false);
            int getNext (T &beginEl, T &endEl);
            void resetGet (void);

            // Returns 0 in case the range contains duplicate TSNs or 1 if the
            // TSN was successfully added with no duplicate TSNs
            int addTSN (T elBegin, T elEnd);

            // Returns 0 in case of a duplicate TSN or 1 if the TSN was
            // successfully added
            int addTSN (T el);

            void removeTSN (T el);

            // Remove all the ranges within [elBegin, elEnd] (including elBegin
            // and elEnd)
            void removeTSN (T elBegin, T elEnd);
            void removeAllTSN (RangeDLList<T> &ranges);

            void reset (void);

            bool validate (void);

        protected:
            struct Range
            {
                Range (void);
                ~Range (void);

                Range *pPrev;
                Range *pNext;
                T begin;
                T end;
            };

            bool lessThanAndDisjoint (const T l, const T r) const;
            bool greaterThanAndDisjoint (const T l, const T r) const;

            bool lessThan (const T l, const T r) const;
            bool greaterThan (const T l, const T r) const;
            // Returns true if the first range includes the second
            bool includes (const T l1, const T r1, const T l2, const T r2) const;
            bool lessThanOrEqual (const T l, const T r) const;
            bool greaterThanOrEqual (const T l, const T r) const;

        protected:
            const bool _bUseSequentialArithmetic;
            const T _lowerLimit;
            const T _upperLimit;
            Range *_pFirstNode;
            Range *_pLastNode;
            Range *_pGetCurrentNode;
    };

    //--------------------------------------------------------------------------
    //    UInt8RangeDLList
    //--------------------------------------------------------------------------

    class UInt8RangeDLList : public RangeDLList<uint8>
    {
        public:
            UInt8RangeDLList (bool bUseSequentialArithmetic);
            virtual ~UInt8RangeDLList (void);

            void display (FILE *pFileOut);

            int read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize);
            int write (NOMADSUtil::BufferWriter *pWriter, uint32 ui32MaxSize);
    };

    //--------------------------------------------------------------------------
    //    UInt16RangeDLList
    //--------------------------------------------------------------------------

    class UInt16RangeDLList : public RangeDLList<uint16>
    {
        public:
            UInt16RangeDLList (bool bUseSequentialArithmetic);
            virtual ~UInt16RangeDLList (void);

            int read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize);
            int write (NOMADSUtil::BufferWriter *pWriter, uint32 ui32MaxSize);
    };

    //--------------------------------------------------------------------------
    //    UInt32RangeDLList
    //--------------------------------------------------------------------------

    class UInt32RangeDLList : public RangeDLList<uint32>
    {
        public:
            UInt32RangeDLList (bool bUseSequentialArithmetic);
            virtual ~UInt32RangeDLList (void);

            void display (FILE *pFileOut);

            int read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize);
            int write (NOMADSUtil::BufferWriter *pWriter, uint32 ui32MaxSize);
    };

    //--------------------------------------------------------------------------
    //    inline definitions
    //--------------------------------------------------------------------------

    template <class T> RangeDLList<T>::RangeDLList (bool bUseSequentialArithmetic, T lowerLimit, T upperLimit)
        : _bUseSequentialArithmetic (bUseSequentialArithmetic),
          _lowerLimit (lowerLimit),
          _upperLimit (upperLimit)
    {
        _pFirstNode = _pLastNode = NULL;
        _pGetCurrentNode = _pFirstNode;
    }

    template <class T> RangeDLList<T>::~RangeDLList (void)
    {
        reset();
    }

    template <class T> int RangeDLList<T>::addTSN (T elBegin, T elEnd)
    {
        if (lessThan (elEnd, elBegin)) {
            return -1;
        }
        if (elBegin == elEnd) {
            return addTSN (elBegin);
        }

        if (_pFirstNode == NULL) {
            _pFirstNode = _pLastNode = new Range();
            _pFirstNode->begin = elBegin;
            _pFirstNode->end = elEnd;
            _pFirstNode->pPrev = NULL;
            _pFirstNode->pNext = NULL;

            return 1;
        }
        else if (lessThanAndDisjoint (elEnd, _pFirstNode->begin)) {
            Range *pNewNode = new Range();
            pNewNode->begin = elBegin;
            pNewNode->end = elEnd;
            pNewNode->pPrev = NULL;
            pNewNode->pNext = _pFirstNode;

            _pFirstNode->pPrev = pNewNode;
            _pFirstNode = pNewNode;

            return 1;
        }

        // Optimization: it often happens that TSN are sent sequentially, and
        // therefore added in sequence (even though there may be missing TSNs.
        // Checking whether the TSN should just be appended saves from iterating
        // through the whole list, which may be long for large files if the drop
        // probability is high).
        if (greaterThanOrEqual (elBegin, _pLastNode->begin) && lessThanOrEqual (elBegin, _pLastNode->end)) {
            if (greaterThan (elEnd, _pLastNode->end)) {
                _pLastNode->end = elEnd;

                return 1;
            }

            return 0;
        }
        else if (greaterThanAndDisjoint (elBegin, _pLastNode->end)) {
            Range *pNewNode = new Range();
            pNewNode->begin = elBegin;
            pNewNode->end = elEnd;
            pNewNode->pPrev = _pLastNode;
            pNewNode->pNext = NULL;

            _pLastNode->pNext = pNewNode;
            _pLastNode = pNewNode;

            return 1;
        }

        bool bRangeAdded = false;
        bool bRangeAddedOrMerged = false;
        Range *pCurrNode = _pFirstNode;
        Range *pPrevNode = NULL;
        for (; pCurrNode != NULL; pPrevNode = pCurrNode, pCurrNode = pCurrNode->pNext) {
            if (greaterThanAndDisjoint (elBegin, (pCurrNode->end))) {
                // no overlap - keep searching for overlap
                continue;
            }
            if (lessThanAndDisjoint (elEnd, pCurrNode->begin)) {
                // no overlap - can't be found
                break;
            }

            // The current range and the new range intersect
            if (bRangeAddedOrMerged) {
                // This case handles and input range spanning over multiple
                // ranges in the list.
                if (greaterThan (pCurrNode->end, elEnd)) {
                    // Merge the two nodes, by incrementing the previous node's
                    // end if necessary
                    pPrevNode->end = pCurrNode->end;
                }
                // Remove the current node because either merged with the previous
                // one, or because it is included in the previous one
                pPrevNode->pNext = pCurrNode->pNext;
                if (pCurrNode->pNext != NULL) {
                    pCurrNode->pNext->pPrev = pPrevNode;
                }
                delete pCurrNode;

                pCurrNode = pPrevNode;
                if (pCurrNode->pNext == NULL) {
                    _pLastNode = pCurrNode;
                }
            }
            else {
                if (lessThan (elBegin, pCurrNode->begin)) {
                    pCurrNode->begin = elBegin;
                    bRangeAdded = true;
                }
                if (greaterThan (elEnd, pCurrNode->end)) {
                    pCurrNode->end = elEnd;
                    bRangeAdded = true;
                }
                bRangeAddedOrMerged = true;
            }
        }

        if (!bRangeAddedOrMerged) {
            // The range did not overlap with any of the ranges in the list,
            // add it right after the last range that was visited
            Range *pNewNode = new Range();
            pNewNode->begin = elBegin;
            pNewNode->end = elEnd;
            pNewNode->pPrev = pPrevNode;
            pNewNode->pNext = pCurrNode;

            if (pCurrNode != NULL) {
                pCurrNode->pPrev = pNewNode;
            }
            if (pPrevNode != NULL) {
                pPrevNode->pNext = pNewNode;
            }
            if (pNewNode->pNext == NULL) {
                _pLastNode = pNewNode;
            }

            return 1;
        }

        return bRangeAdded ? 1 : 0;
    }

    template <class T> int RangeDLList<T>::addTSN (T el)
    {
        bool bAddedTSN = false;
        if (_pFirstNode == NULL) {
            // There are no other nodes
            _pFirstNode = _pLastNode = new Range();
            _pFirstNode->begin = _pFirstNode->end = el;
            bAddedTSN = true;
        }
        else if ((el + 1) == _pFirstNode->begin) {
            // This sequence number immediately precedes that of the first range node
            _pFirstNode->begin--;
            bAddedTSN = true;
        }
        else if (el == (_pLastNode->end + 1)) {
            // This sequence number immediately follows that of the last range node
            _pLastNode->end++;
            bAddedTSN = true;
        }
        else if (lessThan (el, _pFirstNode->begin)) {
            // This sequence number belongs to a range that precedes the first range node
            Range *pNewNode = new Range();
            pNewNode->begin = pNewNode->end = el;
            pNewNode->pNext = _pFirstNode;
            _pFirstNode->pPrev = pNewNode;
            _pFirstNode = pNewNode;
            bAddedTSN = true;
        }
        else if (greaterThan (el, _pLastNode->end)) {
            // This sequence number belongs to a range that follows the last range node
            Range *pNewNode = new Range();
            pNewNode->begin = pNewNode->end = el;
            pNewNode->pPrev = _pLastNode;
            _pLastNode->pNext = pNewNode;
            _pLastNode = pNewNode;
            bAddedTSN = true;
        }
        else {
            // Find the right spot for this sequence number
            Range *pCurrNode = _pFirstNode;
            Range *pNextNode = _pFirstNode->pNext;
            while (pCurrNode != NULL) {
                bool bCheckForMerge = false;
                if (el == (pCurrNode->end + 1)) {
                    // The sequence number immediately follows that of the current range node
                    pCurrNode->end++;
                    bCheckForMerge = true;
                    bAddedTSN = true;
                }
                else if (pNextNode != NULL) {
                    if ((el + 1) == pNextNode->begin) {
                        // The sequence number immediately precedes the next range node
                        pNextNode->begin--;
                        bCheckForMerge = true;
                        bAddedTSN = true;
                    }
                    else if (greaterThan (el, pCurrNode->end) && lessThan (el, pNextNode->begin)) {
                        // The sequence number belongs to a new range that is in between the current and next range nodes
                        Range *pNewNode = new Range();
                        pNewNode->begin = pNewNode->end = el;
                        pNewNode->pPrev = pCurrNode;
                        pNewNode->pNext = pNextNode;
                        pCurrNode->pNext = pNewNode;
                        pNextNode->pPrev = pNewNode;
                        bAddedTSN = true;
                        break;
                    }
                }
                // Check to see if we need to do a merge
                if (bCheckForMerge && (pNextNode != NULL)) {
                    if ((pCurrNode->end + 1) == pNextNode->begin) {
                        // Need to merge current and next range nodes
                        pCurrNode->end = pNextNode->end;
                        pCurrNode->pNext = pNextNode->pNext;
                        if (_pLastNode == pNextNode) {
                            // pNextNode was the last node - update the pLastNode pointer
                            _pLastNode = pCurrNode;
                        }
                        else {
                            // pNextNode is going away, so make the back pointer of the node
                            // following pNextNode point to pCurrNode
                            // NOTE: Since pNextNode is not the last note (as determined by the
                            // previous if test), there must be a node following pNextNode
                            pNextNode->pNext->pPrev = pCurrNode;
                        }
                        delete pNextNode;
                        break;
                    }
                }
                pCurrNode = pNextNode;
                if (pNextNode) {
                    pNextNode = pNextNode->pNext;
                }
            }
        }

        if (bAddedTSN) {
            return 1;
        }
        else {
            return 0;
        }
    }

    template <class T> void RangeDLList<T>::removeTSN (T el)
    {
        if (_pFirstNode == NULL) {
            // The list is empty. Nothing to do.
            return;
        }
        Range *pRange = _pFirstNode;
        while ((pRange != NULL) && // NOTE: _pLastNode == NULL iff _pFirstNode, therefore I do not need to check for it
               lessThanOrEqual (el, _pLastNode->end)) {
            if (el == pRange->begin) {
                if (el == pRange->end) {
                    // Remove the whole range
                    if (pRange->pPrev != NULL) {
                        pRange->pPrev->pNext = pRange->pNext;
                    }
                    else {
                        // It's the root - I have to update _pFirstNode as well
                        _pFirstNode = pRange->pNext;
                    }
                    if (pRange->pNext != NULL) {
                        pRange->pNext->pPrev = pRange->pPrev;
                    }
                    else {
                        // It's the tail - I have to update _pLastNode as well
                        _pLastNode = pRange->pPrev;
                    }
                    delete pRange;
                    pRange = NULL;
                    break;
                }
                else {
                    // Shorten the range
                    T tmp = pRange->begin; 
                    pRange->begin = (++tmp);
                }
            }
            else if (el == pRange->end) {
                // Shorten the range
                T tmp = pRange->end;
                pRange->end = (--tmp);
            }
            else if (greaterThan (el, pRange->begin) && lessThan (el, pRange->end)) {
                // Split the range
                Range *pNewRange = new Range();
                pNewRange->begin = pRange->begin;
                pNewRange->end = el - 1;
                pNewRange->pNext = pRange;
                pNewRange->pPrev = pRange->pPrev;
                if (pRange->pPrev == NULL) {
                    // pRange is the root - I have to update _pFirstNode as well
                    _pFirstNode = pNewRange;
                }
                else {
                    pRange->pPrev->pNext = pNewRange;
                }

                pRange->pPrev = pNewRange;
                pRange->begin = el + 1;
            }

            if (pRange == NULL) {
                break;
            }
            pRange = pRange->pNext;
        }

        return;
    }

    template <class T> void RangeDLList<T>::removeTSN (T elBegin, T elEnd)
    {
        if (greaterThan (elBegin, elEnd)) {
            return;
        }
        if (_pFirstNode == NULL) {
            // The list is empty. Nothing to do.
            return;
        }
        Range *pRange = _pFirstNode;
        while ((pRange != NULL) && // NOTE: _pLastNode == NULL iff _pFirstNode, therefore I do not need to check for it
               lessThanOrEqual (elBegin, pRange->end)) {
            if (greaterThan (elBegin, pRange->end) || lessThan (elEnd, pRange->begin)) {
                // No overlap - nothing to do
                pRange = pRange->pNext;
                continue;
            }

            if (lessThanOrEqual (elBegin, pRange->begin) && greaterThanOrEqual (elEnd, pRange->end)) {
                // The current range is included in the one to be removed - remove it
                if (pRange->pPrev != NULL) {
                    pRange->pPrev->pNext = pRange->pNext;
                }
                else {
                    // It's the root - I have to update _pFirstNode as well
                    _pFirstNode = pRange->pNext; 
                }
                if (pRange->pNext != NULL) {
                    pRange->pNext->pPrev = pRange->pPrev;
                }
                else {
                    // It's the tail - I have to update _pLastNode as well
                    _pLastNode = pRange->pPrev;
                }
                Range *pTmp = pRange->pNext;
                delete pRange;
                pRange = pTmp;
                continue;
            }

            if (lessThan (pRange->begin, elBegin) && greaterThan (pRange->end, elEnd)) {
                // the range to remove is included in the current range - split
                // the current range in two and shorten the new range                
                Range *pNewRange = new Range();
                pNewRange->pNext = pRange->pNext;
                pNewRange->pPrev = pRange;
                pNewRange->begin = elEnd + 1;
                pNewRange->end = pRange->end;
                if (pRange->pNext == NULL) {
                    // It's the tail - I have to update _pLastNode as well
                    _pLastNode = pNewRange;
                }
                pRange->pNext = pNewRange;
                pRange->end = elBegin - 1;
            }
            else if (greaterThan (elBegin, pRange->begin)) {
                // Shorten current range from the right
                pRange->end = elBegin - 1; 
            }
            else if (lessThan (elEnd, pRange->end)) {
                // Shorten current rage from the left
                pRange->begin = elEnd + 1;
            }
            else if (elBegin == pRange->end) {
                // Shorten from the left
                pRange->begin++;
            }
            else if (elEnd == pRange->begin) {
                // Shorten from the right
                pRange->end--;
            }

            pRange = pRange->pNext;
        }
    }

    template <class T> void RangeDLList<T>::removeAllTSN (RangeDLList &ranges)
    {
        T elBegin, elEnd;
        for (int rc = ranges.getFirst (elBegin, elEnd); rc == 0; rc = ranges.getNext (elBegin, elEnd)) {
            removeTSN (elBegin, elEnd);
        }
    }

    template <class T> void RangeDLList<T>::reset()
    {
        while (_pFirstNode != NULL) {
            Range *pTempNode = _pFirstNode;
            _pFirstNode = _pFirstNode->pNext;
            delete pTempNode;
        }
        _pFirstNode = _pLastNode = NULL;
    }

    template <class T> bool RangeDLList<T>::haveInformation()
    {
        return (_pFirstNode != NULL);
    }

    template <class T> bool RangeDLList<T>::hasTSN (T el) const
    {
        if ((_pFirstNode == NULL) || lessThan (el, _pFirstNode->begin) || greaterThan (el, _pLastNode->end))  {
            return false;
        }    
        Range *pCurrNode = _pFirstNode;
        while (pCurrNode != NULL) {
            if (lessThanOrEqual (pCurrNode->begin, el) &&
                lessThanOrEqual (el, pCurrNode->end)) {
                return true;
            }
            pCurrNode = pCurrNode->pNext;
        }
        return false;
    }

    template <class T> int RangeDLList<T>::getFirst (T &beginEl, T &endEl, bool bResetGet)
    {
        if (bResetGet) {
            resetGet();
        }
        if (_pFirstNode != NULL) {
            beginEl = _pFirstNode->begin;
            endEl = _pFirstNode->end;
            return 0;
        }
        return -1;
    }

    template <class T> int RangeDLList<T>::getNext (T &beginEl, T &endEl)
    {
        if (_pGetCurrentNode != NULL) {
            _pGetCurrentNode = _pGetCurrentNode->pNext;
            if (_pGetCurrentNode != NULL) {
                beginEl = _pGetCurrentNode->begin;
                endEl = _pGetCurrentNode->end;
                return 0;
            }
        }
        return -1;
    }

    template <class T> void RangeDLList<T>::resetGet (void)
    {
        _pGetCurrentNode = _pFirstNode;
    }

    template <class T> bool RangeDLList<T>::validate (void)
    {
        // Validate going forward
        if ((_pFirstNode == NULL) && (_pLastNode == NULL)) {
            return true;
        }
        else if (_pFirstNode == _pLastNode) {
            if ((_pFirstNode->pPrev != NULL) || (_pFirstNode->pNext != NULL) || (_pLastNode->pPrev != NULL) || (_pLastNode->pNext != NULL)) {
                return false;
            }
            if (_pFirstNode->begin > _pFirstNode->end) {
                return false;
            }
            if ((_pFirstNode->begin > 3000) || (_pFirstNode->end > 3000)) {
                return false;
            }
            return true;
        }
        else {
            int count = 0;
            Range *pCurrNode = _pFirstNode;
            while (pCurrNode != NULL) {
                if (pCurrNode->pNext != NULL) {
                    if (pCurrNode->pNext->pPrev != pCurrNode) {
                        return false;
                    }
                }
                else if (pCurrNode != _pLastNode) {
                    return false;
                }
                if (pCurrNode->begin > pCurrNode->end) {
                    return false;
                }
                if ((pCurrNode->begin > 3000) || (pCurrNode->end > 3000)) {
                    return false;
                }
                pCurrNode = pCurrNode->pNext;
                count++;
                if (count > 100) {
                    return false;
                }
            }
            pCurrNode = _pLastNode;
            while (pCurrNode != NULL) {
                if (pCurrNode->pPrev != NULL) {
                    if (pCurrNode->pPrev->pNext != pCurrNode) {
                        return false;
                    }
                }
                else if (pCurrNode != _pFirstNode) {
                    return false;
                }
                if (pCurrNode->begin > pCurrNode->end) {
                    return false;
                }
                if ((pCurrNode->begin > 3000) || (pCurrNode->end > 3000)) {
                    return false;
                }
                pCurrNode = pCurrNode->pPrev;
                count++;
                if (count > 100) {
                    return false;
                }
            }
        }
        return true;
    }

    template <class T> bool RangeDLList<T>::greaterThan (const T l, const T r) const
    {
        return _bUseSequentialArithmetic ? SequentialArithmetic::greaterThan (l, r) : (l > r);
    }

    template <class T> bool  RangeDLList<T>::greaterThanAndDisjoint (const T l, const T r) const
    {
        if (_bUseSequentialArithmetic) {
            return greaterThan (l, r + 1);
        }
        else if (l > _lowerLimit) {
            return greaterThan (l - 1, r);
        }
        // (l == _lowerLimit), therefore either (r == _lowerLimit) || (l < r).
        return false;
    }

    template <class T> bool RangeDLList<T>::greaterThanOrEqual (const T l, const T r) const
    {
        return _bUseSequentialArithmetic ? SequentialArithmetic::greaterThanOrEqual (l, r) : (l >= r);
    }

    template <class T> bool RangeDLList<T>::includes (const T l1, const T r1, const T l2, const T r2) const
    {
        return (greaterThanOrEqual (l2, l1) && lessThanOrEqual (r2, r1));
    }

    template <class T> bool RangeDLList<T>::lessThan (const T l, const T r) const
    {
        return _bUseSequentialArithmetic ? SequentialArithmetic::lessThan (l, r) : (l < r);
    }

    template <class T> bool RangeDLList<T>::lessThanAndDisjoint (const T l, const T r) const
    {        
        if (_bUseSequentialArithmetic) {
            return lessThan (l + 1, r);
        }
        else if (l < _upperLimit) {
            return lessThan (l + 1, r);
        }
        // l == upperBound, therefore either (r == upperBound) || (l > r)
        return false;
    }       

    template <class T> bool RangeDLList<T>::lessThanOrEqual (const T l, const T r) const
    {
        return _bUseSequentialArithmetic ? SequentialArithmetic::lessThanOrEqual (l, r) : (l <= r);
    }

    //--------------------------------------------------------------------------
    // TSNRangeHandler::Node
    //--------------------------------------------------------------------------
    template <class T> RangeDLList<T>::Range::Range (void)
    {
        pPrev = pNext = NULL;
        begin = end = 0;
    }

    template <class T> RangeDLList<T>::Range::~Range (void)
    {
        pPrev = pNext = NULL;
        begin = end = 0;
    }
}

#endif  // INCL_RANGE_DLLIST_H
