/*
 * TClass.cpp
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

#include "TClass.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

using namespace NOMADSUtil;

Time::Time (Modulus mod, Mode mode)
{
    this->mod = mod;
    this->mode = mode;
    set();
}

Time::Time (Modulus mod, Mode mode, time_t t)
{
    this->mod = mod;
    this->mode = mode;
    set(t);
}

Time::Time (Modulus mod, Mode mode, char *pszTime)
{
    this->mod = mod;
    this->mode = mode;
    set(pszTime);
}

Time::Time (Modulus mod, Mode mode, int h, int m, int s)
{
    this->mod = mod;
    this->mode = mode;
    set (h, m, s);
}

Time::Time (Modulus mod, Mode mode, int h, int m, int s, int dw)
{
    this->mod = mod;
    this->mode = mode;
    set (h, m, s, dw);
}

Time::Time (Modulus mod, Mode mode, int h, int m, int s, int dw, int dm)   // dw is ignored
{
    this->mod = mod;
    this->mode = mode;
    set (h, m, s, dw, dm);
}

Time::Time (Modulus mod, Mode mode, int h, int m, int s, int dw, int dm, int mon) // dw is ignored
{
    this->mod = mod;
    this->mode = mode;
    set (h, m, s, dw, dm, mon);
}

Time::Time (Modulus mod, Mode mode, int h, int m, int s, int dw, int dm, int mon, int y)     // dw is ignored
{
    this->mod = mod;
    this->mode = mode;
    set (h, m, s, dw, dm, mon, y);
}

void Time::set (void)
{
    if (mode == Absolute)
    {
         secs = time (NULL);
    }
    else
    {
         secs = 0;
    }
}

void Time::set (time_t t)
{
    secs = t;
}

void Time::set (char *pszTime)
{
    // pszTime should be in the format mm or hh:mm, or hh:mm:ss
    char *apszData[3];
    if (pszTime)
    {
         if (NULL != (apszData[0] = strtok (pszTime, ":")))
         {
              if (NULL != (apszData[1] = strtok (NULL, ":")))
              {
                   if (NULL != (apszData[2] = strtok (NULL, ":")))
                   {
                        set (atoi(apszData[0]), atoi(apszData[1]), atoi(apszData[2]));
                   }
                   else
                   {
                        set (atoi(apszData[0]), atoi(apszData[1]), 0);
                   }
              }
              else
              {
                   set (0, atoi(apszData[0]), 0);
              }
         }
    }
}

void Time::set (int h, int m, int s)
{
    if (mode == Absolute)
    {
         secs = time (NULL);
         struct tm *t = localtime(&secs);
         secs += (s - t->tm_sec);
         secs += (m - t->tm_min)*SECS_IN_MIN;
         secs += (h - t->tm_hour)*SECS_IN_HOUR;
    }
    else
    {
         secs = h*SECS_IN_HOUR + m*SECS_IN_MIN + s;
    }
}

void Time::set (int h, int m, int s, int dw)
{
    if (mode == Absolute)
    {
         secs = time (NULL);
         struct tm *t = localtime (&secs);
         secs += (dw - t->tm_wday) * SECS_IN_DAY;     // Do the date first in case
                                                      // the time changes to daylight savings time
         secs += (s - t->tm_sec);
         secs += (m - t->tm_min) * SECS_IN_MIN;
         secs += (h - t->tm_hour) * SECS_IN_HOUR;
    }
    else
    {
         secs = dw*SECS_IN_DAY + h*SECS_IN_HOUR + m*SECS_IN_MIN + s;
    }
}

void Time::set (int h, int m, int s, int dw, int dm)       // dw is ignored!
{
    if (mode == Absolute)
    {
         secs = time (NULL);
         struct tm *t = localtime (&secs);
         secs += (dm - t->tm_mday) * SECS_IN_DAY;     // Do the date first in case
                                                      // the time changes to daylight savings time
         secs += (s - t->tm_sec);
         secs += (m - t->tm_min) * SECS_IN_MIN;
         secs += (h - t->tm_hour) * SECS_IN_HOUR;
    }
    else
    {
         secs = dm*SECS_IN_DAY + h*SECS_IN_HOUR + m*SECS_IN_MIN + s;
    }
}

void Time::set (int h, int m, int s, int dw, int dm, int mon)     // dw is ignored
{
    mon--;    // System treats January as 0
    if (mode == Absolute)
    {
         secs = time (NULL);
         struct tm *t = localtime (&secs);
         secs += (dm - t->tm_mday) * SECS_IN_DAY;
         t = localtime (&secs);
         if ((dm < 0) || (dm > 31))
         {
              return;
         }
         if ((mon < 0) || (mon > 11))
         {
              return;
         }
         while ((t->tm_mday != dm) || (t->tm_mon != mon))
         {
              if (t->tm_mon > mon)
              {
                   secs -= SECS_IN_DAY * 20;
              }
              else if (t->tm_mon < mon)
              {
                   secs += SECS_IN_DAY * 20;
              }
              else if (t->tm_mday > dm)
              {
                   secs -= SECS_IN_DAY;
              }
              else
              {
                   secs += SECS_IN_DAY;
              }
              t = localtime (&secs);
         }
         secs += (s - t->tm_sec);
         secs += (m - t->tm_min) * SECS_IN_MIN;
         secs += (h - t->tm_hour) * SECS_IN_HOUR;

    }
    else
    {
         secs = dm*SECS_IN_DAY + h*SECS_IN_HOUR + m*SECS_IN_MIN + s;
         for (int i = 0; i < mon; i++)
         {
			 secs += secondsInMonth ((Time::Month)i);      // Will be incorrect if this is a leap year!
         }
    }
}

void Time::set (int h, int m, int s, int dw, int dm, int mon, int y)   // dw is ignored
{
    mon--;    // System treats January as 0
    if (mode == Absolute) {
        #if defined (OSX) || defined (ANDROID)
            secs = h*SECS_IN_HOUR + m*SECS_IN_MIN + s; /*!!*/ // Need to figure out how to handle timezone
        #else
            secs = h*SECS_IN_HOUR + m*SECS_IN_MIN + s + _timezone;
        #endif

        struct tm *ptm = localtime (&secs);
        while ((ptm->tm_year != y) || (ptm->tm_mon != mon) || (ptm->tm_mday != dm)) {
            if (ptm->tm_year > y) {
                secs -= SECS_IN_DAY * 365;
            }
            else if (ptm->tm_year < y) {
                secs += SECS_IN_DAY * 365;
            }
            else if (ptm->tm_mon > mon) {
                secs -= SECS_IN_DAY * 20;
            }
            else if (ptm->tm_mon < mon) {
                secs += SECS_IN_DAY * 20;
            }
            else if (ptm->tm_mday > dm) {
                secs -= SECS_IN_DAY;
            }
            else {
                secs += SECS_IN_DAY;
            }
            ptm = localtime (&secs);
        }
        secs += (h - ptm->tm_hour) * SECS_IN_HOUR;   // Adjust the hour again if needed
                                                     // to account for daylight savings time
    }
    else {
        secs = (time_t) (y*365.25*SECS_IN_DAY + dm*SECS_IN_DAY + h*SECS_IN_HOUR + m*SECS_IN_MIN + s);
        for (int i = 0; i < mon; i++) {
            secs += secondsInMonth ((Time::Month) i);   // Will be incorrect if this is a leap year!
        }
    }
}

