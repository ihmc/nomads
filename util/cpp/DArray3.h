/*
 * DArray3.h
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2014 IHMC.
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
 * DArray3 is a pseudo-dynamic array. It provides a single-dimensional array but
 * internally stores the array elements in a sparse two-dimensional array.
 *
 * Two parameters are specified to the array at initialization time. One specifies
 * the vertical size of the array and the other specifies the horizontal size of
 * the array. The array is represented as a collection of "strips" as depicted
 * below:
 *
 *                      Strip
 *     +----+
 *     |    |   +---+---+---+---+---+
 *     |  --+-->|   +   +   +   +   + ...
 *     |    |   +---+---+---+---+---+
 *     +----+
 *     |    |   +---+---+---+---+---+
 *     |  --+-->|   +   +   +   +   + ...
 *     |    |   +---+---+---+---+---+
 *     +----+
 *     |    |   +---+---+---+---+---+
 *     |  --+-->|   +   +   +   +   + ...
 *     |    |   +---+---+---+---+---+
 *     +----+
 *     |    |
 *     |    |
 *     |    |
 *     +----+
 *
 * The length of the strip is specified by the uchStripSizeBits parameter, which
 * specify the number of bits that are used to index an element of the strip.
 * Therefore, the length of the strip is 2 ** uchStripSizeBits. For example,
 * specifying a value of 5 for uchStripSizeBits will make each strip 32 elements long.
 *
 * The maximum number of strips allowed is specified by the uchNumStripsBits parameter.
 * This also specifies the number of bits that are used to select a particular strip.
 * Therefore, the number of strips allowed is 2 ** uchNumStripsBits.
 *
 * The maximum size of the array is determined by the sum of uchStripSizeBits and
 * uchNumStripsBits. It is equal to 2 ** (uchNumStripsBits + uchStripSizeBits).
 * Needless to say, (uchNumStripsBits + uchStripSizeBits) should be <= 32.
 *
 * The array is dynamic in the sense that strips are allocated only on an as needed
 * basis. The growth of the array will be in multiples of the strip size.
 *
 * The vertical array is an array of pointers to strips. Note that this array is
 * created at initialization time and imposes a fixed cost on the array.
 *
 * For a given maximum size of an array, a number of combinations of the strip size
 * and the number of strips are possible. Increasing the strip size reduces the fixed
 * overhead cost of the vertical array but increases the increment size and vice-versa.
 */

#ifndef INCL_DARRAY_3_H
#define INCL_DARRAY_3_H

#include <assert.h>
#include <stddef.h>

#pragma warning (disable:4786)

namespace NOMADSUtil
{

    template<class T>
    class DArray3
    {
        public:
            DArray3 (unsigned char uchNumStripsBits, unsigned char uchStripSizeBits);
            ~DArray3 (void);

            // Returns an element from the array at the specified index
            // Does NOT check whether the element (or the strip containing the element exists)!
            // Caller must make sure that the element at index was previously set (or accessed
            //     with the [] operator)
            // Also does NOT check to see if the array index is out of bounds
            T & get (unsigned long ulIndex);

            // Sets an element at the specified index
            // Does NOT check to see if the array index is out of bounds
            void set (unsigned long ulIndex, const T &value);

            // Accesses the element at the specified index
            // Does NOT check to see if the array index is out of bounds
            T & operator [] (unsigned long ulIndex);

            // Returns the size of the array
            // Does NOT imply that all elements upto size()-1 exist in the array (because
            //     in-between strips may never have been allocated)
            // Also does NOT imply that there are not more than size() elements allocated
            //     in the array (since the array is grown in multiples of strip size)
            unsigned long size (void);

        private:
            T **_ppElements;
            unsigned char _uchStripSizeBits;
            unsigned char _uchNumStripsBits;
            unsigned long _ulStripIndexMask;
            unsigned long _ulSize;
    };

    template<class T> DArray3<T>::DArray3 (unsigned char uchNumStripsBits, unsigned char uchStripSizeBits)
    {
        if ((uchStripSizeBits > 32) || (uchNumStripsBits > 32)) {
            //Need to throw an exception here
        }
        else if ((uchStripSizeBits+uchNumStripsBits) > 32) {
            //Need to throw an exception here
        }
        _uchStripSizeBits = uchStripSizeBits;
        _uchNumStripsBits = uchNumStripsBits;
        _ulStripIndexMask = (0x00000001UL << _uchStripSizeBits) - 1;

        _ulSize = 0;

        unsigned long ulNumStrips = (0x00000001UL << _uchNumStripsBits);
        _ppElements = new T* [ulNumStrips];
        for (unsigned long ul = 0; ul < ulNumStrips; ul++) {
            _ppElements[ul] = NULL;
        }
    }

    template<class T> DArray3<T>::~DArray3 (void)
    {
        unsigned long ulNumStrips = (0x00000001UL << _uchNumStripsBits);
        for (unsigned long ul = 0; ul < ulNumStrips; ul++) {
            delete[] _ppElements[ul];
        }
        delete[] _ppElements;
    }

    template<class T> inline T & DArray3<T>::get (unsigned long ulIndex)
    {
        #if defined (ERROR_CHECKING)
            if (ulIndex >= _ulSize) {
                assert (0);
            }
        #endif

        return _ppElements[ulIndex>>_uchStripSizeBits][ulIndex&_ulStripIndexMask];
    }

    template<class T> inline void DArray3<T>::set (unsigned long ulIndex, const T &value)
    {
        T * pStrip;
        if (NULL == (pStrip = _ppElements[ulIndex>>_uchStripSizeBits])) {
            pStrip = _ppElements[ulIndex>>_uchStripSizeBits] = new T [0x00000001UL << _uchStripSizeBits];
        }
        pStrip[ulIndex&_ulStripIndexMask] = value;
        if (ulIndex >= _ulSize) {
            _ulSize = ulIndex+1;
        }
    }

    template<class T> inline T & DArray3<T>::operator [] (unsigned long ulIndex)
    {
        T * pStrip;
        if (NULL == (pStrip = _ppElements[ulIndex>>_uchStripSizeBits])) {
            pStrip = _ppElements[ulIndex>>_uchStripSizeBits] = new T [0x00000001UL << _uchStripSizeBits];
        }
        if (ulIndex >= _ulSize) {
            _ulSize = ulIndex+1;
        }
        return pStrip[ulIndex&_ulStripIndexMask];
    }

    template<class T> inline unsigned long DArray3<T>::size (void)
    {
        return _ulSize;
    }

}

#endif   // #ifndef INCL_DARRAY_3_H
