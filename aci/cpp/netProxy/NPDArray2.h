#ifndef INCL_NPDARRAY2_H
#define INCL_NPDARRAY2_H

/*
 * NPDarray2.h
 * 
 * This file is part of the IHMC NetProxy Library/Component
 * Copyright (c) 2010-2014 IHMC.
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
 * C++ header file that defines the Dynamic Array Class
 * to be used in the NetProxy component.
 *
 * The NPDArray2 class defines a new data type, the Dynamic Array. This array,
 * as opposed to regular arrays, grows automatically. This combines
 * the advantages of a linked list (unlimited size) with the advantages
 * of an array (indexed, direct access to elements).
 *
 * This class differs from DArray by maintaining a dynamic array of pointers to
 * the objects to be stored. While this adds an extra overhead of 4 bytes
 * per element, it does support storing objects with constructors.
 *
 * See the DArray.h file in the util library for more comments.
 *
 * Written by Niranjan Suri
 *
 * RCSInfo: "$Header: /export/cvs/nomads.root/aci/cpp/netProxy/NPDArray2.h,v 1.3 2014/10/29 21:06:03 amorelli Exp $"
 * Revision: "$Revision: 1.3 $"
 */

#include <stdlib.h>
#ifdef __OS2__
  #include <memory.h>
#endif

#ifdef USE_ERROR_HANDLER_FOR_DARRAY2
    #include <stdio.h>
    #include "berrhand.h"
#endif

#include "DArray.h"


namespace ACMNetProxy
{
    template <class T> class NPDArray2
    {
        public:
            NPDArray2 (void);
            NPDArray2 (unsigned int uiSize);
            NPDArray2 (const NPDArray2<T> &SourceArray);
            T & operator [] (unsigned int uiIndex);      // Indexing operator
            void trimSize (unsigned int uiSize);
            T & get (unsigned int uiIndex) const { return *Array.get (uiIndex); }
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
            ~NPDArray2 (void);
        protected:
            NOMADSUtil::DArray <T*> Array;
    };

    template <class T> inline NPDArray2<T>::NPDArray2 (void)
        : Array ((T*)NULL)
    {
    }

    template <class T> inline NPDArray2<T>::NPDArray2 (unsigned int uiSize)
        : Array ((T*)NULL,uiSize)
    {
    }

    template <class T> NPDArray2<T>::NPDArray2 (const NPDArray2<T> &SourceArray)   // Copy constructor
        : Array ((T*)NULL,0)
    {
        #ifdef USE_ERROR_HANDLER_FOR_DARRAY2
             pErr->enter ("NPDArray2::NPDArray2 (NPDArray2 &sourceArray)");
        #endif
        Array [SourceArray.Array.getSize()] = NULL;       // Grow array to size req
        for (unsigned int i = 0; i < SourceArray.getSize(); i++)   // Copy each element
        {
             if (SourceArray.Array.get (i) == NULL)
             {
                  Array [i] = NULL;
             }
             else
             {
                  Array [i] = new T (*(SourceArray.Array.get (i)));
             }
        }
        Array.setHighestIndex (SourceArray.getHighestIndex ());
        #ifdef USE_ERROR_HANDLER_FOR_DARRAY2
             pErr->leaveOk();
        #endif
    }

    template <class T> inline T & NPDArray2<T>::operator [] (unsigned int uiIndex)   // Array indexing operator
    {
        #ifdef USE_ERROR_HANDLER_FOR_DARRAY2
             if (uiIndex == 65535U)
             {
                  pErr->enter ("NPDArray2<T>::operator[](unsigned int uiIndex)");
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

    template <class T> inline void NPDArray2<T>::trimSize (unsigned int uiSize)   // Shrink the size of the array
    {
        for (unsigned int ui = uiSize; ui < Array.getSize(); ++ui)
        {
            if (Array.get (ui))
            {
                delete (Array.get (ui));
            }
        }
        Array.trimSize (uiSize);
    }

    template <class T> inline int NPDArray2<T>::used (unsigned int uiIndex)
    {
        return (Array[uiIndex] != NULL);
    }

    template <class T> inline int NPDArray2<T>::clear (unsigned int uiIndex)
    {
        if (Array[uiIndex])
        {
             delete Array[uiIndex];
             Array[uiIndex] = NULL;
             return 0;
        }
        return -1;
    }

    template <class T> inline int NPDArray2<T>::firstFree (void)
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

    template <class T> inline void NPDArray2<T>::add (const T &newValue)
    {
        (*this)[getHighestIndex()+1] = newValue;
    }

    template <class T> T * NPDArray2<T>::toBufferArray (void)
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

    template <class T> NPDArray2<T>::~NPDArray2 (void)                     // Destructor
    {
        trimSize (0);
    }
}

#endif       // #ifndef INCL_DARRAY2_H
