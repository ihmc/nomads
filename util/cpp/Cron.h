/*
 * Cron.h
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

#ifndef INCL_CRON_H
#define INCL_CRON_H

#include "StrClass.h"
#include "TClass.h"
#include "PtrLList.h"
#include "OSThread.h"

namespace NOMADSUtil
{

    typedef void (*EventCallback) (void *pEvent);

    /**
     * A Timer class with some cron like functinality
     */
    class Cron
    {
        public:
            Cron (void);
            ~Cron (void);

            class Event {
                public:
                    // Constructors
                    Event (String name, ATime startTime, DTime period, int count, EventCallback ec, void *pArgs);
                    Event (String name, ATime startTime, DTime period, int count, unsigned char uchDaysOfWeek, unsigned long ulDaysOfMonth, EventCallback ec, void *pArgs);
                    String getName (void);               // Get the name of the event
                    ATime getStartTime (void);           // Get the time when the event should fire
                    DTime getPeriod (void);              // Get the interval between when the event should fire
                    int getCount (void);                 // Get the number of times the event should fire
                    int getCurrentCount (void);          // Get the number of times the event has firec
                    void setTime (ATime time);           // Set the time when the event should fire
                    void incrCount (void);               // Increment the current count by 1
                    bool operator == (Event &event);     // See if two events are eqaul
                    unsigned char getDaysOfWeek (void);  // Get the days of the week the event should fire
                    unsigned long getDaysOfMonth (void); // Get the days of the month when the event should fire
                    void * getArguments (void);
                    EventCallback getEventCallback (void);
                    int _id;
                protected:
                    EventCallback _ec;
                    void *_pArgs;
                    String _name;
                    ATime _startTime;
                    int _count;                      // 0 means that this event should always occur
                    DTime _period;                     
                    unsigned char _uchDaysOfWeek;    // LSB == Sunday, MSB-1 == Saturday
                    unsigned long _ulDaysOfMonth;    // LSB == 1st
                    int _currentCount;
                    
             };

            int run (void);
            int start (void);
            static void callRunFunction (void *pArg);

            // Return the event id if the event was successfully added or a
            // negative value in case of error
            int addEvent (Event *e);

            int deleteEvent (int iEventId);
            
        private:
            int getId (void);
            int _currentId;
            void *_pEventList;
            OSThread _ost;

    };

}
 
#endif
