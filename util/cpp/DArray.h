/*
 * darray.h
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
 * C++ header file that defines the Dynamic Array Class.
 *
 * The DArray class defines a new data type, the Dynamic Array. This array,
 * as opposed to regular arrays, grows automatically. This combines
 * the advantages of a linked list (unlimited size) with the advantages
 * of an array (indexed, direct access to elements).
 *
 * Note that this class should not be used to store objects that have
 * constructors. Use the DArray2 class for such objects.
 *
 * The array is grown or shrunk as needed using the C library realloc ()
 * function. This function allows a region of allocated memory to grow,
 * yet remain contiguous (essential for an array). If there is no room for
 * the memory block to grow, it is automatically moved, thereby preserving
 * existing data.
 *
 * It is possible to fine tune the array's performance by setting a minimum
 * growth increment. The default value is 2. Each time the array has to grow,
 * it is grown by at least the minimum growth increment.
 *
 * Another way to use the class more efficiently is to let it expand to the
 * required size at first. For example, if it is known that 100 elements are
 * to be inserted. First access the 100th element, thereby growing the array
 * to size 100 in one step. This can be done by something like array [99] = x.
 * Alternatively, if the final size is not known but it is known that many
 * elements will be inserted, increase the growth increment.
 *
 * Written by Niranjan Suri
 *
 */

#ifndef INCL_DARRAY_H
#define INCL_DARRAY_H

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef __OS2__
  #include <memory.h>
#endif

#define DEF_SIZE 5;
#define DEF_INCR 23;

#pragma warning (disable:4786)

namespace NOMADSUtil
{

    template <class T>
    class DArray
    {
        public:
            DArray (void);
            DArray (unsigned int uiSize);
            DArray (T initialValue);
            DArray (T initialValue, unsigned int uiSize);
            DArray (const DArray<T> &SourceArray);       // Copy Constructor         
            void setInitialValue (T initialValue);
            void initialize (void);
            T & operator [] (unsigned int uiIndex);      // Indexing operator
            void trimSize (unsigned int uiSize);         // Shrink size function
            T & get (unsigned int uiIndex) const;
            unsigned int getSize (void) const { return uiCurrSize; }
            unsigned int getDefIncr (void) { return uiCurrIncr; }
            void setDefIncr (unsigned int uiDefIncr) { uiCurrIncr = uiDefIncr; }
            long getHighestIndex (void) const { return lHighestIndex; }
            void setHighestIndex (long lHighestIdx) {this->lHighestIndex = lHighestIdx;}
            unsigned int size (void) const { return ((unsigned int)lHighestIndex+1); }
            T* getData(void);
            T* relinquishData (void);
            ~DArray (void);                              // Destructor
        protected:
            T *pArray;                           // Storage for elements
            T *pInitialValue;
            unsigned int uiCurrSize;
            unsigned int uiCurrIncr;
            long lHighestIndex;
    };

    // typedef template <class T> DArray<T> *PDArray<T>;   // Define pointer type

    template <class T> DArray<T>::DArray (void)            // Constructor
    {
        uiCurrSize = DEF_SIZE;             // Set size to default size
        uiCurrIncr = DEF_INCR;             // Set increment to default increment
        pArray = NULL;
        pInitialValue = NULL;
        if (uiCurrSize)                    // if size > 0, create array
        {
             pArray = (T *) malloc (sizeof (T) * uiCurrSize);
        }
        lHighestIndex = -1L;
    }

    template <class T> DArray<T>::DArray (unsigned int uiSize)  // Constructor for array of
    {                                                           // specific initial size
        uiCurrSize = uiSize;
        uiCurrIncr = DEF_INCR;
        pArray = NULL;
        pInitialValue = NULL;
        if (uiCurrSize)                              // if size > 0, create array
        {
             pArray = (T *) malloc (sizeof (T) * uiCurrSize);
        }
        lHighestIndex = -1L;
    }

    template <class T> DArray<T>::DArray (T initialValue)
    {
        uiCurrSize = DEF_SIZE;
        uiCurrIncr = DEF_INCR;
        pArray = NULL;
        pInitialValue = new T;
        *pInitialValue = initialValue;
        if (uiCurrSize)
        {
             pArray = (T *) malloc (sizeof (T) * uiCurrSize);
        }
        for (unsigned int ui = 0; ui < uiCurrSize; ui++)         // initialize elements
        {
             pArray [ui] = initialValue;
        }
        lHighestIndex = -1L;
    }

    template <class T> DArray<T>::DArray (T initialValue, unsigned int uiSize)
    {
        uiCurrSize = uiSize;
        uiCurrIncr = DEF_INCR;
        pArray = NULL;
        pInitialValue = new T;
        *pInitialValue = initialValue;
        if (uiCurrSize)
        {
             pArray = (T *) malloc (sizeof (T) * uiCurrSize);
        }
        for (unsigned int i = 0; i < uiCurrSize; i++)         // initialize elements
        {
             pArray [i] = initialValue;
        }
        lHighestIndex = -1L;
    }