time_t Time::secondsInMonth (Month mon, int iYear)
{
    switch (mon)
    {
         case January:
         case March:
         case May:
         case July:
         case August:
         case October:
         case December:
              return SECS_IN_DAY * 31;

         case April:
         case June:
         case September:
         case November:
              return SECS_IN_DAY * 30;

         case February:
              return iYear % 4 ? SECS_IN_DAY*28 : SECS_IN_DAY*29;
    }
    return 0;
}

int ATime::h12 (void)
{
    int h = localtime(&secs)->tm_hour;
    if (h > 12)
    {
         h -= 12;
    }
    if (h < 1)
    {
         h += 12;
    }
    return h;
}

void ATime::adjustHrs (int h)
{
    struct tm *t = localtime (&secs);
    secs += (h - t->tm_hour) * SECS_IN_HOUR;
}

void ATime::adjustMin (int m)
{
    struct tm *t = localtime (&secs);
    secs += (m - t->tm_min) * SECS_IN_MIN;
}

void ATime::adjustSec (int s)
{
    struct tm *t = localtime (&secs);
    secs += (s - t->tm_sec);
}

void ATime::adjustDayOfWeek (int dw)
{
    struct tm *t = localtime (&secs);
    secs += (dw - t->tm_wday) * SECS_IN_DAY;
}

