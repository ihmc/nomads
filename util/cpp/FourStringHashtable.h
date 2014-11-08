/*
 * FourStringHashtable.h
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
 */

#ifndef INCL_FOUR_STRING_HASHTABLE_H
#define INCL_FOUR_STRING_HASHTABLE_H

#include <string.h>
#include <stdio.h>
#include <ctype.h>

#if defined (UNIX)
    #define stricmp strcasecmp
#elif defined (WIN32)
    #define stricmp _stricmp
#endif

namespace NOMADSUtil
{

    template <class T> class FourStringHashtable
    {
        public:
            FourStringHashtable (bool bCaseSensitiveKeys = true,
                                  bool bCloneKeys = false,
                                  bool bDeleteKeys = false);

            FourStringHashtable (unsigned short usInitSize,
                                  bool bCaseSensitiveKeys = true,
                                  bool bCloneKeys = false,
                                  bool bDeleteKeys = false);  

            virtual ~FourStringHashtable (void);

            class Iterator
            {
                public:
                    ~Iterator (void) {
                        _pTable = NULL;
                        _ulState = 0;
                        _usIndex = 0;
                        _pCurrElement = NULL;
                    }

                    bool end (void) {
                        return (_pCurrElement == NULL);
                    }
                    
                    bool nextElement (void) {
                        if (_pCurrElement) {
                            if (((HashtableEntry*)_pCurrElement)->pNext) {
                                _pCurrElement = ((HashtableEntry*)_pCurrElement)->pNext;
                                return true;
                            }
                        }
                        while (++_usIndex < _pTable->_usTableSize) {
                            if (_pTable->_pHashtable[_usIndex].pszKeyOne) {
                                _pCurrElement = &_pTable->_pHashtable[_usIndex];
                                return true;
                            }
                        }
                        _pCurrElement = NULL;
                        return false;
                    }

                    const char * getKeyOne (void) {
                        if (_pCurrElement) {
                            return ((HashtableEntry*)_pCurrElement)->pszKeyOne;
                        }
                        return NULL;
                    }
                    
                    const char * getKeyTwo (void) {
                        if (_pCurrElement) {
                            return ((HashtableEntry*)_pCurrElement)->pszKeyTwo;
                        }
                        return NULL;
                    }
                    
                    const char * getKeyThree (void) {
                        if (_pCurrElement) {
                            return ((HashtableEntry*)_pCurrElement)->pszKeyThree;
                        }
                        return NULL;
                    }
                    
                    const char * getKeyFour (void) {
                        if (_pCurrElement) {
                            return ((HashtableEntry*)_pCurrElement)->pszKeyFour;
                        }
                        return NULL;
                    }


                    T *getValue (void) {
                        if (_pCurrElement) {
                            return ((HashtableEntry*)_pCurrElement)->pValue;
                        }
                        return NULL;
                    }
                    
                private:
                    Iterator (FourStringHashtable<T> *pTable, unsigned long ulState) {
                        _pTable    = pTable;
                        _ulState   = ulState;
                        _usIndex   = 0;
                        _pCurrElement = NULL;
                        while (_usIndex < _pTable->_usTableSize) {
                            if (_pTable->_pHashtable[_usIndex].pszKeyOne) {
                                _pCurrElement = &_pTable->_pHashtable[_usIndex];
                                break;
                            }
                            _usIndex++;
                        }
                    }
                    
                    FourStringHashtable<T> *_pTable;
                    unsigned long           _ulState;
                    unsigned short          _usIndex;
                    void                    *_pCurrElement;
                    static const float      fTHRESHOLD;

                private:
                    friend class FourStringHashtable<T>;
            };
            
            virtual T * put      (const char *pszKeyOne, const char *pszKeyTwo, const char *pszKeyThree, const char *pszKeyFour, T *Value);
            virtual T * get      (const char *pszKeyOne, const char *pszKeyTwo, const char *pszKeyThree, const char *pszKeyFour);
            virtual T * getCaseS (const char *pszKeyOne, const char *pszKeyTwo, const char *pszKeyThree, const char *pszKeyFour);
            virtual T * getCaseI (const char *pszKeyOne, const char *pszKeyTwo, const char *pszKeyThree, const char *pszKeyFour);
            virtual T * remove   (const char *pszKeyOne, const char *pszKeyTwo, const char *pszKeyThree, const char *pszKeyFour);
            void removeAll (void);

            virtual unsigned short getCount (void);
            virtual Iterator getAllElements (void);

            virtual void printStructure (void);

        protected:
            struct HashtableEntry
            {
                HashtableEntry (void) {
                    pszKeyOne     = NULL;
                    pszKeyTwo     = NULL;
                    pszKeyThree   = NULL;
                    pszKeyFour    = NULL;
                    pNext         = NULL;
                    pNextLinear   = NULL;
                }
                char   *pszKeyOne;
                char   *pszKeyTwo;
                char   *pszKeyThree;
                char   *pszKeyFour;
                T      *pValue;
                struct HashtableEntry *pNext;
                struct HashtableEntry *pNextLinear;
            };

        protected:
            virtual int keycomp  (const char *pszKeyOne, const char *pszKeyTwo);
            virtual int hashCode (const char *pszKeyOne, const char *pszKeyTwo, const char *pszKeyThree, const char *pszKeyFour);
            virtual int hashCode2(const char *pszKeyOne, const char *pszKeyTwo, const char *pszKeyThree, const char *pszKeyFour);
            void        rehash   (void);
            void        deleteTable (HashtableEntry *pTable, unsigned short usSize);

        private:
            HashtableEntry *_pHashtable;
            unsigned short _usInitSize;
            unsigned short _usTableSize;
            unsigned short _usCount;
            unsigned long  _ulState;
            bool           _bCaseSensitiveKeys;
            bool           _bCloneKeys;
            bool           _bDeleteKeys;

            HashtableEntry *_pFirstAdded;
            HashtableEntry *_pLastAdded;
            bool           _bUseLinear;

        private:
            friend class Iterator;
    };

