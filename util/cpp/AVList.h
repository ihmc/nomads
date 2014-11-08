/*
 * AVList.h
 *
 * Implements an dynamically-growing array of attribute-value pairs.
 *
 * The array is grown as needed using the C library realloc () function.
 * This function allows a region of allocated memory to grow, yet remain
 * contiguous (essential for an array). If there is no room for
 * the memory block to grow, it is automatically moved, thereby preserving
 * existing data.
 *
 * To use the class more efficiently is to let it expand to the required size at
 * first. For example, if it is known that 100 elements are to be inserted it is
 * possible to set the initial size to 100.
 *
 * This file is part of the IHMC Utility Library
 * Copyright (c) IHMC. All Rights Reserved.
 *
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 *
 * authors : Silvia Rota                 srota@ihmc.us
 *           Giacomo Benincasa           gbenincasa@ihmc.us
 */

#ifndef INCL_AVLIST_H
#define INCL_AVLIST_H

#include "FTypes.h"

namespace NOMADSUtil
{
    class AVList
    {
        public:
            AVList (void);

            /**
             * Allocate an initial size for the array.
             *
             * @param initialSize
             */
            AVList (unsigned int uiInitialSize);
            virtual ~AVList (void);

            /**
             * Add methods, the return value is
             * @param pszAttrName
             * @param pszValue
             * @return 1 if there is an error, 0 otherwise
             */
            int addPair (const char *pszAttrName, const char *pszValue);
            int addPair (const char *pszAttrName, int iValue);
            int addPair (const char *pszAttrName, int64 i64Value);
            int addPair (const char *pszAttrName, float fValue);
            int addPair (const char *pszAttrName, double dValue);

            /**
             *
             * @param index
             * @return -1 if "index" is out of bounds, 0 otherwise
             */
            int deletePair (unsigned int uiIndex);

            /**
             * @return the current length of the array.
             */
            unsigned int getLength (void) const;

            /**
             *
             * @param index
             * @return NULL if "index" is out of bounds.
             */
            const char * getAttribute (unsigned int uiIndex) const;
            const char * getValueByIndex (unsigned int uiIndex) const;
            const char * getValue (const char *pszAttrName) const;

            bool isEmpty (void) const;

            /**
             * Add the "second" list at the end of the first.
             *
             * @param first
             * @param second
             */
            void concatLists (AVList *pFirst, AVList *pSecond);

            /**
             *
             * @param first
             * @param second
             * @return a new AVList created by the concatenation of the two
             * passed AVLists
             */
            AVList * concatListsInNewOne (AVList *pFirst, AVList *pSecond);

            /**
             * @return a copy of this object.
             */
            AVList * copyList (void);

            /**
             * Empty the list.
             */
            void emptyList (void);

        protected:
            struct Pair {
                char *_pszAttribute;
                char *_pszValue;
            };
            Pair *_pPair;
            unsigned int _uiMaxSize;
            unsigned int _uiCurrSize;
            char _pszNumber[50]; // used to convert float and double to char *
    };

    inline unsigned int AVList::getLength (void) const
    {
        return _uiCurrSize;
    }
    
    inline bool AVList::isEmpty (void) const
    {
        return (_uiCurrSize <= 0);
    }
}

#endif /* INCL_AVLIST_H */
