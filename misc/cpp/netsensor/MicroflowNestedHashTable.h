#ifndef NETSENSOR_MicroflowNestedHashTable__INCLUDED
#define NETSENSOR_MicroflowNestedHashTable__INCLUDED

/*
* MicroflowNestedHashTable.h
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
* This Class impelemnts a five levels hash map that will contain
* stats identified by source ip, dest ip, protocol, source port, dest port
*/
#include"StringHashtable.h"
#include"TrafficElement.h"
#include"MicroflowId.h"
#include"NLFLib.h"
#include"NetSensorConstants.h"
#include"traffic.pb.h"
#include"NetSensorUtilities.h"
#include <list>
namespace IHMC_NETSENSOR
{
class MicroflowNestedHashTable
{
public:
	MicroflowNestedHashTable (void);
	~MicroflowNestedHashTable (void);

	/*
	* ui32CleaningNumber : Maximum number of cleanings to perform
	*  Returns : Number of entries removed.
	*/
	// uint32 cleanTable (const uint32 ui32CleaningNumber);
	uint32 cleanTable2 (const uint32 ui32CleaningNumber);

	void fillTrafficProto (netsensor::TrafficByInterface* pT);

	TrafficElement* get(NOMADSUtil::String entryID);


	void put (NOMADSUtil::String entryID, TrafficElement* t);

	void print (void);

private:
	void divideByID		(std::map<std::string, std::list<StaticTrafficElement>> &m, TrafficElement *pEl);
	void fillFromList	(std::list<StaticTrafficElement> &list, netsensor::TrafficByInterface* pT);
	void fillAvg		(netsensor::Average* pAvg, StaticTrafficElement pEl);
	bool fillStat		(netsensor::Stat* pStat, StaticTrafficElement pEl);
	//sip,dip,prot,sp,dp
	NOMADSUtil::StringHashtable<TrafficElement> _microflowTable;
	NOMADSUtil::Mutex _pTMutex;
};
}
#endif