void ATime::adjustDayOfMonth (int dm)
{
    struct tm *t = localtime (&secs);
    secs += (dm - t->tm_mday) * SECS_IN_DAY;
}

void ATime::adjustMonth (int mon)
{
    mon--;    // System treats January as 0
    struct tm *t = localtime (&secs);
    int dm = t->tm_mday;
    int h = t->tm_hour;
    if (dm > secondsInMonth ( (Time::Month)(mon+1))/SECS_IN_DAY)
    {
		dm = (int) (secondsInMonth ((Time::Month)(mon+1))/SECS_IN_DAY);
    }
    while ((t->tm_mon != mon) || (t->tm_mday != dm) || (t->tm_hour != h))
    {
         if (t->tm_mon > mon)
         {
              secs -= SECS_IN_DAY * 20;
         }
         else if (t->tm_mon < mon)
         {
              secs += SECS_IN_DAY * 20;
         }
         else if (t->tm_mday > dm)
         {
              secs -= SECS_IN_DAY;
         }
         else if (t->tm_mday < dm)
         {
              secs += SECS_IN_DAY;
         }
         else if (t->tm_hour > h)
         {
              secs -= SECS_IN_HOUR;
         }
         else
         {
              secs += SECS_IN_HOUR;
         }
         t = localtime (&secs);
    }
}

void ATime::adjustYear (int y)
{
    struct tm *t = localtime (&secs);
    int mon = t->tm_mon;
    int dm = t->tm_mday;
    int h = t->tm_hour;
    while ((t->tm_year != y) || (t->tm_mon != mon) ||
           (t->tm_mday != dm) || (t->tm_hour != h))
    {
         if (t->tm_year > y)
         {
              secs -= SECS_IN_DAY * 365;
         }
         else if (t->tm_year < y)
         {
              secs += SECS_IN_DAY * 365;
         }
         else if (t->tm_mon > mon)
         {
              secs -= SECS_IN_DAY * 20;
         }
         else if (t->tm_mon < mon)
         {
              secs += SECS_IN_DAY * 20;
         }
         else if (t->tm_mday > dm)
         {
              secs -= SECS_IN_DAY;
         }
         else if (t->tm_mday < dm)
         {
              secs += SECS_IN_DAY;
         }
         else if (t->tm_hour > h)
         {
              secs -= SECS_IN_HOUR;
         }
         else
         {
              secs += SECS_IN_HOUR;
         }
         t = localtime (&secs);
    }
}

