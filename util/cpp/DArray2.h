/*
 * darray2.h
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
 * The DArray2 class defines a new data type, the Dynamic Array. This array,
 * as opposed to regular arrays, grows automatically. This combines
 * the advantages of a linked list (unlimited size) with the advantages
 * of an array (indexed, direct access to elements).
 *
 * This class differs from DArray by maintaining a dynamic array of pointers to
 * the objects to be stored. While this adds an extra overhead of 4 bytes
 * per element, it does support storing objects with constructors.
 *
 * See the darray.h file for more comments.
 *
 * Written by Niranjan Suri
 *
 */

#ifndef INCL_DARRAY2_H
#define INCL_DARRAY2_H

#include <stdlib.h>
#ifdef __OS2__
  #include <memory.h>
#endif

#ifdef USE_ERROR_HANDLER_FOR_DARRAY2
    #include <stdio.h>
    #include "berrhand.h"
#endif

#include "DArray.h"

namespace NOMADSUtil
{

    template <class T>
    class DArray2
    {
        public:
             DArray2 (void);
             DArray2 (unsigned int uiSize);
             DArray2 (DArray2<T> &SourceArray);
             T & operator [] (unsigned int uiIndex);      // Indexing operator
             void trimSize (unsigned int uiSize);
             unsigned int getSize (void) const { return Array.getSize(); }
             unsigned int getDefIncr (void) { return Array.getDefIncr(); }
             void setDefIncr (unsigned int uiDefIncr) { Array.setDefIncr (uiDefIncr); }
             long getHighestIndex (void) const { return Array.getHighestIndex(); }
             void setHighestIndex (long lHighestIndex) { Array.setHighestIndex (lHighestIndex); }
             unsigned int size (void) const { return Array.size(); }
             int used (unsigned int uiIndex);
             int clear (unsigned int uiIndex);
             int firstFree (void);
             void add (const T &newValue);
             T * toBufferArray (void);
             ~DArray2 (void);
        protected:
             DArray <T*> Array;
    };

    template <class T> inline DArray2<T>::DArray2 (void)
        : Array ((T*)NULL)
    {
    }

    template <class T> inline DArray2<T>::DArray2 (unsigned int uiSize)
        : Array ((T*)NULL,uiSize)
    {
    }

    template <class T> DArray2<T>::DArray2 (DArray2<T> &SourceArray)   // Copy constructor
        : Array ((T*)NULL,0)
    {
        #ifdef USE_ERROR_HANDLER_FOR_DARRAY2
             pErr->enter ("DArray2::DArray2 (DArray2 &sourceArray)");
        #endif
        Array [SourceArray.Array.getSize()] = NULL;       // Grow array to size req
        for (int i = 0; i < SourceArray.getSize(); i++)   // Copy each element
        {
             if (SourceArray.Array [i] == NULL)
             {
                  Array [i] = NULL;
             }
             else
             {
                  Array [i] = new T (*(SourceArray.Array[i]));
             }
        }
        #ifdef USE_ERROR_HANDLER_FOR_DARRAY2
             pErr->leaveOk();
        #endif
    }

    template <class T> inline T & DArray2<T>::operator [] (unsigned int uiIndex)   // Array indexing operator
    {
        #ifdef USE_ERROR_HANDLER_FOR_DARRAY2
             if (uiIndex == 65535U)
             {
                  pErr->enter ("DArray2<T>::operator[](unsigned int uiIndex)");
                  pErr->leaveErr (-1, "invalid index: -1");
                  FILE *f;
                  f = fopen ("error.log", "w");
                  fprintf (f, "%s\n", pErr->getErrorLog());
                  fclose (f);
                  static T junk;
                  return junk;
             }
        #endif
        T **pItem = &Array [uiIndex];
        if (*pItem == NULL)
        {
             *pItem = new T;
        }
        return **pItem;
    }

    template <class T> inline void DArray2<T>::trimSize (unsigned int uiSize)   // Shrink the size of the array
    {
        if (Array.getSize () > uiSize)
        {
             for (unsigned int ui = uiSize; ui < Array.getSize (); ui++)
             {
                  if (Array [ui])
                  {
                       delete (Array [ui]);
                  }
             }
             Array.trimSize (uiSize);
        }
    }

    template <class T> inline int DArray2<T>::used (unsigned int uiIndex)
    {
        return (Array[uiIndex] != NULL);
    }

    template <class T> inline int DArray2<T>::clear (unsigned int uiIndex)
    {
        if (Array[uiIndex])
        {
             delete Array[uiIndex];
             Array[uiIndex] = NULL;
             return 0;
        }
        return -1;
    }

    template <class T> inline int DArray2<T>::firstFree (void)
    {
        int j = (int) Array.size();
        for (int i = 0; i < j; i++)
        {
             if (Array[i] == NULL)
             {
                  return i;
             }
        }
        return j;
    }

    template <class T> inline void DArray2<T>::add (const T &newValue)
    {
        (*this)[getHighestIndex()+1] = newValue;
    }

    template <class T> T * DArray2<T>::toBufferArray (void)
    {
        unsigned int uiSize = size();
        if (uiSize < 1) {
            return NULL;
        }
        T *pArray = (T *) calloc (uiSize+1, sizeof (T));
        if (pArray == NULL) {
            return NULL;
        }
        unsigned int j = 0;
        for (unsigned int i = 0; i < uiSize; i++) {
            if (used (i)) {
                pArray[j] = *(Array[i]);
                j++;
            }
        }
        return pArray;
    }

    template <class T> DArray2<T>::~DArray2 (void)                     // Destructor
    {
        trimSize (0);
    }
}

#endif       // #ifndef INCL_DARRAY2_H
