//ifdef linux
#ifndef IW_STATION_DUMP
#define IW_STATION_DUMP
#include <string>
#include "FTypes.h"
namespace IHMC_NETSENSOR
{
    class IWStationDump
    {
    public:
        IWStationDump();
        IWStationDump (const IWStationDump& dump);
        ~IWStationDump();
        void print();
        std::string sMacAddr;
        std::string sDevName;
        std::string sTxBitrate;
        std::string sLinkState;
        std::string sPeerPowerMode;
        std::string sLocalPowerMode;
        std::string sPreamble;
        std::string sInterface;
        uint32 ui32InactiveTime;
        uint32 ui32RxBytes;
        uint32 ui32RxPackets;
        uint32 ui32TxBytes;
        uint32 ui32TxPackets;
        uint32 ui32TxRetries;
        uint32 ui32TxFailed;
        uint8 ui8SignalPower;
        uint8 ui8SignalPowerAvg;
        uint64 ui64TOffset;
        int32 meshLLID;
        int32 meshPLID;
        bool bAuthenticated;
        bool bAuthorized;
        bool WMM_WME;
        bool MFP;
        bool TDLS;
    };
}


#endif