int ATime::cmp (const ATime &rhsATime)
{
    Modulus cmpMod = mod < rhsATime.mod ? mod : rhsATime.mod;     // Compare with the smallest modulus
    struct tm t1 = *localtime (&secs);
    struct tm t2 = *localtime (&rhsATime.secs);
    switch (cmpMod)
    {
         case Mod_Day:
              return (t1.tm_hour-t2.tm_hour ? t1.tm_hour-t2.tm_hour :
                                              (t1.tm_min - t2.tm_min ? t1.tm_min - t2.tm_min :
                                                                       (t1.tm_sec - t2.tm_sec)));
         case Mod_Week:
              return (t1.tm_wday-t2.tm_wday ? t1.tm_wday-t2.tm_wday :
                                              (t1.tm_hour-t2.tm_hour ? t1.tm_hour-t2.tm_hour :
                                                                       (t1.tm_min - t2.tm_min ? t1.tm_min - t2.tm_min :
                                                                                                (t1.tm_sec - t2.tm_sec))));
         case Mod_Month:
              return (t1.tm_mday-t2.tm_mday ? t1.tm_mday-t2.tm_mday :
                                              (t1.tm_hour-t2.tm_hour ? t1.tm_hour-t2.tm_hour :
                                                                       (t1.tm_min - t2.tm_min ? t1.tm_min - t2.tm_min :
                                                                                                (t1.tm_sec - t2.tm_sec))));
         case Mod_Year:
              return (t1.tm_yday-t2.tm_yday ? t1.tm_yday-t2.tm_yday :
                                              (t1.tm_hour-t2.tm_hour ? t1.tm_hour-t2.tm_hour :
                                                                       (t1.tm_min - t2.tm_min ? t1.tm_min - t2.tm_min :
                                                                                                (t1.tm_sec - t2.tm_sec))));
         case Mod_None:
              if (secs > rhsATime.secs)
              {
                   return 1;
              }
              else if (secs < rhsATime.secs)
              {
                   return -1;
              }
              else
              {
                   return 0;
              }
    }
    return 0;
}

int DTime::h12 (void)
{
    int h = h24();
    if (h > 12)
    {
         h -= 12;
    }
    if (h < 1)
    {
         h += 12;
    }
    return h;
}

int DTime::h24 (void)
{
    time_t t = secs % SECS_IN_DAY;
    return (int) (t / SECS_IN_HOUR);
}

int DTime::min (void)
{
    time_t t = secs % SECS_IN_HOUR;
    return (int) (t / SECS_IN_MIN);
}

int DTime::sec (void)
{
    return (int) (secs % SECS_IN_MIN);
}

int DTime::cmp (const DTime &rhsDTime)
{
    Modulus cmpMod = mod < rhsDTime.mod ? mod : rhsDTime.mod;     // Compare with the smallest modulus
    time_t s;
    #if defined (OSX) || defined (ANDROID)
        s = secs;   /*!!*/ // Need to figure out how to handle timezone
    #else
        s = secs+_timezone;
    #endif
    struct tm t1 = *localtime (&s);
    #if defined (OSX) || defined (ANDROID)
        s = rhsDTime.secs;    /*!!*/ // Need to figure out how to handle timezone
    #else
        s = rhsDTime.secs+_timezone;
    #endif
    struct tm t2 = *localtime (&s);
    switch (cmpMod)
    {
         case Mod_Day:
              return (t1.tm_hour-t2.tm_hour ? t1.tm_hour-t2.tm_hour :
                                              (t1.tm_min - t2.tm_min ? t1.tm_min - t2.tm_min :
                                                                       (t1.tm_sec - t2.tm_sec)));
         case Mod_Week:
              return (t1.tm_wday-t2.tm_wday ? t1.tm_wday-t2.tm_wday :
                                              (t1.tm_hour-t2.tm_hour ? t1.tm_hour-t2.tm_hour :
                                                                       (t1.tm_min - t2.tm_min ? t1.tm_min - t2.tm_min :
                                                                                                (t1.tm_sec - t2.tm_sec))));
         case Mod_Month:
              return (t1.tm_mday-t2.tm_mday ? t1.tm_mday-t2.tm_mday :
                                              (t1.tm_hour-t2.tm_hour ? t1.tm_hour-t2.tm_hour :
                                                                       (t1.tm_min - t2.tm_min ? t1.tm_min - t2.tm_min :
                                                                                                (t1.tm_sec - t2.tm_sec))));
         case Mod_Year:
              return (t1.tm_yday-t2.tm_yday ? t1.tm_yday-t2.tm_yday :
                                              (t1.tm_hour-t2.tm_hour ? t1.tm_hour-t2.tm_hour :
                                                                       (t1.tm_min - t2.tm_min ? t1.tm_min - t2.tm_min :
                                                                                                (t1.tm_sec - t2.tm_sec))));
         case Mod_None:
              if (secs > rhsDTime.secs)
              {
                   return 1;
              }
              else if (secs < rhsDTime.secs)
              {
                   return -1;
              }
              else
              {
                   return 0;
              }
    }
    return 0;
}


