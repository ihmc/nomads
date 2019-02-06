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

#include "MicroflowNestedHashTable.h"
#include <forward_list>
#include <tuple>
#include "PtrLList.h"
#include <list>

using namespace netsensor;
using namespace NOMADSUtil;
namespace IHMC_NETSENSOR
{
MicroflowNestedHashTable::MicroflowNestedHashTable()
	: _microflowTable (true, true, true, true)
{

}

MicroflowNestedHashTable::~MicroflowNestedHashTable (void)
{

}

uint32 MicroflowNestedHashTable::cleanTable2 (const uint32 ui32CleaningNumber) 
{
	std::forward_list <String> list;
	bool bKeepCleaning = true;
	uint32 cleaningCounter = 0;
	if ((_pTMutex.lock() == Mutex::RC_Ok)) {
		int64 currentTime = getTimeInMilliseconds();
		for (auto i = _microflowTable.getAllElements(); (!i.end() && bKeepCleaning); i.nextElement()) {
			auto tEl = i.getValue();
			tEl->tiaPackets.expireOldEntries();
			tEl->tiaTraffic.expireOldEntries();
			int64 expiredT = tEl->i64TimeOfLastChange + C_ENTRY_TIME_VALIDITY;
			if (currentTime > expiredT || ((tEl->tiaTraffic.getAverage() == 0))) {
				list.emplace_front (i.getKey());
				cleaningCounter++;
				bKeepCleaning = cleaningCounter < ui32CleaningNumber;
			}
		}
		for (auto iter = list.begin(); iter != list.end(); iter++) {
			delete _microflowTable.remove (*iter);
		}
		_pTMutex.unlock();
		return cleaningCounter;
	}
	return cleaningCounter;
}

/*
uint32 MicroflowNestedHashTable::cleanTable (const uint32 ui32CleaningNumber)
{
	std::forward_list <std::tuple <String, String, String, String, String>> list;
	bool bKeepCleaning = true;

	uint32 cleaningCounter = 0;

	if ((_pTMutex.lock() == Mutex::RC_Ok)) {
		int64 currentTime = getTimeInMilliseconds();

		for (auto v = _microflowNestedHashTable.getAllElements(); (!v.end() && bKeepCleaning); v.nextElement()) {
			auto l4 = v.getValue();

			for (auto iv = l4->getAllElements(); (!iv.end() && bKeepCleaning); iv.nextElement()) {
				auto l3 = iv.getValue();

				for (auto iii = l3->getAllElements(); (!iii.end() && bKeepCleaning); iii.nextElement()) {
					auto l2 = iii.getValue();

					for (auto ii = l2->getAllElements(); (!ii.end() && bKeepCleaning); ii.nextElement()) {
						auto l1 = ii.getValue();

						for (auto i = l1->getAllElements(); (!i.end() && bKeepCleaning); i.nextElement()) {
							TrafficElement* tel = i.getValue();
							tel->tiaPackets.expireOldEntries();
							tel->tiaTraffic.expireOldEntries();

							int64 expiredT = tel->i64TimeOfLastChange +
								C_ENTRY_TIME_VALIDITY;

							if (currentTime > expiredT ||
								((tel->tiaTraffic.getAverage() == 0))) {
								list.emplace_front(i.getKey(), ii.getKey(), iii.getKey(), iv.getKey(), v.getKey());
								cleaningCounter++;
								(cleaningCounter < ui32CleaningNumber) ?
									bKeepCleaning = true :
									bKeepCleaning = false;
							}
						}
					}
				}
			}
		}
		for (auto iter = list.begin(); iter != list.end(); iter++) {
			auto  iv = _microflowNestedHashTable.get(std::get<4>(*iter));
			auto iii = iv->get(std::get<3>(*iter));
			auto   ii = iii->get(std::get<2>(*iter));
			auto   i = ii->get(std::get<1>(*iter));

			delete i->remove(std::get<0>(*iter));
			if (i->getCount() == 0) {
				delete ii->remove(std::get<1>(*iter));
				if (ii->getCount() == 0) {
					delete iii->remove(std::get<2>(*iter));
					if (iii->getCount() == 0) {
						delete iv->remove(std::get<3>(*iter));
						if (iv->getCount() == 0) {
							delete _microflowNestedHashTable.remove(std::get<4>(*iter));
						}
					}
				}
			}
		}
		_pTMutex.unlock();
		return cleaningCounter;
	}
}

void MicroflowNestedHashTable::fillTrafficProto (TrafficByInterface* pT)
{
	if ((_pTMutex.lock() == Mutex::RC_Ok)) {
		for (auto v = _microflowNestedHashTable.getAllElements(); !v.end(); v.nextElement()) {

			uint32 ui32SrcAddr = InetAddr(v.getKey()).getIPAddress();

			ui32SrcAddr = convertIpToHostOrder(ui32SrcAddr);

			auto l4 = v.getValue();
			for (auto iv = l4->getAllElements(); !iv.end(); iv.nextElement()) {
				uint32 ui32DstAddr = InetAddr(iv.getKey()).getIPAddress();

				ui32DstAddr = convertIpToHostOrder(ui32DstAddr);

				bool bAddIt = false;
				Microflow *pMw = new Microflow();

				//Microflow *pMw = pT->add_microflows();

				pMw->set_ipsrc(ui32SrcAddr);
				pMw->set_ipdst(ui32DstAddr);
				auto l3 = iv.getValue();

				for (auto iii = l3->getAllElements(); !iii.end(); iii.nextElement()) {
					auto l2 = iii.getValue();

					for (auto ii = l2->getAllElements(); !ii.end(); ii.nextElement()) {
						auto l1 = ii.getValue();

						for (auto i = l1->getAllElements(); !i.end(); i.nextElement()) {
							TrafficElement* tel = i.getValue();
							Stat *st = pMw->add_stats();
							st->set_stattype(TRAFFIC_AVERAGE);
							st->set_protocol(iii.getKey());
							char*end;
							uint32 ui32SPort = strtol(ii.getKey(), &end, 10);
							uint32 ui32dPort = strtol(i.getKey(), &end, 10);
							st->set_srcport(ui32SPort);
							st->set_dstport(ui32dPort);
							TrafficElement *te = i.getValue();

							if (te->tiaTraffic.getAverage() > 0) {
								Average *avg = st->add_averages();
								bAddIt = true;
								switch (te->classification)
								{
								case TrafficElement::SNT:
									avg->set_sent(te->tiaTraffic.getAverage());
									break;
								case TrafficElement::RCV:
									avg->set_received(te->tiaTraffic.getAverage());
									break;
								case TrafficElement::OBS:
									avg->set_observed(te->tiaTraffic.getAverage());
									break;
								}
								avg->set_resolution(te->resolution);
							}
						}
					}
				}

				if (bAddIt) {
					Microflow *pNewM = pT->add_microflows();
					pNewM->CopyFrom(*pMw);
				}
				delete pMw;
			}
		}
		_pTMutex.unlock();
	}
}
*/

void MicroflowNestedHashTable::divideByID (std::map<std::string, std::list<StaticTrafficElement>> &m, TrafficElement *pEl) 
{
	//String s = value->srcAddr + value->dstAddr;
	auto s = pEl->srcAddr + pEl->dstAddr;
	if (m.find(s.c_str()) == m.end()) {
		m[s.c_str()] = std::list<StaticTrafficElement>();
	}
	StaticTrafficElement el (*pEl);
	m[s.c_str()].push_back (el);
}

void MicroflowNestedHashTable::fillAvg (netsensor::Average* pAvg, StaticTrafficElement el)
{
	switch (el.classification)
	{
	case TrafficElement::SNT:
		pAvg->set_sent (el.avg);
		break;
	case TrafficElement::RCV:
		pAvg->set_received (el.avg);
		break;
	case TrafficElement::OBS:
		pAvg->set_observed (el.avg);
		break;
	}
	pAvg->set_resolution (el.resolution);
}

bool MicroflowNestedHashTable::fillStat (netsensor::Stat* pStat, StaticTrafficElement el)
{
	bool filled = false;
	if (el.avg > 0) {
		filled = true;
		pStat->set_stattype (TRAFFIC_AVERAGE);
		pStat->set_protocol (el.protocol);
		char* end;
		pStat->set_srcport (strtol (el.srcPort, &end, 10));
		pStat->set_dstport (strtol (el.dstPort, &end, 10));
		fillAvg (pStat->add_averages(), el);
	}
	return filled;
}

void MicroflowNestedHashTable::fillFromList (std::list<StaticTrafficElement> &list, TrafficByInterface* pT)
{
	bool bAddIt = false;
	bool init = false;
	auto pMw = new Microflow();
	for (auto pTrafficElement : list) {
		uint32 ui32SrcAddr = getIpInHostOrder (pTrafficElement.srcAddr);
		uint32 ui32DstAddr = getIpInHostOrder (pTrafficElement.dstAddr);

		if (!init) {
			init = true;
			pMw->set_ipsrc (ui32SrcAddr);
			pMw->set_ipdst (ui32DstAddr);
		}

		if (fillStat (pMw->add_stats(), pTrafficElement)) {
			bAddIt = true;
		}
	}

	if (bAddIt) {
		pT->add_microflows()->CopyFrom (*pMw);
	}
	delete pMw;
}

void MicroflowNestedHashTable::fillTrafficProto (TrafficByInterface* pT)
{
	std::map<std::string, std::list<StaticTrafficElement>> m;
	if ((_pTMutex.lock() == Mutex::RC_Ok)) {
		for (auto i = _microflowTable.getAllElements(); !i.end(); i.nextElement()) {	
			divideByID (m, i.getValue());
		}	
		_pTMutex.unlock();
		bool init = false;
		for (auto i : m) {
			fillFromList (i.second, pT);
		}
	}
	m.clear();
}

TrafficElement* MicroflowNestedHashTable::get (String entryID)
{
	TrafficElement* tEl = nullptr;
	if ((_pTMutex.lock() == Mutex::RC_Ok)) {
		tEl = _microflowTable.get(entryID);
	}
	_pTMutex.unlock();
	return tEl;
}

/*
TrafficElement* MicroflowNestedHashTable::get (
	const char* sSA,
	const char* sDA,
	const char* sProtocol,
	const char* sSP,
	const char* sDP)
{
	if ((_pTMutex.lock() == Mutex::RC_Ok)) {
		auto l4 = _microflowNestedHashTable.get(sSA);
		if (l4 != NULL) {
			auto l3 = l4->get(sDA);
			if (l3 != NULL) {
				auto l2 = l3->get(sProtocol);
				if (l2 != NULL) {
					auto l1 = l2->get(sSP);
					if (l1 != NULL) {
						_pTMutex.unlock();
						return l1->get(sDP);
					}
				}
			}
		}
		_pTMutex.unlock();
	}
	return NULL;
}
*/

void MicroflowNestedHashTable::put (String entryID, TrafficElement* t)
{
	TrafficElement* tEl = nullptr;
	if ((_pTMutex.lock() == Mutex::RC_Ok)) {
		tEl = _microflowTable.get (entryID);
		if (tEl == nullptr) {
			_microflowTable.put (entryID, t);
		}
		else {
			_microflowTable.remove (entryID);
			_microflowTable.put (entryID, t);
		}
		_pTMutex.unlock();
	}
}

void MicroflowNestedHashTable::print (void)
{
	int64 i64CurTime = getTimeInMilliseconds();
	if ((_pTMutex.lock() == Mutex::RC_Ok)) {
		for (auto i = _microflowTable.getAllElements(); !i.end(); i.nextElement()) {
			auto tel = i.getValue();
			printf ("%43s: %.1f, last update: %llums\n",
				i.getKey(),
				tel->tiaTraffic.getAverage(), 
				i64CurTime - tel->i64TimeOfLastChange);
		}
	}
	_pTMutex.unlock();
}

}