    template <class T> inline unsigned short FourStringHashtable<T>::getCount (void)
    {
        return _usCount;
    }

    template <class T> FourStringHashtable<T>::FourStringHashtable (bool bCaseSensitiveKeys,
                                                                      bool bCloneKeys,
                                                                      bool bDeleteKeys)
    {
        _usCount     = 0;
        _usInitSize  = 23;
        _pHashtable  = new HashtableEntry[_usInitSize];
        _usTableSize = _usInitSize;
        _ulState     = 0;
        _bCaseSensitiveKeys = bCaseSensitiveKeys;
        if (bCloneKeys) {
            _bCloneKeys  = true;
            _bDeleteKeys = true;
        }
        else {
            _bCloneKeys  = false;
            _bDeleteKeys = bDeleteKeys;
        }
        _pFirstAdded = _pHashtable;
        _pLastAdded  = NULL;
    }

    template <class T> FourStringHashtable<T>::FourStringHashtable (unsigned short usInitSize,
                                                                      bool bCaseSensitiveKeys,
                                                                      bool bCloneKeys,
                                                                      bool bDeleteKeys)
    {
        _usCount     = 0;
        _usInitSize  = (usInitSize>=5? usInitSize: 23);
        _pHashtable  = new HashtableEntry[_usInitSize];
        _usTableSize = _usInitSize;
        _ulState     = 0;
        _bCaseSensitiveKeys = bCaseSensitiveKeys;
        if (bCloneKeys) {
            _bCloneKeys  = true;
            _bDeleteKeys = true;
        }
        else {
            _bCloneKeys  = false;
            _bDeleteKeys = bDeleteKeys;
        }
        _pFirstAdded = _pHashtable;
        _pLastAdded  = NULL;
    }

    template <class T> FourStringHashtable<T>::~FourStringHashtable (void)
    {
        deleteTable (_pHashtable, _usTableSize);
        _pHashtable  = NULL;
        _usCount     = 0;
        _usTableSize = 0;
        _pFirstAdded = NULL;
        _pLastAdded  = NULL;
    }

