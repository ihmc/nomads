#ifndef INCL_REGUTILS_H
#define INCL_REGUTILS_H

#include "DArray2.h"
#include "FTypes.h"
#include "StrClass.h"
#include "StringHashtable.h"

namespace NOMADSUtil
{
    struct RegEntry
    {
        RegEntry (void);
        RegEntry (const RegEntry &rhsRE);
        ~RegEntry (void);
        RegEntry & operator = (const RegEntry &rhsRE);
        String name;
        enum Type {
            RET_Undefined = 0,
            RET_Binary = 1,
            RET_DWORD = 2,
            RET_DWORD_LittleEndian = 3,
            RET_DWORD_BigEndian = 4,
            RET_ExpandableString = 5,
            RET_MultiString = 6,
            RET_QWORD = 7,
            RET_QWORD_LittleEndian = 8,
            RET_String = 9
        };
        Type type;
        String value;
        DArray2<String> values;
        uint32 ui32Value;
        void *pValue;
        uint32 ui32ValueLen;
    };

    typedef DArray2<String> RegKeys;
    typedef StringHashtable<RegEntry> RegEntries;

    RegKeys * getRegistrySubKeys (const char *pszRegistryKey);
    RegEntries * getRegistryEntries (const char *pszRegistryKey);
}

#endif   // #ifndef INCL_REGUTILS_H
