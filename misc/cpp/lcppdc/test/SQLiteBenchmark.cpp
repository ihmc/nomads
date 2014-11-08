/* 
 * File:   main.cpp
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 *
 * Created on January 29, 2011, 12:58 AM
 */

#include "SQLiteFactory.h"

#include "FTypes.h"
#include "NLFLib.h"
#include "StrClass.h"

#include "sqlite3.h"

#include <stdlib.h>

#include <stdio.h>
#include <stdlib.h>

using namespace NOMADSUtil;
using namespace IHMC_MISC;

float MAX_LAT = 90.0f;
float MIN_LAT = -MAX_LAT;

float MAX_LON = 180.0f;
float MIN_LON = -MAX_LON;

const char * floatToString (float fVal)
{
    char * pszVal = (char *) calloc (1, sizeof(fVal)+1);
    sprintf (pszVal, "%f", fVal);
    return pszVal;
}

float stringToFloat (const char * pszVal)
{
    float f;
    sscanf(pszVal, "%f", &f);
    return f;
}

float getRandomFloat (float min, float max)
{
    return ((max - min) * ((float)random()/RAND_MAX)) + min;
}

float getRandomFloatCoord (float min, float max, float &minCoord, float &maxCoord)
{
    minCoord = getRandomFloat (min, max);
    maxCoord = getRandomFloat (max, min);
    if (minCoord > maxCoord) {
        float tmp = maxCoord;
        maxCoord = minCoord;
        minCoord = tmp;
    }
}

int main(int argc, char** argv)
{
    srandom (getTimeInMilliseconds());

    const char * pszFileName = argv[1];

    char *errMessage;
    char **queryResults;
    int noRows, noColumns;

    sqlite3 *_pDB;
    int rc = sqlite3_enable_shared_cache(false);
    if (rc != SQLITE_OK) {
        printf ("Coud not disable shared cache %s\n", SQLiteFactory::getErrorAsString(rc));
        return -1;
    }
    rc = sqlite3_open(pszFileName, &_pDB);
    if (rc != SQLITE_OK) {
        printf ("Can't open the database %s\n", SQLiteFactory::getErrorAsString(rc));
        sqlite3_close(_pDB);
        _pDB = NULL;
        return -1;
    }
    else {
        printf ("DB %s opened successfully\n", pszFileName);
    }

    sqlite3_enable_load_extension (_pDB, 1);

    //void *c = (void *)RTREE_COORD_REAL32;
    //rc = sqlite3_create_module_v2(_pDB, "rtree", &rtreeModule, c, 0);

    const char *pszCreatVTab = "CREATE VIRTUAL TABLE MetaData_Table_GeoIndex USING rtee (id, Right_Lower_Longitude, Left_Upper_Longitude, Left_Upper_Latitude, Right_Lower_Latitude);";
    rc = sqlite3_get_table(_pDB, pszCreatVTab, &queryResults, &noRows, &noColumns, &errMessage);
    if (rc != 0) {
        printf ("Error %s %s: %s\n", SQLiteFactory::getErrorAsString(rc), errMessage, pszCreatVTab);
        return -1;
    }
    const char *pszInser = "INSERT INTO MetaData_Table_GeoIndex SELECT rowid, Right_Lower_Longitude, Left_Upper_Longitude, Left_Upper_Latitude, Right_Lower_Latitude FROM MetaData_Table";
    rc = sqlite3_get_table(_pDB, pszInser, &queryResults, &noRows, &noColumns, &errMessage);
    if (rc != 0) {
        printf ("Error %s %s: %s\n", SQLiteFactory::getErrorAsString(rc), errMessage, pszInser);
        return -1;
    }
    exit (0);

    const char *pszMetaMaxLat = "Left_Upper_Latitude";
    const char *pszMetaMinLat = "Right_Lower_Latitude";
    const char *pszMetaMaxLong = "Right_Lower_Longitude";
    const char *pszMetaMinLong = "Left_Upper_Longitude";

    String sql;

    int64 i64Begin = getTimeInMilliseconds();

    for (int i = 0; i < 100; i++) {
        sql = (String) "select * from MetaData_Table where (Left_Upper_Latitude > " + floatToString (getRandomFloat (MIN_LAT, MAX_LAT))
            + " OR Right_Lower_Latitude < " + floatToString (getRandomFloat(MIN_LAT, MAX_LAT)) + ") and (Left_Upper_Longitude < "
            + floatToString (getRandomFloat (MIN_LON, MAX_LON)) + " OR Right_Lower_Longitude < " + floatToString (getRandomFloat (MIN_LON, MAX_LON)) + ")";
        rc = sqlite3_get_table(_pDB, (const char *) sql, &queryResults, &noRows, &noColumns, &errMessage);
        if (rc != 0) {
            printf ("Error %s %s: %s\n", SQLiteFactory::getErrorAsString(rc), errMessage, (const char *)sql);
            return -1;
        }
        sqlite3_free_table(queryResults);

        float fMinLat, fMaxLat;
        getRandomFloatCoord (MIN_LAT, MAX_LAT, fMinLat, fMaxLat);
        float fMinLon, fMaxLon;
        getRandomFloatCoord (MIN_LON, MAX_LON, fMinLon, fMaxLon);

        sql = (String) "select * from MetaData_Table where "
            + " ((" + pszMetaMinLat + " < " + fMaxLat + " OR "
            + pszMetaMaxLat + " > " + fMinLat + ") AND ("
            + pszMetaMinLong + " < " + fMaxLon + " OR "
            + pszMetaMaxLong + " > " + fMinLon + "))";

        rc = sqlite3_get_table(_pDB, (const char *)sql, &queryResults, &noRows, &noColumns, &errMessage);
        if (rc != 0) {
            printf ("Error %s %s: %s\n", SQLiteFactory::getErrorAsString(rc), errMessage, (const char *)sql);
            return -1;
        }
        sqlite3_free_table(queryResults);
    }

    int64 i64End = getTimeInMilliseconds();

    printf ("It took %lld\n", (i64End-i64Begin));
    return 0;
}