    template <class T> T * FourStringHashtable<T>::put (const char *pszKeyOne,
                                                         const char *pszKeyTwo,
                                                         const char *pszKeyThree,
                                                         const char *pszKeyFour, 
                                                         T* pValue)
    {
        if ( (pszKeyOne == NULL) || (pszKeyTwo == NULL) || (pszKeyThree == NULL) || (pszKeyFour == NULL) || (pValue == NULL) ){
            return NULL;
        }

        if (_usCount >= _usTableSize*0.75) {
            rehash();
        }
        if (_usCount >= 20) {
            // No longer use linear search after that many elements
            _pFirstAdded = NULL;
        }
        int hashValue = hashCode2(pszKeyOne, pszKeyTwo, pszKeyThree, pszKeyFour);
        HashtableEntry *pHTE = &_pHashtable[hashValue];
        if (pHTE->pszKeyOne == NULL) {
            if (_bCloneKeys) {
                pHTE->pszKeyOne   = new char[strlen(pszKeyOne)+1];
                pHTE->pszKeyTwo   = new char[strlen(pszKeyTwo)+1];
                pHTE->pszKeyThree = new char[strlen(pszKeyThree)+1];
                pHTE->pszKeyFour = new char[strlen(pszKeyFour)+1];
                strcpy (pHTE->pszKeyOne,   pszKeyOne);
                strcpy (pHTE->pszKeyTwo,   pszKeyTwo);
                strcpy (pHTE->pszKeyThree, pszKeyThree);
                strcpy (pHTE->pszKeyFour, pszKeyFour);
            }
            else {
                pHTE->pszKeyOne   = (char*) pszKeyOne;
                pHTE->pszKeyTwo   = (char*) pszKeyTwo;
                pHTE->pszKeyThree = (char*) pszKeyThree;
                pHTE->pszKeyFour   = (char*) pszKeyFour;
            }
            pHTE->pValue = pValue;
            pHTE->pNext = NULL;
            if (_pFirstAdded != NULL) {
                if (_usCount) {
                    _pLastAdded->pNextLinear = pHTE;
                }
                else {
                    _pFirstAdded = pHTE;
                }
                _pLastAdded = pHTE;
            }
            _usCount++;
        }
        else if (
                 (0 == keycomp (pHTE->pszKeyTwo,   pszKeyTwo)) && 
                 (0 == keycomp (pHTE->pszKeyThree, pszKeyThree)) &&
                 (0 == keycomp (pHTE->pszKeyFour, pszKeyFour)) &&
                 (0 == keycomp (pHTE->pszKeyOne,   pszKeyOne))
                 ) {
            T * pOldValue = pHTE->pValue;
            pHTE->pValue = pValue;
            // Check to see if key1 or key2 or key3 or key4 should be deleted
            if (_bDeleteKeys) {
                delete[] pHTE->pszKeyOne;
                delete[] pHTE->pszKeyTwo;
                delete[] pHTE->pszKeyThree;
                delete[] pHTE->pszKeyFour;
                pHTE->pszKeyOne = NULL;
                pHTE->pszKeyTwo = NULL;
                pHTE->pszKeyThree = NULL;
                pHTE->pszKeyFour = NULL;
            }
            // Don't check to see if value should be deleted because
            //     the value is returned
            if (_bCloneKeys) {
                pHTE->pszKeyOne   = new char [strlen(pszKeyOne)+1];
                pHTE->pszKeyTwo   = new char [strlen(pszKeyTwo)+1];
                pHTE->pszKeyThree   = new char [strlen(pszKeyThree)+1];
                pHTE->pszKeyFour = new char [strlen(pszKeyFour)+1];
                strcpy (pHTE->pszKeyOne,   pszKeyOne);
                strcpy (pHTE->pszKeyTwo,   pszKeyTwo);
                strcpy (pHTE->pszKeyThree,   pszKeyThree);
                strcpy (pHTE->pszKeyFour, pszKeyFour);
            }
            else {
                pHTE->pszKeyOne   = (char*) pszKeyOne;
                pHTE->pszKeyTwo   = (char*) pszKeyTwo;
                pHTE->pszKeyThree   = (char*) pszKeyThree;
                pHTE->pszKeyFour = (char*) pszKeyFour;
            }
            return pOldValue;
        }
        else {
            while (pHTE->pNext != NULL) {
                pHTE = pHTE->pNext;
                if (
                    (0 == keycomp (pHTE->pszKeyTwo,   pszKeyTwo)) && 
                    (0 == keycomp (pHTE->pszKeyThree, pszKeyThree)) && 
                    (0 == keycomp (pHTE->pszKeyFour, pszKeyFour)) && 
                    (0 == keycomp (pHTE->pszKeyOne,   pszKeyOne))
                    ) {
                    T * pOldValue = pHTE->pValue;
                    pHTE->pValue = pValue;
                    // Check to see if key should be deleted
                    if (_bDeleteKeys) {
                        delete[] pHTE->pszKeyOne;
                        delete[] pHTE->pszKeyTwo;
                        delete[] pHTE->pszKeyThree;
                        delete[] pHTE->pszKeyFour;
                        pHTE->pszKeyOne = NULL;
                        pHTE->pszKeyTwo = NULL;
                        pHTE->pszKeyThree = NULL;
                        pHTE->pszKeyFour = NULL;
                    }
                    // Don't check to see if value should be deleted because
                    //     the value is returned
                    if (_bCloneKeys) {
                        pHTE->pszKeyOne   = new char [strlen(pszKeyOne)+1];
                        pHTE->pszKeyTwo   = new char [strlen(pszKeyTwo)+1];
                        pHTE->pszKeyThree   = new char [strlen(pszKeyThree)+1];
                        pHTE->pszKeyFour = new char [strlen(pszKeyFour)+1];
                        strcpy (pHTE->pszKeyOne,   pszKeyOne);
                        strcpy (pHTE->pszKeyTwo,   pszKeyTwo);
                        strcpy (pHTE->pszKeyThree,   pszKeyThree);
                        strcpy (pHTE->pszKeyFour, pszKeyFour);
                    }
                    else {
                        pHTE->pszKeyOne   = (char*) pszKeyOne;
                        pHTE->pszKeyTwo   = (char*) pszKeyTwo;
                        pHTE->pszKeyThree   = (char*) pszKeyThree;
                        pHTE->pszKeyFour = (char*) pszKeyFour;
                    }
                    return pOldValue;
                }
            }
            HashtableEntry *pNew = new HashtableEntry;
            if (_bCloneKeys) {
                pNew->pszKeyOne   = new char [strlen(pszKeyOne)+1];
                pNew->pszKeyTwo   = new char [strlen(pszKeyTwo)+1];
                pNew->pszKeyThree   = new char [strlen(pszKeyThree)+1];
                pNew->pszKeyFour = new char [strlen(pszKeyFour)+1];
                strcpy (pNew->pszKeyOne,   pszKeyOne);
                strcpy (pNew->pszKeyTwo,   pszKeyTwo);
                strcpy (pNew->pszKeyThree,   pszKeyThree);
                strcpy (pNew->pszKeyFour, pszKeyFour);
            }
            else {
                pNew->pszKeyOne   = (char*) pszKeyOne;
                pNew->pszKeyTwo   = (char*) pszKeyTwo;
                pNew->pszKeyThree   = (char*) pszKeyThree;
                pNew->pszKeyFour = (char*) pszKeyFour;
            }
            pNew->pValue = pValue;
            pNew->pNext = NULL;
            pHTE->pNext = pNew;
            if (_pFirstAdded != NULL) {
                _pLastAdded->pNextLinear = pNew;
                _pLastAdded = pNew;
            }
            _usCount++;
        }
        return NULL;
    }

