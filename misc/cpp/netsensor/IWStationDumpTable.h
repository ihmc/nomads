#ifndef IW_STATION_DUMP_TABLE
#define IW_STATION_DUMP_TABLE
#include "StringHashtable.h"
#include "PtrLList.h"
#include "IWStationDump.h"
#include "PtrLList.h"
#include "Mutex.h"
#include "FTypes.h"

namespace IHMC_NETSENSOR
{
	class IWStationDumpTable
	{

	public:
		IWStationDumpTable();
		~IWStationDumpTable();

		int put(IWStationDump *pDump);
		NOMADSUtil::PtrLList<IWStationDump> * getList();



	private:
		NOMADSUtil::StringHashtable<IWStationDump> _dumpsTable;
        NOMADSUtil::Mutex _mutex;
	};
}
#endif
