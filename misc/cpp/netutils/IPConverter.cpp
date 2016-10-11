#include <stdio.h>

#include "FTypes.h"
#include "InetAddr.h"
#include "NetUtils.h"
#include "NLFLib.h"
#include "StrClass.h"

using namespace NOMADSUtil;

void printUsage (const char *pszProgram)
{
    printf ("Usage: %s <ipAddrAsUint32>\n", pszProgram);
}

int main (int argc, const char *argv[])
{
    if (argc != 2 || argv[1] == NULL) {
        printUsage (argv[0]);
        return 1;
    }

    String addr (argv[1]);

    if (addr.indexOf (".") >= 0) {
        // It dotted form
        InetAddr inet (addr.c_str());
        uint32 ui32Addr = inet.getIPAddress();
        printf ("%s <---> %u\n", addr.c_str(), ui32Addr);
    }
    else {
        // It's a number
        uint32 ui32Addr = atoui32 (addr.c_str());
        const char *pszAddr = NetUtils::ui32Inetoa (ui32Addr);
        printf ("%s <---> %s\n", pszAddr, addr.c_str());
    }

    return 0;
}