    template <class T> T* FourStringHashtable<T>::getCaseS (const char *pszKeyOne,
                                                             const char *pszKeyTwo,
                                                             const char *pszKeyThree,
                                                             const char *pszKeyFour)
    {
        struct HashtableEntry *pHTE;
        // First try linear search if _pFirstAdded is not yet NULL
        for (pHTE = _pFirstAdded; pHTE != NULL; pHTE = pHTE->pNextLinear) {
    	    if (pHTE->pszKeyOne && pHTE->pszKeyTwo && pHTE->pszKeyThree && pHTE->pszKeyFour) {
                if ((0 == strcmp (pszKeyTwo,   pHTE->pszKeyTwo)) && 
            	    (0 == strcmp (pszKeyThree, pHTE->pszKeyThree)) &&
                    (0 == strcmp (pszKeyFour, pHTE->pszKeyFour)) &&
                    (0 == strcmp (pszKeyOne,   pHTE->pszKeyOne))) {
                    return (pHTE->pValue);
                }
    	    }
        }
        if ((_pFirstAdded != NULL) || (pszKeyTwo==NULL)) {
            return NULL;
        }
        // Compute the hashValue right here instead of invoking hashcode2()
        // (the inline definition or hashcode2 is not being honored by the compiler)
        const char * psk2 = pszKeyTwo;
        int hashValue = 0;
        while (*psk2 != '\0') {
            hashValue = (64*hashValue + *psk2) % _usTableSize;
            if (*(++psk2)=='\0') break;
            psk2++;
        }
        
        for (pHTE = &_pHashtable[hashValue]; pHTE != NULL; pHTE = pHTE->pNext) {
    	    if (pHTE->pszKeyOne && pHTE->pszKeyTwo && pHTE->pszKeyThree && pHTE->pszKeyFour) {
    		    if ((0 == strcmp (pszKeyTwo,   pHTE->pszKeyTwo)) && 
    			    (0 == strcmp (pszKeyThree, pHTE->pszKeyThree)) &&	
    			    (0 == strcmp (pszKeyFour, pHTE->pszKeyFour)) &&
    			    (0 == strcmp (pszKeyOne,   pHTE->pszKeyOne))) {
    			    return (pHTE->pValue);
    		    }
    	    }
        }
        return NULL;
    }



