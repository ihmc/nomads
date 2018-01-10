#include "RegUtils.h"

#include "Logger.h"

#define checkAndLogMsg if (pLogger) pLogger->logMsg
#define NOMINMAX
#include <winsock2.h>
#include <windows.h>

namespace NOMADSUtil
{

RegEntry::RegEntry (void)
{
    type = RET_Undefined;
    ui32Value = 0;
    pValue = NULL;
    ui32ValueLen = 0;
}

RegEntry::RegEntry (const RegEntry &rhsRE)
{
    name = rhsRE.name;
    type = rhsRE.type;
    value = rhsRE.value;
    ui32Value = rhsRE.ui32Value;
    if ((rhsRE.ui32ValueLen > 0) && (rhsRE.pValue)) {
        ui32ValueLen = rhsRE.ui32ValueLen;
        pValue = malloc (ui32ValueLen);
        memcpy (pValue, rhsRE.pValue, ui32ValueLen);
    }
    else {
        pValue = NULL;
        ui32ValueLen = 0;
    }
}

RegEntry::~RegEntry (void)
{
    type = RET_Undefined;
    ui32Value = 0;
    if (pValue) {
        free (pValue);
        pValue = NULL;
    }
    ui32ValueLen = 0;
}

RegEntry & RegEntry::operator = (const RegEntry &rhsRE)
{
    name = rhsRE.name;
    type = rhsRE.type;
    value = rhsRE.value;
    ui32Value = rhsRE.ui32Value;
    if ((rhsRE.ui32ValueLen > 0) && (rhsRE.pValue)) {
        ui32ValueLen = rhsRE.ui32ValueLen;
        pValue = malloc (ui32ValueLen);
        memcpy (pValue, rhsRE.pValue, ui32ValueLen);
    }
    else {
        pValue = NULL;
        ui32ValueLen = 0;
    }
    return *this;
}

RegKeys * getRegistrySubKeys (const char *pszRegistryKey)
{
    int rc;
    HKEY regKey;
    if (pszRegistryKey == NULL) {
        checkAndLogMsg ("getRegistrySubKeys", Logger::L_MildError,
                        "pszRegistryKey parameter is NULL\n");
        return NULL;
    }
    if (0 != (rc = RegOpenKeyEx (HKEY_LOCAL_MACHINE, pszRegistryKey, 0, KEY_READ, &regKey))) {
        checkAndLogMsg ("getRegistrySubKeys", Logger::L_MildError,
                        "failed to open registry using key <%s>; rc = %d\n",
                        pszRegistryKey, rc);
        return NULL;
    }
    RegKeys *pRK = new RegKeys();
    int iKeyIndex = 0;
    while (true) {
        char szKeyName[1024];
        DWORD dwKeyNameLen = sizeof (szKeyName);
        if (0 != (rc = RegEnumKeyEx (regKey, iKeyIndex++, szKeyName, &dwKeyNameLen, 0, NULL, NULL, NULL))) {
            if (rc == ERROR_NO_MORE_ITEMS) {
                RegCloseKey (regKey);
                return pRK;
            }
            else {
                checkAndLogMsg ("getRegistrySubKeys", Logger::L_MildError,
                                "RegEnumKeyEx() failed with rc = %d\n", rc);
                RegCloseKey (regKey);
                delete pRK;
                return NULL;
            }
        }
        else {
            String key (szKeyName);
            pRK->add (key);
        }
    }
}

RegEntries * getRegistryEntries (const char *pszRegistryKey)
{
    int rc;
    HKEY regKey;
    if (pszRegistryKey == NULL) {
        checkAndLogMsg ("getRegistryEntries", Logger::L_MildError,
                        "pszRegistryKey parameter is NULL\n");
        return NULL;
    }
    if (0 != (rc = RegOpenKeyEx (HKEY_LOCAL_MACHINE, pszRegistryKey, 0, KEY_READ, &regKey))) {
        checkAndLogMsg ("getRegistryEntries", Logger::L_MildError,
                        "failed to open registry using key <%s>; rc = %d\n",
                        pszRegistryKey, rc);
        return NULL;
    }
    RegEntries *pRE = new RegEntries (true, true, true, true);
    int iKeyIndex = 0;
    while (true) {
        char szValueName[1024];
        DWORD dwValueNameLen = sizeof (szValueName);
        DWORD dwValueType;
        BYTE data[1024];
        DWORD dwDataLen = sizeof (data);
        if (0 != (rc = RegEnumValue (regKey, iKeyIndex++, szValueName, &dwValueNameLen, 0, &dwValueType, data, &dwDataLen))) {
            if (rc == ERROR_NO_MORE_ITEMS) {
                RegCloseKey (regKey);
                return pRE;
            }
            else {
                checkAndLogMsg ("getRegistryEntries", Logger::L_MildError,
                                "RegEnumKeyEx() failed with rc = %d\n", rc);
                RegCloseKey (regKey);
                delete pRE;
                return NULL;
            }
        }
        else {
            RegEntry *pEntry = new RegEntry;
            bool bIgnore = false;
            switch (dwValueType) {
                case REG_BINARY:
                    pEntry->type = RegEntry::RET_Binary;
                    pEntry->ui32ValueLen = dwDataLen;
                    if (pEntry->ui32ValueLen > 0) {
                        pEntry->pValue = malloc (pEntry->ui32ValueLen);
                        memcpy (pEntry->pValue, data, pEntry->ui32ValueLen);
                    }
                    break;
                case REG_SZ:
                    pEntry->type = RegEntry::RET_String;
                    pEntry->value = (const char*) data;
                    break;
                case REG_EXPAND_SZ:
                    pEntry->type = RegEntry::RET_ExpandableString;
                    pEntry->value = (const char*) data;
                    break;
                case REG_MULTI_SZ:
                {
                    pEntry->type= RegEntry::RET_MultiString;
                    const char *pszTemp = (const char *) data;
                    if (pszTemp != NULL) {
                        // Format is: <String1>\0<String2>\0....<StringN>\0\0
                        while (*pszTemp != '\0') {
                            pEntry->values[pEntry->values.getHighestIndex()+1] = pszTemp;
                            while (*pszTemp != '\0') {
                                pszTemp++;
                            }
                            pszTemp++;
                        }
                    }
                    break;
                }
                case REG_DWORD:
                    pEntry->type = RegEntry::RET_DWORD;
                    pEntry->ui32Value = *((DWORD*)data);
                    break;
                case REG_DWORD_BIG_ENDIAN:
                    pEntry->type = RegEntry::RET_DWORD_BigEndian;
                    pEntry->ui32Value = *((DWORD*)data);
                    break;
                default:
                    pEntry->type = RegEntry::RET_Undefined;
                    checkAndLogMsg ("getRegistryEntries", Logger::L_Warning,
                                    "ignoring entry <%s> with value type %d\n",
                                    szValueName, dwValueType);
                    bIgnore = true;
                    break;
            }
            if (!bIgnore) {
                pRE->put (szValueName, pEntry);
            }
            else {
                delete pEntry;
            }
        }
    }
}

}
