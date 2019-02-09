#include "NatsWrapper.h"

#include "Logger.h"
#include "NLFLib.h"

#include <string>
#include <vector>
#include <sstream>
#include <iostream>

#define checkAndLogMsg if (pLogger) pLogger->logMsg

using IHMC_MISC_NATS::NatsWrapper;
using NOMADSUtil::Logger;
using NOMADSUtil::pLogger;
using NOMADSUtil::sleepForMilliseconds;
using std::string;
using std::vector;
using std::stringstream;

namespace NOMADS_NATS_SUBSCRIBER
{
    struct Config
    {
        int port = NatsWrapper::DEFAULT_PORT;
        string addr = "127.0.0.1";
        vector<string> topics;
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
            else if ((opt == "-t") || (opt == "--topic")) {
                stringstream ss (argv[++i]);
                string topic;
                while (getline (ss, topic, ',')) {
                    conf.topics.push_back (topic);
                }
            }
            else {
                return -1;
            }
        }
        if (conf.topics.size() <= 0) {
            return -2;
        }
        return 0;
    }

    class Listener : public NatsWrapper::Listener
    {
        void messageArrived (const char *pszTopic, const void *pMsg, int iLen)
        {
            /*char *pszMsg = (char *) malloc (iLen + 1);
            if (pszMsg != nullptr) {
                memcpy (pszMsg, pMsg, iLen);
                pszMsg[iLen] = '\0';
                std::cout << pszMsg << std::endl;
                free (pszMsg);
            }
            else {*/
                std::cout << "Received message of topic <" << pszTopic << "> of " << iLen << " Bytes." << std::endl;
            //}
        }
    };
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

    Listener listener;
    for (auto const& topic : cfg.topics) {
        nats.subscribe (topic.c_str(), &listener);
        std::cout << topic << " subscribed." << std::endl;
    }

    do {
        sleepForMilliseconds (5000);
    } while (true);
}