    template <class T> T* FourStringHashtable<T>::getCaseI (const char *pszKeyOne,
                                                             const char *pszKeyTwo,
                                                             const char *pszKeyThree,
                                                             const char *pszKeyFour)
    {
        struct HashtableEntry *pHTE;
        // First try linear search if _pFirstAdded is not yet NULL
        for (pHTE = _pFirstAdded; pHTE != NULL; pHTE = pHTE->pNextLinear) {
    	    if (pHTE->pszKeyOne && pHTE->pszKeyTwo && pHTE->pszKeyThree && pHTE->pszKeyFour) {
    		    if ((0 == stricmp (pszKeyTwo,   pHTE->pszKeyTwo)) && 
    			    (0 == stricmp (pszKeyThree, pHTE->pszKeyThree)) &&	
    			    (0 == stricmp (pszKeyFour, pHTE->pszKeyFour)) &&
    			    (0 == stricmp (pszKeyOne,   pHTE->pszKeyOne))) {
    			    return (pHTE->pValue);
    		    }
    	    }
        }
        if ((_pFirstAdded != NULL) || (pszKeyTwo==NULL)) {
            return NULL;
        }

        // Compute the hashValue right here instead of invoking hashcode2()
        // (the inline definition or hashcode2 is not being honored by the compiler)
        const char * psk2 = pszKeyTwo;
        int hashValue = 0;
        while (*psk2 != '\0') {
            hashValue = (64*hashValue + *psk2) % _usTableSize;
            if (*(++psk2)=='\0') break;
            psk2++;
        }

        for (pHTE = &_pHashtable[hashValue]; pHTE != NULL; pHTE = pHTE->pNext) {
    	    if (pHTE->pszKeyOne && pHTE->pszKeyTwo && pHTE->pszKeyThree && pHTE->pszKeyFour) {
    		    if ((0 == stricmp (pszKeyTwo,   pHTE->pszKeyTwo)) && 
    			    (0 == stricmp (pszKeyThree, pHTE->pszKeyThree)) &&
    			    (0 == stricmp (pszKeyFour, pHTE->pszKeyFour)) &&
    			    (0 == stricmp (pszKeyOne,   pHTE->pszKeyOne))) {
    			    return (pHTE->pValue);
    		    }
    	    }
        }
        return NULL;
    }

