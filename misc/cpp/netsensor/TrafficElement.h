#ifndef NETSENSOR_TrafficElement__INCLUDED
#define NETSENSOR_TrafficElement__INCLUDED
/*
* TrafficElement.h
* Author: rfronteddu@ihmc.us
* This file is part of the IHMC NetSensor Library/Component
* Copyright (c) 2010-2017 IHMC.
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
*
* This class represent a stat linked to a mflow
*/

#include "TimeIntervalAverage.h"
#include "StrClass.h"
namespace IHMC_NETSENSOR
{

class TrafficElement
{
public:
	enum Classification { SNT, RCV, OBS };
    TrafficElement (uint32 uint32msResolution);
    ~TrafficElement (void);

//<--------------------------------------------------------------------------->  
    TimeIntervalAverage<uint32> tiaTraffic;
    TimeIntervalAverage<uint32> tiaPackets;
	NOMADSUtil::String srcAddr, dstAddr, protocol, srcPort, dstPort;
    uint32 resolution;
    int64 i64TimeOfLastChange;
    Classification classification;
};

class StaticTrafficElement
{
public:
	StaticTrafficElement (TrafficElement &trafficElement);
	StaticTrafficElement (const StaticTrafficElement &other);

	NOMADSUtil::String srcAddr, dstAddr, protocol, srcPort, dstPort;
	double avg;
	uint32 resolution;
	int64 i64TimeOfLastChange;
	TrafficElement::Classification classification;
};

inline StaticTrafficElement::StaticTrafficElement (TrafficElement &trafficElement)
	:
	srcAddr		(trafficElement.srcAddr),
	dstAddr		(trafficElement.dstAddr),
	protocol	(trafficElement.protocol),
	srcPort		(trafficElement.srcPort),
	dstPort		(trafficElement.dstPort),
	resolution	(trafficElement.resolution),
	i64TimeOfLastChange (trafficElement.i64TimeOfLastChange),
	classification		(trafficElement.classification),
	avg					(trafficElement.tiaTraffic.getAverage())
{
}

inline StaticTrafficElement::StaticTrafficElement (const StaticTrafficElement &other)
{
	srcAddr				= other.srcAddr;
	dstAddr				= other.dstAddr;
	protocol			= other.protocol;
	srcPort				= other.srcPort;
	dstPort				= other.dstPort;
	resolution			= other.resolution;
	i64TimeOfLastChange = other.i64TimeOfLastChange;
	classification		= other.classification;
	avg					= other.avg;
}




inline TrafficElement::TrafficElement (uint32 uint32msResolution)
    : 
    tiaTraffic (uint32msResolution), 
    tiaPackets (uint32msResolution), 
    resolution (uint32msResolution),
    i64TimeOfLastChange (0),
	srcAddr (""),
	dstAddr (""),
	protocol (""),
	srcPort (""),
	dstPort (""),
    classification (SNT) { }

inline TrafficElement::~TrafficElement (void) { }

}
#endif