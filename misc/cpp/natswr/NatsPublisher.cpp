#include "NatsWrapper.h"

#include "Logger.h"

#include <string>
#include <iostream>

#define checkAndLogMsg if (pLogger) pLogger->logMsg

using IHMC_MISC_NATS::NatsWrapper;
using NOMADSUtil::Logger;
using NOMADSUtil::pLogger;
using std::string;

namespace NOMADS_NATS_SUBSCRIBER
{
    struct Config
    {
        int port = NatsWrapper::DEFAULT_PORT;
        string addr = "127.0.0.1";
        string topic = "";
        string message = "";
    };

    int parseArgs (int argc, char *argv[], Config &conf)
    {
        for (int i = 1; i < argc; i++) {
            string opt (argv[i]);

            if ((opt == "-b") || (opt == "--broker")) {
                conf.addr = argv[++i];
            }
            else if ((opt == "-p") || (opt == "--port")) {
                conf.port = atoi (argv[++i]);
            }
            else if ((opt == "-m") || (opt == "--message")) {
                conf.message = argv[++i];
            }
            else if ((opt == "-t") || (opt == "--topic")) {
                conf.topic = argv[++i];
            }
            else {
                return -1;
            }
        }
        if ((conf.topic.length () <= 0) || (conf.message.length() <= 0)) {
            return -2;
        }
        return 0;
    }
}

using namespace NOMADS_NATS_SUBSCRIBER;

int main (int argc, char *argv[])
{
    pLogger = new Logger();
    if (pLogger != NULL) {
        pLogger->setDebugLevel (Logger::L_LowDetailDebug);
        pLogger->enableScreenOutput();
    }

    Config cfg;
    if (parseArgs (argc, argv, cfg) < 0) {
        return 1;
    }

    NatsWrapper nats (true);
    if (nats.init (cfg.addr.c_str(), cfg.port) < 0) {
        return 2;
    }

    nats.publish (cfg.topic.c_str(), cfg.message.c_str(), cfg.message.length());
}