    template <class T> inline T * FourStringHashtable<T>::get (const char *pszKeyOne,
                                                                const char *pszKeyTwo,
                                                                const char *pszKeyThree,
                                                                const char *pszKeyFour)
    {
        if (_bCaseSensitiveKeys) {
            return getCaseS (pszKeyOne, pszKeyTwo, pszKeyThree, pszKeyFour);
        }
        else {
            return getCaseI (pszKeyOne, pszKeyTwo, pszKeyThree, pszKeyFour);
        }
    }


    template <class T> inline int FourStringHashtable<T>::keycomp (const char *pszKeyOne, const char *pszKeyTwo)
    {
        if (_bCaseSensitiveKeys) {
            return strcmp (pszKeyOne, pszKeyTwo);
        }
        else {
            return stricmp (pszKeyOne, pszKeyTwo);
        }
    }

    template <class T> inline int FourStringHashtable<T>::hashCode (const char *pszKeyOne, const char *pszKeyTwo, const char *pszKeyThree, const char *pszKeyFour)
    {
	    // Horner's method for strings calculating the hashcode on each character.
        int iHashValue;
        if (_bCaseSensitiveKeys) {
            for (iHashValue=0; *pszKeyOne != '\0'; pszKeyOne++) {
                iHashValue = (64*iHashValue + *pszKeyOne) % _usTableSize;
            }
            for (; *pszKeyTwo != '\0'; pszKeyTwo++) {
                iHashValue = (64*iHashValue + *pszKeyTwo) % _usTableSize;
            }
            for (; *pszKeyThree != '\0'; pszKeyThree++) {
                iHashValue = (64*iHashValue + *pszKeyThree) % _usTableSize;
            }
            for (; *pszKeyFour != '\0'; pszKeyFour++) {
                iHashValue = (64*iHashValue + *pszKeyFour) % _usTableSize;
            }
        }
        else {
            for (iHashValue=0; *pszKeyOne != '\0'; pszKeyOne++) {
                iHashValue = (64*iHashValue + tolower(*pszKeyOne)) % _usTableSize;
            }
            for (; *pszKeyTwo != '\0'; pszKeyTwo++) {
                iHashValue = (64*iHashValue + tolower(*pszKeyTwo)) % _usTableSize;
            }
            for (; *pszKeyThree != '\0'; pszKeyThree++) {
                iHashValue = (64*iHashValue + tolower(*pszKeyThree)) % _usTableSize;
            }
            for (; *pszKeyFour != '\0'; pszKeyFour++) {
                iHashValue = (64*iHashValue + tolower(*pszKeyFour)) % _usTableSize;
            }        
        }
        return iHashValue;
    }

    template <class T> inline int FourStringHashtable<T>::hashCode2 (const char *pszKeyOne, const char *pszKeyTwo, const char *pszKeyThree, const char *pszKeyFour)
    {
        // Horner's method for strings calculating the hashcode on every two characters.   
        int iHashValue = 0;
        if (_bCaseSensitiveKeys) {
    	    while (*pszKeyOne != '\0') {
   	   		    iHashValue = (64*iHashValue + *pszKeyOne) % _usTableSize;
   	   		    if (*(++pszKeyOne)=='\0') break;
   	   			    pszKeyOne++;
    	    }
    	    while (*pszKeyTwo != '\0') {
    		    iHashValue = (64*iHashValue + *pszKeyTwo) % _usTableSize;
    		    if (*(++pszKeyTwo)=='\0') break;
    			    pszKeyTwo++;
    	    }
    	    while (*pszKeyThree != '\0') {
    		    iHashValue = (64*iHashValue + *pszKeyThree) % _usTableSize;
    		    if (*(++pszKeyThree)=='\0') break;
    			    pszKeyThree++;
    	    }   
    	    while (*pszKeyFour != '\0') {
    		    iHashValue = (64*iHashValue + *pszKeyFour) % _usTableSize;
    		    if (*(++pszKeyFour)=='\0') break;
    			    pszKeyFour++;
    	    }
        }
        else {
    	    while (*pszKeyOne != '\0') {
    	   	    iHashValue = (64*iHashValue + tolower(*pszKeyOne)) % _usTableSize;
    	        if (*(++pszKeyOne)=='\0') break;
    	      	    pszKeyOne++;
   	    	    }
            while (*pszKeyTwo != '\0') {
        	    iHashValue = (64*iHashValue + tolower(*pszKeyTwo)) % _usTableSize;
  	            if (*(++pszKeyTwo)=='\0') break;
   	           	    pszKeyTwo++;
   	        }
   	        while (*pszKeyThree != '\0') {
   	            iHashValue = (64*iHashValue + tolower(*pszKeyThree)) % _usTableSize;
   	            if (*(++pszKeyThree)=='\0') break;
   	          	    pszKeyThree++;
   	        }  
   	        while (*pszKeyFour != '\0') {
   	    	    iHashValue = (64*iHashValue + tolower(*pszKeyFour)) % _usTableSize;
   	     	    if (*(++pszKeyFour)=='\0') break;
   	     		    pszKeyFour++;
   	        }
        }
        return iHashValue;
    }

