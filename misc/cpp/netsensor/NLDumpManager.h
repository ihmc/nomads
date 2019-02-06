#ifdef IW 
#ifndef NL_DUMP_MANAGER
#define NE_DUMP_MANAGER
#include "FTypes.h"
#include "IWStationDump.h"
#include "IWStationDumpTable.h"

struct nl_msg;

namespace IHMC_NETSENSOR
{
	class NLDumpManager
	{
	public:
		NLDumpManager();
		~NLDumpManager();
		static int stationDumpCallback(struct nl_msg *pMsg, void *arg);
		static int debug(struct nl_msg *pMsg, void *arg);
	};
}
#endif
#endif