/* 
 * File:   Timestamp.cpp
 * Author: gbenincasa
 * 
 * Created on April 28, 2015, 5:03 PM
 */

#include "Timestamp.h"

#include <stdio.h>

using namespace NOMADSUtil;

Timestamp::Timestamp()
{
    _time = time (NULL);
}

Timestamp::Timestamp (int64 i64TimestampInMillis)
{
    _time = (i64TimestampInMillis/1000);
}

Timestamp::~Timestamp()
{
}

String Timestamp::toString()
{
    struct tm *pLocalTime = localtime (&_time);
    static char timestamp[17];
    sprintf (timestamp, "%d%02d%02d-%02d_%02d_%02d",
             pLocalTime->tm_year + 1900,
             pLocalTime->tm_mon + 1,
             pLocalTime->tm_mday,
             pLocalTime->tm_hour,
             pLocalTime->tm_min,
             pLocalTime->tm_sec);
    return String (timestamp);
}

