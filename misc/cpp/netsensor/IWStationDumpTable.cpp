
#include "IWStationDumpTable.h"
#include "StringHashtable.h"

using namespace NOMADSUtil;
using namespace IHMC_NETSENSOR;


IWStationDumpTable::IWStationDumpTable(){}
IWStationDumpTable::~IWStationDumpTable(){}

int IWStationDumpTable::put(IWStationDump *pDump)
{
	_mutex.lock();
	_dumpsTable.put(pDump->sMacAddr.c_str(),pDump);
	_mutex.unlock();
	return 0;
}

PtrLList<IWStationDump> * IWStationDumpTable::getList()
{
	_mutex.lock();
	//_dumpsTable.getAllElement();
	_mutex.unlock();
}