    template <class T> T* FourStringHashtable<T>::remove (const char *pszKeyOne, const char *pszKeyTwo, const char *pszKeyThree, const char *pszKeyFour)
    {
        if (pszKeyTwo == NULL) {
            return NULL;
        }

        _pFirstAdded  = NULL; // Don't use linear search any more after any removal
        int hashValue = hashCode2(pszKeyOne, pszKeyTwo, pszKeyThree, pszKeyFour);
        HashtableEntry *pHTE = &_pHashtable[hashValue];
        if ((pHTE->pszKeyOne == NULL) && (pHTE->pszKeyTwo == NULL) && (pHTE->pszKeyThree == NULL) && (pHTE->pszKeyFour == NULL)) {
            // Hashtable bucket is empty - therefore the keys must not exist
            return NULL;
        }
        else {
            if ((0 == keycomp (pHTE->pszKeyTwo,   pszKeyTwo)) && 
                (0 == keycomp (pHTE->pszKeyFour, pszKeyFour)) &&
                (0 == keycomp (pHTE->pszKeyOne,   pszKeyOne))) {
                // Deleting entry in hashtable array
                T *pOldValue = pHTE->pValue;
                if (pHTE->pNext) {
                    HashtableEntry *pTemp = pHTE->pNext;
                    // Check to see if key should be deleted
                    if (_bDeleteKeys) {
                        delete[] pHTE->pszKeyOne;
                        delete[] pHTE->pszKeyTwo;
                        delete[] pHTE->pszKeyThree;
                        delete[] pHTE->pszKeyFour;
                    }
                    // Don't check to see if value should be deleted because
                    //     the value is returned
                    pHTE->pszKeyOne   = pTemp->pszKeyOne;
                    pHTE->pszKeyTwo   = pTemp->pszKeyTwo;
                    pHTE->pszKeyThree   = pTemp->pszKeyThree;
                    pHTE->pszKeyFour = pTemp->pszKeyFour;
                    pHTE->pValue       = pTemp->pValue;
                    pHTE->pNext       = pTemp->pNext;
                    delete pTemp;
                }
                else {
                    // Check to see if the keys should be deleted
                    if (_bDeleteKeys) {
                        delete[] pHTE->pszKeyOne;
                        delete[] pHTE->pszKeyTwo;
                        delete[] pHTE->pszKeyThree;
                        delete[] pHTE->pszKeyFour;
                    }
                    // Otherwise set the pointers to null.
                    // Don't check to see if value should be deleted because
                    //     the value is returned
                    pHTE->pszKeyOne   = NULL;
                    pHTE->pszKeyTwo   = NULL;
                    pHTE->pszKeyThree   = NULL;
                    pHTE->pszKeyFour = NULL;
                    pHTE->pValue       = NULL;
                }
                return pOldValue;
            }
            else {
                HashtableEntry *pPrev = pHTE;
                pHTE = pHTE->pNext;
                while (pHTE != NULL) {
                    if ((0 == keycomp (pHTE->pszKeyOne,   pszKeyOne)) &&
                        (0 == keycomp (pHTE->pszKeyTwo,   pszKeyTwo)) && 
                        (0 == keycomp (pHTE->pszKeyThree,   pszKeyThree)) && 
                        (0 == keycomp (pHTE->pszKeyFour, pszKeyFour))) {
                        // Found the node
                        T * pOldValue = pHTE->pValue;
                        pPrev->pNext = pHTE->pNext;
                        // Check to see if the keys should be deleted
                        if (_bDeleteKeys) {
                            delete[] pHTE->pszKeyOne;
                            delete[] pHTE->pszKeyTwo;
                            delete[] pHTE->pszKeyThree;
                            delete[] pHTE->pszKeyFour;
                        }
                        // Don't check to see if value should be deleted because
                        //     the value is returned
                        delete pHTE;
                        return pOldValue;
                    }
                    else {
                        pPrev = pHTE;
                        pHTE = pHTE->pNext;
                    }
                }
            }
        }
        return NULL;
    }

