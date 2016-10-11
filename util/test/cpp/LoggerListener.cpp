#include "Logger.h"

#include "NLFLib.h"

using namespace NOMADSUtil;

int main (int argc, char **argv)
{
   LoggerNetworkListener l;

   l.init(1306);
   l.start();

   do {
       sleepForMilliseconds (2000);
   } while (l.isRunning());

   printf ("Terminating\n");
}


