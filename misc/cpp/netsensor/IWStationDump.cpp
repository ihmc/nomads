#include "IWStationDump.h"

using namespace IHMC_NETSENSOR;
IWStationDump::IWStationDump(){}

IWStationDump::IWStationDump (const IWStationDump& dump):
sMacAddr (dump.sMacAddr),
sDevName (dump.sDevName),
sTxBitrate (dump.sTxBitrate),
sLinkState (dump.sLinkState),
sPeerPowerMode (dump.sPeerPowerMode),
sLocalPowerMode (dump.sLocalPowerMode),
sPreamble (dump.sPreamble),
sInterface (dump.sInterface),
ui32InactiveTime (dump.ui32InactiveTime),
ui32RxBytes (dump.ui32RxBytes),
ui32RxPackets (dump.ui32RxPackets),
ui32TxBytes (dump.ui32TxBytes),
ui32TxPackets (dump.ui32TxPackets),
ui32TxRetries (dump.ui32TxRetries),
ui32TxFailed (dump.ui32TxFailed),
ui8SignalPower (dump.ui8SignalPower),
ui8SignalPowerAvg (dump.ui8SignalPowerAvg),
ui64TOffset (dump.ui64TOffset),
meshLLID (dump.meshLLID),
meshPLID (dump.meshPLID),
bAuthenticated (dump.bAuthenticated),
bAuthorized (dump.bAuthorized),
WMM_WME (dump.WMM_WME),
MFP (dump.MFP),
TDLS (dump.TDLS)
{   
}
IWStationDump::~IWStationDump(){}

void IWStationDump::print() {
    printf ("Peer Mac Addr = %s\n", sMacAddr.c_str());
    printf ("Device Name = %s\n", sDevName.c_str());
    printf ("Tx Bitrate = %s\n", sTxBitrate.c_str());
    printf ("Link State = %s\n", sLinkState.c_str());
    printf ("Peer Power Mode = %s\n", sPeerPowerMode.c_str());
    printf ("Local Power Mode = %s\n", sLocalPowerMode.c_str());
    printf ("Preamble = %s\n", sPreamble.c_str());
    printf ("Inactive Time = %u\n",ui32InactiveTime);
    printf ("Rx Bytes = %u\n",ui32RxBytes);
    printf ("Rx Packets = %u\n",ui32RxPackets);
    printf ("Tx Bytes = %u\n",ui32TxBytes);
    printf ("Tx Packets = %u\n",ui32RxPackets);
    printf ("Tx Retries = %u\n",ui32TxRetries);
    printf ("Tx Failed = %u\n",ui32TxFailed);
    printf ("Signal Power = %u\n",ui8SignalPower);
    printf ("Signa Power Avg = %u\n",ui8SignalPowerAvg);
    printf ("T Offset = %lu\n",ui64TOffset);
    printf ("mesh LLID = %u\n",meshLLID);
    printf ("mesh PLID = %u\n",meshPLID);
    printf ("Authenticated = %s\n",bAuthenticated?"yes":"no");
    printf ("Authorized = %s\n",bAuthorized?"yes":"no");
    printf ("WMM_WME = %s\n",WMM_WME?"yes":"no");
    printf ("MFP = %s\n",MFP?"yes":"no");
    printf ("TDLS = %s\n",TDLS?"yes":"no");

}