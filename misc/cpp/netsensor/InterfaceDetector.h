
#ifndef NETSENSOR_InterfaceDetector__INCLUDED
#define NETSENSOR_InterfaceDetector__INCLUDED

#include <stdio.h>
#include <iostream>
#include "NetSensorConstants.h"
#include "StrClass.h"
#include "StringHashtable.h"
#include "PtrLList.h"

#if defined (WIN32)
#include <winsock2.h>
#include <iphlpapi.h>
#define stricmp _stricmp
#elif defined (UNIX)
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <arpa/inet.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <sys/types.h>
#include <ifaddrs.h>
#define stricmp strcasecmp
#endif

namespace IHMC_NETSENSOR
{
    class InterfaceDetector
    {
    public:
        InterfaceDetector(void);
        ~InterfaceDetector(void);

        void init(void);
        void printUserFriendlyNames(void);
        int getNext(char **ppNextEl);
#if defined (WIN32)
        char *getRawDeviceName   (const char *pszUserFriendlyName);
        char *getUserFriendlyName(const char *pszAdapterName);
    private:
        char *getCharFriendlyName(IP_ADAPTER_ADDRESSES *pCurrAddr);
        void initAdapterAddresses(void);
        void initInterfaceInfo(void);

        bool isValidInterface(IP_ADAPTER_ADDRESSES *pCurrAddr);
        // <---------------------------------------------------------->

        // Used to get user-friendly name (uses IFIndex from _pInterfaceInfo)
        IP_ADAPTER_ADDRESSES *_pAddresses;

        // Used to get non-loopback interfaces
        IP_INTERFACE_INFO *_pInterfaceInfo;

#elif defined(UNIX)
    private:
        ifaddrs *_pAddresses;
#endif
    private:
        void storeFriendlyNames(void);

        NOMADSUtil::PtrLList<char> _llistFriendlyNames;
    };
}
#endif

