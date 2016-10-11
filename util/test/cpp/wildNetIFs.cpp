#include "net/NICInfo.h"
#include "net/NetUtils.h"

#include <stdio.h>

using namespace NOMADSUtil;

int main (int argc, char **argv)
{
    if (argc != 2) {
        fprintf (stderr, "usage: %s wildIPAddress\nexample: %s 192.168.*\n", argv[0], argv[0]);
		return -1;
    }

    NICInfo **ppNICsInfo = NetUtils::getNICsInfo (argv[1]);
    for (int i = 0; ppNICsInfo[i]; i++) {
        printf ("matching NIC: %s\n", (const char*) ppNICsInfo[i]->toString());
    }

    // delete the memory allocated by ppNICsInfo
    for (int i = 0; ppNICsInfo[i]; i++) {
        delete ppNICsInfo[i];
    }
    delete[] ppNICsInfo;
    return 0;
}

