/*
 * StrClass.h
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
 */

#ifndef INCL_STRCLASS_H
#define INCL_STRCLASS_H

#include "FTypes.h"

namespace NOMADSUtil
{

    class String
    {
        public:
            String (void);

            // Construct an empty string with a buffer of the specified size
            String (unsigned short usSize);

            // Construct a String object using the specified string as the initial value
            String (const char *pszStr);

            // Construct a String object using a substring from the specified string as the initial value
            // The substring will contain usCount characters
            String (const char *pszStr, unsigned short usCount);

            // Copy Constructor
            String (const String &sourceStr);

            ~String (void);

            operator char * (void);
            operator const char * (void) const;
            String operator + (const String &str) const;
            String operator + (const char *pszStr) const;
            String operator + (char c) const;
            String & operator = (const String &newStr);
            String & operator = (const char *pszStr);
            String & operator += (const String &str);
            String & operator += (const char *pszStr);
            String & operator += (char c);
            String & operator += (unsigned char c);
            String & operator += (uint32 ui32);
            char & operator [] (int iPos) const;

            // Performs and exact (case-sensitive) comparison of the two strings
            // Returns 1 if they are equal and 0 if they are not
            int operator == (const String &rhsStr) const;
            int operator == (String &rhsStr);
            int operator == (const char *pszrhsStr) const;

            // Performs a case-insensitive comparison of the two strings
            // Returns 1 if they are equal and 0 if they are not
            int operator ^= (const String &rhsStr) const;
            int operator ^= (const char *pszrhsStr) const;

            int operator != (const String &rhsStr) const;
            int operator != (const char *pszrhsStr) const;
            int length (void) const;
            bool contains (const char *pszStr) const;
            int convertToLowerCase (void);
            int convertToUpperCase (void);
            void setSize (int iNewSize);
            int trim (void);
            int indexOf (char ch) const;
            int indexOf (const char *pszStr) const;
            int lastIndexOf (char ch) const;
            String substring (int beginIndex, int endIndex) const;
            int startsWith (const char *pszStr) const;
            int endsWith (const char *pszStr) const;

            // Returns the null-terminated sequence of characters stored in the
            // internal buffer
            const char * c_str (void) const;

            // Relinquish the null-terminated sequence of characters stored in
            // the internal buffer and returns it.
            // The caller is in char to deallocate the returned buffer.
            char * r_str (void);

        private:
            char * strdup (const char *pszStr) const;

        private:
            char *pszBuf;
    };


    inline String::operator char * (void)
    {
        return pszBuf;
    }

    inline String::operator const char * (void) const
    {
        return (const char*) pszBuf;
    }

    inline String & String::operator = (const String &newStr)
    {
        return ((*this) = newStr.pszBuf);
    }

    inline String & String::operator += (const String &str)
    {
        return ((*this) += str.pszBuf);
    }

    inline int String::operator == (const String &rhsStr) const
    {
        return ((*this) == ((const char *) rhsStr.pszBuf));
    }

    inline int String::operator == (String &rhsStr)
    {
        return ((*this) == ((const char *) rhsStr.pszBuf));
    }

    inline int String::operator ^= (const String &rhsStr) const
    {
        return ((*this) ^= rhsStr.pszBuf);
    }

    inline int String::operator != (const String &rhsStr) const
    {
        return !((*this) == ((const char*) rhsStr.pszBuf));
    }

    inline int String::operator != (const char *pszrhsStr) const
    {
        return !((*this) == pszrhsStr);
    }

    inline const char* String::c_str() const
    {
        return pszBuf;
    }

    // To allow char* + String concatenation operations
    inline String operator + (char * const pszLhs, const String &rsRhs)
    {
        return String (pszLhs) + rsRhs;
    }

    inline const String operator + (const char * const pszLhs, const String &rsRhs)
    {
        return String (pszLhs) + rsRhs;
    }

    // To allow char* == String comparison operations
    inline bool operator== (char * const pszLhs, String &rsRhs)
    {
        return (rsRhs.operator == (pszLhs)) != 0;
    }

    inline bool operator== (const char * const pszLhs, const String &rsRhs)
    {
        return (rsRhs.operator == (pszLhs)) != 0;
    }
}

#endif    // #ifndef INCL_STRCLASS_H