    template <class T> DArray<T>::DArray (const DArray<T> &SourceArray)   // Copy constructor
    {
        uiCurrSize = SourceArray.uiCurrSize;         // Set size to source array
        uiCurrIncr = SourceArray.uiCurrIncr;         // Set increment value
        pArray = NULL;
        if (SourceArray.pInitialValue)
        {
             pInitialValue = new T;
             *pInitialValue = *(SourceArray.pInitialValue);
        }
        else
        {
             pInitialValue = NULL;
        }
        if (uiCurrSize)                              // if size > 0, create array
        {
             pArray = (T *) malloc (sizeof (T) * uiCurrSize);
        }
        for (int i = 0; i < uiCurrSize; i++)         // Create a copy of Source.
        {                                            // Can be made more efficient
             pArray [i] = SourceArray . pArray [i];      // by using memcpy()
        }
        lHighestIndex = SourceArray.lHighestIndex;
    }

    template <class T> void DArray<T>::setInitialValue (T initialValue)
    {
        if (!pInitialValue)
        {
             pInitialValue = new T;
        }
        *pInitialValue = initialValue;
    }

    template <class T> void DArray<T>::initialize (void)
    {
        if (pInitialValue)
        {
             for (int i = 0; i < uiCurrSize; i++)
             {
                  pArray [i] = *pInitialValue;
             }
        }
    }

    template <class T> T* DArray<T>::getData(void)
    {
         return pArray;
    }

    template <class T> T * DArray<T>::relinquishData (void)
    {
        T *pTmp = pArray;

        uiCurrSize = 0;
        pArray = NULL;
        pInitialValue = NULL;
        lHighestIndex = -1L;

        return pTmp;
    }

    template <class T> T & DArray<T>::operator [] (unsigned int uiIndex)   // Array indexing operator
    {
        if (uiIndex < uiCurrSize)                    // if array contains index,
        {                                            // return reference to element
             if ((lHighestIndex < 0) || (((long)uiIndex) > lHighestIndex))
             {
                  lHighestIndex = uiIndex;
             }
             return pArray [uiIndex];
        }
        else                                         // else, array needs to grow
        {
             int uiNewSize;

             if ((uiIndex+1-uiCurrSize) < uiCurrIncr)     // if not growing by min
             {                                            // increment, make it so
                  uiNewSize = uiCurrSize + uiCurrIncr;
             }
             else
             {
                  uiNewSize = uiIndex + 1;                // else, grow as needed
             }
                                                          // Grow memory allocated
             if (pArray)
             {
                  pArray = (T *) realloc (pArray, sizeof (T) * (uiNewSize));
                 if (!pArray) {
                     printf ("DArray Error: Out of memory! Couldn't realloc %d bytes; uiIndex %x uiCurrSize %d uiCurrIncr %d \n",
                                                uiNewSize, uiIndex, uiCurrSize, uiCurrIncr);
                     assert (0);
                 }
             }
             else
             {
                  pArray = (T *) malloc (sizeof (T) * (uiNewSize));
                 if (!pArray) {
                     printf ("DArray Error: Out of memory! Couldn't malloc %d bytes\n", uiNewSize);
                     assert (0);
                 }
             }
             if (pInitialValue)
             {
                  for (int i = uiCurrSize; i < uiNewSize; i++) // Initialize new
                  {                                            // elements
                       pArray [i] = *pInitialValue;
                  }
             }
             uiCurrSize = uiNewSize;                 // Remember new array size
             if ((lHighestIndex < 0) || (((long)uiIndex) > lHighestIndex))
             {
                  lHighestIndex = uiIndex;
             }
             return pArray [uiIndex];                // Return reference to element
        }
    }

    template <class T> void DArray<T>::trimSize (unsigned int uiSize)   // Shrink the size of the array
    {
        if ((uiSize == 0) && (pArray))               // if size = 0, delete all
        {                                            // elements
             free (pArray);
             pArray = NULL;
             uiCurrSize = 0;
        }
        else if (uiSize < uiCurrSize)                // if new size < curr size,
        {                                            // shrink array as requested
             pArray = (T *) realloc (pArray, sizeof (T) * (uiSize));
             uiCurrSize = uiSize;
        }
        lHighestIndex = ((long)uiSize) - 1L;         // lHighestIndex may not refer to
                                                     // an actual element any more.
    }

    template <class T> T & DArray<T>::get (unsigned int uiIndex) const
    {
        if ((lHighestIndex >= 0) && (uiIndex <= ((unsigned int) lHighestIndex))) {
            return pArray[uiIndex];
        }
        return (*pInitialValue);
    }

    template <class T> DArray<T>::~DArray (void)                     // Destructor
    {
        if (pArray)
        {
             free (pArray);                       // Destroy array.
             pArray = NULL;
        }
        if (pInitialValue)
        {
             delete pInitialValue;
             pInitialValue = NULL;
        }
    }

}

#endif        // #ifndef INCL_DARRAY_H
