#include <stdio.h>

#include "net/NetUtils.h"
#include "net/NICInfo.h"

using namespace NOMADSUtil;

int main (int argc, char **argv)
{
    NICInfo **ppNICsInfo = NetUtils::getNICsInfo (false, false);
    if (ppNICsInfo) {
        for (int i = 0; ppNICsInfo[i]; i++) {
            printf ("NetIF[%d]: %s (%s)\n", i, (const char*) ppNICsInfo[i]->toString(), ppNICsInfo[i]->getName().c_str());
        }
		NetUtils::freeNICsInfo (ppNICsInfo);
        return 0;
    }
    return -1;
}

