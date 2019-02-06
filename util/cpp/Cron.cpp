/*
 * Cron.cpp
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

#include <stdio.h>
#include "Cron.h"
#include "NLFLib.h"

using namespace NOMADSUtil;

/**
 * Constructor
 **/
Cron::Cron ()
{
   _pEventList = new PtrLList<Cron::Event>;
   _currentId = 0;

}

/**
 * Deconstructor
 */
Cron::~Cron (void)
{
}

/**
 * The run method. Sits in a while loop checking to
 * see if any of the events in the list are at a time
 * when they should fire
 */
int Cron::run (void)
{
    while (1) {
        Event *pEvent; // temporary event
        PtrLList<Cron::Event> *pEventList = (PtrLList<Cron::Event>*) _pEventList;
        pEventList->resetGet(); // start at the beginning of the list

        // Go through the list
        while ((pEvent = pEventList->getNext()) != NULL) {
            ATime currentTime; // get the current time
            // check to see if the event should happen at this time
            if (pEvent->getStartTime() == currentTime) {

                // if the event should happen at this time, check to see whether
                // the event should happen on this day
                int dayOfMonth = currentTime.dayOfMonth();
                int dayOfWeek = currentTime.dayOfWeek();

                if ((pEvent->getDaysOfMonth() & ((0x00000001UL) << (dayOfMonth - 1))) > 0) {
                    if ((pEvent->getDaysOfWeek() & ((0x01) << (dayOfWeek - 1))) > 0) {
                        (*pEvent->getEventCallback()) (pEvent); // call the function
                        // check the count
                        if (pEvent->getCurrentCount() == pEvent->getCount()) {
                            deleteEvent (pEvent->_id);
                        } else {
                            pEvent->incrCount();
                        }
                        // update the time
                        ATime tmpTime = pEvent->getStartTime() + pEvent->getPeriod();
                        pEvent->setTime(tmpTime);
                    }
                }
            }
        }

        sleepForMilliseconds (60000);

    }
}

/**
 * Add an event.
 */
int Cron::addEvent (Event *e)
{
    e->_id = getId ();
    PtrLList<Cron::Event> *pEventList = (PtrLList<Cron::Event>*) _pEventList;
    pEventList->append (e);
    return e->_id;
}

/**
 * Delete an event, given the eventId
 */
int Cron::deleteEvent (int iEventId)
{
    Event *pEvent;
    PtrLList<Cron::Event> *pEventList = (PtrLList<Cron::Event>*) _pEventList;
    pEventList->resetGet();
    while ((pEvent = pEventList->getNext()) != NULL) {
        if (pEvent->_id == iEventId) {
            pEventList->remove (pEvent);
            return 1;
        }
    }
    return 0;
}

/**
 * Get a unique event id.
 */
int Cron::getId (void)
{
    int nowId = _currentId;
    _currentId++;
    return nowId;
}

// Methods for running Cron as a thread

void Cron::callRunFunction (void *pArg)
{
    Cron *pThis = (Cron*) pArg;
    pThis->run();
}

int Cron::start (void)
{
    return _ost.start (Cron::callRunFunction, this);
}


// Methods for the inner class event

/**
 * Constructor
 */
Cron::Event::Event (String name, ATime startTime, DTime period, int count, EventCallback ec, void *pArgs)
{
   _name = name;
   _startTime = startTime;
   _period = period;
   _ec = ec;
   _count = count;
   _currentCount = 1;
   _uchDaysOfWeek = 0xFF ;
   _ulDaysOfMonth = 0xFFFFFFFFUL;
   _pArgs = pArgs;
}

/**
 * Constructor
 */
Cron::Event::Event (String name,
                    ATime startTime,
                    DTime period,
                    int count,
                    unsigned char uchDaysOfWeek,
                    unsigned long ulDaysOfMonth,
                    EventCallback ec,
                    void *pArgs)
{
   _name = name;
   _startTime = startTime;
   _period = period;
   _ec = ec;
   _count = count;
   _currentCount = 1;
   _uchDaysOfWeek = uchDaysOfWeek;
   _ulDaysOfMonth = ulDaysOfMonth;
   _pArgs = pArgs;

   //_ulDaysOfMonth = _ulDaysOfMonth | (0x00000001UL << (day - 1))
}

/**
 * Get the name of the event
 */
String Cron::Event::getName (void)
{
    return _name;
}

/**
 * Get the time when this event should fire
 */
ATime Cron::Event::getStartTime (void)
{
    return _startTime;
}

/**
 * Get the number of times an event should fire
 */
int Cron::Event::getCount (void)
{
    return _count;
}

/**
 * Increment the current count by 1
 */
void Cron::Event::incrCount (void)
{
    _currentCount++;
}

/**
 * Get the number of times the event has fired
 */
int Cron::Event::getCurrentCount (void)
{
    return _currentCount;
}

/**
 * Set the time when the event should fire
 */
void Cron::Event::setTime (ATime time)
{
    _startTime = time;
}

/**
 * Get the time between firing events
 */
DTime Cron::Event::getPeriod (void)
{
    return _period;
}

/**
 * Get the days of the month when
 * the the event should fire
 */
unsigned long Cron::Event::getDaysOfMonth (void)
{
    return _ulDaysOfMonth;
}

/**
 * Get the days of the week that the event
 * should fire
 */
unsigned char Cron::Event::getDaysOfWeek (void)
{
    return _uchDaysOfWeek;
}

void * Cron::Event::getArguments (void)
{
    return _pArgs;
}

EventCallback Cron::Event::getEventCallback (void)
{
    return _ec;
}

bool Cron::Event::operator == (Cron::Event &evt)
{
    if (this->_id == evt._id) {
        return true;
    }

    return false;
}