    template <class T> void FourStringHashtable<T>::removeAll(void)
    {
        deleteTable (_pHashtable, _usTableSize);    
        _pHashtable  = new HashtableEntry[_usInitSize];
        _usTableSize = _usInitSize;
        _usCount     = 0;
        _ulState     = 0;
        _pFirstAdded = _pHashtable; // We can use linear search again in the get method
        _pLastAdded  = NULL;
    }

    template <class T> inline typename FourStringHashtable<T>::Iterator FourStringHashtable<T>::getAllElements (void)
    {
        return Iterator (this, _ulState);
    }

    template <class T> void FourStringHashtable<T>::printStructure (void)
    {
        for (unsigned short us = 0; us < _usTableSize; us++) {
            unsigned short usCount = 0;
            if ((_pHashtable[us].pszKeyOne) && (_pHashtable[us].pszKeyTwo) && (_pHashtable[us].pszKeyThree) && (_pHashtable[us].pszKeyFour)) {
                usCount++;
                HashtableEntry *pHTE = _pHashtable[us].pNext;
                while (pHTE != NULL) {
                    usCount++;
                    pHTE = pHTE->pNext;
                }
            }
            printf ("[%d] %d ", (int) us, (int) usCount);
            while (usCount-- > 0) {
                printf ("*");
            }
            printf ("\n");
        }
    }

    template <class T> void FourStringHashtable<T>::rehash()
    {
        HashtableEntry *pOldHashtable = _pHashtable;
        unsigned short usOldSize = _usTableSize;
        _usTableSize = usOldSize * 2 + 1;
        _pHashtable  = new HashtableEntry[_usTableSize];
        _usCount     = 0;
        _pFirstAdded = _pHashtable;
        _pLastAdded  = NULL;

        for (unsigned short us = 0; us < usOldSize; us++) {
            for (HashtableEntry *pHTE = &pOldHashtable[us]; pHTE != NULL; pHTE = pHTE->pNext) {
                if ((pHTE->pszKeyOne) && (pHTE->pszKeyTwo) && (pHTE->pszKeyThree) && (pHTE->pszKeyFour)) {
                    put (pHTE->pszKeyOne, pHTE->pszKeyTwo, pHTE->pszKeyThree, pHTE->pszKeyFour, pHTE->pValue);
                }
            }
        }
        deleteTable (pOldHashtable, usOldSize);
    }

    template <class T> void FourStringHashtable<T>::deleteTable (HashtableEntry *pTable, unsigned short usSize)
    {
        for (unsigned short us = 0; us < usSize; us++) {
            HashtableEntry *pHTE = pTable[us].pNext;
            while (pHTE != NULL) {
                HashtableEntry *pTmp = pHTE;
                pHTE = pHTE->pNext;
                if (_bDeleteKeys) {
                    delete[] pTmp->pszKeyOne;
                    delete[] pTmp->pszKeyTwo;
                    delete[] pTmp->pszKeyThree;
                    delete[] pTmp->pszKeyFour;
                }
                delete pTmp;
            }
        }
        delete[] pTable;
    }

}

#endif   // #ifndef INCL_Four_STRING_HASHTABLE_H
