/*
 * TClass.h
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

#ifndef INCL_TIME_CLASS_H
#define INCL_TIME_CLASS_H

#include <time.h>
#if defined (LINUX) || defined (SOLARIS)
    #include <sys/time.h>
    #define _timezone timezone
#endif

#define SECS_IN_MIN   60UL
#define SECS_IN_HOUR  3600UL
#define SECS_IN_DAY   86400UL
#define SECS_IN_WEEK  604800UL

#undef min // so that a macro named min doesn't overide the min
           // method in the class

// General Notes
//
// Ranges for data values:
//     Hours range from 0 to 23
//     Minutes range from 0 to 59
//     Seconds range from 0 to 59
//     dw ranges from 0 to 6 (with 0 being Sunday)
//     dm ranges from 1 to 31 (possibly 28, 29, or 30)
//     mon ranges from 1 to 12 (1 being January)
//     y ranges from 0 to ... (0 being 1970 for ATime)
//
// Specifying an invalid value while setting could result in an infinite loop
//     (For example, specifying February 30 as the date)

namespace NOMADSUtil
{

    class Time
    {
        public:
             enum Modulus {Mod_Day, Mod_Week, Mod_Month, Mod_Year, Mod_None};
             enum Mode {Absolute, Delta};
             enum Day {Sunday, Monday, Tuesday, Wednesday, Thursday, Friday, Saturday};
             enum Month {January=1, February, March, April, May, June, July, August,
                         September, October, November, December};
             void setModulus (Modulus m) {mod = m;}
             void set (void);
             void set (time_t t);
             void set (char *pszTime);
             void set (int h, int m, int s);
             void set (int h, int m, int s, int dw);
             void set (int h, int m, int s, int dw, int dm);                 // dw ignored
             void set (int h, int m, int s, int dw, int dm, int mon);        // dw ignored
             void set (int h, int m, int s, int dw, int dm, int mon, int y); // dw ignored
        protected:
             Time (Modulus mod, Mode mode);
             Time (Modulus mod, Mode mode, time_t t);
             Time (Modulus mod, Mode mode, char *pszTime);
             Time (Modulus mod, Mode mode, int h, int m, int s);
             Time (Modulus mod, Mode mode, int h, int m, int s, int dw);
             Time (Modulus mod, Mode mode, int h, int m, int s, int dw, int dm);
             Time (Modulus mod, Mode mode, int h, int m, int s, int dw, int dm, int mon);
             Time (Modulus mod, Mode mode, int h, int m, int s, int dw, int dm, int mon, int y);
             void changeMode (Mode m) {mode = m;}
             time_t secondsInMonth (Month mon, int iYear = 1); // iYear%4 is used to check for leap years
             Modulus mod;
             Mode mode;
             time_t secs;
    };

    class DTime;

    class ATime : public Time
    {
        public:
             ATime (const ATime &rhsATime);
             ATime (Modulus mod = Mod_None)
                  : Time (mod,Absolute) {;}
             ATime (time_t t, Modulus mod = Mod_None)
                  : Time (mod,Absolute,t) {;}
             ATime (char *pszTime, Modulus mod = Mod_Day)
                  : Time (mod,Absolute,pszTime) {;}
             ATime (int h, int m, int s, Modulus mod = Mod_Day)
                  : Time (mod,Absolute,h,m,s) {;}
             ATime (int h, int m, int s, int dw, Modulus mod = Mod_Week)
                  : Time (mod,Absolute,h,m,s,dw) {;}
             ATime (int h, int m, int s, int dw, int dm, Modulus mod = Mod_Month)                // dw ignored
                  : Time (mod,Absolute,h,m,s,dw,dm) {;}
             ATime (int h, int m, int s, int dw, int dm, int mon, Modulus mod = Mod_Year)        // dw ignored
                  : Time (mod,Absolute,h,m,s,dw,dm,mon) {;}
             ATime (int h, int m, int s, int dw, int dm, int mon, int y, Modulus mod = Mod_None) // dw ignored
                  : Time (mod,Absolute,h,m,s,dw,dm,mon,y) {;}
             int h12 (void);
             int h24 (void) {return localtime(&secs)->tm_hour;}
             int min (void) {return localtime(&secs)->tm_min;}
             int sec (void) {return localtime(&secs)->tm_sec;}
             int dayOfWeek (void) {return localtime(&secs)->tm_wday;}
             int dayOfMonth (void) {return localtime(&secs)->tm_mday;}
             int month (void) {return localtime(&secs)->tm_mon+1;}
             int year (void) {return localtime(&secs)->tm_year+1900;}
             char * ctime (void) {return ::ctime (&secs);}
             void adjustHrs (int h);
             void adjustMin (int m);
             void adjustSec (int s);
             void adjustDayOfWeek (int dw);
             void adjustDayOfMonth (int dm);
             void adjustMonth (int mon);
             void adjustYear (int y);
             void incDay (void) {secs += SECS_IN_DAY;}
             void decDay (void) {secs -= SECS_IN_DAY;}
             void incWeek (void) {secs += SECS_IN_DAY*7;}
             void decWeek (void) {secs -= SECS_IN_DAY*7;}
             void incMonth (void) {adjustMonth (month()+1);}
             void decMonth (void) {adjustMonth (month()-1);}
             void incYear (void) {adjustYear (year()+1);}
             void decYear (void) {adjustYear (year()-1);}
             int cmp (const ATime &rhsATime);
             ATime & operator = (const time_t t) {secs = t; return *this;}
             ATime & operator = (const ATime &rhsATime) {secs = rhsATime.secs; mod = rhsATime.mod; return *this;}
             ATime & operator = (const DTime &rhsDTime);
             ATime operator + (const time_t t) {return ATime(secs+t,mod);}
             ATime operator + (const DTime &rhsDTime);
             ATime operator - (const time_t t) {return ATime(secs-t,mod);}
             ATime operator - (const DTime &rhsDTime);
             DTime operator - (const ATime &rhsATime);
             ATime & operator += (const time_t t) {secs+=t; return *this;}
             ATime & operator += (const DTime &rhsDTime);
             ATime & operator -= (const time_t t) {secs-=t; return *this;}
             ATime & operator -= (const DTime &rhsDTime);
             int operator == (const ATime &rhsATime) {return cmp(rhsATime)==0;}
             int operator != (const ATime &rhsATime) {return cmp(rhsATime)!=0;}
             int operator < (const ATime &rhsATime) {return cmp(rhsATime)<0;}
             int operator <= (const ATime &rhsATime) {return cmp(rhsATime)<=0;}
             int operator > (const ATime &rhsATime) {return cmp(rhsATime)>0;}
             int operator >= (const ATime &rhsATime) {return cmp(rhsATime)>=0;}
             friend class DTime;
    };

    class DTime : public Time
    {
        public:
             DTime (const DTime &rhsDTime);
             DTime (Modulus mod = Mod_None)
                  : Time (mod,Delta) {;}
             DTime (time_t t, Modulus mod = Mod_None)
                  : Time (mod,Delta,t) {;}
             DTime (char *pszTime, Modulus mod = Mod_Day)
                  : Time (mod,Delta,pszTime) {;}
             DTime (int h, int m, int s, Modulus mod = Mod_Day)
                  : Time (mod,Delta,h,m,s) {;}
             DTime (int h, int m, int s, int dw, Modulus mod = Mod_Week)
                  : Time (mod,Delta,h,m,s,dw) {;}
             DTime (int h, int m, int s, int dw, int dm, Modulus mod = Mod_Month)                // dw ignored
                  : Time (mod,Delta,h,m,s,dw,dm) {;}
             DTime (int h, int m, int s, int dw, int dm, int mon, Modulus mod = Mod_Year)        // dw ignored
                  : Time (mod,Delta,h,m,s,dw,dm,mon) {;}
             DTime (int h, int m, int s, int dw, int dm, int mon, int y, Modulus mod = Mod_None) // dw ignored
                  : Time (mod,Delta,h,m,s,dw,dm,mon,y) {;}
             int h12 (void);
             int h24 (void);
             int min (void);
             int sec (void);
             time_t inSecs (void) {return secs;}
             long inMins (void) {return (long) (secs/SECS_IN_MIN);}
             long inHours (void) {return (long) (secs/SECS_IN_HOUR);}
             long inDays (void) {return (long) (secs/SECS_IN_DAY);}
             long inWeeks (void) {return (long) (secs/SECS_IN_WEEK);}
             int cmp (const DTime &rhsDTime);
             DTime & operator = (const time_t t) {secs=t; return *this;}
             DTime & operator = (const ATime &rhsATime);
             DTime & operator = (const DTime &rhsDTime) {secs=rhsDTime.secs;mod=rhsDTime.mod;return *this;}
             DTime operator + (const time_t t) {return DTime(secs+t,mod);}
             DTime operator + (const DTime &rhsDTime) {return DTime(secs+rhsDTime.secs,mod);}
             ATime operator + (const ATime &rhsATime) {return ATime(secs+rhsATime.secs,mod);}
             DTime operator - (const time_t t) {return DTime(secs-t,mod);}
             DTime operator - (const DTime &rhsDTime) {return DTime(secs-rhsDTime.secs,mod);}
             DTime & operator += (const time_t t) {secs+=t;return *this;}
             DTime & operator += (const DTime &rhsDTime) {secs+=rhsDTime.secs;return *this;}
             DTime & operator -= (const time_t t) {secs-=t;return *this;}
             DTime & operator -= (const DTime &rhsDTime) {secs-=rhsDTime.secs;return *this;}
             int operator == (const DTime &rhsDTime) {return cmp(rhsDTime)==0;}
             int operator != (const DTime &rhsDTime) {return cmp(rhsDTime)!=0;}
             int operator < (const DTime &rhsDTime) {return cmp(rhsDTime)<0;}
             int operator <= (const DTime &rhsDTime) {return cmp(rhsDTime)<=0;}
             int operator > (const DTime &rhsDTime) {return cmp(rhsDTime)>0;}
             int operator >= (const DTime &rhsDTime) {return cmp(rhsDTime)>=0;}
             friend class ATime;
    };

    inline ATime::ATime (const ATime &rhsATime)
        : Time (rhsATime.mod, rhsATime.mode)
    {
        secs = rhsATime.secs;
    }

    inline ATime & ATime::operator = (const DTime &rhsDTime)
    {
        #if defined (OSX) || defined (ANDROID)
            secs = rhsDTime.secs;    /*!!*/ // Need to figure out how to handle timezone
        #else
            secs = rhsDTime.secs + _timezone;
        #endif
        mod = rhsDTime.mod;
        return *this;
    }

    inline ATime ATime::operator + (const DTime &rhsDTime)
    {
        return ATime(secs+rhsDTime.secs,mod);
    }

    inline ATime ATime::operator - (const DTime &rhsDTime)
    {
        return ATime(secs-rhsDTime.secs,mod);
    }

    inline DTime ATime::operator - (const ATime &rhsATime)
    {
        return DTime(secs-rhsATime.secs,mod);
    }

    inline ATime & ATime::operator += (const DTime &rhsDTime)
    {
        secs+=rhsDTime.secs;
        return *this;
    }

    inline ATime & ATime::operator -= (const DTime &rhsDTime)
    {
        secs-=rhsDTime.secs;
        return *this;
    }

    inline DTime::DTime (const DTime &rhsDTime)
        : Time (rhsDTime.mod, rhsDTime.mode)
    {
        secs = rhsDTime.secs;
    }

    inline DTime & DTime::operator = (const ATime &rhsATime)
    {
        #if defined (OSX) || defined (ANDROID)
            secs = rhsATime.secs;   /*!!*/ // Need to figure out how to handle timezone
        #else
            secs = rhsATime.secs - _timezone;
        #endif
        mod = rhsATime.mod;
        return *this;
    }

}

#endif   // #ifndef INCL_TCLASS_H


